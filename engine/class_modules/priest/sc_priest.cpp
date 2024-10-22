// ==========================================================================
// Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"

#include "class_modules/apl/apl_priest.hpp"
#include "sc_enums.hpp"
#include "sim/option.hpp"
#include "tcb/span.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions
{
namespace spells
{
// ==========================================================================
// Expiation
// ==========================================================================
struct expiation_t final : public priest_spell_t
{
  timespan_t consume_time;

  expiation_t( priest_t& p )
    : priest_spell_t( "expiation", p, p.talents.discipline.expiation ),
      consume_time( timespan_t::from_seconds( data().effectN( 2 ).base_value() ) )
  {
    background = dual = true;
    may_crit          = true;
    tick_may_crit     = true;
    // Spell data has this listed as physical, but in-game it's shadow
    school = SCHOOL_SHADOW;

    // TODO: check if this double dips from any multipliers or takes 100% exactly the calculated dot values.
    // also check that the STATE_NO_MULTIPLIER does exactly what we expect.
    snapshot_flags &= ~STATE_NO_MULTIPLIER;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( s );

    if ( priest().talents.discipline.schism.enabled() )
    {
      const priest_td_t* td = priest().find_target_data( s->target );
      if ( td && td->buffs.schism->check() )
      {
        auto adjust_percent = priest().talents.discipline.schism_debuff->effectN( 1 ).percent();
        d *= 1.0 + adjust_percent;
        sim->print_debug( "schism modifies expiation damage by {} (new total: {})", adjust_percent, d );
      }
    }

    return d;
  }

  void impact( action_state_t* s ) override
  {
    priest_td_t& td = get_td( s->target );
    dot_t* dot =
        priest().talents.discipline.purge_the_wicked.enabled() ? td.dots.purge_the_wicked : td.dots.shadow_word_pain;

    auto dot_damage = priest().tick_damage_over_time( consume_time, dot );
    if ( dot_damage > 0 )
    {
      sim->print_debug( "Expiation consumed {} seconds, dealing {}", consume_time, dot_damage );
      base_dd_min = base_dd_max = dot_damage;
      priest_spell_t::impact( s );
      dot->adjust_duration( -consume_time );
    }
    else
    {
      priest().procs.expiation_lost_no_dot->occur();
    }
  }
};

// ==========================================================================
// Mind Blast
// ==========================================================================
struct mind_blast_base_t : public priest_spell_t
{
private:
  timespan_t void_summoner_cdr;
  propagate_const<expiation_t*> child_expiation;

public:
  mind_blast_base_t( priest_t& p, util::string_view options_str, const spell_data_t* s )
    : priest_spell_t( s->name_cstr(), p, s ),
      void_summoner_cdr(
          priest()
              .talents.discipline.void_summoner->effectN( priest().talents.shared.mindbender.enabled() ? 2 : 1 )
              .time_value() ),
      child_expiation( nullptr )
  {
    parse_options( options_str );
    affected_by_shadow_weaving = true;
    cooldown                   = p.cooldowns.mind_blast;
    cooldown->hasted           = true;

    if ( priest().talents.discipline.expiation.enabled() )
    {
      child_expiation             = new expiation_t( priest() );
      child_expiation->background = true;
    }

    // Extra charge of Mind Blast
    apply_affecting_aura( p.talents.shadow.thought_harvester );

    triggers_atonement = true;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.discipline.void_summoner.enabled() )
    {
      priest().cooldowns.fiend->adjust( void_summoner_cdr );
    }

    if ( priest().talents.shadow.mind_melt.enabled() && priest().buffs.mind_melt->check() )
    {
      priest().buffs.mind_melt->expire();
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T29, B2 ) )
    {
      priest().buffs.gathering_shadows->trigger();
    }

    if ( priest().buffs.shadowy_insight->check() )
    {
      priest().buffs.shadowy_insight->decrement();
    }

    if ( priest().specialization() == PRIEST_DISCIPLINE && priest().talents.voidweaver.entropic_rift.enabled() )
    {
      priest().trigger_entropic_rift();
    }
  }

  bool insidious_ire_active() const
  {
    if ( !priest().talents.shadow.insidious_ire.enabled() )
      return false;

    return priest().buffs.insidious_ire->check();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    if ( insidious_ire_active() )
    {
      m *= 1 + priest().talents.shadow.insidious_ire->effectN( 1 ).percent();
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T30, B2 ) && priest().buffs.shadowy_insight->check() )
    {
      m *= 1 + priest().sets->set( PRIEST_SHADOW, T30, B2 )->effectN( 1 ).percent();
    }
    return m;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest_td_t& td = get_td( s->target );

    if ( result_is_hit( s->result ) )
    {
      // Benefit Tracking
      if ( priest().talents.shadow.insidious_ire.enabled() )
      {
        priest().buffs.insidious_ire->up();
      }

      if ( priest().talents.discipline.schism.enabled() )
      {
        td.buffs.schism->trigger();
      }

      if ( priest().talents.shared.inescapable_torment.enabled() )
      {
        priest().trigger_inescapable_torment( s->target );
      }

      if ( priest().talents.shadow.mind_devourer.enabled() &&
           rng().roll( priest().talents.shadow.mind_devourer->effectN( 1 ).percent() ) )
      {
        priest().buffs.mind_devourer->trigger();
        priest().procs.mind_devourer->occur();
      }

      if ( priest().talents.discipline.dark_indulgence.enabled() )
      {
        int stack = priest().buffs.power_of_the_dark_side->check();

        priest().buffs.power_of_the_dark_side->trigger();

        if ( priest().buffs.power_of_the_dark_side->check() == stack )
        {
          priest().procs.power_of_the_dark_side_dark_indulgence_overflow->occur();
        }
      }

      if ( priest().talents.shadow.shadowy_apparitions.enabled() )
      {
        priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_mb, s->result == RESULT_CRIT );
      }

      priest().trigger_psychic_link( s );

      if ( priest().talents.apathy.enabled() && s->result == RESULT_CRIT )
      {
        td.buffs.apathy->trigger();
      }

      if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T30, B2 ) && priest().buffs.shadowy_insight->check() )
      {
        priest().generate_insanity(
            priest().sets->set( PRIEST_SHADOW, T30, B2 )->effectN( 2 ).resource( RESOURCE_INSANITY ),
            priest().gains.insanity_t30_2pc, s->action );
      }

      if ( child_expiation )
      {
        child_expiation->target = s->target;
        child_expiation->execute();
      }
    }
  }

  timespan_t execute_time() const override
  {
    if ( priest().buffs.shadowy_insight->check() )
    {
      return timespan_t::zero();
    }

    return priest_spell_t::execute_time();
  }

  // Called as a part of action execute
  void update_ready( timespan_t cd_duration ) override
  {
    priest().buffs.voidform->up();  // Benefit tracking

    priest_spell_t::update_ready( cd_duration );
  }
};

struct mind_blast_t final : public mind_blast_base_t
{
  double mind_blast_insanity;

  mind_blast_t( priest_t& p, util::string_view options_str )
    : mind_blast_base_t( p, options_str, p.specs.mind_blast ),
      mind_blast_insanity( p.specs.shadow_priest->effectN( 9 ).resource( RESOURCE_INSANITY ) )
  {
    energize_amount = mind_blast_insanity;
  }

  bool action_ready() override
  {
    if ( ( p().buffs.entropic_rift->check() ||
           ( p().channeling && p().channeling->id == p().talents.shadow.void_torrent->id() ) ) &&
         p().talents.voidweaver.void_blast.enabled() && priest().specialization() == PRIEST_SHADOW )
      return false;

    return mind_blast_base_t::action_ready();
  }
};

struct void_blast_shadow_t final : public mind_blast_base_t
{
  void_blast_shadow_t( priest_t& p, util::string_view options_str )
    : mind_blast_base_t( p, options_str, p.talents.voidweaver.void_blast_shadow )
  {
    energize_amount   = -base_costs[ RESOURCE_INSANITY ];
    energize_type     = action_energize::ON_CAST;
    energize_resource = RESOURCE_INSANITY;

    if ( p.talents.voidweaver.void_infusion.enabled() )
    {
      energize_amount *= 1 + p.talents.voidweaver.void_infusion->effectN( 1 ).percent();
    }

    base_costs[ RESOURCE_INSANITY ] = 0;

    if ( cooldown->duration == 0_s )
    {
      new mind_blast_t( p, options_str );
    }
  }

  void execute() override
  {
    mind_blast_base_t::execute();

    if ( priest().talents.voidweaver.darkening_horizon.enabled() )
    {
      priest().extend_entropic_rift();
    }
  }

  bool action_ready() override
  {
    bool can_cast = ( p().buffs.entropic_rift->check() ||
                      ( p().channeling && p().channeling->id == p().talents.shadow.void_torrent->id() ) );

    if ( !can_cast || !p().talents.voidweaver.void_blast.enabled() || priest().specialization() != PRIEST_SHADOW )
      return false;

    return mind_blast_base_t::action_ready();
  }
};

// ==========================================================================
// Angelic Feather
// ==========================================================================
struct angelic_feather_t final : public priest_spell_t
{
  angelic_feather_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "angelic_feather", p, p.talents.angelic_feather )
  {
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( s->target->buffs.angelic_feather )
    {
      s->target->buffs.angelic_feather->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->buffs.angelic_feather )
    {
      return false;
    }

    return priest_spell_t::target_ready( candidate_target );
  }
};

// ==========================================================================
// Divine Star
// Base Spell, used for both heal and damage spell.
// ==========================================================================
struct divine_star_spell_t final : public priest_spell_t
{
  propagate_const<divine_star_spell_t*> return_spell;

  divine_star_spell_t( util::string_view n, priest_t& p, const spell_data_t* s, bool is_return_spell = false )
    : priest_spell_t( n, p, s ),
      return_spell( ( is_return_spell ? nullptr : new divine_star_spell_t( n, p, s, true ) ) )
  {
    aoe = -1;

    proc = background          = true;
    affected_by_shadow_weaving = true;
    triggers_atonement         = true;

    // This is not found in the affected spells for Dark Ascension, overriding it manually
    force_effect( p.buffs.dark_ascension, 1, p.talents.archon.perfected_form );
    // This is not found in the affected spells for Shadow Covenant, overriding it manually
    // Final two params allow us to override the 25% damage buff when twilight corruption is selected (25% -> 35%)
    force_effect( p.buffs.shadow_covenant, 1, IGNORE_STACKS, USE_DEFAULT, p.talents.discipline.twilight_corruption );
  }

  // Hits twice, but only if you are at the correct distance
  // 24 yards or less for 2 hits, 28 yards or less for 1 hit
  void execute() override
  {
    double distance;

    distance = priest_spell_t::player->get_player_distance( *target );

    if ( distance <= 28 )
    {
      priest_spell_t::execute();

      if ( return_spell && distance <= 24 )
      {
        return_spell->execute();
      }
    }
  }
};

struct divine_star_heal_t final : public priest_heal_t
{
  propagate_const<divine_star_heal_t*> return_spell;

  divine_star_heal_t( util::string_view n, priest_t& p, const spell_data_t* s, bool is_return_spell = false )
    : priest_heal_t( n, p, s ), return_spell( ( is_return_spell ? nullptr : new divine_star_heal_t( n, p, s, true ) ) )
  {
    aoe = -1;

    reduced_aoe_targets = p.talents.divine_star->effectN( 1 ).base_value();

    proc = background = true;
    disc_mastery      = true;
  }

  // Hits twice, but only if you are at the correct distance
  // 24 yards or less for 2 hits, 28 yards or less for 1 hit
  // As this is the heal - Always hit.
  void execute() override
  {
    priest_heal_t::execute();

    if ( return_spell )
    {
      return_spell->execute();
    }

    if ( priest().buffs.twist_of_fate_heal_ally_fake->check() )
    {
      priest().buffs.twist_of_fate->trigger();
    }
  }
};

struct divine_star_t final : public priest_spell_t
{
  divine_star_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "divine_star", p, p.talents.divine_star ),
      _heal_spell_holy( new divine_star_heal_t( "divine_star_heal_holy", p, p.talents.divine_star_heal_holy ) ),
      _dmg_spell_holy( new divine_star_spell_t( "divine_star_damage_holy", p, p.talents.divine_star_dmg_holy ) ),
      _heal_spell_shadow( new divine_star_heal_t( "divine_star_heal_shadow", p, p.talents.divine_star_heal_shadow ) ),
      _dmg_spell_shadow( new divine_star_spell_t( "divine_star_damage_shadow", p, p.talents.divine_star_dmg_shadow ) )
  {
    parse_options( options_str );

    dot_duration = base_tick_time = timespan_t::zero();

    add_child( _heal_spell_holy );
    add_child( _dmg_spell_holy );
    add_child( _heal_spell_shadow );
    add_child( _dmg_spell_shadow );
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().specialization() == PRIEST_SHADOW || priest().buffs.shadow_covenant->check() )
    {
      _heal_spell_shadow->execute();
      _dmg_spell_shadow->execute();
    }
    else
    {
      _heal_spell_holy->execute();
      _dmg_spell_holy->execute();
    }
  }

private:
  action_t* _heal_spell_holy;
  action_t* _dmg_spell_holy;
  action_t* _heal_spell_shadow;
  action_t* _dmg_spell_shadow;
};

// ==========================================================================
// Halo
// Base Spell, used for both damage and heal spell.
// ==========================================================================
struct halo_spell_t final : public priest_spell_t
{
  bool returning;
  action_t* return_spell;
  timespan_t prepull_timespent;

  halo_spell_t( util::string_view n, priest_t& p, const spell_data_t* s, bool return_ = false )
    : priest_spell_t( n, p, s ),
      returning( return_ ),
      return_spell( return_ ? nullptr : new halo_spell_t( name_str + "_return", p, s, true ) ),
      prepull_timespent( timespan_t::zero() )
  {
    aoe        = -1;
    background = true;
    radius     = data().max_range() + p.talents.archon.power_surge->effectN( 1 ).base_value();
    range      = 0;
    travel_speed =
        radius / 2;  // These do not seem to match up to the animation, which would be 2.5s. TODO: Check later
    affected_by_shadow_weaving = true;

    // This is not found in the affected spells for Dark Ascension, overriding it manually
    force_effect( p.buffs.dark_ascension, 1, p.talents.archon.perfected_form );
    // This is not found in the affected spells for Shadow Covenant, overriding it manually
    // Final two params allow us to override the 25% damage buff when twilight corruption is selected (25% -> 35%)
    force_effect( p.buffs.shadow_covenant, 1, IGNORE_STACKS, USE_DEFAULT, p.talents.discipline.twilight_corruption );

    triggers_atonement = true;

    if ( return_spell )
    {
      add_child( return_spell );
    }
  }

  timespan_t travel_time() const override
  {
    if ( !returning )
      return priest_spell_t::travel_time() - prepull_timespent;

    if ( travel_speed == 0 && travel_delay == 0 )
      return timespan_t::from_seconds( min_travel_time );

    double t = travel_delay;

    if ( travel_speed > 0 )
    {
      double distance;
      distance = radius - player->get_player_distance( *target );

      if ( distance > 0 )
        t += distance / travel_speed;
    }

    double v = sim->travel_variance;

    if ( v )
      t = rng().gauss( t, v );

    t = std::max( t, min_travel_time );

    return timespan_t::from_seconds( t );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( p().talents.archon.resonant_energy.enabled() )
    {
      auto td = p().get_target_data( s->target );
      if ( td )
      {
        td->buffs.resonant_energy->trigger();
      }
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( p().talents.archon.divine_halo.enabled() && !returning && return_spell )
    {
      make_event( sim, timespan_t::from_seconds( radius / travel_speed ) - prepull_timespent,
                  [ this ] { return_spell->execute(); } );
    }
  }
};

struct halo_heal_t final : public priest_heal_t
{
  bool returning;
  action_t* return_spell;
  timespan_t prepull_timespent;
  halo_heal_t( util::string_view n, priest_t& p, const spell_data_t* s, bool return_ = false )
    : priest_heal_t( n, p, s ),
      returning( return_ ),
      return_spell( return_ ? nullptr : new halo_heal_t( name_str + "_return", p, s, true ) )
  {
    aoe        = -1;
    background = true;
    radius     = data().max_range() + p.talents.archon.power_surge->effectN( 1 ).base_value();
    range      = 0;
    travel_speed =
        radius / 2;  // These do not seem to match up to the animation, which would be 2.5s. TODO: Check later

    reduced_aoe_targets = p.talents.halo->effectN( 1 ).base_value();
    disc_mastery        = true;

    if ( return_spell )
    {
      add_child( return_spell );
    }
  }

  timespan_t travel_time() const override
  {
    if ( !returning )
      return priest_heal_t::travel_time();

    if ( travel_speed == 0 && travel_delay == 0 )
      return timespan_t::from_seconds( min_travel_time );

    double t = travel_delay;

    if ( travel_speed > 0 )
    {
      double distance;
      distance = radius - player->get_player_distance( *target );

      if ( distance > 0 )
        t += distance / travel_speed;
    }

    double v = sim->travel_variance;

    if ( v )
      t = rng().gauss( t, v );

    t = std::max( t, min_travel_time );

    return timespan_t::from_seconds( t );
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().buffs.twist_of_fate_heal_ally_fake->check() )
    {
      priest().buffs.twist_of_fate->trigger();
    }

    if ( p().talents.archon.divine_halo.enabled() && !returning && return_spell )
    {
      make_event( sim, timespan_t::from_seconds( radius / travel_speed ), [ this ] { return_spell->execute(); } );
    }
  }
};

struct halo_t final : public priest_spell_t
{
  timespan_t prepull_timespent;

  halo_t( priest_t& p, util::string_view options_str ) : halo_t( p, false )
  {
    parse_options( options_str );
  }

  halo_t( priest_t& p, bool power_surge = false )
    : priest_spell_t( "halo", p, p.talents.halo ),
      prepull_timespent( timespan_t::zero() ),
      _heal_spell_holy( new halo_heal_t( "halo_heal_holy", p, p.talents.halo_heal_holy ) ),
      _dmg_spell_holy( new halo_spell_t( "halo_damage_holy", p, p.talents.halo_dmg_holy ) ),
      _heal_spell_shadow( new halo_heal_t( "halo_heal_shadow", p, p.talents.halo_heal_shadow ) ),
      _dmg_spell_shadow( new halo_spell_t( "halo_damage_shadow", p, p.talents.halo_dmg_shadow ) )
  {
    add_child( _heal_spell_holy );
    add_child( _dmg_spell_holy );
    add_child( _heal_spell_shadow );
    add_child( _dmg_spell_shadow );

    if ( power_surge )
    {
      background = dual = true;
    }
  }

  timespan_t travel_time() const override
  {
    if ( is_precombat )
      return 1_ms;

    return priest_spell_t::travel_time();
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( is_precombat )
    {
      _heal_spell_holy->prepull_timespent   = prepull_timespent;
      _dmg_spell_holy->prepull_timespent    = prepull_timespent;
      _heal_spell_shadow->prepull_timespent = prepull_timespent;
      _dmg_spell_shadow->prepull_timespent  = prepull_timespent;

      cooldown->adjust( -prepull_timespent );
    }

    if ( priest().specialization() == PRIEST_SHADOW || priest().buffs.shadow_covenant->check() )
    {
      _heal_spell_shadow->execute();
      _dmg_spell_shadow->execute();
    }
    else
    {
      _heal_spell_holy->execute();
      _dmg_spell_holy->execute();
    }

    if ( is_precombat )
    {
      _heal_spell_holy->prepull_timespent   = timespan_t::zero();
      _dmg_spell_holy->prepull_timespent    = timespan_t::zero();
      _heal_spell_shadow->prepull_timespent = timespan_t::zero();
      _dmg_spell_shadow->prepull_timespent  = timespan_t::zero();
    }

    if ( priest().talents.archon.power_surge.enabled() && !background )
    {
      priest().buffs.power_surge->trigger();

      if ( is_precombat )
      {

        // TODO: Handle very early precombat
        priest().buffs.power_surge->extend_duration( player, -prepull_timespent );


        if (priest().buffs.power_surge->check())
        {
          auto when = -( prepull_timespent % priest().buffs.power_surge->tick_time() );

          priest().buffs.power_surge->reschedule_tick( when );
        }
      }
    }

    if ( priest().talents.archon.sustained_potency.enabled() )
    {
      bool extended = false;
      if ( priest().buffs.voidform->check() )
      {
        extended = true;
        priest().buffs.voidform->extend_duration( player, 1_s );
      }
      if ( priest().buffs.dark_ascension->check() )
      {
        extended = true;
        priest().buffs.dark_ascension->extend_duration( player, 1_s );
      }
      if ( !extended )
      {
        priest().buffs.sustained_potency->trigger();
      }
    }

    if ( priest().talents.archon.manifested_power.enabled() )
    {
      // TODO: check what happens if not talented into these
      switch ( priest().specialization() )
      {
        case PRIEST_HOLY:
          // surge_of_light NYI
          break;
        case PRIEST_SHADOW:
          // You get a full buff of MSI or MFI and keep Surge of Insanity state intact
          if ( priest().talents.shadow.mind_spike.enabled() )
          {
            priest().buffs.mind_spike_insanity->trigger();
          }
          else
          {
            priest().buffs.mind_flay_insanity->trigger();
          }
        default:
          break;
      }
    }
  }

private:
  propagate_const<halo_heal_t*> _heal_spell_holy;
  propagate_const<halo_spell_t*> _dmg_spell_holy;
  propagate_const<halo_heal_t*> _heal_spell_shadow;
  propagate_const<halo_spell_t*> _dmg_spell_shadow;
};  // namespace spells

// ==========================================================================
// Levitate
// ==========================================================================
struct levitate_t final : public priest_spell_t
{
  levitate_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "levitate", p, p.find_class_spell( "Levitate" ) )
  {
    parse_options( options_str );

    ignore_false_positive = true;
    harmful               = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.levitate->trigger();
  }
};

// ==========================================================================
// Power Word: Fortitude
// ==========================================================================
struct power_word_fortitude_t final : public priest_spell_t
{
  power_word_fortitude_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "power_word_fortitude", p, p.find_class_spell( "Power Word: Fortitude" ) )
  {
    parse_options( options_str );
    harmful               = false;
    ignore_false_positive = true;

    background = sim->overrides.power_word_fortitude != 0;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( !sim->overrides.power_word_fortitude )
    {
      sim->auras.power_word_fortitude->trigger();
    }
  }
};

// ==========================================================================
// Smite
// ==========================================================================

struct smite_base_t : public priest_spell_t
{
  timespan_t void_summoner_cdr;
  timespan_t train_of_thought_cdr;
  timespan_t t31_2pc_extend;
  propagate_const<action_t*> child_holy_fire;
  action_t* child_searing_light;

  smite_base_t( priest_t& p, util::string_view name, const spell_data_t* s, bool bg = false,
                util::string_view options_str = {} )
    : priest_spell_t( name, p, s ),
      void_summoner_cdr(
          priest()
              .talents.discipline.void_summoner->effectN( priest().talents.shared.mindbender.enabled() ? 2 : 1 )
              .time_value() ),
      train_of_thought_cdr( priest().talents.discipline.train_of_thought->effectN( 2 ).time_value() ),
      t31_2pc_extend( priest().sets->set( PRIEST_DISCIPLINE, T31, B2 )->effectN( 1 ).time_value() ),
      child_holy_fire( priest().background_actions.holy_fire ),
      child_searing_light( priest().background_actions.searing_light )

  {
    background = bg;
    if ( !background )
    {
      parse_options( options_str );
      if ( priest().talents.holy.divine_word.enabled() )
      {
        child_holy_fire->background = true;
      }
    }

    triggers_atonement = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( s );
    if ( priest().talents.holy.searing_light.enabled() )
    {
      const priest_td_t* td = priest().find_target_data( s->target );
      if ( td && td->dots.holy_fire->is_ticking() )
      {
        auto adjust_percent = priest().talents.holy.searing_light->effectN( 1 ).percent();
        d *= 1.0 + adjust_percent;
        sim->print_debug( "searing_light modifies Smite damage by {} (new total: {})", adjust_percent, d );
      }
    }

    return d;
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = priest_spell_t::execute_time_pct_multiplier();

    if ( priest().talents.unwavering_will.enabled() &&
         priest().health_percentage() > priest().talents.unwavering_will->effectN( 2 ).base_value() )
    {
      mul *= 1 + priest().talents.unwavering_will->effectN( 1 ).percent();
    }

    return mul;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.weal_and_woe->expire();

    if ( priest().talents.discipline.void_summoner.enabled() )
    {
      priest().cooldowns.fiend->adjust( void_summoner_cdr );
    }

    if ( priest().talents.discipline.train_of_thought.enabled() )
    {
      priest().cooldowns.penance->adjust( train_of_thought_cdr );
    }
    // If we have divine word, have triggered divine favor: chastise, and proc the holy fire effect
    if ( child_holy_fire && priest().talents.holy.divine_word.enabled() &&
         priest().buffs.divine_favor_chastise->check() &&
         rng().roll( priest().talents.holy.divine_favor_chastise->effectN( 3 ).percent() ) )
    {
      priest().procs.divine_favor_chastise->occur();
      child_holy_fire->execute();
    }

    // T31 Background set bonus triggers the shadow amp. Core logic ignores background spell execution so manually
    // trigger this here.
    if ( priest().talents.discipline.twilight_equilibrium.enabled() && background )
    {
      priest().buffs.twilight_equilibrium_shadow_amp->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( priest().sets->has_set_bonus( PRIEST_DISCIPLINE, T31, B2 ) )
      {
        if ( p().allies_with_atonement.size() > 0 )
        {
          /*auto it = *( std::min_element( p().allies_with_atonement.begin(), p().allies_with_atonement.end(),
                                         [ this ]( player_t* a, player_t* b ) {
                                           return a->health_percentage() < b->health_percentage() &&
                                                  priest().find_target_data( b )->buffs.atonement->remains() < 30_s;
                                         } ) );*/

          auto idx = rng().range( 0U, as<unsigned>( p().allies_with_atonement.size() ) );

          auto it = p().allies_with_atonement[ idx ];

          auto atone = priest().find_target_data( it )->buffs.atonement;
          if ( atone->remains() < 30_s )
          {
            atone->extend_duration( player, t31_2pc_extend );
          }
        }
      }

      if ( priest().talents.holy.holy_word_chastise.enabled() )
      {
        timespan_t chastise_cdr =
            timespan_t::from_seconds( priest().talents.holy.holy_word_chastise->effectN( 2 ).base_value() );
        if ( priest().buffs.apotheosis->check() || priest().buffs.answered_prayers->check() )
        {
          chastise_cdr *= ( 1 + priest().talents.holy.apotheosis->effectN( 1 ).percent() );
        }
        if ( priest().talents.holy.light_of_the_naaru.enabled() )
        {
          chastise_cdr *= ( 1 + priest().talents.holy.light_of_the_naaru->effectN( 1 ).percent() );
        }
        sim->print_debug( "{} adjusted cooldown of Chastise, by {}, with light_of_the_naaru: {}, apotheosis: {}",
                          priest(), chastise_cdr, priest().talents.holy.light_of_the_naaru.enabled(),
                          ( priest().buffs.apotheosis->check() || priest().buffs.answered_prayers->check() ) );

        priest().cooldowns.holy_word_chastise->adjust( -chastise_cdr );
      }
      if ( child_searing_light && priest().buffs.divine_image->check() )
      {
        for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
        {
          child_searing_light->execute();
        }
      }
    }
  }
};

struct smite_t final : public smite_base_t
{
  smite_base_t* shadow_smite;

  smite_t( priest_t& p, util::string_view options_str )
    : smite_base_t( p, "smite", p.find_class_spell( "Smite" ), false, options_str ), shadow_smite( nullptr )
  {
    if ( p.sets->has_set_bonus( PRIEST_DISCIPLINE, T31, B4 ) )
    {
      shadow_smite = new smite_base_t( p, "smite_t31", priest().specs.smite_t31, true );
      add_child( shadow_smite );
    }
  }

  void impact( action_state_t* s ) override
  {
    smite_base_t::impact( s );

    if ( shadow_smite && priest().buffs.shadow_covenant->up() )
    {
      make_event( sim, 200_ms, [ this, t = s->target ] { shadow_smite->execute_on_target( t ); } );
    }
  }
};

// ==========================================================================
// Power Infusion
// ==========================================================================
struct power_infusion_t final : public priest_spell_t
{
  double power_infusion_magnitude;
  power_infusion_t( priest_t& p, util::string_view options_str, util::string_view name )
    : priest_spell_t( name, p, p.talents.power_infusion ),
      power_infusion_magnitude( player->buffs.power_infusion->default_value +
                                p.talents.archon.concentrated_infusion->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    // Trigger PI on the actor only if casting on itself
    if ( priest().options.self_power_infusion || priest().talents.twins_of_the_sun_priestess.enabled() )
    {
      player->buffs.power_infusion->trigger( 1, power_infusion_magnitude );
    }
  }
};

// ==========================================================================
// Mindgames
// ==========================================================================
struct mindgames_healing_reversal_t final : public priest_spell_t
{
  mindgames_healing_reversal_t( priest_t& p )
    : priest_spell_t( "mindgames_healing_reversal", p, p.talents.pvp.mindgames_healing_reversal )
  {
    background        = true;
    may_crit          = false;
    energize_type     = action_energize::NONE;  // disable insanity gain (parent spell covers this)
    energize_amount   = 0;
    energize_resource = RESOURCE_NONE;

    // Formula found in parent spelldata for $healing
    // $healing=${($SPS*$s5/100)*(1+$@versadmg)*$m3/100}
    spell_power_mod.direct = ( priest().talents.pvp.mindgames->effectN( 5 ).base_value() / 100 ) *
                             ( priest().talents.pvp.mindgames->effectN( 3 ).base_value() / 100 );
  }
};

struct mindgames_damage_reversal_t final : public priest_heal_t
{
  mindgames_damage_reversal_t( priest_t& p )
    : priest_heal_t( "mindgames_damage_reversal", p, p.talents.pvp.mindgames_damage_reversal )
  {
    background        = true;
    harmful           = false;
    may_crit          = false;
    energize_type     = action_energize::NONE;  // disable insanity gain (parent spell covers this)
    energize_amount   = 0;
    energize_resource = RESOURCE_NONE;

    // Formula found in parent spelldata for $damage
    // $damage=${($SPS*$s2/100)*(1+$@versadmg)*$m3/100}
    spell_power_mod.direct = ( priest().talents.pvp.mindgames->effectN( 2 ).base_value() / 100 ) *
                             ( priest().talents.pvp.mindgames->effectN( 3 ).base_value() / 100 );
  }
};

struct mindgames_t final : public priest_spell_t
{
  propagate_const<mindgames_healing_reversal_t*> child_mindgames_healing_reversal;
  propagate_const<mindgames_damage_reversal_t*> child_mindgames_damage_reversal;
  propagate_const<action_t*> child_searing_light;

  mindgames_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mindgames", p, p.talents.pvp.mindgames ),
      child_mindgames_healing_reversal( nullptr ),
      child_mindgames_damage_reversal( nullptr ),
      child_searing_light( priest().background_actions.searing_light )
  {
    parse_options( options_str );

    affected_by_shadow_weaving = true;
    triggers_atonement         = true;

    if ( priest().options.mindgames_healing_reversal )
    {
      child_mindgames_healing_reversal = new mindgames_healing_reversal_t( priest() );
      add_child( child_mindgames_healing_reversal );
    }

    if ( priest().options.mindgames_damage_reversal )
    {
      child_mindgames_damage_reversal = new mindgames_damage_reversal_t( priest() );
      add_child( child_mindgames_damage_reversal );
    }
  }

  void execute() override
  {
    priest_spell_t::execute();
    if ( child_searing_light && priest().buffs.divine_image->check() )
    {
      for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
      {
        child_searing_light->execute();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // Healing reversal creates damage
    if ( child_mindgames_healing_reversal )
    {
      child_mindgames_healing_reversal->target = s->target;
      child_mindgames_healing_reversal->execute();
    }
    // Damage reversal creates healing
    if ( child_mindgames_damage_reversal )
    {
      child_mindgames_damage_reversal->execute();
    }

    if ( priest().specialization() == PRIEST_SHADOW )
    {
      if ( priest().shadow_weaving_active_dots( target, id ) != 3 )
      {
        priest().procs.mindgames_casts_no_mastery->occur();
      }

      if ( result_is_hit( s->result ) )
      {
        priest().trigger_psychic_link( s );
      }
    }
  }
};

// ==========================================================================
// Summon Shadowfiend
//
// Summon Mindbender
// Shadow - 200174 (base effect 2 value)
// Holy/Discipline - 123040 (base effect 3 value)
// ==========================================================================
struct summon_fiend_t final : public priest_spell_t
{
  timespan_t default_duration;
  spawner::pet_spawner_t<pet_t, priest_t>* spawner;

  std::string pet_name( priest_t& p )
  {
    if ( p.talents.voidweaver.voidwraith.enabled() )
      return "voidwraith";

    return p.talents.shared.mindbender.enabled() ? "mindbender" : "shadowfiend";
  }

  spawner::pet_spawner_t<pet_t, priest_t>* pet_spawner( priest_t& p )
  {
    if ( p.talents.voidweaver.voidwraith.enabled() )
      return &p.pets.voidwraith;

    return p.talents.shared.mindbender.enabled() ? &p.pets.mindbender : &p.pets.shadowfiend;
  }

  const spell_data_t* pet_summon_spell( priest_t& p )
  {
    if ( p.talents.voidweaver.voidwraith.enabled() )
      return p.talents.voidweaver.voidwraith_spell;

    return p.talents.shared.mindbender.enabled() ? p.talents.shared.mindbender : p.talents.shadowfiend;
  }

  summon_fiend_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( pet_name( p ), p, pet_summon_spell( p ) ),
      default_duration( data().duration() ),
      spawner( pet_spawner( p ) )
  {
    parse_options( options_str );
    harmful = false;

    if ( p.talents.voidweaver.voidwraith.ok() && p.talents.shared.mindbender.ok() )
    {
      cooldown->duration = p.talents.shared.mindbender->cooldown();
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( spawner )
      spawner->spawn( default_duration );

    if ( priest().talents.discipline.shadow_covenant.enabled() )
    {
      priest().buffs.shadow_covenant->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    if ( priest().talents.shadow.idol_of_yshaarj.enabled() )
    {
      make_event( sim, [ this, s ] { priest().trigger_idol_of_yshaarj( s->target ); } );
    }
  }
};

// ==========================================================================
// Echoing Void
// TODO: move to sc_priest_shadow.cpp
// ==========================================================================
struct echoing_void_demise_t final : public priest_spell_t
{
  int stacks;

  echoing_void_demise_t( priest_t& p )
    : priest_spell_t( "echoing_void_demise", p, p.talents.shadow.echoing_void ), stacks( 1 )
  {
    background          = true;
    proc                = false;
    callbacks           = true;
    may_miss            = false;
    harmful             = false;
    aoe                 = -1;
    range               = data().effectN( 1 ).radius_max();
    reduced_aoe_targets = data().effectN( 2 ).base_value();

    affected_by_shadow_weaving = true;
  }

  // Demise action does not hit the target we are triggering it on, only using it for the proper radius
  std::vector<player_t*>& target_list() const override
  {
    target_cache.is_valid = false;

    std::vector<player_t*>& tl = priest_spell_t::target_list();

    range::erase_remove( tl, target );

    return tl;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    m *= stacks;

    return m;
  }

  // Demise trigger for Idol of N'Zoth
  // When something dies that has stacks it explodes around that target
  // based on how many stacks it has. 10 stacks == 1 explosion 10x more powerful
  void trigger( player_t* target, int trigger_stacks )
  {
    if ( trigger_stacks == 0 )
    {
      return;
    }

    stacks = trigger_stacks;
    sim->print_debug( "{} dies. Triggering {} stacks of echoing_void", target->name(), trigger_stacks );
    set_target( target );
    execute();
  }
};

struct echoing_void_t final : public priest_spell_t
{
  stats_t* child_action_stats;

  echoing_void_t( priest_t& p )
    : priest_spell_t( "echoing_void", p, p.talents.shadow.echoing_void ), child_action_stats( nullptr )
  {
    background          = true;
    proc                = false;
    callbacks           = true;
    may_miss            = false;
    aoe                 = -1;
    range               = data().effectN( 1 ).radius_max();
    reduced_aoe_targets = data().effectN( 2 ).base_value();

    affected_by_shadow_weaving = true;

    child_action_stats = priest().get_stats( "echoing_void_demise" );
    if ( child_action_stats )
    {
      stats->add_child( child_action_stats );
    }
  }
};

// ==========================================================================
// Fade
// ==========================================================================
struct fade_t final : public priest_spell_t
{
  fade_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "fade", p, p.specs.fade )
  {
    parse_options( options_str );
    harmful = false;

    // Reduces CD of Fade
    apply_affecting_aura( p.talents.improved_fade );
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.fade->trigger();
  }
};

// ==========================================================================
// Shadow Word: Death
// https://github.com/simulationcraft/simc/wiki/Priests#shadow-word-death
// ==========================================================================
struct shadow_word_death_self_damage_t final : public priest_spell_t
{
  double self_damage_coeff;
  int parent_chain_number = 0;

  shadow_word_death_self_damage_t( priest_t& p )
    : priest_spell_t( "shadow_word_death_self_damage", p, p.specs.shadow_word_death_self_damage ),
      self_damage_coeff( p.talents.shadow_word_death->effectN( 6 ).percent() )
  {
    background = true;
    may_crit   = false;
    may_miss   = false;
    target     = player;
  }

  double base_da_min( const action_state_t* ) const override
  {
    return player->resources.max[ RESOURCE_HEALTH ] * self_damage_coeff;
  }

  double base_da_max( const action_state_t* ) const override
  {
    return player->resources.max[ RESOURCE_HEALTH ] * self_damage_coeff;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    if ( priest().talents.tithe_evasion.enabled() )
    {
      m *= priest().talents.tithe_evasion->effectN( 1 ).percent();
    }

    // Chained Shadow Word: Deaths only hit the character for 10% of what they normally do.
    // TODO: Unsure if a bug or not
    if ( parent_chain_number > 0 )
    {
      m *= .1;
    }

    return m;
  }

  void trigger( int chain_number )
  {
    parent_chain_number = chain_number;
    execute();
  }

  void execute() override
  {
    base_t::execute();

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T31, B4 ) )
    {
      p().buffs.deaths_torment->trigger();
    }
  }

  void init() override
  {
    base_t::init();

    // We don't want this counted towards our dps
    stats->type = stats_e::STATS_NEUTRAL;

    snapshot_flags |= STATE_MUL_DA;
  }

  proc_types proc_type() const override
  {
    return PROC1_ANY_DAMAGE_TAKEN;
  }
};

struct shadow_word_death_t final : public priest_spell_t
{
protected:
  struct swd_data
  {
    int chain_number  = 0;
    int max_chain     = 2;
    bool deathspeaker = false;
  };
  using state_t = priest_action_state_t<swd_data>;
  using ab      = priest_spell_t;

public:
  double execute_percent;
  double execute_modifier;
  double deathspeaker_mult;
  propagate_const<shadow_word_death_self_damage_t*> shadow_word_death_self_damage;
  timespan_t depth_of_shadows_duration;
  double depth_of_shadows_threshold;
  propagate_const<expiation_t*> child_expiation;
  action_t* child_searing_light;
  timespan_t execute_override;

  shadow_word_death_t( priest_t& p, timespan_t execute_override = timespan_t::min() )
    : ab( "shadow_word_death", p, p.talents.shadow_word_death ),
      execute_percent( data().effectN( 3 ).base_value() ),
      execute_modifier( data().effectN( 4 ).percent() + priest().specs.shadow_priest->effectN( 25 ).percent() ),
      deathspeaker_mult(
          p.talents.shadow.deathspeaker.enabled() ? 1 + p.buffs.deathspeaker->data().effectN( 2 ).percent() : 1.0 ),
      shadow_word_death_self_damage( new shadow_word_death_self_damage_t( p ) ),
      depth_of_shadows_duration(
          timespan_t::from_seconds( p.talents.voidweaver.depth_of_shadows->effectN( 1 ).base_value() ) ),
      depth_of_shadows_threshold( p.talents.voidweaver.depth_of_shadows->effectN( 2 ).base_value() ),
      child_expiation( nullptr ),
      child_searing_light( priest().background_actions.searing_light ),
      execute_override( execute_override )
  {
    affected_by_shadow_weaving = true;

    if ( priest().talents.discipline.expiation.enabled() )
    {
      child_expiation             = new expiation_t( priest() );
      child_expiation->background = true;
    }

    if ( priest().specialization() == PRIEST_SHADOW )
    {
      energize_type     = action_energize::ON_CAST;
      energize_resource = RESOURCE_INSANITY;
      energize_amount   = priest().specs.shadow_priest->effectN( 23 ).resource( RESOURCE_INSANITY );
    }

    spell_power_mod.direct = data().effectN( 2 ).sp_coeff();

    triggers_atonement = true;

    // Devour Matter gives you more Insanity and an extra amount of sp coeff
    // TODO: Refactor this into an additional hit (same spell ID)
    // This additional hit has an independet crit chance, gets the execute mod, and gets the deathspeaker mod
    if ( priest().options.force_devour_matter && priest().talents.voidweaver.devour_matter.enabled() )
    {
      energize_amount += priest().talents.voidweaver.devour_matter->effectN( 3 ).base_value();
      spell_power_mod.direct += data().effectN( 1 ).sp_coeff();
    }
  }

  shadow_word_death_t( priest_t& p, util::string_view options_str ) : shadow_word_death_t( p )
  {
    parse_options( options_str );
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
  }

  timespan_t execute_time() const override
  {
    if ( execute_override > timespan_t::min() )
      return execute_override;

    return ab::execute_time();
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    if ( cast_state( s )->chain_number == 0 )
      cast_state( s )->deathspeaker = p().buffs.deathspeaker->check();

    ab::snapshot_state( s, rt );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_da_multiplier( s );

    if ( cast_state( s )->chain_number > 0 )
    {
      m *= priest().sets->set( PRIEST_SHADOW, T31, B2 )->effectN( 3 ).percent();
    }

    if ( s->target->health_percentage() < execute_percent || cast_state( s )->deathspeaker )
    {
      if ( sim->debug )
      {
        sim->print_debug( "{} below {}% HP. Increasing {} damage by {}%", s->target->name_str, execute_percent, *this,
                          execute_modifier * 100 );
      }
      m *= 1 + execute_modifier;
    }

    if ( cast_state( s )->deathspeaker )
    {
      m *= deathspeaker_mult;
    }

    return m;
  }

  void execute() override
  {
    ab::execute();

    if ( priest().talents.death_and_madness.enabled() )
    {
      // Cooldown is reset only if you have't already gotten a reset
      if ( !priest().buffs.death_and_madness_reset->check() )
      {
        if ( target->health_percentage() <= execute_percent && !priest().buffs.deathspeaker->check() )
        {
          priest().buffs.death_and_madness_reset->trigger();
          cooldown->reset( false );
        }
      }
    }

    if ( priest().talents.shadow.deathspeaker.enabled() )
    {
      priest().buffs.deathspeaker->expire();
    }
  }

  void reset() override
  {
    // Reset max charges to initial value, since it can get out of sync when previous iteration ends with charge-giving
    // buffs up. Do this before calling reset as that will also reset the cooldown.
    if ( priest().specialization() == PRIEST_SHADOW )
    {
      cooldown->charges = data().charges();
    }

    ab::reset();
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( priest().talents.shared.inescapable_torment.enabled() )
    {
      auto mod = 1.0;
      if ( cast_state( s )->chain_number > 0 )
      {
        mod *= priest().sets->set( PRIEST_SHADOW, T31, B2 )->effectN( 3 ).percent();
      }
      priest().trigger_inescapable_torment( s->target, cast_state( s )->chain_number > 0, mod );
    }

    if ( result_is_hit( s->result ) )
    {
      double save_health_percentage = s->target->health_percentage();

      if ( priest().talents.voidweaver.depth_of_shadows.enabled() )
      {
        // TODO: Find out the chance. Placeholder value of 90%. It is not 100% but it is is extremely high.
        if ( ( priest().buffs.deathspeaker->check() || save_health_percentage <= depth_of_shadows_threshold ) &&
             rng().roll( 0.9 ) )
        {
          priest().procs.depth_of_shadows->occur();
          priest().get_current_main_pet().spawn( depth_of_shadows_duration );
        }
      }

      if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T31, B2 ) )
      {
        int number_of_chains;
        state_t* curr_state = cast_state( s );
        if ( curr_state->chain_number > 0 )
        {
          number_of_chains = curr_state->max_chain;
        }
        else
        {
          number_of_chains = as<int>( priest().sets->set( PRIEST_SHADOW, T31, B2 )->effectN( 1 ).base_value() );
          // Chains amount differs if you have a Deathspeaker proc or while in execute but you still keep the modifier
          if ( priest().buffs.deathspeaker->check() || s->target->health_percentage() < execute_percent )
          {
            number_of_chains += as<int>( priest().sets->set( PRIEST_SHADOW, T31, B2 )->effectN( 2 ).base_value() );
          }
        }

        sim->print_debug( "{} shadow_word_death_state: chain_number: {}, max_chain: {}", player->name(),
                          curr_state->chain_number, number_of_chains );

        if ( curr_state->chain_number < curr_state->max_chain )
        {
          shadow_word_death_t* child_death = priest().background_actions.shadow_word_death.get();
          state_t* state                   = child_death->cast_state( child_death->get_state() );
          state->target                    = s->target;
          state->chain_number              = curr_state->chain_number + 1;
          state->deathspeaker              = curr_state->deathspeaker;
          state->max_chain                 = number_of_chains;

          child_death->snapshot_state( state, child_death->amount_type( state ) );

          child_death->schedule_execute( state );
        }
      }

      // TODO: Add in a custom buff that checks after 1 second to see if the target SWD was cast on is now dead.
      if ( !( ( save_health_percentage > 0.0 ) && ( s->target->health_percentage() <= 0.0 ) ) )
      {
        // target is not killed, self damage happens 1s later
        make_event( sim, 1_s,
                    [ this, s ] { shadow_word_death_self_damage->trigger( cast_state( s )->chain_number ); } );
      }

      if ( priest().talents.death_and_madness.enabled() )
      {
        priest_td_t& td = get_td( s->target );
        td.buffs.death_and_madness_debuff->trigger();
      }

      if ( priest().specialization() == PRIEST_SHADOW )
      {
        if ( result_is_hit( s->result ) )
        {
          priest().trigger_psychic_link( s );
        }
      }

      if ( child_expiation )
      {
        child_expiation->target = s->target;
        child_expiation->execute();
      }

      if ( child_searing_light && priest().buffs.divine_image->check() )
      {
        for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
        {
          child_searing_light->execute();
        }
      }
    }
  }
};

// ==========================================================================
// Holy Nova
// ==========================================================================
struct holy_nova_heal_t final : public priest_heal_t
{
  holy_nova_heal_t( util::string_view n, priest_t& p ) : priest_heal_t( n, p, p.talents.holy_nova_heal )
  {
    aoe        = -1;
    background = true;

    reduced_aoe_targets = p.talents.holy_nova->effectN( 2 ).base_value();
    disc_mastery        = true;
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().buffs.twist_of_fate_heal_ally_fake->check() )
    {
      priest().buffs.twist_of_fate->trigger();
    }
  }
};

struct holy_nova_t final : public priest_spell_t
{
  action_t* child_light_eruption;
  action_t* child_heal;
  holy_nova_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "holy_nova", p, p.talents.holy_nova ),
      child_light_eruption( priest().background_actions.light_eruption )
  {
    parse_options( options_str );
    aoe                 = -1;
    full_amount_targets = as<int>( priest().talents.holy_nova->effectN( 3 ).base_value() );
    reduced_aoe_targets = priest().talents.holy_nova->effectN( 3 ).base_value();
    triggers_atonement  = true;

    child_heal = new holy_nova_heal_t( "holy_nova_heal", p );
    add_child( child_heal );
  }

  void execute() override
  {
    priest_spell_t::execute();

    child_heal->execute();

    if ( priest().talents.rhapsody )
    {
      priest().buffs.rhapsody_timer->trigger();

      if ( priest().buffs.rhapsody->check() )
      {
        make_event( sim, 0_ms, [ this ]() { priest().buffs.rhapsody->expire(); } );
      }
    }

    if ( child_light_eruption && priest().buffs.divine_image->check() )
    {
      for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
      {
        child_light_eruption->execute();
      }
    }
  }
};

// ==========================================================================
// Psychic Scream
// ==========================================================================
struct psychic_scream_t final : public priest_spell_t
{
  psychic_scream_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "psychic_scream", p, p.specs.psychic_scream )
  {
    parse_options( options_str );
    aoe      = -1;
    may_miss = may_crit   = false;
    ignore_false_positive = true;

    // CD reduction
    apply_affecting_aura( p.talents.psychic_voice );
  }

  void execute() override
  {
    priest_spell_t::execute();

    // NOTE: This is basically a totem/pet that can currently be healed/take actions
    // If useful consider refactoring
    if ( priest().talents.archon.incessant_screams.enabled() )
    {
      make_event( sim, timespan_t::from_seconds( priest().talents.archon.incessant_screams->effectN( 1 ).base_value() ),
                  [ this ] { priest_spell_t::execute(); } );
    }
  }
};

// ==========================================================================
// Collapsing Void
// ==========================================================================
struct collapsing_void_damage_t final : public priest_spell_t
{
  int parent_stacks;
  collapsing_void_damage_t( priest_t& p )
    : priest_spell_t( "collapsing_void", p, p.talents.voidweaver.collapsing_void_damage ), parent_stacks( 0 )
  {
    affected_by_shadow_weaving = true;

    aoe              = -1;
    radius           = data().effectN( 1 ).radius_max();
    split_aoe_damage = 1;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    if ( parent_stacks > 0 )
    {
      m *= 1.0 + ( parent_stacks * priest().talents.voidweaver.collapsing_void->effectN( 3 ).percent() );
    }

    return m;
  }

  void trigger( player_t* target, int stacks )
  {
    // The first trigger of the buff on the spawn of the rift does not count towards the damage mod stacks
    // Only relevant if you didn't extend the rift at all while active
    parent_stacks = stacks - 1;

    player->sim->print_debug( "{} triggered collapsing_void_damage on target {} with {} stacks", priest(), *target,
                              parent_stacks );

    // TODO: Handle if the target dies between entropic rift start and collapsing void
    // Make sure the target is still available
    if ( this->target_ready( target ) )
    {
      set_target( target );
    }

    execute();
  }
};

// ==========================================================================
// Entropic Rift
// ==========================================================================
struct entropic_rift_damage_t final : public priest_spell_t
{
  double base_radius;
  entropic_rift_damage_t( priest_t& p )
    : priest_spell_t( "entropic_rift", p, p.talents.voidweaver.entropic_rift_damage ),
      base_radius( p.talents.voidweaver.entropic_rift_object->effectN( 1 ).radius_max() / 2 )
  {
    aoe        = -1;
    background = dual = true;
    radius            = base_radius;

    affected_by_shadow_weaving = true;
  }

  double miss_chance( double hit, player_t* t ) const override
  {
    double m = priest_spell_t::miss_chance( hit, t );

    if ( priest().options.entropic_rift_miss_percent > 0.0 )
    {
      // Double miss_percent when fighting more than 2 targets
      double miss_percent = priest().options.entropic_rift_miss_percent;
      if ( target_list().size() > 2 )
      {
        miss_percent = miss_percent * 2;
      }

      sim->print_debug( "entropic_rift_damage sets miss_chance to {} with target count: {}", miss_percent,
                        target_list().size() );

      m = miss_percent;
    }

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    // The initial stack does not count for increasing damage
    // TODO: use the buff data better
    double mod = ( priest().buffs.collapsing_void->check() - 1 ) * priest().buffs.collapsing_void->default_value;
    m *= 1.0 + mod;

    return m;
  }

  void execute() override
  {
    double size_increase_mod = priest().bugs ? 0.5 : 1.0;
    radius                   = base_radius + ( size_increase_mod * priest().buffs.collapsing_void->stack() );
    target_cache.is_valid    = false;

    priest_spell_t::execute();
  }
};

struct entropic_rift_t final : public priest_spell_t
{
  entropic_rift_t( priest_t& p ) : priest_spell_t( "entropic_rift", p, p.talents.voidweaver.entropic_rift )
  {
    min_travel_time = 3;
  }

  timespan_t travel_time() const override
  {
    timespan_t t = priest_spell_t::travel_time();

    return t;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.entropic_rift->trigger();

    // Keep track of this for collapsing void
    priest().state.last_entropic_rift_target = target;

    if ( priest().talents.voidweaver.voidheart.enabled() )
    {
      priest().buffs.voidheart->trigger();
    }

    if ( priest().talents.voidweaver.void_empowerment.enabled() )
    {
      switch ( priest().specialization() )
      {
        case PRIEST_SHADOW:
          priest().buffs.mind_devourer->trigger();
          priest().procs.mind_devourer->occur();
          break;
        case PRIEST_DISCIPLINE:
          priest().buffs.void_empowerment->trigger();
          break;
        default:
          break;
      }
    }
  }
};

}  // namespace spells

namespace heals
{
// ==========================================================================
// Flash Heal
// ==========================================================================
struct flash_heal_t final : public priest_heal_t
{
  timespan_t atonement_duration;
  timespan_t train_of_thought_cdr;
  flash_heal_t* binding_heals;
  double binding_heal_percent;
  bool binding;

  flash_heal_t( priest_t& p, util::string_view options_str ) : flash_heal_t( p, "flash_heal", options_str )
  {
  }

  flash_heal_t( priest_t& p, util::string_view name, util::string_view options_str, bool bind = false )
    : priest_heal_t( name, p, p.find_class_spell( "Flash Heal" ) ),
      atonement_duration( timespan_t::from_seconds( p.talents.discipline.atonement_buff->effectN( 3 ).base_value() ) ),
      train_of_thought_cdr( priest().talents.discipline.train_of_thought->effectN( 1 ).time_value() ),
      binding_heal_percent( p.talents.binding_heals->effectN( 1 ).percent() ),
      binding( bind )
  {
    parse_options( options_str );
    harmful = false;

    apply_affecting_aura( priest().talents.improved_flash_heal );
    disc_mastery = true;

    if ( binding )
    {
      background = proc = true;
    }

    if ( p.talents.binding_heals.enabled() && !binding )
    {
      binding_heals = new flash_heal_t( p, name_str + "_binding", {}, true );
      add_child( binding_heals );
    }
  }

  void init() override
  {
    priest_heal_t::init();

    if ( binding )
    {
      snapshot_flags &= STATE_NO_MULTIPLIER;
      snapshot_flags &= ~( STATE_SP );
    }
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = priest_heal_t::execute_time_pct_multiplier();

    if ( priest().talents.unwavering_will.enabled() &&
         priest().health_percentage() > priest().talents.unwavering_will->effectN( 2 ).base_value() )
    {
      mul *= 1 + priest().talents.unwavering_will->effectN( 1 ).percent();
    }

    return mul;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_heal_t::composite_da_multiplier( s );

    // Value is stored as an int rather than a typical percent
    m *= 1 + ( priest().buffs.from_darkness_comes_light->check_stack_value() / 100 );

    return m;
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().buffs.twist_of_fate_heal_ally_fake->check() )
    {
      priest().buffs.twist_of_fate->trigger();
    }

    priest().buffs.protective_light->trigger();

    if ( priest().talents.discipline.train_of_thought.enabled() && !binding )
    {
      priest().cooldowns.power_word_shield->adjust( train_of_thought_cdr );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    priest().buffs.from_darkness_comes_light->expire();

    if ( result_is_hit( s->result ) )
    {
      if ( priest().talents.discipline.atonement.enabled() )
      {
        priest_td_t& td = get_td( s->target );
        td.buffs.atonement->trigger( atonement_duration );
      }

      if ( s->result_amount > 0 && priest().talents.binding_heals.enabled() && binding_heals && s->target != player )
      {
        binding_heals->execute_on_target( player, s->result_amount * binding_heal_percent );
      }
    }
  }
};

// ==========================================================================
// Renew
// ==========================================================================
struct renew_t final : public priest_heal_t
{
  timespan_t atonement_duration;
  timespan_t train_of_thought_cdr;

  renew_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "renew", p, p.talents.renew ),
      atonement_duration( timespan_t::from_seconds( p.talents.discipline.atonement_buff->effectN( 3 ).base_value() ) ),
      train_of_thought_cdr( priest().talents.discipline.train_of_thought->effectN( 1 ).time_value() )
  {
    parse_options( options_str );
    harmful = false;

    disc_mastery = true;
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().talents.discipline.train_of_thought.enabled() )
    {
      priest().cooldowns.power_word_shield->adjust( train_of_thought_cdr );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( priest().talents.discipline.atonement.enabled() )
      {
        priest_td_t& td = get_td( s->target );
        td.buffs.atonement->trigger( atonement_duration );
      }
    }
  }
};

// ==========================================================================
// Desperate Prayer
// ==========================================================================
struct desperate_prayer_t final : public priest_heal_t
{
  double health_change;
  double max_health_snapshot;

  desperate_prayer_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "desperate_prayer", p, p.find_class_spell( "Desperate Prayer" ) ),
      health_change( data().effectN( 1 ).percent() ),
      max_health_snapshot( player->resources.max[ RESOURCE_HEALTH ] )
  {
    parse_options( options_str );
    harmful  = false;
    may_crit = false;

    // does not seem to proc anything other than heal specific actions
    callbacks = false;

    // This is parsed as a Heal and HoT, disabling that manually
    // The "Heal" portion comes from the buff
    base_td_multiplier = 0.0;
    dot_duration       = timespan_t::from_seconds( 0 );

    // CDR
    apply_affecting_aura( p.talents.angels_mercy );

    if ( priest().talents.lights_inspiration.enabled() )
    {
      health_change += priest().talents.lights_inspiration->effectN( 1 ).percent();
    }
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    // Calculate this before the increased in health is applied
    double heal_amount = max_health_snapshot * health_change;

    sim->print_debug( "{} gains desperate_prayer: max_health_snapshot {}, health_change {}, heal_amount: {}",
                      player->name(), max_health_snapshot, health_change, heal_amount );

    // Record raw amd total amount to state
    state->result_raw   = heal_amount;
    state->result_total = heal_amount;
    return heal_amount;
  }

  void execute() override
  {
    // Before we increase the health of the player in the buff, store how much it was
    max_health_snapshot = player->resources.max[ RESOURCE_HEALTH ];

    priest().buffs.desperate_prayer->trigger();

    priest_heal_t::execute();
  }
};

// ==========================================================================
// Crystalline Reflection
// ==========================================================================
struct crystalline_reflection_heal_t final : public priest_heal_t
{
  crystalline_reflection_heal_t( priest_t& p )
    : priest_heal_t( "crystalline_reflection_heal", p, p.find_spell( 373462 ) )
  {
    spell_power_mod.direct = p.talents.crystalline_reflection->effectN( 3 ).sp_coeff();
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    if ( priest().buffs.twist_of_fate_heal_ally_fake->check() )
    {
      priest().buffs.twist_of_fate->trigger();
    }
  }
};

struct crystalline_reflection_damage_t final : public priest_spell_t
{
  crystalline_reflection_damage_t( priest_t& p )
    : priest_spell_t( "crystalline_reflection_damage", p, p.find_spell( 373464 ) )
  {
  }
};

// ==========================================================================
// Power Word: Shield
// TODO: add Rapture and Weal and Woe bonuses
// ==========================================================================
struct power_word_shield_t final : public priest_absorb_t
{
  double insanity;
  timespan_t atonement_duration;
  action_t* crystalline_reflection_heal;
  action_t* crystalline_reflection_damage;

  power_word_shield_t( priest_t& p, util::string_view options_str )
    : priest_absorb_t( "power_word_shield", p, p.find_class_spell( "Power Word: Shield" ) ),
      insanity( priest().specs.hallucinations->effectN( 1 ).resource() ),
      atonement_duration( timespan_t::from_seconds( p.talents.discipline.atonement_buff->effectN( 3 ).base_value() +
                                                    p.talents.discipline.indemnity->effectN( 1 ).base_value() ) ),
      crystalline_reflection_heal( nullptr ),
      crystalline_reflection_damage( nullptr )
  {
    parse_options( options_str );

    gcd_type = gcd_haste_type::SPELL_CAST_SPEED;

    disc_mastery = true;

    if ( p.talents.crystalline_reflection.enabled() )
    {
      crystalline_reflection_heal   = new crystalline_reflection_heal_t( p );
      crystalline_reflection_damage = new crystalline_reflection_damage_t( p );

      add_child( crystalline_reflection_heal );
      add_child( crystalline_reflection_damage );
    }
  }

  // Manually create the buff so we can reference it with Void Shield
  absorb_buff_t* create_buff( const action_state_t* ) override
  {
    return priest().buffs.power_word_shield;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_absorb_t::composite_da_multiplier( s );

    if ( priest().specialization() == PRIEST_DISCIPLINE && priest().talents.voidweaver.void_empowerment.enabled() &&
         priest().buffs.void_empowerment->check() )
    {
      m *= 1 + priest().buffs.void_empowerment->check_value();
    }
    return m;
  }

  void execute() override
  {
    if ( priest().specs.hallucinations->ok() )
    {
      priest().generate_insanity( insanity, priest().gains.hallucinations_power_word_shield, nullptr );
    }

    if ( priest().talents.words_of_the_pious.enabled() )
    {
      priest().buffs.words_of_the_pious->trigger();
    }
    if ( priest().talents.discipline.borrowed_time.enabled() )
    {
      priest().buffs.borrowed_time->trigger();
    }

    priest_absorb_t::execute();

    if ( priest().buffs.void_empowerment->check() )
    {
      priest().buffs.void_empowerment->expire();
    }
  }

  void impact( action_state_t* s ) override
  {
    if ( crystalline_reflection_heal )
    {
      crystalline_reflection_heal->execute_on_target( s->target );
    }

    priest_absorb_t::impact( s );

    if ( priest().talents.body_and_soul.enabled() && s->target->buffs.body_and_soul )
    {
      s->target->buffs.body_and_soul->trigger();
    }

    if ( crystalline_reflection_damage )
    {
      auto cr_damage = s->result_amount * p().talents.crystalline_reflection->effectN( 1 ).percent() *
                       p().options.crystalline_reflection_damage_mult;

      make_event( p().sim, rng().gauss( 2_s, 0.5_s ), [ cr_damage, this ] {
        player_t* target = p().target->is_enemy() && !p().target->is_sleeping() ? p().target : nullptr;

        if ( !target )
        {
          for ( auto potential_target : p().sim->target_non_sleeping_list )
          {
            if ( potential_target->is_sleeping() || !potential_target->is_enemy() )
              continue;

            target = potential_target;
            break;
          }
        }

        if ( target )
        {
          crystalline_reflection_damage->execute_on_target( target, cr_damage );
        }
      } );
    }

    if ( priest().talents.discipline.atonement.enabled() )
    {
      priest_td_t& td = get_td( s->target );
      td.buffs.atonement->trigger( atonement_duration );
    }

    if ( priest().sets->has_set_bonus( PRIEST_DISCIPLINE, T29, B2 ) )
    {
      priest().buffs.light_weaving->trigger();
    }
  }
};

// ==========================================================================
// Power Word: Life
// ==========================================================================
struct power_word_life_t final : public priest_heal_t
{
  double execute_percent;

  power_word_life_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "power_word_life", p, p.talents.power_word_life ),
      execute_percent( data().effectN( 2 ).base_value() )
  {
    parse_options( options_str );
    harmful      = false;
    disc_mastery = true;
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().buffs.twist_of_fate_heal_ally_fake->check() )
    {
      priest().buffs.twist_of_fate->trigger();
    }
  }
};

// ==========================================================================
// Essence Devourer
// ==========================================================================
struct essence_devourer_t final : public priest_heal_t
{
  essence_devourer_t( priest_t& p )
    : priest_heal_t( "essence_devourer", p,
                     p.talents.shared.mindbender.enabled() ? p.talents.essence_devourer_mindbender
                                                           : p.talents.essence_devourer_shadowfiend )
  {
    harmful = false;
  }
};

// ==========================================================================
// Atonement
// ==========================================================================
struct atonement_t final : public priest_heal_t
{
  atonement_t( priest_t& p ) : priest_heal_t( "atonement", p, p.talents.discipline.atonement_spell )
  {
    aoe       = -1;
    may_dodge = may_parry = may_block = harmful = false;
    background                                  = true;
    crit_bonus                                  = 0.0;
    disc_mastery                                = true;
  }

  void init() override
  {
    priest_heal_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA | STATE_MUL_DA;
    snapshot_flags &= ~( STATE_CRIT );
  }

  int num_targets() const override
  {
    return as<int>( p().allies_with_atonement.size() );
  }

  size_t available_targets( std::vector<player_t*>& target_list ) const override
  {
    target_list.clear();

    for ( auto t : p().allies_with_atonement )
    {
      target_list.push_back( t );
    }

    return target_list.size();
  }

  void activate() override
  {
    priest_heal_t::activate();

    priest().allies_with_atonement.register_callback( [ this ]( player_t* ) { target_cache.is_valid = false; } );
  }
};

// ==========================================================================
// Divine Aegis
// ==========================================================================
struct divine_aegis_t final : public priest_absorb_t
{
  double divine_aegis_mult;

  divine_aegis_t( priest_t& p )
    : priest_absorb_t( "divine_aegis", p, p.talents.discipline.divine_aegis ),
      divine_aegis_mult( data().effectN( 1 ).percent() )
  {
    may_miss = may_crit = callbacks = false;
    background = proc = true;
  }

  void init() override
  {
    absorb_t::init();

    snapshot_flags = update_flags = 0;
  }

  absorb_buff_t* create_buff( const action_state_t* s ) override
  {
    buff_t* b = buff_t::find( s->target, name_str );
    if ( b )
      return debug_cast<absorb_buff_t*>( b );

    std::string stats_obj_name = name_str;
    if ( s->target != player )
      stats_obj_name += "_" + player->name_str;
    stats_t* stats_obj = player->get_stats( stats_obj_name, this );
    if ( stats != stats_obj )
    {
      // Add absorb target stats as a child to the main stats object for reporting
      stats->add_child( stats_obj );
    }
    auto buff = make_buff<absorb_buff_t>( s->target, name_str, p().talents.discipline.divine_aegis_buff );
    buff->set_absorb_source( stats_obj );

    return buff;
  }

  void assess_damage( result_amount_type /*heal_type*/, action_state_t* s ) override
  {
    if ( target_specific[ s->target ] == nullptr )
    {
      target_specific[ s->target ] = create_buff( s );
    }

    s->result_amount *= divine_aegis_mult;

    if ( result_is_hit( s->result ) )
    {
      s->result_amount =
          std::min( s->target->max_health() * 0.3, target_specific[ s->target ]->check_value() + s->result_amount );

      target_specific[ s->target ]->trigger( 1, s->result_amount );

      sim->print_log( "{} {} applies absorb on {} for {} ({}) ({})", *player, *this, *s->target, s->result_amount,
                      s->result_total, s->result );
    }

    stats->add_result( 0.0, s->result_total, result_amount_type::ABSORB, s->result, s->block_result, s->target );
  }
};

// ==========================================================================
// Cauterizing Shadows
// ==========================================================================
struct cauterizing_shadows_t final : public priest_heal_t
{
  cauterizing_shadows_t( priest_t& p ) : priest_heal_t( "cauterizing_shadows", p, p.talents.cauterizing_shadows_spell )
  {
    harmful = false;

    may_miss = callbacks = false;
    background = proc = true;

    // They use an additional multiplier instead of just using spell power for some reason
    base_dd_multiplier =
        priest().talents.cauterizing_shadows->effectN( 2 ).percent() * priest().options.cauterizing_shadows_allies;
  }
};

}  // namespace heals

}  // namespace actions

namespace buffs
{
// ==========================================================================
// Power Word: Shield
// ==========================================================================
struct power_word_shield_buff_t : public absorb_buff_t
{
  double initial_size;

  power_word_shield_buff_t( priest_t* player )
    : absorb_buff_t( player, "power_word_shield", player->find_class_spell( "Power Word: Shield" ) )
  {
    set_absorb_source( player->get_stats( "power_word_shield" ) );
    initial_size = 0;
    buff_period = 0_s;  // TODO: Work out why Power Word: Shield has buff period. Work out why shields ticking refreshes
                        // them to full value.
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    sim->print_debug( "{} changes stored Power Word: Shield maximum from {} to {}", *player, initial_size, value );
    initial_size = value;

    return absorb_buff_t::trigger( stacks, value, chance, duration );
  }

  void trigger_void_shield( double result_amount )
  {
    // The initial amount of the shield and our "cap" is stored in the Void Shield buff
    double left_to_refill = initial_size - current_value;
    double refill_amount  = std::min( left_to_refill, result_amount );

    if ( refill_amount > 0 )
    {
      sim->print_debug( "{} adds value to Power Word: Shield. original={}, left_to_refill={}, refill_amount={}",
                        *player, current_value, left_to_refill, refill_amount );
      current_value += refill_amount;
    }
  }
};

// ==========================================================================
// Desperate Prayer - Health Increase buff
// ==========================================================================
struct desperate_prayer_t final : public priest_buff_t<buff_t>
{
  double health_change;

  desperate_prayer_t( priest_t& p )
    : base_t( p, "desperate_prayer", p.find_class_spell( "Desperate Prayer" ) ),
      health_change( data().effectN( 1 ).percent() )
  {
    // Cooldown handled by the action
    cooldown->duration = 0_ms;

    // Additive health increase
    if ( priest().talents.lights_inspiration.enabled() )
    {
      health_change += priest().talents.lights_inspiration->effectN( 1 ).percent();
    }
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    // Instead of increasing health here we perform this inside the heal_t action of the spell
    double old_health     = player->resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player->resources.max[ RESOURCE_HEALTH ];

    player->resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
    player->recalculate_resource_max( RESOURCE_HEALTH );

    sim->print_debug( "{} gains desperate_prayer: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                      player->name(), health_change * 100.0, old_health, player->resources.current[ RESOURCE_HEALTH ],
                      old_max_health, player->resources.max[ RESOURCE_HEALTH ] );
  }

  void expire_override( int, timespan_t ) override
  {
    // Whatever is gained by the heal is kept
    double old_health     = player->resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player->resources.max[ RESOURCE_HEALTH ];

    player->resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player->recalculate_resource_max( RESOURCE_HEALTH );

    sim->print_debug( "{} loses desperate_prayer: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                      player->name(), health_change * 100.0, old_health, player->resources.current[ RESOURCE_HEALTH ],
                      old_max_health, player->resources.max[ RESOURCE_HEALTH ] );
  }
};

// ==========================================================================
// Death and Madness Debuff
// ==========================================================================
struct death_and_madness_debuff_t final : public priest_buff_t<buff_t>
{
  death_and_madness_debuff_t( priest_td_t& actor_pair )
    : base_t( actor_pair, "death_and_madness_death_check",
              actor_pair.priest().talents.death_and_madness->effectN( 3 ).trigger() )
  {
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    // Fake-detect target demise by checking if buff was expired early
    if ( remaining_duration > timespan_t::zero() )
    {
      priest().generate_insanity(
          priest().talents.death_and_madness_insanity->effectN( 1 ).resource( RESOURCE_INSANITY ),
          priest().gains.insanity_death_and_madness, nullptr );
    }

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};
}  // namespace buffs

namespace items
{
void do_trinket_init( const priest_t* priest, specialization_e spec, const special_effect_t*& ptr,
                      const special_effect_t& effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live Simulationcraft. Also
  // ensure correct specialization.
  if ( !priest->find_spell( effect.spell_id )->ok() || priest->specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

using namespace unique_gear;

void init()
{
}

}  // namespace items

// ==========================================================================
// Priest Targetdata Definitions
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t& p ) : actor_target_data_t( target, &p ), dots(), buffs()
{
  dots.shadow_word_pain   = target->get_dot( "shadow_word_pain", &p );
  dots.vampiric_touch     = target->get_dot( "vampiric_touch", &p );
  dots.devouring_plague   = target->get_dot( "devouring_plague", &p );
  dots.mind_flay          = target->get_dot( "mind_flay", &p );
  dots.mind_flay_insanity = target->get_dot( "mind_flay_insanity", &p );
  dots.void_torrent       = target->get_dot( "void_torrent", &p );
  dots.purge_the_wicked   = target->get_dot( "purge_the_wicked", &p );
  dots.holy_fire          = target->get_dot( "holy_fire", &p );

  buffs.schism = make_buff( *this, "schism", p.talents.discipline.schism_debuff )
                     ->apply_affecting_aura( p.talents.discipline.malicious_intent );
  buffs.death_and_madness_debuff = make_buff<buffs::death_and_madness_debuff_t>( *this );
  buffs.echoing_void             = make_buff( *this, "echoing_void", p.talents.shadow.echoing_void_debuff );

  // TODO: stacks generated mid-collapse need to get re-triggered to collapse
  buffs.echoing_void_collapse = make_buff( *this, "echoing_void_collapse" )
                                    ->set_quiet( true )
                                    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                    ->set_tick_behavior( buff_tick_behavior::REFRESH )
                                    ->set_period( timespan_t::from_seconds( 1.0 ) )
                                    ->set_tick_callback( [ this, &p, target ]( buff_t* b, int, timespan_t ) {
                                      if ( buffs.echoing_void->check() )
                                        p.background_actions.echoing_void->execute_on_target( target );
                                      buffs.echoing_void->decrement();
                                      if ( !buffs.echoing_void->check() )
                                      {
                                        make_event( b->sim, [ b ] { b->cancel(); } );
                                      }
                                    } );
  buffs.apathy =
      make_buff( *this, "apathy", p.talents.apathy->effectN( 1 ).trigger() )->set_default_value_from_effect( 1 );

  buffs.psychic_horror = make_buff( *this, "psychic_horror", p.talents.shadow.psychic_horror )->set_cooldown( 0_s );
  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );

  buffs.atonement = make_buff( *this, "atonement", p.talents.discipline.atonement_buff )
                        ->set_refresh_behavior( buff_refresh_behavior::MAX )
                        ->set_stack_change_callback( [ &p, target ]( buff_t*, int, int n ) {
                          if ( n )
                          {
                            p.allies_with_atonement.push_back( target );
                          }
                          else
                          {
                            p.allies_with_atonement.find_and_erase_unordered( target );
                          }
                          size_t idx = std::clamp( as<int>( p.allies_with_atonement.size() ) - 1, 0, 19 );
                          p.buffs.sins_of_the_many->current_value = p.specs.sins_of_the_many_data[ idx ];
                        } );

  buffs.resonant_energy = make_buff_fallback( p.talents.archon.resonant_energy.enabled(), *this, "resonant_energy",
                                              p.talents.archon.resonant_energy_shadow );
}

void priest_td_t::reset()
{
}

void priest_td_t::target_demise()
{
  priest().sim->print_debug( "{} demised. Priest {} resets targetdata for him.", *target, priest() );

  if ( priest().talents.throes_of_pain.enabled() )
  {
    if ( priest().specialization() == PRIEST_SHADOW && dots.shadow_word_pain->is_ticking() )
    {
      priest().generate_insanity( priest().talents.throes_of_pain->effectN( 3 ).resource( RESOURCE_INSANITY ),
                                  priest().gains.throes_of_pain, nullptr );
    }
    if ( priest().specialization() != PRIEST_SHADOW &&
         ( dots.shadow_word_pain->is_ticking() || dots.purge_the_wicked->is_ticking() ) )
    {
      double amount =
          priest().talents.throes_of_pain->effectN( 5 ).percent() / 100.0 * priest().resources.max[ RESOURCE_MANA ];
      priest().resource_gain( RESOURCE_MANA, amount, priest().gains.throes_of_pain );
    }
  }

  // Stacks of Idol of N'Zoth will detonate on death
  if ( priest().talents.shadow.idol_of_nzoth.enabled() )
  {
    if ( buffs.echoing_void->check() )
    {
      priest().background_actions.echoing_void_demise->trigger( target, buffs.echoing_void->check() );
    }
  }

  reset();
}

// ==========================================================================
// Priest Definitions
// ==========================================================================

priest_t::priest_t( sim_t* sim, util::string_view name, race_e r )
  : player_t( sim, PRIEST, name, r ),
    buffs(),
    talents(),
    specs(),
    dot_spells(),
    mastery_spells(),
    cooldowns(),
    rppm(),
    gains(),
    benefits(),
    procs(),
    background_actions(),
    active_items(),
    allies_with_atonement(),
    state(),
    pets( *this ),
    options()
{
  create_cooldowns();
  create_gains();
  create_procs();
  create_benefits();

  resource_regeneration = regen_type::DYNAMIC;
}

/** Construct priest cooldowns */
void priest_t::create_cooldowns()
{
  cooldowns.holy_fire                     = get_cooldown( "holy_fire" );
  cooldowns.holy_word_chastise            = get_cooldown( "holy_word_chastise" );
  cooldowns.holy_word_serenity            = get_cooldown( "holy_word_serenity" );
  cooldowns.holy_word_sanctify            = get_cooldown( "holy_word_sanctify" );
  cooldowns.void_bolt                     = get_cooldown( "void_bolt" );
  cooldowns.mind_blast                    = get_cooldown( "mind_blast" );
  cooldowns.void_eruption                 = get_cooldown( "void_eruption" );
  cooldowns.shadow_word_death             = get_cooldown( "shadow_word_death" );
  cooldowns.power_word_shield             = get_cooldown( "power_word_shield" );
  cooldowns.mindbender                    = get_cooldown( "mindbender" );
  cooldowns.shadowfiend                   = get_cooldown( "shadowfiend" );
  cooldowns.voidwraith                    = get_cooldown( "voidwraith" );
  cooldowns.penance                       = get_cooldown( "penance" );
  cooldowns.maddening_touch_icd           = get_cooldown( "maddening_touch_icd" );
  cooldowns.maddening_touch_icd->duration = 1_s;
}

/** Construct priest gains */
void priest_t::create_gains()
{
  gains.insanity_auspicious_spirits      = get_gain( "Auspicious Spirits" );
  gains.insanity_death_and_madness       = get_gain( "Death and Madness" );
  gains.shadowfiend                      = get_gain( "Shadowfiend" );
  gains.mindbender                       = get_gain( "Mindbender" );
  gains.voidwraith                       = get_gain( "Voidwraith" );
  gains.power_word_solace                = get_gain( "Mana Gained from Power Word: Solace" );
  gains.throes_of_pain                   = get_gain( "Throes of Pain" );
  gains.insanity_idol_of_cthun_mind_flay = get_gain( "Insanity Gained from Idol of C'thun Mind Flay's" );
  gains.insanity_idol_of_cthun_mind_sear = get_gain( "Insanity Gained from Idol of C'thun Mind Sear's" );
  gains.hallucinations_power_word_shield = get_gain( "Insanity Gained from Power Word: Shield with Hallucinations" );
  gains.insanity_maddening_touch         = get_gain( "Maddening Touch" );
  gains.insanity_t30_2pc                 = get_gain( "Insanity Gained from T30 2PC" );
  gains.cauterizing_shadows_health       = get_gain( "Health from Cauterizing Shadows" );
}

/** Construct priest procs */
void priest_t::create_procs()
{
  // Discipline
  procs.power_of_the_dark_side          = get_proc( "Power of the Dark Side procs from dot ticks" );
  procs.power_of_the_dark_side_overflow = get_proc( "Power of the Dark Side from dot ticks lost to overflow" );
  procs.power_of_the_dark_side_dark_indulgence_overflow =
      get_proc( "Power of the Dark Side from Dark Indulgence lost to overflow" );
  procs.expiation_lost_no_dot = get_proc( "Missed chance for expiation to consume a DoT" );
  // Shadow - Talents
  procs.shadowy_apparition_vb          = get_proc( "Shadowy Apparition from Void Bolt" );
  procs.shadowy_apparition_swp         = get_proc( "Shadowy Apparition from Shadow Word: Pain" );
  procs.shadowy_apparition_dp          = get_proc( "Shadowy Apparition from Devouring Plague" );
  procs.shadowy_apparition_mb          = get_proc( "Shadowy Apparition from Mind Blast" );
  procs.shadowy_apparition_mfi         = get_proc( "Shadowy Apparition from Mind Flay: Insanity" );
  procs.shadowy_apparition_msi         = get_proc( "Shadowy Apparition from Mind Spike: Insanity" );
  procs.mind_devourer                  = get_proc( "Mind Devourer free Devouring Plague proc" );
  procs.void_tendril                   = get_proc( "Void Tendril proc from Idol of C'Thun" );
  procs.void_lasher                    = get_proc( "Void Lasher proc from Idol of C'Thun" );
  procs.shadowy_insight                = get_proc( "Shadowy Insight procs" );
  procs.shadowy_insight_overflow       = get_proc( "Shadowy Insight procs lost to overflow" );
  procs.shadowy_insight_missed         = get_proc( "Shadowy Insight procs not consumed" );
  procs.thing_from_beyond              = get_proc( "Thing from Beyond procs" );
  procs.deathspeaker                   = get_proc( "Shadow Word: Death resets from Deathspeaker" );
  procs.idol_of_nzoth_swp              = get_proc( "Idol of N'Zoth procs from Shadow Word: Pain" );
  procs.idol_of_nzoth_vt               = get_proc( "Idol of N'Zoth procs from Vampiric Touch" );
  procs.mind_flay_insanity_wasted      = get_proc( "Mind Flay: Insanity casts that did not channel for full ticks" );
  procs.idol_of_yshaarj_extra_duration = get_proc( "Idol of Y'Shaarj Devoured Violence procs" );
  procs.void_torrent_ticks_no_mastery  = get_proc( "Void Torrent ticks without full Mastery value" );
  procs.mindgames_casts_no_mastery     = get_proc( "Mindgames casts without full Mastery value" );
  procs.inescapable_torment_missed_mb  = get_proc( "Inescapable Torment expired when Mind Blast was ready" );
  procs.inescapable_torment_missed_swd = get_proc( "Inescapable Torment expired when Shadow Word: Death was ready" );
  procs.shadowy_apparition_crit        = get_proc( "Shadowy Apparitions that dealt 100% more damage" );
  procs.depth_of_shadows               = get_proc( "Depth of Shadows spawns of your main pet" );
  // Holy
  procs.divine_favor_chastise = get_proc( "Smite procs Holy Fire via Divine Favor: Chastise" );
  procs.divine_image          = get_proc( "Divine Image from Holy Words" );
}

/** Construct priest benefits */
void priest_t::create_benefits()
{
}

/**
 * Define the acting role of the priest().
 *
 * If base_t::primary_role() has a valid role defined, use it, otherwise select spec-based default.
 */
role_e priest_t::primary_role() const
{
  switch ( base_t::primary_role() )
  {
    case ROLE_HEAL:
      return ROLE_HEAL;
    case ROLE_DPS:
    case ROLE_SPELL:
      return ROLE_SPELL;
    case ROLE_ATTACK:
      return ROLE_SPELL;
    default:
      if ( specialization() == PRIEST_HOLY )
      {
        return ROLE_HEAL;
      }
      break;
  }

  return ROLE_SPELL;
}

/**
 * @brief Convert hybrid stats
 *
 *  Converts hybrid stats that either morph based on spec or only work
 *  for certain specs into the appropriate "basic" stats
 */
stat_e priest_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_STR_INT:
    case STAT_AGI_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
      return STAT_NONE;
    case STAT_SPIRIT:
      if ( specialization() != PRIEST_SHADOW )
      {
        return s;
      }
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

std::unique_ptr<expr_t> priest_t::create_expression( util::string_view expression_str )
{
  auto shadow_expression = create_expression_shadow( expression_str );
  if ( shadow_expression )
  {
    return shadow_expression;
  }

  auto disc_expression = create_expression_discipline( expression_str );
  if ( disc_expression )
  {
    return disc_expression;
  }

  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( auto pet_expr = create_pet_expression( expression_str, splits ) )
  {
    return pet_expr;
  }

  if ( splits.size() >= 2 )
  {
    if ( util::str_compare_ci( splits[ 0 ], "action" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "shadow_crash" ) )
      {
        auto crash_name   = "shadow_crash";
        auto crash_action = find_action( crash_name );

        if ( !crash_action )
          return expr_t::create_constant( expression_str, false );

        auto expr = crash_action->create_expression( util::string_join( util::make_span( splits ).subspan( 2 ), "." ) );
        if ( expr )
        {
          return expr;
        }

        auto tail = expression_str.substr( splits[ 0 ].length() + splits[ 1 ].length() + 2 );

        throw std::invalid_argument( fmt::format( "Unsupported crash expression '{}'.", tail ) );
      }
    }

    if ( util::str_compare_ci( splits[ 0 ], "priest" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "self_power_infusion" ) )
      {
        return expr_t::create_constant( "self_power_infusion", options.self_power_infusion );
      }

      if ( util::str_compare_ci( splits[ 1 ], "force_devour_matter" ) )
      {
        return expr_t::create_constant( "force_devour_matter", options.force_devour_matter );
      }

      if ( util::str_compare_ci( splits[ 1 ], "cthun_last_trigger_attempt" ) )
      {
        if ( talents.shadow.idol_of_cthun.enabled() )
          // std::min( sim->current_time() - last_trigger, max_interval() ).total_seconds();
          return make_fn_expr( "cthun_last_trigger_attempt", [ this ] {
            return std::min( sim->current_time() - rppm.idol_of_cthun->get_last_trigger_attempt(), 3.5_s )
                .total_seconds();
          } );
        else
          return expr_t::create_constant( "cthun_last_trigger_attempt", -1 );
      }

      if ( util::str_compare_ci( splits[ 1 ], "next_tick_si_proc_chance" ) )
      {
        if ( talents.shadow.shadowy_insight.enabled() )
        {
          return make_fn_expr( "next_tick_si_proc_chance", [ this ] {
            double proc_chance = std::max( threshold_rng.shadowy_insight->get_accumulated_chance() +
                                               threshold_rng.shadowy_insight->get_increment_max() - 1.0,
                                           0.0 ) /
                                 threshold_rng.shadowy_insight->get_increment_max();

            return proc_chance;
          } );
        }
        else
        {
          return expr_t::create_constant( "next_tick_si_proc_chance", 0 );
        }
      }

      if ( util::str_compare_ci( splits[ 1 ], "avg_time_until_si_proc" ) )
      {
        if ( talents.shadow.shadowy_insight.enabled() )
        {
          return make_fn_expr( "avg_time_until_si_proc", [ this ] {
            auto td    = get_target_data( target );
            dot_t* swp = td->dots.shadow_word_pain;

            double active_swp = get_active_dots( swp );

            if ( active_swp == 0 || !swp->current_action )
            {
              return std::numeric_limits<double>::infinity();
            }

            action_state_t* swp_state = swp->current_action->get_state( swp->state );
            double dot_tick_time      = ( swp->current_action->tick_time( swp_state ) ).total_seconds();

            double time_til_next_proc = ( 1 - threshold_rng.shadowy_insight->get_accumulated_chance() ) /
                                        threshold_rng.shadowy_insight->get_increment_max() * 2 * dot_tick_time;

            action_state_t::release( swp_state );

            return time_til_next_proc;
          } );
        }
        else
        {
          return expr_t::create_constant( "avg_time_until_si_proc", std::numeric_limits<double>::infinity() );
        }
      }

      if ( util::str_compare_ci( splits[ 1 ], "min_time_until_si_proc" ) )
      {
        if ( talents.shadow.shadowy_insight.enabled() )
        {
          return make_fn_expr( "min_time_until_si_proc", [ this ] {
            auto td    = get_target_data( target );
            dot_t* swp = td->dots.shadow_word_pain;

            double active_swp = get_active_dots( swp );

            if ( active_swp == 0 || !swp->current_action )
            {
              return std::numeric_limits<double>::infinity();
            }

            action_state_t* swp_state = swp->current_action->get_state( swp->state );
            double dot_tick_time      = ( swp->current_action->tick_time( swp_state ) ).total_seconds();

            double time_til_next_proc = ( 1 - threshold_rng.shadowy_insight->get_accumulated_chance() ) /
                                        threshold_rng.shadowy_insight->get_increment_max() * dot_tick_time;

            action_state_t::release( swp_state );

            return time_til_next_proc;
          } );
        }
        else
        {
          return expr_t::create_constant( "min_time_until_si_proc", std::numeric_limits<double>::infinity() );
        }
      }

      throw std::invalid_argument( fmt::format( "Unsupported priest expression '{}'.", splits[ 1 ] ) );
    }
  }

  return player_t::create_expression( expression_str );
}  // namespace priestspace

void priest_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  player_t::assess_damage( school, dtype, s );
}

double priest_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( sets->has_set_bonus( PRIEST_SHADOW, T29, B4 ) && buffs.dark_reveries->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.dark_reveries->current_value );
  }

  if ( buffs.devoured_anger->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.devoured_anger->check_value() );
  }

  if ( buffs.borrowed_time->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.borrowed_time->check_value() );
  }

  return h;
}

double priest_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.devoured_anger->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.devoured_anger->check_value() );
  }

  if ( buffs.borrowed_time->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.borrowed_time->check_value() );
  }

  return h;
}

double priest_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  return sc;
}

double priest_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  // Certain modifiers are only for Guardians, otherwise just give the Pet Modifier

  if ( guardian )
  {
    m *= ( 1.0 + specs.shadow_priest->effectN( 4 ).percent() );
    m *= ( 1.0 + specs.discipline_priest->effectN( 15 ).percent() );
  }
  else
  {
    m *= ( 1.0 + specs.shadow_priest->effectN( 3 ).percent() );
    m *= ( 1.0 + specs.discipline_priest->effectN( 3 ).percent() );
  }

  // TWW1 Set Bonus for pet spells, this double dips with pet spells
  if ( buffs.devouring_chorus->check() )
  {
    m *= ( 1.0 + buffs.devouring_chorus->check_stack_value() );
  }

  // Auto parsing does not cover melee attacks, and other attacks double dip with this
  if ( buffs.devoured_pride->check() )
  {
    m *= ( 1.0 + talents.shadow.devoured_pride->effectN( 2 ).percent() );
  }
  return m;
}

double priest_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  return m;
}

double priest_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_absorb_multiplier( s );

  return m;
}

double priest_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( specialization() == PRIEST_SHADOW && talents.voidweaver.voidheart.enabled() && buffs.voidheart->check() &&
       dbc::is_school( school, SCHOOL_SHADOW ) )
  {
    m *= 1.0 + talents.voidweaver.voidheart->effectN( 1 ).percent();
  }

  return m;
}

double priest_t::composite_player_target_multiplier( player_t* t, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( t, school );

  return m;
}

double priest_t::composite_leech() const
{
  double l = player_t::composite_leech();

  if ( talents.sanguine_teachings.enabled() )
  {
    auto amount = talents.sanguine_teachings->effectN( 1 ).percent();

    if ( talents.sanlayn.enabled() )
    {
      amount += talents.sanlayn->effectN( 3 ).percent();
    }

    l += amount;
  }

  return l;
}

double priest_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double mul = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA && sim->auras.power_word_fortitude->check() )
  {
    double pwf_val = sim->auras.power_word_fortitude->current_value;
    double wof_val = talents.archon.word_of_supremacy->effectN( 1 ).percent();
    mul /= 1.0 + pwf_val;
    mul *= 1.0 + pwf_val + wof_val;
  }

  return mul;
}

void priest_t::pre_analyze_hook()
{
  player_t::pre_analyze_hook();
}

void priest_t::analyze( sim_t& sim )
{
  player_t::analyze( sim );

  if ( specialization() == PRIEST_SHADOW )
  {
    auto vt = find_action( "vampiric_touch" );
    if ( vt && vt->stats->total_execute_time.mean() <= 2 )
    {
      vt->stats->apet = 0;
      vt->stats->ape  = vt->stats->total_amount.mean();
    }

    auto swp = find_action( "shadow_word_pain" );
    if ( swp && swp->stats->total_execute_time.mean() <= 2 )
    {
      swp->stats->apet = 0;
      swp->stats->ape  = swp->stats->total_amount.mean();
    }
  }
}

double priest_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
  {
    return 0.05;
  }

  return 0.0;
}

action_t* priest_t::create_action( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  action_t* shadow_action = create_action_shadow( name, options_str );
  if ( shadow_action )
  {
    return shadow_action;
  }

  action_t* discipline_action = create_action_discipline( name, options_str );
  if ( discipline_action )
  {
    return discipline_action;
  }

  action_t* holy_action = create_action_holy( name, options_str );
  if ( holy_action )
  {
    return holy_action;
  }

  if ( name == "smite" )
  {
    return new smite_t( *this, options_str );
  }
  if ( name == "angelic_feather" )
  {
    return new angelic_feather_t( *this, options_str );
  }
  if ( name == "halo" )
  {
    return new halo_t( *this, options_str );
  }
  if ( name == "divine_star" )
  {
    return new divine_star_t( *this, options_str );
  }
  if ( name == "levitate" )
  {
    return new levitate_t( *this, options_str );
  }
  if ( name == "power_word_shield" )
  {
    return new power_word_shield_t( *this, options_str );
  }
  if ( name == "power_word_fortitude" )
  {
    return new power_word_fortitude_t( *this, options_str );
  }
  if ( ( name == "shadowfiend" ) || ( name == "mindbender" ) || ( name == "fiend" ) || ( name == "voidwraith" ) )
  {
    return new summon_fiend_t( *this, options_str );
  }
  if ( name == "mind_blast" )
  {
    return new mind_blast_t( *this, options_str );
  }
  if ( name == "fade" )
  {
    return new fade_t( *this, options_str );
  }
  if ( name == "desperate_prayer" )
  {
    return new desperate_prayer_t( *this, options_str );
  }
  if ( name == "power_infusion" )
  {
    return new power_infusion_t( *this, options_str, "power_infusion" );
  }
  if ( name == "power_infusion_other" )
  {
    return new power_infusion_t( *this, options_str, "power_infusion_other" );
  }
  if ( name == "mindgames" )
  {
    return new mindgames_t( *this, options_str );
  }
  if ( name == "shadow_word_death" )
  {
    return new shadow_word_death_t( *this, options_str );
  }
  if ( name == "holy_nova" )
  {
    return new holy_nova_t( *this, options_str );
  }
  if ( name == "psychic_scream" )
  {
    return new psychic_scream_t( *this, options_str );
  }
  if ( name == "power_word_life" )
  {
    return new power_word_life_t( *this, options_str );
  }
  if ( name == "flash_heal" )
  {
    return new flash_heal_t( *this, options_str );
  }
  if ( name == "renew" )
  {
    return new renew_t( *this, options_str );
  }
  if ( name == "void_blast" && specialization() == PRIEST_SHADOW )
  {
    return new void_blast_shadow_t( *this, options_str );
  }

  return base_t::create_action( name, options_str );
}

void priest_t::create_pets()
{
  base_t::create_pets();
}

void priest_t::init_base_stats()
{
  // Halo has a 30 yard range
  // Divine Star gets two hits if used at or below 24yd, only 1 up to 28yd. 0 hits from 29-30yd
  if ( base.distance < 1 )
  {
    // Divine Star gets two hits if used at or below 24yd, only 1 up to 28yd. 0 hits from 29-30yd
    if ( talents.divine_star.enabled() )
    {
      base.distance = 24.0;
    }

    if ( talents.halo.enabled() )
    {
      base.distance = 30.0;
    }
  }

  base_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  if ( specialization() == PRIEST_SHADOW )
  {
    resources.base[ RESOURCE_INSANITY ] =
        100.0 + talents.shadow.voidtouched->effectN( 1 ).resource( RESOURCE_INSANITY );
  }
}

void priest_t::init_resources( bool force )
{
  // Can perform pre-pull actions that are harmful without actually hitting the boss
  // to build up Insanity before pulling
  if ( ( specialization() == PRIEST_SHADOW ) && resources.initial_opt[ RESOURCE_INSANITY ] <= 0 &&
       options.init_insanity )
  {
    auto shadow_crash_insanity = talents.shadow.shadow_crash->effectN( 2 ).resource( RESOURCE_INSANITY );

    if ( talents.shadow.shadow_crash.enabled() || talents.shadow.shadow_crash_target.enabled() )
    {
      // One Shadow Crash == 6 Insanity
      resources.initial_opt[ RESOURCE_INSANITY ] = shadow_crash_insanity;
    }
  }

  base_t::init_resources( force );
}

void priest_t::init_scaling()
{
  base_t::init_scaling();
}

void priest_t::init_finished()
{
  base_t::init_finished();
  cooldowns.fiend = talents.voidweaver.voidwraith.enabled()
                        ? cooldowns.voidwraith
                        : ( talents.shared.mindbender.enabled() ? cooldowns.mindbender : cooldowns.shadowfiend );

  /*PRECOMBAT SHENANIGANS
  we do this here so all precombat actions have gone throught init() and init_finished() so if-expr are properly
  parsed and we can adjust travel times accordingly based on subsequent precombat actions that will sucessfully
  cast*/

  for ( auto pre = precombat_action_list.begin(); pre != precombat_action_list.end(); pre++ )
  {
    // TODO: Remove Debugs After Feature is complete
    sim->print_debug( "{} looping through action list. Action: {}", this->name_str, ( *pre )->name_str );

    if ( auto halo = dynamic_cast<actions::spells::halo_t*>( *pre ) )
    {
      sim->print_debug( "{} Halo prepull found.", this->name_str );

      if ( halo->if_expr && !halo->if_expr->success() )
      {
        sim->print_debug( "{} Halo prepull failed if expr.", this->name_str );
        continue;
      }

      int actions           = 0;
      timespan_t time_spent = 0_ms;
      bool harmful_found    = false;

      for ( auto iter = pre + 1; iter < precombat_action_list.end(); iter++ )
      {
        action_t* a = *iter;

        sim->print_debug( "{} Checking for Halo Action: {}, Gcd: {}, expr: {}, expr_succ: {}, ready: {}",
                          this->name_str, a->name_str, a->gcd(), a->if_expr ? true : false,
                          a->if_expr ? ( a->if_expr->success() ? "true" : "false" ) : "N/A", a->action_ready() );

        if ( a->gcd() > 0_ms && ( !a->if_expr || a->if_expr->success() ) && a->action_ready() )
        {
          actions++;
          time_spent += std::max( a->base_execute_time.value(), a->trigger_gcd );
          if ( a->harmful )
          {
            if ( harmful_found )
            {
              break;
            }
            else
            {
              harmful_found = true;
            }
          }
        }
      }

      // Only allow precast Halo if there's only one GCD action following it - It doesn't have a very long
      // travel time. This can be at most 2 seconds or the current code will break.
      if ( time_spent < 2_s )
      {
        halo->harmful = false;
        sim->print_debug( "{} Halo prepull set to nonharmful.", this->name_str );
        // Child contains the travel time
        halo->prepull_timespent = time_spent;
      }

      break;
    }
  }
}

void priest_t::init_special_effects()
{
  if ( unique_gear::find_special_effect( this, 445593 ) &&
       talents.voidweaver.void_blast.enabled() )  // Aberrant Spellforge
  {
    if ( specialization() == PRIEST_DISCIPLINE )
    {
      callbacks.register_callback_trigger_function(
          452030, dbc_proc_callback_t::trigger_fn_type::CONDITION,
          []( const dbc_proc_callback_t*, action_t* a, const action_state_t* ) {
            return a->data().id() == 585 || a->data().id() == 450215;
          } );
    }
    if ( specialization() == PRIEST_SHADOW )
    {
      callbacks.register_callback_trigger_function(
          452030, dbc_proc_callback_t::trigger_fn_type::CONDITION,
          []( const dbc_proc_callback_t*, action_t* a, const action_state_t* ) {
            return a->data().id() == 8092 || a->data().id() == 450983;
          } );
    }
  }

  base_t::init_special_effects();
}

void priest_t::init_spells()
{
  base_t::init_spells();

  auto CT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };
  auto HT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::HERO, n ); };

  init_spells_shadow();
  init_spells_discipline();
  init_spells_holy();

  // Shared Spells
  talents.shared.mindbender          = ST( "Mindbender" );
  talents.shared.inescapable_torment = ST( "Inescapable Torment" );

  // Generic Spells
  specs.mind_blast                    = find_class_spell( "Mind Blast" );
  specs.shadow_word_death_self_damage = find_spell( 32409 );
  specs.psychic_scream                = find_class_spell( "Psychic Scream" );
  specs.fade                          = find_class_spell( "Fade" );
  specs.levitate_buff                 = find_spell( 111759 );

  // Class passives
  specs.priest            = dbc::get_class_passive( *this, SPEC_NONE );
  specs.holy_priest       = dbc::get_class_passive( *this, PRIEST_HOLY );
  specs.discipline_priest = dbc::get_class_passive( *this, PRIEST_DISCIPLINE );
  specs.shadow_priest     = dbc::get_class_passive( *this, PRIEST_SHADOW );

  // DoT Spells
  dot_spells.shadow_word_pain = find_class_spell( "Shadow Word: Pain" );
  dot_spells.vampiric_touch   = find_specialization_spell( "Vampiric Touch", PRIEST_SHADOW );
  dot_spells.devouring_plague = find_talent_spell( talent_tree::SPECIALIZATION, "Devouring Plague" );

  // Mastery Spells
  mastery_spells.grace          = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.shadow_weaving = find_mastery_spell( PRIEST_SHADOW );

  // Priest Tree Talents
  // Row 1
  talents.renew        = CT( "Renew" );         // NYI
  talents.dispel_magic = CT( "Dispel Magic" );  // NYI
  talents.shadowfiend  = CT( "Shadowfiend" );
  // Row 2
  talents.prayer_of_mending   = CT( "Prayer of Mending" );  // NYI
  talents.improved_flash_heal = CT( "Improved Flash Heal" );
  talents.purify_disease      = CT( "Purify Disease" );  // NYI
  talents.psychic_voice       = CT( "Psychic Voice" );
  talents.shadow_word_death   = CT( "Shadow Word: Death" );
  // Row 3
  talents.focused_mending              = CT( "Focused Mending" );  // NYI
  talents.holy_nova                    = CT( "Holy Nova" );
  talents.holy_nova_heal               = find_spell( 281265 );
  talents.protective_light             = CT( "Protective Light" );
  talents.protective_light_buff        = find_spell( 193065 );
  talents.from_darkness_comes_light    = CT( "From Darkness Comes Light" );
  talents.angelic_feather              = CT( "Angelic Feather" );
  talents.phantasm                     = CT( "Phantasm" );  // NYI
  talents.death_and_madness            = CT( "Death and Madness" );
  talents.death_and_madness_insanity   = find_spell( 321973 );
  talents.death_and_madness_reset_buff = find_spell( 390628 );
  // Row 4
  talents.spell_warding    = CT( "Spell Warding" );     // NYI
  talents.blessed_recovery = CT( "Blessed Recovery" );  // NYI
  talents.rhapsody         = CT( "Rhapsody" );
  talents.rhapsody_buff    = find_spell( 390636 );
  talents.leap_of_faith    = CT( "Leap of Faith" );   // NYI
  talents.shackle_undead   = CT( "Shackle Undead" );  // NYI
  talents.sheer_terror     = CT( "Sheer Terror" );    // NYI
  talents.void_tendrils    = CT( "Void Tendrils" );   // NYI
  talents.mind_control     = CT( "Mind Control" );    // NYI
  talents.dominate_mind    = CT( "Dominant Mind" );   // NYI
  // Row 5
  talents.words_of_the_pious = CT( "Words of the Pious" );
  talents.mass_dispel        = CT( "Mass Dispel" );      // NYI
  talents.move_with_grace    = CT( "Move With Grace" );  // NYI
  talents.power_infusion     = CT( "Power Infusion" );
  talents.vampiric_embrace   = CT( "Vampiric Embrace" );
  talents.sanguine_teachings = CT( "Sanguine Teachings" );
  talents.tithe_evasion      = CT( "Tithe Evasion" );
  // Row 6
  talents.inspiration                = CT( "Inspiration" );  // NYI
  talents.mental_agility             = CT( "Mental Agility" );
  talents.body_and_soul              = CT( "Body and Soul" );
  talents.twins_of_the_sun_priestess = CT( "Twins of the Sun Priestess" );
  talents.void_shield                = CT( "Void Shield" );
  talents.sanlayn                    = CT( "San'layn" );
  talents.apathy                     = CT( "Apathy" );
  // Row 7
  talents.unwavering_will    = CT( "Unwavering Will" );
  talents.twist_of_fate      = CT( "Twist of Fate" );
  talents.twist_of_fate_buff = find_spell( 390978 );
  talents.throes_of_pain     = CT( "Throes of Pain" );
  // Row 8
  talents.angels_mercy              = CT( "Angel's Mercy" );
  talents.binding_heals             = CT( "Binding Heals" );  // NYI
  talents.halo                      = CT( "Halo" );
  talents.halo_heal_holy            = find_spell( 120692 );
  talents.halo_dmg_holy             = find_spell( 120696 );
  talents.halo_heal_shadow          = find_spell( 390971 );
  talents.halo_dmg_shadow           = find_spell( 390964 );
  talents.divine_star               = CT( "Divine Star" );
  talents.divine_star_heal_holy     = find_spell( 110745 );
  talents.divine_star_dmg_holy      = find_spell( 122128 );
  talents.divine_star_heal_shadow   = find_spell( 390981 );
  talents.divine_star_dmg_shadow    = find_spell( 390845 );
  talents.translucent_image         = CT( "Translucent Image" );
  talents.cauterizing_shadows       = CT( "Cauterizing Shadows" );
  talents.cauterizing_shadows_spell = find_spell( 459992 );  // Hold sp coeff
  // Row 9
  talents.surge_of_light         = CT( "Surge of Light" );  // TODO: find out buff spell ID
  talents.lights_inspiration     = CT( "Light's Inspiration" );
  talents.crystalline_reflection = CT( "Crystalline Reflection" );  // NYI
  talents.improved_fade          = CT( "Improved Fade" );
  talents.manipulation           = CT( "Manipulation" );  // NYI
  // Row 10
  talents.benevolence                  = CT( "Benevolence" );
  talents.power_word_life              = CT( "Power Word: Life" );
  talents.angelic_bulwark              = CT( "Angelic Bulwark" );  // NYI
  talents.essence_devourer             = CT( "Essence Devourer" );
  talents.essence_devourer_shadowfiend = find_spell( 415673 );   // actual healing spell for Shadowfiend
  talents.essence_devourer_mindbender  = find_spell( 415676 );   // actual healing spell for Mindbender
  talents.void_shift                   = CT( "Void Shift" );     // NYI
  talents.phantom_reach                = CT( "Phantom Reach" );  // NYI

  // Shadow Talents
  // TODO: move this to sc_priest_shadow.cpp
  talents.shadow.echoing_void        = find_spell( 373304 );
  talents.shadow.echoing_void_debuff = find_spell( 373281 );

  // PvP Talents
  talents.pvp.mindgames                  = find_spell( 375901 );
  talents.pvp.mindgames_healing_reversal = find_spell( 323707 );  // TODO: Swap to new DF spells
  talents.pvp.mindgames_damage_reversal  = find_spell( 323706 );  // TODO: Swap to new DF spells 375902 + 375904

  // Archon Hero Talents (Holy/Shadow)
  talents.archon.power_surge            = HT( "Power Surge" );
  talents.archon.power_surge_buff       = find_spell( 453112 );
  talents.archon.perfected_form         = HT( "Perfected Form" );
  talents.archon.resonant_energy        = HT( "Resonant Energy" );
  talents.archon.resonant_energy_shadow = find_spell( 453850 );
  talents.archon.manifested_power       = HT( "Manifested Power" );
  talents.archon.shock_pulse            = HT( "Shock Pulse" );  // NYI
  talents.archon.incessant_screams      = HT( "Incessant Screams" );
  talents.archon.word_of_supremacy      = HT( "Word of Supremacy" );
  talents.archon.heightened_alteration  = HT( "Heightened Alteration" );
  talents.archon.empowered_surges       = HT( "Empowered Surges" );
  talents.archon.energy_compression     = HT( "Energy Compression" );
  talents.archon.sustained_potency      = HT( "Sustained Potency" );
  talents.archon.sustained_potency_buff = find_spell( 454002 );
  talents.archon.concentrated_infusion  = HT( "Concentrated Infusion" );
  talents.archon.energy_cycle           = HT( "Energy Cycle" );
  talents.archon.divine_halo            = HT( "Divine Halo" );

  // Oracle Hero Talents (Holy/Discipline)
  talents.oracle.premonition           = HT( "Premonition" );            // NYI
  talents.oracle.preventive_measures   = HT( "Preventive Measures" );    // NYI
  talents.oracle.preemptive_care       = HT( "Preemptive Care" );        // NYI
  talents.oracle.waste_no_time         = HT( "Waste No Time" );          // NYI
  talents.oracle.miraculous_recovery   = HT( "Miraculous Recovery" );    // NYI
  talents.oracle.assured_safety        = HT( "Assured Safety" );         // NYI
  talents.oracle.prompt_deliverance    = HT( "Prompt Deliverance" );     // NYI
  talents.oracle.divine_feathers       = HT( "Divine Feathers" );        // NYI
  talents.oracle.forseen_circumstances = HT( "Forseen Circumstances" );  // NYI
  talents.oracle.perfect_vision        = HT( "Perfect Vision" );         // NYI
  talents.oracle.clairvoyance          = HT( "Clairvoyance" );           // NYI
  talents.oracle.narrowed_visions      = HT( "Narrowed Visions" );       // NYI
  talents.oracle.fatebender            = HT( "Fatebender" );             // NYI
  talents.oracle.grand_reveal          = HT( "Grand Reveal" );           // NYI

  // Voidweaver Hero Talents (Discipline/Shadow)
  talents.voidweaver.entropic_rift          = HT( "Entropic Rift" );
  talents.voidweaver.entropic_rift_aoe      = find_spell( 450193 );  // Contains AoE radius info
  talents.voidweaver.entropic_rift_damage   = find_spell( 447448 );  // Contains damage coeff
  talents.voidweaver.entropic_rift_driver   = find_spell( 459314 );  // Contains damage coeff
  talents.voidweaver.entropic_rift_object   = find_spell( 447445 );  // Contains spell radius
  talents.voidweaver.no_escape              = HT( "No Escape" );     // NYI
  talents.voidweaver.dark_energy            = HT( "Dark Energy" );
  talents.voidweaver.void_blast             = HT( "Void Blast" );
  talents.voidweaver.void_blast_shadow      = find_spell( 450983 );
  talents.voidweaver.inner_quietus          = HT( "Inner Quietus" );
  talents.voidweaver.devour_matter          = HT( "Devour Matter" );
  talents.voidweaver.void_empowerment       = HT( "Void Empowerment" );
  talents.voidweaver.void_empowerment_buff  = find_spell( 450140 );
  talents.voidweaver.darkening_horizon      = find_talent_spell( 125982 );  // Entry id for Darkening Horizon
  talents.voidweaver.depth_of_shadows       = HT( "Depth of Shadows" );
  talents.voidweaver.voidwraith             = HT( "Voidwraith" );
  talents.voidweaver.voidwraith_spell       = find_spell( 451235 );
  talents.voidweaver.voidheart              = HT( "Voidheart" );
  talents.voidweaver.voidheart_buff         = find_spell( 449887 );  // Voidheart Buff
  talents.voidweaver.void_infusion          = HT( "Void Infusion" );
  talents.voidweaver.void_leech             = HT( "Void Leech" );          // NYI
  talents.voidweaver.embrace_the_shadow     = HT( "Embrace the Shadow" );  // NYI
  talents.voidweaver.collapsing_void        = HT( "Collapsing Void" );
  talents.voidweaver.collapsing_void_damage = find_spell( 448405 );
}

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Generic buffs
  buffs.desperate_prayer  = make_buff<buffs::desperate_prayer_t>( *this );
  buffs.power_word_shield = new buffs::power_word_shield_buff_t( this );
  buffs.fade              = make_buff( this, "fade", find_class_spell( "Fade" ) )->set_default_value_from_effect( 1 );
  buffs.levitate          = make_buff( this, "levitate", specs.levitate_buff )->set_duration( timespan_t::zero() );

  // Shared talent buffs
  // Does not show damage value on the buff spelldata, that is only found on the talent
  buffs.twist_of_fate = make_buff( this, "twist_of_fate", talents.twist_of_fate_buff )
                            ->set_trigger_spell( talents.twist_of_fate )
                            ->set_default_value_from_effect( 1 )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                            ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buffs.twist_of_fate_heal_ally_fake = make_buff( this, "twist_of_fate_can_trigger_on_ally_heal" )
                                           ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  // TODO: Extend functionality to use this.
  buffs.twist_of_fate_heal_self_fake = make_buff( this, "twist_of_fate_can_trigger_on_self_heal" )
                                           ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.rhapsody =
      make_buff( this, "rhapsody", talents.rhapsody_buff )->set_stack_change_callback( ( [ this ]( buff_t*, int, int ) {
        buffs.rhapsody_timer->trigger();
      } ) );
  buffs.rhapsody_timer = make_buff( this, "rhapsody_timer", talents.rhapsody )
                             ->set_quiet( true )
                             ->set_duration( talents.rhapsody->effectN( 1 ).period() )
                             ->set_max_stack( 1 )
                             ->set_stack_change_callback( ( [ this ]( buff_t*, int, int new_ ) {
                               if ( new_ == 0 )
                               {
                                 buffs.rhapsody->trigger();
                               }
                             } ) );
  // Tracking buff to see if the free reset is available for SW:D with DaM talented.
  buffs.death_and_madness_reset = make_buff( this, "death_and_madness_reset", talents.death_and_madness_reset_buff )
                                      ->set_trigger_spell( talents.death_and_madness );
  buffs.protective_light =
      make_buff( this, "protective_light", talents.protective_light_buff )->set_default_value_from_effect( 1 );
  buffs.from_darkness_comes_light =
      make_buff( this, "from_darkness_comes_light", talents.from_darkness_comes_light->effectN( 1 ).trigger() )
          ->set_default_value_from_effect( 1 );
  buffs.words_of_the_pious = make_buff( this, "words_of_the_pious", talents.words_of_the_pious->effectN( 1 ).trigger() )
                                 ->set_default_value_from_effect( 1 );

  // Voidweaver
  buffs.voidheart = make_buff( this, "voidheart", talents.voidweaver.voidheart_buff )
                        ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  // Tracking buff for the APL
  buffs.entropic_rift = make_buff_fallback( talents.voidweaver.entropic_rift.ok(), this, "entropic_rift",
                                            talents.voidweaver.entropic_rift_driver );

  if ( talents.voidweaver.entropic_rift.ok() )
  {
    buffs.entropic_rift->set_refresh_behavior( buff_refresh_behavior::DURATION )
        ->set_tick_zero( false )
        ->set_tick_on_application( false )
        ->set_tick_behavior( buff_tick_behavior::REFRESH )
        ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
        ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
          // Based on initial testing the first tick cannot hit any targets reliably due to the spawn distance/travel
          // time.
          // TODO: Check if this works fine on secondary targets, if so, rewrite this to have state passing to allow it
          // to miss the main target.
          if ( b->current_tick >= 2 )
          {
            background_actions.entropic_rift_damage->execute_on_target( state.last_entropic_rift_target );
          }
        } )
        ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
          if ( !new_ )
          {
            buffs.voidheart->expire();
            buffs.darkening_horizon->expire();
            buffs.collapsing_void->expire();
          }
        } );
  }

  // Tracking buff for Darkening Horizon extension
  buffs.darkening_horizon =
      make_buff( this, "darkening_horizon", talents.voidweaver.darkening_horizon )
          ->set_max_stack( talents.voidweaver.darkening_horizon.enabled()
                               ? as<int>( talents.voidweaver.darkening_horizon->effectN( 2 ).base_value() )
                               : 1 );

  buffs.collapsing_void = make_buff( this, "collapsing_void", talents.voidweaver.collapsing_void )
                              ->set_default_value_from_effect( specialization() == PRIEST_SHADOW ? 3 : 4, 0.01 )
                              ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                              ->set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
                                if ( new_ == 0 )
                                {
                                  background_actions.collapsing_void->trigger( state.last_entropic_rift_target, old_ );
                                }
                              } );
  if ( talents.voidweaver.collapsing_void.enabled() )
  {
    buffs.collapsing_void->set_max_stack(
        static_cast<int>( talents.voidweaver.collapsing_void->effectN( 2 ).base_value() ) );
  }
  // Currently only used for Discipline
  buffs.void_empowerment = make_buff( this, "void_empowerment", talents.voidweaver.void_empowerment_buff )
                               ->set_default_value_from_effect( 1 );
  // Archon
  buffs.power_surge =
      make_buff_fallback( talents.archon.power_surge.enabled(), this, "power_surge", talents.archon.power_surge_buff )
          ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { background_actions.halo->execute(); } );

  buffs.sustained_potency = make_buff_fallback( talents.archon.sustained_potency.enabled(), this, "sustained_potency",
                                                talents.archon.sustained_potency_buff );

  create_buffs_shadow();
  create_buffs_discipline();
  create_buffs_holy();
}

void priest_t::init_rng()
{
  init_rng_shadow();
  init_rng_discipline();
  init_rng_holy();

  player_t::init_rng();
}

void priest_t::init_background_actions()
{
  player_t::init_background_actions();

  background_actions.echoing_void        = new actions::spells::echoing_void_t( *this );
  background_actions.shadow_word_death   = new actions::spells::shadow_word_death_t( *this, 200_ms );
  background_actions.echoing_void_demise = new actions::spells::echoing_void_demise_t( *this );
  background_actions.essence_devourer    = new actions::heals::essence_devourer_t( *this );
  background_actions.atonement           = new actions::heals::atonement_t( *this );
  background_actions.halo                = new actions::spells::halo_t( *this, true );
  background_actions.cauterizing_shadows = new actions::heals::cauterizing_shadows_t( *this );

  // Voidweaver
  background_actions.entropic_rift        = new actions::spells::entropic_rift_t( *this );
  background_actions.entropic_rift_damage = new actions::spells::entropic_rift_damage_t( *this );
  background_actions.collapsing_void      = new actions::spells::collapsing_void_damage_t( *this );

  if ( talents.discipline.divine_aegis.enabled() )
  {
    background_actions.divine_aegis = new actions::heals::divine_aegis_t( *this );
  }

  background_actions.shadow_word_death->background = true;
  background_actions.shadow_word_death->dual       = true;
  background_actions.shadow_word_death->proc       = true;

  init_background_actions_shadow();
  init_background_actions_discipline();
  init_background_actions_holy();
}

void priest_t::do_dynamic_regen( bool forced )
{
  player_t::do_dynamic_regen( forced );
}

void priest_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );
}

void priest_t::apply_affecting_auras_late( action_t& action )
{
  action.apply_affecting_aura( specs.shadow_priest );
  action.apply_affecting_aura( specs.holy_priest );
  action.apply_affecting_aura( specs.discipline_priest );

  // Class Talents
  action.apply_affecting_aura( talents.benevolence );
  action.apply_affecting_aura( talents.mental_agility );

  // Shadow Talents
  action.apply_affecting_aura( talents.shadow.malediction );  // Void Torrent CDR
  action.apply_affecting_aura( talents.shadow.mastermind );
  action.apply_affecting_aura( talents.shadow.mental_decay );

  // Discipline Talents
  action.apply_affecting_aura( talents.discipline.dark_indulgence );
  action.apply_affecting_aura( talents.discipline.expiation );
  action.apply_affecting_aura( talents.discipline.blaze_of_light );

  // Holy Talents
  action.apply_affecting_aura( talents.holy.miracle_worker );

  // Disc T31 2pc
  action.apply_affecting_aura( sets->set( PRIEST_DISCIPLINE, T31, B2 ) );

  // Voidweaver Talents
  action.apply_affecting_aura( talents.voidweaver.inner_quietus );

  // Archon Talents
  action.apply_affecting_aura( talents.archon.empowered_surges );
  action.apply_affecting_aura( talents.archon.energy_compression );

  // TWW1 2pc
  action.apply_affecting_aura( sets->set( PRIEST_SHADOW, TWW1, B2 ) );
}

void priest_t::invalidate_cache( cache_e cache )
{
  player_t::invalidate_cache( cache );

  switch ( cache )
  {
    case CACHE_MASTERY:
      if ( mastery_spells.grace->ok() )
        player_t::invalidate_cache( CACHE_PLAYER_HEAL_MULTIPLIER );
      break;
    default:
      break;
  }
}

void priest_t::init_items()
{
  player_t::init_items();

  set_bonus_type_e tier_to_enable;
  switch ( specialization() )
  {
    case PRIEST_DISCIPLINE:
      tier_to_enable = T31;
      break;
    case PRIEST_HOLY:
      tier_to_enable = T31;
      break;
    case PRIEST_SHADOW:
      tier_to_enable = T30;
      break;
    default:
      return;
  }

  if ( sets->has_set_bonus( specialization(), DF4, B2 ) )
  {
    sets->enable_set_bonus( specialization(), tier_to_enable, B2 );
  }

  if ( sets->has_set_bonus( specialization(), DF4, B4 ) )
  {
    sets->enable_set_bonus( specialization(), tier_to_enable, B4 );
  }
}

std::string priest_t::default_potion() const
{
  return priest_apl::potion( this );
}

std::string priest_t::default_flask() const
{
  return priest_apl::flask( this );
}

std::string priest_t::default_food() const
{
  return priest_apl::food( this );
}

std::string priest_t::default_rune() const
{
  return priest_apl::rune( this );
}

std::string priest_t::default_temporary_enchant() const
{
  return priest_apl::temporary_enchant( this );
}

const priest_td_t* priest_t::find_target_data( const player_t* target ) const
{
  return _target_data[ target ];
}

/**
 * Obtain target_data for given target.
 * Always returns non-null targetdata pointer
 */
priest_td_t* priest_t::get_target_data( player_t* target ) const
{
  priest_td_t*& td = _target_data[ target ];
  if ( !td )
  {
    td = new priest_td_t( target, const_cast<priest_t&>( *this ) );
  }
  return td;
}

void priest_t::init_action_list()
{
  // 2020-12-31: Healing is outdated and not supported (both discipline and holy)
  if ( !sim->allow_experimental_specializations && primary_role() == ROLE_HEAL )
  {
    if ( !quiet )
      sim->error( "Role heal for priest '{}' is currently not supported.", name() );

    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  switch ( specialization() )
  {
    case PRIEST_SHADOW:
      if ( is_ptr() )
      {
        priest_apl::shadow_ptr( this );
      }
      else
      {
        priest_apl::shadow( this );
      }
      break;
    case PRIEST_DISCIPLINE:
      priest_apl::discipline( this );
      break;
    case PRIEST_HOLY:
      priest_apl::holy( this );
      break;
    default:
      priest_apl::no_spec( this );
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

void priest_t::combat_begin()
{
  player_t::combat_begin();
  if ( talents.rhapsody.enabled() )
  {
    buffs.rhapsody_timer->trigger();
  }
  if ( talents.holy.answered_prayers.enabled() )
  {
    buffs.answered_prayers_timer->trigger();
  }
  if ( specialization() == PRIEST_DISCIPLINE )
  {
    buffs.sins_of_the_many->trigger();
  }

  // Removed on Encounter Start
  if ( talents.archon.sustained_potency.enabled() )
  {
    buffs.sustained_potency->cancel();
  }

  if ( talents.twist_of_fate.enabled() )
  {
    struct twist_of_fate_event_t final : public event_t
    {
      timespan_t delta_time;
      priest_t* priest;

      twist_of_fate_event_t( priest_t* p, timespan_t t = 0_ms ) : event_t( *p->sim, t ), delta_time( t ), priest( p )
      {
      }

      const char* name() const override
      {
        return "twist_of_fate_event";
      }

      void execute() override
      {
        // TODO: Add damage event types and make it change the number of affected players. Additionally whether the
        // priest themselves is affected. This is relevant for random aoe heals (Essence Devourer) or for self damage
        // from SWD.
        if ( delta_time > 0_ms )
          priest->buffs.twist_of_fate_heal_ally_fake->trigger(
              rng().gauss_a( priest->options.twist_of_fate_heal_duration_mean,
                             priest->options.twist_of_fate_heal_duration_stddev, 0_s ) );

        double rate = priest->options.twist_of_fate_heal_rppm;
        if ( rate > 0.0 )
        {
          // Model the time between events with a Poisson process.
          timespan_t t = timespan_t::from_minutes( rng().exponential( 1 / rate ) );
          make_event<twist_of_fate_event_t>( sim(), priest, t );
        }
      }
    };

    if ( options.twist_of_fate_heal_rppm )
      make_event<twist_of_fate_event_t>( *sim, this );
  }
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  base_t::reset();

  // Reset Target Data
  for ( player_t* target : sim->target_list )
  {
    if ( auto td = _target_data[ target ] )
    {
      td->reset();
    }
  }

  allies_with_atonement.clear_without_callbacks();
  state = state_t();
}

void priest_t::target_mitigation( school_e school, result_amount_type dt, action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( buffs.dispersion->check() )
  {
    s->result_amount *= 1.0 + buffs.dispersion->data().effectN( 1 ).percent();
  }

  if ( talents.translucent_image.enabled() && buffs.fade->up() )
  {
    s->result_amount *= 1.0 + specs.fade->effectN( 4 ).percent();
  }

  if ( talents.protective_light.enabled() && buffs.protective_light->up() )
  {
    s->result_amount *= 1.0 + talents.protective_light_buff->effectN( 1 ).percent();
  }
}

void priest_t::create_options()
{
  base_t::create_options();

  add_option( opt_bool( "priest.mindgames_healing_reversal", options.mindgames_healing_reversal ) );
  add_option( opt_bool( "priest.mindgames_damage_reversal", options.mindgames_damage_reversal ) );
  add_option( opt_bool( "priest.self_power_infusion", options.self_power_infusion ) );
  // Default is 2, minimum of 1 bounce per second, maximum of 1 bounce per 12 seconds (prayer of mending's cooldown)
  add_option( opt_float( "priest.prayer_of_mending_bounce_rate", options.prayer_of_mending_bounce_rate, 1, 12 ) );
  add_option( opt_bool( "priest.init_insanity", options.init_insanity ) );
  add_option( opt_string( "priest.forced_yshaarj_type", options.forced_yshaarj_type ) );
  add_option( opt_float( "priest.twist_of_fate_heal_rppm", options.twist_of_fate_heal_rppm, 0, 120 ) );
  add_option( opt_timespan( "priest.twist_of_fate_heal_duration_mean", options.twist_of_fate_heal_duration_mean, 0_s,
                            timespan_t::max() ) );
  add_option( opt_timespan( "priest.twist_of_fate_heal_duration_stddev", options.twist_of_fate_heal_duration_stddev,
                            0_s, timespan_t::max() ) );
  add_option( opt_int( "priest.cauterizing_shadows_allies", options.cauterizing_shadows_allies, 0, 3 ) );
  add_option( opt_bool( "priest.force_devour_matter", options.force_devour_matter ) );
  add_option( opt_float( "priest.entropic_rift_miss_percent", options.entropic_rift_miss_percent, 0.0, 1.0 ) );
  add_option(
      opt_float( "priest.crystalline_reflection_damage_mult", options.crystalline_reflection_damage_mult, 0.0, 1.0 ) );
  add_option( opt_bool( "priest.no_channel_macro_mfi", options.no_channel_macro_mfi ) );
}

std::string priest_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  return profile_str;
}

void priest_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  auto* source_p = debug_cast<priest_t*>( source );

  options = source_p->options;
}

void priest_t::arise()
{
  base_t::arise();
}

void priest_t::demise()
{
  base_t::demise();
}

// Idol of C'Thun Talent Trigger
void priest_t::trigger_idol_of_cthun( action_state_t* s )
{
  if ( !talents.shadow.idol_of_cthun.enabled() )
    return;

  if ( rppm.idol_of_cthun->trigger() )
  {
    spawn_idol_of_cthun( s );
  }
}

// Trigger Atonement
void priest_t::trigger_atonement( action_state_t* s )
{
  if ( !talents.discipline.atonement.enabled() )
    return;

  if ( allies_with_atonement.empty() )
    return;

  if ( s->result_amount <= 0 )
    return;

  auto r = s->result_amount;

  r *= talents.discipline.atonement->effectN( 1 ).percent();

  if ( talents.discipline.abyssal_reverie.enabled() &&
       ( dbc::get_school_mask( s->action->school ) & SCHOOL_SHADOW ) != SCHOOL_SHADOW )
    r *= 1 + talents.discipline.abyssal_reverie->effectN( 1 ).percent();

  auto* state = background_actions.atonement->get_state();
  background_actions.atonement->snapshot_state( state, background_actions.atonement->amount_type( state ) );
  state->target      = this;
  state->crit_chance = s->result == RESULT_CRIT ? 1.0 : 0.0;

  background_actions.atonement->base_dd_min = background_actions.atonement->base_dd_max = r;
  background_actions.atonement->schedule_execute( state );
}

// Trigger Divine Aegis
void priest_t::trigger_divine_aegis( action_state_t* s )
{
  background_actions.divine_aegis->execute_on_target( s->target, s->result_amount );
}

void priest_t::trigger_essence_devourer()
{
  if ( !talents.essence_devourer.enabled() )
    return;

  background_actions.essence_devourer->execute();
}

void priest_t::spawn_idol_of_cthun( action_state_t* s )
{
  if ( !talents.shadow.idol_of_cthun.enabled() )
    return;

  if ( s->action->target_list().size() > 2 )
  {
    procs.void_lasher->occur();
    auto spawned_pets = pets.void_lasher.spawn();
  }
  else
  {
    procs.void_tendril->occur();
    auto spawned_pets = pets.void_tendril.spawn();
  }
}

int priest_t::shadow_weaving_active_dots( const player_t* target, const unsigned int spell_id ) const
{
  int dots = 0;

  if ( buffs.voidform->check() )
  {
    dots = 3;
  }
  else
  {
    if ( const priest_td_t* td = find_target_data( target ) )
    {
      const dot_t* swp = td->dots.shadow_word_pain;
      const dot_t* vt  = td->dots.vampiric_touch;
      const dot_t* dp  = td->dots.devouring_plague;

      // You get mastery benefit for a DoT as if it was active, if you are actively putting that DoT up
      // So to get the mastery benefit you either have that DoT ticking, or you are casting it
      bool swp_ticking = ( spell_id == dot_spells.shadow_word_pain->id() ) || swp->is_ticking();
      bool vt_ticking  = ( spell_id == dot_spells.vampiric_touch->id() ) || vt->is_ticking();
      bool dp_ticking  = ( spell_id == dot_spells.devouring_plague->id() ) || dp->is_ticking();

      dots = swp_ticking + vt_ticking + dp_ticking;
    }
  }

  return dots;
}

double priest_t::shadow_weaving_multiplier( const player_t* target, const unsigned int spell_id ) const
{
  double multiplier = 1.0;

  if ( mastery_spells.shadow_weaving->ok() )
  {
    auto dots = shadow_weaving_active_dots( target, spell_id );

    if ( dots > 0 )
    {
      multiplier *= 1 + dots * cache.mastery_value();
    }
  }

  return multiplier;
}

void priest_t::trigger_void_shield( double result_amount )
{
  ( debug_cast<buffs::power_word_shield_buff_t*>( buffs.power_word_shield ) )->trigger_void_shield( result_amount );
}

void priest_t::trigger_entropic_rift()
{
  // Spawn Entropic Rift
  background_actions.entropic_rift->execute();

  // Trigger the first stack of collapsing rift
  // This stack does not count for the damage mod
  if ( !talents.voidweaver.collapsing_void.enabled() )
  {
    return;
  }

  buffs.collapsing_void->trigger();
}

void priest_t::expand_entropic_rift()
{
  if ( !talents.voidweaver.collapsing_void.enabled() || !buffs.entropic_rift->check() )
  {
    return;
  }

  buffs.collapsing_void->trigger();
}

void priest_t::extend_entropic_rift()
{
  if ( !talents.voidweaver.darkening_horizon.enabled() || !buffs.entropic_rift->check() )
  {
    return;
  }

  auto max_stacks = talents.voidweaver.darkening_horizon->effectN( 2 ).base_value();

  // TODO: refactor this to make more sense
  if ( buffs.darkening_horizon->check() < max_stacks )
  {
    auto extension = timespan_t::from_seconds( talents.voidweaver.darkening_horizon->effectN( 3 ).base_value() );

    buffs.entropic_rift->extend_duration( this, extension );

    if ( buffs.voidheart->check() )
    {
      buffs.voidheart->extend_duration( this, extension );
    }

    if ( buffs.darkening_horizon->check() )
    {
      buffs.darkening_horizon->extend_duration( this, extension );
      buffs.darkening_horizon->increment();
    }
    else
    {
      buffs.darkening_horizon->trigger();
    }
  }
}

// Cauterizing Shadows Trigger
void priest_t::trigger_cauterizing_shadows()
{
  if ( !talents.cauterizing_shadows.enabled() )
  {
    return;
  }

  background_actions.cauterizing_shadows->execute();
}

struct priest_module_t final : public module_t
{
  priest_module_t() : module_t( PRIEST )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    return new priest_t( sim, name, r );
  }
  bool valid() const override
  {
    return true;
  }
  void init( player_t* p ) const override
  {
    p->buffs.body_and_soul    = make_buff( p, "body_and_soul", p->find_spell( 65081 ) );
    p->buffs.angelic_feather  = make_buff( p, "angelic_feather", p->find_spell( 121557 ) );
    p->buffs.guardian_spirit  = make_buff( p, "guardian_spirit",
                                           p->find_spell( 47788 ) );  // Let the ability handle the CD
    p->buffs.pain_suppression = make_buff( p, "pain_suppression",
                                           p->find_spell( 33206 ) );  // Let the ability handle the CD
    p->buffs.symbol_of_hope   = make_buff<buffs::symbol_of_hope_t>( p );
  }
  void static_init() const override
  {
    items::init();
  }
  void register_hotfixes() const override
  {
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

}  // namespace priestspace

const module_t* module_t::priest()
{
  static priestspace::priest_module_t m;
  return &m;
}

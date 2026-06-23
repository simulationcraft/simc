// ==========================================================================
// Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"

#include "class_modules/apl/apl_priest.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"
#include "sc_enums.hpp"
#include "sim/option.hpp"
#include "tcb/span.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace buffs
{
// ==========================================================================
// Power Word: Shield
// ==========================================================================
struct power_word_shield_buff_t : public priest_buff_t<absorb_buff_t>
{
  double initial_size;
  double energize_amount;
  using ab = priest_buff_t<absorb_buff_t>;

  power_word_shield_buff_t( priest_t* player, player_t* target )
    : ab( actor_pair_t( target, player ), "power_word_shield", player->find_class_spell( "Power Word: Shield" ) ),
      energize_amount( player->find_spell( 47755 )->effectN( 1 ).percent() / 100 *
                       priest().resources.max[ RESOURCE_MANA ] )
  {
    set_absorb_source( player->get_stats( "power_word_shield" ) );
    set_cooldown( 0_s );
    initial_size = 0;
    disable_ticking( true );  // TODO: Work out why Power Word: Shield has buff period. Work out why shields ticking
                              // refreshes them to full value.
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    sim->print_debug( "{} changes stored Power Word: Shield maximum from {} to {}", *player, initial_size, value );
    initial_size = value;

    return ab::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    if ( remaining_duration > timespan_t::zero() )
    {
      if ( priest().talents.discipline.shield_discipline.enabled() )
      {
        priest().resource_gain( RESOURCE_MANA, energize_amount, priest().gains.shield_discipline );
      }
    }

    ab::expire_override( expiration_stacks, remaining_duration );
  }
};

}  // namespace buffs

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
  propagate_const<expiation_t*> child_expiation;

  double void_blast_cdr;

public:
  mind_blast_base_t( priest_t& p, util::string_view options_str, const spell_data_t* s )
    : priest_spell_t( s->name_cstr(), p, s ),
      child_expiation( nullptr ),
      void_blast_cdr( p.find_spell( 450404 )->effectN( 3 ).percent() )
  {
    parse_options( options_str );
    affected_by_shadow_weaving   = true;
    cooldown                     = p.cooldowns.mind_blast;
    cooldown->hasted             = true;
    triggers_atonement           = true;
    idol_of_nzoth_execute_stacks = 6;

    if ( priest().talents.discipline.expiation.enabled() )
    {
      child_expiation             = new expiation_t( priest() );
      child_expiation->background = true;
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.shadow.shattered_psyche.enabled() && priest().buffs.shattered_psyche->check() )
    {
      priest().buffs.shattered_psyche->expire();
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

    return m;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    [[maybe_unused]] priest_td_t& td = get_td( s->target );

    if ( result_is_hit( s->result ) )
    {
      // Benefit Tracking
      if ( priest().talents.shadow.insidious_ire.enabled() )
      {
        priest().buffs.insidious_ire->up();
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

      priest().trigger_psychic_link( s );

      if ( priest().talents.shadow.dark_thoughts.enabled() && priest().buffs.shadowy_insight->check() )
      {
        priest().generate_insanity( priest().talents.shadow.dark_thoughts->effectN( 1 ).resource( RESOURCE_INSANITY ),
                                    priest().gains.insanity_dark_thoughts, s->action );
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

  void update_ready( timespan_t cd_duration ) override
  {
    // Decrementing a stack of shadowy insight will consume a max charge. Consuming a max charge loses you a current
    // charge. Therefore update_ready needs to not be called in that case.
    if ( priest().buffs.shadowy_insight->up() )
    {
      priest().buffs.shadowy_insight->decrement();
    }
    else
    {
      priest_spell_t::update_ready( cd_duration );
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

    priest_spell_t::reset();
  }
};

struct mind_blast_t final : public mind_blast_base_t
{
  mind_blast_t( priest_t& p, util::string_view options_str ) : mind_blast_base_t( p, options_str, p.talents.mind_blast )
  {
  }

  bool action_ready() override
  {
    if ( ( p().buffs.entropic_rift->check() ||
           ( p().channeling && p().channeling->id == p().talents.voidweaver.void_torrent->id() ) ) &&
         p().talents.voidweaver.void_blast.enabled() && priest().specialization() == PRIEST_SHADOW )
      return false;

    return mind_blast_base_t::action_ready();
  }

  void execute() override
  {
    if ( priest().talents.voidweaver.entropic_rift.enabled() && p().specialization() == PRIEST_DISCIPLINE )
    {
      priest().trigger_entropic_rift();
    }

    mind_blast_base_t::execute();
  }
};

struct void_blast_shadow_t final : public mind_blast_base_t
{
  void_blast_shadow_t( priest_t& p, util::string_view options_str )
    : mind_blast_base_t( p, options_str, p.talents.voidweaver.void_blast_shadow )
  {
    energize_amount   = data().effectN( 2 ).resource( RESOURCE_INSANITY );
    energize_type     = action_energize::ON_CAST;
    energize_resource = RESOURCE_INSANITY;

    base_costs[ RESOURCE_MANA ] = 0;

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
                      ( p().channeling && p().channeling->id == p().talents.voidweaver.void_torrent->id() ) );

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

    target_debuff = p.find_spell( 121557 );
  }

  buff_t* create_debuff( player_t* t ) override
  {
    return priest_spell_t::create_debuff( t )->set_movement_speed_buff_from_data();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    get_debuff( s->target )->trigger();
  }
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
    radius     = data().max_range();
    range      = 0;
    travel_speed =
        radius / 2;  // These do not seem to match up to the animation, which would be 2.5s. TODO: Check later
    affected_by_shadow_weaving = true;
    triggers_atonement         = true;

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
    if ( ( returning && !rng().roll( p().options.archon_halo_return_hit_chance ) ) ||
         ( !returning && !rng().roll( p().options.archon_halo_outgoing_hit_chance ) ) )
      return;

    priest_spell_t::impact( s );

    if ( p().talents.archon.resonant_energy.enabled() && !p().is_ptr() )
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
    radius     = data().max_range();
    range      = 0;
    travel_speed =
        radius / 2;  // These do not seem to match up to the animation, which would be 2.5s. TODO: Check later

    reduced_aoe_targets = p.talents.archon.halo->effectN( 1 ).base_value();
    disc_mastery        = true;
    holy_mastery        = true;

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
    : priest_spell_t( "halo", p, p.talents.archon.halo ),
      prepull_timespent( timespan_t::zero() ),
      _heal_spell_holy( new halo_heal_t( "halo_heal_holy", p, p.talents.archon.halo_heal_holy ) ),
      _dmg_spell_holy( new halo_spell_t( "halo_damage_holy", p, p.talents.archon.halo_dmg_holy ) ),
      _heal_spell_shadow( new halo_heal_t( "halo_heal_shadow", p, p.talents.archon.halo_heal_shadow ) ),
      _dmg_spell_shadow( new halo_spell_t( "halo_damage_shadow", p, p.talents.archon.halo_dmg_shadow ) )
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

    if ( priest().specialization() == PRIEST_SHADOW )
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
        priest().buffs.power_surge->extend_duration( -prepull_timespent );

        if ( priest().buffs.power_surge->check() )
        {
          auto when = -( prepull_timespent % priest().buffs.power_surge->tick_time() );

          priest().buffs.power_surge->reschedule_tick( when );
        }
      }
    }

    if ( priest().talents.archon.sustained_potency.enabled() && !is_precombat )
    {
      bool extended = false;
      if ( priest().buffs.voidform->check() )
      {
        extended = true;
        priest().buffs.voidform->extend_duration( 1_s );
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
          priest().buffs.surge_of_light->trigger( 1, 0, 1 );
          break;
        case PRIEST_SHADOW:
          priest().buffs.mind_flay_insanity->trigger();
          SC_FALLTHROUGH;
        default:
          break;
      }
    }

    // PTR version: Resonant Energy is a self-buff and should trigger once per Halo creation.
    if ( priest().talents.archon.resonant_energy.enabled() && priest().is_ptr() )
    {
      if ( priest().specialization() == PRIEST_SHADOW )
        priest().buffs.resonant_energy_damage->trigger();
      else if ( priest().specialization() == PRIEST_HOLY )
        priest().buffs.resonant_energy_healing->trigger();
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
  timespan_t divine_procession_extend;
  action_t* child_searing_light;

  smite_base_t( priest_t& p, util::string_view name, const spell_data_t* s, bool bg = false,
                util::string_view options_str = {} )
    : priest_spell_t( name, p, s ),
      divine_procession_extend( priest().talents.discipline.divine_procession->effectN( 1 ).time_value() ),
      child_searing_light( priest().background_actions.searing_light )

  {
    background = bg;
    if ( !background )
    {
      parse_options( options_str );
    }

    triggers_atonement = true;
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

    if ( priest().talents.discipline.greater_smite.enabled() )
    {
      priest().buffs.greater_smite->trigger();
    }

    if ( priest().talents.surge_of_light.enabled() )
      priest().buffs.surge_of_light->trigger();

    if ( priest().talents.holy.holy_word_chastise.enabled() )
    {
      timespan_t chastise_cdr =
          timespan_t::from_seconds( priest().talents.holy.holy_word_chastise->effectN( 2 ).base_value() );

      priest().do_holy_word_cdr( priest().cooldowns.holy_word_chastise, chastise_cdr );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( priest().talents.discipline.divine_procession.enabled() )
      {
        if ( p().allies_with_atonement.size() > 0 )
        {
          auto it = *( std::min_element( p().allies_with_atonement.begin(), p().allies_with_atonement.end(),
                                         [ this ]( player_t* a, player_t* b ) {
                                           return a->health_percentage() < b->health_percentage() &&
                                                  priest().find_target_data( b )->buffs.atonement->remains() < 30_s;
                                         } ) );

          auto atone = priest().find_target_data( it )->buffs.atonement;
          if ( atone->remains() < 30_s )
          {
            atone->extend_duration( divine_procession_extend );
          }
        }
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
  }

  void impact( action_state_t* s ) override
  {
    smite_base_t::impact( s );

    if ( shadow_smite )
    {
      make_event( sim, 200_ms, [ this, t = s->target ] { shadow_smite->execute_on_target( t ); } );
    }
  }

  bool action_ready() override
  {
    if ( p().buffs.entropic_rift->check() && p().talents.voidweaver.void_blast.enabled() &&
         priest().specialization() == PRIEST_DISCIPLINE )
      return false;

    return smite_base_t::action_ready();
  }
};

struct void_blast_disc_t final : public smite_base_t
{
  void_blast_disc_t( priest_t& p, util::string_view options_str )
    : smite_base_t( p, "void_blast", p.talents.voidweaver.void_blast_disc, false, options_str )
  {
  }

  double composite_atonement_multiplier( action_state_t* s ) override
  {
    double mul = smite_base_t::composite_atonement_multiplier( s );

    if ( p().talents.voidweaver.void_infusion.enabled() )
      mul *= 1 + p().talents.voidweaver.void_infusion->effectN( 2 ).percent();

    return mul;
  }

  bool action_ready() override
  {
    bool can_cast = ( p().buffs.entropic_rift->check() );

    if ( !can_cast || !p().talents.voidweaver.void_blast.enabled() || priest().specialization() != PRIEST_DISCIPLINE )
      return false;

    return smite_base_t::action_ready();
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
      power_infusion_magnitude( p.buffs.power_infusion->default_value )
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
      priest().buffs.power_infusion->trigger( 1, power_infusion_magnitude, -1,
                                              priest().buffs.power_infusion->buff_duration() );
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

    affected_by_shadow_weaving   = true;
    triggers_atonement           = true;
    idol_of_nzoth_execute_stacks = 10;

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
      if ( priest().shadow_weaving_active_dots( target, id ) < 3.0 )
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
// Fade
// ==========================================================================
struct fade_t final : public priest_spell_t
{
  fade_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "fade", p, p.talents.fade )
  {
    parse_options( options_str );
    harmful = false;
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
    : priest_spell_t( "shadow_word_death_self_damage", p, p.talents.shadow_word_death_self_damage ),
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

struct devour_matter_damage_t final : public priest_spell_t
{
  double execute_percent;
  double execute_modifier;
  int parent_chain_number = 0;

  devour_matter_damage_t( priest_t& p, const spell_data_t* swd_spell )
    : priest_spell_t( "shadow_word_death_devour_matter", p, swd_spell ),
      execute_percent( swd_spell->effectN( 3 ).base_value() ),
      execute_modifier( swd_spell->effectN( 4 ).percent() )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
    triggers_atonement         = false;
    spell_power_mod.direct     = swd_spell->effectN( 1 ).sp_coeff();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    if ( parent_chain_number > 0 )
      m *= priest().talents.shared.deaths_torment->effectN( 2 ).percent();

    if ( s->target->health_percentage() < execute_percent )
      m *= 1 + execute_modifier;

    return m;
  }

  double composite_energize_amount( const action_state_t* /* s */ ) const override
  {
    double ea = priest().talents.voidweaver.devour_matter->effectN( 3 ).base_value();

    if ( parent_chain_number > 0 )
      ea *= priest().talents.shared.deaths_torment->effectN( 2 ).percent();

    return ea;
  }
};

struct shadow_word_death_t final : public priest_spell_t
{
protected:
  struct swd_data
  {
    int chain_number = 0;
    int max_chain    = 2;
  };
  using state_t = priest_action_state_t<swd_data>;
  using ab      = priest_spell_t;

public:
  double execute_percent;
  double execute_modifier;
  propagate_const<shadow_word_death_self_damage_t*> shadow_word_death_self_damage;
  propagate_const<expiation_t*> child_expiation;
  action_t* child_searing_light;
  propagate_const<devour_matter_damage_t*> child_devour_matter;
  timespan_t execute_override;

  // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/1359
  shadow_word_death_t( priest_t& p, timespan_t execute_override = timespan_t::min() )
    : ab( "shadow_word_death", p, p.talents.shadow_word_death ),
      execute_percent( data().effectN( 3 ).base_value() ),
      execute_modifier( data().effectN( 4 ).percent() ),
      shadow_word_death_self_damage( new shadow_word_death_self_damage_t( p ) ),
      child_expiation( nullptr ),
      child_searing_light( priest().background_actions.searing_light ),
      child_devour_matter( nullptr ),
      execute_override( execute_override )
  {
    affected_by_shadow_weaving   = true;
    idol_of_nzoth_execute_stacks = 4;

    if ( priest().talents.discipline.expiation.enabled() )
    {
      child_expiation             = new expiation_t( priest() );
      child_expiation->background = true;
    }

    spell_power_mod.direct = data().effectN( 2 ).sp_coeff();

    triggers_atonement = true;

    if ( priest().talents.voidweaver.devour_matter.enabled() )
    {
      child_devour_matter = new devour_matter_damage_t( priest(), &data() );
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
    ab::snapshot_state( s, rt );
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = ab::composite_energize_amount( s );

    if ( cast_state( s )->chain_number > 0 )
    {
      // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/1385
      if ( priest().bugs )
      {
        ea = std::round( ea * priest().talents.shared.deaths_torment->effectN( 2 ).percent() );
      }
      else
      {
        ea *= priest().talents.shared.deaths_torment->effectN( 2 ).percent();
      }
    }

    return ea;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_da_multiplier( s );

    if ( cast_state( s )->chain_number > 0 )
    {
      m *= priest().talents.shared.deaths_torment->effectN( 2 ).percent();
    }

    if ( s->target->health_percentage() < execute_percent )
    {
      if ( sim->debug )
      {
        sim->print_debug( "{} below {}% HP. Increasing {} damage by {}%", s->target->name_str, execute_percent, *this,
                          execute_modifier * 100 );
      }
      m *= 1 + execute_modifier;
    }

    return m;
  }

  void execute() override
  {
    ab::execute();

    if ( priest().talents.shadow.death_and_madness.enabled() )
    {
      // Cooldown is reset only if you have't already gotten a reset
      if ( !priest().buffs.death_and_madness_reset->check() )
      {
        if ( target->health_percentage() <= execute_percent )
        {
          priest().buffs.death_and_madness_reset->trigger();
          cooldown->reset( false );
        }
      }
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

    if ( child_devour_matter && result_is_hit( s->result ) &&
         ( priest().options.force_devour_matter || s->target->has_absorb() ) )
    {
      child_devour_matter->parent_chain_number = cast_state( s )->chain_number;
      child_devour_matter->set_target( s->target );
      child_devour_matter->execute();
    }

    if ( priest().talents.shared.inescapable_torment.enabled() )
    {
      auto mod = 1.0;
      if ( priest().specialization() == PRIEST_SHADOW )
      {
        if ( cast_state( s )->chain_number > 0 )
        {
          mod *= priest().talents.shared.deaths_torment->effectN( 2 ).percent();
        }
        priest().trigger_inescapable_torment( s->target, cast_state( s )->chain_number > 0, mod );
      }
      else
      {
        // Discipline does not get any negative malus from Death's Torment.
        priest().trigger_inescapable_torment( s->target, false, 1.0 );
      }
    }

    if ( result_is_hit( s->result ) )
    {
      double save_health_percentage = s->target->health_percentage();

      if ( priest().talents.shared.shadowfiend.enabled() )
      {
        double chance = priest().talents.shared.shadowfiend->effectN( 3 ).percent();

        if ( cast_state( s )->chain_number > 0 )
        {
          chance *= priest().talents.shared.deaths_torment->effectN( 2 ).percent();
        }

        if ( ( save_health_percentage <= execute_percent ) && rng().roll( chance ) )
        {
          priest().procs.shadowfiend->occur();
          priest().pets.shadowfiend.spawn();
        }
      }

      if ( priest().talents.shared.deaths_torment.enabled() )
      {
        int number_of_chains;
        state_t* curr_state = cast_state( s );
        if ( curr_state->chain_number > 0 )
        {
          number_of_chains = curr_state->max_chain;
        }
        else
        {
          number_of_chains = as<int>( priest().talents.shared.deaths_torment->effectN( 1 ).base_value() );
        }

        sim->print_debug( "{} shadow_word_death_state: chain_number: {}, max_chain: {}", player->name(),
                          curr_state->chain_number, number_of_chains );

        if ( curr_state->chain_number < curr_state->max_chain )
        {
          shadow_word_death_t* child_death          = priest().background_actions.shadow_word_death.get();
          child_death->idol_of_nzoth_execute_stacks = 1;
          state_t* state                            = child_death->cast_state( child_death->get_state() );

          child_death->set_target( s->target );

          state->target       = s->target;
          state->chain_number = curr_state->chain_number + 1;
          state->max_chain    = number_of_chains;

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

      if ( priest().talents.shadow.death_and_madness.enabled() )
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
    : priest_spell_t( "psychic_scream", p, p.talents.psychic_scream )
  {
    parse_options( options_str );
    aoe      = -1;
    may_miss = may_crit   = false;
    ignore_false_positive = true;
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
      m *= 1.0 + ( parent_stacks * priest().buffs.collapsing_void->default_value );
    }

    return m;
  }

  void trigger( player_t* target, int stacks )
  {
    // The first trigger of the buff on the spawn of the rift does not count towards the damage mod stacks
    // Only relevant if you didn't extend the rift at all while active
    parent_stacks = stacks;

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
    aoe        = p.options.entropic_rift_miss_target_cap > 0 ? p.options.entropic_rift_miss_target_cap : -1;
    background = dual = true;
    radius            = base_radius;

    affected_by_shadow_weaving = true;
  }

  double miss_chance( double hit, player_t* t ) const override
  {
    double m = priest_spell_t::miss_chance( hit, t );

    if ( priest().options.entropic_rift_miss_percent > 0.0 )
    {
      // Use Secondary miss chance on non primary target.
      double miss_percent = t == target ? priest().options.entropic_rift_miss_percent
                                        : priest().options.entropic_rift_miss_percent_secondary;

      sim->print_debug( "entropic_rift_damage sets miss_chance to {} with target count: {}", miss_percent,
                        target_list().size() );

      m = miss_percent;
    }

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    m *= 1.0 + priest().buffs.collapsing_void->check_stack_value();

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
  entropic_rift_t( priest_t& p ) : priest_spell_t( "entropic_rift", p, p.talents.voidweaver.entropic_rift_object )
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
          priest().trigger_shadowy_insight( true );
          break;
        case PRIEST_DISCIPLINE:
          // TODO: Extend five shortest atonement.
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
  flash_heal_t* binding_heals;
  double binding_heal_percent;
  bool binding;

  flash_heal_t( priest_t& p, util::string_view options_str ) : flash_heal_t( p, "flash_heal", options_str )
  {
  }

  flash_heal_t( priest_t& p, util::string_view name, util::string_view options_str, bool bind = false )
    : priest_heal_t( name, p, p.find_class_spell( "Flash Heal" ) ),
      atonement_duration( timespan_t::from_seconds( p.talents.discipline.atonement_buff->effectN( 3 ).base_value() ) ),
      binding_heal_percent( p.talents.binding_heals->effectN( 1 ).percent() ),
      binding( bind )
  {
    parse_options( options_str );
    harmful = false;

    disc_mastery = true;
    holy_mastery = true;

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

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().buffs.twist_of_fate_heal_ally_fake->check() )
    {
      priest().buffs.twist_of_fate->trigger();
    }

    priest().buffs.protective_light->trigger();

    if ( priest().talents.holy.holy_word_serenity.enabled() )
    {
      timespan_t cdr = timespan_t::from_seconds( priest().talents.holy.holy_word_serenity->effectN( 2 ).base_value() );

      priest().do_holy_word_cdr( priest().cooldowns.holy_word_serenity, cdr );
    }

    if ( priest().talents.holy.holy_word_sanctify.enabled() && priest().talents.archon.energy_cycle.enabled() )
    {
      timespan_t cdr = -priest().talents.archon.energy_cycle->effectN( 2 ).time_value();

      priest().do_holy_word_cdr( priest().cooldowns.holy_word_sanctify, cdr );
    }

    p().buffs.surge_of_light->decrement();
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

  renew_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "renew", p, p.specs.renew ),
      atonement_duration( timespan_t::from_seconds( p.talents.discipline.atonement_buff->effectN( 3 ).base_value() ) )
  {
    parse_options( options_str );
    harmful = false;

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
    : priest_heal_t( "desperate_prayer", p, p.talents.desperate_prayer ),
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
// Power Word: Shield
// TODO: add Weal and Woe bonuses
// ==========================================================================
struct power_word_shield_t final : public priest_absorb_t
{
  double insanity;
  timespan_t atonement_duration;
  const spell_data_t* bns_data;

  power_word_shield_t( priest_t& p, util::string_view options_str )
    : priest_absorb_t( "power_word_shield", p, p.find_class_spell( "Power Word: Shield" ) ),
      insanity( priest().specs.hallucinations->effectN( 1 ).resource() ),
      atonement_duration( timespan_t::from_seconds( p.talents.discipline.atonement_buff->effectN( 3 ).base_value() +
                                                    p.talents.discipline.indemnity->effectN( 1 ).base_value() ) ),
      bns_data( p.talents.body_and_soul.ok() ? p.find_spell( 65081 ) : nullptr )
  {
    parse_options( options_str );

    cooldown->hasted = true;

    disc_mastery = true;
    harmful      = false;
  }

  // Manually create the buff so we can reference it with Void Shield
  absorb_buff_t* create_buff( const action_state_t* s ) override
  {
    if ( s->target == player )
    {
      if ( priest().buffs.power_word_shield->absorb_source != stats )
        priest().buffs.power_word_shield->set_absorb_source( stats );
      return priest().buffs.power_word_shield;
    }

    buff_t* b = buff_t::find( s->target, name_str, player );
    if ( b )
      return debug_cast<absorb_buff_t*>( b );

    auto buff = make_buff<buffs::power_word_shield_buff_t>( &priest(), s->target );
    buff->set_absorb_source( stats );

    if ( bns_data )
    {
      auto bns = make_buff( actor_pair_t( s->target, &priest() ), "body_and_soul", bns_data )
                     ->set_movement_speed_buff_from_data();

      buff->add_stack_change_callback( [ bns ]( buff_t*, int old_, int new_ ) {
        if ( !old_ && new_ )
          bns->trigger();
      } );
    }

    return buff;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_absorb_t::composite_da_multiplier( s );

    if ( priest().buffs.weal_and_woe->check() )
    {
      m *= 1 + priest().buffs.weal_and_woe->data().effectN( 2 ).percent() * priest().buffs.weal_and_woe->check();
    }

    return m;
  }

  void execute() override
  {
    if ( priest().specs.hallucinations->ok() )
    {
      priest().generate_insanity( insanity, priest().gains.hallucinations_power_word_shield, nullptr );
    }

    if ( priest().talents.discipline.borrowed_time.enabled() )
    {
      priest().buffs.borrowed_time->trigger();
    }

    priest_absorb_t::execute();

    priest().buffs.weal_and_woe->expire();
  }

  void impact( action_state_t* s ) override
  {
    priest_absorb_t::impact( s );

    if ( priest().talents.discipline.atonement.enabled() )
    {
      priest_td_t& td = get_td( s->target );
      td.buffs.atonement->trigger( atonement_duration );
    }
  }
};

// ==========================================================================
// Atonement
// ==========================================================================
struct atonement_t final : public priest_heal_t
{
  int max_hp_targets;
  int missing_hp_targets;

  atonement_t( priest_t& p )
    : priest_heal_t( "atonement", p, p.talents.discipline.atonement_spell ),
      max_hp_targets( 0 ),
      missing_hp_targets( 0 )
  {
    aoe       = -1;
    may_dodge = may_parry = may_block = harmful = false;
    background                                  = true;
    base_crit_bonus                             = 0.0;
    disc_mastery                                = true;
    divine_aegis                                = false;

    reduced_aoe_targets = p.talents.discipline.atonement->effectN( 2 ).base_value();
  }

  void init() override
  {
    priest_heal_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA | STATE_MUL_DA;
    snapshot_flags &= ~( STATE_CRIT | STATE_VERSATILITY );
  }

  int num_targets() const override
  {
    return as<int>( p().allies_with_atonement.size() );
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    double amount = base_da_max( state );

    if ( round_base_dmg )
      amount = floor( amount + 0.5 );

    if ( amount == 0 )
      return 0;

    amount *= state->composite_da_multiplier();

    // AoE with static reduced damage per target
    if ( state->chain_target > 0 )
      amount *= base_aoe_multiplier;

    // Spell splits damage across all targets equally
    if ( state->action->split_aoe_damage )
      amount /= state->n_targets;

    if ( missing_hp_targets > reduced_aoe_targets && state->target->health_percentage() <= 100.0 )
    {
      amount *= std::sqrt( reduced_aoe_targets / missing_hp_targets );
    }

    amount *= composite_aoe_multiplier( state );

    // Record initial amount to state
    state->result_raw = amount;

    if ( !sim->average_range )
      amount = floor( amount + rng().real() );

    if ( amount < 0 )
    {
      amount = 0;
    }

    if ( sim->debug )
    {
      sim->print_debug(
          "{} direct amount for {}: amount={:.6f} initial_amount={:.6f} s_mod={:.7g} "
          "s_power={:.7g} a_mod={:.7g} a_power={:.7g} mult={:.7g}",
          *player, *this, amount, state->result_raw, spell_direct_power_coefficient( state ),
          state->composite_spell_power(), attack_direct_power_coefficient( state ), state->composite_attack_power(),
          state->composite_da_multiplier() );
    }

    // Record total amount to state
    if ( result_is_miss( state->result ) )
    {
      state->result_total = 0.0;
      return 0.0;
    }
    else
    {
      state->result_total = amount;
      return amount;
    }
  }

  void execute() override
  {
    max_hp_targets     = 0;
    missing_hp_targets = 0;

    for ( auto& t : target_list() )
    {
      if ( t->health_percentage() >= 100.0 )
      {
        max_hp_targets++;
      }
      else
      {
        missing_hp_targets++;
      }
    }

    priest_heal_t::execute();
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

    auto buff = make_buff<absorb_buff_t>( s->target, name_str, p().talents.discipline.divine_aegis_buff );
    buff->set_absorb_source( stats );

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
}  // namespace heals

}  // namespace actions

namespace buffs
{
// ==========================================================================
// Desperate Prayer - Health Increase buff
// ==========================================================================
struct desperate_prayer_t final : public priest_buff_t<buff_t>
{
  double health_change;

  desperate_prayer_t( priest_t& p )
    : base_t( p, "desperate_prayer", p.talents.desperate_prayer ), health_change( data().effectN( 1 ).percent() )
  {
    // Cooldown handled by the action
    cooldown->duration = 0_ms;
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
              actor_pair.priest().talents.shadow.death_and_madness->effectN( 3 ).trigger() )
  {
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    // Fake-detect target demise by checking if buff was expired early
    if ( remaining_duration > timespan_t::zero() )
    {
      priest().generate_insanity(
          priest().talents.shadow.death_and_madness_insanity->effectN( 1 ).resource( RESOURCE_INSANITY ),
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
  dots.shadow_word_pain    = target->get_dot( "shadow_word_pain", &p );
  dots.vampiric_touch      = target->get_dot( "vampiric_touch", &p );
  dots.shadow_word_madness = target->get_dot( "shadow_word_madness", &p );
  dots.mind_flay           = target->get_dot( "mind_flay", &p );
  dots.mind_flay_insanity  = target->get_dot( "mind_flay_insanity", &p );
  dots.void_torrent        = target->get_dot( "void_torrent", &p );
  dots.purge_the_wicked    = target->get_dot( "purge_the_wicked", &p );
  dots.holy_fire           = target->get_dot( "holy_fire", &p );

  buffs.death_and_madness_debuff = make_buff<buffs::death_and_madness_debuff_t>( *this );

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
                          // size_t idx = std::clamp( as<int>( p.allies_with_atonement.size() ) - 1, 0, 19 );
                        } );

  buffs.resonant_energy = make_buff_fallback( p.talents.archon.resonant_energy.enabled() && !p.is_ptr(), *this,
                                              "resonant_energy", p.talents.archon.resonant_energy_shadow );

  buffs.horrific_visions = make_buff( *this, "horrific_visions", p.talents.shadow.horrific_visions );
}

void priest_td_t::reset()
{
}

void priest_td_t::target_demise()
{
  priest().sim->print_debug( "{} demised. Priest {} resets targetdata for him.", *target, priest() );

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
    sample_data(),
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
  cooldowns.holy_word_chastise            = get_cooldown( "holy_word_chastise" );
  cooldowns.holy_word_serenity            = get_cooldown( "holy_word_serenity" );
  cooldowns.holy_word_sanctify            = get_cooldown( "holy_word_sanctify" );
  cooldowns.void_volley                   = get_cooldown( "void_volley" );
  cooldowns.mind_blast                    = get_cooldown( "mind_blast" );
  cooldowns.shadow_word_death             = get_cooldown( "shadow_word_death" );
  cooldowns.power_word_shield             = get_cooldown( "power_word_shield" );
  cooldowns.penance                       = get_cooldown( "penance" );
  cooldowns.ultimate_penitence            = get_cooldown( "ultimate_penitence" );
  cooldowns.maddening_touch_icd           = get_cooldown( "maddening_touch_icd" );
  cooldowns.maddening_touch_icd->duration = 1_s;
}

/** Construct priest gains */
void priest_t::create_gains()
{
  gains.insanity_auspicious_spirits        = get_gain( "Auspicious Spirits" );
  gains.insanity_death_and_madness         = get_gain( "Death and Madness" );
  gains.shadowfiend                        = get_gain( "Shadowfiend" );
  gains.mindbender                         = get_gain( "Mindbender" );
  gains.voidwraith                         = get_gain( "Voidwraith" );
  gains.insanity_idol_of_cthun_mind_flay   = get_gain( "Insanity Gained from Idol of C'thun Mind Flay's" );
  gains.insanity_idol_of_cthun_mind_sear   = get_gain( "Insanity Gained from Idol of C'thun Mind Sear's" );
  gains.hallucinations_power_word_shield   = get_gain( "Insanity Gained from Power Word: Shield with Hallucinations" );
  gains.insanity_maddening_touch           = get_gain( "Maddening Touch" );
  gains.shield_discipline                  = get_gain( "Shield Discipline" );
  gains.insanity_dark_thoughts             = get_gain( "Dark Thoughts" );
  gains.insanity_horrific_vision           = get_gain( "Horrific Vision" );
  gains.insanity_vision_of_nzoth           = get_gain( "Vision of N'Zoth" );
  gains.insanity_mid_s2_4pc_vampiric_touch = get_gain( "Midnight S2 4pc Vampiric Touch" );
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
  procs.shadowy_apparition_swp           = get_proc( "Shadowy Apparition from Tormented Spirits" );
  procs.shadowy_apparition_swm           = get_proc( "Shadowy Apparition from Shadow Word: Madness" );
  procs.shadowy_apparition_mb            = get_proc( "Shadowy Apparition from Mind Blast" );
  procs.shadowy_apparition_mfi           = get_proc( "Shadowy Apparition from Mind Flay: Insanity" );
  procs.shadowy_apparition_yshaarj       = get_proc( "Shadowy Apparition from Idol of Y'Shaarj" );
  procs.shadowy_apparition_nzoth         = get_proc( "Shadowy Apparition from Idol of N'Zoth" );
  procs.shadowy_apparition_yogg          = get_proc( "Shadowy Apparition from Idol of Yogg-Saron" );
  procs.shadowy_apparition_cthun         = get_proc( "Shadowy Apparition from Idol of C'Thun" );
  procs.shadowy_apparition_mid_s2_4pc_vt = get_proc( "Shadowy Apparition from Midnight S2 4pc Vampiric Touch" );
  procs.mind_devourer                    = get_proc( "Mind Devourer free Shadow Word: Madness proc" );
  procs.void_tendril                     = get_proc( "Void Tendril proc from Idol of C'Thun" );
  procs.void_lasher                      = get_proc( "Void Lasher proc from Idol of C'Thun" );
  procs.shadowy_insight                  = get_proc( "Shadowy Insight procs" );
  procs.shadowy_insight_overflow         = get_proc( "Shadowy Insight procs lost to overflow" );
  procs.shadowy_insight_missed           = get_proc( "Shadowy Insight procs not consumed" );
  procs.thing_from_beyond                = get_proc( "Thing from Beyond procs" );
  procs.mind_flay_insanity_wasted        = get_proc( "Mind Flay: Insanity casts that did not channel for full ticks" );
  procs.void_torrent_ticks_no_mastery    = get_proc( "Void Torrent ticks without full Mastery value" );
  procs.mindgames_casts_no_mastery       = get_proc( "Mindgames casts without full Mastery value" );
  procs.inescapable_torment_missed_mb    = get_proc( "Inescapable Torment expired when Mind Blast was ready" );
  procs.inescapable_torment_missed_swd   = get_proc( "Inescapable Torment expired when Shadow Word: Death was ready" );
  procs.shadowfiend                      = get_proc( "Shadowfiend procs from Shadow Word: Death casts" );
  procs.void_apparition                  = get_proc( "Void Apparition procs" );
  procs.void_apparition_yshaarj          = get_proc( "Idol of Y'Shaarj from Tentacle Slam" );
  procs.void_apparition_horrific_vision  = get_proc( "Horrific Vision from Tentacle Slam" );
  procs.void_apparition_vision_of_nzoth  = get_proc( "Vision of N'Zoth from Tentacle Slam" );
  procs.void_apparition_yogg             = get_proc( "Idol of Yogg-Saron from Tentacle Slam" );
  procs.void_apparition_cthun            = get_proc( "Idol of C'Thun from Tentacle Slam" );
  procs.tentacle_slam_idol               = get_proc( "Idol spell from Tentacle Slam" );
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

  if ( splits.size() >= 2 )
  {
    if ( util::str_compare_ci( splits[ 0 ], "action" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "tentacle_slam" ) )
      {
        auto crash_name   = "tentacle_slam";
        auto crash_action = find_action( crash_name );

        if ( !crash_action )
          return expr_t::create_constant( expression_str, false );

        auto expr = crash_action->create_expression( util::string_join( util::make_span( splits ).subspan( 2 ), "." ) );
        if ( expr )
        {
          return expr;
        }

        auto tail = expression_str.substr( splits[ 0 ].length() + splits[ 1 ].length() + 2 );

        throw sc_invalid_apl_argument( fmt::format( "Unsupported tentacle_slam expression '{}'.", tail ) );
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

      throw sc_invalid_apl_argument( fmt::format( "Unsupported priest expression '{}'.", splits[ 1 ] ) );
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

  if ( buffs.idol_of_yshaarj->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.idol_of_yshaarj->check_value() );
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

double priest_t::composite_mitigation_multiplier( const action_state_t* s, school_e school, bool direct ) const
{
  double m = player_t::composite_mitigation_multiplier( s, school, direct );

  if ( talents.translucent_image.ok() && buffs.fade->check() )
    m *= 1.0 + buffs.fade->check_value();

  if ( buffs.protective_light->check() )
    m *= 1.0 + buffs.protective_light->check_value();

  if ( buffs.dispersion->check() )
    m *= 1.0 + specs.dispersion->effectN( 1 ).percent();

  return m;
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

  if ( talents.shadow.voidform.enabled() )
  {
    sample_data.voidform_duration->analyze();
  }
}

double priest_t::matching_gear_multiplier( attribute_e attr ) const
{
  return player_t::matching_gear_multiplier( attr );
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
  if ( name == "flash_heal" )
  {
    return new flash_heal_t( *this, options_str );
  }
  if ( name == "void_blast" )
  {
    if ( specialization() == PRIEST_SHADOW )
      return new void_blast_shadow_t( *this, options_str );
    if ( specialization() == PRIEST_DISCIPLINE )
      return new void_blast_disc_t( *this, options_str );
  }

  return base_t::create_action( name, options_str );
}

void priest_t::create_pets()
{
  base_t::create_pets();
  pets.set_pet_defaults( *this );
}

void priest_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    // Bug: Halo is a few yards short
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/1225
    if ( talents.archon.halo.enabled() )
    {
      base.distance = ( bugs ? 38.0 : 40.0 );
    }

    if ( talents.phantom_reach.enabled() )
    {
      base.distance *= 1.0 + talents.phantom_reach->effectN( 1 ).percent();
    }

    // Going above 40 probably isn't a good idea
    base.distance = std::max( base.distance, 40.00 );
  }

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  base_t::init_base_stats();
}

void priest_t::init_resources( bool force )
{
  base_t::init_resources( force );
}

void priest_t::init_scaling()
{
  base_t::init_scaling();
}

void priest_t::init_finished()
{
  base_t::init_finished();

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

      timespan_t time_spent = 0_ms;

      for ( auto iter = pre + 1; iter < precombat_action_list.end(); iter++ )
      {
        action_t* a = *iter;

        sim->print_debug( "{} Checking for Halo Action: {}, Gcd: {}, expr: {}, expr_succ: {}, ready: {}",
                          this->name_str, a->name_str, a->gcd(), a->if_expr ? true : false,
                          a->if_expr ? ( a->if_expr->success() ? "true" : "false" ) : "N/A", a->action_ready() );

        if ( a->gcd() > 0_ms && ( !a->if_expr || a->if_expr->success() ) && a->action_ready() )
        {
          timespan_t time = std::max( a->base_execute_time.value(), a->trigger_gcd );

          if ( time_spent + time > 2_s )
            break;

          time_spent += time;

          if ( a->harmful )
          {
            break;
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
          []( const dbc_proc_callback_t*, const proc_data_t& data, player_t*, action_state_t*, proc_trigger_type_e ) {
            return data->id() == 585 || data->id() == 450215;
          } );
    }
    if ( specialization() == PRIEST_SHADOW )
    {
      callbacks.register_callback_trigger_function(
          452030, dbc_proc_callback_t::trigger_fn_type::CONDITION,
          []( const dbc_proc_callback_t*, const proc_data_t& data, player_t*, action_state_t*, proc_trigger_type_e ) {
            return data->id() == 8092 || data->id() == 450983;
          } );
    }
  }

  if ( unique_gear::find_special_effect( this, 443393 ) && talents.twist_of_fate.enabled() )
  {
    callbacks.register_callback_execute_function(
        443393, [ this ]( const dbc_proc_callback_t* cb, const spell_data_t*, player_t* t, action_state_t* s ) {
          if ( rng().roll( options.synergistic_brewterializer_tof_chance ) )
          {
            buffs.twist_of_fate->trigger();
          }

          if ( rng().roll( options.synergistic_brewterializer_barrel_hit_chance ) )
          {
            cb->proc_action->set_target( cb->get_target( t, s ) );
            auto proc_state    = cb->proc_action->get_state();
            proc_state->target = cb->proc_action->target;
            cb->proc_action->snapshot_state( proc_state, cb->proc_action->amount_type( proc_state ) );
            cb->proc_action->schedule_execute( proc_state );
          }
        } );
  }

  // Entropic Rift is coded as direct damage, but should not count
  // as casts for Burst of Knowledge
  callbacks.register_callback_trigger_function(
      469925, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      []( const dbc_proc_callback_t*, const proc_data_t& data, player_t*, action_state_t*, proc_trigger_type_e ) {
        return data->id() != 447448;
      } );

  init_special_effects_shadow();
  base_t::init_special_effects();
}

void priest_t::init_spells()
{
  base_t::init_spells();

  auto CT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };
  auto HT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::HERO, n ); };

  [[maybe_unused]] auto sd_nf = spell_data_t::not_found();

  init_spells_shadow();
  init_spells_discipline();
  init_spells_holy();

  // Shared Spells
  talents.shared.mindbender          = ST( "Mindbender" );
  talents.shared.inescapable_torment = ST( "Inescapable Torment" );
  talents.shared.shadowfiend         = ST( "Shadowfiend" );
  talents.shared.deaths_torment      = ST( "Death's Torment" );

  // Generic Spells
  specs.levitate_buff     = find_spell( 111759 );
  specs.renew             = find_class_spell( "Renew" );
  specs.prayer_of_mending = find_class_spell( "Prayer of Mending" );  // NYI
  specs.vampiric_embrace  = find_specialization_spell( "Vampiric Embrace" );

  // Class passives
  specs.priest            = find_spell( dbc::get_class_aura_id( PRIEST ) );
  specs.holy_priest       = find_specialization_spell( "Holy Priest" );
  specs.discipline_priest = find_specialization_spell( "Discipline Priest" );
  specs.shadow_priest     = find_specialization_spell( "Shadow Priest" );

  // DoT Spells
  dot_spells.shadow_word_pain    = find_class_spell( "Shadow Word: Pain" );
  dot_spells.vampiric_touch      = find_specialization_spell( "Vampiric Touch", PRIEST_SHADOW );
  dot_spells.shadow_word_madness = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Word: Madness" );

  // Mastery Spells
  mastery_spells.echo_of_light  = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.grace          = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.shadow_weaving = find_mastery_spell( PRIEST_SHADOW );

  // Priest Tree Talents
  // Row 1
  talents.improved_flash_heal = CT( "Improved Flash Heal" );
  talents.angelic_feather     = CT( "Angelic Feather" );
  talents.mind_blast          = CT( "Mind Blast" );
  talents.holy_fire           = CT( "Holy Fire" );
  // Row 2
  talents.holy_nova          = CT( "Holy Nova" );
  talents.holy_nova_heal     = find_spell( 281265 );
  talents.dispel_magic       = CT( "Dispel Magic" );  // NYI
  talents.spiritual_guidance = CT( "Spiritual Guidance" );
  talents.psychic_scream     = CT( "Psychic Scream" );
  // Row 3
  talents.lightburst         = CT( "Lightburst" );
  talents.leap_of_faith      = CT( "Leap of Faith" );   // NYI
  talents.purify_disease     = CT( "Purify Disease" );  // NYI
  talents.power_infusion     = CT( "Power Infusion" );
  talents.painful_invocation = CT( "Painful Invocation" );
  talents.sheer_terror       = CT( "Sheer Terror" );       // NYI
  talents.petrifying_scream  = CT( "Petrifying Scream" );  // NYI
  // Row 4
  talents.surge_of_light             = CT( "Surge of Light" );
  talents.surge_of_light_buff        = find_spell( 114255 );
  talents.body_and_soul              = CT( "Body and Soul" );
  talents.mass_dispel                = CT( "Mass Dispel" );  // NYI
  talents.twins_of_the_sun_priestess = CT( "Twins of the Sun Priestess" );
  talents.strength_of_soul           = CT( "Strength of Soul" );
  talents.mind_control               = CT( "Mind Control" );   // NYI
  talents.dominate_mind              = CT( "Dominant Mind" );  // NYI
  talents.psychic_voice              = CT( "Psychic Voice" );
  talents.void_tendrils              = CT( "Void Tendrils" );  // NYI
  // Row 5
  talents.everlasting_light  = CT( "Everlasting Light" );  // NYI
  talents.move_with_grace    = CT( "Move With Grace" );    // NYI
  talents.mental_agility     = CT( "Mental Agility" );
  talents.twin_disciplines   = CT( "Twin Disciplines" );
  talents.dark_enlightenment = CT( "Dark Enlightenment" );  // NYI
  talents.false_autonomy     = CT( "False Autonomy" );      // NYI
  talents.shackle_undead     = CT( "Shackle Undead" );      // NYI
  // Row 6
  talents.inspiration                   = CT( "Inspiration" );    // NYI
  talents.binding_heals                 = CT( "Binding Heals" );  // NYI
  talents.shadow_word_death             = CT( "Shadow Word: Death" );
  talents.shadow_word_death_self_damage = find_spell( 32409 );
  talents.sanguine_teachings            = CT( "Sanguine Teachings" );
  // Row 7
  talents.desperate_prayer   = CT( "Desperate Prayer" );
  talents.twist_of_fate      = CT( "Twist of Fate" );
  talents.twist_of_fate_buff = find_spell( 390978 );
  talents.tithe_evasion      = CT( "Tithe Evasion" );
  talents.fade               = CT( "Fade" );
  // Row 8
  talents.angels_mercy          = CT( "Angel's Mercy" );
  talents.protective_light      = CT( "Protective Light" );
  talents.protective_light_buff = find_spell( 193065 );
  talents.mindpierce            = CT( "Mindpierce" );         // NYI
  talents.spectral_illusion     = CT( "Spectral Illusion" );  // NYI
  talents.improved_fade         = CT( "Improved Fade" );
  // Row 9
  talents.lights_inspiration = CT( "Light's Inspiration" );
  talents.unwavering_will    = CT( "Unwavering Will" );
  talents.spell_warding      = CT( "Spell Warding" );  // NYI
  talents.phantasm           = CT( "Phantasm" );       // NYI
  // Row 10
  talents.angelic_bulwark   = CT( "Angelic Bulwark" );  // NYI
  talents.benevolence       = CT( "Benevolence" );
  talents.focused_power     = CT( "Focused Power" );  // NYI
  talents.phantom_reach     = CT( "Phantom Reach" );  // NYI
  talents.translucent_image = CT( "Translucent Image" );

  // PvP Talents
  talents.pvp.mindgames                  = find_spell( 375901 );
  talents.pvp.mindgames_healing_reversal = find_spell( 323707 );
  talents.pvp.mindgames_damage_reversal  = find_spell( 323706 );

  // Archon Hero Talents (Holy/Shadow)
  talents.archon.halo                     = HT( "Halo" );
  talents.archon.halo_heal_holy           = find_spell( 120692 );
  talents.archon.halo_dmg_holy            = find_spell( 120696 );
  talents.archon.halo_heal_shadow         = find_spell( 390971 );
  talents.archon.halo_dmg_shadow          = find_spell( 390964 );
  talents.archon.perfected_form           = HT( "Perfected Form" );
  talents.archon.power_surge              = HT( "Power Surge" );
  talents.archon.power_surge_buff         = find_spell( 453112 );
  talents.archon.manifested_power         = HT( "Manifested Power" );
  talents.archon.energy_conservation      = HT( "Energy Conservation" );
  talents.archon.mind_flay_insanity       = ST( "Mind Flay: Insanity" );
  talents.archon.mind_flay_insanity_spell = find_spell( 391403 );  // Not linked to talent, actual dmg spell
  talents.archon.mind_flay_insanity_buff  = find_spell( 391401 );
  talents.archon.shock_pulse              = HT( "Shock Pulse" );  // NYI
  talents.archon.incessant_screams        = HT( "Incessant Screams" );
  talents.archon.word_of_supremacy        = HT( "Word of Supremacy" );
  talents.archon.heightened_alteration    = HT( "Heightened Alteration" );
  talents.archon.empowered_surges         = HT( "Empowered Surges" );
  talents.archon.spiritwell               = HT( "Spiritwell" );
  talents.archon.realized_potential       = HT( "Realized Potential" );
  talents.archon.energy_compression       = HT( "Energy Compression" );
  talents.archon.sustained_potency        = HT( "Sustained Potency" );
  talents.archon.sustained_potency_buff   = find_spell( 454002 );
  talents.archon.resonant_energy          = HT( "Resonant Energy" );
  talents.archon.resonant_energy_shadow   = find_spell( 453850 );
  talents.archon.resonant_energy_healing  = find_spell( 453846 );
  talents.archon.resonant_energy_damage   = find_spell( 453850 );
  talents.archon.energy_cycle             = HT( "Energy Cycle" );
  talents.archon.focused_outburst         = HT( "Focused Outburst" );
  talents.archon.divine_halo              = HT( "Divine Halo" );

  // Oracle Hero Talents (Holy/Discipline)
  talents.oracle.guiding_light         = HT( "Guiding Light" );
  talents.oracle.preventive_measures   = HT( "Preventive Measures" );
  talents.oracle.preemptive_care       = HT( "Preemptive Care" );
  talents.oracle.waste_no_time         = HT( "Waste No Time" );
  talents.oracle.words_of_the_wise     = HT( "Words of the Wise" );
  talents.oracle.assured_safety        = HT( "Assured Safety" );   // NYI
  talents.oracle.divine_feathers       = HT( "Divine Feathers" );  // NYI
  talents.oracle.save_the_day          = HT( "Save the Day" );     // NYI
  talents.oracle.forseen_circumstances = HT( "Forseen Circumstances" );
  talents.oracle.prophets_insight      = HT( "Prophets Insight" );    // NYI
  talents.oracle.prophets_will         = HT( "Prophets Will" );       // NYI
  talents.oracle.desperate_measures    = HT( "Desperate Measures" );  // NYI
  talents.oracle.prompt_prognosis      = HT( "Prompt Prognosis" );    // NYI
  talents.oracle.piety                 = HT( "Piety" );               // NYI
  talents.oracle.unfolding_vision      = HT( "Unfolding Vision" );    // NYI
  talents.oracle.twinsight             = HT( "Twinsight" );           // NYI
  talents.oracle.twinsight_healing     = find_spell( 1232567 );
  talents.oracle.twinsight_damage      = find_spell( 1232571 );

  // Voidweaver Hero Talents (Discipline/Shadow)
  talents.voidweaver.void_torrent           = HT( "Void Torrent" );   // Shadow Only
  talents.voidweaver.entropic_rift          = HT( "Entropic Rift" );  // Discipline Only
  talents.voidweaver.entropic_rift_aoe      = find_spell( 450193 );   // Contains AoE radius info
  talents.voidweaver.entropic_rift_damage   = find_spell( 447448 );   // Contains damage coeff
  talents.voidweaver.entropic_rift_driver   = find_spell( 459314 );   // Contains damage coeff
  talents.voidweaver.entropic_rift_object   = find_spell( 447445 );   // Contains spell radius
  talents.voidweaver.no_escape              = HT( "No Escape" );      // NYI
  talents.voidweaver.dark_energy            = HT( "Dark Energy" );
  talents.voidweaver.void_blast             = HT( "Void Blast" );
  talents.voidweaver.void_blast_shadow      = find_spell( 450983 );
  talents.voidweaver.void_blast_disc        = find_spell( 450215 );
  talents.voidweaver.inner_quietus          = HT( "Inner Quietus" );
  talents.voidweaver.voidheart              = HT( "Voidheart" );
  talents.voidweaver.voidheart_buff         = find_spell( 449887 );
  talents.voidweaver.devour_matter          = HT( "Devour Matter" );
  talents.voidweaver.void_empowerment       = HT( "Void Empowerment" );
  talents.voidweaver.void_empowerment_buff  = find_spell( 450140 );
  talents.voidweaver.darkening_horizon      = HT( "Darkening Horizon" );
  talents.voidweaver.voidwraith             = HT( "Voidwraith" );
  talents.voidweaver.voidwraith_spell       = find_spell( 451235 );
  talents.voidweaver.touch_of_the_void      = HT( "Touch of the Void" );
  talents.voidweaver.quickened_pulse        = HT( "Quickened Pulse" );
  talents.voidweaver.void_infusion          = HT( "Void Infusion" );
  talents.voidweaver.void_leech             = HT( "Void Leech" );          // NYI
  talents.voidweaver.embrace_the_shadow     = HT( "Embrace the Shadow" );  // NYI
  talents.voidweaver.overwhelming_shadows   = HT( "Overwhelming Shadows" );
  talents.voidweaver.collapsing_void        = HT( "Collapsing Void" );
  talents.voidweaver.collapsing_void_damage = find_spell( 448405 );

  if ( specialization() == PRIEST_SHADOW )
    deregister_passive_effect( talents.voidweaver.overwhelming_shadows->effectN( 2 ) );

  // Register passives
  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();
  parse_raid_buffs();
}

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Generic buffs
  buffs.desperate_prayer  = make_buff<buffs::desperate_prayer_t>( *this );
  buffs.power_word_shield = new buffs::power_word_shield_buff_t( this, this );
  buffs.fade              = make_buff( this, "fade", find_class_spell( "Fade" ) )->set_default_value_from_effect( 4 );
  buffs.levitate          = make_buff( this, "levitate", specs.levitate_buff )->set_duration( timespan_t::zero() );

  // Shared talent buffs
  // Does not show damage value on the buff spelldata, that is only found on the talent
  buffs.twist_of_fate = make_buff( this, "twist_of_fate", talents.twist_of_fate_buff )
                            ->set_trigger_spell( talents.twist_of_fate )
                            ->set_default_value_from_effect( 1 )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                            ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buffs.twist_of_fate_heal_ally_fake = make_buff( this, "twist_of_fate_can_trigger_on_ally_heal" )
                                           ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                                           ->set_proc_callbacks( false );

  // TODO: Extend functionality to use this.
  buffs.twist_of_fate_heal_self_fake = make_buff( this, "twist_of_fate_can_trigger_on_self_heal" )
                                           ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                                           ->set_proc_callbacks( false );

  buffs.protective_light =
      make_buff( this, "protective_light", talents.protective_light_buff )->set_default_value_from_effect( 1 );

  buffs.surge_of_light = make_buff( this, "surge_of_light", talents.surge_of_light_buff )
                             ->set_chance( talents.surge_of_light->effectN( 1 ).percent() );

  // Voidweaver
  buffs.voidheart = make_buff( this, "voidheart", talents.voidweaver.voidheart_buff )
                        ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  // Tracking buff for the APL
  buffs.entropic_rift =
      make_buff_fallback( talents.voidweaver.entropic_rift.ok() || talents.voidweaver.void_torrent.ok(), this,
                          "entropic_rift", talents.voidweaver.entropic_rift_driver );

  if ( talents.voidweaver.entropic_rift.ok() || talents.voidweaver.void_torrent.ok() )
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
            buffs.darkening_horizon->expire();
            background_actions.collapsing_void->trigger( state.last_entropic_rift_target,
                                                         buffs.collapsing_void->check() );
            buffs.collapsing_void->expire();
            buffs.voidheart->expire();

            if ( talents.voidweaver.touch_of_the_void.enabled() )
            {
              buffs.voidheart->trigger(
                  timespan_t::from_seconds( talents.voidweaver.touch_of_the_void->effectN( 1 ).base_value() ) );
            }

            if ( talents.voidweaver.voidwraith.enabled() )
            {
              pets.voidwraith.spawn();
            }
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
                              ->set_duration( 0_s )
                              ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                              ->set_max_stack( specialization() == PRIEST_SHADOW ? 5 : 10 );

  // Unknown what this piece of spell data is for. Discipline testing shows a maximum of 10 stacks.
  /*if ( talents.voidweaver.collapsing_void.enabled() )
  {
    buffs.collapsing_void->set_max_stack(
        static_cast<int>( talents.voidweaver.collapsing_void->effectN( 2 ).base_value() ) );
  }*/

  // Archon
  buffs.power_surge =
      make_buff_fallback( talents.archon.power_surge.enabled(), this, "power_surge", talents.archon.power_surge_buff )
          ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { background_actions.halo->execute(); } );

  buffs.sustained_potency = make_buff_fallback( talents.archon.sustained_potency.enabled(), this, "sustained_potency",
                                                talents.archon.sustained_potency_buff );

  buffs.resonant_energy_healing =
      make_buff_fallback( talents.archon.resonant_energy.enabled() && is_ptr(), this, "resonant_energy_healing",
                          talents.archon.resonant_energy_healing );

  buffs.resonant_energy_damage = make_buff_fallback( talents.archon.resonant_energy.enabled() && is_ptr(), this,
                                                     "resonant_energy_damage", talents.archon.resonant_energy_damage );

  buffs.mind_flay_insanity = make_buff( this, "mind_flay_insanity", talents.archon.mind_flay_insanity_buff );

  create_buffs_shadow();
  create_buffs_discipline();
  create_buffs_holy();
}

void priest_t::init_uptimes()
{
  player_t::init_uptimes();

  if ( talents.shadow.voidform.enabled() )
  {
    sample_data.voidform_duration = std::make_unique<extended_sample_data_t>( "Voidform duration", false );
  }
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

  background_actions.shadow_word_death = new actions::spells::shadow_word_death_t( *this, 200_ms );
  background_actions.atonement         = new actions::heals::atonement_t( *this );
  background_actions.halo              = new actions::spells::halo_t( *this, true );

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

bool priest_t::validate_actor()
{
  switch ( specialization() )
  {
    case PRIEST_HOLY:
    case PRIEST_DISCIPLINE:
      if ( !sim->allow_experimental_specializations )
      {
        if ( !quiet )
          sim->error( "Healing Priest specializations for {} are not currently supported.", *this );
        return false;
      }
    case PRIEST_SHADOW:
      break;
    default:
      assert( false );
  }

  return player_t::validate_actor();
}

void priest_t::do_dynamic_regen( bool forced )
{
  player_t::do_dynamic_regen( forced );
}

double priest_t::composite_mastery_value() const
{
  auto m = player_t::composite_mastery_value();

  if ( specialization() == PRIEST_HOLY && talents.holy.prismatic_echoes.enabled() )
    m *= 1 + talents.holy.prismatic_echoes->effectN( 1 ).percent();

  return m;
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

  // Special Handling for DF S4
  set_bonus_type_e tier_to_enable;
  switch ( specialization() )
  {
    case PRIEST_DISCIPLINE:
      tier_to_enable = DF3;
      break;
    case PRIEST_HOLY:
      tier_to_enable = DF3;
      break;
    case PRIEST_SHADOW:
      tier_to_enable = DF2;
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

void priest_t::init_blizzard_action_list()
{
  [[maybe_unused]] action_priority_list_t* default_ = get_action_priority_list( "default" );
  player_t::init_blizzard_action_list();

  // default overrides
  switch ( specialization() )
  {
    case PRIEST_DISCIPLINE:
      break;
    case PRIEST_HOLY:
      break;
    case PRIEST_SHADOW:
      break;
    default:
      break;
  }

  // precombat overrides
  action_priority_list_t* pre_c = get_action_priority_list( "precombat" );

  switch ( specialization() )
  {
    case PRIEST_DISCIPLINE:
      pre_c->add_action( "smite" );
      break;
    case PRIEST_HOLY:
      pre_c->add_action( "holy_fire" );
      pre_c->add_action( "smite" );
      break;
    case PRIEST_SHADOW:
      pre_c->add_action( "shadowform,if=!buff.shadowform.up" );
      break;
    default:
      break;
  }

  // cooldown overrides
  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );
  // reset this from player.cpp to get a better use_items
  cooldowns->action_list.clear();

  // let the potion global option control this
  cooldowns->add_action( "potion" );

  // check setting to see if cooldowns should be added
  if ( use_cds_with_blizzard_action_list )
  {
    cooldowns->add_action( "blood_fury" );
    cooldowns->add_action( "berserking" );
    cooldowns->add_action( "fireblood" );
    cooldowns->add_action( "ancestral_call" );

    switch ( specialization() )
    {
      case PRIEST_DISCIPLINE:
        cooldowns->add_action( "use_items" );
        cooldowns->add_action( "power_infusion" );
        break;
      case PRIEST_HOLY:
        cooldowns->add_action( "use_items" );
        cooldowns->add_action( "halo,if=talent.power_surge" );
        cooldowns->add_action( "apotheosis" );
        cooldowns->add_action( "power_infusion" );
        break;
      case PRIEST_SHADOW:
        cooldowns->add_action( "use_items,if=buff.voidform.up" );
        cooldowns->add_action( "voidform,if=dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking" );
        cooldowns->add_action( "power_infusion,if=buff.voidform.up" );
        break;
      default:
        break;
    }
  }
}

parsed_assisted_combat_rule_t priest_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                    const assisted_combat_step_data_t& step ) const
{
  // vampiric touch action checks if shadow crash is available
  if ( rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_1 == 1243723 )
  {
    return { "(!action.tentacle_slam.in_flight)" };
  }

  // instead of checking for hidden void blast buff we check for entropic rift
  if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 450404 )
  {
    return { "buff.entropic_rift.up" };
  }

  // guard against buff.power_word_fortitude_highlight, base sim handles this
  if ( step.spell_id == 21562 && rule.condition_value_1 == 1271911 )
  {
    return { "aura.power_word_fortitude.down" };
  }

  return player_t::parse_assisted_combat_rule( rule, step );
}

std::string priest_t::blizzard_apl_action_replace( std::string options )
{
  switch ( specialization() )
  {
    case PRIEST_DISCIPLINE:
      break;
    case PRIEST_HOLY:
      break;
    case PRIEST_SHADOW:
      // override mind_blast into void_blast
      if ( options.find( "buff.entropic_rift.up" ) != std::string::npos )
      {
        return "void_blast";
      }
      // ensure we use mind_flay_insanity action instead of mind_flay
      if ( options.find( "mind_flay_insanity" ) != std::string::npos )
      {
        return "mind_flay_insanity";
      }
      // void_eruption into void_volley
      if ( options.find( "buff.voidform.up" ) != std::string::npos )
      {
        return "void_volley";
      }
      break;
    default:
      break;
  }

  return "";
}

void priest_t::parse_assisted_combat_step( const assisted_combat_step_data_t& step,
                                           action_priority_list_t* assisted_combat )
{
  std::string expr                    = "";
  std::string base_expr               = "";
  std::string comment                 = "";
  bool show_diff                      = false;
  bool cooldown_allow_casting_success = false;
  for ( const auto& rule : assisted_combat_rule_data_t::data( step.id, is_ptr() ) )
  {
    if ( rule.condition_type == AC_COOLDOWN_ALLOW_CASTING_SUCCESS )
      cooldown_allow_casting_success = true;

    parsed_assisted_combat_rule_t derived_combat_rule = parse_assisted_combat_rule( rule, step );
    parsed_assisted_combat_rule_t base_combat_rule    = player_t::parse_assisted_combat_rule( rule, step );

    if ( !derived_combat_rule.expr.empty() )
      expr += expr.empty() ? derived_combat_rule.expr : "&" + derived_combat_rule.expr;
    if ( !derived_combat_rule.comment.empty() )
      comment += comment.empty() ? derived_combat_rule.comment : ", " + derived_combat_rule.comment;
    if ( !base_combat_rule.expr.empty() )
      base_expr += base_expr.empty() ? base_combat_rule.expr : "&" + base_combat_rule.expr;

    show_diff |= derived_combat_rule.show_diff;
  }

  if ( base_expr != expr && show_diff )
    comment += ( comment.empty() ? "" : " " ) + fmt::format( "(Overridden from '{}')", base_expr );

  // This is kinda ugly, maybe find a better way to do this?
  if ( !expr.empty() )
  {
    std::string name = blizzard_apl_action_replace( expr );
    if ( !name.empty() )
    {
      std::string action_str = name;

      if ( !expr.empty() )
        action_str += ",if=" + expr;

      if ( cooldown_allow_casting_success )
        action_str += ",cooldown_allow_casting_success=1";

      assisted_combat->add_action( action_str, comment );
      return;
    }
  }

  for ( const auto& name : action_names_from_spell_id( step.spell_id ) )
  {
    if ( name.empty() )
      continue;

    std::string action_str = name;

    if ( !expr.empty() )
      action_str += ",if=" + expr;

    if ( cooldown_allow_casting_success )
      action_str += ",cooldown_allow_casting_success=1";

    assisted_combat->add_action( action_str, comment );
  }
}

std::vector<std::string> priest_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  if ( spell_id == 8092 && specialization() == PRIEST_HOLY )
    return { "holy_fire" };

  if ( spell_id == 585 && specialization() == PRIEST_DISCIPLINE )
    return { "void_blast", "smite" };

  if ( spell_id == 15407 )
  {
    return { "mind_flay_insanity", "mind_flay" };
  }

  return player_t::action_names_from_spell_id( spell_id );
}

std::string priest_t::aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const
{
  std::string aura_expr = player_t::aura_expr_from_spell_id( spell_id, on_self );

  return aura_expr;
}

void priest_t::combat_begin()
{
  player_t::combat_begin();

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

void priest_t::create_options()
{
  base_t::create_options();

  add_option( opt_bool( "priest.mindgames_healing_reversal", options.mindgames_healing_reversal ) );
  add_option( opt_bool( "priest.mindgames_damage_reversal", options.mindgames_damage_reversal ) );
  add_option( opt_bool( "priest.self_power_infusion", options.self_power_infusion ) );
  // Default is 2, minimum of 1 bounce per second, maximum of 1 bounce per 12 seconds (prayer of mending's cooldown)
  add_option( opt_float( "priest.prayer_of_mending_bounce_rate", options.prayer_of_mending_bounce_rate, 1, 12 ) );
  add_option( opt_bool( "priest.init_insanity", options.init_insanity ) );
  add_option( opt_float( "priest.twist_of_fate_heal_rppm", options.twist_of_fate_heal_rppm, 0, 120 ) );
  add_option( opt_timespan( "priest.twist_of_fate_heal_duration_mean", options.twist_of_fate_heal_duration_mean, 0_s,
                            timespan_t::max() ) );
  add_option( opt_timespan( "priest.twist_of_fate_heal_duration_stddev", options.twist_of_fate_heal_duration_stddev,
                            0_s, timespan_t::max() ) );
  add_option( opt_bool( "priest.force_devour_matter", options.force_devour_matter ) );
  add_option( opt_float( "priest.entropic_rift_miss_percent", options.entropic_rift_miss_percent, 0.0, 1.0 ) );
  add_option( opt_float( "priest.entropic_rift_miss_percent_secondary", options.entropic_rift_miss_percent_secondary,
                         0.0, 1.0 ) );
  add_option( opt_int( "priest.entropic_rift_miss_target_cap", options.entropic_rift_miss_target_cap, 0, 100 ) );
  add_option(
      opt_float( "priest.crystalline_reflection_damage_mult", options.crystalline_reflection_damage_mult, 0.0, 1.0 ) );
  add_option( opt_bool( "priest.no_channel_macro_mfi", options.no_channel_macro_mfi ) );
  add_option( opt_bool( "priest.discipline_in_raid", options.discipline_in_raid ) );
  add_option( opt_float( "priest.synergistic_brewterializer_tof_chance", options.synergistic_brewterializer_tof_chance,
                         0.0, 1.0 ) );
  add_option( opt_float( "priest.synergistic_brewterializer_barrel_hit_chance",
                         options.synergistic_brewterializer_barrel_hit_chance, 0.0, 1.0 ) );
  add_option(
      opt_float( "priest.archon_halo_outgoing_hit_chance", options.archon_halo_outgoing_hit_chance, 0.0, 1.0 ) );
  add_option( opt_float( "priest.archon_halo_return_hit_chance", options.archon_halo_return_hit_chance, 0.0, 1.0 ) );
  add_option( opt_bool( "priest.mid_s2_2pc", options.mid_s2_2pc ) );
  add_option( opt_bool( "priest.mid_s2_4pc", options.mid_s2_4pc ) );
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

void priest_t::merge( player_t& other )
{
  player_t::merge( other );

  priest_t& priest = dynamic_cast<priest_t&>( other );

  if ( talents.shadow.voidform.enabled() )
  {
    sample_data.voidform_duration->merge( *priest.sample_data.voidform_duration );
  }
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
  if ( !talents.shadow.idol_of_cthun.enabled() && !talents.shadow.void_apparitions_3.enabled() )
    return;

  if ( rppm.idol_of_cthun->trigger() )
  {
    spawn_idol_of_cthun( s );
  }
}

// Trigger Atonement
void priest_t::trigger_atonement( action_state_t* s, double mul )
{
  if ( !talents.discipline.atonement.enabled() )
    return;

  if ( allies_with_atonement.empty() )
    return;

  if ( s->result_amount <= 0 )
    return;

  auto r = s->result_amount;

  r *= mul;

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

void priest_t::spawn_idol_of_cthun( action_state_t* s )
{
  if ( !talents.shadow.idol_of_cthun.enabled() && !talents.shadow.void_apparitions_3.enabled() )
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

  if ( talents.shadow.void_apparitions_1.enabled() )
  {
    // BUG: This does not pass through target for Shadeburst currently
    trigger_shadowy_apparitions( procs.shadowy_apparition_cthun, nullptr );
  }
}

double priest_t::shadow_weaving_active_dots( const player_t* target, const unsigned int spell_id ) const
{
  double dots = 0.0;

  if ( buffs.voidform->check() )
  {
    dots = 3.0;

    // Voidform gets this benefit automatically - 6/22/2025
    if ( talents.shadow.madness_weaving.enabled() )
    {
      dots += talents.shadow.madness_weaving->effectN( 1 ).percent();
    }
  }
  else
  {
    if ( const priest_td_t* td = find_target_data( target ) )
    {
      const dot_t* swp = td->dots.shadow_word_pain;
      const dot_t* vt  = td->dots.vampiric_touch;
      const dot_t* swm = td->dots.shadow_word_madness;

      // You get mastery benefit for a DoT as if it was active, if you are actively putting that DoT up
      // So to get the mastery benefit you either have that DoT ticking, or you are casting it
      bool swp_ticking = ( spell_id == dot_spells.shadow_word_pain->id() ) || swp->is_ticking();
      bool vt_ticking  = ( spell_id == dot_spells.vampiric_touch->id() ) || vt->is_ticking();
      bool swm_ticking = ( spell_id == dot_spells.shadow_word_madness->id() ) || swm->is_ticking();

      dots = swp_ticking + vt_ticking + swm_ticking;

      if ( swm_ticking && talents.shadow.madness_weaving.enabled() )
      {
        dots += talents.shadow.madness_weaving->effectN( 1 ).percent();
      }
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

    if ( dots > 0.0 )
    {
      multiplier *= 1.0 + dots * cache.mastery_value();
    }
  }

  return multiplier;
}

void priest_t::trigger_entropic_rift()
{
  // Spawn Entropic Rift
  background_actions.entropic_rift->execute();
}

void priest_t::expand_entropic_rift( int stacks )
{
  if ( !talents.voidweaver.collapsing_void.enabled() || !buffs.entropic_rift->check() )
  {
    return;
  }

  buffs.collapsing_void->trigger( stacks );
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

    buffs.entropic_rift->extend_duration( extension );

    if ( buffs.voidheart->check() )
    {
      buffs.voidheart->extend_duration( extension );
    }

    if ( buffs.darkening_horizon->check() )
    {
      buffs.darkening_horizon->extend_duration( extension );
      buffs.darkening_horizon->increment();
    }
    else
    {
      buffs.darkening_horizon->trigger();
    }
  }
}

class priest_report_t final : public player_report_extension_t
{
public:
  priest_report_t( priest_t& player ) : p( player )
  {
  }

  void shadowy_apparition_piechart( report::sc_html_stream& os )
  {
    highchart::pie_chart_t shadowy_apparition_sources( highchart::build_id( p, "shadowy_apparition_sources" ), *p.sim );
    shadowy_apparition_sources.set_title( "Shadowy Apparition Sources" );
    shadowy_apparition_sources.set( "plotOptions.pie.dataLabels.format", "{point.name}: {point.percentage:.1f}%" );

    std::vector<proc_t*> sa_source_list = {
        p.procs.shadowy_apparition_swp,  p.procs.shadowy_apparition_swm,     p.procs.shadowy_apparition_mb,
        p.procs.shadowy_apparition_mfi,  p.procs.shadowy_apparition_yshaarj, p.procs.shadowy_apparition_nzoth,
        p.procs.shadowy_apparition_yogg, p.procs.shadowy_apparition_cthun,   p.procs.shadowy_apparition_mid_s2_4pc_vt,
    };

    double sum = 0.0;

    // Get total amount of Shadowy Apparitions
    for ( size_t i = 0; i < sa_source_list.size(); ++i )
    {
      const auto& entry = sa_source_list[ i ];

      sum += entry->count.mean();
    }

    // Sort the dataset so it looks better in the chart
    range::sort( sa_source_list, []( const auto& left, const auto& right ) {
      if ( left->count.mean() == right->count.mean() )
      {
        return left->name_str < right->name_str;
      }

      return left->count.mean() > right->count.mean();
    } );

    // Populate the pie chart with each entry
    range::for_each( sa_source_list, [ this, &shadowy_apparition_sources, sum ]( const auto& entry ) {
      if ( entry->count.mean() == 0.0 )
        return;

      std::string prefix = "Shadowy Apparition from ";
      color::rgb color   = color::school_color( SCHOOL_SHADOW );

      js::sc_js_t e;
      e.set( "color", color.str() );
      e.set( "y", util::round( ( entry->count.mean() / sum ) * 100, p.sim->report_precision ) );
      e.set( "name", report_decorators::decorate_html_string(
                         util::encode_html( entry->name_str.substr( prefix.length() ) ), color ) );

      shadowy_apparition_sources.add( "series.0.data", e );
    } );

    os << shadowy_apparition_sources.to_target_div();
    p.sim->add_chart_data( shadowy_apparition_sources );
  }

  void html_customsection_shadowy_apparitions( report::sc_html_stream& os )
  {
    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Shadowy Apparitions</h3>\n"
          "<div class=\"toggle-content\">\n";

    shadowy_apparition_piechart( os );

    os << "</div>\n"
          "</div>\n";
  }

  void html_customsection_voidform( report::sc_html_stream& os )
  {
    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Voidform</h3>\n"
          "<div class=\"toggle-content\">\n";

    auto& d         = *p.sample_data.voidform_duration;
    int num_buckets = std::min( 30, static_cast<int>( 2 * ( d.max() - d.min() ) ) + 1 );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "voidform_duration" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Voidform Duration", d.mean(), d.min(), d.max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Statistics</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>75<sup>th</sup> percentile</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os << "<tr>\n";
    os.printf( "<td class=\"right\">%.3f</td>", d.min() );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", d.mean(), d.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .75 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.max() );
    os << "</tr>\n";
    os << "</table>\n";

    os << "</div>\n"
          "</div>\n";
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.sim->report_details == 0 )
      return;

    // Only show this when you are able to extend Voidform
    if ( p.talents.shadow.ancient_madness.enabled() || p.talents.archon.sustained_potency.enabled() )
    {
      html_customsection_voidform( os );
    }

    if ( p.talents.shadow.shadowy_apparitions.enabled() )
    {
      html_customsection_shadowy_apparitions( os );
    }
  }

private:
  priest_t& p;
};

struct priest_module_t final : public module_t
{
  priest_module_t() : module_t( PRIEST )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new priest_t( sim, name, r );
    p->report_extension = std::make_unique<priest_report_t>( *p );
    return p;
  }
  bool valid() const override
  {
    return true;
  }
  void register_actor_initializers( sim_t* sim ) const override
  {
    sim->register_actor_initializer(
        INIT_ACTOR_CREATE_BUFFS + offset(),
        []( player_t* p ) {
          if ( !p->is_player() )
            return;

          // Always create PI as it's a commonly simmed external
          auto pi_buff = make_buff( p, "power_infusion", p->find_spell( 10060 ) )
                             ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                             ->set_default_value_from_effect_type( A_HASTE_ALL )
                             ->set_cooldown( 0_ms );

          if ( p->type == PRIEST )
          {
            debug_cast<priest_t*>( p )->buffs.power_infusion = pi_buff;
          }

          if ( !p->external_buffs.power_infusion.empty() )
          {
            p->register_timed_buff_triggers( pi_buff, p->external_buffs.power_infusion );
          }
        },
        "create_buffs_priest" );
  }
  void static_init() const override
  {
    items::init();
  }
  void register_hotfixes() const override
  {
  }
};

}  // namespace priestspace

const module_t* module_t::priest()
{
  static priestspace::priest_module_t m;
  return &m;
}

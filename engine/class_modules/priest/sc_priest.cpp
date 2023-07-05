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
        auto adjust_percent = priest().talents.discipline.schism->effectN( 2 ).percent();
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
struct mind_blast_t final : public priest_spell_t
{
private:
  double mind_blast_insanity;
  timespan_t manipulation_cdr;
  timespan_t void_summoner_cdr;
  propagate_const<expiation_t*> child_expiation;

public:
  mind_blast_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mind_blast", p, p.specs.mind_blast ),
      mind_blast_insanity( p.specs.shadow_priest->effectN( 9 ).resource( RESOURCE_INSANITY ) ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) ),
      void_summoner_cdr( priest().talents.discipline.void_summoner->effectN( 2 ).time_value() ),
      child_expiation( nullptr )
  {
    parse_options( options_str );

    affected_by_shadow_weaving = true;
    cooldown->hasted           = true;

    // This was removed from the Mind Blast spell and put on the Shadow Priest spell instead
    energize_amount = mind_blast_insanity;
    if ( priest().talents.discipline.expiation.enabled() )
    {
      child_expiation             = new expiation_t( priest() );
      child_expiation->background = true;
    }

    apply_affecting_aura( p.talents.shadow.thought_harvester );
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.manipulation.enabled() &&
         ( priest().specialization() == PRIEST_SHADOW || priest().specialization() == PRIEST_DISCIPLINE ) )
    {
      priest().cooldowns.mindgames->adjust( -manipulation_cdr );
    }

    if ( priest().talents.discipline.void_summoner.enabled() )
    {
      priest().cooldowns.mindbender->adjust( void_summoner_cdr );
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

      if ( priest().talents.shadow.inescapable_torment.enabled() )
      {
        priest().trigger_inescapable_torment( s->target );
      }

      if ( priest().buffs.mind_devourer->trigger() )
      {
        priest().procs.mind_devourer->occur();
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

    if ( priest().talents.discipline.harsh_discipline.enabled() )
    {
      priest().buffs.harsh_discipline->trigger();
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
// TODO: add reduced healing beyond 6 targets
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

    // This is not found in the affected spells for Dark Ascension, overriding it manually
    force_buff_effect( p.buffs.dark_ascension, 1 );
    // This is not found in the affected spells for Shadow Covenant, overriding it manually
    // Final two params allow us to override the 25% damage buff when twilight corruption is selected (25% -> 35%)
    force_buff_effect( p.buffs.shadow_covenant, 1, false, true );
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

    proc = background = true;
  }

  // Hits twice, but only if you are at the correct distance
  // 24 yards or less for 2 hits, 28 yards or less for 1 hit
  void execute() override
  {
    double distance;

    distance = priest_heal_t::player->get_player_distance( *target );

    if ( distance <= 28 )
    {
      priest_heal_t::execute();

      if ( return_spell && distance <= 24 )
      {
        return_spell->execute();
      }
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
// TODO: add reduced healing beyond 5 targets
// ==========================================================================
struct halo_spell_t final : public priest_spell_t
{
  halo_spell_t( util::string_view n, priest_t& p, const spell_data_t* s ) : priest_spell_t( n, p, s )
  {
    aoe                        = -1;
    background                 = true;
    radius                     = data().max_range();
    range                      = 0;
    travel_speed               = 15;  // Rough estimate, 2021-01-03
    affected_by_shadow_weaving = true;

    // This is not found in the affected spells for Dark Ascension, overriding it manually
    force_buff_effect( p.buffs.dark_ascension, 1 );
    // This is not found in the affected spells for Shadow Covenant, overriding it manually
    // Final two params allow us to override the 25% damage buff when twilight corruption is selected (25% -> 35%)
    force_buff_effect( p.buffs.shadow_covenant, 1, false, true );
  }
};

struct halo_heal_t final : public priest_heal_t
{
  halo_heal_t( util::string_view n, priest_t& p, const spell_data_t* s ) : priest_heal_t( n, p, s )
  {
    aoe          = -1;
    background   = true;
    radius       = data().max_range();
    range        = 0;
    travel_speed = 15;  // Rough estimate, 2021-01-03
  }
};

struct halo_t final : public priest_spell_t
{
  halo_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "halo", p, p.talents.halo ),
      _heal_spell_holy( new halo_heal_t( "halo_heal_holy", p, p.talents.halo_heal_holy ) ),
      _dmg_spell_holy( new halo_spell_t( "halo_damage_holy", p, p.talents.halo_dmg_holy ) ),
      _heal_spell_shadow( new halo_heal_t( "halo_heal_shadow", p, p.talents.halo_heal_shadow ) ),
      _dmg_spell_shadow( new halo_spell_t( "halo_damage_shadow", p, p.talents.halo_dmg_shadow ) )
  {
    parse_options( options_str );

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
  propagate_const<action_t*> _heal_spell_holy;
  propagate_const<action_t*> _dmg_spell_holy;
  propagate_const<action_t*> _heal_spell_shadow;
  propagate_const<action_t*> _dmg_spell_shadow;
};

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
      sim->auras.power_word_fortitude->trigger();
  }
};

// ==========================================================================
// Smite
// ==========================================================================
struct smite_t final : public priest_spell_t
{
  timespan_t manipulation_cdr;
  timespan_t void_summoner_cdr;
  timespan_t train_of_thought_cdr;
  propagate_const<action_t*> child_holy_fire;
  action_t* child_searing_light;

  smite_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "smite", p, p.find_class_spell( "Smite" ) ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) ),
      void_summoner_cdr( priest().talents.discipline.void_summoner->effectN( 2 ).time_value() ),
      train_of_thought_cdr( priest().talents.discipline.train_of_thought->effectN( 2 ).time_value() ),
      child_holy_fire( priest().background_actions.holy_fire ),
      child_searing_light( priest().background_actions.searing_light )
  {
    parse_options( options_str );
    if ( priest().talents.holy.divine_word.enabled() )
    {
      child_holy_fire->background = true;
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( s );
    if ( priest().buffs.wrath_unleashed->check() )
    {
      d *= 1.0 + priest().buffs.wrath_unleashed->data().effectN( 1 ).percent();
      sim->print_debug( "Smite damage modified by {} (new total: {}), from wrath_unleashed",
                        priest().buffs.wrath_unleashed->data().effectN( 1 ).percent(), d );
    }
    if ( priest().buffs.weal_and_woe->check() )
    {
      d *= 1.0 +
           ( priest().buffs.weal_and_woe->data().effectN( 1 ).percent() * priest().buffs.weal_and_woe->current_stack );
      sim->print_debug(
          "Smite damage modified by {} (new total: {}), from weal_and_woe",
          priest().buffs.weal_and_woe->data().effectN( 1 ).percent() * priest().buffs.weal_and_woe->current_stack, d );
    }
    if ( priest().talents.discipline.blaze_of_light.enabled() )
    {
      d *= 1.0 + ( priest().talents.discipline.blaze_of_light->effectN( 1 ).percent() );
      sim->print_debug( "Smite damage modified by {} (new total: {}), from blaze_of_light",
                        priest().talents.discipline.blaze_of_light->effectN( 1 ).percent(), d );
    }
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

  timespan_t execute_time() const override
  {
    timespan_t et = priest_spell_t::execute_time();

    if ( priest().talents.unwavering_will.enabled() && priest().specialization() != PRIEST_SHADOW &&
         priest().health_percentage() > priest().talents.unwavering_will->effectN( 2 ).base_value() )
    {
      et *= 1 + priest().talents.unwavering_will->effectN( 1 ).percent();
    }

    return et;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.manipulation.enabled() &&
         ( priest().specialization() == PRIEST_HOLY || priest().specialization() == PRIEST_DISCIPLINE ) )
    {
      priest().cooldowns.mindgames->adjust( -manipulation_cdr );
    }

    if ( priest().talents.discipline.void_summoner.enabled() )
    {
      priest().cooldowns.mindbender->adjust( void_summoner_cdr );
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
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
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
    if ( priest().talents.discipline.harsh_discipline.enabled() )
    {
      priest().buffs.harsh_discipline->trigger();
    }

    if ( priest().talents.discipline.weal_and_woe.enabled() && priest().buffs.weal_and_woe->check() )
    {
      priest().buffs.weal_and_woe->expire();
    }
  }
};

// ==========================================================================
// Power Infusion
// ==========================================================================
struct power_infusion_t final : public priest_spell_t
{
  power_infusion_t( priest_t& p, util::string_view options_str, util::string_view name )
    : priest_spell_t( name, p, p.talents.power_infusion )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    // Trigger PI on the actor only if casting on itself
    if ( priest().options.self_power_infusion || priest().talents.twins_of_the_sun_priestess->ok() )
    {
      player->buffs.power_infusion->trigger();
    }
  }
};

// ==========================================================================
// Mindgames
// ==========================================================================
struct mindgames_healing_reversal_t final : public priest_spell_t
{
  mindgames_healing_reversal_t( priest_t& p )
    : priest_spell_t( "mindgames_healing_reversal", p, p.talents.mindgames_healing_reversal )
  {
    background        = true;
    may_crit          = false;
    energize_type     = action_energize::NONE;  // disable insanity gain (parent spell covers this)
    energize_amount   = 0;
    energize_resource = RESOURCE_NONE;

    // Formula found in parent spelldata for $healing
    // $healing=${($SPS*$s5/100)*(1+$@versadmg)*$m3/100}
    spell_power_mod.direct = ( priest().talents.mindgames->effectN( 5 ).base_value() / 100 ) *
                             ( priest().talents.mindgames->effectN( 3 ).base_value() / 100 );

    if ( priest().talents.shattered_perceptions.enabled() )
    {
      base_dd_multiplier *= ( 1.0 + priest().talents.shattered_perceptions->effectN( 1 ).percent() );
    }
  }
};

struct mindgames_damage_reversal_t final : public priest_heal_t
{
  mindgames_damage_reversal_t( priest_t& p )
    : priest_heal_t( "mindgames_damage_reversal", p, p.talents.mindgames_damage_reversal )
  {
    background        = true;
    harmful           = false;
    may_crit          = false;
    energize_type     = action_energize::NONE;  // disable insanity gain (parent spell covers this)
    energize_amount   = 0;
    energize_resource = RESOURCE_NONE;

    // Formula found in parent spelldata for $damage
    // $damage=${($SPS*$s2/100)*(1+$@versadmg)*$m3/100}
    spell_power_mod.direct = ( priest().talents.mindgames->effectN( 2 ).base_value() / 100 ) *
                             ( priest().talents.mindgames->effectN( 3 ).base_value() / 100 );

    if ( priest().talents.shattered_perceptions.enabled() )
    {
      base_dd_multiplier *= ( 1.0 + priest().talents.shattered_perceptions->effectN( 1 ).percent() );
    }
  }
};

struct mindgames_t final : public priest_spell_t
{
  propagate_const<mindgames_healing_reversal_t*> child_mindgames_healing_reversal;
  propagate_const<mindgames_damage_reversal_t*> child_mindgames_damage_reversal;
  propagate_const<action_t*> child_searing_light;

  mindgames_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mindgames", p, p.talents.mindgames ),
      child_mindgames_healing_reversal( nullptr ),
      child_mindgames_damage_reversal( nullptr ),
      child_searing_light( priest().background_actions.searing_light )
  {
    parse_options( options_str );

    affected_by_shadow_weaving = true;

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

    if ( priest().talents.shattered_perceptions.enabled() )
    {
      base_dd_multiplier *= ( 1.0 + priest().talents.shattered_perceptions->effectN( 1 ).percent() );
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

  summon_fiend_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( p.talents.shadow.mindbender.enabled() ? "mindbender" : "shadowfiend", p,
                      p.talents.shadow.mindbender.enabled() ? p.talents.shadow.mindbender : p.talents.shadowfiend ),
      default_duration( data().duration() ),
      spawner( p.talents.shadow.mindbender.enabled() ? &p.pets.mindbender : &p.pets.shadowfiend )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( spawner )
      spawner->spawn( default_duration );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    make_event( sim, [ this, s ] { priest().trigger_idol_of_yshaarj( s->target ); } );
  }
};

// ==========================================================================
// Echoing Void
// TODO: move to sc_priest_shadow.cpp
// ==========================================================================
struct echoing_void_demise_t final : public priest_spell_t
{
  int stacks;

  echoing_void_demise_t( priest_t& p ) : priest_spell_t( "echoing_void_demise", p, p.find_spell( 373304 ) ), stacks( 1 )
  {
    background          = true;
    proc                = false;
    callbacks           = true;
    may_miss            = false;
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

    tl.erase( std::remove( tl.begin(), tl.end(), target ), tl.end() );

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
    : priest_spell_t( "echoing_void", p, p.find_spell( 373304 ) ), child_action_stats( nullptr )
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
// ==========================================================================
struct shadow_word_death_self_damage_t final : public priest_spell_t
{
  shadow_word_death_self_damage_t( priest_t& p )
    : priest_spell_t( "shadow_word_death_self_damage", p, p.specs.shadow_word_death_self_damage )
  {
    background = true;
    may_crit   = false;
    may_miss   = false;
    target     = player;
  }

  void trigger( double original_amount )
  {
    if ( priest().talents.tithe_evasion.enabled() )
    {
      original_amount /= ( 1.0 + priest().talents.tithe_evasion->effectN( 1 ).percent() );
    }
    base_td = original_amount;
    execute();
  }

  void init() override
  {
    base_t::init();

    // We don't want this counted towards our dps
    stats->type = stats_e::STATS_NEUTRAL;
  }

  proc_types proc_type() const override
  {
    return PROC1_ANY_DAMAGE_TAKEN;
  }
};

struct shadow_word_death_t final : public priest_spell_t
{
  double execute_percent;
  double execute_modifier;
  propagate_const<shadow_word_death_self_damage_t*> shadow_word_death_self_damage;
  propagate_const<expiation_t*> child_expiation;
  action_t* child_searing_light;

  shadow_word_death_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "shadow_word_death", p, p.talents.shadow_word_death ),
      execute_percent( data().effectN( 2 ).base_value() ),
      execute_modifier( data().effectN( 3 ).percent() + priest().specs.shadow_priest->effectN( 25 ).percent() ),
      shadow_word_death_self_damage( new shadow_word_death_self_damage_t( p ) ),
      child_expiation( nullptr ),
      child_searing_light( priest().background_actions.searing_light )
  {
    parse_options( options_str );

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
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = priest_spell_t::composite_target_da_multiplier( t );

    if ( t->health_percentage() < execute_percent || priest().buffs.deathspeaker->check() )
    {
      if ( sim->debug )
      {
        sim->print_debug( "{} below {}% HP. Increasing {} damage by {}%", t->name_str, execute_percent, *this,
                          execute_modifier * 100 );
      }
      tdm *= 1 + execute_modifier;
    }

    return tdm;
  }

  void execute() override
  {
    priest_spell_t::execute();

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

    priest_spell_t::reset();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().talents.shadow.inescapable_torment.enabled() )
    {
      priest().trigger_inescapable_torment( s->target );
    }

    if ( result_is_hit( s->result ) )
    {
      double save_health_percentage = s->target->health_percentage();

      // TODO: Add in a custom buff that checks after 1 second to see if the target SWD was cast on is now dead.
      if ( !( ( save_health_percentage > 0.0 ) && ( s->target->health_percentage() <= 0.0 ) ) )
      {
        // target is not killed
        shadow_word_death_self_damage->trigger( s->result_amount );
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

      if ( priest().talents.shadow.psychic_link.enabled() )
      {
        priest().trigger_psychic_link( s );
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
struct holy_nova_t final : public priest_spell_t
{
  action_t* child_light_eruption;
  holy_nova_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "holy_nova", p, p.talents.holy_nova ),
      child_light_eruption( priest().background_actions.light_eruption )
  {
    parse_options( options_str );
    aoe                 = -1;
    full_amount_targets = as<int>( priest().talents.holy_nova->effectN( 3 ).base_value() );
    reduced_aoe_targets = priest().talents.holy_nova->effectN( 3 ).base_value();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    if ( priest().talents.rhapsody && priest().buffs.rhapsody->check() )
    {
      m *= 1 + ( priest().talents.rhapsody_buff->effectN( 1 ).percent() * priest().buffs.rhapsody->current_stack );
    }

    return m;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.rhapsody )
    {
      priest().buffs.rhapsody_timer->trigger();
    }
    if ( child_light_eruption && priest().buffs.divine_image->check() )
    {
      for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
      {
        child_light_eruption->execute();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().talents.rhapsody && priest().buffs.rhapsody->check() )
    {
      priest().buffs.rhapsody->expire();
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
};

}  // namespace spells

namespace heals
{
// ==========================================================================
// Flash Heal
// ==========================================================================
struct flash_heal_t final : public priest_heal_t
{
  flash_heal_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "flash_heal", p, p.find_class_spell( "Flash Heal" ) )
  {
    parse_options( options_str );
    harmful = false;

    apply_affecting_aura( priest().talents.improved_flash_heal );
  }

  timespan_t execute_time() const override
  {
    timespan_t et = priest_heal_t::execute_time();

    if ( priest().talents.unwavering_will.enabled() &&
         priest().health_percentage() > priest().talents.unwavering_will->effectN( 2 ).base_value() )
    {
      et *= 1 + priest().talents.unwavering_will->effectN( 1 ).percent();
    }

    return et;
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

    priest().buffs.protective_light->trigger();
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    priest().buffs.from_darkness_comes_light->expire();
  }
};

// ==========================================================================
// Desperate Prayer
// TODO: add Light's Inspiration HoT
// ==========================================================================
struct desperate_prayer_t final : public priest_heal_t
{
  desperate_prayer_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "desperate_prayer", p, p.find_class_spell( "Desperate Prayer" ) )
  {
    parse_options( options_str );
    harmful  = false;
    may_crit = false;

    // does not seem to proc anything other than heal specific actions
    callbacks = false;

    // This is parsed as a HoT, disabling that manually
    base_td_multiplier = 0.0;
    dot_duration       = timespan_t::from_seconds( 0 );
  }

  void execute() override
  {
    priest().buffs.desperate_prayer->trigger();

    priest_heal_t::execute();
  }
};

// ==========================================================================
// Power Word: Shield
// TODO: add Rapture and Weal and Woe bonuses
// ==========================================================================
struct power_word_shield_t final : public priest_absorb_t
{
  double insanity;

  power_word_shield_t( priest_t& p, util::string_view options_str )
    : priest_absorb_t( "power_word_shield", p, p.find_class_spell( "Power Word: Shield" ) ),
      insanity( priest().specs.hallucinations->effectN( 1 ).resource() )
  {
    parse_options( options_str );
    spell_power_mod.direct = 2.8;  // hardcoded into tooltip, last checked 2022-09-04
    apply_affecting_aura( priest().talents.discipline.borrowed_time );
  }

  // Manually create the buff so we can reference it with Void Shield
  absorb_buff_t* create_buff( const action_state_t* ) override
  {
    return priest().buffs.power_word_shield;
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
  }

  void impact( action_state_t* s ) override
  {
    priest_absorb_t::impact( s );

    if ( priest().talents.body_and_soul->ok() && s->target->buffs.body_and_soul )
    {
      s->target->buffs.body_and_soul->trigger();
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
  double execute_modifier;

  power_word_life_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "power_word_life", p, p.talents.power_word_life ),
      execute_percent( data().effectN( 2 ).base_value() ),
      execute_modifier( data().effectN( 3 ).percent() )
  {
    parse_options( options_str );
    harmful = false;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_heal_t::composite_da_multiplier( s );

    if ( s->target->health_percentage() < execute_percent )
    {
      if ( sim->debug )
      {
        sim->print_debug( "{} below {}% HP. Increasing {} healing by {}", s->target->name_str, execute_percent, *this,
                          execute_modifier );
      }
      m *= 1 + execute_modifier;
    }

    return m;
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( target->health_percentage() <= execute_percent )
    {
      cooldown->adjust( timespan_t::from_seconds( data().effectN( 4 ).base_value() ) );
    }
  }
};

struct essence_devourer_t final : public priest_heal_t
{
  essence_devourer_t( priest_t& p )
    : priest_heal_t( "essence_devourer", p,
                     p.talents.shadow.mindbender.enabled() ? p.talents.essence_devourer_mindbender
                                                           : p.talents.essence_devourer_shadowfiend )
  {
    harmful = false;
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

    if ( priest().talents.lights_inspiration.enabled() )
    {
      health_change *= 1.0 + priest().talents.lights_inspiration->effectN( 1 ).percent();
    }
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    double old_health     = player->resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player->resources.max[ RESOURCE_HEALTH ];

    player->resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
    player->recalculate_resource_max( RESOURCE_HEALTH );
    player->resources.current[ RESOURCE_HEALTH ] *=
        1.0 + health_change;  // Update health after the maximum is increased

    sim->print_debug( "{} gains desperate_prayer: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                      player->name(), health_change * 100.0, old_health, player->resources.current[ RESOURCE_HEALTH ],
                      old_max_health, player->resources.max[ RESOURCE_HEALTH ] );
  }

  void expire_override( int, timespan_t ) override
  {
    double old_health     = player->resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player->resources.max[ RESOURCE_HEALTH ];

    player->resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player->resources.current[ RESOURCE_HEALTH ] /= 1.0 + health_change;  // Update health before the maximum is reduced
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

  buffs.schism                   = make_buff( *this, "schism", p.talents.discipline.schism );
  buffs.death_and_madness_debuff = make_buff<buffs::death_and_madness_debuff_t>( *this );
  buffs.echoing_void             = make_buff( *this, "echoing_void", p.find_spell( 373281 ) );

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
  cooldowns.mindgames                     = get_cooldown( "mindgames" );
  cooldowns.mindbender                    = get_cooldown( "mindbender" );
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
  gains.power_word_solace                = get_gain( "Mana Gained from Power Word: Solace" );
  gains.throes_of_pain                   = get_gain( "Throes of Pain" );
  gains.insanity_idol_of_cthun_mind_flay = get_gain( "Insanity Gained from Idol of C'thun Mind Flay's" );
  gains.insanity_idol_of_cthun_mind_sear = get_gain( "Insanity Gained from Idol of C'thun Mind Sear's" );
  gains.hallucinations_power_word_shield = get_gain( "Insanity Gained from Power Word: Shield with Hallucinations" );
  gains.insanity_maddening_touch         = get_gain( "Maddening Touch" );
  gains.insanity_t30_2pc                 = get_gain( "Insanity Gained from T30 2PC" );
}

/** Construct priest procs */
void priest_t::create_procs()
{
  // Discipline
  procs.power_of_the_dark_side          = get_proc( "Power of the Dark Side Penance damage buffed" );
  procs.power_of_the_dark_side_overflow = get_proc( "Power of the Dark Side lost to overflow" );
  procs.expiation_lost_no_dot           = get_proc( "Missed chance for expiation to consume a DoT" );
  // Shadow - Talents
  procs.shadowy_apparition_vb          = get_proc( "Shadowy Apparition from Void Bolt" );
  procs.shadowy_apparition_swp         = get_proc( "Shadowy Apparition from Shadow Word: Pain" );
  procs.shadowy_apparition_dp          = get_proc( "Shadowy Apparition from Devouring Plague" );
  procs.shadowy_apparition_mb          = get_proc( "Shadowy Apparition from Mind Blast" );
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

  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( auto pet_expr = create_pet_expression( expression_str, splits ) )
  {
    return pet_expr;
  }

  if ( splits.size() >= 2 )
  {
    if ( util::str_compare_ci( splits[ 0 ], "priest" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "self_power_infusion" ) )
      {
        return expr_t::create_constant( "self_power_infusion", options.self_power_infusion );
      }

      if ( util::str_compare_ci( splits[ 1 ], "cthun_last_trigger_attempt" ) )
      {
        if ( talents.shadow.idol_of_cthun.ok() )
          // std::min( sim->current_time() - last_trigger, max_interval() ).total_seconds();
          return make_fn_expr( "cthun_last_trigger_attempt", [ this ] {
            return std::min( sim->current_time() - rppm.idol_of_cthun->get_last_trigger_attempt(), 3.5_s )
                .total_seconds();
          } );
        else
          return expr_t::create_constant( "cthun_last_trigger_attempt", -1 );
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

  return h;
}

double priest_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

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
  m *= ( 1.0 + specs.shadow_priest->effectN( 3 ).percent() );

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

  if ( buffs.twist_of_fate->check() )
  {
    m *= 1.0 + buffs.twist_of_fate->current_value;
  }

  if ( mastery_spells.grace->ok() )
  {
    m *= 1 + cache.mastery_value();
  }

  return m;
}

double priest_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_absorb_multiplier( s );

  if ( mastery_spells.grace->ok() )
  {
    m *= 1 + cache.mastery_value();
  }

  return m;
}

double priest_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

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
  if ( ( name == "shadowfiend" ) || ( name == "mindbender" ) || ( name == "fiend" ) )
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
    auto divine_star_insanity  = talents.divine_star->effectN( 3 ).resource( RESOURCE_INSANITY );
    auto halo_insanity         = talents.halo->effectN( 2 ).resource( RESOURCE_INSANITY );
    auto shadow_crash_insanity = talents.shadow.shadow_crash->effectN( 2 ).resource( RESOURCE_INSANITY );

    if ( talents.shadow.shadow_crash.enabled() )
    {
      // Two Shadow Crash + Two Divine Star == 24 Insanity
      if ( talents.divine_star.enabled() )
        resources.initial_opt[ RESOURCE_INSANITY ] = ( shadow_crash_insanity * 2 ) + ( divine_star_insanity * 2 );
      // Two Shadow Crash + One Halo == 22 Insanity
      else if ( talents.halo.enabled() )
        resources.initial_opt[ RESOURCE_INSANITY ] = ( shadow_crash_insanity * 2 ) + halo_insanity;
      else
        // Two Shadow Crash == 12 Insanity
        resources.initial_opt[ RESOURCE_INSANITY ] = ( shadow_crash_insanity * 2 );
    }
    else
    {
      // Three Divine Stars == 18 Insanity
      if ( talents.divine_star.enabled() )
        resources.initial_opt[ RESOURCE_INSANITY ] = ( divine_star_insanity * 3 );
      // One Halo == 10 Insanity
      if ( talents.halo.enabled() )
        resources.initial_opt[ RESOURCE_INSANITY ] = halo_insanity;
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
}

void priest_t::init_spells()
{
  base_t::init_spells();

  auto CT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };

  init_spells_shadow();
  init_spells_discipline();
  init_spells_holy();

  // Generic Spells
  specs.mind_blast                    = find_class_spell( "Mind Blast" );
  specs.shadow_word_death_self_damage = find_spell( 32409 );
  specs.psychic_scream                = find_class_spell( "Psychic Scream" );
  specs.fade                          = find_class_spell( "Fade" );

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
  talents.focused_mending            = CT( "Focused Mending" );  // NYI
  talents.holy_nova                  = CT( "Holy Nova" );
  talents.protective_light           = CT( "Protective Light" );
  talents.protective_light_buff      = find_spell( 193065 );
  talents.from_darkness_comes_light  = CT( "From Darkness Comes Light" );
  talents.angelic_feather            = CT( "Angelic Feather" );
  talents.phantasm                   = CT( "Phantasm" );  // NYI
  talents.death_and_madness          = CT( "Death and Madness" );
  talents.death_and_madness_insanity = find_spell( 321973 );
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
  talents.inspiration                = CT( "Inspiration" );           // NYI
  talents.improved_mass_dispel       = CT( "Improved Mass Dispel" );  // NYI
  talents.body_and_soul              = CT( "Body and Soul" );
  talents.twins_of_the_sun_priestess = CT( "Twins of the Sun Priestess" );
  talents.void_shield                = CT( "Void Shield" );
  talents.sanlayn                    = CT( "San'layn" );
  talents.apathy                     = CT( "Apathy" );
  // Row 7
  talents.unwavering_will = CT( "Unwavering Will" );
  talents.twist_of_fate   = CT( "Twist of Fate" );
  talents.throes_of_pain  = CT( "Throes of Pain" );
  // Row 8
  talents.angels_mercy               = CT( "Angel's Mercy" );  // NYI
  talents.binding_heals              = CT( "Binding Heals" );  // NYI
  talents.halo                       = CT( "Halo" );
  talents.halo_heal_holy             = find_spell( 120692 );
  talents.halo_dmg_holy              = find_spell( 120696 );
  talents.halo_heal_shadow           = find_spell( 390971 );
  talents.halo_dmg_shadow            = find_spell( 390964 );
  talents.divine_star                = CT( "Divine Star" );
  talents.divine_star_heal_holy      = find_spell( 110745 );
  talents.divine_star_dmg_holy       = find_spell( 122128 );
  talents.divine_star_heal_shadow    = find_spell( 390981 );
  talents.divine_star_dmg_shadow     = find_spell( 390845 );
  talents.translucent_image          = CT( "Translucent Image" );
  talents.mindgames                  = CT( "Mindgames" );
  talents.mindgames_healing_reversal = find_spell( 323707 );  // TODO: Swap to new DF spells
  talents.mindgames_damage_reversal  = find_spell( 323706 );  // TODO: Swap to new DF spells 375902 + 375904
  // Row 9
  talents.surge_of_light         = CT( "Surge of Light" );  // TODO: find out buff spell ID
  talents.lights_inspiration     = CT( "Light's Inspiration" );
  talents.crystalline_reflection = CT( "Crystalline Reflection" );  // NYI
  talents.improved_fade          = CT( "Improved Fade" );
  talents.manipulation = CT( "Manipulation" );  // Spell data is not great here, actual/tooltip value is cut in half
  // Row 10
  talents.benevolence      = CT( "Benevolence" );
  talents.power_word_life  = CT( "Power Word: Life" );
  talents.angelic_bulwark  = CT( "Angelic Bulwark" );  // NYI
  talents.essence_devourer = CT( "Essence Devourer" );
  if ( is_ptr() )
  {
    talents.essence_devourer_shadowfiend = find_spell( 415673 );  // actual healing spell for Shadowfiend
    talents.essence_devourer_mindbender  = find_spell( 415676 );  // actual healing spell for Mindbender
  }
  talents.void_shift            = CT( "Void Shift" );  // NYI
  talents.shattered_perceptions = CT( "Shattered Perceptions" );
}

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Generic buffs
  buffs.desperate_prayer  = make_buff<buffs::desperate_prayer_t>( *this );
  buffs.power_word_shield = new buffs::power_word_shield_buff_t( this );
  buffs.fade              = make_buff( this, "fade", find_class_spell( "Fade" ) )->set_default_value_from_effect( 1 );

  // Shared talent buffs
  // Does not show damage value on the buff spelldata, that is only found on the talent
  buffs.twist_of_fate = make_buff( this, "twist_of_fate", find_spell( 390978 ) )
                            ->set_trigger_spell( talents.twist_of_fate )
                            ->set_default_value_from_effect( 1 )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                            ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  buffs.rhapsody =
      make_buff( this, "rhapsody", talents.rhapsody_buff )->set_stack_change_callback( ( [ this ]( buff_t*, int, int ) {
        buffs.rhapsody_timer->trigger();
      } ) );
  buffs.rhapsody_timer = make_buff( this, "rhapsody_timer", talents.rhapsody )
                             ->set_quiet( true )
                             ->set_duration( timespan_t::from_seconds( 5 ) )
                             ->set_max_stack( 1 )
                             ->set_stack_change_callback( ( [ this ]( buff_t*, int, int new_ ) {
                               if ( new_ == 0 )
                               {
                                 buffs.rhapsody->trigger();
                               }
                             } ) );
  // Tracking buff to see if the free reset is available for SW:D with DaM talented.
  buffs.death_and_madness_reset = make_buff( this, "death_and_madness_reset", find_spell( 390628 ) )
                                      ->set_trigger_spell( talents.death_and_madness );
  buffs.protective_light =
      make_buff( this, "protective_light", talents.protective_light_buff )->set_default_value_from_effect( 1 );
  buffs.from_darkness_comes_light =
      make_buff( this, "from_darkness_comes_light", talents.from_darkness_comes_light->effectN( 1 ).trigger() )
          ->set_default_value_from_effect( 1 );
  buffs.words_of_the_pious = make_buff( this, "words_of_the_pious", talents.words_of_the_pious->effectN( 1 ).trigger() )
                                 ->set_default_value_from_effect( 1 );

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
  background_actions.echoing_void_demise = new actions::spells::echoing_void_demise_t( *this );
  background_actions.essence_devourer    = new actions::heals::essence_devourer_t( *this );
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

  action.apply_affecting_aura( specs.shadow_priest );
  action.apply_affecting_aura( specs.holy_priest );
  action.apply_affecting_aura( specs.discipline_priest );

  // Class Talents
  action.apply_affecting_aura( talents.benevolence );

  // Shadow Talents
  action.apply_affecting_aura( talents.shadow.malediction );
  action.apply_affecting_aura( talents.shadow.mastermind );
  action.apply_affecting_aura( talents.shadow.mental_decay );

  // Discipline Talents
  action.apply_affecting_aura( talents.discipline.dark_indulgence );
  action.apply_affecting_aura( talents.discipline.expiation );

  // Holy Talents
  action.apply_affecting_aura( talents.holy.miracle_worker );
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
      priest_apl::shadow( this );
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

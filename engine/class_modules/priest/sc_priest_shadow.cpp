// ==========================================================================
// Shadow Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "action/action_state.hpp"
#include "sc_enums.hpp"
#include "sc_priest.hpp"
#include "util/generic.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions
{
namespace spells
{
// ==========================================================================
// Mind Sear
// ==========================================================================
struct mind_sear_tick_t final : public priest_spell_t
{
  double coalescing_shadows_chance = 0.0;

  mind_sear_tick_t( priest_t& p, const spell_data_t* s ) : priest_spell_t( "mind_sear_tick", p, s )
  {
    affected_by_shadow_weaving = true;
    background                 = true;
    dual                       = true;
    aoe                        = -1;
    callbacks                  = false;
    direct_tick                = false;
    use_off_gcd                = true;
    dynamic_tick_action        = true;
    radius                     = data().effectN( 2 ).radius();  // base radius is 100yd, actual is stored in effect 2
    reduced_aoe_targets        = 5;

    if ( priest().talents.shadow.coalescing_shadows.enabled() )
    {
      coalescing_shadows_chance = priest().talents.shadow.coalescing_shadows->effectN( 1 ).percent() +
                                  priest().talents.shadow.harnessed_shadows->effectN( 2 ).percent();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // Benefit Tracking
    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T29, B2 ) )
    {
      priest().buffs.gathering_shadows->up();
    }

    priest().trigger_idol_of_cthun( s );

    if ( priest().talents.shadow.dark_evangelism.enabled() )
    {
      priest().buffs.dark_evangelism->trigger();
    }

    if ( priest().talents.shadow.mental_decay.enabled() )
    {
      timespan_t dot_extension =
          timespan_t::from_seconds( priest().talents.shadow.mental_decay->effectN( 1 ).base_value() );
      priest_td_t& td = get_td( s->target );

      td.dots.shadow_word_pain->adjust_duration( dot_extension, true );
      td.dots.vampiric_touch->adjust_duration( dot_extension, true );
    }

    if ( priest().talents.shadow.coalescing_shadows.enabled() && rng().roll( coalescing_shadows_chance ) )
    {
      priest().buffs.coalescing_shadows->trigger();
      priest().procs.coalescing_shadows_mind_sear->occur();
    }
  }
};

struct mind_sear_t final : public priest_spell_t
{
  mind_sear_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mind_sear", p, p.talents.shadow.mind_sear )
  {
    parse_options( options_str );
    channeled           = true;
    dynamic_tick_action = true;
    radius = data().effectN( 1 ).trigger()->effectN( 2 ).radius();  // need to set radius in here so that the APL
                                                                    // functions correctly
    if ( priest().specialization() == PRIEST_SHADOW )
    {
      base_costs_per_tick[ RESOURCE_MANA ] = 0.0;

      // TODO: spelldata insanity cost per tick is wrong, manually overriding from tooltip data
      base_costs_per_tick[ RESOURCE_INSANITY ] = data().effectN( 3 ).base_value();
    }

    tick_action = new mind_sear_tick_t( p, data().effectN( 1 ).trigger() );
  }

  bool ready() override
  {
    if ( priest().buffs.mind_devourer->may_react() && priest().talents.shadow.mind_devourer.enabled() )
    {
      return true;
    }
    // You cannot start a cast if you have less than 2 ticks worth
    // TODO replace with cost_per_tick( RESOURCE_INSANITY ) when spelldata is fixed
    if ( priest().resources.current[ RESOURCE_INSANITY ] < data().effectN( 3 ).base_value() * 2 )
    {
      return false;
    }

    return priest_spell_t::ready();
  }

  // TODO: parse_buff_effects doesn't support tick based resource cost modifications
  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    extend_vf( cost_per_tick( RESOURCE_INSANITY ) );

    if ( priest().buffs.mind_devourer_ms_active->check() )
    {
      player->sim->print_debug( "{} {} consumes ticking cost 0 insanity (current={}) from mind_devourer.", priest(),
                                *this, player->resources.current[ RESOURCE_INSANITY ] );
      return true;
    }

    return priest_spell_t::consume_cost_per_tick( dot );
  }

  void execute() override
  {
    // Consume Mind Devourer right away and use a tracking buff to handle the free ticks
    if ( priest().talents.shadow.mind_devourer.enabled() )
    {
      // If somehow you get to this execute block again with this active get rid of the buff
      if ( priest().buffs.mind_devourer_ms_active->check() )
      {
        priest().buffs.mind_devourer_ms_active->expire();
      }

      if ( priest().buffs.mind_devourer->check() )
      {
        priest().buffs.mind_devourer->expire();
        priest().buffs.mind_devourer_ms_active->trigger();
      }
    }

    priest_spell_t::execute();
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );

    priest().buffs.mind_devourer_ms_active->expire();

    priest().buffs.gathering_shadows->expire();
  }

  void tick( dot_t* d ) override
  {
    // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/966
    if ( priest().bugs && priest().options.gathering_shadows_bug )
    {
      double insanity_before_tick = player->resources.current[ RESOURCE_INSANITY ] - cost_per_tick( RESOURCE_INSANITY );

      // Determine if this is the last tick
      if ( insanity_before_tick < ( cost_per_tick( RESOURCE_INSANITY ) ) )
      {
        // This is the last tick, cancel the buff before the damage goes out
        priest().buffs.gathering_shadows->expire();
      }
    }

    priest_spell_t::tick( d );

    if ( priest().talents.shadow.shadowy_apparitions.enabled() )
    {
      if ( rng().roll( priest().talents.shadow.shadowy_apparitions->effectN( 3 ).percent() ) )
      {
        priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_ms, false );
      }
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T29, B4 ) )
    {
      priest().buffs.dark_reveries->trigger();
    }

    double insanity_after_tick = player->resources.current[ RESOURCE_INSANITY ] - cost_per_tick( RESOURCE_INSANITY );

    // Mind Sear will only ever consume 25 Insanity, no partial amounts
    // Cancel the channel after the ticks happen
    if ( insanity_after_tick < cost_per_tick( RESOURCE_INSANITY ) )
    {
      player->sim->print_debug( "{} {} will be cancelled. Ran out of Insanity for next tick, insanity_after_tick={}.",
                                priest(), *this, insanity_after_tick );
      make_event( *sim, 10_ms, [ this ] { this->cancel(); } );
    }
  }
};

// ==========================================================================
// Mind Flay
// ==========================================================================
struct mind_flay_base_t final : public priest_spell_t
{
  double coalescing_shadows_chance = 0.0;

  mind_flay_base_t( util::string_view n, priest_t& p, const spell_data_t* s ) : priest_spell_t( n, p, s )
  {
    affected_by_shadow_weaving = true;
    may_crit                   = false;
    channeled                  = true;
    use_off_gcd                = true;

    if ( priest().talents.shadow.coalescing_shadows.enabled() )
    {
      coalescing_shadows_chance = priest().talents.shadow.coalescing_shadows->effectN( 2 ).percent() +
                                  priest().talents.shadow.harnessed_shadows->effectN( 3 ).percent();
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    priest().trigger_idol_of_cthun( d->state );

    if ( priest().talents.shadow.dark_evangelism.enabled() )
    {
      priest().buffs.dark_evangelism->trigger();
    }

    if ( priest().talents.shadow.mental_decay.enabled() )
    {
      timespan_t dot_extension =
          timespan_t::from_seconds( priest().talents.shadow.mental_decay->effectN( 1 ).base_value() );
      priest_td_t& td = get_td( d->state->target );

      td.dots.shadow_word_pain->adjust_duration( dot_extension, true );
      td.dots.vampiric_touch->adjust_duration( dot_extension, true );
    }

    if ( priest().talents.shadow.coalescing_shadows.enabled() && rng().roll( coalescing_shadows_chance ) )
    {
      priest().buffs.coalescing_shadows->trigger();
      priest().procs.coalescing_shadows_mind_flay->occur();
    }

    if ( priest().talents.shadow.psychic_link.enabled() )
    {
      priest().trigger_psychic_link( d->state );
    }
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );

    // Track when the APL/sim cancels MF:I before you get all ticks off
    if ( this->id == 391403 && d->current_tick < d->num_ticks() )
    {
      player->sim->print_debug( "{} ended {} at {} tick. total ticks={}", priest(), d->name_str, d->current_tick,
                                d->num_ticks() );
      priest().procs.mind_flay_insanity_wasted->occur();
    }
  }
};

struct mind_flay_t final : public priest_spell_t
{
  timespan_t manipulation_cdr;

  mind_flay_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mind_flay", p, p.specs.mind_flay ),
      _base_spell( new mind_flay_base_t( "mind_flay", p, p.specs.mind_flay ) ),
      _insanity_spell( new mind_flay_base_t( "mind_flay_insanity", p, p.talents.shadow.mind_flay_insanity_spell ) ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) )
  {
    parse_options( options_str );

    add_child( _base_spell );
  }

  void execute() override
  {
    if ( priest().talents.manipulation.enabled() && priest().is_ptr() )
    {
      priest().cooldowns.mindgames->adjust( -manipulation_cdr );
    }

    if ( priest().buffs.mind_flay_insanity->check() )
    {
      _insanity_spell->execute();
      priest().buffs.mind_flay_insanity->expire();
    }
    else
    {
      _base_spell->execute();
    }
  }

private:
  propagate_const<action_t*> _base_spell;
  propagate_const<action_t*> _insanity_spell;
};

// ==========================================================================
// Dispersion
// ==========================================================================
struct dispersion_t final : public priest_spell_t
{
  dispersion_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "dispersion", p, p.talents.shadow.dispersion )
  {
    parse_options( options_str );

    ignore_false_positive = true;
    channeled             = true;
    harmful               = false;
    tick_may_crit         = false;
    hasted_ticks          = false;
    may_miss              = false;

    if ( priest().talents.shadow.intangibility.enabled() )
    {
      cooldown->duration += priest().talents.shadow.intangibility->effectN( 1 ).time_value();
    }
  }

  void execute() override
  {
    priest().buffs.dispersion->trigger();

    priest_spell_t::execute();
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    // Unhasted, even though it is a channeled spell.
    return base_tick_time;
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );

    // reset() instead of expire() because it was not properly creating the buff every 2nd time
    priest().buffs.dispersion->reset();
  }
};

// ==========================================================================
// Shadowform
// ==========================================================================
struct shadowform_t final : public priest_spell_t
{
  shadowform_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "shadowform", p, p.specs.shadowform )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest().buffs.shadowform_state->trigger();
    priest().buffs.shadowform->trigger();
  }
};

// ==========================================================================
// Silence
// ==========================================================================
struct silence_t final : public priest_spell_t
{
  silence_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "silence", p, p.talents.shadow.silence )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = is_interrupt = true;

    auto rank2 = p.find_rank_spell( "Silence", "Rank 2" );
    if ( rank2->ok() )
    {
      range += rank2->effectN( 1 ).base_value();
    }

    // CD Reduction
    apply_affecting_aura( priest().talents.shadow.last_word );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !priest_spell_t::target_ready( candidate_target ) )
      return false;

    if ( candidate_target->debuffs.casting && candidate_target->debuffs.casting->check() )
      return true;

    if ( target->type == ENEMY_ADD || target->level() < sim->max_player_level + 3 )
      return true;

    // Check if the target can get blank silenced
    if ( candidate_target->type != ENEMY_ADD && ( candidate_target->level() < sim->max_player_level + 3 ) )
      return true;

    return false;
  }
};

// ==========================================================================
// Vampiric Embrace
// ==========================================================================
struct vampiric_embrace_t final : public priest_spell_t
{
  double insanity;

  vampiric_embrace_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "vampiric_embrace", p, p.talents.vampiric_embrace ),
      insanity( priest().specs.hallucinations->effectN( 1 ).base_value() )
  {
    parse_options( options_str );

    harmful = false;

    // Cooldown reduction
    apply_affecting_aura( priest().talents.sanlayn );
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest().buffs.vampiric_embrace->trigger();

    if ( priest().specs.hallucinations->ok() )
    {
      priest().generate_insanity( insanity, priest().gains.hallucinations_vampiric_embrace, nullptr );
    }
  }

  bool ready() override
  {
    if ( priest().buffs.vampiric_embrace->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

// ==========================================================================
// Shadowy Apparition
// ==========================================================================
struct shadowy_apparition_damage_t final : public priest_spell_t
{
  double insanity_gain;
  double coalescing_shadows_chance = 0.0;

  shadowy_apparition_damage_t( priest_t& p )
    : priest_spell_t( "shadowy_apparition", p, p.talents.shadow.shadowy_apparition ),
      insanity_gain( priest().talents.shadow.auspicious_spirits->effectN( 2 ).percent() )
  {
    affected_by_shadow_weaving = true;
    background                 = true;
    proc                       = false;
    callbacks                  = true;
    may_miss                   = false;
    may_crit                   = false;

    base_dd_multiplier *= 1 + priest().talents.shadow.auspicious_spirits->effectN( 1 ).percent();

    if ( priest().talents.shadow.puppet_master.enabled() )
    {
      coalescing_shadows_chance = priest().talents.shadow.puppet_master->proc_chance();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().talents.shadow.auspicious_spirits.enabled() &&
         ( !priest().bugs || !priest().options.as_insanity_bug ) )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_auspicious_spirits, s->action );
    }

    if ( priest().talents.shadow.puppet_master.enabled() && rng().roll( coalescing_shadows_chance ) )
    {
      priest().buffs.coalescing_shadows->trigger();
      priest().procs.coalescing_shadows_shadowy_apparitions->occur();
    }
  }
};

struct shadowy_apparition_spell_t final : public priest_spell_t
{
  bool gets_crit_mod = false;
  double insanity_gain;

  shadowy_apparition_spell_t( priest_t& p )
    : priest_spell_t( "shadowy_apparitions", p, p.talents.shadow.shadowy_apparitions ),
      insanity_gain( priest().talents.shadow.auspicious_spirits->effectN( 2 ).percent() )
  {
    background   = true;
    proc         = false;
    may_miss     = false;
    may_crit     = false;
    trigger_gcd  = timespan_t::zero();
    travel_speed = priest().talents.shadow.shadowy_apparition->missile_speed();

    impact_action = new shadowy_apparition_damage_t( p );

    add_child( impact_action );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    if ( gets_crit_mod )
    {
      m *= 1 + priest().talents.shadow.shadowy_apparitions->effectN( 2 ).percent();
    }

    return m;
  }

  /** Trigger a shadowy apparition */
  void trigger( player_t* target, proc_t* proc, bool _gets_crit_mod )
  {
    gets_crit_mod = _gets_crit_mod;
    player->sim->print_debug( "{} triggered shadowy apparition on target {} from {}. crit_mod={}", priest(), *target,
                              proc->name(), gets_crit_mod );

    proc->occur();
    set_target( target );
    execute();

    // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/1081
    if ( priest().talents.shadow.auspicious_spirits.enabled() && priest().bugs && priest().options.as_insanity_bug )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_auspicious_spirits, execute_state->action );
    }
  }
};

// ==========================================================================
// Shadow Word: Pain
// ==========================================================================
struct shadow_word_pain_t final : public priest_spell_t
{
  bool casted;

  double coalescing_shadows_chance = 0.0;

  shadow_word_pain_t( priest_t& p, bool _casted = false )
    : priest_spell_t( "shadow_word_pain", p, p.dot_spells.shadow_word_pain )
  {
    affected_by_shadow_weaving = true;
    casted                     = _casted;
    may_crit                   = true;
    tick_zero                  = false;
    if ( !casted )
    {
      base_dd_max            = 0.0;
      base_dd_min            = 0.0;
      energize_type          = action_energize::NONE;  // no insanity gain
      spell_power_mod.direct = 0;
    }

    if ( priest().talents.shadow.misery.enabled() )
    {
      dot_duration += priest().talents.shadow.misery->effectN( 2 ).time_value();
    }

    // Spell Direct and Periodic 3%/5% gain
    apply_affecting_aura( priest().talents.throes_of_pain );

    if ( priest().talents.shadow.coalescing_shadows.enabled() )
    {
      coalescing_shadows_chance = priest().talents.shadow.coalescing_shadows->effectN( 1 ).percent() +
                                  priest().talents.shadow.harnessed_shadows->effectN( 2 ).percent();
    }
  }

  shadow_word_pain_t( priest_t& p, util::string_view options_str ) : shadow_word_pain_t( p, true )
  {
    parse_options( options_str );
  }

  void trigger( player_t* target )
  {
    background = true;
    player->sim->print_debug( "{} triggered shadow_word_pain on target {}.", priest(), *target );

    set_target( target );
    execute();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( priest().talents.shadow.deathspeaker.enabled() && priest().rppm.deathspeaker->trigger() )
      {
        priest().buffs.deathspeaker->trigger();
        priest().procs.deathspeaker->occur();
      }

      priest().refresh_insidious_ire_buff( s );
    }
  }

  // TODO: Check if this is still bugged, as of 23/12/2022 screams is confirmed broken again with mental decay.
  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t t = priest_spell_t::tick_time( state );

    if ( priest().is_screams_of_the_void_up( state->target, id ) )
    {
      t /= ( 1 + priest().talents.shadow.screams_of_the_void->effectN( 1 ).percent() );
    }

    return t;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      trigger_power_of_the_dark_side();

      priest().trigger_idol_of_nzoth( d->state->target, priest().procs.idol_of_nzoth_swp );

      int stack = priest().buffs.shadowy_insight->check();
      if ( priest().buffs.shadowy_insight->trigger() )
      {
        if ( priest().buffs.shadowy_insight->check() == stack )
        {
          priest().procs.shadowy_insight_overflow->occur();
        }
        else
        {
          priest().procs.shadowy_insight->occur();
        }
      }

      if ( priest().talents.shadow.tormented_spirits.enabled() &&
           rng().roll( priest()
                           .talents.shadow.tormented_spirits->effectN( ( d->state->result == RESULT_CRIT ) ? 2 : 1 )
                           .percent() ) )
      {
        priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_swp, false );
      }

      if ( priest().talents.shadow.coalescing_shadows.enabled() && rng().roll( coalescing_shadows_chance ) )
      {
        priest().buffs.coalescing_shadows->trigger();
        priest().procs.coalescing_shadows_shadow_word_pain->occur();
      }

      if ( priest().talents.shadow.deathspeaker.enabled() && priest().rppm.deathspeaker->trigger() )
      {
        priest().buffs.deathspeaker->trigger();
        priest().procs.deathspeaker->occur();
      }
    }
  }
};

// ==========================================================================
// Unfurling Darkness
// ==========================================================================
struct unfurling_darkness_t final : public priest_spell_t
{
  unfurling_darkness_t( priest_t& p )
    : priest_spell_t( "unfurling_darkness", p,
                      p.dot_spells.vampiric_touch )  // Damage value is stored in Vampiric Touch
  {
    background                 = true;
    affected_by_shadow_weaving = true;
    energize_type              = action_energize::NONE;  // no insanity gain
    energize_amount            = 0;
    energize_resource          = RESOURCE_NONE;
    ignores_automatic_mastery  = true;

    // Since we are re-using the Vampiric Touch spell disable the DoT
    dot_duration       = timespan_t::from_seconds( 0 );
    base_td_multiplier = spell_power_mod.tick = 0;
  }
};

// ==========================================================================
// Mental Fortitude
// ==========================================================================
struct mental_fortitude_t : public priest_absorb_t
{
  mental_fortitude_t( priest_t& p ) : priest_absorb_t( "mental_fortitude", p, p.talents.shadow.mental_fortitude )
  {
    may_miss = may_crit = callbacks = false;
    background = proc = true;
  }

  // Self only so we can do this in a simple way
  absorb_buff_t* create_buff( const action_state_t* ) override
  {
    return debug_cast<priest_t*>( player )->buffs.mental_fortitude;
  }

  void init() override
  {
    absorb_t::init();

    snapshot_flags = update_flags = 0;
  }
};

// ==========================================================================
// Vampiric Touch
// ==========================================================================
struct vampiric_touch_t final : public priest_spell_t
{
  struct vampiric_touch_heal_t final : public priest_heal_t
  {
    mental_fortitude_t* mental_fortitude;
    double mental_fortitude_percentage;

    vampiric_touch_heal_t( priest_t& p ) : priest_heal_t( "vampiric_touch_heal", p, p.dot_spells.vampiric_touch )
    {
      background         = true;
      may_crit           = false;
      may_miss           = false;
      base_dd_multiplier = 1.0;

      // Turn off Insanity gen from hit action
      energize_type     = action_energize::NONE;  // no insanity gain
      energize_amount   = 0;
      energize_resource = RESOURCE_NONE;

      // Turn off all damage parts of the spell
      spell_power_mod.direct = spell_power_mod.tick = base_td_multiplier = 0;
      dot_duration                                                       = timespan_t::from_seconds( 0 );

      mental_fortitude            = p.background_actions.mental_fortitude;
      mental_fortitude_percentage = priest().talents.shadow.mental_fortitude->effectN( 1 ).percent();
    }

    void trigger( double original_amount )
    {
      base_dd_min = base_dd_max = original_amount * data().effectN( 2 ).m_value();
      execute();
    }

    void impact( action_state_t* state ) override
    {
      priest_heal_t::impact( state );

      if ( priest().talents.shadow.mental_fortitude.enabled() &&
           state->target->current_health() == state->target->max_health() )
      {
        trigger_mental_fortitude( state );
      }
    }

    void trigger_mental_fortitude( action_state_t* state )
    {
      double current_value = 0;
      if ( mental_fortitude->target_specific[ state->target ] )
        current_value = mental_fortitude->target_specific[ state->target ]->current_value;

      double amount = current_value;
      amount += state->result_total;

      sim->print_debug( "mental_fortitude_percentage: {}", mental_fortitude_percentage );

      amount = std::min( amount, state->target->max_health() * mental_fortitude_percentage );

      mental_fortitude->base_dd_min = mental_fortitude->base_dd_max = amount;

      mental_fortitude->execute();
    }
  };

  propagate_const<vampiric_touch_heal_t*> vampiric_touch_heal;
  propagate_const<shadow_word_pain_t*> child_swp;
  propagate_const<unfurling_darkness_t*> child_ud;
  bool casted;
  double surge_of_darkness_proc_rate;
  bool insanity;
  bool ud_proc;
  bool ud_execute;

  vampiric_touch_t( priest_t& p, bool _casted = false, bool _insanity = true, bool _ud_proc = true,
                    bool _ud_execute = true )
    : priest_spell_t( "vampiric_touch", p, p.dot_spells.vampiric_touch ),
      vampiric_touch_heal( new vampiric_touch_heal_t( p ) ),
      child_swp( nullptr ),
      child_ud( nullptr ),
      surge_of_darkness_proc_rate( priest().talents.shadow.surge_of_darkness->proc_chance() )
  {
    casted                     = _casted;
    insanity                   = _insanity;
    ud_proc                    = _ud_proc;
    ud_execute                 = _ud_execute;
    may_crit                   = false;
    affected_by_shadow_weaving = true;

    // Disable initial hit damage, only Unfurling Darkness uses it
    base_dd_min = base_dd_max = spell_power_mod.direct = 0;

    if ( !insanity )
    {
      energize_type          = action_energize::NONE;  // no insanity gain
      spell_power_mod.direct = 0;
    }

    if ( priest().talents.shadow.misery.enabled() && casted )
    {
      child_swp             = new shadow_word_pain_t( priest(), false );
      child_swp->background = true;
    }

    if ( priest().talents.shadow.unfurling_darkness.enabled() && ud_execute )
    {
      child_ud = new unfurling_darkness_t( priest() );
      add_child( child_ud );
    }
  }

  vampiric_touch_t( priest_t& p, util::string_view options_str ) : vampiric_touch_t( p, true )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    if ( priest().buffs.unfurling_darkness->check() && ud_execute )
    {
      child_ud->target = s->target;
      child_ud->execute();
      priest().buffs.unfurling_darkness->expire();
    }
    else
    {
      if ( priest().talents.shadow.unfurling_darkness.enabled() && ud_proc &&
           !priest().buffs.unfurling_darkness_cd->check() )
      {
        priest().buffs.unfurling_darkness->trigger();
        // The CD Starts as soon as the buff is applied
        priest().buffs.unfurling_darkness_cd->trigger();
      }
    }

    // Trigger SW:P after UD since it does not benefit from the automatic Mastery benefit
    if ( child_swp )
    {
      child_swp->target = s->target;
      child_swp->execute();
    }

    priest_spell_t::impact( s );

    priest().refresh_insidious_ire_buff( s );
  }

  timespan_t execute_time() const override
  {
    if ( priest().buffs.unfurling_darkness->check() )
    {
      return 0_ms;
    }

    return priest_spell_t::execute_time();
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t t = priest_spell_t::tick_time( state );

    if ( priest().is_screams_of_the_void_up( state->target, id ) )
    {
      t /= ( 1 + priest().talents.shadow.screams_of_the_void->effectN( 1 ).percent() );
    }

    return t;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      if ( priest().talents.shadow.maddening_touch.enabled() &&
           rng().roll( priest().talents.shadow.maddening_touch->effectN( 1 ).percent() ) )
      {
        priest().generate_insanity(
            priest().talents.shadow.maddening_touch_insanity->effectN( 1 ).resource( RESOURCE_INSANITY ),
            priest().gains.insanity_maddening_touch, d->state->action );
      }

      priest().trigger_idol_of_nzoth( d->state->target, priest().procs.idol_of_nzoth_vt );
      vampiric_touch_heal->trigger( d->state->result_amount );

      if ( priest().talents.shadow.surge_of_darkness.enabled() && rng().roll( surge_of_darkness_proc_rate ) )
      {
        priest().buffs.surge_of_darkness->trigger();
        priest().procs.surge_of_darkness_vt->occur();
      }
    }
  }
};

// ==========================================================================
// Devouring Plague
// ==========================================================================
struct devouring_plague_dot_state_t : public action_state_t
{
  double rolling_multiplier;

  devouring_plague_dot_state_t( action_t* a, player_t* t ) : action_state_t( a, t ), rolling_multiplier( 1.0 )
  {
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );
    fmt::print( s, " rolling_multiplier={}", rolling_multiplier );
    return s;
  }

  void initialize() override
  {
    action_state_t::initialize();
    rolling_multiplier = 1.0;
  }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    auto other_dp_state = debug_cast<const devouring_plague_dot_state_t*>( o );
    rolling_multiplier  = other_dp_state->rolling_multiplier;
  }

  double composite_ta_multiplier() const override
  {
    // Use the rolling multiplier to get the stored rolling damage of the previous DP (if it exists)
    // This will dynamically adjust as the actor gains/loses intellect
    return action_state_t::composite_ta_multiplier() * rolling_multiplier;
  }
};

struct devouring_plague_t final : public priest_spell_t
{
  struct devouring_plague_heal_t final : public priest_heal_t
  {
    mental_fortitude_t* mental_fortitude;
    double mental_fortitude_percentage;

    devouring_plague_heal_t( priest_t& p ) : priest_heal_t( "devouring_plague_heal", p, p.dot_spells.devouring_plague )
    {
      background         = true;
      may_crit           = false;
      may_miss           = false;
      base_dd_multiplier = 1.0;

      // Turn off resource consumption
      base_costs[ RESOURCE_INSANITY ] = 0;

      // Turn off all damage parts of the spell
      spell_power_mod.direct = spell_power_mod.tick = base_td_multiplier = 0;
      dot_duration                                                       = timespan_t::from_seconds( 0 );

      mental_fortitude            = p.background_actions.mental_fortitude;
      mental_fortitude_percentage = priest().talents.shadow.mental_fortitude->effectN( 1 ).percent();
    }

    void impact( action_state_t* state ) override
    {
      priest_heal_t::impact( state );

      if ( priest().talents.shadow.mental_fortitude.enabled() &&
           state->target->current_health() == state->target->max_health() )
        trigger_mental_fortitude( state );
    }

    void trigger_mental_fortitude( action_state_t* state )
    {
      double current_value = 0;
      if ( mental_fortitude->target_specific[ state->target ] )
        current_value = mental_fortitude->target_specific[ state->target ]->current_value;

      double amount = current_value;
      amount += state->result_total;

      sim->print_debug( "mental_fortitude_percentage: {}", mental_fortitude_percentage );

      amount = std::min( amount, state->target->max_health() * mental_fortitude_percentage );

      mental_fortitude->base_dd_min = mental_fortitude->base_dd_max = amount;

      mental_fortitude->execute();
    }

    void trigger( double original_amount )
    {
      base_dd_min = base_dd_max = original_amount * data().effectN( 2 ).m_value();
      execute();
    }
  };

  propagate_const<devouring_plague_heal_t*> devouring_plague_heal;
  bool casted;
  double surge_of_darkness_proc_rate;

  devouring_plague_t( priest_t& p, bool _casted = false )
    : priest_spell_t( "devouring_plague", p, p.dot_spells.devouring_plague ),
      devouring_plague_heal( new devouring_plague_heal_t( p ) ),
      surge_of_darkness_proc_rate( priest().talents.shadow.surge_of_darkness->proc_chance() )
  {
    casted                     = _casted;
    may_crit                   = true;
    affected_by_shadow_weaving = true;
  }

  devouring_plague_t( priest_t& p, util::string_view options_str ) : devouring_plague_t( p, true )
  {
    parse_options( options_str );
  }

  action_state_t* new_state() override
  {
    return new devouring_plague_dot_state_t( this, target );
  }

  devouring_plague_dot_state_t* cast_state( action_state_t* s )
  {
    return debug_cast<devouring_plague_dot_state_t*>( s );
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_persistent_multiplier( s );

    // Spelldata does not have data for Devouring Plague so apply_buff_effects does not work with DP
    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T29, B2 ) && priest().buffs.gathering_shadows->check() )
    {
      m *= 1 + priest().buffs.gathering_shadows->check_stack_value();
    }

    return m;
  }

  void consume_resource() override
  {
    if ( casted )
    {
      priest_spell_t::consume_resource();
    }

    // Damnation does not consume Mind Devourer - 2022-10-01
    if ( priest().buffs.mind_devourer->up() && casted )
    {
      priest().buffs.mind_devourer->decrement();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // Benefit Tracking
    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T29, B2 ) )
    {
      priest().buffs.gathering_shadows->up();
    }

    // Damnation does not trigger a SA - 2022-10-01
    if ( casted )
    {
      priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_dp, s->result == RESULT_CRIT );
    }

    if ( result_is_hit( s->result ) )
    {
      devouring_plague_heal->trigger( s->result_amount );

      priest().refresh_insidious_ire_buff( s );
    }

    if ( priest().talents.shadow.surge_of_darkness.enabled() && rng().roll( surge_of_darkness_proc_rate ) )
    {
      priest().buffs.surge_of_darkness->trigger();
      priest().procs.surge_of_darkness_dp->occur();
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      devouring_plague_heal->trigger( d->state->result_amount );

      if ( priest().talents.shadow.surge_of_darkness.enabled() && rng().roll( surge_of_darkness_proc_rate ) )
      {
        priest().buffs.surge_of_darkness->trigger();
        priest().procs.surge_of_darkness_dp->occur();
      }
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.shadow.mind_flay_insanity.enabled() )
    {
      priest().buffs.mind_flay_insanity->trigger();
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T29, B2 ) )
    {
      priest().buffs.gathering_shadows->expire();
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T29, B4 ) )
    {
      priest().buffs.dark_reveries->trigger();
    }
  }

  timespan_t calculate_dot_refresh_duration( const dot_t* d, timespan_t duration ) const override
  {
    // if you only have the partial tick, roll that damage over
    if ( d->ticks_left_fractional() < 1 )
    {
      return duration + d->time_to_next_full_tick() - tick_time( d->state );
    }
    // otherwise lose the partial tick
    else
    {
      return duration + d->time_to_next_full_tick();
    }
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    priest_spell_t::snapshot_state( s, rt );

    double multiplier = 1.0;
    dot_t* dot        = get_dot( s->target );

    // Calculate how much damage is left in the remaining Devouring Plague
    // Convert that into a ratio so we can apply a modifier to every new tick of Devouring Plague
    if ( dot->is_ticking() )
    {
      action_state_t* old_s = dot->state;
      action_state_t* new_s = s;

      timespan_t time_to_next_tick = dot->time_to_next_tick();
      timespan_t old_remains       = dot->remains();
      timespan_t new_remains       = calculate_dot_refresh_duration( dot, composite_dot_duration( new_s ) );
      timespan_t old_tick          = tick_time( old_s );
      timespan_t new_tick          = tick_time( new_s );
      double old_multiplier        = cast_state( old_s )->rolling_multiplier;

      sim->print_debug(
          "{} {} calculations - time_to_next_tick: {}, old_remains: {}, new_remains: {}, old_tick: {}, new_tick: {}, "
          "old_multiplier: {}",
          *player, *this, time_to_next_tick, old_remains, new_remains, old_tick, new_tick, old_multiplier );

      // figure out how many old ticks to roll over
      double num_ticks = ( old_remains - time_to_next_tick ) / old_tick;

      // find number of ticks in new DP
      double new_num_ticks = ( new_remains - time_to_next_tick ) / new_tick;

      // if just a partial is left roll that over with the reduced dot duration
      if ( dot->ticks_left_fractional() < 1 )
      {
        num_ticks = dot->ticks_left_fractional() - dot->ticks_left() + 1;
        new_num_ticks += dot->ticks_left_fractional() - 1;
      }

      sim->print_debug( "{} {} calculations - num_ticks: {}, new_num_ticks: {}", *player, *this, num_ticks,
                        new_num_ticks );

      // figure out the increase for each new tick of DP
      double total_coefficient     = num_ticks * old_multiplier;
      double increase_per_new_tick = total_coefficient / ( new_num_ticks + 1 );

      multiplier = 1 + increase_per_new_tick;

      sim->print_debug( "{} {} modifier updated per tick from previous dot. Modifier per tick went from {} to {}.",
                        *player, *this, old_multiplier, multiplier );
    }

    cast_state( s )->rolling_multiplier = multiplier;
  }
};

// ==========================================================================
// Void Bolt
// ==========================================================================
struct void_bolt_t final : public priest_spell_t
{
  struct void_bolt_extension_t final : public priest_spell_t
  {
    timespan_t dot_extension;

    void_bolt_extension_t( priest_t& p, const spell_data_t* rank2_spell )
      : priest_spell_t( "void_bolt_extension", p, rank2_spell )
    {
      dot_extension = data().effectN( 1 ).time_value();
      aoe           = -1;
      radius        = p.specs.void_bolt->effectN( 1 ).trigger()->effectN( 1 ).radius_max();
      may_miss      = false;
      background = dual = true;
      energize_type     = action_energize::ON_CAST;
      travel_speed      = 0;
    }

    void impact( action_state_t* s ) override
    {
      priest_spell_t::impact( s );

      priest_td_t& td = get_td( s->target );

      td.dots.shadow_word_pain->adjust_duration( dot_extension, true );
      td.dots.vampiric_touch->adjust_duration( dot_extension, true );

      priest().refresh_insidious_ire_buff( s );
    }
  };

  void_bolt_extension_t* void_bolt_extension;

  void_bolt_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "void_bolt", p, p.specs.void_bolt ), void_bolt_extension( nullptr )
  {
    parse_options( options_str );
    use_off_gcd                = true;
    energize_type              = action_energize::ON_CAST;
    cooldown->hasted           = true;
    affected_by_shadow_weaving = true;

    auto rank2 = p.find_rank_spell( "Void Bolt", "Rank 2" );
    if ( rank2->ok() )
    {
      void_bolt_extension = new void_bolt_extension_t( p, rank2 );
    }
  }

  bool ready() override
  {
    if ( !priest().buffs.voidform->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_vb, s->result == RESULT_CRIT );

    if ( void_bolt_extension )
    {
      void_bolt_extension->target = s->target;
      void_bolt_extension->schedule_execute();
    }

    if ( result_is_hit( s->result ) && priest().is_ptr() )
    {
      priest().trigger_psychic_link( s );
    }
  }
};

// ==========================================================================
// Dark Ascension
// ==========================================================================
struct dark_ascension_t final : public priest_spell_t
{
  dark_ascension_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "dark_ascension", p, p.talents.shadow.dark_ascension )
  {
    parse_options( options_str );

    may_miss = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.dark_ascension->trigger();

    if ( priest().talents.shadow.ancient_madness.enabled() )
    {
      priest().buffs.ancient_madness->trigger();
    }
  }

  bool ready() override
  {
    return priest_spell_t::ready();
  }
};

// ==========================================================================
// Void Eruption
// ==========================================================================
struct void_eruption_damage_t final : public priest_spell_t
{
  propagate_const<action_t*> void_bolt;

  void_eruption_damage_t( priest_t& p )
    : priest_spell_t( "void_eruption_damage", p, p.talents.shadow.void_eruption_damage ), void_bolt( nullptr )
  {
    may_miss                   = false;
    background                 = true;
    affected_by_shadow_weaving = true;
  }

  void init() override
  {
    priest_spell_t::init();
    void_bolt = player->find_action( "void_bolt" );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    priest_spell_t::impact( s );

    // BUG: on beta this is hitting 4 times instead of 2 on your main target, not sure why
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/963
    if ( priest().bugs && s->target == parent_dot->target )
    {
      priest_spell_t::impact( s );
      priest_spell_t::impact( s );
    }
  }
};

struct void_eruption_t final : public priest_spell_t
{
  void_eruption_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "void_eruption", p, p.talents.shadow.void_eruption )
  {
    parse_options( options_str );

    impact_action = new void_eruption_damage_t( p );
    add_child( impact_action );

    may_miss = false;
    aoe      = -1;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.voidform->trigger();
  }

  void consume_resource() override
  {
    // does not consume any insanity, even though it has a cost. So do nothing.
  }

  bool ready() override
  {
    if ( priest().buffs.voidform->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

// ==========================================================================
// Dark Void
// Currently only hits targets that it will DoT
// TODO: adjust targeting logic to be more accurate above 16 targets
// ==========================================================================
struct dark_void_t final : public priest_spell_t
{
  propagate_const<shadow_word_pain_t*> child_swp;
  double insanity_gain;

  dark_void_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "dark_void", p, p.talents.shadow.dark_void ),
      child_swp( new shadow_word_pain_t( priest(), false ) ),
      insanity_gain( p.talents.shadow.dark_void_insanity->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    may_miss                   = false;
    radius                     = data().effectN( 1 ).radius_max();
    affected_by_shadow_weaving = true;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().generate_insanity( insanity_gain, priest().gains.insanity_dark_void, execute_state->action );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    child_swp->target = s->target;
    child_swp->execute();
  }
};

// ==========================================================================
// Mind Spike
// ==========================================================================
struct mind_spike_t final : public priest_spell_t
{
  timespan_t manipulation_cdr;

  mind_spike_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mind_spike", p, p.talents.shadow.mind_spike ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) )

  {
    parse_options( options_str );
    affected_by_shadow_weaving = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    if ( priest().talents.shadow.surge_of_darkness && priest().buffs.surge_of_darkness->check() )
    {
      m *= 1 + priest().talents.shadow.surge_of_darkness_buff->effectN( 2 ).percent();
    }

    return m;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      priest().trigger_psychic_link( s );

      if ( s->result == RESULT_CRIT && priest().talents.shadow.whispers_of_the_damned.enabled() )
      {
        priest().generate_insanity(
            priest().talents.shadow.whispers_of_the_damned->effectN( 2 ).resource( RESOURCE_INSANITY ),
            priest().gains.insanity_whispers_of_the_damned, s->action );
      }

      if ( priest().talents.shadow.surge_of_darkness.enabled() )
      {
        priest().buffs.surge_of_darkness->decrement();
      }

      priest().buffs.coalescing_shadows->expire();
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.manipulation.enabled() )
    {
      priest().cooldowns.mindgames->adjust( -manipulation_cdr );
    }

    if ( priest().talents.shadow.mind_melt.enabled() )
    {
      priest().buffs.mind_melt->trigger();
    }
  }
};

// ==========================================================================
// Psychic Horror
// ==========================================================================
struct psychic_horror_t final : public priest_spell_t
{
  psychic_horror_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "psychic_horror", p, p.talents.shadow.psychic_horror )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;
  }
};

// ==========================================================================
// Idol of C'Thun (Talent)
// ==========================================================================
struct idol_of_cthun_t final : public priest_spell_t
{
  idol_of_cthun_t( priest_t& p ) : priest_spell_t( "idol_of_cthun", p, p.talents.shadow.idol_of_cthun )
  {
    background = true;
  }
};

// ==========================================================================
// Void Torrent (Talent)
// ==========================================================================
struct void_torrent_t final : public priest_spell_t
{
  double insanity_gain;

  void_torrent_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "void_torrent", p, p.talents.shadow.void_torrent ),
      insanity_gain( p.talents.shadow.void_torrent->effectN( 3 ).trigger()->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    may_crit                   = false;
    channeled                  = true;
    use_off_gcd                = true;
    tick_zero                  = true;
    dot_duration               = data().duration();
    affected_by_shadow_weaving = true;

    // Getting insanity from the trigger spell data, base spell doesn't have it
    energize_type     = action_energize::PER_TICK;
    energize_resource = RESOURCE_INSANITY;
    energize_amount   = insanity_gain;
  }

  // DoT duration is fixed at 3s
  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return dot_duration;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest().talents.shadow.dark_evangelism.enabled() )
    {
      priest().buffs.dark_evangelism->trigger();
    }

    if ( priest().shadow_weaving_active_dots( target, id ) != 3 )
    {
      priest().procs.void_torrent_ticks_no_mastery->occur();
    }

    if ( priest().talents.shadow.psychic_link.enabled() && priest().is_ptr() )
    {
      priest().trigger_psychic_link( d->state );
    }
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );

    priest().buffs.void_torrent->expire();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.void_torrent->trigger();
  }
};

// ==========================================================================
// Psychic Link
// ==========================================================================
struct psychic_link_base_t final : public priest_spell_t
{
  psychic_link_base_t( util::string_view n, priest_t& p, const spell_data_t* s ) : priest_spell_t( n, p, s )
  {
    background = true;
    may_crit   = false;
    may_miss   = false;
    radius     = data().effectN( 1 ).radius_max();
    school     = SCHOOL_SHADOW;
  }

  void trigger( player_t* target, double original_amount, std::string action_name )
  {
    base_dd_min = base_dd_max = ( original_amount * data().effectN( 1 ).percent() );
    player->sim->print_debug( "{} triggered {} psychic_link on target {} from {}.", priest(),
                              data().effectN( 1 ).percent(), *target, action_name );

    set_target( target );
    execute();
  }
};

struct psychic_link_t final : public priest_spell_t
{
  psychic_link_t( priest_t& p )
    : priest_spell_t( "psychic_link", p, p.talents.shadow.psychic_link ),
      _pl_mind_blast( new psychic_link_base_t( "psychic_link_mind_blast", p, p.talents.shadow.psychic_link ) ),
      _pl_mind_spike( new psychic_link_base_t( "psychic_link_mind_spike", p, p.talents.shadow.psychic_link ) ),
      _pl_mind_flay( new psychic_link_base_t( "psychic_link_mind_flay", p, p.talents.shadow.psychic_link ) ),
      _pl_mind_flay_insanity(
          new psychic_link_base_t( "psychic_link_mind_flay_insanity", p, p.talents.shadow.psychic_link ) ),
      _pl_mindgames( new psychic_link_base_t( "psychic_link_mindgames", p, p.talents.shadow.psychic_link ) ),
      _pl_void_bolt( new psychic_link_base_t( "psychic_link_void_bolt", p, p.talents.shadow.psychic_link ) ),
      _pl_void_torrent( new psychic_link_base_t( "psychic_link_void_torrent", p, p.talents.shadow.psychic_link ) )
  {
    background  = true;
    radius      = data().effectN( 1 ).radius_max();
    callbacks   = false;
    base_dd_min = base_dd_max = 0;

    add_child( _pl_mind_blast );
    add_child( _pl_mind_spike );
    add_child( _pl_mind_flay );
    add_child( _pl_mind_flay_insanity );
    add_child( _pl_mindgames );
    add_child( _pl_void_bolt );
    add_child( _pl_void_torrent );
  }

  void trigger( player_t* target, double original_amount, std::string action_name )
  {
    if ( action_name == "mind_blast" )
    {
      _pl_mind_blast->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "mind_spike" )
    {
      _pl_mind_spike->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "mind_flay" )
    {
      _pl_mind_flay->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "mind_flay_insanity" )
    {
      _pl_mind_flay_insanity->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "mindgames" && player->is_ptr() )
    {
      _pl_mindgames->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "void_bolt" && player->is_ptr() )
    {
      _pl_void_bolt->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "void_torrent" && player->is_ptr() )
    {
      _pl_void_torrent->trigger( target, original_amount, action_name );
    }
    else
    {
      player->sim->print_debug( "{} tried to trigger psychic_link from unknown action {}.", priest(), action_name );
    }
  }

private:
  propagate_const<psychic_link_base_t*> _pl_mind_blast;
  propagate_const<psychic_link_base_t*> _pl_mind_spike;
  propagate_const<psychic_link_base_t*> _pl_mind_flay;
  propagate_const<psychic_link_base_t*> _pl_mind_flay_insanity;
  propagate_const<psychic_link_base_t*> _pl_mindgames;
  propagate_const<psychic_link_base_t*> _pl_void_bolt;
  propagate_const<psychic_link_base_t*> _pl_void_torrent;
};

// ==========================================================================
// Pain of Death
// ==========================================================================
struct pain_of_death_t final : public priest_spell_t
{
  pain_of_death_t( priest_t& p ) : priest_spell_t( "pain_of_death", p, p.talents.shadow.pain_of_death )
  {
    background = true;
    may_crit   = false;
    may_miss   = false;
  }

  void trigger( player_t* target, double original_amount )
  {
    base_dd_min = base_dd_max = ( original_amount * data().effectN( 2 ).percent() );
    player->sim->print_debug( "{} triggered pain_of_death on target {} for {}% of damage.", priest(), *target,
                              data().effectN( 2 ).percent() * 100 );

    set_target( target );
    execute();
  }
};

// ==========================================================================
// Shadow Weaving
// ==========================================================================
struct shadow_weaving_t final : public priest_spell_t
{
  shadow_weaving_t( priest_t& p ) : priest_spell_t( "shadow_weaving", p, p.find_spell( 346111 ) )
  {
    background                 = true;
    affected_by_shadow_weaving = false;
    may_crit                   = false;
    may_miss                   = false;
    callbacks                  = false;
  }

  void trigger( player_t* target, double original_amount )
  {
    base_dd_min = base_dd_max = ( original_amount * ( priest().shadow_weaving_multiplier( target, 0 ) - 1 ) );
    player->sim->print_debug( "{} triggered shadow weaving on target {}.", priest(), *target );

    set_target( target );
    execute();
  }
};

// ==========================================================================
// Shadow Crash
// TODO: double check DoT target logic
// ==========================================================================
struct shadow_crash_damage_t final : public priest_spell_t
{
  shadow_crash_damage_t( priest_t& p )
    : priest_spell_t( "shadow_crash_damage", p, p.talents.shadow.shadow_crash->effectN( 1 ).trigger() )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
  }
};

// Shadow Crash DoT interactions:
// Triggers SWP with Misery enabled
// Does not interact with Unfurling Darkness (procs or consumption)
struct shadow_crash_dots_t final : public priest_spell_t
{
  propagate_const<vampiric_touch_t*> child_vt;
  double missile_speed;

  shadow_crash_dots_t( priest_t& p, double _missile_speed )
    : priest_spell_t( "shadow_crash_dots", p, p.talents.shadow.shadow_crash->effectN( 3 ).trigger() ),
      child_vt( new vampiric_touch_t( priest(), true, false, false, false ) ),
      missile_speed( _missile_speed )
  {
    may_miss   = false;
    background = true;
    aoe        = as<int>( data().effectN( 1 ).base_value() );

    child_vt->background = true;
  }

  std::vector<player_t*>& target_list() const override
  {
    // Force regen this every time
    target_cache.is_valid = false;
    auto& tl              = priest_spell_t::target_list();
    double original_size  = tl.size();

    // if target_list is bigger than dot cap shuffle the list
    if ( as<int>( tl.size() ) > aoe )
    {
      // randomize targets
      rng().shuffle( tl.begin(), tl.end() );

      // sort targets without Vampiric Touch to the front
      std::sort( tl.begin(), tl.end(), [ this ]( player_t* l, player_t* r ) {
        priest_td_t* tdl = priest().get_target_data( l );
        priest_td_t* tdr = priest().get_target_data( r );

        if ( !tdl->dots.vampiric_touch->is_ticking() && tdr->dots.vampiric_touch->is_ticking() )
        {
          return true;
        }

        return false;
      } );

      // resize to dot target cap
      tl.resize( aoe );
    }

    player->sim->print_debug( "{} shadow_crash dots {} targets of the available {}.", priest(), tl.size(),
                              original_size );

    return tl;
  }

  // Copy travel time from parent spell
  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( missile_speed );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    child_vt->target = s->target;
    child_vt->execute();
  }
};

struct shadow_crash_t final : public priest_spell_t
{
  double insanity_gain;
  propagate_const<shadow_crash_dots_t*> shadow_crash_dots;

  shadow_crash_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "shadow_crash", p, p.talents.shadow.shadow_crash ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) ),
      shadow_crash_dots( new shadow_crash_dots_t( p, data().missile_speed() ) )
  {
    parse_options( options_str );

    aoe    = -1;
    radius = data().effectN( 1 ).radius();
    range  = data().max_range();

    impact_action = new shadow_crash_damage_t( p );
    add_child( impact_action );
  }

  // Shadow Crash has fixed travel time
  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( data().missile_speed() );
  }

  void execute() override
  {
    priest_spell_t::execute();

    shadow_crash_dots->execute();
  }
};

// ==========================================================================
// Damnation
// Triggers :
//  - SW:P as if it was hard casted
//  - SW:P twice if Misery is talented making it pandemic duration
//  - Unfurling Darkness proc but cannot consume it
//  - Gives 7 Insanity from VT + SWP
// ==========================================================================
struct damnation_t final : public priest_spell_t
{
  propagate_const<shadow_word_pain_t*> child_swp;
  propagate_const<vampiric_touch_t*> child_vt;
  propagate_const<devouring_plague_t*> child_dp;

  damnation_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "damnation", p, p.talents.shadow.damnation ),
      child_swp( new shadow_word_pain_t( priest(), true ) ),
      child_vt( new vampiric_touch_t( priest(), true, true, true, false ) ),
      child_dp( new devouring_plague_t( priest(), false ) )
  {
    parse_options( options_str );
    child_swp->background = true;
    child_vt->background  = true;
    child_dp->background  = true;

    may_miss = false;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    child_swp->target = s->target;
    child_vt->target  = s->target;
    child_dp->target  = s->target;

    child_swp->execute();
    child_vt->execute();
    child_dp->execute();
  }
};

}  // namespace spells

namespace heals
{
}  // namespace heals

}  // namespace actions

namespace buffs
{
// ==========================================================================
// Voidform
// ==========================================================================
struct voidform_t final : public priest_buff_t<buff_t>
{
  voidform_t( priest_t& p ) : base_t( p, "voidform", p.specs.voidform )
  {
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

    // Set cooldown to 0s, cooldown is stored in Void Eruption
    cooldown->duration = timespan_t::from_seconds( 0 );

    // Use a stack change callback to trigger voidform effects.
    set_stack_change_callback( [ this ]( buff_t*, int, int cur ) {
      if ( cur )
      {
        priest().cooldowns.mind_blast->reset( true, -1 );
        priest().cooldowns.void_bolt->reset( true );
      }
    } );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    if ( priest().talents.shadow.ancient_madness.enabled() )
    {
      priest().buffs.ancient_madness->trigger();
    }

    priest().buffs.shadowform->expire();

    return r;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    if ( priest().buffs.shadowform_state->check() )
    {
      priest().buffs.shadowform->trigger();
    }

    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ==========================================================================
// Shadowform
// ==========================================================================
struct shadowform_t final : public priest_buff_t<buff_t>
{
  shadowform_t( priest_t& p ) : base_t( p, "shadowform", p.specs.shadowform )
  {
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

// ==========================================================================
// Shadowform State
// Hidden shadowform state tracking buff, so we can decide whether to bring
// back the shadowform buff after leaving voidform or not.
// ==========================================================================
struct shadowform_state_t final : public priest_buff_t<buff_t>
{
  shadowform_state_t( priest_t& p ) : base_t( p, "shadowform_state" )
  {
    set_chance( 1.0 );
    set_quiet( true );
  }
};

// ==========================================================================
// Death and Madness
// ==========================================================================
struct death_and_madness_buff_t final : public priest_buff_t<buff_t>
{
  double insanity_gain;

  death_and_madness_buff_t( priest_t& p )
    : base_t( p, "death_and_madness_insanity_gain", p.talents.death_and_madness_insanity ),
      insanity_gain( data().effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_death_and_madness, nullptr );
    } );
  }
};

// ==========================================================================
// Mental Fortitude
// ==========================================================================
struct mental_fortitude_buff_t final : public absorb_buff_t
{
  mental_fortitude_buff_t( priest_t* player )
    : absorb_buff_t( player, "mental_fortitude", player->talents.shadow.mental_fortitude )
  {
    set_absorb_source( player->get_stats( "mental_fortitude" ) );
  }
};

// ==========================================================================
// Shadowy Insight
// ==========================================================================
struct shadowy_insight_t final : public priest_buff_t<buff_t>
{
  shadowy_insight_t( priest_t& p ) : base_t( p, "shadowy_insight", p.find_spell( 375981 ) )
  {
    // BUG: RPPM value not found in spelldata
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/956
    set_rppm( RPPM_HASTE, 2.4 );
    // Allow player to react to the buff being applied so they can cast Mind Blast.
    this->reactable = true;

    // Create a stack change callback to adjust the number of Mind Blast charges.
    set_stack_change_callback(
        [ this ]( buff_t*, int old, int cur ) { priest().cooldowns.mind_blast->adjust_max_charges( cur - old ); } );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    if ( remaining_duration == timespan_t::zero() )
    {
      for ( int i = 0; i < expiration_stacks; i++ )
      {
        priest().procs.shadowy_insight_missed->occur();
      }
    }

    base_t::expire_override( expiration_stacks, remaining_duration );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( !priest().talents.shadow.shadowy_insight.enabled() )
      return false;

    return priest_buff_t::trigger( stacks, value, chance, duration );
  }
};

// ==========================================================================
// Ancient Madness
// ==========================================================================
struct ancient_madness_t final : public priest_buff_t<buff_t>
{
  ancient_madness_t( priest_t& p ) : base_t( p, "ancient_madness", p.talents.shadow.ancient_madness )
  {
    if ( !data().ok() )
      return;

    add_invalidate( CACHE_CRIT_CHANCE );
    add_invalidate( CACHE_SPELL_CRIT_CHANCE );
    set_reverse( true );
    set_period( timespan_t::from_seconds( 1 ) );
    set_duration( p.specs.voidform->duration() );  // Uses the same duration as Voidform for tooltip

    // BUG: Ancient Madness consumes twice as much crit with Voidform
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/1030
    if ( priest().bugs && priest().talents.shadow.void_eruption.enabled() && !priest().is_ptr() )
    {
      set_default_value( 0.02 );                                     // 2%
      set_max_stack( as<int>( data().effectN( 3 ).base_value() ) );  // 5/10;
    }
    else
    {
      set_default_value( data().effectN( 2 ).percent() );  // 0.5%/1%
      set_max_stack( 20 );                                 // 20/20;
    }
  }
};

// TODO: implement healing from Intangibility
struct dispersion_t final : public priest_buff_t<buff_t>
{
  // TODO: hook up rank2 to movement speed
  const spell_data_t* rank2;

  dispersion_t( priest_t& p )
    : base_t( p, "dispersion", p.find_class_spell( "Dispersion" ) ),
      rank2( p.find_specialization_spell( 322108, PRIEST_SHADOW ) )
  {
  }
};

}  // namespace buffs

// ==========================================================================
// Tick Damage over Time
// Calculate damage a DoT has left given a certain time period
// ==========================================================================
double priest_t::tick_damage_over_time( timespan_t duration, const dot_t* dot ) const
{
  if ( !dot->is_ticking() )
  {
    return 0.0;
  }
  action_state_t* state = dot->current_action->get_state( dot->state );
  dot->current_action->calculate_tick_amount( state, 1.0 );
  double tick_base_damage  = state->result_raw;
  timespan_t dot_tick_time = dot->current_action->tick_time( state );
  // We don't care how much is remaining on the target, this will always deal
  // Xs worth of DoT ticks even if the amount is currently less
  double ticks_left   = duration / dot_tick_time;
  double total_damage = ticks_left * tick_base_damage;
  action_state_t::release( state );
  return total_damage;
}

// ==========================================================================
// Generate Insanity
// Helper method for generating the proper amount of insanity
// ==========================================================================
void priest_t::generate_insanity( double num_amount, gain_t* g, action_t* action )
{
  if ( specialization() == PRIEST_SHADOW )
  {
    resource_gain( RESOURCE_INSANITY, num_amount, g, action );
  }
}

void priest_t::create_buffs_shadow()
{
  // Baseline
  buffs.shadowform       = make_buff<buffs::shadowform_t>( *this );
  buffs.shadowform_state = make_buff<buffs::shadowform_state_t>( *this );
  buffs.vampiric_embrace = make_buff( this, "vampiric_embrace", talents.vampiric_embrace );
  buffs.voidform         = make_buff<buffs::voidform_t>( *this );
  buffs.dispersion       = make_buff<buffs::dispersion_t>( *this );

  // Talents
  buffs.ancient_madness        = make_buff<buffs::ancient_madness_t>( *this );
  buffs.death_and_madness_buff = make_buff<buffs::death_and_madness_buff_t>( *this );
  buffs.unfurling_darkness =
      make_buff( this, "unfurling_darkness", talents.shadow.unfurling_darkness->effectN( 1 ).trigger() );
  buffs.unfurling_darkness_cd =
      make_buff( this, "unfurling_darkness_cd",
                 talents.shadow.unfurling_darkness->effectN( 1 ).trigger()->effectN( 2 ).trigger() );
  buffs.void_torrent      = make_buff( this, "void_torrent", talents.shadow.void_torrent );
  buffs.surge_of_darkness = make_buff( this, "surge_of_darkness", talents.shadow.surge_of_darkness_buff )
                                ->set_default_value_from_effect( 2 );  // automagic handles the cast time
  buffs.mind_devourer_ms_active = make_buff( this, "mind_devourer_ms_active" )->set_quiet( true )->set_duration( 0_s );
  buffs.mind_devourer           = make_buff( this, "mind_devourer", find_spell( 373204 ) )
                            ->set_trigger_spell( talents.shadow.mind_devourer )
                            ->set_chance( talents.shadow.mind_devourer->effectN( 1 ).percent() );
  buffs.shadowy_insight = make_buff<buffs::shadowy_insight_t>( *this );

  buffs.mental_fortitude = new buffs::mental_fortitude_buff_t( this );

  buffs.insidious_ire = make_buff( this, "insidious_ire", talents.shadow.insidious_ire )
                            ->set_duration( timespan_t::zero() )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  buffs.thing_from_beyond = make_buff( this, "thing_from_beyond", find_spell( 373277 ) );

  buffs.idol_of_yoggsaron =
      make_buff( this, "idol_of_yoggsaron", talents.shadow.idol_of_yoggsaron->effectN( 2 ).trigger() )
          ->set_stack_change_callback( ( [ this ]( buff_t* b, int, int cur ) {
            if ( cur == b->max_stack() )
            {
              make_event( b->sim, [ b ] { b->cancel(); } );
              procs.thing_from_beyond->occur();
              spawn_thing_from_beyond();
            }
          } ) );

  buffs.dark_evangelism =
      make_buff( this, "dark_evangelism", find_spell( 391099 ) )->set_trigger_spell( talents.shadow.dark_evangelism );

  buffs.coalescing_shadows = make_buff( this, "coalescing_shadows", talents.shadow.coalescing_shadows_buff )
                                 ->set_stack_change_callback( ( [ this ]( buff_t*, int old, int cur ) {
                                   if ( cur < old && old > 0 )
                                   {
                                     buffs.coalescing_shadows_dot->trigger();
                                   }
                                 } ) );  // TODO: Check in game if expiry counts as consumption - If not, Custom Buff
                                         // and override Expire to check for consumed rather than expire.
  buffs.coalescing_shadows_dot = make_buff( this, "coalescing_shadows_dot", talents.shadow.coalescing_shadows_dot_buff )
                                     ->set_trigger_spell( talents.shadow.coalescing_shadows_buff );

  buffs.devoured_pride = make_buff( this, "devoured_pride", talents.shadow.devoured_pride )
                             ->set_duration( timespan_t::zero() )
                             ->set_trigger_spell( talents.shadow.idol_of_yshaarj )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.mind_melt = make_buff( this, "mind_melt", talents.shadow.mind_melt->effectN( 1 ).trigger() )
                        ->set_default_value_from_effect( 1 );

  buffs.mind_flay_insanity = make_buff( this, "mind_flay_insanity", find_spell( 391401 ) )
                                 ->set_default_value( talents.shadow.mind_flay_insanity->effectN( 1 ).percent() );

  buffs.deathspeaker = make_buff( this, "deathspeaker", talents.shadow.deathspeaker->effectN( 1 ).trigger() );

  // TODO: use default_value to parse increase instead of stacks
  buffs.dark_ascension = make_buff( this, "dark_ascension", talents.shadow.dark_ascension )
                             ->set_default_value_from_effect( 1 )
                             ->set_cooldown( 0_s )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Tier Sets
  // 393684 -> 394961
  buffs.gathering_shadows =
      make_buff( this, "gathering_shadows", sets->set( PRIEST_SHADOW, T29, B2 )->effectN( 1 ).trigger() )
          ->set_default_value_from_effect( 1 );
  // 393685 -> 394963
  buffs.dark_reveries =
      make_buff<stat_buff_t>( this, "dark_reveries", sets->set( PRIEST_SHADOW, T29, B4 )->effectN( 1 ).trigger() )
          ->add_invalidate( CACHE_HASTE )
          ->set_default_value_from_effect( 1 );
}

void priest_t::init_rng_shadow()
{
  rppm.idol_of_cthun = get_rppm( "idol_of_cthun", talents.shadow.idol_of_cthun );
  rppm.deathspeaker  = get_rppm( "deathspeaker", talents.shadow.deathspeaker );
}

void priest_t::init_spells_shadow()
{
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };

  // Row 2
  talents.shadow.dispersion          = ST( "Dispersion" );
  talents.shadow.shadowy_apparition  = find_spell( 148859 );
  talents.shadow.shadowy_apparitions = ST( "Shadowy Apparitions" );
  talents.shadow.silence             = ST( "Silence" );
  // Row 3
  talents.shadow.intangibility      = ST( "Intangibility" );
  talents.shadow.mental_fortitude   = ST( "Mental Fortitude" );
  talents.shadow.misery             = ST( "Misery" );
  talents.shadow.dark_void          = ST( "Dark Void" );
  talents.shadow.dark_void_insanity = find_spell( 391450 );  // Not linked to Dark Void except in tooltip
  talents.shadow.last_word          = ST( "Last Word" );
  talents.shadow.psychic_horror     = ST( "Psychic Horror" );
  // Row 4
  talents.shadow.coalescing_shadows          = ST( "Coalescing Shadows" );
  talents.shadow.coalescing_shadows_buff     = find_spell( 391243 );
  talents.shadow.coalescing_shadows_dot_buff = find_spell( 391244 );
  talents.shadow.mind_sear                   = ST( "Mind Sear" );
  talents.shadow.mind_spike                  = ST( "Mind Spike" );
  // Row 5
  talents.shadow.puppet_master          = ST( "Puppet Master" );
  talents.shadow.mental_decay           = ST( "Mental Decay" );
  talents.shadow.void_eruption          = ST( "Void Eruption" );  // TODO: confirm CD is 2m
  talents.shadow.void_eruption_damage   = find_spell( 228360 );
  talents.shadow.dark_ascension         = ST( "Dark Ascension" );
  talents.shadow.unfurling_darkness     = ST( "Unfurling Darkness" );
  talents.shadow.surge_of_darkness      = ST( "Surge of Darkness" );
  talents.shadow.surge_of_darkness_buff = find_spell( 87160 );
  // Row 6
  talents.shadow.harnessed_shadows = ST( "Harnessed Shadows" );
  talents.shadow.shadowy_insight   = ST( "Shadowy Insight" );
  talents.shadow.ancient_madness   = ST( "Ancient Madness" );  // TODO: add point scaling
  talents.shadow.shadow_crash      = ST( "Shadow Crash" );
  talents.shadow.mind_melt         = ST( "Mind Melt" );
  // Row 7
  talents.shadow.maddening_touch          = ST( "Maddening Touch" );
  talents.shadow.maddening_touch_insanity = find_spell( 391232 );
  talents.shadow.dark_evangelism          = ST( "Dark Evangelism" );
  talents.shadow.auspicious_spirits       = ST( "Auspicious Spirits" );
  talents.shadow.tormented_spirits        = ST( "Tormented Spirits" );
  talents.shadow.psychic_link             = ST( "Psychic Link" );
  talents.shadow.whispers_of_the_damned   = ST( "Whispers of the Damned" );
  // Row 8
  talents.shadow.mindbender               = ST( "Mindbender" );
  talents.shadow.deathspeaker             = ST( "Deathspeaker" );
  talents.shadow.mind_flay_insanity       = ST( "Mind Flay: Insanity" );
  talents.shadow.mind_flay_insanity_spell = find_spell( 391403 );  // Not linked to talent, actual dmg spell
  talents.shadow.encroaching_shadows      = ST( "Encroaching Shadows" );
  talents.shadow.damnation                = ST( "Damnation" );
  talents.shadow.void_torrent             = ST( "Void Torrent" );
  // Row 9
  talents.shadow.inescapable_torment = ST( "Inescapable Torment" );
  talents.shadow.screams_of_the_void = ST( "Screams of the Void" );
  talents.shadow.pain_of_death       = ST( "Pain of Death" );
  talents.shadow.mind_devourer       = ST( "Mind Devourer" );
  talents.shadow.insidious_ire       = ST( "Insidious Ire" );  // TODO: check values
  talents.shadow.malediction         = ST( "Malediction" );
  // Row 10
  talents.shadow.idol_of_yshaarj = ST( "Idol of Y'Shaarj" );
  // TODO: Implement Stunned/Feared/Enraged Y'shaarj
  talents.shadow.devoured_pride    = find_spell( 373316 );  // Pet Damage, Your Damage - Healthy
  talents.shadow.devoured_despair  = find_spell( 373317 );  // Insanity Generation - Stunned - NYI
  talents.shadow.devoured_anger    = find_spell( 373318 );  // Haste - Enrage - NYI
  talents.shadow.devoured_fear     = find_spell( 373319 );  // Big Personal Damage - Feared - NYI
  talents.shadow.devoured_violence = find_spell( 373320 );  // Pet Extension - Default
  talents.shadow.idol_of_cthun     = ST( "Idol of C'Thun" );
  talents.shadow.idol_of_yoggsaron = ST( "Idol of Yogg-Saron" );
  talents.shadow.idol_of_nzoth     = ST( "Idol of N'Zoth" );

  // General Spells
  specs.mind_flay      = find_specialization_spell( "Mind Flay" );
  specs.shadowform     = find_specialization_spell( "Shadowform" );
  specs.void_bolt      = find_spell( 205448 );
  specs.voidform       = find_spell( 194249 );
  specs.hallucinations = find_spell( 280752 );
}

action_t* priest_t::create_action_shadow( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  if ( name == "mind_flay" )
  {
    return new mind_flay_t( *this, options_str );
  }
  if ( name == "void_bolt" )
  {
    return new void_bolt_t( *this, options_str );
  }
  if ( name == "void_eruption" )
  {
    return new void_eruption_t( *this, options_str );
  }
  if ( name == "mind_sear" )
  {
    return new mind_sear_t( *this, options_str );
  }
  if ( name == "shadow_crash" )
  {
    return new shadow_crash_t( *this, options_str );
  }
  if ( name == "void_torrent" )
  {
    return new void_torrent_t( *this, options_str );
  }
  if ( name == "shadow_word_pain" )
  {
    return new shadow_word_pain_t( *this, options_str );
  }
  if ( name == "vampiric_touch" )
  {
    return new vampiric_touch_t( *this, options_str );
  }
  if ( name == "dispersion" )
  {
    return new dispersion_t( *this, options_str );
  }
  if ( name == "silence" )
  {
    return new silence_t( *this, options_str );
  }
  if ( name == "psychic_horror" )
  {
    return new psychic_horror_t( *this, options_str );
  }
  if ( name == "vampiric_embrace" )
  {
    return new vampiric_embrace_t( *this, options_str );
  }
  if ( name == "shadowform" )
  {
    return new shadowform_t( *this, options_str );
  }
  if ( name == "devouring_plague" )
  {
    return new devouring_plague_t( *this, options_str );
  }
  if ( name == "damnation" )
  {
    return new damnation_t( *this, options_str );
  }
  if ( name == "dark_void" )
  {
    return new dark_void_t( *this, options_str );
  }
  if ( name == "mind_spike" )
  {
    return new mind_spike_t( *this, options_str );
  }
  if ( name == "dark_ascension" )
  {
    return new dark_ascension_t( *this, options_str );
  }

  return nullptr;
}

std::unique_ptr<expr_t> priest_t::create_expression_shadow( util::string_view name_str )
{
  if ( name_str == "shadowy_apparitions_in_flight" )
  {
    return make_fn_expr( name_str, [ this ]() {
      if ( !background_actions.shadowy_apparitions )
      {
        return 0.0;
      }

      return static_cast<double>( background_actions.shadowy_apparitions->num_travel_events() );
    } );
  }

  return nullptr;
}

void priest_t::init_background_actions_shadow()
{
  if ( talents.shadow.shadowy_apparitions.enabled() )
  {
    background_actions.shadowy_apparitions = new actions::spells::shadowy_apparition_spell_t( *this );
  }

  if ( talents.shadow.psychic_link.enabled() )
  {
    background_actions.psychic_link = new actions::spells::psychic_link_t( *this );
  }

  if ( talents.shadow.pain_of_death.enabled() )
  {
    background_actions.pain_of_death = new actions::spells::pain_of_death_t( *this );
  }

  if ( talents.shadow.idol_of_cthun.enabled() )
  {
    background_actions.idol_of_cthun = new actions::spells::idol_of_cthun_t( *this );
  }

  if ( talents.shadow.mental_fortitude.enabled() )
  {
    background_actions.mental_fortitude = new actions::spells::mental_fortitude_t( *this );
  }

  background_actions.shadow_weaving = new actions::spells::shadow_weaving_t( *this );

  background_actions.shadow_word_pain = new actions::spells::shadow_word_pain_t( *this );
}

// ==========================================================================
// Trigger Shadowy Apparitions on all targets affected by vampiric touch
// ==========================================================================
void priest_t::trigger_shadowy_apparitions( proc_t* proc, bool gets_crit_mod )
{
  if ( !talents.shadow.shadowy_apparitions.enabled() )
  {
    return;
  }

  // Idol of Yogg-Saron only triggers for each cast that generates an apparition
  if ( talents.shadow.idol_of_yoggsaron.enabled() )
  {
    buffs.idol_of_yoggsaron->trigger();
  }

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && priest_td->dots.vampiric_touch->is_ticking() )
    {
      background_actions.shadowy_apparitions->trigger( priest_td->target, proc, gets_crit_mod );
    }
  }
}

// ==========================================================================
// Trigger Shadowy Apparitions on all targets affected by vampiric touch
// ==========================================================================
int priest_t::number_of_echoing_voids_active()
{
  int echoing_voids_active = 0;

  if ( !talents.shadow.idol_of_nzoth.enabled() )
  {
    return echoing_voids_active;
  }

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && priest_td->buffs.echoing_void->check() )
    {
      echoing_voids_active++;
    }
  }

  return echoing_voids_active;
}

// ==========================================================================
// Trigger Psychic Link on any targets that weren't the original target and have Vampiric Touch ticking on them
// ==========================================================================
void priest_t::trigger_psychic_link( action_state_t* s )
{
  if ( !talents.shadow.psychic_link.enabled() )
  {
    return;
  }

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && ( priest_td->target != s->target ) && priest_td->dots.vampiric_touch->is_ticking() )
    {
      background_actions.psychic_link->trigger( priest_td->target, s->result_amount, s->action->name_str );
    }
  }
}

// ==========================================================================
// Trigger Pain of Death on any targets that weren't the original target and have Shadow Word: Pain ticking on them
// ==========================================================================
void priest_t::trigger_pain_of_death( action_state_t* s )
{
  if ( !talents.shadow.pain_of_death.enabled() )
  {
    return;
  }

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && ( priest_td->target != s->target ) && priest_td->dots.shadow_word_pain->is_ticking() )
    {
      background_actions.pain_of_death->trigger( priest_td->target, s->result_amount );
    }
  }
}

// ==========================================================================
// Trigger Shadow Weaving on the Target
// ==========================================================================
void priest_t::trigger_shadow_weaving( action_state_t* s )
{
  background_actions.shadow_weaving->trigger( s->target, s->result_amount );
}

bool priest_t::is_screams_of_the_void_up( player_t* target, const unsigned int spell_id ) const
{
  priest_td_t* td = get_target_data( target );

  if ( talents.shadow.screams_of_the_void.enabled() )
  {
    // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/1038
    if ( spell_id == dot_spells.shadow_word_pain->id() &&
         ( td->dots.mind_flay->is_ticking() || td->dots.mind_flay_insanity->is_ticking() ) &&
         talents.shadow.mental_decay.enabled() && options.priest_screams_bug && bugs )
    {
      return false;
    }

    if ( td->dots.mind_flay->is_ticking() || td->dots.void_torrent->is_ticking() ||
         td->dots.mind_flay_insanity->is_ticking() ||
         ( talents.shadow.mind_sear.enabled() && channeling != nullptr &&
           channeling->id == talents.shadow.mind_sear->id() ) )
    {
      return true;
    }
  }
  return false;
}

// Helper function to refresh talbadars buff
void priest_t::refresh_insidious_ire_buff( action_state_t* s )
{
  if ( !talents.shadow.insidious_ire.enabled() )
    return;

  const priest_td_t* td = find_target_data( s->target );

  if ( !td )
    return;

  if ( td->dots.shadow_word_pain->is_ticking() && td->dots.vampiric_touch->is_ticking() &&
       td->dots.devouring_plague->is_ticking() )
  {
    timespan_t min_length = std::min( { td->dots.shadow_word_pain->remains(), td->dots.vampiric_touch->remains(),
                                        td->dots.devouring_plague->remains() } );

    if ( min_length >= buffs.insidious_ire->remains() )
    {
      if ( talents.shadow.insidious_ire.enabled() )
        buffs.insidious_ire->trigger( min_length );
    }
  }
}

void priest_t::trigger_idol_of_nzoth( player_t* target, proc_t* proc )
{
  if ( !talents.shadow.idol_of_nzoth.enabled() )
    return;

  auto td = get_target_data( target );

  if ( !td || ( !td->buffs.echoing_void->up() &&
                number_of_echoing_voids_active() == talents.shadow.idol_of_nzoth->effectN( 1 ).base_value() ) )
    return;

  if ( rng().roll( talents.shadow.idol_of_nzoth->proc_chance() ) )
  {
    proc->occur();
    td->buffs.echoing_void->trigger();
    if ( rng().roll( talents.shadow.idol_of_nzoth->proc_chance() ) )
    {
      int stacks = td->buffs.echoing_void->stack();
      sim->print_debug( "{} triggered echoing_void_collapse on target {} for {} stacks.", *this, target->name_str,
                        stacks );
      td->buffs.echoing_void_collapse->trigger( timespan_t::from_seconds( stacks + 1 ) );
    }
  }
}

void priest_t::spawn_thing_from_beyond()
{
  pets.thing_from_beyond.spawn();
}

}  // namespace priestspace

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
  double insanity_gain;

  mind_sear_tick_t( priest_t& p, const spell_data_t* s )
    : priest_spell_t( "mind_sear_tick", p, s ),
      insanity_gain( p.talents.shadow.mind_sear_insanity->effectN( 1 ).percent() )
  {
    affected_by_shadow_weaving = true;
    background                 = true;
    dual                       = true;
    aoe                        = -1;
    callbacks                  = false;
    direct_tick                = false;
    use_off_gcd                = true;
    dynamic_tick_action        = true;
    energize_type              = action_energize::PER_HIT;
    energize_amount            = insanity_gain;
    energize_resource          = RESOURCE_INSANITY;
    radius                     = data().effectN( 2 ).radius();  // base radius is 100yd, actual is stored in effect 2
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest().trigger_eternal_call_to_the_void( s );
    priest().trigger_idol_of_cthun( s );

    // TODO: convert to mental decay
    // if ( priest().talents.shadow.rot_and_wither.enabled() )
    // {
    //   if ( rng().roll( priest().talents.shadow.rot_and_wither->proc_chance() ) )
    //   {
    //     timespan_t dot_extension =
    //         timespan_t::from_seconds( priest().talents.shadow.rot_and_wither->effectN( 1 ).base_value() );
    //     priest_td_t& td = get_td( s->target );

    //     td.dots.shadow_word_pain->adjust_duration( dot_extension, true );
    //     td.dots.vampiric_touch->adjust_duration( dot_extension, true );
    //   }
    // }
  }
};

struct mind_sear_t final : public priest_spell_t
{
  mind_sear_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mind_sear", p, p.talents.shadow.mind_sear )
  {
    parse_options( options_str );
    channeled           = true;
    may_crit            = false;
    hasted_ticks        = false;
    dynamic_tick_action = true;
    tick_zero           = false;
    radius = data().effectN( 1 ).trigger()->effectN( 2 ).radius();  // need to set radius in here so that the APL
                                                                    // functions correctly
    if ( priest().specialization() == PRIEST_SHADOW )
      base_costs_per_tick[ RESOURCE_MANA ] = 0.0;

    tick_action = new mind_sear_tick_t( p, data().effectN( 1 ).trigger() );
  }
};

// ==========================================================================
// Mind Flay
// ==========================================================================
struct mind_flay_t final : public priest_spell_t
{
  mind_flay_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "mind_flay", p, p.specs.mind_flay )
  {
    parse_options( options_str );

    affected_by_shadow_weaving = true;
    may_crit                   = false;
    channeled                  = true;
    use_off_gcd                = true;
  }

  void trigger_mind_flay_dissonant_echoes()
  {
    if ( !priest().conduits.dissonant_echoes->ok() || priest().buffs.voidform->check() )
    {
      return;
    }

    if ( rng().roll( priest().conduits.dissonant_echoes.percent() ) )
    {
      priest().buffs.dissonant_echoes->trigger();
      priest().procs.dissonant_echoes->occur();
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    priest().trigger_eternal_call_to_the_void( d->state );
    priest().trigger_idol_of_cthun( d->state );
    trigger_mind_flay_dissonant_echoes();

    // TODO: convert to Mental Decay
    // if ( priest().talents.shadow.rot_and_wither.enabled() )
    // {
    //   if ( rng().roll( priest().talents.shadow.rot_and_wither->proc_chance() ) )
    //   {
    //     timespan_t dot_extension =
    //         timespan_t::from_seconds( priest().talents.shadow.rot_and_wither->effectN( 1 ).base_value() );
    //     priest_td_t& td = get_td( d->state->target );

    //     td.dots.shadow_word_pain->adjust_duration( dot_extension, true );
    //     td.dots.vampiric_touch->adjust_duration( dot_extension, true );
    //   }
    // }
  }

  bool ready() override
  {
    // Ascended Blast replaces Mind Flay when Boon of the Ascended is active
    if ( priest().buffs.boon_of_the_ascended->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
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

    if ( priest().talents.shadow.last_word.enabled() )
    {
      // Spell data has a negative value
      cooldown->duration += priest().talents.shadow.last_word->effectN( 1 ).time_value();
    }
  }

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    if ( target->type == ENEMY_ADD || target->level() < sim->max_player_level + 3 )
    {
      if ( priest().legendary.sephuzs_proclamation->ok() && priest().buffs.sephuzs_proclamation )
      {
        priest().buffs.sephuzs_proclamation->trigger();
      }
    }
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
  vampiric_embrace_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "vampiric_embrace", p, p.talents.vampiric_embrace )
  {
    parse_options( options_str );

    harmful = false;

    if ( priest().talents.sanlayn->ok() )
    {
      cooldown->duration += priest().talents.sanlayn->effectN( 1 ).time_value();
    }
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest().buffs.vampiric_embrace->trigger();

    if ( priest().talents.shadow.hallucinations.enabled() )
    {
      priest().generate_insanity( priest().talents.shadow.hallucinations->effectN( 1 ).base_value() / 100,
                                  priest().gains.hallucinations_vampiric_embrace, nullptr );
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

    if ( priest().conduits.haunting_apparitions->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.haunting_apparitions.percent() );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().talents.shadow.auspicious_spirits.enabled() )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_auspicious_spirits, s->action );
    }

    if ( priest().talents.shadow.idol_of_yoggsaron.enabled() )
    {
      priest().buffs.idol_of_yoggsaron->trigger();
    }
  }
};

struct shadowy_apparition_spell_t final : public priest_spell_t
{
  shadowy_apparition_spell_t( priest_t& p )
    : priest_spell_t( "shadowy_apparitions", p, p.talents.shadow.shadowy_apparitions )
  {
    background   = true;
    proc         = false;
    may_miss     = false;
    may_crit     = false;
    trigger_gcd  = timespan_t::zero();
    travel_speed = 6.0;

    impact_action = new shadowy_apparition_damage_t( p );

    add_child( impact_action );
  }

  /** Trigger a shadowy apparition */
  void trigger( player_t* target )
  {
    player->sim->print_debug( "{} triggered shadowy apparition on target {}.", priest(), *target );

    priest().procs.shadowy_apparition->occur();
    set_target( target );
    execute();
  }
};

// ==========================================================================
// Shadow Word: Pain
// ==========================================================================
struct shadow_word_pain_t final : public priest_spell_t
{
  bool casted;

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

    auto rank2 = p.find_rank_spell( "Shadow Word: Pain", "Rank 2" );
    if ( rank2->ok() )
    {
      dot_duration += rank2->effectN( 1 ).time_value();
    }

    if ( priest().talents.shadow.misery.enabled() )
    {
      dot_duration += priest().talents.shadow.misery->effectN( 2 ).time_value();
    }

    if ( priest().talents.throes_of_pain.enabled() )
    {
      base_td_multiplier *= ( 1 + priest().talents.throes_of_pain->effectN( 1 ).percent() );
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

  void trigger_heal()
  {
    // Use a simple option to dictate how many "allies" this will heal. All healing will go to the actor
    double amount_to_heal = priest().options.cauterizing_shadows_allies * priest().intellect() *
                            priest().specs.cauterizing_shadows_health->effectN( 1 ).sp_coeff();
    priest().resource_gain( RESOURCE_HEALTH, amount_to_heal, priest().gains.cauterizing_shadows_health, this );
  }

  void last_tick( dot_t* d ) override
  {
    // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/789
    // You only get the heal when the DoT expires naturally, not when a mob dies or you refresh it
    if ( priest().legendary.cauterizing_shadows->ok() )
    {
      trigger_heal();
    }

    priest_spell_t::last_tick( d );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( priest().buffs.fae_guardians->check() )
      {
        priest_td_t& td = get_td( s->target );

        if ( !td.buffs.wrathful_faerie->up() )
        {
          // There can only be one of these out at once so clear it first
          priest().remove_wrathful_faerie();
          td.buffs.wrathful_faerie->trigger();
        }
      }

      priest().refresh_insidious_ire_buff( s );
    }
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double crit = priest_spell_t::composite_target_crit_chance( target );

    // TODO: convert to new monomania
    // if ( priest().talents.shadow.abyssal_knowledge.enabled() && priest().is_monomania_up( target ) )
    // {
    //   crit += priest().talents.shadow.abyssal_knowledge->effectN( 1 ).percent();
    // }

    return crit;
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t t = priest_spell_t::tick_time( state );

    if ( priest().is_monomania_up( state->target ) )
    {
      t /= ( 1 + priest().talents.shadow.monomania_tickrate->effectN( 1 ).percent() );
    }

    return t;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest().specialization() != PRIEST_DISCIPLINE )
      return;

    if ( d->state->result_amount > 0 )
    {
      trigger_power_of_the_dark_side();
      priest().trigger_idol_of_nzoth( d->state->target );
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

    vampiric_touch_heal_t( priest_t& p )
      : priest_heal_t( "vampiric_touch_heal", p, p.dot_spells.vampiric_touch ),
        mental_fortitude( new mental_fortitude_t( p ) )
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

      // TODO: determine why we need to multiply by rank
      mental_fortitude_percentage = priest().talents.shadow.mental_fortitude->effectN( 1 ).percent() *
                                    priest().talents.shadow.mental_fortitude.rank();
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
  };

  propagate_const<vampiric_touch_heal_t*> vampiric_touch_heal;
  propagate_const<shadow_word_pain_t*> child_swp;
  propagate_const<unfurling_darkness_t*> child_ud;
  bool casted;

  vampiric_touch_t( priest_t& p, bool _casted = false )
    : priest_spell_t( "vampiric_touch", p, p.dot_spells.vampiric_touch ),
      vampiric_touch_heal( new vampiric_touch_heal_t( p ) ),
      child_swp( nullptr ),
      child_ud( nullptr )
  {
    casted                     = _casted;
    may_crit                   = false;
    affected_by_shadow_weaving = true;

    // Disable initial hit damage, only Unfurling Darkness uses it
    base_dd_min = base_dd_max = spell_power_mod.direct = 0;

    if ( priest().talents.shadow.misery.enabled() && casted )
    {
      child_swp             = new shadow_word_pain_t( priest(), false );
      child_swp->background = true;
    }

    if ( priest().talents.shadow.unfurling_darkness.enabled() )
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
    if ( priest().buffs.unfurling_darkness->check() )
    {
      child_ud->target = s->target;
      child_ud->execute();
      priest().buffs.unfurling_darkness->expire();
    }
    else
    {
      if ( priest().talents.shadow.unfurling_darkness.enabled() && !priest().buffs.unfurling_darkness_cd->check() )
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

  double composite_target_crit_chance( player_t* target ) const override
  {
    double crit = priest_spell_t::composite_target_crit_chance( target );

    // TODO: convert this to new monomania
    // if ( priest().talents.shadow.abyssal_knowledge.enabled() && priest().is_monomania_up( target ) )
    // {
    //   crit += priest().talents.shadow.abyssal_knowledge->effectN( 1 ).percent();
    // }

    return crit;
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t t = priest_spell_t::tick_time( state );

    if ( priest().is_monomania_up( state->target ) )
    {
      t /= ( 1 + priest().talents.shadow.monomania_tickrate->effectN( 1 ).percent() );
    }

    return t;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      int stack = priest().buffs.vampiric_insight->check();
      if ( priest().buffs.vampiric_insight->trigger() )
      {
        if ( priest().buffs.vampiric_insight->check() == stack )
        {
          priest().procs.vampiric_insight_overflow->occur();
        }
        else
        {
          priest().procs.vampiric_insight->occur();
        }
      }

      priest().trigger_idol_of_nzoth( d->state->target );
      vampiric_touch_heal->trigger( d->state->result_amount );
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
    }

    void trigger( double original_amount )
    {
      base_dd_min = base_dd_max = original_amount * data().effectN( 2 ).m_value();
      execute();
    }
  };

  propagate_const<devouring_plague_heal_t*> devouring_plague_heal;
  bool casted;

  devouring_plague_t( priest_t& p, bool _casted = false )
    : priest_spell_t( "devouring_plague", p, p.dot_spells.devouring_plague ),
      devouring_plague_heal( new devouring_plague_heal_t( p ) )
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

  void consume_resource() override
  {
    priest_spell_t::consume_resource();

    if ( priest().buffs.mind_devourer->up() && casted )
    {
      priest().buffs.mind_devourer->decrement();
    }
  }

  double cost() const override
  {
    if ( priest().buffs.mind_devourer->check() || !casted )
    {
      return 0;
    }

    return priest_spell_t::cost();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // Damnation does not trigger a SA - 2020-08-08
    if ( casted )
    {
      priest().trigger_shadowy_apparitions( s );
    }

    if ( result_is_hit( s->result ) )
    {
      devouring_plague_heal->trigger( s->result_amount );

      priest().refresh_insidious_ire_buff( s );
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      devouring_plague_heal->trigger( d->state->result_amount );
      priest().trigger_idol_of_nzoth( d->state->target );
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T28, B2 ) &&
         rng().roll( priest().sets->set( PRIEST_SHADOW, T28, B2 )->effectN( 1 ).percent() ) )
    {
      priest().buffs.dark_thought->trigger();
      priest().procs.dark_thoughts_devouring_plague->occur();
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
  propagate_const<cooldown_t*> shadowfiend_cooldown;
  propagate_const<cooldown_t*> mindbender_cooldown;
  timespan_t hungering_void_base_duration;
  timespan_t hungering_void_crit_duration;

  void_bolt_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "void_bolt", p, p.specs.void_bolt ),
      void_bolt_extension( nullptr ),
      shadowfiend_cooldown( p.get_cooldown( "mindbender" ) ),
      mindbender_cooldown( p.get_cooldown( "shadowfiend" ) ),
      hungering_void_base_duration(
          timespan_t::from_seconds( priest().talents.shadow.hungering_void->effectN( 3 ).base_value() ) ),
      hungering_void_crit_duration(
          timespan_t::from_seconds( priest().talents.shadow.hungering_void->effectN( 4 ).base_value() ) )
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

    if ( priest().conduits.dissonant_echoes->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.dissonant_echoes->effectN( 2 ).percent() );
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().buffs.dissonant_echoes->check() )
    {
      priest().buffs.dissonant_echoes->expire();
    }

    // TODO: Determine how this stacks in Dissonant Echoes in pre-patch
    if ( priest().buffs.void_touched->check() )
    {
      priest().buffs.void_touched->expire();
    }

    if ( priest().conduits.dissonant_echoes->ok() && priest().buffs.voidform->check() )
    {
      if ( rng().roll( priest().conduits.dissonant_echoes.percent() ) )
      {
        priest().cooldowns.void_bolt->reset( true );
        priest().procs.dissonant_echoes->occur();
      }
    }
  }

  bool ready() override
  {
    if ( !priest().buffs.voidform->check() && !priest().buffs.dissonant_echoes->check() &&
         !priest().buffs.void_touched->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest().trigger_shadowy_apparitions( s );

    if ( void_bolt_extension )
    {
      void_bolt_extension->target = s->target;
      void_bolt_extension->schedule_execute();
    }

    if ( priest().talents.shadow.hungering_void.enabled() )
    {
      priest_td_t& td = get_td( s->target );
      // Check if this buff is active, every Void Bolt after the first should get this
      // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/678
      // The first Void Bolt on a target will extend Voidform, even if Hungering Void is not active on the target
      if ( ( td.buffs.hungering_void->up() || priest().bugs ) && priest().buffs.voidform->check() )
      {
        timespan_t seconds_to_add_to_voidform =
            s->result == RESULT_CRIT ? hungering_void_crit_duration : hungering_void_base_duration;
        sim->print_debug( "{} extending Voidform duration by {} seconds.", priest(), seconds_to_add_to_voidform );
        // TODO: add some type of tracking for this increase
        priest().buffs.voidform->extend_duration( player, seconds_to_add_to_voidform );
      }
      priest().remove_hungering_void( s->target );
      td.buffs.hungering_void->trigger();
    }
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
// TODO: adjust targeting logic to be more accurate
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

    may_miss = false;
    radius   = data().effectN( 1 ).radius_max();

    if ( !priest().bugs )
    {
      // BUG: only hitting 4 targets where it should be 8 targets
      // 8 targets is found in spelldata though
      // https://github.com/SimCMinMax/WoW-BugTracker/issues/930
      aoe = data().effectN( 2 ).base_value();
      // BUG: Currently does not scale with Mastery
      // https://github.com/SimCMinMax/WoW-BugTracker/issues/931
      affected_by_shadow_weaving = true;
    }
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
// Surrender to Madness
// ==========================================================================
struct void_eruption_stm_damage_t final : public priest_spell_t
{
  propagate_const<action_t*> void_bolt;

  void_eruption_stm_damage_t( priest_t& p )
    : priest_spell_t( "void_eruption_stm_damage", p, p.talents.shadow.void_eruption_damage ), void_bolt( nullptr )
  {
    // This Void Eruption only hits a single target
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
  }
};

struct surrender_to_madness_t final : public priest_spell_t
{
  surrender_to_madness_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "surrender_to_madness", p, p.talents.shadow.surrender_to_madness )
  {
    parse_options( options_str );

    impact_action = new void_eruption_stm_damage_t( p );
    add_child( impact_action );
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.surrender_to_madness->trigger();

    if ( !priest().buffs.voidform->check() )
    {
      priest().buffs.voidform->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest_td_t& td = get_td( s->target );
    td.buffs.surrender_to_madness_debuff->trigger();
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
// Eternal Call to the Void (Shadowlands Legendary)
// ==========================================================================
struct eternal_call_to_the_void_t final : public priest_spell_t
{
  eternal_call_to_the_void_t( priest_t& p )
    : priest_spell_t( "eternal_call_to_the_void", p, p.find_spell( p.legendary.eternal_call_to_the_void->id() ) )
  {
    background = true;
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
struct psychic_link_t final : public priest_spell_t
{
  psychic_link_t( priest_t& p ) : priest_spell_t( "psychic_link", p, p.talents.shadow.psychic_link )
  {
    background = true;
    may_crit   = false;
    may_miss   = false;
    radius     = data().effectN( 1 ).radius_max();
  }

  void trigger( player_t* target, double original_amount )
  {
    base_dd_min = base_dd_max = ( original_amount * data().effectN( 1 ).percent() );
    player->sim->print_debug( "{} triggered psychic link on target {}.", priest(), *target );

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

struct shadow_crash_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_crash_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "shadow_crash", p, p.talents.shadow.shadow_crash ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) )
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
};

// ==========================================================================
// Damnation
// ==========================================================================
struct damnation_t final : public priest_spell_t
{
  propagate_const<shadow_word_pain_t*> child_swp;
  propagate_const<vampiric_touch_t*> child_vt;
  propagate_const<devouring_plague_t*> child_dp;

  damnation_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "damnation", p, p.talents.shadow.damnation ),
      child_swp( new shadow_word_pain_t( priest(), true ) ),  // Damnation still triggers SW:P as if it was hard casted
      child_vt( new vampiric_touch_t( priest(), false ) ),
      child_dp( new devouring_plague_t( priest(), false ) )
  {
    parse_options( options_str );
    child_swp->background = true;
    child_vt->background  = true;
    child_dp->background  = true;

    may_miss = false;

    if ( p.talents.shadow.malediction.enabled() )
    {
      cooldown->duration += p.talents.shadow.malediction->effectN( 1 ).time_value();
    }
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

    // Using Surrender within Voidform does not reset the duration - might be a bug?
    set_refresh_behavior( buff_refresh_behavior::DISABLED );

    // Use a stack change callback to trigger voidform effects.
    set_stack_change_callback( [ this ]( buff_t*, int, int cur ) {
      if ( cur )
      {
        priest().cooldowns.mind_blast->adjust_max_charges( 1 );
        priest().cooldowns.mind_blast->reset( true, -1 );
        priest().cooldowns.void_bolt->reset( true );
      }
      else
      {
        priest().cooldowns.mind_blast->adjust_max_charges( -1 );
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
// Dark Thoughts
// ==========================================================================
struct dark_thought_t final : public priest_buff_t<buff_t>
{
  dark_thought_t( priest_t& p ) : base_t( p, "dark_thought", p.specs.dark_thought )
  {
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
        priest().procs.dark_thoughts_missed->occur();
      }
    }

    base_t::expire_override( expiration_stacks, remaining_duration );
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
// Vampiric Insight
// ==========================================================================
struct vampiric_insight_t final : public priest_buff_t<buff_t>
{
  vampiric_insight_t( priest_t& p ) : base_t( p, "vampiric_insight", p.find_spell( 375981 ) )
  {
    // TODO: determine what rppm value actually is, mostly guesses right now
    // These values are not found in spell data
    set_rppm( RPPM_HASTE, 3.0 );
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
        priest().procs.vampiric_insight_missed->occur();
      }
    }

    base_t::expire_override( expiration_stacks, remaining_duration );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( !priest().talents.shadow.vampiric_insight.enabled() )
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

    set_duration( p.specs.voidform->duration() );        // Uses the same duration as Voidform for tooltip
    set_default_value( data().effectN( 2 ).percent() );  // Each stack is worth 2% from effect 2
    set_max_stack( as<int>( data().effectN( 1 ).base_value() ) /
                   as<int>( data().effectN( 2 ).base_value() ) );  // Set max stacks to 30 / 2
    set_reverse( true );
    set_period( timespan_t::from_seconds( 1 ) );
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
    double amount                           = num_amount;
    double amount_from_surrender_to_madness = 0.0;

    if ( buffs.surrender_to_madness->check() )
    {
      double total_amount = amount * ( 1.0 + talents.shadow.surrender_to_madness->effectN( 2 ).percent() );

      amount_from_surrender_to_madness = amount * talents.shadow.surrender_to_madness->effectN( 2 ).percent();

      // Make sure the maths line up.
      assert( total_amount == amount + amount_from_surrender_to_madness );
    }

    resource_gain( RESOURCE_INSANITY, amount, g, action );

    if ( amount_from_surrender_to_madness > 0.0 )
    {
      resource_gain( RESOURCE_INSANITY, amount_from_surrender_to_madness, gains.insanity_surrender_to_madness, action );
    }
  }
}

void priest_t::create_buffs_shadow()
{
  // Baseline
  buffs.dark_thought     = make_buff<buffs::dark_thought_t>( *this );
  buffs.shadowform       = make_buff<buffs::shadowform_t>( *this );
  buffs.shadowform_state = make_buff<buffs::shadowform_state_t>( *this );
  buffs.vampiric_embrace = make_buff( this, "vampiric_embrace", talents.vampiric_embrace );
  buffs.voidform         = make_buff<buffs::voidform_t>( *this );
  buffs.dispersion       = make_buff<buffs::dispersion_t>( *this );

  // Talents
  buffs.ancient_madness        = make_buff<buffs::ancient_madness_t>( *this );
  buffs.death_and_madness_buff = make_buff<buffs::death_and_madness_buff_t>( *this );
  buffs.surrender_to_madness   = make_buff( this, "surrender_to_madness", talents.shadow.surrender_to_madness );
  buffs.surrender_to_madness_death =
      make_buff( this, "surrender_to_madness_death", talents.shadow.surrender_to_madness )
          ->set_duration( timespan_t::zero() )
          ->set_default_value( 0.0 )
          ->set_chance( 1.0 );
  buffs.unfurling_darkness =
      make_buff( this, "unfurling_darkness", talents.shadow.unfurling_darkness->effectN( 1 ).trigger() );
  buffs.unfurling_darkness_cd =
      make_buff( this, "unfurling_darkness_cd",
                 talents.shadow.unfurling_darkness->effectN( 1 ).trigger()->effectN( 2 ).trigger() );
  buffs.void_torrent = make_buff( this, "void_torrent", talents.shadow.void_torrent );

  // TODO: Check Buff ID(s) for Mind Devourer
  if ( talents.shadow.mind_devourer.enabled() )
  {
    buffs.mind_devourer = make_buff( this, "mind_devourer", find_spell( 373204 ) )
                              ->set_trigger_spell( talents.shadow.mind_devourer )
                              ->set_chance( talents.shadow.mind_devourer->effectN( 2 ).percent() );
  }
  else
  {
    buffs.mind_devourer = make_buff( this, "mind_devourer", find_spell( 373204 ) )
                              ->set_trigger_spell( conduits.mind_devourer )
                              ->set_chance( conduits.mind_devourer->effectN( 2 ).percent() );
  }

  buffs.vampiric_insight = make_buff<buffs::vampiric_insight_t>( *this );

  buffs.void_touched = make_buff( this, "void_touched", find_spell( 373375 ) );

  buffs.mental_fortitude = new buffs::mental_fortitude_buff_t( this );

  buffs.insidious_ire = make_buff( this, "insidious_ire", talents.shadow.insidious_ire )
                            ->set_duration( timespan_t::zero() )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  buffs.thing_from_beyond = make_buff( this, "thing_from_beyond", find_spell( 373277 ) );

  buffs.idol_of_yoggsaron = make_buff( this, "idol_of_yogg-saron", talents.shadow.idol_of_yoggsaron )
                                ->set_duration( timespan_t::zero() )
                                ->set_max_stack( 50 )
                                ->set_stack_change_callback( ( [ this ]( buff_t* b, int, int cur ) {
                                  if ( cur == b->max_stack() )
                                  {
                                    b->expire();
                                    procs.thing_from_beyond->occur();
                                    spawn_thing_from_beyond();
                                  }
                                } ) );

  // TODO: Get real damage amplifier and spell data when blizzard implements this.
  buffs.yshaarj_pride =
      make_buff( this, "yshaarj_pride" )->set_duration( timespan_t::zero() )->set_default_value( 0.1 );

  // Conduits (Shadowlands)
  buffs.dissonant_echoes = make_buff( this, "dissonant_echoes", find_spell( 343144 ) );

  buffs.talbadars_stratagem = make_buff( this, "talbadars_stratagem", find_spell( 342415 ) )
                                  ->set_duration( timespan_t::zero() )
                                  ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  // Tier Sets
  buffs.living_shadow_tier =
      make_buff( this, "living_shadow_tier", find_spell( 363574 ) )->set_duration( timespan_t::zero() );
}

void priest_t::init_rng_shadow()
{
  rppm.eternal_call_to_the_void = get_rppm( "eternal_call_to_the_void", legendary.eternal_call_to_the_void );
  rppm.idol_of_cthun            = get_rppm( "idol_of_cthun", talents.shadow.idol_of_cthun );
}

void priest_t::init_spells_shadow()
{
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };

  // Row 2
  talents.shadow.silence             = ST( "Silence" );
  talents.shadow.shadowy_apparition  = find_spell( 148859 );
  talents.shadow.shadowy_apparitions = ST( "Shadowy Apparitions" );
  talents.shadow.mind_sear           = ST( "Mind Sear" );     // NYI
  talents.shadow.mind_sear_insanity  = find_spell( 208232 );  // TODO: might not need. Insanity is stored here
  // Row 3
  talents.shadow.psychic_horror     = ST( "Psychic Horror" );
  talents.shadow.last_word          = ST( "Last Word" );
  talents.shadow.misery             = ST( "Misery" );
  talents.shadow.dark_void          = ST( "Dark Void" );
  talents.shadow.dark_void_insanity = find_spell( 391450 );  // Not linked to Dark Void except in tooltip
  talents.shadow.auspicious_spirits = ST( "Auspicious Spirits" );
  talents.shadow.tormented_spirits  = ST( "Tormented Spirits" );  // NYI
  talents.shadow.dispersion         = ST( "Dispersion" );
  // Row 4
  talents.shadow.shadow_orbs      = ST( "Shadow Orbs" );  // NYI
  talents.shadow.hallucinations   = ST( "Hallucinations" );
  talents.shadow.tithe_evasion    = ST( "Tithe Evasion" );     // NYI
  talents.shadow.mind_spike       = ST( "Mind Spike" );        // NYI
  talents.shadow.vampiric_insight = ST( "Vampiric Insight" );  // TODO: remove mind blast charge
  talents.shadow.intangibility    = ST( "Intangibility" );
  talents.shadow.mental_fortitude = ST( "Mental Fortitude" );  // NYI
  // Row 5
  talents.shadow.puppet_master     = ST( "Puppet Master" );  // NYI
  talents.shadow.damnation         = ST( "Damnation" );
  talents.shadow.mind_melt         = ST( "Mind Melt" );          // NYI
  talents.shadow.surge_of_darkness = ST( "Surge of Darkness" );  // NYI
  talents.shadow.mental_decay      = ST( "Mental Decay" );       // NYI
  talents.shadow.dark_evangelism   = ST( "Dark Evangelism" );    // NYI
  // Row 6
  talents.shadow.harnessed_shadows  = ST( "Harnessed Shadows" );  // NYI
  talents.shadow.malediction        = ST( "Malediction" );
  talents.shadow.psychic_link       = ST( "Psychic Link" );  // Add in Mind Spike and confirm values
  talents.shadow.void_torrent       = ST( "Void Torrent" );
  talents.shadow.shadow_crash       = ST( "Shadow Crash" );    // TODO implement VT
  talents.shadow.dark_ascension     = ST( "Dark Ascension" );  // NYI
  talents.shadow.unfurling_darkness = ST( "Unfurling Darkness" );
  // Row 7
  talents.shadow.maddening_touch        = ST( "Maddening Touch" );         // NYI
  talents.shadow.whispers_of_the_damned = ST( "Whispers of the Damned" );  // NYI
  talents.shadow.piercing_shadows       = ST( "Piercing Shadows" );        // NYI
  // Row 8
  talents.shadow.mindbender           = ST( "Mindbender" );
  talents.shadow.idol_of_yshaarj      = ST( "Idol of Y'Shaarj" );
  talents.shadow.pain_of_death        = ST( "Pain of Death" );        // NYI
  talents.shadow.mind_flay_insanity   = ST( "Mind Flay: Insanity" );  // NYI
  talents.shadow.derangement          = ST( "Derangement" );          // NYI
  talents.shadow.void_eruption        = ST( "Void Eruption" );        // TODO: confirm CD is 2m
  talents.shadow.void_eruption_damage = find_spell( 228360 );
  // Row 9
  talents.shadow.fiending_dark      = ST( "Fiending Dark" );      // NYI
  talents.shadow.monomania          = ST( "Monomania" );          // NYI
  talents.shadow.monomania_tickrate = find_spell( 375408 );       // TODO: confirm we still need this
  talents.shadow.painbreaker_psalm  = ST( "Painbreaker Psalm" );  // NYI
  talents.shadow.mastermind         = ST( "Mastermind" );         // NYI
  talents.shadow.insidious_ire      = ST( "Insidious Ire" );      // TODO: check values
  talents.shadow.mind_devourer      = ST( "Mind Devourer" );      // TODO: check values
  talents.shadow.ancient_madness    = ST( "Ancient Madness" );    // TODO: add point scaling
  // Row 10
  talents.shadow.shadowflame_prism    = ST( "Shadowflame Prism" );
  talents.shadow.idol_of_cthun        = ST( "Idol of C'Thun" );
  talents.shadow.idol_of_yoggsaron    = ST( "Idol of Yogg-Saron" );
  talents.shadow.idol_of_nzoth        = ST( "Idol of N'Zoth" );
  talents.shadow.lunacy               = ST( "Lunacy" );          // NYI
  talents.shadow.hungering_void       = ST( "Hungering Void" );  // Check values
  talents.shadow.hungering_void_buff  = find_spell( 345219 );
  talents.shadow.surrender_to_madness = ST( "Surrender to Madness" );  // Confirm 2m cd is working

  // General Spells
  specs.mind_flay  = find_specialization_spell( "Mind Flay" );
  specs.shadowform = find_specialization_spell( "Shadowform" );
  specs.void_bolt  = find_spell( 205448 );
  specs.voidform   = find_spell( 194249 );

  // Still need these for pre-patch since 2p/4p still works with DT and not VI
  specs.dark_thought  = find_specialization_spell( "Dark Thought" );
  specs.dark_thoughts = find_specialization_spell( "Dark Thoughts" );

  // Legendary Effects
  specs.cauterizing_shadows_health = find_spell( 336373 );
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
  if ( name == "surrender_to_madness" )
  {
    return new surrender_to_madness_t( *this, options_str );
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

  // TODO: does this stack in pre-patch?
  if ( legendary.eternal_call_to_the_void->ok() )
  {
    background_actions.eternal_call_to_the_void = new actions::spells::eternal_call_to_the_void_t( *this );
  }

  if ( talents.shadow.idol_of_cthun.enabled() )
  {
    background_actions.idol_of_cthun = new actions::spells::idol_of_cthun_t( *this );
  }

  background_actions.shadow_weaving = new actions::spells::shadow_weaving_t( *this );

  background_actions.shadow_word_pain = new actions::spells::shadow_word_pain_t( *this );
}

// ==========================================================================
// Trigger Shadowy Apparitions on all targets affected by vampiric touch
// ==========================================================================
void priest_t::trigger_shadowy_apparitions( action_state_t* s )
{
  if ( !talents.shadow.shadowy_apparitions.enabled() )
  {
    return;
  }
  // TODO: check if this procs non non-hits
  int number_of_apparitions_to_trigger = s->result == RESULT_CRIT ? 2 : 1;

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && priest_td->dots.vampiric_touch->is_ticking() )
    {
      for ( int i = 0; i < number_of_apparitions_to_trigger; ++i )
      {
        background_actions.shadowy_apparitions->trigger( priest_td->target );
      }
    }
  }
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
      background_actions.psychic_link->trigger( priest_td->target, s->result_amount );
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

// ==========================================================================
// Check for the Hungering Void talent and find the debuff on that target
// ==========================================================================
bool priest_t::hungering_void_active( player_t* target ) const
{
  if ( !talents.shadow.hungering_void.enabled() )
    return false;
  const priest_td_t* td = find_target_data( target );
  if ( !td )
    return false;

  return td->buffs.hungering_void->check();
}

// ==========================================================================
// Remove Hungering Void on any targets that don't match
// ==========================================================================
void priest_t::remove_hungering_void( player_t* target )
{
  if ( !talents.shadow.hungering_void.enabled() )
    return;

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && ( priest_td->target != target ) && priest_td->buffs.hungering_void->check() )
    {
      priest_td->buffs.hungering_void->expire();
    }
  }
}

bool priest_t::is_monomania_up( player_t* target ) const
{
  if ( talents.shadow.monomania.enabled() )
  {
    priest_td_t* td = get_target_data( target );
    if ( td->dots.mind_flay->is_ticking() || td->dots.mind_sear->is_ticking() )
    {
      return true;
    }
  }
  return false;
}

// Helper function to refresh talbadars buff
void priest_t::refresh_insidious_ire_buff( action_state_t* s )
{
  if ( !legendary.talbadars_stratagem->ok() && !talents.shadow.insidious_ire.enabled() )
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
      if ( legendary.talbadars_stratagem->ok() )
        buffs.talbadars_stratagem->trigger( min_length );
      if ( talents.shadow.insidious_ire.enabled() )
        buffs.insidious_ire->trigger( min_length );
    }
  }
}

void priest_t::trigger_idol_of_nzoth( player_t* target )
{
  if ( !talents.shadow.idol_of_nzoth.enabled() )
    return;

  if ( rng().roll( talents.shadow.idol_of_nzoth->effectN( 1 ).percent() ) )
  {
    auto td = get_target_data( target );
    td->buffs.echoing_void->trigger();
    if ( !td->buffs.echoing_void_collapse->check() && rng().roll( talents.shadow.idol_of_nzoth->proc_chance() ) )
    {
      td->buffs.echoing_void_collapse->trigger();
    }
  }
}

void priest_t::spawn_thing_from_beyond()
{
  pets.thing_from_beyond.spawn();
}

}  // namespace priestspace

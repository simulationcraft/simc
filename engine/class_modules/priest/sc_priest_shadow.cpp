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
namespace
{
enum class random_idol_e : int
{
  NONE = 0,
  YSHAARJ,
  NZOTH_HORRIFIC_VISION,
  NZOTH_VISION_OF_NZOTH,
  YOGG,
  CTHUN
};
}  // namespace

namespace actions
{
namespace spells
{
// ==========================================================================
// Mind Flay
// ==========================================================================
struct mind_flay_base_t : public priest_spell_t
{
  mind_flay_base_t( util::string_view n, priest_t& p, const spell_data_t* s ) : priest_spell_t( n, p, s )
  {
    affected_by_shadow_weaving = true;
    may_crit                   = false;
    channeled                  = true;
    idol_of_nzoth_tick_stacks  = 1;
  }

  bool insidious_ire_active() const
  {
    if ( !priest().talents.shadow.insidious_ire.enabled() )
      return false;

    return priest().buffs.insidious_ire->check();
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_ta_multiplier( s );

    if ( insidious_ire_active() )
    {
      m *= 1 + priest().talents.shadow.insidious_ire->effectN( 1 ).percent();
    }

    return m;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    priest().trigger_idol_of_cthun( d->state );

    if ( priest().talents.shadow.psychic_link.enabled() )
    {
      priest().trigger_psychic_link( d->state );
    }

    if ( priest().talents.shadow.shattered_psyche.enabled() )
    {
      priest().buffs.shattered_psyche->trigger();
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

struct mind_flay_insanity_t final : public mind_flay_base_t
{
  mind_flay_insanity_t( priest_t& p, util::string_view options_str )
    : mind_flay_base_t( "mind_flay_insanity", p, p.talents.archon.mind_flay_insanity_spell )
  {
    parse_options( options_str );

    // We spell queue out of MFI.
    ability_lag = p.options.no_channel_macro_mfi ? p.world_lag : p.sim->queue_lag;
  }

  void execute() override
  {
    mind_flay_base_t::execute();

    priest().buffs.mind_flay_insanity->decrement();

    // This rolls its own independent chance to crit for the Shadowy Apparition, since it happens on cast.
    // It is not related to the first tick of MF:I's state
    if ( priest().talents.archon.energy_cycle.enabled() )
    {
      priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_mfi );
    }
  }

  bool action_ready() override
  {
    if ( !priest().buffs.mind_flay_insanity->check() )
      return false;

    return mind_flay_base_t::action_ready();
  }
};

struct mind_flay_t final : public mind_flay_base_t
{
  mind_flay_t( priest_t& p, util::string_view options_str ) : mind_flay_base_t( "mind_flay", p, p.specs.mind_flay )
  {
    parse_options( options_str );
  }

  bool action_ready() override
  {
    if ( priest().buffs.mind_flay_insanity->check() )
      return false;

    return mind_flay_base_t::action_ready();
  }
};

// ==========================================================================
// Dispersion
// ==========================================================================
struct dispersion_t final : public priest_spell_t
{
  dispersion_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "dispersion", p, p.specs.dispersion )
  {
    parse_options( options_str );

    ignore_false_positive = true;
    channeled             = true;
    harmful               = false;
    tick_may_crit         = false;
    hasted_ticks          = false;
    may_miss              = false;
  }

  void execute() override
  {
    priest().buffs.dispersion->trigger();

    priest_spell_t::execute();
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );

    // reset() instead of expire() because it was not properly creating the buff every 2nd time
    priest().buffs.dispersion->reset();
  }
};

struct dispersion_heal_t final : public priest_heal_t
{
  dispersion_heal_t( priest_t& p ) : priest_heal_t( "dispersion_heal", p, p.specs.dispersion )
  {
    background = true;
    may_crit = may_miss = false;
    base_dd_multiplier  = 1.0;
    callbacks           = false;  // TODO: verify

    // turn off automatic HoT components
    dot_duration = timespan_t::from_seconds( 0 );
  }

  void trigger( double amount )
  {
    base_dd_min = base_dd_max = amount;
    execute();
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
    target  = player;
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
  silence_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "silence", p, p.specs.silence )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = is_interrupt = true;
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
    : priest_spell_t( "vampiric_embrace", p, p.specs.vampiric_embrace )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest().buffs.vampiric_embrace->trigger();
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
// Void Bolt
// ==========================================================================
struct void_bolt_t final : public priest_spell_t
{
  void_bolt_t( priest_t& p ) : priest_spell_t( "void_bolt", p, p.talents.shadow.void_bolt )
  {
    affected_by_shadow_weaving = true;
    track_cd_waste             = false;
    background                 = true;
  }
};

// ==========================================================================
// Shadowy Apparition
// ==========================================================================
struct shadowy_apparition_state_t : public action_state_t
{
  double number_spawned;

  shadowy_apparition_state_t( action_t* a, player_t* t ) : action_state_t( a, t ), number_spawned( 1.0 )
  {
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );
    fmt::print( s, " number_spawned={}", number_spawned );
    return s;
  }

  void initialize() override
  {
    action_state_t::initialize();
    number_spawned = 1.0;
  }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    auto other_sa_state = debug_cast<const shadowy_apparition_state_t*>( o );
    number_spawned      = other_sa_state->number_spawned;
  }
};

struct shadowy_apparition_base_t : public priest_spell_t
{
protected:
  using state_t = shadowy_apparition_state_t;

public:
  double insanity_gain;

  struct shadowy_apparition_damage_t final : public priest_spell_t
  {
    double insanity_gain;
    double mod;

    shadowy_apparition_damage_t( priest_t& p, std::string name, const spell_data_t* spell, double _mod )
      : priest_spell_t( name, p, spell ),
        insanity_gain( priest().talents.shadow.auspicious_spirits->effectN( 2 ).percent() )
    {
      affected_by_shadow_weaving = true;
      background                 = true;
      proc                       = false;
      callbacks                  = true;
      may_miss                   = false;
      may_crit                   = true;
      mod                        = _mod;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = priest_spell_t::composite_target_multiplier( t );

      if ( priest().talents.shadow.spectral_horrors.enabled() )
      {
        const priest_td_t* td = priest().find_target_data( t );
        if ( td->dots.shadow_word_madness->is_ticking() )
          m *= 1 + priest().talents.shadow.spectral_horrors->effectN( 1 ).percent();
      }

      // Apply mod for Void Apparition
      m *= mod;

      return m;
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

    void impact( action_state_t* s ) override
    {
      priest_spell_t::impact( s );

      if ( priest().talents.shadow.auspicious_spirits.enabled() )
      {
        if ( priest().threshold_rng.auspicious_spirits->trigger( s ) )
        {
          priest().generate_insanity( insanity_gain, priest().gains.insanity_auspicious_spirits, s->action );
        }
      }

      if ( priest().talents.shadow.haunting_shadows.enabled() )
      {
        timespan_t dot_extension =
            timespan_t::from_millis( priest().talents.shadow.haunting_shadows->effectN( 2 ).base_value() );
        priest_td_t& td = get_td( s->target );

        td.dots.shadow_word_pain->adjust_duration( dot_extension );
        td.dots.vampiric_touch->adjust_duration( dot_extension );
      }
    }
  };

  shadowy_apparition_base_t( priest_t& p, std::string name, const spell_data_t* parent, const spell_data_t* damage,
                             double mod = 1.0 )
    : priest_spell_t( name, p, parent ),
      insanity_gain( priest().talents.shadow.auspicious_spirits->effectN( 2 ).percent() )
  {
    background  = true;
    proc        = false;
    may_miss    = false;
    may_crit    = false;
    trigger_gcd = timespan_t::zero();

    // This is the same for all apparitions
    travel_speed = priest().talents.shadow.shadowy_apparition->missile_speed();

    impact_action = new shadowy_apparition_damage_t( p, name.erase( name.size() - 1 ), damage, mod );

    add_child( impact_action );
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

  void impact( action_state_t* s ) override
  {
    impact_action->set_target( s->target );
    auto state = impact_action->get_state( s );
    impact_action->snapshot_state( state, impact_action->amount_type( state ) );
    impact_action->schedule_execute( state );
  }

  /** Trigger a shadowy apparition */
  void trigger( player_t* target, proc_t* proc, int vts )
  {
    player->sim->print_debug( "{} triggered shadowy apparition on target {} from {}. vts_active={}", priest(), *target,
                              proc->name(), vts );

    state_t* s = cast_state( get_state() );

    set_target( target );

    s->target         = target;
    s->number_spawned = vts;

    snapshot_state( s, amount_type( s ) );

    proc->occur();

    schedule_execute( s );
  }
};

struct shadowy_apparition_spell_t final : public shadowy_apparition_base_t
{
  shadowy_apparition_spell_t( priest_t& p )
    : shadowy_apparition_base_t( p, "shadowy_apparitions", p.talents.shadow.shadowy_apparitions,
                                 p.talents.shadow.shadowy_apparition->effectN( 1 ).trigger() )
  {
  }
};

struct void_apparition_spell_t final : public shadowy_apparition_base_t
{
  void_apparition_spell_t( priest_t& p )
    : shadowy_apparition_base_t( p, "void_apparitions", p.talents.shadow.void_apparitions_2,
                                 p.talents.shadow.void_apparition->effectN( 1 ).trigger(),
                                 p.talents.shadow.void_apparitions_2->effectN( 2 ).percent() + 1.0 )
  {
  }

  void execute() override
  {
    priest().background_actions.void_bolt->execute();

    shadowy_apparition_base_t::execute();
  }
};

// ==========================================================================
// Shadow Word: Pain
// ==========================================================================
struct shadow_word_pain_t final : public priest_spell_t
{
  bool casted;
  propagate_const<action_t*> child_searing_light;

  shadow_word_pain_t( priest_t& p, bool _casted = false )
    : priest_spell_t( "shadow_word_pain", p, p.dot_spells.shadow_word_pain ), child_searing_light( nullptr )
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

    if ( casted )
    {
      idol_of_nzoth_execute_stacks = 3;
    }

    if ( priest().talents.holy.divine_image.enabled() )
    {
      child_searing_light = priest().background_actions.searing_light;
    }

    triggers_atonement = true;
  }

  shadow_word_pain_t( priest_t& p, util::string_view options_str ) : shadow_word_pain_t( p, true )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    if ( priest().specialization() == PRIEST_DISCIPLINE && priest().talents.discipline.purge_the_wicked.enabled() )
    {
      return false;
    }

    return priest_spell_t::ready();
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
      priest().refresh_insidious_ire_buff( s );

      if ( child_searing_light && priest().buffs.divine_image->up() )
      {
        for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
        {
          child_searing_light->execute();
        }
      }

      if ( s->result_amount > 0 && priest().talents.shadow.tormented_spirits.enabled() )
      {
        if ( priest().threshold_rng.tormented_spirits->trigger( s ) )
        {
          priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_swp );
        }
      }
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      trigger_power_of_the_dark_side();
      priest().trigger_shadowy_insight( false, d->state );

      if ( priest().talents.shadow.tormented_spirits.enabled() )
      {
        if ( priest().threshold_rng.tormented_spirits->trigger( d->state ) )
        {
          priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_swp );
        }
      }
    }
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

    double composite_da_multiplier( const action_state_t* ) const override
    {
      return 1.0;
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
  bool casted;
  bool insanity;

  vampiric_touch_t( priest_t& p, bool _casted = false, bool _insanity = true )
    : priest_spell_t( "vampiric_touch", p, p.dot_spells.vampiric_touch ),
      vampiric_touch_heal( new vampiric_touch_heal_t( p ) ),
      child_swp( nullptr )
  {
    casted                     = _casted;
    insanity                   = _insanity;
    may_crit                   = false;
    affected_by_shadow_weaving = true;

    // Disable initial hit damage, was used for Unfurling Darkness
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

    if ( casted )
    {
      idol_of_nzoth_execute_stacks = 4;
    }
  }

  vampiric_touch_t( priest_t& p, util::string_view options_str ) : vampiric_touch_t( p, true )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    if ( child_swp )
    {
      child_swp->target = s->target;
      child_swp->execute();
    }

    priest_spell_t::impact( s );

    priest().refresh_insidious_ire_buff( s );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      if ( priest().talents.shadow.maddening_touch.enabled() )
      {
        // No CD?
        // priest().cooldowns.maddening_touch_icd->up();

        if ( priest().threshold_rng.maddening_touch->trigger( d->state ) )
        {
          // priest().cooldowns.maddening_touch_icd->start();
          priest().generate_insanity(
              priest().talents.shadow.maddening_touch->effectN( 2 ).resource( RESOURCE_INSANITY ),
              priest().gains.insanity_maddening_touch, d->state->action );
        }
      }

      vampiric_touch_heal->trigger( d->state->result_amount );
    }
  }
};

// ==========================================================================
// Shadow Word: Madness
// ==========================================================================
struct shadow_word_madness_t final : public priest_spell_t
{
  struct shadow_word_madness_heal_t final : public priest_heal_t
  {
    mental_fortitude_t* mental_fortitude;
    double mental_fortitude_percentage;

    shadow_word_madness_heal_t( priest_t& p )
      : priest_heal_t( "shadow_word_madness_heal", p, p.dot_spells.shadow_word_madness )
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
      base_dd_min = base_dd_max = original_amount * data().effectN( 1 ).m_value();
      execute();
    }
  };

  bool casted;
  bool triggered_by_maddening_tentacles;
  propagate_const<shadow_word_madness_heal_t*> shadow_word_madness_heal;

  shadow_word_madness_t( priest_t& p, bool _casted = false, bool _triggered_by_maddening_tentacles = true )
    : priest_spell_t( "shadow_word_madness", p, p.dot_spells.shadow_word_madness ),
      shadow_word_madness_heal( new shadow_word_madness_heal_t( p ) )
  {
    casted                           = _casted;
    triggered_by_maddening_tentacles = _triggered_by_maddening_tentacles;
    may_crit                         = true;
    affected_by_shadow_weaving       = true;
    idol_of_nzoth_execute_stacks     = 12;
  }

  shadow_word_madness_t( priest_t& p, util::string_view options_str ) : shadow_word_madness_t( p, true, false )
  {
    parse_options( options_str );
  }

  void trigger_dot( action_state_t* s ) override
  {
    timespan_t duration = composite_dot_duration( s );
    if ( duration <= timespan_t::zero() )
      return;

    dot_t* dot = get_dot( s->target );

    dot->current_action = this;
    dot->max_stack      = dot_max_stack;

    if ( !dot->state )
      dot->state = get_state();

    // Combine persistent_multiplier on refresh for Shadow Word: Madness only.
    // When a partial/weak DoT (e.g., from Tentacle Slam / Maddening Tentacles)
    // refreshes an existing DoT, the effective persistent multiplier should be
    // the weighted combination of the old remaining ticks and the new base
    // ticks so total damage is preserved rather than overwritten or doubled.
    if ( dot->is_ticking() )
    {
      double old_persistent = dot->state->persistent_multiplier;
      double old_ticks_left = dot->ticks_left_fractional();

      // copy new state first to capture snapshot values from this cast
      dot->state->copy_state( s );

      // Compute weights in ticks against the refreshed total tick budget.
      // This mirrors rolling refresh semantics: old remaining value + new base
      // value, distributed over the refreshed DoT's total ticks.
      timespan_t new_tick     = tick_time( s );
      timespan_t new_duration = composite_dot_duration( s );
      double new_base_ticks   = ( new_tick > 0_ms ) ? ( new_duration / new_tick ) : 0.0;

      timespan_t refreshed_duration = calculate_dot_refresh_duration( dot, new_duration );
      double new_ticks_left =
          ( new_tick > 0_ms ) ? ( 1.0 + ( refreshed_duration - dot->time_to_next_full_tick() ) / new_tick ) : 0.0;

      if ( new_ticks_left > 0.0 )
      {
        double combined =
            ( old_ticks_left * old_persistent + new_base_ticks * s->persistent_multiplier ) / new_ticks_left;

        sim->print_debug(
            "shadow_word_madness refresh: old_persistent={} new_persistent={} old_ticks_left={} "
            "new_base_ticks={} new_ticks_left={} combined={}",
            old_persistent, s->persistent_multiplier, old_ticks_left, new_base_ticks, new_ticks_left,
            combined );
        dot->state->persistent_multiplier = combined;
      }
      else
      {
        sim->print_debug( "shadow_word_madness refresh: new_ticks_left<=0, preserving old_persistent={}",
                          old_persistent );
        dot->state->persistent_multiplier = old_persistent;
      }
    }
    else
    {
      dot->state->copy_state( s );
    }

    dot->trigger( duration );
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_persistent_multiplier( s );

    // Dummy effect that is hard-coded to 20
    if ( priest().buffs.mind_devourer->check() && casted && !triggered_by_maddening_tentacles )
    {
      m *= 1 + priest().buffs.mind_devourer->data().effectN( 2 ).percent();
    }

    if ( !casted && triggered_by_maddening_tentacles )
    {
      m *= priest().talents.shadow.maddening_tentacles->effectN( 1 ).percent();
    }

    return m;
  }

  void consume_resource() override
  {
    if ( casted )
    {
      priest_spell_t::consume_resource();

      if ( priest().buffs.mind_devourer->up() )
      {
        priest().buffs.mind_devourer->decrement();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_swm );

    if ( result_is_hit( s->result ) )
    {
      shadow_word_madness_heal->trigger( s->result_amount );
      priest().trigger_psychic_link( s );
      priest().refresh_insidious_ire_buff( s );
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      shadow_word_madness_heal->trigger( d->state->result_amount );
      priest().trigger_psychic_link( d->state );
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.shadow.ancient_madness.enabled() && priest().buffs.voidform->up() && casted )
    {
      timespan_t base_duration = timespan_t::from_millis( priest().talents.shadow.voidform->effectN( 2 ).base_value() );

      if ( priest().buffs.ancient_madness->check() )
      {
        // Ancient Madness diminishes by 25% per cast:
        double factor = std::pow( 1 - priest().talents.shadow.ancient_madness->effectN( 3 ).percent(),
                                  priest().buffs.ancient_madness->check() );

        timespan_t extension_ms = base_duration * factor;

        priest().buffs.voidform->extend_duration( extension_ms );
      }
      else
      {
        priest().buffs.voidform->extend_duration( base_duration );
      }

      priest().buffs.ancient_madness->trigger();
    }

    if ( priest().talents.shadow.screams_of_the_void.enabled() )
    {
      priest().buffs.screams_of_the_void->trigger();
    }

    if ( priest().talents.voidweaver.collapsing_void.enabled() && casted )
    {
      priest().expand_entropic_rift();
    }
  }
};

// ==========================================================================
// Void Volley
// missile - 1242173
// damage - 1242189
// ==========================================================================
struct void_volley_damage_t final : public priest_spell_t
{
  void_volley_damage_t( util::string_view n, priest_t& p, const spell_data_t* s ) : priest_spell_t( n, p, s )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
  }

  bool insidious_ire_active() const
  {
    if ( !priest().talents.shadow.insidious_ire.enabled() )
      return false;

    return priest().buffs.insidious_ire->check();
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_ta_multiplier( s );

    if ( insidious_ire_active() )
    {
      m *= 1 + priest().talents.shadow.insidious_ire->effectN( 1 ).percent();
    }

    return m;
  }
};

struct void_volley_damage_aoe_t final : public priest_spell_t
{
  void_volley_damage_aoe_t( util::string_view n, priest_t& p, const spell_data_t* s, double _radius )
    : priest_spell_t( n, p, s )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
    aoe                        = -1;
    radius                     = _radius;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    // base action_t::available_targets with main target removed.
    tl.clear();

    for ( auto* t : sim->target_non_sleeping_list )
    {
      if ( t->is_enemy() && ( t != target ) )
      {
        tl.push_back( t );
      }
    }

    if ( sim->debug && !sim->distance_targeting_enabled )
    {
      sim->print_debug( "{} regenerated target cache for {} ({})", *player, signature_str, *this );
      for ( size_t i = 0; i < tl.size(); i++ )
      {
        sim->print_debug( "[{}, {} (id={})]", i, *tl[ i ], tl[ i ]->actor_index );
      }
    }

    return tl.size();
  }

  bool insidious_ire_active() const
  {
    if ( !priest().talents.shadow.insidious_ire.enabled() )
      return false;

    return priest().buffs.insidious_ire->check();
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_ta_multiplier( s );

    if ( insidious_ire_active() )
    {
      m *= 1 + priest().talents.shadow.insidious_ire->effectN( 1 ).percent();
    }

    return m;
  }
};

struct void_volley_base_t : public priest_spell_t
{
  propagate_const<void_volley_damage_t*> void_volley_damage;
  propagate_const<void_volley_damage_aoe_t*> void_volley_damage_aoe;

  void_volley_base_t( priest_t& p, std::string name )
    : priest_spell_t( name, p, p.talents.shadow.void_volley_missile ), void_volley_damage( nullptr )
  {
    void_volley_damage     = new void_volley_damage_t( name_str + "_damage", p, p.talents.shadow.void_volley_damage );
    void_volley_damage_aoe = new void_volley_damage_aoe_t(
        name_str + "_damage_aoe", p, p.talents.shadow.void_volley_damage, data().effectN( 1 ).radius_max() );
    add_child( void_volley_damage );
    add_child( void_volley_damage_aoe );

    idol_of_nzoth_execute_stacks = 10;
    may_miss                     = false;
  }

  void_volley_base_t( priest_t& p, std::string name, util::string_view options_str ) : void_volley_base_t( p, name )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    if ( !priest().buffs.voidform->check() && !priest().buffs.crushing_void->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    // fire s1 bolts at main target
    void_volley_damage->target = s->target;
    make_repeating_event(
        sim, 50_ms, [ this ] { void_volley_damage->execute(); }, as<int>( data().effectN( 1 ).base_value() ) );

    if ( void_volley_damage_aoe->target != s->target )
    {
      void_volley_damage_aoe->target = s->target;
      // Invalidate the cache if the target has been changed.
      void_volley_damage_aoe->target_cache.is_valid = false;
    }
    // fire s3 bolts at secondary targets with s1 radius
    if ( void_volley_damage_aoe->target_list().size() > 0 )
    {
      make_repeating_event(
          sim, 50_ms, [ this ] { void_volley_damage_aoe->execute(); }, as<int>( data().effectN( 3 ).base_value() ) );
    }

    if ( priest().talents.shadow.crushing_void.enabled() && priest().buffs.crushing_void->check() )
    {
      priest().buffs.crushing_void->expire();
    }
  }
};

// Base version you cast while in Voidform
struct void_volley_t final : public void_volley_base_t
{
  void_volley_t( priest_t& p, util::string_view options ) : void_volley_base_t( p, "void_volley", options )
  {
  }
};

// Free version you get as you cast Voidform initially
// TODO: merge this with the normal Void Volley action
struct void_volley_voidform_t final : public void_volley_base_t
{
  void_volley_voidform_t( priest_t& p ) : void_volley_base_t( p, "void_volley_voidform" )
  {
    background         = true;
    track_cd_waste     = false;
    cooldown->duration = 0_s;

    // 10/03/2025
    // - Generates 10 Insanity
    // - Does not give Idol of N'Zoth stacks
    idol_of_nzoth_execute_stacks = 0;
  }
};

// ==========================================================================
// Voidform
// ==========================================================================
struct voidform_t final : public priest_spell_t
{
  propagate_const<void_volley_voidform_t*> void_volley;

  voidform_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "voidform", p, p.talents.shadow.voidform ), void_volley( nullptr )
  {
    parse_options( options_str );

    void_volley = new void_volley_voidform_t( priest() );
    add_child( void_volley );
    may_miss                     = false;
    idol_of_nzoth_execute_stacks = 10;

    if ( priest().talents.shadow.improved_voidform.enabled() )
    {
      energize_amount   = priest().talents.shadow.improved_voidform->effectN( 1 ).base_value();
      energize_type     = action_energize::ON_CAST;
      energize_resource = RESOURCE_INSANITY;
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.voidform->trigger();

    void_volley->execute();

    if ( priest().buffs.sustained_potency->check() )
    {
      priest().buffs.voidform->extend_duration( timespan_t::from_seconds( priest().buffs.sustained_potency->check() ) );

      priest().buffs.sustained_potency->expire();
    }

    if ( priest().talents.shared.mindbender.enabled() )
    {
      priest().pets.mindbender.spawn();
    }
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
// Idol of C'Thun (Talent)
// Parent action to store Void Tendril/Lasher damage
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
    : priest_spell_t( "void_torrent", p, p.talents.voidweaver.void_torrent ),
      insanity_gain(
          p.talents.voidweaver.void_torrent->effectN( 3 ).trigger()->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    may_crit                   = false;
    channeled                  = true;
    use_off_gcd                = true;
    tick_zero                  = true;
    dot_duration               = data().duration();
    affected_by_shadow_weaving = true;
    idol_of_nzoth_tick_stacks  = 2;

    // Getting insanity from the trigger spell data, base spell doesn't have it
    energize_type     = action_energize::NONE;
    energize_resource = RESOURCE_INSANITY;
    energize_amount   = insanity_gain;
  }

  bool usable_moving() const override
  {
    if ( priest().talents.voidweaver.dark_energy.enabled() )
    {
      return true;
    }

    return priest_spell_t::usable_moving();
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest().shadow_weaving_active_dots( target, id ) < 3.0 )
    {
      priest().procs.void_torrent_ticks_no_mastery->occur();
    }

    if ( priest().talents.shadow.psychic_link.enabled() )
    {
      priest().trigger_psychic_link( d->state );
    }

    priest().trigger_idol_of_cthun( d->state );
  }

  bool insidious_ire_active() const
  {
    if ( !priest().talents.shadow.insidious_ire.enabled() )
      return false;

    return priest().buffs.insidious_ire->check();
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_ta_multiplier( s );

    if ( insidious_ire_active() )
    {
      m *= 1 + priest().talents.shadow.insidious_ire->effectN( 1 ).percent();
    }

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    priest().buffs.void_torrent->expire();

    priest().buffs.entropic_rift->extend_duration( priest().buffs.entropic_rift->buff_duration() -
                                                   priest().buffs.entropic_rift->remains() );

    if ( priest().talents.voidweaver.voidheart.enabled() )
    {
      priest().buffs.voidheart->extend_duration( priest().buffs.voidheart->buff_duration() -
                                                 priest().buffs.voidheart->remains() );
    }

    priest_spell_t::last_tick( d );
  }

  void execute() override
  {
    // Spawn this before Void Torrent so that we get the damage bonus
    priest().trigger_entropic_rift();

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
      _pl_mind_flay( new psychic_link_base_t( "psychic_link_mind_flay", p, p.talents.shadow.psychic_link ) ),
      _pl_mind_flay_insanity(
          new psychic_link_base_t( "psychic_link_mind_flay_insanity", p, p.talents.shadow.psychic_link ) ),
      _pl_shadow_word_madness(
          new psychic_link_base_t( "psychic_link_shadow_word_madness", p, p.talents.shadow.psychic_link ) ),
      _pl_mindgames( new psychic_link_base_t( "psychic_link_mindgames", p, p.talents.shadow.psychic_link ) ),
      _pl_void_torrent( new psychic_link_base_t( "psychic_link_void_torrent", p, p.talents.shadow.psychic_link ) ),
      _pl_shadow_word_death(
          new psychic_link_base_t( "psychic_link_shadow_word_death", p, p.talents.shadow.psychic_link ) ),
      _pl_void_blast( new psychic_link_base_t( "psychic_link_void_blast", p, p.talents.shadow.psychic_link ) ),
      _pl_horrific_vision(
          new psychic_link_base_t( "psychic_link_horrific_vision", p, p.talents.shadow.psychic_link ) ),
      _pl_vision_of_nzoth( new psychic_link_base_t( "psychic_link_vision_of_nzoth", p, p.talents.shadow.psychic_link ) )
  {
    background  = true;
    radius      = data().effectN( 1 ).radius_max();
    callbacks   = false;
    base_dd_min = base_dd_max = 0;

    add_child( _pl_mind_blast );
    add_child( _pl_mind_flay );
    add_child( _pl_mind_flay_insanity );
    add_child( _pl_shadow_word_madness );
    add_child( _pl_mindgames );
    add_child( _pl_void_torrent );
    add_child( _pl_shadow_word_death );
    add_child( _pl_void_blast );
    add_child( _pl_horrific_vision );
    add_child( _pl_vision_of_nzoth );
  }

  void trigger( player_t* target, double original_amount, std::string action_name )
  {
    if ( action_name == "mind_blast" )
    {
      _pl_mind_blast->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "mind_flay" )
    {
      _pl_mind_flay->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "mind_flay_insanity" )
    {
      _pl_mind_flay_insanity->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "shadow_word_madness" )
    {
      _pl_shadow_word_madness->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "mindgames" )
    {
      _pl_mindgames->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "void_torrent" )
    {
      _pl_void_torrent->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "shadow_word_death" )
    {
      _pl_shadow_word_death->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "void_blast" )
    {
      _pl_void_blast->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "horrific_vision" )
    {
      _pl_horrific_vision->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "vision_of_nzoth" )
    {
      _pl_vision_of_nzoth->trigger( target, original_amount, action_name );
    }
    else
    {
      player->sim->print_debug( "{} tried to trigger psychic_link from unknown action {}.", priest(), action_name );
    }
  }

private:
  propagate_const<psychic_link_base_t*> _pl_mind_blast;
  propagate_const<psychic_link_base_t*> _pl_mind_flay;
  propagate_const<psychic_link_base_t*> _pl_mind_flay_insanity;
  propagate_const<psychic_link_base_t*> _pl_shadow_word_madness;
  propagate_const<psychic_link_base_t*> _pl_mindgames;
  propagate_const<psychic_link_base_t*> _pl_void_torrent;
  propagate_const<psychic_link_base_t*> _pl_shadow_word_death;
  propagate_const<psychic_link_base_t*> _pl_void_blast;
  propagate_const<psychic_link_base_t*> _pl_horrific_vision;
  propagate_const<psychic_link_base_t*> _pl_vision_of_nzoth;
};

// ==========================================================================
// Shadow Weaving
// Separate action to handle Mastery increase from Shadowfiend/Mindbender melee's
// Built similar to how the game handles the effect so breakdowns match
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

  // Disable multipliers from double dipping
  double composite_da_multiplier( const action_state_t* ) const override
  {
    return 1.0;
  }

  void trigger( player_t* target, double original_amount )
  {
    auto mult   = priest().shadow_weaving_multiplier( target, 0 ) - 1;
    base_dd_min = base_dd_max = original_amount * mult;
    player->sim->print_debug( "{} triggered shadow weaving on target {}. base: {}, mult: {}", priest(), *target,
                              original_amount, mult );

    set_target( target );
    execute();
  }
};

// ==========================================================================
// Tentacle Slam
// ==========================================================================
struct tentacle_slam_damage_t final : public priest_spell_t
{
  tentacle_slam_damage_t( priest_t& p, const spell_data_t* s ) : priest_spell_t( "tentacle_slam_damage", p, s )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
    reduced_aoe_targets        = 5;
    aoe                        = -1;
  }
};

struct tentacle_slam_dots_t final : public priest_spell_t
{
  propagate_const<vampiric_touch_t*> child_vt;

  tentacle_slam_dots_t( priest_t& p, const spell_data_t* s )
    : priest_spell_t( "tentacle_slam_dots", p, s->effectN( 3 ).trigger() ),
      child_vt( new vampiric_touch_t( priest(), true, false ) )
  {
    may_miss             = false;
    background           = true;
    aoe                  = as<int>( s->effectN( 3 ).base_value() );
    child_vt->background = true;
  }

  std::vector<player_t*>& target_list() const override
  {
    // Force regen this every time
    target_cache.is_valid = false;
    auto& tl              = priest_spell_t::target_list();
    auto original_size    = tl.size();

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

    player->sim->print_debug( "{} tentacle_slam dots {} targets of the available {}.", priest(), tl.size(),
                              original_size );

    return tl;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    child_vt->target = s->target;
    child_vt->execute();
  }
};

struct tentacle_slam_t final : public priest_spell_t
{
  propagate_const<tentacle_slam_dots_t*> tentacle_slam_dots;
  propagate_const<tentacle_slam_damage_t*> tentacle_slam_damage;
  propagate_const<shadow_word_madness_t*> child_swm;

  tentacle_slam_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "tentacle_slam", p, p.talents.shadow.tentacle_slam ),
      tentacle_slam_dots( nullptr ),
      tentacle_slam_damage( nullptr ),
      child_swm( nullptr )
  {
    parse_options( options_str );

    tentacle_slam_dots   = new tentacle_slam_dots_t( p, p.talents.shadow.tentacle_slam );
    tentacle_slam_damage = new tentacle_slam_damage_t( p, priest().talents.shadow.tentacle_slam_damage );
    child_swm            = new shadow_word_madness_t( priest(), false, true );

    add_child( tentacle_slam_damage );
    child_swm->background = true;

    idol_of_nzoth_impact_stacks = 6;
    radius                      = priest().talents.shadow.tentacle_slam_damage->effectN( 1 ).radius_max();
  }

  // TODO: Not found in spelldata, manually tested
  // Alpha: ~400ms
  // Beta: ~750ms
  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 0.5 );
  }

  // Triggers actions that only occur once, not per target hit
  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // DoTs are triggered first so that subsequent actions can apply
    if ( tentacle_slam_dots )
    {
      tentacle_slam_dots->execute();
    }

    // Damage occurs after
    if ( tentacle_slam_damage )
    {
      tentacle_slam_damage->execute();
    }

    // Happens after DoTs have already been applied and Damage has been done
    if ( priest().talents.shadow.maddening_tentacles.enabled() && child_swm )
    {
      child_swm->target = target;
      child_swm->execute();
    }

    if ( priest().talents.shadow.void_apparitions_3.enabled() )
    {
      priest().trigger_random_idol( s );
    }
  }
};

// ==========================================================================
// Idol of N'Zoth
// Horrific Vision - 1243105 - 50 Stacks
// Vision of N'Zoth - 1243106 - 100 Stacks
// ==========================================================================
struct horrific_vision_t final : public priest_spell_t
{
  double parent_targets = 1;

  horrific_vision_t( priest_t& p ) : priest_spell_t( "horrific_vision", p, p.talents.shadow.horrific_vision_damage )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      priest().trigger_psychic_link( s );
    }
  }
};

struct vision_of_nzoth_t final : public priest_spell_t
{
  double parent_targets = 1;

  vision_of_nzoth_t( priest_t& p ) : priest_spell_t( "vision_of_nzoth", p, p.talents.shadow.vision_of_nzoth_damage )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      priest().trigger_psychic_link( s );
    }
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
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

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

    if ( remaining_duration == 0_ms )
    {
      priest().sample_data.voidform_duration->add( elapsed( sim->current_time() ).total_seconds() );
    }

    if ( priest().talents.shadow.crushing_void.enabled() )
    {
      priest().cooldowns.void_volley->reset( true );
      priest().buffs.crushing_void->trigger();
    }

    if ( priest().buffs.ancient_madness->check() )
    {
      priest().buffs.ancient_madness->expire();
    }
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
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
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
    set_chance( 1.0 );
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
// Dispersion
// TODO: apply movement speed increase
// ==========================================================================
struct dispersion_t final : public priest_buff_t<buff_t>
{
  actions::spells::dispersion_heal_t* heal;

  dispersion_t( priest_t& p )
    : base_t( p, "dispersion", p.specs.dispersion ), heal( new actions::spells::dispersion_heal_t( p ) )
  {
    if ( !data().ok() )
      return;

    set_period( data().effectN( 5 ).period() );

    auto eff            = &data().effectN( 5 );
    auto health_percent = eff->percent();

    if ( p.talents.shadow.intangibility.enabled() )
    {
      health_percent += p.talents.shadow.intangibility->effectN( 2 ).percent();
    }

    int num_ticks = as<int>( p.specs.dispersion->duration().total_seconds() / tick_time().total_seconds() ) + 1;
    set_default_value( player->max_health() * health_percent / num_ticks );

    set_tick_callback( [ health_percent, num_ticks, this ]( buff_t*, int, timespan_t ) {
      heal->trigger( player->max_health() * health_percent / num_ticks );
    } );
  }
};

// ==========================================================================
// Void Torrent
// Has a fixed gain for Insanity that is not tied to the ticks of the spell
// ==========================================================================
struct void_torrent_t : public priest_buff_t<buff_t>
{
  void_torrent_t( priest_t& p ) : base_t( p, "void_torrent", p.talents.voidweaver.void_torrent->effectN( 3 ).trigger() )
  {
    set_default_value_from_effect( 1 );
    set_tick_zero( 1 );

    auto eff      = &data().effectN( 1 );
    auto insanity = eff->resource( RESOURCE_INSANITY );
    auto gain     = p.get_gain( "void_torrent" );

    set_tick_callback( [ insanity, gain, this ]( buff_t*, int, timespan_t ) {
      player->resource_gain( RESOURCE_INSANITY, insanity, gain );
    } );
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
double priest_t::generate_insanity( double num_amount, gain_t* g, action_t* action )
{
  if ( specialization() == PRIEST_SHADOW )
  {
    return resource_gain( RESOURCE_INSANITY, num_amount, g, action );
  }

  return 0.0;
}

void priest_t::create_buffs_shadow()
{
  // Baseline
  buffs.shadowform       = make_buff<buffs::shadowform_t>( *this );
  buffs.shadowform_state = make_buff<buffs::shadowform_state_t>( *this );
  buffs.vampiric_embrace = make_buff( this, "vampiric_embrace", specs.vampiric_embrace );
  buffs.voidform         = make_buff<buffs::voidform_t>( *this );
  buffs.dispersion       = make_buff<buffs::dispersion_t>( *this );

  // Talents
  buffs.void_torrent = make_buff<buffs::void_torrent_t>( *this );

  buffs.mind_devourer = make_buff( this, "mind_devourer", find_spell( 373204 ) );

  buffs.shadowy_insight = make_buff<buffs::shadowy_insight_t>( *this );

  buffs.mental_fortitude = new buffs::mental_fortitude_buff_t( this );

  buffs.insidious_ire = make_buff( this, "insidious_ire", talents.shadow.insidious_ire )
                            ->set_duration( timespan_t::zero() )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  buffs.thing_from_beyond = make_buff( this, "thing_from_beyond", find_spell( 373277 ) );

  buffs.screams_of_the_void = make_buff( this, "screams_of_the_void", find_spell( 393919 ) )
                                  ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  buffs.idol_of_yoggsaron =
      make_buff( this, "idol_of_yoggsaron", talents.shadow.idol_of_yoggsaron->effectN( 2 ).trigger() )
          ->set_stack_change_callback( ( [ this ]( buff_t* b, int, int cur ) {
            if ( cur == b->max_stack() )
            {
              make_event( b->sim, [ b ] { b->cancel(); } );
              spawn_thing_from_beyond();
            }
          } ) );

  buffs.idol_of_yshaarj = make_buff( this, "idol_of_yshaarj", talents.shadow.idol_of_yshaarj_buff )
                              ->set_default_value_from_effect( 1 )
                              ->add_invalidate( CACHE_HASTE );

  buffs.shattered_psyche =
      make_buff( this, "shattered_psyche", talents.shadow.shattered_psyche->effectN( 2 ).trigger() )
          ->set_default_value_from_effect( 1 );

  buffs.horrific_vision = make_buff( this, "horrific_vision", talents.shadow.horrific_vision_buff )
                              ->set_default_value_from_effect( 1 )
                              ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                              ->set_freeze_stacks( true )
                              ->set_period( talents.shadow.horrific_vision_buff->effectN( 1 ).period() )
                              ->set_tick_callback( [ this ]( buff_t* buff, int, timespan_t ) {
                                double insanity =
                                    talents.shadow.horrific_vision_buff->effectN( 1 ).resource( RESOURCE_INSANITY );
                                generate_insanity( insanity * buff->check(), gains.insanity_horrific_vision, nullptr );
                              } );
  buffs.vision_of_nzoth = make_buff( this, "vision_of_nzoth", talents.shadow.vision_of_nzoth_buff )
                              ->set_default_value_from_effect( 1 )
                              ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                              ->set_freeze_stacks( true )
                              ->set_period( talents.shadow.vision_of_nzoth_buff->effectN( 1 ).period() )
                              ->set_tick_callback( [ this ]( buff_t* buff, int, timespan_t ) {
                                double insanity =
                                    talents.shadow.vision_of_nzoth_buff->effectN( 1 ).resource( RESOURCE_INSANITY );
                                generate_insanity( insanity * buff->check(), gains.insanity_vision_of_nzoth, nullptr );
                              } );

  // Tracking buff to see if the free reset is available for SW:D with DaM talented.
  buffs.death_and_madness_reset =
      make_buff( this, "death_and_madness_reset", talents.shadow.death_and_madness_reset_buff )
          ->set_trigger_spell( talents.shadow.death_and_madness )
          ->set_proc_callbacks( false );

  buffs.crushing_void = make_buff( this, "crushing_void", talents.shadow.crushing_void_buff );

  buffs.ancient_madness = make_buff( this, "ancient_madness", talents.shadow.ancient_madness )
                              ->set_duration( timespan_t::zero() )
                              ->set_max_stack( 99 );
}  // namespace priestspace

void priest_t::init_rng_shadow()
{
  rppm.idol_of_cthun          = get_rppm( "idol_of_cthun", talents.shadow.idol_of_cthun );
  rppm.power_of_the_dark_side = get_rppm( "power_of_the_dark_side", talents.discipline.power_of_the_dark_side );

  // Deck of cards model for void_apparitions_3 random idol selection: 3/4/1/2/7 out of 17.
  deck_rng.random_idol = get_shuffled_rng( "void_apparitions_3_random_idol",
                                           { { static_cast<int>( random_idol_e::YSHAARJ ), 3 },
                                             { static_cast<int>( random_idol_e::NZOTH_HORRIFIC_VISION ), 4 },
                                             { static_cast<int>( random_idol_e::NZOTH_VISION_OF_NZOTH ), 1 },
                                             { static_cast<int>( random_idol_e::YOGG ), 2 },
                                             { static_cast<int>( random_idol_e::CTHUN ), 7 } } );

  // Shadowy Insight
  const dot_t* shadow_word_pain = get_dot( "shadow_word_pain", this );

  double si_chance = talents.shadow.shadowy_insight.ok() ? 0.08 : 0.0;
  si_chance *= 1.0 + talents.shadow.dark_thoughts->effectN( 2 ).percent();

  threshold_rng.shadowy_insight = get_threshold_rng(
      "shadowy_insight", si_chance,
      [ this, shadow_word_pain ]( double increment_max, action_state_t* ) {
        unsigned active_dots = get_active_dots( shadow_word_pain );
        if ( active_dots == 0 )
          return 0.0;
        return increment_max * rng().range( 0.0, 2.0 ) / pow( active_dots, 0.9 );
      },
      true, true );

  // Maddening Touch
  double mt_chance = talents.shadow.maddening_touch.ok() ? 0.25 : 0.0;

  const dot_t* vampiric_touch = get_dot( "vampiric_touch", this );

  threshold_rng.maddening_touch = get_threshold_rng(
      "maddening_touch", mt_chance,
      [ this, vampiric_touch ]( double increment_max, action_state_t* ) {
        unsigned active_dots = get_active_dots( vampiric_touch );
        if ( active_dots == 0 )
          return 0.0;
        return increment_max * rng().range( 0.25, 1.75 ) / pow( active_dots, 0.6 );
      },
      true, true );

  // Tormented Spirits
  double ts_chance = talents.shadow.tormented_spirits.ok() ? 0.13 : 0;

  threshold_rng.tormented_spirits = get_threshold_rng(
      "tormented_spirits", ts_chance,
      [ this, shadow_word_pain ]( double increment_max, action_state_t* s ) {
        unsigned active_dots = get_active_dots( shadow_word_pain );
        if ( active_dots == 0 )
          return 0.0;

        double chance = increment_max * rng().range( 0.0, 2.0 ) / pow( active_dots, 0.9 );

        if ( s->result == RESULT_CRIT )
          return 2.0 * chance;

        return chance;
      },
      true, true );

  // Auspicious Spirits
  double as_chance = talents.shadow.auspicious_spirits.ok() ? 0.8 : 0;

  threshold_rng.auspicious_spirits = get_threshold_rng(
      "auspicious_spirits", as_chance,
      [ this ]( double increment_max, action_state_t* s ) {
        auto* state = background_actions.shadowy_apparitions->cast_state( s );

        if ( state->number_spawned == 0 )
          return 0.0;

        return increment_max / pow( state->number_spawned, 0.8 );
      },
      true, true );
}

void priest_t::init_spells_shadow()
{
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };

  // Row 2
  talents.shadow.psychic_link       = ST( "Psychic Link" );
  talents.shadow.misery             = ST( "Misery" );
  talents.shadow.invoked_nightmares = ST( "Invoked Nightmares" );
  talents.shadow.intangibility      = ST( "Intangibility" );
  talents.shadow.mental_fortitude   = ST( "Mental Fortitude" );
  // Row 3
  talents.shadow.thought_harvester    = ST( "Thought Harvester" );
  talents.shadow.tentacle_slam        = ST( "Tentacle Slam" );
  talents.shadow.tentacle_slam_damage = find_spell( 1227621 );
  talents.shadow.shadowy_apparition   = find_spell( 148859 );
  talents.shadow.shadowy_apparitions  = ST( "Shadowy Apparitions" );
  // Row 4
  talents.shadow.tormenting_whispers = ST( "Tormenting Whispers" );  // NYI
  talents.shadow.descending_darkness = ST( "Descending Darkness" );
  talents.shadow.surge_of_insanity   = ST( "Surge of Insanity" );  // NYI
  // Row 5
  talents.shadow.shadowy_insight     = ST( "Shadowy Insight" );
  talents.shadow.voidtouched         = ST( "Voidtouched" );
  talents.shadow.voidform            = ST( "Voidform" );
  talents.shadow.void_volley         = ST( "Void Volley" );
  talents.shadow.void_volley_missile = find_spell( 1242173 );
  talents.shadow.void_volley_damage  = find_spell( 1242189 );
  talents.shadow.haunting_shadows    = ST( "Haunting Shadows" );
  talents.shadow.mental_decay        = ST( "Mental Decay" );
  // Row 6
  talents.shadow.dark_thoughts            = ST( "Dark Thoughts" );
  talents.shadow.maddening_touch          = ST( "Maddening Touch" );
  talents.shadow.maddening_touch_insanity = find_spell( 391232 );
  talents.shadow.improved_voidform        = ST( "Improved Voidform" );
  talents.shadow.ancient_madness          = ST( "Ancient Madness" );
  talents.shadow.phantom_menace           = ST( "Phantom Menace" );
  talents.shadow.dark_evangelism          = ST( "Dark Evangelism" );
  talents.shadow.shattered_psyche         = ST( "Shattered Psyche" );
  // Row 7
  talents.shadow.subservient_shadows = ST( "Subservient Shadows" );
  talents.shadow.mastermind          = ST( "Mastermind" );
  talents.shadow.minds_eye           = ST( "Mind's Eye" );
  talents.shadow.distorted_reality   = ST( "Distorted Reality" );
  talents.shadow.spectral_horrors    = ST( "Spectral Horrors" );
  talents.shadow.instilled_doubt     = ST( "Instilled Doubt" );
  // Row 8
  talents.shadow.deathspeaker                 = ST( "Deathspeaker" );
  talents.shadow.death_and_madness            = ST( "Death and Madness" );
  talents.shadow.death_and_madness_insanity   = find_spell( 321973 );
  talents.shadow.death_and_madness_reset_buff = find_spell( 390628 );
  talents.shadow.mind_devourer                = ST( "Mind Devourer" );
  talents.shadow.auspicious_spirits           = ST( "Auspicious Spirits" );
  talents.shadow.maddening_tentacles          = ST( "Maddening Tentacles" );
  // Row 9
  talents.shadow.madness_weaving     = ST( "Madness Weaving" );
  // Deaths Torment (Shared)
  talents.shadow.screams_of_the_void = ST( "Screams of the Void" );
  talents.shadow.tormented_spirits   = ST( "Tormented Spirits" );
  talents.shadow.insidious_ire       = ST( "Insidious Ire" );
  talents.shadow.crushing_void       = ST( "Crushing Void" );
  talents.shadow.crushing_void_buff  = find_spell( 1279437 );
  // Row 10
  talents.shadow.idol_of_yshaarj        = ST( "Idol of Y'Shaarj" );
  talents.shadow.idol_of_yshaarj_buff   = find_spell( 373316 );
  talents.shadow.horrific_visions       = find_spell( 1243069 );  // Idol of N'Zoth debuff
  talents.shadow.horrific_vision_damage = find_spell( 1243105 );  // Idol of N'Zoth 50 stack damage
  talents.shadow.vision_of_nzoth_damage = find_spell( 1243106 );  // Idol of N'Zoth 100 stack damage
  talents.shadow.horrific_vision_buff   = find_spell( 1243113 );  // Idol of N'Zoth 50 stack buff
  talents.shadow.vision_of_nzoth_buff   = find_spell( 1243114 );  // Idol of N'Zoth 100 stack buff
  talents.shadow.idol_of_nzoth          = ST( "Idol of N'Zoth" );
  talents.shadow.idol_of_yoggsaron      = ST( "Idol of Yogg-Saron" );
  talents.shadow.idol_of_cthun          = ST( "Idol of C'Thun" );
  // Apex
  talents.shadow.void_apparitions_1 = find_talent_spell( talent_tree::SPECIALIZATION, 1264096 );
  talents.shadow.void_apparitions_2 = find_talent_spell( talent_tree::SPECIALIZATION, 1264104 );
  talents.shadow.void_apparition    = find_spell( 1264175 );
  talents.shadow.void_bolt          = find_spell( 1264177 );
  talents.shadow.void_apparitions_3 = find_talent_spell( talent_tree::SPECIALIZATION, 1264107 );

  // General Spells
  specs.mind_flay      = find_specialization_spell( "Mind Flay" );
  specs.shadowform     = find_specialization_spell( "Shadowform" );
  specs.voidform       = find_spell( 194249 );
  specs.hallucinations = find_spell( 199579 );
  specs.dispersion     = find_specialization_spell( "Dispersion" );
  specs.silence        = find_specialization_spell( "Silence" );
}

void priest_t::init_special_effects_shadow()
{
}

action_t* priest_t::create_action_shadow( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  if ( name == "mind_flay" )
  {
    return new mind_flay_t( *this, options_str );
  }
  if ( name == "mind_flay_insanity" )
  {
    return new mind_flay_insanity_t( *this, options_str );
  }
  if ( name == "voidform" )
  {
    return new voidform_t( *this, options_str );
  }
  if ( name == "tentacle_slam" )
  {
    return new tentacle_slam_t( *this, options_str );
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
  if ( name == "vampiric_embrace" )
  {
    return new vampiric_embrace_t( *this, options_str );
  }
  if ( name == "shadowform" )
  {
    return new shadowform_t( *this, options_str );
  }
  if ( name == "shadow_word_madness" )
  {
    return new shadow_word_madness_t( *this, options_str );
  }
  if ( name == "void_volley" )
  {
    return new void_volley_t( *this, options_str );
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

  if ( talents.shadow.idol_of_cthun.enabled() || talents.shadow.void_apparitions_3.enabled() )
  {
    background_actions.idol_of_cthun = new actions::spells::idol_of_cthun_t( *this );
  }

  if ( talents.shadow.mental_fortitude.enabled() )
  {
    background_actions.mental_fortitude = new actions::spells::mental_fortitude_t( *this );
  }

  background_actions.shadow_weaving = new actions::spells::shadow_weaving_t( *this );

  background_actions.shadow_word_pain = new actions::spells::shadow_word_pain_t( *this );

  if ( talents.shadow.idol_of_nzoth.enabled() || talents.shadow.void_apparitions_3.enabled() )
  {
    background_actions.horrific_vision = new actions::spells::horrific_vision_t( *this );
    background_actions.vision_of_nzoth = new actions::spells::vision_of_nzoth_t( *this );
  }

  if ( talents.shadow.void_apparitions_2.enabled() )
  {
    background_actions.void_apparitions = new actions::spells::void_apparition_spell_t( *this );
    background_actions.void_bolt        = new actions::spells::void_bolt_t( *this );
  }
}

// ==========================================================================
// Trigger Shadowy Apparitions on all targets affected by vampiric touch
// ==========================================================================
void priest_t::trigger_shadowy_apparitions( proc_t* proc )
{
  if ( !talents.shadow.shadowy_apparitions.enabled() )
  {
    return;
  }

  auto has_vt = []( priest_td_t* t ) { return t && t->dots.vampiric_touch->is_ticking(); };

  int vts = 0;

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( has_vt( priest_td ) )
    {
      vts++;
    }
  }

  // Idol of Yogg-Saron only triggers for each cast that generates an apparition
  if ( talents.shadow.idol_of_yoggsaron.enabled() && vts > 0 )
  {
    buffs.idol_of_yoggsaron->trigger();
  }

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( has_vt( priest_td ) )
    {
      if ( talents.shadow.void_apparitions_2.enabled() &&
           rng().roll( talents.shadow.void_apparitions_2->effectN( 2 ).percent() ) )
      {
        procs.void_apparition->occur();
        background_actions.void_apparitions->trigger( priest_td->target, proc, vts );
      }
      else
      {
        background_actions.shadowy_apparitions->trigger( priest_td->target, proc, vts );
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
      background_actions.psychic_link->trigger( priest_td->target, s->result_amount, s->action->name_str );
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

// Helper function to refresh insidious ire buff
void priest_t::refresh_insidious_ire_buff( action_state_t* s )
{
  if ( !talents.shadow.insidious_ire.enabled() )
    return;

  const priest_td_t* td = find_target_data( s->target );

  if ( !td )
    return;

  if ( td->dots.shadow_word_pain->is_ticking() && td->dots.vampiric_touch->is_ticking() &&
       td->dots.shadow_word_madness->is_ticking() )
  {
    timespan_t min_length = std::min( { td->dots.shadow_word_pain->remains(), td->dots.vampiric_touch->remains(),
                                        td->dots.shadow_word_madness->remains() } );

    if ( min_length >= buffs.insidious_ire->remains() )
    {
      if ( talents.shadow.insidious_ire.enabled() )
        buffs.insidious_ire->trigger( min_length );
    }
  }
}

void priest_t::trigger_horrific_vision( player_t* target )
{
  if ( !talents.shadow.idol_of_nzoth.enabled() && !talents.shadow.void_apparitions_3.enabled() )
  {
    return;
  }

  background_actions.horrific_vision->execute_on_target( target );
  buffs.horrific_vision->trigger();
  if ( talents.shadow.void_apparitions_1.enabled() )
  {
    trigger_shadowy_apparitions( procs.shadowy_apparition_nzoth );
  }
}

void priest_t::trigger_vision_of_nzoth( player_t* target )
{
  if ( !talents.shadow.idol_of_nzoth.enabled() && !talents.shadow.void_apparitions_3.enabled() )
  {
    return;
  }

  background_actions.vision_of_nzoth->execute_on_target( target );
  buffs.vision_of_nzoth->trigger();

  if ( talents.shadow.void_apparitions_1.enabled() )
  {
    trigger_shadowy_apparitions( procs.shadowy_apparition_nzoth );
  }
}

void priest_t::trigger_idol_of_nzoth( player_t* target, int stacks )
{
  if ( !talents.shadow.idol_of_nzoth.enabled() )
  {
    return;
  }

  auto td = get_target_data( target );

  if ( !td )
  {
    return;
  }

  int current_stacks            = td->buffs.horrific_visions->check();
  int new_stacks                = current_stacks + stacks;
  int horrific_vision_threshold = as<int>( talents.shadow.idol_of_nzoth->effectN( 1 ).base_value() );
  int vision_of_nzoth_threshold = as<int>( talents.shadow.idol_of_nzoth->effectN( 2 ).base_value() );

  if ( new_stacks < vision_of_nzoth_threshold )
  {
    td->buffs.horrific_visions->trigger( stacks );
  }

  if ( current_stacks )
  {
    if ( current_stacks < horrific_vision_threshold && new_stacks >= horrific_vision_threshold )
    {
      trigger_horrific_vision( target );
    }
    else if ( current_stacks < vision_of_nzoth_threshold && new_stacks >= vision_of_nzoth_threshold )
    {
      // clear out old stacks
      td->buffs.horrific_visions->expire();

      trigger_vision_of_nzoth( target );

      int leftover_stacks = new_stacks - vision_of_nzoth_threshold;
      if ( leftover_stacks > 0 )
      {
        td->buffs.horrific_visions->trigger( leftover_stacks );
      }

      sim->print_debug(
          "Idol of N'Zoth rollover from {} stacks - current_stacks: {}, new_stacks: {}, leftover_stacks: {}", stacks,
          current_stacks, new_stacks, leftover_stacks );
    }
  }
}

void priest_t::spawn_thing_from_beyond()
{
  pets.thing_from_beyond.spawn();
  procs.thing_from_beyond->occur();

  if ( talents.shadow.void_apparitions_1.enabled() )
  {
    trigger_shadowy_apparitions( procs.shadowy_apparition_yogg );
  }
}

void priest_t::trigger_idol_of_yshaarj()
{
  pets.shadowfiend.spawn();

  // Shadowfiend will only trigger the buff if you have the talent enabled
  // Tentacle Slam will still give this buff, even if not talented into it
  if ( !talents.shadow.idol_of_yshaarj.enabled() )
  {
    buffs.idol_of_yshaarj->trigger();
  }

  if ( talents.shadow.void_apparitions_1.enabled() )
  {
    trigger_shadowy_apparitions( procs.shadowy_apparition_yshaarj );
  }
}

void priest_t::trigger_random_idol( action_state_t* s )
{
  if ( !talents.shadow.void_apparitions_3.enabled() )
  {
    return;
  }

  if ( !rng().roll( talents.shadow.void_apparitions_3->effectN( 1 ).percent() ) )
  {
    return;
  }

  procs.tentacle_slam_idol->occur();

  random_idol_e chosen_idol = random_idol_e::NONE;
  if ( deck_rng.random_idol )
  {
    chosen_idol = static_cast<random_idol_e>( deck_rng.random_idol->trigger() );
  }

  switch ( chosen_idol )
  {
    case random_idol_e::YSHAARJ:
      procs.void_apparition_yshaarj->occur();
      trigger_idol_of_yshaarj();
      break;
    case random_idol_e::NZOTH_HORRIFIC_VISION:
      procs.void_apparition_horrific_vision->occur();
      trigger_horrific_vision( s->target );
      break;
    case random_idol_e::NZOTH_VISION_OF_NZOTH:
      procs.void_apparition_vision_of_nzoth->occur();
      trigger_vision_of_nzoth( s->target );
      break;
    case random_idol_e::YOGG:
      procs.void_apparition_yogg->occur();
      spawn_thing_from_beyond();
      break;
    case random_idol_e::CTHUN:
      procs.void_apparition_cthun->occur();
      // Tentacle Slam already rolled this idol. Spawn directly to avoid additional C'Thun RPPM gating.
      spawn_idol_of_cthun( s );
      break;
    default:
      sim->print_debug( "Could not trigger a valid Idol" );
      break;
  }
}

void priest_t::trigger_shadowy_insight( bool guaranteed, action_state_t* s )
{
  if ( !talents.shadow.shadowy_insight.enabled() && !talents.voidweaver.void_empowerment.enabled() )
  {
    return;
  }

  int stack = buffs.shadowy_insight->check();
  if ( guaranteed || threshold_rng.shadowy_insight->trigger( s ) )
  {
    buffs.shadowy_insight->trigger();

    if ( buffs.shadowy_insight->check() == stack )
    {
      procs.shadowy_insight_overflow->occur();
    }
    else
    {
      procs.shadowy_insight->occur();
    }
  }
}

}  // namespace priestspace

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
// Mind Flay
// ==========================================================================
struct mind_flay_base_t final : public priest_spell_t
{
  mind_flay_base_t( util::string_view n, priest_t& p, const spell_data_t* s ) : priest_spell_t( n, p, s )
  {
    affected_by_shadow_weaving = true;
    may_crit                   = false;
    channeled                  = true;
    use_off_gcd                = true;
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

      td.dots.shadow_word_pain->adjust_duration( dot_extension );
      td.dots.vampiric_touch->adjust_duration( dot_extension );
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
    if ( priest().talents.manipulation.enabled() )
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

  bool ready() override
  {
    if ( priest().talents.shadow.mind_spike.enabled() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }

private:
  propagate_const<action_t*> _base_spell;
  propagate_const<action_t*> _insanity_spell;
};

// ==========================================================================
// Mind Spike
// ==========================================================================
struct mind_spike_base_t : public priest_spell_t
{
  timespan_t manipulation_cdr;

  mind_spike_base_t( util::string_view n, priest_t& p, const spell_data_t* s )
    : priest_spell_t( n, p, s ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) )
  {
    affected_by_shadow_weaving = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      priest().trigger_psychic_link( s );

      if ( priest().talents.shadow.mental_decay.enabled() )
      {
        timespan_t dot_extension =
            timespan_t::from_seconds( priest().talents.shadow.mental_decay->effectN( 2 ).base_value() );
        priest_td_t& td = get_td( s->target );

        td.dots.shadow_word_pain->adjust_duration( dot_extension );
        td.dots.vampiric_touch->adjust_duration( dot_extension );
      }

      priest().trigger_idol_of_cthun( s );
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

    if ( priest().talents.shadow.dark_evangelism.enabled() )
    {
      priest().buffs.dark_evangelism->trigger();
    }
  }
};

struct mind_spike_t final : public mind_spike_base_t
{
  mind_spike_t( priest_t& p, util::string_view options_str )
    : mind_spike_base_t( "mind_spike", p, p.talents.shadow.mind_spike )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    if ( priest().buffs.mind_spike_insanity->check() )
      return false;

    return mind_spike_base_t::ready();
  }
};

struct mind_spike_insanity_t final : public mind_spike_base_t
{
  mind_spike_insanity_t( priest_t& p, util::string_view options_str )
    : mind_spike_base_t( "mind_spike_insanity", p, p.talents.shadow.mind_spike_insanity_spell )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    mind_spike_base_t::execute();
    priest().buffs.mind_spike_insanity->decrement();
  }

  bool ready() override
  {
    if ( !priest().buffs.mind_spike_insanity->check() )
      return false;

    return mind_spike_base_t::ready();
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
      insanity( priest().specs.hallucinations->effectN( 1 ).resource() )
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

struct shadowy_apparition_state_t : public action_state_t
{
  double source_crit;
  double number_spawned;
  bool buffed_by_darkflame_shroud;
  double darkflame_shroud_mult;

  shadowy_apparition_state_t( action_t* a, player_t* t )
    : action_state_t( a, t ),
      source_crit( 1.0 ),
      number_spawned( 1.0 ),
      buffed_by_darkflame_shroud( false ),
      darkflame_shroud_mult( t->find_spell( 410871 )->effectN( 1 ).percent() )
  {
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );
    fmt::print( s, " source_crit={}, number_spawned={}, buffed_by_darkflame_shroud={}", source_crit, number_spawned,
                buffed_by_darkflame_shroud );
    return s;
  }

  void initialize() override
  {
    action_state_t::initialize();
    source_crit                = 1.0;
    number_spawned             = 1.0;
    buffed_by_darkflame_shroud = false;
  }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    auto other_sa_state        = debug_cast<const shadowy_apparition_state_t*>( o );
    number_spawned             = other_sa_state->number_spawned;
    source_crit                = other_sa_state->source_crit;
    buffed_by_darkflame_shroud = other_sa_state->buffed_by_darkflame_shroud;
  }

  double composite_da_multiplier() const override
  {
    double m = action_state_t::composite_da_multiplier();

    m *= source_crit;

    if ( buffed_by_darkflame_shroud )
    {
      m *= 1 + darkflame_shroud_mult;
    }

    return m;
  }
};

struct shadowy_apparition_spell_t final : public priest_spell_t
{
protected:
  using state_t = shadowy_apparition_state_t;

public:
  double insanity_gain;

  struct shadowy_apparition_damage_t final : public priest_spell_t
  {
    double insanity_gain;

    shadowy_apparition_damage_t( priest_t& p )
      : priest_spell_t( "shadowy_apparition", p, p.talents.shadow.shadowy_apparition->effectN( 1 ).trigger() ),
        insanity_gain( priest().talents.shadow.auspicious_spirits->effectN( 2 ).percent() )
    {
      affected_by_shadow_weaving = true;
      background                 = true;
      proc                       = false;
      callbacks                  = true;
      may_miss                   = false;
      may_crit                   = false;

      base_dd_multiplier *= 1 + priest().talents.shadow.auspicious_spirits->effectN( 1 ).percent();
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = priest_spell_t::composite_target_multiplier( t );

      if ( priest().talents.shadow.phantasmal_pathogen.enabled() )
      {
        const priest_td_t* td = priest().find_target_data( t );
        if ( td->dots.devouring_plague->is_ticking() )
          m *= 1 + priest().talents.shadow.phantasmal_pathogen->effectN( 1 ).percent();
      }

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
        // Not found in spelldata, hardcoding based on empirical data
        auto chance = 0.8 * std::pow( cast_state( s )->number_spawned, -0.8 );
        if ( rng().roll( chance ) )
        {
          priest().generate_insanity( insanity_gain, priest().gains.insanity_auspicious_spirits, s->action );
        }
      }
    }
  };

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
    auto state = impact_action->get_state( s );
    impact_action->snapshot_state( state, impact_action->amount_type( state ) );
    impact_action->schedule_execute( state );
  }

  /** Trigger a shadowy apparition */
  void trigger( player_t* target, proc_t* proc, bool _gets_crit_mod, int vts )
  {
    player->sim->print_debug( "{} triggered shadowy apparition on target {} from {}. crit_mod={}, vts_active={}",
                              priest(), *target, proc->name(), _gets_crit_mod, vts );

    state_t* s = cast_state( get_state() );

    snapshot_state( s, amount_type( s ) );

    s->source_crit    = _gets_crit_mod ? 2.0 : 1.0;
    s->number_spawned = vts;
    s->target         = target;

    // Darkflame Shroud buffs Apparitions as they spawn, not on hit
    s->buffed_by_darkflame_shroud = priest().buffs.darkflame_shroud->check();

    proc->occur();

    schedule_execute( s );
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

    if ( priest().talents.shadow.misery.enabled() )
    {
      dot_duration += priest().talents.shadow.misery->effectN( 2 ).time_value();
    }

    // Discipline: 8% / 15% damage increase
    apply_affecting_aura( priest().talents.discipline.pain_and_suffering );
    // Spell Direct and Periodic 3%/5% gain
    apply_affecting_aura( priest().talents.throes_of_pain );

    if ( priest().talents.holy.divine_image.enabled() )
    {
      child_searing_light = priest().background_actions.searing_light;
    }
    if ( priest().sets->has_set_bonus( PRIEST_DISCIPLINE, T30, B2 ) )
    {
      apply_affecting_aura( p.sets->set( PRIEST_DISCIPLINE, T30, B2 ) );
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

      if ( child_searing_light && priest().buffs.divine_image->up() )
      {
        for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
        {
          child_searing_light->execute();
        }
      }
    }
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t t = priest_spell_t::tick_time( state );

    if ( priest().talents.shadow.screams_of_the_void.enabled() && priest().buffs.screams_of_the_void->check() )
    {
      t *= ( 1 + priest().talents.shadow.screams_of_the_void->effectN( 2 ).percent() );
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
        // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/1097
        // Tormented Spirits Shadowy Apparitions get the crit mod if the last action to
        // trigger a Shadowy Apparition crit, not if the SW:P tick crit
        priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_swp,
                                              priest().bugs ? false : d->state->result == RESULT_CRIT );
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
  bool insanity;
  bool ud_proc;
  bool ud_execute;

  vampiric_touch_t( priest_t& p, bool _casted = false, bool _insanity = true, bool _ud_proc = true,
                    bool _ud_execute = true )
    : priest_spell_t( "vampiric_touch", p, p.dot_spells.vampiric_touch ),
      vampiric_touch_heal( new vampiric_touch_heal_t( p ) ),
      child_swp( nullptr ),
      child_ud( nullptr )
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

    apply_affecting_aura( priest().talents.shadow.maddening_touch );
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

    if ( priest().talents.shadow.screams_of_the_void.enabled() && priest().buffs.screams_of_the_void->check() )
    {
      t *= ( 1 + priest().talents.shadow.screams_of_the_void->effectN( 2 ).percent() );
    }

    return t;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      if ( priest().talents.shadow.maddening_touch.enabled() && priest().cooldowns.maddening_touch_icd->up() )
      {
        // Not found in spelldata, based on empirical data
        auto chance = 0.25 * std::pow( priest().get_active_dots( internal_id ), -0.6 );
        if ( rng().roll( chance ) )
        {
          priest().cooldowns.maddening_touch_icd->start();
          priest().generate_insanity(
              priest().talents.shadow.maddening_touch->effectN( 2 ).resource( RESOURCE_INSANITY ),
              priest().gains.insanity_maddening_touch, d->state->action );
        }
      }

      priest().trigger_idol_of_nzoth( d->state->target, priest().procs.idol_of_nzoth_vt );
      vampiric_touch_heal->trigger( d->state->result_amount );
    }
  }
};

// ==========================================================================
// Devouring Plague
// ==========================================================================
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

    apply_affecting_aura( p.talents.shadow.voidtouched );
    apply_affecting_aura( p.talents.shadow.minds_eye );
    apply_affecting_aura( p.talents.shadow.distorted_reality );

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T30, B4 ) )
    {
      apply_affecting_aura( p.sets->set( PRIEST_SHADOW, T30, B4 ) );
    }
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_persistent_multiplier( s );

    // Spelldata does not have data for Devouring Plague so apply_buff_effects does not work with DP
    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T29, B2 ) && priest().buffs.gathering_shadows->check() )
    {
      m *= 1 + priest().buffs.gathering_shadows->check_stack_value();
    }

    if ( priest().buffs.mind_devourer->check() )
    {
      m *= 1 + priest().buffs.mind_devourer->data().effectN( 2 ).percent();
    }

    return m;
  }

  void consume_resource() override
  {
    if ( casted )
    {
      priest_spell_t::consume_resource();
    }

    // TODO: shouldn't need to check casted anymore
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

    // TODO: shouldn't need to check casted anymore
    if ( casted )
    {
      priest().trigger_shadowy_apparitions( priest().procs.shadowy_apparition_dp, s->result == RESULT_CRIT );
    }

    if ( result_is_hit( s->result ) )
    {
      devouring_plague_heal->trigger( s->result_amount );

      priest().trigger_psychic_link( s );
      priest().refresh_insidious_ire_buff( s );
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      devouring_plague_heal->trigger( d->state->result_amount );

      priest().trigger_psychic_link( d->state );
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.shadow.mind_flay_insanity.enabled() )
    {
      priest().buffs.mind_flay_insanity->trigger();
    }

    if ( priest().talents.shadow.surge_of_insanity.enabled() )
    {
      if ( priest().talents.shadow.mind_spike.enabled() )
        priest().buffs.mind_spike_insanity->trigger();
      else
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

    if ( priest().talents.shadow.void_eruption.enabled() && priest().buffs.voidform->up() )
    {
      priest().buffs.voidform->extend_duration(
          &priest(), timespan_t::from_millis( priest().talents.shadow.void_eruption->effectN( 2 ).base_value() ) );
    }

    if ( priest().talents.shadow.screams_of_the_void.enabled() )
    {
      priest().buffs.screams_of_the_void->trigger();
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T30, B4 ) )
    {
      priest().buffs.darkflame_embers->trigger();
    }
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

      td.dots.shadow_word_pain->adjust_duration( dot_extension );
      td.dots.vampiric_touch->adjust_duration( dot_extension );

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

    auto rank2 = p.find_spell( 231688 );
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

    if ( result_is_hit( s->result ) )
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

    // Turn off the dummy periodic effect
    base_td_multiplier = 0;
    dot_duration       = timespan_t::from_seconds( 0 );
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

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( s->target->type == ENEMY_ADD || target->level() < sim->max_player_level + 3 )
    {
      priest_td_t& td = get_td( s->target );
      td.buffs.psychic_horror->trigger();
      s->target->buffs.stunned->trigger( data().duration() );
      s->target->stun();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !priest_spell_t::target_ready( candidate_target ) )
      return false;

    if ( target->type == ENEMY_ADD || target->level() < sim->max_player_level + 3 )
      return true;

    return false;
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
    energize_type     = action_energize::NONE;
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
    priest_spell_t::last_tick( d );

    priest().buffs.void_torrent->expire();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.void_torrent->trigger();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest().spawn_idol_of_cthun( s );
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
      _pl_mind_spike_insanity(
          new psychic_link_base_t( "psychic_link_mind_spike_insanity", p, p.talents.shadow.psychic_link ) ),
      _pl_devouring_plague(
          new psychic_link_base_t( "psychic_link_devouring_plague", p, p.talents.shadow.psychic_link ) ),
      _pl_mindgames( new psychic_link_base_t( "psychic_link_mindgames", p, p.talents.shadow.psychic_link ) ),
      _pl_void_bolt( new psychic_link_base_t( "psychic_link_void_bolt", p, p.talents.shadow.psychic_link ) ),
      _pl_void_torrent( new psychic_link_base_t( "psychic_link_void_torrent", p, p.talents.shadow.psychic_link ) ),
      _pl_shadow_word_death(
          new psychic_link_base_t( "psychic_link_shadow_word_death", p, p.talents.shadow.psychic_link ) )
  {
    background  = true;
    radius      = data().effectN( 1 ).radius_max();
    callbacks   = false;
    base_dd_min = base_dd_max = 0;

    add_child( _pl_mind_blast );
    add_child( _pl_mind_spike );
    add_child( _pl_mind_flay );
    add_child( _pl_mind_flay_insanity );
    add_child( _pl_mind_spike_insanity );
    add_child( _pl_devouring_plague );
    add_child( _pl_mindgames );
    add_child( _pl_void_bolt );
    add_child( _pl_void_torrent );
    add_child( _pl_shadow_word_death );
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
    else if ( action_name == "mind_spike_insanity" )
    {
      _pl_mind_spike_insanity->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "devouring_plague" )
    {
      _pl_devouring_plague->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "mindgames" )
    {
      _pl_mindgames->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "void_bolt" )
    {
      _pl_void_bolt->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "void_torrent" )
    {
      _pl_void_torrent->trigger( target, original_amount, action_name );
    }
    else if ( action_name == "shadow_word_death" )
    {
      _pl_shadow_word_death->trigger( target, original_amount, action_name );
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
  propagate_const<psychic_link_base_t*> _pl_mind_spike_insanity;
  propagate_const<psychic_link_base_t*> _pl_devouring_plague;
  propagate_const<psychic_link_base_t*> _pl_mindgames;
  propagate_const<psychic_link_base_t*> _pl_void_bolt;
  propagate_const<psychic_link_base_t*> _pl_void_torrent;
  propagate_const<psychic_link_base_t*> _pl_shadow_word_death;
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

    if ( priest().talents.shadow.whispering_shadows.enabled() )
    {
      shadow_crash_dots->execute();
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
    // BUG: RPPM value not found in spelldata
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/956
    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T30, B2 ) )
    {
      set_rppm( RPPM_HASTE, 2.4 * ( 1 + priest().sets->set( PRIEST_SHADOW, T30, B2 )->effectN( 3 ).percent() ) );
    }
    else
    {
      set_rppm( RPPM_HASTE, 2.4 );
    }
    // Allow player to react to the buff being applied so they can cast Mind Blast.
    this->reactable = true;

    // Create a stack change callback to adjust the number of Mind Blast charges.
    set_stack_change_callback( [ this ]( buff_t*, int old, int cur ) {
      if ( cur > old )
      {
        priest().cooldowns.mind_blast->reset( true );
      }
    } );
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
  ancient_madness_t( priest_t& p, const spell_data_t* s ) : base_t( p, "ancient_madness", s )
  {
    if ( !data().ok() )
      return;

    // Since we are using data from VF/DA make sure the name in the report does not confuse people
    s_data_reporting = p.talents.shadow.ancient_madness.spell();

    if ( p.talents.shadow.dark_ascension.enabled() )
    {
      set_period( p.talents.shadow.dark_ascension->effectN( 3 ).period() );
      set_duration( p.talents.shadow.dark_ascension->duration() );
    }

    if ( p.talents.shadow.void_eruption.enabled() )
    {
      set_period( p.specs.voidform->effectN( 4 ).period() );
      set_duration( p.specs.voidform->duration() );
    }

    add_invalidate( CACHE_CRIT_CHANCE );
    add_invalidate( CACHE_SPELL_CRIT_CHANCE );
    set_reverse( true );
    set_max_stack( 20 );
    cooldown->duration = 0_s;

    set_default_value( priest().talents.shadow.ancient_madness->effectN( 2 ).percent() / 10 );
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

// ==========================================================================
// Devoured Despair (Idol of Y'Shaarj)
// ==========================================================================
struct devoured_despair_buff_t : public priest_buff_t<buff_t>
{
  devoured_despair_buff_t( priest_t& p ) : base_t( p, "devoured_despair", p.talents.shadow.devoured_despair )
  {
    set_cooldown( 0_ms );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    set_trigger_spell( p.talents.shadow.idol_of_yshaarj );
    set_duration( p.talents.shadow.devoured_pride->duration() );

    auto eff      = &data().effectN( 1 );
    auto insanity = eff->resource( RESOURCE_INSANITY );
    set_default_value( insanity / eff->period().total_seconds() );

    auto gain = p.get_gain( "devoured_despair" );
    set_tick_callback( [ insanity, gain, this ]( buff_t*, int, timespan_t ) {
      player->resource_gain( RESOURCE_INSANITY, insanity, gain );
    } );
  }
};

// ==========================================================================
// Void Torrent
// Has a fixed gain for Insanity that is not tied to the ticks of the spell
// ==========================================================================
struct void_torrent_t : public priest_buff_t<buff_t>
{
  void_torrent_t( priest_t& p ) : base_t( p, "void_torrent", p.talents.shadow.void_torrent->effectN( 3 ).trigger() )
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
  if ( talents.shadow.dark_ascension.enabled() )
  {
    buffs.ancient_madness = make_buff<buffs::ancient_madness_t>( *this, talents.shadow.dark_ascension );
  }
  else
  {
    buffs.ancient_madness = make_buff<buffs::ancient_madness_t>( *this, specs.voidform );
  }

  buffs.unfurling_darkness =
      make_buff( this, "unfurling_darkness", talents.shadow.unfurling_darkness->effectN( 1 ).trigger() );
  buffs.unfurling_darkness_cd =
      make_buff( this, "unfurling_darkness_cd",
                 talents.shadow.unfurling_darkness->effectN( 1 ).trigger()->effectN( 2 ).trigger() );

  buffs.void_torrent = make_buff<buffs::void_torrent_t>( *this );

  buffs.mind_devourer = make_buff( this, "mind_devourer", find_spell( 373204 ) )
                            ->set_trigger_spell( talents.shadow.mind_devourer )
                            ->set_chance( talents.shadow.mind_devourer->effectN( 1 ).percent() );

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
              procs.thing_from_beyond->occur();
              spawn_thing_from_beyond();
            }
          } ) );

  buffs.dark_evangelism =
      make_buff( this, "dark_evangelism", find_spell( 391099 ) )->set_trigger_spell( talents.shadow.dark_evangelism );

  buffs.devoured_pride = make_buff( this, "devoured_pride", talents.shadow.devoured_pride )
                             ->set_trigger_spell( talents.shadow.idol_of_yshaarj )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.devoured_despair = make_buff<buffs::devoured_despair_buff_t>( *this );

  buffs.mind_melt = make_buff( this, "mind_melt", talents.shadow.mind_melt->effectN( 2 ).trigger() )
                        ->set_default_value_from_effect( 1 );

  buffs.mind_flay_insanity = make_buff( this, "mind_flay_insanity", find_spell( 391401 ) );

  buffs.mind_spike_insanity = make_buff( this, "mind_spike_insanity", find_spell( 407468 ) );

  buffs.deathspeaker = make_buff( this, "deathspeaker", talents.shadow.deathspeaker->effectN( 1 ).trigger() )
                           ->set_stack_change_callback( [ this ]( buff_t*, int old, int cur ) {
                             cooldowns.shadow_word_death->adjust_max_charges( cur - old );
                           } );

  buffs.dark_ascension = make_buff( this, "dark_ascension", talents.shadow.dark_ascension )
                             ->set_default_value_from_effect( 1 )
                             ->set_cooldown( 0_s )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // BUG: Tracking buff for bugged Tormented Spirits crit handling
  buffs.last_shadowy_apparition_crit =
      make_buff( this, "last_shadowy_apparition_crit" )->set_quiet( true )->set_duration( 0_s )->set_max_stack( 1 );

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

  // TODO: Wire up spell data, split into helper function.

  auto darkflame_embers  = find_spell( 409502 );
  buffs.darkflame_embers = make_buff( this, "darkflame_embers", darkflame_embers )
                               ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                               ->set_stack_change_callback( [ this ]( buff_t* b, int old, int ) {
                                 if ( old == b->max_stack() )
                                 {
                                   buffs.darkflame_shroud->trigger();
                                 }
                               } );

  if ( darkflame_embers->ok() )
    buffs.darkflame_embers->set_expire_at_max_stack( true );  // Avoid sim warning

  buffs.darkflame_shroud =
      make_buff( this, "darkflame_shroud", find_spell( 410871 ) )->set_default_value_from_effect( 1 );
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
  talents.shadow.intangibility    = ST( "Intangibility" );
  talents.shadow.mental_fortitude = ST( "Mental Fortitude" );
  talents.shadow.misery           = ST( "Misery" );
  talents.shadow.last_word        = ST( "Last Word" );
  talents.shadow.psychic_horror   = ST( "Psychic Horror" );
  // Row 4
  talents.shadow.thought_harvester         = ST( "Thought Harvester" );
  talents.shadow.psychic_link              = ST( "Psychic Link" );
  talents.shadow.surge_of_insanity         = ST( "Surge of Insanity" );
  talents.shadow.mind_spike_insanity_spell = find_spell( 407466 );
  talents.shadow.mind_flay_insanity        = ST( "Mind Flay: Insanity" );
  talents.shadow.mind_flay_insanity_spell  = find_spell( 391403 );  // Not linked to talent, actual dmg spell
  // Row 5
  talents.shadow.shadow_crash         = ST( "Shadow Crash" );
  talents.shadow.unfurling_darkness   = ST( "Unfurling Darkness" );
  talents.shadow.void_eruption        = ST( "Void Eruption" );
  talents.shadow.void_eruption_damage = find_spell( 228360 );
  talents.shadow.dark_ascension       = ST( "Dark Ascension" );
  talents.shadow.mental_decay         = ST( "Mental Decay" );
  talents.shadow.mind_spike           = ST( "Mind Spike" );
  // Row 6
  talents.shadow.whispering_shadows = ST( "Whispering Shadows" );
  talents.shadow.shadowy_insight    = ST( "Shadowy Insight" );
  talents.shadow.ancient_madness    = ST( "Ancient Madness" );
  talents.shadow.voidtouched        = ST( "Voidtouched" );
  talents.shadow.mind_melt          = ST( "Mind Melt" );
  // Row 7
  talents.shadow.maddening_touch          = ST( "Maddening Touch" );
  talents.shadow.maddening_touch_insanity = find_spell( 391232 );
  talents.shadow.dark_evangelism          = ST( "Dark Evangelism" );
  talents.shadow.mind_devourer            = ST( "Mind Devourer" );
  talents.shadow.phantasmal_pathogen      = ST( "Phantasmal Pathogen" );
  talents.shadow.minds_eye                = ST( "Mind's Eye" );
  talents.shadow.distorted_reality        = ST( "Distorted Reality" );
  // Row 8
  talents.shadow.mindbender         = ST( "Mindbender" );
  talents.shadow.deathspeaker       = ST( "Deathspeaker" );
  talents.shadow.auspicious_spirits = ST( "Auspicious Spirits" );
  talents.shadow.void_torrent       = ST( "Void Torrent" );
  // Row 9
  talents.shadow.inescapable_torment = ST( "Inescapable Torment" );
  talents.shadow.mastermind          = ST( "Mastermind" );
  talents.shadow.screams_of_the_void = ST( "Screams of the Void" );
  talents.shadow.tormented_spirits   = ST( "Tormented Spirits" );
  talents.shadow.insidious_ire       = ST( "Insidious Ire" );
  talents.shadow.malediction         = ST( "Malediction" );
  // Row 10
  talents.shadow.idol_of_yshaarj   = ST( "Idol of Y'Shaarj" );
  talents.shadow.devoured_pride    = find_spell( 373316 );  // Pet Damage, Your Damage - Healthy
  talents.shadow.devoured_despair  = find_spell( 373317 );  // Insanity Generation - Stunned - NYI
  talents.shadow.devoured_anger    = find_spell( 373318 );  // Haste - Enrage - NYI
  talents.shadow.devoured_fear     = find_spell( 373319 );  // Big Personal Damage - Feared - NYI
  talents.shadow.devoured_violence = find_spell( 373320 );  // Pet Extension - Default
  talents.shadow.idol_of_nzoth     = ST( "Idol of N'Zoth" );
  talents.shadow.idol_of_yoggsaron = ST( "Idol of Yogg-Saron" );
  talents.shadow.idol_of_cthun     = ST( "Idol of C'Thun" );

  // General Spells
  specs.mind_flay      = find_specialization_spell( "Mind Flay" );
  specs.shadowform     = find_specialization_spell( "Shadowform" );
  specs.void_bolt      = find_spell( 205448 );
  specs.voidform       = find_spell( 194249 );
  specs.hallucinations = find_spell( 199579 );
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
  if ( name == "mind_spike" )
  {
    return new mind_spike_t( *this, options_str );
  }
  if ( name == "mind_spike_insanity" )
  {
    return new mind_spike_insanity_t( *this, options_str );
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

  // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/1097
  // Tormented Spirits Shadowy Apparitions get the crit mod if the last action to
  // trigger a Shadowy Apparition crit, not if the SW:P tick crit
  if ( bugs )
  {
    if ( proc == procs.shadowy_apparition_swp )
    {
      if ( buffs.last_shadowy_apparition_crit->check() )
      {
        sim->print_debug( "{} triggered a shadowy_apparition from tormented_spirits with the crit mod", *this );
        gets_crit_mod = true;
      }
    }
    else
    {
      if ( gets_crit_mod )
      {
        buffs.last_shadowy_apparition_crit->trigger();
      }
      else
      {
        buffs.last_shadowy_apparition_crit->expire();
      }
    }
  }

  // Idol of Yogg-Saron only triggers for each cast that generates an apparition
  if ( talents.shadow.idol_of_yoggsaron.enabled() )
  {
    buffs.idol_of_yoggsaron->trigger();
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

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( has_vt( priest_td ) )
    {
      background_actions.shadowy_apparitions->trigger( priest_td->target, proc, gets_crit_mod, vts );
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
// Trigger Shadow Weaving on the Target
// ==========================================================================
void priest_t::trigger_shadow_weaving( action_state_t* s )
{
  background_actions.shadow_weaving->trigger( s->target, s->result_amount );
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

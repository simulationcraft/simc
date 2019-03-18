// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_priest.hpp"

namespace priestspace
{
namespace actions
{
namespace spells
{
struct dispersion_t final : public priest_spell_t
{
  dispersion_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "dispersion", player, player.find_class_spell( "Dispersion" ) )
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

    // Adjust the Voidform end event (essentially remove it) after the Dispersion buff is up, since it disables insanity
    // drain for the duration of the channel
    priest().insanity.adjust_end_event();
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

    // When Dispersion ends, restart the insanity drain tracking
    priest().insanity.begin_tracking();
  }
};

struct mind_blast_t final : public priest_spell_t
{
private:
  double insanity_gain;
  double whispers_of_the_damned_value;
  double harvested_thoughts_value;
  double whispers_bonus_insanity;

public:
  mind_blast_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "mind_blast", player,
                      player.talents.shadow_word_void->ok() ? player.find_talent_spell( "Shadow Word: Void" )
                                                            : player.find_class_spell( "Mind Blast" ) ),
      whispers_of_the_damned_value( priest().azerite.whispers_of_the_damned.value( 2 ) ),
      harvested_thoughts_value( priest().azerite.thought_harvester.value( 1 ) ),
      whispers_bonus_insanity( priest()
                                   .azerite.whispers_of_the_damned.spell()
                                   ->effectN( 1 ).trigger()
                                   ->effectN( 1 ).trigger()
                                   ->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    insanity_gain = data().effectN( 2 ).resource( RESOURCE_INSANITY );
    insanity_gain *= ( 1.0 + priest().talents.fortress_of_the_mind->effectN( 2 ).percent() );
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data

    spell_power_mod.direct *= 1.0 + player.talents.fortress_of_the_mind->effectN( 4 ).percent();
  }

  void init() override
  {
    priest().cooldowns.mind_blast->hasted = true;

    priest_spell_t::init();
  }

  void schedule_execute( action_state_t* s ) override
  {
    priest_spell_t::schedule_execute( s );

    priest().buffs.shadowy_insight->expire();
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.shadow_word_void->ok() )
    {
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_VERSATILITY_RATING, player );
    }
    else
    {
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, player );
    }
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_da( state );

    if ( priest().azerite.whispers_of_the_damned.enabled() )
    {
      d += whispers_of_the_damned_value;
    }

    return d;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    double total_insanity_gain = insanity_gain;

    if ( priest().azerite.whispers_of_the_damned.enabled() && s->result == RESULT_CRIT )
    {
      total_insanity_gain += whispers_bonus_insanity;
    }

    priest().generate_insanity( total_insanity_gain, priest().gains.insanity_mind_blast, s->action );
  }

  timespan_t execute_time() const override
  {
    if ( priest().buffs.shadowy_insight->check() )
    {
      return timespan_t::zero();
    }

    timespan_t et = priest_spell_t::execute_time();

    return et;
  }

  timespan_t cooldown_base_duration( const cooldown_t& cooldown ) const override
  {
    timespan_t cd = priest_spell_t::cooldown_base_duration( cooldown );
    if ( priest().buffs.voidform->check() )
    {
      cd += priest().buffs.voidform->data().effectN( 6 ).time_value();
    }
    return cd;
  }

  void update_ready( timespan_t cd_duration ) override
  {
    priest().buffs.voidform->up();  // Benefit tracking

    // Shadowy Insight has proc'd during the cast of Mind Blast, the cooldown reset is deferred to the finished cast,
    // instead of "eating" it.
    if ( priest().buffs.shadowy_insight->check() )
    {
      cd_duration            = timespan_t::zero();
      cooldown->last_charged = sim->current_time();

      if ( sim->debug )
      {
        sim->out_debug.printf(
            "%s shadowy insight proc occured during %s cast. Deferring "
            "cooldown reset.",
            priest().name(), name() );
      }
    }

    priest_spell_t::update_ready( cd_duration );
  }
};

struct mind_sear_tick_t final : public priest_spell_t
{
  double insanity_gain;
  double harvested_thoughts_multiplier;
  bool thought_harvester_empowered = false;

  mind_sear_tick_t( priest_t& p, const spell_data_t* mind_sear )
    : priest_spell_t( "mind_sear_tick", p, mind_sear->effectN( 1 ).trigger() ),
      insanity_gain( p.find_spell( 208232 )->effectN( 1 ).percent() ),
      harvested_thoughts_multiplier( priest()
                                         .azerite.thought_harvester.spell()
                                         ->effectN( 1 ).trigger()
                                         ->effectN( 1 ).trigger()
                                         ->effectN( 1 ).percent() )
  {
    background    = true;
    dual          = true;
    aoe           = -1;
    callbacks     = false;
    direct_tick   = false;
    use_off_gcd   = true;
    dynamic_tick_action = true;
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_da( state );

    if ( priest().azerite.searing_dialogue.enabled() )
    {
      auto shadow_word_pain_dot = state->target->get_dot( "shadow_word_pain", player );

      if ( shadow_word_pain_dot != nullptr && shadow_word_pain_dot->is_ticking() )
      {
        d += priest().azerite.searing_dialogue.value( 1 );
      }
    }

    return d;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( thought_harvester_empowered )
    {
      d *= ( 1.0 + harvested_thoughts_multiplier );
    }

    return d;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_mind_sear, s->action );
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, player );
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_HASTE_RATING, player );
    }
  }
};

struct mind_sear_t final : public priest_spell_t
{
  mind_sear_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "mind_sear", p, p.find_specialization_spell( ( "Mind Sear" ) ) )
  {
    parse_options( options_str );
    channeled           = true;
    may_crit            = false;
    hasted_ticks        = false;
    dynamic_tick_action = true;
    tick_zero           = false;

    tick_action = new mind_sear_tick_t( p, p.find_class_spell( "Mind Sear" ) );
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().buffs.harvested_thoughts->check() )
    {
      ( (mind_sear_tick_t*)tick_action )->thought_harvester_empowered = true;
      priest().buffs.harvested_thoughts->expire();
    }
    else {
      ( (mind_sear_tick_t*)tick_action )->thought_harvester_empowered = false;
    }
  }
};

struct mind_flay_t final : public priest_spell_t
{
  double insanity_gain;

  mind_flay_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "mind_flay", p, p.find_specialization_spell( "Mind Flay" ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) *
                     ( 1.0 + p.talents.fortress_of_the_mind->effectN( 1 ).percent() ) )
  {
    parse_options( options_str );

    may_crit      = false;
    channeled     = true;
    hasted_ticks  = false;
    use_off_gcd   = true;
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data

    spell_power_mod.tick *= 1.0 + p.talents.fortress_of_the_mind->effectN( 3 ).percent();
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    priest().generate_insanity( insanity_gain, priest().gains.insanity_mind_flay, d->state->action );
  }
};

struct shadow_word_death_t final : public priest_spell_t
{
  shadow_word_death_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_word_death", p, p.talents.shadow_word_death )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    double total_insanity_gain    = 0.0;
    double save_health_percentage = s->target->health_percentage();

    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // TODO: Add in a custom buff that checks after 1 second to see if the target SWD was cast on is now dead.

      if ( ( ( save_health_percentage > 0.0 ) && ( s->target->health_percentage() <= 0.0 ) ) )
      {
        total_insanity_gain = data().effectN( 4 ).base_value();
      }
      else
      {
        total_insanity_gain = data().effectN( 3 ).base_value();
      }

      priest().generate_insanity( total_insanity_gain, priest().gains.insanity_shadow_word_death, s->action );
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->health_percentage() < as<double>( data().effectN( 2 ).base_value() ) )
    {
      return priest_spell_t::target_ready( candidate_target );
    }

    return false;
  }

  bool ready() override
  {
    if ( !priest_spell_t::ready() )
    {
      return false;
    }

    return true;
  }
};

struct shadow_crash_damage_t final : public priest_spell_t
{
  shadow_crash_damage_t( priest_t& p )
    : priest_spell_t( "shadow_crash_damage", p, p.talents.shadow_crash->effectN( 1 ).trigger() )
  {
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data
    background    = true;
  }
};

struct shadow_crash_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_crash_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_crash", p, p.talents.shadow_crash ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    aoe    = -1;
    radius = data().effectN( 1 ).radius();

    impact_action = new shadow_crash_damage_t( p );
    add_child( impact_action );

    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().generate_insanity( insanity_gain, priest().gains.insanity_shadow_crash, execute_state->action );
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_MASTERY_RATING, player );
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_VERSATILITY_RATING, player );
  }

  timespan_t travel_time() const override
  {
    // Always has the same time to land regardless of distance, probably represented there. Anshlun 2018-08-04
    return timespan_t::from_seconds( data().missile_speed() );
  }
};

struct shadowform_t final : public priest_spell_t
{
  shadowform_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadowform", p, p.find_class_spell( "Shadowform" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.shadowform_state->trigger();
    priest().buffs.shadowform->trigger();
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_VERSATILITY_RATING, player );
  }
};

struct silence_t final : public priest_spell_t
{
  silence_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "silence", player, player.find_class_spell( "Silence" ) )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;

    cooldown           = priest().cooldowns.silence;
    cooldown->duration = data().cooldown();
    if ( priest().talents.last_word->ok() )
    {
      // Spell data has a negative value
      cooldown->duration += priest().talents.last_word->effectN( 1 ).time_value();
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    // Only interrupts, does not keep target silenced. This works in most cases since bosses are rarely able to be
    // completely silenced.
    // Removed to not break Casting Patchwerk - 2017-09-22
    // if ( target->debuffs.casting )
    //{
    // target->debuffs.casting->expire();
    //}
  }

  bool target_ready( player_t* candidate_target ) override
  {
    return priest_spell_t::target_ready( candidate_target );
    // Only available if the target is casting
    // Or if the target can get blank silenced
    if ( !( candidate_target->type != ENEMY_ADD && ( candidate_target->level() < sim->max_player_level + 3 ) &&
            candidate_target->debuffs.casting && candidate_target->debuffs.casting->check() ) )
    {
      return false;
    }
  }
};

struct mind_bomb_t final : public priest_spell_t
{
  mind_bomb_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "mind_bomb", player, player.talents.mind_bomb )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;

    cooldown = priest().cooldowns.mind_bomb;
    // Added 2 seconds to emulate the setup time
    // Simplifying the spell
    cooldown->duration += timespan_t::from_seconds( 2 );
  }
};

struct psychic_horror_t final : public priest_spell_t
{
  psychic_horror_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "psychic_horror", player, player.talents.psychic_horror )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;

    cooldown = priest().cooldowns.psychic_horror;
  }
};

struct surrender_to_madness_t final : public priest_spell_t
{
  surrender_to_madness_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "surrender_to_madness", p, p.talents.surrender_to_madness )
  {
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.surrender_to_madness->trigger();
  }
};

struct vampiric_embrace_t final : public priest_spell_t
{
  vampiric_embrace_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "vampiric_embrace", p, p.find_class_spell( "Vampiric Embrace" ) )
  {
    parse_options( options_str );

    harmful = false;

    if ( priest().talents.sanlayn->ok() )
    {
      cooldown->duration = data().cooldown() + priest().talents.sanlayn->effectN( 1 ).time_value();
    }
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

struct shadowy_apparition_damage_t final : public priest_spell_t
{
  double insanity_gain;
  double spiteful_apparitions_bonus;

  shadowy_apparition_damage_t( priest_t& p )
    : priest_spell_t( "shadowy_apparition", p, p.find_spell( 148859 ) ),
      insanity_gain( priest().talents.auspicious_spirits->effectN( 2 ).percent() ),
      spiteful_apparitions_bonus( priest().azerite.spiteful_apparitions.value( 1 ) )
  {
    background = true;
    proc       = false;
    callbacks  = true;
    may_miss   = false;
    may_crit   = false;

    // Hardcoded value. This is the behavior announced and tested in game
    // However the value doesn't show up anywhere in the known spelldata
    // Anshlun 2018-10-02
    if ( spiteful_apparitions_bonus > 0.0 && !priest().talents.auspicious_spirits->ok() )
    {
      spiteful_apparitions_bonus *= 1.75;
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().talents.auspicious_spirits->ok() )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_auspicious_spirits, s->action );
    }
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_da( state );

    if ( priest().azerite.spiteful_apparitions.enabled() )
    {
      auto vampiric_touch_dot = state->target->get_dot( "vampiric_touch", player );

      if ( vampiric_touch_dot != nullptr && vampiric_touch_dot->is_ticking() )
      {
        d += spiteful_apparitions_bonus;
      }
    }

    return d;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    d *= 1.0 + priest().talents.auspicious_spirits->effectN( 1 ).percent();

    return d;
  }
};

struct shadowy_apparition_spell_t final : public priest_spell_t
{
  shadowy_apparition_spell_t( priest_t& p ) : priest_spell_t( "shadowy_apparitions", p, p.find_spell( 78203 ) )
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
  void trigger()
  {
    if ( priest().sim->debug )
    {
      priest().sim->out_debug << priest().name() << " triggered shadowy apparition.";
    }

    priest().procs.shadowy_apparition->occur();
    schedule_execute();
    // TODO: Determine if this is dependent on talenting into Auspicious Spirits
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_HASTE_RATING, player );
  }
};

// ==========================================================================
// Shadow Word: Pain
// ==========================================================================
struct shadow_word_pain_t final : public priest_spell_t
{
  double insanity_gain;
  bool casted;
  timespan_t increased_time;

  shadow_word_pain_t( priest_t& p, const std::string& options_str, bool _casted = true )
    : priest_spell_t( "shadow_word_pain", p, p.find_class_spell( "Shadow Word: Pain" ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) ),
      increased_time(
          timespan_t::from_millis( priest().azerite.torment_of_torments.spell()->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    casted    = _casted;
    may_crit  = true;
    tick_zero = false;
    if ( !casted )
    {
      base_dd_max = 0.0;
      base_dd_min = 0.0;
    }
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data

    if ( priest().specs.shadowy_apparitions->ok() && !priest().active_spells.shadowy_apparitions )
    {
      priest().active_spells.shadowy_apparitions = new shadowy_apparition_spell_t( p );
      // If SW:P is the only action having SA, then we can add it as a child stat.
      add_child( priest().active_spells.shadowy_apparitions );
    }
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    return casted ? priest_spell_t::spell_direct_power_coefficient( s ) : 0.0;
  }

  double bonus_ta( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_ta( state );

    if ( priest().azerite.death_throes.enabled() )
    {
      d += priest().azerite.death_throes.value( 1 );
    }

    return d;
  }

  // TODO: This assumes death_thoes doesn't affect the direct damage portion - didn't test it
  double bonus_da( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_da( state );

    if ( casted && priest().azerite.torment_of_torments.enabled() )
    {
      d += priest().azerite.torment_of_torments.value( 2 );
    }

    return d;
  }

  timespan_t composite_dot_duration( const action_state_t* state ) const override
  {
    timespan_t t = priest_spell_t::composite_dot_duration( state );

    if ( priest().azerite.torment_of_torments.enabled() )
    {
      t += increased_time;
    }

    return t;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( casted )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_shadow_word_pain_onhit, s->action );
    }

    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_HASTE_RATING, player );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest().active_spells.shadowy_apparitions && ( d->state->result_amount > 0 ) )
    {
      if ( d->state->result == RESULT_CRIT )
      {
        priest().active_spells.shadowy_apparitions->trigger();
      }
    }

    if ( d->state->result_amount > 0 )
    {
      if ( priest().rppm.shadowy_insight->trigger() )
      {
        trigger_shadowy_insight();
        expansion::bfa::trigger_leyshocks_grand_compilation( STAT_MASTERY_RATING, player );
      }
    }
  }

  double cost() const override
  {
    double c = priest_spell_t::cost();

    if ( priest().specialization() == PRIEST_SHADOW )
    {
      return 0.0;
    }

    return c;
  }
};

struct vampiric_touch_t final : public priest_spell_t
{
  double insanity_gain;
  double harvested_thoughts_value;
  propagate_const<shadow_word_pain_t*> child_swp;
  bool ignore_healing;

  vampiric_touch_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "vampiric_touch", p, p.find_class_spell( "Vampiric Touch" ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) ),
      harvested_thoughts_value( priest().azerite.thought_harvester.value( 2 ) ),
      ignore_healing( p.options.priest_ignore_healing )
  {
    parse_options( options_str );

    may_crit = false;

    if ( priest().talents.misery->ok() )
    {
      child_swp             = new shadow_word_pain_t( priest(), std::string( "" ), false );
      child_swp->background = true;
    }
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data
  }

  void trigger_heal( action_state_t* s )
  {
    if ( ignore_healing )
    {
      return;
    }
    /*
    double amount_to_heal = s->result_amount * data().effectN( 2 ).m_value();
    double actual_amount =
        priest().resource_gain( RESOURCE_HEALTH, amount_to_heal, priest().gains.vampiric_touch_health );
    double overheal = amount_to_heal - actual_amount;
    */
  }

  double bonus_ta( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_ta( state );

    if ( priest().azerite.thought_harvester.enabled() )
    {
      d += harvested_thoughts_value;
    }

    return d;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest().generate_insanity( insanity_gain, priest().gains.insanity_vampiric_touch_onhit, s->action );

    if ( priest().talents.misery->ok() )
    {
      child_swp->target = s->target;
      child_swp->execute();
    }
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_MASTERY_RATING, player );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( !ignore_healing )
    {
      trigger_heal( d->state );
    }

    if ( d->state->result_amount > 0 && priest().azerite.thought_harvester.enabled() )
    {
      if ( priest().rppm.harvested_thoughts->trigger() )
      {
        priest().buffs.harvested_thoughts->trigger();
      }
    }
  }
};

struct void_bolt_t final : public priest_spell_t
{
  struct void_bolt_extension_t : public priest_spell_t
  {
    timespan_t dot_extension;

    void_bolt_extension_t( priest_t& player )
      : priest_spell_t( "void_bolt_extension", player, player.find_specialization_spell( 231688 ) )
    {
      dot_extension = data().effectN( 1 ).time_value();
      aoe           = -1;
      radius        = player.find_spell( 234746 )->effectN( 1 ).radius();
      may_miss      = false;
      background = dual = true;
    }

    virtual timespan_t travel_time() const override
    {
      return timespan_t::zero();
    }

    void impact( action_state_t* s ) override
    {
      priest_spell_t::impact( s );

      priest_td_t& td = get_td( s->target );

      if ( td.dots.shadow_word_pain->is_ticking() )
      {
        td.dots.shadow_word_pain->extend_duration( dot_extension, true );
      }

      if ( td.dots.vampiric_touch->is_ticking() )
      {
        td.dots.vampiric_touch->extend_duration( dot_extension, true );
      }
      // note: this is super buggy in-game and doesn't always give the buff
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, player );
    }
  };

  double insanity_gain;
  void_bolt_extension_t* void_bolt_extension;

  void_bolt_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "void_bolt", player, player.find_spell( 205448 ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) ),
      void_bolt_extension( nullptr )
  {
    parse_options( options_str );
    use_off_gcd      = true;
    energize_type    = ENERGIZE_NONE;  // disable resource generation from spell data.
    cooldown->hasted = true;

    auto rank2 = player.find_specialization_spell( 231688 );
    if ( rank2->ok() )
    {
      void_bolt_extension = new void_bolt_extension_t( player );
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().generate_insanity( insanity_gain, priest().gains.insanity_void_bolt, execute_state->action );
  }

  bool ready() override
  {
    if ( !( priest().buffs.voidform->check() ) )
    {
      return false;
    }

    return priest_spell_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( void_bolt_extension )
    {
      void_bolt_extension->target = s->target;
      void_bolt_extension->schedule_execute();
    }
  }
};

struct dark_void_t final : public priest_spell_t
{
  propagate_const<shadow_word_pain_t*> child_swp;
  double insanity_gain;

  dark_void_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "dark_void", p, p.find_talent_spell( "Dark Void" ) ),
      child_swp( new shadow_word_pain_t( priest(), std::string( "" ), false ) ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) )

  {
    parse_options( options_str );
    base_costs[ RESOURCE_INSANITY ] = 0.0;
    energize_type                   = ENERGIZE_NONE;  // disable resource generation from spell data.
    child_swp->background           = true;

    may_miss = false;
    aoe      = -1;
    radius   = data().effectN( 1 ).radius_max();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    child_swp->target = s->target;
    child_swp->execute();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().generate_insanity( insanity_gain, priest().gains.insanity_dark_void, execute_state->action );
  }
};

struct void_eruption_damage_t final : public priest_spell_t
{
  propagate_const<action_t*> void_bolt;

  void_eruption_damage_t( priest_t& p )
    : priest_spell_t( "void_eruption_damage", p, p.find_spell( 228360 ) ), void_bolt( nullptr )
  {
    may_miss   = false;
    background = true;
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
  double insanity_required;

  void_eruption_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "void_eruption", p, p.find_spell( 228260 ) )
  {
    parse_options( options_str );

    if ( priest().talents.legacy_of_the_void->ok() )
    {
      insanity_required = (double)priest().talents.legacy_of_the_void->effectN( 6 ).base_value();
    }
    else
    {
      insanity_required = data().cost( POWER_INSANITY ) / 100.0;
    }

    impact_action = new void_eruption_damage_t( p );
    add_child( impact_action );

    if ( sim->debug )
    {
      sim->print_debug( "Void Eruption requires {} insanity", insanity_required );
    }

    // We don't want to lose insanity when casting it!
    base_costs[ RESOURCE_INSANITY ] = 0;

    may_miss = false;
    aoe      = -1;
    cooldown = priest().cooldowns.void_bolt;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.voidform->trigger();
    priest().cooldowns.void_bolt->reset( true );
  }

  bool ready() override
  {
    if ( !priest().buffs.voidform->check() && priest().resources.current[ RESOURCE_INSANITY ] >= insanity_required )
    {
      return priest_spell_t::ready();
    }
    else
    {
      return false;
    }
  }
};

struct dark_ascension_damage_t final : public priest_spell_t
{
  dark_ascension_damage_t( priest_t& p ) : priest_spell_t( "dark_ascension_damage", p, p.find_spell( 280800 ) )
  {
    background = true;

    // We don't want to lose insanity when casting it!
    base_costs[ RESOURCE_INSANITY ] = 0;

    may_miss = false;  // TODO: check
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    priest_spell_t::impact( s );
  }
};

struct dark_ascension_t final : public priest_spell_t
{
  dark_ascension_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "dark_ascension", p, p.find_talent_spell( "Dark Ascension" ) )
  {
    parse_options( options_str );

    impact_action = new dark_ascension_damage_t( p );
    add_child( impact_action );

    // We don't want to lose insanity when casting it!
    base_costs[ RESOURCE_INSANITY ] = 0;

    may_miss = false;
    aoe      = -1;
    radius   = data().effectN( 1 ).radius_max();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.voidform->expire();
    priest().generate_insanity( data().effectN( 2 ).percent(), priest().gains.insanity_dark_ascension,
                                execute_state->action );
    priest().buffs.voidform->trigger();
  }
};

struct void_torrent_t final : public priest_spell_t
{
  double insanity_gain;

  void_torrent_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "void_torrent", p, p.find_talent_spell( "Void Torrent" ) ),
      insanity_gain( p.talents.void_torrent->effectN( 3 ).trigger()->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    may_crit      = false;
    channeled     = true;
    use_off_gcd   = true;
    tick_zero     = true;
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data

    dot_duration = timespan_t::from_seconds( 4.0 );
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return timespan_t::from_seconds( 4.0 );
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    timespan_t t = base_tick_time;

    double h = priest().composite_spell_haste();

    t *= h;

    return t;
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );

    priest().buffs.void_torrent->expire();

    // When Void Torrent ends, restart the insanity drain tracking
    priest().insanity.begin_tracking();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.void_torrent->trigger();

    // Adjust the Voidform end event (essentially remove it) after the Void Torrent buff is up, since it disables
    // insanity drain for the duration of the channel
    priest().insanity.adjust_end_event();
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    // Partial tick is not counted for insanity gains
    if ( d->get_last_tick_factor() < 1.0 )
      return;

    priest().generate_insanity( insanity_gain, priest().gains.insanity_void_torrent, d->state->action );
  }

  bool ready() override
  {
    return priest_spell_t::ready();
  }
};
}  // namespace spells

namespace heals
{
struct mental_fortitude_t final : public priest_absorb_t
{
  mental_fortitude_t( priest_t& p ) : priest_absorb_t( "mental_fortitude", p, p.find_spell( 194018 ) )
  {
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_absorb_t::impact( s );
  }

  void assess_damage( dmg_e, action_state_t* s ) override
  {
    // Add stacking of buff value, and limit by players health factor.
    auto& buff            = target_specific[ s->target ];
    double stacked_amount = s->result_amount;
    if ( buff && buff->check() )
    {
      stacked_amount += buff->current_value;
    }
    double limit   = priest().resources.max[ RESOURCE_HEALTH ] * 0.08;
    stacked_amount = std::min( stacked_amount, limit );
    if ( sim->log )
    {
      sim->out_log.printf( "%s %s stacked amount: %.2f", player->name(), name(), stacked_amount );
    }

    // Trigger Absorb Buff
    if ( buff == nullptr )
    {
      buff = create_buff( s );
    }

    if ( result_is_hit( s->result ) )
    {
      buff->trigger( 1, stacked_amount );
      if ( sim->log )
      {
        sim->out_log.printf( "%s %s applies absorb on %s for %.0f (%.0f) (%s)", player->name(), name(),
                             s->target->name(), s->result_amount, stacked_amount,
                             util::result_type_string( s->result ) );
      }
    }

    stats->add_result( 0.0, s->result_total, ABSORB, s->result, s->block_result, s->target );
  }
};

}  // namespace heals

}  // namespace actions

namespace buffs
{
// ==========================================================================
// Custom insanity_drain_stacks buff
// ==========================================================================
struct insanity_drain_stacks_t final : public priest_buff_t<buff_t>
{
  struct stack_increase_event_t final : public player_event_t
  {
    propagate_const<insanity_drain_stacks_t*> ids;

    stack_increase_event_t( insanity_drain_stacks_t* s )
      : player_event_t( *s->player, timespan_t::from_seconds( 1.0 ) ), ids( s )
    {
    }

    const char* name() const override
    {
      return "insanity_drain_stack_increase";
    }

    void execute() override
    {
      auto priest = debug_cast<priest_t*>( player() );

      priest->insanity.drain();

      // If we are currently channeling Void Torrent or Dispersion, we don't gain stacks.
      if ( !( priest->buffs.void_torrent->check() || priest->buffs.dispersion->check() ) )
      {
        priest->buffs.insanity_drain_stacks->increment();
      }
      // Once the number of insanity drain stacks are increased, adjust the end-event to the new value
      priest->insanity.adjust_end_event();

      // Note, the drain() call above may have drained all insanity in very rare cases, in which case voidform is no
      // longer up. Only keep creating stack increase events if is up.
      if ( priest->buffs.voidform->check() )
      {
        ids->stack_increase = make_event<stack_increase_event_t>( sim(), ids );
      }
      // For some reason, as a shadow priest as long as you are not sitting you gain stacks of the VERS buffs
      // I think it is ties to resource gen, but you effectively get 100% uptime
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_VERSATILITY_RATING, priest );
    }
  };

  propagate_const<stack_increase_event_t*> stack_increase;

  insanity_drain_stacks_t( priest_t& p ) : base_t( p, "insanity_drain_stacks" ), stack_increase( nullptr )

  {
    set_max_stack( 1 );
    set_chance( 1.0 );
    set_duration( timespan_t::zero() );
    set_default_value( 1 );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    assert( stack_increase == nullptr );
    stack_increase = make_event<stack_increase_event_t>( *sim, this );
    return r;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    event_t::cancel( stack_increase );

    base_t::expire_override( expiration_stacks, remaining_duration );
  }

  void bump( int stacks, double /* value */ ) override
  {
    buff_t::bump( stacks, current_value + 1 );
    // current_value = value + 1;
  }

  void reset() override
  {
    base_t::reset();

    event_t::cancel( stack_increase );
  }
};

struct voidform_t final : public priest_buff_t<buff_t>
{
  voidform_t( priest_t& p ) : base_t( p, "voidform", p.find_spell( 194249 ) )
  {
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_HASTE );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    priest().buffs.insanity_drain_stacks->trigger();
    priest().buffs.shadowform->expire();
    priest().insanity.begin_tracking();

    return r;
  }

  bool freeze_stacks() override
  {
    // Hotfixed 2016-09-24: Voidform stacks no longer increase while Dispersion is active.
    if ( priest().buffs.dispersion->check() )
    {
      return true;
    }

    return base_t::freeze_stacks();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    priest().buffs.insanity_drain_stacks->expire();

    if ( priest().talents.lingering_insanity->ok() )
    {
      priest().buffs.lingering_insanity->expire();
      priest().buffs.lingering_insanity->trigger( expiration_stacks / 2 );
    }

    if ( priest().buffs.shadowform_state->check() )
    {
      priest().buffs.shadowform->trigger();
    }

    if ( priest().azerite.chorus_of_insanity.enabled() )
    {
      priest().buffs.chorus_of_insanity->expire();
      priest().buffs.chorus_of_insanity->trigger( expiration_stacks );
    }
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct lingering_insanity_t final : public priest_buff_t<buff_t>
{
  lingering_insanity_t( priest_t& p ) : base_t( p, "lingering_insanity", p.talents.lingering_insanity )
  {
    set_reverse( true );
    set_duration( timespan_t::from_seconds( 60 ) );
    set_period( timespan_t::from_seconds( p.talents.lingering_insanity->effectN( 2 ).base_value() ) );
    set_tick_behavior( buff_tick_behavior::REFRESH );
    set_tick_time_behavior( buff_tick_time_behavior::UNHASTED );
    set_max_stack( (int)(float)p.find_spell( 185916 )->effectN( 4 ).base_value() );  // or 18?
    add_invalidate( CACHE_HASTE );
  }

  void expire_override( int stacks, timespan_t ) override
  {
    if ( stacks <= 0 )
    {
      expire();
    }
  }
};
}  // namespace buffs

// Insanity Control Methods
void priest_t::generate_insanity( double num_amount, gain_t* g, action_t* action )
{
  if ( specialization() == PRIEST_SHADOW )
  {
    double amount                               = num_amount;
    double amount_from_surrender_to_madness     = 0.0;
    double amount_wasted_surrendered_to_madness = 0.0;

    if ( buffs.surrendered_to_madness->check() )
    {
      amount_wasted_surrendered_to_madness = amount * buffs.surrendered_to_madness->data().effectN( 1 ).percent();
      amount += amount_wasted_surrendered_to_madness;
    }
    else if ( buffs.surrender_to_madness->check() )
    {
      double total_amount = amount * ( 1.0 + talents.surrender_to_madness->effectN( 1 ).percent() );

      amount_from_surrender_to_madness = amount * talents.surrender_to_madness->effectN( 1 ).percent();

      // Make sure the maths line up.
      assert( total_amount == amount + amount_from_surrender_to_madness );
    }
    else if ( buffs.surrender_to_madness->check() )
    {
      amount_from_surrender_to_madness =
          ( amount * ( 1.0 + talents.surrender_to_madness->effectN( 1 ).percent() ) ) - amount;
    }

    insanity.gain( amount, g, action );

    if ( amount_from_surrender_to_madness > 0.0 )
    {
      insanity.gain( amount_from_surrender_to_madness, gains.insanity_surrender_to_madness, action );
    }
    if ( amount_wasted_surrendered_to_madness )
    {
      insanity.gain( amount_wasted_surrendered_to_madness, gains.insanity_wasted_surrendered_to_madness, action );
    }
  }
}

/// Simple insanity expiration event that kicks the actor out of Voidform
struct priest_t::insanity_end_event_t : public event_t
{
  priest_t& actor;

  insanity_end_event_t( priest_t& actor_, const timespan_t& duration_ )
    : event_t( *actor_.sim, duration_ ), actor( actor_ )
  {
  }

  void execute() override
  {
    if ( actor.sim->debug )
    {
      actor.sim->out_debug.printf( "%s insanity-track insanity-loss", actor.name() );
    }

    actor.buffs.voidform->expire();
    actor.insanity.end = nullptr;
  }
};

/**
 * Insanity tracking
 *
 * Handles the resource gaining from abilities, and insanity draining and manages an event that forcibly punts the
 * actor out of Voidform the exact moment insanity hitszero (millisecond resolution).
 */
priest_t::insanity_state_t::insanity_state_t( priest_t& a )
  : end( nullptr ),
    last_drained( timespan_t::zero() ),
    actor( a ),
    base_drain_per_sec( a.find_spell( 194249 )->effectN( 3 ).base_value() / -500.0 ),
    stack_drain_multiplier( 0.68 ),  // Hardcoded Patch 8.1 (2018-12-09)
    base_drain_multiplier( 1.0 )
{
}

/// Deferred init for actor dependent stuff not ready in the ctor
void priest_t::insanity_state_t::init()
{
}

/// Start the insanity drain tracking
void priest_t::insanity_state_t::set_last_drained()
{
  last_drained = actor.sim->current_time();
}

/// Start (or re-start) tracking of the insanity drain plus end event
void priest_t::insanity_state_t::begin_tracking()
{
  set_last_drained();
  adjust_end_event();
}

timespan_t priest_t::insanity_state_t::time_to_end() const
{
  return end ? end->remains() : timespan_t::zero();
}

void priest_t::insanity_state_t::reset()
{
  end          = nullptr;
  last_drained = timespan_t::zero();
}

/// Compute insanity drain per second with current state of the actor
double priest_t::insanity_state_t::insanity_drain_per_second() const
{
  if ( actor.buffs.voidform->check() == 0 )
  {
    return 0;
  }

  // Insanity does not drain during Dispersion
  if ( actor.buffs.dispersion->check() )
  {
    return 0;
  }

  // Insanity does not drain during Void Torrent
  if ( actor.buffs.void_torrent->check() )
  {
    return 0;
  }

  return base_drain_multiplier *
         ( base_drain_per_sec + ( actor.buffs.insanity_drain_stacks->current_value - 1 ) * stack_drain_multiplier );
}

/// Gain some insanity
void priest_t::insanity_state_t::gain( double value, gain_t* gain_obj, action_t* source_action = nullptr )
{
  // Drain before gaining, but don't adjust end-event yet
  drain();

  if ( actor.sim->debug )
  {
    auto current = actor.resources.current[ RESOURCE_INSANITY ];
    auto max     = actor.resources.max[ RESOURCE_INSANITY ];

    actor.sim->out_debug.printf(
        "%s insanity-track gain, value=%f, current=%.1f/%.1f, "
        "new=%.1f/%.1f",
        actor.name(), value, current, max, clamp( current + value, 0.0, max ), max );
  }

  actor.resource_gain( RESOURCE_INSANITY, value, gain_obj, source_action );

  // Explicitly adjust end-event after gaining some insanity
  adjust_end_event();
}

/**
 * Triggers the insanity drain, and is called in places that changes the insanity state of the actor in a relevant
 * way.
 * These are:
 * - Right before the actor decides to do something (scans APL for an ability to use)
 * - Right before insanity drain stack increases (every second)
 */
void priest_t::insanity_state_t::drain()
{
  double drain_per_second = insanity_drain_per_second();
  double drain_interval   = ( actor.sim->current_time() - last_drained ).total_seconds();

  // Don't drain if draining is disabled, or if we have already drained on this timestamp
  if ( drain_per_second == 0 || drain_interval == 0 )
  {
    return;
  }

  double drained = drain_per_second * drain_interval;
  // Ensure we always have enough to drain. This should always be true, since the drain is
  // always kept track of in relation to time.
#ifndef NDEBUG
  if ( actor.resources.current[ RESOURCE_INSANITY ] < drained )
  {
    actor.sim->errorf( "%s warning, insanity-track overdrain, current=%f drained=%f total=%f", actor.name(),
                       actor.resources.current[ RESOURCE_INSANITY ], drained,
                       actor.resources.current[ RESOURCE_INSANITY ] - drained );
    drained = actor.resources.current[ RESOURCE_INSANITY ];
  }
#else
  assert( actor.resources.current[ RESOURCE_INSANITY ] >= drained );
#endif

  if ( actor.sim->debug )
  {
    auto current = actor.resources.current[ RESOURCE_INSANITY ];
    auto max     = actor.resources.max[ RESOURCE_INSANITY ];

    actor.sim->out_debug.printf(
        "%s insanity-track drain, "
        "drain_per_second=%f, last_drained=%.3f, drain_interval=%.3f, "
        "current=%.1f/%.1f, new=%.1f/%.1f",
        actor.name(), drain_per_second, last_drained.total_seconds(), drain_interval, current, max,
        ( current - drained ), max );
  }

  // Update last drained, we're about to reduce the amount of insanity the actor has
  last_drained = actor.sim->current_time();

  actor.resource_loss( RESOURCE_INSANITY, drained, actor.gains.insanity_drain );
}

/**
 * Predict (with current state) when insanity is going to be fully depleted, and adjust (or create) an event for it.
 * Called in conjunction with insanity_state_t::drain(), after the insanity drain occurs (and potentially after a
 * relevant state change such as insanity drain stack buff increase occurs). */
void priest_t::insanity_state_t::adjust_end_event()
{
  double drain_per_second = insanity_drain_per_second();

  // Ensure that the current insanity level is correct
  if ( last_drained != actor.sim->current_time() )
  {
    drain();
  }

  // All drained, cancel voidform.
  if ( actor.resources.current[ RESOURCE_INSANITY ] == 0 && actor.options.priest_set_voidform_duration == 0 )
  {
    event_t::cancel( end );
    actor.buffs.voidform->expire();
    return;
  }
  else if ( actor.options.priest_set_voidform_duration > 0 &&
            actor.options.priest_set_voidform_duration < actor.buffs.voidform->stack() )
  {
    event_t::cancel( end );
    actor.buffs.voidform->expire();
    actor.resources.current[ RESOURCE_INSANITY ] = 0;
    return;
  }

  timespan_t seconds_left =
      drain_per_second ? timespan_t::from_seconds( actor.resources.current[ RESOURCE_INSANITY ] / drain_per_second )
                       : timespan_t::zero();

  if ( actor.sim->debug && drain_per_second > 0 && ( !end || ( end->remains() != seconds_left ) ) )
  {
    auto current = actor.resources.current[ RESOURCE_INSANITY ];
    auto max     = actor.resources.max[ RESOURCE_INSANITY ];

    actor.sim->out_debug.printf(
        "%s insanity-track adjust-end-event, "
        "drain_per_second=%f, insanity=%.1f/%.1f, seconds_left=%.3f, "
        "old_left=%.3f",
        actor.name(), drain_per_second, current, max, seconds_left.total_seconds(),
        end ? end->remains().total_seconds() : -1.0 );
  }

  // If we have no draining occurring, cancel the event.
  if ( drain_per_second == 0 )
  {
    event_t::cancel( end );
  }
  // We have no drain event yet, so make a new event that triggers the cancellation of Voidform.
  else if ( end == nullptr )
  {
    end = make_event<insanity_end_event_t>( *actor.sim, actor, seconds_left );
  }
  // Adjust existing event
  else
  {
    // New expiry time is sooner than the current insanity depletion event, create a new event with the new expiry
    // time.
    if ( seconds_left < end->remains() )
    {
      event_t::cancel( end );
      end = make_event<insanity_end_event_t>( *actor.sim, actor, seconds_left );
    }
    // End event is in the future, so just reschedule the current end event without creating a new one needlessly.
    else if ( seconds_left > end->remains() )
    {
      end->reschedule( seconds_left );
    }
  }
}

void priest_t::create_buffs_shadow()
{
  // Baseline

  buffs.shadowform = make_buff( this, "shadowform", find_class_spell( "Shadowform" ) )
                         ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.shadowform_state = make_buff( this, "shadowform_state" )->set_chance( 1.0 )->set_quiet( true );
  buffs.shadowy_insight  = make_buff( this, "shadowy_insight", talents.shadowy_insight->effectN( 1 ).trigger() )
                              ->set_trigger_spell( talents.shadowy_insight );
  buffs.voidform              = new buffs::voidform_t( *this );
  buffs.insanity_drain_stacks = new buffs::insanity_drain_stacks_t( *this );
  buffs.vampiric_embrace      = make_buff( this, "vampiric_embrace", find_class_spell( "Vampiric Embrace" ) );
  buffs.dispersion            = make_buff( this, "dispersion", find_class_spell( "Dispersion" ) );

  // Talents
  buffs.void_torrent         = make_buff( this, "void_torrent", talents.void_torrent );
  buffs.surrender_to_madness = make_buff( this, "surrender_to_madness", talents.surrender_to_madness )
                                   ->set_stack_change_callback( [this]( buff_t*, int, int after ) {
                                     if ( after == 0 )
                                       buffs.surrendered_to_madness->trigger();
                                   } );
  buffs.surrendered_to_madness = make_buff( this, "surrendered_to_madness", find_spell( 263406 ) );
  buffs.lingering_insanity     = new buffs::lingering_insanity_t( *this );

  // Azerite Powers
  buffs.chorus_of_insanity =
      make_buff<stat_buff_t>( this, "chorus_of_insanity", azerite.chorus_of_insanity.spell()->effectN( 1 ).trigger() )
          ->add_stat( STAT_CRIT_RATING, azerite.chorus_of_insanity.value( 1 ) )
          ->set_reverse( true )
          ->set_tick_behavior( buff_tick_behavior::REFRESH )
          ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
          ->set_period( timespan_t::from_seconds( 1 ) )
          ->add_invalidate( CACHE_CRIT_CHANCE )
          ->set_max_stack( 100 );

  buffs.harvested_thoughts = make_buff(
      this, "harvested_thoughts", azerite.thought_harvester.spell()->effectN( 1 ).trigger()->effectN( 1 ).trigger() );

  buffs.whispers_of_the_damned =
      make_buff( this, "whispers_of_the_damned", azerite.whispers_of_the_damned.spell()->effectN( 1 ).trigger() )
          ->set_duration( find_spell( 275726 )->duration() )
          ->set_max_stack( find_spell( 275726 )->max_stacks() );
}

void priest_t::init_rng_shadow()
{
  rppm.shadowy_insight    = get_rppm( "shadowy_insighty", talents.shadowy_insight );
  rppm.harvested_thoughts = get_rppm( "harvested_thoughts", azerite.thought_harvester.spell()->effectN( 1 ).trigger() );
}

void priest_t::init_spells_shadow()
{
  // Talents
  // T15
  talents.fortress_of_the_mind = find_talent_spell( "Fortress of the Mind" );
  talents.shadowy_insight      = find_talent_spell( "Shadowy Insight" );
  talents.shadow_word_void     = find_talent_spell( "Shadow Word: Void" );
  // T30
  talents.mania         = find_talent_spell( "Mania" );
  talents.body_and_soul = find_talent_spell( "Body and Soul" );
  talents.sanlayn       = find_talent_spell( "San'layn" );
  // T45
  talents.dark_void     = find_talent_spell( "Dark Void" );
  talents.misery        = find_talent_spell( "Misery" );
  talents.twist_of_fate = find_talent_spell( "Twist of Fate" );
  // T60
  talents.last_word      = find_talent_spell( "Last Word" );
  talents.mind_bomb      = find_talent_spell( "Mind Bomb" );
  talents.psychic_horror = find_talent_spell( "Psychic Horror" );
  // T75
  talents.auspicious_spirits = find_talent_spell( "Auspicious Spirits" );
  talents.shadow_word_death  = find_talent_spell( "Shadow Word: Death" );
  talents.shadow_crash       = find_talent_spell( "Shadow Crash" );
  // T90
  talents.lingering_insanity = find_talent_spell( "Lingering Insanity" );
  talents.mindbender         = find_talent_spell( "Mindbender" );
  talents.void_torrent       = find_talent_spell( "Void Torrent" );
  // T100
  talents.legacy_of_the_void   = find_talent_spell( "Legacy of the Void" );
  talents.dark_ascension       = find_talent_spell( "Dark Ascension" );
  talents.surrender_to_madness = find_talent_spell( "Surrender to Madness" );

  // General Spells
  specs.voidform            = find_specialization_spell( "Voidform" );
  specs.void_eruption       = find_specialization_spell( "Void Eruption" );
  specs.shadowy_apparitions = find_specialization_spell( "Shadowy Apparitions" );
  specs.shadow_priest       = find_specialization_spell( "Shadow Priest" );

  // Azerite
  azerite.sanctum                = find_azerite_spell( "Sanctum" );
  azerite.chorus_of_insanity     = find_azerite_spell( "Chorus of Insanity" );
  azerite.death_throes           = find_azerite_spell( "Death Throes" );
  azerite.depth_of_the_shadows   = find_azerite_spell( "Depth of the Shadows" );
  azerite.searing_dialogue       = find_azerite_spell( "Searing Dialogue" );
  azerite.spiteful_apparitions   = find_azerite_spell( "Spiteful Apparitions" );
  azerite.thought_harvester      = find_azerite_spell( "Thought Harvester" );
  azerite.torment_of_torments    = find_azerite_spell( "Torment of Torments" );
  azerite.whispers_of_the_damned = find_azerite_spell( "Whispers of the Damned" );

  base.distance = 27.0;
}

action_t* priest_t::create_action_shadow( const std::string& name, const std::string& options_str )
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
  if ( name == "shadow_word_death" )
  {
    return new shadow_word_death_t( *this, options_str );
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
  if ( name == "mind_bomb" )
  {
    return new mind_bomb_t( *this, options_str );
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
  if ( name == "dark_void" )
  {
    return new dark_void_t( *this, options_str );
  }
  if ( ( name == "mind_blast" ) || ( name == "shadow_word_void" ) )
  {
    return new mind_blast_t( *this, options_str );
  }
  if ( name == "dark_ascension" )
  {
    return new dark_ascension_t( *this, options_str );
  }

  return nullptr;
}

expr_t* priest_t::create_expression_shadow( const std::string& name_str )
{
  if ( name_str == "shadowy_apparitions_in_flight" )
  {
    return make_fn_expr( name_str, [this]() {
      if ( !active_spells.shadowy_apparitions )
      {
        return 0.0;
      }

      return static_cast<double>( active_spells.shadowy_apparitions->num_travel_events() );
    } );
  }

  else if ( name_str == "current_insanity_drain" )
  {
    // Current Insanity Drain for the next 1.0 sec.
    // Does not account for a new stack occurring in the middle and can be anywhere from 0.0 - 0.5 off the real value.
    return make_fn_expr( name_str, [this]() { return ( insanity.insanity_drain_per_second() ); } );
  }

  return nullptr;
}

void priest_t::generate_apl_shadow()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* cleave       = get_action_priority_list( "cleave" );
  action_priority_list_t* single       = get_action_priority_list( "single" );

  // On-Use Items
  for ( const std::string& item_action : get_item_actions() )
  {
    default_list->add_action( item_action );
  }

  // Professions
  for ( const std::string& profession_action : get_profession_actions() )
  {
    default_list->add_action( profession_action );
  }

  // Potions
  default_list->add_action(
      "potion,if=buff.bloodlust.react|target.time_to_die<=80|"
      "target.health.pct<35" );
  default_list->add_action(
      "variable,name=dots_up,op=set,value="
      "dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking" );

  // Racials
  // as of 7/3/2018 Arcane Torrent being on the GCD results in a DPS loss
  // if ( race == RACE_BLOOD_ELF )
  //     default_list->add_action(
  //         "arcane_torrent,if=prev_gcd.1.mindbender&buff.voidform.up" );
  if ( race == RACE_DARK_IRON_DWARF )
    default_list->add_action( "fireblood,if=buff.voidform.up" );
  if ( race == RACE_TROLL )
    default_list->add_action( "berserking" );
  if ( race == RACE_LIGHTFORGED_DRAENEI )
    default_list->add_action( "lights_judgment" );
  if ( race == RACE_MAGHAR_ORC )
    default_list->add_action( "ancestral_call,if=buff.voidform.up" );

  // Choose which APL to use based on talents and fight conditions.

  default_list->add_action( "run_action_list,name=cleave,if=active_enemies>1" );
  default_list->add_action( "run_action_list,name=single,if=active_enemies=1" );

  // single APL
  single->add_action( this, "Void Eruption" );
  single->add_talent( this, "Dark Ascension", "if=buff.voidform.down" );
  single->add_action( this, "Void Bolt" );
  single->add_action( this, "Mind Sear",
                      "if=buff.harvested_thoughts.up&cooldown.void_bolt.remains>=1.5&"
                      "azerite.searing_dialogue.rank>=1" );
  single->add_talent( this, "Shadow Word: Death",
                      "if=target.time_to_die<3|"
                      "cooldown.shadow_word_death.charges=2|"
                      "(cooldown.shadow_word_death.charges=1&"
                      "cooldown.shadow_word_death.remains<gcd.max)" );
  single->add_talent( this, "Surrender to Madness", "if=buff.voidform.stack>10+(10*buff.bloodlust.up)" );
  single->add_talent( this, "Dark Void", "if=raid_event.adds.in>10" );
  single->add_talent( this, "Mindbender", "if=talent.mindbender.enabled|(buff.voidform.stack>18|target.time_to_die<15)" );
  single->add_talent( this, "Shadow Word: Death",
                      "if=!buff.voidform.up|"
                      "(cooldown.shadow_word_death.charges=2&"
                      "buff.voidform.stack<15)" );
  single->add_talent( this, "Shadow Crash", "if=raid_event.adds.in>5&raid_event.adds.duration<20" );
  // Bank the Shadow Word: Void charges for a bit to try and avoid overcapping on Insanity.
  single->add_action( this, "Mind Blast",
                      "if=variable.dots_up&"
                      "((raid_event.movement.in>cast_time+0.5&raid_event.movement.in<4)|"
                      "!talent.shadow_word_void.enabled|buff.voidform.down|"
                      "buff.voidform.stack>14&(insanity<70|charges_fractional>1.33)|"
                      "buff.voidform.stack<=14&(insanity<60|charges_fractional>1.33))" );
  single->add_talent( this, "Void Torrent",
                      "if=dot.shadow_word_pain.remains>4&"
                      "dot.vampiric_touch.remains>4&buff.voidform.up" );
  single->add_action( this, "Shadow Word: Pain",
                      "if=refreshable&target.time_to_die>4&"
                      "!talent.misery.enabled&!talent.dark_void.enabled" );
  single->add_action( this, "Vampiric Touch",
                      "if=refreshable&target.time_to_die>6|"
                      "(talent.misery.enabled&"
                      "dot.shadow_word_pain.refreshable)" );
  single->add_action( this, "Mind Flay",
                      "chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&"
                      "(cooldown.void_bolt.up|cooldown.mind_blast.up)" );
  single->add_action( this, "Shadow Word: Pain" );

  // cleave APL
  cleave->add_action( this, "Void Eruption" );
  cleave->add_talent( this, "Dark Ascension", "if=buff.voidform.down" );
  cleave->add_action( this, "Vampiric Touch", "if=!ticking&azerite.thought_harvester.rank>=1" );
  cleave->add_action( this, "Mind Sear", "if=buff.harvested_thoughts.up" );
  cleave->add_action( this, "Void Bolt" );
  cleave->add_talent( this, "Shadow Word: Death", "target_if=target.time_to_die<3|buff.voidform.down" );
  cleave->add_talent( this, "Surrender to Madness", "if=buff.voidform.stack>10+(10*buff.bloodlust.up)" );
  cleave->add_talent( this, "Dark Void",
                      "if=raid_event.adds.in>10"
                      "&(dot.shadow_word_pain.refreshable|target.time_to_die>30)" );
  cleave->add_talent( this, "Mindbender" );
  cleave->add_action( this, "Mind Blast",
                      "target_if=spell_targets.mind_sear<variable.mind_blast_targets" );
  cleave->add_talent( this, "Shadow Crash",
                      "if=(raid_event.adds.in>5&raid_event.adds.duration<2)|"
                      "raid_event.adds.duration>2" );
  cleave->add_action( this, "Shadow Word: Pain",
                      "target_if=refreshable&target.time_to_die>"
                      "((-1.2+3.3*spell_targets.mind_sear)*variable.swp_trait_ranks_check*"
                      "(1-0.012*azerite.searing_dialogue.rank*spell_targets.mind_sear))"
                      ",if=!talent.misery.enabled" );
  cleave->add_action( this, "Vampiric Touch",
                      "target_if=refreshable,if=target.time_to_die>"
                      "((1+3.3*spell_targets.mind_sear)*variable.vt_trait_ranks_check*"
                      "(1+0.10*azerite.searing_dialogue.rank*spell_targets.mind_sear))" );
  cleave->add_action( this, "Vampiric Touch",
                      "target_if=dot.shadow_word_pain.refreshable"
                      ",if=(talent.misery.enabled&target.time_to_die>"
                      "((1.0+2.0*spell_targets.mind_sear)*variable.vt_mis_trait_ranks_check*"
                      "(variable.vt_mis_sd_check*spell_targets.mind_sear)))" );
  cleave->add_talent( this, "Void Torrent", "if=buff.voidform.up" );
  cleave->add_action( this, "Mind Sear",
                      "target_if=spell_targets.mind_sear>1,"
                      "chain=1,interrupt_immediate=1,interrupt_if=ticks>=2" );
  cleave->add_action( this, "Mind Flay",
                      "chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&"
                      "(cooldown.void_bolt.up|cooldown.mind_blast.up)" );
  cleave->add_action( this, "Shadow Word: Pain" );
}
}  // namespace priestspace

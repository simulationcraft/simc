#include "sc_priest.hpp"
#include "simulationcraft.hpp"

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

    if ( priest().artifact.from_the_shadows.rank() )
    {
      cooldown->duration = data().cooldown() + priest().artifact.from_the_shadows.time_value();
    }
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

public:
  mind_blast_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "mind_blast", player, player.find_class_spell( "Mind Blast" ) )
  {
    parse_options( options_str );
    is_sphere_of_insanity_spell = true;
    is_mastery_spell            = true;

    insanity_gain = data().effectN( 2 ).resource( RESOURCE_INSANITY );
    insanity_gain *= ( 1.0 + priest().talents.fortress_of_the_mind->effectN( 2 ).percent() );
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data

    spell_power_mod.direct *= 1.0 + player.talents.fortress_of_the_mind->effectN( 4 ).percent();

    if ( player.artifact.mind_shattering.rank() )
    {
      base_multiplier *= 1.0 + player.artifact.mind_shattering.percent();
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B2 ) )
      crit_bonus_multiplier *= 1.0 + priest().sets->set( PRIEST_SHADOW, T21, B2 )->effectN( 1 ).percent();
    background = ( priest().talents.shadow_word_void->ok() );
  }

  void init() override
  {
    if ( priest().active_items.mangazas_madness && priest().cooldowns.mind_blast->charges == 1 )
    {
      priest().cooldowns.mind_blast->charges +=
          priest().active_items.mangazas_madness->driver()->effectN( 1 ).base_value();
    }
    priest().cooldowns.mind_blast->hasted = true;

    priest_spell_t::init();
  }

  void schedule_execute( action_state_t* s ) override
  {
    priest_spell_t::schedule_execute( s );

    priest().buffs.shadowy_insight->expire();
  }

  virtual double composite_crit_chance() const override
  {
    double c = priest_spell_t::composite_crit_chance();

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B4 ) && priest().buffs.overwhelming_darkness->check() )
    {
      c += ( priest().buffs.overwhelming_darkness->check() ) *
           ( priest().buffs.overwhelming_darkness->data().effectN( 1 ).percent() / 2.0 );
    }

    return c;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.power_overwhelming->trigger();
    priest().buffs.empty_mind->up();  // benefit tracking
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( priest().buffs.empty_mind->check() )
    {
      d *= 1.0 + ( priest().buffs.empty_mind->check() * priest().buffs.empty_mind->data().effectN( 1 ).percent() );
    }

    return d;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    double temp_gain = insanity_gain + ( priest().buffs.empty_mind->stack() *
                                         priest().buffs.empty_mind->data().effectN( 2 ).percent() );

    priest().generate_insanity( temp_gain, priest().gains.insanity_mind_blast, s->action );
    priest().buffs.empty_mind->expire();
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
      cd += -timespan_t::from_seconds( 1.5 );  // still hardcoded N1gh7h4wk 2018/01/26
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

struct shadow_word_void_t final : public priest_spell_t
{
  // TODO: Make sure that this replaces Mind Blast somehow
private:
  double insanity_gain;

public:
  shadow_word_void_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "shadow_word_void", player, player.find_talent_spell( "Shadow Word: Void" ) )
  {
    parse_options( options_str );
    is_sphere_of_insanity_spell = true;
    is_mastery_spell            = true;

    insanity_gain = data().effectN( 2 ).resource( RESOURCE_INSANITY );
    insanity_gain *= ( 1.0 + priest().talents.fortress_of_the_mind->effectN( 2 ).percent() );
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data

    spell_power_mod.direct *= 1.0 + player.talents.fortress_of_the_mind->effectN( 4 ).percent();

    if ( player.artifact.mind_shattering.rank() )
    {
      base_multiplier *= 1.0 + player.artifact.mind_shattering.percent();
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B2 ) )
      crit_bonus_multiplier *= 1.0 + priest().sets->set( PRIEST_SHADOW, T21, B2 )->effectN( 1 ).percent();
  }

  void init() override
  {
    if ( priest().active_items.mangazas_madness )  // FIXME is this check needed? &&
                                                   // priest().cooldowns.shadow_word_void->charges < 1)
    {
      priest().cooldowns.shadow_word_void->charges +=
          priest().active_items.mangazas_madness->driver()->effectN( 1 ).base_value();
    }
    priest().cooldowns.shadow_word_void->hasted = true;

    priest_spell_t::init();
  }

  void schedule_execute( action_state_t* s ) override
  {
    priest_spell_t::schedule_execute( s );

    priest().buffs.shadowy_insight->expire();
  }

  virtual double composite_crit_chance() const override
  {
    double c = priest_spell_t::composite_crit_chance();

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B4 ) && priest().buffs.overwhelming_darkness->check() )
    {
      c += ( priest().buffs.overwhelming_darkness->check() ) *
           ( priest().buffs.overwhelming_darkness->data().effectN( 1 ).percent() / 2.0 );
    }

    return c;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.power_overwhelming->trigger();
    priest().buffs.overwhelming_darkness->up();  // benefit tracking
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( priest().buffs.empty_mind->check() )
    {
      d *= 1.0 + ( priest().buffs.empty_mind->check() * priest().buffs.empty_mind->data().effectN( 1 ).percent() );
    }

    return d;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    double temp_gain = insanity_gain + ( priest().buffs.empty_mind->stack() *
                                         priest().buffs.empty_mind->data().effectN( 2 ).percent() );

    priest().generate_insanity( temp_gain, priest().gains.insanity_shadow_word_void, s->action );
    priest().buffs.empty_mind->expire();
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

  // TODO: Mind Sear is missing damage information in spell data
  mind_sear_tick_t( priest_t& p, const spell_data_t* mind_sear )
    : priest_spell_t( "mind_sear_tick", p, mind_sear->effectN( 1 ).trigger() ),
      insanity_gain( p.find_spell( 208232 )->effectN( 1 ).percent() )
  {
    background    = true;
    dual          = true;
    aoe           = -1;
    callbacks     = false;
    direct_tick   = true;
    use_off_gcd   = true;
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_mind_sear, s->action );
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

    if ( p.artifact.void_corruption.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.void_corruption.percent();
    }

    tick_action = new mind_sear_tick_t( p, p.find_class_spell( "Mind Sear" ) );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
  }
};

struct new_void_tendril_mind_flay_t final : public priest_spell_t
{
  bool* active_flag;

  new_void_tendril_mind_flay_t( priest_t& p ) : priest_spell_t( "mind_flay_void_tendril", p, p.find_spell( 193473 ) )

  {
    aoe                    = 1;
    radius                 = 100;
    dot_duration           = timespan_t::zero();
    base_tick_time         = timespan_t::zero();
    background             = true;
    ground_aoe             = true;
    school                 = SCHOOL_SHADOW;
    may_miss               = false;
    may_crit               = true;
    spell_power_mod.direct = spell_power_mod.tick;
    spell_power_mod.tick   = 0.0;
    snapshot_flags &= ~( STATE_MUL_PERSISTENT | STATE_TGT_MUL_DA );
    update_flags &= ~( STATE_MUL_PERSISTENT | STATE_TGT_MUL_DA );
    //  dot_duration = timespan_t::from_seconds(10.0);
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    return timespan_t::from_seconds( data().effectN( 1 ).base_value() );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    return priest().composite_player_pet_damage_multiplier( state );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().artifact.lash_of_insanity.rank() )
    {
      priest().generate_insanity( priest().find_spell( 240843 )->effectN( 1 ).percent(),
                                  priest().gains.insanity_call_to_the_void, s->action );
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

    may_crit                    = false;
    channeled                   = true;
    hasted_ticks                = false;
    use_off_gcd                 = true;
    is_sphere_of_insanity_spell = true;
    is_mastery_spell            = true;
    energize_type               = ENERGIZE_NONE;  // disable resource generation from spell data

    priest().active_spells.void_tendril = new new_void_tendril_mind_flay_t( p );
    add_child( priest().active_spells.void_tendril );

    if ( p.artifact.void_siphon.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.void_siphon.percent();
    }

    spell_power_mod.tick *= 1.0 + p.talents.fortress_of_the_mind->effectN( 3 ).percent();

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B2 ) )
      crit_bonus_multiplier *= 1.0 + ( priest().sets->set( PRIEST_SHADOW, T21, B2 )->effectN( 1 ).percent() );
  }

  virtual double composite_crit_chance() const override
  {
    double c = priest_spell_t::composite_crit_chance();

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B4 ) && priest().buffs.overwhelming_darkness->check() )
    {
      c += ( priest().buffs.overwhelming_darkness->check() ) *
           ( priest().buffs.overwhelming_darkness->data().effectN( 1 ).percent() / 2.0 );
    }

    return c;
  }

  /// Legendary the_twins_painful_touch
  void spread_twins_painsful_dots( action_state_t* s )
  {
    auto td = find_td( s->target );
    if ( !td )
    {
      // If we do not have targetdata, the target does not have any dots. Bail out.
      return;
    }

    std::array<const dot_t*, 2> dots = {{td->dots.shadow_word_pain, td->dots.vampiric_touch}};

    // First check if there is even a dot active, otherwise we can bail out as well.
    if ( range::find_if( dots, []( const dot_t* d ) { return d->remains() > timespan_t::zero(); } ) == dots.end() )
    {
      if ( sim->debug )
      {
        sim->out_debug.printf(
            "%s %s will not spread the_twins_painful_touch dots because no dot "
            "is on the target.",
            priest().name(), name() );
      }
      return;
    }

    // Now find 2 targets to spread dots to.
    double max_distance  = 10.0;
    unsigned max_targets = 2;
    std::vector<player_t*> valid_targets;
    range::remove_copy_if(
        sim->target_list.data(), std::back_inserter( valid_targets ),
        [s, max_distance]( const player_t* p ) { return s->target->get_player_distance( *p ) > max_distance; } );
    // Just cut off to max_targets. No special selection.
    if ( valid_targets.size() > max_targets )
    {
      valid_targets.resize( max_targets );
    }
    if ( sim->debug )
    {
      std::string targets;
      sim->out_debug.printf( "%s %s selected targets for the_twins_painful_touch dots: %s.", priest().name(), name(),
                             targets.c_str() );
    }

    // spread dots to targets
    for ( const dot_t* dot : dots )
    {
      for ( player_t* target : valid_targets )
      {
        dot->copy( target, DOT_COPY_CLONE );
      }
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T20, B2 ) )
    {
      priest().buffs.empty_mind->trigger();
    }

    priest().trigger_call_to_the_void( d );

    priest().generate_insanity( insanity_gain, priest().gains.insanity_mind_flay, d->state->action );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    if ( priest().buffs.the_twins_painful_touch->up() )
    {
      spread_twins_painsful_dots( s );
      priest().buffs.the_twins_painful_touch->expire();
    }
  }
};

struct shadow_word_death_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_word_death_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_word_death", p, p.talents.shadow_word_death ),
      insanity_gain( p.find_spell( 32379 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    const spell_data_t* shadow_word_death_2 = p.find_specialization_spell( 231689 );
    if ( shadow_word_death_2 )
    {
      cooldown->charges += shadow_word_death_2->effectN( 1 ).base_value();
    }
    if ( p.artifact.deaths_embrace.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.deaths_embrace.percent();
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( priest().buffs.zeks_exterminatus->check() )
    {
      d *= 1.0 + priest().buffs.zeks_exterminatus->data().effectN( 1 ).trigger()->effectN( 2 ).percent();
    }

    return d;
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest().buffs.zeks_exterminatus->up();  // benefit tracking
    priest().buffs.zeks_exterminatus->expire();
  }

  void impact( action_state_t* s ) override
  {
    double total_insanity_gain    = 0.0;
    double save_health_percentage = s->target->health_percentage();

    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // http://us.battle.net/wow/en/forum/topic/20743676204#3 - 2016-05-12
      // SWD always grants at least 10 Insanity.
      // TODO: Add in a custom buff that checks after 1 second to see if the target SWD was cast on is now dead.
      // TODO: Check in beta if the target is dead vs. SWD is the killing blow.
      total_insanity_gain = data().effectN( 3 ).base_value();

      if ( ( ( save_health_percentage > 0.0 ) && ( s->target->health_percentage() <= 0.0 ) ) )
      {
        total_insanity_gain = insanity_gain;
      }

      priest().generate_insanity( total_insanity_gain, priest().gains.insanity_shadow_word_death, s->action );
    }
  }

  bool ready() override
  {
    if ( !priest_spell_t::ready() )
      return false;

    if ( priest().buffs.zeks_exterminatus->up() )
    {
      return true;
    }

    if ( target->health_percentage() < as<double>( data().effectN( 2 ).base_value() ) )
    {
      return true;
    }

    return false;
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

    const spell_data_t* missile = priest().find_spell( 205386 );
    school                      = missile->get_school_type();
    spell_power_mod.direct      = missile->effectN( 1 ).sp_coeff();
    aoe                         = -1;
    radius                      = data().effectN( 1 ).radius();

    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().generate_insanity( insanity_gain, priest().gains.insanity_shadow_crash, execute_state->action );
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
  }
};

struct sphere_of_insanity_spell_t final : public priest_spell_t
{
  sphere_of_insanity_spell_t( priest_t& p ) : priest_spell_t( "sphere_of_insanity", p, p.find_spell( 194182 ) )
  {
    may_crit    = false;
    background  = true;
    proc        = false;
    callbacks   = true;
    may_miss    = false;
    aoe         = -1;
    range       = 0.0;
    radius      = 100.0;
    trigger_gcd = timespan_t::zero();
    school      = SCHOOL_SHADOW;
  }

  std::vector<player_t*>& target_list() const
  {
    // Check if target cache is still valid. If not, recalculate it
    if ( !target_cache.is_valid )
    {
      std::vector<player_t*> targets;
      range::for_each( sim->target_non_sleeping_list, [&targets, this]( player_t* t ) {
        const priest_td_t* td = find_td( t );
        if ( td && td->dots.shadow_word_pain->is_ticking() )
        {
          targets.push_back( t );
        }
      } );
      target_cache.list.swap( targets );
      target_cache.is_valid = true;
    }

    return target_cache.list;
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

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    if ( target->type == ENEMY_ADD || target->level() < sim->max_player_level + 3 )
    {
      priest().trigger_sephuzs_secret( state, MECHANIC_SILENCE );
    }
    else
    {
      priest().trigger_sephuzs_secret( state, MECHANIC_INTERRUPT );
    }
  }

  bool ready() override
  {
    return priest_spell_t::ready();
    // Only available if the target is casting
    // Or if the target can get blank silenced
    if ( !( target->type != ENEMY_ADD && ( target->level() < sim->max_player_level + 3 ) && target->debuffs.casting &&
            target->debuffs.casting->check() ) )
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

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    priest().trigger_sephuzs_secret( state, MECHANIC_STUN );
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

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    priest().trigger_sephuzs_secret( state, MECHANIC_STUN );
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
      return false;

    return priest_spell_t::ready();
  }
};

struct shadowy_apparition_spell_t final : public priest_spell_t
{
  double insanity_gain;

  shadowy_apparition_spell_t( priest_t& p )
    : priest_spell_t( "shadowy_apparitions", p, p.find_spell( 78203 ) ),
      insanity_gain( priest().talents.auspicious_spirits->effectN( 2 ).percent() )
  {
    background                   = true;
    proc                         = false;
    callbacks                    = true;
    may_miss                     = false;
    trigger_gcd                  = timespan_t::zero();
    travel_speed                 = 6.0;
    const spell_data_t* dmg_data = p.find_spell( 148859 );  // Hardcoded into tooltip 2014/06/01

    parse_effect_data( dmg_data->effectN( 1 ) );
    school = SCHOOL_SHADOW;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().talents.auspicious_spirits->ok() )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_auspicious_spirits, s->action );
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    d *= 1.0 + priest().talents.auspicious_spirits->effectN( 1 ).percent();

    return d;
  }

  /** Trigger a shadowy apparition */
  void trigger()
  {
    if ( priest().sim->debug )
      priest().sim->out_debug << priest().name() << " triggered shadowy apparition.";

    priest().procs.shadowy_apparition->occur();
    schedule_execute();
  }
};

// ==========================================================================
// Shadow Word: Pain
// ==========================================================================
struct shadow_word_pain_t final : public priest_spell_t
{
  double insanity_gain;
  bool casted;

  shadow_word_pain_t( priest_t& p, const std::string& options_str, bool _casted = true )
    : priest_spell_t( "shadow_word_pain", p, p.find_class_spell( "Shadow Word: Pain" ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );
    casted           = _casted;
    may_crit         = true;
    tick_zero        = false;
    is_mastery_spell = true;
    if ( !casted )
    {
      base_dd_max = 0.0;
      base_dd_min = 0.0;
    }
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data

    if ( p.artifact.to_the_pain.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.to_the_pain.percent();
    }

    if ( priest().specs.shadowy_apparitions->ok() && !priest().active_spells.shadowy_apparitions )
    {
      priest().active_spells.shadowy_apparitions = new shadowy_apparition_spell_t( p );
      if ( !priest().artifact.unleash_the_shadows.rank() )
      {
        // If SW:P is the only action having SA, then we can add it as a child stat.
        add_child( priest().active_spells.shadowy_apparitions );
      }
    }

    if ( priest().artifact.sphere_of_insanity.rank() && !priest().active_spells.sphere_of_insanity )
    {
      priest().active_spells.sphere_of_insanity = new sphere_of_insanity_spell_t( p );
      add_child( priest().active_spells.sphere_of_insanity );
    }
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    return casted ? priest_spell_t::spell_direct_power_coefficient( s ) : 0.0;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( casted )
    {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_shadow_word_pain_onhit, s->action );
    }

    if ( priest().active_items.zeks_exterminatus )
    {
      trigger_zeks();
    }
    if ( priest().artifact.sphere_of_insanity.rank() )
    {
      priest().active_spells.sphere_of_insanity->target_cache.is_valid = false;
    }
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );
    if ( priest().artifact.sphere_of_insanity.rank() )
    {
      priest().active_spells.sphere_of_insanity->target_cache.is_valid = false;
    }
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
      }
    }

    if ( priest().active_items.anunds_seared_shackles )
    {
      trigger_anunds();
    }

    if ( priest().active_items.zeks_exterminatus )
    {
      trigger_zeks();
    }
  }

  double cost() const override
  {
    double c = priest_spell_t::cost();

    if ( priest().specialization() == PRIEST_SHADOW )
      return 0.0;

    return c;
  }

  double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest().artifact.mass_hysteria.rank() )
    {
      m *= 1.0 + ( priest().buffs.voidform->check() * ( priest().artifact.mass_hysteria.percent() ) );
    }

    return m;
  }
};

struct vampiric_touch_t final : public priest_spell_t
{
  double insanity_gain;
  propagate_const<shadow_word_pain_t*> child_swp;
  bool ignore_healing = false;

  vampiric_touch_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "vampiric_touch", p, p.find_class_spell( "Vampiric Touch" ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) ),
      ignore_healing( p.options.priest_ignore_healing )
  {
    parse_options( options_str );
    if ( !ignore_healing )
      init_mental_fortitude();

    may_crit         = false;
    is_mastery_spell = true;

    if ( priest().talents.misery->ok() )
    {
      child_swp             = new shadow_word_pain_t( priest(), std::string( "" ), false );
      child_swp->background = true;
    }
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data

    if ( p.artifact.touch_of_darkness.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.touch_of_darkness.percent();
    }

    if ( !priest().active_spells.shadowy_apparitions && priest().specs.shadowy_apparitions->ok() &&
         p.artifact.unleash_the_shadows.rank() )
    {
      priest().active_spells.shadowy_apparitions = new shadowy_apparition_spell_t( p );
    }
  }
  void init_mental_fortitude();

  void trigger_heal( action_state_t* s )
  {
    if ( ignore_healing )
      return;

    double amount_to_heal = s->result_amount * data().effectN( 2 ).m_value();

    if ( priest().active_items.zenkaram_iridis_anadem )
    {
      amount_to_heal *= 1.0 + priest().active_items.zenkaram_iridis_anadem->driver()->effectN( 1 ).percent();
    }
    double actual_amount =
        priest().resource_gain( RESOURCE_HEALTH, amount_to_heal, priest().gains.vampiric_touch_health );
    double overheal = amount_to_heal - actual_amount;
    if ( priest().active_spells.mental_fortitude && overheal > 0.0 )
    {
      priest().active_spells.mental_fortitude->pre_execute_state =
          priest().active_spells.mental_fortitude->get_state( s );
      priest().active_spells.mental_fortitude->base_dd_min = priest().active_spells.mental_fortitude->base_dd_max =
          overheal;
      priest().active_spells.mental_fortitude->target = &priest();
      priest().active_spells.mental_fortitude->execute();
    }
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
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( !ignore_healing )
    {
      trigger_heal( d->state );
    }

    if ( priest().artifact.unleash_the_shadows.rank() )
    {
      if ( priest().active_spells.shadowy_apparitions && ( d->state->result_amount > 0 ) )
      {
        if ( d->state->result == RESULT_CRIT && rng().roll( priest().artifact.unleash_the_shadows.percent() ) )
        {
          priest().active_spells.shadowy_apparitions->trigger();
        }
      }
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T19, B2 ) )
    {
      priest().generate_insanity( priest().sets->set( PRIEST_SHADOW, T19, B2 )->effectN( 1 ).base_value(),
                                  priest().gains.insanity_vampiric_touch_ondamage, d->state->action );
    }

    if ( priest().active_items.anunds_seared_shackles )
    {
      trigger_anunds();
    }
  }

  double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest().artifact.mass_hysteria.rank() )
    {
      m *= 1.0 + ( priest().buffs.voidform->check() * ( priest().artifact.mass_hysteria.percent() ) );
    }

    return m;
  }
};

struct void_bolt_t final : public priest_spell_t
{
  struct void_bolt_extension_t : public priest_spell_t
  {
    const spell_data_t* rank2;
    int dot_extension;

    void_bolt_extension_t( priest_t& player )
      : priest_spell_t( "void_bolt_extension", player ), rank2( player.find_specialization_spell( 231688 ) )
    {
      dot_extension = rank2->effectN( 1 ).base_value();
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

      if ( rank2->ok() )
      {
        if ( priest_td_t* td = find_td( s->target ) )
        {
          if ( td->dots.shadow_word_pain->is_ticking() )
          {
            td->dots.shadow_word_pain->extend_duration( timespan_t::from_millis( dot_extension ), true );
          }

          if ( td->dots.vampiric_touch->is_ticking() )
          {
            td->dots.vampiric_touch->extend_duration( timespan_t::from_millis( dot_extension ), true );
          }
        }
      }
    }
  };

  double insanity_gain;
  const spell_data_t* rank2;
  void_bolt_extension_t* void_bolt_extension;

  void_bolt_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "void_bolt", player, player.find_spell( 205448 ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) ),
      rank2( player.find_specialization_spell( 231688 ) )
  {
    parse_options( options_str );
    use_off_gcd                 = true;
    is_sphere_of_insanity_spell = true;
    is_mastery_spell            = true;
    energize_type               = ENERGIZE_NONE;  // disable resource generation from spell data.

    if ( player.artifact.sinister_thoughts.rank() )
    {
      base_multiplier *= 1.0 + player.artifact.sinister_thoughts.percent();
    }

    cooldown->hasted = true;

    void_bolt_extension = new void_bolt_extension_t( player );
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().generate_insanity( insanity_gain, priest().gains.insanity_void_bolt, execute_state->action );

    priest().buffs.anunds_last_breath->up();
    priest().buffs.anunds_last_breath->expire();

    if ( priest().buffs.void_vb->up() )
    {
      cooldown->reset( false );
    }
  }

  virtual double composite_crit_chance() const override
  {
    double c = priest_spell_t::composite_crit_chance();

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B4 ) && priest().buffs.overwhelming_darkness->check() )
    {
      c += ( priest().buffs.overwhelming_darkness->check() ) *
           ( priest().buffs.overwhelming_darkness->data().effectN( 1 ).percent() / 2.0 );
    }

    return c;
  }

  bool ready() override
  {
    if ( !( priest().buffs.voidform->check() ) )
      return false;

    return priest_spell_t::ready();
  }

  double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest().buffs.anunds_last_breath->check() )
    {
      m *= 1.0 + ( priest().buffs.anunds_last_breath->data().effectN( 1 ).percent() *
                   priest().buffs.anunds_last_breath->check() );
    }
    return m;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( rank2->ok() )
    {
      void_bolt_extension->target = s->target;
      void_bolt_extension->schedule_execute();
    }
  }
};

struct void_eruption_t final : public priest_spell_t
{
  propagate_const<shadow_word_pain_t*> child_swp;
  propagate_const<action_t*> void_bolt;

  void_eruption_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "void_eruption", p, p.find_spell( 228360 ) ), void_bolt( nullptr )
  {
    parse_options( options_str );
    base_costs[ RESOURCE_INSANITY ] = 0.0;
    if ( priest().talents.dark_void->ok() )
    {
      child_swp             = new shadow_word_pain_t( priest(), std::string( "" ), false );
      child_swp->background = true;
    }

    may_miss          = false;
    is_mastery_spell  = true;
    aoe               = -1;
    range             = 40.0;
    radius            = 10.0;
    school            = SCHOOL_SHADOW;
    cooldown          = priest().cooldowns.void_bolt;
    base_execute_time = p.find_spell( 228260 )->cast_time( p.true_level ) *
                        ( 1.0 + p.talents.legacy_of_the_void->effectN( 3 ).percent() );
  }

  double spell_direct_power_coefficient( const action_state_t* ) const override
  {
    return priest().find_spell( 228360 )->effectN( 1 ).sp_coeff();
  }

  void init() override
  {
    priest_spell_t::init();
    void_bolt = player->find_action( "void_bolt" );
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.voidform->trigger();
    priest().cooldowns.void_bolt->reset( true );

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T19, B4 ) )
    {
      priest().buffs.void_vb->trigger();
    }
    else
    {
      priest().cooldowns.void_bolt->start( void_bolt );
      priest().cooldowns.void_bolt->adjust(
          -timespan_t::from_millis( 1000 * ( 3.0 * priest().composite_spell_speed() ) ), true );
    }

    if ( priest().active_items.mother_shahrazs_seduction )
    {
      int mss_vf_stacks = priest().active_items.mother_shahrazs_seduction->driver()->effectN( 1 ).base_value();

      priest().buffs.voidform->bump( mss_vf_stacks - 1 );  // You start with 3 Stacks of Voidform 2017/01/17
      if ( priest().buffs.overwhelming_darkness->check() )
      {
        priest().buffs.overwhelming_darkness->bump( mss_vf_stacks - 1 );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    priest_spell_t::impact( s );

    if ( priest().talents.dark_void->ok() )
    {
      child_swp->target = s->target;
      child_swp->execute();
    }
  }
  // TODO: Healing part of HotV
  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( priest().active_items.heart_of_the_void )
    {
      d *= 1.0 + ( priest().active_items.heart_of_the_void->driver()->effectN( 1 ).percent() );
    }

    return d;
  }

  bool ready() override
  {
    if ( !priest().buffs.voidform->check() &&
         ( priest().resources.current[ RESOURCE_INSANITY ] >=
               priest().find_spell( 185916 )->effectN( 4 ).base_value() ||
           ( priest().talents.legacy_of_the_void->ok() &&
             ( priest().resources.current[ RESOURCE_INSANITY ] >=
               priest().find_spell( 185916 )->effectN( 4 ).base_value() +
                   priest().talents.legacy_of_the_void->effectN( 2 ).base_value() ) ) ) )
    {
      return priest_spell_t::ready();
    }
    else
    {
      return false;
    }
  }
};

struct void_torrent_t final : public priest_spell_t
{
  void_torrent_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "void_torrent", p, p.talents.void_torrent )
  {
    parse_options( options_str );

    may_crit    = false;
    channeled   = true;
    use_off_gcd = true;
    tick_zero   = true;

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

    if ( priest().artifact.mind_quickening.rank() )
    {
      priest().buffs.mind_quickening->trigger();
    }

    // Adjust the Voidform end event (essentially remove it) after the Void Torrent buff is up, since it disables
    // insanity drain for the duration of the channel
    priest().insanity.adjust_end_event();
  }

  bool ready() override
  {
    if ( !priest().buffs.voidform->check() )
    {
      return false;
    }

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
      buff = create_buff( s );

    if ( result_is_hit( s->result ) )
    {
      buff->trigger( 1, stacked_amount );
      if ( sim->log )
        sim->out_log.printf( "%s %s applies absorb on %s for %.0f (%.0f) (%s)", player->name(), name(),
                             s->target->name(), s->result_amount, stacked_amount,
                             util::result_type_string( s->result ) );
    }

    stats->add_result( 0.0, s->result_total, ABSORB, s->result, s->block_result, s->target );
  }
};

}  // namespace heals

void spells::vampiric_touch_t::init_mental_fortitude()
{
  if ( !priest().active_spells.mental_fortitude && priest().artifact.mental_fortitude.rank() )
  {
    priest().active_spells.mental_fortitude = new heals::mental_fortitude_t( priest() );
  }
}
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
    }
  };

  propagate_const<stack_increase_event_t*> stack_increase;

  insanity_drain_stacks_t( priest_t& p ) : base_t( &p, "insanity_drain_stacks" ), stack_increase( nullptr )

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

struct overwhelming_darkness_t final : public priest_buff_t<stat_buff_t>
{
  overwhelming_darkness_t( priest_t& p ) : base_t( &p, "overwhelming_darkness", p.find_spell( 252909 ) )
  {
    set_max_stack( 100 );
    set_chance( p.sets->has_set_bonus( PRIEST_SHADOW, T21, B4 ) );
    set_period( timespan_t::from_seconds( 1 ) );
    set_duration( timespan_t::zero() );
    set_refresh_behavior( BUFF_REFRESH_DURATION );
    add_invalidate( CACHE_CRIT_CHANCE );
  }

  bool freeze_stacks() override
  {
    if ( priest().buffs.dispersion->check() || !priest().buffs.voidform->check() )
      return true;

    return base_t::freeze_stacks();
  }
};

struct voidform_t final : public priest_buff_t<haste_buff_t>
{
  voidform_t( priest_t& p ) : base_t( &p, "voidform", p.find_spell( 194249 ) )
  {
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_HASTE );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    priest().buffs.insanity_drain_stacks->trigger();
    priest().buffs.the_twins_painful_touch->trigger();
    priest().buffs.iridis_empowerment->trigger();
    priest().buffs.shadowform->expire();
    priest().insanity.begin_tracking();
    if ( priest().artifact.sphere_of_insanity.rank() )
    {
      priest().buffs.sphere_of_insanity->trigger();
    }

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B4 ) )
    {
      priest().buffs.overwhelming_darkness->trigger();
    }

    return r;
  }

  bool freeze_stacks() override
  {
    // Hotfixed 2016-09-24: Voidform stacks no longer increase while Dispersion is active.
    if ( priest().buffs.dispersion->check() )
      return true;

    return base_t::freeze_stacks();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    priest().buffs.insanity_drain_stacks->expire();

    if ( priest().talents.lingering_insanity->ok() )
      priest().buffs.lingering_insanity->trigger( expiration_stacks );

    priest().buffs.the_twins_painful_touch->expire();

    priest().buffs.iridis_empowerment->expire();

    if ( priest().buffs.shadowform_state->check() )
    {
      priest().buffs.shadowform->trigger();
    }

    priest().buffs.sphere_of_insanity->expire();

    if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T21, B4 ) )
    {
      priest().buffs.overwhelming_darkness->expire();
    }

    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct surrender_to_madness_t final : public priest_buff_t<buff_t>
{
  surrender_to_madness_t( priest_t& p ) : base_t( &p, "surrender_to_madness", p.talents.surrender_to_madness )
  {
  }
  void expire_override( int stacks, timespan_t remaining_duration ) override
  {
    priest().buffs.surrender_to_madness_death->trigger();
  }
};

struct lingering_insanity_t final : public priest_buff_t<haste_buff_t>
{
  int hidden_lingering_insanity;

  lingering_insanity_t( priest_t& p )
    : base_t( &p, "lingering_insanity", p.talents.lingering_insanity ), hidden_lingering_insanity( 0 )
  {
    set_reverse( true );
    set_duration( timespan_t::from_seconds( 50 ) );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_behavior( BUFF_TICK_REFRESH );
    set_tick_time_behavior( BUFF_TICK_TIME_UNHASTED );
    set_max_stack( p.find_spell( 185916 )->effectN( 4 ).base_value() );  // or 18?

    // Calculate the amount of stacks lost per second based on the amount of haste lost per second
    // divided by the amount of haste gained per Voidform stack
    hidden_lingering_insanity = p.find_spell( 199849 )->effectN( 1 ).base_value() /
                                ( p.find_spell( 228264 )->effectN( 2 ).base_value() / 10.0 );
  }

  void decrement( int, double ) override
  {
    buff_t::decrement( hidden_lingering_insanity );
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
    double amount                           = num_amount;
    double amount_from_power_infusion       = 0.0;
    double amount_from_surrender_to_madness = 0.0;

    if ( buffs.surrender_to_madness_death->check() )
    {
      double total_amount = 0.0;
    }
    else
    {
      if ( buffs.surrender_to_madness->check() && buffs.power_infusion->check() )
      {
        double total_amount = amount * ( 1.0 + buffs.power_infusion->data().effectN( 2 ).percent() ) *
                              ( 1.0 + talents.surrender_to_madness->effectN( 1 ).percent() );

        amount_from_surrender_to_madness = amount * talents.surrender_to_madness->effectN( 1 ).percent();

        // Since this effect is multiplicitive, we'll give the extra to Power Infusion since it does not last as long as
        // Surrender to Madness
        amount_from_power_infusion = total_amount - amount - amount_from_surrender_to_madness;

        // Make sure the maths line up.
        assert( total_amount == amount + amount_from_power_infusion + amount_from_surrender_to_madness );
      }
      else if ( buffs.surrender_to_madness->check() )
      {
        amount_from_surrender_to_madness =
            ( amount * ( 1.0 + talents.surrender_to_madness->effectN( 1 ).percent() ) ) - amount;
      }
      else if ( buffs.power_infusion->check() )
      {
        amount_from_power_infusion =
            ( amount * ( 1.0 + buffs.power_infusion->data().effectN( 2 ).percent() ) ) - amount;
      }

      insanity.gain( amount, g, action );

      if ( amount_from_power_infusion > 0.0 )
      {
        insanity.gain( amount_from_power_infusion, gains.insanity_power_infusion, action );
      }

      if ( amount_from_surrender_to_madness > 0.0 )
      {
        insanity.gain( amount_from_surrender_to_madness, gains.insanity_surrender_to_madness, action );
      }
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
    stack_drain_multiplier( 2 / 3.0 ),  // Hardcoded Patch 7.1.5 (2016-12-02)
    base_drain_multiplier( 1.0 )
{
}

/// Deferred init for actor dependent stuff not ready in the ctor
void priest_t::insanity_state_t::init()
{
  if ( actor.sets->has_set_bonus( PRIEST_SHADOW, T20, B4 ) )
  {
    if ( actor.talents.surrender_to_madness->ok() )
    {
      base_drain_multiplier -= actor.sets->set( PRIEST_SHADOW, T20, B4 )->effectN( 2 ).percent();
    }
    else
    {
      base_drain_multiplier -= actor.sets->set( PRIEST_SHADOW, T20, B4 )->effectN( 1 ).percent();
    }
  }
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

  // All drained, cancel voidform. TODO: Can this really even happen?
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

void priest_t::trigger_call_to_the_void( const dot_t* d )
{
  if ( rppm.call_to_the_void->trigger() )
  {
    procs.void_tendril->occur();
    make_event<ground_aoe_event_t>( *sim, this,
                                    ground_aoe_params_t()
                                        .target( d->target )
                                        .duration( find_spell( 193473 )->duration() )
                                        .action( active_spells.void_tendril ) );
  }
}

void priest_t::create_buffs_shadow()
{
  // Baseline

  buffs.shadowform = make_buff( this, "shadowform", find_class_spell( "Shadowform" ) )
                         ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.shadowform_state = make_buff( this, "shadowform_state" )->set_chance( 1.0 )->set_quiet( true );
  buffs.shadowy_insight  = make_buff( this, "shadowy_insight", talents.shadowy_insight )
                              ->set_rppm( RPPM_HASTE )
                              ->set_max_stack( 1U );  // Spell Data says 2, really is 1 -- 2016/04/17 Twintop
  buffs.voidform              = new buffs::voidform_t( *this );
  buffs.insanity_drain_stacks = new buffs::insanity_drain_stacks_t( *this );
  buffs.vampiric_embrace      = make_buff( this, "vampiric_embrace", find_class_spell( "Vampiric Embrace" ) );
  buffs.dispersion            = make_buff( this, "dispersion", find_class_spell( "Dispersion" ) );

  // Tier Sets
  buffs.power_overwhelming = make_buff<stat_buff_t>( this, "power_overwhelming",
                                                     sets->set( specialization(), T19OH, B8 )->effectN( 1 ).trigger() );
  buffs.power_overwhelming->set_trigger_spell( sets->set( specialization(), T19OH, B8 ) );
  buffs.void_vb    = make_buff( this, "void", find_spell( 211657 ) )->set_chance( 1.0 );
  buffs.empty_mind = make_buff( this, "empty_mind", find_spell( 247226 ) )
                         ->set_chance( sets->has_set_bonus( PRIEST_SHADOW, T20, B2 ) )
                         ->set_max_stack( 10 );  // TODO Update from spelldata
  buffs.overwhelming_darkness = new buffs::overwhelming_darkness_t( *this );

  // Talents
  buffs.power_infusion = make_buff<haste_buff_t>( this, "power_infusion", talents.power_infusion )
                             ->add_invalidate( CACHE_SPELL_HASTE )
                             ->add_invalidate( CACHE_HASTE );
  buffs.twist_of_fate = make_buff( this, "twist_of_fate", talents.twist_of_fate )
                            ->set_duration( talents.twist_of_fate->effectN( 1 ).trigger()->duration() )
                            ->set_default_value( talents.twist_of_fate->effectN( 1 ).trigger()->effectN( 2 ).percent() )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                            ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  buffs.void_torrent               = make_buff( this, "void_torrent", talents.void_torrent );
  buffs.surrender_to_madness       = new buffs::surrender_to_madness_t( *this );
  buffs.surrender_to_madness_death = make_buff( this, "surrender_to_madness_death", talents.surrender_to_madness )
                                         ->set_chance( 1.0 )
                                         ->set_duration( timespan_t::from_seconds( 30 ) )
                                         ->set_default_value( 0.0 );
  buffs.lingering_insanity = new buffs::lingering_insanity_t( *this );

  // Legendaries
  buffs.anunds_last_breath = make_buff( this, "anunds_last_breath", find_spell( 215210 ) );
  buffs.iridis_empowerment = make_buff( this, "iridis_empowerment", find_spell( 224999 ) )
                                 ->set_chance( active_items.zenkaram_iridis_anadem ? 1.0 : 0.0 );
  buffs.the_twins_painful_touch = make_buff( this, "the_twins_painful_touch", find_spell( 207721 ) )
                                      ->set_chance( active_items.the_twins_painful_touch ? 1.0 : 0.0 );
  buffs.zeks_exterminatus = make_buff( this, "zeks_exterminatus", find_spell( 236545 ) )->set_rppm( RPPM_HASTE );

  // Artifact
  buffs.sphere_of_insanity = make_buff( this, "sphere_of_insanity", find_spell( 194182 ) )
                                 ->set_chance( 1.0 )
                                 ->set_default_value( find_spell( 194182 )->effectN( 3 ).percent() );
  buffs.mind_quickening = make_buff<stat_buff_t>( this, "mind_quickening", find_spell( 240673 ) );
}

void priest_t::init_rng_shadow()
{
  rppm.call_to_the_void = get_rppm( "call_to_the_void", artifact.call_to_the_void );
  rppm.shadowy_insight  = get_rppm( "shadowy_insighty", talents.shadowy_insight );
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
  talents.power_infusion     = find_talent_spell( "Power Infusion" );
  // T100
  talents.legacy_of_the_void   = find_talent_spell( "Legacy of the Void" );
  talents.void_torrent         = find_talent_spell( "Void Torrent" );
  talents.surrender_to_madness = find_talent_spell( "Surrender to Madness" );

  // Artifact - Xal'atath, Blade of the Black Empire

  artifact.call_to_the_void      = find_artifact_spell( "Call to the Void" );
  artifact.creeping_shadows      = find_artifact_spell( "Creeping Shadows" );
  artifact.darkening_whispers    = find_artifact_spell( "Darkening Whispers" );
  artifact.deaths_embrace        = find_artifact_spell( "Death's Embrace" );
  artifact.from_the_shadows      = find_artifact_spell( "From the Shadows" );
  artifact.mass_hysteria         = find_artifact_spell( "Mass Hysteria" );
  artifact.mental_fortitude      = find_artifact_spell( "Mental Fortitude" );
  artifact.mind_shattering       = find_artifact_spell( "Mind Shattering" );
  artifact.sinister_thoughts     = find_artifact_spell( "Sinister Thoughts" );
  artifact.sphere_of_insanity    = find_artifact_spell( "Sphere of Insanity" );
  artifact.thoughts_of_insanity  = find_artifact_spell( "Thoughts of Insanity" );
  artifact.thrive_in_the_shadows = find_artifact_spell( "Thrive in the Shadows" );
  artifact.to_the_pain           = find_artifact_spell( "To the Pain" );
  artifact.touch_of_darkness     = find_artifact_spell( "Touch of Darkness" );
  artifact.unleash_the_shadows   = find_artifact_spell( "Unleash the Shadows" );
  artifact.void_corruption       = find_artifact_spell( "Void Corruption" );
  artifact.void_siphon           = find_artifact_spell( "Void Siphon" );
  //  artifact.void_torrent         = find_artifact_spell( "Void Torrent" );
  artifact.darkness_of_the_conclave = find_artifact_spell( "Darkness of the Conclave" );
  artifact.fiending_dark            = find_artifact_spell( "Fiending Dark" );
  artifact.mind_quickening          = find_artifact_spell( "Mind Quickening" );
  artifact.lash_of_insanity         = find_artifact_spell( "Lash of Insanity" );

  // General Spells
  specs.voidform            = find_specialization_spell( "Voidform" );
  specs.void_eruption       = find_specialization_spell( "Void Eruption" );
  specs.shadowy_apparitions = find_specialization_spell( "Shadowy Apparitions" );
  specs.shadow_priest       = find_specialization_spell( "Shadow Priest" );

  base.distance = 27.0;
}

action_t* priest_t::create_action_shadow( const std::string& name, const std::string& options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  if ( name == "mind_flay" )
    return new mind_flay_t( *this, options_str );
  if ( name == "void_bolt" )
    return new void_bolt_t( *this, options_str );
  if ( name == "void_eruption" )
    return new void_eruption_t( *this, options_str );
  if ( name == "mind_sear" )
    return new mind_sear_t( *this, options_str );
  if ( name == "shadow_crash" )
    return new shadow_crash_t( *this, options_str );
  if ( name == "shadow_word_death" )
    return new shadow_word_death_t( *this, options_str );
  if ( name == "void_torrent" )
    return new void_torrent_t( *this, options_str );
  if ( name == "shadow_word_pain" )
    return new shadow_word_pain_t( *this, options_str );
  if ( name == "vampiric_touch" )
    return new vampiric_touch_t( *this, options_str );
  if ( name == "dispersion" )
    return new dispersion_t( *this, options_str );
  if ( name == "surrender_to_madness" )
    return new surrender_to_madness_t( *this, options_str );
  if ( name == "silence" )
    return new silence_t( *this, options_str );
  if ( name == "mind_bomb" )
    return new mind_bomb_t( *this, options_str );
  if ( name == "psychic_horror" )
    return new psychic_horror_t( *this, options_str );
  if ( name == "vampiric_embrace" )
    return new vampiric_embrace_t( *this, options_str );
  if ( name == "shadowform" )
    return new shadowform_t( *this, options_str );
  if ( ( name == "mind_blast" ) || ( name == "shadow_word_void" ) )
  {
    if ( talents.shadow_word_void->ok() )
    {
      return new shadow_word_void_t( *this, options_str );
    }
    else
    {
      return new mind_blast_t( *this, options_str );
    }
  }

  return nullptr;
}

expr_t* priest_t::create_expression_shadow( action_t* a, const std::string& name_str )
{
  if ( name_str == "shadowy_apparitions_in_flight" )
  {
    return make_fn_expr( name_str, [this]() {
      if ( !active_spells.shadowy_apparitions )
        return 0.0;

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
  action_priority_list_t* check        = get_action_priority_list( "check" );
  action_priority_list_t* main         = get_action_priority_list( "main" );
  action_priority_list_t* s2m          = get_action_priority_list( "s2m" );
  action_priority_list_t* vf           = get_action_priority_list( "vf" );

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
      "potion,if=buff.bloodlust.react|target.time_to_die<"
      "=80|(target.health.pct<35&cooldown.power_infusion.remains<30)" );

  // Choose which APL to use based on talents and fight conditions.

  default_list->add_action(
      "call_action_list,name=check,if=talent.surrender_to_madness.enabled&!buff"
      ".surrender_to_madness.up" );
  default_list->add_action(
      "run_action_list,name=s2m,if=buff.voidform.up&buff.surrender_to_madness."
      "up" );
  default_list->add_action( "run_action_list,name=vf,if=buff.voidform.up" );
  default_list->add_action( "run_action_list,name=main" );

  // s2mcheck APL
  check->add_action( "variable,op=set,name=actors_fight_time_mod,value=0" );
  check->add_action(
      "variable,op=set,name=actors_fight_time_mod,value=-((-(450)+(time+target."
      "time_to_die))%10),if=time+target.time_to_die>450&time+target.time_to_"
      "die<600" );
  check->add_action(
      "variable,op=set,name=actors_fight_time_mod,value=((450-(time+target."
      "time_to_die))%5),if=time+target.time_to_die<=450" );
  check->add_action(
      "variable,op=set,name=s2mcheck,value=variable.s2msetup_time-(variable."
      "actors_fight_time_mod*nonexecute_actors_pct)" );
  check->add_action( "variable,op=min,name=s2mcheck,value=180" );

  // Main APL
  main->add_talent( this, "Surrender to Madness",
                    "if=talent.surrender_to_madness.enabled&target.time_to_die<=variable.s2mcheck" );
  main->add_action( this, "Shadow Word: Death",
                    "if=equipped.zeks_exterminatus&equipped.mangazas_madness&buff.zeks_exterminatus.react" );
  main->add_action( this, "Shadow Word: Pain",
                    "if=talent.misery.enabled&dot.shadow_word_pain.remains<gcd.max,moving=1,cycle_targets=1" );
  main->add_action( this, "Vampiric Touch",
                    "if=talent.misery.enabled&(dot.vampiric_touch.remains<3*gcd.max|dot.shadow_word_pain.remains<3*gcd."
                    "max),cycle_targets=1" );
  main->add_action( this, "Shadow Word: Pain", "if=!talent.misery.enabled&dot.shadow_word_pain.remains<(3+(4%3))*gcd" );
  main->add_action( this, "Vampiric Touch", "if=!talent.misery.enabled&dot.vampiric_touch.remains<(4+(4%3))*gcd" );
  main->add_action( this, "Void Eruption",
                    "if=(talent.mindbender.enabled&cooldown.mindbender.remains<(variable.erupt_eval+gcd.max*4%3))|!"
                    "talent.mindbender.enabled|set_bonus.tier20_4pc" );
  main->add_talent( this, "Shadow Crash", "if=talent.shadow_crash.enabled" );
  main->add_action( this, "Shadow Word: Death",
                    "if=(active_enemies<=4|(talent.reaper_of_souls.enabled&active_enemies<=2))&cooldown.shadow_word_"
                    "death.charges=2&insanity<="
                    "(85-15*talent.reaper_of_souls.enabled)|(equipped.zeks_exterminatus&"
                    "buff.zeks_exterminatus.react)" );
  main->add_action( this, "Mind Blast",
                    "if=active_enemies<=4&talent.legacy_of_the_void.enabled&(insanity"
                    "<=81|(insanity<=75.2&talent.fortress_of_the_mind.enabled))" );
  main->add_action( this, "Mind Blast",
                    "if=active_enemies<=4&!talent.legacy_of_the_void.enabled|(insanity"
                    "<=96|(insanity<=95.2&talent.fortress_of_the_mind.enabled))" );
  main->add_action( this, "Shadow Word: Pain",
                    "if=!talent.misery.enabled&!ticking&target.time_to_die"
                    ">10&(active_enemies<5&(talent.auspicious_spirits.enabled|talent.shadowy_"
                    "insight.enabled)),cycle_targets=1" );
  main->add_action( this, "Vampiric Touch",
                    "if=active_enemies>1&!talent.misery.enabled&!ticking&(variable"
                    ".dot_vt_dpgcd*target.time_to_die%(gcd.max*(156+variable.sear_dpgcd*(active_"
                    "enemies-1))))>1,cycle_targets=1" );
  main->add_action( this, "Shadow Word: Pain",
                    "if=active_enemies>1&!talent.misery.enabled&!ticking&(variable"
                    ".dot_swp_dpgcd*target.time_to_die%(gcd.max*(118+variable.sear_dpgcd*(active_"
                    "enemies-1))))>1,cycle_targets=1" );
  main->add_talent( this, "Shadow Word: Void",
                    "if=talent.shadow_word_void.enabled&(insanity<=75-10*"
                    "talent.legacy_of_the_void.enabled)" );
  main->add_action( this, "Mind Flay", "interrupt=1,chain=1" );
  main->add_action( this, "Shadow Word: Pain" );

  // Surrender to Madness APL
  if ( !options.priest_suppress_sephuz )
    s2m->add_action( this, "Silence",
                     "if=equipped.sephuzs_secret&(target.is_add|target.debuff.casting."
                     "react)&cooldown.buff_sephuzs_secret.up&!buff.sephuzs_secret.up"
                     ",cycle_targets=1" );
  s2m->add_action( this, "Void Bolt", "if=buff.insanity_drain_stacks.value<6&set_bonus.tier19_4pc" );
  if ( !options.priest_suppress_sephuz )
    s2m->add_talent( this, "Mind Bomb",
                     "if=equipped.sephuzs_secret&target.is_add&cooldown.buff_sephuzs_"
                     "secret.remains<1&!buff.sephuzs_secret.up,cycle_targets=1" );
  s2m->add_talent( this, "Shadow Crash", "if=talent.shadow_crash.enabled" );
  s2m->add_talent( this, "Mindbender",
                   "if=cooldown.shadow_word_death.charges=0&buff.voidform.stack>(45"
                   "+25*set_bonus.tier20_4pc)" );
  s2m->add_action( this, "Void Torrent",
                   "if=dot.shadow_word_pain.remains>5.5&dot.vampiric_"
                   "touch.remains>5.5&!buff.power_infusion.up|buff.voidform.stack<5" );
  s2m->add_action( "berserking,if=buff.voidform.stack>=65" );
  s2m->add_action( this, "Shadow Word: Death",
                   "if=current_insanity_drain*gcd.max>insanity&(insanity-"
                   "(current_insanity_drain*gcd.max)+(30+30*talent.reaper_of_souls.enabled)<100)" );
  if ( race == RACE_BLOOD_ELF )
    s2m->add_action(
        "arcane_torrent,if=buff.insanity_drain_stacks.value>=65"
        "&(insanity-(current_insanity_drain*gcd.max)+30)<100" );
  s2m->add_talent( this, "Power Infusion",
                   "if=cooldown.shadow_word_death.charges=0&buff.voidform.stack"
                   ">(45+25*set_bonus.tier20_4pc)|target.time_to_die<=30" );
  s2m->add_action( this, "Void Bolt" );
  s2m->add_action( this, "Shadow Word: Death",
                   "if=(active_enemies<=4|(talent.reaper_of_souls.enabled&active_"
                   "enemies<=2))&current_insanity_drain*gcd.max>insanity&(insanity-"
                   "(current_insanity_drain*gcd.max)+(30+30*talent.reaper_of_souls.enabled))"
                   "<100" );
  s2m->add_action(
      "wait,sec=action.void_bolt.usable_in,if=action.void_bolt.usable_in<gcd."
      "max*0.28" );
  s2m->add_action( this, "Dispersion",
                   "if=current_insanity_drain*gcd.max>insanity&!buff.power_"
                   "infusion.up|(buff.voidform.stack>76&cooldown.shadow_word_death.charges=0"
                   "&current_insanity_drain*gcd.max>insanity)" );
  s2m->add_action( this, "Mind Blast", "if=active_enemies<=5" );
  s2m->add_action(
      "wait,sec=action.mind_blast.usable_in,if=action.mind_blast.usable_in<gcd."
      "max*0.28&active_enemies<=5" );
  s2m->add_action( this, "Shadow Word: Death",
                   "if=(active_enemies<=4|(talent.reaper_of_souls.enabled&"
                   "active_enemies<=2))&cooldown.shadow_word_death.charges=2" );
  s2m->add_action( this, "Shadowfiend", "if=!talent.mindbender.enabled&buff.voidform.stack>15" );
  s2m->add_talent( this, "Shadow Word: Void",
                   "if=talent.shadow_word_void.enabled&(insanity-(current_"
                   "insanity_drain*gcd.max)+50)<100" );
  s2m->add_action( this, "Shadow Word: Pain",
                   "if=talent.misery.enabled&dot.shadow_word_pain.remains"
                   "<gcd,moving=1,cycle_targets=1" );
  s2m->add_action( this, "Vampiric Touch",
                   "if=talent.misery.enabled&(dot.vampiric_touch.remains<3*"
                   "gcd.max|dot.shadow_word_pain.remains<3*gcd.max),cycle_targets=1" );
  s2m->add_action( this, "Shadow Word: Pain",
                   "if=!talent.misery.enabled&!ticking&(active_enemies<5|"
                   "talent.auspicious_spirits.enabled|talent.shadowy_insight.enabled|"
                   "artifact"
                   ".sphere_of_insanity.rank)" );
  s2m->add_action( this, "Vampiric Touch",
                   "if=!talent.misery.enabled&!ticking&(active_enemies<4"
                   "|(talent.auspicious_spirits.enabled&artifact.unleash_the_shadows.rank))" );
  s2m->add_action( this, "Shadow Word: Pain",
                   "if=!talent.misery.enabled&!ticking&target.time_to_die>"
                   "10&"
                   "(active_enemies<5&(talent.auspicious_spirits.enabled|talent.shadowy_"
                   "insight"
                   ".enabled)),cycle_targets=1" );
  s2m->add_action( this, "Vampiric Touch",
                   "if=!talent.misery.enabled&!ticking&target.time_to_die>10&"
                   "(active_enemies<4|(talent.auspicious_spirits."
                   "enabled&"
                   "artifact.unleash_the_shadows.rank)),cycle_targets=1" );
  s2m->add_action( this, "Shadow Word: Pain",
                   "if=!talent.misery.enabled&!ticking&target.time_to_die>"
                   "10&"
                   "(active_enemies<5&artifact.sphere_of_insanity.rank),cycle_targets=1" );
  s2m->add_action( this, "Mind Flay",
                   "chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&(action."
                   "void_"
                   "bolt.usable|(current_insanity_drain*gcd.max>insanity&(insanity-(current_"
                   "insanity_drain*gcd.max)+60)<100&cooldown.shadow_word_death.charges>=1)"
                   ")" );

  // Voidform APL
  vf->add_talent( this, "Surrender to Madness",
                  "if=talent.surrender_to_madness.enabled&insanity>="
                  "25&(cooldown.void_bolt.up|cooldown.void_torrent.up|cooldown.shadow_word_"
                  "death.up|buff.shadowy_insight.up)&target.time_to_die<=variable.s2mcheck-"
                  "(buff.insanity_drain_stacks.value)" );
  if ( !options.priest_suppress_sephuz )
    vf->add_action( this, "Silence",
                    "if=equipped.sephuzs_secret&(target.is_add|target.debuff.casting."
                    "react)&cooldown.buff_sephuzs_secret.up&!buff.sephuzs_secret.up"
                    "&buff.insanity_drain_stacks.value>10,cycle_targets=1" );
  vf->add_action( this, "Void Bolt" );
  vf->add_action( this, "Shadow Word: Death",
                  "if=equipped.zeks_exterminatus&equipped."
                  "mangazas_madness&buff.zeks_exterminatus.react" );
  if ( race == RACE_BLOOD_ELF )
    vf->add_action(
        "arcane_torrent,if=buff.insanity_drain_stacks.value>=20&(insanity-"
        "(current_insanity_drain*gcd.max)+15)<100" );
  if ( !options.priest_suppress_sephuz )
    vf->add_talent( this, "Mind Bomb",
                    "if=equipped.sephuzs_secret&target.is_add&cooldown.buff_sephuzs_"
                    "secret.remains<1&!buff.sephuzs_secret.up&buff.insanity_drain_stacks.value>10"
                    ",cycle_targets=1" );
  vf->add_talent( this, "Shadow Crash", "if=talent.shadow_crash.enabled" );
  vf->add_action( this, "Void Torrent",
                  "if=dot.shadow_word_pain.remains>5.5&dot.vampiric_touch.remains"
                  ">5.5&(!talent.surrender_to_madness.enabled|(talent.surrender_to_madness."
                  "enabled&target.time_to_die>variable.s2mcheck-(buff.insanity_drain_"
                  "stacks.value)+60))" );
  vf->add_talent( this, "Mindbender",
                  "if=buff.insanity_drain_stacks.value>=(variable.cd_time+(variable.haste_"
                  "eval*!set_bonus.tier20_4pc)-(3*set_bonus.tier20_4pc*(raid_event.movement.in<15)*"
                  "((active_enemies-(raid_event"
                  ".adds.count*(raid_event.adds.remains>0)))=1))+(5-3*set_bonus.tier20_4pc)*buff.bloodlust."
                  "up+2*talent.fortress_of_the_mind.enabled*set_bonus.tier20_4pc)&(!talent.surrender_to_"
                  "madness.enabled|(talent.surrender_to_madness.enabled&target.time_to_die>variable."
                  "s2mcheck-buff.insanity_drain_stacks.value))" );
  vf->add_talent( this, "Power Infusion",
                  "if=buff.insanity_drain_stacks.value>=(variable.cd_time+5*buff."
                  "bloodlust.up*(1+1*set_bonus.tier20_4pc))&(!talent.surrender_to_madness.enabled"
                  "|(talent.surrender_to_madness.enabled&target.time_to_die>variable.s2mcheck-(buff"
                  ".insanity_drain_stacks.value)+61))" );
  vf->add_action(
      "berserking,if=buff.voidform.stack>=10&buff.insanity_drain_stacks.value<="
      "20&"
      "(!talent.surrender_to_madness.enabled|(talent.surrender_to_madness."
      "enabled&"
      "target.time_to_die>variable.s2mcheck-(buff.insanity_drain_stacks.value)+"
      "60))" );
  vf->add_action( this, "Shadow Word: Death",
                  "if=(active_enemies<=4|(talent.reaper_of_souls.enabled&"
                  "active_"
                  "enemies<=2))&current_insanity_drain*gcd.max>insanity&(insanity-"
                  "(current_insanity_drain*gcd.max)+(15+15*talent.reaper_of_souls.enabled))"
                  "<100" );
  vf->add_action(
      "wait,sec=action.void_bolt.usable_in,if=action.void_bolt.usable_in<gcd."
      "max*0.28" );
  vf->add_action( this, "Mind Blast", "if=active_enemies<=4" );
  vf->add_action(
      "wait,sec=action.mind_blast.usable_in,if=action.mind_blast.usable_in<gcd."
      "max*0.28&active_enemies<=4" );
  vf->add_action( this, "Shadow Word: Death",
                  "if=(active_enemies<=4|(talent.reaper_of_souls"
                  ".enabled&active_enemies<=2))&cooldown.shadow_word_death.charges=2|"
                  "(equipped.zeks_exterminatus&buff.zeks_exterminatus.react)" );
  vf->add_action( this, "Shadowfiend", "if=!talent.mindbender.enabled&buff.voidform.stack>15" );
  vf->add_talent( this, "Shadow Word: Void",
                  "if=talent.shadow_word_void.enabled&(insanity-(current_"
                  "insanity_drain*gcd.max)+25)<100" );
  vf->add_action( this, "Shadow Word: Pain",
                  "if=talent.misery.enabled&dot.shadow_word_pain.remains"
                  "<gcd,moving=1,cycle_targets=1" );
  vf->add_action( this, "Vampiric Touch",
                  "if=talent.misery.enabled&(dot.vampiric_touch.remains<3*"
                  "gcd.max|dot.shadow_word_pain.remains<3*gcd.max)&target.time_to_die>5*gcd."
                  "max,cycle_targets=1" );
  vf->add_action( this, "Shadow Word: Pain",
                  "if=!talent.misery.enabled&!ticking&(active_enemies<5|"
                  "talent.auspicious_spirits.enabled|talent.shadowy_insight.enabled|"
                  "artifact"
                  ".sphere_of_insanity.rank)" );
  vf->add_action( this, "Vampiric Touch",
                  "if=!talent.misery.enabled&!ticking&(active_enemies<4|"
                  "(talent.auspicious_spirits.enabled&artifact.unleash_the_shadows.rank))" );
  vf->add_action( this, "Vampiric Touch",
                  "if=active_enemies>1&!talent.misery.enabled&!ticking&((1+0.02"
                  "*buff.voidform.stack)*variable.dot_vt_dpgcd*target.time_to_die%(gcd.max*(156"
                  "+variable.sear_dpgcd*(active_enemies-1))))>1,cycle_targets=1" );
  vf->add_action( this, "Shadow Word: Pain",
                  "if=active_enemies>1&!talent.misery.enabled&!ticking&((1+0.02"
                  "*buff.voidform.stack)*variable.dot_swp_dpgcd*target.time_to_die%(gcd.max*(118"
                  "+variable.sear_dpgcd*(active_enemies-1))))>1,cycle_targets=1" );
  if ( race == RACE_LIGHTFORGED_DRAENEI )
    vf->add_action( "lights_judgment,if=buff.voidform.stack<10" );
  vf->add_action( this, "Mind Flay",
                  "chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&(action."
                  "void_"
                  "bolt.usable|(current_insanity_drain*gcd.max>insanity&(insanity-(current_"
                  "insanity_drain*gcd.max)+30)<100&cooldown.shadow_word_death.charges>=1)"
                  ")" );
  vf->add_action( this, "Shadow Word: Pain" );
}
}  // namespace priestspace

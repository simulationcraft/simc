// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action/sc_action_state.hpp"
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
// Mind Blast
// ==========================================================================
struct mind_blast_t final : public priest_spell_t
{
private:
  double mind_blast_insanity;
  double whispers_of_the_damned_value;
  double harvested_thoughts_value;
  double whispers_bonus_insanity;
  const spell_data_t* mind_flay_spell;
  const spell_data_t* mind_sear_spell;
  bool only_cwc;
  cooldown_t* dark_thought_dummy_cooldown;
  cooldown_t* action_cooldown;

public:
  mind_blast_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "mind_blast", player, player.find_class_spell( "Mind Blast" ) ),
      mind_blast_insanity( priest().find_spell( 137033 )->effectN( 12 ).resource( RESOURCE_INSANITY ) ),
      whispers_of_the_damned_value( priest().azerite.whispers_of_the_damned.value( 2 ) ),
      harvested_thoughts_value( priest().azerite.thought_harvester.value( 1 ) ),
      whispers_bonus_insanity( priest()
                                   .azerite.whispers_of_the_damned.spell()
                                   ->effectN( 1 )
                                   .trigger()
                                   ->effectN( 1 )
                                   .trigger()
                                   ->effectN( 1 )
                                   .resource( RESOURCE_INSANITY ) ),
      mind_flay_spell( player.find_specialization_spell( "Mind Flay" ) ),
      mind_sear_spell( player.find_class_spell( "Mind Sear" ) ),
      only_cwc( false ),
      dark_thought_dummy_cooldown( player.get_cooldown( "dark_thought_dummy_cooldown" ) ),
      action_cooldown( player.get_cooldown( "mind_blast" ) )
  {
    add_option( opt_bool( "only_cwc", only_cwc ) );
    parse_options( options_str );

    affected_by_shadow_weaving = true;

    // This was removed from the Mind Blast spell and put on the Shadow Priest spell instead
    energize_amount = mind_blast_insanity;
    energize_amount *= 1 + priest().talents.fortress_of_the_mind->effectN( 2 ).percent();

    spell_power_mod.direct *= 1.0 + player.talents.fortress_of_the_mind->effectN( 4 ).percent();

    if ( priest().conduits.mind_devourer->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.mind_devourer.percent() );
    }

    cooldown->hasted     = true;
    usable_while_casting = use_while_casting = only_cwc;

    base_dd_adder += whispers_of_the_damned_value;

    // Cooldown reduction
    apply_affecting_aura( player.find_rank_spell( "Mind Blast", "Rank 2", PRIEST_SHADOW ) );
  }

  // Returns either mind blasts action cooldown, or when dark thoughts is up a dummy cooldown
  cooldown_t* active_cooldown() const
  {
    if ( priest().buffs.dark_thoughts->check() )
    {
      return dark_thought_dummy_cooldown;
    }
    return action_cooldown;
  }

  bool action_ready() override
  {
    cooldown   = active_cooldown();
    auto ready = priest_spell_t::action_ready();
    cooldown   = action_cooldown;
    return ready;
  }

  void execute() override
  {
    cooldown = active_cooldown();
    priest_spell_t::execute();
    cooldown = action_cooldown;
  }

  bool ready() override
  {
    if ( only_cwc )
    {
      if ( !priest().buffs.dark_thoughts->check() )
        return false;
      if ( player->channeling == nullptr )
        return false;
      if ( player->channeling->data().id() == mind_flay_spell->id() ||
           player->channeling->data().id() == mind_sear_spell->id() )
        return priest_spell_t::ready();
      return false;
    }
    else
    {
      return priest_spell_t::ready();
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double amount = priest_spell_t::composite_energize_amount( s );

    if ( s->result == RESULT_CRIT )
    {
      amount += whispers_bonus_insanity;
    }

    return amount;
  }

  bool talbadars_stratagem_active( const player_t* target ) const
  {
    if ( !priest().legendary.talbadars_stratagem->ok() )
      return false;
    const priest_td_t* td = find_td( target );
    if ( !td )
      return false;

    return td->dots.shadow_word_pain->is_ticking() && td->dots.vampiric_touch->is_ticking() &&
           td->dots.devouring_plague->is_ticking();
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = action_t::composite_target_da_multiplier( t );

    if ( talbadars_stratagem_active( t ) )
    {
      tdm *= 1 + priest().legendary.talbadars_stratagem->effectN( 1 ).percent();
    }

    return tdm;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().legendary.shadowflame_prism->ok() )
    {
      priest().trigger_shadowflame_prism( s->target );
    }

    if ( priest().buffs.mind_devourer->trigger() )
    {
      priest().procs.mind_devourer->occur();
    }

    priest().trigger_shadowy_apparitions( s );

    if ( priest().talents.psychic_link->ok() )
    {
      priest().trigger_psychic_link( s );
    }
  }

  timespan_t execute_time() const override
  {
    if ( priest().buffs.dark_thoughts->check() )
    {
      return timespan_t::zero();
    }

    return priest_spell_t::execute_time();
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

    if ( priest().buffs.dark_thoughts->up() )
      priest().buffs.dark_thoughts->decrement();
    else
      priest_spell_t::update_ready( cd_duration );
  }
};

// ==========================================================================
// Mind Sear
// ==========================================================================
struct mind_sear_tick_t final : public priest_spell_t
{
  double insanity_gain;
  double harvested_thoughts_multiplier;
  bool thought_harvester_empowered = false;

  mind_sear_tick_t( priest_t& p, const spell_data_t* s )
    : priest_spell_t( "mind_sear_tick", p, s ),
      insanity_gain( p.find_spell( 208232 )->effectN( 1 ).percent() ),
      harvested_thoughts_multiplier( priest()
                                         .azerite.thought_harvester.spell()
                                         ->effectN( 1 )
                                         .trigger()
                                         ->effectN( 1 )
                                         .trigger()
                                         ->effectN( 1 )
                                         .percent() )
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

    priest().trigger_eternal_call_to_the_void( s );

    trigger_dark_thoughts( s->target, priest().procs.dark_thoughts_sear, s );
  }
};

struct mind_sear_t final : public priest_spell_t
{
  mind_sear_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mind_sear", p, p.find_class_spell( ( "Mind Sear" ) ) )
  {
    parse_options( options_str );
    channeled           = true;
    may_crit            = false;
    hasted_ticks        = false;
    dynamic_tick_action = true;
    tick_zero           = false;
    radius = data().effectN( 1 ).trigger()->effectN( 2 ).radius();  // need to set radius in here so that the APL
                                                                    // functions correctly

    tick_action = new mind_sear_tick_t( p, data().effectN( 1 ).trigger() );
  }

  void execute() override
  {
    priest_spell_t::execute();

    auto mind_sear_tick_action = debug_cast<mind_sear_tick_t*>( tick_action );

    if ( priest().buffs.harvested_thoughts->check() )
    {
      mind_sear_tick_action->thought_harvester_empowered = true;
      priest().buffs.harvested_thoughts->expire();
    }
    else
    {
      mind_sear_tick_action->thought_harvester_empowered = false;
    }
  }
};

// ==========================================================================
// Mind Flay
// ==========================================================================
struct mind_flay_t final : public priest_spell_t
{
  mind_flay_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mind_flay", p, p.find_specialization_spell( "Mind Flay" ) )
  {
    parse_options( options_str );

    affected_by_shadow_weaving = true;
    may_crit                   = false;
    channeled                  = true;
    hasted_ticks               = false;
    use_off_gcd                = true;

    energize_amount *= 1 + p.talents.fortress_of_the_mind->effectN( 1 ).percent();

    spell_power_mod.tick *= 1.0 + p.talents.fortress_of_the_mind->effectN( 3 ).percent();
  }

  void trigger_mind_flay_dissonant_echoes()
  {
    if ( !priest().conduits.dissonant_echoes->ok() || priest().buffs.voidform->check() )
    {
      return;
    }

    if ( rng().roll( priest().conduits.dissonant_echoes.percent() ) )
    {
      priest().cooldowns.void_bolt->reset( true );
      priest().buffs.dissonant_echoes->trigger();
      priest().procs.dissonant_echoes->occur();
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    priest().trigger_eternal_call_to_the_void( d->state );
    trigger_dark_thoughts( d->target, priest().procs.dark_thoughts_flay, d->state );
    trigger_mind_flay_dissonant_echoes();
  }

  void execute() override
  {
    priest_spell_t::execute();

    // Dissonant Echoes can proc on tick or on initial execute
    // since it doesn't have a tick_zero we put it in both places
    trigger_mind_flay_dissonant_echoes();
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
// Shadow Word: Death
// ==========================================================================
struct painbreaker_psalm_t final : public priest_spell_t
{
  timespan_t consume_time;

  painbreaker_psalm_t( priest_t& p )
    : priest_spell_t( "painbreaker_psalm", p, p.legendary.painbreaker_psalm ),
      consume_time( timespan_t::from_seconds( data().effectN( 1 ).base_value() ) )
  {
    background = true;

    // TODO: check if this double dips from any multipliers or takes 100% exactly the calculated dot values.
    // also check that the STATE_NO_MULTIPLIER does exactly what we expect.
    snapshot_flags &= ~STATE_NO_MULTIPLIER;
  }

  void impact( action_state_t* s ) override
  {
    priest_td_t& td = get_td( s->target );
    dot_t* swp      = td.dots.shadow_word_pain;
    dot_t* vt       = td.dots.vampiric_touch;

    auto swp_damage = priest().tick_damage_over_time( consume_time, td.dots.shadow_word_pain );
    auto vt_damage  = priest().tick_damage_over_time( consume_time, td.dots.vampiric_touch );
    base_dd_min = base_dd_max = swp_damage + vt_damage;

    sim->print_debug( "{} {} calculated dot damage sw:p={} vt={} total={}", *player, *this, swp_damage, vt_damage,
                      swp_damage + vt_damage );

    priest_spell_t::impact( s );

    swp->adjust_duration( -consume_time );
    vt->adjust_duration( -consume_time );
  }
};

struct shadow_word_death_t final : public priest_spell_t
{
  double execute_percent;
  double execute_modifier;

  shadow_word_death_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "shadow_word_death", p, p.find_class_spell( "Shadow Word: Death" ) ),
      execute_percent( data().effectN( 2 ).base_value() ),
      execute_modifier( data().effectN( 3 ).percent() )
  {
    parse_options( options_str );

    affected_by_shadow_weaving = true;

    auto rank2 = p.find_rank_spell( "Shadow Word: Death", "Rank 2" );
    if ( rank2->ok() )
    {
      cooldown->duration += rank2->effectN( 1 ).time_value();
    }

    if ( priest().legendary.painbreaker_psalm->ok() )
    {
      impact_action = new painbreaker_psalm_t( p );
      add_child( impact_action );
    }

    cooldown->hasted = true;

    if ( priest().legendary.painbreaker_psalm->ok() )
    {
      // TODO: Check if this ever gets un-hardcoded for (336165)
      energize_type     = action_energize::ON_HIT;
      energize_resource = RESOURCE_INSANITY;
      energize_amount   = 10;
    }
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = action_t::composite_target_da_multiplier( t );

    if ( t->health_percentage() < execute_percent )
    {
      if ( sim->debug )
      {
        sim->print_debug( "{} below {}% HP. Increasing {} damage by {}", t->name_str, execute_percent, *this,
                          execute_modifier );
      }
      tdm *= 1 + execute_modifier;
    }

    return tdm;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest().legendary.shadowflame_prism->ok() )
    {
      priest().trigger_shadowflame_prism( s->target );
    }

    if ( result_is_hit( s->result ) )
    {
      double save_health_percentage = s->target->health_percentage();

      // TODO: Add in a custom buff that checks after 1 second to see if the target SWD was cast on is now dead.

      if ( !( ( save_health_percentage > 0.0 ) && ( s->target->health_percentage() <= 0.0 ) ) )
      {
        // target is not killed
        inflict_self_damage( s->result_amount );
      }

      if ( priest().talents.death_and_madness->ok() )
      {
        priest_td_t& td = get_td( s->target );
        td.buffs.death_and_madness_debuff->trigger();
      }
    }
  }

  void inflict_self_damage( double damage_inflicted_to_target )
  {
    priest().resource_loss( RESOURCE_HEALTH, damage_inflicted_to_target, priest().gains.shadow_word_death_self_damage,
                            this );
  }
};

// ==========================================================================
// Dispersion
// ==========================================================================
struct dispersion_t final : public priest_spell_t
{
  dispersion_t( priest_t& player, util::string_view options_str )
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
  shadowform_t( priest_t& p, util::string_view options_str )
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

// ==========================================================================
// Silence
// ==========================================================================
struct silence_t final : public priest_spell_t
{
  silence_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "silence", player, player.find_class_spell( "Silence" ) )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = is_interrupt = true;

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
    if ( !priest_spell_t::target_ready( candidate_target ) )
      return false;

    if ( candidate_target->debuffs.casting && candidate_target->debuffs.casting->check() )
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
    : priest_spell_t( "vampiric_embrace", p, p.find_class_spell( "Vampiric Embrace" ) )
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
  double spiteful_apparitions_bonus;

  shadowy_apparition_damage_t( priest_t& p )
    : priest_spell_t( "shadowy_apparition", p, p.find_spell( 148859 ) ),
      insanity_gain( priest().talents.auspicious_spirits->effectN( 2 ).percent() ),
      spiteful_apparitions_bonus( priest().azerite.spiteful_apparitions.value( 1 ) )
  {
    affected_by_shadow_weaving = true;
    background                 = true;
    proc                       = false;
    callbacks                  = true;
    may_miss                   = false;
    may_crit                   = false;

    base_dd_multiplier *= 1 + priest().talents.auspicious_spirits->effectN( 1 ).percent();

    if ( priest().conduits.haunting_apparitions->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.haunting_apparitions.percent() );
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
      auto vampiric_touch_dot = state->target->find_dot( "vampiric_touch", player );

      if ( vampiric_touch_dot != nullptr && vampiric_touch_dot->is_ticking() )
      {
        d += spiteful_apparitions_bonus;
      }
    }

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
    : priest_spell_t( "shadow_word_pain", p, p.find_class_spell( "Shadow Word: Pain" ) )
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

    // TODO: This assumes death_throes doesn't affect the direct damage portion - didn't test it
    base_ta_adder += get_death_throes_bonus();

    if ( casted && priest().azerite.torment_of_torments.enabled() )
    {
      base_dd_adder += priest().azerite.torment_of_torments.value( 2 );
    }

    dot_duration += priest().azerite.torment_of_torments.spell()->effectN( 1 ).time_value();
  }

  shadow_word_pain_t( priest_t& p, util::string_view options_str ) : shadow_word_pain_t( p, true )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // Only applied if you hard cast SW:P, Misery and Damnation do not trigger this
    if ( casted && result_is_hit( s->result ) )
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
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( d->state->result_amount > 0 )
    {
      trigger_power_of_the_dark_side();
    }
  }
};

// ==========================================================================
// Unfurling Darkness
// ==========================================================================
struct unfurling_darkness_t final : public priest_spell_t
{
  double vampiric_touch_sp;

  unfurling_darkness_t( priest_t& p )
    : priest_spell_t( "unfurling_darkness", p, p.find_talent_spell( "Unfurling Darkness" ) ),
      vampiric_touch_sp( p.find_spell( 34914 )->effectN( 4 ).sp_coeff() )
  {
    background                 = true;
    spell_power_mod.direct     = vampiric_touch_sp;
    affected_by_shadow_weaving = true;
  }
};

// ==========================================================================
// Vampiric Touch
// ==========================================================================
struct vampiric_touch_t final : public priest_spell_t
{
  propagate_const<shadow_word_pain_t*> child_swp;
  propagate_const<unfurling_darkness_t*> child_ud;
  bool ignore_healing;
  bool casted;

  vampiric_touch_t( priest_t& p, bool _casted = false )
    : priest_spell_t( "vampiric_touch", p, p.find_class_spell( "Vampiric Touch" ) ),
      child_swp( nullptr ),
      child_ud( nullptr ),
      ignore_healing( p.options.priest_ignore_healing )
  {
    casted                     = _casted;
    may_crit                   = false;
    affected_by_shadow_weaving = true;

    if ( priest().talents.misery->ok() && casted )
    {
      child_swp             = new shadow_word_pain_t( priest(), false );
      child_swp->background = true;
    }

    base_ta_adder += priest().azerite.thought_harvester.value( 2 );

    if ( priest().talents.unfurling_darkness->ok() )
    {
      child_ud = new unfurling_darkness_t( priest() );
    }
  }

  vampiric_touch_t( priest_t& p, util::string_view options_str ) : vampiric_touch_t( p, true )
  {
    parse_options( options_str );
  }

  void trigger_heal( action_state_t* s )
  {
    if ( ignore_healing )
    {
      return;
    }

    double amount_to_heal = s->result_amount * data().effectN( 2 ).m_value();
    priest().resource_gain( RESOURCE_HEALTH, amount_to_heal, priest().gains.vampiric_touch_health, this );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    trigger_heal( s );

    if ( child_swp )
    {
      child_swp->target = s->target;
      child_swp->execute();
    }

    // TODO: check if talbadars_stratagem can proc this
    // Damnation does not proc Unfurling Darkness, but can generate it
    if ( priest().buffs.unfurling_darkness->check() && casted )
    {
      child_ud->target = s->target;
      child_ud->execute();
      priest().buffs.unfurling_darkness->expire();
    }
    else
    {
      if ( priest().talents.unfurling_darkness->ok() && !priest().buffs.unfurling_darkness_cd->check() )
      {
        priest().buffs.unfurling_darkness->trigger();
        // The CD Starts as soon as the buff is applied
        priest().buffs.unfurling_darkness_cd->trigger();
      }
    }
  }

  timespan_t execute_time() const override
  {
    if ( priest().buffs.unfurling_darkness->check() )
    {
      return 0_ms;
    }

    return priest_spell_t::execute_time();
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    trigger_heal( d->state );

    if ( d->state->result_amount > 0 && priest().azerite.thought_harvester.enabled() )
    {
      priest().buffs.harvested_thoughts->trigger();
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
  bool casted;

  devouring_plague_t( priest_t& p, bool _casted = false )
    : priest_spell_t( "devouring_plague", p, p.find_class_spell( "Devouring Plague" ) )
  {
    casted                     = _casted;
    may_crit                   = true;
    affected_by_shadow_weaving = true;
  }

  devouring_plague_t( priest_t& p, util::string_view options_str ) : devouring_plague_t( p, true )
  {
    parse_options( options_str );
  }

  action_state_t* new_state()
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

    if ( priest().buffs.mind_devourer->up() )
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

    void_bolt_extension_t( priest_t& player, const spell_data_t* rank2_spell )
      : priest_spell_t( "void_bolt_extension", player, rank2_spell )
    {
      dot_extension = data().effectN( 1 ).time_value();
      aoe           = -1;
      radius        = player.find_spell( 234746 )->effectN( 1 ).radius();
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
    }
  };

  void_bolt_extension_t* void_bolt_extension;
  propagate_const<cooldown_t*> shadowfiend_cooldown;
  propagate_const<cooldown_t*> mindbender_cooldown;
  timespan_t hungering_void_base_duration;
  timespan_t hungering_void_crit_duration;

  void_bolt_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "void_bolt", player, player.find_spell( 205448 ) ),
      void_bolt_extension( nullptr ),
      shadowfiend_cooldown( player.get_cooldown( "mindbender" ) ),
      mindbender_cooldown( player.get_cooldown( "shadowfiend" ) ),
      hungering_void_base_duration(
          timespan_t::from_seconds( priest().talents.hungering_void->effectN( 3 ).base_value() ) ),
      hungering_void_crit_duration(
          timespan_t::from_seconds( priest().talents.hungering_void->effectN( 4 ).base_value() ) )
  {
    parse_options( options_str );
    use_off_gcd                = true;
    energize_type              = action_energize::ON_CAST;
    cooldown->hasted           = true;
    affected_by_shadow_weaving = true;

    auto rank2 = player.find_rank_spell( "Void Bolt", "Rank 2" );
    if ( rank2->ok() )
    {
      void_bolt_extension = new void_bolt_extension_t( player, rank2 );
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
    if ( !priest().buffs.voidform->check() && !priest().buffs.dissonant_echoes->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = action_t::composite_target_da_multiplier( t );

    if ( priest().hungering_void_active( t ) )
    {
      tdm *= 1 + priest().talents.hungering_void->effectN( 1 ).percent();
    }

    return tdm;
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

    if ( priest().talents.hungering_void->ok() && priest().buffs.voidform->check() )
    {
      priest_td_t& td = get_td( s->target );
      // Check if this buff is active, every Void Bolt after the first should get this
      if ( td.buffs.hungering_void_tracking->up() )
      {
        timespan_t seconds_to_add_to_voidform = s->result == RESULT_CRIT
                                                    ? hungering_void_base_duration + hungering_void_crit_duration
                                                    : hungering_void_base_duration;
        sim->print_debug( "{} extending Voidform duration by {} seconds.", priest(), seconds_to_add_to_voidform );
        // TODO: add some type of tracking for this increase
        priest().buffs.voidform->extend_duration( player, seconds_to_add_to_voidform );

        td.buffs.hungering_void->trigger();
      }
      td.buffs.hungering_void_tracking->trigger();
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
    : priest_spell_t( "void_eruption_damage", p, p.find_spell( 228360 ) ), void_bolt( nullptr )
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
    : priest_spell_t( "void_eruption", p, p.find_spell( 228260 ) )
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
    priest().cooldowns.mind_blast->charges = 2;
    priest().cooldowns.mind_blast->reset( true, 2 );
    priest().cooldowns.void_bolt->reset( true );
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
// Surrender to Madness
// ==========================================================================
struct void_eruption_stm_damage_t final : public priest_spell_t
{
  propagate_const<action_t*> void_bolt;

  void_eruption_stm_damage_t( priest_t& p )
    : priest_spell_t( "void_eruption_stm_damage", p, p.find_spell( 228360 ) ), void_bolt( nullptr )
  {
    // This Void Eruption currently only hits a single target
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
    : priest_spell_t( "surrender_to_madness", p, p.talents.surrender_to_madness )
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
      priest().cooldowns.mind_blast->charges = 2;
      priest().cooldowns.mind_blast->reset( true, 2 );
      priest().cooldowns.void_bolt->reset( true );
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
// Mind Bomb
// ==========================================================================
struct mind_bomb_t final : public priest_spell_t
{
  mind_bomb_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "mind_bomb", player, player.talents.mind_bomb )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;

    // Add 2 seconds to emulate the setup time, simplifying the spell
    cooldown->duration += timespan_t::from_seconds( 2 );
  }
};

// ==========================================================================
// Psychic Horror
// ==========================================================================
struct psychic_horror_t final : public priest_spell_t
{
  psychic_horror_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "psychic_horror", player, player.talents.psychic_horror )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;
  }
};

// ==========================================================================
// Void Torrent
// ==========================================================================
struct void_torrent_t final : public priest_spell_t
{
  double insanity_gain;
  propagate_const<devouring_plague_t*> child_dp;

  void_torrent_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "void_torrent", p, p.talents.void_torrent ),
      insanity_gain( p.talents.void_torrent->effectN( 3 ).trigger()->effectN( 1 ).resource( RESOURCE_INSANITY ) ),
      child_dp( new devouring_plague_t( priest(), false ) )
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

    child_dp->background = true;
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
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.void_torrent->trigger();

    // TODO: Verify if this triggers just the DoT, or the upfront damage as well
    // Void Torrent just applies Devouring Plague, it does not refresh it per tick
    child_dp->set_target( target );
    child_dp->execute();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest_td_t& td = get_td( s->target );

    td.dots.shadow_word_pain->refresh_duration();
    td.dots.vampiric_touch->refresh_duration();
  }
};

// ==========================================================================
// Psychic Link
// ==========================================================================
struct psychic_link_t final : public priest_spell_t
{
  psychic_link_t( priest_t& p ) : priest_spell_t( "psychic_link", p, p.find_spell( 199486 ) )
  {
    background = true;
    may_crit   = false;
    may_miss   = false;
    radius     = data().effectN( 1 ).radius_max();
  }

  void trigger( player_t* target, double original_amount )
  {
    base_dd_min = base_dd_max = ( original_amount * priest().talents.psychic_link->effectN( 1 ).percent() );
    player->sim->print_debug( "{} triggered psychic link on target {}.", priest(), *target );

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
    : priest_spell_t( "shadow_crash_damage", p, p.talents.shadow_crash->effectN( 1 ).trigger() )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = action_t::composite_target_da_multiplier( t );

    const priest_td_t* td = find_td( t );

    if ( td && td->buffs.shadow_crash_debuff->check() )
    {
      int stack             = td->buffs.shadow_crash_debuff->check();
      double increase       = priest().talents.shadow_crash->effectN( 1 ).trigger()->effectN( 2 ).percent();
      double stack_increase = increase * stack;
      player->sim->print_debug( "{} target has {} stacks of the shadow_crash_debuff. Increasing Damage by {}",
                                t->name_str, stack, stack_increase );
      tdm *= 1 + stack_increase;
    }

    return tdm;
  }
};

struct shadow_crash_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_crash_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "shadow_crash", p, p.talents.shadow_crash ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    aoe              = -1;
    radius           = data().effectN( 1 ).radius();
    range            = data().max_range();
    cooldown->hasted = true;

    impact_action = new shadow_crash_damage_t( p );
    add_child( impact_action );
  }

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    if ( state->n_targets == 1 )
    {
      priest_td_t& td = get_td( state->target );
      td.buffs.shadow_crash_debuff->trigger();
    }
  }

  timespan_t travel_time() const override
  {
    // Always has the same time to land regardless of distance, probably represented there. Anshlun 2018-08-04
    return timespan_t::from_seconds( data().missile_speed() );
  }

  // Ensuring that we can't cast on a target that is too close
  bool target_ready( player_t* candidate_target ) override
  {
    auto effective_min_range = data().min_range() - data().effectN( 1 ).radius();
    if ( sim->debug )
    {
      sim->print_debug( "Shadow Crash eval: {} < {}", player->get_player_distance( *candidate_target ),
                        effective_min_range );
    }
    if ( player->get_player_distance( *candidate_target ) < effective_min_range )
    {
      return false;
    }

    return priest_spell_t::target_ready( candidate_target );
  }
};

// ==========================================================================
// Searing Nightmare
// ==========================================================================
struct searing_nightmare_t final : public priest_spell_t
{
  propagate_const<shadow_word_pain_t*> child_swp;
  const spell_data_t* mind_sear_spell;

  searing_nightmare_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "searing_nightmare", p, p.find_talent_spell( "Searing Nightmare" ) ),
      child_swp( new shadow_word_pain_t( priest(), false ) ),
      mind_sear_spell( p.find_class_spell( "Mind Sear" ) )
  {
    parse_options( options_str );
    child_swp->background = true;

    may_miss                   = false;
    aoe                        = -1;
    radius                     = data().effectN( 2 ).radius_max();
    usable_while_casting       = use_while_casting;
    affected_by_shadow_weaving = true;
  }

  bool ready() override
  {
    if ( player->channeling == nullptr || player->channeling->data().id() != mind_sear_spell->id() )
      return false;
    return priest_spell_t::ready();
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = action_t::composite_target_da_multiplier( t );

    const priest_td_t* td = find_td( t );

    if ( td && td->dots.shadow_word_pain->is_ticking() )
    {
      tdm *= data().effectN( 1 ).percent();
    }

    return tdm;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    child_swp->target = s->target;
    child_swp->execute();
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
    : priest_spell_t( "damnation", p, p.find_talent_spell( "Damnation" ) ),
      child_swp( new shadow_word_pain_t( priest(), false ) ),
      child_vt( new vampiric_touch_t( priest(), false ) ),
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
  voidform_t( priest_t& p ) : base_t( p, "voidform", p.find_spell( 194249 ) )
  {
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

    // Using Surrender within Voidform does not reset the duration - might be a bug?
    set_refresh_behavior( buff_refresh_behavior::DISABLED );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    if ( priest().talents.ancient_madness->ok() )
    {
      priest().buffs.ancient_madness->trigger();
    }

    priest().buffs.shadowform->expire();

    return r;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    /// TODO: Verify if functionality is properly matching how it works in game.
    if ( sim->debug )
    {
      sim->print_debug( "{} has {} charges of mind blast as voidform ended", *player,
                        priest().cooldowns.mind_blast->charges_fractional() );
    }
    // Call new generic function to adjust charges.
    adjust_max_charges( priest().cooldowns.mind_blast, 1 );
    if ( sim->debug )
    {
      sim->print_debug( "{} has {} charges of mind blast after voidform ended", *player,
                        priest().cooldowns.mind_blast->charges_fractional() );
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

    // It is expected that this tracking buff resets after each voidform, right now this is just universal and will
    // count every subsequent void bolt after the first for the increase
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/703
    if ( priest().talents.hungering_void->ok() && !priest().bugs )
    {
      priest().remove_hungering_void_tracking();
    }
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ==========================================================================
// Shadowform
// ==========================================================================
struct shadowform_t final : public priest_buff_t<buff_t>
{
  shadowform_t( priest_t& p ) : base_t( p, "shadowform", p.find_class_spell( "Shadowform" ) )
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
struct dark_thoughts_t final : public priest_buff_t<buff_t>
{
  dark_thoughts_t( priest_t& p ) : base_t( p, "dark_thoughts", p.find_specialization_spell( "Dark Thoughts" ) )
  {
    // Spell data does not contain information about the spell, must manually set.
    this->set_max_stack( 5 );
    this->set_duration( timespan_t::from_seconds( 6 ) );
    this->set_refresh_behavior( buff_refresh_behavior::DURATION );
    // Allow player to react to the buff being applied so they can cast Mind Blast.
    this->reactable = true;
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
    : base_t( p, "death_and_madness_insanity_gain", p.find_spell( 321973 ) ),
      insanity_gain( data().effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      priest().generate_insanity( insanity_gain, priest().gains.insanity_death_and_madness, nullptr );
    } );
  }
};

// ==========================================================================
// Ancient Madness
// ==========================================================================
struct ancient_madness_t final : public priest_buff_t<buff_t>
{
  ancient_madness_t( priest_t& p ) : base_t( p, "ancient_madness", p.find_talent_spell( "Ancient Madness" ) )
  {
    if ( !data().ok() )
      return;

    add_invalidate( CACHE_CRIT_CHANCE );
    add_invalidate( CACHE_SPELL_CRIT_CHANCE );

    set_duration( p.find_spell( 194249 )->duration() );
    set_default_value( data().effectN( 2 ).percent() );  // Each stack is worth 2% from effect 2
    set_max_stack( as<int>( data().effectN( 1 ).base_value() ) /
                   as<int>( data().effectN( 2 ).base_value() ) );  // Set max stacks to 30 / 2
    set_reverse( true );
    set_period( timespan_t::from_seconds( 1 ) );
  }
};

// ==========================================================================
// Whispers of the Damned (Battle for Azeroth)
// ==========================================================================
struct whispers_of_the_damned_t final : public priest_buff_t<buff_t>
{
  whispers_of_the_damned_t( priest_t& p )
    : base_t( p, "whispers_of_the_damned", p.azerite.whispers_of_the_damned.spell()->effectN( 1 ).trigger() )
  {
    set_trigger_spell( p.azerite.whispers_of_the_damned.spell() );
  }
};

// ==========================================================================
// Chorus of Insanity (Battle for Azeroth)
// ==========================================================================
struct chorus_of_insanity_t final : public priest_buff_t<stat_buff_t>
{
  chorus_of_insanity_t( priest_t& p ) : base_t( p, "chorus_of_insanity", p.find_spell( 279572 ) )
  {
    add_stat( STAT_CRIT_RATING, p.azerite.chorus_of_insanity.value( 1 ) );
    set_reverse( true );
    set_tick_behavior( buff_tick_behavior::REFRESH );
  }
};

// ==========================================================================
// Harvested Thoughts (Battle for Azeroth)
// ==========================================================================
struct harvested_thoughts_t final : public priest_buff_t<buff_t>
{
  harvested_thoughts_t( priest_t& p )
    : base_t( p, "harvested_thoughts",
              p.azerite.thought_harvester.spell()->effectN( 1 ).trigger()->effectN( 1 ).trigger() )
  {
    set_trigger_spell( p.azerite.thought_harvester.spell()->effectN( 1 ).trigger() );
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
};

// ==========================================================================
// Generate Insanity
// Helper method for generating the proper amount of insanity
// ==========================================================================
void priest_t::generate_insanity( double num_amount, gain_t* g, action_t* action )
{
  if ( specialization() == PRIEST_SHADOW )
  {
    double amount                             = num_amount;
    double amount_from_surrender_to_madness   = 0.0;
    double amount_from_memory_of_lucid_dreams = 0.0;

    if ( buffs.surrender_to_madness->check() )
    {
      double total_amount = amount * ( 1.0 + talents.surrender_to_madness->effectN( 2 ).percent() );

      amount_from_surrender_to_madness = amount * talents.surrender_to_madness->effectN( 2 ).percent();

      if ( player_t::buffs.memory_of_lucid_dreams->check() )
      {
        // If both are up, give the benefit to Memory of Lucid Dreams because it is shorter
        amount_from_memory_of_lucid_dreams += ( amount + amount_from_surrender_to_madness ) *
                                              ( azerite_essence.memory_of_lucid_dreams->effectN( 1 ).percent() );

        total_amount = amount * ( 1.0 + talents.surrender_to_madness->effectN( 2 ).percent() ) *
                       ( 1.0 + azerite_essence.memory_of_lucid_dreams->effectN( 1 ).percent() );
      }

      // Make sure the maths line up.
      assert( total_amount == amount + amount_from_surrender_to_madness + amount_from_memory_of_lucid_dreams );
    }
    else if ( player_t::buffs.memory_of_lucid_dreams->check() )
    {
      double total_amount;

      amount_from_memory_of_lucid_dreams +=
          ( amount ) * ( azerite_essence.memory_of_lucid_dreams->effectN( 1 ).percent() );

      total_amount = amount * ( 1.0 + azerite_essence.memory_of_lucid_dreams->effectN( 1 ).percent() );

      // Make sure the maths line up.
      assert( total_amount == amount + amount_from_memory_of_lucid_dreams );
    }

    resource_gain( RESOURCE_INSANITY, amount, g, action );

    if ( amount_from_surrender_to_madness > 0.0 )
    {
      resource_gain( RESOURCE_INSANITY, amount_from_surrender_to_madness, gains.insanity_surrender_to_madness, action );
    }
    if ( amount_from_memory_of_lucid_dreams > 0.0 )
    {
      resource_gain( RESOURCE_INSANITY, amount_from_memory_of_lucid_dreams, gains.insanity_memory_of_lucid_dreams,
                     action );
    }
  }
}

void priest_t::create_buffs_shadow()
{
  // Baseline
  buffs.shadowform       = make_buff<buffs::shadowform_t>( *this );
  buffs.shadowform_state = make_buff<buffs::shadowform_state_t>( *this );
  buffs.voidform         = make_buff<buffs::voidform_t>( *this );
  buffs.vampiric_embrace = make_buff( this, "vampiric_embrace", find_class_spell( "Vampiric Embrace" ) );
  buffs.dark_thoughts    = make_buff<buffs::dark_thoughts_t>( *this );

  // Talents
  buffs.void_torrent           = make_buff( this, "void_torrent", find_talent_spell( "Void Torrent" ) );
  buffs.surrender_to_madness   = make_buff( this, "surrender_to_madness", find_talent_spell( "Surrender to Madness" ) );
  buffs.death_and_madness_buff = make_buff<buffs::death_and_madness_buff_t>( *this );
  buffs.ancient_madness        = make_buff<buffs::ancient_madness_t>( *this );
  buffs.unfurling_darkness =
      make_buff( this, "unfurling_darkness", find_talent_spell( "Unfurling Darkness" )->effectN( 1 ).trigger() );
  buffs.unfurling_darkness_cd = make_buff( this, "unfurling_darkness_cd", find_spell( 341291 ) );
  buffs.surrender_to_madness_death =
      make_buff( this, "surrender_to_madness_death", find_talent_spell( "Surrender to Madness" ) )
          ->set_duration( timespan_t::zero() )
          ->set_default_value( 0.0 )
          ->set_chance( 1.0 );

  // Azerite Powers (Battle for Azeroth)
  buffs.chorus_of_insanity     = make_buff<buffs::chorus_of_insanity_t>( *this );
  buffs.harvested_thoughts     = make_buff<buffs::harvested_thoughts_t>( *this );
  buffs.whispers_of_the_damned = make_buff<buffs::whispers_of_the_damned_t>( *this );

  // Conduits (Shadowlands)
  buffs.mind_devourer = make_buff( this, "mind_devourer", find_spell( 338333 ) )
                            ->set_trigger_spell( conduits.mind_devourer )
                            ->set_chance( conduits.mind_devourer->effectN( 2 ).percent() );
  buffs.dissonant_echoes = make_buff( this, "dissonant_echoes", find_spell( 343144 ) );
}

void priest_t::init_rng_shadow()
{
  rppm.eternal_call_to_the_void = get_rppm( "eternal_call_to_the_void", legendary.eternal_call_to_the_void );
}

void priest_t::init_spells_shadow()
{
  // Talents
  // T15
  talents.fortress_of_the_mind = find_talent_spell( "Fortress of the Mind" );
  talents.death_and_madness    = find_talent_spell( "Death and Madness" );
  talents.unfurling_darkness   = find_talent_spell( "Unfurling Darkness" );
  // T25
  talents.body_and_soul = find_talent_spell( "Body and Soul" );
  talents.sanlayn       = find_talent_spell( "San'layn" );
  talents.intangibility = find_talent_spell( "intangibility" );
  // T30
  talents.twist_of_fate     = find_talent_spell( "Twist of Fate" );
  talents.misery            = find_talent_spell( "Misery" );
  talents.searing_nightmare = find_talent_spell( "Searing Nightmare" );
  // T35
  talents.last_word      = find_talent_spell( "Last Word" );
  talents.mind_bomb      = find_talent_spell( "Mind Bomb" );
  talents.psychic_horror = find_talent_spell( "Psychic Horror" );
  // T40
  talents.auspicious_spirits = find_talent_spell( "Auspicious Spirits" );
  talents.psychic_link       = find_talent_spell( "Psychic Link" );
  talents.shadow_crash       = find_talent_spell( "Shadow Crash" );
  // T45
  talents.damnation    = find_talent_spell( "Damnation" );
  talents.mindbender   = find_talent_spell( "Mindbender" );
  talents.void_torrent = find_talent_spell( "Void Torrent" );
  // T50
  talents.ancient_madness      = find_talent_spell( "Ancient Madness" );
  talents.hungering_void       = find_talent_spell( "Hungering Void" );
  talents.surrender_to_madness = find_talent_spell( "Surrender to Madness" );

  // General Spells
  specs.voidform            = find_specialization_spell( "Voidform" );
  specs.void_eruption       = find_specialization_spell( "Void Eruption" );
  specs.shadowy_apparitions = find_specialization_spell( "Shadowy Apparitions" );
  specs.shadow_priest       = find_specialization_spell( "Shadow Priest" );
  specs.dark_thoughts       = find_specialization_spell( "Dark Thoughts" );

  // Azerite
  azerite.chorus_of_insanity     = find_azerite_spell( "Chorus of Insanity" );
  azerite.death_throes           = find_azerite_spell( "Death Throes" );
  azerite.depth_of_the_shadows   = find_azerite_spell( "Depth of the Shadows" );
  azerite.searing_dialogue       = find_azerite_spell( "Searing Dialogue" );
  azerite.spiteful_apparitions   = find_azerite_spell( "Spiteful Apparitions" );
  azerite.thought_harvester      = find_azerite_spell( "Thought Harvester" );
  azerite.torment_of_torments    = find_azerite_spell( "Torment of Torments" );
  azerite.whispers_of_the_damned = find_azerite_spell( "Whispers of the Damned" );
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
  if ( name == "mind_blast" )
  {
    return new mind_blast_t( *this, options_str );
  }
  if ( name == "devouring_plague" )
  {
    return new devouring_plague_t( *this, options_str );
  }
  if ( name == "damnation" )
  {
    return new damnation_t( *this, options_str );
  }
  if ( name == "searing_nightmare" )
  {
    return new searing_nightmare_t( *this, options_str );
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

void priest_t::generate_apl_shadow()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* main         = get_action_priority_list( "main" );
  action_priority_list_t* cwc          = get_action_priority_list( "cwc" );
  action_priority_list_t* cds          = get_action_priority_list( "cds" );
  action_priority_list_t* boon         = get_action_priority_list( "boon" );
  action_priority_list_t* essences     = get_action_priority_list( "essences" );

  // Professions
  for ( const auto& profession_action : get_profession_actions() )
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
  default_list->add_action(
      "variable,name=all_dots_up,op=set,value="
      "dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking&dot.devouring_plague.ticking" );
  default_list->add_action( "variable,name=searing_nightmare_cutoff,op=set,value=spell_targets.mind_sear>3" );

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
  if ( race == RACE_VULPERA )
    default_list->add_action( "bag_of_tricks" );

  default_list->add_call_action_list( cwc );
  default_list->add_run_action_list( main );

  // BfA Essences for Pre-patch
  // Delete this after Shadowlands launch
  essences->add_action( "memory_of_lucid_dreams" );
  essences->add_action( "blood_of_the_enemy" );
  essences->add_action( "guardian_of_azeroth" );
  essences->add_action( "focused_azerite_beam,if=spell_targets.mind_sear>=2|raid_event.adds.in>60" );
  essences->add_action( "purifying_blast,if=spell_targets.mind_sear>=2|raid_event.adds.in>60" );
  essences->add_action(
      "concentrated_flame,line_cd=6,"
      "if=time<=10|full_recharge_time<gcd|target.time_to_die<5" );
  essences->add_action( "ripple_in_space" );
  essences->add_action( "reaping_flames" );
  essences->add_action( "worldvein_resonance" );
  essences->add_action( "the_unbound_force" );

  // CDs
  cds->add_action( this, "Power Infusion", "if=buff.voidform.up" );
  cds->add_action( this, covenant.fae_guardians, "Fae Guardians" );
  cds->add_action( this, covenant.mindgames, "Mindgames",
                   "target_if=insanity<90&(variable.all_dots_up|buff.voidform.up)" );
  cds->add_action( this, covenant.unholy_nova, "Unholy Nova", "if=raid_event.adds.in>20" );
  cds->add_action( this, covenant.boon_of_the_ascended, "Boon of the Ascended",
                   "if=!buff.voidform.up&!cooldown.void_eruption.up&spell_targets.mind_sear>1&!talent.searing_"
                   "nightmare.enabled|(buff.voidform.up&spell_targets.mind_sear<2&!talent.searing_nightmare.enabled)|("
                   "buff.voidform.up&talent.searing_nightmare.enabled)" );
  cds->add_call_action_list( essences );
  cds->add_action( "use_items", "Default fallback for usable items: Use on cooldown." );

  // APL to use when Boon of the Ascended is active
  boon->add_action( this, covenant.boon_of_the_ascended, "ascended_blast", "if=spell_targets.mind_sear<=3" );
  boon->add_action( this, covenant.boon_of_the_ascended, "ascended_nova",
                    "if=(spell_targets.mind_sear>2&talent.searing_nightmare.enabled|(spell_targets.mind_sear>1&!talent."
                    "searing_nightmare.enabled))&spell_targets.ascended_nova>1" );

  // Cast While Casting actions. Set at higher priority to short circuit interrupt conditions on Mind Sear/Flay
  cwc->add_talent( this, "Searing Nightmare",
                   "use_while_casting=1,target_if=(variable.searing_nightmare_cutoff&!cooldown.power_infusion.up)|("
                   "dot.shadow_word_pain.refreshable&spell_targets.mind_sear>1)",
                   "Use Searing Nightmare if you will hit enough targets and Power Infusion and Voidform are not "
                   "ready, or to refresh SW:P on two or more targets." );
  cwc->add_talent( this, "Searing Nightmare",
                   "use_while_casting=1,target_if=talent.searing_nightmare.enabled&dot.shadow_word_pain.refreshable&"
                   "spell_targets.mind_sear>2",
                   "Short Circuit Searing Nightmare condition to keep SW:P up in AoE" );
  cwc->add_action( this, "Mind Blast", "only_cwc=1",
                   "Only_cwc makes the action only usable during channeling and not as a regular action." );

  // Main APL, should cover all ranges of targets and scenarios
  main->add_call_action_list( this, covenant.boon_of_the_ascended, boon, "if=buff.boon_of_the_ascended.up" );
  main->add_action( this, "Void Eruption", "if=cooldown.power_infusion.up&insanity>=40",
                    "Sync up Voidform and Power Infusion Cooldowns and of using LotV pool insanity before casting." );
  main->add_action( this, "Shadow Word: Pain", "if=buff.fae_guardians.up&!debuff.wrathful_faerie.up",
                    "Make sure you put up SW:P ASAP on the target if Wrathful Faerie isn't active." );
  main->add_call_action_list( cds );
  main->add_action( this, "Mind Sear",
                    "target_if=talent.searing_nightmare.enabled&spell_targets.mind_sear>(variable.mind_sear_cutoff+1)&!"
                    "dot.shadow_word_pain.ticking&!cooldown.mindbender.up",
                    "High Priority Mind Sear action to refresh DoTs with Searing Nightmare" );
  main->add_talent( this, "Damnation", "target_if=!variable.all_dots_up",
                    "Prefer to use Damnation ASAP if any DoT is not up." );
  main->add_action( this, "Devouring Plague",
                    "target_if=(refreshable|insanity>75)&!cooldown.power_infusion.up&(!talent.searing_nightmare."
                    "enabled|(talent.searing_nightmare.enabled&!variable.searing_nightmare_cutoff))",
                    "Don't use Devouring Plague if you can get into Voidform instead, or if Searing Nightmare is "
                    "talented and will hit enough targets." );
  main->add_action( this, "Void Bolt", "if=spell_targets.mind_sear<(4+conduit.dissonant_echoes.enabled)&insanity<=85",
                    "Use VB on CD if you don't need to cast Devouring Plague, and there are less than 4 targets out (5 "
                    "with conduit)." );
  main->add_action( this, "Shadow Word: Death",
                    "target_if=target.health.pct<20|(pet.fiend.active&runeforge.shadowflame_prism.equipped)",
                    "Use Shadow Word: Death if the target is about to die or you have Shadowflame Prism equipped with "
                    "Mindbender or Shadowfiend active." );
  main->add_talent( this, "Surrender to Madness", "target_if=target.time_to_die<25&buff.voidform.down",
                    "Use Surrender to Madness on a target that is going to die at the right time." );
  main->add_talent( this, "Mindbender" );
  main->add_talent( this, "Void Torrent", "target_if=variable.all_dots_up&!buff.voidform.up&target.time_to_die>4",
                    "Use Void Torrent only if all DoTs are active and the target won't die during the channel." );
  main->add_action(
      this, "Shadow Word: Death",
      "if=runeforge.painbreaker_psalm.equipped&variable.dots_up&target.time_to_pct_20>(cooldown.shadow_word_death."
      "duration+gcd)",
      "Use SW:D with Painbreaker Psalm unless the target will be below 20% before the cooldown comes back" );
  main->add_talent(
      this, "Shadow Crash",
      "if=spell_targets.shadow_crash=1&(cooldown.shadow_crash.charges=3|debuff.shadow_crash_debuff.up|action.shadow_"
      "crash.in_flight|target.time_to_die<cooldown.shadow_crash.full_recharge_time)&raid_event."
      "adds.in>30",
      "Use all charges of Shadow Crash in a row on Single target, or if the boss is about to die." );
  main->add_talent( this, "Shadow Crash", "if=raid_event.adds.in>30&spell_targets.shadow_crash>1",
                    "Use Shadow Crash on CD unless there are adds incoming." );
  main->add_action(
      this, "Mind Sear",
      "target_if=spell_targets.mind_sear>variable.mind_sear_cutoff&buff.dark_thoughts.up,chain=1,interrupt_immediate=1,"
      "interrupt_if=ticks>=2",
      "Use Mind Sear to consume Dark Thoughts procs on AOE. TODO Confirm is this is a higher priority than redotting "
      "on AOE unless dark thoughts is about to time out" );
  main->add_action( this, "Mind Flay",
                    "if=buff.dark_thoughts.up&variable.dots_up,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&"
                    "cooldown.void_bolt.up",
                    "Use Mind Flay to consume Dark Thoughts procs on ST. TODO Confirm if this is a higher priority "
                    "than redotting unless dark thoughts is about to time out" );
  main->add_action( this, "Mind Blast",
                    "if=variable.dots_up&raid_event.movement.in>cast_time+0.5&spell_targets.mind_sear<4",
                    "TODO Verify target cap" );
  main->add_action( this, "Vampiric Touch",
                    "target_if=refreshable&target.time_to_die>6|(talent.misery.enabled&dot.shadow_word_pain."
                    "refreshable)|buff.unfurling_darkness.up" );
  main->add_action( this, "Shadow Word: Pain",
                    "if=refreshable&target.time_to_die>4&!talent.misery.enabled&talent.psychic_link.enabled&spell_"
                    "targets.mind_sear>2",
                    "Special condition to stop casting SW:P on off-targets when fighting 3 or more stacked mobs and "
                    "using Psychic Link and NOT Misery." );
  main->add_action(
      this, "Shadow Word: Pain",
      "target_if=refreshable&target.time_to_die>4&!talent.misery.enabled&(!talent.psychic_link.enabled|(talent.psychic_"
      "link.enabled&spell_targets.mind_sear<=2))",
      "Keep SW:P up on as many targets as possible, except when fighting 3 or more stacked mobs with Psychic Link." );
  main->add_action( this, "Mind Sear",
                    "target_if=spell_targets.mind_sear>variable.mind_sear_cutoff,chain=1,interrupt_immediate=1,"
                    "interrupt_if=ticks>=2" );
  main->add_action( this, "Mind Flay", "chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&cooldown.void_bolt.up" );
  main->add_action( this, "Shadow Word: Pain" );
}

void priest_t::init_background_actions_shadow()
{
  if ( specs.shadowy_apparitions->ok() )
  {
    background_actions.shadowy_apparitions = new actions::spells::shadowy_apparition_spell_t( *this );
  }

  if ( talents.psychic_link->ok() )
  {
    background_actions.psychic_link = new actions::spells::psychic_link_t( *this );
  }
}

// ==========================================================================
// Trigger Shadowy Apparitions on all targets affected by vampiric touch
// ==========================================================================
void priest_t::trigger_shadowy_apparitions( action_state_t* s )
{
  if ( !specs.shadowy_apparitions->ok() )
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
  if ( !talents.psychic_link->ok() )
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
// Check for the Hungering Void talent and find the debuff on that target
// ==========================================================================
bool priest_t::hungering_void_active( player_t* target ) const
{
  if ( !talents.hungering_void->ok() )
    return false;
  const priest_td_t* td = find_target_data( target );
  if ( !td )
    return false;

  return td->buffs.hungering_void->check();
}

// ==========================================================================
// Helper function to expire all tracking debuffs after Voidform expires
// ==========================================================================
void priest_t::remove_hungering_void_tracking()
{
  if ( !talents.hungering_void->ok() )
  {
    return;
  }

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && priest_td->buffs.hungering_void_tracking->check() )
    {
      priest_td->buffs.hungering_void_tracking->expire();
    }
  }
}

}  // namespace priestspace

// ==========================================================================
// Shadow Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
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
  const spell_data_t* mind_flay_spell;
  const spell_data_t* mind_sear_spell;
  bool only_cwc;

public:
  mind_blast_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mind_blast", p, p.specs.mind_blast ),
      mind_blast_insanity( priest().specs.shadow_priest->effectN( 12 ).resource( RESOURCE_INSANITY ) ),
      mind_flay_spell( p.specs.mind_flay ),
      mind_sear_spell( p.specs.mind_sear ),
      only_cwc( false )
  {
    add_option( opt_bool( "only_cwc", only_cwc ) );
    parse_options( options_str );

    affected_by_shadow_weaving = true;

    // This was removed from the Mind Blast spell and put on the Shadow Priest spell instead
    energize_amount = mind_blast_insanity;
    energize_amount *= 1 + priest().talents.fortress_of_the_mind->effectN( 2 ).percent();

    spell_power_mod.direct *= 1.0 + p.talents.fortress_of_the_mind->effectN( 4 ).percent();

    if ( priest().conduits.mind_devourer->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.mind_devourer.percent() );
    }

    cooldown->hasted     = true;
    usable_while_casting = use_while_casting = only_cwc;

    // Cooldown reduction
    apply_affecting_aura( p.find_rank_spell( "Mind Blast", "Rank 2", PRIEST_SHADOW ) );
  }

  void reset() override
  {
    priest_spell_t::reset();

    // Reset charges to initial value, since it can get out of sync when previous iteration ends with charge-giving
    // buffs up.
    cooldown->charges = data().charges();
  }

  bool ready() override
  {
    if ( only_cwc )
    {
      if ( !priest().buffs.dark_thought->check() )
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

  bool talbadars_stratagem_active() const
  {
    if ( !priest().legendary.talbadars_stratagem->ok() )
      return false;

    return priest().buffs.talbadars_stratagem->check();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_spell_t::composite_da_multiplier( s );

    if ( talbadars_stratagem_active() )
    {
      m *= 1 + priest().legendary.talbadars_stratagem->effectN( 1 ).percent();
    }

    return m;
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
    if ( priest().buffs.dark_thought->check() )
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
    // Decrementing a stack of dark thoughts will consume a max charge. Consuming a max charge loses you a current
    // charge. Therefore update_ready needs to not be called in that case.
    if ( priest().buffs.dark_thought->up() )
      priest().buffs.dark_thought->decrement();
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

  mind_sear_tick_t( priest_t& p, const spell_data_t* s )
    : priest_spell_t( "mind_sear_tick", p, s ), insanity_gain( p.specs.mind_sear_insanity->effectN( 1 ).percent() )
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

    trigger_dark_thoughts( s->target, priest().procs.dark_thoughts_sear, s );
  }
};

struct mind_sear_t final : public priest_spell_t
{
  mind_sear_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "mind_sear", p, p.specs.mind_sear )
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

    priest().refresh_talbadars_buff( s );
  }
};

struct shadow_word_death_t final : public priest_spell_t
{
  double execute_percent;
  double execute_modifier;
  double insanity_per_dot;

  shadow_word_death_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "shadow_word_death", p, p.specs.shadow_word_death ),
      execute_percent( data().effectN( 2 ).base_value() ),
      execute_modifier( data().effectN( 3 ).percent() ),
      insanity_per_dot( p.specs.painbreaker_psalm_insanity->effectN( 2 ).base_value() /
                        10 )  // Spell Data stores this as 100 not 1000 or 10
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

    if ( priest().legendary.kiss_of_death->ok() )
    {
      cooldown->duration += priest().legendary.kiss_of_death->effectN( 1 ).time_value();
    }

    cooldown->hasted = true;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = priest_spell_t::composite_target_da_multiplier( t );

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
      if ( priest().legendary.painbreaker_psalm->ok() )
      {
        int dots = 0;

        if ( const priest_td_t* td = priest().find_target_data( target ) )
        {
          bool swp_ticking = td->dots.shadow_word_pain->is_ticking();
          bool vt_ticking  = td->dots.vampiric_touch->is_ticking();

          dots = swp_ticking + vt_ticking;
        }

        double insanity_gain = dots * insanity_per_dot;

        priest().generate_insanity( insanity_gain, priest().gains.painbreaker_psalm, s->action );
      }

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
  dispersion_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "dispersion", p, p.specs.dispersion )
  {
    parse_options( options_str );

    ignore_false_positive = true;
    channeled             = true;
    harmful               = false;
    tick_may_crit         = false;
    hasted_ticks          = false;
    may_miss              = false;

    if ( priest().talents.intangibility->ok() )
    {
      cooldown->duration += priest().talents.intangibility->effectN( 1 ).time_value();
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
  silence_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "silence", p, p.specs.silence )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = is_interrupt = true;

    auto rank2 = p.find_rank_spell( "Silence", "Rank 2" );
    if ( rank2->ok() )
    {
      range += rank2->effectN( 1 ).base_value();
    }

    if ( priest().talents.last_word->ok() )
    {
      // Spell data has a negative value
      cooldown->duration += priest().talents.last_word->effectN( 1 ).time_value();
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
    : priest_spell_t( "vampiric_embrace", p, p.specs.vampiric_embrace )
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

  shadowy_apparition_damage_t( priest_t& p )
    : priest_spell_t( "shadowy_apparition", p, p.specs.shadowy_apparition ),
      insanity_gain( priest().talents.auspicious_spirits->effectN( 2 ).percent() )
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
};

struct shadowy_apparition_spell_t final : public priest_spell_t
{
  shadowy_apparition_spell_t( priest_t& p ) : priest_spell_t( "shadowy_apparitions", p, p.specs.shadowy_apparitions )
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
  }

  shadow_word_pain_t( priest_t& p, util::string_view options_str ) : shadow_word_pain_t( p, true )
  {
    parse_options( options_str );
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

      priest().refresh_talbadars_buff( s );
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest().specialization() != PRIEST_DISCIPLINE )
      return;

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
  unfurling_darkness_t( priest_t& p )
    : priest_spell_t( "unfurling_darkness", p,
                      p.dot_spells.vampiric_touch )  // Damage value is stored in Vampiric Touch
  {
    background                 = true;
    affected_by_shadow_weaving = true;
    energize_type              = action_energize::NONE;  // no insanity gain
    energize_amount            = 0;
    energize_resource          = RESOURCE_NONE;
    ignores_automatic_mastery  = 1;

    // Since we are re-using the Vampiric Touch spell disable the DoT
    dot_duration       = timespan_t::from_seconds( 0 );
    base_td_multiplier = spell_power_mod.tick = 0;
  }
};

// ==========================================================================
// Vampiric Touch
// ==========================================================================
struct vampiric_touch_t final : public priest_spell_t
{
  struct vampiric_touch_heal_t final : public priest_heal_t
  {
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

      // TODO: Confirm if this healing can proc trinkets/etc
      callbacks = false;
    }

    void trigger( double original_amount )
    {
      base_dd_min = base_dd_max = original_amount * data().effectN( 2 ).m_value();
      execute();
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

    if ( priest().talents.misery->ok() && casted )
    {
      child_swp             = new shadow_word_pain_t( priest(), false );
      child_swp->background = true;
    }

    if ( priest().talents.unfurling_darkness->ok() )
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
      if ( priest().talents.unfurling_darkness->ok() && !priest().buffs.unfurling_darkness_cd->check() )
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

    priest().refresh_talbadars_buff( s );
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

    if ( result_is_hit( d->state->result ) )
    {
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

      // TODO: Confirm if this healing can proc trinkets/etc
      callbacks = false;
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

      priest().refresh_talbadars_buff( s );
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) )
    {
      devouring_plague_heal->trigger( d->state->result_amount );
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

      priest().refresh_talbadars_buff( s );
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
          timespan_t::from_seconds( priest().talents.hungering_void->effectN( 3 ).base_value() ) ),
      hungering_void_crit_duration(
          timespan_t::from_seconds( priest().talents.hungering_void->effectN( 4 ).base_value() ) )
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

    // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/678
    // Dissonant Echoes proc is on the ghost impact, not on execute
    if ( !priest().bugs && priest().conduits.dissonant_echoes->ok() && priest().buffs.voidform->check() )
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

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest().trigger_shadowy_apparitions( s );

    if ( void_bolt_extension )
    {
      void_bolt_extension->target = s->target;
      void_bolt_extension->schedule_execute();
    }

    // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/678
    // Dissonant Echoes proc is on the ghost impact, not on execute
    if ( priest().bugs && priest().conduits.dissonant_echoes->ok() && priest().buffs.voidform->check() )
    {
      if ( rng().roll( priest().conduits.dissonant_echoes.percent() ) )
      {
        priest().cooldowns.void_bolt->reset( true );
        priest().procs.dissonant_echoes->occur();
      }
    }

    if ( priest().talents.hungering_void->ok() )
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
    : priest_spell_t( "void_eruption_damage", p, p.specs.void_eruption_damage ), void_bolt( nullptr )
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
    : priest_spell_t( "void_eruption", p, p.specs.void_eruption )
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
// Surrender to Madness
// ==========================================================================
struct void_eruption_stm_damage_t final : public priest_spell_t
{
  propagate_const<action_t*> void_bolt;

  void_eruption_stm_damage_t( priest_t& p )
    : priest_spell_t( "void_eruption_stm_damage", p, p.specs.void_eruption_damage ), void_bolt( nullptr )
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
  mind_bomb_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "mind_bomb", p, p.talents.mind_bomb )
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
  psychic_horror_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "psychic_horror", p, p.talents.psychic_horror )
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
// Void Torrent
// ==========================================================================
struct void_torrent_t final : public priest_spell_t
{
  double insanity_gain;

  void_torrent_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "void_torrent", p, p.talents.void_torrent ),
      insanity_gain( p.talents.void_torrent->effectN( 3 ).trigger()->effectN( 1 ).resource( RESOURCE_INSANITY ) )
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
  psychic_link_t( priest_t& p ) : priest_spell_t( "psychic_link", p, p.talents.psychic_link )
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
};

struct shadow_crash_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_crash_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "shadow_crash", p, p.talents.shadow_crash ),
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
// Searing Nightmare
// ==========================================================================
struct searing_nightmare_t final : public priest_spell_t
{
  propagate_const<shadow_word_pain_t*> child_swp;
  const spell_data_t* mind_sear_spell;

  searing_nightmare_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "searing_nightmare", p, p.talents.searing_nightmare ),
      child_swp( new shadow_word_pain_t( priest(), false ) ),
      mind_sear_spell( p.specs.mind_sear )
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
    double tdm = priest_spell_t::composite_target_da_multiplier( t );

    const priest_td_t* td = find_td( t );

    if ( td && td->dots.shadow_word_pain->is_ticking() )
    {
      tdm *= ( 1 + data().effectN( 1 ).percent() );
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
    : priest_spell_t( "damnation", p, p.talents.damnation ),
      child_swp( new shadow_word_pain_t( priest(), true ) ),  // Damnation still triggers SW:P as if it was hard casted
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

    if ( priest().talents.ancient_madness->ok() )
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

    // Create a stack change callback to adjust the number of mindblast charges.
    set_stack_change_callback( [ this ]( buff_t*, int old, int cur ) {
      priest().cooldowns.mind_blast->adjust_max_charges( cur - old );
    } );
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
// Ancient Madness
// ==========================================================================
struct ancient_madness_t final : public priest_buff_t<buff_t>
{
  ancient_madness_t( priest_t& p ) : base_t( p, "ancient_madness", p.talents.ancient_madness )
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
      double total_amount = amount * ( 1.0 + talents.surrender_to_madness->effectN( 2 ).percent() );

      amount_from_surrender_to_madness = amount * talents.surrender_to_madness->effectN( 2 ).percent();

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
  buffs.vampiric_embrace = make_buff( this, "vampiric_embrace", specs.vampiric_embrace );
  buffs.voidform         = make_buff<buffs::voidform_t>( *this );
  buffs.dispersion       = make_buff<buffs::dispersion_t>( *this );

  // Talents
  buffs.ancient_madness            = make_buff<buffs::ancient_madness_t>( *this );
  buffs.death_and_madness_buff     = make_buff<buffs::death_and_madness_buff_t>( *this );
  buffs.surrender_to_madness       = make_buff( this, "surrender_to_madness", talents.surrender_to_madness );
  buffs.surrender_to_madness_death = make_buff( this, "surrender_to_madness_death", talents.surrender_to_madness )
                                         ->set_duration( timespan_t::zero() )
                                         ->set_default_value( 0.0 )
                                         ->set_chance( 1.0 );
  buffs.unfurling_darkness =
      make_buff( this, "unfurling_darkness", talents.unfurling_darkness->effectN( 1 ).trigger() );
  buffs.unfurling_darkness_cd = make_buff( this, "unfurling_darkness_cd",
                                           talents.unfurling_darkness->effectN( 1 ).trigger()->effectN( 2 ).trigger() );
  buffs.void_torrent          = make_buff( this, "void_torrent", talents.void_torrent );

  // Conduits (Shadowlands)
  buffs.mind_devourer = make_buff( this, "mind_devourer", find_spell( 338333 ) )
                            ->set_trigger_spell( conduits.mind_devourer )
                            ->set_chance( conduits.mind_devourer->effectN( 2 ).percent() );

  buffs.dissonant_echoes = make_buff( this, "dissonant_echoes", find_spell( 343144 ) );

  buffs.talbadars_stratagem = make_buff( this, "talbadars_stratagem", find_spell( 342415 ) )
                                  ->set_duration( timespan_t::zero() )
                                  ->set_refresh_behavior( buff_refresh_behavior::DURATION );
}

void priest_t::init_rng_shadow()
{
  rppm.eternal_call_to_the_void = get_rppm( "eternal_call_to_the_void", legendary.eternal_call_to_the_void );
}

void priest_t::init_spells_shadow()
{
  // Talents
  // T15
  talents.fortress_of_the_mind       = find_talent_spell( "Fortress of the Mind" );
  talents.death_and_madness          = find_talent_spell( "Death and Madness" );
  talents.death_and_madness_insanity = find_spell( 321973 );
  talents.unfurling_darkness         = find_talent_spell( "Unfurling Darkness" );
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
  talents.hungering_void_buff  = find_spell( 345219 );
  talents.surrender_to_madness = find_talent_spell( "Surrender to Madness" );

  // General Spells
  specs.dark_thought         = find_spell( 341207 );
  specs.dark_thoughts        = find_specialization_spell( "Dark Thoughts" );
  specs.dispersion           = find_specialization_spell( "Dispersion" );
  specs.mind_flay            = find_specialization_spell( "Mind Flay" );
  specs.shadowy_apparition   = find_spell( 148859 );
  specs.shadowy_apparitions  = find_specialization_spell( "Shadowy Apparitions" );
  specs.shadowform           = find_specialization_spell( "Shadowform" );
  specs.silence              = find_specialization_spell( "Silence" );
  specs.vampiric_embrace     = find_specialization_spell( "Vampiric Embrace" );
  specs.void_bolt            = find_spell( 205448 );
  specs.voidform             = find_spell( 194249 );
  specs.void_eruption        = find_specialization_spell( "Void Eruption" );
  specs.void_eruption_damage = find_spell( 228360 );

  // Legendary Effects
  specs.cauterizing_shadows_health = find_spell( 336373 );
  specs.painbreaker_psalm_insanity = find_spell( 336167 );
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

  if ( legendary.eternal_call_to_the_void->ok() )
  {
    background_actions.eternal_call_to_the_void = new actions::spells::eternal_call_to_the_void_t( *this );
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
// Remove Hungering Void on any targets that don't match
// ==========================================================================
void priest_t::remove_hungering_void( player_t* target )
{
  if ( !talents.hungering_void->ok() )
    return;

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && ( priest_td->target != target ) && priest_td->buffs.hungering_void->check() )
    {
      priest_td->buffs.hungering_void->expire();
    }
  }
}

// Helper function to refresh talbadars buff
void priest_t::refresh_talbadars_buff( action_state_t* s )
{
  if ( !legendary.talbadars_stratagem->ok() )
    return;

  const priest_td_t* td = find_target_data( s->target );

  if ( !td )
    return;

  if ( td->dots.shadow_word_pain->is_ticking() && td->dots.vampiric_touch->is_ticking() &&
       td->dots.devouring_plague->is_ticking() )
  {
    timespan_t min_length = std::min( { td->dots.shadow_word_pain->remains(), td->dots.vampiric_touch->remains(),
                                        td->dots.devouring_plague->remains() } );

    if ( buffs.talbadars_stratagem->up() && min_length <= buffs.talbadars_stratagem->remains() )
      return;

    buffs.talbadars_stratagem->trigger( min_length );
  }
}

}  // namespace priestspace

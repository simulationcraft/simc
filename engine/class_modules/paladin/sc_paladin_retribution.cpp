#include <unordered_set>

#include "simulationcraft.hpp"
#include "dbc/specialization.hpp"
#include "sc_enums.hpp"
#include "sc_paladin.hpp"
#include "class_modules/apl/apl_paladin.hpp"

//
namespace paladin {

// Execution Sentence =======================================================

struct es_explosion_t : public paladin_spell_t
{
  double accumulated;

  es_explosion_t( paladin_t* p ) :
    paladin_spell_t( "execution_sentence", p, p->find_spell( 387113 ) ),
    accumulated( 0.0 )
  {
    dual = background = true;
    may_crit = false;

    attack_power_mod.direct = 0;
  }

  double calculate_direct_amount( action_state_t* state ) const
  {
    double amount = sim->averaged_range( base_da_min( state ), base_da_max( state ) );

    if ( round_base_dmg )
      amount = floor( amount + 0.5 );

    if ( amount == 0 && weapon_multiplier == 0 && attack_direct_power_coefficient( state ) == 0 &&
        spell_direct_power_coefficient( state ) == 0 && accumulated == 0 )
      return 0;

    double base_direct_amount = amount;
    double weapon_amount      = 0;

    if ( weapon_multiplier > 0 )
    {
      // x% weapon damage + Y
      // e.g. Obliterate, Shred, Backstab
      amount += calculate_weapon_damage( state->attack_power );
      amount *= weapon_multiplier;
      weapon_amount = amount;
    }
    amount += spell_direct_power_coefficient( state ) * ( state->composite_spell_power() );
    amount += attack_direct_power_coefficient( state ) * ( state->composite_attack_power() );

    // this is the only difference from normal direct_amount!
    amount += accumulated;

    // OH penalty, this applies to any OH attack even if is not based on weapon damage
    double weapon_slot_modifier = 1.0;
    if ( weapon && weapon->slot == SLOT_OFF_HAND )
    {
      weapon_slot_modifier = 0.5;
      amount *= weapon_slot_modifier;
      weapon_amount *= weapon_slot_modifier;
    }

    // Bonus direct damage historically appears to bypass the OH penalty for yellow attacks in-game
    // White damage bonuses (such as Jeweled Signet of Melandrus and older weapon enchants) do not
    if ( !special )
      amount += bonus_da( state ) * weapon_slot_modifier;
    else
      amount += bonus_da( state );

    amount *= state->composite_da_multiplier();

    // damage variation in WoD is based on the delta field in the spell data, applied to entire amount
    double delta_mod = amount_delta_modifier( state );
    if ( !sim->average_range && delta_mod > 0 )
      amount *= 1 + delta_mod / 2 * sim->averaged_range( -1.0, 1.0 );

    // AoE with decay per target
    if ( state->chain_target > 0 && chain_multiplier != 1.0 )
      amount *= pow( chain_multiplier, state->chain_target );

    if ( state->chain_target > 0 && chain_bonus_damage != 0.0 )
      amount *= std::max( 1.0 + chain_bonus_damage * state->chain_target, 0.0 );

    // AoE with static reduced damage per target
    if ( state->chain_target > 0 )
      amount *= base_aoe_multiplier;

    // Spell splits damage across all targets equally
    if ( state->action->split_aoe_damage )
      amount /= state->n_targets;

    // New Shadowlands AoE damage reduction based on total target count
    // The square root factor reaches its minimum when the number of targets is equal
    // to sim->max_aoe_enemies (usually 20), after that it remains constant.
    if ( state->chain_target >= state->action->full_amount_targets &&
        state->action->reduced_aoe_targets > 0.0 &&
        as<double>( state->n_targets ) > state->action->reduced_aoe_targets )
    {
      amount *= std::sqrt( state->action->reduced_aoe_targets / std::min<int>( sim->max_aoe_enemies, state->n_targets ) );
    }

    amount *= composite_aoe_multiplier( state );

    // Spell goes over the maximum number of AOE targets - ignore for enemies
    // Note that this split damage factor DOES affect spells that are supposed
    // to do full damage to the main target.
    if ( !state->action->split_aoe_damage &&
        state->n_targets > static_cast<size_t>( sim->max_aoe_enemies ) &&
        !state->action->player->is_enemy() )
    {
      amount *= sim->max_aoe_enemies / static_cast<double>( state->n_targets );
    }

    // Record initial amount to state
    state->result_raw = amount;

    if ( state->result == RESULT_GLANCE )
    {
      double delta_skill = ( state->target->level() - player->level() ) * 5.0;

      if ( delta_skill < 0.0 )
        delta_skill = 0.0;

      double max_glance = 1.3 - 0.03 * delta_skill;

      if ( max_glance > 0.99 )
        max_glance = 0.99;
      else if ( max_glance < 0.2 )
        max_glance = 0.20;

      double min_glance = 1.4 - 0.05 * delta_skill;

      if ( min_glance > 0.91 )
        min_glance = 0.91;
      else if ( min_glance < 0.01 )
        min_glance = 0.01;

      if ( min_glance > max_glance )
      {
        double temp = min_glance;
        min_glance  = max_glance;
        max_glance  = temp;
      }

      amount *= sim->averaged_range( min_glance, max_glance );  // 0.75 against +3 targets.
    }

    if ( !sim->average_range )
      amount = floor( amount + rng().real() );

    if ( amount < 0 )
    {
      amount = 0;
    }

    if ( sim->debug )
    {
      sim->print_debug(
          "{} direct amount for {}: amount={} initial_amount={} weapon={} base={} s_mod={} s_power={} "
          "a_mod={} a_power={} mult={} w_mult={} w_slot_mod={} bonus_da={}",
          *player, *this, amount, state->result_raw, weapon_amount, base_direct_amount,
          spell_direct_power_coefficient( state ), state->composite_spell_power(),
          attack_direct_power_coefficient( state ), state->composite_attack_power(), state->composite_da_multiplier(),
          weapon_multiplier, weapon_slot_modifier, bonus_da( state ) );
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
};

struct execution_sentence_t : public paladin_melee_attack_t
{
  execution_sentence_t( paladin_t* p, util::string_view options_str ) :
    paladin_melee_attack_t( "execution_sentence", p, p->talents.execution_sentence )
  {
    parse_options( options_str );

    // disable if not talented
    if ( ! ( p->talents.execution_sentence->ok() ) )
      background = true;

    // ... this appears to be true for the base damage only,
    // and is not automatically obtained from spell data.
    affected_by.highlords_judgment = true;

    // unclear why this is needed...
    cooldown->duration = data().cooldown();
  }

  void init() override
  {
    paladin_melee_attack_t::init();
    snapshot_flags |= STATE_TARGET_NO_PET | STATE_MUL_TA | STATE_MUL_DA;
    update_flags &= ~STATE_TARGET;
    update_flags |= STATE_MUL_TA | STATE_MUL_DA;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p()->talents.judge_jury_and_executioner->ok() )
    {
      p()->buffs.judge_jury_and_executioner->trigger( p()->buffs.judge_jury_and_executioner->data().max_stacks() );
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuff.execution_sentence->trigger();
    }
  }
};

struct expurgation_t : public paladin_spell_t
{
  expurgation_t( paladin_t* p ):
    paladin_spell_t( "expurgation", p, p->find_spell( 383346 ) )
  {
    // Jurisdiction doesn't increase Expurgation's damage in-game
    // It's increasing Spell Direct Amount instead of Spell Periodic Amount
    if ( p->talents.jurisdiction->ok() && p->bugs)
    {
      base_multiplier *= 1.0 + p->talents.jurisdiction->effectN( 4 ).percent();
    }
  }

  double get_bank( dot_t* d )
  {
    if ( !d->is_ticking() )
      return 0.0;

    auto state = d->state;
    return calculate_tick_amount( state, d->current_stack() ) * d->ticks_left_fractional();
  }
};

// Blade of Justice =========================================================

struct blade_of_justice_t : public paladin_melee_attack_t
{

  blade_of_justice_t( paladin_t* p, util::string_view options_str ) :
    paladin_melee_attack_t( "blade_of_justice", p, p->talents.blade_of_justice )
  {
    parse_options( options_str );

    if ( p->talents.blade_of_vengeance->ok() )
    {
      base_aoe_multiplier *= p->find_spell( 404358 )->effectN( 1 ).ap_coeff() / attack_power_mod.direct;
      aoe = -1;
      reduced_aoe_targets = 5;
    }

    triggers_higher_calling   = true;
    affected_by.highlords_judgment = true;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p()->spells.consecrated_blade->ok() && p()->cooldowns.consecrated_blade_icd->up() )
    {
      p()->active.background_cons->schedule_execute();
      p()->cooldowns.consecrated_blade_icd->start();
    }
  }

  void impact( action_state_t* state ) override
  {
    paladin_melee_attack_t::impact( state );

    if ( p()->talents.expurgation->ok() )
    {
      p()->active.expurgation->target = state->target;
      p()->active.expurgation->execute();
    }
  }
};

// Divine Storm =============================================================

struct divine_storm_echo_tempest_t : public paladin_melee_attack_t
{
  divine_storm_echo_tempest_t( paladin_t* p )
    : paladin_melee_attack_t( "divine_storm_echo_tempest", p, p->find_spell( 423593 ) )
  {
    background = true;

    aoe = -1;
    base_multiplier *= p->buffs.echoes_of_wrath->data().effectN( 1 ).percent();
    clears_judgment = false;
  }
};

struct divine_storm_tempest_t : public paladin_melee_attack_t
{
  divine_storm_tempest_t( paladin_t* p ) :
    paladin_melee_attack_t( "divine_storm_tempest", p, p->find_spell( 224239 ) )
  {
    background = true;

    aoe = -1;
    base_multiplier *= p->talents.tempest_of_the_lightbringer->effectN( 1 ).percent();
    clears_judgment = false;
  }
};

struct divine_storm_echo_t : public paladin_melee_attack_t
{
  divine_storm_echo_tempest_t* tempest;
  divine_storm_echo_t( paladin_t* p, double multiplier )
    : paladin_melee_attack_t( "divine_storm_echo", p, p->talents.divine_storm ), tempest( nullptr )
  {
    background = true;

    aoe = -1;
    base_multiplier *= multiplier;
    clears_judgment                   = false;
    base_costs[ RESOURCE_HOLY_POWER ] = 0;

    if ( p->talents.tempest_of_the_lightbringer->ok() )
    {
      tempest = new divine_storm_echo_tempest_t( p );
      add_child( tempest );
    }
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p()->talents.tempest_of_the_lightbringer->ok() )
      tempest->schedule_execute();
  }
};

struct divine_storm_t: public holy_power_consumer_t<paladin_melee_attack_t>
{
  divine_storm_tempest_t* tempest;
  divine_storm_echo_t* echo;
  divine_storm_echo_t* sunrise_echo;

  divine_storm_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "divine_storm", p, p->talents.divine_storm ),
    tempest( nullptr ), echo( nullptr ), sunrise_echo( nullptr )
  {
    parse_options( options_str );

    if ( !( p->talents.divine_storm->ok() ) )
      background = true;

    is_divine_storm = true;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();

    if ( p->talents.tempest_of_the_lightbringer->ok() )
    {
      tempest = new divine_storm_tempest_t( p );
      add_child( tempest );
    }

    if ( p->talents.herald_of_the_sun.second_sunrise->ok() )
    {
      sunrise_echo = new divine_storm_echo_t( p, p->talents.herald_of_the_sun.second_sunrise->effectN( 2 ).percent() );
      add_child( sunrise_echo );
    }
  }

  divine_storm_t( paladin_t* p, bool is_free, double mul ) :
    holy_power_consumer_t( "divine_storm", p, p->talents.divine_storm ),
    tempest( nullptr ), echo( nullptr ), sunrise_echo( nullptr )
  {
    is_divine_storm = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();

    background = is_free;
    base_multiplier *= mul;

    if ( p->talents.tempest_of_the_lightbringer->ok() )
    {
      tempest = new divine_storm_tempest_t( p );
      add_child( tempest );
    }

    if ( p->talents.herald_of_the_sun.second_sunrise->ok() )
    {
      sunrise_echo = new divine_storm_echo_t( p, p->talents.herald_of_the_sun.second_sunrise->effectN( 2 ).percent() * mul );
      add_child( sunrise_echo );
    }
  }

  void execute() override
  {
    holy_power_consumer_t::execute();

    if ( p()->talents.tempest_of_the_lightbringer->ok() )
      tempest->schedule_execute();

    if ( p()->buffs.echoes_of_wrath->up() )
    {
      p()->buffs.echoes_of_wrath->expire();
      echo->start_action_execute_event( 700_ms ); // Maybe this 700ms is Echoes of Wrath effect 2? It's more like 600-700ms
    }

    if ( sunrise_echo && p()->cooldowns.second_sunrise_icd->up() )
    {
      if ( rng().roll( p()->talents.herald_of_the_sun.second_sunrise->effectN( 1 ).percent() ) )
      {
        p()->cooldowns.second_sunrise_icd->start();
        // TODO(mserrano): validate the correct delay here
        sunrise_echo->start_action_execute_event( 200_ms );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      paladin_td_t* target_data = td( s->target );
      if ( p()->talents.sanctify->ok() )
        target_data->debuff.sanctify->trigger();

      if ( s->result == RESULT_CRIT && p()->talents.herald_of_the_sun.sun_sear->ok() )
      {
        p()->active.sun_sear->target = s->target;
        p()->active.sun_sear->execute();
      }
    }
  }
};

struct templars_verdict_echo_t : public paladin_melee_attack_t
{
  bool is_fv;
  templars_verdict_echo_t( paladin_t* p ) :
    paladin_melee_attack_t(( p->talents.final_verdict->ok() ) ? "final_verdict_echo" : "templars_verdict_echo",
      p,
      ( p->talents.final_verdict->ok() ) ? ( p->find_spell( 383328 ) ) : ( p->find_specialization_spell( "Templar's Verdict" ) ) ),
      is_fv(p->talents.final_verdict->ok())
  {
    background = true;
    base_multiplier *= p->buffs.echoes_of_wrath->data().effectN( 1 ).percent();
    clears_judgment                   = false;
    base_costs[ RESOURCE_HOLY_POWER ] = 0;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    // FV Echo can reset Hammer of Wrath
    if ( is_fv )
    {
      double proc_chance = data().effectN( 2 ).percent();
      if ( rng().roll( proc_chance ) )
      {
        p()->cooldowns.judgment->reset( true );
      }
    }
  }
};

struct templars_verdict_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  // Templar's Verdict damage is stored in a different spell
  struct templars_verdict_damage_t : public paladin_melee_attack_t
  {
    templars_verdict_damage_t( paladin_t *p ) :
      paladin_melee_attack_t( "templars_verdict_dmg", p, p->find_spell( 224266 ) )
    {
      dual = background = true;

      // spell data please?
      aoe = 0;
    }
  };

  bool is_fv;
  templars_verdict_echo_t* echo;

  templars_verdict_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t(
      ( p->talents.final_verdict->ok() ) ? "final_verdict" : "templars_verdict",
      p,
      ( p->talents.final_verdict->ok() ) ? ( p->find_spell( 383328 ) ) : ( p->find_specialization_spell( "Templar's Verdict" ) ) ),
    is_fv( p->talents.final_verdict->ok() ),
    echo(nullptr)
  {
    parse_options( options_str );

    // spell is not usable without a 2hander
    if ( p->items[ SLOT_MAIN_HAND ].dbc_inventory_type() != INVTYPE_2HWEAPON )
      background = true;

    // wtf is happening in spell data?
    aoe = 0;

    if ( ! is_fv ) {
      callbacks = false;
      may_block = false;

      impact_action = new templars_verdict_damage_t( p );
      impact_action->stats = stats;

      // Okay, when did this get reset to 1?
      weapon_multiplier = 0;
    }
  }

  void record_data( action_state_t* state ) override {
    if ( is_fv )
      holy_power_consumer_t::record_data( state );
  }

  void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // missed/dodged/parried TVs do not consume Holy Power
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state->result ) && c > 0 )
    {
      p()->resource_gain( RESOURCE_HOLY_POWER, c, p()->gains.hp_templars_verdict_refund );
    }

    if ( p()->buffs.empyrean_legacy->up() )
    {
      p()->active.empyrean_legacy->schedule_execute();
      p()->buffs.empyrean_legacy->expire();
    }

    if ( is_fv )
    {
      double proc_chance = data().effectN( 2 ).percent();
      if ( rng().roll( proc_chance ) )
      {
        p()->cooldowns.judgment->reset( true );
      }
    }

    if ( !background )
    {
      if ( p()->buffs.echoes_of_wrath->up() )
      {
        p()->buffs.echoes_of_wrath->expire();
        echo->target = execute_state->target;
        echo->start_action_execute_event( 700_ms );
      }
    }
  }
};

// Judgment - Retribution =================================================================

struct judgment_ret_t : public judgment_t
{
  int holy_power_generation;
  bool local_is_divine_toll;

  judgment_ret_t( paladin_t* p, util::string_view name, util::string_view options_str, double mul = 1.0 ) :
    judgment_t( p, name ),
    holy_power_generation( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) ),
    local_is_divine_toll( false )
  {
    parse_options( options_str );
    base_multiplier *= mul;

    if ( p->talents.blessed_champion->ok() )
    {
      aoe = as<int>( 1 + p->talents.blessed_champion->effectN( 4 ).base_value() );
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }
  }

  judgment_ret_t( paladin_t* p, util::string_view name, bool is_divine_toll, double mul = 1.0 )
    : judgment_t( p, name ),
      holy_power_generation( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) ),
      local_is_divine_toll( is_divine_toll )
  {
    // This is for Divine Toll's background judgments
    background = true;
    cooldown   = p->get_cooldown( "dummy_cd" );
    base_multiplier *= mul;

    // according to skeletor this is given the bonus of 326011
    if ( is_divine_toll )
      base_multiplier *= 1.0 + p->find_spell( 326011 )->effectN( 1 ).percent();
    // This is called for Divine Resonance Judgments, they benefit from Blessed Champion
    else
    {
      if ( p->talents.blessed_champion->ok() )
      {
        aoe = as<int>( 1 + p->talents.blessed_champion->effectN( 4 ).base_value() );
        base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
      }
    }

    // we don't do the blessed champion stuff here; DT judgments do not seem to cleave
  }

  void execute() override
  {
    judgment_t::execute();

    if ( p()->spec.judgment_3->ok() )
      p()->resource_gain( RESOURCE_HOLY_POWER, holy_power_generation, p()->gains.judgment );
  }

  void impact(action_state_t* s) override
  {
    judgment_t::impact( s );
    double mastery_chance = p()->cache.mastery() * p()->mastery.highlords_judgment->effectN( 4 ).mastery_value();
    if ( p()->talents.boundless_judgment->ok() )
      mastery_chance *= 1.0 + p()->talents.boundless_judgment->effectN( 3 ).percent();
   if ( p()->talents.highlords_wrath->ok() )
      mastery_chance *= 1.0 + p()->talents.highlords_wrath->effectN( 3 ).percent() / p()->talents.highlords_wrath->effectN( 2 ).base_value();

    if ( rng().roll( mastery_chance ) )
    {
      p()->active.highlords_judgment->set_target( s->target );
      p()->active.highlords_judgment->execute();
    }
  }
};

// Wake of Ashes (Retribution) ================================================

struct truths_wake_t : public paladin_spell_t
{
  truths_wake_t( paladin_t* p ) :
    paladin_spell_t( "truths_wake", p, p->find_spell( 403695 ) )
  {
    hasted_ticks = tick_may_crit = true;
  }

  void tick( dot_t* d ) override
  {
    paladin_spell_t::tick( d );

    if ( d->state->result == RESULT_CRIT && p()->talents.burn_to_ash->ok() && d->remains() > 0_ms )
    {
      d->adjust_duration( timespan_t::from_seconds( p()->talents.burn_to_ash->effectN( 1 ).base_value() ) );
    }
  }
};

struct seething_flames_t : public paladin_spell_t
{
  seething_flames_t( paladin_t* p, util::string_view name, int spell_id ) :
    paladin_spell_t( name, p, p->find_spell( spell_id ) )
  {
    background = true;
    // This is from logs; I assume it must be in spelldata somewhere but have not yet found it.
    base_aoe_multiplier *= 0.6;

    // what's up with spelldata and not being aoe
    aoe = -1;
  }
};

struct seething_flames_event_t : public event_t
{
  seething_flames_t* action;
  paladin_t* paladin;
  player_t* target;

  seething_flames_event_t( paladin_t* p, player_t* tgt, seething_flames_t* spell, timespan_t delay ) :
    event_t( *p, delay ), action( spell ), paladin( p ), target( tgt )
  {
  }

  const char* name() const override
  { return "seething_flames_delay"; }

  void execute() override
  {
    action->set_target( target );
    action->schedule_execute();
  }
};

struct wake_of_ashes_t : public paladin_spell_t
{
  truths_wake_t* truths_wake;
  seething_flames_t* seething_flames[2];

  wake_of_ashes_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "wake_of_ashes", p, p->talents.wake_of_ashes ),
    truths_wake( nullptr )
  {
    parse_options( options_str );

    if ( !( p->talents.wake_of_ashes->ok() ) )
      background = true;

    may_crit = true;

    if ( p->talents.seething_flames->ok() )
    {
      // This is from logs; I assume it must be in spelldata somewhere but have not yet found it.
      base_aoe_multiplier *= 0.6;
    }

    aoe = -1;

    truths_wake = new truths_wake_t( p );
    add_child( truths_wake );

    if ( p->talents.seething_flames->ok() )
    {
      seething_flames[0] = new seething_flames_t( p, "seething_flames_0", 405345 );
      seething_flames[1] = new seething_flames_t( p, "seething_flames_1", 405350 );
      add_child( seething_flames[0] );
      add_child( seething_flames[1] );
    }
  }

  void impact( action_state_t* s) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      truths_wake->set_target( s->target );
      truths_wake->execute();
    }
  }

  void execute() override
  {
    if ( p()->talents.radiant_glory->ok() )
    {
      bool do_avatar = p()->talents.herald_of_the_sun.suns_avatar->ok() &&
                       !( p()->buffs.avenging_wrath->up() );
      if ( p()->talents.avenging_wrath->ok() )
      {
        if ( !p()->buffs.avenging_wrath->up() )
        {
          p()->active.background_avenging_wrath->execute_on_target( p() );
        }
        p()->buffs.avenging_wrath->extend_duration_or_trigger( timespan_t::from_seconds( 8 ) );
      }

      if ( do_avatar )
      {
        p()->apply_avatar_dawnlights();
      }
    }

    paladin_spell_t::execute();

    if ( p()->talents.seething_flames->ok() )
    {
      for ( int i = 0; i < as<int>( p()->talents.seething_flames->effectN( 1 ).base_value() ); i++ )
      {
        make_event<seething_flames_event_t>( *sim, p(), execute_state->target, seething_flames[i], timespan_t::from_millis( 500 * (i + 1) ) );
      }
    }
    if ( p()->talents.templar.lights_guidance->ok() )
    {
      p()->buffs.templar.hammer_of_light_ready->trigger();
    }

    if ( p()->talents.templar.sacrosanct_crusade->ok() )
    {
      p()->buffs.templar.sacrosanct_crusade->trigger();
    }

    if ( p()->talents.herald_of_the_sun.dawnlight->ok() )
    {
      p()->buffs.herald_of_the_sun.dawnlight->trigger( as<int>( p()->talents.herald_of_the_sun.dawnlight->effectN( 1 ).base_value() ) );
    }

    if ( p()->talents.herald_of_the_sun.aurora->ok() && p()->cooldowns.aurora_icd->up() )
    {
      p()->cooldowns.aurora_icd->start();
      p()->buffs.divine_purpose->trigger();
    }

    if ( p()->sets->has_set_bonus( PALADIN_RETRIBUTION, TWW1, B4 ) )
    {
      p()->buffs.rise_from_ash->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p()->buffs.templar.hammer_of_light_ready->up() )
    {
      return false;
    }
    return paladin_spell_t::target_ready( candidate_target );
  }
};

struct adjudication_blessed_hammer_tick_t : public paladin_spell_t
{
  adjudication_blessed_hammer_tick_t( paladin_t* p )
    : paladin_spell_t( "blessed_hammer_tick", p, p->find_spell( 404139 ) )
  {
    aoe = -1;
    background = dual = direct_tick = true;
    callbacks = true;
    radius = 9.0;
    may_crit = true;
  }
};

struct adjudication_blessed_hammer_t : public paladin_spell_t
{
  adjudication_blessed_hammer_tick_t* hammer;
  // TODO: make this configurable
  double num_strikes;

  adjudication_blessed_hammer_t( paladin_t* p )
    : paladin_spell_t( "blessed_hammer", p, /* p->find_spell( 404140 ) */ spell_data_t::nil() ),
      hammer( new adjudication_blessed_hammer_tick_t( p ) ), num_strikes( 2 )
  {
    background = true;

    dot_duration = 0_ms;
    base_tick_time = 0_ms;
    may_miss = false;
    tick_may_crit = true;

    add_child( hammer );
  }

  void execute() override
  {
    paladin_spell_t::execute();
    timespan_t initial_delay = num_strikes < 3 ? data().duration() * 0.25 : 0_ms;
    // Let strikes be a decimal rather than int, and roll a random number to decide
    // hits each time.
    int roll_strikes = static_cast<int>(floor(num_strikes));
    if ( num_strikes - roll_strikes != 0 && rng().roll( num_strikes - roll_strikes ))
      roll_strikes += 1;
    if (roll_strikes > 0)
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
          .target( execute_state->target )
          // spawn at feet of player
          .x( execute_state->action->player->x_position )
          .y( execute_state->action->player->y_position )
          .pulse_time( /* TODO: replace with data().duration() */ timespan_t::from_seconds( 5 ) / roll_strikes )
          .n_pulses( roll_strikes )
          .start_time( sim->current_time() + initial_delay )
          .action( hammer ), true );
  }
};

struct base_templar_strike_t : public paladin_melee_attack_t
{
  base_templar_strike_t( util::string_view n, paladin_t* p, util::string_view options_str, const spell_data_t* s )
    : paladin_melee_attack_t( n, p, s )
  {
    parse_options( options_str );

    if ( !p->talents.templar_strikes->ok() )
      background = true;

    if ( p->talents.blessed_champion->ok() )
    {
      aoe = as<int>( 1 + p->talents.blessed_champion->effectN( 4 ).base_value() );
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }

    triggers_higher_calling = true;
  }

  void impact( action_state_t *s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s->result ) && p()->talents.empyrean_power->ok() )
    {
      if ( rng().roll( p()->talents.empyrean_power->effectN( 1 ).percent() ) )
      {
        p()->procs.empyrean_power->occur();
        p()->buffs.empyrean_power->trigger();
      }
    }
  }
};

struct templar_strike_t : public base_templar_strike_t
{
  templar_strike_t( paladin_t* p, util::string_view options_str )
    : base_templar_strike_t( "templar_strike", p, options_str, p->find_spell( 407480 ) )
  {
  }

  void execute() override
  {
    base_templar_strike_t::execute();
    p()->buffs.templar_strikes->trigger();
  }

  bool ready() override
  {
    bool orig = paladin_melee_attack_t::ready();
    return orig && !(p()->buffs.templar_strikes->up());
  }
};

struct templar_slash_dot_t : public paladin_spell_t
{
  templar_slash_dot_t( paladin_t* p )
    : paladin_spell_t( "templar_slash_dot", p, p->find_spell( 447142 ) )
  {
    background = true;
    hasted_ticks = false;
    affected_by.crusade = affected_by.avenging_wrath = affected_by.highlords_judgment = false;
  }

  void init() override
  {
    paladin_spell_t::init();
    snapshot_flags = update_flags = STATE_MUL_SPELL_TA | STATE_TGT_MUL_TA;
  }
};

struct templar_slash_t : public base_templar_strike_t
{
  templar_slash_dot_t* dot;

  templar_slash_t( paladin_t* p, util::string_view options_str )
    : base_templar_strike_t( "templar_slash", p, options_str, p->find_spell( 406647 ) ),
      dot( new templar_slash_dot_t( p ) )
  {
    add_child( dot );
  }

  void execute() override
  {
    base_templar_strike_t::execute();
    p()->buffs.templar_strikes->expire();
  }

  void impact( action_state_t* s ) override
  {
    base_templar_strike_t::impact( s );

    dot->target = s->target;
    // TODO: figure out where this formula comes from
    double mult = 0.5;
    dot->base_td = ( s->result_total * mult ) / 4;
    dot->execute();
  }

  bool ready() override
  {
    bool orig = paladin_melee_attack_t::ready();
    return orig && p()->buffs.templar_strikes->up();
  }
};

struct highlords_judgment_t : public paladin_spell_t
{
  highlords_judgment_t( paladin_t* p ) : paladin_spell_t( "highlords_judgment", p, p->find_spell( 383921 ) )
  {
    background = true;
    skip_es_accum = true;
  }
};

struct sun_sear_t : public paladin_spell_t
{
  sun_sear_t( paladin_t* p ) :
    paladin_spell_t( "sun_sear", p, p->find_spell( 431414 ) )
  {
    hasted_ticks = tick_may_crit = true;
  }
};

void paladin_t::trigger_es_explosion( player_t* target )
{
  double ta = 0.0;
  double accumulated = get_target_data( target )->debuff.execution_sentence->get_accumulated_damage();

  sim->print_debug( "{}'s execution_sentence has accumulated {} total additional damage.", target->name(), accumulated );
  ta += accumulated;

  es_explosion_t* explosion = static_cast<es_explosion_t*>( active.es_explosion );
  explosion->set_target( target );
  explosion->accumulated = ta;
  explosion->schedule_execute();
}

divine_exaction_ret_t::divine_exaction_ret_t( paladin_t* p )
  : paladin_spell_t( "divine_exaction_ret", p ),
    judgment( new judgment_ret_t( p, "judgment_divine_exaction", "",
                                  p->talents.templar.divine_exaction->effectN( 2 ).percent() ) ),
    hammer_of_wrath( new hammer_of_wrath_t( p, "hammer_of_wrath_divine_exaction",
                                            p->talents.templar.divine_exaction->effectN( 2 ).percent() ) )
{
  background = true;
}
void divine_exaction_ret_t::execute()
{
  int times = p()->talents.templar.divine_exaction->effectN( 1 ).base_value() - 1;
  if (p()->wings_up())
  {
    hammer_of_wrath->execute_on_target( execute_state->target );
    for ( int i = 0; i < times; i++ )
    {
      make_event<delayed_execute_event_t>( *sim, p(), hammer_of_wrath, execute_state->target, 300_ms );
    }
  }
  else
  {
    judgment->execute_on_target( execute_state->target );
    for ( int i = 0; i < times; i++ )
    {
      make_event<delayed_execute_event_t>( *sim, p(), judgment, execute_state->target, 300_ms * ( i + 1 ) );
    }
  }
}

// Initialization

void paladin_t::create_ret_actions()
{
  if ( talents.empyrean_legacy->ok() )
  {
    double empyrean_legacy_mult = 1.0 + talents.empyrean_legacy->effectN( 2 ).percent();
    active.empyrean_legacy = new divine_storm_t( this, true, empyrean_legacy_mult );
  }
  else
  {
    active.empyrean_legacy = nullptr;
  }

  if ( talents.execution_sentence->ok() )
  {
    active.es_explosion = new es_explosion_t( this );
  }
  else
  {
    active.es_explosion = nullptr;
  }

  if ( talents.adjudication->ok() )
  {
    active.background_blessed_hammer = new adjudication_blessed_hammer_t( this );
  }

  if (talents.expurgation->ok())
  {
    active.expurgation = new expurgation_t( this );
  }

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    active.divine_toll = new judgment_ret_t( this, "divine_toll_judgment", true );
    active.divine_resonance = new judgment_ret_t( this, "divine_resonance_judgment", false );
    active.highlords_judgment = new highlords_judgment_t( this );
    if ( talents.herald_of_the_sun.sun_sear->ok() )
    {
      active.sun_sear = new sun_sear_t( this );
    }
  }
}

action_t* paladin_t::create_action_retribution( util::string_view name, util::string_view options_str )
{
  if ( name == "blade_of_justice"          ) return new blade_of_justice_t         ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "wake_of_ashes"             ) return new wake_of_ashes_t            ( this, options_str );
  if ( name == "templar_strike"            ) return new templar_strike_t           ( this, options_str );
  if ( name == "templar_slash"             ) return new templar_slash_t            ( this, options_str );
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    if ( name == "judgment" ) return new judgment_ret_t( this, "judgment", options_str );
  }

  return nullptr;
}

void paladin_t::create_buffs_retribution()
{
  buffs.rush_of_light = make_buff( this, "rush_of_light", find_spell( 407065 ) )
    ->add_invalidate( CACHE_HASTE )
    ->set_default_value( talents.rush_of_light->effectN( 1 ).percent() );

  buffs.templar_strikes = make_buff( this, "templar_strikes", find_spell( 406648 ) );
  buffs.empyrean_power = make_buff( this, "empyrean_power", find_spell( 326733 ) )
                          ->set_trigger_spell( talents.empyrean_power );
  buffs.judge_jury_and_executioner = make_buff( this, "judge_jury_and_executioner", find_spell( 1253174 ) );

  // legendaries
  buffs.empyrean_legacy = make_buff( this, "empyrean_legacy", find_spell( 387178 ) );

  buffs.echoes_of_wrath = make_buff( this, "echoes_of_wrath", find_spell( 423590 ) );

  buffs.rise_from_ash = make_buff( this, "rise_from_ash", find_spell( 454693 ) );
  buffs.winning_streak = make_buff( this, "winning_streak", find_spell( 1216828 ) )
    ->set_default_value_from_effect( 1 );
  buffs.all_in = make_buff( this, "all_in", find_spell( 1216837 ) )
    ->set_default_value_from_effect( 1 );

  buffs.art_of_war = make_buff( this, "art_of_war", find_spell( 406086 ) );
}

void paladin_t::init_rng_retribution()
{
}

void paladin_t::init_spells_retribution()
{
  // Talents
  talents.blade_of_justice            = find_talent_spell( talent_tree::SPECIALIZATION, "Blade of Justice" );
  talents.divine_storm                = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Storm" );
  talents.swift_justice               = find_talent_spell( talent_tree::SPECIALIZATION, "Swift Justice" );
  talents.light_of_justice            = find_talent_spell( talent_tree::SPECIALIZATION, "Light of Justice" );
  talents.expurgation                 = find_talent_spell( talent_tree::SPECIALIZATION, "Expurgation" );
  talents.judgment_of_justice         = find_talent_spell( talent_tree::SPECIALIZATION, "Judgment of Justice");
  talents.final_verdict               = find_talent_spell( talent_tree::SPECIALIZATION, "Final Verdict" );
  talents.improved_blade_of_justice   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Blade of Justice" );
  talents.holy_blade                  = find_talent_spell( talent_tree::SPECIALIZATION, "Holy Blade" );
  talents.art_of_war                  = find_talent_spell( talent_tree::SPECIALIZATION, "Art of War" );
  talents.righteous_cause             = find_talent_spell( talent_tree::SPECIALIZATION, "Righteous Cause" );
  talents.jurisdiction                = find_talent_spell( talent_tree::SPECIALIZATION, "Jurisdiction" ); // TODO: range increase
  talents.tempest_of_the_lightbringer = find_talent_spell( talent_tree::SPECIALIZATION, "Tempest of the Lightbringer" );
  talents.rush_of_light               = find_talent_spell( talent_tree::SPECIALIZATION, "Rush of Light" );
  talents.sanctify                    = find_talent_spell( talent_tree::SPECIALIZATION, "Sanctify" );
  talents.holy_flames                 = find_talent_spell( talent_tree::SPECIALIZATION, "Holy Flames" );
  talents.improved_judgment           = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Judgment" );
  talents.boundless_judgment          = find_talent_spell( talent_tree::SPECIALIZATION, "Boundless Judgment" );
  talents.zealots_fervor              = find_talent_spell( talent_tree::SPECIALIZATION, "Zealot's Fervor" );
  talents.heart_of_the_crusader       = find_talent_spell( talent_tree::SPECIALIZATION, "Heart of the Crusader" );
  talents.blade_of_vengeance          = find_talent_spell( talent_tree::SPECIALIZATION, "Blade of Vengeance" );
  talents.empyrean_power              = find_talent_spell( talent_tree::SPECIALIZATION, "Empyrean Power" );
  talents.highlords_wrath             = find_talent_spell( talent_tree::SPECIALIZATION, "Highlord's Wrath");
  talents.templar_strikes             = find_talent_spell( talent_tree::SPECIALIZATION, "Templar Strikes" );
  talents.crusading_strikes           = find_talent_spell( talent_tree::SPECIALIZATION, "Crusading Strikes" );
  talents.blessed_champion            = find_talent_spell( talent_tree::SPECIALIZATION, "Blessed Champion" );
  talents.burning_crusade             = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Crusade" );
  talents.blades_of_light             = find_talent_spell( talent_tree::SPECIALIZATION, "Blades of Light" );
  talents.wake_of_ashes               = find_talent_spell( talent_tree::SPECIALIZATION, "Wake of Ashes" );
  talents.divine_wrath                = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Wrath" );
  talents.execution_sentence          = find_talent_spell( talent_tree::SPECIALIZATION, "Execution Sentence" );
  talents.seething_flames             = find_talent_spell( talent_tree::SPECIALIZATION, "Seething Flames" );
  talents.empyrean_legacy             = find_talent_spell( talent_tree::SPECIALIZATION, "Empyrean Legacy" );
  talents.judge_jury_and_executioner  = find_talent_spell( talent_tree::SPECIALIZATION, "Judge, Jury and Executioner" );
  talents.radiant_glory               = find_talent_spell( talent_tree::SPECIALIZATION, "Radiant Glory" );
  talents.burn_to_ash                 = find_talent_spell( talent_tree::SPECIALIZATION, "Burn to Ash" );
  talents.crusade                     = find_talent_spell( talent_tree::SPECIALIZATION, "Crusade" );

  talents.healing_hands  = find_talent_spell( talent_tree::CLASS, "Healing Hands" );

  talents.light_within_1 = find_talent_spell( talent_tree::SPECIALIZATION, 1261113 );
  talents.light_within_2 = find_talent_spell( talent_tree::SPECIALIZATION, 1261111 );
  talents.light_within_3 = find_talent_spell( talent_tree::SPECIALIZATION, 1261159 );

  talents.shield_of_vengeance         = find_talent_spell( talent_tree::CLASS, "Shield of Vengeance" );

  // Spec passives and useful spells
  spec.retribution_paladin = find_specialization_spell( "Retribution Paladin" );
  spec.retribution_paladin_2 = specialization() == PALADIN_RETRIBUTION ? find_spell( 412314 ) : spell_data_t::not_found();
  mastery.highlords_judgment = find_mastery_spell( PALADIN_RETRIBUTION );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    spec.judgment_3 = find_specialization_spell( 315867 );
    spec.judgment_4 = find_specialization_spell( 231663 );
    spec.improved_crusader_strike = find_specialization_spell( 383254 );

    spells.judgment_debuff = find_spell( 197277 );
    spells.consecrated_blade = find_specialization_spell( 404834 );
  }

  spells.crusade = find_spell( 231895 );
  spells.highlords_judgment_hidden = find_spell( 449198 );

  spells.winning_streak = find_spell( 1216828 );
  spells.all_in = find_spell( 1216837 );
}

// Action Priority List Generation
void paladin_t::generate_action_prio_list_ret()
{
  paladin_apl::retribution( this );
}

} // end namespace paladin

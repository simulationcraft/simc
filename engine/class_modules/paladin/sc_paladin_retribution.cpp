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
  struct es_inner_t : public paladin_melee_attack_t
  {
    es_inner_t( paladin_t* p ) :
      paladin_melee_attack_t( "execution_sentence_init", p, p->find_spell( 1260251 ) )
    {
      aoe = -1;
      dual = background = true;
    }

    void impact( action_state_t* s ) override
    {
      if ( result_is_hit( s->result ) )
      {
        auto tgt = td( s->target );
        if ( s->chain_target == 0 )
          tgt->debuff.execution_sentence->trigger();
        tgt->debuff.execution_sentence_gather->trigger();
      }
      paladin_melee_attack_t::impact( s );
    }

    void init() override
    {
      paladin_melee_attack_t::init();
      snapshot_flags |= STATE_TARGET_NO_PET | STATE_MUL_TA | STATE_MUL_DA;
      update_flags &= ~STATE_TARGET;
      update_flags |= STATE_MUL_TA | STATE_MUL_DA;
    }
  };

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

    impact_action = new es_inner_t( p );
    // impact_action->stats = stats;
  }

  void execute() override
  {
    p()->buffs.execution_sentence->trigger();

    paladin_melee_attack_t::execute();

    if ( p()->talents.judge_jury_and_executioner->ok() )
    {
      p()->buffs.judge_jury_and_executioner->trigger( p()->buffs.judge_jury_and_executioner->data().max_stacks() );
    }
  }
};

struct expurgation_data_t
{
  double damage_mult;
};
struct expurgation_t : public paladin_spell_t
{
  using state_t = paladin_action_state_t<expurgation_data_t>;
  expurgation_t( paladin_t* p ):
    paladin_spell_t( "expurgation", p, p->spells.expurgation )
  {
    background = true;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  double composite_rolling_ta_multiplier( const action_state_t* s ) const override
  {
    auto s_ = static_cast<const state_t*>( s );
    // Copied from base composite_rolling_ta_multiplier
    
    // Our base damage mult should be the effectiveness mult
    double m = s_->damage_mult;

    dot_t* dot = find_dot( s->target );
    if ( dot && dot->is_ticking() )
    {
      double ticks_left       = dot->ticks_left_fractional();
      timespan_t new_tick     = tick_time( s );
      timespan_t new_duration = composite_dot_duration( s );
      double new_base_ticks   = new_duration / new_tick;
      // Calculate ticks_left_fractional for the DoT after it is refreshed.
      double new_ticks_left =
          1.0 + ( calculate_dot_refresh_duration( dot, new_duration ) - dot->time_to_next_full_tick() ) / new_tick;
      // Roll the multiplier for the old ticks that will be lost into a multiplier for the new DoT.
      // New base ticks should only be applied at a fraction
      m = ( ticks_left * dot->state->rolling_ta_multiplier + new_base_ticks * s_->damage_mult ) / new_ticks_left;
      sim->print_debug(
          "{} {} rolling_ta_multiplier updated: old_multiplier={} to new_multiplier={} ticks_left={} new_base_ticks={} "
          "new_ticks_left={}.",
          *player, *this, dot->state->rolling_ta_multiplier, m, ticks_left, new_base_ticks, new_ticks_left );
    }
    return m;
  }
  void execute() override
  {
    snapshot_state( pre_execute_state, amount_type( pre_execute_state ) );
    paladin_spell_t::execute();
  }
};
void paladin_t::trigger_expurgation(player_t* target, double effectiveness = 1.0)
{
  if ( talents.expurgation->ok() )
  {
    auto e_state = static_cast<paladin_action_state_t<expurgation_data_t>*>( active.expurgation->get_state() );
    e_state->damage_mult = effectiveness;
    e_state->target      = target;
    active.expurgation->schedule_execute( e_state );
  }
}

// Blade of Justice =========================================================

struct blade_of_justice_t : public paladin_melee_attack_t
{
  struct light_within_t :public paladin_spell_t
  {
    light_within_t( paladin_t* p, std::string name ) : paladin_spell_t( "light_within_" + name, p, p->spells.light_within )
    {
      background          = true;
      aoe                 = -1;
      reduced_aoe_targets = 8;
    }
    double composite_da_multiplier(const action_state_t* s) const override
    {
      double da = paladin_spell_t::composite_da_multiplier( s );
      if (s->chain_target == 0)
      {
        da *= 1.0 + p()->talents.light_within_3->effectN( 1 ).percent();
      }
      return da;
    }
  };
  light_within_t* lw;
  blade_of_justice_t(paladin_t* p, double mul)
    : paladin_melee_attack_t( "blade_of_justice_walk_into_light", p, p->talents.blade_of_justice ),
    lw(nullptr)
  {
    background = true;
    base_multiplier *= mul;

    if ( p->talents.blade_of_vengeance->ok() )
    {
      base_aoe_multiplier *= p->find_spell( 404358 )->effectN( 1 ).ap_coeff() / attack_power_mod.direct;
      aoe                 = -1;
      reduced_aoe_targets = 5;
    }
    if ( p->talents.light_within_3->ok() )
    {
      lw = new light_within_t( p, "boj_wil" );
      add_child( lw );
    }

    triggers_higher_calling        = true;
    affected_by.highlords_judgment = true;
    cooldown->duration             = 0_ms;
  }
  blade_of_justice_t( paladin_t* p, util::string_view options_str ) : paladin_melee_attack_t( "blade_of_justice", p, p->talents.blade_of_justice ), lw(nullptr)
  {
    parse_options( options_str );

    if ( p->talents.blade_of_vengeance->ok() )
    {
      base_aoe_multiplier *= p->find_spell( 404358 )->effectN( 1 ).ap_coeff() / attack_power_mod.direct;
      aoe = -1;
      reduced_aoe_targets = 5;
    }
    if (p->talents.light_within_3->ok())
    {
      lw = new light_within_t( p, "boj" );
      add_child( lw );
    }

    triggers_higher_calling   = true;
    affected_by.highlords_judgment = true;
  }

  void execute() override
  {
    bool buff_up = p()->buffs.art_of_war->up() || p()->buffs.righteous_cause->up();
    paladin_melee_attack_t::execute();

    if ( p()->spells.consecrated_blade->ok() && p()->cooldowns.consecrated_blade_icd->up() )
    {
      p()->active.background_cons->schedule_execute();
      p()->cooldowns.consecrated_blade_icd->start();
    }
    if ( p()->talents.light_within_3->ok() && buff_up )
    {
      make_event<delayed_execute_event_t>( *sim, p(), lw, execute_state->target, 350_ms );
      p()->buffs.righteous_cause->expire();
    }
  }

  void impact( action_state_t* state ) override
  {
    paladin_melee_attack_t::impact( state );
    p()->trigger_expurgation( state->target );
  }
};

// Divine Storm =============================================================

struct divine_storm_second_sunrise_tempest_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  divine_storm_second_sunrise_tempest_t( paladin_t* p )
    : holy_power_consumer_t<paladin_melee_attack_t>( "divine_storm_second_sunrise_tempest", p, p->talents.divine_storm )
  {
    background = true;

    aoe = -1;
    base_multiplier *= p->talents.tempest_of_the_lightbringer->effectN( 1 ).percent();
    clears_judgment          = false;
    is_divine_storm          = true;
    triggers_endless_gleam   = false;
    triggers_divine_purpose  = false;
    triggers_crusade_stacks  = false;
    triggers_righteous_cause = false;
  }
  void impact(action_state_t* s) override
  {
    holy_power_consumer_t::impact( s );
    if ( p()->sets->has_set_bonus( PALADIN_RETRIBUTION, MID1, B4 ) && p()->talents.expurgation->ok() )
    {
      double mult = p()->sets->set( PALADIN_RETRIBUTION, MID1, B4 )->effectN( 2 ).percent() * base_multiplier;
      p()->trigger_expurgation( execute_state->target, mult );
    }
  }
};

struct divine_storm_tempest_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  divine_storm_tempest_t( paladin_t* p )
    : holy_power_consumer_t<paladin_melee_attack_t>( "divine_storm_tempest", p, p->find_spell( 224239 ) )
  {
    background = true;

    aoe = -1;
    base_multiplier *= p->talents.tempest_of_the_lightbringer->effectN( 1 ).percent();
    clears_judgment          = false;
    is_divine_storm          = true;
    triggers_endless_gleam   = false;
    triggers_divine_purpose  = true;
    triggers_crusade_stacks  = false;
    triggers_righteous_cause = false;
  }
  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );
    if ( p()->sets->has_set_bonus( PALADIN_RETRIBUTION, MID1, B4 ) && p()->talents.expurgation->ok() )
    {
      double mult = p()->sets->set( PALADIN_RETRIBUTION, MID1, B4 )->effectN( 2 ).percent() * base_multiplier;
      p()->trigger_expurgation( execute_state->target, mult );
    }
  }
};

struct divine_storm_second_sunrise_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  divine_storm_second_sunrise_tempest_t* tempest;
  divine_storm_second_sunrise_t( paladin_t* p, double multiplier )
    : holy_power_consumer_t<paladin_melee_attack_t>( "divine_storm_second_sunrise", p, p->talents.divine_storm ),
      tempest( nullptr )
  {
    background = true;

    aoe = -1;
    base_multiplier *= multiplier;
    clears_judgment                   = false;
    base_costs[ RESOURCE_HOLY_POWER ] = 0;

    if ( p->talents.tempest_of_the_lightbringer->ok() )
    {
      tempest = new divine_storm_second_sunrise_tempest_t( p );
      add_child( tempest );
    }
    is_divine_storm          = true;
    triggers_endless_gleam   = true;
    triggers_divine_purpose  = false;
    triggers_crusade_stacks  = false;
    triggers_righteous_cause = false;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p()->talents.tempest_of_the_lightbringer->ok() )
      tempest->schedule_execute();
  }
  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );
    if ( p()->sets->has_set_bonus( PALADIN_RETRIBUTION, MID1, B4 ) && p()->talents.expurgation->ok() )
    {
      double mult = p()->sets->set( PALADIN_RETRIBUTION, MID1, B4 )->effectN( 2 ).percent() * base_multiplier;
      p()->trigger_expurgation( execute_state->target, mult );
    }
  }
};

struct divine_storm_t: public holy_power_consumer_t<paladin_melee_attack_t>
{
  divine_storm_tempest_t* tempest;
  divine_storm_second_sunrise_t* sunrise_echo;

  divine_storm_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "divine_storm", p, p->talents.divine_storm ),
    tempest( nullptr ), sunrise_echo( nullptr )
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
      sunrise_echo = new divine_storm_second_sunrise_t( p, p->talents.herald_of_the_sun.second_sunrise->effectN( 2 ).percent() );
      add_child( sunrise_echo );
    }
    triggers_endless_gleam   = true;
    triggers_divine_purpose  = true;
    triggers_crusade_stacks  = true;
    triggers_righteous_cause = true;
  }

  // Constructor for Empyrean Legacy
  divine_storm_t( paladin_t* p, bool is_free, double mul ) :
    holy_power_consumer_t( "divine_storm", p, p->talents.divine_storm ),
    tempest( nullptr ), sunrise_echo( nullptr )
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
      sunrise_echo = new divine_storm_second_sunrise_t( p, p->talents.herald_of_the_sun.second_sunrise->effectN( 2 ).percent() );
      add_child( sunrise_echo );
    }
    triggers_endless_gleam   = true;
    triggers_divine_purpose  = true;
    triggers_crusade_stacks  = false;
    triggers_righteous_cause = false;
  }

  void execute() override
  {
    holy_power_consumer_t::execute();

    if ( p()->talents.tempest_of_the_lightbringer->ok() )
      tempest->schedule_execute();

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
    if ( p()->sets->has_set_bonus( PALADIN_RETRIBUTION, MID1, B4 ) && p()->talents.expurgation->ok() )
    {
      double mult = p()->sets->set(PALADIN_RETRIBUTION, MID1, B4)->effectN(2).percent() * base_multiplier;
      p()->trigger_expurgation( execute_state->target, mult );
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

  templars_verdict_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t(
      ( p->talents.final_verdict->ok() ) ? "final_verdict" : "templars_verdict",
      p,
      ( p->talents.final_verdict->ok() ) ? ( p->find_spell( 383328 ) ) : ( p->find_specialization_spell( "Templar's Verdict" ) ) ),
    is_fv( p->talents.final_verdict->ok() )
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
    triggers_endless_gleam = true;
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
        // Safeguard if one of those spells is not used in the APL, so it doesn't crash
        if ( p()->cooldowns.judgment != nullptr )
          p()->cooldowns.judgment->reset( true );
        if ( p()->cooldowns.hammer_of_wrath != nullptr )
          p()->cooldowns.hammer_of_wrath->reset( true );
      }
    }
  }
  void impact(action_state_t* s) override
  {
    holy_power_consumer_t::impact(s);
    if ( p()->sets->has_set_bonus( PALADIN_RETRIBUTION, MID1, B4 ) && p()->talents.expurgation->ok() )
    {
      double mult = p()->sets->set( PALADIN_RETRIBUTION, MID1, B4 )->effectN( 1 ).percent() * base_multiplier;
      p()->trigger_expurgation( execute_state->target, mult );
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
    if ( p()->talents.radiant_glory->ok() && p()->talents.avenging_wrath->ok() )
    {
      p()->active.background_avenging_wrath->execute_on_target( p() );
    }

    paladin_spell_t::execute();

    if ( p()->talents.seething_flames->ok() )
    {
      for ( int i = 0; i < as<int>( p()->talents.seething_flames->effectN( 1 ).base_value() ); i++ )
      {
        make_event<seething_flames_event_t>( *sim, p(), execute_state->target, seething_flames[i], timespan_t::from_millis( 500 * (i + 1) ) );
      }
    }
    if ( p()->templar() )
    {
      p()->buffs.templar.hammer_of_light_ready->trigger();
    }

    if ( p()->talents.templar.sacrosanct_crusade->ok() )
    {
      p()->buffs.templar.sacrosanct_crusade->trigger();
    }

    if ( p()->herald_of_the_sun() )
    {
      p()->buffs.herald_of_the_sun.dawnlight->trigger(
          as<int>( p()->talents.herald_of_the_sun.dawnlight->effectN( 1 ).base_value() ) );
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
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }

    triggers_higher_calling = true;
  }
  void execute() override
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit( execute_state->result ) && p()->talents.empyrean_power->ok() )
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

struct templar_slash_t : public base_templar_strike_t
{
  templar_slash_t( paladin_t* p, util::string_view options_str )
    : base_templar_strike_t( "templar_slash", p, options_str, p->find_spell( 406647 ) )
  {

  }

  void execute() override
  {
    base_templar_strike_t::execute();
    p()->buffs.templar_strikes->expire();
  }

  bool ready() override
  {
    bool orig = paladin_melee_attack_t::ready();
    return orig && p()->buffs.templar_strikes->up();
  }
  double composite_crit_chance() const override
  {
    return 1.0;
  }
};

struct highlords_judgment_t : public paladin_spell_t
{
  highlords_judgment_t( paladin_t* p ) : paladin_spell_t( "highlords_judgment", p, p->find_spell( 383921 ) )
  {
    background = true;
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

void paladin_t::accumulate_es_damage( action_state_t* s, double mult )
{
  if ( !buffs.execution_sentence->up() )
    return;

  double es_accum = buffs.execution_sentence->check_value();

  double amount_accumulated = s->result_total * mult;

  sim->print_debug( "{}'s Execution Sentence accumulates {} additional damage: {} -> {}", name_str,
    amount_accumulated, es_accum, es_accum + amount_accumulated );

  es_accum += amount_accumulated;

  buffs.execution_sentence->current_value = es_accum;
}

void paladin_t::trigger_es_explosion( player_t* target )
{
  double ta = 0.0;
  double accumulated = buffs.execution_sentence->check_value() * talents.execution_sentence->effectN( 2 ).percent();

  sim->print_debug( "{}'s execution_sentence has accumulated {} total additional damage.", target->name(), accumulated );
  ta += accumulated;

  es_explosion_t* explosion = static_cast<es_explosion_t*>( active.es_explosion );
  explosion->set_target( target );
  explosion->accumulated = ta;
  explosion->schedule_execute();

  buffs.execution_sentence->expire();
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
    active.highlords_judgment = new highlords_judgment_t( this );
    if ( talents.herald_of_the_sun.sun_sear->ok() )
    {
      active.sun_sear = new sun_sear_t( this );
    }
  }
  if ( talents.herald_of_the_sun.walk_into_light->ok() )
  {
    active.blade_of_justice =
        new blade_of_justice_t( this, talents.herald_of_the_sun.walk_into_light->effectN( 3 ).percent() );
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

  buffs.rise_from_ash = make_buff( this, "rise_from_ash", find_spell( 454693 ) );
  buffs.winning_streak = make_buff( this, "winning_streak", find_spell( 1216828 ) )
    ->set_default_value_from_effect( 1 );
  buffs.all_in = make_buff( this, "all_in", find_spell( 1216837 ) )
    ->set_default_value_from_effect( 1 );

  buffs.art_of_war = make_buff( this, "art_of_war", find_spell( 406086 ) );
  buffs.righteous_cause = make_buff( this, "righteous_cause", find_spell( 402916 ) )->set_chance( 1.0 );

  buffs.execution_sentence = make_buff( this, "execution_sentence", find_spell( 1234189 ) )
    ->set_default_value( 0.0 )
    ->set_duration( 10025_ms ); // make it slightly longer than the debuff to avoid races with expiry
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
  spells.light_within   = find_spell( 1261160 );
  spells.expurgation               = find_spell( 383346 );
  spells.judgment_ret              = find_spell( 20271 );
  spells.judgment_ret_dt           = find_spell( 406957 );
  spells.hammer_of_wrath_ret       = find_spell( 24275 );
  spells.hammer_of_wrath_ret_dt    = find_spell( 1279408 );
  spells.crusading_strikes_data    = find_spell( 406834 );
}

// Action Priority List Generation
void paladin_t::generate_action_prio_list_ret()
{
  paladin_apl::retribution( this );
}

} // end namespace paladin

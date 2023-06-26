#include <unordered_set>

#include "simulationcraft.hpp"
#include "sc_paladin.hpp"
#include "class_modules/apl/apl_paladin.hpp"

//
namespace paladin {

namespace buffs {
  crusade_buff_t::crusade_buff_t( paladin_t* p ) :
      buff_t( p, "crusade", p->spells.crusade ),
      damage_modifier( 0.0 ),
      haste_bonus( 0.0 )
  {
    if ( !p->talents.crusade->ok() )
    {
      set_chance( 0 );
    }
    set_refresh_behavior( buff_refresh_behavior::DISABLED );
    // TODO(mserrano): fix this when Blizzard turns the spelldata back to sane
    //  values
    if ( p->talents.avenging_wrath->ok() )
      damage_modifier = data().effectN( 1 ).percent() / 10.0;
    haste_bonus = data().effectN( 3 ).percent() / 10.0;

    auto* paladin = static_cast<paladin_t*>( p );
    if ( paladin->talents.divine_wrath->ok() )
    {
      base_buff_duration += paladin->talents.divine_wrath->effectN( 1 ).time_value();
    }


    // let the ability handle the cooldown
    cooldown->duration = 0_ms;

    add_invalidate( CACHE_HASTE );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_MASTERY );
  }

  struct shield_of_vengeance_buff_t : public absorb_buff_t
  {
    shield_of_vengeance_buff_t( player_t* p ):
      absorb_buff_t( p, "shield_of_vengeance", p->find_talent_spell( talent_tree::SPECIALIZATION, "Shield of Vengeance" ) )
    {
      cooldown->duration = 0_ms;
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

      auto* p = static_cast<paladin_t*>( player );
      // do thing
      if ( p->options.fake_sov )
      {
        // TODO(mserrano): This is a horrible hack
        p->active.shield_of_vengeance_damage->base_dd_max = p->active.shield_of_vengeance_damage->base_dd_min = current_value;
        p->active.shield_of_vengeance_damage->execute();
      }
    }
  };
}

// Crusade
struct crusade_t : public paladin_spell_t
{
  crusade_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "crusade", p, p->spells.crusade )
  {
    parse_options( options_str );

    if ( ! ( p->talents.crusade->ok() ) )
      background = true;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    // If Visions already procced the buff and this spell is used, all stacks are reset to 1
    // The duration is also set to its default value, there's no extending or pandemic
    if ( p()->buffs.crusade->up() )
      p()->buffs.crusade->expire();

    p()->buffs.crusade->trigger();
  }
};

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

    // this is the only difference from normal direct_amount!
    amount += accumulated;

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

    // Spelldata doesn't seem to have this
    hasted_gcd = true;

    // ... this appears to be true for the base damage only,
    // and is not automatically obtained from spell data.
    affected_by.hand_of_light = true;

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

    if ( p()->talents.divine_auxiliary->ok() )
    {
      p()->resource_gain( RESOURCE_HOLY_POWER, p()->talents.divine_auxiliary->effectN( 1 ).base_value(),
                            p()->gains.hp_divine_auxiliary );
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

// Blade of Justice =========================================================

struct blade_of_justice_t : public paladin_melee_attack_t
{
  struct expurgation_t : public paladin_spell_t
  {
    expurgation_t( paladin_t* p ):
      paladin_spell_t( "expurgation", p, p->find_spell( 383346 ) )
    {
      searing_light_disabled = true;

      if ( p->talents.jurisdiction->ok() )
      {
        base_multiplier *= 1.0 + p->talents.jurisdiction->effectN( 4 ).percent();
      }
    }
  };

  expurgation_t* expurgation;

  blade_of_justice_t( paladin_t* p, util::string_view options_str ) :
    paladin_melee_attack_t( "blade_of_justice", p, p->talents.blade_of_justice ),
    expurgation( nullptr )
  {
    parse_options( options_str );

    if ( p->talents.expurgation->ok() )
    {
      expurgation = new expurgation_t( p );
      add_child( expurgation );
    }

    if ( p->talents.holy_blade->ok() )
    {
      energize_amount += p->talents.holy_blade->effectN( 1 ).resource( RESOURCE_HOLY_POWER );
    }

    if ( p->talents.light_of_justice->ok() )
    {
      cooldown->duration += timespan_t::from_millis( p->talents.light_of_justice->effectN( 1 ).base_value() );
    }

    if ( p->talents.improved_blade_of_justice->ok() )
    {
      cooldown->charges += as<int>( p->talents.improved_blade_of_justice->effectN( 1 ).base_value() );
    }

    if ( p->talents.jurisdiction->ok() )
    {
      base_multiplier *= 1.0 + p->talents.jurisdiction->effectN( 4 ).percent();
    }

    if ( p->talents.blade_of_vengeance->ok() )
    {
      attack_power_mod.direct = p->find_spell( 404358 )->effectN( 1 ).ap_coeff();
      aoe = -1;
      reduced_aoe_targets = 5;
    }

    affected_by.hand_of_light = true;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p()->talents.consecrated_blade->ok() && p()->cooldowns.consecrated_blade_icd->up() )
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
      expurgation->set_target( state->target );
      expurgation->execute();
    }
  }
};

// Divine Storm =============================================================

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

struct divine_storm_t: public holy_power_consumer_t<paladin_melee_attack_t>
{
  divine_storm_tempest_t* tempest;

  divine_storm_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "divine_storm", p, p->talents.divine_storm ),
    tempest( nullptr )
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
  }

  divine_storm_t( paladin_t* p, bool is_free, double mul ) :
    holy_power_consumer_t( "divine_storm", p, p->talents.divine_storm ),
    tempest( nullptr )
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
  }

  double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();

    if ( p()->buffs.empyrean_power->up() )
      am *= 1.0 + p()->buffs.empyrean_power->data().effectN( 1 ).percent();

    if ( p()->buffs.inquisitors_ire->up() )
      am *= 1.0 + p()->buffs.inquisitors_ire->check_stack_value();

    return am;
  }

  void execute() override
  {
    holy_power_consumer_t::execute();

    if ( p()->talents.tempest_of_the_lightbringer->ok() )
      tempest->schedule_execute();

    if ( p()->buffs.inquisitors_ire->up() )
      p()->buffs.inquisitors_ire->expire();
  }

  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p()->talents.sanctify->ok() )
        td( s->target )->debuff.sanctify->trigger();
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

    if ( p->talents.jurisdiction->ok() )
    {
      base_multiplier *= 1.0 + p->talents.jurisdiction->effectN( 4 ).percent();
    }

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

    // TODO(mserrano): figure out the actionbar override thing instead of this hack.
    if ( is_fv )
    {
      double proc_chance = data().effectN( 2 ).percent();
      if ( rng().roll( proc_chance ) )
      {
        p()->cooldowns.hammer_of_wrath->reset( true );
        p()->buffs.final_verdict->trigger();
      }
    }

    if ( !background )
    {
      if ( p()->talents.righteous_cause->ok() )
      {
        if ( rng().roll( p()->talents.righteous_cause->proc_chance() ) )
        {
          p()->procs.righteous_cause->occur();
          p()->cooldowns.blade_of_justice->reset( true );
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );

    if ( p()->buffs.divine_arbiter->stack() == as<int>( p()->buffs.divine_arbiter->data().effectN( 2 ).base_value() ) )
    {
      p()->active.divine_arbiter->set_target( s->target );
      p()->active.divine_arbiter->schedule_execute();
      p()->buffs.divine_arbiter->expire();
    }
  }
};

// Final Reckoning

struct final_reckoning_t : public paladin_spell_t
{
  final_reckoning_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "final_reckoning", p, p->talents.final_reckoning )
  {
    parse_options( options_str );

    aoe = -1;

    if ( ! ( p->talents.final_reckoning->ok() ) )
      background = true;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    if ( p()->talents.divine_auxiliary->ok() )
    {
      p()->resource_gain( RESOURCE_HOLY_POWER, p()->talents.divine_auxiliary->effectN( 1 ).base_value(),
                            p()->gains.hp_divine_auxiliary );
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuff.final_reckoning->trigger();
    }
  }
};

// Judgment - Retribution =================================================================

struct judgment_ret_t : public judgment_t
{
  int holy_power_generation;

  judgment_ret_t( paladin_t* p, util::string_view name, util::string_view options_str ) :
    judgment_t( p, name ),
    holy_power_generation( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    if ( p->talents.highlords_judgment->ok() )
      base_multiplier *= 1.0 + p->talents.highlords_judgment->effectN( 1 ).percent();

    if ( p->talents.boundless_judgment->ok() )
    {
      holy_power_generation += as<int>( p->talents.boundless_judgment->effectN( 1 ).base_value() );
    }

    if ( p->talents.blessed_champion->ok() )
    {
      aoe = as<int>( 1 + p->talents.blessed_champion->effectN( 4 ).base_value() );
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }

    if ( p->talents.judge_jury_and_executioner->ok() )
    {
      base_crit += p->talents.judge_jury_and_executioner->effectN( 1 ).percent();
    }

    if ( p->sets->has_set_bonus( PALADIN_RETRIBUTION, T30, B2 ) )
    {
      crit_bonus_multiplier *= 1.0 + p->sets->set( PALADIN_RETRIBUTION, T30, B2 )->effectN( 2 ).percent();
      base_multiplier *= 1.0 + p->sets->set( PALADIN_RETRIBUTION, T30, B2 )->effectN( 1 ).percent();
    }
  }

  judgment_ret_t( paladin_t* p, util::string_view name, bool is_divine_toll ) :
    judgment_t( p, name ),
    holy_power_generation( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) )
  {
    // This is for Divine Toll's background judgments
    background = true;
    cooldown = p->get_cooldown( "dummy_cd" );

    if ( p->talents.highlords_judgment->ok() )
      base_multiplier *= 1.0 + p->talents.highlords_judgment->effectN( 1 ).percent();

    // according to skeletor this is given the bonus of 326011
    if ( is_divine_toll )
      base_multiplier *= 1.0 + p->find_spell( 326011 )->effectN( 1 ).percent();

    if ( p->talents.boundless_judgment->ok() )
    {
      holy_power_generation += as<int>( p->talents.boundless_judgment->effectN( 1 ).base_value() );
    }

    // we don't do the blessed champion stuff here; DT judgments do not seem to cleave

    if ( p->talents.judge_jury_and_executioner->ok() )
    {
      base_crit += p->talents.judge_jury_and_executioner->effectN( 1 ).percent();
    }
  }

  void execute() override
  {
    judgment_t::execute();

    if ( p()->spec.judgment_3->ok() )
      p()->resource_gain( RESOURCE_HOLY_POWER, holy_power_generation, p()->gains.judgment );

    if ( p()->talents.empyrean_legacy->ok() )
    {
      if ( ! ( p()->buffs.empyrean_legacy_cooldown->up() ) )
      {
        p()->buffs.empyrean_legacy->trigger();
        p()->buffs.empyrean_legacy_cooldown->trigger();
      }
    }
  }
};

// Justicar's Vengeance
struct justicars_vengeance_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  justicars_vengeance_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "justicars_vengeance", p, p->talents.justicars_vengeance )
  {
    parse_options( options_str );

    // Spelldata doesn't have this
    hasted_gcd = true;

    weapon_multiplier = 0; // why is this needed?

    // Healing isn't implemented

    if ( p->talents.jurisdiction->ok() )
    {
      base_multiplier *= 1.0 + p->talents.jurisdiction->effectN( 4 ).percent();
    }
  }


  void execute() override
  {
    holy_power_consumer_t::execute();

    if ( p()->talents.righteous_cause->ok() )
    {
      if ( rng().roll( p()->talents.righteous_cause->proc_chance() ) )
      {
        p()->procs.righteous_cause->occur();
        p()->cooldowns.blade_of_justice->reset( true );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );

    if ( p()->buffs.divine_arbiter->stack() == as<int>( p()->buffs.divine_arbiter->data().effectN( 2 ).base_value() ) )
    {
      p()->active.divine_arbiter->set_target( s->target );
      p()->active.divine_arbiter->schedule_execute();
      p()->buffs.divine_arbiter->expire();
    }
  }
};

// SoV

struct shield_of_vengeance_proc_t : public paladin_spell_t
{
  shield_of_vengeance_proc_t( paladin_t* p ) :
    paladin_spell_t( "shield_of_vengeance_proc", p, p->find_spell( 184689 ) )
  {
    may_miss = may_dodge = may_parry = may_glance = false;
    background = true;
    split_aoe_damage = true;
  }

  void init() override {
    paladin_spell_t::init();
    snapshot_flags = 0;
  }

  proc_types proc_type() const override
  {
    return PROC1_MELEE_ABILITY;
  }
};

struct shield_of_vengeance_t : public paladin_absorb_t
{
  shield_of_vengeance_t( paladin_t* p, util::string_view options_str ) :
    paladin_absorb_t( "shield_of_vengeance", p, p->talents.shield_of_vengeance )
  {
    parse_options( options_str );

    harmful = false;

    // unbreakable spirit reduces cooldown
    if ( p->talents.unbreakable_spirit->ok() )
      cooldown->duration = data().cooldown() * ( 1 + p->talents.unbreakable_spirit->effectN( 1 ).percent() );
  }

  void init() override
  {
    paladin_absorb_t::init();
    snapshot_flags |= (STATE_CRIT | STATE_VERSATILITY);
  }

  void execute() override
  {
    double shield_amount = p()->resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent();

    if ( p()->talents.aegis_of_protection->ok() )
      shield_amount *= 1.0 + p()->talents.aegis_of_protection->effectN( 2 ).percent();

    shield_amount *= 1.0 + p()->composite_heal_versatility();

    paladin_absorb_t::execute();
    p()->buffs.shield_of_vengeance->trigger( 1, shield_amount );
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

    if ( p->talents.truths_wake->ok() )
    {
      truths_wake = new truths_wake_t( p );
      add_child( truths_wake );
    }

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
      if ( p()->talents.truths_wake->ok() )
      {
        truths_wake->set_target( s->target );
        truths_wake->execute();
      }
    }
  }

  void execute() override
  {
    paladin_spell_t::execute();

    if ( p()->talents.seething_flames->ok() )
    {
      for ( int i = 0; i < as<int>( p()->talents.seething_flames->effectN( 1 ).base_value() ); i++ )
      {
        make_event<seething_flames_event_t>( *sim, p(), execute_state->target, seething_flames[i], timespan_t::from_millis( 500 * (i + 1) ) );
      }
    }
  }
};

struct divine_hammer_tick_t : public paladin_melee_attack_t
{
  divine_hammer_tick_t( paladin_t* p )
    : paladin_melee_attack_t( "divine_hammer_tick", p, p->find_spell( 198137 ) )
  {
    aoe         = -1;
    reduced_aoe_targets = 8; // does not appear to have a spelldata equivalent
    dual        = true;
    direct_tick = true;
    background  = true;
    may_crit    = true;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( target );

    paladin_td_t* td = p()->get_target_data( target );
    if ( td->debuff.sanctify->up() )
      m *= 1.0 + td->debuff.sanctify->data().effectN( 1 ).percent();

    return m;
  }
};

struct divine_hammer_t : public paladin_spell_t
{
  divine_hammer_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "divine_hammer", p, p->talents.divine_hammer )
  {
    parse_options( options_str );

    if ( !p->talents.divine_hammer->ok() )
      background = true;

    may_miss       = false;
    tick_may_crit  = true;
    // TODO: verify
    tick_zero      = true;

    tick_action = new divine_hammer_tick_t( p );
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

    if ( p->talents.heart_of_the_crusader->ok() )
    {
      crit_bonus_multiplier *= 1.0 + p->talents.heart_of_the_crusader->effectN( 4 ).percent();
      base_multiplier *= 1.0 + p->talents.heart_of_the_crusader->effectN( 3 ).percent();
    }
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p()->talents.empyrean_power->ok() )
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
    if ( p->talents.templar_strikes->ok() )
    {
      cooldown->duration += timespan_t::from_millis( p->talents.templar_strikes->effectN( 2 ).base_value() );
    }

    if ( p->talents.swift_justice->ok() )
    {
      cooldown->duration += timespan_t::from_millis( p->talents.swift_justice->effectN( 2 ).base_value() );
    }

    if ( p->spec.improved_crusader_strike )
    {
      cooldown->charges += as<int>( p->spec.improved_crusader_strike->effectN( 1 ).base_value() );
    }
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
    base_crit = 1.0;
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
};

struct divine_arbiter_t : public paladin_spell_t
{
  divine_arbiter_t( paladin_t* p )
    : paladin_spell_t( "divine_arbiter", p, p->find_spell( 406983 ) )
  {
    background = true;

    // force effect 1 to be used for the direct ratios
    parse_effect_data( data().effectN( 1 ) );

    // but compute the aoe multiplier from the 2nd effect
    base_aoe_multiplier *= data().effectN( 2 ).ap_coeff() / data().effectN( 1 ).ap_coeff();

    // and do aoe, too
    aoe = -1;
  }
};

// TODO: drop the Consecrate
struct searing_light_t : public paladin_spell_t
{
  searing_light_t( paladin_t* p )
    : paladin_spell_t( "searing_light", p, p->find_spell( 407478 ) )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = 8;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p()->active.searing_light_cons->set_target( execute_state->target );
    p()->active.searing_light_cons->execute();
  }
};

void paladin_t::trigger_es_explosion( player_t* target )
{
  double ta = 0.0;
  double accumulated = get_target_data( target )->debuff.execution_sentence->get_accumulated_damage();
  if ( talents.penitence->ok() )
    accumulated *= 1.0 + talents.penitence->effectN( 1 ).percent();
  if ( bugs )
  {
    accumulated /= 1.0 + composite_damage_versatility();
  }

  sim->print_debug( "{}'s execution_sentence has accumulated {} total additional damage.", target->name(), accumulated );
  ta += accumulated;

  es_explosion_t* explosion = static_cast<es_explosion_t*>( active.es_explosion );
  explosion->set_target( target );
  explosion->accumulated = ta;
  explosion->schedule_execute();
}

// Initialization

void paladin_t::create_ret_actions()
{
  active.shield_of_vengeance_damage = new shield_of_vengeance_proc_t( this );
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

  if ( talents.divine_arbiter->ok() )
  {
    active.divine_arbiter = new divine_arbiter_t( this );
  }

  if ( talents.searing_light->ok() )
  {
    active.searing_light = new searing_light_t( this );
  }

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    active.divine_toll = new judgment_ret_t( this, "divine_toll_judgment", true );
    active.divine_resonance = new judgment_ret_t( this, "divine_resonance_judgment", false );
  }
}

action_t* paladin_t::create_action_retribution( util::string_view name, util::string_view options_str )
{
  if ( name == "blade_of_justice"          ) return new blade_of_justice_t         ( this, options_str );
  if ( name == "crusade"                   ) return new crusade_t                  ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "wake_of_ashes"             ) return new wake_of_ashes_t            ( this, options_str );
  if ( name == "justicars_vengeance"       ) return new justicars_vengeance_t      ( this, options_str );
  if ( name == "shield_of_vengeance"       ) return new shield_of_vengeance_t      ( this, options_str );
  if ( name == "final_reckoning"           ) return new final_reckoning_t          ( this, options_str );
  if ( name == "templar_strike"            ) return new templar_strike_t           ( this, options_str );
  if ( name == "templar_slash"             ) return new templar_slash_t            ( this, options_str );
  if ( name == "divine_hammer"             ) return new divine_hammer_t            ( this, options_str );
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    if ( name == "judgment" ) return new judgment_ret_t( this, "judgment", options_str );
  }

  return nullptr;
}

void paladin_t::create_buffs_retribution()
{
  buffs.crusade = new buffs::crusade_buff_t( this );

  buffs.shield_of_vengeance = new buffs::shield_of_vengeance_buff_t( this );

  buffs.rush_of_light = make_buff( this, "rush_of_light", find_spell( 407065 ) )
    ->add_invalidate( CACHE_HASTE )
    ->set_default_value( talents.rush_of_light->effectN( 1 ).percent() );

  buffs.inquisitors_ire = make_buff( this, "inquisitors_ire", find_spell( 403976 ) )
                            ->set_default_value( find_spell( 403976 )->effectN( 1 ).percent() );
  buffs.inquisitors_ire_driver = make_buff( this, "inquisitors_ire_driver", find_spell( 403975 ) )
                                  ->set_period( find_spell( 403975 )->effectN( 1 ).period() )
                                  ->set_quiet( true )
                                  ->set_tick_callback([this](buff_t*, int, const timespan_t&) { buffs.inquisitors_ire->trigger(); })
                                  ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED );

  buffs.templar_strikes = make_buff( this, "templar_strikes", find_spell( 406648 ) );
  buffs.divine_arbiter = make_buff( this, "divine_arbiter", find_spell( 406975 ) )
                          ->set_max_stack( as<int>( find_spell( 406975 )->effectN( 2 ).base_value() ) );
  buffs.empyrean_power = make_buff( this, "empyrean_power", find_spell( 326733 ) )
                          ->set_trigger_spell(talents.empyrean_power);

  // legendaries
  buffs.empyrean_legacy = make_buff( this, "empyrean_legacy", find_spell( 387178 ) );
  buffs.empyrean_legacy_cooldown = make_buff( this, "empyrean_legacy_cooldown", find_spell( 387441 ) );
}

void paladin_t::init_rng_retribution()
{
}

void paladin_t::init_spells_retribution()
{
  // Talents
  talents.blade_of_justice            = find_talent_spell( talent_tree::SPECIALIZATION, "Blade of Justice" );
  talents.divine_storm                = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Storm" );
  talents.art_of_war                  = find_talent_spell( talent_tree::SPECIALIZATION, "Art of War" );
  talents.holy_blade                  = find_talent_spell( talent_tree::SPECIALIZATION, "Holy Blade" );
  talents.shield_of_vengeance         = find_talent_spell( talent_tree::SPECIALIZATION, "Shield of Vengeance" );
  talents.highlords_judgment          = find_talent_spell( talent_tree::SPECIALIZATION, "Highlord's Judgment" );
  talents.sanctify                    = find_talent_spell( talent_tree::SPECIALIZATION, "Sanctify" );
  talents.wake_of_ashes               = find_talent_spell( talent_tree::SPECIALIZATION, "Wake of Ashes" );
  talents.consecrated_blade           = find_talent_spell( talent_tree::SPECIALIZATION, "Consecrated Blade" );
  talents.expurgation                 = find_talent_spell( talent_tree::SPECIALIZATION, "Expurgation" );
  talents.boundless_judgment          = find_talent_spell( talent_tree::SPECIALIZATION, "Boundless Judgment" );
  talents.crusade                     = find_talent_spell( talent_tree::SPECIALIZATION, "Crusade" );
  talents.truths_wake                 = find_talent_spell( talent_tree::SPECIALIZATION, "Truth's Wake" );
  talents.empyrean_power              = find_talent_spell( talent_tree::SPECIALIZATION, "Empyrean Power" );
  talents.consecrated_ground_ret      = find_talent_spell( talent_tree::SPECIALIZATION, "Consecrated Ground", PALADIN_RETRIBUTION );
  talents.tempest_of_the_lightbringer = find_talent_spell( talent_tree::SPECIALIZATION, "Tempest of the Lightbringer" );
  talents.justicars_vengeance         = find_talent_spell( talent_tree::SPECIALIZATION, "Justicar's Vengeance" );
  talents.execution_sentence          = find_talent_spell( talent_tree::SPECIALIZATION, "Execution Sentence" );
  talents.empyrean_legacy             = find_talent_spell( talent_tree::SPECIALIZATION, "Empyrean Legacy" );
  talents.final_verdict               = find_talent_spell( talent_tree::SPECIALIZATION, "Final Verdict" );
  talents.executioners_will           = find_talent_spell( talent_tree::SPECIALIZATION, "Executioner's Will" );
  talents.final_reckoning             = find_talent_spell( talent_tree::SPECIALIZATION, "Final Reckoning" );
  talents.vanguards_momentum          = find_talent_spell( talent_tree::SPECIALIZATION, "Vanguard's Momentum" );
  talents.divine_wrath                = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Wrath" );

  talents.swift_justice               = find_talent_spell( talent_tree::SPECIALIZATION, "Swift Justice" );
  talents.light_of_justice            = find_talent_spell( talent_tree::SPECIALIZATION, "Light of Justice" );
  talents.judgment_of_justice         = find_talent_spell( talent_tree::SPECIALIZATION, "Judgment of Justice");
  talents.improved_blade_of_justice   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Blade of Justice" );
  talents.righteous_cause             = find_talent_spell( talent_tree::SPECIALIZATION, "Righteous Cause" );
  talents.jurisdiction                = find_talent_spell( talent_tree::SPECIALIZATION, "Jurisdiction" ); // TODO: range increase
  talents.inquisitors_ire             = find_talent_spell( talent_tree::SPECIALIZATION, "Inquisitor's Ire" );
  talents.zealots_fervor              = find_talent_spell( talent_tree::SPECIALIZATION, "Zealot's Fervor" );
  talents.rush_of_light               = find_talent_spell( talent_tree::SPECIALIZATION, "Rush of Light" );
  talents.improved_judgment           = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Judgment" );
  talents.blessed_champion            = find_talent_spell( talent_tree::SPECIALIZATION, "Blessed Champion" );
  talents.judge_jury_and_executioner  = find_talent_spell( talent_tree::SPECIALIZATION, "Judge, Jury and Executioner" );
  talents.penitence                   = find_talent_spell( talent_tree::SPECIALIZATION, "Penitence" );
  talents.adjudication                = find_talent_spell( talent_tree::SPECIALIZATION, "Adjudication" );
  talents.heart_of_the_crusader       = find_talent_spell( talent_tree::SPECIALIZATION, "Heart of the Crusader" );
  talents.divine_hammer               = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Hammer" );
  talents.blade_of_vengeance          = find_talent_spell( talent_tree::SPECIALIZATION, "Blade of Vengeance" );
  talents.vanguard_of_justice         = find_talent_spell( talent_tree::SPECIALIZATION, "Vanguard of Justice" );
  talents.highlords_judgment          = find_talent_spell( talent_tree::SPECIALIZATION, "Highlord's Judgment" );
  talents.aegis_of_protection         = find_talent_spell( talent_tree::SPECIALIZATION, "Aegis of Protection" );
  talents.burning_crusade             = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Crusade" );
  talents.blades_of_light             = find_talent_spell( talent_tree::SPECIALIZATION, "Blades of Light" );
  talents.crusading_strikes           = find_talent_spell( talent_tree::SPECIALIZATION, "Crusading Strikes" );
  talents.templar_strikes             = find_talent_spell( talent_tree::SPECIALIZATION, "Templar Strikes" );
  talents.divine_arbiter              = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Arbiter" );
  talents.searing_light               = find_talent_spell( talent_tree::SPECIALIZATION, "Searing Light" );
  talents.divine_auxiliary            = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Auxiliary" );
  talents.seething_flames             = find_talent_spell( talent_tree::SPECIALIZATION, "Seething Flames" );

  talents.vengeful_wrath = find_talent_spell( talent_tree::CLASS, "Vengeful Wrath" );
  // Spec passives and useful spells
  spec.retribution_paladin = find_specialization_spell( "Retribution Paladin" );
  mastery.hand_of_light = find_mastery_spell( PALADIN_RETRIBUTION );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    spec.judgment_3 = find_specialization_spell( 315867 );
    spec.judgment_4 = find_specialization_spell( 231663 );
    spec.improved_crusader_strike = find_specialization_spell( 383254 );

    spells.judgment_debuff = find_spell( 197277 );
  }

  passives.boundless_conviction = find_spell( 115675 );

  spells.crusade = find_spell( 231895 );
}

// Action Priority List Generation
void paladin_t::generate_action_prio_list_ret()
{
  paladin_apl::retribution( this );
}

} // end namespace paladin

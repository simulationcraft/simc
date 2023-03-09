#include <unordered_set>

#include "simulationcraft.hpp"
#include "sc_paladin.hpp"
#include "class_modules/apl/apl_paladin.hpp"

//
namespace paladin {

namespace buffs {
  crusade_buff_t::crusade_buff_t( paladin_t* p ) :
      buff_t( p, "crusade", p -> spells.crusade ),
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
    if ( p -> talents.avenging_wrath -> ok() )
      damage_modifier = data().effectN( 1 ).percent() / 10.0;
    haste_bonus = data().effectN( 3 ).percent() / 10.0;

    // increase duration if we have Light's Decree
    auto* paladin = static_cast<paladin_t*>( p );
    if ( paladin -> azerite.lights_decree.ok() )
      base_buff_duration += paladin -> spells.lights_decree -> effectN( 2 ).time_value();

    if ( paladin -> is_ptr() && paladin -> talents.divine_wrath -> ok() )
    {
      base_buff_duration += paladin -> talents.divine_wrath -> effectN( 1 ).time_value();
    }


    // let the ability handle the cooldown
    cooldown -> duration = 0_ms;

    add_invalidate( CACHE_HASTE );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_MASTERY );
  }

  struct shield_of_vengeance_buff_t : public absorb_buff_t
  {
    shield_of_vengeance_buff_t( player_t* p ):
      absorb_buff_t( p, "shield_of_vengeance", p -> find_talent_spell( talent_tree::SPECIALIZATION, "Shield of Vengeance" ) )
    {
      cooldown -> duration = 0_ms;
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

      auto* p = static_cast<paladin_t*>( player );
      // do thing
      if ( p -> options.fake_sov )
      {
        // TODO(mserrano): This is a horrible hack
        p -> active.shield_of_vengeance_damage -> base_dd_max = p -> active.shield_of_vengeance_damage -> base_dd_min = current_value;
        p -> active.shield_of_vengeance_damage -> execute();
      }
    }
  };
}

// Crusade
struct crusade_t : public paladin_spell_t
{
  crusade_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "crusade", p, p -> spells.crusade )
  {
    parse_options( options_str );

    if ( ! ( p -> talents.crusade -> ok() ) )
      background = true;

    cooldown -> duration *= 1.0 + azerite::vision_of_perfection_cdr( p -> azerite_essence.vision_of_perfection );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    // If Visions already procced the buff and this spell is used, all stacks are reset to 1
    // The duration is also set to its default value, there's no extending or pandemic
    if ( p() -> buffs.crusade -> up() )
      p() -> buffs.crusade -> expire();

    p() -> buffs.crusade -> trigger();

    if ( p() -> azerite.avengers_might.ok() )
      p() -> buffs.avengers_might -> trigger( 1, p() -> buffs.avengers_might -> default_value, -1.0, p() -> buffs.crusade -> buff_duration() );
  }
};

// Execution Sentence =======================================================

struct es_explosion_t : public paladin_spell_t
{
  double accumulated;

  es_explosion_t( paladin_t* p ) :
    paladin_spell_t( "execution_sentence", p, p -> find_spell( 387113 ) ),
    accumulated( 0.0 )
  {
    dual = background = true;
    may_crit = false;

    if ( p -> talents.executioners_wrath -> ok() )
      aoe = -1;

    // apparently base damage is affected
    affected_by.hand_of_light = true;
    affected_by.divine_purpose = true;
    affected_by.judgment = true;
    affected_by.final_reckoning = true;
    affected_by.reckoning = true;
  }

  double calculate_direct_amount( action_state_t* state ) const
  {
    double amount = sim->averaged_range( base_da_min( state ), base_da_max( state ) );

    if ( round_base_dmg )
      amount = floor( amount + 0.5 );

    if ( amount == 0 && weapon_multiplier == 0 && attack_direct_power_coefficient( state ) == 0 &&
        spell_direct_power_coefficient( state ) == 0 )
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

struct execution_sentence_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  execution_sentence_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "execution_sentence", p, p -> talents.execution_sentence )
  {
    parse_options( options_str );

    // disable if not talented
    if ( ! ( p -> talents.execution_sentence -> ok() ) )
      background = true;

    // Spelldata doesn't seem to have this
    hasted_gcd = true;

    // ... this appears to be true for the base damage only,
    // and is not automatically obtained from spell data.
    // TODO: check if judgment double-dips when it's there on hit
    // as well as on cast
    affected_by.hand_of_light = true;
    affected_by.divine_purpose = true;
    affected_by.judgment = true;
    affected_by.final_reckoning = true;
    affected_by.reckoning = true;

    // unclear why this is needed...
    cooldown -> duration = data().cooldown();
  }

  void init() override
  {
    holy_power_consumer_t::init();
    snapshot_flags |= STATE_TARGET_NO_PET | STATE_MUL_TA | STATE_MUL_DA;
    update_flags &= ~STATE_TARGET;
    update_flags |= STATE_MUL_TA | STATE_MUL_DA;
  }

  void impact( action_state_t* s) override
  {
    holy_power_consumer_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuff.execution_sentence -> trigger();
    }
  }
};

// Blade of Justice =========================================================

struct blade_of_justice_t : public paladin_melee_attack_t
{

  struct conduit_expurgation_t : public paladin_spell_t
  {
    conduit_expurgation_t( paladin_t* p ):
      paladin_spell_t( "expurgation", p, p -> find_spell( 344067 ) )
    {
      hasted_ticks = false;
      tick_may_crit = false;
    }
  };

  struct expurgation_t : public paladin_spell_t
  {
    expurgation_t( paladin_t* p ):
      paladin_spell_t( "expurgation", p, p -> find_spell( 383346 ) )
    {
      hasted_ticks = false;
      tick_may_crit = false;

      if ( p -> is_ptr() )
      {
        if ( p -> talents.jurisdiction -> ok() )
        {
          base_multiplier *= 1.0 + p -> talents.jurisdiction -> effectN( 4 ).percent();
        }
      }
    }
  };

  conduit_expurgation_t* conduit_expurgation;
  expurgation_t* expurgation;

  blade_of_justice_t( paladin_t* p, util::string_view options_str ) :
    paladin_melee_attack_t( "blade_of_justice", p, p -> talents.blade_of_justice ),
    conduit_expurgation( nullptr ),
    expurgation( nullptr )
  {
    parse_options( options_str );

    if ( p -> conduit.expurgation -> ok() )
    {
      conduit_expurgation = new conduit_expurgation_t( p );
      add_child( conduit_expurgation );
    }

    if ( p -> talents.expurgation -> ok() )
    {
      expurgation = new expurgation_t( p );
      add_child( expurgation );
    }

    if ( p -> talents.holy_blade -> ok() )
    {
      energize_amount += p -> talents.holy_blade -> effectN( 1 ).resource( RESOURCE_HOLY_POWER );
    }

    if ( p -> is_ptr() )
    {
      if ( p -> talents.light_of_justice -> ok() )
      {
        cooldown -> duration += timespan_t::from_millis( p -> talents.light_of_justice -> effectN( 1 ).base_value() );
      }

      if ( p -> talents.improved_blade_of_justice -> ok() )
      {
        cooldown->charges += as<int>( p->talents.improved_blade_of_justice->effectN( 1 ).base_value() );
      }

      if ( p -> talents.jurisdiction -> ok() )
      {
        base_multiplier *= 1.0 + p -> talents.jurisdiction -> effectN( 4 ).percent();
      }

      if ( p -> talents.blade_of_vengeance -> ok() )
      {
        attack_power_mod.direct = p -> find_spell( 404358 ) -> effectN( 1 ).ap_coeff();
        aoe = -1;
        reduced_aoe_targets = 5;
      }
    }
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    if ( p() -> buffs.blade_of_wrath -> up() )
      am *= 1.0 + p() -> buffs.blade_of_wrath -> data().effectN( 1 ).percent();
    if ( p() -> buffs.sealed_verdict -> up() )
      am *= 1.0 + p() -> talents.sealed_verdict -> effectN( 1 ).percent();
    return am;
  }

  double composite_crit_chance() const override {
    double chance = paladin_melee_attack_t::composite_crit_chance();
    if ( p() -> talents.blade_of_condemnation -> ok() )
      chance += p() -> talents.blade_of_condemnation -> effectN( 1 ).percent();
    return chance;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p() -> buffs.blade_of_wrath -> up() )
      p() -> buffs.blade_of_wrath -> expire();

    if ( p() -> buffs.sealed_verdict -> up() )
      p() -> buffs.sealed_verdict -> expire();

    if ( p() -> buffs.consecrated_blade -> up() )
    {
      p() -> active.background_cons -> schedule_execute();
      p() -> buffs.consecrated_blade -> expire();
    }

    if ( p() -> is_ptr() && p() -> talents.consecrated_blade -> ok() )
    {
      p() -> active.background_cons -> schedule_execute();
    }
  }

  void impact( action_state_t* state ) override
  {
    paladin_melee_attack_t::impact( state );

    if ( state -> result == RESULT_CRIT )
    {
      if ( p() -> talents.expurgation -> ok() )
      {
        expurgation -> base_td = state -> result_amount * p() -> talents.expurgation -> effectN( 1 ).percent();
        expurgation -> set_target( state -> target );
        expurgation -> execute();
      }

      if ( p() -> conduit.expurgation -> ok() )
      {
        conduit_expurgation -> base_td = state -> result_amount * p() -> conduit.expurgation.percent();
        conduit_expurgation -> set_target( state -> target );
        conduit_expurgation -> execute();
      }
    }

    if ( p()->buffs.virtuous_command_conduit->up() && p()->active.virtuous_command_conduit )
    {
      action_t* vc    = p()->active.virtuous_command_conduit;
      vc->base_dd_min = vc->base_dd_max = state->result_amount * p()->conduit.virtuous_command.percent();
      vc->set_target( state->target );
      vc->schedule_execute();
    }

    if ( p()->buffs.virtuous_command->up() && p()->active.virtuous_command )
    {
      action_t* vc    = p()->active.virtuous_command;
      vc->base_dd_min = vc->base_dd_max = state->result_amount * p()->talents.virtuous_command->effectN( 1 ).percent();
      vc->set_target( state->target );
      vc->schedule_execute();
    }
  }
};

// Divine Storm =============================================================

struct divine_storm_tempest_t : public paladin_melee_attack_t
{
  divine_storm_tempest_t( paladin_t* p ) :
    paladin_melee_attack_t( "divine_storm_tempest", p, p -> find_spell( 224239 ) )
  {
    background = true;

    aoe = -1;
    base_multiplier *= p -> talents.tempest_of_the_lightbringer -> effectN( 1 ).percent();
  }
};

struct divine_storm_t: public holy_power_consumer_t<paladin_melee_attack_t>
{
  divine_storm_tempest_t* tempest;

  divine_storm_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "divine_storm", p, p -> talents.divine_storm ),
    tempest( nullptr )
  {
    parse_options( options_str );

    if ( !( p -> talents.divine_storm -> ok() ) )
      background = true;

    is_divine_storm = true;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();

    if ( p -> legendary.tempest_of_the_lightbringer -> ok() )
      base_multiplier *= 1.0 + p -> legendary.tempest_of_the_lightbringer -> effectN( 2 ).percent();

    if ( p -> talents.tempest_of_the_lightbringer -> ok() )
    {
      tempest = new divine_storm_tempest_t( p );
      add_child( tempest );
    }
  }

  divine_storm_t( paladin_t* p, bool is_free, double mul ) :
    holy_power_consumer_t( "divine_storm", p, p -> talents.divine_storm ),
    tempest( nullptr )
  {
    is_divine_storm = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();

    background = is_free;
    base_multiplier *= mul;

    if ( p -> legendary.tempest_of_the_lightbringer -> ok() )
      base_multiplier *= 1.0 + p -> legendary.tempest_of_the_lightbringer -> effectN( 2 ).percent();

    if ( p -> talents.tempest_of_the_lightbringer -> ok() )
    {
      tempest = new divine_storm_tempest_t( p );
      add_child( tempest );
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = holy_power_consumer_t::bonus_da( s );

    if ( p() -> buffs.empyrean_power_azerite -> up() )
    {
      b += p() -> azerite.empyrean_power.value();
    }

    return b;
  }

  double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();

    if ( p() -> buffs.empyrean_power -> up() )
      am *= 1.0 + p() -> buffs.empyrean_power -> data().effectN( 1 ).percent();

    if ( p() -> is_ptr() && p() -> buffs.inquisitors_ire -> up() )
      am *= 1.0 + p() -> buffs.inquisitors_ire -> check_stack_value();

    return am;
  }

  void execute() override
  {
    if ( p() -> bugs && p() -> buffs.empyrean_power -> up() )
    {
      if ( p() -> buffs.fires_of_justice -> up() )
        p() -> buffs.fires_of_justice -> expire();
    }

    holy_power_consumer_t::execute();

    if ( p() -> talents.tempest_of_the_lightbringer -> ok() )
    {
      tempest -> schedule_execute();
    }

    if ( p() -> is_ptr() && p() -> buffs.inquisitors_ire -> up() )
      p() -> buffs.inquisitors_ire -> expire();
  }

  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.sanctify -> ok() )
        td( s -> target ) -> debuff.sanctify -> trigger();
    }
  }
};

struct echoed_spell_event_t : public event_t
{
  paladin_melee_attack_t* echo;
  paladin_t* paladin;
  player_t* target;
  double amount;

  echoed_spell_event_t( paladin_t* p, player_t* tgt, paladin_melee_attack_t* spell, timespan_t delay, double amt = 0 ) :
    event_t( *p, delay ), echo( spell ), paladin( p ), target( tgt ), amount( amt )
  {
  }

  const char* name() const override
  { return "echoed_spell_delay"; }

  void execute() override
  {
    echo -> set_target( target );
    if ( amount > 0 )
      echo -> base_dd_min = echo -> base_dd_max = amount * echo -> base_multiplier;
    echo -> schedule_execute();
  }
};

struct templars_verdict_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  struct echoed_templars_verdict_conduit_t : public paladin_melee_attack_t
  {
    echoed_templars_verdict_conduit_t( paladin_t *p ) :
      paladin_melee_attack_t( "echoed_verdict", p, p -> find_spell( 339538 ) )
    {
      base_multiplier *= p -> conduit.templars_vindication -> effectN( 2 ).percent();
      background = true;
      may_crit = false;

      // spell data please
      aoe = 0;
    }

    double action_multiplier() const override
    {
      double am = paladin_melee_attack_t::action_multiplier();
      if ( p() -> buffs.righteous_verdict -> check() )
        am *= 1.0 + p() -> buffs.righteous_verdict -> data().effectN( 1 ).percent();
      return am;
    }
  };

  struct echoed_templars_verdict_t : public paladin_melee_attack_t
  {
    echoed_templars_verdict_t( paladin_t *p ) :
      // TODO(mserrano): this spell ID is probably wrong
      paladin_melee_attack_t( "echoed_verdict", p, p -> find_spell( 339538 ) )
    {
      base_multiplier *= p -> talents.templars_vindication -> effectN( 2 ).percent();
      background = true;
      may_crit = false;

      // spell data please
      aoe = 0;
    }

    double action_multiplier() const override
    {
      double am = paladin_melee_attack_t::action_multiplier();
      if ( p() -> buffs.righteous_verdict -> check() )
        am *= 1.0 + p() -> buffs.righteous_verdict -> data().effectN( 1 ).percent();
      return am;
    }
  };

  // Templar's Verdict damage is stored in a different spell
  struct templars_verdict_damage_t : public paladin_melee_attack_t
  {
    echoed_templars_verdict_t* echo;
    echoed_templars_verdict_conduit_t* echo_conduit;

    templars_verdict_damage_t( paladin_t *p, echoed_templars_verdict_t* e, echoed_templars_verdict_conduit_t* ec ) :
      paladin_melee_attack_t( "templars_verdict_dmg", p, p -> find_spell( 224266 ) ),
      echo( e ),
      echo_conduit( ec )
    {
      dual = background = true;

      // spell data please?
      aoe = 0;
    }

    void impact( action_state_t* s ) override
    {
      paladin_melee_attack_t::impact( s );

      if ( p()->buffs.virtuous_command_conduit->up() && p()->active.virtuous_command_conduit )
      {
        action_t* vc    = p()->active.virtuous_command_conduit;
        vc->base_dd_min = vc->base_dd_max = s->result_amount * p()->conduit.virtuous_command.percent();
        vc->set_target( s->target );
        vc->schedule_execute();
      }

      if ( p()->buffs.virtuous_command->up() && p()->active.virtuous_command )
      {
        action_t* vc    = p()->active.virtuous_command;
        vc->base_dd_min = vc->base_dd_max = s->result_amount * p()->talents.virtuous_command->effectN( 1 ).percent();
        vc->set_target( s->target );
        vc->schedule_execute();
      }

      if ( p() -> conduit.templars_vindication -> ok() )
      {
        if ( rng().roll( p() -> conduit.templars_vindication.percent() ) )
        {
          // TODO(mserrano): figure out if 600ms is still correct; there does appear to be some delay
          make_event<echoed_spell_event_t>( *sim, p(), execute_state -> target, echo_conduit, timespan_t::from_millis( 600 ), s -> result_amount );
        }
      }

      if ( p() -> talents.templars_vindication -> ok() )
      {
        if ( rng().roll( p() -> talents.templars_vindication -> effectN( 1 ).percent() ) )
        {
          // TODO(mserrano): figure out if 600ms is still correct; there does appear to be some delay
          make_event<echoed_spell_event_t>( *sim, p(), execute_state -> target, echo, timespan_t::from_millis( 600 ), s -> result_amount );
        }
      }
    }

    double action_multiplier() const override
    {
      double am = paladin_melee_attack_t::action_multiplier();
      if ( p() -> buffs.righteous_verdict -> check() )
        am *= 1.0 + p() -> buffs.righteous_verdict -> data().effectN( 1 ).percent();
      return am;
    }
  };

  echoed_templars_verdict_t* echo;
  echoed_templars_verdict_conduit_t* echo_conduit;
  bool is_fv;

  templars_verdict_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t(
      ( p -> legendary.final_verdict -> ok() || p -> talents.final_verdict -> ok() ) ? "final_verdict" : "templars_verdict",
      p,
      ( p -> talents.final_verdict -> ok() ) ? ( p -> find_spell( 383328 ) ) : ( p -> legendary.final_verdict -> ok() ? ( p -> find_spell( 336872 ) ) : ( p -> find_specialization_spell( "Templar's Verdict" ) ) ) ),
    echo( nullptr ),
    echo_conduit( nullptr ),
    is_fv( p -> legendary.final_verdict -> ok() || p -> talents.final_verdict -> ok() )
  {
    parse_options( options_str );

    // spell is not usable without a 2hander
    if ( p -> items[ SLOT_MAIN_HAND ].dbc_inventory_type() != INVTYPE_2HWEAPON )
      background = true;

    // wtf is happening in spell data?
    aoe = 0;

    if ( p -> conduit.templars_vindication -> ok() )
    {
      echo_conduit = new echoed_templars_verdict_conduit_t( p );
    }

    if ( p -> talents.templars_vindication -> ok() )
    {
      echo = new echoed_templars_verdict_t( p );
    }

    if ( p -> is_ptr() )
    {
      if ( p -> talents.jurisdiction -> ok() )
      {
        base_multiplier *= 1.0 + p -> talents.jurisdiction -> effectN( 4 ).percent();
      }
    }

    if ( ! is_fv ) {
      callbacks = false;
      may_block = false;

      impact_action = new templars_verdict_damage_t( p, echo, echo_conduit );
      impact_action -> stats = stats;

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
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }

    if ( p() -> talents.righteous_verdict -> ok() )
    {
      p() -> buffs.righteous_verdict -> expire();
      p() -> buffs.righteous_verdict -> trigger();
    }

    if ( p() -> buffs.vanquishers_hammer -> up() )
    {
      p() -> active.necrolord_divine_storm -> schedule_execute();
      p() -> buffs.vanquishers_hammer -> decrement( 1 );
    }

    if ( p() -> buffs.empyrean_legacy -> up() )
    {
      p() -> active.empyrean_legacy -> schedule_execute();
      p() -> buffs.empyrean_legacy -> expire();
    }

    // TODO(mserrano): figure out the actionbar override thing instead of this hack.
    if ( is_fv )
    {
      double proc_chance = ( p() -> talents.final_verdict -> ok() ) ? data().effectN( 2 ).percent() : p() -> legendary.final_verdict -> effectN( 2 ).percent();
      if ( rng().roll( proc_chance ) )
      {
        p() -> cooldowns.hammer_of_wrath -> reset( true );
        p() -> buffs.final_verdict -> trigger();
      }
    }

    if ( p() -> is_ptr() && !background )
    {
      if ( p() -> talents.righteous_cause -> ok() )
      {
        if ( rng().roll( p() -> talents.righteous_cause -> proc_chance() ) )
        {
          p() -> procs.righteous_cause -> occur();
          p() -> cooldowns.blade_of_justice -> reset( true );
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );

    if ( is_fv )
    {
      if ( p()->buffs.virtuous_command_conduit->up() && p()->active.virtuous_command_conduit )
      {
        action_t* vc    = p()->active.virtuous_command_conduit;
        vc->base_dd_min = vc->base_dd_max = s->result_amount * p()->conduit.virtuous_command.percent();
        vc->set_target( s->target );
        vc->schedule_execute();
      }

      if ( p()->buffs.virtuous_command->up() && p()->active.virtuous_command )
      {
        action_t* vc    = p()->active.virtuous_command;
        vc->base_dd_min = vc->base_dd_max = s->result_amount * p()->talents.virtuous_command->effectN( 1 ).percent();
        vc->set_target( s->target );
        vc->schedule_execute();
      }

      if ( p() -> conduit.templars_vindication -> ok() )
      {
        if ( rng().roll( p() -> conduit.templars_vindication.percent() ) )
        {
          // TODO(mserrano): figure out if 600ms is still correct; there does appear to be some delay
          make_event<echoed_spell_event_t>( *sim, p(), execute_state -> target, echo_conduit, timespan_t::from_millis( 600 ), s -> result_amount );
        }
      }

      if ( p() -> talents.templars_vindication -> ok() )
      {
        if ( rng().roll( p() -> talents.templars_vindication -> effectN( 1 ).percent() ) )
        {
          // TODO(mserrano): figure out if 600ms is still correct; there does appear to be some delay
          make_event<echoed_spell_event_t>( *sim, p(), execute_state -> target, echo, timespan_t::from_millis( 600 ), s -> result_amount );
        }
      }
    }

    paladin_td_t* tgt = td( s -> target );
    if ( p() -> talents.executioners_will -> ok() && tgt -> debuff.execution_sentence -> up() )
    {
      tgt -> debuff.execution_sentence -> do_will_extension();
    }
  }

  double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();
    if ( is_fv )
    {
      if ( p() -> buffs.righteous_verdict -> check() )
        am *= 1.0 + p() -> buffs.righteous_verdict -> data().effectN( 1 ).percent();

      // this happens twice apparently
      if ( p() -> bugs && p() -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T29, B4 ) )
        am *= 1.0 + p() -> sets -> set( PALADIN_RETRIBUTION, T29, B4 ) -> effectN( 1 ).percent();
    }
    return am;
  }

};

// Final Reckoning

struct reckoning_t : public paladin_spell_t
{
  reckoning_t( paladin_t* p ) : paladin_spell_t( "reckoning", p, p -> spells.reckoning ) {
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuff.reckoning -> trigger();
    }
  }
};

struct final_reckoning_t : public paladin_spell_t
{
  final_reckoning_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "final_reckoning", p, p -> talents.final_reckoning )
  {
    parse_options( options_str );

    aoe = -1;

    if ( ! ( p -> talents.final_reckoning -> ok() ) )
      background = true;
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuff.final_reckoning -> trigger();
    }
  }
};

// Judgment - Retribution =================================================================

struct seal_of_wrath_t : public paladin_spell_t
{
  seal_of_wrath_t( paladin_t* p ) :
    paladin_spell_t( "seal_of_wrath", p, p -> find_spell( 386911 ) )
  {
    background = true;
  }
};

struct judgment_ret_t : public judgment_t
{
  int holy_power_generation;
  seal_of_wrath_t* seal_of_wrath;
  bool is_boundless;

  judgment_ret_t( paladin_t* p, util::string_view name, util::string_view options_str ) :
    judgment_t( p, name ),
    holy_power_generation( as<int>( p -> find_spell( 220637 ) -> effectN( 1 ).base_value() ) ),
    seal_of_wrath( nullptr ),
    is_boundless( false )
  {
    parse_options( options_str );

    if ( p -> talents.highlords_judgment -> ok() )
      base_multiplier *= 1.0 + p -> talents.highlords_judgment -> effectN( 1 ).percent();

    if ( p -> talents.timely_judgment -> ok() )
      cooldown->duration += timespan_t::from_millis( p -> talents.timely_judgment -> effectN( 1 ).base_value() );

    if ( p -> talents.seal_of_wrath -> ok() )
    {
      seal_of_wrath = new seal_of_wrath_t( p );
    }

    if ( p -> is_ptr() )
    {
      if ( p -> talents.boundless_judgment -> ok() )
      {
        holy_power_generation += p -> talents.boundless_judgment -> effectN( 1 ).base_value();
      }

      if ( p -> talents.blessed_champion -> ok() )
      {
        aoe = 1 + p -> talents.blessed_champion -> effectN( 4 ).base_value();
        base_aoe_multiplier *= 1.0 - p -> talents.blessed_champion -> effectN( 3 ).percent();
      }

      if ( p -> talents.judge_jury_and_executioner -> ok() )
      {
        base_crit += p -> talents.judge_jury_and_executioner -> effectN( 1 ).percent();
      }
    }
  }

  judgment_ret_t( paladin_t* p, util::string_view name, bool is_divine_toll ) :
    judgment_t( p, name ),
    holy_power_generation( as<int>( p -> find_spell( 220637 ) -> effectN( 1 ).base_value() ) ),
    seal_of_wrath( nullptr ),
    is_boundless( false )
  {
    // This is for Divine Toll's background judgments
    background = true;

    if ( p -> talents.highlords_judgment -> ok() )
      base_multiplier *= 1.0 + p -> talents.highlords_judgment -> effectN( 1 ).percent();

    // according to skeletor this is given the bonus of 326011
    if ( is_divine_toll )
      base_multiplier *= 1.0 + p -> find_spell( 326011 ) -> effectN( 1 ).percent();

    if ( p -> talents.seal_of_wrath -> ok() )
    {
      seal_of_wrath = new seal_of_wrath_t( p );
    }
  }

  void init() override
  {
    judgment_t::init();
    is_boundless = false;
  }

  void execute() override
  {
    if ( p() -> talents.boundless_judgment -> ok() && !( p() -> is_ptr() ) )
    {
      if ( rng().roll( p() -> talents.boundless_judgment -> effectN( 1 ).percent() ) )
      {
        is_boundless = true;
      }
    }

    judgment_t::execute();

    if ( p() -> talents.zeal -> ok() )
    {
      p() -> buffs.zeal -> trigger( as<int>( p() -> talents.zeal -> effectN( 1 ).base_value() ) );
    }

    if ( p() -> spec.judgment_3 -> ok() )
      p() -> resource_gain( RESOURCE_HOLY_POWER, holy_power_generation, p() -> gains.judgment );

    if ( p() -> talents.empyrean_legacy -> ok() )
    {
      if ( ! ( p() -> buffs.empyrean_legacy_cooldown -> up() ) )
      {
        p() -> buffs.empyrean_legacy -> trigger();
        p() -> buffs.empyrean_legacy_cooldown -> trigger();
      }
    }

    if ( p() -> talents.virtuous_command -> ok() )
      p() -> buffs.virtuous_command -> trigger();

    is_boundless = false;
  }

  int n_targets() const override
  {
    int target_count = judgment_t::n_targets();

    if ( is_boundless )
      target_count = 2;

    return target_count;
  }

  // Special things that happen when Judgment damages target
  void impact( action_state_t* s ) override
  {
    judgment_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.seal_of_wrath -> ok() )
      {
        if ( rng().roll( p() -> talents.seal_of_wrath -> effectN( 1 ).percent() ) )
        {
          seal_of_wrath -> set_target( s -> target );
          seal_of_wrath -> schedule_execute();
        }
      }
    }
  }
};

// Justicar's Vengeance
struct justicars_vengeance_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  justicars_vengeance_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "justicars_vengeance", p, p -> talents.justicars_vengeance )
  {
    parse_options( options_str );

    // Spelldata doesn't have this
    hasted_gcd = true;

    weapon_multiplier = 0; // why is this needed?

    // Healing isn't implemented

    if ( p -> is_ptr() )
    {
      if ( p -> talents.jurisdiction -> ok() )
      {
        base_multiplier *= 1.0 + p -> talents.jurisdiction -> effectN( 4 ).percent();
      }
    }
  }


  void execute() override
  {
    holy_power_consumer_t::execute();

    if ( p() -> is_ptr() && p() -> talents.righteous_cause -> ok() )
    {
      if ( rng().roll( p() -> talents.righteous_cause -> proc_chance() ) )
      {
        p() -> procs.righteous_cause -> occur();
        p() -> cooldowns.blade_of_justice -> reset( true );
      }
    }
  }
};

// SoV

struct shield_of_vengeance_proc_t : public paladin_spell_t
{
  shield_of_vengeance_proc_t( paladin_t* p ) :
    paladin_spell_t( "shield_of_vengeance_proc", p, p -> find_spell( 184689 ) )
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
    paladin_absorb_t( "shield_of_vengeance", p, p -> talents.shield_of_vengeance )
  {
    parse_options( options_str );

    harmful = false;

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );
  }

  void init() override
  {
    paladin_absorb_t::init();
    snapshot_flags |= (STATE_CRIT | STATE_VERSATILITY);
  }

  void execute() override
  {
    double shield_amount = p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent();
    paladin_absorb_t::execute();
    p() -> buffs.shield_of_vengeance -> trigger( 1, shield_amount );
  }
};


// Wake of Ashes (Retribution) ================================================

struct truths_wake_t : public paladin_spell_t
{
  truths_wake_t( paladin_t* p, util::string_view name ) :
    paladin_spell_t( name, p, p -> find_spell( 383351 ) )
  {
    hasted_ticks = false;
    tick_may_crit = false;
  }
};

struct wake_of_ashes_t : public paladin_spell_t
{
  struct truths_wake_conduit_t : public paladin_spell_t
  {
    truths_wake_conduit_t( paladin_t* p ) :
      paladin_spell_t( "truths_wake_conduit", p, p -> find_spell( 339376 ) )
    {
      hasted_ticks = false;
      tick_may_crit = false;
    }
  };


  truths_wake_conduit_t* truths_wake_conduit;
  truths_wake_t* truths_wake;

  wake_of_ashes_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "wake_of_ashes", p, p -> talents.wake_of_ashes ),
    truths_wake_conduit( nullptr ),
    truths_wake( nullptr )
  {
    parse_options( options_str );

    if ( p -> talents.radiant_decree -> ok() ||  !( p -> talents.wake_of_ashes -> ok() ) )
      background = true;

    may_crit = true;
    full_amount_targets = 1;
    reduced_aoe_targets = 1.0;

    aoe = -1;

    if ( p -> conduit.truths_wake -> ok() )
    {
      truths_wake_conduit = new truths_wake_conduit_t( p );
      add_child( truths_wake_conduit );
    }

    if ( p -> talents.truths_wake -> ok() )
    {
      truths_wake = new truths_wake_t( p, "truths_wake_woa" );
      add_child( truths_wake );
    }
  }

  void impact( action_state_t* s) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> conduit.truths_wake -> ok() )
      {
        double truths_wake_mul = p() -> conduit.truths_wake.percent() / p() -> conduit.truths_wake -> effectN( 2 ).base_value();
        truths_wake_conduit -> base_td = s -> result_amount * truths_wake_mul;
        truths_wake_conduit -> set_target( s -> target );
        truths_wake_conduit -> execute();
      }

      if ( p() -> talents.truths_wake -> ok() )
      {
        double truths_wake_mul = p() -> talents.truths_wake -> effectN( 1 ).percent() / p() -> talents.truths_wake -> effectN( 2 ).base_value();
        truths_wake -> base_td = s -> result_amount * truths_wake_mul;
        truths_wake -> set_target( s -> target );
        truths_wake -> execute();
      }
    }
  }
};

// Radiant Decree

struct radiant_decree_t : public holy_power_consumer_t<paladin_spell_t>
{
  truths_wake_t* truths_wake;

  radiant_decree_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "radiant_decree", p, p -> find_spell( 383469 ) ),
    truths_wake( nullptr )
  {
    parse_options( options_str );

    if ( !( p -> talents.radiant_decree -> ok() ) )
      background = true;

    if ( p -> talents.truths_wake -> ok() )
    {
      truths_wake = new truths_wake_t( p, "truths_wake_rd" );
      add_child( truths_wake );
    }

    reduced_aoe_targets = 5.0;

    aoe = -1;
  }

  void impact( action_state_t* s) override
  {
    holy_power_consumer_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.truths_wake -> ok() )
      {
        double truths_wake_mul = p() -> talents.truths_wake -> effectN( 1 ).percent() / p() -> talents.truths_wake -> effectN( 2 ).base_value();
        truths_wake -> base_td = s -> result_amount * truths_wake_mul;
        truths_wake -> set_target( s -> target );
        truths_wake -> execute();
      }
    }
  }
};

struct zeal_t : public paladin_melee_attack_t
{
  zeal_t( paladin_t* p ) : paladin_melee_attack_t( "zeal", p, p -> find_spell( 269937 ) )
  { background = true; }
};

struct exorcism_t : public paladin_spell_t
{
  struct exorcism_dot_t : public paladin_spell_t
  {
    exorcism_dot_t( paladin_t* p ) :
      paladin_spell_t( "exorcism_dot", p, p -> find_spell( 383208 ) )
    {
      background = true;
      if ( p->bugs )
        tick_may_crit = false;
    }

    int n_targets() const override
    {
      // TODO(mserrano): change this to use the target instead
      // technically this should be based on the target, but for now just use the paladin themselves
      if ( p() -> standing_in_consecration() )
        return -1;

      return paladin_spell_t::n_targets();
    }
  };

  // Exorcism damage is stored in a different spell
  struct exorcism_damage_t : public paladin_spell_t
  {
    exorcism_damage_t( paladin_t *p ) :
      paladin_spell_t( "exorcism_dmg", p, p -> find_spell( 383921 ) )
    {
      dual = background = true;
    }
  };

  exorcism_dot_t* dot_action;

  exorcism_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "exorcism", p, p -> talents.exorcism ),
    dot_action( nullptr )
  {
    parse_options( options_str );

    impact_action = new exorcism_damage_t( p );
    impact_action -> stats = stats;

    dot_action = new exorcism_dot_t( p );
    add_child( dot_action );
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      dot_action -> target = s -> target;
      dot_action -> schedule_execute();
    }
  }
};

struct divine_hammer_tick_t : public paladin_melee_attack_t
{
  divine_hammer_tick_t( paladin_t* p )
    : paladin_melee_attack_t( "divine_hammer_tick", p, p -> find_spell( 198137 ) )
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

    if ( !p->is_ptr() || !p->talents.divine_hammer->ok() )
      background = true;

    may_miss       = false;
    tick_may_crit  = true;
    // TODO: verify
    tick_zero      = true;

    tick_action = new divine_hammer_tick_t( p );
  }
};

void paladin_t::trigger_es_explosion( player_t* target )
{
  double ta = 0.0;
  double accumulated = get_target_data( target ) -> debuff.execution_sentence -> get_accumulated_damage();
  sim -> print_debug( "{}'s execution_sentence has accumulated {} total additional damage.", target -> name(), accumulated );
  ta += accumulated;

  es_explosion_t* explosion = static_cast<es_explosion_t*>( active.es_explosion );
  explosion->set_target( target );
  explosion->accumulated = ta;
  explosion->schedule_execute();
}

// Initialization

void paladin_t::create_ret_actions()
{
  if ( talents.sanctified_wrath->ok() && specialization() == PALADIN_RETRIBUTION )
  {
    active.sanctified_wrath = new sanctified_wrath_t( this );
  }

  if ( talents.zeal->ok() )
  {
    active.zeal = new zeal_t( this );
  }

  if ( talents.final_reckoning->ok() )
  {
    active.reckoning = new reckoning_t( this );
  }

  active.shield_of_vengeance_damage = new shield_of_vengeance_proc_t( this );
  double necrolord_mult = 1.0 + covenant.necrolord -> effectN( 2 ).percent();
  active.necrolord_divine_storm = new divine_storm_t( this, true, necrolord_mult );
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
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "wake_of_ashes"             ) return new wake_of_ashes_t            ( this, options_str );
  if ( name == "justicars_vengeance"       ) return new justicars_vengeance_t      ( this, options_str );
  if ( name == "shield_of_vengeance"       ) return new shield_of_vengeance_t      ( this, options_str );
  if ( name == "final_reckoning"           ) return new final_reckoning_t          ( this, options_str );

  if ( is_ptr() )
  {
    if ( name == "divine_hammer" ) return new divine_hammer_t( this, options_str );
  }
  else
  {
    if ( name == "radiant_decree" ) return new radiant_decree_t( this, options_str );
    if ( name == "exorcism" ) return new exorcism_t( this, options_str );
  }

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    if ( name == "judgment") return new judgment_ret_t( this, "judgment", options_str );
  }

  return nullptr;
}

void paladin_t::create_buffs_retribution()
{
  buffs.blade_of_wrath = make_buff( this, "blade_of_wrath", find_spell( 281178 ) )
                       -> set_default_value( find_spell( 281178 ) -> effectN( 1 ).percent() );
  buffs.crusade = new buffs::crusade_buff_t( this );
  buffs.fires_of_justice = make_buff( this, "fires_of_justice", talents.fires_of_justice -> effectN( 1 ).trigger() )
                         -> set_chance( talents.fires_of_justice -> ok() ? talents.fires_of_justice -> proc_chance() : 0 );
  buffs.righteous_verdict = make_buff( this, "righteous_verdict", find_spell( 267611 ) );

  buffs.shield_of_vengeance = new buffs::shield_of_vengeance_buff_t( this );
  buffs.zeal = make_buff( this, "zeal", find_spell( 269571 ) )
             -> add_invalidate( CACHE_ATTACK_SPEED )
             -> set_cooldown( timespan_t::from_millis( 500 ) );

  buffs.vanguards_momentum = make_buff( this, "vanguards_momentum", find_spell( 383311 ) )
                                        -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                        -> set_default_value( talents.vanguards_momentum->effectN( 1 ).percent() );

  buffs.sealed_verdict = make_buff( this, "sealed_verdict", find_spell( 387643 ) );
  buffs.consecrated_blade = make_buff( this, "consecrated_blade", find_spell( 382522 ) );
  buffs.rush_of_light = make_buff( this, "rush_of_light", find_spell( 407065 ) )
    -> add_invalidate( CACHE_HASTE )
    -> set_default_value( talents.rush_of_light->effectN( 1 ).percent() );

  buffs.inquisitors_ire = make_buff( this, "inquisitors_ire", find_spell( 403976 ) )
                            -> set_default_value( find_spell( 403976 ) -> effectN( 1 ).percent() );
  buffs.inquisitors_ire_driver = make_buff( this, "inquisitors_ire_driver", find_spell( 403975 ) )
                                  -> set_period( find_spell( 403975 ) -> effectN( 1 ).period() )
                                  -> set_quiet( true )
                                  -> set_tick_callback([this](buff_t*, int, const timespan_t&) { buffs.inquisitors_ire -> trigger(); })
                                  -> set_tick_time_behavior( buff_tick_time_behavior::UNHASTED );

  // Azerite
  buffs.empyrean_power_azerite = make_buff( this, "empyrean_power_azerite", find_spell( 286393 ) )
                       -> set_default_value( azerite.empyrean_power.value() );
  buffs.empyrean_power = make_buff( this, "empyrean_power", find_spell( 326733 ) )
                          ->set_trigger_spell(talents.empyrean_power);
  buffs.relentless_inquisitor_azerite = make_buff<stat_buff_t>(this, "relentless_inquisitor_azerite", find_spell( 279204 ) )
                              -> add_stat( STAT_HASTE_RATING, azerite.relentless_inquisitor.value() );

  // legendaries
  buffs.vanguards_momentum_legendary = make_buff( this, "vanguards_momentum_legendary", find_spell( 345046 ) )
                                        -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                        -> set_default_value( find_spell( 345046 ) -> effectN( 1 ).percent() );

  buffs.inner_grace = make_buff( this, "inner_grace", talents.inner_grace )
    -> set_tick_zero( false )
    -> set_period( 12_s ) // weirdly this is currently set to 1s
    -> set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
        // this 1 appears hardcoded
        resource_gain( RESOURCE_HOLY_POWER, 1, gains.hp_inner_grace );
      } );

  buffs.empyrean_legacy = make_buff( this, "empyrean_legacy", find_spell( 387178 ) );
  buffs.empyrean_legacy_cooldown = make_buff( this, "empyrean_legacy_cooldown", find_spell( 387441 ) );
}

void paladin_t::init_rng_retribution()
{
  final_reckoning_rppm = get_rppm( "final_reckoning", find_spell( 343723 ) );
  final_reckoning_rppm -> set_scaling( RPPM_HASTE );
}

void paladin_t::init_spells_retribution()
{
  // Talents
  talents.blade_of_justice            = find_talent_spell( talent_tree::SPECIALIZATION, "Blade of Justice" );
  talents.divine_storm                = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Storm" );
  talents.art_of_war                  = find_talent_spell( talent_tree::SPECIALIZATION, "Art of War" );
  talents.timely_judgment             = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Judgment" );
  talents.improved_crusader_strike    = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Crusader Strike" );
  talents.holy_crusader               = find_talent_spell( talent_tree::SPECIALIZATION, "Holy Crusader" );
  talents.holy_blade                  = find_talent_spell( talent_tree::SPECIALIZATION, "Holy Blade" );
  talents.blade_of_condemnation       = find_talent_spell( talent_tree::SPECIALIZATION, "Blade of Condemnation" );
  talents.zeal                        = find_talent_spell( talent_tree::SPECIALIZATION, "Zeal" );
  talents.shield_of_vengeance         = find_talent_spell( talent_tree::SPECIALIZATION, "Shield of Vengeance" );
  talents.divine_protection           = find_talent_spell( talent_tree::SPECIALIZATION, "Divine Protection" );
  talents.blade_of_wrath              = find_talent_spell( talent_tree::SPECIALIZATION, "Blade of Wrath" );
  talents.highlords_judgment          = find_talent_spell( talent_tree::SPECIALIZATION, "Highlord's Judgment" );
  talents.righteous_verdict           = find_talent_spell( talent_tree::SPECIALIZATION, "Righteous Verdict" );
  talents.sanctify                    = find_talent_spell( talent_tree::SPECIALIZATION, "Sanctify" );
  talents.wake_of_ashes               = find_talent_spell( talent_tree::SPECIALIZATION, "Wake of Ashes" );
  talents.consecrated_blade           = find_talent_spell( talent_tree::SPECIALIZATION, "Consecrated Blade" );
  talents.seal_of_wrath               = find_talent_spell( talent_tree::SPECIALIZATION, "Seal of Wrath" );
  talents.expurgation                 = find_talent_spell( talent_tree::SPECIALIZATION, "Expurgation" );
  talents.boundless_judgment          = find_talent_spell( talent_tree::SPECIALIZATION, "Boundless Judgment" );
  talents.sanctification              = find_talent_spell( talent_tree::SPECIALIZATION, "Sanctification" );
  talents.inner_grace                 = find_talent_spell( talent_tree::SPECIALIZATION, "Inner Grace" );
  talents.ashes_to_dust               = find_talent_spell( talent_tree::SPECIALIZATION, "Ashes to Dust" );
  talents.radiant_decree              = find_talent_spell( talent_tree::SPECIALIZATION, "Radiant Decree" );
  talents.crusade                     = find_talent_spell( talent_tree::SPECIALIZATION, "Crusade" );
  talents.truths_wake                 = find_talent_spell( talent_tree::SPECIALIZATION, "Truth's Wake" );
  talents.empyrean_power              = find_talent_spell( talent_tree::SPECIALIZATION, "Empyrean Power" );
  talents.fires_of_justice            = find_talent_spell( talent_tree::SPECIALIZATION, "Fires of Justice" );
  talents.sealed_verdict              = find_talent_spell( talent_tree::SPECIALIZATION, "Sealed Verdict" );
  talents.consecrated_ground_ret      = find_talent_spell( talent_tree::SPECIALIZATION, "Consecrated Ground", PALADIN_RETRIBUTION );
  talents.sanctified_ground_ret       = find_talent_spell( talent_tree::SPECIALIZATION, "Sanctified Ground" );
  talents.exorcism                    = find_talent_spell( talent_tree::SPECIALIZATION, "Exorcism" );
  talents.hand_of_hindrance           = find_talent_spell( talent_tree::SPECIALIZATION, "Hand of Hindrance" );
  talents.selfless_healer             = find_talent_spell( talent_tree::SPECIALIZATION, "Selfless Healer" );
  talents.healing_hands               = find_talent_spell( talent_tree::SPECIALIZATION, "Healing Hands" );
  talents.tempest_of_the_lightbringer = find_talent_spell( talent_tree::SPECIALIZATION, "Tempest of the Lightbringer" );
  talents.justicars_vengeance         = find_talent_spell( talent_tree::SPECIALIZATION, "Justicar's Vengeance" );
  talents.eye_for_an_eye              = find_talent_spell( talent_tree::SPECIALIZATION, "Eye for an Eye" );
  talents.ashes_to_ashes              = find_talent_spell( talent_tree::SPECIALIZATION, "Ashes to Ashes" );
  talents.templars_vindication        = find_talent_spell( talent_tree::SPECIALIZATION, "Templar's Vindication" );
  talents.execution_sentence          = find_talent_spell( talent_tree::SPECIALIZATION, "Execution Sentence" );
  talents.empyrean_legacy             = find_talent_spell( talent_tree::SPECIALIZATION, "Empyrean Legacy" );
  talents.virtuous_command            = find_talent_spell( talent_tree::SPECIALIZATION, "Virtuous Command" );
  talents.final_verdict               = find_talent_spell( talent_tree::SPECIALIZATION, "Final Verdict" );
  talents.executioners_will           = find_talent_spell( talent_tree::SPECIALIZATION, "Executioner's Will" );
  talents.executioners_wrath          = find_talent_spell( talent_tree::SPECIALIZATION, "Executioner's Wrath" );
  talents.final_reckoning             = find_talent_spell( talent_tree::SPECIALIZATION, "Final Reckoning" );
  talents.vanguards_momentum          = find_talent_spell( talent_tree::SPECIALIZATION, "Vanguard's Momentum" );

  talents.swift_justice               = find_talent_spell( talent_tree::SPECIALIZATION, "Swift Justice" );
  talents.light_of_justice            = find_talent_spell( talent_tree::SPECIALIZATION, "Light of Justice" );
  talents.judgment_of_justice         = find_talent_spell( talent_tree::SPECIALIZATION, "Judgment of Justice");
  talents.improved_blade_of_justice   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Blade of Justice" );
  talents.righteous_cause             = find_talent_spell( talent_tree::SPECIALIZATION, "Righteous Cause" );
  talents.jurisdiction                = find_talent_spell( talent_tree::SPECIALIZATION, "Jurisdiction" ); // TODO: range increase
  talents.inquisitors_ire             = find_talent_spell( talent_tree::SPECIALIZATION, "Inquisitor's Ire" ); // TODO
  talents.zealots_fervor              = find_talent_spell( talent_tree::SPECIALIZATION, "Zealot's Fervor" );
  talents.rush_of_light               = find_talent_spell( talent_tree::SPECIALIZATION, "Rush of Light" );
  talents.improved_judgment           = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Judgment" );
  talents.blessed_champion            = find_talent_spell( talent_tree::SPECIALIZATION, "Blessed Champion" );
  talents.judge_jury_and_executioner  = find_talent_spell( talent_tree::SPECIALIZATION, "Judge, Jury and Executioner" );
  talents.penitence                   = find_talent_spell( talent_tree::SPECIALIZATION, "Penitence" );

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

  spells.lights_decree = find_spell( 286231 );
  spells.reckoning = find_spell( 343724 );
  spells.sanctified_wrath_damage = find_spell( 326731 );
  spells.crusade = find_spell( 231895 );

  // Azerite traits
  azerite.expurgation           = find_azerite_spell( "Expurgation" );
  azerite.relentless_inquisitor = find_azerite_spell( "Relentless Inquisitor" );
  azerite.empyrean_power        = find_azerite_spell( "Empyrean Power" );
  azerite.lights_decree         = find_azerite_spell( "Light's Decree" );
}

void empyrean_power( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 286392 );

  buff_t* buff = buff_t::find( effect.player, "empyrean_power" );

  effect.custom_buff = buff;
  effect.spell_id = driver -> id();

  struct empyrean_power_cb_t : public dbc_proc_callback_t
  {
    empyrean_power_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      if ( proc_buff )
        proc_buff -> trigger();
    }
  };

  new empyrean_power_cb_t( effect );
}

// Action Priority List Generation
void paladin_t::generate_action_prio_list_ret()
{
  paladin_apl::retribution( this );
}

} // end namespace paladin

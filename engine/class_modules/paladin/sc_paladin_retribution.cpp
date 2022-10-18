#include <unordered_set>

#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

namespace paladin {

namespace buffs {
  crusade_buff_t::crusade_buff_t( paladin_t* p ) :
      buff_t( p, "crusade", p -> find_spell( 231895 ) ),
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
    paladin_spell_t( "crusade", p, p -> find_spell( 231895 ) )
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

struct execution_sentence_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  struct es_explosion_t : public holy_power_consumer_t<paladin_melee_attack_t>
  {
    double accumulated;

    es_explosion_t( paladin_t* p ) :
      holy_power_consumer_t( "execution_sentence", p, p -> talents.executioners_wrath -> ok() ? p -> find_spell( 387200 ) : p -> find_spell( 387113 ) ),
      accumulated( 0.0 )
    {
      dual = background = true;
      may_crit = false;

      if ( p -> talents.executioners_wrath -> ok() )
        aoe = -1;
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

  es_explosion_t* explosion;

  execution_sentence_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "execution_sentence", p, p -> talents.execution_sentence ),
    explosion( nullptr )
  {
    parse_options( options_str );

    // disable if not talented
    if ( ! ( p -> talents.execution_sentence -> ok() ) )
      background = true;

    // Spelldata doesn't seem to have this
    hasted_gcd = true;

    tick_may_crit = may_crit = false;

    // ... this appears to be true for the base damage only,
    // and is not automatically obtained from spell data.
    // TODO: check if judgment double-dips when it's there on hit
    // as well as on cast
    affected_by.hand_of_light = true;
    affected_by.divine_purpose = true;
    affected_by.judgment = true;
    affected_by.final_reckoning = true;
    affected_by.reckoning = true;

    explosion = new es_explosion_t( p );
  }

  void init() override
  {
    holy_power_consumer_t::init();
    snapshot_flags |= STATE_TARGET_NO_PET | STATE_MUL_TA | STATE_MUL_DA;
    update_flags &= ~STATE_TARGET;
    update_flags |= STATE_MUL_TA | STATE_MUL_DA;

    base_tick_time = data().duration();
    dot_duration = data().duration();
  }

  void impact( action_state_t* s) override
  {
    holy_power_consumer_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuff.execution_sentence -> trigger();
    }
  }

  void last_tick( dot_t* d ) override
  {
    double ta = 0.0;
    double accumulated = td( d->target ) -> debuff.execution_sentence -> get_accumulated_damage();
    sim -> print_debug( "{}'s {} has accumulated {} total additional damage.", player -> name(), name(), accumulated );
    ta += accumulated * data().effectN( 2 ).percent();

    explosion->set_target( d->target );
    explosion->accumulated = ta;
    explosion->schedule_execute();

    holy_power_consumer_t::last_tick( d );
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

    aoe = as<int>( data().effectN( 2 ).base_value() );

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
    aoe = as<int>( data().effectN( 2 ).base_value() );

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

    return am;
  }

  void execute() override
  {
    holy_power_consumer_t::execute();

    if ( p() -> talents.tempest_of_the_lightbringer -> ok() )
    {
      tempest -> schedule_execute();
    }
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
  }

  judgment_ret_t( paladin_t* p, util::string_view name, bool is_divine_toll ) :
    judgment_t( p, name ),
    holy_power_generation( as<int>( p -> find_spell( 220637 ) -> effectN( 1 ).base_value() ) ),
    seal_of_wrath( nullptr ),
    is_boundless( false )
  {
    // This is for Divine Toll's background judgments
    background = true;

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
    if ( p() -> talents.boundless_judgment -> ok() )
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
  truths_wake_t( paladin_t* p ) :
    paladin_spell_t( "truths_wake", p, p -> find_spell( 383351 ) )
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

    may_crit = true;
    full_amount_targets = 1;
    reduced_aoe_targets = 1.06; // this is approximate, from ingame testing

    aoe = -1;

    if ( p -> conduit.truths_wake -> ok() )
    {
      truths_wake_conduit = new truths_wake_conduit_t( p );
      add_child( truths_wake_conduit );
    }

    if ( p -> talents.truths_wake -> ok() )
    {
      truths_wake = new truths_wake_t( p );
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
        truths_wake_conduit -> base_td = s -> result_raw * truths_wake_mul;
        truths_wake_conduit -> set_target( s -> target );
        truths_wake_conduit -> execute();
      }

      if ( p() -> talents.truths_wake -> ok() )
      {
        double truths_wake_mul = p() -> talents.truths_wake -> effectN( 1 ).percent();
        truths_wake -> base_td = s -> result_raw * truths_wake_mul;
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
    holy_power_consumer_t( "radiant_decree", p, p -> talents.radiant_decree ),
    truths_wake( nullptr )
  {
    parse_options( options_str );

    if ( !( p -> talents.radiant_decree -> ok() ) )
      background = true;

    if ( p -> talents.truths_wake -> ok() )
    {
      truths_wake = new truths_wake_t( p );
      add_child( truths_wake );
    }
  }

  void impact( action_state_t* s) override
  {
    holy_power_consumer_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.truths_wake -> ok() )
      {
        double truths_wake_mul = p() -> talents.truths_wake -> effectN( 1 ).percent();
        truths_wake -> base_td = s -> result_raw * truths_wake_mul;
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
  if ( name == "radiant_decree"            ) return new radiant_decree_t           ( this, options_str );
  if ( name == "exorcism"                  ) return new exorcism_t                 ( this, options_str );

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

  // Spec passives and useful spells
  spec.retribution_paladin = find_specialization_spell( "Retribution Paladin" );
  mastery.hand_of_light = find_mastery_spell( PALADIN_RETRIBUTION );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    spec.judgment_3 = find_specialization_spell( 315867 );
    spec.judgment_4 = find_specialization_spell( 231663 );

    spells.judgment_debuff = find_spell( 197277 );
  }

  passives.boundless_conviction = find_spell( 115675 );

  spells.lights_decree = find_spell( 286231 );
  spells.reckoning = find_spell( 343724 );
  spells.sanctified_wrath_damage = find_spell( 326731 );

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
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Raid consumables
  precombat -> add_action( "flask" );
  precombat -> add_action( "food" );
  precombat -> add_action( "augmentation" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Precombat casts
  precombat -> add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent" );
  precombat -> add_action( "arcane_torrent,if=talent.final_reckoning&talent.seraphim" );
  precombat -> add_action( "blessing_of_the_seasons" );
  precombat -> add_action( this, "Shield of Vengeance" );

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* cds = get_action_priority_list( "cooldowns" );
  action_priority_list_t* es_fr_active = get_action_priority_list( "es_fr_active" );
  action_priority_list_t* es_fr_pooling = get_action_priority_list( "es_fr_pooling" );
  action_priority_list_t* generators = get_action_priority_list( "generators" );
  action_priority_list_t* finishers = get_action_priority_list( "finishers" );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Rebuke" );
  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "call_action_list,name=es_fr_pooling,if=(!raid_event.adds.exists|raid_event.adds.up|raid_event.adds.in<9|raid_event.adds.in>30)&(talent.execution_sentence&cooldown.execution_sentence.remains<9&spell_targets.divine_storm<5|talent.final_reckoning&cooldown.final_reckoning.remains<9)&target.time_to_die>8" );
  def -> add_action( "call_action_list,name=es_fr_active,if=debuff.execution_sentence.up|debuff.final_reckoning.up" );
  def -> add_action( "call_action_list,name=generators" );

  if ( sim -> allow_potions )
  {
    cds -> add_action( "potion,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<25" );
  }



  std::vector<std::string> racial_actions = get_racial_actions();
  for (auto & racial_action : racial_actions)
  {

    if (racial_action == "lights_judgment" )
    {
      cds -> add_action( "lights_judgment,if=spell_targets.lights_judgment>=2|!raid_event.adds.exists|raid_event.adds.in>75|raid_event.adds.up" );
    }

    if ( racial_action == "fireblood" )
    {
      cds -> add_action( "fireblood,if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10)&!talent.execution_sentence" );
    }

    /*
    TODO(mserrano): work out if the other racials are worth using
    else
    {
      opener -> add_action( racial_actions[ i ] );
      cds -> add_action( racial_actions[ i ] );
    } */
  }
  cds -> add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent,interrupt_immediate=1,interrupt_global=1,interrupt_if=soulbind.volatile_solvent" );
  cds -> add_action( this, "Shield of Vengeance", "if=(!talent.execution_sentence|cooldown.execution_sentence.remains<52)&fight_remains>15" );
  cds -> add_action( "blessing_of_the_seasons" );

  // Items

  // special-cased items
  std::unordered_set<std::string> special_items { "skulkers_wing", "macabre_sheet_music", "memory_of_past_sins", "dreadfire_vessel", "darkmoon_deck_voracity", "overwhelming_power_crystal", "spare_meat_hook", "grim_codex", "inscrutable_quantum_device", "salvaged_fusion_amplifier", "the_first_sigil", "scars_of_fraternal_strife", "chains_of_domination", "earthbreakers_impact", "heart_of_the_swarm", "cosmic_gladiators_badge_of_ferocity", "cosmic_aspirants_badge_of_ferocity", "unchained_gladiators_badge_of_ferocity", "unchained_aspirants_badge_of_ferocity", "sinful_gladiators_badge_of_ferocity", "sinful_aspirants_badge_of_ferocity", "faulty_countermeasure", "giant_ornamental_pearl", "windscar_whetstone", "gavel_of_the_first_arbiter" };

  cds -> add_action( "use_item,name=gavel_of_the_first_arbiter" );
  cds -> add_action( "use_item,name=the_first_sigil,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<20" );
  cds -> add_action( "use_item,name=inscrutable_quantum_device,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<30" );
  cds -> add_action( "use_item,name=overwhelming_power_crystal,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<15" );
  cds -> add_action( "use_item,name=darkmoon_deck_voracity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<20" );
  cds -> add_action( "use_item,name=macabre_sheet_music,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<20" );
  cds -> add_action( "use_item,name=faulty_countermeasure,if=!talent.crusade|buff.crusade.up|fight_remains<30" );
  cds -> add_action( "use_item,name=dreadfire_vessel" );
  cds -> add_action( "use_item,name=skulkers_wing" );
  cds -> add_action( "use_item,name=grim_codex" );
  cds -> add_action( "use_item,name=memory_of_past_sins" );
  cds -> add_action( "use_item,name=spare_meat_hook" );
  cds -> add_action( "use_item,name=salvaged_fusion_amplifier" );
  cds -> add_action( "use_item,name=giant_ornamental_pearl" );
  cds -> add_action( "use_item,name=windscar_whetstone" );
  cds -> add_action( "use_item,name=scars_of_fraternal_strife" );
  cds -> add_action( "use_item,name=chains_of_domination" );
  cds -> add_action( "use_item,name=earthbreakers_impact" );
  cds -> add_action( "use_item,name=heart_of_the_swarm,if=!buff.avenging_wrath.up&!buff.crusade.up" );
  cds -> add_action( "use_item,name=cosmic_gladiators_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cds -> add_action( "use_item,name=cosmic_aspirants_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cds -> add_action( "use_item,name=unchained_gladiators_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cds -> add_action( "use_item,name=unchained_aspirants_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cds -> add_action( "use_item,name=sinful_gladiators_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cds -> add_action( "use_item,name=sinful_aspirants_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );

  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) && special_items.find( items[i].name_str ) == special_items.end() )
    {
      if ( items[i].slot != SLOT_WAIST )
      {
        cds -> add_action( "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up)" );
      }
    }
  }

  cds -> add_action( this, "Avenging Wrath", "if=(holy_power>=4&time<5|holy_power>=3&(time>5|runeforge.the_magistrates_judgment)|holy_power>=2&runeforge.vanguards_momentum&talent.final_reckoning|talent.holy_avenger&cooldown.holy_avenger.remains=0)&(!talent.seraphim|!talent.final_reckoning|cooldown.seraphim.remains>0)" );
  cds -> add_talent( this, "Crusade", "if=holy_power>=4&time<5|holy_power>=3&time>5" );
  cds -> add_action( "ashen_hallow" );
  cds -> add_talent( this, "Holy Avenger" , "if=time_to_hpg=0&holy_power<=2&(buff.avenging_wrath.up|talent.crusade&(cooldown.crusade.remains=0|buff.crusade.up)|fight_remains<20)" );
  cds -> add_talent( this, "Final Reckoning", "if=(holy_power>=4&time<8|holy_power>=3&(time>=8|spell_targets.divine_storm>=2&covenant.kyrian))&cooldown.avenging_wrath.remains>gcd&time_to_hpg=0&(!talent.seraphim|buff.seraphim.up)&(!raid_event.adds.exists|raid_event.adds.up|raid_event.adds.in>40)&(!buff.avenging_wrath.up|holy_power=5|cooldown.hammer_of_wrath.remains|spell_targets.divine_storm>=2&covenant.kyrian)" );

  es_fr_active -> add_action( "fireblood" );
  es_fr_active -> add_action( "call_action_list,name=finishers,if=holy_power=5|debuff.judgment.up|debuff.final_reckoning.up&(debuff.final_reckoning.remains<gcd.max|spell_targets.divine_storm>=2&!talent.execution_sentence)|debuff.execution_sentence.up&debuff.execution_sentence.remains<gcd.max" );
  es_fr_active -> add_action( "divine_toll,if=holy_power<=2" );
  es_fr_active -> add_action( "vanquishers_hammer" );
  es_fr_active -> add_action( this, "Wake of Ashes", "if=holy_power<=2&(debuff.final_reckoning.up&debuff.final_reckoning.remains<gcd*2&!runeforge.divine_resonance|debuff.execution_sentence.up&debuff.execution_sentence.remains<gcd|spell_targets.divine_storm>=5&runeforge.divine_resonance&talent.execution_sentence)" );
  es_fr_active -> add_action( this, "Blade of Justice", "if=conduit.expurgation&(!runeforge.divine_resonance&holy_power<=3|holy_power<=2)" );
  es_fr_active -> add_action( this, "Judgment", "if=!debuff.judgment.up&(holy_power>=1&runeforge.the_magistrates_judgment|holy_power>=2)" );
  es_fr_active -> add_action( "call_action_list,name=finishers" );
  es_fr_active -> add_action( this, "Wake of Ashes", "if=holy_power<=2" );
  es_fr_active -> add_action( this, "Blade of Justice", "if=holy_power<=3" );
  es_fr_active -> add_action( this, "Judgment", "if=!debuff.judgment.up" );
  es_fr_active -> add_action( this, "Hammer of Wrath" );
  es_fr_active -> add_action( this, "Crusader Strike" );
  es_fr_active -> add_action( "arcane_torrent" );
  es_fr_active -> add_action( this, "Consecration" );

  es_fr_pooling -> add_talent( this, "Seraphim", "if=holy_power=5&(!talent.final_reckoning|cooldown.final_reckoning.remains<=gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains<=gcd*3|talent.final_reckoning)&(!covenant.kyrian|cooldown.divine_toll.remains<9)" );
  es_fr_pooling -> add_action( "call_action_list,name=finishers,if=holy_power=5|debuff.final_reckoning.up|buff.crusade.up&buff.crusade.stack<10" );
  es_fr_pooling -> add_action( "vanquishers_hammer,if=buff.seraphim.up" );
  es_fr_pooling -> add_action( this, "Hammer of Wrath", "if=runeforge.vanguards_momentum" );
  es_fr_pooling -> add_action( this, "Wake of Ashes", "if=holy_power<=2&set_bonus.tier28_4pc" );
  es_fr_pooling -> add_action( this, "Blade of Justice", "if=holy_power<=3" );
  es_fr_pooling -> add_action( this, "Judgment", "if=!debuff.judgment.up" );
  es_fr_pooling -> add_action( this, "Hammer of Wrath" );
  es_fr_pooling -> add_action( this, "Crusader Strike", "if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2)" );
  es_fr_pooling -> add_talent( this, "Seraphim", "if=!talent.final_reckoning&cooldown.execution_sentence.remains<=gcd*3&(!covenant.kyrian|cooldown.divine_toll.remains<9)" );
  es_fr_pooling -> add_action( "call_action_list,name=finishers" );
  es_fr_pooling -> add_action( this, "Crusader Strike" );
  es_fr_pooling -> add_action( "arcane_torrent,if=holy_power<=4" );
  es_fr_pooling -> add_talent( this, "Seraphim", "if=(!talent.final_reckoning|cooldown.final_reckoning.remains<=gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains<=gcd*3|talent.final_reckoning)&(!covenant.kyrian|cooldown.divine_toll.remains<9)" );
  es_fr_pooling -> add_action( this, "Consecration" );

  finishers -> add_action( "variable,name=ds_castable,value=spell_targets.divine_storm=2&!(runeforge.final_verdict|talent.righteous_verdict)|spell_targets.divine_storm>2|buff.empyrean_power.up&!debuff.judgment.up&!buff.divine_purpose.up" );
  finishers -> add_talent( this, "Seraphim", "if=(cooldown.avenging_wrath.remains>15|cooldown.crusade.remains>15)&!talent.final_reckoning&(!talent.execution_sentence|spell_targets.divine_storm>=5)&(!raid_event.adds.exists|raid_event.adds.in>40|raid_event.adds.in<gcd|raid_event.adds.up)&(!covenant.kyrian|cooldown.divine_toll.remains<9)|fight_remains<15&fight_remains>5|buff.crusade.up&buff.crusade.stack<10" );
  finishers -> add_talent( this, "Execution Sentence", "if=(buff.crusade.down&cooldown.crusade.remains>10|buff.crusade.stack>=3|cooldown.avenging_wrath.remains>10)&(!talent.final_reckoning|cooldown.final_reckoning.remains>10)&target.time_to_die>8&spell_targets.divine_storm<5" );
  finishers -> add_action( this, "Divine Storm", "if=variable.ds_castable&!buff.vanquishers_hammer.up&((!talent.crusade|cooldown.crusade.remains>gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains>gcd*6|cooldown.execution_sentence.remains>gcd*4&holy_power>=4|target.time_to_die<8|spell_targets.divine_storm>=5|!talent.seraphim&cooldown.execution_sentence.remains>gcd*2)&(!talent.final_reckoning|cooldown.final_reckoning.remains>gcd*6|cooldown.final_reckoning.remains>gcd*4&holy_power>=4|!talent.seraphim&cooldown.final_reckoning.remains>gcd*2)|talent.holy_avenger&cooldown.holy_avenger.remains<gcd*3|buff.holy_avenger.up|buff.crusade.up&buff.crusade.stack<10)" );
  finishers -> add_action( this, "Templar's Verdict", "if=(!talent.crusade|cooldown.crusade.remains>gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains>gcd*6|cooldown.execution_sentence.remains>gcd*4&holy_power>=4|target.time_to_die<8|!talent.seraphim&cooldown.execution_sentence.remains>gcd*2)&(!talent.final_reckoning|cooldown.final_reckoning.remains>gcd*6|cooldown.final_reckoning.remains>gcd*4&holy_power>=4|!talent.seraphim&cooldown.final_reckoning.remains>gcd*2)|talent.holy_avenger&cooldown.holy_avenger.remains<gcd*3|buff.holy_avenger.up|buff.crusade.up&buff.crusade.stack<10" );

  generators -> add_action( "call_action_list,name=finishers,if=holy_power=5|(debuff.judgment.up|holy_power=4)&buff.divine_resonance.up|buff.holy_avenger.up" );
  generators -> add_action( "vanquishers_hammer,if=!runeforge.dutybound_gavel|!talent.final_reckoning&!talent.execution_sentence|fight_remains<8" );
  generators -> add_action( this, "Hammer of Wrath", "if=runeforge.the_mad_paragon|covenant.venthyr&cooldown.ashen_hallow.remains>210" );
  generators -> add_action( this, "Wake of Ashes", "if=holy_power<=2&set_bonus.tier28_4pc&(cooldown.avenging_wrath.remains|cooldown.crusade.remains)" );
  generators -> add_action( "divine_toll,if=holy_power<=2&!debuff.judgment.up&(!talent.seraphim|buff.seraphim.up)&(!raid_event.adds.exists|raid_event.adds.in>30|raid_event.adds.up)&!talent.final_reckoning&(!talent.execution_sentence|fight_remains<8|spell_targets.divine_storm>=5)&(cooldown.avenging_wrath.remains>15|cooldown.crusade.remains>15|fight_remains<8)" );
  generators -> add_action( this, "Judgment", "if=!debuff.judgment.up&(holy_power>=1&runeforge.the_magistrates_judgment|holy_power>=2)" );
  generators -> add_action( this, "Wake of Ashes", "if=(holy_power=0|holy_power<=2&cooldown.blade_of_justice.remains>gcd*2)&(!raid_event.adds.exists|raid_event.adds.in>20|raid_event.adds.up)&(!talent.seraphim|cooldown.seraphim.remains>5|covenant.kyrian)&(!talent.execution_sentence|cooldown.execution_sentence.remains>15|target.time_to_die<8|spell_targets.divine_storm>=5)&(!talent.final_reckoning|cooldown.final_reckoning.remains>15|fight_remains<8)&(cooldown.avenging_wrath.remains|cooldown.crusade.remains)" );
  generators -> add_action( "call_action_list,name=finishers,if=holy_power>=3&buff.crusade.up&buff.crusade.stack<10" );
  generators -> add_action( this, "Blade of Justice", "if=conduit.expurgation&holy_power<=3" );
  generators -> add_action( this, "Judgment", "if=!debuff.judgment.up" );
  generators -> add_action( this, "Hammer of Wrath" );
  generators -> add_action( this, "Blade of Justice", "if=holy_power<=3" );
  generators -> add_action( "call_action_list,name=finishers,if=(target.health.pct<=20|buff.avenging_wrath.up|buff.crusade.up|buff.empyrean_power.up)" );
  generators -> add_action( this, "Consecration", "if=!consecration.up&spell_targets.divine_storm>=2" );
  generators -> add_action( this, "Crusader Strike", "if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2)" );
  generators -> add_action( "call_action_list,name=finishers" );
  generators -> add_action( this, "Consecration", "if=!consecration.up" );
  generators -> add_action( this, "Crusader Strike" );
  generators -> add_action( "arcane_torrent" );
  generators -> add_action( this, "Consecration" );
}

} // end namespace paladin

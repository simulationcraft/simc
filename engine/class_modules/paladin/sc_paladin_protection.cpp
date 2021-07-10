#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

// TODO :
// Defensive stuff :
// - correctly update sotr's armor bonus on each cast
// - avenger's valor defensive benefit

namespace paladin {

// Ardent Defender (Protection) ===============================================

struct ardent_defender_t : public paladin_spell_t
{
  ardent_defender_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "ardent_defender", p, p -> find_specialization_spell( "Ardent Defender" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = 0_ms;

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.ardent_defender -> trigger();
  }
};

// Avengers Shield ==========================================================

struct avengers_shield_base_t : public paladin_spell_t
{
  avengers_shield_base_t( util::string_view n, paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( n, p, p -> find_specialization_spell( "Avenger's Shield" ) )
  {
    parse_options( options_str );
    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }
    may_crit = true;

    aoe = data().effectN( 1 ).chain_target();
    if ( p -> azerite.soaring_shield.enabled() )
    {
      aoe = as<int>( p -> azerite.soaring_shield.spell() -> effectN( 2 ).base_value() );
    }

    // First Avenger hits +2 targets
    aoe += as<int>( p -> talents.first_avenger -> effectN( 1 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( p() -> azerite.soaring_shield.enabled() )
    {
      p() -> buffs.soaring_shield -> trigger();
    }

    // First Avenger absorb shield. Amount is additive per hit.
    if ( p() -> talents.first_avenger -> ok() )
    {
      //First Avenger is capped at 30% of max hp. Hardcoded here.
      double max_absorb = 0.3 * p() -> resources.max[ RESOURCE_HEALTH ];

      double new_absorb = s -> result_amount * p() -> talents.first_avenger -> effectN( 2 ).percent();
      if( p() -> buffs.first_avenger_absorb -> value() + new_absorb < max_absorb )
        p() -> buffs.first_avenger_absorb -> trigger( 1, p() -> buffs.first_avenger_absorb -> value() + new_absorb );
      else
        p() -> buffs.first_avenger_absorb -> trigger( 1, max_absorb );
    }

    if ( p() -> conduit.vengeful_shock -> ok() )
    {
      td( s -> target ) -> debuff.vengeful_shock -> trigger();
    }

    if ( p() -> legendary.bulwark_of_righteous_fury -> ok() )
      p() -> buffs.bulwark_of_righteous_fury -> trigger();
  }
};

struct avengers_shield_dt_t : public avengers_shield_base_t
{
  avengers_shield_dt_t( paladin_t* p ) :
    avengers_shield_base_t( "avengers_shield_divine_toll", p, "" )
  {
    background = true;
  }
};

struct avengers_shield_dr_t : public avengers_shield_base_t
{
  avengers_shield_dr_t( paladin_t* p ):
    avengers_shield_base_t( "avengers_shield_divine_resonance", p, "" )
  {
    background = true;
  }

  double action_multiplier() const override
  {
    // 2021-06-26 Divine resonance damage is increased by Moment of Glory, but
    // does not consume the buff.
    double m = avengers_shield_base_t::action_multiplier();
    if( p() -> bugs && p() -> buffs.moment_of_glory -> up() )
      m *= 1.0 + p() -> buffs.moment_of_glory -> value();
    return m;
  }
};

struct avengers_shield_t : public avengers_shield_base_t
{
  avengers_shield_t( paladin_t* p, util::string_view options_str ) :
    avengers_shield_base_t( "avengers_shield", p, options_str )
  {
    cooldown = p -> cooldowns.avengers_shield;
  }

  void execute() override
  {
    avengers_shield_base_t::execute();

    bool wasted_reset = false;

    // Moment of Glory (MoG) will be consumed regardless of Holy Avengers Engraved Sigil (HAES) proc
    // If they happen at the same time, HAES proc will be considered wasted over MoG as its cost is higher
    // Assuming Legendary > Talents > Baseline
    // Feel free to swap around these parts if the stance on the above changes
    if ( p() -> buffs.moment_of_glory -> up() )
    {
      if ( ! wasted_reset )
      {
        p() -> cooldowns.avengers_shield -> reset( false );
        p() -> procs.as_moment_of_glory -> occur();
        wasted_reset = true;
      }
      else
        p() -> procs.as_moment_of_glory_wasted -> occur();

      p() -> buffs.moment_of_glory -> decrement( 1 );
    }

    if ( p() -> legendary.holy_avengers_engraved_sigil -> ok() &&
      rng().roll( p() -> legendary.holy_avengers_engraved_sigil -> proc_chance() ) )
    {
      if ( ! wasted_reset )
      {
        // With 35% chance to proc, the average skill player can expect a proc to happen
        p() -> cooldowns.avengers_shield -> reset( false );
        p() -> procs.as_engraved_sigil -> occur();
        wasted_reset = true;
      }
      else
        p() -> procs.as_engraved_sigil_wasted -> occur();
    }
  }

  double action_multiplier() const override
  {
    double m = avengers_shield_base_t::action_multiplier();
    if( p() -> buffs.moment_of_glory -> up() )
      m *= 1.0 + p() -> buffs.moment_of_glory -> value();

    return m;
  }
};

// Moment of Glory ============================================================

struct moment_of_glory_t : public paladin_spell_t
{
  moment_of_glory_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "moment_of_glory", p, p -> talents.moment_of_glory )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    paladin_spell_t::execute();
    // You reset the cooldown of AS, get 3 no cooldown 20% damage increase ASs,
    // then when you use the last stack you get another (4th) AS without a cooldown
    // but it doesn't get the damage increase. Doesn't interact with Divine Toll.
    if ( ! p() -> cooldowns.avengers_shield -> is_ready() )
    {
      p() -> procs.as_moment_of_glory -> occur();
      p() -> cooldowns.avengers_shield -> reset( false );
    }
    else
      p() -> procs.as_moment_of_glory_wasted -> occur();

    p() -> buffs.moment_of_glory -> trigger( data().initial_stacks() );
  }
};

// Blessed Hammer (Protection) ================================================

struct blessed_hammer_tick_t : public paladin_spell_t
{
  blessed_hammer_tick_t( paladin_t* p ) :
    paladin_spell_t( "blessed_hammer_tick", p, p -> find_spell( 204301 ) )
  {
    aoe = -1;
    background = dual = direct_tick = true;
    callbacks = false;
    radius = 9.0; // Guess, must be > 8 (cons) but < 10 (HoJ)
    may_crit = true;
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );
    // apply BH debuff to target_data structure
    // Does not interact with vers.
    // To Do: Investigate refresh behaviour
    td( s -> target ) -> debuff.blessed_hammer -> trigger(
      1,
      s -> attack_power * p() -> talents.blessed_hammer -> effectN( 1 ).percent()
    );
  }
};

struct blessed_hammer_t : public paladin_spell_t
{
  blessed_hammer_tick_t* hammer;
  double num_strikes;

  blessed_hammer_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "blessed_hammer", p, p -> talents.blessed_hammer ),
    hammer( new blessed_hammer_tick_t( p ) ), num_strikes( 2 )
  {
    add_option( opt_float( "strikes", num_strikes) );
    parse_options( options_str );

    // Sanity check for num_strikes
    if ( num_strikes <= 0 || num_strikes > 10)
    {
      num_strikes = 2;
      sim -> error( "{} invalid blessed_hammer strikes, value changed to 2", p -> name() );
    }

    dot_duration = 0_ms; // The periodic event is handled by ground_aoe_event_t
    base_tick_time = 0_ms;

    may_miss = false;
    cooldown -> hasted = true;

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
          .target( execute_state -> target )
          // spawn at feet of player
          .x( execute_state -> action -> player -> x_position )
          .y( execute_state -> action -> player -> y_position )
          .pulse_time( data().duration()/roll_strikes )
          .n_pulses( roll_strikes )
          .start_time( sim -> current_time() + initial_delay )
          .action( hammer ), true );

    // Grand Crusader can proc on cast, but not on impact
    p() -> trigger_grand_crusader();
  }
};

// Blessing of Spellwarding =====================================================

struct blessing_of_spellwarding_t : public paladin_spell_t
{
  blessing_of_spellwarding_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "blessing_of_spellwarding", p, p -> talents.blessing_of_spellwarding )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p() -> trigger_forbearance( execute_state -> target );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::target_ready( candidate_target );
  }
};

// Guardian of Ancient Kings ============================================

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "guardian_of_ancient_kings", p, p -> find_specialization_spell( "Guardian of Ancient Kings" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
    trigger_gcd = 0_ms;
    cooldown = p -> cooldowns.guardian_of_ancient_kings;

    if ( p -> conduit.royal_decree -> ok() )
      cooldown -> duration += p -> conduit.royal_decree.time_value();
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.guardian_of_ancient_kings -> trigger();
  }
};

// Hammer of the Righteous ==================================================

struct hammer_of_the_righteous_aoe_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t( paladin_t* p ) :
    paladin_melee_attack_t( "hammer_of_the_righteous_aoe", p, p -> find_spell( 88263 ) )
  {
    // AoE effect always hits if single-target attack succeeds
    // Doesn't proc Grand Crusader
    may_dodge = may_parry = may_miss = false;
    background = true;
    aoe        = -1;
    trigger_gcd = 0_ms; // doesn't incur GCD (HotR does that already)
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    paladin_melee_attack_t::available_targets( tl );

    for ( size_t i = 0; i < tl.size(); i++ )
    {
      if ( tl[i] == target ) // Cannot hit the original target.
      {
        tl.erase( tl.begin() + i );
        break;
      }
    }
    return tl.size();
  }
};

struct hammer_of_the_righteous_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t* hotr_aoe;
  hammer_of_the_righteous_t( paladin_t* p, util::string_view options_str ) :
    paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ) )
  {
    parse_options( options_str );

    if ( p -> talents.blessed_hammer -> ok() )
      background = true;

    hotr_aoe = new hammer_of_the_righteous_aoe_t( p );
    // Attach AoE proc as a child
    add_child( hotr_aoe );
    if ( p -> spec.hammer_of_the_righteous_2 -> ok() && ! p -> talents.blessed_hammer -> ok() )
      cooldown -> charges += as<int>( p -> spec.hammer_of_the_righteous_2 -> effectN( 1 ).base_value() );
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    // Special things that happen when HotR connects
    if ( result_is_hit( execute_state -> result ) )
    {
      // Grand Crusader
      p() -> trigger_grand_crusader();

      if ( hotr_aoe -> target != execute_state -> target )
        hotr_aoe -> target_cache.is_valid = false;

      if ( p() -> standing_in_consecration() )
      {
        hotr_aoe -> target = execute_state -> target;
        hotr_aoe -> execute();
      }
    }
  }
};

// Judgment - Protection =================================================================

// TODO(mserrano): fix judgment
struct judgment_prot_t : public judgment_t
{
  int judge_holy_power, sw_holy_power;
  judgment_prot_t( paladin_t* p, util::string_view options_str ) :
    judgment_t( p, options_str ),
    judge_holy_power( as<int>( p -> find_spell( 220637 ) -> effectN( 1 ).base_value() ) ),
    sw_holy_power( as<int>( p -> talents.prot_sanctified_wrath -> effectN( 2 ).base_value() ) )
  {
    cooldown -> charges += as<int>( p -> talents.crusaders_judgment -> effectN( 1 ).base_value() );
  }

  judgment_prot_t( paladin_t* p ) :
    judgment_t( p ),
    judge_holy_power( as<int>( p -> find_spell( 220637 ) -> effectN( 1 ).base_value() ) ),
    sw_holy_power( as<int>( p -> talents.prot_sanctified_wrath -> effectN( 2 ).base_value() ) )
  {
    // this is for divine resonance
    background = true;
  }

  // Special things that happen when Judgment damages target
  void impact( action_state_t* s ) override
  {
    judgment_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> spec.judgment_4 -> ok() )
        td( s -> target ) -> debuff.judgment -> trigger();

      int hopo = 0;
      if ( p() -> spec.judgment_3 -> ok() )
        hopo += judge_holy_power;
      if ( p() -> talents.prot_sanctified_wrath -> ok() && p() -> buffs.avenging_wrath -> up() )
        hopo += sw_holy_power;
      if( hopo > 0 )
        p() -> resource_gain( RESOURCE_HOLY_POWER, hopo, p() -> gains.judgment );
    }
  }
};

// Shield of the Righteous ==================================================

shield_of_the_righteous_buff_t::shield_of_the_righteous_buff_t( paladin_t* p ) :
  buff_t( p, "shield_of_the_righteous", p -> spells.sotr_buff )
{
  add_invalidate( CACHE_BONUS_ARMOR );
  set_default_value( p -> spells.sotr_buff -> effectN( 1 ).percent() );
  set_refresh_behavior( buff_refresh_behavior::EXTEND );
  cooldown -> duration = 0_ms; // handled by the ability
}

void shield_of_the_righteous_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  buff_t::expire_override( expiration_stacks, remaining_duration );

  paladin_t* p = debug_cast< paladin_t* >( player );

  if ( p -> azerite.inner_light.enabled() )
  {
    p -> buffs.inner_light -> trigger();
  }
}

struct shield_of_the_righteous_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  shield_of_the_righteous_t( paladin_t* p, util::string_view options_str ) :
    holy_power_consumer_t( "shield_of_the_righteous", p, p -> spec.shield_of_the_righteous )
  {
    parse_options( options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }

    aoe = -1;

    // no weapon multiplier
    weapon_multiplier = 0.0;
  }

  shield_of_the_righteous_t( paladin_t* p ) :
    holy_power_consumer_t( "shield_of_the_righteous_vanquishers_hammer", p, p -> spec.shield_of_the_righteous )
  {
    // This is the "free" SotR from vanq hammer. Identifiable by being background.
    background = true;
    aoe = -1;
    trigger_gcd = 0_ms;
    weapon_multiplier = 0.0;
  }

  void execute() override
  {
    holy_power_consumer_t::execute();

    // Buff granted regardless of combat roll result
    // Duration and armor bonus recalculation handled in the buff
    p() -> buffs.shield_of_the_righteous -> trigger();

    if ( p() -> talents.redoubt -> ok() )
    {
      p() -> buffs.redoubt -> trigger();
    }

    if ( p() -> azerite_essence.memory_of_lucid_dreams.enabled() )
    {
      p() -> trigger_memory_of_lucid_dreams( 1.0 );
    }

    // As of 2020-11-07 Resolute Defender now always provides its CDR.
    if ( p() -> conduit.resolute_defender -> ok() )
    {
      p() -> cooldowns.ardent_defender -> adjust( -1.0_s * p() -> conduit.resolute_defender.value() );
      if ( p() -> buffs.ardent_defender -> up() )
        p() -> buffs.ardent_defender -> extend_duration( p(),
          p() -> conduit.resolute_defender -> effectN( 2 ).percent() * p() -> buffs.ardent_defender -> buff_duration()
        );
    }

    if ( !background )
    {
      if ( p() -> buffs.shining_light_free -> up() || p() -> buffs.shining_light_stacks -> at_max_stacks() )
      {
        p() -> buffs.shining_light_stacks -> expire();
        p() -> buffs.shining_light_free -> trigger();
      }
      else
        p() -> buffs.shining_light_stacks -> trigger();
    }

    p() -> buffs.bulwark_of_righteous_fury -> expire();
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = holy_power_consumer_t::recharge_multiplier( cd );

    if ( p() -> player_t::buffs.memory_of_lucid_dreams -> check() )
    {
      rm /= 1.0 + p() -> player_t::buffs.memory_of_lucid_dreams -> data().effectN( 1 ).percent();
    }

    return rm;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = holy_power_consumer_t::composite_target_multiplier( t );
    if ( td( t ) -> debuff.judgment -> up() && p() -> conduit.punish_the_guilty -> ok() )
      ctm *= 1.0 + p() -> conduit.punish_the_guilty.percent();
    return ctm;
  }

  double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();
    // Range increase on bulwark of righteous fury not implemented.
    am *= 1.0 + p() -> buffs.bulwark_of_righteous_fury -> stack_value();
    return am;
  }
};


// paladin_t::target_mitigation ===============================================

void paladin_t::target_mitigation( school_e school,
                                   result_amount_type    dt,
                                   action_state_t* s )
{
  player_t::target_mitigation( school, dt, s );

  // various mitigation effects, Ardent Defender goes last due to absorb/heal mechanics

  // Passive sources (Sanctuary)
  s -> result_amount *= 1.0 + passives.sanctuary -> effectN( 1 ).percent();

  if ( passives.aegis_of_light_2 -> ok() )
    s -> result_amount *= 1.0 + passives.aegis_of_light_2 -> effectN( 1 ).percent();

  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> print_debug( "Damage to {} after passive mitigation is {}", s -> target -> name(), s -> result_amount );

  // Damage Reduction Cooldowns

  // Guardian of Ancient Kings
  if ( buffs.guardian_of_ancient_kings -> up() )
  {
    s -> result_amount *= 1.0 + buffs.guardian_of_ancient_kings -> data().effectN( 3 ).percent();
  }

  // Divine Protection
  if ( buffs.divine_protection -> up() )
  {
    s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 1 ).percent();
  }

  if ( buffs.ardent_defender -> up() )
  {
    s -> result_amount *= 1.0 + buffs.ardent_defender -> data().effectN( 1 ).percent()
      + legendary.the_ardent_protectors_sanctum -> effectN( 1 ).percent();
  }

  if ( buffs.blessing_of_dusk -> up() )
  {
    s -> result_amount *= 1.0 + buffs.blessing_of_dusk -> value();
  }

  if ( buffs.devotion_aura -> up() )
    s -> result_amount *= 1.0 + buffs.devotion_aura -> value();

  // Divine Bulwark and consecration reduction
  if ( standing_in_consecration() && specialization() == PALADIN_PROTECTION )
  {
    s -> result_amount *= 1.0 + spec.consecration_2 -> effectN( 1 ).percent()
      + cache.mastery() * mastery.divine_bulwark_2 -> effectN( 1 ).mastery_value();
  }

  // Blessed Hammer
  if ( talents.blessed_hammer -> ok() && s -> action )
  {
    buff_t* b = get_target_data( s -> action -> player ) -> debuff.blessed_hammer;

    // BH now reduces all damage; not just melees.
    if ( b -> up() )
    {
      // Absorb value collected from (de)buff value
      // Calculate actual amount absorbed.
      double amount_absorbed = std::min( s->result_amount, b -> value() );
      b -> expire();

      sim -> print_debug( "{} Blessed Hammer absorbs {} out of {} damage",
        name(),
        amount_absorbed,
        s -> result_amount
      );

      // update the relevant counters
      iteration_absorb_taken += amount_absorbed;
      s -> self_absorb_amount += amount_absorbed;
      s -> result_amount -= amount_absorbed;
      s -> result_absorbed = s -> result_amount;

      // hack to register this on the abilities table
      buffs.blessed_hammer_absorb -> trigger( 1, amount_absorbed );
      buffs.blessed_hammer_absorb -> consume( amount_absorbed );
    }
  }

  // Other stuff
  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> print_debug( "Damage to {} after mitigation effects is {}", s -> target -> name(), s -> result_amount );


  // Ardent Defender
  if ( buffs.ardent_defender -> check() )
  {
    if ( s -> result_amount > 0 && s -> result_amount >= resources.current[ RESOURCE_HEALTH ] )
    {
      // Ardent defender is a little odd - it doesn't heal you *for* 20%, it heals you *to* 12%.
      // It does this by either absorbing all damage and healing you for the difference between 20% and your current health (if current < 20%)
      // or absorbing any damage that would take you below 20% (if current > 20%).
      // To avoid complications with absorb modeling, we're just going to kludge it by adjusting the amount gained or lost accordingly.
      s -> result_amount = 0.0;
      double AD_health_threshold = resources.max[ RESOURCE_HEALTH ] * buffs.ardent_defender -> data().effectN( 2 ).percent();
      if ( resources.current[ RESOURCE_HEALTH ] >= AD_health_threshold )
      {
        resource_loss( RESOURCE_HEALTH,
                       resources.current[ RESOURCE_HEALTH ] - AD_health_threshold,
                       nullptr,
                       s -> action );
      }
      else
      {
        // Ardent Defender, like most cheat death effects, is capped at a 200% max health overkill situation
        resource_gain( RESOURCE_HEALTH,
                       std::min( AD_health_threshold - resources.current[ RESOURCE_HEALTH ], 2 * resources.max[ RESOURCE_HEALTH ] ),
                       nullptr,
                       s -> action );
      }
      buffs.ardent_defender -> expire();
    }

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> print_debug( "Damage to {} after Ardent Defender (death-save) is {}", s -> target -> name(), s -> result_amount );
  }
}

void paladin_t::trigger_grand_crusader()
{
  // escape if we don't have Grand Crusader
  if ( ! passives.grand_crusader -> ok() )
    return;

  double gc_proc_chance = passives.grand_crusader -> proc_chance();
  if ( azerite.inspiring_vanguard.enabled() )
  {
    gc_proc_chance = azerite.inspiring_vanguard.spell() -> effectN( 2 ).percent();
  }

  // The bonus from First Avenger is added after Inspiring Vanguard
  bool success = rng().roll( gc_proc_chance );
  if ( ! success )
    return;

  // reset AS cooldown and count procs
  if ( ! cooldowns.avengers_shield -> is_ready() )
  {
    procs.as_grand_crusader -> occur();
    cooldowns.avengers_shield -> reset( true );
  }
  else
    procs.as_grand_crusader_wasted -> occur();

  if ( talents.crusaders_judgment -> ok() && cooldowns.judgment -> current_charge < cooldowns.judgment -> charges )
  {
    cooldowns.judgment -> adjust( -( cooldowns.judgment -> duration ), true ); //decrease remaining time by the duration of one charge, i.e., add one charge
  }

  if ( azerite.inspiring_vanguard.enabled() )
  {
    buffs.inspiring_vanguard -> trigger();
  }
}

void paladin_t::trigger_holy_shield( action_state_t* s )
{
  // escape if we don't have Holy Shield
  if ( ! talents.holy_shield -> ok() )
    return;

  // sanity check - no friendly-fire
  if ( ! s -> action -> player -> is_enemy() )
    return;

  active.holy_shield_damage -> set_target( s -> action -> player );
  active.holy_shield_damage -> schedule_execute();
}

bool paladin_t::standing_in_consecration() const
{
  if ( ! sim -> distance_targeting_enabled )
  {
    return active_consecration != nullptr;
  }

  // new
  if ( active_consecration != nullptr )
  {
    // calculate current distance to each consecration
    ground_aoe_event_t* cons_to_test = active_consecration;

    double distance = get_position_distance( cons_to_test -> params -> x(), cons_to_test -> params -> y() );

    // exit with true if we're in range of any one Cons center
    if ( distance <= find_spell( 81297 ) -> effectN( 1 ).radius() )
      return true;
  }
  // if we're not in range of any of them
  return false;
}

// Initialization
void paladin_t::create_prot_actions()
{
  active.divine_toll = new avengers_shield_dt_t( this );
  active.necrolord_shield_of_the_righteous = new shield_of_the_righteous_t( this );

  if ( specialization() == PALADIN_PROTECTION )
    active.judgment = new judgment_prot_t( this );
    active.divine_resonance = new avengers_shield_dr_t( this );
}

action_t* paladin_t::create_action_protection( util::string_view name, const std::string& options_str )
{
  if ( name == "ardent_defender"           ) return new ardent_defender_t          ( this, options_str );
  if ( name == "avengers_shield"           ) return new avengers_shield_t          ( this, options_str );
  if ( name == "blessed_hammer"            ) return new blessed_hammer_t           ( this, options_str );
  if ( name == "blessing_of_spellwarding"  ) return new blessing_of_spellwarding_t ( this, options_str );
  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );
  if ( name == "hammer_of_the_righteous"   ) return new hammer_of_the_righteous_t  ( this, options_str );
  if ( name == "shield_of_the_righteous"   ) return new shield_of_the_righteous_t  ( this, options_str );
  if ( name == "moment_of_glory"           ) return new moment_of_glory_t          ( this, options_str );

  if ( specialization() == PALADIN_PROTECTION )
  {
    if ( name == "judgment" ) return new judgment_prot_t( this, options_str );
  }

  return nullptr;
}

void paladin_t::create_buffs_protection()
{
  buffs.ardent_defender = make_buff( this, "ardent_defender", find_specialization_spell( "Ardent Defender" ) )
        -> set_cooldown( 0_ms ); // handled by the ability
  buffs.guardian_of_ancient_kings = make_buff( this, "guardian_of_ancient_kings", find_specialization_spell( "Guardian of Ancient Kings" ) )
        -> set_cooldown( 0_ms )
        -> set_stack_change_callback( [ this ] ( buff_t*, int /*old*/, int curr )
        {
          if ( curr == 1 && conduit.royal_decree -> ok() )
            this -> buffs.royal_decree -> trigger();
        } );

//HS and BH fake absorbs
  buffs.holy_shield_absorb = make_buff<absorb_buff_t>( this, "holy_shield", talents.holy_shield );
  buffs.holy_shield_absorb -> set_absorb_school( SCHOOL_MAGIC )
        -> set_absorb_source( get_stats( "holy_shield_absorb" ) )
        -> set_absorb_gain( get_gain( "holy_shield_absorb" ) );
  buffs.blessed_hammer_absorb = make_buff<absorb_buff_t>( this, "blessed_hammer_absorb", find_spell( 204301 ) );
  buffs.blessed_hammer_absorb -> set_absorb_source( get_stats( "blessed_hammer_absorb" ) )
        -> set_absorb_gain( get_gain( "blessed_hammer_absorb" ) );
  buffs.first_avenger_absorb = make_buff<absorb_buff_t>( this, "first_avenger", find_spell( 327225 ) )
        -> set_absorb_source( get_stats( "first_avenger_absorb" ) );
  buffs.redoubt = make_buff( this, "redoubt", talents.redoubt -> effectN( 1 ).trigger() )
        -> set_default_value( talents.redoubt -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
        -> add_invalidate( CACHE_STRENGTH )
        -> add_invalidate( CACHE_STAMINA );
  buffs.shield_of_the_righteous = new shield_of_the_righteous_buff_t( this );
  buffs.moment_of_glory = make_buff( this, "moment_of_glory", talents.moment_of_glory )
        -> set_default_value( talents.moment_of_glory -> effectN( 2 ).percent() );
  buffs.bulwark_of_righteous_fury = make_buff( this, "bulwark_of_righteous_fury", find_spell( 337848) )
        -> set_default_value( find_spell( 337848 ) -> effectN( 1 ).percent() );
  buffs.shielding_words = make_buff<absorb_buff_t>( this, "shielding_words", conduit.shielding_words )
        -> set_absorb_source( get_stats( "shielding_words" ) );
  buffs.shining_light_stacks = make_buff( this, "shining_light_stacks", find_spell( 182104 ) )
  // Kind of lazy way to make sure that SL only triggers for prot. That spelldata doesn't have to be used anywhere else so /shrug
    -> set_trigger_spell( find_specialization_spell( "Shining Light" ) );
  buffs.shining_light_free = make_buff( this, "shining_light_free", find_spell( 327510 ) );

  buffs.royal_decree = make_buff( this, "royal_decree", find_spell( 340147 ) );
  buffs.reign_of_ancient_kings = make_buff( this, "reign_of_ancient_kings",
    legendary.reign_of_endless_kings -> effectN( 2 ).trigger() -> effectN( 2 ).trigger() );

  // Azerite traits
  buffs.inner_light = make_buff( this, "inner_light", find_spell( 275481 ) )
        -> set_default_value( azerite.inner_light.value( 1 ) );
  buffs.inspiring_vanguard = make_buff<stat_buff_t>( this, "inspiring_vanguard", azerite.inspiring_vanguard.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
        -> add_stat( STAT_STRENGTH, azerite.inspiring_vanguard.value( 1 ) );
  buffs.soaring_shield = make_buff<stat_buff_t>( this, "soaring_shield", azerite.soaring_shield.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
        -> add_stat( STAT_MASTERY_RATING, azerite.soaring_shield.value( 1 ) );

  if ( specialization() == PALADIN_PROTECTION )
    player_t::buffs.memory_of_lucid_dreams -> set_stack_change_callback( [ this ]( buff_t*, int, int )
    { this -> cooldowns.shield_of_the_righteous -> adjust_recharge_multiplier(); } );
}

void paladin_t::init_spells_protection()
{
  // Talents
  talents.holy_shield                = find_talent_spell( "Holy Shield" );
  talents.redoubt                    = find_talent_spell( "Redoubt" );
  talents.blessed_hammer             = find_talent_spell( "Blessed Hammer" );

  talents.first_avenger              = find_talent_spell( "First Avenger" );
  talents.crusaders_judgment         = find_talent_spell( "Crusader's Judgment" );
  talents.moment_of_glory            = find_talent_spell( "Moment of Glory" );

  talents.blessing_of_spellwarding   = find_talent_spell( "Blessing of Spellwarding" );

  talents.consecrated_ground         = find_talent_spell( "Consecrated Ground" );

  talents.prot_sanctified_wrath      = find_talent_spell( "Sanctified Wrath", PALADIN_PROTECTION );
  talents.righteous_protector        = find_talent_spell( "Righteous Protector" );
  talents.final_stand                = find_talent_spell( "Final Stand" );

  // Spec passives and useful spells
  spec.protection_paladin = find_specialization_spell( "Protection Paladin" );
  mastery.divine_bulwark = find_mastery_spell( PALADIN_PROTECTION );
  mastery.divine_bulwark_2 = find_specialization_spell( "Mastery: Divine Bulwark", "Rank 2" );
  spec.hammer_of_the_righteous_2 = find_rank_spell( "Hammer of the Righteous", "Rank 2" );

  if ( specialization() == PALADIN_PROTECTION )
  {
    spec.consecration_3 = find_rank_spell( "Consecration", "Rank 3" );
    spec.consecration_2 = find_rank_spell( "Consecration", "Rank 2" );
    spec.judgment_3 = find_rank_spell( "Judgment", "Rank 3" );
    spec.judgment_4 = find_rank_spell( "Judgment", "Rank 4" );

    spells.judgment_debuff = find_spell( 197277 );
  }

  spec.shield_of_the_righteous = find_class_spell( "Shield of the Righteous" );
  spells.sotr_buff = find_spell( 132403 );

  passives.grand_crusader      = find_specialization_spell( "Grand Crusader" );
  passives.riposte             = find_specialization_spell( "Riposte" );
  passives.sanctuary           = find_specialization_spell( "Sanctuary" );
  
  passives.aegis_of_light      = find_specialization_spell( "Aegis of Light" );
  passives.aegis_of_light_2    = find_rank_spell( "Aegis of Light", "Rank 2" );

  // Azerite traits
  azerite.inspiring_vanguard = find_azerite_spell( "Inspiring Vanguard" );
  azerite.inner_light        = find_azerite_spell( "Inner Light"        );
  azerite.soaring_shield     = find_azerite_spell( "Soaring Shield"     );
}

void paladin_t::generate_action_prio_list_prot()
{
  ///////////////////////
  // Precombat List
  ///////////////////////

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  precombat -> add_action( "flask" );
  precombat -> add_action( "food" );
  precombat -> add_action( "augmentation" );
  precombat -> add_action( "devotion_aura" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  precombat -> add_action( "potion" );
  precombat -> add_action( this, "Consecration" );
  precombat -> add_action( "lights_judgment" );
  precombat -> add_action( "Ashen Hallow");

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* cds = get_action_priority_list( "cooldowns" );
  action_priority_list_t* std = get_action_priority_list( "standard" );

  def -> add_action( "auto_attack" );

  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "call_action_list,name=standard" );

  cds -> add_action( "fireblood,if=buff.avenging_wrath.up" );
  cds -> add_talent( this, "Seraphim" );
  cds -> add_action( this, "Avenging Wrath" );
  cds -> add_talent( this, "Holy Avenger", "if=buff.avenging_wrath.up|cooldown.avenging_wrath.remains>60" );
  cds -> add_action( "potion,if=buff.avenging_wrath.up" );
  cds -> add_action( "use_items,if=buff.seraphim.up|!talent.seraphim.enabled" );
  cds -> add_talent( this, "Moment of Glory", "if=prev_gcd.1.avengers_shield&cooldown.avengers_shield.remains" );

  std -> add_action( this, "Shield of the Righteous" , "if=debuff.judgment.up" );
  std -> add_action( this, "Shield of the Righteous" , "if=holy_power=5|buff.holy_avenger.up|holy_power=4&talent.sanctified_wrath.enabled&buff.avenging_wrath.up" );
  std -> add_action( this, "Judgment", "target_if=min:debuff.judgment.remains,if=charges=2|!talent.crusaders_judgment.enabled" );
  std -> add_action( this, "Hammer of Wrath" );
  std -> add_action( "blessing_of_the_seasons" );
  std -> add_action( this, "Avenger's Shield" );
  std -> add_action( this, "Judgment", "target_if=min:debuff.judgment.remains" );
  std -> add_action( "vanquishers_hammer" );
  std -> add_action( this, "Consecration", "if=!consecration.up" );
  std -> add_action( "divine_toll" );
  std -> add_talent( this, "Blessed Hammer", "strikes=2.4,if=charges=3" );
  std -> add_action( "ashen_hallow" );
  std -> add_action( this, "Hammer of the Righteous", "if=charges=2" );
  std -> add_action( this, "Word of Glory", "if=buff.vanquishers_hammer.up" );
  std -> add_talent( this, "Blessed Hammer", "strikes=2.4" );
  std -> add_action( this, "Hammer of the Righteous" );
  std -> add_action( "lights_judgment" );
  std -> add_action( "arcane_torrent" );
  std -> add_action( this, "Consecration" );
  std -> add_action( this, "Word of Glory", "if=buff.shining_light_free.up&!covenant.necrolord" );

}
}

#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

// TODO :
// Defensive stuff :
// - correctly update sotr's armor bonus on each cast
// - avenger's valor defensive benefit

namespace paladin {

// Aegis of Light (Protection) ================================================

struct aegis_of_light_t : public paladin_spell_t
{
  aegis_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "aegis_of_light", p, p -> talents.aegis_of_light )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.aegis_of_light -> trigger();
  }
};

// Ardent Defender (Protection) ===============================================

struct ardent_defender_t : public paladin_spell_t
{
  ardent_defender_t( paladin_t* p, const std::string& options_str ) :
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

struct avengers_shield_t : public paladin_spell_t
{
  avengers_shield_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "avengers_shield", p, p -> find_specialization_spell( "Avenger's Shield" ) )
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

    // Redoubt offensive benefit
    aoe += as<int>( p -> talents.redoubt -> effectN( 2 ).base_value() );

    base_multiplier *= 1.0 + p -> talents.first_avenger -> effectN( 1 ).percent();
    // First Avenger only increases primary target damage
    base_aoe_multiplier *= 1.0 / ( 1.0 + p -> talents.first_avenger -> effectN( 1 ).percent() );

    // link needed for trigger_grand_crusader
    cooldown = p -> cooldowns.avengers_shield;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.avengers_valor -> trigger();
    if ( p() -> talents.redoubt -> ok() )
    {
      p() -> buffs.redoubt -> trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( p() -> azerite.soaring_shield.enabled() )
    {
      p() -> buffs.soaring_shield -> trigger();
    }
  }
};

// Bastion of Light ========================================================

struct bastion_of_light_t : public paladin_spell_t
{
  bastion_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "bastion_of_light", p, p -> talents.bastion_of_light )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> cooldowns.shield_of_the_righteous -> reset(false, -1);
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
    td( s -> target ) -> debuff.blessed_hammer -> trigger();
  }
};

struct blessed_hammer_t : public paladin_spell_t
{
  blessed_hammer_tick_t* hammer;
  int num_strikes;

  blessed_hammer_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessed_hammer", p, p -> talents.blessed_hammer ),
    hammer( new blessed_hammer_tick_t( p ) ), num_strikes( 2 )
  {
    add_option( opt_int( "strikes", num_strikes ) );
    parse_options( options_str );

    // Sane bounds for num_strikes - only makes three revolutions, impossible to hit one target more than 3 times.
    // Likewise calling the spell with 0 or negative strikes is sort of pointless.
    if ( num_strikes < 1 )
    {
      num_strikes = 1;
      sim -> error( "{} trying to hit less than one time with blessed_hammer, value changed to 1", p -> name() );
    }
    if ( num_strikes > 3 )
    {
      num_strikes = 3;
      sim -> error( "{} trying to hit more than three times with blessed_hammer, value changed to 3", p -> name() );
    }

    dot_duration = 0_ms; // The periodic event is handled by ground_aoe_event_t
    may_miss = false;
    base_tick_time = 1667_ms; // Rough estimation based on stopwatch testing
    cooldown -> hasted = true;

    tick_may_crit = true;

    add_child( hammer );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    timespan_t initial_delay = num_strikes < 3 ? base_tick_time * 0.25 : 0_ms;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        // spawn at feet of player
        .x( execute_state -> action -> player -> x_position )
        .y( execute_state -> action -> player -> y_position )
        .pulse_time( base_tick_time )
        .duration( base_tick_time * ( num_strikes - 0.5 ) )
        .start_time( sim -> current_time() + initial_delay )
        .action( hammer ), true );

    // Grand Crusader can proc on cast, but not on impact
    p() -> trigger_grand_crusader();
  }
};

// Blessing of Spellwarding =====================================================

struct blessing_of_spellwarding_t : public paladin_spell_t
{
  blessing_of_spellwarding_t( paladin_t* p, const std::string& options_str ) :
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
  guardian_of_ancient_kings_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "guardian_of_ancient_kings", p, p -> find_specialization_spell( "Guardian of Ancient Kings" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
    trigger_gcd = 0_ms;
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
  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ) )
  {
    parse_options( options_str );

    if ( p -> talents.blessed_hammer -> ok() )
      background = true;

    hotr_aoe = new hammer_of_the_righteous_aoe_t( p );
    // Attach AoE proc as a child
    add_child( hotr_aoe );
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

struct judgment_prot_t : public judgment_t
{
  timespan_t sotr_cdr; // needed for sotr interaction for protection

  judgment_prot_t( paladin_t* p, const std::string& options_str ) :
    judgment_t( p, options_str )
  {
    cooldown -> charges += as<int>( p -> talents.crusaders_judgment -> effectN( 1 ).base_value() );
    cooldown -> duration *= 1.0 + p -> spec.protection_paladin -> effectN( 4 ).percent();

    base_multiplier *= 1.0 + p -> spec.protection_paladin -> effectN( 12 ).percent();

    sotr_cdr = -1.0 * timespan_t::from_seconds( p -> spec.judgment_2 -> effectN( 1 ).base_value() );
  }

  void execute() override
  {
    judgment_t::execute();

    if ( p() -> talents.fist_of_justice -> ok() )
    {
      double cdr = -1.0 * p() -> talents.fist_of_justice -> effectN( 2 ).base_value();
      p() -> cooldowns.hammer_of_justice -> adjust( timespan_t::from_seconds( cdr ) );
    }
  }

  // Special things that happen when Judgment damages target
  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      // Judgment hits/crits reduce SotR recharge time
      if ( p() -> sets -> has_set_bonus( PALADIN_PROTECTION, T20, B2 ) &&
           rng().roll( p() -> sets -> set( PALADIN_PROTECTION, T20, B2 ) -> proc_chance() ) )
      {
        p() -> cooldowns.avengers_shield -> reset( true );
      }

      p() -> cooldowns.shield_of_the_righteous -> adjust( s -> result == RESULT_CRIT ? 2.0 * sotr_cdr : sotr_cdr );
    }

    judgment_t::impact( s );
  }
};

// Light of the Protector ===================================================

struct light_of_the_protector_base_t : public paladin_heal_t
{
  light_of_the_protector_base_t( paladin_t* p, const std::string& name, const spell_data_t* spell, const std::string& options_str ) :
    paladin_heal_t( name, p, spell )
  {
    parse_options( options_str );
    may_crit = true;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_heal_t::composite_target_multiplier( t );

    // Heals for a base amount, increased by up to +200% based on the target's missing health
    // Linear increase, each missing health % increases the healing by 2%
    double missing_health_percent = 1.0 -  t -> resources.pct( RESOURCE_HEALTH );

    m *= 1.0 + missing_health_percent * data().effectN( 2 ).percent();

    sim -> print_log( "Player {} missing {:.2f}% health, healing increased by {:.2f}%",
                      t -> name(), missing_health_percent * 100,
                      missing_health_percent * data().effectN( 2 ).percent() * 100 );
    return m;
  }
};

struct light_of_the_protector_t : public light_of_the_protector_base_t
{
  light_of_the_protector_t( paladin_t* p, const std::string& options_str ) :
    light_of_the_protector_base_t( p, "light_of_the_protector", p -> find_specialization_spell( "Light of the Protector" ), options_str )
  {
    target = p;
    // link needed for Righteous Protector / SotR cooldown reduction
    cooldown = p -> cooldowns.light_of_the_protector;
  }

  bool ready() override
  {
    if ( p() -> talents.hand_of_the_protector -> ok() )
      return false;

    return paladin_heal_t::ready();
  }
};

// Hand of the Protector (Protection) =======================================

struct hand_of_the_protector_t : public light_of_the_protector_base_t
{
  hand_of_the_protector_t( paladin_t* p, const std::string& options_str ) :
    light_of_the_protector_base_t( p, "hand_of_the_protector", p -> talents.hand_of_the_protector, options_str )
  {
    // link needed for Righteous Protector / SotR cooldown reduction
    cooldown = p -> cooldowns.hand_of_the_protector;
  }

  bool ready() override
  {
    if ( ! p() -> talents.hand_of_the_protector -> ok() )
      return false;

    return paladin_heal_t::ready();
  }
};

// Seraphim ( Protection ) ==================================================

struct seraphim_t : public paladin_spell_t
{
  seraphim_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "seraphim", p, p -> talents.seraphim )
  {
    parse_options( options_str );

    harmful = false;
    may_miss = false;

    // ugly hack required if SotR isn't found in the APL
    if ( p -> cooldowns.shield_of_the_righteous -> charges < 3 )
    {
      const spell_data_t* sotr = p -> find_class_spell( "Shield of the Righteous" );
      p -> cooldowns.shield_of_the_righteous -> charges = p -> cooldowns.shield_of_the_righteous -> current_charge = sotr -> _charges;
      p -> cooldowns.shield_of_the_righteous -> duration = timespan_t::from_millis( sotr -> _charge_cooldown );
    }
  }

  void execute() override
  {
    paladin_spell_t::execute();

    // duration depends on sotr charges, need to do some math
    timespan_t duration = 0_ms;
    int available_charges = p() -> cooldowns.shield_of_the_righteous -> current_charge;
    double charges_used = 0;
    timespan_t remains = p() -> cooldowns.shield_of_the_righteous -> current_charge_remains();

    if ( available_charges >= 1 )
    {
      charges_used += std::min( available_charges, 2 );
      for ( int i = 0; i < charges_used; i++ )
        p() -> cooldowns.shield_of_the_righteous -> start( p() -> active.sotr );
    }

    if ( charges_used < 2 )
    {
      charges_used += ( p() -> cooldowns.shield_of_the_righteous -> duration.total_seconds() - remains.total_seconds() ) / p() -> cooldowns.shield_of_the_righteous -> duration.total_seconds();
      p() -> cooldowns.shield_of_the_righteous -> adjust( p() -> cooldowns.shield_of_the_righteous -> duration - remains );
    }

    duration = charges_used * p() -> talents.seraphim -> duration();

    if ( duration > 0_ms )
      p() -> buffs.seraphim -> trigger( 1, -1.0, -1.0, duration );

    if ( p() -> azerite_essence.memory_of_lucid_dreams.enabled() && duration > 0_ms )
    {
      p() -> trigger_memory_of_lucid_dreams( charges_used );
    }
  }
};

// Shield of the Righteous ==================================================

shield_of_the_righteous_buff_t::shield_of_the_righteous_buff_t( paladin_t* p ) :
  buff_t( p, "shield_of_the_righteous", p -> spells.sotr_buff ),
  default_av_increase( p -> passives.avengers_valor -> effectN( 2 ).percent() )
{
  avengers_valor_increase = 0;
  add_invalidate( CACHE_BONUS_ARMOR );
  set_default_value( p -> spells.sotr_buff -> effectN( 1 ).percent() );
  set_refresh_behavior( buff_refresh_behavior::EXTEND );
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

// Custom trigger function to (re-)calculate the increase from avenger's valor
bool shield_of_the_righteous_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  // Shield of the righteous' armor bonus is dynamic with strength, but uses a multiplier that is only updated on sotr cast
  // avengers_valor_increase varies between 0 and 0.2 based on avenger's valor being up or not, and on the previous buff's remaining duration
  paladin_t* p = debug_cast< paladin_t* >( player );

  double new_avengers_valor = p -> buffs.avengers_valor -> up() ? default_av_increase : 0;

  if ( this -> up() && new_avengers_valor != avengers_valor_increase )
  {
    avengers_valor_increase = avengers_valor_increase * remains().total_seconds() / ( remains().total_seconds() + buff_duration.total_seconds() )
      + new_avengers_valor * buff_duration.total_seconds() / ( remains().total_seconds() + buff_duration.total_seconds() );
  }
  else
  {
    avengers_valor_increase = new_avengers_valor;
  }

  sim -> print_debug( "Shield of the Righteous buff triggered with a {} increase from Avenger's Valor", avengers_valor_increase );

  return buff_t::trigger( stacks, value, chance, duration );
}

struct shield_of_the_righteous_t : public paladin_melee_attack_t
{
  shield_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "shield_of_the_righteous", p, p -> spec.shield_of_the_righteous )
  {
    parse_options( options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }

    aoe = -1;

    // not on GCD, usable off-GCD
    trigger_gcd = 0_ms;
    use_off_gcd = true;

    // no weapon multiplier
    weapon_multiplier = 0.0;

    // links needed for Judgment cooldown reduction and seraphim respectively
    cooldown = p -> cooldowns.shield_of_the_righteous;
    if ( ! p -> active.sotr )
    {
      p -> active.sotr = this;
    }
  }

  double action_multiplier() const override
  {
    double m = paladin_melee_attack_t::action_multiplier();

    m *= 1.0 + p() -> buffs.avengers_valor -> value();

    return m;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    // Buff granted regardless of combat roll result
    // Duration and armor bonus recalculation handled in the buff
    p() -> buffs.shield_of_the_righteous -> trigger();

    if ( result_is_hit( execute_state -> result ) )
    {
      // SotR hits reduce Light of the Protector and Avenging Wrath cooldown times if Righteous Protector is talented
      if ( p() -> talents.righteous_protector -> ok() )
      {
        timespan_t reduction = timespan_t::from_seconds( -1.0 * p() -> talents.righteous_protector -> effectN( 1 ).base_value() );
        p() -> cooldowns.avenging_wrath -> adjust( reduction );
        p() -> cooldowns.light_of_the_protector -> adjust( reduction );
        p() -> cooldowns.hand_of_the_protector -> adjust( reduction );
      }
    }

    p() -> buffs.avengers_valor -> expire();

    if ( p() -> azerite_essence.memory_of_lucid_dreams.enabled() )
    {
      p() -> trigger_memory_of_lucid_dreams( 1.0 );
    }
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = paladin_melee_attack_t::recharge_multiplier( cd );

    if ( p() -> player_t::buffs.memory_of_lucid_dreams -> check() )
    {
      rm /= 1.0 + p() -> player_t::buffs.memory_of_lucid_dreams -> data().effectN( 1 ).percent();
    }

    return rm;
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

  // Last Defender
  if ( talents.last_defender -> ok() )
  {
    // Last Defender gives a multiplier of 0.97^N
    s -> result_amount *= last_defender_mitigation();
  }

  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> print_debug( "Damage to {} after passive mitigation is {}", s -> target -> name(), s -> result_amount );

  // Damage Reduction Cooldowns

  // Guardian of Ancient Kings
  if ( buffs.guardian_of_ancient_kings -> up() )
  {
    s -> result_amount *= 1.0 + buffs.guardian_of_ancient_kings -> data().effectN( 3 ).percent();
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> print_debug( "Damage to {} after GoAK is {}", s -> target -> name(), s -> result_amount );
  }

  // Divine Protection
  if ( buffs.divine_protection -> up() )
  {
    s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 1 ).percent();
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> print_debug( "Damage to {} after Divine Protection is {}", s -> target -> name(), s -> result_amount );
  }

  if ( buffs.ardent_defender -> up() )
  {
    s -> result_amount *= 1.0 + buffs.ardent_defender -> data().effectN( 1 ).percent();
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> print_debug( "Damage to {} after Ardent Defender is {}", s -> target -> name(), s -> result_amount );
  }

  // Other stuff

  // Blessed Hammer
  if ( talents.blessed_hammer -> ok() && s -> action )
  {
    buff_t* b = get_target_data( s -> action -> player ) -> debuff.blessed_hammer;

    // BH only reduces auto-attack damage
    if ( b -> up() && !s -> action -> special )
    {
      // apply mitigation and expire the BH buff
      s -> result_amount *= 1.0 - b -> data().effectN( 2 ).percent();
      b -> expire();

      // TODO: Show the absorb on the results like Holy Shield
    }
  }

  // Aegis of Light
  if ( talents.aegis_of_light -> ok() && buffs.aegis_of_light -> up() )
  {
    s -> result_amount *= 1.0 + talents.aegis_of_light -> effectN( 1 ).percent();
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> print_debug( "Damage to {} after Aegis of Light is {}", s -> target -> name(), s -> result_amount );
  }

  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> print_debug( "Damage to {} after mitigation effects is {}", s -> target -> name(), s -> result_amount );

  // Divine Bulwark

  if ( standing_in_consecration() )
  {
    s -> result_amount *= 1.0 + ( cache.mastery() * mastery.divine_bulwark -> effectN( 2 ).mastery_value() );
  }

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
  bool success = rng().roll( gc_proc_chance + talents.first_avenger -> effectN( 2 ).percent() );
  if ( ! success )
    return;

  // reset AS cooldown
  cooldowns.avengers_shield -> reset( true );

  if ( talents.crusaders_judgment -> ok() && cooldowns.judgment -> current_charge < cooldowns.judgment -> charges )
  {
    cooldowns.judgment -> adjust( -( cooldowns.judgment -> duration ), true ); //decrease remaining time by the duration of one charge, i.e., add one charge
  }

  if ( azerite.inspiring_vanguard.enabled() )
  {
    buffs.inspiring_vanguard -> trigger();
  }

  procs.grand_crusader -> occur();
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
{ }

action_t* paladin_t::create_action_protection( const std::string& name, const std::string& options_str )
{
  if ( name == "aegis_of_light"            ) return new aegis_of_light_t           ( this, options_str );
  if ( name == "ardent_defender"           ) return new ardent_defender_t          ( this, options_str );
  if ( name == "avengers_shield"           ) return new avengers_shield_t          ( this, options_str );
  if ( name == "bastion_of_light"          ) return new bastion_of_light_t         ( this, options_str );
  if ( name == "blessed_hammer"            ) return new blessed_hammer_t           ( this, options_str );
  if ( name == "blessing_of_spellwarding"  ) return new blessing_of_spellwarding_t ( this, options_str );
  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );
  if ( name == "hammer_of_the_righteous"   ) return new hammer_of_the_righteous_t  ( this, options_str );
  if ( name == "hand_of_the_protector"     ) return new hand_of_the_protector_t    ( this, options_str );
  if ( name == "light_of_the_protector"    ) return new light_of_the_protector_t   ( this, options_str );
  if ( name == "seraphim"                  ) return new seraphim_t                 ( this, options_str );
  if ( name == "shield_of_the_righteous"   ) return new shield_of_the_righteous_t  ( this, options_str );

  if ( specialization() == PALADIN_PROTECTION )
  {
    if ( name == "judgment" ) return new judgment_prot_t( this, options_str );
  }

  return nullptr;
}

void paladin_t::create_buffs_protection()
{
  buffs.aegis_of_light = make_buff( this, "aegis_of_light", talents.aegis_of_light );
  buffs.ardent_defender = make_buff( this, "ardent_defender", find_specialization_spell( "Ardent Defender" ) );
  buffs.avengers_valor = make_buff( this, "avengers_valor", passives.avengers_valor )
                       -> set_default_value( passives.avengers_valor -> effectN( 1 ).percent() );

  buffs.guardian_of_ancient_kings = make_buff( this, "guardian_of_ancient_kings", find_specialization_spell( "Guardian of Ancient Kings" ) )
                                  -> set_cooldown( 0_ms );

  buffs.holy_shield_absorb = make_buff<absorb_buff_t>( this, "holy_shield", talents.holy_shield );
  buffs.holy_shield_absorb -> set_absorb_school( SCHOOL_MAGIC )
                           -> set_absorb_source( get_stats( "holy_shield_absorb" ) )
                           -> set_absorb_gain( get_gain( "holy_shield_absorb" ) );
  buffs.redoubt = make_buff( this, "redoubt", talents.redoubt -> effectN( 1 ).trigger() );

  buffs.seraphim = make_buff<stat_buff_t>( this, "seraphim", talents.seraphim )
                 -> add_stat( STAT_HASTE_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                 -> add_stat( STAT_CRIT_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                 -> add_stat( STAT_MASTERY_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                 -> add_stat( STAT_VERSATILITY_RATING, talents.seraphim -> effectN( 1 ).average( this ) );
  buffs.seraphim -> set_cooldown( 0_ms ); // let the ability handle the cooldown

  buffs.shield_of_the_righteous = new shield_of_the_righteous_buff_t( this );

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
  talents.bastion_of_light           = find_talent_spell( "Bastion of Light" );

  talents.retribution_aura           = find_talent_spell( "Retribution Aura" );
  //talents.cavalier                   = find_talent_spell( "Cavalier" );
  talents.blessing_of_spellwarding   = find_talent_spell( "Blessing of Spellwarding" );

  //talents.unbreakable_spirit         = find_talent_spell( "Unbreakable Spirit" );
  talents.final_stand                = find_talent_spell( "Final Stand" );
  talents.hand_of_the_protector      = find_talent_spell( "Hand of the Protector" );

  //talents.judgment_of_light          = find_talent_spell( "Judgment of Light" );
  talents.consecrated_ground         = find_talent_spell( "Consecrated Ground" );
  talents.aegis_of_light             = find_talent_spell( "Aegis of Light" );

  talents.last_defender              = find_talent_spell( "Last Defender" );
  talents.righteous_protector        = find_talent_spell( "Righteous Protector" );
  talents.seraphim                   = find_talent_spell( "Seraphim" );

  // Spec passives and useful spells
  spec.protection_paladin = find_specialization_spell( "Protection Paladin" );
  mastery.divine_bulwark = find_mastery_spell( PALADIN_PROTECTION );

  if ( specialization() == PALADIN_PROTECTION )
  {
    spec.judgment_2 = find_specialization_spell( 231657 );

    // Prot doesn't have a version of Judgment debuff or Divine Purpose
    spells.divine_purpose_buff = spell_data_t::nil();
    spells.judgment_debuff = spell_data_t::nil();
  }

  spec.shield_of_the_righteous = find_specialization_spell( "Shield of the Righteous" );
  spells.sotr_buff = find_spell( 132403 );

  passives.avengers_valor      = find_specialization_spell( "Avenger's Shield" ) -> effectN( 4 ).trigger();
  passives.grand_crusader      = find_specialization_spell( "Grand Crusader" );
  passives.riposte             = find_specialization_spell( "Riposte" );
  passives.sanctuary           = find_specialization_spell( "Sanctuary" );

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

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  precombat -> add_action( "potion" );
  precombat -> add_action( this, "Consecration" );
  precombat -> add_action( "lights_judgment" );

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* cds = get_action_priority_list( "cooldowns" );

  def -> add_action( "auto_attack" );

  def -> add_action( "call_action_list,name=cooldowns" );

  cds -> add_action( "fireblood,if=buff.avenging_wrath.up" );
  cds -> add_action( "use_item,name=azsharas_font_of_power,if=cooldown.seraphim.remains<=10|!talent.seraphim.enabled" );
  cds -> add_action( "use_item,name=ashvanes_razor_coral,if=(debuff.razor_coral_debuff.stack>7&buff.avenging_wrath.up)|debuff.razor_coral_debuff.stack=0" );
  cds -> add_talent( this, "Seraphim", "if=cooldown.shield_of_the_righteous.charges_fractional>=2" );
  cds -> add_action( this, "Avenging Wrath", "if=buff.seraphim.up|cooldown.seraphim.remains<2|!talent.seraphim.enabled" );
  cds -> add_action( "memory_of_lucid_dreams,if=!talent.seraphim.enabled|cooldown.seraphim.remains<=gcd|buff.seraphim.up" );
  cds -> add_talent( this, "Bastion of Light", "if=cooldown.shield_of_the_righteous.charges_fractional<=0.5" );
  cds -> add_action( "potion,if=buff.avenging_wrath.up" );

  cds -> add_action( "use_items,if=buff.seraphim.up|!talent.seraphim.enabled" );
  cds -> add_action( "use_item,name=grongs_primal_rage,if=cooldown.judgment.full_recharge_time>4&cooldown.avengers_shield.remains>4&(buff.seraphim.up|cooldown.seraphim.remains+4+gcd>expected_combat_length-time)&consecration.up" );
  cds -> add_action( "use_item,name=pocketsized_computation_device,if=cooldown.judgment.full_recharge_time>4*spell_haste&cooldown.avengers_shield.remains>4*spell_haste&(!equipped.grongs_primal_rage|!trinket.grongs_primal_rage.cooldown.up)&consecration.up" );
  cds -> add_action( "use_item,name=merekthas_fang,if=!buff.avenging_wrath.up&(buff.seraphim.up|!talent.seraphim.enabled)" );
  cds -> add_action( "use_item,name=razdunks_big_red_button" );

  def -> add_action( "worldvein_resonance,if=buff.lifeblood.stack<3" );
  def -> add_action( this, "Shield of the Righteous", "if=(buff.avengers_valor.up&cooldown.shield_of_the_righteous.charges_fractional>=2.5)&(cooldown.seraphim.remains>gcd|!talent.seraphim.enabled)", "Dumping SotR charges" );
  def -> add_action( this, "Shield of the Righteous", "if=(buff.avenging_wrath.up&!talent.seraphim.enabled)|buff.seraphim.up&buff.avengers_valor.up" );
  def -> add_action( this, "Shield of the Righteous", "if=(buff.avenging_wrath.up&buff.avenging_wrath.remains<4&!talent.seraphim.enabled)|(buff.seraphim.remains<4&buff.seraphim.up)" );
  def -> add_talent( this, "Bastion of light", "if=(cooldown.shield_of_the_righteous.charges_fractional<=0.2)&(!talent.seraphim.enabled|buff.seraphim.up)" );
  def -> add_action( "lights_judgment,if=buff.seraphim.up&buff.seraphim.remains<3" );
  def -> add_action( this, "Consecration", "if=!consecration.up" );

  def -> add_action( this, "Judgment", "if=(cooldown.judgment.remains<gcd&cooldown.judgment.charges_fractional>1&cooldown_react)|!talent.crusaders_judgment.enabled" );
  def -> add_action( this, "Avenger's Shield", "if=cooldown_react" );
  def -> add_action( this, "Judgment","if=cooldown_react|!talent.crusaders_judgment.enabled" );
  def -> add_action( "concentrated_flame,if=(!talent.seraphim.enabled|buff.seraphim.up)&!dot.concentrated_flame_burn.remains>0|essence.the_crucible_of_flame.rank<3" );
  def -> add_action( "lights_judgment,if=!talent.seraphim.enabled|buff.seraphim.up" );
  def -> add_action( "anima_of_death" );
  def -> add_talent( this, "Blessed Hammer", "strikes=3" );
  def -> add_action( this, "Hammer of the Righteous" );
  def -> add_action( this, "Consecration" );
  def -> add_action( "heart_essence,if=!(essence.the_crucible_of_flame.major|essence.worldvein_resonance.major|essence.anima_of_life_and_death.major|essence.memory_of_lucid_dreams.major)" );


}
}

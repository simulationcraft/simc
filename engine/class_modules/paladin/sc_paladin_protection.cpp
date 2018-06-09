#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

namespace paladin {

namespace buffs {

  struct ardent_defender_buff_t: public buff_t
  {
    bool oneup_triggered;

    ardent_defender_buff_t( player_t* p ):
      buff_t( p, "ardent_defender", p -> find_specialization_spell( "Ardent Defender" ) ),
      oneup_triggered( false )
    {

    }

    void use_oneup()
    {
      oneup_triggered = true;
    }
  };
}

// Aegis of Light (Protection) ================================================

struct aegis_of_light_t : public paladin_spell_t
{
  aegis_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "aegis_of_light", p, p -> find_talent_spell( "Aegis of Light" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void execute() override
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
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.ardent_defender -> trigger();
  }
};

// Avengers Shield ==========================================================

struct avengers_shield_t : public paladin_spell_t
{
  avengers_shield_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "avengers_shield", p, p -> find_class_spell( "Avenger's Shield" ) )
  {
    parse_options( options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }
    may_crit     = true;

    // TODO: add and legendary bonuses
    base_multiplier *= 1.0 + p -> talents.first_avenger -> effectN( 1 ).percent();
  base_aoe_multiplier *= 1.0;
  if ( p ->talents.first_avenger->ok() )
    base_aoe_multiplier *= 2.0 / 3.0;
  aoe = 3;
  aoe = std::max( aoe, 0 );


    // link needed for trigger_grand_crusader
    cooldown = p -> cooldowns.avengers_shield;
    cooldown -> duration = data().cooldown();
  }

  void init() override
  {
    paladin_spell_t::init();

    if (p()->ferren_marcuss_strength){
      aoe += (p()->spells.ferren_marcuss_strength->effectN(1).base_value());
      base_multiplier *= 1.0 + p()->spells.ferren_marcuss_strength->effectN(2).percent();
    }

  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( p() -> gift_of_the_golden_valkyr ) {
      timespan_t reduction = timespan_t::from_seconds( -1.0 * p() -> spells.gift_of_the_golden_valkyr -> effectN(1).base_value() );
      p() -> cooldowns.guardian_of_ancient_kings -> adjust( reduction );
    }
  }
};

// Bastion of Light ========================================================

struct bastion_of_light_t : public paladin_spell_t
{
  bastion_of_light_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "bastion_of_light", p, p -> find_talent_spell( "Bastion of Light" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p()->cooldowns.shield_of_the_righteous->reset(false, true);
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

  virtual void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    // apply BH debuff to target_data structure
    td( s -> target ) -> buffs.blessed_hammer_debuff -> trigger();
  }

};

struct blessed_hammer_t : public paladin_spell_t
{
  blessed_hammer_tick_t* hammer;
  int num_strikes;

  blessed_hammer_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessed_hammer", p, p -> find_talent_spell( "Blessed Hammer" ) ),
    hammer( new blessed_hammer_tick_t( p ) ), num_strikes( 3 )
  {
    add_option( opt_int( "strikes", num_strikes ) );
    parse_options( options_str );

    // Sane bounds for num_strikes - only makes three revolutions, impossible to hit one target more than 3 times.
    // Likewise calling the pell with 0 strikes is sort of pointless.
    if ( num_strikes < 1 ) { num_strikes = 1; sim -> out_debug.printf( "%s tried to hit less than one time with blessed_hammer", p -> name() ); }
    if ( num_strikes > 3 ) { num_strikes = 3; sim -> out_debug.printf( "%s tried to hit more than three times with blessed_hammer", p -> name() ); }

    dot_duration = timespan_t::zero(); // The periodic event is handled by ground_aoe_event_t
    may_miss = false;
    base_tick_time = timespan_t::from_seconds( 1.667 ); // Rough estimation based on stopwatch testing
    cooldown -> hasted = true;

    tick_may_crit = true;

    add_child( hammer );
  }


  void execute() override
  {
    paladin_spell_t::execute();

    timespan_t initial_delay = num_strikes < 3 ? base_tick_time * 0.25 : timespan_t::zero();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        // spawn at feet of player
        .x( execute_state -> action -> player -> x_position )
        .y( execute_state -> action -> player -> y_position )
        .pulse_time( base_tick_time )
        .duration( base_tick_time * ( num_strikes - 0.5 ) )
        .start_time( sim -> current_time() + initial_delay )
        .action( hammer ), true );

    // Oddly, Grand Crusader seems to proc on cast whether it hits or not (tested 6/19/2016 by Theck on PTR)
    p() -> trigger_grand_crusader();
  }

};

// Blessing of Spellwarding =====================================================

struct blessing_of_spellwarding_t : public paladin_spell_t
{
  blessing_of_spellwarding_t(paladin_t* p, const std::string& options_str)
    : paladin_spell_t("blessing_of_spellwarding", p, p -> find_talent_spell("Blessing of Spellwarding"))
  {
    parse_options(options_str);
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p() -> trigger_forbearance( execute_state -> target );
  }

  virtual bool ready() override
  {
    if (target->debuffs.forbearance->check())
      return false;

    return paladin_spell_t::ready();
  }
};

// Guardian of Ancient Kings ============================================

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "guardian_of_ancient_kings", p, p -> find_specialization_spell( "Guardian of Ancient Kings" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

  cooldown = p->cooldowns.guardian_of_ancient_kings;
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.guardian_of_ancient_kings -> trigger();

  }
};


// Hammer of the Righteous ==================================================

struct hammer_of_the_righteous_aoe_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t( paladin_t* p )
    : paladin_melee_attack_t( "hammer_of_the_righteous_aoe", p, p -> find_spell( 88263 ) )
  {
    // AoE effect always hits if single-target attack succeeds
    // Doesn't proc Grand Crusader
    may_dodge  = false;
    may_parry  = false;
    may_miss   = false;
    background = true;
    aoe        = -1;
    trigger_gcd = timespan_t::zero(); // doesn't incur GCD (HotR does that already)
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
  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ) )
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

// Holy Shield damage proc ====================================================

struct holy_shield_proc_t : public paladin_spell_t
{
  holy_shield_proc_t( paladin_t* p )
    : paladin_spell_t( "holy_shield", p, p -> find_spell( 157122 ) ) // damage data stored in 157122
  {
    background = true;
    proc = true;
    may_miss = false;
    may_crit = true;
  }

};

// Light of the Titans proc ===================================================

struct light_of_the_titans_t : public paladin_heal_t
{
  light_of_the_titans_t( paladin_t* p )
    : paladin_heal_t( "light_of_the_titans", p, p -> find_spell( 209540 ) ) // hot stored in 209540
  {
    background = true;
    proc = true;
  }
};

// Light of the Protector (Protection) ========================================

struct light_of_the_protector_t : public paladin_heal_t
{
  double health_diff_pct;
  light_of_the_titans_t* titans_proc;

  light_of_the_protector_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "light_of_the_protector", p, p -> find_specialization_spell( "Light of the Protector" ) ), health_diff_pct( 0 )
  {
    parse_options( options_str );

    may_crit = false;
    use_off_gcd = true;
    target = p;

    // pct of missing health returned
    health_diff_pct = data().effectN( 1 ).percent();

    // link needed for Righteous Protector / SotR cooldown reduction
    cooldown = p -> cooldowns.light_of_the_protector;
    // prevent spamming
    internal_cooldown -> duration = timespan_t::from_seconds( 1.0 );

    // disable if Hand of the Protector talented
    if ( p -> talents.hand_of_the_protector -> ok() )
      background = true;

    // light of the titans object attached to this
    titans_proc = nullptr;
  }

  void init() override
  {
    paladin_heal_t::init();

    if ( p() -> saruans_resolve ) {
      cooldown -> charges = 2;
    }
  }

  double recharge_multiplier() const override{
    double cdr = paladin_heal_t::recharge_multiplier();
    if (p()->saruans_resolve){
      cdr *= (1 + p()->spells.saruans_resolve->effectN(3).percent());
    }
    return cdr;
  }

  virtual void execute() override
  {
    // heals for 25% of missing health.
    base_dd_min = base_dd_max = health_diff_pct * ( p() -> resources.max[ RESOURCE_HEALTH ] - std::max( p() -> resources.current[ RESOURCE_HEALTH ], 0.0 ) );

    paladin_heal_t::execute();
    if ( titans_proc ){
      titans_proc -> schedule_execute();
    }

  }

};

// Hand of the Protector (Protection) =========================================

struct hand_of_the_protector_t : public paladin_heal_t
{
  double health_diff_pct;
  light_of_the_titans_t* titans_proc;

  hand_of_the_protector_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "hand_of_the_protector", p, p -> find_talent_spell( "Hand of the Protector" ) ), health_diff_pct( 0 )
  {
    parse_options( options_str );

    may_crit = false;
    use_off_gcd = true;

    // pct of missing health returned
    health_diff_pct = data().effectN( 1 ).percent();

    // link needed for Righteous Protector / SotR cooldown reduction
    cooldown = p -> cooldowns.hand_of_the_protector;

    // prevent spamming
    internal_cooldown -> duration = timespan_t::from_seconds( .75 );

    // disable if Hand of the Protector is not talented
    if ( ! p -> talents.hand_of_the_protector -> ok() )
      background = true;

    // light of the titans object attached to this
    titans_proc = nullptr;
  }

  void init() override
  {
    paladin_heal_t::init();

    if ( p() -> saruans_resolve ) {
      cooldown -> charges = 2;
    }
  }

  double recharge_multiplier() const override{
    double cdr = paladin_heal_t::recharge_multiplier();
    if (p()->saruans_resolve){
      cdr *= (1 + p()->spells.saruans_resolve->effectN(3).percent());
    }
    return cdr;
  }

  virtual void execute() override
  {
    // heals for % of missing health.
    base_dd_min = base_dd_max = health_diff_pct * (target->resources.max[RESOURCE_HEALTH] - std::max(target->resources.current[RESOURCE_HEALTH], 0.0) );

    paladin_heal_t::execute();

    // Light of the Titans only works if self-cast
    if ( titans_proc && target == p() )
      titans_proc -> schedule_execute();

  }

};


// Seraphim ( Protection ) ====================================================

struct seraphim_t : public paladin_spell_t
{
  seraphim_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "seraphim", p, p -> find_talent_spell( "Seraphim" ) )
  {
    parse_options( options_str );

    harmful = false;
    may_miss = false;

    // ugly hack for if SotR hasn't been put in the APL yet
    if ( p -> cooldowns.shield_of_the_righteous -> charges < 3 )
    {
      const spell_data_t* sotr = p -> find_class_spell( "Shield of the Righteous" );
      p -> cooldowns.shield_of_the_righteous -> charges = p -> cooldowns.shield_of_the_righteous -> current_charge = sotr -> _charges;
      p -> cooldowns.shield_of_the_righteous -> duration = timespan_t::from_millis( sotr -> _charge_cooldown );
    }
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    // duration depends on sotr charges, need to do some math
    timespan_t duration = timespan_t::zero();
    int available_charges = p() -> cooldowns.shield_of_the_righteous -> current_charge;
    int full_charges_used = 0;
    timespan_t remains = p() -> cooldowns.shield_of_the_righteous -> current_charge_remains();

    if ( available_charges >= 1 )
    {
      full_charges_used = std::min( available_charges, 2 );
      duration = full_charges_used * p() -> talents.seraphim -> duration();
      for ( int i = 0; i < full_charges_used; i++ )
        p() -> cooldowns.shield_of_the_righteous -> start( p() -> active_sotr  );
    }
    if ( full_charges_used < 2 )
    {
      double partial_charges_used = 0.0;
      partial_charges_used = ( p() -> cooldowns.shield_of_the_righteous -> duration.total_seconds() - remains.total_seconds() ) / p() -> cooldowns.shield_of_the_righteous -> duration.total_seconds();
      duration += partial_charges_used * p() -> talents.seraphim -> duration();
      p() -> cooldowns.shield_of_the_righteous -> adjust( p() -> cooldowns.shield_of_the_righteous -> duration - remains );
    }

    if ( duration > timespan_t::zero() )
      p() -> buffs.seraphim -> trigger( 1, -1.0, -1.0, duration );

  }

};


// Shield of the Righteous ==================================================

struct shield_of_the_righteous_t : public paladin_melee_attack_t
{
  shield_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "shield_of_the_righteous", p, p -> find_class_spell( "Shield of the Righteous" ) )
  {
    parse_options( options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }

    // not on GCD, usable off-GCD
    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;

    // no weapon multiplier
    weapon_multiplier = 0.0;

    // link needed for Judgment cooldown reduction
    cooldown = p -> cooldowns.shield_of_the_righteous;
    p -> active_sotr = this;
  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();

    //Buff granted regardless of combat roll result
    //Duration is additive on refresh, stats not recalculated
    if ( p() -> buffs.shield_of_the_righteous -> check() )
    {
      p() -> buffs.shield_of_the_righteous -> extend_duration( p(), p() -> buffs.shield_of_the_righteous -> buff_duration );
    }
    else
      p() -> buffs.shield_of_the_righteous -> trigger();
  }

  // Special things that happen when SotR damages target
  virtual void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) ) // TODO: not needed anymore? Can we even miss?
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

    paladin_melee_attack_t::impact( s );
  }
};

// paladin_t::target_mitigation ===============================================

void paladin_t::target_mitigation( school_e school,
                                   dmg_e    dt,
                                   action_state_t* s )
{
  player_t::target_mitigation( school, dt, s );

  // various mitigation effects, Ardent Defender goes last due to absorb/heal mechanics

  // Passive sources (Sanctuary)
  s -> result_amount *= 1.0 + passives.sanctuary -> effectN( 1 ).percent();

  // Last Defender
  if ( talents.last_defender -> ok() )
  {
    // Last Defender gives a multiplier of 0.97^N - coded using spell data in case that changes
    s -> result_amount *= std::pow( 1.0 - talents.last_defender -> effectN( 2 ).percent(), buffs.last_defender -> current_stack );
  }

  // heathcliffs
  if (standing_in_consecration() && heathcliffs_immortality)
  {
    s->result_amount *= 1.0 - spells.heathcliffs_immortality->effectN(1).percent();
  }


  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> out_debug.printf( "Damage to %s after passive mitigation is %f", s -> target -> name(), s -> result_amount );

  // Damage Reduction Cooldowns

  // Guardian of Ancient Kings
  if ( buffs.guardian_of_ancient_kings -> up() && specialization() == PALADIN_PROTECTION )
  {
    s -> result_amount *= 1.0 + buffs.guardian_of_ancient_kings -> data().effectN( 3 ).percent();
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after GAnK is %f", s -> target -> name(), s -> result_amount );
  }

  // Divine Protection
  if ( buffs.divine_protection -> up() )
  {
    if ( util::school_type_component( school, SCHOOL_MAGIC ) )
    {
      s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 1 ).percent();
    }
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Divine Protection is %f", s -> target -> name(), s -> result_amount );
  }

  // Other stuff

  // Blessed Hammer
  if ( talents.blessed_hammer -> ok() )
  {
    buff_t* b = get_target_data( s -> action -> player ) -> buffs.blessed_hammer_debuff;

    // BH only reduces auto-attack damage. The only distinguishing feature of auto attacks is that
    // our enemies call them "melee_main_hand" and "melee_off_hand", so we need to check for "hand" in name_str
    if ( b -> up() && util::str_in_str_ci( s -> action -> name_str, "_hand" ) )
    {
      // apply mitigation and expire the BH buff
      s -> result_amount *= 1.0 - b -> data().effectN( 2 ).percent();
      b -> expire();
    }
  }

  // Knight Templar
  if ( talents.knight_templar -> ok() && buffs.divine_steed -> up() )
    s -> result_amount *= 1.0 + talents.knight_templar -> effectN( 2 ).percent();

  // Aegis of Light
  if ( talents.aegis_of_light -> ok() && buffs.aegis_of_light -> up() )
    s -> result_amount *= 1.0 + talents.aegis_of_light -> effectN( 1 ).percent();

  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> out_debug.printf( "Damage to %s after other mitigation effects but before SotR is %f", s -> target -> name(), s -> result_amount );

  // Shield of the Righteous
  if ( buffs.shield_of_the_righteous -> check())
  {
    // sotr has a lot going on, so we'll be verbose
    double sotr_mitigation;

    // base effect
    sotr_mitigation = buffs.shield_of_the_righteous -> data().effectN( 1 ).percent();

    // mastery bonus
    sotr_mitigation += cache.mastery() * passives.divine_bulwark -> effectN( 4 ).mastery_value();


    // clamp is hardcoded in tooltip, not shown in effects
    sotr_mitigation = std::max( -0.80, sotr_mitigation );
    sotr_mitigation = std::min( -0.25, sotr_mitigation );

    s -> result_amount *= 1.0 + sotr_mitigation;

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after SotR mitigation is %f", s -> target -> name(), s -> result_amount );
  }

  // Ardent Defender
  if ( buffs.ardent_defender -> check() )
  {
    if ( s -> result_amount > 0 && s -> result_amount >= resources.current[ RESOURCE_HEALTH ] )
    {
      // Ardent defender is a little odd - it doesn't heal you *for* 12%, it heals you *to* 12%.
      // It does this by either absorbing all damage and healing you for the difference between 12% and your current health (if current < 12%)
      // or absorbing any damage that would take you below 12% (if current > 12%).
      // To avoid complications with absorb modeling, we're just going to kludge it by adjusting the amount gained or lost accordingly.
      // Also arbitrarily capping at 3x max health because if you're seriously simming bosses that hit for >300% player health I hate you.
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
        // completely arbitrary heal-capping here at 3x max health, done just so paladin health timelines don't look so insane
        resource_gain( RESOURCE_HEALTH,
                       std::min( AD_health_threshold - resources.current[ RESOURCE_HEALTH ], 3 * resources.max[ RESOURCE_HEALTH ] ),
                       nullptr,
                       s -> action );
      }
      buffs.ardent_defender -> use_oneup();
      buffs.ardent_defender -> expire();
    }

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Ardent Defender is %f", s -> target -> name(), s -> result_amount );
  }
}

void paladin_t::trigger_grand_crusader()
{
  // escape if we don't have Grand Crusader
  if ( ! passives.grand_crusader -> ok() )
    return;

  // attempt to proc the buff, returns true if successful
  if ( buffs.grand_crusader -> trigger() )
  {
    // reset AS cooldown
    cooldowns.avengers_shield -> reset( true );

    if (talents.crusaders_judgment -> ok() && cooldowns.judgment -> current_charge < cooldowns.judgment -> charges)
    {
      cooldowns.judgment -> adjust( -(cooldowns.judgment -> duration) ); //decrease remaining time by the duration of one charge, i.e., add one charge
    }
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

  // Check for proc
  if ( rng().roll( talents.holy_shield -> proc_chance() ) )
  {
    active_holy_shield_proc -> target = s -> action -> player;
    active_holy_shield_proc -> schedule_execute();
  }
}

bool paladin_t::standing_in_consecration() const
{
  // new
  for ( size_t i = 0; i < active_consecrations.size(); i++ )
  {
    // calculate current distance to each consecration
    paladin_ground_aoe_t* cons_to_test = active_consecrations[ i ];

    double distance = get_position_distance( cons_to_test -> params -> x(), cons_to_test -> params -> y() );

    // exit with true if we're in range of any one Cons center
    if ( distance <= cons_to_test -> radius )
      return true;
  }
  // if we're not in range of any of them
  return false;
}

// Initialization
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

  return nullptr;
}

void paladin_t::create_buffs_protection()
{
  buffs.guardian_of_ancient_kings      = make_buff( this, "guardian_of_ancient_kings", find_specialization_spell( "Guardian of Ancient Kings" ) )
                                          ->set_cooldown( timespan_t::zero() ); // let the ability handle the CD
  buffs.grand_crusader                 = make_buff( this, "grand_crusader", passives.grand_crusader -> effectN( 1 ).trigger() )
    ->set_chance( passives.grand_crusader -> proc_chance() + ( 0.0 + talents.first_avenger -> effectN( 2 ).percent() ) );
  buffs.shield_of_the_righteous        = make_buff( this, "shield_of_the_righteous", find_spell( 132403 ) );
  buffs.ardent_defender                = new buffs::ardent_defender_buff_t( this );
  buffs.aegis_of_light                 = make_buff( this, "aegis_of_light", find_talent_spell( "Aegis of Light" ) );
  buffs.seraphim                       = stat_buff_creator_t( this, "seraphim", talents.seraphim )
                                          .add_stat( STAT_HASTE_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                                          .add_stat( STAT_CRIT_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                                          .add_stat( STAT_MASTERY_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                                          .add_stat( STAT_VERSATILITY_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                                          .cd( timespan_t::zero() ); // let the ability handle the cooldown
  buffs.bulwark_of_order               = make_buff<absorb_buff_t>( this, "bulwark_of_order", find_spell( 209388 ) );
  buffs.bulwark_of_order->set_absorb_source( get_stats( "bulwark_of_order" ) )
    ->set_absorb_gain( get_gain( "bulwark_of_order" ) )
    ->set_max_stack( 1 ); // not sure why data says 3 stacks

  // Talents
  buffs.holy_shield_absorb     = make_buff<absorb_buff_t>( this, "holy_shield", find_spell( 157122 ) );
  buffs.holy_shield_absorb->set_absorb_school( SCHOOL_MAGIC )
      ->set_absorb_source( get_stats( "holy_shield_absorb" ) )
      ->set_absorb_gain( get_gain( "holy_shield_absorb" ) );
}

void paladin_t::init_spells_protection()
{
  // Talents
  talents.first_avenger              = find_talent_spell( "First Avenger" );
  talents.bastion_of_light           = find_talent_spell( "Bastion of Light" );
  talents.crusaders_judgment         = find_talent_spell( "Crusader's Judgment" );
  talents.holy_shield                = find_talent_spell( "Holy Shield" );
  talents.blessed_hammer             = find_talent_spell( "Blessed Hammer" );
  talents.redoubt                    = find_talent_spell( "Redoubt" );
  talents.blessing_of_spellwarding   = find_talent_spell( "Blessing of Spellwarding" );
  talents.blessing_of_salvation      = find_talent_spell( "Blessing of Salvation" );
  talents.retribution_aura           = find_talent_spell( "Retribution Aura" );
  talents.hand_of_the_protector      = find_talent_spell( "Hand of the Protector" );
  talents.knight_templar             = find_talent_spell( "Knight Templar" );
  talents.final_stand                = find_talent_spell( "Final Stand" );
  talents.aegis_of_light             = find_talent_spell( "Aegis of Light" );
  //talents.judgment_of_light          = find_talent_spell( "Judgment of Light" );
  talents.consecrated_ground         = find_talent_spell( "Consecrated Ground" );
  talents.righteous_protector        = find_talent_spell( "Righteous Protector" );
  talents.seraphim                   = find_talent_spell( "Seraphim" );
  talents.last_defender              = find_talent_spell( "Last Defender" );

  // Spells
  spells.pillars_of_inmost_light       = find_spell( 248102 );
  spells.ferren_marcuss_strength       = find_spell( 207614 );
  spells.saruans_resolve               = find_spell( 234653 );
  spells.gift_of_the_golden_valkyr     = find_spell( 207628 );
  spells.heathcliffs_immortality       = find_spell( 207599 );
  spells.consecration_bonus            = find_spell( 188370 );

  // mastery
  passives.divine_bulwark         = find_mastery_spell( PALADIN_PROTECTION );

  // Prot Passives
  passives.bladed_armor           = find_specialization_spell( "Bladed Armor" );
  passives.grand_crusader         = find_specialization_spell( "Grand Crusader" );
  passives.guarded_by_the_light   = find_specialization_spell( "Guarded by the Light" );
  passives.sanctuary              = find_specialization_spell( "Sanctuary" );
  passives.riposte                = find_specialization_spell( "Riposte" );
  passives.improved_block         = find_specialization_spell( "Improved Block" );

  if ( specialization() == PALADIN_PROTECTION )
  {
    extra_regen_period  = passives.sanctuary -> effectN( 5 ).period();
    extra_regen_percent = passives.sanctuary -> effectN( 5 ).percent();

    spec.judgment_2 = find_specialization_spell( 231657 );
  }

  if ( talents.holy_shield -> ok() )
    active_holy_shield_proc = new holy_shield_proc_t( this );
}

void paladin_t::generate_action_prio_list_prot()
{
  ///////////////////////
  // Precombat List
  ///////////////////////

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  precombat -> add_action( "flask" );

  // Food
  precombat -> add_action( "food" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting
  precombat -> add_action( "potion" );

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list("default");
  action_priority_list_t* prot = get_action_priority_list("prot");
  //action_priority_list_t* prot_aoe = get_action_priority_list("prot_aoe");
  action_priority_list_t* dps = get_action_priority_list("max_dps");
  action_priority_list_t* surv = get_action_priority_list("max_survival");

  dps->action_list_comment_str = "This is a high-DPS (but low-survivability) configuration.\n# Invoke by adding \"actions+=/run_action_list,name=max_dps\" to the beginning of the default APL.";
  surv->action_list_comment_str = "This is a high-survivability (but low-DPS) configuration.\n# Invoke by adding \"actions+=/run_action_list,name=max_survival\" to the beginning of the default APL.";

  def->add_action("auto_attack");

  // usable items
  int num_items = (int)items.size();
  for (int i = 0; i < num_items; i++)
    if (items[i].has_special_effect(SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE))
      def->add_action("use_item,name=" + items[i].name_str);

  // profession actions
  std::vector<std::string> profession_actions = get_profession_actions();
  for (size_t i = 0; i < profession_actions.size(); i++)
    def->add_action(profession_actions[i]);

  // racial actions
  std::vector<std::string> racial_actions = get_racial_actions();
  for (size_t i = 0; i < racial_actions.size(); i++)
    def->add_action(racial_actions[i]);

  // clone all of the above to the other two action lists
  dps->action_list = def->action_list;
  surv->action_list = def->action_list;

  // TODO: Create this.

  //threshold for defensive abilities
  std::string threshold = "incoming_damage_2500ms>health.max*0.4";
  std::string threshold_lotp = "incoming_damage_13000ms<health.max*1.6";
  std::string threshold_lotp_rp = "incoming_damage_10000ms<health.max*1.25";
  std::string threshold_hotp = "incoming_damage_9000ms<health.max*1.2";
  std::string threshold_hotp_rp = "incoming_damage_6000ms<health.max*0.7";

  for (size_t i = 0; i < racial_actions.size(); i++)
    def->add_action(racial_actions[i]);
  def->add_action("call_action_list,name=prot");

  //defensive
  //prot->add_talent(this, "Seraphim", "if=talent.seraphim.enabled&action.shield_of_the_righteous.charges>=2");
  prot->add_action(this, "Shield of the Righteous", "if=!talent.seraphim.enabled&(action.shield_of_the_righteous.charges>2)&!(buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  //prot->add_action(this, "Shield of the Righteous", "if=(talent.bastion_of_light.enabled&talent.seraphim.enabled&buff.seraphim.up&cooldown.bastion_of_light.up)&!(buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  //prot->add_action(this, "Shield of the Righteous", "if=(talent.bastion_of_light.enabled&!talent.seraphim.enabled&cooldown.bastion_of_light.up)&!(buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  prot->add_talent(this, "Bastion of Light", "if=!talent.seraphim.enabled&talent.bastion_of_light.enabled&action.shield_of_the_righteous.charges<1");
  prot->add_action(this, "Light of the Protector", "if=(health.pct<40)");
  prot->add_talent(this, "Hand of the Protector",  "if=(health.pct<40)");
  prot->add_action(this, "Light of the Protector", "if=("+threshold_lotp_rp+")&health.pct<55&talent.righteous_protector.enabled");
  prot->add_action(this, "Light of the Protector", "if=("+threshold_lotp+")&health.pct<55");
  prot->add_talent(this, "Hand of the Protector",  "if=("+threshold_hotp_rp+")&health.pct<65&talent.righteous_protector.enabled");
  prot->add_talent(this, "Hand of the Protector",  "if=("+threshold_hotp+")&health.pct<55");
  prot->add_action(this, "Divine Steed", "if=!talent.seraphim.enabled&talent.knight_templar.enabled&" + threshold + "&!(buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_talent(this, "Aegis of Light", "if=!talent.seraphim.enabled&" + threshold + "&!(buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Guardian of Ancient Kings", "if=!talent.seraphim.enabled&" + threshold + "&!(buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Divine Shield", "if=!talent.seraphim.enabled&talent.final_stand.enabled&" + threshold + "&!(buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Ardent Defender", "if=!talent.seraphim.enabled&" + threshold + "&!(buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Lay on Hands", "if=!talent.seraphim.enabled&health.pct<15");

  //potion
  if (sim->allow_potions)
  {
    if (level() > 100)
    {
      prot->add_action("potion,name=old_war,if=buff.avenging_wrath.up&talent.seraphim.enabled&active_enemies<3");
      prot->add_action("potion,name=prolonged_power,if=buff.avenging_wrath.up&talent.seraphim.enabled&active_enemies>=3");
      prot->add_action("potion,name=unbending_potion,if=!talent.seraphim.enabled");
    }
    else if (true_level >= 80)
    {
      prot->add_action("potion,if=" + threshold + "&!(buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)|target.time_to_die<=25");
    }
  }

  //stoneform
  prot->add_action("stoneform,if=!talent.seraphim.enabled&" + threshold + "&!(buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");

  //dps-single-target
  prot->add_action(this, "Avenging Wrath", "if=!talent.seraphim.enabled");
  //prot->add_action(this, "Avenging Wrath", "if=talent.seraphim.enabled&buff.seraphim.up");
  //prot->add_action( "call_action_list,name=prot_aoe,if=spell_targets.avenger_shield>3" );
  prot->add_action(this, "Judgment", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Avenger's Shield","if=!talent.seraphim.enabled&talent.crusaders_judgment.enabled&buff.grand_crusader.up");
  prot->add_talent(this, "Blessed Hammer", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Avenger's Shield", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Consecration", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Hammer of the Righteous", "if=!talent.seraphim.enabled");

  //max dps build

  prot->add_talent(this,"Seraphim","if=talent.seraphim.enabled&action.shield_of_the_righteous.charges>=2");
  prot->add_action(this, "Avenging Wrath", "if=talent.seraphim.enabled&(buff.seraphim.up|cooldown.seraphim.remains<4)");
  prot->add_action(this, "Ardent Defender", "if=talent.seraphim.enabled&buff.seraphim.up");
  prot->add_action(this, "Shield of the Righteous", "if=talent.seraphim.enabled&(cooldown.consecration.remains>=0.1&(action.shield_of_the_righteous.charges>2.5&cooldown.seraphim.remains>3)|(buff.seraphim.up))");
  prot->add_action(this, "Avenger's Shield", "if=talent.seraphim.enabled");
  prot->add_action(this, "Judgment", "if=talent.seraphim.enabled&(active_enemies<2|set_bonus.tier20_2pc)");
  prot->add_action(this, "Consecration", "if=talent.seraphim.enabled&(buff.seraphim.remains>6|buff.seraphim.down)");
  prot->add_action(this, "Judgment", "if=talent.seraphim.enabled");
  prot->add_action(this, "Consecration", "if=talent.seraphim.enabled");
  prot->add_talent(this, "Blessed Hammer", "if=talent.seraphim.enabled");
  prot->add_action(this, "Hammer of the Righteous", "if=talent.seraphim.enabled");
}
}

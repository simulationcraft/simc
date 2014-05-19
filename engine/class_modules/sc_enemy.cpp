// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Enemy
// ==========================================================================

namespace { // UNNAMED NAMESPACE

enum tmi_boss_e
{
  TMI_NONE = 0,
  TMI_T15LFR, TMI_T15N, TMI_T15H, TMI_T16_10N, TMI_T16_25N, TMI_T16_10H, TMI_T16_25H, TMI_T17_Q
};

// Enemy actions are generic to serve both enemy_t and enemy_add_t,
// so they can only rely on player_t and should have no knowledge of class definitions

// Melee ====================================================================

struct melee_t : public melee_attack_t
{
  melee_t( const std::string& name, player_t* player ) :
    melee_attack_t( name, player, spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    may_crit    = true;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    base_dd_min = 26000;
    base_execute_time = timespan_t::from_seconds( 2.4 );
    aoe = -1;
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.
    tl.clear();
    tl.push_back( target );

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      //only add non heal_target tanks to this list for now
      if ( !sim -> actor_list[ i ] -> is_sleeping() &&
           !sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] -> primary_role() == ROLE_TANK &&
           sim -> actor_list[ i ] != target &&
           sim -> actor_list[ i ] != sim -> heal_target )
        tl.push_back( sim -> actor_list[ i ] );
    }
    //if we have no target (no tank), add the healing target as substitute
    if ( tl.empty() )
    {
      tl.push_back( sim->heal_target );
    }

    return tl.size();
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public attack_t
{
  auto_attack_t( player_t* p, const std::string& options_str ) :
    attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    p -> main_hand_attack = new melee_t( "melee_main_hand", p );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = timespan_t::from_seconds( 2.4 );

    int aoe_tanks = 0;
    double damage_range = -1;
    option_t options[] =
    {
      opt_float( "damage", p -> main_hand_attack -> base_dd_min ),
      opt_timespan( "attack_speed", p -> main_hand_attack -> base_execute_time ),
      opt_bool( "aoe_tanks", aoe_tanks ),
      opt_float( "range", damage_range ),
      opt_null()
    };
    parse_options( options, options_str );

    p -> main_hand_attack -> target = target;

    if ( aoe_tanks == 1 )
      p -> main_hand_attack -> aoe = -1;
    else
      p->main_hand_attack->aoe = aoe_tanks;

    // check that the specified damage range is sane
    if ( damage_range > p -> main_hand_attack -> base_dd_min || damage_range < 0 )
      damage_range = 0.1 * p -> main_hand_attack -> base_dd_min; // if not, set to +/-10%

    // set damage range to mean +/- range
    p -> main_hand_attack -> base_dd_max = p -> main_hand_attack -> base_dd_min + damage_range;
    p -> main_hand_attack -> base_dd_min -= damage_range;

    // if the execute time is somehow less than 10 ms, set it back to the default of 1.5 seconds
    if ( p -> main_hand_attack -> base_execute_time < timespan_t::from_seconds( 0.01 ) )
      p -> main_hand_attack -> base_execute_time = timespan_t::from_seconds( 1.5 );

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );
    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    name_str = name_str + "_" + target -> name();

    trigger_gcd = timespan_t::zero();
  }

  virtual double calculate_direct_amount( action_state_t* s )
  {
    // force boss attack size to vary regardless of whether the sim itself does
    int previous_average_range_state = sim ->average_range;
    sim -> average_range = 0;

    double amount = attack_t::calculate_direct_amount( s );

    sim -> average_range = previous_average_range_state;

    return amount;
  }

  virtual void execute()
  {
    player -> main_hand_attack -> schedule_execute();
    if ( player -> off_hand_attack )
    {
      player -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    if ( player -> is_moving() ) return false;
    return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Melee Nuke ===============================================================

struct melee_nuke_t : public attack_t
{
  melee_nuke_t( player_t* p, const std::string& options_str ) :
    attack_t( "melee_nuke", p, spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    may_miss = may_dodge = may_parry = false;
    may_block = true;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min = 25000;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );

    int aoe_tanks = 0;
    double damage_range = -1;
    option_t options[] =
    {
      opt_float( "damage", base_dd_min ),
      opt_timespan( "attack_speed", base_execute_time ),
      opt_timespan( "cooldown",     cooldown -> duration ),
      opt_bool( "aoe_tanks", aoe_tanks ),
      opt_float( "range", damage_range ),
      opt_null()
    };
    parse_options( options, options_str );
        
    // check that the specified damage range is sane
    if ( damage_range > base_dd_min || damage_range < 0 )
      damage_range = 0.1 * base_dd_min; // if not, set to +/-10%

    base_dd_max = base_dd_min + damage_range;
    base_dd_min -= damage_range;

    if ( base_execute_time < timespan_t::zero() )
      base_execute_time = timespan_t::from_seconds( 3.0 );

    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    name_str = name_str + "_" + target -> name();

    if ( aoe_tanks == 1 )
      aoe = -1;
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.

    tl.clear();
    tl.push_back( target );
    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      //only add non heal_target tanks to this list for now
      if ( !sim -> actor_list[ i ] -> is_sleeping() &&
           !sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] -> primary_role() == ROLE_TANK &&
           sim -> actor_list[ i ] != target &&
           sim -> actor_list[ i ] != sim -> heal_target )
        tl.push_back( sim -> actor_list[ i ] );
    }
    //if we have no target (no tank), add the healing target as substitute
    if ( tl.empty() )
    {
      tl.push_back( sim->heal_target );
    }
    return tl.size();
  }

  
  virtual double calculate_direct_amount( action_state_t* s )
  {
    // force boss attack size to vary regardless of whether the sim itself does
    int previous_average_range_state = sim ->average_range;
    sim -> average_range = 0;

    double amount = attack_t::calculate_direct_amount( s );

    sim -> average_range = previous_average_range_state;

    return amount;
  }
};

// Spell Nuke ===============================================================

struct spell_nuke_t : public spell_t
{
  spell_nuke_t( player_t* p, const std::string& options_str ) :
    spell_t( "spell_nuke", p, spell_data_t::nil() )
  {
    school = SCHOOL_FIRE;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min = 5000;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );

    int aoe_tanks = 0;
    double damage_range = -1;
    option_t options[] =
    {
      opt_float( "damage", base_dd_min ),
      opt_timespan( "attack_speed", base_execute_time ),
      opt_timespan( "cooldown",     cooldown -> duration ),
      opt_bool( "aoe_tanks", aoe_tanks ),
      opt_float( "range", damage_range ),
      opt_null()
    };
    parse_options( options, options_str );
    
    // check that the specified damage range is sane
    if ( damage_range > base_dd_min || damage_range < 0 )
      damage_range = 0.1 * base_dd_min; // if not, set to +/-10%

    base_dd_max = base_dd_min + damage_range;
    base_dd_min -= damage_range;

    if ( base_execute_time < timespan_t::zero() )
      base_execute_time = timespan_t::from_seconds( 3.0 );

    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    name_str = name_str + "_" + target -> name();

    may_crit = false;
    if ( aoe_tanks == 1 )
      aoe = -1;
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.

    tl.clear();
    tl.push_back( target );
    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      //only add non heal_target tanks to this list for now
      if ( !sim -> actor_list[ i ] -> is_sleeping() &&
           !sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] -> primary_role() == ROLE_TANK &&
           sim -> actor_list[ i ] != target &&
           sim -> actor_list[ i ] != sim -> heal_target )
        tl.push_back( sim -> actor_list[ i ] );
    }
    //if we have no target (no tank), add the healing target as substitute
    if ( tl.empty() )
    {
      tl.push_back( sim->heal_target );
    }
    return tl.size();
  }

  
  virtual double calculate_direct_amount( action_state_t* s )
  {
    // force boss attack size to vary regardless of whether the sim itself does
    int previous_average_range_state = sim ->average_range;
    sim -> average_range = 0;

    double amount = spell_t::calculate_direct_amount( s );

    sim -> average_range = previous_average_range_state;

    return amount;
  }

};

// Spell DoT ================================================================

struct spell_dot_t : public spell_t
{
  spell_dot_t( player_t* p, const std::string& options_str ) :
    spell_t( "spell_dot", p, spell_data_t::nil() )
  {
    school = SCHOOL_FIRE;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks = 10;
    base_td = 5000;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );

    int aoe_tanks = 0;
    option_t options[] =
    {
      opt_float( "damage", base_td ),
      opt_timespan( "tick_time", base_tick_time ),
      opt_int( "num_ticks", num_ticks ),
      opt_timespan( "cooldown",     cooldown -> duration ),
      opt_bool( "aoe_tanks", aoe_tanks ),
      opt_null()
    };
    parse_options( options, options_str );

    if ( base_tick_time < timespan_t::zero() ) // User input sanity check
      base_execute_time = timespan_t::from_seconds( 1.0 );

    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    name_str = name_str + "_" + target -> name();

    tick_may_crit = false; // FIXME: should ticks crit or not?
    may_crit = false;
    if ( aoe_tanks == 1 )
      aoe = -1;
  }

  virtual void execute()
  {
    target_cache.is_valid = false;
    spell_t::execute();
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.

    tl.clear();
    tl.push_back( target );
    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      //only add non heal_target tanks to this list for now
      if ( !sim -> actor_list[ i ] -> is_sleeping() &&
           !sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] -> primary_role() == ROLE_TANK &&
           sim -> actor_list[ i ] != target &&
           sim -> actor_list[ i ] != sim -> heal_target )
        tl.push_back( sim -> actor_list[ i ] );
    }
    //if we have no target (no tank), add the healing target as substitute
    if ( tl.empty() )
    {
      tl.push_back( sim->heal_target );
    }
    return tl.size();
  }

};

// Spell AoE ================================================================

struct spell_aoe_t : public spell_t
{
  spell_aoe_t( player_t* p, const std::string& options_str ) :
    spell_t( "spell_aoe", p, spell_data_t::nil() )
  {
    school = SCHOOL_FIRE;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min = 5000;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );

    option_t options[] =
    {
      opt_float( "damage", base_dd_min ),
      opt_timespan( "cast_time", base_execute_time ),
      opt_timespan( "cooldown", cooldown -> duration ),
      opt_null()
    };
    parse_options( options, options_str );

    base_dd_max = base_dd_min;
    if ( base_execute_time < timespan_t::from_seconds( 0.01 ) )
      base_execute_time = timespan_t::from_seconds( 3.0 );

    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    name_str = name_str + "_" + target -> name();

    may_crit = false;

    aoe = -1;
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.

    tl.clear();
    tl.push_back( target );

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; ++i )
    {
      if ( ! sim -> actor_list[ i ] -> is_sleeping() &&
           !sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] != target )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }

};

// Summon Add ===============================================================

struct summon_add_t : public spell_t
{
  std::string add_name;
  timespan_t summoning_duration;
  pet_t* pet;

  summon_add_t( player_t* p, const std::string& options_str ) :
    spell_t( "summon_add", player, spell_data_t::nil() ),
    add_name( "" ), summoning_duration( timespan_t::zero() ), pet( 0 )
  {
    option_t options[] =
    {
      opt_string( "name", add_name ),
      opt_timespan( "duration", summoning_duration ),
      opt_timespan( "cooldown", cooldown -> duration ),
      opt_null()
    };
    parse_options( options, options_str );

    school = SCHOOL_PHYSICAL;
    pet = p -> find_pet( add_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p -> name(), add_name.c_str() );
      sim -> cancel();
    }

    harmful = false;

    trigger_gcd = timespan_t::from_seconds( 1.5 );
  }

  virtual void execute()
  {
    spell_t::execute();

    player -> summon_pet( add_name, summoning_duration );
  }

  virtual bool ready()
  {
    if ( ! pet -> is_sleeping() )
      return false;

    return spell_t::ready();
  }
};

action_t* enemy_create_action( player_t* p, const std::string& name, const std::string& options_str )
{
  if ( name == "auto_attack" ) return new auto_attack_t( p, options_str );
  if ( name == "melee_nuke"  ) return new  melee_nuke_t( p, options_str );
  if ( name == "spell_nuke"  ) return new  spell_nuke_t( p, options_str );
  if ( name == "spell_dot"   ) return new   spell_dot_t( p, options_str );
  if ( name == "spell_aoe"   ) return new   spell_aoe_t( p, options_str );
  if ( name == "summon_add"  ) return new  summon_add_t( p, options_str );

  return NULL;
}

struct enemy_t : public player_t
{
  double fixed_health, initial_health;
  double fixed_health_percentage, initial_health_percentage;
  double health_recalculation_dampening_exponent;
  timespan_t waiting_time;
  std::string tmi_boss_str;
  tmi_boss_e tmi_boss_enum;

  std::vector<buff_t*> buffs_health_decades;

  enemy_t( sim_t* s, const std::string& n, race_e r = RACE_HUMANOID, player_e type = ENEMY ) :
    player_t( s, type, n, r ),
    fixed_health( 0 ), initial_health( 0 ),
    fixed_health_percentage( 0 ), initial_health_percentage( 100.0 ),
    health_recalculation_dampening_exponent( 1.0 ),
    waiting_time( timespan_t::from_seconds( 1.0 ) ),
    tmi_boss_str( "none" ),
    tmi_boss_enum( TMI_NONE )
  {
    s -> target_list.push_back( this );
    position_str = "front";
    level = level + 3;
  }

  virtual role_e primary_role() const
  { return ROLE_TANK; }

  virtual resource_e primary_resource() const
  { return RESOURCE_MANA; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual void init_base_stats();
  virtual void init_defense();
  virtual void create_buffs();
  virtual void init_resources( bool force = false );
  virtual void init_target();
  virtual void init_action_list();
  virtual void init_stats();
  virtual double resource_loss( resource_e, double, gain_t*, action_t* );
  virtual void create_options();
  virtual pet_t* create_pet( const std::string& add_name, const std::string& pet_type = std::string() );
  virtual void create_pets();
  virtual double health_percentage() const;
  virtual void combat_begin();
  virtual void combat_end();
  virtual void recalculate_health();
  virtual void demise();
  virtual expr_t* create_expression( action_t* action, const std::string& type );
  virtual timespan_t available() const { return waiting_time; }

  virtual tmi_boss_e convert_tmi_string( const std::string& tmi_string );
};

// ==========================================================================
// Enemy Add
// ==========================================================================

struct add_t : public pet_t
{
  add_t( sim_t* s, enemy_t* o, const std::string& n, pet_e pt = PET_ENEMY ) :
    pet_t( s, o, n, pt )
  {
    level = default_level + 3;
  }

  virtual void init_action_list()
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/snapshot_stats";
    }

    pet_t::init_action_list();
  }

  double health_percentage() const
  {
    timespan_t remainder = timespan_t::zero();
    timespan_t divisor = timespan_t::zero();
    if ( duration > timespan_t::zero() )
    {
      remainder = expiration -> remains();
      divisor = duration;
    }
    else
    {
      remainder = std::max( timespan_t::zero(), sim -> expected_iteration_time - sim -> current_time );
      divisor = sim -> expected_iteration_time;
    }

    return remainder / divisor * 100.0;
  }

  virtual resource_e primary_resource() const
  { return RESOURCE_HEALTH; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    action_t* a = enemy_create_action( this, name, options_str );

    if ( !a )
      a = pet_t::create_action( name, options_str );

    return a;
  }
};

struct heal_enemy_t : public enemy_t
{
  heal_enemy_t( sim_t* s, const std::string& n, race_e r = RACE_HUMANOID ) :
    enemy_t( s, n, r, HEALING_ENEMY )
  {
    target = this;
    level = default_level; // set to default player level, not default+3
  }

  virtual void init_resources( bool /* force */ )
  {
    resources.base[ RESOURCE_HEALTH ] = 20000000.0;

    player_t::init_resources( true );

    resources.current[ RESOURCE_HEALTH ] = resources.base[ RESOURCE_HEALTH ] / 1.5;
  }
  virtual void init_base_stats()
  {
    enemy_t::init_base_stats();

    collected_data.htps.change_mode( false );

    level = std::min( 90, level );
  }
  virtual resource_e primary_resource() const
  { return RESOURCE_HEALTH; }

};

// enemy_t::create_action ===================================================

action_t* enemy_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  action_t* a = enemy_create_action( this, name, options_str );

  if ( !a )
    a = player_t::create_action( name, options_str );

  return a;
}

// enemy_t::convert_tmi_string ==============================================

tmi_boss_e enemy_t::convert_tmi_string( const std::string& tmi_string )
{
  // this function translates between the "tmi_boss" option string and the tmi_boss_e enum
  // eventually plan on using regular expressions here
  if ( tmi_string == "T15LFR" || tmi_string == "T15N10" )
    return TMI_T15LFR;
  if ( tmi_string == "T15N" || tmi_string == "T15N25" || tmi_string == "T15H10" )
    return TMI_T15N;
  if ( tmi_string == "T15H" || tmi_string == "T15H25" )
    return TMI_T15H;
  if ( tmi_string == "T16Q" || tmi_string == "T16N10" )
    return TMI_T16_10N;
  if ( tmi_string == "T16N" || tmi_string == "T16N25" )
    return TMI_T16_25N;
  if ( tmi_string == "T16H10" )
    return TMI_T16_10H;
  if ( tmi_string == "T16H" || tmi_string == "T16H25" )
    return TMI_T16_25H;
  if ( tmi_string == "T17Q" )
    return TMI_T17_Q;

  if ( ! tmi_string.empty() && sim -> debug )
    sim -> out_debug.printf( "Unknown TMI string input provided: %s", tmi_string.c_str() );

  return TMI_NONE;

}

// enemy_t::init_base =======================================================

void enemy_t::init_base_stats()
{
  player_t::init_base_stats();

  resources.infinite_resource[ RESOURCE_HEALTH ] = false;

  tmi_boss_enum = convert_tmi_string( tmi_boss_str );

  level = sim -> max_player_level + 3;

  if ( tmi_boss_enum == TMI_NONE ) // skip overrides for TMI standard bosses
  {
    // target_level override
    if ( sim -> target_level >= 0 )
      level = sim -> target_level;
    else if ( ( sim -> max_player_level + sim -> rel_target_level ) >= 0 )
      level = sim -> max_player_level + sim -> rel_target_level;

    // waiting_time override
    waiting_time = timespan_t::from_seconds( 5.0 );
    if ( waiting_time < timespan_t::from_seconds( 1.0 ) )
      waiting_time = timespan_t::from_seconds( 1.0 );

    // target_race override
    if ( ! sim -> target_race.empty() )
    {
      race = util::parse_race_type( sim -> target_race );
      race_str = util::race_type_string( race );
    }
  }

  base.attack_crit = 0.05;

  initial_health = ( sim -> overrides.target_health ) ? sim -> overrides.target_health : fixed_health;

  if ( this == sim -> target )
  {
    if ( sim -> overrides.target_health > 0 || fixed_health > 0 ) {
      if ( sim -> debug )
        sim -> out_debug << "Setting vary_combat_length forcefully to 0.0 because main target fixed health simulation was detected.";

      sim -> vary_combat_length = 0.0;
    }
  }

  if ( ( initial_health_percentage < 1   ) ||
       ( initial_health_percentage > 100 ) )
  {
    initial_health_percentage = 100.0;
  }

  // TODO-WOD: Base block?
//  base.miss  = 0.030; //90, level differential handled in miss_chance()
//  base.dodge = 0.030; //90, level differential handled in dodge_chance()
//  base.parry = 0.030; //90, level differential handled in parry_chance()
  base.block = 0.030; //90, level differential handled in block_chance()
  base.block_reduction = 0.3;
}

void enemy_t::init_defense()
{
  player_t::init_defense();

  if ( ( gear.armor + enchant.armor ) <= 0 )
  {
    double& a = initial.stats.armor;

    switch ( level )
    {
      case 90: a = 1047; break; // From DBC Data. Reia will likely replace this with automation magic at some point.
      case 91: a = 1185; break;
      case 92: a = 1342; break;
//      case 93: a = 1518; break;
      case 93: a = 5234; break; // WOD-TODO: Dummy value to not mess up level 90 sims in the interim period of WoD alpha/beta.
      case 94: a = 1718; break;
      case 95: a = 1945; break;
      case 96: a = 2201; break;
      case 97: a = 2419; break;
      case 98: a = 2819; break;
      case 99: a = 3190; break;
      case 100: a = 3610; break;
      case 101: a = 4086; break;
      case 102: a = 4624; break;
      case 103: a = 5234; break;
      default: if ( level < 90 )
          a = ( int ) floor ( ( level * 10 ) + 147 );
        break;
    }
  }
}

// enemy_t::init_buffs ======================================================

void enemy_t::create_buffs()
{
  player_t::create_buffs();

  for ( unsigned int i = 1; i <= 10; ++ i )
    buffs_health_decades.push_back( buff_creator_t( this, "Health Decade (" + util::to_string( ( i - 1 ) * 10 ) + " - " + util::to_string( i * 10 ) + ")" ) );
}

// enemy_t::init_resources ==================================================

void enemy_t::init_resources( bool /* force */ )
{
  double health_adjust = sim -> iteration_time_adjust();

  resources.base[ RESOURCE_HEALTH ] = initial_health * health_adjust;

  player_t::init_resources( true );

  if ( initial_health_percentage > 0 )
  {
    resources.max[ RESOURCE_HEALTH ] *= 100.0 / initial_health_percentage;
  }
}

// enemy_t::init_target =====================================================

void enemy_t::init_target()
{
  if ( ! target_str.empty() )
  {
    target = sim -> find_player( target_str );
  }

  if ( target )
    return;

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* q = sim -> player_list[ i ];
    if ( q -> primary_role() != ROLE_TANK )
      continue;

    target = q;
    break;
  }

  if ( !target )
    target = sim -> target;
}

// enemy_t::init_actions ====================================================

void enemy_t::init_action_list()
{
  if ( ! is_add() && is_enemy() )
  {
    if ( action_list_str.empty() )
    {
      std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;
      precombat_list += "/snapshot_stats";

      double level_mult = sim -> dbc.combat_rating( RATING_BLOCK, sim -> max_player_level ) / sim -> dbc.combat_rating( RATING_BLOCK, 90 );
      level_mult = std::pow( level_mult, 1.5 );
      if ( ! target -> is_enemy() )
      {
        switch ( tmi_boss_enum )
        {
          case TMI_NONE:
            action_list_str += "/auto_attack,damage=" + util::to_string( 15000 * level_mult ) + ",attack_speed=2,aoe_tanks=1";
            action_list_str += "/spell_dot,damage=" + util::to_string( 6000 * level_mult ) + ",tick_time=2,num_ticks=10,cooldown=40,aoe_tanks=1,if=!ticking";
            action_list_str += "/spell_nuke,damage=" + util::to_string( 10000 * level_mult ) + ",cooldown=35,attack_speed=3,aoe_tanks=1";
            action_list_str += "/melee_nuke,damage=" + util::to_string( 16000 * level_mult ) + ",cooldown=27,attack_speed=3,aoe_tanks=1";
            break;
          default:
            // boss damage information ( could move outside this function and make a constant )
            int aa_damage [ 9 ] =  { 0, 5500, 7500, 9000, 12500, 15000, 25000, 45000, 100000 };
            int dot_damage [ 9 ] = { 0,  2700,  3750,  4500,   6250,   10000,   15000,  35000,  60000 };
            action_list_str += "/auto_attack,damage=" + util::to_string( aa_damage[ tmi_boss_enum ] ) + ",attack_speed=1.5";
            action_list_str += "/spell_dot,damage=" + util::to_string( dot_damage[ tmi_boss_enum ] ) + ",tick_time=2,num_ticks=15,aoe_tanks=1,if=!ticking";
        }
      }
      else if ( sim -> heal_target && this != sim -> heal_target )
      {
        unsigned int healers = 0;
        for ( size_t i = 0; i < sim -> player_list.size(); ++i )
          if ( !sim -> player_list[ i ] -> is_pet() && sim -> player_list[ i ] -> primary_role() == ROLE_HEAL )
            ++healers;

        action_list_str += "/auto_attack,damage=" + util::to_string( 20000 * healers * level / 85 ) + ",attack_speed=2.0,target=" + sim -> heal_target -> name_str;
      }
    }
  }
  player_t::init_action_list();
}

// Hack to get this executed after player_t::init_action_list.
void enemy_t::init_stats()
{
  player_t::init_stats();

  // Small hack to increase waiting time for target without any actions
  for ( size_t i = 0; i < action_list.size(); ++i )
  {
    action_t* action = action_list[ i ];
    if ( action -> background ) continue;
    if ( action -> name_str == "snapshot_stats" ) continue;
    if ( action -> name_str.find( "auto_attack" ) != std::string::npos )
      continue;
    waiting_time = timespan_t::from_seconds( 1.0 );
    break;
  }
}

// enemy_t::resource_loss ===================================================

double enemy_t::resource_loss( resource_e resource_type,
                               double    amount,
                               gain_t*   source,
                               action_t* action )
{
  // This mechanic compares pre and post health decade, and if it switches to a lower decade,
  // it triggers the respective trigger buff.
  // Starting decade ( 100% to 90% ) is triggered in combat_begin()

  int pre_health = static_cast<int>( resources.pct( RESOURCE_HEALTH ) * 100 ) / 10;
  double r = player_t::resource_loss( resource_type, amount, source, action );
  int post_health = static_cast<int>( resources.pct( RESOURCE_HEALTH ) * 100 ) / 10;

  if ( pre_health < 10 && pre_health > post_health && post_health >= 0 )
  {
    buffs_health_decades.at( post_health + 1 ) -> expire();
    buffs_health_decades.at( post_health ) -> trigger();
  }

  return r;
}

// enemy_t::create_options ==================================================

void enemy_t::create_options()
{
  option_t target_options[] =
  {
    opt_float( "enemy_health", fixed_health ),
    opt_float( "enemy_initial_health_percentage", initial_health_percentage ),
    opt_float( "enemy_fixed_health_percentage", fixed_health_percentage ),
    opt_float( "health_recalculation_dampening_exponent", health_recalculation_dampening_exponent ),
    opt_float( "enemy_size", size ),
    opt_string( "enemy_tank", target_str ),
    opt_string( "tmi_boss", tmi_boss_str ),
    opt_null()
  };

  option_t::copy( sim -> options, target_options );

  player_t::create_options();
}

// enemy_t::create_add ======================================================

pet_t* enemy_t::create_pet( const std::string& add_name, const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( add_name );
  if ( !p )
    p = new add_t( sim, this, add_name, PET_ENEMY );
  return p;
}

// enemy_t::create_pets =====================================================

void enemy_t::create_pets()
{
  for ( int i = 0; i < sim -> target_adds; i++ )
  {
    create_pet( "add" + util::to_string( i ) );
  }
}

// enemy_t::health_percentage() =============================================

double enemy_t::health_percentage() const
{
  if ( fixed_health_percentage > 0 ) return fixed_health_percentage;

  if ( resources.base[ RESOURCE_HEALTH ] == 0 || sim -> fixed_time ) // first iteration or fixed time sim.
  {
    timespan_t remainder = std::max( timespan_t::zero(), ( sim -> expected_iteration_time - sim -> current_time ) );

    return ( remainder / sim -> expected_iteration_time ) * ( initial_health_percentage - death_pct ) + death_pct;
  }

  return resources.pct( RESOURCE_HEALTH ) * 100 ;
}

// enemy_t::recalculate_health ==============================================

void enemy_t::recalculate_health()
{
  if ( sim -> expected_iteration_time <= timespan_t::zero() || fixed_health > 0 ) return;

  if ( initial_health == 0 ) // first iteration
  {
    initial_health = iteration_dmg_taken * ( sim -> expected_iteration_time / sim -> current_time ) * ( 1.0 / ( 1.0 - death_pct / 100 ) );
  }
  else
  {
    timespan_t delta_time = sim -> current_time - sim -> expected_iteration_time;
    delta_time /= std::pow( ( sim -> current_iteration + 1 ), health_recalculation_dampening_exponent ); // dampening factor, by default 1/n
    double factor = 1.0 - ( delta_time / sim -> expected_iteration_time );

    if ( factor > 1.5 ) factor = 1.5;
    if ( factor < 0.5 ) factor = 0.5;

    if ( sim -> current_time > sim -> expected_iteration_time && this != sim -> target ) // Special case for aoe targets that do not die before fluffy pillow.
      factor = 1;

    initial_health *= factor;
  }

  if ( sim -> debug ) sim -> out_debug.printf( "Target %s initial health calculated to be %.0f. Damage was %.0f", name(), initial_health, iteration_dmg_taken );
}

// enemy_t::create_expression ===============================================

expr_t* enemy_t::create_expression( action_t* action,
                                    const std::string& name_str )
{
  if ( name_str == "adds" )
    return make_ref_expr( name_str, active_pets.size() );

  // override enemy health.pct expression
  if ( name_str == "health.pct" )
    return make_mem_fn_expr( name_str, *this, &enemy_t::health_percentage );

  return player_t::create_expression( action, name_str );
}

// enemy_t::combat_begin ====================================================

void enemy_t::combat_begin()
{
  player_t::combat_begin();

  buffs_health_decades[ 9 ] -> trigger();
}

// enemy_t::combat_end ======================================================

void enemy_t::combat_end()
{
  player_t::combat_end();

  if ( ! sim -> overrides.target_health )
    recalculate_health();
}

void enemy_t::demise()
{
  if ( this == sim -> target )
  {
    if ( sim -> current_iteration != 0 || sim -> overrides.target_health > 0 || fixed_health > 0 )
      // For the main target, end simulation on death.
      sim -> cancel_iteration();
  }

  player_t::demise();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class enemy_report_t : public player_report_extension_t
{
public:
  enemy_report_t( enemy_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  enemy_t& p;
};

// ENEMY MODULE INTERFACE ===================================================

struct enemy_module_t : public module_t
{
  enemy_module_t() : module_t( ENEMY ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e /* r = RACE_NONE */ ) const
  {
    enemy_t* p = new enemy_t( sim, name );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new enemy_report_t( *p ) );
    return p;
  }
  virtual bool valid() const { return true; }
  virtual void init        ( sim_t* ) const {}
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end  ( sim_t* ) const {}
};

// HEAL ENEMY MODULE INTERFACE ==============================================

struct heal_enemy_module_t : public module_t
{
  heal_enemy_module_t() : module_t( ENEMY ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e /* r = RACE_NONE */ ) const
  {
    return new heal_enemy_t( sim, name );
  }
  virtual bool valid() const { return true; }
  virtual void init        ( sim_t* ) const {}
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end  ( sim_t* ) const {}
};

} // END UNNAMED NAMESPACE

const module_t* module_t::enemy()
{
  static enemy_module_t m = enemy_module_t();
  return &m;
}

const module_t* module_t::heal_enemy()
{
  static heal_enemy_module_t m = heal_enemy_module_t();
  return &m;
}

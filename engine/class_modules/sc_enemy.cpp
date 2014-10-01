// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Enemy
// ==========================================================================

namespace { // UNNAMED NAMESPACE

enum boss_type_e
{
  BOSS_NONE = 0,
  BOSS_FLUFFY_PILLOW, BOSS_TMI_STANDARD, BOSS_TANK_DUMMY
};

enum tank_dummy_e
{
  TANK_DUMMY_NONE = 0,
  TANK_DUMMY_WEAK, TANK_DUMMY_DUNGEON, TANK_DUMMY_RAID, TANK_DUMMY_MYTHIC
};

enum tmi_boss_e
{
  TMI_NONE = 0,
  TMI_T15LFR, TMI_T15N, TMI_T15H, TMI_T16_10N, TMI_T16_25N, TMI_T16_10H, TMI_T16_25H, TMI_T17N, TMI_T17H, TMI_T17M
};


struct enemy_t : public player_t
{
  double fixed_health, initial_health;
  double fixed_health_percentage, initial_health_percentage;
  double health_recalculation_dampening_exponent;
  timespan_t waiting_time;
  std::string boss_type_str;
  boss_type_e boss_type_enum;
  std::string tank_dummy_str;
  tank_dummy_e tank_dummy_enum;
  std::string tmi_boss_str;
  tmi_boss_e tmi_boss_enum;

  int current_target;
  int apply_damage_taken_debuff;

  std::vector<buff_t*> buffs_health_decades;

  enemy_t( sim_t* s, const std::string& n, race_e r = RACE_HUMANOID, player_e type = ENEMY ) :
    player_t( s, type, n, r ),
    fixed_health( 0 ), initial_health( 0 ),
    fixed_health_percentage( 0 ), initial_health_percentage( 100.0 ),
    health_recalculation_dampening_exponent( 1.0 ),
    waiting_time( timespan_t::from_seconds( 1.0 ) ),
    boss_type_str( "none" ),
    boss_type_enum( BOSS_NONE ),
    tank_dummy_str( "none" ),
    tank_dummy_enum( TANK_DUMMY_NONE ),
    tmi_boss_str( "none" ),
    tmi_boss_enum( TMI_NONE ),
    current_target( 0 ),
    apply_damage_taken_debuff( 0 )
  {
    s -> target_list.push_back( this );
    position_str = "front";
    level = 0;
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
  virtual std::string fluffy_pillow_action_list();
  virtual std::string tmi_boss_action_list();
  virtual std::string tank_dummy_action_list();
  virtual std::string retrieve_action_list();
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

  virtual boss_type_e convert_boss_type( const std::string& boss_type );
  virtual tmi_boss_e convert_tmi_string( const std::string& tmi_string );
  virtual tank_dummy_e convert_tank_dummy_string( const std::string& tank_dummy_string );

  virtual bool taunt( player_t* source );
};


// Enemy actions are generic to serve both enemy_t and enemy_add_t,
// so they can only rely on player_t and should have no knowledge of class definitions

template <typename ACTION_TYPE>
struct enemy_action_t : public ACTION_TYPE
{
  typedef ACTION_TYPE action_type_t;
  typedef enemy_action_t<ACTION_TYPE> base_t;

  bool apply_debuff;
  std::string dmg_type_override;
  int num_debuff_stacks;
  double damage_range;
  timespan_t cooldown_;
  int aoe_tanks;

  std::vector<option_t> options;

  enemy_action_t( const std::string& name, player_t* player ) :
    action_type_t( name, player ),
    apply_debuff( false ), num_debuff_stacks( -1000000 ), damage_range( -1 ),
    cooldown_( timespan_t::zero() ), aoe_tanks( 0 )
  {
    options.push_back( opt_float( "damage", this -> base_dd_min ) );
    options.push_back( opt_timespan( "attack_speed", this -> base_execute_time ) );
    options.push_back( opt_int( "apply_debuff", num_debuff_stacks ) );
    options.push_back( opt_int( "aoe_tanks", aoe_tanks ) );
    options.push_back( opt_float( "range", damage_range ) );
    options.push_back( opt_timespan( "cooldown", cooldown_ ) );
    options.push_back( opt_string( "type", dmg_type_override ) );
    options.push_back( opt_null() );

    dmg_type_override = "none";
  }

  void add_option( const option_t& new_option )
  {
    size_t i;

    for ( i = 0; i < options.size(); i++ )
    {
      if ( options[ i ].name_cstr() && util::str_compare_ci( options[ i ].name_cstr(), new_option.name_cstr() ) )
      {
        options[ i ] = new_option;
        break;
      }
    }

    if ( i == options.size() )
      options.insert( options.begin(), new_option );
  }

  void init()
  {
    action_type_t::init();

    if ( aoe_tanks == 1 )
      this -> aoe = -1;
    else
      this -> aoe = aoe_tanks;

    this -> name_str = this -> name_str + "_" + this -> target -> name();
    this -> cooldown = this -> player -> get_cooldown( this -> name_str );

    this -> stats = this -> player -> get_stats( this -> name_str, this );
    this -> stats -> school = this -> school;

    if ( cooldown_ > timespan_t::zero() )
      this -> cooldown -> duration = cooldown_;

    // if the debuff increment size is specified in the options string, it takes precedence
    if ( num_debuff_stacks != -1e6 && num_debuff_stacks != 0 )
      apply_debuff = true;

    if ( enemy_t* e = dynamic_cast< enemy_t* >( this -> player ) )
    {
      // if the debuff increment hasn't been specified at all, set it
      if ( num_debuff_stacks == -1e6 )
      {
        if ( e -> apply_damage_taken_debuff == 0 )
          num_debuff_stacks = 0;
        else
        {
          apply_debuff = true;
          num_debuff_stacks = e -> apply_damage_taken_debuff;
        }
      }
    }
    if ( dmg_type_override != "none" )
      this -> school = util::parse_school_type( dmg_type_override );
  }

  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.
    tl.clear();
    tl.push_back( this -> target );

    for ( size_t i = 0, actors = this -> sim -> actor_list.size(); i < actors; i++ )
    {
      player_t* actor = this -> sim -> actor_list[ i ];
      //only add non heal_target tanks to this list for now
      if ( ! actor -> is_sleeping() &&
           ! actor -> is_enemy() &&
           actor -> primary_role() == ROLE_TANK &&
           actor != this -> target &&
           actor != this -> sim -> heal_target )
        tl.push_back( actor );
    }
    //if we have no target (no tank), add the healing target as substitute
    if ( tl.empty() )
    {
      tl.push_back( this -> sim -> heal_target );
    }

    return tl.size();
  }

  double calculate_direct_amount( action_state_t* s )
  {
    // force boss attack size to vary regardless of whether the sim itself does
    int previous_average_range_state = this -> sim -> average_range;
    this -> sim -> average_range = 0;

    double amount = action_type_t::calculate_direct_amount( s );

    this -> sim -> average_range = previous_average_range_state;

    return amount;
  }

  void impact( action_state_t* s )
  {
    if ( apply_debuff && num_debuff_stacks >= 0 )
      this -> target -> debuffs.damage_taken -> trigger( num_debuff_stacks );
    else if ( apply_debuff )
      this -> target -> debuffs.damage_taken -> decrement( - num_debuff_stacks );

    action_type_t::impact( s );
  }

  void tick( dot_t* d )
  {
    if ( apply_debuff && num_debuff_stacks >= 0 )
      this -> target -> debuffs.damage_taken -> trigger( num_debuff_stacks );
    else if ( apply_debuff )
      this -> target -> debuffs.damage_taken -> decrement( - num_debuff_stacks );

    action_type_t::tick( d );
  }
};

// Melee ====================================================================

struct melee_t : public enemy_action_t<melee_attack_t>
{
  bool first;
  melee_t( const std::string& name, player_t* player, const std::string options_str ) :
    base_t( name, player ), first( false )
  {
    school = SCHOOL_PHYSICAL;
    may_crit = true;
    background = true;
    trigger_gcd = timespan_t::zero();
    base_dd_min = 26000;
    base_execute_time = timespan_t::from_seconds( 1.5 );
    repeating = true;

    parse_options( options.data(), options_str );
  }

  void init()
  {
    base_t::init();

    // check that the specified damage range is sane
    if ( damage_range > base_dd_min || damage_range < 0 )
      damage_range = 0.1 * base_dd_min; // if not, set to +/-10%

    // set damage range to mean +/- range
    base_dd_max = base_dd_min + damage_range;
    base_dd_min -= damage_range;

    // if the execute time is somehow less than 10 ms, set it back to the default of 1.5 seconds
    if ( base_execute_time < timespan_t::from_seconds( 0.01 ) )
      base_execute_time = timespan_t::from_seconds( 1.5 );
  }

  timespan_t execute_time() const
  {
    timespan_t et = base_t::execute_time();

    if ( first )
    {
      et += this -> base_execute_time / 2;
    }

    return et;
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public enemy_action_t<attack_t>
{
  melee_t* mh;

  // default constructor
  auto_attack_t( player_t* p, const std::string& options_str ) :
    base_t( "auto_attack", p ), mh( 0 )
  {
    parse_options( options.data(), options_str );

    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    mh = new melee_t( "melee_main_hand", p, options_str );
    mh -> weapon = &( p -> main_hand_weapon );
    if ( ! mh -> target )
      mh -> target = target;

    p -> main_hand_attack = mh;
  }

  void init()
  {
    base_t::init();

    if ( enemy_t* e = dynamic_cast< enemy_t* >( player ) )
    {
      // if the number of debuff stacks hasn't been specified yet, set it
      if ( mh -> num_debuff_stacks == -1e6 )
      {
        if ( e -> apply_damage_taken_debuff == 0 )
          mh -> num_debuff_stacks = 0;
        else
        {
          mh -> apply_debuff = true;
          mh -> num_debuff_stacks = e -> apply_damage_taken_debuff;
        }
      }
    }
  }

  virtual void execute()
  {
    player -> main_hand_attack = mh;
    player -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( player -> is_moving() ) return false;
    return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Auto Attack Off-Hand =======================================================

struct auto_attack_off_hand_t : public enemy_action_t<attack_t>
{
  melee_t* oh;

  // default constructor
  auto_attack_off_hand_t( player_t* p, const std::string& options_str ) :
    base_t( "auto_attack_off_hand", p ), oh( 0 )
  {
    parse_options( options.data(), options_str );
    
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    oh = new melee_t( "melee_off_hand", p, options_str );
    oh -> weapon = &( p -> off_hand_weapon );
    if ( !oh -> target )
      oh -> target = target;

    p -> off_hand_attack = oh;
  }

  void init()
  {
    base_t::init();

    if ( enemy_t* e = dynamic_cast< enemy_t* >( player ) )
    {
      // if the number of debuff stacks hasn't been specified yet, set it
      if ( oh && oh -> num_debuff_stacks == -1e6 )
      {
        if ( e -> apply_damage_taken_debuff == 0 )
          oh -> num_debuff_stacks = 0;
        else
        {
          oh -> apply_debuff = true;
          oh -> num_debuff_stacks = e -> apply_damage_taken_debuff;
        }
      }
    }
  }

  virtual void execute()
  {
    oh -> first = true;
    player -> off_hand_attack = oh;
    player -> off_hand_attack -> schedule_execute();

  }

  virtual bool ready()
  {
    if ( player -> is_moving() ) return false;
    return( player -> off_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Melee Nuke ===============================================================

struct melee_nuke_t : public enemy_action_t<attack_t>
{
  // default constructor
  melee_nuke_t( player_t* p, const std::string& options_str ) :
    base_t( "melee_nuke", p )
  {
    school = SCHOOL_PHYSICAL;
    may_miss = may_dodge = may_parry = false;
    may_block = true;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min = 25000;

    parse_options( options.data(), options_str );
  }

  void init()
  {
    base_t::init();

    // check that the specified damage range is sane
    if ( damage_range > base_dd_min || damage_range < 0 )
      damage_range = 0.1 * base_dd_min; // if not, set to +/-10%

    base_dd_max = base_dd_min + damage_range;
    base_dd_min -= damage_range;

    if ( base_execute_time < timespan_t::zero() )
      base_execute_time = timespan_t::from_seconds( 3.0 );
  }
};

// Spell Nuke ===============================================================

struct spell_nuke_t : public enemy_action_t<spell_t>
{
  spell_nuke_t( player_t* p, const std::string& options_str ) :
    base_t( "spell_nuke", p )
  {
    school            = SCHOOL_FIRE;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min       = 5000;

    parse_options( options.data(), options_str );
  }

  void init()
  {
    base_t::init();

    // check that the specified damage range is sane
    if ( damage_range > base_dd_min || damage_range < 0 )
      damage_range = 0.1 * base_dd_min; // if not, set to +/-10%

    base_dd_max = base_dd_min + damage_range;
    base_dd_min -= damage_range;

    if ( base_execute_time < timespan_t::zero() )
      base_execute_time = timespan_t::from_seconds( 3.0 );
  }
};

// Spell DoT ================================================================

struct spell_dot_t : public enemy_action_t<spell_t>
{
  spell_dot_t( player_t* p, const std::string& options_str ) :
    base_t( "spell_dot", p )
  {
    school         = SCHOOL_FIRE;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    dot_duration   = timespan_t::from_seconds( 10.0 );
    base_td        = 5000;
    tick_may_crit  = false; // FIXME: should ticks crit or not?
    may_crit       = false;

    // Replace damage option
    add_option( opt_float( "damage", base_td ) );
    add_option( opt_timespan( "dot_duration", dot_duration ) );
    add_option( opt_timespan( "tick_time", base_tick_time ) );

    parse_options( options.data(), options_str );
  }

  void init()
  {
    base_t::init();

    if ( base_tick_time < timespan_t::zero() ) // User input sanity check
      base_tick_time = timespan_t::from_seconds( 1.0 );
  }

  virtual void execute()
  {
    target_cache.is_valid = false;

    base_t::execute();
  }
};

// Spell AoE ================================================================

struct spell_aoe_t : public enemy_action_t<spell_t>
{
  // default constructor
  spell_aoe_t( player_t* p, const std::string& options_str ) :
    base_t( "spell_aoe", p )
  {
    school            = SCHOOL_FIRE;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min       = 5000;
    aoe               = -1;
    may_crit          = false;

    parse_options( options.data(), options_str );
  }

  void init()
  {
    base_t::init();

    base_dd_max = base_dd_min;
    if ( base_execute_time < timespan_t::from_seconds( 0.01 ) )
      base_execute_time = timespan_t::from_seconds( 3.0 );
  }

  size_t available_targets( std::vector< player_t* >& tl ) const
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
  if ( name == "auto_attack" )          return new          auto_attack_t( p, options_str );
  if ( name == "auto_attack_off_hand" ) return new auto_attack_off_hand_t( p, options_str );
  if ( name == "melee_nuke"  )          return new           melee_nuke_t( p, options_str );
  if ( name == "spell_nuke"  )          return new           spell_nuke_t( p, options_str );
  if ( name == "spell_dot"   )          return new            spell_dot_t( p, options_str );
  if ( name == "spell_aoe"   )          return new            spell_aoe_t( p, options_str );
  if ( name == "summon_add"  )          return new           summon_add_t( p, options_str );

  return NULL;
}
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

//  void init_action_list()
//  { }
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

// enemy_t::convert_boss_type ===========================================
boss_type_e enemy_t::convert_boss_type( const std::string& boss_type )
{
  if ( util::str_compare_ci( boss_type, "none" ) )
    return BOSS_NONE;
  if ( util::str_compare_ci( boss_type, "tank_dummy" ) )
    return BOSS_TANK_DUMMY;
  if ( util::str_compare_ci( boss_type, "tmi_standard_boss" ) )
    return BOSS_TMI_STANDARD;
  if ( util::str_compare_ci( boss_type, "fluffy_pillow" ) || util::str_compare_ci( boss_type, "none" ) )
    return BOSS_FLUFFY_PILLOW;

  if ( ! boss_type.empty() && sim -> debug )
    sim -> out_debug.printf( "Unknown boss category string input provided: %s", boss_type.c_str() );

  return BOSS_NONE;
}

// enemy_t::convert_tmi_string ==============================================

tmi_boss_e enemy_t::convert_tmi_string( const std::string& tmi_string )
{
  // this function translates between the "tmi_boss" option string and the tmi_boss_e enum
  // eventually plan on using regular expressions here
  if ( util::str_compare_ci( tmi_string, "none" ) )
    return TMI_NONE;
  if ( util::str_compare_ci( tmi_string, "T15LFR" ) || util::str_compare_ci( tmi_string, "T15N10" ) )
    return TMI_T15LFR;
  if ( util::str_compare_ci( tmi_string, "T15N" ) || util::str_compare_ci( tmi_string, "T15N25" ) || util::str_compare_ci( tmi_string, "T15H10" ) )
    return TMI_T15N;
  if ( util::str_compare_ci( tmi_string, "T15H" ) || util::str_compare_ci( tmi_string, "T15H25" ) )
    return TMI_T15H;
  if ( util::str_compare_ci( tmi_string, "T16Q" ) || util::str_compare_ci( tmi_string, "T16N10" ) )
    return TMI_T16_10N;
  if ( util::str_compare_ci( tmi_string, "T16N" ) || util::str_compare_ci( tmi_string, "T16N25" ) )
    return TMI_T16_25N;
  if ( util::str_compare_ci( tmi_string, "T16H10" ) )
    return TMI_T16_10H;
  if ( util::str_compare_ci( tmi_string, "T16H" ) || util::str_compare_ci( tmi_string, "T16H25" ) )
    return TMI_T16_25H;
  if ( util::str_compare_ci( tmi_string, "T17N" ) )
    return TMI_T17N;
  if ( util::str_compare_ci( tmi_string, "T17H" ) )
    return TMI_T17H;
  if ( util::str_compare_ci( tmi_string, "T17M" ) )
    return TMI_T17M;

  if ( ! tmi_string.empty() && sim -> debug )
    sim -> out_debug.printf( "Unknown TMI string input provided: %s", tmi_string.c_str() );

  return TMI_NONE;

}

// enemy_t::convert_tank_dummy_string =========================================

tank_dummy_e enemy_t::convert_tank_dummy_string( const std::string& tank_dummy_string )
{
  if ( util::str_compare_ci( tank_dummy_string, "none" ) )
    return TANK_DUMMY_NONE;
  if ( util::str_compare_ci( tank_dummy_string, "weak" ) )
    return TANK_DUMMY_WEAK;
  if ( util::str_compare_ci( tank_dummy_string, "dungeon" ) )
    return TANK_DUMMY_DUNGEON;
  if ( util::str_compare_ci( tank_dummy_string, "raid" ) )
    return TANK_DUMMY_RAID;
  if ( util::str_compare_ci( tank_dummy_string, "mythic" ) )
    return TANK_DUMMY_MYTHIC;

  if ( ! tank_dummy_string.empty() &&  sim -> debug )
    sim -> out_debug.printf( "Unknown Tank Dummy string input provided: %s", tank_dummy_string.c_str() );

  return TANK_DUMMY_NONE;
}

// enemy_t::init_base =======================================================

void enemy_t::init_base_stats()
{
  player_t::init_base_stats();

  resources.infinite_resource[ RESOURCE_HEALTH ] = false;

  // check if the user has specified a boss type via the gui or cli
  boss_type_enum = convert_boss_type( boss_type_str );

  // logic to determine appropriate boss type. TMI bosses take precedence over Tank dummies
  if ( boss_type_enum == BOSS_TMI_STANDARD || boss_type_enum == BOSS_NONE )
  {
    tmi_boss_enum = convert_tmi_string( tmi_boss_str );
    if ( tmi_boss_enum != TMI_NONE )
      boss_type_enum = BOSS_TMI_STANDARD;
  }
  if ( boss_type_enum == BOSS_TANK_DUMMY || boss_type_enum == BOSS_NONE )
  {
    tank_dummy_enum = convert_tank_dummy_string( tank_dummy_str );
    if ( tank_dummy_enum != TANK_DUMMY_NONE )
      boss_type_enum = BOSS_TANK_DUMMY;
  }
  

  if ( level == 0 )
    level = sim -> max_player_level + 3;

  // skip overrides for TMI standard bosses and raid dummies
  if ( boss_type_enum == BOSS_TMI_STANDARD || boss_type_enum == BOSS_TANK_DUMMY ) 
  {
    // target_level override
    if ( sim -> target_level >= 0 )
      level = sim -> target_level;
    else if ( sim -> rel_target_level > 0 && ( sim -> max_player_level + sim -> rel_target_level ) >= 0 )
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
  else if ( boss_type_enum == BOSS_TANK_DUMMY )
  {
    switch ( tank_dummy_enum )
    {
      case TANK_DUMMY_DUNGEON:
        level = 102; break;
      case TANK_DUMMY_WEAK:
        level = 100; break;
      default:
        level = 103;
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
}

void enemy_t::init_defense()
{
  player_t::init_defense();

  collected_data.health_changes_tmi.collect = false;
  collected_data.health_changes.collect = false;

  if ( ( gear.armor + enchant.armor ) <= 0 )
  {
    double& a = initial.stats.armor;

    // a wild equation appears. It's super effective.
    if ( level < 100 )
      a = std::floor( 0.006464588162215 * std::exp( 0.123782410252464 * level ) + 0.5 );
    else
      a = 134*level-11864;
  }
    
  // for future reference, the equations above fit the given values 
  // in the first colum table below. These numbers are magically accurate.
  // Level  P/W/R   Mage
  //   90     445    403
  //   91     504    457
  //   92     571    517
  //   93     646    585
  //   94     731    662
  //   95     827    749
  //   96     936    847
  //   97    1059    959
  //   98    1199   1086
  //   99    1357   1129
  //   100   1536   1336
  //   101   1670   1443
  //   102   1804   1550
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
  {
    current_target = (int) target -> actor_index;
    return;
  }

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

  current_target = (int) target -> actor_index;
}

// enemy_t::init_actions ====================================================

std::string enemy_t::fluffy_pillow_action_list()
{
  std::string als = "";
  double level_mult = sim -> dbc.combat_rating( RATING_BLOCK, sim -> max_player_level ) / sim -> dbc.combat_rating( RATING_BLOCK, 100 );
  level_mult = std::pow( level_mult, 1.5 );

  // this is the standard Fluffy Pillow action list
  als += "/auto_attack,damage=" + util::to_string( 100000 * level_mult ) + ",attack_speed=2,aoe_tanks=1";
  als += "/spell_dot,damage=" + util::to_string( 60000 * level_mult ) + ",tick_time=2,dot_duration=20,cooldown=40,aoe_tanks=1,if=!ticking";
  als += "/spell_nuke,damage=" + util::to_string( 100000 * level_mult ) + ",cooldown=35,attack_speed=3,aoe_tanks=1";
  als += "/melee_nuke,damage=" + util::to_string( 200000 * level_mult ) + ",cooldown=27,attack_speed=3,aoe_tanks=1";
  
  return als;
}

std::string enemy_t::tmi_boss_action_list()
{
  // Bosses are (roughly) standardized based on content level. dot damage is 2/15 of melee damage (0.1333 multiplier)
  std::string als = "";
  const int num_bosses = 11;
  int aa_damage[ num_bosses ] = { 0, 5500, 7500, 9000, 12500, 15000, 25000, 45000, 100000, 150000, 200000 };

  als += "/auto_attack,damage=" + util::to_string( aa_damage[ tmi_boss_enum ] ) + ",attack_speed=1.5,aoe_tanks=1";
  als += "/spell_dot,damage=" + util::to_string( aa_damage[ tmi_boss_enum ] * 2 / 15 ) + ",tick_time=2,dot_duration=30,aoe_tanks=1,if=!ticking";

  return als;
}

std::string enemy_t::tank_dummy_action_list()
{
  std::string als = "";
  int aa_damage[ 5 ] = { 0, 5000, 37500, 125000, 175000 };
  int aa_damage_var[ 5 ] = { 0, 0, 7500, 25000, 35000 };
  int dummy_strike_damage[ 5 ] = { 0, 0, 50, 50, 50 }; // % weapon damage multipliers for dummy_strike
  int uber_strike_damage[ 5 ] = { 0, 0, 0, 0, 50 }; // % weapon damage multipliers for uber_strike

  als += "/auto_attack,damage=" + util::to_string( aa_damage[ tank_dummy_enum ] ) + ",range=" + util::to_string( aa_damage_var[ tank_dummy_enum ] ) + ",attack_speed=1.5,aoe_tanks=1";
  if ( tank_dummy_enum > TANK_DUMMY_WEAK )
    als += "/melee_nuke,damage=" + util::to_string( aa_damage[ tank_dummy_enum] * dummy_strike_damage[ tank_dummy_enum ] / 100 ) + ",range=" + util::to_string( aa_damage_var[ tank_dummy_enum ] * dummy_strike_damage[ tank_dummy_enum ] / 100 ) + "attack_speed=0,cooldown=6,aoe_tanks=1";
  if ( tank_dummy_enum > TANK_DUMMY_RAID )
    als += "/spell_nuke,damage=" + util::to_string( aa_damage[ tank_dummy_enum] * uber_strike_damage[ tank_dummy_enum ] / 100 ) + ",range=" + util::to_string( aa_damage_var[ tank_dummy_enum ] * uber_strike_damage[ tank_dummy_enum ] / 100 ) + "attack_speed=1,cooldown=10,aoe_tanks=1,apply_debuff=5";

  return als;
}

std::string enemy_t::retrieve_action_list()
{
  switch ( boss_type_enum )
  {
  // Fluffy Pillow
  case BOSS_TMI_STANDARD:
    return tmi_boss_action_list();
  case BOSS_TANK_DUMMY:
    return tank_dummy_action_list();
  default:
    return fluffy_pillow_action_list();
  }  
}

void enemy_t::init_action_list()
{
  if ( ! is_add() && is_enemy() )
  {
    // If the action list string is empty, automatically populate it 
    if ( action_list_str.empty() )
    {
      std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;
      precombat_list += "/snapshot_stats";

      // If targeting an player, use Fluffy Pillow or TMI boss as appropriate
      if ( ! target -> is_enemy() )
        action_list_str += retrieve_action_list();
      
      // Otherwise just auto-attack the heal target
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

  /* If we have more than one tank in the simulation, we do some fancy stuff.
     We take the default APL and clone it into a new APL for each tank, appending 
     a "target=Tank_Name" to each ability. Then the default APL is replaced with
     a series of /run_action_list entries at the end. This is how we support tank swaps.
  */
  // only do this for enemies targeting players
  if ( sim -> enable_taunts && ! target -> is_enemy() && this != sim -> heal_target )
  {
    // count the number of tanks in the simulation
    std::vector<player_t*> tanks;
    for ( size_t i = 0, players = sim -> player_list.size(); i < players; i++ )
    {
      player_t* q = sim -> player_list[ i ];
      if ( q -> primary_role() == ROLE_TANK )
        tanks.push_back( q );
    }

    // If we have more than one tank, create a new action list for each
    // Only do this if the user hasn't specified additional action lists beyond precombat & default
    if ( tanks.size() > 1 && action_priority_list.size() < 3 )
    {
      std::string new_action_list_str = "";

      // split the action_list_str into individual actions so we can modify each later
      std::vector<std::string> splits = util::string_split( action_list_str, "/" );

      for ( size_t i = 0; i < tanks.size(); i++ )
      {
        // create a new APL sub-entry with the name of the tank in question
        std::string& tank_str = get_action_priority_list( tanks[ i ] -> name_str ) -> action_list_str;

        // Reconstruct the action_list_str for this tank by appending ",target=Tank_Name"
        // to each action if it doesn't already specify a different target
        for ( size_t j = 0, jmax = splits.size(); j < jmax; j++ )
        {
          tank_str += "/" + splits[ j ];

          if ( !util::str_in_str_ci( "target=", splits[ j ] ) )
            tank_str += ",target=" + tanks[ i ] -> name_str;
        }

        // add a /run_action_list line to the new default APL
        new_action_list_str += "/run_action_list,name=" + tanks[ i ] -> name_str + ",if=current_target=" + util::to_string( tanks[ i ] -> actor_index );
      }

      // finish off the default action list with an instruction to run the default target's APL
      if ( !target -> is_enemy() )
        new_action_list_str += "/run_action_list,name=" + target -> name_str;
      else
        new_action_list_str += "/run_action_list,name=" + tanks[ 0 ] -> name_str;

      // replace the default APL with our new one.
      action_list_str = new_action_list_str;
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
  // this first part handles enemy options that are sim-wide
  option_t target_options[] =
  {
    opt_float( "enemy_health", fixed_health ),
    opt_float( "enemy_initial_health_percentage", initial_health_percentage ),
    opt_float( "enemy_fixed_health_percentage", fixed_health_percentage ),
    opt_float( "health_recalculation_dampening_exponent", health_recalculation_dampening_exponent ),
    opt_float( "enemy_size", size ),
    opt_string( "enemy_tank", target_str ),
    opt_int( "apply_debuff", apply_damage_taken_debuff ),
    opt_null()
  };

  option_t::copy( sim -> options, target_options );

  // the next part handles actor-specific options for enemies
  player_t::create_options();

  option_t enemy_options[] = 
  {
    opt_string( "boss_type", boss_type_str ),
    opt_string( "tank_dummy", tank_dummy_str ),
    opt_string( "tmi_boss", tmi_boss_str ),
    opt_null()
  };

  option_t::copy( options, enemy_options );

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

bool enemy_t::taunt( player_t* source )
{ 
  current_target = (int) source -> actor_index;
  if ( main_hand_attack && main_hand_attack -> execute_event ) 
    core_event_t::cancel( main_hand_attack -> execute_event );  
  if ( off_hand_attack && off_hand_attack -> execute_event )
    core_event_t::cancel( off_hand_attack -> execute_event );

  return true; 
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

  // current target (for tank/taunting purposes)
  if ( name_str == "current_target" )
    return make_ref_expr( name_str, current_target );

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits[ 0 ] == "current_target" )
  {
    if ( splits.size() == 2 && splits[ 1 ] == "name" )
    {
      struct current_target_name_expr_t : public expr_t
      {
        enemy_t* boss;

        current_target_name_expr_t( enemy_t* e ) :
          expr_t( "current_target_name" ), boss( e )
        {}

        double evaluate()
        {
          return (double) boss -> sim -> actor_list[ boss -> current_target ] -> actor_index;
        }
      };

      return new current_target_name_expr_t( this );

    }
    else if ( splits.size() == 3 && splits[ 1 ] == "debuff" )
    {
      struct current_target_debuff_expr_t : public expr_t
      {
        enemy_t* boss;
        std::string debuff_str;

        current_target_debuff_expr_t( enemy_t* e, const std::string& debuff_str ) :
          expr_t( "current_target_debuff" ), boss( e ), debuff_str( debuff_str )
        {}

        double evaluate()
        {
          if ( debuff_str == "damage_taken" )
            return boss -> sim -> actor_list[ boss -> current_target ] -> debuffs.damage_taken -> current_stack;
          else if ( debuff_str == "vulnerable" )
            return boss -> sim -> actor_list[ boss -> current_target ] -> debuffs.vulnerable -> current_stack;
          else if ( debuff_str == "mortal_wounds" )
            return boss -> sim -> actor_list[ boss -> current_target ] -> debuffs.mortal_wounds -> current_stack;
          // may add others here as desired
          else
            return 0;
        }
      };

      return new current_target_debuff_expr_t( this, splits[ 2 ] );

    }

  }

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
    ( void ) p;
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
  heal_enemy_module_t() : module_t( HEALING_ENEMY ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e /* r = RACE_NONE */ ) const
  {
    heal_enemy_t* p = new heal_enemy_t( sim, name );
    return p;
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

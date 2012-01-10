// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Enemy
// ==========================================================================

struct enemy_t : public player_t
{
  double fixed_health, initial_health;
  double fixed_health_percentage, initial_health_percentage;
  double waiting_time;

  enemy_t( sim_t* s, const std::string& n, race_type r = RACE_HUMANOID ) :
    player_t( s, ENEMY, n, r ),
    fixed_health( 0 ), initial_health( 0 ),
    fixed_health_percentage( 0 ), initial_health_percentage( 100.0 ),
    waiting_time( 1.0 )

  {
    player_t** last = &( sim -> target_list );
    while ( *last ) last = &( ( *last ) -> next );
    *last = this;
    next = 0;

    create_talents();
    create_glyphs();
    create_options();
  }

// target_t::combat_begin ===================================================

  virtual void combat_begin()
  {
    player_t::combat_begin();

    if ( sim -> overrides.bleeding ) debuffs.bleeding -> override();
  }

// target_t::primary_role ===================================================

  virtual int primary_role() const
  {
    return ROLE_TANK;
  }

  virtual int primary_resource() const
  {
    return RESOURCE_NONE;
  }

// target_t::base_armor =====================================================

  virtual double base_armor() const
  {
    return armor;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual void init();
  virtual void init_base();
  virtual void init_resources( bool force=false );
  virtual void init_target();
  virtual void init_actions();
  virtual double composite_tank_block() const;
  virtual void create_options();
  virtual pet_t* create_pet( const std::string& add_name, const std::string& pet_type = std::string() );
  virtual void create_pets();
  virtual pet_t* find_pet( const std::string& add_name );
  virtual double health_percentage() const;
  virtual void combat_end();
  virtual void recalculate_health();
  virtual action_expr_t* create_expression( action_t* action, const std::string& type );
  virtual double available() const { return waiting_time; }
};

// ==========================================================================
// Enemy Add
// ==========================================================================

struct enemy_add_t : public pet_t
{
  enemy_add_t( sim_t* s, player_t* o, const std::string& n, pet_type_t pt = PET_ENEMY ) :
    pet_t( s, o, n, pt )
  {
    type = ENEMY_ADD;
    create_options();
  }

  virtual void init_actions()
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/snapshot_stats";
    }

    pet_t::init_actions();
  }

  virtual int primary_resource() const
  {
    return RESOURCE_HEALTH;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// Melee ====================================================================

struct melee_t : public attack_t
{
  melee_t( const char* name, player_t* player ) :
    attack_t( name, player, RESOURCE_MANA, SCHOOL_PHYSICAL )
  {
    may_crit    = true;
    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;
    base_dd_min = 260000;
    base_execute_time = 2.4;
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public attack_t
{
  auto_attack_t( player_t* p, const std::string& options_str ) :
    attack_t( "auto_attack", p, RESOURCE_MANA, SCHOOL_PHYSICAL )
  {
    p -> main_hand_attack = new melee_t( "melee_main_hand", player );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = 2.4;

    option_t options[] =
    {
      { "damage",       OPT_FLT, &p -> main_hand_attack -> base_dd_min       },
      { "attack_speed", OPT_FLT, &p -> main_hand_attack -> base_execute_time },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    p -> main_hand_attack -> target = target;

    p -> main_hand_attack -> base_dd_max = p -> main_hand_attack -> base_dd_min;
    if ( p -> main_hand_attack -> base_execute_time < 0.01 )
      p -> main_hand_attack -> base_execute_time = 2.4;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );
    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    enemy_t* p = player -> cast_enemy();
    p -> main_hand_attack -> schedule_execute();
    if ( p -> off_hand_attack )
    {
      p -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    enemy_t* p = player -> cast_enemy();
    if ( p -> is_moving() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Spell Nuke ===============================================================

struct spell_nuke_t : public spell_t
{
  spell_nuke_t( player_t* p, const std::string& options_str ) :
    spell_t( "spell_nuke", p, RESOURCE_MANA, SCHOOL_FIRE )
  {
    base_execute_time = 3.0;
    base_dd_min = 50000;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );

    option_t options[] =
    {
      { "damage",       OPT_FLT, &base_dd_min          },
      { "attack_speed", OPT_FLT, &base_execute_time    },
      { "cooldown",     OPT_FLT, &cooldown -> duration },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_dd_max = base_dd_min;
    if ( base_execute_time < 0.00 )
      base_execute_time = 3.0;

    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();

    may_crit = false;
  }
};

// Spell AoE ================================================================

struct spell_aoe_t : public spell_t
{
  spell_aoe_t( player_t* p, const std::string& options_str ) :
    spell_t( "spell_aoe", p, RESOURCE_MANA, SCHOOL_FIRE )
  {
    base_execute_time = 3.0;
    base_dd_min = 50000;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );

    option_t options[] =
    {
      { "damage",       OPT_FLT, &base_dd_min          },
      { "cast_time", OPT_FLT, &base_execute_time    },
      { "cooldown",     OPT_FLT, &cooldown -> duration },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_dd_max = base_dd_min;
    if ( base_execute_time < 0.01 )
      base_execute_time = 3.0;

    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();

    may_crit = false;
  }
};

// Summon Add ===============================================================

struct summon_add_t : public spell_t
{
  std::string add_name;
  double summoning_duration;
  pet_t* pet;

  summon_add_t( player_t* player, const std::string& options_str ) :
    spell_t( "summon_add", player, RESOURCE_MANA, SCHOOL_PHYSICAL ),
    add_name( "" ), summoning_duration( 0 ), pet( 0 )
  {
    option_t options[] =
    {
      { "name",     OPT_STRING, &add_name             },
      { "duration", OPT_FLT,    &summoning_duration   },
      { "cooldown", OPT_FLT,    &cooldown -> duration },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    enemy_t* p = player -> cast_enemy();

    pet = p -> find_pet( add_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p -> name(), add_name.c_str() );
      sim -> cancel();
    }

    harmful = false;

    trigger_gcd = 1.5;
  }

  virtual void execute()
  {
    enemy_t* p = player -> cast_enemy();

    spell_t::execute();

    p -> summon_pet( add_name.c_str(), summoning_duration );
  }

  virtual bool ready()
  {
    if ( ! pet -> sleeping )
      return false;

    return spell_t::ready();
  }
};
}

// ==========================================================================
// Enemy Extensions
// ==========================================================================

// enemy_t::create_action ===================================================

action_t* enemy_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if ( name == "auto_attack" ) return new auto_attack_t( this, options_str );
  if ( name == "spell_nuke"  ) return new  spell_nuke_t( this, options_str );
  if ( name == "spell_aoe"   ) return new   spell_aoe_t( this, options_str );
  if ( name == "summon_add"  ) return new  summon_add_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// enemy_t::init ============================================================

void enemy_t::init()
{
  level = sim -> max_player_level + 3;

  if ( sim -> target_level >= 0 )
    level = sim -> target_level;

  player_t::init();
}

// enemy_t::init_base =======================================================

void enemy_t::init_base()
{
  waiting_time = std::min( ( int ) floor( sim -> max_time ), sim -> wheel_seconds );
  if ( waiting_time < 1.0 )
    waiting_time = 1.0;

  health_per_stamina = 10;

  base_attack_crit = 0.05;

  if ( initial_armor <= 0 )
  {
    // TO-DO: Fill in the blanks.
    // For level 80+ at least it seems to pretty much follow a trend line of: armor = 280.26168*level - 12661.51713
    switch ( level )
    {
    case 80: initial_armor = 9729; break;
    case 81: initial_armor = 10034; break;
    case 82: initial_armor = 10338; break;
    case 83: initial_armor = 10643; break;
    case 84: initial_armor = 10880; break; // Need real value
    case 85: initial_armor = 11092; break;
    case 86: initial_armor = 11387; break;
    case 87: initial_armor = 11682; break;
    case 88: initial_armor = 11977; break;
    default: if ( level < 80 )
        initial_armor = ( int ) floor ( ( level / 80.0 ) * 9729 ); // Need a better value here.
      break;
    }
  }
  player_t::base_armor = initial_armor;

  initial_health = fixed_health;

  if ( ( initial_health_percentage < 1   ) ||
       ( initial_health_percentage > 100 ) )
  {
    initial_health_percentage = 100.0;
  }
}

// enemy_t::init_resources ==================================================

void enemy_t::init_resources( bool /* force */ )
{
  double health_adjust = 1.0 + sim -> vary_combat_length * sim -> iteration_adjust();

  resource_base[ RESOURCE_HEALTH ] = initial_health * health_adjust;

  player_t::init_resources( true );

  if ( initial_health_percentage > 0 )
  {
    resource_max[ RESOURCE_HEALTH ] *= 100.0 / initial_health_percentage;
  }
}

// enemy_t::init_target ====================================================

void enemy_t::init_target()
{
  if ( ! target_str.empty() )
  {
    target = sim -> find_player( target_str );
  }

  if ( target )
    return;

  for ( player_t* q = sim -> player_list; q; q = q -> next )
  {
    if ( q -> primary_role() != ROLE_TANK )
      continue;
    target = q;
    break;
  }

  if ( ! target )
  {
    target = sim -> target;
  }
}

// enemy_t::init_actions ====================================================

void enemy_t::init_actions()
{
  if ( !is_add() )
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/snapshot_stats";

      if ( ! is_add() && target != this && !is_enemy() )
      {
        action_list_str += "/auto_attack";
        action_list_str += "/spell_nuke,damage=6000,cooldown=4,attack_speed=0.1";
      }
    }
  }
  player_t::init_actions();

  // Small hack to increase waiting time for target without any actions
  for ( action_t* action = action_list; action; action = action -> next )
  {
    if ( action -> background ) continue;
    if ( action -> name_str == "snapshot_stats" ) continue;
    if ( action -> name_str.find( "auto_attack" ) != std::string::npos )
      continue;
    waiting_time = 1.0;
    break;
  }
}

// enemy_t::composite_tank_block ============================================

double enemy_t::composite_tank_block() const
{
  double b = player_t::composite_tank_block();

  b += 0.05;

  return b;
}

// enemy_t::create_options ==================================================

void enemy_t::create_options()
{
  option_t target_options[] =
  {
    { "target_health",                    OPT_FLT,    &( fixed_health                      ) },
    { "target_initial_health_percentage", OPT_FLT,    &( initial_health_percentage         ) },
    { "target_fixed_health_percentage",   OPT_FLT,    &( fixed_health_percentage           ) },
    { "target_tank",                      OPT_STRING, &( target_str                        ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( sim -> options, target_options );

  player_t::create_options();
}

// enemy_t::create_add ======================================================

pet_t* enemy_t::create_pet( const std::string& add_name, const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( add_name );
  if ( p ) return p;

  return new enemy_add_t( sim, this, add_name, PET_ENEMY );

  return 0;
}

// enemy_t::create_pets =====================================================

void enemy_t::create_pets()
{
  for ( int i=0; i < sim -> target_adds; i++ )
  {
    char buffer[ 1024 ];
    snprintf( buffer, sizeof( buffer ), "Add %i", i );

    create_pet( buffer );
  }
}

// enemy_t::find_add ========================================================

pet_t* enemy_t::find_pet( const std::string& add_name )
{
  for ( pet_t* p = pet_list; p; p = p -> next_pet )
    if ( p -> name_str == add_name )
      return p;

  return 0;
}

// enemy_t::health_percentage() =============================================

double enemy_t::health_percentage() const
{
  if ( fixed_health_percentage > 0 ) return fixed_health_percentage;

  if ( resource_base[ RESOURCE_HEALTH ] == 0 ) // first iteration
  {
    double remainder = std::max( 0.0, ( sim -> expected_time - sim -> current_time ) );

    return ( remainder / sim -> expected_time ) * ( initial_health_percentage - sim -> target_death_pct ) + sim ->  target_death_pct;
  }

  return resource_current[ RESOURCE_HEALTH ] / resource_max[ RESOURCE_HEALTH ] * 100 ;
}

// enemy_t::recalculate_health ==============================================

void enemy_t::recalculate_health()
{
  if ( sim -> expected_time <= 0 || fixed_health > 0 ) return;

  if ( initial_health == 0 ) // first iteration
  {
    initial_health = iteration_dmg_taken * ( sim -> expected_time / sim -> current_time );
  }
  else
  {
    double delta_time = sim -> current_time - sim -> expected_time;
    delta_time /= ( sim -> current_iteration + 1 ); // dampening factor
    double factor = 1.0 - ( delta_time / sim -> expected_time );

    if ( factor > 1.5 ) factor = 1.5;
    if ( factor < 0.5 ) factor = 0.5;

    initial_health *= factor;
  }

  if ( sim -> debug ) log_t::output( sim, "Target %s initial health calculated to be %.0f. Damage was %.0f", name(), initial_health, iteration_dmg_taken );
}

// enemy_t::create_expression ===============================================

action_expr_t* enemy_t::create_expression( action_t* action,
    const std::string& name_str )
{
  if ( name_str == "adds" )
  {
    struct target_adds_expr_t : public action_expr_t
    {
      player_t* target;
      target_adds_expr_t( action_t* a, player_t* t ) :
        action_expr_t( a, "target_adds", TOK_NUM ), target( t ) {}
      virtual int evaluate() { result_num = target -> active_pets;  return TOK_NUM; }
    };
    return new target_adds_expr_t( action, this );
  }

  return player_t::create_expression( action, name_str );
}

// enemy_t::combat_end ======================================================

void enemy_t::combat_end()
{
  player_t::combat_end();

  recalculate_health();
}

// ==========================================================================
// Enemy Add Extensions
// ==========================================================================

action_t* enemy_add_t::create_action( const std::string& name,
                                      const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "spell_nuke"              ) return new               spell_nuke_t( this, options_str );

  return pet_t::create_action( name, options_str );
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_enemy ===================================================

player_t* player_t::create_enemy( sim_t* sim, const std::string& name, race_type /* r */ )
{
  return new enemy_t( sim, name );
}

// player_t::enemy_init =====================================================

void player_t::enemy_init( sim_t* /* sim */ )
{

}

// player_t::enemy_combat_begin =============================================

void player_t::enemy_combat_begin( sim_t* /* sim */ )
{

}

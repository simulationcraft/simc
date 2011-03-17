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
  double fixed_health, initial_health, max_health, initial_health_percentage, fixed_health_percentage;
  double total_dmg;
// enemy_t::enemy_t =======================================================

enemy_t( sim_t* s, const std::string& n, race_type r = RACE_HUMANOID ) :
    player_t( s, ENEMY, n, r ),
    fixed_health( 0 ), initial_health( 0 ), max_health( 0 ),
    initial_health_percentage( 100.0 ), fixed_health_percentage( 0 ),
    total_dmg( 0.0 )

{
  player_t** last = &( sim -> target_list );
  while ( *last ) last = &( ( *last ) -> next );
  *last = this;
  next = 0;

  create_talents();
  create_glyphs();
  create_options();

}
// target_t::combat_begin ====================================================

virtual void combat_begin()
{
  player_t::combat_begin();

  if ( sim -> overrides.bleeding ) debuffs.bleeding -> override();
}

// target_t::primary_role ====================================================

virtual int primary_role() SC_CONST
{
  return ROLE_TANK;
}

virtual int primary_resource() SC_CONST
{
  return RESOURCE_HEALTH;
}

// target_t::base_armor ======================================================

virtual double base_armor() SC_CONST
{
  return armor;
}

virtual action_t* create_action( const std::string& name, const std::string& options_str );
virtual void init_base();
virtual void init_actions();
virtual double composite_tank_block() SC_CONST;
virtual void create_options();
virtual pet_t* create_pet( const std::string& add_name, const std::string& pet_type );
virtual pet_t* find_pet( const std::string& add_name );
virtual void recalculate_health();
virtual action_expr_t* create_expression( action_t* action, const std::string& type );
virtual void reset();
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

  virtual int primary_resource() SC_CONST
  {
    return RESOURCE_HEALTH;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
};


namespace   // ANONYMOUS NAMESPACE ==========================================
{
struct auto_attack_t : public attack_t
{
  auto_attack_t( player_t* p, const std::string& options_str ) :
      attack_t( "auto_attack", p, RESOURCE_MANA, SCHOOL_PHYSICAL )
  {
    base_execute_time = 2.0;
    base_dd_min = 120000;

    option_t options[] =
    {
      { "damage",       OPT_FLT, &base_dd_min       },
      { "attack_speed", OPT_FLT, &base_execute_time },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_dd_max = base_dd_min;
    if ( base_execute_time < 0.01 )
      base_execute_time = 2.0;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );
    stats = player -> get_stats( name_str + "_" + target -> name() );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();

    may_crit = true;

    weapon = &( player -> main_hand_weapon );
  }
};

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
    if ( base_execute_time < 0.01 )
      base_execute_time = 3.0;


    stats = player -> get_stats( name_str + "_" + target -> name() );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();

    may_crit = true;

  }
};

struct summon_add_t : public spell_t
{
  std::string add_name;
  double summoning_duration;
  pet_t* pet;

  summon_add_t( player_t* player, const std::string& options_str ) :
      spell_t( "summon_add", player, RESOURCE_MANA, SCHOOL_PHYSICAL ),
      add_name(""), summoning_duration(0), pet(0)
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

action_t* enemy_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "spell_nuke"              ) return new               spell_nuke_t( this, options_str );
  if ( name == "summon_add"              ) return new               summon_add_t( this, options_str );


  return player_t::create_action( name, options_str );
}

void enemy_t::init_base()
{
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
    case 85: initial_armor = 11161; break; // Need real value
    case 86: initial_armor = 11441; break; // Need real value
    case 87: initial_armor = 11682; break;
    case 88: initial_armor = 11977; break;
    default: if ( level < 80 )
               initial_armor = (int) floor ( ( level / 80.0 ) * 9729 ); // Need a better value here.
             break;
    }
  }
  player_t::base_armor = initial_armor;

}

// target_t::init_base =====================================================

void enemy_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str += "/snapshot_stats";

    bool tank = false;

    if ( !is_add() )
    {
      for ( player_t* q = sim -> player_list; q; q = q -> next )
      {
        if ( q -> primary_role() != ROLE_TANK )
          continue;
        action_list_str += "/auto_attack,target=";
        action_list_str += q -> name_str;
        tank = true;
        break;
      }
    }
  }

  player_t::init_actions();
}

// target_t::composite_tank_block ==================================================

double enemy_t::composite_tank_block() SC_CONST
{
  double b = player_t::composite_tank_block();

  b += 0.05;

  return b;
}

// target_t::create_options ====================================================

void enemy_t::create_options()
{
  option_t target_options[] =
  {
      { "target_health",                    OPT_FLT,    &( fixed_health                      ) },
      { "target_initial_health_percentage", OPT_FLT,    &( initial_health_percentage         ) },
      { "target_fixed_health_percentage",   OPT_FLT,    &( fixed_health_percentage           ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( sim -> options, target_options );

  player_t::create_options();
}


// target_t::create_add =====================================================

pet_t* enemy_t::create_pet( const std::string& add_name, const std::string& pet_type )
{

  pet_t* p = find_pet( add_name );
    if ( p ) return p;

  return new enemy_add_t( sim, this, add_name, PET_ENEMY );

  return 0;
}


// target_t::find_add =======================================================

pet_t* enemy_t::find_pet( const std::string& add_name )
{
  for ( pet_t* p = pet_list; p; p = p -> next_pet )
    if ( p -> name_str == add_name )
      return p;

  return 0;
}
// enemy_t::recalculate_health ==============================================

void enemy_t::recalculate_health()
{
  if ( sim -> expected_time == 0 ) return;

  if ( ( initial_health_percentage < 0.001 ) || ( initial_health_percentage > 100.0 ) )
    initial_health_percentage = 100.0;

  if ( fixed_health != 0.0 )
  {
    resource_current[ RESOURCE_HEALTH ] = fixed_health;
    max_health = resource_base[ RESOURCE_HEALTH ] = fixed_health / ( initial_health_percentage / 100.0 );
    return;
  }

  if ( resource_base[ RESOURCE_HEALTH ] == 0 || sim -> current_iteration == 0 )
  {
    initial_health = dmg_taken * ( sim -> expected_time / sim -> current_time );
    resource_base[ RESOURCE_HEALTH ] = resource_max[ RESOURCE_HEALTH ] = initial_health;
    resource_current[ RESOURCE_HEALTH ] = initial_health - dmg_taken;
  }
  else
  {
    double delta_time = sim -> current_time - sim -> expected_time;
    delta_time /= log( 1.0 * ( sim -> current_iteration + 1 ) ); // dampening factor
    double time_factor = ( delta_time / sim -> expected_time );

    double delta_dmg = initial_health - dmg_taken * ( sim -> expected_time / sim -> current_time );
    delta_dmg /= log( 1.0 * ( sim -> current_iteration + 1 ) ); // dampening factor
    double dmg_factor = ( delta_dmg / ( dmg_taken * ( sim -> expected_time / sim -> current_time ) )  );

    double factor = 1.0;
    if ( fabs( time_factor ) > fabs( dmg_factor ) )
      factor = 1- time_factor;
    else
      factor = 1 - dmg_factor;

    if ( factor > 1.5 ) factor = 1.5;
    if ( factor < 0.5 ) factor = 0.5;

    initial_health *= factor;
  }

  max_health = resource_base[ RESOURCE_HEALTH ] / ( initial_health_percentage / 100.0 );

  if ( sim -> debug ) log_t::output( sim, "Target %s initial health calculated to be %.0f. Damage was %.0f", name(), resource_base[ RESOURCE_HEALTH ], dmg_taken );
}


// target_t::create_expression ================================================

action_expr_t* enemy_t::create_expression( action_t* action,
                                          const std::string& name_str )
{

   if ( name_str == "adds" )
   {
     struct target_adds_expr_t : public action_expr_t
     {
       player_t* target;
       target_adds_expr_t( action_t* a, player_t* t ) :
         action_expr_t( a, "target_adds", TOK_NUM ), target(t) {}
       virtual int evaluate() { result_num = target -> active_pets;  return TOK_NUM; }
     };
     return new target_adds_expr_t( action, this );
   }

   return player_t::create_expression( action, name_str );
}

void enemy_t::reset()
{
  resource_base[ RESOURCE_HEALTH ] = initial_health * ( 1.0 + sim -> vary_combat_length * sim -> iteration_adjust() );


  player_t::reset();

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

// player_t::create_warrior =================================================

player_t* player_t::create_enemy( sim_t* sim, const std::string& name, race_type r )
{
  return new enemy_t( sim, name );
}

// warrior_init =============================================================

void player_t::enemy_init( sim_t* sim )
{

}

// player_t::warrior_combat_begin ===========================================

void player_t::enemy_combat_begin( sim_t* sim )
{

}

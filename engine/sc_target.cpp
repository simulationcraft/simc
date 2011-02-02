// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Target
// ==========================================================================

// target_t::target_t =======================================================

target_t::target_t( sim_t* s, const std::string& n ) :
    player_t( s, ENEMY, n, RACE_HUMANOID ),
    next( 0 ), target_level( -1 ),
    initial_armor( -1 ), armor( 0 ), block_value( 0.3 ),
    attack_speed( 3.0 ), attack_damage( 50000 ), weapon_skill( 0 ),
    fixed_health( 0 ), initial_health( 0 ), current_health( 0 ), fixed_health_percentage( 0 ),
    total_dmg( 0 ), adds_nearby( 0 ), initial_adds_nearby( 0 ), resilience( 0 )
{
  for ( int i=0; i < SCHOOL_MAX; i++ ) spell_resistance[ i ] = 0;
  create_options();

}

// target_t::id =============================================================

const char* target_t::id()
{
  if ( id_str.empty() )
  {
    char buffer[ 1024 ];
    sprintf( buffer, "0xF1300079AA001884,\"%s\",0x10a28", name_str.c_str() );
    id_str = buffer;
  }

  return id_str.c_str();
}

// target_t::assess_damage ==================================================

void target_t::assess_damage( double amount,
                              const school_type school,
                              int    dmg_type,
                              action_t* a,
                              player_t* s )
{
  total_dmg += amount;

  resource_loss( RESOURCE_HEALTH, amount );

  if ( current_health > 0 )
  {
    current_health -= amount;

    if ( current_health <= 0 )
    {
      if ( sim -> log ) log_t::output( sim, "%s has died.", name() );
    }
    else if ( sim -> debug ) log_t::output( sim, "Target %s has %.0f remaining health", name(), current_health );
  }
}

// target_t::recalculate_health ==============================================

void target_t::recalculate_health()
{
  if ( sim -> expected_time == 0 ) return;

  if ( fixed_health == 0 )
  {
    current_health = total_dmg;
    initial_health = current_health * ( sim -> expected_time / sim -> current_time );
    fixed_health = initial_health;
    resource_base[ RESOURCE_HEALTH ] = initial_health;
  }
  else
  {
    double delta_time = sim -> current_time - sim -> expected_time;
    delta_time /= sim -> current_iteration + 1; // dampening factor

    double factor = 1 - ( delta_time / sim -> expected_time );
    if ( factor > 1.5 ) factor = 1.5;
    if ( factor < 0.5 ) factor = 0.5;

    fixed_health *= factor;
    resource_base[ RESOURCE_HEALTH ] *= factor;
  }

  if ( sim -> debug ) log_t::output( sim, "Target fixed health calculated to be %.0f", fixed_health );
}

// target_t::time_to_die =====================================================

double target_t::time_to_die() SC_CONST
{
  if ( initial_health > 0 )
  {
    return sim -> current_time * current_health / total_dmg;
  }
  else
  {
    return sim -> expected_time - sim -> current_time;
  }
}

// target_t::health_percentage ===============================================

double target_t::health_percentage() SC_CONST
{
  if ( fixed_health_percentage > 0 )
  {
    return fixed_health_percentage;
  }
  else if ( initial_health > 0 ) 
  {
    return 100.0 * current_health / initial_health;
  }
  else
  {
    return 100.0 * ( sim -> expected_time - sim -> current_time ) / sim -> expected_time;
  }
}

// target_t::base_armor ======================================================

double target_t::base_armor() SC_CONST
{
  return armor;
}

// target_t::aura_gain ======================================================

void target_t::aura_gain( const char* aura_name , int aura_id )
{
  if( sim -> log ) log_t::output( sim, "Target %s gains %s", name(), aura_name );
}

// target_t::aura_loss ======================================================

void target_t::aura_loss( const char* aura_name , int aura_id )
{
  if( sim -> log ) log_t::output( sim, "Target %s loses %s", name(), aura_name );
}

// target_t::init ============================================================

void target_t::init()
{
  if ( target_level < 0 )
  {
    level = sim -> max_player_level + 3;
  }

  initialized = 1;
  init_talents();
  init_spells();
  // init_glyphs();
  init_rating();
  init_race();
  init_base();
  // init_racials();
  init_items();
  init_core();
  init_spell();
  init_attack();
  init_defense();
  init_weapon( &main_hand_weapon );
  init_weapon( &off_hand_weapon );
  init_weapon( &ranged_weapon );
  init_unique_gear();
  init_enchant();
  // init_professions();
  // init_consumables();
  init_scaling();
  init_buffs();
  init_actions();
  init_gains();
  init_procs();
  init_uptimes();
  init_rng();
  init_stats();
  init_values();
}

// target_t::init_base =====================================================

void target_t::init_base()
{
  resource_base[ RESOURCE_HEALTH ] = 0;

  health_per_stamina = 10;

  if ( initial_armor < 0 )
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

  if( resilience > 0 )
  {
    // TO-DO: Needs work.
    // 1414.5 current capped resilience
    resilience = std::min(resilience, 1414.5);
  }

  if ( weapon_skill == 0 ) weapon_skill = 5.0 * level;
}

// target_t::init_base =====================================================

void target_t::init_items()
{
  items[ SLOT_MAIN_HAND ].options_str = "Skullcrusher,weapon=sword2h_3.00speed_180000min_360000max";

  player_t::init_items();
}

// target_t::init_base =====================================================

void target_t::init_actions()
{
  action_list_str += "/auto_attack";

  player_t::init_actions();
}

// target_t::init_gains =====================================================

void target_t::init_gains()
{
  player_t::init_gains();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    gains.push_back(get_gain( p -> name_str ) );
  }

}

// target_t::reset ===========================================================

void target_t::reset()
{
  if ( sim -> debug ) log_t::output( sim, "Reseting target %s", name() );
  total_dmg = 0;
  armor = initial_armor;
  current_health = initial_health = fixed_health * ( 1.0 + sim -> vary_combat_length * sim -> iteration_adjust() );
  adds_nearby = initial_adds_nearby;

  player_t::reset();
}

// target_t::combat_begin ====================================================

void target_t::combat_begin()
{
  player_t::combat_begin();

  if ( sim -> overrides.bleeding ) debuffs.bleeding -> override();
}

// target_t::primary_resource ====================================================

int target_t::primary_resource() SC_CONST
{
  return RESOURCE_HEALTH;
}

// target_t::primary_role ====================================================

int target_t::primary_role() SC_CONST
{
  return ROLE_HYBRID;
}

// target_t::composite_attack_haste ========================================

double target_t::composite_attack_haste() SC_CONST
{
  return attack_haste;

}

// target_t::debuffs_t::snared ===============================================

bool target_t::debuffs_t::snared()
{
  if ( infected_wounds -> check() ) return true;
  if ( judgements_of_the_just -> check() ) return true;
  if ( slow -> check() ) return true;
  if ( thunder_clap -> check() ) return true;
  return false;
}

struct auto_attack_t : public attack_t
{
  auto_attack_t( player_t* player, const std::string& options_str ) :
      attack_t( "auto_attack", player, RESOURCE_MANA, SCHOOL_PHYSICAL )
  {
    parse_options( NULL, options_str );

    base_dd_min = base_dd_max = 1;

    player_t* q = sim -> find_player( "Fluffy_Tank" );
    if ( q )
    {
      target = q;
      weapon = &( player -> main_hand_weapon );
    }
  }

  virtual double execute_time() SC_CONST
  {
    return sim -> gauss( 3.0, 0.25 );
  }
};

// shaman_t::create_action  =================================================

action_t* target_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );


  return player_t::create_action( name, options_str );
}

// target_t::create_options ====================================================

void target_t::create_options()
{
  option_t target_options[] =
  {
    { "target_name",                    OPT_STRING, &( name_str                          ) },
    { "target_race",                    OPT_STRING, &( race_str                          ) },
    { "target_level",                   OPT_INT,    &( target_level                             ) },
    { "target_health",                  OPT_FLT,    &( fixed_health                      ) },
    { "target_id",                      OPT_STRING, &( id_str                            ) },
    { "target_adds",                    OPT_INT,    &( initial_adds_nearby               ) },

    { "target_armor",                   OPT_INT,    &( initial_armor                     ) },
    { "target_block",                   OPT_FLT,    &( block_value                       ) },
    { "target_attack_speed",            OPT_FLT,    &( attack_speed                      ) },
    { "target_attack_damage",           OPT_FLT,    &( attack_damage                     ) },
    { "target_weapon_skill",            OPT_FLT,    &( weapon_skill                      ) },
    { "target_resilience",              OPT_FLT,    &( resilience                        ) },
    { "target_fixed_health_percentage", OPT_FLT,    &( fixed_health_percentage           ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( sim -> options, target_options );
}

// target_t::find =========================================================

target_t* target_t::find( sim_t* sim,
                          const std::string& name_str )
{
  for ( target_t* t = sim -> target_list; t; t = t -> next )
    if ( name_str == t -> name() )
      return t;

  return 0;
}

// target_t::create_expression ================================================

action_expr_t* target_t::create_expression( action_t* action,
                                          const std::string& type )
{
   if ( name_str == "level" )
   {
     struct level_expr_t : public action_expr_t
     {
       target_t* target;
       level_expr_t( action_t* a, target_t* t ) : 
         action_expr_t( a, "target_level", TOK_NUM ), target(t) {}
       virtual int evaluate() { result_num = target -> level; return TOK_NUM; }
     };
     return new level_expr_t( action, this );
   }
   if ( type == "time_to_die" )
   {
     struct target_time_to_die_expr_t : public action_expr_t
     {
       target_t* target;
       target_time_to_die_expr_t( action_t* a, target_t* t ) :
         action_expr_t( a, "target_time_to_die", TOK_NUM ), target(t) {}
       virtual int evaluate() { result_num = target -> time_to_die();  return TOK_NUM; }
     };
     return new target_time_to_die_expr_t( action, this );
   }
   else if ( type == "health_pct" )
   {
     struct target_health_pct_expr_t : public action_expr_t
     {
       target_t* target;
       target_health_pct_expr_t( action_t* a, target_t* t ) :
         action_expr_t( a, "target_health_pct", TOK_NUM ), target(t) {}
       virtual int evaluate() { result_num = target -> health_percentage();  return TOK_NUM; }
     };
     return new target_health_pct_expr_t( action, this );
   }

   else if ( type == "adds" )
   {
     struct target_adds_expr_t : public action_expr_t
     {
       target_t* target;
       target_adds_expr_t( action_t* a, target_t* t ) :
         action_expr_t( a, "target_adds", TOK_NUM ), target(t) {}
       virtual int evaluate() { result_num = target -> adds_nearby;  return TOK_NUM; }
     };
     return new target_adds_expr_t( action, this );
   }

   else if ( type == "adds_never" )
   {
     struct target_adds_never_expr_t : public action_expr_t
     {
       target_t* target;
       target_adds_never_expr_t( action_t* a, target_t* t ) :
         action_expr_t( a, "target_adds_never", TOK_NUM ), target(t)
       {
         bool adds = target -> initial_adds_nearby > 0;
         int num_events = ( int ) a -> sim -> raid_events.size();
         for ( int i=0; i < num_events; i++ )
           if ( a -> sim -> raid_events[ i ] -> name_str == "adds" )
         adds = true;
         result_num = adds ? 0.0 : 1.0;
       }
     };
     return new target_adds_never_expr_t( action, this );
   }


   return player_t::create_expression( action, name_str );
}

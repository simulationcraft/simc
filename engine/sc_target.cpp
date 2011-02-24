// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Target
// ==========================================================================

// target_t::target_t =======================================================

target_t::target_t( sim_t* s, const std::string& n, player_type pt ) :
    player_t( s, pt, n, RACE_HUMANOID ),
    next( 0 ), target_level( -1 ),
    initial_armor( -1 ), armor( 0 ),
    attack_speed( 3.0 ), attack_damage( 50000 ),
    fixed_health( 0 ), initial_health( 0 ), current_health( 0 ), fixed_health_percentage( 0 ),
    total_dmg( 0 ), adds_nearby( 0 ), initial_adds_nearby( 0 ), resilience( 0 ),
    add_list( 0 )
{
  for ( int i=0; i < SCHOOL_MAX; i++ ) spell_resistance[ i ] = 0;
  create_options();

  target_t** last = &( sim -> target_list );
  while ( *last ) last = &( ( *last ) -> next );
  *last = this;
  next = 0;


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

double target_t::assess_damage( double            amount,
				const school_type school,
				int               dmg_type,
				int               result,
				action_t*         action )
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

  return amount;
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
  }
  else
  {
    double delta_time = sim -> current_time - sim -> expected_time;
    delta_time /= sim -> current_iteration + 1; // dampening factor
    double time_factor = ( delta_time / sim -> expected_time );

    double delta_dmg = fixed_health - total_dmg * ( sim -> expected_time / sim -> current_time );
    delta_dmg /= sim -> current_iteration + 1; // dampening factor
    double dmg_factor = ( delta_dmg / ( total_dmg * ( sim -> expected_time / sim -> current_time ) )  );

    double factor = 1.0;
    if ( fabs( time_factor ) > fabs( dmg_factor ) )
      factor = 1- time_factor;
    else
      factor = 1 - dmg_factor;

    if ( factor > 1.5 ) factor = 1.5;
    if ( factor < 0.5 ) factor = 0.5;

    fixed_health *= factor;
  }

  if ( sim -> debug ) log_t::output( sim, "Target %s fixed health calculated to be %.0f. Total Damage was %.0f", name(), fixed_health, total_dmg );
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
  else
    level = target_level;

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

  health_per_stamina = 10;

  base_attack_crit = 0.05;

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
}

// target_t::init_items =====================================================

void target_t::init_items()
{
  items[ SLOT_MAIN_HAND ].options_str = "Skullcrusher,weapon=axe2h_3.00speed_120000min_140000max";

  player_t::init_items();
}

// target_t::init_base =====================================================

void target_t::init_actions()
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

  player_t::init_actions();
}

// target_t::reset ===========================================================

void target_t::reset()
{
  if ( sim -> debug ) log_t::output( sim, "Reseting target %s", name() );
  total_dmg = 0;
  armor = initial_armor;
  player_t::base_armor = initial_armor;
  current_health = initial_health = fixed_health * ( 1.0 + sim -> vary_combat_length * sim -> iteration_adjust() );

  adds_nearby = initial_adds_nearby;

  resource_base[ RESOURCE_HEALTH ] = fixed_health;

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
  return ROLE_TANK;
}

// target_t::composite_tank_block ==================================================

double target_t::composite_tank_block() SC_CONST
{
  double b = player_t::composite_tank_block();

  b += 0.05;

  return b;
}



struct auto_attack_t : public attack_t
{
  auto_attack_t( player_t* p, const std::string& options_str ) :
      attack_t( "auto_attack", p, RESOURCE_MANA, SCHOOL_PHYSICAL )
  {
    parse_options( NULL, options_str );
    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );
    stats = player -> get_stats( name_str + "_" + target -> name() );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();
    base_execute_time = 2.0;
    may_crit = true;

    base_dd_min = base_dd_max = 1;

    if ( player -> is_enemy() && target != player)
    {
      weapon = &( player -> main_hand_weapon );
    }
  }
/*
  virtual double execute_time() SC_CONST
  {
    return 0;
  }*/

};

struct summon_add_t : public spell_t
{
  std::string name_str;
  add_t* add_to_summon;
  double health;

  summon_add_t( target_t* p, const std::string& options_str ) :
    spell_t( "summon_add", p, RESOURCE_MANA, SCHOOL_FIRE ),
    name_str( "" ), add_to_summon( 0 ), health( 0 )
  {
    option_t options[] =
    {
      { "name",   OPT_STRING,    &name_str  },
      { "health", OPT_FLT,       &health    },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    for ( add_t* add = p -> add_list; add; add = add -> next_add )
        if ( name_str == add -> name_str )
          add_to_summon = add;

    cooldown -> duration = 100;
  }


  virtual void execute()
  {
    spell_t::execute();

    assert( add_to_summon );

    add_to_summon -> summon( 50, health );
  }

  virtual bool ready()
  {
    if ( ! add_to_summon )
      return false;

    if ( !add_to_summon -> sleeping )
      return false;

    return spell_t::ready();
  }
};

// shaman_t::create_action  =================================================

action_t* target_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "summon_add"              ) return new               summon_add_t( this, options_str );


  return player_t::create_action( name, options_str );
}

// target_t::create_options ====================================================

void target_t::create_options()
{
  option_t target_options[] =
  {
    { "target_name",                    OPT_STRING, &( name_str                          ) },
    { "target_race",                    OPT_STRING, &( race_str                          ) },
    { "target_level",                   OPT_INT,    &( target_level                      ) },
    { "target_health",                  OPT_FLT,    &( fixed_health                      ) },
    { "target_id",                      OPT_STRING, &( id_str                            ) },
    { "target_adds",                    OPT_INT,    &( initial_adds_nearby               ) },
    { "target_armor",                   OPT_INT,    &( initial_armor                     ) },
    { "target_attack_speed",            OPT_FLT,    &( attack_speed                      ) },
    { "target_attack_damage",           OPT_FLT,    &( attack_damage                     ) },
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

// target_t::create_add =====================================================

add_t* target_t::create_add( const std::string& add_name )
{
  int i = 1;
  for ( add_t* add = add_list; add; add = add -> next_add )
    i++;

  std::stringstream stream;
  stream << "Evil_Add" << i;

  if ( add_name == "Evil_Add"     ) return new    add_t( sim, this, stream.str() );

  return 0;
}

// target_t::create_adds ====================================================

void target_t::create_adds()
{
  if ( is_add() )
    return;

  for ( int i = 0; i < 9; i++ )
    create_add( "Evil_Add" );
}

// target_t::find_add =======================================================

add_t* target_t::find_add( const std::string& add_name )
{
  for ( add_t* p = add_list; p; p = p -> next_add )
    if ( p -> name_str == add_name )
      return p;

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

// ==========================================================================
// Add
// ==========================================================================

// add_t::add_t =============================================================

void add_t::_init_add_t()
{
  level = owner -> level;
  full_name_str = owner -> name_str + "_" + name_str;

  add_t** last = &( owner -> add_list );
  while ( *last ) last = &( ( *last ) -> next_add );
  *last = this;
  next_add = 0;

  party = owner -> party;

  // By default, only report statistics in the context of the owner
  quiet = 1;
  player_data.set_parent( &( owner -> player_data ) );
}

add_t::add_t( sim_t*             s,
              target_t*          o,
              const std::string& n ) :
    target_t( s, n, ENEMY_ADD ), owner( o ), next_add( 0 ), summoned( false )
{
  _init_add_t();
}

// add_t::init ==============================================================

void add_t::init()
{
  level = owner -> level;
  target_t::init();

}

// add_t::reset =============================================================

void add_t::reset()
{
  target_t::reset();
  sleeping = 1;
  summon_time = 0;
}

// add_t::summon ============================================================

void add_t::summon( double duration, double health)
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s summons %s.", owner -> name(), name() );
    //log_t::summon_event( this );
  }


  distance = owner -> distance;

  summon_time = sim -> current_time;
  summoned=true;
  stat_gain( STAT_MAX_HEALTH, health );

  if( duration > 0 )
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p, double duration ) : event_t( sim, p )
      {
        sim -> add_event( this, duration );
      }
      virtual void execute()
      {
        player -> cast_add() -> dismiss();
      }
    };
    new ( sim ) expiration_t( sim, this, duration );
  }

  arise();
}

// add_t::dismiss ===========================================================

void add_t::dismiss()
{
  if ( sim -> log ) log_t::output( sim, "%s dismisses %s", owner -> name(), name() );

  demise();
}

// add_t::resource_loss =================================================

double add_t::resource_loss( int       resource,
                                double    amount,
                                action_t* action )
{
  double d = target_t::resource_loss( resource, amount, action );

  if ( resource == RESOURCE_HEALTH && amount > resource_current[ resource ] )
  {
    if ( sim -> debug ) log_t::output( sim, "Add %s died, dismissing!", name() );
    dismiss();
  }

  return d;
}

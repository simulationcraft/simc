// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Target
// ==========================================================================

// target_t::target_t =======================================================

target_t::target_t( sim_t* s ) :
    sim( s ), name_str( "Fluffy Pillow" ), race( RACE_HUMANOID ), level( 83 ),
    initial_armor( -1 ), armor( 0 ), block_value( 100 ), 
    attack_speed( 2.0 ), attack_damage( 2000 ), weapon_skill( 0 ),
    fixed_health( 0 ), initial_health( 0 ), current_health( 0 ), total_dmg( 0 ),
    adds_nearby( 0 ), initial_adds_nearby( 0 ), resilience( 0 )
{
  for ( int i=0; i < SCHOOL_MAX; i++ ) spell_resistance[ i ] = 0;
}

// target_t::~target_t ======================================================

target_t::~target_t()
{
}

// target_t::id =============================================================

const char* target_t::id()
{
  if ( id_str.empty() )
  {
    id_str = "0xF1300079AA001884,\"Heroic Training Dummy\",0x10a28";
  }

  return id_str.c_str();
}

// target_t::assess_damage ==================================================

void target_t::assess_damage( double amount,
                              int    school,
                              int    dmg_type )
{
  total_dmg += amount;

  if ( current_health > 0 )
  {
    current_health -= amount;

    if ( current_health <= 0 )
    {
      if ( sim -> log ) log_t::output( sim, "%s has died.", name() );
    }
    else if ( sim -> debug ) log_t::output( sim, "Target %s has %.0f remaining health", name(), current_health );
  }

  // FIXME! Someday create true "callbacks" for the various events to clean up crap like this.

  if ( ( school == SCHOOL_PHYSICAL ) && ( dmg_type == DMG_DIRECT ) )
  {
    debuffs.hemorrhage -> decrement();
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
  }
  else
  {
    double delta_time = sim -> current_time - sim -> expected_time;
    delta_time /= sim -> current_iteration + 1; // dampening factor

    double factor = 1 - ( delta_time / sim -> expected_time );
    if ( factor > 1.5 ) factor = 1.5;
    if ( factor < 0.5 ) factor = 0.5;

    fixed_health *= factor;
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
  if ( initial_health > 0 ) 
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
  if ( initial_armor < 0 ) 
  {
    switch ( level )
    {
    case 80: initial_armor = 9729;  break;
    case 81: initial_armor = 10034; break;
    case 82: initial_armor = 10338; break;
    case 83: initial_armor = 10643; break;
    default: initial_armor = 10643;
    }
  }

  if( resilience > 0 )
  {
  	// 1414.5 current capped resilience
  	resilience = std::min(resilience, 1414.5);
  }

  if ( weapon_skill == 0 ) weapon_skill = 5.0 * level;

  if ( ! race_str.empty() )
  {
    for ( race = RACE_NONE; race < RACE_MAX; race++ )
      if ( race_str == util_t::race_type_string( race ) )
        break;

    if ( race == RACE_MAX )
    {
      sim -> errorf( "'%s' is not a valid value for 'target_race'\n", race_str.c_str() );
      sim -> cancel();
    }
  }

  // Infinite-Stacking De-Buffs
  debuffs.bleeding     = new debuff_t( sim, "bleeding",     -1 );
  debuffs.casting      = new debuff_t( sim, "casting",      -1 );
  debuffs.invulnerable = new debuff_t( sim, "invulnerable", -1 );
  debuffs.vulnerable   = new debuff_t( sim, "vulnerable",   -1 );
}

// target_t::reset ===========================================================

void target_t::reset()
{
  if ( sim -> debug ) log_t::output( sim, "Reseting target %s", name() );
  total_dmg = 0;
  armor = initial_armor;
  current_health = initial_health = fixed_health * ( 1.0 + sim -> vary_combat_length * sim -> iteration_adjust() );
  adds_nearby = initial_adds_nearby;
}

// target_t::combat_begin ====================================================

void target_t::combat_begin()
{
  if ( sim -> overrides.bleeding ) debuffs.bleeding -> override();

  if ( sim -> overrides.bloodlust )
  {
    // Setup a periodic check for Bloodlust

    struct bloodlust_check_t : public event_t
    {
      bloodlust_check_t( sim_t* sim ) : event_t( sim, 0 )
      {
        name = "Bloodlust Check";
        sim -> add_event( this, 1.0 );
      }
      virtual void execute()
      {
        target_t* t = sim -> target;
        if ( ( sim -> overrides.bloodlust_early && ( sim -> current_time > ( double ) sim -> overrides.bloodlust_early ) ) ||
             ( t -> health_percentage() < 25 ) ||
             ( t -> time_to_die()       < 60 ) )
        {
	  for ( player_t* p = sim -> player_list; p; p = p -> next )
          {
	    if ( p -> sleeping ) continue;
	    p -> buffs.bloodlust -> trigger();
	  }
        }
        else
        {
          new ( sim ) bloodlust_check_t( sim );
        }
      }
    };

    new ( sim ) bloodlust_check_t( sim );
  }
}

// target_t::combat_end ======================================================

void target_t::combat_end()
{}

// target_t::debuffs_t::snared ===============================================

bool target_t::debuffs_t::snared()
{
  if ( frozen() ) return true;
  if (  slow -> check() ) return true;
  if ( thunder_clap -> check() ) return true;
  if ( infected_wounds -> check() ) return true;
  if ( judgements_of_the_just -> check() ) return true;
  return false;
}

// target_t::get_options =======================================================

int target_t::get_options( std::vector<option_t>& options )
{
  option_t target_options[] =
  {
    // @option_doc loc=global/target/general title="Target General"
    { "target_name",           OPT_STRING, &( name_str                          ) },
    { "target_race",           OPT_STRING, &( race_str                          ) },
    { "target_level",          OPT_INT,    &( level                             ) },
    { "target_health",         OPT_FLT,    &( fixed_health                      ) },
    { "target_id",             OPT_STRING, &( id_str                            ) },
    { "target_adds",           OPT_INT,    &( initial_adds_nearby               ) },
    // @option_doc loc=global/target/defense title="Target Defense"
    { "target_resist_holy",    OPT_INT,    &( spell_resistance[ SCHOOL_HOLY   ] ) },
    { "target_resist_shadow",  OPT_INT,    &( spell_resistance[ SCHOOL_SHADOW ] ) },
    { "target_resist_arcane",  OPT_INT,    &( spell_resistance[ SCHOOL_ARCANE ] ) },
    { "target_resist_frost",   OPT_INT,    &( spell_resistance[ SCHOOL_FROST  ] ) },
    { "target_resist_fire",    OPT_INT,    &( spell_resistance[ SCHOOL_FIRE   ] ) },
    { "target_resist_nature",  OPT_INT,    &( spell_resistance[ SCHOOL_NATURE ] ) },
    { "target_armor",          OPT_INT,    &( initial_armor                     ) },
    { "target_block",          OPT_INT,    &( block_value                       ) },
    // @option_doc loc=global/target/auto_attack title="Target Auto-Attack"
    { "target_attack_speed",   OPT_FLT,    &( attack_speed                      ) },
    { "target_attack_damage",  OPT_FLT,    &( attack_damage                     ) },
    { "target_weapon_skill",   OPT_FLT,    &( weapon_skill                      ) },
    { "target_resilience",     OPT_FLT,    &( resilience                        ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, target_options );

  return ( int ) options.size();
}


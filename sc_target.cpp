// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Target
// ==========================================================================

// target_t::target_t =======================================================

target_t::target_t( sim_t* s ) :
  sim(s), name_str("Fluffy Pillow"), race(RACE_HUMANOID), level(83), 
  initial_armor(13000), armor(0), block_value(0), shield(0), 
  initial_health( 0 ), current_health(0), total_dmg(0), uptime_list(0)
{
  for( int i=0; i < SCHOOL_MAX; i++ ) spell_resistance[ i ] = 0;
}

// target_t::~target_t ======================================================

target_t::~target_t()
{
  while( uptime_t* u = uptime_list )
  {
    uptime_list = u -> next;
    delete u;
  }
}

// target_t::assess_damage ===================================================

void target_t::assess_damage( double amount, 
                              int    school, 
                              int    dmg_type )
{
  total_dmg += amount;

  if( initial_health > 0 )
  {
    current_health -= amount;

    if( current_health <= 0 ) 
    {
      if( sim -> log ) report_t::log( sim, "%s has died.", name() );
    }
    else if( sim -> debug ) report_t::log( sim, "Target %s has %.0f remaining health", name(), current_health );
  }

  // FIXME! Someday create true "callbacks" for the various events to clean up crap like this.
  
  if( ( school == SCHOOL_PHYSICAL ) && ( dmg_type == DMG_DIRECT ) && ( debuffs.hemorrhage_charges > 0 ) )
  {
    debuffs.hemorrhage_charges--;
    if( debuffs.hemorrhage_charges == 0 ) event_t::cancel( expirations.hemorrhage );
  }
}
   
// target_t::recalculate_health ==============================================

void target_t::recalculate_health()
{
  current_health = total_dmg;
  initial_health = current_health * ( sim -> max_time / sim -> current_time );
  
  if( sim -> debug ) report_t::log( sim, "Target initial health calculated to be %.0f", initial_health );     
}

// target_t::time_to_die =====================================================

double target_t::time_to_die()
{
  if( initial_health > 0 )
  {
    return sim -> current_time * current_health / total_dmg;
  }
  else
  {
    return sim -> max_time - sim -> current_time;
  }
}

// target_t::health_percentage ===============================================

double target_t::health_percentage()
{
  if( initial_health <= 0 ) return 100;

  return 100.0 * current_health / initial_health;
}

// target_t::composite_armor =================================================

double target_t::composite_armor()
{
  double adjusted_armor = armor;

  adjusted_armor -= std::max( debuffs.sunder_armor, debuffs.expose_armor );

  adjusted_armor -= debuffs.faerie_fire;

  return adjusted_armor;
}

// target_t::get_uptime =====================================================

uptime_t* target_t::get_uptime( const std::string& name )
{
  uptime_t* u=0;

  for( u = uptime_list; u; u = u -> next )
  {
    if( u -> name_str == name )
      return u;
  }

  u = new uptime_t( name );

  uptime_t** tail = &uptime_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  u -> next = *tail;
  *tail = u;

  return u;
}

// target_t::init ============================================================

void target_t::init()
{
  if( ! race_str.empty() )
  {
    for( race = RACE_NONE; race < RACE_MAX; race++ )
      if( race_str == util_t::race_type_string( race ) )
	break;

    if( race == RACE_MAX )
    {
      printf( "simcraft: '%s' is not a valid value for 'target_race'\n", race_str.c_str() );
      exit(0);
    }
  }

  uptimes.winters_grasp   = get_uptime( "winters_grasp"   );
  uptimes.winters_chill   = get_uptime( "winters_chill"   );
  uptimes.improved_scorch = get_uptime( "improved_scorch" );
  uptimes.blood_frenzy    = get_uptime( "blood_frenzy"    );
  uptimes.savage_combat   = get_uptime( "savage_combat"   );
  uptimes.totem_of_wrath  = get_uptime( "totem_of_wrath"  );
  uptimes.master_poisoner = get_uptime( "master_poisoner" );
}

// target_t::reset ===========================================================

void target_t::reset()
{
  if( sim -> debug ) report_t::log( sim, "Reseting target %s", name() );
  total_dmg = 0;
  armor = initial_armor;
  current_health = initial_health;
  debuffs.reset();
  expirations.reset();
  cooldowns.reset();
}

// target_t::combat_begin ====================================================

void target_t::combat_begin()
{
  if( sim -> optimal_raid )
  {
    // Static De-Buffs
    debuffs.blood_frenzy = 1;
    debuffs.crypt_fever = 1;
    debuffs.judgement_of_wisdom = 1;
    debuffs.mangle = 1;
    debuffs.razorice = 1;
    debuffs.snare = 1;
    debuffs.sunder_armor = 3925;
    debuffs.thunder_clap = 1;

    // Dynamic De-Buffs
    debuffs.affliction_effects = 12;
    debuffs.curse_of_elements = 13;
    debuffs.faerie_fire = 1260;
    debuffs.ferocious_inspiration = 1;
    debuffs.hunters_mark = 450;
    debuffs.improved_faerie_fire = 3;
    debuffs.improved_scorch = 5;
    debuffs.master_poisoner = 1;
    debuffs.misery = 3;
    debuffs.misery_stack = 1;
    debuffs.earth_and_moon = 13;
    debuffs.poisoned = 1;
    debuffs.savage_combat = 1;
    debuffs.totem_of_wrath = 1;
    debuffs.winters_chill = 5;

    // Setup a periodic check for Bloodlust

    struct bloodlust_proc_t : public event_t
    {
      bloodlust_proc_t( sim_t* sim ) : event_t( sim, 0 )
      {
	name = "Bloodlust Proc";
        for( player_t* p = sim -> player_list; p; p = p -> next )
        {
          if( p -> sleeping ) continue;
          if( sim -> cooldown_ready( p -> cooldowns.bloodlust ) )
          {
            p -> aura_gain( "Bloodlust" );
            p -> buffs.bloodlust = 1;
            p -> cooldowns.bloodlust = sim -> current_time + 300;
          }
        }
	sim -> add_event( this, 40.0 );
      }
      virtual void execute()
      {
        for( player_t* p = sim -> player_list; p; p = p -> next )
        {
          if( p -> buffs.bloodlust > 0 )
          {
            p -> aura_loss( "Bloodlust" );
            p -> buffs.bloodlust = 0;
          }
        }
      }
    };
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
	if( ( t -> health_percentage() < 25 ) || 
	    ( t -> time_to_die()       < 60 ) )
	{
	  new ( sim ) bloodlust_proc_t( sim );
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
{
}

// target_t::parse_option ======================================================

bool target_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    // General
    { "target_race",           OPT_STRING, &( race_str                          ) },
    { "target_level",          OPT_INT,    &( level                             ) },
    { "target_resist_holy",    OPT_INT,    &( spell_resistance[ SCHOOL_HOLY   ] ) },
    { "target_resist_shadow",  OPT_INT,    &( spell_resistance[ SCHOOL_SHADOW ] ) },
    { "target_resist_arcane",  OPT_INT,    &( spell_resistance[ SCHOOL_ARCANE ] ) },
    { "target_resist_frost",   OPT_INT,    &( spell_resistance[ SCHOOL_FROST  ] ) },
    { "target_resist_fire",    OPT_INT,    &( spell_resistance[ SCHOOL_FIRE   ] ) },
    { "target_resist_nature",  OPT_INT,    &( spell_resistance[ SCHOOL_NATURE ] ) },
    { "target_armor",          OPT_INT,    &( initial_armor                     ) },
    { "target_shield",         OPT_INT,    &( shield                            ) },
    { "target_block",          OPT_INT,    &( block_value                       ) },
    { "target_health",         OPT_FLT,    &( initial_health                    ) },
    // FIXME! Once appropriate class implemented, these will be removed
    { "blood_frenzy",          OPT_INT,    &( debuffs.blood_frenzy              ) },
    { "crypt_fever",           OPT_INT,    &( debuffs.crypt_fever               ) },
    { "judgement_of_wisdom",   OPT_INT,    &( debuffs.judgement_of_wisdom       ) },
    { "mangle",                OPT_INT,    &( debuffs.mangle                    ) },
    { "razorice",              OPT_INT,    &( debuffs.razorice                  ) },
    { "snare",                 OPT_INT,    &( debuffs.snare                     ) },
    { "sunder_armor",          OPT_FLT,    &( debuffs.sunder_armor              ) },
    { "thunder_clap",          OPT_INT,    &( debuffs.thunder_clap              ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( sim, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}


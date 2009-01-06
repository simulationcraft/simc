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
  sim( s ), name_str( "Fluffy Pillow" ), level( 73 ), 
  initial_armor(6600), armor(0), block_value(0), shield(0), 
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

void target_t::assess_damage( double  amount, 
                              int8_t  school, 
                              int8_t  dmg_type )
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
}
   
// target_t::recalculate_health ==============================================

void target_t::recalculate_health()
{
  current_health = total_dmg;
  initial_health = current_health * ( sim -> max_time / sim -> current_time );
  
  if( sim -> debug ) report_t::log( sim, "Target initial health calculated to be %.0f", initial_health );     
}

// target_t::composite_armor =================================================

double target_t::composite_armor( player_t* p )
{
  double adjusted_armor = armor;

  adjusted_armor -= debuffs.sunder_armor;
  adjusted_armor -= debuffs.faerie_fire;

  if( p -> buffs.executioner ) adjusted_armor -= 840;

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
  uptimes.winters_grasp   = get_uptime( "winters_grasp"   );
  uptimes.winters_chill   = get_uptime( "winters_chill"   );
  uptimes.improved_scorch = get_uptime( "improved_scorch" );
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

// target_t::parse_option ======================================================

bool target_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    // General
    { "target_level",          OPT_INT8,  &( level                             ) },
    { "target_resist_holy",    OPT_INT16, &( spell_resistance[ SCHOOL_HOLY   ] ) },
    { "target_resist_shadow",  OPT_INT16, &( spell_resistance[ SCHOOL_SHADOW ] ) },
    { "target_resist_arcane",  OPT_INT16, &( spell_resistance[ SCHOOL_ARCANE ] ) },
    { "target_resist_frost",   OPT_INT16, &( spell_resistance[ SCHOOL_FROST  ] ) },
    { "target_resist_fire",    OPT_INT16, &( spell_resistance[ SCHOOL_FIRE   ] ) },
    { "target_resist_nature",  OPT_INT16, &( spell_resistance[ SCHOOL_NATURE ] ) },
    { "target_armor",          OPT_INT16, &( initial_armor                     ) },
    { "target_shield",         OPT_INT8,  &( shield                            ) },
    { "target_block",          OPT_INT16, &( block_value                       ) },
    { "target_health",         OPT_FLT,   &( initial_health                    ) },
    // FIXME! Once appropriate class implemented, these will be removed
    { "blood_frenzy",          OPT_INT8,  &( debuffs.blood_frenzy              ) },
    { "crypt_fever",           OPT_INT8,  &( debuffs.crypt_fever               ) },
    { "judgement_of_wisdom",   OPT_INT8,  &( debuffs.judgement_of_wisdom       ) },
    { "mangle",                OPT_INT8,  &( debuffs.mangle                    ) },
    { "razorice",              OPT_INT8,  &( debuffs.razorice                  ) },
    { "savage_combat",         OPT_INT8,  &( debuffs.savage_combat             ) },
    { "snare",                 OPT_INT8,  &( debuffs.snare                     ) },
    { "sunder_armor",          OPT_FLT,   &( debuffs.sunder_armor              ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( sim, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}


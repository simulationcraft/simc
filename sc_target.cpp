// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Target
// ==========================================================================

// target_t::target_t =======================================================

target_t::target_t( sim_t* s ) :
  sim( s ), name_str( "Fluffy Pillow" ), level( 73 ), 
  initial_armor(6600), armor(0), block_value(0), shield(0), 
  initial_health( 0 ), current_health(0), total_dmg(0)
{
  for( int i=0; i < SCHOOL_MAX; i++ ) spell_resistance[ i ] = 0;
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

  if( dmg_type == DMG_DIRECT )
  {
    if( school == SCHOOL_SHADOW )
    {
      if( debuffs.shadow_vulnerability_charges > 0 )
      {
	debuffs.shadow_vulnerability_charges--;
	if( debuffs.shadow_vulnerability_charges == 0 ) 
	{
	  if( sim -> log ) report_t::log( sim, "%s loses Shadow Vulnerability", name() );
	  debuffs.shadow_vulnerability = 0;
	}
      }
    }

    if( school == SCHOOL_NATURE )
    {
      if( debuffs.nature_vulnerability_charges > 0 )
      {
	debuffs.nature_vulnerability_charges--;
	if( debuffs.nature_vulnerability_charges == 0 ) 
	{
	  if( sim -> log ) report_t::log( sim, "%s loses Nature Vulnerability", name() );
	  debuffs.nature_vulnerability = 0;
	}
      }
    }
  }
}
   
// target_t::recalculate_health ==============================================

void target_t::recalculate_health()
{
  current_health = total_dmg;
  initial_health = current_health * ( sim -> max_time / sim -> current_time );
  
  if( sim -> debug ) report_t::log( sim, "Target initial health calculated to be %.0f", initial_health );     
}

// target_t::init ============================================================

void target_t::init()
{
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
    { "curse_of_elements",     OPT_INT8,  &( debuffs.curse_of_elements         ) },
    { "judgement_of_crusader", OPT_INT16, &( debuffs.judgement_of_crusader     ) },
    { "judgement_of_wisdom",   OPT_FLT,   &( debuffs.judgement_of_wisdom       ) },
    { "shadow_vulnerability",  OPT_INT8,  &( debuffs.shadow_vulnerability      ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( sim, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}


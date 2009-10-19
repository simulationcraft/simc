// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace
{ // ANONYMOUS NAMESPACE ==========================================

// is_swing =================================================================

static bool is_swing( action_t* a )
{
  if ( a -> special ) return false;
  if ( a -> type != ACTION_ATTACK ) return false;
  if ( a -> weapon == 0 ) return false;
  if ( a -> weapon -> slot == SLOT_RANGED ) return false;
  return true;
}

// result_name ==============================================================

static const char* result_name( int result )
{
  switch ( result )
  {
  case RESULT_MISS:   return "MISS";
  case RESULT_RESIST: return "RESIST";
  case RESULT_DODGE:  return "DODGE";
  case RESULT_PARRY:  return "PARRY";
  case RESULT_BLOCK:  return "BLOCK";
  case RESULT_GLANCE: return "GLANCE";
  case RESULT_CRIT:   return "CRIT";
  case RESULT_HIT:    return "HIT";
  }
  assert( 0 );
  return 0;
}

// school_id ================================================================

static int school_id( int school )
{
  switch ( school )
  {
  case SCHOOL_ARCANE:    return 0x40;
  case SCHOOL_BLEED:     return 0x01;
  case SCHOOL_CHAOS:     return 0x02;
  case SCHOOL_FIRE:      return 0x04;
  case SCHOOL_FROST:     return 0x10;
  case SCHOOL_FROSTFIRE: return 0x14;
  case SCHOOL_HOLY:      return 0x02;
  case SCHOOL_NATURE:    return 0x08;
  case SCHOOL_PHYSICAL:  return 0x01;
  case SCHOOL_SHADOW:    return 0x20;
  default:               return 0x01;
  }
  return -1;
}

// resource_id ==============================================================

static int resource_id( int resource )
{
  switch ( resource )
  {
  case  RESOURCE_MANA:   return 0;
  case  RESOURCE_RAGE:   return 1;
  case  RESOURCE_ENERGY: return 3;
  case  RESOURCE_FOCUS:  return 2;
  case  RESOURCE_RUNIC:  return 6;
  }
  assert( 0 );
  return -1;
}

// default_id ===============================================================

static int default_id( sim_t*      sim,
                       const char* name )
{
  int offset = 1000000;
  int dictionary_size = ( int ) sim -> id_dictionary.size();

  for ( int i=0; i < dictionary_size; i++ )
    if ( sim -> id_dictionary[ i ] == name )
      return offset + i;

  sim -> id_dictionary.push_back( name );

  return offset + dictionary_size;
}

// nil_target_id ============================================================

static const char* nil_target_id()
{
  return "0x0000000000000000,nil,0x80000000";
}

// write_timestamp ==========================================================

static bool write_timestamp( sim_t* sim )
{
  if ( ! sim -> log_file ) return false;

  int hours, minutes, seconds, milisec;

  milisec  = ( int ) ( sim -> current_time * 1000 );
  seconds  = milisec / 1000;
  milisec %= 1000;
  minutes  = seconds / 60;
  seconds %= 60;
  hours    = minutes / 60;
  minutes %= 60;

  util_t::fprintf( sim -> log_file, "1/1 %02d:%02d:%02d.%03d  ", hours, minutes, seconds, milisec );

  return true;
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Log
// ==========================================================================

// log_t::output ============================================================

void log_t::output( sim_t* sim, const char* format, ... )
{
  va_list vap;
  char buffer[2048];

  va_start( vap, format );
  util_t::fprintf( sim -> output_file, "%-8.2f ", sim -> current_time );
  vsnprintf( buffer, 2047,  format, vap );
  buffer[2047] = '\0';
  util_t::fprintf( sim -> output_file, "%s", buffer );
  util_t::fprintf( sim -> output_file, "\n" );
  fflush( sim -> output_file );
}

// log_t::start_event =======================================================

void log_t::start_event( action_t* a )
{
  if ( ! write_timestamp( a -> sim ) ) return;

  util_t::fprintf( a -> sim -> log_file,
                   "SPELL_CAST_START,%s,%s,%d,\"%s\",0x%X\n",
                   a -> player -> id(),
                   nil_target_id(),
                   ( a -> id ? a -> id : default_id( a -> sim, a -> name() ) ),
                   a -> name(),
                   school_id( a -> school ) );
}

// log_t::damage_event ======================================================

void log_t::damage_event( action_t* a,
                          double    dmg,
                          int       dmg_type )
{
  if ( ! write_timestamp( a -> sim ) ) return;

  if ( is_swing( a ) )
  {
    util_t::fprintf( a -> sim -> log_file, "SWING_DAMAGE,%s,%s", a -> player -> id(), a -> sim -> target -> id() );
  }
  else
  {
    util_t::fprintf( a -> sim -> log_file, "%s,%s,%s,%d,\"%s\",0x%X",
                     ( ( dmg_type == DMG_DIRECT ) ? "SPELL_DAMAGE" : "SPELL_PERIODIC_DAMAGE" ),
                     a -> player -> id(),
                     a -> sim -> target -> id(),
                     ( a -> id ? a -> id : default_id( a -> sim, a -> name() ) ),
                     a -> name(),
                     school_id( a -> school ) );
  }

  util_t::fprintf( a -> sim -> log_file,
                   ",%d,0,%d,%d,0,%d,%s,%s,nil\n",
                   ( int ) dmg,
                   school_id( a -> school ),
                   ( int ) a -> resisted_dmg,
                   ( int ) a -> blocked_dmg,
                   ( ( a -> result == RESULT_CRIT   ) ? "1" : "nil" ),
                   ( ( a -> result == RESULT_GLANCE ) ? "1" : "nil" ) );

}

// log_t::miss_event ========================================================

void log_t::miss_event( action_t* a )
{
  if ( ! write_timestamp( a -> sim ) ) return;

  util_t::fprintf( a -> sim -> log_file,
                   "%s,%s,%s,%d,\"%s\",0x%X,%s\n",
                   ( is_swing( a ) ? "SWING_MISSED" : "SPELL_MISSED" ),
                   a -> player -> id(),
                   a -> player -> id(),
                   ( a -> id ? a -> id : default_id( a -> sim, a -> name() ) ),
                   a -> name(),
                   school_id( a -> school ),
                   result_name( a -> result ) );
}

// log_t::resource_gain_event ===============================================

void log_t::resource_gain_event( player_t* p,
                                 int       resource,
                                 double    amount,
                                 double    actual_amount,
                                 gain_t*   source )
{
  if ( ! write_timestamp( p -> sim ) ) return;

  // FIXME! Unable to tell from which player this resource gain occurred, so we assume it is from the player himself.

  if ( resource == RESOURCE_HEALTH )
  {
    util_t::fprintf( p -> sim -> log_file,
                     "SPELL_PERIODIC_HEAL,%s,%s,%d,\"%s\",0x%X,%d,%d,nil\n",
                     p -> id(),
                     p -> id(),
                     ( source -> id ? source -> id : default_id( p -> sim, source -> name() ) ),
                     source -> name(),
                     school_id( SCHOOL_PHYSICAL ),
                     ( int ) actual_amount,
                     ( int ) ( amount - actual_amount ) );
  }
  else
  {
    util_t::fprintf( p -> sim -> log_file,
                     "SPELL_ENERGIZE,%s,%s,%d,\"%s\",0x%X,%d,%d\n",
                     p -> id(),
                     p -> id(),
                     ( source -> id ? source -> id : default_id( p -> sim, source -> name() ) ),
                     source -> name(),
                     school_id( SCHOOL_PHYSICAL ),
                     ( int ) actual_amount,
                     resource_id( resource ) );
  }

}

// log_t::aura_gain_event ===================================================

void log_t::aura_gain_event( player_t*   p,
                             const char* name,
                             int         id )
{
  if ( ! write_timestamp( p -> sim ) ) return;

  util_t::fprintf( p -> sim -> log_file,
                   "SPELL_AURA_APPLIED,%s,%s,%d,\"%s\",0x%X,BUFF\n",
                   p -> id(),
                   p -> id(),
                   ( id ? id : default_id( p -> sim, name ) ),
                   name,
                   school_id( SCHOOL_PHYSICAL ) );
}

// log_t::aura_loss_event ===================================================

void log_t::aura_loss_event( player_t*   p,
                             const char* name,
                             int         id )
{
  if ( ! write_timestamp( p -> sim ) ) return;

  util_t::fprintf( p -> sim -> log_file,
                   "SPELL_AURA_REMOVED,%s,%s,%d,\"%s\",0x%X,BUFF\n",
                   p -> id(),
                   p -> id(),
                   ( id ? id : default_id( p -> sim, name ) ),
                   name,
                   school_id( SCHOOL_PHYSICAL ) );
}

// log_t::summon_event ======================================================

void log_t::summon_event( pet_t* pet )
{
  // Some parsers are sensitive to this when they need to bind pet to his owner, so several lines are generated

  if ( ! write_timestamp( pet -> sim ) ) return;

  util_t::fprintf( pet -> sim -> log_file,
                   "SPELL_SUMMON,%s,%s,688,\"Ritual Enslavement\",0x%X\n",
                   pet -> id(),
                   pet -> owner -> id(),
                   school_id( SCHOOL_SHADOW ) );

  write_timestamp( pet -> sim );

  util_t::fprintf( pet -> sim -> log_file,
                   "SPELL_SUMMON,%s,%s,688,\"Summon Pet\",0x%X\n",
                   pet -> id(),
                   pet -> owner -> id(),
                   school_id( SCHOOL_SHADOW ) );

  write_timestamp( pet -> sim );

  util_t::fprintf( pet -> sim -> log_file,
                   "SPELL_ENERGIZE,0,0,%s,%s,688,\"Summon Pet\",0x%X\n",
                   pet -> id(),
                   pet -> owner -> id(),
                   school_id( SCHOOL_SHADOW ) );
}

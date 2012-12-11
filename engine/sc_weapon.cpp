// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Weapon
// ==========================================================================

weapon_t::weapon_t( weapon_e t,
                    double d,
                    timespan_t st,
                    school_e s ) :
  type( t ),
  school( s ),
  damage( d ),
  dps( d / st.total_seconds() ),
  min_dmg( d ),
  max_dmg( d ),
  swing_time( st ),
  slot( SLOT_INVALID ),
  buff_type( 0 ),
  buff_value( 0 ),
  bonus_dmg( 0 )
{}

// weapon_t::group ==========================================================

weapon_e weapon_t::group()
{
  switch ( type )
  {
  case WEAPON_DAGGER:
  case WEAPON_SMALL:
    return WEAPON_SMALL;

  case WEAPON_SWORD:
  case WEAPON_MACE:
  case WEAPON_AXE:
  case WEAPON_FIST:
  case WEAPON_1H:
  case WEAPON_BEAST:
    return WEAPON_1H;

  case WEAPON_SWORD_2H:
  case WEAPON_MACE_2H:
  case WEAPON_AXE_2H:
  case WEAPON_STAFF:
  case WEAPON_POLEARM:
  case WEAPON_2H:
  case WEAPON_BEAST_2H:
    return WEAPON_2H;

  case WEAPON_BOW:
  case WEAPON_CROSSBOW:
  case WEAPON_GUN:
  case WEAPON_WAND:
  case WEAPON_THROWN:
  case WEAPON_RANGED:
    return WEAPON_RANGED;

  default:
    return WEAPON_NONE;
  }
}

// weapon_t::normalized_weapon_speed ========================================

timespan_t weapon_t::normalized_weapon_speed()
{
  int g = group();

  if ( g == WEAPON_SMALL  ) return timespan_t::from_seconds( 1.7 );
  if ( g == WEAPON_1H     ) return timespan_t::from_seconds( 2.4 );
  if ( g == WEAPON_2H     ) return timespan_t::from_seconds( 3.3 );
  if ( g == WEAPON_RANGED ) return timespan_t::from_seconds( 2.8 );

  assert( 0 );
  return timespan_t::zero();
}

// weapon_t::proc_chance_on_swing ===========================================

double weapon_t::proc_chance_on_swing( double PPM,
                                       timespan_t adjusted_swing_time )
{
  if ( adjusted_swing_time == timespan_t::zero() ) adjusted_swing_time = swing_time;

  timespan_t time_to_proc = timespan_t::from_seconds( 60.0 ) / PPM;
  double proc_chance = adjusted_swing_time / time_to_proc;

  return proc_chance;
}

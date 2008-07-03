// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Weapon
// ==========================================================================

// weapon_t::group ==========================================================

int weapon_t::group()
{
  switch( type )
  {
  case WEAPON_DAGGER:
  case WEAPON_FIST:
    return WEAPON_SMALL;

  case WEAPON_SWORD:
  case WEAPON_MACE:
  case WEAPON_AXE:
    return WEAPON_1H;

  case WEAPON_SWORD_2H:
  case WEAPON_MACE_2H:
  case WEAPON_AXE_2H:
  case WEAPON_STAFF:
  case WEAPON_POLEARM:
    return WEAPON_2H;

  case WEAPON_BOW:
  case WEAPON_CROSSBOW:
  case WEAPON_GUN:
  case WEAPON_WAND:
  case WEAPON_THROWN:
    return WEAPON_RANGED;
  }
  return WEAPON_NONE;
}

// weapon_t::normalized_weapon_speed ========================================

double weapon_t::normalized_weapon_speed()
{
  int g = group();

  if( g == WEAPON_SMALL ) return 1.7;
  if( g == WEAPON_1H    ) return 2.4;
  if( g == WEAPON_2H    ) return 3.3;

  assert(0);
  return 0;
}

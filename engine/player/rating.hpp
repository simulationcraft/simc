// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "dbc/dbc.hpp"
#include <cassert>
#include <string>


struct rating_t
{
  double  spell_haste,  spell_hit,  spell_crit;
  double attack_haste, attack_hit, attack_crit;
  double ranged_haste, ranged_hit, ranged_crit;
  double expertise;
  double dodge, parry, block;
  double mastery;
  double pvp_resilience, pvp_power;
  double damage_versatility, heal_versatility, mitigation_versatility;
  double leech, speed, avoidance;
  double corruption, corruption_resistance;

  double& get_mutable( rating_e r )
  {
    switch ( r )
    {
      case RATING_SPELL_HASTE: return spell_haste;
      case RATING_SPELL_HIT: return spell_hit;
      case RATING_SPELL_CRIT: return spell_crit;
      case RATING_MELEE_HASTE: return attack_haste;
      case RATING_MELEE_HIT: return attack_hit;
      case RATING_MELEE_CRIT: return attack_crit;
      case RATING_RANGED_HASTE: return ranged_haste;
      case RATING_RANGED_HIT: return ranged_hit;
      case RATING_RANGED_CRIT: return ranged_crit;
      case RATING_EXPERTISE: return expertise;
      case RATING_DODGE: return dodge;
      case RATING_PARRY: return parry;
      case RATING_BLOCK: return block;
      case RATING_MASTERY: return mastery;
      case RATING_PVP_POWER: return pvp_power;
      case RATING_PVP_RESILIENCE: return pvp_resilience;
      case RATING_DAMAGE_VERSATILITY: return damage_versatility;
      case RATING_HEAL_VERSATILITY: return heal_versatility;
      case RATING_MITIGATION_VERSATILITY: return mitigation_versatility;
      case RATING_LEECH: return leech;
      case RATING_SPEED: return speed;
      case RATING_AVOIDANCE: return avoidance;
      case RATING_CORRUPTION: return corruption;
      case RATING_CORRUPTION_RESISTANCE: return corruption_resistance;
      default: break;
    }
    assert( false ); return mastery;
  }

  double get( rating_e r ) const
  {
    switch ( r )
    {
      case RATING_SPELL_HASTE: return spell_haste;
      case RATING_SPELL_HIT: return spell_hit;
      case RATING_SPELL_CRIT: return spell_crit;
      case RATING_MELEE_HASTE: return attack_haste;
      case RATING_MELEE_HIT: return attack_hit;
      case RATING_MELEE_CRIT: return attack_crit;
      case RATING_RANGED_HASTE: return ranged_haste;
      case RATING_RANGED_HIT: return ranged_hit;
      case RATING_RANGED_CRIT: return ranged_crit;
      case RATING_EXPERTISE: return expertise;
      case RATING_DODGE: return dodge;
      case RATING_PARRY: return parry;
      case RATING_BLOCK: return block;
      case RATING_MASTERY: return mastery;
      case RATING_PVP_POWER: return pvp_power;
      case RATING_PVP_RESILIENCE: return pvp_resilience;
      case RATING_DAMAGE_VERSATILITY: return damage_versatility;
      case RATING_HEAL_VERSATILITY: return heal_versatility;
      case RATING_MITIGATION_VERSATILITY: return mitigation_versatility;
      case RATING_LEECH: return leech;
      case RATING_SPEED: return speed;
      case RATING_AVOIDANCE: return avoidance;
      case RATING_CORRUPTION: return corruption;
      case RATING_CORRUPTION_RESISTANCE: return corruption_resistance;
      default: break;
    }
    assert( false ); return mastery;
  }

  // Initialize all ratings to a very high number by default
  rating_t( double max = +1.0E+50 )
  {
    for ( rating_e i = static_cast<rating_e>( 0 ); i < RATING_MAX; ++i )
    {
      get_mutable( i ) = max;
    }
  }

  void init( dbc_t& dbc, int level )
  {
    // Read ratings from DBC
    for ( rating_e i = static_cast<rating_e>( 0 ); i < RATING_MAX; ++i )
    {
      get_mutable( i ) = dbc.combat_rating( i,  level );
      if ( i == RATING_MASTERY || i == RATING_CORRUPTION || i == RATING_CORRUPTION_RESISTANCE  )
        get_mutable( i ) /= 100.0;

      if ( get_mutable( i ) == 0.0 )
        get_mutable( i ) = 1.0;
    }
  }

  std::string to_string()
  {
    std::ostringstream s;
    for ( rating_e i = static_cast<rating_e>( 0 ); i < RATING_MAX; ++i )
    {
      if ( i > 0 ) s << " ";
      s << util::cache_type_string( cache_from_rating( i ) ) << "=" << get( i ); // hacky
    }
    return s.str();
  }
};
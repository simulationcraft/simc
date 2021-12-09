// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player/gear_stats.hpp"
#include "util/util.hpp"
#include <sstream>

// ==========================================================================
// Gear Stats
// ==========================================================================

// gear_stats_t::add_stat ===================================================

void gear_stats_t::add_stat( stat_e stat,
                             double      value )
{
  switch ( stat )
  {
    case STAT_NONE: break;

    case STAT_STRENGTH:  attribute[ ATTR_STRENGTH  ] += value; break;
    case STAT_AGILITY:   attribute[ ATTR_AGILITY   ] += value; break;
    case STAT_STAMINA:   attribute[ ATTR_STAMINA   ] += value; break;
    case STAT_INTELLECT: attribute[ ATTR_INTELLECT ] += value; break;
    case STAT_SPIRIT:    attribute[ ATTR_SPIRIT    ] += value; break;

    case STAT_AGI_INT:  attribute[ ATTR_AGI_INT ] += value; break;
    case STAT_STR_AGI:  attribute[ ATTR_STR_AGI ] += value; break;
    case STAT_STR_INT:  attribute[ ATTR_STR_INT ] += value; break;
    case STAT_STR_AGI_INT: attribute[ ATTR_STR_AGI_INT ] += value; break;

    case STAT_HEALTH: resource[ RESOURCE_HEALTH ] += value; break;
    case STAT_MANA:   resource[ RESOURCE_MANA   ] += value; break;
    case STAT_RAGE:   resource[ RESOURCE_RAGE   ] += value; break;
    case STAT_ENERGY: resource[ RESOURCE_ENERGY ] += value; break;
    case STAT_FOCUS:  resource[ RESOURCE_FOCUS  ] += value; break;
    case STAT_RUNIC:  resource[ RESOURCE_RUNIC_POWER  ] += value; break;

    case STAT_MAX_HEALTH:
    case STAT_MAX_MANA:
    case STAT_MAX_RAGE:
    case STAT_MAX_ENERGY:
    case STAT_MAX_FOCUS:
    case STAT_MAX_RUNIC:  break;

    case STAT_SPELL_POWER:       spell_power       += value; break;

    case STAT_ATTACK_POWER:             attack_power             += value; break;
    case STAT_EXPERTISE_RATING:         expertise_rating         += value; break;
    case STAT_EXPERTISE_RATING2:        expertise_rating2        += value; break;

    case STAT_HIT_RATING:   hit_rating   += value; break;
    case STAT_HIT_RATING2:  hit_rating2  += value; break;
    case STAT_CRIT_RATING:  crit_rating  += value; break;
    case STAT_HASTE_RATING: haste_rating += value; break;
    case STAT_VERSATILITY_RATING: versatility_rating += value; break;
    case STAT_LEECH_RATING: leech_rating += value; break;
    case STAT_SPEED_RATING: speed_rating += value; break;
    case STAT_AVOIDANCE_RATING: avoidance_rating += value; break;

    case STAT_WEAPON_DPS:   weapon_dps   += value; break;

    case STAT_WEAPON_OFFHAND_DPS:    weapon_offhand_dps    += value; break;

    case STAT_ARMOR:             armor          += value; break;
    case STAT_BONUS_ARMOR:       bonus_armor    += value; break;
    case STAT_DODGE_RATING:      dodge_rating   += value; break;
    case STAT_PARRY_RATING:      parry_rating   += value; break;

    case STAT_BLOCK_RATING: block_rating += value; break;

    case STAT_MASTERY_RATING: mastery_rating += value; break;

    case STAT_RESILIENCE_RATING: resilience_rating += value; break;
    case STAT_PVP_POWER:                 pvp_power += value; break;

    case STAT_CORRUPTION: corruption_rating += value; break;
    case STAT_CORRUPTION_RESISTANCE: corruption_resistance_rating += value; break;

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_STAT_ALL_MAX; i++ )
      { attribute[ i ] += value; }
      break;

    default: assert( 0 ); break;
  }
}

// gear_stats_t::set_stat ===================================================

void gear_stats_t::set_stat( stat_e stat,
                             double      value )
{
  switch ( stat )
  {
    case STAT_NONE: break;

    case STAT_STRENGTH:  attribute[ ATTR_STRENGTH  ] = value; break;
    case STAT_AGILITY:   attribute[ ATTR_AGILITY   ] = value; break;
    case STAT_STAMINA:   attribute[ ATTR_STAMINA   ] = value; break;
    case STAT_INTELLECT: attribute[ ATTR_INTELLECT ] = value; break;
    case STAT_SPIRIT:    attribute[ ATTR_SPIRIT    ] = value; break;

    case STAT_AGI_INT:  attribute[ ATTR_AGI_INT ] = value; break;
    case STAT_STR_AGI:  attribute[ ATTR_STR_AGI ] = value; break;
    case STAT_STR_INT:  attribute[ ATTR_STR_INT ] = value; break;
    case STAT_STR_AGI_INT: attribute[ ATTR_STR_AGI_INT ] = value; break;

    case STAT_HEALTH: resource[ RESOURCE_HEALTH ] = value; break;
    case STAT_MANA:   resource[ RESOURCE_MANA   ] = value; break;
    case STAT_RAGE:   resource[ RESOURCE_RAGE   ] = value; break;
    case STAT_ENERGY: resource[ RESOURCE_ENERGY ] = value; break;
    case STAT_FOCUS:  resource[ RESOURCE_FOCUS  ] = value; break;
    case STAT_RUNIC:  resource[ RESOURCE_RUNIC_POWER  ] = value; break;

    case STAT_MAX_HEALTH:
    case STAT_MAX_MANA:
    case STAT_MAX_RAGE:
    case STAT_MAX_ENERGY:
    case STAT_MAX_FOCUS:
    case STAT_MAX_RUNIC:  break;

    case STAT_SPELL_POWER:       spell_power       = value; break;

    case STAT_ATTACK_POWER:             attack_power             = value; break;
    case STAT_EXPERTISE_RATING:         expertise_rating         = value; break;
    case STAT_EXPERTISE_RATING2:        expertise_rating2        = value; break;

    case STAT_HIT_RATING:   hit_rating   = value; break;
    case STAT_HIT_RATING2:  hit_rating2  = value; break;
    case STAT_CRIT_RATING:  crit_rating  = value; break;
    case STAT_HASTE_RATING: haste_rating = value; break;
    case STAT_VERSATILITY_RATING: versatility_rating = value; break;
    case STAT_LEECH_RATING: leech_rating = value; break;
    case STAT_SPEED_RATING: speed_rating = value; break;
    case STAT_AVOIDANCE_RATING: avoidance_rating = value; break;

    case STAT_WEAPON_DPS:   weapon_dps   = value; break;

    case STAT_WEAPON_OFFHAND_DPS:    weapon_offhand_dps    = value; break;

    case STAT_ARMOR:             armor          = value; break;
    case STAT_BONUS_ARMOR:       bonus_armor    = value; break;
    case STAT_DODGE_RATING:      dodge_rating   = value; break;
    case STAT_PARRY_RATING:      parry_rating   = value; break;

    case STAT_BLOCK_RATING: block_rating = value; break;

    case STAT_MASTERY_RATING: mastery_rating = value; break;

    case STAT_RESILIENCE_RATING: resilience_rating = value; break;
    case STAT_PVP_POWER:                 pvp_power = value; break;

    case STAT_CORRUPTION: corruption_rating = value; break;
    case STAT_CORRUPTION_RESISTANCE: corruption_resistance_rating = value; break;

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_STAT_ALL_MAX; i++ )
      { attribute[ i ] = value; }
      break;

    default: assert( 0 ); break;
  }
}

// gear_stats_t::get_stat ===================================================

double gear_stats_t::get_stat( stat_e stat ) const
{
  switch ( stat )
  {
    case STAT_NONE: return default_value;

    case STAT_STRENGTH:  return attribute[ ATTR_STRENGTH  ];
    case STAT_AGILITY:   return attribute[ ATTR_AGILITY   ];
    case STAT_STAMINA:   return attribute[ ATTR_STAMINA   ];
    case STAT_INTELLECT: return attribute[ ATTR_INTELLECT ];
    case STAT_SPIRIT:    return attribute[ ATTR_SPIRIT    ];

    case STAT_AGI_INT:   return attribute[ ATTR_AGI_INT ];
    case STAT_STR_AGI:   return attribute[ ATTR_STR_AGI ];
    case STAT_STR_INT:   return attribute[ ATTR_STR_INT ];
    case STAT_STR_AGI_INT: return attribute[ ATTR_STR_AGI_INT ];

    case STAT_HEALTH: return resource[ RESOURCE_HEALTH ];
    case STAT_MANA:   return resource[ RESOURCE_MANA   ];
    case STAT_RAGE:   return resource[ RESOURCE_RAGE   ];
    case STAT_ENERGY: return resource[ RESOURCE_ENERGY ];
    case STAT_FOCUS:  return resource[ RESOURCE_FOCUS  ];
    case STAT_RUNIC:  return resource[ RESOURCE_RUNIC_POWER  ];

    case STAT_MAX_HEALTH:
    case STAT_MAX_MANA:
    case STAT_MAX_RAGE:
    case STAT_MAX_ENERGY:
    case STAT_MAX_FOCUS:
    case STAT_MAX_RUNIC:  return default_value;

    case STAT_SPELL_POWER:       return spell_power;

    case STAT_ATTACK_POWER:             return attack_power;
    case STAT_EXPERTISE_RATING:         return expertise_rating;
    case STAT_EXPERTISE_RATING2:        return expertise_rating2;

    case STAT_HIT_RATING:   return hit_rating;
    case STAT_HIT_RATING2:  return hit_rating2;
    case STAT_CRIT_RATING:  return crit_rating;
    case STAT_HASTE_RATING: return haste_rating;
    case STAT_VERSATILITY_RATING: return versatility_rating;
    case STAT_LEECH_RATING: return leech_rating;
    case STAT_SPEED_RATING: return speed_rating;
    case STAT_AVOIDANCE_RATING: return avoidance_rating;

    case STAT_WEAPON_DPS:   return weapon_dps;

    case STAT_WEAPON_OFFHAND_DPS:    return weapon_offhand_dps;

    case STAT_ARMOR:             return armor;
    case STAT_BONUS_ARMOR:       return bonus_armor;
    case STAT_DODGE_RATING:      return dodge_rating;
    case STAT_PARRY_RATING:      return parry_rating;

    case STAT_BLOCK_RATING: return block_rating;

    case STAT_MASTERY_RATING: return mastery_rating;

    case STAT_RESILIENCE_RATING: return resilience_rating;
    case STAT_PVP_POWER:         return pvp_power;

    case STAT_CORRUPTION: return corruption_rating;
    case STAT_CORRUPTION_RESISTANCE: return corruption_resistance_rating;

    case STAT_ALL: return default_value;

    default: assert( 0 ); break;
  }
  return default_value;
}

// gear_stats_t::stat_mod ===================================================

double gear_stats_t::stat_mod( stat_e stat )
{
  switch ( stat )
  {
    case STAT_ATTACK_POWER:      return 1.0;
    case STAT_SPELL_POWER:       return 1.0;
    default:                     return 1.0;
  }
}

void sc_format_to( const gear_stats_t& s, fmt::format_context::iterator out )
{
  for ( stat_e i = STAT_STRENGTH; i < STAT_MAX; i++ )
  {
    if ( i > 0 )
      fmt::format_to( out, " " );
    fmt::format_to( out, "{}={}", util::stat_type_abbrev( i ), s.get_stat( i ) );
  }
}

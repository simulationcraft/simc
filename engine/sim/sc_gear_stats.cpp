// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

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
    case STAT_MULTISTRIKE_RATING: multistrike_rating += value; break;
    case STAT_READINESS_RATING: readiness_rating += value; break;

    case STAT_WEAPON_DPS:   weapon_dps   += value; break;
    case STAT_WEAPON_SPEED: weapon_speed += value; break;

    case STAT_WEAPON_OFFHAND_DPS:    weapon_offhand_dps    += value; break;
    case STAT_WEAPON_OFFHAND_SPEED:  weapon_offhand_speed  += value; break;

    case STAT_ARMOR:             armor          += value; break;
    case STAT_BONUS_ARMOR:       bonus_armor    += value; break;
    case STAT_DODGE_RATING:      dodge_rating   += value; break;
    case STAT_PARRY_RATING:      parry_rating   += value; break;

    case STAT_BLOCK_RATING: block_rating += value; break;

    case STAT_MASTERY_RATING: mastery_rating += value; break;

    case STAT_RESILIENCE_RATING: resilience_rating += value; break;
    case STAT_PVP_POWER:                 pvp_power += value; break;

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
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
    case STAT_MULTISTRIKE_RATING: multistrike_rating = value; break;
    case STAT_READINESS_RATING: readiness_rating = value; break;

    case STAT_WEAPON_DPS:   weapon_dps   = value; break;
    case STAT_WEAPON_SPEED: weapon_speed = value; break;

    case STAT_WEAPON_OFFHAND_DPS:    weapon_offhand_dps    = value; break;
    case STAT_WEAPON_OFFHAND_SPEED:  weapon_offhand_speed  = value; break;

    case STAT_ARMOR:             armor          = value; break;
    case STAT_BONUS_ARMOR:       bonus_armor    = value; break;
    case STAT_DODGE_RATING:      dodge_rating   = value; break;
    case STAT_PARRY_RATING:      parry_rating   = value; break;

    case STAT_BLOCK_RATING: block_rating = value; break;

    case STAT_MASTERY_RATING: mastery_rating = value; break;

    case STAT_RESILIENCE_RATING: resilience_rating += value; break;
    case STAT_PVP_POWER:                 pvp_power += value; break;

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
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
    case STAT_NONE: return 0;

    case STAT_STRENGTH:  return attribute[ ATTR_STRENGTH  ];
    case STAT_AGILITY:   return attribute[ ATTR_AGILITY   ];
    case STAT_STAMINA:   return attribute[ ATTR_STAMINA   ];
    case STAT_INTELLECT: return attribute[ ATTR_INTELLECT ];
    case STAT_SPIRIT:    return attribute[ ATTR_SPIRIT    ];

    case STAT_AGI_INT:   return attribute[ ATTR_AGI_INT ];
    case STAT_STR_AGI:   return attribute[ ATTR_STR_AGI ];
    case STAT_STR_INT:   return attribute[ ATTR_STR_INT ];

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
    case STAT_MAX_RUNIC:  return 0;

    case STAT_SPELL_POWER:       return spell_power;

    case STAT_ATTACK_POWER:             return attack_power;
    case STAT_EXPERTISE_RATING:         return expertise_rating;
    case STAT_EXPERTISE_RATING2:        return expertise_rating2;

    case STAT_HIT_RATING:   return hit_rating;
    case STAT_HIT_RATING2:  return hit_rating2;
    case STAT_CRIT_RATING:  return crit_rating;
    case STAT_HASTE_RATING: return haste_rating;
    case STAT_MULTISTRIKE_RATING: return multistrike_rating;
    case STAT_READINESS_RATING: return readiness_rating;

    case STAT_WEAPON_DPS:   return weapon_dps;
    case STAT_WEAPON_SPEED: return weapon_speed;

    case STAT_WEAPON_OFFHAND_DPS:    return weapon_offhand_dps;
    case STAT_WEAPON_OFFHAND_SPEED:  return weapon_offhand_speed;

    case STAT_ARMOR:             return armor;
    case STAT_BONUS_ARMOR:       return bonus_armor;
    case STAT_DODGE_RATING:      return dodge_rating;
    case STAT_PARRY_RATING:      return parry_rating;

    case STAT_BLOCK_RATING: return block_rating;

    case STAT_MASTERY_RATING: return mastery_rating;

    case STAT_RESILIENCE_RATING: return resilience_rating;
    case STAT_PVP_POWER:         return pvp_power;

    case STAT_ALL: return 0;

    default: assert( 0 ); break;
  }
  return 0;
}

std::string gear_stats_t::to_string()
{
  std::ostringstream s;
  for ( stat_e i = STAT_STRENGTH; i < STAT_MAX; i++ )
  {
    if ( i > 0 )
      s << " ";
    s << util::stat_type_abbrev( i ) << "=" << get_stat( i );
  }
  return s.str();
}

// gear_stats_t::stat_mod ===================================================

double gear_stats_t::stat_mod( stat_e stat )
{
  switch ( stat )
  {
    case STAT_ATTACK_POWER:      return 0.50;
    case STAT_SPELL_POWER:       return 0.86;
    default:                     return 1.0;
  }
}

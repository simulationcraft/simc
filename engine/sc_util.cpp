// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#if !defined(SC_WINDOWS)
#include <sys/time.h>
#endif


#include <cerrno>

namespace { // anonymous namespace ==========================================

struct spec_map_t
{
  specialization_e spec;
  const char* name;
};

const spec_map_t spec_map[] =
{
  { WARRIOR_ARMS,         "Arms Warrior"         },
  { WARRIOR_FURY,         "Fury Warrior"         },
  { WARRIOR_PROTECTION,   "Protection Warrior"   },
  { PALADIN_HOLY,         "Holy Paladin"         },
  { PALADIN_PROTECTION,   "Protection Paladin"   },
  { PALADIN_RETRIBUTION,  "Retribution Paladin"  },
  { HUNTER_BEAST_MASTERY, "Beast Mastery Hunter" },
  { HUNTER_BEAST_MASTERY, "Beast-Mastery Hunter" }, // Alternate
  { HUNTER_MARKSMANSHIP,  "Marksmanship Hunter"  },
  { HUNTER_SURVIVAL,      "Survival Hunter"      },
  { ROGUE_ASSASSINATION,  "Assassination Rogue"  },
  { ROGUE_OUTLAW,         "Outlaw Rogue"         },
  { ROGUE_SUBTLETY,       "Subtlety Rogue"       },
  { PRIEST_DISCIPLINE,    "Discipline Priest"    },
  { PRIEST_HOLY,          "Holy Priest"          },
  { PRIEST_SHADOW,        "Shadow Priest"        },
  { DEATH_KNIGHT_BLOOD,   "Blood Death Knight"   }, // Default
  { DEATH_KNIGHT_BLOOD,   "Blood DeathKnight"    }, // Alternate (battle.net match)
  { DEATH_KNIGHT_FROST,   "Frost Death Knight"   }, // Default
  { DEATH_KNIGHT_FROST,   "Frost DeathKnight"    }, // Alternate (battle.net match)
  { DEATH_KNIGHT_UNHOLY,  "Unholy Death Knight"  }, // Default
  { DEATH_KNIGHT_UNHOLY,  "Unholy DeathKnight"   }, // Alternate (battle.net match)
  { SHAMAN_ELEMENTAL,     "Elemental Shaman"     },
  { SHAMAN_ENHANCEMENT,   "Enhancement Shaman"   },
  { SHAMAN_RESTORATION,   "Restoration Shaman"   },
  { MAGE_ARCANE,          "Arcane Mage"          },
  { MAGE_FIRE,            "Fire Mage"            },
  { MAGE_FROST,           "Frost Mage"           },
  { WARLOCK_AFFLICTION,   "Affliction Warlock"   },
  { WARLOCK_DEMONOLOGY,   "Demonology Warlock"   },
  { WARLOCK_DESTRUCTION,  "Destruction Warlock"  },
  { MONK_BREWMASTER,      "Brewmaster Monk"      },
  { MONK_MISTWEAVER,      "Mistweaver Monk"      },
  { MONK_WINDWALKER,      "Windwalker Monk"      },
  { DRUID_BALANCE,        "Balance Druid"        },
  { DRUID_FERAL,          "Feral Druid"          },
  { DRUID_GUARDIAN,       "Guardian Druid"       },
  { DRUID_RESTORATION,    "Restoration Druid"    },
  { DEMON_HUNTER_HAVOC,   "Havoc Demon Hunter"   },
  { DEMON_HUNTER_HAVOC,   "Havoc DemonHunter"    },
  { DEMON_HUNTER_VENGEANCE, "Vengeance Demon Hunter" },
  { DEMON_HUNTER_VENGEANCE, "Vengeance DemonHunter"  },
};

struct html_named_character_t
{
  const char* encoded;
  const char* decoded;
};

const html_named_character_t html_named_character_map[] =
{
  { "amp", "&" },
  { "gt", ">" },
  { "lt", "<" },
  { "quot", "\"" },
};

// parse_enum ===============================================================

template <typename T, T Min, T Max, const char* F( T )>
inline T parse_enum( const std::string& name )
{
  for ( T i = Min; i < Max; ++i )
    if ( util::str_compare_ci( name, F( i ) ) )
      return i;
  return Min;
}

// pred_ci ==================================================================

bool pred_ci ( char a, char b )
{
  return std::tolower( a ) == std::tolower( b );
}

// vfprintf_helper ==========================================================

int vfprintf_helper( FILE *stream, const char *fmt, va_list args )
{
  std::string p_locale = setlocale( LC_CTYPE, nullptr );
  setlocale( LC_CTYPE, "" );

  int retcode = ::fprintf( stream, "%s", str::format( fmt, args ).c_str() );

  setlocale( LC_CTYPE, p_locale.c_str() );

  return retcode;
}

stopwatch_t wall_sw( STOPWATCH_WALL );
stopwatch_t  cpu_sw( STOPWATCH_CPU  );

} // anonymous namespace ============================================


double util::wall_time() { return wall_sw.elapsed(); }
double util::cpu_time() { return cpu_sw.elapsed(); }

/// case-insensitive string comparison
bool util::str_compare_ci( const std::string& l,
                           const std::string& r )
{
  if ( l.size() != r.size() )
    return false;
  else
    return std::equal( l.begin(), l.end(), r.begin(), pred_ci );
}

// glyph_name ===============================================================

std::string& util::glyph_name( std::string& n )
{
  tokenize( n );

  if ( n.compare( 0, 7, "glyph__" ) == 0 )
    n.erase( 0, 7 );
  else if ( n.compare( 0, 13, "glyph_of_the_" ) == 0 )
    n.erase( 0, 13 );
  else if ( n.compare( 0, 9, "glyph_of_" ) == 0 )
    n.erase( 0, 9 );

  return n;
}

// str_prefix_ci ============================================================

bool util::str_prefix_ci( const std::string& str,
                          const std::string& prefix )
{
  if ( str.size() < prefix.size() )
    return false;
  else
    return std::equal( prefix.begin(), prefix.end(), str.begin(), pred_ci );
}

// str_in_str_ci ============================================================

bool util::str_in_str_ci( const std::string& l,
                          const std::string& r )
{
  return std::search( l.begin(), l.end(), r.begin(), r.end(), pred_ci ) != l.end();
}

// rating_t::interpolate ====================================================

double util::interpolate( int    level,
                          double val_60,
                          double val_70,
                          double val_80,
                          double val_85 )
{
  if ( val_85 < 0 ) val_85 = val_80; // TODO
  if ( level <= 60 )
  {
    return val_60;
  }
  else if ( level == 70 )
  {
    return val_70;
  }
  else if ( level == 80 )
  {
    return val_80;
  }
  else if ( level >= 85 )
  {
    return val_85;
  }
  else if ( level < 70 )
  {
    // Assume linear progression for now.
    double adjust = ( level - 60 ) / 10.0;
    return val_60 + adjust * ( val_70 - val_60 );
  }
  else if ( level < 80 )
  {
    // Assume linear progression for now.
    double adjust = ( level - 70 ) / 10.0;
    return val_70 + adjust * ( val_80 - val_70 );
  }
  else // ( level < 85 )
  {
    // Assume linear progression for now.
    double adjust = ( level - 80 ) / 5.0;
    return val_80 + adjust * ( val_85 - val_80 );
  }
  assert( 0 );
  return 0;
}

// dot_behavior_type_string =================================================

const char* util::dot_behavior_type_string( dot_behavior_e t )
{
  switch ( t )
  {
    case DOT_REFRESH: return "DOT_REFRESH";
    case DOT_CLIP:    return "DOT_CLIP";
    case DOT_EXTEND:  return "DOT_EXTEND";
    default:          return "unknown";
  }
}

// role_type_string =========================================================

const char* util::role_type_string( role_e role )
{
  switch ( role )
  {
    case ROLE_ATTACK:    return "attack";
    case ROLE_SPELL:     return "spell";
    case ROLE_HYBRID:    return "hybrid";
    case ROLE_DPS:       return "dps";
    case ROLE_TANK:      return "tank";
    case ROLE_HEAL:      return "heal";
    case ROLE_NONE:      return "auto";
    default:             return "unknown";
  }
}

// parse_role_type ==========================================================

role_e util::parse_role_type( const std::string& name )
{
  return parse_enum<role_e, ROLE_NONE, ROLE_MAX, role_type_string>( name );
}

// race_type_string =========================================================

const char* util::race_type_string( race_e type )
{
  switch ( type )
  {
    case RACE_NONE:      return "none";
    case RACE_BEAST:     return "beast";
    case RACE_BLOOD_ELF: return "blood_elf";
    case RACE_DEMON:     return "demon";
    case RACE_DRAENEI:   return "draenei";
    case RACE_DRAGONKIN: return "dragonkin";
    case RACE_DWARF:     return "dwarf";
    case RACE_GIANT:     return "giant";
    case RACE_GNOME:     return "gnome";
    case RACE_HUMAN:     return "human";
    case RACE_HUMANOID:  return "humanoid";
    case RACE_NIGHT_ELF: return "night_elf";
    case RACE_ORC:       return "orc";
    case RACE_TAUREN:    return "tauren";
    case RACE_TROLL:     return "troll";
    case RACE_UNDEAD:    return "undead";
    case RACE_GOBLIN:    return "goblin";
    case RACE_WORGEN:    return "worgen";
    case RACE_PANDAREN:  return "pandaren";
    case RACE_PANDAREN_ALLIANCE:  return "pandaren_alliance";
    case RACE_PANDAREN_HORDE:  return "pandaren_horde";
    default:             return "unknown";
  }
}

// race_type_string =========================================================

const char* util::stats_type_string( stats_e type )
{
  switch ( type )
  {
    case STATS_NEUTRAL:  return "neutral";
    case STATS_DMG:      return "damage";
    case STATS_HEAL:     return "heal";
    case STATS_ABSORB:   return "absorb";
    default:             return "unknown";
  }
}

// parse_race_type ==========================================================

race_e util::parse_race_type( const std::string &name )
{
  if ( name == "forsaken" ) return RACE_UNDEAD;

  return parse_enum<race_e, RACE_NONE, RACE_MAX, race_type_string>( name );
}

// position_type_string =====================================================

const char* util::position_type_string( position_e type )
{
  switch ( type )
  {
    case POSITION_NONE:         return "none";
    case POSITION_BACK:         return "back";
    case POSITION_FRONT:        return "front";
    case POSITION_RANGED_BACK:  return "ranged_back";
    case POSITION_RANGED_FRONT: return "ranged_front";
    default:                    return "unknown";
  }
}

// parse_position_type ======================================================

position_e util::parse_position_type( const std::string &name )
{
  return parse_enum<position_e, POSITION_NONE, POSITION_MAX, position_type_string>( name );
}

// profession_type_string ===================================================

const char* util::profession_type_string( profession_e type )
{
  switch ( type )
  {
    case PROFESSION_NONE:     return "none";
    case PROF_ALCHEMY:        return "alchemy";
    case PROF_BLACKSMITHING:  return "blacksmithing";
    case PROF_ENCHANTING:     return "enchanting";
    case PROF_ENGINEERING:    return "engineering";
    case PROF_HERBALISM:      return "herbalism";
    case PROF_INSCRIPTION:    return "inscription";
    case PROF_JEWELCRAFTING:  return "jewelcrafting";
    case PROF_LEATHERWORKING: return "leatherworking";
    case PROF_MINING:         return "mining";
    case PROF_SKINNING:       return "skinning";
    case PROF_TAILORING:      return "tailoring";
    default:                  return "unknown";
  }
}

// parse_profession_type ====================================================

profession_e util::parse_profession_type( const std::string& name )
{
  return parse_enum<profession_e, PROFESSION_NONE, PROFESSION_MAX, profession_type_string>( name );
}

// translate_profession_id ==================================================

profession_e util::translate_profession_id( int skill_id )
{
  switch ( skill_id )
  {
    case 164: return PROF_BLACKSMITHING;
    case 165: return PROF_LEATHERWORKING;
    case 171: return PROF_ALCHEMY;
    case 182: return PROF_HERBALISM;
    case 186: return PROF_MINING;
    case 197: return PROF_TAILORING;
    case 202: return PROF_ENGINEERING;
    case 333: return PROF_ENCHANTING;
    case 393: return PROF_SKINNING;
    case 755: return PROF_JEWELCRAFTING;
    case 773: return PROF_INSCRIPTION;
  }
  return PROFESSION_NONE;
}

// player_type_string =======================================================

const char* util::player_type_string( player_e type )
{
  switch ( type )
  {
    case PLAYER_NONE:     return "none";
    case DEATH_KNIGHT:    return "deathknight";
    case DEMON_HUNTER:    return "demonhunter";
    case DRUID:           return "druid";
    case HUNTER:          return "hunter";
    case MAGE:            return "mage";
    case MONK:            return "monk";
    case PALADIN:         return "paladin";
    case PRIEST:          return "priest";
    case ROGUE:           return "rogue";
    case SHAMAN:          return "shaman";
    case WARLOCK:         return "warlock";
    case WARRIOR:         return "warrior";
    case PLAYER_PET:      return "pet";
    case PLAYER_GUARDIAN: return "guardian";
    case ENEMY:           return "enemy";
    case ENEMY_ADD:       return "add";
    case TMI_BOSS:        return "tmi_boss";
    case TANK_DUMMY:      return "tank_dummy";
    default:              return "unknown";
  }
}

// parse_player_type ========================================================

player_e util::parse_player_type( const std::string& name )
{
  return parse_enum<player_e, PLAYER_NONE, PLAYER_MAX, player_type_string>( name );
}

// translate_class_str ======================================================

player_e util::translate_class_str( const std::string& s )
{
  return parse_enum<player_e, PLAYER_NONE, PLAYER_MAX, player_type_string>( s );
}


// pet_type_string ==========================================================

const char* util::pet_type_string( pet_e type )
{
  switch ( type )
  {
    case PET_NONE:                return "none";
    case PET_CARRION_BIRD:        return "carrion_bird";
    case PET_CAT:                 return "cat";
    case PET_CORE_HOUND:          return "core_hound";
    case PET_DEVILSAUR:           return "devilsaur";
    case PET_FOX:                 return "fox";
    case PET_HYENA:               return "hyena";
    case PET_MOTH:                return "moth";
    case PET_MONKEY:              return "monkey";
    case PET_DOG:                 return "dog";
    case PET_BEETLE:              return "beetle";
    case PET_RAPTOR:              return "raptor";
    case PET_SPIRIT_BEAST:        return "spirit_beast";
    case PET_TALLSTRIDER:         return "tallstrider";
    case PET_WASP:                return "wasp";
    case PET_WOLF:                return "wolf";
    case PET_BEAR:                return "bear";
    case PET_BOAR:                return "boar";
    case PET_CRAB:                return "crab";
    case PET_CROCOLISK:           return "crocolisk";
    case PET_GORILLA:             return "gorilla";
    case PET_RHINO:               return "rhino";
    case PET_SCORPID:             return "scorpid";
    case PET_SHALE_SPIDER:        return "shale_spider";
    case PET_TURTLE:              return "turtle";
    case PET_WARP_STALKER:        return "warp_stalker";
    case PET_WORM:                return "worm";
    //case PET_RIVERBEAST:          return "riverbeast";
    case PET_BAT:                 return "bat";
    case PET_BIRD_OF_PREY:        return "bird_of_prey";
    case PET_CHIMAERA:            return "chimaera";
    case PET_DRAGONHAWK:          return "dragonhawk";
    case PET_NETHER_RAY:          return "nether_ray";
    case PET_RAVAGER:             return "ravager";
    case PET_SERPENT:             return "serpent";
    case PET_SILITHID:            return "silithid";
    case PET_SPIDER:              return "spider";
    case PET_SPOREBAT:            return "sporebat";
    case PET_WIND_SERPENT:        return "wind_serpent";
    case PET_FELGUARD:            return "felguard";
    case PET_FELHUNTER:           return "felhunter";
    case PET_IMP:                 return "imp";
    case PET_VOIDWALKER:          return "voidwalker";
    case PET_SUCCUBUS:            return "succubus";
    case PET_INFERNAL:            return "infernal";
    case PET_DOOMGUARD:           return "doomguard";
    case PET_WILD_IMP:            return "wild_imp";
    case PET_DREADSTALKER:        return "dreadstalker";
    case PET_SERVICE_IMP:         return "service_imp";
    case PET_SERVICE_FELHUNTER:   return "service_felhunter";
    case PET_OBSERVER:            return "observer";
    case PET_GHOUL:               return "ghoul";
    case PET_BLOODWORMS:          return "bloodworms";
    case PET_DANCING_RUNE_WEAPON: return "dancing_rune_weapon";
    case PET_TREANTS:             return "treants";
    case PET_WATER_ELEMENTAL:     return "water_elemental";
    case PET_SHADOWFIEND:         return "shadowfiend";
    case PET_SPIRIT_WOLF:         return "spirit_wolf";
    case PET_FIRE_ELEMENTAL:      return "fire_elemental";
    case PET_EARTH_ELEMENTAL:     return "earth_elemental";
    case PET_ENEMY:               return "pet_enemy";
    default:                      return "unknown";
  }
}

// parse_pet_type ===========================================================

pet_e util::parse_pet_type( const std::string& name )
{
  return parse_enum<pet_e, PET_NONE, PET_MAX, pet_type_string>( name );
}

// attribute_type_string ====================================================

const char* util::attribute_type_string( attribute_e type )
{
  switch ( type )
  {
    case ATTR_STRENGTH:  return "strength";
    case ATTR_AGILITY:   return "agility";
    case ATTR_STAMINA:   return "stamina";
    case ATTR_INTELLECT: return "intellect";
    case ATTR_SPIRIT:    return "spirit";
    case ATTR_AGI_INT:   return "agility/intellect";
    case ATTR_STR_AGI:   return "strength/agility";
    case ATTR_STR_INT:   return "strength/intellect";
    case ATTR_STR_AGI_INT: return "strength/agility/intellect";
    default:             return "unknown";
  }
}

// parse_attribute_type =====================================================

attribute_e util::parse_attribute_type( const std::string& name )
{
  return parse_enum<attribute_e, ATTRIBUTE_NONE, ATTRIBUTE_MAX, attribute_type_string>( name );
}

// meta_gem_type_string =====================================================

const char* util::meta_gem_type_string( meta_gem_e type )
{
  switch ( type )
  {
    case META_AGILE_SHADOWSPIRIT:         return "agile_shadowspirit";
    case META_AGILE_PRIMAL:               return "agile_primal";
    case META_AUSTERE_EARTHSIEGE:         return "austere_earthsiege";
    case META_AUSTERE_SHADOWSPIRIT:       return "austere_shadowspirit";
    case META_AUSTERE_PRIMAL:             return "austere_primal";
    case META_BEAMING_EARTHSIEGE:         return "beaming_earthsiege";
    case META_BRACING_EARTHSIEGE:         return "bracing_earthsiege";
    case META_BRACING_EARTHSTORM:         return "bracing_earthstorm";
    case META_BRACING_SHADOWSPIRIT:       return "bracing_shadowspirit";
    case META_BURNING_SHADOWSPIRIT:       return "burning_shadowspirit";
    case META_BURNING_PRIMAL:             return "burning_primal";
    case META_CHAOTIC_SHADOWSPIRIT:       return "chaotic_shadowspirit";
    case META_CHAOTIC_SKYFIRE:            return "chaotic_skyfire";
    case META_CHAOTIC_SKYFLARE:           return "chaotic_skyflare";
    case META_DESTRUCTIVE_SHADOWSPIRIT:   return "destructive_shadowspirit";
    case META_DESTRUCTIVE_PRIMAL:         return "destructive_primal";
    case META_DESTRUCTIVE_SKYFIRE:        return "destructive_skyfire";
    case META_DESTRUCTIVE_SKYFLARE:       return "destructive_skyflare";
    case META_EFFULGENT_SHADOWSPIRIT:     return "effulgent_shadowspirit";
    case META_EFFULGENT_PRIMAL:           return "effulgent_primal";
    case META_EMBER_SHADOWSPIRIT:         return "ember_shadowspirit";
    case META_EMBER_PRIMAL:               return "ember_primal";
    case META_EMBER_SKYFIRE:              return "ember_skyfire";
    case META_EMBER_SKYFLARE:             return "ember_skyflare";
    case META_ENIGMATIC_SHADOWSPIRIT:     return "enigmatic_shadowspirit";
    case META_ENIGMATIC_PRIMAL:           return "enigmatic_primal";
    case META_ENIGMATIC_SKYFLARE:         return "enigmatic_skyflare";
    case META_ENIGMATIC_STARFLARE:        return "enigmatic_starflare";
    case META_ENIGMATIC_SKYFIRE:          return "enigmatic_skyfire";
    case META_ETERNAL_EARTHSIEGE:         return "eternal_earthsiege";
    case META_ETERNAL_EARTHSTORM:         return "eternal_earthstorm";
    case META_ETERNAL_SHADOWSPIRIT:       return "eternal_shadowspirit";
    case META_ETERNAL_PRIMAL:             return "eternal_primal";
    case META_FLEET_SHADOWSPIRIT:         return "fleet_shadowspirit";
    case META_FLEET_PRIMAL:               return "fleet_primal";
    case META_FORLORN_SHADOWSPIRIT:       return "forlorn_shadowspirit";
    case META_FORLORN_PRIMAL:             return "forlorn_primal";
    case META_FORLORN_SKYFLARE:           return "forlorn_skyflare";
    case META_FORLORN_STARFLARE:          return "forlorn_starflare";
    case META_IMPASSIVE_SHADOWSPIRIT:     return "impassive_shadowspirit";
    case META_IMPASSIVE_PRIMAL:           return "impassive_primal";
    case META_IMPASSIVE_SKYFLARE:         return "impassive_skyflare";
    case META_IMPASSIVE_STARFLARE:        return "impassive_starflare";
    case META_INSIGHTFUL_EARTHSIEGE:      return "insightful_earthsiege";
    case META_INSIGHTFUL_EARTHSTORM:      return "insightful_earthstorm";
    case META_INVIGORATING_EARTHSIEGE:    return "invigorating_earthsiege";
    case META_MYSTICAL_SKYFIRE:           return "mystical_skyfire";
    case META_PERSISTENT_EARTHSHATTER:    return "persistent_earthshatter";
    case META_PERSISTENT_EARTHSIEGE:      return "persistent_earthsiege";
    case META_POWERFUL_EARTHSHATTER:      return "powerful_earthshatter";
    case META_POWERFUL_EARTHSIEGE:        return "powerful_earthsiege";
    case META_POWERFUL_EARTHSTORM:        return "powerful_earthstorm";
    case META_POWERFUL_SHADOWSPIRIT:      return "powerful_shadowspirit";
    case META_POWERFUL_PRIMAL:            return "powerful_primal";
    case META_RELENTLESS_EARTHSIEGE:      return "relentless_earthsiege";
    case META_RELENTLESS_EARTHSTORM:      return "relentless_earthstorm";
    case META_REVERBERATING_SHADOWSPIRIT: return "reverberating_shadowspirit";
    case META_REVERBERATING_PRIMAL:       return "reverberating_primal";
    case META_REVITALIZING_SHADOWSPIRIT:  return "revitalizing_shadowspirit";
    case META_REVITALIZING_PRIMAL:        return "revitalizing_primal";
    case META_REVITALIZING_SKYFLARE:      return "revitalizing_skyflare";
    case META_SWIFT_SKYFIRE:              return "swift_skyfire";
    case META_SWIFT_SKYFLARE:             return "swift_skyflare";
    case META_SWIFT_STARFIRE:             return "swift_starfire";
    case META_SWIFT_STARFLARE:            return "swift_starflare";
    case META_THUNDERING_SKYFIRE:         return "thundering_skyfire";
    case META_THUNDERING_SKYFLARE:        return "thundering_skyflare";
    case META_TIRELESS_STARFLARE:         return "tireless_starflare";
    case META_TIRELESS_SKYFLARE:          return "tireless_skyflare";
    case META_TRENCHANT_EARTHSHATTER:     return "trenchant_earthshatter";
    case META_TRENCHANT_EARTHSIEGE:       return "trenchant_earthsiege";
    case META_SINISTER_PRIMAL:            return "sinister_primal";
    case META_CAPACITIVE_PRIMAL:          return "capacitive_primal";
    case META_INDOMITABLE_PRIMAL:         return "indomitable_primal";
    case META_COURAGEOUS_PRIMAL:          return "courageous_primal";
    default:                              return "unknown";
  }
}

// parse_meta_gem_type ======================================================

meta_gem_e util::parse_meta_gem_type( const std::string& name )
{
  return parse_enum<meta_gem_e, META_GEM_NONE, META_GEM_MAX, meta_gem_type_string>( name );
}

// result_type_string =======================================================

const char* util::result_type_string( result_e type )
{
  switch ( type )
  {
    case RESULT_NONE:       return "none";
    case RESULT_MISS:       return "miss";
    case RESULT_DODGE:      return "dodge";
    case RESULT_PARRY:      return "parry";
    case RESULT_GLANCE:     return "glance";
    case RESULT_CRIT:       return "crit";
    case RESULT_HIT:        return "hit";
    default:                return "unknown";
  }
}

// block_result_type_string =================================================

const char* util::block_result_type_string( block_result_e type )
{
  switch ( type )
  {
    case BLOCK_RESULT_UNBLOCKED:     return "unblocked or avoided";
    case BLOCK_RESULT_BLOCKED:       return "blocked";
    case BLOCK_RESULT_CRIT_BLOCKED:  return "crit-blocked";
    default:                         return "unknown";
  }
}

// full_result_type_string ==================================================

const char* util::full_result_type_string( full_result_e fulltype )
{
  switch ( fulltype )
  {
    case FULLTYPE_NONE:             return "none";
    case FULLTYPE_MISS:             return "miss";
    case FULLTYPE_DODGE:            return "dodge";
    case FULLTYPE_PARRY:            return "parry";
    case FULLTYPE_GLANCE_CRITBLOCK: return "glance (crit blocked)";
    case FULLTYPE_GLANCE_BLOCK:     return "glance (blocked)";
    case FULLTYPE_GLANCE:           return "glance";
    case FULLTYPE_CRIT_CRITBLOCK:   return "crit (crit blocked)";
    case FULLTYPE_CRIT_BLOCK:       return "crit (blocked)";
    case FULLTYPE_CRIT:             return "crit";
    case FULLTYPE_HIT_CRITBLOCK:    return "hit (crit blocked)";
    case FULLTYPE_HIT_BLOCK:        return "hit (blocked)";
    case FULLTYPE_HIT:              return "hit";
    default:                        return "unknown";
  }

}

// amount_type_string =======================================================

const char* util::amount_type_string( dmg_e type )
{
  switch ( type )
  {
    case RESULT_TYPE_NONE: return "none";
    case DMG_DIRECT:       return "direct_damage";
    case DMG_OVER_TIME:    return "tick_damage";
    case HEAL_DIRECT:      return "direct_heal";
    case HEAL_OVER_TIME:   return "tick_heal";
    case ABSORB:           return "absorb";
    default:               return "unknown";
  }
}

// parse_result_type ========================================================

result_e util::parse_result_type( const std::string& name )
{
  return parse_enum<result_e, RESULT_NONE, RESULT_MAX, result_type_string>( name );
}

// resource_type_string =====================================================

const char* util::resource_type_string( resource_e resource_type )
{
  switch ( resource_type )
  {
    case RESOURCE_NONE:          return "none";
    case RESOURCE_HEALTH:        return "health";
    case RESOURCE_MANA:          return "mana";
    case RESOURCE_RAGE:          return "rage";
    case RESOURCE_ASTRAL_POWER:  return "astral_power";
    case RESOURCE_ENERGY:        return "energy";
    case RESOURCE_FOCUS:         return "focus";
    case RESOURCE_RUNIC_POWER:   return "runic_power";
    case RESOURCE_RUNE:          return "rune";
    case RESOURCE_SOUL_SHARD:    return "soul_shard";
    case RESOURCE_HOLY_POWER:    return "holy_power";
    case RESOURCE_CHI:           return "chi";
    case RESOURCE_COMBO_POINT:   return "combo_points";
    case RESOURCE_MAELSTROM:     return "maelstrom";
    case RESOURCE_FURY:          return "fury";
    case RESOURCE_PAIN:          return "pain";
    case RESOURCE_INSANITY:      return "insanity";
    default:                     return "unknown";
  }
}

// parse_resource_type ======================================================

resource_e util::parse_resource_type( const std::string& name )
{
  return parse_enum<resource_e, RESOURCE_NONE, RESOURCE_MAX, resource_type_string>( name );
}

// school_type_component ====================================================

uint32_t util::school_type_component( school_e s_type, school_e c_type )
{
  return( dbc::get_school_mask( s_type ) &
          dbc::get_school_mask( c_type ) );
}

// school_type_string =======================================================

const char* util::school_type_string( school_e school )
{
  switch ( school )
  {
    case SCHOOL_ASTRAL:           return "astral";
    case SCHOOL_ARCANE:           return "arcane";
    case SCHOOL_CHAOS:            return "chaos";
    case SCHOOL_FIRE:             return "fire";
    case SCHOOL_FROST:            return "frost";
    case SCHOOL_FROSTFIRE:        return "frostfire";
    case SCHOOL_HOLY:             return "holy";
    case SCHOOL_NATURE:           return "nature";
    case SCHOOL_PHYSICAL:         return "physical";
    case SCHOOL_SHADOW:           return "shadow";
    case SCHOOL_HOLYSTRIKE:       return "holystrike";
    case SCHOOL_FLAMESTRIKE:      return "flamestrike";
    case SCHOOL_HOLYFIRE:         return "holyfire";
    case SCHOOL_STORMSTRIKE:      return "stormstrike";
    case SCHOOL_HOLYSTORM:        return "holystorm";
    case SCHOOL_FIRESTORM:        return "firestorm";
    case SCHOOL_FROSTSTRIKE:      return "froststrike";
    case SCHOOL_HOLYFROST:        return "holyfrost";
    case SCHOOL_FROSTSTORM:       return "froststorm";
    case SCHOOL_SHADOWSTRIKE:     return "shadowstrike";
    case SCHOOL_SHADOWLIGHT:      return "shadowlight";
    case SCHOOL_SHADOWFLAME:      return "shadowflame";
    case SCHOOL_SHADOWSTORM:      return "shadowstorm";
    case SCHOOL_SHADOWFROST:      return "shadowfrost";
    case SCHOOL_SPELLSTRIKE:      return "spellstrike";
    case SCHOOL_DIVINE:           return "divine";
    case SCHOOL_SPELLFIRE:        return "spellfire";
    case SCHOOL_SPELLFROST:       return "spellfrost";
    case SCHOOL_SPELLSHADOW:      return "spellshadow";
    case SCHOOL_ELEMENTAL:        return "elemental";
    case SCHOOL_CHROMATIC:        return "chromatic";
    case SCHOOL_MAGIC:            return "magic";
    case SCHOOL_DRAIN:            return "drain";
    case SCHOOL_NONE:             return "none";
    default:                      return "unknown";
  }
}

// parse_school_type ========================================================

school_e util::parse_school_type( const std::string& name )
{
  return parse_enum<school_e, SCHOOL_NONE, SCHOOL_MAX, school_type_string>( name );
}

resource_e util::translate_power_type( power_e pt )
{
  switch ( pt )
  {
    case POWER_HEALTH:        return RESOURCE_HEALTH;
    case POWER_MANA:          return RESOURCE_MANA;
    case POWER_RAGE:          return RESOURCE_RAGE;
    case POWER_FOCUS:         return RESOURCE_FOCUS;
    case POWER_ENERGY:        return RESOURCE_ENERGY;
    case POWER_COMBO_POINT:   return RESOURCE_COMBO_POINT;
    case POWER_RUNE:          return RESOURCE_RUNE;
    case POWER_RUNIC_POWER:   return RESOURCE_RUNIC_POWER;
    case POWER_SOUL_SHARDS:   return RESOURCE_SOUL_SHARD;
    case POWER_ASTRAL_POWER:  return RESOURCE_ASTRAL_POWER;
    case POWER_HOLY_POWER:    return RESOURCE_HOLY_POWER;
    case POWER_CHI:           return RESOURCE_CHI;
    case POWER_INSANITY:      return RESOURCE_INSANITY;
    case POWER_MAELSTROM:     return RESOURCE_MAELSTROM;
    case POWER_FURY:          return RESOURCE_FURY;
    case POWER_PAIN:          return RESOURCE_PAIN;
    default:                  return RESOURCE_NONE;
  }
}

// weapon_type_string =======================================================

const char* util::weapon_type_string( weapon_e weapon )
{
  switch ( weapon )
  {
    case WEAPON_NONE:      return "none";
    case WEAPON_DAGGER:    return "dagger";
    case WEAPON_FIST:      return "fist";
    case WEAPON_BEAST:     return "beast";
    case WEAPON_SWORD:     return "sword";
    case WEAPON_MACE:      return "mace";
    case WEAPON_AXE:       return "axe";
    case WEAPON_BEAST_2H:  return "beast2h";
    case WEAPON_SWORD_2H:  return "sword2h";
    case WEAPON_MACE_2H:   return "mace2h";
    case WEAPON_AXE_2H:    return "axe2h";
    case WEAPON_STAFF:     return "staff";
    case WEAPON_POLEARM:   return "polearm";
    case WEAPON_BOW:       return "bow";
    case WEAPON_CROSSBOW:  return "crossbow";
    case WEAPON_GUN:       return "gun";
    case WEAPON_THROWN:    return "thrown";
    case WEAPON_WAND:      return "wand";
    case WEAPON_WARGLAIVE: return "warglaive";
    default:               return "unknown";
  }
}

// weapon_subclass_string ===================================================

const char* util::weapon_subclass_string( int subclass )
{
  switch ( subclass )
  {
    case ITEM_SUBCLASS_WEAPON_AXE:
    case ITEM_SUBCLASS_WEAPON_AXE2:     return "Axe";
    case ITEM_SUBCLASS_WEAPON_BOW:      return "Bow";
    case ITEM_SUBCLASS_WEAPON_GUN:      return "Gun";
    case ITEM_SUBCLASS_WEAPON_MACE:
    case ITEM_SUBCLASS_WEAPON_MACE2:    return "Mace";
    case ITEM_SUBCLASS_WEAPON_POLEARM:  return "Polearm";
    case ITEM_SUBCLASS_WEAPON_SWORD:
    case ITEM_SUBCLASS_WEAPON_SWORD2:   return "Sword";
    case ITEM_SUBCLASS_WEAPON_STAFF:    return "Staff";
    case ITEM_SUBCLASS_WEAPON_FIST:     return "Fist Weapon";
    case ITEM_SUBCLASS_WEAPON_DAGGER:   return "Dagger";
    case ITEM_SUBCLASS_WEAPON_THROWN:   return "Thrown";
    case ITEM_SUBCLASS_WEAPON_CROSSBOW: return "Crossbow";
    case ITEM_SUBCLASS_WEAPON_WAND:     return "Wand";
    default:                            return "Unknown";
  }
}

// weapon_class_string ======================================================

const char* util::weapon_class_string( int it )
{
  switch ( it )
  {
    case INVTYPE_WEAPON:
      return "One Hand";
    case INVTYPE_2HWEAPON:
      return "Two-Hand";
    case INVTYPE_WEAPONMAINHAND:
      return "Main Hand";
    case INVTYPE_WEAPONOFFHAND:
      return "Off Hand";
    case INVTYPE_RANGEDRIGHT:
      return "Ranged";
    default:
      return nullptr;
  }
}

// parse_weapon_type ========================================================

weapon_e util::parse_weapon_type( const std::string& name )
{
  return parse_enum<weapon_e, WEAPON_NONE, WEAPON_MAX, weapon_type_string>( name );
}

// slot_type_string =========================================================

const char* util::slot_type_string( slot_e slot )
{
  switch ( slot )
  {
    case SLOT_HEAD:      return "head";
    case SLOT_NECK:      return "neck";
    case SLOT_SHOULDERS: return "shoulders";
    case SLOT_SHIRT:     return "shirt";
    case SLOT_CHEST:     return "chest";
    case SLOT_WAIST:     return "waist";
    case SLOT_LEGS:      return "legs";
    case SLOT_FEET:      return "feet";
    case SLOT_WRISTS:    return "wrists";
    case SLOT_HANDS:     return "hands";
    case SLOT_FINGER_1:  return "finger1";
    case SLOT_FINGER_2:  return "finger2";
    case SLOT_TRINKET_1: return "trinket1";
    case SLOT_TRINKET_2: return "trinket2";
    case SLOT_BACK:      return "back";
    case SLOT_MAIN_HAND: return "main_hand";
    case SLOT_OFF_HAND:  return "off_hand";
    case SLOT_TABARD:    return "tabard";
    default:             return "unknown";
  }
}

/// Slots with matching type of armour (cloth/leather/mail/plate)
bool util::is_match_slot( slot_e s )
{
  switch ( s )
  {
    case SLOT_HEAD:
    case SLOT_SHOULDERS:
    case SLOT_CHEST:
    case SLOT_WAIST:
    case SLOT_LEGS:
    case SLOT_FEET:
    case SLOT_WRISTS:
    case SLOT_HANDS:
      return true;
    default:
      return false;
  }
}
// matching_armor_type ======================================================

item_subclass_armor util::matching_armor_type( player_e ptype )
{
  switch ( ptype )
  {
    case WARRIOR:
    case PALADIN:
    case DEATH_KNIGHT:
      return ITEM_SUBCLASS_ARMOR_PLATE;
    case HUNTER:
    case SHAMAN:
      return ITEM_SUBCLASS_ARMOR_MAIL;
    case DRUID:
    case ROGUE:
    case MONK:
    case DEMON_HUNTER:
      return ITEM_SUBCLASS_ARMOR_LEATHER;
    case MAGE:
    case PRIEST:
    case WARLOCK:
      return ITEM_SUBCLASS_ARMOR_CLOTH;
    default:
      return ITEM_SUBCLASS_ARMOR_MISC;
  }
}

const char* util::armor_type_string( int type )
{ return armor_type_string( static_cast<item_subclass_armor>( type ) ); }

const char* util::armor_type_string( item_subclass_armor type )
{
  switch ( type )
  {
    case ITEM_SUBCLASS_ARMOR_CLOTH: return "cloth";
    case ITEM_SUBCLASS_ARMOR_LEATHER: return "leather";
    case ITEM_SUBCLASS_ARMOR_MAIL: return "mail";
    case ITEM_SUBCLASS_ARMOR_PLATE: return "plate";
    case ITEM_SUBCLASS_ARMOR_SHIELD: return "shield";
    case ITEM_SUBCLASS_ARMOR_MISC: return "misc";
    default: return "";
  }
}

item_subclass_armor util::parse_armor_type( const std::string& name )
{
  return parse_enum<item_subclass_armor, ITEM_SUBCLASS_ARMOR_MISC, ITEM_SUBCLASS_ARMOR_PLATE, armor_type_string>( name );
}

// parse_slot_type ==========================================================

slot_e util::parse_slot_type( const std::string& name )
{
  return parse_enum<slot_e, SLOT_INVALID, SLOT_MAX, slot_type_string>( name );
}

// movement_direction_string ================================================

const char* util::movement_direction_string( movement_direction_e m )
{
  switch ( m )
  {
    case MOVEMENT_OMNI: return "omni";
    case MOVEMENT_TOWARDS: return "towards";
    case MOVEMENT_AWAY: return "away";
    case MOVEMENT_RANDOM: return "random";
    case MOVEMENT_NONE: return "none";
    default: return "";
  }
}

// parse_movement_type ======================================================

movement_direction_e util::parse_movement_direction( const std::string& name )
{
  return parse_enum<movement_direction_e, MOVEMENT_UNKNOWN, MOVEMENT_DIRECTION_MAX, movement_direction_string>( name );
}

// cache_type_string ========================================================

const char* util::cache_type_string( cache_e c )
{
  switch ( c )
  {
    case CACHE_STRENGTH:  return "strength";
    case CACHE_AGILITY:   return "agility";
    case CACHE_STAMINA:   return "stamina";
    case CACHE_INTELLECT: return "intellect";
    case CACHE_SPIRIT:    return "spirit";

    case CACHE_SPELL_POWER:  return "spell_power";
    case CACHE_ATTACK_POWER: return "attack_power";

    case CACHE_EXP:          return "expertise";
    case CACHE_ATTACK_EXP:   return "attack_expertise";
    case CACHE_HIT:          return "hit";
    case CACHE_ATTACK_HIT:   return "attack_hit";
    case CACHE_SPELL_HIT:    return "spell_hit";
    case CACHE_CRIT_CHANCE:  return "crit_chance";
    case CACHE_ATTACK_CRIT_CHANCE: return "attack_crit_chance";
    case CACHE_SPELL_CRIT_CHANCE:  return "spell_crit_chance";
    case CACHE_HASTE:        return "haste";
    case CACHE_ATTACK_HASTE: return "attack_haste";
    case CACHE_SPELL_HASTE:  return "spell_haste";
    case CACHE_SPEED:        return "speed";
    case CACHE_ATTACK_SPEED: return "attack_speed";
    case CACHE_SPELL_SPEED:  return "spell_speed";
    case CACHE_MASTERY:      return "mastery";
    case CACHE_PLAYER_DAMAGE_MULTIPLIER: return "player_dmg_mult";
    case CACHE_PLAYER_HEAL_MULTIPLIER: return "player_heal_mult";
    case CACHE_PARRY:        return "parry";
    case CACHE_DODGE:        return "dodge";
    case CACHE_BLOCK:        return "block";
    case CACHE_ARMOR:        return "armor";
    case CACHE_BONUS_ARMOR:  return "bonus_armor";
    case CACHE_VERSATILITY:  return "versatility";
    case CACHE_DAMAGE_VERSATILITY:  return "damage_versatility";
    case CACHE_HEAL_VERSATILITY:  return "heal_versatility";
    case CACHE_MITIGATION_VERSATILITY:  return "mitigation_versatility";
    case CACHE_LEECH: return "leech";
    case CACHE_RUN_SPEED: return "run_speed";

    default: return "unknown";
  }
}

// proc_type_tring ==========================================================

const char* util::proc_type_string( proc_types type )
{
  switch ( type )
  { 
    case PROC1_KILLED:               return "Killed";
    case PROC1_KILLING_BLOW:         return "KillingBlow";
    case PROC1_MELEE:                return "MeleeSwing";
    case PROC1_MELEE_TAKEN:          return "MeleeSwingTaken";
    case PROC1_MELEE_ABILITY:        return "MeleeAbility";
    case PROC1_MELEE_ABILITY_TAKEN:  return "MeleeAbilityTaken";
    case PROC1_RANGED:               return "RangedShot";
    case PROC1_RANGED_TAKEN:         return "RangedShotTaken";
    case PROC1_RANGED_ABILITY:       return "RangedAbility";
    case PROC1_RANGED_ABILITY_TAKEN: return "RangedAbilityTaken";
    case PROC1_AOE_HEAL:             return "AoeHeal";
    case PROC1_AOE_HEAL_TAKEN:       return "AoeHealTaken";
    case PROC1_AOE_SPELL:            return "AoeHarmfulSpell";
    case PROC1_AOE_SPELL_TAKEN:      return "AoeHarmfulSpellTaken";
    case PROC1_HEAL:                 return "Heal";
    case PROC1_HEAL_TAKEN:           return "HealTaken";
    case PROC1_SPELL:                return "HarmfulSpell";
    case PROC1_SPELL_TAKEN:          return "HarmfulSpellTaken";
    case PROC1_PERIODIC:             return "HarmfulTick";
    case PROC1_PERIODIC_TAKEN:       return "HarmfulTickTaken";
    case PROC1_ANY_DAMAGE_TAKEN:     return "AnyDamageTaken";
    case PROC1_PERIODIC_HEAL:        return "TickHeal";
    case PROC1_PERIODIC_HEAL_TAKEN:  return "TickHealTaken";
    default:                         return "Unknown";
  }
}

// proc_type2_string ========================================================

const char* util::proc_type2_string( proc_types2 type )
{
  switch ( type )
  {
    case PROC2_HIT:    return "HitAmount";
    case PROC2_CRIT:   return "CritAmount";
    case PROC2_GLANCE: return "GlanceAmount";
    case PROC2_DODGE:  return "Dodge";
    case PROC2_PARRY:  return "Parry";
    case PROC2_MISS:   return "Miss";
    case PROC2_LANDED: return "Impact";
    case PROC2_CAST:   return "Cast";
    case PROC2_CAST_DAMAGE: return "DamageCast";
    case PROC2_CAST_HEAL: return "HealCast";
    default:           return "Unknown";
  }
}

// special_effect_string ====================================================

const char* util::special_effect_string( special_effect_e type )
{
  switch ( type )
  {
    case SPECIAL_EFFECT_EQUIP: return "equip";
    case SPECIAL_EFFECT_USE:   return "use";
    case SPECIAL_EFFECT_FALLBACK: return "fallback";
    default:                   return "unknown";
  }
}

// special_effect_source_string =============================================

const char* util::special_effect_source_string( special_effect_source_e type )
{
  switch ( type )
  {
    case SPECIAL_EFFECT_SOURCE_ITEM:    return "item";
    case SPECIAL_EFFECT_SOURCE_ENCHANT: return "enchant";
    case SPECIAL_EFFECT_SOURCE_ADDON:   return "addon";
    case SPECIAL_EFFECT_SOURCE_GEM:     return "gem";
    case SPECIAL_EFFECT_SOURCE_SOCKET_BONUS: return "socket_bonus";
    case SPECIAL_EFFECT_SOURCE_FALLBACK: return "fallback";
    default:                            return "unknown";
  }
}

// stat_type_string =========================================================

const char* util::stat_type_string( stat_e stat )
{
  switch ( stat )
  {
    case STAT_STRENGTH:  return "strength";
    case STAT_AGILITY:   return "agility";
    case STAT_STAMINA:   return "stamina";
    case STAT_INTELLECT: return "intellect";
    case STAT_SPIRIT:    return "spirit";

    case STAT_AGI_INT:   return "agiint";
    case STAT_STR_AGI:   return "stragi";
    case STAT_STR_INT:   return "strint";
    case STAT_STR_AGI_INT: return "stragiint";

    case STAT_HEALTH: return "health";
    case STAT_MAX_HEALTH: return "maximum_health";
    case STAT_MANA:   return "mana";
    case STAT_MAX_MANA: return "maximum_mana";
    case STAT_RAGE:   return "rage";
    case STAT_MAX_RAGE: return "maximum_rage";
    case STAT_ENERGY: return "energy";
    case STAT_MAX_ENERGY: return "maximum_energy";
    case STAT_FOCUS:  return "focus";
    case STAT_MAX_FOCUS: return "maximum_focus";
    case STAT_RUNIC:  return "runic";
    case STAT_MAX_RUNIC: return "maximum_runic";

    case STAT_SPELL_POWER:       return "spell_power";

    case STAT_ATTACK_POWER:             return "attack_power";
    case STAT_EXPERTISE_RATING:         return "expertise_rating";
    case STAT_EXPERTISE_RATING2:        return "inverse_expertise_rating";

    case STAT_HIT_RATING:   return "hit_rating";
    case STAT_HIT_RATING2:  return "inverse_hit_rating";
    case STAT_CRIT_RATING:  return "crit_rating";
    case STAT_HASTE_RATING: return "haste_rating";

    case STAT_WEAPON_DPS:   return "weapon_dps";

    case STAT_WEAPON_OFFHAND_DPS:    return "weapon_offhand_dps";

    case STAT_ARMOR:             return "armor";
    case STAT_BONUS_ARMOR:       return "bonus_armor";
    case STAT_RESILIENCE_RATING: return "resilience_rating";
    case STAT_DODGE_RATING:      return "dodge_rating";
    case STAT_PARRY_RATING:      return "parry_rating";

    case STAT_BLOCK_RATING: return "block_rating";

    case STAT_MASTERY_RATING: return "mastery_rating";

    case STAT_PVP_POWER: return "pvp_power";
    case STAT_VERSATILITY_RATING: return "versatility_rating";

    case STAT_LEECH_RATING: return "leech_rating";
    case STAT_SPEED_RATING: return "speed_rating";
    case STAT_AVOIDANCE_RATING: return "avoidance_rating";

    case STAT_ALL: return "all";

    default: return "unknown";
  }
}

// stat_type_abbrev =========================================================

const char* util::stat_type_abbrev( stat_e stat )
{
  switch ( stat )
  {
    case STAT_STRENGTH:  return "Str";
    case STAT_AGILITY:   return "Agi";
    case STAT_STAMINA:   return "Sta";
    case STAT_INTELLECT: return "Int";
    case STAT_SPIRIT:    return "Spi";

    case STAT_AGI_INT:   return "AgiInt";
    case STAT_STR_AGI:   return "StrAgi";
    case STAT_STR_INT:   return "StrInt";
    case STAT_STR_AGI_INT: return "StrAgiInt";

    case STAT_HEALTH: return "Health";
    case STAT_MAX_HEALTH: return "MaxHealth";
    case STAT_MANA:   return "Mana";
    case STAT_MAX_MANA: return "MaxMana";
    case STAT_RAGE:   return "Rage";
    case STAT_MAX_RAGE: return "MaxRage";
    case STAT_ENERGY: return "Energy";
    case STAT_MAX_ENERGY: return "MaxEnergy";
    case STAT_FOCUS:  return "Focus";
    case STAT_MAX_FOCUS: return "MaxFocus";
    case STAT_RUNIC:  return "Runic";
    case STAT_MAX_RUNIC: return "MaxRunic";

    case STAT_SPELL_POWER:       return "SP";

    case STAT_ATTACK_POWER:             return "AP";
    case STAT_EXPERTISE_RATING:         return "Exp";
    case STAT_EXPERTISE_RATING2:        return "InvExp";

    case STAT_HIT_RATING:   return "Hit";
    case STAT_HIT_RATING2:  return "InvHit";
    case STAT_CRIT_RATING:  return "Crit";
    case STAT_HASTE_RATING: return "Haste";

    case STAT_WEAPON_DPS:   return "Wdps";

    case STAT_WEAPON_OFFHAND_DPS:    return "WOHdps";

    case STAT_ARMOR:             return "Armor";
    case STAT_BONUS_ARMOR:       return "BonusArmor";
    case STAT_RESILIENCE_RATING: return "Resil";
    case STAT_DODGE_RATING:      return "Dodge";
    case STAT_PARRY_RATING:      return "Parry";

    case STAT_BLOCK_RATING: return "BlockR";

    case STAT_MASTERY_RATING: return "Mastery";

    case STAT_PVP_POWER: return "PvPP";

    case STAT_VERSATILITY_RATING: return "Vers";

    case STAT_LEECH_RATING: return "Leech";
    case STAT_SPEED_RATING: return "RunSpeed";
    case STAT_AVOIDANCE_RATING: return "Avoidance";

    case STAT_ALL: return "All";

    default: return "unknown";
  }
}

// stat_type_wowhead ========================================================

const char* util::stat_type_wowhead( stat_e stat )
{
  switch ( stat )
  {
    case STAT_STRENGTH:  return "str";
    case STAT_AGILITY:   return "agi";
    case STAT_STAMINA:   return "sta";
    case STAT_INTELLECT: return "int";
    case STAT_SPIRIT:    return "spr";

    case STAT_AGI_INT:   return "agiint";
    case STAT_STR_AGI:   return "agistr";
    case STAT_STR_INT:   return "strint";
    case STAT_STR_AGI_INT: return "agistrint";

    case STAT_HEALTH: return "health";
    case STAT_MANA:   return "mana";
    case STAT_RAGE:   return "rage";
    case STAT_ENERGY: return "energy";
    case STAT_FOCUS:  return "focus";
    case STAT_RUNIC:  return "runic";

    case STAT_SPELL_POWER:       return "spellPower";

    case STAT_ATTACK_POWER:             return "attackPower";
    case STAT_EXPERTISE_RATING:         return "expertiseRating";

    case STAT_HIT_RATING:   return "hitRating";
    case STAT_CRIT_RATING:  return "critRating";
    case STAT_HASTE_RATING: return "hasteRating";

    case STAT_WEAPON_DPS:   return "__dps";

    case STAT_ARMOR:             return "armor"; 
    case STAT_BONUS_ARMOR:       return "armorbonus";
    case STAT_RESILIENCE_RATING: return "resilRating";
    case STAT_DODGE_RATING:      return "dodgeRating";
    case STAT_PARRY_RATING:      return "parryRating";

    case STAT_MASTERY_RATING: return "masteryRating";

    case STAT_VERSATILITY_RATING: return "versatility";

    case STAT_LEECH_RATING: return "lifesteal";
    case STAT_SPEED_RATING: return "speedbonus";
    case STAT_AVOIDANCE_RATING: return "avoidance";

    case STAT_MAX: return "__all";
    default: return "unknown";
  }
}

// stat_type_blizzard =======================================================

const char* util::stat_type_gem( stat_e stat )
{
  switch ( stat )
  {
    case STAT_STRENGTH:  return "Strength";
    case STAT_AGILITY:   return "Agility";
    case STAT_STAMINA:   return "Stamina";
    case STAT_INTELLECT: return "Intellect";

    case STAT_HIT_RATING: return "Hit";
    case STAT_EXPERTISE_RATING: return "Expertise";
    case STAT_CRIT_RATING: return "Critical Strike";
    case STAT_HASTE_RATING: return "Haste";
    case STAT_MASTERY_RATING: return "Mastery";

    case STAT_DODGE_RATING: return "Dodge";
    case STAT_PARRY_RATING: return "Parry";

    case STAT_PVP_POWER: return "PvP Power";

    default: return "unknown";
  }
}

// parse_stat_type ==========================================================

stat_e util::parse_stat_type( const std::string& name )
{
  stat_e s = parse_enum<stat_e, STAT_NONE, STAT_MAX, stat_type_string>( name );
  if ( s != STAT_NONE ) return s;

  s = parse_enum<stat_e, STAT_NONE, STAT_MAX, stat_type_abbrev>( name );
  if ( s != STAT_NONE ) return s;

  s = parse_enum<stat_e, STAT_NONE, STAT_MAX, stat_type_wowhead>( name );
  if ( s != STAT_NONE ) return s;

  if ( name == "rgdcritstrkrtng" ) return STAT_CRIT_RATING;

  // in-case wowhead changes their mind again
  if ( name == "atkpwr"         ) return STAT_ATTACK_POWER;
  if ( name == "critstrkrtng"   ) return STAT_CRIT_RATING;
  if ( name == "dodgertng"      ) return STAT_DODGE_RATING;
  if ( name == "exprtng"        ) return STAT_EXPERTISE_RATING;
  if ( name == "hastertng"      ) return STAT_HASTE_RATING;
  if ( name == "hitrtng"        ) return STAT_HIT_RATING;
  if ( name == "mastrtng"       ) return STAT_MASTERY_RATING;
  if ( name == "parryrtng"      ) return STAT_PARRY_RATING;
  if ( name == "resiliencertng" ) return STAT_RESILIENCE_RATING;
  if ( name == "splpwr"         ) return STAT_SPELL_POWER;
  if ( name == "spi"            ) return STAT_SPIRIT;
  if ( str_compare_ci( name, "__wpds"   ) ) return STAT_WEAPON_DPS;


  return STAT_NONE;
}

// scale_metric_type_string ===================================================

const char* util::scale_metric_type_string( scale_metric_e sm )
{
  switch ( sm )
  {
    case SCALE_METRIC_DPS:       return "Damage per Second";
    case SCALE_METRIC_DPSE:      return "Damage per Second (effective)";
    case SCALE_METRIC_DPSP:      return "Damage per Second to Priority Target/Boss";
    case SCALE_METRIC_HPS:       return "Healing per Second";
    case SCALE_METRIC_HPSE:      return "Healing per Second (effective)";
    case SCALE_METRIC_APS:       return "Absorb per Second";
    case SCALE_METRIC_HAPS:      return "Healing/Absorb per Second";
    case SCALE_METRIC_DTPS:      return "Damage Taken per Second";
    case SCALE_METRIC_DMG_TAKEN: return "Damage Taken";
    case SCALE_METRIC_HTPS:      return "Healing Taken per Second";
    case SCALE_METRIC_TMI:       return "Theck-Meloree-Index";
    case SCALE_METRIC_ETMI:      return "Effective Theck-Meloree-Index";
    case SCALE_METRIC_DEATHS:    return "Deaths";
    default:                     return "Damage per Second"; //When set to "Default", assume dps until we have a better solution.
  }
}

// scale_metric_type_abbrev ===================================================

const char* util::scale_metric_type_abbrev( scale_metric_e sm )
{
  switch ( sm )
  {
    case SCALE_METRIC_DPS:       return "dps";
    case SCALE_METRIC_DPSP:      return "prioritydps";
    case SCALE_METRIC_DPSE:      return "dpse";
    case SCALE_METRIC_HPS:       return "hps";
    case SCALE_METRIC_HPSE:      return "hpse";
    case SCALE_METRIC_APS:       return "aps";
    case SCALE_METRIC_HAPS:      return "haps";
    case SCALE_METRIC_DTPS:      return "dtps";
    case SCALE_METRIC_DMG_TAKEN: return "dmg_taken";
    case SCALE_METRIC_HTPS:      return "htps";
    case SCALE_METRIC_TMI:       return "tmi";
    case SCALE_METRIC_ETMI:      return "etmi";
    case SCALE_METRIC_DEATHS:    return "deaths";
    default:                     return "dps"; //When set to "Default", assume dps until we have a better solution.
  }
}

// parse_scale_metric =========================================================

scale_metric_e util::parse_scale_metric( const std::string& name )
{
  scale_metric_e sm = parse_enum<scale_metric_e, SCALE_METRIC_NONE, SCALE_METRIC_MAX, scale_metric_type_abbrev>( name );
  if ( sm != SCALE_METRIC_NONE ) return sm;

  if ( name == "theck_meloree_index" ) return SCALE_METRIC_TMI;
  if ( name == "effective_theck_meloree_index" ) return SCALE_METRIC_ETMI;

  return SCALE_METRIC_NONE;
}

// parse_origin =============================================================

bool util::parse_origin( std::string& region_str,
                         std::string& server_str,
                         std::string& name_str,
                         const std::string& origin_str )
{
  std::vector<std::string> tokens = string_split( origin_str, "/:.?&=" );

  if ( origin_str.find( ".battle.net" ) != std::string::npos )
  {
    if ( tokens.size() < 2 || tokens[ 0 ] != "http" || tokens[ 1 ].empty() )
      return false;
    region_str = tokens[ 1 ];

    std::vector<std::string>::const_iterator pos = range::find( tokens, "character" );
    if ( pos == tokens.end() || ++pos == tokens.end() || pos -> empty() )
      return false;
    server_str = *pos;

    if ( ++pos == tokens.end() || pos -> empty() )
      return false;
    name_str = *pos;

    return true;
  }

  if ( origin_str.find( ".battlenet.com." ) != std::string::npos )
  {
    std::vector<std::string>::const_iterator pos = range::find( tokens, "battlenet" );
    if ( pos == tokens.end() || ++pos == tokens.end() || *pos != "com" )
      return false;
    if ( ++pos == tokens.end() || pos -> empty() )
      return false;
    region_str = *pos;

    pos = range::find( tokens, "character" );
    if ( pos == tokens.end() || ++pos == tokens.end() || pos -> empty() )
      return false;
    server_str = *pos;

    if ( ++pos == tokens.end() || pos -> empty() )
      return false;
    name_str = *pos;

    return true;
  }

  return false;
}

// class_id_mask ============================================================

int util::class_id_mask( player_e type )
{
  int cid = class_id( type );
  if ( cid <= 0 ) return 0;
  return 1 << ( cid - 1 );
}

// class_id =================================================================

int util::class_id( player_e type )
{
  switch ( type )
  {
    case WARRIOR:      return  1;
    case PALADIN:      return  2;
    case HUNTER:       return  3;
    case ROGUE:        return  4;
    case PRIEST:       return  5;
    case DEATH_KNIGHT: return  6;
    case SHAMAN:       return  7;
    case MAGE:         return  8;
    case WARLOCK:      return  9;
    case MONK:         return 10;
    case DRUID:        return 11;
    case DEMON_HUNTER: return 12;
    case PLAYER_SPECIAL_SCALE: return 13;
    case PLAYER_SPECIAL_SCALE2: return 14;
    case PLAYER_SPECIAL_SCALE3: return 15;
    case PLAYER_SPECIAL_SCALE4: return 16;
    case PLAYER_SPECIAL_SCALE5: return 17;
    case PLAYER_SPECIAL_SCALE6: return 18;
    default:           return 0;
  }
}

// race_id ==================================================================

unsigned util::race_id( race_e r )
{
  switch ( r )
  {
    case RACE_NIGHT_ELF: return 4;
    case RACE_HUMAN: return 1;
    case RACE_GNOME: return 7;
    case RACE_DWARF: return 3;
    case RACE_DRAENEI: return 11;
    case RACE_WORGEN: return 22;
    case RACE_ORC: return 2;
    case RACE_TROLL: return 8;
    case RACE_UNDEAD: return 5;
    case RACE_BLOOD_ELF: return 10;
    case RACE_TAUREN: return 6;
    case RACE_GOBLIN: return 9;
    case RACE_PANDAREN: return 24;
    case RACE_PANDAREN_ALLIANCE: return 25;
    case RACE_PANDAREN_HORDE: return 26;
    default: return 0;
  }
}

// race_mask ================================================================

unsigned util::race_mask( race_e r )
{
  uint32_t id = race_id( r );

  if ( id > 0 )
    return ( 1 << ( id - 1 ) );

  return 0x00;
}

// pet_class_type ===========================================================

player_e util::pet_class_type( pet_e c )
{
  player_e p = WARRIOR;

  if ( c <= PET_HUNTER )
  {
    p = WARRIOR;
  }
  else if ( c == PET_GHOUL )
  {
    p = ROGUE;
  }
  else if ( c == PET_FELGUARD )
  {
    p = WARRIOR;
  }
  else if ( c <= PET_WARLOCK )
  {
    p = WARLOCK;
  }

  return p;
}

// pet_mask =================================================================

unsigned util::pet_mask( pet_e p )
{
  if ( p <= PET_FEROCITY_TYPE )
    return 0x1;
  if ( p <= PET_TENACITY_TYPE )
    return 0x2;
  if ( p <= PET_CUNNING_TYPE )
    return 0x4;

  return 0x0;
}

// pet_id ===================================================================

unsigned util::pet_id( pet_e p )
{
  uint32_t mask = pet_mask( p );

  switch ( mask )
  {
    case 0x1: return 1;
    case 0x2: return 2;
    case 0x4: return 3;
  }

  return 0;
}

// translate_class_id =======================================================

player_e util::translate_class_id( int cid )
{
  switch ( cid )
  {
    case  1: return WARRIOR;
    case  2: return PALADIN;
    case  3: return HUNTER;
    case  4: return ROGUE;
    case  5: return PRIEST;
    case  6: return DEATH_KNIGHT;
    case  7: return SHAMAN;
    case  8: return MAGE;
    case  9: return WARLOCK;
    case 10: return MONK;
    case 11: return DRUID;
    case 12: return DEMON_HUNTER;
    default: return PLAYER_NONE;
  }
}

// translate_race_id ========================================================

race_e util::translate_race_id( int rid )
{
  switch ( rid )
  {
    case  1: return RACE_HUMAN;
    case  2: return RACE_ORC;
    case  3: return RACE_DWARF;
    case  4: return RACE_NIGHT_ELF;
    case  5: return RACE_UNDEAD;
    case  6: return RACE_TAUREN;
    case  7: return RACE_GNOME;
    case  8: return RACE_TROLL;
    case  9: return RACE_GOBLIN;
    case 10: return RACE_BLOOD_ELF;
    case 11: return RACE_DRAENEI;
    case 22: return RACE_WORGEN;
    case 24: return RACE_PANDAREN;
    case 25: return RACE_PANDAREN_ALLIANCE;
    case 26: return RACE_PANDAREN_HORDE;
  }

  return RACE_NONE;
}

// translate_item_mod =======================================================

stat_e util::translate_item_mod( int item_mod )
{
  switch ( item_mod )
  {
    case ITEM_MOD_AGILITY:             return STAT_AGILITY;
    case ITEM_MOD_STRENGTH:            return STAT_STRENGTH;
    case ITEM_MOD_INTELLECT:           return STAT_INTELLECT;
    case ITEM_MOD_SPIRIT:              return STAT_SPIRIT;
    case ITEM_MOD_STAMINA:             return STAT_STAMINA;
    case ITEM_MOD_DODGE_RATING:        return STAT_DODGE_RATING;
    case ITEM_MOD_PARRY_RATING:        return STAT_PARRY_RATING;
    case ITEM_MOD_BLOCK_RATING:        return STAT_BLOCK_RATING;
    case ITEM_MOD_HIT_RATING:          return STAT_HIT_RATING;
    case ITEM_MOD_CRIT_RATING:         return STAT_CRIT_RATING;
    case ITEM_MOD_HASTE_RATING:        return STAT_HASTE_RATING;
    case ITEM_MOD_EXPERTISE_RATING:    return STAT_EXPERTISE_RATING;
    case ITEM_MOD_ATTACK_POWER:        return STAT_ATTACK_POWER;
    case ITEM_MOD_RANGED_ATTACK_POWER: return STAT_ATTACK_POWER;
    case ITEM_MOD_SPELL_POWER:         return STAT_SPELL_POWER;
    case ITEM_MOD_MASTERY_RATING:      return STAT_MASTERY_RATING;
    case ITEM_MOD_EXTRA_ARMOR:         return STAT_BONUS_ARMOR;
    case ITEM_MOD_RESILIENCE_RATING:   return STAT_RESILIENCE_RATING;
    case ITEM_MOD_PVP_POWER:           return STAT_PVP_POWER;
    case ITEM_MOD_STRENGTH_AGILITY_INTELLECT: return STAT_STR_AGI_INT;
    case ITEM_MOD_AGILITY_INTELLECT:   return STAT_AGI_INT;
    case ITEM_MOD_STRENGTH_AGILITY:    return STAT_STR_AGI;
    case ITEM_MOD_STRENGTH_INTELLECT:  return STAT_STR_INT;
    case ITEM_MOD_VERSATILITY_RATING:  return STAT_VERSATILITY_RATING;
    case ITEM_MOD_LEECH_RATING:        return STAT_LEECH_RATING;
    case ITEM_MOD_SPEED_RATING:        return STAT_SPEED_RATING;
    case ITEM_MOD_AVOIDANCE_RATING:    return STAT_AVOIDANCE_RATING;
    default:                           return STAT_NONE;
  }
}

int util::translate_stat( stat_e stat )
{
  switch ( stat )
  {
    case STAT_AGILITY:            return ITEM_MOD_AGILITY;
    case STAT_STRENGTH:           return ITEM_MOD_STRENGTH;
    case STAT_INTELLECT:          return ITEM_MOD_INTELLECT;
    case STAT_SPIRIT:             return ITEM_MOD_SPIRIT;
    case STAT_STAMINA:            return ITEM_MOD_STAMINA;
    case STAT_DODGE_RATING:       return ITEM_MOD_DODGE_RATING;
    case STAT_PARRY_RATING:       return ITEM_MOD_PARRY_RATING;
    case STAT_HIT_RATING:         return ITEM_MOD_HIT_RATING;
    case STAT_CRIT_RATING:        return ITEM_MOD_CRIT_RATING;
    case STAT_HASTE_RATING:       return ITEM_MOD_HASTE_RATING;
    case STAT_EXPERTISE_RATING:   return ITEM_MOD_EXPERTISE_RATING;
    case STAT_ATTACK_POWER:       return ITEM_MOD_ATTACK_POWER;
    case STAT_SPELL_POWER:        return ITEM_MOD_SPELL_POWER;
    case STAT_MASTERY_RATING:     return ITEM_MOD_MASTERY_RATING;
    case STAT_BONUS_ARMOR:        return ITEM_MOD_EXTRA_ARMOR;
    case STAT_RESILIENCE_RATING:  return ITEM_MOD_RESILIENCE_RATING;
    case STAT_PVP_POWER:          return ITEM_MOD_PVP_POWER;
    case STAT_STR_AGI_INT:        return ITEM_MOD_STRENGTH_AGILITY_INTELLECT;
    case STAT_AGI_INT:            return ITEM_MOD_AGILITY_INTELLECT;
    case STAT_STR_AGI:            return ITEM_MOD_STRENGTH_AGILITY;
    case STAT_STR_INT:            return ITEM_MOD_STRENGTH_INTELLECT;
    case STAT_VERSATILITY_RATING: return ITEM_MOD_VERSATILITY_RATING;
    case STAT_LEECH_RATING:       return ITEM_MOD_LEECH_RATING;
    case STAT_SPEED_RATING:       return ITEM_MOD_SPEED_RATING;
    case STAT_AVOIDANCE_RATING:   return ITEM_MOD_AVOIDANCE_RATING;
    default:                      return ITEM_MOD_NONE;
  }
}

// translate_rating_mod =====================================================

std::vector<stat_e> util::translate_all_rating_mod( unsigned ratings )
{
  std::vector<stat_e> stats;

  for ( unsigned i = 0; i < sizeof( unsigned ) * 8; i++ )
  {
    if ( ! ( ratings & ( 1 << i ) ) )
      continue;

    stat_e stat = translate_rating_mod( ratings & ( 1 << i ) );
    if ( stat != STAT_NONE && std::find( stats.begin(), stats.end(), stat ) == stats.end() )
      stats.push_back( stat );
  }

  return stats;
}

stat_e util::translate_rating_mod( unsigned ratings )
{
  if ( ratings & RATING_MOD_DODGE )
    return STAT_DODGE_RATING;
  else if ( ratings & RATING_MOD_PARRY )
    return STAT_PARRY_RATING;
  else if ( ratings & ( RATING_MOD_HIT_MELEE | RATING_MOD_HIT_RANGED | RATING_MOD_HIT_SPELL ) )
    return STAT_HIT_RATING;
  else if ( ratings & ( RATING_MOD_CRIT_MELEE | RATING_MOD_CRIT_RANGED | RATING_MOD_CRIT_SPELL ) )
    return STAT_CRIT_RATING;
  else if ( ratings & RATING_MOD_RESILIENCE )
    return STAT_RESILIENCE_RATING;
  else if ( ratings & ( RATING_MOD_HASTE_MELEE | RATING_MOD_HASTE_RANGED | RATING_MOD_HASTE_SPELL ) )
    return STAT_HASTE_RATING;
  else if ( ratings & ( RATING_MOD_VERS_DAMAGE | RATING_MOD_VERS_HEAL | RATING_MOD_VERS_MITIG ) )
    return STAT_VERSATILITY_RATING;
  else if ( ratings & RATING_MOD_EXPERTISE )
    return STAT_EXPERTISE_RATING;
  else if ( ratings & RATING_MOD_MASTERY )
    return STAT_MASTERY_RATING;
  else if ( ratings & RATING_MOD_PVP_POWER )
    return STAT_PVP_POWER;
  else if ( ratings & RATING_MOD_LEECH )
    return STAT_LEECH_RATING;
  else if ( ratings & RATING_MOD_SPEED )
    return STAT_SPEED_RATING;

  return STAT_NONE;
}

// translate_weapon_subclass ================================================

weapon_e util::translate_weapon_subclass( int id )
{
  switch ( id )
  {
    case ITEM_SUBCLASS_WEAPON_AXE:          return WEAPON_AXE;
    case ITEM_SUBCLASS_WEAPON_AXE2:         return WEAPON_AXE_2H;
    case ITEM_SUBCLASS_WEAPON_BOW:          return WEAPON_BOW;
    case ITEM_SUBCLASS_WEAPON_GUN:          return WEAPON_GUN;
    case ITEM_SUBCLASS_WEAPON_MACE:         return WEAPON_MACE;
    case ITEM_SUBCLASS_WEAPON_MACE2:        return WEAPON_MACE_2H;
    case ITEM_SUBCLASS_WEAPON_POLEARM:      return WEAPON_POLEARM;
    case ITEM_SUBCLASS_WEAPON_SWORD:        return WEAPON_SWORD;
    case ITEM_SUBCLASS_WEAPON_SWORD2:       return WEAPON_SWORD_2H;
    case ITEM_SUBCLASS_WEAPON_STAFF:        return WEAPON_STAFF;
    case ITEM_SUBCLASS_WEAPON_FIST:         return WEAPON_FIST;
    case ITEM_SUBCLASS_WEAPON_DAGGER:       return WEAPON_DAGGER;
    case ITEM_SUBCLASS_WEAPON_THROWN:       return WEAPON_THROWN;
    case ITEM_SUBCLASS_WEAPON_CROSSBOW:     return WEAPON_CROSSBOW;
    case ITEM_SUBCLASS_WEAPON_WAND:         return WEAPON_WAND;
    case ITEM_SUBCLASS_WEAPON_WARGLAIVE:    return WEAPON_WARGLAIVE;
    default: return WEAPON_NONE;
  }

  return WEAPON_NONE;
}

// translate_weapon =========================================================

item_subclass_weapon util::translate_weapon( weapon_e weapon )
{
  switch ( weapon )
  {
    case WEAPON_AXE:       return ITEM_SUBCLASS_WEAPON_AXE;
    case WEAPON_AXE_2H:    return ITEM_SUBCLASS_WEAPON_AXE2;
    case WEAPON_BOW:       return ITEM_SUBCLASS_WEAPON_BOW;
    case WEAPON_CROSSBOW:  return ITEM_SUBCLASS_WEAPON_CROSSBOW;
    case WEAPON_GUN:       return ITEM_SUBCLASS_WEAPON_GUN;
    case WEAPON_MACE:      return ITEM_SUBCLASS_WEAPON_MACE;
    case WEAPON_MACE_2H:   return ITEM_SUBCLASS_WEAPON_MACE2;
    case WEAPON_POLEARM:   return ITEM_SUBCLASS_WEAPON_POLEARM;
    case WEAPON_SWORD:     return ITEM_SUBCLASS_WEAPON_SWORD;
    case WEAPON_SWORD_2H:  return ITEM_SUBCLASS_WEAPON_SWORD2;
    case WEAPON_STAFF:     return ITEM_SUBCLASS_WEAPON_STAFF;
    case WEAPON_FIST:      return ITEM_SUBCLASS_WEAPON_FIST;
    case WEAPON_DAGGER:    return ITEM_SUBCLASS_WEAPON_DAGGER;
    case WEAPON_THROWN:    return ITEM_SUBCLASS_WEAPON_THROWN;
    case WEAPON_WAND:      return ITEM_SUBCLASS_WEAPON_WAND;
    case WEAPON_WARGLAIVE: return ITEM_SUBCLASS_WEAPON_WARGLAIVE;
    default:
      assert( false );
      return ITEM_SUBCLASS_WEAPON_MISC;
  }
}

// translate_invtype ========================================================

slot_e util::translate_invtype( inventory_type inv_type )
{
  switch ( inv_type )
  {
    case INVTYPE_2HWEAPON:
    case INVTYPE_WEAPON:
    case INVTYPE_WEAPONMAINHAND:
    case INVTYPE_RANGED:
    case INVTYPE_RANGEDRIGHT:
      return SLOT_MAIN_HAND;
    case INVTYPE_WEAPONOFFHAND:
    case INVTYPE_SHIELD:
    case INVTYPE_HOLDABLE:
      return SLOT_OFF_HAND;
    case INVTYPE_THROWN:
      return SLOT_MAIN_HAND;
    case INVTYPE_CHEST:
    case INVTYPE_ROBE:
      return SLOT_CHEST;
    case INVTYPE_CLOAK:
      return SLOT_BACK;
    case INVTYPE_FEET:
      return SLOT_FEET;
    case INVTYPE_FINGER:
      return SLOT_FINGER_1;
    case INVTYPE_HANDS:
      return SLOT_HANDS;
    case INVTYPE_HEAD:
      return SLOT_HEAD;
    case INVTYPE_LEGS:
      return SLOT_LEGS;
    case INVTYPE_NECK:
      return SLOT_NECK;
    case INVTYPE_SHOULDERS:
      return SLOT_SHOULDERS;
    case INVTYPE_TABARD:
      return SLOT_TABARD;
    case INVTYPE_TRINKET:
      return SLOT_TRINKET_1;
    case INVTYPE_WAIST:
      return SLOT_WAIST;
    case INVTYPE_WRISTS:
      return SLOT_WRISTS;
    case INVTYPE_BODY:
      return SLOT_SHIRT;
    default:
      return SLOT_INVALID;
  }
}

// socket_gem_match =========================================================

bool util::socket_gem_match( item_socket_color socket, item_socket_color gem )
{
  return ( gem & socket ) != 0;
}

// string_split =============================================================

std::vector<std::string> util::string_split( const std::string& str, const std::string& delim )
{
  std::vector<std::string> results;
  if ( str.empty() )
    return results;

  std::string::size_type cut_pt, start = 0;

  while ( ( cut_pt = str.find_first_of( delim, start ) ) != str.npos )
  {
    if ( cut_pt > start ) // Found something, push to the vector
      results.push_back( str.substr( start, cut_pt - start ) );

    start = cut_pt + 1; // skip the found delimeter
  }

  if ( start < str.size() )
  {
    // Push the tail
    results.push_back( str.substr( start, str.size() - start ) );
  }

  return results;
}

/* Splits the string while skipping and stripping quoted parts in the string
 */
size_t util::string_split_allow_quotes( std::vector<std::string>& results, const std::string& str, const char* delim )
{
  std::string buffer = str;
  std::string::size_type cut_pt, start = 0;

  std::string not_in_quote = delim;
  not_in_quote += '"';

  static const std::string in_quote = "\"";
  const std::string* search = &not_in_quote;

  while ( ( cut_pt = buffer.find_first_of( *search, start ) ) != buffer.npos )
  {
    if ( buffer[ cut_pt ] == '"' )
    {
      buffer.erase( cut_pt, 1 );
      start = cut_pt;
      search = ( search == &not_in_quote ) ? &in_quote : &not_in_quote;
    }
    else if ( search == &not_in_quote )
    {
      if ( cut_pt > 0 )
        results.push_back( buffer.substr( 0, cut_pt ) );
      buffer.erase( 0, cut_pt + 1 );
      start = 0;
    }
  }

  if ( buffer.length() > 0 )
    results.push_back( buffer );

  return results.size();
}


/* replaces all occurrences of 'from' in the string 's', with 'to'
 */
void util::replace_all( std::string& s, const std::string& from, const std::string& to )
{
  std::string::size_type pos;
  if ( ( pos = s.find( from ) ) != s.npos )
  {
    std::string::size_type from_length = from.length();
    std::string::size_type to_len = to.length();
    do
    {
      s.replace( pos, from_length, to );
      pos += to_len;
    }
    while ( ( pos = s.find( from, pos ) ) != s.npos );
  }
}

// erase_all ================================================================

void util::erase_all( std::string& s, const std::string& from )
{
  std::string::size_type pos;

  if ( ( pos = s.find( from ) ) != s.npos )
  {
    do
    {
      s.erase( pos, from.length() );
    }
    while ( ( pos = s.find( from ) ) != s.npos );
  }
}

// item_quality_string ======================================================

const char* util::item_quality_string( int quality )
{
  switch ( quality )
  {
    case 1:   return "common";
    case 2:   return "uncommon";
    case 3:   return "rare";
    case 4:   return "epic";
    case 5:   return "legendary";
    case 6:   return "artifact";
    case 7:   return "heirloom";
    default:  return "poor";
  }
}

// specialization_string ====================================================

const char* util::specialization_string( specialization_e spec )
{
  for ( size_t i = 0; i < sizeof_array( spec_map ); i++ )
  {
    if ( spec == spec_map[ i ].spec )
      return spec_map[ i ].name;
  }

  return "Unknown";
}

// parse_position_type ======================================================

specialization_e util::parse_specialization_type( const std::string &name )
{
  for ( size_t i = 0; i < sizeof_array( spec_map ); i++ )
  {
    if ( util::str_compare_ci( name, spec_map[ i ].name ) )
      return spec_map[ i ].spec;
  }

  return SPEC_NONE;
}

// parse_item_quality =======================================================

int util::parse_item_quality( const std::string& quality )
{
  int i = 6;

  while ( --i > 0 )
    if ( str_compare_ci( quality, item_quality_string( i ) ) )
      break;

  return i;
}

// string_split =============================================================

size_t util::string_split( const std::string& str,
                           const char*        delim,
                           const char*        format, ... )
{
  std::vector<std::string> str_splits = string_split( str,    delim );
  std::vector<std::string> format_splits = string_split( format, " " );

  if ( str_splits.size() == format_splits.size() )
  {
    va_list vap;
    va_start( vap, format );

    for ( size_t i = 0; i < str_splits.size(); i++ )
    {
      std::string& f = format_splits[ i ];
      const char*  s =    str_splits[ i ].c_str();

      if      ( f == "i" ) *( va_arg( vap, int*    ) ) = atoi( s );
      else if ( f == "f" ) *( va_arg( vap, double* ) ) = atof( s );
      else if ( f == "d" ) *( va_arg( vap, double* ) ) = atof( s );
      else if ( f == "S" ) *( va_arg( vap, std::string* ) ) = s;
      else assert( 0 );
    }

    va_end( vap );
  }

  return str_splits.size();
}

// string_strip_quotes ======================================================

void util::string_strip_quotes( std::string& str )
{
  std::string::size_type pos = str.find( '"' );
  if ( pos == str.npos ) return;

  std::string::iterator dst = str.begin() + pos, src = dst;
  while ( ++src != str.end() )
  {
    if ( *src != '"' )
      *dst++ = *src;
  }

  str.resize( dst - str.begin() );
}

// to_string ================================================================

std::string util::to_string( double f, int precision )
{
  std::ostringstream ss;
  ss << std::fixed << std::setprecision( precision ) << f;
  return ss.str();
}

// to_string ================================================================

std::string util::to_string( double f )
{
  if ( std::abs( f - static_cast<int>( f ) ) < 0.001 )
    return to_string( static_cast<int>( f ) );
  else
    return to_string( f, 3 );
}

// to_unsigned ==============================================================

unsigned util::to_unsigned( const std::string& str )
{ return util::to_unsigned( str.c_str() ); }

unsigned util::to_unsigned( const char* str )
{
  errno = 0;

  unsigned l = strtoul( str, nullptr, 0 );
  if ( errno != 0 )
    return 0;

  return l;
}

// to_int ===================================================================

int util::to_int( const std::string& str )
{ return util::to_int( str.c_str() ); }

int util::to_int( const char* str )
{
  errno = 0;

  long l = strtol( str, nullptr, 0 );
  if ( errno != 0 )
    return 0;

  return l;
}

// parse_date ===============================================================

int64_t util::parse_date( const std::string& month_day_year )
{
  std::vector<std::string> splits = string_split( month_day_year, " _,;-/ \t\n\r" );
  if ( splits.size() != 3 ) return 0;

  std::string& month = splits[ 0 ];
  std::string& day   = splits[ 1 ];
  std::string& year  = splits[ 2 ];

  tolower( month );

  if ( month.find( "jan" ) != std::string::npos ) month = "01";
  if ( month.find( "feb" ) != std::string::npos ) month = "02";
  if ( month.find( "mar" ) != std::string::npos ) month = "03";
  if ( month.find( "apr" ) != std::string::npos ) month = "04";
  if ( month.find( "may" ) != std::string::npos ) month = "05";
  if ( month.find( "jun" ) != std::string::npos ) month = "06";
  if ( month.find( "jul" ) != std::string::npos ) month = "07";
  if ( month.find( "aug" ) != std::string::npos ) month = "08";
  if ( month.find( "sep" ) != std::string::npos ) month = "09";
  if ( month.find( "oct" ) != std::string::npos ) month = "10";
  if ( month.find( "nov" ) != std::string::npos ) month = "11";
  if ( month.find( "dec" ) != std::string::npos ) month = "12";

  if ( month.size() == 1 ) month.insert( month.begin(), '0'  );
  if ( day  .size() == 1 ) day  .insert( day  .begin(), '0'  );
  if ( year .size() == 2 ) year .insert( year .begin(), 2, '0' );

  if ( day.size()   != 2 ) return 0;
  if ( month.size() != 2 ) return 0;
  if ( year.size()  != 4 ) return 0;

  std::string buffer = year + month + day;

  return atoi( buffer.c_str() );
}

// fprintf ==================================================================

int util::fprintf( FILE *stream, const char *format,  ... )
{
  va_list fmtargs;
  va_start( fmtargs, format );

  int retcode = vfprintf_helper( stream, format, fmtargs );

  va_end( fmtargs );

  return retcode;
}

// printf ===================================================================

int util::printf( const char *format,  ... )
{
  va_list fmtargs;
  va_start( fmtargs, format );

  int retcode = vfprintf_helper( stdout, format, fmtargs );

  va_end( fmtargs );

  return retcode;
}

// snprintf =================================================================

int util::snformat( char* buf, size_t size, const char* fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  std::string buffer;
  str::format( buffer, fmt, ap );
  va_end( ap );
  assert( size > buffer.size() );
  strncpy( buf, buffer.c_str(), size-1 );
  return static_cast<int>( buffer.size() );
}

// vfprintf =================================================================

int util::vfprintf( FILE *stream, const char *format, va_list fmtargs )
{
  return vfprintf_helper( stream, format, fmtargs );
}

// vprintf ==================================================================

int util::vprintf( const char *format, va_list fmtargs )
{
  return util::vfprintf( stdout, format, fmtargs );
}

// urlencode ================================================================

void util::urlencode( std::string& str )
{
  if ( str.empty() )
    return;

  std::string temp;

  for ( std::string::size_type i = 0, l = str.length(); i < l; ++i )
  {
    unsigned char c = str[ i ];

    if ( c > 0x7F || c == ' ' || c == '\'' )
    {
      temp += "%";
      temp += uchar_to_hex( c );
    }
    else if ( c == '+' )
      temp += "%20";
    else if ( c < 0x20 )
      continue;
    else
      temp += c;
  }

  str.swap( temp );
}

// google image chart encoding ================================================

std::string util::google_image_chart_encode( const std::string& str )
{
  std::string::size_type l = str.length();
  if ( ! l ) return str;

  std::string temp;
  for ( std::string::size_type i = 0; i < l; ++i )
  {
    unsigned char c = str[ i ];
    if ( c == '+' )
      temp += "%2B";
    else if ( c == '&' )
      temp += "%26";
    else if ( c == '|' )
      temp += "%7E"; // pipe is a newline in google API, replace with ~
    else if ( c == '>' )
      temp += "%3E";
    else
      temp += c;
  }

  return temp;
}

// create_wowhead_artifact_url ==============================================
std::string util::create_wowhead_artifact_url( const player_t& p )
{
  std::string base_url = "http://legion.wowhead.com/artifact-calc#";

  unsigned artifact_id = p.dbc.artifact_by_spec( p.specialization() );
  std::string artifact_str = util::to_string( artifact_id ) + ":";
  std::array<unsigned, MAX_ARTIFACT_RELIC> relics;
  if ( range::find_if( p.artifact.relics, []( unsigned r ) { return r > 0; } ) != p.artifact.relics.end() )
  {
    relics = p.artifact.relics;
  }
  else
  {
    auto artifact_it = range::find_if( p.items, []( const item_t& i ) { return i.parsed.data.id_artifact > 0; } );
    if ( artifact_it != p.items.end() )
    {
      range::copy( ( *artifact_it ).parsed.gem_id, relics.begin() );
    }
  }

  range::for_each( relics, [ &artifact_str ]( unsigned relic_id ) {
    artifact_str += util::to_string( relic_id ) + ":";
  } );

  std::vector<const artifact_power_data_t*> artifact_powers = p.dbc.artifact_powers( artifact_id );
  for ( size_t i = 0; i < artifact_powers.size(); ++i )
  {
    auto total_ranks = p.artifact.points[ i ].first + p.artifact.points[ i ].second;
    if ( total_ranks == 0 )
    {
      continue;
    }

    artifact_str += util::to_string( artifact_powers[ i ] -> id );
    artifact_str += ":";
    artifact_str += util::to_string( +total_ranks );

    if ( i < artifact_powers.size() - 1 )
    {
      artifact_str += ":";
    }
  }

  return base_url + artifact_str;
}

// create_blizzard_talent_url ===============================================

std::string util::create_blizzard_talent_url( const player_t& p )
{
  std::string url = "http://us.battle.net/wow/en/tool/talent-calculator#";
  switch ( p.specialization() )
  {
   case DEATH_KNIGHT_BLOOD:   url += "daa"; break;
   case DEATH_KNIGHT_FROST:   url += "dZa"; break;
   case DEATH_KNIGHT_UNHOLY:  url += "dba"; break;
   case DEMON_HUNTER_HAVOC:   url += "gaa"; break;
   case DEMON_HUNTER_VENGEANCE: url += "gZa"; break;
   case DRUID_BALANCE:        url += "Uaa"; break;
   case DRUID_FERAL:          url += "UZa"; break;
   case DRUID_GUARDIAN:       url += "Uba"; break;
   case DRUID_RESTORATION:    url += "UYa"; break;
   case HUNTER_BEAST_MASTERY: url += "Yaa"; break;
   case HUNTER_MARKSMANSHIP:  url += "YZa"; break;
   case HUNTER_SURVIVAL:      url += "Yba"; break;
   case MAGE_ARCANE:          url += "eaa"; break;
   case MAGE_FIRE:            url += "eZa"; break;
   case MAGE_FROST:           url += "eba"; break;
   case MONK_BREWMASTER:      url += "faa"; break;
   case MONK_MISTWEAVER:      url += "fZa"; break;
   case MONK_WINDWALKER:      url += "fba"; break;
   case PALADIN_HOLY:         url += "baa"; break;
   case PALADIN_PROTECTION:   url += "bZa"; break;
   case PALADIN_RETRIBUTION:  url += "bba"; break;
   case PRIEST_DISCIPLINE:    url += "Xaa"; break;
   case PRIEST_HOLY:          url += "XZa"; break;
   case PRIEST_SHADOW:        url += "Xba"; break;
   case ROGUE_ASSASSINATION:  url += "caa"; break;
   case ROGUE_OUTLAW:         url += "cZa"; break;
   case ROGUE_SUBTLETY:       url += "cba"; break;
   case SHAMAN_ELEMENTAL:     url += "Waa"; break;
   case SHAMAN_ENHANCEMENT:   url += "WZa"; break;
   case SHAMAN_RESTORATION:   url += "Wba"; break;
   case WARLOCK_AFFLICTION:   url += "Vaa"; break;
   case WARLOCK_DEMONOLOGY:   url += "VZa"; break;
   case WARLOCK_DESTRUCTION:  url += "Vba"; break;
   case WARRIOR_ARMS:         url += "Zaa"; break;
   case WARRIOR_FURY:         url += "ZZa"; break;
   case WARRIOR_PROTECTION:   url += "Zba"; break;
   default: return "";
  }
  url += "!";
  for ( int i = 0; i < MAX_TALENT_ROWS; i++ )
  {
    if ( p.talent_points.choice( i ) >= 0 )
      url += util::to_string( p.talent_points.choice( i ) );
    else
      url += ".";  
  }
  return url;
}

// urldecode ================================================================

void util::urldecode( std::string& str )
{
  if ( str.empty() )
    return;

  std::string temp;

  for ( std::string::size_type i = 0, l = str.length(); i < l; ++i )
  {
    unsigned char c = ( unsigned char ) str[ i ];

    if ( c == '%' && i + 2 < l )
    {
      long v = strtol( str.substr( i + 1, 2 ).c_str(), nullptr, 16 );
      if ( v ) temp += ( unsigned char ) v;
      i += 2;
    }
    else if ( c == '+' )
      temp += ' ';
    else
      temp += c;
  }

  str.swap( temp );
}

// decode_html ==============================================================

std::string util::decode_html( const std::string& input )
{
  std::string output;
  std::string::size_type lastpos = 0;

  while ( true )
  {
    const std::string::size_type pos = input.find( '&', lastpos );
    output.append( input, lastpos, pos - lastpos );
    if ( pos == std::string::npos )
      break;

    const std::string::size_type end = input.find( ';', pos + 1 );
    if ( end == std::string::npos )
    {
      // Junk: pass it through.
      output.append( input, pos, std::string::npos );
      break;
    }

    if ( input[ pos + 1 ] == '#' )
    {
      char* endp;
      bool hex = ( std::tolower( input[ pos + 2 ] ) == 'x' );
      uint32_t codepoint = strtoul( input.c_str() + pos + 2 + ( hex ? 1 : 0 ), &endp, hex ? 16 : 10 );
      if ( endp != input.c_str() + end )
      {
        // Not everything parsed. Oh well.
        
      }
      utf8::append( codepoint, std::back_inserter( output ) );
    }
    else
    {
      int i = as<int>( sizeof_array( html_named_character_map ) );
      while ( --i >= 0 )
      {
        if ( ! input.compare( pos + 1, end - ( pos + 1 ), html_named_character_map[ i ].encoded ) )
        {
          output += html_named_character_map[ i ].decoded;
          break;
        }
      }

      if ( i < 0 )
      {
        // No match: Pass it through.
        output.append( input, pos, end - pos + 1 );
      }
    }

    lastpos = end + 1;
  }

  return output;
}

bool util::is_combat_rating( item_mod_type t )
{
  switch ( t )
  {
    case ITEM_MOD_MASTERY_RATING:
    case ITEM_MOD_DODGE_RATING:
    case ITEM_MOD_PARRY_RATING:
    case ITEM_MOD_BLOCK_RATING:
    case ITEM_MOD_HIT_MELEE_RATING:
    case ITEM_MOD_HIT_RANGED_RATING:
    case ITEM_MOD_HIT_SPELL_RATING:
    case ITEM_MOD_CRIT_MELEE_RATING:
    case ITEM_MOD_CRIT_RANGED_RATING:
    case ITEM_MOD_CRIT_SPELL_RATING:
    case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
    case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
    case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
    case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
    case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
    case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
    case ITEM_MOD_HASTE_MELEE_RATING:
    case ITEM_MOD_HASTE_RANGED_RATING:
    case ITEM_MOD_HASTE_SPELL_RATING:
    case ITEM_MOD_HIT_RATING:
    case ITEM_MOD_CRIT_RATING:
    case ITEM_MOD_HIT_TAKEN_RATING:
    case ITEM_MOD_CRIT_TAKEN_RATING:
    case ITEM_MOD_RESILIENCE_RATING:
    case ITEM_MOD_HASTE_RATING:
    case ITEM_MOD_EXPERTISE_RATING:
    case ITEM_MOD_MULTISTRIKE_RATING:
    case ITEM_MOD_SPEED_RATING:
    case ITEM_MOD_LEECH_RATING:
    case ITEM_MOD_AVOIDANCE_RATING:
    case ITEM_MOD_VERSATILITY_RATING:
      return true;
    default:
      return false;
  }
}

// encode_html ==============================================================

std::string util::encode_html( const std::string& s )
{
  std::string buffer = s;
  replace_all( buffer, "&", "&amp;" );
  replace_all( buffer, "<", "&lt;" );
  replace_all( buffer, ">", "&gt;" );
  return buffer;
}

/* Turn a unsigned char ( 0-255 ) into a hexadecimal string representation
 * with length 2 and upper case letters
 */
std::string util::uchar_to_hex( unsigned char c )
{
  std::stringstream out;

  out.width( 2 ); out.fill( '0' ); // Make sure we always fill out two spaces, so we get 00 not 0
  out << std::fixed << std::uppercase << std::hex << static_cast<int>( c );

  return out.str();
}

// floor ====================================================================

double util::floor( double X, unsigned int decplaces )
{
  switch ( decplaces )
  {
    case 0: return ::floor( X );
    case 1: return ::floor( X * 10.0 ) * 0.1;
    case 2: return ::floor( X * 100.0 ) * 0.01;
    case 3: return ::floor( X * 1000.0 ) * 0.001;
    case 4: return ::floor( X * 10000.0 ) * 0.0001;
    case 5: return ::floor( X * 100000.0 ) * 0.00001;
    default:
      double mult = 1000000.0;
      double div = 0.000001;
      for ( unsigned int i = 6; i < decplaces; i++ )
      {
        mult *= 10.0;
        div *= 0.1;
      }
      return ::floor( X * mult ) * div;
  }
}

// ceil =====================================================================

double util::ceil( double X, unsigned int decplaces )
{
  switch ( decplaces )
  {
    case 0: return ::ceil( X );
    case 1: return ::ceil( X * 10.0 ) * 0.1;
    case 2: return ::ceil( X * 100.0 ) * 0.01;
    case 3: return ::ceil( X * 1000.0 ) * 0.001;
    case 4: return ::ceil( X * 10000.0 ) * 0.0001;
    case 5: return ::ceil( X * 100000.0 ) * 0.00001;
    default:
      double mult = 1000000.0;
      double div = 0.000001;
      for ( unsigned int i = 6; i < decplaces; i++ )
      {
        mult *= 10.0;
        div *= 0.1;
      }
      return ::ceil( X * mult ) * div;
  }
}

// round ====================================================================

double util::round( double X, unsigned int decplaces )
{
  switch ( decplaces )
  {
    case 0: return ::floor( X + 0.5 );
    case 1: return ::floor( X * 10.0 + 0.5 ) * 0.1;
    case 2: return ::floor( X * 100.0 + 0.5 ) * 0.01;
    case 3: return ::floor( X * 1000.0 + 0.5 ) * 0.001;
    case 4: return ::floor( X * 10000.0 + 0.5 ) * 0.0001;
    case 5: return ::floor( X * 100000.0 + 0.5 ) * 0.00001;
    default:
      double mult = 1000000.0;
      double div = 0.000001;
      for ( unsigned int i = 6; i < decplaces; i++ )
      {
        mult *= 10.0;
        div *= 0.1;
      }
      return ::floor( X * mult + 0.5 ) * div;
  }
}

// tolower ==================================================================

void util::tolower( std::string& str )
{
  // Transform all chars to lower case
  range::transform_self( str, ( int( * )( int ) ) std::tolower );
}

// tokenize =================================================================

void util::tokenize( std::string& name )
{
  if ( name.empty() ) return;

  // remove leading '_' or '+'
  std::string::size_type n = name.find_first_not_of( "_+" );
  std::string::iterator it = name.erase( name.begin(), name.begin() + n );

  for ( ; it != name.end(); )
  {
    unsigned char c = *it;

    if ( c >= 0x80 )
    {
      it = name.erase( it );
    }
    else if ( std::isalpha( c ) )
    {
      *it++ = std::tolower( c );
    }
    else if ( c == ' ' )
    {
      *it++ = '_';
    }
    else if ( c != '_' &&
              c != '+' &&
              c != '.' &&
              c != '%' &&
              ! std::isdigit( c ) )
    {
      it = name.erase( it );
    }
    else
    {
      ++it; // Just skip it
    }
  }
}

// inverse_tokenize =========================================================

std::string util::inverse_tokenize( const std::string& name )
{
  std::string s = name;

  for ( std::string::iterator i = s.begin(); i != s.end(); ++i )
  {
    if ( *i == '_' )
    {
      *i = ' ';
      ++i;
      if ( i == s.end() )
        break;
      *i = std::toupper( *i );
    }
    else if ( i == s.begin() )
    {
      *i = std::toupper( *i );
    }
  }

  return s;
}

// is_number ================================================================

bool util::is_number( const std::string& s )
{
  for (auto & elem : s)
    if ( ! std::isdigit( elem ) )
      return false;
  return true;
}

// stat_search ==============================================================

static void stat_search( std::string&              encoding_str,
                         std::vector<std::string>& description_tokens,
                         stat_e                    type,
                         const std::string&        stat_str )
{
  std::vector<std::string> stat_tokens = util::string_split( stat_str, " " );
  size_t num_descriptions = description_tokens.size();

  for ( size_t i = 0; i < num_descriptions; i++ )
  {
    bool match = true;

    for ( size_t j = 0; j < stat_tokens.size() && match; j++ )
    {
      if ( ( i + j ) == num_descriptions )
      {
        match = false;
      }
      else
      {
        if ( stat_tokens[ j ][ 0 ] == '!' )
        {
          if ( stat_tokens[ j ].substr( 1 ) == description_tokens[ i + j ] )
          {
            match = false;
          }
        }
        else
        {
          if ( stat_tokens[ j ] != description_tokens[ i + j ] )
          {
            match = false;
          }
        }
      }
    }

    if ( match )
    {
      std::string value_str;

      if ( ( i > 0 ) &&
           ( util::is_number( description_tokens[ i - 1 ] ) ) )
      {
        value_str = description_tokens[ i - 1 ];
      }
      if ( ( ( i + stat_tokens.size() + 1 ) < num_descriptions ) &&
           ( description_tokens[ i + stat_tokens.size() ] == "by" ) &&
           ( util::is_number( description_tokens[ i + stat_tokens.size() + 1 ] ) ) )
      {
        value_str = description_tokens[ i + stat_tokens.size() + 1 ];
      }

      if ( ! value_str.empty() )
      {
        encoding_str += '_' + value_str + util::stat_type_abbrev( type );
      }
    }
  }
}

// is_proc_description ======================================================

static bool is_proc_description( const std::string& description_str )
{
  if ( description_str.find( "chance" ) != std::string::npos ) return true;
  if ( description_str.find( "stack"  ) != std::string::npos ) return true;
  if ( description_str.find( "time"   ) != std::string::npos ) return true;
  if ( ( description_str.find( "_sec"   ) != std::string::npos ) &&
       ! ( ( description_str.find( "restores" ) != std::string::npos ) &&
           ( ( description_str.find( "_per_5_sec" ) != std::string::npos ) ||
             ( description_str.find( "_every_5_sec" ) != std::string::npos ) ) ) )
    return true;

  return false;
}

// fuzzy_stats ==============================================================

void util::fuzzy_stats( std::string&       encoding_str,
                        const std::string& description_str )
{
  if ( description_str.empty() ) return;

  std::string buffer = description_str;
  util::tokenize( buffer );

  if ( is_proc_description( buffer ) )
    return;

  std::vector<std::string> splits = util::string_split( buffer, "_." );

  stat_search( encoding_str, splits, STAT_ALL,  "all stats" );
  stat_search( encoding_str, splits, STAT_ALL,  "to all stats" );

  stat_search( encoding_str, splits, STAT_STRENGTH,  "strength" );
  stat_search( encoding_str, splits, STAT_AGILITY,   "agility" );
  stat_search( encoding_str, splits, STAT_STAMINA,   "stamina" );
  stat_search( encoding_str, splits, STAT_INTELLECT, "intellect" );
  stat_search( encoding_str, splits, STAT_SPIRIT,    "spirit" );

  stat_search( encoding_str, splits, STAT_SPELL_POWER, "spell power" );

  stat_search( encoding_str, splits, STAT_ATTACK_POWER,     "attack power" );
  stat_search( encoding_str, splits, STAT_EXPERTISE_RATING, "expertise rating" );

  stat_search( encoding_str, splits, STAT_HASTE_RATING,     "haste" );
  stat_search( encoding_str, splits, STAT_HASTE_RATING,     "haste rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,       "ranged hit rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,       "hit rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,       "hit" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,      "ranged critical strike" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,      "critical strike rating" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,      "critical strike" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,      "crit rating" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,      "crit" );
  stat_search( encoding_str, splits, STAT_MASTERY_RATING,   "mastery rating" );
  stat_search( encoding_str, splits, STAT_MASTERY_RATING,   "mastery" );

  stat_search( encoding_str, splits, STAT_DODGE_RATING,     "dodge rating" );
  stat_search( encoding_str, splits, STAT_PARRY_RATING,     "parry rating" );
  stat_search( encoding_str, splits, STAT_BLOCK_RATING,     "block rating" );
  stat_search( encoding_str, splits, STAT_BONUS_ARMOR,      "bonus armor rating" );
  // WOD-TODO: hybrid primary stats?
}

/* Determine number of digits for a given Number
 *
 * generic solution
 */
template <class T>
int util::numDigits( T number )
{
  int digits = 0;
  if ( number < 0 ) digits = 1; // remove this line if '-' counts as a digit
  while ( number )
  {
    number /= 10;
    digits++;
  }
  return digits;
}

bool util::contains_non_ascii( const std::string& s )
{
  for (const auto & elem : s)
  {
    if ( elem < 0 || ! isprint( elem ) )
      return true;
  }

  return false;
}

namespace util {
/* Determine number of digits for a given Number
 *
 * partial specialization optimization for 32-bit numbers
 */
template<>
int numDigits( int32_t x )
{
  if ( x == std::numeric_limits<int32_t>::min() ) return 10 + 1;
  if ( x < 0 ) return numDigits( -x ) + 1;

  if ( x >= 10000 )
  {
    if ( x >= 10000000 )
    {
      if ( x >= 100000000 )
      {
        if ( x >= 1000000000 )
          return 10;
        return 9;
      }
      return 8;
    }
    if ( x >= 100000 )
    {
      if ( x >= 1000000 )
        return 7;
      return 6;
    }
    return 5;
  }
  if ( x >= 100 )
  {
    if ( x >= 1000 )
      return 4;
    return 3;
  }
  if ( x >= 10 )
    return 2;
  return 1;
}

double crit_multiplier( meta_gem_e gem )
{
  switch ( gem )
  {
    case META_AGILE_SHADOWSPIRIT:
    case META_AGILE_PRIMAL:
    case META_BURNING_SHADOWSPIRIT:
    case META_BURNING_PRIMAL:
    case META_CHAOTIC_SKYFIRE:
    case META_CHAOTIC_SKYFLARE:
    case META_CHAOTIC_SHADOWSPIRIT:
    case META_RELENTLESS_EARTHSIEGE:
    case META_RELENTLESS_EARTHSTORM:
    case META_REVERBERATING_SHADOWSPIRIT:
    case META_REVERBERATING_PRIMAL:
    case META_REVITALIZING_SKYFLARE:
    case META_REVITALIZING_SHADOWSPIRIT:
    case META_REVITALIZING_PRIMAL:
      return 1.03;
    default:
      return 1.0;
  }
}

// stream_printf ============================================================

std::ostream& stream_printf( std::ostream& stream, const char* fmt, ... )
{
  va_list fmtargs;
  va_start( fmtargs, fmt );
  stream << str::format( fmt, fmtargs );
  va_end( fmtargs );
  return stream;
}

} // namespace util

#ifdef _MSC_VER

// vsnprintf ================================================================

int vsnprintf_simc( char* buf, size_t size, const char* fmt, va_list ap )
{
  if ( buf && size )
  {
    std::string buffer = str::format( fmt, ap );
    strncpy( buf, buffer.c_str(), size-1 );
    return static_cast<int>( buffer.size() );
  }
  return vfprintf_helper( stdout, fmt, ap );
}

#endif

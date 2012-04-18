// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // ANONYMOUS ====================================================

// pred_ci ==================================================================

bool pred_ci ( char a, char b )
{
  return std::tolower( a ) == std::tolower( b );
}

// parse_enum ===============================================================

template <typename T, T Min, T Max, const char* F( T )>
inline T parse_enum( const std::string& name )
{
  for ( T i = Min; i < Max; ++i )
    if ( util_t::str_compare_ci( name, F( i ) ) )
      return i;
  return Min;
}

} // ANONYMOUS namespace ====================================================

// util_t::str_compare_ci ===================================================

bool util_t::str_compare_ci( const std::string& l,
                             const std::string& r )
{
  if ( l.size() != r.size() || l.size() == 0 )
    return false;

  return std::equal( l.begin(), l.end(), r.begin(), pred_ci );
}

std::string& util_t::glyph_name( std::string& n )
{
  util_t::armory_format( n );

  if ( n.substr( 0, 9 ) == "glyph_of_" ) n.erase( 0, 9 );
  if ( n.substr( 0, 7 ) == "glyph__" )   n.erase( 0, 7 );

  return n;
}

// util_t::str_prefix_ci ====================================================

bool util_t::str_prefix_ci( const std::string& str,
                            const std::string& prefix )
{
  if ( str.size() < prefix.size() )
    return false;

  return std::equal( prefix.begin(), prefix.end(), str.begin(), pred_ci );
}

// util_t::str_in_str_ci ====================================================

bool util_t::str_in_str_ci( const std::string& l,
                            const std::string& r )
{
  return std::search( l.begin(), l.end(), r.begin(), r.end(), pred_ci ) != l.end();
}

// util_t::talent_rank ======================================================

double util_t::talent_rank( int    num,
                            int    max,
                            double increment )
{
  assert( num >= 0 );
  assert( max > 0 );

  if ( num > max ) num = max;

  return num * increment;
}

// util_t::talent_rank ======================================================

int util_t::talent_rank( int num,
                         int max,
                         int increment )
{
  assert( num >= 0 );
  assert( max > 0 );

  if ( num > max ) num = max;

  return num * increment;
}

// util_t::talent_rank ======================================================

double util_t::talent_rank( int    num,
                            int    max,
                            double first,
                            double second, ... )
{
  assert( num >= 0 );
  assert( max > 0 );

  if ( num > max ) num = max;

  if ( num == 1 ) return first;
  if ( num == 2 ) return second;

  va_list vap;
  va_start( vap, second );

  double value=0;

  for ( int i=3; i <= num; i++ )
  {
    value = ( double ) va_arg( vap, double );
  }
  va_end( vap );

  return value;
}

// util_t::talent_rank ======================================================

int util_t::talent_rank( int num,
                         int max,
                         int first,
                         int second, ... )
{
  assert( num >= 0 );
  assert( max > 0 );

  if ( num > max ) num = max;

  if ( num == 1 ) return first;
  if ( num == 2 ) return second;

  va_list vap;
  va_start( vap, second );

  int value=0;

  for ( int i=3; i <= num; i++ )
  {
    value = ( int ) va_arg( vap, int );
  }
  va_end( vap );

  return value;
}

// util_t::ability_rank =====================================================

double util_t::ability_rank( int    player_level,
                             double ability_value,
                             int    ability_level, ... )
{
  va_list vap;
  va_start( vap, ability_level );

  while ( player_level < ability_level )
  {
    ability_value = ( double ) va_arg( vap, double );
    ability_level = ( int ) va_arg( vap, int );
  }

  va_end( vap );

  return ability_value;
}

// util_t::ability_rank =====================================================

int util_t::ability_rank( int player_level,
                          int ability_value,
                          int ability_level, ... )
{
  va_list vap;
  va_start( vap, ability_level );

  while ( player_level < ability_level )
  {
    ability_value = va_arg( vap, int );
    ability_level = va_arg( vap, int );
  }

  va_end( vap );

  return ability_value;
}

#ifdef _MSC_VER
// vsnprintf ================================================================

int vsnprintf_simc( char* buf, size_t size, const char* fmt, va_list ap )
{
  if ( buf && size )
  {
    int rval = _vsnprintf( buf, size, fmt, ap );
    if ( rval < 0 || static_cast<size_t>( rval ) < size )
      return rval;

    buf[ size - 1 ] = '\0';
  }

  return _vscprintf( fmt, ap );
}
#endif

// util_t::dot_behavior_type_string =========================================

const char* util_t::dot_behavior_type_string( dot_behavior_type_e t )
{
  switch ( t )
  {
  case DOT_REFRESH: return "DOT_REFRESH";
  case DOT_CLIP:    return "DOT_CLIP";
  case DOT_EXTEND:  return "DOT_EXTEND";
  default:          return "unknown";
  }
}

// util_t::role_type_string =================================================

const char* util_t::role_type_string( role_type_e role )
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

// util_t::parse_role_type ==================================================

role_type_e util_t::parse_role_type( const std::string& name )
{ return parse_enum<role_type_e,ROLE_NONE,ROLE_MAX,role_type_string>( name ); }

// util_t::race_type_string =================================================

const char* util_t::race_type_string( race_type_e type )
{
  switch ( type )
  {
  case RACE_NONE:      return "none";
  case RACE_BEAST:     return "beast";
  case RACE_BLOOD_ELF: return "blood_elf";
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
  default:             return "unknown";
  }
}

// util_t::parse_race_type ==================================================

race_type_e util_t::parse_race_type( const std::string &name )
{ return parse_enum<race_type_e,RACE_NONE,RACE_MAX,race_type_string>( name ); }

// util_t::position_type_string =============================================

const char* util_t::position_type_string( position_type_e type )
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

// util_t::parse_position_type ==============================================

position_type_e util_t::parse_position_type( const std::string &name )
{ return parse_enum<position_type_e,POSITION_NONE,POSITION_MAX,position_type_string>( name ); }

// util_t::profession_type_string ===========================================

const char* util_t::profession_type_string( profession_type_e type )
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

// util_t::parse_profession_type ============================================

profession_type_e util_t::parse_profession_type( const std::string& name )
{ return parse_enum<profession_type_e,PROFESSION_NONE,PROFESSION_MAX,profession_type_string>( name ); }

// util_t::translate_profession_id ==========================================

profession_type_e util_t::translate_profession_id( int skill_id )
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

// util_t::player_type_string ===============================================

const char* util_t::player_type_string( player_type_e type )
{
  switch ( type )
  {
  case PLAYER_NONE:     return "none";
  case DEATH_KNIGHT:    return "deathknight";
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
  default:              return "unknown";
  }
}

// util_t::parse_player_type ================================================

player_type_e util_t::parse_player_type( const std::string& name )
{ return parse_enum<player_type_e,PLAYER_NONE,PLAYER_MAX,player_type_string>( name ); }

// util_t::translate_class_str ================================================

player_type_e util_t::translate_class_str( std::string& s )
{
  return parse_enum<player_type_e,PLAYER_NONE,PLAYER_MAX,player_type_string>( s );
}


// util_t::pet_type_string ==================================================

const char* util_t::pet_type_string( pet_type_e type )
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
  case PET_BAT:                 return "bat";
  case PET_BIRD_OF_PREY:        return "bird_of_prey";
  case PET_CHIMERA:             return "chimera";
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
  case PET_EBON_IMP:            return "ebon_imp";
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

// util_t::parse_pet_type ===================================================

pet_type_e util_t::parse_pet_type( const std::string& name )
{ return parse_enum<pet_type_e,PET_NONE,PET_MAX,pet_type_string>( name ); }

// util_t::attribute_type_string ============================================

const char* util_t::attribute_type_string( attribute_type_e type )
{
  switch ( type )
  {
  case ATTR_STRENGTH:  return "strength";
  case ATTR_AGILITY:   return "agility";
  case ATTR_STAMINA:   return "stamina";
  case ATTR_INTELLECT: return "intellect";
  case ATTR_SPIRIT:    return "spirit";
  default:             return "unknown";
  }
}

// util_t::parse_attribute_type =============================================

attribute_type_e util_t::parse_attribute_type( const std::string& name )
{ return parse_enum<attribute_type_e,ATTRIBUTE_NONE,ATTRIBUTE_MAX,attribute_type_string>( name ); }

// util_t::dmg_type_string ==================================================

const char* util_t::dmg_type_string( dmg_type_e type )
{
  switch ( type )
  {
  case DMG_DIRECT:    return "hit";
  case DMG_OVER_TIME: return "tick";
  case HEAL_DIRECT:   return "heal";
  case HEAL_OVER_TIME:return "hot";
  case ABSORB:        return "absorb";
  default:            return "unknown";
  }
}

// util_t::gem_type_string ==================================================

const char* util_t::gem_type_string( gem_type_e type )
{
  switch ( type )
  {
  case GEM_META:      return "meta";
  case GEM_PRISMATIC: return "prismatic";
  case GEM_RED:       return "red";
  case GEM_YELLOW:    return "yellow";
  case GEM_BLUE:      return "blue";
  case GEM_ORANGE:    return "orange";
  case GEM_GREEN:     return "green";
  case GEM_PURPLE:    return "purple";
  case GEM_COGWHEEL:  return "cogwheel";
  default:            return "unknown";
  }
}

// util_t::parse_gem_type ===================================================

gem_type_e util_t::parse_gem_type( const std::string& name )
{ return parse_enum<gem_type_e,GEM_NONE,GEM_MAX,gem_type_string>( name ); }

// util_t::meta_gem_type_string =============================================

const char* util_t::meta_gem_type_string( meta_gem_type_e type )
{
  switch ( type )
  {
  case META_AGILE_SHADOWSPIRIT:         return "agile_shadowspirit";
  case META_AUSTERE_EARTHSIEGE:         return "austere_earthsiege";
  case META_AUSTERE_SHADOWSPIRIT:       return "austere_shadowspirit";
  case META_BEAMING_EARTHSIEGE:         return "beaming_earthsiege";
  case META_BRACING_EARTHSIEGE:         return "bracing_earthsiege";
  case META_BRACING_EARTHSTORM:         return "bracing_earthstorm";
  case META_BRACING_SHADOWSPIRIT:       return "bracing_shadowspirit";
  case META_BURNING_SHADOWSPIRIT:       return "burning_shadowspirit";
  case META_CHAOTIC_SHADOWSPIRIT:       return "chaotic_shadowspirit";
  case META_CHAOTIC_SKYFIRE:            return "chaotic_skyfire";
  case META_CHAOTIC_SKYFLARE:           return "chaotic_skyflare";
  case META_DESTRUCTIVE_SHADOWSPIRIT:   return "destructive_shadowspirit";
  case META_DESTRUCTIVE_SKYFIRE:        return "destructive_skyfire";
  case META_DESTRUCTIVE_SKYFLARE:       return "destructive_skyflare";
  case META_EFFULGENT_SHADOWSPIRIT:     return "effulgent_shadowspirit";
  case META_EMBER_SHADOWSPIRIT:         return "ember_shadowspirit";
  case META_EMBER_SKYFIRE:              return "ember_skyfire";
  case META_EMBER_SKYFLARE:             return "ember_skyflare";
  case META_ENIGMATIC_SHADOWSPIRIT:     return "enigmatic_shadowspirit";
  case META_ENIGMATIC_SKYFLARE:         return "enigmatic_skyflare";
  case META_ENIGMATIC_STARFLARE:        return "enigmatic_starflare";
  case META_ENIGMATIC_SKYFIRE:          return "enigmatic_skyfire";
  case META_ETERNAL_EARTHSIEGE:         return "eternal_earthsiege";
  case META_ETERNAL_EARTHSTORM:         return "eternal_earthstorm";
  case META_ETERNAL_SHADOWSPIRIT:       return "eternal_shadowspirit";
  case META_FLEET_SHADOWSPIRIT:         return "fleet_shadowspirit";
  case META_FORLORN_SHADOWSPIRIT:       return "forlorn_shadowspirit";
  case META_FORLORN_SKYFLARE:           return "forlorn_skyflare";
  case META_FORLORN_STARFLARE:          return "forlorn_starflare";
  case META_IMPASSIVE_SHADOWSPIRIT:     return "impassive_shadowspirit";
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
  case META_RELENTLESS_EARTHSIEGE:      return "relentless_earthsiege";
  case META_RELENTLESS_EARTHSTORM:      return "relentless_earthstorm";
  case META_REVERBERATING_SHADOWSPIRIT: return "reverberating_shadowspirit";
  case META_REVITALIZING_SHADOWSPIRIT:  return "revitalizing_shadowspirit";
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
  default:                              return "unknown";
  }
}

// util_t::parse_meta_gem_type ==============================================

meta_gem_type_e util_t::parse_meta_gem_type( const std::string& name )
{ return parse_enum<meta_gem_type_e,META_GEM_NONE,META_GEM_MAX,meta_gem_type_string>( name ); }

// util_t::result_type_string ===============================================

const char* util_t::result_type_string( result_type_e type )
{
  switch ( type )
  {
  case RESULT_NONE:       return "none";
  case RESULT_MISS:       return "miss";
  case RESULT_DODGE:      return "dodge";
  case RESULT_PARRY:      return "parry";
  case RESULT_BLOCK:      return "block";
  case RESULT_GLANCE:     return "glance";
  case RESULT_CRIT:       return "crit";
  case RESULT_CRIT_BLOCK: return "crit-block";
  case RESULT_HIT:        return "hit";
  default:                return "unknown";
  }
}

// util_t::parse_result_type ================================================

result_type_e util_t::parse_result_type( const std::string& name )
{ return parse_enum<result_type_e,RESULT_NONE,RESULT_MAX,result_type_string>( name ); }

// util_t::resource_type_string =============================================

const char* util_t::resource_type_string( resource_type_e resource_type )
{
  switch ( resource_type )
  {
  case RESOURCE_NONE:          return "none";
  case RESOURCE_HEALTH:        return "health";
  case RESOURCE_MANA:          return "mana";
  case RESOURCE_RAGE:          return "rage";
  case RESOURCE_ENERGY:        return "energy";
  case RESOURCE_MONK_ENERGY:   return "energy";
  case RESOURCE_FOCUS:         return "focus";
  case RESOURCE_RUNIC_POWER:   return "runic_power";
  case RESOURCE_RUNE:          return "rune";
  case RESOURCE_RUNE_BLOOD:    return "blood_rune";
  case RESOURCE_RUNE_UNHOLY:   return "unholy_rune";
  case RESOURCE_RUNE_FROST:    return "frost_rune";
  case RESOURCE_SOUL_SHARD:    return "soul_shard";
  case RESOURCE_BURNING_EMBER: return "burning_ember";
  case RESOURCE_DEMONIC_FURY:  return "demonic_fury";
  case RESOURCE_HOLY_POWER:    return "holy_power";
  case RESOURCE_CHI:           return "chi";
  case RESOURCE_SHADOW_ORB:    return "shadow_orb";
  default:                     return "unknown";
  }
}

// util_t::parse_resource_type ==============================================

resource_type_e util_t::parse_resource_type( const std::string& name )
{ return parse_enum<resource_type_e,RESOURCE_NONE,RESOURCE_MAX,resource_type_string>( name ); }

// util_t::school_type_component ============================================

uint32_t util_t::school_type_component( school_type_e s_type, school_type_e c_type )
{
  return spell_data_t::get_school_mask( s_type )
         & spell_data_t::get_school_mask( c_type );
}

// util_t::school_type_string ===============================================

const char* util_t::school_type_string( school_type_e school )
{
  switch ( school )
  {
  case SCHOOL_ARCANE:           return "arcane";
  case SCHOOL_BLEED:            return "bleed";
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
  case SCHOOL_SPELLSTORM:       return "spellstorm";
  case SCHOOL_SPELLFROST:       return "spellfrost";
  case SCHOOL_SPELLSHADOW:      return "spellshadow";
  case SCHOOL_ELEMENTAL:        return "elemental";
  case SCHOOL_CHROMATIC:        return "chromatic";
  case SCHOOL_MAGIC:            return "magic";
  case SCHOOL_DRAIN:            return "drain";
  default:                      return "unknown";
  }
}

// util_t::parse_school_type ================================================

school_type_e util_t::parse_school_type( const std::string& name )
{
  return parse_enum<school_type_e,SCHOOL_NONE,SCHOOL_MAX,school_type_string>( name );
}

// util_t::translate_spec_str ===============================================

specialization_e util_t::translate_spec_str( player_type_e ptype, const std::string& spec_str )
{
  switch ( ptype )
  {
  case DEATH_KNIGHT:
  {
    if ( util_t::str_compare_ci( spec_str, "blood" ) )
      return DEATH_KNIGHT_BLOOD;
    if ( util_t::str_compare_ci( spec_str, "tank" ) )
      return DEATH_KNIGHT_BLOOD;
    else if ( util_t::str_compare_ci( spec_str, "frost" ) )
      return DEATH_KNIGHT_FROST;
    else if ( util_t::str_compare_ci( spec_str, "unholy" ) )
      return DEATH_KNIGHT_UNHOLY;
    break;
  }
  case DRUID:
  {
    if ( util_t::str_compare_ci( spec_str, "balance" ) )
      return DRUID_BALANCE;
    if ( util_t::str_compare_ci( spec_str, "caster" ) )
      return DRUID_BALANCE;
    else if ( util_t::str_compare_ci( spec_str, "feral" ) )
      return DRUID_FERAL;
    else if ( util_t::str_compare_ci( spec_str, "cat" ) )
      return DRUID_FERAL;
    else if ( util_t::str_compare_ci( spec_str, "melee" ) )
      return DRUID_FERAL;
    else if ( util_t::str_compare_ci( spec_str, "guardian" ) )
      return DRUID_GUARDIAN;
    else if ( util_t::str_compare_ci( spec_str, "bear" ) )
      return DRUID_GUARDIAN;
    else if ( util_t::str_compare_ci( spec_str, "tank" ) )
      return DRUID_GUARDIAN;
    else if ( util_t::str_compare_ci( spec_str, "restoration" ) )
      return DRUID_RESTORATION;
    else if ( util_t::str_compare_ci( spec_str, "resto" ) )
      return DRUID_RESTORATION;
    else if ( util_t::str_compare_ci( spec_str, "healer" ) )
      return DRUID_RESTORATION;

    break;
  }
  case HUNTER:
  {
    if ( util_t::str_compare_ci( spec_str, "beast_mastery" ) )
      return HUNTER_BEAST_MASTERY;
    if ( util_t::str_compare_ci( spec_str, "bm" ) )
      return HUNTER_BEAST_MASTERY;
    else if ( util_t::str_compare_ci( spec_str, "marksmanship" ) )
      return HUNTER_MARKSMANSHIP;
    else if ( util_t::str_compare_ci( spec_str, "mm" ) )
      return HUNTER_MARKSMANSHIP;
    else if ( util_t::str_compare_ci( spec_str, "survival" ) )
      return HUNTER_SURVIVAL;
    else if ( util_t::str_compare_ci( spec_str, "sv" ) )
      return HUNTER_SURVIVAL;
    break;
  }
  case MAGE:
  {
    if ( util_t::str_compare_ci( spec_str, "arcane" ) )
      return MAGE_ARCANE;
    else if ( util_t::str_compare_ci( spec_str, "fire" ) )
      return MAGE_FIRE;
    else if ( util_t::str_compare_ci( spec_str, "frost" ) )
      return MAGE_FROST;
    break;
  }
  case MONK:
  {
    if ( util_t::str_compare_ci( spec_str, "brewmaster" ) )
      return MONK_BREWMASTER;
    if ( util_t::str_compare_ci( spec_str, "tank" ) )
      return MONK_BREWMASTER;
    else if ( util_t::str_compare_ci( spec_str, "mistweaver" ) )
      return MONK_MISTWEAVER;
    else if ( util_t::str_compare_ci( spec_str, "healer" ) )
      return MONK_MISTWEAVER;
    else if ( util_t::str_compare_ci( spec_str, "windwalker" ) )
      return MONK_WINDWALKER;
    else if ( util_t::str_compare_ci( spec_str, "dps" ) )
      return MONK_WINDWALKER;
    else if ( util_t::str_compare_ci( spec_str, "melee" ) )
      return MONK_WINDWALKER;
    break;
  }
  case PALADIN:
  {
    if ( util_t::str_compare_ci( spec_str, "holy" ) )
      return PALADIN_HOLY;
    if ( util_t::str_compare_ci( spec_str, "healer" ) )
      return PALADIN_HOLY;
    else if ( util_t::str_compare_ci( spec_str, "protection" ) )
      return PALADIN_PROTECTION;
    else if ( util_t::str_compare_ci( spec_str, "prot" ) )
      return PALADIN_PROTECTION;
    else if ( util_t::str_compare_ci( spec_str, "tank" ) )
      return PALADIN_PROTECTION;
    else if ( util_t::str_compare_ci( spec_str, "retribution" ) )
      return PALADIN_RETRIBUTION;
    else if ( util_t::str_compare_ci( spec_str, "ret" ) )
      return PALADIN_RETRIBUTION;
    else if ( util_t::str_compare_ci( spec_str, "dps" ) )
      return PALADIN_RETRIBUTION;
    else if ( util_t::str_compare_ci( spec_str, "melee" ) )
      return PALADIN_RETRIBUTION;
    break;
  }
  case PRIEST:
  {
    if ( util_t::str_compare_ci( spec_str, "discipline" ) )
      return PRIEST_DISCIPLINE;
    if ( util_t::str_compare_ci( spec_str, "disc" ) )
      return PRIEST_DISCIPLINE;
    else if ( util_t::str_compare_ci( spec_str, "holy" ) )
      return PRIEST_HOLY;
    else if ( util_t::str_compare_ci( spec_str, "shadow" ) )
      return PRIEST_SHADOW;
    else if ( util_t::str_compare_ci( spec_str, "caster" ) )
      return PRIEST_SHADOW;
    break;
  }
  case ROGUE:
  {
    if ( util_t::str_compare_ci( spec_str, "assassination" ) )
      return ROGUE_ASSASSINATION;
    if ( util_t::str_compare_ci( spec_str, "ass" ) )
      return ROGUE_ASSASSINATION;
    if ( util_t::str_compare_ci( spec_str, "mut" ) )
      return ROGUE_ASSASSINATION;
    else if ( util_t::str_compare_ci( spec_str, "combat" ) )
      return ROGUE_COMBAT;
    else if ( util_t::str_compare_ci( spec_str, "subtlety" ) )
      return ROGUE_SUBTLETY;
    else if ( util_t::str_compare_ci( spec_str, "sub" ) )
      return ROGUE_SUBTLETY;
    break;
  }
  case SHAMAN:
  {
    if ( util_t::str_compare_ci( spec_str, "elemental" ) )
      return SHAMAN_ELEMENTAL;
    if ( util_t::str_compare_ci( spec_str, "ele" ) )
      return SHAMAN_ELEMENTAL;
    if ( util_t::str_compare_ci( spec_str, "caster" ) )
      return SHAMAN_ELEMENTAL;
    else if ( util_t::str_compare_ci( spec_str, "enhancement" ) )
      return SHAMAN_ENHANCEMENT;
    else if ( util_t::str_compare_ci( spec_str, "enh" ) )
      return SHAMAN_ENHANCEMENT;
    else if ( util_t::str_compare_ci( spec_str, "melee" ) )
      return SHAMAN_ENHANCEMENT;
    else if ( util_t::str_compare_ci( spec_str, "restoration" ) )
      return SHAMAN_RESTORATION;
    else if ( util_t::str_compare_ci( spec_str, "resto" ) )
      return SHAMAN_RESTORATION;
    else if ( util_t::str_compare_ci( spec_str, "healer" ) )
      return SHAMAN_RESTORATION;
    break;
  }
  case WARLOCK:
  {
    if ( util_t::str_compare_ci( spec_str, "affliction" ) )
      return WARLOCK_AFFLICTION;
    if ( util_t::str_compare_ci( spec_str, "affl" ) )
      return WARLOCK_AFFLICTION;
    if ( util_t::str_compare_ci( spec_str, "aff" ) )
      return WARLOCK_AFFLICTION;
    else if ( util_t::str_compare_ci( spec_str, "demonology" ) )
      return WARLOCK_DEMONOLOGY;
    else if ( util_t::str_compare_ci( spec_str, "demo" ) )
      return WARLOCK_DEMONOLOGY;
    else if ( util_t::str_compare_ci( spec_str, "destruction" ) )
      return WARLOCK_DESTRUCTION;
    else if ( util_t::str_compare_ci( spec_str, "destro" ) )
      return WARLOCK_DESTRUCTION;
    break;
  }
  case WARRIOR:
  {
    if ( util_t::str_compare_ci( spec_str, "arms" ) )
      return WARRIOR_ARMS;
    else if ( util_t::str_compare_ci( spec_str, "fury" ) )
      return WARRIOR_FURY;
    else if ( util_t::str_compare_ci( spec_str, "protection" ) )
      return WARRIOR_PROTECTION;
    else if ( util_t::str_compare_ci( spec_str, "prot" ) )
      return WARRIOR_PROTECTION;
    else if ( util_t::str_compare_ci( spec_str, "tank" ) )
      return WARRIOR_PROTECTION;
    break;
  }
  default: break;
  }
  return SPEC_NONE;
}

// util_t::specialization_string ===============================================

const char* util_t::specialization_string( specialization_e spec, bool armory_format )
{
  if ( armory_format )
  {
    switch ( spec )
    {
    case WARRIOR_ARMS: return "arms";
    case WARRIOR_FURY: return "fury";
    case WARRIOR_PROTECTION: return "protection";
    case PALADIN_HOLY: return "holy";
    case PALADIN_PROTECTION: return "protection";
    case PALADIN_RETRIBUTION: return "retribution";
    case HUNTER_BEAST_MASTERY: return "beast_mastery";
    case HUNTER_MARKSMANSHIP: return "marksmanship";
    case HUNTER_SURVIVAL: return "survival";
    case ROGUE_ASSASSINATION: return "assassination";
    case ROGUE_COMBAT: return "combat";
    case ROGUE_SUBTLETY: return "subtlety";
    case PRIEST_DISCIPLINE: return "discipline";
    case PRIEST_HOLY: return "holy";
    case PRIEST_SHADOW: return "shadow";
    case DEATH_KNIGHT_BLOOD: return "blood";
    case DEATH_KNIGHT_FROST: return "frost";
    case DEATH_KNIGHT_UNHOLY: return "unholy";
    case SHAMAN_ELEMENTAL: return "elemental";
    case SHAMAN_ENHANCEMENT: return "enhancement";
    case SHAMAN_RESTORATION: return "restoration";
    case MAGE_ARCANE: return "arcane";
    case MAGE_FIRE: return "fire";
    case MAGE_FROST: return "frost";
    case WARLOCK_AFFLICTION: return "affliction";
    case WARLOCK_DEMONOLOGY: return "demonology";
    case WARLOCK_DESTRUCTION: return "destruction";
    case MONK_BREWMASTER: return "brewmaster";
    case MONK_MISTWEAVER: return "mistweaver";
    case MONK_WINDWALKER: return "windwalker";
    case DRUID_BALANCE: return "balance";
    case DRUID_FERAL: return "feral";
    case DRUID_GUARDIAN: return "guardian";
    case DRUID_RESTORATION: return "restoration";
    case PET_FEROCITY: return "ferocity";
    case PET_TENACITY: return "tenacity";
    case PET_CUNNING: return "cunning";
    default: return "unknown";
    }
  }
  else
  {
    switch ( spec )
    {
    case WARRIOR_ARMS: return "Arms";
    case WARRIOR_FURY: return "Fury";
    case WARRIOR_PROTECTION: return "Protection";
    case PALADIN_HOLY: return "Holy";
    case PALADIN_PROTECTION: return "Protection";
    case PALADIN_RETRIBUTION: return "Retribution";
    case HUNTER_BEAST_MASTERY: return "Beast Mastery";
    case HUNTER_MARKSMANSHIP: return "Marksmanship";
    case HUNTER_SURVIVAL: return "Survival";
    case ROGUE_ASSASSINATION: return "Assassination";
    case ROGUE_COMBAT: return "Combat";
    case ROGUE_SUBTLETY: return "Subtlety";
    case PRIEST_DISCIPLINE: return "Discipline";
    case PRIEST_HOLY: return "Holy";
    case PRIEST_SHADOW: return "Shadow";
    case DEATH_KNIGHT_BLOOD: return "Blood";
    case DEATH_KNIGHT_FROST: return "Frost";
    case DEATH_KNIGHT_UNHOLY: return "Unholy";
    case SHAMAN_ELEMENTAL: return "Elemental";
    case SHAMAN_ENHANCEMENT: return "Enhancement";
    case SHAMAN_RESTORATION: return "Restoration";
    case MAGE_ARCANE: return "Arcane";
    case MAGE_FIRE: return "Fire";
    case MAGE_FROST: return "Frost";
    case WARLOCK_AFFLICTION: return "Affliction";
    case WARLOCK_DEMONOLOGY: return "Demonology";
    case WARLOCK_DESTRUCTION: return "Destruction";
    case MONK_BREWMASTER: return "Brewmaster";
    case MONK_MISTWEAVER: return "Mistweaver";
    case MONK_WINDWALKER: return "Windwalker";
    case DRUID_BALANCE: return "Balance";
    case DRUID_FERAL: return "Feral";
    case DRUID_GUARDIAN: return "Guardian";
    case DRUID_RESTORATION: return "Restoration";
    case PET_FEROCITY: return "Ferocity";
    case PET_TENACITY: return "Tenacity";
    case PET_CUNNING: return "Cunning";
    default:                 return "Unknown";
    }
  }
}

resource_type_e util_t::translate_power_type( power_type_e pt )
{
  switch ( pt )
  {
  case POWER_HEALTH:        return RESOURCE_HEALTH;
  case POWER_MANA:          return RESOURCE_MANA;
  case POWER_RAGE:          return RESOURCE_RAGE;
  case POWER_FOCUS:         return RESOURCE_FOCUS;
  case POWER_ENERGY:        return RESOURCE_ENERGY;
  case POWER_MONK_ENERGY:   return RESOURCE_MONK_ENERGY;
  case POWER_RUNIC_POWER:   return RESOURCE_RUNIC_POWER;
  case POWER_SOUL_SHARDS:   return RESOURCE_SOUL_SHARD;
  case POWER_BURNING_EMBER: return RESOURCE_BURNING_EMBER;
  case POWER_DEMONIC_FURY:  return RESOURCE_DEMONIC_FURY;
  case POWER_HOLY_POWER:    return RESOURCE_HOLY_POWER;
  case POWER_CHI:           return RESOURCE_CHI;
  case POWER_SHADOW_ORB:    return RESOURCE_SHADOW_ORB;
  default:                  return RESOURCE_NONE;
  }
}

// util_t::weapon_type_string ===============================================

const char* util_t::weapon_type_string( weapon_type_e weapon )
{
  switch ( weapon )
  {
  case WEAPON_NONE:     return "none";
  case WEAPON_DAGGER:   return "dagger";
  case WEAPON_FIST:     return "fist";
  case WEAPON_BEAST:    return "beast";
  case WEAPON_SWORD:    return "sword";
  case WEAPON_MACE:     return "mace";
  case WEAPON_AXE:      return "axe";
  case WEAPON_BEAST_2H: return "beast2h";
  case WEAPON_SWORD_2H: return "sword2h";
  case WEAPON_MACE_2H:  return "mace2h";
  case WEAPON_AXE_2H:   return "axe2h";
  case WEAPON_STAFF:    return "staff";
  case WEAPON_POLEARM:  return "polearm";
  case WEAPON_BOW:      return "bow";
  case WEAPON_CROSSBOW: return "crossbow";
  case WEAPON_GUN:      return "gun";
  case WEAPON_THROWN:   return "thrown";
  case WEAPON_WAND:     return "wand";
  default:              return "unknown";
  }
}

// util_t::weapon_subclass_string ===========================================

const char* util_t::weapon_subclass_string( item_subclass_weapon subclass )
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

// util_t::weapon_class_string ==============================================

const char* util_t::weapon_class_string( inventory_type it )
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
  //case INVTYPE_RANGED:
  //case INVTYPE_THROWN:
  default:
    return 0;
  }
}

// util_t::set_item_type_string =============================================

const char* util_t::set_item_type_string( int item_set )
{
  switch ( item_set )
  {
    // Melee sets
  case 1057:   // DK T13
  case 1058:   // Druid T13
  case 1061:   // Hunter T13
  case 1064:   // Paladin T13
  case 1068:   // Rogue T13
  case 1071:   // Shaman T13
  case 1073:   // Warrior T13
    return "Melee";

    // Tank sets
  case 1056:   // DK T13
  case 1065:   // Paladin T13
  case 1074:   // Warrior T13
    return "Tank";

    // Healer sets
  case 1060:   // Druid T13
  case 1063:   // Paladin T13
  case 1066:   // Priest T13
  case 1069:   // Shaman T13
    return "Healer";

    // DPS Caster sets
  case 1059:   // Druid T13
  case 1062:   // Mage T13
  case 1067:   // Priest T13
  case 1070:   // Shaman T13
  case 1072:   // Warlock T13
    return "Caster";
  }
  return 0;
}

// util_t::parse_weapon_type ================================================

weapon_type_e util_t::parse_weapon_type( const std::string& name )
{ return parse_enum<weapon_type_e,WEAPON_NONE,WEAPON_MAX,weapon_type_string>( name ); }

// util_t::flask_type_string ================================================

const char* util_t::flask_type_string( flask_type_e flask )
{
  switch ( flask )
  {
  case FLASK_NONE:               return "none";
  case FLASK_BLINDING_LIGHT:     return "blinding_light";
  case FLASK_DISTILLED_WISDOM:   return "distilled_wisdom";
  case FLASK_DRACONIC_MIND:      return "draconic_mind";
  case FLASK_ENDLESS_RAGE:       return "endless_rage";
  case FLASK_ENHANCEMENT:        return "enhancement";
  case FLASK_FLOWING_WATER:      return "flowing_water";
  case FLASK_FROST_WYRM:         return "frost_wyrm";
  case FLASK_MIGHTY_RESTORATION: return "mighty_restoration";
  case FLASK_NORTH:              return "north";
  case FLASK_PURE_DEATH:         return "pure_death";
  case FLASK_PURE_MOJO:          return "pure_mojo";
  case FLASK_RELENTLESS_ASSAULT: return "relentless_assault";
  case FLASK_SUPREME_POWER:      return "supreme_power";
  case FLASK_STEELSKIN:          return "steelskin";
  case FLASK_TITANIC_STRENGTH:   return "titanic_strength";
  case FLASK_WINDS:              return "winds";
  default:                       return "unknown";
  }
}

// util_t::parse_flask_type =================================================

flask_type_e util_t::parse_flask_type( const std::string& name )
{ return parse_enum<flask_type_e,FLASK_NONE,FLASK_MAX,flask_type_string>( name ); }

// util_t::food_type_string =================================================

const char* util_t::food_type_string( food_type_e food )
{
  switch ( food )
  {
  case FOOD_NONE:                     return "none";
  case FOOD_BAKED_ROCKFISH:           return "baked_rockfish";
  case FOOD_BASILISK_LIVERDOG:        return "basilisk_liverdog";
  case FOOD_BEER_BASTED_CROCOLISK:    return "beer_basted_crocolisk";
  case FOOD_BLACKBELLY_SUSHI:         return "blackbelly_sushi";
  case FOOD_BLACKENED_BASILISK:       return "blackened_basilisk";
  case FOOD_BLACKENED_DRAGONFIN:      return "blackened_dragonfin";
  case FOOD_CROCOLISK_AU_GRATIN:      return "crocolisk_au_gratin";
  case FOOD_CRUNCHY_SERPENT:          return "crunchy_serpent";
  case FOOD_DELICIOUS_SAGEFISH_TAIL:  return "delicious_sagefish_tail";
  case FOOD_DRAGONFIN_FILET:          return "dragonfin_filet";
  case FOOD_FISH_FEAST:               return "fish_feast";
  case FOOD_FORTUNE_COOKIE:           return "fortune_cookie";
  case FOOD_GOLDEN_FISHSTICKS:        return "golden_fishsticks";
  case FOOD_GREAT_FEAST:              return "great_feast";
  case FOOD_GRILLED_DRAGON:           return "grilled_dragon";
  case FOOD_HEARTY_RHINO:             return "hearty_rhino";
  case FOOD_IMPERIAL_MANTA_STEAK:     return "imperial_manta_steak";
  case FOOD_LAVASCALE_FILLET:         return "lavascale_fillet";
  case FOOD_MEGA_MAMMOTH_MEAL:        return "mega_mammoth_meal";
  case FOOD_MUSHROOM_SAUCE_MUDFISH:   return "mushroom_sauce_mudfish";
  case FOOD_POACHED_NORTHERN_SCULPIN: return "poached_northern_sculpin";
  case FOOD_POACHED_BLUEFISH:         return "poached_bluefish";
  case FOOD_RHINOLICIOUS_WORMSTEAK:   return "rhinolicious_wormsteak";
  case FOOD_SEAFOOD_MAGNIFIQUE_FEAST: return "seafood_magnifique_feast";
  case FOOD_SEVERED_SAGEFISH_HEAD:    return "severed_sagefish_head";
  case FOOD_SKEWERED_EEL:             return "skewered_eel";
  case FOOD_SMOKED_SALMON:            return "smoked_salmon";
  case FOOD_SNAPPER_EXTREME:          return "snapper_extreme";
  case FOOD_TENDER_SHOVELTUSK_STEAK:  return "tender_shoveltusk_steak";
  case FOOD_VERY_BURNT_WORG:          return "very_burnt_worg";
  default:                            return "unknown";
  }
}

// util_t::parse_food_type ==================================================

food_type_e util_t::parse_food_type( const std::string& name )
{ return parse_enum<food_type_e,FOOD_NONE,FOOD_MAX,food_type_string>( name ); }

// util_t::set_bonus_string =================================================

const char* util_t::set_bonus_string( set_type_e type )
{
  switch ( type )
  {
  case SET_T13_2PC_CASTER: return "tier13_2pc_caster";
  case SET_T13_4PC_CASTER: return "tier13_4pc_caster";
  case SET_T13_2PC_MELEE:  return "tier13_2pc_melee";
  case SET_T13_4PC_MELEE:  return "tier13_4pc_melee";
  case SET_T13_2PC_TANK:   return "tier13_2pc_tank";
  case SET_T13_4PC_TANK:   return "tier13_4pc_tank";
  case SET_T13_2PC_HEAL:   return "tier13_2pc_heal";
  case SET_T13_4PC_HEAL:   return "tier13_4pc_heal";
  case SET_T14_2PC_CASTER: return "tier14_2pc_caster";
  case SET_T14_4PC_CASTER: return "tier14_4pc_caster";
  case SET_T14_2PC_MELEE:  return "tier14_2pc_melee";
  case SET_T14_4PC_MELEE:  return "tier14_4pc_melee";
  case SET_T14_2PC_TANK:   return "tier14_2pc_tank";
  case SET_T14_4PC_TANK:   return "tier14_4pc_tank";
  case SET_T14_2PC_HEAL:   return "tier14_2pc_heal";
  case SET_T14_4PC_HEAL:   return "tier14_4pc_heal";
  case SET_T15_2PC_CASTER: return "tier15_2pc_caster";
  case SET_T15_4PC_CASTER: return "tier15_4pc_caster";
  case SET_T15_2PC_MELEE:  return "tier15_2pc_melee";
  case SET_T15_4PC_MELEE:  return "tier15_4pc_melee";
  case SET_T15_2PC_TANK:   return "tier15_2pc_tank";
  case SET_T15_4PC_TANK:   return "tier15_4pc_tank";
  case SET_T15_2PC_HEAL:   return "tier15_2pc_heal";
  case SET_T15_4PC_HEAL:   return "tier15_4pc_heal";
  case SET_T16_2PC_CASTER: return "tier16_2pc_caster";
  case SET_T16_4PC_CASTER: return "tier16_4pc_caster";
  case SET_T16_2PC_MELEE:  return "tier16_2pc_melee";
  case SET_T16_4PC_MELEE:  return "tier16_4pc_melee";
  case SET_T16_2PC_TANK:   return "tier16_2pc_tank";
  case SET_T16_4PC_TANK:   return "tier16_4pc_tank";
  case SET_T16_2PC_HEAL:   return "tier16_2pc_heal";
  case SET_T16_4PC_HEAL:   return "tier16_4pc_heal";
  case SET_PVP_2PC_CASTER: return "pvp_2pc_caster";
  case SET_PVP_4PC_CASTER: return "pvp_4pc_caster";
  case SET_PVP_2PC_MELEE:  return "pvp_2pc_melee";
  case SET_PVP_4PC_MELEE:  return "pvp_4pc_melee";
  case SET_PVP_2PC_TANK:   return "pvp_2pc_tank";
  case SET_PVP_4PC_TANK:   return "pvp_4pc_tank";
  case SET_PVP_2PC_HEAL:   return "pvp_2pc_heal";
  case SET_PVP_4PC_HEAL:   return "pvp_4pc_heal";
  default:                 return "unknown";
  }
}

// util_t::parse_set_bonus ==================================================

set_type_e util_t::parse_set_bonus( const std::string& name )
{ return parse_enum<set_type_e,SET_NONE,SET_MAX,set_bonus_string>( name ); }

// util_t::slot_type_string =================================================

const char* util_t::slot_type_string( slot_type_e slot )
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
  case SLOT_RANGED:    return "ranged";
  case SLOT_TABARD:    return "tabard";
  default:             return "unknown";
  }
}

// util_t::armor_type_string ================================================

const char* util_t::armor_type_string( player_type_e ptype, slot_type_e s )
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
    break;
  default:
    return 0;
  }

  switch ( ptype )
  {
  case WARRIOR:
  case PALADIN:
  case DEATH_KNIGHT:
    return "plate";
  case HUNTER:
  case SHAMAN:
    return "mail";
  case DRUID:
  case ROGUE:
  case MONK:
    return "leather";
  case MAGE:
  case PRIEST:
  case WARLOCK:
    return "cloth";
  default:
    return NULL;
  }
}

// util_t::parse_slot_type ==================================================

slot_type_e util_t::parse_slot_type( const std::string& name )
{ return parse_enum<slot_type_e,SLOT_MIN,SLOT_MAX,slot_type_string>( name ); }

// util_t::stat_type_string =================================================

const char* util_t::stat_type_string( stat_type_e stat )
{
  switch ( stat )
  {
  case STAT_STRENGTH:  return "strength";
  case STAT_AGILITY:   return "agility";
  case STAT_STAMINA:   return "stamina";
  case STAT_INTELLECT: return "intellect";
  case STAT_SPIRIT:    return "spirit";

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
  case STAT_MP5:               return "mp5";

  case STAT_ATTACK_POWER:             return "attack_power";
  case STAT_EXPERTISE_RATING:         return "expertise_rating";
  case STAT_EXPERTISE_RATING2:        return "inverse_expertise_rating";

  case STAT_HIT_RATING:   return "hit_rating";
  case STAT_HIT_RATING2:  return "inverse_hit_rating";
  case STAT_CRIT_RATING:  return "crit_rating";
  case STAT_HASTE_RATING: return "haste_rating";

  case STAT_WEAPON_DPS:   return "weapon_dps";
  case STAT_WEAPON_SPEED: return "weapon_speed";

  case STAT_WEAPON_OFFHAND_DPS:    return "weapon_offhand_dps";
  case STAT_WEAPON_OFFHAND_SPEED:  return "weapon_offhand_speed";

  case STAT_ARMOR:             return "armor";
  case STAT_BONUS_ARMOR:       return "bonus_armor";
  case STAT_RESILIENCE_RATING: return "resilience_rating";
  case STAT_DODGE_RATING:      return "dodge_rating";
  case STAT_PARRY_RATING:      return "parry_rating";

  case STAT_BLOCK_RATING: return "block_rating";

  case STAT_MASTERY_RATING: return "mastery_rating";

  case STAT_ALL: return "all";

  default: return "unknown";
  }
}

// util_t::stat_type_abbrev =================================================

const char* util_t::stat_type_abbrev( stat_type_e stat )
{
  switch ( stat )
  {
  case STAT_STRENGTH:  return "Str";
  case STAT_AGILITY:   return "Agi";
  case STAT_STAMINA:   return "Sta";
  case STAT_INTELLECT: return "Int";
  case STAT_SPIRIT:    return "Spi";

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
  case STAT_MP5:               return "MP5";

  case STAT_ATTACK_POWER:             return "AP";
  case STAT_EXPERTISE_RATING:         return "Exp";
  case STAT_EXPERTISE_RATING2:        return "InvExp";

  case STAT_HIT_RATING:   return "Hit";
  case STAT_HIT_RATING2:  return "InvHit";
  case STAT_CRIT_RATING:  return "Crit";
  case STAT_HASTE_RATING: return "Haste";

  case STAT_WEAPON_DPS:   return "Wdps";
  case STAT_WEAPON_SPEED: return "Wspeed";

  case STAT_WEAPON_OFFHAND_DPS:    return "WOHdps";
  case STAT_WEAPON_OFFHAND_SPEED:  return "WOHspeed";

  case STAT_ARMOR:             return "Armor";
  case STAT_BONUS_ARMOR:       return "BArmor";
  case STAT_RESILIENCE_RATING: return "Resil";
  case STAT_DODGE_RATING:      return "Dodge";
  case STAT_PARRY_RATING:      return "Parry";

  case STAT_BLOCK_RATING: return "BlockR";

  case STAT_MASTERY_RATING: return "Mastery";

  case STAT_ALL: return "All";

  default: return "unknown";
  }
}

// util_t::stat_type_wowhead ================================================

const char* util_t::stat_type_wowhead( stat_type_e stat )
{
  switch ( stat )
  {
  case STAT_STRENGTH:  return "str";
  case STAT_AGILITY:   return "agi";
  case STAT_STAMINA:   return "sta";
  case STAT_INTELLECT: return "int";
  case STAT_SPIRIT:    return "spr";

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
  case STAT_WEAPON_SPEED: return "__speed";

  case STAT_ARMOR:             return "armor";
  case STAT_BONUS_ARMOR:       return "__armor"; // FIXME! Does wowhead distinguish "bonus" armor?
  case STAT_RESILIENCE_RATING: return "resilRating";
  case STAT_DODGE_RATING:      return "dodgeRating";
  case STAT_PARRY_RATING:      return "parryRating";

  case STAT_MASTERY_RATING: return "masteryRating";

  case STAT_MAX: return "__all";
  default: return "unknown";
  }
}

// util_t::parse_stat_type ==================================================

stat_type_e util_t::parse_stat_type( const std::string& name )
{
  stat_type_e s = parse_enum<stat_type_e,STAT_NONE,STAT_MAX,stat_type_string>( name );
  if ( s != STAT_NONE ) return s;

  s = parse_enum<stat_type_e,STAT_NONE,STAT_MAX,stat_type_abbrev>( name );
  if ( s != STAT_NONE ) return s;

  s = parse_enum<stat_type_e,STAT_NONE,STAT_MAX,stat_type_wowhead>( name );
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
  if ( util_t::str_compare_ci( name, "__wpds"   ) ) return STAT_WEAPON_DPS;
  if ( util_t::str_compare_ci( name, "__wspeed" ) ) return STAT_WEAPON_SPEED;

  return STAT_NONE;
}

// util_t::parse_reforge_type ===============================================

stat_type_e util_t::parse_reforge_type( const std::string& name )
{
  stat_type_e s = util_t::parse_stat_type( name );

  switch ( s )
  {
  case STAT_EXPERTISE_RATING:
  case STAT_HIT_RATING:
  case STAT_CRIT_RATING:
  case STAT_HASTE_RATING:
  case STAT_MASTERY_RATING:
  case STAT_SPIRIT:
  case STAT_DODGE_RATING:
  case STAT_PARRY_RATING:
    return s;
  default:
    return STAT_NONE;
  }
}

// util_t::parse_origin =====================================================

bool util_t::parse_origin( std::string& region_str,
                           std::string& server_str,
                           std::string& name_str,
                           const std::string& origin_str )
{
  if ( ( origin_str.find( ".battle."    ) == std::string::npos ) &&
       ( origin_str.find( ".wowarmory." ) == std::string::npos ) )
    return false;

  std::vector<std::string> tokens;
  size_t num_tokens = util_t::string_split( tokens, origin_str, "/:.?&=" );

  for ( size_t i = 0; i < num_tokens; i++ )
  {
    std::string& t = tokens[ i ];

    if ( t == "http" )
    {
      if ( ( i+1 ) >= num_tokens ) return false;
      region_str = tokens[ ++i ];
    }
    else if ( t == "r" ) // old armory
    {
      if ( ( i+1 ) >= num_tokens ) return false;
      server_str = tokens[ ++i ];
    }
    else if ( t == "n" || t == "cn" ) // old armory
    {
      if ( ( i+1 ) >= num_tokens ) return false;
      name_str = tokens[ ++i ];
    }
    else if ( t == "character" ) // new battle.net
    {
      if ( ( i+2 ) >= num_tokens ) return false;
      server_str = tokens[ ++i ];
      name_str   = tokens[ ++i ];
    }
  }

  if ( region_str.empty() ) return false;
  if ( server_str.empty() ) return false;
  if (   name_str.empty() ) return false;

  return true;
}

// util_t::class_id_mask ====================================================

int util_t::class_id_mask( player_type_e type )
{
  int cid = class_id( type );
  if ( cid <= 0 ) return 0;
  return 1 << ( cid - 1 );
}

// util_t::class_id =========================================================

int util_t::class_id( player_type_e type )
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
  case PLAYER_SPECIAL_SCALE: return 12;
  default:           return 0;
  }
}

// util_t::race_id ==========================================================

unsigned util_t::race_id( race_type_e r )
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

// util_t::race_mask ========================================================

unsigned util_t::race_mask( race_type_e r )
{
  uint32_t id = race_id( r );

  if ( id > 0 )
    return ( 1 << ( id - 1 ) );

  return 0x00;
}

// util_t::pet_class_type ===================================================

player_type_e util_t::pet_class_type( pet_type_e c )
{
  player_type_e p = WARRIOR;

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

// util_t::pet_mask =========================================================

unsigned util_t::pet_mask( pet_type_e p )
{
  if ( p <= PET_FEROCITY_TYPE )
    return 0x1;
  if ( p <= PET_TENACITY_TYPE )
    return 0x2;
  if ( p <= PET_CUNNING_TYPE )
    return 0x4;

  return 0x0;
}

// util_t::pet_id ===========================================================

unsigned util_t::pet_id( pet_type_e p )
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

// util_t::class_id_string ==================================================

const char* util_t::class_id_string( player_type_e type )
{
  switch ( type )
  {
  case WARRIOR:      return  "1";
  case PALADIN:      return  "2";
  case HUNTER:       return  "3";
  case ROGUE:        return  "4";
  case PRIEST:       return  "5";
  case DEATH_KNIGHT: return  "6";
  case SHAMAN:       return  "7";
  case MAGE:         return  "8";
  case WARLOCK:      return  "9";
  case MONK:         return "10";
  case DRUID:        return "11";
  default:           return "0";
  }
}

// util_t::translate_class_id ===============================================

player_type_e util_t::translate_class_id( int cid )
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
  default: return PLAYER_NONE;
  }
}

// util_t::translate_race_id ================================================

race_type_e util_t::translate_race_id( int rid )
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

// util_t::translate_item_mod ===============================================

stat_type_e util_t::translate_item_mod( item_mod_type item_mod )
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
  default:                           return STAT_NONE;
  }
}

// util_t::translate_weapon_subclass ========================================

weapon_type_e util_t::translate_weapon_subclass( item_subclass_weapon id )
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
  default: return WEAPON_NONE;
  }

  return WEAPON_NONE;
}

// util_t::translate_invtype ================================================

slot_type_e util_t::translate_invtype( inventory_type inv_type )
{
  switch ( inv_type )
  {
  case INVTYPE_2HWEAPON:
  case INVTYPE_WEAPON:
  case INVTYPE_WEAPONMAINHAND:
    return SLOT_MAIN_HAND;
  case INVTYPE_WEAPONOFFHAND:
  case INVTYPE_SHIELD:
  case INVTYPE_HOLDABLE:
    return SLOT_OFF_HAND;
  case INVTYPE_THROWN:
  case INVTYPE_RELIC:
  case INVTYPE_RANGED:
  case INVTYPE_RANGEDRIGHT:
    return SLOT_RANGED;
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
  default:
    return SLOT_INVALID;
  }
}

// util_t::socket_gem_match =================================================

bool util_t::socket_gem_match( gem_type_e socket, gem_type_e gem )
{
  if ( socket == GEM_NONE || gem == GEM_PRISMATIC ) return true;

  if ( socket == GEM_COGWHEEL ) return ( gem == GEM_COGWHEEL );
  if ( socket == GEM_META ) return ( gem == GEM_META );

  if ( socket == GEM_RED    ) return ( gem == GEM_RED    || gem == GEM_ORANGE || gem == GEM_PURPLE );
  if ( socket == GEM_YELLOW ) return ( gem == GEM_YELLOW || gem == GEM_ORANGE || gem == GEM_GREEN  );
  if ( socket == GEM_BLUE   ) return ( gem == GEM_BLUE   || gem == GEM_PURPLE || gem == GEM_GREEN  );

  return false;
}

// util_t::translate_gem_color ==============================================

gem_type_e util_t::translate_socket_color( item_socket_color c )
{
  switch ( c )
  {
  case SOCKET_COLOR_BLUE:      return GEM_BLUE;
  case SOCKET_COLOR_META:      return GEM_META;
  case SOCKET_COLOR_PRISMATIC: return GEM_PRISMATIC;
  case SOCKET_COLOR_RED:       return GEM_RED;
  case SOCKET_COLOR_YELLOW:    return GEM_YELLOW;
  case SOCKET_COLOR_COGWHEEL:  return GEM_COGWHEEL;
  case SOCKET_COLOR_NONE:
  default:                     return GEM_NONE;
  }
}

// util_t::item_quality_string ==============================================

const char* util_t::item_quality_string( int quality )
{
  switch ( quality )
  {
  case 1:   return "common";
  case 2:   return "uncommon";
  case 3:   return "rare";
  case 4:   return "epic";
  case 5:   return "legendary";
  default:  return "poor";
  }
}

// util_t::parse_item_quality ===============================================

int util_t::parse_item_quality( const std::string& quality )
{
  int i = 6;

  while ( --i > 0 )
    if ( str_compare_ci( quality, item_quality_string( i ) ) )
      break;

  return i;
}

// util_t::string_split =====================================================

void util_t::string_split_( std::vector<std::string>& results,
                            const std::string&        str,
                            const char*               delim,
                            bool                      allow_quotes )
{
  std::string buffer = str;
  std::string::size_type cut_pt, start = 0;

  std::string not_in_quote = delim;
  if ( allow_quotes )
    not_in_quote += '"';

  static const std::string in_quote = "\"";
  const std::string* search = &not_in_quote;

  while ( ( cut_pt = buffer.find_first_of( *search, start ) ) != buffer.npos )
  {
    if ( allow_quotes && ( buffer[ cut_pt ] == '"' ) )
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

  /*
    std::string buffer = str;
    std::string::size_type cut_pt;

    while ( ( cut_pt = buffer.find_first_of( delim ) ) != buffer.npos )
    {
      if ( cut_pt > 0 )
      {
        results.push_back( buffer.substr( 0, cut_pt ) );
      }
      buffer = buffer.substr( cut_pt + 1 );
    }
    if ( buffer.length() > 0 )
    {
      results.push_back( buffer );
    }
  */
}

// util_t::string_split =====================================================

int util_t::string_split( const std::string& str,
                          const char*        delim,
                          const char*        format, ... )
{
  std::vector<std::string>    str_splits;
  std::vector<std::string> format_splits;

  int    str_size = util_t::string_split(    str_splits, str,    delim );
  int format_size = util_t::string_split( format_splits, format, " "   );

  if ( str_size == format_size )
  {
    va_list vap;
    va_start( vap, format );

    for ( int i=0; i < str_size; i++ )
    {
      std::string& f = format_splits[ i ];
      const char*  s =    str_splits[ i ].c_str();

      if      ( f == "i" ) *( ( int* )         va_arg( vap, int*    ) ) = atoi( s );
      else if ( f == "f" ) *( ( double* )      va_arg( vap, double* ) ) = atof( s );
      else if ( f == "d" ) *( ( double* )      va_arg( vap, double* ) ) = atof( s );
      else if ( f == "S" ) *( ( std::string* ) va_arg( vap, std::string* ) ) = s;
      else assert( 0 );
    }

    va_end( vap );
  }

  return str_size;
}

// util_t::string_strip_quotes ==============================================

void util_t::string_strip_quotes( std::string& str )
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

// util_t::replace_all ======================================================

void util_t::replace_all_( std::string& s, const char* from, char to )
{
  std::string::size_type pos = s.find( from );
  if ( pos != s.npos )
  {
    std::size_t len = std::strlen( from );
    do
      s.replace( pos, len, 1, to );
    while ( ( pos = s.find( from, pos ) ) != s.npos );
  }
}

void util_t::replace_all_( std::string& s, char from, const char* to )
{
  std::string::size_type pos;
  if ( ( pos = s.find( from ) ) != s.npos )
  {
    std::size_t len = std::strlen( to );
    do
    {
      s.replace( pos, 1, to, len );
      pos += len;
    }
    while ( ( pos = s.find( from, pos ) ) != s.npos );
  }
}

// util_t::to_string ========================================================

std::string util_t::to_string( double f, int precision )
{
  std::ostringstream ss;
  ss << std::fixed << std::setprecision( precision ) << f;
  return ss.str();
}

// util_t::to_string ========================================================

std::string util_t::to_string( double f )
{
  if ( std::abs( f - static_cast<int>( f ) ) < 0.001 )
    return to_string( static_cast<int>( f ) );
  else
    return to_string( f, 3 );
}

// util_t::milliseconds =====================================================

int64_t util_t::milliseconds()
{
  return 1000 * clock() / CLOCKS_PER_SEC;
}

// util_t::parse_date =======================================================

int64_t util_t::parse_date( const std::string& month_day_year )
{
  std::vector<std::string> splits;
  size_t num_splits = util_t::string_split( splits, month_day_year, " _,;-/ \t\n\r" );
  if ( num_splits != 3 ) return 0;

  std::string& month = splits[ 0 ];
  std::string& day   = splits[ 1 ];
  std::string& year  = splits[ 2 ];

  util_t::tolower_( month );

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

// util_t::vfprintf_helper ==================================================

int util_t::vfprintf_helper( FILE *stream, const char *format, va_list args )
{
  std::string p_locale = setlocale( LC_CTYPE, NULL );
  setlocale( LC_CTYPE, "" );

  int retcode = ::vfprintf( stream, format, args );

  setlocale( LC_CTYPE, p_locale.c_str() );

  return retcode;
}

// util_t::fprintf ==========================================================

int util_t::fprintf( FILE *stream, const char *format,  ... )
{
  va_list fmtargs;
  va_start( fmtargs, format );

  int retcode = vfprintf_helper( stream, format, fmtargs );

  va_end( fmtargs );

  return retcode;
}

// util_t::printf ===========================================================

int util_t::printf( const char *format,  ... )
{
  va_list fmtargs;
  va_start( fmtargs, format );

  int retcode = vfprintf_helper( stdout, format, fmtargs );

  va_end( fmtargs );

  return retcode;
}

// util_t::snprintf =========================================================

int util_t::snprintf( char* buf, size_t size, const char* fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  int rval = ::vsnprintf( buf, size, fmt, ap );
  va_end( ap );
  if ( rval >= 0 )
    assert( static_cast<size_t>( rval ) < size );
  return rval;
}


// util_t::str_to_utf8_ =====================================================

void util_t::str_to_utf8_( std::string& str )
{
  std::string::iterator p = utf8::find_invalid( str.begin(), str.end() );
  if ( p == str.end() ) return;

  std::string temp( str.begin(), p );
  for ( std::string::iterator e = str.end(); p != e; ++p )
    utf8::append( static_cast<unsigned char>( *p ), std::back_inserter( temp ) );

  str.swap( temp );
}

// util_t::str_to_latin1_ ===================================================

void util_t::str_to_latin1_( std::string& str )
{
  if ( str.empty() ) return;
  if ( ! range::is_valid_utf8( str ) ) return;


  std::string temp;
  std::string::iterator i = str.begin(), e = str.end();

  while ( i != e )
    temp += ( unsigned char ) utf8::next( i, e );

  str.swap( temp );
}

// util_t::urlencode_ =======================================================

void util_t::urlencode_( std::string& str )
{
  std::string::size_type l = str.length();
  if ( ! l ) return;

  std::string temp;

  for ( std::string::size_type i = 0; i < l; ++i )
  {
    unsigned char c = str[ i ];

    if ( c > 0x7F || c == ' ' || c == '\'' )
    {
      char enc_str[4];
      snprintf( enc_str, sizeof( enc_str ), "%%%02X", c );
      temp += enc_str;
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

// util_t::urldecode_ =======================================================

void util_t::urldecode_( std::string& str )
{
  std::string::size_type l = str.length();
  if ( ! l ) return;

  std::string temp;

  for ( std::string::size_type i = 0; i < l; ++i )
  {
    unsigned char c = ( unsigned char ) str[ i ];

    if ( c == '%' && i + 2 < l )
    {
      long v = strtol( str.substr( i + 1, 2 ).c_str(), 0, 16 );
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

// util_t::format_text ======================================================

void util_t::format_text_( std::string& name, bool input_is_utf8 )
{
  if ( name.empty() ) return;
  bool is_utf8 = range::is_valid_utf8( name );

  if ( is_utf8 && ! input_is_utf8 )
    util_t::str_to_latin1( name );
  else if ( ! is_utf8 && input_is_utf8 )
    util_t::str_to_utf8( name );
}

// util_t::html_special_char_decode_ ========================================

void util_t::html_special_char_decode_( std::string& str )
{
  std::string::size_type pos = 0;

  while ( ( pos = str.find( "&", pos ) ) != std::string::npos )
  {
    if ( str[ pos+1 ] == '#' )
    {
      std::string::size_type end = str.find( ';', pos + 2 );
      char encoded = ( char ) atoi( str.substr( pos + 2, end ).c_str() );
      str.erase( pos, end - pos + 1 );
      str.insert( pos, 1, encoded );
    }
    else if ( 0 == str.compare( pos, 6, "&quot;" ) )
    {
      str.erase( pos, 6 );
      str.insert( pos, "\"" );
    }
    else if ( 0 == str.compare( pos, 5, "&amp;" ) )
    {
      str.erase( pos, 5 );
      str.insert( pos, "&" );
    }
    else if ( 0 == str.compare( pos, 4, "&lt;" ) )
    {
      str.erase( pos, 4 );
      str.insert( pos, "<" );
    }
    else if ( 0 == str.compare( pos, 4, "&gt;" ) )
    {
      str.erase( pos, 4 );
      str.insert( pos, ">" );
    }
  }
}

// util_t::floor ============================================================

double util_t::floor( double X, unsigned int decplaces )
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

// util_t::ceil =============================================================

double util_t::ceil( double X, unsigned int decplaces )
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

// util_t::round ============================================================

double util_t::round( double X, unsigned int decplaces )
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

// util_t::tolower_ =========================================================

void util_t::tolower_( std::string& str )
{
  // Transform all chars to lower case
  range::transform_self( str, ( int( * )( int ) ) std::tolower );
}

std::string tolower( const std::string& src )
{
  std::string dest;
  // Transform all chars to lowercase from src to dest
  range::transform( src, back_inserter( dest ), ( int( * )( int ) ) std::tolower );
  return dest;
}

std::string util_t::encode_html( const std::string& s )
{
  std::string buffer = std::string();
  buffer += s;
  util_t::replace_all( buffer, '&', "&amp;" );
  util_t::replace_all( buffer, '<', "&lt;" );
  util_t::replace_all( buffer, '>', "&gt;" );
  return buffer;
}


void util_t::armory_format( std::string& name, format_type_e f )
{
  std::string::size_type l = name.length();
  if ( ! l ) return;


  util_t::str_to_utf8( name );

  // remove leading '_' or '+'
  while ( ( name[ 0 ] == '_' || name[ 0 ] == '+' ) && !name.empty() )
  {
    name.erase( 0, 1 );
  }

  std::string buffer;
  l = name.length();
  for ( std::string::size_type i = 0; i < l; ++i )
  {
    unsigned char c = name[ i ];

    if ( c >= 0x80 )
    {
      continue;
    }
    else if ( std::isalpha( c ) )
    {
      if ( f == FORMAT_CHAR_NAME )
      {
        if ( i != 0 )
          c = std::tolower( c );
      }
      else
        c = std::tolower( c );
    }
    else if ( c == ' ' )
    {
       c = '_';
    }
    else if ( c != '_' &&
              c != '+' &&
              c != '.' &&
              c != '%' &&
              ! std::isdigit( c ) )
    {
      continue;
    }
    buffer += c;
  }
  name.swap( buffer );
}

bool util_t::is_number( const std::string& s )
{
  for ( std::string::size_type i = 0, l = s.length(); i < l; ++i )
    if ( ! std::isdigit( s[ i ] ) )
      return false;
  return true;
}

#if 0 // UNUSED
std::string trim( const std::string& src )
{
  std::string dest;

  std::string::size_type begin = src.find_first_not_of( ' ' );
  if ( begin != src.npos )
  {
    std::string::size_type end = src.find_last_not_of( ' ' );
    dest.assign( src, begin, end - begin );
  }

  return dest;
}

void replace_char( std::string& str, char old_c, char new_c  )
{
  for ( std::string::size_type i = 0, n = str.length(); i < n; ++i )
  {
    if ( str[ i ] == old_c )
      str[ i ] = new_c;
  }
}

void replace_str( std::string& src, const std::string& old_str, const std::string& new_str  )
{
  if ( old_str.empty() ) return;

  std::string::size_type p = 0;
  while ( ( p = src.find( old_str, p ) ) != std::string::npos )
  {
    src.replace( p, p + old_str.length(), new_str );
    p += new_str.length();
  }
}

bool str_to_float( std::string src, double& dest )
{
  bool was_sign=false;
  bool was_digit=false;
  bool was_dot=false;
  bool res=true;
  //check each char
  for ( std::size_t p=0; res&&( p<src.length() ); p++ )
  {
    char ch=src[p];
    if ( ch==' ' ) continue;
    if ( ( ( ch=='+' )||( ch=='-' ) )&& !( was_sign||was_digit ) )
    {
      was_sign=true;
      continue;
    }
    if ( ( ch=='.' )&& was_digit && !was_dot )
    {
      was_dot=true;
      continue;
    }
    if ( ( ch>='0' )&&( ch<='9' ) )
    {
      was_digit=true;
      continue;
    }
    //if none of above, fail
    res=false;
  }
  //check if we had at least one digit
  if ( !was_digit ) res=false;
  //return result
  dest=atof( src.c_str() );
  return res;
}
#endif

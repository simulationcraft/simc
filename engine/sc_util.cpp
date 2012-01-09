// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"
#include "utf8.h"

namespace { // ANONYMOUS ====================================================

// pred_ci ==================================================================

bool pred_ci ( char a, char b )
{
  return std::tolower( a ) == std::tolower( b );
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

// util_t::dup ==============================================================

char* util_t::dup( const char *value )
{
  std::size_t n = strlen( value ) + 1;
  void *p = malloc( n );
  if ( p ) memcpy( p, value, n );
  return static_cast<char*>( p );
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

// util_t::role_type_string =================================================

const char* util_t::role_type_string( int role )
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
  }
  return "unknown";
}

// util_t::parse_role_type ==================================================

role_type util_t::parse_role_type( const std::string& name )
{
  for ( int i=( int ) ROLE_NONE; i < ( int )ROLE_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::role_type_string( i ) ) )
      return ( role_type ) i;

  return ROLE_HYBRID;
}

// util_t::race_type_string =================================================

const char* util_t::race_type_string( int type )
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
  }
  return "unknown";
}

// util_t::parse_race_type ==================================================

race_type util_t::parse_race_type( const std::string& name )
{
  for ( int i=( int ) RACE_NONE; i < ( int )RACE_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::race_type_string( i ) ) )
      return ( race_type ) i;

  return RACE_NONE;
}

// util_t::parse_position_type ==============================================

position_type util_t::parse_position_type( const std::string& name )
{
  for ( int i = ( int ) POSITION_NONE; i < ( int ) POSITION_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::position_type_string( i ) ) )
      return ( position_type ) i;

  return POSITION_NONE;
}

// util_t::position_type_string =============================================

const char* util_t::position_type_string( int type )
{
  switch ( type )
  {
  case POSITION_NONE:         return "none";
  case POSITION_BACK:         return "back";
  case POSITION_FRONT:        return "front";
  case POSITION_RANGED_BACK:  return "ranged_back";
  case POSITION_RANGED_FRONT: return "ranged_front";
  }

  return "unknown";
}

// util_t::profession_type_string ===========================================

const char* util_t::profession_type_string( int type )
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
  }
  return "unknown";
}

// util_t::parse_profession_type ============================================

int util_t::parse_profession_type( const std::string& name )
{
  for ( int i=0; i < PROFESSION_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::profession_type_string( i ) ) )
      return i;

  return PROFESSION_NONE;
}

// util_t::translate_profession_id ==========================================

profession_type util_t::translate_profession_id( int skill_id )
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

const char* util_t::player_type_string( int type )
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
  }
  return "unknown";
}

// util_t::translate_class_str ==============================================

int util_t::translate_class_str( std::string& s )
{
  std::string fmt_s = armory_t::format( s );
  if      ( fmt_s == "death_knight" ) return DEATH_KNIGHT;
  else if ( fmt_s == "deathknight"  ) return DEATH_KNIGHT;
  else if ( fmt_s == "druid"        ) return DRUID;
  else if ( fmt_s == "hunter"       ) return HUNTER;
  else if ( fmt_s == "mage"         ) return MAGE;
  else if ( fmt_s == "monk"         ) return MONK;
  else if ( fmt_s == "paladin"      ) return PALADIN;
  else if ( fmt_s == "priest"       ) return PRIEST;
  else if ( fmt_s == "rogue"        ) return ROGUE;
  else if ( fmt_s == "shaman"       ) return SHAMAN;
  else if ( fmt_s == "warlock"      ) return WARLOCK;
  else if ( fmt_s == "warrior"      ) return WARRIOR;
  return PLAYER_NONE;
}

// util_t::parse_player_type ================================================

player_type util_t::parse_player_type( const std::string& name )
{
  for ( int i=( int ) PLAYER_NONE; i < ( int ) PLAYER_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::player_type_string( i ) ) )
      return ( player_type ) i;

  return PLAYER_NONE;
}

// util_t::pet_type_string ==================================================

const char* util_t::pet_type_string( int type )
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
  }
  return "unknown";
}

// util_t::parse_pet_type ===================================================

pet_type_t util_t::parse_pet_type( const std::string& name )
{
  for ( int i=( int )PET_NONE; i < ( int )PET_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::pet_type_string( i ) ) )
      return ( pet_type_t )i;

  return PET_NONE;
}

// util_t::attribute_type_string ============================================

const char* util_t::attribute_type_string( int type )
{
  switch ( type )
  {
  case ATTR_STRENGTH:  return "strength";
  case ATTR_AGILITY:   return "agility";
  case ATTR_STAMINA:   return "stamina";
  case ATTR_INTELLECT: return "intellect";
  case ATTR_SPIRIT:    return "spirit";
  }
  return "unknown";
}

// util_t::parse_attribute_type =============================================

int util_t::parse_attribute_type( const std::string& name )
{
  for ( int i=0; i < ATTRIBUTE_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::attribute_type_string( i ) ) )
      return i;

  return ATTRIBUTE_NONE;
}

// util_t::dmg_type_string ==================================================

const char* util_t::dmg_type_string( int type )
{
  switch ( type )
  {
  case DMG_DIRECT:    return "hit";
  case DMG_OVER_TIME: return "tick";
  }
  return "unknown";
}

// util_t::gem_type_string ==================================================

const char* util_t::gem_type_string( int type )
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
  }
  return "unknown";
}

// util_t::parse_gem_type ===================================================

int util_t::parse_gem_type( const std::string& name )
{
  for ( int i=0; i < GEM_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::gem_type_string( i ) ) )
      return i;

  return GEM_NONE;
}

// util_t::meta_gem_type_string =============================================

const char* util_t::meta_gem_type_string( int type )
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
  case META_EFFULGENT_SHADOWSPIRIT:     return "effulgent_shadowspirit";
  case META_EMBER_SHADOWSPIRIT:         return "ember_shadowspirit";
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
  case META_TRENCHANT_EARTHSHATTER:     return "trenchant_earthshatter";
  case META_TRENCHANT_EARTHSIEGE:       return "trenchant_earthsiege";
  }
  return "unknown";
}

// util_t::parse_meta_gem_type ==============================================

int util_t::parse_meta_gem_type( const std::string& name )
{
  for ( int i=0; i < META_GEM_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::meta_gem_type_string( i ) ) )
      return i;

  return META_GEM_NONE;
}

// util_t::result_type_string ===============================================

const char* util_t::result_type_string( int type )
{
  switch ( type )
  {
  case RESULT_NONE:   return "none";
  case RESULT_MISS:   return "miss";
  case RESULT_RESIST: return "resist";
  case RESULT_DODGE:  return "dodge";
  case RESULT_PARRY:  return "parry";
  case RESULT_BLOCK:  return "block";
  case RESULT_GLANCE: return "glance";
  case RESULT_CRIT:   return "crit";
  case RESULT_CRIT_BLOCK: return "crit-block";
  case RESULT_HIT:    return "hit";
  }
  return "unknown";
}

// util_t::parse_result_type ================================================

int util_t::parse_result_type( const std::string& name )
{
  for ( int i=0; i < RESULT_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::result_type_string( i ) ) )
      return i;

  return RESULT_NONE;
}

// util_t::resource_type_string =============================================

const char* util_t::resource_type_string( int type )
{
  switch ( type )
  {
  case RESOURCE_NONE:         return "none";
  case RESOURCE_HEALTH:       return "health";
  case RESOURCE_MANA:         return "mana";
  case RESOURCE_RAGE:         return "rage";
  case RESOURCE_ENERGY:       return "energy";
  case RESOURCE_FOCUS:        return "focus";
  case RESOURCE_RUNIC:        return "runic_power";
  case RESOURCE_RUNE:         return "all_runes";
  case RESOURCE_RUNE_BLOOD:   return "blood_runes";
  case RESOURCE_RUNE_UNHOLY:  return "unholy_runes";
  case RESOURCE_RUNE_FROST:   return "frost_runes";
  case RESOURCE_SOUL_SHARDS:  return "soul_shards";
  case RESOURCE_HOLY_POWER:   return "holy_power";
  case RESOURCE_CHI:          return "chi";
  case RESOURCE_LIGHT_FORCE:  return "light_force";
  case RESOURCE_DARK_FORCE:   return "dark_Force";
  }
  return "unknown";
}

// util_t::parse_resource_type ==============================================

int util_t::parse_resource_type( const std::string& name )
{
  for ( int i=0; i < RESOURCE_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::resource_type_string( i ) ) )
      return i;

  return RESOURCE_NONE;
}

// util_t::school_type_component ============================================

int util_t::school_type_component( int s_type, int c_type )
{
  uint32_t s_mask = spell_id_t::get_school_mask( ( school_type ) s_type );
  uint32_t c_mask = spell_id_t::get_school_mask( ( school_type ) c_type );

  return s_mask & c_mask;
}

// util_t::school_type_string ===============================================

const char* util_t::school_type_string( int school )
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
  }
  return "unknown";
}

// util_t::parse_school_type ================================================

school_type util_t::parse_school_type( const std::string& name )
{
  for ( int i=SCHOOL_NONE; i < SCHOOL_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::school_type_string( i ) ) )
      return ( school_type )( i );

  return SCHOOL_NONE;
}

// util_t::talent_tree ======================================================

int util_t::talent_tree( int tree, player_type ptype )
{
  switch ( ptype )
  {
  case DEATH_KNIGHT:
    switch ( tree )
    {
    case DEATH_KNIGHT_BLOOD:    return TREE_BLOOD;
    case DEATH_KNIGHT_FROST:    return TREE_FROST;
    case DEATH_KNIGHT_UNHOLY:   return TREE_UNHOLY;
    }
  case DRUID:
    switch ( tree )
    {
    case DRUID_BALANCE:         return TREE_BALANCE;
    case DRUID_FERAL:           return TREE_FERAL;
    case DRUID_RESTORATION:     return TREE_RESTORATION;
    }
  case HUNTER:
    switch ( tree )
    {
    case HUNTER_BEAST_MASTERY:  return TREE_BEAST_MASTERY;
    case HUNTER_MARKSMANSHIP:   return TREE_MARKSMANSHIP;
    case HUNTER_SURVIVAL:       return TREE_SURVIVAL;
    }
  case MAGE:
    switch ( tree )
    {
    case MAGE_ARCANE:           return TREE_ARCANE;
    case MAGE_FIRE:             return TREE_FIRE;
    case MAGE_FROST:            return TREE_FROST;
    }
  case PALADIN:
    switch ( tree )
    {
    case PALADIN_HOLY:          return TREE_HOLY;
    case PALADIN_PROTECTION:    return TREE_PROTECTION;
    case PALADIN_RETRIBUTION:   return TREE_RETRIBUTION;
    }
  case PRIEST:
    switch ( tree )
    {
    case PRIEST_DISCIPLINE:     return TREE_DISCIPLINE;
    case PRIEST_HOLY:           return TREE_HOLY;
    case PRIEST_SHADOW:         return TREE_SHADOW;
    }
  case ROGUE:
    switch ( tree )
    {
    case ROGUE_ASSASSINATION:   return TREE_ASSASSINATION;
    case ROGUE_COMBAT:          return TREE_COMBAT;
    case ROGUE_SUBTLETY:        return TREE_SUBTLETY;
    }
  case SHAMAN:
    switch ( tree )
    {
    case SHAMAN_ELEMENTAL:      return TREE_ELEMENTAL;
    case SHAMAN_ENHANCEMENT:    return TREE_ENHANCEMENT;
    case SHAMAN_RESTORATION:    return TREE_RESTORATION;
    }
  case WARLOCK:
    switch ( tree )
    {
    case WARLOCK_AFFLICTION:    return TREE_AFFLICTION;
    case WARLOCK_DEMONOLOGY:    return TREE_DEMONOLOGY;
    case WARLOCK_DESTRUCTION:   return TREE_DESTRUCTION;
    }
  case WARRIOR:
    switch ( tree )
    {
    case WARRIOR_ARMS:          return TREE_ARMS;
    case WARRIOR_FURY:          return TREE_FURY;
    case WARRIOR_PROTECTION:    return TREE_PROTECTION;
    }
  default:
    break;
  }
  return TREE_NONE;
}

// util_t::talent_tree_string ===============================================

const char* util_t::talent_tree_string( int tree, bool armory_format )
{
  if ( armory_format )
  {
    switch ( tree )
    {
    case TREE_BLOOD:         return "blood";
    case TREE_UNHOLY:        return "unholy";
    case TREE_BALANCE:       return "balance";
    case TREE_FERAL:         return "feral";
    case TREE_RESTORATION:   return "restoration";
    case TREE_BEAST_MASTERY: return "beast_mastery";
    case TREE_MARKSMANSHIP:  return "marksmanship";
    case TREE_SURVIVAL:      return "survival";
    case TREE_ARCANE:        return "arcane";
    case TREE_FIRE:          return "fire";
    case TREE_FROST:         return "frost";
    case TREE_RETRIBUTION:   return "retribution";
    case TREE_DISCIPLINE:    return "discipline";
    case TREE_HOLY:          return "holy";
    case TREE_SHADOW:        return "shadow";
    case TREE_ASSASSINATION: return "assassination";
    case TREE_COMBAT:        return "combat";
    case TREE_SUBTLETY:      return "subtlety";
    case TREE_ELEMENTAL:     return "elemental";
    case TREE_ENHANCEMENT:   return "enhancement";
    case TREE_AFFLICTION:    return "affliction";
    case TREE_DEMONOLOGY:    return "demonology";
    case TREE_DESTRUCTION:   return "destruction";
    case TREE_ARMS:          return "arms";
    case TREE_FURY:          return "fury";
    case TREE_PROTECTION:    return "protection";
    default: return "Unknown";
    }
  }
  else
  {
    switch ( tree )
    {
    case TREE_BLOOD:         return "Blood";
    case TREE_UNHOLY:        return "Unholy";
    case TREE_BALANCE:       return "Balance";
    case TREE_FERAL:         return "Feral";
    case TREE_RESTORATION:   return "Restoration";
    case TREE_BEAST_MASTERY: return "Beast Mastery";
    case TREE_MARKSMANSHIP:  return "Marksmanship";
    case TREE_SURVIVAL:      return "Survival";
    case TREE_ARCANE:        return "Arcane";
    case TREE_FIRE:          return "Fire";
    case TREE_FROST:         return "Frost";
    case TREE_RETRIBUTION:   return "Retribution";
    case TREE_DISCIPLINE:    return "Discipline";
    case TREE_HOLY:          return "Holy";
    case TREE_SHADOW:        return "Shadow";
    case TREE_ASSASSINATION: return "Assassination";
    case TREE_COMBAT:        return "Combat";
    case TREE_SUBTLETY:      return "Subtlety";
    case TREE_ELEMENTAL:     return "Elemental";
    case TREE_ENHANCEMENT:   return "Enhancement";
    case TREE_AFFLICTION:    return "Affliction";
    case TREE_DEMONOLOGY:    return "Demonology";
    case TREE_DESTRUCTION:   return "Destruction";
    case TREE_ARMS:          return "Arms";
    case TREE_FURY:          return "Fury";
    case TREE_PROTECTION:    return "Protection";
    default: return "Unknown";
    }
  }
}

// util_t::parse_talent_tree ================================================

int util_t::parse_talent_tree( const std::string& name )
{
  for ( int i=0; i < TALENT_TREE_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::talent_tree_string( i ) ) )
      return i;

  return TREE_NONE;
}

// util_t::weapon_type_string ===============================================

const char* util_t::weapon_type_string( int weapon )
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
  }
  return "unknown";
}

// util_t::weapon_subclass_string ===========================================

const char* util_t::weapon_subclass_string( int subclass )
{
  switch ( subclass )
  {
  case ITEM_SUBCLASS_WEAPON_AXE:      return "Axe";
  case ITEM_SUBCLASS_WEAPON_AXE2:     return "Axe";
  case ITEM_SUBCLASS_WEAPON_BOW:      return "Bow";
  case ITEM_SUBCLASS_WEAPON_GUN:      return "Gun";
  case ITEM_SUBCLASS_WEAPON_MACE:     return "Mace";
  case ITEM_SUBCLASS_WEAPON_MACE2:    return "Mace";
  case ITEM_SUBCLASS_WEAPON_POLEARM:  return "Polearm";
  case ITEM_SUBCLASS_WEAPON_SWORD:    return "Sword";
  case ITEM_SUBCLASS_WEAPON_SWORD2:   return "Sword";
  case ITEM_SUBCLASS_WEAPON_STAFF:    return "Staff";
  case ITEM_SUBCLASS_WEAPON_FIST:     return "Fist Weapon";
  case ITEM_SUBCLASS_WEAPON_DAGGER:   return "Dagger";
  case ITEM_SUBCLASS_WEAPON_THROWN:   return "Thrown";
  case ITEM_SUBCLASS_WEAPON_CROSSBOW: return "Crossbow";
  case ITEM_SUBCLASS_WEAPON_WAND:     return "Wand";
  }
  return "Unknown";
}

// util_t::weapon_class_string ==============================================

const char* util_t::weapon_class_string( int class_ )
{
  switch ( class_ )
  {
  case INVTYPE_WEAPON:
    return "One Hand";
  case INVTYPE_2HWEAPON:
    return "Two-Hand";
  case INVTYPE_WEAPONMAINHAND:
    return "Main Hand";
  case INVTYPE_WEAPONOFFHAND:
    return "Off Hand";
  }
  return 0;
}

// util_t::set_item_type_string =============================================

const char* util_t::set_item_type_string( int item_set )
{
  switch ( item_set )
  {
    // Melee sets
  case 925:   // Death Knight T11
  case 927:   // Druid T11
  case 932:   // Paladin T11
  case 939:   // Shaman T11
  case 942:   // Warrior T11
  case 1000:  // Death Knight T12
  case 1002:  // Druid T12
  case 1011:  // Paladin T12
  case 1015:  // Shaman T12
  case 1017:  // Warrior T12
    return "Melee";

    // Tank sets
  case 926:   // Death Knight T11
  case 934:   // Paladin T11
  case 943:   // Warrior T11
  case 1001:  // Death Knight T12
  case 1013:  // Paladin T12
  case 1018:  // Warrior T12
    return "Tank";

    // Healer sets
  case 928:   // Druid T11
  case 933:   // Paladin T11
  case 935:   // Priest T11
  case 938:   // Shaman T11
  case 1003:  // Druid T12
  case 1010:  // Priest T12
  case 1012:  // Paladin T12
  case 1014:  // Shaman T12
    return "Healer";

    // DPS Caster sets
  case 929:   // Druid T11
  case 936:   // Priest T11
  case 940:   // Shaman T11
  case 1004:  // Druid T12
  case 1009:  // Priest T12
  case 1016:  // Shaman T12
    return "Caster";
  }
  return 0;
}

// util_t::parse_weapon_type ================================================

int util_t::parse_weapon_type( const std::string& name )
{
  for ( int i=0; i < WEAPON_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::weapon_type_string( i ) ) )
      return i;

  return WEAPON_NONE;
}

// util_t::flask_type_string ================================================

const char* util_t::flask_type_string( int flask )
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
  }
  return "unknown";
}

// util_t::parse_flask_type =================================================

int util_t::parse_flask_type( const std::string& name )
{
  for ( int i=0; i < FLASK_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::flask_type_string( i ) ) )
      return i;

  return FLASK_NONE;
}

// util_t::food_type_string =================================================

const char* util_t::food_type_string( int food )
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
  }
  return "unknown";
}

// util_t::parse_food_type ==================================================

int util_t::parse_food_type( const std::string& name )
{
  for ( int i=0; i < FOOD_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::food_type_string( i ) ) )
      return i;

  return FOOD_NONE;
}

// util_t::set_bonus_string =================================================

const char* util_t::set_bonus_string( set_type type )
{
  switch ( type )
  {
  case SET_T11_2PC_CASTER: return "tier11_2pc_caster";
  case SET_T11_4PC_CASTER: return "tier11_4pc_caster";
  case SET_T11_2PC_MELEE:  return "tier11_2pc_melee";
  case SET_T11_4PC_MELEE:  return "tier11_4pc_melee";
  case SET_T11_2PC_TANK:   return "tier11_2pc_tank";
  case SET_T11_4PC_TANK:   return "tier11_4pc_tank";
  case SET_T12_2PC_CASTER: return "tier12_2pc_caster";
  case SET_T12_4PC_CASTER: return "tier12_4pc_caster";
  case SET_T12_2PC_MELEE:  return "tier12_2pc_melee";
  case SET_T12_4PC_MELEE:  return "tier12_4pc_melee";
  case SET_T12_2PC_TANK:   return "tier12_2pc_tank";
  case SET_T12_4PC_TANK:   return "tier12_4pc_tank";
  case SET_T13_2PC_CASTER: return "tier13_2pc_caster";
  case SET_T13_4PC_CASTER: return "tier13_4pc_caster";
  case SET_T13_2PC_MELEE:  return "tier13_2pc_melee";
  case SET_T13_4PC_MELEE:  return "tier13_4pc_melee";
  case SET_T13_2PC_TANK:   return "tier13_2pc_tank";
  case SET_T13_4PC_TANK:   return "tier13_4pc_tank";
  case SET_T14_2PC_CASTER: return "tier14_2pc_caster";
  case SET_T14_4PC_CASTER: return "tier14_4pc_caster";
  case SET_T14_2PC_MELEE:  return "tier14_2pc_melee";
  case SET_T14_4PC_MELEE:  return "tier14_4pc_melee";
  case SET_T14_2PC_TANK:   return "tier14_2pc_tank";
  case SET_T14_4PC_TANK:   return "tier14_4pc_tank";
  case SET_PVP_2PC_CASTER: return "pvp_2pc_caster";
  case SET_PVP_4PC_CASTER: return "pvp_4pc_caster";
  case SET_PVP_2PC_MELEE:  return "pvp_2pc_melee";
  case SET_PVP_4PC_MELEE:  return "pvp_4pc_melee";
  case SET_PVP_2PC_TANK:   return "pvp_2pc_tank";
  case SET_PVP_4PC_TANK:   return "pvp_4pc_tank";
  default:
    break;
  }
  return "unknown";
}

// util_t::parse_set_bonus ==================================================

set_type util_t::parse_set_bonus( const std::string& name )
{
  for ( int i=0; i < SET_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::set_bonus_string( ( set_type ) i ) ) )
      return ( set_type ) i;

  return SET_NONE;
}

// util_t::slot_type_string =================================================

const char* util_t::slot_type_string( int slot )
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
  }
  return "unknown";
}

// util_t::armor_type_string ================================================

const char* util_t::armor_type_string( player_type ptype, int slot_type )
{
  bool has_armor_type = false;

  switch ( slot_type )
  {
  case SLOT_HEAD:
  case SLOT_SHOULDERS:
  case SLOT_CHEST:
  case SLOT_WAIST:
  case SLOT_LEGS:
  case SLOT_FEET:
  case SLOT_WRISTS:
  case SLOT_HANDS:
    has_armor_type = true;
    break;
  default:
    has_armor_type = false;
    break;
  }

  switch ( ptype )
  {
  case WARRIOR:
  case PALADIN:
  case DEATH_KNIGHT:
    return ( has_armor_type ? "plate" : NULL );
  case HUNTER:
  case SHAMAN:
    return ( has_armor_type ? "mail" : NULL );
  case DRUID:
  case ROGUE:
  case MONK:
    return ( has_armor_type ? "leather" : NULL );
  case MAGE:
  case PRIEST:
  case WARLOCK:
    return ( has_armor_type ? "cloth" : NULL );
  default:
    return NULL;
  }
}

// util_t::parse_slot_type ==================================================

int util_t::parse_slot_type( const std::string& name )
{
  for ( int i=0; i < SLOT_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::slot_type_string( i ) ) )
      return i;

  return SLOT_NONE;
}

// util_t::stat_type_string =================================================

const char* util_t::stat_type_string( int stat )
{
  switch ( stat )
  {
  case STAT_STRENGTH:  return "strength";
  case STAT_AGILITY:   return "agility";
  case STAT_STAMINA:   return "stamina";
  case STAT_INTELLECT: return "intellect";
  case STAT_SPIRIT:    return "spirit";

  case STAT_HEALTH: return "health";
  case STAT_MANA:   return "mana";
  case STAT_MAX_MANA: return "maximum_mana";
  case STAT_RAGE:   return "rage";
  case STAT_ENERGY: return "energy";
  case STAT_FOCUS:  return "focus";
  case STAT_RUNIC:  return "runic";

  case STAT_SPELL_POWER:       return "spell_power";
  case STAT_SPELL_PENETRATION: return "spell_penetration";
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

  case STAT_MAX: return "all";
  }
  return "unknown";
}

// util_t::stat_type_abbrev =================================================

const char* util_t::stat_type_abbrev( int stat )
{
  switch ( stat )
  {
  case STAT_STRENGTH:  return "Str";
  case STAT_AGILITY:   return "Agi";
  case STAT_STAMINA:   return "Sta";
  case STAT_INTELLECT: return "Int";
  case STAT_SPIRIT:    return "Spi";

  case STAT_HEALTH: return "Health";
  case STAT_MANA:   return "Mana";
  case STAT_RAGE:   return "Rage";
  case STAT_ENERGY: return "Energy";
  case STAT_FOCUS:  return "Focus";
  case STAT_RUNIC:  return "Runic";

  case STAT_SPELL_POWER:       return "SP";
  case STAT_SPELL_PENETRATION: return "SPen";
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

  case STAT_MAX: return "All";
  }
  return "unknown";
}

// util_t::stat_type_wowhead ================================================

const char* util_t::stat_type_wowhead( int stat )
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
  case STAT_SPELL_PENETRATION: return "spellPen";

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
  }
  return "unknown";
}

// util_t::parse_stat_type ==================================================

stat_type util_t::parse_stat_type( const std::string& name )
{
  for ( int i=0; i <= STAT_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::stat_type_string( i ) ) )
      return ( stat_type ) i;

  for ( int i=0; i <= STAT_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::stat_type_abbrev( i ) ) )
      return ( stat_type ) i;

  for ( int i=0; i <= STAT_MAX; i++ )
    if ( util_t::str_compare_ci( name, util_t::stat_type_wowhead( i ) ) )
      return ( stat_type ) i;

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
  if ( name == "splpen"         ) return STAT_SPELL_PENETRATION;
  if ( name == "spi"            ) return STAT_SPIRIT;
  if ( util_t::str_compare_ci( name, "__wpds"   ) ) return STAT_WEAPON_DPS;
  if ( util_t::str_compare_ci( name, "__wspeed" ) ) return STAT_WEAPON_SPEED;

  return STAT_NONE;
}

// util_t::parse_reforge_type ===============================================

stat_type util_t::parse_reforge_type( const std::string& name )
{
  stat_type s = util_t::parse_stat_type( name );

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
  int num_tokens = util_t::string_split( tokens, origin_str, "/:.?&=" );

  for ( int i=0; i < num_tokens; i++ )
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

int util_t::class_id_mask( int type )
{
  int cid = class_id( type );
  if ( cid <= 0 ) return 0;
  return 1 << ( cid - 1 );
}

// util_t::class_id =========================================================

int util_t::class_id( int type )
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
  case DRUID:        return 11;
  case PLAYER_SPECIAL_SCALE: return 12;
  }

  return 0;
}

// util_t::race_id ==========================================================

unsigned util_t::race_id( int r )
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
  default: break;
  }
  return 0;
}

// util_t::race_mask ========================================================

unsigned util_t::race_mask( int r )
{
  uint32_t id = race_id( r );

  if ( id > 0 )
    return ( 1 << ( id - 1 ) );

  return 0x00;
}

// util_t::pet_class_type ===================================================

player_type util_t::pet_class_type( int c )
{
  player_type p = WARRIOR;

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

unsigned util_t::pet_mask( int p )
{
  if ( p <= PET_FEROCITY )
    return 0x1;
  if ( p <= PET_TENACITY )
    return 0x2;
  if ( p <= PET_CUNNING )
    return 0x4;

  return 0x0;
}

// util_t::pet_id ===========================================================

unsigned util_t::pet_id( int p )
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

const char* util_t::class_id_string( int type )
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
  case DRUID:        return "11";
  }

  return "0";
}

// util_t::translate_class_id ===============================================

int util_t::translate_class_id( int cid )
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
  case 11: return DRUID;
  }

  return PLAYER_NONE;
}

// util_t::translate_race_id ================================================

race_type util_t::translate_race_id( int rid )
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
  }

  return RACE_NONE;
}

// util_t::translate_item_mod ===============================================

stat_type util_t::translate_item_mod( int item_mod )
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
  }

  return STAT_NONE;
}

// util_t::translate_weapon_subclass ========================================

weapon_type util_t::translate_weapon_subclass( item_subclass_weapon id )
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

slot_type util_t::translate_invtype( int inv_type )
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
  }

  return SLOT_NONE;
}

// util_t::socket_gem_match =================================================

bool util_t::socket_gem_match( int socket,
                               int gem )
{
  if ( socket == GEM_NONE || gem == GEM_PRISMATIC ) return true;

  if ( socket == GEM_COGWHEEL && gem == GEM_COGWHEEL ) return true;

  if ( socket == GEM_META ) return ( gem == GEM_META );

  if ( socket == GEM_RED    ) return ( gem == GEM_RED    || gem == GEM_ORANGE || gem == GEM_PURPLE );
  if ( socket == GEM_YELLOW ) return ( gem == GEM_YELLOW || gem == GEM_ORANGE || gem == GEM_GREEN  );
  if ( socket == GEM_BLUE   ) return ( gem == GEM_BLUE   || gem == GEM_PURPLE || gem == GEM_GREEN  );

  return false;
}

// util_t::item_quality_string ==============================================

const char* util_t::item_quality_string( int quality )
{
  switch( quality )
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
  char buffer[ 64 ];
  snprintf( buffer, sizeof( buffer ), "%.*f", precision, f );
  return std::string( buffer );
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
  int num_splits = util_t::string_split( splits, month_day_year, " _,;-/ \t\n\r" );
  if ( num_splits != 3 ) return 0;

  std::string& month = splits[ 0 ];
  std::string& day   = splits[ 1 ];
  std::string& year  = splits[ 2 ];

  std::transform( month.begin(), month.end(), month.begin(), ( int( * )( int ) ) std::tolower );

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
  char *p_locale = util_t::dup( setlocale( LC_CTYPE, NULL ) );
  setlocale( LC_CTYPE, "" );

  int retcode = ::vfprintf( stream, format, args );

  setlocale( LC_CTYPE, p_locale );
  free( p_locale );

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
  if ( ! utf8::is_valid( str.begin(), str.end() ) ) return;


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
  bool is_utf8 = utf8::is_valid( name.begin(), name.end() );

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
    double div = SC_EPSILON;
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
    double div = SC_EPSILON;
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
    double div = SC_EPSILON;
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
  for ( std::string::size_type i = 0, n = str.length(); i < n; ++i )
    str[i] = ::tolower( str[i] );
}

//-------------------------------
// std::STRING   utils
//-------------------------------

// utility functions
std::string tolower( const std::string& src )
{
  std::string dest;
  for ( std::string::size_type i = 0, n = src.length(); i < n; ++i )
    dest.push_back( tolower( src[ i ] ) );
  return dest;
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

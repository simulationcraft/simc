// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

bool my_isdigit( char c );

// str_compare_ci ==========================================================

static bool str_compare_ci( const std::string& l,
			    const std::string& r )
{
  int l_size = ( int ) l.size();
  int r_size = ( int ) r.size();

  if( l_size != r_size ) return false;

  if( l_size == 0 ) return true;

  for( int i=0; i < l_size; i++ )
    if( tolower( l[ i ] ) != tolower( r[ i ] ) )
      return false;

  return true;
}

// util_t::talent_rank =====================================================

double util_t::talent_rank( int    num,
                            int    max,
                            double increment )
{
  assert( num >= 0 );
  assert( max > 0 );

  if ( num > max ) num = max;

  return num * increment;
}

// util_t::talent_rank =====================================================

int util_t::talent_rank( int num,
                         int max,
                         int increment )
{
  assert( num >= 0 );
  assert( max > 0 );

  if ( num > max ) num = max;

  return num * increment;
}

// util_t::talent_rank =====================================================

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

// util_t::talent_rank =====================================================

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

// util_t::ability_rank ====================================================

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

// util_t::ability_rank ====================================================

int util_t::ability_rank( int player_level,
                          int ability_value,
                          int ability_level, ... )
{
  va_list vap;
  va_start( vap, ability_level );

  while ( player_level < ability_level )
  {
    ability_value = ( int )    va_arg( vap, int );
    ability_level = ( int ) va_arg( vap, int );
  }

  va_end( vap );

  return ability_value;
}

// util_t::dup =============================================================

char* util_t::dup( const char *value )
{
  return strcpy( ( char* ) malloc( strlen( value ) + 1 ), value );
}

// util_t::race_type_string ================================================

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
  }
  return "unknown";
}

// util_t::parse_race_type =================================================

int util_t::parse_race_type( const std::string& name )
{
  for ( int i=0; i < RACE_MAX; i++ )
    if ( str_compare_ci( name, util_t::race_type_string( i ) ) )
      return i;

  return RACE_NONE;
}

// util_t::profession_type_string ==========================================

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

// util_t::parse_profession_type ===========================================

int util_t::parse_profession_type( const std::string& name )
{
  for ( int i=0; i < PROFESSION_MAX; i++ )
    if ( str_compare_ci( name, util_t::profession_type_string( i ) ) )
      return i;

  return PROFESSION_NONE;
}

// util_t::player_type_string ==============================================

const char* util_t::player_type_string( int type )
{
  switch ( type )
  {
  case PLAYER_NONE:     return "none";
  case DEATH_KNIGHT:    return "death_knight";
  case DRUID:           return "druid";
  case HUNTER:          return "hunter";
  case MAGE:            return "mage";
  case PALADIN:         return "paladin";
  case PRIEST:          return "priest";
  case ROGUE:           return "rogue";
  case SHAMAN:          return "shaman";
  case WARLOCK:         return "warlock";
  case WARRIOR:         return "warrior";
  case PLAYER_PET:      return "pet";
  case PLAYER_GUARDIAN: return "guardian";
  }
  return "unknown";
}

// util_t::parse_player_type ===============================================

int util_t::parse_player_type( const std::string& name )
{
  for ( int i=0; i < PLAYER_MAX; i++ )
    if ( str_compare_ci( name, util_t::player_type_string( i ) ) )
      return i;

  return PLAYER_NONE;
}

// util_t::attribute_type_string ===========================================

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

// util_t::parse_attribute_type ============================================

int util_t::parse_attribute_type( const std::string& name )
{
  for ( int i=0; i < ATTRIBUTE_MAX; i++ )
    if ( str_compare_ci( name, util_t::attribute_type_string( i ) ) )
      return i;

  return ATTRIBUTE_NONE;
}

// util_t::dmg_type_string =================================================

const char* util_t::dmg_type_string( int type )
{
  switch ( type )
  {
  case DMG_DIRECT:    return "hit";
  case DMG_OVER_TIME: return "tick";
  }
  return "unknown";
}

// util_t::gem_type_string =================================================

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
  }
  return "unknown";
}

// util_t::parse_gem_type ==================================================

int util_t::parse_gem_type( const std::string& name )
{
  for ( int i=0; i < GEM_MAX; i++ )
    if ( str_compare_ci( name, util_t::gem_type_string( i ) ) )
      return i;

  return GEM_NONE;
}

// util_t::meta_gem_type_string ============================================

const char* util_t::meta_gem_type_string( int type )
{
  switch ( type )
  {
  case META_AUSTERE_EARTHSIEGE:      return "austere_earthsiege";
  case META_BEAMING_EARTHSIEGE:      return "beaming_earthsiege";
  case META_BRACING_EARTHSIEGE:      return "bracing_earthsiege";
  case META_BRACING_EARTHSTORM:      return "bracing_earthstorm";
  case META_CHAOTIC_SKYFIRE:         return "chaotic_skyfire";
  case META_CHAOTIC_SKYFLARE:        return "chaotic_skyflare";
  case META_EMBER_SKYFLARE:          return "ember_skyflare";
  case META_ENIGMATIC_SKYFLARE:      return "enigmatic_skyflare";
  case META_ENIGMATIC_STARFLARE:     return "enigmatic_starflare";
  case META_ENIGMATIC_SKYFIRE:       return "enigmatic_skyfire";
  case META_ETERNAL_EARTHSIEGE:      return "eternal_earthsiege";
  case META_ETERNAL_EARTHSTORM:      return "eternal_earthstorm";
  case META_FORLORN_SKYFLARE:        return "forlorn_skyflare";
  case META_FORLORN_STARFLARE:       return "forlorn_starflare";
  case META_IMPASSIVE_SKYFLARE:      return "impassive_skyflare";
  case META_IMPASSIVE_STARFLARE:     return "impassive_starflare";
  case META_INSIGHTFUL_EARTHSIEGE:   return "insightful_earthsiege";
  case META_INSIGHTFUL_EARTHSTORM:   return "insightful_earthstorm";
  case META_INVIGORATING_EARTHSIEGE: return "invigorating_earthsiege";
  case META_MYSTICAL_SKYFIRE:        return "mystical_skyfire";
  case META_PERSISTENT_EARTHSHATTER: return "persistent_earthshatter";
  case META_PERSISTENT_EARTHSIEGE:   return "persistent_earthsiege";
  case META_POWERFUL_EARTHSHATTER:   return "powerful_earthshatter";
  case META_POWERFUL_EARTHSIEGE:     return "powerful_earthsiege";
  case META_POWERFUL_EARTHSTORM:     return "powerful_earthstorm";
  case META_RELENTLESS_EARTHSIEGE:   return "relentless_earthsiege";
  case META_RELENTLESS_EARTHSTORM:   return "relentless_earthstorm";
  case META_REVITALIZING_SKYFLARE:   return "revitalizing_skyflare";
  case META_SWIFT_SKYFIRE:           return "swift_skyfire";
  case META_SWIFT_SKYFLARE:          return "swift_skyflare";
  case META_SWIFT_STARFIRE:          return "swift_starfire";
  case META_SWIFT_STARFLARE:         return "swift_starflare";
  case META_TIRELESS_STARFLARE:      return "tireless_starflare";
  case META_TRENCHANT_EARTHSHATTER:  return "trenchant_earthshatter";
  case META_TRENCHANT_EARTHSIEGE:    return "trenchant_earthsiege";
  }
  return "unknown";
}

// util_t::parse_meta_gem_type =============================================

int util_t::parse_meta_gem_type( const std::string& name )
{
  for ( int i=0; i < META_GEM_MAX; i++ )
    if ( str_compare_ci( name, util_t::meta_gem_type_string( i ) ) )
      return i;

  return META_GEM_NONE;
}

// util_t::result_type_string ==============================================

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
  case RESULT_HIT:    return "hit";
  }
  return "unknown";
}

// util_t::parse_result_type ===============================================

int util_t::parse_result_type( const std::string& name )
{
  for ( int i=0; i < RESULT_MAX; i++ )
    if ( str_compare_ci( name, util_t::result_type_string( i ) ) )
      return i;

  return RESULT_NONE;
}

// util_t::resource_type_string ============================================

const char* util_t::resource_type_string( int type )
{
  switch ( type )
  {
  case RESOURCE_NONE:   return "none";
  case RESOURCE_HEALTH: return "health";
  case RESOURCE_MANA:   return "mana";
  case RESOURCE_RAGE:   return "rage";
  case RESOURCE_ENERGY: return "energy";
  case RESOURCE_FOCUS:  return "focus";
  case RESOURCE_RUNIC:  return "runic_power";
  }
  return "unknown";
}

// util_t::parse_resource_type =============================================

int util_t::parse_resource_type( const std::string& name )
{
  for ( int i=0; i < RESOURCE_MAX; i++ )
    if ( str_compare_ci( name, util_t::resource_type_string( i ) ) )
      return i;

  return RESOURCE_NONE;
}

// util_t::school_type_string ==============================================

const char* util_t::school_type_string( int school )
{
  switch ( school )
  {
  case SCHOOL_ARCANE:    return "arcane";
  case SCHOOL_BLEED:     return "bleed";
  case SCHOOL_CHAOS:     return "chaos";
  case SCHOOL_FIRE:      return "fire";
  case SCHOOL_FROST:     return "frost";
  case SCHOOL_FROSTFIRE: return "frostfire";
  case SCHOOL_HOLY:      return "holy";
  case SCHOOL_NATURE:    return "nature";
  case SCHOOL_PHYSICAL:  return "physical";
  case SCHOOL_SHADOW:    return "shadow";
  case SCHOOL_DRAIN:     return "drain";
  }
  return "unknown";
}

// util_t::parse_school_type ===============================================

int util_t::parse_school_type( const std::string& name )
{
  for ( int i=0; i < SCHOOL_MAX; i++ )
    if ( str_compare_ci( name, util_t::school_type_string( i ) ) )
      return i;

  return SCHOOL_NONE;
}

// util_t::talent_tree_string ==============================================

const char* util_t::talent_tree_string( int tree )
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
  }
  return "unknown";
}

// util_t::parse_talent_tree ===============================================

int util_t::parse_talent_tree( const std::string& name )
{
  for ( int i=0; i < TALENT_TREE_MAX; i++ )
    if ( str_compare_ci( name, util_t::talent_tree_string( i ) ) )
      return i;

  return TREE_NONE;
}

// util_t::weapon_type_string ==============================================

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

// util_t::parse_weapon_type ===============================================

int util_t::parse_weapon_type( const std::string& name )
{
  for ( int i=0; i < WEAPON_MAX; i++ )
    if ( str_compare_ci( name, util_t::weapon_type_string( i ) ) )
      return i;

  return WEAPON_NONE;
}

// util_t::flask_type_string ===============================================

const char* util_t::flask_type_string( int flask )
{
  switch ( flask )
  {
  case FLASK_NONE:               return "none";
  case FLASK_BLINDING_LIGHT:     return "blinding_light";
  case FLASK_DISTILLED_WISDOM:   return "distilled_wisdom";
  case FLASK_ENDLESS_RAGE:       return "endless_rage";
  case FLASK_FROST_WYRM:         return "frost_wyrm";
  case FLASK_MIGHTY_RESTORATION: return "mighty_restoration";
  case FLASK_PURE_DEATH:         return "pure_death";
  case FLASK_PURE_MOJO:          return "pure_mojo";
  case FLASK_RELENTLESS_ASSAULT: return "relentless_assault";
  case FLASK_SUPREME_POWER:      return "supreme_power";
  }
  return "unknown";
}

// util_t::parse_flask_type ================================================

int util_t::parse_flask_type( const std::string& name )
{
  for ( int i=0; i < FLASK_MAX; i++ )
    if ( str_compare_ci( name, util_t::flask_type_string( i ) ) )
      return i;

  return FLASK_NONE;
}

// util_t::food_type_string ================================================

const char* util_t::food_type_string( int food )
{
  switch ( food )
  {
  case FOOD_NONE:                     return "none";
  case FOOD_BLACKENED_BASILISK:       return "blackened_basilisk";
  case FOOD_BLACKENED_DRAGONFIN:      return "blackened_dragonfin";
  case FOOD_CRUNCHY_SERPENT:          return "crunchy_serpent";
  case FOOD_DRAGONFIN_FILET:          return "dragonfin_filet";
  case FOOD_FISH_FEAST:               return "fish_feast";
  case FOOD_GOLDEN_FISHSTICKS:        return "golden_fishsticks";
  case FOOD_GREAT_FEAST:              return "great_feast";
  case FOOD_HEARTY_RHINO:             return "hearty_rhino";
  case FOOD_IMPERIAL_MANTA_STEAK:     return "imperial_manta_steak";
  case FOOD_MEGA_MAMMOTH_MEAL:        return "mega_mammoth_meal";
  case FOOD_POACHED_NORTHERN_SCULPIN: return "poached_northern_sculpin";
  case FOOD_POACHED_BLUEFISH:         return "poached_bluefish";
  case FOOD_RHINOLICIOUS_WORMSTEAK:   return "rhinolicious_wormsteak";
  case FOOD_SMOKED_SALMON:            return "smoked_salmon";
  case FOOD_SNAPPER_EXTREME:          return "snapper_extreme";
  case FOOD_TENDER_SHOVELTUSK_STEAK:  return "tender_shoveltusk_steak";
  case FOOD_VERY_BURNT_WORG:          return "very_burnt_worg";
  }
  return "unknown";
}

// util_t::parse_food_type =================================================

int util_t::parse_food_type( const std::string& name )
{
  for ( int i=0; i < FOOD_MAX; i++ )
    if ( str_compare_ci( name, util_t::food_type_string( i ) ) )
      return i;

  return FOOD_NONE;
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

// util_t::parse_slot_type =================================================

int util_t::parse_slot_type( const std::string& name )
{
  for ( int i=0; i < SLOT_MAX; i++ )
    if ( str_compare_ci( name, util_t::slot_type_string( i ) ) )
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
  case STAT_RAGE:   return "rage";
  case STAT_ENERGY: return "energy";
  case STAT_FOCUS:  return "focus";
  case STAT_RUNIC:  return "runic";

  case STAT_SPELL_POWER:       return "spell_power";
  case STAT_SPELL_PENETRATION: return "spell_penetration";
  case STAT_MP5:               return "mp5";

  case STAT_ATTACK_POWER:             return "attack_power";
  case STAT_EXPERTISE_RATING:         return "expertise_rating";
  case STAT_ARMOR_PENETRATION_RATING: return "armor_penetration_rating";

  case STAT_HIT_RATING:   return "hit_rating";
  case STAT_CRIT_RATING:  return "crit_rating";
  case STAT_HASTE_RATING: return "haste_rating";

  case STAT_WEAPON_DPS:   return "weapon_dps";
  case STAT_WEAPON_SPEED: return "weapon_speed";

  case STAT_WEAPON_OFFHAND_DPS:    return "weapon_offhand_dps";
  case STAT_WEAPON_OFFHAND_SPEED:  return "weapon_offhand_speed";

  case STAT_ARMOR:          return "armor";
  case STAT_BONUS_ARMOR:    return "bonus_armor";
  case STAT_DEFENSE_RATING: return "defense_rating";
  case STAT_DODGE_RATING:   return "dodge_rating";
  case STAT_PARRY_RATING:   return "parry_rating";

  case STAT_BLOCK_RATING: return "block_rating";
  case STAT_BLOCK_VALUE:  return "block_value";

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
  case STAT_ARMOR_PENETRATION_RATING: return "ArPen";

  case STAT_HIT_RATING:   return "Hit";
  case STAT_CRIT_RATING:  return "Crit";
  case STAT_HASTE_RATING: return "Haste";

  case STAT_WEAPON_DPS:   return "Wdps";
  case STAT_WEAPON_SPEED: return "Wspeed";

  case STAT_WEAPON_OFFHAND_DPS:    return "WOHdps";
  case STAT_WEAPON_OFFHAND_SPEED:  return "WOHspeed";

  case STAT_ARMOR:          return "Armor";
  case STAT_BONUS_ARMOR:    return "BArmor";
  case STAT_DEFENSE_RATING: return "Def";
  case STAT_DODGE_RATING:   return "Dodge";
  case STAT_PARRY_RATING:   return "Parry";

  case STAT_BLOCK_RATING: return "BlockR";
  case STAT_BLOCK_VALUE: return "BlockV";

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
  case STAT_SPIRIT:    return "spi";

  case STAT_HEALTH: return "health";
  case STAT_MANA:   return "mana";
  case STAT_RAGE:   return "rage";
  case STAT_ENERGY: return "energy";
  case STAT_FOCUS:  return "focus";
  case STAT_RUNIC:  return "runic";

  case STAT_SPELL_POWER:       return "splpwr";
  case STAT_SPELL_PENETRATION: return "splpen";
  case STAT_MP5:               return "manargn";

  case STAT_ATTACK_POWER:             return "atkpwr";
  case STAT_EXPERTISE_RATING:         return "exprtng";
  case STAT_ARMOR_PENETRATION_RATING: return "armorpenrtng";

  case STAT_HIT_RATING:   return "hitrtng";
  case STAT_CRIT_RATING:  return "critstrkrtng";
  case STAT_HASTE_RATING: return "hastertng";

  case STAT_WEAPON_DPS:   return "__wdps";
  case STAT_WEAPON_SPEED: return "__wspeed";

  case STAT_ARMOR:          return "armor";
  case STAT_BONUS_ARMOR:    return "__armor"; // FIXME! Does wowhead distinguish "bonus" armor?
  case STAT_DEFENSE_RATING: return "defrtng";
  case STAT_DODGE_RATING:   return "dodgertng";
  case STAT_PARRY_RATING:   return "parryrtng";

  case STAT_BLOCK_RATING: return "blockrtng";
  case STAT_BLOCK_VALUE: return "block";

  case STAT_MAX: return "__all";
  }
  return "unknown";
}

// util_t::parse_stat_type =================================================

int util_t::parse_stat_type( const std::string& name )
{
  for ( int i=0; i <= STAT_MAX; i++ )
    if ( str_compare_ci( name, util_t::stat_type_string( i ) ) )
      return i;

  for ( int i=0; i <= STAT_MAX; i++ )
    if ( str_compare_ci( name, util_t::stat_type_abbrev( i ) ) )
      return i;

  for ( int i=0; i <= STAT_MAX; i++ )
    if ( str_compare_ci( name, util_t::stat_type_wowhead( i ) ) )
      return i;

  if ( name == "rgdcritstrkrtng" ) return STAT_CRIT_RATING;

  return STAT_NONE;
}

// util_t::translate_class_id ==============================================

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

// util_t::translate_class_id ==============================================

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

// util_t::translate_race_id ===============================================

int util_t::translate_race_id( int rid )
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
  case 10: return RACE_BLOOD_ELF;
  case 11: return RACE_DRAENEI;
  }

  return RACE_NONE;
}

// util_t::socket_gem_match ================================================

bool util_t::socket_gem_match( int socket,
                               int gem )
{
  if ( socket == GEM_NONE || gem == GEM_PRISMATIC ) return true;

  if ( socket == GEM_META ) return ( gem == GEM_META );

  if ( socket == GEM_RED    ) return ( gem == GEM_RED    || gem == GEM_ORANGE || gem == GEM_PURPLE );
  if ( socket == GEM_YELLOW ) return ( gem == GEM_YELLOW || gem == GEM_ORANGE || gem == GEM_GREEN  );
  if ( socket == GEM_BLUE   ) return ( gem == GEM_BLUE   || gem == GEM_PURPLE || gem == GEM_GREEN  );

  return false;
}

// util_t::string_split ====================================================

int util_t::string_split( std::vector<std::string>& results,
                          const std::string&        str,
                          const char*               delim,
                          bool                      allow_quotes )
{
  std::string buffer = str;
  std::string::size_type cut_pt;

  std::string search = delim;
  search += '"';
  bool in_quotes = false;
  std::string temp_str;

  while ( ( cut_pt = buffer.find_first_of( search ) ) != buffer.npos )
  {
    if ( cut_pt > 0 )
    {
      if ( allow_quotes && ( buffer[cut_pt] == '"' ) )
      {
        in_quotes = !in_quotes;
        temp_str = buffer.substr( 0, cut_pt );
        temp_str += buffer.substr( cut_pt + 1 );
        buffer = temp_str;
        if ( in_quotes )
        {
          search = '"';
        }
        else
        {
          search = delim;
          search += '"';
        }
      }
      if ( !in_quotes )
      {
        results.push_back( buffer.substr( 0, cut_pt ) );
      }
    }
    if ( !in_quotes )
    {
      if ( buffer.length() > cut_pt )
      {
        buffer = buffer.substr( cut_pt + 1 );
      }
      else
      {
        buffer = "";
      }
    }
  }
  if ( buffer.length() > 0 )
  {
    results.push_back( buffer );
  }

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
  return static_cast<int>( results.size() );
}

// util_t::string_split ====================================================

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
      else if ( f == "s" ) strcpy( ( char* )   va_arg( vap, char* ), s );
      else if ( f == "S" ) *( ( std::string* ) va_arg( vap, std::string* ) ) = s;
      else assert( 0 );
    }
    va_end( vap );
  }

  return str_size;
}

// util_t::string_strip_quotes =============================================

int util_t::string_strip_quotes( std::string& str )
{
  std::string old_string = str;

  std::string::size_type cut_pt;

  std::string search = "";
  search += '"';
  std::string temp_str;

  while ( ( cut_pt = old_string.find_first_of( search ) ) != old_string.npos )
  {
    if ( cut_pt > 0 )
    {
      temp_str = old_string.substr( 0, cut_pt );
      temp_str += old_string.substr( cut_pt + 1 );
      old_string = temp_str;
    }
    else
    {
      temp_str = old_string.substr( 1 );
      old_string = temp_str;
    }
  }

  str = old_string;

  return 0;
}

// util_t::to_string =======================================================

std::string& util_t::to_string( int i )
{
  static std::string s;
  char buffer[ 1024 ];
  sprintf( buffer, "%d", i );
  s = buffer;
  return s;
}

// util_t::to_string =======================================================

std::string& util_t::to_string( double f, int precision )
{
  static std::string s;
  char buffer[ 1024 ];
  sprintf( buffer, "%.*f", precision, f );
  s = buffer;
  return s;
}

// util_t::milliseconds ====================================================

int64_t util_t::milliseconds()
{
  return 1000 * clock() / CLOCKS_PER_SEC;
}

// util_t::parse_date ======================================================

int64_t util_t::parse_date( const std::string& month_day_year )
{
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, month_day_year, " _,;-/" );
  if ( num_splits != 3 ) return 0;

  std::string month = splits[ 0 ];
  std::string day   = splits[ 1 ];
  std::string year  = splits[ 2 ];

  for ( int i=0; i < month.size(); i++ ) month[ i ] = tolower( month[ i ] );

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
  if ( month.find( "ded" ) != std::string::npos ) month = "12";

  if ( month.size() == 1 ) month.insert( month.begin(), '0'  );
  if ( day  .size() == 1 ) day  .insert( day  .begin(), '0'  );
  if ( year .size() == 2 ) year .insert( year .begin(), 2, '0' );

  if ( day.size()   != 2 ) return 0;
  if ( month.size() != 2 ) return 0;
  if ( year.size()  != 4 ) return 0;

  std::string buffer = year + month + day;

  return atoi( buffer.c_str() );
}

// util_t::printf ========================================================


int util_t::printf( const char *format,  ... )
{
  va_list fmtargs;
  int retcode = 0;
  char *p_locale = NULL;
  char buffer_locale[ 1024 ];

  p_locale = setlocale( LC_CTYPE, NULL );
  if ( p_locale != NULL )
  {
    strncpy( buffer_locale, p_locale, 1023 );
    buffer_locale[1023] = '\0';
  }
  else
  {
    buffer_locale[0] = '\0';
  }

  setlocale( LC_CTYPE, "" );

  va_start( fmtargs, format );
  retcode = vprintf( format, fmtargs );
  va_end( fmtargs );

  setlocale( LC_CTYPE, p_locale );

  return retcode;
}

// util_t::fprintf ========================================================

int util_t::fprintf( FILE *stream, const char *format,  ... )
{
  va_list fmtargs;
  int retcode = 0;
  char *p_locale = NULL;
  char buffer_locale[ 1024 ];

  p_locale = setlocale( LC_CTYPE, NULL );
  if ( p_locale != NULL )
  {
    strncpy( buffer_locale, p_locale, 1023 );
    buffer_locale[1023] = '\0';
  }
  else
  {
    buffer_locale[0] = '\0';
  }

  setlocale( LC_CTYPE, "" );

  va_start( fmtargs, format );
  retcode = vfprintf( stream, format, fmtargs );
  va_end( fmtargs );

  setlocale( LC_CTYPE, p_locale );

  return retcode;
}

std::string& util_t::utf8_binary_to_hex( std::string& name )
{
  if ( name.empty() ) return name;

  std::string buffer="";
  char buffer2[32];

  int size = ( int ) name.size();
  for ( int i=0; i < size; i++ )
  {
    unsigned char c = name[ i ];

    if ( c >= 0x80 )
    {
      snprintf( buffer2,31,"%%%0X",c );
      buffer2[31] = '\0';
      buffer += buffer2;
      continue;
    }

    buffer += c;
  }
  name = buffer;

  return name;
}

std::string& util_t::ascii_binary_to_utf8_hex( std::string& name )
{
  if ( name.empty() ) return name;

  std::string buffer="";
  char buffer2[32];

  int size = ( int ) name.size();
  for ( int i=0; i < size; i++ )
  {
    unsigned char c = name[ i ];

    if ( c >= 0x80 )
    {
      if ( c < 0xC0 )
      {
        buffer += "%C2";
      }
      else
      {
        buffer += "%C3";
        c -= 0x40;
      }
      snprintf( buffer2,31,"%%%0X",c );
      buffer2[31] = '\0';
      buffer += buffer2;
      continue;
    }

    buffer += c;
  }
  name = buffer;

  return name;
}

std::string& util_t::utf8_hex_to_ascii( std::string& name )
{
  if ( name.empty() ) return name;

  std::string buffer="";

  int size = ( int ) name.size();
  for ( int i=0; i < size; i++ )
  {
    unsigned char c = name[ i ];

    if ( c == '%' )
    {
      if ( ( ( i + 5 ) < size ) && ( name[ i + 3 ] == '%' ) &&
           ( tolower( name[ i + 1 ] ) == 'c' ) &&
           ( name[ i + 2 ] >= '2' ) && ( name[ i + 2 ] <= '3' ) &&
           (
             ( name[ i + 4 ] == '8' ) || ( name[ i + 4 ] == '9' ) ||
             ( tolower( name[ i + 4 ] ) == 'a' ) || ( tolower( name[ i + 4 ] ) == 'b' )
           )
         )
      {
        int num = 0;
        switch ( tolower( name[ i + 4 ] ) )
        {
        case 8:
          num=0x80;
          break;
        case 9:
          num=0x90;
          break;
        case 'a':
          num=0xA0;
          break;
        case 'b':
          num=0xB0;
          break;
        }

        int c2 = tolower( name[ i + 5 ] );
        if ( ( c2 >= '0' ) && ( c2 <= '9' ) )
        {
          num += c2 - '0';
        }
        else if ( ( c2 >= 'a' ) && ( c2 <= 'f' ) )
        {
          num += 10 + c2 - 'a';
        }
        else
        {
          buffer += c;
          continue;
        }

        if ( name[ i + 2 ] == '3' )
        {
          num += 0x40;
        }
        buffer += ( unsigned char ) num;

        i += 5;
        continue;
      }

      if ( ( i + 2 ) < size )
      {
        int num = 0;
        unsigned char d;

        num = tolower( name[ i + 1 ] ) - 'c';
        if ( ( num < 0 ) || ( num > 3 ) )
        {
          buffer += c;
          continue;
        }
        num = num << 4;
        num += 0xC0;
        d = tolower( name[ i + 2 ] );
        if ( isdigit( d ) )
        {
          num += d - '0';
        }
        else if ( ( d >= 'a' ) || ( d <= 'f' ) )
        {
          num += 10 + d - 'a';
        }
        else
        {
          buffer += c;
          continue;
        }

        if ( num < 0xC2 || num > 0xF4 )
        {
          buffer += c;
          continue;
        }
        if ( num >= 0xC2 && num <= 0xDF )
        {
          i += 5;
        }
        else if ( num >= 0xE0 && num <= 0xEF )
        {
          i += 8;
        }
        else
        {
          i += 11;
        }
      }
      continue;
    }

    buffer += c;
  }
  name = buffer;

  return name;
}


std::string& util_t::format_name( std::string& name )
{
  if ( name.empty() ) return name;

  util_t::utf8_hex_to_ascii( name );

  return name;
}

void util_t::add_base_stats( base_stats_t& result, base_stats_t& a, base_stats_t b )
{
  result.level      = a.level      + b.level;
  result.health     = a.health     + b.health;
  result.mana       = a.mana       + b.mana;
  result.strength   = a.strength   + b.strength;
  result.agility    = a.agility    + b.agility;
  result.stamina    = a.stamina    + b.stamina;
  result.intellect  = a.intellect  + b.intellect;
  result.spirit     = a.spirit     + b.spirit;
  result.spell_crit = a.spell_crit + b.spell_crit;
  result.melee_crit = a.melee_crit + b.melee_crit;
}

void util_t::translate_talent_trees( std::vector<talent_translation_t>& talent_list, talent_translation_t translation_table[][ MAX_TALENT_TREES ], size_t table_size )
{
  size_t count = 0;
	int trees[ MAX_TALENT_TREES ];
  size_t max_i = table_size / sizeof( talent_translation_t ) / MAX_TALENT_TREES;

	for( size_t j = 0; j < MAX_TALENT_TREES; j++ )
	{
	  trees[ j ] = 0;
		for( size_t i = 0; i < max_i; i++ )
    {
			if( translation_table[ i ][ j ].index > 0 )
			{
				talent_list.push_back( translation_table[ i ][ j ] );
				talent_list[ count ].tree = j;
				talent_list[ count ].name = "";
				if( talent_list[ count ].req > 0 )
				{
					for( size_t k = 0; k < j; k++ )
						talent_list[ count ].req += trees[ k ];
				}
				talent_list[ count ].index = count+1;
				count++;
				trees[ j ]++;
			}
		}
	}
}

//-------------------------------
// std::STRING   utils
//-------------------------------


// utility functions
std::string tolower( std::string src )
{
  std::string dest=src;
  for ( unsigned i=0; i<dest.length(); i++ )
    dest[i]=tolower( dest[i] );
  return dest;
}

std::string trim( std::string src )
{
  if ( src=="" ) return "";
  std::string dest=src;
  //remove left
  int p=0;
  while ( ( p<( int )dest.length() )&&( dest[p]==' ' ) ) p++;
  if ( p>0 )
    dest.erase( 0,p );
  //remove right
  p= ( int ) dest.length()-1;
  while ( ( p>=0 )&&( dest[p]==' ' ) ) p--;
  if ( p<( int )dest.length()-1 )
    dest.erase( p+1 );
  //return trimmed string
  return dest;
}


void replace_char( std::string& src, char old_c, char new_c  )
{
  for ( int i=0; i<( int )src.length(); i++ )
    if ( src[i]==old_c )
      src[i]=new_c;
}

void replace_str( std::string& src, std::string old_str, std::string new_str  )
{
  if ( old_str=="" ) return;
  std::string dest="";
  size_t p;
  while ( ( p=src.find( old_str ) )!=std::string::npos )
  {
    dest+=src.substr( 0,p )+new_str;
    src.erase( 0,p+old_str.length() );
  }
  dest+=src;
  src=dest;
}

bool my_isdigit( char c )
{
  if ( c=='+' ) return true;
  if ( c=='-' ) return true;
  return isdigit( c )!=0;
}

bool str_to_float( std::string src, double& dest )
{
  bool was_sign=false;
  bool was_digit=false;
  bool was_dot=false;
  bool res=true;
  //check each char
  for ( size_t p=0; res&&( p<src.length() ); p++ )
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

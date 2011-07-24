// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef __SIMULATIONCRAFT_H
#define __SIMULATIONCRAFT_H

// Platform Initialization ==================================================

#if defined( _MSC_VER ) || defined( __MINGW__ ) || defined( _WINDOWS ) || defined( WIN32 )
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  define _CRT_SECURE_NO_WARNINGS
#  define DIRECTORY_DELIMITER "\\"
#  define SIGACTION 0
#else
#  define DIRECTORY_DELIMITER "/"
#  define SIGACTION 1
#endif

// Switching of using 'const'-flag on methods possible ======================
// Results of the sim should always be the same, no matter if this is set to:
// #define SC_CONST const    -OR-
// #define SC_CONST
#define SC_CONST const

#if defined( _MSC_VER )
#  include "../vs/stdint.h"
#  define snprintf _snprintf
#  define strdup _strdup
#else
#  include <stdint.h>
#endif

#if defined(__GNUC__)
#  define likely(x)       __builtin_expect((x),1)
#  define unlikely(x)     __builtin_expect((x),0)
#else
#  define likely(x) (x)
#  define unlikely(x) (x)
#endif

#include <typeinfo>
#include <stdarg.h>
#include <float.h>
#include <time.h>
#include <string>
#include <queue>
#include <vector>
#include <list>
#include <map>
#include <limits>
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sstream>
#include <cctype>

#include "data_enums.hh"

#if __BSD_VISIBLE
#  include <netinet/in.h>
#  if !defined(CLOCKS_PER_SEC)
#    define CLOCKS_PER_SEC 1000000
#  endif
#endif

#if SIGACTION
#include <signal.h>
#endif

#include "data_definitions.hh"

#define SC_MAJOR_VERSION "420"
#define SC_MINOR_VERSION "3"
#define SC_USE_PTR ( 0 )
#define SC_BETA ( 0 )

// Forward Declarations ======================================================

struct action_t;
struct action_callback_t;
struct action_expr_t;
struct alias_t;
struct attack_t;
struct base_stats_t;
struct buff_t;
struct callback_t;
struct cooldown_t;
struct dbc_t;
struct death_knight_t;
struct druid_t;
struct dot_t;
struct effect_t;
struct enemy_t;
struct enchant_t;
struct event_t;
struct gain_t;
struct hunter_t;
struct item_t;
struct js_node_t;
struct mage_t;
struct option_t;
struct paladin_t;
struct pet_t;
struct player_t;
struct plot_t;
struct priest_t;
struct proc_t;
struct raid_event_t;
struct rank_t;
struct rating_t;
struct reforge_plot_t;
struct report_t;
struct rng_t;
struct talent_t;
struct spell_id_t;
struct active_spell_t;
struct passive_spell_t;
struct rogue_t;
struct scaling_t;
struct shaman_t;
struct sim_t;
struct spell_t;
struct spell_data_t;
struct spelleffect_data_t;
struct heal_t;
struct stats_t;
struct talent_t;
struct talent_translation_t;
struct unique_gear_t;
struct uptime_t;
struct warlock_t;
struct warrior_t;
struct weapon_t;
struct xml_node_t;

// Enumerations ==============================================================

enum race_type
{
  RACE_NONE=0,
  // Target Races
  RACE_BEAST, RACE_DRAGONKIN, RACE_GIANT, RACE_HUMANOID, RACE_DEMON, RACE_ELEMENTAL,
  // Player Races
  RACE_NIGHT_ELF, RACE_HUMAN, RACE_GNOME, RACE_DWARF, RACE_DRAENEI, RACE_WORGEN,
  RACE_ORC, RACE_TROLL, RACE_UNDEAD, RACE_BLOOD_ELF, RACE_TAUREN, RACE_GOBLIN,
  RACE_MAX
};

enum player_type
{
  PLAYER_SPECIAL_SCALE=-1,
  PLAYER_NONE=0,
  DEATH_KNIGHT, DRUID, HUNTER, MAGE, PALADIN, PRIEST, ROGUE, SHAMAN, WARLOCK, WARRIOR,
  PLAYER_PET, PLAYER_GUARDIAN,
  ENEMY, ENEMY_ADD,
  PLAYER_MAX
};

enum pet_type_t
{
  PET_NONE=0,

  // Ferocity
  PET_CARRION_BIRD,
  PET_CAT,
  PET_CORE_HOUND,
  PET_DEVILSAUR,
  PET_DOG,
  PET_FOX,
  PET_HYENA,
  PET_MOTH,
  PET_RAPTOR,
  PET_SPIRIT_BEAST,
  PET_TALLSTRIDER,
  PET_WASP,
  PET_WOLF,

  PET_FEROCITY,

  // Tenacity
  PET_BEAR,
  PET_BEETLE,
  PET_BOAR,
  PET_CRAB,
  PET_CROCOLISK,
  PET_GORILLA,
  PET_RHINO,
  PET_SCORPID,
  PET_SHALE_SPIDER,
  PET_TURTLE,
  PET_WARP_STALKER,
  PET_WORM,

  PET_TENACITY,

  // Cunning
  PET_BAT,
  PET_BIRD_OF_PREY,
  PET_CHIMERA,
  PET_DRAGONHAWK,
  PET_MONKEY,
  PET_NETHER_RAY,
  PET_RAVAGER,
  PET_SERPENT,
  PET_SILITHID,
  PET_SPIDER,
  PET_SPOREBAT,
  PET_WIND_SERPENT,

  PET_CUNNING,

  PET_HUNTER,

  PET_FELGUARD,
  PET_FELHUNTER,
  PET_IMP,
  PET_VOIDWALKER,
  PET_SUCCUBUS,
  PET_INFERNAL,
  PET_DOOMGUARD,
  PET_EBON_IMP,
  PET_WARLOCK,

  PET_GHOUL,
  PET_BLOODWORMS,
  PET_DANCING_RUNE_WEAPON,
  PET_DEATH_KNIGHT,

  PET_TREANTS,
  PET_DRUID,

  PET_WATER_ELEMENTAL,
  PET_MAGE,

  PET_SHADOWFIEND,
  PET_PRIEST,

  PET_SPIRIT_WOLF,
  PET_FIRE_ELEMENTAL,
  PET_EARTH_ELEMENTAL,
  PET_SHAMAN,

  PET_ENEMY,

  PET_MAX
};

enum dmg_type { DMG_DIRECT=0, DMG_OVER_TIME=1, HEAL_DIRECT, HEAL_OVER_TIME, ABSORB };

enum stats_type { STATS_DMG, STATS_HEAL, STATS_ABSORB };

enum dot_behavior_type { DOT_CLIP=0, DOT_REFRESH };

enum attribute_type { ATTRIBUTE_NONE=0, ATTR_STRENGTH, ATTR_AGILITY, ATTR_STAMINA, ATTR_INTELLECT, ATTR_SPIRIT, ATTRIBUTE_MAX };

enum base_stat_type { BASE_STAT_STRENGTH=0, BASE_STAT_AGILITY, BASE_STAT_STAMINA, BASE_STAT_INTELLECT, BASE_STAT_SPIRIT,
                      BASE_STAT_HEALTH, BASE_STAT_MANA,
                      BASE_STAT_MELEE_CRIT_PER_AGI, BASE_STAT_SPELL_CRIT_PER_INT,
                      BASE_STAT_DODGE_PER_AGI,
                      BASE_STAT_MELEE_CRIT, BASE_STAT_SPELL_CRIT, BASE_STAT_MP5, BASE_STAT_SPI_REGEN, BASE_STAT_MAX };

enum resource_type
{
  RESOURCE_NONE=0,
  RESOURCE_HEALTH, RESOURCE_MANA,  RESOURCE_RAGE, RESOURCE_ENERGY, RESOURCE_FOCUS, RESOURCE_RUNIC,
  RESOURCE_RUNE, RESOURCE_HAPPINESS, RESOURCE_SOUL_SHARDS, RESOURCE_ECLIPSE, RESOURCE_HOLY_POWER,
  RESOURCE_MAX
};

enum result_type
{
  RESULT_UNKNOWN=-1,
  RESULT_NONE=0,
  RESULT_MISS,  RESULT_RESIST, RESULT_DODGE, RESULT_PARRY,
  RESULT_BLOCK, RESULT_CRIT_BLOCK, RESULT_GLANCE, RESULT_CRIT, RESULT_HIT,
  RESULT_MAX
};

#define RESULT_HIT_MASK  ( (1<<RESULT_GLANCE) | (1<<RESULT_BLOCK) | (1<<RESULT_CRIT) | (1<<RESULT_HIT) )
#define RESULT_CRIT_MASK ( (1<<RESULT_CRIT) )
#define RESULT_MISS_MASK ( (1<<RESULT_MISS) )
#define RESULT_NONE_MASK ( (1<<RESULT_NONE) )
#define RESULT_ALL_MASK  -1

enum proc_type
{
  PROC_NONE=0,
  PROC_DAMAGE,
  PROC_HEAL,
  PROC_TICK_DAMAGE,
  PROC_DIRECT_DAMAGE,
  PROC_DIRECT_HEAL,
  PROC_TICK_HEAL,
  PROC_ATTACK,
  PROC_SPELL,
  PROC_TICK,
  PROC_SPELL_AND_TICK,
  PROC_HEAL_SPELL,
  PROC_HARMFUL_SPELL,
  PROC_MAX
};

enum action_type { ACTION_USE=0, ACTION_SPELL, ACTION_ATTACK, ACTION_SEQUENCE, ACTION_OTHER, ACTION_MAX };

enum school_type
{
  SCHOOL_NONE=0,
  SCHOOL_ARCANE,      SCHOOL_FIRE,        SCHOOL_FROST,       SCHOOL_HOLY,        SCHOOL_NATURE,
  SCHOOL_SHADOW,      SCHOOL_PHYSICAL,    SCHOOL_MAX_PRIMARY, SCHOOL_FROSTFIRE,
  SCHOOL_HOLYSTRIKE,  SCHOOL_FLAMESTRIKE, SCHOOL_HOLYFIRE,    SCHOOL_STORMSTRIKE, SCHOOL_HOLYSTORM,
  SCHOOL_FIRESTORM,   SCHOOL_FROSTSTRIKE, SCHOOL_HOLYFROST,   SCHOOL_FROSTSTORM,  SCHOOL_SHADOWSTRIKE,
  SCHOOL_SHADOWLIGHT, SCHOOL_SHADOWFLAME, SCHOOL_SHADOWSTORM, SCHOOL_SHADOWFROST, SCHOOL_SPELLSTRIKE,
  SCHOOL_DIVINE,      SCHOOL_SPELLFIRE,   SCHOOL_SPELLSTORM,  SCHOOL_SPELLFROST,  SCHOOL_SPELLSHADOW,
  SCHOOL_ELEMENTAL,   SCHOOL_CHROMATIC,   SCHOOL_MAGIC,       SCHOOL_CHAOS,       SCHOOL_BLEED,
  SCHOOL_DRAIN,
  SCHOOL_MAX
};

const int64_t SCHOOL_ATTACK_MASK = ( ( int64_t( 1 ) << SCHOOL_BLEED )        | ( int64_t( 1 ) << SCHOOL_PHYSICAL )     |
                                     ( int64_t( 1 ) << SCHOOL_HOLYSTRIKE )   | ( int64_t( 1 ) << SCHOOL_FLAMESTRIKE )  |
                                     ( int64_t( 1 ) << SCHOOL_STORMSTRIKE )  | ( int64_t( 1 ) << SCHOOL_FROSTSTRIKE )  |
                                     ( int64_t( 1 ) << SCHOOL_SHADOWSTRIKE ) | ( int64_t( 1 ) << SCHOOL_SPELLSTRIKE )  );
                                      // SCHOOL_CHAOS should probably be added here too.

const int64_t SCHOOL_SPELL_MASK  ( ( int64_t( 1 ) << SCHOOL_ARCANE )         | ( int64_t( 1 ) << SCHOOL_CHAOS )        |
                                   ( int64_t( 1 ) << SCHOOL_FIRE )           | ( int64_t( 1 ) << SCHOOL_FROST )        |
                                   ( int64_t( 1 ) << SCHOOL_FROSTFIRE )      | ( int64_t( 1 ) << SCHOOL_HOLY )         |
                                   ( int64_t( 1 ) << SCHOOL_NATURE )         | ( int64_t( 1 ) << SCHOOL_SHADOW )       |
                                   ( int64_t( 1 ) << SCHOOL_HOLYSTRIKE )     | ( int64_t( 1 ) << SCHOOL_FLAMESTRIKE )  |
                                   ( int64_t( 1 ) << SCHOOL_HOLYFIRE )       | ( int64_t( 1 ) << SCHOOL_STORMSTRIKE )  |
                                   ( int64_t( 1 ) << SCHOOL_HOLYSTORM )      | ( int64_t( 1 ) << SCHOOL_FIRESTORM )    |
                                   ( int64_t( 1 ) << SCHOOL_FROSTSTRIKE )    | ( int64_t( 1 ) << SCHOOL_HOLYFROST )    |
                                   ( int64_t( 1 ) << SCHOOL_FROSTSTORM )     | ( int64_t( 1 ) << SCHOOL_SHADOWSTRIKE ) |
                                   ( int64_t( 1 ) << SCHOOL_SHADOWLIGHT )    | ( int64_t( 1 ) << SCHOOL_SHADOWFLAME )  |
                                   ( int64_t( 1 ) << SCHOOL_SHADOWSTORM )    | ( int64_t( 1 ) << SCHOOL_SHADOWFROST )  |
                                   ( int64_t( 1 ) << SCHOOL_SPELLSTRIKE )    | ( int64_t( 1 ) << SCHOOL_DIVINE )       |
                                   ( int64_t( 1 ) << SCHOOL_SPELLFIRE )      | ( int64_t( 1 ) << SCHOOL_SPELLSTORM )   |
                                   ( int64_t( 1 ) << SCHOOL_SPELLFROST )     | ( int64_t( 1 ) << SCHOOL_SPELLSHADOW )  |
                                   ( int64_t( 1 ) << SCHOOL_ELEMENTAL )      | ( int64_t( 1 ) << SCHOOL_CHROMATIC )    |
                                   ( int64_t( 1 ) << SCHOOL_MAGIC ) );
#define SCHOOL_ALL_MASK    ( int64_t( -1 ) )

enum talent_tree_type
{
  TREE_NONE=0,
  TREE_BLOOD,         TREE_UNHOLY,                         // DEATH KNIGHT
  TREE_BALANCE,       TREE_FERAL,        TREE_RESTORATION, // DRUID
  TREE_BEAST_MASTERY, TREE_MARKSMANSHIP, TREE_SURVIVAL,    // HUNTER
  TREE_ARCANE,        TREE_FIRE,         TREE_FROST,       // MAGE
                                         TREE_RETRIBUTION, // PALADIN
  TREE_DISCIPLINE,    TREE_HOLY,         TREE_SHADOW,      // PRIEST
  TREE_ASSASSINATION, TREE_COMBAT,       TREE_SUBTLETY,    // ROGUE
  TREE_ELEMENTAL,     TREE_ENHANCEMENT,                    // SHAMAN
  TREE_AFFLICTION,    TREE_DEMONOLOGY,   TREE_DESTRUCTION, // WARLOCK
  TREE_ARMS,          TREE_FURY,         TREE_PROTECTION,  // WARRIOR
  TALENT_TREE_MAX
};

enum talent_tab_type
{
  TALENT_TAB_NONE = -1,
  DEATH_KNIGHT_BLOOD = 0,   DEATH_KNIGHT_FROST,  DEATH_KNIGHT_UNHOLY, // DEATH KNIGHT
  DRUID_BALANCE = 0,        DRUID_FERAL,         DRUID_RESTORATION,   // DRUID
  HUNTER_BEAST_MASTERY = 0, HUNTER_MARKSMANSHIP, HUNTER_SURVIVAL,     // HUNTER
  MAGE_ARCANE = 0,          MAGE_FIRE,           MAGE_FROST,          // MAGE
  PALADIN_HOLY = 0,         PALADIN_PROTECTION,  PALADIN_RETRIBUTION, // PALADIN
  PRIEST_DISCIPLINE = 0,    PRIEST_HOLY,         PRIEST_SHADOW,       // PRIEST
  ROGUE_ASSASSINATION = 0,  ROGUE_COMBAT,        ROGUE_SUBTLETY,      // ROGUE
  SHAMAN_ELEMENTAL = 0,     SHAMAN_ENHANCEMENT,  SHAMAN_RESTORATION,  // SHAMAN
  WARLOCK_AFFLICTION = 0,   WARLOCK_DEMONOLOGY,  WARLOCK_DESTRUCTION, // WARLOCK
  WARRIOR_ARMS = 0,         WARRIOR_FURY,        WARRIOR_PROTECTION,  // WARRIOR
  PET_TAB_FEROCITY = 0,     PET_TAB_TENACITY,    PET_TAB_CUNNING,     // PET
};

enum weapon_type
{
  WEAPON_NONE=0,
  WEAPON_DAGGER,                                                                                   WEAPON_SMALL,
  WEAPON_BEAST,    WEAPON_SWORD,    WEAPON_MACE,     WEAPON_AXE,    WEAPON_FIST,                   WEAPON_1H,
  WEAPON_BEAST_2H, WEAPON_SWORD_2H, WEAPON_MACE_2H,  WEAPON_AXE_2H, WEAPON_STAFF,  WEAPON_POLEARM, WEAPON_2H,
  WEAPON_BOW,      WEAPON_CROSSBOW, WEAPON_GUN,      WEAPON_WAND,   WEAPON_THROWN,                 WEAPON_RANGED,
  WEAPON_MAX
};

enum glyph_type
{
  GLYPH_MAJOR=0,
  GLYPH_MINOR,
  GLYPH_PRIME,
  GLYPH_MAX
};

enum slot_type   // these enum values match armory settings
{
  SLOT_NONE      = -1,
  SLOT_HEAD      = 0,
  SLOT_NECK      = 1,
  SLOT_SHOULDERS = 2,
  SLOT_SHIRT     = 3,
  SLOT_CHEST     = 4,
  SLOT_WAIST     = 5,
  SLOT_LEGS      = 6,
  SLOT_FEET      = 7,
  SLOT_WRISTS    = 8,
  SLOT_HANDS     = 9,
  SLOT_FINGER_1  = 10,
  SLOT_FINGER_2  = 11,
  SLOT_TRINKET_1 = 12,
  SLOT_TRINKET_2 = 13,
  SLOT_BACK      = 14,
  SLOT_MAIN_HAND = 15,
  SLOT_OFF_HAND  = 16,
  SLOT_RANGED    = 17,
  SLOT_TABARD    = 18,
  SLOT_MAX       = 19
};

// Tiers 11..14
#ifndef N_TIER
#define N_TIER 4
#endif

// Caster 2/4, Melee 2/4, Tank 2/4, Heal 2/4
#ifndef N_TIER_BONUS
#define N_TIER_BONUS 8
#endif

enum set_type
{
  SET_NONE = 0,
  SET_T11_CASTER, SET_T11_2PC_CASTER, SET_T11_4PC_CASTER,
  SET_T11_MELEE,  SET_T11_2PC_MELEE,  SET_T11_4PC_MELEE,
  SET_T11_TANK,   SET_T11_2PC_TANK,   SET_T11_4PC_TANK,
  SET_T11_HEAL,   SET_T11_2PC_HEAL,   SET_T11_4PC_HEAL,
  SET_T12_CASTER, SET_T12_2PC_CASTER, SET_T12_4PC_CASTER,
  SET_T12_MELEE,  SET_T12_2PC_MELEE,  SET_T12_4PC_MELEE,
  SET_T12_TANK,   SET_T12_2PC_TANK,   SET_T12_4PC_TANK,
  SET_T12_HEAL,   SET_T12_2PC_HEAL,   SET_T12_4PC_HEAL,
  SET_T13_CASTER, SET_T13_2PC_CASTER, SET_T13_4PC_CASTER,
  SET_T13_MELEE,  SET_T13_2PC_MELEE,  SET_T13_4PC_MELEE,
  SET_T13_TANK,   SET_T13_2PC_TANK,   SET_T13_4PC_TANK,
  SET_T13_HEAL,   SET_T13_2PC_HEAL,   SET_T13_4PC_HEAL,
  SET_T14_CASTER, SET_T14_2PC_CASTER, SET_T14_4PC_CASTER,
  SET_T14_MELEE,  SET_T14_2PC_MELEE,  SET_T14_4PC_MELEE,
  SET_T14_TANK,   SET_T14_2PC_TANK,   SET_T14_4PC_TANK,
  SET_T14_HEAL,   SET_T14_2PC_HEAL,   SET_T14_4PC_HEAL,
  SET_MAX
};

enum gem_type
{
  GEM_NONE=0,
  GEM_META, GEM_PRISMATIC,
  GEM_RED, GEM_YELLOW, GEM_BLUE,
  GEM_ORANGE, GEM_GREEN, GEM_PURPLE,
  GEM_COGWHEEL,
  GEM_MAX
};

enum meta_gem_type
{
  META_GEM_NONE=0,
  META_AGILE_SHADOWSPIRIT,
  META_AUSTERE_EARTHSIEGE,
  META_AUSTERE_SHADOWSPIRIT,
  META_BEAMING_EARTHSIEGE,
  META_BRACING_EARTHSIEGE,
  META_BRACING_EARTHSTORM,
  META_BRACING_SHADOWSPIRIT,
  META_BURNING_SHADOWSPIRIT,
  META_CHAOTIC_SHADOWSPIRIT,
  META_CHAOTIC_SKYFIRE,
  META_CHAOTIC_SKYFLARE,
  META_DESTRUCTIVE_SHADOWSPIRIT,
  META_DESTRUCTIVE_SKYFIRE,
  META_DESTRUCTIVE_SKYFLARE,
  META_EFFULGENT_SHADOWSPIRIT,
  META_EMBER_SHADOWSPIRIT,
  META_EMBER_SKYFIRE,
  META_EMBER_SKYFLARE,
  META_ENIGMATIC_SHADOWSPIRIT,
  META_ENIGMATIC_SKYFLARE,
  META_ENIGMATIC_STARFLARE,
  META_ENIGMATIC_SKYFIRE,
  META_ETERNAL_EARTHSIEGE,
  META_ETERNAL_EARTHSTORM,
  META_ETERNAL_SHADOWSPIRIT,
  META_FLEET_SHADOWSPIRIT,
  META_FORLORN_SHADOWSPIRIT,
  META_FORLORN_SKYFLARE,
  META_FORLORN_STARFLARE,
  META_IMPASSIVE_SHADOWSPIRIT,
  META_IMPASSIVE_SKYFLARE,
  META_IMPASSIVE_STARFLARE,
  META_INSIGHTFUL_EARTHSIEGE,
  META_INSIGHTFUL_EARTHSTORM,
  META_INVIGORATING_EARTHSIEGE,
  META_MYSTICAL_SKYFIRE,
  META_PERSISTENT_EARTHSIEGE,
  META_PERSISTENT_EARTHSHATTER,
  META_POWERFUL_EARTHSIEGE,
  META_POWERFUL_EARTHSHATTER,
  META_POWERFUL_EARTHSTORM,
  META_POWERFUL_SHADOWSPIRIT,
  META_RELENTLESS_EARTHSIEGE,
  META_RELENTLESS_EARTHSTORM,
  META_REVERBERATING_SHADOWSPIRIT,
  META_REVITALIZING_SHADOWSPIRIT,
  META_REVITALIZING_SKYFLARE,
  META_SWIFT_SKYFIRE,
  META_SWIFT_SKYFLARE,
  META_SWIFT_STARFIRE,
  META_SWIFT_STARFLARE,
  META_THUNDERING_SKYFIRE,
  META_THUNDERING_SKYFLARE,
  META_TIRELESS_STARFLARE,
  META_TIRELESS_SKYFLARE,
  META_TRENCHANT_EARTHSIEGE,
  META_TRENCHANT_EARTHSHATTER,
  META_GEM_MAX
};

enum stat_type
{
  STAT_NONE=0,
  STAT_STRENGTH, STAT_AGILITY, STAT_STAMINA, STAT_INTELLECT, STAT_SPIRIT,
  STAT_HEALTH, STAT_MANA, STAT_RAGE, STAT_ENERGY, STAT_FOCUS, STAT_RUNIC,
  STAT_MAX_HEALTH, STAT_MAX_MANA, STAT_MAX_RAGE, STAT_MAX_ENERGY, STAT_MAX_FOCUS, STAT_MAX_RUNIC,
  STAT_SPELL_POWER, STAT_SPELL_PENETRATION, STAT_MP5,
  STAT_ATTACK_POWER, STAT_EXPERTISE_RATING,
  STAT_HIT_RATING, STAT_CRIT_RATING, STAT_HASTE_RATING,STAT_MASTERY_RATING,
  STAT_WEAPON_DPS, STAT_WEAPON_SPEED,
  STAT_WEAPON_OFFHAND_DPS, STAT_WEAPON_OFFHAND_SPEED,
  STAT_ARMOR, STAT_BONUS_ARMOR, STAT_RESILIENCE_RATING, STAT_DODGE_RATING, STAT_PARRY_RATING,
  STAT_BLOCK_RATING,
  STAT_MAX
};

enum elixir_type
{
  ELIXIR_NONE=0,
  ELIXIR_DRAENIC_WISDOM,
  ELIXIR_MAJOR_MAGEBLOOD,
  ELIXIR_GUARDIAN,
  ELIXIR_ADEPT,
  ELIXIR_FEL_STRENGTH,
  ELIXIR_GREATER_ARCANE,
  ELIXIR_MAJOR_AGILITY,
  ELIXIR_MAJOR_FIRE_POWER,
  ELIXIR_MAJOR_FROST_POWER,
  ELIXIR_MAJOR_SHADOW_POWER,
  ELIXIR_MAJOR_STRENGTH,
  ELIXIR_MASTERY,
  ELIXIR_MONGOOSE,
  ELIXIR_ONSLAUGHT,
  ELIXIR_SAGES,
  ELIXIR_BATTLE,
  ELIXIR_MAX
};

enum flask_type
{
  FLASK_NONE=0,
  FLASK_BLINDING_LIGHT,
  FLASK_DISTILLED_WISDOM,
  FLASK_DRACONIC_MIND,
  FLASK_ENDLESS_RAGE,
  FLASK_ENHANCEMENT,
  FLASK_FLOWING_WATER,
  FLASK_FROST_WYRM,
  FLASK_MIGHTY_RESTORATION,
  FLASK_NORTH,
  FLASK_PURE_DEATH,
  FLASK_PURE_MOJO,
  FLASK_RELENTLESS_ASSAULT,
  FLASK_SUPREME_POWER,
  FLASK_STEELSKIN,
  FLASK_TITANIC_STRENGTH,
  FLASK_WINDS,
  FLASK_MAX
};

enum food_type
{
  FOOD_NONE=0,
  FOOD_BAKED_ROCKFISH,
  FOOD_BASILISK_LIVERDOG,
  FOOD_BEER_BASTED_CROCOLISK,
  FOOD_BLACKBELLY_SUSHI,
  FOOD_BLACKENED_BASILISK,
  FOOD_BLACKENED_DRAGONFIN,
  FOOD_CROCOLISK_AU_GRATIN,
  FOOD_CRUNCHY_SERPENT,
  FOOD_DELICIOUS_SAGEFISH_TAIL,
  FOOD_DRAGONFIN_FILET,
  FOOD_FISH_FEAST,
  FOOD_FORTUNE_COOKIE,
  FOOD_GOLDEN_FISHSTICKS,
  FOOD_GREAT_FEAST,
  FOOD_GRILLED_DRAGON,
  FOOD_HEARTY_RHINO,
  FOOD_IMPERIAL_MANTA_STEAK,
  FOOD_LAVASCALE_FILLET,
  FOOD_MEGA_MAMMOTH_MEAL,
  FOOD_MUSHROOM_SAUCE_MUDFISH,
  FOOD_POACHED_BLUEFISH,
  FOOD_POACHED_NORTHERN_SCULPIN,
  FOOD_RHINOLICIOUS_WORMSTEAK,
  FOOD_SEAFOOD_MAGNIFIQUE_FEAST,
  FOOD_SEVERED_SAGEFISH_HEAD,
  FOOD_SKEWERED_EEL,
  FOOD_SMOKED_SALMON,
  FOOD_SNAPPER_EXTREME,
  FOOD_TENDER_SHOVELTUSK_STEAK,
  FOOD_VERY_BURNT_WORG,
  FOOD_MAX
};

enum position_type { POSITION_NONE=0, POSITION_FRONT, POSITION_BACK, POSITION_RANGED_FRONT, POSITION_RANGED_BACK, POSITION_MAX };

enum profession_type
{
  PROFESSION_NONE=0,
  PROF_ALCHEMY,
  PROF_MINING,
  PROF_HERBALISM,
  PROF_LEATHERWORKING,
  PROF_ENGINEERING,
  PROF_BLACKSMITHING,
  PROF_INSCRIPTION,
  PROF_SKINNING,
  PROF_TAILORING,
  PROF_JEWELCRAFTING,
  PROF_ENCHANTING,
  PROFESSION_MAX
};

enum role_type { ROLE_NONE=0, ROLE_ATTACK, ROLE_SPELL, ROLE_HYBRID, ROLE_DPS, ROLE_TANK, ROLE_HEAL, ROLE_MAX };

enum rng_type
{
  // Specifies the general class of RNG desired
  RNG_DEFAULT=0,     // Do not care/know where it will be used
  RNG_GLOBAL,        // Returns reference to global RNG on sim_t
  RNG_DETERMINISTIC, // Returns reference to global deterministic RNG on sim_t
  RNG_CYCLIC,        // Normalized even/periodical results are acceptable
  RNG_DISTRIBUTED,   // Normalized variable/distributed values should be returned

  // Specifies a particular RNG desired
  RNG_STANDARD,          // Creates RNG using srand() and rand()
  RNG_MERSENNE_TWISTER,  // Creates RNG using SIMD oriented Fast Mersenne Twister
  RNG_PHASE_SHIFT,       // Simplistic cycle-based RNG, unsuitable for overlapping procs
  RNG_DISTANCE_SIMPLE,   // Simple normalized proc-separation RNG, suitable for fixed proc chance
  RNG_DISTANCE_BANDS,    // Complex normalized proc-separation RNG, suitable for varying proc chance
  RNG_PRE_FILL,          // Deterministic number of procs with random distribution
  RNG_MAX
};

enum save_type
{
  // Specifies the type of profile data to be saved
  SAVE_ALL=0,
  SAVE_GEAR,
  SAVE_TALENTS,
  SAVE_ACTIONS,
  SAVE_MAX
};

enum format_type
{
  FORMAT_NONE=0,
  FORMAT_NAME,
  FORMAT_CHAR_NAME,
  FORMAT_CONVERT_HEX,
  FORMAT_CONVERT_UTF8,
  FORMAT_MAX
};

#define FORMAT_CHAR_NAME_MASK  ( (1<<FORMAT_NAME) | (1<<FORMAT_CHAR_NAME) )
#define FORMAT_GUILD_NAME_MASK ( (1<<FORMAT_NAME) )
#define FORMAT_ALL_NAME_MASK   ( (1<<FORMAT_NAME) | (1<<FORMAT_CHAR_NAME) )
#define FORMAT_UTF8_MASK       ( (1<<FORMAT_CONVERT_HEX) | (1<<FORMAT_CONVERT_UTF8) )
#define FORMAT_ASCII_MASK      ( (1<<FORMAT_CONVERT_UTF8) )
#define FORMAT_CONVERT_MASK    ( (1<<FORMAT_CONVERT_HEX) | (1<<FORMAT_CONVERT_UTF8) )
#define FORMAT_DEFAULT         ( FORMAT_ASCII_MASK )
#define FORMAT_ALL_MASK        -1

// Data Access ====================================================
#ifndef MAX_LEVEL
#define MAX_LEVEL (85)
#endif

#ifndef MAX_RANK
#define MAX_RANK (3)
#endif

#ifndef NUM_SPELL_FLAGS
#define NUM_SPELL_FLAGS (10)
#endif

#ifndef MAX_EFFECTS
#define MAX_EFFECTS (3)
#endif

#ifndef MAX_TALENT_TABS
#define MAX_TALENT_TABS (3)
#endif

#ifndef MAX_TALENTS
#define MAX_TALENTS (100)
#endif

enum power_type
{
  POWER_MANA = 0,
  POWER_RAGE = 1,
  POWER_FOCUS = 2,
  POWER_ENERGY = 3,
  POWER_HAPPINESS = 4,
  POWER_RUNE = 5,
  POWER_RUNIC_POWER = 6,
  POWER_SOUL_SHARDS = 7,
  POWER_ECLIPSE = 8,
  POWER_HOLY_POWER = 9,
  POWER_HEALTH = 0xFFFFFFFE, // (or -2 if signed)
  POWER_NONE = 0xFFFFFFFF // None.
};

enum rating_type {
  RATING_DODGE = 0,
  RATING_PARRY,
  RATING_BLOCK,
  RATING_MELEE_HIT,
  RATING_RANGED_HIT,
  RATING_SPELL_HIT,
  RATING_MELEE_CRIT,
  RATING_RANGED_CRIT,
  RATING_SPELL_CRIT,
  RATING_MELEE_HASTE,
  RATING_RANGED_HASTE,
  RATING_SPELL_HASTE,
  RATING_EXPERTISE,
  RATING_MASTERY,
  RATING_MAX
};

struct stat_data_t {
  double      strength;
  double      agility;
  double      stamina;
  double      intellect;
  double      spirit;
  double      base_health;
  double      base_resource;
};

#define DEFAULT_MISC_VALUE std::numeric_limits<int>::min()

// These names come from the MANGOS project.
enum spell_attribute_t {
 SPELL_ATTR_UNK0 = 0x0000, // 0
 SPELL_ATTR_RANGED, // 1 All ranged abilites have this flag
 SPELL_ATTR_ON_NEXT_SWING_1, // 2 on next swing
 SPELL_ATTR_UNK3, // 3 not set in 3.0.3
 SPELL_ATTR_UNK4, // 4 isAbility
 SPELL_ATTR_TRADESPELL, // 5 trade spells, will be added by client to a sublist of profession spell
 SPELL_ATTR_PASSIVE, // 6 Passive spell
 SPELL_ATTR_UNK7, // 7 can't be linked in chat?
 SPELL_ATTR_UNK8, // 8 hide created item in tooltip (for effect=24)
 SPELL_ATTR_UNK9, // 9
 SPELL_ATTR_ON_NEXT_SWING_2, // 10 on next swing 2
 SPELL_ATTR_UNK11, // 11
 SPELL_ATTR_DAYTIME_ONLY, // 12 only useable at daytime, not set in 2.4.2
 SPELL_ATTR_NIGHT_ONLY, // 13 only useable at night, not set in 2.4.2
 SPELL_ATTR_INDOORS_ONLY, // 14 only useable indoors, not set in 2.4.2
 SPELL_ATTR_OUTDOORS_ONLY, // 15 Only useable outdoors.
 SPELL_ATTR_NOT_SHAPESHIFT, // 16 Not while shapeshifted
 SPELL_ATTR_ONLY_STEALTHED, // 17 Must be in stealth
 SPELL_ATTR_UNK18, // 18
 SPELL_ATTR_LEVEL_DAMAGE_CALCULATION, // 19 spelldamage depends on caster level
 SPELL_ATTR_STOP_ATTACK_TARGE, // 20 Stop attack after use this spell (and not begin attack if use)
 SPELL_ATTR_IMPOSSIBLE_DODGE_PARRY_BLOCK, // 21 Cannot be dodged/parried/blocked
 SPELL_ATTR_UNK22, // 22
 SPELL_ATTR_UNK23, // 23 castable while dead?
 SPELL_ATTR_CASTABLE_WHILE_MOUNTED, // 24 castable while mounted
 SPELL_ATTR_DISABLED_WHILE_ACTIVE, // 25 Activate and start cooldown after aura fade or remove summoned creature or go
 SPELL_ATTR_UNK26, // 26
 SPELL_ATTR_CASTABLE_WHILE_SITTING, // 27 castable while sitting
 SPELL_ATTR_CANT_USED_IN_COMBAT, // 28 Cannot be used in combat
 SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY, // 29 unaffected by invulnerability (hmm possible not...)
 SPELL_ATTR_UNK30, // 30 breakable by damage?
 SPELL_ATTR_CANT_CANCEL, // 31 positive aura can't be canceled

 SPELL_ATTR_EX_UNK0 = 0x0100, // 0
 SPELL_ATTR_EX_DRAIN_ALL_POWER, // 1 use all power (Only paladin Lay of Hands and Bunyanize)
 SPELL_ATTR_EX_CHANNELED_1, // 2 channeled 1
 SPELL_ATTR_EX_UNK3, // 3
 SPELL_ATTR_EX_UNK4, // 4
 SPELL_ATTR_EX_NOT_BREAK_STEALTH, // 5 Not break stealth
 SPELL_ATTR_EX_CHANNELED_2, // 6 channeled 2
 SPELL_ATTR_EX_NEGATIVE, // 7
 SPELL_ATTR_EX_NOT_IN_COMBAT_TARGET, // 8 Spell req target not to be in combat state
 SPELL_ATTR_EX_UNK9, // 9
 SPELL_ATTR_EX_NO_INITIAL_AGGRO, // 10 no generates threat on cast 100%
 SPELL_ATTR_EX_UNK11, // 11
 SPELL_ATTR_EX_UNK12, // 12
 SPELL_ATTR_EX_UNK13, // 13
 SPELL_ATTR_EX_UNK14, // 14
 SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY, // 15 remove auras on immunity
 SPELL_ATTR_EX_UNAFFECTED_BY_SCHOOL_IMMUNE, // 16 unaffected by school immunity
 SPELL_ATTR_EX_UNK17, // 17 for auras AURA_TRACK_CREATURES, AURA_TRACK_RESOURCES and AURA_TRACK_STEALTHED select non-stacking tracking spells
 SPELL_ATTR_EX_UNK18, // 18
 SPELL_ATTR_EX_UNK19, // 19
 SPELL_ATTR_EX_REQ_COMBO_POINTS1, // 20 Req combo points on target
 SPELL_ATTR_EX_UNK21, // 21
 SPELL_ATTR_EX_REQ_COMBO_POINTS2, // 22 Req combo points on target
 SPELL_ATTR_EX_UNK23, // 23
 SPELL_ATTR_EX_UNK24, // 24 Req fishing pole??
 SPELL_ATTR_EX_UNK25, // 25
 SPELL_ATTR_EX_UNK26, // 26
 SPELL_ATTR_EX_UNK27, // 27
 SPELL_ATTR_EX_UNK28, // 28
 SPELL_ATTR_EX_UNK29, // 29
 SPELL_ATTR_EX_UNK30, // 30 overpower
 SPELL_ATTR_EX_UNK31, // 31

 SPELL_ATTR_EX2_UNK0 = 0x0200, // 0
 SPELL_ATTR_EX2_UNK1, // 1
 SPELL_ATTR_EX2_CANT_REFLECTED, // 2 ? used for detect can or not spell reflected // do not need LOS (e.g. 18220 since 3.3.3)
 SPELL_ATTR_EX2_UNK3, // 3 auto targeting? (e.g. fishing skill enhancement items since 3.3.3)
 SPELL_ATTR_EX2_UNK4, // 4
 SPELL_ATTR_EX2_AUTOREPEAT_FLAG, // 5
 SPELL_ATTR_EX2_UNK6, // 6 only usable on tabbed by yourself
 SPELL_ATTR_EX2_UNK7, // 7
 SPELL_ATTR_EX2_UNK8, // 8 not set in 3.0.3
 SPELL_ATTR_EX2_UNK9, // 9
 SPELL_ATTR_EX2_UNK10, // 10
 SPELL_ATTR_EX2_HEALTH_FUNNEL, // 11
 SPELL_ATTR_EX2_UNK12, // 12
 SPELL_ATTR_EX2_UNK13, // 13
 SPELL_ATTR_EX2_UNK14, // 14
 SPELL_ATTR_EX2_UNK15, // 15 not set in 3.0.3
 SPELL_ATTR_EX2_UNK16, // 16
 SPELL_ATTR_EX2_UNK17, // 17 suspend weapon timer instead of resetting it, (?Hunters Shot and Stings only have this flag?)
 SPELL_ATTR_EX2_UNK18, // 18 Only Revive pet - possible req dead pet
 SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT, // 19 does not necessarly need shapeshift
 SPELL_ATTR_EX2_UNK20, // 20
 SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD, // 21 for ice blocks, pala immunity buffs, priest absorb shields, but used also for other spells -> not sure!
 SPELL_ATTR_EX2_UNK22, // 22
 SPELL_ATTR_EX2_UNK23, // 23 Only mage Arcane Concentration have this flag
 SPELL_ATTR_EX2_UNK24, // 24
 SPELL_ATTR_EX2_UNK25, // 25
 SPELL_ATTR_EX2_UNK26, // 26 unaffected by school immunity
 SPELL_ATTR_EX2_UNK27, // 27
 SPELL_ATTR_EX2_UNK28, // 28 no breaks stealth if it fails??
 SPELL_ATTR_EX2_CANT_CRIT, // 29 Spell can't crit
 SPELL_ATTR_EX2_UNK30, // 30
 SPELL_ATTR_EX2_FOOD_BUFF, // 31 Food or Drink Buff (like Well Fed)

 SPELL_ATTR_EX3_UNK0 = 0x0300, // 0
 SPELL_ATTR_EX3_UNK1, // 1
 SPELL_ATTR_EX3_UNK2, // 2
 SPELL_ATTR_EX3_UNK3, // 3
 SPELL_ATTR_EX3_UNK4, // 4 Druid Rebirth only this spell have this flag
 SPELL_ATTR_EX3_UNK5, // 5
 SPELL_ATTR_EX3_UNK6, // 6
 SPELL_ATTR_EX3_UNK7, // 7 create a separate (de)buff stack for each caster
 SPELL_ATTR_EX3_UNK8, // 8
 SPELL_ATTR_EX3_UNK9, // 9
 SPELL_ATTR_EX3_MAIN_HAND, // 10 Main hand weapon required
 SPELL_ATTR_EX3_BATTLEGROUND, // 11 Can casted only on battleground
 SPELL_ATTR_EX3_CAST_ON_DEAD, // 12 target is a dead player (not every spell has this flag)
 SPELL_ATTR_EX3_UNK13, // 13
 SPELL_ATTR_EX3_UNK14, // 14 "Honorless Target" only this spells have this flag
 SPELL_ATTR_EX3_UNK15, // 15 Auto Shoot, Shoot, Throw, - this is autoshot flag
 SPELL_ATTR_EX3_UNK16, // 16 no triggers effects that trigger on casting a spell??
 SPELL_ATTR_EX3_UNK17, // 17 no triggers effects that trigger on casting a spell??
 SPELL_ATTR_EX3_UNK18, // 18
 SPELL_ATTR_EX3_UNK19, // 19
 SPELL_ATTR_EX3_DEATH_PERSISTENT, // 20 Death persistent spells
 SPELL_ATTR_EX3_UNK21, // 21
 SPELL_ATTR_EX3_REQ_WAND, // 22 Req wand
 SPELL_ATTR_EX3_UNK23, // 23
 SPELL_ATTR_EX3_REQ_OFFHAND, // 24 Req offhand weapon
 SPELL_ATTR_EX3_UNK25, // 25 no cause spell pushback ?
 SPELL_ATTR_EX3_UNK26, // 26
 SPELL_ATTR_EX3_UNK27, // 27
 SPELL_ATTR_EX3_UNK28, // 28
 SPELL_ATTR_EX3_UNK29, // 29
 SPELL_ATTR_EX3_UNK30, // 30
 SPELL_ATTR_EX3_UNK31, // 31

 SPELL_ATTR_EX4_UNK0 = 0x0400, // 0
 SPELL_ATTR_EX4_UNK1, // 1 proc on finishing move?
 SPELL_ATTR_EX4_UNK2, // 2
 SPELL_ATTR_EX4_UNK3, // 3
 SPELL_ATTR_EX4_UNK4, // 4 This will no longer cause guards to attack on use??
 SPELL_ATTR_EX4_UNK5, // 5
 SPELL_ATTR_EX4_NOT_STEALABLE, // 6 although such auras might be dispellable, they cannot be stolen
 SPELL_ATTR_EX4_UNK7, // 7
 SPELL_ATTR_EX4_STACK_DOT_MODIFIER, // 8 no effect on non DoTs?
 SPELL_ATTR_EX4_UNK9, // 9
 SPELL_ATTR_EX4_SPELL_VS_EXTEND_COST, // 10 Rogue Shiv have this flag
 SPELL_ATTR_EX4_UNK11, // 11
 SPELL_ATTR_EX4_UNK12, // 12
 SPELL_ATTR_EX4_UNK13, // 13
 SPELL_ATTR_EX4_UNK14, // 14
 SPELL_ATTR_EX4_UNK15, // 15
 SPELL_ATTR_EX4_NOT_USABLE_IN_ARENA, // 16 not usable in arena
 SPELL_ATTR_EX4_USABLE_IN_ARENA, // 17 usable in arena
 SPELL_ATTR_EX4_UNK18, // 18
 SPELL_ATTR_EX4_UNK19, // 19
 SPELL_ATTR_EX4_UNK20, // 20 do not give "more powerful spell" error message
 SPELL_ATTR_EX4_UNK21, // 21
 SPELL_ATTR_EX4_UNK22, // 22
 SPELL_ATTR_EX4_UNK23, // 23
 SPELL_ATTR_EX4_UNK24, // 24
 SPELL_ATTR_EX4_UNK25, // 25 pet scaling auras
 SPELL_ATTR_EX4_CAST_ONLY_IN_OUTLAND, // 26 Can only be used in Outland.
 SPELL_ATTR_EX4_UNK27, // 27
 SPELL_ATTR_EX4_UNK28, // 28
 SPELL_ATTR_EX4_UNK29, // 29
 SPELL_ATTR_EX4_UNK30, // 30
 SPELL_ATTR_EX4_UNK31, // 31

 SPELL_ATTR_EX5_UNK0 = 0x0500, // 0
 SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP, // 1 not need reagents if UNIT_FLAG_PREPARATION
 SPELL_ATTR_EX5_UNK2, // 2 removed at enter arena (e.g. 31850 since 3.3.3)
 SPELL_ATTR_EX5_USABLE_WHILE_STUNNED, // 3 usable while stunned
 SPELL_ATTR_EX5_UNK4, // 4
 SPELL_ATTR_EX5_SINGLE_TARGET_SPELL, // 5 Only one target can be apply at a time
 SPELL_ATTR_EX5_UNK6, // 6
 SPELL_ATTR_EX5_UNK7, // 7
 SPELL_ATTR_EX5_UNK8, // 8
 SPELL_ATTR_EX5_START_PERIODIC_AT_APPLY, // 9 begin periodic tick at aura apply
 SPELL_ATTR_EX5_UNK10, // 10
 SPELL_ATTR_EX5_UNK11, // 11
 SPELL_ATTR_EX5_UNK12, // 12
 SPELL_ATTR_EX5_UNK13, // 13 haste affects duration (e.g. 8050 since 3.3.3)
 SPELL_ATTR_EX5_UNK14, // 14
 SPELL_ATTR_EX5_UNK15, // 15
 SPELL_ATTR_EX5_UNK16, // 16
 SPELL_ATTR_EX5_USABLE_WHILE_FEARED, // 17 usable while feared
 SPELL_ATTR_EX5_USABLE_WHILE_CONFUSED, // 18 usable while confused
 SPELL_ATTR_EX5_UNK19, // 19
 SPELL_ATTR_EX5_UNK20, // 20
 SPELL_ATTR_EX5_UNK21, // 21
 SPELL_ATTR_EX5_UNK22, // 22
 SPELL_ATTR_EX5_UNK23, // 23
 SPELL_ATTR_EX5_UNK24, // 24
 SPELL_ATTR_EX5_UNK25, // 25
 SPELL_ATTR_EX5_UNK26, // 26
 SPELL_ATTR_EX5_UNK27, // 27
 SPELL_ATTR_EX5_UNK28, // 28
 SPELL_ATTR_EX5_UNK29, // 29
 SPELL_ATTR_EX5_UNK30, // 30
 SPELL_ATTR_EX5_UNK31, // 31 Forces all nearby enemies to focus attacks caster

 SPELL_ATTR_EX6_UNK0 = 0x0600, // 0 Only Move spell have this flag
 SPELL_ATTR_EX6_ONLY_IN_ARENA, // 1 only usable in arena, not used in 3.2.0a and early
 SPELL_ATTR_EX6_UNK2, // 2
 SPELL_ATTR_EX6_UNK3, // 3
 SPELL_ATTR_EX6_UNK4, // 4
 SPELL_ATTR_EX6_UNK5, // 5
 SPELL_ATTR_EX6_UNK6, // 6
 SPELL_ATTR_EX6_UNK7, // 7
 SPELL_ATTR_EX6_UNK8, // 8
 SPELL_ATTR_EX6_UNK9, // 9
 SPELL_ATTR_EX6_UNK10, // 10
 SPELL_ATTR_EX6_NOT_IN_RAID_INSTANCE, // 11 not usable in raid instance
 SPELL_ATTR_EX6_UNK12, // 12 for auras AURA_TRACK_CREATURES, AURA_TRACK_RESOURCES and AURA_TRACK_STEALTHED select non-stacking tracking spells
 SPELL_ATTR_EX6_UNK13, // 13
 SPELL_ATTR_EX6_UNK14, // 14
 SPELL_ATTR_EX6_UNK15, // 15 not set in 3.0.3
 SPELL_ATTR_EX6_UNK16, // 16
 SPELL_ATTR_EX6_UNK17, // 17
 SPELL_ATTR_EX6_UNK18, // 18
 SPELL_ATTR_EX6_UNK19, // 19
 SPELL_ATTR_EX6_UNK20, // 20
 SPELL_ATTR_EX6_UNK21, // 21
 SPELL_ATTR_EX6_UNK22, // 22
 SPELL_ATTR_EX6_UNK23, // 23 not set in 3.0.3
 SPELL_ATTR_EX6_UNK24, // 24 not set in 3.0.3
 SPELL_ATTR_EX6_UNK25, // 25 not set in 3.0.3
 SPELL_ATTR_EX6_UNK26, // 26 not set in 3.0.3
 SPELL_ATTR_EX6_UNK27, // 27 not set in 3.0.3
 SPELL_ATTR_EX6_UNK28, // 28 not set in 3.0.3
 SPELL_ATTR_EX6_NO_DMG_PERCENT_MODS, // 29 do not apply damage percent mods (usually in cases where it has already been applied)
 SPELL_ATTR_EX6_UNK30, // 30 not set in 3.0.3
 SPELL_ATTR_EX6_UNK31, // 31 not set in 3.0.3

 SPELL_ATTR_EX7_UNK0 = 0x0700, // 0
 SPELL_ATTR_EX7_UNK1, // 1
 SPELL_ATTR_EX7_UNK2, // 2
 SPELL_ATTR_EX7_UNK3, // 3
 SPELL_ATTR_EX7_UNK4, // 4
 SPELL_ATTR_EX7_UNK5, // 5
 SPELL_ATTR_EX7_UNK6, // 6
 SPELL_ATTR_EX7_UNK7, // 7
 SPELL_ATTR_EX7_UNK8, // 8
 SPELL_ATTR_EX7_UNK9, // 9
 SPELL_ATTR_EX7_UNK10, // 10
 SPELL_ATTR_EX7_UNK11, // 11
 SPELL_ATTR_EX7_UNK12, // 12
 SPELL_ATTR_EX7_UNK13, // 13
 SPELL_ATTR_EX7_UNK14, // 14
 SPELL_ATTR_EX7_UNK15, // 15
 SPELL_ATTR_EX7_UNK16, // 16
 SPELL_ATTR_EX7_UNK17, // 17
 SPELL_ATTR_EX7_UNK18, // 18
 SPELL_ATTR_EX7_UNK19, // 19
 SPELL_ATTR_EX7_UNK20, // 20
 SPELL_ATTR_EX7_UNK21, // 21
 SPELL_ATTR_EX7_UNK22, // 22
 SPELL_ATTR_EX7_UNK23, // 23
 SPELL_ATTR_EX7_UNK24, // 24
 SPELL_ATTR_EX7_UNK25, // 25
 SPELL_ATTR_EX7_UNK26, // 26
 SPELL_ATTR_EX7_UNK27, // 27
 SPELL_ATTR_EX7_UNK28, // 28
 SPELL_ATTR_EX7_UNK29, // 29
 SPELL_ATTR_EX7_UNK30, // 30
 SPELL_ATTR_EX7_UNK31, // 31

 SPELL_ATTR_EX8_UNK0 = 0x0800, // 0
 SPELL_ATTR_EX8_UNK1, // 1
 SPELL_ATTR_EX8_UNK2, // 2
 SPELL_ATTR_EX8_UNK3, // 3
 SPELL_ATTR_EX8_UNK4, // 4
 SPELL_ATTR_EX8_UNK5, // 5
 SPELL_ATTR_EX8_UNK6, // 6
 SPELL_ATTR_EX8_UNK7, // 7
 SPELL_ATTR_EX8_UNK8, // 8
 SPELL_ATTR_EX8_UNK9, // 9
 SPELL_ATTR_EX8_UNK10, // 10
 SPELL_ATTR_EX8_UNK11, // 11
 SPELL_ATTR_EX8_UNK12, // 12
 SPELL_ATTR_EX8_UNK13, // 13
 SPELL_ATTR_EX8_UNK14, // 14
 SPELL_ATTR_EX8_UNK15, // 15
 SPELL_ATTR_EX8_UNK16, // 16
 SPELL_ATTR_EX8_UNK17, // 17
 SPELL_ATTR_EX8_UNK18, // 18
 SPELL_ATTR_EX8_UNK19, // 19
 SPELL_ATTR_EX8_UNK20, // 20
 SPELL_ATTR_EX8_UNK21, // 21
 SPELL_ATTR_EX8_UNK22, // 22
 SPELL_ATTR_EX8_UNK23, // 23
 SPELL_ATTR_EX8_UNK24, // 24
 SPELL_ATTR_EX8_UNK25, // 25
 SPELL_ATTR_EX8_UNK26, // 26
 SPELL_ATTR_EX8_UNK27, // 27
 SPELL_ATTR_EX8_UNK28, // 28
 SPELL_ATTR_EX8_UNK29, // 29
 SPELL_ATTR_EX8_UNK30, // 30
 SPELL_ATTR_EX8_UNK31, // 31

 SPELL_ATTR_EX9_UNK0 = 0x0900, // 0
 SPELL_ATTR_EX9_UNK1, // 1
 SPELL_ATTR_EX9_UNK2, // 2
 SPELL_ATTR_EX9_UNK3, // 3
 SPELL_ATTR_EX9_UNK4, // 4
 SPELL_ATTR_EX9_UNK5, // 5
 SPELL_ATTR_EX9_UNK6, // 6
 SPELL_ATTR_EX9_UNK7, // 7
 SPELL_ATTR_EX9_UNK8, // 8
 SPELL_ATTR_EX9_UNK9, // 9
 SPELL_ATTR_EX9_UNK10, // 10
 SPELL_ATTR_EX9_UNK11, // 11
 SPELL_ATTR_EX9_UNK12, // 12
 SPELL_ATTR_EX9_UNK13, // 13
 SPELL_ATTR_EX9_UNK14, // 14
 SPELL_ATTR_EX9_UNK15, // 15
 SPELL_ATTR_EX9_UNK16, // 16
 SPELL_ATTR_EX9_UNK17, // 17
 SPELL_ATTR_EX9_UNK18, // 18
 SPELL_ATTR_EX9_UNK19, // 19
 SPELL_ATTR_EX9_UNK20, // 20
 SPELL_ATTR_EX9_UNK21, // 21
 SPELL_ATTR_EX9_UNK22, // 22
 SPELL_ATTR_EX9_UNK23, // 23
 SPELL_ATTR_EX9_UNK24, // 24
 SPELL_ATTR_EX9_UNK25, // 25
 SPELL_ATTR_EX9_UNK26, // 26
 SPELL_ATTR_EX9_UNK27, // 27
 SPELL_ATTR_EX9_UNK28, // 28
 SPELL_ATTR_EX9_UNK29, // 29
 SPELL_ATTR_EX9_UNK30, // 30
 SPELL_ATTR_EX9_UNK31, // 31
};

// DBC related classes ========================================================

struct spell_data_t {
  const char * _name;               // Spell name from Spell.dbc stringblock (enGB)
  unsigned     _id;                 // Spell ID in dbc
  unsigned     _flags;              // Unused for now, 0x00 for all
  double       _prj_speed;          // Projectile Speed
  unsigned     _school;             // Spell school mask
  int          _power_type;         // Resource type
  unsigned     _class_mask;         // Class mask for spell
  unsigned     _race_mask;          // Racial mask for the spell
  int          _scaling_type;       // Array index for gtSpellScaling.dbc. -1 means the last sub-array, 0 disabled
  double       _extra_coeff;        // An "extra" coefficient (used for some spells to indicate AP based coefficient)
  // SpellLevels.dbc
  unsigned     _spell_level;        // Spell learned on level. NOTE: Only accurate for "class abilities"
  unsigned     _max_level;          // Maximum level for scaling
  // SpellRange.dbc
  double       _min_range;          // Minimum range in yards
  double       _max_range;          // Maximum range in yards
  // SpellCooldown.dbc
  unsigned     _cooldown;           // Cooldown in milliseconds
  unsigned     _gcd;                // GCD in milliseconds
  // SpellCategories.dbc
  unsigned     _category;           // Spell category (for shared cooldowns, effects?)
  // SpellDuration.dbc
  double       _duration;           // Spell duration in milliseconds
  // SpellPower.dbc
  unsigned     _cost;               // Resource cost
  // SpellRuneCost.dbc
  unsigned     _rune_cost;          // Bitmask of rune cost 0x1, 0x2 = Blood | 0x4, 0x8 = Unholy | 0x10, 0x20 = Frost
  unsigned     _runic_power_gain;   // Amount of runic power gained ( / 10 )
  // SpellAuraOptions.dbc
  unsigned     _max_stack;          // Maximum stack size for spell
  unsigned     _proc_chance;        // Spell proc chance in percent
  unsigned     _proc_charges;       // Per proc charge amount
  // SpellEquippedItems.dbc
  unsigned     _equipped_class;
  unsigned     _equipped_invtype_mask;
  unsigned     _equipped_subclass_mask;
  // SpellScaling.dbc
  int          _cast_min;           // Minimum casting time in milliseconds
  int          _cast_max;           // Maximum casting time in milliseconds
  int          _cast_div;           // A divisor used in the formula for casting time scaling (20 always?)
  double       _c_scaling;          // A scaling multiplier for level based scaling
  unsigned     _c_scaling_level;    // A scaling divisor for level based scaling
  // SpellEffect.dbc
  unsigned     _effect[3];          // Effect identifiers
  // Spell.dbc flags
  unsigned     _attributes[10];     // Spell.dbc "flags", record field 1..10, note that 12694 added a field here after flags_7
  const char * _desc;               // Spell.dbc description stringblock
  const char * _tooltip;            // Spell.dbc tooltip stringblock
  // SpellDescriptionVariables.dbc
  const char * _desc_vars;          // Spell description variable stringblock, if present
  // SpellIcon.dbc
  const char * _icon;

  // Pointers for runtime linking
  spelleffect_data_t* _effect1;
  spelleffect_data_t* _effect2;
  spelleffect_data_t* _effect3;

  const spelleffect_data_t& effect1() const { return *_effect1; }
  const spelleffect_data_t& effect2() const { return *_effect2; }
  const spelleffect_data_t& effect3() const { return *_effect3; }

  static spell_data_t* nil();
  static spell_data_t* find( unsigned, const std::string& confirmation = std::string(), bool ptr = false );
  static spell_data_t* find( const std::string& name, bool ptr = false );
  static spell_data_t* list( bool ptr = false );
  static void          link( bool ptr = false );

  bool                 is_used() SC_CONST;
  bool                 is_enabled() SC_CONST;
  void                 set_used( bool value );
  void                 set_enabled( bool value );

  inline unsigned      id() SC_CONST { return _id; }
  double               missile_speed() SC_CONST;
  uint32_t             school_mask() SC_CONST;
  resource_type        power_type() SC_CONST;
  bool                 is_class( player_type c ) SC_CONST;
  bool                 is_race( race_type r ) SC_CONST;
  bool                 is_level( uint32_t level ) SC_CONST;
  uint32_t             level() SC_CONST;
  uint32_t             max_level() SC_CONST;
  uint32_t             class_mask() SC_CONST;
  uint32_t             race_mask() SC_CONST;
  player_type          scaling_class() SC_CONST;
  double               min_range() SC_CONST;
  double               max_range() SC_CONST;
  bool                 in_range( double range ) SC_CONST;
  double               cooldown() SC_CONST;
  double               gcd() SC_CONST;
  uint32_t             category() SC_CONST;
  double               duration() SC_CONST;
  double               cost() SC_CONST;
  uint32_t             rune_cost() SC_CONST;
  double               runic_power_gain() SC_CONST;
  uint32_t             max_stacks() SC_CONST;
  uint32_t             initial_stacks() SC_CONST;
  double               proc_chance() SC_CONST;
  double               cast_time( uint32_t level ) SC_CONST;
  uint32_t             effect_id( uint32_t effect_num ) SC_CONST;
  bool                 flags( spell_attribute_t f ) SC_CONST;
  const char*          name_cstr() SC_CONST;
  const char*          desc() SC_CONST;
  const char*          tooltip() SC_CONST;
  double               scaling_multiplier() SC_CONST;
  unsigned             scaling_threshold() SC_CONST;
  double               extra_coeff() SC_CONST;
};

// SpellEffect.dbc
struct spelleffect_data_t {
  unsigned         _id;              // Effect id
  unsigned         _flags;           // Unused for now, 0x00 for all
  unsigned         _spell_id;        // Spell this effect belongs to
  unsigned         _index;           // Effect index for the spell
  effect_type_t    _type;            // Effect type
  effect_subtype_t _subtype;         // Effect sub-type
  // SpellScaling.dbc
  double           _m_avg;           // Effect average spell scaling multiplier
  double           _m_delta;         // Effect delta spell scaling multiplier
  double           _m_unk;           // Unused effect scaling multiplier
  //
  double           _coeff;           // Effect coefficient
  double           _amplitude;       // Effect amplitude (e.g., tick time)
  // SpellRadius.dbc
  double           _radius;          // Minimum spell radius
  double           _radius_max;      // Maximum spell radius
  //
  int              _base_value;      // Effect value
  int              _misc_value;      // Effect miscellaneous value
  int              _misc_value_2;    // Effect miscellaneous value 2
  int              _trigger_spell_id;// Effect triggers this spell id
  double           _m_chain;         // Effect chain multiplier
  double           _pp_combo_points; // Effect points per combo points
  double           _real_ppl;        // Effect real points per level
  int              _die_sides;       // Effect damage range

  // Pointers for runtime linking
  spell_data_t*    _spell;
  spell_data_t*    _trigger_spell;

  static spelleffect_data_t* nil();
  static spelleffect_data_t* find( unsigned, bool ptr = false );
  static spelleffect_data_t* list( bool ptr = false );
  static void                link( bool ptr = false );

  bool                       is_used() SC_CONST;
  bool                       is_enabled() SC_CONST;
  void                       set_used( bool value );
  void                       set_enabled( bool value );

  inline unsigned            id() SC_CONST { return _id; }
  unsigned                   index() SC_CONST;
  uint32_t                   spell_id() SC_CONST;
  uint32_t                   spell_effect_num() SC_CONST;
  effect_type_t              type() SC_CONST;
  effect_subtype_t           subtype() SC_CONST;
  inline int32_t             base_value() SC_CONST { return _base_value; }
  inline int32_t             misc_value1() SC_CONST { return _misc_value; }
  inline int32_t             misc_value2() SC_CONST { return _misc_value_2; }
  uint32_t                   trigger_spell_id() SC_CONST;
  double                     chain_multiplier() SC_CONST;

  double                     m_average() SC_CONST;
  double                     m_delta() SC_CONST;
  double                     m_unk() SC_CONST;
  double                     coeff() SC_CONST;
  double                     period() SC_CONST;
  double                     radius() SC_CONST;
  double                     radius_max() SC_CONST;
  double                     pp_combo_points() SC_CONST;
  double                     real_ppl() SC_CONST;
  int                        die_sides() SC_CONST;

  spell_data_t& spell()   const { return *_spell; }
  spell_data_t& trigger() const { return *_trigger_spell; }
  double percent() const { return _base_value / 100.0; }
  double seconds() const { return _base_value / 1000.0; }
  double resource( int type ) const
  {
    if( type == RESOURCE_RUNIC ) return _base_value / 10.0;
    if( type == RESOURCE_RAGE  ) return _base_value / 10.0;
    if( type == RESOURCE_MANA  ) return _base_value / 100.0;
    return _base_value;
  }
};

struct talent_data_t {
  const char * _name;        // Talent name
  unsigned     _id;          // Talent id
  unsigned     _flags;       // Unused for now, 0x00 for all
  unsigned     _tab_page;    // Talent tab page
  unsigned     _m_class;     // Class mask
  unsigned     _m_pet;       // Pet mask
  unsigned     _dependance;  // Talent depends on this talent id
  unsigned     _depend_rank; // Requires this rank of depended talent
  unsigned     _col;         // Talent column
  unsigned     _row;         // Talent row
  unsigned     _rank_id[3];  // Talent spell rank identifiers for ranks 1..3

  // Pointers for runtime linking
  spell_data_t* spell1;
  spell_data_t* spell2;
  spell_data_t* spell3;

  static talent_data_t* nil();
  static talent_data_t* find( unsigned, const std::string& confirmation = std::string(), bool ptr = false );
  static talent_data_t* find( const std::string& name, bool ptr = false );
  static talent_data_t* list( bool ptr = false );
  static void           link( bool ptr = false );

  bool                  is_used() SC_CONST;
  bool                  is_enabled() SC_CONST;
  void                  set_used( bool value );
  void                  set_enabled( bool value );

  inline unsigned       id() SC_CONST { return _id; }
  const char*           name_cstr() SC_CONST;
  uint32_t              tab_page() SC_CONST;
  bool                  is_class( player_type c ) SC_CONST;
  bool                  is_pet( pet_type_t p ) SC_CONST;
  uint32_t              depends_id() SC_CONST;
  uint32_t              depends_rank() SC_CONST;
  uint32_t              col() SC_CONST;
  uint32_t              row() SC_CONST;
  uint32_t              rank_spell_id( uint32_t rank ) SC_CONST;
  unsigned              mask_class() SC_CONST;
  unsigned              mask_pet() SC_CONST;

  uint32_t              max_rank() SC_CONST;
};

struct dbc_t
{
  bool ptr;

  dbc_t() : ptr( false ) { }
  dbc_t( bool ptr ) : ptr( ptr ) { }
  dbc_t( const dbc_t& other ) : ptr( other.ptr ) { }

  // Static Initialization
  static void init();
  static void de_init();
  static int glyphs( std::vector<unsigned>& glyph_ids, int cid, bool ptr = false );

  static const char* build_level( bool ptr = false );
  static const char* wow_version( bool ptr = false );
  static void        create_spell_data_index( bool ptr = false );
  static void        create_spelleffect_data_index( bool ptr = false );
  static void        create_talent_data_index( bool ptr = false );

  static const item_data_t* items( bool ptr = false );
  static size_t             n_items( bool ptr = false );

  // Index access
  spell_data_t** spell_data_index() SC_CONST;
  unsigned spell_data_index_size() SC_CONST;

  spelleffect_data_t** spelleffect_data_index() SC_CONST;
  unsigned spelleffect_data_index_size() SC_CONST;

  talent_data_t** talent_data_index() SC_CONST;
  unsigned talent_data_index_size() SC_CONST;

  // Game data table access
  double melee_crit_base( player_type t ) SC_CONST;
  double melee_crit_base( pet_type_t t ) SC_CONST;
  double spell_crit_base( player_type t ) SC_CONST;
  double spell_crit_base( pet_type_t t ) SC_CONST;
  double dodge_base( player_type t ) SC_CONST;
  double dodge_base( pet_type_t t ) SC_CONST;
  double regen_base( player_type t, unsigned level ) SC_CONST;
  double regen_base( pet_type_t t, unsigned level ) SC_CONST;
  stat_data_t& attribute_base( player_type t, unsigned level ) SC_CONST;
  stat_data_t& attribute_base( pet_type_t t, unsigned level ) SC_CONST;
  stat_data_t& race_base( race_type r ) SC_CONST;
  stat_data_t& race_base( pet_type_t t ) SC_CONST;

  double spell_scaling( player_type t, unsigned level ) SC_CONST;
  double melee_crit_scaling( player_type t, unsigned level ) SC_CONST;
  double melee_crit_scaling( pet_type_t t, unsigned level ) SC_CONST;
  double spell_crit_scaling( player_type t, unsigned level ) SC_CONST;
  double spell_crit_scaling( pet_type_t t, unsigned level ) SC_CONST;
  double dodge_scaling( player_type t, unsigned level ) SC_CONST;
  double dodge_scaling( pet_type_t t, unsigned level ) SC_CONST;

  double regen_spirit( player_type t, unsigned level ) SC_CONST;
  double regen_spirit( pet_type_t t, unsigned level ) SC_CONST;
  double oct_regen_mp( player_type t, unsigned level ) SC_CONST;
  double oct_regen_mp( pet_type_t t, unsigned level ) SC_CONST;

  double combat_rating( unsigned combat_rating_id, unsigned level ) SC_CONST;
  double oct_combat_rating( unsigned combat_rating_id, player_type t ) SC_CONST;

  const spell_data_t*            spell( unsigned spell_id ) SC_CONST;
  const spelleffect_data_t*      effect( unsigned effect_id ) SC_CONST;
  const talent_data_t*           talent( unsigned talent_id ) SC_CONST;
  const item_data_t*             item( unsigned item_id ) SC_CONST;

  const random_suffix_data_t&    random_suffix( unsigned suffix_id ) SC_CONST;
  const item_enchantment_data_t& item_enchantment( unsigned enchant_id ) SC_CONST;
  const gem_property_data_t&     gem_property( unsigned gem_id ) SC_CONST;

  const random_prop_data_t&      random_property( unsigned ilevel ) SC_CONST;
  const item_scale_data_t&       item_damage_1h( unsigned ilevel ) SC_CONST;
  const item_scale_data_t&       item_damage_2h( unsigned ilevel ) SC_CONST;
  const item_scale_data_t&       item_damage_caster_1h( unsigned ilevel ) SC_CONST;
  const item_scale_data_t&       item_damage_caster_2h( unsigned ilevel ) SC_CONST;
  const item_scale_data_t&       item_damage_ranged( unsigned ilevel ) SC_CONST;
  const item_scale_data_t&       item_damage_thrown( unsigned ilevel ) SC_CONST;
  const item_scale_data_t&       item_damage_wand( unsigned ilevel ) SC_CONST;

  const item_scale_data_t&       item_armor_quality( unsigned ilevel ) SC_CONST;
  const item_scale_data_t&       item_armor_shield( unsigned ilevel ) SC_CONST;
  const item_armor_type_data_t&  item_armor_total( unsigned ilevel ) SC_CONST;
  const item_armor_type_data_t&  item_armor_inv_type( unsigned inv_type ) SC_CONST;

  // Derived data access
  unsigned class_ability( unsigned class_id, unsigned tree_id, unsigned n ) SC_CONST;
  unsigned class_ability_tree_size() SC_CONST;
  unsigned class_ability_size() SC_CONST;

  unsigned race_ability( unsigned race_id, unsigned class_id, unsigned n ) SC_CONST;
  unsigned race_ability_size() SC_CONST;

  unsigned specialization_ability( unsigned class_id, unsigned tree_id, unsigned n ) SC_CONST;
  unsigned specialization_ability_tree_size() SC_CONST;
  unsigned specialization_ability_size() SC_CONST;

  unsigned mastery_ability( unsigned class_id, unsigned n ) SC_CONST;
  unsigned mastery_ability_size() SC_CONST;

  unsigned glyph_spell( unsigned class_id, unsigned glyph_type, unsigned n ) SC_CONST;
  unsigned glyph_spell_size() SC_CONST;

  unsigned set_bonus_spell( unsigned class_id, unsigned tier, unsigned n ) SC_CONST;
  unsigned set_bonus_spell_size() SC_CONST;

  // Helper methods
  double   weapon_dps( unsigned item_id ) SC_CONST;

  double   effect_average( unsigned effect_id, unsigned level ) SC_CONST;
  double   effect_delta( unsigned effect_id, unsigned level ) SC_CONST;

  double   effect_min( unsigned effect_id, unsigned level ) SC_CONST;
  double   effect_max( unsigned effect_id, unsigned level ) SC_CONST;
  double   effect_bonus( unsigned effect_id, unsigned level ) SC_CONST;

  unsigned class_ability_id( player_type c, const char* spell_name, int tree = -1 ) SC_CONST;
  unsigned race_ability_id( player_type c, race_type r, const char* spell_name ) SC_CONST;
  unsigned specialization_ability_id( player_type c, const char* spell_name, int tree = -1 ) SC_CONST;
  unsigned mastery_ability_id( player_type c, const char* spell_name ) SC_CONST;
  unsigned glyph_spell_id( player_type c, const char* spell_name ) SC_CONST;
  unsigned set_bonus_spell_id( player_type c, const char* spell_name, int tier = -1 ) SC_CONST;

  int      class_ability_tree( player_type c, uint32_t spell_id ) SC_CONST;
  int      specialization_ability_tree( player_type c, uint32_t spell_id ) SC_CONST;

  bool     is_class_ability( uint32_t spell_id ) SC_CONST;
  bool     is_race_ability( uint32_t spell_id ) SC_CONST;
  bool     is_specialization_ability( uint32_t spell_id ) SC_CONST;
  bool     is_mastery_ability( uint32_t spell_id ) SC_CONST;
  bool     is_glyph_spell( uint32_t spell_id ) SC_CONST;
  bool     is_set_bonus_spell( uint32_t spell_id ) SC_CONST;

  // Static helper methods
  static double fmt_value( double v, effect_type_t type, effect_subtype_t sub_type );
};

// Options ====================================================================

enum option_type_t
{
  OPT_NONE=0,
  OPT_STRING,     // std::string*
  OPT_APPEND,     // std::string* (append)
  OPT_CHARP,      // char*
  OPT_BOOL,       // int (only valid values are 1 and 0)
  OPT_INT,        // int
  OPT_FLT,        // double
  OPT_LIST,       // std::vector<std::string>*
  OPT_FUNC,       // function pointer
  OPT_TALENT_RANK, // talent rank
  OPT_SPELL_ENABLED, // spell enabled
  OPT_DEPRECATED,
  OPT_UNKNOWN
};

typedef bool ( *option_function_t )( sim_t* sim, const std::string& name, const std::string& value );

struct option_t
{
  const char* name;
  int type;
  void* address;

  void print( FILE* );
  void save ( FILE* );
  bool parse( sim_t*, const std::string& name, const std::string& value );

  static void add( std::vector<option_t>&, const char* name, int type, void* address );
  static void copy( std::vector<option_t>& opt_vector, option_t* opt_array );
  static bool parse( sim_t*, std::vector<option_t>&, const std::string& name, const std::string& value );
  static bool parse( sim_t*, const char* context, std::vector<option_t>&, const std::string& options_str );
  static bool parse( sim_t*, const char* context, option_t*,              const std::string& options_str );
  static bool parse_file( sim_t*, FILE* file );
  static bool parse_line( sim_t*, char* line );
  static bool parse_token( sim_t*, std::string& token );
};

// Talent Translation =========================================================

#define MAX_TALENT_POINTS 41
#define MAX_TALENT_ROW ((MAX_TALENT_POINTS+4)/5)
#define MAX_TALENT_TREES 3
#define MAX_TALENT_COL 4
#define MAX_TALENT_SLOTS (MAX_TALENT_TREES*MAX_TALENT_ROW*MAX_TALENT_COL)
#define MAX_TALENT_RANK_SLOTS ( 90 )

#define MAX_CLASS ( 12 )

struct talent_translation_t
{
  int  index;
  int  max;
  int* address;
  int  row;
  int  req;
  int  tree;
  std::string name;
};

// Utilities =================================================================

struct util_t
{
  static double talent_rank( int num, int max, double increment );
  static double talent_rank( int num, int max, double value1, double value2, ... );

  static int talent_rank( int num, int max, int increment );
  static int talent_rank( int num, int max, int value1, int value2, ... );

  static double ability_rank( int player_level, double ability_value, int ability_level, ... );
  static int    ability_rank( int player_level, int    ability_value, int ability_level, ... );

  static char* dup( const char* );

  static const char* attribute_type_string     ( int type );
  static const char* dmg_type_string           ( int type );
  static const char* elixir_type_string        ( int type );
  static const char* flask_type_string         ( int type );
  static const char* food_type_string          ( int type );
  static const char* gem_type_string           ( int type );
  static const char* meta_gem_type_string      ( int type );
  static const char* player_type_string        ( int type );
  static const char* pet_type_string           ( int type );
  static const char* profession_type_string    ( int type );
  static const char* race_type_string          ( int type );
  static const char* role_type_string          ( int type );
  static const char* resource_type_string      ( int type );
  static const char* result_type_string        ( int type );
  static int         school_type_component     ( int s_type, int c_type );
  static const char* school_type_string        ( int type );
  static const char* armor_type_string         ( player_type ptype, int slot_type );
  static const char* set_bonus_string          ( set_type type );
  static const char* slot_type_string          ( int type );
  static const char* stat_type_string          ( int type );
  static const char* stat_type_abbrev          ( int type );
  static const char* stat_type_wowhead         ( int type );
  static int         talent_tree               ( int tree, player_type ptype );
  static const char* talent_tree_string        ( int tree, bool armory_format = true );
  static const char* weapon_type_string        ( int type );
  static const char* weapon_class_string       ( int _class );
  static const char* weapon_subclass_string    ( int subclass );
  static const char* set_item_type_string      ( int item_set );

  static int parse_attribute_type              ( const std::string& name );
  static int parse_dmg_type                    ( const std::string& name );
  static int parse_elixir_type                 ( const std::string& name );
  static int parse_flask_type                  ( const std::string& name );
  static int parse_food_type                   ( const std::string& name );
  static int parse_gem_type                    ( const std::string& name );
  static int parse_meta_gem_type               ( const std::string& name );
  static player_type parse_player_type         ( const std::string& name );
  static pet_type_t parse_pet_type             ( const std::string& name );
  static int parse_profession_type             ( const std::string& name );
  static race_type parse_race_type             ( const std::string& name );
  static role_type parse_role_type             ( const std::string& name );
  static int parse_resource_type               ( const std::string& name );
  static int parse_result_type                 ( const std::string& name );
  static school_type parse_school_type         ( const std::string& name );
  static set_type parse_set_bonus              ( const std::string& name );
  static int parse_slot_type                   ( const std::string& name );
  static stat_type parse_stat_type             ( const std::string& name );
  static stat_type parse_reforge_type          ( const std::string& name );
  static int parse_talent_tree                 ( const std::string& name );
  static int parse_weapon_type                 ( const std::string& name );

  static bool parse_origin( std::string& region, std::string& server, std::string& name, const std::string& origin );

  static int class_id_mask( int type );
  static int class_id( int type );
  static unsigned race_mask( int type );
  static unsigned race_id( int type );
  static unsigned pet_mask( int type );
  static unsigned pet_id( int type );
  static player_type pet_class_type( int type );

  static const char* class_id_string( int type );
  static int translate_class_id( int cid );
  static int translate_class_str( std::string& s );
  static race_type translate_race_id( int rid );
  static stat_type translate_item_mod( int stat_mod );
  static slot_type translate_invtype( int inv_type );
  static weapon_type translate_weapon_subclass( item_subclass_weapon weapon_subclass );
  static bool socket_gem_match( int socket, int gem );

  static int string_split( std::vector<std::string>& results, const std::string& str, const char* delim, bool allow_quotes = false );
  static int string_split( const std::string& str, const char* delim, const char* format, ... );
  static int string_strip_quotes( std::string& str );

  static std::string& to_string( int i );
  static std::string& to_string( double f, int precision );

  static int64_t milliseconds();
  static int64_t parse_date( const std::string& month_day_year );

  static int printf( const char *format,  ... );
  static int fprintf( FILE *stream, const char *format,  ... );

  static std::string& str_to_utf8( std::string& str );
  static std::string& str_to_latin1( std::string& str );
  static std::string& urlencode( std::string& str );
  static std::string& urldecode( std::string& str );

  static std::string& format_text( std::string& name, bool input_is_utf8 );

  static std::string& html_special_char_decode( std::string& str );

  static bool str_compare_ci( const std::string& l, const std::string& r );
  static bool str_in_str_ci ( const std::string& l, const std::string& r );

  static void add_base_stats( base_stats_t& result, base_stats_t& a, base_stats_t b );

  static double floor( double X, unsigned int decplaces = 0 );
  static double ceil( double X, unsigned int decplaces = 0 );
  static double round( double X, unsigned int decplaces = 0 );

  static std::string& tolower( std::string& );
};

// Spell information struct, holding static functions to output spell data in a human readable form

struct spell_info_t
{
  static std::string to_str( sim_t* sim, const spell_data_t* spell );
  static std::string to_str( sim_t* sim, uint32_t spell_id );
  static std::string talent_to_str( sim_t* sim, const talent_data_t* talent );
  static std::ostringstream& effect_to_str( sim_t* sim, const spell_data_t* spell, const spelleffect_data_t* effect, std::ostringstream& s );
};

// Spell ID class

enum s_type_t
{
  T_SPELL = 0,
  T_TALENT,
  T_MASTERY,
  T_GLYPH,
  T_CLASS,
  T_RACE,
  T_SPEC,
  T_ITEM
};

struct spell_id_t
{
  s_type_t                   s_type;
  uint32_t                   s_id;
  const spell_data_t*        s_data;
  bool                       s_enabled;
  player_t*                  s_player;
  bool                       s_overridden;      // Spell enable/disable status was manually overridden
  std::string                s_token;
  const talent_t*            s_required_talent;
  const spelleffect_data_t*  s_effects[ MAX_EFFECTS ];
  const spelleffect_data_t*  s_single;
  int                        s_tree;

  // Construction & deconstruction
  spell_id_t( const spell_id_t& copy );
  spell_id_t( player_t* player = 0, const char* t_name = 0 );
  spell_id_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent = 0 );
  spell_id_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent = 0 );
  virtual ~spell_id_t();

  // Generic spell data initialization
  bool initialize( const char* s_name = 0 );
  virtual bool enable( bool override_value = false );

  // Spell data object validity check
  virtual bool ok() SC_CONST;

  // Spell state output
  std::string to_str() SC_CONST;

  // Access methods
  virtual uint32_t spell_id() SC_CONST { return ( s_player && s_data ) ? s_id : 0; }
  virtual const char* real_name() SC_CONST;
  virtual const std::string token() SC_CONST;
  virtual double missile_speed() SC_CONST;
  virtual uint32_t school_mask() SC_CONST;
  virtual school_type get_school_type() SC_CONST;
  virtual resource_type power_type() SC_CONST;
  virtual double min_range() SC_CONST;
  virtual double max_range() SC_CONST;
  virtual double extra_coeff() SC_CONST;
  virtual bool in_range() SC_CONST;
  virtual double cooldown() SC_CONST;
  virtual double gcd() SC_CONST;
  virtual uint32_t category() SC_CONST;
  virtual double duration() SC_CONST;
  virtual double cost() SC_CONST;
  virtual uint32_t rune_cost() SC_CONST;
  virtual double runic_power_gain() SC_CONST;
  virtual uint32_t max_stacks() SC_CONST;
  virtual uint32_t initial_stacks() SC_CONST;
  virtual double proc_chance() SC_CONST;
  virtual double cast_time() SC_CONST;
  virtual uint32_t effect_id( uint32_t effect_num ) SC_CONST;
  virtual bool flags( spell_attribute_t f ) SC_CONST;
  virtual const char* desc() SC_CONST;
  virtual const char* tooltip() SC_CONST;
  virtual int32_t effect_type( uint32_t effect_num ) SC_CONST;
  virtual int32_t effect_subtype( uint32_t effect_num ) SC_CONST;
  virtual int32_t effect_base_value( uint32_t effect_num ) SC_CONST;
  virtual int32_t effect_misc_value1( uint32_t effect_num ) SC_CONST;
  virtual int32_t effect_misc_value2( uint32_t effect_num ) SC_CONST;
  virtual uint32_t effect_trigger_spell( uint32_t effect_num ) SC_CONST;
  virtual double effect_chain_multiplier( uint32_t effect_num ) SC_CONST;
  virtual double effect_average( uint32_t effect_num ) SC_CONST;
  virtual double effect_delta( uint32_t effect_num ) SC_CONST;
  virtual double effect_bonus( uint32_t effect_num ) SC_CONST;
  virtual double effect_min( uint32_t effect_num ) SC_CONST;
  virtual double effect_max( uint32_t effect_num ) SC_CONST;
  virtual double effect_coeff( uint32_t effect_num ) SC_CONST;
  virtual double effect_period( uint32_t effect_num ) SC_CONST;
  virtual double effect_radius( uint32_t effect_num ) SC_CONST;
  virtual double effect_radius_max( uint32_t effect_num ) SC_CONST;
  virtual double effect_pp_combo_points( uint32_t effect_num ) SC_CONST;
  virtual double effect_real_ppl( uint32_t effect_num ) SC_CONST;
  virtual int    effect_die_sides( uint32_t effect_num ) SC_CONST;

  // Generalized access API
  virtual double base_value( effect_type_t type = E_MAX, effect_subtype_t sub_type = A_MAX, int misc_value = DEFAULT_MISC_VALUE, int misc_value2 = DEFAULT_MISC_VALUE ) SC_CONST;

  // Return an additive modifier from the spell (E_APPLY_AURA -> A_ADD_PCT_MODIFIER|A_ADD_FLAT_MODIFIER ),
  // based on the property of the modifier (as defined by misc_value)
  virtual double mod_additive( property_type_t = P_MAX ) SC_CONST;

  // Spell data specific static methods
  static uint32_t get_school_mask( const school_type s );
  static school_type get_school_type( const uint32_t mask );
  static bool is_school( const school_type s, const school_type s2 );
private:
};

// Talent class

struct talent_t : spell_id_t
{
  const talent_data_t* t_data;
  unsigned             t_rank;
  bool                 t_enabled;
  bool                 t_overridden;
  const spell_id_t*    t_rank_spells[ MAX_RANK ];
  const spell_id_t*    t_default_rank;

  talent_t( player_t* p, talent_data_t* td );
  virtual ~talent_t();
  virtual bool set_rank( uint32_t value, bool overridden = false );
  virtual bool ok() SC_CONST;
  std::string to_str() SC_CONST;
  virtual uint32_t rank() SC_CONST;

  virtual uint32_t spell_id() SC_CONST;
  virtual uint32_t max_rank() SC_CONST;
  virtual uint32_t rank_spell_id( const uint32_t r ) SC_CONST;
  virtual const spell_id_t* rank_spell( uint32_t r = 0 ) SC_CONST;

  // Future trimmed down access
  talent_data_t* td;
  spell_data_t* sd;
  spell_data_t* trigger;
  // unsigned rank;

  talent_data_t& data() { return *td; }
  spell_data_t& spell( unsigned r=0 )
  {
    if ( r == 1 ) return *( td -> spell1 );
    if ( r == 2 ) return *( td -> spell2 );
    if ( r == 3 ) return *( td -> spell3 );
    return *sd;
  }
  const spelleffect_data_t& effect1() const { return sd -> effect1(); }
  const spelleffect_data_t& effect2() const { return sd -> effect2(); }
  const spelleffect_data_t& effect3() const { return sd -> effect3(); }
};

// Active Spell ID class

struct active_spell_t : public spell_id_t
{
  active_spell_t( const active_spell_t& copy ) : spell_id_t( copy ) { }
  active_spell_t( player_t* player = 0, const char* t_name = 0 ) : spell_id_t( player, t_name ) { }
  active_spell_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent = 0 );
  active_spell_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent = 0 );
  virtual ~active_spell_t() {}
};

// Passive Spell ID class

struct passive_spell_t : public spell_id_t
{
  passive_spell_t( const passive_spell_t& copy ) : spell_id_t( copy ) { }
  passive_spell_t( player_t* player = 0, const char* t_name = 0 ) : spell_id_t( player, t_name ) { }
  passive_spell_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent = 0 );
  passive_spell_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent = 0 );
  virtual ~passive_spell_t() {}
};

struct glyph_t : public spell_id_t
{
  // Future trimmed down access
  spell_data_t* sd;
  spell_data_t* sd_enabled;
  int enabled() { return ( sd == sd_enabled ) ? 1 : 0; }

  glyph_t( player_t* p, spell_data_t* sd );
  virtual ~glyph_t() {}

  virtual bool enable( bool override_value = true );

  const spell_data_t& spell() { return *sd_enabled; }
  const spelleffect_data_t& effect1() const { return sd_enabled -> effect1(); }
  const spelleffect_data_t& effect2() const { return sd_enabled -> effect2(); }
  const spelleffect_data_t& effect3() const { return sd_enabled -> effect3(); }
};

struct mastery_t : public spell_id_t
{
  int m_tree;

  mastery_t( const mastery_t& copy ) : spell_id_t( copy ), m_tree( copy.m_tree ) { }
  mastery_t( player_t* player = 0, const char* t_name = 0 ) : spell_id_t( player, t_name ) { }
  mastery_t( player_t* player, const char* t_name, const uint32_t id, int tree );
  mastery_t( player_t* player, const char* t_name, const char* s_name, int tree );
  virtual ~mastery_t() {}

  std::string to_str() SC_CONST;

  virtual bool ok() SC_CONST;
  virtual double base_value( effect_type_t type = E_MAX,
                             effect_subtype_t sub_type = A_MAX,
                             int misc_value = DEFAULT_MISC_VALUE,
                             int misc_value2 = DEFAULT_MISC_VALUE ) SC_CONST;
};

// Raid Event

struct raid_event_t
{
  sim_t* sim;
  std::string name_str;
  int64_t num_starts;
  double first, last;
  double cooldown;
  double cooldown_stddev;
  double cooldown_min;
  double cooldown_max;
  double duration;
  double duration_stddev;
  double duration_min;
  double duration_max;
  double distance_min;
  double distance_max;
  double saved_duration;
  rng_t* rng;
  std::vector<player_t*> affected_players;

  raid_event_t( sim_t*, const char* name );
  virtual ~raid_event_t() {}

  virtual double cooldown_time() SC_CONST;
  virtual double duration_time() SC_CONST;
  virtual void schedule();
  virtual void reset();
  virtual void start();
  virtual void finish();
  virtual void parse_options( option_t*, const std::string& options_str );
  virtual const char* name() SC_CONST { return name_str.c_str(); }
  static raid_event_t* create( sim_t* sim, const std::string& name, const std::string& options_str );
  static void init( sim_t* );
  static void reset( sim_t* );
  static void combat_begin( sim_t* );
  static void combat_end( sim_t* ) {}
};

// Gear Stats ================================================================

struct gear_stats_t
{
  double attribute[ ATTRIBUTE_MAX ];
  double resource[ RESOURCE_MAX ];
  double spell_power;
  double spell_penetration;
  double mp5;
  double attack_power;
  double expertise_rating;
  double hit_rating;
  double crit_rating;
  double haste_rating;
  double weapon_dps;
  double weapon_speed;
  double weapon_offhand_dps;
  double weapon_offhand_speed;
  double armor;
  double bonus_armor;
  double dodge_rating;
  double parry_rating;
  double block_rating;
  double mastery_rating;

  gear_stats_t() { memset( ( void* ) this, 0x00, sizeof( gear_stats_t ) ); }

  void   add_stat( int stat, double value );
  void   set_stat( int stat, double value );
  double get_stat( int stat ) SC_CONST;
  void   print( FILE* );
  static double stat_mod( int stat );
};


// Buffs ======================================================================

struct buff_t : public spell_id_t
{
  sim_t* sim;
  player_t* player;
  player_t* source;
  std::string name_str;
  std::vector<std::string> aura_str;
  std::vector<double> stack_occurrence,stack_react_time;
  int current_stack, max_stack;
  double current_value, react, buff_duration, buff_cooldown, default_chance;
  double last_start, last_trigger, start_intervals_sum, trigger_intervals_sum, uptime_sum;
  int64_t up_count, down_count, start_intervals, trigger_intervals, start_count, refresh_count;
  int64_t trigger_attempts, trigger_successes;
  double uptime_pct, benefit_pct, trigger_pct, avg_start_interval, avg_trigger_interval, avg_start, avg_refresh;
  bool reverse, constant, quiet, overridden;
  int aura_id;
  event_t* expiration;
  int rng_type;
  rng_t* rng;
  cooldown_t* cooldown;
  buff_t* next;

  buff_t() : sim( 0 ) {}
  virtual ~buff_t() { };

  // Raid Aura
  buff_t( sim_t*, const std::string& name,
          int max_stack=1, double buff_duration=0, double buff_cooldown=0,
          double chance=1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0 );

  // Player Buff
  buff_t( player_t*, const std::string& name,
          int max_stack=1, double buff_duration=0, double buff_cooldown=0,
          double chance=1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0 );

  // Player Buff with extracted data
  buff_t( player_t*, talent_t*, ... );
  buff_t( player_t*, spell_data_t*, ... );

  // Player Buff as spell_id_t by name
  buff_t( player_t*, const std::string& name, const char* sname,
          double chance=-1, double duration=-1.0,
          bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC );

  // Player Buff as spell_id_t by id
  buff_t( player_t*, const uint32_t id, const std::string& name,
          double chance=-1, double duration=-1.0,
          bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC );

  // Use check() inside of ready() methods to prevent skewing of "benefit" calculations.
  // Use up() where the presence of the buff affects the action mechanics.

  int    check() { return current_stack; }
  bool   up()    { if( current_stack > 0 ) { up_count++; } else { down_count++; } return current_stack > 0; }
  int    stack() { if( current_stack > 0 ) { up_count++; } else { down_count++; } return current_stack; }
  double value() { if( current_stack > 0 ) { up_count++; } else { down_count++; } return current_value; }
  double remains();
  bool   remains_gt( double time );
  bool   remains_lt( double time );
  bool   trigger  ( action_t*, int stacks=1, double value=-1.0 );
  virtual bool   trigger  ( int stacks=1, double value=-1.0, double chance=-1.0 );
  virtual void   increment( int stacks=1, double value=-1.0 );
  void   decrement( int stacks=1, double value=-1.0 );
  void   extend_duration( player_t* p, double seconds );

  virtual void start    ( int stacks=1, double value=-1.0 );
  virtual void refresh  ( int stacks=0, double value=-1.0 );
  virtual void bump     ( int stacks=1, double value=-1.0 );
  virtual void override ( int stacks=1, double value=-1.0 );
  virtual bool may_react( int stacks=1 );
  virtual int stack_react();
  virtual void expire();
  virtual void predict();
  virtual void reset();
  virtual void aura_gain();
  virtual void aura_loss();
  virtual void merge( buff_t* other_buff );
  virtual void analyze();
  virtual void init();
  virtual void parse_options( va_list vap );
  virtual const char* name() { return name_str.c_str(); }

  action_expr_t* create_expression( action_t*, const std::string& type );
  std::string    to_str() SC_CONST;

  static buff_t* find(    sim_t*, const std::string& name );
  static buff_t* find( player_t*, const std::string& name );

  virtual void _init_buff_t();

  const spelleffect_data_t& effect1() const { return s_data -> effect1(); }
  const spelleffect_data_t& effect2() const { return s_data -> effect2(); }
  const spelleffect_data_t& effect3() const { return s_data -> effect3(); }
};

struct stat_buff_t : public buff_t
{
  int stat;
  double amount;
  stat_buff_t( player_t*, const std::string& name,
               int stat, double amount,
               int max_stack=1, double buff_duration=0, double buff_cooldown=0,
               double chance=1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0 );
  stat_buff_t( player_t*, const uint32_t id, const std::string& name,
                 int stat, double amount,
                 double chance=1.0, double buff_cooldown=-1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC );
  virtual ~stat_buff_t() { };
  virtual void bump     ( int stacks=1, double value=-1.0 );
  virtual void decrement( int stacks=1, double value=-1.0 );
  virtual void expire();
};

struct cost_reduction_buff_t : public buff_t
{
  int school;
  double amount;
  bool refreshes;

  cost_reduction_buff_t( player_t*, const std::string& name,
                         int school, double amount,
                         int max_stack=1, double buff_duration=0, double buff_cooldown=0,
                         double chance=1.0, bool refreshes=false, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0 );
  cost_reduction_buff_t( player_t*, const uint32_t id, const std::string& name,
                         int school, double amount,
                         double chance=1.0, double buff_cooldown=-1.0, bool refreshes=false, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC );
  virtual ~cost_reduction_buff_t() { };
  virtual void bump     ( int stacks=1, double value=-1.0 );
  virtual void decrement( int stacks=1, double value=-1.0 );
  virtual void expire();
  virtual void refresh  ( int stacks=0, double value=-1.0 );
};

struct debuff_t : public buff_t
{
  // Player De-Buff
  debuff_t( player_t*, const std::string& name,
            int max_stack=1, double buff_duration=0, double buff_cooldown=0,
            double chance=1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0 );

  // Player De-Buff as spell_id_t by id
  debuff_t( player_t*, const uint32_t id, const std::string& name,
            double chance=-1, double duration=-1.0,
            bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC );

};

typedef struct buff_t aura_t;


// Expressions =================================================================

enum token_type_t {
  TOK_UNKNOWN=0,
  TOK_PLUS,
  TOK_MINUS,
  TOK_MULT,
  TOK_DIV,
  TOK_ADD,
  TOK_SUB,
  TOK_AND,
  TOK_OR,
  TOK_NOT,
  TOK_EQ,
  TOK_NOTEQ,
  TOK_LT,
  TOK_LTEQ,
  TOK_GT,
  TOK_GTEQ,
  TOK_LPAR,
  TOK_RPAR,
  TOK_IN,
  TOK_NOTIN,
  TOK_NUM,
  TOK_STR,
  TOK_ABS,
  TOK_SPELL_LIST
};

struct new_buff_t : public buff_t
{
  int                       default_stack_charge;
  const spelleffect_data_t* e_data[MAX_EFFECTS];
  const spelleffect_data_t* single;

  new_buff_t( player_t*, const std::string&, uint32_t, double override_chance = 0.0, bool quiet = false, bool reverse = false, int rng_type = RNG_CYCLIC );

  virtual bool trigger( int stacks = -1, double value = -1.0, double chance = -1.0 );
  virtual double base_value( effect_type_t type = E_MAX, effect_subtype_t sub_type = A_MAX, int misc_value = DEFAULT_MISC_VALUE, int misc_value2 = DEFAULT_MISC_VALUE ) SC_CONST;
};

struct expr_token_t
{
  int type;
  std::string label;
};

enum expr_data_type_t {
  DATA_SPELL = 0,
  DATA_TALENT,
  DATA_EFFECT,
  DATA_TALENT_SPELL,
  DATA_CLASS_SPELL,
  DATA_RACIAL_SPELL,
  DATA_MASTERY_SPELL,
  DATA_SPECIALIZATION_SPELL,
  DATA_GLYPH_SPELL,
  DATA_SET_BONUS_SPELL
};

struct expression_t
{
  static int precedence( int token_type );
  static int is_unary( int token_type );
  static int is_binary( int token_type );
  static int next_token( action_t* action, const std::string& expr_str, int& current_index, std::string& token_str, token_type_t prev_token );
  static void parse_tokens( action_t* action, std::vector<expr_token_t>& tokens, const std::string& expr_str );
  static void print_tokens( std::vector<expr_token_t>& tokens, sim_t* sim );
  static void convert_to_unary( action_t* action, std::vector<expr_token_t>& tokens );
  static bool convert_to_rpn( action_t* action, std::vector<expr_token_t>& tokens );
};

struct action_expr_t
{
  action_t* action;
  std::string name_str;

  int result_type;
  double result_num;
  std::string result_str;

  action_expr_t( action_t* a, const std::string& n, int t=TOK_UNKNOWN ) : action(a), name_str(n), result_type(t), result_num(0) {}
  action_expr_t( action_t* a, const std::string& n, double       constant_value ) : action(a), name_str(n) { result_type = TOK_NUM; result_num = constant_value; }
  action_expr_t( action_t* a, const std::string& n, std::string& constant_value ) : action(a), name_str(n) { result_type = TOK_STR; result_str = constant_value; }
  virtual ~action_expr_t() { name_str.clear(); result_str.clear(); };
  virtual int evaluate() { return result_type; }
  virtual const char* name() { return name_str.c_str(); }
  virtual bool success() { return ( evaluate() == TOK_NUM ) && ( result_num != 0 ); }

  static action_expr_t* parse( action_t*, const std::string& expr_str );
};

struct spell_data_expr_t
{
  std::string name_str;
  sim_t* sim;
  expr_data_type_t data_type;
  bool effect_query;

  int result_type;
  double result_num;
  std::vector<uint32_t> result_spell_list;
  std::string result_str;

  spell_data_expr_t( sim_t* sim, const std::string& n, expr_data_type_t dt = DATA_SPELL, bool eq = false, int t=TOK_UNKNOWN ) : name_str(n), sim(sim), data_type( dt ), effect_query( eq ), result_type(t), result_num(0), result_spell_list() {}
  spell_data_expr_t( sim_t* sim, const std::string& n, double       constant_value ) : name_str(n), sim(sim), data_type( DATA_SPELL) { result_type = TOK_NUM; result_num = constant_value; }
  spell_data_expr_t( sim_t* sim, const std::string& n, std::string& constant_value ) : name_str(n), sim(sim), data_type( DATA_SPELL) { result_type = TOK_STR; result_str = constant_value; }
  spell_data_expr_t( sim_t* sim, const std::string& n, std::vector<uint32_t>& constant_value ) : name_str(n), sim(sim), data_type( DATA_SPELL) { result_type = TOK_SPELL_LIST; result_spell_list = constant_value; }
  virtual ~spell_data_expr_t() { name_str.clear(); result_str.clear(); };
  virtual int evaluate() { return result_type; }
  virtual const char* name() { return name_str.c_str(); }

  virtual std::vector<uint32_t> operator|(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator&(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator-(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> operator<(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator<=(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>=(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator==(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator!=(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> in(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> not_in(const spell_data_expr_t& other) { return std::vector<uint32_t>(); }

  static spell_data_expr_t* parse( sim_t* sim, const std::string& expr_str );
  static spell_data_expr_t* create_spell_expression( sim_t* sim, const std::string& name_str );
};

// Simulation Engine =========================================================

struct sim_t
{
  int         argc;
  char**      argv;
  sim_t*      parent;
  event_t*    free_list;
  player_t*   target;
  player_t*   target_list;
  player_t*   player_list;
  player_t*   active_player;
  int         num_players;
  int         num_enemies;
  int         max_player_level;
  int         canceled;
  double      queue_lag, queue_lag_stddev;
  double      gcd_lag, gcd_lag_stddev;
  double      channel_lag, channel_lag_stddev;
  double      queue_gcd_reduction;
  int         strict_gcd_queue;
    // Latency
  double      world_lag, world_lag_stddev;
  double      travel_variance, default_skill, reaction_time, regen_periodicity;
  double      current_time, max_time, expected_time, vary_combat_length, last_event;
  int         fixed_time;
  int64_t     events_remaining, max_events_remaining;
  int64_t     events_processed, total_events_processed;
  int         seed, id, iterations, current_iteration, current_slot;
  int         armor_update_interval, weapon_speed_scale_factors;
  int         optimal_raid, log, debug;
  int         save_profiles, default_actions;
  int         normalized_stat;
  std::string current_name, default_region_str, default_server_str, save_prefix_str,save_suffix_str;
  bool        input_is_utf8;
  std::vector<player_t*> actor_list;
  std::string main_target_str;
  double      dtr_proc_chance;

  // Target options
  double      target_death_pct;
  int         target_level;
  std::string target_race;
  int         target_adds;

  // Data access
  dbc_t       dbc;

  // Default stat enchants
  gear_stats_t enchant;

  std::map<std::string,std::string> var_map;
  std::vector<option_t> options;
  std::vector<std::string> party_encoding;
  std::vector<std::string> item_db_sources;

  // Random Number Generation
  rng_t* rng;
  rng_t* deterministic_rng;
  rng_t* rng_list;
  int smooth_rng, deterministic_roll, average_range, average_gauss;
  int convergence_scale;

  // Timing Wheel Event Management
  event_t** timing_wheel;
  int    wheel_seconds, wheel_size, wheel_mask, timing_slice;
  double wheel_granularity;

  // Raid Events
  std::vector<raid_event_t*> raid_events;
  std::string raid_events_str;
  std::string fight_style;

  // Buffs and Debuffs Overrides
  struct overrides_t
  {
    int abominations_might;
    int arcane_brilliance;
    int arcane_tactics;
    int battle_shout;
    int bleeding;
    int blessing_of_kings;
    int blessing_of_might;
    int blood_frenzy_bleed;
    int blood_frenzy_physical;
    int bloodlust;
    int brittle_bones;
    int communion;
    int corrosive_spit;
    int corruption_absolute;
    int critical_mass;
    int curse_of_elements;
    int dark_intent;
    int demonic_pact;
    int demoralizing_roar;
    int demoralizing_screech;
    int demoralizing_shout;
    int devotion_aura;
    int earth_and_moon;
    int ebon_plaguebringer;
    int elemental_oath;
    int essence_of_the_red;
    int exhaustion;
    int expose_armor;
    int faerie_fire;
    int fel_intelligence;
    int ferocious_inspiration;
    int flametongue_totem;
    int focus_magic;
    int fortitude;
    int hellscreams_warsong;
    int hemorrhage;
    int honor_among_thieves;
    int horn_of_winter;
    int hunters_mark;
    int hunting_party;
    int improved_icy_talons;
    int infected_wounds;
    int insect_swarm;
    int judgements_of_the_just;
    int leader_of_the_pack;
    int lightning_breath;
    int mana_spring_totem;
    int mangle;
    int mark_of_the_wild;
    int master_poisoner;
    int mind_quickening;
    int moonkin_aura;
    int poisoned;
    int rampage;
    int ravage;
    int replenishment;
    int roar_of_courage;
    int savage_combat;
    int scarlet_fever;
    int shadow_and_flame;
    int strength_of_earth;
    int strength_of_wrynn;
    int sunder_armor;
    int tailspin;
    int tear_armor;
    int tendon_rip;
    int thunder_clap;
    int trueshot_aura;
    int qiraji_fortitude;
    int unleashed_rage;
    int vindication;
    int windfury_totem;
    int wrath_of_air;
    overrides_t() { memset( ( void* ) this, 0x0, sizeof( overrides_t ) ); }
  };
  overrides_t overrides;

  // Auras
  struct auras_t
  {
    aura_t* abominations_might;
    aura_t* arcane_tactics;
    aura_t* battle_shout;
    aura_t* communion;
    aura_t* demonic_pact;
    aura_t* devotion_aura;
    aura_t* elemental_oath;
    aura_t* fel_intelligence;
    aura_t* ferocious_inspiration;
    aura_t* flametongue_totem;
    aura_t* honor_among_thieves;
    aura_t* horn_of_winter;
    aura_t* hunting_party;
    aura_t* improved_icy_talons;
    aura_t* leader_of_the_pack;
    aura_t* mana_spring_totem;
    aura_t* moonkin;
    aura_t* mind_quickening;
    aura_t* qiraji_fortitude;
    aura_t* rampage;
    aura_t* roar_of_courage;
    aura_t* strength_of_earth;
    aura_t* trueshot;
    aura_t* unleashed_rage;
    aura_t* windfury_totem;
    aura_t* wrath_of_air;
    auras_t() { memset( (void*) this, 0x0, sizeof( auras_t ) ); }
  };
  auras_t auras;

  // Auras and De-Buffs
  buff_t* buff_list;
  double aura_delay;

  cooldown_t* cooldown_list;

  // Replenishment
  int replenishment_targets;
  std::vector<player_t*> replenishment_candidates;

  // Reporting
  report_t*  report;
  scaling_t* scaling;
  plot_t*    plot;
  reforge_plot_t* reforge_plot;
  double     raid_dps, total_dmg, raid_hps, total_heal, total_seconds, elapsed_cpu_seconds;
  int        report_progress;
  int        bloodlust_percent, bloodlust_time;
  std::string reference_player_str;
  std::vector<player_t*> players_by_rank;
  std::vector<player_t*> players_by_name;
  std::vector<player_t*> targets_by_name;
  std::vector<std::string> id_dictionary;
  std::vector<std::string> dps_charts, gear_charts, dpet_charts;
  std::string downtime_chart;
  std::vector<double> iteration_timeline;
  std::vector<int> distribution_timeline;
  std::vector<int> divisor_timeline;
  std::string timeline_chart;
  std::string output_file_str, html_file_str,  xml_file_str;
  std::string path_str;
  std::deque<std::string> active_files;
  std::vector<std::string> error_list;
  FILE* output_file;
  int armory_throttle;
  int current_throttle;
  int debug_exp;
  int report_precision;
  int report_pets_separately;
  int report_targets;
  int report_details;
  int report_rng;
  int hosted_html;
  int print_styles;

  // Multi-Threading
  int threads;
  std::vector<sim_t*> children;
  void* thread_handle;
  int  thread_index;

  // Spell database access
  spell_data_expr_t* spell_query;

  sim_t( sim_t* parent=0, int thrdID=0 );
  virtual ~sim_t();

  int       main( int argc, char** argv );
  void      cancel();
  double    progress( std::string& phase );
  void      combat( int iteration );
  void      combat_begin();
  void      combat_end();
  void      add_event( event_t*, double delta_time );
  void      reschedule_event( event_t* );
  void      flush_events();
  void      cancel_events( player_t* );
  event_t*  next_event();
  void      reset();
  bool      init();
  void      analyze_player( player_t* p );
  void      analyze();
  void      merge( sim_t& other_sim );
  void      merge();
  bool      iterate();
  void      partition();
  bool      execute();
  void      print_options();
  void      create_options();
  bool      parse_option( const std::string& name, const std::string& value );
  bool      parse_options( int argc, char** argv );
  bool      time_to_think( double proc_time );
  double    total_reaction_time ();
  int       roll( double chance );
  double    range( double min, double max );
  double    gauss( double mean, double stddev );
  double    real();
  rng_t*    get_rng( const std::string& name, int type=RNG_DEFAULT );
  double    iteration_adjust();
  player_t* find_player( const std::string& name );
  player_t* find_player( int index );
  cooldown_t* get_cooldown( const std::string& name );
  void      use_optimal_buffs_and_debuffs( int value );
  void      aura_gain( const char* name, int aura_id=0 );
  void      aura_loss( const char* name, int aura_id=0 );
  action_expr_t* create_expression( action_t*, const std::string& name );
  int       errorf( const char* format, ... );
};

// Scaling ===================================================================

struct scaling_t
{
  sim_t* sim;
  sim_t* baseline_sim;
  sim_t* ref_sim;
  sim_t* delta_sim;
  sim_t* ref_sim2;
  sim_t* delta_sim2;
  int    scale_stat;
  double scale_value;
  double scale_delta_multiplier;
  int    calculate_scale_factors;
  int    center_scale_delta;
  int    positive_scale_delta;
  int    scale_lag;
  double scale_factor_noise;
  int    normalize_scale_factors;
  int    smooth_scale_factors;
  int    debug_scale_factors;
  std::string scale_only_str;
  int    current_scaling_stat, num_scaling_stats, remaining_scaling_stats;
  double    scale_haste_iterations, scale_expertise_iterations, scale_crit_iterations, scale_hit_iterations, scale_mastery_iterations;
  std::string scale_over;

  // Gear delta for determining scale factors
  gear_stats_t stats;

  scaling_t( sim_t* s );

  void init_deltas();
  void analyze();
  void analyze_stats();
  void analyze_lag();
  void analyze_gear_weights();
  void normalize();
  void derive();
  double progress( std::string& phase );
  void create_options();
  bool has_scale_factors();
  double scale_over_function( sim_t* s, player_t* p );
  double scale_over_function_error( sim_t* s, player_t* p );
};

// Plot ======================================================================

struct plot_t
{
  sim_t* sim;
  std::string dps_plot_stat_str;
  double dps_plot_step;
  int    dps_plot_points;
  int    dps_plot_iterations;
  int    dps_plot_debug;
  int    current_plot_stat, num_plot_stats, remaining_plot_stats;
  int    remaining_plot_points;
  bool	 dps_plot_positive;

  plot_t( sim_t* s );

  void analyze();
  void analyze_stats();
  double progress( std::string& phase );
  void create_options();
};

// Reforge Plot ===============================================================

struct reforge_plot_t
{
  sim_t* sim;
  std::string reforge_plot_stat_str;
  std::string reforge_plot_output_file_str;
  FILE* reforge_plot_output_file;
  std::vector<int> reforge_plot_stat_indices;
  int    reforge_plot_step;
  int    reforge_plot_amount;
  int    reforge_plot_iterations;
  int    reforge_plot_debug;
  int    current_stat_combo;
  int    num_stat_combos;

  reforge_plot_t( sim_t* s );

  void generate_stat_mods( std::vector<std::vector<int> > &stat_mods,
                           const std::vector<int> &stat_indices,
                           int cur_mod_stat,
                           std::vector<int> cur_stat_mods );
  void analyze();
  void analyze_stats();
  double progress( std::string& phase );
  void create_options();
};

// Event =====================================================================

struct event_t
{
  event_t*  next;
  sim_t*    sim;
  player_t* player;
  uint32_t  id;
  double    time;
  double    reschedule_time;
  int       canceled;
  const char* name;
  event_t( sim_t* s, player_t* p=0, const char* n=0 ) :
      next( 0 ), sim( s ), player( p ), reschedule_time( 0 ), canceled( 0 ), name( n )
  {
    if ( ! name ) name = "unknown";
  }
  double occurs()  SC_CONST { return ( reschedule_time != 0 ) ? reschedule_time : time; }
  double remains() SC_CONST { return occurs() - sim -> current_time; }
  virtual void reschedule( double new_time );
  virtual void execute() { util_t::printf( "%s\n", name ? name : "(no name)" ); assert( 0 ); }
  virtual ~event_t() {}
  static void cancel( event_t*& e );
  static void  early( event_t*& e );
  // Simple free-list memory manager.
  static void* operator new( size_t, sim_t* );
  static void* operator new( size_t ) throw();  // DO NOT USE!
  static void  operator delete( void* );
  static void  operator delete( void*, sim_t* ) {}
  static void deallocate( event_t* e );
};

struct event_compare_t
{
  bool operator () ( event_t* lhs, event_t* rhs ) SC_CONST
  {
    return( lhs -> time == rhs -> time ) ? ( lhs -> id > rhs -> id ) : ( lhs -> time > rhs -> time );
  }
};

// Gear Rating Conversions ===================================================

struct rating_t
{
  double  spell_haste,  spell_hit,  spell_crit;
  double attack_haste, attack_hit, attack_crit;
  double ranged_haste, ranged_hit,ranged_crit;
  double expertise;
  double dodge, parry, block;
  double mastery;
  rating_t() { memset( this, 0x00, sizeof( rating_t ) ); }
  void init( sim_t*, dbc_t& pData, int level, int type );
  static double interpolate( int level, double val_60, double val_70, double val_80, double val_85 = -1 );
  static double get_attribute_base( sim_t*, dbc_t& pData, int level, player_type class_type, race_type race, base_stat_type stat_type );
};

// Weapon ====================================================================

struct weapon_t
{
  int    type;
  school_type school;
  double damage, dps;
  double min_dmg, max_dmg;
  double swing_time;
  int    slot;
  int    buff_type;
  double buff_value;
  double bonus_dmg;

  int    group() SC_CONST;
  double normalized_weapon_speed() SC_CONST;
  double proc_chance_on_swing( double PPM, double adjusted_swing_time=0 ) SC_CONST;

  weapon_t( int t=WEAPON_NONE, double d=0, double st=2.0, school_type s=SCHOOL_PHYSICAL ) :
    type(t), school(s), damage(d), min_dmg(d), max_dmg(d), swing_time(st), slot(SLOT_NONE), buff_type(0), buff_value(0), bonus_dmg(0) { }
};

// Item ======================================================================

struct item_t
{
  sim_t* sim;
  player_t* player;
  int slot, quality, ilevel;
  bool unique, unique_enchant, unique_addon, is_heroic, is_ptr, is_matching_type, is_reforged;
  stat_type reforged_from;
  stat_type reforged_to;

  // Option Data
  std::string option_name_str;
  std::string option_id_str;
  std::string option_stats_str;
  std::string option_gems_str;
  std::string option_enchant_str;
  std::string option_addon_str;
  std::string option_equip_str;
  std::string option_use_str;
  std::string option_weapon_str;
  std::string option_heroic_str;
  std::string option_armor_type_str;
  std::string option_reforge_str;
  std::string option_random_suffix_str;
  std::string option_ilevel_str;
  std::string option_quality_str;
  std::string option_data_source_str;
  std::string options_str;

  // Armory Data
  std::string armory_name_str;
  std::string armory_id_str;
  std::string armory_stats_str;
  std::string armory_gems_str;
  std::string armory_enchant_str;
  std::string armory_addon_str;
  std::string armory_weapon_str;
  std::string armory_heroic_str;
  std::string armory_armor_type_str;
  std::string armory_reforge_str;
  std::string armory_ilevel_str;
  std::string armory_quality_str;
  std::string armory_random_suffix_str;

  // Encoded Data
  std::string id_str;
  std::string encoded_name_str;
  std::string encoded_stats_str;
  std::string encoded_gems_str;
  std::string encoded_enchant_str;
  std::string encoded_addon_str;
  std::string encoded_equip_str;
  std::string encoded_use_str;
  std::string encoded_weapon_str;
  std::string encoded_heroic_str;
  std::string encoded_armor_type_str;
  std::string encoded_reforge_str;
  std::string encoded_ilevel_str;
  std::string encoded_quality_str;
  std::string encoded_random_suffix_str;

  // Extracted data
  gear_stats_t base_stats,stats;
  struct special_effect_t
  {
    std::string name_str, trigger_str;
    int trigger_type;
    int64_t trigger_mask;
    int stat;
    school_type school;
    int max_stacks;
    double stat_amount, discharge_amount, discharge_scaling;
    double proc_chance, duration, cooldown, tick;
    bool cost_reduction;
    bool no_crit;
    bool no_player_benefits;
    bool no_debuffs;
    bool no_refresh;
    bool chance_to_discharge;
    bool reverse;
    special_effect_t() :
        trigger_type( 0 ), trigger_mask( 0 ), stat( 0 ), school( SCHOOL_NONE ),
        max_stacks( 0 ), stat_amount( 0 ), discharge_amount( 0 ), discharge_scaling( 0 ),
        proc_chance( 0 ), duration( 0 ), cooldown( 0 ),
        tick( 0 ), cost_reduction( false ), no_crit( false ), no_player_benefits( false ), no_debuffs( false ),
        no_refresh( false ), chance_to_discharge( false ), reverse( false ) {}
    bool active() { return stat || school; }
  } use, equip, enchant, addon;

  item_t() : sim( 0 ), player( 0 ), slot( SLOT_NONE ), quality( 0 ), ilevel( 0 ), unique( false ), unique_enchant( false ), unique_addon( false ), is_heroic( false ), is_matching_type( false ), is_reforged( false ) {}
  item_t( player_t*, const std::string& options_str );
  bool active() SC_CONST;
  bool heroic() SC_CONST;
  bool ptr() SC_CONST;
  bool reforged() SC_CONST;
  bool matching_type();
  const char* name() SC_CONST;
  const char* slot_name() SC_CONST;
  const char* armor_type();
  weapon_t* weapon() SC_CONST;
  bool init();
  bool parse_options();
  void encode_options();
  bool decode_stats();
  bool decode_gems();
  bool decode_enchant();
  bool decode_addon();
  bool decode_special( special_effect_t&, const std::string& encoding );
  bool decode_weapon();
  bool decode_heroic();
  bool decode_armor_type();
  bool decode_reforge();
  bool decode_random_suffix();
  bool decode_ilevel();
  bool decode_quality();

  static bool download_slot( item_t& item,
                             const std::string& item_id,
                             const std::string& enchant_id,
                             const std::string& addon_id,
                             const std::string& reforge_id,
                             const std::string& rsuffix_id,
                             const std::string gem_ids[ 3 ] );
  static bool download_item( item_t&, const std::string& item_id );
  static bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id );
  static int  parse_gem( item_t&            item,
                         const std::string& gem_id );
};

// Item database =============================================================
struct item_database_t
{
  static bool     download_slot(      item_t& item,
                                      const std::string& item_id,
                                      const std::string& enchant_id,
                                      const std::string& addon_id,
                                      const std::string& reforge_id,
                                      const std::string& rsuffix_id,
                                      const std::string gem_ids[ 3 ] );
  static bool     download_item(      item_t& item, const std::string& item_id );
  static bool     download_glyph(     player_t* player, std::string& glyph_name, const std::string& glyph_id );
  static int      parse_gem(          item_t& item, const std::string& gem_id );
  static bool     initialize_item_sources( const item_t& item, std::vector<std::string>& source_list );

  static int      random_suffix_type( const item_t& item );
  static uint32_t armor_value(        const item_t& item, unsigned item_id );
  static uint32_t armor_value(        const item_data_t*, const dbc_t& );
  static uint32_t weapon_dmg_min(     const item_t& item, unsigned item_id );
  static uint32_t weapon_dmg_min(     const item_data_t*, const dbc_t& );
  static uint32_t weapon_dmg_max(     const item_t& item, unsigned item_id );
  static uint32_t weapon_dmg_max(     const item_data_t*, const dbc_t& );
};

// Set Bonus =================================================================

struct set_bonus_t
{
  int count[ SET_MAX ];
  int tier11_2pc_caster() SC_CONST; int tier11_2pc_melee() SC_CONST; int tier11_2pc_tank() SC_CONST; int tier11_2pc_heal() SC_CONST;
  int tier11_4pc_caster() SC_CONST; int tier11_4pc_melee() SC_CONST; int tier11_4pc_tank() SC_CONST; int tier11_4pc_heal() SC_CONST;
  int tier12_2pc_caster() SC_CONST; int tier12_2pc_melee() SC_CONST; int tier12_2pc_tank() SC_CONST; int tier12_2pc_heal() SC_CONST;
  int tier12_4pc_caster() SC_CONST; int tier12_4pc_melee() SC_CONST; int tier12_4pc_tank() SC_CONST; int tier12_4pc_heal() SC_CONST;
  int tier13_2pc_caster() SC_CONST; int tier13_2pc_melee() SC_CONST; int tier13_2pc_tank() SC_CONST; int tier13_2pc_heal() SC_CONST;
  int tier13_4pc_caster() SC_CONST; int tier13_4pc_melee() SC_CONST; int tier13_4pc_tank() SC_CONST; int tier13_4pc_heal() SC_CONST;
  int tier14_2pc_caster() SC_CONST; int tier14_2pc_melee() SC_CONST; int tier14_2pc_tank() SC_CONST; int tier14_2pc_heal() SC_CONST;
  int tier14_4pc_caster() SC_CONST; int tier14_4pc_melee() SC_CONST; int tier14_4pc_tank() SC_CONST; int tier14_4pc_heal() SC_CONST;
  int decode( player_t*, item_t& item ) SC_CONST;
  bool init( player_t* );
  set_bonus_t();

  action_expr_t* create_expression( action_t*, const std::string& type );
};

struct set_bonus_array_t
{
  const spell_id_t* set_bonuses[ SET_MAX ];
  const spell_id_t* default_value;
  player_t*         p;

  set_bonus_array_t( player_t* p, uint32_t a_bonus[ N_TIER ][ N_TIER_BONUS ] );
  virtual ~set_bonus_array_t();

  virtual bool              has_set_bonus( set_type s ) SC_CONST;
  virtual const spell_id_t* set( set_type s ) SC_CONST;
  virtual const spell_id_t* create_set_bonus( player_t* p, uint32_t spell_id ) SC_CONST;
};

// Player ====================================================================

struct player_t
{
  sim_t*      sim;
  bool        ptr;
  std::string name_str, talents_str, glyphs_str, id_str, target_str;
  std::string region_str, server_str, origin_str;
  player_t*   next;
  int         index;
  player_type type;
  role_type   role;
  player_t*   target;
  int         level, use_pre_potion, party, member;
  double      skill, initial_skill, distance, default_distance, gcd_ready, base_gcd;
  int         potion_used, sleeping, initialized;
  rating_t    rating;
  pet_t*      pet_list;
  int64_t     last_modified;
  int         bugs;
  int         specialization;
  int         invert_scaling;
  bool        vengeance_enabled;
  double      vengeance_damage, vengeance_value, vengeance_max; // a percentage of maximum possible vengeance (i.e. 1.0 means 10% of your health)
  int         active_pets;
  double      dtr_proc_chance;
  double      dtr_base_proc_chance;
  double      reaction_mean,reaction_stddev,reaction_nu;
  int         infinite_resource[ RESOURCE_MAX ];

  // Latency
  double      world_lag, world_lag_stddev;
  double      brain_lag, brain_lag_stddev;
  bool        world_lag_override, world_lag_stddev_override;

  int    events;

  // Data access
  dbc_t       dbc;

  // Option Parsing
  std::vector<option_t> options;

  // Talent Parsing
  int tree_type[ MAX_TALENT_TREES ];
  int talent_tab_points[ MAX_TALENT_TREES ];
  std::vector<talent_t*> talent_trees[ MAX_TALENT_TREES ];
  std::vector<glyph_t*> glyphs;

  std::list<spell_id_t*> spell_list;

  // Profs
  std::string professions_str;
  int profession[ PROFESSION_MAX ];

  // Race
  std::string race_str;
  race_type race;

  // Haste
  double base_haste_rating, initial_haste_rating, haste_rating;
  double spell_haste, buffed_spell_haste;
  double attack_haste, buffed_attack_haste;

  // Attributes
  double attribute                   [ ATTRIBUTE_MAX ];
  double attribute_base              [ ATTRIBUTE_MAX ];
  double attribute_initial           [ ATTRIBUTE_MAX ];
  double attribute_multiplier        [ ATTRIBUTE_MAX ];
  double attribute_multiplier_initial[ ATTRIBUTE_MAX ];
  double attribute_buffed            [ ATTRIBUTE_MAX ];

  double mastery, buffed_mastery, mastery_rating, initial_mastery_rating,base_mastery;

  // Spell Mechanics
  double base_spell_power,       initial_spell_power[ SCHOOL_MAX+1 ], spell_power[ SCHOOL_MAX+1 ], buffed_spell_power;
  double base_spell_hit,         initial_spell_hit,                   spell_hit,                   buffed_spell_hit;
  double base_spell_crit,        initial_spell_crit,                  spell_crit,                  buffed_spell_crit;
  double base_spell_penetration, initial_spell_penetration,           spell_penetration,           buffed_spell_penetration;
  double base_mp5,               initial_mp5,                         mp5,                         buffed_mp5;
  double spell_power_multiplier,    initial_spell_power_multiplier;
  double spell_power_per_intellect, initial_spell_power_per_intellect;
  double spell_power_per_spirit,    initial_spell_power_per_spirit;
  double spell_crit_per_intellect,  initial_spell_crit_per_intellect;
  double mp5_per_intellect;
  double mana_regen_base;
  double mana_regen_while_casting;
  double base_energy_regen_per_second;
  double base_focus_regen_per_second;
  double resource_reduction[ SCHOOL_MAX ], initial_resource_reduction[ SCHOOL_MAX ];
  double last_cast;

  // Attack Mechanics
  double base_attack_power,       initial_attack_power,        attack_power,       buffed_attack_power;
  double base_attack_hit,         initial_attack_hit,          attack_hit,         buffed_attack_hit;
  double base_attack_expertise,   initial_attack_expertise,    attack_expertise,   buffed_attack_expertise;
  double base_attack_crit,        initial_attack_crit,         attack_crit,        buffed_attack_crit;
  double attack_power_multiplier,   initial_attack_power_multiplier;
  double attack_power_per_strength, initial_attack_power_per_strength;
  double attack_power_per_agility,  initial_attack_power_per_agility;
  double attack_crit_per_agility,   initial_attack_crit_per_agility;
  int    position;

  // Defense Mechanics
  event_t* target_auto_attack;
  double base_armor,       initial_armor,       armor,       buffed_armor;
  double base_bonus_armor, initial_bonus_armor, bonus_armor;
  double base_miss,        initial_miss,        miss,        buffed_miss, buffed_crit;
  double base_dodge,       initial_dodge,       dodge,       buffed_dodge;
  double base_parry,       initial_parry,       parry,       buffed_parry;
  double base_block,       initial_block,       block,       buffed_block;
  double armor_multiplier,  initial_armor_multiplier;
  double dodge_per_agility, initial_dodge_per_agility;
  double parry_rating_per_strength, initial_parry_rating_per_strength;
  double diminished_dodge_capi, diminished_parry_capi, diminished_kfactor;
  double armor_coeff;
  double half_resistance_rating;
  int spell_resistance[ SCHOOL_MAX ];

  // Weapons
  weapon_t main_hand_weapon;
  weapon_t off_hand_weapon;
  weapon_t ranged_weapon;

  // Main, offhand, and ranged attacks
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;
  attack_t* ranged_attack;


  // Resources
  double  resource_base   [ RESOURCE_MAX ];
  double  resource_initial[ RESOURCE_MAX ];
  double  resource_max    [ RESOURCE_MAX ];
  double  resource_current[ RESOURCE_MAX ];
  double  resource_buffed [ RESOURCE_MAX ];
  double  mana_per_intellect;
  double  health_per_stamina;

  // Replenishment
  std::vector<player_t*> replenishment_targets;

  // Callbacks
  action_callback_t* dark_intent_cb;

  // Consumables
  std::string flask_str, elixirs_str, food_str;
  int elixir_guardian;
  int elixir_battle;
  int flask;
  int food;

  // Events
  action_t* executing;
  action_t* channeling;
  event_t*  readying;
  bool      in_combat;
  bool      action_queued;

  // Delay time used by "cast_delay" expression to determine when an action
  // can be used at minimum after a spell cast has finished, including GCD
  double    cast_delay_reaction;
  double    cast_delay_occurred;

  // Callbacks
  std::vector<action_callback_t*> all_callbacks;
  std::vector<action_callback_t*> attack_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*>  spell_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*>  heal_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*>   tick_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*> direct_damage_callbacks[ SCHOOL_MAX ];
  std::vector<action_callback_t*>   tick_damage_callbacks[ SCHOOL_MAX ];
  std::vector<action_callback_t*>   direct_heal_callbacks[ SCHOOL_MAX ];
  std::vector<action_callback_t*>     tick_heal_callbacks[ SCHOOL_MAX ];
  std::vector<action_callback_t*> resource_gain_callbacks[ RESOURCE_MAX ];
  std::vector<action_callback_t*> resource_loss_callbacks[ RESOURCE_MAX ];

  // Action Priority List
  action_t*   action_list;
  std::string action_list_str;
  std::string action_list_skip;
  std::string modify_action;
  int         action_list_default;
  cooldown_t* cooldown_list;
  dot_t*      dot_list;
  std::map<std::string,int> action_map;

  // Reporting
  int       quiet;
  action_t* last_foreground_action;
  double    current_time, total_seconds;
  double    total_waiting, total_foreground_actions;
  double    iteration_dmg, total_dmg;
  double    resource_lost  [ RESOURCE_MAX ];
  double    resource_gained[ RESOURCE_MAX ];
  double    dps, dpse, dps_min, dps_max, dps_std_dev, dps_error, dps_convergence;
  double    dps_10_percentile,dps_90_percentile;
  double    dpr, rps_gain, rps_loss;
  int       death_count;
  std::vector<double> death_time;
  double    avg_death_time, death_count_pct, min_death_time;
  double    dmg_taken, total_dmg_taken;
  buff_t*   buff_list;
  proc_t*   proc_list;
  gain_t*   gain_list;
  stats_t*  stats_list;
  uptime_t* uptime_list;
  std::vector<double> dps_plot_data[ STAT_MAX ];
  std::vector<std::vector<double> > reforge_plot_data;
  std::vector<double> timeline_resource;
  std::vector<double> timeline_health;
  std::vector<double> timeline_dmg;
  std::vector<double> timeline_dps;
  std::vector<double> iteration_dps;
  std::vector<int> distribution_dps;
  std::vector<double> dps_convergence_error;
  std::string action_sequence;
  std::string action_dpet_chart, action_dmg_chart, gains_chart;
  std::string timeline_resource_chart, timeline_dps_chart, timeline_dps_error_chart, timeline_resource_health_chart;
  std::string distribution_dps_chart, scaling_dps_chart, scale_factors_chart;
  std::string reforge_dps_chart, dps_error_chart;
  std::string gear_weights_lootrank_link, gear_weights_wowhead_link, gear_weights_wowreforge_link;
  std::string gear_weights_pawn_std_string, gear_weights_pawn_alt_string;
  std::string save_str;
  std::string save_gear_str;
  std::string save_talents_str;
  std::string save_actions_str;
  std::string comment_str;

  // Gear
  std::string items_str, meta_gem_str;
  std::vector<item_t> items;
  gear_stats_t stats, initial_stats, gear, enchant;
  set_bonus_t set_bonus;
  set_bonus_array_t * sets;
  int meta_gem;
  bool matching_gear;

  // Scale Factors
  gear_stats_t scaling;
  gear_stats_t scaling_normalized;
  gear_stats_t scaling_error;
  double       scaling_lag;
  int          scales_with[ STAT_MAX ];
  double       over_cap[ STAT_MAX ];

  // Movement & Position
  double base_movement_speed;
  double x_position, y_position;

  struct buffs_t
  {
    buff_t* arcane_brilliance;
    buff_t* berserking;
    buff_t* blessing_of_kings;
    buff_t* blessing_of_might;
    buff_t* blessing_of_might_regen; // the mana regen part of BoM
    buff_t* blood_fury_ap;
    buff_t* blood_fury_sp;
    buff_t* bloodlust;
    buff_t* body_and_soul;
    buff_t* corruption_absolute;
    buff_t* dark_intent;
    buff_t* dark_intent_feedback;
    buff_t* destruction_potion;
    buff_t* earthen_potion;
    buff_t* essence_of_the_red;
    buff_t* exhaustion;
    buff_t* focus_magic;
    buff_t* fortitude;
    buff_t* furious_howl;
    buff_t* golemblood_potion;
    buff_t* grace;
    buff_t* hellscreams_warsong;
    buff_t* heroic_presence;
    buff_t* hymn_of_hope;
    buff_t* indestructible_potion;
    buff_t* innervate;
    buff_t* inspiration;
    buff_t* lifeblood;
    buff_t* mana_tide;
    buff_t* mark_of_the_wild;
    buff_t* mongoose_mh;
    buff_t* mongoose_oh;
    buff_t* power_infusion;
    buff_t* replenishment;
    buff_t* raid_movement;
    buff_t* self_movement;
    buff_t* speed_potion;
    buff_t* stoneform;
    buff_t* strength_of_wrynn;
    buff_t* stunned;
    buff_t* tolvir_potion;
    buff_t* tricks_of_the_trade;
    buff_t* unholy_frenzy;
    buff_t* volcanic_potion;
    buff_t* weakened_soul;
    buff_t* wild_magic_potion_crit;
    buff_t* wild_magic_potion_sp;
    buff_t* blessing_of_ancient_kings;

    buffs_t() { memset( (void*) this, 0x0, sizeof( buffs_t ) ); }
  };
  buffs_t buffs;

  struct debuffs_t
  {
    debuff_t* bleeding;
    debuff_t* blood_frenzy_bleed;
    debuff_t* blood_frenzy_physical;
    debuff_t* brittle_bones;
    debuff_t* casting;
    debuff_t* corrosive_spit;
    debuff_t* critical_mass;
    debuff_t* curse_of_elements;
    debuff_t* demoralizing_roar;
    debuff_t* demoralizing_screech;
    debuff_t* demoralizing_shout;
    debuff_t* earth_and_moon;
    debuff_t* ebon_plaguebringer;
    debuff_t* expose_armor;
    debuff_t* faerie_fire;
    debuff_t* hemorrhage;
    debuff_t* hunters_mark;
    debuff_t* infected_wounds;
    debuff_t* insect_swarm;
    debuff_t* invulnerable;
    debuff_t* judgements_of_the_just;
    debuff_t* lightning_breath;
    debuff_t* mangle;
    debuff_t* master_poisoner;
    debuff_t* poisoned;
    debuff_t* ravage;
    debuff_t* savage_combat;
    debuff_t* scarlet_fever;
    debuff_t* shadow_and_flame;
    debuff_t* shattering_throw;
    debuff_t* slow;
    debuff_t* sunder_armor;
    debuff_t* tailspin;
    debuff_t* tear_armor;
    debuff_t* tendon_rip;
    debuff_t* thunder_clap;
    debuff_t* vindication;
    debuff_t* vulnerable;

    debuffs_t() { memset( (void*) this, 0x0, sizeof( debuffs_t ) ); }
    bool snared();
  };
  debuffs_t debuffs;

  struct gains_t
  {
    gain_t* arcane_torrent;
    gain_t* blessing_of_might;
    gain_t* dark_rune;
    gain_t* energy_regen;
    gain_t* essence_of_the_red;
    gain_t* focus_regen;
    gain_t* innervate;
    gain_t* glyph_of_innervate;
    gain_t* mana_potion;
    gain_t* mana_spring_totem;
    gain_t* mp5_regen;
    gain_t* replenishment;
    gain_t* restore_mana;
    gain_t* spellsurge;
    gain_t* spirit_intellect_regen;
    gain_t* vampiric_embrace;
    gain_t* vampiric_touch;
    gain_t* water_elemental;
    gain_t* hymn_of_hope;
    void reset() { memset( ( void* ) this, 0x00, sizeof( gains_t ) ); }
    gains_t() { reset(); }
  };
  gains_t gains;

  struct procs_t
  {
    proc_t* hat_donor;
    void reset() { memset( ( void* ) this, 0x00, sizeof( procs_t ) ); }
    procs_t() { reset(); }
  };
  procs_t procs;

  rng_t* rng_list;

  struct rngs_t
  {
    rng_t* lag_channel;
    rng_t* lag_gcd;
    rng_t* lag_queue;
    rng_t* lag_ability;
    rng_t* lag_reaction;
    rng_t* lag_world;
    rng_t* lag_brain;
    void reset() { memset( ( void* ) this, 0x00, sizeof( rngs_t ) ); }
    rngs_t() { reset(); }
  };
  rngs_t rngs;


  player_t( sim_t* sim, player_type type, const std::string& name, race_type race_type = RACE_NONE );

  virtual ~player_t();

  virtual const char* name() SC_CONST { return name_str.c_str(); }

  virtual void init();
  virtual void init_glyphs();
  virtual void init_base() = 0;
  virtual void init_items();
  virtual void init_meta_gem( gear_stats_t& );
  virtual void init_core();
  virtual void init_race();
  virtual void init_racials();
  virtual void init_spell();
  virtual void init_attack();
  virtual void init_defense();
  virtual void init_weapon( weapon_t* );
  virtual void init_unique_gear();
  virtual void init_enchant();
  virtual void init_resources( bool force = false );
  virtual void init_consumables();
  virtual void init_professions();
  virtual void init_use_item_actions( const std::string& append = std::string() );
  virtual void init_use_profession_actions( const std::string& append = std::string() );
  virtual void init_use_racial_actions( const std::string& append = std::string() );
  virtual void init_actions();
  virtual void init_rating();
  virtual void init_scaling();
  virtual void init_talents();
  virtual void init_spells();
  virtual void init_buffs();
  virtual void init_gains();
  virtual void init_procs();
  virtual void init_uptimes();
  virtual void init_rng();
  virtual void init_stats();
  virtual void init_values();
  virtual void init_target();

  virtual void reset();
  virtual void combat_begin();
  virtual void combat_end();

  virtual double composite_mastery() SC_CONST { return floor( ( mastery * 100.0 ) + 0.5 ) * 0.01; }

  virtual double energy_regen_per_second() SC_CONST;
  virtual double focus_regen_per_second() SC_CONST;
  virtual double composite_attack_haste() SC_CONST;
  virtual double composite_attack_speed() SC_CONST;
  virtual double composite_attack_power() SC_CONST;
  virtual double composite_attack_crit() SC_CONST;
  virtual double composite_attack_expertise() SC_CONST { return attack_expertise; }
  virtual double composite_attack_hit() SC_CONST;

  virtual double composite_spell_haste() SC_CONST;
  virtual double composite_spell_power( const school_type school ) SC_CONST;
  virtual double composite_spell_crit() SC_CONST;
  virtual double composite_spell_hit() SC_CONST;
  virtual double composite_spell_penetration() SC_CONST { return spell_penetration; }
  virtual double composite_mp5() SC_CONST;


  virtual double composite_armor()                 SC_CONST;
  virtual double composite_armor_multiplier()      SC_CONST;
  virtual double composite_spell_resistance( const school_type school ) SC_CONST;
  virtual double composite_tank_miss( const school_type school ) SC_CONST;
  virtual double composite_tank_dodge()            SC_CONST;
  virtual double composite_tank_parry()            SC_CONST;
  virtual double composite_tank_block()            SC_CONST;
  virtual double composite_tank_crit_block()            SC_CONST;
  virtual double composite_tank_crit( const school_type school ) SC_CONST;

  virtual double diminished_dodge()             SC_CONST;
  virtual double diminished_parry()             SC_CONST;

  virtual double composite_attack_power_multiplier() SC_CONST;
  virtual double composite_spell_power_multiplier() SC_CONST;
  virtual double composite_attribute_multiplier( int attr ) SC_CONST;

  virtual double matching_gear_multiplier( const attribute_type attr ) SC_CONST { return 0.0; }

  virtual double composite_player_multiplier   ( const school_type school, action_t* a = NULL ) SC_CONST;
  virtual double composite_player_dd_multiplier( const school_type school, action_t* a = NULL ) SC_CONST { return 1.0; }
  virtual double composite_player_td_multiplier( const school_type school, action_t* a = NULL ) SC_CONST;

  virtual double composite_player_heal_multiplier   ( const school_type school ) SC_CONST;
  virtual double composite_player_dh_multiplier( const school_type school ) SC_CONST { return 1.0; }
  virtual double composite_player_th_multiplier( const school_type school ) SC_CONST;

  virtual double composite_player_absorb_multiplier   ( const school_type school ) SC_CONST;

  virtual double composite_movement_speed() SC_CONST;

  virtual double strength() SC_CONST;
  virtual double agility() SC_CONST;
  virtual double stamina() SC_CONST;
  virtual double intellect() SC_CONST;
  virtual double spirit() SC_CONST;

  virtual void      interrupt();
  virtual void      halt();
  virtual void      moving();
  virtual void      stun();
  virtual void      clear_debuffs();
  virtual void      schedule_ready( double delta_time=0, bool waiting=false );
  virtual void      arise();
  virtual void      demise();
  virtual double    available() SC_CONST { return 0.1; }
  virtual action_t* execute_action();

  virtual std::string print_action_map( int iterations, int precision );

  virtual void   regen( double periodicity=2.0 );
  virtual double resource_gain( int resource, double amount, gain_t* g=0, action_t* a=0 );
  virtual double resource_loss( int resource, double amount, action_t* a=0 );
  virtual bool   resource_available( int resource, double cost ) SC_CONST;
  virtual int    primary_resource() SC_CONST { return RESOURCE_NONE; }
  virtual int    primary_role() SC_CONST;
  virtual int    primary_tree() SC_CONST;
  virtual int    primary_tab();
  virtual const char* primary_tree_name() SC_CONST;
  virtual int    normalize_by() SC_CONST;

  virtual double health_percentage() SC_CONST;
  virtual double time_to_die() SC_CONST;
  virtual double total_reaction_time() SC_CONST;

  virtual void stat_gain( int stat, double amount, gain_t* g=0, action_t* a=0 );
  virtual void stat_loss( int stat, double amount, action_t* a=0 );

  virtual void cost_reduction_gain( int school, double amount, gain_t* g=0, action_t* a=0 );
  virtual void cost_reduction_loss( int school, double amount, action_t* a=0 );

  virtual double assess_damage( double amount, const school_type school, int type, int result, action_t* a=0 );
  virtual double target_mitigation( double amount, const school_type school, int type, int result, action_t* a=0 );

  virtual void  summon_pet( const char* name, double duration=0 );
  virtual void dismiss_pet( const char* name );

  virtual bool ooc_buffs() { return true; }

  virtual bool is_moving() { return buffs.raid_movement -> check() || buffs.self_movement -> check(); }

  virtual void register_callbacks();
  virtual void register_resource_gain_callback( int resource,        action_callback_t* );
  virtual void register_resource_loss_callback( int resource,        action_callback_t* );
  virtual void register_attack_callback       ( int64_t result_mask, action_callback_t* );
  virtual void register_spell_callback        ( int64_t result_mask, action_callback_t* );
  virtual void register_tick_callback         ( int64_t result_mask, action_callback_t* );
  virtual void register_heal_callback         ( int64_t result_mask, action_callback_t* );
  virtual void register_harmful_spell_callback( int64_t result_mask, action_callback_t* );
  virtual void register_tick_damage_callback  ( int64_t result_mask, action_callback_t* );
  virtual void register_direct_damage_callback( int64_t result_mask, action_callback_t* );
  virtual void register_tick_heal_callback    ( int64_t result_mask, action_callback_t* );
  virtual void register_direct_heal_callback  ( int64_t result_mask, action_callback_t* );

  virtual bool parse_talent_trees( int talents[], const uint32_t size );
  virtual bool parse_talents_armory ( const std::string& talent_string );
  virtual bool parse_talents_wowhead( const std::string& talent_string );

  virtual void create_talents();
  virtual void create_glyphs();

  virtual talent_t* find_talent( const std::string& name, int tree = TALENT_TAB_NONE );
  virtual glyph_t*  find_glyph ( const std::string& name );

  virtual action_expr_t* create_expression( action_t*, const std::string& name );

  virtual void create_options();
  virtual bool create_profile( std::string& profile_str, int save_type=SAVE_ALL, bool save_html=false );

  virtual void copy_from( player_t* source );

  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual void      create_pets() { }
  virtual pet_t*    create_pet( const std::string& name,  const std::string& type = std::string() ) { return 0; }
  virtual pet_t*    find_pet  ( const std::string& name );

  virtual void trigger_replenishment();

  virtual int decode_set( item_t& item ) { assert( item.name() ); return SET_NONE; }

  virtual void recalculate_haste();

  virtual void armory_extensions( const std::string& region, const std::string& server, const std::string& character ) {}

  // Class-Specific Methods

  static player_t* create( sim_t* sim, const std::string& type, const std::string& name, race_type r = RACE_NONE );

  static player_t * create_death_knight( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_druid       ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_hunter      ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_mage        ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_paladin     ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_priest      ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_rogue       ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_shaman      ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_warlock     ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_warrior     ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t * create_enemy       ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );

  // Raid-wide aura/buff/debuff maintenance
  static bool init        ( sim_t* sim );
  static void combat_begin( sim_t* sim );
  static void combat_end  ( sim_t* sim );

  // Raid-wide Death Knight buff maintenance
  static void death_knight_init        ( sim_t* sim );
  static void death_knight_combat_begin( sim_t* sim );
  static void death_knight_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Druid buff maintenance
  static void druid_init        ( sim_t* sim );
  static void druid_combat_begin( sim_t* sim );
  static void druid_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Hunter buff maintenance
  static void hunter_init        ( sim_t* sim );
  static void hunter_combat_begin( sim_t* sim );
  static void hunter_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Mage buff maintenance
  static void mage_init        ( sim_t* sim );
  static void mage_combat_begin( sim_t* sim );
  static void mage_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Paladin buff maintenance
  static void paladin_init        ( sim_t* sim );
  static void paladin_combat_begin( sim_t* sim );
  static void paladin_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Priest buff maintenance
  static void priest_init        ( sim_t* sim );
  static void priest_combat_begin( sim_t* sim );
  static void priest_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Rogue buff maintenance
  static void rogue_init        ( sim_t* sim );
  static void rogue_combat_begin( sim_t* sim );
  static void rogue_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Shaman buff maintenance
  static void shaman_init        ( sim_t* sim );
  static void shaman_combat_begin( sim_t* sim );
  static void shaman_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Warlock buff maintenance
  static void warlock_init        ( sim_t* sim );
  static void warlock_combat_begin( sim_t* sim );
  static void warlock_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Warrior buff maintenance
  static void warrior_init        ( sim_t* sim );
  static void warrior_combat_begin( sim_t* sim );
  static void warrior_combat_end  ( sim_t* sim ) { assert( sim ); }

  // Raid-wide Enemy buff maintenance
  static void enemy_init        ( sim_t* sim );
  static void enemy_combat_begin( sim_t* sim );
  static void enemy_combat_end  ( sim_t* sim ) { assert( sim ); }

  bool is_pet() SC_CONST { return type == PLAYER_PET || type == PLAYER_GUARDIAN || type == ENEMY_ADD; }
  bool is_enemy() SC_CONST { return type == ENEMY; }
  bool is_add() SC_CONST { return type == ENEMY_ADD; }

  death_knight_t* cast_death_knight() { assert( type == DEATH_KNIGHT ); return ( death_knight_t* ) this; }
  druid_t       * cast_druid       () { assert( type == DRUID        ); return ( druid_t       * ) this; }
  hunter_t      * cast_hunter      () { assert( type == HUNTER       ); return ( hunter_t      * ) this; }
  mage_t        * cast_mage        () { assert( type == MAGE         ); return ( mage_t        * ) this; }
  paladin_t     * cast_paladin     () { assert( type == PALADIN      ); return ( paladin_t     * ) this; }
  priest_t      * cast_priest      () { assert( type == PRIEST       ); return ( priest_t      * ) this; }
  rogue_t       * cast_rogue       () { assert( type == ROGUE        ); return ( rogue_t       * ) this; }
  shaman_t      * cast_shaman      () { assert( type == SHAMAN       ); return ( shaman_t      * ) this; }
  warlock_t     * cast_warlock     () { assert( type == WARLOCK      ); return ( warlock_t     * ) this; }
  warrior_t     * cast_warrior     () { assert( type == WARRIOR      ); return ( warrior_t     * ) this; }
  pet_t         * cast_pet         () { assert( is_pet()             ); return ( pet_t         * ) this; }
  enemy_t       * cast_enemy       () { assert( type == ENEMY        ); return ( enemy_t       * ) this; }

  bool      in_gcd() SC_CONST { return gcd_ready > sim -> current_time; }
  bool      recent_cast();
  item_t*   find_item( const std::string& );
  action_t* find_action( const std::string& );
  double    mana_regen_per_second();
  bool      dual_wield() SC_CONST { return main_hand_weapon.type != WEAPON_NONE && off_hand_weapon.type != WEAPON_NONE; }
  void      aura_gain( const char* name, double value=0 );
  void      aura_loss( const char* name, double value=0 );

  cooldown_t* find_cooldown( const std::string& name );
  dot_t*      find_dot     ( const std::string& name );

  cooldown_t* get_cooldown( const std::string& name );
  dot_t*      get_dot     ( const std::string& name );
  gain_t*     get_gain    ( const std::string& name );
  proc_t*     get_proc    ( const std::string& name );
  stats_t*    get_stats   ( const std::string& name, action_t* action=0 );
  uptime_t*   get_uptime  ( const std::string& name );
  rng_t*      get_rng     ( const std::string& name, int type=RNG_DEFAULT );
  double      get_player_distance( player_t* p );
  double      get_position_distance( double m=0, double v=0 );
};

// Pet =======================================================================

struct pet_t : public player_t
{
  std::string full_name_str;
  player_t* owner;
  pet_t* next_pet;
  double stamina_per_owner;
  double intellect_per_owner;
  double summon_time;
  bool summoned;
  pet_type_t pet_type;

  void _init_pet_t();
  pet_t( sim_t* sim, player_t* owner, const std::string& name, bool guardian=false );
  pet_t( sim_t* sim, player_t* owner, const std::string& name, pet_type_t pt, bool guardian=false );

  // Pets gain their owners' hit rating, but it rounds down to a
  // percentage.  Also, heroic presence does not contribute to pet
  // expertise, so we use raw attack_hit.
  virtual double composite_attack_expertise() SC_CONST { return floor(floor(100.0 * owner -> attack_hit) * 26.0 / 8.0) / 100.0; }
  virtual double composite_attack_hit()       SC_CONST { return floor(100.0 * owner -> composite_attack_hit()) / 100.0; }
  virtual double composite_spell_hit()        SC_CONST { return floor(100.0 * owner -> composite_spell_hit()) / 100.0;  }

  virtual double stamina() SC_CONST;
  virtual double intellect() SC_CONST;

  virtual void init();
  virtual void init_base();
  virtual void init_talents();
  virtual void init_target();
  virtual void reset();
  virtual void summon( double duration=0 );
  virtual void dismiss();
  virtual bool ooc_buffs() { return false; }

  virtual double assess_damage( double amount, const school_type school, int type, int result, action_t* a=0 );

  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual const char* name() SC_CONST { return full_name_str.c_str(); }
  virtual const char* id();
};

// Stats =====================================================================

struct stats_t
{
  std::string name_str;
  sim_t* sim;
  player_t* player;
  stats_t* next;
  stats_t* parent;
  school_type school;
  stats_type type;
  std::vector<action_t*> action_list;
  bool analyzed;
  bool initialized;
  bool quiet;

  int resource;
  double resource_consumed, resource_portion;
  double frequency, num_executes, num_ticks;
  double num_direct_results, num_tick_results;
  double total_execute_time, total_tick_time;
  double total_dmg, portion_dmg;
  double dps, portion_dps, dpe, dpet, dpr, rpe, etpe, ttpt;
  double total_intervals, num_intervals;
  double last_execute;

  std::vector<stats_t*> children;
  double compound_dmg;
  double opportunity_cost;

  struct stats_results_t
  {
    double count, min_dmg, max_dmg, avg_dmg, total_dmg, pct;
  };
  stats_results_t direct_results[ RESULT_MAX ];
  stats_results_t   tick_results[ RESULT_MAX ];

  int num_buckets;
  std::vector<double> timeline_dmg;
  std::vector<double> timeline_dps;

  void add_child( stats_t* child );
  void consume_resource( double r ) { resource_consumed += r; }
  void add_result( double amount, int dmg_type, int result );
  void add_tick   ( double time );
  void add_execute( double time );
  void init();
  void reset();
  void analyze();
  void merge( stats_t* other );
  stats_t( const std::string& name, player_t* );
};

// Rank ======================================================================

struct rank_t
{
  int level, index;
  double dd_min, dd_max, tick, cost;
};

// Base Stats ================================================================

struct base_stats_t
{
  int level;
  double health, mana;
  double strength, agility, stamina, intellect, spirit;
  double spell_crit, melee_crit;
};

// Action ====================================================================

struct action_t : public spell_id_t
{
  sim_t* sim;
  int type;
  std::string name_str;
  player_t* player;
  player_t* target;
  uint32_t id;
  school_type school;
  int resource, tree, result, aoe;
  bool dual, callbacks, special, binary, channeled, background, sequence;
  bool direct_tick, repeating, harmful, proc, item_proc, may_trigger_dtr, discharge_proc, auto_cast, initialized;
  bool may_hit, may_miss, may_resist, may_dodge, may_parry, may_glance, may_block, may_crush, may_crit;
  bool tick_may_crit, tick_zero, hasted_ticks;
  bool no_buffs, no_debuffs;
  int dot_behavior;
  double ability_lag, ability_lag_stddev;
  double rp_gain;
  double min_gcd, trigger_gcd, range;
  double weapon_power_mod, direct_power_mod, tick_power_mod;
  double base_execute_time, base_tick_time, base_cost;
  double base_dd_min, base_dd_max, base_td, base_td_init;
  double   base_dd_multiplier,   base_td_multiplier;
  double player_dd_multiplier, player_td_multiplier;
  double   base_multiplier,   base_hit,   base_crit,   base_penetration;
  double player_multiplier, player_hit, player_crit, player_penetration;
  double target_multiplier, target_hit, target_crit, target_penetration;
  double   base_spell_power,   base_attack_power;
  double player_spell_power, player_attack_power;
  double target_spell_power, target_attack_power;
  double   base_spell_power_multiplier,   base_attack_power_multiplier;
  double player_spell_power_multiplier, player_attack_power_multiplier;
  double crit_multiplier, crit_bonus_multiplier, crit_bonus;
  double base_dd_adder, player_dd_adder, target_dd_adder;
  double player_haste;
  double resource_consumed;
  double direct_dmg, tick_dmg;
  double snapshot_crit, snapshot_haste, snapshot_mastery;
  int num_ticks;
  weapon_t* weapon;
  double weapon_multiplier;
  double base_add_multiplier;
  bool normalize_weapon_speed;
  rng_t* rng[ RESULT_MAX ];
  rng_t* rng_travel;
  cooldown_t* cooldown;
  dot_t* dot;
  stats_t* stats;
  event_t* execute_event;
  event_t* travel_event;
  double time_to_execute, time_to_tick, time_to_travel, travel_speed;
  int rank_index, bloodlust_active;
  double max_haste;
  double haste_gain_percentage;
  double min_current_time, max_current_time;
  double min_health_percentage, max_health_percentage;
  int moving, vulnerable, invulnerable, wait_on_ready, interrupt;
  bool round_base_dmg;
  bool class_flag1;
  std::string if_expr_str;
  action_expr_t* if_expr;
  std::string interrupt_if_expr_str;
  action_expr_t* interrupt_if_expr;
  std::string sync_str;
  action_t* sync_action;
  action_t* next;
  char marker;
  std::string signature_str;
  std::string target_str;
  std::string label_str;
  double last_reaction_time;

  action_t( int type, const char* name, player_t* p=0, int r=RESOURCE_NONE, const school_type s=SCHOOL_NONE, int t=TREE_NONE, bool special=false );
  action_t( int type, const active_spell_t& s, int t=TREE_NONE, bool special=false );
  action_t( int type, const char* name, const char* sname, player_t* p=0, int t=TREE_NONE, bool special=false );
  action_t( int type, const char* name, const uint32_t id, player_t* p=0, int t=TREE_NONE, bool special=false );
  virtual ~action_t();

  virtual void _init_action_t();

  virtual void      parse_data();
  virtual void      parse_effect_data( int spell_id, int effect_nr );
  virtual void      parse_options( option_t*, const std::string& options_str );
  virtual option_t* merge_options( std::vector<option_t>&, option_t*, option_t* );
  virtual rank_t*   init_rank( rank_t* rank_list, int id=0 );


  virtual double cost() SC_CONST;
  virtual double total_haste() SC_CONST  { return haste();           }
  virtual double haste() SC_CONST        { return 1.0;               }
  virtual double gcd() SC_CONST;
  virtual double execute_time() SC_CONST { return base_execute_time; }
  virtual double tick_time() SC_CONST;
  virtual int    hasted_num_ticks( double d=-1 ) SC_CONST;
  virtual double travel_time();
  virtual void   player_buff();
  virtual void   player_tick() {}
  virtual void   target_debuff( player_t* t, int dmg_type );
  virtual void   snapshot();
  virtual void   calculate_result() { assert( 0 ); }
  virtual bool   result_is_hit ( int r=RESULT_UNKNOWN ) SC_CONST;
  virtual bool   result_is_miss( int r=RESULT_UNKNOWN ) SC_CONST;
  virtual double calculate_direct_damage();
  virtual double calculate_tick_damage();
  virtual double calculate_weapon_damage();
  virtual double armor() SC_CONST;
  virtual double resistance() SC_CONST;
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   tick();
  virtual void   last_tick();
  virtual void   travel( player_t*, int result, double dmg );
  virtual void   assess_damage( player_t* t, double amount, int dmg_type, int travel_result );
  virtual void   additional_damage( player_t* t, double amount, int dmg_type, int travel_result );
  virtual void   schedule_execute();
  virtual void   schedule_tick();
  virtual void   schedule_travel( player_t* t );
  virtual void   reschedule_execute( double time );
  virtual void   refresh_duration();
  virtual void   extend_duration( int extra_ticks );
  virtual void   extend_duration_seconds( double extra_ticks );
  virtual void   update_ready();
  virtual bool   usable_moving();
  virtual bool   ready();
  virtual void   init();
  virtual void   reset();
  virtual void   cancel();
  virtual void   interrupt_action();
  virtual void   check_talent( int talent_rank );
  virtual void   check_spec( int necessary_spec );
  virtual void   check_min_level( int level );
  virtual const char* name() SC_CONST { return name_str.c_str(); }

  virtual double   miss_chance( int delta_level ) SC_CONST { delta_level=0; return 0; }
  virtual double  dodge_chance( int delta_level ) SC_CONST { return 0.0; }
  virtual double  parry_chance( int delta_level ) SC_CONST { return 0.0; }
  virtual double glance_chance( int delta_level ) SC_CONST { delta_level=0; return 0; }
  virtual double  block_chance( int delta_level ) SC_CONST { delta_level=0; return 0; }
  virtual double   crit_chance( int delta_level ) SC_CONST { delta_level=0; return 0; }

  virtual double total_multiplier() SC_CONST { return   base_multiplier * player_multiplier * target_multiplier; }
  virtual double total_hit() SC_CONST        { return   base_hit        + player_hit        + target_hit;        }
  virtual double total_crit() SC_CONST       { return   base_crit       + player_crit       + target_crit;       }
  virtual double total_crit_bonus() SC_CONST;

  virtual double total_spell_power() SC_CONST  { return floor( ( base_spell_power  + player_spell_power  + target_spell_power  ) * base_spell_power_multiplier  * player_spell_power_multiplier  ); }
  virtual double total_attack_power() SC_CONST { return floor( ( base_attack_power + player_attack_power + target_attack_power ) * base_attack_power_multiplier * player_attack_power_multiplier ); }
  virtual double total_power() SC_CONST;

  // Some actions require different multipliers for the "direct" and "tick" portions.

  virtual double total_dd_multiplier() SC_CONST { return total_multiplier() * base_dd_multiplier * player_dd_multiplier; }
  virtual double total_td_multiplier() SC_CONST { return total_multiplier() * base_td_multiplier * player_td_multiplier; }

  virtual action_expr_t* create_expression( const std::string& name );

  virtual double ppm_proc_chance( double PPM ) SC_CONST;

  void add_child( action_t* child ) { stats -> add_child( child -> stats ); }

  // Move to ability_t in future
  const spell_data_t* spell;
  const spelleffect_data_t& effect1() const { return spell -> effect1(); }
  const spelleffect_data_t& effect2() const { return spell -> effect2(); }
  const spelleffect_data_t& effect3() const { return spell -> effect3(); }
};

struct attack_t : public action_t
{
  double base_expertise, player_expertise, target_expertise;

  attack_t( const active_spell_t& s, int t=TREE_NONE, bool special=false );
  attack_t( const char* n=0, player_t* p=0, int r=RESOURCE_NONE, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=false );
  attack_t( const char* name, const char* sname, player_t* p, int t = TREE_NONE, bool special=false );
  attack_t( const char* name, const uint32_t id, player_t* p, int t = TREE_NONE, bool special=false );
  virtual void _init_attack_t();

  // Attack Overrides
  virtual double haste() SC_CONST;
  virtual double total_haste() SC_CONST  { return swing_haste();           }
  virtual double swing_haste() SC_CONST;
  virtual double execute_time() SC_CONST;
  virtual void   player_buff();
  virtual void   target_debuff( player_t* t, int dmg_type );
  virtual int    build_table( double* chances, int* results );
  virtual void   calculate_result();
  virtual void   execute();

  virtual double total_expertise() SC_CONST;

  virtual double   miss_chance( int delta_level ) SC_CONST;
  virtual double  dodge_chance( int delta_level ) SC_CONST;
  virtual double  parry_chance( int delta_level ) SC_CONST;
  virtual double glance_chance( int delta_level ) SC_CONST;
  virtual double  block_chance( int delta_level ) SC_CONST;
  virtual double  crit_block_chance( int delta_level ) SC_CONST;
  virtual double   crit_chance( int delta_level ) SC_CONST;
};

// Spell =====================================================================

struct spell_t : public action_t
{
  spell_t( const active_spell_t& s, int t=TREE_NONE );
  spell_t( const char* n=0, player_t* p=0, int r=RESOURCE_NONE, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE );
  spell_t( const char* name, const char* sname, player_t* p, int t = TREE_NONE );
  spell_t( const char* name, const uint32_t id, player_t* p, int t = TREE_NONE );
  virtual void _init_spell_t();

  // Spell Overrides
  virtual double haste() SC_CONST;
  virtual double gcd() SC_CONST;
  virtual double execute_time() SC_CONST;
  virtual void   player_buff();
  virtual void   target_debuff( player_t* t, int dmg_type );
  virtual void   calculate_result();
  virtual void   execute();

  virtual double miss_chance( int delta_level ) SC_CONST;
  virtual double crit_chance( int delta_level ) SC_CONST;

  virtual void   schedule_execute();
};

// Heal ======================================================================

struct heal_t : public spell_t
{
  std::vector<player_t*> heal_target;

  spell_t* valanyr;

  // Reporting
  double total_heal, total_actual;

  void _init_heal_t();
  heal_t(const char* n, player_t* player, const char* sname, int t = TREE_NONE);
  heal_t(const char* n, player_t* player, const uint32_t id, int t = TREE_NONE);

  virtual void parse_options( option_t* options, const std::string& options_str );
  virtual void player_buff();
  virtual void target_debuff( player_t* t, int dmg_type );
  virtual double haste() SC_CONST;
  virtual void execute();
  virtual void assess_damage( player_t* t, double amount,
                                  int    dmg_type, int travel_result );
  virtual void calculate_result();
  virtual double calculate_direct_damage();
  virtual double calculate_tick_damage();
  virtual void travel( player_t*, int travel_result, double travel_dmg );
  virtual void tick();
  virtual void last_tick();
  virtual player_t* find_greatest_difference_player();
  virtual player_t* find_lowest_player();
  virtual void refresh_duration();

};

// Absorb ======================================================================

struct absorb_t : public spell_t
{
  std::vector<player_t*> heal_target;

  // Reporting
  double total_heal, total_actual;

  void _init_absorb_t();
  absorb_t(const char* n, player_t* player, const char* sname, int t = TREE_NONE);
  absorb_t(const char* n, player_t* player, const uint32_t id, int t = TREE_NONE);

  virtual void parse_options( option_t* options, const std::string& options_str );
  virtual void player_buff();
  virtual void target_debuff( player_t* t, int dmg_type );
  virtual double haste() SC_CONST;
  virtual void execute();
  virtual void assess_damage( player_t* t, double amount,
                                    int    dmg_type, int travel_result );
  virtual void calculate_result();
  virtual double calculate_direct_damage();
  virtual void travel( player_t*, int travel_result, double travel_dmg );

};

// Sequence ==================================================================

struct sequence_t : public action_t
{
  std::vector<action_t*> sub_actions;
  int current_action;

  sequence_t( player_t*, const std::string& sub_action_str );
  virtual ~sequence_t();
  virtual void schedule_execute();
  virtual void reset();
  virtual bool ready();
  virtual void restart() { current_action=0; }
};

// Cooldown ==================================================================

struct cooldown_t
{
  sim_t* sim;
  player_t* player;
  std::string name_str;
  double duration;
  double ready;
  cooldown_t* next;
  cooldown_t( const std::string& n, player_t* p ) : sim(p->sim), player(p), name_str(n), duration(0), ready(-1), next(0) {}
  cooldown_t( const std::string& n, sim_t* s ) : sim(s), player(0), name_str(n), duration(0), ready(-1), next(0) {}
  void reset() { ready=-1; }
  void start( double override=-1, double delay=0 )
  {
    if ( override >= 0 ) duration = override;
    if ( duration > 0 ) ready = sim -> current_time + duration + delay;
  }
  double remains()
  {
    double diff = ready - sim -> current_time;
    if ( diff < 0 ) diff = 0;
    return diff;
  }
  const char* name() { return name_str.c_str(); }
};

// DoT =======================================================================

struct dot_t
{
  player_t* player;
  action_t* action;
  std::string name_str;
  event_t* tick_event;
  int num_ticks, current_tick, added_ticks, ticking;
  double added_seconds;
  double ready;
  double miss_time;
  dot_t* next;
  dot_t() : player(0) {}
  dot_t( const std::string& n, player_t* p ) :
    player(p), action(0), name_str(n), tick_event(0),
    num_ticks(0), current_tick(0), added_ticks(0), ticking(0),
    added_seconds( 0.0 ), ready(-1), miss_time(-1), next(0) {}
  virtual ~dot_t() {}
  virtual void reset() { tick_event=0; current_tick=0; added_ticks=0; ticking=0; added_seconds=0.0; ready=-1; miss_time=-1; }
  virtual void recalculate_ready()
  {
    // Extending a DoT does not interfere with the next tick event.  To determine the
    // new finish time for the DoT, start from the time of the next tick and add the time
    // for the remaining ticks to that event.
    int remaining_ticks = num_ticks - current_tick;
    ready = 0.001 + tick_event -> time + action -> tick_time() * ( remaining_ticks - 1 );
  }
  virtual double remains()
  {
    if ( ! action ) return 0;
    if ( ! ticking ) return 0;
    return ready - player -> sim -> current_time;
  }
  virtual int ticks()
  {
    if ( ! action ) return 0;
    if ( ! ticking ) return 0;
    return ( num_ticks - current_tick );
  }
  virtual const char* name() { return name_str.c_str(); }
};

struct action_callback_t
{
  sim_t* sim;
  player_t* listener;
  bool active;
  bool allow_self_procs;
  bool allow_item_procs;
  bool allow_procs;

  action_callback_t( sim_t* s, player_t* l, bool ap=false, bool aip=false, bool asp=false ) : sim( s ), listener( l ), active( true ), allow_self_procs( asp ), allow_item_procs( aip ), allow_procs( ap )
  {
    if ( l )
    {
      l -> all_callbacks.push_back( this );
    }
  }
  virtual ~action_callback_t() {}
  virtual void trigger( action_t*, void* call_data=0 ) = 0;
  virtual void reset() {}
  virtual void activate() { active=true; }
  virtual void deactivate() { active=false; }
  static void trigger( std::vector<action_callback_t*>& v, action_t* a, void* call_data=0 )
  {
    if ( ! a -> player -> in_combat ) return;

    size_t size = v.size();
    for ( size_t i=0; i < size; i++ )
    {
      action_callback_t* cb = v[ i ];
      if ( cb -> active )
      {
        if ( ! cb -> allow_item_procs && a -> item_proc ) return;
        if ( ! cb -> allow_procs && a -> proc ) return;
        cb -> trigger( a, call_data );
      }
    }
  }
  static void reset( std::vector<action_callback_t*>& v )
  {
    size_t size = v.size();
    for ( size_t i=0; i < size; i++ )
    {
      v[ i ] -> reset();
    }
  }
};

// Player Ready Event ========================================================

struct player_ready_event_t : public event_t
{
  player_ready_event_t( sim_t* sim, player_t* p, double delta_time );
  virtual void execute();
};

// Action Execute Event ======================================================

struct action_execute_event_t : public event_t
{
  action_t* action;
  action_execute_event_t( sim_t* sim, action_t* a, double time_to_execute );
  virtual void execute();
};

// DoT Tick Event ============================================================

struct dot_tick_event_t : public event_t
{
  dot_t* dot;
  dot_tick_event_t( sim_t* sim, dot_t* d, double time_to_tick );
  virtual void execute();
};

// Action Travel Event =======================================================

struct action_travel_event_t : public event_t
{
  action_t* action;
  player_t* target;
  int result;
  double damage;
  action_travel_event_t( sim_t* sim, player_t* t, action_t* a, double time_to_travel );
  virtual void execute();
};

// Regen Event ===============================================================

struct regen_event_t : public event_t
{
  regen_event_t( sim_t* sim );
  virtual void execute();
};

// Unique Gear ===============================================================

struct unique_gear_t
{
  static void init( player_t* );

  static action_callback_t* register_stat_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                int stat, int max_stacks, double amount,
                                                double proc_chance, double duration, double cooldown,
                                                double tick=0, bool reverse=false, int rng_type=RNG_DEFAULT );

  static action_callback_t* register_cost_reduction_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                          int school, int max_stacks, double amount,
                                                          double proc_chance, double duration, double cooldown,
                                                          bool refreshes=false, bool reverse=false, int rng_type=RNG_DEFAULT );

  static action_callback_t* register_discharge_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                     int max_stacks, const school_type school, double amount, double scaling,
                                                     double proc_chance, double cooldown, bool no_crits, bool no_buffs, bool no_debuffs,
                                                     int rng_type=RNG_DEFAULT );

  static action_callback_t* register_chance_discharge_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                            int max_stacks, const school_type school, double amount, double scaling,
                                                            double proc_chance, double cooldown, bool no_crits, bool no_buffs, bool no_debuffs,
                                                            int rng_type=RNG_DEFAULT );

  static action_callback_t* register_stat_discharge_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                          int stat, int max_stacks, double stat_amount,
                                                          const school_type school, double discharge_amount, double discharge_scaling,
                                                          double proc_chance, double duration, double cooldown, bool no_crits, bool no_buffs,
                                                          bool no_debuffs );

  static action_callback_t* register_stat_proc( item_t&, item_t::special_effect_t& );
  static action_callback_t* register_cost_reduction_proc( item_t&, item_t::special_effect_t& );
  static action_callback_t* register_discharge_proc( item_t&, item_t::special_effect_t& );
  static action_callback_t* register_chance_discharge_proc( item_t&, item_t::special_effect_t& );
  static action_callback_t* register_stat_discharge_proc( item_t&, item_t::special_effect_t& );

  static bool get_equip_encoding( std::string& encoding,
                                  const std::string& item_name,
                                  const bool         item_heroic,
                                  const bool         ptr,
                                  const std::string& item_id=std::string() );

  static bool get_use_encoding  ( std::string& encoding,
                                  const std::string& item_name,
                                  const bool         item_heroic,
                                  const bool         ptr,
                                  const std::string& item_id=std::string() );
};

// Enchants ===================================================================

struct enchant_t
{
  static void init( player_t* );
  static bool get_encoding        ( std::string& name, std::string& encoding, const std::string& enchant_id, const bool ptr );
  static bool get_addon_encoding  ( std::string& name, std::string& encoding, const std::string& addon_id, const bool ptr );
  static bool get_reforge_encoding( std::string& name, std::string& encoding, const std::string& reforge_id );
  static int  get_reforge_id      ( stat_type stat_from, stat_type stat_to );
  static bool download        ( item_t&, const std::string& enchant_id );
  static bool download_addon  ( item_t&, const std::string& addon_id   );
  static bool download_reforge( item_t&, const std::string& reforge_id );
  static bool download_rsuffix( item_t&, const std::string& rsuffix_id );
};

// Consumable ================================================================

struct consumable_t
{
  static void init_flask  ( player_t* );
  static void init_elixirs( player_t* );
  static void init_food   ( player_t* );

  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
};

// Up-Time =====================================================================

struct uptime_t
{
  std::string name_str;
  uint64_t up, down;
  uptime_t* next;
  uptime_t( const std::string& n ) : name_str( n ), up( 0 ), down( 0 ) {}
  virtual ~uptime_t() {}
  void   update( bool is_up ) { if ( is_up ) up++; else down++; }
  void   update( int  is_up ) { update( is_up ? true : false ); }
  double percentage() SC_CONST { return ( up==0 ) ? 0 : ( 100.0*up/( up+down ) ); }
  virtual void   merge( uptime_t* other ) { up += other -> up; down += other -> down; }
  const char* name() SC_CONST { return name_str.c_str(); }
};

// Gain ======================================================================

struct gain_t
{
  std::string name_str;
  double actual, overflow, count;
  resource_type type;
  int id;
  gain_t* next;
  gain_t( const std::string& n, int _id=0 ) :
    name_str( n ), actual( 0 ), overflow( 0 ), count( 0 ), type( RESOURCE_NONE ), id( _id ) {}
  void add( double a, double o=0 ) { actual += a; overflow += o; count++; }
  void merge( gain_t* other ) { actual += other -> actual; overflow += other -> overflow; count += other -> count; }
  void analyze( sim_t* sim ) { actual /= sim -> iterations; overflow /= sim -> iterations; count /= sim -> iterations; }
  const char* name() SC_CONST { return name_str.c_str(); }
};

// Proc ======================================================================

struct proc_t
{
  sim_t* sim;
  player_t* player;
  std::string name_str;
  double count;
  double frequency;
  double interval_sum;
  double interval_count;
  double last_proc;
  proc_t* next;
  proc_t( sim_t* s, const std::string& n ) :
    sim(s), player( 0 ), name_str(n), count(0), frequency(0), interval_sum(0), interval_count(0), last_proc(0), next( 0 ) {}
  void occur()
  {
    count++;
    if ( last_proc > 0 && last_proc < sim -> current_time )
    {
      interval_sum += sim -> current_time - last_proc;
      interval_count++;
    }
    last_proc = sim -> current_time;
  }
  void merge( proc_t* other )
  {
    count          += other -> count;
    interval_sum   += other -> interval_sum;
    interval_count += other -> interval_count;
  }
  void analyze( sim_t* sim )
  {
    count /= sim -> iterations;
    if ( interval_count > 0 ) frequency = interval_sum / interval_count;
  }
  const char* name() SC_CONST { return name_str.c_str(); }
};


// Report =====================================================================

struct report_t
{
  static void encode_html( std::string& buffer );
  static void print_spell_query( sim_t* );
  static void print_profiles( sim_t* );
  static void print_text( FILE*, sim_t*, bool detail=true );
  static void print_html( sim_t* );
  static void print_xml( sim_t* );
  static void print_suite( sim_t* );
};

// Chart ======================================================================

struct chart_t
{
  static int raid_dps ( std::vector<std::string>& images, sim_t* );
  static int raid_dpet( std::vector<std::string>& images, sim_t* );
  static int raid_gear( std::vector<std::string>& images, sim_t* );

  static const char* raid_downtime    ( std::string& s, sim_t* );
  static const char* raid_timeline    ( std::string& s, sim_t* );
  static const char* action_dpet      ( std::string& s, player_t* );
  static const char* action_dmg       ( std::string& s, player_t* );
  static const char* gains            ( std::string& s, player_t*, resource_type );
  static const char* timeline_resource( std::string& s, player_t* );
  static const char* timeline_health  ( std::string& s, player_t* );
  static const char* timeline_dps     ( std::string& s, player_t* );
  static const char* timeline_dps_error( std::string& s, player_t* );
  static const char* scale_factors    ( std::string& s, player_t* );
  static const char* scaling_dps      ( std::string& s, player_t* );
  static const char* reforge_dps      ( std::string& s, player_t* );
  static const char* distribution_dps ( std::string& s, player_t* );

  static const char* gear_weights_lootrank  ( std::string& s, player_t* );
  static const char* gear_weights_wowhead   ( std::string& s, player_t* );
  static const char* gear_weights_wowreforge( std::string& s, player_t* );
  static const char* gear_weights_pawn      ( std::string& s, player_t*, bool hit_expertise=true );

  static const char* dps_error( std::string& s, player_t* );
};

// Log =======================================================================

struct log_t
{
  // Generic Output
  static void output( sim_t*, const char* format, ... );

  // Combat Log (unsupported)
};

// Pseudo Random Number Generation ===========================================

struct rng_t
{
  std::string name_str;
  bool   gauss_pair_use;
  double gauss_pair_value;
  double expected_roll,  actual_roll,  num_roll;
  double expected_range, actual_range, num_range;
  double expected_gauss, actual_gauss, num_gauss;
  bool   average_range, average_gauss;
  rng_t* next;

  rng_t( const std::string& n, bool avg_range=false, bool avg_gauss=false );
  virtual ~rng_t() {}

  virtual int    type() SC_CONST { return RNG_STANDARD; }
  virtual double real();
  virtual int    roll( double chance );
  virtual double range( double min, double max );
  virtual double gauss( double mean, double stddev );
  virtual double exgauss( double mean, double stddev, double nu );
  virtual void   seed( uint32_t start );
  virtual void   report( FILE* );

  static rng_t* create( sim_t*, const std::string& name, int type=RNG_STANDARD );
};

// String utils =================================================================

std::string tolower( std::string src );
std::string trim( std::string src );
void replace_char( std::string& src, char old_c, char new_c  );
void replace_str( std::string& src, std::string old_str, std::string new_str  );
bool str_to_float( std::string src, double& dest );
std::string proper_option_name( const std::string& full_name );

// Thread Wrappers ===========================================================

struct thread_t
{
  static void init();
  static void de_init();
  static void launch( sim_t* );
  static void wait( sim_t* );
  static void mutex_init( void*& mutex );
  static void mutex_lock( void*& mutex );
  static void mutex_unlock( void*& mutex );
};

// Armory ====================================================================

struct armory_t
{
  static int download_servers( std::vector<std::string>& servers,
                               const std::string& region );
  static int download_guild( std::vector<std::string>& characters,
                             const std::string& region,
                             const std::string& server,
                             const std::string& name );
  static bool download_guild( sim_t* sim,
                              const std::string& region,
                              const std::string& server,
                              const std::string& name,
                              const std::vector<int>& ranks,
                              int player_type = PLAYER_NONE,
                              int max_rank=0,
                              int cache=0 );
  static player_t* download_player( sim_t* sim,
                                    const std::string& region,
                                    const std::string& server,
                                    const std::string& name,
                                    const std::string& talents,
                                    int cache=0 );
  static bool download_slot( item_t&, const std::string& item_id, int cache_only=0 );
  static bool download_item( item_t&, const std::string& item_id, int cache_only=0 );
  static void fuzzy_stats( std::string& encoding, const std::string& description );
  static int  parse_meta_gem( const std::string& description );
  static std::string& format( std::string& name, int format_type = FORMAT_DEFAULT );
};

// Battle Net ================================================================

struct battle_net_t
{
  static player_t* download_player( sim_t* sim,
                                    const std::string& region,
                                    const std::string& server,
                                    const std::string& name,
                                    const std::string& talents,
                                    int cache=0 );
  static bool download_guild( sim_t* sim,
                              const std::string& region,
                              const std::string& server,
                              const std::string& name,
                              const std::vector<int>& ranks,
                              int player_type = PLAYER_NONE,
                              int max_rank=0,
                              int cache=0 );
};

// Wowhead  ==================================================================

struct wowhead_t
{
  static player_t* download_player( sim_t* sim,
                                    const std::string& region,
                                    const std::string& server,
                                    const std::string& name,
                                    int active=1 );
  static player_t* download_player( sim_t* sim, const std::string& id, int active=1 );
  static bool download_slot( item_t&,
                             const std::string& item_id,
                             const std::string& enchant_id,
                             const std::string& addon_id,
                             const std::string& reforge_id,
                             const std::string& rsuffix_id,
                             const std::string gem_ids[ 3 ],
                             int cache_only=0,
                             bool ptr=false );
  static bool download_item( item_t&, const std::string& item_id, int cache_only=0,bool ptr=false );
  static bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id, int cache_only=0, bool ptr=false );
  static int  parse_gem( item_t& item, const std::string& gem_id, int cache_only=0, bool ptr=false );
};

// CharDev  ==================================================================

struct chardev_t
{
  static player_t* download_player( sim_t* sim, const std::string& id );
};

// MMO Champion ==============================================================

struct mmo_champion_t
{
  static bool download_slot( item_t&,
                             const std::string& item_id,
                             const std::string& enchant_id,
                             const std::string& addon_id,
                             const std::string& reforge_id,
                             const std::string& rsuffix_id,
                             const std::string gem_ids[ 3 ],
                             int cache_only=0 );
  static bool download_item( item_t&, const std::string& item_id, int cache_only=0 );
  static bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id, int cache_only=0 );
  static int  parse_gem( item_t& item, const std::string& gem_id, int cache_only=0 );
};

// Rawr ======================================================================

struct rawr_t
{
  static player_t* load_player( sim_t*, const std::string& character_filename );
  static player_t* load_player( sim_t*, const std::string& character_filename, const std::string& character_xml );
};

// HTTP Download  ============================================================

struct http_t
{
  static std::string proxy_type;
  static std::string proxy_host;
  static int proxy_port;
  static bool cache_load();
  static bool cache_save();
  static void cache_clear();
  static void cache_set( const std::string& url, const std::string& result, int64_t timestamp=0 );
  static bool cache_get( std::string& result, const std::string& url, int64_t timestamp=0 );
  static bool download( std::string& result, const std::string& url );
  static bool get( std::string& result, const std::string& url, const std::string& confirmation=std::string(), int64_t timestamp=0, int throttle_seconds=0 );
  static bool clear_cache( sim_t*, const std::string& name, const std::string& value );
  static std::string& format( std::string& encoded_url, const std::string& url );
};

// XML =======================================================================

struct xml_t
{
  static const char* get_name( xml_node_t* node );
  static xml_node_t* get_child( xml_node_t* root, const std::string& name );
  static xml_node_t* get_node ( xml_node_t* root, const std::string& path );
  static xml_node_t* get_node ( xml_node_t* root, const std::string& path, const std::string& parm_name, const std::string& parm_value );
  static int  get_children( std::vector<xml_node_t*>&, xml_node_t* root, const std::string& name = std::string() );
  static int  get_nodes   ( std::vector<xml_node_t*>&, xml_node_t* root, const std::string& path );
  static int  get_nodes   ( std::vector<xml_node_t*>&, xml_node_t* root, const std::string& path, const std::string& parm_name, const std::string& parm_value );
  static bool get_value( std::string& value, xml_node_t* root, const std::string& path = std::string() );
  static bool get_value( int&         value, xml_node_t* root, const std::string& path = std::string() );
  static bool get_value( double&      value, xml_node_t* root, const std::string& path = std::string() );
  static xml_node_t* download( sim_t* sim, const std::string& url, const std::string& confirmation=std::string(), int64_t timestamp=0, int throttle_seconds=0 );
  static xml_node_t* download_cache( sim_t* sim, const std::string& url, int64_t timestamp=0 );
  static xml_node_t* create( sim_t* sim, const std::string& input );
  static xml_node_t* create( sim_t* sim, FILE* input );
  static void print( xml_node_t* root, FILE* f=0, int spacing=0 );
};


// Java Script ===============================================================

struct js_t
{
  static js_node_t* get_child( js_node_t* root, const std::string& name );
  static js_node_t* get_node ( js_node_t* root, const std::string& path );
  static int  get_children( std::vector<js_node_t*>&, js_node_t* root, const std::string& name = std::string() );
  static int  get_nodes   ( std::vector<js_node_t*>&, js_node_t* root, const std::string& path );
  static int  get_value( std::vector<std::string>& value, js_node_t* root, const std::string& path = std::string() );
  static bool get_value( std::string& value, js_node_t* root, const std::string& path = std::string() );
  static bool get_value( int&         value, js_node_t* root, const std::string& path = std::string() );
  static bool get_value( double&      value, js_node_t* root, const std::string& path = std::string() );
  static js_node_t* create( sim_t* sim, const std::string& input );
  static js_node_t* create( sim_t* sim, FILE* input );
  static void print( js_node_t* root, FILE* f=0, int spacing=0 );
  static const char* get_name( js_node_t* root );
};

#ifdef WHAT_IF

#define AOE_CAP 10

struct ticker_t // replacement for dot_t, handles any ticking buff/debuff
{
  dot_t stuff
  double crit;
  double haste;
  double mastery;
};

struct sim_t
{
...
actor_t* actor_list;
std::vector<player_t*> player_list;
std::vector<mob_t*> mob_list;
...
int get_aura_slot( const std::string& n, actor_t* source );
...
};

struct actor_t
{
  items, stats, action list, resource management...
  actor_t( const std::string& n, int type ) {}
  virtual actor_t*  choose_target();
  virtual void      execute_action() { choose_target(); execute_first_ready_action(); }
  virtual debuff_t* get_debuff( int aura_slot );
  virtual ticker_t* get_ticker( int aura_slot );
  virtual bool      is_ally( actor_t* other );
};

struct player_t : public actor_t
{
  scaling, current_target (actor), pet_list ...
  player_t( const std::string& n, int type ) : actor_t( n, type ) { sim.player_list.push_back(this); }
};

struct pet_t : public actor_t
{
  owner, summon, dismiss...
  pet_t( const std::string& n, int type ) : actor_t( n, type ) {}
};

struct enemy_t : public actor_t
{
  health_by_time, health_per_player, arrise_at_time, arise_at_percent, ...
  enemy_t( const std::string& n ) : actor_t( n, ACTOR_ENEMY ) { sim.enemy_list.push_back(this); }
};

struct action_t
{
  actor_t, ...
  action_t( const std::string& n );
  virtual int execute();
  virtual void schedule_execute();
  virtual double execute_time();
  virtual double haste();
  virtual double gcd();
  virtual bool ready();
  virtual void cancel();
  virtual void reset();
};

struct result_t
{
  actor_t* target;
  int type;
  bool hit;  // convenience
  bool crit; // convenience, two-roll (blocked crits, etc)
  double amount;
  ticker_t ticker;
};

struct ability_t : public action_t
{
  spell_data, resource, weapon, two_roll, self_cast, aoe, base_xyz (no player_xyz or target_xyz),
  direct_sp_coeff, direct_ap_coeff, tick_sp_coeff, tick_ap_coeff,
  harmful, healing, callbacks, std::vector<result_t> results,
  NO binary, NO repeating, NO direct_dmg, NO tick_dmg
  ability_t( spell_data_t* s ) : action_t( s -> name() ) {}
  virtual void  execute()
  {
    actor_t* targets[ AOE_CAP ];
    int num_targets = area_of_effect( targets );
    results.resize( num_targets );
    for( int i=0; i < num_targets; i++ )
    {
      calculate_result( results[ i ], targets[ i ] );
      // "result" callbacks
    }
    consume_resource();
    for( int i=0; i < num_targets; i++ )
    {
      schedule_travel( results[ i ] );
    }
    update_ready();
    // "cast" callbacks
  }
  virtual void travel( result_t& result )
  {
    if( result.hit )
    {
      if ( result.amount > 0 )
      {
        if ( harmful )
        {
          assess_damage( result );
        }
        else if( healing )
        {
          assess_healing( result );
        }
      }
      if ( num_ticks > 0 )
      {
        ticker_t* ticker = get_ticker( result.target );  // caches aura_slot
        ticker -> trigger( this, result.ticker );
        // ticker_t::trigger() handles dot work in existing action_t::travel()
      }
    }
    else
    {
      // miss msg
      stat -> add_result( result );
    }
  }
  virtual void   tick         ( ticker_t* );
  virtual void   last_tick    ( ticker_t* );
  virtual void   schedule_tick( ticker_t* );
  virtual int    calculate_num_ticks( double haste, double duration=0 );
  virtual double cost();
  virtual double haste();
  virtual bool   ready();
  virtual void   cancel();
  virtual void   reset();
  virtual void   consume_resource();
  virtual result_type calculate_attack_roll( actor_t* target );
  virtual result_type calculate_spell_roll( actor_t* target );
  virtual result_type calculate_roll( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_roll( target );
    else
      return calculate_spell_roll( target );
  }
  virtual void calculate_result( result_t& result, actor_t* target )
  {
    result.type = calculate_roll( target );
    result.hit  = ( roll == ? );
    result.crit = ( roll == ? ) || ( two_roll );
    result.amount = calculate_direct_amount( target );
    if ( result.hit && num_ticks )
    {
      calculate_ticker( &( result.ticker ), target );
    }
    return result;
  }
  virtual void calculate_ticker( ticker_t* ticker, target )
  {
    if ( target ) ticker -> target = target;
    ticker -> amount  = calculate_tick_amount( ticker -> target );
    ticker -> crit    = calculate_crit_chance( ticker -> target );
    ticker -> haste   = calculate_haste      ( ticker -> target );
    ticker -> mastery = calculate_mastery    ( ticker -> target );
  }
  virtual void refresh_ticker( ticker_t* ticker )
  {
    calculate_ticker( ticker );
    ticker -> current_tick = 0;
    ticker -> added_ticks = 0;
    ticker -> added_time = 0;
    ticker -> num_ticks = calculate_num_ticks( ticker -> haste );
    ticker -> recalculate_finish();
  }
  virtual void extend_ticker_by_time( ticker_t* ticker, double extra_time )
  {
    int    full_tick_count   = ticker -> ticks() - 1;
    double full_tick_remains = ticker -> finish - ticker -> tick_event -> time;

    ticker -> haste = calculate_haste( ticker -> target );
    ticker -> num_ticks += calculate_num_ticks( ticker -> haste, full_tick_remains ) - full_tick_count;
    ticker -> recalculate_finish();
  }
  virtual void extend_ticker_by_ticks( ticker_t* ticker, double extra_ticks )
  {
    calculate_ticker( ticker );
    ticker -> added_ticks += extra_ticks;
    ticker -> num_ticks   += extra_ticks;
    ticker -> recalculate_finish();
  }
  virtual double calculate_weapon_amount( actor_t* target );
  virtual double calculate_direct_amount( actor_t* target );
  virtual double calculate_tick_amount  ( actor_t* target );
  virtual double calculate_power( actor_t* target )
  {
    return AP_multiplier * AP() + SP_multiplier * SP();
  }
  virtual double calculate_haste( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_haste( target );
    else
      return calculate_spell_haste( target );
  }
  virtual double calculate_mastery( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_mastery( target );
    else
      return calculate_spell_mastery( target );
  }
  virtual double calculate_miss_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_miss_chance( target );
    else
      return calculate_spell_miss_chance( target );
  }
  virtual double calculate_dodge_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_dodge_chance( target );
    else
      return 0;
  }
  virtual double calculate_parry_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_parry_chance( target );
    else
      return 0;
  }
  virtual double calculate_glance_chance( actor_t* target )
  {
    if ( weapon && auto_attack )
      return calculate_attack_glance_chance( target );
    else
      return 0;
  }
  virtual double calculate_block_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_block_chance( target );
    else
      return 0;
  }
  virtual double calculate_crit_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_crit_chance( target );
    else
      return calculate_spell_crit_chance( target );
  }
  virtual double calculate_crit_chance( ticker_t* ticker );
  virtual double calculate_source_multiplier();
  virtual double calculate_direct_multiplier() { return calculate_source_multiplier(); } // include crit bonus here
  virtual double calculate_tick_multiplier  () { return calculate_source_multiplier(); }
  virtual double calculate_target_multiplier( actor_t* target );
  virtual int       area_of_effect( actor_t* targets[] ) { targets[ 0 ] = self_cast ? actor : actor -> current_target; return 1; }
  virtual result_t& result(); // returns 0th "result", asserts if aoe
  virtual double    travel_time( actor_t* target );
  virtual void      schedule_travel( result_t& result );
  virtual void assess_damage( result_t& result )
  {
    result.amount *= calculate_target_multiplier( result.target ); // allows for action-specific target multipliers
    result.target -> assess_damage( result ); // adjust result as needed for flexibility
    stat -> add_result( result );
  }
  virtual void assess_healing( result_t& result )
  {
    result.amount *= calculate_target_multiplier( result.target ); // allows for action-specific target multipliers
    result.target -> assess_healing( result ); // adjust result as needed for flexibility
    stat -> add_result( result );
  }
};

#endif

#endif // __SIMULATIONCRAFT_H


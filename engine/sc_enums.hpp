// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

// Enumerations =============================================================
// annex _e to enumerations


// Type of internal execution of an action
enum class execute_type : unsigned
{
  FOREGROUND = 0u,
  OFF_GCD,
  CAST_WHILE_CASTING
};


// Source of the profile, defaults to command line / addon / etc
enum class profile_source
{
  DEFAULT, /// Anything non-blizzard-api
  BLIZZARD_API
};

// Attack power computation modes for Battle for Azeroth+
enum attack_power_e
{
  AP_NONE = -1,         /// Unset value
  AP_WEAPON_MH = 0,     /// Default mode, Attack power is a composite of power and mainhand weapon dps
  AP_WEAPON_OH,         /// Attack power is a composite of power and offhand weapon dps
  AP_WEAPON_BOTH,       /// Attack power is a composite of power and both weapon dps
  AP_NO_WEAPON,         /// Attack power is purely based on player power (main stat)
  AP_DEFAULT = AP_WEAPON_MH,
};


// Retargeting request event sources. Context in ACTOR_ is the actor that triggered the event
enum retarget_event_e
{
  ACTOR_ARISE = 0U,     // Any actor arises
  ACTOR_DEMISE,         // Any actor demises
  ACTOR_INVULNERABLE,   // Actor becomes invulnerable
  ACTOR_VULNERABLE,     // Actor becomes vulnerable (after becoming invulnerable)
  SELF_ARISE            // Actor has arisen (no context provided)
};

// Misc Constants
enum
{
  MAX_ARTIFACT_POWER = 29, /// Maximum number of artifact perks per weapon. Looks like max is 17 on weapons but setting higher just in case -- 2016/04/04 - Twintop. Increase to 25 to encompass new traits.

  MAX_ARTIFACT_RELIC = 4,
  RELIC_ILEVEL_BONUS_CURVE = 1718, /// Seemingly hard coded CurvePoint identifier for the data that returns the item level increase of a relic, based on the relic's own item level

  ITEM_TRINKET_BURST_CATEGORY = 1141, /// Trinket On-Use effect default category (for shared CD)
  MAX_GEM_SLOTS = 4, /// Global maximum number of gem slots in any specific item

  WEAPON_POWER_COEFFICIENT = 6, // WDPS -> Attack Power Coefficient used for BfA Attack Power calculations

  MAX_AZERITE_LEVEL = 300, // Maximum Azerite level (for Heart of Azeroth) at the start of Battle for Azeroth
};

// Azerite control
enum azerite_control
{
  AZERITE_ENABLED = 0,          /// All azerite-related effects enabled (default)
  AZERITE_DISABLED_ITEMS,       /// Azerite effects from items are disabled
  AZERITE_DISABLED_ALL          /// All azerite-related effects disabled
};

enum regen_type_e
{
  /**
   * @brief Old resource regeneration model.
   *
   * Actors regen every 'periodicity' seconds through a single global event.
   * Default.
   */
  REGEN_STATIC,

  /**
   * @brief Dynamic resource regeneration model.
   *
   * Resources are regenerated at dynamic intervals when an actor is about to
   * execute an action, and when the state of the actor changes in a way that
   * affects resource regeneration.
   *
   * See comment on player_t::regen_caches how to define what state changes
   * affect resource regneration.
   */
  REGEN_DYNAMIC,

  /// Resource regeneration is disabled for the actor
  REGEN_DISABLED
};

enum class buff_tick_behavior
{
  NONE,

  // tick timer is reset on refresh of the dot/buff (next tick is rescheduled)
  CLIP,

  // tick timer stays intact on refresh of the dot/buff (next tick won't be rescheduled)
  REFRESH
};

enum class buff_tick_time_behavior
{
  UNHASTED,
  HASTED,
  CUSTOM
};

/**
 * @brief Buff refresh mechanism during trigger.
 *
 * Defaults to _PANDEMIC for ticking buffs,
 * _DURATION for normal buffs.
 */
enum class buff_refresh_behavior
{
  /// Constructor default, determines "autodetection" in buff_t::buff_t
  NONE = -1,

  /// Disable refresh by triggering
  DISABLED,

  /// Refresh to given duration
  DURATION,

  /// Refresh to given duration plus remaining duration
  EXTEND,

  /// Refresh to given duration plus min( 0.3 * new_duration, remaining_duration
  /// )
  PANDEMIC,

  /// Refresh to given duration plus ongoing tick time
  TICK,

  /// Refresh to duration returned by the custom callback
  CUSTOM,
};

enum class buff_stack_behavior
{
  DEFAULT,

  /**
   * Asynchronous buffs use separate duration for each stack application.
   * -> Each stack expires on its own.
   */
  ASYNCHRONOUS,
};

enum movement_direction_e
{
  MOVEMENT_UNKNOWN = -1,
  MOVEMENT_NONE,
  MOVEMENT_OMNI,
  MOVEMENT_TOWARDS,
  MOVEMENT_AWAY,
  MOVEMENT_RANDOM,  // Reserved for raid event
  MOVEMENT_DIRECTION_MAX,
  MOVEMENT_RANDOM_MIN = MOVEMENT_OMNI,
  MOVEMENT_RANDOM_MAX = MOVEMENT_RANDOM
};

enum talent_format_e
{
  TALENT_FORMAT_NUMBERS = 0,
  TALENT_FORMAT_ARMORY,
  TALENT_FORMAT_WOWHEAD,
  TALENT_FORMAT_UNCHANGED,
  TALENT_FORMAT_MAX
};

enum race_e
{
  RACE_NONE = 0,
  // Target Races
  RACE_BEAST,
  RACE_DRAGONKIN,
  RACE_GIANT,
  RACE_HUMANOID,
  RACE_DEMON,
  RACE_ELEMENTAL,
  // Player Races
  RACE_NIGHT_ELF,
  RACE_HUMAN,
  RACE_GNOME,
  RACE_DWARF,
  RACE_DRAENEI,
  RACE_WORGEN,
  RACE_ORC,
  RACE_TROLL,
  RACE_UNDEAD,
  RACE_BLOOD_ELF,
  RACE_TAUREN,
  RACE_GOBLIN,
  RACE_PANDAREN,
  RACE_PANDAREN_ALLIANCE,
  RACE_PANDAREN_HORDE,
  RACE_VOID_ELF,
  RACE_LIGHTFORGED_DRAENEI,
  RACE_HIGHMOUNTAIN_TAUREN,
  RACE_NIGHTBORNE,
  RACE_DARK_IRON_DWARF,
  RACE_MAGHAR_ORC,
  RACE_ZANDALARI_TROLL,
  RACE_KUL_TIRAN,

  RACE_UNKNOWN,
  RACE_MAX
};

inline bool is_pandaren( race_e r )
{
  return RACE_PANDAREN <= r && r <= RACE_PANDAREN_HORDE;
}

enum player_e
{
  PLAYER_SPECIAL_SCALE8 = -8,
  PLAYER_SPECIAL_SCALE7 = -7,
  PLAYER_SPECIAL_SCALE6 = -6,
  PLAYER_SPECIAL_SCALE5 = -5,
  PLAYER_SPECIAL_SCALE4 = -4,
  PLAYER_SPECIAL_SCALE3 = -3,
  PLAYER_SPECIAL_SCALE2 = -2,
  PLAYER_SPECIAL_SCALE  = -1,
  PLAYER_NONE           = 0,
  DEATH_KNIGHT,
  DEMON_HUNTER,
  DRUID,
  HUNTER,
  MAGE,
  MONK,
  PALADIN,
  PRIEST,
  ROGUE,
  SHAMAN,
  WARLOCK,
  WARRIOR,
  PLAYER_PET,
  PLAYER_GUARDIAN,
  HEALING_ENEMY,
  ENEMY,
  ENEMY_ADD,
  TMI_BOSS,
  TANK_DUMMY,
  PLAYER_MAX
};

enum pet_e
{
  PET_NONE = 0,

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

  PET_FEROCITY_TYPE,

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
  //PET_RIVERBEAST,

  PET_TENACITY_TYPE,

  // Cunning
  PET_BAT,
  PET_BIRD_OF_PREY,
  PET_CHIMAERA,
  PET_DRAGONHAWK,
  PET_MONKEY,
  PET_NETHER_RAY,
  PET_RAVAGER,
  PET_SERPENT,
  PET_SILITHID,
  PET_SPIDER,
  PET_SPOREBAT,
  PET_WIND_SERPENT,

  PET_CUNNING_TYPE,

  PET_HUNTER,

  PET_FELGUARD,
  PET_FELHUNTER,
  PET_IMP,
  PET_VOIDWALKER,
  PET_SUCCUBUS,
  PET_INFERNAL,
  PET_DOOMGUARD,
  PET_WILD_IMP,
  PET_DREADSTALKER,
  PET_VILEFIEND,
  PET_DEMONIC_TYRANT,
  PET_SERVICE_IMP,
  PET_SERVICE_FELHUNTER,
  PET_SERVICE_FELGUARD,
  PET_WARLOCK_RANDOM,
  PET_DARKGLARE,
  PET_OBSERVER,
  PET_THAL_KIEL,
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
  PET_MINDBENDER,
  PET_VOID_TENDRIL,
  PET_LIGHTWELL,
  PET_PRIEST,

  PET_SPIRIT_WOLF,
  PET_FIRE_ELEMENTAL,
  PET_EARTH_ELEMENTAL,
  PET_SHAMAN,

  PET_ENEMY,

  PET_MAX
};

enum dmg_e
{
  RESULT_TYPE_NONE = -1,
  DMG_DIRECT       = 0,
  DMG_OVER_TIME    = 1,
  HEAL_DIRECT,
  HEAL_OVER_TIME,
  ABSORB
};

enum stats_e
{
  STATS_DMG,
  STATS_HEAL,
  STATS_ABSORB,
  STATS_NEUTRAL
};

enum dot_behavior_e
{
  DOT_CLIP,
  DOT_REFRESH,
  DOT_EXTEND
};

enum dot_copy_e
{
  DOT_COPY_START,
  DOT_COPY_CLONE
};

enum attribute_e
{
  ATTRIBUTE_NONE = 0,
  ATTR_STRENGTH,
  ATTR_AGILITY,
  ATTR_STAMINA,
  ATTR_INTELLECT,
  ATTR_SPIRIT,
  // WoD Hybrid attributes
  ATTR_AGI_INT,
  ATTR_STR_AGI,
  ATTR_STR_INT,
  ATTR_STR_AGI_INT,
  ATTRIBUTE_MAX,

  /**
   * "All stats" enchant attribute cap to prevent
   * double/triple/quadrupledipping
   */
  ATTRIBUTE_STAT_ALL_MAX = ATTR_AGI_INT
};

enum resource_e
{
  RESOURCE_NONE = 0,
  RESOURCE_HEALTH,
  RESOURCE_MANA,
  RESOURCE_RAGE,
  RESOURCE_FOCUS,
  RESOURCE_ENERGY,
  RESOURCE_RUNIC_POWER,
  RESOURCE_SOUL_SHARD,
  RESOURCE_ASTRAL_POWER,
  RESOURCE_HOLY_POWER,
  /* Unknown_2, */
  RESOURCE_MAELSTROM,
  RESOURCE_CHI,
  RESOURCE_INSANITY,
  RESOURCE_FURY,
  RESOURCE_PAIN,
  RESOURCE_RUNE,
  RESOURCE_COMBO_POINT,
  RESOURCE_MAX
};

enum result_e
{
  RESULT_UNKNOWN = -1,
  RESULT_NONE    = 0,
  RESULT_MISS,
  RESULT_DODGE,
  RESULT_PARRY,
  RESULT_GLANCE,
  RESULT_CRIT,
  RESULT_HIT,
  RESULT_MAX
};

enum block_result_e
{
  BLOCK_RESULT_UNKNOWN   = -1,
  BLOCK_RESULT_UNBLOCKED = 0,
  BLOCK_RESULT_BLOCKED,
  BLOCK_RESULT_CRIT_BLOCKED,
  BLOCK_RESULT_MAX
};

enum full_result_e
{
  FULLTYPE_UNKNOWN = -1,
  FULLTYPE_NONE    = 0,
  FULLTYPE_MISS,
  FULLTYPE_DODGE,
  FULLTYPE_PARRY,
  FULLTYPE_GLANCE_CRITBLOCK,
  FULLTYPE_GLANCE_BLOCK,
  FULLTYPE_GLANCE,
  FULLTYPE_CRIT_CRITBLOCK,
  FULLTYPE_CRIT_BLOCK,
  FULLTYPE_CRIT,
  FULLTYPE_HIT_CRITBLOCK,
  FULLTYPE_HIT_BLOCK,
  FULLTYPE_HIT,
  FULLTYPE_MAX
};

const unsigned RESULT_HIT_MASK =
    ( 1 << RESULT_GLANCE ) | ( 1 << RESULT_CRIT ) | ( 1 << RESULT_HIT );

const unsigned RESULT_CRIT_MASK = ( 1 << RESULT_CRIT );

const unsigned RESULT_MISS_MASK = ( 1 << RESULT_MISS );

const unsigned RESULT_DODGE_MASK = ( 1 << RESULT_DODGE );

const unsigned RESULT_PARRY_MASK = ( 1 << RESULT_PARRY );

const unsigned RESULT_NONE_MASK = ( 1 << RESULT_NONE );

const unsigned RESULT_ALL_MASK = -1;

enum special_effect_e
{
  SPECIAL_EFFECT_NONE = -1,
  SPECIAL_EFFECT_EQUIP,
  SPECIAL_EFFECT_USE,
  SPECIAL_EFFECT_FALLBACK // Internal use only for fallback special effects
};

enum special_effect_source_e
{
  SPECIAL_EFFECT_SOURCE_NONE = -1,
  SPECIAL_EFFECT_SOURCE_ITEM,
  SPECIAL_EFFECT_SOURCE_ENCHANT,
  SPECIAL_EFFECT_SOURCE_ADDON,
  SPECIAL_EFFECT_SOURCE_GEM,
  SPECIAL_EFFECT_SOURCE_SOCKET_BONUS,
  SPECIAL_EFFECT_SOURCE_RACE,
  SPECIAL_EFFECT_SOURCE_AZERITE,
  SPECIAL_EFFECT_SOURCE_FALLBACK
};

enum special_effect_buff_e
{
  SPECIAL_EFFECT_BUFF_NONE = -1,
  SPECIAL_EFFECT_BUFF_CUSTOM,
  SPECIAL_EFFECT_BUFF_STAT,
  SPECIAL_EFFECT_BUFF_ABSORB,
  SPECIAL_EFFECT_BUFF_DISABLED
};

enum special_effect_action_e
{
  SPECIAL_EFFECT_ACTION_NONE = -1,
  SPECIAL_EFFECT_ACTION_CUSTOM,
  SPECIAL_EFFECT_ACTION_SPELL,
  SPECIAL_EFFECT_ACTION_HEAL,
  SPECIAL_EFFECT_ACTION_ATTACK,
  SPECIAL_EFFECT_ACTION_RESOURCE,
  SPECIAL_EFFECT_ACTION_DISABLED
};

enum action_e
{
  /// On use actions
  ACTION_USE = 0,

  /// Hostile spells
  ACTION_SPELL,

  /// Hostile attacks
  ACTION_ATTACK,

  /// Heals
  ACTION_HEAL,

  /// Absorbs
  ACTION_ABSORB,

  /// Sequences
  ACTION_SEQUENCE,

  /// Miscellaneous actions
  ACTION_OTHER,

  // Call other action lists
  ACTION_CALL,

  // Perform operations on a variable
  ACTION_VARIABLE,

  ACTION_MAX
};

enum action_var_e
{
  /// Invalid operation
  OPERATION_NONE = -1,

  /// Set variable to value
  OPERATION_SET,

  /// (debug) Print variable data to standard output
  OPERATION_PRINT,

  /// Reset variable to default value
  OPERATION_RESET,

  /// Add value to variable
  OPERATION_ADD,

  // Subtract value from variable
  OPERATION_SUB,

  /// Multiply variable by value
  OPERATION_MUL,

  /// Divide variable by value
  OPERATION_DIV,

  /// Raise variable to power of value
  OPERATION_POW,

  /// Take variable remainder of value
  OPERATION_MOD,

  /// Assign minimum of variable, value to variable
  OPERATION_MIN,

  /// Assign maximum of variable, value to variable
  OPERATION_MAX,

  /// Floor variable
  OPERATION_FLOOR,

  /// Raise variable to next integer value
  OPERATION_CEIL,

  ///Set variable to value if condition met
  OPERATION_SETIF
};

enum school_e
{
  SCHOOL_NONE = 0,
  SCHOOL_ARCANE,
  SCHOOL_FIRE,
  SCHOOL_FROST,
  SCHOOL_HOLY,
  SCHOOL_NATURE,
  SCHOOL_SHADOW,
  SCHOOL_PHYSICAL,
  SCHOOL_MAX_PRIMARY,
  SCHOOL_FROSTFIRE,
  SCHOOL_HOLYSTRIKE,
  SCHOOL_FLAMESTRIKE,
  SCHOOL_HOLYFIRE,
  SCHOOL_STORMSTRIKE,
  SCHOOL_HOLYSTORM,
  SCHOOL_FIRESTORM,
  SCHOOL_FROSTSTRIKE,
  SCHOOL_HOLYFROST,
  SCHOOL_FROSTSTORM,
  SCHOOL_SHADOWSTRIKE,
  SCHOOL_SHADOWLIGHT,
  SCHOOL_SHADOWFLAME,
  SCHOOL_SHADOWSTORM,
  SCHOOL_SHADOWFROST,
  SCHOOL_SPELLSTRIKE,
  SCHOOL_DIVINE,
  SCHOOL_SPELLFIRE,
  SCHOOL_ASTRAL,
  SCHOOL_SPELLFROST,
  SCHOOL_SPELLSHADOW,
  SCHOOL_ELEMENTAL,
  SCHOOL_CHROMATIC,
  SCHOOL_MAGIC,
  SCHOOL_CHAOS,
  SCHOOL_DRAIN,
  SCHOOL_MAX
};

enum school_mask_e
{
  SCHOOL_MASK_PHYSICAL = 0x01,
  SCHOOL_MASK_HOLY     = 0x02,
  SCHOOL_MASK_FIRE     = 0x04,
  SCHOOL_MASK_NATURE   = 0x08,
  SCHOOL_MASK_FROST    = 0x10,
  SCHOOL_MASK_SHADOW   = 0x20,
  SCHOOL_MASK_ARCANE   = 0x40,
};

const int64_t SCHOOL_ATTACK_MASK = ( ( int64_t( 1 ) << SCHOOL_PHYSICAL ) |
                                     ( int64_t( 1 ) << SCHOOL_HOLYSTRIKE ) |
                                     ( int64_t( 1 ) << SCHOOL_FLAMESTRIKE ) |
                                     ( int64_t( 1 ) << SCHOOL_STORMSTRIKE ) |
                                     ( int64_t( 1 ) << SCHOOL_FROSTSTRIKE ) |
                                     ( int64_t( 1 ) << SCHOOL_SHADOWSTRIKE ) |
                                     ( int64_t( 1 ) << SCHOOL_SPELLSTRIKE ) );
// SCHOOL_CHAOS should probably be added here too.

const int64_t SCHOOL_SPELL_MASK(
    ( int64_t( 1 ) << SCHOOL_ARCANE ) | ( int64_t( 1 ) << SCHOOL_CHAOS ) |
    ( int64_t( 1 ) << SCHOOL_FIRE ) | ( int64_t( 1 ) << SCHOOL_FROST ) |
    ( int64_t( 1 ) << SCHOOL_FROSTFIRE ) | ( int64_t( 1 ) << SCHOOL_HOLY ) |
    ( int64_t( 1 ) << SCHOOL_NATURE ) | ( int64_t( 1 ) << SCHOOL_SHADOW ) |
    ( int64_t( 1 ) << SCHOOL_HOLYSTRIKE ) |
    ( int64_t( 1 ) << SCHOOL_FLAMESTRIKE ) |
    ( int64_t( 1 ) << SCHOOL_HOLYFIRE ) |
    ( int64_t( 1 ) << SCHOOL_STORMSTRIKE ) |
    ( int64_t( 1 ) << SCHOOL_HOLYSTORM ) |
    ( int64_t( 1 ) << SCHOOL_FIRESTORM ) |
    ( int64_t( 1 ) << SCHOOL_FROSTSTRIKE ) |
    ( int64_t( 1 ) << SCHOOL_HOLYFROST ) |
    ( int64_t( 1 ) << SCHOOL_FROSTSTORM ) |
    ( int64_t( 1 ) << SCHOOL_SHADOWSTRIKE ) |
    ( int64_t( 1 ) << SCHOOL_SHADOWLIGHT ) |
    ( int64_t( 1 ) << SCHOOL_SHADOWFLAME ) |
    ( int64_t( 1 ) << SCHOOL_SHADOWSTORM ) |
    ( int64_t( 1 ) << SCHOOL_SHADOWFROST ) |
    ( int64_t( 1 ) << SCHOOL_SPELLSTRIKE ) | ( int64_t( 1 ) << SCHOOL_DIVINE ) |
    ( int64_t( 1 ) << SCHOOL_SPELLFIRE ) |
    ( int64_t( 1 ) << SCHOOL_ASTRAL ) |
    ( int64_t( 1 ) << SCHOOL_SPELLFROST ) |
    ( int64_t( 1 ) << SCHOOL_SPELLSHADOW ) |
    ( int64_t( 1 ) << SCHOOL_ELEMENTAL ) |
    ( int64_t( 1 ) << SCHOOL_CHROMATIC ) | ( int64_t( 1 ) << SCHOOL_MAGIC ) );

const int64_t SCHOOL_MAGIC_MASK( ( int64_t( 1 ) << SCHOOL_ARCANE ) |
                                 ( int64_t( 1 ) << SCHOOL_FIRE ) |
                                 ( int64_t( 1 ) << SCHOOL_FROST ) |
                                 ( int64_t( 1 ) << SCHOOL_FROSTFIRE ) |
                                 ( int64_t( 1 ) << SCHOOL_HOLY ) |
                                 ( int64_t( 1 ) << SCHOOL_NATURE ) |
                                 ( int64_t( 1 ) << SCHOOL_SHADOW ) );

const int64_t SCHOOL_ALL_MASK( -1 );

enum weapon_e
{
  WEAPON_NONE = 0,
  WEAPON_DAGGER,
  WEAPON_SMALL,
  WEAPON_BEAST,
  WEAPON_SWORD,
  WEAPON_MACE,
  WEAPON_AXE,
  WEAPON_FIST,
  WEAPON_WARGLAIVE,
  WEAPON_1H,
  WEAPON_BEAST_2H,
  WEAPON_SWORD_2H,
  WEAPON_MACE_2H,
  WEAPON_AXE_2H,
  WEAPON_STAFF,
  WEAPON_POLEARM,
  WEAPON_2H,
  WEAPON_BOW,
  WEAPON_CROSSBOW,
  WEAPON_GUN,
  WEAPON_WAND,
  WEAPON_THROWN,
  WEAPON_BEAST_RANGED,
  WEAPON_RANGED,
  WEAPON_MAX
};

enum slot_e  // these enum values match armory settings
{
  SLOT_INVALID   = -1,
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
  SLOT_MAX       = 19,
  SLOT_MIN       = 0
};

// Tiers 14..22 + PVP
const unsigned N_TIER   = 10;
const unsigned MIN_TIER = 14;

// Set bonus .. bonus. They map to a vector internally, so each enum value is just the vector
// element index.
enum set_bonus_e
{
  B_NONE = -1,
  B2     = 1,
  B3     = 2,
  B4     = 3,
  B5     = 4,
  B6     = 5,
  B7     = 6,
  B8     = 7,
  B_MAX  = 8
};

/**
 * @brief Type safe tier enum.
 *
 * MUST correspond to the ordering in
 * dbc_extract/dbc/generator.py SetBonusGenerator::set_bonus_map
 */
enum set_bonus_type_e
{
  SET_BONUS_NONE = -1,

  // Actual tier support in SIMC
  PVP,
  T17LFR,
  T18LFR,
  T19OH,
  T19P_G1,
  T19P_G2,
  T19P_CLOTH,
  T19P_LEATHER,
  T19P_MAIL,
  T19P_PLATE,
  T17,
  T18,
  T19,
  T20,
  T21,
  T21P_G1,
  T23_GIFT_OF_THE_LOA,
  T23_KEEPSAKES,

  SET_BONUS_MAX
};

enum meta_gem_e
{
  META_GEM_NONE = 0,
  META_AGILE_SHADOWSPIRIT,
  META_AGILE_PRIMAL,
  META_AUSTERE_EARTHSIEGE,
  META_AUSTERE_SHADOWSPIRIT,
  META_AUSTERE_PRIMAL,
  META_BEAMING_EARTHSIEGE,
  META_BRACING_EARTHSIEGE,
  META_BRACING_EARTHSTORM,
  META_BRACING_SHADOWSPIRIT,
  META_BURNING_SHADOWSPIRIT,
  META_BURNING_PRIMAL,
  META_CHAOTIC_SHADOWSPIRIT,
  META_CHAOTIC_SKYFIRE,
  META_CHAOTIC_SKYFLARE,
  META_DESTRUCTIVE_SHADOWSPIRIT,
  META_DESTRUCTIVE_SKYFIRE,
  META_DESTRUCTIVE_SKYFLARE,
  META_DESTRUCTIVE_PRIMAL,
  META_EFFULGENT_SHADOWSPIRIT,
  META_EFFULGENT_PRIMAL,
  META_EMBER_SHADOWSPIRIT,
  META_EMBER_PRIMAL,
  META_EMBER_SKYFIRE,
  META_EMBER_SKYFLARE,
  META_ENIGMATIC_SHADOWSPIRIT,
  META_ENIGMATIC_PRIMAL,
  META_ENIGMATIC_SKYFLARE,
  META_ENIGMATIC_STARFLARE,
  META_ENIGMATIC_SKYFIRE,
  META_ETERNAL_EARTHSIEGE,
  META_ETERNAL_EARTHSTORM,
  META_ETERNAL_SHADOWSPIRIT,
  META_ETERNAL_PRIMAL,
  META_FLEET_SHADOWSPIRIT,
  META_FLEET_PRIMAL,
  META_FORLORN_SHADOWSPIRIT,
  META_FORLORN_PRIMAL,
  META_FORLORN_SKYFLARE,
  META_FORLORN_STARFLARE,
  META_IMPASSIVE_SHADOWSPIRIT,
  META_IMPASSIVE_PRIMAL,
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
  META_POWERFUL_PRIMAL,
  META_RELENTLESS_EARTHSIEGE,
  META_RELENTLESS_EARTHSTORM,
  META_REVERBERATING_SHADOWSPIRIT,
  META_REVERBERATING_PRIMAL,
  META_REVITALIZING_SHADOWSPIRIT,
  META_REVITALIZING_PRIMAL,
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
  // Legendaries
  META_SINISTER_PRIMAL,
  META_COURAGEOUS_PRIMAL,
  META_INDOMITABLE_PRIMAL,
  META_CAPACITIVE_PRIMAL,
  META_GEM_MAX
};

enum stat_e
{
  STAT_NONE = 0,
  STAT_STRENGTH,
  STAT_AGILITY,
  STAT_STAMINA,
  STAT_INTELLECT,
  STAT_SPIRIT,
  STAT_AGI_INT,
  STAT_STR_AGI,
  STAT_STR_INT,
  STAT_STR_AGI_INT,
  STAT_HEALTH,
  STAT_MANA,
  STAT_RAGE,
  STAT_ENERGY,
  STAT_FOCUS,
  STAT_RUNIC,
  STAT_MAX_HEALTH,
  STAT_MAX_MANA,
  STAT_MAX_RAGE,
  STAT_MAX_ENERGY,
  STAT_MAX_FOCUS,
  STAT_MAX_RUNIC,
  STAT_SPELL_POWER,
  STAT_ATTACK_POWER,
  STAT_EXPERTISE_RATING,
  STAT_EXPERTISE_RATING2,
  STAT_HIT_RATING,
  STAT_HIT_RATING2,
  STAT_CRIT_RATING,
  STAT_HASTE_RATING,
  STAT_MASTERY_RATING,
  STAT_VERSATILITY_RATING,
  STAT_LEECH_RATING,
  STAT_SPEED_RATING,
  STAT_AVOIDANCE_RATING,
  STAT_ARMOR,
  STAT_BONUS_ARMOR,
  STAT_RESILIENCE_RATING,
  STAT_DODGE_RATING,
  STAT_PARRY_RATING,
  STAT_BLOCK_RATING, // Block CHANCE rating. Block damage reduction is in player_t::composite_block_reduction()
  STAT_PVP_POWER,
  STAT_WEAPON_DPS,
  STAT_WEAPON_OFFHAND_DPS,
  STAT_ALL,
  STAT_MAX
};
#define check( x )                                                             \
  static_assert( static_cast<int>( STAT_##x ) == static_cast<int>( ATTR_##x ), \
                 "stat_e and attribute_e must be kept in sync" )
check( STRENGTH );
check( AGILITY );
check( STAMINA );
check( INTELLECT );
check( SPIRIT );
check( AGI_INT );
check( STR_AGI );
check( STR_INT );
check( STR_AGI_INT );
#undef check

inline stat_e stat_from_attr( attribute_e a )
{
  // Assumes that ATTR_X == STAT_X
  return static_cast<stat_e>( a );
}

enum scale_metric_e
{
  SCALE_METRIC_NONE = 0,
  SCALE_METRIC_DPS,
  SCALE_METRIC_DPSP,
  SCALE_METRIC_DPSE,
  SCALE_METRIC_HPS,
  SCALE_METRIC_HPSE,
  SCALE_METRIC_APS,
  SCALE_METRIC_HAPS,
  SCALE_METRIC_DTPS,
  SCALE_METRIC_DMG_TAKEN,
  SCALE_METRIC_HTPS,
  SCALE_METRIC_TMI,
  SCALE_METRIC_ETMI,
  SCALE_METRIC_DEATHS,
  SCALE_METRIC_MAX
};

enum cache_e
{
  CACHE_NONE = 0,
  CACHE_STRENGTH,
  CACHE_AGILITY,
  CACHE_STAMINA,
  CACHE_INTELLECT,
  CACHE_SPIRIT,
  CACHE_AGI_INT,
  CACHE_STR_AGI,
  CACHE_STR_INT,
  CACHE_SPELL_POWER,
  CACHE_ATTACK_POWER,
  CACHE_EXP,
  CACHE_ATTACK_EXP,
  CACHE_HIT,
  CACHE_ATTACK_HIT,
  CACHE_SPELL_HIT,
  CACHE_CRIT_CHANCE,
  CACHE_ATTACK_CRIT_CHANCE,
  CACHE_SPELL_CRIT_CHANCE,
  CACHE_RPPM_CRIT,
  CACHE_HASTE,
  CACHE_ATTACK_HASTE,
  CACHE_SPELL_HASTE,
  CACHE_RPPM_HASTE,
  CACHE_SPEED,
  CACHE_ATTACK_SPEED,
  CACHE_SPELL_SPEED,
  CACHE_VERSATILITY,
  CACHE_DAMAGE_VERSATILITY,
  CACHE_HEAL_VERSATILITY,
  CACHE_MITIGATION_VERSATILITY,
  CACHE_MASTERY,
  CACHE_DODGE,
  CACHE_PARRY,
  CACHE_BLOCK,
  CACHE_CRIT_BLOCK,
  CACHE_ARMOR,
  CACHE_BONUS_ARMOR,
  CACHE_CRIT_AVOIDANCE,
  CACHE_MISS,
  CACHE_LEECH,
  CACHE_RUN_SPEED,
  CACHE_AVOIDANCE,
  CACHE_PLAYER_DAMAGE_MULTIPLIER,
  CACHE_PLAYER_HEAL_MULTIPLIER,
  CACHE_MAX
};

#define check( x )                                                   \
  static_assert(                                                     \
      static_cast<int>( CACHE_##x ) == static_cast<int>( ATTR_##x ), \
      "cache_e and attribute_e must be kept in sync" )
check( STRENGTH );
check( AGILITY );
check( STAMINA );
check( INTELLECT );
check( SPIRIT );
check( AGI_INT );
check( STR_AGI );
check( STR_INT );
#undef check

inline cache_e cache_from_stat( stat_e st )
{
  switch ( st )
    {
      case STAT_STRENGTH:
      case STAT_AGILITY:
      case STAT_STAMINA:
      case STAT_INTELLECT:
      case STAT_SPIRIT:
        return static_cast<cache_e>( st );
      case STAT_SPELL_POWER:
        return CACHE_SPELL_POWER;
      case STAT_ATTACK_POWER:
        return CACHE_ATTACK_POWER;
      case STAT_EXPERTISE_RATING:
      case STAT_EXPERTISE_RATING2:
        return CACHE_EXP;
      case STAT_HIT_RATING:
      case STAT_HIT_RATING2:
        return CACHE_HIT;
      case STAT_CRIT_RATING:
        return CACHE_CRIT_CHANCE;
      case STAT_HASTE_RATING:
        return CACHE_HASTE;
      case STAT_MASTERY_RATING:
        return CACHE_MASTERY;
      case STAT_DODGE_RATING:
        return CACHE_DODGE;
      case STAT_PARRY_RATING:
        return CACHE_PARRY;
      case STAT_BLOCK_RATING:
        return CACHE_BLOCK;
      case STAT_ARMOR:
        return CACHE_ARMOR;
      case STAT_BONUS_ARMOR:
        return CACHE_BONUS_ARMOR;
      case STAT_VERSATILITY_RATING:
        return CACHE_VERSATILITY;
      case STAT_LEECH_RATING:
        return CACHE_LEECH;
      case STAT_SPEED_RATING:
        return CACHE_RUN_SPEED;
      case STAT_AVOIDANCE_RATING:
        return CACHE_AVOIDANCE;
      default:
        break;
    }
  return CACHE_NONE;
}

enum position_e
{
  POSITION_NONE = 0,
  POSITION_FRONT,
  POSITION_BACK,
  POSITION_RANGED_FRONT,
  POSITION_RANGED_BACK,
  POSITION_MAX
};

enum profession_e
{
  PROFESSION_NONE = 0,
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

enum role_e
{
  ROLE_NONE = 0,
  ROLE_ATTACK,
  ROLE_SPELL,
  ROLE_HYBRID,
  ROLE_DPS,
  ROLE_TANK,
  ROLE_HEAL,
  ROLE_MAX
};

enum save_e : unsigned
{
  // Specifies the type of profile data to be saved
  SAVE_GEAR = 0x1,
  SAVE_TALENTS = 0x2,
  SAVE_ACTIONS = 0x4,
  SAVE_PLAYER = 0x8,
  SAVE_ALL = SAVE_GEAR | SAVE_TALENTS | SAVE_ACTIONS | SAVE_PLAYER,
};

enum power_e
{
  POWER_HEALTH      = -2,
  POWER_MANA        = 0,
  POWER_RAGE        = 1,
  POWER_FOCUS       = 2,
  POWER_ENERGY      = 3,
  POWER_COMBO_POINT = 4,
  POWER_RUNE        = 5,
  POWER_RUNIC_POWER = 6,
  POWER_SOUL_SHARDS = 7,
  POWER_ASTRAL_POWER = 8,
  POWER_HOLY_POWER = 9,
  // Not yet used (MoP Monk deprecated resource #1)
  // Not yet used
  POWER_MAELSTROM     = 11,
  POWER_CHI           = 12,
  POWER_INSANITY      = 13,
  POWER_BURNING_EMBER = 14,
  POWER_DEMONIC_FURY  = 15,
  // Not yet used?
  POWER_FURY          = 17,
  POWER_PAIN          = 18,
  // Helpers
  POWER_MAX,
  POWER_NONE   = 0xFFFFFFFF,  // None.
  POWER_OFFSET = 2,
};

// New stuff
enum snapshot_state_e
{
  STATE_HASTE          = 0x000001,
  STATE_CRIT           = 0x000002,
  STATE_AP             = 0x000004,
  STATE_SP             = 0x000008,

  STATE_MUL_DA         = 0x000010,
  STATE_MUL_TA         = 0x000020,
  STATE_VERSATILITY    = 0x000040,
  STATE_MUL_PERSISTENT = 0x000080,  // Persistent modifier for the few abilities that snapshot

  STATE_TGT_CRIT       = 0x000100,
  STATE_TGT_MUL_DA     = 0x000200,
  STATE_TGT_MUL_TA     = 0x000400,

  // User-defined state flags
  STATE_USER_1         = 0x001000,
  STATE_USER_2         = 0x002000,
  STATE_USER_3         = 0x004000,
  STATE_USER_4         = 0x008000,

  STATE_TGT_MITG_DA    = 0x010000,
  STATE_TGT_MITG_TA    = 0x020000,
  STATE_TGT_ARMOR      = 0x040000,

  /// Multiplier from the owner to pet damage
  STATE_MUL_PET        = 0x080000,

  // User-defined target-specific state flags
  STATE_TGT_USER_1     = 0x10000000,
  STATE_TGT_USER_2     = 0x20000000,
  STATE_TGT_USER_3     = 0x40000000,
  STATE_TGT_USER_4     = 0x80000000,

  /**
   * No multiplier helper, use in action_t::init() (after parent init) by issuing snapshot_flags &= STATE_NO_MULTIPLIER
   * (and/or update_flags &= STATE_NO_MULTIPLIER if a dot). This disables all multipliers, including versatility, and
   * any/all persistent multipliers the action would use. */
  STATE_NO_MULTIPLIER = ~( STATE_MUL_DA | STATE_MUL_TA | STATE_VERSATILITY | STATE_MUL_PERSISTENT | STATE_TGT_MUL_DA |
                           STATE_MUL_PET | STATE_TGT_MUL_TA | STATE_TGT_ARMOR ),

  /// Target-specific state variables
  STATE_TARGET =
      ( STATE_TGT_CRIT | STATE_TGT_MUL_DA | STATE_TGT_MUL_TA | STATE_TGT_ARMOR | STATE_TGT_MITG_DA | STATE_TGT_MITG_TA |
        STATE_TGT_USER_1 | STATE_TGT_USER_2 | STATE_TGT_USER_3 | STATE_TGT_USER_4 )
};

enum ready_e
{
  READY_POLL    = 0,
  READY_TRIGGER = 1
};

/// Real PPM scale stats
enum rppm_scale_e : uint8_t
{
  RPPM_NONE         = 0x00,
  RPPM_HASTE        = 0x01,
  RPPM_CRIT         = 0x02,
  RPPM_ATTACK_SPEED = 0x04,
  RPPM_DISABLE      = 0xff
};

enum action_energize_e
{
  ENERGIZE_NONE = 0,
  ENERGIZE_ON_CAST,
  ENERGIZE_ON_HIT,
  ENERGIZE_PER_HIT,
  ENERGIZE_PER_TICK
};

// A simple enumeration to indicate a broad haste stat type for various things in the simulator
enum haste_type_e
{
  HASTE_NONE = 0U,
  HASTE_SPELL,
  HASTE_ATTACK, // TODO: This should probably be Range/Melee
  SPEED_SPELL,
  SPEED_ATTACK,
  HASTE_ANY, // Special value to indicate any (all) haste types
  SPEED_ANY,
};

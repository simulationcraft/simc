// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SIMULATIONCRAFT_H
#define SIMULATIONCRAFT_H

#define SC_MAJOR_VERSION "530"
#define SC_MINOR_VERSION "3"
#define SC_USE_PTR ( 0 )
#define SC_BETA ( 0 )

// Platform Initialization ==================================================

#define SC_VERSION ( SC_MAJOR_VERSION "-" SC_MINOR_VERSION )

// Simplified access to compiler version
#if defined( __GNUC__ )
#  define SC_GCC ( __GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ )
#endif
#if defined( __clang__ )
#  define SC_CLANG ( __clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__ )
#endif
#if defined( _MSC_VER )
#  define SC_VS ( _MSC_VER / 100 - 6 )
#  if SC_VS < 9
#    error "Visual Studio 8 ( 2005 ) or lower not supported"
#  endif
#endif

#if defined(__SSE2__) || ( defined( SC_VS ) && ( defined(_M_X64) || ( defined(_M_IX86_FP) && _M_IX86_FP >= 2 ) ) )
#  define SC_USE_SSE2
#endif

#if defined(__APPLE__) || defined(__MACH__)
#  define SC_OSX
#endif

#if defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32 )
#  define SC_WINDOWS
#  if defined( __MINGW__ ) || defined( __MINGW32__ )
#    define SC_MINGW
#  endif
#  if defined(SC_USE_SSE2)
#    if defined(SC_MINGW)
       // <HACK> Include these headers (in this order) early to avoid
       // an order-of-inclusion bug with MinGW headers.
#      include <stdlib.h>
       // Workaround MinGW header bug: http://sourceforge.net/tracker/?func=detail&atid=102435&aid=2962480&group_id=2435
       extern "C" {
#        include <emmintrin.h>
       }
#      include <malloc.h>
       // </HACK>
#    else
#      include <emmintrin.h>
#    endif
#  endif
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#  endif
#  define DIRECTORY_DELIMITER "\\"
#  ifndef UNICODE
#    define UNICODE
#  endif
#else
#  if defined(SC_USE_SSE2)
#    include <emmintrin.h>
#    include <mm_malloc.h>
#  endif
#  define DIRECTORY_DELIMITER "/"
#  define SC_SIGACTION
#endif

// Workaround for LLVM/Clang 3.2+ using glibc headers.
#if defined( SC_CLANG ) && SC_CLANG >= 30200
# define __extern_always_inline extern __always_inline __attribute__(( __gnu_inline__ ))
#endif

// C++11 workarounds for older compiler versions.
#if __cplusplus < 201103L && ( ! defined( SC_GCC ) || ! __GXX_EXPERIMENTAL_CXX0X__ || SC_GCC < 40600 ) && ( ! defined( SC_VS ) || SC_VS < 10 )
namespace std {
class nullptr_t
{
public:
  template <typename T> operator T* () const { return 0; }
  template <typename T, typename U> operator T U::* () const { return 0; }
  operator bool () const { return false; }
};
}
#define nullptr ( std::nullptr_t() )
#endif

#if defined( SC_VS ) && SC_VS < 10
#  include "../vs/stdint.h"
#else
#  include <stdint.h>
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <typeinfo>
#include <vector>

#if defined( SC_VS ) || __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
// Use C++11
#include <array>
#include <memory>
#include <type_traits>
#include <unordered_map>
#if ( defined( SC_VS ) && SC_VS < 10 )
namespace std {using namespace tr1; }
#endif
#else
// Use TR1
#include <tr1/array>
#include <tr1/functional>
#include <tr1/memory>
#include <tr1/type_traits>
#include <tr1/unordered_map>
namespace std {using namespace tr1; }
#endif

#include "dbc/data_enums.hh"
#include "dbc/data_definitions.hh"
#include "utf8.h"

#if __BSD_VISIBLE
#  include <netinet/in.h>
#  if !defined(CLOCKS_PER_SEC)
#    define CLOCKS_PER_SEC 1000000
#  endif
#endif

#if ( ! defined(_MSC_VER) || _MSC_VER < 1600 ) && __cplusplus < 201103L && ! defined(__GXX_EXPERIMENTAL_CXX0X__)
#define static_assert( condition, message )
#endif

#if defined(__GNUC__)
#  define likely(x)         __builtin_expect((x),1)
#  define unlikely(x)       __builtin_expect((x),0)
#else
#  define likely(x)        (x)
#  define unlikely(x)      (x)
#  define __attribute__(x)
#endif

#define SC_PACKED_STRUCT      __attribute__((packed))
#define PRINTF_ATTRIBUTE(a,b) __attribute__((format(printf,a,b)))

#ifndef M_PI
#define M_PI ( 3.14159265358979323846 )
#endif

#define SC_STAT_CACHE

#define SC_USE_INTEGER_TIME
#include "sc_timespan.hpp"

// Generic programming tools
#include "sc_generic.hpp"

// Sample Data
#include "util/sample_data.hpp"

// Timeline
#include "util/timeline.hpp"

/* SimulationCraft timeline:
 * - data_type is double
 * - timespan_t add helper function
 */
struct sc_timeline_t : public tl::timeline_t<double>
{
  typedef tl::timeline_t<double> base_t;
  using base_t::add;

  // Add 'value' at the corresponding time
  void add( timespan_t current_time, double value )
  { base_t::add( static_cast<size_t>( current_time.total_millis() / 1000 ), value ); }

};

// Random Number Generators
#include "util/rng.hpp"

/* Derived SimulationCraft RNG Distribution class with some timespan_t helper function
 */
template <typename RNG_GENERATOR>
class sc_distribution_t : public rng::distribution_t<RNG_GENERATOR>
{
public:
  typedef rng::distribution_t<RNG_GENERATOR> base_t;
  sc_distribution_t() : base_t() {}

  // Make sure base functions aren't hidden, because overload resolution does not search for them
  using base_t::range;
  using base_t::gauss;
  using base_t::exgauss;

  timespan_t range( timespan_t min, timespan_t max )
  {
    return timespan_t::from_native( range( static_cast<double>( timespan_t::to_native( min ) ),
                                           static_cast<double>( timespan_t::to_native( max ) ) ) );
  }

  timespan_t gauss( timespan_t mean, timespan_t stddev )
  {
    return timespan_t::from_native( gauss( static_cast<double>( timespan_t::to_native( mean ) ),
                                           static_cast<double>( timespan_t::to_native( stddev ) ) ) );
  }

  timespan_t exgauss( timespan_t mean, timespan_t stddev, timespan_t nu )
  {
    return timespan_t::from_native(
             exgauss( static_cast<double>( timespan_t::to_native( mean   ) ),
                      static_cast<double>( timespan_t::to_native( stddev ) ),
                      static_cast<double>( timespan_t::to_native( nu ) )
                    )
           );
  }

};

// Hookup rng containers for easy use in SimulationCraft

#if defined(RNG_USE_SSE2)
typedef sc_distribution_t<rng::rng_engine_mt_sse2_t> rng_t;
#else
typedef sc_distribution_t<rng::rng_engine_mt_t> rng_t;
#endif


// Forward Declarations =====================================================

struct absorb_buff_t;
struct action_callback_t;
struct action_priority_t;
struct action_priority_list_t;
struct action_state_t;
struct action_t;
struct alias_t;
struct attack_t;
struct benefit_t;
struct buff_t;
struct callback_t;
struct cooldown_t;
struct cost_reduction_buff_t;
class  dbc_t;
struct debuff_t;
struct dot_t;
struct event_t;
class  expr_t;
struct gain_t;
struct haste_buff_t;
struct heal_state_t;
struct item_t;
struct js_node_t;
struct module_t;
class option_t;
struct pet_t;
struct player_t;
struct plot_t;
struct proc_t;
struct reforge_plot_t;
struct scaling_t;
struct sim_t;
struct spell_data_t;
struct spell_id_t;
struct spelleffect_data_t;
struct stats_t;
struct stat_buff_t;
struct stat_pair_t;
struct stormlash_callback_t;
struct tick_buff_t;
struct travel_event_t;
struct xml_node_t;

// Enumerations =============================================================
// annex _e to enumerations

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
  RACE_BEAST, RACE_DRAGONKIN, RACE_GIANT, RACE_HUMANOID, RACE_DEMON, RACE_ELEMENTAL,
  // Player Races
  RACE_NIGHT_ELF, RACE_HUMAN, RACE_GNOME, RACE_DWARF, RACE_DRAENEI, RACE_WORGEN,
  RACE_ORC, RACE_TROLL, RACE_UNDEAD, RACE_BLOOD_ELF, RACE_TAUREN, RACE_GOBLIN,
  RACE_PANDAREN, RACE_PANDAREN_ALLIANCE, RACE_PANDAREN_HORDE,
  RACE_MAX
};

inline bool is_pandaren( race_e r ) { return RACE_PANDAREN <= r && r <= RACE_PANDAREN_HORDE; }

enum player_e
{
  PLAYER_SPECIAL_SCALE4 = -4,
  PLAYER_SPECIAL_SCALE3 = -3,
  PLAYER_SPECIAL_SCALE2 = -2,
  PLAYER_SPECIAL_SCALE = -1,
  PLAYER_NONE = 0,
  DEATH_KNIGHT, DRUID, HUNTER, MAGE, MONK, PALADIN, PRIEST, ROGUE, SHAMAN, WARLOCK, WARRIOR,
  PLAYER_PET, PLAYER_GUARDIAN,
  HEALING_ENEMY, ENEMY, ENEMY_ADD,
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

  PET_TENACITY_TYPE,

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
  PET_PRIEST,

  PET_SPIRIT_WOLF,
  PET_FIRE_ELEMENTAL,
  PET_EARTH_ELEMENTAL,
  PET_SHAMAN,

  PET_ENEMY,

  PET_MAX
};

enum dmg_e { RESULT_TYPE_NONE = -1, DMG_DIRECT = 0, DMG_OVER_TIME = 1, HEAL_DIRECT, HEAL_OVER_TIME, ABSORB };

enum stats_e { STATS_DMG, STATS_HEAL, STATS_ABSORB, STATS_NEUTRAL };

enum dot_behavior_e { DOT_CLIP, DOT_REFRESH, DOT_EXTEND };

enum attribute_e { ATTRIBUTE_NONE = 0, ATTR_STRENGTH, ATTR_AGILITY, ATTR_STAMINA, ATTR_INTELLECT, ATTR_SPIRIT, ATTRIBUTE_MAX };

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
  RESOURCE_ECLIPSE,
  RESOURCE_HOLY_POWER,
  /* Unknown_2, */
  /* Unknown_3 */
  RESOURCE_CHI,
  RESOURCE_SHADOW_ORB,
  RESOURCE_BURNING_EMBER,
  RESOURCE_DEMONIC_FURY,
  /* Dummy resources for reporting */
  RESOURCE_RUNE,
  RESOURCE_RUNE_BLOOD,
  RESOURCE_RUNE_UNHOLY,
  RESOURCE_RUNE_FROST,
  RESOURCE_MAX
};

enum result_e
{
  RESULT_UNKNOWN = -1,
  RESULT_NONE = 0,
  RESULT_MISS,  RESULT_DODGE, RESULT_PARRY,
  RESULT_BLOCK, RESULT_CRIT_BLOCK, RESULT_GLANCE, RESULT_CRIT, RESULT_HIT,
  RESULT_MAX
};

#define RESULT_HIT_MASK   ( (1<<RESULT_GLANCE) | (1<<RESULT_BLOCK) | (1<<RESULT_CRIT_BLOCK) | (1<<RESULT_CRIT) | (1<<RESULT_HIT) )
#define RESULT_CRIT_MASK  ( (1<<RESULT_CRIT) )
#define RESULT_MISS_MASK  ( (1<<RESULT_MISS) )
#define RESULT_BLOCK_MASK ( (1<<RESULT_BLOCK) )
#define RESULT_DODGE_MASK ( (1<<RESULT_DODGE) )
#define RESULT_PARRY_MASK ( (1<<RESULT_PARRY) )
#define RESULT_NONE_MASK  ( (1<<RESULT_NONE) )
#define RESULT_ALL_MASK  -1

enum proc_e
{
  PROC_NONE = 0,
  PROC_DAMAGE,
  PROC_HEAL,
  PROC_TICK_DAMAGE,
  PROC_SPELL_TICK_DAMAGE,
  PROC_DIRECT_HARMFUL_SPELL,
  PROC_SPELL_DIRECT_DAMAGE,
  PROC_DIRECT_DAMAGE,
  PROC_DIRECT_CRIT,
  PROC_DIRECT_HEAL,
  PROC_TICK_HEAL,
  PROC_ATTACK,
  PROC_SPELL,
  PROC_TICK,
  PROC_SPELL_AND_TICK,
  PROC_HEAL_SPELL,
  PROC_HARMFUL_SPELL,
  PROC_HARMFUL_SPELL_LANDING,
  PROC_DAMAGE_HEAL,
  PROC_DAMAGE_HEAL_SPELL,
  PROC_MAX
};

enum action_e { ACTION_USE = 0, ACTION_SPELL, ACTION_ATTACK, ACTION_HEAL, ACTION_ABSORB, ACTION_SEQUENCE, ACTION_OTHER, ACTION_MAX };

enum school_e
{
  SCHOOL_NONE = 0,
  SCHOOL_ARCANE,      SCHOOL_FIRE,        SCHOOL_FROST,       SCHOOL_HOLY,        SCHOOL_NATURE,
  SCHOOL_SHADOW,      SCHOOL_PHYSICAL,    SCHOOL_MAX_PRIMARY, SCHOOL_FROSTFIRE,
  SCHOOL_HOLYSTRIKE,  SCHOOL_FLAMESTRIKE, SCHOOL_HOLYFIRE,    SCHOOL_STORMSTRIKE, SCHOOL_HOLYSTORM,
  SCHOOL_FIRESTORM,   SCHOOL_FROSTSTRIKE, SCHOOL_HOLYFROST,   SCHOOL_FROSTSTORM,  SCHOOL_SHADOWSTRIKE,
  SCHOOL_SHADOWLIGHT, SCHOOL_SHADOWFLAME, SCHOOL_SHADOWSTORM, SCHOOL_SHADOWFROST, SCHOOL_SPELLSTRIKE,
  SCHOOL_DIVINE,      SCHOOL_SPELLFIRE,   SCHOOL_SPELLSTORM,  SCHOOL_SPELLFROST,  SCHOOL_SPELLSHADOW,
  SCHOOL_ELEMENTAL,   SCHOOL_CHROMATIC,   SCHOOL_MAGIC,       SCHOOL_CHAOS,
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

const int64_t SCHOOL_ATTACK_MASK = ( ( int64_t( 1 ) << SCHOOL_PHYSICAL )     |
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

const int64_t SCHOOL_MAGIC_MASK  ( ( int64_t( 1 ) << SCHOOL_ARCANE )         |
                                   ( int64_t( 1 ) << SCHOOL_FIRE )           | ( int64_t( 1 ) << SCHOOL_FROST )        |
                                   ( int64_t( 1 ) << SCHOOL_FROSTFIRE )      | ( int64_t( 1 ) << SCHOOL_HOLY )         |
                                   ( int64_t( 1 ) << SCHOOL_NATURE )         | ( int64_t( 1 ) << SCHOOL_SHADOW ) );
#define SCHOOL_ALL_MASK    ( int64_t( -1 ) )

enum weapon_e
{
  WEAPON_NONE = 0,
  WEAPON_DAGGER,                                                                                   WEAPON_SMALL,
  WEAPON_BEAST,    WEAPON_SWORD,    WEAPON_MACE,     WEAPON_AXE,    WEAPON_FIST,                   WEAPON_1H,
  WEAPON_BEAST_2H, WEAPON_SWORD_2H, WEAPON_MACE_2H,  WEAPON_AXE_2H, WEAPON_STAFF,  WEAPON_POLEARM, WEAPON_2H,
  WEAPON_BOW,      WEAPON_CROSSBOW, WEAPON_GUN,      WEAPON_WAND,   WEAPON_THROWN,                 WEAPON_RANGED,
  WEAPON_MAX
};

enum glyph_e
{
  GLYPH_MAJOR = 0,
  GLYPH_MINOR,
  GLYPH_PRIME,
  GLYPH_MAX
};

enum slot_e   // these enum values match armory settings
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

// Tiers 13..16 + PVP
#define N_TIER 5
#define MIN_TIER ( 13 )

// Caster 2/4, Melee 2/4, Tank 2/4, Heal 2/4
#define N_TIER_BONUS 8

typedef uint32_t set_bonus_description_t[N_TIER][N_TIER_BONUS];

enum set_e
{
  SET_NONE = 0,
  SET_T13_CASTER, SET_T13_2PC_CASTER, SET_T13_4PC_CASTER,
  SET_T13_MELEE,  SET_T13_2PC_MELEE,  SET_T13_4PC_MELEE,
  SET_T13_TANK,   SET_T13_2PC_TANK,   SET_T13_4PC_TANK,
  SET_T13_HEAL,   SET_T13_2PC_HEAL,   SET_T13_4PC_HEAL,
  SET_T14_CASTER, SET_T14_2PC_CASTER, SET_T14_4PC_CASTER,
  SET_T14_MELEE,  SET_T14_2PC_MELEE,  SET_T14_4PC_MELEE,
  SET_T14_TANK,   SET_T14_2PC_TANK,   SET_T14_4PC_TANK,
  SET_T14_HEAL,   SET_T14_2PC_HEAL,   SET_T14_4PC_HEAL,
  SET_T15_CASTER, SET_T15_2PC_CASTER, SET_T15_4PC_CASTER,
  SET_T15_MELEE,  SET_T15_2PC_MELEE,  SET_T15_4PC_MELEE,
  SET_T15_TANK,   SET_T15_2PC_TANK,   SET_T15_4PC_TANK,
  SET_T15_HEAL,   SET_T15_2PC_HEAL,   SET_T15_4PC_HEAL,
  SET_T16_CASTER, SET_T16_2PC_CASTER, SET_T16_4PC_CASTER,
  SET_T16_MELEE,  SET_T16_2PC_MELEE,  SET_T16_4PC_MELEE,
  SET_T16_TANK,   SET_T16_2PC_TANK,   SET_T16_4PC_TANK,
  SET_T16_HEAL,   SET_T16_2PC_HEAL,   SET_T16_4PC_HEAL,
  SET_PVP_CASTER, SET_PVP_2PC_CASTER, SET_PVP_4PC_CASTER,
  SET_PVP_MELEE,  SET_PVP_2PC_MELEE,  SET_PVP_4PC_MELEE,
  SET_PVP_TANK,   SET_PVP_2PC_TANK,   SET_PVP_4PC_TANK,
  SET_PVP_HEAL,   SET_PVP_2PC_HEAL,   SET_PVP_4PC_HEAL,
  SET_MAX
};
static_assert( static_cast<int>( SET_MAX ) == ( 1 + N_TIER * 3 * N_TIER_BONUS / 2 ), "enum set_e must be structured correctly." );

enum gem_e
{
  GEM_NONE = 0,
  GEM_META, GEM_PRISMATIC,
  GEM_RED, GEM_YELLOW, GEM_BLUE,
  GEM_ORANGE, GEM_GREEN, GEM_PURPLE,
  GEM_COGWHEEL, GEM_HYDRAULIC,
  GEM_MAX
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
  STAT_STRENGTH, STAT_AGILITY, STAT_STAMINA, STAT_INTELLECT, STAT_SPIRIT,
  STAT_HEALTH, STAT_MANA, STAT_RAGE, STAT_ENERGY, STAT_FOCUS, STAT_RUNIC,
  STAT_MAX_HEALTH, STAT_MAX_MANA, STAT_MAX_RAGE, STAT_MAX_ENERGY, STAT_MAX_FOCUS, STAT_MAX_RUNIC,
  STAT_SPELL_POWER,
  STAT_ATTACK_POWER, STAT_EXPERTISE_RATING, STAT_EXPERTISE_RATING2,
  STAT_HIT_RATING, STAT_HIT_RATING2, STAT_CRIT_RATING, STAT_HASTE_RATING, STAT_MASTERY_RATING,
  STAT_WEAPON_DPS, STAT_WEAPON_SPEED,
  STAT_WEAPON_OFFHAND_DPS, STAT_WEAPON_OFFHAND_SPEED,
  STAT_ARMOR, STAT_RESILIENCE_RATING, STAT_DODGE_RATING, STAT_PARRY_RATING,
  STAT_BLOCK_RATING, STAT_PVP_POWER,
  STAT_ALL,
  STAT_MAX
};
#define check(x) static_assert( static_cast<int>( STAT_##x ) == static_cast<int>( ATTR_##x ), \
                                "stat_e and attribute_e must be kept in sync" )
check( STRENGTH );
check( AGILITY );
check( STAMINA );
check( INTELLECT );
check( SPIRIT );
#undef check

inline stat_e stat_from_attr( attribute_e a )
{
  // Assumes that ATTR_X == STAT_X
  return static_cast<stat_e>( a );
}

enum cache_e
{
  CACHE_NONE = 0,
  CACHE_STRENGTH, CACHE_AGILITY, CACHE_STAMINA, CACHE_INTELLECT, CACHE_SPIRIT,
  CACHE_SPELL_POWER, CACHE_ATTACK_POWER,
  CACHE_EXP,   CACHE_ATTACK_EXP,
  CACHE_HIT,   CACHE_ATTACK_HIT,   CACHE_SPELL_HIT,
  CACHE_CRIT,  CACHE_ATTACK_CRIT,  CACHE_SPELL_CRIT,
  CACHE_HASTE, CACHE_ATTACK_HASTE, CACHE_SPELL_HASTE,
  CACHE_SPEED, CACHE_ATTACK_SPEED, CACHE_SPELL_SPEED,
  CACHE_MASTERY,
  CACHE_DODGE, CACHE_PARRY, CACHE_BLOCK, CACHE_CRIT_BLOCK, CACHE_ARMOR,
  CACHE_CRIT_AVOIDANCE, CACHE_MISS,
  CACHE_PLAYER_DAMAGE_MULTIPLIER,
  CACHE_PLAYER_HEAL_MULTIPLIER,
  CACHE_MAX
};

#define check(x) static_assert( static_cast<int>( CACHE_##x ) == static_cast<int>( ATTR_##x ), \
                                "cache_e and attribute_e must be kept in sync" )
check( STRENGTH );
check( AGILITY );
check( STAMINA );
check( INTELLECT );
check( SPIRIT );
#undef check

inline cache_e cache_from_stat( stat_e st )
{
  switch ( st )
  {
    case STAT_STRENGTH: case STAT_AGILITY: case STAT_STAMINA: case STAT_INTELLECT: case STAT_SPIRIT:
      return static_cast<cache_e>( st );
    case STAT_SPELL_POWER: return CACHE_SPELL_POWER;
    case STAT_ATTACK_POWER: return CACHE_ATTACK_POWER;
    case STAT_EXPERTISE_RATING: case STAT_EXPERTISE_RATING2: return CACHE_EXP;
    case STAT_HIT_RATING: case STAT_HIT_RATING2: return CACHE_HIT;
    case STAT_CRIT_RATING: return CACHE_CRIT;
    case STAT_HASTE_RATING: return CACHE_HASTE;
    case STAT_MASTERY_RATING: return CACHE_MASTERY;
    case STAT_DODGE_RATING: return CACHE_DODGE;
    case STAT_PARRY_RATING: return CACHE_PARRY;
    case STAT_BLOCK_RATING: return CACHE_BLOCK;
    case STAT_ARMOR: return CACHE_ARMOR;
    default: break;
  }
  return CACHE_NONE;
}

enum flask_e
{
  FLASK_NONE = 0,
  // cataclysm
  FLASK_DRACONIC_MIND,
  FLASK_FLOWING_WATER,
  FLASK_STEELSKIN,
  FLASK_TITANIC_STRENGTH,
  FLASK_WINDS,
  // mop
  FLASK_WARM_SUN,
  FLASK_FALLING_LEAVES,
  FLASK_EARTH,
  FLASK_WINTERS_BITE,
  FLASK_SPRING_BLOSSOMS,
  FLASK_CRYSTAL_OF_INSANITY,
  // alchemist's flask
  FLASK_ALCHEMISTS,
  FLASK_MAX
};

enum food_e
{
  FOOD_NONE = 0,
  FOOD_BAKED_ROCKFISH,
  FOOD_BANQUET_OF_THE_BREW,
  FOOD_BANQUET_OF_THE_GRILL,
  FOOD_BANQUET_OF_THE_OVEN,
  FOOD_BANQUET_OF_THE_POT,
  FOOD_BANQUET_OF_THE_STEAMER,
  FOOD_BANQUET_OF_THE_WOK,
  FOOD_BASILISK_LIVERDOG,
  FOOD_BEER_BASTED_CROCOLISK,
  FOOD_BLACK_PEPPER_RIBS_AND_SHRIMP,
  FOOD_BLACKBELLY_SUSHI,
  FOOD_BOILED_SILKWORM_PUPA,
  FOOD_BRAISED_TURTLE,
  FOOD_BLANCHED_NEEDLE_MUSHROOMS,
  FOOD_CHARBROILED_TIGER_STEAK,
  FOOD_CHUN_TIAN_SPRING_ROLLS,
  FOOD_CROCOLISK_AU_GRATIN,
  FOOD_DELICIOUS_SAGEFISH_TAIL,
  FOOD_DRIED_NEEDLE_MUSHROOMS,
  FOOD_DRIED_PEACHES,
  FOOD_ETERNAL_BLOSSOM_FISH,
  FOOD_FIRE_SPIRIT_SALMON,
  FOOD_FISH_FEAST,
  FOOD_FORTUNE_COOKIE,
  FOOD_GREAT_BANQUET_OF_THE_BREW,
  FOOD_GREAT_BANQUET_OF_THE_GRILL,
  FOOD_GREAT_BANQUET_OF_THE_OVEN,
  FOOD_GREAT_BANQUET_OF_THE_POT,
  FOOD_GREAT_BANQUET_OF_THE_STEAMER,
  FOOD_GREAT_BANQUET_OF_THE_WOK,
  FOOD_GREAT_PANDAREN_BANQUET,
  FOOD_GREEN_CURRY_FISH,
  FOOD_GRILLED_DRAGON,
  FOOD_LAVASCALE_FILLET,
  FOOD_MOGU_FISH_STEW,
  FOOD_MUSHROOM_SAUCE_MUDFISH,
  FOOD_PANDAREN_BANQUET,
  FOOD_PEACH_PIE,
  FOOD_PEARL_MILK_TEA,
  FOOD_POUNDED_RICE_CAKE,
  FOOD_RED_BEAN_BUN,
  FOOD_RICE_PUDDING,
  FOOD_ROASTED_BARLEY_TEA,
  FOOD_SAUTEED_CARROTS,
  FOOD_SEA_MIST_RICE_NOODLES,
  FOOD_SEAFOOD_MAGNIFIQUE_FEAST,
  FOOD_SEVERED_SAGEFISH_HEAD,
  FOOD_SHRIMP_DUMPLINGS,
  FOOD_SKEWERED_EEL,
  FOOD_SKEWERED_PEANUT_CHICKEN,
  FOOD_SPICY_SALMON,
  FOOD_SPICY_VEGETABLE_BOWL,
  FOOD_STEAMED_CRAB_SURPRISE,
  FOOD_SWIRLING_MIST_SOUP,
  FOOD_TANGY_YOGURT,
  FOOD_TOASTED_FISH_JERKY,
  FOOD_TWIN_FISH_PLATTER,
  FOOD_VALLEY_STIR_FRY,
  FOOD_WILDFOWL_GINSENG_SOUP,
  FOOD_WILDFOWL_ROAST,
  FOOD_YAK_CHEESE_CURDS,
  FOOD_MAX
};

enum position_e { POSITION_NONE = 0, POSITION_FRONT, POSITION_BACK, POSITION_RANGED_FRONT, POSITION_RANGED_BACK, POSITION_MAX };

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

enum role_e { ROLE_NONE = 0, ROLE_ATTACK, ROLE_SPELL, ROLE_HYBRID, ROLE_DPS, ROLE_TANK, ROLE_HEAL, ROLE_MAX };

enum save_e
{
  // Specifies the type of profile data to be saved
  SAVE_ALL = 0,
  SAVE_GEAR,
  SAVE_TALENTS,
  SAVE_ACTIONS,
  SAVE_MAX
};

enum power_e
{
  POWER_HEALTH        = -2,
  POWER_MANA          = 0,
  POWER_RAGE          = 1,
  POWER_FOCUS         = 2,
  POWER_ENERGY        = 3,
  POWER_MONK_ENERGY   = 4, // translated to RESOURCE_ENERGY
  POWER_RUNE          = 5,
  POWER_RUNIC_POWER   = 6,
  POWER_SOUL_SHARDS   = 7,
  // Not yet used
  POWER_HOLY_POWER    = 9,
  // Not yet used (MoP Monk deprecated resource #1)
  // Not yet used
  POWER_CHI           = 12,
  POWER_SHADOW_ORB    = 13,
  POWER_BURNING_EMBER = 14,
  POWER_DEMONIC_FURY  = 15,
  // Helpers
  POWER_MAX           = 16,
  POWER_NONE          = 0xFFFFFFFF, // None.
  POWER_OFFSET        = 2,
};

// New stuff
enum snapshot_state_e
{
  STATE_HASTE         = 0x000001,
  STATE_CRIT          = 0x000002,
  STATE_AP            = 0x000004,
  STATE_SP            = 0x000008,

  STATE_MUL_DA        = 0x000010,
  STATE_MUL_TA        = 0x000020,
  STATE_TGT_CRIT      = 0x000100,
  STATE_TGT_MUL_DA    = 0x000200,
  STATE_TGT_MUL_TA    = 0x000400,

  STATE_USER_1        = 0x000800,
  STATE_USER_2        = 0x001000,
  STATE_USER_3        = 0x002000,
  STATE_USER_4        = 0x004000,

  STATE_TGT_MITG_DA   = 0x008000,
  STATE_TGT_MITG_TA   = 0x010000,
};

enum ready_e
{ READY_POLL = 0, READY_TRIGGER = 1 };

// Real PPM scale stats
enum rppm_scale_e
{
  RPPM_HASTE = 0,
  RPPM_SPELL_CRIT,
  RPPM_ATTACK_CRIT
};

// Cache Control ============================================================

namespace cache {

typedef int era_t;
static const era_t INVALID_ERA = -1;
static const era_t IN_THE_BEGINNING = 0;  // A time before any other possible era;
// used to mark persistent caches at load.

enum behavior_e
{
  ANY,      // * Use any version present in the cache, retrieve if not present.
  CURRENT,  // * Use only current info from the cache; validate old versions as needed.
  ONLY,     // * Use any version present in the cache, fail if not present.
};

class cache_control_t
{
private:
  era_t current_era;
  behavior_e player_cache_behavior;
  behavior_e item_cache_behavior;

public:
  cache_control_t() :
    current_era( IN_THE_BEGINNING ),
    player_cache_behavior( CURRENT ),
    item_cache_behavior( ANY )
  {}

  era_t era() const { return current_era; }
  void advance_era() { ++current_era; }

  behavior_e cache_players() const { return player_cache_behavior; }
  void cache_players( behavior_e b ) { player_cache_behavior = b; }

  behavior_e cache_items() const { return item_cache_behavior; }
  void cache_items( behavior_e b ) { item_cache_behavior = b; }

  static cache_control_t singleton;
};

// Caching system's global notion of the current time.
inline era_t era()
{ return cache_control_t::singleton.era(); }

// Time marches on.
inline void advance_era()
{ cache_control_t::singleton.advance_era(); }

// Get/Set default cache behaviors.
inline behavior_e players()
{ return cache_control_t::singleton.cache_players(); }
inline void players( behavior_e b )
{ cache_control_t::singleton.cache_players( b ); }

inline behavior_e items()
{ return cache_control_t::singleton.cache_items(); }
inline void items( behavior_e b )
{ cache_control_t::singleton.cache_items( b ); }

} // cache

struct stat_data_t
{
  double strength;
  double agility;
  double stamina;
  double intellect;
  double spirit;
};

// Options ==================================================================

class option_t
{
public:
  typedef bool function_t( sim_t* sim, const std::string& name, const std::string& value );
  typedef std::map<std::string, std::string> map_t;
  typedef std::vector<std::string> list_t;

  enum option_e
  {
    NONE = 0,
    BOOL,          // bool
    INT,           // int
    INT_BOOL,      // int (only valid values are 1 and 0)
    UINT,          // unsigned int
    FLT,           // double
    STRING,        // std::string
    APPEND,        // std::string (append)
    TIMESPAN,      // timespan_t
    LIST,          // std::vector<std::string>
    MAP,           // std::map<std::string,std::string>
    FUNC,          // function_t*
#if 0
    TALENT_RANK,   // talent rank
    SPELL_ENABLED, // spell enabled
#endif
    DEPRECATED
  };

private:
  const char* name;
  option_e type;
  union data_t
  {
    void* address;
    const char* cstr;
    function_t* func;
    data_t( void* p ) : address( p ) {}
    data_t( function_t* p ) : func( p ) {}
    data_t( const char* p ) : cstr( p ) {}
  } data;

  void print( FILE*, bool print_if_zero );

public:
  option_t( const char* n, option_e t, void* a ) : name( n ), type( t ), data( a ) {}
  option_t( const char* n, const char* str ) : name( n ), type( DEPRECATED ), data( str ) {}
  option_t( const char* n, function_t* f ) : name( n ), type( FUNC ), data( f ) {}

  void print( FILE* f ) { print( f, true ); }
  void save ( FILE* f ) { print ( f, false ); }
  bool parse( sim_t*, const std::string& name, const std::string& value );

  static void copy( std::vector<option_t>& opt_vector, const option_t* opt_array );
  static bool parse( sim_t*, std::vector<option_t>&, const std::string& name, const std::string& value );
  static bool parse( sim_t*, const char* context, std::vector<option_t>&, const std::string& options_str );
  static bool parse( sim_t*, const char* context, std::vector<option_t>&, const std::vector<std::string>& strings );
  static bool parse( sim_t*, const char* context, const option_t*,        const std::vector<std::string>& strings );
  static bool parse( sim_t*, const char* context, const option_t*,        const std::string& options_str );
  static bool parse_file( sim_t*, FILE* file );
  static bool parse_line( sim_t*, const char* line );
  static bool parse_token( sim_t*, const std::string& token );
  static option_t* merge( std::vector<option_t>& out, const option_t* in1, const option_t* in2 );
};

inline option_t opt_string( const char* n, std::string& v )
{ return option_t( n, option_t::STRING, &v ); }

inline option_t opt_append( const char* n, std::string& v )
{ return option_t( n, option_t::APPEND, &v ); }

inline option_t opt_bool( const char* n, int& v )
{ return option_t( n, option_t::INT_BOOL, &v ); }

inline option_t opt_bool( const char* n, bool& v )
{ return option_t( n, option_t::BOOL, &v ); }

inline option_t opt_int( const char* n, int& v )
{ return option_t( n, option_t::INT, &v ); }

inline option_t opt_uint( const char* n, unsigned& v )
{ return option_t( n, option_t::UINT, &v ); }

inline option_t opt_float( const char* n, double& v )
{ return option_t( n, option_t::FLT, &v ); }

inline option_t opt_timespan( const char* n, timespan_t& v )
{ return option_t( n, option_t::TIMESPAN, &v ); }

inline option_t opt_list( const char* n, option_t::list_t& v )
{ return option_t( n, option_t::LIST, &v ); }

inline option_t opt_map( const char* n, option_t::map_t& v )
{ return option_t( n, option_t::MAP, &v ); }

inline option_t opt_func( const char* name, option_t::function_t& f )
{ return option_t( name, f ); }

inline option_t opt_deprecated( const char* n, const char* r )
{ return option_t( n, r ); }

inline option_t opt_null()
{ return option_t( 0, option_t::NONE, ( void* )0 ); }

// Talent Translation =======================================================

#ifndef MAX_TALENT_ROWS
#define MAX_TALENT_ROWS ( 6 )
#endif

#ifndef MAX_TALENT_COLS
#define MAX_TALENT_COLS ( 3 )
#endif

#ifndef MAX_TALENT_SLOTS
#define MAX_TALENT_SLOTS ( MAX_TALENT_ROWS * MAX_TALENT_COLS )
#endif


// Utilities ================================================================

#ifdef _MSC_VER
// C99-compliant snprintf - MSVC _snprintf is NOT the same.

#undef vsnprintf
int vsnprintf_simc( char* buf, size_t size, const char* fmt, va_list ap );
#define vsnprintf vsnprintf_simc

#undef snprintf
inline int snprintf( char* buf, size_t size, const char* fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  int rval = vsnprintf( buf, size, fmt, ap );
  va_end( ap );
  return rval;
}
#endif

enum stopwatch_e { STOPWATCH_CPU, STOPWATCH_WALL, STOPWATCH_THREAD };

struct stopwatch_t
{
  stopwatch_e type;
  int64_t start_sec, sec;
  int64_t start_usec, usec;
  void now( int64_t* now_sec, int64_t* now_usec );
  void mark() { now( &start_sec, &start_usec ); }
  void accumulate();
  double current();
  double elapsed();
  stopwatch_t( stopwatch_e t = STOPWATCH_CPU ) : type( t ) { mark(); }
};

namespace util
{
double wall_time();

template <typename T>
T ability_rank( int player_level, T ability_value, int ability_level, ... );
double interpolate( int level, double val_60, double val_70, double val_80, double val_85 = -1 );

const char* attribute_type_string     ( attribute_e type );
const char* dot_behavior_type_string  ( dot_behavior_e t );
const char* flask_type_string         ( flask_e type );
const char* food_type_string          ( food_e type );
const char* gem_type_string           ( gem_e type );
const char* meta_gem_type_string      ( meta_gem_e type );
const char* player_type_string        ( player_e );
const char* pet_type_string           ( pet_e type );
const char* position_type_string      ( position_e );
const char* profession_type_string    ( profession_e );
const char* race_type_string          ( race_e );
const char* stats_type_string         ( stats_e );
const char* role_type_string          ( role_e );
const char* resource_type_string      ( resource_e );
const char* result_type_string        ( result_e type );
const char* amount_type_string        ( dmg_e type );
uint32_t    school_type_component     ( school_e s_type, school_e c_type );
const char* school_type_string        ( school_e type );
const char* armor_type_string         ( int type );
const char* armor_type_string         ( item_subclass_armor type );
const char* set_bonus_string          ( set_e type );
const char* cache_type_string         ( cache_e type );

bool is_match_slot( slot_e slot );
item_subclass_armor matching_armor_type ( player_e ptype );

const char* slot_type_string          ( slot_e type );
const char* stat_type_string          ( stat_e type );
const char* stat_type_abbrev          ( stat_e type );
const char* stat_type_wowhead         ( stat_e type );
const char* stat_type_gem             ( stat_e type );
resource_e translate_power_type  ( power_e );
const char* weapon_type_string        ( weapon_e type );
const char* weapon_class_string       ( int class_ );
const char* weapon_subclass_string    ( int subclass );

const char* set_item_type_string      ( int item_set );
const char* item_quality_string       ( int item_quality );

attribute_e parse_attribute_type ( const std::string& name );
dmg_e parse_dmg_type             ( const std::string& name );
flask_e parse_flask_type         ( const std::string& name );
food_e parse_food_type           ( const std::string& name );
gem_e parse_gem_type             ( const std::string& name );
meta_gem_e parse_meta_gem_type   ( const std::string& name );
player_e parse_player_type       ( const std::string& name );
pet_e parse_pet_type             ( const std::string& name );
profession_e parse_profession_type( const std::string& name );
position_e parse_position_type   ( const std::string& name );
race_e parse_race_type           ( const std::string& name );
role_e parse_role_type           ( const std::string& name );
resource_e parse_resource_type   ( const std::string& name );
result_e parse_result_type       ( const std::string& name );
school_e parse_school_type       ( const std::string& name );
set_e parse_set_bonus            ( const std::string& name );
slot_e parse_slot_type           ( const std::string& name );
stat_e parse_stat_type           ( const std::string& name );
stat_e parse_reforge_type        ( const std::string& name );
stat_e parse_gem_stat            ( const std::string& name );

item_subclass_armor parse_armor_type( const std::string& name );
weapon_e parse_weapon_type       ( const std::string& name );

int parse_item_quality                ( const std::string& quality );

bool parse_origin( std::string& region, std::string& server, std::string& name, const std::string& origin );

int class_id_mask( player_e type );
int class_id( player_e type );
unsigned race_mask( race_e type );
unsigned race_id( race_e type );
unsigned pet_mask( pet_e type );
unsigned pet_id( pet_e type );
player_e pet_class_type( pet_e type );

const char* class_id_string( player_e type );
player_e translate_class_id( int cid );
player_e translate_class_str( std::string& s );
race_e translate_race_id( int rid );
stat_e translate_item_mod( int stat_mod );
int translate_stat( stat_e stat );
stat_e translate_rating_mod( unsigned ratings );
slot_e translate_invtype( inventory_type inv_type );
weapon_e translate_weapon_subclass( int weapon_subclass );
item_subclass_weapon translate_weapon( weapon_e weapon );
profession_e translate_profession_id( int skill_id );
gem_e translate_socket_color( item_socket_color );

bool socket_gem_match( gem_e socket, unsigned gem );
double crit_multiplier( meta_gem_e gem );
double stat_itemization_weight( stat_e s );
std::vector<std::string> string_split( const std::string& str, const std::string& delim );
size_t string_split_allow_quotes( std::vector<std::string>& results, const std::string& str, const char* delim );
size_t string_split( const std::string& str, const char* delim, const char* format, ... );
void string_strip_quotes( std::string& str );
std::string& replace_all( std::string& s, const std::string&, const std::string& );
std::string& erase_all( std::string& s, const std::string& from );

template <typename T>
std::string to_string( const T& t )
{ std::stringstream s; s << t; return s.str(); }

std::string to_string( double f );
std::string to_string( double f, int precision );

unsigned to_unsigned( const std::string& str );
unsigned to_unsigned( const char* str );
int to_int( const std::string& str );
int to_int( const char* str );

int64_t milliseconds();
int64_t parse_date( const std::string& month_day_year );

int printf( const char *format, ... ) PRINTF_ATTRIBUTE( 1, 2 );
int fprintf( FILE *stream, const char *format, ... ) PRINTF_ATTRIBUTE( 2, 3 );
int vfprintf( FILE *stream, const char *format, va_list fmtargs ) PRINTF_ATTRIBUTE( 2, 0 );
int vprintf( const char *format, va_list fmtargs ) PRINTF_ATTRIBUTE( 1, 0 );

std::string encode_html( const std::string& );
std::string decode_html( const std::string& str );
std::string& urlencode( std::string& str );
std::string& urldecode( std::string& str );
std::string uchar_to_hex( unsigned char );

bool str_compare_ci( const std::string& l, const std::string& r );
std::string& glyph_name( std::string& n );
bool str_in_str_ci ( const std::string& l, const std::string& r );
bool str_prefix_ci ( const std::string& str, const std::string& prefix );

double floor( double X, unsigned int decplaces = 0 );
double ceil( double X, unsigned int decplaces = 0 );
double round( double X, unsigned int decplaces = 0 );

std::string& tolower( std::string& str );

void tokenize( std::string& name );
std::string inverse_tokenize( const std::string& name );

bool is_number( const std::string& s );

int snprintf( char* buf, size_t size, const char* fmt, ... ) PRINTF_ATTRIBUTE( 3, 4 );
void fuzzy_stats( std::string& encoding, const std::string& description );

template <class T>
int numDigits( T number );

template <typename T>
T str_to_num( const std::string& );

bool contains_non_ascii( const std::string& );

} // namespace util


// Data Access ==============================================================
#ifndef MAX_LEVEL
#define MAX_LEVEL (90)
#endif

// Include DBC Module
#include "dbc/dbc.hpp"

namespace io {
// Converts a wide (UTF-16 or UTF-32) string to narrow (UTF-8)
std::string narrow( const wchar_t* wstr );
inline std::string narrow( const std::wstring& wstr )
{ return narrow( wstr.c_str() ); }

// Converts a narrow (UTF-8) string to wide (UTF-16 or UTF-32)
std::wstring widen( const char* str );
std::wstring widen( const std::string& str );

std::string latin1_to_utf8( const std::string& str );
std::string utf8_to_latin1( const std::string& str );
std::string maybe_latin1_to_utf8( const std::string& str );

// Like std::fopen, but works with UTF-8 filenames on windows.
FILE* fopen( const std::string& filename, const char* mode );

// RAII wrapper for FILE*.
class cfile
{
  std::shared_ptr<FILE> file;

  static void dont_close( FILE* ) {}
  static void safe_close( FILE* f )
  { if ( f ) std::fclose( f ); }

public:
  struct no_close {};

  cfile() {} // = default;
  cfile( const std::string& filename, const char* mode ) :
    file( fopen( filename, mode ), safe_close ) {}
  explicit cfile( FILE* f ) : file( f, safe_close ) {}
  cfile( FILE* f, no_close ) : file( f, dont_close ) {}

  operator FILE* () const { return file.get(); }

  // Only closes if this is the last reference to the file.
  void close() { file.reset(); }
};

class ofstream : public std::ofstream
{
public:
  ofstream& printf( const char* format, ... );
  void open( sim_t* sim, const std::string& filename, openmode mode = out | trunc );
  void open( const char* filename, openmode mode = out | trunc );
  void open( const std::string& filename, openmode mode = out | trunc )
  { return open( filename.c_str(), mode ); }
};

class ifstream : public std::ifstream
{
public:
  void open( sim_t* sim, const std::string& filename, openmode mode = in );
  void open( const char* filename, openmode mode = in );
  void open( const std::string& filename, openmode mode = in )
  { return open( filename.c_str(), mode ); }
};

class utf8_args : public std::vector<std::string>
{
public:
  utf8_args( int argc, char** argv );
};

#ifndef SC_WINDOWS
inline FILE* fopen( const std::string& filename, const char* mode )
{ return std::fopen( filename.c_str(), mode ); }
#endif

inline int fclose( cfile& file ) { file.close(); return 0; }
} // namespace io

// Spell information struct, holding static functions to output spell data in a human readable form

namespace spell_info
{
std::string to_str( sim_t* sim, const spell_data_t* spell, int level = MAX_LEVEL );
void        to_xml( sim_t* sim, const spell_data_t* spell, xml_node_t* parent, int level = MAX_LEVEL );
//static std::string to_str( sim_t* sim, uint32_t spell_id, int level = MAX_LEVEL );
std::string talent_to_str( sim_t* sim, const talent_data_t* talent, int level = MAX_LEVEL );
void        talent_to_xml( sim_t* sim, const talent_data_t* talent, xml_node_t* parent, int level = MAX_LEVEL );
std::ostringstream& effect_to_str( sim_t* sim, const spell_data_t* spell, const spelleffect_data_t* effect, std::ostringstream& s, int level = MAX_LEVEL );
void                effect_to_xml( sim_t* sim, const spell_data_t* spell, const spelleffect_data_t* effect, xml_node_t*    parent, int level = MAX_LEVEL );
}


// Raid Event

struct raid_event_t
{
protected:
  sim_t* sim;
  std::string name_str;
  int64_t num_starts;
  timespan_t first, last;
  timespan_t cooldown;
  timespan_t cooldown_stddev;
  timespan_t cooldown_min;
  timespan_t cooldown_max;
  timespan_t duration;
  timespan_t duration_stddev;
  timespan_t duration_min;
  timespan_t duration_max;

  // Player filter options
  double     distance_min; // Minimal player distance
  double     distance_max; // Maximal player distance
  bool players_only; // Don't affect pets
  double player_chance; // Chance for individual player to be affected by raid event

  std::string affected_role_str;
  role_e     affected_role;

  timespan_t saved_duration;
  rng_t* rng;
  std::vector<player_t*> affected_players;

  raid_event_t( sim_t*, const std::string& );
private:
  virtual void _start() = 0;
  virtual void _finish() = 0;
public:
  virtual ~raid_event_t() {}

  virtual bool filter_player( const player_t* );

  timespan_t cooldown_time();
  timespan_t duration_time();
  void schedule();
  void reset();
  void start();
  void finish();
  void parse_options( option_t*, const std::string& options_str );
  static raid_event_t* create( sim_t* sim, const std::string& name, const std::string& options_str );
  static void init( sim_t* );
  static void reset( sim_t* );
  static void combat_begin( sim_t* );
  static void combat_end( sim_t* ) {}
  const char* name() const { return name_str.c_str(); }
};

// Gear Stats ===============================================================

struct gear_stats_t
{
  std::array<double, ATTRIBUTE_MAX> attribute;
  std::array<double, RESOURCE_MAX> resource;
  double spell_power;
  double attack_power;
  double expertise_rating;
  double expertise_rating2;
  double hit_rating;
  double hit_rating2;
  double crit_rating;
  double haste_rating;
  double weapon_dps;
  double weapon_speed;
  double weapon_offhand_dps;
  double weapon_offhand_speed;
  double armor;
  double dodge_rating;
  double parry_rating;
  double block_rating;
  double mastery_rating;
  double resilience_rating;
  double pvp_power;

  gear_stats_t() :
    attribute(), resource(),
    spell_power( 0.0 ), attack_power( 0.0 ), expertise_rating( 0.0 ), expertise_rating2( 0.0 ),
    hit_rating( 0.0 ), hit_rating2( 0.0 ), crit_rating( 0.0 ), haste_rating( 0.0 ), weapon_dps( 0.0 ), weapon_speed( 0.0 ),
    weapon_offhand_dps( 0.0 ), weapon_offhand_speed( 0.0 ), armor( 0.0 ), dodge_rating( 0.0 ),
    parry_rating( 0.0 ), block_rating( 0.0 ), mastery_rating( 0.0 ), resilience_rating( 0.0 ), pvp_power( 0.0 )
  { }

  friend gear_stats_t operator+( const gear_stats_t& left, const gear_stats_t& right )
  {
    gear_stats_t a = gear_stats_t( left );
    a += right;
    return a;
  }

  gear_stats_t& operator+=( const gear_stats_t& right )
  {
    spell_power += right.spell_power;
    attack_power += right.attack_power;
    expertise_rating += right.expertise_rating;
    expertise_rating2 += right.expertise_rating2;
    hit_rating += right.hit_rating;
    hit_rating2 += right.hit_rating2;
    crit_rating += right.crit_rating;
    haste_rating += right.haste_rating;
    weapon_dps += right.weapon_dps;
    weapon_speed += right.weapon_speed;
    weapon_offhand_dps += right.weapon_offhand_dps;
    weapon_offhand_speed += right.weapon_offhand_speed;
    armor += right.armor;
    dodge_rating += right.dodge_rating;
    parry_rating += right.parry_rating;
    block_rating += right.block_rating;
    mastery_rating += right.mastery_rating;
    resilience_rating += right.resilience_rating;
    pvp_power += right.pvp_power;
    range::transform ( attribute, right.attribute, attribute.begin(), std::plus<int>() );
    range::transform ( resource, right.resource, resource.begin(), std::plus<int>() );
    return *this;
  }

  void   add_stat( stat_e stat, double value );
  void   set_stat( stat_e stat, double value );
  double get_stat( stat_e stat ) const;
  std::string to_string();
  static double stat_mod( stat_e stat );
};

// Actor Pair ===============================================================

struct actor_pair_t
{
  player_t* target;
  player_t* source;

  actor_pair_t( player_t* target, player_t* source )
    : target( target ), source( source )
  {}

  actor_pair_t( player_t* p = 0 )
    : target( p ), source( p )
  {}

  virtual ~actor_pair_t() {}
};

// Uptime ==================================================================

struct uptime_common_t
{
private:
  timespan_t last_start;
  timespan_t iteration_uptime_sum;
public:
  simple_sample_data_t uptime_sum;

  uptime_common_t() :
    last_start( timespan_t::min() ),
    iteration_uptime_sum( timespan_t::zero() ),
    uptime_sum()
  { }
  void update( bool is_up, timespan_t current_time )
  {
    if ( is_up )
    {
      if ( last_start < timespan_t::zero() )
        last_start = current_time;
    }
    else if ( last_start >= timespan_t::zero() )
    {
      iteration_uptime_sum += current_time - last_start;
      reset();
    }
  }
  void datacollection_begin()
  { iteration_uptime_sum = timespan_t::zero(); }
  void datacollection_end( timespan_t t )
  { uptime_sum.add( t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0 ); }
  void reset() { last_start = timespan_t::min(); }
  void merge( const uptime_common_t& other )
  { uptime_sum.merge( other.uptime_sum ); }
};

struct uptime_t : public uptime_common_t
{
  std::string name_str;

  uptime_t( const std::string& n ) :
    uptime_common_t(), name_str( n )
  {}

  const char* name() const
  { return name_str.c_str(); }
};

struct buff_uptime_t : public uptime_common_t
{
  buff_uptime_t() :
    uptime_common_t() {}
};

// Buff Creation ====================================================================
namespace buff_creation {

// This is the base buff creator class containing data to create a buff_t
struct buff_creator_basics_t
{
protected:
  actor_pair_t _player;
  sim_t* _sim;
  std::string _name;
  const spell_data_t* s_data;
  double _chance;
  double _default_value;
  int _max_stack;
  timespan_t _duration, _cooldown;
  int _quiet, _reverse, _activated;
  std::vector<cache_e> _invalidate_list;
  friend struct ::buff_t;
  friend struct ::debuff_t;
private:
  void init();
public:
  buff_creator_basics_t( actor_pair_t, const std::string& name, const spell_data_t* = spell_data_t::nil() );
  buff_creator_basics_t( actor_pair_t, uint32_t id, const std::string& name );
  buff_creator_basics_t( sim_t*, const std::string& name, const spell_data_t* = spell_data_t::nil() );
};

// This helper template is necessary so that reference functions of the classes inheriting from it return the type of the derived class.
// eg. buff_creator_helper_t<stat_buff_creator_t>::chance() will return a reference of type stat_buff_creator_t
template <typename T>
struct buff_creator_helper_t : public buff_creator_basics_t
{
  typedef T bufftype;
  typedef buff_creator_helper_t base_t;

public:
  buff_creator_helper_t( actor_pair_t q, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    buff_creator_basics_t( q, name, s ) {}
  buff_creator_helper_t( actor_pair_t q, uint32_t id, const std::string& name ) :
    buff_creator_basics_t( q, id, name ) {}
  buff_creator_helper_t( sim_t* sim, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    buff_creator_basics_t( sim, name, s ) {}

  bufftype& duration( timespan_t d )
  { _duration = d; return *( static_cast<bufftype*>( this ) ); }
  bufftype& default_value( double v )
  { _default_value = v; return *( static_cast<bufftype*>( this ) ); }
  bufftype& chance( double c )
  { _chance = c; return *( static_cast<bufftype*>( this ) ); }
  bufftype& max_stack( unsigned ms )
  { _max_stack = ms; return *( static_cast<bufftype*>( this ) ); }
  bufftype& cd( timespan_t t )
  { _cooldown = t; return *( static_cast<bufftype*>( this ) ); }
  bufftype& reverse( bool r )
  { _reverse = r; return *( static_cast<bufftype*>( this ) ); }
  bufftype& quiet( bool q )
  { _quiet = q; return *( static_cast<bufftype*>( this ) ); }
  bufftype& activated( bool a )
  { _activated = a; return *( static_cast<bufftype*>( this ) ); }
  bufftype& spell( const spell_data_t* s )
  { s_data = s; return *( static_cast<bufftype*>( this ) ); }
  bufftype& add_invalidate( cache_e c )
  { _invalidate_list.push_back( c ); return *( static_cast<bufftype*>( this ) ); }
};

struct buff_creator_t : public buff_creator_helper_t<buff_creator_t>
{
public:
  buff_creator_t( actor_pair_t q, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( q, name, s ) {}
  buff_creator_t( actor_pair_t q, uint32_t id, const std::string& name ) :
    base_t( q, id, name ) {}
  buff_creator_t( sim_t* sim, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( sim, name, s ) {}

  operator buff_t* () const;
  operator debuff_t* () const;
};

struct stat_buff_creator_t : public buff_creator_helper_t<stat_buff_creator_t>
{
private:

  struct buff_stat_t
  {
    stat_e stat;
    double amount;
    bool ( *check_func )( void* p );
    void *data;

    buff_stat_t( stat_e s, double a, bool ( *c )( void* ) = 0, void *d = 0 ) :
      stat( s ), amount( a ), check_func( c ), data( d ) {}
  };

  std::vector<buff_stat_t> stats;

  friend struct ::stat_buff_t;
public:
  stat_buff_creator_t( actor_pair_t q, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( q, name, s ) {}
  stat_buff_creator_t( sim_t* sim, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( sim, name, s ) {}

  bufftype& add_stat( stat_e s, double a, bool ( *c )( void* ) = 0, void *d = 0 )
  { stats.push_back( buff_stat_t( s, a, c, d ) ); return *this; }

  operator stat_buff_t* () const;
};

struct absorb_buff_creator_t : public buff_creator_helper_t<absorb_buff_creator_t>
{
private:
  school_e _absorb_school;
  stats_t* _absorb_source;
  friend struct ::absorb_buff_t;
public:
  absorb_buff_creator_t( actor_pair_t q, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( q, name, s ),
    _absorb_school( SCHOOL_CHAOS ),
    _absorb_source( 0 ) {}
  absorb_buff_creator_t( sim_t* sim, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( sim, name, s ),
    _absorb_school( SCHOOL_CHAOS ),
    _absorb_source( 0 ) {}

  bufftype& source( stats_t* s )
  { _absorb_source = s; return *this; }

  bufftype& school( school_e school )
  { _absorb_school = school; return *this; }

  operator absorb_buff_t* () const;
};

struct cost_reduction_buff_creator_t : public buff_creator_helper_t<cost_reduction_buff_creator_t>
{
private:
  double _amount;
  school_e _school;
  bool _refreshes;
  friend struct ::cost_reduction_buff_t;
public:
  cost_reduction_buff_creator_t( actor_pair_t q, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( q, name, s ),
    _amount( 0 ), _school( SCHOOL_NONE ), _refreshes( false )
  {}
  cost_reduction_buff_creator_t( sim_t* sim, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( sim, name, s ),
    _amount( 0 ), _school( SCHOOL_NONE ), _refreshes( false )
  {}

  bufftype& amount( double a )
  { _amount = a; return *this; }
  bufftype& school( school_e s )
  { _school = s; return *this; }
  bufftype& refreshes( bool r )
  { _refreshes = r; return *this; }

  operator cost_reduction_buff_t* () const;
};

struct haste_buff_creator_t : public buff_creator_helper_t<haste_buff_creator_t>
{
private:
  friend struct ::haste_buff_t;
public:
  haste_buff_creator_t( actor_pair_t q, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( q, name, s )
  { }

  operator haste_buff_t* () const;
};

struct tick_buff_creator_t : public buff_creator_helper_t<tick_buff_creator_t>
{
private:
  timespan_t _period;

  friend struct ::tick_buff_t;
public:
  tick_buff_creator_t( actor_pair_t q, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( q, name, s ), _period( timespan_t::min() )
  { }

  bufftype& period( timespan_t p )
  { _period = p; return *this; }

  operator tick_buff_t* () const;
};

} // END NAMESPACE buff_creation

using namespace buff_creation;

// Buffs ====================================================================

struct buff_t : public noncopyable
{
public:
  sim_t* const sim;
  player_t* const player;
  const std::string name_str;
  const spell_data_t* s_data;
  player_t* const source;
  event_t* expiration;
  event_t* delay;
  rng_t* rng;
  cooldown_t* cooldown;
  std::vector<cache_e> invalidate_list;
  sc_timeline_t uptime_array;

  // static values
private: // private because changing max_stacks requires resizing some stack-dependant vectors
  int _max_stack;
public:
  double default_value;
  bool activated, reactable;
  bool reverse, constant, quiet, overridden;
  bool requires_invalidation;

  // dynamic values
  double current_value;
  int current_stack;
  timespan_t buff_duration;
  double default_chance;
  std::vector<timespan_t> stack_occurrence, stack_react_time;
  std::vector<event_t*> stack_react_ready_triggers;

  // tmp data collection
protected:
  timespan_t last_start;
  timespan_t last_trigger;
  timespan_t iteration_uptime_sum;
  unsigned int up_count, down_count, start_count, refresh_count;
  int trigger_attempts, trigger_successes;
  int simulation_max_stack;

  // report data
public:
  simple_sample_data_t benefit_pct, trigger_pct;
  simple_sample_data_t avg_start, avg_refresh;
  simple_sample_data_t uptime_pct, start_intervals, trigger_intervals;
  auto_dispose< std::vector<buff_uptime_t*> > stack_uptime;

  virtual ~buff_t() {}

protected:
  buff_t( const buff_creator_basics_t& params );
  friend struct buff_creation::buff_creator_t;
public:
  const spell_data_t& data() const { return *s_data; }

  // Use check() inside of ready() and cost() methods to prevent skewing of "benefit" calculations.
  // Use up() where the presence of the buff affects the action mechanics.
  int             check() { return current_stack; }
  inline bool     up()    { if ( current_stack > 0 ) { up_count++; } else { down_count++; } return current_stack > 0; }
  inline int      stack() { if ( current_stack > 0 ) { up_count++; } else { down_count++; } return current_stack; }
  inline double   value() { if ( current_stack > 0 ) { up_count++; } else { down_count++; } return current_value; }
  timespan_t remains() const;
  bool   remains_gt( timespan_t time );
  bool   remains_lt( timespan_t time );
  bool   trigger  ( action_t*, int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual bool   trigger  ( int stacks = 1, double value = DEFAULT_VALUE(), double chance = -1.0, timespan_t duration = timespan_t::min() );
  virtual void   execute ( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void   increment( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  void   decrement( int stacks = 1, double value = DEFAULT_VALUE() );
  void   extend_duration( player_t* p, timespan_t seconds );

  virtual void start    ( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void refresh  ( int stacks = 0, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void bump     ( int stacks = 1, double value = DEFAULT_VALUE() );
  virtual void override ( int stacks = 1, double value = DEFAULT_VALUE() );
  virtual bool may_react( int stacks = 1 );
  virtual int stack_react();
  void expire();

  // Called only if previously active buff expires
  virtual void expire_override() {}
  virtual void predict();
  virtual void reset();
  virtual void aura_gain();
  virtual void aura_loss();
  virtual void merge( const buff_t& other_buff );
  virtual void analyze();
  virtual void datacollection_begin();
  virtual void datacollection_end();

#ifdef SC_STAT_CACHE
  virtual void invalidate_cache();
#else
  void invalidate_cache() {}
#endif

  virtual int total_stack();

  static expr_t* create_expression( std::string buff_name,
                                    action_t* action,
                                    const std::string& type,
                                    buff_t* static_buff = 0 );
  std::string to_str() const;

  static double DEFAULT_VALUE() { return std::numeric_limits< double >::min(); }
  static buff_t* find( const std::vector<buff_t*>&, const std::string& name, player_t* source = 0 );
  static buff_t* find(    sim_t*, const std::string& name );
  static buff_t* find( player_t*, const std::string& name, player_t* source = 0 );

  const char* name() const { return name_str.c_str(); }
  std::string source_name() const;
  int max_stack() const { return _max_stack; }
};

struct stat_buff_t : public buff_t
{
  struct buff_stat_t
  {
    stat_e stat;
    double amount;
    double current_value;
    bool ( *check_func )( void* a );
    void* data;

    buff_stat_t( stat_e s, double a, bool ( *c )( void* a ) = 0, void *d = 0 ) :
      stat( s ), amount( a ), current_value( 0 ), check_func( c ), data( d ) {}
  };
  std::vector<buff_stat_t> stats;

  virtual void bump     ( int stacks = 1, double value = -1.0 );
  virtual void decrement( int stacks = 1, double value = -1.0 );
  virtual void expire_override();

protected:
  stat_buff_t( const stat_buff_creator_t& params );
  friend struct buff_creation::stat_buff_creator_t;
};

struct absorb_buff_t : public buff_t
{
  school_e absorb_school;
  stats_t* absorb_source;

protected:
  absorb_buff_t( const absorb_buff_creator_t& params );
  friend struct buff_creation::absorb_buff_creator_t;

  // Hook for derived classes to recieve notification when some of the absorb is consumed.
  // Called after the adjustment to current_value.
  virtual void absorb_used( double /* amount */ ) {}

public:
  virtual void start( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void expire_override();

  void consume( double amount );
};

struct cost_reduction_buff_t : public buff_t
{
  double amount;
  school_e school;
  bool refreshes;

protected:
  cost_reduction_buff_t( const cost_reduction_buff_creator_t& params );
  friend struct buff_creation::cost_reduction_buff_creator_t;
public:
  virtual void bump     ( int stacks = 1, double value = -1.0 );
  virtual void decrement( int stacks = 1, double value = -1.0 );
  virtual void expire_override();
  virtual void refresh  ( int stacks = 0, double value = -1.0, timespan_t duration = timespan_t::min() );
};

struct haste_buff_t : public buff_t
{
protected:
  haste_buff_t( const haste_buff_creator_t& params );
  friend struct buff_creation::haste_buff_creator_t;
public:
  virtual void execute( int stacks = 1, double value = -1.0, timespan_t duration = timespan_t::min() );
  virtual void expire_override();
};

struct tick_buff_t : public buff_t
{
  timespan_t period;

protected:
  tick_buff_t( const tick_buff_creator_t& params );
  friend struct buff_creation::tick_buff_creator_t;
public:
  virtual bool trigger( int stacks = 1, double value = -1.0, double chance = -1.0, timespan_t duration = timespan_t::min() );
};

struct debuff_t : public buff_t
{
protected:
  debuff_t( const buff_creator_basics_t& params );
  friend struct buff_creation::buff_creator_t;
};

typedef struct buff_t aura_t;

struct stormlash_buff_t : public buff_t
{
  action_t*             stormlash_aggregate;
  stormlash_callback_t* stormlash_cb;

  stormlash_buff_t( player_t* p, const spell_data_t* s );

  virtual void execute( int stacks = 1, double value = -1.0, timespan_t duration = timespan_t::min() );
  virtual void expire_override();
};

// Expressions ==============================================================

enum token_e
{
  TOK_UNKNOWN = 0,
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
  TOK_SPELL_LIST,
  TOK_FLOOR,
  TOK_CEIL
};

struct expr_token_t
{
  token_e type;
  std::string label;
};

struct expression_t
{
  static int precedence( token_e );
  static bool is_unary( token_e );
  static bool is_binary( token_e );
  static token_e next_token( action_t* action, const std::string& expr_str, int& current_index,
                             std::string& token_str, token_e prev_token );
  static std::vector<expr_token_t> parse_tokens( action_t* action, const std::string& expr_str );
  static void print_tokens( std::vector<expr_token_t>& tokens, sim_t* sim );
  static void convert_to_unary( std::vector<expr_token_t>& tokens );
  static bool convert_to_rpn( std::vector<expr_token_t>& tokens );
};

// Action expression types ==================================================

class expr_t
{
  std::string name_;

protected:
  template <typename T> static double coerce( T t ) { return static_cast<double>( t ); }
  static double coerce( timespan_t t ) { return t.total_seconds(); }

  virtual double evaluate() = 0;

public:
  expr_t( const std::string& name ) : name_( name ) {}
  virtual ~expr_t() {}

  const std::string& name() { return name_; }

  double eval() { return evaluate(); }
  bool success() { return eval() != 0; }

  static expr_t* parse( action_t*, const std::string& expr_str );
};

// Reference Expression - ref_expr_t
// Class Template to create a expression with a reference ( ref ) of arbitrary type T, and evaluate that reference
template <typename T>
class ref_expr_t : public expr_t
{
  const T& t;
  virtual double evaluate() { return coerce( t ); }

public:
  ref_expr_t( const std::string& name, const T& t_ ) : expr_t( name ), t( t_ ) {}
};

// Template to return a reference expression
template <typename T>
inline expr_t* make_ref_expr( const std::string& name, const T& t )
{ return new ref_expr_t<T>( name, t ); }

// Function Expression - fn_expr_t
// Class Template to create a function ( fn ) expression with arbitrary functor f, which gets evaluated
template <typename F>
class fn_expr_t : public expr_t
{
  F f;

  virtual double evaluate() { return coerce( f() ); }

public:
  fn_expr_t( const std::string& name, F f_ ) :
    expr_t( name ), f( f_ ) {}
};

// Template to return a function expression
template <typename F>
inline expr_t* make_fn_expr( const std::string& name, F f )
{ return new fn_expr_t<F>( name, f ); }

// Make member function expression - make_mem_fn_expr
// Template to return function expression that calls a member ( mem ) function ( fn ) f on reference t.
template <typename F, typename T>
inline expr_t* make_mem_fn_expr( const std::string& name, T& t, F f )
{ return make_fn_expr( name, std::bind( std::mem_fn( f ), &t ) ); }


// Spell query expression types =============================================

enum expr_data_e
{
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

struct spell_data_expr_t
{
  std::string name_str;
  sim_t* sim;
  expr_data_e data_type;
  bool effect_query;

  token_e result_tok;
  double result_num;
  std::vector<uint32_t> result_spell_list;
  std::string result_str;

  spell_data_expr_t( sim_t* sim, const std::string& n, expr_data_e dt = DATA_SPELL, bool eq = false, token_e t = TOK_UNKNOWN )
    : name_str( n ), sim( sim ), data_type( dt ),         effect_query( eq ),  result_tok( t ),            result_num( 0 ),            result_spell_list(),               result_str( "" ) {}
  spell_data_expr_t( sim_t* sim, const std::string& n, double       constant_value )
    : name_str( n ), sim( sim ), data_type( DATA_SPELL ), effect_query( false ), result_tok( TOK_NUM ),        result_num( constant_value ), result_spell_list(),               result_str( "" ) {}
  spell_data_expr_t( sim_t* sim, const std::string& n, const std::string& constant_value )
    : name_str( n ), sim( sim ), data_type( DATA_SPELL ), effect_query( false ), result_tok( TOK_STR ),        result_num( 0.0 ),            result_spell_list(),               result_str( constant_value ) {}
  spell_data_expr_t( sim_t* sim, const std::string& n, const std::vector<uint32_t>& constant_value )
    : name_str( n ), sim( sim ), data_type( DATA_SPELL ), effect_query( false ), result_tok( TOK_SPELL_LIST ), result_num( 0.0 ),            result_spell_list( constant_value ), result_str( "" ) {}
  virtual ~spell_data_expr_t() {}
  virtual int evaluate() { return result_tok; }
  const char* name() { return name_str.c_str(); }

  virtual std::vector<uint32_t> operator|( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator&( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator-( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> operator<( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator<=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> in( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> not_in( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  static spell_data_expr_t* parse( sim_t* sim, const std::string& expr_str );
  static spell_data_expr_t* create_spell_expression( sim_t* sim, const std::string& name_str );
};

class mutex_t : public noncopyable
{
private:
  class native_t;
  native_t* native_handle;

public:
  mutex_t();
  ~mutex_t();

  void lock();
  void unlock();
};

class sc_thread_t : public noncopyable
{
private:
  class native_t;
  native_t* native_handle;

protected:
  sc_thread_t();
  virtual ~sc_thread_t();
public:
  virtual void run() = 0;

  void launch();
  void wait();

  static void sleep( timespan_t );
};

class auto_lock_t
{
private:
  mutex_t& mutex;
public:
  auto_lock_t( mutex_t& mutex_ ) : mutex( mutex_ ) { mutex.lock(); }
  ~auto_lock_t() { mutex.unlock(); }
};

// Simulation Setup =========================================================

struct option_tuple_t
{
  std::string scope, name, value;
  option_tuple_t( const std::string& s, const std::string& n, const std::string& v ) : scope( s ), name( n ), value( v ) {}
};

struct option_db_t : public std::vector<option_tuple_t>
{
  std::string auto_path;
  std::unordered_map<std::string, std::string> var_map;

  option_db_t();
  void add( const std::string& scope, const std::string& name, const std::string& value )
  {
    push_back( option_tuple_t( scope, name, value ) );
  }
  FILE* open_file( const std::string& name );
  bool parse_file( FILE* file );
  bool parse_token( const std::string& token );
  bool parse_line( const std::string& line );
  bool parse_text( const std::string& text );
  bool parse_args( const std::vector<std::string>& args );
};

struct player_description_t
{
  // Add just enough to describe a player

  // name, class, talents, glyphs, gear, professions, actions explicitly stored
  std::string name;
  // etc

  // flesh out API, these functions cannot depend upon sim_t
  // ideally they remain static, but if not then move to sim_control_t
  static void load_bcp    ( player_description_t& /*etc*/ );
  static void load_wowhead( player_description_t& /*etc*/ );
  static void load_chardev( player_description_t& /*etc*/ );
  static void load_rawr   ( player_description_t& /*etc*/ );
};

struct combat_description_t
{
  std::string name;
  int target_seconds;
  std::string raid_events;
  // etc
};

struct sim_control_t
{
  combat_description_t combat;
  std::vector<player_description_t> players;
  option_db_t options;
};

// Progress Bar =============================================================

struct progress_bar_t
{
  sim_t& sim;
  int steps, updates, interval;
  double start_time;
  std::string status;

  progress_bar_t( sim_t& s );
  void init();
  bool update( bool finished = false );
};

/* The simplest form of a callback
 * pure virtual class which requires one to override execute function
 * static trigger for a vector of callbacks to trigger them.
 *
 * NO AUTMATED MEMORY MANAGMENT!
 */
struct callback_t
{
  virtual void execute() = 0;
  static void trigger( const std::vector<callback_t*>& v )
  {
    for( size_t i = 0, size = v.size(); i < size; ++i )
      v[ i ] -> execute();
  }
  virtual ~callback_t() {}
};

/* Encapsulated Vector
 * const read access
 * Modifying the vector triggers registered callbacks
 */
template <typename T>
struct vector_with_callback
{
private:
  std::vector<T> _data;
  std::vector<callback_t*> _callbacks ;
public:
  /* Register your custom callback, which will be called when the vector is modified
   */
  void register_callback( callback_t* c )
  { _callbacks.push_back( c ); }

  void trigger_callbacks() const
  { callback_t::trigger( _callbacks ); }

  void push_back( T x )
  { _data.push_back( x ); trigger_callbacks(); }

  void find_and_erase( T x )
  {
    typename std::vector<T>::iterator it = range::find( _data, x );
    if ( it != _data.end() )
      erase( it );
  }

  void find_and_erase_unordered( T x )
  {
    typename std::vector<T>::iterator it = range::find( _data, x );
    if ( it != _data.end() )
      erase_unordered( it );
  }

  // Warning: If you directly modify the vector, you need to trigger callbacks manually!
  std::vector<T>& data()
  { return _data; }

  player_t* operator[]( size_t i ) const
  { return _data[ i ]; }

  size_t size() const
  { return _data.size(); }

  bool empty() const
  { return _data.empty(); }

private:
  void erase_unordered( typename std::vector<T>::iterator it )
  { ::erase_unordered( _data, it ); trigger_callbacks(); }

  void erase( typename std::vector<T>::iterator it )
  { _data.erase( it ); trigger_callbacks(); }
};

// Simulation Engine ========================================================

struct sim_t : private sc_thread_t
{
  sim_control_t* control;
  sim_t*      parent;
  bool initialized;
  player_t*   target;
  player_t*   heal_target;
  vector_with_callback<player_t*> target_list;
  vector_with_callback<player_t*> target_non_sleeping_list;
  vector_with_callback<player_t*> player_list;
  vector_with_callback<player_t*> player_no_pet_list;
  vector_with_callback<player_t*> player_non_sleeping_list;
  player_t*   active_player;
  int         num_players;
  int         num_enemies;
  int         max_player_level;
  int         canceled, iteration_canceled;
  timespan_t  queue_lag, queue_lag_stddev;
  timespan_t  gcd_lag, gcd_lag_stddev;
  timespan_t  channel_lag, channel_lag_stddev;
  timespan_t  queue_gcd_reduction;
  int         strict_gcd_queue;
  double      confidence, confidence_estimator;
  // Latency
  timespan_t  world_lag, world_lag_stddev;
  double     travel_variance, default_skill;
  timespan_t  reaction_time, regen_periodicity;
  timespan_t  ignite_sampling_delta;
  timespan_t  current_time, max_time, expected_time;
  double      vary_combat_length;
  int         fixed_time;
  int64_t     events_remaining, max_events_remaining;
  int64_t     events_processed, total_events_processed;
  int         seed, id, iterations, current_iteration, current_slot;
  int         armor_update_interval, weapon_speed_scale_factors;
  int         optimal_raid, log, debug;
  int         save_profiles, default_actions;
  stat_e normalized_stat;
  std::string current_name, default_region_str, default_server_str, save_prefix_str, save_suffix_str;
  int         save_talent_str;
  talent_format_e talent_format;
  auto_dispose< std::vector<player_t*> > actor_list;
  std::string main_target_str;
  int         auto_ready_trigger;
  int         stat_cache;
  int         max_aoe_enemies;

  // Target options
  double      target_death;
  double      target_death_pct;
  int         rel_target_level, target_level;
  std::string target_race;
  int         target_adds;
  std::string sim_phase_str;

  // Data access
  dbc_t       dbc;

  // Default stat enchants
  gear_stats_t enchant;

  bool challenge_mode; // if active, players will get scaled down to 463 and set bonusses are deactivated
  int scale_to_itemlevel; //itemlevel to scale to. if -1, we don't scale down

  // Actor tracking
  int active_enemies;
  int active_allies;

  std::unordered_map<std::string, std::string> var_map;
  std::vector<option_t> options;
  std::vector<std::string> party_encoding;
  std::vector<std::string> item_db_sources;

  // Random Number Generation
private:
  auto_dispose< std::vector<rng_t*> > rng_list;
  int deterministic_rng;
  rng_t rng;
public:
  int separated_rng, average_range, average_gauss;
  int convergence_scale;

  rng_t* default_rng()  { return &rng; }

  bool      roll( double chance );
  double    range( double min, double max );
  double    averaged_range( double min, double max );
  double    gauss( double mean, double stddev );
  timespan_t gauss( timespan_t mean, timespan_t stddev );
  double    real() ;
  rng_t*    get_rng( const std::string& name = std::string() );

  // Timing Wheel Event Management
  event_t* recycled_event_list;
  std::vector<event_t*> timing_wheel; // This should be a vector of forward_list's
  int    wheel_seconds, wheel_size, wheel_mask, wheel_shift;
  unsigned timing_slice;
  double wheel_granularity;
  timespan_t wheel_time;

  // Raid Events
  auto_dispose< std::vector<raid_event_t*> > raid_events;
  std::string raid_events_str;
  std::string fight_style;

  // Buffs and Debuffs Overrides
  struct overrides_t
  {
    // Buff overrides
    int attack_speed;
    int attack_power_multiplier;
    int critical_strike;
    int mastery;
    int spell_haste;
    int spell_power_multiplier;
    int stamina;
    int str_agi_int;

    // Debuff overrides
    int slowed_casting;
    int magic_vulnerability;
    int mortal_wounds;
    int physical_vulnerability;
    int ranged_vulnerability;
    int weakened_armor;
    int weakened_blows;
    int bleeding;

    // Misc stuff needs resolving
    int    bloodlust;
    double target_health;
    int    stormlash;
    int    skull_banner;
  } overrides;

  // Auras
  struct auras_t
  {
    // Raid-wide auras from various classes
    aura_t* attack_power_multiplier;
    aura_t* attack_speed;
    aura_t* critical_strike;
    aura_t* mastery;
    aura_t* spell_haste;
    aura_t* spell_power_multiplier;
    aura_t* stamina;
    aura_t* str_agi_int;
  } auras;

  // Auras and De-Buffs
  auto_dispose< std::vector<buff_t*> > buff_list;
  timespan_t aura_delay;

  // Global aura related delay
  timespan_t default_aura_delay;
  timespan_t default_aura_delay_stddev;

  auto_dispose< std::vector<cooldown_t*> > cooldown_list;

  // Reporting
  progress_bar_t progress_bar;
  scaling_t* const scaling;
  plot_t*    const plot;
  reforge_plot_t* const reforge_plot;
  timespan_t elapsed_cpu;
  timespan_t elapsed_time;
  double     iteration_dmg, iteration_heal;
  simple_sample_data_t raid_dps, total_dmg, raid_hps, total_heal;
  extended_sample_data_t simulation_length;
  int        report_progress;
  int        bloodlust_percent;
  timespan_t bloodlust_time;
  std::string reference_player_str;
  std::vector<player_t*> players_by_dps;
  std::vector<player_t*> players_by_hps;
  std::vector<player_t*> players_by_name;
  std::vector<player_t*> targets_by_name;
  std::vector<std::string> id_dictionary;
  std::vector<timespan_t> iteration_timeline;
  std::vector<int> divisor_timeline;
  std::string output_file_str, html_file_str;
  std::string xml_file_str, xml_stylesheet_file_str;
  std::string reforge_plot_output_file_str;
  std::vector<std::string> error_list;
  FILE* output_file;
  int debug_exp;
  int report_precision;
  int report_pets_separately;
  int report_targets;
  int report_details;
  int report_raw_abilities;
  int report_rng;
  int hosted_html;
  int print_styles;
  int report_overheal;
  int save_raid_summary;
  int save_gear_comments;
  int statistics_level;
  int separate_stats_by_actions;
  int report_raid_summary;
  int buff_uptime_timeline;

  stopwatch_t event_stopwatch;
  int monitor_cpu;

  int allow_potions;
  int allow_food;
  int allow_flasks;
  int solo_raid;
  int global_item_upgrade_level;

  struct report_information_t
  {
    bool charts_generated;
    std::vector<std::string> dps_charts, hps_charts, gear_charts, dpet_charts;
    std::string timeline_chart, downtime_chart;
    report_information_t() { charts_generated = false; }
  } report_information;

  // Multi-Threading
  mutex_t mutex;
  int threads;
  std::vector<sim_t*> children; // Manual delete!
  int thread_index;

  virtual void run();

  // Spell database access
  spell_data_expr_t* spell_query;
  unsigned           spell_query_level;
  std::string        spell_query_xml_output_file_str;

  sim_t( sim_t* parent = 0, int thrdID = 0 );
  virtual ~sim_t();

  int       main( const std::vector<std::string>& args );
  void      cancel();
  double    progress( int* current = 0, int* final = 0 );
  double    progress( std::string& phase );
  void      combat( int iteration );
  void      combat_begin();
  void      combat_end();
  void      datacollection_begin();
  void      datacollection_end();
  void      add_event( event_t*, timespan_t delta_time );
  void      reschedule_event( event_t* );
  void      flush_events();
  event_t*  next_event();
  void      reset();
  bool      init();
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
  bool      setup( sim_control_t* );
  bool      time_to_think( timespan_t proc_time );
  timespan_t total_reaction_time ();
  double    iteration_adjust();
  player_t* find_player( const std::string& name ) ;
  player_t* find_player( int index ) ;
  cooldown_t* get_cooldown( const std::string& name );
  void      use_optimal_buffs_and_debuffs( int value );
  expr_t* create_expression( action_t*, const std::string& name );
  int       errorf( const char* format, ... ) PRINTF_ATTRIBUTE( 2, 3 );

  virtual void output(         const char* format, ... ) PRINTF_ATTRIBUTE( 2, 3 );
  static  void output( sim_t*, const char* format, ... ) PRINTF_ATTRIBUTE( 2, 3 );

  static double distribution_mean_error( const sim_t& s, const extended_sample_data_t& sd )
  { return s.confidence_estimator * sd.mean_std_dev; }
};

// Module ===================================================================

struct module_t
{
  player_e type;

  module_t( player_e t ) :
    type( t ) {}

  virtual ~module_t() {}
  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const = 0;
  virtual bool valid() const = 0;
  virtual void init( sim_t* ) const = 0;
  virtual void combat_begin( sim_t* ) const = 0;
  virtual void combat_end( sim_t* ) const = 0;

  static const module_t* death_knight();
  static const module_t* druid();
  static const module_t* hunter();
  static const module_t* mage();
  static const module_t* monk();
  static const module_t* paladin();
  static const module_t* priest();
  static const module_t* rogue();
  static const module_t* shaman();
  static const module_t* warlock();
  static const module_t* warrior();
  static const module_t* enemy();
  static const module_t* heal_enemy();

  static const module_t* get( player_e t )
  {
    switch ( t )
    {
      case DEATH_KNIGHT: return death_knight();
      case DRUID:        return druid();
      case HUNTER:       return hunter();
      case MAGE:         return mage();
      case MONK:         return monk();
      case PALADIN:      return paladin();
      case PRIEST:       return priest();
      case ROGUE:        return rogue();
      case SHAMAN:       return shaman();
      case WARLOCK:      return warlock();
      case WARRIOR:      return warrior();
      case ENEMY:        return enemy();
      default:;
    }
    return NULL;
  }
  static const module_t* get( const std::string& n )
  {
    return get( util::parse_player_type( n ) );
  }
  static void init()
  {
    for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; i++ )
      get( i );
  }
};

// Scaling ==================================================================

struct scaling_t
{
  sim_t* sim;
  sim_t* baseline_sim;
  sim_t* ref_sim;
  sim_t* delta_sim;
  sim_t* ref_sim2;
  sim_t* delta_sim2;
  stat_e scale_stat;
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
  stat_e current_scaling_stat;
  int num_scaling_stats, remaining_scaling_stats;
  std::string scale_over;
  std::string scale_over_player;

  // Gear delta for determining scale factors
  gear_stats_t stats;

  scaling_t( sim_t* s );

  void init_deltas();
  void analyze();
  void analyze_stats();
  void analyze_ability_stats( stat_e, double, player_t*, player_t*, player_t* );
  void analyze_lag();
  void normalize();
  void derive();
  double progress( std::string& phase );
  void create_options();
  bool has_scale_factors();
};

// Plot =====================================================================

struct plot_t
{
  sim_t* sim;
  std::string dps_plot_stat_str;
  double dps_plot_step;
  int    dps_plot_points;
  int    dps_plot_iterations;
  int    dps_plot_debug;
  stat_e current_plot_stat;
  int    num_plot_stats, remaining_plot_stats, remaining_plot_points;
  bool   dps_plot_positive;

  plot_t( sim_t* s );

  void analyze();
  void analyze_stats();
  double progress( std::string& phase );
  void create_options();
};

// Reforge Plot =============================================================

struct reforge_plot_t
{
  sim_t* sim;
  sim_t* current_reforge_sim;
  std::string reforge_plot_stat_str;
  std::vector<stat_e> reforge_plot_stat_indices;
  int    reforge_plot_step;
  int    reforge_plot_amount;
  int    reforge_plot_iterations;
  int    reforge_plot_debug;
  int    current_stat_combo;
  int    num_stat_combos;

  reforge_plot_t( sim_t* s );

  void generate_stat_mods( std::vector<std::vector<int> > &stat_mods,
                           const std::vector<stat_e> &stat_indices,
                           int cur_mod_stat,
                           std::vector<int> cur_stat_mods );
  void analyze();
  void analyze_stats();
  double progress( std::string& phase );
  void create_options();
};

struct plot_data_t
{
  double plot_step;
  double value;
  double error;
};

// Event ====================================================================
//
// event_t is designed to be a very simple light-weight event transporter and
// as such there are rules of use that must be honored:
//
// (1) The pure virtual execute() method MUST be implemented in the sub-class
// (2) There is 1 * sizeof( event_t ) space available to extend the sub-class
// (3) sim_t is responsible for deleting the memory associated with allocated events

struct event_t
{
  sim_t&      sim;
  player_t*   player;
  const char* name;
  timespan_t  time;
  timespan_t  reschedule_time;
  uint32_t    id;
  bool        canceled;
  event_t*    next;

  event_t( sim_t& s, const char* n = "unknown" );
  event_t( sim_t& s, player_t* p, const char* n = "unknown" );
  event_t( player_t* p, const char* n = "unknown" );

  timespan_t occurs()  { return ( reschedule_time != timespan_t::zero() ) ? reschedule_time : time; }
  timespan_t remains() { return occurs() - sim.current_time; }

  void reschedule( timespan_t new_time );

  virtual void execute() = 0; // MUST BE IMPLEMENTED IN SUB-CLASS!
  virtual ~event_t() {}

  static void cancel( event_t*& e );

  static void* allocate( std::size_t size, sim_t& );
  static void  recycle( event_t* );
  static void  release( sim_t& );

  static void* operator new( std::size_t size, sim_t& sim ) { return allocate( size, sim ); }

  static void* operator new( std::size_t ) throw() { std::terminate(); return nullptr; } // DO NOT USE
  static void  operator delete( void* ) { std::terminate(); } // DO NOT USE
  static void  operator delete( void*, sim_t& ) { std::terminate(); } // DO NOT USE
};

// Gear Rating Conversions ==================================================

enum rating_e
{
  RATING_DODGE = 0,
  RATING_PARRY,
  RATING_BLOCK,
  RATING_MELEE_HIT,
  RATING_RANGED_HIT,
  RATING_SPELL_HIT,
  RATING_MELEE_CRIT,
  RATING_RANGED_CRIT,
  RATING_SPELL_CRIT,
  RATING_PVP_RESILIENCE,
  RATING_MELEE_HASTE,
  RATING_RANGED_HASTE,
  RATING_SPELL_HASTE,
  RATING_EXPERTISE,
  RATING_MASTERY,
  RATING_PVP_POWER,
  RATING_MAX
};

inline cache_e cache_from_rating( rating_e r )
{
  switch ( r )
  {
    case RATING_SPELL_HASTE: return CACHE_SPELL_HASTE;
    case RATING_SPELL_HIT: return CACHE_SPELL_HIT;
    case RATING_SPELL_CRIT: return CACHE_SPELL_CRIT;
    case RATING_MELEE_HASTE: return CACHE_ATTACK_HASTE;
    case RATING_MELEE_HIT: return CACHE_ATTACK_HIT;
    case RATING_MELEE_CRIT: return CACHE_ATTACK_CRIT;
    case RATING_RANGED_HASTE: return CACHE_ATTACK_HASTE;
    case RATING_RANGED_HIT: return CACHE_ATTACK_HIT;
    case RATING_RANGED_CRIT: return CACHE_ATTACK_CRIT;
    case RATING_EXPERTISE: return CACHE_EXP;
    case RATING_DODGE: return CACHE_DODGE;
    case RATING_PARRY: return CACHE_PARRY;
    case RATING_BLOCK: return CACHE_BLOCK;
    case RATING_MASTERY: return CACHE_MASTERY;
    case RATING_PVP_POWER: return CACHE_NONE;
    case RATING_PVP_RESILIENCE: return CACHE_NONE;
    default: break;
  }
  assert( false ); return CACHE_NONE;
}

struct rating_t
{
  double  spell_haste,  spell_hit,  spell_crit;
  double attack_haste, attack_hit, attack_crit;
  double ranged_haste, ranged_hit, ranged_crit;
  double expertise;
  double dodge, parry, block;
  double mastery;
  double pvp_resilience, pvp_power;

  double& get( rating_e r )
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
      default: break;
    }
    assert( false ); return mastery;
  }

  rating_t()
  {
    // Initialize all ratings to a very high number
    double max = +1.0E+50;
    for ( rating_e i = static_cast<rating_e>( 0 ); i < RATING_MAX; ++i )
    {
      get( i ) = max;
    }
  }

  void init( dbc_t& dbc, int level )
  {
    // Read ratings from DBC
    for ( rating_e i = static_cast<rating_e>( 0 ); i < RATING_MAX; ++i )
    {
      get( i ) = dbc.combat_rating( i,  level );
      if ( i == RATING_MASTERY )
        get( i ) /= 100.0;
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

// Weapon ===================================================================

struct weapon_t
{
  weapon_e type;
  school_e school;
  double damage, dps;
  double min_dmg, max_dmg;
  timespan_t swing_time;
  slot_e slot;
  int    buff_type;
  double buff_value;
  double bonus_dmg;

  weapon_t( weapon_e t    = WEAPON_NONE,
            double d      = 0.0,
            timespan_t st = timespan_t::from_seconds( 2.0 ),
            school_e s    = SCHOOL_PHYSICAL ) :
    type( t ),
    school( s ),
    damage( d ),
    dps( d / st.total_seconds() ),
    min_dmg( d ),
    max_dmg( d ),
    swing_time( st ),
    slot( SLOT_INVALID ),
    buff_type( 0 ),
    buff_value( 0.0 ),
    bonus_dmg( 0.0 )
  {}

  weapon_e group()
  {
    if ( type <= WEAPON_SMALL )
      return WEAPON_SMALL;

    if ( type <= WEAPON_1H )
      return WEAPON_1H;

    if ( type <= WEAPON_2H )
      return WEAPON_2H;

    if ( type <= WEAPON_RANGED )
      return WEAPON_RANGED;

    return WEAPON_NONE;
  }

  timespan_t normalized_weapon_speed()
  {
    weapon_e g = group();

    if ( g == WEAPON_SMALL  ) return timespan_t::from_seconds( 1.7 );
    if ( g == WEAPON_1H     ) return timespan_t::from_seconds( 2.4 );
    if ( g == WEAPON_2H     ) return timespan_t::from_seconds( 3.3 );
    if ( g == WEAPON_RANGED ) return timespan_t::from_seconds( 2.8 );

    assert( false );
    return timespan_t::zero();
  }

  double proc_chance_on_swing( double PPM, timespan_t adjusted_swing_time = timespan_t::zero() )
  {
    if ( adjusted_swing_time == timespan_t::zero() ) adjusted_swing_time = swing_time;

    timespan_t time_to_proc = timespan_t::from_seconds( 60.0 ) / PPM;
    double proc_chance = adjusted_swing_time / time_to_proc;

    return proc_chance;
  }
};

// Special Effect ===========================================================

struct special_effect_t
{
  std::string name_str, trigger_str, encoding_str;
  proc_e trigger_type;
  int64_t trigger_mask;
  stat_e stat;
  school_e school;
  int max_stacks;
  double stat_amount, discharge_amount, discharge_scaling;
  double proc_chance;
  double ppm;
  rppm_scale_e rppm_scale;
  timespan_t duration, cooldown, tick;
  bool cost_reduction;
  bool no_refresh;
  bool chance_to_discharge;
  unsigned int override_result_es_mask;
  unsigned result_es_mask;
  bool reverse;
  int aoe;
  bool proc_delay;
  bool unique;
  unsigned spell_id, aura_spell_id;
  action_t* execute_action; // Allows custom action to be executed on use

  special_effect_t() :
    trigger_type( PROC_NONE ), trigger_mask( 0 ), stat( STAT_NONE ), school( SCHOOL_NONE ),
    max_stacks( 0 ), stat_amount( 0 ), discharge_amount( 0 ), discharge_scaling( 0 ),
    proc_chance( 0 ), ppm( 0 ), rppm_scale( RPPM_HASTE ), duration( timespan_t::zero() ), cooldown( timespan_t::zero() ),
    tick( timespan_t::zero() ), cost_reduction( false ),
    no_refresh( false ), chance_to_discharge( false ), override_result_es_mask( 0 ),
    result_es_mask( 0 ), reverse( false ), aoe( 0 ), proc_delay( false ), unique( false ),
    spell_id( 0 ), aura_spell_id( 0 ), execute_action( nullptr )
  { }

  std::string to_string();
  bool active() { return stat != STAT_NONE || school != SCHOOL_NONE || execute_action; }
  void clear()
  {
    name_str.clear(); trigger_type = PROC_NONE; trigger_mask = 0; stat = STAT_NONE;
    school = SCHOOL_NONE; max_stacks = 0; stat_amount = 0; discharge_amount = 0;
    discharge_scaling = 0; proc_chance = 0; ppm = 0; rppm_scale = RPPM_HASTE;
    duration = timespan_t::zero(); cooldown = timespan_t::zero();
    tick = timespan_t::zero(); cost_reduction = false; no_refresh = false;
    chance_to_discharge = false; override_result_es_mask = 0;
    result_es_mask = 0; reverse = false; aoe = 0; proc_delay = false;
    unique = false; spell_id = 0; aura_spell_id = 0;
  }
};

// Item =====================================================================

struct stat_pair_t
{
  stat_e stat;
  int value;

  stat_pair_t() : stat( STAT_NONE ), value( 0 )
  { }

  stat_pair_t( stat_e s, int v ) : stat( s ), value( v )
  { }
};

struct item_t
{
  sim_t* sim;
  player_t* player;
  slot_e slot;
  bool unique, unique_addon, is_ptr, fetched;

  // Structure contains the "parsed form" of this specific item, be the data
  // from user options, or a data source such as the Blizzard API, or Wowhead
  struct parsed_input_t
  {
    stat_e                   reforged_from;
    stat_e                   reforged_to;
    int                      upgrade_level;
    int                      suffix_id;
    unsigned                 enchant_id;
    unsigned                 addon_id;
    unsigned                 reforge_id;
    int                      armor;
    std::array<int, 3>       gem_id;
    std::vector<stat_pair_t> gem_stats;
    std::vector<stat_pair_t> enchant_stats;
    std::vector<stat_pair_t> addon_stats;
    std::vector<stat_pair_t> suffix_stats;
    item_data_t              data;
    special_effect_t         use, equip, enchant, addon;
    std::vector<std::string> source_list;

    parsed_input_t() :
      reforged_from( STAT_NONE ), reforged_to( STAT_NONE ), upgrade_level( 0 ),
      suffix_id( 0 ), enchant_id( 0 ), addon_id( 0 ), reforge_id( 0 ), armor( 0 ),
      data(), use(), equip(), enchant(), addon()
    {
      range::fill( data.stat_type_e, -1 );
      range::fill( data.stat_val, 0 );
      range::fill( data.cooldown_spell, -1 );
      range::fill( data.cooldown_category, -1 );
      range::fill( gem_id, 0 );
    }
  } parsed;

  xml_node_t* xml;

  std::string name_str;
  std::string icon_str;

  std::string source_str;
  std::string options_str;

  // Option Data
  std::string option_name_str;
  std::string option_stats_str;
  std::string option_gems_str;
  std::string option_enchant_str;
  std::string option_addon_str;
  std::string option_equip_str;
  std::string option_use_str;
  std::string option_weapon_str;
  std::string option_heroic_str;
  std::string option_lfr_str;
  std::string option_thunderforged_str;
  std::string option_armor_type_str;
  std::string option_reforge_str;
  std::string option_ilevel_str;
  std::string option_quality_str;
  std::string option_data_source_str;

  // Extracted data
  gear_stats_t base_stats, stats;

  item_t() : sim( 0 ), player( 0 ), slot( SLOT_INVALID ), unique( false ),
    unique_addon( false ), is_ptr( false ), fetched( false ),
    parsed(), xml( 0 ) { }
  item_t( player_t*, const std::string& options_str );

  bool active();
  const char* name();
  const char* slot_name();
  weapon_t* weapon();
  bool init();
  bool parse_options();

  bool is_matching_type();
  bool is_valid_type();

  unsigned item_level();
  unsigned upgrade_level();
  stat_e stat( size_t idx );
  int stat_value( size_t idx );
  bool has_item_stat( stat_e stat );

  std::string encoded_item();
  std::string encoded_comment();

  std::string encoded_stats();
  std::string encoded_weapon();
  std::string encoded_gems();
  std::string encoded_enchant();
  std::string encoded_addon();
  std::string encoded_upgrade_level();
  std::string encoded_random_suffix_id();
  std::string encoded_reforge();

  bool decode_stats();
  bool decode_gems();
  bool decode_enchant();
  bool decode_addon();
  bool decode_special( special_effect_t&, const std::string& encoding );
  bool decode_weapon();
  bool decode_heroic();
  bool decode_lfr();
  bool decode_thunderforged();
  bool decode_armor_type();
  bool decode_reforge();
  bool decode_random_suffix();
  bool decode_upgrade_level();
  bool decode_ilevel();
  bool decode_quality();
  bool decode_data_source();

  bool decode_proc_spell( special_effect_t& effect );

  bool parse_reforge_id();

  static bool download_slot( item_t& item );
  static bool download_item( item_t& );
  static bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id );
  static unsigned parse_gem( item_t& item, unsigned gem_id );

  static std::vector<stat_pair_t> str_to_stat_pair( const std::string& stat_str );
  static std::string stat_pairs_to_str( const std::vector<stat_pair_t>& stat_pairs );

  std::string to_string();
};


// Benefit ==================================================================

struct benefit_t : public noncopyable
{
private:
  int up, down;
public:
  simple_sample_data_t ratio;
  const std::string name_str;

  explicit benefit_t( const std::string& n ) :
    up( 0 ), down( 0 ),
    ratio(), name_str( n ) {}

  void update( bool is_up )
  { if ( is_up ) up++; else down++; }
  void datacollection_begin()
  { up = down = 0; }
  void datacollection_end()
  { ratio.add( up != 0 ? 100.0 * up / ( down + up ) : 0.0 ); }
  void merge( const benefit_t& other )
  { ratio.merge( other.ratio ); }

  const char* name() const
  { return name_str.c_str(); }
};

// Proc =====================================================================

struct proc_t : public noncopyable
{
private:
  sim_t& sim;
  size_t iteration_count; // track number of procs during the current iteration
  timespan_t last_proc; // track time of the last proc
public:
  const std::string name_str;
  simple_sample_data_t interval_sum;
  simple_sample_data_t count;

  proc_t( sim_t& s, const std::string& n ) :
    sim( s ),
    iteration_count(),
    last_proc( timespan_t::min() ),
    name_str( n ),
    interval_sum(),
    count()
  {}

  void occur()
  {
    iteration_count++;
    if ( last_proc >= timespan_t::zero() && last_proc < sim.current_time )
    {
      interval_sum.add( ( sim.current_time - last_proc ).total_seconds() );
      reset();
    }
    if ( sim.debug )
      sim.output( "[PROC] %s: iteration_count=%u count.sum=%u last_proc=%f",
                  name(),
                  static_cast<unsigned>( iteration_count ),
                  static_cast<unsigned>( count.sum() ),
                  last_proc.total_seconds() );

    last_proc = sim.current_time;
  }

  void reset()
  { last_proc = timespan_t::min(); }

  void merge( const proc_t& other )
  {
    count.merge( other.count );
    interval_sum.merge( other.interval_sum );
  }

  void datacollection_begin()
  { iteration_count = 0; }
  void datacollection_end()
  { count.add( static_cast<double>( iteration_count ) ); }

  const char* name() const
  { return name_str.c_str(); }
};

// Set Bonus ================================================================

struct set_bonus_t
{
  std::array<int, SET_MAX> count;

  int tier13_2pc_caster(); int tier13_2pc_melee(); int tier13_2pc_tank(); int tier13_2pc_heal();
  int tier13_4pc_caster(); int tier13_4pc_melee(); int tier13_4pc_tank(); int tier13_4pc_heal();
  int tier14_2pc_caster(); int tier14_2pc_melee(); int tier14_2pc_tank(); int tier14_2pc_heal();
  int tier14_4pc_caster(); int tier14_4pc_melee(); int tier14_4pc_tank(); int tier14_4pc_heal();
  int tier15_2pc_caster(); int tier15_2pc_melee(); int tier15_2pc_tank(); int tier15_2pc_heal();
  int tier15_4pc_caster(); int tier15_4pc_melee(); int tier15_4pc_tank(); int tier15_4pc_heal();
  int tier16_2pc_caster(); int tier16_2pc_melee(); int tier16_2pc_tank(); int tier16_2pc_heal();
  int tier16_4pc_caster(); int tier16_4pc_melee(); int tier16_4pc_tank(); int tier16_4pc_heal();
  int pvp_2pc_caster(); int pvp_2pc_melee(); int pvp_2pc_tank(); int pvp_2pc_heal();
  int pvp_4pc_caster(); int pvp_4pc_melee(); int pvp_4pc_tank(); int pvp_4pc_heal();
  int decode( player_t*, item_t& item );
  bool init( player_t* );

  set_bonus_t();

  expr_t* create_expression( player_t*, const std::string& type );
};

struct set_bonus_array_t
{
private:
  const spell_data_t* default_value;
  const spell_data_t* set_bonuses[ SET_MAX ];
  player_t* p;

  const spell_data_t* create_set_bonus( uint32_t spell_id );

public:
  set_bonus_array_t( player_t* p, const uint32_t a_bonus[ N_TIER ][ N_TIER_BONUS ] );

  bool              has_set_bonus( set_e s );
  const spell_data_t* set( set_e s );
};

struct action_sequence_data_t
{
  action_t* action;
  player_t* target;
  timespan_t time;
  std::vector<buff_t*> buff_list;

  action_sequence_data_t( action_t* a, player_t* t, timespan_t ts, std::vector<buff_t*> bl ) : action( a ), target( t ), time( ts )
  {
    for ( size_t i = 0; i < bl.size(); ++i )
      if ( bl[ i ] -> check() && ! bl[ i ] -> quiet )
        buff_list.push_back( bl[ i ] );
  }
};

// Cooldown =================================================================

struct cooldown_t
{
  sim_t& sim;
  player_t* player;
  std::string name_str;
  timespan_t duration;
  timespan_t ready;
  timespan_t reset_react;
  int charges;
  int current_charge;
  event_t* recharge_event;
  event_t* ready_trigger_event;
  timespan_t last_start;

  cooldown_t( const std::string& name, player_t& );
  cooldown_t( const std::string& name, sim_t& );

  void adjust( timespan_t );
  void reset( bool require_reaction );
  void start( timespan_t override = timespan_t::min(), timespan_t delay = timespan_t::zero() );

  timespan_t remains() const
  { return std::max( timespan_t::zero(), ready - sim.current_time ); }

  // return true if the cooldown is done (i.e., the associated ability is ready)
  bool up() const
  { return ready <= sim.current_time; }

  // Return true if the cooldown is currently ticking down
  bool down() const
  { return ready > sim.current_time; }

  const char* name() const
  { return name_str.c_str(); }

  timespan_t reduced_cooldown() const
  { return ready - last_start; }

private:
  static timespan_t ready_init()
  { return timespan_t::from_seconds( -60 * 60 ); }
};

// Player Callbacks
struct player_callbacks_t
{
  std::vector<action_callback_t*> all_callbacks;
  std::array< std::vector<action_callback_t*>, RESULT_MAX > attack;
  std::array< std::vector<action_callback_t*>, RESULT_MAX > spell;
  std::array< std::vector<action_callback_t*>, RESULT_MAX > harmful_spell;
  std::array< std::vector<action_callback_t*>, RESULT_MAX > direct_harmful_spell;
  std::array< std::vector<action_callback_t*>, RESULT_MAX > heal;
  std::array< std::vector<action_callback_t*>, RESULT_MAX > absorb;
  std::array< std::vector<action_callback_t*>, RESULT_MAX > tick;

  std::array< std::vector<action_callback_t*>, SCHOOL_MAX > direct_damage;
  std::array< std::vector<action_callback_t*>, SCHOOL_MAX > direct_crit;
  std::array< std::vector<action_callback_t*>, SCHOOL_MAX > tick_damage;
  std::array< std::vector<action_callback_t*>, SCHOOL_MAX > spell_direct_damage;
  std::array< std::vector<action_callback_t*>, SCHOOL_MAX > spell_tick_damage;
  std::array< std::vector<action_callback_t*>, SCHOOL_MAX > direct_heal;
  std::array< std::vector<action_callback_t*>, SCHOOL_MAX > tick_heal;

  std::array< std::vector<action_callback_t*>, RESOURCE_MAX > resource_gain;
  std::array< std::vector<action_callback_t*>, RESOURCE_MAX > resource_loss;

  std::array< std::vector<action_callback_t*>, RESULT_MAX > incoming_attack;

  virtual ~player_callbacks_t()
  { range::sort( all_callbacks ); dispose( all_callbacks.begin(), range::unique( all_callbacks ) ); }

  void reset();

  void register_resource_gain_callback       ( resource_e,     action_callback_t*      );
  void register_resource_loss_callback       ( resource_e,     action_callback_t*      );
  void register_attack_callback              ( int64_t result_mask, action_callback_t* );
  void register_spell_callback               ( int64_t result_mask, action_callback_t* );
  void register_tick_callback                ( int64_t result_mask, action_callback_t* );
  void register_heal_callback                ( int64_t result_mask, action_callback_t* );
  void register_absorb_callback              ( int64_t result_mask, action_callback_t* );
  void register_harmful_spell_callback       ( int64_t result_mask, action_callback_t* );
  void register_direct_harmful_spell_callback( int64_t result_mask, action_callback_t* );
  void register_tick_damage_callback         ( int64_t result_mask, action_callback_t* );
  void register_direct_damage_callback       ( int64_t result_mask, action_callback_t* );
  void register_direct_crit_callback         ( int64_t result_mask, action_callback_t* );
  void register_spell_tick_damage_callback   ( int64_t result_mask, action_callback_t* );
  void register_spell_direct_damage_callback ( int64_t result_mask, action_callback_t* );
  void register_tick_heal_callback           ( int64_t result_mask, action_callback_t* );
  void register_direct_heal_callback         ( int64_t result_mask, action_callback_t* );
  void register_incoming_attack_callback     ( int64_t result_mask, action_callback_t* );
};
/* The Cache system increases simulation performance by moving the calculation point
 * from call-time to modification-time of a stat. Because a stat is called much more
 * often than it is changed, this reduces costly and unnecessary repetition of floating-point
 * operations.
 *
 * When a stat is accessed, its 'valid'-state is checked:
 *   If its true, the cached value is accessed.
 *   If it is false, the stat value is recalculated and written to the cache.
 *
 * To indicate when a stat gets modified, it needs to be 'invalidated'. Every time a stat
 * is invalidated, its 'valid'-state gets set to false.
 */

/* - To invalidate a stat, use player_t::invalidate_cache( cache_e )
 * - using player_t::stat_gain/loss automatically invalidates the corresponding cache
 * - Same goes for stat_buff_t, which works through player_t::stat_gain/loss
 * - Buffs with effects in a composite_ function need invalidates added to their buff_creator
 *
 * To create invalidation chains ( eg. Priest: Spirit invalidates Hit ) override the
 * virtual player_t::invalidate_cache( cache_e ) function.
 *
 * Attention: player_t::invalidate_cache( cache_e ) is recursive and may call itself again.
 */
struct player_stat_cache_t
{
  player_t* player;
  // 'valid'-states
  std::array<bool, CACHE_MAX> valid;
  std::array < bool, SCHOOL_MAX + 1 > spell_power_valid, player_mult_valid, player_heal_mult_valid;
private:
  // cached values
  double _strength, _agility, _stamina, _intellect, _spirit;
  double _spell_power[SCHOOL_MAX + 1], _attack_power;
  double _attack_expertise;
  double _attack_hit, _spell_hit;
  double _attack_crit, _spell_crit;
  double _attack_haste, _spell_haste;
  double _attack_speed, _spell_speed;
  double _dodge, _parry, _block, _crit_block, _armor;
  double _mastery_value, _crit_avoidance, _miss;
  double _player_mult[SCHOOL_MAX + 1], _player_heal_mult[SCHOOL_MAX + 1];
public:
  bool active; // runtime active-flag
  void invalidate();
  double get_attribute( attribute_e );
  player_stat_cache_t( player_t* p ) : player( p ), active( false ) { invalidate(); }
#ifdef SC_STAT_CACHE
  // Cache stat functions
  double strength();
  double agility();
  double stamina();
  double intellect();
  double spirit();
  double spell_power( school_e );
  double attack_power();
  double attack_expertise();
  double attack_hit();
  double attack_crit();
  double attack_haste();
  double attack_speed();
  double spell_hit();
  double spell_crit();
  double spell_haste();
  double spell_speed();
  double dodge();
  double parry();
  double block();
  double crit_block();
  double crit_avoidance();
  double miss();
  double armor();
  double mastery_value();
  double player_multiplier( school_e );
  double player_heal_multiplier( school_e );
#else
  // Passthrough cache stat functions for inactive cache
  double strength()  { return player -> strength();  }
  double agility()   { return player -> agility();   }
  double stamina()   { return player -> stamina();   }
  double intellect() { return player -> intellect(); }
  double spirit()    { return player -> spirit();    }
  double spell_power( school_e s ) { return player -> composite_spell_power( s ); }
  double attack_power()            { return player -> composite_melee_attack_power();   }
  double attack_expertise() { return player -> composite_melee_expertise(); }
  double attack_hit()       { return player -> composite_melee_hit();       }
  double attack_crit()      { return player -> composite_melee_crit();      }
  double attack_haste()     { return player -> composite_melee_haste();     }
  double attack_speed()     { return player -> composite_melee_speed();     }
  double spell_hit()        { return player -> composite_spell_hit();       }
  double spell_crit()       { return player -> composite_spell_crit();      }
  double spell_haste()      { return player -> composite_spell_haste();     }
  double spell_speed()      { return player -> composite_spell_speed();     }
  double dodge()            { return player -> composite_dodge();      }
  double parry()            { return player -> composite_parry();      }
  double block()            { return player -> composite_block();      }
  double crit_block()       { return player -> composite_crit_block(); }
  double crit_avoidance()   { return player -> composite_crit_avoidance();       }
  double miss()             { return player -> composite_miss();       }
  double armor()            { return player -> composite_armor();           }
  double mastery_value()    { return player -> composite_mastery_value();   }
#endif
};

// Player Vengeance

struct player_vengeance_t
{
private:
  sc_timeline_t& timeline_; // reference to collected_data.vengeance_timeline
  event_t* event; // pointer to collection event so we can cancel it at the end of combat.

public:
  player_vengeance_t( sc_timeline_t& vengeance_tl ) :
    timeline_( vengeance_tl ), event( nullptr ) {}

  void init( player_t& p );
  void start( player_t& p );
  void stop();

  bool is_initialized() const
  { return ! timeline_.data().empty(); }
  bool is_started() const
  { return event != 0; }

  void adjust( const std::vector<int>& divisor_timeline )
  {
    if ( timeline_.data().size() > 0 )
      timeline_.adjust( divisor_timeline );
  }

  void merge( const player_vengeance_t& other )
  { timeline_.merge( other.timeline_ ); }

  const sc_timeline_t& timeline() const { return timeline_; }
};

struct player_processed_report_information_t
{
  bool charts_generated, buff_lists_generated;
  std::string action_dpet_chart, action_dmg_chart, time_spent_chart;
  std::array<std::string, RESOURCE_MAX> timeline_resource_chart, gains_chart;
  std::string timeline_dps_chart, timeline_dps_error_chart, timeline_resource_health_chart;
  std::string distribution_dps_chart, scaling_dps_chart, scale_factors_chart;
  std::string reforge_dps_chart, dps_error_chart, distribution_deaths_chart;
  std::string gear_weights_lootrank_link, gear_weights_wowhead_std_link, gear_weights_wowhead_alt_link, gear_weights_wowreforge_link, gear_weights_wowupgrade_link;
  std::string gear_weights_pawn_std_string, gear_weights_pawn_alt_string;
  std::string save_str;
  std::string save_gear_str;
  std::string save_talents_str;
  std::string save_actions_str;
  std::string comment_str;
  std::string thumbnail_url;
  std::string html_profile_str;
  std::vector<buff_t*> buff_list, dynamic_buffs, constant_buffs;

  player_processed_report_information_t() : charts_generated(), buff_lists_generated() {}
};

/* Contains any data collected during / at the end of combat
 * Mostly statistical data collection, represented as sample data containers
 */
struct player_collected_data_t
{
  extended_sample_data_t fight_length;
  simple_sample_data_t waiting_time, executed_foreground_actions;

  // DMG
  extended_sample_data_t dmg;
  extended_sample_data_t compound_dmg;
  extended_sample_data_t dps;
  extended_sample_data_t dpse;
  extended_sample_data_t dtps;
  extended_sample_data_t dmg_taken;
  sc_timeline_t timeline_dmg;
  sc_timeline_t timeline_dmg_taken;
  // Heal
  extended_sample_data_t heal;
  extended_sample_data_t compound_heal;
  extended_sample_data_t hps;
  extended_sample_data_t hpse;
  extended_sample_data_t htps;
  extended_sample_data_t heal_taken;
  // Tank
  extended_sample_data_t deaths;
  sc_timeline_t vengeance_timeline;

  struct resource_timeline_t
  {
    resource_e type;
    sc_timeline_t timeline;

    resource_timeline_t( resource_e t = RESOURCE_NONE ) : type( t ) {}
  };
  // Druid requires 4 resource timelines health/mana/energy/rage
  std::vector<resource_timeline_t> resource_timelines;

  std::vector<simple_sample_data_with_min_max_t > combat_end_resource;

  struct action_sequence_data_t
  {
    const action_t* action;
    const player_t* target;
    const timespan_t time;
    std::vector<buff_t*> buff_list;
    std::array<double, RESOURCE_MAX> resource_snapshot;

    action_sequence_data_t( const action_t* a, const player_t* t, const timespan_t& ts, const player_t* p );
  };
  auto_dispose< std::vector<action_sequence_data_t*> > action_sequence;

  player_collected_data_t( const std::string& player_name, sim_t& );
  void reserve_memory( const player_t& );
  void merge( const player_collected_data_t& );
  void analyze( const player_t& );
  void collect_data( const player_t& );
};

// Actor

struct actor_t : public noncopyable
{
#ifndef NDEBUG
  static const bool ACTOR_EVENT_BOOKKEEPING = true;
#else
  static const bool ACTOR_EVENT_BOOKKEEPING = false;
#endif

  sim_t* sim; // owner
  std::string name_str;
  int event_counter; // safety counter. Shall never be less than zero

  actor_t( sim_t* s, const std::string& name ) :
    sim( s ), name_str( name ),
    event_counter( 0 )
  {

  }
  virtual ~ actor_t() { }
};

// Player ===================================================================

struct player_t : public actor_t
{
  static const int default_level = 90;

  // static values
  player_e type;
  int index;
  size_t actor_index;
  // (static) attributes - things which should not change during combat
  race_e       race;
  role_e       role;
  int          level;
  int          party, member;
  int          ready_type;
  specialization_e  _spec;
  bool         bugs; // If true, include known InGame mechanics which are probably the cause of a bug and not inteded
  bool scale_player;

  // dynamic attributes - things which change during combat
  player_t*   target;
  int         initialized;
  bool        potion_used;

  std::string talents_str, glyphs_str, id_str, target_str;
  std::string region_str, server_str, origin_str;
  std::string race_str, professions_str, position_str;
  timespan_t  gcd_ready, base_gcd, started_waiting;
  std::vector<pet_t*> pet_list;
  std::vector<pet_t*> active_pets;
  std::vector<absorb_buff_t*> absorb_buff_list;

  int         invert_scaling;
  double      avg_ilvl;

  // Reaction
  timespan_t  reaction_offset, reaction_mean, reaction_stddev, reaction_nu;
  // Latency
  timespan_t  world_lag, world_lag_stddev;
  timespan_t  brain_lag, brain_lag_stddev;
  bool        world_lag_override, world_lag_stddev_override;

  // Data access
  dbc_t       dbc;

  // Option Parsing
  std::vector<option_t> options;

  // Talent Parsing
  class talent_points_t
  {
    std::array<int, MAX_TALENT_ROWS> choices;

    static void row_check( int row )
    { assert( row >= 0 && row < MAX_TALENT_ROWS ); ( void )row; }

    static void column_check( int col )
    { assert( col >= 0 && col < MAX_TALENT_COLS ); ( void )col; }

    static void row_col_check( int row, int col )
    { row_check( row ); column_check( col ); }

  public:
    talent_points_t() { clear(); }

    int choice( int row ) const
    {
      row_check( row );
      return choices[ row ];
    }

    void clear( int row )
    {
      row_check( row );
      choices[ row ] = -1;
    }

    bool has_row_col( int row, int col ) const
    { return choice( row ) == col; }

    void select_row_col( int row, int col )
    {
      row_col_check( row, col );
      choices[ row ] = col;
    }

    void clear();
    std::string to_string() const;

    friend std::ostream& operator << ( std::ostream& os, const talent_points_t& tp )
    { os << tp.to_string(); return os; }
  } talent_points;
  std::string talent_overrides_str;

  // Glyph Parsing
  std::vector<const spell_data_t*> glyph_list;

  // Profs
  std::array<int, PROFESSION_MAX> profession;

  struct base_initial_current_t
  {
    base_initial_current_t();
    std::string to_string();
    gear_stats_t stats;

    double mana_regen_per_second;

    double spell_power_per_intellect, spell_crit_per_intellect;
    double attack_power_per_strength, attack_power_per_agility, attack_crit_per_agility;
    double dodge_per_agility, parry_rating_per_strength;
    double mana_regen_per_spirit, mana_regen_from_spirit_multiplier, health_per_stamina;
    std::array<double, SCHOOL_MAX> resource_reduction;
    double miss, dodge, parry, block;
    double spell_crit, attack_crit, block_reduction, mastery;
    double skill, distance;
    double armor_coeff;
  private:
    friend struct player_t;
    bool sleeping;
    rating_t    rating;
  public:

    std::array<double, ATTRIBUTE_MAX> attribute_multiplier;
    double spell_power_multiplier, attack_power_multiplier, armor_multiplier;
    position_e position;
  }
  base, // Base values, from some database or overridden by user
  initial, // Base + Gear + Global Enchants
  current; // Current values, reset to initial before every iteration

  const rating_t& current_rating() const
  { return current.rating; }
  const rating_t& initial_rating() const
  { return initial.rating; }

  // Spell Mechanics
  double base_energy_regen_per_second;
  double base_focus_regen_per_second;
  double base_chi_regen_per_second;
  timespan_t last_cast;

  // Defense Mechanics
  double diminished_dodge_cap, diminished_parry_cap, diminished_block_cap, diminished_kfactor;

  // Weapons
  weapon_t main_hand_weapon;
  weapon_t off_hand_weapon;

  // Main, offhand, and ranged attacks
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  // Resources
  struct resources_t
  {
    std::array<double, RESOURCE_MAX> base, initial, max, current,
        base_multiplier, initial_multiplier;
    std::array<int, RESOURCE_MAX> infinite_resource;

    resources_t()
    {
      range::fill( base, 0.0 );
      range::fill( initial, 0.0 );
      range::fill( max, 0.0 );
      range::fill( current, 0.0 );
      range::fill( base_multiplier, 1.0 );
      range::fill( initial_multiplier, 1.0 );
      range::fill( infinite_resource, 0 );
    }

    double pct( resource_e rt ) const
    { return current[ rt ] / max[ rt ]; }

    bool is_infinite( resource_e rt ) const
    { return infinite_resource[ rt ] != 0; }
  } resources;

  // Consumables
  std::string flask_str, elixirs_str, food_str;
  struct {
    bool guardian, battle;
  } active_elixir;
  int flask;
  int food;

  // Events
  action_t* executing;
  action_t* channeling;
  event_t*  readying;
  event_t*  off_gcd;
  bool      in_combat;
  bool      action_queued;
  action_t* last_foreground_action;

  // Delay time used by "cast_delay" expression to determine when an action
  // can be used at minimum after a spell cast has finished, including GCD
  timespan_t cast_delay_reaction;
  timespan_t cast_delay_occurred;

  // Callbacks
  player_callbacks_t callbacks;

  // Action Priority List
  auto_dispose< std::vector<action_t*> > action_list;
  std::string action_list_str;
  std::string choose_action_list;
  std::string action_list_skip;
  std::string modify_action;
  int         action_list_default;
  auto_dispose< std::vector<dot_t*> > dot_list;
  auto_dispose< std::vector<action_priority_list_t*> > action_priority_list;
  std::vector<action_t*> precombat_action_list;
  action_priority_list_t* active_action_list;
  action_priority_list_t* active_off_gcd_list;
  action_priority_list_t* restore_action_list;
  std::map<std::string, std::string> alist_map;
  std::string action_list_information; // comment displayed in profile
  bool no_action_list_provided;

  // Reporting
  int       quiet;
  timespan_t iteration_fight_length,arise_time;
  timespan_t iteration_waiting_time;
  int       iteration_executed_foreground_actions;
  std::array< double, RESOURCE_MAX > resource_lost, resource_gained;
  double    rps_gain, rps_loss;

  // Buffed snapshot_stats (for reporting)
  struct buffed_stats_t
  {
    std::array< double, ATTRIBUTE_MAX > attribute;
    std::array< double, RESOURCE_MAX > resource;

    double spell_power, spell_hit, spell_crit, manareg_per_second;
    double attack_power,  attack_hit,  mh_attack_expertise,  oh_attack_expertise, attack_crit;
    double armor, miss, crit, dodge, parry, block;
    double spell_haste, spell_speed, attack_haste, attack_speed;
    double mastery_value;
  } buffed;

  auto_dispose< std::vector<buff_t*> > buff_list;
  auto_dispose< std::vector<proc_t*> > proc_list;
  auto_dispose< std::vector<gain_t*> > gain_list;
  auto_dispose< std::vector<stats_t*> > stats_list;
  auto_dispose< std::vector<benefit_t*> > benefit_list;
  auto_dispose< std::vector<uptime_t*> > uptime_list;
  auto_dispose< std::vector<cooldown_t*> > cooldown_list;
  auto_dispose< std::vector<rng_t*> > rng_list;
  std::array< std::vector<plot_data_t>, STAT_MAX > dps_plot_data;
  std::vector<std::vector<plot_data_t> > reforge_plot_data;

  // All Data collected during / end of combat
  player_collected_data_t collected_data;

private:
  player_vengeance_t vengeance;
public:
  void vengeance_init() { vengeance.init( *this ); }
  void vengeance_start() { vengeance.start( *this ); }
  void vengeance_stop() { vengeance.stop(); }
  bool vengeance_is_started() const { return vengeance.is_started(); }
  const sc_timeline_t& vengeance_timeline() const { return vengeance.timeline(); }

  // Damage
  double iteration_dmg, iteration_dmg_taken; // temporary accumulators
  double dpr;

  std::vector<double> dps_convergence_error;
  double dps_convergence;

  // Heal
  double iteration_heal, iteration_heal_taken; // temporary accumulators
  double hpr;


  player_processed_report_information_t report_information;

  void sequence_add( const action_t* a, const player_t* target, const timespan_t& ts );

  // Gear
  std::string items_str, meta_gem_str;
  std::vector<item_t> items;
  gear_stats_t gear, enchant, temporary;
  gear_stats_t total_gear; // composite of gear, enchant and for non-pets sim -> enchant
  set_bonus_t set_bonus;
  set_bonus_array_t* sets;
  meta_gem_e meta_gem;
  bool matching_gear;
  cooldown_t item_cooldown;
  //bool has_shield_equipped;

  // Scale Factors
  gear_stats_t scaling;
  gear_stats_t scaling_normalized;
  gear_stats_t scaling_error;
  gear_stats_t scaling_delta_dps;
  gear_stats_t scaling_compare_error;
  double       scaling_lag, scaling_lag_error;
  std::array<bool, STAT_MAX> scales_with;
  std::array<double, STAT_MAX> over_cap;
  std::vector<stat_e> scaling_stats; // sorting vector

  // Movement & Position
  double base_movement_speed;
  double x_position, y_position;

  struct buffs_t
  {
    buff_t* beacon_of_light;
    buff_t* blood_fury;
    buff_t* body_and_soul;
    buff_t* earth_shield;
    buff_t* exhaustion;
    buff_t* grace;
    buff_t* guardian_spirit;
    buff_t* heroic_presence;
    buff_t* hymn_of_hope;
    buff_t* illuminated_healing;
    buff_t* innervate;
    buff_t* lifeblood;
    buff_t* mongoose_mh;
    buff_t* mongoose_oh;
    buff_t* pain_supression;
    buff_t* raid_movement;
    buff_t* self_movement;
    buff_t* skull_banner;
    buff_t* stoneform;
    buff_t* stormlash;
    buff_t* stunned;
    buff_t* tricks_of_the_trade;
    buff_t* weakened_soul;
    buff_t* vengeance;

    haste_buff_t* berserking;
    haste_buff_t* bloodlust;
    haste_buff_t* unholy_frenzy;

    // Legendary meta stuff
    buff_t* courageous_primal_diamond_lucidity;
    buff_t* tempus_repit;
    buff_t* fortitude;
  } buffs;

  struct potion_buffs_t
  {
    // Cataclysm
    stat_buff_t* earthen;
    stat_buff_t* golemblood;
    stat_buff_t* speed;
    stat_buff_t* tolvir;
    stat_buff_t* volcanic;

    // MoP
    stat_buff_t* virmens_bite;
    stat_buff_t* mogu_power;
    stat_buff_t* jade_serpent;
    stat_buff_t* mountains;
  } potion_buffs;

  struct debuffs_t
  {
    debuff_t* bleeding;
    debuff_t* casting;
    debuff_t* flying;
    debuff_t* forbearance;
    debuff_t* invulnerable;
    debuff_t* vulnerable;

    // MoP debuffs
    debuff_t* slowed_casting;
    debuff_t* magic_vulnerability;
    debuff_t* mortal_wounds;
    debuff_t* physical_damage;
    debuff_t* physical_vulnerability;
    debuff_t* ranged_vulnerability;
    debuff_t* weakened_blows;
    debuff_t* weakened_armor;

    // Class specific "general" debuffs
    debuff_t* shattering_throw;
    debuff_t* tooth_and_claw_absorb;
  } debuffs;

  struct gains_t
  {
    gain_t* arcane_torrent;
    gain_t* blessing_of_might;
    gain_t* dark_rune;
    gain_t* energy_regen;
    gain_t* essence_of_the_red;
    gain_t* focus_regen;
    gain_t* glyph_of_innervate;
    gain_t* hymn_of_hope;
    gain_t* innervate;
    gain_t* mana_potion;
    gain_t* mana_spring_totem;
    gain_t* mp5_regen;
    gain_t* restore_mana;
    gain_t* spellsurge;
    gain_t* touch_of_the_grave;
    gain_t* vampiric_embrace;
    gain_t* vampiric_touch;
    gain_t* water_elemental;
  } gains;

  struct procs_t
  {
    proc_t* hat_donor;
    proc_t* parry_haste;
  } procs;

  struct rngs_t
  {
    rng_t* lag_ability;
    rng_t* lag_brain;
    rng_t* lag_channel;
    rng_t* lag_gcd;
    rng_t* lag_queue;
    rng_t* lag_reaction;
    rng_t* lag_world;
  } rngs;

  struct uptimes_t
  {
    uptime_t* primary_resource_cap;
  } uptimes;

  struct racials_t
  {
    const spell_data_t* quickness;
    const spell_data_t* command;
  } racials;

  bool active_during_iteration;
  const spelleffect_data_t* _mastery; // = find_mastery_spell( specialization() ) -> effectN( 1 );

  stopwatch_t event_stopwatch;

  player_t( sim_t* sim, player_e type, const std::string& name, race_e race_e = RACE_NONE );

  virtual ~player_t();

  virtual const char* name() const { return name_str.c_str(); }

  virtual void init();
  virtual void override_talent( std::string override_str );
  virtual void init_meta_gem( gear_stats_t& );
  virtual bool weapon_racial( weapon_t* );
  virtual void init_resources( bool force = false );
  virtual std::string init_use_item_actions( const std::string& append = std::string() );
  virtual std::string init_use_profession_actions( const std::string& append = std::string() );
  virtual std::string init_use_racial_actions( const std::string& append = std::string() );
  virtual std::vector<std::string> get_item_actions();
  virtual std::vector<std::string> get_profession_actions();
  virtual std::vector<std::string> get_racial_actions();
  bool add_action( std::string action, std::string options = "", std::string alist = "default" );
  bool add_action( const spell_data_t* s, std::string options = "", std::string alist = "default" );
  std::string include_default_on_use_items( player_t&, const std::string& exclude_effects );
  std::string include_specific_on_use_item( player_t&, const std::string& effect_names, const std::string& options );



  virtual void init_target();
  void init_character_properties();
  virtual void init_race();
  virtual void init_talents();
  virtual void init_glyphs();
  virtual void replace_spells();
  virtual void init_position();
  virtual void init_professions();
  virtual void init_spells();
  virtual void init_items();
  virtual void init_weapon( weapon_t& );
  virtual void init_base_stats();
  virtual void init_initial_stats();

  virtual void init_defense();
  virtual void create_buffs();
  virtual void init_unique_gear();
  virtual void init_enchant();
  virtual void init_scaling();
  virtual void init_actions() {}
  virtual void init_gains();
  virtual void init_procs();
  virtual void init_uptimes();
  virtual void init_benefits();
  virtual void init_rng();
  virtual void init_stats();
  virtual void register_callbacks() {}

private:
  void _init_actions();

public:

  virtual void reset();
  virtual void combat_begin();
  virtual void combat_end();
  virtual void merge( player_t& other );

  virtual void datacollection_begin();
  virtual void datacollection_end();

  virtual double energy_regen_per_second();
  virtual double focus_regen_per_second();
  virtual double mana_regen_per_second();

  virtual double composite_melee_haste();
  virtual double composite_melee_speed();
  virtual double composite_melee_attack_power();
  virtual double composite_melee_hit();
  virtual double composite_melee_crit();
  virtual double composite_melee_crit_multiplier() { return 1.0; }
  virtual double composite_melee_expertise( weapon_t* w = 0 );

  virtual double composite_spell_haste();//This is the subset of the old_spell_haste that applies to RPPM
  virtual double composite_spell_speed();//This is the old spell_haste and incorporates everything that buffs cast speed
  virtual double composite_spell_power( school_e school );
  virtual double composite_spell_crit();
  virtual double composite_spell_crit_multiplier() { return 1.0; }
  virtual double composite_spell_hit();
  virtual double composite_mastery();
  virtual double composite_mastery_value();

  virtual double composite_armor();
  virtual double composite_armor_multiplier();
  virtual double composite_miss();
  virtual double composite_dodge();
  virtual double composite_parry();
  virtual double composite_block();
  virtual double composite_block_reduction();
  virtual double composite_crit_block();
  virtual double composite_crit_avoidance();

  virtual double composite_attack_power_multiplier();
  virtual double composite_spell_power_multiplier();

  virtual double matching_gear_multiplier( attribute_e /* attr */ ) { return 0; }

  virtual double composite_player_multiplier   ( school_e );
  virtual double composite_player_dd_multiplier( school_e, action_t* /* a */ = NULL ) { return 1; }
  virtual double composite_player_td_multiplier( school_e, action_t* a = NULL );

  virtual double composite_player_heal_multiplier( school_e school );
  virtual double composite_player_dh_multiplier( school_e /* school */ ) { return 1; }
  virtual double composite_player_th_multiplier( school_e school );

  virtual double composite_player_absorb_multiplier( school_e school );

  virtual double composite_mitigation_multiplier( school_e );

  virtual double composite_movement_speed();

  virtual double composite_attribute( attribute_e attr );
  virtual double composite_attribute_multiplier( attribute_e attr );

  double get_attribute( attribute_e a )
  { return util::round( composite_attribute( a ) * composite_attribute_multiplier( a ) ); }

  double strength()  { return get_attribute( ATTR_STRENGTH ); }
  double agility()   { return get_attribute( ATTR_AGILITY ); }
  double stamina()   { return get_attribute( ATTR_STAMINA ); }
  double intellect() { return get_attribute( ATTR_INTELLECT ); }
  double spirit()    { return get_attribute( ATTR_SPIRIT ); }
  double mastery_coefficient() { return _mastery -> mastery_value(); }

  // Stat Caching
  player_stat_cache_t cache;
#ifdef SC_STAT_CACHE
  virtual void invalidate_cache( cache_e c );
#else
  void invalidate_cache( cache_e ) {}
#endif

  virtual void interrupt();
  virtual void halt();
  virtual void moving();
  virtual void stun();
  virtual void clear_debuffs();
  virtual void trigger_ready();
  virtual void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false );
  virtual void arise();
  virtual void demise();
  virtual timespan_t available() { return timespan_t::from_seconds( 0.1 ); }
  action_t* execute_action();

  virtual void   regen( timespan_t periodicity = timespan_t::from_seconds( 0.25 ) );
  virtual double resource_gain( resource_e resource_type, double amount, gain_t* g = 0, action_t* a = 0 );
  virtual double resource_loss( resource_e resource_type, double amount, gain_t* g = 0, action_t* a = 0 );
  virtual void   recalculate_resource_max( resource_e resource_type );
  virtual bool   resource_available( resource_e resource_type, double cost );
  void collect_resource_timeline_information();
  virtual resource_e primary_resource() { return RESOURCE_NONE; }
  virtual role_e   primary_role();
  specialization_e specialization() const { return _spec; }
  const char* primary_tree_name();
  virtual stat_e normalize_by();

  virtual double health_percentage();
  virtual timespan_t time_to_die();
  timespan_t total_reaction_time();

  virtual void stat_gain( stat_e stat, double amount, gain_t* g = 0, action_t* a = 0, bool temporary = false );
  virtual void stat_loss( stat_e stat, double amount, gain_t* g = 0, action_t* a = 0, bool temporary = false );

  virtual void modify_current_rating( rating_e stat, double amount );

  virtual void cost_reduction_gain( school_e school, double amount, gain_t* g = 0, action_t* a = 0 );
  virtual void cost_reduction_loss( school_e school, double amount, action_t* a = 0 );

  virtual void assess_damage( school_e, dmg_e, action_state_t* );
  virtual void target_mitigation( school_e, dmg_e, action_state_t* );

  virtual void assess_heal( school_e, dmg_e, heal_state_t* );

  virtual void  summon_pet( const std::string& name, timespan_t duration = timespan_t::zero() );
  virtual void dismiss_pet( const std::string& name );

  bool is_moving() { return buffs.raid_movement -> check() || buffs.self_movement -> check(); }

  bool parse_talents_numbers( const std::string& talent_string );
  bool parse_talents_armory( const std::string& talent_string );
  bool parse_talents_wowhead( const std::string& talent_string );

  void create_talents_numbers();
  void create_talents_armory();
  void create_talents_wowhead();


  const spell_data_t* find_glyph( const std::string& name ) const;
  const spell_data_t* find_racial_spell( const std::string& name, const std::string& token = std::string(), race_e s = RACE_NONE ) const;
  const spell_data_t* find_class_spell( const std::string& name, const std::string& token = std::string(), specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_pet_spell( const std::string& name, const std::string& token = std::string() ) const;
  const spell_data_t* find_talent_spell( const std::string& name, const std::string& token = std::string(), bool name_tokenized = false, bool check_validity = true ) const;
  const spell_data_t* find_glyph_spell( const std::string& name, const std::string& token = std::string() ) const;
  const spell_data_t* find_specialization_spell( const std::string& name, const std::string& token = std::string(), specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_mastery_spell( specialization_e s, const std::string& token = std::string(), uint32_t idx = 0 ) const;
  const spell_data_t* find_spell( const std::string& name, const std::string& token = std::string(), specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_spell( const unsigned int id, const std::string& token = std::string() ) const;

  virtual expr_t* create_expression( action_t*, const std::string& name );
  expr_t* create_resource_expression( const std::string& name );

  virtual void create_options();
  void recreate_talent_str( talent_format_e format = TALENT_FORMAT_NUMBERS );
  virtual bool create_profile( std::string& profile_str, save_e = SAVE_ALL, bool save_html = false );

  virtual void copy_from( player_t* source );

  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual void      create_pets() { }
  virtual pet_t*    create_pet( const std::string& /* name*/,  const std::string& /* type */ = std::string() ) { return 0; }
  virtual pet_t*    find_pet  ( const std::string& name );

  virtual int decode_set( item_t& item ) { ( void )item; assert( item.name() ); return SET_NONE; }

  virtual void armory_extensions( const std::string& /* region */, const std::string& /* server */, const std::string& /* character */,
                                  cache::behavior_e /* behavior */ = cache::players() )
  {}

  // Class-Specific Methods
  static player_t* create( sim_t* sim, const player_description_t& );

  // Raid-wide aura/buff/debuff maintenance
  static bool init ( sim_t* sim );

  bool is_pet() const { return type == PLAYER_PET || type == PLAYER_GUARDIAN || type == ENEMY_ADD; }
  bool is_enemy() const { return _is_enemy( type ); }
  static bool _is_enemy( player_e t ) { return t == ENEMY || t == ENEMY_ADD; }
  bool is_add() { return type == ENEMY_ADD; }
  static bool _is_sleeping( const player_t* t ) { return t -> current.sleeping; }
  bool is_sleeping() const { return _is_sleeping( this ); }

  pet_t* cast_pet() { return debug_cast<pet_t*>( this ); }

  bool      in_gcd() { return gcd_ready > sim -> current_time; }
  bool      recent_cast();
  item_t*   find_item( const std::string& );
  action_t* find_action( const std::string& );
  bool      dual_wield() { return main_hand_weapon.type != WEAPON_NONE && off_hand_weapon.type != WEAPON_NONE; }
  bool      has_shield_equipped() const
  { return  items[ SLOT_OFF_HAND ].parsed.data.item_subclass == ITEM_SUBCLASS_ARMOR_SHIELD; }


  action_priority_list_t* find_action_priority_list( const std::string& name );
  void                    clear_action_priority_lists() const;

  cooldown_t* find_cooldown( const std::string& name );
  dot_t*      find_dot     ( const std::string& name, player_t* source );
  stats_t*    find_stats   ( const std::string& name );
  gain_t*     find_gain    ( const std::string& name );
  proc_t*     find_proc    ( const std::string& name );
  benefit_t*  find_benefit ( const std::string& name );
  uptime_t*   find_uptime  ( const std::string& name );

  cooldown_t* get_cooldown( const std::string& name );
  dot_t*      get_dot     ( const std::string& name, player_t* source );
  gain_t*     get_gain    ( const std::string& name );
  proc_t*     get_proc    ( const std::string& name );
  stats_t*    get_stats   ( const std::string& name, action_t* action = 0 );
  benefit_t*  get_benefit ( const std::string& name );
  uptime_t*   get_uptime  ( const std::string& name );
  rng_t*      get_rng     ( const std::string& name );
  double      get_player_distance( player_t& );
  double      get_position_distance( double m = 0, double v = 0 );
  action_priority_list_t* get_action_priority_list( const std::string& name, const std::string& comment = std::string() );
  virtual actor_pair_t* get_target_data( player_t* /* target */ )
  { return NULL; }

  // Opportunity to perform any stat fixups before analysis
  virtual void pre_analyze_hook() {}

  /* New stuff */
  virtual double composite_player_vulnerability( school_e );
  virtual double composite_ranged_attack_player_vulnerability();

  virtual void activate_action_list( action_priority_list_t* a, bool off_gcd = false );

  virtual void analyze( sim_t& );

  const extended_sample_data_t& scales_over();

  void change_position( position_e );
  position_e position() const
  { return current.position; }
};

// Target Specific ==========================================================

template < class T >
struct target_specific_t
{
  std::vector<T> data;

  T& operator[]( player_t* target )
  {
    assert( target );
    if ( data.empty() ) data.resize( target -> sim -> actor_list.size() );
    return data[ target -> actor_index ];
  }
};

// Pet ======================================================================

struct pet_t : public player_t
{
  typedef player_t base_t;

  std::string full_name_str;
  player_t* const owner;
  double stamina_per_owner;
  double intellect_per_owner;
  bool summoned;
  pet_e pet_type;
  event_t* expiration;
  timespan_t duration;

  struct owner_coefficients_t
  {
    double armor, health, ap_from_ap, ap_from_sp, sp_from_ap, sp_from_sp;
    owner_coefficients_t();
  } owner_coeff;

private:
  void init_pet_t_();
public:
  pet_t( sim_t* sim, player_t* owner, const std::string& name, bool guardian = false );
  pet_t( sim_t* sim, player_t* owner, const std::string& name, pet_e pt, bool guardian = false );

  virtual void init_base_stats();
  virtual void init_target();
  virtual void reset();
  virtual void summon( timespan_t duration = timespan_t::zero() );
  virtual void dismiss();
  virtual void assess_damage( school_e, dmg_e, action_state_t* s );
  virtual void combat_begin();

  virtual const char* name() const { return full_name_str.c_str(); }

  const spell_data_t* find_pet_spell( const std::string& name, const std::string& token = std::string() );

  virtual double composite_attribute( attribute_e attr );

  // new pet scaling by Ghostcrawler, see http://us.battle.net/wow/en/forum/topic/5889309137?page=49#977
  // http://us.battle.net/wow/en/forum/topic/5889309137?page=58#1143

  double hit_exp();

  virtual double composite_melee_expertise( weapon_t* )
  { return hit_exp(); }
  virtual double composite_melee_hit()
  { return hit_exp(); }
  virtual double composite_spell_hit()
  { return hit_exp() * 2.0; }

  double pet_crit();

  virtual double composite_melee_crit()
  { return pet_crit(); }
  virtual double composite_spell_crit()
  { return pet_crit(); }

  virtual double composite_melee_speed()
  { return owner -> cache.attack_speed(); }

  virtual double composite_melee_haste()
  { return owner -> cache.attack_haste(); }

  virtual double composite_spell_haste()
  { return owner -> cache.spell_haste(); }

  virtual double composite_spell_speed()
  { return owner -> cache.spell_speed(); }

  virtual double composite_melee_attack_power();

  virtual double composite_spell_power( school_e school );

  // Assuming diminishing returns are transfered to the pet as well
  virtual double composite_dodge()
  { return owner -> cache.dodge(); }

  virtual double composite_parry()
  { return owner -> cache.parry(); }

  // Influenced by coefficients [ 0, 1 ]
  virtual double composite_armor()
  { return owner -> cache.armor() * owner_coeff.armor; }

  virtual void init_resources( bool force );
};


// Gain =====================================================================

struct gain_t : public noncopyable
{
public:
  std::array<double, RESOURCE_MAX> actual, overflow, count;
  const std::string name_str;

  gain_t( const std::string& n ) :
    actual(),
    overflow(),
    count(),
    name_str( n )
  { }
  void add( resource_e rt, double amount, double overflow_ = 0.0 )
  { actual[ rt ] += amount; overflow[ rt ] += overflow_; count[ rt ]++; }
  void merge( const gain_t& other )
  {
    for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
    { actual[ i ] += other.actual[ i ]; overflow[ i ] += other.overflow[ i ]; count[ i ] += other.count[ i ]; }
  }
  void analyze( const sim_t& sim )
  {
    for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
    { actual[ i ] /= sim.iterations; overflow[ i ] /= sim.iterations; count[ i ] /= sim.iterations; }
  }
  const char* name() const { return name_str.c_str(); }
};

// Stats ====================================================================

struct stats_t : public noncopyable
{
private:
  sim_t& sim;
public:
  const std::string name_str;
  player_t* player;
  stats_t* parent;
  // We should make school and type const or const-like, and either stricly define when, where and who defines the values,
  // or make sure that it is equal to the value of all it's actions.
  school_e school;
  stats_e type;

  std::vector<action_t*> action_list;
  gain_t resource_gain;
  // Flags
  bool analyzed;
  bool quiet;
  bool background;

  simple_sample_data_t num_executes, num_ticks, num_refreshes, num_direct_results, num_tick_results;
  unsigned int iteration_num_executes, iteration_num_ticks, iteration_num_refreshes;
  // Variables used both during combat and for reporting
  simple_sample_data_t total_execute_time, total_tick_time;
  timespan_t iteration_total_execute_time, iteration_total_tick_time;
  double portion_amount;
  simple_sample_data_t total_intervals;
  timespan_t last_execute;
  extended_sample_data_t actual_amount, total_amount, portion_aps, portion_apse;
  std::vector<stats_t*> children;

  struct stats_results_t
  {
  public:
    simple_sample_data_with_min_max_t actual_amount, avg_actual_amount;
    simple_sample_data_t total_amount, fight_actual_amount, fight_total_amount, overkill_pct;
    simple_sample_data_t count;
    double pct;
  private:
    int iteration_count;
    double iteration_actual_amount, iteration_total_amount;
    friend struct stats_t;
  public:

    stats_results_t();
    void analyze( double num_results );
    void merge( const stats_results_t& other );
    void datacollection_begin();
    void datacollection_end();
  };
  std::vector<stats_results_t> direct_results;
  std::vector<stats_results_t>   tick_results;

  sc_timeline_t timeline_amount;

  // Reporting only
  std::array<double, RESOURCE_MAX> resource_portion, apr, rpe;
  double rpe_sum, compound_amount, overkill_pct;
  double aps, ape, apet, etpe, ttpt;
  timespan_t total_time;
  std::string aps_distribution_chart;

  std::string timeline_aps_chart;

  // Scale factor container
  gear_stats_t scaling;
  gear_stats_t scaling_error;

  stats_t( const std::string& name, player_t* );

  void add_child( stats_t* child );
  void consume_resource( resource_e resource_type, double resource_amount );
  void add_result( double act_amount, double tot_amount, dmg_e dmg_type, result_e result, player_t* target );
  void add_execute( timespan_t time, player_t* target );
  void add_tick   ( timespan_t time, player_t* target );
  void add_refresh( player_t* target );
  void datacollection_begin();
  void datacollection_end();
  void reset();
  void analyze();
  void merge( const stats_t& other );
  const char* name() const { return name_str.c_str(); }
};

// Action ===================================================================

struct action_t : public noncopyable
{
  const spell_data_t* s_data;
  sim_t* const sim;
  const action_e type;
  std::string name_str;
  player_t* const player;
  player_t* target;

  /* Target Cache System
   * - list: contains the cached target pointers
   * - callback: unique_Ptr to callback
   * - is_valid: gets invalidated by the callback from the target list source.
   *  When the target list is requested in action_t::target_list(), it gets recalculated if
   *  flag is false, otherwise cached version is used
   */
  struct target_cache_t {
    std::vector< player_t* > list;
    callback_t* callback;
    bool is_valid;
    target_cache_t() : callback( nullptr ),is_valid( false ) {}
    ~target_cache_t() { delete callback; }
  } target_cache;

  school_e school;

  uint32_t id;
  result_e result;
  resource_e resource_current;
  int aoe, pre_combat;
  // true if this action should not be counted for executes
  bool dual;
  bool callbacks, special, channeled, background, sequence, use_off_gcd, quiet;
  bool direct_tick, direct_tick_callbacks, periodic_hit, repeating, harmful, proc, item_proc, proc_ignores_slot;
  bool discharge_proc, auto_cast, initialized;
  bool may_hit, may_miss, may_dodge, may_parry, may_glance, may_block, may_crush, may_crit;
  bool tick_may_crit, tick_zero, hasted_ticks;
  dot_behavior_e dot_behavior;
  timespan_t ability_lag, ability_lag_stddev;
  double rp_gain;
  timespan_t min_gcd, trigger_gcd;
  double range;
  double weapon_power_mod, direct_power_mod, tick_power_mod;
  timespan_t base_execute_time;
  timespan_t base_tick_time;
  std::array< double, RESOURCE_MAX > base_costs;
  std::array< int, RESOURCE_MAX > costs_per_second;
  double base_dd_min, base_dd_max, base_td;
  double base_dd_multiplier, base_td_multiplier;
  double base_multiplier, base_hit, base_crit;
  double base_spell_power, base_attack_power;
  double base_spell_power_multiplier, base_attack_power_multiplier;
  double crit_multiplier, crit_bonus_multiplier, crit_bonus;
  double base_dd_adder;
  double base_ta_adder;
  double stormlash_da_multiplier, stormlash_ta_multiplier;
  int num_ticks;
  weapon_t* weapon;
  double weapon_multiplier;
  double base_add_multiplier;
  double base_aoe_multiplier; // Static reduction of damage for AoE
  bool split_aoe_damage;
  bool normalize_weapon_speed;
  rng_t* rng_result;
  rng_t* rng_travel;
  cooldown_t* cooldown;
  stats_t* stats;
  event_t* execute_event;
  timespan_t time_to_execute, time_to_travel;
  double travel_speed, resource_consumed;
  int moving, wait_on_ready, interrupt, chain, cycle_targets, max_cycle_targets, target_number;
  bool round_base_dmg;
  std::string if_expr_str;
  expr_t* if_expr;
  std::string interrupt_if_expr_str;
  std::string early_chain_if_expr_str;
  expr_t* interrupt_if_expr;
  expr_t* early_chain_if_expr;
  std::string sync_str;
  action_t* sync_action;
  char marker;
  std::string signature_str;
  std::string target_str;
  std::string label_str;
  timespan_t last_reaction_time;
  target_specific_t<dot_t*> target_specific_dot;
  std::string action_list;
  action_t* tick_action;
  action_t* execute_action;
  action_t* impact_action;
  bool dynamic_tick_action;
  bool special_proc;
  int64_t total_executions;
  cooldown_t line_cooldown;
  const action_priority_t* signature;


  action_t( action_e type, const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  virtual ~action_t();
  void init_dot( const std::string& dot_name );

  const spell_data_t& data() { return ( *s_data ); }
  void parse_spell_data( const spell_data_t& );
  void parse_effect_data( const spelleffect_data_t& );

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost();
  virtual timespan_t gcd();
  virtual timespan_t execute_time() { return base_execute_time; }
  virtual timespan_t tick_time( double haste );
  virtual int    hasted_num_ticks( double haste, timespan_t d = timespan_t::min() );
  virtual timespan_t travel_time();
  virtual result_e calculate_result( action_state_t* /* state */ ) { assert( false ); return RESULT_UNKNOWN; }
  virtual double calculate_direct_amount( action_state_t* state, int chain_target );
  virtual double calculate_tick_amount( action_state_t* state );

  virtual double calculate_weapon_damage( double attack_power );
  virtual double target_armor( player_t* t )
  { return t -> cache.armor(); }
  virtual void   consume_resource();
  virtual resource_e current_resource()
  { return resource_current; }
  virtual void   execute();
  virtual void   tick( dot_t* d );
  virtual void   last_tick( dot_t* d );
  virtual void   assess_damage( dmg_e, action_state_t* assess_state );
  virtual void   schedule_execute( action_state_t* execute_state = 0 );
  virtual void   reschedule_execute( timespan_t time );
  virtual void   update_ready( timespan_t cd_duration = timespan_t::min() );
  virtual bool   usable_moving();
  virtual bool   ready();
  virtual void   init();
  virtual void   init_target_cache();
  virtual void   reset();
  virtual void   cancel();
  virtual void   interrupt_action();
  void   check_spec( specialization_e );
  void   check_spell( const spell_data_t* );
  const char* name() const { return name_str.c_str(); }

  inline bool result_is_hit( result_e r = RESULT_UNKNOWN )
  {
    if ( r == RESULT_UNKNOWN ) r = result;
    return( r == RESULT_HIT        ||
            r == RESULT_CRIT       ||
            r == RESULT_GLANCE     ||
            r == RESULT_BLOCK      ||
            r == RESULT_CRIT_BLOCK ||
            r == RESULT_NONE       );
  }

  inline bool result_is_miss( result_e r = RESULT_UNKNOWN )
  {
    if ( r == RESULT_UNKNOWN ) r = result;
    return( r == RESULT_MISS   ||
            r == RESULT_DODGE  ||
            r == RESULT_PARRY );
  }

  virtual double   miss_chance( double /* hit */, int /* delta_level */ ) { return 0; }
  virtual double  dodge_chance( double /* expertise */, int /* delta_level */ ) { return 0; }
  virtual double  parry_chance( double /* expertise */, int /* delta_level */ ) { return 0; }
  virtual double glance_chance( int /* delta_level */ ) { return 0; }
  virtual double  block_chance( int /* delta_level */ ) { return 0; }
  virtual double   crit_chance( double /* crit */, int /* delta_level */ );

  virtual double total_crit_bonus(); // Check if we want to move this into the stateless system.

  // Some actions require different multipliers for the "direct" and "tick" portions.

  virtual expr_t* create_expression( const std::string& name );

  virtual double ppm_proc_chance( double PPM );
  virtual double real_ppm_proc_chance( double PPM, timespan_t last_proc_attempt, timespan_t last_successful_proc, rppm_scale_e scales_with );

  dot_t* find_dot( player_t* t = nullptr )
  {
    if ( ! t ) t = target;
    if ( ! t ) return nullptr;

    return target_specific_dot[ t ];
  }

  dot_t* get_dot( player_t* t = nullptr )
  {
    if ( ! t ) t = target;
    if ( ! t ) return nullptr;

    dot_t*& dot = target_specific_dot[ t ];
    if ( ! dot ) dot = t -> get_dot( name_str, player );
    return dot;
  }

  void add_child( action_t* child ) { stats -> add_child( child -> stats ); }

  virtual int num_targets();
  virtual size_t available_targets( std::vector< player_t* >& );
  virtual std::vector< player_t* >& target_list();
  virtual player_t* find_target_by_number( int number );

  auto_dispose< std::vector<action_state_t*> > state_cache;
  action_state_t* execute_state; /* State of the last execute() */
  action_state_t* pre_execute_state; /* Optional - if defined before execute(), will be copied in */
  uint32_t snapshot_flags;
  uint32_t update_flags;

  virtual action_state_t* new_state();
  virtual action_state_t* get_state( const action_state_t* = 0 );
private:
  friend struct action_state_t;
  virtual void release_state( action_state_t* );
public:
  virtual void schedule_travel( action_state_t* );
  virtual void impact( action_state_t* );
  virtual void trigger_dot( action_state_t* );

  virtual void snapshot_internal( action_state_t*, uint32_t, dmg_e );
  virtual void   snapshot_state( action_state_t* s, dmg_e rt )
  { snapshot_internal( s, snapshot_flags, rt ); }
  virtual void   update_state( action_state_t* s, dmg_e rt )
  { snapshot_internal( s, update_flags, rt ); }
  virtual void consolidate_snapshot_flags();

  virtual double direct_power_coefficient( const action_state_t* )
  { return direct_power_mod; }
  virtual double tick_power_coefficient( const action_state_t* )
  { return tick_power_mod; }
  virtual double base_da_min( const action_state_t* )
  { return base_dd_min; }
  virtual double base_da_max( const action_state_t* )
  { return base_dd_max; }
  virtual double base_ta( const action_state_t* )
  { return base_td; }
  virtual double bonus_da( const action_state_t* )
  { return base_dd_adder; }
  virtual double bonus_ta( const action_state_t* )
  { return base_ta_adder; }

  virtual double action_multiplier() { return base_multiplier; }
  virtual double action_da_multiplier() { return base_dd_multiplier; }
  virtual double action_ta_multiplier() { return base_td_multiplier; }

  virtual double composite_hit() { return base_hit; }
  virtual double composite_crit() { return base_crit; }
  virtual double composite_crit_multiplier() { return 1.0; }
  virtual double composite_haste() { return 1.0; }
  virtual double composite_attack_power() { return base_attack_power + player -> cache.attack_power(); }
  virtual double composite_spell_power()
  { return base_spell_power + player -> cache.spell_power( school ); }
  virtual double composite_target_crit( player_t* /* target */ );
  virtual double composite_target_multiplier( player_t* target ) { return target -> composite_player_vulnerability( school ); }

  // the direct amount multiplier due to debuffs on the target
  virtual double composite_target_da_multiplier( player_t* target ) { return composite_target_multiplier( target ); }

  // the tick amount multiplier due to debuffs on the target
  virtual double composite_target_ta_multiplier( player_t* target ) { return composite_target_multiplier( target ); }

  virtual double composite_da_multiplier()
  {
    return action_multiplier() * action_da_multiplier() *
           player -> cache.player_multiplier( school ) *
           player -> composite_player_dd_multiplier( school, this );
  }
  virtual double composite_ta_multiplier()
  {
    return action_multiplier() * action_ta_multiplier() *
           player -> cache.player_multiplier( school ) *
           player -> composite_player_td_multiplier( school, this );
  }


  virtual double composite_target_mitigation( player_t* t, school_e s )
  { return t -> composite_mitigation_multiplier( s ); }

  event_t* start_action_execute_event( timespan_t time, action_state_t* execute_state = 0 );

private:
  std::vector<travel_event_t*> travel_events;
public:
  void add_travel_event( travel_event_t* e ) { travel_events.push_back( e ); }
  void remove_travel_event( travel_event_t* e );
  bool has_travel_events() const { return ! travel_events.empty(); }
  bool has_travel_events_for( const player_t* target ) const;
};

struct action_state_t : public noncopyable
{
  // Source action, target actor
  action_t*       action;
  player_t*       target;
  // Results
  dmg_e           result_type;
  result_e        result;
  double          result_amount;
  // Snapshotted stats during execution
  double          haste;
  double          crit;
  double          target_crit;
  double          attack_power;
  double          spell_power;
  // Multipliers
  double          da_multiplier;
  double          ta_multiplier;
  double          target_da_multiplier;
  double          target_ta_multiplier;
  // Target mitigation multipliers
  double          target_mitigation_da_multiplier;
  double          target_mitigation_ta_multiplier;

  static void release( action_state_t*& s ) { s -> action -> release_state( s ); s = 0; }

  action_state_t( action_t*, player_t* );
  virtual ~action_state_t() {}

  virtual void copy_state( const action_state_t* );
  virtual void initialize();

  virtual std::ostringstream& debug_str( std::ostringstream& debug_str );
  virtual void debug();

  virtual double composite_crit()
  { return crit + target_crit; }

  virtual double composite_attack_power()
  { return attack_power * action -> base_attack_power_multiplier; }

  virtual double composite_spell_power()
  { return spell_power * action -> base_spell_power_multiplier; }

  virtual double composite_power()
  { return composite_attack_power() + composite_spell_power(); }

  virtual double composite_da_multiplier()
  { return da_multiplier * target_da_multiplier; }

  virtual double composite_ta_multiplier()
  { return ta_multiplier * target_ta_multiplier; }

  virtual double composite_target_mitigation_da_multiplier()
  { return target_mitigation_da_multiplier; }

  virtual double composite_target_mitigation_ta_multiplier()
  { return target_mitigation_ta_multiplier; }
};

struct heal_state_t : public action_state_t
{
  double total_result_amount;

  heal_state_t( action_t* a, player_t* t ) :
    action_state_t( a, t ), total_result_amount( 0.0 )
  { }

  virtual std::ostringstream& debug_str( std::ostringstream& s )
  { action_state_t::debug_str( s ) << "total_result_amount=" << total_result_amount; return s; }

  virtual void initialize()
  { action_state_t::initialize(); total_result_amount = 0; }

  virtual void copy_state( const action_state_t* o )
  {
    action_state_t::copy_state( o );
    const heal_state_t* h = debug_cast<const heal_state_t*>( o );
    total_result_amount = h -> total_result_amount;
  }
};

// Attack ===================================================================

struct attack_t : public action_t
{
  std::array<double, RESULT_MAX> chances;
  std::array<result_e, RESULT_MAX> results;
  double base_attack_expertise;
  int num_results;
  double attack_table_sum;

  attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Attack Overrides
  virtual timespan_t execute_time();
  virtual void execute();
  void build_table( std::array<double, RESULT_MAX>& chances,
                    std::array<result_e, RESULT_MAX>& results,
                    double miss_chance, double dodge_chance,
                    double parry_chance, double glance_chance,
                    double crit_chance );
  virtual result_e calculate_result( action_state_t* );
  virtual void   init();

  virtual double   miss_chance( double hit, int delta_level );
  virtual double  block_chance( int /* delta_level */ ) { return 0.0; }
  virtual double  crit_block_chance( int delta_level );

  virtual double composite_hit()
  { return action_t::composite_hit() + player -> cache.attack_hit(); }

  virtual double composite_crit()
  { return action_t::composite_crit() + player -> cache.attack_crit(); }

  virtual double composite_crit_multiplier()
  { return action_t::composite_crit_multiplier() * player -> composite_melee_crit_multiplier(); }

  virtual double composite_haste()
  { return action_t::composite_haste() * player -> cache.attack_haste(); }

  virtual double composite_expertise()
  { return base_attack_expertise + player -> cache.attack_expertise(); }

  virtual void reschedule_auto_attack( double old_swing_haste );

  virtual void reset()
  { num_results = 0; attack_table_sum = std::numeric_limits<double>::min(); action_t::reset(); }
};

// Melee Attack ===================================================================

struct melee_attack_t : public attack_t
{
  melee_attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Melee Attack Overrides
  virtual void init();
  virtual double  dodge_chance( double /* expertise */, int delta_level );
  virtual double  parry_chance( double /* expertise */, int delta_level );
  virtual double glance_chance( int delta_level );

};

// Ranged Attack ===================================================================

struct ranged_attack_t : public attack_t
{
  ranged_attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Ranged Attack Overrides
  virtual double  dodge_chance( double /* expertise */, int delta_level );
  virtual double  parry_chance( double /* expertise */, int delta_level );
  virtual double glance_chance( int delta_level );
  virtual double composite_target_multiplier( player_t* target )
  {
    double v = attack_t::composite_target_multiplier( target );
    v *= target -> composite_ranged_attack_player_vulnerability();
    return v;
  }
  virtual void schedule_execute( action_state_t* execute_state = 0 );
};

// Spell Base ====================================================================

struct spell_base_t : public action_t
{
  // special item flags
  bool procs_courageous_primal_diamond;

  spell_base_t( action_e at, const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Spell Base Overrides
  virtual timespan_t gcd();
  virtual timespan_t execute_time();
  virtual timespan_t tick_time( double haste );
  virtual result_e   calculate_result( action_state_t* );
  virtual void   execute();
  virtual void   schedule_execute( action_state_t* execute_state = 0 );

  virtual double composite_crit()
  { return action_t::composite_crit() + player -> cache.spell_crit(); }

  virtual double composite_haste()
  { return action_t::composite_haste() * player -> cache.spell_speed(); }

  virtual double composite_crit_multiplier()
  { return action_t::composite_crit_multiplier() * player -> composite_spell_crit_multiplier(); }

};

// Harmful Spell ====================================================================

struct spell_t : public spell_base_t
{
public:
  spell_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Harmful Spell Overrides
  virtual void   assess_damage( dmg_e, action_state_t* );
  virtual void   execute();
  virtual double miss_chance( double hit, int delta_level );
  virtual void   init();

  virtual double composite_hit()
  { return action_t::composite_hit() + player -> cache.spell_hit(); }
};

// Heal =====================================================================

struct heal_t : public spell_base_t
{
public:
  typedef spell_base_t base_t;
  bool group_only;
  double pct_heal;

  heal_t( const std::string& name, player_t* p, const spell_data_t* s = spell_data_t::nil() );


  virtual void assess_damage( dmg_e, action_state_t* );
  virtual size_t available_targets( std::vector< player_t* >& );
  virtual void init_target_cache();
  virtual double calculate_direct_amount( action_state_t* state, int chain_target );
  virtual void execute();
  player_t* find_greatest_difference_player();
  player_t* find_lowest_player();
  virtual int num_targets();
  virtual void   parse_effect_data( const spelleffect_data_t& );


  virtual double composite_da_multiplier()
  {
    return action_multiplier() * action_da_multiplier() *
           player -> cache.player_heal_multiplier( school ) *
           player -> composite_player_dh_multiplier( school );
  }
  virtual double composite_ta_multiplier()
  {
    return action_multiplier() * action_ta_multiplier() *
           player -> cache.player_heal_multiplier( school ) *
           player -> composite_player_th_multiplier( school );
  }

  virtual action_state_t* new_state()
  { return new heal_state_t( this, target ); }

  virtual expr_t* create_expression( const std::string& name );
};

// Absorb ===================================================================

struct absorb_t : public spell_base_t
{
  absorb_t( const std::string& name, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  virtual void execute();
  virtual void assess_damage( dmg_e, action_state_t* );
  virtual void impact( action_state_t* );
  virtual void init_target_cache();
  virtual size_t available_targets( std::vector< player_t* >& );
  virtual int num_targets();

  virtual double composite_da_multiplier()
  {
    return action_multiplier() * action_da_multiplier() *
           player -> composite_player_absorb_multiplier( school );
  }
  virtual double composite_ta_multiplier()
  {
    return action_multiplier() * action_ta_multiplier() *
           player -> composite_player_absorb_multiplier( school );
  }

  virtual action_state_t* new_state()
  { return new heal_state_t( this, target ); }
};

// Sequence =================================================================

struct sequence_t : public action_t
{
  bool waiting;
  int sequence_wait_on_ready;
  std::vector<action_t*> sub_actions;
  int current_action;
  bool restarted;
  timespan_t last_restart;

  sequence_t( player_t*, const std::string& sub_action_str );

  virtual void schedule_execute( action_state_t* execute_state = 0 );
  virtual void reset();
  virtual bool ready();
  void restart() { current_action = 0; restarted = true; last_restart = sim -> current_time; }
  bool can_restart()
  { return ! restarted && last_restart < sim -> current_time; }
};

// DoT ======================================================================

struct dot_t : public noncopyable
{
private:
  sim_t& sim;
public:
  player_t* const target;
  player_t* const source;
  action_t* current_action;
  action_state_t* state;
  event_t* tick_event;
  int num_ticks, current_tick, added_ticks;
  bool ticking;
  timespan_t added_seconds;
  timespan_t ready;
  timespan_t miss_time;
  timespan_t time_to_tick;
  const std::string name_str;
  double tick_amount;

  dot_t( const std::string& n, player_t* target, player_t* source );

  void   cancel();
  void   extend_duration( int extra_ticks, bool cap = false, uint32_t state_flags = -1 );
  void   extend_duration_seconds( timespan_t extra_seconds, uint32_t state_flags = -1 );
  void   recalculate_ready();
  void   refresh_duration( uint32_t state_flags = -1 );
  void   reset();
  void   last_tick();
  void   schedule_tick();
  int    ticks();
  void   copy( player_t* destination );
  bool   is_higher_priority_action_available();

  timespan_t remains() const
  {
    if ( ! current_action ) return timespan_t::zero();
    if ( ! ticking ) return timespan_t::zero();
    return ready - sim.current_time;
  };

  expr_t* create_expression( action_t* action, const std::string& name_str, bool dynamic );

  const char* name() const { return name_str.c_str(); }
};

// "Real" 'Procs per Minute' helper class =====================================

struct real_ppm_t
{
private:
  rng_t*       rng;
  double       freq;
  timespan_t   last_trigger_attempt;
  timespan_t   last_successful_trigger;
  rppm_scale_e scales_with;
public:
  real_ppm_t( const std::string& name, player_t& p, double frequency = std::numeric_limits<double>::min(), rppm_scale_e s = RPPM_HASTE ) :
    rng( p.get_rng( name ) ),
    freq( frequency ),
    last_trigger_attempt( timespan_t::from_seconds( -10.0 ) ),
    last_successful_trigger( timespan_t::from_seconds( -300.0 ) ), // Assume 5min out of combat before pull
    scales_with( s )
  { }

  void set_frequency( double frequency )
  { freq = frequency; }

  double get_frequency()
  { return freq; }

  void reset()
  {
    last_trigger_attempt = timespan_t::from_seconds( -10.0 );
    last_successful_trigger = timespan_t::from_seconds( -300.0 );
  }

  bool trigger( action_t& a )
  {
    assert( freq != std::numeric_limits<double>::min() && "Real PPM Frequency not set!" );

    if ( last_trigger_attempt == a.sim -> current_time )
      return false;

    double chance = a.real_ppm_proc_chance( freq, last_trigger_attempt, last_successful_trigger, scales_with );
    last_trigger_attempt = a.sim -> current_time;

    bool success = rng -> roll( chance );
    if ( success )
    {
      last_successful_trigger = a.sim -> current_time;
    }
    return success;
  }
};

// Action Callback ==========================================================

struct action_callback_t
{
  player_t* listener;
  bool active;
  bool allow_self_procs;
  bool allow_item_procs;
  bool allow_procs;

  action_callback_t( player_t* l, bool ap = false, bool aip = false, bool asp = false ) :
    listener( l ), active( true ), allow_self_procs( asp ), allow_item_procs( aip ), allow_procs( ap )
  {
    assert( l );
    l -> callbacks.all_callbacks.push_back( this );
  }
  virtual ~action_callback_t() {}
  virtual void trigger( action_t*, void* call_data ) = 0;
  virtual void reset() {}
  virtual void activate() { active = true; }
  virtual void deactivate() { active = false; }

  static void trigger( const std::vector<action_callback_t*>& v, action_t* a, void* call_data = nullptr )
  {
    if ( a && ! a -> player -> in_combat ) return;

    std::size_t size = v.size();
    for ( std::size_t i = 0; i < size; i++ )
    {
      action_callback_t* cb = v[ i ];
      if ( cb -> active )
      {
        if ( ! cb -> allow_item_procs && a && a -> item_proc ) return;
        if ( ! cb -> allow_procs && a && a -> proc ) return;
        cb -> trigger( a, call_data );
      }
    }
  }

  static void reset( const std::vector<action_callback_t*>& v )
  {
    std::size_t size = v.size();
    for ( std::size_t i = 0; i < size; i++ )
    {
      v[ i ] -> reset();
    }
  }
};

// Generic proc callback ======================================================

template<typename T_CALLDATA>
struct proc_callback_t : public action_callback_t
{
  special_effect_t proc_data;
  real_ppm_t       rppm;
  cooldown_t*      cooldown;
  rng_t*           proc_rng;

  struct delay_event_t : public event_t
  {
    proc_callback_t& callback;
    action_t* action;
    T_CALLDATA* call_data;

    delay_event_t( player_t& p, proc_callback_t& cb, action_t* a, T_CALLDATA* cd ) :
      event_t( &p, "proc_callback_delay" ),
      callback( cb ), action( a ), call_data( cd )
    {
      timespan_t delay = sim.gauss( sim.default_aura_delay, sim.default_aura_delay_stddev );
      sim.add_event( this, delay );
    }

    virtual void execute()
    {
      callback.execute( action, call_data );
    }
  };

  proc_callback_t( player_t* p, const special_effect_t& data ) :
    action_callback_t( p ),
    proc_data( data ),
    rppm( proc_data.name_str, *listener, is_rppm() ? std::fabs( data.ppm ) : std::numeric_limits<double>::min(), data.rppm_scale ),
    cooldown( nullptr ), proc_rng( nullptr )
  {
    if ( proc_data.cooldown != timespan_t::zero() )
    {
      cooldown = listener -> get_cooldown( proc_data.name_str );
      cooldown -> duration = proc_data.cooldown;
    }

    if ( is_ppm() || proc_data.proc_chance > 0 )
      proc_rng = listener -> get_rng( proc_data.name_str );
  }

private:
  // Execute should be doing all the proc mechanisms, trigger is for triggering
  virtual void execute( action_t* a, T_CALLDATA* call_data ) = 0;
public:
  virtual void trigger( action_t* action, void* call_data )
  {
    if ( cooldown && cooldown -> down() ) return;

    bool triggered = false;
    double chance = proc_chance();

    if ( chance == 0 ) return;

    if ( is_rppm() )
    {
      rppm.set_frequency( chance );
      triggered = rppm.trigger( *action );
    }
    else if ( is_ppm() )
      triggered = proc_rng -> roll( action -> ppm_proc_chance( chance ) );
    else if ( chance > 0 )
      triggered = proc_rng -> roll( chance );

    if ( triggered )
    {
      T_CALLDATA* arg = static_cast<T_CALLDATA*>( call_data );

      if ( proc_data.proc_delay )
        new ( *listener -> sim ) delay_event_t( *listener, *this, action, arg );
      else
        execute( action, arg );

      if ( cooldown ) cooldown -> start();
    }
  }

  bool is_rppm() { return proc_data.ppm < 0; }
  bool is_ppm() { return proc_data.ppm > 0; }

  virtual double proc_chance()
  {
    if ( is_rppm() )
      return std::fabs( proc_data.ppm );
    else if ( is_ppm() )
      return proc_data.ppm;
    else if ( proc_data.proc_chance > 0 )
      return proc_data.proc_chance;

    return 0;
  }

  void reset()
  {
    action_callback_t::reset();
    if ( cooldown ) cooldown -> reset( false );
    rppm.reset();
  }
};

template <typename T_BUFF, typename T_CALLDATA = action_state_t>
struct buff_proc_callback_t : public proc_callback_t<T_CALLDATA>
{
  struct tick_stack_t : public event_t
  {
    buff_proc_callback_t* callback;

    tick_stack_t( player_t* p, buff_proc_callback_t* cb, timespan_t initial_delay = timespan_t::zero() ) :
      event_t( p, "cb_tick_stack" ), callback( cb )
    { sim.add_event( this, callback -> proc_data.tick + initial_delay ); }

    virtual void execute()
    {
      if ( callback -> buff -> current_stack > 0 &&
           callback -> buff -> current_stack < callback -> buff -> max_stack() )
      {
        callback -> buff -> bump();
        new ( sim ) tick_stack_t( player, callback );
      }
    }
  };

  T_BUFF* buff;

  buff_proc_callback_t( player_t* p, const special_effect_t& data, T_BUFF* b = nullptr ) :
    proc_callback_t<T_CALLDATA>( p, data ), buff( b )
  {
    if ( this -> proc_data.max_stacks == 0 ) this -> proc_data.max_stacks = 1;
    if ( this -> proc_data.proc_chance == 0 ) this -> proc_data.proc_chance = 1;
  }

  void execute( action_t* action, T_CALLDATA* /* call_data */ )
  {
    assert( buff );
    buff -> trigger( this -> proc_data.reverse ? this -> proc_data.max_stacks : 1 );

    if ( this -> proc_data.tick != timespan_t::zero() ) // The buff stacks over time.
    {
      timespan_t initial_delay;
      if ( buff -> delay )
        initial_delay = buff -> delay -> remains();
      new ( *this -> listener -> sim ) tick_stack_t( action -> player, this, initial_delay );
    }
  }
};

template <typename T_ACTION, typename T_CALLDATA = action_state_t>
struct discharge_proc_t : public proc_callback_t<T_CALLDATA>
{
  int       discharge_stacks;
  T_ACTION* discharge_action;
  proc_t*   discharge_proc;

  discharge_proc_t( player_t* p, const special_effect_t& data, T_ACTION* a ) :
    proc_callback_t<T_CALLDATA>( p, data ),
    discharge_stacks( 0 ), discharge_action( a ),
    discharge_proc( proc_callback_t<T_CALLDATA>::listener -> get_proc( data.name_str ) )
  {
    // Discharge Procs have a delay by default
    this -> proc_data.proc_delay = true;
  }

  void reset()
  {
    proc_callback_t<T_CALLDATA>::reset();
    discharge_stacks = 0;
  }

  void execute( action_t* action, T_CALLDATA* /* call_data */ )
  {
    assert( discharge_action );
    if ( ++discharge_stacks >= this -> proc_data.max_stacks )
    {
      discharge_stacks = 0;
      if ( this -> listener -> sim -> debug )
        this -> listener -> sim -> output( "%s procs %s", action -> name(), discharge_action -> name() );
      discharge_action -> execute();
      discharge_proc -> occur();
    }
  }
};

// Action Priority List =====================================================

struct action_priority_t
{
  std::string action_;
  std::string comment_;

  action_priority_t( const std::string& a, const std::string& c ) :
    action_( a ), comment_( c )
  { }

  action_priority_t* comment( const std::string& c )
  { comment_ = c; return this; }
};

struct action_priority_list_t
{
  std::string name_str;
  std::string action_list_comment_str;
  std::string action_list_str;
  std::vector<action_priority_t> action_list;
  player_t* player;
  bool used;
  std::vector<action_t*> foreground_action_list;
  std::vector<action_t*> off_gcd_actions;
  action_priority_list_t( std::string name, player_t* p, const std::string& list_comment = std::string() ) :
    name_str( name ), action_list_comment_str( list_comment ), player( p ), used( false ),
    foreground_action_list( 0 ), off_gcd_actions( 0 )
  { }

  action_priority_t* add_action( const std::string& action_priority_str, const std::string& comment = std::string() );
  action_priority_t* add_action( const player_t* p, const spell_data_t* s, const std::string& action_name,
                                 const std::string& action_options = std::string(), const std::string& comment = std::string() );
  action_priority_t* add_action( const player_t* p, const std::string& name, const std::string& action_options = std::string(),
                                 const std::string& comment = std::string() );
  action_priority_t* add_talent( const player_t* p, const std::string& name, const std::string& action_options = std::string(),
                                 const std::string& comment = std::string() );
};

struct travel_event_t : public event_t
{
  action_t* action;
  action_state_t* state;
  travel_event_t( action_t* a, action_state_t* state, timespan_t time_to_travel );
  virtual ~travel_event_t() { if ( unlikely( state && canceled ) ) action_state_t::release( state ); }
  virtual void execute();
};

// Item database ============================================================

namespace item_database
{
bool     download_slot(      item_t& item );
bool     download_item(      item_t& item );
bool     download_glyph(     player_t* player, std::string& glyph_name, const std::string& glyph_id );
unsigned parse_gem(          item_t& item, unsigned gem_id );
bool     initialize_item_sources( item_t& item, std::vector<std::string>& source_list );

int      random_suffix_type( item_t& item );
int      random_suffix_type( const item_data_t* );
uint32_t armor_value(        item_t& item );
uint32_t armor_value(        const item_data_t*, const dbc_t&, unsigned item_level = 0 );
// Uses weapon's own (upgraded) ilevel to calculate the damage
uint32_t weapon_dmg_min(     item_t& item );
// Allows custom ilevel to be specified
uint32_t weapon_dmg_min(     const item_data_t*, const dbc_t&, unsigned item_level = 0 );
uint32_t weapon_dmg_max(     item_t& item );
uint32_t weapon_dmg_max(     const item_data_t*, const dbc_t&, unsigned item_level = 0 );

bool     load_item_from_data( item_t& item );
bool     parse_gems(          item_t&      item );

// Parse anything relating to the use of ItemSpellEnchantment.dbc. This includes
// enchants, and engineering addons.
bool     parse_item_spell_enchant( item_t& item, std::vector<stat_pair_t>& stats, special_effect_t& effect, unsigned enchant_id );

std::string stat_to_str( int stat, int stat_amount );

// Stat scaling methods for items, or item stats
double approx_scale_coefficient( unsigned current_ilevel, unsigned new_ilevel );
int scaled_stat( const item_data_t& item, const dbc_t& dbc, size_t idx, unsigned new_ilevel );

unsigned upgrade_ilevel( const item_data_t& item, unsigned upgrade_level );
stat_pair_t item_enchantment_effect_stats( const item_enchantment_data_t& enchantment, int index );
};

// Unique Gear ==============================================================

namespace unique_gear
{
void init( player_t* );

action_callback_t* register_stat_proc( player_t*, special_effect_t& );
action_callback_t* register_cost_reduction_proc( player_t*, special_effect_t& );

action_callback_t* register_discharge_proc( player_t*, special_effect_t& );

action_callback_t* register_chance_discharge_proc( player_t*, special_effect_t& );

action_callback_t* register_stat_discharge_proc( player_t*, special_effect_t& );

action_callback_t* register_discharge_proc( item_t&, special_effect_t& );
action_callback_t* register_chance_discharge_proc( item_t&, special_effect_t& );
action_callback_t* register_stat_discharge_proc( item_t&, special_effect_t& );

bool get_equip_encoding( std::string& encoding,
                         const std::string& item_name,
                         bool heroic,
                         bool lfr,
                         bool thunderforged,
                         bool ptr,
                         unsigned item_id = 0 );

bool get_use_encoding( std::string& encoding,
                       const std::string& item_name,
                       bool heroic,
                       bool lfr,
                       bool thunderforged,
                       bool ptr,
                       unsigned item_id = 0 );

void initialize_special_effects( player_t* player );
};

// Consumable ===============================================================

namespace consumable
{
action_t* create_action( player_t*, const std::string& name, const std::string& options );
}

// Report ===================================================================

namespace report
{
// In the end we will restore the simplicity of the code structure, but for now these are duplicated here.
void print_spell_query( sim_t*, unsigned level );
void print_profiles( sim_t* );
void print_text( FILE*, sim_t*, bool detail );
void print_suite( sim_t* );
};

// Wowhead  =================================================================

namespace wowhead
{
enum wowhead_e
{
  LIVE,
  PTR
};

bool download_slot( item_t&, wowhead_e source = LIVE,
                    cache::behavior_e b = cache::items() );
bool download_item( item_t&, wowhead_e source = LIVE, cache::behavior_e b = cache::items() );
bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id,
                     wowhead_e source = LIVE, cache::behavior_e b = cache::items() );
gem_e parse_gem( item_t& item, unsigned gem_id,
                 wowhead_e source = LIVE, cache::behavior_e b = cache::items() );
bool download_item_data( item_t&            item,
                         cache::behavior_e  caching,
                         wowhead_e          source );
}

// CharDev  =================================================================

namespace chardev
{
player_t* download_player( sim_t* sim, const std::string& id, cache::behavior_e b = cache::players() );
}

// Rawr =====================================================================

namespace rawr
{
player_t* load_player( sim_t*, const std::string& character_filename );
player_t* load_player( sim_t*, const std::string& character_filename, const std::string& character_xml );
}

// Blizzard Community Platform API ==========================================

namespace bcp_api
{
bool download_guild( sim_t* sim,
                     const std::string& region,
                     const std::string& server,
                     const std::string& name,
                     const std::vector<int>& ranks,
                     int player_e = PLAYER_NONE,
                     int max_rank = 0,
                     cache::behavior_e b = cache::players() );

player_t* download_player( sim_t*,
                           const std::string& region,
                           const std::string& server,
                           const std::string& name,
                           const std::string& talents = std::string( "active" ),
                           cache::behavior_e b = cache::players() );

player_t* from_local_json( sim_t*,
                           const std::string&,
                           const std::string&,
                           const std::string&
                         );

bool download_item( item_t&, cache::behavior_e b = cache::items() );

bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id,
                     cache::behavior_e b = cache::items() );

bool download_slot( item_t& item, cache::behavior_e b = cache::items() );
gem_e parse_gem( item_t& item, unsigned gem_id, cache::behavior_e b = cache::items() );
}

// Wowreforge ===============================================================

namespace wowreforge
{
player_t* download_player( sim_t* sim, const std::string& id, cache::behavior_e b = cache::players() );
}

// HTTP Download  ===========================================================

namespace http
{
struct proxy_t
{
  std::string type;
  std::string host;
  int port;
};
void set_proxy( const std::string& type, const std::string& host, const unsigned port );

void cache_load( const char* file_name );
void cache_save( const char* file_name );
bool clear_cache( sim_t*, const std::string& name, const std::string& value );

bool get( std::string& result, const std::string& url, cache::behavior_e b,
          const std::string& confirmation = std::string() );
}

// XML ======================================================================

struct xml_parm_t
{
  std::string name_str;
  std::string value_str;
  xml_parm_t( const std::string& n, const std::string& v ) : name_str( n ), value_str( v ) {}
  const char* name() { return name_str.c_str(); }
};

struct xml_node_t
{
  std::string name_str;
  std::vector<xml_node_t*> children;
  std::vector<xml_parm_t> parameters;
  ~xml_node_t();
  xml_node_t() {}
  xml_node_t( const std::string& n ) : name_str( n ) {}
  const char* name() { return name_str.c_str(); }
  xml_node_t* get_child( const std::string& name );
  xml_node_t* get_node ( const std::string& path );
  xml_node_t* get_node ( const std::string& path, const std::string& parm_name, const std::string& parm_value );
  int  get_children( std::vector<xml_node_t*>&, const std::string& name = std::string() );
  int  get_nodes   ( std::vector<xml_node_t*>&, const std::string& path );
  int  get_nodes   ( std::vector<xml_node_t*>&, const std::string& path, const std::string& parm_name, const std::string& parm_value );
  bool get_value( std::string& value, const std::string& path = std::string() );
  bool get_value( int&         value, const std::string& path = std::string() );
  bool get_value( double&      value, const std::string& path = std::string() );
  xml_parm_t* get_parm( const std::string& parm_name );

  xml_node_t* create_node     ( sim_t* sim, const std::string& input, std::string::size_type& index );
  int         create_children ( sim_t* sim, const std::string& input, std::string::size_type& index );
  void        create_parameter( const std::string& input, std::string::size_type& index );

  xml_node_t* search_tree( const std::string& node_name );
  xml_node_t* search_tree( const std::string& node_name, const std::string& parm_name, const std::string& parm_value );
  xml_node_t* split_path ( std::string& key, const std::string& path );

  void print( FILE* f = 0, int spacing = 0 );
  void print_xml( FILE* f = 0, int spacing = 0 );
  static xml_node_t* get( sim_t* sim, const std::string& url, cache::behavior_e b,
                          const std::string& confirmation = std::string() );
  static xml_node_t* create( sim_t* sim, const std::string& input );
  static xml_node_t* create( sim_t* sim, FILE* input );

  xml_node_t* add_child( const std::string& name );
  template <typename T>
  void add_parm( const std::string& name, const T& value )
  {
    std::ostringstream s;
    s << value;
    parameters.push_back( xml_parm_t( name, s.str() ) );
  }
};


// Java Script ==============================================================

namespace js
{
js_node_t* get_child( js_node_t* root, const std::string& name );
js_node_t* get_node ( js_node_t* root, const std::string& path );
int  get_children( std::vector<js_node_t*>&, js_node_t* root );
int  get_value( std::vector<std::string>& value, js_node_t* root, const std::string& path = std::string() );
bool get_value( std::string& value, js_node_t* root, const std::string& path = std::string() );
bool get_value( int&         value, js_node_t* root, const std::string& path = std::string() );
bool get_value( unsigned&    value, js_node_t* root, const std::string& path = std::string() );
bool get_value( double&      value, js_node_t* root, const std::string& path = std::string() );
js_node_t* create( sim_t* sim, const std::string& input );
js_node_t* create( sim_t* sim, FILE* input );
void print( js_node_t* root, FILE* f = 0, int spacing = 0 );
const char* get_name( js_node_t* root );
void delete_node( js_node_t* root );
};

// Handy Actions ============================================================

struct wait_action_base_t : public action_t
{
  wait_action_base_t( player_t* player, const std::string& name ) :
    action_t( ACTION_OTHER, name, player )
  { trigger_gcd = timespan_t::zero(); }

  virtual void execute()
  { player -> iteration_waiting_time += time_to_execute; }
};

// Wait For Cooldown Action =================================================

struct wait_for_cooldown_t : public wait_action_base_t
{
  cooldown_t* wait_cd;
  action_t* a;
  wait_for_cooldown_t( player_t* player, const std::string& cd_name );
  virtual bool usable_moving() { return a -> usable_moving(); }
  virtual timespan_t execute_time();
};

namespace ignite {

// Template for a ignite like action. Not mandatory, but encouraged to use it.

// It makes sure that the travel time corresponds to our ignite model,
// and sets various flags so as to not further modify the damage with which it gets executed

template <class Base>
struct pct_based_action_t : public Base
{
private:
  typedef Base ab; // typedef for the templated action type, spell_t, or heal_t
public:
  typedef pct_based_action_t base_t;

  template <typename T>
  pct_based_action_t( const std::string& n, T& p, const spell_data_t* s ) :
    ab( n, p, s )
  {
    ab::background = true;

    ab::tick_may_crit = false;
    ab::hasted_ticks  = false;
    ab::may_crit = false;
    ab::tick_power_mod = 0;
    ab::dot_behavior  = DOT_REFRESH;
  }

  virtual void impact( action_state_t* s )
  {
    double saved_impact_dmg = s -> result_amount;
    s -> result_amount = 0;
    ab::impact( s );

    dot_t* dot = ab::get_dot( s -> target );
    assert( dot -> ticking && dot -> ticks() && "No dot has been triggered for ignite_pct_based_action!" );
    ab::base_td = saved_impact_dmg / dot -> ticks();
  }

  virtual timespan_t travel_time()
  {
    // Starts directly after ignite amount is calculated
    return timespan_t::zero();
  }

  virtual void execute()
  { assert( 0 ); }

  void init()
  {
    ab::init();

    ab::update_flags = ab::snapshot_flags = 0;
  }

  virtual double composite_ta_multiplier() { return 1.0; } // stateless
};

// This is a template for Ignite like mechanics, like of course Ignite, Hunter Piercing Shots, Priest Echo of Light, etc.
// It should get specialized in the class module
void trigger_pct_based( action_t* ignite_action,
                        player_t* t,
                        double dmg );

}  // namespace ignite

template <typename Action>
struct residual_dot_action : public Action
{
  typedef residual_dot_action< Action > residual_dot_action_t;

  template <typename Player>
  residual_dot_action( const std::string& n, Player& p, const spell_data_t* s ) :
    Action( n, p, s )
  {
    Action::background = true;
    Action::tick_may_crit = false;
    Action::hasted_ticks  = false;
    Action::may_crit = false;
    Action::tick_power_mod = 0;
    Action::dot_behavior  = DOT_REFRESH;
  }

  virtual void execute() { assert( 0 ); }

  // Never need to update the state.
  virtual void snapshot_state( action_state_t*, dmg_e ) { }
  virtual void update_state( action_state_t*, dmg_e ) { }

  virtual double calculate_tick_amount( action_state_t* s )
  { return s -> result_amount = Action::get_dot( s -> target ) -> tick_amount; }

  virtual void impact( action_state_t* s )
  {
    // Amortize damage AFTER dot initialized so we know how many ticks. Only works if tick_zero=false.
    assert( ! Action::tick_zero );
    dot_t* dot = Action::get_dot( s -> target );
    if ( dot -> ticking )
      dot -> tick_amount *= dot -> ticks();
    else
      dot -> tick_amount = 0;
    dot -> tick_amount += s -> result_amount;
    Action::trigger_dot( s );
    dot -> tick_amount /= dot -> ticks();
  }

  virtual timespan_t travel_time()
  {
    return Action::sim -> gauss( Action::sim -> default_aura_delay,
                                 Action::sim -> default_aura_delay_stddev );
  }

  void trigger( player_t* t, double amount )
  {
    action_state_t* s = Action::get_state();
    s -> result = RESULT_HIT;
    s -> result_amount = amount;
    s -> target = t;
    Action::schedule_travel( s );
  }
};

// Inlines ==================================================================

// buff_t inlines

inline buff_t* buff_t::find( sim_t* s, const std::string& name )
{
  return find( s -> buff_list, name );
}
inline buff_t* buff_t::find( player_t* p, const std::string& name, player_t* source )
{
  return find( p -> buff_list, name, source );
}
inline std::string buff_t::source_name() const
{
  if ( player ) return player -> name_str;
  return "noone";
}
// sim_t inlines

inline double sim_t::real()                { return default_rng() -> real(); }
inline bool   sim_t::roll( double chance ) { return default_rng() -> roll( chance ); }
inline double sim_t::range( double min, double max )
{
  return default_rng() -> range( min, max );
}
inline double sim_t::averaged_range( double min, double max )
{
  if ( average_range ) return ( min + max ) / 2.0;

  return default_rng() -> range( min, max );
}

inline buff_creator_t::operator buff_t* () const
{ return new buff_t( *this ); }

inline stat_buff_creator_t::operator stat_buff_t* () const
{ return new stat_buff_t( *this ); }

inline absorb_buff_creator_t::operator absorb_buff_t* () const
{ return new absorb_buff_t( *this ); }

inline cost_reduction_buff_creator_t::operator cost_reduction_buff_t* () const
{ return new cost_reduction_buff_t( *this ); }

inline haste_buff_creator_t::operator haste_buff_t* () const
{ return new haste_buff_t( *this ); }

inline tick_buff_creator_t::operator tick_buff_t* () const
{ return new tick_buff_t( *this ); }

inline buff_creator_t::operator debuff_t* () const
{ return new debuff_t( *this ); }


/* Simple String to Number function, using stringstream
 * This will NOT translate all numbers in the string to a number,
 * but stops at the first non-numeric character.
 */
template <typename T>
T util::str_to_num ( const std::string& text )
{
  std::istringstream ss( text );
  T result;
  return ss >> result ? result : T();
}

// ability_rank =====================================================
template <typename T>
T util::ability_rank( int player_level,
                      T   ability_value,
                      int ability_level, ... )
{
  va_list vap;
  va_start( vap, ability_level );

  while ( player_level < ability_level )
  {
    ability_value = va_arg( vap, T );
    ability_level = va_arg( vap, int );
  }

  va_end( vap );

  return ability_value;
}

// NEW ITEM STUFF REMOVED IN r<xxxx>

#endif // SIMULATIONCRAFT_H

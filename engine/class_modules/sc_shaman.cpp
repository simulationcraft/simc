// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player/pet_spawner.hpp"
#include "action/action_callback.hpp"
#include "report/highchart.hpp"

#include "simulationcraft.hpp"

#include "util/string_view.hpp"

#include <cassert>
#include <regex>
#include <string>

// ==========================================================================
// Shaman
// ==========================================================================

// Dragonflight TODO
//
// Elemental
// - Liquid Magma Totem: Randomize target
// - Inundate
//
// Enhancement
// - Review target caps

// Resto DPS?

namespace
{  // UNNAMED NAMESPACE

struct shaman_t;

enum class mw_proc_state
{
  DEFAULT,
  ENABLED,
  DISABLED
};

enum wolf_type_e
{
  SPIRIT_WOLF = 0,
  FIRE_WOLF,
  FROST_WOLF,
  LIGHTNING_WOLF,
  UNSPECIFIED
};

enum class feral_spirit_cast : unsigned
{
  NORMAL = 0U,
  TIER28,
  TIER31, // .. and Dragonflight Season 4
  ROLLING_THUNDER
};

enum class elemental
{
  GREATER_FIRE,
  PRIMAL_FIRE,
  GREATER_STORM,
  PRIMAL_STORM,
  GREATER_EARTH,
  PRIMAL_EARTH
};

enum class elemental_variant
{
  GREATER,
  LESSER
};

enum class spell_variant : unsigned
{
  NORMAL = 0,
  ASCENDANCE,
  DEEPLY_ROOTED_ELEMENTS,
  PRIMORDIAL_WAVE,
  THORIMS_INVOCATION,
  FUSION_OF_ELEMENTS
};

enum class strike_variant : unsigned
{
  NORMAL = 0,
  STORMFLURRY,
  WHIRLING_AIR
};

enum class ancestor_cast : unsigned
{
  LAVA_BURST = 0,
  CHAIN_LIGHTNING,
  ELEMENTAL_BLAST,
  DISABLED
};

enum imbue_e
{
  IMBUE_NONE = 0,
  FLAMETONGUE_IMBUE,
  WINDFURY_IMBUE,
  EARTHLIVING_IMBUE,
  THUNDERSTRIKE_WARD
};

enum rotation_type_e
{
  ROTATION_INVALID,
  ROTATION_STANDARD,
  ROTATION_SIMPLE,
  ROTATION_FUNNEL
};

static std::vector<std::pair<util::string_view, rotation_type_e>> __rotation_options = {
  { "simple",   ROTATION_SIMPLE   },
  { "standard", ROTATION_STANDARD },
  { "funnel",   ROTATION_FUNNEL   },
};

static rotation_type_e parse_rotation( util::string_view rotation_str )
{
  auto it = range::find_if( __rotation_options, [ rotation_str ]( const auto& entry ) {
    return util::str_compare_ci( entry.first, rotation_str );
  } );

  if ( it != __rotation_options.end() )
  {
    return it->second;
  }

  return ROTATION_INVALID;
}

static std::string rotation_options()
{
  std::vector<util::string_view> opts;
  range::for_each( __rotation_options, [ &opts ]( const auto& entry ) {
    opts.emplace_back( entry.first );
  } );

  return util::string_join( opts, ", " );
}

/**
  Check_distance_targeting is only called when distance_targeting_enabled is true. Otherwise,
  available_targets is called.  The following code is intended to generate a target list that
  properly accounts for range from each target during chain lightning.  On a very basic level, it
  starts at the original target, and then finds a path that will hit 4 more, if possible.  The
  code below randomly cycles through targets until it finds said path, or hits the maximum amount
  of attempts, in which it gives up and just returns the current best path.  I wouldn't be
  terribly surprised if Blizz did something like this in game.


  TODO: Electrified Shocks?
**/
static std::vector<player_t*>& __check_distance_targeting( const action_t* action, std::vector<player_t*>& tl )
{
  sim_t* sim = action->sim;
  if ( !sim->distance_targeting_enabled )
  {
    return tl;
  }

  player_t* target = action->target;
  player_t* player = action->player;
  double radius    = action->radius;
  int aoe          = action->aoe;

  player_t* last_chain;  // We have to track the last target that it hit.
  last_chain = target;
  std::vector<player_t*>
      best_so_far;  // Keeps track of the best chain path found so far, so we can use it if we give up.
  std::vector<player_t*> current_attempt;
  best_so_far.push_back( last_chain );
  current_attempt.push_back( last_chain );

  size_t num_targets  = sim->target_non_sleeping_list.size();
  size_t max_attempts = static_cast<size_t>(
      std::min( ( num_targets - 1.0 ) * 2.0, 30.0 ) );  // With a lot of targets this can get pretty high. Cap it at 30.
  size_t local_attempts = 0;
  size_t attempts = 0;
  size_t chain_number = 1;
  std::vector<player_t*> targets_left_to_try(
      sim->target_non_sleeping_list.data() );  // This list contains members of a vector that haven't been tried yet.
  auto position = std::find( targets_left_to_try.begin(), targets_left_to_try.end(), target );
  if ( position != targets_left_to_try.end() )
    targets_left_to_try.erase( position );

  std::vector<player_t*> original_targets(
      targets_left_to_try );  // This is just so we don't have to constantly remove the original target.

  bool stop_trying = false;

  while ( !stop_trying )
  {
    local_attempts = 0;
    attempts++;
    if ( attempts >= max_attempts )
      stop_trying = true;
    while ( !targets_left_to_try.empty() && local_attempts < num_targets * 2 )
    {
      player_t* possibletarget;
      size_t rng_target = static_cast<size_t>(
          sim->rng().range( 0.0, ( static_cast<double>( targets_left_to_try.size() ) - 0.000001 ) ) );
      possibletarget = targets_left_to_try[ rng_target ];

      double distance_from_last_chain = last_chain->get_player_distance( *possibletarget );
      if ( distance_from_last_chain <= radius + possibletarget->combat_reach )
      {
        last_chain = possibletarget;
        current_attempt.push_back( last_chain );
        targets_left_to_try.erase( targets_left_to_try.begin() + rng_target );
        chain_number++;
      }
      else
      {
        // If there is no hope of this target being chained to, there's no need to test it again
        // for other possibilities.
        if ( distance_from_last_chain > ( ( radius + possibletarget->combat_reach ) * ( aoe - chain_number ) ) )
          targets_left_to_try.erase( targets_left_to_try.begin() + rng_target );
        local_attempts++;  // Only count failures towards the limit-cap.
      }
      // If we run out of targets to hit, or have hit 5 already. Break.
      if ( static_cast<int>( current_attempt.size() ) == aoe || current_attempt.size() == num_targets )
      {
        stop_trying = true;
        break;
      }
    }
    if ( current_attempt.size() > best_so_far.size() )
      best_so_far = current_attempt;

    current_attempt.clear();
    current_attempt.push_back( target );
    last_chain          = target;
    targets_left_to_try = original_targets;
    chain_number        = 1;
  }

  if ( sim->log )
    sim->out_debug.printf( "%s Total attempts at finding path: %.3f - %.3f targets found - %s target is first chain",
                           player->name(), static_cast<double>( attempts ), static_cast<double>( best_so_far.size() ),
                           target->name() );
  tl.swap( best_so_far );
  return tl;
}

static std::string elemental_name( elemental type, elemental_variant variant )
{
  std::string name_;

  switch ( variant )
  {
    case elemental_variant::LESSER:
      name_ += "lesser_";
      break;
    default:
      break;
  }

  switch ( type )
  {
    case elemental::GREATER_FIRE:
      name_ += "fire_elemental";
      break;
    case elemental::PRIMAL_FIRE:
      name_ += "primal_fire_elemental";
      break;
    case elemental::GREATER_STORM:
      name_ += "storm_elemental";
      break;
    case elemental::PRIMAL_STORM:
      name_ += "primal_storm_elemental";
      break;
    case elemental::GREATER_EARTH:
      name_ += "earth_elemental";
      break;
    case elemental::PRIMAL_EARTH:
      name_ += "primal_earth_elemental";
      break;
    default:
      assert( 0 );
  }

  return name_;
}

static bool is_pet_elemental( elemental type )
{
  return type == elemental::PRIMAL_FIRE || type == elemental::PRIMAL_STORM || type == elemental::PRIMAL_EARTH;
}

static bool elemental_autoattack( elemental type )
{
  return type == elemental::PRIMAL_EARTH || type == elemental::GREATER_EARTH;
}

static util::string_view ancestor_cast_str( ancestor_cast cast )
{
  switch ( cast )
  {
    case ancestor_cast::LAVA_BURST: return "lava_burst";
    case ancestor_cast::CHAIN_LIGHTNING: return "chain_lightning";
    case ancestor_cast::ELEMENTAL_BLAST: return "elemental_blast";
    default: return "disabled";
  }
}

static std::string action_name( util::string_view name, spell_variant t )
{
  switch ( t )
  {
    case spell_variant::ASCENDANCE: return fmt::format( "{}_asc", name );
    case spell_variant::DEEPLY_ROOTED_ELEMENTS: return fmt::format( "{}_dre", name );
    case spell_variant::PRIMORDIAL_WAVE: return fmt::format( "{}_pw", name );
    case spell_variant::THORIMS_INVOCATION: return fmt::format( "{}_ti", name );
    case spell_variant::FUSION_OF_ELEMENTS: return fmt::format( "{}_foe", name );
    default: return std::string( name );
  }
}

static util::string_view exec_type_str( spell_variant t )
{
  switch ( t )
  {
    case spell_variant::ASCENDANCE: return "ascendance";
    case spell_variant::DEEPLY_ROOTED_ELEMENTS: return "deeply_rooted_elements";
    case spell_variant::PRIMORDIAL_WAVE: return "primordial_wave";
    case spell_variant::THORIMS_INVOCATION: return "thorims_invocation";
    case spell_variant::FUSION_OF_ELEMENTS: return "fusion_of_elements";
    default: return "normal";
  }
}

struct shaman_attack_t;
struct shaman_spell_t;
struct shaman_heal_t;

template <typename T>
struct shaman_totem_pet_t;

template <typename T>
struct totem_pulse_event_t;

template <typename T>
struct totem_pulse_action_t;

using spell_totem_action_t = totem_pulse_action_t<spell_t>;
using spell_totem_pet_t = shaman_totem_pet_t<spell_t>;
using heal_totem_action_t = totem_pulse_action_t<heal_t>;
using heal_totem_pet_t = shaman_totem_pet_t<heal_t>;

namespace pet
{
struct base_wolf_t;
struct primal_elemental_t;
}

struct shaman_td_t : public actor_target_data_t
{
  struct dots
  {
    dot_t* flame_shock;
  } dot;

  struct debuffs
  {
    // Elemental
    buff_t* lightning_rod;

    // Enhancement
    buff_t* lashing_flames;
  } debuff;

  struct heals
  {
    dot_t* riptide;
    dot_t* earthliving;
  } heal;

  shaman_td_t( player_t* target, shaman_t* p );

  shaman_t* actor() const
  {
    return debug_cast<shaman_t*>( source );
  }
};

struct shaman_t : public player_t
{
public:
  // Misc
  bool lava_surge_during_lvb;
  bool sk_during_cast;
  /// Shaman ability cooldowns
  std::vector<cooldown_t*> ability_cooldowns;
  player_t* earthen_rage_target =
      nullptr;  // required for Earthen Rage, whichs' ticks damage the most recently attacked target
  event_t* earthen_rage_event;

  /// Legacy of the Frost Witch maelstrom stack counter
  unsigned lotfw_counter;

  // Options
  bool raptor_glyph;

  // A vector of action objects that need target cache invalidation whenever the number of
  // Flame Shocks change
  std::vector<action_t*> flame_shock_dependants;
  /// A time-ordered list of active Flame Shocks on enemies
  std::vector<std::pair<timespan_t, dot_t*>> active_flame_shock;
  /// Maximum number of active flame shocks
  unsigned max_active_flame_shock;

  /// Maelstrom Weapon blocklist, allowlist; (spell_id, { override_state, proc tracking object })
  std::vector<mw_proc_state> mw_proc_state_list;

  /// Maelstrom generator/spender tracking
  std::vector<std::pair<simple_sample_data_t, simple_sample_data_t>> mw_source_list;
  std::vector<std::array<simple_sample_data_t, 11>> mw_spend_list;

  /// Deeply Rooted Elements tracking
  extended_sample_data_t dre_samples;
  extended_sample_data_t dre_uptime_samples;

  extended_sample_data_t lvs_samples;

  unsigned dre_attempts;
  double lava_surge_attempts_normalized;

  // Elemental Shamans can extend Ascendance by x sec via Further Beyond (talent)
  // what if there was an overall cap on Ascendance duration extension per Ascendance proc?
  timespan_t accumulated_ascendance_extension_time;
  timespan_t ascendance_extension_cap;

  /// Tempest stack count
  unsigned tempest_counter;

  /// Rolling Thunder last trigger
  timespan_t rt_last_trigger;

  /// Buff state tracking
  unsigned buff_state_lightning_rod;
  unsigned buff_state_lashing_flames;

  // Cached actions
  struct actions_t
  {
    spell_t* lightning_shield;
    attack_t* crash_lightning_aoe;
    action_t* lightning_bolt_pw;
    action_t* lightning_bolt_ti;
    action_t* tempest_ti;
    action_t* chain_lightning_ti;
    action_t* ti_trigger;
    action_t* lava_burst_pw;
    action_t* flame_shock;
    action_t* elemental_blast;

    action_t* lightning_rod;
    action_t* tempest_strikes;

    action_t* stormflurry_ss;
    action_t* stormflurry_ws;

    action_t* feral_spirit_t28;
    action_t* feral_spirit_t31;
    action_t* feral_spirit_rt;

    /// Totemic Recall last used totem (action)
    action_t* totemic_recall_totem;

    // Legendaries
    action_t* dre_ascendance; // Deeply Rooted Elements

    // Cached action pointers
    action_t* feral_spirits; // MW Tracking
    action_t* ascendance; // MW Tracking

    // TWW Hero Talent stuff
    action_t* awakening_storms; // Awakening Storms damage
    action_t* whirling_air_ss;
    action_t* whirling_air_ws;

    action_t* elemental_blast_foe; // Fusion of Elements

    action_t* thunderstrike_ward;

    action_t* earthen_rage;
  } action;

  // Pets
  struct pets_t
  {
    spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t> fire_elemental;
    spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t> storm_elemental;
    spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t> earth_elemental;
    spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t> lesser_fire_elemental;
    spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t> lesser_storm_elemental;
    spawner::pet_spawner_t<pet_t, shaman_t> lightning_elemental;

    spawner::pet_spawner_t<pet_t, shaman_t> ancestor;

    spawner::pet_spawner_t<pet::base_wolf_t, shaman_t> spirit_wolves;
    spawner::pet_spawner_t<pet::base_wolf_t, shaman_t> fire_wolves;
    spawner::pet_spawner_t<pet::base_wolf_t, shaman_t> frost_wolves;
    spawner::pet_spawner_t<pet::base_wolf_t, shaman_t> lightning_wolves;

    spawner::pet_spawner_t<spell_totem_pet_t, shaman_t> liquid_magma_totem;
    spawner::pet_spawner_t<heal_totem_pet_t, shaman_t> healing_stream_totem;
    spawner::pet_spawner_t<spell_totem_pet_t, shaman_t> capacitor_totem;
    spawner::pet_spawner_t<spell_totem_pet_t, shaman_t> surging_totem;
    spawner::pet_spawner_t<spell_totem_pet_t, shaman_t> searing_totem;

    std::vector<pet::base_wolf_t*> all_wolves;

    pets_t( shaman_t* );
  } pet;

  // Constants
  struct
  {
    /// Matching Gear multiplier
    double mul_matching_gear;
    /// Lightning Rod damage_multiplier
    double mul_lightning_rod;
  } constant;

  // Buffs
  struct
  {
    // shared between all three specs
    buff_t* ascendance;
    buff_t* ghost_wolf;
    buff_t* flurry;
    buff_t* natures_swiftness;
    buff_t* primordial_wave;

    // Elemental, Restoration
    buff_t* lava_surge;

    // Elemental, Enhancement
    buff_t* elemental_blast_crit;
    buff_t* elemental_blast_haste;
    buff_t* elemental_blast_mastery;
    buff_t* flametongue_weapon;

    // Elemental
    buff_t* echoes_of_great_sundering_es;
    buff_t* echoes_of_great_sundering_eb;
    buff_t* elemental_equilibrium;
    buff_t* elemental_equilibrium_debuff;
    buff_t* elemental_equilibrium_fire;
    buff_t* elemental_equilibrium_frost;
    buff_t* elemental_equilibrium_nature;
    buff_t* fire_elemental;
    buff_t* storm_elemental;
    buff_t* earth_elemental;
    buff_t* flux_melting;
    buff_t* icefury_dmg;
    buff_t* icefury_cast;
    buff_t* magma_chamber;
    buff_t* master_of_the_elements;
    buff_t* power_of_the_maelstrom;
    buff_t* splintered_elements;
    buff_t* stormkeeper;
    buff_t* surge_of_power;
    buff_t* wind_gust;  // Storm Elemental passive 263806
    buff_t* fusion_of_elements_nature;
    buff_t* fusion_of_elements_fire;
    buff_t* storm_frenzy;
    buff_t* lesser_fire_elemental;
    buff_t* lesser_storm_elemental;
    buff_t* fury_of_the_storms;
    buff_t* call_of_the_ancestors;
    buff_t* ancestral_swiftness;
    buff_t* thunderstrike_ward;

    buff_t* tww1_4pc_ele;

    // Enhancement
    buff_t* maelstrom_weapon;
    buff_t* feral_spirit_maelstrom;

    buff_t* crash_lightning;     // Buffs stormstrike and lava lash after using crash lightning
    buff_t* cl_crash_lightning;  // Buffs crash lightning with extra damage, after using chain lightning
    buff_t* hot_hand;
    buff_t* lightning_shield;
    buff_t* stormbringer;
    buff_t* hailstorm;
    buff_t* windfury_weapon;

    buff_t* forceful_winds;
    buff_t* icy_edge;
    buff_t* molten_weapon;
    buff_t* crackling_surge;
    buff_t* earthen_weapon;
    buff_t* converging_storms;
    buff_t* static_accumulation;
    buff_t* doom_winds;
    buff_t* ice_strike;
    buff_t* ashen_catalyst;
    buff_t* witch_doctors_ancestry;
    buff_t* legacy_of_the_frost_witch;

    // Shared talent stuff
    buff_t* tempest;
    buff_t* unlimited_power;
    buff_t* arc_discharge;
    buff_t* amplification_core;

    buff_t* whirling_air;
    buff_t* whirling_fire;
    buff_t* whirling_earth;

    buff_t* awakening_storms;
    buff_t* totemic_rebound;

    // Restoration
    buff_t* spirit_walk;
    buff_t* spiritwalkers_grace;
    buff_t* tidal_waves;

    //Tier 29
    buff_t* t29_2pc_enh;
    buff_t* t29_4pc_enh;

    // Tier 30
    buff_t* t30_2pc_enh;
    buff_t* t30_4pc_enh_damage;
    buff_t* t30_4pc_enh_cl;

    // PvP
    buff_t* thundercharge;

  } buff;

  // Options
  struct options_t
  {
    rotation_type_e rotation = ROTATION_STANDARD;
    int dre_flat_chance = -1;
    unsigned dre_forced_failures = 2U;

    // Icefury Deck-of-Cards RNG parametrization
    unsigned icefury_positive = 0U;
    unsigned icefury_total = 0U;

    // Ancient Fellowship Deck-of-Cards RNG parametrization
    unsigned ancient_fellowship_positive = 0U;
    unsigned ancient_fellowship_total = 0U;

    // Thunderstrike Ward Uniform RNG proc chance
    // TODO: Double check for CL. A ~5h LB test resulted in a ~30% chance.
    double thunderstrike_ward_proc_chance = 0.3;

    double earthquake_spell_power_coefficient = 0.3884;
  } options;

  // Cooldowns
  struct
  {
    cooldown_t* ascendance;
    cooldown_t* crash_lightning;
    cooldown_t* feral_spirits;
    cooldown_t* fire_elemental;
    cooldown_t* flame_shock;
    cooldown_t* frost_shock;
    cooldown_t* lava_burst;
    cooldown_t* lava_lash;
    cooldown_t* liquid_magma_totem;
    cooldown_t* natures_swiftness;
    cooldown_t* primordial_wave;
    cooldown_t* shock;  // shared CD of flame shock/frost shock for enhance
    cooldown_t* storm_elemental;
    cooldown_t* stormkeeper;
    cooldown_t* strike;  // shared CD of Storm Strike and Windstrike
    cooldown_t* totemic_recall;
    cooldown_t* tempest_strikes;
    cooldown_t* ancestral_swiftness;
  } cooldown;

  // Expansion-specific Legendaries
  struct legendary_t
  {
  } legendary;

  // Gains
  struct
  {
    gain_t* aftershock;
    gain_t* ascendance;
    gain_t* resurgence;
    gain_t* feral_spirit;
    gain_t* fire_elemental;
    gain_t* spirit_of_the_maelstrom;
    gain_t* searing_flames;
    gain_t* inundate;
    gain_t* storm_swell;
  } gain;

  // Tracked Procs
  struct
  {
    // Elemental, Restoration
    proc_t* lava_surge;
    proc_t* wasted_lava_surge;
    proc_t* surge_during_lvb;
    proc_t* deeply_rooted_elements;

    // Elemental
    proc_t* aftershock;
    proc_t* flash_of_lightning;
    proc_t* lightning_rod;
    proc_t* searing_flames;

    std::array<proc_t*, 21> magma_chamber;

    proc_t* surge_of_power_lightning_bolt;
    proc_t* surge_of_power_sk_lightning_bolt;
    proc_t* surge_of_power_lava_burst;
    proc_t* surge_of_power_frost_shock;
    proc_t* surge_of_power_flame_shock;
    proc_t* surge_of_power_wasted;

    proc_t* elemental_blast_haste;
    proc_t* elemental_blast_crit;
    proc_t* elemental_blast_mastery;

    // Enhancement
    proc_t* hot_hand;
    proc_t* stormflurry;
    proc_t* stormflurry_failed;
    proc_t* windfury_uw;
    proc_t* t28_4pc_enh;
    proc_t* reset_swing_mw;

    // TWW Trackers
    proc_t* tempest_awakening_storms;
  } proc;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* mail_specialization;
    const spell_data_t* shaman;

    // Elemental
    const spell_data_t* elemental_shaman;   // general spec multiplier
    const spell_data_t* lightning_bolt_2;   // casttime reduction
    const spell_data_t* lava_burst_2;       // 7.1 Lava Burst autocrit with FS passive
    const spell_data_t* maelstrom;
    const spell_data_t* lava_surge;
    const spell_data_t* inundate;

    // Enhancement
    const spell_data_t* critical_strikes;
    const spell_data_t* dual_wield;
    const spell_data_t* enhancement_shaman;
    const spell_data_t* maelstrom_weapon;

    const spell_data_t* windfury;
    const spell_data_t* lava_lash_2;
    const spell_data_t* stormbringer;
    const spell_data_t* feral_lunge;

    // Restoration
    const spell_data_t* purification;
    const spell_data_t* resurgence;
    const spell_data_t* riptide;
    const spell_data_t* tidal_waves;
    const spell_data_t* spiritwalkers_grace;
    const spell_data_t* restoration_shaman;  // general spec multiplier
  } spec;

  // Masteries
  struct
  {
    const spell_data_t* elemental_overload;
    const spell_data_t* enhanced_elements;
    const spell_data_t* deep_healing;
  } mastery;

  // Uptimes
  struct
  {
    uptime_t* hot_hand;
  } uptime;

  // Talents
  struct
  {
    // Class tree
    // Row 1
    player_talent_t lava_burst;
    player_talent_t chain_lightning;
    // Row 2
    player_talent_t earth_elemental;
    player_talent_t wind_shear;
    player_talent_t spirit_wolf; // TODO: NYU
    player_talent_t thunderous_paws; // TODO: NYI
    player_talent_t frost_shock;
    // Row 3
    player_talent_t earth_shield;
    player_talent_t fire_and_ice;
    player_talent_t capacitor_totem;
    // Row 4
    player_talent_t spiritwalkers_grace;
    player_talent_t static_charge;
    player_talent_t guardians_cudgel; // TODO: NYI
    player_talent_t flurry;
    // Row 5
    player_talent_t graceful_spirit; // TODO: Movement Speed
    player_talent_t natures_fury;
    player_talent_t tempest_strikes;
    // Row 6
    player_talent_t totemic_surge;
    player_talent_t winds_of_alakir; // TODO: NYI
    // Row 7
    player_talent_t healing_stream_totem;
    player_talent_t improved_lightning_bolt;
    player_talent_t spirit_walk;
    player_talent_t gust_of_wind; // TODO: NYI
    player_talent_t enhanced_imbues;
    // Row 8
    player_talent_t natures_swiftness;
    player_talent_t thunderstorm;
    player_talent_t totemic_focus; // TODO: NYI
    player_talent_t surging_shields; // TODO: NYI
    player_talent_t go_with_the_flow; // TODO: Gust of Wind
    // Row 9
    player_talent_t lightning_lasso;
    player_talent_t thundershock;
    player_talent_t totemic_recall;
    // Row 10
    player_talent_t ancestral_guidance;
    player_talent_t creation_core; // TODO: NYI
    player_talent_t call_of_the_elements;

    // Spec - Shared
    player_talent_t ancestral_wolf_affinity; // TODO: NYI
    player_talent_t elemental_blast;
    player_talent_t primordial_wave;
    player_talent_t ascendance;
    player_talent_t deeply_rooted_elements;
    player_talent_t splintered_elements;

    // Enhancement
    // Row 1
    player_talent_t stormstrike;
    // Row 2
    player_talent_t windfury_weapon;
    player_talent_t lava_lash;
    // Row 3
    player_talent_t forceful_winds;
    player_talent_t improved_maelstrom_weapon;
    player_talent_t molten_assault;
    // Row 4
    player_talent_t unruly_winds; // TODO: Spell data still has conduit scaling (prolly non-issue)
    player_talent_t raging_maelstrom;
    player_talent_t ashen_catalyst;
    // Row 5
    player_talent_t doom_winds;
    player_talent_t sundering;
    player_talent_t overflowing_maelstrom;
    player_talent_t fire_nova;
    player_talent_t hailstorm;
    player_talent_t elemental_weapons;
    player_talent_t crashing_storms;
    // Row 6
    player_talent_t storms_wrath;
    player_talent_t crash_lightning;
    player_talent_t stormflurry;
    player_talent_t ice_strike;
    // Row 7
    player_talent_t stormblast;
    player_talent_t converging_storms;
    player_talent_t hot_hand;
    player_talent_t swirling_maelstrom;
    player_talent_t lashing_flames;
    // Row 8
    player_talent_t feral_spirit;
    // Row 9
    player_talent_t primal_maelstrom;
    player_talent_t elemental_assault;
    player_talent_t witch_doctors_ancestry;
    player_talent_t legacy_of_the_frost_witch;
    player_talent_t static_accumulation;
    // Row 10
    player_talent_t alpha_wolf;
    player_talent_t elemental_spirits;
    player_talent_t thorims_invocation;

    // Elemental

    // Row 1
    player_talent_t earth_shock;
    // Row 2
    player_talent_t earthquake_reticle;
    player_talent_t earthquake_target;
    player_talent_t elemental_fury;
    player_talent_t fire_elemental;
    player_talent_t storm_elemental;
    // Row 3
    player_talent_t flash_of_lightning;
    player_talent_t aftershock;
    player_talent_t surge_of_power;
    player_talent_t echo_of_the_elements;
    // Row 4
    player_talent_t icefury;
    player_talent_t unrelenting_calamity;
    player_talent_t master_of_the_elements;
    // Row 5
    player_talent_t fusion_of_elements;
    player_talent_t storm_frenzy;
    player_talent_t swelling_maelstrom;
    player_talent_t primordial_fury;
    player_talent_t flow_of_power;
    player_talent_t elemental_unity;
    // Row 6
    player_talent_t flux_melting;
    player_talent_t lightning_conduit;
    player_talent_t power_of_the_maelstrom;
    player_talent_t improved_flametongue_weapon;
    player_talent_t everlasting_elements;
    player_talent_t flames_of_the_cauldron;
    // Row 7
    player_talent_t eye_of_the_storm;
    player_talent_t thunderstrike_ward;
    player_talent_t echo_chamber;
    player_talent_t searing_flames;
    player_talent_t earthen_rage; // NEW Partial implementation
    // Row 8
    player_talent_t elemental_equilibrium;
    player_talent_t stormkeeper;
    player_talent_t echo_of_the_elementals;
    // Row 9
    player_talent_t mountains_will_fall;
    player_talent_t first_ascendant;
    player_talent_t preeminence;
    player_talent_t fury_of_the_storms;
    player_talent_t skybreakers_fiery_demise;
    player_talent_t magma_chamber;
    // Row 10
    player_talent_t echoes_of_great_sundering;
    player_talent_t lightning_rod;
    player_talent_t primal_elementalist;
    player_talent_t liquid_magma_totem;

    // Stormbringer

    // Row 1
    player_talent_t tempest;

    // Row 2
    player_talent_t unlimited_power;
    player_talent_t stormcaller;
    // player_talent_t shocking_grasp;

    // Row 3
    player_talent_t supercharge;
    player_talent_t storm_swell;
    player_talent_t arc_discharge;
    player_talent_t rolling_thunder;

    // Row 4
    player_talent_t voltaic_surge;
    player_talent_t conductive_energy;
    player_talent_t surging_currents;

    // Row 5
    player_talent_t awakening_storms;

    // Totemic

    // Row 1
    player_talent_t surging_totem;

    // Row 2
    player_talent_t totemic_rebound;
    player_talent_t amplification_core;
    player_talent_t oversurge;
    player_talent_t lively_totems;

    // Row 3
    player_talent_t reactivity;

    // Row 4
    player_talent_t imbuement_mastery;
    player_talent_t pulse_capacitor;
    player_talent_t supportive_imbuements;
    player_talent_t totemic_coordination;
    player_talent_t earthsurge;

    // Row 5
    player_talent_t whirling_elements;

    // Farseer

    // Row 1
    player_talent_t call_of_the_ancestors;

    // Row 2
    player_talent_t latent_wisdom;
    player_talent_t ancient_fellowship;
    player_talent_t heed_my_call;
    player_talent_t routine_communication;
    player_talent_t elemental_reverb;

    // Row 3
    player_talent_t offering_from_beyond;
    player_talent_t primordial_capacity;

    // Row 4
    player_talent_t maelstrom_supremacy;
    player_talent_t final_calling; // NEW Partial implementation (rest are bugs?)

    // Row 5
    player_talent_t ancestral_swiftness;

  } talent;

  // Misc Spells
  struct
  {
    const spell_data_t* ascendance;  // proxy spell data for normal & dre ascendance
    const spell_data_t* resurgence;
    const spell_data_t* maelstrom_weapon;
    const spell_data_t* feral_spirit;
    const spell_data_t* earth_elemental;
    const spell_data_t* fire_elemental;
    const spell_data_t* storm_elemental;
    const spell_data_t* flametongue_weapon;
    const spell_data_t* windfury_weapon;
    const spell_data_t* t28_2pc_enh;
    const spell_data_t* t28_4pc_enh;
    const spell_data_t* inundate;
    const spell_data_t* storm_swell;
    const spell_data_t* lightning_rod;
    const spell_data_t* improved_flametongue_weapon;
    const spell_data_t* earthen_rage;
  } spell;

  struct rng_obj_t
  {
    real_ppm_t* awakening_storms;
    real_ppm_t* lively_totems;

    shuffled_rng_t* icefury;
    shuffled_rng_t* ancient_fellowship;
  } rng_obj;

  // Cached pointer for ascendance / normal white melee
  shaman_attack_t* melee_mh;
  shaman_attack_t* melee_oh;
  shaman_attack_t* ascendance_mh;
  shaman_attack_t* ascendance_oh;

  // Weapon Enchants
  shaman_attack_t* windfury_mh;
  shaman_spell_t* flametongue;
  shaman_attack_t* hailstorm;

  shaman_t( sim_t* sim, util::string_view name, race_e r = RACE_TAUREN )
    : player_t( sim, SHAMAN, name, r ),
      lava_surge_during_lvb( false ),
      sk_during_cast(false),
      lotfw_counter( 0U ),
      raptor_glyph( false ),
      dre_samples( "dre_tracker", false ),
      dre_uptime_samples( "dre_uptime_tracker", false ),
      lvs_samples( "lvs_tracker", false ),
      dre_attempts( 0U ),
      lava_surge_attempts_normalized( 0.0 ),
      accumulated_ascendance_extension_time( timespan_t::from_seconds( 0 ) ),
      ascendance_extension_cap( timespan_t::from_seconds( 0 ) ),
      tempest_counter( 0U ),
      action(),
      pet( this ),
      constant(),
      buff(),
      cooldown(),
      legendary( legendary_t() ),
      gain(),
      proc(),
      spec(),
      mastery(),
      uptime(),
      talent(),
      spell(),
      rng_obj()
  {
    // Cooldowns
    cooldown.ascendance         = get_cooldown( "ascendance" );
    cooldown.crash_lightning    = get_cooldown( "crash_lightning" );
    cooldown.feral_spirits      = get_cooldown( "feral_spirit" );
    cooldown.fire_elemental     = get_cooldown( "fire_elemental" );
    cooldown.flame_shock        = get_cooldown( "flame_shock" );
    cooldown.frost_shock        = get_cooldown( "frost_shock" );
    cooldown.lava_burst         = get_cooldown( "lava_burst" );
    cooldown.lava_lash          = get_cooldown( "lava_lash" );
    cooldown.liquid_magma_totem = get_cooldown( "liquid_magma_totem" );
    cooldown.natures_swiftness  = get_cooldown( "natures_swiftness" );
    cooldown.primordial_wave    = get_cooldown( "primordial_wave" );
    cooldown.shock              = get_cooldown( "shock" );
    cooldown.storm_elemental    = get_cooldown( "storm_elemental" );
    cooldown.stormkeeper        = get_cooldown( "stormkeeper" );
    cooldown.strike             = get_cooldown( "strike" );
    cooldown.totemic_recall     = get_cooldown( "totemic_recall" );
    cooldown.tempest_strikes    = get_cooldown( "tempest_strikes" );
    cooldown.ancestral_swiftness= get_cooldown( "ancestral_swiftness" );

    melee_mh      = nullptr;
    melee_oh      = nullptr;
    ascendance_mh = nullptr;
    ascendance_oh = nullptr;

    // Weapon Enchants
    windfury_mh = nullptr;
    flametongue = nullptr;
    hailstorm   = nullptr;

    if ( specialization() == SHAMAN_ELEMENTAL || specialization() == SHAMAN_ENHANCEMENT )
      resource_regeneration = regen_type::DISABLED;
    else
      resource_regeneration = regen_type::DYNAMIC;

    dre_samples.reserve( 8192 );
    dre_uptime_samples.reserve( 8192 );

    lvs_samples.reserve( 8192 );

    // Buff States
    buff_state_lightning_rod = 0U;
    buff_state_lashing_flames = 0U;
  }

  ~shaman_t() override = default;

  // Misc
  bool is_elemental_pet_active() const;
  pet_t* get_active_elemental_pet() const;
  void summon_elemental( elemental type, timespan_t override_duration = 0_ms );
  void summon_ancestor( double proc_chance = 1.0 );
  void trigger_elemental_blast_proc();
  void summon_lesser_elemental( elemental type, timespan_t override_duration = 0_ms );

  mw_proc_state set_mw_proc_state( action_t* action, mw_proc_state state )
  {
    if ( as<unsigned>( action->internal_id ) >= mw_proc_state_list.size() )
    {
      mw_proc_state_list.resize( action->internal_id + 1U, mw_proc_state::DEFAULT );
    }

    // Use explicit mw_proc_state::DEFAULT set as initialization in shaman_t::action_init_finished
    if ( state == mw_proc_state::DEFAULT )
    {
      return mw_proc_state_list[ action->internal_id ];
    }

    mw_proc_state_list[ action->internal_id ] = state;

    return mw_proc_state_list[ action->internal_id ];
  }

  mw_proc_state set_mw_proc_state( action_t& action, mw_proc_state state )
  { return set_mw_proc_state( &( action ), state ); }

  mw_proc_state get_mw_proc_state( action_t* action ) const
  {
    assert( as<unsigned>( action->internal_id ) < mw_proc_state_list.size() &&
            "No Maelstrom Weapon proc-state found" );

    return mw_proc_state_list[ action->internal_id ];
  }

  mw_proc_state get_mw_proc_state( action_t& action ) const
  { return get_mw_proc_state( &( action ) ); }

  // trackers, big code blocks that shall not be doublicated
  void track_magma_chamber();

  double windfury_proc_chance();

  // triggers
  void trigger_maelstrom_gain( double maelstrom_gain, gain_t* gain = nullptr );
  void trigger_windfury_weapon( const action_state_t* );
  void trigger_flametongue_weapon( const action_state_t* );
  void trigger_stormbringer( const action_state_t* state, double proc_chance = -1.0, proc_t* proc_obj = nullptr );
  void trigger_hot_hand( const action_state_t* state );
  void trigger_lava_surge();
  void trigger_splintered_elements( action_t* secondary );
  void trigger_flash_of_lightning();
  void trigger_swirling_maelstrom( const action_state_t* state );
  void trigger_static_accumulation_refund( const action_state_t* state, int mw_stacks );
  void trigger_elemental_assault( const action_state_t* state );
  void trigger_tempest_strikes( const action_state_t* state );
  void trigger_stormflurry( const action_state_t* state );
  void trigger_primordial_wave_damage( shaman_spell_t* spell );

  // TWW Triggers
  template <typename T>
  void trigger_tempest( T resource_count );
  void trigger_awakening_storms( const action_state_t* state );
  void trigger_earthsurge( const action_state_t* state );
  void trigger_whirling_air( const action_state_t* state );
  void trigger_reactivity( const action_state_t* state );
  void trigger_fusion_of_elements( const action_state_t* state );
  void trigger_thunderstrike_ward( const action_state_t* state );
  void trigger_earthen_rage( const action_state_t* state );
  void trigger_totemic_rebound( const action_state_t* state );
  void trigger_ancestor( ancestor_cast cast, const action_state_t* state );

  // Legendary
  void trigger_legacy_of_the_frost_witch( const action_state_t* state, unsigned consumed_stacks );
  void trigger_elemental_equilibrium( const action_state_t* state );
  void trigger_deeply_rooted_elements( const action_state_t* state );

  void trigger_secondary_flame_shock( player_t* target ) const;
  void trigger_secondary_flame_shock( const action_state_t* state ) const;
  void regenerate_flame_shock_dependent_target_list( const action_t* action ) const;

  void generate_maelstrom_weapon( const action_t* action, int stacks = 1 );
  void generate_maelstrom_weapon( const action_state_t* state, int stacks = 1 );
  void consume_maelstrom_weapon( const action_state_t* state, int stacks );


  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void create_actions() override;
  void create_options() override;
  void init_gains() override;
  void init_procs() override;
  void init_uptimes() override;
  void init_assessors() override;
  void init_rng() override;
  void init_items() override;
  std::string create_profile( save_e ) override;
  void create_special_effects() override;
  void action_init_finished( action_t& action ) override;
  void analyze( sim_t& sim ) override;
  void datacollection_end() override;

  // APL releated methods
  void init_action_list() override;
  void init_action_list_enhancement();
  void init_action_list_elemental();
  void init_action_list_restoration_dps();
  std::string generate_bloodlust_options();
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  double resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* ) override;

  void apply_affecting_auras( action_t& ) override;

  void moving() override;
  void invalidate_cache( cache_e c ) override;
  double non_stacking_movement_modifier() const override;
  double stacking_movement_modifier() const override;
  double composite_melee_crit_chance() const override;
  double composite_melee_auto_attack_speed() const override;
  double composite_melee_haste() const override;
  double composite_spell_crit_chance() const override;
  double composite_spell_haste() const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t* state, bool guardian ) const override;
  double composite_maelstrom_gain_coefficient( const action_state_t* /* state */ = nullptr ) const
  { return 1.0; }
  double matching_gear_multiplier( attribute_e attr ) const override;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = {} ) override;
  void create_pets() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void combat_begin() override;
  void reset() override;
  void merge( player_t& other ) override;
  void copy_from( player_t* ) override;

  target_specific_t<shaman_td_t> target_data;

  const shaman_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  shaman_td_t* get_target_data( player_t* target ) const override
  {
    shaman_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new shaman_td_t( target, const_cast<shaman_t*>( this ) );
    }
    return td;
  }

  // Helper to trigger a secondary ability through action scheduling (i.e., schedule_execute()),
  // without breaking targeting information. Note, causes overhead as an extra action_state_t object
  // is needed to carry the information.
  void trigger_secondary_ability( const action_state_t* source_state, action_t* secondary_action,
                                  bool inherit_state = false );

  template <typename T_CONTAINER, typename T_DATA>
  T_CONTAINER* get_data_entry( util::string_view name, std::vector<T_DATA*>& entries )
  {
    for ( size_t i = 0; i < entries.size(); i++ )
    {
      if ( entries[ i ]->first == name )
      {
        return &( entries[ i ]->second );
      }
    }

    entries.push_back( new T_DATA( name, T_CONTAINER() ) );
    return &( entries.back()->second );
  }
};

// ==========================================================================
// Shaman Custom Buff Declaration
// ==========================================================================
//

struct maelstrom_weapon_buff_t : public buff_t
{
  shaman_t* shaman;

  maelstrom_weapon_buff_t( shaman_t* p ) :
    buff_t( p, "maelstrom_weapon", p->find_spell( 344179 ) ), shaman( p )
  {
    set_max_stack( data().max_stacks() + as<int>( p->talent.raging_maelstrom->effectN( 1 ).base_value() ) );
  }

  void increment( int stacks, double value, timespan_t duration ) override
  {
    buff_t::increment( stacks, value, duration );

    if ( shaman->buff.witch_doctors_ancestry->check() )
    {
      shaman->cooldown.feral_spirits->adjust(
            -( stacks == -1 ? 1 : stacks ) *
            shaman->talent.witch_doctors_ancestry->effectN( 2 ).time_value() );
    }
  }
};

struct ascendance_buff_t : public buff_t
{
  action_t* lava_burst;

  ascendance_buff_t( shaman_t* p )
    : buff_t( p, "ascendance", p->spell.ascendance ),
      lava_burst( nullptr )
  {
    set_cooldown( timespan_t::zero() );  // Cooldown is handled by the action

    if ( p->talent.preeminence.ok() )
    {
      add_invalidate( CACHE_HASTE );
      set_duration( data().duration() + p->talent.preeminence->effectN( 1 ).time_value() );
    }
  }

  void ascendance( attack_t* mh, attack_t* oh );
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};


struct hot_hand_buff_t : public buff_t
{
  shaman_t* shaman;
  hot_hand_buff_t( shaman_t* p )
      : buff_t( p, "hot_hand", p->talent.hot_hand->effectN( 1 ).trigger() ), shaman( p )
  {
    set_cooldown( timespan_t::zero() );
    set_trigger_spell( shaman->talent.hot_hand );
    set_default_value( shaman->talent.hot_hand->effectN( 2 ).percent() );
    set_stack_change_callback(
        [ this ]( buff_t*, int, int ) { shaman->cooldown.lava_lash->adjust_recharge_multiplier(); } );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    bool trigger = buff_t::trigger( s, v, c, d );
    if ( trigger )
    {
      shaman->uptime.hot_hand->update( trigger, sim->current_time() );
    }
    return trigger;
  }

  void expire_override( int s, timespan_t d ) override
  {
    shaman->uptime.hot_hand->update( false, sim->current_time() );
    buff_t::expire_override( s, d );
  }
};

struct cl_crash_lightning_buff_t : public buff_t
{
  shaman_t* shaman;
  cl_crash_lightning_buff_t( shaman_t* p ) : buff_t( p, "cl_crash_lightning", p->find_spell(333964) ),
      shaman( p )
  {
    int max_stack = data().max_stacks();
    if (p->talent.crashing_storms->ok())
    {
      max_stack += as<int>( p->talent.crashing_storms.spell()->effectN( 3 ).base_value() );
    }

    set_max_stack( max_stack );
    set_default_value_from_effect_type( A_ADD_PCT_LABEL_MODIFIER, P_GENERIC );
  }
};

// Changed behavior from in-game single buff to a stacking buff per extra LB so that the haste "stack"
// uptimes can be analyzed in the report and interacted with in APLs
struct splintered_elements_buff_t : public buff_t
{
  shaman_t* shaman;
  splintered_elements_buff_t( shaman_t* p ) :
    buff_t( p, "splintered_elements", p->find_spell( 382043 ) ), shaman( p )
  {
    unsigned max_targets = as<unsigned>(
      shaman->find_class_spell( "Flame Shock" )->max_targets() );
    //set_default_value_from_effect_type( A_HASTE_ALL );
    //set_pct_buff_type( STAT_PCT_BUFF_HASTE );

    // Note, explicitly set here, as value is derived through a formula, not by buff value.
    add_invalidate( cache_e::CACHE_HASTE );
    set_stack_behavior( buff_stack_behavior::DEFAULT );
    set_max_stack( max_targets ? max_targets : 1 );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    //Triggering splintered elements wipes away the old stack count entirely instead of adding to it or refreshing existing.
    this->expire();
    return buff_t::trigger( s, v, c, d );
  };
};

shaman_td_t::shaman_td_t( player_t* target, shaman_t* p ) : actor_target_data_t( target, p )
{
  heal.riptide = nullptr;
  heal.earthliving = nullptr;
  // Shared
  dot.flame_shock = target->get_dot( "flame_shock", p );

  // Elemental
  debuff.lightning_rod      = make_buff( *this, "lightning_rod", p->find_spell( 197209 ) )
    ->set_default_value( p->constant.mul_lightning_rod )
    ->set_stack_change_callback(
      [ p ]( buff_t*, int old, int new_ ) {
        if ( new_ - old > 0 )
        {
          p->buff_state_lightning_rod++;
        }
        else
        {
          p->buff_state_lightning_rod--;
        }
      }
    );

  // Enhancement
  debuff.lashing_flames = make_buff( *this, "lashing_flames", p->find_spell( 334168 ) )
    ->set_trigger_spell( p->talent.lashing_flames )
    ->set_stack_change_callback(
      [ p ]( buff_t*, int old, int new_ ) {
        if ( new_ - old > 0 )
        {
          p->buff_state_lashing_flames++;
        }
        else
        {
          p->buff_state_lashing_flames--;
        }
      }
    )
    ->set_default_value_from_effect( 1 );
}

// ==========================================================================
// Shaman Action Base Template
// ==========================================================================

struct shaman_action_state_t : public action_state_t
{
  spell_variant exec_type = spell_variant::NORMAL;

  shaman_action_state_t( action_t* action_, player_t* target_ ) :
    action_state_t( action_, target_ )
  { }

  void initialize() override
  {
    action_state_t::initialize();
    exec_type = spell_variant::NORMAL;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );

    auto lbs = debug_cast<const shaman_action_state_t*>( s );
    exec_type = lbs->exec_type;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );

    s << " exec_type=" << exec_type_str( exec_type );

    return s;
  }
};

template <class Base>
struct shaman_action_t : public Base
{
private:
  using ab = Base;  // action base, eg. spell_t
public:
  using base_t = shaman_action_t<Base>;

  // General things
  spell_variant exec_type;

  // Ghost wolf unshift
  bool unshift_ghost_wolf;

  // Maelstrom stuff
  gain_t* gain;
  double maelstrom_gain;
  double maelstrom_gain_coefficient;
  bool maelstrom_gain_per_target;

  bool affected_by_molten_weapon_da;
  bool affected_by_molten_weapon_ta;
  bool affected_by_crackling_surge_da;
  bool affected_by_crackling_surge_ta;
  bool affected_by_icy_edge_da;
  bool affected_by_icy_edge_ta;
  bool affected_by_earthen_weapon_da;
  bool affected_by_earthen_weapon_ta;
  bool affected_by_natures_fury;
  bool affected_by_xs_cost;
  bool affected_by_xs_cast_time;
  bool affected_by_enh_mastery_da;
  bool affected_by_enh_mastery_ta;
  bool affected_by_enh_t29_2pc;
  bool affected_by_lotfw_da;
  bool affected_by_lotfw_ta;
  bool affected_by_enh_t30_4pc_da;
  bool affected_by_enh_t30_4pc_ta;

  bool affected_by_stormkeeper_cast_time;
  bool affected_by_stormkeeper_damage;
  bool affected_by_arc_discharge;

  bool affected_by_amplification_core_da;
  bool affected_by_amplification_core_ta;

  bool affected_by_enhanced_imbues_da;

  bool affected_by_storm_frenzy;

  bool affected_by_elemental_unity_fe_da;
  bool affected_by_elemental_unity_fe_ta;
  bool affected_by_elemental_unity_se_da;
  bool affected_by_elemental_unity_se_ta;

  bool affected_by_ele_mastery_da;
  bool affected_by_ele_mastery_ta;

  bool affected_by_lightning_elemental_da;
  bool affected_by_lightning_elemental_ta;

  bool affected_by_ele_tww1_4pc_cc;
  bool affected_by_ele_tww1_4pc_cd;

  shaman_action_t( util::string_view n, shaman_t* player, const spell_data_t* s = spell_data_t::nil(),
                  spell_variant type_ = spell_variant::NORMAL )
    : ab( n, player, s ),
      exec_type( type_ ),
      unshift_ghost_wolf( true ),
      gain( player->get_gain( s->id() > 0 ? s->name_cstr() : n ) ),
      maelstrom_gain( 0 ),
      maelstrom_gain_coefficient( 1.0 ),
      maelstrom_gain_per_target( true ),
      affected_by_molten_weapon_da( false ),
      affected_by_molten_weapon_ta( false ),
      affected_by_crackling_surge_da( false ),
      affected_by_crackling_surge_ta( false ),
      affected_by_icy_edge_da( false ),
      affected_by_icy_edge_ta( false ),
      affected_by_natures_fury( false ),
      affected_by_xs_cost( false ),
      affected_by_xs_cast_time( false ),
      affected_by_enh_mastery_da( false ),
      affected_by_enh_mastery_ta( false ),
      affected_by_enh_t29_2pc( false ),
      affected_by_lotfw_da( false ),
      affected_by_lotfw_ta( false ),
      affected_by_enh_t30_4pc_da( false ),
      affected_by_enh_t30_4pc_ta( false ),
      affected_by_stormkeeper_cast_time( false ),
      affected_by_stormkeeper_damage( false ),
      affected_by_arc_discharge( false ),
      affected_by_amplification_core_da( false ),
      affected_by_amplification_core_ta( false ),
      affected_by_enhanced_imbues_da( false ), // Enhancement damage effects, Ele stuff is handled elsewhere
      affected_by_storm_frenzy( false ),
      affected_by_elemental_unity_fe_da( false ),
      affected_by_elemental_unity_fe_ta( false ),
      affected_by_elemental_unity_se_da( false ),
      affected_by_elemental_unity_se_ta( false ),
      affected_by_ele_mastery_da( false ),
      affected_by_ele_mastery_ta( false ),
      affected_by_lightning_elemental_da( false ),
      affected_by_lightning_elemental_ta( false ),
      affected_by_ele_tww1_4pc_cc( false ),
      affected_by_ele_tww1_4pc_cd( false )
  {
    ab::may_crit = true;
    ab::track_cd_waste = s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero();

    // Auto-parse maelstrom gain from energize
    for ( size_t i = 1; i <= ab::data().effect_count(); i++ )
    {
      const spelleffect_data_t& effect = ab::data().effectN( i );
      if ( effect.type() != E_ENERGIZE || static_cast<power_e>( effect.misc_value1() ) != POWER_MAELSTROM )
      {
        continue;
      }

      maelstrom_gain    = effect.resource( RESOURCE_MAELSTROM );
      ab::energize_type = action_energize::NONE;  // disable resource generation from spell data.
    }
    affected_by_stormkeeper_cast_time =
        ab::data().affected_by( player->find_spell( 191634 )->effectN( 1 ) );
    affected_by_stormkeeper_damage    =
        ab::data().affected_by( player->find_spell( 191634 )->effectN( 2 ) );
    affected_by_molten_weapon_da =
        ab::data().affected_by( player->find_spell( 224125 )->effectN( 1 ) );
    affected_by_molten_weapon_ta =
        ab::data().affected_by( player->find_spell( 224125 )->effectN( 2 ) );

    affected_by_crackling_surge_da =
        ab::data().affected_by( player->find_spell( 224127 )->effectN( 1 ) );
    affected_by_crackling_surge_ta =
        ab::data().affected_by( player->find_spell( 224127 )->effectN( 2 ) );

    affected_by_icy_edge_da =
        ab::data().affected_by( player->find_spell( 224126 )->effectN( 1 ) );
    affected_by_icy_edge_ta =
        ab::data().affected_by( player->find_spell( 224126 )->effectN( 2 ) );

    affected_by_earthen_weapon_da =
        ab::data().affected_by( player->find_spell( 392375 )->effectN( 1 ) );
    affected_by_earthen_weapon_ta =
        ab::data().affected_by( player->find_spell( 392375 )->effectN( 2 ) );

    affected_by_natures_fury =
      ab::data().affected_by( player->talent.natures_fury->effectN( 1 ) ) ||
      ab::data().affected_by_label( player->talent.natures_fury->effectN( 2 ) );

    affected_by_xs_cost = ab::data().affected_by( player->talent.natures_swiftness->effectN( 1 ) ) ||
                          ab::data().affected_by( player->talent.natures_swiftness->effectN( 3 ) ) ||
                          ab::data().affected_by( player->buff.ancestral_swiftness->data().effectN( 1 ) ) ||
                          ab::data().affected_by( player->buff.ancestral_swiftness->data().effectN( 3 ) );
    affected_by_xs_cast_time =
      ab::data().affected_by( player->talent.natures_swiftness->effectN( 2 ) ) ||
      ab::data().affected_by( player->buff.ancestral_swiftness->data().effectN( 2 ) );

    affected_by_enh_mastery_da = ab::data().affected_by( player->mastery.enhanced_elements->effectN( 1 ) );
    affected_by_enh_mastery_ta = ab::data().affected_by( player->mastery.enhanced_elements->effectN( 5 ) );
    affected_by_lotfw_da = ab::data().affected_by( player->find_spell( 384451 )->effectN( 1 ) );
    affected_by_lotfw_ta = ab::data().affected_by( player->find_spell( 384451 )->effectN( 2 ) );

    //T29 Enhance 2pc
    affected_by_enh_t29_2pc = ab::data().affected_by( player->find_spell( 394677 )->effectN( 1 ) );

    if ( p()->sets->has_set_bonus( SHAMAN_ENHANCEMENT, T30, B4 ) )
    {
      affected_by_enh_t30_4pc_da = ab::data().affected_by( player->find_spell( 409833 )->effectN( 1 ) );
      affected_by_enh_t30_4pc_ta = ab::data().affected_by( player->find_spell( 409833 )->effectN( 2 ) );
    }

    affected_by_arc_discharge = ab::data().affected_by( player->buff.arc_discharge->data().effectN( 1 ) );

    affected_by_amplification_core_da = ab::data().affected_by( player->find_spell( 456369 )->effectN( 1 ) );
    affected_by_amplification_core_ta = ab::data().affected_by( player->find_spell( 456369 )->effectN( 2 ) );

    affected_by_enhanced_imbues_da = ab::data().affected_by( player->talent.enhanced_imbues->effectN( 2 ) );

    affected_by_storm_frenzy = ab::data().affected_by( player->buff.storm_frenzy->data().effectN( 1 ) );

    affected_by_elemental_unity_fe_da = ab::data().affected_by( player->buff.fire_elemental->data().effectN( 4 ) ) ||
                                        ab::data().affected_by( player->buff.lesser_fire_elemental->data().effectN( 4 ) );
    affected_by_elemental_unity_fe_ta = ab::data().affected_by( player->buff.fire_elemental->data().effectN( 5 ) ) ||
                                        ab::data().affected_by( player->buff.lesser_fire_elemental->data().effectN( 5 ) );
    affected_by_elemental_unity_se_da = ab::data().affected_by( player->buff.storm_elemental->data().effectN( 4 ) ) ||
                                        ab::data().affected_by( player->buff.lesser_storm_elemental->data().effectN( 4 ) );
    affected_by_elemental_unity_se_ta = ab::data().affected_by( player->buff.storm_elemental->data().effectN( 5 ) ) ||
                                        ab::data().affected_by( player->buff.lesser_storm_elemental->data().effectN( 5 ) );

    affected_by_ele_mastery_da = ab::data().affected_by( player->mastery.elemental_overload->effectN( 4 ) );
    affected_by_ele_mastery_ta = ab::data().affected_by( player->mastery.elemental_overload->effectN( 5 ) );

    affected_by_lightning_elemental_da = ab::data().affected_by( player->buff.fury_of_the_storms->data().effectN( 2 ) );
    affected_by_lightning_elemental_ta = ab::data().affected_by( player->buff.fury_of_the_storms->data().effectN( 3 ) );

    affected_by_ele_tww1_4pc_cc = ab::data().affected_by(
      player->sets->set( SHAMAN_ELEMENTAL, TWW1, B4 )->effectN( 1 ).trigger()->effectN( 1 ) );
    affected_by_ele_tww1_4pc_cd = ab::data().affected_by(
      player->sets->set( SHAMAN_ELEMENTAL, TWW1, B4 )->effectN( 1 ).trigger()->effectN( 2 ) );
  }

  std::string full_name() const
  {
    std::string n = ab::data().name_cstr();
    return n.empty() ? ab::name_str : n;
  }

  void init_finished() override
  {
    ab::init_finished();

    // Set hasted cooldown here; Note, apply_affecting_auras cannot be used for this, since
    // Shamans have shared cooldowns, and the forementioned method gets called in action
    // constructor.
    if ( ab::data().affected_by( p()->spec.shaman->effectN( 2 ) ) )
    {
      ab::cooldown->hasted = true;
    }

    // Set hasted GCD here; Note, apply_affecting_auras cannot be used for this, since
    // Shamans have shared cooldowns, and the forementioned method gets called in action
    // constructor.
    if ( ab::data().affected_by( p()->spec.shaman->effectN( 3 ) ) )
    {
      ab::gcd_type = gcd_haste_type::ATTACK_HASTE;
    }

    if ( ab::cooldown->duration > timespan_t::zero() )
    {
      p()->ability_cooldowns.push_back( this->cooldown );
    }
  }

  static shaman_action_state_t* cast_state( action_state_t* s )
  { return debug_cast<shaman_action_state_t*>( s ); }

  static const shaman_action_state_t* cast_state( const action_state_t* s )
  { return debug_cast<const shaman_action_state_t*>( s ); }

  action_state_t* new_state() override
  { return new shaman_action_state_t( this, this->target ); }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    ab::snapshot_internal( s, flags, rt );

    cast_state( s )->exec_type = this->exec_type;
  }

  double composite_total_attack_power() const override
  {
    double m = ab::composite_total_attack_power();

    return m;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = ab::recharge_multiplier( cd );

    m *= 1.0 / ( 1.0 + p()->buff.thundercharge->stack_value() );

    // TODO: Current presumption is self-cast, giving multiplicative effect
    m *= 1.0 / ( 1.0 + p()->buff.thundercharge->stack_value() );

    return m;
  }

  double action_da_multiplier() const override
  {
    double m = ab::action_da_multiplier();

    if ( affected_by_enh_mastery_da )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by_ele_mastery_da )
    {
      m *= 1.0 + p()->mastery.elemental_overload->effectN( 4 ).mastery_value() * p()->cache.mastery();
    }

    if ( affected_by_lightning_elemental_da && p()->buff.fury_of_the_storms->up() )
    {
      m *= 1.0 + p()->buff.fury_of_the_storms->data().effectN( 2 ).percent();
    }

    if ( affected_by_lotfw_da && p()->buff.legacy_of_the_frost_witch->check() )
    {
      m *= 1.0 + p()->buff.legacy_of_the_frost_witch->value();
    }

    if ( affected_by_molten_weapon_da && p()->buff.molten_weapon->check() )
    {
      for ( int x = 1; x <= p()->buff.molten_weapon->check(); x++ )
      {
        m *= 1.0 + p()->buff.molten_weapon->value();
      }
    }

    if ( affected_by_crackling_surge_da && p()->buff.crackling_surge->check() )
    {
      for ( int x = 1; x <= p()->buff.crackling_surge->check(); x++ )
      {
        m *= 1.0 + p()->buff.crackling_surge->value();
      }
    }

    if ( affected_by_icy_edge_da && p()->buff.icy_edge->check() )
    {
      for ( int x = 1; x <= p()->buff.icy_edge->check(); x++ )
      {
        m *= 1.0 + p()->buff.icy_edge->value();
      }
    }

    if ( affected_by_earthen_weapon_da && p()->buff.earthen_weapon->check() )
    {
      for ( int x = 1; x <= p()->buff.earthen_weapon->check(); x++ )
      {
        m *= 1.0 + p()->buff.earthen_weapon->value();
      }
    }

    if ( affected_by_enh_t29_2pc && p()->buff.t29_2pc_enh->check() )
    {
      m *= 1.0 + p()->buff.t29_2pc_enh->value();
    }

    if ( affected_by_enh_t30_4pc_da && p()->buff.t30_4pc_enh_damage->check() )
    {
      m *= 1.0 + p()->buff.t30_4pc_enh_damage->value();
    }

    if ( affected_by_arc_discharge && p()->buff.arc_discharge->check() )
    {
      m *= 1.0 + p()->buff.arc_discharge->value();
    }

    if ( affected_by_amplification_core_da && p()->buff.amplification_core->check() )
    {
      m *= 1.0 + p()->buff.amplification_core->value();
    }

    if ( affected_by_enhanced_imbues_da )
    {
      m *= 1.0 + p()->talent.enhanced_imbues->effectN( 2 ).percent();
    }

    if ( affected_by_elemental_unity_fe_da && p()->talent.elemental_unity.ok() &&
         p()->buff.fire_elemental->check() )
    {
      m *= 1.0 + p()->buff.fire_elemental->data().effectN( 4 ).percent();
    }

    if ( affected_by_elemental_unity_fe_da && p()->talent.elemental_unity.ok() &&
         p()->buff.lesser_fire_elemental->check() )
    {
      m *= 1.0 + p()->buff.lesser_fire_elemental->data().effectN( 4 ).percent();
    }

    if ( affected_by_elemental_unity_se_da && p()->talent.elemental_unity.ok() &&
         p()->buff.storm_elemental->check() )
    {
      m *= 1.0 + p()->buff.storm_elemental->data().effectN( 4 ).percent();
    }

    if ( affected_by_elemental_unity_se_da && p()->talent.elemental_unity.ok() &&
         p()->buff.lesser_storm_elemental->check() )
    {
      m *= 1.0 + p()->buff.lesser_storm_elemental->data().effectN( 4 ).percent();
    }

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = ab::action_ta_multiplier();

    if ( affected_by_enh_mastery_ta )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by_ele_mastery_ta )
    {
      m *= 1.0 + p()->mastery.elemental_overload->effectN( 5 ).mastery_value() * p()->cache.mastery();
    }

    if ( affected_by_lightning_elemental_da && p()->buff.fury_of_the_storms->up() )
    {
      m *= 1.0 + p()->buff.fury_of_the_storms->data().effectN( 3 ).percent();
    }

    if ( affected_by_lotfw_ta && p()->buff.legacy_of_the_frost_witch->check() )
    {
      m *= 1.0 + p()->buff.legacy_of_the_frost_witch->value();
    }

    if ( affected_by_molten_weapon_ta && p()->buff.molten_weapon->check() )
    {
      for ( int x = 1; x <= p()->buff.molten_weapon->check(); x++ )
      {
        m *= 1.0 + p()->buff.molten_weapon->value();
      }
    }

    if ( affected_by_crackling_surge_ta && p()->buff.crackling_surge->up() )
    {
      for ( int x = 1; x <= p()->buff.crackling_surge->check(); x++ )
      {
        m *= 1.0 + p()->buff.crackling_surge->value();
      }
    }

    if ( affected_by_icy_edge_ta && p()->buff.icy_edge->up() )
    {
      for ( int x = 1; x <= p()->buff.icy_edge->check(); x++ )
      {
        m *= 1.0 + p()->buff.icy_edge->value();
      }
    }

    if ( affected_by_earthen_weapon_ta && p()->buff.earthen_weapon->up() )
    {
      for ( int x = 1; x <= p()->buff.earthen_weapon->check(); x++ )
      {
        m *= 1.0 + p()->buff.earthen_weapon->value();
      }
    }

    if ( affected_by_enh_t30_4pc_ta && p()->buff.t30_4pc_enh_damage->check() )
    {
      m *= 1.0 + p()->buff.t30_4pc_enh_damage->value();
    }

    if ( affected_by_amplification_core_ta && p()->buff.amplification_core->check() )
    {
      m *= 1.0 + p()->buff.amplification_core->value();
    }

    if ( affected_by_elemental_unity_fe_ta && p()->talent.elemental_unity.ok() &&
         p()->buff.fire_elemental->check() )
    {
      m *= 1.0 + p()->buff.fire_elemental->data().effectN( 5 ).percent();
    }

    if ( affected_by_elemental_unity_fe_ta && p()->talent.elemental_unity.ok() &&
         p()->buff.lesser_fire_elemental->check() )
    {
      m *= 1.0 + p()->buff.lesser_fire_elemental->data().effectN( 5 ).percent();
    }

    if ( affected_by_elemental_unity_se_ta && p()->talent.elemental_unity.ok() &&
         p()->buff.storm_elemental->check() )
    {
      m *= 1.0 + p()->buff.storm_elemental->data().effectN( 5 ).percent();
    }

    if ( affected_by_elemental_unity_se_ta && p()->talent.elemental_unity.ok() &&
         p()->buff.lesser_storm_elemental->check() )
    {
      m *= 1.0 + p()->buff.lesser_storm_elemental->data().effectN( 5 ).percent();
    }

    return m;
  }

  double composite_crit_chance_multiplier() const override
  {
    double m = ab::composite_crit_chance_multiplier();

    if ( affected_by_ele_tww1_4pc_cc )
    {
      m *= 1.0 + p()->buff.tww1_4pc_ele->value();
    }

    return m;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = ab::composite_crit_damage_bonus_multiplier();

    if ( affected_by_ele_tww1_4pc_cd && p()->buff.tww1_4pc_ele->up() )
    {
      m *= 1.0 + p()->buff.tww1_4pc_ele->data().effectN( 2 ).percent();
    }

    return m;
  }

  shaman_t* p()
  {
    return debug_cast<shaman_t*>( ab::player );
  }
  const shaman_t* p() const
  {
    return debug_cast<shaman_t*>( ab::player );
  }

  shaman_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  virtual double composite_maelstrom_gain_coefficient( const action_state_t* state = nullptr ) const
  {
    double m = maelstrom_gain_coefficient;

    m *= p()->composite_maelstrom_gain_coefficient( state );

    return m;
  }

  virtual bool is_damaging_ability() const
  {
    return is_direct_damage_ability() || is_periodic_damage_ability();
  }

  virtual bool is_direct_damage_ability() const
  {
    return this->harmful && (
        this->base_dd_min > 0 ||
        this->spell_power_mod.direct > 0 ||
        this->attack_power_mod.direct > 0
    );
  }

  virtual bool is_periodic_damage_ability() const
  {
    return this->harmful && (
        this->base_td > 0 ||
        this->spell_power_mod.tick > 0 ||
        this->attack_power_mod.tick > 0
    );
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = ab::execute_time_pct_multiplier();

    if ( affected_by_xs_cast_time && p()->buff.natures_swiftness->check() && !ab::background )
    {
      mul *= 1.0 + p()->talent.natures_swiftness->effectN( 2 ).percent();
    }

    if ( affected_by_xs_cast_time && p()->buff.ancestral_swiftness->check() && !ab::background )
    {
      mul *= 1.0 + p()->buff.ancestral_swiftness->data().effectN( 2 ).percent();
    }

    if ( affected_by_arc_discharge && p()->buff.arc_discharge->check() )
    {
      mul *= 1.0 + p()->buff.arc_discharge->data().effectN( 1 ).percent();
    }

    if ( affected_by_storm_frenzy && p()->buff.storm_frenzy->check() )
    {
      mul *= 1.0 + p()->buff.storm_frenzy->value();
    }

    return mul;
  }

  double cost_flat_modifier() const override
  {
    double c = ab::cost_flat_modifier();

    // check all effectN entries and apply them if appropriate
    for ( auto i = 1U; i <= p()->talent.eye_of_the_storm->effect_count(); i++ )
    {
        if ( this->data().affected_by( p()->talent.eye_of_the_storm->effectN( i ) ) )
        {
          c += p()->talent.eye_of_the_storm->effectN( i ).base_value();
        }
    }

    return c;
  }

  double cost_pct_multiplier() const override
  {
    double c = ab::cost_pct_multiplier();

    if ( affected_by_xs_cost && p()->buff.natures_swiftness->check() && !ab::background && ab::current_resource() != RESOURCE_MAELSTROM )
    {
      c *= 1.0 + p()->talent.natures_swiftness->effectN( 1 ).percent();
    }

    if ( affected_by_xs_cost && p()->buff.ancestral_swiftness->check() && !ab::background && ab::current_resource() != RESOURCE_MAELSTROM )
    {
      c *= 1.0 + p()->buff.ancestral_swiftness->data().effectN( 1 ).percent();
    }


    return c;
  }

  void execute() override
  {
    ab::execute();

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      trigger_maelstrom_gain( ab::execute_state );
    }

    if ( p()->talent.flurry.ok() && this->execute_state->result == RESULT_CRIT )
    {
      p()->buff.flurry->trigger( p()->buff.flurry->max_stack() );
    }

    if ( ( affected_by_xs_cast_time || affected_by_xs_cost ) && !(affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->up()) && !ab::background)
    {
      p()->buff.natures_swiftness->decrement();
    }

    if ( ( affected_by_xs_cast_time || affected_by_xs_cost ) && !(affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->up()) && !ab::background)
    {
      p()->buff.ancestral_swiftness->decrement();
    }

    if ( exec_type != spell_variant::PRIMORDIAL_WAVE && affected_by_enh_t29_2pc &&
         this->p()->buff.t29_2pc_enh->up() )
    {
      this->p()->generate_maelstrom_weapon( this->execute_state );
      //this->p()->buff.maelstrom_weapon->increment( 1 );
      this->p()->buff.t29_2pc_enh->expire();
    }

    if ( affected_by_arc_discharge )
    {
      this->p()->buff.arc_discharge->decrement();
    }

    if ( affected_by_storm_frenzy && !this->background && exec_type == spell_variant::NORMAL )
    {
      this->p()->buff.storm_frenzy->decrement();
    }
  }

  void impact( action_state_t* state ) override
  {
    ab::impact( state );

    p()->trigger_stormbringer( state );
  }

  void schedule_execute( action_state_t* execute_state = nullptr ) override
  {
    if ( !ab::background && unshift_ghost_wolf )
    {
      p()->buff.ghost_wolf->expire();
    }

    ab::schedule_execute( execute_state );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( !util::str_compare_ci( name, "cooldown.higher_priority.min_remains" ) )
      return ab::create_expression( name );

    struct hprio_cd_min_remains_expr_t : public expr_t
    {
      action_t* action_;
      std::vector<cooldown_t*> cd_;

      // TODO: Line_cd support
      hprio_cd_min_remains_expr_t( action_t* a ) : expr_t( "min_remains" ), action_( a )
      {
        action_priority_list_t* list = a->player->get_action_priority_list( a->action_list->name_str );
        for ( auto list_action : list->foreground_action_list )
        {
          // Jump out when we reach this action
          if ( list_action == action_ )
            break;

          // Skip if this action's cooldown is the same as the list action's cooldown
          if ( list_action->cooldown == action_->cooldown )
            continue;

          // Skip actions with no cooldown
          if ( list_action->cooldown && list_action->cooldown->duration == timespan_t::zero() )
            continue;

          // Skip cooldowns that are already accounted for
          if ( std::find( cd_.begin(), cd_.end(), list_action->cooldown ) != cd_.end() )
            continue;

          // std::cout << "Appending " << list_action -> name() << " to check list" << std::endl;
          cd_.push_back( list_action->cooldown );
        }
      }

      double evaluate() override
      {
        if ( cd_.empty() )
          return 0;

        timespan_t min_cd = cd_[ 0 ]->remains();
        for ( size_t i = 1, end = cd_.size(); i < end; i++ )
        {
          timespan_t remains = cd_[ i ]->remains();
          // std::cout << "cooldown.higher_priority.min_remains " << cd_[ i ] -> name_str << " remains=" <<
          // remains.total_seconds() << std::endl;
          if ( remains < min_cd )
            min_cd = remains;
        }

        // std::cout << "cooldown.higher_priority.min_remains=" << min_cd.total_seconds() << std::endl;
        return min_cd.total_seconds();
      }
    };

    return std::make_unique<hprio_cd_min_remains_expr_t>( this );
  }

  virtual void trigger_maelstrom_gain( const action_state_t* state )
  {
    if ( maelstrom_gain == 0 )
    {
      return;
    }

    double g = maelstrom_gain;
    g *= composite_maelstrom_gain_coefficient( state );

    if ( maelstrom_gain_per_target ) {
      g *= state->n_targets;
    }

    ab::player->resource_gain( RESOURCE_MAELSTROM, g, gain, this );
  }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public shaman_action_t<melee_attack_t>
{
private:
  using ab = shaman_action_t<melee_attack_t>;

public:
  bool may_proc_windfury;
  bool may_proc_flametongue;
  bool may_proc_stormbringer;
  bool may_proc_lightning_shield;
  bool may_proc_hot_hand;
  bool may_proc_ability_procs;  // For things that explicitly state they proc from "abilities"

  proc_t *proc_wf, *proc_ft, *proc_fb, *proc_mw, *proc_sb, *proc_ls, *proc_hh;

  shaman_attack_t( util::string_view token, shaman_t* p, const spell_data_t* s )
    : base_t( token, p, s ),
      may_proc_windfury( p->talent.windfury_weapon.ok() ),
      may_proc_flametongue( true ),
      may_proc_stormbringer( p->spec.stormbringer->ok() ),
      may_proc_lightning_shield( false ),
      may_proc_hot_hand( p->talent.hot_hand.ok() ),
      may_proc_ability_procs( true ),
      proc_wf( nullptr ),
      proc_ft( nullptr ),
      proc_mw( nullptr ),
      proc_sb( nullptr ),
      proc_hh( nullptr )
  {
    special    = true;
    may_glance = false;
  }

  void init() override
  {
    ab::init();

    if ( may_proc_stormbringer )
    {
      may_proc_stormbringer = ab::weapon != nullptr;
    }

    if ( may_proc_flametongue )
    {
      may_proc_flametongue = ab::weapon != nullptr;
    }

    if ( may_proc_windfury )
    {
      may_proc_windfury = ab::weapon != nullptr;
    }

    if ( may_proc_hot_hand )
    {
      may_proc_hot_hand = ab::weapon != nullptr && !special;
    }

    may_proc_lightning_shield = ab::weapon != nullptr;

  }

  void init_finished() override
  {
    if ( may_proc_flametongue )
    {
      proc_ft = player->get_proc( std::string( "Flametongue: " ) + full_name() );
    }

    if ( may_proc_hot_hand )
    {
      proc_hh = player->get_proc( std::string( "Hot Hand: " ) + full_name() );
    }

    if ( may_proc_stormbringer )
    {
      proc_sb = player->get_proc( std::string( "Stormbringer: " ) + full_name() );
    }

    if ( may_proc_windfury )
    {
      proc_wf = player->get_proc( std::string( "Windfury: " ) + full_name() );
    }

    base_t::init_finished();
  }

  void execute() override
  {
    base_t::execute();

    if ( !special )
    {
      p()->buff.flurry->decrement();
    }
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    // Bail out early if the result is a miss/dodge/parry/ms
    if ( !result_is_hit( state->result ) )
      return;

    p()->trigger_windfury_weapon( state );
    p()->trigger_flametongue_weapon( state );
    p()->trigger_hot_hand( state );
  }

  virtual double stormbringer_proc_chance() const
  {
    double base_mul = p()->mastery.enhanced_elements->effectN( 3 ).mastery_value() *
      ( 1.0 + p()->talent.storms_wrath->effectN( 1 ).percent() );
    double base_chance = p()->spec.stormbringer->proc_chance() +
                         p()->cache.mastery() * base_mul;

    return base_chance;
  }
};

// ==========================================================================
// Shaman Base Spell
// ==========================================================================

template <class Base>
struct shaman_spell_base_t : public shaman_action_t<Base>
{
private:
  using ab = shaman_action_t<Base>;

public:
  using base_t = shaman_spell_base_t<Base>;

  bool affected_by_maelstrom_weapon = false;

  int mw_consume_max_stack, mw_consumed_stacks, mw_affected_stacks;
  // Cache execute MW multiplier into a variable upon cast finish
  double mw_multiplier;

  ancestor_cast ancestor_trigger;

  shaman_spell_base_t( util::string_view n, shaman_t* player,
                       const spell_data_t* s = spell_data_t::nil(),
                       spell_variant type_ = spell_variant::NORMAL )
    : ab( n, player, s, type_ ), mw_consume_max_stack( 0 ), mw_consumed_stacks( 0 ),
      mw_affected_stacks( 0 ), mw_multiplier( 0.0 ), ancestor_trigger( ancestor_cast::DISABLED )
  {
    if ( this->data().affected_by( player->spell.maelstrom_weapon->effectN( 1 ) ) )
    {
      affected_by_maelstrom_weapon = true;
    }

    mw_consume_max_stack = std::max(
        as<int>( this->p()->buff.maelstrom_weapon->data().max_stacks() ),
        as<int>( this->p()->talent.overflowing_maelstrom->effectN( 1 ).base_value() )
    );
  }

  virtual bool benefit_from_maelstrom_weapon() const
  {
    return affected_by_maelstrom_weapon && this->p()->buff.maelstrom_weapon->up();
  }

  // Some spells do not consume Maelstrom Weapon stacks always, so need to control this on
  // a spell to spell level
  virtual bool consume_maelstrom_weapon() const
  {
    if ( this->exec_type == spell_variant::THORIMS_INVOCATION )
    {
      return true;
    }

    return benefit_from_maelstrom_weapon() && !this->background;
  }

  virtual int maelstrom_weapon_stacks() const
  {
    if ( !benefit_from_maelstrom_weapon() )
    {
      return 0;
    }

    auto mw_stacks = std::min( mw_consume_max_stack, this->p()->buff.maelstrom_weapon->check() );

    if ( this->exec_type == spell_variant::THORIMS_INVOCATION )
    {
      mw_stacks = std::min( mw_stacks,
                            as<int>( this->p()->talent.thorims_invocation->effectN( 1 ).base_value() ) );
    }

    return mw_stacks;
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = ab::execute_time_pct_multiplier();

    mul *= 1.0 + this->p()->spell.maelstrom_weapon->effectN( 1 ).percent() * maelstrom_weapon_stacks();

    return mul;
  }

  double action_multiplier() const override
  {
    double m = ab::action_multiplier();

    m *= 1.0 + mw_multiplier;

    if ( this->p()->main_hand_weapon.buff_type == FLAMETONGUE_IMBUE &&
         this->p()->talent.improved_flametongue_weapon.ok() &&
         dbc::is_school( this->school, SCHOOL_FIRE ) )
    {
      // spelldata doesn't have the 5% yet. It's hardcoded in the tooltip.
      // Enhanced Imbues cannot be applied through passive effects.
      m *= 1.0 + this->p()->spell.improved_flametongue_weapon->effectN( 1 ).percent() *
        ( 1.0 + this->p()->talent.enhanced_imbues->effectN( 1 ).percent() );
    }

    return m;
  }

  void compute_mw_multiplier()
  {
    if ( !this->p()->spec.maelstrom_weapon->ok() || !affected_by_maelstrom_weapon )
    {
      return;
    }

    mw_multiplier = 0.0;
    mw_affected_stacks = maelstrom_weapon_stacks();
    mw_consumed_stacks = consume_maelstrom_weapon() ? mw_affected_stacks : 0;

    if ( mw_affected_stacks && affected_by_maelstrom_weapon )
    {
      double stack_value = this->p()->talent.improved_maelstrom_weapon->effectN( 1 ).percent() +
                           this->p()->talent.raging_maelstrom->effectN( 2 ).percent();

      mw_multiplier = stack_value * mw_affected_stacks;
    }

    if ( this->sim->debug && mw_multiplier )
    {
      this->sim->out_debug.print(
        "{} {} mw_affected={}, mw_benefit={}, mw_consumed={}, mw_stacks={}, mw_multiplier={}",
        this->player->name(), this->name(), affected_by_maelstrom_weapon,
        benefit_from_maelstrom_weapon(), mw_consumed_stacks,
        mw_affected_stacks, mw_multiplier );
    }
  }

  void execute() override
  {
    // Compute and cache Maelstrom Weapon multiplier before executing the spell. MW multiplier is
    // used to compute the damage of the spell, either during execute or during impact (Lava Burst).
    compute_mw_multiplier();

    ab::execute();

    // Main hand swing timer resets if the MW-affected spell is not instant cast
    // Need to check this before spending the MW or autos will be lost.
    if ( affected_by_maelstrom_weapon && mw_consumed_stacks < 5 )
    {
      if ( this->p()->main_hand_attack && this->p()->main_hand_attack->execute_event &&
           ( this->p()->bugs || !this->background ) )
      {
        if ( this->sim->debug )
        {
          this->sim->out_debug.print( "{} resetting {} due to MW spell cast of {}",
                                     this->p()->name(), this->p()->main_hand_attack->name(),
                                     this->name() );
        }
        event_t::cancel( this->p()->main_hand_attack->execute_event );
        this->p()->main_hand_attack->schedule_execute();
        this->p()->proc.reset_swing_mw->occur();
      }
    }

    // for benefit tracking purpose
    this->p()->buff.spiritwalkers_grace->up();

    if ( this->p()->talent.aftershock->ok() &&
         this->current_resource() == RESOURCE_MAELSTROM &&
         this->last_resource_cost > 0 &&
         this->rng().roll( this->p()->talent.aftershock->effectN( 1 ).percent() ) )
    {
      this->p()->trigger_maelstrom_gain( this->last_resource_cost,
          this->p()->gain.aftershock );
      this->p()->proc.aftershock->occur();
    }

    this->p()->consume_maelstrom_weapon( this->execute_state, mw_consumed_stacks );

    if ( this->exec_type == spell_variant::NORMAL && !this->background )
    {
      this->p()->trigger_ancestor( ancestor_trigger, this->execute_state );
    }
  }
};

// ==========================================================================
// Shaman Offensive Spell
// ==========================================================================

struct elemental_overload_event_t : public event_t
{
  action_state_t* state;

  elemental_overload_event_t( action_state_t* s )
    : event_t( *s->action->player, timespan_t::from_millis( 400 ) ), state( s )
  { }

  ~elemental_overload_event_t() override
  {
    if ( state )
      action_state_t::release( state );
  }

  const char* name() const override
  {
    return "elemental_overload_event_t";
  }

  void execute() override
  {
    state->action->schedule_execute( state );
    state = nullptr;
  }
};

struct shaman_spell_t : public shaman_spell_base_t<spell_t>
{
  action_t* overload;

  bool may_proc_stormbringer = false;
  proc_t* proc_sb;
  bool affected_by_master_of_the_elements = false;
  proc_t* proc_moe;

  // Lightning Rod management
  double accumulated_lightning_rod_damage;
  event_t* lr_event;

  shaman_spell_t( util::string_view token, shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 spell_variant type_ = spell_variant::NORMAL ) :
    base_t( token, p, s, type_ ), overload( nullptr ), proc_sb( nullptr ), proc_moe( nullptr ),
    accumulated_lightning_rod_damage( 0.0 ), lr_event( nullptr )
  {
    may_proc_stormbringer = false;
  }

  void init_finished() override
  {
    if ( may_proc_stormbringer )
    {
      proc_sb = player->get_proc( std::string( "Stormbringer: " ) + full_name() );
    }

    if ( affected_by_master_of_the_elements && p()->talent.master_of_the_elements.ok() )
    {
      proc_moe = p()->get_proc( "Master of the Elements: " + full_name() );
    }

    base_t::init_finished();
  }

  void reset() override
  {
    base_t::reset();

    accumulated_lightning_rod_damage = 0.0;
    lr_event = nullptr;
  }

  double action_multiplier() const override
  {
    double m = base_t::action_multiplier();

    if ( affected_by_master_of_the_elements && p()->talent.master_of_the_elements.ok() )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->value();
    }

    if ( affected_by_stormkeeper_damage && p()->buff.stormkeeper->up() && !p()->sk_during_cast )
    {
      m *= 1.0 + p()->buff.stormkeeper->value();
    }

    return m;
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = base_t::execute_time_pct_multiplier();

    if ( affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->up() && !p()->sk_during_cast )
    {
      // stormkeeper has a -100% value as effect 1
      mul *= 1.0 + p()->buff.stormkeeper->data().effectN( 1 ).percent();
    }

    return mul;
  }

  void execute() override
  {
    base_t::execute();

    if ( affected_by_master_of_the_elements && (!background || id == 188389) && p()->buff.master_of_the_elements->check() )
    {
      p()->buff.master_of_the_elements->decrement();
      proc_moe->occur();
    }

    if ( exec_type == spell_variant::NORMAL && !background && this->harmful ) //TODO: Make this proc on impact
    {
      p()->trigger_fusion_of_elements( execute_state );
    }


    p()->trigger_earthen_rage( execute_state );
  }

  void schedule_travel( action_state_t* s ) override
  {
    trigger_elemental_overload( s );

    base_t::schedule_travel( s );
  }

  bool usable_moving() const override
  {
    if ( p()->buff.spiritwalkers_grace->check() || execute_time() == timespan_t::zero() )
      return true;

    return base_t::usable_moving();
  }

  virtual double overload_chance( const action_state_t* ) const
  {
    double chance = 0.0;

    if ( p()->mastery.elemental_overload->ok() )
    {
      chance += p()->cache.mastery_value();
    }

    // Add excessive amount to ensure overload proc with SK,
    // since chain spells divide chance by X
    if ( affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->check() && !p()->sk_during_cast )
    {
      chance += 10.0;
    }

    return chance;
  }

  bool trigger_elemental_overload( const action_state_t* source_state, double override_chance = -1.0 ) const
  {
    if ( !p()->mastery.elemental_overload->ok() )
    {
      return false;
    }

    if ( !overload )
    {
      return false;
    }

    double proc_chance = override_chance == -1.0
      ? overload_chance( source_state )
      : override_chance;

    if ( !rng().roll( proc_chance ) )
    {
      return false;
    }

    action_state_t* s = overload->get_state();
    s->target = source_state->target;
    overload->snapshot_state( s, result_amount_type::DMG_DIRECT );

    make_event<elemental_overload_event_t>( *sim, s );

    if ( sim->debug )
    {
      sim->out_debug.print( "{} elemental overload {}, chance={:.5f}{}, target={}", p()->name(),
        name(), proc_chance, override_chance != -1.0 ? " (overridden)" : "",
        source_state->target->name() );
    }

    return true;
  }

  void trigger_lightning_rod_debuff( player_t* target, timespan_t override_delay = timespan_t::min() )
  {
    auto delay = override_delay == timespan_t::min() ? rng().range( 10_ms, 100_ms ) : override_delay;

    sim->print_debug( "{} trigger_lightning_rod_debuff, action={}, delay={}, target={}",
      player->name(), name(), delay, target->name() );

    make_event( *sim, delay,
      [ this, target ]() { td( target )->debuff.lightning_rod->trigger(); } );
  }

  void accumulate_lightning_rod_damage( const action_state_t* state )
  {
    if ( !p()->talent.lightning_rod.ok() && !p()->talent.conductive_energy.ok() )
    {
      return;
    }

    if ( p()->buff_state_lightning_rod == 0 )
    {
      return;
    }

    accumulated_lightning_rod_damage += state->result_amount;

    sim->print_debug( "{} accumulate_lightning_rod_damage, action={}, amount={}, total={}",
      player->name(), name(), state->result_amount, accumulated_lightning_rod_damage );

    // Trigger a single "damage event" for Lightning Rod that after the cast, will iterate over
    // all the LR targets and proc the accumulated damage of a single cast on it. Note that this
    // event needs to be triggered before the debuff application event below to ensure that the
    // first application of the LR debuff will not trigger any damage on the target.
    if ( lr_event == nullptr )
    {
      sim->print_debug( "{} accumulate_lightning_rod_damage creating deferred damage event",
        player->name() );

      lr_event = make_event( *sim, [ this ]() {
        trigger_lightning_rod_damage();
        lr_event = nullptr;
      } );
    }
  }

  void trigger_lightning_rod_damage()
  {
    if ( !p()->talent.lightning_rod.ok() && !p()->talent.conductive_energy.ok() )
    {
      return;
    }

    range::for_each( sim->target_non_sleeping_list, [ this ]( player_t* target ) {
      if ( !td( target )->debuff.lightning_rod->up() )
      {
        return;
      }

      sim->print_debug( "{} trigger_lightning_rod_damage, action={}, target={}, amount={}",
        player->name(), name(), target->name(),
        accumulated_lightning_rod_damage * p()->constant.mul_lightning_rod );

      p()->action.lightning_rod->execute_on_target( target,
        accumulated_lightning_rod_damage * p()->constant.mul_lightning_rod );
    } );

    accumulated_lightning_rod_damage = 0.0;
  }

  virtual double stormbringer_proc_chance() const
  {
    double base_mul = p()->mastery.enhanced_elements->effectN( 3 ).mastery_value() *
      ( 1.0 + p()->talent.storms_wrath->effectN( 1 ).percent() );
    double base_chance = p()->spec.stormbringer->proc_chance() +
                         p()->cache.mastery() * base_mul;

    return base_chance;
  }
};

// ==========================================================================
// Shaman Heal
// ==========================================================================

struct shaman_heal_t : public shaman_spell_base_t<heal_t>
{
  double elw_proc_high, elw_proc_low, resurgence_gain;

  bool proc_tidal_waves, consume_tidal_waves;

  shaman_heal_t( util::string_view n, shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 util::string_view options = {} )
    : base_t( n, p, s ),
      elw_proc_high( .2 ),
      elw_proc_low( 1.0 ),
      resurgence_gain( 0 ),
      proc_tidal_waves( false ),
      consume_tidal_waves( false )
  {
    parse_options( options );
  }

  double composite_total_spell_power() const override
  {
    double sp = base_t::composite_total_spell_power();

    if ( p()->main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
      sp += p()->main_hand_weapon.buff_value * p()->composite_spell_power_multiplier();

    return sp;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_da_multiplier( state );
    m *= 1.0 + p()->spec.purification->effectN( 1 ).percent();
    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_ta_multiplier( state );
    m *= 1.0 + p()->spec.purification->effectN( 1 ).percent();
    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = base_t::composite_target_multiplier( target );
    return m;
  }

  void impact( action_state_t* s ) override;

  void execute() override
  {
    base_t::execute();

    if ( consume_tidal_waves )
      p()->buff.tidal_waves->decrement();
  }

  virtual double deep_healing( const action_state_t* s )
  {
    if ( !p()->mastery.deep_healing->ok() )
      return 0.0;

    double hpp = ( 1.0 - s->target->health_percentage() / 100.0 );

    return 1.0 + hpp * p()->cache.mastery_value();
  }
};

namespace pet
{
// Simple helper to summon n (default 1) sleeping pet(s) from a container
template <typename T>
void summon( const T& container, timespan_t duration, size_t n = 1 )
{
  size_t summoned = 0;

  for ( size_t i = 0, end = container.size(); i < end; ++i )
  {
    auto ptr = container[ i ];
    if ( !ptr->is_sleeping() )
    {
      continue;
    }

    ptr->summon( duration );
    if ( ++summoned == n )
    {
      break;
    }
  }
}
// ==========================================================================
// Base Shaman Pet
// ==========================================================================

struct shaman_pet_t : public pet_t
{
  bool use_auto_attack;

  shaman_pet_t( shaman_t* owner, util::string_view name, bool guardian = true, bool auto_attack = true )
    : pet_t( owner->sim, owner, name, guardian ), use_auto_attack( auto_attack )
  {
    resource_regeneration = regen_type::DISABLED;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
  }

  shaman_t* o() const
  {
    return debug_cast<shaman_t*>( owner );
  }

  virtual void create_default_apl()
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( use_auto_attack )
    {
      def->add_action( "auto_attack" );
    }
  }

  void init_action_list() override
  {
    pet_t::init_action_list();

    if ( action_list_str.empty() )
    {
      create_default_apl();
    }
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;

  virtual attack_t* create_auto_attack()
  {
    return nullptr;
  }

  // Apparently shaman pets by default do not inherit attack speed buffs from owner
  double composite_melee_auto_attack_speed() const override
  {
    return o()->cache.attack_haste();
  }

  void apply_affecting_auras( action_t& action ) override
  {
    o()->apply_affecting_auras( action );
  }
};

// ==========================================================================
// Base Shaman Pet Action
// ==========================================================================

template <typename T_PET, typename T_ACTION>
struct pet_action_t : public T_ACTION
{
  using super = pet_action_t<T_PET, T_ACTION>;

  pet_action_t( T_PET* pet, util::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                util::string_view options = {} )
    : T_ACTION( name, pet, spell )
  {
    this->parse_options( options );

    this->special  = true;
    this->may_crit = true;
    // this -> crit_bonus_multiplier *= 1.0 + p() -> o() -> spec.elemental_fury -> effectN( 1 ).percent();
  }

  T_PET* p() const
  {
    return debug_cast<T_PET*>( this->player );
  }

  shaman_t* o() const
  { return debug_cast<shaman_t*>( p()->owner ); }

  void init() override
  {
    T_ACTION::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( p()->o()->pet_list,
                                [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != p()->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }

  double cost() const override
  { return 0; }
};

// ==========================================================================
// Base Shaman Pet Melee Attack
// ==========================================================================

template <typename T_PET>
struct pet_melee_attack_t : public pet_action_t<T_PET, melee_attack_t>
{
  using super = pet_melee_attack_t<T_PET>;

  pet_melee_attack_t( T_PET* pet, util::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                      util::string_view options = {} )
    : pet_action_t<T_PET, melee_attack_t>( pet, name, spell, options )
  {
    if ( this->school == SCHOOL_NONE )
      this->school = SCHOOL_PHYSICAL;

    if ( this->p()->owner_coeff.sp_from_sp > 0 || this->p()->owner_coeff.sp_from_ap > 0 )
    {
      this->spell_power_mod.direct = 1.0;
    }
  }

  void init() override
  {
    pet_action_t<T_PET, melee_attack_t>::init();

    if ( !this->special )
    {
      this->weapon            = &( this->p()->main_hand_weapon );
      this->base_execute_time = this->weapon->swing_time;
    }
  }

  void execute() override
  {
    // If we're casting, we should clip a swing
    if ( this->time_to_execute > timespan_t::zero() && this->player->executing )
      this->schedule_execute();
    else
      pet_action_t<T_PET, melee_attack_t>::execute();
  }
};

// ==========================================================================
// Generalized Auto Attack Action
// ==========================================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( shaman_pet_t* player ) : melee_attack_t( "auto_attack", player )
  {
    assert( player->main_hand_weapon.type != WEAPON_NONE );
    player->main_hand_attack = player->create_auto_attack();
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    return ( player->main_hand_attack->execute_event == nullptr );
  }
};

// ==========================================================================
// Base Shaman Pet Spell
// ==========================================================================

template <typename T_PET>
struct pet_spell_t : public pet_action_t<T_PET, spell_t>
{
  using super = pet_spell_t<T_PET>;

  pet_spell_t( T_PET* pet, util::string_view name, const spell_data_t* spell = spell_data_t::nil(),
               util::string_view options = {} )
    : pet_action_t<T_PET, spell_t>( pet, name, spell, options )
  {
    this->parse_options( options );
  }
};

// ==========================================================================
// Base Shaman Pet Method Definitions
// ==========================================================================

template <typename T>
struct spirit_bomb_t : public pet_melee_attack_t<T>
{
  spirit_bomb_t( T* player ) :
    pet_melee_attack_t<T>( player, "alpha_wolf", player -> find_spell( 198455 ) )
  {
    this -> background = true;
    this -> aoe = -1;
  }

  double composite_target_armor( player_t* ) const override
  { return 0.0; }

  double action_da_multiplier() const override
  {
    double m = pet_melee_attack_t<T>::action_da_multiplier();

    m *= 1.0 + this->o()->buff.legacy_of_the_frost_witch->value();

    for ( int x = 1; x <= this->o()->buff.earthen_weapon->check(); x++ )
    {
      m *= 1.0 + this->o()->buff.earthen_weapon->value();
    }

    m *= 1.0 + this->o()->buff.t30_4pc_enh_damage->value();

    return m;
  }

};

action_t* shaman_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" )
    return new auto_attack_t( this );

  return pet_t::create_action( name, options_str );
}

// ==========================================================================
// Feral Spirit
// ==========================================================================

struct base_wolf_t : public shaman_pet_t
{
  action_t* alpha_wolf;
  buff_t* alpha_wolf_buff;
  wolf_type_e wolf_type;

  base_wolf_t( shaman_t* owner, util::string_view name )
    : shaman_pet_t( owner, name ), alpha_wolf( nullptr ), alpha_wolf_buff( nullptr ), wolf_type( SPIRIT_WOLF )
  {
    owner_coeff.ap_from_ap = 1.125;

    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
  }

  void create_buffs() override
  {
    shaman_pet_t::create_buffs();

    alpha_wolf_buff = make_buff( this, "alpha_wolf", o()->find_spell( 198486 ) )
                      ->set_tick_behavior( buff_tick_behavior::REFRESH )
                      ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                        alpha_wolf->set_target( o() -> target );
                        alpha_wolf->schedule_execute();
                      } );
  }

  void create_actions() override
  {
    shaman_pet_t::create_actions();

    if ( o()->talent.alpha_wolf.ok() )
    {
      alpha_wolf = new spirit_bomb_t<base_wolf_t>( this );
    }
  }

  void trigger_alpha_wolf() const
  {
    if ( o()->talent.alpha_wolf.ok() )
    {
      alpha_wolf_buff->trigger();
    }
  }
};

template <typename T>
struct wolf_base_auto_attack_t : public pet_melee_attack_t<T>
{
  using super = wolf_base_auto_attack_t<T>;

  wolf_base_auto_attack_t( T* wolf, util::string_view n, const spell_data_t* spell = spell_data_t::nil(),
                           util::string_view options_str = {} )
    : pet_melee_attack_t<T>( wolf, n, spell )
  {
    this->parse_options( options_str );

    this->background = this->repeating = true;
    this->special                      = false;

    this->weapon            = &( this->p()->main_hand_weapon );
    this->weapon_multiplier = 1.0;

    this->base_execute_time = this->weapon->swing_time;
    this->school            = SCHOOL_PHYSICAL;
  }

  void execute() override
  {
    pet_melee_attack_t<T>::execute();

    if ( this->p()->o()->sets->has_set_bonus( SHAMAN_ENHANCEMENT, T28, B4 ) &&
         this->rng().roll( this->p()->o()->spell.t28_4pc_enh->effectN( 1 ).percent() ) )
    {
      this->p()->o()->buff.stormbringer->trigger();
      this->p()->o()->cooldown.strike->reset( true );
      this->p()->o()->proc.t28_4pc_enh->occur();
    }
  }
};

struct spirit_wolf_t : public base_wolf_t
{
  struct fs_melee_t : public wolf_base_auto_attack_t<spirit_wolf_t>
  {
    fs_melee_t( spirit_wolf_t* player ) : super( player, "melee" )
    { }
  };

  spirit_wolf_t( shaman_t* owner ) : base_wolf_t( owner, owner->raptor_glyph ? "spirit_raptor" : "spirit_wolf" )
  {
    dynamic = true;
    npc_id = 29264;
  }

  attack_t* create_auto_attack() override
  {
    return new fs_melee_t( this );
  }
};

// ==========================================================================
// DOOM WOLVES OF NOT REALLY DOOM ANYMORE
// ==========================================================================

struct elemental_wolf_base_t : public base_wolf_t
{
  struct dw_melee_t : public wolf_base_auto_attack_t<elemental_wolf_base_t>
  {
    dw_melee_t( elemental_wolf_base_t* player ) : super( player, "melee" )
    { }
  };

  cooldown_t* special_ability_cd;

  elemental_wolf_base_t( shaman_t* owner, util::string_view name )
    : base_wolf_t( owner, name ), special_ability_cd( nullptr )
  {
    dynamic = true;
  }

  attack_t* create_auto_attack() override
  {
    return new dw_melee_t( this );
  }
};

struct frost_wolf_t : public elemental_wolf_base_t
{
  frost_wolf_t( shaman_t* owner ) : elemental_wolf_base_t( owner, owner->raptor_glyph ? "frost_raptor" : "frost_wolf" )
  {
    wolf_type = FROST_WOLF;
    npc_id = 100820;
    npc_suffix = "Frost";
  }
};

struct fire_wolf_t : public elemental_wolf_base_t
{
  fire_wolf_t( shaman_t* owner ) : elemental_wolf_base_t( owner, owner->raptor_glyph ? "fiery_raptor" : "fiery_wolf" )
  {
    wolf_type = FIRE_WOLF;
    npc_id = 100820;
    npc_suffix = "Fire";
  }
};

struct lightning_wolf_t : public elemental_wolf_base_t
{
  lightning_wolf_t( shaman_t* owner )
    : elemental_wolf_base_t( owner, owner->raptor_glyph ? "lightning_raptor" : "lightning_wolf" )
  {
    wolf_type = LIGHTNING_WOLF;
    npc_id = 100820;
    npc_suffix = "Lightning";
  }
};

// ==========================================================================
// Primal Elemental Base
// ==========================================================================

struct primal_elemental_t : public shaman_pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player )
    { background = true; }

    void execute() override
    { player->current.distance = 1; }

    timespan_t execute_time() const override
    { return timespan_t::from_seconds( player->current.distance / 10.0 ); }

    bool ready() override
    { return ( player->current.distance > 1 ); }

    bool usable_moving() const override
    { return true; }
  };

  elemental type;
  elemental_variant variant;

  primal_elemental_t( shaman_t* owner, elemental type_, elemental_variant variant_ )
    : shaman_pet_t( owner, elemental_name( type_, variant_ ), !is_pet_elemental( type_ ),
                    elemental_autoattack( type_ ) ),
      type( type_ ), variant( variant_ )
  { }

  void create_default_apl() override
  {
    if ( use_auto_attack )
    {
      // Travel must come before auto attacks
      action_priority_list_t* def = get_action_priority_list( "default" );
      def->add_action( "travel" );
    }

    shaman_pet_t::create_default_apl();
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "travel" )
      return new travel_t( this );

    return shaman_pet_t::create_action( name, options_str );
  }

  double composite_attack_power_multiplier() const override
  {
    double m = pet_t::composite_attack_power_multiplier();

    m *= 1.0 + o()->talent.primal_elementalist->effectN( 1 ).percent();

    return m;
  }

  double composite_spell_power_multiplier() const override
  {
    double m = pet_t::composite_spell_power_multiplier();

    m *= 1.0 + o()->talent.primal_elementalist->effectN( 1 ).percent();

    return m;
  }

  attack_t* create_auto_attack() override
  {
    auto attack               = new pet_melee_attack_t<primal_elemental_t>( this, "melee" );
    attack->background        = true;
    attack->repeating         = true;
    attack->special           = false;
    attack->school            = SCHOOL_PHYSICAL;
    attack->weapon_multiplier = 1.0;
    return attack;
  }
};

// ==========================================================================
// Earth Elemental
// ==========================================================================

struct earth_elemental_t : public primal_elemental_t
{
  earth_elemental_t( shaman_t* owner, elemental type_, elemental_variant variant_ ) :
    primal_elemental_t( owner, type_, variant_ )
  {
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_sp      = 0.25;

    npc_id = type_ == elemental::GREATER_EARTH ? 95072 : 187322;
  }
};

// ==========================================================================
// Fire Elemental
// ==========================================================================

struct fire_elemental_t : public primal_elemental_t
{
  cooldown_t* meteor_cd;

  fire_elemental_t( shaman_t* owner, elemental type_, elemental_variant variant_ ) :
    primal_elemental_t( owner, type_, variant_ )
  {
    owner_coeff.sp_from_sp = variant == elemental_variant::GREATER ? 1.0 : 0.65;
    switch ( type_ )
    {
      case elemental::GREATER_FIRE:
        npc_id = variant == elemental_variant::GREATER ? 95061 : 229800;
        break;
      case elemental::PRIMAL_FIRE:
        npc_id = variant == elemental_variant::GREATER ? 61029 : 229799;
        break;
      default:
        break;
    }

    meteor_cd = get_cooldown( "meteor" );
  }

  struct meteor_t : public pet_spell_t<fire_elemental_t>
  {
    meteor_t( fire_elemental_t* player, util::string_view options )
      : super( player, "meteor", player->find_spell( 117588 ), options )
    {
      aoe = -1;
    }
  };

  struct fire_blast_t : public pet_spell_t<fire_elemental_t>
  {
    fire_blast_t( fire_elemental_t* player, util::string_view options )
      : super( player, "fire_blast", player->find_spell( 57984 ), options )
    { }

    bool usable_moving() const override
    { return true; }
  };

  struct immolate_t : public pet_spell_t<fire_elemental_t>
  {
    immolate_t( fire_elemental_t* player, util::string_view options )
      : super( player, "immolate", player->find_spell( 118297 ), options )
    {
      hasted_ticks = tick_may_crit = true;
    }
  };

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );

    if ( type == elemental::PRIMAL_FIRE )
    {
      def->add_action( "meteor" );
      def->add_action( "immolate,target_if=!ticking" );
    }

    def->add_action( "fire_blast" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "fire_blast" )
      return new fire_blast_t( this, options_str );
    if ( name == "meteor" )
      return new meteor_t( this, options_str );
    if ( name == "immolate" )
      return new immolate_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
  }

  void summon( timespan_t duration ) override
  {
    primal_elemental_t::summon( duration );

    if ( type == elemental::PRIMAL_FIRE )
    {
      meteor_cd->reset( false );
    }
  }

  void dismiss( bool expired ) override
  {
    primal_elemental_t::dismiss( expired );

    if ( variant == elemental_variant::GREATER && o()->talent.echo_of_the_elementals.ok() && expired )
    {
      o()->summon_lesser_elemental( type );
    }
  }
};

// ==========================================================================
// Storm Elemental
// ==========================================================================

struct storm_elemental_t : public primal_elemental_t
{
  struct tempest_aoe_t : public pet_spell_t<storm_elemental_t>
  {
    int tick_number   = 0;
    double damage_amp = 0.0;

    tempest_aoe_t( storm_elemental_t* player, util::string_view options )
      : super( player, "tempest_aoe", player->find_spell( 269005 ), options )
    {
      aoe        = -1;
      background = true;

      // parent spell (tempest_t) has the damage increase percentage
      damage_amp = player->o()->find_spell( 157375 )->effectN( 2 ).percent();
    }

    double action_multiplier() const override
    {
      double m = pet_spell_t::action_multiplier();
      m *= std::pow( 1.0 + damage_amp, tick_number );
      return m;
    }
  };

  struct tempest_t : public pet_spell_t<storm_elemental_t>
  {
    tempest_aoe_t* breeze = nullptr;

    tempest_t( storm_elemental_t* player, util::string_view options )
      : super( player, "tempest", player->find_spell( 157375 ), options )
    {
      channeled   = true;
      tick_action = breeze = new tempest_aoe_t( player, options );
    }

    void tick( dot_t* d ) override
    {
      breeze->tick_number = d->current_tick;
      pet_spell_t::tick( d );
    }

    bool ready() override
    {
      if ( p()->o()->talent.primal_elementalist->ok() )
      {
        return pet_spell_t<storm_elemental_t>::ready();
      }
      return false;
    }
  };

  struct wind_gust_t : public pet_spell_t<storm_elemental_t>
  {
    wind_gust_t( storm_elemental_t* player, util::string_view options )
      : super( player, "wind_gust", player->find_spell( 157331 ), options )
    { }
  };

  struct call_lightning_t : public pet_spell_t<storm_elemental_t>
  {
    call_lightning_t( storm_elemental_t* player, util::string_view options )
      : super( player, "call_lightning", player->find_spell( 157348 ), options )
    { }

    void execute() override
    {
      super::execute();

      p()->call_lightning->trigger();
    }
  };

  buff_t* call_lightning;
  cooldown_t* tempest_cd;

  storm_elemental_t( shaman_t* owner, elemental type_, elemental_variant variant_ )
    : primal_elemental_t( owner, type_, variant_ ), call_lightning( nullptr )
  {
    owner_coeff.sp_from_sp = variant == elemental_variant::GREATER ? 1.0 : 0.65;
    switch ( type_ )
    {
      case elemental::GREATER_STORM:
        npc_id = variant == elemental_variant::GREATER ? 77936 : 229801;
        break;
      case elemental::PRIMAL_STORM:
        npc_id = variant == elemental_variant::GREATER ? 77942 : 229798;
        break;
      default:
        break;
    }

    tempest_cd = get_cooldown( "tempest" );
  }

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( type == elemental::PRIMAL_STORM )
    {
      def->add_action( "tempest,if=buff.call_lightning.remains>=10" );
    }
    def->add_action( "call_lightning" );
    def->add_action( "wind_gust" );
  }

  void create_buffs() override
  {
    primal_elemental_t::create_buffs();

    call_lightning = make_buff( this, "call_lightning",
      find_spell( 157348 ) )->set_cooldown( timespan_t::zero() );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = primal_elemental_t::composite_player_multiplier( school );

    if ( call_lightning->up() )
    {
      m *= 1.0 + call_lightning->data().effectN( 2 ).percent();
    }

    return m;
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "tempest" )
      return new tempest_t( this, options_str );
    if ( name == "call_lightning" )
      return new call_lightning_t( this, options_str );
    if ( name == "wind_gust" )
      return new wind_gust_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
  }

  void summon( timespan_t duration ) override
  {
    primal_elemental_t::summon( duration );

    if ( type == elemental::PRIMAL_STORM )
    {
      tempest_cd->reset( false );
    }

    o()->buff.wind_gust->reset();
  }

  void dismiss( bool expired ) override
  {
    primal_elemental_t::dismiss( expired );

    if ( variant == elemental_variant::GREATER && o()->talent.echo_of_the_elementals.ok() && expired )
    {
      o()->summon_lesser_elemental( type );
    }

    if ( o()->pet.storm_elemental.n_active_pets() + o()->pet.lesser_storm_elemental.n_active_pets() == 0 )
    {
      o()->buff.wind_gust->expire();
    }
  }
};

// ==========================================================================
// Greater Lightning Elemental (Fury of the Storms Talent)
// ==========================================================================

struct greater_lightning_elemental_t : public shaman_pet_t
{
  struct lightning_blast_t : public pet_spell_t<greater_lightning_elemental_t>
  {
    lightning_blast_t( greater_lightning_elemental_t* p, util::string_view options ) :
      super( p, "lightning_blast", p->find_spell( 191726 ), options )
    {
      ability_lag = { timespan_t::from_millis( 300 ), timespan_t::from_millis( 25 ) };
    }
  };

  struct chain_lightning_t : public pet_spell_t<greater_lightning_elemental_t>
  {
    chain_lightning_t( greater_lightning_elemental_t* p, util::string_view options ) :
      super( p, "chain_lightning", p->find_spell( 191732 ), options )
    {
      if ( data().effectN( 1 ).chain_multiplier() != 0 )
      {
        chain_multiplier = data().effectN( 1 ).chain_multiplier();
      }

      ability_lag = { timespan_t::from_millis( 300 ), timespan_t::from_millis( 25 ) };
    }
  };

  greater_lightning_elemental_t( shaman_t* owner ) :
    shaman_pet_t( owner, "greater_lightning_elemental", true, false )
  {
    owner_coeff.sp_from_sp = 1.0;
    npc_id = 97022;
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "lightning_blast" ) return new lightning_blast_t( this, options_str );
    if ( name == "chain_lightning" ) return new chain_lightning_t( this, options_str );

    return shaman_pet_t::create_action( name, options_str );
  }

  void create_default_apl() override
  {
    shaman_pet_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );

    def -> add_action( "chain_lightning,if=spell_targets.chain_lightning>1" );
    def -> add_action( "lightning_blast" );
  }
};

// ==========================================================================
// Ancestor (Call of the Ancestors Talent)
// ==========================================================================

struct ancestor_t : public shaman_pet_t
{
  action_t* lava_burst;
  action_t* chain_lightning;
  action_t* elemental_blast;

  struct lava_burst_t : public pet_spell_t<ancestor_t>
  {
    lava_burst_t( ancestor_t* p ) : super( p, "lava_burst", p->find_spell( 447419 ) )
    {
      background = true;
      base_crit = 1.0;
    }
  };

  struct chain_lightning_t : public pet_spell_t<ancestor_t>
  {
    chain_lightning_t( ancestor_t* p ) : super( p, "chain_lightning", p->find_spell( 447425 ) )
    { background = true; }
  };

  struct elemental_blast_t : public pet_spell_t<ancestor_t>
  {
    elemental_blast_t( ancestor_t* p ) : super( p, "elemental_blast", p->find_spell( 447427 ) )
    {
        background = true;
        spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    }

    void execute() override
    {
      o()->trigger_elemental_blast_proc();
      pet_spell_t::execute();
    }
  };

  ancestor_t( shaman_t* owner ) : shaman_pet_t( owner, "ancestor", true, false ),
    lava_burst( nullptr ), chain_lightning( nullptr ), elemental_blast( nullptr )
  {
    owner_coeff.sp_from_sp = 1.0;
    npc_id = 221177;
  }

  void init_background_actions() override
  {
    shaman_pet_t::init_background_actions();

    lava_burst = new lava_burst_t( this );
    chain_lightning = new chain_lightning_t( this );
    elemental_blast = new elemental_blast_t( this );
  }

  void trigger_cast( ancestor_cast type, player_t* target )
  {
    switch ( type )
    {
      case ancestor_cast::LAVA_BURST:
        lava_burst->execute_on_target( target );
        break;
      case ancestor_cast::CHAIN_LIGHTNING:
        chain_lightning->execute_on_target( target );
        break;
      case ancestor_cast::ELEMENTAL_BLAST:
        elemental_blast->execute_on_target( target );
        break;
      default:
        break;
    }
  }

  void dismiss( bool expiration ) override
  {
    if ( expiration && o()->talent.final_calling.ok() )
    {
      // Pick a random target to shoot the Elemental Blast on for now
      if ( !elemental_blast->target_list().empty() )
      {
        auto idx = static_cast<unsigned>( rng().range(
          as<double>( elemental_blast->target_list().size() ) ) );

        trigger_cast( ancestor_cast::ELEMENTAL_BLAST, elemental_blast->target_list()[ idx ] );
      }
    }

    shaman_pet_t::dismiss( expiration );
    if ( expiration && o()->rng_obj.ancient_fellowship->trigger() )
    {
      o()->summon_ancestor();
    }
  }
};
}  // end namespace pet

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct stormblast_t : public shaman_attack_t
{
  stormblast_t( shaman_t* p, util::string_view name ) :
    shaman_attack_t( name, p, p->find_spell( 390287 ) )
  {
    weapon = &( p->main_hand_weapon );
    background = may_crit = callbacks = false;

      affected_by_enh_mastery_da = true;
      // [BUG] 2024-07-17: Stormblast has a mystery 50% damage bonus multiplier in-game
      base_dd_multiplier *= p->bugs ? 1.5 : 1.0;
  }

  void init() override
  {
    shaman_attack_t::init();

    snapshot_flags = update_flags = ~STATE_MUL_PLAYER_DAM & ( STATE_MUL_DA | STATE_TGT_MUL_DA );

    may_proc_windfury = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_ability_procs = false;

    p()->set_mw_proc_state( this, mw_proc_state::DISABLED );
  }
};

struct lightning_rod_damage_t : public shaman_spell_t
{
  lightning_rod_damage_t( shaman_t* p ) :
    shaman_spell_t( "lightning_rod", p, p->find_spell( 197568 ) )
  {
    background = true;
    may_crit = false;
  }

  void init() override
  {
    shaman_spell_t::init();

    snapshot_flags = update_flags = STATE_TGT_MUL_DA;
  }
};

struct tempest_strikes_damage_t : public shaman_spell_t
{
  tempest_strikes_damage_t( shaman_t* p ) :
    shaman_spell_t( "tempest_strikes", p, p->find_spell( 428078 ) )
  {
    background = true;
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t  // flametongue_attack
{
  flametongue_weapon_spell_t( util::string_view n, shaman_t* player, weapon_t* /* w */ )
    : shaman_spell_t( n, player, player->find_spell( 10444 ) )
  {
    may_crit = background      = true;

    snapshot_flags          = STATE_AP;
    attack_power_mod.direct = 0.0198;

    if ( player->main_hand_weapon.type != WEAPON_NONE )
    {
      attack_power_mod.direct *= player->main_hand_weapon.swing_time.total_seconds() / 2.6;
    }
  }
};

struct windfury_attack_t : public shaman_attack_t
{
  struct
  {
    std::array<proc_t*, 6> at_fw;
  } stats_;

  windfury_attack_t( util::string_view n, shaman_t* player, const spell_data_t* s, weapon_t* w )
    : shaman_attack_t( n, player, s )
  {
    weapon     = w;
    school     = SCHOOL_PHYSICAL;
    background = true;

    // Windfury can not proc itself
    may_proc_windfury = false;

    for ( size_t i = 0; i < stats_.at_fw.size(); i++ )
    {
      stats_.at_fw[ i ] = player->get_proc( "Windfury-ForcefulWinds: " + std::to_string( i ) );
    }
  }

  void init_finished() override
  {
    shaman_attack_t::init_finished();

    if ( may_proc_stormbringer )
    {
      if ( weapon->slot == SLOT_MAIN_HAND )
      {
        proc_sb = player->get_proc( std::string( "Stormbringer: " ) + full_name() );
      }

      if ( weapon->slot == SLOT_OFF_HAND )
      {
        proc_sb = player->get_proc( std::string( "Stormbringer: " ) + full_name() + " Off-Hand" );
      }
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    m *= 1.0 + p()->buff.forceful_winds->stack_value();

    if ( p()->buff.doom_winds->up() )
    {
      m *= 1.0 + p()->talent.doom_winds->effectN( 2 ).percent();
    }

    if ( p()->talent.imbuement_mastery.ok() )
    {
     m *= 1.0 + p()->talent.imbuement_mastery->effectN( 2 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    if ( p()->talent.forceful_winds->ok() )
    {
      stats_.at_fw[ p()->buff.forceful_winds->check() ]->occur();
    }
  }
};

struct crash_lightning_attack_t : public shaman_attack_t
{
  crash_lightning_attack_t( shaman_t* p ) : shaman_attack_t( "crash_lightning_proc", p, p->find_spell( 195592 ) )
  {
    weapon     = &( p->main_hand_weapon );
    background = true;
    aoe        = -1;
    may_proc_ability_procs = false;
    reduced_aoe_targets = 6.0;
    full_amount_targets = 1;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = false;
  }
};

struct icy_edge_attack_t : public shaman_attack_t
{
  icy_edge_attack_t( util::string_view n, shaman_t* p, weapon_t* w ) : shaman_attack_t( n, p, p->find_spell( 271920 ) )
  {
    weapon                 = w;
    background             = true;
    may_proc_ability_procs = false;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = false;
  }
};

struct stormstrike_attack_state_t : public shaman_action_state_t
{
  bool stormbringer;

  stormstrike_attack_state_t( action_t* action_, player_t* target_ ) :
    shaman_action_state_t( action_, target_ ), stormbringer( false )
  { }

  void initialize() override
  {
    shaman_action_state_t::initialize();

    stormbringer = false;
  }

  void copy_state( const action_state_t* s ) override
  {
    shaman_action_state_t::copy_state( s );

    auto lbs = debug_cast<const stormstrike_attack_state_t*>( s );
    stormbringer= lbs->stormbringer;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    shaman_action_state_t::debug_str( s );

    s << " stormbringer=" << stormbringer;

    return s;
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  bool stormbringer;
  strike_variant strike_type;

  action_t* stormblast;

  stormstrike_attack_t( util::string_view n, shaman_t* player, const spell_data_t* s, weapon_t* w,
                        strike_variant sf = strike_variant::NORMAL )
    : shaman_attack_t( n, player, s ), stormbringer( false ), strike_type( sf ), stormblast( nullptr )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
    weapon = w;
    school = SCHOOL_PHYSICAL;

    if ( player->talent.stormblast.ok() )
    {
      std::string name_str { "stormblast_" };
      name_str += n;
      stormblast = new stormblast_t( player, name_str );
      add_child( stormblast );
    }
  }

  action_state_t* new_state() override
  { return new stormstrike_attack_state_t( this, target ); }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p()->buff.converging_storms->up() )
    {
      m *= 1.0 + p()->buff.converging_storms->check_stack_value();
    }

    if ( strike_type == strike_variant::STORMFLURRY )
    {
      m *= p()->talent.stormflurry->effectN( 2 ).percent();
    }

    if ( strike_type == strike_variant::WHIRLING_AIR )
    {
      m *= p()->buff.whirling_air->data().effectN( 4 ).percent();
    }

    m *= 1.0 + p()->buff.whirling_air->stack_value();

    return m;
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    shaman_attack_t::snapshot_internal( s, flags, rt );

    auto state = debug_cast<stormstrike_attack_state_t*>( s );
    state->stormbringer = stormbringer;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    stormbringer = false;
  }

  void impact( action_state_t* s ) override
  {
    shaman_attack_t::impact( s );

    auto state = debug_cast<stormstrike_attack_state_t*>( s );

    if ( state->stormbringer && p()->talent.stormblast.ok() && result_is_hit( state->result ) )
    {
      auto dmg = p()->talent.stormblast->effectN( 1 ).percent() * state->result_amount;
      stormblast->base_dd_min = stormblast->base_dd_max = dmg;

      stormblast->execute_on_target( state->target );
    }
  }
};

struct windstrike_attack_t : public stormstrike_attack_t
{
  windstrike_attack_t( util::string_view n, shaman_t* player, const spell_data_t* s, weapon_t* w,
                       strike_variant sf = strike_variant::NORMAL )
    : stormstrike_attack_t( n, player, s, w, sf )
  { }

  double composite_target_armor( player_t* ) const override
  {
    return 0.0;
  }
};

struct windlash_t : public shaman_attack_t
{
  double swing_timer_variance;

  windlash_t( util::string_view n, const spell_data_t* s, shaman_t* player, weapon_t* w, double stv )
    : shaman_attack_t( n, player, s ), swing_timer_variance( stv )
  {
    background = repeating = may_miss = may_dodge = may_parry = true;
    may_proc_ability_procs = may_glance = special = false;
    weapon                                        = w;
    weapon_multiplier                             = 1.0;
    base_execute_time                             = w->swing_time;
    trigger_gcd                                   = timespan_t::zero();
  }

  // Windlash is a special ability, but treated as an autoattack in terms of proccing
  proc_types proc_type() const override
  {
    return PROC1_MELEE;
  }

  double composite_target_armor( player_t* ) const override
  {
    return 0.0;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = shaman_attack_t::execute_time();

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds(
          const_cast<windlash_t*>( this )->rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim->debug )
        sim->out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(),
                               t.total_seconds(), st.total_seconds() );

      return st;
    }
    else
      return t;
  }
};

// Ground AOE pulse
struct ground_aoe_spell_t : public spell_t
{
  ground_aoe_spell_t( shaman_t* p, util::string_view name, const spell_data_t* spell ) : spell_t( name, p, spell )
  {
    aoe        = -1;
    callbacks  = false;
    ground_aoe = background = may_crit = true;
  }
};

struct lightning_shield_damage_t : public shaman_spell_t
{
  lightning_shield_damage_t( shaman_t* player )
    : shaman_spell_t( "lightning_shield", player, player->find_spell( 273324 ) )
  {
    background = true;
    callbacks  = false;
  }
};

struct lightning_shield_defense_damage_t : public shaman_spell_t
{
  lightning_shield_defense_damage_t( shaman_t* player )
    : shaman_spell_t( "lifghtning_shield_defense_damage", player, player->find_spell( 192109 ) )
  {
    background = true;
    callbacks  = false;
  }
};

struct awakening_storms_t : public shaman_spell_t
{
  awakening_storms_t( shaman_t* player ) :
    shaman_spell_t( "awakening_storms", player, player->find_spell( 455130 ))
  {
    background = true;
  }
};

// Elemental overloads

struct elemental_overload_spell_t : public shaman_spell_t
{
  shaman_spell_t* parent;

  elemental_overload_spell_t( shaman_t* p, util::string_view name, const spell_data_t* s,
                              shaman_spell_t* parent_, double multiplier = -1.0,
                              spell_variant type_ = spell_variant::NORMAL )
    : shaman_spell_t( name, p, s, type_ ), parent( parent_ )
  {
    base_execute_time = timespan_t::zero();
    background        = true;
    callbacks         = false;

    base_multiplier *=
        p->mastery.elemental_overload->effectN( 2 ).percent() +
        p->talent.echo_chamber->effectN( 1 ).percent();

    // multiplier is used by Mountains Will Fall and is applied after
    // overload damage multiplier is calculated.
    if ( multiplier != -1.0 )
    {
      base_multiplier *= multiplier;
    }
  }

  void init_finished() override
  {
    shaman_spell_t::init_finished();

    // Generate a new stats object for the elemental overload spell based on the parent
    // stats object name. This will more or less always let us build correct stats
    // hierarchies for the overload-capable spells, so that the various different
    // (reporting) hierarchies function correctly.
    /*
    auto stats_ = player->get_stats( parent->stats->name_str + "_overload", this );
    stats_->school = get_school();
    stats = stats_;
    parent->stats->add_child( stats );
    */
    parent->add_child( this );
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    shaman_spell_t::snapshot_internal( s, flags, rt );

    cast_state( s )->exec_type = parent->exec_type;
  }
};


struct thunderstrike_ward_damage_t : public shaman_spell_t
{
  thunderstrike_ward_damage_t( shaman_t* player )
    : shaman_spell_t( "thunderstrike", player, player->find_spell( 462763 ) )
  {

    background = true;
  }

  double action_da_multiplier() const override
  {
    double m = shaman_spell_t::action_da_multiplier();

    if ( p()->talent.enhanced_imbues->ok() )
    {
      m *= 1.0 + p()->talent.enhanced_imbues->effectN( 9 ).percent();
    }

    return m;
  }
};

struct earthen_rage_damage_t : public shaman_spell_t
{
  earthen_rage_damage_t( shaman_t* p ) : shaman_spell_t( "earthen_rage", p, p -> find_spell( 170379 ) )
  {
    background = proc = true;
    callbacks = false;
  }
};

struct earthen_rage_event_t : public event_t
{
  shaman_t* player;
  timespan_t end_time;

  earthen_rage_event_t( shaman_t* p, timespan_t et ) :
    event_t( *p, next_event( p ) ), player( p ), end_time( et )
  { }

  timespan_t next_event( shaman_t* p ) const
  {
    return p->rng().range(
      1_ms,
      2.0 * p->spell.earthen_rage->effectN( 1 ).period() * p->composite_spell_cast_speed()
    );
  }

  void set_end_time( timespan_t t )
  { end_time = t; }

  void execute() override
  {
    if ( sim().current_time() > end_time )
    {
      sim().print_debug( "{} earthen_rage fades", player->name() );
      player->earthen_rage_event = nullptr;
      return;
    }

    sim().print_debug( "{} triggers earthen_rage on target={}", player->name(),
      player->earthen_rage_target->name() );

    player->action.earthen_rage->execute_on_target( player->earthen_rage_target );

    player->earthen_rage_event = make_event<earthen_rage_event_t>( sim(), player, end_time );
    sim().print_debug( "{} schedules earthen_rage, next_event={}, tick_time={}",
      player->name(), player->earthen_rage_event->occurs(),
      player->earthen_rage_event->occurs() - sim().current_time() );
  }
};

// Honestly why even bother with resto heals?
// shaman_heal_t::impact ====================================================

void shaman_heal_t::impact( action_state_t* s )
{
  // Todo deep healing to adjust s -> result_amount by x% before impacting
  if ( sim->debug && p()->mastery.deep_healing->ok() )
  {
    sim->out_debug.printf( "%s Deep Heals %s@%.2f%% mul=%.3f %.0f -> %.0f", player->name(), s->target->name(),
                           s->target->health_percentage(), deep_healing( s ), s->result_amount,
                           s->result_amount * deep_healing( s ) );
  }

  s->result_amount *= deep_healing( s );

  base_t::impact( s );

  if ( proc_tidal_waves )
    p()->buff.tidal_waves->trigger( p()->buff.tidal_waves->data().initial_stacks() );

  if ( s->result == RESULT_CRIT )
  {
    if ( resurgence_gain > 0 )
      p()->resource_gain( RESOURCE_MANA, resurgence_gain, p()->gain.resurgence );
  }

  if ( p()->main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
  {
    double chance = ( s->target->resources.pct( RESOURCE_HEALTH ) > .35 ) ? elw_proc_high : elw_proc_low;

    if ( rng().roll( chance ) )
    {
      // Todo proc earthliving on target
    }
  }
}

// ==========================================================================
// Shaman Attack
// ==========================================================================

// shaman_attack_t::impact ============================================

// Melee Attack =============================================================

struct melee_t : public shaman_attack_t
{
  int sync_weapons;
  bool first;
  double swing_timer_variance;

  melee_t( util::string_view name, const spell_data_t* s, shaman_t* player, weapon_t* w, int sw, double stv )
    : shaman_attack_t( name, player, s ), sync_weapons( sw ), first( true ), swing_timer_variance( stv )
  {
    id                                  = w->slot == SLOT_MAIN_HAND ? 1U : 2U;
    background = repeating = may_glance = true;
    allow_class_ability_procs           = true;
    not_a_proc                          = true;
    special                             = false;
    trigger_gcd                         = timespan_t::zero();
    weapon                              = w;
    weapon_multiplier                   = 1.0;
    base_execute_time                   = w->swing_time;

    if ( p()->specialization() == SHAMAN_ENHANCEMENT && p()->dual_wield() )
      base_hit -= 0.19;

    may_proc_flametongue      = true;
  }

  void reset() override
  {
    shaman_attack_t::reset();

    first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = shaman_attack_t::execute_time();
    if ( first )
    {
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 )
                                               : timespan_t::zero();
    }

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds(
          const_cast<melee_t*>( this )->rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim->debug )
        sim->out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(),
                               t.total_seconds(), st.total_seconds() );
      return st;
    }
    else
      return t;
  }

  void execute() override
  {
    if ( first )
    {
      first = false;
    }

    shaman_attack_t::execute();
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public shaman_attack_t
{
  int sync_weapons;
  double swing_timer_variance;

  auto_attack_t( shaman_t* player, util::string_view options_str )
    : shaman_attack_t( "auto_attack", player, spell_data_t::nil() ), sync_weapons( 0 ), swing_timer_variance( 0.00 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    add_option( opt_float( "swing_timer_variance", swing_timer_variance ) );
    parse_options( options_str );
    ignore_false_positive  = true;
    may_proc_ability_procs = false;

    assert( p()->main_hand_weapon.type != WEAPON_NONE );

    p()->melee_mh = new melee_t( "Main Hand", spell_data_t::nil(), player, &( p()->main_hand_weapon ), sync_weapons,
                                 swing_timer_variance );
    p()->melee_mh->school = SCHOOL_PHYSICAL;

    if ( ( player->talent.deeply_rooted_elements.ok() || player->talent.ascendance.ok() ) &&
          player->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->ascendance_mh = new windlash_t( "Windlash", player->find_spell( 114089 ), player, &( p()->main_hand_weapon ),
                                           swing_timer_variance );
    }

    p()->main_hand_attack = p()->melee_mh;

    if ( p()->off_hand_weapon.type != WEAPON_NONE && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( !p()->dual_wield() )
        return;

      p()->melee_oh = new melee_t( "Off-Hand", spell_data_t::nil(), player, &( p()->off_hand_weapon ), sync_weapons,
                                   swing_timer_variance );
      p()->melee_oh->school = SCHOOL_PHYSICAL;

      if ( player->talent.deeply_rooted_elements.ok() || player->talent.ascendance.ok() )
      {
        p()->ascendance_oh = new windlash_t( "Windlash Off-Hand", player->find_spell( 114093 ), player,
            &( p()->off_hand_weapon ), swing_timer_variance );
      }

      p()->off_hand_attack = p()->melee_oh;
    }

    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    p()->main_hand_attack->schedule_execute();
    if ( p()->off_hand_attack )
      p()->off_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( p()->is_moving() )
      return false;
    return ( p()->main_hand_attack->execute_event == nullptr );  // not swinging
  }
};

// Molten Weapon Dot ============================================================

struct molten_weapon_dot_t : public residual_action::residual_periodic_action_t<spell_t>
{
  molten_weapon_dot_t( shaman_t* p ) : base_t( "molten_weapon", p, p->find_spell( 271924 ) )
  {
    // spell data seems messed up - need to whitelist?
    dual           = true;
    dot_duration   = timespan_t::from_seconds( 4 );
    base_tick_time = timespan_t::from_seconds( 2 );
    tick_zero      = false;
    hasted_ticks   = false;
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  molten_weapon_dot_t* mw_dot;
  unsigned max_spread_targets;

  lava_lash_t( shaman_t* player, util::string_view options_str ) :
    shaman_attack_t( "lava_lash", player, player->talent.lava_lash ),
    mw_dot( nullptr ),
    max_spread_targets( as<unsigned>( p()->talent.molten_assault->effectN( 2 ).base_value() ) )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    school = SCHOOL_FIRE;
    // Add a 12 yard radius to support Flame Shock spreading in 11.0
    radius = 12.0;

    parse_options( options_str );
    weapon = &( player->off_hand_weapon );

    if ( weapon->type == WEAPON_NONE )
      background = true;  // Do not allow execution.

    if ( player->talent.elemental_spirits->ok() )
    {
      mw_dot = new molten_weapon_dot_t( player );
      add_child( mw_dot );
    }
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_stormbringer = true;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = shaman_attack_t::recharge_multiplier( cd );

    if ( p()->buff.hot_hand->check() )
    {
      m /= 1.0 + p()->buff.hot_hand->stack_value();
    }

    return m;
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p()->buff.hot_hand->up() )
    {
      m *= 1.0 + p()->talent.hot_hand->effectN( 3 ).percent();
    }

    // Flametongue imbue only increases Lava Lash damage if it is imbued on the off-hand
    // weapon
    if ( p()->off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      m *= 1.0 + data().effectN( 2 ).percent();
    }

    m *= 1.0 + p()->buff.ashen_catalyst->stack_value();

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = shaman_attack_t::composite_crit_chance();

    c += p()->buff.whirling_fire->value();

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double c = shaman_attack_t::composite_crit_damage_bonus_multiplier();

    if ( p()->buff.whirling_fire->check() )
    {
      c *= 1.0 + p()->buff.whirling_fire->data().effectN( 2 ).percent();
    }

    return c;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->trigger_elemental_assault( execute_state );
    p()->trigger_tempest_strikes( execute_state );
    p()->trigger_reactivity( execute_state );

    p()->buff.ashen_catalyst->expire();
    p()->buff.whirling_fire->decrement();

    if ( p()->rng_obj.lively_totems->trigger() )
    {
      // 2024-07-10: Searing Totem death seems to be delayed from basically nothing to approximately
      // 850ms. Makes it possible to get an extra searing bolt if the timing is right.
      p()->pet.searing_totem.spawn( timespan_t::from_seconds( 8.0 + rng().range( 0.85 ) ) );
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    td( state->target )->debuff.lashing_flames->trigger();

    trigger_flame_shock( state );

    if ( result_is_hit( state->result ) && p()->buff.crash_lightning->up() )
    {
      p()->action.crash_lightning_aoe->set_target( state->target );
      p()->action.crash_lightning_aoe->schedule_execute();
    }
  }

  void move_random_target( std::vector<player_t*>& in, std::vector<player_t*>& out ) const
  {
    auto idx = rng().range( 0U, as<unsigned>( in.size() ) );
    out.push_back( in[ idx ] );
    in.erase( in.begin() + idx );
  }

  static std::string actor_list_str( const std::vector<player_t*>& actors,
                                     util::string_view             delim = ", " )
  {
    static const auto transform_fn = []( player_t* t ) { return t->name(); };
    std::vector<const char*> tmp;

    range::transform( actors, std::back_inserter( tmp ), transform_fn );

    return tmp.size() ? util::string_join( tmp, delim ) : "none";
  }

  void trigger_flame_shock( const action_state_t* state ) const
  {
    if ( !p()->talent.molten_assault->ok() )
    {
      return;
    }

    if ( !td( state->target )->dot.flame_shock->is_ticking() )
    {
      return;
    }

    // Targets to spread Flame Shock to
    std::vector<player_t*> targets;
    // Maximum number of spreads, deduct one from available targets since the target of this Lava
    // Lash execution (always receives it) is in there
    unsigned actual_spread_targets = std::min( max_spread_targets,
        as<unsigned>( target_list().size() ) - 1U );

    if ( actual_spread_targets == 0 )
    {
      // Always trigger Flame Shock on main target
      p()->trigger_secondary_flame_shock( state->target );
      return;
    }

    // Lashing Flames, no Flame Shock
    std::vector<player_t*> lf_no_fs_targets,
    // Lashing Flames, Flame Shock
                           lf_fs_targets,
    // No Lashing Flames, no Flame Shock
                           no_lf_no_fs_targets,
    // No Lashing Flames, Flame Shock
                           no_lf_fs_targets;

    // Target of the Lava Lash has Lashing Flames
    bool mt_has_lf = td( state->target )->debuff.lashing_flames->check();

    // Categorize all available targets (within 8 yards of the main target) based on Lashing
    // Flames and Flame Shock state.
    range::for_each( target_list(), [&]( player_t* t ) {
      // Ignore main target
      if ( t == state->target )
      {
        return;
      }

      if ( td( t )->debuff.lashing_flames->check() &&
           !td( t )->dot.flame_shock->is_ticking() )
      {
        lf_no_fs_targets.push_back( t );
      }
      else if ( td( t )->debuff.lashing_flames->check() &&
                td( t )->dot.flame_shock->is_ticking() )
      {
        lf_fs_targets.push_back( t );
      }
      else if ( !td( t )->debuff.lashing_flames->check() &&
                 td( t )->dot.flame_shock->is_ticking() )
      {
        no_lf_fs_targets.push_back( t );
      }
      else if ( !td( t )->debuff.lashing_flames->check() &&
                !td( t )->dot.flame_shock->is_ticking() )
      {
        no_lf_no_fs_targets.push_back( t );
      }
    } );

    if ( sim->debug )
    {
      sim->out_debug.print( "{} spreads flame_shock, n_fs={} ll_target={} "
                            "state={}LF{}FS, targets_in_range={{ {} }}",
        player->name(), p()->active_flame_shock.size(), state->target->name(),
        td( state->target )->debuff.lashing_flames->check() ? '+' : '-',
        td( state->target )->dot.flame_shock->is_ticking() ? '+' : '-',
        actor_list_str( target_list() ) );

      sim->out_debug.print( "{} +LF-FS: targets={{ {} }}", player->name(),
          actor_list_str( lf_no_fs_targets ) );
      sim->out_debug.print( "{} -LF-FS: targets={{ {} }}", player->name(),
          actor_list_str( no_lf_no_fs_targets ) );
      sim->out_debug.print( "{} +LF+FS: targets={{ {} }}", player->name(),
          actor_list_str( lf_fs_targets ) );
      sim->out_debug.print( "{} -LF+FS: targets={{ {} }}", player->name(),
          actor_list_str( no_lf_fs_targets ) );
    }

    // 1) Randomly select targets with Lashing Flame and no Flame Shock, unless there already are
    // the maximum number of Flame Shocked targets with Lashing Flames up.
    while ( lf_no_fs_targets.size() > 0 &&
            ( lf_fs_targets.size() + mt_has_lf ) < p()->max_active_flame_shock &&
            targets.size() < actual_spread_targets )
    {
      move_random_target( lf_no_fs_targets, targets );
    }

    // 2) Randomly select targets without Lashing Flames and Flame Shock, but only if we are not at
    // Flame Shock cap.
    while ( no_lf_no_fs_targets.size() > 0 &&
            ( lf_fs_targets.size() + no_lf_fs_targets.size() + 1U ) < p()->max_active_flame_shock &&
            targets.size() < actual_spread_targets )
    {
      move_random_target( no_lf_no_fs_targets, targets );
    }

    // 3) Randomly select targets that have Lashing Flames and Flame Shock on them. This prioritizes
    // refreshing existing Flame Shocks on targets with Lashing Flames up.
    while ( lf_fs_targets.size() > 0 && targets.size() < actual_spread_targets )
    {
      move_random_target( lf_fs_targets, targets );
    }

    // 4) Randomly select targets that don't have Lashing Flames but have Flame Shock on them. This
    // prioritizes refreshing existing Flame Shocks on targets when we are at maximum Flame Shocks,
    // preventing random expirations.
    while ( no_lf_fs_targets.size() > 0 && targets.size() < actual_spread_targets )
    {
      move_random_target( no_lf_fs_targets, targets );
    }

    if ( sim->debug )
    {
      sim->out_debug.print( "{} selected targets={{ {} }}",
          player->name(), actor_list_str( targets ) );
    }

    // Always trigger Flame Shock on main target
    p()->trigger_secondary_flame_shock( state->target );

    range::for_each( targets, [ shaman = p() ]( player_t* target ) {
      shaman->trigger_secondary_flame_shock( target );
    } );
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_base_t : public shaman_attack_t
{
  struct stormflurry_event_t : public event_t
  {
    stormstrike_base_t* action;
    player_t* target;

    stormflurry_event_t( stormstrike_base_t* a, player_t* t, timespan_t delay ) :
      event_t( *a->player, delay ), action( a ), target( t )
    { }

    const char* name() const override
    { return "stormflurry_event"; }

    void execute() override
    {
      // Ensure we can execute on target, before doing anything
      if ( !action->target_ready( target ) )
      {
        action->p()->proc.stormflurry_failed->occur();
        return;
      }

      action->set_target( target );
      action->execute();
      action->p()->proc.stormflurry->occur();
    }
  };

  stormstrike_attack_t *mh, *oh;
  strike_variant strike_type;

  stormstrike_base_t( shaman_t* player, util::string_view name, const spell_data_t* spell,
                      util::string_view options_str, strike_variant t = strike_variant::NORMAL )
    : shaman_attack_t( name, player, spell ), mh( nullptr ), oh( nullptr ), strike_type( t )
  {
    parse_options( options_str );

    weapon             = &( p()->main_hand_weapon );
    weapon_multiplier  = 0.0;
    may_crit           = false;
    school             = SCHOOL_PHYSICAL;

    switch ( strike_type )
    {
      case strike_variant::STORMFLURRY:
      case strike_variant::WHIRLING_AIR:
        cooldown = player->get_cooldown( "__strike_secondary" );
        cooldown->duration = 0_ms;

        dual = true;
        background = true;
        base_costs[ RESOURCE_MANA ] = 0.0;
        break;
      default:
        cooldown = p()->cooldown.strike;
        cooldown->duration = data().cooldown();
        cooldown->action = this;
        break;
    }
  }

  void init() override
  {
    shaman_attack_t::init();
    may_proc_flametongue = may_proc_windfury = may_proc_stormbringer = false;

    p()->set_mw_proc_state( this, mw_proc_state::DISABLED );
  }

  void execute() override
  {
    shaman_attack_t::execute();

    auto stormbringer_state = strike_type == strike_variant::NORMAL && p()->buff.stormbringer->up();

    if ( stormbringer_state )
    {
      p()->buff.stormbringer->decrement();
    }

    if ( result_is_hit( execute_state->result ) )
    {
      mh->set_target( execute_state->target );
      mh->stormbringer = stormbringer_state;
      mh->execute();
      if ( oh )
      {
        oh->set_target( execute_state->target );
        oh->stormbringer = stormbringer_state;
        oh->execute();
      }

      if ( p()->buff.crash_lightning->up() )
      {
        p()->action.crash_lightning_aoe->set_target( execute_state->target );
        p()->action.crash_lightning_aoe->execute();
      }

      p()->trigger_tempest_strikes( execute_state );
    }

    p()->trigger_stormflurry( execute_state );

    p()->buff.converging_storms->expire();

    if ( strike_type == strike_variant::NORMAL )
    {
      p()->trigger_elemental_assault( execute_state );
    }

    if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->trigger_deeply_rooted_elements( execute_state );
    }

    if ( p()->sets->has_set_bonus( SHAMAN_ENHANCEMENT, T29, B2 ) )
    {
      p()->buff.t29_2pc_enh->trigger();
    }

    p()->trigger_awakening_storms( execute_state );
    p()->trigger_whirling_air( execute_state );
    p()->trigger_totemic_rebound( execute_state );
  }
};

struct stormstrike_t : public stormstrike_base_t
{
  stormstrike_t( shaman_t* player, util::string_view options_str, strike_variant sf = strike_variant::NORMAL )
    : stormstrike_base_t( player, "stormstrike", player->talent.stormstrike, options_str, sf )
  {
    // Actual damaging attacks are done by stormstrike_attack_t
    mh = new stormstrike_attack_t( "stormstrike_mh", player, data().effectN( 1 ).trigger(),
                                   &( player->main_hand_weapon ), strike_type );
    add_child( mh );

    if ( p()->off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new stormstrike_attack_t( "stormstrike_offhand", player, data().effectN( 2 ).trigger(),
                                     &( player->off_hand_weapon ), strike_type );
      add_child( oh );
    }
  }

  bool ready() override
  {
    if ( p()->buff.ascendance->check() )
      return false;

    return stormstrike_base_t::ready();
  }
};

// Windstrike Attack ========================================================

struct windstrike_t : public stormstrike_base_t
{
  windstrike_t( shaman_t* player, util::string_view options_str, strike_variant sf = strike_variant::NORMAL )
    : stormstrike_base_t( player, "windstrike", player->find_spell( 115356 ), options_str, sf )
  {
    // Actual damaging attacks are done by stormstrike_attack_t
    mh = new windstrike_attack_t( "windstrike_mh", player, data().effectN( 1 ).trigger(),
                                  &( player->main_hand_weapon ), strike_type );
    add_child( mh );

    if ( p()->off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new windstrike_attack_t( "windstrike_offhand", player, data().effectN( 2 ).trigger(),
                                    &( player->off_hand_weapon ), strike_type );
      add_child( oh );
    }
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    auto m = stormstrike_base_t::recharge_multiplier( cd );

    if ( p()->buff.ascendance->up() )
    {
      m *= 1.0 + p()->buff.ascendance->data().effectN( 4 ).percent();
    }

    return m;
  }

  bool ready() override
  {
    if ( p()->buff.ascendance->remains() <= cooldown->queue_delay() )
    {
      return false;
    }

    return stormstrike_base_t::ready();
  }

  void execute() override
  {
    stormstrike_base_t::execute();

    if ( strike_type == strike_variant::NORMAL &&
         p()->talent.thorims_invocation.ok() &&
         p()->buff.maelstrom_weapon->check() )
    {
      action_t* spell = nullptr;

      if ( p()->action.ti_trigger == p()->action.lightning_bolt_ti )
      {
        if ( p()->buff.tempest->check() )
        {
          spell = p()->action.tempest_ti;
        }
        else
        {
          spell = p()->action.ti_trigger;
        }
      }
      else if ( p()->action.ti_trigger == p()->action.chain_lightning_ti )
      {
        spell = p()->action.ti_trigger;
      }
      else
      {
        spell = p()->action.lightning_bolt_ti; // Default to lightning bolt if nothing is seen
      }

      spell->set_target( execute_state->target );
      spell->execute();
    }
  }
};

// Ice Strike Spell ========================================================

struct ice_strike_t : public shaman_attack_t
{
  ice_strike_t( shaman_t* player, util::string_view options_str )
    : shaman_attack_t( "ice_strike", player, player->talent.ice_strike )
  {
    parse_options( options_str );

    weapon = &( player->main_hand_weapon );
    weapon_multiplier = 0.0;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_flametongue = true;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->trigger_swirling_maelstrom( execute_state );
    p()->buff.ice_strike->trigger();

    if ( result_is_hit( execute_state->result ) && p()->buff.crash_lightning->up() )
    {
      p()->action.crash_lightning_aoe->set_target( execute_state->target );
      p()->action.crash_lightning_aoe->schedule_execute();
    }

    p()->trigger_elemental_assault( execute_state );
    p()->trigger_tempest_strikes( execute_state );
  }
};

// Sundering Spell =========================================================

struct sundering_t : public shaman_attack_t
{
  sundering_t( shaman_t* player, util::string_view options_str )
    : shaman_attack_t( "sundering", player, player->talent.sundering )
  {
    weapon = &( player->main_hand_weapon );

    parse_options( options_str );
    aoe    = -1;  // TODO: This is likely not going to affect all enemies but it will do for now
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_stormbringer = may_proc_flametongue = true;
  }

  double action_da_multiplier() const override
  {
    double m = shaman_attack_t::action_da_multiplier();

    // In-Game, Sundering damage double dips on T30 4PC damage buff
    if ( player->bugs )
    {
      m *= 1.0 + p()->buff.t30_4pc_enh_damage->value();
    }

    return m;
  }

  void execute() override
  {
    // In-game, Sundering that procs T30 4PC already benefits from the T30 damage buff
    p()->buff.t30_4pc_enh_damage->trigger();
    p()->buff.t30_4pc_enh_cl->trigger( p()->buff.t30_4pc_enh_cl->data().max_stacks() );

    shaman_attack_t::execute();

    p()->buff.t30_2pc_enh->trigger();
    p()->trigger_earthsurge( execute_state );

    if ( p()->buff.whirling_earth->up() )
    {
      p()->buff.whirling_earth->decrement();
      cooldown->adjust( -p()->buff.whirling_earth->data().effectN( 1 ).time_value() );
    }
  }
};

// Weapon imbues

struct weapon_imbue_t : public shaman_spell_t
{
  std::string slot_str;
  slot_e slot, default_slot;
  imbue_e imbue;
  buff_t* imbue_buff;

  weapon_imbue_t( util::string_view name, shaman_t* player, slot_e d_, const spell_data_t* spell, util::string_view options_str ) :
    shaman_spell_t( name, player, spell ), slot( SLOT_INVALID ), default_slot( d_ ), imbue( IMBUE_NONE ),
    imbue_buff( nullptr )
  {
    harmful = callbacks = false;
    target = player;

    add_option( opt_string( "slot", slot_str ) );

    parse_options( options_str );

    if ( slot_str.empty() )
    {
      slot = default_slot;
    }
    else
    {
      slot = util::parse_slot_type( slot_str );
    }
  }

  void init_finished() override
  {
    shaman_spell_t::init_finished();

    if ( player->items[ slot ].active() &&
         player->items[ slot ].selected_temporary_enchant() > 0 )
    {
      sim->error( "Player {} has a temporary enchant {} on slot {}, disabling {}",
        player->name(), util::slot_type_string( slot ),
        player->items[ slot ].selected_temporary_enchant(), name() );
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( slot == SLOT_MAIN_HAND && player->main_hand_weapon.type != WEAPON_NONE )
    {
      player->main_hand_weapon.buff_type = imbue;
    }
    else if ( slot == SLOT_OFF_HAND && player->off_hand_weapon.type != WEAPON_NONE )
    {
      player->off_hand_weapon.buff_type = imbue;
    }

    if ( imbue_buff != nullptr )
    {
      imbue_buff->trigger();
    }
  }

  bool ready() override
  {
    if ( slot == SLOT_INVALID )
    {
      return false;
    }

    if ( player->items[ slot ].active() &&
         player->items[ slot ].selected_temporary_enchant() > 0 )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Windfury Imbue =========================================================

struct windfury_weapon_t : public weapon_imbue_t
{
  windfury_weapon_t( shaman_t* player, util::string_view options_str ) :
    weapon_imbue_t( "windfury_weapon", player, SLOT_MAIN_HAND, player->talent.windfury_weapon,
                    options_str )
  {
    imbue = WINDFURY_IMBUE;
    imbue_buff = player->buff.windfury_weapon;

    if ( slot == SLOT_MAIN_HAND )
    {
      add_child( player->windfury_mh );
    }
    // Technically, you can put Windfury on the off-hand slot but it disables the proc
    else if ( slot == SLOT_OFF_HAND )
    {
      ;
    }
    else
    {
      sim->error( "{} invalid windfury slot '{}'", player->name(), slot_str );
    }
  }
};

// Flametongue Imbue =========================================================

struct flametongue_weapon_t : public weapon_imbue_t
{
  flametongue_weapon_t( shaman_t* player, util::string_view options_str ) :
    weapon_imbue_t( "flametongue_weapon", player,
                    player->specialization() == SHAMAN_ENHANCEMENT
                    ? SLOT_OFF_HAND
                    : SLOT_MAIN_HAND,
                    player->find_class_spell( "Flametongue Weapon" ), options_str )
  {
    imbue = FLAMETONGUE_IMBUE;
    imbue_buff = player->buff.flametongue_weapon;

    if ( slot == SLOT_MAIN_HAND || slot == SLOT_OFF_HAND )
    {
      add_child( player->flametongue );
    }
    else
    {
      sim->error( "{} invalid flametongue slot '{}'", player->name(), slot_str );
    }
  }
};

// Thunderstrike Ward Imbue ================================================

struct thunderstrike_ward_t : public weapon_imbue_t
{
  thunderstrike_ward_t( shaman_t* player, util::string_view options_str ) :
    weapon_imbue_t( "thunderstrike_ward", player, SLOT_OFF_HAND,
                    player->talent.thunderstrike_ward, options_str )
  {
    if ( !player->has_shield_equipped() )
    {
      sim->errorf( "%s: %s only usable with shield equipped in offhand\n", player->name(), name() );
    }
    else
    {
      imbue      = THUNDERSTRIKE_WARD;
      imbue_buff = player->buff.thunderstrike_ward;
    }
  }
};

// Crash Lightning Attack ===================================================

struct crash_lightning_t : public shaman_attack_t
{
  crash_lightning_t( shaman_t* player, util::string_view options_str )
    : shaman_attack_t( "crash_lightning", player, player->talent.crash_lightning )
  {
    parse_options( options_str );

    aoe     = -1;
    reduced_aoe_targets = 6.0;
    full_amount_targets = 1;
    weapon  = &( p()->main_hand_weapon );
    ap_type = attack_power_type::WEAPON_BOTH;
  }

  void init() override
  {
    shaman_attack_t::init();

    if ( p()->action.crash_lightning_aoe )
    {
      add_child( p()->action.crash_lightning_aoe );
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    m *= 1.0 + p()->buff.cl_crash_lightning->stack_value();

    return m;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( num_targets_hit > 1 )
      {
        p()->buff.crash_lightning->trigger();
      }

      if ( p()->talent.converging_storms->ok() )
      {
        p()->buff.converging_storms->trigger( num_targets_hit );
      }
    }

    if ( p()->talent.alpha_wolf.ok() )
    {
      for ( auto pet : p()->pet.spirit_wolves )
      {
        debug_cast<pet::base_wolf_t*>( pet )->trigger_alpha_wolf();
      }

      for ( auto pet : p()->pet.fire_wolves )
      {
        debug_cast<pet::base_wolf_t*>( pet )->trigger_alpha_wolf();
      }

      for ( auto pet : p()->pet.frost_wolves )
      {
        debug_cast<pet::base_wolf_t*>( pet )->trigger_alpha_wolf();
      }

      for ( auto pet : p()->pet.lightning_wolves )
      {
        debug_cast<pet::base_wolf_t*>( pet )->trigger_alpha_wolf();
      }
    }

    p()->buff.cl_crash_lightning->expire();
  }
};

// Earth Elemental ===========================================================

struct earth_elemental_t : public shaman_spell_t
{
  earth_elemental_t( shaman_t* player, util::string_view options_str )
    : shaman_spell_t( "earth_elemental", player, player->talent.earth_elemental )
  {
    parse_options( options_str );

    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->summon_elemental( elemental::GREATER_EARTH );
  }
};

// Fire Elemental ===========================================================

struct fire_elemental_t : public shaman_spell_t
{
  fire_elemental_t( shaman_t* player, util::string_view options_str )
    : shaman_spell_t( "fire_elemental", player, player->talent.fire_elemental )
  {
    parse_options( options_str );
    harmful  = true;
    may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->summon_elemental( elemental::GREATER_FIRE );
  }

  bool ready() override
  {
    if ( p()->talent.storm_elemental->ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Storm Elemental ==========================================================

struct storm_elemental_t : public shaman_spell_t
{
  storm_elemental_t( shaman_t* player, util::string_view options_str )
    : shaman_spell_t( "storm_elemental", player, player->talent.storm_elemental )
  {
    parse_options( options_str );
    harmful  = true;
    may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    // 2022-03-04 hotfix: if you cast Storm Elemental again while having a Storm Elemental active, the Wind Gust buff
    // will be reset.
    // https://us.forums.blizzard.com/en/wow/t/elemental-shaman-class-tuning-march-8/1195446
    p()->buff.wind_gust->expire();

    p()->summon_elemental( elemental::GREATER_STORM );
  }
};

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "lightning_shield", player, player->find_class_spell( "Lightning Shield" ) )
  {
    parse_options( options_str );
    harmful = false;

    // if ( player->action.lightning_shield )
    //{
    // add_child( player->action.lightning_shield );
    //}
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.lightning_shield->trigger();
  }
};

// Earth Shield Spell =======================================================

// Barebones implementation to consume Vesper Totem charges for damage specs
struct earth_shield_t : public shaman_heal_t
{
  earth_shield_t( shaman_t* player, util::string_view options_str ) :
    shaman_heal_t( "earth_shield", player, player->talent.earth_shield )
  {
    parse_options( options_str );
  }

  // Needed to work around a combined Specialization and Talent spell
  bool verify_actor_spec() const override
  { return player->specialization() == SHAMAN_RESTORATION || data().ok(); }

  void execute() override
  {
    shaman_heal_t::execute();

    p()->buff.lightning_shield->expire();
  }
};

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "bloodlust", player, player->find_class_spell( "Bloodlust" ) )
  {
    parse_options( options_str );
    harmful = false;
    track_cd_waste = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    for ( auto* p : sim->player_non_sleeping_list )
    {
       if ( p->buffs.exhaustion->check() || p->is_pet() )
        continue;
      p->buffs.bloodlust->trigger();
      p->buffs.exhaustion->trigger();
    }
  }

  bool ready() override
  {
    // If the global bloodlust override doesn't allow bloodlust, disable bloodlust
    if ( !sim->overrides.bloodlust )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning and Lava Beam Spells =========================================

struct chained_overload_base_t : public elemental_overload_spell_t
{
  chained_overload_base_t( shaman_t* p, util::string_view name, spell_variant t,
                           const spell_data_t* spell, double mg, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, ::action_name( name, t ), spell, parent_, -1.0, t )
  {
    if ( data().effectN( 1 ).chain_multiplier() != 0 )
    {
      chain_multiplier = data().effectN( 1 ).chain_multiplier();
    }

    if ( p->specialization() == SHAMAN_ELEMENTAL )
    {
      maelstrom_gain = mg;
      energize_type  = action_energize::NONE;  // disable resource generation from spell data.
    }
    radius = 10.0;
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }
};

struct chain_lightning_overload_t : public chained_overload_base_t
{
  chain_lightning_overload_t( shaman_t* p, spell_variant t, shaman_spell_t* parent_ ) :
    chained_overload_base_t( p, "chain_lightning_overload", t, p->find_spell( 45297 ),
        p->spec.maelstrom->effectN( 6 ).resource( RESOURCE_MAELSTROM ), parent_ )
  {
    affected_by_master_of_the_elements = true;
  }

  int n_targets() const override
  {
    int t = chained_overload_base_t::n_targets();

    if ( p()->buff.surge_of_power->up() )
    {
      t += as<int>( p()->talent.surge_of_power->effectN( 4 ).base_value() );
    }

    return t;
  }

  void impact( action_state_t* state ) override
  {
    chained_overload_base_t::impact( state );

    // Accumulate Lightning Rod damage from all targets hit by this cast.
    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }
  }
};

struct lava_beam_overload_t : public chained_overload_base_t
{
  lava_beam_overload_t( shaman_t* p, spell_variant t, shaman_spell_t* parent_ )
    : chained_overload_base_t( p, "lava_beam_overload", t, p->find_spell( 114738 ),
        p->spec.maelstrom->effectN( 6 ).resource( RESOURCE_MAELSTROM ), parent_ )
  {
    affected_by_master_of_the_elements = true;
  }

  int n_targets() const override
  {
    int t = chained_overload_base_t::n_targets();

    if ( p()->buff.surge_of_power->up() )
    {
      t += as<int>( p()->talent.surge_of_power->effectN( 4 ).base_value() );
    }

    return t;
  }

  void impact( action_state_t* state ) override
  {
    chained_overload_base_t::impact( state );

    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }
  }
};

struct chained_base_t : public shaman_spell_t
{
  chained_base_t( shaman_t* player, util::string_view name, spell_variant t,
                  const spell_data_t* spell, double mg, util::string_view options_str )
    : shaman_spell_t( ::action_name( name, t ), player, spell, t )
  {
    parse_options( options_str );

    if ( data().effectN( 1 ).chain_multiplier() != 0 )
    {
      chain_multiplier = data().effectN( 1 ).chain_multiplier();
    }
    radius = 10.0;
    ancestor_trigger = ancestor_cast::CHAIN_LIGHTNING;

    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      maelstrom_gain = mg;
      energize_type  = action_energize::NONE;  // disable resource generation from spell data.
    }
  }

  double overload_chance( const action_state_t* s ) const override
  {
    double base_chance = shaman_spell_t::overload_chance( s );

    return base_chance / 3.0;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( exec_type == spell_variant::NORMAL )
    {
      if ( !p()->sk_during_cast )
      {
        p()->buff.stormkeeper->decrement();
      }
      p()->sk_during_cast = false;
    }

    p()->trigger_static_accumulation_refund( execute_state, mw_consumed_stacks );
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }
};

struct chain_lightning_t : public chained_base_t
{
  chain_lightning_t( shaman_t* player, spell_variant t = spell_variant::NORMAL, util::string_view options_str = {} )
    : chained_base_t( player, "chain_lightning", t, player->talent.chain_lightning,
        player->spec.maelstrom->effectN( 5 ).resource( RESOURCE_MAELSTROM ), options_str )
  {
    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new chain_lightning_overload_t( player, t, this );
    }

    affected_by_master_of_the_elements = true;

    switch ( exec_type )
    {
      case spell_variant::THORIMS_INVOCATION:
      {
        background = true;
        base_execute_time = 0_s;
        base_costs[ RESOURCE_MANA ] = 0;
        if ( auto ws_action = p()->find_action( "windstrike" ) )
        {
          ws_action->add_child( this );
        }
      }
      default:
        break;
    }
  }

  double action_da_multiplier() const override
  {
    double m = shaman_spell_t::action_da_multiplier();

    m *= 1.0 + p()->buff.t30_4pc_enh_cl->value();

    return m;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    tl.clear();

    if ( !target->is_sleeping() )
    {
      tl.push_back( target );
    }

    // The rest
    range::for_each( sim->target_non_sleeping_list, [&tl]( player_t* t ) {
      if ( t->is_enemy() && !range::contains( tl, t ) )
      {
        tl.emplace_back( t );
      }
    } );

    return tl.size();
  }

  bool benefit_from_maelstrom_weapon() const override
  {
    if ( p()->buff.stormkeeper->check() )
    {
      return false;
    }

    return shaman_spell_t::benefit_from_maelstrom_weapon();
  }

  // If Stormkeeper is up, Chain Lightning will not consume Maelstrom Weapon stacks, but
  // will allow Chain Lightning to fully benefit from the stacks.
  bool consume_maelstrom_weapon() const override
  {
    if ( p()->buff.stormkeeper->check() )
    {
      return false;
    }

    return shaman_spell_t::consume_maelstrom_weapon();
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = chained_base_t::execute_time_pct_multiplier();

    mul *= 1.0 + p()->buff.wind_gust->stack_value();

    return mul;
  }

  int n_targets() const override
  {
    int t = chained_base_t::n_targets();

    if ( p()->buff.surge_of_power->up() )
    {
      t += as<int>( p()->talent.surge_of_power->effectN( 4 ).base_value() );
    }

    return t;
  }


  timespan_t gcd() const override
  {
    timespan_t t = chained_base_t::gcd();
    t *= 1.0 + p()->buff.wind_gust->stack_value();

    // testing shows the min GCD is 0.6 sec
    if ( t < timespan_t::from_millis( 600 ) )
    {
      t = timespan_t::from_millis( 600 );
    }
    return t;
  }

  bool ready() override
  {
    if ( p()->specialization() == SHAMAN_ELEMENTAL && p()->buff.ascendance->check() )
      return false;

    return shaman_spell_t::ready();
  }

  void execute() override
  {
    chained_base_t::execute();

    // Storm Elemental Wind Gust passive buff trigger
    if ( p()->buff.storm_elemental->check() || p()->buff.lesser_storm_elemental->check() )
    {
      p()->buff.wind_gust->trigger();
    }

    if ( num_targets_hit - 1 > 0 && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->buff.cl_crash_lightning->trigger( num_targets_hit );
    }

    if ( p()->talent.crash_lightning.ok() )
    {
      p()->cooldown.crash_lightning->adjust(
          -( p()->talent.chain_lightning->effectN( 3 ).time_value() * num_targets_hit ) );

      if ( sim->debug )
      {
        sim->print_debug( "{} reducing Crash Lightning cooldown by {}, remains={}",
            p()->name(),
            -( p()->talent.chain_lightning->effectN( 3 ).time_value() * num_targets_hit ),
            p()->cooldown.crash_lightning->remains() );
      }
    }

    p()->trigger_flash_of_lightning();
    p()->buff.surge_of_power->decrement();

    if ( p()->talent.alpha_wolf.ok() )
    {
      for ( auto pet : p()->pet.spirit_wolves )
      {
        debug_cast<pet::base_wolf_t*>( pet )->trigger_alpha_wolf();
      }

      for ( auto pet : p()->pet.fire_wolves )
      {
        debug_cast<pet::base_wolf_t*>( pet )->trigger_alpha_wolf();
      }

      for ( auto pet : p()->pet.frost_wolves )
      {
        debug_cast<pet::base_wolf_t*>( pet )->trigger_alpha_wolf();
      }

      for ( auto pet : p()->pet.lightning_wolves )
      {
        debug_cast<pet::base_wolf_t*>( pet )->trigger_alpha_wolf();
      }
    }

    // Track last cast for LB / CL because of Thorim's Invocation
    if ( p()->talent.thorims_invocation.ok() && exec_type == spell_variant::NORMAL )
    {
      p()->action.ti_trigger = p()->action.chain_lightning_ti;
    }

    if ( p()->buff.t30_4pc_enh_cl->check() )
    {
      auto refunded = as<int>(
        std::ceil( mw_consumed_stacks * p()->buff.t30_4pc_enh_cl->data().effectN( 3 ).percent() ) );

      p()->generate_maelstrom_weapon( execute_state, refunded );
      // In-game, 0 stack Chain Lightning casts don't consume T30 4PC buff
      if ( !p()->bugs || refunded > 0 )
      {
        p()->buff.t30_4pc_enh_cl->decrement();
      }
    }

    if ( exec_type == spell_variant::NORMAL &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( execute_state->action,
                                      as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }

    if ( exec_type == spell_variant::NORMAL )
    {
      p()->trigger_awakening_storms( execute_state );
    }

    p()->trigger_thunderstrike_ward( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    chained_base_t::impact( state );

    // Accumulate Lightning Rod damage from all targets hit by this cast.
    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }

    if ( state->chain_target == 0 && p()->talent.conductive_energy.ok() &&
         p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      trigger_lightning_rod_debuff( state->target );
    }
  }

  void schedule_travel(action_state_t* s) override
  {
    if ( s->chain_target == 0 && p()->buff.power_of_the_maelstrom->up() )
    {
      if ( trigger_elemental_overload( s, 1.0 ) )
      {
        trigger_elemental_overload( s, p()->talent.supercharge->effectN( 1 ).percent() );
      }

      p()->buff.power_of_the_maelstrom->decrement();
    }

    if ( trigger_elemental_overload( s ) )
    {
      trigger_elemental_overload( s, p()->talent.supercharge->effectN( 1 ).percent() );
    }

    // Don't call shaman_spell_t::schedule_travel, because we need to override .. override behavior
    // for this spell to support Supercharge.
    base_t::schedule_travel( s );
  }


};

struct lava_beam_t : public chained_base_t
{
  // This is actually a tooltip bug in-game: real testing shows that Lava Beam and
  // Lava Beam Overload generate resources identical to their Chain Lightning counterparts
  lava_beam_t( shaman_t* player, spell_variant t = spell_variant::NORMAL, util::string_view options_str = {} )
    : chained_base_t( player, "lava_beam", t, player->find_spell( 114074 ),
                      player->spec.maelstrom->effectN( 5 ).resource( RESOURCE_MAELSTROM ), options_str )
  {
    affected_by_master_of_the_elements = true;

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lava_beam_overload_t( player, t, this );
    }
  }

  bool ready() override
  {
    if ( !p()->buff.ascendance->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }

  int n_targets() const override
  {
    int t = chained_base_t::n_targets();

    if ( p()->buff.surge_of_power->up() )
    {
      t += as<int>( p()->talent.surge_of_power->effectN( 4 ).base_value() );
    }

    return t;
  }

  void execute() override
  {
    chained_base_t::execute();

    p()->buff.surge_of_power->decrement();
  }

  void impact( action_state_t* state ) override
  {
    chained_base_t::impact( state );

    // Accumulate Lightning Rod damage from all targets hit by this cast.
    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }
  }

  void schedule_travel(action_state_t* s) override
  {
    if ( s->chain_target == 0 && p()->buff.power_of_the_maelstrom->up() )
    {
      trigger_elemental_overload( s, 1.0 );
      p()->buff.power_of_the_maelstrom->decrement();
    }

    chained_base_t::schedule_travel( s );
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_state_t : public shaman_action_state_t
{
  lava_burst_state_t( action_t* action_, player_t* target_ ) :
    shaman_action_state_t( action_, target_ )
  { }
};

// As of 8.1 Lava Burst checks its state on impact. Lava Burst -> Flame Shock now forces the critical strike
struct lava_burst_overload_t : public elemental_overload_spell_t
{
  unsigned impact_flags;

  lava_burst_overload_t( shaman_t* player, spell_variant type, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( player, ::action_name( "lava_burst_overload", type ),
        player->find_spell( 285466 ), parent_, -1.0, type ),
      impact_flags()
  {
    maelstrom_gain = player->spec.maelstrom->effectN( 4 ).resource( RESOURCE_MAELSTROM );
    //maelstrom_gain += player->talent.flow_of_power->effectN( 4 ).base_value();
    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    travel_speed = player->find_spell( 77451 )->missile_speed();
  }

  static lava_burst_state_t* cast_state( action_state_t* s )
  { return debug_cast<lava_burst_state_t*>( s ); }

  static const lava_burst_state_t* cast_state( const action_state_t* s )
  { return debug_cast<const lava_burst_state_t*>( s ); }

  action_state_t* new_state() override
  { return new lava_burst_state_t( this, target ); }

  void snapshot_impact_state( action_state_t* s, result_amount_type rt )
  {
    auto et = cast_state( s )->exec_type;

    snapshot_internal( s, impact_flags, rt );

    cast_state( s )->exec_type = et;
  }

  double calculate_direct_amount( action_state_t* /* s */ ) const override
  {
    // Don't do any extra work, this result won't be used.
    return 0.0;
  }

  result_e calculate_result( action_state_t* /* s */ ) const override
  {
    // Don't do any extra work, this result won't be used.
    return RESULT_NONE;
  }

  void impact( action_state_t* s ) override
  {
    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    snapshot_impact_state( s, amount_type( s ) );

    s->result        = elemental_overload_spell_t::calculate_result( s );
    s->result_amount = elemental_overload_spell_t::calculate_direct_amount( s );

    elemental_overload_spell_t::impact( s );
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( exec_type == spell_variant::PRIMORDIAL_WAVE )
    {
      if ( p()->talent.primordial_wave->ok() )
      {
        m *= p()->talent.primordial_wave->effectN( 3 ).percent();
      }
    }

    if ( p()->buff.ascendance->up() )
    {
      m *= 1.0 + p()->cache.spell_crit_chance();
    }

    m *= 1.0 + p()->buff.flux_melting->value();

    return m;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_crit_chance( t );

    // TODO Elemental: confirm is this effect needs to be hardcoded
    /* if ( p()->spec.lava_burst_2->ok() && td( target )->dot.flame_shock->is_ticking() ) */
    if ( td( target )->dot.flame_shock->is_ticking() )
    {
      // hardcoded because I didn't find it in spell data
      m = 1.0;
    }

    return m;
  }

};

struct flame_shock_spreader_t : public shaman_spell_t
{
  flame_shock_spreader_t( shaman_t* p ) : shaman_spell_t( "flame_shock_spreader", p )
  {
    quiet = background = true;
    may_miss = may_crit = callbacks = false;
  }

  player_t* shortest_duration_target() const
  {
    player_t* copy_target  = nullptr;
    timespan_t min_remains = timespan_t::zero();

    for ( auto t : sim->target_non_sleeping_list )
    {
      // Skip source target
      if ( t == target )
      {
        continue;
      }

      // Skip targets that are further than 8 yards from the original target
      if ( sim->distance_targeting_enabled && t->get_player_distance( *target ) > 8 + t->combat_reach )
      {
        continue;
      }

      shaman_td_t* target_td = td( t );
      if ( min_remains == timespan_t::zero() || min_remains > target_td->dot.flame_shock->remains() )
      {
        min_remains = target_td->dot.flame_shock->remains();
        copy_target = t;
      }
    }

    if ( copy_target && sim->debug )
    {
      sim->out_debug.printf(
          "%s spreads flame_shock from %s to shortest remaining target %s (remains=%.3f)",
          player->name(), target->name(), copy_target->name(), min_remains.total_seconds() );
    }

    return copy_target;
  }

  player_t* closest_target() const
  {
    player_t* copy_target = nullptr;
    double min_distance   = -1;

    for ( auto t : sim->target_non_sleeping_list )
    {
      // Skip source target
      if ( t == target )
      {
        continue;
      }

      double distance = 0;
      if ( sim->distance_targeting_enabled )
      {
        distance = t->get_player_distance( *target );
      }

      // Skip targets that are further than 8 yards from the original target
      if ( sim->distance_targeting_enabled && distance > 8 + t->combat_reach )
      {
        continue;
      }

      shaman_td_t* target_td = td( t );
      if ( target_td->dot.flame_shock->is_ticking() )
      {
        continue;
      }

      // If we are not using distance-based targeting, just return the first available target
      if ( !sim->distance_targeting_enabled )
      {
        copy_target = t;
        break;
      }
      else if ( min_distance < 0 || min_distance > distance )
      {
        min_distance = distance;
        copy_target  = t;
      }
    }

    if ( copy_target && sim->debug )
    {
      sim->out_debug.printf( "%s spreads flame_shock from %s to closest target %s (distance=%.3f)",
                             player->name(), target->name(), copy_target->name(), min_distance );
    }

    return copy_target;
  }

  void execute() override
  {
    shaman_td_t* source_td = td( target );
    player_t* copy_target  = nullptr;
    if ( !source_td->dot.flame_shock->is_ticking() )
    {
      return;
    }

    // If all targets have flame shock, pick the shortest remaining time
    if ( player->get_active_dots( source_td->dot.flame_shock ) ==
         sim->target_non_sleeping_list.size() )
    {
      copy_target = shortest_duration_target();
    }
    // Pick closest target without Flame Shock
    else
    {
      copy_target = closest_target();
    }

    // With distance targeting it is possible that no target will be around to spread flame shock to
    if ( copy_target )
    {
      source_td->dot.flame_shock->copy( copy_target, DOT_COPY_CLONE );
    }
  }
};

// Fire Nova Spell ==========================================================

struct fire_nova_explosion_t : public shaman_spell_t
{
  fire_nova_explosion_t( shaman_t* p ) :
    shaman_spell_t( "fire_nova_explosion", p, p->find_spell( 333977 ) )
  {
    background = true;
  }

  double action_da_multiplier() const override
  {
    double m = shaman_spell_t::action_da_multiplier();

    // In-game, Fire Nova damage double-dips on T30 4PC buff
    if ( player->bugs )
    {
      m *= 1.0 + p()->buff.t30_4pc_enh_damage->value();
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = shaman_spell_t::composite_crit_chance();

    c += p()->buff.whirling_fire->value();

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double c = shaman_spell_t::composite_crit_damage_bonus_multiplier();

    if ( p()->buff.whirling_fire->check() )
    {
      c *= 1.0 + p()->buff.whirling_fire->data().effectN( 2 ).percent();
    }

    return c;
  }
};

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_t( shaman_t* p, util::string_view options_str )
    : shaman_spell_t( "fire_nova", p, p->talent.fire_nova )
  {
    parse_options( options_str );
    may_crit = may_miss = callbacks = false;
    aoe                             = -1;

    impact_action = new fire_nova_explosion_t( p );

    p->flame_shock_dependants.push_back( this );

    add_child( impact_action );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    shaman_spell_t::available_targets( tl );

    p()->regenerate_flame_shock_dependent_target_list( this );

    return tl.size();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->trigger_reactivity( execute_state );

    p()->buff.whirling_fire->decrement();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    p()->trigger_swirling_maelstrom( state );
  }
};

/**
 * As of 8.1 Lava Burst checks its state on impact. Lava Burst -> Flame Shock now forces the critical strike
 */
struct lava_burst_t : public shaman_spell_t
{
  unsigned impact_flags;

  lava_burst_t( shaman_t* player, spell_variant type_, util::string_view options_str = {} )
    : shaman_spell_t( ::action_name( "lava_burst", type_ ), player, player->talent.lava_burst, type_ ),
      impact_flags()
  {
    parse_options( options_str );
    // Manacost is only for resto
    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      base_costs[ RESOURCE_MANA ] = 0;

      maelstrom_gain = player->spec.maelstrom->effectN( 3 ).resource( RESOURCE_MAELSTROM );
      maelstrom_gain += player->talent.flow_of_power->effectN( 1 ).base_value();
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lava_burst_overload_t( player, exec_type, this );
    }

    spell_power_mod.direct = player->find_spell( 285452 )->effectN( 1 ).sp_coeff();
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( exec_type != spell_variant::NORMAL )
    {
      aoe = -1;
      background = true;
      base_execute_time = 0_s;
      cooldown->duration = 0_s;
      switch ( exec_type )
      {
        case spell_variant::PRIMORDIAL_WAVE:
          if ( auto pw_action = p()->find_action( "primordial_wave" ) )
          {
            pw_action->add_child( this );
          }
          break;
        case spell_variant::ASCENDANCE:
          {
            auto asc_action = p()->find_action( "ascendance" );
            if ( p()->talent.ascendance->ok() && asc_action )
            {
              asc_action->add_child( this );
            }
          }
          break;
        case spell_variant::DEEPLY_ROOTED_ELEMENTS:
          {
            auto dre_asc_action = p()->find_action( "dre_ascendance" );
            if ( dre_asc_action )
            {
              dre_asc_action->add_child( this );
            }
          }
          break;
        default:
          break;
      }
    }
  }

  static lava_burst_state_t* cast_state( action_state_t* s )
  { return debug_cast<lava_burst_state_t*>( s ); }

  static const lava_burst_state_t* cast_state( const action_state_t* s )
  { return debug_cast<const lava_burst_state_t*>( s ); }

  action_state_t* new_state() override
  { return new lava_burst_state_t( this, target ); }

  void init() override
  {
    shaman_spell_t::init();

    std::swap( snapshot_flags, impact_flags );

  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    shaman_spell_t::available_targets( tl );

    p()->regenerate_flame_shock_dependent_target_list( this );

    return tl.size();
  }

  void snapshot_impact_state( action_state_t* s, result_amount_type rt )
  {
    snapshot_internal( s, impact_flags, rt );
  }

  double calculate_direct_amount( action_state_t* /* s */ ) const override
  {
    // Don't do any extra work, this result won't be used.
    return 0.0;
  }

  result_e calculate_result( action_state_t* /* s */ ) const override
  {
    // Don't do any extra work, this result won't be used.
    return RESULT_NONE;
  }

  void impact( action_state_t* s ) override
  {
    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    snapshot_impact_state( s, amount_type( s ) );

    s->result        = shaman_spell_t::calculate_result( s );
    s->result_amount = shaman_spell_t::calculate_direct_amount( s );

    shaman_spell_t::impact( s );
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    // Note, only Elemental Shaman gets the primordial_wave state set, so don't need
    // separate specialization checks here
    if ( exec_type == spell_variant::PRIMORDIAL_WAVE )
    {
      if ( p()->talent.primordial_wave->ok() )
      {
        m *= p()->talent.primordial_wave->effectN( 3 ).percent();
      }
    }

    if ( p()->buff.ascendance->up() )
    {
      m *= 1.0 + p()->cache.spell_crit_chance();
    }

    m *= 1.0 + p()->buff.flux_melting->value();

    return m;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_crit_chance( t );

    // TODO Elemental: confirm is this effect needs to be hardcoded
    /* if ( p()->spec.lava_burst_2->ok() && td( target )->dot.flame_shock->is_ticking() ) */
    if ( td( target )->dot.flame_shock->is_ticking() )
    {
      // hardcoded because I didn't find it in spell data
      m = 1.0;
    }

    return m;
  }

  void update_ready( timespan_t /* cd_duration */ ) override
  {
    timespan_t d = cooldown->duration;

    if ( p()->buff.ascendance->up() )
    {
      d = timespan_t::zero();
    }

    // Lava Surge has procced during the cast of Lava Burst, the cooldown
    // reset is deferred to the finished cast, instead of "eating" it.

    if ( p()->lava_surge_during_lvb )
    {
      d                      = timespan_t::zero();
      cooldown->last_charged = sim->current_time();
    }

    shaman_spell_t::update_ready( d );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( exec_type == spell_variant::NORMAL && p()->buff.surge_of_power->up() )
    {
      p()->cooldown.fire_elemental->adjust( -1.0 * p()->talent.surge_of_power->effectN( 1 ).time_value() );
      p()->cooldown.storm_elemental->adjust( -1.0 * p()->talent.surge_of_power->effectN( 1 ).time_value() );
      p()->buff.surge_of_power->decrement();
      p()->proc.surge_of_power_lava_burst->occur();
    }

    if ( exec_type == spell_variant::NORMAL && p()->talent.master_of_the_elements->ok() )
    {
      p()->buff.master_of_the_elements->trigger();
    }

    // Lava Surge buff does not get eaten, if the Lava Surge proc happened
    // during the Lava Burst cast
    if ( exec_type == spell_variant::NORMAL && !p()->lava_surge_during_lvb && p()->buff.lava_surge->check() )
    {
      p()->buff.lava_surge->decrement();
    }

    p()->lava_surge_during_lvb = false;

    // Trigger primordial wave if there's targets to trigger it on
    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      p()->trigger_primordial_wave_damage( this );
    }

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      p()->trigger_deeply_rooted_elements( execute_state );
    }

    // Rolls on execute and on impact
    if ( exec_type == spell_variant::NORMAL && rng().roll( p()->talent.power_of_the_maelstrom->effectN( 2 ).percent() ) )
    {
      p()->buff.power_of_the_maelstrom->trigger();
    }

    if ( exec_type == spell_variant::NORMAL )
    {
      p()->buff.flux_melting->decrement();
    }

    if ( exec_type == spell_variant::NORMAL && p()->rng_obj.icefury->trigger() )
    {
      p()->buff.icefury_cast->trigger();
    }
    

    if ( p()->talent.routine_communication.ok() && exec_type == spell_variant::NORMAL )
    {
      p()->summon_ancestor( p()->talent.routine_communication->effectN( 2 ).percent() );
    }

    // [BUG] 2024-08-23 Supercharge works on Lava Burst in-game
    if ( p()->bugs && exec_type == spell_variant::NORMAL &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( this, as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }
  }

  timespan_t execute_time() const override
  {
    if ( p()->buff.lava_surge->up() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::execute_time();
  }

  bool ready() override
  {
    if ( player->specialization() == SHAMAN_ENHANCEMENT &&
         p()->talent.elemental_blast.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_overload_t : public elemental_overload_spell_t
{
  lightning_bolt_overload_t( shaman_t* p, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, "lightning_bolt_overload", p->find_spell( 45284 ), parent_ )
  {
    maelstrom_gain  = p->spec.maelstrom->effectN( 2 ).resource( RESOURCE_MAELSTROM );
    //maelstrom_gain += p->talent.flow_of_power->effectN( 4 ).base_value();

    affected_by_master_of_the_elements = true;
    // Stormkeeper affected by flagging is applied to the Energize spell ...
    affected_by_stormkeeper_damage = p->talent.stormkeeper.ok() && p->specialization() == SHAMAN_ELEMENTAL;
  }

  void impact( action_state_t* state ) override
  {
    elemental_overload_spell_t::impact( state );

    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }
  }
};

struct lightning_bolt_t : public shaman_spell_t
{
  timespan_t lr_delay;

  lightning_bolt_t( shaman_t* player, spell_variant type_, util::string_view options_str = {} ) :
    shaman_spell_t( ::action_name( "lightning_bolt", type_ ),
        player, player->find_class_spell( "Lightning Bolt" ), type_ )
  {
    parse_options( options_str );
    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      affected_by_master_of_the_elements = true;

      maelstrom_gain = player->spec.maelstrom->effectN( 1 ).resource( RESOURCE_MAELSTROM );
      maelstrom_gain += player->talent.flow_of_power->effectN( 2 ).base_value();
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lightning_bolt_overload_t( player, this );
      //add_child( overload );
    }

    ancestor_trigger = ancestor_cast::LAVA_BURST;

    switch ( exec_type )
    {
      case spell_variant::PRIMORDIAL_WAVE:
      {
        aoe = -1;
        background = true;
        base_execute_time = 0_s;
        base_costs[ RESOURCE_MANA ] = 0;
        if ( auto pw_action = p()->find_action( "primordial_wave" ) )
        {
          pw_action->add_child( this );
        }
        break;
      }
      case spell_variant::THORIMS_INVOCATION:
      {
        background = true;
        base_execute_time = 0_s;
        base_costs[ RESOURCE_MANA ] = 0;
        if ( auto ws_action = p()->find_action( "windstrike" ) )
        {
          ws_action->add_child( this );
        }
      }
      default:
        break;
    }
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    shaman_spell_t::available_targets( tl );

    if ( exec_type == spell_variant::PRIMORDIAL_WAVE )
    {
      p()->regenerate_flame_shock_dependent_target_list( this );
    }

    return tl.size();
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p()->buff.primordial_wave->check() && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      m *= p()->buff.primordial_wave->value();
    }

    return m;
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = shaman_spell_t::execute_time_pct_multiplier();

    mul *= 1.0 + p()->buff.wind_gust->stack_value();

    return mul;
  }

  timespan_t gcd() const override
  {
    timespan_t t = shaman_spell_t::gcd();
    t *= 1.0 + p()->buff.wind_gust->stack_value();

    // testing shows the min GCD is 0.6 sec
    if ( t < timespan_t::from_millis( 600 ) )
    {
      t = timespan_t::from_millis( 600 );
    }
    return t;
  }

  void execute() override
  {
    // PW needs to execute before the primary spell executes so we can retain proper
    // Maelstrom Weapon stacks for the AoE Lightning Bolt
    if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->trigger_primordial_wave_damage( this );
    }

    shaman_spell_t::execute();

    // Storm Elemental Wind Gust passive buff trigger
    if ( p()->buff.storm_elemental->check() || p()->buff.lesser_storm_elemental->check() )
    {
      p()->buff.wind_gust->trigger();
    }

    if ( exec_type == spell_variant::NORMAL &&
         p()->specialization() == SHAMAN_ELEMENTAL )
    {

      if ( !p()->sk_during_cast )
      {
        p()->buff.stormkeeper->decrement();
      }
      p()->sk_during_cast = false;
    }

    p()->trigger_flash_of_lightning();
    p()->trigger_static_accumulation_refund( execute_state, mw_consumed_stacks );

    if ( exec_type == spell_variant::NORMAL )
    {
      p()->trigger_awakening_storms( execute_state );
    }

    // Track last cast for LB / CL because of Thorim's Invocation
    if ( p()->talent.thorims_invocation.ok() && exec_type == spell_variant::NORMAL )
    {
      p()->action.ti_trigger = p()->action.lightning_bolt_ti;
    }

    if ( exec_type == spell_variant::NORMAL &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( execute_state->action,
                                      as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }

    p()->trigger_thunderstrike_ward( execute_state );
  }

  void schedule_travel( action_state_t* s ) override
  {
    if ( exec_type == spell_variant::NORMAL &&
         p()->buff.power_of_the_maelstrom->up() )
    {
      if ( trigger_elemental_overload( s, 1.0 ) )
      {
        trigger_elemental_overload( s, p()->talent.supercharge->effectN( 1 ).percent() );
      }

      p()->buff.power_of_the_maelstrom->decrement();
    }

    if ( exec_type == spell_variant::NORMAL &&
         p()->buff.surge_of_power->check() )
    {
      if ( p()->buff.stormkeeper->check() )
      {
        p()->proc.surge_of_power_sk_lightning_bolt->occur();
      }

      p()->proc.surge_of_power_lightning_bolt->occur();

      for ( auto i = 0; i < as<int>( p()->talent.surge_of_power->effectN( 2 ).base_value() ); ++i )
      {
        if ( trigger_elemental_overload( s, 1.0 ) )
        {
          trigger_elemental_overload( s, p()->talent.supercharge->effectN( 1 ).percent() );
        }
      }
      p()->buff.surge_of_power->decrement();
    }

    if ( trigger_elemental_overload( s ) )
    {
      trigger_elemental_overload( s, p()->talent.supercharge->effectN( 1 ).percent() );
    }

    // Don't call shaman_spell_t::schedule_travel, because we need to override .. override behavior
    // for this spell to support Supercharge.
    base_t::schedule_travel( s );
  }
  //void reset_swing_timers()
  //{
  //  if ( player->main_hand_attack && player->main_hand_attack->execute_event )
  //  {
  //    event_t::cancel( player->main_hand_attack->execute_event );
  //    player->main_hand_attack->schedule_execute();
  //  }

  //  if ( player->off_hand_attack && player->off_hand_attack->execute_event )
  //  {
  //    event_t::cancel( player->off_hand_attack->execute_event );
  //    player->off_hand_attack->schedule_execute();
  //  }
  //}

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    // [2024-09-05] BUG: Lightning Bolts generated by Primordial Wave do not trigger Lightning Rod
    // damage. Presumption is that they should work just like normal Lightning Bolts when
    // interacting with Lightning Rod (through Conductive Energy).
    if ( p()->specialization() == SHAMAN_ENHANCEMENT && p()->talent.conductive_energy.ok() &&
         ( !p()->bugs || ( exec_type != spell_variant::PRIMORDIAL_WAVE ) ) )
    {
      accumulate_lightning_rod_damage( state );
    }
    else if ( p()->specialization() == SHAMAN_ELEMENTAL && p()->talent.lightning_rod.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }

    if ( p()->talent.conductive_energy.ok() && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      // On first impact, randomize a delay for the lightning rod debuff that is associated with all
      // the subsequent debuff triggers
      if ( state->chain_target == 0 )
      {
        lr_delay = rng().range( 10_ms, 100_ms );
      }

      trigger_lightning_rod_debuff( state->target, lr_delay );
    }
  }

  bool ready() override
  {
    if ( exec_type == spell_variant::NORMAL && p()->buff.tempest->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Elemental Blast Spell ====================================================

void trigger_elemental_blast_proc( shaman_t* p )
{
  unsigned b = static_cast<unsigned>( p->rng().range( 0, 3 ) );

  // if for some reason (Ineffable Truth, corruption) Elemental Blast can trigger four times, just let it overwrite
  // something
  if ( !p->buff.elemental_blast_crit->check() || !p->buff.elemental_blast_haste->check() ||
       !p->buff.elemental_blast_mastery->check() )
  {
    // EB can no longer proc the same buff twice
    while ( ( b == 0 && p->buff.elemental_blast_crit->check() ) ||
            ( b == 1 && p->buff.elemental_blast_haste->check() ) ||
            ( b == 2 && p->buff.elemental_blast_mastery->check() ) )
    {
      b = static_cast<unsigned>( p->rng().range( 0, 3 ) );
    }
  }

  if ( b == 0 )
  {
    p->buff.elemental_blast_crit->trigger();
    p->proc.elemental_blast_crit->occur();
  }
  else if ( b == 1 )
  {
    p->buff.elemental_blast_haste->trigger();
    p->proc.elemental_blast_haste->occur();
  }
  else if ( b == 2 )
  {
    p->buff.elemental_blast_mastery->trigger();
    p->proc.elemental_blast_mastery->occur();
  }
}

struct elemental_blast_overload_t : public elemental_overload_spell_t
{
  elemental_blast_overload_t( shaman_t* p, spell_variant type, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, ::action_name( "elemental_blast_overload", type ), p->find_spell( 120588 ), parent_,
        p->talent.mountains_will_fall->effectN( 1 ).percent(), type )
  {
    affected_by_master_of_the_elements = true;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.magma_chamber->check_stack_value();

    if ( exec_type == spell_variant::FUSION_OF_ELEMENTS )
    {
      m *= p()->talent.fusion_of_elements->effectN( 1 ).percent();
    }    
    return m;
  }


  void execute() override
  {
    // Trigger buff before executing the spell, because apparently the buffs affect the cast result
    // itself.
    ::trigger_elemental_blast_proc( p() );
    elemental_overload_spell_t::execute();
  }
};

struct elemental_blast_t : public shaman_spell_t
{
  elemental_blast_t( shaman_t* player, spell_variant type_, util::string_view options_str = {}) :
    shaman_spell_t(
      ::action_name("elemental_blast", type_),
      player,
      player->find_spell( 117014 ),
      type_
    )
  {
    parse_options( options_str );

    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      affected_by_master_of_the_elements = true;

      if ( p()->talent.mountains_will_fall.enabled() )
      {
        overload = new elemental_blast_overload_t( player, exec_type, this );
      }

      resource_current = RESOURCE_MAELSTROM;
    }
    else if ( player->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( player->talent.elemental_blast.ok() && player->talent.lava_burst.ok() )
      {
        cooldown->charges += as<int>( player->find_spell( 394152 )->effectN( 2 ).base_value() );
      }
    }

    switch ( type_ )
    {
      case spell_variant::PRIMORDIAL_WAVE:
      case spell_variant::FUSION_OF_ELEMENTS:
        base_execute_time = 0_ms;
        background = true;
        base_costs[ RESOURCE_MANA ] = 0;
        base_costs[ RESOURCE_MAELSTROM ] = 0;
        break;
      default:
        break;
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.magma_chamber->stack_value();

    if ( exec_type == spell_variant::FUSION_OF_ELEMENTS )
    {
      m *= p()->talent.fusion_of_elements->effectN( 1 ).percent();
    }

    return m;
  }

  bool ready() override
  {
    if ( !p()->talent.elemental_blast->ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    ::trigger_elemental_blast_proc( p() );

    // these are effects which ONLY trigger when the player cast the spell directly
    if ( exec_type == spell_variant::NORMAL )
    {
      if ( p()->talent.echoes_of_great_sundering.ok() )
      {
        p()->buff.echoes_of_great_sundering_eb->trigger();
      }

      // talents
      p()->buff.storm_frenzy->trigger();

      if ( p()->buff.whirling_earth->up() )
      {
        p()->buff.whirling_earth->decrement();
        cooldown->adjust( -p()->buff.whirling_earth->data().effectN( 1 ).time_value() );
      }
      p()->track_magma_chamber();
      p()->buff.magma_chamber->expire();

      if ( p()->talent.surge_of_power->ok() )
      {
        p()->buff.surge_of_power->trigger();
      }
    }

    // [BUG] 2024-08-23 Supercharge works on Elemental Blast in-game
    if ( p()->bugs && exec_type == spell_variant::NORMAL &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( this, as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->talent.lightning_rod.ok() )
    {
      trigger_lightning_rod_debuff( state->target );
    }
  }
};

// Icefury Spell ====================================================

struct icefury_overload_t : public elemental_overload_spell_t
{
  icefury_overload_t( shaman_t* p, shaman_spell_t* parent_ ) :
    elemental_overload_spell_t( p, "icefury_overload", p->find_spell( 219271 ), parent_ )
  {
    affected_by_master_of_the_elements = true;
    maelstrom_gain = p->spec.maelstrom->effectN( 9 ).resource( RESOURCE_MAELSTROM );
  }
};

struct icefury_t : public shaman_spell_t
{
  icefury_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "icefury", player, player->find_spell( 210714 ) )
  {
    parse_options( options_str );
    affected_by_master_of_the_elements = true;
    maelstrom_gain = player->spec.maelstrom->effectN( 8 ).resource( RESOURCE_MAELSTROM );
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new icefury_overload_t( player, this );
      //add_child( overload );
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.icefury_dmg->trigger( as<int>( p()->buff.icefury_dmg->data().effectN( 4 ).base_value() ) );

    p()->buff.fusion_of_elements_nature->trigger();
    p()->buff.fusion_of_elements_fire->trigger();

    p()->buff.icefury_cast->decrement();
  }

  bool ready() override
  {
    if ( !p()->talent.icefury.ok() )
    {
      return false;
    }

    if ( !p()->buff.icefury_cast->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Spirit Wolf Spell ========================================================

struct feral_spirit_spell_t : public shaman_spell_t
{
  feral_spirit_cast type;
  timespan_t duration;
  unsigned n_summons;

  feral_spirit_spell_t( shaman_t* player, util::string_view options_str,
                        feral_spirit_cast c = feral_spirit_cast::NORMAL ) :
    shaman_spell_t( "feral_spirit", player,
                   c == feral_spirit_cast::NORMAL
                   ? player->talent.feral_spirit
                   : player->find_spell( 51533 ) ), type( c )
  {
    parse_options( options_str );
    harmful = true;

    switch ( type )
    {
      case feral_spirit_cast::NORMAL:
        duration = player->find_spell( 228562 )->duration();
        n_summons = as<unsigned>( player->find_spell( 228562 )->effectN( 1 ).base_value() ) +
          as<unsigned>( player->sets->set( SHAMAN_ENHANCEMENT, TWW1, B4 )->effectN( 1 ).base_value() );
        break;
      // TODO: Figure out the T31 driver
      case feral_spirit_cast::ROLLING_THUNDER:
      case feral_spirit_cast::TIER31:
        duration = player->find_spell( 228562 )->duration();
        n_summons = 1U;
        background = true;
        cooldown = player->get_cooldown( "feral_spirit_proc" );
        cooldown->duration = 0_ms;
        break;
      case feral_spirit_cast::TIER28:
        duration = timespan_t::from_seconds( player->spell.t28_2pc_enh->effectN( 2 ).base_value() );
        n_summons = 1U;
        background = true;
        cooldown = player->get_cooldown( "feral_spirit_proc" );
        cooldown->duration = 0_ms;
        break;
    }

    // Cache pointer for MW tracking uses
    if ( !p()->action.feral_spirits )
    {
      p()->action.feral_spirits = this;
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    // Evaluate before n gets messed with
    if ( player->sets->has_set_bonus( SHAMAN_ENHANCEMENT, T31, B4 ) )
    {
      p()->cooldown.primordial_wave->adjust( -1.0 *
        player->sets->set( SHAMAN_ENHANCEMENT, T31, B4 )->effectN( 1 ).time_value() * n_summons );
    }

    if ( type == feral_spirit_cast::TIER31 || type == feral_spirit_cast::ROLLING_THUNDER )
    {
      p()->pet.lightning_wolves.spawn( duration );
      p()->buff.crackling_surge->trigger( n_summons, buff_t::DEFAULT_VALUE(), -1, duration );
    }
    else
    {
      // No elemental spirits selected, just summon normal pets
      if ( !p()->talent.elemental_spirits->ok() )
      {
        p()->pet.spirit_wolves.spawn( duration, n_summons );
        for ( unsigned i = 0; i < n_summons; ++i )
        {
          p()->buff.earthen_weapon->trigger( 1, buff_t::DEFAULT_VALUE(), -1, duration );
        }
      }
      else
      {
        // This summon evaluates the wolf type to spawn as the roll, instead of rolling against
        // the available pool of wolves to spawn.
        auto n = n_summons;
        while ( n )
        {
          switch ( static_cast<wolf_type_e>( 1 + rng().range( 0, 3 ) ) )
          {
            case FIRE_WOLF:
              p()->pet.fire_wolves.spawn( duration );
              p()->buff.molten_weapon->trigger( 1, buff_t::DEFAULT_VALUE(), -1, duration );
              break;
            case FROST_WOLF:
              p()->pet.frost_wolves.spawn( duration );
              p()->buff.icy_edge->trigger( 1, buff_t::DEFAULT_VALUE(), -1, duration );
              break;
            case LIGHTNING_WOLF:
              p()->pet.lightning_wolves.spawn( duration );
              p()->buff.crackling_surge->trigger( 1, buff_t::DEFAULT_VALUE(), -1, duration );
              break;
            default:
              assert( 0 );
              break;
          }
          n--;
        }
      }
    }

    // Enhancement T28 or T31 bonuses will only override the buff from manually cast spell
    // if the new duration exceeds the remaining duration of the buff.
    if ( type == feral_spirit_cast::NORMAL ||
      duration > p()->buff.feral_spirit_maelstrom->remains() )
    {
      p()->buff.feral_spirit_maelstrom->trigger( 1, duration );
    }
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  thunderstorm_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "thunderstorm", player, player->talent.thunderstorm )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "spiritwalkers_grace", player, player->talent.spiritwalkers_grace )
  {
    parse_options( options_str );
    may_miss = may_crit = harmful = callbacks = false;
    cooldown->duration += p()->talent.graceful_spirit->effectN( 1 ).time_value();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.spiritwalkers_grace->trigger();
  }
};

// Earthquake ===============================================================

struct earthquake_damage_base_t : public shaman_spell_t
{
  // Deeptremor Totem needs special handling to enable persistent MoTE buff. Normal
  // Earthquake can use the persistent multiplier below
  bool mote_buffed;

  action_t* parent;

  earthquake_damage_base_t( shaman_t* player, util::string_view name, const spell_data_t* spell, action_t* p = nullptr ) :
    shaman_spell_t( name, player, spell ), mote_buffed( false ), parent( p )
  {
    aoe        = -1;
    ground_aoe = background = true;
  }

  // Snapshot base state from the parent to grab proper persistent multiplier for all damage
  // (normal, overload)
  void snapshot_state( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    // TODO: remove check for parent when we remove runeforged effects (Shadowlands legendaries)
    if ( parent )
    {
      s->copy_state( parent->execute_state );
    }
    else
    {
      shaman_spell_t::snapshot_state( s, flags, rt );
    }
  }

  double composite_target_armor( player_t* ) const override
  { return 0; }

  // Persistent multiplier handling is also here to support Deeptremor Totem, since it will not have
  // a parent defined
  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );

    if ( mote_buffed || p()->buff.master_of_the_elements->up() )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->default_value;
    }

    m *= 1.0 + p()->buff.echoes_of_great_sundering_es->value();
    m *= 1.0 + p()->buff.echoes_of_great_sundering_eb->value();
    m *= 1.0 + p()->buff.magma_chamber->stack_value();

    return m;
  }

  double get_spell_power_coefficient_from_options() {
      auto default_options = new shaman_t::options_t();
      auto default_value = default_options->earthquake_spell_power_coefficient;
      auto option_value = p()->options.earthquake_spell_power_coefficient;
      delete( default_options );

      if ( option_value != default_value )
      {
          return option_value;
      }
      return 0.0;
  }

  double get_spell_power_coefficient_from_sdb() {
      auto coeff = 0.0;

      if ( auto vars = p()->dbc->spell_desc_vars( 462620 ).desc_vars() ) {
          std::cmatch m;
          std::regex get_var( R"(\$damage=\$\{\$SPN\*([\d\.]+)\*.*\})" );

          if ( std::regex_search( vars, m, get_var ) )
          {
              coeff = util::to_double( m.str( 1 ) );
          }
      }

      assert( coeff > 0.0 && "Could not parse Earthquake Spell Power coefficient from SDB" );

      return coeff;
  }
};

struct earthquake_base_t : public shaman_spell_t
{
  earthquake_damage_base_t* rumble;

  earthquake_base_t( shaman_t* player, util::string_view name, const spell_data_t* spell ) :
    shaman_spell_t( name, player, spell ),
    rumble( nullptr )
  {
    dot_duration = timespan_t::zero();  // The periodic effect is handled by ground_aoe_event_t

    ancestor_trigger = ancestor_cast::CHAIN_LIGHTNING;
  }

  void init_finished() override
  {
    shaman_spell_t::init_finished();

    // Copy state flagging from the damage spell so we an inherit snapshot state in the damage spell
    // properly when the ground aoe event below is executed. This ensures proper inheritance of
    // persistent multipliers to the base earthquake, as well as the overload.
    snapshot_flags = rumble->snapshot_flags;
    update_flags = rumble->update_flags;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );

    if ( p()->buff.master_of_the_elements->up() )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->default_value;
    }

    m *= 1.0 + p()->buff.echoes_of_great_sundering_es->value();
    m *= 1.0 + p()->buff.echoes_of_great_sundering_eb->value();
    m *= 1.0 + p()->buff.magma_chamber->stack_value();

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    make_event<ground_aoe_event_t>(
        *sim, p(),
        ground_aoe_params_t()
          .target( execute_state->target )
          .duration( data().duration() + p()->talent.unrelenting_calamity->effectN(2).time_value() )
          .action( rumble ) );
  }
};

struct earthquake_overload_damage_t : public earthquake_damage_base_t
{
  earthquake_overload_damage_t( shaman_t* player, earthquake_base_t* parent ) :
    earthquake_damage_base_t( player, "earthquake_overload_damage", player->find_spell( 298765 ), parent )
  {
      auto coeff = get_spell_power_coefficient_from_options();
      if ( coeff == 0.0 )
      {
          coeff = get_spell_power_coefficient_from_sdb();
      }

      spell_power_mod.direct = coeff * player->talent.mountains_will_fall->effectN( 1 ).percent();
  }
};

struct earthquake_overload_t : public earthquake_base_t
{
  earthquake_base_t* parent;

  earthquake_overload_t( shaman_t* player, earthquake_base_t* p ) :
    earthquake_base_t( player, "earthquake_overload", player->find_spell( 298762 ) ),
    parent( p )
  {
    background = true;
    callbacks = false;
    base_execute_time = 0_s;

    rumble = new earthquake_overload_damage_t( player, this );
    add_child( rumble );
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    earthquake_base_t::snapshot_internal( s, flags, rt );

    cast_state( s )->exec_type = parent->exec_type;
  }
};

struct earthquake_damage_t : public earthquake_damage_base_t
{
  earthquake_damage_t( shaman_t* player, earthquake_base_t* parent = nullptr ) :
    earthquake_damage_base_t( player, "earthquake_damage", player->find_spell( 77478 ), parent )
  {
      auto coeff = get_spell_power_coefficient_from_options();

      if ( coeff == 0.0 )
      {
          coeff = get_spell_power_coefficient_from_sdb();
      }

      spell_power_mod.direct = coeff;
  }
};

struct earthquake_t : public earthquake_base_t
{
  earthquake_t( shaman_t* player, util::string_view options_str ) :
    earthquake_base_t( player, "earthquake", player->talent.earthquake_reticle.ok()
      ? player->talent.earthquake_reticle
      : player->talent.earthquake_target )
  {
    parse_options( options_str );

    rumble = new earthquake_damage_t( player, this );
    add_child( rumble );
    affected_by_master_of_the_elements = true;

    if ( player->talent.mountains_will_fall.ok() )
    {
      overload = new earthquake_overload_t( player, this );
      add_child( overload );
    }
  }

  // Earthquake uses a "smart" Lightning Rod targeting system
  // 1) Current target, if Lightning Rod is not enabled on it
  // 2) A close-by target without Lightning Rod
  //
  // Note that Earthquake does not refresh existing Lightning Rod debuffs
  void trigger_lightning_rod( const action_state_t* state )
  {
    if ( !p()->talent.lightning_rod.ok() )
    {
      return;
    }

    auto tdata = td( state->target );
    if ( !tdata->debuff.lightning_rod->check() )
    {
      trigger_lightning_rod_debuff( state->target );
    }
    else
    {
      std::vector<player_t*> eligible_targets;
      range::for_each( target_list(), [ this, &eligible_targets ]( player_t* t ) {
        if ( !td( t )->debuff.lightning_rod->check() )
        {
          eligible_targets.emplace_back( t );
        }
      } );

      if ( !eligible_targets.empty() )
      {
        auto idx = rng().range( 0U, as<unsigned>( eligible_targets.size() ) );
        trigger_lightning_rod_debuff( eligible_targets[ idx ] );
      }
    }
  }

  void execute() override
  {
    earthquake_base_t::execute();

    trigger_lightning_rod( execute_state );

    if ( p()->talent.surge_of_power->ok() )
    {
      p()->buff.surge_of_power->trigger();
    }

    // Note, needs to be decremented after ground_aoe_event_t is created so that the rumble gets the
    // buff multiplier as persistent.
    p()->track_magma_chamber();
    p()->buff.magma_chamber->expire();

    p()->buff.master_of_the_elements->decrement();
    p()->buff.echoes_of_great_sundering_es->decrement();
    p()->buff.echoes_of_great_sundering_eb->decrement();

    if ( exec_type == spell_variant::NORMAL )
    {
      p()->buff.storm_frenzy->trigger();
    }
  }
};

struct spirit_walk_t : public shaman_spell_t
{
  spirit_walk_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "spirit_walk", player, player->talent.spirit_walk )
  {
    parse_options( options_str );
    may_miss = may_crit = harmful = callbacks = false;

    cooldown->duration += player->talent.go_with_the_flow->effectN( 1 ).time_value();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.spirit_walk->trigger();
  }
};

struct ghost_wolf_t : public shaman_spell_t
{
  ghost_wolf_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "ghost_wolf", player, player->find_class_spell( "Ghost Wolf" ) )
  {
    parse_options( options_str );
    unshift_ghost_wolf = false;  // Customize unshifting logic here
    harmful = callbacks = may_crit = false;
  }

  timespan_t gcd() const override
  {
    if ( p()->buff.ghost_wolf->check() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::gcd();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( !p()->buff.ghost_wolf->check() )
    {
      p()->buff.ghost_wolf->trigger();
    }
    else
    {
      p()->buff.ghost_wolf->expire();
    }
  }
};

struct feral_lunge_t : public shaman_spell_t
{
  struct feral_lunge_attack_t : public shaman_attack_t
  {
    feral_lunge_attack_t( shaman_t* p ) : shaman_attack_t( "feral_lunge_attack", p, p->find_spell( 215802 ) )
    {
      background = true;
      callbacks = false;
    }

    void init() override
    {
      shaman_attack_t::init();

      may_proc_windfury = may_proc_flametongue = false;

      p()->set_mw_proc_state( this, mw_proc_state::DISABLED );
    }
  };

  feral_lunge_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "feral_lunge", player, player->spec.feral_lunge )
  {
    parse_options( options_str );
    unshift_ghost_wolf = false;

    impact_action = new feral_lunge_attack_t( player );
  }

  bool ready() override
  {
    if ( !p()->buff.ghost_wolf->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Nature's Swiftness Spell =================================================

struct natures_swiftness_t : public shaman_spell_t
{
  natures_swiftness_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "natures_swiftness", player, player->talent.natures_swiftness )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.natures_swiftness->trigger();
  }

  bool ready() override
  {
    if ( p()->talent.ancestral_swiftness.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Ancestral Swiftness Spell =================================================

struct ancestral_swiftness_t : public shaman_spell_t
{
  ancestral_swiftness_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "ancestral_swiftness", player, player->find_spell( 443454 ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.ancestral_swiftness->trigger();

    if ( p()->talent.natures_swiftness.ok() )
    {
      p()->summon_ancestor();
    }
  }

  bool ready() override
  {
    if ( !p()->talent.ancestral_swiftness.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================
struct earth_shock_overload_t : public elemental_overload_spell_t
{
  earth_shock_overload_t( shaman_t* p, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, "earth_shock_overload", p->find_spell( 381725 ), parent_,
        p->talent.mountains_will_fall->effectN( 1 ).percent() )
  {
    affected_by_master_of_the_elements = true;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.magma_chamber->check_stack_value();

    return m;
  }
};

struct earth_shock_t : public shaman_spell_t
{
  earth_shock_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "earth_shock", player, player->talent.earth_shock )
  {
    parse_options( options_str );
    // hardcoded because spelldata doesn't provide the resource type
    resource_current                   = RESOURCE_MAELSTROM;
    affected_by_master_of_the_elements = true;
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( p()->talent.mountains_will_fall.enabled() )
    {
      overload = new earth_shock_overload_t( player, this );
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.magma_chamber->stack_value();

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->talent.echoes_of_great_sundering.ok() )
    {
      p()->buff.echoes_of_great_sundering_eb->expire();
      p()->buff.echoes_of_great_sundering_es->trigger();
    }

    if (p()->talent.surge_of_power->ok() )
    {
      p()->buff.surge_of_power->trigger();
    }

    p()->track_magma_chamber();
    p()->buff.magma_chamber->expire();
    p()->buff.storm_frenzy->trigger();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->talent.lightning_rod.ok() )
    {
      trigger_lightning_rod_debuff( state->target );
    }
  }

  bool ready() override
  {
    bool r = shaman_spell_t::ready();
    if ( p()->talent.elemental_blast.enabled() )
    {
      r = false;
    }
    return r;
  }
};
// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
private:
  flame_shock_spreader_t* spreader;
  const spell_data_t* elemental_resource;

  void track_flame_shock( const action_state_t* state )
  {
    // No need to track anything if there are not enough enemies
    if ( sim->target_list.size() <= p()->max_active_flame_shock )
    {
      return;
    }

    // Remove tracking on the newly applied dot. It'll be re-added to the tracking at the
    // end of this method to keep ascending start-time order intact.
    auto dot = get_dot( state->target );
    untrack_flame_shock( dot );

    // Max targets reached (the new Flame Shock application is on a target without a dot
    // active), remove one of the oldest applied dots to make room.
    if ( p()->active_flame_shock.size() == p()->max_active_flame_shock )
    {
      auto start_time = p()->active_flame_shock.front().first;
      auto entry = range::find_if( p()->active_flame_shock, [ start_time ]( const auto& entry ) {
          return entry.first != start_time;
      } );

      // Randomize equal start time application removal
      auto candidate_targets = as<double>(
          std::distance( p()->active_flame_shock.begin(), entry ) );
      auto idx = static_cast<unsigned>( rng().range( 0.0, candidate_targets ) );

      if ( sim->debug )
      {
        std::vector<util::string_view> enemies;
        for ( auto it = p()->active_flame_shock.begin(); it < entry; ++it )
        {
          enemies.emplace_back( it->second->target->name() );
        }

        sim->out_debug.print(
          "{} canceling oldest {}: new_target={} cancel_target={} (index={}), start_time={}, "
          "candidate_targets={} ({})",
          player->name(), name(), state->target->name(),
          p()->active_flame_shock[ idx ].second->state->target->name(), idx,
          p()->active_flame_shock[ idx ].first, as<unsigned>( candidate_targets ),
          util::string_join( enemies ) );
      }

      p()->active_flame_shock[ idx ].second->cancel();
    }

    p()->active_flame_shock.emplace_back( sim->current_time(), dot );
  }

  void untrack_flame_shock( const dot_t* d )
  {
    unsigned max_targets = as<unsigned>( data().max_targets() );

    // No need to track anything if there are not enough enemies
    if ( sim->target_list.size() <= max_targets )
    {
      return;
    }

    auto it = range::find_if( p()->active_flame_shock, [ d ]( const auto& dot_state ) {
      return dot_state.second == d;
    } );

    if ( it != p()->active_flame_shock.end() )
    {
      p()->active_flame_shock.erase( it );
    }
  }

  void invalidate_dependant_targets()
  {
    range::for_each( p()->flame_shock_dependants, []( action_t* a ) {
      a->target_cache.is_valid = false;
    } );
  }

public:
  flame_shock_t( shaman_t* player, util::string_view options_str = {} )
    : shaman_spell_t( "flame_shock", player, player->find_class_spell( "Flame Shock" ) ),
      spreader( player->talent.surge_of_power->ok() ? new flame_shock_spreader_t( player ) : nullptr ),
    elemental_resource( player->find_spell( 263819 ) )
  {
    parse_options( options_str );

    affected_by_master_of_the_elements = true;

    // Ensure Flame Shock is single target, since Simulationcraft naively interprets a
    // Max Targets value on a spell to mean "aoe this many targets"
    aoe = 0;
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( player->specialization() == SHAMAN_ENHANCEMENT )
    {
      cooldown->duration = data().cooldown();
      cooldown->hasted   = data().affected_by( p()->spec.enhancement_shaman->effectN( 8 ) );
    }
    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      cooldown->duration = data().cooldown() + p()->talent.flames_of_the_cauldron->effectN( 2 ).time_value();
    }
  }

  void trigger_dot( action_state_t* state ) override
  {
    if ( !get_dot( state->target )->is_ticking() )
    {
      invalidate_dependant_targets();
    }

    track_flame_shock( state );

    shaman_spell_t::trigger_dot( state );
  }

  double composite_crit_chance() const override
  {
    double m = shaman_spell_t::composite_crit_chance();

    m += p()->talent.skybreakers_fiery_demise->effectN( 3 ).percent();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );

    m *= 1.0 + td( t )->debuff.lashing_flames->stack_value();

    return m;
  }

  double dot_duration_pct_multiplier( const action_state_t* s ) const override
  {
    auto mul = shaman_spell_t::dot_duration_pct_multiplier( s );

    if ( p()->buff.fire_elemental->check() && p()->spell.fire_elemental->ok() )
    {
      mul *= 1.0 + p()->spell.fire_elemental->effectN( 3 ).percent();
    }

    if ( p()->buff.lesser_fire_elemental->check() )
    {
      mul *= 1.0 + p()->buff.lesser_fire_elemental->data().effectN( 3 ).percent();
    }

    return mul;
  }

  double tick_time_pct_multiplier( const action_state_t* state ) const override
  {
    auto mul = shaman_spell_t::tick_time_pct_multiplier( state );

    mul *= 1.0 + p()->buff.fire_elemental->stack_value();
    mul *= 1.0 + p()->buff.lesser_fire_elemental->stack_value();

    mul *= 1.0 + p()->talent.flames_of_the_cauldron->effectN( 1 ).percent();

    return mul;
  }

  void tick( dot_t* d ) override
  {
    shaman_spell_t::tick( d );

    if ( p()->spec.lava_surge->ok() )
    {
      double active_flame_shocks = p()->get_active_dots( d );
      p()->lava_surge_attempts_normalized += 1.0/active_flame_shocks;
      double proc_chance =
          std::max( 0.0, 0.6-std::pow(1.16, -2*(p()->lava_surge_attempts_normalized-5)));

      if ( p()->spec.restoration_shaman->ok() )
      {
        proc_chance += p()->spec.restoration_shaman->effectN( 7 ).percent();
      }

      if ( rng().roll( proc_chance ) )
      {
        p()->trigger_lava_surge();
        p()->lvs_samples.add( p()->lava_surge_attempts_normalized );
        p()->lava_surge_attempts_normalized = 0.0;
      }
    }

    if ( d->state->result == RESULT_CRIT && p()->talent.skybreakers_fiery_demise.ok() )
    {
      p()->cooldown.storm_elemental->adjust( -1.0 * p()->talent.skybreakers_fiery_demise->effectN( 1 ).time_value() );
      p()->cooldown.fire_elemental->adjust( -1.0 * p()->talent.skybreakers_fiery_demise->effectN( 2 ).time_value() );
    }

    if ( p()->talent.ashen_catalyst.ok() && d->state->result_amount > 0 )
    {
      auto reduction = p()->talent.ashen_catalyst->effectN( 1 ).base_value() / 10.0;
      reduction /= 1.0 + p()->buff.hot_hand->check_value();

      p()->cooldown.lava_lash->adjust( timespan_t::from_seconds( -reduction ) );
      p()->buff.ashen_catalyst->trigger();
    }

    // TODO: Determine proc chance / model
    // First single target test showed a 25% chance. I didn't find it in
    // spelldata.
    if ( p()->talent.searing_flames->ok() && rng().roll( 0.25 ) )
    {
      p()->trigger_maelstrom_gain( p()->talent.searing_flames->effectN( 1 ).base_value(), p()->gain.searing_flames );
      p()->proc.searing_flames->occur();
    }

    if ( p()->talent.magma_chamber->ok() )
    {
      p()->buff.magma_chamber->trigger();
    }

  }

  void last_tick( dot_t* d ) override
  {
    shaman_spell_t::last_tick( d );

    untrack_flame_shock( d );
    invalidate_dependant_targets();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->buff.surge_of_power->up() && sim->target_non_sleeping_list.size() == 1 )
    {
      p()->proc.surge_of_power_wasted->occur();
      p()->buff.surge_of_power->decrement();
    }

    if ( p()->buff.surge_of_power->up() && sim->target_non_sleeping_list.size() > 1 )
    {
      shaman_td_t* source_td = td( target );
      player_t* additional_target = nullptr;

      spreader->set_target( state->target );
      // If all targets have flame shock, pick the shortest remaining time
      if ( player->get_active_dots( source_td->dot.flame_shock ) ==
           sim->target_non_sleeping_list.size() )
      {
        additional_target = spreader->shortest_duration_target();
      }
      // Pick closest target without Flame Shock
      else
      {
        additional_target = spreader->closest_target();
      }
      if ( additional_target )
      {
        // expire first to prevent infinity
        p()->proc.surge_of_power_flame_shock->occur();
        p()->buff.surge_of_power->decrement();
        p()->trigger_secondary_flame_shock( additional_target );
      }
    }
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "frost_shock", player, player->talent.frost_shock )
  {
    parse_options( options_str );
    affected_by_master_of_the_elements = true;
    maelstrom_gain_per_target = false;
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( player->specialization() == SHAMAN_ENHANCEMENT )
    {
      cooldown->duration = p()->spec.enhancement_shaman->effectN( 7 ).time_value();
      cooldown->hasted   = data().affected_by( p()->spec.enhancement_shaman->effectN( 8 ) );
      track_cd_waste = true;
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.icefury_dmg->value();

    m *= 1.0 + p()->buff.hailstorm->stack_value();

    m *= 1.0 + p()->buff.ice_strike->stack_value();

    return m;
  }

  int n_targets() const override
  {
    int t = shaman_spell_t::n_targets();

    if ( p()->buff.hailstorm->check() )
    {
      // sure would be nice to have good looking client data
      //auto additionalMaxTargets = p()->talent.hailstorm->effectN( 1 ).base_value() * 100;
      int additionalMaxTargets = 5;
      auto targets = p()->buff.hailstorm->check() > additionalMaxTargets
                         ? additionalMaxTargets : p()->buff.hailstorm->check();
      t += targets;
    }

    return t;
  }

  void execute() override
  {
    if ( p()->buff.icefury_dmg->up() )
    {
      maelstrom_gain = p()->spec.maelstrom->effectN( 7 ).resource( RESOURCE_MAELSTROM );
    }

    shaman_spell_t::execute();

    if ( p()->buff.hailstorm->check() >=
         p()->talent.swirling_maelstrom->effectN( 2 ).base_value() )
    {
      p()->trigger_swirling_maelstrom( execute_state );
    }

    if ( p()->buff.hailstorm->up() )
    {
      p()->trigger_reactivity( execute_state );
    }

    p()->buff.flux_melting->trigger();

    p()->buff.icefury_dmg->decrement();

    p()->buff.hailstorm->expire();
    p()->buff.ice_strike->expire();

    if ( p()->buff.surge_of_power->up())
    {
      p()->proc.surge_of_power_wasted->occur();
      p()->buff.surge_of_power->decrement();
    }

    maelstrom_gain = 0.0;
  }

  bool ready() override
  {
    if ( p()->buff.icefury_cast->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "wind_shear", player, player->talent.wind_shear )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;
    is_interrupt = true;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->debuffs.casting && !candidate_target->debuffs.casting->check() )
    {
      return false;
    }

    return shaman_spell_t::target_ready( candidate_target );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->spec.inundate->ok() )
    {
      p()->trigger_maelstrom_gain( p()->spell.inundate->effectN( 1 ).base_value(), p()->gain.inundate );
    }
  }
};

// Ascendance Enhance Damage Spell =========================================================

struct ascendance_damage_t : public shaman_spell_t
{
  ascendance_damage_t( shaman_t* player, util::string_view name_str )
    : shaman_spell_t( name_str, player, player->find_spell( 344548 ) )
  {
    aoe = -1;
    background = true;
  }
};

// Ascendance Spell =========================================================

struct ascendance_t : public shaman_spell_t
{
  ascendance_damage_t* ascendance_damage;
  lava_burst_t* lvb;

  ascendance_t( shaman_t* player, util::string_view name_str, util::string_view options_str = {} ) :
    shaman_spell_t( name_str, player, player->spell.ascendance ),
    ascendance_damage( nullptr ), lvb( nullptr )
  {
    parse_options( options_str );
    harmful = false;

    if ( ascendance_damage )
    {
      add_child( ascendance_damage );
    }
    // Periodic effect for Enhancement handled by the buff
    dot_duration = base_tick_time = timespan_t::zero();
    ancestor_trigger = ancestor_cast::CHAIN_LIGHTNING;

    // Cache pointer for MW tracking uses
    p()->action.ascendance = this;
  }

  void init() override
  {
    shaman_spell_t::init();

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      if ( auto trigger_spell = p()->find_action( "lava_burst_ascendance" ) )
      {
        lvb = debug_cast<lava_burst_t*>( trigger_spell );
      }
      else
      {
        lvb = new lava_burst_t( p(), spell_variant::ASCENDANCE );
        add_child( lvb );
      }
    }

    if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( auto trigger_spell = p()->find_action( "ascendance_damage" ) )
      {
        ascendance_damage = debug_cast<ascendance_damage_t*>( trigger_spell );
      }
      else
      {
        ascendance_damage = new ascendance_damage_t( p(), "ascendance_damage" );
        add_child( ascendance_damage );
      }
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->cooldown.strike->reset( false );

    auto dre_duration = p()->talent.deeply_rooted_elements->effectN( 1 ).time_value();

    if ( background )
    {
      p()->buff.ascendance->extend_duration_or_trigger( dre_duration, player );
    }
    else
    {
      p()->buff.ascendance->trigger();
    }

    if ( lvb )
    {
      lvb->set_target( player->target );
      lvb->target_cache.is_valid = false;
      if ( !lvb->target_list().empty() )
      {
        lvb->execute();
      }
    }

    if ( ascendance_damage )
    {
      ascendance_damage->set_target( target );
      ascendance_damage->execute();
    }

    // Refresh Flame Shock to max duration
    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      auto max_duration = p()->action.flame_shock->composite_dot_duration( execute_state );

      // Apparently the Flame Shock durations get set to current Flame Shock max duration,
      // bypassing normal dot refresh behavior.
      range::for_each( sim->target_non_sleeping_list, [ this, max_duration ]( player_t* target ) {
        auto fs_dot = td( target )->dot.flame_shock;
        if ( fs_dot->is_ticking() )
        {
          auto new_duration = max_duration < fs_dot->remains()
                              ? -( fs_dot->remains() - max_duration )
                              : max_duration - fs_dot->remains();
          fs_dot->adjust_duration( new_duration, timespan_t::min(), -1, true );
        }
      } );
    }

    if ( p()->talent.static_accumulation.ok() )
    {
      if ( background )
      {
        p()->buff.static_accumulation->extend_duration_or_trigger( dre_duration, player );
      }
      else
      {
        p()->buff.static_accumulation->trigger();
      }
    }
  }

  bool ready() override
  {
    if ( !p()->talent.ascendance->ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

struct ascendance_dre_t : public ascendance_t
{
  ascendance_dre_t( shaman_t* player ) : ascendance_t( player, "ascendance_dre" )
  {
    background = true;
    cooldown->duration = 0_s;
  }

  // Note, bypasses calling ascendance_t::init() to not bother initializing the ascendance
  // version for the lava burst
  void init() override
  {
    shaman_spell_t::init();

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      if ( auto trigger_spell = p()->find_action( "lava_burst_dre" ) )
      {
        lvb = debug_cast<lava_burst_t*>( trigger_spell );
      }
      else
      {
        lvb = new lava_burst_t( p(), spell_variant::DEEPLY_ROOTED_ELEMENTS );
        add_child( lvb );
      }
    }

    if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( auto trigger_spell = p()->find_action( "ascendance_damage_dre" ) )
      {
        ascendance_damage = debug_cast<ascendance_damage_t*>( trigger_spell );
      }
      else
      {
        ascendance_damage = new ascendance_damage_t( p(), "ascendance_damage_dre" );
        add_child( ascendance_damage );
      }
    }
  }
};

// Stormkeeper Spell ========================================================

struct stormkeeper_t : public shaman_spell_t
{
  stormkeeper_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "stormkeeper", player, player->find_spell( 191634 ) )
  {
    parse_options( options_str );
    may_crit = harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.stormkeeper->trigger( p()->buff.stormkeeper->max_stack() );

    if ( p()->buff.fury_of_the_storms->trigger() )
    {
      p() -> pet.lightning_elemental.spawn( p()->buff.fury_of_the_storms->buff_duration() );
    }
  }

  bool ready() override
  {
    if ( !p()->talent.stormkeeper.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Doom Winds Spell ===========================================================

struct doom_winds_t : public shaman_attack_t
{
  doom_winds_t( shaman_t* player, util::string_view options_str ) :
    shaman_attack_t( "doom_winds", player, player->talent.doom_winds )
  {
    parse_options( options_str );

    weapon = &( player->main_hand_weapon );
    weapon_multiplier = 0.0;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_stormbringer = false;
  }

  void execute() override
  {
    p()->buff.doom_winds->trigger();

    shaman_attack_t::execute();
  }
};

// Ancestral Guidance Spell ===================================================

struct ancestral_guidance_t : public shaman_heal_t
{
  ancestral_guidance_t( shaman_t* player, util::string_view options_str ) :
    shaman_heal_t( "ancestral_guidance", player, player->talent.ancestral_guidance )
  {
    parse_options( options_str );
  }
};

// Healing Surge Spell ======================================================

struct healing_surge_t : public shaman_heal_t
{
  healing_surge_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("healing_surge", player, player->find_class_spell( "Healing Surge" ), options_str )
  {
    resurgence_gain =
        0.6 * p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }

  double composite_crit_chance() const override
  {
    double c = shaman_heal_t::composite_crit_chance();

    if ( p()->buff.tidal_waves->up() )
    {
      c += p()->spec.tidal_waves->effectN( 1 ).percent();
    }

    return c;
  }
};

// Healing Wave Spell =======================================================

struct healing_wave_t : public shaman_heal_t
{
  healing_wave_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("healing_wave", player, player->find_specialization_spell( "Healing Wave" ), options_str )
  {
    resurgence_gain =
        p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = shaman_heal_t::execute_time_pct_multiplier();

    if ( p()->buff.tidal_waves->up() )
    {
      mul *= 1.0 - p()->spec.tidal_waves->effectN( 1 ).percent();
    }

    return mul;
  }
};

// Greater Healing Wave Spell ===============================================

struct greater_healing_wave_t : public shaman_heal_t
{
  greater_healing_wave_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("greater_healing_wave", player, player->find_specialization_spell( "Greater Healing Wave" ), options_str )
  {
    resurgence_gain =
        p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = shaman_heal_t::execute_time_pct_multiplier();

    if ( p()->buff.tidal_waves->up() )
    {
      mul *= 1.0 - p()->spec.tidal_waves->effectN( 1 ).percent();
    }

    return mul;
  }
};

// Riptide Spell ============================================================

struct riptide_t : public shaman_heal_t
{
  riptide_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("riptide", player, player->find_specialization_spell( "Riptide" ), options_str )
  {
    resurgence_gain =
        0.6 * p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }
};

// Chain Heal Spell =========================================================

struct chain_heal_t : public shaman_heal_t
{
  chain_heal_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("chain_heal", player, player->find_class_spell( "Chain Heal" ), options_str )
  {
    resurgence_gain =
        0.333 * p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = shaman_heal_t::composite_target_da_multiplier( t );

    if ( td( t )->heal.riptide && td( t )->heal.riptide->is_ticking() )
      m *= 1.0 + p()->spec.riptide->effectN( 3 ).percent();

    return m;
  }
};

// Healing Rain Spell =======================================================

struct healing_rain_t : public shaman_heal_t
{
  struct healing_rain_aoe_tick_t : public shaman_heal_t
  {
    healing_rain_aoe_tick_t( shaman_t* player )
      : shaman_heal_t( "healing_rain_tick", player, player->find_spell( 73921 ) )
    {
      background = true;
      aoe        = -1;
    }
  };

  healing_rain_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t( "healing_rain", player, player->find_specialization_spell( "Healing Rain" ),
                     options_str )
  {
    base_tick_time = data().effectN( 2 ).period();
    dot_duration   = data().duration();
    hasted_ticks   = false;
    tick_action    = new healing_rain_aoe_tick_t( player );
  }
};

// Totemic Recall Spell =====================================================

struct totemic_recall_t : public shaman_spell_t
{
  totemic_recall_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "totemic_recall", player, player->talent.totemic_recall )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->action.totemic_recall_totem )
    {
      p()->action.totemic_recall_totem->cooldown->reset( false );
    }
  }
};


// ==========================================================================
// Shaman Totem System
// ==========================================================================

template <typename T>
struct shaman_totem_pet_t : public pet_t
{
  // Pulse related functionality
  totem_pulse_action_t<T>* pulse_action;
  event_t* pulse_event;
  timespan_t pulse_amplitude;
  bool pulse_on_expire;

  // Summon related functionality
  std::string pet_name;
  pet_t* summon_pet;

  shaman_totem_pet_t( shaman_t* p, util::string_view n )
    : pet_t( p->sim, p, n, true ),
      pulse_action( nullptr ),
      pulse_event( nullptr ),
      pulse_amplitude( timespan_t::zero() ),
      pulse_on_expire( true ),
      summon_pet( nullptr )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  void summon( timespan_t = timespan_t::zero() ) override;
  void dismiss( bool expired = false ) override;

  void init_finished() override
  {
    if ( !pet_name.empty() )
    {
      summon_pet = owner->find_pet( pet_name );
    }

    pet_t::init_finished();
  }

  shaman_t* o()
  {
    return debug_cast<shaman_t*>( owner );
  }

  /*
  //  Code to make a totem double dip on player multipliers.
  //  As of 7.3.5 this is no longer needed for Liquid Magma Totem (Elemental)
  virtual double composite_player_multiplier( school_e school ) const override
  { return owner -> cache.player_multiplier( school ); }
  //*/

  double composite_spell_hit() const override
  {
    return owner->cache.spell_hit();
  }

  double composite_spell_crit_chance() const override
  {
    return owner->cache.spell_crit_chance();
  }

  double composite_spell_power( school_e school ) const override
  {
    return owner->composite_spell_power( school );
  }

  double composite_spell_power_multiplier() const override
  {
    return owner->composite_spell_power_multiplier();
  }

  double composite_total_spell_power( school_e school ) const override
  {
    return owner->composite_total_spell_power( school );
  }

  double composite_total_attack_power_by_type( attack_power_type type ) const override
  {
    return owner->composite_total_attack_power_by_type( type );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, duration );

    return pet_t::create_expression( name );
  }

  void apply_affecting_auras( action_t& action ) override
  {
    o()->apply_affecting_auras( action );
  }
};

template <typename TOTEM, typename BASE>
struct shaman_totem_t : public BASE
{
  timespan_t totem_duration;
  spawner::pet_spawner_t<TOTEM, shaman_t>& spawner;

  shaman_totem_t( util::string_view totem_name, shaman_t* player, util::string_view options_str,
                  const spell_data_t* spell_data, spawner::pet_spawner_t<TOTEM, shaman_t>& sp ) :
    BASE( totem_name, player, spell_data ),
    totem_duration( this->data().duration() ), spawner( sp )
  {
    this->parse_options( options_str );
    this->harmful = this->callbacks = this->may_miss = this->may_crit = false;
    this->ignore_false_positive = true;
    this->dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    BASE::execute();

    spawner.spawn( totem_duration );

    // Cooldown threshold is hardcoded into the spell description
    if ( this->p()->talent.totemic_recall.ok() && this->data().cooldown() < 180_s )
    {
      this->p()->action.totemic_recall_totem = this;
    }
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    // Redirect active/remains to "pet.<totem name>.active/remains" so things work ok with the
    // pet initialization order shenanigans. Otherwise, at this point in time (when
    // create_expression is called), the pets don't actually exist yet.
    if ( util::str_compare_ci( name, "active" ) )
      return this->player->create_expression( "pet." + this->name_str + ".active" );
    else if ( util::str_compare_ci( name, "remains" ) )
      return this->player->create_expression( "pet." + this->name_str + ".remains" );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, totem_duration );

    return BASE::create_expression( name );
  }
};

template <typename T>
struct totem_pulse_action_t : public T
{
  bool hasted_pulse;
  double pulse_multiplier;
  shaman_totem_pet_t<T>* totem;
  unsigned pulse;

  bool affected_by_enh_mastery_da;
  bool affected_by_enh_mastery_ta;

  bool affected_by_ele_mastery_da;
  bool affected_by_ele_mastery_ta;

  bool affected_by_totemic_rebound_da;

  bool affected_by_amplification_core_da;
  bool affected_by_amplification_core_ta;

  bool affected_by_molten_weapon_da;
  bool affected_by_molten_weapon_ta;

  bool affected_by_crackling_surge_da;
  bool affected_by_crackling_surge_ta;

  bool affected_by_earthen_weapon_da;
  bool affected_by_earthen_weapon_ta;

  bool affected_by_lotfw_da;
  bool affected_by_lotfw_ta;

  totem_pulse_action_t( const std::string& token, shaman_totem_pet_t<T>* p, const spell_data_t* s )
    : T( token, p, s ), hasted_pulse( false ), pulse_multiplier( 1.0 ), totem( p ), pulse ( 0 )
  {
    this->may_crit = this->background = true;
    this->callbacks             = false;

    if ( this->type == ACTION_HEAL )
    {
      this->harmful = false;
      this->target = totem->owner;
    }
    else
    {
      this->harmful = true;
    }

    affected_by_enh_mastery_da = T::data().affected_by( o()->mastery.enhanced_elements->effectN( 1 ) );
    affected_by_enh_mastery_ta = T::data().affected_by( o()->mastery.enhanced_elements->effectN( 5 ) );
    affected_by_ele_mastery_da        = T::data().affected_by( o()->mastery.elemental_overload->effectN( 4 ) );
    affected_by_ele_mastery_ta        = T::data().affected_by( o()->mastery.elemental_overload->effectN( 5 ) );
    affected_by_amplification_core_da = T::data().affected_by( o()->buff.amplification_core->data().effectN( 1 ) );
    affected_by_amplification_core_ta = T::data().affected_by( o()->buff.amplification_core->data().effectN( 2 ) );
    affected_by_totemic_rebound_da = T::data().affected_by_all( o()->buff.totemic_rebound->data().effectN( 1 ) ) ||
                                     T::data().affected_by_all( o()->buff.totemic_rebound->data().effectN( 2 ) );
    affected_by_molten_weapon_da = T::data().affected_by( o()->buff.molten_weapon->data().effectN( 1 ) );
    affected_by_molten_weapon_ta = T::data().affected_by( o()->buff.molten_weapon->data().effectN( 2 ) );
    affected_by_crackling_surge_da = T::data().affected_by( o()->buff.crackling_surge->data().effectN( 1 ) );
    affected_by_crackling_surge_ta = T::data().affected_by( o()->buff.crackling_surge->data().effectN( 2 ) );
    affected_by_earthen_weapon_da = T::data().affected_by( o()->buff.earthen_weapon->data().effectN( 1 ) );
    affected_by_earthen_weapon_ta = T::data().affected_by( o()->buff.earthen_weapon->data().effectN( 2 ) );
    affected_by_lotfw_da = T::data().affected_by( o()->buff.legacy_of_the_frost_witch->data().effectN( 1 ) );
    affected_by_lotfw_ta = T::data().affected_by( o()->buff.legacy_of_the_frost_witch->data().effectN( 2 ) );
  }

  void init() override
  {
    T::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( totem->o()->pet_list,
          [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != totem->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }

  shaman_t* o() const
  {
    return debug_cast<shaman_t*>( this->player->cast_pet()->owner );
  }

  shaman_td_t* td( player_t* target ) const
  {
    return o()->get_target_data( target );
  }

  double action_multiplier() const override
  {
    double m = T::action_multiplier();

    m *= pulse_multiplier;

    return m;
  }

  double action_da_multiplier() const override
  {
    double m = T::action_da_multiplier();

    if ( affected_by_enh_mastery_da )
    {
      m *= 1.0 + o()->cache.mastery_value();
    }

    if ( affected_by_ele_mastery_da )
    {
      m *= 1.0 + o()->mastery.elemental_overload->effectN( 4 ).mastery_value() * o()->cache.mastery();
    }

    if ( affected_by_totemic_rebound_da )
    {
      m *= 1.0 + o()->buff.totemic_rebound->stack_value();
    }

    if ( affected_by_lotfw_da && o()->buff.legacy_of_the_frost_witch->check() )
    {
      m *= 1.0 + o()->buff.legacy_of_the_frost_witch->value();
    }

    if ( affected_by_amplification_core_da && o()->buff.amplification_core->check() )
    {
      m *= 1.0 + o()->buff.amplification_core->value();
    }

    if ( affected_by_molten_weapon_da && o()->buff.molten_weapon->check() )
    {
      for ( int x = 1; x <= o()->buff.molten_weapon->check(); x++ )
      {
        m *= 1.0 + o()->buff.molten_weapon->value();
      }
    }

    if ( affected_by_crackling_surge_da && o()->buff.crackling_surge->check() )
    {
      for ( int x = 1; x <= o()->buff.crackling_surge->check(); x++ )
      {
        m *= 1.0 + o()->buff.crackling_surge->value();
      }
    }

    if ( affected_by_earthen_weapon_da && o()->buff.earthen_weapon->check() )
    {
      for ( int x = 1; x <= o()->buff.earthen_weapon->check(); x++ )
      {
        m *= 1.0 + o()->buff.earthen_weapon->value();
      }
    }

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = T::action_ta_multiplier();

    if ( affected_by_enh_mastery_ta )
    {
      m *= 1.0 + o()->cache.mastery_value();
    }

    if ( affected_by_ele_mastery_ta )
    {
      m *= 1.0 + o()->mastery.elemental_overload->effectN( 5 ).mastery_value() * o()->cache.mastery();
    }

    if ( affected_by_lotfw_ta && o()->buff.legacy_of_the_frost_witch->check() )
    {
      m *= 1.0 + o()->buff.legacy_of_the_frost_witch->value();
    }

    if ( affected_by_amplification_core_ta && o()->buff.amplification_core->check() )
    {
      m *= 1.0 + o()->buff.amplification_core->value();
    }

    if ( affected_by_molten_weapon_ta && o()->buff.molten_weapon->check() )
    {
      for ( int x = 1; x <= o()->buff.molten_weapon->check(); x++ )
      {
        m *= 1.0 + o()->buff.molten_weapon->value();
      }
    }

    if ( affected_by_crackling_surge_ta && o()->buff.crackling_surge->check() )
    {
      for ( int x = 1; x <= o()->buff.crackling_surge->check(); x++ )
      {
        m *= 1.0 + o()->buff.crackling_surge->value();
      }
    }

    if ( affected_by_earthen_weapon_ta && o()->buff.earthen_weapon->check() )
    {
      for ( int x = 1; x <= o()->buff.earthen_weapon->check(); x++ )
      {
        m *= 1.0 + o()->buff.earthen_weapon->value();
      }
    }

    return m;
  }

  void execute() override
  {
    T::execute();

    pulse++;
  }

  void reset() override
  {
    T::reset();
    pulse_multiplier = 1.0;
    pulse = 0;
  }

  /// Reset the internal counters relating to totem pulsing
  void reset_pulse()
  {
    pulse_multiplier = 1.0;
    pulse = 0;
  }

};

template <typename T>
struct totem_pulse_event_t : public event_t
{
  shaman_totem_pet_t<T>* totem;
  timespan_t real_amplitude;

  totem_pulse_event_t( shaman_totem_pet_t<T>& t, timespan_t amplitude )
    : event_t( t ), totem( &t ), real_amplitude( amplitude )
  {
    if ( totem->pulse_action->hasted_pulse )
      real_amplitude *= totem->cache.spell_cast_speed();

    schedule( real_amplitude );
  }

  const char* name() const override
  { return "totem_pulse"; }

  void execute() override
  {
    if ( totem->pulse_action )
      totem->pulse_action->execute();

    totem->pulse_event = make_event<totem_pulse_event_t<T>>( sim(), *totem, totem->pulse_amplitude );
  }
};

template <typename T>
void shaman_totem_pet_t<T>::summon( timespan_t duration )
{
  pet_t::summon( duration );

  if ( this->pulse_action )
  {
    this->pulse_action->reset_pulse();
    this->pulse_event = make_event<totem_pulse_event_t<T>>( *sim, *this, this->pulse_amplitude );
  }

  if ( this->summon_pet )
    this->summon_pet->summon();
}

template <typename T>
void shaman_totem_pet_t<T>::dismiss( bool expired )
{
  if ( pulse_action && pulse_event && expired && pulse_on_expire )
  {
    auto e = debug_cast<totem_pulse_event_t<T>*>( pulse_event );
    if ( pulse_event->remains() > timespan_t::zero() && pulse_event->remains() != e->real_amplitude )
    {
      pulse_action->pulse_multiplier = ( e->real_amplitude - pulse_event->remains() ) / e->real_amplitude;
    }
    pulse_action->execute();
  }

  event_t::cancel( pulse_event );

  if ( summon_pet )
  {
    summon_pet->dismiss();
  }

  pet_t::dismiss( expired );
}

// Liquid Magma totem =======================================================

struct magma_eruption_t : public shaman_spell_t
{
  magma_eruption_t( shaman_t* p ) :
    shaman_spell_t( "magma_eruption", p, p->find_spell( 383061 ) )
  {
    aoe = -1;
    background = true;
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );
       
  }

  void execute() override
  {
    shaman_spell_t::execute();
    std::vector<player_t*> tl( target_cache.list );
    auto it = std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* target ) {
      return p()->get_target_data( target )->dot.flame_shock->is_ticking();
    } );
    tl.erase( it, tl.end() );

    if ( tl.size() < 3 )
    {
      // TODO: make more clever if ingame behaviour improves too.
      for ( size_t i = 0; i < std::min( target_list().size(), as<size_t>( data().effectN( 2 ).base_value() ) ); ++i )
      {
        p()->trigger_secondary_flame_shock( target_list()[ i ] );
      }
    }
    else
    {
      for ( size_t i = 0; i < std::min( tl.size(), as<size_t>( data().effectN( 2 ).base_value() ) ); ++i )
      {
        p()->trigger_secondary_flame_shock( tl[ i ] );
      }
    }
  }
};

struct liquid_magma_globule_t : public spell_totem_action_t
{
  liquid_magma_globule_t( spell_totem_pet_t* p ) :
    spell_totem_action_t( "liquid_magma", p, p->find_spell( 192231 ) )
  {
    aoe        = -1;
    background = may_crit = true;
    callbacks             = false;
  }
};

struct liquid_magma_totem_pulse_t : public spell_totem_action_t
{
  liquid_magma_globule_t* globule;

  liquid_magma_totem_pulse_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "liquid_magma_driver", totem, totem->find_spell( 192226 ) ),
      globule( new liquid_magma_globule_t( totem ) )
  {
    // TODO: "Random enemies" implicates number of targets
    aoe          = 1;
    hasted_pulse = quiet = dual = true;
    dot_duration                = timespan_t::zero();
  }

  void impact( action_state_t* state ) override
  {
    spell_totem_action_t::impact( state );

    globule->execute_on_target( state->target );
  }
};

struct liquid_magma_totem_t : public spell_totem_pet_t
{
  liquid_magma_totem_t( shaman_t* owner ) : spell_totem_pet_t( owner, "liquid_magma_totem" )
  {
    pulse_amplitude = owner->find_spell( 192226 )->effectN( 1 ).period();
    npc_id = 97369;
  }

  void init_spells() override
  {
    spell_totem_pet_t::init_spells();

    pulse_action = new liquid_magma_totem_pulse_t( this );
  }
};

struct liquid_magma_totem_spell_t : public shaman_totem_t<spell_totem_pet_t, shaman_spell_t>
{
  magma_eruption_t* eruption;

  liquid_magma_totem_spell_t( shaman_t* p, util::string_view options_str ) :
    shaman_totem_t<spell_totem_pet_t, shaman_spell_t>( "liquid_magma_totem", p, options_str,
        p->talent.liquid_magma_totem, p->pet.liquid_magma_totem ),
    eruption( new magma_eruption_t( p ) )
  {
    add_child( eruption );

    ancestor_trigger = ancestor_cast::CHAIN_LIGHTNING;
  }

  void execute() override
  {
    shaman_totem_t<spell_totem_pet_t, shaman_spell_t>::execute();

    eruption->execute_on_target( execute_state->target );
  }
};

// Capacitor Totem =========================================================

struct capacitor_totem_pulse_t : public spell_totem_action_t
{
  cooldown_t* totem_cooldown;

  capacitor_totem_pulse_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "static_charge", totem, totem->find_spell( 118905 ) )
  {
    aoe   = 1;
    quiet = dual   = true;
    totem_cooldown = totem->o()->get_cooldown( "capacitor_totem" );
  }

  void execute() override
  {
    spell_totem_action_t::execute();
    if ( totem->o()->talent.static_charge->ok() )
    {
      // This implementation assumes that every hit target counts. Ingame boss dummy testing showed that only
      // stunned targets count. TODO: check every hit target for whether it is stunned, or not.
      int cd_reduction = (int)( num_targets_hit * ( totem->o()->talent.static_charge->effectN( 1 ).base_value() ) );
      cd_reduction = -std::min( cd_reduction, as<int>( totem->o()->talent.static_charge->effectN( 2 ).base_value() ) );
      totem_cooldown->adjust( timespan_t::from_seconds( cd_reduction ) );
    }
  }
};

struct capacitor_totem_t : public spell_totem_pet_t
{
  capacitor_totem_t( shaman_t* owner ) : spell_totem_pet_t( owner, "capacitor_totem" )
  {
    pulse_amplitude = owner->find_spell( 192058 )->duration();
    npc_id = 199672;
  }

  void init_spells() override
  {
    spell_totem_pet_t::init_spells();

    pulse_action = new capacitor_totem_pulse_t( this );
  }
};

// Healing Stream Totem =====================================================

struct healing_stream_totem_pulse_t : public heal_totem_action_t
{
  healing_stream_totem_pulse_t( heal_totem_pet_t* totem )
    : heal_totem_action_t( "healing_stream_totem_heal", totem, totem->find_spell( 52042 ) )
  { }
};

struct healing_stream_totem_t : public heal_totem_pet_t
{
  healing_stream_totem_t( shaman_t* owner ) :
    heal_totem_pet_t( owner, "healing_stream_totem" )
  {
    pulse_amplitude = owner->find_spell( 5672 )->effectN( 1 ).period();
    npc_id = 3527;
  }

  void init_spells() override
  {
    heal_totem_pet_t::init_spells();

    pulse_action = new healing_stream_totem_pulse_t( this );
  }
};

struct healing_stream_totem_spell_t : public shaman_totem_t<heal_totem_pet_t, shaman_heal_t>
{
  healing_stream_totem_spell_t( shaman_t* p, util::string_view options_str ) :
    shaman_totem_t<heal_totem_pet_t, shaman_heal_t>( "healing_stream_totem", p, options_str,
        p->find_spell( 5394 ), p->pet.healing_stream_totem )
  { }

  void execute() override
  {
    shaman_totem_t<heal_totem_pet_t, shaman_heal_t>::execute();

    if ( p()->spec.inundate->ok() )
    {
      p()->trigger_maelstrom_gain( p()->spell.inundate->effectN( 1 ).base_value(), p()->gain.inundate );
    }
  }
};

// Surging Totem ============================================================

struct surging_totem_pulse_t : public spell_totem_action_t
{
  bool sundered;

  surging_totem_pulse_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "tremor", totem, totem->find_spell( 455622 ) ), sundered( false )
  {
    aoe          = -1;
    reduced_aoe_targets = as<double>( data().effectN( 2 ).base_value() );
    hasted_pulse = true;
  }

  void trigger_earthsurge()
  {
    sundered = true;
    execute();
    sundered = false;
  }

  void init() override
  {
    spell_totem_action_t::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( totem->o()->pet_list,
          [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != totem->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }

  double action_multiplier() const override
  {
    double m = spell_totem_action_t::action_multiplier();

    if ( o()->buff.ascendance->up() && o()->talent.oversurge.ok() )
    {
      m *= 1.0 + o()->talent.oversurge->effectN( 1 ).percent();
    }

    // Hardcoded in tooltip
    if ( sundered )
    {
      m *= 3.0;
    }

    return m;
  }

  void reset() override
  {
    spell_totem_action_t::reset();

    sundered = false;
  }
};

struct surging_bolt_t : public spell_totem_action_t
{
  surging_bolt_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "surging_bolt", totem, totem->find_spell( 458267 ) )
  {
    background = true;
  }
};

struct surging_totem_t : public spell_totem_pet_t
{
  surging_bolt_t* surging_bolt;

  surging_totem_t( shaman_t* owner ) : spell_totem_pet_t( owner, "surging_totem" ), surging_bolt( nullptr )
  {
    pulse_amplitude = owner->find_spell(
      owner->specialization() == SHAMAN_ENHANCEMENT ? 455593 : 45594 )->effectN( 1 ).period();
    npc_id = 225409;
  }

  void trigger_surging_bolt( player_t* target )
  {
    if ( surging_bolt )
    {
      surging_bolt->execute_on_target( target );
    }
  }

  void init_spells() override
  {
    spell_totem_pet_t::init_spells();

    pulse_action = new surging_totem_pulse_t( this );

    if ( o()->talent.totemic_rebound.ok() )
    {
      surging_bolt = new surging_bolt_t( this );
    }
  }

  void summon( timespan_t duration ) override
  {
    spell_totem_pet_t::summon( duration );

    pulse_action->execute_on_target( target );
  }

  void demise() override
  {
    spell_totem_pet_t::demise();

    o()->buff.whirling_air->expire();
    o()->buff.whirling_fire->expire();
    o()->buff.whirling_earth->expire();
    o()->buff.totemic_rebound->expire();
  }
};

struct surging_totem_spell_t : public shaman_totem_t<spell_totem_pet_t, shaman_spell_t>
{
  surging_totem_spell_t( shaman_t* p, util::string_view options_str ) :
    shaman_totem_t<spell_totem_pet_t, shaman_spell_t>( "surging_totem", p, options_str,
        p->find_spell( 444995 ), p->pet.surging_totem )
  { }

  void execute() override
  {
    shaman_totem_t<spell_totem_pet_t, shaman_spell_t>::execute();

    p()->buff.amplification_core->trigger();
    p()->buff.whirling_air->trigger();
    p()->buff.whirling_fire->trigger();
    p()->buff.whirling_earth->trigger();
  }

  bool ready() override
  {
    if ( !p()->talent.surging_totem.ok() )
    {
      return false;
    }

    return shaman_totem_t<spell_totem_pet_t, shaman_spell_t>::ready();
  }
};

// Searing Totem ============================================================

struct searing_totem_pulse_t : public spell_totem_action_t
{
  searing_totem_pulse_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "searing_bolt", totem, totem->find_spell( 3606 ) )
  {
    //hasted_pulse = true;
  }

  void init() override
  {
    spell_totem_action_t::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( totem->o()->pet_list,
          [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != totem->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }
};

struct searing_volley_t : public spell_totem_action_t
{
  searing_volley_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "searing_volley", totem, totem->find_spell( 458147 ) )
  { }

  void init() override
  {
    spell_totem_action_t::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( totem->o()->pet_list,
          [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != totem->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }
};

struct searing_totem_t : public spell_totem_pet_t
{
  searing_volley_t* volley;

  searing_totem_t( shaman_t* owner ) : spell_totem_pet_t( owner, "searing_totem" ), volley( nullptr )
  {
    pulse_amplitude = owner->find_spell( 3606 )->cast_time();
    npc_id = 2523;
  }

  void init_spells() override
  {
    spell_totem_pet_t::init_spells();

    pulse_action = new searing_totem_pulse_t( this );
  }

  void create_actions() override
  {
    spell_totem_pet_t::create_actions();

    if ( o()->talent.reactivity.ok() )
    {
      volley = new searing_volley_t( this );
    }
  }
};


// ==========================================================================
// PvP talents/abilities
// ==========================================================================

 struct lightning_lasso_t : public shaman_spell_t
{
  lightning_lasso_t( shaman_t* player, util::string_view options_str )
    : shaman_spell_t( "lightning_lasso", player, player->find_spell( 305485 ) )
  {
    parse_options( options_str );
    affected_by_master_of_the_elements = true;
    cooldown->duration                 = p()->find_spell( 305483 )->cooldown();
    trigger_gcd                        = p()->find_spell( 305483 )->gcd();
    channeled                          = true;
    tick_may_crit                      = true;
  }

  bool ready() override
  {
    if ( !p()->talent.lightning_lasso.ok() )
    {
      return false;
    }
    return shaman_spell_t::ready();
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );
    if ( p()->buff.master_of_the_elements->up() )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->default_value;
    }
    return m;
  }
};

struct thundercharge_t : public shaman_spell_t
{
  thundercharge_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "thundercharge", player, player->find_spell( 204366 ) )
  {
    parse_options( options_str );
    background = true;
    harmful    = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.thundercharge->trigger();
  }
};

// ==========================================================================
// Primordial Wave
// ==========================================================================

struct primordial_wave_t : public shaman_spell_t
{
  struct primordial_wave_damage_t : public shaman_spell_t
  {
    primordial_wave_damage_t( shaman_t* player ) :
      shaman_spell_t( "primordial_wave_damage", player, player->find_spell( 375984 )  )
    {
      background = true;
    }

    void impact( action_state_t* s ) override
    {
      shaman_spell_t::impact( s );

      p()->buff.primordial_wave->trigger();

      p()->trigger_secondary_flame_shock( s );
    }
  };

  primordial_wave_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "primordial_wave", player, player->talent.primordial_wave )
  {
    parse_options( options_str );

    impact_action = new primordial_wave_damage_t( player );
    add_child( impact_action );

    ancestor_trigger = ancestor_cast::LAVA_BURST;
  }

  void init() override
  {
    shaman_spell_t::init();

    // Spell data claims Maelstrom Weapon (still) affects Primordia Wave, however in-game
    // this is not true
    affected_by_maelstrom_weapon = false;
  }

  void execute() override
  {
    // Primordial Wave that summons an Ancestor will trigger a Lava Burst
    p()->summon_ancestor();

    shaman_spell_t::execute();

    if ( p()->talent.primal_maelstrom.ok() )
    {
      p()->generate_maelstrom_weapon( execute_state,
                                      as<int>( p()->talent.primal_maelstrom->effectN( 1 ).base_value() ) );
    }

    if ( p()->sets->has_set_bonus( SHAMAN_ENHANCEMENT, T31, B2 ) )
    {
      p()->action.feral_spirit_t31->set_target( execute_state->target );
      p()->action.feral_spirit_t31->execute();
    }
  }
};

// ==========================================================================
// Tempest
// ==========================================================================

struct tempest_overload_t : public elemental_overload_spell_t
{
  tempest_overload_t( shaman_t* p, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, "tempest_overload", p->find_spell( 463351 ), parent_ )
  {
    aoe = -1;
    // Blizzard forgot to apply Tempest's AOE soft cap hotfix to its overload spell
    // reduced_aoe_targets = as<double>( data().effectN( 3 ).base_value() );
    base_aoe_multiplier = data().effectN( 2 ).percent();
  }
};

struct tempest_t : public shaman_spell_t
{
  tempest_t( shaman_t* player, spell_variant type_, util::string_view options_str = {} ) :
    shaman_spell_t( ::action_name( "tempest", type_ ), player, player->find_spell( 452201 ), type_ )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = as<double>( data().effectN( 3 ).base_value() );
    base_aoe_multiplier = data().effectN( 2 ).percent();

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new tempest_overload_t( player, this );
    }

    switch ( exec_type )
    {
      case spell_variant::PRIMORDIAL_WAVE:
      {
        background = true;
        base_execute_time = 0_s;
        base_costs[ RESOURCE_MANA ] = 0;
        if ( auto pw_action = p()->find_action( "primordial_wave" ) )
        {
          pw_action->add_child( this );
        }
        break;
      }
      case spell_variant::THORIMS_INVOCATION:
      {
        background = true;
        base_execute_time = 0_s;
        base_costs[ RESOURCE_MANA ] = 0;
        if ( auto ws_action = p()->find_action( "windstrike" ) )
        {
          ws_action->add_child( this );
        }
      }
      default:
        break;
    }
  }

  void execute() override
  {
    p()->buff.tempest->expire();

    // PW needs to execute before the primary spell executes so we can retain proper
    // Maelstrom Weapon stacks for the AoE Lightning Bolt
    if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->trigger_primordial_wave_damage( this );
    }

    shaman_spell_t::execute();

    p()->trigger_static_accumulation_refund( execute_state, mw_consumed_stacks );

    if ( exec_type == spell_variant::NORMAL &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( execute_state->action,
                                      as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }

    if ( p()->talent.storm_swell.ok() && execute_state->n_targets == 1 )
    {
      if ( p()->specialization() == SHAMAN_ENHANCEMENT )
      {
        p()->generate_maelstrom_weapon( this, as<int>( p()->talent.storm_swell->effectN( 1 ).base_value() ) );
      }
      else
      {
        p()->trigger_maelstrom_gain( p()->spell.storm_swell->effectN( 1 ).base_value(),
                                     p()->gain.storm_swell );
      }
    }

    if ( p()->talent.arc_discharge.ok() && execute_state->n_targets >= 2 )
    {
      p()->buff.arc_discharge->trigger( p()->buff.arc_discharge->max_stack() );
    }

    if ( p()->talent.rolling_thunder.ok() && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->action.feral_spirit_rt->set_target( execute_state->target );
      p()->action.feral_spirit_rt->execute();
    }

    if ( p()->talent.thorims_invocation.ok() && exec_type == spell_variant::NORMAL )
    {
      if ( execute_state->n_targets == 1 )
      {
        p()->action.ti_trigger = p()->action.lightning_bolt_ti;
      }
      else if ( execute_state->n_targets > 1 )
      {
        p()->action.ti_trigger = p()->action.chain_lightning_ti;
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( ( p()->specialization() == SHAMAN_ENHANCEMENT && p()->talent.conductive_energy.ok() ) ||
         ( p()->specialization() == SHAMAN_ELEMENTAL && p()->talent.conductive_energy.ok() && !p()->bugs ) )
    {
      accumulate_lightning_rod_damage( state );
    }

    if ( state->chain_target == 0 &&
         ( ( p()->specialization() == SHAMAN_ENHANCEMENT && p()->talent.conductive_energy.ok() ) ||
         ( p()->specialization() == SHAMAN_ELEMENTAL && p()->talent.conductive_energy.ok() &&
           p()->talent.lightning_rod.ok() ) ) )
    {
      trigger_lightning_rod_debuff( state->target );
    }
  }

  bool ready() override
  {
    if ( !p()->buff.tempest->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Shaman Custom Buff implementation
// ==========================================================================

void ascendance_buff_t::ascendance( attack_t* mh, attack_t* oh )
{
  // Presume that ascendance trigger and expiration will not reset the swing
  // timer, so we need to cancel and reschedule autoattack with the
  // remaining swing time of main/off hands
  if ( player->specialization() == SHAMAN_ENHANCEMENT )
  {
    bool executing         = false;
    timespan_t time_to_hit = timespan_t::zero();
    if ( player->main_hand_attack && player->main_hand_attack->execute_event )
    {
      executing   = true;
      time_to_hit = player->main_hand_attack->execute_event->remains();
#ifndef NDEBUG
      if ( time_to_hit < timespan_t::zero() )
      {
        fmt::print( stderr, "Ascendance {} time_to_hit={}", player->main_hand_attack->name(), time_to_hit );
        assert( 0 );
      }
#endif
      event_t::cancel( player->main_hand_attack->execute_event );
    }

    if ( sim->debug )
    {
      sim->out_debug.print( "{} ascendance swing timer for main-hand, executing={}, time_to_hit={}",
                            player->name(), executing, time_to_hit );
    }

    player->main_hand_attack = mh;
    if ( executing )
    {
      // Kick off the new main hand attack, by instantly scheduling
      // and rescheduling it to the remaining time to hit. We cannot use
      // normal reschedule mechanism here (i.e., simply use
      // event_t::reschedule() and leave it be), because the rescheduled
      // event would be triggered before the full swing time (of the new
      // auto attack) in most cases.
      player->main_hand_attack->base_execute_time = timespan_t::zero();
      player->main_hand_attack->schedule_execute();
      player->main_hand_attack->base_execute_time = player->main_hand_attack->weapon->swing_time;
      if ( player->main_hand_attack->execute_event )
      {
        player->main_hand_attack->execute_event->reschedule( time_to_hit );
      }
    }

    if ( player->off_hand_attack )
    {
      time_to_hit = timespan_t::zero();
      executing   = false;

      if ( player->off_hand_attack->execute_event )
      {
        executing   = true;
        time_to_hit = player->off_hand_attack->execute_event->remains();
#ifndef NDEBUG
        if ( time_to_hit < timespan_t::zero() )
        {
          fmt::print( stderr, "Ascendance {} time_to_hit={}", player->off_hand_attack->name(), time_to_hit );
          assert( 0 );
        }
#endif
        event_t::cancel( player->off_hand_attack->execute_event );
      }

      if ( sim->debug )
      {
        sim->out_debug.print( "{} ascendance swing timer for off-hand, executing={}, time_to_hit={}",
                              player->name(), executing, time_to_hit );
      }

      player->off_hand_attack = oh;
      if ( executing )
      {
        // Kick off the new off hand attack, by instantly scheduling
        // and rescheduling it to the remaining time to hit. We cannot use
        // normal reschedule mechanism here (i.e., simply use
        // event_t::reschedule() and leave it be), because the rescheduled
        // event would be triggered before the full swing time (of the new
        // auto attack) in most cases.
        player->off_hand_attack->base_execute_time = timespan_t::zero();
        player->off_hand_attack->schedule_execute();
        player->off_hand_attack->base_execute_time = player->off_hand_attack->weapon->swing_time;
        if ( player->off_hand_attack->execute_event )
        {
          player->off_hand_attack->execute_event->reschedule( time_to_hit );
        }
      }
    }
  }
  // Elemental simply resets the Lava Burst cooldown, Lava Beam replacement
  // will be handled by action list and ready() in Chain Lightning / Lava
  // Beam
  else if ( player->specialization() == SHAMAN_ELEMENTAL )
  {
    if ( lava_burst )
    {
      lava_burst->cooldown->reset( false );
    }
  }
}

inline bool ascendance_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  shaman_t* p = debug_cast<shaman_t*>( player );

  if ( player->specialization() == SHAMAN_ELEMENTAL && !lava_burst )
  {
    lava_burst = player->find_action( "lava_burst" );
  }

  ascendance( p->ascendance_mh, p->ascendance_oh );
  // Don't record CD waste during Ascendance.
  if ( lava_burst )
  {
    lava_burst->cooldown->last_charged = timespan_t::zero();
  }

  return buff_t::trigger( stacks, value, chance, duration );
}

inline void ascendance_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  shaman_t* p = debug_cast<shaman_t*>( player );

  ascendance( p->melee_mh, p->melee_oh );

  // Start CD waste recollection from when Ascendance buff fades, since Lava
  // Burst is guaranteed to be very much ready when Ascendance ends.
  if ( lava_burst )
  {
    lava_burst->cooldown->last_charged = sim->current_time();
  }
  buff_t::expire_override( expiration_stacks, remaining_duration );
}

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::trigger_secondary_ability ======================================

void shaman_t::trigger_secondary_ability( const action_state_t* source_state, action_t* secondary_action,
                                          bool inherit_state )
{
  auto secondary_state = secondary_action->get_state( inherit_state ? source_state : nullptr );
  // Snapshot the state if no inheritance is defined
  if ( !inherit_state )
  {
    secondary_state->target = source_state->target;
    secondary_action->snapshot_state( secondary_state, secondary_action->amount_type( secondary_state ) );
  }

  secondary_action->schedule_execute( secondary_state );
}

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( util::string_view name, util::string_view options_str )
{
  // shared
  if ( name == "ascendance" )
    return new ascendance_t( this, "ascendance", options_str );
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "bloodlust" )
    return new bloodlust_t( this, options_str );
  if ( name == "capacitor_totem" )
    return new shaman_totem_t<spell_totem_pet_t, shaman_spell_t>( "capacitor_totem",
        this, options_str, talent.capacitor_totem, pet.capacitor_totem );
  if ( name == "elemental_blast" )
    return new elemental_blast_t( this, spell_variant::NORMAL, options_str );
  if ( name == "flame_shock" )
    return new flame_shock_t( this, options_str );
  if ( name == "frost_shock" )
    return new frost_shock_t( this, options_str );
  if ( name == "ghost_wolf" )
    return new ghost_wolf_t( this, options_str );
  if ( name == "lightning_bolt" )
    return new lightning_bolt_t( this, spell_variant::NORMAL, options_str );
  if ( name == "chain_lightning" )
    return new chain_lightning_t( this, spell_variant::NORMAL, options_str );
  if ( name == "stormkeeper" )
    return new stormkeeper_t( this, options_str );
  if ( name == "wind_shear" )
    return new wind_shear_t( this, options_str );
  if ( name == "healing_stream_totem" )
    return new healing_stream_totem_spell_t( this, options_str );
  if ( name == "earth_shield" )
    return new earth_shield_t( this, options_str );
  if ( name == "natures_swiftness" )
    return new natures_swiftness_t( this, options_str );
  if ( name == "totemic_recall" )
    return new totemic_recall_t( this, options_str );
  if ( name == "primordial_wave" )
    return new primordial_wave_t( this, options_str );
  if ( name == "tempest" )
    return new tempest_t( this, spell_variant::NORMAL, options_str );

  // elemental

  if ( name == "earth_elemental" )
    return new earth_elemental_t( this, options_str );
  if ( name == "earth_shock" )
    return new earth_shock_t( this, options_str );
  if ( name == "earthquake" )
    return new earthquake_t( this, options_str );
  if ( name == "fire_elemental" )
    return new fire_elemental_t( this, options_str );
  if ( name == "icefury" )
    return new icefury_t( this, options_str );
  if ( name == "lava_beam" )
    return new lava_beam_t( this, spell_variant::NORMAL, options_str );
  if ( name == "lava_burst" )
    return new lava_burst_t( this, spell_variant::NORMAL, options_str );
  if ( name == "liquid_magma_totem" )
    return new liquid_magma_totem_spell_t( this, options_str );
  if ( name == "ancestral_guidance" )
    return new ancestral_guidance_t( this, options_str );
  if ( name == "storm_elemental" )
    return new storm_elemental_t( this, options_str );
  if ( name == "thunderstorm" )
    return new thunderstorm_t( this, options_str );
  if ( name == "lightning_lasso" )
    return new lightning_lasso_t( this, options_str );

  // enhancement
  if ( name == "crash_lightning" )
    return new crash_lightning_t( this, options_str );
  if ( name == "feral_lunge" )
    return new feral_lunge_t( this, options_str );
  if ( name == "feral_spirit" )
    return new feral_spirit_spell_t( this, options_str );
  if ( name == "flametongue_weapon" )
    return new flametongue_weapon_t( this, options_str );
  if ( name == "windfury_weapon" )
    return new windfury_weapon_t( this, options_str );
  if ( name == "ice_strike" )
    return new ice_strike_t( this, options_str );
  if ( name == "lava_lash" )
    return new lava_lash_t( this, options_str );
  if ( name == "lightning_shield" )
    return new lightning_shield_t( this, options_str );
  if ( name == "spirit_walk" )
    return new spirit_walk_t( this, options_str );
  if ( name == "stormstrike" )
    return new stormstrike_t( this, options_str );
  if ( name == "sundering" )
    return new sundering_t( this, options_str );
  if ( name == "windstrike" )
    return new windstrike_t( this, options_str );
  if ( util::str_compare_ci( name, "thundercharge" ) )
    return new thundercharge_t( this, options_str );
  if ( name == "fire_nova" )
    return new fire_nova_t( this, options_str );
  if ( name == "doom_winds" )
    return new doom_winds_t( this, options_str );

  // restoration
  if ( name == "spiritwalkers_grace" )
    return new spiritwalkers_grace_t( this, options_str );
  if ( name == "chain_heal" )
    return new chain_heal_t( this, options_str );
  if ( name == "greater_healing_wave" )
    return new greater_healing_wave_t( this, options_str );
  if ( name == "healing_rain" )
    return new healing_rain_t( this, options_str );
  if ( name == "healing_surge" )
    return new healing_surge_t( this, options_str );
  if ( name == "healing_wave" )
    return new healing_wave_t( this, options_str );
  if ( name == "riptide" )
    return new riptide_t( this, options_str );

  // Hero talents
  if ( name == "surging_totem" )
    return new surging_totem_spell_t( this, options_str );
  if ( name == "thunderstrike_ward" )
    return new thunderstrike_ward_t( this, options_str );
  if ( name == "ancestral_swiftness" )
    return new ancestral_swiftness_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( util::string_view pet_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  return nullptr;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  player_t::create_pets();
}

// shaman_t::create_expression ==============================================

std::unique_ptr<expr_t> shaman_t::create_expression( util::string_view name )
{
  if ( util::str_compare_ci( name, "rolling_thunder.next_tick" ) )
  {
    return make_fn_expr( name, [ this ]() {
      return rt_last_trigger + timespan_t::from_seconds( talent.rolling_thunder->effectN( 1 ).base_value()) - sim->current_time();
    } );
  }

  if ( util::str_compare_ci( name, "dre_chance_pct" ) )
  {
    return make_fn_expr( name, [ this ]() {
      return 100.0 * std::max( 0.0, dre_attempts * 0.01 - 0.01 * options.dre_forced_failures );
    } );
  }

  auto splits = util::string_split<util::string_view>( name, "." );

  if ( util::str_compare_ci( splits[ 0 ], "feral_spirit" ) )
  {
    if ( !talent.feral_spirit.ok() )
    {
      return expr_t::create_constant( splits[ 0 ], 0 );
    }

    if ( ( talent.feral_spirit.ok() || talent.elemental_spirits->ok() ) && !find_action( "feral_spirit" ) )
    {
      return expr_t::create_constant( name, 0 );
    }

    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( name, [ this ]() {
        return as<double>( pet.all_wolves.size() );
      } );
    }
    else if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      auto max_remains_fn = []( const pet_t* l, const pet_t* r ) {
        if ( !l->expiration && r->expiration )
        {
          return true;
        }
        else if ( l->expiration && !r->expiration )
        {
          return false;
        }
        else if ( !l->expiration && !r->expiration )
        {
          return false;
        }
        else
        {
          return l->expiration->remains() < r->expiration->remains();
        }
      };

      return make_fn_expr( name, [ this, &max_remains_fn ]() {
        auto it = std::max_element( pet.all_wolves.cbegin(), pet.all_wolves.cend(), max_remains_fn );
        if ( it == pet.all_wolves.end() )
          {
            return 0.0;
          }

          return ( *it )->expiration ? ( *it )->expiration->remains().total_seconds() : 0.0;
      } );
    }
  }

  if ( util::str_compare_ci( splits[ 0 ], "ti_lightning_bolt" ) )
  {
    return make_fn_expr( name, [ this ]() {
        return !action.ti_trigger || action.ti_trigger == action.lightning_bolt_ti ||
               action.ti_trigger == action.tempest_ti;
    } );
  }

  if ( util::str_compare_ci( splits[ 0 ], "ti_chain_lightning" ) )
  {
    return make_fn_expr( name, [ this ]() {
        return action.ti_trigger == action.chain_lightning_ti;
    } );
  }

  if ( util::str_compare_ci( splits[ 0 ], "alpha_wolf_min_remains" ) )
  {
    if ( talent.alpha_wolf.ok() )
    {
      return make_fn_expr( name, [ this ]() {
        if ( pet.all_wolves.empty() )
        {
          return 0_ms;
        }

        auto it = std::min_element( pet.all_wolves.begin(), pet.all_wolves.end(),
          []( const pet::base_wolf_t* l, const pet::base_wolf_t* r ) {
            return l->alpha_wolf_buff->remains() < r->alpha_wolf_buff->remains();
        } );

        return ( *it )->alpha_wolf_buff->remains();
      } );
    }
    else
    {
      return expr_t::create_constant( splits[ 0 ], 0 );
    }
  }

  if ( util::str_compare_ci( splits[ 0 ], "rotation" ) )
  {
    auto rotation_type = parse_rotation( splits[ 1 ] );
    if ( rotation_type == ROTATION_INVALID )
    {
      throw std::invalid_argument( fmt::format( "Invalid rotation type {}, available values: {}",
                                               splits[ 1 ], rotation_options() ) );
    }

    return expr_t::create_constant( name, rotation_type == options.rotation );
  }

  if ( util::str_compare_ci( splits[ 0 ], "tempest_mael_count" ) )
  {
    return make_ref_expr( splits[ 0 ], tempest_counter );
  }

  if ( util::str_compare_ci( splits[ 0 ], "windfury_chance" ) )
  {
    return make_fn_expr( splits[ 0 ], [ this ]() {
      return std::min( 1.0, windfury_proc_chance() );
    } );
  }

  if ( util::str_compare_ci( splits[ 0 ], "lashing_flames" ) )
  {
    return make_ref_expr( splits[ 0 ], buff_state_lashing_flames );
  }

  if ( util::str_compare_ci( splits[ 0 ], "lightning_rod" ) )
  {
    return make_ref_expr( splits[ 0 ], buff_state_lightning_rod );
  }

  return player_t::create_expression( name );
}

// shaman_t::create_actions =================================================

void shaman_t::create_actions()
{
  player_t::create_actions();

  if ( talent.crash_lightning->ok() )
  {
    action.crash_lightning_aoe = new crash_lightning_attack_t( this );
  }

  // Collect Primordial Wave Lava burst stats separately
  if ( specialization() == SHAMAN_ENHANCEMENT && talent.primordial_wave.ok() )
  {
    action.lightning_bolt_pw = new lightning_bolt_t( this, spell_variant::PRIMORDIAL_WAVE );
  }

  if ( specialization() == SHAMAN_ELEMENTAL && talent.primordial_wave.ok() )
  {
    action.lava_burst_pw = new lava_burst_t( this, spell_variant::PRIMORDIAL_WAVE );
  }

  if ( talent.thorims_invocation.ok() )
  {
    action.lightning_bolt_ti = new lightning_bolt_t( this, spell_variant::THORIMS_INVOCATION );
    action.tempest_ti = new tempest_t( this, spell_variant::THORIMS_INVOCATION );
    action.chain_lightning_ti = new chain_lightning_t( this, spell_variant::THORIMS_INVOCATION );
  }

  if ( talent.lightning_rod.ok() || talent.conductive_energy.ok() )
  {
    action.lightning_rod = new lightning_rod_damage_t( this );
  }

  if ( talent.deeply_rooted_elements.ok() )
  {
    action.dre_ascendance = new ascendance_dre_t( this );
  }

  if ( talent.tempest_strikes.ok() )
  {
    action.tempest_strikes = new tempest_strikes_damage_t( this );
  }

  if ( talent.stormflurry.ok() )
  {
    action.stormflurry_ss = new stormstrike_t( this, "", strike_variant::STORMFLURRY );
    action.stormflurry_ws = new windstrike_t( this, "", strike_variant::STORMFLURRY );
  }

  if ( sets->has_set_bonus( SHAMAN_ENHANCEMENT, T28, B2 ) )
  {
    action.feral_spirit_t28 = new feral_spirit_spell_t( this, "", feral_spirit_cast::TIER28 );
  }

  if ( sets->has_set_bonus( SHAMAN_ENHANCEMENT, T31, B2 ) )
  {
    action.feral_spirit_t31 = new feral_spirit_spell_t( this, "", feral_spirit_cast::TIER31 );
  }

  if ( talent.rolling_thunder.ok() && specialization() == SHAMAN_ENHANCEMENT )
  {
    action.feral_spirit_rt = new feral_spirit_spell_t( this, "", feral_spirit_cast::ROLLING_THUNDER );
  }

  if ( talent.awakening_storms.ok() )
  {
    action.awakening_storms = new awakening_storms_t( this );
  }

  if ( talent.whirling_elements.ok() )
  {
    action.whirling_air_ss = new stormstrike_t( this, "", strike_variant::WHIRLING_AIR );
    action.whirling_air_ws = new windstrike_t( this, "", strike_variant::WHIRLING_AIR );
  }

  if ( talent.fusion_of_elements.ok() )
  {
    action.elemental_blast_foe = new elemental_blast_t( this, spell_variant::FUSION_OF_ELEMENTS );
  }

  if ( talent.thunderstrike_ward.ok() )
  {
    action.thunderstrike_ward = new thunderstrike_ward_damage_t( this );
  }

  if ( talent.earthen_rage.ok() )
  {
    action.earthen_rage = new earthen_rage_damage_t( this );
  }

  // Generic Actions
  action.flame_shock = new flame_shock_t( this );
  action.flame_shock->background = true;
  action.flame_shock->cooldown = get_cooldown( "flame_shock_secondary" );
  action.flame_shock->base_costs[ RESOURCE_MANA ] = 0;
}

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();
  add_option( opt_bool( "raptor_glyph", raptor_glyph ) );
  // option allows Shamans to switch to a different APL
  add_option( opt_func( "rotation", [ this ]( sim_t*, util::string_view, util::string_view val ) {
    options.rotation = parse_rotation( val );
    if ( options.rotation == ROTATION_INVALID )
    {
      throw std::invalid_argument( fmt::format( "Available options: {}.", rotation_options() ) );
    }

    return true;
  } ) );
  add_option( opt_obsoleted( "shaman.chain_harvest_allies" ) );
  add_option( opt_int( "shaman.dre_flat_chance", options.dre_flat_chance, -1, 1 ) );
  add_option( opt_uint( "shaman.dre_forced_failures", options.dre_forced_failures, 0U, 10U ) );

  add_option( opt_uint( "shaman.icefury_positive", options.icefury_positive, 0U, 100U ) );
  add_option( opt_uint( "shaman.icefury_total", options.icefury_total , 0U, 100U ) );

  add_option( opt_uint( "shaman.ancient_fellowship_positive", options.ancient_fellowship_positive, 0U, 100U ) );
  add_option( opt_uint( "shaman.ancient_fellowship_total", options.ancient_fellowship_total, 0U, 100U ) );

  add_option( opt_float( "shaman.thunderstrike_ward_proc_chance", options.thunderstrike_ward_proc_chance,
                         0.0, 1.0 ) );

  add_option( opt_float( "shaman.earthquake_spell_power_coefficient", options.earthquake_spell_power_coefficient, 0.0, 100.0 ) );
}

// shaman_t::create_profile ================================================

std::string shaman_t::create_profile( save_e save_type )
{
  std::string profile = player_t::create_profile( save_type );

  if ( save_type & SAVE_PLAYER )
  {
    if ( options.rotation == ROTATION_SIMPLE )
      profile += "rotation=simple\n";
  }

  return profile;
}

// shaman_t::copy_from =====================================================

void shaman_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  shaman_t* p  = debug_cast<shaman_t*>( source );
  raptor_glyph = p->raptor_glyph;
  options.rotation = p->options.rotation;
  options.dre_flat_chance = p->options.dre_flat_chance;
  options.dre_forced_failures = p->options.dre_forced_failures;
}

// shaman_t::create_special_effects ========================================

struct maelstrom_weapon_cb_t : public dbc_proc_callback_t
{
  shaman_t* shaman;

  maelstrom_weapon_cb_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect ), shaman( debug_cast<shaman_t*>( effect.player ) )
  { }

  // Fully override trigger + execute behavior of the proc
  void trigger( action_t* /* a */, action_state_t* state ) override
  {
    auto override_state = shaman->get_mw_proc_state( state->action );
    assert( override_state != mw_proc_state::DEFAULT );

    if ( override_state == mw_proc_state::DISABLED )
    {
      return;
    }

    if ( shaman->buff.ghost_wolf->check() )
    {
      return;
    }

    double proc_chance = shaman->spec.maelstrom_weapon->proc_chance();
    proc_chance += shaman->buff.witch_doctors_ancestry->stack_value();

    auto triggered = rng().roll( proc_chance );

    if ( listener->sim->debug )
    {
      listener->sim->print_debug( "{} attempts to proc {} on {}: {:d}", listener->name(),
          effect, state->action->name(), triggered );
    }

    if ( triggered )
    {
      shaman->generate_maelstrom_weapon( state );
      //shaman->buff.maelstrom_weapon->increment();
    }
  }
};

void shaman_t::create_special_effects()
{
  player_t::create_special_effects();

  if ( spec.maelstrom_weapon->ok() )
  {
    auto mw_effect = new special_effect_t( this );
    mw_effect->spell_id = spec.maelstrom_weapon->id();
    mw_effect->proc_flags2_ = PF2_ALL_HIT;

    special_effects.push_back( mw_effect );

    new maelstrom_weapon_cb_t( *mw_effect );
  }
}

// shaman_t::action_init_finished ==========================================

void shaman_t::action_init_finished( action_t& action )
{
  // Always initialize Maelstrom Weapon proc state for the action
  set_mw_proc_state( action, mw_proc_state::DEFAULT );

  // Enable Maelstrom Weapon proccing for selected abilities, if they
  // fulfill the basic conditions for the proc
  if ( spec.maelstrom_weapon->ok() && action.callbacks &&
       get_mw_proc_state( action ) == mw_proc_state::DEFAULT && (
         // Auto-attacks (shaman-module convention to set mh to action id 1, oh to id 2)
         ( action.id == 1 || action.id == 2 ) ||
         // Actions with spell data associated
         ( action.data().id() != 0 &&
           !action.data().flags( spell_attribute::SX_SUPPRESS_CASTER_PROCS ) &&
           action.data().dmg_class() == 2U )
       ) )
  {
    set_mw_proc_state( action, mw_proc_state::ENABLED );
  }

  // Explicitly disable any action from proccing Maelstrom Weapon that does not have
  // it set enabled (above), or had its state adjusted to enabled or disabled during
  // action initialization.
  if ( get_mw_proc_state( action ) == mw_proc_state::DEFAULT )
  {
    set_mw_proc_state( action, mw_proc_state::DISABLED );
  }
}

void shaman_t::analyze( sim_t& sim )
{
  player_t::analyze( sim );

  int iterations = collected_data.total_iterations > 0
    ? collected_data.total_iterations
    : sim.iterations;

  if ( iterations > 1 )
  {
    // Re-use MW stack containers to report iteration average of stacks generated
    range::for_each( mw_source_list, [ iterations ]( auto& container ) {
      auto sum_actual = container.first.sum();
      auto sum_overflow = container.second.sum();

      container.first.reset();
      container.first.add( sum_actual / as<double>( iterations ) );

      container.second.reset();
      container.second.add( sum_overflow / as<double>( iterations ) );
    } );

    // Re-use MW spend containers to report iteration average over stacks consumed
    range::for_each( mw_spend_list, [ iterations ]( auto& container_wrapper ) {
      range::for_each( container_wrapper, [ idx = 0, iterations ]( auto& container ) mutable {
        auto sum = container.sum();
        auto count = container.count();

        container.reset();
        // 0-stack MW casts are just the count divided by iterations, not the sum
        if ( idx++ == 0 )
        {
          container.add( count / as<double>( iterations ) );
        }
        else
        {
          container.add( sum / as<double>( iterations ) );
        }
      } );
    } );
  }

  if ( talent.deeply_rooted_elements.ok() )
  {
    dre_samples.analyze();
    dre_samples.create_histogram( static_cast<unsigned>( dre_samples.max() - dre_samples.min() + 1 ) );
    dre_uptime_samples.analyze();
    dre_uptime_samples.create_histogram( static_cast<unsigned>( std::ceil( dre_uptime_samples.max() ) - std::floor( dre_uptime_samples.min() ) + 1 ) );
  }

  lvs_samples.analyze();
  lvs_samples.create_histogram( static_cast<unsigned>( lvs_samples.max() - lvs_samples.min() + 1 ) );
}

// shaman_t::datacollection_end ============================================

void shaman_t::datacollection_end()
{
  player_t::datacollection_end();

  if ( buff.ascendance->iteration_uptime() > 0_ms )
  {
    dre_uptime_samples.add( 100.0 * buff.ascendance->iteration_uptime() / iteration_fight_length );
  }
}

// shaman_t::init_spells ===================================================

void shaman_t::init_spells()
{
  //
  // Generic spells
  //
  spec.mail_specialization          = find_specialization_spell( "Mail Specialization" );
  spec.shaman                       = find_spell( 137038 );

  // Elemental
  spec.elemental_shaman  = find_specialization_spell( "Elemental Shaman" );
  spec.maelstrom         = find_specialization_spell( 343725 );
  spec.lava_surge        = find_specialization_spell( "Lava Surge" );
  spec.lightning_bolt_2  = find_rank_spell( "Lightning Bolt", "Rank 2" );
  spec.lava_burst_2      = find_rank_spell( "Lava Burst", "Rank 2" );
  spec.inundate          = find_specialization_spell( "Inundate" );

  // Enhancement
  spec.critical_strikes   = find_specialization_spell( "Critical Strikes" );
  spec.dual_wield         = find_specialization_spell( "Dual Wield" );
  spec.enhancement_shaman = find_specialization_spell( "Enhancement Shaman" );
  spec.stormbringer       = find_specialization_spell( "Stormbringer" );
  spec.maelstrom_weapon   = find_specialization_spell( "Maelstrom Weapon" );

  // Restoration
  spec.purification       = find_specialization_spell( "Purification" );
  spec.resurgence         = find_specialization_spell( "Resurgence" );
  spec.riptide            = find_specialization_spell( "Riptide" );
  spec.tidal_waves        = find_specialization_spell( "Tidal Waves" );
  spec.restoration_shaman = find_specialization_spell( "Restoration Shaman" );

  //
  // Masteries
  //
  mastery.elemental_overload = find_mastery_spell( SHAMAN_ELEMENTAL );
  mastery.enhanced_elements  = find_mastery_spell( SHAMAN_ENHANCEMENT );
  mastery.deep_healing       = find_mastery_spell( SHAMAN_RESTORATION );

  // Talents
  auto _CT = [this]( util::string_view name ) {
    return find_talent_spell( talent_tree::CLASS, name );
  };

  auto _ST = [this]( util::string_view name ) {
    return find_talent_spell( talent_tree::SPECIALIZATION, name );
  };

  // Class tree
  // Row 1
  talent.lava_burst      = _CT( "Lava Burst" );
  talent.chain_lightning = _CT( "Chain Lightning" );
  // Row 2
  talent.earth_elemental = _CT( "Earth Elemental" );
  talent.wind_shear      = _CT( "Wind Shear" );
  talent.spirit_wolf     = _CT( "Spirit Wolf" );
  talent.thunderous_paws = _CT( "Thunderous Paws" );
  talent.frost_shock     = _CT( "Frost Shock" );
  // Row 3
  talent.earth_shield     = _CT( "Earth Shield" );
  talent.fire_and_ice     = _CT( "Fire and Ice" );
  talent.capacitor_totem  = _CT( "Capacitor Totem" );
  // Row 4
  talent.spiritwalkers_grace = _CT( "Spiritwalker's Grace" );
  talent.static_charge       = _CT( "Static Charge" );
  talent.guardians_cudgel    = _CT( "Guardian's Cudgel" );
  // Row 5
  talent.graceful_spirit     = _CT( "Graceful Spirit" );
  talent.natures_fury        = _CT( "Nature's Fury" );
  // Row 6
  talent.totemic_surge       = _CT( "Totemic Surge" );
  talent.winds_of_alakir     = _CT( "Winds of Al'Akir" );
  // Row 7
  talent.healing_stream_totem    = _CT( "Healing Stream Totem" );
  talent.improved_lightning_bolt = _CT( "Improved Lightning Bolt" );
  talent.spirit_walk             = _CT( "Spirit Walk" );
  talent.gust_of_wind            = _CT( "Gust of Wind" );
  talent.enhanced_imbues         = _CT( "Enhanced Imbues" );
  // Row 8
  talent.natures_swiftness       = _CT( "Nature's Swiftness" );
  talent.thunderstorm            = _CT( "Thunderstorm" );
  talent.totemic_focus           = _CT( "Totemic Focus ");
  talent.surging_shields         = _CT( "Surging Shields" );
  talent.go_with_the_flow        = _CT( "Go With the Flow ");
  // Row 9
  talent.lightning_lasso         = _CT( "Lightning Lasso" );
  talent.thundershock            = _CT( "Thundershock" );
  talent.totemic_recall          = _CT( "Totemic Recall" );
  // Row 10
  talent.ancestral_guidance      = _CT( "Ancestral Guidance" );
  talent.creation_core           = _CT( "Creation Core" );
  talent.call_of_the_elements = _CT( "Call of the Elements" );

  // Spec - Shared
  talent.ancestral_wolf_affinity = _ST( "Ancestral Wolf Affinity" );
  talent.elemental_blast         = _ST( "Elemental Blast" );
  talent.primordial_wave         = _ST( "Primordial Wave" );
  talent.ascendance              = _ST( "Ascendance" );
  talent.deeply_rooted_elements  = _ST( "Deeply Rooted Elements" );
  talent.splintered_elements     = _ST( "Splintered Elements" );

  // Enhancement
  // Row 1
  talent.stormstrike = _ST( "Stormstrike" );
  // Row 2
  talent.windfury_weapon = _ST( "Windfury Weapon" );
  talent.lava_lash = find_talent_spell( talent_tree::SPECIALIZATION, 60103 );
  // Row 3
  talent.forceful_winds = _ST( "Forceful Winds" );
  talent.improved_maelstrom_weapon = _ST( "Improved Maelstrom Weapon" );
  talent.molten_assault = _ST( "Molten Assault" );
  // Row 4
  talent.unruly_winds = _ST( "Unruly Winds" );
  talent.raging_maelstrom = _ST( "Raging Maelstrom" );
  talent.lashing_flames = _ST( "Lashing Flames" );
  talent.ashen_catalyst = _ST( "Ashen Catalyst" );
  // Row 5
  talent.doom_winds = _ST( "Doom Winds" );
  talent.sundering = _ST( "Sundering" );
  talent.overflowing_maelstrom = _ST( "Overflowing Maelstrom" );
  talent.fire_nova = _ST( "Fire Nova" );
  talent.hailstorm = _ST( "Hailstorm" );
  talent.elemental_weapons = _ST( "Elemental Weapons" );
  talent.crashing_storms = _ST( "Crashing Storms" );
  talent.tempest_strikes = _ST( "Tempest strikes" );
  talent.flurry          = _ST( "Flurry" );
  // Row 6
  talent.storms_wrath = _ST( "Storm's Wrath" );
  talent.crash_lightning = _ST( "Crash Lightning" );
  talent.stormflurry = _ST( "Stormflurry" );
  talent.ice_strike = _ST( "Ice Strike" );
  // Row 7
  talent.stormblast = _ST( "Stormblast" );
  talent.converging_storms = _ST( "Converging Storms" );
  talent.hot_hand = _ST( "Hot Hand" );
  talent.swirling_maelstrom = _ST( "Swirling Maelstrom" );
  // Row 8
  talent.feral_spirit = _ST( "Feral Spirit" );
  // Row 9
  talent.primal_maelstrom = _ST( "Primal Maelstrom" );
  talent.elemental_assault = _ST( "Elemental Assault" );
  talent.witch_doctors_ancestry = _ST( "Witch Doctor's Ancestry" );
  talent.legacy_of_the_frost_witch = _ST( "Legacy of the Frost Witch" );
  talent.static_accumulation = _ST( "Static Accumulation" );
  // Row 10
  talent.alpha_wolf = _ST( "Alpha Wolf" );
  talent.elemental_spirits = _ST( "Elemental Spirits" );
  talent.thorims_invocation = _ST( "Thorim's Invocation" );

  // Elemental
  // Row 1
  talent.earth_shock = _ST( "Earth Shock" );
  // Row 2
  talent.earthquake_reticle = find_talent_spell( talent_tree::SPECIALIZATION, 61882 );
  talent.earthquake_target = find_talent_spell( talent_tree::SPECIALIZATION, 462620 );

  talent.elemental_fury = _ST( "Elemental Fury" );
  talent.fire_elemental = _ST( "Fire Elemental" );
  talent.storm_elemental = _ST( "Storm Elemental" );
  // Row 3
  talent.flash_of_lightning     = _ST( "Flash of Lightning" );
  talent.aftershock             = _ST( "Aftershock" );
  talent.surge_of_power         = _ST( "Surge of Power" );
  talent.echo_of_the_elements   = _ST( "Echo of the Elements" );
  // Row 4
  talent.icefury                = _ST( "Icefury" );
  talent.unrelenting_calamity   = _ST( "Unrelenting Calamity" );
  talent.master_of_the_elements = _ST( "Master of the Elements" );
  // Row 5
  talent.fusion_of_elements     = _ST( "Fusion of Elements" );
  talent.storm_frenzy           = _ST( "Storm Frenzy" );
  talent.swelling_maelstrom     = _ST( "Swelling Maelstrom" );
  talent.primordial_fury        = _ST( "Primordial Fury" );
  talent.flow_of_power          = _ST( "Flow of Power" );
  talent.elemental_unity        = _ST( "Elemental Unity" );
  // Row 6
  talent.flux_melting           = _ST( "Flux Melting" );
  talent.lightning_conduit      = _ST( "Lightning Conduit" );
  talent.power_of_the_maelstrom = _ST( "Power of the Maelstrom" );
  talent.improved_flametongue_weapon = _ST( "Improved Flametongue Weapon" );
  talent.everlasting_elements   = _ST( "Everlasting Elements" );
  talent.flames_of_the_cauldron = _ST( "Flames of the Cauldron" );
  // Row 7
  talent.eye_of_the_storm       = _ST( "Eye of the Storm" );
  talent.thunderstrike_ward     = _ST( "Thunderstrike Ward" );
  talent.echo_chamber           = _ST( "Echo Chamber" );
  talent.searing_flames         = _ST( "Searing Flames" );
  talent.earthen_rage           = _ST( "Earthen Rage" );
  // Row 8
  talent.elemental_equilibrium  = _ST( "Elemental Equilibrium" );
  talent.stormkeeper            = _ST( "Stormkeeper" );
  talent.echo_of_the_elementals = _ST( "Echo of the Elementals" );
  // Row 9
  talent.mountains_will_fall    = _ST( "Mountains Will Fall" );
  talent.first_ascendant        = _ST( "First Ascendant" );
  talent.preeminence            = _ST( "Preeminence" );
  talent.fury_of_the_storms     = _ST( "Fury of the Storms" );
  talent.skybreakers_fiery_demise = _ST( "Skybreaker's Fiery Demise" );
  talent.magma_chamber          = _ST( "Magma Chamber" );
  // Row 10
  talent.echoes_of_great_sundering = _ST( "Echoes of Great Sundering" );
  talent.lightning_rod          = _ST( "Lightning Rod" );
  talent.primal_elementalist    = _ST( "Primal Elementalist" );
  talent.liquid_magma_totem     = _ST( "Liquid Magma Totem" );

  // Stormbringer

  talent.tempest               = find_talent_spell( talent_tree::HERO, 454009 );

  talent.unlimited_power       = find_talent_spell( talent_tree::HERO, "Unlimited Power" );
  talent.stormcaller           = find_talent_spell( talent_tree::HERO, "Stormcaller" );
  //talent.shocking_grasp   = find_talent_spell( talent_tree::HERO, "Shocking Grasp" );

  talent.supercharge           = find_talent_spell( talent_tree::HERO, "Supercharge" );
  talent.storm_swell           = find_talent_spell( talent_tree::HERO, "Storm Swell" );
  talent.arc_discharge         = find_talent_spell( talent_tree::HERO, "Arc Discharge" );
  talent.rolling_thunder       = find_talent_spell( talent_tree::HERO, "Rolling Thunder" );

  talent.voltaic_surge         = find_talent_spell( talent_tree::HERO, "Voltaic Surge" );
  talent.conductive_energy     = find_talent_spell( talent_tree::HERO, "Conductive Energy" );
  talent.surging_currents      = find_talent_spell( talent_tree::HERO, "Surging Currents" );

  talent.awakening_storms      = find_talent_spell( talent_tree::HERO, "Awakening Storms" );

  // Totemic

  talent.surging_totem         = find_talent_spell( talent_tree::HERO, "Surging Totem" );

  talent.totemic_rebound       = find_talent_spell( talent_tree::HERO, "Totemic Rebound" );
  talent.amplification_core    = find_talent_spell( talent_tree::HERO, "Amplification Core" );
  talent.oversurge             = find_talent_spell( talent_tree::HERO, "Oversurge" );
  talent.lively_totems         = find_talent_spell( talent_tree::HERO, "Lively Totems" );

  talent.reactivity            = find_talent_spell( talent_tree::HERO, "Reactivity" );

  talent.imbuement_mastery     = find_talent_spell( talent_tree::HERO, "Imbuement Mastery" );
  talent.pulse_capacitor       = find_talent_spell( talent_tree::HERO, "Pulse Capacitor" );
  talent.supportive_imbuements = find_talent_spell( talent_tree::HERO, "Supportive Imbuements" );
  talent.totemic_coordination  = find_talent_spell( talent_tree::HERO, "Totemic Coordination" );
  talent.earthsurge            = find_talent_spell( talent_tree::HERO, "Earthsurge" );

  talent.whirling_elements     = find_talent_spell( talent_tree::HERO, "Whirling Elements" );

  // Farseer

  talent.call_of_the_ancestors = find_talent_spell( talent_tree::HERO, "Call of the Ancestors" );

  talent.latent_wisdom         = find_talent_spell( talent_tree::HERO, "Latent Wisdom" );
  talent.ancient_fellowship    = find_talent_spell( talent_tree::HERO, "Ancient Fellowship" );
  talent.heed_my_call          = find_talent_spell( talent_tree::HERO, "Heed My Call" );
  talent.routine_communication = find_talent_spell( talent_tree::HERO, "Routine Communication" );
  talent.elemental_reverb      = find_talent_spell( talent_tree::HERO, "Elemental Reverb" );

  talent.offering_from_beyond  = find_talent_spell( talent_tree::HERO, "Offering from Beyond" );
  talent.primordial_capacity   = find_talent_spell( talent_tree::HERO, "Primordial Capacity" );

  talent.maelstrom_supremacy   = find_talent_spell( talent_tree::HERO, "Maelstrom Supremacy" );
  talent.final_calling         = find_talent_spell( talent_tree::HERO, "Final Calling" );

  talent.ancestral_swiftness   = find_talent_spell( talent_tree::HERO, "Ancestral Swiftness" );

  //
  // Misc spells
  //
  spell.ascendance          = talent.ascendance.spell();
  if ( !spell.ascendance->ok() && talent.deeply_rooted_elements.ok() )
  {
    switch ( specialization() )
    {
      case SHAMAN_ELEMENTAL:   spell.ascendance = find_spell( 114050 ); break;
      case SHAMAN_ENHANCEMENT: spell.ascendance = find_spell( 114051 ); break;
      case SHAMAN_RESTORATION: spell.ascendance = find_spell( 114052 ); break;
      default:                 break;
    }
  }

  spell.resurgence          = find_spell( 101033 );
  spell.maelstrom_weapon    = find_spell( 187881 );
  spell.feral_spirit        = find_spell( 228562 );
  spell.fire_elemental      = find_spell( 188592 );
  spell.storm_elemental     = find_spell( 157299 );
  spell.earth_elemental     = find_spell( 188616 );
  spell.flametongue_weapon  = find_spell( 318038 );
  spell.windfury_weapon     = find_spell( 319773 );
  spell.inundate            = find_spell( 378777 );
  spell.storm_swell         = find_spell( 455089 );
  spell.lightning_rod       = find_spell( 210689 );
  spell.improved_flametongue_weapon = find_spell( 382028 );
  spell.earthen_rage        = find_spell( 170377 );

  spell.t28_2pc_enh        = sets->set( SHAMAN_ENHANCEMENT, T28, B2 );
  spell.t28_4pc_enh        = sets->set( SHAMAN_ENHANCEMENT, T28, B4 );

  // Misc spell-related init
  max_active_flame_shock   = as<unsigned>( find_class_spell( "Flame Shock" )->max_targets() );

  // Constants
  constant.mul_matching_gear = spec.mail_specialization->effectN( 1 ).percent();
  constant.mul_lightning_rod = find_spell( 210689 )->effectN( 2 ).percent() +
                               spec.enhancement_shaman->effectN( 27 ).percent();

  player_t::init_spells();
}

// shaman_t::init_base ======================================================

void shaman_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = ( specialization() == SHAMAN_ENHANCEMENT ) ? 5 : 30;

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  if ( specialization() == SHAMAN_ELEMENTAL )
  {
    resources.base[ RESOURCE_MAELSTROM ] = 100;
    resources.base[ RESOURCE_MAELSTROM ]+= talent.swelling_maelstrom->effectN( 1 ).base_value();
    resources.base[ RESOURCE_MAELSTROM ]+= talent.primordial_capacity->effectN( 1 ).base_value();
  }

  if ( specialization() == SHAMAN_RESTORATION )
  {
    resources.base[ RESOURCE_MANA ]               = 20000;
    resources.initial_multiplier[ RESOURCE_MANA ] = 1.0;
    resources.initial_multiplier[ RESOURCE_MANA ]+= spec.restoration_shaman->effectN( 5 ).percent();
    resources.initial_multiplier[ RESOURCE_MANA ]+= talent.primordial_capacity->effectN( 2 ).percent();
  }
}

// shaman_t::init_scaling ===================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
      scaling->enable( STAT_WEAPON_OFFHAND_DPS );
      scaling->disable( STAT_STRENGTH );
      scaling->disable( STAT_SPELL_POWER );
      scaling->disable( STAT_INTELLECT );
      break;
    case SHAMAN_RESTORATION:
      scaling->disable( STAT_MASTERY_RATING );
      break;
    default:
      break;
  }
}

// ==========================================================================
// Shaman Misc helpers
// ==========================================================================

bool shaman_t::is_elemental_pet_active() const
{
  return pet.fire_elemental.n_active_pets() || pet.lesser_fire_elemental.n_active_pets() ||
    pet.storm_elemental.n_active_pets() || pet.lesser_storm_elemental.n_active_pets();
}

pet_t* shaman_t::get_active_elemental_pet() const
{
  if ( talent.storm_elemental.ok() )
  {
    if ( pet.storm_elemental.n_active_pets() )
    {
      return pet.storm_elemental.active_pet();
    }
    else if ( pet.lesser_storm_elemental.n_active_pets() )
    {
      return pet.lesser_storm_elemental.active_pet();
    }
  }
  else
  {
    if ( pet.fire_elemental.n_active_pets() )
    {
      return pet.fire_elemental.active_pet();
    }
    else if ( pet.lesser_fire_elemental.n_active_pets() )
    {
      return pet.lesser_fire_elemental.active_pet();
    }
  }

  return nullptr;
}

void shaman_t::summon_elemental( elemental type, timespan_t override_duration )
{
  spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t>* spawner_ptr = nullptr;
  buff_t* elemental_buff = nullptr;

  switch ( type )
  {
    case elemental::GREATER_FIRE:
    case elemental::PRIMAL_FIRE:
    {
      elemental_buff = buff.fire_elemental;
      spawner_ptr = &( pet.fire_elemental );

      pet.earth_elemental.despawn();
      pet.storm_elemental.despawn();
      buff.storm_elemental->expire();
      break;
    }
    case elemental::GREATER_STORM:
    case elemental::PRIMAL_STORM:
    {
      elemental_buff = buff.storm_elemental;
      spawner_ptr = &( pet.storm_elemental );

      pet.earth_elemental.despawn();
      pet.fire_elemental.despawn();
      buff.fire_elemental->expire();
      break;
    }
    case elemental::GREATER_EARTH:
    case elemental::PRIMAL_EARTH:
    {
      elemental_buff = buff.earth_elemental;
      spawner_ptr = &( pet.earth_elemental );

      pet.storm_elemental.despawn();
      pet.fire_elemental.despawn();
      buff.fire_elemental->expire();
      buff.storm_elemental->expire();
      break;
    }
    default:
      assert( 0 );
      break;
  }

  if ( spawner_ptr->n_active_pets() > 0 )
  {
    timespan_t new_duration = spawner_ptr->active_pet()->expiration->remains();
    new_duration += override_duration > 0_ms ? override_duration : elemental_buff->buff_duration();

    elemental_buff->extend_duration( this,
      override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
    spawner_ptr->active_pet()->expiration->reschedule( new_duration );
  }
  else
  {
    elemental_buff->trigger( override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
    spawner_ptr->spawn( override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
  }
}

void shaman_t::trigger_elemental_blast_proc()
{
    ::trigger_elemental_blast_proc( this );
}

void shaman_t::summon_ancestor( double proc_chance )
{
  if ( !talent.call_of_the_ancestors.ok() )
  {
    return;
  }

  if ( !rng().roll( proc_chance ) )
  {
    return;
  }

  if ( talent.offering_from_beyond.ok() )
  {
    cooldown.fire_elemental->adjust( talent.offering_from_beyond->effectN( 1 ).time_value() );
    cooldown.storm_elemental->adjust( talent.offering_from_beyond->effectN( 1 ).time_value() );
  }

  pet.ancestor.spawn( buff.call_of_the_ancestors->buff_duration() );
  buff.call_of_the_ancestors->trigger();
}

void shaman_t::summon_lesser_elemental( elemental type, timespan_t override_duration )
{
  spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t>* spawner_ptr = nullptr;
  buff_t* elemental_buff = nullptr;

  switch ( type )
  {
    case elemental::GREATER_FIRE:
    case elemental::PRIMAL_FIRE:
    {
      elemental_buff = buff.lesser_fire_elemental;
      spawner_ptr = &( pet.lesser_fire_elemental );

      pet.lesser_storm_elemental.despawn();
      buff.lesser_storm_elemental->expire();
      break;
    }
    case elemental::GREATER_STORM:
    case elemental::PRIMAL_STORM:
    {
      elemental_buff = buff.lesser_storm_elemental;
      spawner_ptr = &( pet.lesser_storm_elemental );

      buff.wind_gust->expire();
      pet.lesser_fire_elemental.despawn();
      buff.lesser_fire_elemental->expire();
      break;
    }
    default:
      assert( 0 );
      break;
  }

  if ( spawner_ptr->n_active_pets() > 0 )
  {
    timespan_t new_duration = spawner_ptr->active_pet()->expiration->remains();
    new_duration += override_duration > 0_ms ? override_duration : elemental_buff->buff_duration();

    elemental_buff->extend_duration( this,
      override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
    spawner_ptr->active_pet()->expiration->reschedule( new_duration );
  }
  else
  {
    elemental_buff->trigger( override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
    spawner_ptr->spawn( override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
  }
}

// ==========================================================================
// Shaman Tracking - code blocks that shall not be doublicated
// ==========================================================================

void shaman_t::track_magma_chamber()
{
  if ( !talent.magma_chamber->ok() )
    return;

  int d = buff.magma_chamber->check();
  assert( d < as<int>( proc.magma_chamber.size() ) && "The procs.magma_chamber array needs to be expanded." );
  if ( d >= 0 && d < as<int>( proc.magma_chamber.size() ) )
  {
    proc.magma_chamber[ d ]->occur();
  }
}

// ==========================================================================
// Shaman Ability Triggers
// ==========================================================================

void shaman_t::trigger_stormbringer( const action_state_t* state, double override_proc_chance,
                                     proc_t* override_proc_obj )
{
  // assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr &&
  //        "Stormbringer called on invalid action type" );

  if ( buff.ghost_wolf->check() )
  {
    return;
  }

  if ( !state->action->special )
  {
    return;
  }

  shaman_attack_t* attack = nullptr;
  shaman_spell_t* spell   = nullptr;

  if ( state->action->type == ACTION_ATTACK )
  {
    attack = debug_cast<shaman_attack_t*>( state->action );
  }
  else if ( state->action->type == ACTION_SPELL )
  {
    spell = debug_cast<shaman_spell_t*>( state->action );
  }

  if ( attack )
  {
    if ( attack->may_proc_stormbringer )
    {
      result_e r = state->result;
      if ( r == RESULT_HIT || r == RESULT_CRIT || r == RESULT_GLANCE || r == RESULT_NONE )
      {
        if ( override_proc_chance < 0 )
        {
          override_proc_chance = attack->stormbringer_proc_chance();
        }

        if ( override_proc_obj == nullptr )
        {
          override_proc_obj = attack->proc_sb;
        }

        if ( rng().roll( override_proc_chance ) )
        {
          buff.stormbringer->trigger( buff.stormbringer->max_stack() );
          cooldown.strike->reset( true );
          override_proc_obj->occur();
        }
      }
    }
  }

  if ( spell )
  {
    if ( spell->may_proc_stormbringer )
    {
      if ( override_proc_chance < 0 )
      {
        override_proc_chance = spell->stormbringer_proc_chance();
      }

      if ( override_proc_obj == nullptr )
      {
        override_proc_obj = spell->proc_sb;
      }

      if ( rng().roll( override_proc_chance ) )
      {
        buff.stormbringer->trigger( buff.stormbringer->max_stack() );
        cooldown.strike->reset( true );
        override_proc_obj->occur();
      }
    }
  }
}

void shaman_t::trigger_hot_hand( const action_state_t* state )
{
  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr && "Hot Hand called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );

  if ( !attack->may_proc_hot_hand )
  {
    return;
  }

  if ( main_hand_weapon.buff_type != FLAMETONGUE_IMBUE &&
       off_hand_weapon.buff_type != FLAMETONGUE_IMBUE )
  {
    return;
  }

  if ( buff.hot_hand->trigger() )
  {
    attack->proc_hh->occur();
    cooldown.lava_lash->reset(true);
  }
}

void shaman_t::trigger_legacy_of_the_frost_witch( const action_state_t* /* state */,
                                                  unsigned consumed_stacks )
{
  if ( !talent.legacy_of_the_frost_witch.ok() )
  {
    return;
  }

  lotfw_counter += consumed_stacks;

  auto threshold = as<unsigned>( talent.legacy_of_the_frost_witch->effectN( 2 ).base_value() );

  if ( lotfw_counter >= threshold )
  {
    lotfw_counter -= threshold;
    buff.legacy_of_the_frost_witch->trigger();
    cooldown.strike->reset( false );
  }
}

void shaman_t::trigger_elemental_equilibrium( const action_state_t* state )
{
  // Apparently Flametongue cannot proc Elemental Equilibrium, but pretty much everything
  // else (including consumables and trinkets) can.
  if ( state->action->id == 10444 )
  {
    return;
  }

  auto school = state->action->get_school();

  if ( !dbc::is_school( school, SCHOOL_FIRE ) &&
       !dbc::is_school( school, SCHOOL_NATURE ) &&
       !dbc::is_school( school, SCHOOL_FROST ) )
  {
    return;
  }

  if ( buff.elemental_equilibrium_debuff->check() )
  {
    return;
  }

  if ( dbc::is_school( school, SCHOOL_FIRE ) )
  {
    buff.elemental_equilibrium_fire->trigger();
  }

  if ( dbc::is_school( school, SCHOOL_FROST ) )
  {
    buff.elemental_equilibrium_frost->trigger();
  }

  if ( dbc::is_school( school, SCHOOL_NATURE ) )
  {
    buff.elemental_equilibrium_nature->trigger();
  }

  if ( buff.elemental_equilibrium_fire->up() &&
       buff.elemental_equilibrium_frost->up() &&
       buff.elemental_equilibrium_nature->up() )
  {
    buff.elemental_equilibrium->trigger();
    buff.elemental_equilibrium_debuff->trigger();
    buff.elemental_equilibrium_fire->expire();
    buff.elemental_equilibrium_frost->expire();
    buff.elemental_equilibrium_nature->expire();
  }
}

void shaman_t::trigger_deeply_rooted_elements( const action_state_t* state )
{
  if ( !talent.deeply_rooted_elements.ok() )
  {
    return;
  }

  double proc_chance = 0.0;

  if ( specialization() == SHAMAN_ELEMENTAL )
  {
    auto lvb = debug_cast<lava_burst_t*>( state->action );
    if ( lvb->exec_type != spell_variant::NORMAL )
    {
      return;
    }

    proc_chance = talent.deeply_rooted_elements->effectN( 2 ).percent();
  }
  else if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    auto sb = debug_cast<stormstrike_base_t*>( state->action );
    if ( sb->strike_type == strike_variant::STORMFLURRY )
    {
      return;
    }

    proc_chance = talent.deeply_rooted_elements->effectN( 3 ).percent();
  }
  // No resto support for now
  else
  {
    return;
  }

  dre_attempts++;
  if ( options.dre_flat_chance <= 0 )
  {
    // per attempt there exists an ever growing 1% chance
    // proc curve is pushed down by 2%, so the first two attempts have a 0% chance to occur
    proc_chance = dre_attempts * 0.01 - 0.01 * options.dre_forced_failures;
  }

  if ( !rng().roll( proc_chance ) )
  {
    return;
  }

  dre_samples.add( as<double>( dre_attempts ) );
  dre_attempts = 0U;

  action.dre_ascendance->set_target( state->target );
  action.dre_ascendance->execute();
  proc.deeply_rooted_elements->occur();
}

void shaman_t::trigger_secondary_flame_shock( player_t* target ) const
{
  action.flame_shock->set_target( target );
  action.flame_shock->execute();
}

void shaman_t::trigger_secondary_flame_shock( const action_state_t* state ) const
{
  if ( !state->action->result_is_hit( state->result ) )
  {
    return;
  }

  trigger_secondary_flame_shock( state->target );
}

void shaman_t::regenerate_flame_shock_dependent_target_list( const action_t* action ) const
{
  auto& tl = action->target_cache.list;

  auto it = std::remove_if( tl.begin(), tl.end(),
    [ this ]( player_t* target ) {
      return !get_target_data( target )->dot.flame_shock->is_ticking();
    }
  );

  tl.erase( it, tl.end() );

  if ( sim->debug )
  {
    sim->print_debug("{} targets with flame_shock on:", *this );
    for ( size_t i = 0; i < tl.size(); i++ )
    {
      sim->print_debug( "[{}, {} (id={})]", i, *tl[ i ], tl[ i ]->actor_index );
    }
  }
}

void shaman_t::consume_maelstrom_weapon( const action_state_t* state, int stacks )
{
  if ( !spec.maelstrom_weapon->ok() )
  {
    return;
  }

  if ( state->action->internal_id >= as<int>( mw_spend_list.size() ) )
  {
    mw_spend_list.resize( state->action->internal_id + 1 );
  }

  mw_spend_list[ state->action->internal_id ][ stacks ].add( stacks );

  if ( stacks > 0 )
  {
    buff.maelstrom_weapon->decrement( stacks );

    if ( talent.hailstorm.ok() )
    {
      buff.hailstorm->trigger( stacks );
    }

    trigger_legacy_of_the_frost_witch( state, stacks );

    if ( sets->has_set_bonus( SHAMAN_ENHANCEMENT, T28, B2 ) &&
         rng().roll( spell.t28_2pc_enh->effectN( 1 ).percent() * stacks ) )
    {
      if ( sim->debug )
      {
        sim->out_debug.print( "{} Enhancement T28 2PC", name() );
      }

      action.feral_spirit_t28->set_target( state->target );
      action.feral_spirit_t28->execute();
    }

    if ( sets->has_set_bonus( SHAMAN_ENHANCEMENT, T29, B4 ) )
    {
      //4pc refreshes duration and adds stacks
      buff.t29_4pc_enh->trigger( stacks );
    }

    trigger_tempest( stacks );

    if ( talent.unlimited_power.ok() )
    {
      buff.unlimited_power->trigger();
    }
  }
}

void shaman_t::trigger_maelstrom_gain( double maelstrom_gain, gain_t* gain )
{
  if ( maelstrom_gain <= 0 )
  {
    return;
  }

  double g = maelstrom_gain;
  g *= composite_maelstrom_gain_coefficient();
  resource_gain( RESOURCE_MAELSTROM, g, gain );
}

void shaman_t::generate_maelstrom_weapon( const action_t* action, int stacks )
{
  if ( !spec.maelstrom_weapon->ok() )
  {
    return;
  }

  auto stacks_avail = buff.maelstrom_weapon->max_stack() - buff.maelstrom_weapon->check();
  auto stacks_added = std::min( stacks_avail, stacks );
  auto overflow = stacks - stacks_added;

  if ( action != nullptr )
  {
    if ( as<unsigned>( action->internal_id ) >= mw_source_list.size() )
    {
      mw_source_list.resize( action->internal_id + 1 );
    }

    mw_source_list[ action->internal_id ].first.add( as<double>( stacks_added ) );

    if ( overflow > 0 )
    {
      mw_source_list[ action->internal_id ].second.add( as<double>( overflow ) );
    }
  }

  buff.maelstrom_weapon->trigger( stacks );
}

void shaman_t::generate_maelstrom_weapon( const action_state_t* state, int stacks )
{
  generate_maelstrom_weapon( state->action, stacks );
}

double shaman_t::windfury_proc_chance()
{
  double proc_chance = spell.windfury_weapon->proc_chance();
  proc_chance += talent.imbuement_mastery->effectN( 1 ).percent();
  double proc_mul = mastery.enhanced_elements->effectN( 4 ).mastery_value() *
    ( 1.0 + talent.storms_wrath->effectN( 2 ).percent() );

  proc_chance += cache.mastery() * proc_mul;
  if ( buff.doom_winds->up() )
  {
    proc_chance *= talent.windfury_weapon.ok() *
                  talent.doom_winds->effectN( 1 ).base_value();
  }

  return proc_chance;
}

void shaman_t::trigger_windfury_weapon( const action_state_t* state )
{
  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr && "Windfury Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_windfury )
    return;

  if ( buff.ghost_wolf->check() )
  {
    return;
  }

  // Note, applying Windfury-imbue to off-hand disables procs in game.
  if ( main_hand_weapon.buff_type != WINDFURY_IMBUE ||
      off_hand_weapon.buff_type == WINDFURY_IMBUE )
  {
    return;
  }

  if ( state->action->weapon->slot == SLOT_MAIN_HAND && rng().roll( windfury_proc_chance() ) )
  {
    action_t* a = windfury_mh;

    if ( talent.forceful_winds->ok() )
    {
      buff.forceful_winds->trigger();
    }

    // Note, windfury needs to do a discrete execute event because in AoE situations, Forceful Winds
    // must be let to stack (fully) before any Windfury Attacks are executed. In this case, the
    // schedule must be done through a pre-snapshotted state object to preserve targeting
    // information.
    trigger_secondary_ability( state, a );
    trigger_secondary_ability( state, a );

    double chance = talent.unruly_winds->effectN( 1 ).percent();

    if ( rng().roll( chance ) )
    {
      trigger_secondary_ability( state, a );
      proc.windfury_uw->occur();
    }

    attack->proc_wf->occur();
  }
}

void shaman_t::trigger_flametongue_weapon( const action_state_t* state )
{
  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr &&
          "Flametongue Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_flametongue )
  {
    return;
  }

  if ( main_hand_weapon.buff_type != FLAMETONGUE_IMBUE &&
       off_hand_weapon.buff_type != FLAMETONGUE_IMBUE )
  {
    return;
  }

  if ( buff.ghost_wolf->check() )
  {
    return;
  }

  flametongue->set_target( state->target );
  flametongue->execute();
  attack->proc_ft->occur();
}

void shaman_t::trigger_lava_surge()
{
  if ( buff.lava_surge->check() )
  {
    proc.wasted_lava_surge->occur();
  }

  proc.lava_surge->occur();

  if ( !executing || executing->id != 51505 )
  {
    cooldown.lava_burst->reset( true );
  }
  else
  {
    proc.surge_during_lvb->occur();
    lava_surge_during_lvb = true;
  }

  buff.lava_surge->trigger();
}

void shaman_t::trigger_splintered_elements( action_t* secondary )
{
  if ( !talent.splintered_elements->ok() )
  {
    return;
  }

  auto count_duplicates = secondary->target_list().size();
  if ( count_duplicates == 0 )
  {
    return;
  }

  buff.splintered_elements->trigger( as<int>( count_duplicates ) );
}

void shaman_t::trigger_flash_of_lightning()
{
  if ( !talent.flash_of_lightning.ok() ) {
    return;
  }

  auto reduction = talent.flash_of_lightning.spell()->effectN( 1 ).time_value();

  if ( talent.storm_elemental.enabled() )
  {
    cooldown.storm_elemental->adjust( reduction, false );
  }

  if ( talent.stormkeeper.enabled() )
  {
    cooldown.stormkeeper->adjust( reduction, false );
  }

  if ( talent.natures_swiftness.enabled() )
  {
    cooldown.natures_swiftness->adjust( reduction, false );
  }

  if ( talent.totemic_recall.enabled() )
  {
    cooldown.totemic_recall->adjust( reduction, false );
  }

  if ( talent.primordial_wave.enabled() )
  {
    cooldown.primordial_wave->adjust( reduction, false );
  }

  if ( talent.ancestral_swiftness.enabled() )
  {
    cooldown.ancestral_swiftness->adjust( reduction, false );
  }

  cooldown.flame_shock->adjust( reduction, false );

  proc.flash_of_lightning->occur();
}

void shaman_t::trigger_swirling_maelstrom( const action_state_t* state )
{
  if ( !talent.swirling_maelstrom.ok() )
  {
    return;
  }

  generate_maelstrom_weapon( state, as<int>( talent.swirling_maelstrom->effectN( 1 ).base_value() ) );
}

void shaman_t::trigger_static_accumulation_refund( const action_state_t* state, int mw_stacks )
{
  if ( !talent.static_accumulation.ok() || mw_stacks == 0 )
  {
    return;
  }

  if ( !rng().roll( talent.static_accumulation->effectN( 2 ).percent() ) )
  {
    return;
  }

  generate_maelstrom_weapon( state, mw_stacks );
}

void shaman_t::trigger_elemental_assault( const action_state_t* state )
{
  if ( !talent.elemental_assault.ok() )
  {
    return;
  }

  if ( !rng().roll( talent.elemental_assault->effectN( 3 ).percent() )  )
  {
    return;
  }

  make_event( sim, 0_s, [ this, state ]() {
    generate_maelstrom_weapon( state,
                               as<int>( talent.elemental_assault->effectN( 2 ).base_value() ) );
    } );
}

void shaman_t::trigger_tempest_strikes( const action_state_t* state )
{
  if ( !talent.tempest_strikes.ok() || cooldown.tempest_strikes->down() )
  {
    return;
  }

  if ( !rng().roll( talent.tempest_strikes->proc_chance() ) )
  {
    return;
  }

  action.tempest_strikes->set_target( state->target );
  action.tempest_strikes->execute();

  cooldown.tempest_strikes->start( talent.tempest_strikes->internal_cooldown() );
}

void shaman_t::trigger_stormflurry( const action_state_t* state )
{
  if ( !talent.stormflurry.ok() )
  {
    return;
  }

  if ( !rng().roll( talent.stormflurry->effectN( 1 ).percent() ) )
  {
    return;
  }

  auto a = state->action->id == 115356 ? action.stormflurry_ws : action.stormflurry_ss;

  timespan_t delay = rng().gauss<200,25>();
  if ( sim->debug )
  {
    auto ss = static_cast<stormstrike_base_t*>( state->action );
    sim->out_debug.print(
      "{} scheduling stormflurry source={}, action={}, target={}, delay={}, chained={}",
      name(), state->action->name(), a->name(), state->target->name(), delay,
      static_cast<unsigned>( ss->strike_type ) );
  }

  make_event<stormstrike_t::stormflurry_event_t>( *sim, static_cast<stormstrike_base_t*>( a ),
                                                 state->target, delay );
}

void shaman_t::trigger_primordial_wave_damage( shaman_spell_t* spell )
{
  if ( !talent.primordial_wave.ok() )
  {
    return;
  }

  if ( spell->exec_type != spell_variant::NORMAL || !buff.primordial_wave->up() )
  {
    return;
  }

  action_t* damage_spell = nullptr;

  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    damage_spell = debug_cast<shaman_spell_t*>( action.lightning_bolt_pw );
  }
  else if ( specialization() == SHAMAN_ELEMENTAL )
  {
    damage_spell = debug_cast<shaman_spell_t*>( action.lava_burst_pw );
  }

  damage_spell->set_target( spell->target );
  if ( !damage_spell->target_list().empty() )
  {
    damage_spell->execute();
  }
  trigger_splintered_elements( damage_spell );

  buff.primordial_wave->expire();
}

template <typename T>
void shaman_t::trigger_tempest( T resource_count )
{
  if ( !talent.tempest.ok() )
  {
    return;
  }

  unsigned tempest_threshold = as<unsigned>( talent.tempest->effectN(
    specialization() == SHAMAN_ELEMENTAL ? 1 : 2 ).base_value() );

  tempest_counter += as<unsigned>( resource_count );

  if ( tempest_counter >= tempest_threshold )
  {
    tempest_counter -= tempest_threshold;
    buff.tempest->trigger();
  }
}

void shaman_t::trigger_awakening_storms( const action_state_t* state )
{
  if ( !talent.awakening_storms.ok() )
  {
    return;
  }

  if ( !rng_obj.awakening_storms->trigger() )
  {
    return;
  }

  buff.awakening_storms->trigger();

  if ( buff.awakening_storms->stack() == talent.awakening_storms->effectN( 2 ).base_value() )
  {
    if ( buff.tempest->check() )
    {
      proc.tempest_awakening_storms->occur();
    }
    buff.awakening_storms->expire();
    buff.tempest->trigger();
  }

  action.awakening_storms->execute_on_target( state->target );
}

void shaman_t::trigger_earthsurge( const action_state_t* /* state */ )
{
  if ( !talent.earthsurge.ok() )
  {
    return;
  }

  surging_totem_t* totem = debug_cast<surging_totem_t*>( pet.surging_totem.active_pet() );
  if ( totem == nullptr )
  {
    return;
  }

  debug_cast<surging_totem_pulse_t*>( totem->pulse_action )->trigger_earthsurge();
}

void shaman_t::trigger_whirling_air( const action_state_t* state )
{
  if ( !talent.whirling_elements.ok() )
  {
    return;
  }

  if ( !buff.whirling_air->up() )
  {
    return;
  }

  auto a = state->action->id == 115356 ? action.whirling_air_ws : action.whirling_air_ss;

  int n_targets = as<int>( buff.whirling_air->data().effectN( 3 ).base_value() );
  size_t target_idx = 0U;

  do
  {
    if ( a->target_list()[ target_idx ] != state->target )
    {
      make_event( sim, [a, target_idx]() {
        a->execute_on_target( a->target_list()[ target_idx ] );
      } );

      --n_targets;
    }

    ++target_idx;
  } while ( n_targets > 0 && target_idx < a->target_list().size() );

  buff.whirling_air->decrement();
}

void shaman_t::trigger_reactivity( const action_state_t* state )
{
  if ( !talent.reactivity.ok() )
  {
    return;
  }

  for ( auto totem_ptr : pet.searing_totem )
  {
    searing_totem_t* st = debug_cast<searing_totem_t*>( totem_ptr );
    st->volley->execute_on_target( state->target );
  }
}

void shaman_t::trigger_fusion_of_elements( const action_state_t* state )
{
  if ( !talent.fusion_of_elements.ok() )
  {
    return;
  }

  bool consumed = false;

  if ( buff.fusion_of_elements_nature->up() && dbc::is_school( state->action->school, SCHOOL_NATURE ) )
  {
    buff.fusion_of_elements_nature->expire();
    consumed = true;
  }

  if ( buff.fusion_of_elements_fire->up() && dbc::is_school( state->action->school, SCHOOL_FIRE ) )
  {
    buff.fusion_of_elements_fire->expire();
    consumed = true;
  }

  if ( consumed && !buff.fusion_of_elements_nature->check() && !buff.fusion_of_elements_fire->check() )
  {
    action.elemental_blast_foe->execute_on_target( state->target );
  }
}

void shaman_t::trigger_thunderstrike_ward( const action_state_t* state )
{
  if ( !buff.thunderstrike_ward->up() )
  {
    return;
  }

  if ( !rng().roll( options.thunderstrike_ward_proc_chance ) )
  {
    return;
  }

  for ( int i = 0; i < talent.thunderstrike_ward->effectN( 1 ).base_value(); ++i )
  {
    action.thunderstrike_ward->execute_on_target( state->target );
  }
}

// TODO: Target swaps
void shaman_t::trigger_earthen_rage( const action_state_t* state )
{
  if ( !talent.earthen_rage -> ok() )
  {
    return;
  }

  if ( !state->action->harmful )
  {
    return;
  }

  if ( !state->action->result_is_hit( state -> result ) )
  {
    return;
  }

  // Earthen Rage damage does not trigger itself
  if ( state->action == action.earthen_rage )
  {
    return;
  }

  if ( sim->debug )
  {
    sim->out_debug.print( "{} earthen_rage proc by {}", *this, *state->action );
  }

  earthen_rage_target = state->target;
  if ( earthen_rage_event == nullptr )
  {
    earthen_rage_event = make_event<earthen_rage_event_t>( *sim, this,
      sim->current_time() + spell.earthen_rage->duration() );
  }
  else
  {
    debug_cast<earthen_rage_event_t*>( earthen_rage_event )
        ->set_end_time( sim->current_time() + spell.earthen_rage->duration() );
  }
}

void shaman_t::trigger_totemic_rebound( const action_state_t* state )
{
  if ( !pet.surging_totem.n_active_pets() )
  {
    return;
  }

  if ( !buff.totemic_rebound->trigger() )
  {
    return;
  }

  for ( auto totem : pet.surging_totem )
  {
    debug_cast<surging_totem_t*>( totem )->trigger_surging_bolt( state->target );
  }
}

void shaman_t::trigger_ancestor( ancestor_cast cast, const action_state_t* state )
{
  if ( cast == ancestor_cast::DISABLED )
  {
    return;
  }

  if ( !talent.call_of_the_ancestors.ok() )
  {
    return;
  }

  for ( auto ancestor : pet.ancestor )
  {
    if ( sim->debug )
    {
      sim->out_debug.print( "{} ancestor triggers {} from {} at {}",
        name(), ancestor_cast_str( cast ), state->action->name(), state->target->name() );
    }

    debug_cast<pet::ancestor_t*>( ancestor )->trigger_cast( cast, state->target );
  }
}

// shaman_t::init_buffs =====================================================

void shaman_t::create_buffs()
{
  player_t::create_buffs();

  //
  // Shared
  //
  buff.ascendance = new ascendance_buff_t( this );
  buff.ghost_wolf = make_buff( this, "ghost_wolf", find_class_spell( "Ghost Wolf" ) );
  buff.flurry = make_buff( this, "flurry", talent.flurry->effectN( 1 ).trigger() )
    ->set_default_value( talent.flurry->effectN( 1 ).trigger()->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );
  buff.natures_swiftness = make_buff( this, "natures_swiftness", talent.natures_swiftness );

  buff.elemental_blast_crit = make_buff<buff_t>( this, "elemental_blast_critical_strike", find_spell( 118522 ) )
    ->set_default_value_from_effect_type(A_MOD_ALL_CRIT_CHANCE)
    ->apply_affecting_aura(spec.elemental_shaman)
    ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.elemental_blast_haste = make_buff<buff_t>( this, "elemental_blast_haste", find_spell( 173183 ) )
    ->set_default_value_from_effect_type(A_HASTE_ALL)
    ->apply_affecting_aura(spec.elemental_shaman)
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.elemental_blast_mastery = make_buff<buff_t>( this, "elemental_blast_mastery", find_spell( 173184 ) )
    ->set_default_value_from_effect_type(A_MOD_MASTERY_PCT)
    ->apply_affecting_aura(spec.elemental_shaman)
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.stormkeeper = make_buff( this, "stormkeeper", find_spell( 191634 ) )
    ->set_cooldown( timespan_t::zero() )  // Handled by the action
    ->set_default_value_from_effect( 2 ); // Damage bonus as default value

  buff.tww1_4pc_ele =
    make_buff( this, "maelstrom_surge", sets->set( SHAMAN_ELEMENTAL, TWW1, B4 )->effectN( 1 ).trigger() )
        ->set_default_value_from_effect( 1 )
        ->set_trigger_spell( sets->set( SHAMAN_ELEMENTAL, TWW1, B4 ) );

  buff.primordial_wave = make_buff( this, "primordial_wave", find_spell( 327164 ) )
    ->set_default_value( talent.primordial_wave->effectN( specialization() == SHAMAN_ELEMENTAL ? 3 : 4 ).percent() )
    ->set_trigger_spell( talent.primordial_wave );

  buff.tempest = make_buff( this, "tempest", find_spell( 454015 ) );
  buff.unlimited_power = make_buff( this, "unlimited_power", find_spell( 454394 ) )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED );
  buff.arc_discharge = make_buff( this, "arc_discharge", find_spell( 455097 ) )
    ->set_default_value_from_effect( 2 );
  buff.amplification_core = make_buff( this, "amplification_core", find_spell( 456369 ) )
    ->set_default_value_from_effect( 1 )
    ->set_trigger_spell( talent.amplification_core );

  buff.whirling_air = make_buff( this, "whirling_air", find_spell( 453409 ) )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
    ->set_trigger_spell( talent.whirling_elements );
  buff.whirling_fire = make_buff( this, "whirling_fire", find_spell( 453405 ) )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_CRIT )
    ->set_trigger_spell( talent.whirling_elements );
  buff.whirling_earth = make_buff( this, "whirling_earth", find_spell( 453406 ) )
    ->set_trigger_spell( talent.whirling_elements );
  buff.lightning_shield = make_buff( this, "lightning_shield", find_spell( 192106 ) )
      ->add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER);

  buff.awakening_storms = make_buff( this, "awakening_storms", find_spell( 462131 ) )
    ->set_chance( talent.awakening_storms.ok() ? 1.0 : 0.0 );

  buff.totemic_rebound = make_buff( this, "totemic_rebound", find_spell( 458269 ) )
    ->set_default_value_from_effect( 1 )
    ->set_trigger_spell( talent.totemic_rebound );

  buff.flametongue_weapon = make_buff( this, "flametongue_weapon", find_class_spell( "Flametongue Weapon") );

  //
  // Elemental
  //
  buff.lava_surge = make_buff( this, "lava_surge", find_spell( 77762 ) )
                        ->set_activated( false )
                        ->set_chance( 1.0 );  // Proc chance is handled externally

  buff.surge_of_power = make_buff( this, "surge_of_power", talent.surge_of_power )
    ->set_duration( find_spell( 285514 )->duration() );

  buff.icefury_dmg = make_buff( this, "icefury_dmg", find_spell( 210714 ) )
                     ->set_default_value( find_spell( 210714 )->effectN( 2 ).percent() )
                     ->set_trigger_spell( talent.icefury );

  buff.icefury_cast = make_buff( this, "icefury", find_spell( 462818 ) )
                     ->set_trigger_spell( talent.icefury );

  buff.master_of_the_elements = make_buff( this, "master_of_the_elements", talent.master_of_the_elements->effectN(1).trigger() )
          ->set_default_value( talent.master_of_the_elements->effectN( 2 ).percent() );

  buff.wind_gust = make_buff( this, "wind_gust", find_spell( 263806 ) )
                       ->set_default_value( find_spell( 263806 )->effectN( 1 ).percent() );

  buff.echoes_of_great_sundering_es =
      make_buff( this, "echoes_of_great_sundering_es", find_spell( 336217 ) )
        ->set_default_value( talent.echoes_of_great_sundering->effectN( 1 ).percent() )
        ->set_trigger_spell( talent.echoes_of_great_sundering );
  buff.echoes_of_great_sundering_eb =
      make_buff( this, "echoes_of_great_sundering_eb", find_spell( 336217 ) )
        ->set_default_value( talent.echoes_of_great_sundering->effectN( 2 ).percent() )
        ->set_trigger_spell( talent.echoes_of_great_sundering );

  buff.flux_melting = make_buff( this, "flux_melting", talent.flux_melting->effectN( 1 ).trigger() )
                            ->set_default_value( talent.flux_melting->effectN( 1 ).trigger()->effectN(1).percent() );

  buff.magma_chamber = make_buff( this, "magma_chamber", find_spell( 381933 ) )
                            ->set_default_value( talent.magma_chamber->effectN( 1 ).base_value() * ( 1 / 1000.0) );

  buff.power_of_the_maelstrom =
      make_buff( this, "power_of_the_maelstrom", talent.power_of_the_maelstrom->effectN( 1 ).trigger() )
          ->set_default_value( talent.power_of_the_maelstrom->effectN( 1 ).trigger()->effectN( 1 ).base_value() );

  // PvP
  buff.thundercharge = make_buff( this, "thundercharge", find_spell( 204366 ) )
                           ->set_cooldown( timespan_t::zero() )
                           ->set_default_value( find_spell( 204366 )->effectN( 1 ).percent() )
                           ->set_stack_change_callback( [ this ]( buff_t*, int, int ) {
                             range::for_each( ability_cooldowns, []( cooldown_t* cd ) {
                               if ( cd->down() )
                               {
                                 cd->adjust_recharge_multiplier();
                               }
                             } );
                           } );

  buff.earth_elemental = make_buff( this, "earth_elemental", find_spell( 188616 ))
                        ->set_duration( find_spell( 188616 )->duration() *
                          ( 1.0 + talent.everlasting_elements->effectN( 1 ).percent() ) );
  buff.fire_elemental = make_buff( this, "fire_elemental", spell.fire_elemental )
                        ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_TICK_TIME )
                        ->set_duration( spell.fire_elemental->duration() *
                          ( 1.0 + talent.everlasting_elements->effectN( 1 ).percent() ) );
  buff.lesser_fire_elemental = make_buff( this, "lesser_fire_elemental", find_spell( 462992 ))
                        ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_TICK_TIME )
                        ->set_duration( find_spell( 462992 )->duration() *
                          ( 1.0 + talent.everlasting_elements->effectN( 1 ).percent() ) );
  buff.storm_elemental = make_buff( this, "storm_elemental", spell.storm_elemental )
                         ->set_duration( spell.storm_elemental->duration() *
                          ( 1.0 + talent.everlasting_elements->effectN( 1 ).percent() ) );
  buff.lesser_storm_elemental = make_buff( this, "lesser_storm_elemental", find_spell( 462993 ))
                        ->set_duration( find_spell( 462993 )->duration() *
                          ( 1.0 + talent.everlasting_elements->effectN( 1 ).percent() ) );
  buff.splintered_elements = new splintered_elements_buff_t( this );

  buff.fusion_of_elements_nature = make_buff( this, "fusion_of_elements_nature",
                                         talent.fusion_of_elements->effectN( 1 ).trigger() )
                                ->set_trigger_spell( talent.fusion_of_elements );
  buff.fusion_of_elements_fire = make_buff( this, "fusion_of_elements_fire",
                                         talent.fusion_of_elements->effectN( 2 ).trigger() )
                                ->set_trigger_spell( talent.fusion_of_elements );
  buff.storm_frenzy = make_buff( this, "storm_frenzy", find_spell(462725) )
                                ->set_default_value_from_effect( 1 )
                                ->set_trigger_spell( talent.storm_frenzy );
  buff.fury_of_the_storms = make_buff( this, "fury_of_storms", find_spell( 191716 ) )
                                ->set_trigger_spell( talent.fury_of_the_storms )
                                ->set_duration( find_spell( 191716 )->duration() *
                                  ( 1.0 + talent.everlasting_elements->effectN( 1 ).percent() ) );

  buff.call_of_the_ancestors = make_buff( this, "call_of_the_ancestors", find_spell( 447244 ) )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->apply_affecting_aura( talent.heed_my_call )
    ->set_trigger_spell( talent.call_of_the_ancestors );
  buff.ancestral_swiftness = make_buff( this, "ancestral_swiftness", find_spell( 443454 ) )
    ->set_trigger_spell( talent.ancestral_swiftness )
    ->set_cooldown( 0_ms );
  buff.thunderstrike_ward = make_buff( this, "thunderstrike_ward", talent.thunderstrike_ward );

  //
  // Enhancement
  //

  buff.feral_spirit_maelstrom = make_buff( this, "feral_spirit", find_spell( 333957 ) )
                                    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                    ->set_tick_behavior( buff_tick_behavior::REFRESH )
                                    ->set_tick_zero( true )
                                    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
                                      generate_maelstrom_weapon( action.feral_spirits,
                                                               as<int>( b->data().effectN( 1 ).base_value() ) );
                                    } );

  buff.forceful_winds   = make_buff<buff_t>( this, "forceful_winds", find_spell( 262652 ) )
                            ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
                            ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC );

  buff.icy_edge = make_buff<buff_t>( this, "icy_edge", find_spell( 224126 ) )
    ->set_max_stack( 10 )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_default_value_from_effect( 1 );
  buff.molten_weapon    = make_buff<buff_t>( this, "molten_weapon", find_spell( 224125 ) )
    ->set_max_stack( 10 )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_default_value_from_effect( 1 );
  buff.crackling_surge  = make_buff<buff_t>( this, "crackling_surge", find_spell( 224127 ) )
    ->set_max_stack( 10 )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_default_value_from_effect( 1 );
  buff.earthen_weapon = make_buff<buff_t>( this, "earthen_weapon", find_spell( 392375 ) )
    ->set_max_stack( 10 )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_default_value_from_effect( 1 );
  buff.converging_storms = make_buff( this, "converging_storms", find_spell( 198300 ) )
      ->set_default_value_from_effect( 1 );
  buff.ashen_catalyst = make_buff( this, "ashen_catalyst", find_spell( 390371 ) )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
    ->set_trigger_spell( talent.ashen_catalyst );
  buff.witch_doctors_ancestry = make_buff<buff_t>( this, "witch_doctors_ancestry",
      talent.witch_doctors_ancestry )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_PROC_CHANCE );

  buff.t29_4pc_enh = make_buff<buff_t>( this, "fury_of_the_storm", find_spell( 396006 ) )  //, find_spell( 393693 ) )
                         ->set_default_value_from_effect_type( A_HASTE_ALL )
                         ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.t29_2pc_enh = make_buff<buff_t>( this, "maelstrom_of_elements", find_spell( 394677 ) )
                   ->set_default_value_from_effect( 1 )
                   ->set_max_stack( 1 );

  buff.t30_2pc_enh = make_buff<buff_t>( this, "earthen_might", find_spell( 409689 ) )
    ->set_default_value_from_effect( 1 )
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
    ->set_trigger_spell( sets->set( SHAMAN_ENHANCEMENT, T30, B2 ) );

  buff.t30_4pc_enh_damage = make_buff<buff_t>( this, "volcanic_strength", find_spell( 409833 ) )
    ->set_default_value_from_effect( 1 )
    ->set_trigger_spell( sets->set( SHAMAN_ENHANCEMENT, T30, B4 ) );
  buff.t30_4pc_enh_cl = make_buff<buff_t>( this, "crackling_thunder", find_spell( 409834 ) )
    ->set_default_value_from_effect( 2 )
    ->set_trigger_spell( sets->set( SHAMAN_ENHANCEMENT, T30, B4 ) );

  // Buffs stormstrike and lava lash after using crash lightning
  buff.crash_lightning = make_buff( this, "crash_lightning", find_spell( 187878 ) );
  // Buffs crash lightning with extra damage, after using chain lightning
  buff.cl_crash_lightning = new cl_crash_lightning_buff_t( this );

  buff.hot_hand = new hot_hand_buff_t( this );
  buff.spirit_walk  = make_buff( this, "spirit_walk", talent.spirit_walk );
  buff.stormbringer = make_buff( this, "stormbringer", find_spell( 201846 ) );
  buff.maelstrom_weapon = new maelstrom_weapon_buff_t( this );
  buff.hailstorm        = make_buff( this, "hailstorm", find_spell( 334196 ) )
                            ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
                            ->set_max_stack(
                              talent.overflowing_maelstrom.ok()
                              ? as<int>( talent.overflowing_maelstrom->effectN( 1 ).base_value() )
                              : find_spell( 334196 )->max_stacks() );
  buff.static_accumulation = make_buff( this, "static_accumulation", find_spell( 384437 ) )
    ->set_default_value( talent.static_accumulation->effectN( 1 ).base_value() )
    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
      generate_maelstrom_weapon( action.ascendance, as<int>( b->value() ) );
    } );
  buff.doom_winds = make_buff( this, "doom_winds", talent.doom_winds );
  buff.ice_strike = make_buff( this, "ice_strike", talent.ice_strike->effectN( 3 ).trigger() )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER );
  buff.windfury_weapon = make_buff( this, "windfury_weapon", find_spell( 319773 ) )
    ->set_trigger_spell( talent.windfury_weapon );

  //
  // Restoration
  //
  buff.spiritwalkers_grace =
      make_buff( this, "spiritwalkers_grace", find_specialization_spell( "Spiritwalker's Grace" ) )
          ->set_cooldown( timespan_t::zero() );
  buff.tidal_waves =
      make_buff( this, "tidal_waves", spec.tidal_waves->ok() ? find_spell( 53390 ) : spell_data_t::not_found() );


  // Legendary buffs
  buff.legacy_of_the_frost_witch = make_buff<buff_t>( this, "legacy_of_the_frost_witch", find_spell( 384451 ) )
    ->set_default_value( talent.legacy_of_the_frost_witch->effectN( 1 ).percent() );
  buff.elemental_equilibrium = make_buff<buff_t>( this, "elemental_equilibrium",
      find_spell( 378275 ) )
    ->set_default_value( talent.elemental_equilibrium->effectN( 4 ).percent() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.elemental_equilibrium_debuff = make_buff<buff_t>( this, "elemental_equilibrium_debuff",
      find_spell( 347349 ) );
  buff.elemental_equilibrium_frost = make_buff<buff_t>( this, "elemental_equilibrium_frost",
      find_spell( 336731 ) );
  buff.elemental_equilibrium_nature = make_buff<buff_t>( this, "elemental_equilibrium_nature",
      find_spell( 336732 ) );
  buff.elemental_equilibrium_fire = make_buff<buff_t>( this, "elemental_equilibrium_fire",
      find_spell( 336733 ) );
}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gain.aftershock              = get_gain( "Aftershock" );
  gain.searing_flames          = get_gain( "Searing Flames" );
  gain.ascendance              = get_gain( "Ascendance" );
  gain.resurgence              = get_gain( "resurgence" );
  gain.feral_spirit            = get_gain( "Feral Spirit" );
  gain.fire_elemental          = get_gain( "Fire Elemental" );
  gain.spirit_of_the_maelstrom = get_gain( "Spirit of the Maelstrom" );
  gain.inundate                = get_gain( "Inundate" );
  gain.storm_swell             = get_gain( "Storm Swell" );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.lava_surge                               = get_proc( "Lava Surge" );
  proc.wasted_lava_surge                        = get_proc( "Lava Surge: Wasted" );
  proc.surge_during_lvb                         = get_proc( "Lava Surge: During Lava Burst" );

  proc.deeply_rooted_elements                   = get_proc( "Deeply Rooted Elements" );

  proc.surge_of_power_lightning_bolt = get_proc( "Surge of Power: Lightning Bolt" );
  proc.surge_of_power_sk_lightning_bolt = get_proc( "Surge of Power: SK Lightning Bolt" );
  proc.surge_of_power_lava_burst     = get_proc( "Surge of Power: Lava Burst" );
  proc.surge_of_power_frost_shock    = get_proc( "Surge of Power: Frost Shock" );
  proc.surge_of_power_flame_shock    = get_proc( "Surge of Power: Flame Shock" );
  proc.surge_of_power_wasted    = get_proc( "Surge of Power: Wasted" );

  proc.aftershock           = get_proc( "Aftershock" );
  proc.flash_of_lightning   = get_proc( "Flash of Lightning" );
  proc.lightning_rod        = get_proc( "Lightning Rod" );
  proc.searing_flames       = get_proc( "Searing Flames" );
  for ( size_t i = 0; i < proc.magma_chamber.size(); i++ )
  {
    proc.magma_chamber[ i ] = get_proc( fmt::format( "Magma Chamber {}", i ) );
  }
  proc.elemental_blast_crit = get_proc( "Elemental Blast: Critical Strike" );
  proc.elemental_blast_haste = get_proc( "Elemental Blast: Haste" );
  proc.elemental_blast_mastery = get_proc( "Elemental Blast: Mastery" );

  proc.windfury_uw            = get_proc( "Windfury: Unruly Winds" );
  proc.stormflurry            = get_proc( "Stormflurry" );
  proc.stormflurry_failed     = get_proc( "Stormflurry (failed)" );

  proc.t28_4pc_enh       = get_proc( "Set Bonus: Tier28 4PC Enhancement" );

  proc.reset_swing_mw            = get_proc( "Maelstrom Weapon Swing Reset" );

  proc.tempest_awakening_storms = get_proc( "Awakened Storms w/ Tempest");
}

// shaman_t::init_uptimes ====================================================
void shaman_t::init_uptimes()
{
  player_t::init_uptimes();

  uptime.hot_hand = get_uptime( "Hot Hand" )->collect_uptime( *sim )->collect_duration( *sim );
}

// shaman_t::init_assessors =================================================

void shaman_t::init_assessors()
{
  player_t::init_assessors();

  if ( talent.elemental_equilibrium.ok() )
  {
    assessor_out_damage.add( assessor::LEECH + 10,
      [ this ]( result_amount_type type, action_state_t* state ) {
        if ( type == result_amount_type::DMG_DIRECT && state->result_amount > 0 )
        {
          trigger_elemental_equilibrium( state );
        }
        return assessor::CONTINUE;
      } );
  }
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  player_t::init_rng();

  rng_obj.awakening_storms = get_rppm( "awakening_storms", talent.awakening_storms );
  rng_obj.lively_totems = get_rppm( "lively_totems", talent.lively_totems );

  if ( options.ancient_fellowship_positive == 0 ) {
    options.ancient_fellowship_positive = as<unsigned>( talent.ancient_fellowship->effectN( 3 ).base_value() );
  }

  if ( options.ancient_fellowship_total == 0 ) {
    options.ancient_fellowship_total = as<unsigned>( talent.ancient_fellowship->effectN( 2 ).base_value() );
  }
  rng_obj.ancient_fellowship =
    get_shuffled_rng( "ancient_fellowship", options.ancient_fellowship_positive, options.ancient_fellowship_total );
  if ( options.icefury_positive == 0 ) {
    options.icefury_positive = as<unsigned>( talent.icefury->effectN( 1 ).base_value() );
  }

  if ( options.icefury_total == 0 ) {
    options.icefury_total = as<unsigned>( talent.icefury->effectN( 2 ).base_value() );
  }
  rng_obj.icefury = get_shuffled_rng( "icefury", options.icefury_positive, options.icefury_total );
}

// shaman_t::init_items =====================================================

void shaman_t::init_items()
{
  player_t::init_items();

  if ( sets->has_set_bonus( specialization(), DF4, B2 ) )
  {
    sets->enable_set_bonus( specialization(), T31 , B2 );
  }

  if ( sets->has_set_bonus( specialization(), DF4, B4 ) )
  {
    sets->enable_set_bonus( specialization(), T31 , B4 );
  }
}

// shaman_t::apply_affecting_auras ==========================================

void shaman_t::apply_affecting_auras( action_t& action )
{
  auto print_debug = [ this, &action ]( const spelleffect_data_t& effect ) {
    if ( !sim->debug )
    {
      return;
    }

    const spell_data_t& spell = *effect.spell();
    std::string desc_str;
    const auto& spell_text = dbc->spell_text( spell.id() );
    if ( spell_text.rank() )
      desc_str = fmt::format( " (desc={})", spell_text.rank() );

    sim->print_debug( "{} {} is affected by effect {} ({}{} (id={}) - effect #{})",
      *this, action, effect.id(), spell.name_cstr(), desc_str, spell.id(),
      effect.spell_effect_num() + 1 );
  };

  // Generic
  action.apply_affecting_aura( spec.shaman );
  action.apply_affecting_aura( talent.call_of_the_elements );

  // Specialization
  action.apply_affecting_aura( spec.elemental_shaman );
  action.apply_affecting_aura( spec.enhancement_shaman );
  action.apply_affecting_aura( spec.restoration_shaman );
  action.apply_affecting_aura( spec.lightning_bolt_2 );

  // Talents
  action.apply_affecting_aura( talent.echo_of_the_elements );
  action.apply_affecting_aura( talent.elemental_assault );
  action.apply_affecting_aura( talent.elemental_fury );
  action.apply_affecting_aura( talent.improved_lightning_bolt );
  action.apply_affecting_aura( talent.molten_assault );
  action.apply_affecting_aura( talent.natures_fury );
  action.apply_affecting_aura( talent.thundershock );
  action.apply_affecting_aura( talent.totemic_surge );
  action.apply_affecting_aura( talent.unrelenting_calamity );
  action.apply_affecting_aura( talent.swelling_maelstrom );
  action.apply_affecting_aura( talent.crashing_storms );
  action.apply_affecting_aura( talent.healing_stream_totem );
  action.apply_affecting_aura( talent.stormkeeper );
  action.apply_affecting_aura( talent.fire_and_ice );
  action.apply_affecting_aura( talent.thorims_invocation );
  action.apply_affecting_aura( talent.flash_of_lightning );

  action.apply_affecting_aura( talent.stormcaller );
  action.apply_affecting_aura( talent.voltaic_surge );
  action.apply_affecting_aura( talent.pulse_capacitor );
  action.apply_affecting_aura( talent.supportive_imbuements );
  action.apply_affecting_aura( talent.totemic_coordination );
  action.apply_affecting_aura( talent.first_ascendant );

  action.apply_affecting_aura( talent.latent_wisdom );
  action.apply_affecting_aura( talent.elemental_reverb );
  action.apply_affecting_aura( talent.maelstrom_supremacy );

  // Set bonuses
  action.apply_affecting_aura( sets->set( SHAMAN_ENHANCEMENT, TWW1, B2 ) );
  action.apply_affecting_aura( sets->set( SHAMAN_ELEMENTAL, TWW1, B2 ) );

  // Custom

  // Elemental Fury + Primordial Fury
  if ( action.data().affected_by_all( talent.elemental_fury->effectN( 1 ) ) )
  {
    print_debug( talent.elemental_fury->effectN( 1 ) );
    action.crit_bonus_multiplier = 1.0 + talent.elemental_fury->effectN( 1 ).percent() +
                                   talent.primordial_fury->effectN( 1 ).percent();
    sim->print_debug( "{} critical damage bonus multiplier modified by {}%", *this,
      talent.elemental_fury->effectN( 1 ).base_value() + talent.primordial_fury->effectN( 1 ).base_value() );
  }

  if ( action.data().affected_by_all( talent.elemental_fury->effectN( 2 ) ) )
  {
    print_debug( talent.elemental_fury->effectN( 2 ) );
    action.crit_bonus_multiplier = 1.0 + talent.elemental_fury->effectN( 2 ).percent() +
                                   talent.primordial_fury->effectN( 2 ).percent();
    sim->print_debug( "{} critical damage bonus multiplier modified by {}%", *this,
      talent.elemental_fury->effectN( 2 ).base_value() + talent.primordial_fury->effectN( 2 ).base_value() );
  }
}

// shaman_t::generate_bloodlust_options =====================================

std::string shaman_t::generate_bloodlust_options()
{
  std::string bloodlust_options = "if=";

  if ( sim->bloodlust_percent > 0 )
    bloodlust_options += "target.health.pct<" + util::to_string( sim->bloodlust_percent ) + "|";

  if ( sim->bloodlust_time < timespan_t::zero() )
    bloodlust_options += "target.time_to_die<" + util::to_string( -sim->bloodlust_time.total_seconds() ) + "|";

  if ( sim->bloodlust_time > timespan_t::zero() )
    bloodlust_options += "time>" + util::to_string( sim->bloodlust_time.total_seconds() ) + "|";
  bloodlust_options.erase( bloodlust_options.end() - 1 );

  return bloodlust_options;
}

// shaman_t::default_potion =================================================

std::string shaman_t::default_potion() const
{
  std::string elemental_potion = ( true_level >= 71 ) ? "tempered_potion_3" :
                                 ( true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
                                 ( true_level >= 51 ) ? "potion_of_spectral_intellect" :
                                 ( true_level >= 45 ) ? "potion_of_unbridled_fury" :
                                 "disabled";

  std::string enhancement_potion = ( true_level >= 71 ) ? "tempered_potion_3" :
                                   ( true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
                                   ( true_level >= 51 ) ? "potion_of_spectral_agility" :
                                   ( true_level >= 45 ) ? "potion_of_unbridled_fury" :
                                   "disabled";

  std::string restoration_potion = ( true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
                                   ( true_level >= 51 ) ? "potion_of_spectral_intellect" :
                                   ( true_level >= 45 ) ? "potion_of_unbridled_fury" :
                                   "disabled";

  switch(specialization()) {
    case SHAMAN_ELEMENTAL:
      return elemental_potion;
    case SHAMAN_ENHANCEMENT:
      return enhancement_potion;
    case SHAMAN_RESTORATION:
      return restoration_potion;
    default:
      return "disabled";
  }
}

// shaman_t::default_flask ==================================================

std::string shaman_t::default_flask() const
{
  std::string elemental_flask = ( true_level >= 71 ) ? "flask_of_tempered_swiftness_3" :
                                ( true_level >= 61 ) ? "iced_phial_of_corrupting_rage_3" :
                                ( true_level >= 51 ) ? "spectral_flask_of_power" :
                                ( true_level >= 45 ) ? "greater_flask_of_endless_fathoms" :
                                "disabled";

  std::string enhancement_flask = ( true_level >= 71 ) ? "flask_of_tempered_swiftness_3" :
                                  ( true_level >= 61 ) ? "iced_phial_of_corrupting_rage_3" :
                                  ( true_level >= 51 ) ? "spectral_flask_of_power" :
                                  ( true_level >= 45 ) ? "greater_flask_of_the_currents" :
                                  "disabled";

  std::string restoration_flask = ( true_level >= 61 ) ? "phial_of_static_empowerment_3" :
                                  ( true_level >= 51 ) ? "spectral_flask_of_power" :
                                  ( true_level >= 45 ) ? "greater_flask_of_endless_fathoms" :
                                  "disabled";

  switch(specialization()) {
    case SHAMAN_ELEMENTAL:
      return elemental_flask;
    case SHAMAN_ENHANCEMENT:
      return enhancement_flask;
    case SHAMAN_RESTORATION:
      return restoration_flask;
    default:
      return "disabled";
  }
}

// shaman_t::default_food ===================================================

std::string shaman_t::default_food() const
{
  std::string elemental_food = ( true_level >= 71 ) ? "feast_of_the_divine_day" :
                               ( true_level >= 61 ) ? "sizzling_seafood_medley" :
                               ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
                               ( true_level >= 45 ) ? "mechdowels_big_mech" :
                               "disabled";

  std::string enhancement_food = ( true_level >= 71 ) ? "feast_of_the_divine_day" :
                                 ( true_level >= 61 ) ? "fated_fortune_cookie" :
                                 ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
                                 ( true_level >= 45 ) ? "baked_port_tato" :
                                 "disabled";

  std::string restoration_food = ( true_level >= 61 ) ? "fated_fortune_cookie" :
                                 ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
                                 ( true_level >= 45 ) ? "baked_port_tato" :
                                 "disabled";

  switch(specialization()) {
    case SHAMAN_ELEMENTAL:
      return elemental_food;
    case SHAMAN_ENHANCEMENT:
      return enhancement_food;
    case SHAMAN_RESTORATION:
      return restoration_food;
    default:
      return "disabled";
  }
}

// shaman_t::default_rune ===================================================

std::string shaman_t::default_rune() const
{
  return ( true_level >= 71 ) ? "crystallized" :
         ( true_level >= 61 ) ? "draconic" :
         ( true_level >= 60 ) ? "veiled" :
         ( true_level >= 50 ) ? "battle_scarred" :
         ( true_level >= 45 ) ? "defiled" :
         ( true_level >= 40 ) ? "hyper" :
         "disabled";
}

// shaman_t::default_temporary_enchant ======================================

std::string shaman_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case SHAMAN_ELEMENTAL:
      return "main_hand:algari_mana_oil_3,if=!talent.improved_flametongue_weapon";
    case SHAMAN_ENHANCEMENT:
      return "disabled";
    case SHAMAN_RESTORATION:
      if ( true_level >= 60 )
        return "main_hand:shadowcore_oil";
      SC_FALLTHROUGH;
    default:
      return "disabled";
  }
}

// shaman_t::resource_loss ==================================================

double shaman_t::resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* a )
{
  double loss = player_t::resource_loss( resource_type, amount, source, a );

  if ( resource_type == RESOURCE_MAELSTROM && loss > 0 )
  {
    trigger_tempest( loss );

    if ( talent.unlimited_power.ok() )
    {
      buff.unlimited_power->trigger();
    }
  
    buff.tww1_4pc_ele->trigger();
  }

  return loss;
}

// shaman_t::init_action_list_elemental =====================================

void shaman_t::init_action_list_elemental()
{
  action_priority_list_t* precombat     = get_action_priority_list( "precombat" );
  action_priority_list_t* def           = get_action_priority_list( "default" );

  action_priority_list_t* single_target    = get_action_priority_list( "single_target" );
  action_priority_list_t* aoe              = get_action_priority_list( "aoe" );

  if (options.rotation == ROTATION_SIMPLE || options.rotation == ROTATION_STANDARD) {
    precombat->add_action( "flask" );
    precombat->add_action( "food" );
    precombat->add_action( "augmentation" );
    precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
    precombat->add_action( "flametongue_weapon,if=talent.improved_flametongue_weapon.enabled", "Ensure weapon enchant is applied if you've selected Improved Flametongue Weapon." );
    precombat->add_action( "potion" );
    precombat->add_action( "stormkeeper" );
    precombat->add_action( "lightning_shield" );
    precombat->add_action( "thunderstrike_ward" );
    precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom.enabled+25*talent.primordial_capacity.enabled,op=set" );
    precombat->add_action( "variable,name=spymaster_in_1st,value=trinket.1.is.spymasters_web" );
    precombat->add_action( "variable,name=spymaster_in_2nd,value=trinket.2.is.spymasters_web" );
	  
    // "Default" APL controlling logic flow to specialized sub-APLs
    def->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
    def->add_action( "wind_shear", "Interrupt of casts." );
    // Racials
    def->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
    def->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    //def->add_action( "bag_of_tricks,if=!talent.ascendance.enabled|!buff.ascendance.up" );
    def->add_action( "use_item,slot=trinket1,if=!variable.spymaster_in_1st|target.time_to_die<45&cooldown.stormkeeper.remains<5|fight_remains<22" );
    def->add_action( "use_item,slot=trinket2,if=!variable.spymaster_in_2nd|target.time_to_die<45&cooldown.stormkeeper.remains<5|fight_remains<22" );
    def->add_action( "use_item,slot=main_hand" );
    def->add_action( "lightning_shield,if=buff.lightning_shield.down" );
    // def->add_action( "auto_attack" );
    def->add_action( "natures_swiftness" );
    def->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
    def->add_action( "potion" );

    // Pick APL to run
    def->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>2" );
    def->add_action( "run_action_list,name=single_target" );

    // Aoe APL
    aoe->add_action( "fire_elemental,if=!buff.fire_elemental.up" );
    aoe->add_action( "storm_elemental,if=!buff.storm_elemental.up" );
    aoe->add_action( "stormkeeper,if=!buff.stormkeeper.up" );
    aoe->add_action( "totemic_recall,if=cooldown.liquid_magma_totem.remains>15&talent.fire_elemental.enabled", "{Fire} Reset LMT CD as early as possible." );
    aoe->add_action( "liquid_magma_totem" );
    aoe->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=buff.surge_of_power.up|!talent.surge_of_power.enabled|maelstrom<60-5*talent.eye_of_the_storm.enabled",
        "Spread Flame Shock via Primordial Wave using Surge of Power if possible." );
    aoe->add_action( "ancestral_swiftness" );
    aoe->add_action( "flame_shock,target_if=refreshable,if=buff.surge_of_power.up&talent.lightning_rod.enabled&dot.flame_shock.remains<target.time_to_die-16&active_dot.flame_shock<(spell_targets.chain_lightning>?6)&!talent.liquid_magma_totem.enabled",
        "{Lightning} Spread Flame Shock using Surge of Power if LMT is not picked." );
    aoe->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=buff.primordial_wave.up&buff.stormkeeper.up&maelstrom<60-5*talent.eye_of_the_storm.enabled-(8+2*talent.flow_of_power.enabled)*active_dot.flame_shock&spell_targets.chain_lightning>=6&active_dot.flame_shock<6",
        "{Lightning} Cast extra Flame Shock to help getting to next spender for SK+SoP on 6+ targets. Mostly opener." );
    aoe->add_action( "flame_shock,target_if=refreshable,if=talent.fire_elemental.enabled&(buff.surge_of_power.up|!talent.surge_of_power.enabled)&dot.flame_shock.remains<target.time_to_die-5&(active_dot.flame_shock<6|dot.flame_shock.remains>0)",
                     "{Fire} Spread and refresh Flame Shock using Surge of Power (if talented) up to 6." );
    aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=!buff.arc_discharge.up" );
    aoe->add_action( "ascendance",
                     "JUST DO IT! "
                     "https://i.kym-cdn.com/entries/icons/mobile/000/018/147/"
                     "Shia_LaBeouf__Just_Do_It__Motivational_Speech_(Original_Video_by_LaBeouf__R%C3%B6nkk%C3%B6___"
                     "Turner)_0-4_screenshot.jpg" );
    aoe->add_action( "lava_beam,if=active_enemies>=6&buff.surge_of_power.up&buff.ascendance.remains>cast_time",
        "Against 6 targets or more Surge of Power should be used with Lava Beam rather than Lava Burst." );
    aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up",
        "Against 6 targets or more Surge of Power should be used with Chain Lightning rather than Lava Burst." );
    aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&buff.stormkeeper.up&maelstrom<60-5*talent.eye_of_the_storm.enabled&spell_targets.chain_lightning>=6&talent.surge_of_power.enabled",
        "Consume Primordial Wave buff immediately if you have Stormkeeper buff, fighting 6+ enemies and need maelstrom to spend." );
    aoe->add_action(
        "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&(buff.primordial_wave.remains<4|buff.lava_surge.up)", "Cast Lava burst to consume Primordial Wave proc. Wait for Lava Surge proc if possible." );
    aoe->add_action(
        "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements.enabled&talent.fire_elemental.enabled",
        "{Fire} Use Lava Surge proc to buff <anything> with Master of the Elements." );
    aoe->add_action(
        "earthquake,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)",
        "Activate Surge of Power if next global is Primordial wave. Respect Echoes of Great Sundering." );
    aoe->add_action(
        "earthquake,target_if=max:debuff.lightning_rod.remains,if=(debuff.lightning_rod.remains=0&talent.lightning_rod.enabled|maelstrom>variable.mael_cap-30)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)",
        "Spend if all Lightning Rods ran out or you are close to overcaping. Respect Echoes of Great Sundering." );
    aoe->add_action(
        "earthquake,if=buff.stormkeeper.up&spell_targets.chain_lightning>=6&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)",
        "Spend to buff your follow-up Stormkeeper with Surge of Power on 6+ targets. Respect Echoes of Great Sundering." );
    aoe->add_action(
        "earthquake,if=(buff.master_of_the_elements.up|spell_targets.chain_lightning>=5)&(buff.fusion_of_elements_nature.up|buff.ascendance.remains>9|!buff.ascendance.up)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)&talent.fire_elemental.enabled",
        "{Fire} Spend if you have Master of the elements buff or fighting 5+ enemies. Bank maelstrom during the end of Ascendance. Respect Echoes of Great Sundering." );
    aoe->add_action(
        "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_eb.up&(!buff.maelstrom_surge.up&set_bonus.tww1_4pc|maelstrom>variable.mael_cap-30)",
        "Use the talents you selected. Spread Lightning Rod to as many targets as possible." );
    aoe->add_action(
        "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_es.up&(!buff.maelstrom_surge.up&set_bonus.tww1_4pc|maelstrom>variable.mael_cap-30)",
        "Use the talents you selected. Spread Lightning Rod to as many targets as possible." );
    aoe->add_action( "icefury,if=talent.fusion_of_elements.enabled&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury for Fusion f Elements proc.");
    aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&!buff.ascendance.up&talent.fire_elemental.enabled",
        "{Fire} Proc Master of the Elements outside Ascendance." );
    aoe->add_action( "lava_beam,if=buff.stormkeeper.up&(buff.surge_of_power.up|spell_targets.lava_beam<6)", "Stormkeeper is strong and should be used." );
    aoe->add_action( "chain_lightning,if=buff.stormkeeper.up&(buff.surge_of_power.up|spell_targets.chain_lightning<6)", "Stormkeeper is strong and should be used." );
    aoe->add_action( "lava_beam,if=buff.power_of_the_maelstrom.up&buff.ascendance.remains>cast_time&!buff.stormkeeper.up", "Power of the Maelstrom is strong and should be used." );
    aoe->add_action( "chain_lightning,if=buff.power_of_the_maelstrom.up&!buff.stormkeeper.up", "Power of the Maelstrom is strong and should be used." );
    aoe->add_action( "lava_beam,if=(buff.master_of_the_elements.up&spell_targets.lava_beam>=4|spell_targets.lava_beam>=5)&buff.ascendance.remains>cast_time&!buff.stormkeeper.up", "Consume Master of the Elements with Lava Beam on 4+ targets. Just spam it over hardcasted Lava Burst on 5+ targets." );
    aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.deeply_rooted_elements.enabled", "Gamble away for Deeply Rooted Elements procs." );
    aoe->add_action( "lava_beam,if=buff.ascendance.remains>cast_time" );
    aoe->add_action( "chain_lightning" );
    aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
    aoe->add_action( "frost_shock,moving=1" );

    // Single target APL
    single_target->add_action( "fire_elemental,if=!buff.fire_elemental.up" );
    single_target->add_action( "storm_elemental,if=!buff.storm_elemental.up" );
    single_target->add_action( "stormkeeper,if=!buff.ascendance.up&!buff.stormkeeper.up", "Just use Stormkeeper." );
    single_target->add_action( "totemic_recall,if=cooldown.liquid_magma_totem.remains>15&spell_targets.chain_lightning>1&talent.fire_elemental.enabled", "{Fire} Reset LMT CD as early as possible." );
    single_target->add_action( "liquid_magma_totem,if=!buff.ascendance.up&(talent.fire_elemental.enabled|spell_targets.chain_lightning>1)", "Use LMT outside Ascendance in fire builds and on 2 targets for lightning." );
    single_target->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains", "Use Primordial Wave as much as possible." );
    single_target->add_action( "ancestral_swiftness" );
    single_target->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=active_enemies=1&(dot.flame_shock.remains<2|active_dot.flame_shock=0)&(dot.flame_shock.remains<cooldown.primordial_wave.remains|!talent.primordial_wave.enabled)&(dot.flame_shock.remains<cooldown.liquid_magma_totem.remains|!talent.liquid_magma_totem.enabled)&!buff.surge_of_power.up&talent.fire_elemental.enabled", "{Fire} Manually refresh Flame shock if better options are not available." );
    single_target->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=active_dot.flame_shock<active_enemies&spell_targets.chain_lightning>1&(talent.deeply_rooted_elements.enabled|talent.ascendance.enabled|talent.primordial_wave.enabled|talent.searing_flames.enabled|talent.magma_chamber.enabled)&(!buff.surge_of_power.up&buff.stormkeeper.up|!talent.surge_of_power.enabled|cooldown.ascendance.remains=0)",
      "Use Flame shock without Surge of Power if you can't spread it with SoP because it is going to be used on Stormkeeper or Surge of Power is not talented." );
    single_target->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=spell_targets.chain_lightning>1&(talent.deeply_rooted_elements.enabled|talent.ascendance.enabled|talent.primordial_wave.enabled|talent.searing_flames.enabled|talent.magma_chamber.enabled)&(buff.surge_of_power.up&!buff.stormkeeper.up|!talent.surge_of_power.enabled)&dot.flame_shock.remains<6,cycle_targets=1", "Spread Flame Shock to multiple targets only if talents were selected that benefit from it." );
    single_target->add_action( "tempest" );
    single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up", "Stormkeeper is strong and should be used." );
    single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.stormkeeper.up&!buff.master_of_the_elements.up&!talent.surge_of_power.enabled&talent.master_of_the_elements.enabled", "Buff stormkeeper with MotE when not using Surge of Power." );
    single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&!talent.surge_of_power.enabled&(buff.master_of_the_elements.up|!talent.master_of_the_elements.enabled)", "Buff Stormkeeper with at least something if you can." );
    single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up&!buff.ascendance.up&talent.echo_chamber.enabled", "Surge of Power is strong and should be used." );
    single_target->add_action( "ascendance,if=cooldown.lava_burst.charges_fractional<1.0" );
    single_target->add_action( "lava_burst,if=cooldown_react&buff.lava_surge.up&talent.fire_elemental.enabled", "{Fire} Lava Surge is neat. Utilize it." );
    single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up", "Consume Primordial wave buff." );
    single_target->add_action( "earthquake,if=buff.master_of_the_elements.up&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|spell_targets.chain_lightning>1&!talent.echoes_of_great_sundering.enabled&!talent.elemental_blast.enabled)&(buff.fusion_of_elements_nature.up|maelstrom>variable.mael_cap-15|buff.ascendance.remains>9|!buff.ascendance.up)&talent.fire_elemental.enabled", 
			"{Fire} Spend if you have MotE buff and: not in Ascendance OR Ascendance gona last so long you will need to spend anyway OR nature fusion buff up OR close to maelstrom cap. Respect Echoes of Great Sundering." );
    single_target->add_action( "elemental_blast,if=buff.master_of_the_elements.up&(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up|maelstrom>variable.mael_cap-15|buff.ascendance.remains>6|!buff.ascendance.up)&talent.fire_elemental.enabled", 
			"{Fire} Spend if you have MotE buff and: not in Ascendance OR Ascendance gona last so long you will need to spend anyway OR any fusion buff up OR close to maelstrom cap." );
    single_target->add_action( "earth_shock,if=buff.master_of_the_elements.up&(buff.fusion_of_elements_nature.up|maelstrom>variable.mael_cap-15|buff.ascendance.remains>9|!buff.ascendance.up)&talent.fire_elemental.enabled", 
			"{Fire} Spend if you have MotE buff and: not in Ascendance OR Ascendance gona last so long you will need to spend anyway OR nature fusion buff up OR close to maelstrom cap." );
    single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|spell_targets.chain_lightning>1&!talent.echoes_of_great_sundering.enabled&!talent.elemental_blast.enabled)&(buff.master_of_the_elements.up&cooldown.stormkeeper.remains>10|maelstrom>variable.mael_cap-15|buff.stormkeeper.up)&talent.storm_elemental.enabled", 
			"{Lightning} Spend if you have Master of the Elements buff and Stormkeeper is not coming up soon OR Stormkeeper is active OR you are close to maelstrom cap. Respect Echoes of Great Sundering." );
    single_target->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=(buff.master_of_the_elements.up&cooldown.stormkeeper.remains>10|maelstrom>variable.mael_cap-15|buff.stormkeeper.up)&talent.storm_elemental.enabled", 
			"{Lightning} Spend if you have Master of the Elements buff and Stormkeeper is not coming up soon OR Stormkeeper is active OR you are close to maelstrom cap. Spread Lightning Rod to as many targets as possible." );
    single_target->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=(buff.master_of_the_elements.up&cooldown.stormkeeper.remains>10|maelstrom>variable.mael_cap-15|buff.stormkeeper.up)&talent.storm_elemental.enabled", 
			"{Lightning} Spend if you have Master of the Elements buff and Stormkeeper is not coming up soon OR Stormkeeper is active OR you are close to maelstrom cap. Spread Lightning Rod to as many targets as possible." );
    single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&buff.icefury.stack=2&(talent.fusion_of_elements.enabled|!buff.ascendance.up)", "Don't waste Icefury stacks even during Ascendance." );
    single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.ascendance.up", "Spam Lava burst in Ascendance." );
    single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&talent.fire_elemental.enabled", "{Fire} Buff your next <anything> with MotE." );
    single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.stormkeeper.up&(buff.lava_surge.up|time<10)", "Spend all Lava Burst charges in opener to get one Stormkeeper buffed with Surge of Power. Lava Surge can be used as emergency generator in combat to help with buffing Stormkeeper." );
    single_target->add_action( "earthquake,target_if=max:debuff.lightning_rod.remains,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|spell_targets.chain_lightning>1&!talent.echoes_of_great_sundering.enabled&!talent.elemental_blast.enabled)&(maelstrom>variable.mael_cap-15|debuff.lightning_rod.remains<gcd|fight_remains<5)",
        "Spend if close to overcaping or all Lightning Rods ran out. Respect Echoes of Great Sundering." );
    single_target->add_action( "elemental_blast,target_if=max:debuff.lightning_rod.remains,if=maelstrom>variable.mael_cap-15|debuff.lightning_rod.remains<gcd|fight_remains<5", 
        "Spend if close to overcaping or all Lightning Rods ran out." );
    single_target->add_action( "earth_shock,target_if=max:debuff.lightning_rod.remains,if=maelstrom>variable.mael_cap-15|debuff.lightning_rod.remains<gcd|fight_remains<5", 
        "Spend if close to overcaping or all Lightning Rods ran out." );
    single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
    single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury if you won't overwrite Fusion of Elements buffs." );
    single_target->add_action( "frost_shock,if=buff.icefury_dmg.up&(spell_targets.chain_lightning=1|buff.stormkeeper.up)&talent.surge_of_power.enabled", 
        "Use Icefury-buffed Frost Shock against 1 target or if you need to generate for SoP buff on Stormkeeper." );
    single_target->add_action( "chain_lightning,if=buff.power_of_the_maelstrom.up&spell_targets.chain_lightning>1&!buff.stormkeeper.up", "Utilize the Power of the Maelstrom buff." );
    single_target->add_action( "lightning_bolt,if=buff.power_of_the_maelstrom.up&!buff.stormkeeper.up", "Utilize the Power of the Maelstrom buff." );
    single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.deeply_rooted_elements.enabled", "Fish for DRE procs." );
    single_target->add_action( "chain_lightning,if=spell_targets.chain_lightning>1", "Casting Chain Lightning at two targets is more efficient than Lightning Bolt." );
    single_target->add_action( "lightning_bolt", "Filler spell. Always available. Always the bottom line." );
    single_target->add_action( "flame_shock,moving=1,target_if=refreshable" );
    single_target->add_action( "flame_shock,moving=1,if=movement.distance>6" );
    single_target->add_action( "frost_shock,moving=1", "Frost Shock is our movement filler." );
  }

}

// shaman_t::init_action_list_enhancement ===================================

void shaman_t::init_action_list_enhancement()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default" );
  action_priority_list_t* single    = get_action_priority_list( "single", "Single target action priority list" );
  action_priority_list_t* aoe       = get_action_priority_list( "aoe", "Multi target action priority list" );
  action_priority_list_t* funnel    = get_action_priority_list( "funnel", "Funnel action priority list");

  // action_priority_list_t* cds              = get_action_priority_list( "cds" );

  // Consumables
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );

  // Self-buffs
  precombat->add_action( "windfury_weapon" );
  precombat->add_action( "flametongue_weapon" );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "variable,name=trinket1_is_weird,value=trinket.1.is.algethar_puzzle_box|trinket.1.is.manic_grieftorch|trinket.1.is.elementium_pocket_anvil|trinket.1.is.beacon_to_the_beyond" );
  precombat->add_action( "variable,name=trinket2_is_weird,value=trinket.2.is.algethar_puzzle_box|trinket.2.is.manic_grieftorch|trinket.2.is.elementium_pocket_anvil|trinket.2.is.beacon_to_the_beyond" );
  precombat->add_action( "variable,name=min_talented_cd_remains,value=((cooldown.feral_spirit.remains%(4*talent.witch_doctors_ancestry.enabled))+1000*!talent.feral_spirit.enabled)>?(cooldown.doom_winds.remains+1000*!talent.doom_winds.enabled)>?(cooldown.ascendance.remains+1000*!talent.ascendance.enabled)" );
  precombat->add_action( "variable,name=target_nature_mod,value=(1+debuff.chaos_brand.up*debuff.chaos_brand.value)*(1+(debuff.hunters_mark.up*target.health.pct>=80)*debuff.hunters_mark.value)" );
  precombat->add_action( "variable,name=expected_lb_funnel,value=action.lightning_bolt.damage*(1+debuff.lightning_rod.up*variable.target_nature_mod*(1+buff.primordial_wave.up*active_dot.flame_shock*buff.primordial_wave.value)*debuff.lightning_rod.value)" );
  precombat->add_action( "variable,name=expected_cl_funnel,value=action.chain_lightning.damage*(1+debuff.lightning_rod.up*variable.target_nature_mod*(active_enemies>?(3+2*talent.crashing_storms.enabled))*debuff.lightning_rod.value)" );

  // Snapshot stats
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // All Shamans Bloodlust by default
  def->add_action( this, "Bloodlust", "line_cd=600" );

  // In-combat potion
  def->add_action( "potion,if=(buff.ascendance.up|buff.feral_spirit.up|buff.doom_winds.up|(fight_remains%%300<=30)|(!talent.ascendance.enabled&!talent.feral_spirit.enabled&!talent.doom_winds.enabled))" );

  // "Default" APL controlling logic flow to specialized sub-APLs
  def->add_action( this, "Wind Shear", "", "Interrupt of casts." );
  // Turn on auto-attack first thing
  def->add_action( "auto_attack" );

  //_Use_items
    def->add_action( "use_item,name=elementium_pocket_anvil,use_off_gcd=1" );
    def->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=(!buff.ascendance.up&!buff.feral_spirit.up&!buff.doom_winds.up)|(talent.ascendance.enabled&(cooldown.ascendance.remains<2*action.stormstrike.gcd))|(fight_remains%%180<=30)" );
    def->add_action( "use_item,slot=trinket1,if=!variable.trinket1_is_weird&trinket.1.has_use_buff&(buff.ascendance.up|buff.feral_spirit.up|buff.doom_winds.up|(fight_remains%%trinket.1.cooldown.duration<=trinket.1.buff.any.duration)|(variable.min_talented_cd_remains>=trinket.1.cooldown.duration)|(!talent.ascendance.enabled&!talent.feral_spirit.enabled&!talent.doom_winds.enabled))" );
    def->add_action( "use_item,slot=trinket2,if=!variable.trinket2_is_weird&trinket.2.has_use_buff&(buff.ascendance.up|buff.feral_spirit.up|buff.doom_winds.up|(fight_remains%%trinket.2.cooldown.duration<=trinket.2.buff.any.duration)|(variable.min_talented_cd_remains>=trinket.2.cooldown.duration)|(!talent.ascendance.enabled&!talent.feral_spirit.enabled&!talent.doom_winds.enabled))" );
    def->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=(!buff.ascendance.up&!buff.feral_spirit.up&!buff.doom_winds.up)|(fight_remains%%150<=5)" );
    def->add_action( "use_item,name=manic_grieftorch,use_off_gcd=1,if=(!buff.ascendance.up&!buff.feral_spirit.up&!buff.doom_winds.up)|(fight_remains%%120<=5)" );
    def->add_action( "use_item,slot=trinket1,if=!variable.trinket1_is_weird&!trinket.1.has_use_buff" );
    def->add_action( "use_item,slot=trinket2,if=!variable.trinket2_is_weird&!trinket.2.has_use_buff" );

    //_Racials
    def->add_action( "blood_fury,if=(buff.ascendance.up|buff.feral_spirit.up|buff.doom_winds.up|(fight_remains%%action.blood_fury.cooldown<=action.blood_fury.duration)|(variable.min_talented_cd_remains>=action.blood_fury.cooldown)|(!talent.ascendance.enabled&!talent.feral_spirit.enabled&!talent.doom_winds.enabled))" );
    def->add_action( "berserking,if=(buff.ascendance.up|buff.feral_spirit.up|buff.doom_winds.up|(fight_remains%%action.berserking.cooldown<=action.berserking.duration)|(variable.min_talented_cd_remains>=action.berserking.cooldown)|(!talent.ascendance.enabled&!talent.feral_spirit.enabled&!talent.doom_winds.enabled))" );
    def->add_action( "fireblood,if=(buff.ascendance.up|buff.feral_spirit.up|buff.doom_winds.up|(fight_remains%%action.fireblood.cooldown<=action.fireblood.duration)|(variable.min_talented_cd_remains>=action.fireblood.cooldown)|(!talent.ascendance.enabled&!talent.feral_spirit.enabled&!talent.doom_winds.enabled))" );
    def->add_action( "ancestral_call,if=(buff.ascendance.up|buff.feral_spirit.up|buff.doom_winds.up|(fight_remains%%action.ancestral_call.cooldown<=action.ancestral_call.duration)|(variable.min_talented_cd_remains>=action.ancestral_call.cooldown)|(!talent.ascendance.enabled&!talent.feral_spirit.enabled&!talent.doom_winds.enabled))" );

    //_Cooldowns
    def->add_action( "invoke_external_buff,name=power_infusion,if=(buff.ascendance.up|buff.feral_spirit.up|buff.doom_winds.up|(fight_remains%%120<=20)|(variable.min_talented_cd_remains>=120)|(!talent.ascendance.enabled&!talent.feral_spirit.enabled&!talent.doom_winds.enabled))" );
    def->add_action( "primordial_wave,if=set_bonus.tier31_2pc&(raid_event.adds.in>(action.primordial_wave.cooldown%(1+set_bonus.tier31_4pc))|raid_event.adds.in<6)" );
    def->add_action( "feral_spirit,if=talent.elemental_spirits.enabled|(talent.alpha_wolf.enabled&active_enemies>1)" );
    def->add_action( "surging_totem" );
    def->add_action( "ascendance,if=dot.flame_shock.ticking&((ti_lightning_bolt&active_enemies=1&raid_event.adds.in>=action.ascendance.cooldown%2)|(ti_chain_lightning&active_enemies>1))" );

    def->add_action( "call_action_list,name=single,if=active_enemies=1" );
    def->add_action( "call_action_list,name=aoe,if=active_enemies>1&(rotation.standard|rotation.simple)" );
    def->add_action( "call_action_list,name=funnel,if=active_enemies>1&rotation.funnel" );

    single->add_action( "windstrike,if=talent.thorims_invocation.enabled&buff.maelstrom_weapon.stack>1&ti_lightning_bolt&!talent.elemental_spirits.enabled" );
    single->add_action( "feral_spirit" );
    single->add_action( "tempest,if=buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack|(buff.maelstrom_weapon.stack>=5&(tempest_mael_count>30|buff.awakening_storms.stack=2))" );
    single->add_action( "doom_winds,if=raid_event.adds.in>=action.doom_winds.cooldown" );
    single->add_action( "windstrike,if=talent.thorims_invocation.enabled&buff.maelstrom_weapon.stack>1&ti_lightning_bolt" );
    single->add_action( "sundering,if=buff.ascendance.up&pet.surging_totem.active&talent.earthsurge.enabled" );
    single->add_action( "primordial_wave,if=!dot.flame_shock.ticking&talent.lashing_flames.enabled&(raid_event.adds.in>(action.primordial_wave.cooldown%(1+set_bonus.tier31_4pc))|raid_event.adds.in<6)" );
    single->add_action( "flame_shock,if=!ticking&talent.lashing_flames.enabled" );
    single->add_action( "elemental_blast,if=buff.maelstrom_weapon.stack>=5&talent.elemental_spirits.enabled&feral_spirit.active>=4" );
    single->add_action( "lightning_bolt,if=talent.supercharge.enabled&buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack" );
    single->add_action( "sundering,if=set_bonus.tier30_2pc&raid_event.adds.in>=action.sundering.cooldown" );
    single->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack>=5&buff.crackling_thunder.down&buff.ascendance.up&ti_chain_lightning&(buff.ascendance.remains>(cooldown.strike.remains+gcd))" );
    single->add_action( "stormstrike,if=!talent.elemental_spirits.enabled&(buff.doom_winds.up|talent.deeply_rooted_elements.enabled|(talent.stormblast.enabled&buff.stormbringer.up))" );
    single->add_action( "lava_lash,if=buff.hot_hand.up" );
    single->add_action( "elemental_blast,if=buff.maelstrom_weapon.stack>=5&charges=max_charges" );
    single->add_action( "tempest,if=buff.maelstrom_weapon.stack>=8" );
    single->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack>=8&buff.primordial_wave.up&raid_event.adds.in>buff.primordial_wave.remains&(!buff.splintered_elements.up|fight_remains<=12)" );
    single->add_action( "chain_lightning,if=buff.maelstrom_weapon.stack>=8&buff.crackling_thunder.up&talent.elemental_spirits.enabled" );
    single->add_action( "elemental_blast,if=buff.maelstrom_weapon.stack>=8&(feral_spirit.active>=2|!talent.elemental_spirits.enabled)" );
    single->add_action( "lava_burst,if=!talent.thorims_invocation.enabled&buff.maelstrom_weapon.stack>=5" );
    single->add_action( "lightning_bolt,if=((buff.maelstrom_weapon.stack>=8)|(talent.static_accumulation.enabled&buff.maelstrom_weapon.stack>=5))&buff.primordial_wave.down" );
    single->add_action( "crash_lightning,if=talent.alpha_wolf.enabled&feral_spirit.active&alpha_wolf_min_remains=0" );
    single->add_action( "primordial_wave,if=raid_event.adds.in>(action.primordial_wave.cooldown%(1+set_bonus.tier31_4pc))|raid_event.adds.in<6" );
    single->add_action( "stormstrike,if=talent.elemental_spirits.enabled&(buff.doom_winds.up|talent.deeply_rooted_elements.enabled|(talent.stormblast.enabled&buff.stormbringer.up))" );
    single->add_action( "flame_shock,if=!ticking" );
    single->add_action( "windstrike,if=(talent.totemic_rebound.enabled&(time-(action.stormstrike.last_used<?action.windstrike.last_used))>=3.5)|(talent.awakening_storms.enabled&(time-(action.stormstrike.last_used<?action.windstrike.last_used<?action.lightning_bolt.last_used<?action.tempest.last_used<?action.chain_lightning.last_used))>=3.5)" );
    single->add_action( "stormstrike,if=(talent.totemic_rebound.enabled&(time-(action.stormstrike.last_used<?action.windstrike.last_used))>=3.5)|(talent.awakening_storms.enabled&(time-(action.stormstrike.last_used<?action.windstrike.last_used<?action.lightning_bolt.last_used<?action.tempest.last_used<?action.chain_lightning.last_used))>=3.5)" );
    single->add_action( "lava_lash,if=talent.lively_totems.enabled&(time-action.lava_lash.last_used>=3.5)" );
    single->add_action( "ice_strike,if=talent.elemental_assault.enabled&talent.swirling_maelstrom.enabled" );
    single->add_action( "lava_lash,if=talent.lashing_flames.enabled" );
    single->add_action( "ice_strike,if=!buff.ice_strike.up" );
    single->add_action( "frost_shock,if=buff.hailstorm.up" );
    single->add_action( "crash_lightning,if=talent.converging_storms.enabled" );
    single->add_action( "lava_lash" );
    single->add_action( "ice_strike" );
    single->add_action( "windstrike" );
    single->add_action( "stormstrike" );
    single->add_action( "sundering,if=raid_event.adds.in>=action.sundering.cooldown" );
    single->add_action( "tempest,if=buff.maelstrom_weapon.stack>=5" );
    single->add_action( "lightning_bolt,if=talent.hailstorm.enabled&buff.maelstrom_weapon.stack>=5&buff.primordial_wave.down" );
    single->add_action( "frost_shock" );
    single->add_action( "crash_lightning" );
    single->add_action( "fire_nova,if=active_dot.flame_shock" );
    single->add_action( "earth_elemental" );
    single->add_action( "flame_shock" );
    single->add_action( "chain_lightning,if=buff.maelstrom_weapon.stack>=5&buff.crackling_thunder.up&talent.elemental_spirits.enabled" );
    single->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack>=5&buff.primordial_wave.down" );

    aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack|(buff.maelstrom_weapon.stack>=5&(tempest_mael_count>30|buff.awakening_storms.stack=2))" );
    aoe->add_action( "windstrike,target_if=min:debuff.lightning_rod.remains,if=talent.thorims_invocation.enabled&buff.maelstrom_weapon.stack>1&ti_chain_lightning" );
    aoe->add_action( "crash_lightning,if=talent.crashing_storms.enabled&((talent.unruly_winds.enabled&active_enemies>=10)|active_enemies>=15)" );
    aoe->add_action( "lightning_bolt,target_if=min:debuff.lightning_rod.remains,if=(!talent.tempest.enabled|(tempest_mael_count<=10&buff.awakening_storms.stack<=1))&((active_dot.flame_shock=active_enemies|active_dot.flame_shock=6)&buff.primordial_wave.up&buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack&(!buff.splintered_elements.up|fight_remains<=12|raid_event.adds.remains<=gcd))" );
    aoe->add_action( "lava_lash,if=talent.molten_assault.enabled&(talent.primordial_wave.enabled|talent.fire_nova.enabled)&dot.flame_shock.ticking&(active_dot.flame_shock<active_enemies)&active_dot.flame_shock<6" );
    aoe->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=!buff.primordial_wave.up" );
    aoe->add_action( "chain_lightning,target_if=min:debuff.lightning_rod.remains,if=buff.arc_discharge.up&buff.maelstrom_weapon.stack>=5" );
    aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=(!talent.elemental_spirits.enabled|(talent.elemental_spirits.enabled&(charges=max_charges|feral_spirit.active>=2)))&buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack&(!talent.crashing_storms.enabled|active_enemies<=3)" );
    aoe->add_action( "chain_lightning,target_if=min:debuff.lightning_rod.remains,if=buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack" );
    aoe->add_action( "feral_spirit" );
    aoe->add_action( "doom_winds" );
    aoe->add_action( "crash_lightning,if=buff.doom_winds.up|!buff.crash_lightning.up|(talent.alpha_wolf.enabled&feral_spirit.active&alpha_wolf_min_remains=0)" );
    aoe->add_action( "sundering,if=buff.doom_winds.up|set_bonus.tier30_2pc|talent.earthsurge.enabled" );
    aoe->add_action( "fire_nova,if=active_dot.flame_shock=6|(active_dot.flame_shock>=4&active_dot.flame_shock=active_enemies)" );
    aoe->add_action( "lava_lash,target_if=min:debuff.lashing_flames.remains,if=talent.lashing_flames.enabled" );
    aoe->add_action( "lava_lash,if=talent.molten_assault.enabled&dot.flame_shock.ticking");
    aoe->add_action( "ice_strike,if=talent.hailstorm.enabled&!buff.ice_strike.up" );
    aoe->add_action( "frost_shock,if=talent.hailstorm.enabled&buff.hailstorm.up" );
    aoe->add_action( "sundering" );
    aoe->add_action( "flame_shock,if=talent.molten_assault.enabled&!ticking" );
    aoe->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=(talent.fire_nova.enabled|talent.primordial_wave.enabled)&(active_dot.flame_shock<active_enemies)&active_dot.flame_shock<6" );
    aoe->add_action( "fire_nova,if=active_dot.flame_shock>=3" );
    aoe->add_action( "stormstrike,if=buff.crash_lightning.up&(talent.deeply_rooted_elements.enabled|buff.converging_storms.stack=buff.converging_storms.max_stack)" );
    aoe->add_action( "crash_lightning,if=talent.crashing_storms.enabled&buff.cl_crash_lightning.up&active_enemies>=4" );
    aoe->add_action( "windstrike" );
    aoe->add_action( "stormstrike" );
    aoe->add_action( "ice_strike" );
    aoe->add_action( "lava_lash" );
    aoe->add_action( "crash_lightning" );
    aoe->add_action( "fire_nova,if=active_dot.flame_shock>=2" );
    aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=(!talent.elemental_spirits.enabled|(talent.elemental_spirits.enabled&(charges=max_charges|feral_spirit.active>=2)))&buff.maelstrom_weapon.stack>=5&(!talent.crashing_storms.enabled|active_enemies<=3)" );
    aoe->add_action( "chain_lightning,target_if=min:debuff.lightning_rod.remains,if=buff.maelstrom_weapon.stack>=5" );
    aoe->add_action( "flame_shock,if=!ticking" );
    aoe->add_action( "frost_shock,if=!talent.hailstorm.enabled" );

    funnel->add_action( "ascendance" );
    funnel->add_action( "windstrike,if=(talent.thorims_invocation.enabled&buff.maelstrom_weapon.stack>1)|buff.converging_storms.stack=buff.converging_storms.max_stack" );
    funnel->add_action( "tempest,if=buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack|(buff.maelstrom_weapon.stack>=5&(tempest_mael_count>30|buff.awakening_storms.stack=2))" );
    funnel->add_action( "lightning_bolt,if=(active_dot.flame_shock=active_enemies|active_dot.flame_shock=6)&buff.primordial_wave.up&buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack&(!buff.splintered_elements.up|fight_remains<=12|raid_event.adds.remains<=gcd)" );
    funnel->add_action( "elemental_blast,if=buff.maelstrom_weapon.stack>=5&talent.elemental_spirits.enabled&feral_spirit.active>=4" );
    funnel->add_action( "lightning_bolt,if=talent.supercharge.enabled&buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack&(variable.expected_lb_funnel>variable.expected_cl_funnel)" );
    funnel->add_action( "chain_lightning,if=(talent.supercharge.enabled&buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack)|buff.arc_discharge.up&buff.maelstrom_weapon.stack>=5" );
    funnel->add_action( "lava_lash,if=(talent.molten_assault.enabled&dot.flame_shock.ticking&(active_dot.flame_shock<active_enemies)&active_dot.flame_shock<6)|(talent.ashen_catalyst.enabled&buff.ashen_catalyst.stack=buff.ashen_catalyst.max_stack)" );
    funnel->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=!buff.primordial_wave.up" );
    funnel->add_action( "elemental_blast,if=(!talent.elemental_spirits.enabled|(talent.elemental_spirits.enabled&(charges=max_charges|buff.feral_spirit.up)))&buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack" );
    funnel->add_action( "feral_spirit" );
    funnel->add_action( "doom_winds" );
    funnel->add_action( "stormstrike,if=buff.converging_storms.stack=buff.converging_storms.max_stack" );
    funnel->add_action( "chain_lightning,if=buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack&buff.crackling_thunder.up" );
    funnel->add_action( "lava_burst,if=(buff.molten_weapon.stack+buff.volcanic_strength.up>buff.crackling_surge.stack)&buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack" );
    funnel->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack&(variable.expected_lb_funnel>variable.expected_cl_funnel)" );
    funnel->add_action( "chain_lightning,if=buff.maelstrom_weapon.stack=buff.maelstrom_weapon.max_stack" );
    funnel->add_action( "crash_lightning,if=buff.doom_winds.up|!buff.crash_lightning.up|(talent.alpha_wolf.enabled&feral_spirit.active&alpha_wolf_min_remains=0)|(talent.converging_storms.enabled&buff.converging_storms.stack<buff.converging_storms.max_stack)" );
    funnel->add_action( "sundering,if=buff.doom_winds.up|set_bonus.tier30_2pc|talent.earthsurge.enabled" );
    funnel->add_action( "fire_nova,if=active_dot.flame_shock=6|(active_dot.flame_shock>=4&active_dot.flame_shock=active_enemies)" );
    funnel->add_action( "ice_strike,if=talent.hailstorm.enabled&!buff.ice_strike.up" );
    funnel->add_action( "frost_shock,if=talent.hailstorm.enabled&buff.hailstorm.up" );
    funnel->add_action( "sundering" );
    funnel->add_action( "flame_shock,if=talent.molten_assault.enabled&!ticking" );
    funnel->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=(talent.fire_nova.enabled|talent.primordial_wave.enabled)&(active_dot.flame_shock<active_enemies)&active_dot.flame_shock<6" );
    funnel->add_action( "fire_nova,if=active_dot.flame_shock>=3" );
    funnel->add_action( "stormstrike,if=buff.crash_lightning.up&talent.deeply_rooted_elements.enabled" );
    funnel->add_action( "crash_lightning,if=talent.crashing_storms.enabled&buff.cl_crash_lightning.up&active_enemies>=4" );
    funnel->add_action( "windstrike" );
    funnel->add_action( "stormstrike" );
    funnel->add_action( "ice_strike" );
    funnel->add_action( "lava_lash" );
    funnel->add_action( "crash_lightning" );
    funnel->add_action( "fire_nova,if=active_dot.flame_shock>=2" );
    funnel->add_action( "elemental_blast,if=(!talent.elemental_spirits.enabled|(talent.elemental_spirits.enabled&(charges=max_charges|buff.feral_spirit.up)))&buff.maelstrom_weapon.stack>=5" );
    funnel->add_action( "lava_burst,if=(buff.molten_weapon.stack+buff.volcanic_strength.up>buff.crackling_surge.stack)&buff.maelstrom_weapon.stack>=5" );
    funnel->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack>=5&(variable.expected_lb_funnel>variable.expected_cl_funnel)" );
    funnel->add_action( "chain_lightning,if=buff.maelstrom_weapon.stack>=5" );
    funnel->add_action( "flame_shock,if=!ticking" );
    funnel->add_action( "frost_shock,if=!talent.hailstorm.enabled" );

  // def->add_action( "call_action_list,name=opener" );
}
// shaman_t::init_action_list_restoration ===================================

void shaman_t::init_action_list_restoration_dps()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default" );

  // Grabs whatever Elemental is using
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( this, "Earth Elemental" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "potion" );

  // Default APL
  def->add_action( this, "Spiritwalker's Grace", "moving=1,if=movement.distance>6" );
  def->add_action( this, "Wind Shear", "", "Interrupt of casts." );
  def->add_action( "potion" );
  def->add_action( "use_items" );
  def->add_action( this, "Flame Shock", "if=!ticking" );
  def->add_action( this, "Earth Elemental" );

  // Racials
  def->add_action( "blood_fury" );
  def->add_action( "berserking" );
  def->add_action( "fireblood" );
  def->add_action( "ancestral_call" );
  def->add_action( "bag_of_tricks" );

  def->add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time&cooldown_react" );
  def->add_action( "primordial_wave" );
  def->add_action( this, "Lightning Bolt", "if=spell_targets.chain_lightning<3" );
  def->add_action( this, "Chain Lightning", "if=spell_targets.chain_lightning>2" );
  def->add_action( this, "Flame Shock", "moving=1" );
  def->add_action( this, "Frost Shock", "moving=1" );
}

// shaman_t::init_actions ===================================================

void shaman_t::init_action_list()
{
  if ( !( primary_role() == ROLE_ATTACK && specialization() == SHAMAN_ENHANCEMENT ) &&
       !( primary_role() == ROLE_SPELL && specialization() == SHAMAN_ELEMENTAL ) &&
       !( primary_role() == ROLE_SPELL && specialization() == SHAMAN_RESTORATION ) )
  {
    if ( !quiet )
      sim->errorf( "Player %s's role (%s) or spec(%s) isn't supported yet.", name(),
                   util::role_type_string( primary_role() ), util::specialization_string( specialization() ) );
    quiet = true;
    return;
  }

  // Restoration isn't supported atm
  if ( !sim->allow_experimental_specializations && specialization() == SHAMAN_RESTORATION &&
       primary_role() == ROLE_HEAL )
  {
    if ( !quiet )
      sim->errorf( "Restoration Shaman healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }

  // After error checks, initialize secondary actions for various things
  windfury_mh = new windfury_attack_t( "windfury_attack", this, find_spell( 25504 ), &( main_hand_weapon ) );
  flametongue = new flametongue_weapon_spell_t( "flametongue_attack", this,
      specialization() == SHAMAN_ENHANCEMENT
      ? &( off_hand_weapon )
      : &( main_hand_weapon ) );

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
      init_action_list_enhancement();
      break;
    case SHAMAN_ELEMENTAL:
      init_action_list_elemental();
      break;
    case SHAMAN_RESTORATION:
      init_action_list_restoration_dps();
      break;
    default:
      break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// shaman_t::moving =========================================================

void shaman_t::moving()
{
  // Spiritwalker's Grace complicates things, as you can cast it while casting
  // anything. So, to model that, if a raid move event comes, we need to check
  // if we can trigger Spiritwalker's Grace. If so, conditionally execute it, to
  // allow the currently executing cast to finish.
  if ( true_level >= 85 )
  {
    action_t* swg = find_action( "spiritwalkers_grace" );

    // We need to bypass swg -> ready() check here, so whip up a special
    // readiness check that only checks for player skill, cooldown and resource
    // availability
    if ( swg && executing && swg->ready() )
    {
      // Shaman executes SWG mid-cast during a movement event, if
      // 1) The profile is casting Lava Burst (without Lava Surge)
      // 2) The profile is casting Chain Lightning
      // 3) The profile is casting Lightning Bolt
      if ( ( executing->id == 51505 ) || ( executing->id == 421 ) || ( executing->id == 403 ) )
      {
        if ( sim->log )
          sim->out_log.printf( "%s spiritwalkers_grace during spell cast, next cast (%s) should finish", name(),
                               executing->name() );
        swg->execute();
      }
    }
    else
    {
      interrupt();
    }

    if ( main_hand_attack )
      main_hand_attack->cancel();
    if ( off_hand_attack )
      off_hand_attack->cancel();
  }
  else
  {
    halt();
  }
}

// shaman_t::matching_gear_multiplier =======================================

double shaman_t::matching_gear_multiplier( attribute_e attr ) const
{
  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
      return attr == ATTR_AGILITY ? constant.mul_matching_gear : 0;
    case SHAMAN_RESTORATION:
      return attr == ATTR_INTELLECT ? constant.mul_matching_gear : 0;
    case SHAMAN_ELEMENTAL:
      return attr == ATTR_INTELLECT ? constant.mul_matching_gear : 0;
    default:
      return 0.0;
  }
}

// shaman_t::composite_spell_crit_chance ===========================================

double shaman_t::composite_spell_crit_chance() const
{
  double m = player_t::composite_spell_crit_chance();

  m += spec.critical_strikes->effectN( 1 ).percent();

  return m;
}

// shaman_t::non_stacking_movement_modifier ========================================

double shaman_t::non_stacking_movement_modifier() const
{
  double ms = player_t::non_stacking_movement_modifier();

  if ( buff.spirit_walk->up() )
    ms = std::max( buff.spirit_walk->data().effectN( 1 ).percent(), ms );

  return ms;
}

// shaman_t::stacking_movement_modifier ============================================

double shaman_t::stacking_movement_modifier() const
{
  double ms = player_t::stacking_movement_modifier();

  if ( buff.ghost_wolf->up() )
  {
    ms *= 1.0 + buff.ghost_wolf->data().effectN( 2 ).percent();
  }

  return ms;
}

// shaman_t::composite_melee_crit_chance ===========================================

double shaman_t::composite_melee_crit_chance() const
{
  double m = player_t::composite_melee_crit_chance();

  m += spec.critical_strikes->effectN( 1 ).percent();

  return m;
}

// shaman_t::composite_attack_speed =========================================

double shaman_t::composite_melee_auto_attack_speed() const
{
  double speed = player_t::composite_melee_auto_attack_speed();

  speed *= 1.0 / ( 1.0 + buff.flurry->value() );

  return speed;
}

// shaman_t::composite_melee_haste =========================================

double shaman_t::composite_melee_haste() const
{
  double haste = player_t::composite_melee_haste();

  if ( buff.splintered_elements->up() )
  {
    haste *= 1.0 / ( 1.0 + talent.splintered_elements->effectN( 1 ).percent() +
                      std::max( buff.splintered_elements->stack() - 1, 0 ) *
                          talent.splintered_elements->effectN( 2 ).percent() );
  }

  if ( talent.preeminence.ok() && buff.ascendance->up() )
  {
    haste *= 1.0 / ( 1.0 + talent.preeminence->effectN( 2 ).percent() );
  }

  return haste;
}

// shaman_t::composite_spell_haste =========================================

double shaman_t::composite_spell_haste() const
{
  double haste = player_t::composite_spell_haste();

  if ( buff.splintered_elements->up() )
  {
    haste *= 1.0 / ( 1.0 + talent.splintered_elements->effectN( 1 ).percent() +
                      std::max( buff.splintered_elements->stack() - 1, 0 ) *
                          talent.splintered_elements->effectN( 2 ).percent() );
  }

  if ( talent.preeminence.ok() && buff.ascendance->up() )
  {
    haste *= 1.0 / ( 1.0 + talent.preeminence->effectN( 2 ).percent() );
  }

  return haste;
}

// shaman_t::composite_player_multiplier ====================================

double shaman_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  m *= 1.0 + buff.elemental_equilibrium->value();

  if ( talent.elemental_weapons.ok() &&
       ( dbc::is_school( school, SCHOOL_FROST ) || dbc::is_school( school, SCHOOL_FIRE ) ||
         dbc::is_school( school, SCHOOL_NATURE ) ) )
  {
    unsigned n_imbues = ( main_hand_weapon.buff_type != 0 ) + ( off_hand_weapon.buff_type != 0 );
    m *= 1.0 + talent.elemental_weapons->effectN( 1 ).percent() / 10.0 * n_imbues;
  }

  if ( dbc::is_school( school, SCHOOL_NATURE ) && buff.lightning_shield->up() &&
       talent.lightning_conduit.ok() )
  {
    m *= 1.0 + talent.lightning_conduit->effectN( 3 ).percent();
  }

  return m;
}

// shaman_t::composite_player_target_multiplier ==============================

double shaman_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  return m;
}

// shaman_t::composite_player_pet_damage_multiplier =========================

double shaman_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  if ( !guardian )
  {
    m *= 1.0 + spec.elemental_shaman->effectN( 3 ).percent();
    m *= 1.0 + spec.elemental_shaman->effectN( 28 ).percent();

    m *= 1.0 + spec.enhancement_shaman->effectN( 3 ).percent();

    //m *= 1.0 + buff.elemental_equilibrium->value();  TODO: check what this was doing here
  }
  else
  {
    m *= 1.0 + spec.elemental_shaman->effectN( 4 ).percent();
    m *= 1.0 + spec.elemental_shaman->effectN( 28 ).percent();

    m *= 1.0 + spec.enhancement_shaman->effectN( 13 ).percent();
  }

  return m;
}

// shaman_t::invalidate_cache ===============================================

void shaman_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_AGILITY:
    case CACHE_STRENGTH:
    case CACHE_ATTACK_POWER:
      if ( specialization() == SHAMAN_ENHANCEMENT )
        player_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    default:
      break;
  }
}

// shaman_t::combat_begin ====================================================

struct rt_event_t : public event_t
{
  shaman_t* player;
  rt_event_t( shaman_t* p, timespan_t delay = timespan_t::min() ) :
    event_t( *p, delay > 0_ms ? delay : p->talent.rolling_thunder->effectN( 2 ).period() ), player( p )
  { }

  const char* name() const override
  { return "rolling_thunder_event"; }

  void trigger_stormkeeper()
  {
    if ( sim().current_time() - player->rt_last_trigger <
         timespan_t::from_seconds( player->talent.rolling_thunder->effectN( 1 ).base_value() ) )
    {
      return;
    }

    if ( player->buff.stormkeeper->check() )
    {
      return;
    }

    player->buff.stormkeeper->trigger( 1 );
    player->rt_last_trigger = sim().current_time();
  }

  void execute() override
  {
    trigger_stormkeeper();
    make_event<rt_event_t>( sim(), player );
  }
};

void shaman_t::combat_begin()
{
  player_t::combat_begin();

  buff.witch_doctors_ancestry->trigger();

  if ( specialization() == SHAMAN_ELEMENTAL && talent.rolling_thunder.ok() )
  {
    make_event<rt_event_t>( *sim, this, rng().range( 1_ms, talent.rolling_thunder->effectN( 2 ).period() ) );
  }
}

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  player_t::reset();

  lava_surge_during_lvb = false;
  sk_during_cast        = false;

  accumulated_ascendance_extension_time = timespan_t::from_seconds( 0.0 );
  ascendance_extension_cap = timespan_t::from_seconds( 0.0 );
  tempest_counter = 0U;

  lotfw_counter = 0U;
  dre_attempts = 0U;
  lava_surge_attempts_normalized         = 0.0;
  action.ti_trigger = nullptr;
  action.totemic_recall_totem = nullptr;

  pet.all_wolves.clear();

  earthen_rage_target = nullptr;
  earthen_rage_event = nullptr;

  if ( specialization() == SHAMAN_ELEMENTAL && talent.rolling_thunder.ok() )
  {
    rt_last_trigger = -timespan_t::from_seconds( talent.rolling_thunder->effectN( 1 ).base_value() );
  }

  assert( active_flame_shock.empty() );
  assert( buff_state_lightning_rod == 0U );
  assert( buff_state_lashing_flames == 0U );
}


// shaman_t::merge ==========================================================

void shaman_t::merge( player_t& other )
{
  player_t::merge( other );

  const shaman_t& s = static_cast<shaman_t&>( other );

  if ( s.mw_source_list.size() > mw_source_list.size() )
  {
    mw_source_list.resize( s.mw_source_list.size() );
  }

  for ( auto i = 0U; i < s.mw_source_list.size(); ++i )
  {
    mw_source_list[ i ].first.merge( s.mw_source_list[ i ].first );
    mw_source_list[ i ].second.merge( s.mw_source_list[ i ].second );
  }

  if ( s.mw_spend_list.size() > mw_spend_list.size() )
  {
    mw_spend_list.resize( s.mw_spend_list.size() );
  }

  for ( auto i = 0U; i < s.mw_spend_list.size(); ++i )
  {
    for ( auto j = 0U; j < s.mw_spend_list[ i ].size(); ++j )
    {
      mw_spend_list[ i ][ j ].merge( s.mw_spend_list[ i ][ j ] );
    }
  }

  if ( talent.deeply_rooted_elements.ok() )
  {
    dre_samples.merge( s.dre_samples );
    dre_uptime_samples.merge( s.dre_uptime_samples );
  }

  lvs_samples.merge( s.lvs_samples );
}

// shaman_t::primary_role ===================================================

role_e shaman_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HYBRID;  // To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == SHAMAN_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_SPELL;
  }

  else if ( specialization() == SHAMAN_ENHANCEMENT )
    return ROLE_ATTACK;

  else if ( specialization() == SHAMAN_ELEMENTAL )
    return ROLE_SPELL;

  return player_t::primary_role();
}

// shaman_t::convert_hybrid_stat ===========================================

stat_e shaman_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
      if ( specialization() == SHAMAN_ENHANCEMENT )
        return STAT_AGILITY;
      else
        return STAT_INTELLECT;
    case STAT_STR_AGI:
      // This is a guess at how AGI/STR gear will work for Resto/Elemental, TODO: confirm
      return STAT_AGILITY;
    case STAT_STR_INT:
      // This is a guess at how STR/INT gear will work for Enhance, TODO: confirm
      // this should probably never come up since shamans can't equip plate, but....
      return STAT_INTELLECT;
    case STAT_SPIRIT:
      if ( specialization() == SHAMAN_RESTORATION )
        return s;
      else
        return STAT_NONE;
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class shaman_report_t : public player_report_extension_t
{
private:
  shaman_t& p;

public:
  shaman_report_t( shaman_t& player ) : p( player )
  { }

  void mw_consumer_stack_header( report::sc_html_stream& os )
  {
    auto columns = std::max( p.buff.maelstrom_weapon->data().max_stacks(),
      as<unsigned>( p.talent.overflowing_maelstrom->effectN( 1 ).base_value() ) ) + 1;

    os << "<table class=\"sc sort\" style=\"float: left;margin-right: 10px;\">\n"
       << "<thead>\n"
       << "<tr>\n";
    os << fmt::format( "<th colspan=\"{}\"><strong>Casts per Maelstrom Weapon Stack Consumed</strong></th>\n", columns + 1 )
       << "</tr>\n"
       << "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n";
    for ( auto col = 0U; col < columns; ++col )
    {
       os << fmt::format( "<th>{}</th>\n", col );
    }
    os << "</tr>\n"
       << "</thead>\n";
  }

  void mw_consumer_stack_contents( report::sc_html_stream& os )
  {
    auto columns = std::max( p.buff.maelstrom_weapon->data().max_stacks(),
      as<unsigned>( p.talent.overflowing_maelstrom->effectN( 1 ).base_value() ) ) + 1;

    int row = 0;
    std::vector<double> row_totals( columns, 0.0 );

    for ( auto i = 0; i < as<int>( p.mw_spend_list.size() ); ++i )
    {
      const auto& ref = p.mw_spend_list[ i ];

      auto action_sum = range::accumulate( ref, 0.0, &simple_sample_data_t::sum ) - ref[ 0 ].sum();
      if ( action_sum == 0.0 )
      {
        continue;
      }

      auto action = range::find_if( p.action_list, [ i ]( const action_t* action ) {
        return action->internal_id == i;
      } );

      os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
      os << fmt::format( "<td class=\"left\">{}</td>", report_decorators::decorated_action( **action ) );

      for ( auto col = 0; col < as<int>( columns ); ++col )
      {
        auto casts = ref[ col ].sum() / ( col > 1 ? as<double>( col ) : 1.0 );

        if ( ref[ col ].sum() == 0.0 )
        {
          os << "<td class=\"left\" style=\"min-width: 5ch;\">&nbsp;</td>\n";
        }
        else
        {
          os << fmt::format( "<td class=\"left\" style=\"min-width: 5ch;\">{:.2f}</td>\n", casts );
        }

        row_totals[ col ] += casts;
      }

      os << "</tr>\n";
    }

    os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" )
       << "<td class=\"left\"><strong>Total</strong>\n";

    auto total_sum = range::accumulate( row_totals, 0.0 );
    range::for_each( row_totals, [ &os, total_sum ]( auto row_sum ) {
      if ( row_sum == 0.0 )
      {
        os << "<td class=\"left\" style=\"min-width: 5ch;\">&nbsp;</td>\n";
      }
      else
      {
        os << fmt::format( "<td class=\"left\" style=\"min-width: 5ch;\"><strong>{:.2f}</strong><br/>({:.2f}%)</td>\n",
          row_sum, 100 * row_sum / total_sum );
      }
    } );

    os << "</tr>\n";
  }

  void mw_consumer_stack_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void mw_consumer_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc sort\" style=\"float: left;margin-right: 10px;\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th colspan=\"3\"><strong>Maelstrom Weapon Consumers</strong></th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n"
       << "<th class=\"toggle-sort\">Actual</th>\n"
       << "<th class=\"toggle-sort\">% Total</th>\n"
       << "</tr>\n"
       << "</thead>\n";
  }

  void mw_consumer_contents( report::sc_html_stream& os )
  {
      int row = 0;
      double total = 0.0;

      range::for_each( p.mw_spend_list,  [ &total ]( const auto& entry ) {
        total = range::accumulate( entry, total, &simple_sample_data_t::sum ) - entry[ 0 ].sum();
      } );

      for ( auto i = 0; i < as<int>( p.mw_spend_list.size() ); ++i )
      {
        const auto& ref = p.mw_spend_list[ i ];

        auto action = range::find_if( p.action_list, [ i ]( const action_t* action ) {
          return action->internal_id == i;
        } );

        auto action_sum = range::accumulate( ref, 0.0, &simple_sample_data_t::sum ) - ref[ 0 ].sum();

        if ( action_sum == 0.0 )
        {
          continue;
        }

        os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
        os << fmt::format( "<td class=\"left\">{}</td>", report_decorators::decorated_action( **action ) );
        os << fmt::format( "<td class=\"left\">{:.1f}</td>", action_sum );
        os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 100.0 * action_sum / total );
        os << "</tr>\n";
      }

      os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
      os << fmt::format( "<td class=\"left\"><strong>Total Spent</strong></td>" );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", total );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 100.0 );
  }

  void mw_consumer_piechart_contents( report::sc_html_stream& os )
  {
    highchart::pie_chart_t mw_cons( highchart::build_id( p, "mw_con" ), *p.sim );
    mw_cons.set_title( "Maelstrom Weapon Consumers" );
    mw_cons.set( "plotOptions.pie.dataLabels.format", "{point.name}: {point.y:.1f}" );

    std::vector<std::pair<action_t*, double>> processed_data;

    for ( size_t i = 0; i < p.mw_spend_list.size(); ++i )
    {
      const auto& entry = p.mw_spend_list[ i ];

      auto sum = range::accumulate( entry, 0.0, &simple_sample_data_t::sum ) - entry[ 0 ].sum();
      if ( sum == 0.0 )
      {
        continue;
      }

      auto action_it = range::find_if( p.action_list, [ i ]( const action_t* action ) {
        return action->internal_id == as<int>( i );
      } );

      processed_data.emplace_back( *action_it, sum );
    }

    range::sort( processed_data, []( const auto& left, const auto& right ) {
      if ( left.second == right.second )
      {
        return left.first->name_str < right.first->name_str;
      }

      return left.second > right.second;
    } );

    range::for_each( processed_data, [ this, &mw_cons ]( const auto& entry ) {
      color::rgb color = color::school_color( entry.first->school );

      js::sc_js_t e;
      e.set( "color", color.str() );
      e.set( "y", util::round( entry.second, p.sim->report_precision ) );
      e.set( "name", report_decorators::decorate_html_string(
          util::encode_html( entry.first->name_str ), color ) );

      mw_cons.add( "series.0.data", e );
    } );

    os << mw_cons.to_target_div();
    p.sim->add_chart_data( mw_cons );
  }

  void mw_consumer_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void mw_generator_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc sort even\" style=\"float: left;margin-right: 10px;\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th colspan=\"5\"><strong>Maelstrom Weapon Sources</strong></th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n"
       << "<th class=\"toggle-sort\">Actual</th>\n"
       << "<th class=\"toggle-sort\">Overflow</th>\n"
       << "<th class=\"toggle-sort\">% Actual</th>\n"
       << "<th class=\"toggle-sort\">% Overflow</th>\n"
       << "</tr>\n"
       << "</thead>\n";
  }

  void mw_generator_piechart_contents( report::sc_html_stream& os )
  {
    highchart::pie_chart_t mw_src( highchart::build_id( p, "mw_src" ), *p.sim );
    mw_src.set_title( "Maelstrom Weapon Sources" );
    mw_src.set( "plotOptions.pie.dataLabels.format", "{point.name}: {point.y:.1f}" );

    double overflow = 0.0;
    std::vector<std::pair<action_t*, double>> processed_data;

    for ( size_t i = 0; i < p.mw_source_list.size(); ++i )
    {
      const auto& entry = p.mw_source_list[ i ];
      overflow += entry.second.sum();

      if ( entry.first.sum() == 0.0 )
      {
        continue;
      }

      auto action_it = range::find_if( p.action_list, [ i ]( const action_t* action ) {
        return action->internal_id == as<int>( i );
      } );

      processed_data.emplace_back( *action_it, entry.first.sum() );
    }

    range::sort( processed_data, []( const auto& left, const auto& right ) {
      if ( left.second == right.second )
      {
        return left.first->name_str < right.first->name_str;
      }

      return left.second > right.second;
    } );

    range::for_each( processed_data, [ this, &mw_src ]( const auto& entry ) {
      color::rgb color = color::school_color( entry.first->school );

      js::sc_js_t e;
      e.set( "color", color.str() );
      e.set( "y", util::round( entry.second, p.sim->report_precision ) );
      e.set( "name", report_decorators::decorate_html_string(
          util::encode_html( entry.first->name_str ), color ) );

      mw_src.add( "series.0.data", e );
    } );

    if ( overflow > 0.0 )
    {
      js::sc_js_t e;
      e.set( "color", color::WHITE.str() );
      e.set( "y", util::round( overflow, p.sim->report_precision ) );
      e.set( "name", "overflow" );
      mw_src.add( "series.0.data", e );
    }

    os << mw_src.to_target_div();
    p.sim->add_chart_data( mw_src );
  }

  void mw_generator_contents( report::sc_html_stream& os )
  {
      int row = 0;
      std::string row_class_str;
      double actual = 0.0, overflow = 0.0;

      range::for_each( p.mw_source_list,  [ &actual, &overflow ]( const auto& entry ) {
        actual += entry.first.sum();
        overflow += entry.second.sum();
      } );

      for ( auto i = 0; i < as<int>( p.mw_source_list.size() ); ++i )
      {
        const auto& ref = p.mw_source_list[ i ];

        if ( ref.first.sum() == 0.0 && ref.second.sum() == 0.0 )
        {
          continue;
        }

        auto action = range::find_if( p.action_list, [ i ]( const action_t* action ) {
          return action->internal_id == i;
        } );

        os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
        os << fmt::format( "<td class=\"left\">{}</td>", report_decorators::decorated_action( **action ) );
        os << fmt::format( "<td class=\"left\">{:.1f}</td>", ref.first.sum() );
        os << fmt::format( "<td class=\"left\">{:.1f}</td>", ref.second.sum() );
        os << fmt::format( "<td class=\"left\">{:.2f}%</td>",
                          100.0 * ref.first.sum() / actual );
        os << fmt::format( "<td class=\"left\">{:.2f}%</td>",
                          100.0 * ref.second.sum() / overflow );
        os << "</tr>\n";
      }

      os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
      os << fmt::format( "<td class=\"left\"><strong>Overflow Stacks</strong></td>" );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", 0.0 );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", overflow );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 0.0 );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 100.0 * overflow / ( actual + overflow ) );

      os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
      os << fmt::format( "<td class=\"left\"><strong>Actual Stacks</strong></td>" );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", actual );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", 0.0 );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 100.0 * actual / ( actual + overflow ) );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 0.0 );
  }

  void mw_generator_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void dre_uptime_distribution_contents( report::sc_html_stream& os )
  {
    highchart::histogram_chart_t chart( highchart::build_id( p, "dre_uptime" ), *p.sim );

    chart.set( "plotOptions.column.color", color::GREY3.str() );
    chart.set( "plotOptions.column.pointStart", std::floor( p.dre_uptime_samples.min() ) );
    chart.set_title( fmt::format( "DRE Iteration Uptime% (min={:.2f}% median={:.2f}% max={:.2f}%)",
                                 p.dre_uptime_samples.min(),
                                 p.dre_uptime_samples.percentile( 0.5 ),
                                 p.dre_uptime_samples.max() ) );
    chart.set( "yAxis.title.text", "# of Iterations" );
    chart.set( "xAxis.title.text", "Uptime%" );
    chart.set( "series.0.name", "# of Iterations" );

    range::for_each( p.dre_uptime_samples.distribution, [ &chart ]( size_t n ) {
      js::sc_js_t e;

      e.set( "y", static_cast<double>( n ) );

      chart.add( "series.0.data", e );
    } );

    os << chart.to_target_div();
    p.sim->add_chart_data( chart );
  }

  void dre_proc_distribution_contents( report::sc_html_stream& os )
  {
    highchart::histogram_chart_t chart( highchart::build_id( p, "dre" ), *p.sim );

    chart.set( "plotOptions.column.color", color::RED.str() );
    chart.set( "plotOptions.column.pointStart", p.options.dre_forced_failures + 1 );
    chart.set_title( fmt::format( "DRE Attempts (min={} median={} max={})", p.dre_samples.min(),
                                 p.dre_samples.percentile( 0.5 ), p.dre_samples.max() ) );
    chart.set( "yAxis.title.text", "# of Triggered Procs" );
    chart.set( "xAxis.title.text", "Proc on Attempt #" );
    chart.set( "series.0.name", "Triggered Procs" );

    range::for_each( p.dre_samples.distribution, [ &chart ]( size_t n ) {
      js::sc_js_t e;

      e.set( "y", static_cast<double>( n ) );

      chart.add( "series.0.data", e );
    } );

    os << chart.to_target_div();
    p.sim->add_chart_data( chart );
  }

  void lvs_proc_distribution_contents( report::sc_html_stream& os )
  {
    highchart::histogram_chart_t chart( highchart::build_id( p, "lvs" ), *p.sim );

    chart.set( "plotOptions.column.color", color::RED.str() );
    chart.set( "plotOptions.column.pointStart", 0 );
    chart.set_title( fmt::format( "LVS Attempts (min={} median={} max={})", p.lvs_samples.min(),
                                  p.lvs_samples.percentile( 0.5 ), p.lvs_samples.max() ) );
    chart.set( "yAxis.title.text", "# of Triggered Procs" );
    chart.set( "xAxis.title.text", "Proc on Attempt #" );
    chart.set( "series.0.name", "Triggered Procs" );

    range::for_each( p.lvs_samples.distribution, [ &chart ]( size_t n ) {
      js::sc_js_t e;

      e.set( "y", static_cast<double>( n ) );

      chart.add( "series.0.data", e );
    } );

    os << chart.to_target_div();
    p.sim->add_chart_data( chart );
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    if ( p.spec.maelstrom_weapon->ok() )
    {
      os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Maelstrom Weapon Details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      mw_generator_header( os );
      mw_generator_contents( os );
      mw_generator_piechart_contents( os );
      mw_generator_footer( os );

      os << "<div class=\"clear\"></div>\n";

      mw_consumer_header( os );
      mw_consumer_contents( os );
      mw_consumer_footer( os );

      mw_consumer_stack_header( os );
      mw_consumer_stack_contents( os );
      mw_consumer_stack_footer( os );

      os << "<div class=\"clear\"></div>\n";

      mw_consumer_piechart_contents( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";

      os << "\t\t\t\t\t</div>\n";
    }

    if ( p.talent.deeply_rooted_elements.ok() )
    {
      os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Deeply Rooted Elements Proc Details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      dre_proc_distribution_contents( os );
      dre_uptime_distribution_contents( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";

      os << "\t\t\t\t\t</div>\n";
    }

    if ( p.spec.lava_surge->ok() )
    {
      lvs_proc_distribution_contents( os );
    }
  }
};

// SHAMAN MODULE INTERFACE ==================================================

using namespace unique_gear;

struct shaman_module_t : public module_t
{
  shaman_module_t() : module_t( SHAMAN )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new shaman_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new shaman_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void init( player_t* p ) const override
  {
    p->buffs.bloodlust = make_buff( p, "bloodlust", p->find_spell( 2825 ) )
          ->set_max_stack( 1 )
          ->set_default_value_from_effect_type( A_HASTE_ALL )
          ->add_invalidate( CACHE_HASTE );

    p->buffs.exhaustion = make_buff( p, "exhaustion", p->find_spell( 57723 ) )->set_max_stack( 1 )->set_quiet( true );
  }

  void static_init() const override
  { }

  void combat_begin( sim_t* ) const override
  { }

  void combat_end( sim_t* ) const override
  { }
};

shaman_t::pets_t::pets_t( shaman_t* s ) :
    fire_elemental( "fire_elemental", s, []( shaman_t* s ) {
      return new pet::fire_elemental_t( s,
        s->talent.primal_elementalist.ok() ? elemental::PRIMAL_FIRE : elemental::GREATER_FIRE,
        elemental_variant::GREATER );
    } ),

    storm_elemental( "storm_elemental", s, []( shaman_t* s ) {
      return new pet::storm_elemental_t( s,
        s->talent.primal_elementalist.ok() ? elemental::PRIMAL_STORM : elemental::GREATER_STORM,
        elemental_variant::GREATER );
    } ),

    earth_elemental( "earth_elemental", s, []( shaman_t* s ) {
      return new pet::earth_elemental_t( s,
        s->talent.primal_elementalist.ok() ? elemental::PRIMAL_EARTH : elemental::GREATER_EARTH,
        elemental_variant::GREATER );
    } ),

    lesser_fire_elemental( "lesser_fire_elemental", s, []( shaman_t* s ) {
      return new pet::fire_elemental_t( s,
        s->talent.primal_elementalist.ok() ? elemental::PRIMAL_FIRE : elemental::GREATER_FIRE,
        elemental_variant::LESSER );
    } ),

    lesser_storm_elemental( "lesser_storm_elemental", s, []( shaman_t* s ) {
      return new pet::storm_elemental_t( s,
        s->talent.primal_elementalist.ok() ? elemental::PRIMAL_STORM : elemental::GREATER_STORM,
        elemental_variant::LESSER );
    } ),

    lightning_elemental( "greater_lightning_elemental", s, []( shaman_t* s ) {
      return new pet::greater_lightning_elemental_t( s );
    } ),

    ancestor( "ancestor", s, []( shaman_t* s ) { return new pet::ancestor_t( s ); } ),

    spirit_wolves( "spirit_wolf", s, []( shaman_t* s ) { return new pet::spirit_wolf_t( s ); } ),
    fire_wolves( "fiery_wolf", s, []( shaman_t* s ) { return new pet::fire_wolf_t( s ); } ),
    frost_wolves( "frost_wolf", s, []( shaman_t* s ) { return new pet::frost_wolf_t( s ); } ),
    lightning_wolves( "lightning_wolf", s, []( shaman_t* s ) { return new pet::lightning_wolf_t( s ); } ),

    liquid_magma_totem( "liquid_magma_totem", s, []( shaman_t* s ) { return new liquid_magma_totem_t( s ); } ),
    healing_stream_totem( "healing_stream_totem", s, []( shaman_t* s ) { return new healing_stream_totem_t( s ); } ),
    capacitor_totem( "capacitor_totem", s, []( shaman_t* s ) { return new capacitor_totem_t( s ); } ),
    surging_totem( "surging_totem", s, []( shaman_t* s ) { return new surging_totem_t( s ); } ),
    searing_totem( "searing_totem", s, []( shaman_t* s ) { return new searing_totem_t( s ); } )
{
  spirit_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  fire_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  frost_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  lightning_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );

  auto event_fn = [ s ]( spawner::pet_event_type t, pet::base_wolf_t* pet ) {
    auto it = range::find_if( s->pet.all_wolves, [ pet ]( const auto entry ) {
      return pet == entry;
    } );

    switch ( t )
    {
      case spawner::pet_event_type::ARISE:
      {
        assert( it == s->pet.all_wolves.end() );
        s->pet.all_wolves.emplace_back( pet );
        break;
      }
      case spawner::pet_event_type::DEMISE:
      {
        assert( it != s->pet.all_wolves.end() );
        s->pet.all_wolves.erase( it );
        break;
      }
      default:
        break;
    }
  };

  spirit_wolves.set_event_callback( { spawner::pet_event_type::ARISE, spawner::pet_event_type::DEMISE }, event_fn );
  fire_wolves.set_event_callback( { spawner::pet_event_type::ARISE, spawner::pet_event_type::DEMISE }, event_fn );
  frost_wolves.set_event_callback( { spawner::pet_event_type::ARISE, spawner::pet_event_type::DEMISE }, event_fn );
  lightning_wolves.set_event_callback( { spawner::pet_event_type::ARISE, spawner::pet_event_type::DEMISE }, event_fn );
}

}  // namespace

const module_t* module_t::shaman()
{
  static ::shaman_module_t m;
  return &m;
}

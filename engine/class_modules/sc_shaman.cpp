// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player/covenant.hpp"
#include "player/pet_spawner.hpp"
#include "action/action_callback.hpp"
#include "sc_enums.hpp"

#include "simulationcraft.hpp"

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

using data_t = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

struct shaman_t;

enum totem_e
{
  TOTEM_NONE = 0,
  TOTEM_AIR,
  TOTEM_EARTH,
  TOTEM_FIRE,
  TOTEM_WATER,
  TOTEM_MAX
};

enum wolf_type_e
{
  SPIRIT_WOLF = 0,
  FIRE_WOLF,
  FROST_WOLF,
  LIGHTNING_WOLF
};

enum class elemental
{
  FIRE,
  EARTH,
  STORM,
};

enum class execute_type : unsigned
{
  NORMAL = 0,
  ASCENDANCE,
  DEEPLY_ROOTED_ELEMENTS,
  SHAKE_THE_FOUNDATIONS,
  PRIMORDIAL_WAVE,
  THORIMS_INVOCATION
};

enum imbue_e
{
  IMBUE_NONE = 0,
  FLAMETONGUE_IMBUE,
  WINDFURY_IMBUE,
  FROSTBRAND_IMBUE,
  EARTHLIVING_IMBUE
};

enum rotation_type_e
{
  ROTATION_STANDARD,
  ROTATION_SIMPLE
};

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

static std::string action_name( util::string_view name, execute_type t )
{
  switch ( t )
  {
    case execute_type::ASCENDANCE: return fmt::format( "{}_asc", name );
    case execute_type::DEEPLY_ROOTED_ELEMENTS: return fmt::format( "{}_dre", name );
    case execute_type::SHAKE_THE_FOUNDATIONS: return fmt::format( "{}_stf", name );
    case execute_type::PRIMORDIAL_WAVE: return fmt::format( "{}_pw", name );
    case execute_type::THORIMS_INVOCATION: return fmt::format( "{}_ti", name );
    default: return std::string( name );
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

struct shaman_td_t : public actor_target_data_t
{
  struct dots
  {
    dot_t* flame_shock;
  } dot;

  struct debuffs
  {
    // Elemental
    buff_t* electrified_shocks;
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

struct counter_t
{
  const sim_t* sim;

  double value, interval;
  timespan_t last;

  counter_t( shaman_t* p );

  void add( double val )
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim->current_iteration == 0 && sim->iterations > sim->threads && !sim->debug && !sim->log )
      return;

    value += val;
    if ( last > timespan_t::min() )
      interval += ( sim->current_time() - last ).total_seconds();
    last = sim->current_time();
  }

  void reset()
  {
    last = timespan_t::min();
  }

  double divisor() const
  {
    if ( !sim->debug && !sim->log && sim->iterations > sim->threads )
      return sim->iterations - sim->threads;
    else
      return std::min( sim->iterations, sim->threads );
  }

  double mean() const
  {
    return value / divisor();
  }

  double interval_mean() const
  {
    return interval / divisor();
  }

  void merge( const counter_t& other )
  {
    value += other.value;
    interval += other.interval;
  }
};

struct shaman_t : public player_t
{
public:
  // Misc
  bool lava_surge_during_lvb;
  std::vector<counter_t*> counters;
  /// Shaman ability cooldowns
  std::vector<cooldown_t*> ability_cooldowns;
  player_t* recent_target =
      nullptr;  // required for Earthen Rage, whichs' ticks damage the most recently attacked target

  /// Legacy of the Frost Witch maelstrom stack counter
  unsigned lotfw_counter;

  /// Vesper totem ground aoe events
  ground_aoe_event_t* ev_vesper_totem_dmg, *ev_vesper_totem_heal;
  /// Vesper totem damage charges left, heal charges left, charges used
  unsigned vesper_totem_dmg_charges, vesper_totem_heal_charges, vesper_totem_used_charges;
  /// Vesper totem placement target, needed for Raging Vesper Vortex proper targeting
  player_t* vesper_totem_target;
  /// Vesper totem damage action
  action_t* vesper_totem_damage, *vesper_totem_heal;
  /// Raging Vesper Vortex
  action_t* raging_vesper_vortex;

  // Options
  bool raptor_glyph;

  // Data collection for cooldown waste
  auto_dispose<std::vector<data_t*> > cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*> > cd_waste_iter;

  // A vector of action objects that need target cache invalidation whenever the number of
  // Flame Shocks change
  std::vector<action_t*> flame_shock_dependants;
  /// A time-ordered list of active Flame Shocks on enemies
  std::vector<std::pair<timespan_t, dot_t*>> active_flame_shock;
  /// Maximum number of active flame shocks
  unsigned max_active_flame_shock;

  struct
  {
    /// Number of allies hit by Chain Harvest heals, range 0..5
    int chain_harvest_allies = 5;
  } opt_sl; // Shadowlands Shaman-specific options

  // Cached actions
  struct actions_t
  {
    spell_t* lightning_shield;
    attack_t* crash_lightning_aoe;
    action_t* lightning_bolt_pw;
    action_t* lightning_bolt_ti;
    action_t* chain_lightning_ti;
    action_t* ti_trigger;
    action_t* lava_burst_pw;
    action_t* flame_shock;

    action_t* lightning_rod;

    /// Totemic Recall last used totem (action)
    action_t* totemic_recall_totem;

    // Legendaries
    action_t* dre_ascendance; // Deeply Rooted Elements
  } action;

  // Pets
  struct pets_t
  {
    pet_t* pet_fire_elemental;
    pet_t* pet_storm_elemental;
    pet_t* pet_earth_elemental;

    pet_t* guardian_fire_elemental;
    pet_t* guardian_storm_elemental;
    pet_t* guardian_earth_elemental;

    spawner::pet_spawner_t<pet_t, shaman_t> spirit_wolves;
    spawner::pet_spawner_t<pet_t, shaman_t> fire_wolves;
    spawner::pet_spawner_t<pet_t, shaman_t> frost_wolves;
    spawner::pet_spawner_t<pet_t, shaman_t> lightning_wolves;

    pet_t* bron;

    pets_t( shaman_t* );
  } pet;

  // Constants
  struct
  {
    double matching_gear_multiplier;
  } constant;

  // Buffs
  struct
  {
    // shared between all three specs
    buff_t* ascendance;
    buff_t* ghost_wolf;
    buff_t* flurry;
    buff_t* natures_swiftness;

    // Covenant Class Ability Buffs
    buff_t* primordial_wave;
    buff_t* vesper_totem;

    //Covenant Legendaries
    buff_t* seeds_of_rampant_growth;

    // Legendaries
    buff_t* chains_of_devastation_chain_heal;
    buff_t* chains_of_devastation_chain_lightning;
    buff_t* doom_winds_buff;
    buff_t* doom_winds_debuff;
    buff_t* legacy_of_the_frost_witch;
    buff_t* primal_lava_actuators;
    buff_t* witch_doctors_wolf_bones;

    // Elemental, Restoration
    buff_t* lava_surge;

    // Elemental, Enhancement
    buff_t* elemental_blast_crit;
    buff_t* elemental_blast_haste;
    buff_t* elemental_blast_mastery;

    // Elemental
    buff_t* echoes_of_great_sundering;
    buff_t* elemental_equilibrium;
    buff_t* elemental_equilibrium_debuff;
    buff_t* elemental_equilibrium_fire;
    buff_t* elemental_equilibrium_frost;
    buff_t* elemental_equilibrium_nature;
    buff_t* fire_elemental;
    buff_t* flux_melting;
    buff_t* primordial_surge;
    buff_t* primordial_surge_lava_burst_buff;
    buff_t* icefury;
    buff_t* magma_chamber;
    buff_t* master_of_the_elements;
    buff_t* oath_of_the_far_seer;
    buff_t* power_of_the_maelstrom;
    buff_t* splintered_elements;
    buff_t* stormkeeper;
    buff_t* surge_of_power;
    buff_t* wind_gust;  // Storm Elemental passive 263806
    buff_t* windspeakers_lava_resurgence;

    buff_t* fireheart;

    buff_t* t29_2pc_ele;
    buff_t* t29_4pc_ele;

    // Enhancement
    buff_t* maelstrom_weapon;
    buff_t* feral_spirit_maelstrom;

    buff_t* crash_lightning;     // Buffs stormstrike and lava lash after using crash lightning
    buff_t* cl_crash_lightning;  // Buffs crash lightning with extra damage, after using chain lightning
    buff_t* hot_hand;
    buff_t* lightning_shield;
    buff_t* stormbringer;
    buff_t* hailstorm;

    buff_t* forceful_winds;
    buff_t* icy_edge;
    buff_t* molten_weapon;
    buff_t* crackling_surge;
    buff_t* earthen_weapon;
    buff_t* gathering_storms;
    buff_t* static_accumulation;
    buff_t* doom_winds_talent;
    buff_t* ice_strike;
    buff_t* ashen_catalyst;
    buff_t* witch_doctors_ancestry;

    // Restoration
    buff_t* spirit_walk;
    buff_t* spiritwalkers_grace;
    buff_t* tidal_waves;
    
    //Tier 29
    buff_t* t29_2pc_enh;
    buff_t* t29_4pc_enh;

    // PvP
    buff_t* thundercharge;

  } buff;

  // Options
  struct options_t
  {
    rotation_type_e rotation = ROTATION_STANDARD;
  } options;

  // Cooldowns
  struct
  {
    cooldown_t* ascendance;
    cooldown_t* chain_harvest;
    cooldown_t* crash_lightning;
    cooldown_t* feral_spirits;
    cooldown_t* fire_elemental;
    cooldown_t* flame_shock;
    cooldown_t* frost_shock;
    cooldown_t* icefury;
    cooldown_t* lava_burst;
    cooldown_t* lava_lash;
    cooldown_t* liquid_magma_totem;
    cooldown_t* natures_swiftness;
    cooldown_t* primordial_wave;
    cooldown_t* shock;  // shared CD of flame shock/frost shock for enhance
    cooldown_t* storm_elemental;
    cooldown_t* stormkeeper;
    cooldown_t* strike;  // shared CD of Storm Strike and Windstrike
  } cooldown;

  // Covenant Class Abilities
  struct
  {
    // Necrolord
    const spell_data_t* necrolord;  // Primordial Wave

    // Night Fae
    const spell_data_t* night_fae;  // Fae Transfusion

    // Venthyr
    const spell_data_t* venthyr;  // Chain Harvest

    // Kyrian
    const spell_data_t* kyrian;  // Vesper Totem
  } covenant;

  // Conduits
  struct conduit_t
  {
    // Covenant-specific
    conduit_data_t essential_extraction;  // Night Fae
    conduit_data_t lavish_harvest;        // Venthyr
    conduit_data_t tumbling_waves;        // Necrolord
    conduit_data_t elysian_dirge;         // Kyrian

    // Elemental
    conduit_data_t call_of_flame;
    conduit_data_t high_voltage;
    conduit_data_t pyroclastic_shock;
    conduit_data_t shake_the_foundations;

    // Enhancement
    conduit_data_t chilled_to_the_core;
    conduit_data_t focused_lightning;
    conduit_data_t magma_fist;
    conduit_data_t unruly_winds;
  } conduit;

  // Legendaries
  struct legendary_t
  {
    // Shared
    item_runeforge_t ancestral_reminder;
    item_runeforge_t chains_of_devastation;
    item_runeforge_t deeptremor_stone;
    item_runeforge_t deeply_rooted_elements;

    //Covenant
    item_runeforge_t splintered_elements;
    item_runeforge_t seeds_of_rampant_growth;
    item_runeforge_t elemental_conduit;
    item_runeforge_t raging_vesper_vortex;

    // Elemental
    item_runeforge_t skybreakers_fiery_demise;
    item_runeforge_t elemental_equilibrium;
    item_runeforge_t echoes_of_great_sundering;
    item_runeforge_t windspeakers_lava_resurgence;

    // Enhancement
    item_runeforge_t doom_winds;
    item_runeforge_t legacy_of_the_frost_witch;
    item_runeforge_t primal_lava_actuators;
    item_runeforge_t witch_doctors_wolf_bones;
  } legendary;

  // Gains
  struct
  {
    gain_t* aftershock;
    gain_t* high_voltage;
    gain_t* ascendance;
    gain_t* resurgence;
    gain_t* feral_spirit;
    gain_t* fire_elemental;
    gain_t* spirit_of_the_maelstrom;
    gain_t* searing_flames;
    gain_t* chain_harvest_heal;
    gain_t* inundate;
  } gain;

  // Tracked Procs
  struct
  {
    // Elemental, Restoration
    proc_t* lava_surge;
    proc_t* wasted_lava_surge;
    proc_t* surge_during_lvb;

    // Elemental
    proc_t* lava_surge_fireheart;
    proc_t* wasted_lava_surge_fireheart;
    proc_t* lava_surge_primordial_surge;
    proc_t* wasted_lava_surge_primordial_surge;
    proc_t* lava_surge_windspeakers_lava_resurgence;

    proc_t* aftershock;
    proc_t* flash_of_lightning;
    proc_t* further_beyond;
    proc_t* lightning_rod;
    proc_t* searing_flames;
    proc_t* tumbling_waves;

    proc_t* magma_chamber_1;
    proc_t* magma_chamber_2;
    proc_t* magma_chamber_3;
    proc_t* magma_chamber_4;
    proc_t* magma_chamber_5;
    proc_t* magma_chamber_6;
    proc_t* magma_chamber_7;
    proc_t* magma_chamber_8;
    proc_t* magma_chamber_9;
    proc_t* magma_chamber_10;
    proc_t* magma_chamber_11;
    proc_t* magma_chamber_12;
    proc_t* magma_chamber_13;
    proc_t* magma_chamber_14;
    proc_t* magma_chamber_15;
    proc_t* magma_chamber_16;
    proc_t* magma_chamber_17;
    proc_t* magma_chamber_18;
    proc_t* magma_chamber_19;
    proc_t* magma_chamber_20;

    proc_t* t28_4pc_ele_cd_extension;
    proc_t* t28_4pc_ele_cd_reduction;

    proc_t* t29_2pc_ele_1;
    proc_t* t29_2pc_ele_2;
    proc_t* t29_2pc_ele_3;
    proc_t* t29_2pc_ele_4;
    proc_t* t29_2pc_ele_5;

    proc_t* pyroclastic_shock;

    proc_t* surge_of_power_lightning_bolt;
    proc_t* surge_of_power_lava_burst;
    proc_t* surge_of_power_frost_shock;
    proc_t* surge_of_power_flame_shock;
    proc_t* surge_of_power_wasted;


    // Enhancement
    proc_t* hot_hand;
    proc_t* maelstrom_weapon_fs;
    proc_t* maelstrom_weapon_ea;
    proc_t* maelstrom_weapon_cttc;
    proc_t* maelstrom_weapon_ft;
    proc_t* maelstrom_weapon_sm;
    proc_t* maelstrom_weapon_pw;
    proc_t* maelstrom_weapon_sa;
    proc_t* stormflurry;
    proc_t* windfury_uw;
    proc_t* t28_4pc_enh;
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

    // Enhancement
    const spell_data_t* critical_strikes;
    const spell_data_t* dual_wield;
    const spell_data_t* enhancement_shaman;

    const spell_data_t* windfury;
    const spell_data_t* lava_lash_2;
    const spell_data_t* stormbringer;

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
    player_talent_t maelstrom_weapon;
    // Row 3
    player_talent_t earth_shield;
    player_talent_t fire_and_ice;
    player_talent_t capacitor_totem;
    // Row 4
    player_talent_t spiritwalkers_grace;
    player_talent_t ancestral_defense; // TODO: Avoidance
    player_talent_t static_charge;
    player_talent_t guardians_cudgel; // TODO: NYI
    player_talent_t flurry;
    // Row 5
    player_talent_t graceful_spirit; // TODO: Movement Speed
    player_talent_t natures_fury;
    // Row 6
    player_talent_t totemic_surge;
    player_talent_t winds_of_alakir; // TODO: NYI
    // Row 7
    player_talent_t healing_stream_totem;
    player_talent_t improved_lightning_bolt;
    player_talent_t spirit_walk;
    player_talent_t gust_of_wind; // TODO: NYI
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
    player_talent_t feral_lunge;
    player_talent_t ashen_catalyst;
    // Row 5
    player_talent_t doom_winds;
    player_talent_t sundering;
    player_talent_t overflowing_maelstrom;
    player_talent_t windfury_totem;
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
    player_talent_t gathering_storms;
    player_talent_t chain_lightning2;
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
    player_talent_t earthquake;
    player_talent_t elemental_fury;
    player_talent_t fire_elemental;
    player_talent_t storm_elemental;
    // Row 3
    player_talent_t inundate;
    // Row 4
    player_talent_t call_of_thunder;
    player_talent_t flow_of_power;
    player_talent_t lava_surge;
    // Row 5
    player_talent_t unrelenting_calamity;
    player_talent_t icefury;
    player_talent_t swelling_maelstrom;
    player_talent_t echo_of_the_elements;
    player_talent_t call_of_fire;
    // Row 6
    player_talent_t stormkeeper;
    player_talent_t electrified_shocks;
    player_talent_t flux_melting;
    player_talent_t aftershock;
    player_talent_t surge_of_power;
    player_talent_t flames_of_the_cauldron;
    // Row 7
    player_talent_t flash_of_lightning;
    player_talent_t eye_of_the_storm;
    player_talent_t power_of_the_maelstrom;
    player_talent_t master_of_the_elements;
    player_talent_t improved_flametongue_weapon;
    // Row 8
    player_talent_t deeply_rooted_elements;
    player_talent_t liquid_magma_totem;
    player_talent_t primal_elementalist;
    // Row 9
    player_talent_t echoes_of_great_sundering;
    player_talent_t elemental_equilibrium;
    player_talent_t rolling_magma;
    player_talent_t echo_chamber;
    player_talent_t oath_of_the_far_seer;
    player_talent_t magma_chamber;
    player_talent_t searing_flames;
    // Row 10
    player_talent_t stormkeeper2;
    player_talent_t lightning_rod;
    player_talent_t primordial_surge;
    player_talent_t mountains_will_fall;
    player_talent_t further_beyond;
    player_talent_t skybreakers_fiery_demise;
    player_talent_t windspeakers_lava_resurgence;
  } talent;

  // Misc Spells
  struct
  {
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
    const spell_data_t* t28_2pc_ele;
    const spell_data_t* t28_4pc_ele;
    const spell_data_t* t29_2pc_ele;
    const spell_data_t* t29_4pc_ele;
    const spell_data_t* inundate;
  } spell;

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
      lotfw_counter( 0U ),
      ev_vesper_totem_dmg( nullptr ), ev_vesper_totem_heal( nullptr ),
      vesper_totem_dmg_charges( 0 ), vesper_totem_heal_charges( 0 ),
      vesper_totem_used_charges( 0 ), vesper_totem_target( nullptr ),
      vesper_totem_damage( nullptr ), vesper_totem_heal( nullptr ),
      raging_vesper_vortex( nullptr ),
      raptor_glyph( false ),
      action(),
      pet( this ),
      constant(),
      buff(),
      cooldown(),
      covenant(),
      conduit( conduit_t() ),
      legendary( legendary_t() ),
      gain(),
      proc(),
      spec(),
      mastery(),
      uptime(),
      talent(),
      spell()
  {
    /*
    range::fill( pet.spirit_wolves, nullptr );
    range::fill( pet.elemental_wolves, nullptr );
    */

    // Cooldowns
    cooldown.ascendance         = get_cooldown( "ascendance" );
    cooldown.chain_harvest      = get_cooldown( "chain_harvest" );
    cooldown.crash_lightning    = get_cooldown( "crash_lightning" );
    cooldown.feral_spirits      = get_cooldown( "feral_spirit" );
    cooldown.fire_elemental     = get_cooldown( "fire_elemental" );
    cooldown.flame_shock        = get_cooldown( "flame_shock" );
    cooldown.frost_shock        = get_cooldown( "frost_shock" );
    cooldown.icefury            = get_cooldown( "icefury" );
    cooldown.lava_burst         = get_cooldown( "lava_burst" );
    cooldown.lava_lash          = get_cooldown( "lava_lash" );
    cooldown.liquid_magma_totem = get_cooldown( "liquid_magma_totem" );
    cooldown.natures_swiftness  = get_cooldown( "natures_swiftness" );
    cooldown.primordial_wave    = get_cooldown( "primordial_wave" );
    cooldown.shock              = get_cooldown( "shock" );
    cooldown.storm_elemental    = get_cooldown( "storm_elemental" );
    cooldown.stormkeeper        = get_cooldown( "stormkeeper" );
    cooldown.strike             = get_cooldown( "strike" );

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
  }

  ~shaman_t() override;

  // Misc
  bool is_elemental_pet_active() const;
  pet_t* get_active_elemental_pet() const;
  void summon_feral_spirits( timespan_t duration, unsigned n = 2, bool t28 = false );
  void summon_fire_elemental( timespan_t duration );
  void summon_storm_elemental( timespan_t duration );

  // trackers, big code blocks that shall not be doublicated
  void track_magma_chamber();
  void track_t29_2pc_ele();

  // triggers
  void trigger_maelstrom_gain( double maelstrom_gain, gain_t* gain = nullptr );
  void trigger_windfury_weapon( const action_state_t* );
  void trigger_maelstrom_weapon( const action_state_t* );
  void trigger_flametongue_weapon( const action_state_t* );
  void trigger_stormbringer( const action_state_t* state, double proc_chance = -1.0, proc_t* proc_obj = nullptr );
  void trigger_lightning_shield( const action_state_t* state );
  void trigger_hot_hand( const action_state_t* state );
  void trigger_vesper_totem( const action_state_t* state );
  void trigger_lava_surge( bool fireheart = false, bool primordial_surge = false );
  void trigger_splintered_elements( action_t* secondary );
  void trigger_flash_of_lightning();
  void trigger_lightning_rod_damage( const action_state_t* state );
  void trigger_swirling_maelstrom();

  // Legendary
  void trigger_legacy_of_the_frost_witch( unsigned consumed_stacks );
  void trigger_elemental_equilibrium( const action_state_t* state );
  void trigger_deeply_rooted_elements( const action_state_t* state );

  void trigger_secondary_flame_shock( player_t* target ) const;
  void trigger_secondary_flame_shock( const action_state_t* state ) const;
  void regenerate_flame_shock_dependent_target_list( const action_t* action ) const;

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
  std::string create_profile( save_e ) override;

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

  void create_special_effects() override;
  void init_special_effects() override;
  void init_special_effect( special_effect_t& effect ) override;
  void apply_affecting_auras( action_t& ) override;

  double resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void moving() override;
  void invalidate_cache( cache_e c ) override;
  double temporary_movement_modifier() const override;
  double passive_movement_modifier() const override;
  double composite_melee_crit_chance() const override;
  double composite_melee_haste() const override;
  double composite_melee_speed() const override;
  double composite_attack_power_multiplier() const override;
  double composite_attribute_multiplier( attribute_e ) const override;
  double composite_spell_crit_chance() const override;
  double composite_spell_haste() const override;
  double composite_spell_power( school_e school ) const override;
  double composite_spell_power_multiplier() const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t* state, bool guardian ) const override;
  double composite_leech() const override;
  double composite_maelstrom_gain_coefficient( const action_state_t* state = nullptr ) const;
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

  void datacollection_begin() override;
  void datacollection_end() override;

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

shaman_t::~shaman_t()
{
  range::dispose( counters );
}

counter_t::counter_t( shaman_t* p ) : sim( p->sim ), value( 0 ), interval( 0 ), last( timespan_t::min() )
{
  p->counters.push_back( this );
}

// ==========================================================================
// Shaman Custom Buff Declaration
// ==========================================================================
//

struct lightning_shield_buff_t : public buff_t
{
  lightning_shield_buff_t( shaman_t* p ) : buff_t( p, "lightning_shield", p->find_spell( 192106 ) )
  {
    set_duration( s_data->duration() );
  }
};

struct maelstrom_weapon_buff_t : public buff_t
{
  shaman_t* shaman;

  maelstrom_weapon_buff_t( shaman_t* p ) :
    buff_t( p, "maelstrom_weapon", p->find_spell( 344179 ) ), shaman( p )
  {
    set_max_stack( data().max_stacks() + p->talent.raging_maelstrom->effectN( 1 ).base_value() );
  }

  void increment( int stacks, double value, timespan_t duration ) override
  {
    buff_t::increment( stacks, value, duration );

    if ( shaman->buff.witch_doctors_wolf_bones->check() )
    {
      shaman->cooldown.feral_spirits->adjust(
            -( stacks == -1 ? 1 : stacks ) *
            shaman->legendary.witch_doctors_wolf_bones->effectN( 2 ).time_value() );
    }

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
    : buff_t( p, "ascendance",
              p->specialization() == SHAMAN_ENHANCEMENT ? p->find_spell( 114051 )
                                                        : p->find_spell( 114050 ) ),  // No resto for now
      lava_burst( nullptr )
  {
    set_cooldown( timespan_t::zero() );  // Cooldown is handled by the action
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
      max_stack += p->talent.crashing_storms.spell()->effectN( 3 ).base_value();
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
  splintered_elements_buff_t( shaman_t* p ) : buff_t( p, "splintered_elements", p->find_spell( 354648 ) ), shaman( p )
  {
    unsigned max_targets = as<unsigned>( shaman->find_class_spell( "Flame Shock" )->max_targets() );
    set_default_value_from_effect_type( A_HASTE_ALL );
    set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    set_stack_behavior( buff_stack_behavior::DEFAULT );
    set_max_stack( max_targets );
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
  debuff.lightning_rod      = make_buff( *this, "lightning_rod", p->find_spell( 197209 ) );
  debuff.electrified_shocks = make_buff( *this, "electrified_shocks", p->find_spell( 382089 ) )
    ->set_default_value_from_effect( 1 );

  // Enhancement
  debuff.lashing_flames = make_buff( *this, "lashing_flames", p->find_spell( 334168 ) )
      ->set_trigger_spell( p->talent.lashing_flames )
      ->set_default_value_from_effect( 1 );
}

// ==========================================================================
// Shaman Action Base Template
// ==========================================================================

template <class Base>
struct shaman_action_t : public Base
{
private:
  using ab = Base;  // action base, eg. spell_t
public:
  using base_t = shaman_action_t<Base>;

  // Cooldown tracking
  bool track_cd_waste;
  simple_sample_data_with_min_max_t *cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;

  // Ghost wolf unshift
  bool unshift_ghost_wolf;

  // Maelstrom stuff
  gain_t* gain;
  double maelstrom_gain;
  double maelstrom_gain_coefficient;

  bool affected_by_molten_weapon_da;
  bool affected_by_molten_weapon_ta;
  bool affected_by_crackling_surge_da;
  bool affected_by_crackling_surge_ta;
  bool affected_by_icy_edge_da;
  bool affected_by_icy_edge_ta;
  bool affected_by_earthen_weapon_da;
  bool affected_by_earthen_weapon_ta;
  bool affected_by_natures_fury;
  bool affected_by_ns_cost;
  bool affected_by_ns_cast_time;
  bool affected_by_enh_mastery_da;
  bool affected_by_enh_mastery_ta;
  bool affected_by_enh_t29_2pc;
  bool affected_by_lotfw_da;
  bool affected_by_lotfw_ta;

  bool may_proc_bron;
  proc_t* bron_proc;

  shaman_action_t( util::string_view n, shaman_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      track_cd_waste( s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero() ),
      cd_wasted_exec( nullptr ),
      cd_wasted_cumulative( nullptr ),
      cd_wasted_iter( nullptr ),
      unshift_ghost_wolf( true ),
      gain( player->get_gain( s->id() > 0 ? s->name_cstr() : n ) ),
      maelstrom_gain( 0 ),
      maelstrom_gain_coefficient( 1.0 ),
      affected_by_molten_weapon_da( false ),
      affected_by_molten_weapon_ta( false ),
      affected_by_crackling_surge_da( false ),
      affected_by_crackling_surge_ta( false ),
      affected_by_icy_edge_da( false ),
      affected_by_icy_edge_ta( false ),
      affected_by_natures_fury( false ),
      affected_by_ns_cost( false ),
      affected_by_ns_cast_time( false ),
      affected_by_enh_mastery_da( false ),
      affected_by_enh_mastery_ta( false ),
      affected_by_enh_t29_2pc( false ),
      affected_by_lotfw_da( false ),
      affected_by_lotfw_ta( false ),
      may_proc_bron( false ), bron_proc( nullptr )
  {
    ab::may_crit = true;

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

    affected_by_ns_cost = ab::data().affected_by( player->talent.natures_swiftness->effectN( 1 ) );
    affected_by_ns_cast_time =
      ab::data().affected_by( player->talent.natures_swiftness->effectN( 1 ) );

    affected_by_enh_mastery_da = ab::data().affected_by( player->mastery.enhanced_elements->effectN( 1 ) );
    affected_by_enh_mastery_ta = ab::data().affected_by( player->mastery.enhanced_elements->effectN( 5 ) );
    affected_by_lotfw_da = ab::data().affected_by( player->find_spell( 384451 )->effectN( 1 ) );
    affected_by_lotfw_ta = ab::data().affected_by( player->find_spell( 384451 )->effectN( 2 ) );

    //T29 Enhance 2pc
    affected_by_enh_t29_2pc = ab::data().affected_by( player->find_spell( 394677 )->effectN( 1 ) );
  }

  std::string full_name() const
  {
    std::string n = ab::data().name_cstr();
    return n.empty() ? ab::name_str : n;
  }

  void init() override
  {
    ab::init();

    if ( track_cd_waste )
    {
      cd_wasted_exec =
          p()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p()->cd_waste_exec );
      cd_wasted_cumulative = p()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>(
          ab::name_str, p()->cd_waste_cumulative );
      cd_wasted_iter =
          p()->template get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, p()->cd_waste_iter );
    }

    // Setup Hasted CD for Enhancement
    if ( ab::data().affected_by( p()->spec.shaman->effectN( 2 ) ) )
    {
      ab::cooldown->hasted = true;
    }

    // Setup Hasted GCD for Enhancement
    if ( ab::data().affected_by( p()->spec.shaman->effectN( 3 ) ) )
    {
      ab::gcd_type = gcd_haste_type::ATTACK_HASTE;
    }

    may_proc_bron = !this->background &&
      ( this->spell_power_mod.direct || this->spell_power_mod.tick ||
        this->attack_power_mod.direct || this->attack_power_mod.tick ||
        this->base_dd_min || this->base_dd_max || this->base_td );
  }

  void init_finished() override
  {
    ab::init_finished();

    if ( this->cooldown->duration > timespan_t::zero() )
    {
      p()->ability_cooldowns.push_back( this->cooldown );
    }

    if ( may_proc_bron )
    {
      bron_proc = p()->get_proc( std::string( "Bron's Call to Action: " ) + full_name() );
    }
  }

  double composite_attack_power() const override
  {
    double m = ab::composite_attack_power();

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

    if ( affected_by_lotfw_da && p()->buff.legacy_of_the_frost_witch->check() &&
         !p()->legendary.legacy_of_the_frost_witch.ok() )
    {
      m *= 1.0 + p()->talent.legacy_of_the_frost_witch->effectN( 1 ).percent();
    }

    if ( affected_by_molten_weapon_da && p()->buff.molten_weapon->check() )
    {
      for ( int x = 1; x <= p()->buff.molten_weapon->check(); x++ )
      {
        m *= 1.0 + p()->buff.molten_weapon->value();
      }
    }

    if ( affected_by_crackling_surge_da && p()->buff.crackling_surge->up() )
    {
      for ( int x = 1; x <= p()->buff.crackling_surge->check(); x++ )
      {
        m *= 1.0 + p()->buff.crackling_surge->value();
      }
    }

    if ( affected_by_icy_edge_da && p()->buff.icy_edge->up() )
    {
      for ( int x = 1; x <= p()->buff.icy_edge->check(); x++ )
      {
        m *= 1.0 + p()->buff.icy_edge->value();
      }
    }

    if ( affected_by_earthen_weapon_da && p()->buff.earthen_weapon->up() )
    {
      for ( int x = 1; x <= p()->buff.earthen_weapon->check(); x++ )
      {
        m *= 1.0 + p()->buff.earthen_weapon->value();
      }
    }

    if ( affected_by_enh_t29_2pc && p()->buff.t29_2pc_enh->up() )
    {
      m *= 1.0 + p()->buff.t29_2pc_enh->value();
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

    if ( affected_by_lotfw_ta && p()->buff.legacy_of_the_frost_witch->check() &&
         !p()->legendary.legacy_of_the_frost_witch.ok() )
    {
      m *= 1.0 + p()->talent.legacy_of_the_frost_witch->effectN( 1 ).percent();
    }

    if ( affected_by_molten_weapon_ta && p()->buff.molten_weapon->check() )
    {
      m *= 1.0 + p()->buff.molten_weapon->stack_value();
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

  timespan_t execute_time() const override
  {
    auto et = ab::execute_time();

    if ( affected_by_ns_cast_time && p()->buff.natures_swiftness->check() )
    {
      et *= 1.0 + p()->talent.natures_swiftness->effectN( 2 ).percent();
    }

    return et;
  }

  double cost() const override
  {
    double c = ab::cost();

    if ( affected_by_ns_cost && p()->buff.natures_swiftness->check() )
    {
      c *= 1.0 + p()->talent.natures_swiftness->effectN( 1 ).percent();
    }

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

    if ( affected_by_ns_cast_time || affected_by_ns_cost )
    {
      p()->buff.natures_swiftness->decrement();
    }

    if ( this->p()->buff.t29_2pc_enh->up() && affected_by_enh_t29_2pc )
    {
      this->p()->buff.maelstrom_weapon->increment( 1 );
      this->p()->buff.t29_2pc_enh->expire();
    } 

    // TODO: wire up enh MW gains
    // I ended up coding MW gains inside attack since it only procs off melee attacks
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

  void update_ready( timespan_t cd ) override
  {
    if ( cd_wasted_exec &&
         ( cd > timespan_t::zero() || ( cd <= timespan_t::zero() && ab::cooldown->duration > timespan_t::zero() ) ) &&
         ab::cooldown->current_charge == ab::cooldown->charges && ab::cooldown->last_charged > timespan_t::zero() &&
         ab::cooldown->last_charged < ab::sim->current_time() )
    {
      double time_ = ( ab::sim->current_time() - ab::cooldown->last_charged ).total_seconds();
      if ( p()->sim->debug )
      {
        p()->sim->out_debug.printf( "%s %s cooldown waste tracking waste=%.3f exec_time=%.3f", p()->name(), ab::name(),
                                    time_, ab::time_to_execute.total_seconds() );
      }
      time_ -= ab::time_to_execute.total_seconds();

      if ( time_ > 0 )
      {
        cd_wasted_exec->add( time_ );
        cd_wasted_iter->add( time_ );
      }
    }

    ab::update_ready( cd );
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
    // TODO: Some sort of selector whether it's per cast or per target. Per target is the "default".
    g *= state->n_targets;
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
  bool may_proc_maelstrom_weapon;
  bool may_proc_stormbringer;
  bool may_proc_lightning_shield;
  bool may_proc_hot_hand;
  bool may_proc_ability_procs;  // For things that explicitly state they proc from "abilities"

  proc_t *proc_wf, *proc_ft, *proc_fb, *proc_mw, *proc_sb, *proc_ls, *proc_hh;

  shaman_attack_t( util::string_view token, shaman_t* p, const spell_data_t* s )
    : base_t( token, p, s ),
      may_proc_windfury( p->talent.windfury_weapon.ok() ),
      may_proc_flametongue( true ),
      may_proc_maelstrom_weapon( true ),
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

    if ( may_proc_maelstrom_weapon )
    {
      may_proc_maelstrom_weapon = ab::weapon != nullptr;
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

    if ( may_proc_maelstrom_weapon )
    {
      proc_mw = player->get_proc( std::string( "Maelstrom Weapon: " ) + full_name() );
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

    p()->trigger_maelstrom_weapon( state );
    p()->trigger_windfury_weapon( state );
    p()->trigger_flametongue_weapon( state );
    p()->trigger_lightning_shield( state );
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

  int mw_consume_max_stack;
  // Cache execute MW multiplier into a variable upon cast finish
  double mw_multiplier;

  shaman_spell_base_t( util::string_view n, shaman_t* player,
                       const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ), mw_consume_max_stack( 0 ), mw_multiplier( 0.0 )
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
    return benefit_from_maelstrom_weapon() && !this->background;
  }

  virtual int maelstrom_weapon_stacks() const
  {
    if ( !benefit_from_maelstrom_weapon() )
    {
      return 0;
    }

    return std::min( mw_consume_max_stack, this->p()->buff.maelstrom_weapon->check() );
  }

  timespan_t execute_time() const override
  {
    auto t = ab::execute_time();

    t *= 1.0 + this->p()->spell.maelstrom_weapon->effectN( 1 ).percent() *
      maelstrom_weapon_stacks();

    return std::max( t, 0_s );
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
      m *= 1.0 + 0.05;
    }

    return m;
  }

  void compute_mw_multiplier()
  {
    mw_multiplier = 0.0;

    auto mw_stacks = maelstrom_weapon_stacks();
    if ( mw_stacks && affected_by_maelstrom_weapon )
    {
      double stack_value = this->p()->talent.improved_maelstrom_weapon->effectN( 2 ).percent() +
                           this->p()->talent.raging_maelstrom->effectN( 2 ).percent() +
                           this->p()->conduit.focused_lightning.percent();

      mw_multiplier = stack_value * mw_stacks;
    }

    if ( this->sim->debug && mw_multiplier )
    {
      this->sim->out_debug.print( "{} {} mw_affected={}, mw_benefit={}, mw_consumed={}, mw_stacks={}, mw_multiplier={}",
          this->player->name(), this->name(), affected_by_maelstrom_weapon, benefit_from_maelstrom_weapon(),
          consume_maelstrom_weapon(), mw_stacks, mw_multiplier );
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
    if ( affected_by_maelstrom_weapon && execute_time() > 0_ms )
    {
      if ( this->p()->main_hand_attack && this->p()->main_hand_attack->execute_event )
      {
        event_t::cancel( this->p()->main_hand_attack->execute_event );
        this->p()->main_hand_attack->schedule_execute();
      }

      if ( this->p()->off_hand_attack && this->p()->off_hand_attack->execute_event )
      {
        event_t::cancel( this->p()->off_hand_attack->execute_event );
        this->p()->off_hand_attack->schedule_execute();
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

    auto stacks = maelstrom_weapon_stacks();
    if ( stacks && consume_maelstrom_weapon() )
    {
      this->p()->buff.maelstrom_weapon->decrement( stacks );
      if ( this->p()->talent.hailstorm.ok() )
      {
        this->p()->buff.hailstorm->trigger( stacks );
      }

      this->p()->trigger_legacy_of_the_frost_witch( stacks );

      if ( this->p()->sets->has_set_bonus( SHAMAN_ENHANCEMENT, T28, B2 ) &&
           this->p()->rng().roll(
             this->p()->spell.t28_2pc_enh->effectN( 1 ).percent() * stacks ) )
      {
        if ( this->sim->debug )
        {
          this->sim->out_debug.print( "{} Enhancement T28 2PC", this->player->name() );
        }
        this->p()->summon_feral_spirits(
            timespan_t::from_seconds(
              this->p()->spell.t28_2pc_enh->effectN( 2 ).base_value() ),
              1,
              true );
      }

      if ( this->p()->sets->has_set_bonus( SHAMAN_ENHANCEMENT, T29, B4 ) )
      { 
        //4pc refreshes duration and adds stacks
        this->p()->buff.t29_4pc_enh->trigger( stacks );
      }
    }
  }
};

// ==========================================================================
// Shaman Offensive Spell
// ==========================================================================

struct shaman_spell_state_t : public action_state_t
{
  execute_type exec_type = execute_type::NORMAL;

  shaman_spell_state_t( action_t* action_, player_t* target_ ) :
    action_state_t( action_, target_ )
  { }

  void initialize() override
  {
    action_state_t::initialize();
    exec_type = execute_type::NORMAL;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );

    auto lbs = debug_cast<const shaman_spell_state_t*>( s );
    exec_type = lbs->exec_type;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );

    s << " exec_type=" << static_cast<unsigned>( exec_type );

    return s;
  }
};

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
  bool affected_by_stormkeeper_cast_time  = false;
  bool affected_by_stormkeeper_damage     = false;

  // General things
  execute_type exec_type;

  shaman_spell_t( util::string_view token, shaman_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( token, p, s ), overload( nullptr ), proc_sb( nullptr ), proc_moe( nullptr ),
    exec_type( execute_type::NORMAL )
  {
    affected_by_stormkeeper_cast_time = data().affected_by( p->find_spell( 191634 )->effectN( 1 ) );
    affected_by_stormkeeper_damage = data().affected_by( p->find_spell( 191634 )->effectN( 2 ) );

    may_proc_stormbringer = false;
  }

  static shaman_spell_state_t* cast_state( action_state_t* s )
  { return debug_cast<shaman_spell_state_t*>( s ); }

  static const shaman_spell_state_t* cast_state( const action_state_t* s )
  { return debug_cast<const shaman_spell_state_t*>( s ); }

  action_state_t* new_state() override
  { return new shaman_spell_state_t( this, target ); }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    base_t::snapshot_internal( s, flags, rt );

    cast_state( s )->exec_type = exec_type;
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

  double action_multiplier() const override
  {
    double m = base_t::action_multiplier();

    if ( affected_by_master_of_the_elements && p()->talent.master_of_the_elements.ok() )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->value();
    }

    if ( affected_by_stormkeeper_damage && p()->buff.stormkeeper->up() )
    {
      m *= 1.0 + p()->buff.stormkeeper->value();
    }

    return m;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = base_t::execute_time();

    if ( affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->up() )
    {
      // stormkeeper has a -100% value as effect 1
      t *= 1.0 + p()->buff.stormkeeper->data().effectN( 1 ).percent();
    }

    return t;
  }

  void execute() override
  {
    base_t::execute();

    // BfA Elemental talent - Master of the Elements
    if ( affected_by_master_of_the_elements && !background && p()->buff.master_of_the_elements->check() )
    {
      p()->buff.master_of_the_elements->decrement();
      proc_moe->occur();
    }
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
    if ( affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->check() )
    {
      chance += 10.0;
    }

    return chance;
  }

  void trigger_elemental_overload( const action_state_t* source_state, double override_chance = -1.0 ) const
  {
    if ( !p()->mastery.elemental_overload->ok() )
    {
      return;
    }

    if ( !overload )
    {
      return;
    }

    double proc_chance = override_chance == -1.0
      ? overload_chance( source_state )
      : override_chance;

    if ( !rng().roll( proc_chance ) )
    {
      return;
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

  double composite_spell_power() const override
  {
    double sp = base_t::composite_spell_power();

    if ( p()->main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
      sp += p()->main_hand_weapon.buff_value;

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
  double composite_melee_speed() const override
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

  spirit_wolf_t( shaman_t* owner ) : base_wolf_t( owner, "spirit_wolf" )
  {
    dynamic = true;
  }

  void create_actions() override;

  attack_t* create_auto_attack() override
  {
    return new fs_melee_t( this );
  }
};

template <typename T>
struct spirit_bomb_t : public pet_melee_attack_t<T>
{
  spirit_bomb_t( T* player ) :
    pet_melee_attack_t<T>( player, "spirit_bomb", player -> find_spell( 198455 ) )
  {
    this -> background = true;
    this -> aoe = -1;
  }

  double composite_target_armor( player_t* ) const override
  { return 0.0; }

  double action_da_multiplier() const override
  {
    double m = pet_melee_attack_t<T>::action_da_multiplier();

    if ( this->o()->buff.legacy_of_the_frost_witch->check() &&
         !this->o()->legendary.legacy_of_the_frost_witch.ok() )
    {
      m *= 1.0 + this->o()->talent.legacy_of_the_frost_witch->effectN( 1 ).percent();
    }

    return m;
  }

};

void spirit_wolf_t::create_actions()
{
  shaman_pet_t::create_actions();

  if ( o()->talent.alpha_wolf.ok() )
  {
    alpha_wolf = new spirit_bomb_t<spirit_wolf_t>( this );
  }
}

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
  }
};

struct fire_wolf_t : public elemental_wolf_base_t
{
  fire_wolf_t( shaman_t* owner ) : elemental_wolf_base_t( owner, owner->raptor_glyph ? "fiery_raptor" : "fiery_wolf" )
  {
    wolf_type = FIRE_WOLF;
  }
};

struct lightning_wolf_t : public elemental_wolf_base_t
{
  lightning_wolf_t( shaman_t* owner )
    : elemental_wolf_base_t( owner, owner->raptor_glyph ? "lightning_raptor" : "lightning_wolf" )
  {
    wolf_type = LIGHTNING_WOLF;
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
    {
      background = true;
    }
    void execute() override
    {
      player->current.distance = 1;
    }
    timespan_t execute_time() const override
    {
      return timespan_t::from_seconds( player->current.distance / 10.0 );
    }
    bool ready() override
    {
      return ( player->current.distance > 1 );
    }
    bool usable_moving() const override
    {
      return true;
    }
  };

  primal_elemental_t( shaman_t* owner, util::string_view name, bool guardian = false, bool auto_attack = true )
    : shaman_pet_t( owner, name, guardian, auto_attack )
  {
  }

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
  action_t* deeptremor_eq; // Deeptremor Stone Earthquake damage
  buff_t* deeptremor_stone;

  earth_elemental_t( shaman_t* owner, bool guardian )
    : primal_elemental_t( owner, ( !guardian ) ? "primal_earth_elemental" : "greater_earth_elemental", guardian ),
    deeptremor_eq( nullptr ), deeptremor_stone( nullptr )
  {
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_sp      = 0.25;
  }

  // Defined later
  void create_actions() override;
  void arise() override;

  void create_buffs() override
  {
    primal_elemental_t::create_buffs();

    if ( o()->legendary.deeptremor_stone.ok() )
    {
      deeptremor_stone = make_buff<buff_t>( this, "deeptremor_stone", o()->legendary.deeptremor_stone )
        ->set_period( 1_s )
        ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
        ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
          deeptremor_eq->set_target( target );
          deeptremor_eq->execute();
        } );
    }
  }
};

// ==========================================================================
// Fire Elemental
// ==========================================================================

struct fire_elemental_t : public primal_elemental_t
{
  fire_elemental_t( shaman_t* owner, bool guardian )
    : primal_elemental_t( owner, ( guardian ) ? "greater_fire_elemental" : "primal_fire_elemental", guardian, false )
  {
    owner_coeff.sp_from_sp = 1.0;
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
    {
    }

    bool usable_moving() const override
    {
      return true;
    }
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

    if ( o()->talent.primal_elementalist->ok() )
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

  void dismiss( bool expired ) override
  {
    primal_elemental_t::dismiss( expired );
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
    {
    }
  };

  struct call_lightning_t : public pet_spell_t<storm_elemental_t>
  {
    call_lightning_t( storm_elemental_t* player, util::string_view options )
      : super( player, "call_lightning", player->find_spell( 157348 ), options )
    {
    }

    void execute() override
    {
      super::execute();

      p()->call_lightning->trigger();
    }
  };

  buff_t* call_lightning;

  storm_elemental_t( shaman_t* owner, bool guardian )
    : primal_elemental_t( owner, ( !guardian ) ? "primal_storm_elemental" : "greater_storm_elemental", guardian,
                          false ),
      call_lightning( nullptr )
  {
    owner_coeff.sp_from_sp = 1.0000;
  }

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( o()->talent.primal_elementalist->ok() )
    {
      def->add_action( "tempest,if=buff.call_lightning.remains>=10" );
    }
    def->add_action( "call_lightning" );
    def->add_action( "wind_gust" );
  }

  void create_buffs() override
  {
    primal_elemental_t::create_buffs();

    call_lightning = make_buff( this, "call_lightning", find_spell( 157348 ) )->set_cooldown( timespan_t::zero() );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = primal_elemental_t::composite_player_multiplier( school );

    if ( call_lightning->up() )
      m *= 1.0 + call_lightning->data().effectN( 2 ).percent();

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

  void dismiss( bool expired ) override
  {
    primal_elemental_t::dismiss( expired );
    o()->buff.wind_gust->expire();
  }

  void demise() override {
    primal_elemental_t::demise();
    o()->buff.wind_gust->expire();
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

    affected_by_enh_mastery_da = true; // TODO: Until Blizzard fixes data
  }

  void init() override
  {
    shaman_attack_t::init();

    snapshot_flags = update_flags = STATE_MUL_DA | STATE_VERSATILITY | STATE_CRIT;

    may_proc_windfury = may_proc_flametongue = may_proc_maelstrom_weapon = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_ability_procs = false;
  }
};

struct lightning_rod_damage_t : public shaman_spell_t
{
  lightning_rod_damage_t( shaman_t* p ) :
    shaman_spell_t( "lightning_rod", p, p->find_spell( 197568 ) )
  {
    background = may_crit = false;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  { return 1.0; }

  double composite_versatility( const action_state_t* ) const override
  { return 1.0; }
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

    if ( p()->buff.doom_winds_buff->check() )
    {
      m *= 1.0 + p()->buff.doom_winds_buff->data().effectN( 3 ).percent();
    }

    if ( p()->buff.doom_winds_talent->up() )
    {
      m *= 1.0 + p()->talent.doom_winds->effectN( 2 ).percent();
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

struct stormstrike_attack_state_t : public action_state_t
{
  bool stormbringer;

  stormstrike_attack_state_t( action_t* action_, player_t* target_ ) :
    action_state_t( action_, target_ ), stormbringer( false )
  { }

  void initialize() override
  {
    action_state_t::initialize();

    stormbringer = false;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );

    auto lbs = debug_cast<const stormstrike_attack_state_t*>( s );
    stormbringer= lbs->stormbringer;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );

    s << " stormbringer=" << stormbringer;

    return s;
  }
};
struct stormstrike_attack_t : public shaman_attack_t
{
  bool stormflurry, stormbringer;

  action_t* stormblast;

  stormstrike_attack_t( util::string_view n, shaman_t* player, const spell_data_t* s, weapon_t* w )
    : shaman_attack_t( n, player, s ), stormflurry( false ), stormbringer( false ),
    stormblast( nullptr )
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

    if ( p()->buff.gathering_storms->up() )
    {
      m *= 1.0 + p()->buff.gathering_storms->check_stack_value();
    }

    if ( stormflurry )
    {
      m *= p()->talent.stormflurry->effectN( 2 ).percent();
    }

    if ( !stormflurry &&
         p()->legendary.legacy_of_the_frost_witch.ok() &&
         p()->buff.legacy_of_the_frost_witch->check() )
    {
      m *= 1.0 + p()->buff.legacy_of_the_frost_witch->stack_value();
    }

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

    stormflurry = false;
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

      stormblast->set_target( state->target );
      stormblast->execute();
    }
  }
};

struct windstrike_attack_t : public stormstrike_attack_t
{
  windstrike_attack_t( util::string_view n, shaman_t* player, const spell_data_t* s, weapon_t* w )
    : stormstrike_attack_t( n, player, s, w )
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

// Elemental overloads

struct elemental_overload_spell_t : public shaman_spell_t
{
  shaman_spell_t* parent;

  elemental_overload_spell_t( shaman_t* p, util::string_view name, const spell_data_t* s, shaman_spell_t* parent_, double multiplier = -1.0 )
    : shaman_spell_t( name, p, s ), parent( parent_ )
  {
    base_execute_time = timespan_t::zero();
    background        = true;
    callbacks         = false;

    if ( multiplier == -1.0 )
    {
      base_multiplier *=
          p->mastery.elemental_overload->effectN( 2 ).percent() +
          p->talent.echo_chamber->effectN( 1 ).percent();
    }
    else
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

    if ( ( player->legendary.deeply_rooted_elements.ok() || player->talent.deeply_rooted_elements.ok() ||
           player->talent.ascendance.ok() ) &&
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
    if ( player->legendary.deeply_rooted_elements.ok() || player->talent.deeply_rooted_elements.ok() ||
         player->talent.ascendance.ok() )
      {
        p()->ascendance_oh    = new windlash_t( "Windlash Off-Hand", player->find_spell( 114093 ), player,
                                             &( p()->off_hand_weapon ), swing_timer_variance );
      }

      p()->off_hand_attack = p()->melee_oh;

      p()->off_hand_attack->id = 1;
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
    // Add a 8 yard radius to support Flame Shock spreading in 9.2
    radius = 8.0;

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

    m *= 1.0 + p()->buff.primal_lava_actuators->stack_value();
    m *= 1.0 + p()->buff.ashen_catalyst->stack_value();

    return m;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double tc = shaman_attack_t::composite_target_crit_chance( target );

    if ( td( target )->dot.flame_shock->is_ticking() )
    {
      tc += p()->conduit.magma_fist.percent();
    }

    return tc;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->buff.primal_lava_actuators->expire();
    p()->buff.ashen_catalyst->expire();
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
    auto idx = static_cast<unsigned>( rng().range( 0, in.size() ) );
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
  stormstrike_attack_t *mh, *oh;
  bool stormflurry;

  stormstrike_base_t( shaman_t* player, util::string_view name, const spell_data_t* spell,
                      util::string_view options_str )
    : shaman_attack_t( name, player, spell ), mh( nullptr ), oh( nullptr ),
      stormflurry( false )
  {
    parse_options( options_str );

    weapon             = &( p()->main_hand_weapon );
    cooldown           = p()->cooldown.strike;
    cooldown->duration = data().cooldown();
    cooldown->action   = this;
    weapon_multiplier  = 0.0;
    may_crit           = false;
    school             = SCHOOL_PHYSICAL;
  }

  void init() override
  {
    shaman_attack_t::init();
    may_proc_flametongue = may_proc_windfury = may_proc_stormbringer = false;
    may_proc_maelstrom_weapon = false;

    may_proc_bron = true;
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( background )
    {
      cd_duration = timespan_t::zero();
    }

    shaman_attack_t::update_ready( cd_duration );
  }

  void execute() override
  {
    shaman_attack_t::execute();

    auto stormbringer_state = p()->buff.stormbringer->up();

    if ( !stormflurry )
    {
      p()->buff.stormbringer->decrement();
    }

    if ( result_is_hit( execute_state->result ) )
    {
      mh->stormflurry = stormflurry;
      mh->stormbringer = stormbringer_state;
      mh->execute();
      if ( oh )
      {
        oh->stormflurry = stormflurry;
        oh->stormbringer = stormbringer_state;
        oh->execute();
      }

      if ( p()->buff.crash_lightning->up() )
      {
        p()->action.crash_lightning_aoe->set_target( execute_state->target );
        p()->action.crash_lightning_aoe->execute();
      }
    }

    // Don't try this at home, or anywhere else ..
    if ( p()->talent.stormflurry.ok() &&
         rng().roll( p()->talent.stormflurry->effectN( 1 ).percent() ) )
    {
      background = dual = stormflurry = true;
      schedule_execute();
      p()->proc.stormflurry->occur();
    }
    // Potential stormflurrying ends, reset things
    else
    {
      background  = false;
      dual        = false;
      stormflurry = false;
    }

    p()->buff.gathering_storms->expire();

    if ( p()->talent.elemental_assault.ok() && !stormflurry &&
        p()->rng().roll( p()->talent.elemental_assault->effectN( 3 ).percent() ) )
    {
      make_event( sim, 0_s, [ this ]() {
        p()->buff.maelstrom_weapon->trigger(
          p()->talent.elemental_assault->effectN( 2 ).base_value() );
        p()->proc.maelstrom_weapon_ea->occur();
      } );
    }

    if ( p()->legendary.legacy_of_the_frost_witch.ok() )
    {
      p()->buff.legacy_of_the_frost_witch->expire();
    }

    if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->trigger_deeply_rooted_elements( execute_state );
    }
    
    if ( p()->sets->has_set_bonus( SHAMAN_ENHANCEMENT, T29, B2 ) )
    {
      p()->buff.t29_2pc_enh->trigger();
    }
  }

  void reset() override
  {
    shaman_attack_t::reset();
    background  = false;
    dual        = false;
    stormflurry = false;
  }
};

struct stormstrike_t : public stormstrike_base_t
{
  stormstrike_t( shaman_t* player, util::string_view options_str )
    : stormstrike_base_t( player, "stormstrike", player->talent.stormstrike, options_str )
  {
    // Actual damaging attacks are done by stormstrike_attack_t
    mh = new stormstrike_attack_t( "stormstrike_mh", player, data().effectN( 1 ).trigger(),
                                   &( player->main_hand_weapon ) );
    add_child( mh );

    if ( p()->off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new stormstrike_attack_t( "stormstrike_offhand", player, data().effectN( 2 ).trigger(),
                                     &( player->off_hand_weapon ) );
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
  timespan_t cd;

  windstrike_t( shaman_t* player, util::string_view options_str )
    : stormstrike_base_t( player, "windstrike", player->find_spell( 115356 ), options_str ), cd( data().cooldown() )
  {
    // Actual damaging attacks are done by stormstrike_attack_t
    mh = new windstrike_attack_t( "windstrike_mh", player, data().effectN( 1 ).trigger(),
                                  &( player->main_hand_weapon ) );
    add_child( mh );

    if ( p()->off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new windstrike_attack_t( "windstrike_offhand", player, data().effectN( 2 ).trigger(),
                                    &( player->off_hand_weapon ) );
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

    if ( !stormflurry &&
         p()->talent.thorims_invocation.ok() &&
         p()->buff.maelstrom_weapon->check() )
    {
      auto spell = p()->action.ti_trigger
        ? p()->action.ti_trigger
        : p()->action.lightning_bolt_ti; // Default to lightning bolt if nothing is seen
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

    p()->trigger_swirling_maelstrom();
    p()->buff.ice_strike->trigger();

    if ( result_is_hit( execute_state->result ) && p()->buff.crash_lightning->up() )
    {
      p()->action.crash_lightning_aoe->set_target( execute_state->target );
      p()->action.crash_lightning_aoe->schedule_execute();
    }
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
};

// Weapon imbues

struct weapon_imbue_t : public shaman_spell_t
{
  std::string slot_str;
  slot_e slot, default_slot;
  imbue_e imbue;

  weapon_imbue_t( util::string_view name, shaman_t* player, slot_e d_, const spell_data_t* spell, util::string_view options_str ) :
    shaman_spell_t( name, player, spell ), slot( SLOT_INVALID ), default_slot( d_ ), imbue( IMBUE_NONE )
  {
    harmful = false;

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
         player->items[ slot ].parsed.temporary_enchant_id > 0 )
    {
      sim->error( "Player {} has a temporary enchant {} on slot {}, disabling {}",
        player->name(), util::slot_type_string( slot ), player->items[ slot ].parsed.temporary_enchant_id, name() );
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
  }

  bool ready() override
  {
    if ( slot == SLOT_INVALID )
    {
      return false;
    }

    if ( player->items[ slot ].active() &&
         player->items[ slot ].parsed.temporary_enchant_id > 0 )
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

      if ( p()->talent.gathering_storms->ok() )
      {
        p()->buff.gathering_storms->trigger( num_targets_hit );
      }
    }

    for ( auto pet : p()->pet.spirit_wolves.active_pets() )
    {
      debug_cast<pet::spirit_wolf_t*>( pet )->trigger_alpha_wolf();
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

  void init() override
  {
    shaman_spell_t::init();

    may_proc_bron = true;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->talent.primal_elementalist->ok() )
    {
      if ( p()->talent.storm_elemental->ok() && p()->pet.pet_storm_elemental )
      {
        p()->pet.pet_storm_elemental->demise();
      }
      else if ( !p()->talent.storm_elemental->ok() && p()->pet.pet_fire_elemental )
      {
        p()->pet.pet_fire_elemental->demise();
      }
      p()->pet.pet_earth_elemental->summon( p()->spell.earth_elemental->duration() );
    }
    else
    {
      p()->pet.guardian_earth_elemental->summon( p()->spell.earth_elemental->duration() );
    }

    // Earth Elemental in game exhibits the same bug as maelstrom-weapon empowered spells
    if ( p()->bugs && p()->main_hand_attack && p()->main_hand_attack->execute_event )
    {
      event_t::cancel( p()->main_hand_attack->execute_event );
      p()->main_hand_attack->schedule_execute();
    }
  }
};

// Fire Elemental ===========================================================

struct fire_elemental_t : public shaman_spell_t
{
  fire_elemental_t( shaman_t* player, util::string_view options_str )
    : shaman_spell_t( "fire_elemental", player, player->talent.fire_elemental )
  {
    parse_options( options_str );
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    timespan_t fire_elemental_duration = p()->spell.fire_elemental->duration();

    if ( p()->conduit.call_of_flame->ok() )
    {
      fire_elemental_duration *= ( 1.0 + p()->conduit.call_of_flame.percent() );
    }

    p()->summon_fire_elemental( fire_elemental_duration );
    p()->buff.fire_elemental->trigger();
    if ( p()->sets->has_set_bonus( SHAMAN_ELEMENTAL, T28, B2 ) )
    {
      p()->buff.fireheart->trigger();
    }
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
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    timespan_t storm_elemental_duration = p()->spell.storm_elemental->duration();

    if ( p()->conduit.call_of_flame->ok() )
    {
      storm_elemental_duration *= ( 1.0 + p()->conduit.call_of_flame.percent() );
    }

    // 2022-03-04 hotfix: if you cast Storm Elemental again while having a Storm Elemental active, the Wind Gust buff
    // will be reset.
    // https://us.forums.blizzard.com/en/wow/t/elemental-shaman-class-tuning-march-8/1195446
    p()->buff.wind_gust->expire();

    if ( p()->sets->has_set_bonus( SHAMAN_ELEMENTAL, T28, B2 ) )
    {
      p()->buff.fireheart->trigger();
    }

    p()->summon_storm_elemental( storm_elemental_duration );
  }
};

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "lightning_shield", player, player->find_talent_spell( "Lightning Shield" ) )
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

    // Earth Shield application consumes a Vesper Totem healing charge
    p()->trigger_vesper_totem( execute_state );

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
  chained_overload_base_t( shaman_t* p, util::string_view name, execute_type t,
                           const spell_data_t* spell, double mg, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, ::action_name( name, t ), spell, parent_ )
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
  chain_lightning_overload_t( shaman_t* p, execute_type t, shaman_spell_t* parent_ ) :
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
      t += p()->talent.surge_of_power->effectN( 4 ).base_value();
    }

    return t;
  }

  void execute() override
  {
    chained_overload_base_t::execute();

    p()->buff.t29_2pc_ele->trigger();
  }

  void impact( action_state_t* state ) override
  {
    chained_overload_base_t::impact( state );

    p()->trigger_lightning_rod_damage( state );
  }
};

struct lava_beam_overload_t : public chained_overload_base_t
{
  lava_beam_overload_t( shaman_t* p, execute_type t, shaman_spell_t* parent_ )
    : chained_overload_base_t( p, "lava_beam_overload", t, p->find_spell( 114738 ),
        p->spec.maelstrom->effectN( 6 ).resource( RESOURCE_MAELSTROM ), parent_ )
  {
    affected_by_master_of_the_elements = true;
  }
};

struct chained_base_t : public shaman_spell_t
{
  execute_type type;

  chained_base_t( shaman_t* player, util::string_view name, execute_type t,
                  const spell_data_t* spell, double mg, util::string_view options_str )
    : shaman_spell_t( ::action_name( name, t ), player, spell ), type( t )
  {
    parse_options( options_str );

    if ( data().effectN( 1 ).chain_multiplier() != 0 )
    {
      chain_multiplier = data().effectN( 1 ).chain_multiplier();
    }
    radius = 10.0;

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

    if ( exec_type == execute_type::NORMAL )
    {
      p()->buff.stormkeeper->decrement();
    }
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }
};

struct chain_lightning_t : public chained_base_t
{
  chain_lightning_t( shaman_t* player, execute_type t = execute_type::NORMAL, util::string_view options_str = {} )
    : chained_base_t( player, "chain_lightning", t, player->talent.chain_lightning,
        player->spec.maelstrom->effectN( 5 ).resource( RESOURCE_MAELSTROM ), options_str )
  {
    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new chain_lightning_overload_t( player, t, this );
    }

    affected_by_master_of_the_elements = true;

    switch ( type )
    {
      case execute_type::THORIMS_INVOCATION:
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
    tl.clear();

    if ( !target->is_sleeping() )
    {
      tl.push_back( target );
    }

    // Collect debuffed targets first
    if ( p()->talent.electrified_shocks.ok() )
    {
      range::for_each( sim->target_non_sleeping_list, [this, &tl]( player_t* t ) {
        if ( td( t )->debuff.electrified_shocks->check() && t->is_enemy() && t != target )
        {
          tl.emplace_back( t );
        }
      } );
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

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if (p()->buff.chains_of_devastation_chain_lightning->up())
    {
      m *= 1.0 + p()->buff.chains_of_devastation_chain_lightning->data().effectN( 2 ).percent();
    }

    return m;
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

  timespan_t execute_time() const override
  {
    timespan_t t = chained_base_t::execute_time();

    t *= 1.0 + p()->buff.wind_gust->stack_value();

    if ( p()->buff.chains_of_devastation_chain_lightning->up() )
    {
      t *= 1.0 + p()->buff.chains_of_devastation_chain_lightning->data().effectN( 1 ).percent();
    }

    return t;
  }

  int n_targets() const override
  {
    int t = chained_base_t::n_targets();

    if ( p()->buff.surge_of_power->up() )
    {
      t += p()->talent.surge_of_power->effectN( 4 ).base_value();
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

    if ( p()->legendary.chains_of_devastation->ok() )
    {
      p()->buff.chains_of_devastation_chain_heal->trigger();
      p()->buff.chains_of_devastation_chain_lightning->expire();
    }

    // Storm Elemental Wind Gust passive buff trigger
    if ( p()->talent.storm_elemental->ok() )
    {
      if ( p()->talent.primal_elementalist->ok() && p()->pet.pet_storm_elemental &&
           !p()->pet.pet_storm_elemental->is_sleeping() )
      {
        p()->buff.wind_gust->trigger();
      }
      else if ( !p()->talent.primal_elementalist->ok() && p()->pet.guardian_storm_elemental &&
                !p()->pet.guardian_storm_elemental->is_sleeping() )
      {
        p()->buff.wind_gust->trigger();
      }
    }

    if ( num_targets_hit - 1 > 0 && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->buff.cl_crash_lightning->trigger( num_targets_hit );
    }

    if ( p()->talent.chain_lightning2->ok() )
    {
      p()->cooldown.crash_lightning->adjust(
          -( p()->talent.chain_lightning2->effectN( 1 ).time_value() * num_targets_hit ) );

      if ( sim->debug )
      {
        sim->print_debug( "{} reducing Crash Lightning cooldown by {}, remains={}",
            p()->name(),
            -( p()->talent.chain_lightning2->effectN( 1 ).time_value() * num_targets_hit ),
            p()->cooldown.crash_lightning->remains() );
      }
    }

    p()->trigger_flash_of_lightning();
    p()->buff.surge_of_power->decrement();

    for ( auto pet : p()->pet.spirit_wolves.active_pets() )
    {
      debug_cast<pet::spirit_wolf_t*>( pet )->trigger_alpha_wolf();
    }

    // Track last cast for LB / CL because of Thorim's Invocation
    if ( p()->talent.thorims_invocation.ok() && type == execute_type::NORMAL )
    {
      p()->action.ti_trigger = p()->action.chain_lightning_ti;
    }

    p()->buff.t29_2pc_ele->trigger();
  }

  void impact( action_state_t* state ) override
  {
    chained_base_t::impact( state );

    p()->trigger_lightning_rod_damage( state );
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

struct lava_beam_t : public chained_base_t
{
  // This is actually a tooltip bug in-game: real testing shows that Lava Beam and
  // Lava Beam Overload generate resources identical to their Chain Lightning counterparts
  lava_beam_t( shaman_t* player, execute_type t = execute_type::NORMAL, util::string_view options_str = {} )
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
};

// Lava Burst Spell =========================================================

struct lava_burst_state_t : public shaman_spell_state_t
{
  bool wlr_buffed;
  bool ps_buffed;

  lava_burst_state_t( action_t* action_, player_t* target_ ) :
    shaman_spell_state_t( action_, target_ ), wlr_buffed( false ), ps_buffed( false )
  { }

  void initialize() override
  {
    shaman_spell_state_t::initialize();

    wlr_buffed = false;
    ps_buffed = false;
  }

  void copy_state( const action_state_t* s ) override
  {
    shaman_spell_state_t::copy_state( s );

    auto lbs = debug_cast<const lava_burst_state_t*>( s );
    wlr_buffed = lbs->wlr_buffed;
    ps_buffed = lbs->ps_buffed;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    shaman_spell_state_t::debug_str( s );

    s << " wlr_buffed=" << wlr_buffed;
    s << " ps_buffed=" << ps_buffed;

    return s;
  }
};

// As of 8.1 Lava Burst checks its state on impact. Lava Burst -> Flame Shock now forces the critical strike
struct lava_burst_overload_t : public elemental_overload_spell_t
{
  unsigned impact_flags;
  execute_type type;
  bool wlr_buffed_impact;
  bool ps_buffed_impact;

  lava_burst_overload_t( shaman_t* player, execute_type type, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( player, ::action_name( "lava_burst_overload", type ),
        player->find_spell( 285466 ), parent_ ),
      impact_flags(), type(type), wlr_buffed_impact( false ), ps_buffed_impact( false )
  {
    maelstrom_gain         = player->spec.maelstrom->effectN( 4 ).resource( RESOURCE_MAELSTROM );
    if ( player->talent.flow_of_power.enabled() )
    {
      maelstrom_gain += player->talent.flow_of_power->effectN( 4 ).base_value();
    }
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
    wlr_buffed_impact = cast_state( s )->wlr_buffed;
    ps_buffed_impact = cast_state( s )->ps_buffed;

    snapshot_internal( s, impact_flags, rt );

    cast_state( s )->exec_type = et;
    // Restore state for debugging purposes, this->wlr_buffed_impact is used for state
    cast_state( s )->wlr_buffed = wlr_buffed_impact;
    // Restore state for debugging purposes, this->ps_buffed_impact is used for state
    cast_state( s )->ps_buffed = ps_buffed_impact;
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

    if ( type == execute_type::PRIMORDIAL_WAVE )
    {
      if ( p()->covenant.necrolord->ok() )
      {
        m *= p()->covenant.necrolord->effectN( 3 ).percent();
      }
      if ( p()->talent.primordial_wave->ok() )
      {
        m *= p()->talent.primordial_wave->effectN( 3 ).percent();
      }
    }

    if ( p()->buff.ascendance->up() )
    {
      m *= 1.0 + p()->cache.spell_crit_chance();
    }

    // Buff damage in both "snapshotted" and "current state" cases
    if ( wlr_buffed_impact || p()->buff.windspeakers_lava_resurgence->up() )
    {
      m *= 1.0 + p()->buff.windspeakers_lava_resurgence->data().effectN( 1 ).percent();
    }

    if ( ps_buffed_impact || p()->buff.primordial_surge_lava_burst_buff->up() )
    {
      m *= 1.0 + p()->buff.primordial_surge_lava_burst_buff->default_value;
    }

    if ( p()->sets->has_set_bonus( SHAMAN_ELEMENTAL, T28, B2 ) && p()->buff.fireheart->check() )
    {
          m *= 1.0 + p()->spell.t28_2pc_ele->effectN( 2 ).percent();
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

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->sets->has_set_bonus( SHAMAN_ELEMENTAL, T28, B4 ) )
    {
      // duration extension
      pet_t* pet = p()->get_active_elemental_pet();
      if ( pet )
      {
        // yes, this is not a proper time value and has a different schema than effectN 3. Here 1 second has a value of
        // 10
        timespan_t extension = timespan_t::from_millis( p()->spell.t28_4pc_ele->effectN( 1 ).base_value() * 100.0 );

        p()->buff.fireheart->extend_duration( p(), extension );
        pet->adjust_duration( extension );
        p()->buff.fire_elemental->extend_duration( p(), extension );

        p()->proc.t28_4pc_ele_cd_extension->occur();
      }

      // CD reduction
      if ( !p()->is_elemental_pet_active() && rng().roll( p()->spell.t28_4pc_ele->effectN( 2 ).percent() ) )
      {
        // yes this is not a proper time value, and a different schema for it than effectN 1. Here 1 second has a value
        // of 1
        timespan_t extension = timespan_t::from_seconds( p()->spell.t28_4pc_ele->effectN( 3 ).base_value() );

        p()->cooldown.storm_elemental->adjust( -1.0 * extension );
        p()->cooldown.fire_elemental->adjust( -1.0 * extension );

        p()->proc.t28_4pc_ele_cd_reduction->occur();
      }
    }

    if ( p()->talent.primordial_wave.ok() && p()->talent.rolling_magma.ok() )
    {
      p()->cooldown.primordial_wave->adjust( p()->talent.rolling_magma->effectN( 1 ).time_value() );
    }

    p()->buff.t29_2pc_ele->trigger();
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
    if ( player->get_active_dots( source_td->dot.flame_shock->current_action->internal_id ) ==
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

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    p()->trigger_swirling_maelstrom();
  }
};

/**
 * As of 8.1 Lava Burst checks its state on impact. Lava Burst -> Flame Shock now forces the critical strike
 */
struct lava_burst_t : public shaman_spell_t
{
  execute_type type;
  unsigned impact_flags;
  bool wlr_buffed_impact;
  bool ps_buffed_impact;

  lava_burst_t( shaman_t* player, execute_type type_, util::string_view options_str = {} )
    : shaman_spell_t( ::action_name( "lava_burst", type_ ), player, player->talent.lava_burst ),
      type( type_ ), impact_flags(), wlr_buffed_impact( false ), ps_buffed_impact( false )
  {
    parse_options( options_str );
    // Manacost is only for resto
    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      base_costs[ RESOURCE_MANA ] = 0;

      maelstrom_gain = player->spec.maelstrom->effectN( 3 ).resource( RESOURCE_MAELSTROM );
      maelstrom_gain += player->talent.flow_of_power->effectN( 3 ).base_value();
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lava_burst_overload_t( player, type, this );
    }

    spell_power_mod.direct = player->find_spell( 285452 )->effectN( 1 ).sp_coeff();

    if ( type != execute_type::NORMAL )
    {
      aoe = -1;
      background = true;
      base_execute_time = 0_s;
      cooldown->duration = 0_s;
      switch ( type )
      {
        case execute_type::PRIMORDIAL_WAVE:
          if ( auto pw_action = p()->find_action( "primordial_wave" ) )
          {
            pw_action->add_child( this );
          }
          break;
        case execute_type::ASCENDANCE:
          {
            auto asc_action = p()->find_action( "ascendance" );
            if ( p()->talent.ascendance->ok() && asc_action )
            {
              asc_action->add_child( this );
            }
          }
          break;
        case execute_type::DEEPLY_ROOTED_ELEMENTS:
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

    // Elemental and Restoration gain a second Lava Burst charge via Echo of the Elements
    if ( p()->talent.echo_of_the_elements->ok() )
    {
      cooldown->charges = as<int>( data().charges() ) +
        as<int>( p()->talent.echo_of_the_elements->effectN( 1 ).base_value() );
    }
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    shaman_spell_t::available_targets( tl );

    p()->regenerate_flame_shock_dependent_target_list( this );

    return tl.size();
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    shaman_spell_t::snapshot_internal( s, flags, rt );

    cast_state( s )->wlr_buffed = p()->buff.windspeakers_lava_resurgence->check();
    cast_state( s )->ps_buffed = p()->buff.primordial_surge_lava_burst_buff->check();
  }

  void snapshot_impact_state( action_state_t* s, result_amount_type rt )
  {
    wlr_buffed_impact = cast_state( s )->wlr_buffed;
    ps_buffed_impact = cast_state( s )->ps_buffed;

    snapshot_internal( s, impact_flags, rt );

    // Restore state for debugging purposes, this->wlr_buffed_impact is used for state
    cast_state( s )->wlr_buffed = wlr_buffed_impact;
    // Restore state for debugging purposes, this->ps_buffed_impact is used for state
    cast_state( s )->ps_buffed = ps_buffed_impact;
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

    if ( s->chain_target == 0 && result_is_hit( s->result ) )
    {
      if ( p()->buff.windspeakers_lava_resurgence->up() ) {
        p()->buff.windspeakers_lava_resurgence->decrement();
      }

      if ( p()->buff.primordial_surge_lava_burst_buff->up() ) {
        p()->buff.primordial_surge_lava_burst_buff->decrement();
      }

      if ( p()->buff.surge_of_power->up() )
      {
        p()->cooldown.fire_elemental->adjust( -1.0 * p()->talent.surge_of_power->effectN( 1 ).time_value() );
        p()->cooldown.storm_elemental->adjust( -1.0 * p()->talent.surge_of_power->effectN( 1 ).time_value() );
        p()->buff.surge_of_power->decrement();
        p()->proc.surge_of_power_lava_burst->occur();
      }
      p()->buff.flux_melting->decrement();
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    // Note, only Elemental Shaman gets the primordial_wave state set, so don't need
    // separate specialization checks here
    if ( type == execute_type::PRIMORDIAL_WAVE )
    {
      if ( p()->covenant.necrolord->ok() )
      {
        m *= p()->covenant.necrolord->effectN( 3 ).percent();
      }
      if ( p()->talent.primordial_wave->ok() )
      {
        m *= p()->talent.primordial_wave->effectN( 3 ).percent();
      }
    }

    if ( p()->buff.ascendance->up() )
    {
      m *= 1.0 + p()->cache.spell_crit_chance();
    }

    // Buff damage in both "snapshotted" and "current state" cases
    if ( wlr_buffed_impact || p()->buff.windspeakers_lava_resurgence->up() )
    {
      m *= 1.0 + p()->buff.windspeakers_lava_resurgence->data().effectN( 1 ).percent();
    }

    if ( ps_buffed_impact || p()->buff.primordial_surge_lava_burst_buff->up() )
    {
      m *= 1.0 + p()->buff.primordial_surge_lava_burst_buff->default_value;
    }

    if ( p()->sets->has_set_bonus( SHAMAN_ELEMENTAL, T28, B2 ) && p()->buff.fireheart->check() )
    {
        m *= 1.0 + p()->spell.t28_2pc_ele->effectN( 2 ).percent();
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

    if ( p()->sets->has_set_bonus( SHAMAN_ELEMENTAL, T28, B4 ) )
    {
      // duration extension
      pet_t* pet = p()->get_active_elemental_pet();
      if ( pet )
      {
        // yes, this is not a proper time value and has a different schema than effectN 3. Here 1 second has a value of 10
        timespan_t extension = timespan_t::from_millis( p()->spell.t28_4pc_ele->effectN( 1 ).base_value() * 100.0 );

        p()->buff.fireheart->extend_duration( p(), extension );
        pet->adjust_duration( extension );
        p()->buff.fire_elemental->extend_duration( p(), extension );

        p()->proc.t28_4pc_ele_cd_extension->occur();
      }

      // CD reduction
      if ( !p()->is_elemental_pet_active() && rng().roll( p()->spell.t28_4pc_ele->effectN( 2 ).percent() ) )
      {
        // yes this is not a proper time value, and a different schema for it than effectN 1. Here 1 second has a value
        // of 1
        timespan_t extension = timespan_t::from_seconds( p()->spell.t28_4pc_ele->effectN( 3 ).base_value() );

        p()->cooldown.storm_elemental->adjust( -1.0 * extension );
        p()->cooldown.fire_elemental->adjust( -1.0 * extension );

        p()->proc.t28_4pc_ele_cd_reduction->occur();
      }
    }

    if ( p()->talent.master_of_the_elements->ok() )
    {
      p()->buff.master_of_the_elements->trigger();
    }

    // Lava Surge buff does not get eaten, if the Lava Surge proc happened
    // during the Lava Burst cast
    if ( !p()->lava_surge_during_lvb && p()->buff.lava_surge->check() )
    {
      p()->buff.lava_surge->decrement();
    }

    p()->lava_surge_during_lvb = false;

    // Trigger primordial wave if there's targets to trigger it on
    if ( p()->specialization() == SHAMAN_ELEMENTAL && type == execute_type::NORMAL &&
         p()->buff.primordial_wave->up() && p()->action.lava_burst_pw )
    {
      p()->buff.primordial_wave->decrement();
      p()->action.lava_burst_pw->set_target( execute_state->target );
      if ( !p()->action.lava_burst_pw->target_list().empty() )
      {
        p()->action.lava_burst_pw->schedule_execute();
      }
      p()->trigger_splintered_elements( p()->action.lava_burst_pw );
    }

    if ( type == execute_type::NORMAL && p()->specialization() == SHAMAN_ELEMENTAL )
    {
      p()->trigger_deeply_rooted_elements( execute_state );
    }

    // TODO Elemental: double check if this can indeed trigger from all Lava Bursts, or only from NORMAL ones.
    if ( rng().roll( p()->talent.power_of_the_maelstrom->effectN( 2 ).percent() ) )
    {
      p()->buff.power_of_the_maelstrom->trigger();
    }

    if ( p()->talent.primordial_wave.ok() && p()->talent.rolling_magma.ok() )
    {
      p()->cooldown.primordial_wave->adjust( p()->talent.rolling_magma->effectN( 1 ).time_value() );
    }

    p()->buff.t29_2pc_ele->trigger();
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
    maelstrom_gain                     = p->spec.maelstrom->effectN( 2 ).resource( RESOURCE_MAELSTROM );
    if ( p->talent.flow_of_power.enabled() )
    {
      maelstrom_gain += p->talent.flow_of_power->effectN( 4 ).base_value();
    }

    affected_by_master_of_the_elements = true;
    // Stormkeeper affected by flagging is applied to the Energize spell ...
    affected_by_stormkeeper_damage = ( p->talent.stormkeeper.ok() || p->talent.stormkeeper2.ok() ) &&
      p->specialization() == SHAMAN_ELEMENTAL;
  }

  void execute() override
  {
    elemental_overload_spell_t::execute();

    p()->buff.t29_2pc_ele->trigger();
  }

  void impact( action_state_t* state ) override
  {
    elemental_overload_spell_t::impact( state );

    p()->trigger_lightning_rod_damage( state );
  }
};

struct lightning_bolt_t : public shaman_spell_t
{
  execute_type type;

  lightning_bolt_t( shaman_t* player, execute_type type_, util::string_view options_str = {} ) :
    shaman_spell_t( ::action_name( "lightning_bolt", type_ ),
        player, player->find_class_spell( "Lightning Bolt" ) ),
    type( type_ )
  {
    parse_options( options_str );
    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      affected_by_master_of_the_elements = true;

      maelstrom_gain = player->spec.maelstrom->effectN( 1 ).resource( RESOURCE_MAELSTROM );
      maelstrom_gain += player->talent.flow_of_power->effectN( 3 ).base_value();
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lightning_bolt_overload_t( player, this );
      //add_child( overload );
    }

    switch ( type_ )
    {
      case execute_type::PRIMORDIAL_WAVE:
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
      case execute_type::THORIMS_INVOCATION:
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

  bool consume_maelstrom_weapon() const override
  {
    if ( type == execute_type::THORIMS_INVOCATION )
    {
      return true;
    }

    return shaman_spell_t::consume_maelstrom_weapon();
  }

  int maelstrom_weapon_stacks() const override
  {
    double mw_stacks = as<double>( shaman_spell_t::maelstrom_weapon_stacks() );

    if ( type == execute_type::THORIMS_INVOCATION )
    {
      mw_stacks = std::min( mw_stacks, p()->talent.thorims_invocation->effectN( 1 ).base_value() );
    }

    return mw_stacks;
  }

  double composite_maelstrom_gain_coefficient( const action_state_t* state ) const override
  {
    double coeff = shaman_spell_t::composite_maelstrom_gain_coefficient( state );
    if ( p()->conduit.high_voltage->ok() && rng().roll( p()->conduit.high_voltage.percent() ) )
    {
      coeff *= 2.0;
    }
    return coeff;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    shaman_spell_t::available_targets( tl );

    p()->regenerate_flame_shock_dependent_target_list( this );

    return tl.size();
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p()->buff.primordial_wave->check() &&
         p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( p()->covenant.necrolord->ok() )
      {
        m *= p()->covenant.necrolord->effectN( 4 ).percent();
      }
      else if ( p()->talent.primordial_wave.ok() )
      {
        m *= p()->talent.primordial_wave->effectN( 4 ).percent();
      }
    }

    return m;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = shaman_spell_t::execute_time();

    t *= 1.0 + p()->buff.wind_gust->stack_value();

    return t;
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
    if ( p()->specialization() == SHAMAN_ENHANCEMENT &&
         type == execute_type::NORMAL && p()->buff.primordial_wave->up() )
    {
      p()->action.lightning_bolt_pw->set_target( target );
      if ( !p()->action.lightning_bolt_pw->target_list().empty() )
      {
        p()->action.lightning_bolt_pw->execute();
      }
      p()->trigger_splintered_elements( p()->action.lightning_bolt_pw );
      p()->buff.primordial_wave->expire();
    }


    shaman_spell_t::execute();

    // Storm Elemental Wind Gust passive buff trigger
    if ( p()->talent.storm_elemental->ok() )
    {
      if ( p()->talent.primal_elementalist->ok() && p()->pet.pet_storm_elemental &&
           !p()->pet.pet_storm_elemental->is_sleeping() )
      {
        p()->buff.wind_gust->trigger();
      }
      else if ( !p()->talent.primal_elementalist->ok() && p()->pet.guardian_storm_elemental &&
                !p()->pet.guardian_storm_elemental->is_sleeping() )
      {
        p()->buff.wind_gust->trigger();
      }
    }

    if ( type == execute_type::NORMAL &&
         p()->specialization() == SHAMAN_ELEMENTAL )
    {
      p()->buff.stormkeeper->decrement();
    }

    p()->trigger_flash_of_lightning();
    p()->trigger_lightning_rod_damage( execute_state );

    // Track last cast for LB / CL because of Thorim's Invocation
    if ( p()->talent.thorims_invocation.ok() && type == execute_type::NORMAL )
    {
      p()->action.ti_trigger = p()->action.lightning_bolt_ti;
    }

    p()->buff.t29_2pc_ele->trigger();
  }

  void schedule_travel( action_state_t* s ) override
  {
    if ( exec_type == execute_type::NORMAL &&
         p()->buff.power_of_the_maelstrom->up() )
    {
      trigger_elemental_overload( s, 1.0 );
      p()->buff.power_of_the_maelstrom->decrement();
    }

    if ( exec_type == execute_type::NORMAL &&
         p()->buff.surge_of_power->check() )
    {
      p()->proc.surge_of_power_lightning_bolt->occur();

      for ( auto i = 0; i < as<int>( p()->talent.surge_of_power->effectN( 2 ).base_value() ); ++i )
      {
        trigger_elemental_overload( s, 1.0 );
      }
      p()->buff.surge_of_power->decrement();
    }

    shaman_spell_t::schedule_travel( s );
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
};

// Elemental Blast Spell ====================================================

void trigger_elemental_blast_proc( shaman_t* p )
{
  unsigned b = static_cast<unsigned>( p->rng().range( 0, 3 ) );

  // if for some reason (Ineffable Truth, corruption) Elemental Blast can trigger four times, just let it overwrite
  // something
  if ( !p->buff.elemental_blast_crit->check() && !p->buff.elemental_blast_haste->check() &&
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
  }
  else if ( b == 1 )
  {
    p->buff.elemental_blast_haste->trigger();
  }
  else if ( b == 2 )
  {
    p->buff.elemental_blast_mastery->trigger();
  }
}

struct elemental_blast_overload_t : public elemental_overload_spell_t
{
  elemental_blast_overload_t( shaman_t* p, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, "elemental_blast_overload", p->find_spell( 120588 ), parent_,
        p->talent.mountains_will_fall->effectN( 1 ).percent() )
  {
    affected_by_master_of_the_elements = true;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.t29_2pc_ele->check_stack_value();
    m *= 1.0 + p()->buff.magma_chamber->check_stack_value();

    return m;
  }


  void execute() override
  {
    // Trigger buff before executing the spell, because apparently the buffs affect the cast result
    // itself.
    trigger_elemental_blast_proc( p() );
    elemental_overload_spell_t::execute();
  }
};

struct elemental_blast_t : public shaman_spell_t
{
  elemental_blast_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "elemental_blast", player,
      player->specialization() == SHAMAN_ELEMENTAL
        ? player->talent.elemental_blast
        : player->talent.elemental_blast.ok()
          ? player->find_spell( 117014 )
          : spell_data_t::not_found() )
  {
    parse_options( options_str );
    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      affected_by_master_of_the_elements = true;

      if ( p()->talent.mountains_will_fall.enabled() )
      {
        overload = new elemental_blast_overload_t( player, this );
      }

      resource_current = RESOURCE_MAELSTROM;
      cooldown->duration = 0_ms;
    }
    else if ( player->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( player->talent.elemental_blast.ok() && player->talent.lava_burst.ok() )
      {
        cooldown->charges += player->find_spell( 394152 )->effectN( 2 ).base_value();
      }
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.t29_2pc_ele->stack_value();
    m *= 1.0 + p()->buff.magma_chamber->stack_value();

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    trigger_elemental_blast_proc( p() );

    if ( p()->legendary.echoes_of_great_sundering.ok() || p()->talent.echoes_of_great_sundering.ok() )
    {
      p()->buff.echoes_of_great_sundering->trigger();
    }

    if ( p()->talent.further_beyond->ok() && p()->buff.ascendance->up() )
    {
      p()->buff.ascendance->extend_duration( p(), p()->talent.further_beyond->effectN( 2 ).time_value() );
      p()->buff.oath_of_the_far_seer->extend_duration( p(), p()->talent.further_beyond->effectN( 2 ).time_value() );
      p()->proc.further_beyond->occur();
    }

    if ( p()->talent.surge_of_power->ok() )
    {
      p()->buff.surge_of_power->trigger();
    }

    if ( p()->buff.magma_chamber->up() )
    {
      p()->track_magma_chamber();
      p()->buff.magma_chamber->expire();
    }

    if ( p()->legendary.windspeakers_lava_resurgence.ok() || p()->talent.windspeakers_lava_resurgence.ok() )
    {
      p()->buff.windspeakers_lava_resurgence->trigger();

      if ( p()->buff.lava_surge->check() )
      {
        p()->proc.wasted_lava_surge->occur();
      }

      p()->proc.lava_surge_windspeakers_lava_resurgence->occur();
      if ( !p()->executing || p()->executing->id != 51505 )
      {
        p()->cooldown.lava_burst->reset( true );
      }

      p()->buff.lava_surge->trigger();
    }

    if ( p()->buff.t29_2pc_ele->up() )
    {
      p()->track_t29_2pc_ele();
      p()->buff.t29_2pc_ele->expire();
    }

    p()->buff.t29_4pc_ele->trigger();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->talent.lightning_rod.ok() )
    {
      td( state->target )->debuff.lightning_rod->trigger();
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
    shaman_spell_t( "icefury", player, player->talent.icefury )
  {
    parse_options( options_str );
    affected_by_master_of_the_elements = true;
    maelstrom_gain = player->spec.maelstrom->effectN( 8 ).resource( RESOURCE_MAELSTROM );

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new icefury_overload_t( player, this );
      //add_child( overload );
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.icefury->trigger( data().initial_stacks() );
  }
};

// Spirit Wolf Spell ========================================================

struct feral_spirit_spell_t : public shaman_spell_t
{
  feral_spirit_spell_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "feral_spirit", player, player->talent.feral_spirit )
  {
    parse_options( options_str );
    harmful = false;
  }

  void init() override
  {
    shaman_spell_t::init();

    may_proc_bron = true;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->summon_feral_spirits( p()->spell.feral_spirit->duration() );
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

    m *= 1.0 + p()->buff.echoes_of_great_sundering->value();
    m *= 1.0 + p()->buff.magma_chamber->stack_value();
    m *= 1.0 + p()->buff.t29_2pc_ele->stack_value();

    return m;
  }
};

struct earthquake_base_t : public shaman_spell_t
{
  earthquake_damage_base_t* rumble;

  action_t* shake_the_foundations_cl;
  action_t* shake_the_foundations_lb;

  earthquake_base_t( shaman_t* player, util::string_view name, const spell_data_t* spell ) :
    shaman_spell_t( name, player, spell ),
    rumble( nullptr ), shake_the_foundations_cl( nullptr ),
    shake_the_foundations_lb( nullptr )
  {
    dot_duration = timespan_t::zero();  // The periodic effect is handled by ground_aoe_event_t

    if ( p()->conduit.shake_the_foundations.ok() )
    {
      shake_the_foundations_cl = new chain_lightning_t( player, execute_type::SHAKE_THE_FOUNDATIONS );
      shake_the_foundations_cl->background = true;
      shake_the_foundations_cl->base_costs[ RESOURCE_MANA ] = 0;

      add_child( shake_the_foundations_cl );

      shake_the_foundations_lb = new lava_beam_t( player, execute_type::SHAKE_THE_FOUNDATIONS );
      shake_the_foundations_lb->background = true;
      shake_the_foundations_lb->base_costs[ RESOURCE_MANA ] = 0;

      add_child( shake_the_foundations_lb );
    }
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

    m *= 1.0 + p()->buff.echoes_of_great_sundering->value();
    m *= 1.0 + p()->buff.magma_chamber->stack_value();
    m *= 1.0 + p()->buff.t29_2pc_ele->stack_value();

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

    if ( rng().roll( p()->conduit.shake_the_foundations.percent() ) &&
         !rumble->target_list().empty() )
    {
      auto t = rumble->target_list()[ static_cast<unsigned>( rng().range( 0,
            as<double>( rumble->target_list().size() ) ) ) ];

      if ( p()->buff.ascendance->check() )
      {
        shake_the_foundations_lb->set_target( t );
        shake_the_foundations_lb->execute();
      }
      else
      {
        shake_the_foundations_cl->set_target( t );
        shake_the_foundations_cl->execute();
      }
    }
  }
};

struct earthquake_overload_damage_t : public earthquake_damage_base_t
{
  earthquake_overload_damage_t( shaman_t* player, earthquake_base_t* parent ) :
    earthquake_damage_base_t( player, "earthquake_overload_damage", player->find_spell( 298765 ), parent )
  {
    // Earthquake modifier is hardcoded rather than using effects, so we set the modifier here
    spell_power_mod.direct = 0.1936 * player->talent.mountains_will_fall->effectN( 1 ).percent();
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
    // Earthquake modifier is hardcoded rather than using effects, so we set the modifier here
    spell_power_mod.direct = 0.1936;
  }
};

struct earthquake_t : public earthquake_base_t
{
  earthquake_t( shaman_t* player, util::string_view options_str ) :
    earthquake_base_t( player, "earthquake", player->talent.earthquake )
  {
    parse_options( options_str );

    rumble = new earthquake_damage_t( player, this );
    add_child( rumble );

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
      tdata->debuff.lightning_rod->trigger();
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
        auto idx = static_cast<unsigned>( rng().range( 0, eligible_targets.size() ) );
        td( eligible_targets[ idx ] )->debuff.lightning_rod->trigger();
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
    if ( p()->buff.magma_chamber->up() )
    {
      p()->track_magma_chamber();
      p()->buff.magma_chamber->expire();
    }

    p()->buff.master_of_the_elements->decrement();
    p()->buff.echoes_of_great_sundering->decrement();

    if ( p()->talent.further_beyond->ok() && p()->buff.ascendance->up() )
    {
      p()->buff.ascendance->extend_duration( p(), p()->talent.further_beyond->effectN( 1 ).time_value() );
      p()->buff.oath_of_the_far_seer->extend_duration( p(), p()->talent.further_beyond->effectN( 1 ).time_value() );
      p()->proc.further_beyond->occur();
    }

    if ( p()->buff.t29_2pc_ele->up() )
    {
      p()->track_t29_2pc_ele();
      p()->buff.t29_2pc_ele->expire();
    }

    p()->buff.t29_4pc_ele->trigger();

    if ( p()->talent.windspeakers_lava_resurgence.ok() )
    {
      p()->buff.windspeakers_lava_resurgence->trigger();

      if ( p()->buff.lava_surge->check() )
      {
        p()->proc.wasted_lava_surge->occur();
      }

      p()->proc.lava_surge_windspeakers_lava_resurgence->occur();
      if ( !p()->executing || p()->executing->id != 51505 )
      {
        p()->cooldown.lava_burst->reset( true );
      }

      p()->buff.lava_surge->trigger();
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
      callbacks = may_proc_windfury = may_proc_flametongue = may_proc_maelstrom_weapon = false;
    }
  };

  feral_lunge_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "feral_lunge", player, player->talent.feral_lunge )
  {
    parse_options( options_str );
    unshift_ghost_wolf = false;

    impact_action = new feral_lunge_attack_t( player );
  }

  void init() override
  {
    shaman_spell_t::init();
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

    m *= 1.0 + p()->buff.t29_2pc_ele->check_stack_value();
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

    if ( p()->talent.mountains_will_fall.enabled() )
    {
      overload = new earth_shock_overload_t( player, this );
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.t29_2pc_ele->stack_value();
    m *= 1.0 + p()->buff.magma_chamber->stack_value();

    return m;
  }


  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->legendary.echoes_of_great_sundering.ok() ||
        p()->talent.echoes_of_great_sundering.ok() )
    {
      p()->buff.echoes_of_great_sundering->trigger();
    }

    if (p()->talent.surge_of_power->ok() )
    {
      p()->buff.surge_of_power->trigger();
    }

    if ( p()->legendary.windspeakers_lava_resurgence.ok() ||
         p()->talent.windspeakers_lava_resurgence.ok() )
    {
      p()->buff.windspeakers_lava_resurgence->trigger();

      if ( p()->buff.lava_surge->check() ) {
        p()->proc.wasted_lava_surge->occur();
      }

      p()->proc.lava_surge_windspeakers_lava_resurgence->occur();
      if ( !p()->executing || p()->executing->id != 51505 ) {
        p()->cooldown.lava_burst->reset( true );
      }

      p()->buff.lava_surge->trigger();
    }

    if ( p()->conduit.pyroclastic_shock->ok() && rng().roll( p()->conduit.pyroclastic_shock.percent() ) )
    {
      dot_t* fs = this->td(target)->dot.flame_shock;
      timespan_t extend_time = p()->conduit.pyroclastic_shock->effectN( 2 ).time_value();
      if ( fs->is_ticking() )
      {
        fs->adjust_duration( extend_time );
        p()->proc.pyroclastic_shock->occur();
      }
    }

    if ( p()->talent.further_beyond->ok() && p()->buff.ascendance->up() )
    {
      p()->buff.ascendance->extend_duration( p(), p()->talent.further_beyond->effectN( 1 ).time_value() );
      p()->buff.oath_of_the_far_seer->extend_duration( p(), p()->talent.further_beyond->effectN( 1 ).time_value() );
      p()->proc.further_beyond->occur();
    }

    if ( p()->buff.magma_chamber->up() )
    {
      p()->track_magma_chamber();
      p()->buff.magma_chamber->expire();
    }

    if ( p()->buff.t29_2pc_ele->up() )
    {
      p()->track_t29_2pc_ele();
      p()->buff.t29_2pc_ele->expire();
    }

    p()->buff.t29_4pc_ele->trigger();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->talent.lightning_rod.ok() )
    {
      td( state->target )->debuff.lightning_rod->trigger();
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
  const spell_data_t* skybreakers_effect;
  const spell_data_t* elemental_conduit;

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
    elemental_resource( player->find_spell( 263819 ) ),
    skybreakers_effect( player->find_spell( 336734 ) ),
    elemental_conduit( player->find_spell( 356250 ) )
  {
    parse_options( options_str );

    // Ensure Flame Shock is single target, since Simulationcraft naively interprets a
    // Max Targets value on a spell to mean "aoe this many targets"
    aoe = 0;

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

    if ( p()->legendary.skybreakers_fiery_demise.ok() || p()->talent.skybreakers_fiery_demise.ok() )
    {
      m += skybreakers_effect->effectN( 3 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );

    m *= 1.0 + td( t )->debuff.lashing_flames->stack_value();

    return m;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    auto d = shaman_spell_t::composite_dot_duration( s );

    if ( p()->buff.fire_elemental->check() && p()->spell.fire_elemental->ok() )
    {
      d *= 1.0 + p()->spell.fire_elemental->effectN( 3 ).percent();
    }

    return d;
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    auto tt = shaman_spell_t::tick_time( state );

    tt *= 1.0 + p()->buff.fire_elemental->stack_value();

    tt *= 1.0 + p()->talent.flames_of_the_cauldron->effectN( 1 ).percent();

    return tt;
  }

  void tick( dot_t* d ) override
  {
    shaman_spell_t::tick( d );

    // proc chance suddenly became 100% and the actual chance became effectN 1
    double proc_chance = p()->talent.lava_surge->effectN( 1 ).percent();

    if ( p()->talent.lava_surge.ok() && p()->spec.restoration_shaman->ok() )
    {
      proc_chance += p()->spec.restoration_shaman->effectN( 8 ).percent();
    }

    if ( rng().roll( proc_chance ) )
    {
      p()->trigger_lava_surge();
    }

    if ( d->state->result == RESULT_CRIT &&
        ( p()->legendary.skybreakers_fiery_demise.ok() || p()->talent.skybreakers_fiery_demise.ok() ) )
    {
      p()->cooldown.storm_elemental->adjust( -1.0 * skybreakers_effect->effectN( 1 ).time_value() );
      p()->cooldown.fire_elemental->adjust( -1.0 * skybreakers_effect->effectN( 2 ).time_value() );
    }

    if ( d->state->result == RESULT_CRIT && p()->legendary.elemental_conduit->ok() )
    {
      p()->cooldown.chain_harvest->adjust( -1.0 * elemental_conduit->effectN( 3 ).time_value() );
    }

    if ( p()->legendary.primal_lava_actuators.ok() && d->state->result_amount > 0 )
    {
      auto reduction = p()->legendary.primal_lava_actuators->effectN( 1 ).base_value() / 10.0;
      if ( reduction == 0.0 )
      {
        reduction = p()->talent.ashen_catalyst->effectN( 1 ).base_value() / 10.0;
      }
      reduction /= 1.0 + p()->buff.hot_hand->check_value();

      p()->cooldown.lava_lash->adjust( timespan_t::from_seconds( -reduction ) );
      p()->buff.primal_lava_actuators->trigger();
    }

    if ( p()->talent.ashen_catalyst.ok() && d->state->result_amount > 0 )
    {
      auto reduction = p()->talent.ashen_catalyst->effectN( 1 ).base_value() / 10.0;
      reduction /= 1.0 + p()->buff.hot_hand->check_value();

      p()->cooldown.lava_lash->adjust( timespan_t::from_seconds( -reduction ) );
      p()->buff.ashen_catalyst->trigger();
    }

    if ( p()->talent.searing_flames->ok() && rng().roll( p()->talent.searing_flames->effectN( 2 ).percent() ) )
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

    if ( state->result == RESULT_CRIT && p()->legendary.elemental_conduit->ok() )
    {
      p()->cooldown.chain_harvest->adjust( -1.0 * elemental_conduit->effectN( 3 ).time_value() );
    }

    if ( p()->buff.surge_of_power->up() && sim->target_non_sleeping_list.size() == 1 )
    {
      p()->proc.surge_of_power_wasted->occur();
      p()->buff.surge_of_power->decrement();
    }

    if ( p()->buff.surge_of_power->up() && sim->target_non_sleeping_list.size() > 1 )
    {
      shaman_td_t* source_td = td( target );
      player_t* additional_target = nullptr;
      // If all targets have flame shock, pick the shortest remaining time
      if ( player->get_active_dots( source_td->dot.flame_shock->current_action->internal_id ) ==
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

    if ( p()->buff.icefury->up() )
    {
      m *= 1.0 + p()->buff.icefury->value();
    }

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

    if ( p() ->buff.icefury->up() && p()->talent.electrified_shocks->ok() )
    {
      t += p()->talent.electrified_shocks->effectN( 1 ).base_value();
    }

    return t;
  }

  void execute() override
  {
    if ( p()->buff.icefury->up() )
    {
      maelstrom_gain = p()->spec.maelstrom->effectN( 7 ).resource( RESOURCE_MAELSTROM );
    }

    shaman_spell_t::execute();

    if ( p()->buff.hailstorm->check() >=
         p()->talent.swirling_maelstrom->effectN( 2 ).base_value() )
    {
      p()->trigger_swirling_maelstrom();
    }

    p()->buff.flux_melting->trigger();

    p()->buff.icefury->decrement();

    p()->buff.hailstorm->expire();
    p()->buff.ice_strike->expire();

    maelstrom_gain = 0.0;
  }

  void impact( action_state_t* s ) override
  {
    shaman_spell_t::impact( s );

    if ( rng().roll( p()->conduit.chilled_to_the_core.percent() ) )
    {
      p()->buff.maelstrom_weapon->trigger( p()->conduit.chilled_to_the_core->effectN( 2 ).base_value() );
      p()->proc.maelstrom_weapon_cttc->occur();
      p()->proc.maelstrom_weapon_cttc->occur();
    }

    if ( p()->buff.icefury->up() && p()->talent.electrified_shocks->ok() )
    {
      td( s->target )->debuff.electrified_shocks->trigger();
    }
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

    if ( p()->talent.inundate.ok() )
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
    shaman_spell_t( name_str, player, player->find_talent_spell( "Ascendance", player->specialization(), false, false ) ),
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
        lvb = new lava_burst_t( p(), execute_type::ASCENDANCE );
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

    may_proc_bron = true;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->cooldown.strike->reset( false );

    if ( background )
    {
      p()->buff.ascendance->extend_duration_or_trigger(
          p()->legendary.deeply_rooted_elements.ok()
          ? p()->legendary.deeply_rooted_elements->effectN( 1 ).time_value()
          : p()->talent.deeply_rooted_elements->effectN( 1 ).time_value(), player );
    }
    else
    {
      p()->buff.ascendance->trigger();
    }

    if ( lvb )
    {
      lvb->set_target( player->target );
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
          fs_dot->adjust_duration( new_duration, -1 );
        }
      } );

      if ( p()->talent.oath_of_the_far_seer.ok() )
      {
        if ( background )
        {
            p()->buff.oath_of_the_far_seer->extend_duration_or_trigger(
                p()->legendary.deeply_rooted_elements.ok()
                    ? p()->legendary.deeply_rooted_elements->effectN( 1 ).time_value()
                    : p()->talent.deeply_rooted_elements->effectN( 1 ).time_value(),
                player );
        }
        else
        {
            p()->buff.oath_of_the_far_seer->trigger();
        }
      }
    }

    if ( p()->talent.static_accumulation.ok() )
    {
      p()->buff.static_accumulation->trigger();
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
        lvb = new lava_burst_t( p(), execute_type::DEEPLY_ROOTED_ELEMENTS );
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
  }

  bool ready() override
  {
    if ( !p()->talent.stormkeeper.ok() && !p()->talent.stormkeeper2.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Doom Winds Spell ===========================================================

struct doom_winds_t : public shaman_spell_t
{
  doom_winds_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "doom_winds", player, player->talent.doom_winds )
  {
    parse_options( options_str );
    may_crit = harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.doom_winds_talent->trigger();
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

  void init() override
  {
    shaman_heal_t::init();

    may_proc_bron = true;
  }

  void execute() override
  {
    shaman_heal_t::execute();

    p()->trigger_vesper_totem( execute_state );
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

  timespan_t execute_time() const override
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p()->buff.tidal_waves->up() )
    {
      c *= 1.0 - p()->spec.tidal_waves->effectN( 1 ).percent();
    }

    return c;
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

  timespan_t execute_time() const override
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p()->buff.tidal_waves->up() )
    {
      c *= 1.0 - p()->spec.tidal_waves->effectN( 1 ).percent();
    }

    return c;
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

  timespan_t execute_time() const override
  {
    timespan_t t = shaman_heal_t::execute_time();

    if ( p()->buff.chains_of_devastation_chain_heal->up() )
    {
      t *= 1 + p()->buff.chains_of_devastation_chain_heal->data().effectN( 1 ).percent();
    }

    return t;
  }

  void execute() override
  {
    shaman_heal_t::execute();

    if ( p()->legendary.chains_of_devastation->ok() )
    {
      p()->buff.chains_of_devastation_chain_lightning->trigger();
      p()->buff.chains_of_devastation_chain_heal->expire();
    }
  }

  double cost() const override
  {
    if ( p()->buff.chains_of_devastation_chain_heal->up() &&
         ( p()->specialization() == SHAMAN_ENHANCEMENT || p()->specialization() == SHAMAN_ELEMENTAL ) )
    {
      return 0.0;
    }

    return shaman_heal_t::cost();
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

// Windfury Totem Spell =====================================================

struct windfury_totem_t : public shaman_spell_t
{
  windfury_totem_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "windfury_totem", player, player->talent.windfury_totem )
  {
    parse_options( options_str );
    harmful = false;

    // If the Shaman has the Doom Winds legendary equipped or the simulator environment
    // does not enable the "global" windfury totem, we need to ensure the global
    // Windfury Totem buff for the shaman is proper duration (to force the shaman to
    // re-cast it periodically).
    auto wft_buff = buff_t::find( player, "windfury_totem" );
    if ( !sim->overrides.windfury_totem || p()->legendary.doom_winds.ok() )
    {
      wft_buff->set_duration( data().duration() );

      // Need to also create the effect since the core sim won't do it when
      // sim->overrides.windfury_totem is not set
      if ( !sim->overrides.windfury_totem )
      {
        special_effect_t effect( player );

        unique_gear::initialize_special_effect( effect, 327942 );
        if ( !effect.custom_init_object.empty() )
        {
          player->special_effects.push_back( new special_effect_t( effect ) );
        }
      }
    }

    // Set the proc-chance of the Windfury Totem buff unconditionally to 1.0 so it will
    // properly trigger in all situations (i.e., with or without legendary / global
    // override)
    wft_buff->set_chance( 1.0 );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buffs.windfury_totem->trigger();

    if ( !p()->buff.doom_winds_debuff->up() )
    {
      p()->buff.doom_winds_buff->trigger();
    }
  }

  bool ready() override
  {
    // If the shaman does not have Doom Winds legendary equipped, and the global windfury
    // totem is enabled, don't waste GCDs casting the shaman's own Windfury Totem
    if ( sim->overrides.windfury_totem && !p()->legendary.doom_winds.ok() && !p()->talent.doom_winds.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
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

  // Summon related functionality
  std::string pet_name;
  pet_t* summon_pet;

  shaman_totem_pet_t( shaman_t* p, util::string_view n )
    : pet_t( p->sim, p, n, true ),
      pulse_action( nullptr ),
      pulse_event( nullptr ),
      pulse_amplitude( timespan_t::zero() ),
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
    return owner->cache.spell_power( school );
  }

  double composite_spell_power_multiplier() const override
  {
    return owner->composite_spell_power_multiplier();
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

template <typename T, typename V>
struct shaman_totem_t : public V
{
  shaman_totem_pet_t<T>* totem_pet;
  timespan_t totem_duration;

  shaman_totem_t( util::string_view totem_name, shaman_t* player, util::string_view options_str,
                  const spell_data_t* spell_data ) :
    V( totem_name, player, spell_data ),
    totem_pet( nullptr ), totem_duration( this->data().duration() )
  {
    this->parse_options( options_str );
    this->harmful = this->callbacks = this->may_miss = this->may_crit = false;
    this->ignore_false_positive = true;
    this->dot_duration = timespan_t::zero();
  }

  void init_finished() override
  {
    totem_pet = debug_cast<shaman_totem_pet_t<T>*>( this->player->find_pet( this->name() ) );

    V::init_finished();
  }

  void execute() override
  {
    V::execute();
    totem_pet->summon( totem_duration );

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

    return V::create_expression( name );
  }

  bool ready() override
  {
    if ( !totem_pet )
    {
      return false;
    }

    return V::ready();
  }
};

template <typename T>
struct totem_pulse_action_t : public T
{
  bool hasted_pulse;
  double pulse_multiplier;
  shaman_totem_pet_t<T>* totem;
  unsigned pulse;

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

  void init() override
  {
    T::init();

    // Hacky, but constructor wont work.
    this->crit_multiplier *= util::crit_multiplier( totem->o()->meta_gem );
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
      real_amplitude *= totem->cache.spell_speed();

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
  // Disable last (partial) tick on dismiss, as it seems not to happen in game atm
  if ( pulse_action && pulse_event && expiration && expiration->remains() == timespan_t::zero() )
  {
    if ( pulse_event->remains() > timespan_t::zero() )
      pulse_action->pulse_multiplier =
          pulse_event->remains() / debug_cast<totem_pulse_event_t<T>*>( pulse_event )->real_amplitude;
    pulse_action->execute();
  }

  event_t::cancel( pulse_event );

  if ( summon_pet )
    summon_pet->dismiss();

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

    // TODO: make more clever if ingame behaviour improves too.
    for ( size_t i = 0;
        i < std::min( target_list().size(), as<size_t>( data().effectN( 2 ).base_value() ) );
        ++i )
    {
      p()->trigger_secondary_flame_shock( target_list()[ i ] );
    }
  }

};

struct liquid_magma_globule_t : public spell_t
{
  liquid_magma_globule_t( spell_totem_pet_t* p ) : spell_t( "liquid_magma", p, p->find_spell( 192231 ) )
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

    globule->set_target( state->target );
    globule->schedule_execute();
  }
};

struct liquid_magma_totem_t : public spell_totem_pet_t
{
  action_t* magma_eruption;

  liquid_magma_totem_t( shaman_t* owner ) : spell_totem_pet_t( owner, "liquid_magma_totem" )
  {
    pulse_amplitude = owner->find_spell( 192226 )->effectN( 1 ).period();
  }

  void init_spells() override
  {
    spell_totem_pet_t::init_spells();

    pulse_action = new liquid_magma_totem_pulse_t( this );
  }
};

struct liquid_magma_totem_spell_t : public shaman_totem_t<spell_t, shaman_spell_t>
{
  magma_eruption_t* eruption;

  liquid_magma_totem_spell_t( shaman_t* p, util::string_view options_str ) :
    shaman_totem_t<spell_t, shaman_spell_t>( "liquid_magma_totem", p, options_str,
        p->talent.liquid_magma_totem ),
    eruption( new magma_eruption_t( p ) )
  { }

  void execute() override
  {
    shaman_totem_t<spell_t, shaman_spell_t>::execute();

    eruption->set_target( execute_state->target );
    eruption->execute();
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

  void execute() override
  {
    heal_totem_action_t::execute();

    // Trigger Vesper Totem on the first pulse instead of totem summon
    if ( pulse == 1U )
    {
      o()->trigger_vesper_totem( execute_state );
    }
  }
};

struct healing_stream_totem_t : public heal_totem_pet_t
{
  healing_stream_totem_t( shaman_t* owner ) :
    heal_totem_pet_t( owner, "healing_stream_totem" )
  {
    pulse_amplitude = owner->find_spell( 5672 )->effectN( 1 ).period();
  }

  void init_spells() override
  {
    heal_totem_pet_t::init_spells();

    pulse_action = new healing_stream_totem_pulse_t( this );
  }
};

struct healing_stream_totem_spell_t : public shaman_totem_t<heal_t, shaman_heal_t>
{
  healing_stream_totem_spell_t( shaman_t* p, util::string_view options_str ) :
    shaman_totem_t<heal_t, shaman_heal_t>( "healing_stream_totem", p, options_str,
        p->find_spell( 5394 ) )
  { }

  void execute() override
  {
    shaman_totem_t<heal_t, shaman_heal_t>::execute();

    if ( p()->talent.inundate.ok() )
    {
      p()->trigger_maelstrom_gain( p()->spell.inundate->effectN( 1 ).base_value(), p()->gain.inundate );
    }
  }
};

// ==========================================================================
// PvP talents/abilities
// ==========================================================================

struct lightning_lasso_t : public shaman_spell_t
{
  lightning_lasso_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "lightning_lasso", player, player->talent.lightning_lasso )
  {
    parse_options( options_str );
    affected_by_master_of_the_elements = false;
    // if the major effect is not available the action is a background action, thus can't be used in the apl
    background         = true;
    channeled          = true;
    tick_may_crit      = true;
    may_crit           = false;
  }

  timespan_t tick_time( const action_state_t* /* s */ ) const override
  {
    return base_tick_time;
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
// Fae Transfusion - Night Fae Covenant
// ==========================================================================

struct fae_transfusion_tick_t : public shaman_spell_t
{
  const spell_data_t* seeds_effect;

  fae_transfusion_tick_t( util::string_view n, shaman_t* player )
    : shaman_spell_t( n, player, player->find_spell( 328928 ) ),
      seeds_effect( player->find_spell( 356218 ) )
  {
    affected_by_master_of_the_elements = true;
    allow_class_ability_procs = true;
    not_a_proc = true;

    aoe        = 4;
    background = split_aoe_damage = true;
  }

  void execute() override
  {
    shaman_spell_t::execute();
    if ( p()->legendary.seeds_of_rampant_growth->ok() )
    {
      if ( p()->specialization() == SHAMAN_ELEMENTAL )
      {
        p()->cooldown.fire_elemental->adjust( -1.0 * seeds_effect->effectN( 2 ).time_value() );
        p()->cooldown.storm_elemental->adjust( -1.0 * seeds_effect->effectN( 2 ).time_value() );
      }
      if ( p()->specialization() == SHAMAN_ENHANCEMENT )
      {
        p()->cooldown.feral_spirits->adjust( -1.0 * seeds_effect->effectN( 3 ).time_value() );
      }
      // shared effect
      p()->buff.seeds_of_rampant_growth->increment();
    }
  }
};

struct fae_transfusion_t : public shaman_spell_t
{
  fae_transfusion_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "fae_transfusion", player, player->find_covenant_spell( "Fae Transfusion" ) )
  {
    parse_options( options_str );
    if ( !player->covenant.night_fae->ok() )
      return;

    affected_by_master_of_the_elements = false;

    channeled   = true;
    tick_action = new fae_transfusion_tick_t( "fae_transfusion_tick", player );
    cooldown->duration += p()->conduit.essential_extraction.time_value();
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  { return result_amount_type::DMG_DIRECT; }

  void last_tick( dot_t* d ) override
  {
    shaman_spell_t::last_tick( d );

    if ( d->current_tick == d->num_ticks() )
    {
      if ( p()->specialization() == SHAMAN_ENHANCEMENT )
      {
        p()->buff.maelstrom_weapon->trigger( p()->covenant.night_fae->effectN( 4 ).base_value() );
        for ( int i = 0; i < p()->covenant.night_fae->effectN( 4 ).base_value(); ++i )
        {
          p()->proc.maelstrom_weapon_ft->occur();
        }
      }
      p()->buff.master_of_the_elements->decrement();
    }
  }
};

// ==========================================================================
// Primordial Wave - Necrolord Covenant
// ==========================================================================

struct primordial_wave_t : public shaman_spell_t
{
  primordial_wave_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "primordial_wave", player, player->talent.primordial_wave )
  {
    parse_options( options_str );
    // attack/spell power valujes are on a secondary spell
    attack_power_mod.direct = player->find_spell( 327162 )->effectN( 1 ).ap_coeff();
    spell_power_mod.direct  = player->find_spell( 327162 )->effectN( 1 ).sp_coeff();
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
    shaman_spell_t::execute();

    p()->buff.primordial_wave->trigger();

    if ( p()->talent.primal_maelstrom.ok() )
    {
      p()->buff.maelstrom_weapon->trigger( p()->talent.primal_maelstrom->effectN( 1 ).base_value() );
      for ( auto i = 0; i < p()->talent.primal_maelstrom->effectN( 1 ).base_value(); ++i )
      {
        p()->proc.maelstrom_weapon_pw->occur();
      }
    }

    double proc_chance = p()->conduit.tumbling_waves.value() / 1000.0;
    if ( proc_chance && rng().roll( proc_chance ) )
    {
      cooldown->reset( true );
    }

    if ( p()->talent.primordial_surge.ok() )
    {
      p()->buff.primordial_surge->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    shaman_spell_t::impact( s );

    p()->trigger_secondary_flame_shock( s );
  }
};

// ==========================================================================
// Chain Harvest - Venthyr Covenant
// ==========================================================================

struct chain_harvest_t : public shaman_spell_t
{
  int critical_hits = 0;
  chain_harvest_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "chain_harvest", player, player->covenant.venthyr )
  {
    parse_options( options_str );
    aoe = 5;
    spell_power_mod.direct = player->find_spell( 320752 )->effectN( 1 ).sp_coeff();

    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      maelstrom_gain   = player->find_spell( 368583 )->effectN( 1 ).base_value();
      resource_current = resource_e::RESOURCE_MAELSTROM;
    }

    base_multiplier *= 1.0 + player->spec.elemental_shaman->effectN( 7 ).percent();

    base_crit += p()->conduit.lavish_harvest.percent();
  }

  void schedule_travel( action_state_t* s ) override
  {
    if ( s->result == RESULT_CRIT )
    {
      critical_hits++;
    }

    shaman_spell_t::schedule_travel( s );
  }

  void impact(action_state_t* s ) override
  {
    shaman_spell_t::impact( s );
    if ( p()->legendary.elemental_conduit->ok() )
    {
      p()->trigger_secondary_flame_shock( s );
    }
  }

  void update_ready( timespan_t ) override
  {
    auto cd = cooldown_duration();

    for ( int i = 0; i < p()->opt_sl.chain_harvest_allies; ++i )
    {
      critical_hits += as<int>( rng().roll( composite_crit_chance() ) );
    }

    if ( sim->debug )
    {
      sim->print_debug( "{} {} n_crit={}, cooldown={}",
        player->name(), name(), critical_hits,
        cd - critical_hits * timespan_t::from_seconds( 5 ) );
    }

    cd -= critical_hits * timespan_t::from_seconds( 5 );

    shaman_spell_t::update_ready( cd );

    critical_hits = 0;
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }

  void trigger_maelstrom_gain( const action_state_t* state ) override
  {
    shaman_spell_t::trigger_maelstrom_gain( state );
    if ( maelstrom_gain == 0 )
    {
      return;
    }

    // Generate Maelstrom for friendly targets hit.
    double g = maelstrom_gain;
    g *= composite_maelstrom_gain_coefficient( state );
    g *= p()->opt_sl.chain_harvest_allies;
    p()->resource_gain( RESOURCE_MAELSTROM, g, p()->gain.chain_harvest_heal, this );
  }
};

// ==========================================================================
// Vesper Totem - Kyrian Covenant
// ==========================================================================

struct raging_vesper_vortex_t : public shaman_spell_t
{
  raging_vesper_vortex_t( shaman_t* player )
    : shaman_spell_t( "raging_vesper_vortex", player, player->find_spell( 356790 ) )
  {
    ground_aoe = background = true;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );

    if ( t == target )
    {
      m *= 1.0 + p()->conduit.elysian_dirge.percent();
    }

    return m;
  }
};

struct vesper_totem_damage_t : public shaman_spell_t
{
  // Cache closest target so we don't need to compute it for all hit targets
  player_t* closest_target;

  vesper_totem_damage_t( shaman_t* player )
    : shaman_spell_t( "vesper_totem_damage", player, player->find_spell( 324520 ) )
  {
    background = true;
  }

  void find_closest_target()
  {
    if ( sim->distance_targeting_enabled == 0 )
    {
      closest_target = target_list().front();
    }
    else
    {
      closest_target = nullptr;
      auto min_distance = std::numeric_limits<double>::max();

      range::for_each( target_list(), [this, &min_distance] ( player_t* t ) {
        if ( sim->debug )
        {
          sim->print_debug( "{} {} candidate={} (x={}, y={}, distance={}",
            player->name(), name(), t->name(), t->x_position, t->y_position,
            player->get_player_distance( *t ) );

        }

        double dt = p()->get_player_distance( *t );
        if ( dt < min_distance )
        {
          dt = min_distance;
          closest_target = t;
        }
      } );
    }

    if ( sim->debug )
    {
      sim->print_debug( "{} {} closest_target={} (x={}, y={}, distance={})",
          player->name(), name(), closest_target ? closest_target->name() : "none",
          closest_target ? closest_target->x_position : 0.0,
          closest_target ? closest_target->y_position : 0.0,
          closest_target ? player->get_player_distance( *closest_target ) : 0.0 );

    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );

    if ( closest_target && t == closest_target )
    {
      m *= 1.0 + p()->conduit.elysian_dirge.percent();
    }

    return m;
  }

  void execute() override
  {
    find_closest_target();

    shaman_spell_t::execute();
  }
};

struct vesper_totem_heal_t : public shaman_heal_t
{
  vesper_totem_heal_t( shaman_t* player )
    : shaman_heal_t( "vesper_totem_heal", player, player->find_spell( 324522 ) )
  {
    background = true;
    aoe = 6;
  }
};

struct vesper_totem_cb_t : public dbc_proc_callback_t
{
  shaman_t* shaman;

  vesper_totem_cb_t( const special_effect_t& e, shaman_t* s ) :
    dbc_proc_callback_t( e.player, e ), shaman( s )
  { }

  void execute( action_t* /* a */, action_state_t* s ) override
  {
    shaman->trigger_vesper_totem( s );
  }
};

struct vesper_totem_t : public shaman_spell_t
{
  vesper_totem_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "vesper_totem", player, player->covenant.kyrian )
  {
    parse_options( options_str );

    if ( !player->vesper_totem_heal )
    {
      player->vesper_totem_heal = new vesper_totem_heal_t( player );
    }

    if ( !player->vesper_totem_damage )
    {
      player->vesper_totem_damage = new vesper_totem_damage_t( player );
    }

    add_child( player->vesper_totem_damage );
    add_child( player->vesper_totem_heal );

    if ( player->raging_vesper_vortex )
    {
      add_child( player->raging_vesper_vortex );
    }
  }

  // Snapshot totem position by the use of a ground action that is never triggered
  // automatically. Instead, shaman_t::trigger_vesper_totem will handle Vesper Totem
  // pulsing by using a generic proc callback and some special magic to handle other
  // totems (e.g., HST) proccing it.
  void trigger_vesper_totem_ground_aoe( action_t*            action,
                                        ground_aoe_event_t*& event_ptr,
                                        player_t*            totem_target,
                                        unsigned             charges )
  {
    make_event<ground_aoe_event_t>(
        *player->sim, player,
        ground_aoe_params_t()
            .target( totem_target )
            .pulse_time( sim->max_time * 2 )
            .n_pulses( charges )
            .action( action )
            // Automatically keep track of aoe event state in a shaman_t cached pointers
            .state_callback( [ &event_ptr ]( ground_aoe_params_t::state_type event_type,
                                             ground_aoe_event_t* ptr ) {
              switch ( event_type )
              {
                case ground_aoe_params_t::state_type::EVENT_CREATED:
                  assert( event_ptr == nullptr );
                  event_ptr = ptr;
                  break;
                case ground_aoe_params_t::state_type::EVENT_DESTRUCTED:
                  assert( event_ptr == ptr );
                  event_ptr = nullptr;
                  break;
                default:
                  break;
              }
            } ) );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->vesper_totem_dmg_charges = data().effectN( 2 ).base_value();
    p()->vesper_totem_heal_charges = data().effectN( 4 ).base_value();
    p()->vesper_totem_used_charges = 0;
    p()->vesper_totem_target = execute_state->target;

    p()->buff.vesper_totem->trigger();
    trigger_vesper_totem_ground_aoe( p()->vesper_totem_damage, p()->ev_vesper_totem_dmg,
        execute_state->target, p()->vesper_totem_dmg_charges );
    trigger_vesper_totem_ground_aoe( p()->vesper_totem_heal, p()->ev_vesper_totem_heal,
        player, p()->vesper_totem_heal_charges );
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
        if ( player->off_hand_attack->execute_action )
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
    return new shaman_totem_t<spell_t, shaman_spell_t>( "capacitor_totem", this, options_str, talent.capacitor_totem );
  if ( name == "elemental_blast" )
    return new elemental_blast_t( this, options_str );
  if ( name == "flame_shock" )
    return new flame_shock_t( this, options_str );
  if ( name == "frost_shock" )
    return new frost_shock_t( this, options_str );
  if ( name == "ghost_wolf" )
    return new ghost_wolf_t( this, options_str );
  if ( name == "lightning_bolt" )
    return new lightning_bolt_t( this, execute_type::NORMAL, options_str );
  if ( name == "chain_lightning" )
    return new chain_lightning_t( this, execute_type::NORMAL, options_str );
  if ( name == "stormkeeper" )
    return new stormkeeper_t( this, options_str );
  if ( name == "wind_shear" )
    return new wind_shear_t( this, options_str );
  if ( name == "windfury_totem" )
    return new windfury_totem_t( this, options_str );
  if ( name == "healing_stream_totem" )
    return new healing_stream_totem_spell_t( this, options_str );
  if ( name == "earth_shield" )
    return new earth_shield_t( this, options_str );
  if ( name == "natures_swiftness" )
    return new natures_swiftness_t( this, options_str );
  if ( name == "totemic_recall" )
    return new totemic_recall_t( this, options_str );

  // covenants
  if ( name == "primordial_wave" )
    return new primordial_wave_t( this, options_str );
  if ( name == "fae_transfusion" )
    return new fae_transfusion_t( this, options_str );
  if ( name == "chain_harvest" )
    return new chain_harvest_t( this, options_str );
  if ( name == "vesper_totem" )
  {
    return new vesper_totem_t( this, options_str );
  }

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
    return new lava_beam_t( this, execute_type::NORMAL, options_str );
  if ( name == "lava_burst" )
    return new lava_burst_t( this, execute_type::NORMAL, options_str );
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

  return player_t::create_action( name, options_str );
}

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( util::string_view pet_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  if ( pet_name == "primal_fire_elemental" )
    return new pet::fire_elemental_t( this, false );
  if ( pet_name == "greater_fire_elemental" )
    return new pet::fire_elemental_t( this, true );
  if ( pet_name == "primal_storm_elemental" )
    return new pet::storm_elemental_t( this, false );
  if ( pet_name == "greater_storm_elemental" )
    return new pet::storm_elemental_t( this, true );
  if ( pet_name == "primal_earth_elemental" )
    return new pet::earth_elemental_t( this, false );
  if ( pet_name == "greater_earth_elemental" )
    return new pet::earth_elemental_t( this, true );
  if ( pet_name == "liquid_magma_totem" )
    return new liquid_magma_totem_t( this );
  if ( pet_name == "capacitor_totem" )
    return new capacitor_totem_t( this );
  if ( pet_name == "healing_stream_totem" )
    return new healing_stream_totem_t( this );

  return nullptr;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  if ( talent.primal_elementalist->ok() )
  {
    if ( find_action( "fire_elemental" ) && !talent.storm_elemental->ok() )
    {
      pet.pet_fire_elemental = create_pet( "primal_fire_elemental" );
    }

    if ( find_action( "earth_elemental" ) )
    {
      pet.pet_earth_elemental = create_pet( "primal_earth_elemental" );
    }

    if ( talent.storm_elemental->ok() && find_action( "storm_elemental" ) )
    {
      pet.pet_storm_elemental = create_pet( "primal_storm_elemental" );
    }
  }
  else
  {
    if ( find_action( "fire_elemental" ) && !talent.storm_elemental->ok() )
    {
      pet.guardian_fire_elemental = new pet::fire_elemental_t( this, true );
    }

    if ( find_action( "earth_elemental" ) )
    {
      pet.guardian_earth_elemental = create_pet( "greater_earth_elemental" );
    }

    if ( talent.storm_elemental->ok() && find_action( "storm_elemental" ) )
    {
      pet.guardian_storm_elemental = new pet::storm_elemental_t( this, true );
    }
  }

  if ( talent.liquid_magma_totem->ok() && find_action( "liquid_magma_totem" ) )
  {
    create_pet( "liquid_magma_totem" );
  }

  if ( find_action( "capacitor_totem" ) )
  {
    create_pet( "capacitor_totem" );
  }

  if ( find_action( "healing_stream_totem" ) )
  {
    create_pet( "healing_stream_totem" );
  }
}

// shaman_t::create_expression ==============================================

std::unique_ptr<expr_t> shaman_t::create_expression( util::string_view name )
{
  auto splits = util::string_split<util::string_view>( name, "." );

  if ( splits.size() >= 3 && util::str_compare_ci( splits[ 0 ], "pet" ) )
  {
    auto require_primal = splits[ 1 ].find( "primal_" ) != util::string_view::npos;
    auto et             = elemental::FIRE;
    if ( util::str_in_str_ci( splits[ 1 ], "fire" ) )
    {
      et = elemental::FIRE;
    }
    else if ( util::str_in_str_ci( splits[ 1 ], "earth" ) )
    {
      et = elemental::EARTH;
    }
    else if ( util::str_in_str_ci( splits[ 1 ], "storm" ) )
    {
      et = elemental::STORM;
    }
    else
    {
      return player_t::create_expression( name );
    }

    const pet_t* p = nullptr;
    auto pe        = require_primal || talent.primal_elementalist->ok();
    switch ( et )
    {
      case elemental::FIRE:
        p = pe ? pet.pet_fire_elemental : pet.guardian_fire_elemental;
        break;
      case elemental::EARTH:
        p = pe ? pet.pet_earth_elemental : pet.guardian_earth_elemental;
        break;
      case elemental::STORM:
        p = pe ? pet.pet_storm_elemental : pet.guardian_storm_elemental;
        break;
    }

    if ( !p )
    {
      return expr_t::create_constant( name, 0.0 );
    }

    if ( util::str_compare_ci( splits[ 2 ], "active" ) )
    {
      return make_fn_expr( name, [ p ]() { return static_cast<double>( !p->is_sleeping() ); } );
    }
    else if ( util::str_compare_ci( splits[ 2 ], "remains" ) )
    {
      return make_fn_expr( name, [ p ]() { return p->expiration ? p->expiration->remains().total_seconds() : 0.0; } );
    }
    else
    {
      return player_t::create_expression( name );
    }
  }

  if ( util::str_compare_ci( splits[ 0 ], "feral_spirit" ) )
  {
    if ( talent.elemental_spirits->ok() && !find_action( "feral_spirit" ) )
    {
      return expr_t::create_constant( name, 0 );
    }
    else if ( !talent.elemental_spirits->ok() && !find_action( "feral_spirit" ) )
    {
      return expr_t::create_constant( name, 0 );
    }

    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( name, [ this ]() {
        if ( talent.elemental_spirits->ok() )
        {
          return pet.fire_wolves.n_active_pets() + pet.frost_wolves.n_active_pets() +
                 pet.lightning_wolves.n_active_pets();
        }
        else
        {
          return pet.spirit_wolves.n_active_pets();
        }
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
        if ( talent.elemental_spirits->ok() )
        {
          std::vector<pet_t*> max_remains;

          if ( auto p = pet.fire_wolves.active_pet_max_remains() )
          {
            max_remains.push_back( p );
          }

          if ( auto p = pet.frost_wolves.active_pet_max_remains() )
          {
            max_remains.push_back( p );
          }

          if ( auto p = pet.lightning_wolves.active_pet_max_remains() )
          {
            max_remains.push_back( p );
          }

          auto it = std::max_element( max_remains.cbegin(), max_remains.cend(), max_remains_fn );
          if ( it == max_remains.end() )
          {
            return 0.0;
          }

          return ( *it )->expiration ? ( *it )->expiration->remains().total_seconds() : 0.0;
        }
        else
        {
          auto p = pet.spirit_wolves.active_pet_max_remains();
          return p ? p->expiration->remains().total_seconds() : 0;
        }
      } );
    }
  }

  if ( util::str_compare_ci( splits[ 0 ], "vesper_totem_dmg_charges" ) )
  {
    return make_ref_expr( name, vesper_totem_dmg_charges );
  }

  if ( util::str_compare_ci( splits[ 0 ], "vesper_totem_heal_charges" ) )
  {
    return make_ref_expr( name, vesper_totem_heal_charges );
  }

  if ( util::str_compare_ci( splits[ 0 ], "vesper_totem_used_charges" ) )
  {
    return make_ref_expr( name, vesper_totem_used_charges );
  }

  if ( util::str_compare_ci( splits[ 0 ], "ti_lightning_bolt" ) )
  {
    return make_fn_expr( name, [ this ]() {
        return !action.ti_trigger || action.ti_trigger == action.lightning_bolt_ti;
    } );
  }

  if ( util::str_compare_ci( splits[ 0 ], "ti_chain_lightning" ) )
  {
    return make_fn_expr( name, [ this ]() {
        return action.ti_trigger == action.chain_lightning_ti;
    } );
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
  if ( specialization() == SHAMAN_ENHANCEMENT && ( talent.primordial_wave.ok() || covenant.necrolord->ok() ) )
  {
    action.lightning_bolt_pw = new lightning_bolt_t( this, execute_type::PRIMORDIAL_WAVE );
  }

  if ( specialization() == SHAMAN_ELEMENTAL && ( talent.primordial_wave.ok() || covenant.necrolord->ok() ) )
  {
    action.lava_burst_pw = new lava_burst_t( this, execute_type::PRIMORDIAL_WAVE );
  }

  if ( talent.thorims_invocation.ok() )
  {
    action.lightning_bolt_ti = new lightning_bolt_t( this, execute_type::THORIMS_INVOCATION );
    action.chain_lightning_ti = new chain_lightning_t( this, execute_type::THORIMS_INVOCATION );
  }

  if ( talent.lightning_rod.ok() )
  {
    action.lightning_rod = new lightning_rod_damage_t( this );
  }

  // Generic Actions
  action.flame_shock = new flame_shock_t( this );
  action.flame_shock->background = true;
  action.flame_shock->cooldown = get_cooldown( "flame_shock_secondary" );
  action.flame_shock->base_costs[ RESOURCE_MANA ] = 0;

  // Legendaries
  if ( legendary.deeply_rooted_elements.ok() || talent.deeply_rooted_elements.ok() )
  {
    action.dre_ascendance = new ascendance_dre_t( this );
  }
}

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();
  add_option( opt_bool( "raptor_glyph", raptor_glyph ) );
  // option allows Elemental Shamans to switch to a different APL
  add_option( opt_func( "rotation", [ this ]( sim_t*, util::string_view, util::string_view val ) {
    if ( util::str_compare_ci( val, "standard" ) )
      options.rotation = ROTATION_STANDARD;
    else if ( util::str_compare_ci( val, "simple" ) )
      options.rotation = ROTATION_SIMPLE;
    else
      return false;
    return true;
  } ) );
  add_option( opt_int( "shaman.chain_harvest_allies", opt_sl.chain_harvest_allies, 0, 5 ) );
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
}

// shaman_t::init_spells ====================================================

void shaman_t::init_spells()
{
  //
  // Generic spells
  //
  spec.mail_specialization          = find_specialization_spell( "Mail Specialization" );
  constant.matching_gear_multiplier = spec.mail_specialization->effectN( 1 ).percent();
  spec.shaman                       = find_spell( 137038 );

  // Elemental
  spec.elemental_shaman  = find_specialization_spell( "Elemental Shaman" );
  spec.maelstrom         = find_specialization_spell( 343725 );

  spec.lightning_bolt_2  = find_rank_spell( "Lightning Bolt", "Rank 2" );
  spec.lava_burst_2      = find_rank_spell( "Lava Burst", "Rank 2" );

  // Enhancement
  spec.critical_strikes   = find_specialization_spell( "Critical Strikes" );
  spec.dual_wield         = find_specialization_spell( "Dual Wield" );
  spec.enhancement_shaman = find_specialization_spell( "Enhancement Shaman" );
  spec.stormbringer       = find_specialization_spell( "Stormbringer" );

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
  talent.maelstrom_weapon = _CT( "Maelstrom Weapon" );
  // Row 3
  talent.earth_shield     = _CT( "Earth Shield" );
  talent.fire_and_ice     = _CT( "Fire and Ice" );
  talent.capacitor_totem  = _CT( "Capacitor Totem" );
  // Row 4
  talent.spiritwalkers_grace = _CT( "Spiritwalker's Grace" );
  talent.ancestral_defense   = _CT( "Ancestral Defense" );
  talent.static_charge       = _CT( "Static Charge" );
  talent.guardians_cudgel    = _CT( "Guardian's Cudgel" );
  talent.flurry              = _CT( "Flurry" );
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
  // Row 8
  talent.natures_swiftness       = _CT( "Nature's Swiftness" );
  talent.thunderstorm            = _CT( "Thunderstorm" );
  talent.totemic_focus           = _CT( "Totemic Focus ");
  talent.surging_shields         = _CT( "Surging Shields" );
  talent.go_with_the_flow        = _CT( "Go With the Flow ");
  // Row 9i  
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
  talent.feral_lunge = _ST( "Feral Lunge" );
  talent.lashing_flames = _ST( "Lashing Flames" );
  talent.ashen_catalyst = _ST( "Ashen Catalyst" );
  // Row 5
  talent.doom_winds = _ST( "Doom Winds" );
  talent.sundering = _ST( "Sundering" );
  talent.overflowing_maelstrom = _ST( "Overflowing Maelstrom" );
  talent.windfury_totem = _ST( "Windfury Totem" );
  talent.fire_nova = _ST( "Fire Nova" );
  talent.hailstorm = _ST( "Hailstorm" );
  talent.elemental_weapons = _ST( "Elemental Weapons" );
  talent.crashing_storms = _ST( "Crashing Storms" );
  // Row 6
  talent.storms_wrath = _ST( "Storm's Wrath" );
  talent.crash_lightning = _ST( "Crash Lightning" );
  talent.stormflurry = _ST( "Stormflurry" );
  talent.ice_strike = _ST( "Ice Strike" );
  // Row 7
  talent.stormblast = _ST( "Stormblast" );
  talent.gathering_storms = _ST( "Gathering Storms" );
  talent.chain_lightning2 = _ST( "Chain Lightning" );
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
  talent.earthquake = _ST( "Earthquake" );
  talent.elemental_fury = _ST( "Elemental Fury" );
  talent.fire_elemental = _ST( "Fire Elemental" );
  talent.storm_elemental = _ST( "Storm Elemental" );
  // Row 3
  talent.inundate = _ST( "Inundate" );
  // Row 4
  talent.call_of_thunder    = _ST( "Call of Thunder" );
  talent.flow_of_power      = _ST( "Flow of Power" );
  talent.lava_surge         = _ST( "Lava Surge" );
  // Row 5
  talent.unrelenting_calamity = _ST( "Unrelenting Calamity" );
  talent.icefury              = _ST( "Icefury" );
  talent.swelling_maelstrom   = _ST( "Swelling Maelstrom" );
  talent.echo_of_the_elements = _ST( "Echo of the Elements" );
  talent.call_of_fire         = _ST( "Call of Fire" );
  // Row 6
  talent.stormkeeper = find_talent_spell( talent_tree::SPECIALIZATION, 392714 );
  talent.electrified_shocks = _ST( "Electrified Shocks" );
  talent.flux_melting = _ST( "Flux Melting" );
  talent.aftershock = _ST( "Aftershock" );
  talent.surge_of_power         = _ST( "Surge of Power" );
  talent.flames_of_the_cauldron = _ST( "Flames of the Cauldron" );
  // Row 7
  talent.flash_of_lightning = _ST( "Flash of Lightning" );
  talent.eye_of_the_storm = _ST( "Eye of the Storm" );
  talent.power_of_the_maelstrom = _ST( "Power of the Maelstrom" );
  talent.master_of_the_elements = _ST( "Master of the Elements" );
  talent.improved_flametongue_weapon = _ST( "Improved Flametongue Weapon" );
  // Row 8
  talent.deeply_rooted_elements = _ST( "Deeply Rooted Elements" );
  talent.liquid_magma_totem = _ST( "Liquid Magma Totem" );
  talent.primal_elementalist = _ST( "Primal Elementalist" );
  // Row 9
  talent.echoes_of_great_sundering = _ST( "Echoes of Great Sundering" );
  talent.elemental_equilibrium = _ST( "Elemental Equilibrium" );
  talent.rolling_magma = _ST( "Rolling Magma" );
  talent.echo_chamber = _ST( "Echo Chamber" );
  talent.oath_of_the_far_seer = _ST( "Oath of the Far Seer" );
  talent.magma_chamber = _ST( "Magma Chamber" );
  talent.searing_flames = _ST( "Searing Flames" );
  // Row 10
  talent.stormkeeper2 = find_talent_spell( talent_tree::SPECIALIZATION, 392763 );
  talent.lightning_rod = _ST( "Lightning Rod" );
  talent.primordial_surge = _ST( "Primordial Surge" );
  talent.mountains_will_fall = _ST( "Mountains Will Fall" );
  talent.further_beyond = _ST( "Further Beyond" );
  talent.skybreakers_fiery_demise = _ST( "Skybreaker's Fiery Demise" );
  talent.windspeakers_lava_resurgence = _ST( "Windspeaker's Lava Resurgence" );

  // Covenants
  covenant.necrolord = find_covenant_spell( "Primordial Wave" );
  covenant.night_fae = find_covenant_spell( "Fae Transfusion" );
  covenant.venthyr   = find_covenant_spell( "Chain Harvest" );
  covenant.kyrian    = find_covenant_spell( "Vesper Totem" );

  // Covenant-specific conduits
  conduit.essential_extraction = find_conduit_spell( "Essential Extraction" );
  conduit.lavish_harvest       = find_conduit_spell( "Lavish Harvest" );
  conduit.tumbling_waves       = find_conduit_spell( "Tumbling Waves" );
  conduit.elysian_dirge        = find_conduit_spell( "Elysian Dirge" );

  // Elemental Conduits
  conduit.call_of_flame = find_conduit_spell( "Call of Flame" );
  conduit.high_voltage  = find_conduit_spell( "High Voltage" );
  conduit.pyroclastic_shock = find_conduit_spell( "Pyroclastic Shock" );
  conduit.shake_the_foundations = find_conduit_spell( "Shake the Foundations" );

  // Enhancement Conduits
  conduit.chilled_to_the_core = find_conduit_spell( "Chilled to the Core" );
  conduit.focused_lightning = find_conduit_spell( "Focused Lightning" );
  conduit.magma_fist = find_conduit_spell( "Magma Fist" );
  conduit.unruly_winds = find_conduit_spell( "Unruly Winds" );

  // Shared Legendaries
  legendary.ancestral_reminder     = find_runeforge_legendary( "Ancestral Reminder" );
  legendary.chains_of_devastation  = find_runeforge_legendary( "Chains of Devastation" );
  legendary.deeptremor_stone       = find_runeforge_legendary( "Deeptremor Stone" );
  legendary.deeply_rooted_elements = find_runeforge_legendary( "Deeply Rooted Elements" );

  // Covenant Legendaries
  legendary.splintered_elements = find_runeforge_legendary( "Splintered Elements" );
  legendary.raging_vesper_vortex = find_runeforge_legendary( "Raging Vesper Vortex" );
  legendary.seeds_of_rampant_growth = find_runeforge_legendary( "Seeds of Rampant Growth" );
  legendary.elemental_conduit      = find_runeforge_legendary( "Elemental Conduit" );

  // Elemental Legendaries
  legendary.skybreakers_fiery_demise     = find_runeforge_legendary( "Skybreaker's Fiery Demise" );
  legendary.elemental_equilibrium        = find_runeforge_legendary( "Elemental Equilibrium" );
  legendary.echoes_of_great_sundering    = find_runeforge_legendary( "Echoes of Great Sundering" );
  legendary.windspeakers_lava_resurgence = find_runeforge_legendary( "Windspeaker's Lava Resurgence" );

  // Enhancement Legendaries
  legendary.doom_winds                = find_runeforge_legendary( "Doom Winds" );
  legendary.legacy_of_the_frost_witch = find_runeforge_legendary( "Legacy of the Frost Witch" );
  legendary.primal_lava_actuators     = find_runeforge_legendary( "Primal Lava Actuators" );
  legendary.witch_doctors_wolf_bones  = find_runeforge_legendary( "Witch Doctor's Wolf Bones" );

  //
  // Misc spells
  //
  spell.resurgence          = find_spell( 101033 );
  spell.maelstrom_weapon    = find_spell( 187881 );
  spell.feral_spirit        = find_spell( 228562 );
  spell.fire_elemental      = find_spell( 188592 );
  spell.storm_elemental     = find_spell( 157299 );
  spell.earth_elemental     = find_spell( 188616 );
  spell.flametongue_weapon  = find_spell( 318038 );
  spell.windfury_weapon     = find_spell( 319773 );
  spell.inundate            = find_spell( 378777 );

  spell.t28_2pc_enh        = sets->set( SHAMAN_ENHANCEMENT, T28, B2 );
  spell.t28_4pc_enh        = sets->set( SHAMAN_ENHANCEMENT, T28, B4 );

  //spell.t28_2pc_ele        = sets->set( SHAMAN_ELEMENTAL, T28, B2 );
  // this is the actually useful spell
  spell.t28_2pc_ele        = find_spell( 364523 );
  spell.t28_4pc_ele        = sets->set( SHAMAN_ELEMENTAL, T28, B4 );

  spell.t29_2pc_ele        = find_spell( 394651 );
  spell.t29_4pc_ele        = find_spell( 394670 );
  // spell.t29_2pc_ele        = sets->set( SHAMAN_ELEMENTAL, T29, B2 );
  // spell.t29_4pc_ele        = sets->set( SHAMAN_ELEMENTAL, T29, B4 );

  // Misc spell-related init
  max_active_flame_shock   = as<unsigned>( find_class_spell( "Flame Shock" )->max_targets() );

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
    resources.base[ RESOURCE_MAELSTROM ] = 100 + talent.swelling_maelstrom.spell()->effectN( 1 ).base_value();
  }

  if ( specialization() == SHAMAN_RESTORATION )
  {
    resources.base[ RESOURCE_MANA ]               = 20000;
    resources.initial_multiplier[ RESOURCE_MANA ] = 1.0 + spec.restoration_shaman->effectN( 5 ).percent();
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
  if ( talent.primal_elementalist->ok() )
  {
    return ( pet.pet_fire_elemental && !pet.pet_fire_elemental->is_sleeping() ) ||
           ( pet.pet_storm_elemental && !pet.pet_storm_elemental->is_sleeping() );
  }
  else
  {
    return ( pet.guardian_fire_elemental && !pet.guardian_fire_elemental->is_sleeping() ) ||
           ( pet.guardian_storm_elemental && !pet.guardian_storm_elemental->is_sleeping() );
  }
}

pet_t* shaman_t::get_active_elemental_pet() const
{
  if ( talent.primal_elementalist->ok() )
  {
    if( pet.pet_fire_elemental && !pet.pet_fire_elemental->is_sleeping() )
    {
      return pet.pet_fire_elemental;
    }
    if ( pet.pet_storm_elemental && !pet.pet_storm_elemental->is_sleeping() )
    {
      return pet.pet_storm_elemental;
    }
  }
  else
  {
    if ( pet.guardian_fire_elemental && !pet.guardian_fire_elemental->is_sleeping() )
    {
      return pet.guardian_fire_elemental;
    }
    if ( pet.guardian_storm_elemental && !pet.guardian_storm_elemental->is_sleeping() )
    {
      return pet.guardian_storm_elemental;
    }
  }
  return nullptr;
}

void shaman_t::summon_feral_spirits( timespan_t duration, unsigned n, bool t28 )
{
  // No elemental spirits selected, just summon normal pets
  if ( !talent.elemental_spirits->ok() )
  {
    pet.spirit_wolves.spawn( duration, n );
    for ( unsigned i = 0; i < n; ++i )
    {
      buff.earthen_weapon->trigger( 1, buff_t::DEFAULT_VALUE(), -1, duration );
    }
  }
  else
  {
    // This summon evaluates the wolf type to spawn as the roll, instead of rolling against
    // the available pool of wolves to spawn.
    while ( n )
    {
      // +1, because the normal spirit wolves are enum value 0
      switch ( static_cast<wolf_type_e>( 1 + rng().range( 0, 3 ) ) )
      {
        case FIRE_WOLF:
          pet.fire_wolves.spawn( duration );
          buff.molten_weapon->trigger( 1, buff_t::DEFAULT_VALUE(), -1, duration );
          break;
        case FROST_WOLF:
          pet.frost_wolves.spawn( duration );
          buff.icy_edge->trigger( 1, buff_t::DEFAULT_VALUE(), -1, duration );
          break;
        case LIGHTNING_WOLF:
          pet.lightning_wolves.spawn( duration );
          buff.crackling_surge->trigger( 1, buff_t::DEFAULT_VALUE(), -1, duration );
          break;
        default:
          assert( 0 );
          break;
      }
      n--;
    }
  }

  // Enhancement T28 bonus will only override the buff from manually cast spell
  // if the new duration exceeds the remaining duration of the buff.
  if ( !t28 || ( t28 && duration > buff.feral_spirit_maelstrom->remains() ) )
  {
    buff.feral_spirit_maelstrom->trigger( 1, duration );
  }
}

void shaman_t::summon_fire_elemental( timespan_t duration )
{
  if ( talent.storm_elemental->ok() )
  {
    return;
  }

  if ( talent.primal_elementalist->ok() )
  {
    if ( pet.pet_fire_elemental->is_sleeping() )
    {
      if ( pet.pet_earth_elemental )
      {
        pet.pet_earth_elemental->demise();
      }
      pet.pet_fire_elemental->summon( duration );
      pet.pet_fire_elemental->get_cooldown( "meteor" )->reset( false );
    }
    else
    {
      auto new_duration = pet.pet_fire_elemental->expiration->remains();
      new_duration += duration;
      pet.pet_fire_elemental->expiration->reschedule( new_duration );
    }
  }
  else
  {
    if ( pet.guardian_fire_elemental->is_sleeping() )
    {
      pet.guardian_fire_elemental->summon( duration );
    }
    else
    {
      auto new_duration = pet.guardian_fire_elemental->expiration->remains();
      new_duration += duration;
      pet.guardian_fire_elemental->expiration->reschedule( new_duration );
    }
  }
}

void shaman_t::summon_storm_elemental( timespan_t duration )
{
  if ( !talent.storm_elemental->ok() )
  {
    return;
  }

  if ( talent.primal_elementalist->ok() )
  {
    if ( pet.pet_storm_elemental->is_sleeping() )
    {
      if ( pet.pet_earth_elemental )
      {
        pet.pet_earth_elemental->demise();
      }
      pet.pet_storm_elemental->summon( duration );
      pet.pet_storm_elemental->get_cooldown( "tempest" )->reset( false );
    }
    else
    {
      auto new_duration = pet.pet_storm_elemental->expiration->remains();
      new_duration += duration;
      pet.pet_storm_elemental->expiration->reschedule( new_duration );
    }
  }
  else
  {
    if ( pet.guardian_storm_elemental->is_sleeping() )
    {
      pet.guardian_storm_elemental->summon( duration );
    }
    else
    {
      auto new_duration = pet.guardian_storm_elemental->expiration->remains();
      new_duration += duration;
      pet.guardian_storm_elemental->expiration->reschedule( new_duration );
    }
  }
}

// ==========================================================================
// Shaman Tracking - code blocks that shall not be doublicated
// ==========================================================================

void shaman_t::track_magma_chamber()
{
  switch ( buff.magma_chamber->check() )
  {
    case 1:
      proc.magma_chamber_1->occur();
      break;
    case 2:
      proc.magma_chamber_2->occur();
      break;
    case 3:
      proc.magma_chamber_3->occur();
      break;
    case 4:
      proc.magma_chamber_4->occur();
      break;
    case 5:
      proc.magma_chamber_5->occur();
      break;
    case 6:
      proc.magma_chamber_6->occur();
      break;
    case 7:
      proc.magma_chamber_7->occur();
      break;
    case 8:
      proc.magma_chamber_8->occur();
      break;
    case 9:
      proc.magma_chamber_9->occur();
      break;
    case 10:
      proc.magma_chamber_10->occur();
      break;
    case 11:
      proc.magma_chamber_11->occur();
      break;
    case 12:
      proc.magma_chamber_12->occur();
      break;
    case 13:
      proc.magma_chamber_13->occur();
      break;
    case 14:
      proc.magma_chamber_14->occur();
      break;
    case 15:
      proc.magma_chamber_15->occur();
      break;
    case 16:
      proc.magma_chamber_16->occur();
      break;
    case 17:
      proc.magma_chamber_17->occur();
      break;
    case 18:
      proc.magma_chamber_18->occur();
      break;
    case 19:
      proc.magma_chamber_19->occur();
      break;
    case 20:
      proc.magma_chamber_20->occur();
      break;
    default:
      break;
  }
}

void shaman_t::track_t29_2pc_ele()
{
  switch ( buff.t29_2pc_ele->check() )
  {
    case 1:
      proc.t29_2pc_ele_1->occur();
      break;
    case 2:
      proc.t29_2pc_ele_2->occur();
      break;
    case 3:
      proc.t29_2pc_ele_3->occur();
      break;
    case 4:
      proc.t29_2pc_ele_4->occur();
      break;
    case 5:
      proc.t29_2pc_ele_5->occur();
      break;
    default:
      break;
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

void shaman_t::trigger_vesper_totem( const action_state_t* state )
{
  if ( !buff.vesper_totem->up() )
  {
    return;
  }

  // TODO: Whitelist?

  ground_aoe_event_t* current_event = nullptr;
  unsigned* charges = nullptr;
  switch ( state->action->type )
  {
    case ACTION_ATTACK:
    case ACTION_SPELL:
    {
      if ( bugs == true && std::string_view{state->action->name()} == "Fire Nova" )
      {
        current_event = ev_vesper_totem_heal;
        charges = &( vesper_totem_heal_charges );
        break;
      }
      current_event = ev_vesper_totem_dmg;
      charges = &( vesper_totem_dmg_charges );
      break;
    }
    case ACTION_HEAL:
    {
      current_event = ev_vesper_totem_heal;
      charges = &( vesper_totem_heal_charges );
      break;
    }
    default:
      break;
  }

  if ( charges == nullptr || *charges == 0 )
  {
    return;
  }

  if ( sim->debug )
  {
    sim->out_debug.print(
      "{} vesper_totem state, type={}, trigger={}, dmg_charges={}, heal_charges={}, "
      "used_charges={}",
      name(), state->action->type, state->action->name(), vesper_totem_dmg_charges,
      vesper_totem_heal_charges, vesper_totem_used_charges );
  }

  // Pulse the ground aoe event manually
  current_event->execute();

  // Aand cancel the event .. shaman_t::vesper_totem_*_event will have the new event pointer,
  // so we cancel the cached pointer here
  event_t::cancel( current_event );
  (*charges)--;
  vesper_totem_used_charges++;

  if ( legendary.raging_vesper_vortex->ok() &&
      vesper_totem_used_charges == legendary.raging_vesper_vortex->effectN( 1 ).base_value() &&
      !vesper_totem_damage->target_list().empty() )
  {
    if ( sim-> debug )
    {
      sim->out_debug.print(
        "{} raging_vesper_vortex trigger, target={}, dmg_charges={}, heal_charges={}",
        name(), vesper_totem_target->name(), vesper_totem_dmg_charges,
        vesper_totem_heal_charges );
    }
    raging_vesper_vortex->set_target( vesper_totem_damage->target );
    raging_vesper_vortex->execute();
    vesper_totem_used_charges = 0;
  }

  if ( !vesper_totem_dmg_charges && !vesper_totem_heal_charges )
  {
    buff.vesper_totem->expire();
  }
}

void shaman_t::trigger_legacy_of_the_frost_witch( unsigned consumed_stacks )
{
  if ( !legendary.legacy_of_the_frost_witch.ok() && !talent.legacy_of_the_frost_witch.ok() )
  {
    return;
  }

  lotfw_counter += consumed_stacks;

  auto threshold = as<unsigned>( legendary.legacy_of_the_frost_witch->effectN( 1 ).base_value() );
  if ( threshold == 0 )
  {
    threshold = as<unsigned>( talent.legacy_of_the_frost_witch->effectN( 2 ).base_value() );
  }

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
  if ( !legendary.deeply_rooted_elements.ok() && !talent.deeply_rooted_elements.ok() )
  {
    return;
  }

  double proc_chance = 0.0;

  if ( specialization() == SHAMAN_ELEMENTAL )
  {
    auto lvb = debug_cast<lava_burst_t*>( state->action );
    if ( lvb->type != execute_type::NORMAL )
    {
      return;
    }

    proc_chance = legendary.deeply_rooted_elements->effectN( 2 ).percent();
    if ( proc_chance == 0 )
    {
      proc_chance = talent.deeply_rooted_elements->effectN( 2 ).percent();
    }
  }
  else if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    auto sb = debug_cast<stormstrike_base_t*>( state->action );
    if ( sb->stormflurry )
    {
      return;
    }

    proc_chance = legendary.deeply_rooted_elements->effectN( 3 ).percent();
    if ( proc_chance == 0 )
    {
      proc_chance = talent.deeply_rooted_elements->effectN( 3 ).percent();
    }
  }
  // No resto support for now
  else
  {
    return;
  }

  if ( !rng().roll( proc_chance ) )
  {
    return;
  }

  action.dre_ascendance->set_target( state->target );
  action.dre_ascendance->execute();
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

double shaman_t::composite_maelstrom_gain_coefficient( const action_state_t* /* state */ ) const
{
  double m = 1.0;
  return m;
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

  double proc_chance = spell.windfury_weapon->proc_chance();
  double proc_mul = mastery.enhanced_elements->effectN( 4 ).mastery_value() *
    ( 1.0 + talent.storms_wrath->effectN( 2 ).percent() );

  proc_chance += cache.mastery() * proc_mul;
  proc_chance += buff.doom_winds_buff->stack_value();

  if ( buff.doom_winds_talent->up() )
  {
    proc_chance *= talent.windfury_weapon.ok() *
                  talent.doom_winds->effectN( 1 ).base_value();
  }

  if ( state->action->weapon->slot == SLOT_MAIN_HAND && rng().roll( proc_chance ) )
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

    double chance = conduit.unruly_winds.percent();
    if ( chance == 0.0 )
    {
      chance = talent.unruly_winds->effectN( 1 ).percent();
    }

    if ( rng().roll( chance ) )
    {
      trigger_secondary_ability( state, a );
      proc.windfury_uw->occur();
    }

    attack->proc_wf->occur();
  }
}

void shaman_t::trigger_maelstrom_weapon( const action_state_t* state )
{
  if ( !talent.maelstrom_weapon.ok() )
  {
    return;
  }

  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr &&
          "Maelstrom Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_maelstrom_weapon )
    return;

  if ( buff.ghost_wolf->check() )
  {
    return;
  }

  double proc_chance = talent.maelstrom_weapon->proc_chance();
  proc_chance += buff.witch_doctors_wolf_bones->stack_value();
  proc_chance += buff.witch_doctors_ancestry->stack_value();

  if ( rng().roll( proc_chance ) )
  {
    buff.maelstrom_weapon->increment();
    attack->proc_mw->occur();
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

void shaman_t::trigger_lightning_shield( const action_state_t* state )
{
  // Maybe replace with defensive implementation
  return;

  if ( !buff.lightning_shield->up() )
  {
    return;
  }

  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_lightning_shield )
  {
    return;
  }
}

void shaman_t::trigger_lava_surge( bool fireheart, bool primordial_surge )
{
  if ( buff.lava_surge->check() )
  {
    if ( fireheart )
    {
      proc.wasted_lava_surge_fireheart->occur();
    }
    else if (primordial_surge)
    {
      proc.wasted_lava_surge_primordial_surge->occur();
    }
    else
    {
      proc.wasted_lava_surge->occur();
    }
  }

  if ( fireheart )
  {
    proc.lava_surge_fireheart->occur();
  }
  else if (primordial_surge)
  {
    proc.lava_surge_primordial_surge->occur();
  }
  else
  {
    proc.lava_surge->occur();
  }

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
  if ( !legendary.splintered_elements.ok() && !talent.splintered_elements->ok() )
  {
    return;
  }

  auto count_duplicates = secondary->target_list().size();
  if ( count_duplicates == 0 )
  {
    return;
  }

  double value = 0.0;
  if ( legendary.splintered_elements.ok() )
  {
    value = legendary.splintered_elements->effectN( 1 ).percent();
  }
  else
  {
    value = talent.splintered_elements->effectN( 1 ).percent();
  }

  buff.splintered_elements->trigger( count_duplicates, value );
}

void shaman_t::trigger_flash_of_lightning()
{
  if ( !talent.stormkeeper.enabled() && !talent.stormkeeper2.enabled() &&
       !talent.storm_elemental.enabled() )
  {
    return;
  }

  if ( talent.storm_elemental.enabled() )
  {
    cooldown.storm_elemental->adjust( talent.flash_of_lightning.spell()->effectN( 1 ).time_value(), false );
  }
  if ( talent.stormkeeper.enabled() || talent.stormkeeper2.enabled() )
  {
    cooldown.stormkeeper->adjust( talent.flash_of_lightning.spell()->effectN( 1 ).time_value(), false );
  }
  if ( talent.natures_swiftness.enabled() )
  {
    cooldown.natures_swiftness->adjust( talent.flash_of_lightning.spell()->effectN( 1 ).time_value(), false );
  }

  proc.flash_of_lightning->occur();
}

void shaman_t::trigger_lightning_rod_damage( const action_state_t* state )
{
  if ( !talent.lightning_rod.ok() )
  {
    return;
  }

  if ( state->action->result_is_miss( state->result ) )
  {
    return;
  }

  range::for_each( sim->target_non_sleeping_list, [ this, state ]( player_t* target ) {
    if ( !get_target_data( target )->debuff.lightning_rod->up() )
    {
      return;
    }

    double amount = state->result_amount * talent.lightning_rod->effectN( 2 ).percent();

    action.lightning_rod->base_dd_min = action.lightning_rod->base_dd_max = amount;
    action.lightning_rod->set_target( target );
    action.lightning_rod->execute();
  } );
}

void shaman_t::trigger_swirling_maelstrom()
{
  if ( !talent.swirling_maelstrom.ok() )
  {
    return;
  }

  buff.maelstrom_weapon->trigger( talent.swirling_maelstrom->effectN( 1 ).base_value() );
  for ( auto i = 0; i < talent.swirling_maelstrom->effectN( 1 ).base_value(); ++i )
  {
    proc.maelstrom_weapon_sm->occur();
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
    ->add_invalidate( CACHE_ATTACK_SPEED );
  buff.natures_swiftness = make_buff( this, "natures_swiftness", talent.natures_swiftness );

  buff.elemental_blast_crit = make_buff<buff_t>( this, "elemental_blast_critical_strike", find_spell( 118522 ) )
    ->set_default_value_from_effect_type(A_MOD_ALL_CRIT_CHANCE)
    ->apply_affecting_aura(spec.elemental_shaman)
    ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );

  buff.elemental_blast_haste = make_buff<buff_t>( this, "elemental_blast_haste", find_spell( 173183 ) )
    ->set_default_value_from_effect_type(A_HASTE_ALL)
    ->apply_affecting_aura(spec.elemental_shaman)
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.elemental_blast_mastery = make_buff<buff_t>( this, "elemental_blast_mastery", find_spell( 173184 ) )
    ->set_default_value_from_effect_type(A_MOD_MASTERY_PCT)
    ->apply_affecting_aura(spec.elemental_shaman)
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );

  buff.stormkeeper = make_buff( this, "stormkeeper", find_spell( 191634 ) )
    ->set_cooldown( timespan_t::zero() )  // Handled by the action
    ->set_default_value_from_effect( 2 ); // Damage bonus as default value

  buff.t29_2pc_ele = make_buff( this, "seismic_accumulation", spell.t29_2pc_ele )
                      ->set_default_value_from_effect(1)
                      ->set_trigger_spell( sets->set( SHAMAN_ELEMENTAL, T29, B2 ) );
  buff.t29_4pc_ele = make_buff<buff_t>( this, "elemental_mastery", spell.t29_4pc_ele )
                      ->set_default_value_from_effect(1)
                      ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
                      ->set_trigger_spell( sets->set( SHAMAN_ELEMENTAL, T29, B4 ) );

  if ( legendary.ancestral_reminder->ok() )
  {
    auto legendary_spell = find_spell( 336741 );
    auto buff            = buffs.bloodlust;
    buff->modify_duration( legendary_spell->effectN( 1 ).time_value() );
    buff->modify_default_value( legendary_spell->effectN( 2 ).percent() );
  }

  // Covenants
  buff.primordial_wave = make_buff( this, "primordial_wave", find_spell( 327164 ) )
    ->set_trigger_spell( talent.primordial_wave );
  buff.vesper_totem = make_buff( this, "vesper_totem", covenant.kyrian )
                          ->set_cooldown( 0_s ) // Handled by the action
                          ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                            if ( new_ == 0 )
                            {
                              event_t::cancel( ev_vesper_totem_dmg );
                              event_t::cancel( ev_vesper_totem_heal );
                              vesper_totem_used_charges = 0;
                              vesper_totem_dmg_charges = 0;
                              vesper_totem_heal_charges = 0;
                            }
                          } );
  // Covenant Legendaries
  buff.splintered_elements = new splintered_elements_buff_t( this );

  buff.seeds_of_rampant_growth = make_buff( this, "seeds_of_rampant_growth", find_spell( 358945 ) )
                                  ->set_default_value_from_effect_type(A_MOD_ALL_CRIT_CHANCE)
                                  ->set_pct_buff_type(STAT_PCT_BUFF_CRIT);

  //
  // Elemental
  //
  buff.lava_surge = make_buff( this, "lava_surge", find_spell( 77762 ) )
                        ->set_activated( false )
                        ->set_chance( 1.0 );  // Proc chance is handled externally

  buff.surge_of_power =
      make_buff( this, "surge_of_power", talent.surge_of_power )->set_duration( find_spell( 285514 )->duration() );

  buff.icefury = make_buff( this, "icefury", talent.icefury )
                     ->set_cooldown( timespan_t::zero() )  // Handled by the action
                     ->set_default_value( talent.icefury->effectN( 2 ).percent() );

  buff.master_of_the_elements = make_buff( this, "master_of_the_elements", talent.master_of_the_elements->effectN(1).trigger() )
          ->set_default_value( talent.master_of_the_elements->effectN( 2 ).percent() );

  buff.wind_gust = make_buff( this, "wind_gust", find_spell( 263806 ) )
                       ->set_default_value( find_spell( 263806 )->effectN( 1 ).percent() );

  buff.echoes_of_great_sundering =
      make_buff( this, "echoes_of_great_sundering", find_spell( 336217 ) )
        ->set_default_value( legendary.echoes_of_great_sundering.ok() 
            ? find_spell( 336217 )->effectN( 2 ).percent() 
            : talent.elemental_blast.ok()
                ? talent.echoes_of_great_sundering->effectN( 2 ).percent()
                : talent.echoes_of_great_sundering->effectN( 1 ).percent() )
        ->set_trigger_spell( 
            legendary.echoes_of_great_sundering.ok()
                ? legendary.echoes_of_great_sundering
                : talent.echoes_of_great_sundering
        );

  buff.flux_melting = make_buff( this, "flux_melting", talent.flux_melting->effectN( 1 ).trigger() )
                            ->set_default_value( talent.flux_melting->effectN( 1 ).trigger()->effectN(1).percent() );

  buff.primordial_surge_lava_burst_buff = make_buff( this, "primordial_surge_lava_burst_buff", find_spell( 396484 ))
                                    ->set_default_value( find_spell( 396484 )->effectN( 1 ).percent() );

  buff.primordial_surge = make_buff( this, "primordial_surge", find_spell( 387622 ) )
          ->set_tick_callback( [ this ]( buff_t* /* b */, int, timespan_t ) { 
              trigger_lava_surge( false, true );
              buff.primordial_surge_lava_burst_buff->trigger();
            } );

  buff.magma_chamber = make_buff( this, "magma_chamber", find_spell( 381933 ) )
                            ->set_default_value( talent.magma_chamber->effectN( 1 ).percent() );

  buff.oath_of_the_far_seer = make_buff<buff_t>( this, "oath_of_the_far_seer", talent.oath_of_the_far_seer )
                                ->set_default_value( talent.oath_of_the_far_seer->effectN( 1 ).percent() )
                                ->set_duration( buff.ascendance->buff_duration() )
                                ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.power_of_the_maelstrom =
      make_buff( this, "power_of_the_maelstrom", talent.power_of_the_maelstrom->effectN( 1 ).trigger() )
          ->set_default_value( talent.power_of_the_maelstrom->effectN( 1 ).trigger()->effectN( 1 ).base_value() );

  buff.chains_of_devastation_chain_heal = make_buff( this, "chains_of_devastation_chain_heal", find_spell( 336737 ) );
  buff.chains_of_devastation_chain_lightning =
      make_buff( this, "chains_of_devastation_chain_lightning", find_spell( 336736 ) );

  buff.windspeakers_lava_resurgence = make_buff( 
                                        this, 
                                        "windspeakers_lava_resurgence", 
                                        legendary.windspeakers_lava_resurgence.ok() 
                                              ? find_spell( 336065 ) 
                                              : talent.windspeakers_lava_resurgence->effectN(1).trigger() 
                                      )
                                      ->set_default_value(
                                        legendary.windspeakers_lava_resurgence.ok()
                                              ? find_spell( 336065 )->effectN( 1 ).percent()
                                              : talent.windspeakers_lava_resurgence->effectN( 1 ).trigger()->effectN( 1 ).percent() 
                                      );

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

  buff.fire_elemental = make_buff( this, "fire_elemental", spell.fire_elemental )
                            ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_TICK_TIME )
                            ->apply_affecting_conduit( conduit.call_of_flame );

  buff.fireheart = make_buff( this, "fireheart", spell.t28_2pc_ele )
                       ->set_duration( buff.fire_elemental->buff_duration() )
                       ->set_tick_callback( [ this ]( buff_t* /* b */, int, timespan_t ) {
                         trigger_lava_surge( true );
                       } )
                       ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  //
  // Enhancement
  //
  buff.lightning_shield = new lightning_shield_buff_t( this );
  buff.feral_spirit_maelstrom = make_buff( this, "feral_spirit", find_spell( 333957 ) )
                                    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
                                      buff.maelstrom_weapon->trigger( b->data().effectN( 1 ).base_value() );
                                      proc.maelstrom_weapon_fs->occur();
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
  buff.gathering_storms = make_buff( this, "gathering_storms", find_spell( 198300 ) )
      ->set_default_value_from_effect( 1 );
  buff.ashen_catalyst = make_buff( this, "ashen_catalyst", find_spell( 390371 ) )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
    ->set_trigger_spell( talent.ashen_catalyst );
  buff.witch_doctors_ancestry = make_buff<buff_t>( this, "witch_doctors_ancestry",
      talent.witch_doctors_ancestry )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_PROC_CHANCE );

  //NYI in game - spell data is lacking.
  buff.t29_4pc_enh = make_buff<buff_t>( this, "t29_4pc_enh" )  //, find_spell( 393693 ) )
                         ->set_default_value( 0.01f )
                         ->set_max_stack( 10 )
                         ->set_duration( timespan_t::from_seconds( 4 ) )
                         ->set_default_value_from_effect_type( A_HASTE_ALL )
                         ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.t29_2pc_enh = make_buff<buff_t>( this, "t29_2pc_enh", find_spell( 394677 ) )
                   ->set_default_value_from_effect( 1 )
                   ->set_max_stack( 1 );
                   

  // Buffs stormstrike and lava lash after using crash lightning
  buff.crash_lightning = make_buff( this, "crash_lightning", find_spell( 187878 ) );
  // Buffs crash lightning with extra damage, after using chain lightning
  buff.cl_crash_lightning = new cl_crash_lightning_buff_t( this );

  buff.hot_hand = new hot_hand_buff_t( this );
  /*buff.hot_hand = make_buff( this, "hot_hand", talent.hot_hand->effectN( 1 ).trigger() )
    ->set_trigger_spell( talent.hot_hand )
    ->set_default_value_from_effect( 2 )
    -> set_stack_change_callback( [this]( buff_t*, int, int ) {
      cooldown.lava_lash->adjust_recharge_multiplier();
    } );*/
  buff.spirit_walk  = make_buff( this, "spirit_walk", talent.spirit_walk );
  buff.stormbringer = make_buff( this, "stormbringer", find_spell( 201846 ) );
  buff.maelstrom_weapon = new maelstrom_weapon_buff_t( this );
  buff.hailstorm        = make_buff( this, "hailstorm", find_spell( 334196 ) )
                            ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
                            ->set_max_stack( 10 ); // TODO: Fix hardcoding when client data updates
  buff.static_accumulation = make_buff( this, "static_accumulation", find_spell( 384437 ) )
    ->set_default_value( talent.static_accumulation->effectN( 1 ).base_value() )
    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
      buff.maelstrom_weapon->trigger( b->value() );
      for ( int i = 0; i < b->check_value(); ++i )
      {
        proc.maelstrom_weapon_sa->occur();
      }
    } );
  buff.doom_winds_talent = make_buff( this, "doom_winds_talent", talent.doom_winds );
  buff.ice_strike = make_buff( this, "ice_strike", talent.ice_strike->effectN( 3 ).trigger() )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER );

  //
  // Restoration
  //
  buff.spiritwalkers_grace =
      make_buff( this, "spiritwalkers_grace", find_specialization_spell( "Spiritwalker's Grace" ) )
          ->set_cooldown( timespan_t::zero() );
  buff.tidal_waves =
      make_buff( this, "tidal_waves", spec.tidal_waves->ok() ? find_spell( 53390 ) : spell_data_t::not_found() );


  // Legendary buffs
  buff.doom_winds_buff = make_buff<buff_t>( this, "doom_winds",
    legendary.doom_winds->effectN( 1 ).trigger() )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_PROC_CHANCE )
    ->set_stack_change_callback( [this]( buff_t*, int, int new_ ) {
        if ( new_ > 0 )
        {
          buff.doom_winds_debuff->trigger();
        }
    } );
  buff.doom_winds_debuff = make_buff<buff_t>( this, "doom_winds_debuff",
    buff.doom_winds_buff->data().effectN( 2 ).trigger() );
  buff.legacy_of_the_frost_witch = make_buff<buff_t>( this, "legacy_of_the_frost_witch",
      find_spell( legendary.legacy_of_the_frost_witch.ok() ? 335901 : 384451 ) )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC );
  buff.primal_lava_actuators = make_buff<buff_t>( this, "primal_lava_actuators",
      find_spell( 335896 ) )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC );
  buff.witch_doctors_wolf_bones = make_buff<buff_t>( this, "witch_doctors_wolf_bones",
      legendary.witch_doctors_wolf_bones )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_PROC_CHANCE );
  buff.elemental_equilibrium = make_buff<buff_t>( this, "elemental_equilibrium",
      find_spell( 347348 ) )
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
  gain.high_voltage            = get_gain( "High Voltage" );
  gain.ascendance              = get_gain( "Ascendance" );
  gain.resurgence              = get_gain( "resurgence" );
  gain.feral_spirit            = get_gain( "Feral Spirit" );
  gain.fire_elemental          = get_gain( "Fire Elemental" );
  gain.spirit_of_the_maelstrom = get_gain( "Spirit of the Maelstrom" );
  gain.chain_harvest_heal      = get_gain( "Chain Harvest Heal" );
  gain.inundate                = get_gain( "Inundate" );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.lava_surge                               = get_proc( "Lava Surge" );
  proc.lava_surge_fireheart                     = get_proc( "Lava Surge: Fireheart" );
  proc.lava_surge_primordial_surge              = get_proc( "Lava Surge: Primordial Surge" );
  proc.lava_surge_windspeakers_lava_resurgence  = get_proc( "Lava Surge: Windspeaker's Fiery Demise" );
  proc.wasted_lava_surge                        = get_proc( "Lava Surge: Wasted" );
  proc.wasted_lava_surge_fireheart              = get_proc( "Lava Surge: Wasted Fireheart" );
  proc.wasted_lava_surge_primordial_surge       = get_proc( "Lava Surge: Primordial Surge" );
  proc.surge_during_lvb                         = get_proc( "Lava Surge: During Lava Burst" );

  proc.surge_of_power_lightning_bolt = get_proc( "Surge of Power: Lightning Bolt" );
  proc.surge_of_power_lava_burst     = get_proc( "Surge of Power: Lava Burst" );
  proc.surge_of_power_frost_shock    = get_proc( "Surge of Power: Frost Shock" );
  proc.surge_of_power_flame_shock    = get_proc( "Surge of Power: Flame Shock" );
  proc.surge_of_power_wasted    = get_proc( "Surge of Power: Wasted" );

  proc.aftershock           = get_proc( "Aftershock" );
  proc.flash_of_lightning   = get_proc( "Flash of Lightning" );
  proc.further_beyond       = get_proc( "Further Beyond" );
  proc.lightning_rod        = get_proc( "Lightning Rod" );
  proc.searing_flames       = get_proc( "Searing Flames" );
  proc.magma_chamber_1      = get_proc( "Magma Chamber 1" );
  proc.magma_chamber_2      = get_proc( "Magma Chamber 2" );
  proc.magma_chamber_3      = get_proc( "Magma Chamber 3" );
  proc.magma_chamber_4      = get_proc( "Magma Chamber 4" );
  proc.magma_chamber_5      = get_proc( "Magma Chamber 5" );
  proc.magma_chamber_6      = get_proc( "Magma Chamber 6" );
  proc.magma_chamber_7      = get_proc( "Magma Chamber 7" );
  proc.magma_chamber_8      = get_proc( "Magma Chamber 8" );
  proc.magma_chamber_9      = get_proc( "Magma Chamber 9" );
  proc.magma_chamber_10      = get_proc( "Magma Chamber 10" );
  proc.magma_chamber_11      = get_proc( "Magma Chamber 11" );
  proc.magma_chamber_12      = get_proc( "Magma Chamber 12" );
  proc.magma_chamber_13      = get_proc( "Magma Chamber 13" );
  proc.magma_chamber_14      = get_proc( "Magma Chamber 14" );
  proc.magma_chamber_15      = get_proc( "Magma Chamber 15" );
  proc.magma_chamber_16      = get_proc( "Magma Chamber 16" );
  proc.magma_chamber_17      = get_proc( "Magma Chamber 17" );
  proc.magma_chamber_18      = get_proc( "Magma Chamber 18" );
  proc.magma_chamber_19      = get_proc( "Magma Chamber 19" );
  proc.magma_chamber_20      = get_proc( "Magma Chamber 20" );

  proc.pyroclastic_shock    = get_proc( "Pyroclastic Shock" );

  proc.windfury_uw            = get_proc( "Windfury: Unruly Winds" );
  proc.maelstrom_weapon_fs    = get_proc( "Maelstrom Weapon: Feral Spirit" );
  proc.maelstrom_weapon_ea    = get_proc( "Maelstrom Weapon: Elemental Assault" );
  proc.maelstrom_weapon_cttc  = get_proc( "Maelstrom Weapon: Chilled to the Core" );
  proc.maelstrom_weapon_ft    = get_proc( "Maelstrom Weapon: Fae Transfusion" );
  proc.maelstrom_weapon_sm    = get_proc( "Maelstrom Weapon: Swirling Maelstrom" );
  proc.maelstrom_weapon_pw    = get_proc( "Maelstrom Weapon: Primordial Wave" );
  proc.maelstrom_weapon_sa    = get_proc( "Maelstrom Weapon: Static Accumulation" );
  proc.stormflurry            = get_proc( "Stormflurry" );

  proc.t28_4pc_enh       = get_proc( "Set Bonus: Tier28 4PC Enhancement" );
  proc.t28_4pc_ele_cd_reduction = get_proc( "Set Bonus: Tier28 4PC Elemental CD Reduction" );
  proc.t28_4pc_ele_cd_extension = get_proc( "Set Bonus: Tier28 4PC Elemental CD Extension" );

  proc.t29_2pc_ele_1 = get_proc( "Set Bonus: Tier29 2PC Elemental spender empowerement, stack 1" );
  proc.t29_2pc_ele_2 = get_proc( "Set Bonus: Tier29 2PC Elemental spender empowerement, stack 2" );
  proc.t29_2pc_ele_3 = get_proc( "Set Bonus: Tier29 2PC Elemental spender empowerement, stack 3" );
  proc.t29_2pc_ele_4 = get_proc( "Set Bonus: Tier29 2PC Elemental spender empowerement, stack 4" );
  proc.t29_2pc_ele_5 = get_proc( "Set Bonus: Tier29 2PC Elemental spender empowerement, stack 5" );
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

  if ( legendary.elemental_equilibrium.ok() || talent.elemental_equilibrium.ok() )
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

// shaman_t::create_special_effects =========================================

void shaman_t::create_special_effects()
{
  player_t::create_special_effects();

  // Create a base-bones special effect for vesper totem, handle actual init in
  // shaman_t::init_special_effect
  if ( covenant.kyrian->ok() )
  {
    special_effect_t* effect = new special_effect_t( this );
    effect->type = SPECIAL_EFFECT_EQUIP;
    effect->source = SPECIAL_EFFECT_SOURCE_COVENANT;
    effect->spell_id = covenant.kyrian->id();
    effect->cooldown_ = 0_s;
    effect->proc_flags2_ = PF2_CAST_DAMAGE | PF2_CAST_HEAL;
    effect->custom_init = [ this ]( special_effect_t& effect ) {
      new vesper_totem_cb_t( effect, this );
    };

    special_effects.push_back( effect );
  }
}

// shaman_t::init_special_effects ===========================================

void shaman_t::init_special_effects()
{
  player_t::init_special_effects();

  // Custom trigger condition for Bron's Call to Arms. Completely overrides the trigger
  // behavior of the generic proc to get control back to the Shaman class module in terms
  // of what triggers it.
  //
  // 2021-07-04 Eligible spells that can proc Bron's Call to Arms:
  // - Any foreground amount spell / attack
  // - Ascendance
  // - Stormstrike/Windstrike "base" (or might be strike itself + icd handling)
  //
  // Note, also has to handle the ICD and pet-related trigger conditions.
  callbacks.register_callback_trigger_function( 333950,
      dbc_proc_callback_t::trigger_fn_type::TRIGGER,
      [this]( const dbc_proc_callback_t* cb, action_t* a, action_state_t* ) {
      if ( cb->cooldown->down() )
      {
        return false;
      }

      // Defer finding the bron pet until the first proc attempt
      if ( !pet.bron )
      {
        pet.bron = find_pet( "bron" );
        assert( pet.bron );
      }

      if ( pet.bron->is_active() )
      {
        return false;
      }

      if ( a->type == ACTION_ATTACK )
      {
        auto attack = dynamic_cast<shaman_attack_t*>( a );
        if ( attack && attack->may_proc_bron )
        {
          attack->bron_proc->occur();
          return true;
        }
      }
      else if ( a->type == ACTION_HEAL )
      {
        auto heal = dynamic_cast<shaman_heal_t*>( a );
        if ( heal && heal->may_proc_bron )
        {
          heal->bron_proc->occur();
          return true;
        }
      }
      else if ( a->type == ACTION_SPELL )
      {
        auto spell = dynamic_cast<shaman_spell_t*>( a );
        if ( spell && spell->may_proc_bron )
        {
          spell->bron_proc->occur();
          return true;
        }
      }

      return false;
  } );
}

// shaman_t::init_special_effect ============================================

void shaman_t::init_special_effect( special_effect_t& effect )
{
  switch ( effect.driver()->id() )
  {
    // Bron's Call to Arms
    //
    // Shaman module has custom triggering logic (defined above) so override the initial
    // proc flags so we get wider trigger attempts than the core implementation. The
    // overridden proc condition above will take care of filtering out actions that are
    // not allowed to proc it.
    case 333950:
      effect.proc_flags2_ |= PF2_CAST;
      break;
    default:
      break;
  }
}

// shaman_t::apply_affecting_auras ==========================================

void shaman_t::apply_affecting_auras( action_t& action )
{
  // Generic
  action.apply_affecting_aura( spec.shaman );
  action.apply_affecting_aura( talent.call_of_the_elements );

  // Specialization
  action.apply_affecting_aura( spec.elemental_shaman );
  action.apply_affecting_aura( spec.enhancement_shaman );
  action.apply_affecting_aura( spec.restoration_shaman );
  action.apply_affecting_aura( spec.lightning_bolt_2 );

  // Talents
  action.apply_affecting_aura( talent.call_of_fire );
  action.apply_affecting_aura( talent.call_of_thunder );
  action.apply_affecting_aura( talent.elemental_assault );
  action.apply_affecting_aura( talent.elemental_fury );
  action.apply_affecting_aura( talent.improved_lightning_bolt );
  action.apply_affecting_aura( talent.molten_assault );
  action.apply_affecting_aura( talent.natures_fury );
  action.apply_affecting_aura( talent.thundershock );
  action.apply_affecting_aura( talent.totemic_surge );
  action.apply_affecting_aura( talent.unrelenting_calamity );
  action.apply_affecting_aura( talent.crashing_storms );
  action.apply_affecting_aura( talent.healing_stream_totem );
  action.apply_affecting_aura( talent.stormkeeper );
  action.apply_affecting_aura( talent.stormkeeper2 );
  action.apply_affecting_aura( talent.oath_of_the_far_seer );
  action.apply_affecting_aura( talent.fire_and_ice );
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
  std::string elemental_potion = ( true_level >= 51 ) ? "potion_of_spectral_intellect" :
                                 ( true_level >= 45 ) ? "potion_of_unbridled_fury" :
                                 "disabled";

  std::string enhancement_potion = ( true_level >= 51 ) ? "potion_of_spectral_agility" :
                                   ( true_level >= 45 ) ? "potion_of_unbridled_fury" :
                                   "disabled";

  std::string restoration_potion = ( true_level >= 51 ) ? "potion_of_spectral_intellect" :
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
  std::string elemental_flask = ( true_level >= 51 ) ? "spectral_flask_of_power" :
                                ( true_level >= 45 ) ? "greater_flask_of_endless_fathoms" :
                                "disabled";

  std::string enhancement_flask = ( true_level >= 51 ) ? "spectral_flask_of_power" :
                                  ( true_level >= 45 ) ? "greater_flask_of_the_currents" :
                                  "disabled";

  std::string restoration_flask = ( true_level >= 51 ) ? "spectral_flask_of_power" :
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
  std::string elemental_food = ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
                               ( true_level >= 45 ) ? "mechdowels_big_mech" :
                               "disabled";

  std::string enhancement_food = ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
                                 ( true_level >= 45 ) ? "baked_port_tato" :
                                 "disabled";

  std::string restoration_food = ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
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
  return ( true_level >= 60 ) ? "veiled" :
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
      return "disabled";
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

// shaman_t::init_action_list_elemental =====================================

void shaman_t::init_action_list_elemental()
{
  action_priority_list_t* precombat     = get_action_priority_list( "precombat" );
  action_priority_list_t* def           = get_action_priority_list( "default" );

  action_priority_list_t* single_target    = get_action_priority_list( "single_target" );
  action_priority_list_t* aoe              = get_action_priority_list( "aoe" );
  /*
  if ( options.rotation == ROTATION_STANDARD )
  {
    action_priority_list_t* se_single_target = get_action_priority_list( "se_single_target" );
    precombat->add_action( "flask" );
    precombat->add_action( "food" );
    precombat->add_action( "augmentation" );
    precombat->add_action( "snapshot_stats" );
    precombat->add_action( this, "Earth Elemental", "if=!talent.primal_elementalist.enabled" );
    precombat->add_talent( this, "Stormkeeper",
                           "if=talent.stormkeeper.enabled&(raid_event.adds.count<3|raid_event.adds.in>50)" );
    precombat->add_action( this, "Fire Elemental" );
    precombat->add_talent( this, "Elemental Blast",
                           "if=talent.elemental_blast.enabled&spell_targets.chain_lightning<3" );
    precombat->add_action( this, "Lava Burst",
                           "if=!talent.elemental_blast.enabled&spell_targets.chain_lightning<3|buff.stormkeeper.up" );
    precombat->add_action( this, "Chain Lightning",
                           "if=!talent.elemental_blast.enabled&spell_targets.chain_lightning>=3&!buff.stormkeeper.up" );
    precombat->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent" );
    precombat->add_action( "potion" );

    def->add_action( this, "Spiritwalker's Grace", "moving=1,if=movement.distance>6" );
    def->add_action( this, "Wind Shear" );
    def->add_action( "potion" );
    def->add_action( "use_items" );
    def->add_action( this, "Flame Shock",
                     "if=(!talent.elemental_blast.enabled)&!ticking&!pet.storm_elemental.active&(spell_targets.chain_"
                     "lightning<3|talent.master_of_the_elements.enabled|talent.skybreakers_fiery_demise|runeforge.skybreakers_fiery_demise.equipped)" );
    def->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,cycle_targets=1,if=!buff.primordial_wave.up&"
                     "(!pet.storm_elemental.active|spell_targets.chain_lightning<3&buff.wind_gust.stack<20|soulbind.lead_by_example.enabled|runeforge.splintered_elements.equipped|talent.splintered_elements)&"
                     "(spell_targets.chain_lightning<5|talent.master_of_the_elements.enabled|talent.skybreakers_fiery_demise|runeforge.skybreakers_fiery_demise.equipped|"
                     "soulbind.lead_by_example.enabled|runeforge.splintered_elements.equipped|talent.splintered_elements)&!buff.splintered_elements.up" );
    def->add_action( this, "Flame Shock",
                     "if=!ticking&(!pet.storm_elemental.active|spell_targets.chain_lightning<3&buff.wind_gust.stack<20)"
                     "&(spell_targets.chain_lightning<3|talent.master_of_the_elements.enabled|runeforge.skybreakers_"
                     "fiery_demise.equipped)" );
    def->add_action( this, "Fire Elemental" );
    def->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
    def->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "vesper_totem,if=covenant.kyrian" );
    def->add_action(
        "fae_transfusion,if=covenant.night_fae&!runeforge.seeds_of_rampant_growth.equipped&"
        "(!talent.master_of_the_elements.enabled|buff.master_of_the_elements.up)&"
        "spell_targets.chain_lightning<3" );
    def->add_action(
        "fae_transfusion,if=covenant.night_fae&runeforge.seeds_of_rampant_growth.equipped&"
        "(!talent.master_of_the_elements.enabled|buff.master_of_the_elements.up|spell_targets.chain_lightning>=3)&"
        "(cooldown.fire_elemental.remains>20|cooldown.storm_elemental.remains>20)" );
    def->add_action(
        "run_action_list,name=aoe,if=active_enemies>2&(spell_targets.chain_lightning>2|spell_targets.lava_beam>2)" );
    def->add_action( "run_action_list,name=single_target,if=!talent.storm_elemental.enabled&active_enemies<=2" );
    def->add_action( "run_action_list,name=se_single_target,if=talent.storm_elemental.enabled&active_enemies<=2" );

    aoe->add_talent( this, "Storm Elemental", "if=!pet.storm_elemental.active" );
    aoe->add_action( "chain_harvest" );
    aoe->add_talent( this, "Stormkeeper", "if=talent.stormkeeper.enabled" );
    aoe->add_action( this, "Flame Shock",
                     "if=(active_dot.flame_shock<2&active_enemies<=3&cooldown.primordial_wave.remains<16&!pet.storm_elemental.active|active_dot.flame_"
                     "shock<1&active_enemies>=4&!pet.storm_elemental.active&talent.master_of_the_elements.enabled)|("
                     "(talent.skybreakers_fiery_demise|runeforge.skybreakers_fiery_demise.equipped)&!pet.storm_elemental.active)|"
                     "((runeforge.splintered_elements.equipped|talent.splintered_elements)&(active_dot.flame_shock<3&(!talent.echoes_of_great_sundering&!runeforge.echoes_of_great_sundering.equipped)|active_dot.flame_shock<4)&"
                     "(cooldown.primordial_wave.remains<16|buff.primordial_wave.up)),target_if=refreshable" );
    aoe->add_action( this, "Flame Shock",
                     "if=!active_dot.flame_shock&!pet.storm_elemental.active&(talent."
                     "master_of_the_elements.enabled|talent.skybreakers_fiery_demise|runeforge.skybreakers_fiery_demise.equipped)|((runeforge.splintered_elements.equipped|talent.splintered_elements)&!ticking&buff.primordial_wave.up)" );
    aoe->add_talent( this, "Ascendance",
                     "if=talent.ascendance.enabled&(!pet.storm_elemental.active)&(!talent.icefury.enabled|!buff."
                     "icefury.up&!cooldown.icefury.up)" );
    aoe->add_talent( this, "Liquid Magma Totem", "if=talent.liquid_magma_totem.enabled" );
    aoe->add_action( this, "Chain Lightning",
                     "if=spell_targets.chain_lightning<4&buff.master_of_the_elements.up&maelstrom<50" );
    aoe->add_action( this, "Earth Shock",
                     "if=(talent.echoes_of_gret_sundering|runeforge.echoes_of_great_sundering.equipped)&!buff.echoes_of_great_sundering.up" );
    aoe->add_action( this, "Lava Burst","target_if=dot.flame_shock.remains,if=spell_targets.chain_lightning<4&(!pet.storm_elemental.active)&(buff.lava_surge.up&!buff."
                     "master_of_the_elements.up&talent.master_of_the_elements.enabled)" );
    aoe->add_action( this, "Earthquake",
                     "if=spell_targets.chain_lightning>=2&!talent.echoes_of_great_sundering&!runeforge.echoes_of_great_sundering.equipped&(talent.master_"
                     "of_the_elements.enabled&maelstrom>=50&!buff.master_of_the_elements.up)" );
    aoe->add_action( this, "Lava Burst",
                     "target_if=dot.flame_shock.remains,if=covenant.necrolord&(talent.echoes_of_great_sundering|runeforge.echoes_of_great_sundering.equipped)&"
                     "set_bonus.tier28_4pc&buff.lava_surge.up&!buff.primordial_wave.up" );
    aoe->add_action( this, "Lava Burst",
                     "target_if=dot.flame_shock.remains,if=buff.lava_surge.up&buff.primordial_wave.up&(buff.primordial_wave.remains<3*gcd|"
                     "active_dot.flame_shock=spell_targets.chain_lightning|active_dot.flame_shock=3&(!talent.echoes_of_great_sundering&!runeforge.echoes_of_great_sundering.equipped)|"
                     "active_dot.flame_shock=4)" );
    aoe->add_action( this, "Lava Burst",
                     "target_if=dot.flame_shock.remains,if=spell_targets.chain_lightning<4&runeforge.skybreakers_fiery_"
                     "demise.equipped&buff.lava_surge."
                     "up&talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&maelstrom>=50" );
    aoe->add_action( this, "Lava Burst",
                     "target_if=dot.flame_shock.remains,if=(spell_targets.chain_lightning<4&runeforge.skybreakers_"
                     "fiery_demise.equipped&talent.master_of_"
                     "the_elements.enabled)|(talent.master_of_the_elements.enabled&maelstrom>=50&!buff.master_of_the_"
                     "elements.up&((!talent.echoes_of_great_sundering&!runeforge.echoes_of_great_sundering.equipped)|buff.echoes_of_great_sundering.up)&!"
                     "runeforge.skybreakers_fiery_demise.equipped&!talent.skybreakers_fiery_demise)" );
    aoe->add_action( this, "Lava Burst",
                     "target_if=dot.flame_shock.remains,if=spell_targets.chain_lightning=4&runeforge.skybreakers_fiery_"
                     "demise.equipped&buff.lava_surge."
                     "up&talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&maelstrom>=50" );
    aoe->add_action( this, "Earthquake", "if=spell_targets.chain_lightning>=2" );
    aoe->add_action( this, "Chain Lightning", "if=buff.stormkeeper.remains<3*gcd*buff.stormkeeper.stack" );
    aoe->add_action(
        this, "Lava Burst",
        "target_if=dot.flame_shock.remains,if=set_bonus.tier28_4pc&buff.lava_surge.up&!buff.primordial_wave.up" );
    aoe->add_action(
        this, "Lava Burst",
        "if=set_bonus.tier28_4pc&buff.lava_surge.up&!buff.primordial_wave.up" );
    aoe->add_action(
        this, "Lava Burst",
        "if=buff.lava_surge.up&spell_targets.chain_lightning<4&(!pet.storm_elemental.active)&dot.flame_shock.ticking" );
    aoe->add_talent(
        this, "Elemental Blast",
        "if=talent.elemental_blast.enabled&spell_targets.chain_lightning<5&(!pet.storm_elemental.active)" );
    aoe->add_action( this, "Lava Beam", "if=talent.ascendance.enabled" );
    aoe->add_action( this, "Chain Lightning" );
    aoe->add_action( this, "Lava Burst", "moving=1,if=buff.lava_surge.up&cooldown_react" );
    aoe->add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
    aoe->add_action( this, "Frost Shock", "moving=1" );

    single_target->add_action( this, "Lightning Bolt", "if=(buff.stormkeeper.remains<1.1*gcd*buff.stormkeeper.stack)" );
    single_target->add_action(
        this, "Frost Shock",
        "if=talent.icefury.enabled&buff.icefury.up&buff.icefury.remains<1.1*gcd*buff.icefury.stack" );
    single_target->add_action( this, "Flame Shock",
                               "target_if=(!ticking|dot.flame_shock.remains<=gcd|talent.ascendance.enabled&"
                               "dot.flame_shock.remains<(cooldown.ascendance.remains+buff.ascendance.duration)&"
                               "cooldown.ascendance.remains<4)&(buff.lava_surge.up|!buff.bloodlust.up)" );
    single_target->add_action(
        this, "Flame Shock",
        "if=buff.primordial_wave.up,target_if=min:dot.flame_shock.remains,cycle_targets=1,target_if=refreshable" );
    single_target->add_talent( this, "Ascendance",
                               "if=talent.ascendance.enabled&(time>=60|buff.bloodlust.up)&(cooldown.lava_burst.remains>"
                               "0)&(!talent.icefury.enabled|!buff.icefury.up&!cooldown.icefury.up)" );
    single_target->add_action( this, "Lava Burst",
                               "if=buff.lava_surge.up&(talent.windspeakers_lava_resurgence|runeforge.windspeakers_lava_resurgence.equipped|!buff.master_of_"
                               "the_elements.up&talent.master_of_the_elements.enabled)" );
    single_target->add_talent( this, "Elemental Blast", "if=talent.elemental_blast.enabled&(maelstrom<70)&!buff.ascendance.up" );
    single_target->add_talent(
        this, "Stormkeeper",
        "if=talent.stormkeeper.enabled&(raid_event.adds.count<3|raid_event.adds.in>50)&(maelstrom<44)" );
    single_target->add_talent( this, "Liquid Magma Totem", "if=talent.liquid_magma_totem.enabled" );
    single_target->add_action(
        this, "Earthquake",
        "if=buff.echoes_of_great_sundering.up&talent.master_of_the_elements.enabled&buff.master_of_the_elements.up" );
    single_target->add_action( this, "Lightning Bolt",
                               "if=buff.stormkeeper.up&buff.master_of_the_elements.up&maelstrom<60" );
    single_target->add_action(
        this, "Earthquake",
        "if=buff.echoes_of_great_sundering.up&(talent.master_of_the_elements.enabled&(buff.master_of_the_elements.up|"
        "cooldown.lava_burst.remains>0&maelstrom>=92|spell_targets.chain_lightning<2&buff.stormkeeper.up&cooldown.lava_"
        "burst.remains<=gcd)|!talent.master_of_the_elements.enabled|cooldown.elemental_blast.remains<=1.1*gcd*2)" );
    single_target->add_action( this, "Earthquake",
                               "if=spell_targets.chain_lightning>1&!dot.flame_shock.refreshable&!runeforge.echoes_of_"
                               "great_sundering.equipped&(!talent.master_of_the_elements.enabled|buff.master_of_the_"
                               "elements.up|cooldown.lava_burst.remains>0&maelstrom>=92)" );
    single_target->add_action(
        this, "Earth Shock",
        "if=(talent.windspeakers_lava_resurgence|runeforge.windspeakers_lava_resurgence.equipped)&buff.ascendance.up" );
    single_target->add_action( this, "Lava Burst",
                               "if=cooldown_react&(!buff.master_of_the_elements.up&buff.icefury.up)" );
    single_target->add_action( this, "Lava Burst",
                               "if=cooldown_react&charges>talent.echo_of_the_elements.enabled&!buff.icefury.up" );
    single_target->add_action( this, "Lava Burst",
                               "if=talent.echo_of_the_elements.enabled&!buff.master_of_the_elements.up&maelstrom>=50&!"
                               "buff.echoes_of_great_sundering.up" );
    single_target->add_action(
        this, "Earth Shock",
        "if=(talent.echoes_of_great_sundering|runeforge.echoes_of_great_sundering.equipped|spell_targets.chain_lightning<2)&(talent.master_of_the_"
        "elements.enabled&!buff.echoes_of_great_sundering.up&(buff.master_of_the_elements.up|maelstrom>=92|spell_"
        "targets.chain_lightning<2&buff.stormkeeper.up&cooldown.lava_burst.remains<=gcd)|!talent.master_of_the_"
        "elements.enabled|cooldown.elemental_blast.remains<=1.1*gcd*2)" );
    single_target->add_action( this, "Frost Shock",
                               "if=talent.icefury.enabled&talent.master_of_the_elements.enabled&buff.icefury.up&buff."
                               "master_of_the_elements.up" );
    single_target->add_action( this, "Lava Burst", "if=buff.ascendance.up" );
    single_target->add_action( this, "Lava Burst", "if=cooldown_react&!talent.master_of_the_elements.enabled" );
    single_target->add_talent( this, "Icefury",
                               "if=talent.icefury.enabled&!(maelstrom>35&cooldown.lava_burst.remains<=0)" );
    single_target->add_action( this, "Frost Shock",
                               "if=talent.icefury.enabled&buff.icefury.up&(buff.icefury.remains<gcd*4*buff.icefury."
                               "stack|buff.stormkeeper.up|!talent.master_of_the_elements.enabled)" );
    single_target->add_action( this, "Lava Burst" );
    single_target->add_action( "bag_of_tricks" );
    single_target->add_action( this, "Flame Shock", "target_if=refreshable" );
    single_target->add_action( this, "Frost Shock",
                               "if=(talent.elemental_equilibrium|runeforge.elemental_equilibrium.equipped)&!buff.elemental_equilibrium_debuff.up&!"
                               "talent.elemental_blast.enabled" );
    single_target->add_action( "chain_harvest" );
    single_target->add_action( this, "Frost Shock",
                               "if=talent.icefury.enabled&buff.icefury.up" );
    single_target->add_action( "fleshcraft,if=soulbind.volatile_solvent&!buff.volatile_solvent_humanoid.up,interrupt_immediate=1,interrupt_global=1,interrupt_if=soulbind.volatile_solvent" );
    single_target->add_action( this, "Earth Elemental",
                               "if=!talent.primal_elementalist.enabled|!pet.fire_elemental.active" );
    single_target->add_action( this, "Chain Lightning", "if=spell_targets.chain_lightning>1" );
    single_target->add_action( this, "Lightning Bolt" );
    single_target->add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
    single_target->add_action( this, "Flame Shock", "moving=1,if=movement.distance>6" );
    single_target->add_action( this, "Frost Shock", "moving=1" );

    se_single_target->add_talent( this, "Storm Elemental", "if=!pet.storm_elemental.active" );
    se_single_target->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,cycle_targets=1,if=!buff.primordial_wave.up&!buff.splintered_elements.up" );
    se_single_target->add_action( this, "Frost Shock",
                                  "if=talent.icefury.enabled&buff.icefury.up&buff.icefury.remains<1.1*gcd*buff.icefury."
                                  "stack&buff.wind_gust.stack<18" );
    se_single_target->add_action( this,
                                  "Flame Shock", "target_if=refreshable" );
    se_single_target->add_action(
        this, "Frost Shock",
        "if=talent.icefury.enabled&buff.icefury.up&buff.icefury.remains<1.1*gcd*buff.icefury.stack" );
    se_single_target->add_talent( this, "Elemental Blast", "if=talent.elemental_blast.enabled" );
    se_single_target->add_talent( this, "Stormkeeper", "if=talent.stormkeeper.enabled" );
    se_single_target->add_action( this, "Lava Burst",
                                  "if=(buff.wind_gust.stack<18&!buff.bloodlust.up)|buff.lava_surge.up" );
    se_single_target->add_action( this, "Lava Burst,target_if=dot.flame_shock.remains",
                                  "if=buff.lava_surge.up&buff.primordial_wave.up" );
    se_single_target->add_action( this, "Lightning Bolt", "if=buff.stormkeeper.up" );
    se_single_target->add_action( this, "Earthquake", "if=buff.echoes_of_great_sundering.up" );
    se_single_target->add_action(
        this, "Earth Shock",
        "if=spell_targets.chain_lightning<2&maelstrom>=60&(buff.wind_gust.stack<20|maelstrom>90)|((runeforge.echoes_of_"
        "great_sundering.equipped|talent.echoes_of_great_sundering)&!buff.echoes_of_great_sundering.up)|talent.windspeakers_lava_resurgence|runeforge.windspeakers_lava_resurgence.equipped" );
    se_single_target->add_action( this, "Earthquake",
                                  "if=(spell_targets.chain_lightning>1)&(!dot.flame_shock.refreshable)" );
    se_single_target->add_action( this, "Chain Lightning",
                                  "if=active_enemies>1&pet.storm_elemental.active&buff.bloodlust.up" );
    se_single_target->add_action( this, "Lightning Bolt", "if=pet.storm_elemental.active&buff.bloodlust.up" );
    se_single_target->add_action( this, "Lava Burst", "if=buff.ascendance.up" );
    se_single_target->add_action( this, "Lava Burst", "if=cooldown_react" );
    se_single_target->add_action( this, "Icefury",
                                  "if=talent.icefury.enabled&!(maelstrom>75&cooldown.lava_burst.remains<=0)" );
    se_single_target->add_action( this, "Lava Burst", "if=cooldown_react&charges>talent.echo_of_the_elements.enabled" );
    se_single_target->add_action( this, "Frost Shock", "if=talent.icefury.enabled&buff.icefury.up" );
    se_single_target->add_action( "chain_harvest" );
    se_single_target->add_action( "fleshcraft,if=soulbind.volatile_solvent&!buff.volatile_solvent_humanoid.up,interrupt_immediate=1,interrupt_global=1,interrupt_if=soulbind.volatile_solvent" );
    se_single_target->add_action(
        this, "Earth Elemental",
        "if=!talent.primal_elementalist.enabled|talent.primal_elementalist.enabled&(!pet.storm_elemental.active)" );
    se_single_target->add_action( this, "Chain Lightning",
                                  "if=active_enemies>1&(spell_targets.chain_lightning>1|spell_targets.lava_beam>1)" );
    se_single_target->add_action( this, "Lightning Bolt" );
    se_single_target->add_action( "bag_of_tricks" );
    se_single_target->add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
    se_single_target->add_action( this, "Flame Shock", "moving=1,if=movement.distance>6" );
    se_single_target->add_action( this, "Frost Shock", "moving=1" );
  }*/
  if (options.rotation == ROTATION_SIMPLE || options.rotation == ROTATION_STANDARD) {
    precombat->add_action( "flask" );
    precombat->add_action( "food" );
    precombat->add_action( "augmentation" );
    precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
    precombat->add_action( this, "Flametongue Weapon", "if=talent.improved_flametongue_weapon.enabled", "Ensure weapon enchant is applied." );
    precombat->add_action( "potion" );

    // "Default" APL controlling logic flow to specialized sub-APLs
    def->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
    def->add_action( "wind_shear", "Interrupt of casts." );
    // Racials
    def->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
    def->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "bag_of_tricks,if=!talent.ascendance.enabled|!buff.ascendance.up" );
    def->add_action( "use_items" );
    def->add_action( "auto_attack" );

    // Pick APL to run
    def->add_action(
        "run_action_list,name=aoe,if=active_enemies>2&(spell_targets.chain_lightning>2|spell_targets.lava_beam>2)" );
    def->add_action( "run_action_list,name=single_target" );

    // Aoe APL
    aoe->add_action( "fire_elemental", "Keep your cooldowns rolling." );
    aoe->add_action( "storm_elemental", "Keep your cooldowns rolling." );
    aoe->add_action( "stormkeeper", "Keep your cooldowns rolling." );
    aoe->add_action(
        "primordial_wave,target_if=min:dot.flame_shock.remains,if=!buff.primordial_wave.up&buff.surge_of_power.up&!"
        "buff.splintered_elements.up",
        "Spread Flame Shock using Surge of Power. Don't waste buffs by resets (resets are gone, but I'll "
        "keep that logic here)." );
    aoe->add_action(
        "primordial_wave,target_if=min:dot.flame_shock.remains,if=!buff.primordial_wave.up"
        "&talent.deeply_rooted_elements.enabled&!talent.surge_of_power.enabled&!buff.splintered_elements.up",
        "Spread Flame Shock using Surge of Power. Don't waste buffs by resets (resets are gone, but I'll "
        "keep that logic here)." );
    aoe->add_action( "flame_shock,target_if=refreshable,if=buff.surge_of_power.up",
                     "Spread Flame Shock using Surge of Power." );
    aoe->add_action( "flame_shock,target_if=refreshable,if=talent.deeply_rooted_elements.enabled&!talent.surge_of_power.enabled",
                     "Spread Flame Shock to gamble on Deeply Rooted Element procs." );
    aoe->add_action( "ascendance",
                     "JUST DO IT! "
                     "https://i.kym-cdn.com/entries/icons/mobile/000/018/147/"
                     "Shia_LaBeouf__Just_Do_It__Motivational_Speech_(Original_Video_by_LaBeouf__R%C3%B6nkk%C3%B6___"
                     "Turner)_0-4_screenshot.jpg" );
    aoe->add_action( "liquid_magma_totem", "Keep your cooldowns rolling." );
    aoe->add_action(
        "earthquake,if=!talent.echoes_of_great_sundering.enabled|buff.echoes_of_great_sundering.up",
        "Use the talents you selected. Did you invest only 1 point in it? In this case this'll be a DPS decrease." );
    aoe->add_action(
        "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled",
        "Use the talents you selected. Did you invest only 1 point in it? In this case this'll be a DPS decrease. "
        "Spread Lightning Rod to as many targets as possible." );
    aoe->add_action(
        "elemental_blast,if=talent.echoes_of_great_sundering.enabled",
        "Use the talents you selected. Did you invest only 1 point in it? In this case this'll be a DPS decrease." );
    aoe->add_action(
        "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled",
        "Use the talents you selected. Did you invest only 1 point in it? In this case this'll be a DPS decrease. "
        "Spread Lightning Rod to as many targets as possible." );
    aoe->add_action(
        "earth_shock,if=talent.echoes_of_great_sundering.enabled",
        "Use the talents you selected. Did you invest only 1 point in it? In this case this'll be a DPS decrease." );
    aoe->add_action( "lava_beam,if=buff.stormkeeper.up", "Stormkeeper is strong and should be used." );
    aoe->add_action( "chain_lightning,if=buff.stormkeeper.up", "Stormkeeper is strong and should be used." );
    aoe->add_action( "chain_lightning,if=buff.power_of_the_maelstrom.up",
                     "Power of the Maelstrom is strong and should be used." );
    aoe->add_action(
        "lava_beam,if=active_enemies>=6&buff.surge_of_power.up",
        "Against 6 targets or more Surge of Power should be used with Lava Beam rather than Lava Burst." );
    aoe->add_action(
        "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up",
        "Against 6 targets or more Surge of Power should be used with Chain Lightning rather than Lava Burst." );
    aoe->add_action(
        "lava_burst,target_if=dot.flame_shock.remains,if=buff.lava_surge.up&talent.deeply_rooted_elements.enabled",
        "Gamble away for Deeply Rooted Elements procs whenever Lava Surge makes Lava Burst more efficient." );
    aoe->add_action(
        "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&talent.master_of_the_elements.enabled&(maelstrom>=60-5*talent.eye_of_the_storm.rank-2*talent.flow_of_power.enabled)&(!talent.echoes_of_great_sundering.enabled|buff.echoes_of_great_sundering.up)",
        "Cast Lava Burst to buff your immediately follow-up Earthquake with Master of the Elements." );
    aoe->add_action( "icefury,if=talent.electrified_shocks.enabled&active_enemies<5", "Use Icefury if you can get the full benefit from Electrified Shocks. If more targets are present ignore it." );
    aoe->add_action( "frost_shock,if=buff.icefury.up&talent.electrified_shocks.enabled&!debuff.electrified_shocks.up&active_enemies<5", "Spread out your Frost Shock casts to empower as many Chain Lightnings as possible." );
    aoe->add_action( "lava_beam", "" );
    aoe->add_action( "chain_lightning" );
    aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
    aoe->add_action( "frost_shock,moving=1" );

    // Single target APL
    single_target->add_action( "fire_elemental", "Keep your cooldowns rolling." );
    single_target->add_action( "storm_elemental", "Keep your cooldowns rolling." );
    single_target->add_action(
        "primordial_wave,target_if=min:dot.flame_shock.remains,if=!buff.primordial_wave.up&!buff.splintered_elements.up",
        "Use Primordial Wave as much as possible without wasting buffs." );
    single_target->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=refreshable&!buff.surge_of_power.up,cycle_targets=1" );
    single_target->add_action( "liquid_magma_totem", "Keep your cooldowns rolling." );
    single_target->add_action( "stormkeeper,if=!buff.ascendance.up" );
    single_target->add_action( "ascendance,if=!buff.stormkeeper.up" );
    single_target->add_action( "cancel_buff,name=lava_surge,if=buff.stormkeeper.up&buff.surge_of_power.up" );
    single_target->add_action( "lava_burst,if=buff.stormkeeper.up&buff.surge_of_power.up" );
    single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up", "Stormkeeper is strong and should be used." );
    single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&!talent.surge_of_power.enabled", "Stormkeeper is strong and should be used." );
    single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up", "Surge of Power is strong and should be used." );
    single_target->add_action( "icefury,if=talent.electrified_shocks.enabled" );
    single_target->add_action( "frost_shock,if=buff.icefury.up&talent.electrified_shocks.enabled&(!debuff.electrified_shocks.up|buff.icefury.remains<=gcd)" );
    single_target->add_action( "frost_shock,if=buff.icefury.up&talent.electrified_shocks.enabled&maelstrom>=50&debuff.electrified_shocks.remains<2*gcd&buff.stormkeeper.up" );
    single_target->add_action( "lava_burst,if=buff.windspeakers_lava_resurgence.up", "Windspeaker's Lava Resurgence is strong. Don't sit on it." );
    single_target->add_action( "lava_burst,if=cooldown_react&buff.lava_surge.up",
                               "Lava Surge is neat. Utilize it." );
    single_target->add_action(
        "earthquake,if=buff.echoes_of_great_sundering.up&(!talent.elemental_blast.enabled&active_enemies<2|active_enemies>1)",
        "Use the talents you selected. Did you invest only 1 point in it? In this case this'll be a DPS decrease. Additionally Elemental Blast is stronger than EoGS. In this case don't use Earthquake on single target." );
    single_target->add_action(
        "earthquake,if=active_enemies>1&(spell_targets.chain_lightning>1|spell_targets.lava_beam>1)&!talent.windspeakers_lava_resurgence.enabled&!talent.echoes_of_great_sundering.enabled",
        "Use Earthquake against two enemies unless you have to alternate because of Echoes of Great Sundering." );
    single_target->add_action( "elemental_blast" );
    single_target->add_action( "earth_shock" );
    single_target->add_action( "lava_burst,if=buff.flux_melting.up", "Utilize present buffs." );
    single_target->add_action( "frost_shock,if=buff.icefury.up&talent.flux_melting.enabled&!buff.flux_melting.up",
                               "Spread out your Icefury usage if you can get more use out of accompanied buffs." );
    single_target->add_action( "frost_shock,if=buff.icefury.up&(talent.electrified_shocks.enabled&!debuff.electrified_shocks.up|buff.icefury.remains<6)",
                               "Spread out your Icefury usage if you can get more use out of accompanied buffs." );
    single_target->add_action(
        "lightning_bolt,if=buff.power_of_the_maelstrom.up&talent.unrelenting_calamity.enabled",
        "Utilize the Power of the Maelstrom buff if your Lightning Bolt is empowered by Unrelenting Calamity." );
    single_target->add_action( "icefury" );
    single_target->add_action(
        "lightning_bolt,if=pet.storm_elemental.active",
        "Spam Lightning Bolt if Storm Elemental is active. But honor all previous priorities." );
    single_target->add_action( "lava_burst" );
    single_target->add_action( "frost_shock,if=buff.icefury.up&!talent.electrified_shocks.enabled&!talent.flux_melting.enabled",
                               "Use your Icefury buffs if you didn't improve the talent." );
    single_target->add_action( "chain_lightning,if=active_enemies>1&(spell_targets.chain_lightning>1|spell_targets.lava_beam>1)", "Casting Chain Lightning at two targets is mor efficient than Lightning Bolt." );
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

  // action_priority_list_t* cds              = get_action_priority_list( "cds" );

  // Consumables
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );

  // Self-buffs
  precombat->add_action("windfury_weapon");
  precombat->add_action("flametongue_weapon" );
  precombat->add_action("lightning_shield" );
  precombat->add_action("windfury_totem,if=!runeforge.doom_winds.equipped");
  precombat->add_action("fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent");
  precombat->add_action("variable,name=trinket1_is_weird,value=trinket.1.is.the_first_sigil|trinket.1.is.scars_of_fraternal_strife|trinket.1.is.cache_of_acquired_treasures");
  precombat->add_action("variable,name=trinket2_is_weird,value=trinket.2.is.the_first_sigil|trinket.2.is.scars_of_fraternal_strife|trinket.2.is.cache_of_acquired_treasures");

  // Snapshot stats
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // All Shamans Bloodlust by default
  def->add_action( this, "Bloodlust", "line_cd=600");

  // In-combat potion
  def->add_action( "potion,if=(talent.ascendance.enabled&raid_event.adds.in>=90&cooldown.ascendance.remains<10)|(talent.hot_hand.enabled&buff.molten_weapon.up)|buff.icy_edge.up|(talent.stormflurry.enabled&buff.crackling_surge.up)|active_enemies>1|fight_remains<30" );

  // "Default" APL controlling logic flow to specialized sub-APLs
  def->add_action( this, "Wind Shear", "", "Interrupt of casts." );
  // Turn on auto-attack first thing
  def->add_action( "auto_attack" );

  // Heart of Azeroth major essence
  def->add_action( "heart_essence" );

  //_Use_items
    def->add_action("use_item,name=the_first_sigil,if=(talent.ascendance.enabled&raid_event.adds.in>=90&cooldown.ascendance.remains<10)|(talent.hot_hand.enabled&buff.molten_weapon.up)|buff.icy_edge.up|(talent.stormflurry.enabled&buff.crackling_surge.up)|active_enemies>1|fight_remains<30");
    def->add_action("use_item,name=cache_of_acquired_treasures,if=buff.acquired_sword.up|fight_remains<25");
    def->add_action("use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up|fight_remains<31|raid_event.adds.in<16|active_enemies>1");
    def->add_action("use_items,slots=trinket1,if=!variable.trinket1_is_weird");
    def->add_action("use_items,slots=trinket2,if=!variable.trinket2_is_weird");

    //_Racials
    def->add_action("blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50");
    def->add_action("berserking,if=!talent.ascendance.enabled|buff.ascendance.up");
    def->add_action("fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50");
    def->add_action("ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50");

    def->add_action("feral_spirit");
    def->add_action("fae_transfusion,if=runeforge.seeds_of_rampant_growth.equipped|soulbind.grove_invigoration|soulbind.field_of_blossoms|active_enemies=1");
    def->add_action("vesper_totem,if=raid_event.adds.in>40|active_enemies>1");
    def->add_action("ascendance,if=(ti_lightning_bolt&active_enemies=1&raid_event.adds.in>=90)|(ti_chain_lightning&active_enemies>1)");
    def->add_action("doom_winds,if=raid_event.adds.in>=90|active_enemies>1");
    def->add_action("windfury_totem,if=runeforge.doom_winds.equipped&buff.doom_winds_debuff.down&(raid_event.adds.in>=60|active_enemies>1)");

    def->add_action("call_action_list,name=single,if=active_enemies=1", "If_only_one_enemy,_priority_follows_the_'single'_action_list.");
    def->add_action("call_action_list,name=aoe,if=active_enemies>1", "On_multiple_enemies,_the_priority_follows_the_'aoe'_action_list.");

    single->add_action("windstrike");
    single->add_action("lava_lash,if=buff.hot_hand.up|buff.ashen_catalyst.stack=8|(runeforge.primal_lava_actuators.equipped&buff.primal_lava_actuators.stack=8)");
    single->add_action("windfury_totem,if=!buff.windfury_totem.up");
    single->add_action("stormstrike,if=buff.doom_winds.up|buff.doom_winds_talent.up");
    single->add_action("crash_lightning,if=buff.doom_winds.up|buff.doom_winds_talent.up");
    single->add_action("ice_strike,if=buff.doom_winds.up|buff.doom_winds_talent.up");
    single->add_action("sundering,if=buff.doom_winds.up|buff.doom_winds_talent.up");
    single->add_action("primordial_wave,if=buff.primordial_wave.down&(raid_event.adds.in>42|raid_event.adds.in<6)");
    single->add_action("flame_shock,if=!ticking");
    single->add_action("lightning_bolt,if=buff.maelstrom_weapon.stack>=5&buff.primordial_wave.up&raid_event.adds.in>buff.primordial_wave.remains&(!buff.splintered_elements.up|fight_remains<=12)");
    single->add_action("ice_strike,if=talent.hailstorm.enabled");
    single->add_action("frost_shock,if=buff.hailstorm.up");
    single->add_action("lava_lash,if=dot.flame_shock.refreshable");
    single->add_action("stormstrike,if=talent.stormflurry.enabled&buff.stormbringer.up");
    single->add_action("elemental_blast,if=(!talent.elemental_spirits.enabled|(talent.elemental_spirits.enabled&(charges=max_charges|buff.feral_spirit.up)))&buff.maelstrom_weapon.stack>=5");
    single->add_action("chain_harvest,if=buff.maelstrom_weapon.stack>=5&raid_event.adds.in>=90");
    single->add_action("lava_burst,if=buff.maelstrom_weapon.stack>=5");
    single->add_action("lightning_bolt,if=buff.maelstrom_weapon.stack=10&buff.primordial_wave.down");
    single->add_action("stormstrike");
    single->add_action("fleshcraft,interrupt=1,if=soulbind.volatile_solvent");
    single->add_action("windfury_totem,if=buff.windfury_totem.remains<10");
    single->add_action("ice_strike");
    single->add_action("lava_lash");
    single->add_action("bag_of_tricks");
    single->add_action("lightning_bolt,if=buff.maelstrom_weapon.stack>=5&buff.primordial_wave.down");
    single->add_action("sundering,if=raid_event.adds.in>=40");
    single->add_action("fire_nova,if=talent.swirling_maelstrom.enabled&active_dot.flame_shock");
    single->add_action("frost_shock");
    single->add_action("crash_lightning");
    single->add_action("fire_nova,if=active_dot.flame_shock");
    single->add_action("fleshcraft,if=soulbind.pustule_eruption");
    single->add_action("earth_elemental");
    single->add_action("flame_shock");
    single->add_action("windfury_totem,if=buff.windfury_totem.remains<30");

    aoe->add_action("crash_lightning,if=(talent.doom_winds|runeforge.doom_winds.equipped)&(buff.doom_winds.up|buff.doom_winds_talent.up)");
    aoe->add_action("lightning_bolt,if=(active_dot.flame_shock=active_enemies|active_dot.flame_shock=6)&buff.primordial_wave.up&buff.maelstrom_weapon.stack>=(5+5*talent.overflowing_maelstrom.enabled)&(!buff.splintered_elements.up|fight_remains<=12|raid_event.adds.remains<=gcd)");
    aoe->add_action("chain_harvest,if=buff.maelstrom_weapon.stack>=5");
    aoe->add_action("sundering,if=(talent.doomwinds|runeforge.doom_winds.equipped)&(buff.doom_winds.up|buff.doom_winds_talent.up)");
    aoe->add_action("fire_nova,if=active_dot.flame_shock>=6|(active_dot.flame_shock>=4&active_dot.flame_shock=active_enemies)");
    aoe->add_action("primordial_wave,target_if=min:dot.flame_shock.remains,cycle_targets=1,if=!buff.primordial_wave.up");
    aoe->add_action("windstrike,if=talent.thorims_invocation.enabled&ti_chain_lightning&buff.maelstrom_weapon.stack>1");
    aoe->add_action("lava_lash,target_if=min:debuff.lashing_flames.remains,cycle_targets=1,if=dot.flame_shock.ticking&(active_dot.flame_shock<active_enemies&active_dot.flame_shock<6)");
    aoe->add_action("flame_shock,if=!ticking");
    aoe->add_action("flame_shock,target_if=min:dot.flame_shock.remains,cycle_targets=1,if=!talent.hailstorm.enabled&active_dot.flame_shock<active_enemies&active_dot.flame_shock<6");
    aoe->add_action("ice_strike,if=talent.hailstorm.enabled" );
    aoe->add_action("frost_shock,if=talent.hailstorm.enabled&buff.hailstorm.up");
    aoe->add_action("sundering");
    aoe->add_action("fire_nova,if=active_dot.flame_shock>=4");
    aoe->add_action("lava_lash,target_if=min:debuff.lashing_flames.remains,cycle_targets=1,if=talent.lashing_flames.enabled");
    aoe->add_action("fire_nova,if=active_dot.flame_shock>=3");
    aoe->add_action("elemental_blast,if=(!talent.elemental_spirits.enabled|(talent.elemental_spirits.enabled&(charges=max_charges|buff.feral_spirit.up)))&buff.maelstrom_weapon.stack=10&(!talent.crashing_storms.enabled|active_enemies<=3)");
    aoe->add_action("chain_lightning,if=buff.maelstrom_weapon.stack=10");
    aoe->add_action("crash_lightning,if=buff.cl_crash_lightning.up");
    aoe->add_action("lava_lash,if=buff.crash_lightning.up&buff.ashen_catalyst.stack=8|buff.primal_lava_actuators.stack=8");
    aoe->add_action("windstrike,if=buff.crash_lightning.up");
    aoe->add_action("stormstrike,if=buff.crash_lightning.up&buff.gathering_storms.stack=6");
    aoe->add_action("lava_lash,if=buff.crash_lightning.up");
    aoe->add_action("ice_strike,if=buff.crash_lightning.up");
    aoe->add_action("stormstrike,if=buff.crash_lightning.up");
    aoe->add_action("elemental_blast,if=(!talent.elemental_spirits.enabled|(talent.elemental_spirits.enabled&(charges=max_charges|buff.feral_spirit.up)))&buff.maelstrom_weapon.stack>=5&(!talent.crashing_storms.enabled|active_enemies<=3)");
    aoe->add_action("fire_nova,if=active_dot.flame_shock>=2" );
    aoe->add_action("crash_lightning");
    aoe->add_action("windstrike");
    aoe->add_action("lava_lash");
    aoe->add_action("ice_strike");
    aoe->add_action("stormstrike");
    aoe->add_action("flame_shock,target_if=refreshable,cycle_targets=1");
    aoe->add_action("frost_shock");
    aoe->add_action("chain_lightning,if=buff.maelstrom_weapon.stack>=5");
    aoe->add_action("earth_elemental");
    aoe->add_action("windfury_totem,if=buff.windfury_totem.remains<30");

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

  // Covenants
  def->add_action( "chain_harvest,if=covenant.venthyr" );
  def->add_action( "vesper_totem,if=covenant.kyrian" );

  def->add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time&cooldown_react" );
  def->add_action( "fae_transfusion,if=covenant.night_fae" );
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

  if ( legendary.raging_vesper_vortex.ok() )
  {
    raging_vesper_vortex = new raging_vesper_vortex_t( this );
  }

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

// shaman_t::resource_loss ==================================================

double shaman_t::resource_loss( resource_e resource_type, double amount, gain_t* g, action_t* a )
{
  auto actual_loss = player_t::resource_loss( resource_type, amount, g, a );

  return actual_loss;
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
      return attr == ATTR_AGILITY ? constant.matching_gear_multiplier : 0;
    case SHAMAN_RESTORATION:
      return attr == ATTR_INTELLECT ? constant.matching_gear_multiplier : 0;
    case SHAMAN_ELEMENTAL:
      return attr == ATTR_INTELLECT ? constant.matching_gear_multiplier : 0;
    default:
      return 0.0;
  }
}

// shaman_t::composite_spell_haste ==========================================

double shaman_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  return h;
}

// shaman_t::composite_spell_crit_chance ===========================================

double shaman_t::composite_spell_crit_chance() const
{
  double m = player_t::composite_spell_crit_chance();

  m += spec.critical_strikes->effectN( 1 ).percent();

  return m;
}

// shaman_t::temporary_movement_modifier =======================================

double shaman_t::temporary_movement_modifier() const
{
  double ms = player_t::temporary_movement_modifier();

  if ( buff.spirit_walk->up() )
    ms = std::max( buff.spirit_walk->data().effectN( 1 ).percent(), ms );

  if ( buff.ghost_wolf->up() )
  {
    ms *= 1.0 + buff.ghost_wolf->data().effectN( 2 ).percent();
  }

  return ms;
}

// shaman_t::passive_movement_modifier =======================================

double shaman_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  return ms;
}

// shaman_t::composite_melee_crit_chance ===========================================

double shaman_t::composite_melee_crit_chance() const
{
  double m = player_t::composite_melee_crit_chance();

  m += spec.critical_strikes->effectN( 1 ).percent();

  return m;
}

// shaman_t::composite_attack_haste =========================================

double shaman_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  return h;
}

// shaman_t::composite_attack_speed =========================================

double shaman_t::composite_melee_speed() const
{
  double speed = player_t::composite_melee_speed();

  speed *= 1.0 / ( 1.0 + buff.flurry->value() );

  return speed;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( school_e school ) const
{
  double sp = 0;
  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    double added_spell_power = 0;
    if ( main_hand_weapon.type != WEAPON_NONE )
    {
      added_spell_power = main_hand_weapon.dps * WEAPON_POWER_COEFFICIENT;
    }
    else
    {
      added_spell_power = 0.5 * WEAPON_POWER_COEFFICIENT;
    }

    sp = composite_attack_power_multiplier() * ( cache.attack_power() + added_spell_power ) *
         spec.enhancement_shaman->effectN( 4 ).percent();
  }
  else
    sp = player_t::composite_spell_power( school );

  return sp;
}

// shaman_t::composite_spell_power_multiplier ===============================

double shaman_t::composite_spell_power_multiplier() const
{
  if ( specialization() == SHAMAN_ENHANCEMENT )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// shaman_t::composite_attack_power_multiplier ===============================

double shaman_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  return m;
}

// shaman_t::composite_attribute_multiplier =================================

double shaman_t::composite_attribute_multiplier( attribute_e attribute ) const
{
  double m = player_t::composite_attribute_multiplier( attribute );

  switch ( attribute )
  {
    case ATTR_AGILITY:
      break;
    case ATTR_STAMINA:
      // m *= 1.0 + artifact.might_of_the_earthen_ring.data().effectN( 2 ).percent();
      break;
    default:
      break;
  }

  return m;
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
    m *= 1.0 + talent.elemental_weapons->effectN( 1 ).percent();
  }

  return m;
}

// shaman_t::composite_player_target_multiplier ==============================

double shaman_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  if ( get_target_data( target )->debuff.electrified_shocks->check() &&
       dbc::is_school( school, SCHOOL_NATURE ))
  {
    m *= 1.0 + get_target_data( target )->debuff.electrified_shocks->value();
  }

  return m;
}

// shaman_t::composite_player_pet_damage_multiplier =========================

double shaman_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  // Elemental
  m *= 1.0 + spec.elemental_shaman->effectN( 3 ).percent();

  // Enhancement
  m *= 1.0 + spec.enhancement_shaman->effectN( 3 ).percent();

  m *= 1.0 + buff.elemental_equilibrium->value();

  return m;
}

// shaman_t::composite_leech ================================================

double shaman_t::composite_leech() const
{
  double l = player_t::composite_leech();

  l *= 1.0 + talent.ancestral_defense->effectN( 1 ).percent();

  return l;
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
    case CACHE_MASTERY:
      if ( mastery.enhanced_elements->ok() )
      {
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      }
      break;
    default:
      break;
  }
}

// shaman_t::combat_begin ====================================================

void shaman_t::combat_begin()
{
  player_t::combat_begin();

  buff.witch_doctors_wolf_bones->trigger();
  buff.witch_doctors_ancestry->trigger();
}

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  player_t::reset();

  lava_surge_during_lvb = false;
  for ( auto& elem : counters )
    elem->reset();

  ev_vesper_totem_dmg = ev_vesper_totem_heal = nullptr;
  vesper_totem_used_charges = vesper_totem_heal_charges = vesper_totem_dmg_charges = 0;
  lotfw_counter = 0U;
  action.ti_trigger = nullptr;
  action.totemic_recall_totem = nullptr;

  assert( active_flame_shock.empty() );
}

// shaman_t::merge ==========================================================

void shaman_t::merge( player_t& other )
{
  player_t::merge( other );

  const shaman_t& s = static_cast<shaman_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ]->merge( *s.counters[ i ] );

  for ( size_t i = 0, end = cd_waste_exec.size(); i < end; i++ )
  {
    cd_waste_exec[ i ]->second.merge( s.cd_waste_exec[ i ]->second );
    cd_waste_cumulative[ i ]->second.merge( s.cd_waste_cumulative[ i ]->second );
  }
}

// shaman_t::datacollection_begin ===========================================

void shaman_t::datacollection_begin()
{
  if ( active_during_iteration )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_iter[ i ]->second.reset();
    }
  }

  player_t::datacollection_begin();
}

// shaman_t::datacollection_end =============================================

void shaman_t::datacollection_end()
{
  if ( requires_data_collection() )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_cumulative[ i ]->second.add( cd_waste_iter[ i ]->second.sum() );
    }
  }

  player_t::datacollection_end();
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

void pet::earth_elemental_t::create_actions()
{
  primal_elemental_t::create_actions();

  if ( o()->legendary.deeptremor_stone.ok() )
  {
    deeptremor_eq = new earthquake_damage_t( o() );
    // Create a separate stats object bound to the Earth Elemental instead of the shaman
    // for this
    deeptremor_eq->stats = get_stats( "earthquake_rumble", deeptremor_eq );
    deeptremor_eq->stats->school = deeptremor_eq->get_school();
    deeptremor_eq->init();
  }
}

void pet::earth_elemental_t::arise()
{
  primal_elemental_t::arise();

  if ( deeptremor_stone )
  {
    debug_cast<earthquake_damage_t*>( deeptremor_eq )->mote_buffed =
      o()->buff.master_of_the_elements->check();
    deeptremor_stone->trigger();
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class shaman_report_t : public player_report_extension_t
{
public:
  shaman_report_t( shaman_t& player ) : p( player )
  {
  }
  /*
  void mwgen_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
                     << "<tr>\n"
                                   << "<th>Ability</th>\n"
                                   << "<th>Generated</th>\n"
                                   << "<th>Wasted</th>\n"
                     << "</tr>\n";
  }

  void mwuse_table_header( report::sc_html_stream& os )
  {
    const shaman_t& s = static_cast<const shaman_t&>( p );
    size_t n_mwstack = s.buff.maelstrom_weapon -> max_stack();
    os << "<table class=\"sc\" style=\"float: left;\">\n"
                     << "<tr style=\"vertical-align: bottom;\">\n"
                                   << "<th rowspan=\"2\">Ability</th>\n"
                                   << "<th rowspan=\"2\">Event</th>\n";

    for ( size_t i = 0; i <= n_mwstack; ++i )
    {
                  os   << "<th rowspan=\"2\">" << i << "</th>\n";
    }

    os     << "<th colspan=\"2\">Total</th>\n"
                     << "</tr>\n"
                     << "<tr><th>casts</th><th>charges</th></tr>\n";
  }
  */
  void cdwaste_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th colspan=\"3\">Seconds per Execute</th>\n"
       << "<th colspan=\"3\">Seconds per Iteration</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Ability</th>\n"
       << "<th>Average</th>\n"
       << "<th>Minimum</th>\n"
       << "<th>Maximum</th>\n"
       << "<th>Average</th>\n"
       << "<th>Minimum</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";
  }

  /*
  void mwgen_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void mwuse_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }
  */
  void cdwaste_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }
  /*
  void mwgen_table_contents( report::sc_html_stream& os )
  {
    double total_generated = 0, total_wasted = 0;
    int n = 0;

    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
                  stats_t* stats = p.stats_list[ i ];
                  double n_generated = 0, n_wasted = 0;

                  for (auto & elem : stats -> action_list)
                  {
                    shaman_attack_t* a = dynamic_cast<shaman_attack_t*>( elem );
                    if ( ! a )
                                  continue;

                    if ( ! a -> may_proc_maelstrom )
                                  continue;

                    n_generated += a -> maelstrom_procs -> mean();
                    total_generated += a -> maelstrom_procs -> mean();
                    n_wasted += a -> maelstrom_procs_wasted -> mean();
                    total_wasted += a -> maelstrom_procs_wasted -> mean();
                  }

                  if ( n_generated > 0 || n_wasted > 0 )
                  {
                    std::string name_str = report::decorated_action_name( stats -> action_list[ 0 ] );
                    std::string row_class_str = "";
                    if ( ++n & 1 )
                                  row_class_str = " class=\"odd\"";

                    os.format("<tr%s><td class=\"left\">%s</td><td class=\"right\">%.2f</td><td class=\"right\">%.2f
  (%.2f%%)</td></tr>\n", row_class_str.c_str(), name_str.c_str(), util::round( n_generated, 2 ), util::round(
  n_wasted, 2 ), util::round( 100.0 * n_wasted / n_generated, 2 ) );
                  }
    }

    os.format("<tr><td class=\"left\">Total</td><td class=\"right\">%.2f</td><td class=\"right\">%.2f
  (%.2f%%)</td></tr>\n", total_generated, total_wasted, 100.0 * total_wasted / total_generated );
  }

  void mwuse_table_contents( report::sc_html_stream& os )
  {
    const shaman_t& s = static_cast<const shaman_t&>( p );
    size_t n_mwstack = s.buff.maelstrom_weapon -> max_stack();
    std::vector<double> total_mw_cast( n_mwstack + 2 );
    std::vector<double> total_mw_executed( n_mwstack + 2 );
    int n = 0;
    std::string row_class_str = "";

    for ( size_t i = 0, end = p.action_list.size(); i < end; i++ )
    {
                  if ( shaman_spell_t* s = dynamic_cast<shaman_spell_t*>( p.action_list[ i ] ) )
                  {
                    for ( size_t j = 0, end2 = s -> maelstrom_weapon_cast.size() - 1; j < end2; j++ )
                    {
                                  total_mw_cast[ j ] += s -> maelstrom_weapon_cast[ j ] -> mean();
                                  total_mw_cast[ n_mwstack + 1 ] += s -> maelstrom_weapon_cast[ j ] -> mean();

                                  total_mw_executed[ j ] += s -> maelstrom_weapon_executed[ j ] -> mean();
                                  total_mw_executed[ n_mwstack + 1 ] += s -> maelstrom_weapon_executed[ j ] -> mean();
                    }
                  }
    }

    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
                  stats_t* stats = p.stats_list[ i ];
                  std::vector<double> n_cast( n_mwstack + 2 );
                  std::vector<double> n_executed( n_mwstack + 2 );
                  double n_cast_charges = 0, n_executed_charges = 0;
                  bool has_data = false;

                  for (auto & elem : stats -> action_list)
                  {
                    if ( shaman_spell_t* s = dynamic_cast<shaman_spell_t*>( elem ) )
                    {
                                  for ( size_t k = 0, end3 = s -> maelstrom_weapon_cast.size() - 1; k < end3; k++ )
                                  {
                                    if ( s -> maelstrom_weapon_cast[ k ] -> mean() > 0 || s ->
  maelstrom_weapon_executed[ k ] -> mean() > 0 ) has_data = true;

                                    n_cast[ k ] += s -> maelstrom_weapon_cast[ k ] -> mean();
                                    n_cast[ n_mwstack + 1 ] += s -> maelstrom_weapon_cast[ k ] -> mean();

                                    n_cast_charges += s -> maelstrom_weapon_cast[ k ] -> mean() * k;

                                    n_executed[ k ] += s -> maelstrom_weapon_executed[ k ] -> mean();
                                    n_executed[ n_mwstack + 1 ] += s -> maelstrom_weapon_executed[ k ] -> mean();

                                    n_executed_charges += s -> maelstrom_weapon_executed[ k ] -> mean() * k;
                                  }
                    }
                  }

                  if ( has_data )
                  {
                    row_class_str = "";
                    if ( ++n & 1 )
                                  row_class_str = " class=\"odd\"";

                    std::string name_str = report::decorated_action_name( stats -> action_list[ 0 ] );

                    os.format("<tr%s><td rowspan=\"2\" class=\"left\" style=\"vertical-align: top;\">%s</td>",
                                    row_class_str.c_str(), name_str.c_str() );

                    os << "<td class=\"left\">Cast</td>";

                    for ( size_t j = 0, end2 = n_cast.size(); j < end2; j++ )
                    {
                                  double pct = 0;
                                  if ( total_mw_cast[ j ] > 0 )
                                    pct = 100.0 * n_cast[ j ] / n_cast[ n_mwstack + 1 ];

                                  if ( j < end2 - 1 )
                                    os.format("<td class=\"right\">%.1f (%.1f%%)</td>", util::round( n_cast[ j ], 1 ),
  util::round( pct, 1 ) ); else
                                  {
                                    os.format("<td class=\"right\">%.1f</td>", util::round( n_cast[ j ], 1 ) );
                                    os.format("<td class=\"right\">%.1f</td>", util::round( n_cast_charges, 1 ) );
                                  }
                    }

                    os << "</tr>\n";

                    os.format("<tr%s>", row_class_str.c_str() );

                    os << "<td class=\"left\">Execute</td>";

                    for ( size_t j = 0, end2 = n_executed.size(); j < end2; j++ )
                    {
                                  double pct = 0;
                                  if ( total_mw_executed[ j ] > 0 )
                                    pct = 100.0 * n_executed[ j ] / n_executed[ n_mwstack + 1 ];

                                  if ( j < end2 - 1 )
                                    os.format("<td class=\"right\">%.1f (%.1f%%)</td>", util::round( n_executed[ j ],
  1
  ), util::round( pct, 1 )
  ); else
                                  {
                                    os.format("<td class=\"right\">%.1f</td>", util::round( n_executed[ j ], 1 ) );
                                    os.format("<td class=\"right\">%.1f</td>", util::round( n_executed_charges, 1 ) );
                                  }
                    }

                    os << "</tr>\n";
                  }
    }
  }
  */
  void cdwaste_table_contents( report::sc_html_stream& os )
  {
    size_t n = 0;
    for ( size_t i = 0; i < p.cd_waste_exec.size(); i++ )
    {
      const data_t* entry = p.cd_waste_exec[ i ];
      if ( entry->second.count() == 0 )
      {
        continue;
      }

      const data_t* iter_entry = p.cd_waste_cumulative[ i ];

      action_t* a          = p.find_action( entry->first );
      std::string name_str = entry->first;
      if ( a )
      {
        name_str = report_decorators::decorated_action( *a );
      }
      else
      {
        name_str = util::encode_html( name_str );
      }

      std::string row_class_str;
      if ( ++n & 1 )
        row_class_str = " class=\"odd\"";

      os.printf( "<tr%s>", row_class_str.c_str() );
      os << "<td class=\"left\">" << name_str << "</td>";
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.min() );
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.max() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.min() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.max() );
      os << "</tr>\n";
    }
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
    /*
                    if ( p.specialization() == SHAMAN_ENHANCEMENT )
                    {
                      os << "\t\t\t\t\t<h3 class=\"toggle open\">Maelstrom Weapon details</h3>\n"
                                     << "\t\t\t\t\t<div class=\"toggle-content\">\n";

                      mwgen_table_header( os );
                      mwgen_table_contents( os );
                      mwgen_table_footer( os );

                      mwuse_table_header( os );
                      mwuse_table_contents( os );
                      mwuse_table_footer( os );

                      os << "\t\t\t\t\t\t</div>\n";

                      os << "<div class=\"clear\"></div>\n";
                    }
    */
    if ( !p.cd_waste_exec.empty() )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown waste details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      cdwaste_table_header( os );
      cdwaste_table_contents( os );
      cdwaste_table_footer( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }

    os << "\t\t\t\t\t</div>\n";
  }

private:
  shaman_t& p;
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

shaman_t::pets_t::pets_t( shaman_t* s )
  : pet_fire_elemental( nullptr ),
    pet_storm_elemental( nullptr ),
    pet_earth_elemental( nullptr ),
    guardian_fire_elemental( nullptr ),
    guardian_storm_elemental( nullptr ),
    guardian_earth_elemental( nullptr ),

    spirit_wolves( "spirit_wolf", s, []( shaman_t* s ) { return new pet::spirit_wolf_t( s ); } ),
    fire_wolves( "fiery_wolf", s, []( shaman_t* s ) { return new pet::fire_wolf_t( s ); } ),
    frost_wolves( "frost_wolf", s, []( shaman_t* s ) { return new pet::frost_wolf_t( s ); } ),
    lightning_wolves( "lightning_wolf", s, []( shaman_t* s ) { return new pet::lightning_wolf_t( s ); } ),

    /// Bron's Call to Arms trigger logic is completely overridden by the Shaman module
    bron( nullptr )
{
  spirit_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  fire_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  frost_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  lightning_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
}

}  // namespace

const module_t* module_t::shaman()
{
  static shaman_module_t m;
  return &m;
}

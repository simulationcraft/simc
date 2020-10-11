// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player/covenant.hpp"
#include "player/pet_spawner.hpp"
#include "sc_enums.hpp"

#include "simulationcraft.hpp"

// ==========================================================================
// Shaman
// ==========================================================================

// Shadowlands TODO
//
// Shared
// - Covenants
//   - Kyrian
//     - healing? (do we actually care?)
//     - totem replacement? (see above)
//   - Necrolord
//     - apply background flame shock to target
//     - cast background lava burst to all flame shocked targets
// - Class Legendaries
//   - Ancestral Reminder
//     - extending lust duration is easy, but increasing effect of lust seems trickier
//   - Deeptremor Stone
//   - Deeply Rooted Elements
//     - currently also triggers Ascendance cooldown but likely to be fixed
//     - will require background lava burst support similar to primordial wave
// - Covenant Conduits
//   - Elysian Dirge (Kyrian)
//
// Elemental
// - Implement Static Discharge
// - Implement Echoing Shock
// - Spec Legendaries
//   - Elemental Equilibrium
//     - There are a number of spell-specific bugs here about which buffs get applied
//   - Windspeaker's Lava Resurgence
//     - Implement the bugged behavior, not the tooltip behavior
// - Spec Conduits
//   - Earth and Sky
//     - Still waiting on them to reimplement this on beta for post-Fulmination
//   - Shake the Foundations
//     - Background CL cast will be similar to Echoing Shock implementation
// - PreRaid profile gear
// - Castle Nathria profile gear
// - APL
//   - We intentionally have a very simplified version of the APL in place now to indicate
//     that we have not done *any* work on developing one for Shadowlands yet.
//   - From discussions:
//     - single_target, cleave, and aoe APLs
//
// Enhancement
// whole huge pile of stuff to do
//
// Resto DPS?

namespace
{  // UNNAMED NAMESPACE

struct echoing_shock_event_t : public event_t
{
  action_t* action;
  player_t* target;

  echoing_shock_event_t( action_t* action_, player_t* target_, timespan_t delay_ )
    : event_t( *action_->player, delay_ ), action( action_ ), target( target_ )
  { }

  const char* name() const override
  { return "echoing_shock_event_t"; }

  // Defined below for ease
  void execute() override;
};

/**
  Check_distance_targeting is only called when distance_targeting_enabled is true. Otherwise,
  available_targets is called.  The following code is intended to generate a target list that
  properly accounts for range from each target during chain lightning.  On a very basic level, it
  starts at the original target, and then finds a path that will hit 4 more, if possible.  The
  code below randomly cycles through targets until it finds said path, or hits the maximum amount
  of attempts, in which it gives up and just returns the current best path.  I wouldn't be
  terribly surprised if Blizz did something like this in game.
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
  size_t local_attempts = 0, attempts = 0, chain_number = 1;
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
    while ( targets_left_to_try.size() > 0 && local_attempts < num_targets * 2 )
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
typedef std::pair<std::string, simple_sample_data_with_min_max_t> data_t;
typedef std::pair<std::string, simple_sample_data_t> simple_data_t;

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

enum class execute_type
{
  NORMAL,
  ECHOING_SHOCK
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

struct shaman_attack_t;
struct shaman_spell_t;
struct shaman_heal_t;
struct shaman_totem_pet_t;
struct totem_pulse_event_t;
struct totem_pulse_action_t;

struct shaman_td_t : public actor_target_data_t
{
  struct dots
  {
    dot_t* flame_shock;
    dot_t* molten_weapon;
  } dot;

  struct debuffs
  {
    // Elemental

    // Enhancement
    buff_t* earthen_spike;
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
  ground_aoe_event_t* vesper_totem;
  /// Shaman ability cooldowns
  std::vector<cooldown_t*> ability_cooldowns;
  player_t* recent_target =
      nullptr;  // required for Earthen Rage, whichs' ticks damage the most recently attacked target

  // Options
  bool raptor_glyph;

  // Data collection for cooldown waste
  auto_dispose<std::vector<data_t*> > cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*> > cd_waste_iter;

  // Cached actions
  struct actions_t
  {
    spell_t* lightning_shield;
    spell_t* earthen_rage;
    spell_t* static_discharge_tick;
    spell_t* crashing_storm;
    attack_t* crash_lightning_aoe;
    spell_t* molten_weapon;
    action_t* molten_weapon_dot;

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

    // Covenant Class Ability Buffs
    buff_t* primordial_wave;
    buff_t* vesper_totem;

    // Legendaries
    buff_t* chains_of_devastation_chain_heal;
    buff_t* chains_of_devastation_chain_lightning;
    buff_t* echoes_of_great_sundering;

    // Elemental, Restoration
    buff_t* lava_surge;

    // Elemental, Enhancement
    stat_buff_t* elemental_blast_crit;
    stat_buff_t* elemental_blast_haste;
    stat_buff_t* elemental_blast_mastery;
    buff_t* stormkeeper;

    // Elemental
    buff_t* earthen_rage;
    buff_t* echoing_shock;
    buff_t* master_of_the_elements;
    buff_t* static_discharge;
    buff_t* surge_of_power;
    buff_t* icefury;
    buff_t* unlimited_power;
    buff_t* wind_gust;  // Storm Elemental passive 263806

    // Enhancement
    buff_t* crash_lightning;
    buff_t* hot_hand;
    buff_t* lightning_shield;
    buff_t* stormbringer;

    buff_t* forceful_winds;
    buff_t* icy_edge;
    buff_t* molten_weapon;
    buff_t* crackling_surge;
    buff_t* gathering_storms;

    // Restoration
    buff_t* spirit_walk;
    buff_t* spiritwalkers_grace;
    buff_t* tidal_waves;

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
    cooldown_t* fire_elemental;
    cooldown_t* feral_spirits;
    cooldown_t* lava_burst;
    cooldown_t* storm_elemental;
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

    // Elemental
    conduit_data_t call_of_flame;
    conduit_data_t high_voltage;
    conduit_data_t pyroclastic_shock;
  } conduit;

  // Legendaries
  struct legendary_t
  {
    // Shared
    item_runeforge_t ancestral_reminder;
    item_runeforge_t chains_of_devastation;
    item_runeforge_t deeptremor_stone;
    item_runeforge_t deeply_rooted_elements;  // NYI

    // Elemental
    item_runeforge_t skybreakers_fiery_demise;
    item_runeforge_t elemental_equilibrium;  // NYI
    item_runeforge_t echoes_of_great_sundering;
    item_runeforge_t windspeakers_lava_resurgence;  // NYI

    // Enhancement
    item_runeforge_t doom_winds;                 // NYI
    item_runeforge_t legacy_of_the_frost_witch;  // NYI
    item_runeforge_t primal_lava_actuators;      // NYI
    item_runeforge_t witch_doctors_wolf_bones;   // NYI
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
    gain_t* forceful_winds;
  } gain;

  // Tracked Procs
  struct
  {
    // Elemental, Restoration
    proc_t* lava_surge;
    proc_t* wasted_lava_surge;
    proc_t* surge_during_lvb;

    // Enhancement
    proc_t* windfury;
    proc_t* hot_hand;
  } proc;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* mail_specialization;
    const spell_data_t* shaman;

    // Elemental
    const spell_data_t* chain_lightning_2;  // 7.1 Chain Lightning additional 2 targets passive
    const spell_data_t* elemental_fury;     // general crit multiplier
    const spell_data_t* elemental_shaman;   // general spec multiplier
    const spell_data_t* lava_burst_2;       // 7.1 Lava Burst autocrit with FS passive
    const spell_data_t* lava_surge;

    // Enhancement
    const spell_data_t* crash_lightning;
    const spell_data_t* critical_strikes;
    const spell_data_t* dual_wield;
    const spell_data_t* enhancement_shaman;
    const spell_data_t* feral_spirit_2;  // 7.1 Feral Spirit Maelstrom gain passive
    const spell_data_t* maelstrom_weapon;
    const spell_data_t* stormbringer;
    const spell_data_t* flametongue;

    const spell_data_t* windfury;

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

  // Talents
  struct
  {
    // Generic / Shared
    const spell_data_t* elemental_blast;
    const spell_data_t* spirit_wolf;
    const spell_data_t* earth_shield;
    const spell_data_t* static_charge;
    const spell_data_t* natures_guardian;
    const spell_data_t* wind_rush_totem;
    const spell_data_t* stormkeeper;
    const spell_data_t* ascendance;

    // Elemental
    const spell_data_t* earthen_rage;
    const spell_data_t* echo_of_the_elements;
    const spell_data_t* static_discharge;

    const spell_data_t* aftershock;
    const spell_data_t* echoing_shock;
    // elemental blast - shared

    // spirit wolf - shared
    // earth shield - shared
    // static charge - shared

    const spell_data_t* master_of_the_elements;
    const spell_data_t* storm_elemental;
    const spell_data_t* liquid_magma_totem;

    // natures guardian - shared
    const spell_data_t* ancestral_guidance;
    // wind rush totem - shared

    const spell_data_t* surge_of_power;
    const spell_data_t* primal_elementalist;
    const spell_data_t* icefury;

    const spell_data_t* unlimited_power;
    // stormkeeper - shared
    // ascendance - shared

    // Enhancement

    // lashing flames
    const spell_data_t* forceful_winds;
    // elemental blast - shared

    // stormfury
    const spell_data_t* hot_hand;
    const spell_data_t* ice_strike;

    // spirit wolf - shared
    // earth shield - shared
    // static charge - shared

    // cycle of the elements
    const spell_data_t* hailstorm;
    // fire nova

    // natures guardian - shared
    const spell_data_t* feral_lunge;
    // wind rush totem - shared

    const spell_data_t* crashing_storm;
    // stormkeeper - shared
    const spell_data_t* sundering;

    const spell_data_t* elemental_spirits;
    const spell_data_t* earthen_spike;
    // ascendance - shared

    // Restoration
    const spell_data_t* graceful_spirit;
  } talent;

  // Misc Spells
  struct
  {
    const spell_data_t* resurgence;
    const spell_data_t* maelstrom_melee_gain;
    const spell_data_t* feral_spirit;
    const spell_data_t* fire_elemental;
    const spell_data_t* storm_elemental;
  } spell;

  // Cached pointer for ascendance / normal white melee
  shaman_attack_t* melee_mh;
  shaman_attack_t* melee_oh;
  shaman_attack_t* ascendance_mh;
  shaman_attack_t* ascendance_oh;

  // Weapon Enchants
  shaman_attack_t *windfury_mh, *windfury_oh;
  shaman_spell_t* flametongue;
  shaman_attack_t* hailstorm;

  // Elemental Spirits attacks
  shaman_attack_t* molten_weapon;
  shaman_attack_t* icy_edge;

  shaman_t( sim_t* sim, util::string_view name, race_e r = RACE_TAUREN )
    : player_t( sim, SHAMAN, name, r ),
      lava_surge_during_lvb( false ),
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
      talent(),
      spell()
  {
    /*
    range::fill( pet.spirit_wolves, nullptr );
    range::fill( pet.elemental_wolves, nullptr );
    */

    // Cooldowns
    cooldown.ascendance      = get_cooldown( "ascendance" );
    cooldown.fire_elemental  = get_cooldown( "fire_elemental" );
    cooldown.storm_elemental = get_cooldown( "storm_elemental" );
    cooldown.feral_spirits   = get_cooldown( "feral_spirit" );
    cooldown.lava_burst      = get_cooldown( "lava_burst" );
    cooldown.strike          = get_cooldown( "strike" );

    melee_mh      = nullptr;
    melee_oh      = nullptr;
    ascendance_mh = nullptr;
    ascendance_oh = nullptr;

    // Weapon Enchants
    windfury_mh = nullptr;
    windfury_oh = nullptr;
    flametongue = nullptr;
    hailstorm   = nullptr;

    // Elemental Spirits attacks
    molten_weapon = nullptr;
    icy_edge      = nullptr;

    if ( specialization() == SHAMAN_ELEMENTAL || specialization() == SHAMAN_ENHANCEMENT )
      resource_regeneration = regen_type::DISABLED;
    else
      resource_regeneration = regen_type::DYNAMIC;
  }

  ~shaman_t() override;

  // Misc
  bool active_elemental_pet() const;
  void summon_feral_spirits( timespan_t duration );
  void summon_fire_elemental( timespan_t duration );
  void summon_storm_elemental( timespan_t duration );

  // triggers
  void trigger_maelstrom_gain( double base, gain_t* gain = nullptr );
  void trigger_windfury_weapon( const action_state_t* );
  void trigger_flametongue_weapon( const action_state_t* );
  void trigger_icy_edge( const action_state_t* );
  void trigger_stormbringer( const action_state_t* state, double proc_chance = -1.0, proc_t* proc_obj = nullptr );
  void trigger_lightning_shield( const action_state_t* state );
  void trigger_hot_hand( const action_state_t* state );
  void trigger_vesper_totem( const action_state_t* state );

  // Legendary
  // empty - for now

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void create_actions() override;
  void create_options() override;
  void init_gains() override;
  void init_procs() override;
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

  void init_rng() override;
  void init_special_effects() override;

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
  double composite_player_pet_damage_multiplier( const action_state_t* state ) const override;
  double composite_maelstrom_gain_coefficient( const action_state_t* state = nullptr ) const;
  double matching_gear_multiplier( attribute_e attr ) const override;
  action_t* create_action( util::string_view name, const std::string& options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = "" ) override;
  void create_pets() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void arise() override;
  void combat_begin() override;
  void reset() override;
  void merge( player_t& other ) override;
  void copy_from( player_t* ) override;

  void datacollection_begin() override;
  void datacollection_end() override;

  target_specific_t<shaman_td_t> target_data;

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
  T_CONTAINER* get_data_entry( const std::string& name, std::vector<T_DATA*>& entries )
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

struct forceful_winds_buff_t : public buff_t
{
  forceful_winds_buff_t( shaman_t* p ) : buff_t( p, "forceful_winds", p->find_spell( 262652 ) )
  {
  }
};

struct icy_edge_buff_t : public buff_t
{
  icy_edge_buff_t( shaman_t* p ) : buff_t( p, "icy_edge", p->find_spell( 224126 ) )
  {
    set_duration( s_data->duration() );
    set_max_stack( 10 );
    set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  }
};

struct molten_weapon_buff_t : public buff_t
{
  molten_weapon_buff_t( shaman_t* p ) : buff_t( p, "molten_weapon", p->find_spell( 224125 ) )
  {
    set_duration( s_data->duration() );
    set_default_value( 1.0 + s_data->effectN( 1 ).percent() );
    set_max_stack( 10 );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  }
};

struct crackling_surge_buff_t : public buff_t
{
  crackling_surge_buff_t( shaman_t* p ) : buff_t( p, "crackling_surge", p->find_spell( 224127 ) )
  {
    set_duration( s_data->duration() );
    set_default_value( s_data->effectN( 1 ).percent() );
    set_max_stack( 10 );
    set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  }
};

struct gathering_storms_buff_t : public buff_t
{
  gathering_storms_buff_t( shaman_t* p ) : buff_t( p, "gathering_storms", p->find_spell( 198300 ) )
  {
    set_duration( s_data->duration() );
    // Buff applied to player is id# 198300, but appears to pull the value data from the crash lightning ability id
    // instead.  Probably setting an override value on gathering storms from crash lightning data.
    // set_default_value( s_data->effectN( 1 ).percent() ); --replace with this if ever changed
    set_default_value( p->find_spell( 187874 )->effectN( 2 ).percent() );
    // set_max_stack( 1 );
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
    set_trigger_spell( p->talent.ascendance );
    set_tick_callback( [ p ]( buff_t* b, int, timespan_t ) {
      double g = b->data().effectN( 4 ).base_value();
      p->trigger_maelstrom_gain( g, p->gain.ascendance );
    } );
    set_cooldown( timespan_t::zero() );  // Cooldown is handled by the action
  }

  void ascendance( attack_t* mh, attack_t* oh );
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

shaman_td_t::shaman_td_t( player_t* target, shaman_t* p ) : actor_target_data_t( target, p )
{
  // Shared
  dot.flame_shock = target->get_dot( "flame_shock", p );

  // Elemental

  // Enhancement
  dot.molten_weapon    = target->get_dot( "molten_weapon", p );
  debuff.earthen_spike = make_buff( *this, "earthen_spike", p->talent.earthen_spike )
                             ->set_cooldown( timespan_t::zero() )  // Handled by the action
                             // -10% resistance in spell data, treat it as a multiplier instead
                             ->set_default_value( 1.0 + p->talent.earthen_spike->effectN( 2 ).percent() );
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
  bool enable_enh_mastery_scaling;

  bool affected_by_molten_weapon;

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
      enable_enh_mastery_scaling( false ),
      affected_by_molten_weapon( false )
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

    if ( ab::data().affected_by( player->spec.elemental_shaman->effectN( 1 ) ) )
    {
      ab::base_dd_multiplier *= 1.0 + player->spec.elemental_shaman->effectN( 1 ).percent();
    }
    if ( ab::data().affected_by( player->spec.elemental_shaman->effectN( 2 ) ) )
    {
      ab::base_td_multiplier *= 1.0 + player->spec.elemental_shaman->effectN( 2 ).percent();
    }

    if ( ab::data().affected_by( player->spec.enhancement_shaman->effectN( 1 ) ) )
    {
      ab::base_multiplier *= 1.0 + player->spec.enhancement_shaman->effectN( 1 ).percent();
    }

    if ( ab::data().affected_by( player->spec.restoration_shaman->effectN( 3 ) ) )
    {
      ab::base_dd_multiplier *= 1.0 + player->spec.restoration_shaman->effectN( 3 ).percent();
    }
    if ( ab::data().affected_by( player->spec.restoration_shaman->effectN( 4 ) ) )
    {
      ab::base_td_multiplier *= 1.0 + player->spec.restoration_shaman->effectN( 4 ).percent();
    }
    if ( ab::data().affected_by( player->spec.restoration_shaman->effectN( 7 ) ) )
    {
      ab::base_multiplier *= 1.0 + player->spec.restoration_shaman->effectN( 7 ).percent();
    }

    affected_by_molten_weapon =
        ab::data().affected_by_label( player->find_spell( 224125 )->effectN( 1 ).misc_value2() );
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
  }

  void init_finished() override
  {
    ab::init_finished();

    if ( this->cooldown->duration > timespan_t::zero() )
    {
      p()->ability_cooldowns.push_back( this->cooldown );
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

  double action_multiplier() const override
  {
    double m = ab::action_multiplier();

    if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( ( dbc::is_school( this->school, SCHOOL_FIRE ) || dbc::is_school( this->school, SCHOOL_FROST ) ||
             dbc::is_school( this->school, SCHOOL_NATURE ) ) &&
           p()->mastery.enhanced_elements->ok() )
      {
        if ( ab::data().affected_by( p()->mastery.enhanced_elements->effectN( 1 ) ) ||
             ab::data().affected_by( p()->mastery.enhanced_elements->effectN( 5 ) ) || enable_enh_mastery_scaling )
        {
          //...hopefully blizzard never makes direct and periodic scaling different from eachother in our mastery..
          m *= 1.0 + p()->cache.mastery_value();
        }
      }
    }

    if ( affected_by_molten_weapon && p()->buff.molten_weapon->check() )
    {
      m *= std::pow( p()->buff.molten_weapon->check_value(), p()->buff.molten_weapon->check() );
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

  void execute() override
  {
    ab::execute();

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      trigger_maelstrom_gain( ab::execute_state );
    }

    // TODO: wire up enh MW gains
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );
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
        if ( cd_.size() == 0 )
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
  bool may_proc_icy_edge;
  bool may_proc_ability_procs;  // For things that explicitly state they proc from "abilities"

  proc_t *proc_wf, *proc_ft, *proc_fb, *proc_mw, *proc_sb, *proc_ls, *proc_hh, *proc_pp;

  shaman_attack_t( const std::string& token, shaman_t* p, const spell_data_t* s )
    : base_t( token, p, s ),
      may_proc_windfury( p->spec.windfury->ok() ),
      may_proc_flametongue( p->spec.flametongue->ok() ),
      may_proc_maelstrom_weapon( false ),  // Change to whitelisting
      may_proc_stormbringer( p->spec.stormbringer->ok() ),
      may_proc_lightning_shield( false ),
      may_proc_hot_hand( p->talent.hot_hand->ok() ),
      may_proc_icy_edge( false ),
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
      may_proc_stormbringer = ab::weapon;
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
      may_proc_hot_hand = ab::weapon != nullptr;
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

    if ( may_proc_lightning_shield )  // Needs to refactor to defensive version
    {
      proc_ls = player->get_proc( std::string( "Lightning Shield Overcharge: " ) + full_name() );
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

  // need to roll MW gain proc and add stack
  // virtual double maelstrom_weapon_energize_amount( const action_state_t* /* source */ ) const
  //{
  //  return p()->spell.maelstrom_melee_gain->effectN( 1 ).resource( RESOURCE_MAELSTROM );
  //}

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    // Bail out early if the result is a miss/dodge/parry/ms
    if ( !result_is_hit( state->result ) )
      return;

    trigger_maelstrom_weapon( state );
    p()->trigger_windfury_weapon( state );
    p()->trigger_flametongue_weapon( state );
    p()->trigger_lightning_shield( state );
    p()->trigger_hot_hand( state );
    p()->trigger_icy_edge( state );
  }

  void trigger_maelstrom_weapon( const action_state_t* source_state, double amount = 0 )
  {
    if ( !may_proc_maelstrom_weapon )
    {
      return;
    }

    /*if ( p()->buff.ghost_wolf->check() )
    {
      return;
    }*/

    // needs to roll stacks of MW weapon
    // proc_mw->occur();

    return;
  }

  virtual double stormbringer_proc_chance() const
  {
    double base_chance = p()->spec.stormbringer->proc_chance() +
                         p()->cache.mastery() * p()->mastery.enhanced_elements->effectN( 3 ).mastery_value();

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

  shaman_spell_base_t( util::string_view n, shaman_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s )
  {
  }

  void execute() override
  {
    ab::execute();

    // for benefit tracking purpose
    ab::p()->buff.spiritwalkers_grace->up();

    if ( ab::p()->talent.aftershock->ok() && ab::current_resource() == RESOURCE_MAELSTROM &&
         ab::last_resource_cost > 0 && ab::rng().roll( ab::p()->talent.aftershock->effectN( 1 ).percent() ) )
    {
      ab::p()->trigger_maelstrom_gain( ab::last_resource_cost, ab::p()->gain.aftershock );
    }
  }
};

// ==========================================================================
// Shaman Offensive Spell
// ==========================================================================

struct shaman_spell_t : public shaman_spell_base_t<spell_t>
{
  action_t* overload;

public:
  bool may_proc_stormbringer = false;
  proc_t* proc_sb;
  bool affected_by_master_of_the_elements = false;
  bool affected_by_stormkeeper            = false;

  // Echoing Shock stuff
  bool may_proc_echoing_shock;
  stats_t* echoing_shock_stats;

  // General things
  execute_type exec_type;

  shaman_spell_t( util::string_view token, shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() )
    : base_t( token, p, s ), overload( nullptr ), proc_sb( nullptr ),
      may_proc_echoing_shock( false ),
      echoing_shock_stats( nullptr ),
      exec_type( execute_type::NORMAL )
  {
    parse_options( options );

    if ( data().affected_by( p->spec.elemental_fury->effectN( 1 ) ) )
    {
      crit_bonus_multiplier *= 1.0 + p->spec.elemental_fury->effectN( 1 ).percent();
    }

    if ( data().affected_by( p->find_spell( 260734 )->effectN( 1 ) ) )
    {
      affected_by_master_of_the_elements = true;
    }

    if ( data().affected_by( p->find_spell( 191634 )->effectN( 1 ) ) )
    {
      affected_by_stormkeeper = true;
    }

    may_proc_stormbringer = false;
  }

  void init() override
  {
    base_t::init();

    may_proc_echoing_shock = !background && p()->talent.echoing_shock->ok() &&
      id != p()->talent.echoing_shock->id() && ( base_dd_min || spell_power_mod.direct );

    if ( may_proc_echoing_shock )
    {
      if ( auto echoing_shock = p()->find_action( "echoing_shock" ) )
      {
        echoing_shock_stats = p()->get_stats( this->name_str + "_echo", this );
        echoing_shock->stats->add_child( echoing_shock_stats );
      }
    }
  }

  void init_finished() override
  {
    if ( may_proc_stormbringer )
    {
      proc_sb = player->get_proc( std::string( "Stormbringer: " ) + full_name() );
    }

    base_t::init_finished();
  }

  double action_multiplier() const override
  {
    double m = shaman_action_t::action_multiplier();
    // BfA Elemental talent - Master of the Elements
    if ( affected_by_master_of_the_elements )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->value();
    }
    return m;
  }

  double composite_spell_power() const override
  {
    double sp = base_t::composite_spell_power();

    return sp;
  }

  void execute() override
  {
    base_t::execute();

    if ( p()->talent.earthen_rage->ok() && !background /*&& execute_state->action->harmful*/ )
    {
      p()->recent_target = execute_state->target;
      p()->buff.earthen_rage->trigger();
    }

    // BfA Elemental talent - Master of the Elements
    if ( affected_by_master_of_the_elements && !background )
    {
      p()->buff.master_of_the_elements->decrement();
    }

    p()->trigger_vesper_totem( execute_state );
    trigger_echoing_shock( execute_state->target );
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
    return p()->cache.mastery_value();
  }

  // Additional guaranteed overloads
  virtual size_t n_overloads( const action_state_t* ) const
  {
    return 0;
  }

  // Additional overload chances
  virtual size_t n_overload_chances( const action_state_t* ) const
  {
    return 0;
  }

  bool is_echoed_spell() const
  { return exec_type == execute_type::ECHOING_SHOCK; }

  void trigger_elemental_overload( const action_state_t* source_state ) const
  {
    struct elemental_overload_event_t : public event_t
    {
      action_state_t* state;

      elemental_overload_event_t( action_state_t* s )
        : event_t( *s->action->player, timespan_t::from_millis( 400 ) ), state( s )
      {
      }

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

    if ( !p()->mastery.elemental_overload->ok() )
    {
      return;
    }

    if ( !overload )
    {
      return;
    }

    /* Hacky to recreate ingame behavior. Stormkeeper forces only the first overload to happen. */
    unsigned overloads = rng().roll( overload_chance( source_state ) );

    if ( p()->buff.stormkeeper->up() && affected_by_stormkeeper )
    {
      overloads = 1;
    }

    overloads += (unsigned)n_overloads( source_state );

    for ( size_t i = 0, end = overloads; i < end; ++i )
    {
      action_state_t* s = overload->get_state();
      overload->snapshot_state( s, result_amount_type::DMG_DIRECT );
      s->target = source_state->target;

      make_event<elemental_overload_event_t>( *sim, s );
    }
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    // p()->trigger_stormbringer( state );
  }

  virtual double stormbringer_proc_chance() const
  {
    double base_chance = 0;

    base_chance += p()->spec.stormbringer->proc_chance() +
                   p()->cache.mastery() * p()->mastery.enhanced_elements->effectN( 3 ).mastery_value();

    return base_chance;
  }

  void trigger_echoing_shock( player_t* target )
  {
    if ( !may_proc_echoing_shock )
    {
      return;
    }

    if ( !p()->buff.echoing_shock->up() )
    {
      return;
    }

    if ( exec_type == execute_type::ECHOING_SHOCK )
    {
      return;
    }

    if ( sim->debug )
    {
      sim->print_debug( "{} echoes {} (target={})", p()->name(), name(), target->name() );
    }

    make_event<echoing_shock_event_t>( *sim, this, target,
        p()->talent.echoing_shock->effectN( 2 ).time_value() );

    p()->buff.echoing_shock->decrement();
  }
};

// ==========================================================================
// Shaman Heal
// ==========================================================================

struct shaman_heal_t : public shaman_spell_base_t<heal_t>
{
  double elw_proc_high, elw_proc_low, resurgence_gain;

  bool proc_tidal_waves, consume_tidal_waves;

  shaman_heal_t( const std::string& token, shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() )
    : base_t( token, p, s ),
      elw_proc_high( .2 ),
      elw_proc_low( 1.0 ),
      resurgence_gain( 0 ),
      proc_tidal_waves( false ),
      consume_tidal_waves( false )
  {
    parse_options( options );
  }

  shaman_heal_t( shaman_t* p, const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() )
    : base_t( "", p, s ),
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

  shaman_pet_t( shaman_t* owner, const std::string& name, bool guardian = true, bool auto_attack = true )
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

  action_t* create_action( util::string_view name, const std::string& options_str ) override;

  virtual attack_t* create_auto_attack()
  {
    return nullptr;
  }

  // Apparently shaman pets by default do not inherit attack speed buffs from owner
  double composite_melee_speed() const override
  {
    return o()->cache.attack_haste();
  }
};

// ==========================================================================
// Base Shaman Pet Action
// ==========================================================================

template <typename T_PET, typename T_ACTION>
struct pet_action_t : public T_ACTION
{
  typedef pet_action_t<T_PET, T_ACTION> super;

  pet_action_t( T_PET* pet, const std::string& name, const spell_data_t* spell = spell_data_t::nil(),
                const std::string& options = std::string() )
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

  pet_melee_attack_t( T_PET* pet, const std::string& name, const spell_data_t* spell = spell_data_t::nil(),
                      const std::string& options = std::string() )
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

  pet_spell_t( T_PET* pet, const std::string& name, const spell_data_t* spell = spell_data_t::nil(),
               const std::string& options = std::string() )
    : pet_action_t<T_PET, spell_t>( pet, name, spell, options )
  {
    this->parse_options( options );
  }
};

// ==========================================================================
// Base Shaman Pet Method Definitions
// ==========================================================================

action_t* shaman_pet_t::create_action( util::string_view name, const std::string& options_str )
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

  base_wolf_t( shaman_t* owner, const std::string& name )
    : shaman_pet_t( owner, name ), alpha_wolf( nullptr ), alpha_wolf_buff( nullptr ), wolf_type( SPIRIT_WOLF )
  {
    owner_coeff.ap_from_ap = 0.6;

    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
  }
};

template <typename T>
struct wolf_base_attack_t : public pet_melee_attack_t<T>
{
  using super = wolf_base_attack_t<T>;

  wolf_base_attack_t( T* wolf, const std::string& n, const spell_data_t* spell = spell_data_t::nil(),
                      const std::string& options_str = std::string() )
    : pet_melee_attack_t<T>( wolf, n, spell )
  {
    this->parse_options( options_str );
  }

  void execute() override
  {
    pet_melee_attack_t<T>::execute();
  }

  void tick( dot_t* d ) override
  {
    pet_melee_attack_t<T>::tick( d );
  }
};

template <typename T>
struct wolf_base_auto_attack_t : public pet_melee_attack_t<T>
{
  using super = wolf_base_auto_attack_t<T>;

  wolf_base_auto_attack_t( T* wolf, const std::string& n, const spell_data_t* spell = spell_data_t::nil(),
                           const std::string& options_str = std::string() )
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
};

struct spirit_wolf_t : public base_wolf_t
{
  struct fs_melee_t : public wolf_base_auto_attack_t<spirit_wolf_t>
  {
    const spell_data_t* maelstrom;

    fs_melee_t( spirit_wolf_t* player ) : super( player, "melee" ), maelstrom( player->find_spell( 190185 ) )
    {
    }

    void impact( action_state_t* state ) override
    {
      melee_attack_t::impact( state );

      shaman_t* o = p()->o();
      if ( o->spec.feral_spirit_2->ok() )
      {
        o->trigger_maelstrom_gain( maelstrom->effectN( 1 ).resource( RESOURCE_MAELSTROM ), o->gain.feral_spirit );
      }
    }
  };

  spirit_wolf_t( shaman_t* owner ) : base_wolf_t( owner, "spirit_wolf" )
  {
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
    const spell_data_t* maelstrom;

    dw_melee_t( elemental_wolf_base_t* player ) : super( player, "melee" ), maelstrom( player->find_spell( 190185 ) )
    {
    }

    void impact( action_state_t* state ) override
    {
      super::impact( state );

      p()->o()->trigger_maelstrom_gain( maelstrom->effectN( 1 ).resource( RESOURCE_MAELSTROM ),
                                        p()->o()->gain.feral_spirit );
    }
  };

  cooldown_t* special_ability_cd;

  elemental_wolf_base_t( shaman_t* owner, const std::string& name )
    : base_wolf_t( owner, name ), special_ability_cd( nullptr )
  {
    // Make Wolves dynamic so we get accurate reporting for special abilities
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

  primal_elemental_t( shaman_t* owner, const std::string& name, bool guardian = false, bool auto_attack = true )
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

  action_t* create_action( util::string_view name, const std::string& options_str ) override
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
  earth_elemental_t( shaman_t* owner, bool guardian )
    : primal_elemental_t( owner, ( !guardian ) ? "primal_earth_elemental" : "greater_earth_elemental", guardian )
  {
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_sp      = 0.25;
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
    meteor_t( fire_elemental_t* player, const std::string& options )
      : super( player, "meteor", player->find_spell( 117588 ), options )
    {
      aoe = -1;
    }
  };

  struct fire_blast_t : public pet_spell_t<fire_elemental_t>
  {
    fire_blast_t( fire_elemental_t* player, const std::string& options )
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
    immolate_t( fire_elemental_t* player, const std::string& options )
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

  action_t* create_action( util::string_view name, const std::string& options_str ) override
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
  struct eye_of_the_storm_aoe_t : public pet_spell_t<storm_elemental_t>
  {
    int tick_number   = 0;
    double damage_amp = 0.0;

    eye_of_the_storm_aoe_t( storm_elemental_t* player, const std::string& options )
      : super( player, "eye_of_the_storm_aoe", player->find_spell( 269005 ), options )
    {
      aoe        = -1;
      background = true;

      // parent spell (eye_of_the_storm_t) has the damage increase percentage
      damage_amp = player->o()->find_spell( 157375 )->effectN( 2 ).percent();
    }

    double action_multiplier() const override
    {
      double m = pet_spell_t::action_multiplier();
      m *= std::pow( 1.0 + damage_amp, tick_number );
      return m;
    }
  };

  struct eye_of_the_storm_t : public pet_spell_t<storm_elemental_t>
  {
    eye_of_the_storm_aoe_t* breeze = nullptr;

    eye_of_the_storm_t( storm_elemental_t* player, const std::string& options )
      : super( player, "eye_of_the_storm", player->find_spell( 157375 ), options )
    {
      channeled   = true;
      tick_action = breeze = new eye_of_the_storm_aoe_t( player, options );
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
    wind_gust_t( storm_elemental_t* player, const std::string& options )
      : super( player, "wind_gust", player->find_spell( 157331 ), options )
    {
    }
  };

  struct call_lightning_t : public pet_spell_t<storm_elemental_t>
  {
    call_lightning_t( storm_elemental_t* player, const std::string& options )
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
      def->add_action( "eye_of_the_storm,if=buff.call_lightning.remains>=10" );
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

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "eye_of_the_storm" )
      return new eye_of_the_storm_t( this, options_str );
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
};

}  // end namespace pet

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct flametongue_weapon_spell_t : public shaman_spell_t  // flametongue_attack
{
  flametongue_weapon_spell_t( const std::string& n, shaman_t* player, weapon_t* /* w */ )
    : shaman_spell_t( n, player, player->find_spell( 10444 ) )
  {
    may_crit = background      = true;
    enable_enh_mastery_scaling = true;

    if ( player->specialization() == SHAMAN_ENHANCEMENT )
    {
      snapshot_flags          = STATE_AP;
      attack_power_mod.direct = 0.0264;

      if ( player->main_hand_weapon.type != WEAPON_NONE )
      {
        attack_power_mod.direct *= player->main_hand_weapon.swing_time.total_seconds() / 2.6;
      }
    }
  }
};

struct windfury_attack_t : public shaman_attack_t
{
  struct
  {
    std::array<proc_t*, 6> at_fw;
  } stats_;
  windfury_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w )
    : shaman_attack_t( n, player, s )
  {
    weapon     = w;
    school     = SCHOOL_PHYSICAL;
    background = true;

    // Windfury can not proc itself
    may_proc_windfury         = false;
    may_proc_maelstrom_weapon = true;

    for ( size_t i = 0; i < stats_.at_fw.size(); i++ )
      stats_.at_fw[ i ] = player->get_proc( "Windfury-ForcefulWinds: " + std::to_string( i ) );
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

  // Needs to do maelstrom weapon things
  /*double maelstrom_weapon_energize_amount( const action_state_t* source ) const override
  {
    return shaman_attack_t::maelstrom_weapon_energize_amount( source );
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();
    m *= 1.0 + p()->buff.forceful_winds->stack_value();
    return m;
  }

  void impact( action_state_t* state ) override
  {
    if ( p()->talent.forceful_winds->ok() )
    {
      stats_.at_fw[ p()->buff.forceful_winds->check() ]->occur();
      double bonus_resource =
          p()->buff.forceful_winds->s_data->effectN( 2 ).base_value() * p()->buff.forceful_winds->check();
      p()->trigger_maelstrom_gain( bonus_resource, p()->gain.forceful_winds );
    }

    shaman_attack_t::impact( state );
  }*/
};

struct crash_lightning_attack_t : public shaman_attack_t
{
  crash_lightning_attack_t( shaman_t* p ) : shaman_attack_t( "crash_lightning_aoe", p, p->find_spell( 195592 ) )
  {
    weapon     = &( p->main_hand_weapon );
    background = true;
    aoe        = -1;
    base_multiplier *= 1.0;
    may_proc_ability_procs = false;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_maelstrom_weapon = false;
  }
};

struct crashing_storm_damage_t : public shaman_spell_t
{
  crashing_storm_damage_t( shaman_t* player ) : shaman_spell_t( "crashing_storm", player, player->find_spell( 210801 ) )
  {
    aoe        = -1;
    ground_aoe = background = true;
    school                  = SCHOOL_NATURE;
    // attack_power_mod.direct = 0.125;  // still cool to hardcode the SP% into tooltip
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );
  }
};

struct crashing_storm_t : public shaman_spell_t
{
  crashing_storm_damage_t* zap;

  crashing_storm_t( shaman_t* player )
    : shaman_spell_t( "crashing_storm", player, player->find_spell( 192246 ) ),
      zap( new crashing_storm_damage_t( player ) )
  {
    background   = true;
    dot_duration = timespan_t::zero();  // The periodic effect is handled by ground_aoe_event_t
    add_child( zap );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    make_event<ground_aoe_event_t>(
        *sim, p(),
        ground_aoe_params_t().target( execute_state->target ).duration( timespan_t::from_seconds( 6 ) ).action( zap ),
        true );  // No duration in spell data rn, cool.
  }
};

struct icy_edge_attack_t : public shaman_attack_t
{
  icy_edge_attack_t( const std::string& n, shaman_t* p, weapon_t* w ) : shaman_attack_t( n, p, p->find_spell( 271920 ) )
  {
    weapon                 = w;
    background             = true;
    may_proc_ability_procs = false;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_maelstrom_weapon = false;
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  stormstrike_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w )
    : shaman_attack_t( n, player, s )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
    weapon                           = w;
    base_multiplier *= 1.0;
    school = SCHOOL_PHYSICAL;
  }

  void init() override
  {
    shaman_attack_t::init();
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p()->buff.crackling_surge->up() )
    {
      for ( int x = 1; x <= p()->buff.crackling_surge->check(); x++ )
      {
        m *= 1.0 + p()->buff.crackling_surge->value();
      }
    }

    if ( p()->buff.stormbringer->up() )
    {
      m *= 1.0 + p()->buff.stormbringer->data().effectN( 4 ).percent();
    }

    if ( p()->buff.gathering_storms->up() )
    {
      m *= p()->buff.gathering_storms->check_value();
    }

    return m;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = shaman_attack_t::bonus_da( s );

    return b;
  }

  double composite_crit_chance() const override
  {
    double c = shaman_attack_t::composite_crit_chance();

    return c;
  }

  void execute() override
  {
    shaman_attack_t::execute();
  }
};

struct windstrike_attack_t : public stormstrike_attack_t
{
  windstrike_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w )
    : stormstrike_attack_t( n, player, s, w )
  {
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = shaman_attack_t::bonus_da( s );

    return b;
  }

  double target_armor( player_t* ) const override
  {
    return 0.0;
  }
};

struct windlash_t : public shaman_attack_t
{
  double swing_timer_variance;

  windlash_t( const std::string& n, const spell_data_t* s, shaman_t* player, weapon_t* w, double stv )
    : shaman_attack_t( n, player, s ), swing_timer_variance( stv )
  {
    background = repeating = may_miss = may_dodge = may_parry = true;
    may_proc_ability_procs = may_glance = special = false;
    weapon                                        = w;
    weapon_multiplier                             = 1.0;
    base_execute_time                             = w->swing_time;
    trigger_gcd                                   = timespan_t::zero();

    may_proc_maelstrom_weapon = true;  // Presumption, but should be safe
  }

  // Windlash is a special ability, but treated as an autoattack in terms of proccing
  proc_types proc_type() const override
  {
    return PROC1_MELEE;
  }

  double target_armor( player_t* ) const override
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
  ground_aoe_spell_t( shaman_t* p, const std::string& name, const spell_data_t* spell ) : spell_t( name, p, spell )
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

struct earthen_rage_spell_t : public shaman_spell_t
{
  earthen_rage_spell_t( shaman_t* p ) : shaman_spell_t( "earthen_rage", p, p->find_spell( 170379 ) )
  {
    background = proc = true;
  }
};

// Elemental overloads

struct elemental_overload_spell_t : public shaman_spell_t
{
  elemental_overload_spell_t( shaman_t* p, const std::string& name, const spell_data_t* s )
    : shaman_spell_t( name, p, s )
  {
    base_execute_time = timespan_t::zero();
    background        = true;
    callbacks         = false;

    base_multiplier *= p->mastery.elemental_overload->effectN( 2 ).percent();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->talent.unlimited_power->ok() )
    {
      p()->buff.unlimited_power->trigger();
    }
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

  melee_t( const std::string& name, const spell_data_t* s, shaman_t* player, weapon_t* w, int sw, double stv )
    : shaman_attack_t( name, player, s ), sync_weapons( sw ), first( true ), swing_timer_variance( stv )
  {
    background = repeating = may_glance = true;
    special                             = false;
    trigger_gcd                         = timespan_t::zero();
    weapon                              = w;
    weapon_multiplier                   = 1.0;
    base_execute_time                   = w->swing_time;

    if ( p()->specialization() == SHAMAN_ENHANCEMENT && p()->dual_wield() )
      base_hit -= 0.19;

    may_proc_maelstrom_weapon = true;
    may_proc_icy_edge         = true;
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

  auto_attack_t( shaman_t* player, const std::string& options_str )
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
    p()->ascendance_mh = new windlash_t( "Windlash", player->find_spell( 114089 ), player, &( p()->main_hand_weapon ),
                                         swing_timer_variance );

    p()->main_hand_attack = p()->melee_mh;

    if ( p()->off_hand_weapon.type != WEAPON_NONE && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( !p()->dual_wield() )
        return;

      p()->melee_oh = new melee_t( "Off-Hand", spell_data_t::nil(), player, &( p()->off_hand_weapon ), sync_weapons,
                                   swing_timer_variance );
      p()->melee_oh->school = SCHOOL_PHYSICAL;
      p()->ascendance_oh    = new windlash_t( "Windlash Off-Hand", player->find_spell( 114093 ), player,
                                           &( p()->off_hand_weapon ), swing_timer_variance );

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

struct molten_weapon_dot_t : public residual_action::residual_periodic_action_t<shaman_spell_t>
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

  lava_lash_t( shaman_t* player, const std::string& options_str )
    : shaman_attack_t( "lava_lash", player, player->find_specialization_spell( "Lava Lash" ) ), mw_dot( nullptr )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    school = SCHOOL_FIRE;

    base_multiplier *= 1.0;

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

  double bonus_da( const action_state_t* s ) const override
  {
    double b = shaman_attack_t::bonus_da( s );

    return b;
  }

  double cost() const override
  {
    if ( p()->buff.hot_hand->check() )
    {
      return 0;
    }

    return shaman_attack_t::cost();
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    auto m = shaman_attack_t::composite_target_multiplier( target );

    return m;
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p()->buff.hot_hand->up() )
    {
      m *= 1.0 + p()->buff.hot_hand->data().effectN( 1 ).percent();
    }

    return m;
  }

  void execute() override
  {
    shaman_attack_t::execute();
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    if ( result_is_hit( state->result ) && p()->buff.crash_lightning->up() )
    {
      p()->action.crash_lightning_aoe->set_target( state->target );
      p()->action.crash_lightning_aoe->schedule_execute();
    }

    if ( p()->buff.molten_weapon->up() )
    {
      trigger_molten_weapon_dot( state->target, state->result_amount );
    }
  }

  virtual void trigger_molten_weapon_dot( player_t* t, double dmg )
  {
    double wolf_count_multi = p()->buff.molten_weapon->data().effectN( 2 ).percent() * p()->buff.molten_weapon->check();
    double feed_amount      = wolf_count_multi * dmg;
    residual_action::trigger( p()->action.molten_weapon_dot,  // ignite spell
                              t,                              // target
                              feed_amount );
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_base_t : public shaman_attack_t
{
  stormstrike_attack_t *mh, *oh;
  bool background_action;

  stormstrike_base_t( shaman_t* player, const std::string& name, const spell_data_t* spell,
                      const std::string& options_str )
    : shaman_attack_t( name, player, spell ), mh( nullptr ), oh( nullptr ), background_action( false )
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
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( p()->buff.stormbringer->up() || background == true )
    {
      cd_duration = timespan_t::zero();
    }

    shaman_attack_t::update_ready( cd_duration );
  }

  double cost() const override
  {
    double c = shaman_attack_t::cost();

    if ( p()->buff.stormbringer->check() )
    {
      c *= 1.0 + p()->buff.stormbringer->data().effectN( 3 ).percent();
    }

    return c;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      mh->execute();
      if ( oh )
      {
        oh->execute();
      }

      if ( p()->buff.crash_lightning->up() )
      {
        p()->action.crash_lightning_aoe->set_target( execute_state->target );
        p()->action.crash_lightning_aoe->execute();
      }
    }

    p()->buff.gathering_storms->decrement();
    p()->buff.stormbringer->decrement();
  }

  void reset() override
  {
    shaman_attack_t::reset();
    background = background_action;
    dual       = false;
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );
  }
};

struct stormstrike_t : public stormstrike_base_t
{
  stormstrike_t( shaman_t* player, const std::string& options_str )
    : stormstrike_base_t( player, "stormstrike", player->find_specialization_spell( "Stormstrike" ), options_str )
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

  void execute() override
  {
    stormstrike_base_t::execute();
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

  windstrike_t( shaman_t* player, const std::string& options_str )
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

  double cost() const override
  {
    double c = stormstrike_base_t::cost();

    if ( p()->buff.ascendance->check() )
    {
      c *= 1.0 + p()->buff.ascendance->data().effectN( 5 ).percent();
    }

    return c;
  }

  bool ready() override
  {
    if ( p()->buff.ascendance->remains() <= cooldown->queue_delay() )
      return false;

    return stormstrike_base_t::ready();
  }

  void execute() override
  {
    stormstrike_base_t::execute();
  }
};

// Ice Strike Spell ========================================================

struct ice_strike_t : public shaman_spell_t
{
  ice_strike_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "ice_strike", player, player->talent.ice_strike, options_str )
  {
    // placeholder
  }
};

// Sundering Spell =========================================================

struct sundering_t : public shaman_attack_t
{
  sundering_t( shaman_t* player, const std::string& options_str )
    : shaman_attack_t( "sundering", player, player->talent.sundering )
  {
    parse_options( options_str );
    school = SCHOOL_FLAMESTRIKE;
    aoe    = -1;  // TODO: This is likely not going to affect all enemies but it will do for now
  }

  void init() override
  {
    shaman_attack_t::init();
    may_proc_stormbringer = may_proc_windfury = may_proc_flametongue = false;
    may_proc_hot_hand                                                = p()->talent.hot_hand->ok();
  }
};

// Flametongue Spell =========================================================

// Needs to imbue MH on cast
struct flametongue_t : public shaman_spell_t
{
  flametongue_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "flametongue", player, player->find_specialization_spell( "Flametongue" ), options_str )
  {
    add_child( player->flametongue );
  }

  void init() override
  {
    shaman_spell_t::init();
    may_proc_stormbringer = true;
  }

  void execute() override
  {
    shaman_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    shaman_spell_t::impact( s );
  }
};

// Crash Lightning Attack ===================================================

struct crash_lightning_t : public shaman_attack_t
{
  size_t ecc_min_targets;

  crash_lightning_t( shaman_t* player, const std::string& options_str )
    : shaman_attack_t( "crash_lightning", player, player->find_specialization_spell( "Crash Lightning" ) ),
      ecc_min_targets( 0 )
  {
    parse_options( options_str );

    aoe     = -1;
    weapon  = &( p()->main_hand_weapon );
    ap_type = attack_power_type::WEAPON_BOTH;

    if ( player->action.crashing_storm )
    {
      add_child( player->action.crashing_storm );
    }

    if ( player->action.crash_lightning_aoe )
    {
      add_child( player->action.crash_lightning_aoe );
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    return m;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( p()->talent.crashing_storm->ok() )
    {
      // p()->action.crashing_storm->schedule_execute();

      /*make_event<ground_aoe_event_t>( *sim, p(),
                                      ground_aoe_params_t()
                                          .target( execute_state->target )
                                          .duration( p()->find_spell( 205532 )->duration() )
                                          .action( p()->action.crashing_storm ),
                                      true );*/

      make_event<ground_aoe_event_t>( *sim, p(),
                                      ground_aoe_params_t()
                                          .target( execute_state->target )
                                          .duration( timespan_t::from_seconds( 6 ) )
                                          .action( p()->action.crashing_storm ),
                                      true );
    }

    if ( result_is_hit( execute_state->result ) )
    {
      if ( execute_state->n_targets > ( 1 - 0 ) )  // currently bugged and proc'ing on 1 target less than it should.
      {
        p()->buff.crash_lightning->trigger();
      }

      double v = 1.0 + p()->buff.gathering_storms->default_value *
                           ( execute_state->n_targets +
                             0 );  // currently bugged and acting as if there's an extra target present
      p()->buff.gathering_storms->trigger( 1, v );
    }
  }
};

// Earthquake ===============================================================

struct earthquake_damage_t : public shaman_spell_t
{
  double kb_chance;

  earthquake_damage_t( shaman_t* player )
    : shaman_spell_t( "earthquake_", player, player->find_spell( 77478 ) ), kb_chance( data().effectN( 2 ).percent() )
  {
    aoe        = -1;
    ground_aoe = background = true;
    school                  = SCHOOL_PHYSICAL;
    spell_power_mod.direct  = 0.2875;  // still cool to hardcode the SP% into tooltip
  }

  double target_armor( player_t* ) const override
  {
    return 0;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );

    m *= 1.0 + p()->buff.master_of_the_elements->value();

    if ( p()->buff.echoes_of_great_sundering->up() )
    {
      m *= 1.0 + p()->buff.echoes_of_great_sundering->value();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );
  }
};

struct earthquake_t : public shaman_spell_t
{
  earthquake_damage_t* rumble;

  earthquake_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "earthquake", player, player->find_specialization_spell( "Earthquake" ), options_str ),
      rumble( new earthquake_damage_t( player ) )
  {
    dot_duration = timespan_t::zero();  // The periodic effect is handled by ground_aoe_event_t
    add_child( rumble );
  }

  double cost() const override
  {
    double d = shaman_spell_t::cost();
    return d;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    make_event<ground_aoe_event_t>(
        *sim, p(),
        ground_aoe_params_t().target( execute_state->target ).duration( data().duration() ).action( rumble ) );

    // Note, needs to be decremented after ground_aoe_event_t is created so that the rumble gets the
    // buff multiplier as persistent.
    p()->buff.master_of_the_elements->expire();
    p()->buff.echoes_of_great_sundering->expire();
  }
};

// Earth Elemental ===========================================================

struct earth_elemental_t : public shaman_spell_t
{
  earthquake_damage_t* rumble;

  earth_elemental_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "earth_elemental", player, player->find_spell( 188616 ), options_str ),
      rumble( new earthquake_damage_t( player ) )
  {
    harmful = may_crit = false;
    cooldown->duration =
        player->find_spell( 198103 )->cooldown();  // earth ele cd and durations are on different spells.. go figure.
    add_child( rumble );
  }

  void execute() override
  {
    shaman_spell_t::execute();
    if ( p()->legendary.deeptremor_stone->ok() )
    {
      make_event<ground_aoe_event_t>(
          *sim, p(),
          ground_aoe_params_t().target( execute_state->target ).duration( data().duration() ).action( rumble ) );
    }

    if ( p()->talent.primal_elementalist->ok() )
    {
      p()->pet.pet_earth_elemental->summon( s_data->duration() );
    }
    else
    {
      p()->pet.guardian_earth_elemental->summon( s_data->duration() );
    }
  }
};

// Fire Elemental ===========================================================

struct fire_elemental_t : public shaman_spell_t
{
  fire_elemental_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "fire_elemental", player, player->find_specialization_spell( "Fire Elemental" ), options_str )
  {
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
  storm_elemental_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "storm_elemental", player, player->talent.storm_elemental, options_str )
  {
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

    p()->summon_storm_elemental( storm_elemental_duration );
  }
};

// Eathen Spike =============================================================

struct earthen_spike_t : public shaman_attack_t
{
  earthen_spike_t( shaman_t* player, const std::string& options_str )
    : shaman_attack_t( "earthen_spike", player, player->talent.earthen_spike )
  {
    parse_options( options_str );
  }

  void init() override
  {
    shaman_attack_t::init();
  }

  void impact( action_state_t* s ) override
  {
    shaman_attack_t::impact( s );

    td( target )->debuff.earthen_spike->trigger();
  }
};

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "lightning_shield", player, player->find_talent_spell( "Lightning Shield" ), options_str )
  {
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

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "bloodlust", player, player->find_class_spell( "Bloodlust" ), options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    for ( size_t i = 0; i < sim->player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim->player_non_sleeping_list[ i ];
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
  chained_overload_base_t( shaman_t* p, const std::string& name, const spell_data_t* spell, double mg )
    : elemental_overload_spell_t( p, name, spell )
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

    if ( data().affected_by( p->spec.chain_lightning_2->effectN( 1 ) ) )
    {
      aoe += (int)p->spec.chain_lightning_2->effectN( 1 ).base_value();
    }
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }
};

struct chain_lightning_overload_t : public chained_overload_base_t
{
  chain_lightning_overload_t( shaman_t* p )
    : chained_overload_base_t( p, "chain_lightning_overload", p->find_spell( 45297 ),
                               p->find_spell( 343725 )->effectN( 6 ).resource( RESOURCE_MAELSTROM ) )
  {
    affected_by_master_of_the_elements = true;
  }

  void impact( action_state_t* state ) override
  {
    chained_overload_base_t::impact( state );
  }
};

struct lava_beam_overload_t : public chained_overload_base_t
{
  lava_beam_overload_t( shaman_t* p )
    : chained_overload_base_t( p, "lava_beam_overload", p->find_spell( 114738 ),
                               p->find_spell( 343725 )->effectN( 6 ).resource( RESOURCE_MAELSTROM ) )
  {
  }
};

struct chained_base_t : public shaman_spell_t
{
  chained_base_t( shaman_t* player, const std::string& name, const spell_data_t* spell, double mg,
                  const std::string& options_str )
    : shaman_spell_t( name, player, spell, options_str )
  {
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

    if ( data().affected_by( player->spec.chain_lightning_2->effectN( 1 ) ) )
    {
      aoe += (int)player->spec.chain_lightning_2->effectN( 1 ).base_value();
    }
  }

  double overload_chance( const action_state_t* s ) const override
  {
    /*
    // Please check shaman_spell_t for the overload logic of Stormkeeper
    if ( p()->buff.stormkeeper->check() )
    {
      return 1.0;
    }*/

    double base_chance = shaman_spell_t::overload_chance( s );

    return base_chance / 3.0;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->buff.stormkeeper->check() )
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
  chain_lightning_t( shaman_t* player, const std::string& options_str )
    : chained_base_t( player, "chain_lightning", player->find_class_spell( "Chain Lightning" ),
                      player->find_spell( 343725 )->effectN( 5 ).resource( RESOURCE_MAELSTROM ), options_str )
  {
    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new chain_lightning_overload_t( player );
      add_child( overload );
    }
    affected_by_master_of_the_elements = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = chained_base_t::execute_time();

    t *= 1.0 + p()->buff.wind_gust->stack_value();

    if ( p()->buff.stormkeeper->up() )
    {
      // stormkeeper has a -100% value as effect 1
      t *= 1 + p()->talent.stormkeeper->effectN( 1 ).percent();
    }

    if ( p()->buff.chains_of_devastation_chain_lightning->up() )
    {
      t *= 1 + p()->buff.chains_of_devastation_chain_lightning->data().effectN( 1 ).percent();
    }

    return t;
  }

  timespan_t gcd() const override
  {
    timespan_t t = chained_base_t::gcd();
    t *= 1.0 + p()->buff.wind_gust->stack_value();

    // testing shows the min GCD is 0.5 sec
    if ( t < timespan_t::from_millis( 500 ) )
    {
      t = timespan_t::from_millis( 500 );
    }
    return t;
  }

  bool ready() override
  {
    if ( p()->specialization() == SHAMAN_ELEMENTAL && p()->buff.ascendance->check() )
      return false;

    return shaman_spell_t::ready();
  }

  /* Number of potential overloads */
  size_t n_overload_chances( const action_state_t* ) const override
  {
    return (size_t)0;
  }

  void impact( action_state_t* state ) override
  {
    chained_base_t::impact( state );
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
  }
};

struct lava_beam_t : public chained_base_t
{
  // This is actually a tooltip bug in-game: real testing shows that Lava Beam and
  // Lava Beam Overload generate resources identical to their Chain Lightning counterparts
  lava_beam_t( shaman_t* player, const std::string& options_str )
    : chained_base_t( player, "lava_beam", player->find_spell( 114074 ),
                      player->find_spell( 343725 )->effectN( 5 ).resource( RESOURCE_MAELSTROM ), options_str )
  {
    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lava_beam_overload_t( player );
      add_child( overload );
    }
  }

  bool ready() override
  {
    if ( !p()->buff.ascendance->check() )
      return false;

    return shaman_spell_t::ready();
  }

  /* Number of potential overloads */
  size_t n_overload_chances( const action_state_t* ) const override
  {
    return (size_t)0;
  }
};

// Lava Burst Spell =========================================================

// As of 8.1 Lava Burst checks its state on impact. Lava Burst -> Flame Shock now forces the critical strike
struct lava_burst_overload_t : public elemental_overload_spell_t
{
  unsigned impact_flags;

  lava_burst_overload_t( shaman_t* player )
    : elemental_overload_spell_t( player, "lava_burst_overload", player->find_spell( 77451 ) ), impact_flags()
  {
    maelstrom_gain         = player->find_spell( 343725 )->effectN( 4 ).resource( RESOURCE_MAELSTROM );
    spell_power_mod.direct = player->find_spell( 285466 )->effectN( 1 ).sp_coeff();
  }

  void init() override
  {
    elemental_overload_spell_t::init();

    std::swap( snapshot_flags, impact_flags );
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

    s->result        = elemental_overload_spell_t::calculate_result( s );
    s->result_amount = elemental_overload_spell_t::calculate_direct_amount( s );

    elemental_overload_spell_t::impact( s );
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p()->buff.ascendance->up() )
    {
      m *= 1.0 + p()->cache.spell_crit_chance();
    }

    return m;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = shaman_spell_t::bonus_da( s );

    return b;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_crit_chance( t );

    if ( p()->spec.lava_burst_2->ok() && td( target )->dot.flame_shock->is_ticking() )
    {
      // hardcoded because I didn't find it in spelldata yet
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
          "%s surge_of_power spreads flame_shock from %s to shortest remaining target %s (remains=%.3f)",
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
      sim->out_debug.printf( "%s surge_of_power spreads flame_shock from %s to closest target %s (distance=%.3f)",
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

/**
 * As of 8.1 Lava Burst checks its state on impact. Lava Burst -> Flame Shock now forces the critical strike
 */
struct lava_burst_t : public shaman_spell_t
{
  unsigned impact_flags;

  lava_burst_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "lava_burst", player, player->find_specialization_spell( "Lava Burst" ), options_str ),
      impact_flags()
  {
    // Manacost is only for resto
    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      base_costs[ RESOURCE_MANA ] = 0;
      maelstrom_gain              = player->find_spell( 343725 )->effectN( 3 ).resource( RESOURCE_MAELSTROM );
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lava_burst_overload_t( player );
      add_child( overload );
    }

    if ( p()->specialization() == SHAMAN_RESTORATION )
      resource_current = RESOURCE_MANA;

    spell_power_mod.direct = player->find_spell( 285452 )->effectN( 1 ).sp_coeff();
  }

  void init() override
  {
    shaman_spell_t::init();

    std::swap( snapshot_flags, impact_flags );

    // Elemental and Restoration gain a second Lava Burst charge via Echo of the Elements
    if ( p()->talent.echo_of_the_elements->ok() )
    {
      cooldown->charges = (int)data().charges() + (int)p()->talent.echo_of_the_elements->effectN( 2 ).base_value();
    }
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

    if ( result_is_hit( s->result ) )
    {
      if ( p()->buff.surge_of_power->up() )
      {
        p()->cooldown.fire_elemental->adjust( -1.0 * p()->talent.surge_of_power->effectN( 1 ).time_value() );
        p()->cooldown.storm_elemental->adjust( -1.0 * p()->talent.surge_of_power->effectN( 1 ).time_value() );
        p()->buff.surge_of_power->decrement();
      }
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = shaman_spell_t::bonus_da( s );

    return b;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p()->buff.ascendance->up() )
    {
      m *= 1.0 + p()->cache.spell_crit_chance();
    }

    return m;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_crit_chance( t );

    if ( p()->spec.lava_burst_2->ok() && td( target )->dot.flame_shock->is_ticking() )
    {
      // hardcoded because I didn't find it in spell data yet
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

    if ( p()->specialization() == SHAMAN_ELEMENTAL && p()->covenant.necrolord->ok() && p()->buff.primordial_wave->up() )
    {
      // TODO: trigger a Lava Burst on every Flame Shocked target in the future
      p()->buff.primordial_wave->expire();
    }

    // Echoed Lava Burst does not generate Master of the Elements
    if ( !is_echoed_spell() && p()->talent.master_of_the_elements->ok() )
    {
      p()->buff.master_of_the_elements->trigger();
    }

    // Lava Surge buff does not get eaten, if the Lava Surge proc happened
    // during the Lava Burst cast
    if ( !p()->lava_surge_during_lvb && p()->buff.lava_surge->check() )
      p()->buff.lava_surge->expire();

    p()->lava_surge_during_lvb = false;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buff.lava_surge->up() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::execute_time();
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_overload_t : public elemental_overload_spell_t
{
  lightning_bolt_overload_t( shaman_t* p )
    : elemental_overload_spell_t( p, "lightning_bolt_overload", p->find_spell( 45284 ) )
  {
    maelstrom_gain                     = player->find_spell( 343725 )->effectN( 2 ).resource( RESOURCE_MAELSTROM );
    affected_by_master_of_the_elements = true;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    auto m = shaman_spell_t::composite_target_multiplier( target );
    return m;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();
    if ( p()->buff.stormkeeper->up() )
    {
      m *= 1.0 + p()->talent.stormkeeper->effectN( 2 ).percent();
    }
    return m;
  }

  void impact( action_state_t* state ) override
  {
    elemental_overload_spell_t::impact( state );
  }
};

struct lightning_bolt_t : public shaman_spell_t
{
  double m_overcharge;

  lightning_bolt_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "lightning_bolt", player, player->find_class_spell( "Lightning Bolt" ), options_str ),
      m_overcharge( 0 )
  {
    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      affected_by_master_of_the_elements = true;
      maelstrom_gain                     = player->find_spell( 343725 )->effectN( 1 ).resource( RESOURCE_MAELSTROM );
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lightning_bolt_overload_t( player );
      add_child( overload );
    }
  }

  // TODO: once bug is fixed, uncomment this
  // double composite_maelstrom_gain_coefficient( const action_state_t* state ) const override
  // {
  //   double coeff = shaman_spell_t::composite_maelstrom_gain_coefficient( state );
  //   if ( p()->conduit.high_voltage->ok() && rng().roll( p()->conduit.high_voltage.percent() ) )
  //     coeff *= 2.0;
  //   return coeff;
  // }

  double overload_chance( const action_state_t* s ) const override
  {
    double chance = shaman_spell_t::overload_chance( s );

    /*
    if ( p()->buff.stormkeeper->check() )
    {
      chance = 1.0;
    }*/

    return chance;
  }

  /* Number of guaranteed overloads */
  size_t n_overloads( const action_state_t* t ) const override
  {
    size_t n = shaman_spell_t::n_overloads( t );
    // Surge of Power is an addition to the base overload chance
    if ( p()->buff.surge_of_power->up() )
    {
      // roll one 100 sided dice once.
      //  2% chance to get 3 overloads,
      // 18% chance to get 2 overloads,
      // 80% chance to get 1 overload
      double roll = rng().real();
      if ( roll >= 0.98 )
      {
        n += 3u;
      }
      else if ( roll >= 0.8 )
      {
        n += 2u;
      }
      else
      {
        n += 1u;
      }
    }

    return n;
  }

  /* Number of potential overloads */
  size_t n_overload_chances( const action_state_t* t ) const override
  {
    return shaman_spell_t::n_overload_chances( t );
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    auto m = shaman_spell_t::composite_target_multiplier( target );
    return m;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();
    if ( p()->buff.stormkeeper->up() )
    {
      m *= 1.0 + p()->talent.stormkeeper->effectN( 2 ).percent();
    }
    return m;
  }

  double spell_direct_power_coefficient( const action_state_t* /* state */ ) const override
  {
    return spell_power_mod.direct * ( 1.0 + m_overcharge * cost() );
  }

  timespan_t execute_time() const override
  {
    if ( p()->buff.stormkeeper->up() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::execute_time() * ( 1.0 + p()->buff.wind_gust->stack_value() );
  }

  timespan_t gcd() const override
  {
    timespan_t t = shaman_spell_t::gcd();
    t *= 1.0 + p()->buff.wind_gust->stack_value();

    // testing shows the min GCD is 0.5 sec
    if ( t < timespan_t::from_millis( 500 ) )
    {
      t = timespan_t::from_millis( 500 );
    }
    return t;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    // TODO: remove this when the high voltage bug is fixed and it properly generates double instead of 5
    if ( p()->conduit.high_voltage->ok() && rng().roll( p()->conduit.high_voltage.percent() ) )
    {
      p()->trigger_maelstrom_gain( 5.0, p()->gain.high_voltage );
    }

    if ( p()->specialization() == SHAMAN_ENHANCEMENT && p()->covenant.necrolord->ok() &&
         p()->buff.primordial_wave->up() )
    {
      // TODO: trigger a Lightning Bolt on every Flame Shocked target in the future
      p()->buff.primordial_wave->expire();
    }

    p()->buff.stormkeeper->decrement();

    p()->buff.surge_of_power->decrement();

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
  }

  void reset_swing_timers()
  {
    if ( player->main_hand_attack && player->main_hand_attack->execute_event )
    {
      event_t::cancel( player->main_hand_attack->execute_event );
      player->main_hand_attack->schedule_execute();
    }

    if ( player->off_hand_attack && player->off_hand_attack->execute_event )
    {
      event_t::cancel( player->off_hand_attack->execute_event );
      player->off_hand_attack->schedule_execute();
    }
  }
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
  elemental_blast_overload_t( shaman_t* p )
    : elemental_overload_spell_t( p, "elemental_blast_overload", p->find_spell( 120588 ) )
  {
    affected_by_master_of_the_elements = true;
    maelstrom_gain                     = player->find_spell( 343725 )->effectN( 11 ).resource( RESOURCE_MAELSTROM );
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
  elemental_blast_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "elemental_blast", player, player->talent.elemental_blast, options_str )
  {
    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      affected_by_master_of_the_elements = true;
      maelstrom_gain                     = player->find_spell( 343725 )->effectN( 10 ).resource( RESOURCE_MAELSTROM );

      overload = new elemental_blast_overload_t( player );
      add_child( overload );
    }
  }

  void execute() override
  {
    // Trigger buff before executing the spell, because apparently the buffs affect the cast result
    // itself.
    trigger_elemental_blast_proc( p() );

    shaman_spell_t::execute();
  }
};

// Icefury Spell ====================================================

struct icefury_overload_t : public elemental_overload_spell_t
{
  icefury_overload_t( shaman_t* p ) : elemental_overload_spell_t( p, "icefury_overload", p->find_spell( 219271 ) )
  {
    affected_by_master_of_the_elements = true;
    maelstrom_gain                     = player->find_spell( 343725 )->effectN( 9 ).resource( RESOURCE_MAELSTROM );
  }
};

struct icefury_t : public shaman_spell_t
{
  icefury_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "icefury", player, player->talent.icefury, options_str )
  {
    affected_by_master_of_the_elements = true;
    maelstrom_gain                     = player->find_spell( 343725 )->effectN( 8 ).resource( RESOURCE_MAELSTROM );

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new icefury_overload_t( player );
      add_child( overload );
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
  feral_spirit_spell_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "feral_spirit", player, player->find_specialization_spell( "Feral Spirit" ), options_str )
  {
    harmful = false;

    cooldown->duration += player->talent.elemental_spirits->effectN( 1 ).time_value();
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
  thunderstorm_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "thunderstorm", player, player->find_specialization_spell( "Thunderstorm" ), options_str )
  {
    aoe = -1;
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "spiritwalkers_grace", player, player->find_specialization_spell( "Spiritwalker's Grace" ),
                      options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
    if ( p()->talent.graceful_spirit->ok() )
    {
      cooldown->duration += p()->talent.graceful_spirit->effectN( 1 ).time_value();
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.spiritwalkers_grace->trigger();
  }
};

struct spirit_walk_t : public shaman_spell_t
{
  spirit_walk_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "spirit_walk", player, player->find_specialization_spell( "Spirit Walk" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.spirit_walk->trigger();
  }
};

struct ghost_wolf_t : public shaman_spell_t
{
  ghost_wolf_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "ghost_wolf", player, player->find_class_spell( "Ghost Wolf" ), options_str )
  {
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

  feral_lunge_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "feral_lunge", player, player->talent.feral_lunge, options_str )
  {
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

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================
struct earth_shock_t : public shaman_spell_t
{
  earth_shock_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "earth_shock", player, player->find_specialization_spell( "Earth Shock" ), options_str )
  {
    // hardcoded because spelldata doesn't provide the resource type
    resource_current                   = RESOURCE_MAELSTROM;
    affected_by_master_of_the_elements = true;
  }

  double cost() const override
  {
    double d = shaman_spell_t::cost();

    return d;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = shaman_spell_t::bonus_da( s );

    return b;
  }

  double action_multiplier() const override
  {
    auto m = shaman_spell_t::action_multiplier();

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    // Echoed Earth Shock does not generate Surge of Power stacks
    if ( !is_echoed_spell() && p()->talent.surge_of_power->ok() )
    {
      p()->buff.surge_of_power->trigger();
    }

    if ( p()->legendary.echoes_of_great_sundering->ok() )
    {
      p()->buff.echoes_of_great_sundering->trigger();
    }

    if ( p()->legendary.echoes_of_great_sundering->ok() )
    {
      p()->buff.echoes_of_great_sundering->trigger();
    }
    if ( p()->conduit.pyroclastic_shock->ok() && rng().roll( p()->conduit.pyroclastic_shock.percent() ) )
    {
      dot_t* fs = this->td(target)->dot.flame_shock;
      timespan_t extend_time = p()->conduit.pyroclastic_shock->effectN( 2 ).time_value();
      if ( fs->is_ticking() )
      {
        fs->adjust_duration( extend_time );
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  flame_shock_spreader_t* spreader;
  const spell_data_t* elemental_resource;
  const spell_data_t* skybreakers_effect;

  flame_shock_t( shaman_t* player, const std::string& options_str = std::string() )
    : shaman_spell_t( "flame_shock", player, player->find_class_spell( "Flame Shock" ), options_str ),
      spreader( player->talent.surge_of_power->ok() ? new flame_shock_spreader_t( player ) : nullptr ),
      elemental_resource( player->find_spell( 263819 ) ),
      skybreakers_effect( player->find_spell( 336734 ) )
  {
    tick_may_crit  = true;
    track_cd_waste = false;
  }

  double composite_crit_chance() const override
  {
    double m = shaman_spell_t::composite_crit_chance();

    if ( p()->legendary.skybreakers_fiery_demise->ok() )
    {
      m += skybreakers_effect->effectN( 3 ).percent();
    }

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = shaman_spell_t::action_ta_multiplier();

    return m;
  }

  void tick( dot_t* d ) override
  {
    shaman_spell_t::tick( d );

    double proc_chance = p()->spec.lava_surge->proc_chance();

    // proc chance suddenly bacame 100% and the actual chance became effectN 1
    proc_chance = p()->spec.lava_surge->effectN( 1 ).percent();

    if ( p()->spec.restoration_shaman->ok() )
    {
      proc_chance += p()->spec.restoration_shaman->effectN( 8 ).percent();
    }

    if ( rng().roll( proc_chance ) )
    {
      if ( p()->buff.lava_surge->check() )
        p()->proc.wasted_lava_surge->occur();

      p()->proc.lava_surge->occur();
      if ( !p()->executing || p()->executing->id != 51505 )
        p()->cooldown.lava_burst->reset( true );
      else
      {
        p()->proc.surge_during_lvb->occur();
        p()->lava_surge_during_lvb = true;
      }

      p()->buff.lava_surge->trigger();
    }

    // Fire Elemental passive effect (MS generation on FS tick)
    if ( !p()->talent.storm_elemental->ok() )
    {
      if ( p()->talent.primal_elementalist->ok() && p()->pet.pet_fire_elemental &&
           !p()->pet.pet_fire_elemental->is_sleeping() )
      {
        p()->trigger_maelstrom_gain( elemental_resource->effectN( 1 ).base_value(), p()->gain.fire_elemental );
      }
      else if ( !p()->talent.primal_elementalist->ok() && p()->pet.guardian_fire_elemental &&
                !p()->pet.guardian_fire_elemental->is_sleeping() )
      {
        p()->trigger_maelstrom_gain( elemental_resource->effectN( 1 ).base_value(), p()->gain.fire_elemental );
      }
    }

    if ( d->state->result == RESULT_CRIT && p()->legendary.skybreakers_fiery_demise->ok() )
    {
      p()->cooldown.storm_elemental->adjust( -1.0 * skybreakers_effect->effectN( 1 ).time_value() );
      p()->cooldown.fire_elemental->adjust( -1.0 * skybreakers_effect->effectN( 2 ).time_value() );
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );
    if ( p()->buff.surge_of_power->up() && sim->target_non_sleeping_list.size() > 1 )
    {
      spreader->target = state->target;
      spreader->execute();
    }
    p()->buff.surge_of_power->expire();
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "frost_shock", player, player->find_class_spell( "Frost Shock" ), options_str )

  {
    affected_by_master_of_the_elements = true;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.icefury->value();

    return m;
  }

  void execute() override
  {
    if ( p()->buff.icefury->up() )
    {
      // FIXME: This is currently a tooltip bug in-game and isn't attached as an effect to maelstrom or frost shock, so
      // hardcoding for now. Check spell_id=343725 at a later date.
      maelstrom_gain = 8.0;
    }

    shaman_spell_t::execute();

    // Echoed Frost Shock does not consume Icefury
    if ( !is_echoed_spell() )
    {
      p()->buff.icefury->decrement();
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "wind_shear", player, player->find_class_spell( "Wind Shear" ), options_str )
  {
    may_miss = may_crit   = false;
    ignore_false_positive = true;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->debuffs.casting && !candidate_target->debuffs.casting->check() )
      return false;
    return shaman_spell_t::target_ready( candidate_target );
  }

  void execute() override
  {
    shaman_spell_t::execute();
  }
};

// Ascendance Spell =========================================================

struct ascendance_t : public shaman_spell_t
{
  ascendance_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "ascendance", player, player->talent.ascendance, options_str )
  {
    harmful = false;
    // Periodic effect for Enhancement handled by the buff
    dot_duration = base_tick_time = timespan_t::zero();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->cooldown.strike->reset( false );
    p()->buff.ascendance->trigger();
  }
};

// Stormkeeper Spell ========================================================
struct stormkeeper_t : public shaman_spell_t
{
  stormkeeper_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "stormkeeper", player, player->talent.stormkeeper, options_str )
  {
    may_crit = harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.stormkeeper->trigger( p()->buff.stormkeeper->data().initial_stacks() );
  }
};

// Static Discharge Spell ===================================================

struct static_discharge_t : public shaman_spell_t
{
  static_discharge_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "static_discharge", player, player->talent.static_discharge, options_str )
  {
    affected_by_master_of_the_elements = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.static_discharge->trigger();
  }

  bool ready() override
  {             //TODO: Implement Lightning Shield
    if ( false) //!p()->buff.lightning_shield->up() )
    {
      return false;
    }
    else
    {
      return shaman_spell_t::ready();
    }
  }
};

struct static_discharge_tick_t : public shaman_spell_t
{
  static_discharge_tick_t( shaman_t* player)
    : shaman_spell_t( "static discharge tick", player, player->find_spell( 342244 ) )
  {
    background = true;
  }

  void execute() override
  {
    target_cache.is_valid          = false;
    std::vector<player_t*> targets = target_list();
    auto it                        = std::remove_if( targets.begin(), targets.end(), [ this ]( player_t* target ) {
      return !this->td( target )->dot.flame_shock->is_ticking();
    } );

    targets.erase( it, targets.end() );
    if ( targets.size() == 0 )
    {
      sim->print_debug( "Static Discharge Tick without an active FS" );
      return;
    }
    player_t* target = targets[p()->rng().range(targets.size())];
    this->set_target( target );
    shaman_spell_t::execute();
  }
};

// Echoing Shock Spell ======================================================

struct echoing_shock_t : public shaman_spell_t
{
  echoing_shock_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "echoing_shock", player, player->talent.echoing_shock, options_str )
  {
    // placeholder
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.echoing_shock->trigger();
  }
};

// Healing Surge Spell ======================================================

struct healing_surge_t : public shaman_heal_t
{
  healing_surge_t( shaman_t* player, const std::string& options_str )
    : shaman_heal_t( player, player->find_class_spell( "Healing Surge" ), options_str )
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
  healing_wave_t( shaman_t* player, const std::string& options_str )
    : shaman_heal_t( player, player->find_specialization_spell( "Healing Wave" ), options_str )
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
  greater_healing_wave_t( shaman_t* player, const std::string& options_str )
    : shaman_heal_t( player, player->find_specialization_spell( "Greater Healing Wave" ), options_str )
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
  riptide_t( shaman_t* player, const std::string& options_str )
    : shaman_heal_t( player, player->find_specialization_spell( "Riptide" ), options_str )
  {
    resurgence_gain =
        0.6 * p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }
};

// Chain Heal Spell =========================================================

struct chain_heal_t : public shaman_heal_t
{
  chain_heal_t( shaman_t* player, const std::string& options_str )
    : shaman_heal_t( player, player->find_class_spell( "Chain Heal" ), options_str )
  {
    resurgence_gain =
        0.333 * p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = shaman_heal_t::composite_target_da_multiplier( t );

    if ( td( t )->heal.riptide->is_ticking() )
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

  healing_rain_t( shaman_t* player, const std::string& options_str )
    : shaman_heal_t( player, player->find_specialization_spell( "Healing Rain" ), options_str )
  {
    base_tick_time = data().effectN( 2 ).period();
    dot_duration   = data().duration();
    hasted_ticks   = false;
    tick_action    = new healing_rain_aoe_tick_t( player );
  }
};

// ==========================================================================
// Shaman Totem System
// ==========================================================================

struct shaman_totem_pet_t : public pet_t
{
  // Pulse related functionality
  totem_pulse_action_t* pulse_action;
  event_t* pulse_event;
  timespan_t pulse_amplitude;

  // Summon related functionality
  std::string pet_name;
  pet_t* summon_pet;

  shaman_totem_pet_t( shaman_t* p, const std::string& n )
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
};

struct shaman_totem_t : public shaman_spell_t
{
  shaman_totem_pet_t* totem_pet;
  timespan_t totem_duration;

  shaman_totem_t( const std::string& totem_name, shaman_t* player, const std::string& options_str,
                  const spell_data_t* spell_data )
    : shaman_spell_t( totem_name, player, spell_data, options_str ),
      totem_pet( nullptr ),
      totem_duration( data().duration() )
  {
    harmful = callbacks = may_miss = may_crit = false;
    ignore_false_positive                     = true;
    dot_duration                              = timespan_t::zero();
  }

  void init_finished() override
  {
    totem_pet = debug_cast<shaman_totem_pet_t*>( player->find_pet( name() ) );

    shaman_spell_t::init_finished();
  }

  void execute() override
  {
    shaman_spell_t::execute();
    totem_pet->summon( totem_duration );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    // Redirect active/remains to "pet.<totem name>.active/remains" so things work ok with the
    // pet initialization order shenanigans. Otherwise, at this point in time (when
    // create_expression is called), the pets don't actually exist yet.
    if ( util::str_compare_ci( name, "active" ) )
      return player->create_expression( "pet." + name_str + ".active" );
    else if ( util::str_compare_ci( name, "remains" ) )
      return player->create_expression( "pet." + name_str + ".remains" );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, totem_duration );

    return shaman_spell_t::create_expression( name );
  }

  bool ready() override
  {
    if ( !totem_pet )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

struct totem_pulse_action_t : public spell_t
{
  bool hasted_pulse;
  double pulse_multiplier;
  shaman_totem_pet_t* totem;

  totem_pulse_action_t( const std::string& token, shaman_totem_pet_t* p, const spell_data_t* s )
    : spell_t( token, p, s ), hasted_pulse( false ), pulse_multiplier( 1.0 ), totem( p )
  {
    may_crit = harmful = background = true;
    callbacks                       = false;

    crit_bonus_multiplier *= 1.0 + totem->o()->spec.elemental_fury->effectN( 1 ).percent();
  }

  shaman_t* o() const
  {
    return debug_cast<shaman_t*>( player->cast_pet()->owner );
  }

  shaman_td_t* td( player_t* target ) const
  {
    return o()->get_target_data( target );
  }

  double action_multiplier() const override
  {
    double m = spell_t::action_multiplier();

    m *= pulse_multiplier;

    return m;
  }

  void init() override
  {
    spell_t::init();

    // Hacky, but constructor wont work.
    crit_multiplier *= util::crit_multiplier( totem->o()->meta_gem );
  }

  void reset() override
  {
    spell_t::reset();
    pulse_multiplier = 1.0;
  }
};

struct totem_pulse_event_t : public event_t
{
  shaman_totem_pet_t* totem;
  timespan_t real_amplitude;

  totem_pulse_event_t( shaman_totem_pet_t& t, timespan_t amplitude )
    : event_t( t ), totem( &t ), real_amplitude( amplitude )
  {
    if ( totem->pulse_action->hasted_pulse )
      real_amplitude *= totem->cache.spell_speed();

    schedule( real_amplitude );
  }
  const char* name() const override
  {
    return "totem_pulse";
  }
  void execute() override
  {
    if ( totem->pulse_action )
      totem->pulse_action->execute();

    totem->pulse_event = make_event<totem_pulse_event_t>( sim(), *totem, totem->pulse_amplitude );
  }
};

void shaman_totem_pet_t::summon( timespan_t duration )
{
  pet_t::summon( duration );

  if ( pulse_action )
  {
    pulse_action->pulse_multiplier = 1.0;
    pulse_event                    = make_event<totem_pulse_event_t>( *sim, *this, pulse_amplitude );
  }

  if ( summon_pet )
    summon_pet->summon();
}

void shaman_totem_pet_t::dismiss( bool expired )
{
  // Disable last (partial) tick on dismiss, as it seems not to happen in game atm
  if ( pulse_action && pulse_event && expiration && expiration->remains() == timespan_t::zero() )
  {
    if ( pulse_event->remains() > timespan_t::zero() )
      pulse_action->pulse_multiplier =
          pulse_event->remains() / debug_cast<totem_pulse_event_t*>( pulse_event )->real_amplitude;
    pulse_action->execute();
  }

  event_t::cancel( pulse_event );

  if ( summon_pet )
    summon_pet->dismiss();

  pet_t::dismiss( expired );
}

// Liquid Magma totem =======================================================

struct liquid_magma_globule_t : public spell_t
{
  liquid_magma_globule_t( shaman_totem_pet_t* p ) : spell_t( "liquid_magma", p, p->find_spell( 192231 ) )
  {
    aoe        = -1;
    background = may_crit = true;
    callbacks             = false;

    if ( p->o()->spec.elemental_fury->ok() )
    {
      crit_bonus_multiplier *= 1.0 + p->o()->spec.elemental_fury->effectN( 1 ).percent();
    }
  }
};

struct liquid_magma_totem_pulse_t : public totem_pulse_action_t
{
  liquid_magma_globule_t* globule;

  liquid_magma_totem_pulse_t( shaman_totem_pet_t* totem )
    : totem_pulse_action_t( "liquid_magma_driver", totem, totem->find_spell( 192226 ) ),
      globule( new liquid_magma_globule_t( totem ) )
  {
    // TODO: "Random enemies" implicates number of targets
    aoe          = 1;
    hasted_pulse = quiet = dual = true;
    dot_duration                = timespan_t::zero();
  }

  void impact( action_state_t* state ) override
  {
    totem_pulse_action_t::impact( state );

    globule->set_target( state->target );
    globule->schedule_execute();
  }
};

struct liquid_magma_totem_t : public shaman_totem_pet_t
{
  liquid_magma_totem_t( shaman_t* owner ) : shaman_totem_pet_t( owner, "liquid_magma_totem" )
  {
    pulse_amplitude = owner->find_spell( 192226 )->effectN( 1 ).period();
  }

  void init_spells() override
  {
    shaman_totem_pet_t::init_spells();

    pulse_action = new liquid_magma_totem_pulse_t( this );
  }
};

// Capacitor Totem =========================================================

struct capacitor_totem_pulse_t : public totem_pulse_action_t
{
  cooldown_t* totem_cooldown;

  capacitor_totem_pulse_t( shaman_totem_pet_t* totem )
    : totem_pulse_action_t( "static_charge", totem, totem->find_spell( 118905 ) )
  {
    aoe   = 1;
    quiet = dual   = true;
    totem_cooldown = totem->o()->get_cooldown( "capacitor_totem" );
  }

  void execute() override
  {
    totem_pulse_action_t::execute();
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

struct capacitor_totem_t : public shaman_totem_pet_t
{
  capacitor_totem_t( shaman_t* owner ) : shaman_totem_pet_t( owner, "capacitor_totem" )
  {
    pulse_amplitude = owner->find_spell( 192058 )->duration();
  }

  void init_spells() override
  {
    shaman_totem_pet_t::init_spells();

    pulse_action = new capacitor_totem_pulse_t( this );
  }
};

// ==========================================================================
// PvP talents/abilities
// ==========================================================================

struct lightning_lasso_t : public shaman_spell_t
{
  lightning_lasso_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "lightning_lasso", player, player->find_spell( 305485 ), options_str )
  {
    affected_by_master_of_the_elements = false;
    // if the major effect is not available the action is a background action, thus can't be used in the apl
    background         = true;
    cooldown->duration = p()->find_spell( 305483 )->cooldown();
    channeled          = true;
    tick_may_crit      = true;
    may_crit           = false;
    trigger_gcd        = p()->find_spell( 305483 )->gcd();
  }

  timespan_t tick_time( const action_state_t* /* s */ ) const override
  {
    return base_tick_time;
  }
};

struct thundercharge_t : public shaman_spell_t
{
  thundercharge_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "thundercharge", player, player->find_spell( 204366 ), options_str )
  {
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
  fae_transfusion_tick_t( util::string_view n, shaman_t* player )
    : shaman_spell_t( n, player, player->find_spell( 328928 ) )
  {
    affected_by_master_of_the_elements = false;

    aoe        = 4;
    background = true;
    callbacks  = false;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p()->conduit.essential_extraction->ok() )
    {
      m *= 1.0 + p()->conduit.essential_extraction->effectN( 1 ).percent();
    }

    return m;
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }
};

struct fae_transfusion_t : public shaman_spell_t
{
  fae_transfusion_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "fae_transfusion", player, player->find_covenant_spell( "Fae Transfusion" ), options_str )
  {
    if ( !player->covenant.night_fae->ok() )
      return;

    affected_by_master_of_the_elements = false;

    channeled   = true;
    tick_action = new fae_transfusion_tick_t( "fae_transfusion_tick", player );

    if ( player->conduit.essential_extraction->ok() )
    {
      base_tick_time *= 1.0 + p()->conduit.essential_extraction->effectN( 3 ).percent();
    }
  }
};

// ==========================================================================
// Primordial Wave - Necrolord Covenant
// ==========================================================================
struct primordial_wave_t : public shaman_spell_t
{
  primordial_wave_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "primordial_wave", player, player->covenant.necrolord, options_str )
  {
    if ( !player->covenant.necrolord->ok() )
      return;

    // attack/spell power valujes are on a secondary spell
    attack_power_mod.direct = player->find_spell( 327162 )->effectN( 1 ).ap_coeff();
    spell_power_mod.direct  = player->find_spell( 327162 )->effectN( 1 ).sp_coeff();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.primordial_wave->trigger();

    if ( p()->conduit.tumbling_waves->ok() && rng().roll( p()->conduit.tumbling_waves.percent() ) )
    {
      cooldown->reset( true );
    }
  }
};

// ==========================================================================
// Chain Harvest - Venthyr Covenant
// ==========================================================================
struct chain_harvest_t : public chained_base_t
{
  int critical_hits = 0;

  chain_harvest_t( shaman_t* player, const std::string& options_str )
    : chained_base_t( player, "chain_harvest", player->covenant.venthyr, 0, options_str )
  {
    if ( !player->covenant.venthyr->ok() )
      return;

    aoe                    = 5;
    spell_power_mod.direct = player->find_spell( 320752 )->effectN( 1 ).sp_coeff();
  }

  double composite_crit_chance() const override
  {
    double c = chained_base_t::composite_crit_chance();

    if ( p()->conduit.lavish_harvest->ok() )
    {
      c += p()->conduit.lavish_harvest.percent();
    }

    return c;
  }

  void impact( action_state_t* s ) override
  {
    chained_base_t::impact( s );

    if ( s->result == RESULT_CRIT )
    {
      critical_hits++;
    }
  }

  void update_ready( timespan_t ) override
  {
    auto cd = cooldown_duration();
    cd -= critical_hits * timespan_t::from_seconds( 5 );

    critical_hits = 0;

    chained_base_t::update_ready( cd );
  }

  void execute() override
  {
    // do not consume stormkeeper, as happens in chained_base_t
    shaman_spell_t::execute();
  }
};

// ==========================================================================
// Vesper Totem - Kyrian Covenant
// ==========================================================================

struct vesper_totem_damage_t : public shaman_spell_t
{
  vesper_totem_damage_t( shaman_t* player )
    : shaman_spell_t( "vesper_totem_damage", player, player->find_spell( 324520 ) )
  {
    background = true;
  }
};

struct vesper_totem_t : public shaman_spell_t
{
  vesper_totem_damage_t* damage;

  vesper_totem_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "vesper_totem", player, player->covenant.kyrian, options_str ),
      damage( new vesper_totem_damage_t( player ) )
  {
    add_child( damage );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.vesper_totem->trigger();
    make_event<ground_aoe_event_t>(
        *player->sim, player,
        ground_aoe_params_t()
            .target( execute_state->target )
            .pulse_time( sim->max_time )
            .n_pulses( data().effectN( 2 ).base_value() )
            .action( damage )
            .state_callback( [ this ]( ground_aoe_params_t::state_type event_type, ground_aoe_event_t* ptr ) {
              switch ( event_type )
              {
                case ground_aoe_params_t::state_type::EVENT_CREATED:
                  assert( this->p()->vesper_totem == nullptr );
                  this->p()->vesper_totem = ptr;
                  break;
                case ground_aoe_params_t::state_type::EVENT_DESTRUCTED:
                  assert( this->p()->vesper_totem == ptr );
                  this->p()->vesper_totem = nullptr;
                  break;
                case ground_aoe_params_t::state_type::EVENT_STOPPED:
                  this->p()->buff.vesper_totem->expire();
                  break;
                default:
                  break;
              }
            } ) );
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

struct flametongue_buff_t : public buff_t
{
  shaman_t* p;

  flametongue_buff_t( shaman_t* p )
    : buff_t( p, "flametongue", p->find_class_spell( "Flametongue" )->effectN( 3 ).trigger() ), p( p )
  {
    set_period( timespan_t::zero() );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }

  void execute( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override
  {
    buff_t::execute( stacks, value, duration );
  }
};

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

action_t* shaman_t::create_action( util::string_view name, const std::string& options_str )
{
  // shared
  if ( name == "ascendance" )
    return new ascendance_t( this, options_str );
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "bloodlust" )
    return new bloodlust_t( this, options_str );
  if ( name == "capacitor_totem" )
    return new shaman_totem_t( "capacitor_totem", this, options_str, find_spell( 192058 ) );
  if ( name == "elemental_blast" )
    return new elemental_blast_t( this, options_str );
  if ( name == "flame_shock" )
    return new flame_shock_t( this, options_str );
  if ( name == "frost_shock" )
    return new frost_shock_t( this, options_str );
  if ( name == "ghost_wolf" )
    return new ghost_wolf_t( this, options_str );
  if ( name == "lightning_bolt" )
    return new lightning_bolt_t( this, options_str );
  if ( name == "stormkeeper" )
    return new stormkeeper_t( this, options_str );
  if ( name == "wind_shear" )
    return new wind_shear_t( this, options_str );

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
  if ( name == "chain_lightning" )
    return new chain_lightning_t( this, options_str );
  if ( name == "earth_elemental" )
    return new earth_elemental_t( this, options_str );
  if ( name == "earth_shock" )
    return new earth_shock_t( this, options_str );
  if ( name == "earthquake" )
    return new earthquake_t( this, options_str );
  if ( name == "echoing_shock" )
    return new echoing_shock_t( this, options_str );
  if ( name == "fire_elemental" )
    return new fire_elemental_t( this, options_str );
  if ( name == "icefury" )
    return new icefury_t( this, options_str );
  if ( name == "lava_beam" )
    return new lava_beam_t( this, options_str );
  if ( name == "lava_burst" )
    return new lava_burst_t( this, options_str );
  if ( name == "liquid_magma_totem" )
    return new shaman_totem_t( "liquid_magma_totem", this, options_str, talent.liquid_magma_totem );
  if ( name == "static_discharge" )
    return new static_discharge_t( this, options_str );
  if ( name == "storm_elemental" )
    return new storm_elemental_t( this, options_str );
  if ( name == "thunderstorm" )
    return new thunderstorm_t( this, options_str );
  if ( name == "lightning_lasso" )
    return new lightning_lasso_t( this, options_str );

  // enhancement
  if ( name == "crash_lightning" )
    return new crash_lightning_t( this, options_str );
  if ( name == "earthen_spike" )
    return new earthen_spike_t( this, options_str );
  if ( name == "feral_lunge" )
    return new feral_lunge_t( this, options_str );
  if ( name == "feral_spirit" )
    return new feral_spirit_spell_t( this, options_str );
  if ( name == "flametongue" )
    return new flametongue_t( this, options_str );
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
}

// shaman_t::create_expression ==============================================

std::unique_ptr<expr_t> shaman_t::create_expression( util::string_view name )
{
  std::vector<std::string> splits = util::string_split( name, "." );

  if ( splits.size() >= 3 && util::str_compare_ci( splits[ 0 ], "pet" ) )
  {
    auto require_primal = splits[ 1 ].find( "primal_" ) != std::string::npos;
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
    auto pe        = require_primal || talent.primal_elementalist->ok() == true;
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
      return make_fn_expr( name, [ p ]() { return p->expiration->remains().total_seconds(); } );
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

  return player_t::create_expression( name );
}

// shaman_t::create_actions =================================================

void shaman_t::create_actions()
{
  if ( talent.crashing_storm->ok() )
  {
    action.crashing_storm = new crashing_storm_damage_t( this );
  }

  if ( talent.earthen_rage->ok() )
  {
    action.earthen_rage = new earthen_rage_spell_t( this );
  }

  if ( talent.static_discharge->ok())
  {
    action.static_discharge_tick = new static_discharge_tick_t( this );
  }

  if ( spec.crash_lightning->ok() )
  {
    action.crash_lightning_aoe = new crash_lightning_attack_t( this );
  }

  player_t::create_actions();
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
  spec.chain_lightning_2 = find_specialization_spell( 231722 );
  spec.elemental_fury    = find_specialization_spell( "Elemental Fury" );
  spec.elemental_shaman  = find_specialization_spell( "Elemental Shaman" );
  spec.lava_burst_2      = find_specialization_spell( 231721 );
  spec.lava_surge        = find_specialization_spell( "Lava Surge" );

  // Enhancement
  spec.crash_lightning    = find_specialization_spell( "Crash Lightning" );
  spec.critical_strikes   = find_specialization_spell( "Critical Strikes" );
  spec.dual_wield         = find_specialization_spell( "Dual Wield" );
  spec.enhancement_shaman = find_specialization_spell( "Enhancement Shaman" );
  spec.feral_spirit_2     = find_specialization_spell( 231723 );
  spec.flametongue        = find_specialization_spell( "Flametongue" );
  spec.maelstrom_weapon   = find_specialization_spell( "Maelstrom Weapon" );
  spec.stormbringer       = find_specialization_spell( "Stormbringer" );
  spec.windfury           = find_specialization_spell( "Windfury" );

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

  //
  // Talents
  //
  // Shared
  talent.ascendance      = find_talent_spell( "Ascendance" );
  talent.static_charge   = find_talent_spell( "Static Charge" );
  talent.elemental_blast = find_talent_spell( "Elemental Blast" );
  talent.spirit_wolf     = find_talent_spell( "Spirit Wolf" );
  talent.earth_shield    = find_talent_spell( "Earth Shield" );
  talent.static_charge    = find_talent_spell( "Static Charge" );
  talent.static_discharge = find_talent_spell( "Static Discharge" );
  talent.stormkeeper     = find_talent_spell( "Stormkeeper" );

  // Elemental
  talent.earthen_rage         = find_talent_spell( "Earthen Rage" );
  talent.echo_of_the_elements = find_talent_spell( "Echo of the Elements" );
  talent.echoing_shock        = find_talent_spell( "Echoing Shock" );
  // static discharge

  talent.aftershock = find_talent_spell( "Aftershock" );
  // echoing shock
  // elemental blast

  talent.master_of_the_elements = find_talent_spell( "Master of the Elements" );
  talent.storm_elemental        = find_talent_spell( "Storm Elemental" );
  talent.liquid_magma_totem     = find_talent_spell( "Liquid Magma Totem" );

  // ancestral guidance

  talent.surge_of_power      = find_talent_spell( "Surge of Power" );
  talent.primal_elementalist = find_talent_spell( "Primal Elementalist" );
  talent.icefury             = find_talent_spell( "Icefury" );

  talent.unlimited_power = find_talent_spell( "Unlimited Power" );

  // Enhancement
  // lashing flames
  talent.forceful_winds = find_talent_spell( "Forceful Winds" );
  // elemental blast

  // stormfury
  talent.hot_hand   = find_talent_spell( "Hot Hand" );
  talent.ice_strike = find_talent_spell( "Ice Strike" );

  // cycle of the elements
  talent.hailstorm = find_talent_spell( "Hailstorm" );
  // fire nova

  talent.crashing_storm = find_talent_spell( "Crashing Storm" );
  talent.sundering      = find_talent_spell( "Sundering" );

  talent.elemental_spirits = find_talent_spell( "Elemental Spirits" );
  talent.earthen_spike     = find_talent_spell( "Earthen Spike" );

  // Restoration
  talent.graceful_spirit = find_talent_spell( "Graceful Spirit" );

  // Covenants
  covenant.necrolord = find_covenant_spell( "Primordial Wave" );
  covenant.night_fae = find_covenant_spell( "Fae Transfusion" );
  covenant.venthyr   = find_covenant_spell( "Chain Harvest" );
  covenant.kyrian    = find_covenant_spell( "Vesper Totem" );

  // Covenant-specific conduits
  conduit.essential_extraction = find_conduit_spell( "Essential Extraction" );
  conduit.lavish_harvest       = find_conduit_spell( "Lavish Harvest" );
  conduit.tumbling_waves       = find_conduit_spell( "Tumbling Waves" );

  // Elemental Conduits
  conduit.call_of_flame = find_conduit_spell( "Call of Flame" );
  conduit.high_voltage  = find_conduit_spell( "High Voltage" );
  conduit.pyroclastic_shock = find_conduit_spell( "Pyroclastic Shock" );

  // Shared Legendaries
  legendary.ancestral_reminder     = find_runeforge_legendary( "Ancestral Reminder" );
  legendary.chains_of_devastation  = find_runeforge_legendary( "Chains of Devastation" );
  legendary.deeptremor_stone       = find_runeforge_legendary( "Deeptremor Stone" );
  legendary.deeply_rooted_elements = find_runeforge_legendary( "Deeply Rooted Elements" );

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
  spell.resurgence           = find_spell( 101033 );
  spell.maelstrom_melee_gain = find_spell( 187890 );
  spell.feral_spirit         = find_spell( 228562 );
  spell.fire_elemental       = find_spell( 188592 );
  spell.storm_elemental      = find_spell( 157299 );

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

  if ( specialization() == SHAMAN_ELEMENTAL || specialization() == SHAMAN_ENHANCEMENT )
    resources.base[ RESOURCE_MAELSTROM ] = 100;

  if ( specialization() == SHAMAN_RESTORATION )
  {
    resources.base[ RESOURCE_MANA ]               = 20000;
    resources.initial_multiplier[ RESOURCE_MANA ] = 1.0 + spec.restoration_shaman->effectN( 5 ).percent();
  }

  if ( spec.enhancement_shaman->ok() )
    resources.base[ RESOURCE_MAELSTROM ] += spec.enhancement_shaman->effectN( 6 ).base_value();
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

bool shaman_t::active_elemental_pet() const
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

void shaman_t::summon_feral_spirits( timespan_t duration )
{
  // No elemental spirits selected, just summon normal pets and exit
  if ( !talent.elemental_spirits->ok() )
  {
    pet.spirit_wolves.spawn( duration, 2u );
    return;
  }

  // This summon evaluates the wolf type to spawn as the roll, instead of rolling against
  // the available pool of wolves to spawn.
  size_t n = 2;
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
      pet.pet_storm_elemental->summon( duration );
      pet.pet_storm_elemental->get_cooldown( "eye_of_the_storm" )->reset( false );
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

  // Needs to check off hand imbue
  /*if ( !buff.flametongue->up() )
  {
    return;
  }*/

  buff.hot_hand->trigger();
  attack->proc_hh->occur();
}

void shaman_t::trigger_vesper_totem( const action_state_t* state )
{
  if ( !vesper_totem )
  {
    return;
  }

  if ( state->action->background )
  {
    return;
  }

  if ( state->result_amount == 0 )
  {
    return;
  }

  auto current_event = vesper_totem;

  // Pulse the ground aoe event manually
  current_event->execute();

  // Aand cancel the event .. shaman_t::vesper_totem will have the new event pointer, so
  // we cancel the cached pointer here
  event_t::cancel( current_event );
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

  double proc_chance = spec.windfury->proc_chance();
  proc_chance += cache.mastery() * mastery.enhanced_elements->effectN( 4 ).mastery_value();

  if ( rng().roll( proc_chance ) )
  {
    action_t* a = nullptr;
    if ( !state->action->weapon || state->action->weapon->slot == SLOT_MAIN_HAND )
    {
      a = windfury_mh;
    }
    else
    {
      return;
    }

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

    attack->proc_wf->occur();
  }
}

void shaman_t::trigger_icy_edge( const action_state_t* state )
{
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );

  if ( !attack->may_proc_icy_edge )
    return;

  if ( !buff.icy_edge->up() )
    return;

  if ( buff.ghost_wolf->check() )
  {
    return;
  }

  if ( buff.icy_edge->up() )
  {
    for ( int x = 1; x <= buff.icy_edge->check(); x++ )
    {
      icy_edge->set_target( state->target );
      icy_edge->execute();
    }
  }
}

void shaman_t::trigger_flametongue_weapon( const action_state_t* state )
{
  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr &&
          "Flametongue Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_flametongue )
    return;

  // Check off hand imbue
  /*if ( !buff.flametongue->up() )
    return;*/

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

// shaman_t::init_buffs =====================================================

void shaman_t::create_buffs()
{
  player_t::create_buffs();

  //
  // Shared
  //
  buff.ascendance = new ascendance_buff_t( this );
  buff.ghost_wolf = make_buff( this, "ghost_wolf", find_class_spell( "Ghost Wolf" ) );

  buff.elemental_blast_crit = make_buff<stat_buff_t>( this, "elemental_blast_critical_strike", find_spell( 118522 ) );
  buff.elemental_blast_crit->set_max_stack( 1 );
  buff.elemental_blast_haste = make_buff<stat_buff_t>( this, "elemental_blast_haste", find_spell( 173183 ) );
  buff.elemental_blast_haste->set_max_stack( 1 );
  buff.elemental_blast_mastery = make_buff<stat_buff_t>( this, "elemental_blast_mastery", find_spell( 173184 ) );
  buff.elemental_blast_mastery->set_max_stack( 1 );
  buff.stormkeeper = make_buff( this, "stormkeeper", talent.stormkeeper )
                         ->set_cooldown( timespan_t::zero() );  // Handled by the action

  if ( legendary.ancestral_reminder->ok() )
  {
      
    auto legendary_spell = find_spell( 336741 );
    auto buff            = buffs.bloodlust;
    buff->modify_duration( timespan_t::from_millis(legendary_spell->effectN( 1 ).base_value()) );
    buff->modify_default_value( legendary_spell->effectN( 2 ).percent() );
  }

  // Covenants
  buff.primordial_wave =
      make_buff( this, "primordial_wave", covenant.necrolord )->set_duration( find_spell( 327164 )->duration() );
  buff.vesper_totem = make_buff( this, "vesper_totem", covenant.kyrian )
                          ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                            if ( new_ == 0 )
                            {
                              event_t::cancel( vesper_totem );
                            }
                          } );

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

  buff.earthen_rage = make_buff( this, "earthen_rage", find_spell( 170377 ) )
                          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                          ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
                          ->set_tick_behavior( buff_tick_behavior::REFRESH )
                          ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                            assert( action.earthen_rage );
                            action.earthen_rage->set_target( recent_target );
                            action.earthen_rage->execute();
                          } );

  buff.static_discharge = make_buff( this, "static_discharge", talent.static_discharge )
                              ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
                              ->set_tick_behavior( buff_tick_behavior::REFRESH )
                              ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                                assert( action.static_discharge_tick );
                                action.static_discharge_tick->execute();
                              } );

  buff.master_of_the_elements = make_buff( this, "master_of_the_elements", find_spell( 260734 ) )
                                    ->set_default_value( find_spell( 260734 )->effectN( 1 ).percent() );
  buff.unlimited_power = make_buff( this, "unlimited_power", find_spell( 272737 ) )
                             ->add_invalidate( CACHE_HASTE )
                             ->set_default_value( find_spell( 272737 )->effectN( 1 ).percent() )
                             ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buff.wind_gust = make_buff( this, "wind_gust", find_spell( 263806 ) )
                       ->set_default_value( find_spell( 263806 )->effectN( 1 ).percent() );

  buff.echoes_of_great_sundering = make_buff( this, "echoes_of_great_sundering", find_spell( 336217 ) )
                                       ->set_default_value( find_spell( 336217 )->effectN( 2 ).percent() );

  buff.chains_of_devastation_chain_heal = make_buff( this, "chains_of_devastation_chain_heal", find_spell( 329772 ) );
  buff.chains_of_devastation_chain_lightning =
      make_buff( this, "chains_of_devastation_chain_lightning", find_spell( 329771 ) );

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

  buff.echoing_shock = make_buff( this, "echoing_shock", talent.echoing_shock );

  //
  // Enhancement
  //

  buff.lightning_shield = new lightning_shield_buff_t( this );
  buff.forceful_winds   = make_buff<buff_t>( this, "forceful_winds", find_spell( 262652 ) )
                            ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
                            ->set_default_value( find_spell( 262652 )->effectN( 1 ).percent() );

  buff.icy_edge         = new icy_edge_buff_t( this );
  buff.molten_weapon    = new molten_weapon_buff_t( this );
  buff.crackling_surge  = new crackling_surge_buff_t( this );
  buff.gathering_storms = new gathering_storms_buff_t( this );

  buff.crash_lightning = make_buff( this, "crash_lightning", find_spell( 187878 ) );
  buff.hot_hand =
      make_buff( this, "hot_hand", talent.hot_hand->effectN( 1 ).trigger() )->set_trigger_spell( talent.hot_hand );
  buff.spirit_walk  = make_buff( this, "spirit_walk", find_specialization_spell( "Spirit Walk" ) );
  buff.stormbringer = make_buff( this, "stormbringer", find_spell( 201846 ) )
                          ->set_activated( false )
                          ->set_max_stack( find_spell( 201846 )->initial_stacks() );

  //
  // Restoration
  //
  buff.spiritwalkers_grace =
      make_buff( this, "spiritwalkers_grace", find_specialization_spell( "Spiritwalker's Grace" ) )
          ->set_cooldown( timespan_t::zero() );
  buff.tidal_waves =
      make_buff( this, "tidal_waves", spec.tidal_waves->ok() ? find_spell( 53390 ) : spell_data_t::not_found() );
}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gain.aftershock              = get_gain( "Aftershock" );
  gain.high_voltage            = get_gain( "High Voltage" );
  gain.ascendance              = get_gain( "Ascendance" );
  gain.resurgence              = get_gain( "resurgence" );
  gain.feral_spirit            = get_gain( "Feral Spirit" );
  gain.fire_elemental          = get_gain( "Fire Elemental" );
  gain.spirit_of_the_maelstrom = get_gain( "Spirit of the Maelstrom" );
  gain.forceful_winds          = get_gain( "Forceful Winds" );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.lava_surge        = get_proc( "Lava Surge" );
  proc.wasted_lava_surge = get_proc( "Lava Surge: Wasted" );
  proc.windfury          = get_proc( "Windfury" );
  proc.surge_during_lvb  = get_proc( "Lava Surge: During Lava Burst" );
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  player_t::init_rng();
}

// shaman_t::init_special_effects ===========================================

void shaman_t::init_special_effects()
{
  player_t::init_special_effects();
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
  std::string elemental_pot =
      ( true_level > 110 )
          ? "potion_of_unbridled_fury"
          : ( true_level > 100 )
                ? "prolonged_power"
                : ( true_level >= 90 )
                      ? "draenic_intellect"
                      : ( true_level >= 85 ) ? "jade_serpent" : ( true_level >= 80 ) ? "volcanic" : "disabled";

  std::string enhance_pot =
      ( true_level > 110 )
          ? "potion_of_unbridled_fury"
          : ( true_level > 100 )
                ? "prolonged_power"
                : ( true_level >= 90 )
                      ? "draenic_agility"
                      : ( true_level >= 85 ) ? "virmens_bite" : ( true_level >= 80 ) ? "tolvir" : "disabled";

  return specialization() == SHAMAN_ENHANCEMENT ? enhance_pot : elemental_pot;
}

// shaman_t::default_flask ==================================================

std::string shaman_t::default_flask() const
{
  std::string elemental_flask =
      ( true_level > 110 )
          ? "greater_flask_of_endless_fathoms"
          : ( true_level > 100 )
                ? "whispered_pact"
                : ( true_level >= 90 )
                      ? "greater_draenic_intellect_flask"
                      : ( true_level >= 85 ) ? "warm_sun" : ( true_level >= 80 ) ? "draconic_mind" : "disabled";

  std::string enhance_flask =
      ( true_level > 110 )
          ? "greater_flask_of_the_currents"
          : ( true_level > 100 )
                ? "seventh_demon"
                : ( true_level >= 90 )
                      ? "greater_draenic_agility_flask"
                      : ( true_level >= 85 ) ? "spring_blossoms" : ( true_level >= 80 ) ? "winds" : "disabled";

  return specialization() == SHAMAN_ENHANCEMENT ? enhance_flask : elemental_flask;
}

// shaman_t::default_food ===================================================

std::string shaman_t::default_food() const
{
  std::string elemental_food = ( true_level > 110 )
                                   ? "mechdowels_big_mech"
                                   : ( true_level > 100 )
                                         ? "lemon_herb_filet"
                                         : ( true_level > 90 )
                                               ? "pickled_eel"
                                               : ( true_level >= 90 )
                                                     ? "mogu_fish_stew"
                                                     : ( true_level >= 80 ) ? "seafood_magnifique_feast" : "disabled";

  std::string enhance_food = ( true_level > 110 )
                                 ? "baked_port_tato"
                                 : ( true_level > 100 )
                                       ? "lemon_herb_filet"
                                       : ( true_level > 90 )
                                             ? "buttered_sturgeon"
                                             : ( true_level >= 90 )
                                                   ? "sea_mist_rice_noodles"
                                                   : ( true_level >= 80 ) ? "seafood_magnifique_feast" : "disabled";

  std::string restoration_food = ( true_level > 110 )
                                     ? "baked_port_tato"
                                     : ( true_level > 100 )
                                           ? "lemon_herb_filet"
                                           : ( true_level > 90 )
                                                 ? "pickled_eel"
                                                 : ( true_level >= 90 )
                                                       ? "mogu_fish_stew"
                                                       : ( true_level >= 80 ) ? "seafood_magnifique_feast" : "disabled";

  return specialization() == SHAMAN_ENHANCEMENT
             ? enhance_food
             : specialization() == SHAMAN_ELEMENTAL ? elemental_food : restoration_food;
}

// shaman_t::default_rune ===================================================

std::string shaman_t::default_rune() const
{
  std::string elemental_rune = ( true_level >= 120 )
                                   ? "battle_scarred"
                                   : ( true_level >= 110 ) ? "defiled" : ( true_level >= 100 ) ? "focus" : "disabled";

  std::string enhance_rune = ( true_level >= 120 )
                                 ? "battle_scarred"
                                 : ( true_level >= 110 ) ? "defiled" : ( true_level >= 100 ) ? "hyper" : "disabled";

  return specialization() == SHAMAN_ENHANCEMENT ? enhance_rune : elemental_rune;
}

// shaman_t::init_action_list_elemental =====================================

void shaman_t::init_action_list_elemental()
{
  action_priority_list_t* precombat     = get_action_priority_list( "precombat" );
  action_priority_list_t* def           = get_action_priority_list( "default" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  if ( options.rotation == ROTATION_STANDARD ) {
    def->add_action( this, "Bloodlust" );
    def->add_action( this, "Lightning Bolt" );
  } else if (options.rotation == ROTATION_SIMPLE) {
    action_priority_list_t* single_target = get_action_priority_list( "single_target" );
    action_priority_list_t* aoe           = get_action_priority_list( "aoe" );
    // "Default" APL controlling logic flow to specialized sub-APLs
    def->add_action( this, "Wind Shear", "", "Interrupt of casts." );
    def->add_action( "use_items" );
    def->add_action( this, "Flame Shock", "if=!ticking" );
    def->add_action( this, "Fire Elemental" );
    def->add_talent( this, "Storm Elemental" );
    // Racials
    def->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
    def->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
    def->add_action( "bag_of_tricks,if=!talent.ascendance.enabled|!buff.ascendance.up" );

    // Covenants
    def->add_action( "primordial_wave,if=covenant.necrolord" );
    def->add_action( "vesper_totem,if=covenant.kyrian" );
    def->add_action( "chain_harvest,if=covenant.venthyr" );
    def->add_action( "fae_transfusion,if=covenant.night_fae" );

    // Pick APL to run
    def->add_action(
        "run_action_list,name=aoe,if=active_enemies>2&(spell_targets.chain_lightning>2|spell_targets.lava_beam>2)" );
    def->add_action( "run_action_list,name=single_target,if=active_enemies<=2" );

    // Aoe APL
    aoe->add_talent( this, "Stormkeeper", "if=talent.stormkeeper.enabled" );
    aoe->add_action( this, "Flame Shock", "target_if=refreshable" );
    aoe->add_talent( this, "Liquid Magma Totem", "if=talent.liquid_magma_totem.enabled" );
    aoe->add_action( this, "Lava Burst", "if=talent.master_of_the_elements.enabled&maelstrom>=50&buff.lava_surge.up" );
    aoe->add_talent( this, "Echoing Shock", "if=talent.echoing_shock.enabled" );
    aoe->add_action( this, "Earthquake" );
    aoe->add_action( this, "Chain Lightning" );
    aoe->add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
    aoe->add_action( this, "Frost Shock", "moving=1" );

    // Single target APL
    single_target->add_action( this, "Flame Shock", "target_if=refreshable" );
    single_target->add_talent( this, "Elemental Blast", "if=talent.elemental_blast.enabled" );
    single_target->add_talent( this, "Stormkeeper", "if=talent.stormkeeper.enabled" );
    single_target->add_talent( this, "Liquid Magma Totem", "if=talent.liquid_magma_totem.enabled" );
    single_target->add_talent( this, "Echoing Shock", "if=talent.echoing_shock.enabled" );
    single_target->add_talent( this, "Ascendance", "if=talent.ascendance.enabled" );
    single_target->add_action( this, "Lava Burst", "if=cooldown_react" );
    single_target->add_action( this, "Lava Burst", "if=cooldown_react" );
    single_target->add_action( this, "Earthquake", "if=(spell_targets.chain_lightning>1&!runeforge.echoes_of_great_sundering.equipped|buff.echoes_of_great_sundering.up)" );
    single_target->add_action( this, "Earth Shock" );
    single_target->add_action( "lightning_lasso" );
    single_target->add_action( this, "Frost Shock", "if=talent.icefury.enabled&buff.icefury.up" );
    single_target->add_talent( this, "Icefury", "if=talent.icefury.enabled" );
    single_target->add_action( this, "Lightning Bolt" );
    single_target->add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
    single_target->add_action( this, "Flame Shock", "moving=1,if=movement.distance>6" );
    single_target->add_action( this, "Frost Shock", "moving=1", "Frost Shock is our movement filler." );
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
  // action_priority_list_t* cds              = get_action_priority_list( "cds" );

  // Flask
  // precombat->add_action( "flask" );
  // Food
  // precombat->add_action( "food" );
  // Rune
  // precombat->add_action( "augmentation" );
  // Snapshot stats
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  // Precombat potion
  // precombat->add_action( "potion" );
  // Lightning shield can be turned on pre-combat
  precombat->add_talent( this, "Lightning Shield" );
  // Use precombat time to channel buff trinket

  // All Shamans Bloodlust and Wind Shear by default
  def->add_action( this, "Wind Shear" );
  // Turn on auto-attack first thing
  def->add_action( "auto_attack" );
  def->add_action( "windstrike" );
  def->add_action( this, "Feral Spirit" );
  def->add_action( this, "Earth Elemental" );
  def->add_action( this, "Ascendance" );
  def->add_action( this, "Stormkeeper" );
  def->add_action( this, "Sundering" );
  def->add_action( this, "Earthen Spike" );
  def->add_action( this, "Elemental Blast" );
  def->add_action( this, "Lava Lash" );
  def->add_action( this, "Stormstrike" );
  def->add_action( this, "Crash Lightning" );
  def->add_action( this, "Flame Shock" );
  def->add_action( this, "Frost Shock" );
  def->add_action( this, "Lightning Bolt" );
  def->add_action( this, "Chain Lightning" );
  def->add_action( this, "Totem Mastery" );

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
  // Snapshot stats
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  // Actual precombat
  precombat->add_action( "potion" );
  precombat->add_action( "use_item,name=azsharas_font_of_power" );
  precombat->add_action( this, "Lava Burst" );

  // In-combat potion
  def->add_action( "potion" );

  // "Default"
  def->add_action( this, "Wind Shear" );
  def->add_action( this, "Spiritwalker's Grace", "moving=1,if=movement.distance>6" );
  // On-use items
  def->add_action( this, "Flame Shock", "target_if=(!ticking|dot.flame_shock.remains<=gcd)|refreshable" );
  def->add_action( "use_items" );
  def->add_action( "blood_fury" );
  def->add_action( "berserking" );
  def->add_action( "fireblood" );
  def->add_action( "ancestral_call" );
  def->add_action( "worldvein_resonance" );
  def->add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time&cooldown_react" );
  def->add_action( "concentrated_flame,if=dot.concentrated_flame_burn.remains=0" );
  def->add_action( "ripple_in_space" );
  def->add_action( this, "Earth Elemental" );
  def->add_action( "bag_of_tricks" );
  def->add_action( "fae_transfusion" );
  def->add_action( this, "Lightning Bolt", "if=spell_targets.chain_lightning<2" );
  def->add_action( this, "Chain Lightning", "if=spell_targets.chain_lightning>1" );
  def->add_action( this, "Flame Shock", "moving=1" );
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
  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    windfury_mh = new windfury_attack_t( "windfury_attack", this, find_spell( 25504 ), &( main_hand_weapon ) );
    if ( off_hand_weapon.type != WEAPON_NONE )
    {
      windfury_oh = new windfury_attack_t( "windfury_attack_oh", this, find_spell( 33750 ), &( off_hand_weapon ) );
    }
    flametongue = new flametongue_weapon_spell_t( "flametongue_attack", this, &( off_hand_weapon ) );

    icy_edge = new icy_edge_attack_t( "icy_edge", this, &( main_hand_weapon ) );

    action.molten_weapon_dot = new molten_weapon_dot_t( this );
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

  if ( buff.unlimited_power->up() )
    h *= 1.0 / ( 1.0 + buff.unlimited_power->stack_value() );

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
      // m *= 1.0 + buff.landslide->stack_value();
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

  return m;
}

// shaman_t::composite_player_target_multiplier ==============================

double shaman_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  shaman_td_t* td = get_target_data( target );

  if ( td->debuff.earthen_spike->up() &&
       ( dbc::is_school( school, SCHOOL_PHYSICAL ) || dbc::is_school( school, SCHOOL_NATURE ) ) )
  {
    m *= td->debuff.earthen_spike->check_value();
  }

  return m;
}

// shaman_t::composite_player_pet_damage_multiplier =========================

double shaman_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  // Elemental
  m *= 1.0 + spec.elemental_shaman->effectN( 3 ).percent();

  // Enhancement
  m *= 1.0 + spec.enhancement_shaman->effectN( 3 ).percent();

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

// shaman_t::arise() ========================================================

void shaman_t::arise()
{
  player_t::arise();

  assert( main_hand_attack == melee_mh && off_hand_attack == melee_oh );

  if ( main_hand_weapon.type != WEAPON_NONE )
    main_hand_weapon.buff_type = WINDFURY_IMBUE;

  if ( off_hand_weapon.type != WEAPON_NONE )
    off_hand_weapon.buff_type = FLAMETONGUE_IMBUE;
}

// shaman_t::combat_begin ====================================================

void shaman_t::combat_begin()
{
  player_t::combat_begin();
}

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  player_t::reset();

  lava_surge_during_lvb = false;
  for ( auto& elem : counters )
    elem->reset();

  vesper_totem = nullptr;
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

// echoing_shock_event_t::execute ===========================================

inline void echoing_shock_event_t::execute()
{
  auto spell = static_cast<shaman_spell_t*>( action );

  if ( target->is_sleeping() )
  {
    return;
  }

  // Tweak echoed spell so that it looks like a background cast. During the execute,
  // shaman_spell_t::is_echoed_spell() will return true.
  auto orig_stats = spell->stats;
  auto orig_target = spell->target;
  auto orig_tte = spell->time_to_execute;

  spell->background = true;
  spell->stats = spell->echoing_shock_stats;
  spell->time_to_execute = 0_s;
  spell->exec_type = execute_type::ECHOING_SHOCK;
  spell->set_target( target );
  spell->execute();

  spell->background = false;
  spell->stats = orig_stats;
  spell->exec_type = execute_type::NORMAL;
  spell->time_to_execute = orig_tte;
  spell->set_target( orig_target );
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

      std::string row_class_str = "";
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
    if ( p.cd_waste_exec.size() > 0 )
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
    p->buffs.bloodlust =
        make_buff( p, "bloodlust", p->find_spell( 2825 ) )->set_max_stack( 1 )->add_invalidate( CACHE_HASTE );

    p->buffs.exhaustion = make_buff( p, "exhaustion", p->find_spell( 57723 ) )->set_max_stack( 1 )->set_quiet( true );
  }

  void static_init() const override
  {
  }

  void register_hotfixes() const override
  {
  }

  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
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
    lightning_wolves( "lightning_wolf", s, []( shaman_t* s ) { return new pet::lightning_wolf_t( s ); } )
{
}

}  // namespace

const module_t* module_t::shaman()
{
  static shaman_module_t m;
  return &m;
}

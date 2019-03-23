// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Shaman
// ==========================================================================

// Battle for Azeroth TODO
//
// Shared
// - Further seperate Totem Mastery buffs between the 2 specs
//
// Elemental
//
// + implemented
// - missing
//      Name                        ID
//    + Ancestral Resonance         277666
//    + Echo of the Elementals      275381
//    + Igneous Potential           279829
//    + Lava Shock                  273448
//    + Natural Harmony             278697
//    + Synapse Shock               277671
//    + Tectonic Thunder            286949
//
// Enhancement
// Nothing at the moment.
//
// Legion TODO
//
// Remove any remaining vestiges of legion stuff
//
// CYA LEGION

namespace
{  // UNNAMED NAMESPACE

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

enum imbue_e
{
  IMBUE_NONE = 0,
  FLAMETONGUE_IMBUE,
  WINDFURY_IMBUE,
  FROSTBRAND_IMBUE,
  EARTHLIVING_IMBUE
};

struct shaman_attack_t;
struct shaman_spell_t;
struct shaman_heal_t;
struct shaman_totem_pet_t;
struct totem_pulse_event_t;
struct totem_pulse_action_t;
struct stormlash_buff_t;

struct shaman_td_t : public actor_target_data_t
{
  struct dots
  {
    dot_t* flame_shock;
    dot_t* searing_assault;
    dot_t* molten_weapon;
  } dot;

  struct debuffs
  {
    // Elemental

    // Enhancement
    buff_t* earthen_spike;
    buff_t* lightning_conduit;
    buff_t* primal_primer;
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
  player_t* recent_target =
      nullptr;  // required for Earthen Rage, whichs' ticks damage the most recently attacked target

  // Options
  unsigned stormlash_targets;
  bool raptor_glyph;

  // Data collection for cooldown waste
  auto_dispose<std::vector<data_t*> > cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*> > cd_waste_iter;

  // Cached actions
  struct actions_t
  {
    action_t* ancestral_awakening;
    spell_t* lightning_shield;
    spell_t* earthen_rage;
    spell_t* crashing_storm;
    spell_t* searing_assault;
    spell_t* molten_weapon;
    action_t* molten_weapon_dot;
    action_t* fury_of_air;

    // Azerite
    spell_t* lightning_conduit;
    spell_t* strength_of_earth;

  } action;

  // Pets
  struct pets_t
  {
    pet_t* pet_fire_elemental;
    pet_t* pet_earth_elemental;
    pet_t* pet_storm_elemental;

    std::array<pet_t*, 2> guardian_fire_elemental;
    pet_t* guardian_ember_elemental;
    std::array<pet_t*, 2> guardian_storm_elemental;
    pet_t* guardian_spark_elemental;
    pet_t* guardian_earth_elemental;

    std::array<pet_t*, 2> spirit_wolves;
    std::array<pet_t*, 6> elemental_wolves;
  } pet;

  // Constants
  struct
  {
    double matching_gear_multiplier;
    double haste_ancestral_swiftness;
  } constant;

  // Buffs
  struct
  {
    // shared between all three specs
    buff_t* ascendance;
    buff_t* ghost_wolf;

    buff_t* synapse_shock;

    // Elemental, Restoration
    buff_t* lava_surge;

    // Elemental
    buff_t* earthen_rage;
    buff_t* master_of_the_elements;
    buff_t* surge_of_power;
    buff_t* icefury;
    buff_t* unlimited_power;
    buff_t* stormkeeper;
    stat_buff_t* elemental_blast_crit;
    stat_buff_t* elemental_blast_haste;
    stat_buff_t* elemental_blast_mastery;
    buff_t* wind_gust;  // Storm Elemental passive 263806

    // Enhancement
    buff_t* crash_lightning;
    buff_t* feral_spirit;
    buff_t* flametongue;
    buff_t* frostbrand;
    buff_t* fury_of_air;
    buff_t* hot_hand;
    buff_t* lightning_shield;
    buff_t* lightning_shield_overcharge;
    buff_t* stormbringer;
    buff_t* forceful_winds;
    buff_t* landslide;
    buff_t* icy_edge;
    buff_t* molten_weapon;
    buff_t* crackling_surge;
    buff_t* gathering_storms;

    // Restoration
    buff_t* spirit_walk;
    buff_t* spiritwalkers_grace;
    buff_t* tidal_waves;

    // Set bonuses
    buff_t* t21_2pc_elemental;
    buff_t* t21_2pc_enhancement;

    // Totem Mastery
    buff_t* resonance_totem;
    buff_t* storm_totem;
    buff_t* ember_totem;
    buff_t* tailwind_totem_ele;
    buff_t* tailwind_totem_enh;

    // Azerite traits
    buff_t* ancestral_resonance;
    buff_t* lava_shock;
    stat_buff_t* natural_harmony_fire;    // crit
    stat_buff_t* natural_harmony_frost;   // mastery
    stat_buff_t* natural_harmony_nature;  // haste
    buff_t* roiling_storm_buff_driver;
    buff_t* strength_of_earth;
    buff_t* tectonic_thunder;
    buff_t* thunderaans_fury;
  } buff;

  // Cooldowns
  struct
  {
    cooldown_t* ascendance;
    cooldown_t* fire_elemental;
    cooldown_t* feral_spirits;
    cooldown_t* lava_burst;
    cooldown_t* storm_elemental;
    cooldown_t* strike;  // shared CD of Storm Strike and Windstrike
    cooldown_t* rockbiter;
    cooldown_t* t20_2pc_elemental;
  } cooldown;

  // Gains
  struct
  {
    gain_t* aftershock;
    gain_t* ascendance;
    gain_t* resurgence;
    gain_t* feral_spirit;
    gain_t* fire_elemental;
    gain_t* fury_of_air;
    gain_t* spirit_of_the_maelstrom;
    gain_t* resonance_totem;
    gain_t* forceful_winds;
    gain_t* lightning_shield_overcharge;

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
    proc_t* landslide;
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
    const spell_data_t* critical_strikes;
    const spell_data_t* dual_wield;
    const spell_data_t* enhancement_shaman;
    const spell_data_t* feral_spirit_2;  // 7.1 Feral Spirit Maelstrom gain passive
    const spell_data_t* flametongue;
    const spell_data_t* frostbrand;
    const spell_data_t* maelstrom_weapon;
    const spell_data_t* stormbringer;
    const spell_data_t* stormlash;
    const spell_data_t* windfury;

    // Restoration
    const spell_data_t* ancestral_awakening;
    const spell_data_t* ancestral_focus;
    const spell_data_t* purification;
    const spell_data_t* resurgence;
    const spell_data_t* riptide;
    const spell_data_t* tidal_waves;
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
    const spell_data_t* ascendance;
    const spell_data_t* totem_mastery;
    const spell_data_t* static_charge;

    // Elemental
    const spell_data_t* earthen_rage;
    const spell_data_t* echo_of_the_elements;
    const spell_data_t* elemental_blast;

    const spell_data_t* aftershock;
    const spell_data_t* call_the_thunder;

    const spell_data_t* master_of_the_elements;
    const spell_data_t* storm_elemental;
    const spell_data_t* liquid_magma_totem;

    const spell_data_t* surge_of_power;
    const spell_data_t* primal_elementalist;
    const spell_data_t* icefury;

    const spell_data_t* unlimited_power;
    const spell_data_t* stormkeeper;

    // Enhancement
    const spell_data_t* boulderfist;
    const spell_data_t* hot_hand;
    const spell_data_t* lightning_shield;

    const spell_data_t* landslide;
    const spell_data_t* forceful_winds;

    const spell_data_t* spirit_wolf;
    const spell_data_t* earth_shield;
    // const spell_data_t* static_charge;

    const spell_data_t* searing_assault;
    const spell_data_t* hailstorm;
    const spell_data_t* overcharge;

    // const spell_data_t* natures_guardian;
    const spell_data_t* feral_lunge;
    // const spell_data_t* wind_rush_totem;

    const spell_data_t* crashing_storm;
    const spell_data_t* fury_of_air;
    const spell_data_t* sundering;

    const spell_data_t* elemental_spirits;
    const spell_data_t* earthen_spike;
    // const spell_data_t* ascendance;

    // Restoration
    const spell_data_t* gust_of_wind;
  } talent;

  // Artifact
  struct artifact_spell_data_t
  {
  } artifact;

  // Azerite traits
  struct
  {
    // Elemental
    azerite_power_t echo_of_the_elementals;
    azerite_power_t igneous_potential;
    azerite_power_t lava_shock;
    azerite_power_t tectonic_thunder;

    // Enhancement
    // azerite_power_t electropotence;  // hasn't been found in game yet, so current un-implemented in simc.
    // azerite_power_t storms_eye;      // hasn't been found in game yet, so current un-implemented in simc.
    // azerite_power_t strikers_grace;  // hasn't been found in game yet, so current un-implemented in simc.
    azerite_power_t lightning_conduit;
    azerite_power_t primal_primer;
    azerite_power_t roiling_storm;
    azerite_power_t strength_of_earth;
    azerite_power_t thunderaans_fury;

    // shared
    azerite_power_t ancestral_resonance;
    azerite_power_t natural_harmony;
    azerite_power_t synapse_shock;

  } azerite;

  // Misc Spells
  struct
  {
    const spell_data_t* resurgence;
    const spell_data_t* maelstrom_melee_gain;
  } spell;

  struct legendary_t
  {
  } legendary;

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

  // Azerite Effects
  shaman_spell_t* lightning_conduit;
  shaman_spell_t* strength_of_earth;

  shaman_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN )
    : player_t( sim, SHAMAN, name, r ),
      lava_surge_during_lvb( false ),
      stormlash_targets( 17 ),  // Default to 2 tanks + 15 dps
      raptor_glyph( false ),
      action(),
      pet(),
      constant(),
      buff(),
      cooldown(),
      gain(),
      proc(),
      spec(),
      mastery(),
      talent(),
      spell(),
      legendary()
  {
    range::fill( pet.spirit_wolves, nullptr );
    range::fill( pet.elemental_wolves, nullptr );

    // Cooldowns
    cooldown.ascendance        = get_cooldown( "ascendance" );
    cooldown.fire_elemental    = get_cooldown( "fire_elemental" );
    cooldown.storm_elemental   = get_cooldown( "storm_elemental" );
    cooldown.feral_spirits     = get_cooldown( "feral_spirit" );
    cooldown.lava_burst        = get_cooldown( "lava_burst" );
    cooldown.strike            = get_cooldown( "strike" );
    cooldown.rockbiter         = get_cooldown( "rockbiter" );
    cooldown.t20_2pc_elemental = get_cooldown( "t20_2pc_elemental" );

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

    // Azerite Effects
    lightning_conduit = nullptr;

    regen_type = REGEN_DISABLED;
  }

  virtual ~shaman_t();

  // Misc
  bool active_elemental_pet() const;

  // triggers
  void trigger_windfury_weapon( const action_state_t* );
  void trigger_searing_assault( const action_state_t* state );
  void trigger_flametongue_weapon( const action_state_t* );
  void trigger_icy_edge( const action_state_t* );
  void trigger_hailstorm( const action_state_t* );
  void trigger_stormbringer( const action_state_t* state, double proc_chance = -1.0, proc_t* proc_obj = nullptr );
  void trigger_lightning_shield( const action_state_t* state );
  void trigger_hot_hand( const action_state_t* state );
  void trigger_natural_harmony( const action_state_t* );
  void trigger_strength_of_earth( const action_state_t* );
  void trigger_primal_primer( const action_state_t* );
  void trigger_ancestral_resonance( const action_state_t* );

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

  // APL releated methods
  void init_action_list() override;
  void init_action_list_enhancement();
  void init_action_list_elemental();
  std::string generate_bloodlust_options();
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  void init_rng() override;
  void init_special_effects() override;

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
  double matching_gear_multiplier( attribute_e attr ) const override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  pet_t* create_pet( const std::string& name, const std::string& type = std::string() ) override;
  void create_pets() override;
  expr_t* create_expression( const std::string& name ) override;
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

// buff.tailwind_totem = make_buff<haste_buff_t>(this, "tailwind_totem", find_spell(210659));

// buff.tailwind_totem->add_invalidate(CACHE_HASTE)
//->set_duration(talent.totem_mastery->effectN(4).trigger()->duration())
//->set_default_value(1.0 / (1.0 + find_spell(210659)->effectN(1).percent()));

struct resonance_totem_buff_t : public buff_t
{
  resonance_totem_buff_t( shaman_t* p )
    : buff_t( p, "resonance_totem",
              p->specialization() == SHAMAN_ENHANCEMENT ? p->find_spell( 262417 ) : p->find_spell( 202192 ) )
  {
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    set_duration( p->talent.totem_mastery->effectN( 1 ).trigger()->duration() );
    set_period( s_data->effectN( 1 ).period() );

    set_tick_callback( [p]( buff_t* b, int, const timespan_t& ) {
      double g = b->data().effectN( 1 ).base_value();
      p->resource_gain( RESOURCE_MAELSTROM, g, p->gain.resonance_totem );
    } );
  }
};

struct storm_totem_buff_t : public buff_t
{
  storm_totem_buff_t( shaman_t* p )
    : buff_t( p, "storm_totem",
              p->specialization() == SHAMAN_ENHANCEMENT ? p->find_spell( 262397 ) : p->find_spell( 210652 ) )
  {
    set_duration( p->talent.totem_mastery->effectN( 2 ).trigger()->duration() );
    set_cooldown( timespan_t::zero() );
    set_default_value( s_data->effectN( 1 ).percent() );
  }
};

struct ember_totem_buff_t : public buff_t
{
  ember_totem_buff_t( shaman_t* p )
    : buff_t( p, "ember_totem",
              p->specialization() == SHAMAN_ENHANCEMENT ? p->find_spell( 262399 ) : p->find_spell( 210658 ) )
  {
    set_duration( p->talent.totem_mastery->effectN( 3 ).trigger()->duration() );
    set_default_value( 1.0 + s_data->effectN( 1 ).percent() );
  }
};

struct tailwind_totem_buff_ele_t : public buff_t
{
  tailwind_totem_buff_ele_t( shaman_t* p ) : buff_t( p, "tailwind_totem", p->find_spell( 210659 ) )
  {
    add_invalidate( CACHE_HASTE );
    set_duration( p->talent.totem_mastery->effectN( 4 ).trigger()->duration() );
    set_default_value( s_data->effectN( 1 ).percent() );
  }
};

struct tailwind_totem_buff_enh_t : public buff_t
{
  tailwind_totem_buff_enh_t( shaman_t* p ) : buff_t( p, "tailwind_totem", p->find_spell( 262400 ) )
  {
    set_duration( p->talent.totem_mastery->effectN( 4 ).trigger()->duration() );
    set_default_value( s_data->effectN( 1 ).percent() );
  }
};

struct roiling_storm_buff_driver_t : public buff_t
{
  roiling_storm_buff_driver_t( shaman_t* p ) : buff_t( p, "roiling_storm_driver", p->find_spell( 279513 ) )
  {
    set_period( s_data->internal_cooldown() );
    set_quiet( true );

    if ( p->azerite.roiling_storm.ok() )
    {
      set_tick_callback( [p]( buff_t*, int, const timespan_t& ) {
        p->buff.stormbringer->trigger( p->buff.stormbringer->max_stack() );
      } );
    }
  }
};

struct strength_of_earth_buff_t : public buff_t
{
  double default_value;
  strength_of_earth_buff_t( shaman_t* p )
    : buff_t( p, "strength_of_earth", p->find_spell( 273465 ) ), default_value( p->azerite.strength_of_earth.value() )
  {
    set_default_value( default_value );
    set_duration( s_data->duration() );
    set_max_stack( 1 );
  }
};

struct thunderaans_fury_buff_t : public buff_t
{
  thunderaans_fury_buff_t( shaman_t* p ) : buff_t( p, "thunderaans_fury", p->find_spell( 287802 ) )
  {
    set_default_value( s_data->effectN( 2 ).percent() );
    set_duration( s_data->duration() );
  }
};

struct lightning_shield_buff_t : public buff_t
{
  lightning_shield_buff_t( shaman_t* p ) : buff_t( p, "lightning_shield", p->find_spell( 192106 ) )
  {
    set_chance( p->talent.lightning_shield->ok() );
  }
};

struct lightning_shield_overcharge_buff_t : public buff_t
{
  lightning_shield_overcharge_buff_t( shaman_t* p )
    : buff_t( p, "lightning_shield_overcharge", p->find_spell( 273323 ) )
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

struct landslide_buff_t : public buff_t
{
  landslide_buff_t( shaman_t* p ) : buff_t( p, "landslide", p->find_spell( 202004 ) )
  {
    set_duration( s_data->duration() );
    set_default_value( s_data->effectN( 1 ).percent() );
  }
};

struct icy_edge_buff_t : public buff_t
{
  icy_edge_buff_t( shaman_t* p ) : buff_t( p, "icy_edge", p->find_spell( 224126 ) )
  {
    set_duration( s_data->duration() );
    set_max_stack( 2 );
  }
};

struct molten_weapon_buff_t : public buff_t
{
  molten_weapon_buff_t( shaman_t* p ) : buff_t( p, "molten_weapon", p->find_spell( 224125 ) )
  {
    set_duration( s_data->duration() );
    set_default_value( s_data->effectN( 1 ).percent() );
    set_max_stack( 2 );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct crackling_surge_buff_t : public buff_t
{
  crackling_surge_buff_t( shaman_t* p ) : buff_t( p, "crackling_surge", p->find_spell( 224127 ) )
  {
    set_duration( s_data->duration() );
    set_default_value( s_data->effectN( 1 ).percent() );
    set_max_stack( 2 );
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
    set_tick_callback( [p]( buff_t* b, int, const timespan_t& ) {
      double g = b->data().effectN( 4 ).base_value();
      p->resource_gain( RESOURCE_MAELSTROM, g, p->gain.ascendance );
    } );
    set_cooldown( timespan_t::zero() );  // Cooldown is handled by the action
  }

  void ascendance( attack_t* mh, attack_t* oh, timespan_t lvb_cooldown );
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

struct ancestral_resonance_buff_t : public stat_buff_t
{
  double standard_rppm;
  double bloodlust_rppm;

  ancestral_resonance_buff_t( shaman_t* p )
    : stat_buff_t( p, "ancestral_resonance", p->find_spell( 277943 ) ),
      standard_rppm( 1u ),  // standard value is not in spell data
      bloodlust_rppm( p->find_spell( 277926 )->real_ppm() )
  {
    add_invalidate( CACHE_MASTERY );
    add_stat( stat_e::STAT_MASTERY_RATING, p->azerite.ancestral_resonance.value( 1 ) );
    set_rppm( rppm_scale_e::RPPM_HASTE, standard_rppm );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( player->buffs.bloodlust->up() )
    {
      rppm->set_frequency( bloodlust_rppm );
    }
    else
    {
      rppm->set_frequency( standard_rppm );
    }

    return stat_buff_t::trigger( stacks, value, chance, duration );
  }
};

shaman_td_t::shaman_td_t( player_t* target, shaman_t* p ) : actor_target_data_t( target, p )
{
  // Elemental
  dot.flame_shock = target->get_dot( "flame_shock", p );

  // Enhancement
  dot.searing_assault  = target->get_dot( "searing_assault", p );
  dot.molten_weapon    = target->get_dot( "molten_weapon", p );
  debuff.earthen_spike = buff_creator_t( *this, "earthen_spike", p->talent.earthen_spike )
                             .cd( timespan_t::zero() )  // Handled by the action
                             // -10% resistance in spell data, treat it as a multiplier instead
                             .default_value( 1.0 + p->talent.earthen_spike->effectN( 2 ).percent() );

  // Azerite Traits
  debuff.lightning_conduit = buff_creator_t( *this, "lightning_conduit", p->azerite.lightning_conduit )
                                 .trigger_spell( p->find_spell( 275391 ) )
                                 .duration( p->find_spell( 275391 )->duration() )
                                 .default_value( p->azerite.primal_primer.value() );
  debuff.primal_primer = buff_creator_t( *this, "primal_primer", p->azerite.primal_primer )
                             .trigger_spell( p->find_spell( 273006 ) )
                             .duration( p->find_spell( 273006 )->duration() )
                             .max_stack( p->find_spell( 273006 )->max_stacks() )
                             // Primal Primer has a hardcoded /2 in its tooltip
                             .default_value( 0.5 * p->azerite.primal_primer.value() );
}

// ==========================================================================
// Shaman Action Base Template
// ==========================================================================

template <class Base>
struct shaman_action_t : public Base
{
private:
  typedef Base ab;  // action base, eg. spell_t
public:
  typedef shaman_action_t base_t;

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

  // Generic procs

  shaman_action_t( const std::string& n, shaman_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      track_cd_waste( s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero() ),
      cd_wasted_exec( nullptr ),
      cd_wasted_cumulative( nullptr ),
      cd_wasted_iter( nullptr ),
      unshift_ghost_wolf( true ),
      gain( player->get_gain( s->id() > 0 ? s->name_cstr() : n ) ),
      maelstrom_gain( 0 ),
      maelstrom_gain_coefficient( 1.0 ),
      enable_enh_mastery_scaling( false )
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
      ab::energize_type = ENERGIZE_NONE;  // disable resource generation from spell data.
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
      ab::gcd_haste = HASTE_ATTACK;
    }
  }

  double composite_attack_power() const override
  {
    double m = ab::composite_attack_power();

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

    return m;
  }

  void init_finished() override
  {
    ab::init_finished();
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

  virtual double composite_maelstrom_gain_coefficient( const action_state_t* ) const
  {
    double m = maelstrom_gain_coefficient;

    return m;
  }

  void execute() override
  {
    ab::execute();

    trigger_maelstrom_gain( ab::execute_state );
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );
  }

  void impact( action_state_t* state ) override
  {
    ab::impact( state );

    p()->trigger_stormbringer( state );
    p()->trigger_ancestral_resonance( state );
  }

  void schedule_execute( action_state_t* execute_state = nullptr ) override
  {
    if ( !ab::background && unshift_ghost_wolf )
    {
      p()->buff.ghost_wolf->expire();
    }

    ab::schedule_execute( execute_state );
  }

  virtual void update_ready( timespan_t cd ) override
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

  void consume_resource() override
  {
    ab::consume_resource();
  }

  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    auto ret = ab::consume_cost_per_tick( dot );

    return ret;
  }

  expr_t* create_expression( const std::string& name ) override
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

    return new hprio_cd_min_remains_expr_t( this );
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
  typedef shaman_action_t<melee_attack_t> ab;

public:
  bool may_proc_windfury;
  bool may_proc_flametongue;
  bool may_proc_frostbrand;
  bool may_proc_maelstrom_weapon;
  bool may_proc_stormbringer;
  bool may_proc_lightning_shield;
  bool may_proc_hot_hand;
  bool may_proc_icy_edge;
  bool may_proc_strength_of_earth;
  bool may_proc_primal_primer;
  bool may_proc_ability_procs;  // For things that explicitly state they proc from "abilities" (like Ancestral
                                // Resonance)
  double tf_proc_chance;

  proc_t *proc_wf, *proc_ft, *proc_fb, *proc_mw, *proc_sb, *proc_ls, *proc_hh, *proc_pp;

  shaman_attack_t( const std::string& token, shaman_t* p, const spell_data_t* s )
    : base_t( token, p, s ),
      may_proc_windfury( p->spec.windfury->ok() ),
      may_proc_flametongue( p->spec.flametongue->ok() ),
      may_proc_frostbrand( p->spec.frostbrand->ok() ),
      may_proc_maelstrom_weapon( false ),  // Change to whitelisting
      may_proc_stormbringer( p->spec.stormbringer->ok() ),
      may_proc_lightning_shield( false ),
      may_proc_hot_hand( p->talent.hot_hand->ok() ),
      may_proc_icy_edge( false ),
      may_proc_strength_of_earth( true ),
      may_proc_primal_primer( true ),
      may_proc_ability_procs( true ),
      tf_proc_chance( 0 ),
      proc_wf( nullptr ),
      proc_ft( nullptr ),
      proc_fb( nullptr ),
      proc_mw( nullptr ),
      proc_sb( nullptr ),
      proc_ls( nullptr ),
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

    if ( may_proc_frostbrand )
    {
      may_proc_frostbrand = ab::weapon != nullptr;
    }

    if ( may_proc_hot_hand )
    {
      may_proc_hot_hand = ab::weapon != nullptr;
    }

    may_proc_lightning_shield = ab::weapon != nullptr;

    may_proc_strength_of_earth = true;

    if ( p()->azerite.thunderaans_fury.ok() )
    {
      tf_proc_chance = p()->find_spell( 287801 )->proc_chance();
    }
  }

  void init_finished() override
  {
    if ( may_proc_flametongue )
    {
      proc_ft = player->get_proc( std::string( "Flametongue: " ) + full_name() );
    }

    if ( may_proc_frostbrand )
    {
      proc_fb = player->get_proc( std::string( "Frostbrand: " ) + full_name() );
    }

    if ( may_proc_hot_hand )
    {
      proc_hh = player->get_proc( std::string( "Hot Hand: " ) + full_name() );
    }

    if ( may_proc_lightning_shield )
    {
      proc_ls = player->get_proc( std::string( "Lightning Shield Overcharge: " ) + full_name() );
    }

    if ( may_proc_maelstrom_weapon )
    {
      proc_mw = player->get_proc( std::string( "Maelstrom Weapon: " ) + full_name() );
    }

    if ( may_proc_stormbringer )
    {
      proc_sb = player->get_proc( std::string( "Stormbringer: " ) + full_name() );
    }

    if ( may_proc_windfury )
    {
      proc_wf = player->get_proc( std::string( "Windfury: " ) + full_name() );
    }

    if ( may_proc_primal_primer )
    {
      proc_pp = player->get_proc( std::string( "Primal Primer: " ) + full_name() );
    }

    base_t::init_finished();
  }

  virtual double maelstrom_weapon_energize_amount( const action_state_t* /* source */ ) const
  {
    return p()->spell.maelstrom_melee_gain->effectN( 1 ).resource( RESOURCE_MAELSTROM );
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    // Bail out early if the result is a miss/dodge/parry/ms
    if ( !result_is_hit( state->result ) )
      return;

    trigger_maelstrom_weapon( state );
    p()->trigger_windfury_weapon( state );
    // p()->trigger_stormbringer( state );
    p()->trigger_flametongue_weapon( state );
    p()->trigger_hailstorm( state );
    p()->trigger_lightning_shield( state );
    p()->trigger_hot_hand( state );
    p()->trigger_icy_edge( state );
    p()->trigger_primal_primer( state );

    // Azerite
    p()->trigger_strength_of_earth( state );
    p()->trigger_natural_harmony( state );
  }

  void trigger_maelstrom_weapon( const action_state_t* source_state, double amount = 0 )
  {
    if ( !may_proc_maelstrom_weapon )
    {
      return;
    }

    if ( !this->weapon )
    {
      return;
    }

    if ( p()->buff.ghost_wolf->check() )
    {
      return;
    }

    if ( source_state->result_raw <= 0 )
    {
      return;
    }

    if ( amount == 0 )
    {
      amount = this->maelstrom_weapon_energize_amount( source_state );
    }

    p()->resource_gain( RESOURCE_MAELSTROM, amount, gain, this );
    proc_mw->occur();
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
  typedef shaman_action_t<Base> ab;

public:
  typedef shaman_spell_base_t<Base> base_t;

  shaman_spell_base_t( const std::string& n, shaman_t* player, const spell_data_t* s = spell_data_t::nil() )
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
      ab::p()->resource_gain( RESOURCE_MAELSTROM, ab::last_resource_cost, ab::p()->gain.aftershock, nullptr );
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, ab::player );
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
  bool may_proc_stormbringer      = false;
  bool may_proc_strength_of_earth = false;
  proc_t* proc_sb;
  bool affected_by_master_of_the_elements = false;
  bool affected_by_stormkeeper            = false;

  shaman_spell_t( const std::string& token, shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() )
    : base_t( token, p, s ), overload( nullptr ), proc_sb( nullptr )
  {
    parse_options( options );

    if ( data().affected_by( p->spec.elemental_fury->effectN( 1 ) ) )
    {
      crit_bonus_multiplier *= 1.0 + p->spec.elemental_fury->effectN( 1 ).percent();
    }

    if ( data().affected_by( player->sets->set( SHAMAN_ELEMENTAL, T19, B2 ) ) )
    {
      base_crit += player->sets->set( SHAMAN_ELEMENTAL, T19, B2 )->effectN( 1 ).percent();
    }

    if ( data().affected_by( p->find_spell( 260734 )->effectN( 1 ) ) )
    {
      affected_by_master_of_the_elements = true;
    }

    if ( data().affected_by( p->find_spell( 191634 )->effectN( 1 ) ) )
    {
      affected_by_stormkeeper = true;
    }

    may_proc_stormbringer      = false;
    may_proc_strength_of_earth = false;
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
  }

  void schedule_travel( action_state_t* s ) override
  {
    trigger_elemental_overload( s );

    base_t::schedule_travel( s );
  }

  virtual bool usable_moving() const override
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

  void trigger_elemental_overload( const action_state_t* source_state ) const
  {
    struct elemental_overload_event_t : public event_t
    {
      action_state_t* state;

      elemental_overload_event_t( action_state_t* s )
        : event_t( *s->action->player, timespan_t::from_millis( 400 ) ), state( s )
      {
      }

      ~elemental_overload_event_t()
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
      overload->snapshot_state( s, DMG_DIRECT );
      s->target = source_state->target;

      make_event<elemental_overload_event_t>( *sim, s );
    }
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    // p()->trigger_stormbringer( state );

    // Azerite
    p()->trigger_strength_of_earth( state );
    p()->trigger_natural_harmony( state );
  }

  virtual double stormbringer_proc_chance() const
  {
    double base_chance = 0;

    base_chance += p()->spec.stormbringer->proc_chance() +
                   p()->cache.mastery() * p()->mastery.enhanced_elements->effectN( 3 ).mastery_value();

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
void summon( const T& container, const timespan_t& duration, size_t n = 1 )
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
    regen_type = REGEN_DISABLED;

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

  action_t* create_action( const std::string& name, const std::string& options_str ) override;

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
                                [this]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

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
  typedef pet_melee_attack_t<T_PET> super;

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
  typedef pet_spell_t<T_PET> super;

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

action_t* shaman_pet_t::create_action( const std::string& name, const std::string& options_str )
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
        o->resource_gain( RESOURCE_MAELSTROM, maelstrom->effectN( 1 ).resource( RESOURCE_MAELSTROM ),
                          o->gain.feral_spirit, state->action );
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
// DOOM WOLVES OF DOOM
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

      p()->o()->resource_gain( RESOURCE_MAELSTROM, maelstrom->effectN( 1 ).resource( RESOURCE_MAELSTROM ),
                               p()->o()->gain.feral_spirit, state->action );
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

  action_t* create_action( const std::string& name, const std::string& options_str ) override
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
    else
    {
      // basic fire elemental doesn't have a second attack. Only fire_blast
    }
    def->add_action( "fire_blast" );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
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

    if ( o()->azerite.echo_of_the_elementals.ok() )
    {
      o()->pet.guardian_ember_elemental->summon( o()->find_spell( 275385 )->duration() );
    }
  }
};

// create baby azerite trait version

struct ember_elemental_t : public primal_elemental_t
{
  ember_elemental_t( shaman_t* owner, bool guardian ) : primal_elemental_t( owner, "ember_elemental", guardian, false )
  {
  }

  struct ember_blast_t : public pet_spell_t<ember_elemental_t>
  {
    ember_blast_t( ember_elemental_t* player, const std::string& options )
      : super( player, "ember_blast", player->find_spell( 275382 ), options )
    {
      may_crit    = true;
      base_dd_min = base_dd_max = player->o()->azerite.echo_of_the_elementals.value();
    }

    bool usable_moving() const override
    {
      return true;
    }
  };

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );

    def->add_action( "ember_blast" );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "ember_blast" )
      return new ember_blast_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
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

    call_lightning = buff_creator_t( this, "call_lightning", find_spell( 157348 ) ).cd( timespan_t::zero() );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = primal_elemental_t::composite_player_multiplier( school );

    if ( call_lightning->up() )
      m *= 1.0 + call_lightning->data().effectN( 2 ).percent();

    return m;
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
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
    if ( o()->azerite.echo_of_the_elementals.ok() )
    {
      o()->pet.guardian_spark_elemental->summon( o()->find_spell( 275386 )->duration() );
    }
  }
};

// create baby azerite trait version

struct spark_elemental_t : public primal_elemental_t
{
  spark_elemental_t( shaman_t* owner, bool guardian ) : primal_elemental_t( owner, "spark_elemental", guardian, false )
  {
  }

  struct shocking_blast_t : public pet_spell_t<spark_elemental_t>
  {
    shocking_blast_t( spark_elemental_t* player, const std::string& options )
      : super( player, "shocking_blast", player->find_spell( 275384 ), options )
    {
      may_crit    = true;
      base_dd_min = base_dd_max = player->o()->azerite.echo_of_the_elementals.value();
    }

    bool usable_moving() const override
    {
      return true;
    }
  };

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );

    def->add_action( "shocking_blast" );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "shocking_blast" )
      return new shocking_blast_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
  }
};

}  // namespace pet

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct flametongue_weapon_spell_t : public shaman_spell_t
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

struct searing_assault_t : public shaman_spell_t
{
  searing_assault_t( shaman_t* player ) : shaman_spell_t( "searing_assault", player, player->find_spell( 268429 ) )
  {
    tick_may_crit = true;
    may_crit      = true;
    hasted_ticks  = false;
    school        = SCHOOL_FIRE;
    background    = true;

    // dot_duration = s->duration();
    base_tick_time = dot_duration / 3;
  }

  double action_ta_multiplier() const override
  {
    double m = shaman_spell_t::action_ta_multiplier();

    return m;
  }

  void tick( dot_t* d ) override
  {
    shaman_spell_t::tick( d );
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

  double maelstrom_weapon_energize_amount( const action_state_t* source ) const override
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
      p()->resource_gain( RESOURCE_MAELSTROM, bonus_resource, p()->gain.forceful_winds );
    }

    shaman_attack_t::impact( state );
  }
};

struct crash_lightning_attack_t : public shaman_attack_t
{
  crash_lightning_attack_t( shaman_t* p, const std::string& n ) : shaman_attack_t( n, p, p->find_spell( 195592 ) )
  {
    weapon     = &( p->main_hand_weapon );
    background = true;
    callbacks  = false;
    aoe        = -1;
    base_multiplier *= 1.0;
    may_proc_ability_procs = false;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_frostbrand = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_maelstrom_weapon = may_proc_lightning_shield = false;
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

struct hailstorm_attack_t : public shaman_attack_t
{
  hailstorm_attack_t( const std::string& n, shaman_t* p, weapon_t* w )
    : shaman_attack_t( n, p, p->find_spell( 210854 ) )
  {
    weapon                 = w;
    background             = true;
    callbacks              = false;
    may_proc_ability_procs = false;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_frostbrand = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_maelstrom_weapon = may_proc_lightning_shield = false;
  }
};

struct icy_edge_attack_t : public shaman_attack_t
{
  icy_edge_attack_t( const std::string& n, shaman_t* p, weapon_t* w ) : shaman_attack_t( n, p, p->find_spell( 271920 ) )
  {
    weapon                 = w;
    background             = true;
    callbacks              = false;
    may_proc_ability_procs = false;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_frostbrand = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_maelstrom_weapon = may_proc_lightning_shield = false;
  }
};

struct lightning_conduit_zap_t : public shaman_spell_t
{
  double damage;
  lightning_conduit_zap_t( shaman_t* player )
    : shaman_spell_t( "lightning_conduit", player, player->find_spell( 275394 ) ),
      damage( p()->azerite.lightning_conduit.value() )
  {
    base_dd_min = base_dd_max = damage;
    // base_td    = player->azerite.lightning_conduit.value(); --maybe not needed? spell isnt white listed atm.
    background = true;
    may_crit   = true;
    callbacks  = false;
  }
};

struct strength_of_earth_t : public shaman_spell_t
{
  double damage;
  strength_of_earth_t( shaman_t* player )
    : shaman_spell_t( "strength_of_earth", player, player->find_spell( 273466 ) ),
      damage( p()->azerite.strength_of_earth.value() )
  {
    base_dd_min = base_dd_max = damage;
    background                = true;
    may_crit                  = true;
    callbacks                 = false;
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
    may_proc_lightning_shield = true;
    school                    = SCHOOL_PHYSICAL;
  }

  void init() override
  {
    shaman_attack_t::init();
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p()->buff.storm_totem->up() )
    {
      m *= 1.0 + p()->buff.storm_totem->data().effectN( 1 ).percent();
    }

    if ( p()->buff.landslide->up() )
    {
      m *= 1.0 + p()->buff.landslide->value();
    }

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

    if ( p()->azerite.thunderaans_fury.ok() )
    {
      // currently buggy on ptr, is applying 2/3 to each hit instead of 1/3 on oh
      // double tf_bonus = 0.5 * p()->azerite.thunderaans_fury.value( 2 );
      double tf_bonus = ( 2.0 / 3.0 ) * p()->azerite.thunderaans_fury.value( 2 );
      b += tf_bonus;
    }

    if ( p()->buff.stormbringer->check() )
    {
      double rs_bonus = ( 2.0 / 3.0 ) * p()->azerite.roiling_storm.value( 1 );
      // New Roiling Storm has 66% penalty from the tooltip applied to MH and 33% to OH.
      if ( weapon && weapon->slot == SLOT_OFF_HAND )
      {
        rs_bonus *= 0.5;
      }

      b += rs_bonus;
    }

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

    if ( p()->azerite.thunderaans_fury.ok() )
    {
      // currently buggy on ptr, is applying 2/3 to each hit instead of 1/3 on oh
      // double tf_bonus = 0.5 * p()->azerite.thunderaans_fury.value( 2 );
      double tf_bonus = ( 2 / 3.0 ) * p()->azerite.thunderaans_fury.value( 2 );
      b += tf_bonus;
    }

    if ( p()->buff.stormbringer->check() )
    {
      double rs_bonus = ( 2.0 / 3.0 ) * p()->azerite.roiling_storm.value( 1 );
      // New Roiling Storm has 66% penalty from the tooltip applied to MH and 33% to OH.
      if ( weapon && weapon->slot == SLOT_OFF_HAND )
      {
        rs_bonus *= 0.5;
      }

      b += rs_bonus;
    }

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
    may_proc_ability_procs;
    may_glance = special = false;
    weapon               = w;
    weapon_multiplier    = 1.0;
    base_execute_time    = w->swing_time;
    trigger_gcd          = timespan_t::zero();

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
    callbacks         = false;
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

struct ancestral_awakening_t : public shaman_heal_t
{
  ancestral_awakening_t( shaman_t* player )
    : shaman_heal_t( "ancestral_awakening", player, player->find_spell( 52752 ) )
  {
    background = proc = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_heal_t::composite_da_multiplier( state );
    m *= p()->spec.ancestral_awakening->effectN( 1 ).percent();
    return m;
  }

  void execute() override
  {
    target = find_lowest_player();
    shaman_heal_t::execute();
  }
};

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

    if ( p()->spec.ancestral_awakening->ok() )
    {
      if ( !p()->action.ancestral_awakening )
      {
        p()->action.ancestral_awakening = new ancestral_awakening_t( p() );
        p()->action.ancestral_awakening->init();
      }

      p()->action.ancestral_awakening->base_dd_min = s->result_total;
      p()->action.ancestral_awakening->base_dd_max = s->result_total;
    }
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

  virtual timespan_t execute_time() const override
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
  crash_lightning_attack_t* cl;
  molten_weapon_dot_t* mw_dot;

  lava_lash_t( shaman_t* player, const std::string& options_str )
    : shaman_attack_t( "lava_lash", player, player->find_specialization_spell( "Lava Lash" ) ),
      cl( new crash_lightning_attack_t( player, "lava_lash_cl" ) ),
      mw_dot( nullptr )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    school = SCHOOL_FIRE;

    base_multiplier *= 1.0;

    parse_options( options_str );
    weapon = &( player->off_hand_weapon );

    if ( weapon->type == WEAPON_NONE )
      background = true;  // Do not allow execution.

    add_child( cl );

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
    if ( s->target )
      b += td( s->target )->debuff.primal_primer->stack_value();

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

    if ( p()->buff.ember_totem->up() )
    {
      m *= 1.0 + p()->buff.ember_totem->data().effectN( 1 ).percent();
    }

    return m;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->buff.hot_hand->decrement();
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    if ( result_is_hit( state->result ) && p()->buff.crash_lightning->up() )
    {
      cl->set_target( state->target );
      cl->schedule_execute();
    }

    if ( p()->buff.molten_weapon->up() )
    {
      trigger_molten_weapon_dot( state->target, state->result_amount );
    }

    td( state->target )->debuff.primal_primer->expire();
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
  crash_lightning_attack_t* cl;
  stormstrike_attack_t *mh, *oh;
  bool background_action;

  stormstrike_base_t( shaman_t* player, const std::string& name, const spell_data_t* spell,
                      const std::string& options_str )
    : shaman_attack_t( name, player, spell ),
      cl( new crash_lightning_attack_t( player, name + "_cl" ) ),
      mh( nullptr ),
      oh( nullptr ),
      background_action( false )
  {
    parse_options( options_str );

    weapon             = &( p()->main_hand_weapon );
    cooldown           = p()->cooldown.strike;
    cooldown->duration = data().cooldown();
    weapon_multiplier  = 0.0;
    may_crit           = false;
    school             = SCHOOL_PHYSICAL;

    add_child( cl );
  }

  void init() override
  {
    shaman_attack_t::init();
    may_proc_flametongue = may_proc_windfury = may_proc_stormbringer = may_proc_frostbrand = false;
    may_proc_strength_of_earth                                                             = true;
    may_proc_primal_primer                                                                 = false;
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
        cl->set_target( execute_state->target );
        cl->execute();
      }

      if ( p()->azerite.thunderaans_fury.ok() )
      {
        if ( rng().roll( tf_proc_chance ) )
        {
          p()->buff.thunderaans_fury->trigger();
        }
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
    if ( p()->buff.lightning_shield->up() )
    {
      if ( !state->action->result_is_hit( state->result ) )
      {
        return;
      }

      p()->buff.lightning_shield->trigger();
      p()->buff.lightning_shield->trigger();

      if ( p()->buff.lightning_shield->stack() >=
           20 )  // if 20 or greater, trigger overcharge and remove all stacks, then trigger LS back to 1.
      {          // is there a way to do this without expiring lightning shield entirely?
        p()->buff.lightning_shield_overcharge->trigger();
        p()->buff.lightning_shield->expire();
        p()->buff.lightning_shield->trigger();
      }
    }

    if ( p()->azerite.lightning_conduit.ok() )
    {
      std::vector<player_t*> tl = target_list();
      for ( size_t i = 0, actors = tl.size(); i < actors; i++ )
      {
        if ( p()->get_target_data( tl[ i ] )->debuff.lightning_conduit->up() )
        {
          p()->lightning_conduit->set_target( tl[ i ] );
          p()->lightning_conduit->schedule_execute();
        }
      }

      // lightning conduit applies to your primary target after it deals damage to others.
      td( target )->debuff.lightning_conduit->trigger();
    }

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

    p()->buff.landslide->expire();
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

  double recharge_multiplier() const override
  {
    auto m = stormstrike_base_t::recharge_multiplier();

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

    p()->buff.landslide->decrement();
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
    may_proc_stormbringer = may_proc_windfury = may_proc_flametongue = may_proc_frostbrand = false;
    may_proc_lightning_shield                                                              = true;
    may_proc_hot_hand                                                                      = p()->talent.hot_hand->ok();
  }
};

// Rockbiter Spell =========================================================

struct rockbiter_t : public shaman_spell_t
{
  rockbiter_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "rockbiter", player, player->find_specialization_spell( "Rockbiter" ), options_str )
  {
    base_multiplier *= 1.0;

    base_multiplier *= 1.0 + player->talent.boulderfist->effectN( 2 ).percent();

    // TODO: SpellCategory + SpellEffect based detection
    cooldown->hasted = true;
  }

  double recharge_multiplier() const override
  {
    double m = shaman_spell_t::recharge_multiplier();

    m *= 1.0 + p()->talent.boulderfist->effectN( 1 ).percent();

    return m;
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();
    return m;
  }

  void init() override
  {
    shaman_spell_t::init();
    may_proc_stormbringer      = true;
    may_proc_strength_of_earth = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    shaman_spell_t::impact( s );

    if ( p()->talent.landslide->ok() )
    {
      double proc_chance = p()->talent.landslide->proc_chance();
      if ( rng().roll( proc_chance ) )
      {
        p()->buff.landslide->trigger();
      }
    }

    if ( p()->azerite.strength_of_earth.ok() )
    {
      p()->buff.strength_of_earth->trigger();
    }
  }
};

// Flametongue Spell =========================================================

struct flametongue_t : public shaman_spell_t
{
  flametongue_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "flametongue", player, player->find_specialization_spell( "Flametongue" ), options_str )
  {
    add_child( player->flametongue );
    if ( player->action.searing_assault )
    {
      add_child( player->action.searing_assault );
    }
  }

  void init() override
  {
    shaman_spell_t::init();
    may_proc_stormbringer      = true;
    may_proc_strength_of_earth = true;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.flametongue->trigger();
  }

  void impact( action_state_t* s ) override
  {
    shaman_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->trigger_searing_assault( s );
    }
  }
};

// Frostbrand Spell =========================================================

struct frostbrand_t : public shaman_spell_t
{
  frostbrand_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "frostbrand", player, player->find_specialization_spell( "Frostbrand" ), options_str )
  {
    dot_duration   = timespan_t::zero();
    base_tick_time = timespan_t::zero();

    if ( player->hailstorm )
      add_child( player->hailstorm );
  }

  // TODO: If some spells are intended to proc stormbringer and becomes perm, move init and impact into spell base.
  // Currently assumed to be bugged.
  void init() override
  {
    shaman_spell_t::init();
    may_proc_stormbringer      = true;
    may_proc_strength_of_earth = true;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.frostbrand->trigger();
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
    ap_type = AP_WEAPON_BOTH;

    if ( player->action.crashing_storm )
    {
      add_child( player->action.crashing_storm );
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

// Earth Elemental ===========================================================

struct earth_elemental_t : public shaman_spell_t
{
  earth_elemental_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "earth_elemental", player, player->find_spell( 188616 ), options_str )
  {
    harmful = may_crit = false;
    cooldown->duration =
        player->find_spell( 198103 )->cooldown();  // earth ele cd and durations are on different spells.. go figure.
  }

  void execute() override
  {
    shaman_spell_t::execute();

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
  const spell_data_t* base_spell;

  fire_elemental_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "fire_elemental", player, player->find_specialization_spell( "Fire Elemental" ), options_str ),
      base_spell( player->find_spell( 188592 ) )
  {
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->talent.primal_elementalist->ok() && p()->pet.pet_fire_elemental->is_sleeping() )
    {
      p()->pet.pet_fire_elemental->summon( base_spell->duration() );
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, player );
    }
    else if ( !p()->talent.primal_elementalist->ok() )
    {
      // Summon first non sleeping Fire Elemental
      pet::summon( p()->pet.guardian_fire_elemental, base_spell->duration() );
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_VERSATILITY_RATING, player );
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

// create baby summon of azerite trait Echo of the Elementals
struct ember_elemental_t : public shaman_spell_t
{
  const spell_data_t* base_spell;

  ember_elemental_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "ember_elemental", player, player->find_specialization_spell( "Ember Elemental" ), options_str ),
      base_spell( player->find_spell( 275385 ) )
  {
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->azerite.echo_of_the_elementals.ok() )
    {
      p()->pet.guardian_ember_elemental->summon( base_spell->duration() );
    }
  }
};

// Storm Elemental ==========================================================

struct storm_elemental_t : public shaman_spell_t
{
  const spell_data_t* summon_spell;

  storm_elemental_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "storm_elemental", player, player->talent.storm_elemental, options_str ),
      summon_spell( player->find_spell( 157299 ) )
  {
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->talent.primal_elementalist->ok() && p()->pet.pet_storm_elemental->is_sleeping() )
    {
      p()->pet.pet_storm_elemental->summon( summon_spell->duration() );
    }
    else if ( !p()->talent.primal_elementalist->ok() )
    {
      // Summon first non sleeping Storm Elemental
      pet::summon( p()->pet.guardian_storm_elemental, summon_spell->duration() );
    }
  }
};

// create baby summon of azerite trait Echo of the Elementals
struct spark_elemental_t : public shaman_spell_t
{
  const spell_data_t* base_spell;

  spark_elemental_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "spark_elemental", player, player->find_specialization_spell( "Spark Elemental" ), options_str ),
      base_spell( player->find_spell( 275386 ) )
  {
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->azerite.echo_of_the_elementals.ok() )
    {
      p()->pet.guardian_spark_elemental->summon( base_spell->duration() );
    }
  }
};

struct fury_of_air_aoe_t : public shaman_attack_t
{
  fury_of_air_aoe_t( shaman_t* player ) : shaman_attack_t( "fury_of_air_damage", player, player->find_spell( 197385 ) )
  {
    background = true;
    aoe        = -1;
    school     = SCHOOL_NATURE;
    ap_type    = AP_WEAPON_BOTH;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_flametongue = may_proc_stormbringer = may_proc_frostbrand = false;
  }
};

struct fury_of_air_t : public shaman_spell_t
{
  fury_of_air_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "fury_of_air", player, player->talent.fury_of_air, options_str )
  {
    callbacks = false;

    // Handled by the buff
    base_tick_time = timespan_t::zero();

    if ( player->action.fury_of_air )
    {
      add_child( player->action.fury_of_air );
    }
  }

  void init() override
  {
    shaman_spell_t::init();

    // Set up correct gain object to collect resource spending information on the ticking buff
    p()->gain.fury_of_air = &( stats->resource_gain );
  }

  timespan_t gcd() const override
  {
    // Disabling Fury of Air does not incur a global cooldown
    if ( p()->buff.fury_of_air->check() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::gcd();
  }

  double cost() const override
  {
    // Disabling Fury of Air costs an amount of maelstrom relative to the elapsed tick time of the
    // on-going tick .. probably
    if ( p()->buff.fury_of_air->check() )
    {
      return base_costs[ RESOURCE_MAELSTROM ] *
             ( 1 - p()->buff.fury_of_air->tick_event->remains() / data().effectN( 1 ).period() );
    }

    return shaman_spell_t::cost();
  }

  void execute() override
  {
    // Don't record disables by setting dual = true before executing
    if ( p()->buff.fury_of_air->check() )
    {
      dual = true;
    }

    shaman_spell_t::execute();

    if ( p()->buff.fury_of_air->check() )
    {
      p()->buff.fury_of_air->expire();
      dual = false;
    }
    else
    {
      p()->buff.fury_of_air->trigger();
    }
  }
};

// TODO: Convert to shaman_spell_t, it is a spell, not an attack
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

    may_proc_primal_primer = false;
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

  virtual void execute() override
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

  virtual void execute() override
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

    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_HASTE_RATING, player );
  }

  virtual bool ready() override
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
    energize_type  = ENERGIZE_NONE;  // disable resource generation from spell data.
    maelstrom_gain = mg;
    radius         = 10.0;

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
                               p->find_spell( 190493 )->effectN( 6 ).resource( RESOURCE_MAELSTROM ) )
  {
    affected_by_master_of_the_elements = true;
  }

  void impact( action_state_t* state ) override
  {
    chained_overload_base_t::impact( state );

    if ( p()->azerite.synapse_shock.ok() )
    {
      p()->buff.synapse_shock->trigger();
    }
  }
};

struct lava_beam_overload_t : public chained_overload_base_t
{
  lava_beam_overload_t( shaman_t* p )
    : chained_overload_base_t( p, "lava_beam_overload", p->find_spell( 114738 ),
                               p->find_spell( 190493 )->effectN( 6 ).resource( RESOURCE_MAELSTROM ) )
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

    maelstrom_gain = mg;
    energize_type  = ENERGIZE_NONE;  // disable resource generation from spell data.

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
    base_chance += p()->buff.storm_totem->value();

    return base_chance / 3.0;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->buff.stormkeeper->check() )
    {
      p()->buff.stormkeeper->decrement();
    }
    else
    {
      p()->buff.tectonic_thunder->expire();
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
    : chained_base_t( player, "chain_lightning", player->find_specialization_spell( "Chain Lightning" ),
                      player->find_spell( 190493 )->effectN( 3 ).resource( RESOURCE_MAELSTROM ), options_str )
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
    else if ( p()->buff.tectonic_thunder->up() )
    {
      // Tectonic Thunder makes CL instant
      t *= 1 + p()->buff.tectonic_thunder->value();
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

    if ( p()->azerite.synapse_shock.ok() )
    {
      p()->buff.synapse_shock->trigger();
    }
  }

  void execute() override
  {
    chained_base_t::execute();

    // Storm Elemental Wind Gust passive buff trigger
    if ( p()->talent.storm_elemental->ok() )
    {
      if ( p()->talent.primal_elementalist->ok() && p()->pet.pet_storm_elemental &&
           !p()->pet.pet_storm_elemental->is_sleeping() )
      {
        p()->buff.wind_gust->trigger();
      }
      else if ( !p()->talent.primal_elementalist->ok() )
      {
        auto it = range::find_if( p()->pet.guardian_storm_elemental,
                                  []( const pet_t* p ) { return p && !p->is_sleeping(); } );

        if ( it != p()->pet.guardian_storm_elemental.end() )
        {
          p()->buff.wind_gust->trigger();
        }
      }
    }
  }
};

struct lava_beam_t : public chained_base_t
{
  lava_beam_t( shaman_t* player, const std::string& options_str )
    : chained_base_t( player, "lava_beam", player->find_spell( 114074 ),
                      player->find_spell( 114074 )->effectN( 3 ).base_value(), options_str )
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
    maelstrom_gain = player->find_spell( 190493 )->effectN( 5 ).resource( RESOURCE_MAELSTROM );

    spell_power_mod.direct = player->find_spell( 285466 )->effectN( 1 ).sp_coeff();
  }

  void init() override
  {
    elemental_overload_spell_t::init();

    std::swap( snapshot_flags, impact_flags );
  }

  void snapshot_impact_state( action_state_t* s, dmg_e rt )
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

    if ( p()->azerite.igneous_potential.ok() )
    {
      b += p()->azerite.igneous_potential.value( 2 );
    }
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
      maelstrom_gain              = player->find_spell( 190493 )->effectN( 2 ).resource( RESOURCE_MAELSTROM );
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lava_burst_overload_t( player );
      add_child( overload );
    }

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

  void snapshot_impact_state( action_state_t* s, dmg_e rt )
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
      p()->buff.t21_2pc_elemental->trigger();
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = shaman_spell_t::bonus_da( s );

    if ( p()->azerite.igneous_potential.ok() )
    {
      b += p()->azerite.igneous_potential.value( 2 );
    }
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

    if ( p()->buff.surge_of_power->up() )
    {
      p()->cooldown.fire_elemental->adjust( -1.0 * p()->talent.surge_of_power->effectN( 1 ).time_value() );
      p()->cooldown.storm_elemental->adjust( -1.0 * p()->talent.surge_of_power->effectN( 1 ).time_value() );
      p()->buff.surge_of_power->decrement();
    }

    if ( p()->talent.master_of_the_elements->ok() )
    {
      p()->buff.master_of_the_elements->trigger();
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_MASTERY_RATING, player );
    }
    else
    {
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_HASTE_RATING, player );
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
    maelstrom_gain                     = player->find_spell( 190493 )->effectN( 4 ).resource( RESOURCE_MAELSTROM );
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

    if ( p()->azerite.synapse_shock.ok() )
    {
      p()->buff.synapse_shock->trigger();
    }
  }
};

struct lightning_bolt_t : public shaman_spell_t
{
  double m_overcharge;

  lightning_bolt_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "lightning_bolt", player, player->find_specialization_spell( "Lightning Bolt" ), options_str ),
      m_overcharge( 0 )
  {
    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      maelstrom_gain                     = player->find_spell( 190493 )->effectN( 1 ).resource( RESOURCE_MAELSTROM );
      affected_by_master_of_the_elements = true;
    }

    if ( player->talent.overcharge->ok() )
    {
      cooldown->duration += player->talent.overcharge->effectN( 3 ).time_value();
      m_overcharge =
          player->talent.overcharge->effectN( 2 ).percent() / player->talent.overcharge->effectN( 1 ).base_value();
      track_cd_waste = true;
    }

    // TODO: Is it still 10% per Maelstrom with Stormbringer?
    if ( player->talent.overcharge->ok() )
    {
      secondary_costs[ RESOURCE_MAELSTROM ] = player->talent.overcharge->effectN( 1 ).base_value();
      resource_current                      = RESOURCE_MAELSTROM;
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lightning_bolt_overload_t( player );
      add_child( overload );
    }
  }

  double overload_chance( const action_state_t* s ) const override
  {
    double chance = shaman_spell_t::overload_chance( s );
    chance += p()->buff.storm_totem->value();

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

    p()->buff.stormkeeper->decrement();

    p()->buff.surge_of_power->decrement();

    if ( !p()->talent.overcharge->ok() && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      reset_swing_timers();
    }

    // Storm Elemental Wind Gust passive buff trigger
    if ( p()->talent.storm_elemental->ok() )
    {
      if ( p()->talent.primal_elementalist->ok() && p()->pet.pet_storm_elemental &&
           !p()->pet.pet_storm_elemental->is_sleeping() )
      {
        p()->buff.wind_gust->trigger();
      }
      else if ( !p()->talent.primal_elementalist->ok() )
      {
        auto it = range::find_if( p()->pet.guardian_storm_elemental,
                                  []( const pet_t* p ) { return p && !p->is_sleeping(); } );

        if ( it != p()->pet.guardian_storm_elemental.end() )
        {
          p()->buff.wind_gust->trigger();
        }
      }
    }

    // Azerite trait
    if ( p()->azerite.synapse_shock.ok() )
    {
      p()->buff.synapse_shock->trigger();
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

  // EB can no longer proc the same buff twice
  while ( ( b == 0 && p->buff.elemental_blast_crit->check() ) || ( b == 1 && p->buff.elemental_blast_haste->check() ) ||
          ( b == 2 && p->buff.elemental_blast_mastery->check() ) )
  {
    b = static_cast<unsigned>( p->rng().range( 0, 3 ) );
  }

  if ( b == 0 )
  {
    p->buff.elemental_blast_crit->trigger();
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_MASTERY_RATING, p );
  }
  else if ( b == 1 )
  {
    p->buff.elemental_blast_haste->trigger();
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_VERSATILITY_RATING, p );
  }
  else if ( b == 2 )
  {
    p->buff.elemental_blast_mastery->trigger();
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, p );
  }
}

struct elemental_blast_overload_t : public elemental_overload_spell_t
{
  elemental_blast_overload_t( shaman_t* p )
    : elemental_overload_spell_t( p, "elemental_blast_overload", p->find_spell( 120588 ) )
  {
    affected_by_master_of_the_elements = true;
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
    overload = new elemental_blast_overload_t( player );
    add_child( overload );
    affected_by_master_of_the_elements = true;
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
    maelstrom_gain                     = player->find_spell( 190493 )->effectN( 8 ).resource( RESOURCE_MAELSTROM );
    affected_by_master_of_the_elements = true;
  }
};

struct icefury_t : public shaman_spell_t
{
  icefury_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "icefury", player, player->talent.icefury, options_str )
  {
    maelstrom_gain                     = player->find_spell( 190493 )->effectN( 7 ).resource( RESOURCE_MAELSTROM );
    affected_by_master_of_the_elements = true;

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
  const spell_data_t* feral_spirit_summon;
  const spell_data_t* elemental_wolves;

  feral_spirit_spell_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "feral_spirit", player, player->find_specialization_spell( "Feral Spirit" ), options_str ),
      feral_spirit_summon( player->find_spell( 228562 ) ),
      elemental_wolves( player->talent.elemental_spirits )
  {
    harmful = false;
    cooldown->duration += player->talent.elemental_spirits->effectN( 1 ).time_value();
  }

  double recharge_multiplier() const override
  {
    double m = shaman_spell_t::recharge_multiplier();

    return m;
  }

  timespan_t summon_duration() const
  {
    return feral_spirit_summon->duration();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->talent.elemental_spirits->ok() )
    {
      // This summon evaluates the wolf type to spawn as the roll, instead of rolling against
      // the available pool of wolves to spawn.
      size_t n = 2;
      while ( n )
      {
        int wolf_roll = rng().range( 0, 3 ) + 1;  // add 1 to get the correct enum range

        for ( auto wolf : p()->pet.elemental_wolves )
        {
          if ( debug_cast<pet::base_wolf_t*>( wolf )->wolf_type == wolf_roll && wolf->is_sleeping() )
          {
            wolf->summon( summon_duration() );
            if ( wolf_roll == FIRE_WOLF )
            {
              p()->buff.molten_weapon->trigger();
            }
            if ( wolf_roll == FROST_WOLF )
            {
              p()->buff.icy_edge->trigger();
            }
            if ( wolf_roll == LIGHTNING_WOLF )
            {
              p()->buff.crackling_surge->trigger();
            }
            break;
          }
        }
        n--;
      }

      // This summon distribution produces a 1/3 chance for the first wolf, then 1/5 chance to get a repeat,
      // since it'll skip an already spawned wolf and re-roll.
      // This is incorrect as it should be 2 1/3 chances for each wolf to spawn.
      /*size_t n = 2;
      while ( n )
      {
        size_t idx = rng().range( size_t(), p()->pet.elemental_wolves.size() );
        if ( !p()->pet.elemental_wolves[ idx ]->is_sleeping() )
        {
          continue;
        }

        p()->pet.elemental_wolves[ idx ]->summon( summon_duration() );
    if (idx == 0 || idx == 1)
    {
      p()->buff.icy_edge->trigger();
    }
    if (idx == 2 || idx == 3)
    {
      p()->buff.molten_weapon->trigger();
    }
    if (idx == 4 || idx == 5)
    {
      p()->buff.crackling_surge->trigger();
    }
        n--;
      }*/
    }
    else
    {
      // Summon all Spirit Wolves
      pet::summon( p()->pet.spirit_wolves, summon_duration(), p()->pet.spirit_wolves.size() );
      p()->buff.feral_spirit->trigger();
    }
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
  }

  virtual void execute() override
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
      callbacks = may_proc_windfury = may_proc_frostbrand = may_proc_flametongue = may_proc_maelstrom_weapon = false;
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

    m *= 1.0 + p()->buff.t21_2pc_elemental->stack_value();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );
  }
};

struct tectonic_thunder_damage_t : public shaman_spell_t
{
  tectonic_thunder_damage_t( shaman_t* player )
    : shaman_spell_t( "tectonic_thunder_", player, player->find_spell( 286949 ) )
  {
    aoe        = -1;
    ground_aoe = background = true;
    school                  = SCHOOL_PHYSICAL;
    base_dd_min = base_dd_max = p()->azerite.tectonic_thunder.value( 1 );
  }

  double target_armor( player_t* ) const override
  {
    return 0;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );

    m *= 1.0 + p()->buff.master_of_the_elements->value();

    m *= 1.0 + p()->buff.t21_2pc_elemental->stack_value();

    return m;
  }
};

struct earthquake_t : public shaman_spell_t
{
  earthquake_damage_t* rumble;
  tectonic_thunder_damage_t* thunder;  // azerite trait

  earthquake_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "earthquake", player, player->find_specialization_spell( "Earthquake" ), options_str ),
      rumble( new earthquake_damage_t( player ) ),
      thunder( new tectonic_thunder_damage_t( player ) )
  {
    dot_duration = timespan_t::zero();  // The periodic effect is handled by ground_aoe_event_t
    add_child( rumble );
    if ( p()->azerite.tectonic_thunder.ok() )
    {
      add_child( thunder );  // azerite trait
    }
  }

  double cost() const override
  {
    double d = shaman_spell_t::cost();
    if ( p()->talent.call_the_thunder->ok() )
    {
      d += p()->talent.call_the_thunder->effectN( 1 ).base_value();
    }
    return d;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    make_event<ground_aoe_event_t>(
        *sim, p(),
        ground_aoe_params_t().target( execute_state->target ).duration( data().duration() ).action( rumble ) );

    if ( p()->azerite.tectonic_thunder.ok() )
    {
      make_event<ground_aoe_event_t>( *sim, p(),
                                      ground_aoe_params_t()
                                          .target( execute_state->target )
                                          .duration( timespan_t::from_seconds( 1 ) )
                                          .action( thunder ) );
    }

    if ( p()->azerite.tectonic_thunder.ok() &&
         rng().roll( p()->azerite.tectonic_thunder.spell()->effectN( 2 ).percent() ) )
    {
      p()->buff.tectonic_thunder->trigger();
    }

    // Note, needs to be decremented after ground_aoe_event_t is created so that the rumble gets the
    // buff multiplier as persistent.
    p()->buff.master_of_the_elements->expire();
    p()->buff.t21_2pc_elemental->expire();
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================

// T21 4pc bonus
struct earth_shock_overload_t : public elemental_overload_spell_t
{
  earth_shock_overload_t( shaman_t* p )
    : elemental_overload_spell_t( p, "earth_shock_overload", p->find_spell( 252143 ) )
  {
  }

  void init() override
  {
    elemental_overload_spell_t::init();

    snapshot_flags = update_flags = STATE_MUL_DA;
  }
};

struct earth_shock_t : public shaman_spell_t
{
  action_t* t21_4pc;

  earth_shock_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "earth_shock", player, player->find_specialization_spell( "Earth Shock" ), options_str ),
      t21_4pc( nullptr )
  {
    // hardcoded because spelldata doesn't provide the resource type
    resource_current                   = RESOURCE_MAELSTROM;
    affected_by_master_of_the_elements = true;

    if ( player->sets->has_set_bonus( SHAMAN_ELEMENTAL, T21, B4 ) )
    {
      t21_4pc = new earth_shock_overload_t( player );
      add_child( t21_4pc );
    }
  }

  double cost() const override
  {
    double d = shaman_spell_t::cost();
    if ( p()->talent.call_the_thunder->ok() )
    {
      d += p()->talent.call_the_thunder->effectN( 1 ).base_value();
    }
    return d;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = shaman_spell_t::bonus_da( s );
    b += p()->buff.lava_shock->stack_value();
    return b;
  }

  double action_multiplier() const override
  {
    auto m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.t21_2pc_elemental->stack_value();

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.t21_2pc_elemental->expire();
    p()->buff.lava_shock->expire();

    if ( p()->talent.surge_of_power->ok() )
    {
      p()->buff.surge_of_power->trigger();
    }

    expansion::bfa::trigger_leyshocks_grand_compilation(
        last_resource_cost == 100.0 ? STAT_HASTE_RATING : STAT_MASTERY_RATING, player );
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( t21_4pc && rng().roll( p()->sets->set( SHAMAN_ELEMENTAL, T21, B4 )->effectN( 1 ).percent() ) )
    {
      t21_4pc->base_dd_min = t21_4pc->base_dd_max = state->result_amount;
      t21_4pc->set_target( state->target );
      t21_4pc->execute();
    }
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  flame_shock_spreader_t* spreader;

  flame_shock_t( shaman_t* player, const std::string& options_str = std::string() )
    : shaman_spell_t( "flame_shock", player, player->find_specialization_spell( "Flame Shock" ), options_str ),
      spreader( player->talent.surge_of_power->ok() ? new flame_shock_spreader_t( player ) : nullptr )
  {
    tick_may_crit  = true;
    track_cd_waste = false;
  }

  double composite_crit_chance() const override
  {
    double m = shaman_spell_t::composite_crit_chance();

    if ( player->sets->has_set_bonus( SHAMAN_ELEMENTAL, T20, B4 ) && p()->active_elemental_pet() )
    {
      m += p()->find_spell( 246594 )->effectN( 1 ).percent();
    }

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = shaman_spell_t::action_ta_multiplier();

    if ( p()->buff.ember_totem->up() )
    {
      m *= p()->buff.ember_totem->check_value();
    }

    if ( player->sets->has_set_bonus( SHAMAN_ELEMENTAL, T20, B4 ) && p()->active_elemental_pet() )
    {
      m *= 1.0 + p()->find_spell( 246594 )->effectN( 2 ).percent();
    }

    return m;
  }

  void tick( dot_t* d ) override
  {
    shaman_spell_t::tick( d );

    double proc_chance = p()->spec.lava_surge->proc_chance();

    // proc chance suddenly bacame 100% and the actual chance became effectN 1
    proc_chance = p()->spec.lava_surge->effectN( 1 ).percent();

    if ( p()->azerite.igneous_potential.ok() )
    {
      proc_chance = p()->azerite.igneous_potential.spell_ref().effectN( 3 ).percent();
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
        p()->resource_gain( RESOURCE_MAELSTROM, p()->find_spell( 263819 )->effectN( 1 ).base_value(),
                            p()->gain.fire_elemental );
      }
      else if ( !p()->talent.primal_elementalist->ok() )
      {
        auto it =
            range::find_if( p()->pet.guardian_fire_elemental, []( const pet_t* p ) { return p && !p->is_sleeping(); } );

        if ( it != p()->pet.guardian_fire_elemental.end() )
        {
          p()->resource_gain( RESOURCE_MAELSTROM, p()->find_spell( 263819 )->effectN( 1 ).base_value(),
                              p()->gain.fire_elemental );
        }
      }
    }

    if ( p()->azerite.lava_shock.ok() )
    {
      p()->buff.lava_shock->trigger();
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

// T21 4pc bonus
struct frost_shock_overload_t : public elemental_overload_spell_t
{
  frost_shock_overload_t( shaman_t* p )
    : elemental_overload_spell_t( p, "frost_shock_overload", p->find_spell( 252143 ) )
  {
  }

  void init() override
  {
    elemental_overload_spell_t::init();

    snapshot_flags = update_flags = STATE_MUL_DA;
  }
};

struct frost_shock_t : public shaman_spell_t
{
  action_t* t21_4pc;

  frost_shock_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "frost_shock", player, player->find_specialization_spell( "Frost Shock" ), options_str ),
      t21_4pc( nullptr )
  {
    affected_by_master_of_the_elements = true;
    if ( player->sets->has_set_bonus( SHAMAN_ELEMENTAL, T21, B4 ) )
    {
      t21_4pc = new frost_shock_overload_t( player );
      add_child( t21_4pc );
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p()->buff.icefury->value();

    m *= 1.0 + p()->buff.t21_2pc_elemental->stack_value();

    return m;
  }

  void execute() override
  {
    if ( p()->buff.icefury->up() )
    {
      maelstrom_gain = player->find_spell( 190493 )->effectN( 9 ).resource( RESOURCE_MAELSTROM );
    }

    shaman_spell_t::execute();

    maelstrom_gain = 0.0;

    p()->buff.icefury->decrement();

    p()->buff.t21_2pc_elemental->expire();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( t21_4pc && rng().roll( p()->sets->set( SHAMAN_ELEMENTAL, T21, B4 )->effectN( 1 ).percent() ) )
    {
      t21_4pc->base_dd_min = t21_4pc->base_dd_max = state->result_amount;
      t21_4pc->set_target( state->target );
      t21_4pc->execute();
    }
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "wind_shear", player, player->find_specialization_spell( "Wind Shear" ), options_str )
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

// Totemic Mastery Spell ====================================================

struct totem_mastery_t : public shaman_spell_t
{
  totem_mastery_t( shaman_t* player, const std::string& options_str )
    : shaman_spell_t( "totem_mastery", player, player->talent.totem_mastery, options_str )
  {
    harmful = may_crit = callbacks = may_miss = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.resonance_totem->trigger();
    p()->buff.storm_totem->trigger();
    p()->buff.ember_totem->trigger();

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      p()->buff.tailwind_totem_ele->trigger();
    }
    else if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->buff.tailwind_totem_enh->trigger();
    }
  }
};

// Healing Surge Spell ======================================================

struct healing_surge_t : public shaman_heal_t
{
  healing_surge_t( shaman_t* player, const std::string& options_str )
    : shaman_heal_t( player, player->find_specialization_spell( "Healing Surge" ), options_str )
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
    : shaman_heal_t( player, player->find_specialization_spell( "Chain Heal" ), options_str )
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
    regen_type = REGEN_DISABLED;
  }

  virtual void summon( timespan_t = timespan_t::zero() ) override;
  virtual void dismiss( bool expired = false ) override;

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

  virtual double composite_spell_hit() const override
  {
    return owner->cache.spell_hit();
  }

  virtual double composite_spell_crit_chance() const override
  {
    return owner->cache.spell_crit_chance();
  }

  virtual double composite_spell_power( school_e school ) const override
  {
    return owner->cache.spell_power( school );
  }

  virtual double composite_spell_power_multiplier() const override
  {
    return owner->composite_spell_power_multiplier();
  }

  virtual expr_t* create_expression( const std::string& name ) override
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

  virtual void execute() override
  {
    shaman_spell_t::execute();
    totem_pet->summon( totem_duration );
  }

  virtual expr_t* create_expression( const std::string& name ) override
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
  virtual const char* name() const override
  {
    return "totem_pulse";
  }
  virtual void execute() override
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

  virtual void execute() override
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
// Shaman Custom Buff implementation
// ==========================================================================

void ascendance_buff_t::ascendance( attack_t* mh, attack_t* oh, timespan_t lvb_cooldown )
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
        util::fprintf( stderr, "Ascendance %s time_to_hit=%f", player->main_hand_attack->name(),
                       time_to_hit.total_seconds() );
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
      player->main_hand_attack->execute_event->reschedule( time_to_hit );
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
          util::fprintf( stderr, "Ascendance %s time_to_hit=%f", player->off_hand_attack->name(),
                         time_to_hit.total_seconds() );
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
        player->off_hand_attack->execute_event->reschedule( time_to_hit );
      }
    }
  }
  // Elemental simply changes the Lava Burst cooldown, Lava Beam replacement
  // will be handled by action list and ready() in Chain Lightning / Lava
  // Beam
  else if ( player->specialization() == SHAMAN_ELEMENTAL )
  {
    if ( lava_burst )
    {
      lava_burst->cooldown->duration = lvb_cooldown;
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

  ascendance( p->ascendance_mh, p->ascendance_oh, timespan_t::zero() );
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

  timespan_t lvbcd;
  lvbcd = lava_burst ? lava_burst->data().charge_cooldown() : timespan_t::zero();

  ascendance( p->melee_mh, p->melee_oh, lvbcd );
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
    : buff_t( p, "flametongue", p->find_specialization_spell( "Flametongue" )->effectN( 3 ).trigger() ), p( p )
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

action_t* shaman_t::create_action( const std::string& name, const std::string& options_str )
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
  if ( name == "ghost_wolf" )
    return new ghost_wolf_t( this, options_str );
  if ( name == "lightning_bolt" )
    return new lightning_bolt_t( this, options_str );
  if ( name == "wind_shear" )
    return new wind_shear_t( this, options_str );

  // elemental
  if ( name == "chain_lightning" )
    return new chain_lightning_t( this, options_str );
  if ( name == "earth_elemental" )
    return new earth_elemental_t( this, options_str );
  if ( name == "earth_shock" )
    return new earth_shock_t( this, options_str );
  if ( name == "earthquake" )
    return new earthquake_t( this, options_str );
  if ( name == "elemental_blast" )
    return new elemental_blast_t( this, options_str );
  if ( name == "fire_elemental" )
    return new fire_elemental_t( this, options_str );
  if ( name == "flame_shock" )
    return new flame_shock_t( this, options_str );
  if ( name == "frost_shock" )
    return new frost_shock_t( this, options_str );
  if ( name == "icefury" )
    return new icefury_t( this, options_str );
  if ( name == "lava_beam" )
    return new lava_beam_t( this, options_str );
  if ( name == "lava_burst" )
    return new lava_burst_t( this, options_str );
  if ( name == "liquid_magma_totem" )
    return new shaman_totem_t( "liquid_magma_totem", this, options_str, talent.liquid_magma_totem );
  if ( name == "storm_elemental" )
    return new storm_elemental_t( this, options_str );
  if ( name == "stormkeeper" )
    return new stormkeeper_t( this, options_str );
  if ( name == "thunderstorm" )
    return new thunderstorm_t( this, options_str );
  if ( name == "totem_mastery" )
    return new totem_mastery_t( this, options_str );

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
  if ( name == "frostbrand" )
    return new frostbrand_t( this, options_str );
  if ( name == "fury_of_air" )
    return new fury_of_air_t( this, options_str );
  if ( name == "lava_lash" )
    return new lava_lash_t( this, options_str );
  if ( name == "lightning_shield" )
    return new lightning_shield_t( this, options_str );
  if ( name == "rockbiter" )
    return new rockbiter_t( this, options_str );
  if ( name == "spirit_walk" )
    return new spirit_walk_t( this, options_str );
  if ( name == "stormstrike" )
    return new stormstrike_t( this, options_str );
  if ( name == "sundering" )
    return new sundering_t( this, options_str );
  if ( name == "windstrike" )
    return new windstrike_t( this, options_str );

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

pet_t* shaman_t::create_pet( const std::string& pet_name, const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  if ( pet_name == "primal_fire_elemental" )
    return new pet::fire_elemental_t( this, false );
  if ( pet_name == "greater_fire_elemental" )
    return new pet::fire_elemental_t( this, true );
  if ( pet_name == "ember_elemental" )
    return new pet::ember_elemental_t( this, true );
  if ( pet_name == "spark_elemental" )
    return new pet::spark_elemental_t( this, true );
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
      for ( size_t i = 0; i < pet.guardian_fire_elemental.size(); ++i )
      {
        pet.guardian_fire_elemental[ i ] = new pet::fire_elemental_t( this, true );
      }
    }

    if ( find_action( "earth_elemental" ) )
    {
      pet.guardian_earth_elemental = create_pet( "greater_earth_elemental" );
    }

    if ( talent.storm_elemental->ok() && find_action( "storm_elemental" ) )
    {
      for ( size_t i = 0; i < pet.guardian_storm_elemental.size(); ++i )
      {
        pet.guardian_storm_elemental[ i ] = new pet::storm_elemental_t( this, true );
      }
    }
  }

  if ( azerite.echo_of_the_elementals.ok() )
  {
    if ( !talent.storm_elemental->ok() )
      pet.guardian_ember_elemental = create_pet( "ember_elemental" );
    if ( talent.storm_elemental->ok() )
      pet.guardian_spark_elemental = create_pet( "spark_elemental" );
  }

  if ( talent.liquid_magma_totem->ok() && find_action( "liquid_magma_totem" ) )
  {
    create_pet( "liquid_magma_totem" );
  }

  if ( find_action( "capacitor_totem" ) )
  {
    create_pet( "capacitor_totem" );
  }

  if ( specialization() == SHAMAN_ENHANCEMENT && find_action( "feral_spirit" ) && !talent.elemental_spirits->ok() )
  {
    for ( size_t i = 0; i < pet.spirit_wolves.size(); i++ )
    {
      pet.spirit_wolves[ i ] = new pet::spirit_wolf_t( this );
    }
  }

  if ( find_action( "feral_spirit" ) && talent.elemental_spirits->ok() )
  {
    pet.elemental_wolves[ 0 ] = new pet::frost_wolf_t( this );
    pet.elemental_wolves[ 1 ] = new pet::frost_wolf_t( this );
    pet.elemental_wolves[ 2 ] = new pet::fire_wolf_t( this );
    pet.elemental_wolves[ 3 ] = new pet::fire_wolf_t( this );
    pet.elemental_wolves[ 4 ] = new pet::lightning_wolf_t( this );
    pet.elemental_wolves[ 5 ] = new pet::lightning_wolf_t( this );
  }
}

// shaman_t::create_expression ==============================================

expr_t* shaman_t::create_expression( const std::string& name )
{
  std::vector<std::string> splits = util::string_split( name, "." );

  if ( util::str_compare_ci( splits[ 0 ], "feral_spirit" ) )
  {
    if ( talent.elemental_spirits->ok() && pet.elemental_wolves[ 0 ] == nullptr )
    {
      return expr_t::create_constant( name, 0 );
    }
    else if ( !talent.elemental_spirits->ok() && pet.spirit_wolves[ 0 ] == nullptr )
    {
      return expr_t::create_constant( name, 0 );
    }

    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( name, [this]() {
        if ( talent.elemental_spirits->ok() )
        {
          return range::find_if( pet.elemental_wolves, []( const pet_t* p ) { return !p->is_sleeping(); } ) !=
                 pet.elemental_wolves.end();
        }
        else
        {
          return range::find_if( pet.spirit_wolves, []( const pet_t* p ) { return !p->is_sleeping(); } ) !=
                 pet.spirit_wolves.end();
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

      return make_fn_expr( name, [this, &max_remains_fn]() {
        if ( talent.elemental_spirits->ok() )
        {
          auto it = std::max_element( pet.elemental_wolves.begin(), pet.elemental_wolves.end(), max_remains_fn );
          return ( *it )->expiration ? ( *it )->expiration->remains().total_seconds() : 0;
        }
        else
        {
          auto it = std::max_element( pet.spirit_wolves.begin(), pet.spirit_wolves.end(), max_remains_fn );
          return ( *it )->expiration ? ( *it )->expiration->remains().total_seconds() : 0;
        }
      } );
    }
  }

  return player_t::create_expression( name );
}

// shaman_t::create_actions =================================================

void shaman_t::create_actions()
{
  if ( talent.lightning_shield->ok() )
  {
    action.lightning_shield = new lightning_shield_damage_t( this );
  }

  if ( talent.crashing_storm->ok() )
  {
    action.crashing_storm = new crashing_storm_damage_t( this );
  }

  if ( talent.earthen_rage->ok() )
  {
    action.earthen_rage = new earthen_rage_spell_t( this );
  }

  if ( talent.searing_assault->ok() )
  {
    action.searing_assault = new searing_assault_t( this );
  }

  // Always create the Fury of Air damage action so spell_targets.fury_of_air works with or without
  // the talent
  action.fury_of_air = new fury_of_air_aoe_t( this );

  player_t::create_actions();
}

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();

  add_option( opt_uint( "stormlash_targets", stormlash_targets ) );
  add_option( opt_bool( "raptor_glyph", raptor_glyph ) );
}

// shaman_t::copy_from =====================================================

void shaman_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  shaman_t* p = debug_cast<shaman_t*>( source );

  stormlash_targets = p->stormlash_targets;
  raptor_glyph      = p->raptor_glyph;
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
  spec.critical_strikes   = find_specialization_spell( "Critical Strikes" );
  spec.dual_wield         = find_specialization_spell( "Dual Wield" );
  spec.enhancement_shaman = find_specialization_spell( "Enhancement Shaman" );
  spec.feral_spirit_2     = find_specialization_spell( 231723 );
  spec.flametongue        = find_specialization_spell( "Flametongue" );
  spec.frostbrand         = find_specialization_spell( "Frostbrand" );
  spec.maelstrom_weapon   = find_specialization_spell( "Maelstrom Weapon" );
  spec.stormbringer       = find_specialization_spell( "Stormbringer" );
  spec.stormlash          = find_specialization_spell( "Stormlash" );
  spec.windfury           = find_specialization_spell( "Windfury" );

  // Restoration
  spec.ancestral_awakening = find_specialization_spell( "Ancestral Awakening" );
  spec.ancestral_focus     = find_specialization_spell( "Ancestral Focus" );
  spec.purification        = find_specialization_spell( "Purification" );
  spec.resurgence          = find_specialization_spell( "Resurgence" );
  spec.riptide             = find_specialization_spell( "Riptide" );
  spec.tidal_waves         = find_specialization_spell( "Tidal Waves" );

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
  talent.ascendance    = find_talent_spell( "Ascendance" );
  talent.static_charge = find_talent_spell( "Static Charge" );
  talent.totem_mastery = find_talent_spell( "Totem Mastery" );

  // Elemental
  talent.earthen_rage         = find_talent_spell( "Earthen Rage" );
  talent.echo_of_the_elements = find_talent_spell( "Echo of the Elements" );
  talent.elemental_blast      = find_talent_spell( "Elemental Blast" );

  talent.aftershock       = find_talent_spell( "Aftershock" );
  talent.call_the_thunder = find_talent_spell( "Call the Thunder" );
  // talent.totem_mastery          = find_talent_spell( "Totem Mastery" );

  talent.master_of_the_elements = find_talent_spell( "Master of the Elements" );
  talent.storm_elemental        = find_talent_spell( "Storm Elemental" );
  talent.liquid_magma_totem     = find_talent_spell( "Liquid Magma Totem" );

  talent.surge_of_power      = find_talent_spell( "Surge of Power" );
  talent.primal_elementalist = find_talent_spell( "Primal Elementalist" );
  talent.icefury             = find_talent_spell( "Icefury" );

  talent.unlimited_power = find_talent_spell( "Unlimited Power" );
  talent.stormkeeper     = find_talent_spell( "Stormkeeper" );

  // Enhancement
  talent.boulderfist      = find_talent_spell( "Boulderfist" );
  talent.hot_hand         = find_talent_spell( "Hot Hand" );
  talent.lightning_shield = find_talent_spell( "Lightning Shield" );

  talent.landslide      = find_talent_spell( "Landslide" );
  talent.forceful_winds = find_talent_spell( "Forceful Winds" );
  // talent.totem_mastery  = find_talent_spell( "Totem Mastery" );

  talent.spirit_wolf   = find_talent_spell( "Spirit Wolf" );
  talent.earth_shield  = find_talent_spell( "Earth Shield" );
  talent.static_charge = find_talent_spell( "Static Charge" );

  talent.searing_assault = find_talent_spell( "Searing Assault" );
  talent.hailstorm       = find_talent_spell( "Hailstorm" );
  talent.overcharge      = find_talent_spell( "Overcharge" );

  talent.crashing_storm = find_talent_spell( "Crashing Storm" );
  talent.fury_of_air    = find_talent_spell( "Fury of Air" );
  talent.sundering      = find_talent_spell( "Sundering" );

  talent.elemental_spirits = find_talent_spell( "Elemental Spirits" );
  talent.earthen_spike     = find_talent_spell( "Earthen Spike" );

  // Restoration
  talent.gust_of_wind = find_talent_spell( "Gust of Wind" );

  //
  // Azerite traits
  //
  // Elemental
  azerite.echo_of_the_elementals = find_azerite_spell( "Echo of the Elementals" );
  azerite.igneous_potential      = find_azerite_spell( "Igneous Potential" );
  azerite.lava_shock             = find_azerite_spell( "Lava Shock" );
  azerite.tectonic_thunder       = find_azerite_spell( "Tectonic Thunder" );

  // Enhancement
  azerite.lightning_conduit = find_azerite_spell( "Lightning Conduit" );
  azerite.primal_primer     = find_azerite_spell( "Primal Primer" );
  azerite.roiling_storm     = find_azerite_spell( "Roiling Storm" );
  azerite.strength_of_earth = find_azerite_spell( "Strength of Earth" );
  azerite.thunderaans_fury  = find_azerite_spell( "Thunderaan's Fury" );

  // Shared
  azerite.ancestral_resonance = find_azerite_spell( "Ancestral Resonance" );
  azerite.natural_harmony     = find_azerite_spell( "Natural Harmony" );
  azerite.synapse_shock       = find_azerite_spell( "Synapse Shock" );

  //
  // Misc spells
  //
  spell.resurgence           = find_spell( 101033 );
  spell.maelstrom_melee_gain = find_spell( 187890 );

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

  if ( specialization() == SHAMAN_ELEMENTAL && talent.call_the_thunder->ok() )
  {
    resources.base[ RESOURCE_MAELSTROM ] += talent.call_the_thunder->effectN( 2 ).base_value();
  }

  if ( spec.enhancement_shaman->ok() )
    resources.base[ RESOURCE_MAELSTROM ] += spec.enhancement_shaman->effectN( 6 ).base_value();

  // if ( specialization() == SHAMAN_ENHANCEMENT )
  //  ready_type = READY_TRIGGER;
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
    if ( talent.storm_elemental->ok() )
    {
      auto it = range::find_if( pet.guardian_storm_elemental, []( const pet_t* p ) { return p && !p->is_sleeping(); } );

      return it != pet.guardian_storm_elemental.end();
    }
    else
    {
      auto it = range::find_if( pet.guardian_fire_elemental, []( const pet_t* p ) { return p && !p->is_sleeping(); } );

      return it != pet.guardian_fire_elemental.end();
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

void shaman_t::trigger_ancestral_resonance( const action_state_t* state )
{
  if ( !azerite.ancestral_resonance.ok() )
    return;

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
    if ( attack->may_proc_ability_procs )
    {
      buff.ancestral_resonance->trigger();
    }
  }

  if ( spell )
  {
    if ( !spell->background )
    {
      buff.ancestral_resonance->trigger();
    }
  }
}

void shaman_t::trigger_natural_harmony( const action_state_t* state )
{
  if ( !azerite.natural_harmony.ok() )
    return;

  if ( !state->action->harmful )
    return;

  if ( state->result_amount <= 0 )
    return;

  auto school = state->action->get_school();

  if ( dbc::is_school( school, SCHOOL_FIRE ) )
  {
    buff.natural_harmony_fire->trigger();
  }

  if ( dbc::is_school( school, SCHOOL_NATURE ) )
  {
    buff.natural_harmony_nature->trigger();
  }

  if ( dbc::is_school( school, SCHOOL_FROST ) )
  {
    buff.natural_harmony_frost->trigger();
  }
}

void shaman_t::trigger_strength_of_earth( const action_state_t* state )
{
  if ( !azerite.strength_of_earth.ok() )
    return;
  if ( state->action->background )
    return;

  if ( !buff.strength_of_earth->up() )
    return;

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

  if ( attack && attack->may_proc_strength_of_earth )
  {
    strength_of_earth->set_target( state->target );
    strength_of_earth->execute();
    buff.strength_of_earth->decrement();
  }
  else if ( spell && spell->may_proc_strength_of_earth )
  {
    strength_of_earth->set_target( state->target );
    strength_of_earth->execute();
    buff.strength_of_earth->decrement();
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

  if ( !buff.flametongue->up() )
  {
    return;
  }

  buff.hot_hand->trigger();
  attack->proc_hh->occur();
}

void shaman_t::trigger_primal_primer( const action_state_t* state )
{
  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr && "Primal primer called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );

  if ( !azerite.primal_primer.enabled() )
  {
    return;
  }

  if ( !attack->may_proc_primal_primer )
  {
    return;
  }

  if ( !state->result_amount )
  {
    return;
  }

  if ( !buff.flametongue->up() )
  {
    return;
  }

  get_target_data( state->target )->debuff.primal_primer->trigger();
  attack->proc_pp->occur();
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
  if ( buff.tailwind_totem_enh )
  {
    proc_chance *= 1.0 + buff.tailwind_totem_enh->value();
  }

  if ( buff.thunderaans_fury->up() )
  {
    proc_chance = proc_chance * ( 1.0 + buff.thunderaans_fury->value() );
  }

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

void shaman_t::trigger_searing_assault( const action_state_t* state )
{
  assert( debug_cast<shaman_spell_t*>( state->action ) != nullptr && "Searing Assault called on invalid action type" );

  if ( !talent.searing_assault->ok() )
  {
    return;
  }

  action.searing_assault->set_target( state->target );
  action.searing_assault->execute();
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

  if ( !buff.flametongue->up() )
    return;

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
  if ( !buff.lightning_shield->up() )
  {
    return;
  }

  if ( !buff.lightning_shield_overcharge->up() )
  {
    return;
  }

  if ( !state->action->result_is_hit( state->result ) )
  {
    return;
  }

  if ( state->result_amount <= 0 )
  {
    return;
  }

  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_lightning_shield )
  {
    return;
  }

  action.lightning_shield->set_target( state->target );
  action.lightning_shield->execute();
  attack->proc_ls->occur();
}

void shaman_t::trigger_hailstorm( const action_state_t* state )
{
  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr && "Hailstorm called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_frostbrand )
  {
    return;
  }

  if ( !talent.hailstorm->ok() )
  {
    return;
  }

  if ( !buff.frostbrand->up() )
  {
    return;
  }

  if ( buff.ghost_wolf->check() )
  {
    return;
  }

  hailstorm->set_target( state->target );
  hailstorm->schedule_execute();
  attack->proc_fb->occur();
}

// shaman_t::init_buffs =====================================================

void shaman_t::create_buffs()
{
  player_t::create_buffs();

  //
  // Shared
  //
  buff.ascendance      = new ascendance_buff_t( this );
  buff.resonance_totem = new resonance_totem_buff_t( this );
  buff.storm_totem     = new storm_totem_buff_t( this );
  buff.ember_totem     = new ember_totem_buff_t( this );
  buff.ghost_wolf      = make_buff( this, "ghost_wolf", find_class_spell( "Ghost Wolf" ) );

  // Apply Azerite Trait Ancestral Resonance to Bloodlust
  if ( azerite.ancestral_resonance.ok() )
  {
    buffs.bloodlust->buff_duration =
        timespan_t::from_seconds( azerite.ancestral_resonance.spell_ref().effectN( 2 ).base_value() );
  }
  //
  // Elemental
  //
  if ( specialization() == SHAMAN_ELEMENTAL )
  {
    buff.tailwind_totem_ele = new tailwind_totem_buff_ele_t( this );
  }
  buff.elemental_blast_crit = make_buff<stat_buff_t>( this, "elemental_blast_critical_strike", find_spell( 118522 ) );
  buff.elemental_blast_crit->set_max_stack( 1 );
  buff.elemental_blast_haste = make_buff<stat_buff_t>( this, "elemental_blast_haste", find_spell( 173183 ) );
  buff.elemental_blast_haste->set_max_stack( 1 );
  buff.elemental_blast_mastery = make_buff<stat_buff_t>( this, "elemental_blast_mastery", find_spell( 173184 ) );
  buff.elemental_blast_mastery->set_max_stack( 1 );
  buff.lava_surge = make_buff( this, "lava_surge", find_spell( 77762 ) )
                        ->set_activated( false )
                        ->set_chance( 1.0 );  // Proc chance is handled externally
  buff.stormkeeper = make_buff( this, "stormkeeper", talent.stormkeeper )
                         ->set_cooldown( timespan_t::zero() );  // Handled by the action

  buff.surge_of_power =
      make_buff( this, "surge_of_power", talent.surge_of_power )->set_duration( find_spell( 285514 )->duration() );

  buff.icefury = make_buff( this, "icefury", talent.icefury )
                     ->set_cooldown( timespan_t::zero() )  // Handled by the action
                     ->set_default_value( talent.icefury->effectN( 3 ).percent() );

  buff.earthen_rage = make_buff( this, "earthen_rage", find_spell( 170377 ) )
                          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                          ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
                          ->set_tick_behavior( buff_tick_behavior::REFRESH )
                          ->set_tick_callback( [this]( buff_t*, int, const timespan_t& ) {
                            assert( action.earthen_rage );
                            action.earthen_rage->set_target( recent_target );
                            action.earthen_rage->execute();
                          } );

  buff.master_of_the_elements = make_buff( this, "master_of_the_elements", find_spell( 260734 ) )
                                    ->set_default_value( find_spell( 260734 )->effectN( 1 ).percent() );
  buff.unlimited_power = make_buff( this, "unlimited_power", find_spell( 272737 ) )
                             ->add_invalidate( CACHE_HASTE )
                             ->set_default_value( find_spell( 272737 )->effectN( 1 ).percent() )
                             ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buff.wind_gust = make_buff( this, "wind_gust", find_spell( 263806 ) )
                       ->set_default_value( find_spell( 263806 )->effectN( 1 ).percent() );

  // Tier
  buff.t21_2pc_elemental =
      make_buff( this, "earthen_strength", sets->set( SHAMAN_ELEMENTAL, T21, B2 )->effectN( 1 ).trigger() )
          ->set_trigger_spell( sets->set( SHAMAN_ELEMENTAL, T21, B2 ) )
          ->set_default_value( sets->set( SHAMAN_ELEMENTAL, T21, B2 )->effectN( 1 ).trigger()->effectN( 1 ).percent() );

  // Azerite Traits - Shared
  buff.natural_harmony_fire = make_buff<stat_buff_t>( this, "natural_harmony_fire", find_spell( 279028 ) )
                                  ->add_stat( STAT_CRIT_RATING, azerite.natural_harmony.value() );

  buff.natural_harmony_frost = make_buff<stat_buff_t>( this, "natural_harmony_frost", find_spell( 279029 ) )
                                   ->add_stat( STAT_MASTERY_RATING, azerite.natural_harmony.value() );

  buff.natural_harmony_nature = make_buff<stat_buff_t>( this, "natural_harmony_nature", find_spell( 279033 ) )
                                    ->add_stat( STAT_HASTE_RATING, azerite.natural_harmony.value() );

  buff.synapse_shock = make_buff<stat_buff_t>( this, "synapse_shock", find_spell( 277960 ) )
                           ->add_stat( STAT_INTELLECT, azerite.synapse_shock.value() )
                           ->add_stat( STAT_AGILITY, azerite.synapse_shock.value() )
                           ->set_trigger_spell( azerite.synapse_shock );

  buff.ancestral_resonance = new ancestral_resonance_buff_t( this );

  // Azerite Traits - Ele
  buff.lava_shock = make_buff( this, "lava_shock", azerite.lava_shock )
                        ->set_default_value( azerite.lava_shock.value() )
                        ->set_trigger_spell( find_spell( 273453 ) )
                        ->set_max_stack( find_spell( 273453 )->max_stacks() )
                        ->set_duration( find_spell( 273453 )->duration() );

  buff.tectonic_thunder = make_buff( this, "tectonic_thunder", find_spell( 286976 ) )
                              ->set_default_value( find_spell( 286976 )->effectN( 1 ).percent() );

  // Azerite Traits - Enh
  buff.roiling_storm_buff_driver = new roiling_storm_buff_driver_t( this );
  buff.strength_of_earth         = new strength_of_earth_buff_t( this );
  buff.thunderaans_fury          = new thunderaans_fury_buff_t( this );

  //
  // Enhancement
  //
  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    buff.tailwind_totem_enh = new tailwind_totem_buff_enh_t( this );
  }
  buff.lightning_shield            = new lightning_shield_buff_t( this );
  buff.lightning_shield_overcharge = new lightning_shield_overcharge_buff_t( this );
  buff.flametongue                 = new flametongue_buff_t( this );
  buff.forceful_winds              = make_buff<buff_t>( this, "forceful_winds", find_spell( 262652 ) )
                            ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
                            ->set_default_value( find_spell( 262652 )->effectN( 1 ).percent() );

  buff.landslide        = new landslide_buff_t( this );
  buff.icy_edge         = new icy_edge_buff_t( this );
  buff.molten_weapon    = new molten_weapon_buff_t( this );
  buff.crackling_surge  = new crackling_surge_buff_t( this );
  buff.gathering_storms = new gathering_storms_buff_t( this );

  buff.crash_lightning = make_buff( this, "crash_lightning", find_spell( 187878 ) );
  buff.feral_spirit =
      make_buff( this, "t17_4pc_melee", sets->set( SHAMAN_ENHANCEMENT, T17, B4 )->effectN( 1 ).trigger() )
          ->set_cooldown( timespan_t::zero() );
  buff.hot_hand =
      make_buff( this, "hot_hand", talent.hot_hand->effectN( 1 ).trigger() )->set_trigger_spell( talent.hot_hand );
  buff.spirit_walk = make_buff( this, "spirit_walk", find_specialization_spell( "Spirit Walk" ) );
  buff.frostbrand  = make_buff( this, "frostbrand", spec.frostbrand )
                        ->set_period( timespan_t::zero() )
                        ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
  buff.stormbringer = make_buff( this, "stormbringer", find_spell( 201846 ) )
                          ->set_activated( false )
                          ->set_max_stack( find_spell( 201846 )->initial_stacks() );
  buff.fury_of_air = make_buff( this, "fury_of_air", talent.fury_of_air )
                         ->set_tick_callback( [this]( buff_t* b, int, const timespan_t& ) {
                           action.fury_of_air->set_target( target );
                           action.fury_of_air->execute();

                           double actual_amount =
                               resource_loss( RESOURCE_MAELSTROM, talent.fury_of_air->powerN( 1 ).cost() );
                           gain.fury_of_air->add( RESOURCE_MAELSTROM, actual_amount );

                           // If the actor reaches 0 maelstrom after the tick cost, cancel the buff. Otherwise, keep
                           // going. This allows "one extra tick" with less than 3 maelstrom, which seems to mirror in
                           // game behavior. In game, the buff only fades after the next tick (i.e., it has a one second
                           // delay), but modeling that seems pointless. Gaining maelstrom during that delay will not
                           // change the outcome of the fading.
                           if ( resources.current[ RESOURCE_MAELSTROM ] == 0 )
                           {
                             // Separate the expiration event to happen immediately after tick processing
                             make_event( *sim, timespan_t::zero(), [b]() { b->expire(); } );
                           }
                         } );

  //
  // Restoration
  //
  buff.spiritwalkers_grace =
      make_buff( this, "spiritwalkers_grace", find_specialization_spell( "Spiritwalker's Grace" ) );
  buff.tidal_waves =
      make_buff( this, "tidal_waves", spec.tidal_waves->ok() ? find_spell( 53390 ) : spell_data_t::not_found() );
}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gain.aftershock                  = get_gain( "Aftershock" );
  gain.ascendance                  = get_gain( "Ascendance" );
  gain.resurgence                  = get_gain( "resurgence" );
  gain.feral_spirit                = get_gain( "Feral Spirit" );
  gain.fire_elemental              = get_gain( "Fire Elemental" );
  gain.spirit_of_the_maelstrom     = get_gain( "Spirit of the Maelstrom" );
  gain.resonance_totem             = get_gain( "Resonance Totem" );
  gain.lightning_shield_overcharge = get_gain( "Lightning Shield Overcharge" );
  gain.forceful_winds              = get_gain( "Forceful Winds" );
  // Note, Fury of Air gain pointer is initialized in the base action
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
          ? "battle_potion_of_intellect"
          : ( true_level > 100 )
                ? "prolonged_power"
                : ( true_level >= 90 )
                      ? "draenic_intellect"
                      : ( true_level >= 85 ) ? "jade_serpent" : ( true_level >= 80 ) ? "volcanic" : "disabled";

  std::string enhance_pot =
      ( true_level > 110 )
          ? "battle_potion_of_agility"
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
          ? "flask_of_endless_fathoms"
          : ( true_level > 100 )
                ? "whispered_pact"
                : ( true_level >= 90 )
                      ? "greater_draenic_intellect_flask"
                      : ( true_level >= 85 ) ? "warm_sun" : ( true_level >= 80 ) ? "draconic_mind" : "disabled";

  std::string enhance_flask =
      ( true_level > 110 )
          ? "currents"
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
                                   ? "bountiful_captains_feast"
                                   : ( true_level > 100 )
                                         ? "lemon_herb_filet"
                                         : ( true_level > 90 )
                                               ? "pickled_eel"
                                               : ( true_level >= 90 )
                                                     ? "mogu_fish_stew"
                                                     : ( true_level >= 80 ) ? "seafood_magnifique_feast" : "disabled";

  std::string enhance_food = ( true_level > 110 )
                                 ? "bountiful_captains_feast"
                                 : ( true_level > 100 )
                                       ? "lemon_herb_filet"
                                       : ( true_level > 90 )
                                             ? "buttered_sturgeon"
                                             : ( true_level >= 90 )
                                                   ? "sea_mist_rice_noodles"
                                                   : ( true_level >= 80 ) ? "seafood_magnifique_feast" : "disabled";

  return specialization() == SHAMAN_ENHANCEMENT ? enhance_food : elemental_food;
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
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default" );
  action_priority_list_t* single_target =
      get_action_priority_list( "single_target", "Single Target Action Priority List" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe", "Multi target action priority list" );

  // Flask
  precombat->add_action( "flask" );

  // Food
  precombat->add_action( "food" );

  // Rune
  precombat->add_action( "augmentation" );

  // Snapshot stats
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  precombat->add_talent( this, "Totem Mastery" );
  precombat->add_action( this, "Earth Elemental", "if=!talent.primal_elementalist.enabled" );
  precombat->add_action( this, "Stormkeeper",
                         "if=talent.stormkeeper.enabled&(raid_event.adds.count<3|raid_event.adds.in>50)",
                         "Use Stormkeeper precombat unless some adds will spawn soon." );
  precombat->add_action( this, "Fire Elemental", "if=!talent.storm_elemental.enabled" );
  precombat->add_talent( this, "Storm Elemental", "if=talent.storm_elemental.enabled" );
  precombat->add_action( "potion" );
  precombat->add_talent( this, "Elemental Blast", "if=talent.elemental_blast.enabled" );
  precombat->add_action( this, "Lava Burst", "if=!talent.elemental_blast.enabled&spell_targets.chain_lightning<3" );
  precombat->add_action( this, "Chain Lightning", "if=spell_targets.chain_lightning>2" );

  // All Shamans Bloodlust by default
  def->add_action( this, "Bloodlust", "if=azerite.ancestral_resonance.enabled",
                   "Cast Bloodlust manually if the Azerite Trait Ancestral Resonance is present." );

  // In-combat potion
  def->add_action(
      "potion,if=expected_combat_length-time<30|cooldown.fire_elemental.remains>120|cooldown.storm_"
      "elemental.remains>120",
      "In-combat potion is preferentially linked to your Elemental, unless combat will end shortly" );

  // "Default" APL controlling logic flow to specialized sub-APLs
  def->add_action( this, "Wind Shear", "", "Interrupt of casts." );
  def->add_talent( this, "Totem Mastery", "if=talent.totem_mastery.enabled&buff.resonance_totem.remains<2" );
  def->add_action( this, "Fire Elemental", "if=!talent.storm_elemental.enabled" );
  def->add_talent( this, "Storm Elemental",
                   "if=talent.storm_elemental.enabled&(!talent.icefury.enabled|!buff.icefury.up&!cooldown.icefury.up)&("
                   "!talent.ascendance.enabled|!cooldown.ascendance.up)" );
  def->add_action(
      this, "Earth Elemental",
      "if=!talent.primal_elementalist.enabled|talent.primal_elementalist.enabled&(cooldown.fire_elemental.remains<120&!"
      "talent.storm_elemental.enabled|cooldown.storm_elemental.remains<120&talent.storm_elemental.enabled)" );
  // On-use items
  def->add_action( "use_items" );
  // Racials
  def->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  def->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
  def->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  def->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );

  // Pick APL to run
  def->add_action(
      "run_action_list,name=aoe,if=active_enemies>2&(spell_targets.chain_lightning>2|spell_targets.lava_beam>2)" );
  def->add_action( "run_action_list,name=single_target" );

  // Aoe APL
  aoe->add_talent( this, "Stormkeeper", "if=talent.stormkeeper.enabled" );
  aoe->add_action( this, "Flame Shock",
                   "target_if=refreshable&(spell_targets.chain_lightning<(5-!talent.totem_mastery.enabled)|!talent."
                   "storm_elemental.enabled&(cooldown.fire_elemental.remains>(120+14*spell_haste)|cooldown.fire_"
                   "elemental.remains<(24-14*spell_haste)))&(!talent.storm_elemental.enabled|cooldown.storm_elemental."
                   "remains<120|spell_targets.chain_lightning=3&buff.wind_gust.stack<14)",
                   "Spread Flame Shock in <= 4 target fights, but not during SE uptime,"
                   "unless you're fighting 3 targets and have less than 14 Wind Gust stacks." );
  aoe->add_talent( this, "Ascendance",
                   "if=talent.ascendance.enabled&(talent.storm_elemental.enabled&cooldown.storm_elemental.remains<120&"
                   "cooldown.storm_elemental.remains>15|!talent.storm_elemental.enabled)&(!talent.icefury.enabled|!"
                   "buff.icefury.up&!cooldown.icefury.up)" );
  aoe->add_talent( this, "Liquid Magma Totem", "if=talent.liquid_magma_totem.enabled" );
  aoe->add_action(
      this, "Earthquake",
      "if=!talent.master_of_the_elements.enabled|buff.stormkeeper.up|maelstrom>=(100-4*spell_targets.chain_lightning)|"
      "buff.master_of_the_elements.up|spell_targets.chain_lightning>3",
      "Try to game Earthquake with Master of the Elements buff when fighting 3 targets. Don't overcap Maelstrom!" );
  aoe->add_action( this, "Chain Lightning", "if=buff.stormkeeper.remains<3*gcd*buff.stormkeeper.stack",
                   "Make sure you don't lose a Stormkeeper." );
  aoe->add_action( this, "Lava Burst",
                   "if=buff.lava_surge.up&spell_targets.chain_lightning<4&(!talent.storm_elemental.enabled|cooldown."
                   "storm_elemental.remains<120)&dot.flame_shock.ticking",
                   "Only cast Lava Burst on three targets if it is an instant and Storm Elemental is NOT active." );
  aoe->add_talent( this, "Icefury", "if=spell_targets.chain_lightning<4&!buff.ascendance.up" );
  aoe->add_action( this, "Frost Shock", "if=spell_targets.chain_lightning<4&buff.icefury.up&!buff.ascendance.up" );
  aoe->add_talent( this, "Elemental Blast",
                   "if=talent.elemental_blast.enabled&spell_targets.chain_lightning<4&(!talent.storm_elemental."
                   "enabled|cooldown.storm_elemental.remains<120)",
                   "Use Elemental Blast against up to 3 targets as long as Storm Elemental is not active." );
  aoe->add_action( this, "Lava Beam", "if=talent.ascendance.enabled" );
  aoe->add_action( this, "Chain Lightning" );
  aoe->add_action( this, "Lava Burst", "moving=1,if=talent.ascendance.enabled" );
  aoe->add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
  aoe->add_action( this, "Frost Shock", "moving=1" );

  // Single target APL
  single_target->add_action(
      this, "Flame Shock",
      "target_if=(!ticking|talent.storm_elemental.enabled&cooldown.storm_elemental.remains<2*gcd|dot.flame_shock."
      "remains<=gcd|"
      "talent.ascendance.enabled&dot.flame_shock.remains<(cooldown.ascendance.remains+buff.ascendance.duration)&"
      "cooldown.ascendance.remains<4&(!talent.storm_elemental.enabled|talent.storm_elemental.enabled&cooldown.storm_"
      "elemental.remains<120))&(buff.wind_gust.stack<14|azerite.igneous_potential.rank>=2|buff.lava_surge.up|!buff."
      "bloodlust.up)&!buff.surge_of_power.up",
      "Ensure FS is active unless you have 14 or more stacks of Wind Gust from Storm Elemental. (Edge case: upcoming "
      "Asc but active SE; don't )" );
  single_target->add_talent( this, "Ascendance",
                             "if=talent.ascendance.enabled&(time>=60|buff.bloodlust.up)&cooldown.lava_burst.remains>0&("
                             "cooldown.storm_elemental.remains<120|!talent.storm_elemental.enabled)&(!talent.icefury."
                             "enabled|!buff.icefury.up&!cooldown.icefury.up)",
                             "Use Ascendance after you've spent all Lava Burst charges and only if neither Storm "
                             "Elemental nor Icefury are currently active." );
  single_target->add_talent(
      this, "Elemental Blast",
      "if=talent.elemental_blast.enabled&(talent.master_of_the_elements.enabled&buff.master_of_the_elements.up&"
      "maelstrom<60|!talent.master_of_the_elements.enabled)&(!(cooldown.storm_elemental.remains>120&talent.storm_"
      "elemental.enabled)|azerite.natural_harmony.rank=3&buff.wind_gust.stack<14)",
      "Don't use Elemental Blast if you could cast a Master of the Elements empowered Earth Shock instead. Don't "
      "cast Elemental Blast during Storm Elemental unless you have 3x Natural Harmony. But in this case stop using "
      "Elemental Blast once you reach 14 stacks of Wind Gust." );
  single_target->add_talent(
      this, "Stormkeeper",
      "if=talent.stormkeeper.enabled&(raid_event.adds.count<3|raid_event.adds.in>50)&(!talent.surge_of_power.enabled|"
      "buff.surge_of_power.up|maelstrom>=44)",
      "Keep SK for large or soon add waves. Unless you have Surge of Power, in which case you want to double buff "
      "Lightning Bolt by pooling Maelstrom beforehand. Example sequence: 100MS, ES, SK, LB, LvB, ES, LB" );
  single_target->add_talent( this, "Liquid Magma Totem",
                             "if=talent.liquid_magma_totem.enabled&(raid_event.adds.count<3|raid_event.adds.in>50)" );
  single_target->add_action(
      this, "Lightning Bolt",
      "if=buff.stormkeeper.up&spell_targets.chain_lightning<2&(azerite.lava_shock.rank*buff.lava_shock.stack)<26&(buff."
      "master_of_the_elements.up&!talent.surge_of_power.enabled|buff.surge_of_power.up)",
      "Combine Stormkeeper with Master of the Elements or Surge of Power unless you have the Lava Shock trait and "
      "multiple stacks." );
  single_target->add_action(
      this, "Earthquake",
      "if=(spell_targets.chain_lightning>1|azerite.tectonic_thunder.rank>=3&!talent.surge_of_power.enabled&azerite."
      "lava_shock.rank<1)&azerite.lava_shock.rank*buff.lava_shock.stack<(36+3*azerite.tectonic_thunder.rank*spell_"
      "targets.chain_lightning)&(!talent.surge_of_power.enabled|!dot.flame_shock.refreshable|cooldown.storm_elemental."
      "remains>120)&(!talent.master_of_the_elements.enabled|buff.master_of_the_elements.up|cooldown.lava_burst.remains>"
      "0&maelstrom>=92+30*talent.call_the_thunder.enabled)",
      "Use Earthquake versus 2 targets, unless you have Lava Shock. Use Earthquake versus 1 target if you have "
      "Tectonic Thunder 3 times and NO Surge of Power enabled and NO Lava Shock." );
  single_target->add_action(
      this, "Earth Shock",
      "if=!buff.surge_of_power.up&talent.master_of_the_elements.enabled&(buff.master_of_the_elements.up|cooldown.lava_"
      "burst.remains>0&maelstrom>=92+30*talent.call_the_thunder.enabled|spell_targets.chain_lightning<2&(azerite.lava_"
      "shock.rank*buff.lava_shock.stack<26)&buff.stormkeeper.up&cooldown.lava_burst.remains<=gcd)",
      "Cast Earth Shock with Master of the Elements talent but no active Surge of Power buff, and active Stormkeeper "
      "buff and Lava Burst coming off CD within the next GCD, and either active Master of the Elements buff, or no "
      "available Lava Burst while near MS cap, or single target and multiple Lava Shock traits and many stacks." );
  single_target->add_action(
      this, "Earth Shock",
      "if=!talent.master_of_the_elements.enabled&!(azerite.igneous_potential.rank>2&buff.ascendance.up)&(buff."
      "stormkeeper.up|maelstrom>=90+30*talent.call_the_thunder.enabled|!(cooldown.storm_elemental.remains>120&talent."
      "storm_elemental.enabled)&expected_combat_length-time-cooldown.storm_elemental.remains-150*floor((expected_"
      "combat_length-time-cooldown.storm_elemental.remains)%150)>=30*(1+(azerite.echo_of_the_elementals.rank>=2)))",
      "You know what? I had some short explanation here once. But then the condition grew, and I had to split the one "
      "Earth Shock line into four...so you have to deal with this abomination now: Cast Earth Shock without Master of "
      "the Elements talent, and without having triple Igneous Potential and active Ascendance, and active Stormkeeper "
      "buff or near MS cap, or Storm Elemental is inactive, and we can't expect to get an additional use of Storm "
      "Elemental in the remaining fight from Surge of Power." );
  single_target->add_action(
      this, "Earth Shock",
      "if=talent.surge_of_power.enabled&!buff.surge_of_power.up&cooldown.lava_burst.remains<=gcd&(!talent.storm_"
      "elemental.enabled&!(cooldown.fire_elemental.remains>120)|talent.storm_elemental.enabled&!(cooldown.storm_"
      "elemental.remains>120))",
      "Use Earth Shock if Surge of Power is talented, but neither it nor a DPS Elemental is active "
      "at the moment, and Lava Burst is ready or will be ready within the next GCD." );

  single_target->add_action(
      this, "Lightning Bolt",
      "if=cooldown.storm_elemental.remains>120&talent.storm_elemental.enabled&(azerite.igneous_potential.rank<2|!buff."
      "lava_surge.up&buff.bloodlust.up)",
      "Spam Lightning Bolts during Storm Elemental duration, if you don't have Igneous Potential or have it only once, "
      "and don't use Lightning Bolt during Bloodlust if you have a Lava Surge Proc." );
  single_target->add_action( this, "Lightning Bolt",
                             "if=(buff.stormkeeper.remains<1.1*gcd*buff.stormkeeper.stack|buff.stormkeeper.up&"
                             "buff.master_of_the_elements.up)",
                             "Cast Lightning Bolt regardless of the previous condition if you'd lose a Stormkeeper "
                             "stack or have Stormkeeper and Master of the Elements active." );
  single_target->add_action(
      this, "Frost Shock",
      "if=talent.icefury.enabled&talent.master_of_the_elements.enabled&buff.icefury.up&buff.master_of_the_elements.up",
      "Use Frost Shock with Icefury and Master of the Elements." );
  single_target->add_action( this, "Lava Burst", "if=buff.ascendance.up" );
  single_target->add_action( this, "Flame Shock", "target_if=refreshable&active_enemies>1&buff.surge_of_power.up",
                             "Utilize Surge of Power to spread Flame Shock if multiple enemies are present." );
  single_target->add_action(
      this, "Lava Burst",
      "if=talent.storm_elemental.enabled&cooldown_react&buff.surge_of_power.up&(expected_combat_length-time-cooldown."
      "storm_elemental.remains-150*floor((expected_combat_length-time-cooldown.storm_elemental.remains)%150)<30*(1+("
      "azerite.echo_of_the_elementals.rank>=2))|(1.16*(expected_combat_length-time)-cooldown.storm_elemental.remains-"
      "150*floor((1.16*(expected_combat_length-time)-cooldown.storm_elemental.remains)%150))<(expected_combat_length-"
      "time-cooldown.storm_elemental.remains-150*floor((expected_combat_length-time-cooldown.storm_elemental.remains)%"
      "150)))",
      "Use Lava Burst with Surge of Power if the last potential usage of Storm Elemental hasn't a full duration OR "
      "if you could get another usage of the DPS Elemental if the remaining fight was 16% longer." );
  single_target->add_action(
      this, "Lava Burst",
      "if=!talent.storm_elemental.enabled&cooldown_react&buff.surge_of_power.up&(expected_combat_length-time-cooldown."
      "fire_elemental.remains-150*floor((expected_combat_length-time-cooldown.fire_elemental.remains)%150)<30*(1+("
      "azerite.echo_of_the_elementals.rank>=2))|(1.16*(expected_combat_length-time)-cooldown.fire_elemental.remains-"
      "150*floor((1.16*(expected_combat_length-time)-cooldown.fire_elemental.remains)%150))<(expected_combat_length-"
      "time-cooldown.fire_elemental.remains-150*floor((expected_combat_length-time-cooldown.fire_elemental.remains)%"
      "150)))",
      "Use Lava Burst with Surge of Power if the last potential usage of Fire Elemental hasn't a full duration OR "
      "if you could get another usage of the DPS Elemental if the remaining fight was 16% longer." );

  single_target->add_action( this, "Lightning Bolt", "if=buff.surge_of_power.up" );
  single_target->add_action( this, "Lava Burst", "if=cooldown_react&!talent.master_of_the_elements.enabled" );
  single_target->add_talent(
      this, "Icefury",
      "if=talent.icefury.enabled&!(maelstrom>75&cooldown.lava_burst.remains<=0)&(!talent.storm_"
      "elemental.enabled|cooldown.storm_elemental.remains<120)",
      "Slightly game Icefury buff to hopefully buff some empowered Frost Shocks with Master of the Elements." );
  single_target->add_action( this, "Lava Burst", "if=cooldown_react&charges>talent.echo_of_the_elements.enabled" );
  single_target->add_action(
      this, "Frost Shock", "if=talent.icefury.enabled&buff.icefury.up&buff.icefury.remains<1.1*gcd*buff.icefury.stack",
      "Slightly delay using Icefury empowered Frost Shocks to empower them with Master of the Elements too." );
  single_target->add_action( this, "Lava Burst", "if=cooldown_react" );
  single_target->add_action( this, "Flame Shock", "target_if=refreshable&!buff.surge_of_power.up",
                             "Don't accidentally use Surge of Power with Flame Shock during single target." );
  single_target->add_talent( this, "Totem Mastery",
                             "if=talent.totem_mastery.enabled&(buff.resonance_totem.remains<6|(buff.resonance_totem."
                             "remains<(buff.ascendance.duration+"
                             "cooldown.ascendance.remains)&cooldown.ascendance.remains<15))" );
  single_target->add_action( this, "Frost Shock",
                             "if=talent.icefury.enabled&buff.icefury.up&(buff.icefury.remains<gcd*4*buff.icefury.stack|"
                             "buff.stormkeeper.up|!talent.master_of_the_elements.enabled)" );
  single_target->add_action( this, "Chain Lightning",
                             "if=buff.tectonic_thunder.up&!buff.stormkeeper.up&spell_targets.chain_lightning>1" );
  single_target->add_action( this, "Lightning Bolt" );
  single_target->add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
  single_target->add_action( this, "Flame Shock", "moving=1,if=movement.distance>6" );
  single_target->add_action( this, "Frost Shock", "moving=1", "Frost Shock is our movement filler." );
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

  action_priority_list_t* precombat        = get_action_priority_list( "precombat" );
  action_priority_list_t* def              = get_action_priority_list( "default" );
  action_priority_list_t* cds              = get_action_priority_list( "cds" );
  action_priority_list_t* priority         = get_action_priority_list( "priority" );
  action_priority_list_t* maintenance      = get_action_priority_list( "maintenance" );
  action_priority_list_t* freezerburn_core = get_action_priority_list( "freezerburn_core" );
  action_priority_list_t* default_core     = get_action_priority_list( "default_core" );
  action_priority_list_t* filler           = get_action_priority_list( "filler" );
  action_priority_list_t* opener           = get_action_priority_list( "opener" );
  action_priority_list_t* asc              = get_action_priority_list( "asc" );

  // Flask
  precombat->add_action( "flask" );
  // Food
  precombat->add_action( "food" );
  // Rune
  precombat->add_action( "augmentation" );
  // Snapshot stats
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  // Precombat potion
  precombat->add_action( "potion" );
  // Lightning shield can be turned on pre-combat
  precombat->add_talent( this, "Lightning Shield" );
  // All Shamans Bloodlust and Wind Shear by default
  def->add_action( this, "Wind Shear" );
  def->add_action(
      "variable,name=cooldown_sync,value=(talent.ascendance.enabled&(buff.ascendance.up|cooldown.ascendance.remains>50)"
      ")"
      "|(!talent.ascendance.enabled&(feral_spirit.remains>5|cooldown.feral_spirit.remains>50))",
      "Attempt to sync racial cooldowns with Ascendance or Feral Spirits, or use on cooldown if saving them will "
      "result "
      "in significant cooldown waste" );
  def->add_action(
      "variable,name=furyCheck_SS,value=maelstrom>=(talent.fury_of_air.enabled*(6+action.stormstrike.cost))",
      "Do not use a maelstrom-costing ability if it will bring you to 0 maelstrom and cancel fury of air." );
  def->add_action(
      "variable,name=furyCheck_LL,value=maelstrom>=(talent.fury_of_air.enabled*(6+action.lava_lash.cost))" );
  def->add_action(
      "variable,name=furyCheck_CL,value=maelstrom>=(talent.fury_of_air.enabled*(6+action.crash_lightning.cost))" );
  def->add_action(
      "variable,name=furyCheck_FB,value=maelstrom>=(talent.fury_of_air.enabled*(6+action.frostbrand.cost))" );
  def->add_action(
      "variable,name=furyCheck_ES,value=maelstrom>=(talent.fury_of_air.enabled*(6+action.earthen_spike.cost))" );
  def->add_action( "variable,name=furyCheck_LB,value=maelstrom>=(talent.fury_of_air.enabled*(6+40))" );
  def->add_action(
      "variable,name=OCPool,value=(active_enemies>1|(cooldown.lightning_bolt.remains>=2*gcd))",
      "Attempt to pool maelstrom so you'll be able to cast a fully-powered lightning bolt as soon as it's available "
      "when fighting one target." );
  def->add_action(
      "variable,name=OCPool_SS,value=(variable.OCPool|maelstrom>=(talent.overcharge.enabled*(40+action.stormstrike."
      "cost)))" );
  def->add_action(
      "variable,name=OCPool_LL,value=(variable.OCPool|maelstrom>=(talent.overcharge.enabled*(40+action.lava_lash.cost))"
      ")" );
  def->add_action(
      "variable,name=OCPool_CL,value=(variable.OCPool|maelstrom>=(talent.overcharge.enabled*(40+action.crash_lightning."
      "cost)))" );
  def->add_action(
      "variable,name=OCPool_FB,value=(variable.OCPool|maelstrom>=(talent.overcharge.enabled*(40+action.frostbrand.cost)"
      "))" );
  def->add_action(
      "variable,name=CLPool_LL,value=active_enemies=1|maelstrom>=(action.crash_lightning.cost+action.lava_lash.cost)",
      "Attempt to pool maelstrom for Crash Lightning if multiple targets are present." );
  def->add_action(
      "variable,name=CLPool_SS,value=active_enemies=1|maelstrom>=(action.crash_lightning.cost+action.stormstrike."
      "cost)" );
  def->add_action(
      "variable,name=freezerburn_enabled,value=(talent.hot_hand.enabled&talent.hailstorm.enabled&azerite.primal_primer."
      "enabled)" );
  def->add_action(
      "variable,name=rockslide_enabled,value=(!variable.freezerburn_enabled&(talent.boulderfist.enabled&talent."
      "landslide.enabled"
      "&azerite.strength_of_earth.enabled))" );

  // Turn on auto-attack first thing
  def->add_action( "auto_attack" );
  // On-use actions
  def->add_action( "use_items" );

  def->add_action( "call_action_list,name=opener" );
  def->add_action( "call_action_list,name=asc,if=buff.ascendance.up" );
  def->add_action( "call_action_list,name=priority" );
  def->add_action( "call_action_list,name=maintenance,if=active_enemies<3" );
  def->add_action( "call_action_list,name=cds" );
  def->add_action( "call_action_list,name=freezerburn_core,if=variable.freezerburn_enabled" );
  def->add_action( "call_action_list,name=default_core,if=!variable.freezerburn_enabled" );
  def->add_action( "call_action_list,name=maintenance,if=active_enemies>=3" );
  def->add_action( "call_action_list,name=filler" );

  opener->add_action( this, "Rockbiter", "if=maelstrom<15&time<gcd" );

  asc->add_action( this, "Crash Lightning", "if=!buff.crash_lightning.up&active_enemies>1&variable.furyCheck_CL" );
  asc->add_action( this, "Rockbiter", "if=talent.landslide.enabled&!buff.landslide.up&charges_fractional>1.7" );
  asc->add_action( this, "Windstrike" );

  priority->add_action( this, "Crash Lightning",
                        "if=active_enemies>=(8-(talent.forceful_winds.enabled*3))"
                        "&variable.freezerburn_enabled&variable.furyCheck_CL" );
  priority->add_action( this, "Lava Lash",
                        "if=azerite.primal_primer.rank>=2&debuff.primal_primer.stack=10"
                        "&active_enemies=1&variable.freezerburn_enabled&variable.furyCheck_LL" );
  priority->add_action( this, "Crash Lightning",
                        "if=!buff.crash_lightning.up&active_enemies>1"
                        "&variable.furyCheck_CL" );
  priority->add_talent( this, "Fury of Air",
                        "if=!buff.fury_of_air.up&maelstrom>=20"
                        "&spell_targets.fury_of_air_damage>=(1+variable.freezerburn_enabled)" );
  priority->add_talent( this, "Fury of Air",
                        "if=buff.fury_of_air.up&&spell_targets.fury_of_air_damage<(1+variable.freezerburn_enabled)" );
  priority->add_talent( this, "Totem Mastery", "if=buff.resonance_totem.remains<=2*gcd" );
  priority->add_talent( this, "Sundering", "if=active_enemies>=3" );
  priority->add_action( this, "Rockbiter", "if=talent.landslide.enabled&!buff.landslide.up&charges_fractional>1.7" );
  priority->add_action(
      this, "Frostbrand",
      "if=(azerite.natural_harmony.enabled&buff.natural_harmony_frost.remains<=2*gcd)"
      "&talent.hailstorm.enabled&variable.furyCheck_FB",
      "With Natural Harmony, elevate the priority of elemental attacks in order to maintain the buffs when "
      "they're about to expire." );
  priority->add_action( this, "Flametongue",
                        "if=(azerite.natural_harmony.enabled&buff.natural_harmony_fire.remains<=2*gcd)" );
  priority->add_action( this, "Rockbiter",
                        "if=(azerite.natural_harmony.enabled&buff.natural_harmony_nature.remains<=2*gcd)"
                        "&maelstrom<70" );

  maintenance->add_action( this, "Flametongue", "if=!buff.flametongue.up" );
  maintenance->add_action( this, "Frostbrand",
                           "if=talent.hailstorm.enabled&!buff.frostbrand.up&variable.furyCheck_FB" );

  cds->add_action( this, "Bloodlust", "if=azerite.ancestral_resonance.enabled",
                   "Cast Bloodlust manually if the Azerite Trait Ancestral Resonance is present." );
  cds->add_action( "berserking,if=variable.cooldown_sync" );
  cds->add_action( "blood_fury,if=variable.cooldown_sync" );
  cds->add_action( "fireblood,if=variable.cooldown_sync" );
  cds->add_action( "ancestral_call,if=variable.cooldown_sync" );
  cds->add_action(
      "potion,if=buff.ascendance.up|!talent.ascendance.enabled&feral_spirit.remains>5|target.time_to_die<=60",
      "Attempt to sync your DPS potion with a cooldown, unless the target is about to die." );
  cds->add_action( this, "Feral Spirit" );
  cds->add_talent( this, "Ascendance", "if=cooldown.strike.remains>0" );
  cds->add_action( this, "Earth Elemental" );

  freezerburn_core->add_action( this, "Lava Lash",
                                "target_if=max:debuff.primal_primer.stack,if=azerite.primal_primer.rank>=2"
                                "&debuff.primal_primer.stack=10&variable.furyCheck_LL&variable.CLPool_LL" );
  freezerburn_core->add_talent( this, "Earthen Spike", "if=variable.furyCheck_ES" );
  freezerburn_core->add_action( this, "Stormstrike",
                                "cycle_targets=1,if=active_enemies>1&azerite.lightning_conduit.enabled"
                                "&!debuff.lightning_conduit.up&variable.furyCheck_SS" );
  freezerburn_core->add_action( this, "Stormstrike",
                                "if=buff.stormbringer.up|(active_enemies>1&buff.gathering_storms.up"
                                "&variable.furyCheck_SS)" );
  freezerburn_core->add_action( this, "Crash Lightning", "if=active_enemies>=3&variable.furyCheck_CL" );
  freezerburn_core->add_action( this, "Lightning Bolt",
                                "if=talent.overcharge.enabled&active_enemies=1"
                                "&variable.furyCheck_LB&maelstrom>=40" );
  freezerburn_core->add_action( this, "Lava Lash",
                                "if=azerite.primal_primer.rank>=2&debuff.primal_primer.stack>7"
                                "&variable.furyCheck_LL&variable.CLPool_LL" );
  freezerburn_core->add_action( this, "Stormstrike", "if=variable.OCPool_SS&variable.furyCheck_SS&variable.CLPool_SS" );
  freezerburn_core->add_action( this, "Lava Lash", "if=debuff.primal_primer.stack=10&variable.furyCheck_LL" );

  default_core->add_talent( this, "Earthen Spike", "if=variable.furyCheck_ES" );
  default_core->add_action( this, "Stormstrike",
                            "cycle_targets=1,if=active_enemies>1&azerite.lightning_conduit.enabled"
                            "&!debuff.lightning_conduit.up&variable.furyCheck_SS" );
  default_core->add_action( this, "Stormstrike",
                            "if=buff.stormbringer.up|(active_enemies>1&buff.gathering_storms.up"
                            "&variable.furyCheck_SS)" );
  default_core->add_action( this, "Crash Lightning", "if=active_enemies>=3&variable.furyCheck_CL" );
  default_core->add_action( this, "Lightning Bolt",
                            "if=talent.overcharge.enabled&active_enemies=1"
                            "&variable.furyCheck_LB&maelstrom>=40" );
  default_core->add_action( this, "Stormstrike", "if=variable.OCPool_SS&variable.furyCheck_SS" );

  filler->add_talent( this, "Sundering" );
  filler->add_action( this, "Crash Lightning",
                      "if=talent.forceful_winds.enabled&active_enemies>1&variable.furyCheck_CL" );
  filler->add_action( this, "Flametongue", "if=talent.searing_assault.enabled" );
  filler->add_action( this, "Lava Lash",
                      "if=!azerite.primal_primer.enabled&talent.hot_hand.enabled&buff.hot_hand.react" );
  filler->add_action( this, "Crash Lightning", "if=active_enemies>1&variable.furyCheck_CL" );
  filler->add_action( this, "Rockbiter", "if=maelstrom<70&!buff.strength_of_earth.up" );
  filler->add_action( this, "Crash Lightning", "if=talent.crashing_storm.enabled&variable.OCPool_CL" );
  filler->add_action( this, "Lava Lash", "if=variable.OCPool_LL&variable.furyCheck_LL" );
  filler->add_action( this, "Rockbiter" );
  filler->add_action( this, "Frostbrand",
                      "if=talent.hailstorm.enabled&buff.frostbrand.remains<4.8+gcd"
                      "&variable.furyCheck_FB" );
  filler->add_action( this, "Flametongue" );
}

// shaman_t::init_actions ===================================================

void shaman_t::init_action_list()
{
  if ( !( primary_role() == ROLE_ATTACK && specialization() == SHAMAN_ENHANCEMENT ) &&
       !( primary_role() == ROLE_SPELL && specialization() == SHAMAN_ELEMENTAL ) )
  {
    if ( !quiet )
      sim->errorf( "Player %s's role (%s) or spec(%s) isn't supported yet.", name(),
                   util::role_type_string( primary_role() ), util::specialization_string( specialization() ) );
    quiet = true;
    return;
  }

#ifdef NDEBUG  // Only restrict on release builds.
  // Restoration isn't supported atm
  if ( specialization() == SHAMAN_RESTORATION && primary_role() == ROLE_HEAL )
  {
    if ( !quiet )
      sim->errorf( "Restoration Shaman healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }
#endif

  // After error checks, initialize secondary actions for various things
  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    windfury_mh = new windfury_attack_t( "windfury_attack", this, find_spell( 25504 ), &( main_hand_weapon ) );
    if ( off_hand_weapon.type != WEAPON_NONE )
    {
      windfury_oh = new windfury_attack_t( "windfury_attack_oh", this, find_spell( 33750 ), &( off_hand_weapon ) );
    }
    flametongue = new flametongue_weapon_spell_t( "flametongue_attack", this, &( off_hand_weapon ) );

    if ( talent.hailstorm->ok() )
    {
      hailstorm = new hailstorm_attack_t( "hailstorm", this, &( main_hand_weapon ) );
    }

    icy_edge = new icy_edge_attack_t( "icy_edge", this, &( main_hand_weapon ) );

    action.molten_weapon_dot = new molten_weapon_dot_t( this );

    // Azerite cached actions
    if ( azerite.lightning_conduit.ok() )
      lightning_conduit = new lightning_conduit_zap_t( this );

    if ( azerite.strength_of_earth.ok() )
      strength_of_earth = new strength_of_earth_t( this );
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
      // 1) The profile does not have Glyph of Unleashed Lightning and is
      //    casting a Lightning Bolt (non-instant cast)
      // 2) The profile is casting Lava Burst (without Lava Surge)
      // 3) The profile is casting Chain Lightning
      // 4) The profile is casting Elemental Blast
      if ( ( executing->id == 51505 ) || ( executing->id == 421 ) || ( executing->id == 117014 ) )
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

  if ( buff.tailwind_totem_ele && buff.tailwind_totem_ele->up() )
    h *= 1.0 / ( 1.0 + buff.tailwind_totem_ele->check_value() );

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

  if ( buff.feral_spirit->up() )
    ms = std::max( buff.feral_spirit->data().effectN( 1 ).percent(), ms );

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

  if ( buff.tailwind_totem_ele && buff.tailwind_totem_ele->up() )
  {
    h *= 1.0 / ( 1.0 + buff.tailwind_totem_ele->check_value() );
  }

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

  if ( dbc::is_school( school, SCHOOL_FIRE ) && buff.molten_weapon->up() )
  {
    for ( int x = 1; x <= buff.molten_weapon->check(); x++ )
    {
      m *= 1.0 + buff.molten_weapon->default_value;
    }
  }
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

  if ( azerite.roiling_storm.ok() )
  {
    buff.roiling_storm_buff_driver->trigger();
    buff.stormbringer->trigger( buff.stormbringer->max_stack() );
  }
}

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  player_t::reset();

  lava_surge_during_lvb = false;
  for ( auto& elem : counters )
    elem->reset();
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
        name_str = report::action_decorator_t( a ).decorate();
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

  virtual void html_customsection( report::sc_html_stream& os ) override
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

  player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
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

    p->buffs.exhaustion = buff_creator_t( p, "exhaustion", p->find_spell( 57723 ) ).max_stack( 1 ).quiet( true );
  }

  void static_init() const override
  {
    // Shaman Leyshock's Grand Compendium basic hooks

    // Totem Mastery
    expansion::bfa::register_leyshocks_trigger( 210643, STAT_CRIT_RATING );
    // Lightning Bolt Overload (damage)
    expansion::bfa::register_leyshocks_trigger( 45284, STAT_CRIT_RATING );
    // Frost Shock
    expansion::bfa::register_leyshocks_trigger( 196840, STAT_CRIT_RATING );
    // Flame Shock
    expansion::bfa::register_leyshocks_trigger( 188389, STAT_HASTE_RATING );
    // Chain Lightning
    expansion::bfa::register_leyshocks_trigger( 188443, STAT_HASTE_RATING );
    // Earthquake rumbles
    expansion::bfa::register_leyshocks_trigger( 77478, STAT_MASTERY_RATING );
    // Stormkeeper
    expansion::bfa::register_leyshocks_trigger( 191634, STAT_MASTERY_RATING );
    // Liquid Magma Totem
    expansion::bfa::register_leyshocks_trigger( 192222, STAT_MASTERY_RATING );
    // Thunderstorm
    expansion::bfa::register_leyshocks_trigger( 51490, STAT_MASTERY_RATING );
    // Wind Shear
    expansion::bfa::register_leyshocks_trigger( 57994, STAT_MASTERY_RATING );
    // Chain Lightning Overload
    expansion::bfa::register_leyshocks_trigger( 45297, STAT_MASTERY_RATING );
    // Icefury
    expansion::bfa::register_leyshocks_trigger( 210714, STAT_MASTERY_RATING );
    // Ascendance (elemental)
    expansion::bfa::register_leyshocks_trigger( 114050, STAT_MASTERY_RATING );
    // Earth Elemental
    expansion::bfa::register_leyshocks_trigger( 198103, STAT_VERSATILITY_RATING );
    // Lava Surge buff
    expansion::bfa::register_leyshocks_trigger( 77762, STAT_MASTERY_RATING );
    // Earthquake (TODO: Something more complex with number of targets?)
    expansion::bfa::register_leyshocks_trigger( 61882, STAT_VERSATILITY_RATING );
    // Lightning Bolt (elemental)
    expansion::bfa::register_leyshocks_trigger( 188196, STAT_VERSATILITY_RATING );
    // Lava Burst Overload
    expansion::bfa::register_leyshocks_trigger( 77451, STAT_VERSATILITY_RATING );
    // Earthen Rage
    expansion::bfa::register_leyshocks_trigger( 170379, STAT_VERSATILITY_RATING );
    // Storm Elemental
    expansion::bfa::register_leyshocks_trigger( 192249, STAT_VERSATILITY_RATING );
    // Lava Beam
    expansion::bfa::register_leyshocks_trigger( 114074, STAT_VERSATILITY_RATING );
    // Lava Beam Overload
    expansion::bfa::register_leyshocks_trigger( 114738, STAT_VERSATILITY_RATING );
  }

  void register_hotfixes() const override
  {
    hotfix::register_effect( "Shaman", "2019-01-07",
                             "Incorrect Maelstrom generation value for Chain Lightning Overloads.",
                             723964 )  // spell 190493
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 2 )
        .verification_value( 3 );
  }

  virtual void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

}  // UNNAMED NAMESPACE

const module_t* module_t::shaman()
{
  static shaman_module_t m;
  return &m;
}

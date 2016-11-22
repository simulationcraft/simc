// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Shaman
// ==========================================================================

// Legion TODO
// Generic
// - Clean up the "may proc" stuff
// Elemental
// - Verification
// - Path of Flame spread mechanism (would be good to generalize this "nearby" spreading)
// - At what point does Greater Lightning Elemental start aoeing?

namespace { // UNNAMED NAMESPACE

/**
  Check_distance_targeting is only called when distance_targeting_enabled is true. Otherwise,
  available_targets is called.  The following code is intended to generate a target list that
  properly accounts for range from each target during chain lightning.  On a very basic level, it
  starts at the original target, and then finds a path that will hit 4 more, if possible.  The
  code below randomly cycles through targets until it finds said path, or hits the maximum amount
  of attempts, in which it gives up and just returns the current best path.  I wouldn't be
  terribly surprised if Blizz did something like this in game.
**/
static std::vector<player_t*> __check_distance_targeting( const action_t* action, std::vector< player_t* >& tl )
{
  sim_t* sim = action -> sim;
  player_t* target = action -> target;
  player_t* player = action -> player;
  double radius = action -> radius;
  int aoe = action -> aoe;

  player_t* last_chain; // We have to track the last target that it hit.
  last_chain = target;
  std::vector< player_t* > best_so_far; // Keeps track of the best chain path found so far, so we can use it if we give up.
  std::vector< player_t* > current_attempt;
  best_so_far.push_back( last_chain );
  current_attempt.push_back( last_chain );

  size_t num_targets = sim -> target_non_sleeping_list.size();
  size_t max_attempts = static_cast<size_t>( std::min( ( num_targets - 1.0 ) * 2.0 , 30.0 ) ); // With a lot of targets this can get pretty high. Cap it at 30.
  size_t local_attempts = 0, attempts = 0, chain_number = 1;
  std::vector<player_t*> targets_left_to_try( sim -> target_non_sleeping_list.data() ); // This list contains members of a vector that haven't been tried yet.
  auto position = std::find( targets_left_to_try.begin(), targets_left_to_try.end(), target );
  if ( position != targets_left_to_try.end() )
    targets_left_to_try.erase( position );

  std::vector<player_t*> original_targets( targets_left_to_try ); // This is just so we don't have to constantly remove the original target.

  bool stop_trying = false; // It's not you, it's me.

  while ( !stop_trying )
  {
    local_attempts = 0;
    attempts++;
    if ( attempts >= max_attempts )
      stop_trying = true;
    while ( targets_left_to_try.size() > 0 && local_attempts < num_targets * 2 )
    {
      player_t* possibletarget;
      size_t rng_target = static_cast<size_t>( sim -> rng().range( 0.0, ( static_cast<double>( targets_left_to_try.size() ) - 0.000001 ) ) );
      possibletarget = targets_left_to_try[rng_target];

      double distance_from_last_chain = last_chain -> get_player_distance( *possibletarget );
      if ( distance_from_last_chain <= radius + possibletarget -> combat_reach )
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
        if ( distance_from_last_chain > ( ( radius + possibletarget -> combat_reach ) * ( aoe - chain_number ) ) )
          targets_left_to_try.erase( targets_left_to_try.begin() + rng_target );
        local_attempts++; // Only count failures towards the limit-cap.
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
    last_chain = target;
    targets_left_to_try = original_targets;
    chain_number = 1;
  }

  if ( sim -> log )
    sim -> out_debug.printf( "%s Total attempts at finding path: %.3f - %.3f targets found - %s target is first chain",
      player -> name(), static_cast<double>(attempts), static_cast<double>( best_so_far.size() ), target -> name() );
  tl.swap( best_so_far );
  return tl;
}
typedef std::pair<std::string, simple_sample_data_with_min_max_t> data_t;
typedef std::pair<std::string, simple_sample_data_t> simple_data_t;

struct shaman_t;

enum totem_e { TOTEM_NONE = 0, TOTEM_AIR, TOTEM_EARTH, TOTEM_FIRE, TOTEM_WATER, TOTEM_MAX };
enum imbue_e { IMBUE_NONE = 0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE, FROSTBRAND_IMBUE, EARTHLIVING_IMBUE };

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
  } dot;

  struct debuffs
  {
    buff_t* earthen_spike;
    buff_t* lightning_rod;
    buff_t* storm_tempests; // 7.0 Legendary
  } debuff;

  struct heals
  {
    dot_t* riptide;
    dot_t* earthliving;
  } heal;

  shaman_td_t( player_t* target, shaman_t* p );

  shaman_t* actor() const
  { return debug_cast<shaman_t*>( source ); }
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
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    value += val;
    if ( last > timespan_t::min() )
      interval += ( sim -> current_time() - last ).total_seconds();
    last = sim -> current_time();
  }

  void reset()
  { last = timespan_t::min(); }

  double divisor() const
  {
    if ( ! sim -> debug && ! sim -> log && sim -> iterations > sim -> threads )
      return sim -> iterations - sim -> threads;
    else
      return std::min( sim -> iterations, sim -> threads );
  }

  double mean() const
  { return value / divisor(); }

  double interval_mean() const
  { return interval / divisor(); }

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
  bool       lava_surge_during_lvb;
  std::vector<counter_t*> counters;
  std::vector<player_t*> lightning_rods;
  int t18_4pc_elemental_counter;

  // Options
  unsigned stormlash_targets;
  bool     raptor_glyph;

  // Data collection for cooldown waste
  auto_dispose< std::vector<data_t*> > cd_waste_exec, cd_waste_cumulative;
  auto_dispose< std::vector<simple_data_t*> > cd_waste_iter;

  // Cached actions
  struct actions_t
  {
    std::array<action_t*, 2> unleash_doom;
    action_t* ancestral_awakening;
    action_t* lightning_strike;
    spell_t*  electrocute;
    action_t* volcanic_inferno;
    spell_t*  lightning_shield;
    spell_t*  earthen_rage;
    spell_t* crashing_storm;
    spell_t* doom_vortex_ll, * doom_vortex_lb;
    action_t* lightning_rod;
    action_t* ppsg; // Pristine Proto-Scale Girdle legendary dot
    action_t* storm_tempests; // Storm Tempests legendary damage spell
  } action;

  // Pets
  struct pets_t
  {
    pet_t* pet_fire_elemental;
    pet_t* guardian_fire_elemental;
    pet_t* pet_storm_elemental;
    pet_t* guardian_storm_elemental;
    pet_t* pet_earth_elemental;
    pet_t* guardian_earth_elemental;

    pet_t* guardian_greater_lightning_elemental;

    std::array<pet_t*, 2> spirit_wolves;
    std::array<pet_t*, 6> doom_wolves;
  } pet;

  const special_effect_t* furious_winds;

  // Constants
  struct
  {
    double matching_gear_multiplier;
    double haste_ancestral_swiftness;
  } constant;

  // Buffs
  struct
  {
    buff_t* ascendance;
    buff_t* echo_of_the_elements;
    buff_t* lava_surge;
    buff_t* liquid_magma;
    buff_t* lightning_shield;
    buff_t* shamanistic_rage;
    buff_t* spirit_walk;
    buff_t* spiritwalkers_grace;
    buff_t* tidal_waves;
    buff_t* focus_of_the_elements;
    buff_t* feral_spirit;

    stat_buff_t* elemental_blast_crit;
    stat_buff_t* elemental_blast_haste;
    stat_buff_t* elemental_blast_mastery;

    buff_t* flametongue;
    buff_t* frostbrand;
    buff_t* stormbringer;
    buff_t* crash_lightning;
    haste_buff_t* windsong;
    buff_t* boulderfist;
    buff_t* landslide;
    buff_t* doom_winds;
    buff_t* unleash_doom;
    haste_buff_t* wind_strikes;
    buff_t* gathering_storms;
    buff_t* ghost_wolf;
    buff_t* elemental_focus;
    buff_t* earth_surge;
    buff_t* icefury;
    buff_t* hot_hand;
    haste_buff_t* elemental_mastery;

    // Set bonuses
    stat_buff_t* t19_oh_8pc;
    haste_buff_t* t18_4pc_elemental;
    buff_t* t18_4pc_enhancement;

    // Legendary buffs
    buff_t* echoes_of_the_great_sundering;
    buff_t* emalons_charged_core;
    buff_t* spiritual_journey;
    buff_t* eotn_fire;
    buff_t* eotn_shock;
    buff_t* eotn_chill;

    // Artifact related buffs
    buff_t* stormkeeper;
    buff_t* static_overload;
    buff_t* power_of_the_maelstrom;

    // Totemic mastery
    buff_t* resonance_totem;
    buff_t* storm_totem;
    buff_t* ember_totem;
    haste_buff_t* tailwind_totem;

    // Stormlash
    stormlash_buff_t* stormlash;
  } buff;

  // Cooldowns
  struct
  {
    cooldown_t* ascendance;
    cooldown_t* elemental_focus;
    cooldown_t* fire_elemental;
    cooldown_t* feral_spirits;
    cooldown_t* lava_burst;
    cooldown_t* lava_lash;
    cooldown_t* storm_elemental;
    cooldown_t* strike;
  } cooldown;

  // Gains
  struct
  {
    gain_t* aftershock;
    gain_t* ascendance;
    gain_t* resurgence;
    gain_t* feral_spirit;
    gain_t* spirit_of_the_maelstrom;
    gain_t* resonance_totem;
    gain_t* wind_gust;
    gain_t* t18_2pc_elemental;
    gain_t* the_deceivers_blood_pact;
  } gain;

  // Tracked Procs
  struct
  {
    proc_t* lava_surge;
    proc_t* wasted_lava_surge;
    proc_t* windfury;

    proc_t* surge_during_lvb;
  } proc;

  struct
  {
    real_ppm_t* stormlash;
  } real_ppm;

  // Various legendary related values
  struct legendary_t
  {
  } legendary;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* mail_specialization;
    const spell_data_t* shaman;

    // Elemental
    const spell_data_t* chain_lightning_2; // 7.1 Chain Lightning additional 2 targets passive
    const spell_data_t* elemental_focus;
    const spell_data_t* elemental_fury;
    const spell_data_t* flame_shock_2; // 7.1 Flame Shock duration extension passive
    const spell_data_t* lava_burst_2; // 7.1 Lava Burst autocrit with FS passive
    const spell_data_t* lava_surge;
    const spell_data_t* spiritual_insight;

    // Enhancement
    const spell_data_t* critical_strikes;
    const spell_data_t* dual_wield;
    const spell_data_t* enhancement_shaman;
    const spell_data_t* feral_spirit_2; // 7.1 Feral Spirit Maelstrom gain passive
    const spell_data_t* flametongue;
    const spell_data_t* frostbrand;
    const spell_data_t* maelstrom_weapon;
    const spell_data_t* stormbringer;
    const spell_data_t* stormlash;
    const spell_data_t* windfury;

    // Restoration
    const spell_data_t* ancestral_awakening;
    const spell_data_t* ancestral_focus;
    const spell_data_t* meditation;
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
    const spell_data_t* ancestral_swiftness;
    const spell_data_t* ascendance;
    const spell_data_t* gust_of_wind;

    // Elemental
    const spell_data_t* path_of_flame;
    const spell_data_t* earthen_rage;
    const spell_data_t* totem_mastery;

    const spell_data_t* elemental_blast;
    const spell_data_t* echo_of_the_elements;

    const spell_data_t* elemental_fusion;
    const spell_data_t* primal_elementalist;
    const spell_data_t* icefury;

    const spell_data_t* elemental_mastery;
    const spell_data_t* storm_elemental;
    const spell_data_t* aftershock;

    const spell_data_t* liquid_magma_totem;
    const spell_data_t* lightning_rod;

    // Enhancement
    const spell_data_t* windsong;
    const spell_data_t* hot_hand;
    const spell_data_t* boulderfist;

    const spell_data_t* feral_lunge;

    const spell_data_t* lightning_shield;
    const spell_data_t* sundering;

    const spell_data_t* tempest;
    const spell_data_t* landslide;
    const spell_data_t* overcharge;

    const spell_data_t* hailstorm;
    const spell_data_t* crashing_storm;

    const spell_data_t* earthen_spike;
    const spell_data_t* fury_of_air;
    const spell_data_t* empowered_stormlash;
  } talent;

  // Artifact
  struct artifact_spell_data_t
  {
    // Elemental
    artifact_power_t stormkeeper;
    artifact_power_t surge_of_power;
    artifact_power_t call_the_thunder;
    artifact_power_t earthen_attunement;
    artifact_power_t the_ground_trembles;
    artifact_power_t lava_imbued;
    artifact_power_t volcanic_inferno;
    artifact_power_t static_overload;
    artifact_power_t electric_discharge;
    artifact_power_t master_of_the_elements;
    artifact_power_t molten_blast;
    artifact_power_t elementalist;
    artifact_power_t firestorm;
    artifact_power_t power_of_the_maelstrom;
    artifact_power_t fury_of_the_storms;
    artifact_power_t stormkeepers_power;

    // Enhancement
    artifact_power_t doom_winds;
    artifact_power_t unleash_doom;
    artifact_power_t raging_storms;
    artifact_power_t stormflurry;
    artifact_power_t hammer_of_storms;
    artifact_power_t forged_in_lava;
    artifact_power_t wind_strikes;
    artifact_power_t gathering_storms;
    artifact_power_t gathering_of_the_maelstrom;
    artifact_power_t doom_vortex;
    artifact_power_t spirit_of_the_maelstrom;
    artifact_power_t doom_wolves;
    artifact_power_t alpha_wolf;
    artifact_power_t earthshattering_blows;
    artifact_power_t weapons_of_the_elements;
    artifact_power_t wind_surge;
  } artifact;

  // Misc Spells
  struct
  {
    const spell_data_t* resurgence;
    const spell_data_t* eruption;
    const spell_data_t* maelstrom_melee_gain;
    const spell_data_t* fury_of_the_storms_driver;
  } spell;

  // Cached pointer for ascendance / normal white melee
  shaman_attack_t* melee_mh;
  shaman_attack_t* melee_oh;
  shaman_attack_t* ascendance_mh;
  shaman_attack_t* ascendance_oh;

  // Weapon Enchants
  shaman_attack_t* windfury_mh, * windfury_oh;
  shaman_spell_t*  flametongue;
  shaman_attack_t* hailstorm;

  shaman_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, SHAMAN, name, r ),
    lava_surge_during_lvb( false ),
    t18_4pc_elemental_counter( 0 ),
    stormlash_targets( 17 ), // Default to 2 tanks + 15 dps
    raptor_glyph( false ),
    action(),
    pet(),
    constant(),
    buff(),
    cooldown(),
    gain(),
    proc(),
    real_ppm(),
    legendary(),
    spec(),
    mastery(),
    talent(),
    spell()
  {
    range::fill( pet.spirit_wolves, nullptr );
    range::fill( pet.doom_wolves, nullptr );

    // Cooldowns
    cooldown.ascendance           = get_cooldown( "ascendance"            );
    cooldown.elemental_focus      = get_cooldown( "elemental_focus"       );
    cooldown.fire_elemental       = get_cooldown( "fire_elemental"        );
    cooldown.storm_elemental      = get_cooldown( "storm_elemental"       );
    cooldown.feral_spirits        = get_cooldown( "feral_spirit"          );
    cooldown.lava_burst           = get_cooldown( "lava_burst"            );
    cooldown.lava_lash            = get_cooldown( "lava_lash"             );
    cooldown.strike               = get_cooldown( "strike"                );

    melee_mh = nullptr;
    melee_oh = nullptr;
    ascendance_mh = nullptr;
    ascendance_oh = nullptr;

    // Weapon Enchants
    windfury_mh = nullptr;
    windfury_oh = nullptr;
    flametongue = nullptr;
    hailstorm   = nullptr;

    regen_type = REGEN_DISABLED;
  }

  virtual           ~shaman_t();

  // triggers
  void trigger_windfury_weapon( const action_state_t* );
  void trigger_flametongue_weapon( const action_state_t* );
  void trigger_hailstorm( const action_state_t* );
  void trigger_t17_2pc_elemental( int );
  void trigger_t17_4pc_elemental( int );
  void trigger_t18_4pc_elemental();
  void trigger_t19_oh_8pc( const action_state_t* );
  void trigger_stormbringer( const action_state_t* state );
  void trigger_elemental_focus( const action_state_t* state );
  void trigger_lightning_shield( const action_state_t* state );
  void trigger_earthen_rage( const action_state_t* state );
  void trigger_stormlash( const action_state_t* state );
  void trigger_doom_vortex( const action_state_t* state );
  void trigger_lightning_rod_damage( const action_state_t* state );
  void trigger_hot_hand( const action_state_t* state );

  // Character Definition
  void      init_spells() override;
  void      init_resources( bool force = false ) override;
  void      init_base_stats() override;
  void      init_scaling() override;
  void      create_buffs() override;
  bool      create_actions() override;
  void      create_options() override;
  void      init_gains() override;
  void      init_procs() override;

  // APL releated methods
  void      init_action_list() override;
  void      init_action_list_enhancement();
  void      init_action_list_elemental();
  std::string generate_bloodlust_options();

  void      init_rng() override;
  bool      init_special_effects() override;

  void      moving() override;
  void      invalidate_cache( cache_e c ) override;
  double    temporary_movement_modifier() const override;
  double    composite_melee_crit_chance() const override;
  double    composite_melee_haste() const override;
  double    composite_melee_speed() const override;
  double    composite_attack_power_multiplier() const override;
  double    composite_attribute_multiplier( attribute_e ) const override;
  double    composite_spell_crit_chance() const override;
  double    composite_spell_haste() const override;
  double    composite_spell_power( school_e school ) const override;
  double    composite_spell_power_multiplier() const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t* state ) const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  action_t* create_proc_action( const std::string& /* name */, const special_effect_t& ) override;
  pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() ) override;
  void      create_pets() override;
  expr_t* create_expression( action_t*, const std::string& name ) override;
  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void      arise() override;
  void      reset() override;
  void      merge( player_t& other ) override;

  void     datacollection_begin() override;
  void     datacollection_end() override;
  bool     has_t18_class_trinket() const override;

  target_specific_t<shaman_td_t> target_data;

  virtual shaman_td_t* get_target_data( player_t* target ) const override
  {
    shaman_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new shaman_td_t( target, const_cast<shaman_t*>(this) );
    }
    return td;
  }

  template <typename T_CONTAINER, typename T_DATA>
  T_CONTAINER* get_data_entry( const std::string& name, std::vector<T_DATA*>& entries )
  {
    for ( size_t i = 0; i < entries.size();i ++ )
    {
      if ( entries[ i ] -> first == name )
      {
        return &( entries[ i ] -> second );
      }
    }

    entries.push_back( new T_DATA( name, T_CONTAINER() ) );
    return &( entries.back() -> second );
  }
};

shaman_t::~shaman_t()
{
  range::dispose( counters );
}

counter_t::counter_t( shaman_t* p ) :
  sim( p -> sim ), value( 0 ), interval( 0 ), last( timespan_t::min() )
{
  p -> counters.push_back( this );
}

// ==========================================================================
// Shaman Custom Buff Declaration
// ==========================================================================

struct ascendance_buff_t : public buff_t
{
  action_t* lava_burst;

  ascendance_buff_t( shaman_t* p ) :
    buff_t( buff_creator_t( p, "ascendance", p -> talent.ascendance )
            .tick_callback( [ p ]( buff_t* b, int, const timespan_t& ) {
               double g = b -> data().effectN( 4 ).base_value();
               p -> resource_gain( RESOURCE_MAELSTROM, g, p -> gain.ascendance );
            } )
            .cd( timespan_t::zero() ) ), // Cooldown is handled by the action
    lava_burst( nullptr )
  { }

  void ascendance( attack_t* mh, attack_t* oh, timespan_t lvb_cooldown );
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

// Stormlash

struct stormlash_spell_t : public spell_t
{
  stormlash_spell_t( player_t* p ) : spell_t( "stormlash", p, p -> find_spell( 213307 ) )
  {
    background = true;
    callbacks = may_crit = may_miss = false;
  }

  void init() override
  {
    spell_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct damage_pool_t
{
  double damage;
  // Technically this implementation shares last proc between all of the pools, but keep it here in
  // case we complicate the model in the future.
  timespan_t last_proc;
  // Keep track of expiration so we can properly null it out in the (very rare) case where maximum
  // number of targets have a stormlash buff already
  event_t* expiration;

  damage_pool_t( player_t* pl, double d, std::vector<damage_pool_t*>& p, const timespan_t& duration );
};

struct stormlash_expiration_t : public player_event_t
{
  std::vector<damage_pool_t*>& damage_pool;
  damage_pool_t* pool;

  stormlash_expiration_t( player_t* pl, damage_pool_t* pool, std::vector<damage_pool_t*>& pools,
                          const timespan_t duration ) :
    player_event_t( *pl, duration ), damage_pool( pools ), pool( pool )
  {
  }

  void execute() override
  {
    // Yank out this pool from the list of damage pools
    auto it = range::find( damage_pool, pool );
    assert( it != damage_pool.end() );

    damage_pool.erase( it );
    if ( sim().debug )
    {
      sim().out_debug.printf( "%s stormlash removing a pool: pool=%.1f, n_pools=%u",
          p() -> name(), pool -> damage, damage_pool.size() );
    }

    delete pool;
  }
};

damage_pool_t::damage_pool_t( player_t* pl, double d, std::vector<damage_pool_t*>& p,
                              const timespan_t& duration ) :
  damage( d ), last_proc( pl -> sim -> current_time() )
{
  expiration = make_event<stormlash_expiration_t>( *pl -> sim, pl, this, p, duration );
  p.push_back( this );
}

struct stormlash_callback_t : public dbc_proc_callback_t
{
  timespan_t buff_duration;
  double coefficient;
  std::vector<damage_pool_t*> damage_pool;
  size_t n_buffs;

  stormlash_spell_t* spell;

  stormlash_callback_t( player_t* p, const special_effect_t& effect ) :
    dbc_proc_callback_t( p, effect ),
    buff_duration( p -> find_spell( 195222 ) -> duration() ),
    coefficient( p -> find_spell( 213307 ) -> effectN( 1 ).ap_coeff() ),
    n_buffs( as<size_t>( shaman() -> spec.stormlash -> effectN( 1 ).base_value() +
            shaman() -> talent.empowered_stormlash -> effectN( 1 ).base_value() ) ),
    spell( new stormlash_spell_t( p ) )
  { }

  shaman_t* shaman() const
  { return debug_cast<shaman_t*>( listener ); }

  // Create n_buffs pools, roughly emulating n_buffs stormstrikes from the shaman going out to
  // random targets in a (full) raid sim
  void add_pool()
  {
    double pool = listener -> cache.attack_power() * coefficient;
    // Add in global damage multipliers
    pool *= listener -> composite_player_multiplier( SCHOOL_NATURE );
    // Add in versatility multiplier
    pool *= 1.0 + listener -> cache.damage_versatility();
    // Add in crit multiplier
    pool *= 1.0 + listener -> cache.attack_crit_chance();
    // TODO: Add in crit damage multiplier

    // Apply Empowered Stormlash damage bonus
    pool *= 1.0 + shaman() -> talent.empowered_stormlash -> effectN( 2 ).percent();

    int replace_buffs = static_cast<int>( ( damage_pool.size() + n_buffs ) - shaman() -> stormlash_targets );
    if ( replace_buffs < 0 )
    {
      replace_buffs = 0;
    }

    // New buffs
    for ( size_t i = 0; i < n_buffs - replace_buffs; ++i )
    {
      new damage_pool_t( listener, pool, damage_pool, buff_duration );
      if ( listener -> sim -> debug )
      {
        listener -> sim -> out_debug.printf( "%s stormlash adding new pool: pool=%.1f, n_pools=%u",
            listener -> name(), pool, damage_pool.size() );
      }
    }

    // Replace buffs
    for ( int i = 0; i < replace_buffs; ++i )
    {
      auto old_pool = damage_pool.front();
      if ( listener -> sim -> debug )
      {
        listener -> sim -> out_debug.printf( "%s stormlash replacing old pool %.1f -> %.1f, n_pools=%u",
            listener -> name(), old_pool -> damage, pool, damage_pool.size() );
      }
      damage_pool.erase( damage_pool.begin() );
      event_t::cancel( old_pool -> expiration );
      delete old_pool;

      new damage_pool_t( listener, pool, damage_pool, buff_duration );
    }
  }

  // Callback executes all generated pools
  void execute( action_t* /* a */, action_state_t* state ) override
  {
    range::for_each( damage_pool, [ this, &state ]( damage_pool_t* pool ) {
      timespan_t interval = listener -> sim -> current_time() - pool -> last_proc;
      // Enforce the 100ms limit here too, since new procs in simc can bypass teh damage icd,
      // resulting in really low damage values. This does not cost any relevant damage, since on the
      // next proc attempt, the interval will be higher, resulting in higher damage.
      if ( interval <= timespan_t::from_millis( 100 ) )
      {
        return;
      }

      double multiplier = interval / buff_duration;
      if ( listener -> sim -> debug )
      {
        listener -> sim -> out_debug.printf( "%s stormlash damage_pool=%.1f previous=%.3f interval=%.3f multiplier=%.3f damage=%.3f",
            listener -> name(), pool -> damage, pool -> last_proc.total_seconds(),
            interval.total_seconds(), multiplier, pool -> damage * multiplier );
      }

      pool -> last_proc = listener -> sim -> current_time();
      spell -> base_dd_min = spell -> base_dd_max = pool -> damage * multiplier;
      spell -> target = state -> target;
      spell -> execute();
    } );
  }

  void reset() override
  {
    dbc_proc_callback_t::reset();

    // Any dangling events at the end of the previous iteration are cancelled alrady, so just delete
    // the leftover stormlash objects in the damage pool, and clear the pool.
    range::dispose( damage_pool );
    damage_pool.clear();

    deactivate();
  }
};

struct stormlash_buff_t : public buff_t
{
  stormlash_callback_t* callback;

  stormlash_buff_t( const buff_creator_t& creator ) : buff_t( creator ), callback(nullptr)
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );
    callback -> add_pool();
    callback -> activate();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    callback -> deactivate();
  }
};

shaman_td_t::shaman_td_t( player_t* target, shaman_t* p ) :
  actor_target_data_t( target, p )
{
  dot.flame_shock       = target -> get_dot( "flame_shock", p );

  debuff.earthen_spike  = buff_creator_t( *this, "earthen_spike", p -> talent.earthen_spike )
                          .cd( timespan_t::zero() ) // Handled by the action
                          // -10% resistance in spell data, treat it as a multiplier instead
                          .default_value( 1.0 + p -> talent.earthen_spike -> effectN( 2 ).percent() );
  debuff.lightning_rod = buff_creator_t( *this, "lightning_rod", p -> find_spell( 197209 ) )
                         .chance( p -> talent.lightning_rod -> effectN( 1 ).percent() )
                         .default_value( p -> talent.lightning_rod -> effectN( 2 ).percent() )
                         .stack_change_callback( [ target, p ]( buff_t*, int, int new_stacks ) {
                            // down -> up
                            if ( new_stacks == 1 )
                              p -> lightning_rods.push_back( target );
                            // up -> down
                            else
                            {
                              auto it = range::find( p -> lightning_rods, target );
                              if ( it != p -> lightning_rods.end() )
                                p -> lightning_rods.erase( it );
                            }
                          } );
  debuff.storm_tempests = buff_creator_t( *this, "storm_tempests", p -> find_spell( 214265 ) )
                          .tick_callback( [ p ]( buff_t* b, int, timespan_t ) {
                            p -> action.storm_tempests -> target = b -> player;
                            p -> action.storm_tempests -> execute();
                          } );
}

// ==========================================================================
// Shaman Action Base Template
// ==========================================================================

template <class Base>
struct shaman_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef shaman_action_t base_t;

  // Cooldown tracking
  bool        track_cd_waste;
  simple_sample_data_with_min_max_t* cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;

  // Ghost wolf unshift
  bool        unshift_ghost_wolf;

  // Maelstrom stuff
  gain_t*     gain;
  double      maelstrom_gain;
  double      maelstrom_gain_coefficient;

  // Generic procs
  bool may_proc_unleash_doom;

  proc_t* proc_ud;

  shaman_action_t( const std::string& n, shaman_t* player,
                   const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    track_cd_waste( s -> cooldown() > timespan_t::zero() || s -> charge_cooldown() > timespan_t::zero() ),
    cd_wasted_exec( nullptr ), cd_wasted_cumulative( nullptr ), cd_wasted_iter( nullptr ),
    unshift_ghost_wolf( true ),
    gain( player -> get_gain( s -> id() > 0 ? s -> name_cstr() : n ) ),
    maelstrom_gain( 0 ), maelstrom_gain_coefficient( 1.0 ),
    may_proc_unleash_doom( false ), proc_ud( nullptr )
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

      maelstrom_gain = effect.resource( RESOURCE_MAELSTROM );
      ab::energize_type = ENERGIZE_NONE; // disable resource generation from spell data.
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
      cd_wasted_exec = p() -> template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p() -> cd_waste_exec );
      cd_wasted_cumulative = p() -> template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p() -> cd_waste_cumulative );
      cd_wasted_iter = p() -> template get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, p() -> cd_waste_iter );
    }

    // Setup Hasted CD for Enhancement
    ab::cooldown -> hasted = ab::data().affected_by( p() -> spec.shaman -> effectN( 2 ) );

    // Setup Hasted GCD for Enhancement
    if ( ab::data().affected_by( p() -> spec.shaman -> effectN( 3 ) ) )
    {
      ab::gcd_haste = HASTE_ATTACK;
    }

    may_proc_unleash_doom = p() -> artifact.unleash_doom.rank() && ! ab::callbacks && ! ab::background && ab::harmful &&
      ( ab::weapon_multiplier > 0 || ab::attack_power_mod.direct > 0 || ab::spell_power_mod.direct > 0 );
  }

  bool init_finished() override
  {
    if ( may_proc_unleash_doom )
    {
      proc_ud = ab::player -> get_proc( std::string( "Unleash Doom: " ) + full_name() );
    }

    return ab::init_finished();
  }


  shaman_t* p()
  { return debug_cast< shaman_t* >( ab::player ); }
  const shaman_t* p() const
  { return debug_cast< shaman_t* >( ab::player ); }

  shaman_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double composite_maelstrom_gain_coefficient( const action_state_t* ) const
  {
    double m = maelstrom_gain_coefficient;

    return m;
  }

  void execute() override
  {
    ab::execute();

    trigger_maelstrom_gain( ab::execute_state );
    trigger_eye_of_twisting_nether( ab::execute_state );
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    trigger_eye_of_twisting_nether( d -> state );
  }

  virtual void impact( action_state_t* state ) override
  {
    ab::impact( state );

    trigger_unleash_doom( state );
  }

  void schedule_execute( action_state_t* execute_state = nullptr ) override
  {
    if ( ! ab::background && unshift_ghost_wolf )
    {
      p() -> buff.ghost_wolf -> expire();
    }

    ab::schedule_execute( execute_state );
  }

  virtual void update_ready( timespan_t cd ) override
  {
    if ( cd_wasted_exec &&
         ( cd > timespan_t::zero() || ( cd <= timespan_t::zero() && ab::cooldown -> duration > timespan_t::zero() ) ) &&
         ab::cooldown -> current_charge == ab::cooldown -> charges &&
         ab::cooldown -> last_charged > timespan_t::zero() &&
         ab::cooldown -> last_charged < ab::sim -> current_time() )
    {
      double time_ = ( ab::sim -> current_time() - ab::cooldown -> last_charged ).total_seconds();
      if ( p() -> sim -> debug )
      {
        p() -> sim -> out_debug.printf( "%s %s cooldown waste tracking waste=%.3f exec_time=%.3f",
            p() -> name(), ab::name(), time_, ab::time_to_execute.total_seconds() );
      }
      time_ -= ab::time_to_execute.total_seconds();

      if ( time_ > 0 )
      {
        cd_wasted_exec -> add( time_ );
        cd_wasted_iter -> add( time_ );
      }
    }

    ab::update_ready( cd );
  }

  virtual expr_t* create_expression( const std::string& name ) override
  {
    if ( ! util::str_compare_ci( name, "cooldown.higher_priority.min_remains" ) )
      return ab::create_expression( name );

    struct hprio_cd_min_remains_expr_t : public expr_t
    {
      action_t* action_;
      std::vector<cooldown_t*> cd_;

      // TODO: Line_cd support
      hprio_cd_min_remains_expr_t( action_t* a ) :
        expr_t( "min_remains" ), action_( a )
      {
        action_priority_list_t* list = a -> player -> get_action_priority_list( a -> action_list -> name_str );
        for (auto list_action : list -> foreground_action_list)
        {
          // Jump out when we reach this action
          if ( list_action == action_ )
            break;

          // Skip if this action's cooldown is the same as the list action's cooldown
          if ( list_action -> cooldown == action_ -> cooldown )
            continue;

          // Skip actions with no cooldown
          if ( list_action -> cooldown && list_action -> cooldown -> duration == timespan_t::zero() )
            continue;

          // Skip cooldowns that are already accounted for
          if ( std::find( cd_.begin(), cd_.end(), list_action -> cooldown ) != cd_.end() )
            continue;

          //std::cout << "Appending " << list_action -> name() << " to check list" << std::endl;
          cd_.push_back( list_action -> cooldown );
        }
      }

      double evaluate() override
      {
        if ( cd_.size() == 0 )
          return 0;

        timespan_t min_cd = cd_[ 0 ] -> remains();
        for ( size_t i = 1, end = cd_.size(); i < end; i++ )
        {
          timespan_t remains = cd_[ i ] -> remains();
          //std::cout << "cooldown.higher_priority.min_remains " << cd_[ i ] -> name_str << " remains=" << remains.total_seconds() << std::endl;
          if ( remains < min_cd )
            min_cd = remains;
        }

        //std::cout << "cooldown.higher_priority.min_remains=" << min_cd.total_seconds() << std::endl;
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
    g *= state -> n_targets;
    ab::player -> resource_gain( RESOURCE_MAELSTROM, g, gain, this );
  }

  void trigger_unleash_doom( const action_state_t* state )
  {
    if ( ! ab::result_is_hit( state -> result ) )
    {
      return;
    }

    if ( ! p() -> buff.unleash_doom -> up() )
    {
      return;
    }

    if ( ! may_proc_unleash_doom )
    {
      return;
    }

    size_t spell_idx = ab::rng().range( 0, p() -> action.unleash_doom.size() );
    p() -> action.unleash_doom[ spell_idx ] -> target = state -> target;
    p() -> action.unleash_doom[ spell_idx ] -> schedule_execute();
    proc_ud -> occur();
  }

  void trigger_eye_of_twisting_nether( const action_state_t* state )
  {
    if ( ab::harmful && state -> result_amount > 0 )
    {
      if ( dbc::is_school( ab::get_school(), SCHOOL_FIRE ) )
      {
        p() -> buff.eotn_fire -> trigger();
      }

      if ( dbc::is_school( ab::get_school(), SCHOOL_NATURE ) )
      {
        p() -> buff.eotn_shock -> trigger();
      }

      if ( dbc::is_school( ab::get_school(), SCHOOL_FROST ) )
      {
        p() -> buff.eotn_chill -> trigger();
      }
    }
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

  proc_t* proc_wf, *proc_ft, *proc_fb, *proc_mw, *proc_sb, *proc_ls, *proc_hh;

  shaman_attack_t( const std::string& token, shaman_t* p, const spell_data_t* s ) :
    base_t( token, p, s ),
    may_proc_windfury( p -> spec.windfury -> ok() ),
    may_proc_flametongue( p -> spec.flametongue -> ok() ),
    may_proc_frostbrand( p -> spec.frostbrand -> ok() ),
    may_proc_maelstrom_weapon( false ), // Change to whitelisting
    may_proc_stormbringer( p -> spec.stormbringer -> ok() ),
    may_proc_lightning_shield( false ),
    may_proc_hot_hand( p -> talent.hot_hand -> ok() ),
    proc_wf( nullptr ), proc_ft( nullptr ), proc_fb( nullptr ), proc_mw( nullptr ),
    proc_sb( nullptr ), proc_ls( nullptr ), proc_hh( nullptr )
  {
    special = true;
    may_glance = false;
  }

  void init() override
  {
    ab::init();

    if ( may_proc_stormbringer )
    {
      may_proc_stormbringer = ab::weapon && ab::weapon -> slot == SLOT_MAIN_HAND;
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

    may_proc_lightning_shield = p() -> talent.lightning_shield -> ok() && weapon && weapon_multiplier > 0;
  }

  bool init_finished() override
  {
    if ( may_proc_flametongue )
    {
      proc_ft = player -> get_proc( std::string( "Flametongue: " ) + full_name() );
    }

    if ( may_proc_frostbrand )
    {
      proc_fb = player -> get_proc( std::string( "Frostbrand: " ) + full_name() );
    }

    if ( may_proc_hot_hand )
    {
      proc_hh = player -> get_proc( std::string( "Hot Hand: " ) + full_name() );
    }

    if ( may_proc_lightning_shield )
    {
      proc_ls = player -> get_proc( std::string( "Lightning Shield: " ) + full_name() );
    }

    if ( may_proc_maelstrom_weapon )
    {
      proc_mw = player -> get_proc( std::string( "Maelstrom Weapon: " ) + full_name() );
    }

    if ( may_proc_stormbringer )
    {
      proc_sb = player -> get_proc( std::string( "Stormbringer: " ) + full_name() );
    }

    if ( may_proc_windfury )
    {
      proc_wf = player -> get_proc( std::string( "Windfury: " ) + full_name() );
    }

    return base_t::init_finished();
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    // Bail out early if the result is a miss/dodge/parry/ms
    if ( ! result_is_hit( state -> result ) )
      return;

    trigger_maelstrom_weapon( state );
    p() -> trigger_windfury_weapon( state );
    p() -> trigger_stormbringer( state );
    p() -> trigger_flametongue_weapon( state );
    p() -> trigger_hailstorm( state );
    p() -> trigger_lightning_shield( state );
    p() -> trigger_stormlash( state );
    p() -> trigger_hot_hand( state );
  }

  void trigger_maelstrom_weapon( const action_state_t* source_state, double amount = 0 )
  {
    if ( ! may_proc_maelstrom_weapon )
    {
      return;
    }

    if ( ! this -> weapon )
    {
      return;
    }

    if ( source_state -> result_raw <= 0 )
    {
      return;
    }

    if ( amount == 0 )
    {
      amount = p() -> spell.maelstrom_melee_gain -> effectN( 1 ).resource( RESOURCE_MAELSTROM );
    }

    p() -> resource_gain( RESOURCE_MAELSTROM, amount, gain, this );
    proc_mw -> occur();

    if ( p() -> action.electrocute &&
         rng().roll( p() -> sets.set( SHAMAN_ENHANCEMENT, T18, B2 ) -> effectN( 1 ).percent() ) )
    {
      p() -> action.electrocute -> target = source_state -> target;
      p() -> action.electrocute -> schedule_execute();
    }
  }

  virtual double stormbringer_proc_chance() const
  {
    return p() -> spec.stormbringer -> proc_chance() +
           p() -> cache.mastery() * p() -> mastery.enhanced_elements -> effectN( 3 ).mastery_value();
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

  shaman_spell_base_t( const std::string& n, shaman_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  { }

  virtual void execute() override
  {
    ab::execute();

    if ( ! ab::background && ab::execute_state -> result_raw > 0 )
    {
      ab::p() -> buff.elemental_focus -> decrement();
    }

    ab::p() -> buff.spiritwalkers_grace -> up();

    if ( ab::p() -> talent.aftershock -> ok() &&
         ab::current_resource() == RESOURCE_MAELSTROM &&
         ab::resource_consumed > 0 )
    {
      ab::p() -> resource_gain( RESOURCE_MAELSTROM,
          ab::resource_consumed * ab::p() -> talent.aftershock -> effectN( 1 ).percent(),
          ab::p() -> gain.aftershock,
          nullptr );
    }
  }

  virtual void schedule_travel( action_state_t* s ) override
  {
    ab::schedule_travel( s );

    // TODO: Check if Elemental Focus triggers per target (Chain Lightning)
    ab::p() -> trigger_elemental_focus( s );
  }
};

// ==========================================================================
// Shaman Offensive Spell
// ==========================================================================

struct shaman_spell_t : public shaman_spell_base_t<spell_t>
{
  action_t* overload;

  shaman_spell_t( const std::string& token, shaman_t* p,
                  const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() ) :
    base_t( token, p, s ), overload( nullptr )
  {
    parse_options( options );

    if ( data().affected_by( p -> spec.elemental_fury -> effectN( 1 ) ) )
    {
      crit_bonus_multiplier *= 1.0 + p -> spec.elemental_fury -> effectN( 1 ).percent();
    }

    if ( data().affected_by( player -> sets.set( SHAMAN_ELEMENTAL, T19, B2 ) ) )
    {
      base_crit += player -> sets.set( SHAMAN_ELEMENTAL, T19, B2 ) -> effectN( 1 ).percent();
    }
  }

  void execute() override
  {
    base_t::execute();

    p() -> trigger_earthen_rage( execute_state );
  }

  void schedule_travel( action_state_t* s ) override
  {
    trigger_elemental_overload( s );

    base_t::schedule_travel( s );
  }

  virtual bool usable_moving() const override
  {
    if ( p() -> buff.spiritwalkers_grace -> check() || execute_time() == timespan_t::zero() )
      return true;

    return base_t::usable_moving();
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_persistent_multiplier( state );

    // TODO: Check if this is a persistent multiplier.
    if ( p() -> buff.elemental_focus -> up() )
    {
      m *= p() -> buff.elemental_focus -> check_value();
    }

    return m;
  }

  virtual double overload_chance( const action_state_t* ) const
  { return p() -> cache.mastery_value(); }

  // Additional guaranteed overloads
  virtual size_t n_overloads( const action_state_t* ) const
  { return 0; }

  void trigger_elemental_overload( const action_state_t* source_state ) const
  {
    struct elemental_overload_event_t : public event_t
    {
      action_state_t* state;

      elemental_overload_event_t( action_state_t* s ) :
        event_t( *s -> action -> player, timespan_t::from_millis( 400 ) ), state( s )
      {
      }

      ~elemental_overload_event_t()
      { if ( state ) action_state_t::release( state ); }

      const char* name() const override
      { return "elemental_overload_event_t"; }

      void execute() override
      {
        state -> action -> schedule_execute( state );
        state = nullptr;
      }
    };

    if ( ! p() -> mastery.elemental_overload -> ok() )
    {
      return;
    }

    if ( ! overload )
    {
      return;
    }

    unsigned overloads = rng().roll( overload_chance( source_state ) ) + ( unsigned ) n_overloads( source_state );

    for ( size_t i = 0, end = overloads; i < end; ++i )
    {
      action_state_t* s = overload -> get_state();
      overload -> snapshot_state( s, DMG_DIRECT );
      s -> target = source_state -> target;

      make_event<elemental_overload_event_t>( *sim, s );
    }
  }
};

// ==========================================================================
// Shaman Heal
// ==========================================================================

struct shaman_heal_t : public shaman_spell_base_t<heal_t>
{
  double elw_proc_high,
         elw_proc_low,
         resurgence_gain;

  bool proc_tidal_waves,
       consume_tidal_waves;

  shaman_heal_t( const std::string& token, shaman_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s ),
    elw_proc_high( .2 ), elw_proc_low( 1.0 ),
    resurgence_gain( 0 ),
    proc_tidal_waves( false ), consume_tidal_waves( false )
  {
    parse_options( options );
  }

  shaman_heal_t( shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( "", p, s ),
    elw_proc_high( .2 ), elw_proc_low( 1.0 ),
    resurgence_gain( 0 ),
    proc_tidal_waves( false ), consume_tidal_waves( false )
  {
    parse_options( options );
  }

  double composite_spell_power() const override
  {
    double sp = base_t::composite_spell_power();

    if ( p() -> main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
      sp += p() -> main_hand_weapon.buff_value;

    return sp;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_da_multiplier( state );
    m *= 1.0 + p() -> spec.purification -> effectN( 1 ).percent();
    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_ta_multiplier( state );
    m *= 1.0 + p() -> spec.purification -> effectN( 1 ).percent();
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
      p() -> buff.tidal_waves -> decrement();
  }

  virtual double deep_healing( const action_state_t* s )
  {
    if ( ! p() -> mastery.deep_healing -> ok() )
      return 0.0;

    double hpp = ( 1.0 - s -> target -> health_percentage() / 100.0 );

    return 1.0 + hpp * p() -> cache.mastery_value();
  }
};

namespace pet
{

// ==========================================================================
// Base Shaman Pet
// ==========================================================================

struct shaman_pet_t : public pet_t
{
  bool use_auto_attack;

  shaman_pet_t( shaman_t* owner, const std::string& name, bool guardian = true, bool auto_attack = true ) :
    pet_t( owner -> sim, owner, name, guardian ), use_auto_attack( auto_attack )
  {
    regen_type            = REGEN_DISABLED;
    main_hand_weapon.type = WEAPON_BEAST;
  }

  shaman_t* o() const
  { return debug_cast<shaman_t*>( owner ); }

  virtual void create_default_apl()
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( use_auto_attack )
    {
      def -> add_action( "auto_attack" );
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
  { return nullptr; }
};

// ==========================================================================
// Base Shaman Pet Action
// ==========================================================================

template <typename T_PET, typename T_ACTION>
struct pet_action_t : public T_ACTION
{
  typedef pet_action_t<T_PET, T_ACTION> super;

  pet_action_t( T_PET* pet, const std::string& name, const spell_data_t* spell = spell_data_t::nil(), const std::string& options = std::string() ) :
    T_ACTION( name, pet, spell )
  {
    this -> parse_options( options );

    this -> special = true;
    this -> may_crit = true;
    //this -> crit_bonus_multiplier *= 1.0 + p() -> o() -> spec.elemental_fury -> effectN( 1 ).percent();
  }

  T_PET* p() const
  { return debug_cast<T_PET*>( this -> player ); }

  void init() override
  {
    T_ACTION::init();

    if ( ! this -> player -> sim -> report_pets_separately )
    {
      auto it = range::find_if( p() -> o() -> pet_list, [ this ]( pet_t* pet ) {
        return this -> player -> name_str == pet -> name_str;
      } );

      if ( it != p() -> o() -> pet_list.end() && this -> player != *it )
      {
        this -> stats = ( *it ) -> get_stats( this -> name(), this );
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

  pet_melee_attack_t( T_PET* pet, const std::string& name, const spell_data_t* spell = spell_data_t::nil(), const std::string& options = std::string() ) :
    pet_action_t<T_PET, melee_attack_t>( pet, name, spell, options )
  {
    if ( this -> school == SCHOOL_NONE )
      this -> school = SCHOOL_PHYSICAL;

    if ( this -> p() -> owner_coeff.sp_from_sp > 0 || this -> p() -> owner_coeff.sp_from_ap > 0 )
    {
      this -> spell_power_mod.direct = 1.0;
    }
  }

  void init() override
  {
    pet_action_t<T_PET, melee_attack_t>::init();

    if ( ! this -> special )
    {
      this -> weapon = &( this -> p() -> main_hand_weapon );
      this -> base_execute_time = this -> weapon -> swing_time;
    }
  }

  void execute() override
  {
    // If we're casting, we should clip a swing
    if ( this -> time_to_execute > timespan_t::zero() && this -> player -> executing )
      this -> schedule_execute();
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
    assert( player -> main_hand_weapon.type != WEAPON_NONE );
    player -> main_hand_attack = player -> create_auto_attack();
  }

  void execute() override
  { player -> main_hand_attack -> schedule_execute(); }

  bool ready() override
  {
    if ( player -> is_moving() ) return false;
    return ( player -> main_hand_attack -> execute_event == nullptr );
  }
};

// ==========================================================================
// Base Shaman Pet Spell
// ==========================================================================

template <typename T_PET>
struct pet_spell_t : public pet_action_t<T_PET, spell_t>
{
  typedef pet_spell_t<T_PET> super;

  pet_spell_t( T_PET* pet, const std::string& name, const spell_data_t* spell = spell_data_t::nil(), const std::string& options = std::string() ) :
    pet_action_t<T_PET, spell_t>( pet, name, spell, options )
  {
    this -> parse_options( options );
  }
};

// ==========================================================================
// Base Shaman Pet Method Definitions
// ==========================================================================

action_t* shaman_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "auto_attack" ) return new auto_attack_t( this );

  return pet_t::create_action( name, options_str );
}

// ==========================================================================
// Feral Spirit
// ==========================================================================

struct base_wolf_t : public shaman_pet_t
{
  action_t* alpha_wolf;
  buff_t* alpha_wolf_buff;

  base_wolf_t( shaman_t* owner, const std::string& name ) :
    shaman_pet_t( owner, name ), alpha_wolf( nullptr ), alpha_wolf_buff( nullptr )
  {
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
    owner_coeff.ap_from_ap      = 2.00;
  }

  void create_buffs() override
  {
    shaman_pet_t::create_buffs();

    if ( o() -> artifact.alpha_wolf.rank() )
    {
      alpha_wolf_buff = buff_creator_t( this, "alpha_wolf", o() -> find_spell( 198486 ) )
                        .tick_behavior( BUFF_TICK_REFRESH )
                        .tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                          alpha_wolf -> target = o() -> target;
                          alpha_wolf -> schedule_execute();
                        } );
    }
  }

  void trigger_alpha_wolf() const
  {
    if ( o() -> artifact.alpha_wolf.rank() )
    {
      alpha_wolf_buff -> trigger();
    }
  }
};

struct spirit_wolf_t : public base_wolf_t
{
  struct windfury_t : public pet_melee_attack_t<spirit_wolf_t>
  {
    windfury_t( spirit_wolf_t* player ) :
      super( player, "windfury_attack", player -> find_spell( 170512 ) )
    { }
  };

  struct fs_melee_t : public pet_melee_attack_t<spirit_wolf_t>
  {
    windfury_t* wf;
    const spell_data_t* maelstrom, * wf_driver;

    fs_melee_t( spirit_wolf_t* player ) : super( player, "melee" ),
      wf( p() -> owner -> sets.has_set_bonus( SHAMAN_ENHANCEMENT, T17, B4 ) ? new windfury_t( player ) : nullptr ),
      maelstrom( player -> find_spell( 190185 ) ), wf_driver( player -> find_spell( 170523 ) )
    {
      background = repeating = true;
      special = false;
    }

    void impact( action_state_t* state ) override
    {
      melee_attack_t::impact( state );

      shaman_t* o = p() -> o();
      if ( o -> spec.feral_spirit_2 -> ok() )
      {
        o -> resource_gain( RESOURCE_MAELSTROM,
                            maelstrom -> effectN( 1 ).resource( RESOURCE_MAELSTROM ),
                            o -> gain.feral_spirit,
                            state -> action );
      }

      if ( result_is_hit( state -> result ) && o -> buff.feral_spirit -> up() &&
           rng().roll( wf_driver -> proc_chance() ) )
      {
        wf -> target = state -> target;
        wf -> schedule_execute();
        wf -> schedule_execute();
        wf -> schedule_execute();
      }
    }
  };

  spirit_wolf_t( shaman_t* owner ) : base_wolf_t( owner, "spirit_wolf" )
  { }

  bool create_actions() override;

  attack_t* create_auto_attack() override
  { return new fs_melee_t( this ); }
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
};

bool spirit_wolf_t::create_actions()
{
  if ( o() -> artifact.alpha_wolf.rank() )
  {
    alpha_wolf = new spirit_bomb_t<spirit_wolf_t>( this );
  }

  return shaman_pet_t::create_actions();
}

// ==========================================================================
// DOOM WOLVES OF DOOM
// ==========================================================================

struct doom_wolf_base_t : public base_wolf_t
{
  struct dw_melee_t : public pet_melee_attack_t<doom_wolf_base_t>
  {
    const spell_data_t* maelstrom;

    dw_melee_t( doom_wolf_base_t* player ) : super( player, "melee" ),
      maelstrom( player -> find_spell( 190185 ) )
    {
      background = repeating = true;
      special = false;
    }

    void impact( action_state_t* state ) override
    {
      melee_attack_t::impact( state );

      p() -> o() -> resource_gain( RESOURCE_MAELSTROM,
          maelstrom -> effectN( 1 ).resource( RESOURCE_MAELSTROM ),
          p() -> o() -> gain.feral_spirit,
          state -> action );
    }
  };

  cooldown_t* special_ability_cd;

  doom_wolf_base_t( shaman_t* owner, const std::string& name ) :
    base_wolf_t( owner, name ), special_ability_cd( nullptr )
  { }

  void arise() override
  {
    shaman_pet_t::arise();

    // The pet AI does not allow the special ability to immediately be used, instead the pets seem
    // to wait roughly 4 seconds before using it. Once the ability is usable, the pets use it
    // roughly every 5 seconds.
    if ( special_ability_cd )
    {
      special_ability_cd -> start( timespan_t::from_seconds( 4 ) );
    }
  }

  attack_t* create_auto_attack() override
  { return new dw_melee_t( this ); }
};

struct frost_wolf_t : public doom_wolf_base_t
{
  struct frozen_bite_t : public pet_melee_attack_t<frost_wolf_t>
  {
    frozen_bite_t( frost_wolf_t* player, const std::string& options ) :
      super( player, "frozen_bite", player -> find_spell( 224126 ), options )
    { p() -> special_ability_cd = cooldown; }
  };

  struct snowstorm_t : public pet_melee_attack_t<frost_wolf_t>
  {
    snowstorm_t( frost_wolf_t* player ) :
      super( player, "snowstorm", player -> find_spell( 198483 ) )
    { background = true; aoe = -1; }
  };

  frost_wolf_t( shaman_t* owner ) :
    doom_wolf_base_t( owner, owner -> raptor_glyph ? "frost_raptor" : "frost_wolf" )
  { }

  bool create_actions() override
  {
    if ( o() -> artifact.alpha_wolf.rank() )
    {
      if ( ! o() -> raptor_glyph )
      {
        alpha_wolf = new snowstorm_t( this );
      }
      else
      {
        alpha_wolf = new spirit_bomb_t<frost_wolf_t>( this );
      }
    }

    return doom_wolf_base_t::create_actions();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "frozen_bite" ) return new frozen_bite_t( this, options_str );

    return doom_wolf_base_t::create_action( name, options_str );
  }

  void create_default_apl() override
  {
    doom_wolf_base_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );
    // TODO: Proper delay
    def -> add_action( "frozen_bite,line_cd=5" );
  }
};

struct fire_wolf_t : public doom_wolf_base_t
{
  struct fiery_jaws_t : public pet_melee_attack_t<fire_wolf_t>
  {
    fiery_jaws_t( fire_wolf_t* player, const std::string& options ) :
      super( player, "fiery_jaws", player -> find_spell( 224125 ), options )
    { p() -> special_ability_cd = cooldown; }
  };

  struct fire_nova_t : public pet_melee_attack_t<fire_wolf_t>
  {
    fire_nova_t( fire_wolf_t* player ) :
      super( player, "fire_nova", player -> find_spell( 198480 ) )
    { background = true; aoe = -1; }
  };

  fire_wolf_t( shaman_t* owner ) :
    doom_wolf_base_t( owner, owner -> raptor_glyph ? "fiery_raptor" : "fiery_wolf" )
  { }

  bool create_actions() override
  {
    if ( o() -> artifact.alpha_wolf.rank() )
    {
      if ( ! o() -> raptor_glyph )
      {
        alpha_wolf = new fire_nova_t( this );
      }
      else
      {
        alpha_wolf = new spirit_bomb_t<fire_wolf_t>( this );
      }
    }

    return doom_wolf_base_t::create_actions();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "fiery_jaws" ) return new fiery_jaws_t( this, options_str );

    return doom_wolf_base_t::create_action( name, options_str );
  }

  void create_default_apl() override
  {
    doom_wolf_base_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );
    // TODO: Proper delay
    def -> add_action( "fiery_jaws,line_cd=5" );
  }
};

struct lightning_wolf_t : public doom_wolf_base_t
{
  struct crackling_surge_t : public pet_melee_attack_t<lightning_wolf_t>
  {
    crackling_surge_t( lightning_wolf_t* player, const std::string& options ) :
      super( player, "crackling_surge", player -> find_spell( 224127 ), options )
    { }

    void execute() override
    {
      super::execute();
      p() -> crackling_surge -> trigger();
    }
  };

  struct thunder_bite_t : public pet_melee_attack_t<lightning_wolf_t>
  {
    thunder_bite_t( lightning_wolf_t* player ) :
      super( player, "thunder_bite", player -> find_spell( 198485 ) )
    { background = true; chain_multiplier = data().effectN( 1 ).chain_multiplier(); }
  };

  buff_t* crackling_surge;

  lightning_wolf_t( shaman_t* owner ) :
    doom_wolf_base_t( owner, owner -> raptor_glyph ? "lightning_raptor" : "lightning_wolf" ),
    crackling_surge(nullptr)
  { }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = doom_wolf_base_t::composite_player_multiplier( s );

    m *= 1.0 + crackling_surge -> stack_value();

    return m;
  }

  void create_buffs() override
  {
    doom_wolf_base_t::create_buffs();

    crackling_surge = haste_buff_creator_t( this, "crackling_surge", find_spell( 224127 ) )
      .default_value( find_spell( 224127 ) -> effectN( 1 ).percent() );
  }

  bool create_actions() override
  {
    if ( o() -> artifact.alpha_wolf.rank() )
    {
      if ( ! o() -> raptor_glyph )
      {
        alpha_wolf = new thunder_bite_t( this );
      }
      else
      {
        alpha_wolf = new spirit_bomb_t<lightning_wolf_t>( this );
      }
    }

    return doom_wolf_base_t::create_actions();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "crackling_surge" ) return new crackling_surge_t( this, options_str );

    return doom_wolf_base_t::create_action( name, options_str );
  }

  void create_default_apl() override
  {
    doom_wolf_base_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );
    // TODO: Proper delay
    def -> add_action( "crackling_surge,line_cd=5" );
  }
};

// ==========================================================================
// Primal Elemental Base
// ==========================================================================

struct primal_elemental_t : public shaman_pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    void execute() override { player -> current.distance = 1; }
    timespan_t execute_time() const override { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
    bool ready() override { return ( player -> current.distance > 1 ); }
    bool usable_moving() const override { return true; }
  };

  primal_elemental_t( shaman_t* owner, const std::string& name, bool guardian = false, bool auto_attack = true ) :
    shaman_pet_t( owner, name, guardian, auto_attack )
  { }

  void create_default_apl() override
  {
    if ( use_auto_attack )
    {
      // Travel must come before auto attacks
      action_priority_list_t* def = get_action_priority_list( "default" );
      def -> add_action( "travel" );
    }

    shaman_pet_t::create_default_apl();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "travel"      ) return new travel_t( this );

    return shaman_pet_t::create_action( name, options_str );
  }

  double composite_attack_power_multiplier() const override
  {
    double m = pet_t::composite_attack_power_multiplier();

    m *= 1.0 + o() -> talent.primal_elementalist -> effectN( 1 ).percent();

    return m;
  }

  double composite_spell_power_multiplier() const override
  {
    double m = pet_t::composite_spell_power_multiplier();

    m *= 1.0 + o() -> talent.primal_elementalist -> effectN( 1 ).percent();

    return m;
  }

  attack_t* create_auto_attack() override
  {
    auto attack = new pet_melee_attack_t<primal_elemental_t>( this, "melee" );
    attack -> background = true;
    attack -> repeating = true;
    attack -> special = false;
    attack -> school = SCHOOL_PHYSICAL;
    return attack;
  }
};

// ==========================================================================
// Earth Elemental
// ==========================================================================

struct earth_elemental_t : public primal_elemental_t
{
  earth_elemental_t( shaman_t* owner, bool guardian ) :
    primal_elemental_t( owner, ( ! guardian ) ? "primal_earth_elemental" : "greater_earth_elemental", guardian )
  {
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_sp      = 0.05;
  }
};

// ==========================================================================
// Fire Elemental
// ==========================================================================

struct fire_elemental_t : public primal_elemental_t
{
  struct fire_nova_t  : public pet_spell_t<fire_elemental_t>
  {
    fire_nova_t( fire_elemental_t* player, const std::string& options ) :
      super( player, "fire_nova", player -> find_spell( 117588 ), options )
    {
      aoe = -1;
    }
  };

  struct fire_blast_t : public pet_spell_t<fire_elemental_t>
  {
    fire_blast_t( fire_elemental_t* player, const std::string& options ) :
      super( player, "fire_blast", player -> find_spell( 57984 ), options )
    { }

    bool usable_moving() const override
    { return true; }
  };

  struct immolate_t : public pet_spell_t<fire_elemental_t>
  {
    immolate_t( fire_elemental_t* player, const std::string& options ) :
      super( player, "immolate", player -> find_spell( 118297 ), options )
    {
      hasted_ticks = tick_may_crit = true;
    }
  };

  fire_elemental_t( shaman_t* owner, bool guardian ) :
    primal_elemental_t( owner, ( ! guardian ) ? "primal_fire_elemental" : "greater_fire_elemental", guardian, false )
  {
    owner_coeff.sp_from_sp = 1.0;
  }

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );

    if ( o() -> talent.primal_elementalist -> ok() )
    {
      def -> add_action( "fire_nova,if=active_enemies>2" );
      def -> add_action( "immolate,target_if=!ticking" );
    }
    else
    {
      def -> add_action( "fire_nova" );
    }
    def -> add_action( "fire_blast" );
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "fire_blast"  ) return new fire_blast_t( this, options_str );
    if ( name == "fire_nova"   ) return new fire_nova_t( this, options_str );
    if ( name == "immolate"    ) return new immolate_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Storm Elemental
// ==========================================================================

struct storm_elemental_t : public primal_elemental_t
{
  // TODO: Healing
  struct wind_gust_t : public pet_spell_t<storm_elemental_t>
  {
    const spell_data_t* energize;

    wind_gust_t( storm_elemental_t* player, const std::string& options ) :
      super( player, "wind_gust", player -> find_spell( 157331 ), options ),
      energize( player -> find_spell( 226180 ) )
    { }

    void execute() override
    {
      super::execute();

      p() -> o() -> resource_gain( RESOURCE_MAELSTROM,
        energize -> effectN( 1 ).resource( RESOURCE_MAELSTROM ),
        p() -> o() -> gain.wind_gust, this );
    }
  };

  struct call_lightning_t : public pet_spell_t<storm_elemental_t>
  {
    call_lightning_t( storm_elemental_t* player, const std::string& options ) :
      super( player, "call_lightning", player -> find_spell( 157348 ), options )
    { }

    void execute() override
    {
      super::execute();

      p() -> call_lightning -> trigger();
    }
  };

  buff_t* call_lightning;

  storm_elemental_t( shaman_t* owner, bool guardian ) :
    primal_elemental_t( owner, ( ! guardian ) ? "primal_storm_elemental" : "greater_storm_elemental", guardian, false ), call_lightning(nullptr)
  {
    owner_coeff.sp_from_sp = 1.0000;
  }

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "call_lightning" );
    def -> add_action( "wind_gust" );
  }

  void create_buffs() override
  {
    primal_elemental_t::create_buffs();

    call_lightning = buff_creator_t( this, "call_lightning", find_spell( 157348 ) )
                     .cd( timespan_t::zero() );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = primal_elemental_t::composite_player_multiplier( school );

    // TODO-WOD: Enhance/Elemental has damage, Restoration has healing
    if ( call_lightning -> up() )
      m *= 1.0 + call_lightning -> data().effectN( 2 ).percent();

    return m;
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "call_lightning" ) return new call_lightning_t( this, options_str );
    if ( name == "wind_gust"      ) return new wind_gust_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Greater Lightning Elemental (Fury of the Storms Artifact Power)
// ==========================================================================

struct greater_lightning_elemental_t : public shaman_pet_t
{
  struct lightning_blast_t : public pet_spell_t<greater_lightning_elemental_t>
  {
    lightning_blast_t( greater_lightning_elemental_t* p ) :
      super( p, "lightning_blast", p -> find_spell( 191726 ) )
    {
      ability_lag        = timespan_t::from_millis( 300 );
      ability_lag_stddev = timespan_t::from_millis( 25 );
    }
  };

  struct chain_lightning_t : public pet_spell_t<greater_lightning_elemental_t>
  {
    chain_lightning_t( greater_lightning_elemental_t* p ) :
      super( p, "chain_lightning", p -> find_spell( 191732 ) )
    {
      chain_multiplier = data().effectN( 1 ).chain_multiplier();
      ability_lag         = timespan_t::from_millis( 300 );
      ability_lag_stddev  = timespan_t::from_millis( 25 );
    }
  };

  greater_lightning_elemental_t( shaman_t* owner ) :
    shaman_pet_t( owner, "greater_lightning_elemental", true, false )
  {
    owner_coeff.sp_from_sp = 1.0;
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "lightning_blast" ) return new lightning_blast_t( this );
    if ( name == "chain_lightning" ) return new chain_lightning_t( this );

    return shaman_pet_t::create_action( name, options_str );
  }

  void create_default_apl() override
  {
    shaman_pet_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "lightning_blast" );
    def -> add_action( "chain_lightning", "if=spell_targets.chain_lightning>1" );
  }
};

} // Namespace pet ends

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct flametongue_weapon_spell_t : public shaman_spell_t
{
  flametongue_weapon_spell_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_spell_t( n, player, player -> find_spell( 10444 ) )
  {
    may_crit = background = true;

    if ( player -> specialization() == SHAMAN_ENHANCEMENT )
    {
      snapshot_flags          = STATE_AP;
      attack_power_mod.direct = w -> swing_time.total_seconds() / 2.6 * 0.125;
    }
  }
};

struct ancestral_awakening_t : public shaman_heal_t
{
  ancestral_awakening_t( shaman_t* player ) :
    shaman_heal_t( "ancestral_awakening", player, player -> find_spell( 52752 ) )
  {
    background = proc = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_heal_t::composite_da_multiplier( state );
    m *= p() -> spec.ancestral_awakening -> effectN( 1 ).percent();
    return m;
  }

  void execute() override
  {
    target = find_lowest_player();
    shaman_heal_t::execute();
  }
};

struct windfury_attack_t : public shaman_attack_t
{
  double furious_winds_chance;

  windfury_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    shaman_attack_t( n, player, s ), furious_winds_chance( 0 )
  {
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    background       = true;
    callbacks        = false;
    base_multiplier *= 1.0 + player -> artifact.wind_surge.percent();

    // Windfury can not proc itself
    may_proc_windfury = false;

    may_proc_maelstrom_weapon = true;
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p() -> buff.doom_winds -> up() )
    {
      m *= 1.0 + p() -> buff.doom_winds -> data().effectN( 2 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    if ( rng().roll( furious_winds_chance ) )
    {
      trigger_maelstrom_weapon( state );
    }
  }
};

struct crash_lightning_attack_t : public shaman_attack_t
{
  crash_lightning_attack_t( shaman_t* p, const std::string& n ) :
    shaman_attack_t( n, p, p -> find_spell( 195592 ) )
  {
    weapon = &( p -> main_hand_weapon );
    background = true;
    callbacks = false;
    aoe = -1;
    cooldown -> duration = timespan_t::zero();
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_frostbrand = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_maelstrom_weapon = may_proc_lightning_shield = false;
  }
};

struct hailstorm_attack_t : public shaman_attack_t
{
  hailstorm_attack_t( const std::string& n, shaman_t* p, weapon_t* w ) :
    shaman_attack_t( n, p, p -> find_spell( 210854 ) )
  {
    weapon = w;
    background = true;
    callbacks = false;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_frostbrand = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_stormbringer = may_proc_maelstrom_weapon = may_proc_lightning_shield = false;
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  bool stormflurry;

  stormstrike_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    shaman_attack_t( n, player, s ), stormflurry( false )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
    weapon = w;
    base_multiplier *= 1.0 + player -> artifact.hammer_of_storms.percent();
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_unleash_doom = true;
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p() -> artifact.raging_storms.rank() && p() -> buff.stormbringer -> up() )
    {
      m *= 1.0 + p() -> artifact.raging_storms.percent();
    }

    if ( p() -> buff.gathering_storms -> up() )
    {
      m *= p() -> buff.gathering_storms -> check_value();
    }

    if ( stormflurry )
    {
      m *= p() -> artifact.stormflurry.percent( 2 );
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = shaman_attack_t::composite_crit_chance();

    if ( p() -> buff.stormbringer -> up() )
    {
      c += p() -> sets.set( SHAMAN_ENHANCEMENT, T19, B2 ) -> effectN( 1 ).percent();
    }

    return c;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    stormflurry = false;
  }
};

struct windstrike_attack_t : public stormstrike_attack_t
{
  windstrike_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    stormstrike_attack_t( n, player, s, w )
  { }

  double target_armor( player_t* ) const override
  { return 0.0; }
};

struct windlash_t : public shaman_attack_t
{
  double swing_timer_variance;

  windlash_t( const std::string& n, const spell_data_t* s, shaman_t* player, weapon_t* w, double stv ) :
    shaman_attack_t( n, player, s ), swing_timer_variance( stv )
  {
    background = repeating = may_miss = may_dodge = may_parry = true;
    may_glance = special = false;
    weapon            = w;
    base_execute_time = w -> swing_time;
    trigger_gcd       = timespan_t::zero();

    may_proc_maelstrom_weapon = true; // Presumption, but should be safe
  }

  double target_armor( player_t* ) const override
  { return 0.0; }

  timespan_t execute_time() const override
  {
    timespan_t t = shaman_attack_t::execute_time();

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds( const_cast<windlash_t*>(this) -> rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim -> debug )
        sim -> out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(), t.total_seconds(), st.total_seconds() );

      return st;
    }
    else
      return t;
  }
};

struct shaman_flurry_of_xuen_t : public shaman_attack_t
{
  shaman_flurry_of_xuen_t( shaman_t* p ) :
    shaman_attack_t( "flurry_of_xuen", p, p -> find_spell( 147891 ) )
  {
    special = may_miss = may_parry = may_block = may_dodge = may_crit = background = true;

    may_proc_windfury = false;
    may_proc_flametongue = false;
    aoe = 5;
  }
};

struct shaman_spontaneous_appendages_t : public shaman_attack_t
{
  shaman_spontaneous_appendages_t( shaman_t* p, const special_effect_t& effect ) :
    shaman_attack_t( "horrific_slam", p,
                     p -> find_spell( effect.trigger() -> effectN( 1 ).trigger() -> id() ) )
  {
    special = may_miss = may_parry = may_block = may_dodge = may_crit = background = true;
    base_dd_min = data().effectN( 1 ).min( effect.item );
    base_dd_max = data().effectN( 1 ).max( effect.item );
    item = effect.item;

    // Spell data has no radius, so manually make it an AoE.
    radius = 8.0;
    aoe = -1;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_frostbrand = may_proc_flametongue = may_proc_hot_hand = false;
    may_proc_maelstrom_weapon = may_proc_lightning_shield = false;

    may_proc_stormbringer = true;
  }

  double target_armor( player_t* ) const override
  { return 0; }
};

struct electrocute_t : public shaman_spell_t
{
  electrocute_t( shaman_t* p ) :
    shaman_spell_t( "electrocute", p, p -> find_spell( 189509 ) )
  {
    background = true;
    aoe = -1;
  }
};

struct unleash_doom_spell_t : public shaman_spell_t
{
  unleash_doom_spell_t( const std::string& n, shaman_t* p, const spell_data_t* s ) :
    shaman_spell_t( n, p, s )
  {
    callbacks = false;
    background = may_crit = true;
  }
};

// Ground AOE pulse
struct ground_aoe_spell_t : public spell_t
{
  ground_aoe_spell_t( shaman_t* p, const std::string& name, const spell_data_t* spell ) :
    spell_t( name, p, spell )
  {
    aoe = -1;
    callbacks = false;
    ground_aoe = background = may_crit = true;
  }
};

struct volcanic_inferno_t : public ground_aoe_spell_t
{
  volcanic_inferno_t( shaman_t* p ) :
    ground_aoe_spell_t( p, "volcanic_inferno", p -> find_spell( 205533 ) )
  {
    background = true;
    aoe = -1;
  }
};

struct lightning_shield_damage_t : public shaman_spell_t
{
  lightning_shield_damage_t( shaman_t* player ) :
    shaman_spell_t( "lightning_shield_damage", player, player -> find_spell( 192109 ) )
  {
    background = true;
    callbacks = false;
  }
};

struct earthen_rage_spell_t : public shaman_spell_t
{
  earthen_rage_spell_t( shaman_t* p ) :
    shaman_spell_t( "earthen_rage", p, p -> find_spell( 170379 ) )
  {
    background = proc = true;
    callbacks = false;
  }
};

struct earthen_rage_driver_t : public spell_t
{
  earthen_rage_spell_t* nuke;

  earthen_rage_driver_t( shaman_t* p ) :
    spell_t( "earthen_rage_driver", p, p -> find_spell( 170377 ) )
  {
    may_miss = may_crit = callbacks = proc = tick_may_crit = false;
    background = hasted_ticks = quiet = dual = true;

    nuke = new earthen_rage_spell_t( p );
  }

  timespan_t tick_time( const action_state_t* state ) const override
  { return timespan_t::from_seconds( rng().range( 0.001, 2 * base_tick_time.total_seconds() * state -> haste ) ); }

  void tick( dot_t* d ) override
  {
    spell_t::tick( d );

    // Last tick will not cast a nuke like our normal dot system does
    if ( d -> remains() > timespan_t::zero() )
    {
      nuke -> target = d -> target;
      nuke -> schedule_execute();
    }
  }

  // Maximum duration is extended by max of 6 seconds
  timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t ) const override
  { return data().duration(); }
};

struct lightning_rod_t : public spell_t
{
  lightning_rod_t( shaman_t* p ) :
    spell_t( "lightning_rod", p, p -> find_spell( 197568 ) )
  {
    background = true;
    callbacks = may_crit = false;
  }

  void init() override
  {
    spell_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct pristine_protoscale_girdle_dot_t : public shaman_spell_t
{
  pristine_protoscale_girdle_dot_t( shaman_t* p ) :
    shaman_spell_t( "pristine_protoscale_girdle", p, p -> find_spell( 224852 ) )
  {
    background = true;
    callbacks = may_crit = hasted_ticks = false;

    dot_max_stack = data().max_stacks();
  }
};

struct storm_tempests_zap_t : public melee_attack_t
{
  storm_tempests_zap_t( shaman_t* p ) :
    melee_attack_t( "storm_tempests", p, p -> find_spell( 214452 ) )
  {
    weapon = &( p -> main_hand_weapon );
    // TODO: Can this crit?
    background = may_crit = true;
    callbacks = false;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    melee_attack_t::available_targets( tl );

    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }

  // TODO: Does this actually zap the enemy itself, if it's the only one available?
  // TODO: Distance targeting, no clue on range.
  void execute() override
  {
    if ( target_list().size() > 0 )
    {
      // Pick a random "nearby" target
      size_t target_idx = static_cast<size_t>( rng().range( 0, target_list().size() ) );
      target = target_list()[ target_idx ];

      melee_attack_t::execute();
    }
  }
};

// Elemental overloads

struct elemental_overload_spell_t : public shaman_spell_t
{
  elemental_overload_spell_t( shaman_t* p, const std::string& name, const spell_data_t* s ) :
    shaman_spell_t( name, p, s )
  {
    base_execute_time = timespan_t::zero();
    background = true;
    callbacks = false;

    base_multiplier *= 1.0 + p -> artifact.master_of_the_elements.percent();
    base_multiplier *= p -> mastery.elemental_overload -> effectN( 2 ).percent();
  }
};

// shaman_heal_t::impact ====================================================

void shaman_heal_t::impact( action_state_t* s )
{
  // Todo deep healing to adjust s -> result_amount by x% before impacting
  if ( sim -> debug && p() -> mastery.deep_healing -> ok() )
  {
    sim -> out_debug.printf( "%s Deep Heals %s@%.2f%% mul=%.3f %.0f -> %.0f",
                   player -> name(), s -> target -> name(),
                   s -> target -> health_percentage(), deep_healing( s ),
                   s -> result_amount, s -> result_amount * deep_healing( s ) );
  }

  s -> result_amount *= deep_healing( s );

  base_t::impact( s );

  if ( proc_tidal_waves )
    p() -> buff.tidal_waves -> trigger( p() -> buff.tidal_waves -> data().initial_stacks() );

  if ( s -> result == RESULT_CRIT )
  {
    if ( resurgence_gain > 0 )
      p() -> resource_gain( RESOURCE_MANA, resurgence_gain, p() -> gain.resurgence );

    if ( p() -> spec.ancestral_awakening -> ok() )
    {
      if ( ! p() -> action.ancestral_awakening )
      {
        p() -> action.ancestral_awakening = new ancestral_awakening_t( p() );
        p() -> action.ancestral_awakening -> init();
      }

      p() -> action.ancestral_awakening -> base_dd_min = s -> result_total;
      p() -> action.ancestral_awakening -> base_dd_max = s -> result_total;
    }
  }

  if ( p() -> main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
  {
    double chance = ( s -> target -> resources.pct( RESOURCE_HEALTH ) > .35 ) ? elw_proc_high : elw_proc_low;

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

  melee_t( const std::string& name, const spell_data_t* s, shaman_t* player, weapon_t* w, int sw, double stv ) :
    shaman_attack_t( name, player, s ), sync_weapons( sw ),
    first( true ), swing_timer_variance( stv )
  {
    background = repeating = may_glance = true;
    special           = false;
    trigger_gcd       = timespan_t::zero();
    weapon            = w;
    base_execute_time = w -> swing_time;

    if ( p() -> specialization() == SHAMAN_ENHANCEMENT && p() -> dual_wield() )
      base_hit -= 0.19;

    may_proc_maelstrom_weapon = true;
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
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    }

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds(const_cast<melee_t*>(this) ->  rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim -> debug )
        sim -> out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(), t.total_seconds(), st.total_seconds() );
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
};

// Auto Attack ==============================================================

struct auto_attack_t : public shaman_attack_t
{
  int sync_weapons;
  double swing_timer_variance;

  auto_attack_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "auto_attack", player, spell_data_t::nil() ),
    sync_weapons( 0 ), swing_timer_variance( 0.00 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    add_option( opt_float( "swing_timer_variance", swing_timer_variance ) );
    parse_options( options_str );
    ignore_false_positive = true;

    assert( p() -> main_hand_weapon.type != WEAPON_NONE );

    p() -> melee_mh      = new melee_t( "Main Hand", spell_data_t::nil(), player, &( p() -> main_hand_weapon ), sync_weapons, swing_timer_variance );
    p() -> melee_mh      -> school = SCHOOL_PHYSICAL;
    p() -> ascendance_mh = new windlash_t( "Wind Lash", player -> find_spell( 114089 ), player, &( p() -> main_hand_weapon ), swing_timer_variance );

    p() -> main_hand_attack = p() -> melee_mh;

    if ( p() -> off_hand_weapon.type != WEAPON_NONE && p() -> specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( ! p() -> dual_wield() ) return;

      p() -> melee_oh = new melee_t( "Off-Hand", spell_data_t::nil(), player, &( p() -> off_hand_weapon ), sync_weapons, swing_timer_variance );
      p() -> melee_oh -> school = SCHOOL_PHYSICAL;
      p() -> ascendance_oh = new windlash_t( "Wind Lash Off-Hand", player -> find_spell( 114093 ), player, &( p() -> off_hand_weapon ), swing_timer_variance );

      p() -> off_hand_attack = p() -> melee_oh;

      p() -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    p() -> main_hand_attack -> schedule_execute();
    if ( p() -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    if ( p() -> is_moving() ) return false;
    return ( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  double aaj_multiplier; // 7.0 legendary Akainu's Absolute Justice multiplier
  crash_lightning_attack_t* cl;

  lava_lash_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "lava_lash", player, player -> find_specialization_spell( "Lava Lash" ) ),
    aaj_multiplier( 0 ),
    cl( new crash_lightning_attack_t( player, "lava_lash_cl" ) )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    school = SCHOOL_FIRE;

    base_multiplier *= 1.0 + player -> artifact.forged_in_lava.percent();

    parse_options( options_str );
    weapon              = &( player -> off_hand_weapon );

    if ( weapon -> type == WEAPON_NONE )
      background = true; // Do not allow execution.

    add_child( cl );
    if ( player -> artifact.doom_vortex.rank() )
    {
      add_child( player -> action.doom_vortex_ll );
    }
  }

  double stormbringer_proc_chance() const override
  { return p() -> sets.set( SHAMAN_ENHANCEMENT, T19, B4 ) -> proc_chance(); }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_stormbringer = p() -> sets.has_set_bonus( SHAMAN_ENHANCEMENT, T19, B4 );
  }

  double cost() const override
  {
    if ( p() -> buff.hot_hand -> check() )
    {
      return 0;
    }

    return shaman_attack_t::cost();
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p() -> buff.hot_hand -> up() )
    {
      m *= 1.0 + p() -> buff.hot_hand -> data().effectN( 1 ).percent();
    }

    if ( aaj_multiplier > 0 && p() -> buff.flametongue -> up() && p() -> buff.frostbrand -> up() )
    {
      m *= 1.0 + aaj_multiplier;
    }

    return m;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p() -> buff.hot_hand -> decrement();
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    if ( result_is_hit( state -> result ) && p() -> buff.crash_lightning -> up() )
    {
      cl -> target = state -> target;
      cl -> schedule_execute();
    }

    p() -> trigger_doom_vortex( state );

  }
};

// Stormstrike Attack =======================================================

struct stormstrike_base_t : public shaman_attack_t
{
  crash_lightning_attack_t* cl;
  stormstrike_attack_t* mh, *oh;
  bool stormflurry;
  bool background_action;

  stormstrike_base_t( shaman_t* player, const std::string& name,
                      const spell_data_t* spell, const std::string& options_str ) :
    shaman_attack_t( name, player, spell ),
    cl( new crash_lightning_attack_t( player, name + "_cl" ) ),
    mh( nullptr ), oh( nullptr ),
    stormflurry( false ), background_action( false )
  {
    parse_options( options_str );

    weapon               = &( p() -> main_hand_weapon );
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = data().cooldown();
    weapon_multiplier    = 0.0;
    may_crit             = false;

    add_child( cl );
  }

  void init() override
  {
    shaman_attack_t::init();
    may_proc_flametongue = may_proc_windfury = may_proc_stormbringer = may_proc_frostbrand = false;
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( p() -> buff.stormbringer -> up() || background == true )
    {
      cd_duration = timespan_t::zero();
    }

    shaman_attack_t::update_ready( cd_duration );
  }

  double cost() const override
  {
    double c = shaman_attack_t::cost();

    if ( stormflurry == true )
    {
      return 0;
    }

    if ( p() -> buff.stormbringer -> check() )
    {
      c *= 1.0 + p() -> buff.stormbringer -> data().effectN( 3 ).percent();
    }

    return c;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh -> stormflurry = stormflurry;
      mh -> execute();
      if ( oh )
      {
        oh -> stormflurry = stormflurry;
        oh -> execute();
      }

      if ( p() -> action.storm_tempests )
      {
        if ( execute_state -> target != p() -> action.storm_tempests -> target )
        {
          p() -> action.storm_tempests -> target_cache.is_valid = false;
        }
        td( execute_state -> target ) -> debuff.storm_tempests -> trigger();
      }

      if ( ! stormflurry && p() -> buff.crash_lightning -> up() )
      {
        cl -> target = execute_state -> target;
        cl -> execute();
      }

      if ( p() -> sets.has_set_bonus( SHAMAN_ENHANCEMENT, T17, B2 ) )
      {
        p() -> cooldown.feral_spirits -> adjust( - p() -> sets.set( SHAMAN_ENHANCEMENT, T17, B2 ) -> effectN( 1 ).time_value() );
      }
    }

    p() -> buff.stormbringer -> decrement();
    p() -> buff.gathering_storms -> decrement();
    p() -> trigger_t19_oh_8pc( execute_state );

    // Don't try this at home, or anywhere else ..
    if ( p() -> artifact.stormflurry.rank() && rng().roll( p() -> artifact.stormflurry.percent() ) )
    {
      background = dual = stormflurry = true;
      schedule_execute();
    }
    // Potential stormflurrying ends, reset things
    else
    {
      background = background_action;
      dual = false;
      stormflurry = false;
    }
  }

  void reset() override
  {
    shaman_attack_t::reset();
    background = background_action;
    dual = false;
    stormflurry = false;
  }
};


struct stormstrike_t : public stormstrike_base_t
{
  stormstrike_t( shaman_t* player, const std::string& options_str ) :
    stormstrike_base_t( player, "stormstrike", player -> find_specialization_spell( "Stormstrike" ), options_str )
  {
    // Actual damaging attacks are done by stormstrike_attack_t
    mh = new stormstrike_attack_t( "stormstrike_mh", player, data().effectN( 1 ).trigger(), &( player -> main_hand_weapon ) );
    add_child( mh );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new stormstrike_attack_t( "stormstrike_offhand", player, data().effectN( 2 ).trigger(), &( player -> off_hand_weapon ) );
      add_child( oh );
    }
  }

  void execute() override
  {
    // Proc unleash doom before the actual damage strikes, they already benefit from the buff
    p() -> buff.unleash_doom -> trigger();

    stormstrike_base_t::execute();
  }

  bool ready() override
  {
    if ( p() -> buff.ascendance -> check() )
      return false;

    return stormstrike_base_t::ready();
  }
};

// Windstrike Attack ========================================================

struct windstrike_t : public stormstrike_base_t
{
  timespan_t cd;

  windstrike_t( shaman_t* player, const std::string& options_str ) :
    stormstrike_base_t( player, "windstrike", player -> find_spell( 115356 ), options_str ),
    cd( data().cooldown() )
  {
    // Actual damaging attacks are done by stormstrike_attack_t
    mh = new windstrike_attack_t( "windstrike_mh", player, data().effectN( 1 ).trigger(), &( player -> main_hand_weapon ) );
    add_child( mh );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new windstrike_attack_t( "windstrike_offhand", player, data().effectN( 2 ).trigger(), &( player -> off_hand_weapon ) );
      add_child( oh );
    }
  }

  bool ready() override
  {
    if ( p() -> buff.ascendance -> remains() <= cooldown -> queue_delay() )
      return false;

    return stormstrike_base_t::ready();
  }
};

// Sundering Spell =========================================================

struct sundering_t : public shaman_attack_t
{
  sundering_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "sundering", player, player -> talent.sundering )
  {
    parse_options( options_str );

    aoe = -1; // TODO: This is likely not going to affect all enemies but it will do for now
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_stormbringer = may_proc_lightning_shield = may_proc_frostbrand = true;
    may_proc_hot_hand = p() -> talent.hot_hand -> ok();
  }
};

// Rockbiter Spell =========================================================

struct rockbiter_t : public shaman_spell_t
{
  rockbiter_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "rockbiter", player, player -> find_specialization_spell( "Rockbiter" ), options_str )
  {
    maelstrom_gain += player -> artifact.gathering_of_the_maelstrom.value();
    base_multiplier *= 1.0 + player -> artifact.weapons_of_the_elements.percent();
  }

  void execute() override
  {
    p() -> buff.landslide-> trigger();

    shaman_spell_t::execute();
  }

  bool ready() override
  {
    if ( p() -> talent.boulderfist -> ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Flametongue Spell =========================================================

struct flametongue_t : public shaman_spell_t
{
  flametongue_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "flametongue", player, player -> find_specialization_spell( "Flametongue" ), options_str )
  {
    base_multiplier *= 1.0 + player -> artifact.weapons_of_the_elements.percent();

    add_child( player -> flametongue );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.flametongue -> trigger();
  }
};

// Frostbrand Spell =========================================================

struct frostbrand_t : public shaman_spell_t
{
  frostbrand_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "frostbrand", player, player -> find_specialization_spell( "Frostbrand" ), options_str )
  {
    base_multiplier *= 1.0 + player -> artifact.weapons_of_the_elements.percent();

    if ( player -> hailstorm )
      add_child( player -> hailstorm );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.frostbrand -> trigger();
  }
};

// Crash Lightning Attack ===================================================

struct crash_lightning_t : public shaman_attack_t
{
  size_t ecc_min_targets;

  crash_lightning_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "crash_lightning", player, player -> find_specialization_spell( "Crash Lightning" ) ),
    ecc_min_targets( 0 )
  {
    parse_options( options_str );

    aoe = -1;
    weapon = &( p() -> main_hand_weapon );

    if ( player -> action.crashing_storm )
    {
      add_child( player -> action.crashing_storm );
    }
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( p() -> talent.crashing_storm -> ok() )
    {
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
          .target( execute_state -> target )
          .duration( p() -> find_spell( 205532 ) -> duration() )
          .action( p() -> action.crashing_storm ), true );
    }

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( execute_state -> n_targets > 1 )
      {
        p() -> buff.crash_lightning -> trigger();
      }

      if ( p() -> artifact.gathering_storms.rank() )
      {
        double v = 1.0 + p() -> artifact.gathering_storms.percent() * execute_state -> n_targets;
        p() -> buff.gathering_storms -> trigger( 1, v );
      }

      if ( ecc_min_targets > 0 && execute_state -> n_targets >= ecc_min_targets )
      {
        p() -> buff.emalons_charged_core -> trigger();
      }
    }

    if ( p() -> artifact.doom_wolves.rank() )
    {
      if ( p() -> pet.doom_wolves[ 0 ] )
      {
        range::for_each( p() -> pet.doom_wolves, [ this ]( pet_t* pet ) {
          pet::base_wolf_t* wolf = debug_cast<pet::doom_wolf_base_t*>( pet );
          if ( ! wolf -> is_sleeping() )
          {
            wolf -> trigger_alpha_wolf();
          }
        } );
      }
    }
    else
    {
      if ( p() -> pet.spirit_wolves[ 0 ] )
      {
        range::for_each( p() -> pet.spirit_wolves, [ this ]( pet_t* pet ) {
          pet::base_wolf_t* wolf = debug_cast<pet::spirit_wolf_t*>( pet );
          if ( ! wolf -> is_sleeping() )
          {
            wolf -> trigger_alpha_wolf();
          }
        } );
      }
    }
  }
};

struct fire_elemental_t : public shaman_spell_t
{
  const spell_data_t* base_spell;

  fire_elemental_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "fire_elemental", player, player -> find_specialization_spell( "Fire Elemental" ), options_str ),
    base_spell( player -> find_spell( 188592 ) )
  {
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p() -> talent.primal_elementalist -> ok() )
    {
      p() -> pet.pet_fire_elemental -> summon( base_spell -> duration() );
    }
    else
    {
      p() -> pet.guardian_fire_elemental -> summon( base_spell -> duration() );
    }
  }

  bool ready() override
  {
    if ( p() -> talent.storm_elemental -> ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

struct windsong_t : public shaman_spell_t
{
  windsong_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "windsong", player, player -> talent.windsong, options_str )
  { }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.windsong -> trigger();
  }
};

struct boulderfist_t : public shaman_spell_t
{
  boulderfist_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "boulderfist", player, player -> talent.boulderfist, options_str )
  {
    base_multiplier *= 1.0 + player -> artifact.weapons_of_the_elements.percent();

    maelstrom_gain += player -> artifact.gathering_of_the_maelstrom.value();
  }

  void init() override
  {
    shaman_spell_t::init();

    // TODO: SpellCategory + SpellEffect based detection
    cooldown -> hasted = true;
  }

  void execute() override
  {
    p() -> buff.landslide-> trigger();

    shaman_spell_t::execute();

    p() -> buff.boulderfist -> trigger();
  }
};

struct fury_of_air_aoe_t : public shaman_attack_t
{
  fury_of_air_aoe_t( shaman_t* player ) :
    shaman_attack_t( "fury_of_air_damage", player, player -> find_spell( 197385 ) )
  {
    background = true;
    aoe = -1;
    school = SCHOOL_NATURE;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_windfury = may_proc_flametongue = may_proc_stormbringer = may_proc_frostbrand = false;
  }
};

struct fury_of_air_t : public shaman_spell_t
{
  fury_of_air_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "fury_of_air", player, player -> talent.fury_of_air, options_str )
  {
    hasted_ticks = callbacks = false;

    tick_action = new fury_of_air_aoe_t( player );
  }

  // Infinite duration, so lets go for twice expected time gimmick.
  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return sim -> expected_iteration_time * 2; }
};

struct earthen_spike_t : public shaman_attack_t
{
  earthen_spike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "earthen_spike", player, player -> talent.earthen_spike )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    shaman_attack_t::impact( s );

    td( target ) -> debuff.earthen_spike -> trigger();
  }
};

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_shield", player, player -> find_talent_spell( "Lightning Shield" ), options_str )
  {
    harmful = false;

    if ( player -> action.lightning_shield )
    {
      add_child( player -> action.lightning_shield );
    }
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.lightning_shield -> trigger();
  }
};

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "bloodlust", player, player -> find_class_spell( "Bloodlust" ), options_str )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> buffs.exhaustion -> check() || p -> is_pet() )
        continue;
      p -> buffs.bloodlust -> trigger();
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready() override
  {
    if ( sim -> overrides.bloodlust )
      return false;

    if ( p() -> buffs.exhaustion -> check() )
      return false;

    if (  p() -> buffs.bloodlust -> cooldown -> down() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning and Lava Beam Spells =========================================

struct chained_overload_base_t: public elemental_overload_spell_t
{
  chained_overload_base_t( shaman_t* p, const std::string& name, const spell_data_t* spell, double mg ) :
    elemental_overload_spell_t( p, name, spell )
  {
    base_multiplier *= 1.0 + p -> artifact.electric_discharge.percent();
    chain_multiplier = data().effectN( 1 ).chain_multiplier();
    energize_type = ENERGIZE_NONE; // disable resource generation from spell data.
    maelstrom_gain = mg;
    radius = 10.0;
  }

  proc_types proc_type() const override
  { return PROC1_SPELL; }

  double action_multiplier() const override
  {
    double m = elemental_overload_spell_t::action_multiplier();

    if ( p() -> buff.stormkeeper -> up() )
    {
      m *= 1.0 + p() -> buff.stormkeeper -> data().effectN( 1 ).percent();
    }

    return m;
  }

  std::vector<player_t*> check_distance_targeting( std::vector< player_t* >& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }

  void impact( action_state_t* state ) override
  {
    elemental_overload_spell_t::impact( state );

    p() -> trigger_lightning_rod_damage( state );
  }
};

struct chain_lightning_overload_t : public chained_overload_base_t
{
  chain_lightning_overload_t( shaman_t* p ) :
    chained_overload_base_t( p, "chain_lightning_overload", p -> find_spell( 45297 ),
        p -> find_spell( 218558 ) -> effectN( 1 ).resource( RESOURCE_MAELSTROM ) )
  { }
};


struct lava_beam_overload_t: public chained_overload_base_t
{
  lava_beam_overload_t( shaman_t* p ) :
    chained_overload_base_t( p, "lava_beam_overload", p -> find_spell( 114738 ),
        p -> find_spell( 218559 ) -> effectN( 1 ).resource( RESOURCE_MAELSTROM ) )
  { }
};

struct chained_base_t : public shaman_spell_t
{
  chained_base_t( shaman_t* player, const std::string& name, const spell_data_t* spell, double mg, const std::string& options_str ):
    shaman_spell_t( name, player, spell, options_str )
  {
    base_multiplier *= 1.0 + player -> artifact.electric_discharge.percent();
    chain_multiplier = data().effectN( 1 ).chain_multiplier();
    radius = 10.0;

    maelstrom_gain = mg;
    energize_type = ENERGIZE_NONE; // disable resource generation from spell data.

    if ( data().affected_by( player -> spec.chain_lightning_2 -> effectN( 1 ) ) )
    {
      aoe += player -> spec.chain_lightning_2 -> effectN( 1 ).base_value();
    }
  }

  // Make Chain Lightning a single target spell for procs
  proc_types proc_type() const override
  { return PROC1_SPELL; }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p() -> buff.stormkeeper -> up() )
    {
      m *= 1.0 + p() -> buff.stormkeeper -> data().effectN( 1 ).percent();
    }

    return m;
  }

  double overload_chance( const action_state_t* s ) const override
  {
    if ( p() -> buff.static_overload -> check() )
    {
      return 1.0;
    }

    return shaman_spell_t::overload_chance( s ) * .2;
  }

  void execute() override
  {
    // Roll for static overload buff before execute
    p() -> buff.static_overload -> trigger();

    shaman_spell_t::execute();

    p() -> buff.stormkeeper -> decrement();
    p() -> buff.static_overload -> decrement();
    p() -> trigger_t18_4pc_elemental();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( state -> chain_target == 0 )
    {
      td( state -> target ) -> debuff.lightning_rod -> trigger();
    }
    p() -> trigger_lightning_rod_damage( state );
  }

  std::vector<player_t*> check_distance_targeting( std::vector< player_t* >& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }
};

struct chain_lightning_t: public chained_base_t
{
  chain_lightning_t( shaman_t* player, const std::string& options_str ):
    chained_base_t( player, "chain_lightning", player -> find_specialization_spell( "Chain Lightning" ),
      player -> find_specialization_spell( "Chain Lightning" ) -> effectN( 2 ).base_value(), options_str )
  {
    if ( player -> mastery.elemental_overload -> ok() )
    {
      overload = new chain_lightning_overload_t( player );
      add_child( overload );
    }
  }

  timespan_t execute_time() const override
  {
    if (p()->buff.stormkeeper->up())
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::execute_time();
  }

  bool ready() override
  {
    if ( p() -> specialization() == SHAMAN_ELEMENTAL && p() -> buff.ascendance -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

struct lava_beam_t : public chained_base_t
{
  lava_beam_t( shaman_t* player, const std::string& options_str ) :
    chained_base_t( player, "lava_beam", player -> find_spell( 114074 ),
        player -> find_spell( 114074 ) -> effectN( 3 ).base_value(), options_str )
  {
    if ( player -> mastery.elemental_overload -> ok() )
    {
      overload = new lava_beam_overload_t( player );
      add_child( overload );
    }
  }

  bool ready() override
  {
    if ( ! p() -> buff.ascendance -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_overload_t : public elemental_overload_spell_t
{
  lava_burst_overload_t( shaman_t* p ) :
    elemental_overload_spell_t( p, "lava_burst_overload", p -> find_spell( 77451 ) )
  {
    base_multiplier *= 1.0 + p -> artifact.lava_imbued.percent();
    base_multiplier *= 1.0 + p -> talent.path_of_flame -> effectN( 1 ).percent();
    // TODO: Additive with Elemental Fury? Spell data claims same effect property, so probably ..
    crit_bonus_multiplier += p -> artifact.molten_blast.percent();
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p() -> buff.ascendance -> up() )
    {
      m *= 1.0 + p() -> cache.spell_crit_chance();
    }

    return m;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_crit_chance( t );

    if ( p() -> spec.lava_burst_2 -> ok() &&
         td( target ) -> dot.flame_shock -> is_ticking() )
    {
      m = 1.0;
    }

    return m;
  }
};

struct flame_shock_spreader_t : public shaman_spell_t
{
  flame_shock_spreader_t( shaman_t* p ) :
    shaman_spell_t( "flame_shock_spreader", p )
  {
    quiet = background = true;
    may_miss = may_crit = callbacks = false;
  }

  player_t* shortest_duration_target() const
  {
    player_t* copy_target = nullptr;
    timespan_t min_remains = timespan_t::zero();

    for ( auto t : sim -> target_non_sleeping_list )
    {
      // Skip source target
      if ( t == target )
      {
        continue;
      }

      // Skip targets that are further than 8 yards from the original target
      if ( sim -> distance_targeting_enabled &&
           t -> get_player_distance( *target ) > 8 + t -> combat_reach )
      {
        continue;
      }

      shaman_td_t* target_td = td( t );
      if ( min_remains == timespan_t::zero() || min_remains > target_td -> dot.flame_shock -> remains() )
      {
        min_remains =  target_td -> dot.flame_shock -> remains();
        copy_target = t;
      }
    }

    if ( copy_target && sim -> debug )
    {
      sim -> out_debug.printf( "%s path_of_flame spreads flame_shock from %s to shortest remaining target %s (remains=%.3f)",
        player -> name(), target -> name(), copy_target -> name(), min_remains.total_seconds() );
    }

    return copy_target;
  }

  player_t* closest_target() const
  {
    player_t* copy_target = nullptr;
    double min_distance = -1;

    for ( auto t : sim -> target_non_sleeping_list )
    {
      // Skip source target
      if ( t == target )
      {
        continue;
      }

      double distance = 0;
      if ( sim -> distance_targeting_enabled )
      {
        distance = t -> get_player_distance( *target );
      }

      // Skip targets that are further than 8 yards from the original target
      if ( sim -> distance_targeting_enabled && distance > 8 + t -> combat_reach )
      {
        continue;
      }

      shaman_td_t* target_td = td( t );
      if ( target_td -> dot.flame_shock -> is_ticking() )
      {
        continue;
      }

      // If we are not using distance-based targeting, just return the first available target
      if ( ! sim -> distance_targeting_enabled )
      {
        copy_target = t;
        break;
      }
      else if ( min_distance < 0 || min_distance > distance )
      {
        min_distance = distance;
        copy_target = t;
      }
    }

    if ( copy_target && sim -> debug )
    {
      sim -> out_debug.printf( "%s path_of_flame spreads flame_shock from %s to closest target %s (distance=%.3f)",
        player -> name(), target -> name(), copy_target -> name(), min_distance );
    }

    return copy_target;
  }

  void execute() override
  {
    shaman_td_t* source_td = td( target );
    player_t* copy_target = nullptr;
    if ( ! source_td -> dot.flame_shock -> is_ticking() )
    {
      return;
    }

    // If all targets have flame shock, pick the shortest remaining time
    if ( player -> get_active_dots( source_td -> dot.flame_shock -> current_action -> internal_id ) ==
         sim -> target_non_sleeping_list.size() )
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
      source_td -> dot.flame_shock -> copy( copy_target, DOT_COPY_CLONE );
    }
  }
};

struct lava_burst_t : public shaman_spell_t
{
  flame_shock_spreader_t* spreader;

  lava_burst_t( shaman_t* player, const std::string& options_str ):
    shaman_spell_t( "lava_burst", player, player -> find_specialization_spell( "Lava Burst" ), options_str ),
    spreader( player -> talent.path_of_flame -> ok() ? new flame_shock_spreader_t( player ) : nullptr )
  {
    base_multiplier *= 1.0 + player -> artifact.lava_imbued.percent();
    base_multiplier *= 1.0 + player -> talent.path_of_flame -> effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + player -> artifact.molten_blast.percent();

    // Manacost is only for resto
    base_costs[ RESOURCE_MANA ] = 0;

    if ( player -> mastery.elemental_overload -> ok() )
    {
      overload = new lava_burst_overload_t( player );
      add_child( overload );
    }
  }

  void init() override
  {
    shaman_spell_t::init();

    if ( p() -> specialization() == SHAMAN_ELEMENTAL &&
         p() -> talent.echo_of_the_elements -> ok() )
    {
      cooldown -> charges = data().charges() +
        p() -> talent.echo_of_the_elements -> effectN( 2 ).base_value();
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p() -> buff.ascendance -> up() )
    {
      m *= 1.0 + p() -> cache.spell_crit_chance();
    }

    return m;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_crit_chance( t );

    if ( p() -> spec.lava_burst_2 -> ok() &&
         td( target ) -> dot.flame_shock -> is_ticking() )
    {
      m = 1.0;
    }

    return m;
  }

  void update_ready( timespan_t /* cd_duration */ ) override
  {
    timespan_t d = cooldown -> duration;

    // Lava Surge has procced during the cast of Lava Burst, the cooldown
    // reset is deferred to the finished cast, instead of "eating" it.
    if ( p() -> lava_surge_during_lvb )
    {
      d = timespan_t::zero();
      cooldown -> last_charged = sim -> current_time();
    }

    shaman_spell_t::update_ready( d );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    // Lava Surge buff does not get eaten, if the Lava Surge proc happened
    // during the Lava Burst cast
    if ( ! p() -> lava_surge_during_lvb && p() -> buff.lava_surge -> check() )
      p() -> buff.lava_surge -> expire();

    p() -> lava_surge_during_lvb = false;
    p() -> cooldown.fire_elemental -> adjust( p() -> artifact.elementalist.time_value() );
    p() -> cooldown.storm_elemental -> adjust( p() -> artifact.elementalist.time_value() );

    p() -> buff.power_of_the_maelstrom -> trigger( p() -> buff.power_of_the_maelstrom -> max_stack() );
  }

  timespan_t execute_time() const override
  {
    if ( p() -> buff.lava_surge -> up() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::execute_time();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( spreader && sim -> target_non_sleeping_list.size() > 1 )
      {
        spreader -> target = state -> target;
        spreader -> execute();
      }

      if ( rng().roll( p() -> artifact.volcanic_inferno.data().proc_chance() ) )
      {
        make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
            .target( state -> target )
            .duration( p() -> find_spell( 199121 ) -> duration() )
            .action( p() -> action.volcanic_inferno ) );
      }

      // Pristine Proto-Scale Girdle legendary
      if ( p() -> action.ppsg )
      {
        p() -> action.ppsg -> target = state -> target;
        p() -> action.ppsg -> schedule_execute();
      }
    }
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_overload_t : public elemental_overload_spell_t
{
  lightning_bolt_overload_t( shaman_t* p ) :
    elemental_overload_spell_t( p, "lightning_bolt_overload", p -> find_spell( 45284 ) )
  {
    base_multiplier *= 1.0 + p -> artifact.surge_of_power.percent();
    maelstrom_gain = player -> find_spell( 214816 ) -> effectN( 1 ).resource( RESOURCE_MAELSTROM );
  }

  double action_multiplier() const override
  {
    double m = elemental_overload_spell_t::action_multiplier();

    if ( p() -> buff.stormkeeper -> up() )
    {
      m *= 1.0 + p() -> buff.stormkeeper -> data().effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    elemental_overload_spell_t::impact( state );

    p() -> trigger_lightning_rod_damage( state );
  }
};

struct lightning_bolt_t : public shaman_spell_t
{
  double m_overcharge;

  lightning_bolt_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_bolt", player, player -> find_specialization_spell( "Lightning Bolt" ), options_str ),
    m_overcharge( 0 )
  {
    base_multiplier *= 1.0 + player -> artifact.surge_of_power.percent();

    if ( player -> specialization() == SHAMAN_ELEMENTAL )
    {
      maelstrom_gain = player -> find_spell( 214815 ) -> effectN( 1 ).resource( RESOURCE_MAELSTROM );
    }

    if ( player -> talent.overcharge -> ok() )
    {
      cooldown -> duration += player -> talent.overcharge -> effectN( 3 ).time_value();
      m_overcharge = player -> talent.overcharge -> effectN( 2 ).percent() /
        player -> talent.overcharge -> effectN( 1 ).base_value();
    }

    // TODO: Is it still 10% per Maelstrom with Stormbringer?
    if ( player -> talent.overcharge -> ok() )
    {
      secondary_costs[ RESOURCE_MAELSTROM ] = player -> talent.overcharge -> effectN( 1 ).base_value();
      resource_current = RESOURCE_MAELSTROM;
    }

    if ( player -> mastery.elemental_overload -> ok() )
    {
      overload = new lightning_bolt_overload_t( player );
      add_child( overload );
    }

    if ( p() -> artifact.doom_vortex.rank() )
    {
      add_child( p() -> action.doom_vortex_lb );
    }
  }

  double overload_chance( const action_state_t* s ) const override
  {
    double chance = shaman_spell_t::overload_chance( s );
    chance += p() -> buff.storm_totem -> value();

    return chance;
  }

  size_t n_overloads( const action_state_t* ) const override
  {
    return p() -> buff.power_of_the_maelstrom -> up();
  }

  double spell_direct_power_coefficient( const action_state_t* /* state */ ) const override
  {
    return spell_power_mod.direct * ( 1.0 + m_overcharge * cost() );
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p() -> buff.stormkeeper -> up() )
    {
      m *= 1.0 + p() -> buff.stormkeeper -> data().effectN( 1 ).percent();
    }

    return m;
  }
  
  timespan_t execute_time() const override
  {
    if ( p() -> buff.stormkeeper -> up())
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::execute_time();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.stormkeeper -> decrement();
    p() -> buff.power_of_the_maelstrom -> decrement();

    // Additional check here for lightning bolt
    if ( p() -> talent.overcharge -> ok() )
    {
      p() -> trigger_doom_vortex( execute_state );
    }

    p() -> trigger_t19_oh_8pc( execute_state );
    p() -> trigger_t18_4pc_elemental();

    if ( ! p() -> talent.overcharge -> ok() &&
         p() -> specialization() == SHAMAN_ENHANCEMENT )
    {
      reset_swing_timers();
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( state -> chain_target == 0 )
    {
      td( state -> target ) -> debuff.lightning_rod -> trigger();
    }
    p() -> trigger_lightning_rod_damage( state );
  }

  void reset_swing_timers()
  {
    if ( player -> main_hand_attack &&
         player -> main_hand_attack -> execute_event )
    {
      event_t::cancel( player -> main_hand_attack -> execute_event );
      player -> main_hand_attack -> schedule_execute();
    }

    if ( player -> off_hand_attack &&
         player -> off_hand_attack -> execute_event )
    {
      event_t::cancel( player -> off_hand_attack -> execute_event );
      player -> off_hand_attack -> schedule_execute();
    }
  }
};

// Elemental Blast Spell ====================================================

void trigger_elemental_blast_proc( shaman_t* p )
{
  unsigned b = static_cast< unsigned >( p -> rng().range( 0, 3 ) );

  if ( b == 0 )
    p -> buff.elemental_blast_crit -> trigger();
  else if ( b == 1 )
    p -> buff.elemental_blast_haste -> trigger();
  else if ( b == 2 )
    p -> buff.elemental_blast_mastery -> trigger();
}

struct elemental_blast_overload_t : public elemental_overload_spell_t
{
  elemental_blast_overload_t( shaman_t* p ) :
    elemental_overload_spell_t( p, "elemental_blast_overload", p -> find_spell( 120588 ) )
  { }

  void execute() override
  {
    // Trigger buff before executing the spell, because apparently the buffs affect the cast result
    // itself.
    trigger_elemental_blast_proc( p() );

    shaman_spell_t::execute();
  }
};

struct elemental_blast_t : public shaman_spell_t
{
  elemental_blast_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_blast", player, player -> talent.elemental_blast, options_str )
  {
    if ( player -> mastery.elemental_overload -> ok() )
    {
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
  icefury_overload_t( shaman_t* p ) :
    elemental_overload_spell_t( p, "icefury_overload", p -> find_spell( 219271 ) )
  { }
};

struct icefury_t : public shaman_spell_t
{
  icefury_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "icefury", player, player -> talent.icefury, options_str )
  {
    if ( player -> mastery.elemental_overload -> ok() )
    {
      overload = new icefury_overload_t( player );
      add_child( overload );
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.icefury -> trigger( data().initial_stacks() );
  }
};

// Shamanistic Rage Spell ===================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  shamanistic_rage_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "shamanistic_rage", player, player -> find_specialization_spell( "Shamanistic Rage" ), options_str )
  {
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.shamanistic_rage -> trigger();
  }
};

// Spirit Wolf Spell ========================================================

struct feral_spirit_spell_t : public shaman_spell_t
{
  const spell_data_t* feral_spirit_summon;
  const spell_data_t* doom_wolves;

  feral_spirit_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "feral_spirit", player, player -> find_specialization_spell( "Feral Spirit" ), options_str ),
      feral_spirit_summon( player -> find_spell( 228562 ) ),
      doom_wolves( player -> artifact.doom_wolves.rank() ? player -> find_spell( 198506 ) : spell_data_t::not_found() )
  {
    harmful = false;
  }

  double recharge_multiplier() const override
  {
    double m = shaman_spell_t::recharge_multiplier();

    if ( p() -> buff.spiritual_journey -> up() )
    {
      m *= 1.0 - p() -> buff.spiritual_journey -> data().effectN( 1 ).percent();
    }

    return m;
  }

  timespan_t summon_duration() const
  {
    if ( doom_wolves -> ok() )
    {
      return doom_wolves -> duration();
    }

    return feral_spirit_summon -> duration();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p() -> artifact.doom_wolves.rank() )
    {
      size_t n = static_cast<size_t>( data().effectN( 1 ).base_value() );
      while ( n )
      {
        size_t idx = static_cast<size_t>( rng().range( 0, p() -> pet.doom_wolves.size() ) );
        if ( ! p() -> pet.doom_wolves[ idx ] -> is_sleeping() )
        {
          continue;
        }

        p() -> pet.doom_wolves[ idx ] -> summon( summon_duration() );
        n--;
      }
    }
    else
    {
      range::for_each( p() -> pet.spirit_wolves, [ this ]( pet_t* p ) {
        p -> summon( summon_duration() );
      } );
      p() -> buff.feral_spirit -> trigger();
    }
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  thunderstorm_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "thunderstorm", player, player -> find_specialization_spell( "Thunderstorm" ), options_str )
  {
    aoe = -1;
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "spiritwalkers_grace", player, player -> find_specialization_spell( "Spiritwalker's Grace" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.spiritwalkers_grace -> trigger();
  }
};

struct spirit_walk_t : public shaman_spell_t
{
  spirit_walk_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "spirit_walk", player, player -> find_specialization_spell( "Spirit Walk" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.spirit_walk -> trigger();
  }
};

struct doom_winds_t : public shaman_spell_t
{
  doom_winds_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "doom_winds", player, player -> artifact.doom_winds, options_str )
  {
    harmful = callbacks = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.doom_winds -> trigger();
  }

  bool ready() override
  {
    if ( ! player -> artifact_enabled() )
    {
      return false;
    }

    if ( p() -> artifact.doom_winds.rank() == 0 )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

struct ghost_wolf_t : public shaman_spell_t
{
  ghost_wolf_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "ghost_wolf", player, player -> find_class_spell( "Ghost Wolf" ), options_str )
  {
    unshift_ghost_wolf = false; // Customize unshifting logic here
    harmful = callbacks = may_crit = false;
  }

  timespan_t gcd() const override
  {
    if ( p() -> buff.ghost_wolf -> check() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::gcd();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( ! p() -> buff.ghost_wolf -> check() )
    {
      p() -> buff.ghost_wolf -> trigger();
    }
    else
    {
      p() -> buff.ghost_wolf -> expire();
    }
  }
};

struct feral_lunge_t : public shaman_spell_t
{
  struct feral_lunge_attack_t : public shaman_attack_t
  {
    feral_lunge_attack_t( shaman_t* p ) :
      shaman_attack_t( "feral_lunge_attack", p, p -> find_spell( 215802 ) )
    {
      background = true;
      callbacks = may_proc_windfury = may_proc_frostbrand = may_proc_flametongue = may_proc_maelstrom_weapon = false;
    }
  };

  feral_lunge_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "feral_lunge", player, player -> talent.feral_lunge, options_str )
  {
    unshift_ghost_wolf = false;

    impact_action = new feral_lunge_attack_t( player );
  }

  void init() override
  {
    shaman_spell_t::init();

    may_proc_unleash_doom = false;
  }

  bool ready() override
  {
    if ( ! p() -> buff.ghost_wolf -> check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

struct storm_elemental_t : public shaman_spell_t
{
  const spell_data_t* summon_spell;

  storm_elemental_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "storm_elemental", player, player -> talent.storm_elemental, options_str ),
    summon_spell( player -> find_spell( 157299 ) )
  {
    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p() -> talent.primal_elementalist -> ok() )
    {
      p() -> pet.pet_storm_elemental -> summon( summon_spell -> duration() );
    }
    else
    {
      p() -> pet.guardian_storm_elemental -> summon( summon_spell -> duration() );
    }
  }
};

// Earthquake totem =========================================================

struct earthquake_damage_t : public shaman_spell_t
{
  earthquake_damage_t( shaman_t* player ) :
    shaman_spell_t( "earthquake_", player, player -> find_spell( 77478 ) )
  {
    aoe = -1;
    ground_aoe = background = true;
    school = SCHOOL_PHYSICAL;
    spell_power_mod.direct = 0.3; // Hardcoded into tooltip because it's cool
    base_multiplier *= 1.0 + p() -> artifact.the_ground_trembles.percent();
  }

  double target_armor( player_t* ) const override
  { return 0; }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );

    if ( p() -> buff.echoes_of_the_great_sundering -> up() )
    {
      m *= 1.0 + p() -> buff.echoes_of_the_great_sundering -> data().effectN( 2 ).percent();
    }

    return m;
  }
};

struct earthquake_t : public shaman_spell_t
{
  earthquake_damage_t* rumble;

  earthquake_t( shaman_t* player, const std::string& options_str ):
    shaman_spell_t( "earthquake", player, player -> find_specialization_spell( "Earthquake" ), options_str ),
    rumble( new earthquake_damage_t( player ) )
  {
    dot_duration = timespan_t::zero(); // The periodic effect is handled by ground_aoe_event_t
    add_child( rumble );
  }

  double cost() const override
  {
    if ( p() -> buff.echoes_of_the_great_sundering -> check() )
    {
      return 0;
    }

    return shaman_spell_t::cost();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        .duration( data().duration() )
        .action( rumble )
        .hasted( ground_aoe_params_t::SPELL_HASTE ), true );

    // Note, needs to be decremented after ground_aoe_event_t is created so that the rumble gets the
    // buff multiplier as persistent.
    p() -> buff.echoes_of_the_great_sundering -> decrement();
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_mastery", player, player -> talent.elemental_mastery, options_str )
  {
    harmful   = false;
    may_crit  = false;
    may_miss  = false;
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.elemental_mastery -> trigger();
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================

struct earth_shock_t : public shaman_spell_t
{
  double base_coefficient;
  double eotgs_base_chance; // 7.0 legendary Echoes of the Great Sundering proc chance
  double tdbp_proc_chance; // 7.0 legendary The Deceiver's Blood Pact proc chance

  earth_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "earth_shock", player, player -> find_specialization_spell( "Earth Shock" ), options_str ),
    base_coefficient( data().effectN( 1 ).sp_coeff() / base_cost() ), eotgs_base_chance( 0 ),
    tdbp_proc_chance( 0 )
  {
    base_multiplier *= 1.0 + player -> artifact.earthen_attunement.percent();
  }

  double spell_direct_power_coefficient( const action_state_t* ) const override
  { return base_coefficient * cost(); }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p() -> buff.earth_surge -> up() )
    {
      m *= p() -> buff.earth_surge -> check_value();
    }

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p() -> sets.has_set_bonus( SHAMAN_ELEMENTAL, T18, B2 ) &&
         rng().roll( p() -> sets.set( SHAMAN_ELEMENTAL, T18, B2 ) -> effectN( 1 ).percent() ) )
    {
      p() -> resource_gain( RESOURCE_MAELSTROM, resource_consumed, p() -> gain.t18_2pc_elemental, this );
    }

    if ( eotgs_base_chance > 0 )
    {
      p() -> buff.echoes_of_the_great_sundering -> trigger( 1, buff_t::DEFAULT_VALUE(),
          eotgs_base_chance * resource_consumed );
    }

    if ( rng().roll( tdbp_proc_chance ) )
    {
      p() -> resource_gain( RESOURCE_MAELSTROM, resource_consumed, p() -> gain.the_deceivers_blood_pact, this );
    }
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  double duration_multiplier; // Elemental Bellows
  timespan_t duration_per_maelstrom;

  flame_shock_t( shaman_t* player, const std::string& options_str = std::string()  ) :
    shaman_spell_t( "flame_shock", player, player -> find_specialization_spell( "Flame Shock" ), options_str ),
    duration_multiplier( 1.0 ), duration_per_maelstrom( timespan_t::zero() )
  {
    tick_may_crit         = true;
    track_cd_waste        = false;
    base_multiplier *= 1.0 + player -> artifact.firestorm.percent();

    if ( player -> spec.flame_shock_2 -> ok() )
    {
      resource_current = RESOURCE_MAELSTROM;
      secondary_costs[ RESOURCE_MAELSTROM ] = player -> spec.flame_shock_2 -> effectN( 1 ).base_value();
      cooldown -> duration = timespan_t::zero(); // TODO: A mystery 6 second cooldown appeared out of nowhere

      duration_per_maelstrom = dot_duration / secondary_costs[ RESOURCE_MAELSTROM ];
    }
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return ( dot_duration + duration_per_maelstrom * cost() ) * duration_multiplier; }

  double action_ta_multiplier() const override
  {
    double m = shaman_spell_t::action_ta_multiplier();

    if ( p() -> buff.ember_totem -> up() )
    {
      m *= p () -> buff.ember_totem -> check_value();
    }

    return m;
  }

  void tick( dot_t* d ) override
  {
    shaman_spell_t::tick( d );

    double proc_chance = p() -> spec.lava_surge -> proc_chance();
    proc_chance += p() -> talent.elemental_fusion -> effectN( 1 ).percent();

    if ( rng().roll( proc_chance ) )
    {
      if ( p() -> buff.lava_surge -> check() )
        p() -> proc.wasted_lava_surge -> occur();

      p() -> proc.lava_surge -> occur();
      if ( ! p() -> executing || p() -> executing -> id != 51505 )
        p() -> cooldown.lava_burst -> reset( true );
      else
      {
        p() -> proc.surge_during_lvb -> occur();
        p() -> lava_surge_during_lvb = true;
      }

      p() -> buff.lava_surge -> trigger();
    }
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  double damage_coefficient;

  frost_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "frost_shock", player, player -> find_specialization_spell( "Frost Shock" ), options_str ),
    damage_coefficient( data().effectN( 3 ).percent() / secondary_costs[ RESOURCE_MAELSTROM ] )
  {
    base_multiplier *= 1.0 + player -> artifact.earthen_attunement.percent();
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p() -> buff.earth_surge -> up() )
    {
      m *= p() -> buff.earth_surge -> check_value();
    }

    m *= 1.0 + cost() * damage_coefficient;

    m *= 1.0 + p() -> buff.icefury -> value();

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.icefury -> decrement();
  }

};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "wind_shear", player, player -> find_specialization_spell( "Wind Shear" ), options_str )
  {
    may_miss = may_crit = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }
};

// Ascendancy Spell =========================================================

struct ascendance_t : public shaman_spell_t
{
  ascendance_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "ascendance", player, player -> talent.ascendance, options_str )
  {
    harmful = false;
    // Periodic effect for Enhancement handled by the buff
    dot_duration = base_tick_time = timespan_t::zero();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> cooldown.strike -> reset( false );
    p() -> buff.ascendance -> trigger();
  }
};

// Stormkeeper Spell ========================================================

struct stormkeeper_t : public shaman_spell_t
{
  stormkeeper_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "stormkeeper", player, player -> artifact.stormkeeper, options_str )
  {
    may_crit = harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.stormkeeper -> trigger( p() -> buff.stormkeeper -> data().initial_stacks() );
    if ( p() -> artifact.fury_of_the_storms.rank() )
    {
      p() -> pet.guardian_greater_lightning_elemental -> summon(
          p() -> spell.fury_of_the_storms_driver -> duration() );
    }
  }
};

// Totemic Mastery Spell ====================================================

struct totem_mastery_t : public shaman_spell_t
{
  totem_mastery_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "totem_mastery", player, player -> talent.totem_mastery, options_str )
  {
    harmful = may_crit = callbacks = may_miss = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.resonance_totem -> trigger();
    p() -> buff.storm_totem -> trigger();
    p() -> buff.ember_totem -> trigger();
    p() -> buff.tailwind_totem -> trigger();
  }
};

// Healing Surge Spell ======================================================

struct healing_surge_t : public shaman_heal_t
{
  healing_surge_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Healing Surge" ), options_str )
  {
    resurgence_gain = 0.6 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  double composite_crit_chance() const override
  {
    double c = shaman_heal_t::composite_crit_chance();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c += p() -> spec.tidal_waves -> effectN( 1 ).percent();
    }

    return c;
  }
};

// Healing Wave Spell =======================================================

struct healing_wave_t : public shaman_heal_t
{
  healing_wave_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Healing Wave" ), options_str )
  {
    resurgence_gain = p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const override
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c *= 1.0 - p() -> spec.tidal_waves -> effectN( 1 ).percent();
    }

    return c;
  }
};

// Greater Healing Wave Spell ===============================================

struct greater_healing_wave_t : public shaman_heal_t
{
  greater_healing_wave_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Greater Healing Wave" ), options_str )
  {
    resurgence_gain = p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const override
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c *= 1.0 - p() -> spec.tidal_waves -> effectN( 1 ).percent();
    }

    return c;
  }
};

// Riptide Spell ============================================================

struct riptide_t : public shaman_heal_t
{
  riptide_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Riptide" ), options_str )
  {
    resurgence_gain = 0.6 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }
};

// Chain Heal Spell =========================================================

struct chain_heal_t : public shaman_heal_t
{
  chain_heal_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Chain Heal" ), options_str )
  {
    resurgence_gain = 0.333 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  double composite_target_da_multiplier( player_t* t) const override
  {
    double m = shaman_heal_t::composite_target_da_multiplier( t );

    if ( td( t ) -> heal.riptide -> is_ticking() )
      m *= 1.0 + p() -> spec.riptide -> effectN( 3 ).percent();

    return m;
  }
};

// Healing Rain Spell =======================================================

struct healing_rain_t : public shaman_heal_t
{
  struct healing_rain_aoe_tick_t : public shaman_heal_t
  {
    healing_rain_aoe_tick_t( shaman_t* player ) :
      shaman_heal_t( "healing_rain_tick", player, player -> find_spell( 73921 ) )
    {
      background = true;
      aoe = -1;
    }
  };

  healing_rain_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Healing Rain" ), options_str )
  {
    base_tick_time = data().effectN( 2 ).period();
    dot_duration = data().duration();
    hasted_ticks = false;
    tick_action = new healing_rain_aoe_tick_t( player );
  }
};

// ==========================================================================
// Shaman Totem System
// ==========================================================================

struct shaman_totem_pet_t : public pet_t
{
  // Pulse related functionality
  totem_pulse_action_t* pulse_action;
  event_t*         pulse_event;
  timespan_t            pulse_amplitude;

  // Summon related functionality
  std::string           pet_name;
  pet_t*                summon_pet;

  shaman_totem_pet_t( shaman_t* p, const std::string& n ) :
    pet_t( p -> sim, p, n, true ),
    pulse_action( nullptr ), pulse_event( nullptr ), pulse_amplitude( timespan_t::zero() ),
    summon_pet( nullptr )
  {
    regen_type = REGEN_DISABLED;
    affects_wod_legendary_ring = false;
  }

  virtual void summon( timespan_t = timespan_t::zero() ) override;
  virtual void dismiss( bool expired = false ) override;

  bool init_finished() override
  {
    if ( ! pet_name.empty() )
    {
      summon_pet = owner -> find_pet( pet_name );
    }

    return pet_t::init_finished();
  }

  shaman_t* o()
  { return debug_cast< shaman_t* >( owner ); }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = owner -> cache.player_multiplier( school );

    // Shaman offensive totems double-dip on the legendary AOE ring damage buff, but do not
    // contribute to explosion damage.
    if ( owner -> buffs.legendary_aoe_ring && owner -> buffs.legendary_aoe_ring -> up() )
    {
      m *= 1.0 + owner -> buffs.legendary_aoe_ring -> default_value;
    }

    return m;
  }

  virtual double composite_spell_hit() const override
  { return owner -> cache.spell_hit(); }

  virtual double composite_spell_crit_chance() const override
  { return owner -> cache.spell_crit_chance(); }

  virtual double composite_spell_power( school_e school ) const override
  { return owner -> cache.spell_power( school ); }

  virtual double composite_spell_power_multiplier() const override
  { return owner -> composite_spell_power_multiplier(); }

  virtual expr_t* create_expression( action_t* a, const std::string& name ) override
  {
    if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, duration );

    return pet_t::create_expression( a, name );
  }
};

struct shaman_totem_t : public shaman_spell_t
{
  shaman_totem_pet_t* totem_pet;
  timespan_t totem_duration;

  shaman_totem_t( const std::string& totem_name, shaman_t* player, const std::string& options_str, const spell_data_t* spell_data ) :
    shaman_spell_t( totem_name, player, spell_data, options_str ), totem_pet(nullptr),
    totem_duration( data().duration() )
  {
    harmful = callbacks = may_miss = may_crit =  false;
    ignore_false_positive = true;
    dot_duration = timespan_t::zero();
  }

  bool init_finished() override
  {
    totem_pet = debug_cast< shaman_totem_pet_t* >( player -> find_pet( name() ) );

    return shaman_spell_t::init_finished();
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();
    totem_pet -> summon( totem_duration );
  }

  virtual expr_t* create_expression( const std::string& name ) override
  {
    // Redirect active/remains to "pet.<totem name>.active/remains" so things work ok with the
    // pet initialization order shenanigans. Otherwise, at this point in time (when
    // create_expression is called), the pets don't actually exist yet.
    if ( util::str_compare_ci( name, "active" ) )
      return player -> create_expression( this, "pet." + name_str + ".active" );
    else if ( util::str_compare_ci( name, "remains" ) )
      return player -> create_expression( this, "pet." + name_str + ".remains" );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, totem_duration );

    return shaman_spell_t::create_expression( name );
  }

  bool ready() override
  {
    if ( ! totem_pet )
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

  totem_pulse_action_t( const std::string& token, shaman_totem_pet_t* p, const spell_data_t* s ) :
    spell_t( token, p, s ), hasted_pulse( false ), pulse_multiplier( 1.0 ), totem( p )
  {
    may_crit = harmful = background = true;
    callbacks = false;

    crit_bonus_multiplier *= 1.0 + totem -> o() -> spec.elemental_fury -> effectN( 1 ).percent();
  }

  shaman_t* o() const
  { return debug_cast< shaman_t* >( player -> cast_pet() -> owner ); }

  shaman_td_t* td( player_t* target ) const
  { return o() -> get_target_data( target ); }

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
    crit_multiplier *= util::crit_multiplier( totem -> o() -> meta_gem );
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

  totem_pulse_event_t( shaman_totem_pet_t& t, timespan_t amplitude ) :
    event_t( t ),
    totem( &t ), real_amplitude( amplitude )
  {
    if ( totem -> pulse_action -> hasted_pulse )
      real_amplitude *= totem -> cache.spell_speed();

    schedule( real_amplitude );
  }
  virtual const char* name() const override
  { return  "totem_pulse"; }
  virtual void execute() override
  {
    if ( totem -> pulse_action )
      totem -> pulse_action -> execute();

    totem -> pulse_event = make_event<totem_pulse_event_t>( sim(), *totem, totem -> pulse_amplitude );
  }
};

void shaman_totem_pet_t::summon( timespan_t duration )
{
  pet_t::summon( duration );

  if ( pulse_action )
  {
    pulse_action -> pulse_multiplier = 1.0;
    pulse_event = make_event<totem_pulse_event_t>( *sim, *this, pulse_amplitude );
  }

  if ( summon_pet )
    summon_pet -> summon();
}

void shaman_totem_pet_t::dismiss( bool expired )
{
  // Disable last (partial) tick on dismiss, as it seems not to happen in game atm
  if ( pulse_action && pulse_event && expiration && expiration -> remains() == timespan_t::zero() )
  {
    if ( pulse_event -> remains() > timespan_t::zero() )
      pulse_action -> pulse_multiplier = pulse_event -> remains() / debug_cast<totem_pulse_event_t*>( pulse_event ) -> real_amplitude;
    pulse_action -> execute();
  }

  event_t::cancel( pulse_event );

  if ( summon_pet )
    summon_pet -> dismiss();

  pet_t::dismiss( expired );
}

// Liquid Magma totem =======================================================

struct liquid_magma_globule_t : public spell_t
{
  liquid_magma_globule_t( shaman_totem_pet_t* p ) :
    spell_t( "liquid_magma", p, p -> find_spell( 192231 ) )
  {
    aoe = -1;
    background = may_crit = true;
    callbacks = false;
  }
};

struct liquid_magma_totem_pulse_t : public totem_pulse_action_t
{
  liquid_magma_globule_t* globule;

  liquid_magma_totem_pulse_t( shaman_totem_pet_t* totem ) :
    totem_pulse_action_t( "liquid_magma_driver", totem, totem -> find_spell( 192226 ) ),
    globule( new liquid_magma_globule_t( totem ) )
  {
    // TODO: "Random enemies" implicates number of targets
    aoe = 1;
    hasted_pulse = quiet = dual = true;
    dot_duration = timespan_t::zero();
  }

  void impact( action_state_t* state ) override
  {
    totem_pulse_action_t::impact( state );

    globule -> target = state -> target;
    globule -> schedule_execute();
  }
};

struct liquid_magma_totem_t : public shaman_totem_pet_t
{
  liquid_magma_totem_t( shaman_t* owner ):
    shaman_totem_pet_t( owner, "liquid_magma_totem" )
  {
    pulse_amplitude = owner -> find_spell( 192226 ) -> effectN( 1 ).period();
  }

  void init_spells() override
  {
    shaman_totem_pet_t::init_spells();

    pulse_action = new liquid_magma_totem_pulse_t( this );
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
  if ( player -> specialization() == SHAMAN_ENHANCEMENT )
  {
    bool executing = false;
    timespan_t time_to_hit = timespan_t::zero();
    if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
    {
      executing = true;
      time_to_hit = player -> main_hand_attack -> execute_event -> remains();
#ifndef NDEBUG
      if ( time_to_hit < timespan_t::zero() )
      {
        util::fprintf( stderr, "Ascendance %s time_to_hit=%f", player -> main_hand_attack -> name(), time_to_hit.total_seconds() );
        assert( 0 );
      }
#endif
      event_t::cancel( player -> main_hand_attack -> execute_event );
    }

    player -> main_hand_attack = mh;
    if ( executing )
    {
      // Kick off the new main hand attack, by instantly scheduling
      // and rescheduling it to the remaining time to hit. We cannot use
      // normal reschedule mechanism here (i.e., simply use
      // event_t::reschedule() and leave it be), because the rescheduled
      // event would be triggered before the full swing time (of the new
      // auto attack) in most cases.
      player -> main_hand_attack -> base_execute_time = timespan_t::zero();
      player -> main_hand_attack -> schedule_execute();
      player -> main_hand_attack -> base_execute_time = player -> main_hand_attack -> weapon -> swing_time;
      player -> main_hand_attack -> execute_event -> reschedule( time_to_hit );
    }

    if ( player -> off_hand_attack )
    {
      time_to_hit = timespan_t::zero();
      executing = false;

      if ( player -> off_hand_attack -> execute_event )
      {
        executing = true;
        time_to_hit = player -> off_hand_attack -> execute_event -> remains();
#ifndef NDEBUG
        if ( time_to_hit < timespan_t::zero() )
        {
          util::fprintf( stderr, "Ascendance %s time_to_hit=%f", player -> off_hand_attack -> name(), time_to_hit.total_seconds() );
          assert( 0 );
        }
#endif
        event_t::cancel( player -> off_hand_attack -> execute_event );
      }

      player -> off_hand_attack = oh;
      if ( executing )
      {
        // Kick off the new off hand attack, by instantly scheduling
        // and rescheduling it to the remaining time to hit. We cannot use
        // normal reschedule mechanism here (i.e., simply use
        // event_t::reschedule() and leave it be), because the rescheduled
        // event would be triggered before the full swing time (of the new
        // auto attack) in most cases.
        player -> off_hand_attack -> base_execute_time = timespan_t::zero();
        player -> off_hand_attack -> schedule_execute();
        player -> off_hand_attack -> base_execute_time = player -> off_hand_attack -> weapon -> swing_time;
        player -> off_hand_attack -> execute_event -> reschedule( time_to_hit );
      }
    }
  }
  // Elemental simply changes the Lava Burst cooldown, Lava Beam replacement
  // will be handled by action list and ready() in Chain Lightning / Lava
  // Beam
  else if ( player -> specialization() == SHAMAN_ELEMENTAL )
  {
    if ( lava_burst )
    {
      lava_burst -> cooldown -> duration = lvb_cooldown;
      lava_burst -> cooldown -> reset( false );
    }
  }
}

inline bool ascendance_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  shaman_t* p = debug_cast< shaman_t* >( player );

  if ( player -> specialization() == SHAMAN_ELEMENTAL && ! lava_burst )
  {
    lava_burst = player -> find_action( "lava_burst" );
  }

  ascendance( p -> ascendance_mh, p -> ascendance_oh, timespan_t::zero() );
  // Don't record CD waste during Ascendance.
  if ( lava_burst )
  {
    lava_burst -> cooldown -> last_charged = timespan_t::zero();
  }

  return buff_t::trigger( stacks, value, chance, duration );
}

inline void ascendance_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  shaman_t* p = debug_cast< shaman_t* >( player );

  timespan_t lvbcd;
  lvbcd = lava_burst ? lava_burst -> data().charge_cooldown() : timespan_t::zero();

  ascendance( p -> melee_mh, p -> melee_oh, lvbcd );
  // Start CD waste recollection from when Ascendance buff fades, since Lava
  // Burst is guaranteed to be very much ready when Ascendance ends.
  if ( lava_burst )
  {
    lava_burst -> cooldown -> last_charged = sim -> current_time();
  }
  buff_t::expire_override( expiration_stacks, remaining_duration );
}

struct flametongue_buff_t : public buff_t
{
  shaman_t* p;

  flametongue_buff_t( shaman_t* p ) :
    buff_t( buff_creator_t( p, "flametongue", p -> find_specialization_spell( "Flametongue" ) -> effectN( 1 ).trigger() ).refresh_behavior( BUFF_REFRESH_PANDEMIC ) ),
    p( p )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    p -> buff.t18_4pc_enhancement -> decrement();
  }

  void execute( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override
  {
    buff_t::execute( stacks, value, duration );

    p -> buff.t18_4pc_enhancement -> trigger();
  }
};

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "ascendance"              ) return new               ascendance_t( this, options_str );
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "bloodlust"               ) return new                bloodlust_t( this, options_str );
  if ( name == "chain_lightning"         ) return new          chain_lightning_t( this, options_str );
  if ( name == "crash_lightning"         ) return new          crash_lightning_t( this, options_str );
  if ( name == "doom_winds"              ) return new               doom_winds_t( this, options_str );
  if ( name == "earthen_spike"           ) return new            earthen_spike_t( this, options_str );
  if ( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if ( name == "earthquake"              ) return new               earthquake_t( this, options_str );
  if ( name == "elemental_blast"         ) return new          elemental_blast_t( this, options_str );
  if ( name == "elemental_mastery"       ) return new        elemental_mastery_t( this, options_str );
  if ( name == "ghost_wolf"              ) return new               ghost_wolf_t( this, options_str );
  if ( name == "feral_lunge"             ) return new              feral_lunge_t( this, options_str );
  if ( name == "fire_elemental"          ) return new           fire_elemental_t( this, options_str );
  if ( name == "flametongue"             ) return new              flametongue_t( this, options_str );
  if ( name == "flame_shock"             ) return new              flame_shock_t( this, options_str );
  if ( name == "frostbrand"              ) return new               frostbrand_t( this, options_str );
  if ( name == "frost_shock"             ) return new              frost_shock_t( this, options_str );
  if ( name == "fury_of_air"             ) return new              fury_of_air_t( this, options_str );
  if ( name == "icefury"                 ) return new                  icefury_t( this, options_str );
  if ( name == "lava_beam"               ) return new                lava_beam_t( this, options_str );
  if ( name == "lava_burst"              ) return new               lava_burst_t( this, options_str );
  if ( name == "lava_lash"               ) return new                lava_lash_t( this, options_str );
  if ( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options_str );
  if ( name == "lightning_shield"        ) return new         lightning_shield_t( this, options_str );
  if ( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if ( name == "windstrike"              ) return new               windstrike_t( this, options_str );
  if ( name == "feral_spirit"            ) return new       feral_spirit_spell_t( this, options_str );
  if ( name == "boulderfist"             ) return new              boulderfist_t( this, options_str );
  if ( name == "rockbiter"               ) return new                rockbiter_t( this, options_str );
  if ( name == "spirit_walk"             ) return new              spirit_walk_t( this, options_str );
  if ( name == "spiritwalkers_grace"     ) return new      spiritwalkers_grace_t( this, options_str );
  if ( name == "storm_elemental"         ) return new          storm_elemental_t( this, options_str );
  if ( name == "stormkeeper"             ) return new              stormkeeper_t( this, options_str );
  if ( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if ( name == "sundering"               ) return new                sundering_t( this, options_str );
  if ( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
  if ( name == "totem_mastery"           ) return new            totem_mastery_t( this, options_str );
  if ( name == "wind_shear"              ) return new               wind_shear_t( this, options_str );
  if ( name == "windsong"                ) return new                 windsong_t( this, options_str );

  if ( name == "chain_heal"              ) return new               chain_heal_t( this, options_str );
  if ( name == "greater_healing_wave"    ) return new     greater_healing_wave_t( this, options_str );
  if ( name == "healing_rain"            ) return new             healing_rain_t( this, options_str );
  if ( name == "healing_surge"           ) return new            healing_surge_t( this, options_str );
  if ( name == "healing_wave"            ) return new             healing_wave_t( this, options_str );
  if ( name == "riptide"                 ) return new                  riptide_t( this, options_str );

  if ( name == "liquid_magma_totem" )
  {
    return new shaman_totem_t( "liquid_magma_totem", this, options_str, talent.liquid_magma_totem );
  }

  return player_t::create_action( name, options_str );
}

action_t* shaman_t::create_proc_action( const std::string& name, const special_effect_t& effect )
{
  if ( util::str_compare_ci( name, "flurry_of_xuen" ) ) return new shaman_flurry_of_xuen_t( this );
  if ( bugs && util::str_compare_ci( name, "horrific_slam" ) ) return new shaman_spontaneous_appendages_t( this, effect );

  return nullptr;
}

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "primal_fire_elemental"    ) return new pet::fire_elemental_t( this, false );
  if ( pet_name == "greater_fire_elemental"   ) return new pet::fire_elemental_t( this, true );
  if ( pet_name == "primal_storm_elemental"   ) return new pet::storm_elemental_t( this, false );
  if ( pet_name == "greater_storm_elemental"  ) return new pet::storm_elemental_t( this, true );
  if ( pet_name == "primal_earth_elemental"   ) return new pet::earth_elemental_t( this, false );
  if ( pet_name == "greater_earth_elemental"  ) return new pet::earth_elemental_t( this, true );
  if ( pet_name == "liquid_magma_totem"       ) return new liquid_magma_totem_t( this );

  return nullptr;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  if ( talent.primal_elementalist -> ok() )
  {
    if ( find_action( "fire_elemental" )  )
    {
      pet.pet_fire_elemental = create_pet( "primal_fire_elemental" );
    }

    if ( find_action( "earth_elemental" ) )
    {
      pet.pet_earth_elemental = create_pet( "primal_earth_elemental" );
    }

    if ( talent.storm_elemental -> ok() && find_action( "storm_elemental" ) )
    {
      pet.pet_storm_elemental = create_pet( "primal_storm_elemental" );
    }
  }
  else
  {
    if ( find_action( "fire_elemental" ) )
    {
      pet.guardian_fire_elemental = create_pet( "greater_fire_elemental" );
    }

    if ( find_action( "earth_elemental" ) )
    {
      pet.guardian_earth_elemental = create_pet( "greater_earth_elemental" );
    }

    if ( talent.storm_elemental -> ok() && find_action( "storm_elemental" ) )
    {
      pet.guardian_storm_elemental = create_pet( "greater_storm_elemental" );
    }
  }

  if ( talent.liquid_magma_totem -> ok() && find_action( "liquid_magma_totem" ) )
  {
    create_pet( "liquid_magma_totem" );
  }

  if ( artifact.fury_of_the_storms.rank() && find_action( "stormkeeper" ) )
  {
    pet.guardian_greater_lightning_elemental = new pet::greater_lightning_elemental_t( this );
  }

  if ( specialization() == SHAMAN_ENHANCEMENT && find_action( "feral_spirit" ) &&
       ! artifact.doom_wolves.rank() )
  {
    const spell_data_t* fs_data = find_specialization_spell( "Feral Spirit" );
    size_t n_feral_spirits = static_cast<size_t>( fs_data -> effectN( 1 ).base_value() );

    for ( size_t i = 0; i < n_feral_spirits; i++ )
    {
      pet.spirit_wolves[ i ] = new pet::spirit_wolf_t( this );
    }
  }

  if ( find_action( "feral_spirit" ) && artifact.doom_wolves.rank() )
  {
    pet.doom_wolves[ 0 ] = new pet::frost_wolf_t( this );
    pet.doom_wolves[ 1 ] = new pet::frost_wolf_t( this );
    pet.doom_wolves[ 2 ] = new pet::fire_wolf_t( this );
    pet.doom_wolves[ 3 ] = new pet::fire_wolf_t( this );
    pet.doom_wolves[ 4 ] = new pet::lightning_wolf_t( this );
    pet.doom_wolves[ 5 ] = new pet::lightning_wolf_t( this );
  }
}

// shaman_t::create_expression ==============================================

expr_t* shaman_t::create_expression( action_t* a, const std::string& name )
{
  std::vector<std::string> splits = util::string_split( name, "." );

  if ( util::str_compare_ci( splits[ 0 ], "feral_spirit" ) )
  {
    if ( artifact.doom_wolves.rank() && pet.doom_wolves[ 0 ] == nullptr )
    {
      return expr_t::create_constant( name, 0 );
    }
    else if ( ! artifact.doom_wolves.rank() && pet.spirit_wolves[ 0 ] == nullptr )
    {
      return expr_t::create_constant( name, 0 );
    }

    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( name, [ this ]() {
        if ( artifact.doom_wolves.rank() )
        {
          return range::find_if( pet.doom_wolves, []( const pet_t* p ) {
            return ! p -> is_sleeping();
          } ) != pet.doom_wolves.end();
        }
        else
        {
          return range::find_if( pet.spirit_wolves, []( const pet_t* p ) {
            return ! p -> is_sleeping();
          } ) != pet.spirit_wolves.end();
        }
      } );
    }
    else if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      auto max_remains_fn = []( const pet_t* l, const pet_t* r ) {
        if ( ! l -> expiration && r -> expiration )
        {
          return true;
        }
        else if ( l -> expiration && ! r -> expiration )
        {
          return false;
        }
        else if ( ! l -> expiration && ! r -> expiration )
        {
          return false;
        }
        else
        {
          return l -> expiration -> remains() < r -> expiration -> remains();
        }
      };

      return make_fn_expr( name, [ this, &max_remains_fn ]() {
        if ( artifact.doom_wolves.rank() )
        {
          auto it = std::max_element( pet.doom_wolves.begin(), pet.doom_wolves.end(), max_remains_fn );
          return ( *it ) -> expiration ? ( *it ) -> expiration -> remains().total_seconds() : 0;
        }
        else
        {
          auto it = std::max_element( pet.spirit_wolves.begin(), pet.spirit_wolves.end(), max_remains_fn );
          return ( *it ) -> expiration ? ( *it ) -> expiration -> remains().total_seconds() : 0;
        }
      } );
    }
  }

  return player_t::create_expression( a, name );
}

// shaman_t::create_actions =================================================

bool shaman_t::create_actions()
{
  if ( talent.lightning_shield -> ok() )
  {
    action.lightning_shield = new lightning_shield_damage_t( this );
  }

  if ( talent.crashing_storm -> ok() )
  {
    action.crashing_storm = new ground_aoe_spell_t( this, "crashing_storm", find_spell( 210801 ) );
  }

  if ( talent.earthen_rage -> ok() )
  {
    action.earthen_rage = new earthen_rage_driver_t( this );
  }

  if ( talent.lightning_rod -> ok() )
  {
    action.lightning_rod = new lightning_rod_t( this );
  }

  if ( artifact.unleash_doom.rank() )
  {
    action.unleash_doom[ 0 ] = new unleash_doom_spell_t( "unleash_lava", this, find_spell( 199053 ) );
    action.unleash_doom[ 1 ] = new unleash_doom_spell_t( "unleash_lightning", this, find_spell( 199054 ) );
  }

  if ( artifact.doom_vortex.rank() )
  {
    action.doom_vortex_ll = new ground_aoe_spell_t( this, "doom_vortex_ll", find_spell( 199116 ) );
    action.doom_vortex_lb = new ground_aoe_spell_t( this, "doom_vortex_lb", find_spell( 199116 ) );
  }

  if ( artifact.volcanic_inferno.rank() )
  {
    action.volcanic_inferno = new volcanic_inferno_t( this );
  }

  if ( sets.has_set_bonus( SHAMAN_ENHANCEMENT, T18, B2 ) )
  {
    action.electrocute = new electrocute_t( this );
  }

  return player_t::create_actions();
}

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();

  add_option( opt_uint( "stormlash_targets", stormlash_targets ) );
  add_option( opt_bool( "raptor_glyph", raptor_glyph ) );
}

// shaman_t::init_spells ====================================================

void shaman_t::init_spells()
{
  // Generic
  spec.mail_specialization   = find_specialization_spell( "Mail Specialization" );
  constant.matching_gear_multiplier = spec.mail_specialization -> effectN( 1 ).percent();
  spec.shaman                = find_spell( 137038 );

  // Elemental / Restoration
  spec.spiritual_insight     = find_specialization_spell( "Spiritual Insight" );

  // Elemental
  spec.chain_lightning_2     = find_specialization_spell( 231722 );
  spec.elemental_focus       = find_specialization_spell( "Elemental Focus" );
  spec.elemental_fury        = find_specialization_spell( "Elemental Fury" );
  spec.flame_shock_2         = find_specialization_spell( 232643 );
  spec.lava_burst_2          = find_specialization_spell( 231721 );
  spec.lava_surge            = find_specialization_spell( "Lava Surge" );

  // Enhancement
  spec.critical_strikes      = find_specialization_spell( "Critical Strikes" );
  spec.dual_wield            = find_specialization_spell( "Dual Wield" );
  spec.enhancement_shaman    = find_specialization_spell( "Enhancement Shaman" );
  spec.feral_spirit_2        = find_specialization_spell( 231723 );
  spec.flametongue           = find_specialization_spell( "Flametongue" );
  spec.frostbrand            = find_specialization_spell( "Frostbrand" );
  spec.maelstrom_weapon      = find_specialization_spell( "Maelstrom Weapon" );
  spec.stormbringer          = find_specialization_spell( "Stormbringer" );
  spec.stormlash             = find_specialization_spell( "Stormlash" );
  spec.windfury              = find_specialization_spell( "Windfury" );

  // Restoration
  spec.ancestral_awakening   = find_specialization_spell( "Ancestral Awakening" );
  spec.ancestral_focus       = find_specialization_spell( "Ancestral Focus" );
  spec.meditation            = find_specialization_spell( "Meditation" );
  spec.purification          = find_specialization_spell( "Purification" );
  spec.resurgence            = find_specialization_spell( "Resurgence" );
  spec.riptide               = find_specialization_spell( "Riptide" );
  spec.tidal_waves           = find_specialization_spell( "Tidal Waves" );

  // Masteries
  mastery.elemental_overload         = find_mastery_spell( SHAMAN_ELEMENTAL   );
  mastery.enhanced_elements          = find_mastery_spell( SHAMAN_ENHANCEMENT );
  mastery.deep_healing               = find_mastery_spell( SHAMAN_RESTORATION );

  // Talents
  talent.ancestral_swiftness         = find_talent_spell( "Ancestral Swiftness"  );
  talent.gust_of_wind                = find_talent_spell( "Gust of Wind"         );
  talent.ascendance                  = find_talent_spell( "Ascendance"           );

  // Elemental
  talent.path_of_flame               = find_talent_spell( "Path of Flame"        );
  talent.earthen_rage                = find_talent_spell( "Earthen Rage"         );
  talent.totem_mastery               = find_talent_spell( "Totem Mastery"        );

  talent.elemental_blast             = find_talent_spell( "Elemental Blast"      );
  talent.echo_of_the_elements        = find_talent_spell( "Echo of the Elements" );

  talent.elemental_fusion            = find_talent_spell( "Elemental Fusion"     );
  talent.primal_elementalist         = find_talent_spell( "Primal Elementalist"  );
  talent.icefury                     = find_talent_spell( "Icefury"              );

  talent.elemental_mastery           = find_talent_spell( "Elemental Mastery"    );
  talent.storm_elemental             = find_talent_spell( "Storm Elemental"      );
  talent.aftershock                  = find_talent_spell( "Aftershock"           );

  talent.lightning_rod               = find_talent_spell( "Lightning Rod"        );
  talent.liquid_magma_totem          = find_talent_spell( "Liquid Magma Totem"   );

  // Enhancement
  talent.windsong                    = find_talent_spell( "Windsong"             );
  talent.hot_hand                    = find_talent_spell( "Hot Hand"             );
  talent.boulderfist                 = find_talent_spell( "Boulderfist"          );

  talent.feral_lunge                 = find_talent_spell( "Feral Lunge"          );

  talent.lightning_shield            = find_talent_spell( "Lightning Shield"     );
  talent.hailstorm                   = find_talent_spell( "Hailstorm"            );

  talent.tempest                     = find_talent_spell( "Tempest"              );
  talent.overcharge                  = find_talent_spell( "Overcharge"           );
  talent.empowered_stormlash         = find_talent_spell( "Empowered Stormlash"  );

  talent.crashing_storm              = find_talent_spell( "Crashing Storm"       );
  talent.fury_of_air                 = find_talent_spell( "Fury of Air"          );
  talent.sundering                   = find_talent_spell( "Sundering"            );

  talent.landslide                   = find_talent_spell( "Landslide"            );
  talent.earthen_spike               = find_talent_spell( "Earthen Spike"        );

  // Artifact

  // Elemental
  artifact.stormkeeper               = find_artifact_spell( "Stormkeeper"        );
  artifact.surge_of_power            = find_artifact_spell( "Surge of Power"     );
  artifact.call_the_thunder          = find_artifact_spell( "Call the Thunder"   );
  artifact.earthen_attunement        = find_artifact_spell( "Earthen Attunement" );
  artifact.the_ground_trembles       = find_artifact_spell( "The Ground Trembles");
  artifact.lava_imbued               = find_artifact_spell( "Lava Imbued"        );
  artifact.volcanic_inferno          = find_artifact_spell( "Volcanic Inferno"   );
  artifact.static_overload           = find_artifact_spell( "Static Overload"    );
  artifact.electric_discharge        = find_artifact_spell( "Electric Discharge" );
  artifact.master_of_the_elements    = find_artifact_spell( "Master of the Elements" );
  artifact.molten_blast              = find_artifact_spell( "Molten Blast"       );
  artifact.elementalist              = find_artifact_spell( "Elementalist"       );
  artifact.firestorm                 = find_artifact_spell( "Firestorm"          );
  artifact.power_of_the_maelstrom    = find_artifact_spell( "Power of the Maelstrom" );
  artifact.fury_of_the_storms        = find_artifact_spell( "Fury of the Storms" );
  artifact.stormkeepers_power        = find_artifact_spell( "Stormkeeper's Power" );

  // Enhancement
  artifact.doom_winds                = find_artifact_spell( "Doom Winds"         );
  artifact.unleash_doom              = find_artifact_spell( "Unleash Doom"       );
  artifact.raging_storms             = find_artifact_spell( "Raging Storms"      );
  artifact.stormflurry               = find_artifact_spell( "Stormflurry"        );
  artifact.hammer_of_storms          = find_artifact_spell( "Hammer of Storms"   );
  artifact.forged_in_lava            = find_artifact_spell( "Forged in Lava"     );
  artifact.wind_strikes              = find_artifact_spell( "Wind Strikes"       );
  artifact.gathering_storms          = find_artifact_spell( "Gathering Storms"   );
  artifact.gathering_of_the_maelstrom= find_artifact_spell( "Gathering of the Maelstrom" );
  artifact.doom_vortex               = find_artifact_spell( "Doom Vortex"        );
  artifact.spirit_of_the_maelstrom   = find_artifact_spell( "Spirit of the Maelstrom" );
  artifact.doom_wolves               = find_artifact_spell( "Doom Wolves"        );
  artifact.alpha_wolf                = find_artifact_spell( "Alpha Wolf"         );
  artifact.earthshattering_blows     = find_artifact_spell( "Earthshattering Blows" );
  artifact.weapons_of_the_elements   = find_artifact_spell( "Weapons of the Elements" );
  artifact.wind_surge                = find_artifact_spell( "Wind Surge" );

  // Misc spells
  spell.resurgence                   = find_spell( 101033 );
  spell.eruption                     = find_spell( 168556 );
  spell.maelstrom_melee_gain         = find_spell( 187890 );
  spell.fury_of_the_storms_driver    = find_spell( 191716 );

  // Constants
  constant.haste_ancestral_swiftness  = 1.0 / ( 1.0 + talent.ancestral_swiftness -> effectN( 1 ).percent() );

  player_t::init_spells();
}

// shaman_t::init_base ======================================================

void shaman_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  if ( specialization() == SHAMAN_ELEMENTAL || specialization() == SHAMAN_ENHANCEMENT )
    resources.base[ RESOURCE_MAELSTROM ] = 100;

  if ( spec.enhancement_shaman -> ok() )
    resources.base[ RESOURCE_MAELSTROM ] += spec.enhancement_shaman -> effectN( 4 ).base_value();

  base.distance = ( specialization() == SHAMAN_ENHANCEMENT ) ? 3 : 30;
  base.mana_regen_from_spirit_multiplier = spec.meditation -> effectN( 1 ).percent();

  //if ( specialization() == SHAMAN_ENHANCEMENT )
  //  ready_type = READY_TRIGGER;
}

void shaman_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_MAELSTROM ] = 0;
}

// shaman_t::init_scaling ===================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
      scales_with[ STAT_STRENGTH              ] = false;
      scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
      scales_with[ STAT_SPELL_POWER           ] = false;
      scales_with[ STAT_INTELLECT             ] = false;
      break;
    case SHAMAN_RESTORATION:
      scales_with[ STAT_MASTERY_RATING ] = false;
      break;
    default:
      break;
  }
}

// ==========================================================================
// Shaman Ability Triggers
// ==========================================================================

void shaman_t::trigger_stormbringer( const action_state_t* state )
{
  assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr &&
          "Stormbringer called on invalid action type" );
  shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );

  if ( ! attack -> may_proc_stormbringer )
  {
    return;
  }

  if ( rng().roll( attack -> stormbringer_proc_chance() ) )
  {
    buff.stormbringer -> trigger( buff.stormbringer -> max_stack() );
    cooldown.strike -> reset( true );
    buff.wind_strikes -> trigger();
    attack -> proc_sb -> occur();
  }
}

void shaman_t::trigger_hot_hand( const action_state_t* state )
{
  assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr &&
          "Hot Hand called on invalid action type" );
  shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );

  if ( ! attack -> may_proc_hot_hand )
  {
    return;
  }

  if ( ! buff.flametongue -> up() )
  {
    return;
  }

  buff.hot_hand -> trigger();
  attack -> proc_hh -> occur();
}

void shaman_t::trigger_elemental_focus( const action_state_t* state )
{
  if ( ! spec.elemental_focus -> ok() )
  {
    return;
  }

  if ( cooldown.elemental_focus -> down() )
  {
    return;
  }

  if ( state -> result != RESULT_CRIT )
  {
    return;
  }

  buff.elemental_focus -> trigger( buff.elemental_focus -> data().initial_stacks() );
  cooldown.elemental_focus -> start( spec.elemental_focus -> internal_cooldown() );
}

void shaman_t::trigger_lightning_shield( const action_state_t* state )
{
  if ( ! buff.lightning_shield -> up() )
  {
    return;
  }

  if ( ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );
  if ( ! attack -> may_proc_lightning_shield )
  {
    return;
  }

  if ( ! rng().roll( talent.lightning_shield -> proc_chance() ) )
  {
    return;
  }

  action.lightning_shield -> target = state -> target;
  action.lightning_shield -> execute();
  attack -> proc_ls -> occur();
}

// TODO: Target swaps
void shaman_t::trigger_earthen_rage( const action_state_t* state )
{
  if ( ! talent.earthen_rage -> ok() )
    return;

  if ( ! state -> action -> harmful )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  // Molten earth does not trigger itself.
  if ( state -> action == debug_cast<earthen_rage_driver_t*>( action.earthen_rage ) -> nuke )
    return;

  action.earthen_rage -> schedule_execute();
}

void shaman_t::trigger_stormlash( const action_state_t* )
{
  if ( ! spec.stormlash -> ok() )
  {
    return;
  }

  if ( ! real_ppm.stormlash -> trigger() )
  {
    return;
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s triggers stormlash", name() );
  }

  buff.stormlash -> trigger();
}

void shaman_t::trigger_doom_vortex( const action_state_t* state )
{
  if ( ! rng().roll( artifact.doom_vortex.data().proc_chance() ) )
  {
    return;
  }

  make_event<ground_aoe_event_t>( *sim, this, ground_aoe_params_t()
      .target( state -> target )
      .duration( find_spell( 199121 ) -> duration() )
      .action( state -> action -> id == 187837 ? action.doom_vortex_lb : action.doom_vortex_ll ) );
}

void shaman_t::trigger_lightning_rod_damage( const action_state_t* state )
{
  if ( ! talent.lightning_rod -> ok() )
  {
    return;
  }

  if ( state -> action -> result_is_miss( state -> result ) )
  {
    return;
  }

  shaman_td_t* td = get_target_data( state -> target );

  if ( ! td -> debuff.lightning_rod -> up() )
  {
    return;
  }

  double amount = state -> result_amount * td -> debuff.lightning_rod -> check_value();
  action.lightning_rod -> base_dd_min = action.lightning_rod -> base_dd_max = amount;

  // Can't schedule_execute here, since Chain Lightning may trigger immediately on multiple
  // Lightning Rod targets, overriding base_dd_min/max with a different value (that would be used
  // for allt he scheduled damage execute events of Lightning Rod).
  range::for_each( lightning_rods, [ this, amount ]( player_t* t ) {
    action.lightning_rod -> target = t;
    action.lightning_rod -> execute();
  } );
}

void shaman_t::trigger_windfury_weapon( const action_state_t* state )
{
  assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr && "Windfury Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );
  if ( ! attack -> may_proc_windfury )
    return;

  // If doom winds is not up, block all off-hand weapon attacks
  if ( ! buff.doom_winds -> check() && attack -> weapon && attack -> weapon -> slot != SLOT_MAIN_HAND )
    return;
  // If doom winds is up, block all off-hand special weapon attacks
  else if ( buff.doom_winds -> check() && attack -> weapon && attack -> weapon -> slot != SLOT_MAIN_HAND && attack -> special )
    return;

  double proc_chance = spec.windfury -> proc_chance();
  proc_chance += cache.mastery() * mastery.enhanced_elements -> effectN( 4 ).mastery_value();
  // Only autoattacks are guaranteed windfury procs during doom winds
  if ( buff.doom_winds -> up() && ! attack -> special )
  {
    proc_chance = 1.0;
  }

  if ( rng().roll( proc_chance ) )
  {
    action_t* a = nullptr;
    if ( ! state -> action -> weapon || state -> action -> weapon -> slot == SLOT_MAIN_HAND )
    {
      a = windfury_mh;
    }
    else
    {
      a = windfury_oh;
    }

    a -> target = state -> target;
    a -> schedule_execute();
    a -> schedule_execute();
    if ( sets.has_set_bonus( SHAMAN_ENHANCEMENT, PVP, B4 ) )
    {
      a -> schedule_execute();
      a -> schedule_execute();
    }

    attack -> proc_wf -> occur();
  }
}

void shaman_t::trigger_t17_2pc_elemental( int stacks )
{
  if ( ! sets.has_set_bonus( SHAMAN_ELEMENTAL, T17, B2 ) )
    return;

  buff.focus_of_the_elements -> trigger( 1, sets.set( SHAMAN_ELEMENTAL, T17, B2 ) -> effectN( 1 ).percent() * stacks );
}

void shaman_t::trigger_t17_4pc_elemental( int stacks )
{
  if ( ! sets.has_set_bonus( SHAMAN_ELEMENTAL, T17, B4 ) )
    return;

  if ( stacks < sets.set( SHAMAN_ELEMENTAL, T17, B4 ) -> effectN( 1 ).base_value() )
    return;

  if ( buff.lava_surge -> check() )
    proc.wasted_lava_surge -> occur();

  proc.lava_surge -> occur();
  buff.lava_surge -> trigger();

  cooldown.lava_burst -> reset( false );
}

void shaman_t::trigger_t18_4pc_elemental()
{
  if ( ! sets.has_set_bonus( SHAMAN_ELEMENTAL, T18, B4 ) )
  {
    return;
  }

  if ( ++t18_4pc_elemental_counter < sets.set( SHAMAN_ELEMENTAL, T18, B4 ) -> effectN( 1 ).base_value() )
  {
    return;
  }

  buff.t18_4pc_elemental -> trigger();
  t18_4pc_elemental_counter = 0;
}

void shaman_t::trigger_t19_oh_8pc( const action_state_t* )
{
  if ( ! sets.has_set_bonus( SHAMAN_ELEMENTAL, T19OH, B8 ) &&
       ! sets.has_set_bonus( SHAMAN_ENHANCEMENT, T19OH, B8 ) )
  {
    return;
  }

  buff.t19_oh_8pc -> trigger();
}

void shaman_t::trigger_flametongue_weapon( const action_state_t* state )
{
  assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr && "Flametongue Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );
  if ( ! attack -> may_proc_flametongue )
    return;

  if ( ! buff.flametongue -> up() )
    return;

  flametongue -> target = state -> target;
  flametongue -> schedule_execute();
  attack -> proc_ft -> occur();
}

void shaman_t::trigger_hailstorm( const action_state_t* state )
{
  assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr && "Hailstorm called on invalid action type" );
  shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );
  if ( ! attack -> may_proc_frostbrand )
  {
    return;
  }

  if ( ! talent.hailstorm -> ok() )
  {
    return;
  }

  if ( ! buff.frostbrand -> up() )
  {
    return;
  }

  hailstorm -> target = state -> target;
  hailstorm -> schedule_execute();
  attack -> proc_fb -> occur();
}

// shaman_t::init_buffs =====================================================

void shaman_t::create_buffs()
{
  player_t::create_buffs();

  buff.ascendance              = new ascendance_buff_t( this );
  buff.echo_of_the_elements    = buff_creator_t( this, "echo_of_the_elements", talent.echo_of_the_elements )
                                 .chance( talent.echo_of_the_elements -> ok() );
  buff.lava_surge              = buff_creator_t( this, "lava_surge", find_spell( 77762 ) )
                                 .activated( false )
                                 .chance( 1.0 ); // Proc chance is handled externally
  buff.lightning_shield        = buff_creator_t( this, "lightning_shield", find_talent_spell( "Lightning Shield" ) )
                                 .chance( talent.lightning_shield -> ok() );
  buff.shamanistic_rage        = buff_creator_t( this, "shamanistic_rage", find_specialization_spell( "Shamanistic Rage" ) );
  buff.spirit_walk             = buff_creator_t( this, "spirit_walk", find_specialization_spell( "Spirit Walk" ) );
  buff.spiritwalkers_grace     = buff_creator_t( this, "spiritwalkers_grace", find_specialization_spell( "Spiritwalker's Grace" ) );
  buff.tidal_waves             = buff_creator_t( this, "tidal_waves", spec.tidal_waves -> ok() ? find_spell( 53390 ) : spell_data_t::not_found() );

  // Stat buffs
  buff.elemental_blast_crit    = stat_buff_creator_t( this, "elemental_blast_critical_strike", find_spell( 118522 ) )
                                 .max_stack( 1 );
  buff.elemental_blast_haste   = stat_buff_creator_t( this, "elemental_blast_haste", find_spell( 173183 ) )
                                 .max_stack( 1 );
  buff.elemental_blast_mastery = stat_buff_creator_t( this, "elemental_blast_mastery", find_spell( 173184 ) )
                                 .max_stack( 1 );
  // TODO: How does this refresh?
  buff.t18_4pc_elemental        = haste_buff_creator_t( this, "lightning_vortex", find_spell( 189063 ) )
    .period( find_spell( 189063 ) -> effectN( 2 ).period() )
    .refresh_behavior( BUFF_REFRESH_DURATION )
    .chance( sets.has_set_bonus( SHAMAN_ELEMENTAL, T18, B4 ) )
    .default_value( find_spell( 189063 ) -> effectN( 1 ).percent() )
    .tick_callback( []( buff_t* b, int t, const timespan_t& ) {
      b -> current_value = ( t - b -> current_tick ) * b -> data().effectN( 2 ).percent();
    } );

  buff.focus_of_the_elements = buff_creator_t( this, "focus_of_the_elements", find_spell( 167205 ) )
                               .chance( static_cast< double >( sets.has_set_bonus( SHAMAN_ELEMENTAL, T17, B2 ) ) );
  buff.feral_spirit          = buff_creator_t( this, "t17_4pc_melee", sets.set( SHAMAN_ENHANCEMENT, T17, B4 ) -> effectN( 1 ).trigger() )
                               .cd( timespan_t::zero() );

  buff.flametongue = new flametongue_buff_t( this );
  buff.frostbrand = buff_creator_t( this, "frostbrand", spec.frostbrand )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  buff.stormbringer = buff_creator_t( this, "stormbringer", find_spell( 201846 ) )
                   .activated( false ) // TODO: Need a delay on this
                   .max_stack( find_spell( 201846 ) -> initial_stacks() + talent.tempest -> effectN( 1 ).base_value() );
  buff.crash_lightning = buff_creator_t( this, "crash_lightning", find_spell( 187878 ) );
  buff.windsong = haste_buff_creator_t( this, "windsong", talent.windsong )
                  .add_invalidate( CACHE_ATTACK_SPEED )
                  .default_value( 1.0 / ( 1.0 + talent.windsong -> effectN( 2 ).percent() ) );
  buff.boulderfist = buff_creator_t( this, "boulderfist", talent.boulderfist -> effectN( 3 ).trigger() )
                        .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                        .add_invalidate( CACHE_CRIT_CHANCE );
  buff.landslide = buff_creator_t( this, "landslide", find_spell( 202004 ) )
                   .add_invalidate( CACHE_AGILITY )
                   .chance( talent.landslide -> ok() )
                   .default_value( find_spell( 202004 ) -> effectN( 1 ).percent() );
  buff.doom_winds = buff_creator_t( this, "doom_winds", &( artifact.doom_winds.data() ) )
                    .cd( timespan_t::zero() ); // handled by the action
  buff.unleash_doom = buff_creator_t( this, "unleash_doom", artifact.unleash_doom.data().effectN( 1 ).trigger() )
                      .trigger_spell( artifact.unleash_doom );
  buff.wind_strikes = haste_buff_creator_t( this, "wind_strikes", find_spell( 198293 ) )
                      .add_invalidate( CACHE_ATTACK_SPEED )
                      .chance( artifact.wind_strikes.rank() > 0 )
                      .default_value( 1.0 / ( 1.0 + artifact.wind_strikes.percent() ) );
  buff.gathering_storms = buff_creator_t( this, "gathering_storms", find_spell( 198300 ) );
  buff.ghost_wolf = buff_creator_t( this, "ghost_wolf", find_class_spell( "Ghost Wolf" ) )
                    .period( artifact.spirit_of_the_maelstrom.rank() ? find_spell( 198240 ) -> effectN( 1 ).period() : timespan_t::min() )
                    .tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
                        this -> resource_gain( RESOURCE_MAELSTROM, this -> artifact.spirit_of_the_maelstrom.value(), this -> gain.spirit_of_the_maelstrom, nullptr );
                    })
                    .stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                      if ( new_ == 1 ) buff.spiritual_journey -> trigger();
                      else             buff.spiritual_journey -> expire();
                    } );
  buff.elemental_focus = buff_creator_t( this, "elemental_focus", spec.elemental_focus -> effectN( 1 ).trigger() )
    .default_value( 1.0 + spec.elemental_focus -> effectN( 1 ).trigger() -> effectN( 1 ).percent() +
                          sets.set( SHAMAN_ELEMENTAL, T19, B4 ) -> effectN( 1 ).percent() )
    .activated( false );
  buff.earth_surge = buff_creator_t( this, "earth_surge", find_spell( 189797 ) )
    .default_value( 1.0 + find_spell( 189797 ) -> effectN( 1 ).percent() );
  buff.stormkeeper = buff_creator_t( this, "stormkeeper", artifact.stormkeeper )
    .cd( timespan_t::zero() ); // Handled by the action
  buff.static_overload = buff_creator_t( this, "static_overload", find_spell( 191634 ) )
    .chance( artifact.static_overload.data().effectN( 1 ).percent() );
  buff.power_of_the_maelstrom = buff_creator_t( this, "power_of_the_maelstrom", artifact.power_of_the_maelstrom.data().effectN( 1 ).trigger() )
    .trigger_spell( artifact.power_of_the_maelstrom );

  buff.resonance_totem = buff_creator_t( this, "resonance_totem", find_spell( 202192 ) )
                         .refresh_behavior( BUFF_REFRESH_DURATION )
                         .duration( talent.totem_mastery -> effectN( 1 ).trigger() -> duration() )
                         .period( find_spell( 202192 ) -> effectN( 1 ).period() )
                         .tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
                          this -> resource_gain( RESOURCE_MAELSTROM, b -> data().effectN( 1 ).resource( RESOURCE_MAELSTROM ),
                              this -> gain.resonance_totem, nullptr ); } );
  buff.storm_totem = buff_creator_t( this, "storm_totem", find_spell( 210651 ) )
                     .duration( talent.totem_mastery -> effectN( 2 ).trigger() -> duration() )
                     .cd( timespan_t::zero() ) // Handled by the action
                     .default_value( find_spell( 210651 ) -> effectN( 2 ).percent() );
  buff.ember_totem = buff_creator_t( this, "ember_totem", find_spell( 210658 ) )
                     .duration( talent.totem_mastery -> effectN( 3 ).trigger() -> duration() )
                     .default_value( 1.0 + find_spell( 210658 ) -> effectN( 1 ).percent() );
  buff.tailwind_totem = haste_buff_creator_t( this, "tailwind_totem", find_spell( 210659 ) )
                        .add_invalidate( CACHE_HASTE )
                        .duration( talent.totem_mastery -> effectN( 4 ).trigger() -> duration() )
                        .default_value( 1.0 / ( 1.0 + find_spell( 210659 ) -> effectN( 1 ).percent() ) );
  buff.icefury = buff_creator_t( this, "icefury", talent.icefury )
    .cd( timespan_t::zero() ) // Handled by the action
    .default_value( talent.icefury -> effectN( 3 ).percent() );

  buff.stormlash = new stormlash_buff_t( buff_creator_t( this, "stormlash", find_spell( 195222 ) )
            .activated( false )
            .cd( timespan_t::zero() ) );

  buff.hot_hand = buff_creator_t( this, "hot_hand", talent.hot_hand -> effectN( 1 ).trigger() )
                  .trigger_spell( talent.hot_hand );

  buff.t19_oh_8pc = stat_buff_creator_t( this, "might_of_the_maelstrom" )
    .spell( sets.set( specialization(), T19OH, B8 ) -> effectN( 1 ).trigger() )
    .trigger_spell( sets.set( specialization(), T19OH, B8 ) );
  buff.elemental_mastery = haste_buff_creator_t( this, "elemental_mastery", talent.elemental_mastery )
                           .default_value( 1.0 / ( 1.0 + talent.elemental_mastery -> effectN( 1 ).percent() ) )
                           .cd( timespan_t::zero() ); // Handled by the action
  buff.t18_4pc_enhancement = buff_creator_t( this, "natures_reprisal" )
    .spell( sets.set( SHAMAN_ENHANCEMENT, T18, B4 ) -> effectN( 1 ).trigger() )
    .trigger_spell( sets.set( SHAMAN_ENHANCEMENT, T18, B4 ) )
    .default_value( sets.set( SHAMAN_ENHANCEMENT, T18, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gain.aftershock           = get_gain( "Aftershock"        );
  gain.ascendance           = get_gain( "Ascendance"        );
  gain.resurgence           = get_gain( "resurgence"        );
  gain.feral_spirit         = get_gain( "Feral Spirit"      );
  gain.spirit_of_the_maelstrom = get_gain( "Spirit of the Maelstrom" );
  gain.resonance_totem      = get_gain( "Resonance Totem"   );
  gain.wind_gust            = get_gain( "Wind Gust"         );
  gain.t18_2pc_elemental    = get_gain( "Elemental T18 2PC" );
  gain.the_deceivers_blood_pact = get_gain( "The Deceiver's Blood Pact" );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.lava_surge         = get_proc( "Lava Surge"              );
  proc.wasted_lava_surge  = get_proc( "Lava Surge: Wasted"      );
  proc.windfury           = get_proc( "Windfury"                );
  proc.surge_during_lvb   = get_proc( "Lava Surge: During Lava Burst" );
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  player_t::init_rng();

  real_ppm.stormlash   = get_rppm( "stormlash", spec.stormlash );
}

// shaman_t::init_special_effects ===========================================

bool shaman_t::init_special_effects()
{
  bool ret = player_t::init_special_effects();

  if ( spec.stormlash -> ok() )
  {
    // shaman_t::create_buffs has been called before init_special_effects
    stormlash_buff_t* stormlash_buff = static_cast<stormlash_buff_t*>( buff_t::find( this, "stormlash" ) );

    special_effect_t* effect = new special_effect_t( this );
    effect -> type = SPECIAL_EFFECT_EQUIP;
    effect -> proc_flags_ = PF_ALL_DAMAGE;
    effect -> proc_flags2_ = PF2_ALL_HIT;
    effect -> spell_id = 195222;
    special_effects.push_back( effect );

    stormlash_buff -> callback = new stormlash_callback_t( this, *effect );
    stormlash_buff -> callback -> initialize();
  }

  return ret;
}

// shaman_t::generate_bloodlust_options =====================================

std::string shaman_t::generate_bloodlust_options()
{
  std::string bloodlust_options = "if=";

  if ( sim -> bloodlust_percent > 0 )
    bloodlust_options += "target.health.pct<" + util::to_string( sim -> bloodlust_percent ) + "|";

  if ( sim -> bloodlust_time < timespan_t::zero() )
    bloodlust_options += "target.time_to_die<" + util::to_string( - sim -> bloodlust_time.total_seconds() ) + "|";

  if ( sim -> bloodlust_time > timespan_t::zero() )
    bloodlust_options += "time>" + util::to_string( sim -> bloodlust_time.total_seconds() ) + "|";
  bloodlust_options.erase( bloodlust_options.end() - 1 );

  return bloodlust_options;
}

// shaman_t::init_action_list_elemental =====================================

void shaman_t::init_action_list_elemental()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default"   );
  action_priority_list_t* single    = get_action_priority_list( "single", "Single target action priority list" );
  action_priority_list_t* aoe       = get_action_priority_list( "aoe", "Multi target action priority list" );

  std::string potion_name = ( true_level > 100 ) ? "deadly_grace" :
                            ( true_level >= 90 ) ? "draenic_intellect" :
                            ( true_level >= 85 ) ? "jade_serpent" :
                            ( true_level >= 80 ) ? "volcanic" :
                            "";
  std::string flask_name  = ( true_level > 100 ) ? "whispered_pact" :
                            ( true_level >= 90 ) ? "greater_draenic_intellect_flask" :
                            ( true_level >= 85 ) ? "warm_sun" :
                            ( true_level >= 80 ) ? "draconic_mind" :
                            "";
  std::string food_name   = ( true_level > 100  ) ? "fishbrul_special" :
                            ( true_level > 90   ) ? "pickled_eel" :
                            ( true_level >= 90  ) ? "mogu_fish_stew" :
                            ( true_level >= 80  ) ? "seafood_magnifique_feast" :
                            "";

  // Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    precombat -> add_action( "flask,type=" + flask_name );
  }

  // Food
  if ( sim -> allow_food && true_level >= 80 )
  {
    precombat -> add_action( "food,name=" + food_name );
  }


  if ( true_level > 100 )
    precombat -> add_action( "augmentation,type=defiled" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( sim -> allow_potions && true_level >= 80 )
  {
    precombat -> add_action( "potion,name=" + potion_name );
  }

  precombat -> add_action( this, "Stormkeeper" );
  precombat -> add_talent( this, "Totem Mastery" );

  // All Shamans Bloodlust by default
  def -> add_action( this, "Bloodlust", generate_bloodlust_options(),
    "Bloodlust casting behavior mirrors the simulator settings for proxy bloodlust. See options 'bloodlust_percent', and 'bloodlust_time'. " );

  // On-use items
  for ( const auto& item : items )
  {
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action( "use_item,slot=" + std::string( item.slot_name() ) );
    }
  }

  // In-combat potion
  if ( sim -> allow_potions && true_level >= 80  )
  {
    def -> add_action( "potion,name=" + potion_name + ",if=buff.elemental_mastery.up|target.time_to_die<=30|buff.bloodlust.up",
        "In-combat potion is preferentially linked to Elemental Mastery or Bloodlust, unless combat will end shortly" );
  }

  def -> add_talent( this, "Totem Mastery", "if=buff.resonance_totem.remains<2" );
  def -> add_action( this, "Fire Elemental" );
  def -> add_talent( this, "Storm Elemental" );
  def -> add_talent( this, "Elemental Mastery" );
  def -> add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  def -> add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
  def -> add_action( "run_action_list,name=aoe,if=active_enemies>2&(spell_targets.chain_lightning>2|spell_targets.lava_beam>2)" );
  def -> add_action( "run_action_list,name=single" );

  // Single target APL

  // Racials
  single -> add_talent( this, "Ascendance", "if=dot.flame_shock.remains>buff.ascendance.duration&"
                                            "(time>=60|buff.bloodlust.up)&cooldown.lava_burst.remains>0&"
                                            "!buff.stormkeeper.up" );
  single -> add_action( this, "Flame Shock", "if=!ticking" );
  single -> add_action( this, "Flame Shock", "if=maelstrom>=20&remains<=buff.ascendance.duration&"
                                             "cooldown.ascendance.remains+buff.ascendance.duration<=duration" );
  single -> add_action( this, "Earthquake", "if=buff.echoes_of_the_great_sundering.up" );
  single -> add_action( this, "Earth Shock", "if=maelstrom>=92" );
  single -> add_talent( this, "Icefury", "if=raid_event.movement.in<5" );
  single -> add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time&"
                                            "(cooldown_react|buff.ascendance.up)" );
  single -> add_talent( this, "Elemental Blast" );
  single -> add_action( this, "Flame Shock", "if=maelstrom>=20&buff.elemental_focus.up,target_if=refreshable" );
  single -> add_action( this, "Frost Shock", "if=talent.icefury.enabled&buff.icefury.up&"
                                             "((maelstrom>=20&raid_event.movement.in>buff.icefury.remains)|"
                                             "buff.icefury.remains<(1.5*spell_haste*buff.icefury.stack))" );
  single -> add_action( this, "Frost Shock", "moving=1,if=buff.icefury.up" );
  single -> add_action( this, "Earth Shock", "if=maelstrom>=86" );
  single -> add_talent( this, "Icefury", "if=maelstrom<=70&raid_event.movement.in>30&"
                                         "((talent.ascendance.enabled&cooldown.ascendance.remains>buff.icefury.duration)|"
                                         "!talent.ascendance.enabled)" );
  single -> add_talent( this, "Liquid Magma Totem", "if=raid_event.adds.count<3|raid_event.adds.in>50" );
  single -> add_action( this, "Stormkeeper", "if=(talent.ascendance.enabled&cooldown.ascendance.remains>10)|"
                                             "!talent.ascendance.enabled" );
  single -> add_talent( this, "Totem Mastery", "if=buff.resonance_totem.remains<10|"
                                               "(buff.resonance_totem.remains<(buff.ascendance.duration+cooldown.ascendance.remains)&"
                                               "cooldown.ascendance.remains<15)" );
  single -> add_action( this, "Lava Beam", "if=active_enemies>1&spell_targets.lava_beam>1" );
  single -> add_action( this, "Lightning Bolt", "if=buff.power_of_the_maelstrom.up,target_if=debuff.lightning_rod.down" );
  single -> add_action( this, "Lightning Bolt", "if=buff.power_of_the_maelstrom.up" );
  single -> add_action( this, "Chain Lightning", "if=active_enemies>1&spell_targets.chain_lightning>1,target_if=debuff.lightning_rod.down" );
  single -> add_action( this, "Chain Lightning", "if=active_enemies>1&spell_targets.chain_lightning>1" );
  single -> add_action( this, "Lightning Bolt", "target_if=debuff.lightning_rod.down" );
  single -> add_action( this, "Lightning Bolt" );
  single -> add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
  single -> add_action( this, "Earth Shock", "moving=1" );

  // Aoe APL
  aoe -> add_action( this, "Stormkeeper" );
  aoe -> add_talent( this, "Ascendance" );
  aoe -> add_talent( this, "Liquid Magma Totem" );
  aoe -> add_action( this, "Flame Shock", "if=spell_targets.chain_lightning=3&maelstrom>=20,target_if=refreshable" );
  aoe -> add_action( this, "Earthquake" );
  aoe -> add_action( this, "Lava Burst", "if=buff.lava_surge.up&spell_targets.chain_lightning=3" );
  aoe -> add_action( this, "Lava Beam" );
  aoe -> add_action( this, "Chain Lightning", "target_if=debuff.lightning_rod.down" );
  aoe -> add_action( this, "Chain Lightning" );
  aoe -> add_action( this, "Lava Burst", "moving=1" );
  aoe -> add_action( this, "Flame Shock", "moving=1,target_if=refreshable" );
}

// shaman_t::init_action_list_enhancement ===================================

void shaman_t::init_action_list_enhancement()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default"   );

  std::string flask_name = ( true_level >  100 ) ? "seventh_demon" :
                           ( true_level >= 90  ) ? "greater_draenic_agility_flask" :
                           ( true_level >= 85  ) ? "spring_blossoms" :
                           ( true_level >= 80  ) ? "winds" :
                           "";
  std::string food_name = ( true_level >  100 ) ? "nightborne_delicacy_platter" :
                          ( true_level >  90  ) ? "buttered_sturgeon" :
                          ( true_level >= 90  ) ? "sea_mist_rice_noodles" :
                          ( true_level >= 80  ) ? "seafood_magnifique_feast" :
                          "";
  std::string potion_name = ( true_level > 100 ) ? "old_war" :
                            ( true_level >= 90 ) ? "draenic_agility" :
                            ( true_level >= 85 ) ? "virmens_bite" :
                            ( true_level >= 80 ) ? "tolvir" :
                            "";

  // Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    precombat -> add_action( "flask,type=" + flask_name );

    // Added Rune if Flask are allowed since there is no "allow_runes" bool.
    if (true_level >= 100)
    {
        std::string rune_action = "augmentation,type=";
        rune_action += ((true_level >= 110) ? "defiled" : (true_level >= 100) ? "hyper" : "");

        precombat->add_action(rune_action);
    }
  }

  // Food
  if ( sim -> allow_food && true_level >= 80 )
  {
    precombat -> add_action( "food,name=" + food_name );
  }

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Precombat potion
  if ( sim -> allow_potions && true_level >= 80 )
  {
    precombat -> add_action( "potion,name=" + potion_name );
  }

  // Lightning shield can be turned on pre-combat
  precombat -> add_talent( this, "Lightning Shield" );

  // All Shamans Bloodlust and Wind Shear by default
  def -> add_action( this, "Wind Shear" );

  def -> add_action( this, "Bloodlust", generate_bloodlust_options(),
    "Bloodlust casting behavior mirrors the simulator settings for proxy bloodlust. See options 'bloodlust_percent', and 'bloodlust_time'. " );

  // Turn on auto-attack first thing
  def -> add_action( "auto_attack" );

  // Use Feral Spirits before off-GCD CDs.
  def -> add_action( this, "Feral Spirit" );
  // Ensure Feral Spirits start using alpha wolf abilities immediately
  def -> add_action( this, "Crash Lightning", "if=artifact.alpha_wolf.rank&prev_gcd.feral_spirit" );

  // On-use items
  for ( const auto& item : items )
  {
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action( "use_item,slot=" + std::string( item.slot_name() ) );
    }
  }

  // In-combat potion
  if ( sim -> allow_potions && true_level >= 80  )
  {
    def -> add_action( "potion,name=" + potion_name + ",if=feral_spirit.remains>5|target.time_to_die<=30" );
  }

  // Racials
  def -> add_action( "berserking,if=buff.ascendance.up|!talent.ascendance.enabled|level<100" );
  def -> add_action( "blood_fury" );

  // Core rotation
  def -> add_action( this, "Crash Lightning", "if=talent.crashing_storm.enabled&active_enemies>=3" );
  def -> add_talent( this, "Boulderfist", "if=buff.boulderfist.remains<gcd&maelstrom>=50&active_enemies>=3" );
  def -> add_talent( this, "Boulderfist", "if=buff.boulderfist.remains<gcd|(charges_fractional>1.75&maelstrom<=100&active_enemies<=2)" );
  def -> add_action( this, "Crash Lightning", "if=buff.crash_lightning.remains<gcd&active_enemies>=2" );
  def -> add_action( this, "Windstrike", "if=active_enemies>=3&!talent.hailstorm.enabled" );
  def -> add_action( this, "Stormstrike", "if=active_enemies>=3&!talent.hailstorm.enabled" );
  def -> add_action( this, "Windstrike", "if=buff.stormbringer.react" );
  def -> add_action( this, "Stormstrike", "if=buff.stormbringer.react" );
  def -> add_action( this, "Frostbrand", "if=talent.hailstorm.enabled&buff.frostbrand.remains<gcd" );
  def -> add_action( this, "Flametongue", "if=buff.flametongue.remains<gcd");
  def -> add_talent( this, "Windsong");
  def -> add_talent( this, "Ascendance" );
  def -> add_talent( this, "Fury of Air", "if=!ticking" );
  def -> add_action( this, "Doom Winds");
  def -> add_action( this, "Crash Lightning", "if=active_enemies>=3" );
  def -> add_action( this, "Windstrike" );
  def -> add_action( this, "Stormstrike" );
  def -> add_action( this, "Lightning Bolt", "if=talent.overcharge.enabled&maelstrom>=60" );
  def -> add_action( this, "Lava Lash", "if=buff.hot_hand.react" );
  def -> add_talent( this, "Earthen Spike" );
  def -> add_action( this, "Crash Lightning", "if=active_enemies>1|talent.crashing_storm.enabled|feral_spirit.remains>5" );
  def -> add_action( this, "Frostbrand", "if=talent.hailstorm.enabled&buff.frostbrand.remains<4.8" );
  def -> add_action( this, "Flametongue", "if=buff.flametongue.remains<4.8");
  def -> add_talent( this, "Sundering" );
  def -> add_action( this, "Lava Lash", "if=maelstrom>=90" );
  def -> add_action( this, "Rockbiter" );
  def -> add_action( this, "Flametongue" );
  def -> add_talent( this, "Boulderfist" );
}

// shaman_t::init_actions ===================================================

void shaman_t::init_action_list()
{
  if ( ! ( primary_role() == ROLE_ATTACK && specialization() == SHAMAN_ENHANCEMENT ) &&
       ! ( primary_role() == ROLE_SPELL  && specialization() == SHAMAN_ELEMENTAL   ) )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's role (%s) or spec(%s) isn't supported yet.",
                     name(), util::role_type_string( primary_role() ), dbc::specialization_string( specialization() ).c_str() );
    quiet = true;
    return;
  }

#ifdef NDEBUG // Only restrict on release builds.
  // Restoration isn't supported atm
  if ( specialization() == SHAMAN_RESTORATION && primary_role() == ROLE_HEAL )
  {
    if ( ! quiet )
      sim -> errorf( "Restoration Shaman healing for player %s is not currently supported.", name() );

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
  }

  if ( talent.hailstorm -> ok() )
  {
    hailstorm = new hailstorm_attack_t( "hailstorm", this, &( main_hand_weapon ) );
  }

  if ( ! action_list_str.empty() )
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
    if ( swg && executing && swg -> ready() )
    {
      // Shaman executes SWG mid-cast during a movement event, if
      // 1) The profile does not have Glyph of Unleashed Lightning and is
      //    casting a Lightning Bolt (non-instant cast)
      // 2) The profile is casting Lava Burst (without Lava Surge)
      // 3) The profile is casting Chain Lightning
      // 4) The profile is casting Elemental Blast
      if ( ( executing -> id == 51505 ) ||
           ( executing -> id == 421 ) ||
           ( executing -> id == 117014 ) )
      {
        if ( sim -> log )
          sim -> out_log.printf( "%s spiritwalkers_grace during spell cast, next cast (%s) should finish",
                         name(), executing -> name() );
        swg -> execute();
      }
    }
    else
    {
      interrupt();
    }

    if ( main_hand_attack ) main_hand_attack -> cancel();
    if (  off_hand_attack )  off_hand_attack -> cancel();
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

  if ( talent.ancestral_swiftness -> ok() )
    h *= constant.haste_ancestral_swiftness;

  h *= 1.0 / ( 1.0 + buff.t18_4pc_elemental -> stack_value() );

  if ( buff.tailwind_totem -> up() )
  {
    h *= buff.tailwind_totem -> check_value();
  }

  if ( buff.elemental_mastery -> up() )
  {
    h *= buff.elemental_mastery -> check_value();
  }

  return h;
}

// shaman_t::composite_spell_crit_chance ===========================================

double shaman_t::composite_spell_crit_chance() const
{
  double m = player_t::composite_spell_crit_chance();

  if ( buff.boulderfist -> up() )
  {
    m += buff.boulderfist -> data().effectN( 1 ).percent();
  }

  m += spec.critical_strikes -> effectN( 1 ).percent();

  return m;
}

// shaman_t::temporary_movement_modifier =======================================

double shaman_t::temporary_movement_modifier() const
{
  double ms = player_t::temporary_movement_modifier();

  if ( buff.spirit_walk -> up() )
    ms = std::max( buff.spirit_walk -> data().effectN( 1 ).percent(), ms );

  if ( buff.feral_spirit -> up() )
    ms = std::max( buff.feral_spirit -> data().effectN( 1 ).percent(), ms );

  if ( buff.ghost_wolf -> up() )
  {
    ms *= 1.0 + buff.ghost_wolf -> data().effectN( 2 ).percent();
  }

  return ms;
}

// shaman_t::composite_melee_crit_chance ===========================================

double shaman_t::composite_melee_crit_chance() const
{
  double m = player_t::composite_melee_crit_chance();

  if ( buff.boulderfist -> up() )
  {
    m += buff.boulderfist -> data().effectN( 1 ).percent();
  }

  m += spec.critical_strikes -> effectN( 1 ).percent();

  return m;
}

// shaman_t::composite_attack_haste =========================================

double shaman_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( talent.ancestral_swiftness -> ok() )
    h *= constant.haste_ancestral_swiftness;

  if ( buff.tailwind_totem -> up() )
  {
    h *= buff.tailwind_totem -> check_value();
  }

  if ( buff.elemental_mastery -> up() )
  {
    h *= buff.elemental_mastery -> check_value();
  }

  return h;
}

// shaman_t::composite_attack_speed =========================================

double shaman_t::composite_melee_speed() const
{
  double speed = player_t::composite_melee_speed();

  if ( buff.windsong -> up() )
  {
    speed *= buff.windsong -> check_value();
  }

  if ( buff.wind_strikes -> up() )
  {
    speed *= buff.wind_strikes -> check_value();
  }

  return speed;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( school_e school ) const
{
  double sp = 0;

  if ( specialization() == SHAMAN_ENHANCEMENT )
    sp = composite_attack_power_multiplier() * cache.attack_power() * spec.enhancement_shaman -> effectN( 1 ).percent();
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
      m *= 1.0 + buff.landslide -> stack_value();
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

  m *= 1.0 + artifact.stormkeepers_power.percent();
  m *= 1.0 + artifact.earthshattering_blows.percent();

  if ( mastery.enhanced_elements -> ok() &&
       ( dbc::is_school( school, SCHOOL_FIRE   ) ||
         dbc::is_school( school, SCHOOL_FROST  ) ||
         dbc::is_school( school, SCHOOL_NATURE ) ) )
  {
    m *= 1.0 + cache.mastery_value();
  }

  if ( buff.boulderfist -> up() )
  {
    m *= 1.0 + buff.boulderfist -> data().effectN( 2 ).percent();
  }

  if ( artifact.call_the_thunder.rank() > 0 && dbc::is_school( school, SCHOOL_NATURE ) )
  {
    m *= 1.0 + artifact.call_the_thunder.percent();
  }

  if ( buff.t18_4pc_enhancement -> up() && dbc::is_school( school, SCHOOL_NATURE ) )
  {
    m *= 1.0 + buff.t18_4pc_enhancement -> check_value();
  }

  if ( buff.emalons_charged_core -> up() )
  {
    m *= 1.0 + buff.emalons_charged_core -> data().effectN( 1 ).percent();
  }

  m *= 1.0 + buff.eotn_fire -> stack_value();
  m *= 1.0 + buff.eotn_shock -> stack_value();
  m *= 1.0 + buff.eotn_chill -> stack_value();

  return m;
}

// shaman_t::composite_player_target_multiplier ==============================

double shaman_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  shaman_td_t* td = get_target_data( target );

  if ( td -> debuff.earthen_spike -> up() &&
    ( dbc::is_school( school, SCHOOL_PHYSICAL ) || dbc::is_school( school, SCHOOL_NATURE ) ) )
  {
    m *= td -> debuff.earthen_spike -> check_value();
  }

  return m;
}

// shaman_t::composite_player_pet_damage_multiplier =========================

double shaman_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  auto school = s -> action -> get_school();
  if ( ( dbc::is_school( school, SCHOOL_FIRE ) || dbc::is_school( school, SCHOOL_FROST ) ||
         dbc::is_school( school, SCHOOL_NATURE ) ) &&
        mastery.enhanced_elements -> ok() )
  {
    m *= 1.0 + cache.mastery_value();
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
    case CACHE_MASTERY:
      if ( mastery.enhanced_elements -> ok() )
      {
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      }
      break;
    default: break;
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

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  player_t::reset();

  lava_surge_during_lvb = false;
  t18_4pc_elemental_counter = 0;
  for (auto & elem : counters)
    elem -> reset();

  assert( lightning_rods.size() == 0 );
}

// shaman_t::merge ==========================================================

void shaman_t::merge( player_t& other )
{
  player_t::merge( other );

  const shaman_t& s = static_cast<shaman_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ] -> merge( *s.counters[ i ] );

  for ( size_t i = 0, end = cd_waste_exec.size(); i < end; i++ )
  {
    cd_waste_exec[ i ] -> second.merge( s.cd_waste_exec[ i ] -> second );
    cd_waste_cumulative[ i ] -> second.merge( s.cd_waste_cumulative[ i ] -> second );
  }
}

// shaman_t::datacollection_begin ===========================================

void shaman_t::datacollection_begin()
{
  if ( active_during_iteration )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_iter[ i ] -> second.reset();
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
      cd_waste_cumulative[ i ] -> second.add( cd_waste_iter[ i ] -> second.sum() );
    }
  }

  player_t::datacollection_end();
}

// shaman_t::has_t18_class_trinket ==========================================

bool shaman_t::has_t18_class_trinket() const
{
  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
    case SHAMAN_ELEMENTAL:
      return items[ SLOT_TRINKET_1 ].parsed.data.id == 124521 ||
             items[ SLOT_TRINKET_2 ].parsed.data.id == 124521;
    default:                 return false;
  }
}

// shaman_t::primary_role ===================================================

role_e shaman_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HYBRID;//To prevent spawning healing_target, as there is no support for healing.

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
  // This is a guess at how AGI/STR gear will work for Resto/Elemental, TODO: confirm
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Enhance, TODO: confirm
  // this should probably never come up since shamans can't equip plate, but....
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
    if ( specialization() == SHAMAN_RESTORATION )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return STAT_NONE;
  default: return s;
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class shaman_report_t : public player_report_extension_t
{
public:
  shaman_report_t( shaman_t& player ) :
      p( player )
  { }
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

        os.format("<tr%s><td class=\"left\">%s</td><td class=\"right\">%.2f</td><td class=\"right\">%.2f (%.2f%%)</td></tr>\n",
            row_class_str.c_str(),
            name_str.c_str(),
            util::round( n_generated, 2 ),
            util::round( n_wasted, 2 ), util::round( 100.0 * n_wasted / n_generated, 2 ) );
      }
    }

    os.format("<tr><td class=\"left\">Total</td><td class=\"right\">%.2f</td><td class=\"right\">%.2f (%.2f%%)</td></tr>\n",
        total_generated, total_wasted, 100.0 * total_wasted / total_generated );
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
            if ( s -> maelstrom_weapon_cast[ k ] -> mean() > 0 || s -> maelstrom_weapon_executed[ k ] -> mean() > 0 )
              has_data = true;

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
            os.format("<td class=\"right\">%.1f (%.1f%%)</td>", util::round( n_cast[ j ], 1 ), util::round( pct, 1 ) );
          else
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
            os.format("<td class=\"right\">%.1f (%.1f%%)</td>", util::round( n_executed[ j ], 1 ), util::round( pct, 1 ) );
          else
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
      if ( entry -> second.count() == 0 )
      {
        continue;
      }

      const data_t* iter_entry = p.cd_waste_cumulative[ i ];

      action_t* a = p.find_action( entry -> first );
      std::string name_str = entry -> first;
      if ( a )
      {
        name_str = report::decorated_action_name( a, a -> stats -> name_str );
      }

      std::string row_class_str = "";
      if ( ++n & 1 )
        row_class_str = " class=\"odd\"";

      os.format( "<tr%s>", row_class_str.c_str() );
      os << "<td class=\"left\">" << name_str << "</td>";
      os.format("<td class=\"right\">%.3f</td>", entry -> second.mean() );
      os.format("<td class=\"right\">%.3f</td>", entry -> second.min() );
      os.format("<td class=\"right\">%.3f</td>", entry -> second.max() );
      os.format("<td class=\"right\">%.3f</td>", iter_entry -> second.mean() );
      os.format("<td class=\"right\">%.3f</td>", iter_entry -> second.min() );
      os.format("<td class=\"right\">%.3f</td>", iter_entry -> second.max() );
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

struct elemental_bellows_t : public scoped_action_callback_t<flame_shock_t>
{
  elemental_bellows_t() : super( SHAMAN_ELEMENTAL, "flame_shock" ) { }

  void manipulate( flame_shock_t* action, const special_effect_t& effect ) override
  {
    action -> base_multiplier *= 1.0 + effect.driver() -> effectN( 1 ).average( effect.item ) / 100.0;
    action -> duration_multiplier = 1.0 + effect.driver() -> effectN( 3 ).average( effect.item ) / 100.0;
  }
};

struct furious_winds_t : public scoped_action_callback_t<windfury_attack_t>
{
  furious_winds_t( const std::string& name_str ) : super( SHAMAN, name_str ) { }

  void manipulate( windfury_attack_t* action, const special_effect_t& effect ) override
  {
    action -> base_multiplier *= 1.0 + effect.driver() -> effectN( 1 ).average( effect.item ) / 100.0;
    action -> furious_winds_chance = effect.driver() -> effectN( 2 ).average( effect.item ) / 100.0;
  }
};

struct echoes_of_the_great_sundering_t : public scoped_action_callback_t<earth_shock_t>
{
  echoes_of_the_great_sundering_t() : super( SHAMAN, "earth_shock" )
  { }

  void manipulate( earth_shock_t* action, const special_effect_t& e ) override
  { action -> eotgs_base_chance = e.driver() -> effectN( 1 ).percent() / action -> base_cost(); }
};

struct echoes_of_the_great_sundering_buff_t : public class_buff_cb_t<buff_t>
{
  echoes_of_the_great_sundering_buff_t() : super( SHAMAN, "echoes_of_the_great_sundering" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<shaman_t*>( e.player ) -> buff.echoes_of_the_great_sundering; }

  buff_creator_t creator( const special_effect_t& e ) const override
  { return super::creator( e ).spell( e.player -> find_spell( 208723 ) ); }
};

struct emalons_charged_core_t : public scoped_action_callback_t<crash_lightning_t>
{
  emalons_charged_core_t() : super( SHAMAN, "crash_lightning" )
  { }

  void manipulate( crash_lightning_t* action, const special_effect_t& e ) override
  { action -> ecc_min_targets = e.driver() -> effectN( 1 ).base_value(); }
};

struct emalons_charged_core_buff_t : public class_buff_cb_t<buff_t>
{
  emalons_charged_core_buff_t() : super( SHAMAN, "emalons_charged_core" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<shaman_t*>( e.player ) -> buff.emalons_charged_core; }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.player -> find_spell( 208742 ) )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct pristine_protoscale_girdle_t : public scoped_action_callback_t<lava_burst_t>
{
  pristine_protoscale_girdle_t() : super( SHAMAN, "lava_burst" )
  { }

  void manipulate( lava_burst_t* action, const special_effect_t& ) override
  {
    if ( ! action -> p() -> action.ppsg )
    {
      action -> p() -> action.ppsg = new pristine_protoscale_girdle_dot_t( action -> p() );
    }
  }
};

struct storm_tempests_t : public scoped_action_callback_t<stormstrike_base_t>
{
  storm_tempests_t( const std::string& strike_str ) : super( SHAMAN, strike_str )
  { }

  void manipulate( stormstrike_base_t* action, const special_effect_t& ) override
  {
    if ( action -> sim -> target_list.size() > 1 && ! action -> p() -> action.storm_tempests )
    {
      action -> p() -> action.storm_tempests = new storm_tempests_zap_t( action -> p() );
    }
  }
};

struct the_deceivers_blood_pact_t: public scoped_action_callback_t<earth_shock_t>
{
  the_deceivers_blood_pact_t() : super( SHAMAN, "earth_shock" )
  { }

  void manipulate( earth_shock_t* action, const special_effect_t& e ) override
  { action -> tdbp_proc_chance = e.driver() -> proc_chance(); }
};

struct spiritual_journey_t : public class_buff_cb_t<buff_t>
{
  spiritual_journey_t() : super( SHAMAN, "spiritual_journey" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<shaman_t*>( e.player ) -> buff.spiritual_journey; }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    cooldown_t* feral_spirit = e.player -> get_cooldown( "feral_spirit" );

    return super::creator( e )
      .spell( e.player -> find_spell( 214170 ) )
      .stack_change_callback( [ feral_spirit ]( buff_t*, int, int ) {
        feral_spirit -> adjust_recharge_multiplier();
      } );
  }
};

struct akainus_absolute_justice_t : public scoped_action_callback_t<lava_lash_t>
{
  akainus_absolute_justice_t() : super( SHAMAN, "lava_lash" )
  { }

  void manipulate( lava_lash_t* action, const special_effect_t& e ) override
  { action -> aaj_multiplier = e.driver() -> effectN( 1 ).percent(); }
};

template <typename T>
struct alakirs_acrimony_t : public scoped_action_callback_t<T>
{
  alakirs_acrimony_t( const std::string& action_str ) :
    scoped_action_callback_t<T>( SHAMAN, action_str )
  { }

  void manipulate( T* action, const special_effect_t& e ) override
  { action -> chain_multiplier *= 1.0 + e.driver() -> effectN( 1 ).percent(); }
};

struct eotn_buff_base_t : public class_buff_cb_t<buff_t>
{
  unsigned sid;

  eotn_buff_base_t( const std::string& name_str, unsigned spell_id ) : super( SHAMAN, name_str ),
    sid( spell_id )
  { }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
           .spell( e.player -> find_spell( sid ) )
           .default_value( e.player -> find_spell( sid ) -> effectN( 1 ).percent() )
           .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct eotn_buff_fire_t : public eotn_buff_base_t
{
  eotn_buff_fire_t() : eotn_buff_base_t( "fire_of_the_twisting_nether", 207995 )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<shaman_t*>( e.player ) -> buff.eotn_fire; }
};

struct eotn_buff_shock_t : public eotn_buff_base_t
{
  eotn_buff_shock_t() : eotn_buff_base_t( "shock_of_the_twisting_nether", 207999 )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<shaman_t*>( e.player ) -> buff.eotn_shock; }
};

struct eotn_buff_chill_t : public eotn_buff_base_t
{
  eotn_buff_chill_t() : eotn_buff_base_t( "chill_of_the_twisting_nether", 207998 )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<shaman_t*>( e.player ) -> buff.eotn_chill; }
};

struct uncertain_reminder_t : public scoped_actor_callback_t<shaman_t>
{
  uncertain_reminder_t() : scoped_actor_callback_t( SHAMAN )
  { }

  void manipulate( shaman_t* shaman, const special_effect_t& e ) override
  {
    auto buff = buff_t::find( shaman, "bloodlust" );
    if ( buff )
    {
      buff -> buff_duration += timespan_t::from_seconds( e.driver() -> effectN( 1 ).base_value() );
    }
  }
};

struct shaman_module_t : public module_t
{
  shaman_module_t() : module_t( SHAMAN ) {}

  player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new shaman_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new shaman_report_t( *p ) );
    return p;
  }

  bool valid() const override
  { return true; }

  void init( player_t* p ) const override
  {
    p -> buffs.bloodlust  = haste_buff_creator_t( p, "bloodlust", p -> find_spell( 2825 ) )
                            .max_stack( 1 );

    p -> buffs.exhaustion = buff_creator_t( p, "exhaustion", p -> find_spell( 57723 ) )
                            .max_stack( 1 )
                            .quiet( true );
  }

  void static_init() const override
  {
    register_special_effect( 184919, elemental_bellows_t() );
    register_special_effect( 184920, furious_winds_t( "windfury_attack" ) );
    register_special_effect( 184920, furious_winds_t( "windfury_attack_oh" ) );
    register_special_effect( 208722, echoes_of_the_great_sundering_t() );
    register_special_effect( 208722, echoes_of_the_great_sundering_buff_t(), true );
    register_special_effect( 208741, emalons_charged_core_t() );
    register_special_effect( 208741, emalons_charged_core_buff_t(), true );
    register_special_effect( 224837, pristine_protoscale_girdle_t() );
    register_special_effect( 214260, storm_tempests_t( "stormstrike" ) ); // TODO: Windstrike?
    register_special_effect( 214131, the_deceivers_blood_pact_t() );
    register_special_effect( 214147, spiritual_journey_t(), true );
    register_special_effect( 213359, akainus_absolute_justice_t() );
    register_special_effect( 208699, alakirs_acrimony_t<chained_base_t>( "chain_lightning" ) );
    register_special_effect( 208699, alakirs_acrimony_t<chained_base_t>( "lava_beam" ) );
    register_special_effect( 208699, alakirs_acrimony_t<chained_overload_base_t>( "chain_lightning_overload" ) );
    register_special_effect( 208699, alakirs_acrimony_t<chained_overload_base_t>( "lava_beam_overload" ) );
    register_special_effect( 207994, eotn_buff_fire_t(), true );
    register_special_effect( 207994, eotn_buff_shock_t(), true );
    register_special_effect( 207994, eotn_buff_chill_t(), true );
    register_special_effect( 234814, uncertain_reminder_t() );
  }

  void register_hotfixes() const override
  {
    /*
    hotfix::register_spell( "Shaman", "2016-08-23", "Windfury base proc rate has been increased to 10% (was 5%.)", 33757 )
      .field( "proc_chance" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 10 )
      .verification_value( 5 );

    hotfix::register_effect( "Shaman", "2016-08-23", "Rockbiter damage has been increased to 155% Attack Power (was 135%).", 284355 )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 1.55 )
      .verification_value( 1.35 );

    hotfix::register_effect( "Shaman", "2016-08-23", "Lightning Bolt (Enhancement) damage has been increased to 30% Spell Power (was 25%).", 273980 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.3 )
      .verification_value( 0.25 );

    hotfix::register_effect( "Shaman", "2016-08-23", "Earthen Spike debuff damage has been increased to 15% (was 10%).", 274448 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 15 )
      .verification_value( 10 );

    hotfix::register_effect( "Shaman", "2016-08-23", "Storm Elemental Wind Gust now generates 10 Maelstrom per cast (was 8).", 339432 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 10 )
      .verification_value( 8 );

    hotfix::register_effect( "Shaman", "2016-08-23", "Frost Shock damage has been increased slightly to 56% (was 52%).", 288995 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.56 )
      .verification_value( 0.52 );

    hotfix::register_effect( "Shaman", "2016-08-23", "Liquid Magma Totem damage has been increased to 80% (was 70%)).", 282015 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.8 )
      .verification_value( 0.7 );
    
    hotfix::register_effect( "Shaman", "2016-09-23", "Chain Lightning (Elemental) damage has been increased by 23%", 275203 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.23 )
      .verification_value( 1.3 );

    hotfix::register_effect( "Shaman", "2016-09-23", "Chain Lightning (Elemental) maelstrom generation increased to 6.", 325428 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 6 )
      .verification_value( 4 );

    hotfix::register_effect( "Shaman", "2016-09-23", "Lightning bolt damaged increased by 23%", 274643 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.23 )
      .verification_value( 1.3 );

    hotfix::register_effect( "Shaman", "2016-09-23", "Lava burst damage has been increased by 5%", 43841 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 2.1 );

    hotfix::register_effect( "Shaman", "2016-09-23", "Elemental Mastery effects have been increased by 12.5%", 238582 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.125 )
      .verification_value( 2 );

    hotfix::register_effect( "Shaman", "2016-09-23", "Storm Elemental's Call Lightning damage has been increased by 20%", 219409 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.2 )
      .verification_value( 0.7 );

    hotfix::register_effect( "Shaman", "2016-09-23", "Storm Elemental's Wind Gust damage has been increased by 20%", 219326 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.2 )
      .verification_value( 0.35 );

    hotfix::register_effect( "Shaman", "2016-09-23", "Chain Lightning (Restoration) damage has been increased by 20%", 155 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.2 )
      .verification_value( 1.8 );

    hotfix::register_spell( "Shaman", "2016-09-23", "Windfury activation chance increased to 20%.", 33757 )
      .field( "proc_chance" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 20 )
      .verification_value( 10 );
      */
 /*   hotfix::register_spell("Shaman", "2016-10-25", "Earth Shock damage increased by 15%. ", 8042)
    .field("sp_coefficient")
    .operation(hotfix::HOTFIX_MUL)
    .modifier(1.15)
    .verification_value(8);

  hotfix::register_spell("Shaman", "2016-10-25", "Frost Shock damage increased by 15%. ", 196840)
    .field("sp_coefficient")
    .operation(hotfix::HOTFIX_MUL)
    .modifier(1.15)
    .verification_value(0.56);*/
  }

  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::shaman()
{
  static shaman_module_t m;
  return &m;
}

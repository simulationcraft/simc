// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "player/covenant.hpp"
#include "util/util.hpp"

namespace {

// ==========================================================================
// Mage
// ==========================================================================

// Forward declarations
struct mage_t;

namespace pets {
  namespace water_elemental {
    struct water_elemental_pet_t;
  }
}

template <typename Action, typename Actor, typename... Args>
action_t* get_action( util::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}

enum frozen_type_e
{
  FROZEN_WINTERS_CHILL = 0,
  FROZEN_FINGERS_OF_FROST,
  FROZEN_ROOT,

  FROZEN_NONE,
  FROZEN_MAX
};

enum frozen_flag_e
{
  FF_WINTERS_CHILL    = 1 << FROZEN_WINTERS_CHILL,
  FF_FINGERS_OF_FROST = 1 << FROZEN_FINGERS_OF_FROST,
  FF_ROOT             = 1 << FROZEN_ROOT
};

enum ground_aoe_type_e
{
  AOE_BLIZZARD = 0,
  AOE_COMET_STORM,
  AOE_FLAME_PATCH,
  AOE_FROZEN_ORB,
  AOE_METEOR_BURN,
  AOE_MAX
};

enum target_trigger_type_e
{
  TT_NONE,
  TT_MAIN_TARGET,
  TT_ALL_TARGETS
};

struct state_switch_t
{
private:
  bool state;
  timespan_t last_enable;
  timespan_t last_disable;

public:
  state_switch_t()
  {
    reset();
  }

  bool enable( timespan_t now )
  {
    if ( last_enable == now )
      return false;

    state = true;
    last_enable = now;
    return true;
  }

  bool disable( timespan_t now )
  {
    if ( last_disable == now )
      return false;

    state = false;
    last_disable = now;
    return true;
  }

  bool on() const
  {
    return state;
  }

  timespan_t duration( timespan_t now ) const
  {
    return state ? now - last_enable : 0_ms;
  }

  void reset()
  {
    state        = false;
    last_enable  = timespan_t::min();
    last_disable = timespan_t::min();
  }
};

/// Icicle container object, contains a timestamp and its corresponding icicle data!
struct icicle_tuple_t
{
  action_t* action;
  event_t*  expiration;
};

struct mage_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* nether_tempest;

    // Covenant Abilities
    dot_t* radiant_spark;
  } dots;

  struct debuffs_t
  {
    buff_t* frozen;
    buff_t* grisly_icicle;
    buff_t* touch_of_the_magi;
    buff_t* winters_chill;

    // Azerite
    buff_t* packed_ice;

    // Covenant Abilities
    buff_t* mirrors_of_torment;
    buff_t* radiant_spark_vulnerability;
  } debuffs;

  mage_td_t( player_t* target, mage_t* mage );
};

struct buff_stack_benefit_t
{
  const buff_t* buff;
  std::vector<benefit_t*> buff_stack_benefit;

  buff_stack_benefit_t( const buff_t* _buff, const std::string& prefix ) :
    buff( _buff ),
    buff_stack_benefit()
  {
    for ( int i = 0; i <= buff->max_stack(); i++ )
    {
      buff_stack_benefit.push_back( buff->player->get_benefit(
        prefix + " " + buff->data().name_cstr() + " " + util::to_string( i ) ) );
    }
  }

  void update()
  {
    auto stack = as<unsigned>( buff->check() );
    for ( unsigned i = 0; i < buff_stack_benefit.size(); i++ )
      buff_stack_benefit[ i ]->update( i == stack );
  }
};

struct cooldown_waste_data_t : private noncopyable
{
  const cooldown_t* cd;
  double buffer;

  extended_sample_data_t normal;
  extended_sample_data_t cumulative;

  cooldown_waste_data_t( const cooldown_t* cooldown, bool simple = true ) :
    cd( cooldown ),
    buffer(),
    normal( cd->name_str + " cooldown waste", simple ),
    cumulative( cd->name_str + " cumulative cooldown waste", simple )
  { }

  void add( timespan_t cd_override = timespan_t::min(), timespan_t time_to_execute = 0_ms )
  {
    if ( cd_override == 0_ms || ( cd_override < 0_ms && cd->duration <= 0_ms ) )
      return;

    if ( cd->ongoing() )
    {
      normal.add( 0.0 );
    }
    else
    {
      double wasted = ( cd->sim.current_time() - cd->last_charged ).total_seconds();

      // Waste caused by execute time is unavoidable for single charge spells, don't count it.
      if ( cd->charges == 1 )
        wasted -= time_to_execute.total_seconds();

      normal.add( wasted );
      buffer += wasted;
    }
  }

  bool active() const
  {
    return normal.count() > 0 && cumulative.sum() > 0;
  }

  void merge( const cooldown_waste_data_t& other )
  {
    normal.merge( other.normal );
    cumulative.merge( other.cumulative );
  }

  void analyze()
  {
    normal.analyze();
    cumulative.analyze();
  }

  void datacollection_begin()
  {
    buffer = 0.0;
  }

  void datacollection_end()
  {
    if ( !cd->ongoing() )
      buffer += ( cd->sim.current_time() - cd->last_charged ).total_seconds();

    cumulative.add( buffer );
  }
};

template <size_t N>
struct effect_source_t : private noncopyable
{
  const std::string name_str;
  std::array<simple_sample_data_t, N> counts;
  std::array<int, N> iteration_counts;

  effect_source_t( util::string_view name ) :
    name_str( name ),
    counts(),
    iteration_counts()
  { }

  void occur( size_t type )
  {
    assert( type < N );
    iteration_counts[ type ]++;
  }

  double count( size_t type ) const
  {
    assert( type < N );
    return counts[ type ].pretty_mean();
  }

  double count_total() const
  {
    double res = 0.0;
    for ( const auto& c : counts )
      res += c.pretty_mean();
    return res;
  }

  bool active() const
  {
    return count_total() > 0.0;
  }

  void merge( const effect_source_t& other )
  {
    for ( size_t i = 0; i < counts.size(); i++ )
      counts[ i ].merge( other.counts[ i ] );
  }

  void datacollection_begin()
  {
    range::fill( iteration_counts, 0 );
  }

  void datacollection_end()
  {
    for ( size_t i = 0; i < counts.size(); i++ )
      counts[ i ].add( as<double>( iteration_counts[ i ] ) );
  }
};

using shatter_source_t = effect_source_t<FROZEN_MAX>;

struct mage_t : public player_t
{
public:
  // Icicles
  std::vector<icicle_tuple_t> icicles;

  // Events
  struct events_t
  {
    event_t* enlightened;
    event_t* focus_magic;
    event_t* icicle;
    event_t* from_the_ashes;
    event_t* time_anomaly;
  } events;

  // Active
  player_t* last_bomb_target;
  player_t* last_frostbolt_target;

  // State switches for rotation selection
  state_switch_t burn_phase;

  // Ground AoE tracking
  std::array<timespan_t, AOE_MAX> ground_aoe_expiration;

  // Miscellaneous
  int remaining_winters_chill;
  double distance_from_rune;
  double lucid_dreams_refund;
  double strive_for_perfection_multiplier;
  double vision_of_perfection_multiplier;

  // Data collection
  auto_dispose<std::vector<cooldown_waste_data_t*> > cooldown_waste_data_list;
  auto_dispose<std::vector<shatter_source_t*> > shatter_source_list;

  // Cached actions
  struct actions_t
  {
    action_t* agonizing_backlash;
    action_t* arcane_assault;
    action_t* arcane_echo;
    action_t* conflagration_flare_up;
    action_t* glacial_assault;
    action_t* ignite;
    action_t* legendary_frozen_orb;
    action_t* legendary_meteor;
    action_t* living_bomb_dot;
    action_t* living_bomb_dot_spread;
    action_t* living_bomb_explosion;
    action_t* tormenting_backlash;
    action_t* touch_of_the_magi_explosion;

    struct icicles_t
    {
      action_t* frostbolt;
      action_t* flurry;
      action_t* lucid_dreams;
    } icicle;
  } action;

  // Benefits
  struct benefits_t
  {
    struct arcane_charge_benefits_t
    {
      std::unique_ptr<buff_stack_benefit_t> arcane_barrage;
      std::unique_ptr<buff_stack_benefit_t> arcane_blast;
      std::unique_ptr<buff_stack_benefit_t> nether_tempest;
    } arcane_charge;

    struct blaster_master_benefits_t
    {
      std::unique_ptr<buff_stack_benefit_t> combustion;
      std::unique_ptr<buff_stack_benefit_t> rune_of_power;
      std::unique_ptr<buff_stack_benefit_t> searing_touch;
    } blaster_master;
  } benefits;

  // Buffs
  struct buffs_t
  {
    // Arcane
    buff_t* arcane_charge;
    buff_t* arcane_power;
    buff_t* clearcasting;
    buff_t* clearcasting_channel; // Hidden buff which governs tick and channel time
    buff_t* evocation;
    buff_t* presence_of_mind;

    buff_t* arcane_familiar;
    buff_t* chrono_shift;
    buff_t* enlightened_damage;
    buff_t* enlightened_mana;
    buff_t* rule_of_threes;
    buff_t* time_warp;


    // Fire
    buff_t* combustion;
    buff_t* fireball;
    buff_t* heating_up;
    buff_t* hot_streak;

    buff_t* alexstraszas_fury;
    buff_t* frenetic_speed;
    buff_t* pyroclasm;


    // Frost
    buff_t* brain_freeze;
    buff_t* fingers_of_frost;
    buff_t* icicles;
    buff_t* icy_veins;

    buff_t* bone_chilling;
    buff_t* chain_reaction;
    buff_t* freezing_rain;
    buff_t* ice_floes;
    buff_t* ray_of_frost;


    // Shared
    buff_t* incanters_flow;
    buff_t* rune_of_power;
    buff_t* focus_magic_crit;
    buff_t* focus_magic_int;


    // Azerite
    buff_t* arcane_pummeling;
    buff_t* brain_storm;

    buff_t* blaster_master;
    buff_t* firemind;
    buff_t* flames_of_alacrity;
    buff_t* wildfire;

    buff_t* frigid_grasp;
    buff_t* tunnel_of_ice;


    // Runeforge Legendaries
    buff_t* arcane_harmony;
    buff_t* siphon_storm;
    buff_t* temporal_warp;

    buff_t* fevered_incantation;
    buff_t* firestorm;
    buff_t* molten_skyfall;
    buff_t* molten_skyfall_ready;
    buff_t* sun_kings_blessing;
    buff_t* sun_kings_blessing_ready;

    buff_t* cold_front;
    buff_t* cold_front_ready;
    buff_t* freezing_winds;
    buff_t* slick_ice;

    buff_t* disciplinary_command;
    buff_t* disciplinary_command_arcane; // Hidden buff
    buff_t* disciplinary_command_frost; // Hidden buff
    buff_t* disciplinary_command_fire; // Hidden buff
    buff_t* expanded_potential;


    // Covenant Abilities
    buff_t* deathborne;


    // Soulbind Conduits
    buff_t* nether_precision;

    buff_t* flame_accretion;
    buff_t* infernal_cascade;

    buff_t* siphoned_malice;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* arcane_power;
    cooldown_t* combustion;
    cooldown_t* cone_of_cold;
    cooldown_t* fire_blast;
    cooldown_t* from_the_ashes;
    cooldown_t* frost_nova;
    cooldown_t* frozen_orb;
    cooldown_t* icy_veins;
    cooldown_t* phoenix_flames;
    cooldown_t* presence_of_mind;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* evocation;
    gain_t* lucid_dreams;
    gain_t* mana_gem;
    gain_t* arcane_barrage;
    gain_t* mirrors_of_torment;
  } gains;

  // Options
  struct options_t
  {
    timespan_t firestarter_time = 0_ms;
    timespan_t frozen_duration = 1.0_s;
    timespan_t scorch_delay = 15_ms;
    double lucid_dreams_proc_chance_arcane = 0.075;
    double lucid_dreams_proc_chance_fire = 0.1;
    double lucid_dreams_proc_chance_frost = 0.075;
    timespan_t enlightened_interval = 2.0_s;
    timespan_t focus_magic_interval = 1.5_s;
    double focus_magic_stddev = 0.1;
    double focus_magic_crit_chance = 0.85;
    timespan_t from_the_ashes_interval = 2.0_s;
    timespan_t mirrors_of_torment_interval = 1.5_s;
  } options;

  // Pets
  struct pets_t
  {
    pets::water_elemental::water_elemental_pet_t* water_elemental = nullptr;
    std::vector<pet_t*> mirror_images;
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* heating_up_generated;         // Crits without HU/HS
    proc_t* heating_up_removed;           // Non-crits with HU >200ms after application
    proc_t* heating_up_ib_converted;      // IBs used on HU
    proc_t* hot_streak;                   // Total HS generated
    proc_t* hot_streak_pyromaniac;        // Total HS from Pyromaniac
    proc_t* hot_streak_spell;             // HU/HS spell impacts
    proc_t* hot_streak_spell_crit;        // HU/HS spell crits
    proc_t* hot_streak_spell_crit_wasted; // HU/HS spell crits with HS

    proc_t* ignite_applied;    // Direct ignite applications
    proc_t* ignite_new_spread; // Spread to new target
    proc_t* ignite_overwrite;  // Spread to target with existing ignite

    proc_t* brain_freeze;
    proc_t* brain_freeze_mirrors;
    proc_t* brain_freeze_used;
    proc_t* fingers_of_frost;
    proc_t* fingers_of_frost_wasted;
    proc_t* winters_chill_applied;
    proc_t* winters_chill_consumed;
  } procs;

  struct shuffled_rngs_t
  {
    shuffled_rng_t* time_anomaly;
  } shuffled_rng;

  // Sample data
  struct sample_data_t
  {
    std::unique_ptr<extended_sample_data_t> icy_veins_duration;
    std::unique_ptr<extended_sample_data_t> burn_duration_history;
    std::unique_ptr<extended_sample_data_t> burn_initial_mana;
  } sample_data;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_barrage_2;
    const spell_data_t* arcane_barrage_3;
    const spell_data_t* arcane_charge;
    const spell_data_t* arcane_explosion_2;
    const spell_data_t* arcane_mage;
    const spell_data_t* arcane_power_2;
    const spell_data_t* arcane_power_3;
    const spell_data_t* clearcasting;
    const spell_data_t* clearcasting_2;
    const spell_data_t* clearcasting_3;
    const spell_data_t* evocation_2;
    const spell_data_t* presence_of_mind_2;
    const spell_data_t* savant;
    const spell_data_t* touch_of_the_magi;
    const spell_data_t* touch_of_the_magi_2;

    // Fire
    const spell_data_t* combustion_2;
    const spell_data_t* critical_mass;
    const spell_data_t* critical_mass_2;
    const spell_data_t* dragons_breath_2;
    const spell_data_t* fireball_2;
    const spell_data_t* fireball_3;
    const spell_data_t* fire_blast_2;
    const spell_data_t* fire_blast_3;
    const spell_data_t* fire_blast_4;
    const spell_data_t* fire_mage;
    const spell_data_t* flamestrike_2;
    const spell_data_t* flamestrike_3;
    const spell_data_t* hot_streak;
    const spell_data_t* ignite;
    const spell_data_t* phoenix_flames_2;
    const spell_data_t* pyroblast_2;

    // Frost
    const spell_data_t* brain_freeze;
    const spell_data_t* brain_freeze_2;
    const spell_data_t* blizzard_2;
    const spell_data_t* blizzard_3;
    const spell_data_t* cold_snap_2;
    const spell_data_t* fingers_of_frost;
    const spell_data_t* frost_mage;
    const spell_data_t* frost_nova_2;
    const spell_data_t* frostbolt_2;
    const spell_data_t* ice_lance_2;
    const spell_data_t* icicles;
    const spell_data_t* icicles_2;
    const spell_data_t* icy_veins_2;
    const spell_data_t* shatter;
    const spell_data_t* shatter_2;
  } spec;

  // State
  struct state_t
  {
    bool brain_freeze_active;
    bool fingers_of_frost_active;
    int mana_gem_charges;
    double from_the_ashes_mastery;
  } state;

  // Talents
  struct talents_list_t
  {
    // Tier 15
    const spell_data_t* amplification;
    const spell_data_t* rule_of_threes;
    const spell_data_t* arcane_familiar;
    const spell_data_t* firestarter;
    const spell_data_t* pyromaniac;
    const spell_data_t* searing_touch;
    const spell_data_t* bone_chilling;
    const spell_data_t* lonely_winter;
    const spell_data_t* ice_nova;

    // Tier 25
    const spell_data_t* shimmer;
    const spell_data_t* master_of_time; // NYI
    const spell_data_t* slipstream;
    const spell_data_t* blazing_soul; // NYI
    const spell_data_t* blast_wave;
    const spell_data_t* glacial_insulation; // NYI
    const spell_data_t* ice_floes;

    // Tier 30
    const spell_data_t* incanters_flow;
    const spell_data_t* focus_magic;
    const spell_data_t* rune_of_power;

    // Tier 35
    const spell_data_t* resonance;
    const spell_data_t* arcane_echo;
    const spell_data_t* nether_tempest;
    const spell_data_t* flame_on;
    const spell_data_t* alexstraszas_fury;
    const spell_data_t* from_the_ashes;
    const spell_data_t* frozen_touch;
    const spell_data_t* chain_reaction;
    const spell_data_t* ebonbolt;

    // Tier 40
    const spell_data_t* ice_ward;
    const spell_data_t* ring_of_frost; // NYI
    const spell_data_t* chrono_shift;
    const spell_data_t* frenetic_speed;
    const spell_data_t* frigid_winds; // NYI

    // Tier 45
    const spell_data_t* reverberate;
    const spell_data_t* arcane_orb;
    const spell_data_t* supernova;
    const spell_data_t* flame_patch;
    const spell_data_t* conflagration;
    const spell_data_t* living_bomb;
    const spell_data_t* freezing_rain;
    const spell_data_t* splitting_ice;
    const spell_data_t* comet_storm;

    // Tier 50
    const spell_data_t* overpowered;
    const spell_data_t* time_anomaly;
    const spell_data_t* enlightened;
    const spell_data_t* kindling;
    const spell_data_t* pyroclasm;
    const spell_data_t* meteor;
    const spell_data_t* thermal_void;
    const spell_data_t* ray_of_frost;
    const spell_data_t* glacial_spike;
  } talents;

  // Azerite Powers
  struct azerite_powers_t
  {
    // Arcane
    azerite_power_t arcane_pressure;
    azerite_power_t arcane_pummeling;
    azerite_power_t brain_storm;
    azerite_power_t equipoise;
    azerite_power_t explosive_echo;
    azerite_power_t galvanizing_spark;

    // Fire
    azerite_power_t blaster_master;
    azerite_power_t duplicative_incineration;
    azerite_power_t firemind;
    azerite_power_t flames_of_alacrity;
    azerite_power_t trailing_embers;
    azerite_power_t wildfire;

    // Frost
    azerite_power_t flash_freeze;
    azerite_power_t frigid_grasp;
    azerite_power_t glacial_assault;
    azerite_power_t packed_ice;
    azerite_power_t tunnel_of_ice;
    azerite_power_t whiteout;
  } azerite;

  // Runeforge Legendaries
  struct runeforge_legendaries_t
  {
    // Arcane
    item_runeforge_t arcane_bombardment;
    item_runeforge_t arcane_harmony;
    item_runeforge_t siphon_storm;
    item_runeforge_t temporal_warp;

    // Fire
    item_runeforge_t fevered_incantation;
    item_runeforge_t firestorm;
    item_runeforge_t molten_skyfall;
    item_runeforge_t sun_kings_blessing;

    // Frost
    item_runeforge_t cold_front;
    item_runeforge_t freezing_winds;
    item_runeforge_t glacial_fragments;
    item_runeforge_t slick_ice;

    // Shared
    item_runeforge_t disciplinary_command;
    item_runeforge_t expanded_potential;
    item_runeforge_t grisly_icicle;
  } runeforge;

  // Soulbind Conduits
  struct soulbind_conduits_t
  {
    // Arcane
    conduit_data_t arcane_prodigy;
    conduit_data_t artifice_of_the_archmage;
    conduit_data_t magis_brand;
    conduit_data_t nether_precision;

    // Fire
    conduit_data_t controlled_destruction;
    conduit_data_t flame_accretion;
    conduit_data_t infernal_cascade;
    conduit_data_t master_flame;

    // Frost
    conduit_data_t ice_bite;
    conduit_data_t icy_propulsion;
    conduit_data_t shivering_core;
    conduit_data_t unrelenting_cold;

    // Shared
    conduit_data_t discipline_of_the_grove;
    conduit_data_t flow_of_time;
    conduit_data_t gift_of_the_lich;
    conduit_data_t grounding_surge;
    conduit_data_t ire_of_the_ascended;
    conduit_data_t siphoned_malice;
  } conduits;

  struct uptimes_t
  {
    uptime_t* burn_phase;
    uptime_t* conserve_phase;
  } uptime;

public:
  mage_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE );

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void create_buffs() override;
  void create_options() override;
  void init_action_list() override;
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  void init_gains() override;
  void init_procs() override;
  void init_benefits() override;
  void init_uptimes() override;
  void init_rng() override;
  void init_finished() override;
  void invalidate_cache( cache_e ) override;
  void init_resources( bool ) override;
  double resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  double resource_loss( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  void recalculate_resource_max( resource_e, gain_t* g = nullptr ) override;
  void reset() override;
  std::unique_ptr<expr_t> create_expression( util::string_view ) override;
  std::unique_ptr<expr_t> create_action_expression( action_t&, util::string_view ) override;
  action_t* create_action( util::string_view, const std::string& ) override;
  void create_actions() override;
  void create_pets() override;
  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_SPELL; }
  stat_e convert_hybrid_stat( stat_e ) const override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_mastery() const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t* ) const override;
  double composite_player_target_multiplier( player_t*, school_e ) const override;
  double composite_spell_crit_chance() const override;
  double composite_rating_multiplier( rating_e ) const override;
  double matching_gear_multiplier( attribute_e ) const override;
  void update_movement( timespan_t ) override;
  void teleport( double, timespan_t ) override;
  double passive_movement_modifier() const override;
  void arise() override;
  void combat_begin() override;
  void combat_end() override;
  std::string create_profile( save_e ) override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;
  void analyze( sim_t& ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void regen( timespan_t ) override;
  void moving() override;
  void vision_of_perfection_proc() override;

  target_specific_t<mage_td_t> target_data;

  mage_td_t* get_target_data( player_t* target ) const override
  {
    mage_td_t*& td = target_data[ target ];
    if ( !td )
      td = new mage_td_t( target, const_cast<mage_t*>( this ) );
    return td;
  }

  // Public mage functions:
  cooldown_waste_data_t* get_cooldown_waste_data( const cooldown_t* cd )
  {
    for ( auto cdw : cooldown_waste_data_list )
    {
      if ( cdw->cd->name_str == cd->name_str )
        return cdw;
    }

    auto cdw = new cooldown_waste_data_t( cd );
    cooldown_waste_data_list.push_back( cdw );
    return cdw;
  }

  shatter_source_t* get_shatter_source( util::string_view name )
  {
    for ( auto ss : shatter_source_list )
    {
      if ( ss->name_str == name )
        return ss;
    }

    auto ss = new shatter_source_t( name );
    shatter_source_list.push_back( ss );
    return ss;
  }

  void      update_rune_distance( double distance );
  void      update_enlightened();
  void      update_from_the_ashes();
  action_t* get_icicle();
  bool      trigger_delayed_buff( buff_t* buff, double chance = -1.0, timespan_t delay = 0.15_s );
  void      trigger_brain_freeze( double chance, proc_t* source, timespan_t delay = 0.15_s );
  void      trigger_fof( double chance, int stacks, proc_t* source );
  void      trigger_icicle( player_t* icicle_target, bool chain = false );
  void      trigger_icicle_gain( player_t* icicle_target, action_t* icicle_action );
  void      trigger_evocation( timespan_t duration_override = timespan_t::min(), bool hasted = true );
  bool      trigger_crowd_control( const action_state_t* s, spell_mechanic type, timespan_t duration = timespan_t::min() );
  void      trigger_lucid_dreams( player_t* trigger_target, double cost );

  void apl_arcane();
  void apl_fire();
  void apl_frost();
};

namespace pets {

struct mage_pet_t : public pet_t
{
  mage_pet_t( sim_t* sim, mage_t* owner, util::string_view pet_name,
              bool guardian = false, bool dynamic = false ) :
    pet_t( sim, owner, pet_name, guardian, dynamic )
  { }

  const mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  mage_t* o()
  { return static_cast<mage_t*>( owner ); }
};

struct mage_pet_spell_t : public spell_t
{
  mage_pet_spell_t( util::string_view n, mage_pet_t* p, const spell_data_t* s ) :
    spell_t( n, p, s )
  {
    may_crit = tick_may_crit = true;
    weapon_multiplier = 0.0;
    gcd_type = gcd_haste_type::NONE;
  }

  mage_t* o()
  { return static_cast<mage_pet_t*>( player )->o(); }

  const mage_t* o() const
  { return static_cast<mage_pet_t*>( player )->o(); }
};

namespace water_elemental {

// ==========================================================================
// Pet Water Elemental
// ==========================================================================

struct water_elemental_pet_t : public mage_pet_t
{
  struct actions_t
  {
    action_t* freeze;
  } action;

  water_elemental_pet_t( sim_t* sim, mage_t* owner ) :
    mage_pet_t( sim, owner, "water_elemental" ),
    action()
  {
    owner_coeff.sp_from_sp = 0.75;
  }

  void init_action_list() override
  {
    action_list_str = "waterbolt";
    mage_pet_t::init_action_list();
  }

  action_t* create_action( util::string_view, const std::string& ) override;
  void      create_actions() override;
};

struct waterbolt_t : public mage_pet_spell_t
{
  waterbolt_t( util::string_view n, water_elemental_pet_t* p, util::string_view options_str ) :
    mage_pet_spell_t( n, p, p->find_pet_spell( "Waterbolt" ) )
  {
    parse_options( options_str );
  }
};

struct freeze_t : public mage_pet_spell_t
{
  freeze_t( util::string_view n, water_elemental_pet_t* p ) :
    mage_pet_spell_t( n, p, p->find_pet_spell( "Freeze" ) )
  {
    background = true;
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    mage_pet_spell_t::impact( s );
    o()->trigger_crowd_control( s, MECHANIC_ROOT );
  }
};

action_t* water_elemental_pet_t::create_action( util::string_view name, const std::string& options_str )
{
  if ( name == "waterbolt" ) return new waterbolt_t( name, this, options_str );

  return mage_pet_t::create_action( name, options_str );
}

void water_elemental_pet_t::create_actions()
{
  action.freeze = get_action<freeze_t>( "freeze", this );

  mage_pet_t::create_actions();
}

}  // water_elemental

namespace mirror_image {

// ==========================================================================
// Pet Mirror Image
// ==========================================================================

struct mirror_image_pet_t : public mage_pet_t
{
  mirror_image_pet_t( sim_t* sim, mage_t* owner ) :
    mage_pet_t( sim, owner, "mirror_image", true, true )
  {
    owner_coeff.sp_from_sp = 0.55;
  }

  action_t* create_action( util::string_view, const std::string& ) override;

  void init_action_list() override
  {
    action_list_str = "frostbolt";
    mage_pet_t::init_action_list();
  }
};

struct frostbolt_t : public mage_pet_spell_t
{
  frostbolt_t( util::string_view n, mirror_image_pet_t* p, util::string_view options_str ) :
    mage_pet_spell_t( n, p, p->find_spell( 59638 ) )
  {
    parse_options( options_str );
  }

  void init_finished() override
  {
    stats = o()->pets.mirror_images.front()->get_stats( name_str );
    mage_pet_spell_t::init_finished();
  }
};

action_t* mirror_image_pet_t::create_action( util::string_view name, const std::string& options_str )
{
  if ( name == "frostbolt" ) return new frostbolt_t( name, this, options_str );

  return mage_pet_t::create_action( name, options_str );
}

}  // mirror_image

}  // pets

namespace buffs {

// Custom buffs =============================================================

// Touch of the Magi debuff =================================================

struct touch_of_the_magi_t : public buff_t
{
  double accumulated_damage;

  touch_of_the_magi_t( mage_td_t* td ) :
    buff_t( *td, "touch_of_the_magi", td->source->find_spell( 210824 ) ),
    accumulated_damage()
  { }

  void reset() override
  {
    buff_t::reset();
    accumulated_damage = 0.0;
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    auto p = debug_cast<mage_t*>( source );
    auto explosion = p->action.touch_of_the_magi_explosion;

    explosion->set_target( player );
    double damage_fraction = p->spec.touch_of_the_magi->effectN( 1 ).percent();
    // TODO: Higher ranks of this spell use floating point values in the spell data.
    // Verify whether we need a floor operation here to trim those values down to integers,
    // which sometimes happens when Blizzard uses floating point numbers like this.
    damage_fraction += p->conduits.magis_brand.percent();
    explosion->base_dd_min = explosion->base_dd_max = damage_fraction * accumulated_damage;
    explosion->execute();

    accumulated_damage = 0.0;
  }

  void accumulate_damage( const action_state_t* s )
  {
    sim->print_debug(
      "{}'s {} accumulates {} additional damage: {} -> {}",
      player->name(), name(), s->result_total,
      accumulated_damage, accumulated_damage + s->result_total );

    accumulated_damage += s->result_total;
  }
};

struct combustion_buff_t : public buff_t
{
  double current_amount;
  double multiplier;

  combustion_buff_t( mage_t* p ) :
    buff_t( p, "combustion", p->find_spell( 190319 ) ),
    current_amount(),
    multiplier( data().effectN( 3 ).percent() )
  {
    set_cooldown( 0_ms );
    set_default_value_from_effect( 1 );
    set_tick_zero( true );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    modify_duration( p->spec.combustion_2->effectN( 1 ).time_value() );

    set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
    {
      if ( cur == 0 )
      {
        player->stat_loss( STAT_MASTERY_RATING, current_amount );
        current_amount = 0.0;
      }
    } );

    set_tick_callback( [ this ] ( buff_t*, int, timespan_t )
    {
      double new_amount = multiplier * player->composite_spell_crit_rating();
      double diff = new_amount - current_amount;

      if ( diff > 0.0 ) player->stat_gain( STAT_MASTERY_RATING,  diff );
      if ( diff < 0.0 ) player->stat_loss( STAT_MASTERY_RATING, -diff );

      current_amount = new_amount;
    } );
  }

  void reset() override
  {
    buff_t::reset();
    current_amount = 0.0;
  }
};

// TODO: Verify what happens when Expanded Potential is triggered and
// consumed at the same time, e.g., what happens when casting Fireball
// into Pyroblast with Hot Streak when Expanded Potential is up, but
// the Fireball also triggers a new instance of Expanded Potential?
struct expanded_potential_buff_t : public buff_t
{
  expanded_potential_buff_t( mage_t* p, util::string_view name, const spell_data_t* spell_data ) :
    buff_t( p, name, spell_data )
  { }

  void decrement( int stacks, double value ) override
  {
    auto mage = debug_cast<mage_t*>( player );
    if ( check() && mage->buffs.expanded_potential->check() )
      mage->buffs.expanded_potential->expire();
    else
      buff_t::decrement( stacks, value );
  }
};

struct ice_floes_buff_t : public buff_t
{
  ice_floes_buff_t( mage_t* p ) :
    buff_t( p, "ice_floes", p->talents.ice_floes )
  { }

  void decrement( int stacks, double value ) override
  {
    if ( check() == 0 )
      return;

    if ( sim->current_time() - last_trigger > 0.5_s )
      buff_t::decrement( stacks, value );
    else
      sim->print_debug( "Ice Floes removal ignored due to 500 ms protection" );
  }
};

struct icy_veins_buff_t : public buff_t
{
  icy_veins_buff_t( mage_t* p ) :
    buff_t( p, "icy_veins", p->find_spell( 12472 ) )
  {
    set_default_value_from_effect( 1 );
    set_cooldown( 0_ms );
    set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    modify_duration( p->spec.icy_veins_2->effectN( 1 ).time_value() );
    modify_duration( p->talents.thermal_void->effectN( 2 ).time_value() );
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    auto mage = debug_cast<mage_t*>( player );
    if ( mage->talents.thermal_void->ok() && duration == 0_ms )
      mage->sample_data.icy_veins_duration->add( elapsed( sim->current_time() ).total_seconds() );

    mage->buffs.frigid_grasp->expire();
    mage->buffs.slick_ice->expire();
  }
};

struct incanters_flow_t : public buff_t
{
  incanters_flow_t( mage_t* p ) :
    buff_t( p, "incanters_flow", p->find_spell( 116267 ) )
  {
    set_duration( 0_ms );
    set_period( p->talents.incanters_flow->effectN( 1 ).period() );
    set_chance( p->talents.incanters_flow->ok() );
    set_default_value_from_effect( 1 );
  }

  void reset() override
  {
    buff_t::reset();
    reverse = false;
  }

  void bump( int stacks, double value ) override
  {
    if ( at_max_stacks() )
      reverse = true;
    else
      buff_t::bump( stacks, value );
  }

  void decrement( int stacks, double value ) override
  {
    if ( check() == 1 )
      reverse = false;
    else
      buff_t::decrement( stacks, value );
  }
};

struct mirrors_of_torment_t : public buff_t
{
  cooldown_t* icd;
  int successful_triggers;

  double mana_pct;
  timespan_t reduction;

  mirrors_of_torment_t( mage_td_t* td ) :
    buff_t( *td, "mirrors_of_torment", td->source->find_spell( 314793 ) ),
    icd( td->source->get_cooldown( "mirrors_of_torment_icd" ) ),
    successful_triggers(),
    mana_pct(),
    reduction()
  {
    set_cooldown( 0_ms );
    set_reverse( true );
    icd->duration = data().internal_cooldown();

    auto p = debug_cast<mage_t*>( source );
    if ( p->options.mirrors_of_torment_interval <= 0_ms )
      return;

    switch ( p->specialization() )
    {
      case MAGE_ARCANE:
        mana_pct = p->find_spell( 345417 )->effectN( 1 ).percent();
        break;
      case MAGE_FIRE:
        reduction = -1000 * data().effectN( 2 ).time_value();
        break;
      default:
        break;
    }

    set_period( p->options.mirrors_of_torment_interval );
    set_tick_behavior( buff_tick_behavior::REFRESH );
    set_tick_callback( [ this, p ] ( buff_t*, int, timespan_t )
    {
      if ( icd->down() )
        return;

      successful_triggers++;
      icd->start();

      if ( successful_triggers % max_stack() == 0 )
      {
        p->action.tormenting_backlash->set_target( player );
        p->action.tormenting_backlash->execute();
      }
      else
      {
        p->action.agonizing_backlash->set_target( player );
        p->action.agonizing_backlash->execute();
      }

      p->buffs.siphoned_malice->trigger();

      switch ( p->specialization() )
      {
        case MAGE_ARCANE:
          p->resource_gain( RESOURCE_MANA, p->resources.max[ RESOURCE_MANA ] * mana_pct, p->gains.mirrors_of_torment );
          break;
        case MAGE_FIRE:
          p->cooldowns.fire_blast->adjust( reduction );
          break;
        case MAGE_FROST:
          p->trigger_brain_freeze( 1.0, p->procs.brain_freeze_mirrors, 0_ms );
          break;
        default:
          break;
      }

      decrement();
    } );
    set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
    { if ( cur == 0 ) successful_triggers = 0; } );
  }

  void reset() override
  {
    buff_t::reset();
    successful_triggers = 0;
  }

  bool freeze_stacks() override
  {
    return true;
  }
};

}  // buffs


namespace actions {

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_state_t : public action_state_t
{
  // Simple bitfield for tracking sources of the Frozen effect.
  unsigned frozen;

  // Damage multiplier that is in efffect only for frozen targets.
  double frozen_multiplier;

  mage_spell_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ),
    frozen(),
    frozen_multiplier( 1.0 )
  { }

  void initialize() override
  {
    action_state_t::initialize();
    frozen = 0u;
    frozen_multiplier = 1.0;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " frozen=";

    std::streamsize ss = s.precision();
    s.precision( 4 );

    if ( frozen )
    {
      std::string str;

      auto concat_flag_str = [ this, &str ] ( const char* flag_str, frozen_flag_e flag )
      {
        if ( frozen & flag )
        {
          if ( !str.empty() )
            str += "|";
          str += flag_str;
        }
      };

      concat_flag_str( "WC", FF_WINTERS_CHILL );
      concat_flag_str( "FOF", FF_FINGERS_OF_FROST );
      concat_flag_str( "ROOT", FF_ROOT );

      s << "{ " << str << " }";
    }
    else
    {
      s << "0";
    }

    s << " frozen_mul=" << frozen_multiplier;

    s.precision( ss );

    return s;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );

    auto mss = debug_cast<const mage_spell_state_t*>( s );
    frozen            = mss->frozen;
    frozen_multiplier = mss->frozen_multiplier;
  }

  virtual double composite_frozen_multiplier() const
  { return frozen ? frozen_multiplier : 1.0; }

  double composite_crit_chance() const override;

  double composite_da_multiplier() const override
  { return action_state_t::composite_da_multiplier() * composite_frozen_multiplier(); }

  double composite_ta_multiplier() const override
  { return action_state_t::composite_ta_multiplier() * composite_frozen_multiplier(); }
};

struct mage_spell_t : public spell_t
{
  static const snapshot_state_e STATE_FROZEN     = STATE_TGT_USER_1;
  static const snapshot_state_e STATE_FROZEN_MUL = STATE_TGT_USER_2;

  struct affected_by_t
  {
    // Permanent damage increase
    bool arcane_mage = true;
    bool fire_mage = true;
    bool frost_mage = true;

    // Temporary damage increase
    bool arcane_power = true;
    bool bone_chilling = true;
    bool incanters_flow = true;
    bool rune_of_power = true;
    bool savant = false;

    bool deathborne = true;
    bool siphoned_malice = true;

    // Misc
    bool combustion = true;
    bool ice_floes = false;
    bool shatter = false;

    bool deathborne_cleave = false;
    bool radiant_spark = true;
    bool shifting_power = true;
  } affected_by;

  struct triggers_t
  {
    bool bone_chilling = false;
    bool from_the_ashes = false;
    bool ignite = false;

    target_trigger_type_e hot_streak = TT_NONE;
    target_trigger_type_e kindling = TT_NONE;

    bool fevered_incantation = true; // TODO: Need to verify what exactly triggers Fevered Incantation.
    bool icy_propulsion = true;
    bool radiant_spark = false;
  } triggers;

  bool track_cd_waste;
  cooldown_waste_data_t* cd_waste;

public:
  mage_spell_t( util::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    affected_by(),
    triggers(),
    track_cd_waste(),
    cd_waste()
  {
    may_crit = tick_may_crit = true;
    weapon_multiplier = 0.0;
    affected_by.ice_floes = data().affected_by( p->talents.ice_floes->effectN( 1 ) );
    track_cd_waste = data().cooldown() > 0_ms || data().charge_cooldown() > 0_ms;
    energize_type = action_energize::NONE;
  }

  mage_t* p()
  { return static_cast<mage_t*>( player ); }

  const mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  mage_spell_state_t* cast_state( action_state_t* s )
  { return debug_cast<mage_spell_state_t*>( s ); }

  const mage_spell_state_t* cast_state( const action_state_t* s ) const
  { return debug_cast<const mage_spell_state_t*>( s ); }

  mage_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  action_state_t* new_state() override
  { return new mage_spell_state_t( this, target ); }

  static bool tt_applicable( const action_state_t* s, target_trigger_type_e t )
  {
    switch ( t )
    {
      case TT_NONE:
        return false;
      case TT_MAIN_TARGET:
        return s->chain_target == 0;
      case TT_ALL_TARGETS:
        return true;
    }
    return false;
  }

  void init() override
  {
    if ( initialized )
      return;

    spell_t::init();

    if ( affected_by.arcane_mage )
      base_multiplier *= 1.0 + p()->spec.arcane_mage->effectN( 1 ).percent();

    if ( affected_by.fire_mage )
      base_multiplier *= 1.0 + p()->spec.fire_mage->effectN( 1 ).percent();

    if ( affected_by.frost_mage )
      base_multiplier *= 1.0 + p()->spec.frost_mage->effectN( 1 ).percent();

    if ( harmful && affected_by.shatter )
    {
      snapshot_flags |= STATE_FROZEN | STATE_FROZEN_MUL;
      update_flags   |= STATE_FROZEN | STATE_FROZEN_MUL;
    }
  }

  void init_finished() override
  {
    if ( track_cd_waste && sim->report_details != 0 )
      cd_waste = p()->get_cooldown_waste_data( cooldown );

    spell_t::init_finished();
  }

  int n_targets() const override
  {
    if ( affected_by.deathborne_cleave )
    {
      assert( spell_t::n_targets() == 0 );
      return p()->buffs.deathborne->check() ? ( 1 + as<int>( p()->buffs.deathborne->data().effectN( 4 ).base_value() ) ) : 0;
    }
    else
    {
      return spell_t::n_targets();
    }
  }

  double action_multiplier() const override
  {
    double m = spell_t::action_multiplier();

    if ( affected_by.arcane_power )
      m *= 1.0 + p()->buffs.arcane_power->check_value();

    if ( affected_by.bone_chilling )
      m *= 1.0 + p()->buffs.bone_chilling->check_stack_value();

    if ( affected_by.incanters_flow )
      m *= 1.0 + p()->buffs.incanters_flow->check_stack_value();

    if ( affected_by.rune_of_power )
      m *= 1.0 + p()->buffs.rune_of_power->check_value();

    if ( affected_by.deathborne )
      m *= 1.0 + p()->buffs.deathborne->check_value();

    if ( affected_by.siphoned_malice )
      m *= 1.0 + p()->buffs.siphoned_malice->check_stack_value();

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = spell_t::composite_da_multiplier( s );

    if ( affected_by.savant )
      m *= 1.0 + p()->cache.mastery() * p()->spec.savant->effectN( 5 ).mastery_value();

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = spell_t::composite_target_multiplier( target );

    if ( auto td = p()->target_data[ target ] )
    {
      // TODO: Confirm how Radiant Spark is supposed to interact with Ignite.
      // Right now in beta, the damage multiplier gets factored out when triggering Ignite.
      if ( affected_by.radiant_spark )
        m *= 1.0 + td->debuffs.radiant_spark_vulnerability->check_stack_value();
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = spell_t::composite_crit_chance();

    if ( affected_by.combustion )
      c += p()->buffs.combustion->check_value();

    return c;
  }

  virtual unsigned frozen( const action_state_t* s ) const
  {
    const mage_td_t* td = p()->target_data[ s->target ];

    if ( !td )
      return 0u;

    unsigned source = 0u;

    if ( td->debuffs.winters_chill->check() )
      source |= FF_WINTERS_CHILL;

    if ( td->debuffs.frozen->check() )
      source |= FF_ROOT;

    return source;
  }

  virtual double frozen_multiplier( const action_state_t* ) const
  { return 1.0; }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    spell_t::snapshot_internal( s, flags, rt );

    if ( flags & STATE_FROZEN )
      cast_state( s )->frozen = frozen( s );

    if ( flags & STATE_FROZEN_MUL )
      cast_state( s )->frozen_multiplier = frozen_multiplier( s );
  }

  double cost() const override
  {
    double c = spell_t::cost();

    if ( p()->buffs.arcane_power->check() )
    {
      c *= 1.0 + ( p()->spec.arcane_power_2->ok() ? p()->buffs.arcane_power->data().effectN( 2 ).percent() : 0.0 )
               + p()->talents.overpowered->effectN( 2 ).percent();
    }

    return c;
  }

  void update_ready( timespan_t cd ) override
  {
    if ( cd_waste )
      cd_waste->add( cd, time_to_execute );

    spell_t::update_ready( cd );
  }

  bool usable_moving() const override
  {
    if ( p()->buffs.ice_floes->check() && affected_by.ice_floes )
      return true;

    return spell_t::usable_moving();
  }

  virtual void consume_cost_reductions()
  { }

  void execute() override
  {
    spell_t::execute();

    // Make sure we remove all cost reduction buffs before we trigger new ones.
    // This will prevent for example Arcane Blast consuming its own Clearcasting proc.
    consume_cost_reductions();

    if ( p()->spec.clearcasting->ok() && harmful && current_resource() == RESOURCE_MANA )
    {
      // Mana spending required for 1% chance.
      double mana_step = p()->spec.clearcasting->cost( POWER_MANA ) * p()->resources.base[ RESOURCE_MANA ];
      mana_step /= p()->spec.clearcasting->effectN( 1 ).percent();

      double proc_chance = 0.01 * last_resource_cost / mana_step;
      proc_chance *= 1.0 + p()->azerite.arcane_pummeling.spell_ref().effectN( 2 ).percent();
      p()->trigger_delayed_buff( p()->buffs.clearcasting, proc_chance );
    }

    if ( !background && affected_by.ice_floes && time_to_execute > 0_ms )
      p()->buffs.ice_floes->decrement();

    // TODO: Verify how this effect interacts with spells that have multiple
    // schools and how it interacts with spells that are not Mage spells.
    if ( !background && p()->runeforge.disciplinary_command.ok() )
    {
      if ( dbc::is_school( get_school(), SCHOOL_ARCANE ) )
        p()->buffs.disciplinary_command_arcane->trigger();
      if ( dbc::is_school( get_school(), SCHOOL_FROST ) )
        p()->buffs.disciplinary_command_frost->trigger();
      if ( dbc::is_school( get_school(), SCHOOL_FIRE ) )
        p()->buffs.disciplinary_command_fire->trigger();
      if ( p()->buffs.disciplinary_command_arcane->check() && p()->buffs.disciplinary_command_frost->check() && p()->buffs.disciplinary_command_fire->check() )
      {
        p()->buffs.disciplinary_command->trigger();
        p()->buffs.disciplinary_command_arcane->expire();
        p()->buffs.disciplinary_command_frost->expire();
        p()->buffs.disciplinary_command_fire->expire();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( s->result_total <= 0.0 )
      return;

    if ( triggers.fevered_incantation )
    {
      if ( s->result == RESULT_CRIT )
        p()->buffs.fevered_incantation->trigger();
      else
        p()->buffs.fevered_incantation->expire();
    }

    if ( triggers.icy_propulsion && s->result == RESULT_CRIT && p()->buffs.icy_veins->check() )
      p()->cooldowns.icy_veins->adjust( -0.1 * p()->conduits.icy_propulsion.time_value( conduit_data_t::S ) );
  }

  void assess_damage( result_amount_type rt, action_state_t* s ) override
  {
    spell_t::assess_damage( rt, s );

    if ( s->result_total <= 0.0 )
      return;

    if ( auto td = p()->target_data[ s->target ] )
    {
      auto spark_dot = td->dots.radiant_spark;
      if ( triggers.radiant_spark && spark_dot->is_ticking() )
      {
        auto spark_debuff = td->debuffs.radiant_spark_vulnerability;
        if ( spark_debuff->at_max_stacks() )
        {
          spark_debuff->expire();
          // Prevent new applications of the vulnerability debuff until the DoT finishes ticking.
          spark_debuff->cooldown->start( spark_dot->remains() );
        }
        else
        {
          spark_debuff->trigger();
        }
      }

      auto totm = debug_cast<buffs::touch_of_the_magi_t*>( td->debuffs.touch_of_the_magi );
      if ( totm->check() )
      {
        totm->accumulate_damage( s );

        if ( p()->talents.arcane_echo->ok() && s->action != p()->action.arcane_echo )
        {
          make_event( *sim, 0_ms, [ this, t = s->target ]
          {
            p()->action.arcane_echo->set_target( t );
            p()->action.arcane_echo->execute();
          } );
        }
      }
    }
  }

  void last_tick( dot_t* d ) override
  {
    spell_t::last_tick( d );

    if ( channeled && affected_by.ice_floes )
      p()->buffs.ice_floes->decrement();
  }

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( current_resource() == RESOURCE_MANA )
      p()->trigger_lucid_dreams( target, last_resource_cost );
  }

  void trigger_legendary_buff( buff_t* counter, buff_t* ready, int offset = 2 )
  {
    if ( ready->check() )
      return;

    if ( counter->at_max_stacks( offset ) )
    {
      counter->expire();
      ready->trigger();
    }
    else
    {
      counter->trigger();
    }
  }
};

using residual_action_t = residual_action::residual_periodic_action_t<mage_spell_t>;

double mage_spell_state_t::composite_crit_chance() const
{
  double c = action_state_t::composite_crit_chance();

  if ( frozen )
  {
    auto a = debug_cast<const mage_spell_t*>( action );
    auto p = a->p();

    if ( a->affected_by.shatter && p->spec.shatter->ok() )
    {
      // Multiplier is not in spell data, apparently.
      c *= 1.5;
      c += p->spec.shatter->effectN( 2 ).percent() + p->spec.shatter_2->effectN( 1 ).percent();
    }
  }

  return c;
}


// ==========================================================================
// Arcane Mage Spell
// ==========================================================================

struct arcane_mage_spell_t : public mage_spell_t
{
  std::vector<buff_t*> cost_reductions;

  arcane_mage_spell_t( util::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    cost_reductions()
  { }

  void consume_cost_reductions() override
  {
    // Consume first applicable buff and then stop.
    for ( auto cr : cost_reductions )
    {
      if ( cr->check() )
      {
        cr->decrement();
        break;
      }
    }
  }

  double cost() const override
  {
    double c = mage_spell_t::cost();

    for ( auto cr : cost_reductions )
      c *= 1.0 + cr->check_value();

    return std::max( c, 0.0 );
  }

  double arcane_charge_multiplier( bool arcane_barrage = false ) const
  {
    double per_ac_bonus = 0.0;

    if ( arcane_barrage )
    {
      per_ac_bonus = p()->spec.arcane_charge->effectN( 2 ).percent()
                   + p()->cache.mastery() * p()->spec.savant->effectN( 3 ).mastery_value();
    }
    else
    {
      per_ac_bonus = p()->spec.arcane_charge->effectN( 1 ).percent()
                   + p()->cache.mastery() * p()->spec.savant->effectN( 2 ).mastery_value();
    }

    return 1.0 + p()->buffs.arcane_charge->check() * per_ac_bonus;
  }
};


// ==========================================================================
// Fire Mage Spell
// ==========================================================================

struct fire_mage_spell_t : public mage_spell_t
{
  fire_mage_spell_t( util::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s )
  { }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( triggers.ignite )
        trigger_ignite( s );

      if ( tt_applicable( s, triggers.hot_streak ) )
        handle_hot_streak( s );

      if ( tt_applicable( s, triggers.kindling )
        && s->result == RESULT_CRIT
        && p()->talents.kindling->ok() )
      {
        p()->cooldowns.combustion->adjust( -p()->talents.kindling->effectN( 1 ).time_value() );
      }

      if ( triggers.from_the_ashes
        && s->result == RESULT_CRIT
        && p()->talents.from_the_ashes->ok()
        && p()->cooldowns.from_the_ashes->up() )
      {
        p()->cooldowns.from_the_ashes->start( p()->talents.from_the_ashes->internal_cooldown() );
        p()->cooldowns.phoenix_flames->adjust( p()->talents.from_the_ashes->effectN( 2 ).time_value() );
      }
    }
  }

  void handle_hot_streak( action_state_t* s )
  {
    mage_t* p = this->p();
    if ( !p->spec.hot_streak->ok() )
      return;

    bool guaranteed = s->composite_crit_chance() >= 1.0;
    p->procs.hot_streak_spell->occur();

    if ( s->result == RESULT_CRIT )
    {
      p->procs.hot_streak_spell_crit->occur();

      // Crit with HS => wasted crit
      if ( p->buffs.hot_streak->check() )
      {
        p->procs.hot_streak_spell_crit_wasted->occur();
        if ( guaranteed )
          p->buffs.hot_streak->predict();
      }
      else
      {
        // Crit with HU => convert to HS
        if ( p->buffs.heating_up->up() )
        {
          p->procs.hot_streak->occur();
          // Check if HS was triggered by IB
          if ( id == 108853 )
            p->procs.heating_up_ib_converted->occur();

          bool hu_react = p->buffs.heating_up->stack_react() > 0;
          p->buffs.heating_up->expire();
          p->buffs.hot_streak->trigger();
          if ( guaranteed && hu_react )
            p->buffs.hot_streak->predict();

          // If Scorch generates Hot Streak and the actor is currently casting Pyroblast,
          // the game will immediately finish the cast. This is presumably done to work
          // around the buff application delay inside Combustion or with Searing Touch
          // active. The following code is a huge hack.
          if ( id == 2948 && p->executing && p->executing->id == 11366 )
          {
            assert( p->executing->execute_event );
            event_t::cancel( p->executing->execute_event );
            event_t::cancel( p->cast_while_casting_poll_event );
            // We need to set time_to_execute to zero, start a new action execute event and
            // adjust GCD. action_t::schedule_execute should handle all these.
            p->executing->schedule_execute();
          }
        }
        // Crit without HU => generate HU
        else
        {
          p->procs.heating_up_generated->occur();
          p->buffs.heating_up->trigger( p->buffs.heating_up->buff_duration() * p->cache.spell_speed() );
          if ( guaranteed )
            p->buffs.heating_up->predict();
        }
      }
    }
    else // Non-crit
    {
      // Non-crit with HU => remove HU
      if ( p->buffs.heating_up->check() )
      {
        if ( p->buffs.heating_up->elapsed( sim->current_time() ) > 0.2_s )
        {
          p->procs.heating_up_removed->occur();
          p->buffs.heating_up->expire();
          sim->print_debug( "Heating Up removed by non-crit" );
        }
        else
        {
          sim->print_debug( "Heating Up removal ignored due to 200 ms protection" );
        }
      }
    }
  }

  virtual double composite_ignite_multiplier( const action_state_t* ) const
  { return 1.0; }

  void trigger_ignite( action_state_t* s )
  {
    if ( !p()->spec.ignite->ok() )
      return;

    double m = s->target_da_multiplier;
    if ( m <= 0.0 )
      return;

    double amount = s->result_total / m * p()->cache.mastery_value();
    if ( amount <= 0.0 )
      return;

    amount *= composite_ignite_multiplier( s );

    if ( !p()->action.ignite->get_dot( s->target )->is_ticking() )
      p()->procs.ignite_applied->occur();

    residual_action::trigger( p()->action.ignite, s->target, amount );

    if ( s->chain_target > 0 )
      return;

    auto& bm = p()->benefits.blaster_master;
    if ( bm.combustion && p()->buffs.combustion->check() )
      bm.combustion->update();
    if ( bm.rune_of_power && p()->buffs.rune_of_power->check() )
      bm.rune_of_power->update();
    if ( bm.searing_touch && s->target->health_percentage() < p()->talents.searing_touch->effectN( 1 ).base_value() )
      bm.searing_touch->update();
  }

  bool firestarter_active( player_t* target ) const
  {
    if ( !p()->talents.firestarter->ok() )
      return false;

    // Check for user-specified override.
    if ( p()->options.firestarter_time > 0_ms )
      return sim->current_time() < p()->options.firestarter_time;
    else
      return target->health_percentage() > p()->talents.firestarter->effectN( 1 ).base_value();
  }

  void trigger_molten_skyfall()
  {
    trigger_legendary_buff( p()->buffs.molten_skyfall, p()->buffs.molten_skyfall_ready );
  }

  void consume_molten_skyfall( player_t* target )
  {
    if ( p()->buffs.molten_skyfall_ready->check() )
    {
      p()->buffs.molten_skyfall_ready->expire();
      p()->action.legendary_meteor->set_target( target );
      p()->action.legendary_meteor->execute();
    }
  }
};

struct hot_streak_state_t : public mage_spell_state_t
{
  bool hot_streak;

  hot_streak_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    hot_streak()
  { }

  void initialize() override
  {
    mage_spell_state_t::initialize();
    hot_streak = false;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s ) << " hot_streak=" << hot_streak;
    return s;
  }

  void copy_state( const action_state_t* s ) override
  {
    mage_spell_state_t::copy_state( s );
    hot_streak = debug_cast<const hot_streak_state_t*>( s )->hot_streak;
  }
};

struct hot_streak_spell_t : public fire_mage_spell_t
{
  // Last available Hot Streak state.
  bool last_hot_streak;

  hot_streak_spell_t( util::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    fire_mage_spell_t( n, p, s ),
    last_hot_streak()
  { }

  action_state_t* new_state() override
  { return new hot_streak_state_t( this, target ); }

  void reset() override
  {
    fire_mage_spell_t::reset();
    last_hot_streak = false;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.hot_streak->check() || p()->buffs.firestorm->check() )
      return 0_ms;

    return fire_mage_spell_t::execute_time();
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    fire_mage_spell_t::snapshot_state( s, rt );
    debug_cast<hot_streak_state_t*>( s )->hot_streak = last_hot_streak;
  }

  double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    c += p()->buffs.firestorm->check_value();

    return c;
  }

  double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    return debug_cast<const hot_streak_state_t*>( s )->hot_streak ? 2.0 : 1.0;
  }

  void execute() override
  {
    last_hot_streak = p()->buffs.hot_streak->up() && time_to_execute == 0_ms;
    fire_mage_spell_t::execute();

    if ( last_hot_streak )
    {
      p()->buffs.hot_streak->decrement();
      p()->buffs.pyroclasm->trigger();
      p()->buffs.firemind->trigger();

      trigger_legendary_buff( p()->buffs.sun_kings_blessing, p()->buffs.sun_kings_blessing_ready, 0 );

      if ( rng().roll( p()->talents.pyromaniac->effectN( 1 ).percent() ) )
      {
        p()->procs.hot_streak->occur();
        p()->procs.hot_streak_pyromaniac->occur();
        p()->buffs.hot_streak->trigger();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    // The buff expiration is slightly delayed, allowing two spells cast at the same time to benefit from this effect.
    p()->buffs.alexstraszas_fury->expire( p()->bugs ? 30_ms : 0_ms );
  }

  double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.alexstraszas_fury->check_value();

    return am;
  }
};


// ==========================================================================
// Frost Mage Spell
// ==========================================================================

// Some Frost spells snapshot on impact (rather than execute). This is handled via
// the calculate_on_impact flag.
//
// When set to true:
//   * All snapshot flags are moved from snapshot_flags to impact_flags.
//   * calculate_result and calculate_direct_amount don't do any calculations.
//   * On spell impact:
//     - State is snapshot via frost_mage_spell_t::snapshot_impact_state.
//     - Result is calculated via frost_mage_spell_t::calculate_impact_result.
//     - Amount is calculated via frost_mage_spell_t::calculate_impact_direct_amount.
//
// The previous functions are virtual and can be overridden when needed.
struct frost_mage_spell_t : public mage_spell_t
{
  bool calculate_on_impact;
  bool consumes_winters_chill;

  proc_t* proc_brain_freeze;
  proc_t* proc_fof;
  proc_t* proc_winters_chill_consumed;

  bool track_shatter;
  shatter_source_t* shatter_source;

  unsigned impact_flags;

  frost_mage_spell_t( util::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    calculate_on_impact(),
    consumes_winters_chill(),
    proc_brain_freeze(),
    proc_fof(),
    proc_winters_chill_consumed(),
    track_shatter(),
    shatter_source(),
    impact_flags()
  {
    affected_by.shatter = true;
  }

  void init() override
  {
    if ( initialized )
      return;

    mage_spell_t::init();

    if ( calculate_on_impact )
      std::swap( snapshot_flags, impact_flags );
  }

  void init_finished() override
  {
    if ( consumes_winters_chill )
      proc_winters_chill_consumed = p()->get_proc( "Winter's Chill stacks consumed by " + std::string( data().name_cstr() ) );

    if ( track_shatter && sim->report_details != 0 )
      shatter_source = p()->get_shatter_source( name_str );

    mage_spell_t::init_finished();
  }

  double icicle_sp_coefficient() const
  {
    return p()->cache.mastery() * p()->spec.icicles->effectN( 3 ).sp_coeff();
  }

  virtual void snapshot_impact_state( action_state_t* s, result_amount_type rt )
  { snapshot_internal( s, impact_flags, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    if ( !calculate_on_impact )
    {
      return mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      // Don't do any extra work, this result won't be used.
      return 0.0;
    }
  }

  virtual double calculate_impact_direct_amount( action_state_t* s ) const
  { return mage_spell_t::calculate_direct_amount( s ); }

  result_e calculate_result( action_state_t* s ) const override
  {
    if ( !calculate_on_impact )
    {
      return mage_spell_t::calculate_result( s );
    }
    else
    {
      // Don't do any extra work, this result won't be used.
      return RESULT_NONE;
    }
  }

  virtual result_e calculate_impact_result( action_state_t* s ) const
  { return mage_spell_t::calculate_result( s ); }

  void record_shatter_source( const action_state_t* s, shatter_source_t* source )
  {
    if ( !source )
      return;

    unsigned frozen = cast_state( s )->frozen;

    if ( frozen & FF_WINTERS_CHILL )
      source->occur( FROZEN_WINTERS_CHILL );
    else if ( frozen & ~FF_FINGERS_OF_FROST )
      source->occur( FROZEN_ROOT );
    else if ( frozen & FF_FINGERS_OF_FROST )
      source->occur( FROZEN_FINGERS_OF_FROST );
    else
      source->occur( FROZEN_NONE );
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( consumes_winters_chill )
      p()->remaining_winters_chill = std::max( p()->remaining_winters_chill - 1, 0 );
  }

  void impact( action_state_t* s ) override
  {
    if ( calculate_on_impact )
    {
      // Spells that calculate damage on impact need to snapshot relevant values
      // right before impact and then recalculate the result and total damage.
      snapshot_impact_state( s, amount_type( s ) );
      s->result = calculate_impact_result( s );
      s->result_amount = calculate_impact_direct_amount( s );
    }

    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( s->chain_target == 0 )
        record_shatter_source( s, shatter_source );

      if ( triggers.bone_chilling )
        p()->buffs.bone_chilling->trigger();

      if ( auto td = p()->target_data[ s->target ] )
      {
        if ( consumes_winters_chill && td->debuffs.winters_chill->check() )
        {
          td->debuffs.winters_chill->decrement();
          proc_winters_chill_consumed->occur();
          p()->procs.winters_chill_consumed->occur();
        }
      }
    }
  }

  void trigger_cold_front()
  {
    trigger_legendary_buff( p()->buffs.cold_front, p()->buffs.cold_front_ready );
  }

  void consume_cold_front( player_t* target )
  {
    if ( p()->buffs.cold_front_ready->check() )
    {
      p()->buffs.cold_front_ready->expire();
      p()->action.legendary_frozen_orb->set_target( target );
      p()->action.legendary_frozen_orb->execute();
    }
  }
};

// Icicles ==================================================================

struct icicle_t : public frost_mage_spell_t
{
  icicle_t( util::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 148022 ) )
  {
    background = true;
    callbacks = false;
    base_dd_min = base_dd_max = 1.0;
    base_dd_adder += p->azerite.flash_freeze.value( 2 );

    if ( p->talents.splitting_ice->ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_multiplier *= 1.0 + p->talents.splitting_ice->effectN( 3 ).percent();
      base_aoe_multiplier *= p->talents.splitting_ice->effectN( 2 ).percent();
    }
  }

  void init_finished() override
  {
    proc_fof = p()->get_proc( "Fingers of Frost from Flash Freeze" );
    frost_mage_spell_t::init_finished();
  }

  void execute() override
  {
    frost_mage_spell_t::execute();
    p()->buffs.icicles->decrement();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      p()->trigger_fof( p()->azerite.flash_freeze.spell_ref().effectN( 1 ).percent(), 1, proc_fof );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  { return frost_mage_spell_t::spell_direct_power_coefficient( s ) + icicle_sp_coefficient(); }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public arcane_mage_spell_t
{
  presence_of_mind_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Presence of Mind" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  bool ready() override
  {
    if ( p()->buffs.presence_of_mind->check() )
      return false;

    return arcane_mage_spell_t::ready();
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();
    p()->buffs.presence_of_mind->trigger( p()->buffs.presence_of_mind->max_stack() );
  }
};

// Ignite Spell =============================================================

struct ignite_t : public residual_action_t
{
  ignite_t( util::string_view n, mage_t* p ) :
    residual_action_t( n, p, p->find_spell( 12654 ) )
  {
    callbacks = true;
    affected_by.radiant_spark = false;
  }

  void init() override
  {
    residual_action_t::init();

    snapshot_flags |= STATE_TGT_MUL_TA;
    update_flags   |= STATE_TGT_MUL_TA;
  }

  void tick( dot_t* d ) override
  {
    residual_action_t::tick( d );

    if ( rng().roll( p()->talents.conflagration->effectN( 1 ).percent() ) )
    {
      p()->action.conflagration_flare_up->set_target( d->target );
      p()->action.conflagration_flare_up->execute();
    }
  }
};

// Arcane Barrage Spell =====================================================

struct arcane_barrage_t : public arcane_mage_spell_t
{
  int artifice_of_the_archmage_charges;

  arcane_barrage_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Barrage" ) ),
    artifice_of_the_archmage_charges()
  {
    parse_options( options_str );
    cooldown->hasted = true;
    base_aoe_multiplier *= data().effectN( 2 ).percent();
    triggers.radiant_spark = true;
    artifice_of_the_archmage_charges = as<int>( p->find_spell( 337244 )->effectN( 1 ).base_value() );
  }

  int n_targets() const override
  {
    int charges = p()->buffs.arcane_charge->check();
    return p()->spec.arcane_barrage_2->ok() && charges > 0 ? charges + 1 : 0;
  }

  void execute() override
  {
    p()->benefits.arcane_charge.arcane_barrage->update();

    arcane_mage_spell_t::execute();

    // Arcane Barrage restores 1% mana per charge in game and states that 2% is restored
    // in the tooltip. The data has a value of 1.5, so this is likely a rounding issue.
    p()->resource_gain( RESOURCE_MANA,
      p()->buffs.arcane_charge->check() * p()->resources.max[ RESOURCE_MANA ] * floor( p()->spec.arcane_barrage_3->effectN( 1 ).base_value() ) * 0.01,
      p()->gains.arcane_barrage, this );
    p()->buffs.arcane_charge->expire();
    p()->buffs.arcane_harmony->expire();
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.chrono_shift->trigger();
      // Multiply by 0.1 because for this data a value of 100 means 10%.
      if ( rng().roll( 0.1 * p()->conduits.artifice_of_the_archmage.percent() ) )
        p()->buffs.arcane_charge->trigger( artifice_of_the_archmage_charges );
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = arcane_mage_spell_t::bonus_da( s );

    if ( s->target->health_percentage() < p()->azerite.arcane_pressure.spell_ref().effectN( 2 ).base_value() )
      da += p()->azerite.arcane_pressure.value() * p()->buffs.arcane_charge->check() / arcane_charge_multiplier( true );

    return da;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = arcane_mage_spell_t::composite_da_multiplier( s );

    m *= 1.0 + s->n_targets * p()->talents.resonance->effectN( 1 ).percent();

    if ( s->target->health_percentage() < p()->runeforge.arcane_bombardment->effectN( 1 ).base_value() )
      m *= 1.0 + p()->runeforge.arcane_bombardment->effectN( 2 ).percent();

    return m;
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_multiplier( true );
    am *= 1.0 + p()->buffs.arcane_harmony->check_stack_value();

    return am;
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public arcane_mage_spell_t
{
  double equipoise_threshold;
  double equipoise_reduction;

  arcane_blast_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Blast" ) ),
    equipoise_threshold(),
    equipoise_reduction()
  {
    parse_options( options_str );
    cost_reductions = { p->buffs.rule_of_threes };
    base_dd_adder += p->azerite.galvanizing_spark.value( 2 );
    reduced_aoe_damage = triggers.radiant_spark = true;
    affected_by.deathborne_cleave = true;

    if ( p->azerite.equipoise.enabled() )
    {
      // Equipoise data is stored across 4 different spells.
      equipoise_threshold = p->find_spell( 264351 )->effectN( 1 ).percent();
      equipoise_reduction = p->find_spell( 264353 )->effectN( 1 ).average( p );
    }
  }

  double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    // TODO: It looks like the flat cost reduction is applied after Arcane Power et al,
    // but before Arcane Charge. This might not be intended, double check later.
    if ( p()->resources.pct( RESOURCE_MANA ) <= equipoise_threshold )
      c += equipoise_reduction;

    c *= 1.0 + p()->buffs.arcane_charge->check()
             * p()->spec.arcane_charge->effectN( 5 ).percent();

    return std::max( c, 0.0 );
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = arcane_mage_spell_t::bonus_da( s );

    if ( p()->resources.pct( RESOURCE_MANA ) > equipoise_threshold )
      da += p()->azerite.equipoise.value();

    return da;
  }

  void execute() override
  {
    p()->benefits.arcane_charge.arcane_blast->update();

    arcane_mage_spell_t::execute();

    p()->buffs.arcane_charge->trigger();
    if ( rng().roll( p()->azerite.galvanizing_spark.spell_ref().effectN( 1 ).percent() ) )
      p()->buffs.arcane_charge->trigger();

    if ( p()->buffs.presence_of_mind->up() )
      p()->buffs.presence_of_mind->decrement();

    // There is a delay where it is possible to cast a spell that would
    // consume Expanded Potential immediately after Expanded Potential
    // is triggered, which will prevent it from being consumed.
    p()->trigger_delayed_buff( p()->buffs.expanded_potential );
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    // With Deathborne, multiple stacks of Nether Precision are removed
    // by a single Arcane blast, but all of them will benefit from the
    // bonus damage.
    // Delay the decrement call because if you cast Arcane Missiles to consume
    // Clearcasting immediately after Arcane Blast, a stack of Nether Precision
    // will be consumed by Arcane Blast will not benefit from the damage bonus.
    // Check if this is still the case closer to Shadowlands release.
    if ( p()->conduits.nether_precision.ok() )
      make_event( sim, 15_ms, [ this ] { p()->buffs.nether_precision->decrement(); } );
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_multiplier();
    am *= 1.0 + p()->buffs.nether_precision->check_value();

    return am;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.presence_of_mind->check() )
      return 0_ms;

    timespan_t t = arcane_mage_spell_t::execute_time();

    t *= 1.0 + p()->buffs.arcane_charge->check()
             * p()->spec.arcane_charge->effectN( 4 ).percent();

    return t;
  }
};

// Arcane Explosion Spell ===================================================

struct arcane_explosion_t : public arcane_mage_spell_t
{
  arcane_explosion_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_class_spell( "Arcane Explosion" ) )
  {
    parse_options( options_str );
    aoe = -1;
    cost_reductions = { p->buffs.clearcasting };
    affected_by.savant = triggers.radiant_spark = true;
    base_dd_adder += p->azerite.explosive_echo.value( 2 );
    base_multiplier *= 1.0 + p->spec.arcane_explosion_2->effectN( 1 ).percent();
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( p()->specialization() == MAGE_ARCANE )
    {
      if ( !target_list().empty() )
        p()->buffs.arcane_charge->trigger();

      if ( num_targets_hit >= as<int>( p()->talents.reverberate->effectN( 2 ).base_value() )
        && rng().roll( p()->talents.reverberate->effectN( 1 ).percent() ) )
      {
        p()->buffs.arcane_charge->trigger();
      }
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = arcane_mage_spell_t::bonus_da( s );

    if ( target_list().size() >= as<size_t>( p()->azerite.explosive_echo.spell_ref().effectN( 1 ).base_value() )
      && rng().roll( p()->azerite.explosive_echo.spell_ref().effectN( 3 ).percent() ) )
    {
      da += p()->azerite.explosive_echo.value( 4 );
    }

    return da;
  }
};

// Arcane Familiar Spell ====================================================

struct arcane_assault_t : public arcane_mage_spell_t
{
  arcane_assault_t( util::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 225119 ) )
  {
    background = true;
    affected_by.radiant_spark = false;
  }
};

struct arcane_familiar_t : public arcane_mage_spell_t
{
  arcane_familiar_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.arcane_familiar )
  {
    parse_options( options_str );
    harmful = track_cd_waste = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();
    p()->buffs.arcane_familiar->trigger();
  }

  bool ready() override
  {
    if ( p()->buffs.arcane_familiar->check() )
      return false;

    return arcane_mage_spell_t::ready();
  }
};

// Arcane Intellect Spell ===================================================

struct arcane_intellect_t : public mage_spell_t
{
  arcane_intellect_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Arcane Intellect" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    background = sim->overrides.arcane_intellect != 0;
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( !sim->overrides.arcane_intellect )
      sim->auras.arcane_intellect->trigger();
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t : public arcane_mage_spell_t
{
  arcane_missiles_tick_t( util::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 7268 ) )
  {
    background = true;
    affected_by.savant = triggers.radiant_spark = true;
    base_multiplier *= 1.0 + p->runeforge.arcane_harmony->effectN( 1 ).percent();
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( p()->buffs.clearcasting_channel->check() )
      p()->buffs.arcane_pummeling->trigger();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = arcane_mage_spell_t::bonus_da( s );

    da += p()->buffs.arcane_pummeling->check_stack_value();

    return da;
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.arcane_harmony->trigger();
      // Multiply by 100 because for this data a value of 1 represents 0.1 seconds.
      p()->cooldowns.arcane_power->adjust( -100 * p()->conduits.arcane_prodigy.time_value(), false );
    }
  }
};

struct am_state_t : public mage_spell_state_t
{
  double tick_time_multiplier;

  am_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    tick_time_multiplier( 1.0 )
  { }

  void initialize() override
  {
    mage_spell_state_t::initialize();
    tick_time_multiplier = 1.0;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s ) << " tick_time_multiplier=" << tick_time_multiplier;
    return s;
  }

  void copy_state( const action_state_t* s ) override
  {
    mage_spell_state_t::copy_state( s );
    tick_time_multiplier = debug_cast<const am_state_t*>( s )->tick_time_multiplier;
  }
};

struct arcane_missiles_t : public arcane_mage_spell_t
{
  double cc_duration_reduction;
  double cc_tick_time_reduction;

  arcane_missiles_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Missiles" ) )
  {
    parse_options( options_str );
    may_miss = false;
    tick_zero = channeled = true;
    tick_action = get_action<arcane_missiles_tick_t>( "arcane_missiles_tick", p );
    cost_reductions = { p->buffs.clearcasting, p->buffs.rule_of_threes };

    auto cc_data = p->buffs.clearcasting_channel->data();
    cc_duration_reduction  = cc_data.effectN( 1 ).percent();
    cc_tick_time_reduction = cc_data.effectN( 2 ).percent() + p->talents.amplification->effectN( 1 ).percent() + p->spec.clearcasting_2->effectN( 1 ).percent();
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }

  action_state_t* new_state() override
  { return new am_state_t( this, target ); }

  // We need to snapshot any tick time reduction effect here so that it correctly affects the whole
  // channel.
  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    arcane_mage_spell_t::snapshot_state( s, rt );

    if ( p()->buffs.clearcasting_channel->check() )
      debug_cast<am_state_t*>( s )->tick_time_multiplier = 1.0 + cc_tick_time_reduction;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // AM channel duration is a bit fuzzy, it will go above or below the standard 2 s
    // to make sure it has the correct number of ticks.
    timespan_t full_duration = dot_duration * s->haste;

    if ( p()->buffs.clearcasting_channel->check() )
      full_duration *= 1.0 + cc_duration_reduction;

    timespan_t tick_duration = tick_time( s );
    double ticks = std::round( full_duration / tick_duration );
    return ticks * tick_duration;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = arcane_mage_spell_t::tick_time( s );

    t *= debug_cast<const am_state_t*>( s )->tick_time_multiplier;

    return t;
  }

  void execute() override
  {
    p()->buffs.arcane_pummeling->expire();

    if ( p()->buffs.clearcasting->check() )
      p()->buffs.clearcasting_channel->trigger();
    else
      p()->buffs.clearcasting_channel->expire();

    arcane_mage_spell_t::execute();
  }

  bool usable_moving() const override
  {
    if ( p()->talents.slipstream->ok() && p()->buffs.clearcasting->check() )
      return true;

    return arcane_mage_spell_t::usable_moving();
  }

  void last_tick( dot_t* d ) override
  {
    arcane_mage_spell_t::last_tick( d );
    p()->buffs.clearcasting_channel->expire();
  }
};

// Arcane Orb Spell =========================================================

struct arcane_orb_bolt_t : public arcane_mage_spell_t
{
  arcane_orb_bolt_t( util::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 153640 ) )
  {
    background = true;
    affected_by.savant = triggers.radiant_spark = true;
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );
    p()->buffs.arcane_charge->trigger();
  }
};

struct arcane_orb_t : public arcane_mage_spell_t
{
  arcane_orb_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.arcane_orb )
  {
    parse_options( options_str );
    may_miss = may_crit = false;
    aoe = -1;

    impact_action = get_action<arcane_orb_bolt_t>( "arcane_orb_bolt", p );
    add_child( impact_action );
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();
    p()->buffs.arcane_charge->trigger();
  }
};

// Arcane Power Spell =======================================================

struct arcane_power_t : public arcane_mage_spell_t
{
  arcane_power_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Power" ) )
  {
    parse_options( options_str );
    harmful = false;
    cooldown->duration *= p->strive_for_perfection_multiplier;
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    p()->buffs.arcane_power->trigger();

    p()->buffs.rune_of_power->trigger();
    p()->distance_from_rune = 0.0;
  }
};

// Blast Wave Spell =========================================================

struct blast_wave_t : public fire_mage_spell_t
{
  blast_wave_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.blast_wave )
  {
    parse_options( options_str );
    aoe = -1;
    triggers.radiant_spark = true;
  }
};

// Blink Spell ==============================================================

struct blink_t : public mage_spell_t
{
  blink_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Blink" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    base_teleport_distance = data().effectN( 1 ).radius_max();
    movement_directionality = movement_direction_type::OMNI;
    cooldown->duration += p->conduits.flow_of_time.time_value();

    background = p->talents.shimmer->ok();
  }
};

// Blizzard Spell ===========================================================

struct blizzard_shard_t : public frost_mage_spell_t
{
  blizzard_shard_t( util::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 190357 ) )
  {
    aoe = -1;
    background = ground_aoe = triggers.bone_chilling = true;
    base_multiplier *= 1.0 + p->spec.blizzard_3->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->conduits.shivering_core.percent();
    triggers.icy_propulsion = false;
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    if ( hit_any_target )
    {
      timespan_t reduction = -10 * num_targets_hit * p()->spec.blizzard_2->effectN( 1 ).time_value();
      p()->cooldowns.frozen_orb->adjust( reduction, false );
    }
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.freezing_rain->check_value();

    return am;
  }
};

struct blizzard_t : public frost_mage_spell_t
{
  action_t* blizzard_shard;

  blizzard_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Blizzard" ) ),
    blizzard_shard( get_action<blizzard_shard_t>( "blizzard_shard", p ) )
  {
    parse_options( options_str );
    add_child( blizzard_shard );
    cooldown->hasted = true;
    may_miss = may_crit = affected_by.shatter = false;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.freezing_rain->check() )
      return 0_ms;

    return frost_mage_spell_t::execute_time();
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    timespan_t ground_aoe_duration = data().duration() * player->cache.spell_speed();
    p()->ground_aoe_expiration[ AOE_BLIZZARD ] = sim->current_time() + ground_aoe_duration;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( target )
      .duration( ground_aoe_duration )
      .action( blizzard_shard )
      .hasted( ground_aoe_params_t::SPELL_SPEED ), true );
  }
};

// Cold Snap Spell ==========================================================

struct cold_snap_t : public frost_mage_spell_t
{
  cold_snap_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Cold Snap" ) )
  {
    parse_options( options_str );
    harmful = false;
    cooldown->duration += p->spec.cold_snap_2->effectN( 1 ).time_value();
  };

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->cooldowns.cone_of_cold->reset( false );
    p()->cooldowns.frost_nova->reset( false );
  }
};

// Combustion Spell =========================================================

struct combustion_t : public fire_mage_spell_t
{
  combustion_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    fire_mage_spell_t( n, p, p->find_specialization_spell( "Combustion" ) )
  {
    parse_options( options_str );
    dot_duration = 0_ms;
    harmful = false;
    usable_while_casting = true;
    cooldown->duration *= p->strive_for_perfection_multiplier;
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    p()->buffs.combustion->trigger();
    p()->buffs.wildfire->trigger();

    p()->buffs.rune_of_power->trigger();
    p()->distance_from_rune = 0.0;
  }
};

// Comet Storm Spell ========================================================

struct comet_storm_projectile_t : public frost_mage_spell_t
{
  comet_storm_projectile_t( util::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 153596 ) )
  {
    aoe = -1;
    background = consumes_winters_chill = triggers.radiant_spark = true;
  }
};

struct comet_storm_t : public frost_mage_spell_t
{
  timespan_t delay;
  action_t* projectile;

  comet_storm_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.comet_storm ),
    delay( timespan_t::from_seconds( p->find_spell( 228601 )->missile_speed() ) ),
    projectile( get_action<comet_storm_projectile_t>( "comet_storm_projectile", p ) )
  {
    parse_options( options_str );
    may_miss = may_crit = affected_by.shatter = false;
    add_child( projectile );
  }

  timespan_t travel_time() const override
  {
    return delay;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();
    p()->remaining_winters_chill = 0;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    int pulse_count = 7;
    timespan_t pulse_time = 0.2_s;
    p()->ground_aoe_expiration[ AOE_COMET_STORM ] = sim->current_time() + pulse_count * pulse_time;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( pulse_time )
      .target( s->target )
      .n_pulses( pulse_count )
      .action( projectile ) );
  }
};

// Cone of Cold Spell =======================================================

struct cone_of_cold_t : public frost_mage_spell_t
{
  cone_of_cold_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Cone of Cold" ) )
  {
    parse_options( options_str );
    aoe = -1;
    triggers.bone_chilling = consumes_winters_chill = triggers.radiant_spark = true;
  }
};

// Conflagration Spell ======================================================

struct conflagration_t : public fire_mage_spell_t
{
  conflagration_t( util::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 226757 ) )
  {
    background = true;
    affected_by.radiant_spark = false;
  }
};

struct conflagration_flare_up_t : public fire_mage_spell_t
{
  conflagration_flare_up_t( util::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 205345 ) )
  {
    background = true;
    affected_by.radiant_spark = false;
    aoe = -1;
  }
};

// Conjure Mana Gem Spell ===================================================

// Technically, this spell cannot be used when a Mana Gem is in the player's inventory,
// but we assume the player would just delete it before conjuring a new one.
struct conjure_mana_gem_t : public arcane_mage_spell_t
{
  conjure_mana_gem_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Conjure Mana Gem" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();
    p()->state.mana_gem_charges = as<int>( data().effectN( 2 ).base_value() );
  }
};

struct use_mana_gem_t : public action_t
{
  use_mana_gem_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    action_t( ACTION_USE, n, p, p->find_spell( 5405 ) )
  {
    parse_options( options_str );
    harmful = callbacks = may_crit = may_miss = false;
    target = player;
    background = p->specialization() != MAGE_ARCANE;
  }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( p->state.mana_gem_charges <= 0 || p->resources.pct( RESOURCE_MANA ) >= 1.0 )
      return false;

    return action_t::ready();
  }

  void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    action_t::execute();

    p->resource_gain( RESOURCE_MANA, p->resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent(), p->gains.mana_gem, this );
    p->state.mana_gem_charges--;
    assert( p->state.mana_gem_charges >= 0 );
  }

  // Needed to satisfy normal execute conditions
  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }
};

// Counterspell Spell =======================================================

struct counterspell_t : public mage_spell_t
{
  counterspell_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Counterspell" ) )
  {
    parse_options( options_str );
    may_miss = may_crit = false;
    ignore_false_positive = is_interrupt = true;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    bool success = p()->trigger_crowd_control( s, MECHANIC_INTERRUPT );
    if ( success )
      make_event( *sim, 0_ms, [ this ] { cooldown->adjust( -100 * p()->conduits.grounding_surge.time_value() ); } );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return mage_spell_t::target_ready( candidate_target );
  }
};

// Dragon's Breath Spell ====================================================

struct dragons_breath_t : public fire_mage_spell_t
{
  dragons_breath_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    fire_mage_spell_t( n, p, p->find_specialization_spell( "Dragon's Breath" ) )
  {
    parse_options( options_str );
    aoe = -1;
    triggers.from_the_ashes = triggers.radiant_spark = true;
    crit_bonus_multiplier *= 1.0 + p->talents.alexstraszas_fury->effectN( 2 ).percent();
    cooldown->duration += p->spec.dragons_breath_2->effectN( 1 ).time_value();

    if ( p->talents.alexstraszas_fury->ok() )
    {
      base_crit = 1.0;
      triggers.hot_streak = TT_MAIN_TARGET;
    }
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    if ( hit_any_target )
      p()->buffs.alexstraszas_fury->trigger();
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );
    p()->trigger_crowd_control( s, MECHANIC_DISORIENT );
  }
};

// Evocation Spell ==========================================================

struct evocation_t : public arcane_mage_spell_t
{
  bool precombat;
  int brain_storm_charges;
  int siphon_storm_charges;

  evocation_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Evocation" ) ),
    precombat(),
    brain_storm_charges(),
    siphon_storm_charges()
  {
    parse_options( options_str );
    base_tick_time = 1.0_s;
    dot_duration = data().duration();
    channeled = ignore_false_positive = tick_zero = true;
    harmful = false;
    cooldown->duration *= 1.0 + p->spec.evocation_2->effectN( 1 ).percent();

    if ( p->azerite.brain_storm.enabled() )
      brain_storm_charges = as<int>( p->find_spell( 288466 )->effectN( 1 ).base_value() );
    if ( p->runeforge.siphon_storm.ok() )
      siphon_storm_charges = as<int>( p->runeforge.siphon_storm->effectN( 1 ).base_value() );
  }

  void on_tick()
  {
    p()->buffs.brain_storm->trigger();
    p()->buffs.siphon_storm->trigger();
  }

  void init_finished() override
  {
    arcane_mage_spell_t::init_finished();

    if ( action_list->name_str == "precombat" )
      precombat = true;
  }

  void trigger_dot( action_state_t* s ) override
  {
    // When Evocation is used from the precombat action list, do not start the channel.
    // Just trigger the appropriate buffs and bail out.
    if ( precombat )
    {
      int ticks = as<int>( tick_zero ) + static_cast<int>( dot_duration / base_tick_time );
      for ( int i = 0; i < ticks; i++ )
        on_tick();
    }
    else
    {
      arcane_mage_spell_t::trigger_dot( s );
      p()->trigger_evocation();
    }
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( brain_storm_charges > 0 )
      p()->buffs.arcane_charge->trigger( brain_storm_charges );
    if ( siphon_storm_charges > 0 )
      p()->buffs.arcane_charge->trigger( siphon_storm_charges );
  }

  void tick( dot_t* d ) override
  {
    arcane_mage_spell_t::tick( d );
    on_tick();
  }

  void last_tick( dot_t* d ) override
  {
    arcane_mage_spell_t::last_tick( d );
    p()->buffs.evocation->expire();
  }

  bool usable_moving() const override
  {
    if ( p()->talents.slipstream->ok() )
      return true;

    return arcane_mage_spell_t::usable_moving();
  }
};

// Ebonbolt Spell ===========================================================

struct ebonbolt_t : public frost_mage_spell_t
{
  ebonbolt_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ebonbolt )
  {
    parse_options( options_str );
    parse_effect_data( p->find_spell( 257538 )->effectN( 1 ) );
    calculate_on_impact = track_shatter = consumes_winters_chill = triggers.radiant_spark = true;
    triggers.icy_propulsion = !p->bugs;

    if ( p->talents.splitting_ice->ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_aoe_multiplier *= p->talents.splitting_ice->effectN( 2 ).percent();
    }
  }

  void init_finished() override
  {
    proc_brain_freeze = p()->get_proc( "Brain Freeze from Ebonbolt" );
    frost_mage_spell_t::init_finished();
  }

  void execute() override
  {
    frost_mage_spell_t::execute();
    p()->trigger_brain_freeze( 1.0, proc_brain_freeze );
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public fire_mage_spell_t
{
  fireball_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    fire_mage_spell_t( n, p, p->find_specialization_spell( "Fireball" ) )
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.kindling = TT_ALL_TARGETS;
    triggers.ignite = triggers.from_the_ashes = triggers.radiant_spark = true;
    affected_by.deathborne_cleave = true;
    base_multiplier *= 1.0 + p->spec.fireball_3->effectN( 1 ).percent();
    base_dd_adder += p->azerite.duplicative_incineration.value( 2 );

    if ( p->talents.conflagration->ok() )
    {
      impact_action = get_action<conflagration_t>( "conflagration", p );
      add_child( impact_action );
    }
  }

  timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( t, 0.75_s );
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    // There is a delay where it is possible to cast a spell that would
    // consume Expanded Potential immediately after Expanded Potential
    // is triggered, which will prevent it from being consumed.
    p()->trigger_delayed_buff( p()->buffs.expanded_potential );

    if ( rng().roll( p()->azerite.duplicative_incineration.spell_ref().effectN( 1 ).percent() ) )
      execute();
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( s->result == RESULT_CRIT )
        p()->buffs.fireball->expire();
      else
        p()->buffs.fireball->trigger();

      consume_molten_skyfall( s->target );
      trigger_molten_skyfall();
    }
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = fire_mage_spell_t::composite_target_crit_chance( target );

    if ( firestarter_active( target ) )
      c += 1.0;

    return c;
  }

  double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    c += p()->buffs.fireball->check_stack_value();

    return c;
  }
};

// Flame Patch Spell ========================================================

struct flame_patch_t : public fire_mage_spell_t
{
  flame_patch_t( util::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 205472 ) )
  {
    aoe = -1;
    ground_aoe = background = true;
    affected_by.radiant_spark = false;
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }
};

// Flamestrike Spell ========================================================

struct flamestrike_t : public hot_streak_spell_t
{
  action_t* flame_patch;
  timespan_t flame_patch_duration;

  flamestrike_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    hot_streak_spell_t( n, p, p->find_specialization_spell( "Flamestrike" ) ),
    flame_patch()
  {
    parse_options( options_str );
    triggers.ignite = triggers.radiant_spark = true;
    aoe = -1;
    base_execute_time += p->spec.flamestrike_2->effectN( 1 ).time_value();
    base_multiplier *= 1.0 + p->spec.flamestrike_3->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->conduits.master_flame.percent();

    if ( p->talents.flame_patch->ok() )
    {
      flame_patch = get_action<flame_patch_t>( "flame_patch", p );
      flame_patch_duration = p->find_spell( 205470 )->duration();
      add_child( flame_patch );
    }
  }

  void execute() override
  {
    hot_streak_spell_t::execute();

    if ( flame_patch )
    {
      p()->ground_aoe_expiration[ AOE_FLAME_PATCH ] = sim->current_time() + flame_patch_duration;

      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( target )
        .duration( flame_patch_duration )
        .action( flame_patch )
        .hasted( ground_aoe_params_t::SPELL_SPEED ) );
    }
  }
};

// Flurry Spell =============================================================

struct glacial_assault_t : public frost_mage_spell_t
{
  glacial_assault_t( util::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 279856 ) )
  {
    background = true;
    aoe = -1;
    base_dd_min = base_dd_max = p->azerite.glacial_assault.value();
  }
};

struct flurry_bolt_t : public frost_mage_spell_t
{
  double glacial_assault_chance;

  flurry_bolt_t( util::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 228354 ) ),
    glacial_assault_chance()
  {
    background = triggers.bone_chilling = triggers.radiant_spark = true;
    base_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();
    glacial_assault_chance = p->azerite.glacial_assault.spell_ref().effectN( 1 ).trigger()->proc_chance();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->state.brain_freeze_active )
    {
      auto wc = td( s->target )->debuffs.winters_chill;
      wc->trigger( wc->max_stack() );
      for ( int i = 0; i < wc->max_stack(); i++ )
        p()->procs.winters_chill_applied->occur();
    }

    if ( rng().roll( glacial_assault_chance ) )
    {
      // Delay is around 1 s, but the impact seems to always happen in
      // the Winter's Chill window. So here we just subtract 1 ms to make
      // sure it hits while the debuff is up.
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( 999_ms )
        .target( s->target )
        .n_pulses( 1 )
        .action( p()->action.glacial_assault ) );
    }

    trigger_cold_front();
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    if ( p()->state.brain_freeze_active )
      am *= 1.0 + p()->buffs.brain_freeze->data().effectN( 2 ).percent();

    return am;
  }
};

struct flurry_t : public frost_mage_spell_t
{
  action_t* flurry_bolt;

  flurry_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Flurry" ) ),
    flurry_bolt( get_action<flurry_bolt_t>( "flurry_bolt", p ) )
  {
    parse_options( options_str );
    may_miss = may_crit = affected_by.shatter = false;

    add_child( flurry_bolt );
    if ( p->spec.icicles->ok() )
      add_child( p->action.icicle.flurry );
    if ( p->action.glacial_assault )
      add_child( p->action.glacial_assault );
  }

  void init() override
  {
    frost_mage_spell_t::init();
    // Snapshot haste for bolt impact timing.
    snapshot_flags |= STATE_HASTE;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.brain_freeze->check() )
      return 0_ms;

    return frost_mage_spell_t::execute_time();
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->trigger_icicle_gain( target, p()->action.icicle.flurry );
    if ( p()->player_t::buffs.memory_of_lucid_dreams->check() )
      p()->trigger_icicle_gain( target, p()->action.icicle.flurry );

    bool brain_freeze = p()->buffs.brain_freeze->up();
    p()->state.brain_freeze_active = brain_freeze;
    p()->buffs.brain_freeze->decrement();

    if ( brain_freeze )
    {
      p()->remaining_winters_chill = 2;
      p()->procs.brain_freeze_used->occur();
    }

    consume_cold_front( target );
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    timespan_t pulse_time = s->haste * 0.4_s;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( pulse_time )
      .target( s->target )
      .n_pulses( as<int>( data().effectN( 1 ).base_value() ) )
      .action( flurry_bolt ), true );
  }
};

// Frostbolt Spell ==========================================================

struct frostbolt_t : public frost_mage_spell_t
{
  frostbolt_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_class_spell( "Frostbolt" ) )
  {
    parse_options( options_str );
    parse_effect_data( p->find_spell( 228597 )->effectN( 1 ) );
    triggers.bone_chilling = calculate_on_impact = track_shatter = consumes_winters_chill = triggers.radiant_spark = affected_by.deathborne_cleave = true;
    base_multiplier *= 1.0 + p->spec.frostbolt_2->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();

    if ( p->spec.icicles->ok() )
      add_child( p->action.icicle.frostbolt );
  }

  void init_finished() override
  {
    proc_brain_freeze = p()->get_proc( "Brain Freeze from Frostbolt" );
    proc_fof = p()->get_proc( "Fingers of Frost from Frostbolt" );
    frost_mage_spell_t::init_finished();
  }

  timespan_t gcd() const override
  {
    timespan_t t = frost_mage_spell_t::gcd();

    t *= 1.0 + p()->buffs.slick_ice->check_stack_value();

    // TODO: This probably isn't intended since it breaks the spec even
    // at fairly mild haste levels.
    t = std::max( t, min_gcd );

    return t;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = frost_mage_spell_t::execute_time();

    t *= 1.0 + p()->buffs.slick_ice->check_stack_value();

    return t;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->trigger_icicle_gain( target, p()->action.icicle.frostbolt );
    if ( p()->player_t::buffs.memory_of_lucid_dreams->check() )
      p()->trigger_icicle_gain( target, p()->action.icicle.frostbolt );

    double fof_proc_chance = p()->spec.fingers_of_frost->effectN( 1 ).percent();
    fof_proc_chance *= 1.0 + p()->talents.frozen_touch->effectN( 1 ).percent();
    p()->trigger_fof( fof_proc_chance, 1, proc_fof );

    double bf_proc_chance = p()->spec.brain_freeze->effectN( 1 ).percent();
    bf_proc_chance *= 1.0 + p()->talents.frozen_touch->effectN( 1 ).percent();
    p()->trigger_brain_freeze( bf_proc_chance, proc_brain_freeze );

    if ( target != p()->last_frostbolt_target )
      p()->buffs.tunnel_of_ice->expire();
    p()->last_frostbolt_target = target;

    p()->trigger_delayed_buff( p()->buffs.expanded_potential );

    if ( p()->buffs.freezing_winds->check() == 0 )
      p()->cooldowns.frozen_orb->adjust( -p()->runeforge.freezing_winds->effectN( 1 ).time_value(), false );

    consume_cold_front( target );

    if ( p()->buffs.icy_veins->check() )
      p()->buffs.slick_ice->trigger();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.tunnel_of_ice->trigger();
      trigger_cold_front();
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = frost_mage_spell_t::bonus_da( s );

    da += p()->buffs.tunnel_of_ice->check_stack_value();

    return da;
  }
};

// Frost Nova Spell =========================================================

struct frost_nova_t : public mage_spell_t
{
  frost_nova_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Frost Nova" ) )
  {
    parse_options( options_str );
    aoe = -1;
    affected_by.shatter = triggers.radiant_spark = true;
    cooldown->charges += as<int>( p->talents.ice_ward->effectN( 1 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    timespan_t duration = timespan_t::min();
    if ( p()->runeforge.grisly_icicle.ok() )
    {
      auto debuff = td( s->target )->debuffs.grisly_icicle;
      duration = debuff->buff_duration();
      debuff->trigger();
    }
    p()->trigger_crowd_control( s, MECHANIC_ROOT, duration );
  }
};

// Frozen Orb Spell =========================================================

// TODO: Frozen Orb actually selects random targets each time it ticks when
// there are more than eight targets in range. This is not important for
// current use-cases. In the future if this becomes important, e.g., there
// is interest about priority target damage for encounters with more than
// eight targets, random target selection should be added as an option for
// all actions, because many target-capped abilities probably work this way.

struct frozen_orb_bolt_t : public frost_mage_spell_t
{
  frozen_orb_bolt_t( util::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 84721 ) )
  {
    aoe = as<int>( data().effectN( 2 ).base_value() );
    base_multiplier *= 1.0 + p->conduits.unrelenting_cold.percent();
    background = triggers.bone_chilling = true;
  }

  void init_finished() override
  {
    proc_fof = p()->get_proc( "Fingers of Frost from Frozen Orb Tick" );
    frost_mage_spell_t::init_finished();
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    if ( hit_any_target )
      p()->trigger_fof( p()->spec.fingers_of_frost->effectN( 2 ).percent(), 1, proc_fof );
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->cache.mastery() * p()->spec.icicles_2->effectN( 1 ).mastery_value();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuffs.packed_ice->trigger();
  }
};

struct frozen_orb_t : public frost_mage_spell_t
{
  action_t* frozen_orb_bolt;

  frozen_orb_t( util::string_view n, mage_t* p, util::string_view options_str, bool legendary = false ) :
    frost_mage_spell_t( n, p, legendary ? p->find_spell( 84714 ) : p->find_specialization_spell( "Frozen Orb" ) ),
    frozen_orb_bolt( get_action<frozen_orb_bolt_t>( legendary ? "legendary_frozen_orb_bolt" : "frozen_orb_bolt", p ) )
  {
    parse_options( options_str );
    may_miss = may_crit = affected_by.shatter = false;
    add_child( frozen_orb_bolt );

    if ( legendary )
    {
      background = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  }

  void init_finished() override
  {
    proc_fof = p()->get_proc( "Fingers of Frost from Frozen Orb Initial Impact" );
    frost_mage_spell_t::init_finished();
  }

  timespan_t travel_time() const override
  {
    timespan_t t = frost_mage_spell_t::travel_time();

    // Frozen Orb activates after about 0.5 s, even in melee range.
    t = std::max( t, 0.5_s );

    return t;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    if ( background )
      return;

    // TODO: Check how Cold Front and Freezing Winds interact
    p()->buffs.freezing_rain->trigger();
    p()->buffs.freezing_winds->trigger();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    p()->trigger_fof( 1.0, 1, proc_fof );

    int pulse_count = 20;
    timespan_t pulse_time = 0.5_s;
    p()->ground_aoe_expiration[ AOE_FROZEN_ORB ] = sim->current_time() + ( pulse_count - 1 ) * pulse_time;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( pulse_time )
      .target( s->target )
      .n_pulses( pulse_count )
      .action( frozen_orb_bolt )
      .expiration_callback( [ this ]
        {
          if ( p()->ground_aoe_expiration[ AOE_FROZEN_ORB ] <= sim->current_time() )
            p()->buffs.freezing_winds->expire();
        } ), true );
  }
};

// Glacial Spike Spell ======================================================

struct glacial_spike_t : public frost_mage_spell_t
{
  glacial_spike_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.glacial_spike )
  {
    parse_options( options_str );
    parse_effect_data( p->find_spell( 228600 )->effectN( 1 ) );
    calculate_on_impact = track_shatter = consumes_winters_chill = triggers.radiant_spark = true;
    base_dd_adder += p->azerite.flash_freeze.value( 2 ) * p->spec.icicles->effectN( 2 ).base_value();

    if ( p->talents.splitting_ice->ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_aoe_multiplier *= p->talents.splitting_ice->effectN( 2 ).percent();
    }
  }

  void init_finished() override
  {
    proc_fof = p()->get_proc( "Fingers of Frost from Flash Freeze" );
    frost_mage_spell_t::init_finished();
  }

  bool ready() override
  {
    // Glacial Spike doesn't check the Icicles buff after it started executing.
    if ( p()->executing != this && !p()->buffs.icicles->at_max_stacks() )
      return false;

    return frost_mage_spell_t::ready();
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    double coef = frost_mage_spell_t::spell_direct_power_coefficient( s );
    if ( p()->bugs )
      return coef;

    double icicle_coef = icicle_sp_coefficient();
    icicle_coef *=       p()->spec.icicles->effectN( 2 ).base_value();
    icicle_coef *= 1.0 + p()->talents.splitting_ice->effectN( 3 ).percent();

    coef += icicle_coef;

    return coef;
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();
    if ( !p()->bugs )
      return am;

    double icicle_coef = icicle_sp_coefficient();
    icicle_coef *=       p()->spec.icicles->effectN( 2 ).base_value();
    icicle_coef *= 1.0 + p()->talents.splitting_ice->effectN( 3 ).percent();

    // The damage from Icicles is added as multiplier that corresponds to
    // 1 + Icicle damage / base damage, for some reason.
    am *= 1.0 + icicle_coef / spell_power_mod.direct;

    return am;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->buffs.icicles->expire();
    while ( !p()->icicles.empty() )
      p()->get_icicle();

    double fof_proc_chance = p()->azerite.flash_freeze.spell_ref().effectN( 1 ).percent();
    for ( int i = 0; i < as<int>( p()->spec.icicles->effectN( 2 ).base_value() ); i++ )
      p()->trigger_fof( fof_proc_chance, 1, proc_fof );
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    p()->trigger_crowd_control( s, MECHANIC_ROOT );
  }
};

// Ice Floes Spell ==========================================================

struct ice_floes_t : public mage_spell_t
{
  ice_floes_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->talents.ice_floes )
  {
    parse_options( options_str );
    may_miss = may_crit = harmful = false;
    usable_while_casting = true;
    internal_cooldown->duration = data().internal_cooldown();
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.ice_floes->trigger();
  }
};

// Ice Lance Spell ==========================================================

struct glacial_fragments_t : public frost_mage_spell_t
{
  glacial_fragments_t( util::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 327498 ) )
  {
    background = true;
    affected_by.shatter = triggers.icy_propulsion = false;
  }
};

struct ice_lance_state_t : public mage_spell_state_t
{
  bool fingers_of_frost;

  ice_lance_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    fingers_of_frost()
  { }

  void initialize() override
  {
    mage_spell_state_t::initialize();
    fingers_of_frost = false;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s ) << " fingers_of_frost=" << fingers_of_frost;
    return s;
  }

  void copy_state( const action_state_t* s ) override
  {
    mage_spell_state_t::copy_state( s );
    fingers_of_frost = debug_cast<const ice_lance_state_t*>( s )->fingers_of_frost;
  }
};

struct ice_lance_t : public frost_mage_spell_t
{
  shatter_source_t* extension_source;
  shatter_source_t* cleave_source;

  action_t* glacial_fragments;

  ice_lance_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Ice Lance" ) ),
    extension_source(),
    cleave_source(),
    glacial_fragments()
  {
    parse_options( options_str );
    parse_effect_data( p->find_spell( 228598 )->effectN( 1 ) );
    calculate_on_impact = track_shatter = consumes_winters_chill = triggers.radiant_spark = true;
    base_multiplier *= 1.0 + p->spec.ice_lance_2->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();
    base_dd_adder += p->azerite.whiteout.value( 3 );

    // TODO: Cleave distance for SI seems to be 8 + hitbox size.
    if ( p->talents.splitting_ice->ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_multiplier *= 1.0 + p->talents.splitting_ice->effectN( 3 ).percent();
      base_aoe_multiplier *= p->talents.splitting_ice->effectN( 2 ).percent();
    }

    if ( p->runeforge.glacial_fragments.ok() )
    {
      glacial_fragments = get_action<glacial_fragments_t>( "glacial_fragments", p );
      add_child( glacial_fragments );
    }
  }

  void init_finished() override
  {
    frost_mage_spell_t::init_finished();

    if ( sim->report_details != 0 && p()->talents.splitting_ice->ok() )
      cleave_source = p()->get_shatter_source( "Ice Lance cleave" );
    if ( sim->report_details != 0 && p()->talents.thermal_void->ok() )
      extension_source = p()->get_shatter_source( "Thermal Void extension" );
  }

  action_state_t* new_state() override
  { return new ice_lance_state_t( this, target ); }

  unsigned frozen( const action_state_t* s ) const override
  {
    unsigned source = frost_mage_spell_t::frozen( s );

    // In game, FoF Ice Lances are implemented using a global flag which determines
    // whether to treat the targets as frozen or not. On IL execute, FoF is checked
    // and the flag set accordingly.
    //
    // This works fine under normal circumstances. However, once GCD drops below IL's
    // travel time, it's possible to:
    //
    //   a) cast FoF IL, cast non-FoF IL before the first one impacts
    //   b) cast non-FoF IL, cast FoF IL before the first one impacts
    //
    // In the a) case, neither Ice Lance gets the extra damage/Shatter bonus, in the
    // b) case, both Ice Lances do.
    if ( p()->bugs )
    {
      if ( p()->state.fingers_of_frost_active )
        source |= FF_FINGERS_OF_FROST;
    }
    else
    {
      if ( debug_cast<const ice_lance_state_t*>( s )->fingers_of_frost )
        source |= FF_FINGERS_OF_FROST;
    }

    return source;
  }

  void execute() override
  {
    p()->state.fingers_of_frost_active = p()->buffs.fingers_of_frost->up();

    frost_mage_spell_t::execute();

    p()->buffs.fingers_of_frost->decrement();

    // Begin casting all Icicles at the target, beginning 0.25 seconds after the
    // Ice Lance cast with remaining Icicles launching at intervals of 0.4
    // seconds, the latter adjusted by haste. Casting continues until all
    // Icicles are gone, including new ones that accumulate while they're being
    // fired. If target dies, Icicles stop.
    if ( !p()->talents.glacial_spike->ok() )
      p()->trigger_icicle( target, true );

    if ( p()->azerite.whiteout.enabled() )
      p()->cooldowns.frozen_orb->adjust( -100 * p()->azerite.whiteout.spell_ref().effectN( 2 ).time_value(), false );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    debug_cast<ice_lance_state_t*>( s )->fingers_of_frost = p()->buffs.fingers_of_frost->check() != 0;
    frost_mage_spell_t::snapshot_state( s, rt );
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    bool primary = s->chain_target == 0;
    unsigned frozen = cast_state( s )->frozen;

    if ( primary && frozen )
    {
      if ( p()->talents.thermal_void->ok() && p()->buffs.icy_veins->check() )
      {
        p()->buffs.icy_veins->extend_duration( p(), 1000 * p()->talents.thermal_void->effectN( 1 ).time_value() );
        record_shatter_source( s, extension_source );
      }

      if ( frozen &  FF_FINGERS_OF_FROST
        && frozen & ~FF_FINGERS_OF_FROST )
      {
        p()->procs.fingers_of_frost_wasted->occur();
      }

      p()->buffs.chain_reaction->trigger();
    }

    if ( !primary )
      record_shatter_source( s, cleave_source );

    // TODO: Determine how it is verified that Blizzard is actively "hitting" the target to increase this chance.
    if ( glacial_fragments )
    {
      double chance = p()->ground_aoe_expiration[ AOE_BLIZZARD ] > sim->current_time()
        ? p()->runeforge.glacial_fragments->effectN( 2 ).percent()
        : p()->runeforge.glacial_fragments->effectN( 1 ).percent();

      if ( rng().roll( chance ) )
      {
        glacial_fragments->set_target( s->target );
        glacial_fragments->execute();
      }
    }
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.chain_reaction->check_stack_value();

    return am;
  }

  double frozen_multiplier( const action_state_t* s ) const override
  {
    double m = frost_mage_spell_t::frozen_multiplier( s );

    m *= 3.0;
    m *= 1.0 + p()->conduits.ice_bite.percent();

    return m;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = frost_mage_spell_t::bonus_da( s );

    if ( auto td = p()->target_data[ s->target ] )
    {
      double pi_bonus = td->debuffs.packed_ice->check_value();

      // Splitting Ice nerfs this trait by 33%, see:
      // https://us.battle.net/forums/en/wow/topic/20769009293#post-1
      if ( num_targets_hit > 1 )
        pi_bonus *= 0.666;

      da += pi_bonus;
    }

    return da;
  }
};

// Ice Nova Spell ===========================================================

struct ice_nova_t : public frost_mage_spell_t
{
  ice_nova_t( util::string_view n, mage_t* p, util::string_view options_str ) :
     frost_mage_spell_t( n, p, p->talents.ice_nova )
  {
    parse_options( options_str );
    aoe = -1;
    consumes_winters_chill = triggers.radiant_spark = reduced_aoe_damage = true;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    p()->trigger_crowd_control( s, MECHANIC_ROOT );
  }
};

// Icy Veins Spell ==========================================================

struct icy_veins_t : public frost_mage_spell_t
{
  icy_veins_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Icy Veins" ) )
  {
    parse_options( options_str );
    harmful = false;
    cooldown->duration *= p->strive_for_perfection_multiplier;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->buffs.icy_veins->trigger();

    // Frigid Grasp buff doesn't refresh.
    p()->buffs.frigid_grasp->expire();
    p()->buffs.frigid_grasp->trigger();

    p()->buffs.rune_of_power->trigger();
    p()->distance_from_rune = 0.0;
  }
};

// Fire Blast Spell =========================================================

struct fire_blast_t : public fire_mage_spell_t
{
  fire_blast_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    fire_mage_spell_t( n, p, p->spec.fire_blast_3->ok() ? p->spec.fire_blast_3 : p->find_class_spell( "Fire Blast" ) )
  {
    parse_options( options_str );
    usable_while_casting = p->spec.fire_blast_3->ok();
    triggers.hot_streak = triggers.kindling = TT_MAIN_TARGET;
    triggers.ignite = triggers.from_the_ashes = triggers.radiant_spark = true;

    cooldown->charges += as<int>( p->spec.fire_blast_4->effectN( 1 ).base_value() );
    cooldown->charges += as<int>( p->talents.flame_on->effectN( 1 ).base_value() );
    cooldown->duration -= 1000 * p->talents.flame_on->effectN( 3 ).time_value();
    cooldown->hasted = true;

    base_crit += p->spec.fire_blast_2->effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.blaster_master->trigger();
      if ( p()->buffs.combustion->check() )
        p()->buffs.infernal_cascade->trigger();
    }
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = fire_mage_spell_t::recharge_multiplier( cd );

    if ( &cd == cooldown && p()->player_t::buffs.memory_of_lucid_dreams->check() )
      m /= 1.0 + p()->player_t::buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();

    return m;
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_dot_t : public fire_mage_spell_t
{
  // The game has two spells for the DoT, one for pre-spread one and one for
  // post-spread one. This allows two copies of the DoT to be up on one target.
  const bool primary;

  static unsigned dot_spell_id( bool primary )
  { return primary ? 217694 : 244813; }

  living_bomb_dot_t( util::string_view n, mage_t* p, bool primary_ ) :
    fire_mage_spell_t( n, p, p->find_spell( dot_spell_id( primary_ ) ) ),
    primary( primary_ )
  {
    background = true;
    affected_by.radiant_spark = false;
  }

  void init() override
  {
    fire_mage_spell_t::init();
    update_flags &= ~STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  void trigger_explosion( player_t* target )
  {
    p()->action.living_bomb_explosion->set_target( target );

    if ( primary )
    {
      auto targets = p()->action.living_bomb_explosion->target_list();
      for ( auto t : targets )
      {
        if ( t == target )
          continue;

        p()->action.living_bomb_dot_spread->set_target( t );
        p()->action.living_bomb_dot_spread->execute();
      }
    }

    p()->action.living_bomb_explosion->execute();
  }

  void trigger_dot( action_state_t* s ) override
  {
    if ( get_dot( s->target )->is_ticking() )
      trigger_explosion( s->target );

    fire_mage_spell_t::trigger_dot( s );
  }

  void last_tick( dot_t* d ) override
  {
    fire_mage_spell_t::last_tick( d );
    trigger_explosion( d->target );
  }
};

struct living_bomb_explosion_t : public fire_mage_spell_t
{
  living_bomb_explosion_t( util::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 44461 ) )
  {
    aoe = -1;
    background = reduced_aoe_damage = true;
    affected_by.radiant_spark = false;
  }
};

struct living_bomb_t : public fire_mage_spell_t
{
  living_bomb_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.living_bomb )
  {
    parse_options( options_str );
    cooldown->hasted = true;
    may_miss = may_crit = false;
    impact_action = p->action.living_bomb_dot;

    if ( data().ok() )
    {
      add_child( p->action.living_bomb_dot );
      add_child( p->action.living_bomb_dot_spread );
      add_child( p->action.living_bomb_explosion );
    }
  }
};

// Meteor Spell =============================================================

// Implementation details from Celestalon:
// http://blue.mmo-champion.com/topic/318876-warlords-of-draenor-theorycraft-discussion/#post301
// Meteor is split over a number of spell IDs
// - Meteor (id=153561) is the talent spell, the driver
// - Meteor (id=153564) is the initial impact damage
// - Meteor Burn (id=155158) is the ground effect tick damage
// - Meteor Burn (id=175396) provides the tooltip's burn duration
// - Meteor (id=177345) contains the time between cast and impact
struct meteor_burn_t : public fire_mage_spell_t
{
  meteor_burn_t( util::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 155158 ) )
  {
    background = ground_aoe = true;
    aoe = -1;
    std::swap( spell_power_mod.direct, spell_power_mod.tick );
    dot_duration = 0_ms;
    radius = p->find_spell( 153564 )->effectN( 1 ).radius_max();
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }
};

struct meteor_impact_t : public fire_mage_spell_t
{
  action_t* meteor_burn;

  timespan_t meteor_burn_duration;
  timespan_t meteor_burn_pulse_time;

  meteor_impact_t( util::string_view n, mage_t* p, action_t* burn ) :
    fire_mage_spell_t( n, p, p->find_spell( 153564 ) ),
    meteor_burn( burn ),
    meteor_burn_duration( p->find_spell( 175396 )->duration() ),
    meteor_burn_pulse_time( p->find_spell( 155158 )->effectN( 1 ).period() )
  {
    background = split_aoe_damage = true;
    aoe = -1;
    triggers.ignite = triggers.radiant_spark = true;
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( data().missile_speed() );
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( s->chain_target == 0 )
    {
      p()->ground_aoe_expiration[ AOE_METEOR_BURN ] = sim->current_time() + meteor_burn_duration;

      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( meteor_burn_pulse_time )
        .target( s->target )
        .duration( meteor_burn_duration )
        .action( meteor_burn ) );
    }
  }
};

struct meteor_t : public fire_mage_spell_t
{
  timespan_t meteor_delay;

  meteor_t( util::string_view n, mage_t* p, util::string_view options_str, bool legendary = false ) :
    fire_mage_spell_t( n, p, legendary ? p->find_spell( 153561 ) : p->talents.meteor ),
    meteor_delay( p->find_spell( 177345 )->duration() )
  {
    parse_options( options_str );

    if ( !data().ok() )
      return;

    action_t* meteor_burn = get_action<meteor_burn_t>( legendary ? "legendary_meteor_burn" : "meteor_burn", p );
    action_t* meteor_impact = get_action<meteor_impact_t>( legendary ? "legendary_meteor_impact" : "meteor_impact", p, meteor_burn );

    impact_action = meteor_impact;

    add_child( meteor_burn );
    add_child( meteor_impact );

    if ( legendary )
    {
      background = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  }

  timespan_t travel_time() const override
  {
    timespan_t impact_time = meteor_delay * p()->cache.spell_speed();
    timespan_t meteor_spawn = impact_time - impact_action->travel_time();
    return std::max( meteor_spawn, 0_ms );
  }
};

// Mirror Image Spell =======================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Mirror Image" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void init_finished() override
  {
    for ( auto image : p()->pets.mirror_images )
    {
      for ( auto a : image->action_list )
        add_child( a );
    }

    mage_spell_t::init_finished();
  }

  void execute() override
  {
    mage_spell_t::execute();

    for ( auto image : p()->pets.mirror_images )
      image->summon( data().duration() );
  }
};

// Nether Tempest Spell =====================================================

struct nether_tempest_aoe_t : public arcane_mage_spell_t
{
  nether_tempest_aoe_t( util::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 114954 ) )
  {
    aoe = -1;
    background = true;
    affected_by.radiant_spark = false;
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( data().missile_speed() );
  }
};

struct nether_tempest_t : public arcane_mage_spell_t
{
  action_t* nether_tempest_aoe;

  nether_tempest_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.nether_tempest ),
    nether_tempest_aoe( get_action<nether_tempest_aoe_t>( "nether_tempest_aoe", p ) )
  {
    parse_options( options_str );
    add_child( nether_tempest_aoe );
    affected_by.radiant_spark = false;
  }

  void execute() override
  {
    p()->benefits.arcane_charge.nether_tempest->update();

    arcane_mage_spell_t::execute();

    if ( hit_any_target )
    {
      if ( p()->last_bomb_target && p()->last_bomb_target != target )
        td( p()->last_bomb_target )->dots.nether_tempest->cancel();
      p()->last_bomb_target = target;
    }
  }

  void tick( dot_t* d ) override
  {
    arcane_mage_spell_t::tick( d );

    action_state_t* aoe_state = nether_tempest_aoe->get_state();
    aoe_state->target = d->target;
    nether_tempest_aoe->snapshot_state( aoe_state, nether_tempest_aoe->amount_type( aoe_state ) );

    aoe_state->persistent_multiplier *= d->state->persistent_multiplier;
    aoe_state->da_multiplier *= d->get_tick_factor();
    aoe_state->ta_multiplier *= d->get_tick_factor();

    nether_tempest_aoe->schedule_execute( aoe_state );
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double m = arcane_mage_spell_t::composite_persistent_multiplier( s );

    m *= arcane_charge_multiplier();

    return m;
  }
};

// Phoenix Flames Spell =====================================================

struct phoenix_flames_splash_t : public fire_mage_spell_t
{
  int max_spread_targets;

  phoenix_flames_splash_t( util::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 257542 ) )
  {
    aoe = -1;
    background = reduced_aoe_damage = true;
    triggers.hot_streak = triggers.kindling = TT_MAIN_TARGET;
    triggers.ignite = triggers.radiant_spark = true;
    max_spread_targets = as<int>( p->spec.ignite->effectN( 4 ).base_value() );
    // TODO: Phoenix Flames currently does not trigger fevered incantation
    // on the beta. Verify whether this is fixed closer to shadowlands release.
    triggers.fevered_incantation = !p->bugs;
  }

  static double ignite_bank( dot_t* ignite )
  {
    if ( !ignite->is_ticking() )
      return 0.0;

    auto ignite_state = debug_cast<residual_action::residual_periodic_state_t*>( ignite->state );
    return ignite_state->tick_amount * ignite->ticks_left();
  }

  void impact( action_state_t* s ) override
  {
    // TODO: Verify what happens when Phoenix Flames hits an immune target.
    if ( result_is_hit( s->result ) && s->chain_target == 0 )
    {
      auto source = s->target->get_dot( "ignite", player );
      if ( source->is_ticking() )
      {
        std::vector<dot_t*> ignites;

        // Collect the Ignite DoT objects of all targets that are in range.
        for ( auto t : target_list() )
          ignites.push_back( t->get_dot( "ignite", player ) );

        // Sort candidate Ignites by descending bank size.
        // TODO: Double check what targets the Ignite spread prioritizes.
        std::stable_sort( ignites.begin(), ignites.end(), [] ( dot_t* a, dot_t* b )
        { return ignite_bank( a ) > ignite_bank( b ); } );

        auto source_bank = ignite_bank( source );
        auto targets_remaining = max_spread_targets;
        for ( auto destination : ignites )
        {
          // The original spread source doesn't count towards the spread target limit.
          if ( source == destination )
            continue;

          // Target cap was reached, stop.
          if ( targets_remaining-- <= 0 )
            break;

          // Source Ignite cannot spread to targets with higher Ignite bank. It will
          // still count towards the spread target cap, though.
          if ( ignite_bank( destination ) >= source_bank )
            continue;

          if ( destination->is_ticking() )
            p()->procs.ignite_overwrite->occur();
          else
            p()->procs.ignite_new_spread->occur();

          // TODO: Exact copies of the Ignite are not spread. Instead, the Ignites can
          // sometimes have partial ticks, but the conditions for this are not known.
          destination->cancel();
          source->copy( destination->target, DOT_COPY_CLONE );
        }
      }
    }

    fire_mage_spell_t::impact( s );
  }
};

struct phoenix_flames_t : public fire_mage_spell_t
{
  phoenix_flames_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    fire_mage_spell_t( n, p, p->find_specialization_spell( "Phoenix Flames" ) )
  {
    parse_options( options_str );
    cooldown->charges += as<int>( p->spec.phoenix_flames_2->effectN( 1 ).base_value() );

    impact_action = get_action<phoenix_flames_splash_t>( "phoenix_flames_splash", p );
    add_child( impact_action );
  }

  timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( t, 0.75_s );
  }
};

// Pyroblast Spell ==========================================================

struct trailing_embers_t : public fire_mage_spell_t
{
  trailing_embers_t( util::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 277703 ) )
  {
    background = tick_zero = true;
    hasted_ticks = false;
    base_td = p->azerite.trailing_embers.value();
  }
};

struct pyroblast_dot_t : public fire_mage_spell_t
{
  pyroblast_dot_t( util::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 321712 ) )
  {
    background = true;
    cooldown->duration = 0_ms;
    affected_by.radiant_spark = false;
  }
};

struct pyroblast_t : public hot_streak_spell_t
{
  action_t* trailing_embers;
  action_t* pyroblast_dot;

  pyroblast_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    hot_streak_spell_t( n, p, p->find_specialization_spell( "Pyroblast" ) ),
    trailing_embers(),
    pyroblast_dot()
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.kindling = TT_MAIN_TARGET;
    triggers.ignite = triggers.from_the_ashes = triggers.radiant_spark = true;
    base_dd_adder += p->azerite.wildfire.value( 2 );
    base_multiplier *= 1.0 + p->conduits.controlled_destruction.percent();

    if ( p->azerite.trailing_embers.enabled() )
    {
      trailing_embers = get_action<trailing_embers_t>( "trailing_embers", p );
      add_child( trailing_embers );
    }

    if ( p->spec.pyroblast_2->ok() )
    {
      pyroblast_dot = get_action<pyroblast_dot_t>( "pyroblast_dot", p );
      add_child( pyroblast_dot );
    }
  }

  double action_multiplier() const override
  {
    double am = hot_streak_spell_t::action_multiplier();

    if ( time_to_execute > 0_ms )
      am *= 1.0 + p()->buffs.pyroclasm->check_value();

    return am;
  }

  void execute() override
  {
    // TODO: Check whether Sun King's Blessing activates when
    // an instant cast Pyroblast without Hot Streak is used
    if ( time_to_execute > 0_ms && p()->buffs.sun_kings_blessing_ready->check() )
    {
      p()->buffs.sun_kings_blessing_ready->expire();
      p()->buffs.combustion->extend_duration_or_trigger( 1000 * p()->runeforge.sun_kings_blessing->effectN( 2 ).time_value() );
    }

    hot_streak_spell_t::execute();

    if ( time_to_execute > 0_ms )
      p()->buffs.pyroclasm->decrement();

    if ( pyroblast_dot )
    {
      pyroblast_dot->set_target( target );
      pyroblast_dot->execute();
    }
  }

  timespan_t travel_time() const override
  {
    timespan_t t = hot_streak_spell_t::travel_time();
    return std::min( t, 0.75_s );
  }

  void impact( action_state_t* s ) override
  {
    hot_streak_spell_t::impact( s );

    if ( trailing_embers )
    {
      auto targets = target_list();
      for ( auto t : targets )
      {
        trailing_embers->set_target( t );
        trailing_embers->execute();
      }
    }

    if ( result_is_hit( s->result ) )
    {
      consume_molten_skyfall( s->target );
      trigger_molten_skyfall();
    }
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = hot_streak_spell_t::composite_target_crit_chance( target );

    if ( firestarter_active( target ) )
      c += 1.0;

    return c;
  }
};

// Ray of Frost Spell =======================================================

struct ray_of_frost_t : public frost_mage_spell_t
{
  ray_of_frost_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ray_of_frost )
  {
    parse_options( options_str );
    channeled = triggers.bone_chilling = triggers.radiant_spark = true;
    triggers.icy_propulsion = false;
  }

  void init_finished() override
  {
    proc_fof = p()->get_proc( "Fingers of Frost from Ray of Frost" );
    frost_mage_spell_t::init_finished();
  }

  void tick( dot_t* d ) override
  {
    frost_mage_spell_t::tick( d );
    p()->buffs.ray_of_frost->trigger();

    // Ray of Frost triggers Bone Chilling on each tick, as well as on execute.
    p()->buffs.bone_chilling->trigger();

    // TODO: Now happens at 2.5 and 5.
    if ( d->current_tick == 3 || d->current_tick == 5 )
      p()->trigger_fof( 1.0, 1, proc_fof );
  }

  void last_tick( dot_t* d ) override
  {
    frost_mage_spell_t::last_tick( d );
    p()->buffs.ray_of_frost->expire();
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.ray_of_frost->check_stack_value();

    return am;
  }
};

// Rune of Power Spell ======================================================

struct rune_of_power_t : public mage_spell_t
{
  rune_of_power_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->talents.rune_of_power )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    mage_spell_t::execute();

    p()->buffs.rune_of_power->trigger();
    p()->distance_from_rune = 0.0;
  }
};

// Scorch Spell =============================================================

struct scorch_t : public fire_mage_spell_t
{
  scorch_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    fire_mage_spell_t( n, p, p->find_specialization_spell( "Scorch" ) )
  {
    parse_options( options_str );
    triggers.hot_streak = TT_MAIN_TARGET;
    triggers.ignite = triggers.from_the_ashes = triggers.radiant_spark = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( s->target->health_percentage() < p()->talents.searing_touch->effectN( 1 ).base_value() )
      m *= 1.0 + p()->talents.searing_touch->effectN( 2 ).percent();

    return m;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = fire_mage_spell_t::composite_target_crit_chance( target );

    if ( target->health_percentage() < p()->talents.searing_touch->effectN( 1 ).base_value() )
      c += 1.0;

    return c;
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      p()->buffs.frenetic_speed->trigger();
  }

  timespan_t travel_time() const override
  {
    return fire_mage_spell_t::travel_time() + p()->options.scorch_delay;
  }

  bool usable_moving() const override
  { return true; }
};

// Shimmer Spell ============================================================

struct shimmer_t : public mage_spell_t
{
  shimmer_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->talents.shimmer )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = usable_while_casting = true;
    base_teleport_distance = data().effectN( 1 ).radius_max();
    movement_directionality = movement_direction_type::OMNI;
    cooldown->duration += p->conduits.flow_of_time.time_value();
  }
};

// Slow Spell ===============================================================

struct slow_t : public arcane_mage_spell_t
{
  slow_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Slow" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }
};

// Supernova Spell ==========================================================

struct supernova_t : public arcane_mage_spell_t
{
  supernova_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.supernova )
  {
    parse_options( options_str );
    aoe = -1;
    affected_by.savant = triggers.radiant_spark = true;

    double sn_mult = 1.0 + p->talents.supernova->effectN( 1 ).percent();
    base_multiplier     *= sn_mult;
    base_aoe_multiplier /= sn_mult;
  }
};

// Summon Water Elemental Spell =============================================

struct summon_water_elemental_t : public frost_mage_spell_t
{
  summon_water_elemental_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Summon Water Elemental" ) )
  {
    parse_options( options_str );
    harmful = track_cd_waste = false;
    ignore_false_positive = true;
    background = p->talents.lonely_winter->ok();
  }

  void execute() override
  {
    frost_mage_spell_t::execute();
    p()->pets.water_elemental->summon();
  }

  bool ready() override
  {
    if ( !p()->pets.water_elemental || !p()->pets.water_elemental->is_sleeping() )
      return false;

    return frost_mage_spell_t::ready();
  }
};

// Time Warp Spell ==========================================================

struct time_warp_t : public mage_spell_t
{
  time_warp_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Time Warp" ) )
  {
    parse_options( options_str );
    harmful = false;
    background = sim->overrides.bloodlust != 0 && !p->runeforge.temporal_warp.ok();
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( player->buffs.exhaustion->check() )
      p()->buffs.temporal_warp->trigger();

    for ( player_t* p : sim->player_non_sleeping_list )
    {
      if ( p->buffs.exhaustion->check() || p->is_pet() )
        continue;

      p->buffs.bloodlust->trigger();
      p->buffs.exhaustion->trigger();
    }
  }

  bool ready() override
  {
    if ( player->buffs.exhaustion->check() && !p()->runeforge.temporal_warp.ok() )
      return false;

    return mage_spell_t::ready();
  }
};

// Touch of the Magi Spell ==================================================

struct touch_of_the_magi_t : public arcane_mage_spell_t
{
  touch_of_the_magi_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->spec.touch_of_the_magi )
  {
    parse_options( options_str );

    if ( data().ok() )
      add_child( p->action.touch_of_the_magi_explosion );
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( p()->spec.touch_of_the_magi_2->ok() )
      p()->buffs.arcane_charge->trigger( as<int>( data().effectN( 2 ).base_value() ) );
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuffs.touch_of_the_magi->trigger();
  }
};

struct touch_of_the_magi_explosion_t : public arcane_mage_spell_t
{
  touch_of_the_magi_explosion_t( util::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 210833 ) )
  {
    background = reduced_aoe_damage = true;
    may_miss = may_crit = callbacks = false;
    affected_by.radiant_spark = false;
    aoe = -1;
    base_dd_min = base_dd_max = 1.0;
  }

  void init() override
  {
    arcane_mage_spell_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = arcane_mage_spell_t::composite_target_multiplier( target );

    // It seems that TotM explosion only double dips on target based damage reductions
    // and not target based damage increases.
    m = std::min( m, 1.0 );

    return m;
  }
};

struct arcane_echo_t : public arcane_mage_spell_t
{
  arcane_echo_t( util::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 342232 ) )
  {
    background = true;
    callbacks = false;
    affected_by.radiant_spark = false;
    aoe = as<int>( p->talents.arcane_echo->effectN( 1 ).base_value() );
  }
};

// ==========================================================================
// Mage Covenant Abilities
// ==========================================================================

// Deathborne Spell =========================================================

struct deathborne_t : public mage_spell_t
{
  deathborne_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_covenant_spell( "Deathborne" ) )
  {
    parse_options( options_str );
    // TODO: Which Ice Floes effect allows the covenant abilities to be cast while moving?
    affected_by.ice_floes = true;
    harmful = false;
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.deathborne->trigger();
  }
};

// Mirrors of Torment Spell =================================================

struct agonizing_backlash_t : public mage_spell_t
{
  agonizing_backlash_t( util::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 320035 ) )
  {
    background = true;
    callbacks = false;
  }
};

struct tormenting_backlash_t : public mage_spell_t
{
  tormenting_backlash_t( util::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 317589 ) )
  {
    background = true;
    callbacks = false;
  }
};

struct mirrors_of_torment_t : public mage_spell_t
{
  mirrors_of_torment_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_covenant_spell( "Mirrors of Torment" ) )
  {
    parse_options( options_str );
    affected_by.ice_floes = true;

    if ( data().ok() )
    {
      add_child( p->action.agonizing_backlash );
      add_child( p->action.tormenting_backlash );
    }
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );
    td( s->target )->debuffs.mirrors_of_torment->trigger();
  }
};

// Radiant Spark Spell ======================================================

struct radiant_spark_t : public mage_spell_t
{
  radiant_spark_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_covenant_spell( "Radiant Spark" ) )
  {
    parse_options( options_str );
    affected_by.ice_floes = affected_by.savant = true;
    affected_by.radiant_spark = false;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( auto td = p()->target_data[ s->target ] )
    {
      // If Radiant Spark is refreshed, the vulnerability debuff can be
      // triggered once again. Any previous stacks of the debuff are removed.
      td->debuffs.radiant_spark_vulnerability->cooldown->reset( false );
      td->debuffs.radiant_spark_vulnerability->expire();
    }
  }

  void last_tick( dot_t* d ) override
  {
    mage_spell_t::last_tick( d );

    if ( auto td = p()->target_data[ d->target ] )
      td->debuffs.radiant_spark_vulnerability->expire();
  }

  timespan_t calculate_dot_refresh_duration( const dot_t* d, timespan_t duration ) const override
  {
    return duration + d->time_to_next_tick();
  }
};

// Shifting Power Spell =====================================================

struct shifting_power_pulse_t : public mage_spell_t
{
  shifting_power_pulse_t( util::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 325130 ) )
  {
    background = true;
    callbacks = false;
    aoe = -1;
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }
};

struct shifting_power_t : public mage_spell_t
{
  std::vector<cooldown_t*> shifting_power_cooldowns;
  timespan_t reduction;

  shifting_power_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    mage_spell_t( n, p, p->find_covenant_spell( "Shifting Power" ) ),
    shifting_power_cooldowns(),
    reduction( data().effectN( 2 ).time_value() )
  {
    parse_options( options_str );
    channeled = affected_by.ice_floes = true;
    affected_by.shifting_power = false;
    tick_action = get_action<shifting_power_pulse_t>( "shifting_power_pulse", p );
  }

  void init_finished() override
  {
    mage_spell_t::init_finished();

    for ( auto a : p()->action_list )
    {
      auto m = dynamic_cast<mage_spell_t*>( a );
      if ( m
        && m->affected_by.shifting_power
        && range::find( shifting_power_cooldowns, m->cooldown ) == shifting_power_cooldowns.end() )
      {
        shifting_power_cooldowns.push_back( m->cooldown );
      }
    }
  }

  void tick( dot_t* d ) override
  {
    mage_spell_t::tick( d );

    for ( auto cd : shifting_power_cooldowns )
      cd->adjust( reduction, false );
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = mage_spell_t::tick_time( s );

    t *= 1.0 + p()->conduits.discipline_of_the_grove.percent();

    return t;
  }
};

// ==========================================================================
// Mage Custom Actions
// ==========================================================================

// Arcane Mage "Burn" State Switch Action ===================================

void report_burn_switch_error( action_t* a )
{
  throw std::runtime_error(
    fmt::format( "{} action {} infinite loop detected (no time passing between executes) at '{}'",
                 a->player->name(), a->name(), a->signature_str ) );
}

struct start_burn_phase_t : public action_t
{
  start_burn_phase_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    action_t( ACTION_OTHER, n, p )
  {
    parse_options( options_str );
    trigger_gcd = 0_ms;
    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    bool success = p->burn_phase.enable( sim->current_time() );
    if ( !success )
      report_burn_switch_error( this );

    p->sample_data.burn_initial_mana->add( 100.0 * p->resources.pct( RESOURCE_MANA ) );
    p->uptime.burn_phase->update( true, sim->current_time() );
    p->uptime.conserve_phase->update( false, sim->current_time() );
  }

  bool ready() override
  {
    if ( debug_cast<mage_t*>( player )->burn_phase.on() )
      return false;

    return action_t::ready();
  }
};

struct stop_burn_phase_t : public action_t
{
  stop_burn_phase_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    action_t( ACTION_OTHER, n, p )
  {
    parse_options( options_str );
    trigger_gcd = 0_ms;
    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    p->sample_data.burn_duration_history->add( p->burn_phase.duration( sim->current_time() ).total_seconds() );

    bool success = p->burn_phase.disable( sim->current_time() );
    if ( !success )
      report_burn_switch_error( this );

    p->uptime.burn_phase->update( false, sim->current_time() );
    p->uptime.conserve_phase->update( true, sim->current_time() );
  }

  bool ready() override
  {
    if ( !debug_cast<mage_t*>( player )->burn_phase.on() )
      return false;

    return action_t::ready();
  }
};

// Proxy Freeze Action ======================================================

struct freeze_t : public action_t
{
  freeze_t( util::string_view n, mage_t* p, util::string_view options_str ) :
    action_t( ACTION_OTHER, n, p, p->find_specialization_spell( "Freeze" ) )
  {
    parse_options( options_str );
    may_miss = may_crit = callbacks = false;
    dual = usable_while_casting = ignore_false_positive = true;
    background = p->talents.lonely_winter->ok();
  }

  void execute() override
  {
    mage_t* m = debug_cast<mage_t*>( player );
    m->pets.water_elemental->action.freeze->set_target( target );
    m->pets.water_elemental->action.freeze->execute();
  }

  bool ready() override
  {
    mage_t* m = debug_cast<mage_t*>( player );

    if ( !m->pets.water_elemental || m->pets.water_elemental->is_sleeping() )
      return false;

    // Make sure the cooldown is actually ready and not just within cooldown tolerance.
    auto freeze = m->pets.water_elemental->action.freeze;
    if ( !freeze->cooldown->up() || !freeze->ready() )
      return false;

    return action_t::ready();
  }
};

}  // namespace actions


namespace events {

struct enlightened_event_t : public event_t
{
  mage_t* mage;

  enlightened_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ),
    mage( &m )
  { }

  const char* name() const override
  { return "enlightened_event"; }

  void execute() override
  {
    mage->events.enlightened = nullptr;
    mage->update_enlightened();
    mage->events.enlightened = make_event<enlightened_event_t>( sim(), *mage, mage->options.enlightened_interval );
  }
};

struct focus_magic_event_t : public event_t
{
  mage_t* mage;

  focus_magic_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ),
    mage( &m )
  { }

  const char* name() const override
  { return "focus_magic_event"; }

  void execute() override
  {
    mage->events.focus_magic = nullptr;

    if ( rng().roll( mage->options.focus_magic_crit_chance ) )
    {
      mage->buffs.focus_magic_crit->trigger();
      mage->buffs.focus_magic_int->trigger();
    }

    if ( mage->options.focus_magic_interval > 0_ms )
    {
      timespan_t period = mage->options.focus_magic_interval;
      period = std::max( 1_ms, mage->rng().gauss( period, period * mage->options.focus_magic_stddev ) );
      mage->events.focus_magic = make_event<focus_magic_event_t>( sim(), *mage, period );
    }
  }
};

struct icicle_event_t : public event_t
{
  mage_t* mage;
  player_t* target;

  icicle_event_t( mage_t& m, player_t* t, bool first = false ) :
    event_t( m ),
    mage( &m ),
    target( t )
  {
    schedule( first ? 0.25_s : 0.4_s * mage->cache.spell_speed() );
  }

  const char* name() const override
  { return "icicle_event"; }

  void execute() override
  {
    mage->events.icicle = nullptr;

    // If the target of the icicle is dead, stop the chain
    if ( target->is_sleeping() )
    {
      sim().print_debug( "{} icicle use on {} (sleeping target), stopping", mage->name(), target->name() );
      return;
    }

    action_t* icicle_action = mage->get_icicle();
    if ( !icicle_action )
      return;

    icicle_action->set_target( target );
    icicle_action->execute();

    if ( !mage->icicles.empty() )
    {
      mage->events.icicle = make_event<icicle_event_t>( sim(), *mage, target );
      sim().print_debug( "{} icicle use on {} (chained), total={}", mage->name(), target->name(), mage->icicles.size() );
    }
  }
};

struct from_the_ashes_event_t : public event_t
{
  mage_t* mage;

  from_the_ashes_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ),
    mage( &m )
  { }

  const char* name() const override
  { return "from_the_ashes_event"; }

  void execute() override
  {
    mage->events.from_the_ashes = nullptr;
    mage->update_from_the_ashes();
    mage->events.from_the_ashes = make_event<from_the_ashes_event_t>( sim(), *mage, mage->options.from_the_ashes_interval );
  }
};

struct time_anomaly_tick_event_t : public event_t
{
  mage_t* mage;

  enum ta_proc_type_e
  {
    TA_ARCANE_POWER,
    TA_EVOCATION,
    TA_TIME_WARP
  };

  time_anomaly_tick_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ),
    mage( &m )
  { }

  const char* name() const override
  { return "time_anomaly_tick_event"; }

  void execute() override
  {
    mage->events.time_anomaly = nullptr;
    sim().print_log( "{} Time Anomaly tick event occurs.", mage->name() );

    if ( mage->shuffled_rng.time_anomaly->trigger() )
    {
      sim().print_log( "{} Time Anomaly proc successful, triggering effects.", mage->name() );

      std::vector<ta_proc_type_e> possible_procs;

      if ( mage->buffs.arcane_power->check() == 0 )
        possible_procs.push_back( TA_ARCANE_POWER );
      if ( mage->buffs.evocation->check() == 0 )
        possible_procs.push_back( TA_EVOCATION );
      if ( mage->buffs.time_warp->check() == 0 && mage->player_t::buffs.bloodlust->check() == 0 )
        possible_procs.push_back( TA_TIME_WARP );

      if ( !possible_procs.empty() )
      {
        auto proc = possible_procs[ rng().range( possible_procs.size() ) ];
        switch ( proc )
        {
          case TA_ARCANE_POWER:
          {
            timespan_t duration = 1000 * mage->talents.time_anomaly->effectN( 1 ).time_value();
            mage->buffs.arcane_power->trigger( duration );
            break;
          }
          case TA_EVOCATION:
          {
            timespan_t duration = 1000 * mage->talents.time_anomaly->effectN( 2 ).time_value();
            mage->trigger_evocation( duration, false );
            break;
          }
          case TA_TIME_WARP:
          {
            mage->buffs.time_warp->trigger();
            break;
          }
          default:
            break;
        }
      }
    }

    mage->events.time_anomaly = make_event<events::time_anomaly_tick_event_t>(
      sim(), *mage, mage->talents.time_anomaly->effectN( 1 ).period() );
  }
};

}  // namespace events

// ==========================================================================
// Mage Character Definition
// ==========================================================================

mage_td_t::mage_td_t( player_t* target, mage_t* mage ) :
  actor_target_data_t( target, mage ),
  dots(),
  debuffs()
{
  // Baseline
  dots.nether_tempest = target->get_dot( "nether_tempest", mage );

  debuffs.frozen            = make_buff( *this, "frozen" )
                                ->set_duration( mage->options.frozen_duration );
  debuffs.touch_of_the_magi = make_buff<buffs::touch_of_the_magi_t>( this );
  debuffs.winters_chill     = make_buff( *this, "winters_chill", mage->find_spell( 228358 ) )
                                ->set_chance( mage->spec.brain_freeze_2->ok() );

  // Runeforge Legendaries
  debuffs.grisly_icicle = make_buff( *this, "grisly_icicle", mage->find_spell( 122 ) )
                            ->set_default_value( mage->runeforge.grisly_icicle->effectN( 2 ).percent() )
                            ->modify_duration( mage->spec.frost_nova_2->effectN( 1 ).time_value() )
                            ->set_chance( mage->runeforge.grisly_icicle.ok() );

  // Covenant Abilities
  dots.radiant_spark = target->get_dot( "radiant_spark", mage );

  debuffs.mirrors_of_torment          = make_buff<buffs::mirrors_of_torment_t>( this );
  debuffs.radiant_spark_vulnerability = make_buff( *this, "radiant_spark_vulnerability", mage->find_spell( 307454 ) )
                                          ->set_default_value_from_effect( 1 )
                                          ->modify_default_value( mage->conduits.ire_of_the_ascended.percent() )
                                          ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  // Azerite
  debuffs.packed_ice = make_buff( *this, "packed_ice", mage->find_spell( 272970 ) )
                         ->set_chance( mage->azerite.packed_ice.enabled() )
                         ->set_default_value( mage->azerite.packed_ice.value() );
}

mage_t::mage_t( sim_t* sim, util::string_view name, race_e r ) :
  player_t( sim, MAGE, name, r ),
  events(),
  last_bomb_target(),
  last_frostbolt_target(),
  ground_aoe_expiration(),
  distance_from_rune(),
  lucid_dreams_refund(),
  strive_for_perfection_multiplier(),
  vision_of_perfection_multiplier(),
  action(),
  benefits(),
  buffs(),
  cooldowns(),
  gains(),
  options(),
  pets(),
  procs(),
  shuffled_rng(),
  sample_data(),
  spec(),
  state(),
  talents(),
  azerite(),
  runeforge(),
  conduits(),
  uptime()
{
  // Cooldowns
  cooldowns.arcane_power     = get_cooldown( "arcane_power"     );
  cooldowns.combustion       = get_cooldown( "combustion"       );
  cooldowns.cone_of_cold     = get_cooldown( "cone_of_cold"     );
  cooldowns.fire_blast       = get_cooldown( "fire_blast"       );
  cooldowns.from_the_ashes   = get_cooldown( "from_the_ashes"   );
  cooldowns.frost_nova       = get_cooldown( "frost_nova"       );
  cooldowns.frozen_orb       = get_cooldown( "frozen_orb"       );
  cooldowns.icy_veins        = get_cooldown( "icy_veins"        );
  cooldowns.phoenix_flames   = get_cooldown( "phoenix_flames"   );
  cooldowns.presence_of_mind = get_cooldown( "presence_of_mind" );

  // Options
  resource_regeneration = regen_type::DYNAMIC;
}

bool mage_t::trigger_crowd_control( const action_state_t* s, spell_mechanic type, timespan_t d )
{
  if ( type == MECHANIC_INTERRUPT )
    return s->target->debuffs.casting->check() != 0;

  if ( action_t::result_is_hit( s->result )
    && ( s->target->is_add() || s->target->level() < sim->max_player_level + 3 ) )
  {
    if ( type == MECHANIC_ROOT )
      get_target_data( s->target )->debuffs.frozen->trigger( d );

    return true;
  }

  return false;
}

action_t* mage_t::create_action( util::string_view name, const std::string& options_str )
{
  using namespace actions;

  // Arcane
  if ( name == "arcane_barrage"         ) return new         arcane_barrage_t( name, this, options_str );
  if ( name == "arcane_blast"           ) return new           arcane_blast_t( name, this, options_str );
  if ( name == "arcane_familiar"        ) return new        arcane_familiar_t( name, this, options_str );
  if ( name == "arcane_missiles"        ) return new        arcane_missiles_t( name, this, options_str );
  if ( name == "arcane_orb"             ) return new             arcane_orb_t( name, this, options_str );
  if ( name == "arcane_power"           ) return new           arcane_power_t( name, this, options_str );
  if ( name == "conjure_mana_gem"       ) return new       conjure_mana_gem_t( name, this, options_str );
  if ( name == "evocation"              ) return new              evocation_t( name, this, options_str );
  if ( name == "nether_tempest"         ) return new         nether_tempest_t( name, this, options_str );
  if ( name == "presence_of_mind"       ) return new       presence_of_mind_t( name, this, options_str );
  if ( name == "slow"                   ) return new                   slow_t( name, this, options_str );
  if ( name == "supernova"              ) return new              supernova_t( name, this, options_str );
  if ( name == "touch_of_the_magi"      ) return new      touch_of_the_magi_t( name, this, options_str );
  if ( name == "use_mana_gem"           ) return new           use_mana_gem_t( name, this, options_str );

  if ( name == "start_burn_phase"       ) return new       start_burn_phase_t( name, this, options_str );
  if ( name == "stop_burn_phase"        ) return new        stop_burn_phase_t( name, this, options_str );

  // Fire
  if ( name == "blast_wave"             ) return new             blast_wave_t( name, this, options_str );
  if ( name == "combustion"             ) return new             combustion_t( name, this, options_str );
  if ( name == "dragons_breath"         ) return new         dragons_breath_t( name, this, options_str );
  if ( name == "fireball"               ) return new               fireball_t( name, this, options_str );
  if ( name == "flamestrike"            ) return new            flamestrike_t( name, this, options_str );
  if ( name == "living_bomb"            ) return new            living_bomb_t( name, this, options_str );
  if ( name == "meteor"                 ) return new                 meteor_t( name, this, options_str );
  if ( name == "phoenix_flames"         ) return new         phoenix_flames_t( name, this, options_str );
  if ( name == "pyroblast"              ) return new              pyroblast_t( name, this, options_str );
  if ( name == "scorch"                 ) return new                 scorch_t( name, this, options_str );

  // Frost
  if ( name == "blizzard"               ) return new               blizzard_t( name, this, options_str );
  if ( name == "cold_snap"              ) return new              cold_snap_t( name, this, options_str );
  if ( name == "comet_storm"            ) return new            comet_storm_t( name, this, options_str );
  if ( name == "cone_of_cold"           ) return new           cone_of_cold_t( name, this, options_str );
  if ( name == "ebonbolt"               ) return new               ebonbolt_t( name, this, options_str );
  if ( name == "flurry"                 ) return new                 flurry_t( name, this, options_str );
  if ( name == "frozen_orb"             ) return new             frozen_orb_t( name, this, options_str );
  if ( name == "glacial_spike"          ) return new          glacial_spike_t( name, this, options_str );
  if ( name == "ice_floes"              ) return new              ice_floes_t( name, this, options_str );
  if ( name == "ice_lance"              ) return new              ice_lance_t( name, this, options_str );
  if ( name == "ice_nova"               ) return new               ice_nova_t( name, this, options_str );
  if ( name == "icy_veins"              ) return new              icy_veins_t( name, this, options_str );
  if ( name == "ray_of_frost"           ) return new           ray_of_frost_t( name, this, options_str );
  if ( name == "summon_water_elemental" ) return new summon_water_elemental_t( name, this, options_str );

  if ( name == "freeze"                 ) return new                 freeze_t( name, this, options_str );

  // Shared spells
  if ( name == "arcane_explosion"       ) return new       arcane_explosion_t( name, this, options_str );
  if ( name == "arcane_intellect"       ) return new       arcane_intellect_t( name, this, options_str );
  if ( name == "blink"                  ) return new                  blink_t( name, this, options_str );
  if ( name == "counterspell"           ) return new           counterspell_t( name, this, options_str );
  if ( name == "fire_blast"             ) return new             fire_blast_t( name, this, options_str );
  if ( name == "frostbolt"              ) return new              frostbolt_t( name, this, options_str );
  if ( name == "frost_nova"             ) return new             frost_nova_t( name, this, options_str );
  if ( name == "mirror_image"           ) return new           mirror_image_t( name, this, options_str );
  if ( name == "time_warp"              ) return new              time_warp_t( name, this, options_str );

  // Shared talents
  if ( name == "rune_of_power"          ) return new          rune_of_power_t( name, this, options_str );
  if ( name == "shimmer"                ) return new                shimmer_t( name, this, options_str );

  // Covenant Abilities
  if ( name == "deathborne"             ) return new             deathborne_t( name, this, options_str );
  if ( name == "mirrors_of_torment"     ) return new     mirrors_of_torment_t( name, this, options_str );
  if ( name == "radiant_spark"          ) return new          radiant_spark_t( name, this, options_str );
  if ( name == "shifting_power"         ) return new         shifting_power_t( name, this, options_str );

  // Special
  if ( name == "blink_any" )
    return create_action( talents.shimmer->ok() ? "shimmer" : "blink", options_str );

  return player_t::create_action( name, options_str );
}

void mage_t::create_actions()
{
  using namespace actions;

  if ( spec.ignite->ok() )
    action.ignite = get_action<ignite_t>( "ignite", this );

  if ( spec.icicles->ok() )
  {
    action.icicle.frostbolt    = get_action<icicle_t>( "frostbolt_icicle", this );
    action.icicle.flurry       = get_action<icicle_t>( "flurry_icicle", this );
    action.icicle.lucid_dreams = get_action<icicle_t>( "lucid_dreams_icicle", this );
  }

  if ( talents.arcane_familiar->ok() )
    action.arcane_assault = get_action<arcane_assault_t>( "arcane_assault", this );

  if ( talents.arcane_echo->ok() )
    action.arcane_echo = get_action<arcane_echo_t>( "arcane_echo", this );

  if ( talents.conflagration->ok() )
    action.conflagration_flare_up = get_action<conflagration_flare_up_t>( "conflagration_flare_up", this );

  if ( talents.living_bomb->ok() )
  {
    action.living_bomb_dot        = get_action<living_bomb_dot_t>( "living_bomb_dot", this, true );
    action.living_bomb_dot_spread = get_action<living_bomb_dot_t>( "living_bomb_dot_spread", this, false );
    action.living_bomb_explosion  = get_action<living_bomb_explosion_t>( "living_bomb_explosion", this );
  }

  if ( spec.touch_of_the_magi->ok() )
    action.touch_of_the_magi_explosion = get_action<touch_of_the_magi_explosion_t>( "touch_of_the_magi_explosion", this );

  if ( azerite.glacial_assault.enabled() )
    action.glacial_assault = get_action<glacial_assault_t>( "glacial_assault", this );

  if ( runeforge.molten_skyfall.ok() )
    action.legendary_meteor = get_action<meteor_t>( "legendary_meteor", this, "", true );

  if ( runeforge.cold_front.ok() )
    action.legendary_frozen_orb = get_action<frozen_orb_t>( "legendary_frozen_orb", this, "", true );

  if ( find_covenant_spell( "Mirrors of Torment" )->ok() )
  {
    action.agonizing_backlash  = get_action<agonizing_backlash_t>( "agonizing_backlash", this );
    action.tormenting_backlash = get_action<tormenting_backlash_t>( "tormenting_backlash", this );
  }

  player_t::create_actions();

  // Ensure the cooldown of Phoenix Flames is properly initialized.
  if ( talents.from_the_ashes->ok() && !find_action( "phoenix_flames" ) )
    create_action( "phoenix_flames", "" );
}

void mage_t::create_options()
{
  add_option( opt_timespan( "firestarter_time", options.firestarter_time ) );
  add_option( opt_timespan( "frozen_duration", options.frozen_duration ) );
  add_option( opt_timespan( "scorch_delay", options.scorch_delay ) );
  add_option( opt_float( "lucid_dreams_proc_chance_arcane", options.lucid_dreams_proc_chance_arcane ) );
  add_option( opt_float( "lucid_dreams_proc_chance_fire", options.lucid_dreams_proc_chance_fire ) );
  add_option( opt_float( "lucid_dreams_proc_chance_frost", options.lucid_dreams_proc_chance_frost ) );
  add_option( opt_timespan( "enlightened_interval", options.enlightened_interval, 1_ms, timespan_t::max() ) );
  add_option( opt_timespan( "focus_magic_interval", options.focus_magic_interval, 0_ms, timespan_t::max() ) );
  add_option( opt_float( "focus_magic_stddev", options.focus_magic_stddev, 0.0, std::numeric_limits<double>::max() ) );
  add_option( opt_float( "focus_magic_crit_chance", options.focus_magic_crit_chance, 0.0, 1.0 ) );
  add_option( opt_timespan( "from_the_ashes_interval", options.from_the_ashes_interval, 1_ms, timespan_t::max() ) );
  add_option( opt_timespan( "mirrors_of_torment_interval", options.mirrors_of_torment_interval, 1_ms, timespan_t::max() ) );

  player_t::create_options();
}

std::string mage_t::create_profile( save_e save_type )
{
  std::string profile = player_t::create_profile( save_type );

  if ( save_type & SAVE_PLAYER )
  {
    if ( options.firestarter_time > 0_ms )
      profile += "firestarter_time=" + util::to_string( options.firestarter_time.total_seconds() ) + "\n";
  }

  return profile;
}

void mage_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  options = debug_cast<mage_t*>( source )->options;
}

void mage_t::merge( player_t& other )
{
  player_t::merge( other );

  mage_t& mage = dynamic_cast<mage_t&>( other );

  for ( size_t i = 0; i < cooldown_waste_data_list.size(); i++ )
    cooldown_waste_data_list[ i ]->merge( *mage.cooldown_waste_data_list[ i ] );

  for ( size_t i = 0; i < shatter_source_list.size(); i++ )
    shatter_source_list[ i ]->merge( *mage.shatter_source_list[ i ] );

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      sample_data.burn_duration_history->merge( *mage.sample_data.burn_duration_history );
      sample_data.burn_initial_mana->merge( *mage.sample_data.burn_initial_mana );
      break;
    case MAGE_FROST:
      if ( talents.thermal_void->ok() )
        sample_data.icy_veins_duration->merge( *mage.sample_data.icy_veins_duration );
      break;
    default:
      break;
  }
}

void mage_t::analyze( sim_t& s )
{
  player_t::analyze( s );

  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::analyze ) );

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      sample_data.burn_duration_history->analyze();
      sample_data.burn_initial_mana->analyze();
      break;
    case MAGE_FROST:
      if ( talents.thermal_void->ok() )
        sample_data.icy_veins_duration->analyze();
      break;
    default:
      break;
  }
}

void mage_t::datacollection_begin()
{
  player_t::datacollection_begin();

  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::datacollection_begin ) );
  range::for_each( shatter_source_list, std::mem_fn( &shatter_source_t::datacollection_begin ) );
}

void mage_t::datacollection_end()
{
  player_t::datacollection_end();

  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::datacollection_end ) );
  range::for_each( shatter_source_list, std::mem_fn( &shatter_source_t::datacollection_end ) );
}

void mage_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( resources.is_active( RESOURCE_MANA ) && buffs.evocation->check() )
  {
    double base = resource_regen_per_second( RESOURCE_MANA );
    if ( base )
    {
      // Base regen was already done, subtract 1.0 from Evocation's mana regen multiplier to make
      // sure we don't apply it twice.
      resource_gain(
        RESOURCE_MANA,
        ( buffs.evocation->check_value() - 1.0 ) * base * periodicity.total_seconds(),
        gains.evocation );
    }
  }
}

void mage_t::moving()
{
  if ( ( executing  && !executing->usable_moving() )
    || ( queueing   && !queueing->usable_moving() )
    || ( channeling && !channeling->usable_moving() ) )
  {
    player_t::moving();
  }
}

void mage_t::create_pets()
{
  player_t::create_pets();

  if ( specialization() == MAGE_FROST && !talents.lonely_winter->ok() && find_action( "summon_water_elemental" ) )
    pets.water_elemental = new pets::water_elemental::water_elemental_pet_t( sim, this );

  auto a = find_action( "mirror_image" );
  if ( a && a->data().ok() )
  {
    for ( int i = 0; i < as<int>( a->data().effectN( 2 ).base_value() ); i++ )
    {
      auto image = new pets::mirror_image::mirror_image_pet_t( sim, this );
      if ( i > 0 )
        image->quiet = true;
      pets.mirror_images.push_back( image );
    }
  }
}

void mage_t::init_spells()
{
  player_t::init_spells();

  // Talents
  // Tier 15
  talents.amplification      = find_talent_spell( "Amplification"      );
  talents.rule_of_threes     = find_talent_spell( "Rule of Threes"     );
  talents.arcane_familiar    = find_talent_spell( "Arcane Familiar"    );
  talents.firestarter        = find_talent_spell( "Firestarter"        );
  talents.pyromaniac         = find_talent_spell( "Pyromaniac"         );
  talents.searing_touch      = find_talent_spell( "Searing Touch"      );
  talents.bone_chilling      = find_talent_spell( "Bone Chilling"      );
  talents.lonely_winter      = find_talent_spell( "Lonely Winter"      );
  talents.ice_nova           = find_talent_spell( "Ice Nova"           );
  // Tier 25
  talents.shimmer            = find_talent_spell( "Shimmer"            );
  talents.master_of_time     = find_talent_spell( "Master of Time"     );
  talents.slipstream         = find_talent_spell( "Slipstream"         );
  talents.blazing_soul       = find_talent_spell( "Blazing Soul"       );
  talents.blast_wave         = find_talent_spell( "Blast Wave"         );
  talents.glacial_insulation = find_talent_spell( "Glacial Insulation" );
  talents.ice_floes          = find_talent_spell( "Ice Floes"          );
  // Tier 30
  talents.incanters_flow     = find_talent_spell( "Incanter's Flow"    );
  talents.focus_magic        = find_talent_spell( "Focus Magic"        );
  talents.rune_of_power      = find_talent_spell( "Rune of Power"      );
  // Tier 35
  talents.resonance          = find_talent_spell( "Resonance"          );
  talents.arcane_echo        = find_talent_spell( "Arcane Echo"        );
  talents.nether_tempest     = find_talent_spell( "Nether Tempest"     );
  talents.flame_on           = find_talent_spell( "Flame On"           );
  talents.alexstraszas_fury  = find_talent_spell( "Alexstrasza's Fury" );
  talents.from_the_ashes     = find_talent_spell( "From the Ashes"     );
  talents.frozen_touch       = find_talent_spell( "Frozen Touch"       );
  talents.chain_reaction     = find_talent_spell( "Chain Reaction"     );
  talents.ebonbolt           = find_talent_spell( "Ebonbolt"           );
  // Tier 40
  talents.ice_ward           = find_talent_spell( "Ice Ward"           );
  talents.ring_of_frost      = find_talent_spell( "Ring of Frost"      );
  talents.chrono_shift       = find_talent_spell( "Chrono Shift"       );
  talents.frenetic_speed     = find_talent_spell( "Frenetic Speed"     );
  talents.frigid_winds       = find_talent_spell( "Frigid Winds"       );
  // Tier 45
  talents.reverberate        = find_talent_spell( "Reverberate"        );
  talents.arcane_orb         = find_talent_spell( "Arcane Orb"         );
  talents.supernova          = find_talent_spell( "Supernova"          );
  talents.flame_patch        = find_talent_spell( "Flame Patch"        );
  talents.conflagration      = find_talent_spell( "Conflagration"      );
  talents.living_bomb        = find_talent_spell( "Living Bomb"        );
  talents.freezing_rain      = find_talent_spell( "Freezing Rain"      );
  talents.splitting_ice      = find_talent_spell( "Splitting Ice"      );
  talents.comet_storm        = find_talent_spell( "Comet Storm"        );
  // Tier 50
  talents.overpowered        = find_talent_spell( "Overpowered"        );
  talents.time_anomaly       = find_talent_spell( "Time Anomaly"       );
  talents.enlightened        = find_talent_spell( "Enlightened"        );
  talents.kindling           = find_talent_spell( "Kindling"           );
  talents.pyroclasm          = find_talent_spell( "Pyroclasm"          );
  talents.meteor             = find_talent_spell( "Meteor"             );
  talents.thermal_void       = find_talent_spell( "Thermal Void"       );
  talents.ray_of_frost       = find_talent_spell( "Ray of Frost"       );
  talents.glacial_spike      = find_talent_spell( "Glacial Spike"      );

  // Spec Spells
  spec.arcane_barrage_2      = find_specialization_spell( "Arcane Barrage", "Rank 2" );
  spec.arcane_barrage_3      = find_specialization_spell( "Arcane Barrage", "Rank 3" );
  spec.arcane_charge         = find_spell( 36032 );
  spec.arcane_explosion_2    = find_specialization_spell( "Arcane Explosion", "Rank 2" );
  spec.arcane_mage           = find_specialization_spell( "Arcane Mage" );
  spec.arcane_power_2        = find_specialization_spell( "Arcane Power", "Rank 2" );
  spec.arcane_power_3        = find_specialization_spell( "Arcane Power", "Rank 3" );
  spec.clearcasting          = find_specialization_spell( "Clearcasting" );
  spec.clearcasting_2        = find_specialization_spell( "Clearcasting", "Rank 2" );
  spec.clearcasting_3        = find_specialization_spell( "Clearcasting", "Rank 3" );
  spec.evocation_2           = find_specialization_spell( "Evocation", "Rank 2" );
  spec.presence_of_mind_2    = find_specialization_spell( "Presence of Mind", "Rank 2" );
  spec.touch_of_the_magi     = find_specialization_spell( "Touch of the Magi" );
  spec.touch_of_the_magi_2   = find_specialization_spell( "Touch of the Magi", "Rank 2" );

  spec.combustion_2          = find_specialization_spell( "Combustion", "Rank 2" );
  spec.critical_mass         = find_specialization_spell( "Critical Mass" );
  spec.critical_mass_2       = find_specialization_spell( "Critical Mass", "Rank 2" );
  spec.dragons_breath_2      = find_specialization_spell( "Dragon's Breath", "Rank 2" );
  spec.fireball_2            = find_specialization_spell( "Fireball", "Rank 2" );
  spec.fireball_3            = find_specialization_spell( "Fireball", "Rank 3" );
  spec.fire_blast_2          = find_specialization_spell( "Fire Blast", "Rank 2" );
  spec.fire_blast_3          = find_specialization_spell( "Fire Blast", "Rank 3" );
  spec.fire_blast_4          = find_specialization_spell( "Fire Blast", "Rank 4" );
  spec.fire_mage             = find_specialization_spell( "Fire Mage" );
  spec.flamestrike_2         = find_specialization_spell( "Flamestrike", "Rank 2" );
  spec.flamestrike_3         = find_specialization_spell( "Flamestrike", "Rank 3" );
  spec.hot_streak            = find_specialization_spell( "Hot Streak" );
  spec.phoenix_flames_2      = find_specialization_spell( "Phoenix Flames", "Rank 2" );
  spec.pyroblast_2           = find_specialization_spell( "Pyroblast", "Rank 2" );

  spec.brain_freeze          = find_specialization_spell( "Brain Freeze" );
  spec.brain_freeze_2        = find_specialization_spell( "Brain Freeze", "Rank 2" );
  spec.blizzard_2            = find_specialization_spell( "Blizzard", "Rank 2" );
  spec.blizzard_3            = find_specialization_spell( "Blizzard", "Rank 3" );
  spec.cold_snap_2           = find_specialization_spell( "Cold Snap", "Rank 2" );
  spec.fingers_of_frost      = find_specialization_spell( "Fingers of Frost" );
  spec.frost_mage            = find_specialization_spell( "Frost Mage" );
  spec.frost_nova_2          = find_specialization_spell( "Frost Nova", "Rank 2" );
  spec.frostbolt_2           = find_specialization_spell( "Frostbolt", "Rank 2" );
  spec.ice_lance_2           = find_specialization_spell( "Ice Lance", "Rank 2" );
  spec.icy_veins_2           = find_specialization_spell( "Icy Veins", "Rank 2" );
  spec.shatter               = find_specialization_spell( "Shatter" );
  spec.shatter_2             = find_specialization_spell( "Shatter", "Rank 2" );


  // Mastery
  spec.savant                = find_mastery_spell( MAGE_ARCANE );
  spec.ignite                = find_mastery_spell( MAGE_FIRE );
  spec.icicles               = find_mastery_spell( MAGE_FROST );
  spec.icicles_2             = find_specialization_spell( "Mastery: Icicles", "Rank 2" );

  // Azerite
  azerite.arcane_pressure          = find_azerite_spell( "Arcane Pressure"          );
  azerite.arcane_pummeling         = find_azerite_spell( "Arcane Pummeling"         );
  azerite.brain_storm              = find_azerite_spell( "Brain Storm"              );
  azerite.equipoise                = find_azerite_spell( "Equipoise"                );
  azerite.explosive_echo           = find_azerite_spell( "Explosive Echo"           );
  azerite.galvanizing_spark        = find_azerite_spell( "Galvanizing Spark"        );

  azerite.blaster_master           = find_azerite_spell( "Blaster Master"           );
  azerite.duplicative_incineration = find_azerite_spell( "Duplicative Incineration" );
  azerite.firemind                 = find_azerite_spell( "Firemind"                 );
  azerite.flames_of_alacrity       = find_azerite_spell( "Flames of Alacrity"       );
  azerite.trailing_embers          = find_azerite_spell( "Trailing Embers"          );
  azerite.wildfire                 = find_azerite_spell( "Wildfire"                 );

  azerite.flash_freeze             = find_azerite_spell( "Flash Freeze"             );
  azerite.frigid_grasp             = find_azerite_spell( "Frigid Grasp"             );
  azerite.glacial_assault          = find_azerite_spell( "Glacial Assault"          );
  azerite.packed_ice               = find_azerite_spell( "Packed Ice"               );
  azerite.tunnel_of_ice            = find_azerite_spell( "Tunnel of Ice"            );
  azerite.whiteout                 = find_azerite_spell( "Whiteout"                 );

  // Runeforge Legendaries
  runeforge.arcane_bombardment   = find_runeforge_legendary( "Arcane Bombardment"   );
  runeforge.arcane_harmony       = find_runeforge_legendary( "Arcane Infinity"      );
  runeforge.siphon_storm         = find_runeforge_legendary( "Siphon Storm"         );
  runeforge.temporal_warp        = find_runeforge_legendary( "Temporal Warp"        );

  runeforge.fevered_incantation  = find_runeforge_legendary( "Fevered Incantation"  );
  runeforge.firestorm            = find_runeforge_legendary( "Firestorm"            );
  runeforge.molten_skyfall       = find_runeforge_legendary( "Molten Skyfall"       );
  runeforge.sun_kings_blessing   = find_runeforge_legendary( "Sun King's Blessing"  );

  runeforge.cold_front           = find_runeforge_legendary( "Cold Front"           );
  runeforge.freezing_winds       = find_runeforge_legendary( "Freezing Winds"       );
  runeforge.glacial_fragments    = find_runeforge_legendary( "Glacial Fragments"    );
  runeforge.slick_ice            = find_runeforge_legendary( "Slick Ice"            );

  runeforge.disciplinary_command = find_runeforge_legendary( "Disciplinary Command" );
  runeforge.expanded_potential   = find_runeforge_legendary( "Expanded Potential"   );
  runeforge.grisly_icicle        = find_runeforge_legendary( "Grisly Icicle"        );

  // Soulbind Conduits
  conduits.arcane_prodigy           = find_conduit_spell( "Arcane Prodigy"           );
  conduits.artifice_of_the_archmage = find_conduit_spell( "Artifice of the Archmage" );
  conduits.magis_brand              = find_conduit_spell( "Magi's Brand"             );
  conduits.nether_precision         = find_conduit_spell( "Nether Precision"         );

  conduits.controlled_destruction   = find_conduit_spell( "Controlled Destruction"   );
  conduits.flame_accretion          = find_conduit_spell( "Flame Accretion"          );
  conduits.infernal_cascade         = find_conduit_spell( "Infernal Cascade"         );
  conduits.master_flame             = find_conduit_spell( "Master Flame"             );

  conduits.ice_bite                 = find_conduit_spell( "Ice Bite"                 );
  conduits.icy_propulsion           = find_conduit_spell( "Icy Propulsion"           );
  conduits.shivering_core           = find_conduit_spell( "Shivering Core"           );
  conduits.unrelenting_cold         = find_conduit_spell( "Unrelenting Cold"         );

  conduits.discipline_of_the_grove  = find_conduit_spell( "Discipline of the Grove"  );
  conduits.flow_of_time             = find_conduit_spell( "Flow of Time"             );
  conduits.gift_of_the_lich         = find_conduit_spell( "Gift of the Lich"         );
  conduits.grounding_surge          = find_conduit_spell( "Grounding Surge"          );
  conduits.ire_of_the_ascended      = find_conduit_spell( "Ire of the Ascended"      );
  conduits.siphoned_malice          = find_conduit_spell( "Siphoned Malice"          );

  auto memory = find_azerite_essence( "Memory of Lucid Dreams" );
  lucid_dreams_refund = memory.spell( 1u, essence_type::MINOR )->effectN( 1 ).percent();

  auto vision = find_azerite_essence( "Vision of Perfection" );
  strive_for_perfection_multiplier = 1.0 + azerite::vision_of_perfection_cdr( vision );
  vision_of_perfection_multiplier =
    vision.spell( 1u, essence_type::MAJOR )->effectN( 1 ).percent() +
    vision.spell( 2u, essence_spell::UPGRADE, essence_type::MAJOR )->effectN( 1 ).percent();
}

void mage_t::init_base_stats()
{
  if ( base.distance < 1.0 )
  {
    if ( specialization() == MAGE_ARCANE )
      base.distance = 10.0;
    else
      base.distance = 30.0;
  }

  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  // Mana Attunement
  resources.base_regen_per_second[ RESOURCE_MANA ] *= 1.0 + find_spell( 121039 )->effectN( 1 ).percent();

  if ( specialization() == MAGE_ARCANE )
    regen_caches[ CACHE_MASTERY ] = true;
}

void mage_t::create_buffs()
{
  player_t::create_buffs();

  // Arcane
  buffs.arcane_charge        = make_buff( this, "arcane_charge", spec.arcane_charge )
                                 ->set_stack_change_callback( [ this ] ( buff_t*, int old, int cur )
                                   { if ( old < 3 && cur >= 3 ) buffs.rule_of_threes->trigger(); } );
  buffs.arcane_power         = make_buff( this, "arcane_power", find_spell( 12042 ) )
                                 ->set_cooldown( 0_ms )
                                 ->set_default_value_from_effect( 1 )
                                 ->modify_default_value( talents.overpowered->effectN( 1 ).percent() )
                                 ->modify_duration( spec.arcane_power_3->effectN( 1 ).time_value() );
  buffs.clearcasting         = make_buff<buffs::expanded_potential_buff_t>( this, "clearcasting", find_spell( 263725 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->modify_max_stack( as<int>( spec.clearcasting_3->effectN( 1 ).base_value() ) )
                                 ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                  // Nether Precision activates when all stacks of Clearcasting expire, regardless of how they expire.
                                   { if ( cur == 0 ) buffs.nether_precision->trigger( buffs.nether_precision->max_stack() ); } );
  buffs.clearcasting_channel = make_buff( this, "clearcasting_channel", find_spell( 277726 ) )
                                 ->set_quiet( true );
  buffs.evocation            = make_buff( this, "evocation", find_spell( 12051 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_cooldown( 0_ms )
                                 ->set_affects_regen( true );
  buffs.presence_of_mind     = make_buff( this, "presence_of_mind", find_spell( 205025 ) )
                                 ->set_cooldown( 0_ms )
                                 ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                   { if ( cur == 0 ) cooldowns.presence_of_mind->start( cooldowns.presence_of_mind->action ); } )
                                 ->modify_max_stack( as<int>( spec.presence_of_mind_2->effectN( 1 ).base_value() ) );

  buffs.arcane_familiar      = make_buff( this, "arcane_familiar", find_spell( 210126 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_period( 3.0_s )
                                 ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
                                 ->set_tick_callback( [ this ] ( buff_t*, int, timespan_t )
                                   {
                                     action.arcane_assault->set_target( target );
                                     action.arcane_assault->execute();
                                   } )
                                 ->set_stack_change_callback( [ this ] ( buff_t*, int, int )
                                   { recalculate_resource_max( RESOURCE_MANA ); } );
  buffs.chrono_shift         = make_buff( this, "chrono_shift", find_spell( 236298 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->add_invalidate( CACHE_RUN_SPEED )
                                 ->set_chance( talents.chrono_shift->ok() );
  buffs.enlightened_damage   = make_buff( this, "enlightened_damage", find_spell( 321388 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.enlightened_mana     = make_buff( this, "enlightened_mana", find_spell( 321390 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_affects_regen( true );
  buffs.rule_of_threes       = make_buff( this, "rule_of_threes", find_spell( 264774 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_chance( talents.rule_of_threes->ok() );
  buffs.time_warp            = make_buff( this, "time_warp", find_spell( 342242 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );


  // Fire
  buffs.combustion        = make_buff<buffs::combustion_buff_t>( this );
  buffs.fireball          = make_buff( this, "fireball", find_spell( 157644 ) )
                              ->set_chance( spec.fireball_2->ok() )
                              ->set_default_value_from_effect( 1 )
                              ->set_stack_change_callback( [ this ] ( buff_t*, int old, int cur )
                                {
                                  if ( cur > old )
                                  {
                                    buffs.flames_of_alacrity->trigger( cur - old );
                                    buffs.flame_accretion->trigger( cur - old );
                                  }
                                  else
                                  {
                                    buffs.flames_of_alacrity->decrement( old - cur );
                                    buffs.flame_accretion->decrement( old - cur );
                                  }
                                } );
  buffs.heating_up        = make_buff( this, "heating_up", find_spell( 48107 ) );
  buffs.hot_streak        = make_buff<buffs::expanded_potential_buff_t>( this, "hot_streak", find_spell( 48108 ) )
                              ->set_stack_change_callback( [ this ] ( buff_t*, int old, int )
                                { if ( old == 0 ) buffs.firestorm->trigger(); } );

  buffs.alexstraszas_fury = make_buff( this, "alexstraszas_fury", find_spell( 334277 ) )
                              ->set_default_value_from_effect( 1 )
                              ->set_chance( talents.alexstraszas_fury->ok() );
  buffs.frenetic_speed    = make_buff( this, "frenetic_speed", find_spell( 236060 ) )
                              ->set_default_value_from_effect( 1 )
                              ->add_invalidate( CACHE_RUN_SPEED )
                              ->set_chance( talents.frenetic_speed->ok() );
  buffs.pyroclasm         = make_buff( this, "pyroclasm", find_spell( 269651 ) )
                              ->set_default_value_from_effect( 1 )
                              ->set_chance( talents.pyroclasm->effectN( 1 ).percent() );


  // Frost
  buffs.brain_freeze     = make_buff<buffs::expanded_potential_buff_t>( this, "brain_freeze", find_spell( 190446 ) );
  buffs.fingers_of_frost = make_buff( this, "fingers_of_frost", find_spell( 44544 ) );
  buffs.icicles          = make_buff( this, "icicles", find_spell( 205473 ) );
  buffs.icy_veins        = make_buff<buffs::icy_veins_buff_t>( this );

  buffs.bone_chilling    = make_buff( this, "bone_chilling", find_spell( 205766 ) )
                             ->set_default_value( 0.1 * talents.bone_chilling->effectN( 1 ).percent() )
                             ->set_chance( talents.bone_chilling->ok() );
  buffs.chain_reaction   = make_buff( this, "chain_reaction", find_spell( 278310 ) )
                             ->set_default_value_from_effect( 1 )
                             ->set_chance( talents.chain_reaction->ok() );
  buffs.freezing_rain    = make_buff( this, "freezing_rain", find_spell( 270232 ) )
                             ->set_default_value_from_effect( 2 )
                             ->set_chance( talents.freezing_rain->ok() );
  buffs.ice_floes        = make_buff<buffs::ice_floes_buff_t>( this );
  buffs.ray_of_frost     = make_buff( this, "ray_of_frost", find_spell( 208141 ) )
                             ->set_default_value_from_effect( 1 );


  // Shared
  buffs.incanters_flow   = make_buff<buffs::incanters_flow_t>( this );
  buffs.rune_of_power    = make_buff( this, "rune_of_power", find_spell( 116014 ) )
                             ->set_default_value_from_effect( 1 )
                             ->set_chance( talents.rune_of_power->ok() );
  buffs.focus_magic_crit = make_buff( this, "focus_magic_crit", find_spell( 321363 ) )
                             ->set_default_value_from_effect( 1 )
                             ->add_invalidate( CACHE_SPELL_CRIT_CHANCE );
  buffs.focus_magic_int  = make_buff( this, "focus_magic_int", find_spell( 334180 ) )
                             ->set_default_value_from_effect( 1 )
                             ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );

  // Azerite
  buffs.arcane_pummeling   = make_buff( this, "arcane_pummeling", find_spell( 270670 ) )
                               ->set_default_value( azerite.arcane_pummeling.value() )
                               ->set_chance( azerite.arcane_pummeling.enabled() );
  buffs.brain_storm        = make_buff<stat_buff_t>( this, "brain_storm", find_spell( 273330 ) )
                               ->add_stat( STAT_INTELLECT, azerite.brain_storm.value() )
                               ->set_chance( azerite.brain_storm.enabled() );

  buffs.blaster_master     = make_buff<stat_buff_t>( this, "blaster_master", find_spell( 274598 ) )
                               ->add_stat( STAT_MASTERY_RATING, azerite.blaster_master.value() )
                               ->set_chance( azerite.blaster_master.enabled() );
  buffs.firemind           = make_buff<stat_buff_t>( this, "firemind", find_spell( 279715 ) )
                               ->add_stat( STAT_INTELLECT, azerite.firemind.value() )
                               ->set_chance( azerite.firemind.enabled() );
  buffs.flames_of_alacrity = make_buff<stat_buff_t>( this, "flames_of_alacrity", find_spell( 272934 ) )
                               ->add_stat( STAT_HASTE_RATING, azerite.flames_of_alacrity.value() )
                               ->set_chance( azerite.flames_of_alacrity.enabled() );
  buffs.wildfire           = make_buff<stat_buff_t>( this, "wildfire", find_spell( 288800 ) )
                               ->set_chance( azerite.wildfire.enabled() );

  proc_t* fg_fof = get_proc( "Fingers of Frost from Frigid Grasp" );
  buffs.frigid_grasp       = make_buff<stat_buff_t>( this, "frigid_grasp", find_spell( 279684 ) )
                               ->add_stat( STAT_INTELLECT, azerite.frigid_grasp.value() )
                               ->set_stack_change_callback( [ this, fg_fof ] ( buff_t*, int old, int )
                                 { if ( old == 0 ) trigger_fof( 1.0, 1, fg_fof ); } )
                               ->set_chance( azerite.frigid_grasp.enabled() );

  buffs.tunnel_of_ice      = make_buff( this, "tunnel_of_ice", find_spell( 277904 ) )
                               ->set_default_value( azerite.tunnel_of_ice.value() )
                               ->set_chance( azerite.tunnel_of_ice.enabled() );

  // Runeforge Legendaries
  buffs.arcane_harmony = make_buff( this, "arcane_harmony", find_spell( 332777 ) )
                           ->set_default_value_from_effect( 1 )
                           ->set_chance( runeforge.arcane_harmony.ok() );
  buffs.siphon_storm   = make_buff( this, "siphon_storm", find_spell( 332934 ) )
                           ->set_default_value_from_effect( 1 )
                           ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                           ->set_chance( runeforge.siphon_storm.ok() );
  buffs.temporal_warp  = make_buff( this, "temporal_warp", find_spell( 327355 ) )
                           ->set_default_value_from_effect( 1 )
                           ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                           ->set_chance( runeforge.temporal_warp.ok() );

  buffs.fevered_incantation      = make_buff( this, "fevered_incantation", find_spell( 333049 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_chance( runeforge.fevered_incantation.ok() );
  buffs.firestorm                = make_buff( this, "firestorm", find_spell( 333100 ) )
                                     ->set_default_value_from_effect( 2 )
                                     ->set_trigger_spell( runeforge.firestorm );
  buffs.molten_skyfall           = make_buff( this, "molten_skyfall", find_spell( 333170 ) )
                                     ->set_chance( runeforge.molten_skyfall.ok() );
  buffs.molten_skyfall_ready     = make_buff( this, "molten_skyfall_ready", find_spell( 333182 ) );
  buffs.sun_kings_blessing       = make_buff( this, "sun_kings_blessing", find_spell( 333314 ) )
                                     ->set_chance( runeforge.sun_kings_blessing.ok() );
  buffs.sun_kings_blessing_ready = make_buff( this, "sun_kings_blessing_ready", find_spell( 333315 ) );

  buffs.cold_front       = make_buff( this, "cold_front", find_spell( 327327 ) )
                             ->set_chance( runeforge.cold_front.ok() );
  buffs.cold_front_ready = make_buff( this, "cold_front_ready", find_spell( 327330 ) );
  proc_t* fw_fof = get_proc( "Fingers of Frost from Freezing Winds" );
  buffs.freezing_winds   = make_buff( this, "freezing_winds", find_spell( 327478 ) )
                             ->set_tick_callback( [ this, fw_fof ] ( buff_t*, int, timespan_t )
                               { trigger_fof( 1.0, 1, fw_fof ); } )
                             ->set_chance( runeforge.freezing_winds.ok() );
  buffs.slick_ice        = make_buff( this, "slick_ice", find_spell( 327509 ) )
                             ->set_default_value_from_effect( 1 )
                             ->set_chance( runeforge.slick_ice.ok() );

  buffs.disciplinary_command        = make_buff( this, "disciplinary_command", find_spell( 327371 ) )
                                        ->set_default_value_from_effect( 1 );
  buffs.disciplinary_command_arcane = make_buff( this, "disciplinary_command_arcane", find_spell( 327369  ) )
                                        ->set_quiet( true )
                                        ->set_chance( runeforge.disciplinary_command.ok() );
  buffs.disciplinary_command_frost  = make_buff( this, "disciplinary_command_frost", find_spell( 327366 ) )
                                        ->set_quiet( true )
                                        ->set_chance( runeforge.disciplinary_command.ok() );
  buffs.disciplinary_command_fire   = make_buff( this, "disciplinary_command_fire", find_spell( 327368 ) )
                                        ->set_quiet( true )
                                        ->set_chance( runeforge.disciplinary_command.ok() );
  buffs.expanded_potential          = make_buff( this, "expanded_potential", find_spell( 327495 ) )
                                        ->set_trigger_spell( runeforge.expanded_potential );

  // Covenant Abilities
  buffs.deathborne = make_buff( this, "deathborne", find_spell( 324220 ) )
                       ->set_default_value_from_effect( 2 )
                       ->modify_duration( conduits.gift_of_the_lich.time_value() );


  // Soulbind Conduits
  buffs.nether_precision = make_buff( this, "nether_precision", find_spell( 336889 ) )
                             ->set_default_value( conduits.nether_precision.percent() )
                             ->set_chance( conduits.nether_precision.ok() );

  buffs.flame_accretion  = make_buff( this, "flame_accretion", find_spell( 157644 ) )
                             ->set_default_value( conduits.flame_accretion.value() )
                             ->set_chance( conduits.flame_accretion.ok() )
                             ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
  buffs.infernal_cascade = make_buff( this, "infernal_cascade", find_spell( 336832 ) )
                             ->set_default_value( conduits.infernal_cascade.percent() )
                             ->set_chance( conduits.infernal_cascade.ok() )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.siphoned_malice = make_buff( this, "siphoned_malice", find_spell( 337090 ) )
                             ->set_default_value( conduits.siphoned_malice.percent() )
                             ->set_chance( conduits.siphoned_malice.ok() );

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      player_t::buffs.memory_of_lucid_dreams->set_affects_regen( true );
      break;
    case MAGE_FIRE:
      player_t::buffs.memory_of_lucid_dreams->set_stack_change_callback( [ this ] ( buff_t*, int, int )
      { cooldowns.fire_blast->adjust_recharge_multiplier(); } );
      break;
    default:
      break;
  }
}

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation          = get_gain( "Evocation"          );
  gains.lucid_dreams       = get_gain( "Lucid Dreams"       );
  gains.mana_gem           = get_gain( "Mana Gem"           );
  gains.arcane_barrage     = get_gain( "Arcane Barrage"     );
  gains.mirrors_of_torment = get_gain( "Mirrors of Torment" );
}

void mage_t::init_procs()
{
  player_t::init_procs();

  switch ( specialization() )
  {
    case MAGE_FIRE:
      procs.heating_up_generated         = get_proc( "Heating Up generated" );
      procs.heating_up_removed           = get_proc( "Heating Up removed" );
      procs.heating_up_ib_converted      = get_proc( "Heating Up converted with Fire Blast" );
      procs.hot_streak                   = get_proc( "Hot Streak procs" );
      procs.hot_streak_pyromaniac        = get_proc( "Hot Streak procs from Pyromaniac" );
      procs.hot_streak_spell             = get_proc( "Hot Streak spells used" );
      procs.hot_streak_spell_crit        = get_proc( "Hot Streak spell crits" );
      procs.hot_streak_spell_crit_wasted = get_proc( "Hot Streak spell crits wasted" );

      procs.ignite_applied    = get_proc( "Direct Ignite applications" );
      procs.ignite_new_spread = get_proc( "Ignites spread to new targets" );
      procs.ignite_overwrite  = get_proc( "Ignites spread to targets with existing Ignite" );
      break;
    case MAGE_FROST:
      procs.brain_freeze            = get_proc( "Brain Freeze" );
      procs.brain_freeze_mirrors    = get_proc( "Brain Freeze from Mirrors of Torment" );
      procs.brain_freeze_used       = get_proc( "Brain Freeze used" );
      procs.fingers_of_frost        = get_proc( "Fingers of Frost" );
      procs.fingers_of_frost_wasted = get_proc( "Fingers of Frost wasted due to Winter's Chill" );
      procs.winters_chill_applied   = get_proc( "Winter's Chill stacks applied" );
      procs.winters_chill_consumed  = get_proc( "Winter's Chill stacks consumed" );
      break;
    default:
      break;
  }
}

void mage_t::init_resources( bool force )
{
  player_t::init_resources( force );

  // This is the call needed to set max mana at the beginning of the sim.
  // If this is called without recalculating max mana afterwards, it will
  // overwrite the recalculating done earlier in cache_invalidate() back
  // to default max mana.
  if ( spec.savant->ok() )
    recalculate_resource_max( RESOURCE_MANA );
}

void mage_t::init_benefits()
{
  player_t::init_benefits();

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      benefits.arcane_charge.arcane_barrage = std::make_unique<buff_stack_benefit_t>( buffs.arcane_charge, "Arcane Barrage" );
      benefits.arcane_charge.arcane_blast   = std::make_unique<buff_stack_benefit_t>( buffs.arcane_charge, "Arcane Blast" );
      if ( talents.nether_tempest->ok() )
        benefits.arcane_charge.nether_tempest = std::make_unique<buff_stack_benefit_t>( buffs.arcane_charge, "Nether Tempest" );
      break;
    case MAGE_FIRE:
      if ( azerite.blaster_master.enabled() )
      {
        benefits.blaster_master.combustion = std::make_unique<buff_stack_benefit_t>( buffs.blaster_master, "Combustion" );
        if ( talents.rune_of_power->ok() )
          benefits.blaster_master.rune_of_power = std::make_unique<buff_stack_benefit_t>( buffs.blaster_master, "Rune of Power" );
        if ( talents.searing_touch->ok() )
          benefits.blaster_master.searing_touch = std::make_unique<buff_stack_benefit_t>( buffs.blaster_master, "Searing Touch" );
      }
      break;
    default:
      break;
  }
}

void mage_t::init_uptimes()
{
  player_t::init_uptimes();

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      uptime.burn_phase     = get_uptime( "Burn Phase" );
      uptime.conserve_phase = get_uptime( "Conserve Phase" );

      sample_data.burn_duration_history = std::make_unique<extended_sample_data_t>( "Burn duration history", false );
      sample_data.burn_initial_mana     = std::make_unique<extended_sample_data_t>( "Burn initial mana", false );
      break;
    case MAGE_FROST:
      if ( talents.thermal_void->ok() )
        sample_data.icy_veins_duration = std::make_unique<extended_sample_data_t>( "Icy Veins duration", false );
      break;
    default:
      break;
  }
}

void mage_t::init_rng()
{
  player_t::init_rng();

  // TODO: There's no data about this in game. Keep an eye out in case Blizzard
  // changes this behind the scenes.
  shuffled_rng.time_anomaly = get_shuffled_rng( "time_anomaly", 1, 16 );
}

void mage_t::init_finished()
{
  player_t::init_finished();

  // Sort the procs to put the proc sources next to each other.
  if ( specialization() == MAGE_FROST )
    range::sort( proc_list, [] ( proc_t* a, proc_t* b ) { return a->name_str < b->name_str; } );
}

void mage_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
      case MAGE_ARCANE:
        apl_arcane();
        break;
      case MAGE_FIRE:
        apl_fire();
        break;
      case MAGE_FROST:
        apl_frost();
        break;
      default:
        break;
    }

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

std::string mage_t::default_potion() const
{
  return "disabled"; // TODO
}

std::string mage_t::default_flask() const
{
  return "disabled"; // TODO
}

std::string mage_t::default_food() const
{
  return "disabled"; // TODO
}

std::string mage_t::default_rune() const
{
  return "disabled";
}

void mage_t::apl_arcane()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* default_ = get_action_priority_list( "default" );
  action_priority_list_t* essences = get_action_priority_list( "essences" );
  action_priority_list_t* opener = get_action_priority_list( "opener" );
  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );
  action_priority_list_t* rotation = get_action_priority_list( "rotation" );
  action_priority_list_t* final_burn = get_action_priority_list( "final_burn" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );
  action_priority_list_t* movement = get_action_priority_list( "movement" );

  precombat->add_action( "variable,name=prepull_evo,op=set,value=0" );
  precombat->add_action( "variable,name=prepull_evo,op=set,value=1,if=runeforge.siphon_storm.equipped&active_enemies>2" );
  precombat->add_action( "variable,name=prepull_evo,op=set,value=1,if=runeforge.siphon_storm.equipped&covenant.necrolord.enabled&active_enemies>1" );
  precombat->add_action( "variable,name=prepull_evo,op=set,value=1,if=runeforge.siphon_storm.equipped&covenant.night_fae.enabled" );
  precombat->add_action( "variable,name=have_opened,op=set,value=0" );
  precombat->add_action( "variable,name=have_opened,op=set,value=1,if=active_enemies>2" );
  precombat->add_action( "variable,name=have_opened,op=set,value=1,if=variable.prepull_evo=1" );
  precombat->add_action( "variable,name=final_burn,op=set,value=0" );
  precombat->add_action( "variable,name=rs_max_delay,op=set,value=5" );
  precombat->add_action( "variable,name=ap_max_delay,op=set,value=10" );
  precombat->add_action( "variable,name=rop_max_delay,op=set,value=20" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=5" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=3,if=runeforge.disciplinary_command.equipped" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=15,if=covenant.night_fae.enabled" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=15,if=conduit.arcane_prodigy.enabled&active_enemies<3" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=30,if=essence.vision_of_perfection.minor" );
  precombat->add_action( "variable,name=barrage_mana_pct,op=set,value=90" );
  precombat->add_action( "variable,name=barrage_mana_pct,op=set,value=80,if=covenant.night_fae.enabled" );
  precombat->add_action( "variable,name=ap_minimum_mana_pct,op=set,value=30" );
  precombat->add_action( "variable,name=ap_minimum_mana_pct,op=set,value=50,if=runeforge.disciplinary_command.equipped" );
  precombat->add_action( "variable,name=ap_minimum_mana_pct,op=set,value=50,if=runeforge.grisly_icicle.equipped" );
  precombat->add_action( "variable,name=aoe_totm_charges,op=set,value=2" );
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_familiar" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "conjure_mana_gem" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "potion" );
  precombat->add_action( "frostbolt,if=variable.prepull_evo=0" );
  precombat->add_action( "evocation,if=variable.prepull_evo=1" );

  default_->add_action( "counterspell,if=target.debuff.casting.react" );
  default_->add_action( "call_action_list,name=essences" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "call_action_list,name=opener,if=variable.have_opened=0" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=rotation,if=variable.final_burn=0" );
  default_->add_action( "call_action_list,name=final_burn,if=variable.final_burn=1" );
  default_->add_action( "call_action_list,name=movement" );

  essences->add_action( "blood_of_the_enemy,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=2&cooldown.arcane_power.remains<=gcd|target.time_to_die<cooldown.arcane_power.remains" );
  essences->add_action( "blood_of_the_enemy,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  essences->add_action( "worldvein_resonance,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=2&cooldown.arcane_power.remains<=gcd|target.time_to_die<cooldown.arcane_power.remains" );
  essences->add_action( "worldvein_resonance,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  essences->add_action( "guardian_of_azeroth,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=2&cooldown.arcane_power.remains<=gcd|target.time_to_die<cooldown.arcane_power.remains" );
  essences->add_action( "guardian_of_azeroth,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  essences->add_action( "concentrated_flame,line_cd=6,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&mana.time_to_max>=execute_time" );
  essences->add_action( "reaping_flames,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&mana.time_to_max>=execute_time" );
  essences->add_action( "focused_azerite_beam,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  essences->add_action( "purifying_blast,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  essences->add_action( "ripple_in_space,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  essences->add_action( "the_unbound_force,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  essences->add_action( "memory_of_lucid_dreams,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );

  opener->add_action( "variable,name=have_opened,op=set,value=1,if=prev_gcd.1.evocation" );
  opener->add_action( "lights_judgment,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  opener->add_action( "bag_of_tricks,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  opener->add_action( "use_items,if=buff.arcane_power.up" );
  opener->add_action( "berserking,if=buff.arcane_power.up" );
  opener->add_action( "blood_fury,if=buff.arcane_power.up" );
  opener->add_action( "fireblood,if=buff.arcane_power.up" );
  opener->add_action( "ancestral_call,if=buff.arcane_power.up" );
  opener->add_action( "fire_blast,if=runeforge.disciplinary_command.equipped&buff.disciplinary_command_frost.up" );
  opener->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&mana.pct>95" );
  opener->add_action( "mirrors_of_torment" );
  opener->add_action( "deathborne" );
  opener->add_action( "radiant_spark,if=mana.pct>40" );
  opener->add_action( "cancel_action,if=action.shifting_power.channeling" );
  opener->add_action( "shifting_power,if=soulbind.field_of_blossoms.enabled" );
  opener->add_action( "touch_of_the_magi" );
  opener->add_action( "arcane_power" );
  opener->add_action( "rune_of_power,if=buff.rune_of_power.down" );
  opener->add_action( "use_mana_gem,if=(talent.enlightened.enabled&mana.pct<=80&mana.pct>=65)|(!talent.enlightened.enabled&mana.pct<=85)" );
  opener->add_action( "berserking,if=buff.arcane_power.up" );
  opener->add_action( "time_warp,if=runeforge.temporal_warp.equipped" );
  opener->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=buff.presence_of_mind.max_stack*action.arcane_blast.execute_time" );
  opener->add_action( "arcane_blast,if=dot.radiant_spark.remains>5|debuff.radiant_spark_vulnerability.stack>0" );
  opener->add_action( "arcane_blast,if=buff.presence_of_mind.up&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=action.arcane_blast.execute_time" );
  opener->add_action( "arcane_barrage,if=buff.arcane_power.up&buff.arcane_power.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  opener->add_action( "arcane_missiles,if=debuff.touch_of_the_magi.up&talent.arcane_echo.enabled&buff.deathborne.down&debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time,chain=1" );
  opener->add_action( "arcane_missiles,if=buff.clearcasting.react,chain=1" );
  opener->add_action( "arcane_orb,if=buff.arcane_charge.stack<=2&(cooldown.arcane_power.remains>10|active_enemies<=2)" );
  opener->add_action( "arcane_blast,if=buff.rune_of_power.up|mana.pct>15" );
  opener->add_action( "evocation,if=buff.rune_of_power.down,interrupt_if=mana.pct>=85,interrupt_immediate=1" );
  opener->add_action( "arcane_barrage" );

  cooldowns->add_action( "lights_judgment,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  cooldowns->add_action( "bag_of_tricks,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  cooldowns->add_action( "use_items,if=buff.arcane_power.up" );
  cooldowns->add_action( "berserking,if=buff.arcane_power.up" );
  cooldowns->add_action( "blood_fury,if=buff.arcane_power.up" );
  cooldowns->add_action( "fireblood,if=buff.arcane_power.up" );
  cooldowns->add_action( "ancestral_call,if=buff.arcane_power.up" );
  cooldowns->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&cooldown.arcane_power.remains>30&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=2&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  cooldowns->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "frostbolt,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&(buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=2&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  cooldowns->add_action( "fire_blast,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down&prev_gcd.1.frostbolt" );
  cooldowns->add_action( "mirrors_of_torment,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=2&cooldown.arcane_power.remains<=gcd" );
  cooldowns->add_action( "mirrors_of_torment,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "deathborne,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=2&cooldown.arcane_power.remains<=gcd" );
  cooldowns->add_action( "deathborne,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains>variable.rs_max_delay&cooldown.arcane_power.remains>variable.rs_max_delay&(talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd|talent.rune_of_power.enabled&cooldown.rune_of_power.remains>variable.rs_max_delay|!talent.rune_of_power.enabled)&buff.arcane_charge.stack>2&debuff.touch_of_the_magi.down" );
  cooldowns->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=2&cooldown.arcane_power.remains<=gcd" );
  cooldowns->add_action( "radiant_spark,if=cooldown.arcane_power.remains=0&((!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct)" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=2&talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay&covenant.kyrian.enabled&cooldown.radiant_spark.remains<=8" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=2&talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay&!covenant.kyrian.enabled" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=2&!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=2&cooldown.arcane_power.remains<=gcd" );
  cooldowns->add_action( "arcane_power,if=(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "rune_of_power,if=buff.rune_of_power.down&cooldown.touch_of_the_magi.remains>variable.rop_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack&(cooldown.arcane_power.remains>15|debuff.touch_of_the_magi.up)" );
  cooldowns->add_action( "presence_of_mind,if=buff.arcane_charge.stack=0&covenant.kyrian.enabled" );
  cooldowns->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.up&!covenant.kyrian.enabled" );
  cooldowns->add_action( "use_mana_gem,if=cooldown.evocation.remains>0&((talent.enlightened.enabled&mana.pct<=80&mana.pct>=65)|(!talent.enlightened.enabled&mana.pct<=85))" );

  rotation->add_action( "variable,name=final_burn,op=set,value=1,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&!buff.rule_of_threes.up&target.time_to_die<=((mana%action.arcane_blast.cost)*action.arcane_blast.execute_time)" );
  rotation->add_action( "arcane_barrage,if=debuff.radiant_spark_vulnerability.stack=debuff.radiant_spark_vulnerability.max_stack&(buff.rune_of_power.down|buff.rune_of_power.remains<=gcd)" );
  rotation->add_action( "arcane_blast,if=dot.radiant_spark.remains>5|debuff.radiant_spark_vulnerability.stack>0" );
  rotation->add_action( "arcane_blast,if=buff.presence_of_mind.up&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=action.arcane_blast.execute_time" );
  rotation->add_action( "arcane_missiles,if=debuff.touch_of_the_magi.up&talent.arcane_echo.enabled&buff.deathborne.down&(debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time|cooldown.presence_of_mind.remains>0|covenant.kyrian.enabled),chain=1" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.expanded_potential.up" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&(buff.arcane_power.up|buff.rune_of_power.up|debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time),chain=1" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.clearcasting.stack=buff.clearcasting.max_stack,chain=1" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.clearcasting.remains<=((buff.clearcasting.stack*action.arcane_missiles.execute_time)+gcd),chain=1" );
  rotation->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.arcane_power.down&debuff.touch_of_the_magi.down" );
  rotation->add_action( "arcane_orb,if=buff.arcane_charge.stack<=2" );
  rotation->add_action( "supernova,if=mana.pct<=95&buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  rotation->add_action( "shifting_power,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>0&cooldown.arcane_power.remains>0&cooldown.touch_of_the_magi.remains>0&(!talent.rune_of_power.enabled|(talent.rune_of_power.enabled&cooldown.rune_of_power.remains>0))" );
  rotation->add_action( "arcane_blast,if=buff.rule_of_threes.up&buff.arcane_charge.stack>3" );
  rotation->add_action( "arcane_barrage,if=mana.pct<variable.barrage_mana_pct&cooldown.evocation.remains>0&buff.arcane_power.down&buff.arcane_charge.stack=buff.arcane_charge.max_stack&essence.vision_of_perfection.minor" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.remains=0&(cooldown.rune_of_power.remains=0|cooldown.arcane_power.remains=0)&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation->add_action( "arcane_barrage,if=mana.pct<=variable.barrage_mana_pct&buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&buff.arcane_charge.stack=buff.arcane_charge.max_stack&cooldown.evocation.remains>0" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&buff.arcane_charge.stack=buff.arcane_charge.max_stack&talent.arcane_orb.enabled&cooldown.arcane_orb.remains<=gcd&mana.pct<=90&cooldown.evocation.remains>0" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_power.up&buff.arcane_power.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation->add_action( "arcane_barrage,if=buff.rune_of_power.up&buff.rune_of_power.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation->add_action( "arcane_blast" );
  rotation->add_action( "evocation,interrupt_if=mana.pct>=85,interrupt_immediate=1" );
  rotation->add_action( "arcane_barrage" );

  final_burn->add_action( "arcane_missiles,if=buff.clearcasting.react,chain=1" );
  final_burn->add_action( "arcane_blast" );
  final_burn->add_action( "arcane_barrage" );

  aoe->add_action( "use_mana_gem,if=(talent.enlightened.enabled&mana.pct<=80&mana.pct>=65)|(!talent.enlightened.enabled&mana.pct<=85)" );
  aoe->add_action( "lights_judgment,if=buff.arcane_power.down" );
  aoe->add_action( "bag_of_tricks,if=buff.arcane_power.down" );
  aoe->add_action( "use_items,if=buff.arcane_power.up" );
  aoe->add_action( "berserking,if=buff.arcane_power.up" );
  aoe->add_action( "blood_fury,if=buff.arcane_power.up" );
  aoe->add_action( "fireblood,if=buff.arcane_power.up" );
  aoe->add_action( "ancestral_call,if=buff.arcane_power.up" );
  aoe->add_action( "time_warp,if=runeforge.temporal_warp.equipped" );
  aoe->add_action( "frostbolt,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&(buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "fire_blast,if=(runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down&prev_gcd.1.frostbolt)|(runeforge.disciplinary_command.equipped&time=0)" );
  aoe->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&cooldown.arcane_power.remains>30&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "arcane_power,if=runeforge.siphon_storm.equipped&prev_gcd.1.evocation" );
  aoe->add_action( "evocation,if=time>30&runeforge.siphon_storm.equipped&cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_charges))&buff.rune_of_power.down),interrupt_if=buff.siphon_storm.stack=buff.siphon_storm.max_stack,interrupt_immediate=1" );
  aoe->add_action( "mirrors_of_torment,if=(cooldown.arcane_power.remains>45|cooldown.arcane_power.remains<=3)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>5)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>5)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains>variable.rs_max_delay&cooldown.arcane_power.remains>variable.rs_max_delay&(talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd|talent.rune_of_power.enabled&cooldown.rune_of_power.remains>variable.rs_max_delay|!talent.rune_of_power.enabled)&buff.arcane_charge.stack<=variable.aoe_totm_charges&debuff.touch_of_the_magi.down" );
  aoe->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "radiant_spark,if=cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "deathborne,if=cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.aoe_totm_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd)" );
  aoe->add_action( "arcane_power,if=((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_charges))&buff.rune_of_power.down" );
  aoe->add_action( "rune_of_power,if=buff.rune_of_power.down&((cooldown.touch_of_the_magi.remains>20&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_charges))&(cooldown.arcane_power.remains>15|debuff.touch_of_the_magi.up)" );
  aoe->add_action( "presence_of_mind,if=buff.deathborne.up&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=buff.presence_of_mind.max_stack*action.arcane_blast.execute_time" );
  aoe->add_action( "arcane_blast,if=buff.deathborne.up&((talent.resonance.enabled&active_enemies<4)|active_enemies<5)" );
  aoe->add_action( "supernova" );
  aoe->add_action( "arcane_orb,if=buff.arcane_charge.stack=0" );
  aoe->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  aoe->add_action( "shifting_power,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&cooldown.arcane_power.remains>0&cooldown.touch_of_the_magi.remains>0&(!talent.rune_of_power.enabled|(talent.rune_of_power.enabled&cooldown.rune_of_power.remains>0))" );
  aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&runeforge.arcane_infinity.equipped&talent.amplification.enabled&active_enemies<4" );
  aoe->add_action( "arcane_explosion,if=buff.arcane_charge.stack<buff.arcane_charge.max_stack" );
  aoe->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  aoe->add_action( "evocation,interrupt_if=mana.pct>=85,interrupt_immediate=1" );

  movement->add_action( "shimmer,if=movement.distance>=10" );
  movement->add_action( "presence_of_mind" );
  movement->add_action( "arcane_missiles,if=movement.distance<10" );
  movement->add_action( "arcane_orb" );
}

void mage_t::apl_fire()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* default_ = get_action_priority_list( "default" );
  action_priority_list_t* active_talents = get_action_priority_list( "active_talents" );
  action_priority_list_t* rop_phase = get_action_priority_list( "rop_phase" );
  action_priority_list_t* combustion_phase = get_action_priority_list( "combustion_phase" );
  action_priority_list_t* standard_rotation = get_action_priority_list( "standard_rotation" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=disable_combustion,op=reset" );
  precombat->add_action( "variable,name=hot_streak_flamestrike,op=set,if=variable.hot_streak_flamestrike=0,value=2*talent.flame_patch.enabled+3*!talent.flame_patch.enabled" );
  precombat->add_action( "variable,name=hard_cast_flamestrike,op=set,if=variable.hard_cast_flamestrike=0,value=2*talent.flame_patch.enabled+3*!talent.flame_patch.enabled" );
  precombat->add_action( "variable,name=combustion_flamestrike,op=set,if=variable.combustion_flamestrike=0,value=3*talent.flame_patch.enabled+6*!talent.flame_patch.enabled" );
  precombat->add_action( "variable,name=delay_flamestrike,default=0,op=reset" );
  precombat->add_action( "variable,name=kindling_reduction,default=0.2,op=reset" );
  precombat->add_action( "variable,name=shifting_power_reduction,op=set,value=action.shifting_power.cast_time%action.shifting_power.tick_time*3,if=covenant.night_fae.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "potion" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "counterspell,if=!runeforge.disciplinary_command.equipped" );
  default_->add_action( "variable,name=time_to_combustion,op=set,value=talent.firestarter.enabled*firestarter.remains+(cooldown.combustion.remains*(1-variable.kindling_reduction*talent.kindling.enabled))*!cooldown.combustion.ready*buff.combustion.down" );
  default_->add_action( "cycling_variable,name=ignite_min,op=min,value=dot.ignite.tick_dmg" );
  default_->add_action( "shifting_power,if=buff.combustion.down&buff.rune_of_power.down&cooldown.combustion.remains>0&(cooldown.rune_of_power.remains>0|!talent.rune_of_power.enabled)" );
  default_->add_action( "radiant_spark,if=(buff.combustion.down&buff.rune_of_power.down&(cooldown.combustion.remains<execute_time|cooldown.combustion.remains>cooldown.radiant_spark.duration))|(buff.rune_of_power.up&cooldown.combustion.remains>30)" );
  default_->add_action( "deathborne,if=buff.combustion.down&buff.rune_of_power.down&cooldown.combustion.remains<execute_time" );
  default_->add_action( "mirror_image,if=buff.combustion.down&debuff.radiant_spark_vulnerability.down" );
  default_->add_action( "counterspell,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_arcane.down&cooldown.combustion.remains>30&!buff.disciplinary_command.up" );
  default_->add_action( "arcane_explosion,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_arcane.down&cooldown.combustion.remains>30&!buff.disciplinary_command.up" );
  default_->add_action( "frostbolt,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&cooldown.combustion.remains>30&!buff.disciplinary_command.up" );
  default_->add_action( "rune_of_power,if=buff.rune_of_power.down&(variable.time_to_combustion>buff.rune_of_power.duration&variable.time_to_combustion>action.fire_blast.full_recharge_time|variable.time_to_combustion>target.time_to_die|variable.disable_combustion)" );
  default_->add_action( "call_action_list,name=combustion_phase,if=!variable.disable_combustion&variable.time_to_combustion<=0" );
  default_->add_action( "variable,name=fire_blast_pooling,value=!variable.disable_combustion&variable.time_to_combustion<action.fire_blast.full_recharge_time-variable.shifting_power_reduction*(cooldown.shifting_power.remains<variable.time_to_combustion)&variable.time_to_combustion<target.time_to_die|runeforge.sun_kings_blessing.equipped&action.fire_blast.charges_fractional<action.fire_blast.max_charges-0.5&(cooldown.shifting_power.remains>15|!covenant.night_fae.enabled)" );
  default_->add_action( "call_action_list,name=rop_phase,if=buff.rune_of_power.up&(variable.time_to_combustion>0|variable.disable_combustion)" );
  default_->add_action( "variable,name=phoenix_pooling,value=!variable.disable_combustion&variable.time_to_combustion<action.phoenix_flames.full_recharge_time&variable.time_to_combustion<target.time_to_die|runeforge.sun_kings_blessing.equipped" );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&(variable.time_to_combustion>0|variable.disable_combustion)&(active_enemies>=variable.hard_cast_flamestrike&(time-buff.combustion.last_expire>variable.delay_flamestrike|variable.disable_combustion))&!firestarter.active&!buff.hot_streak.react" );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=firestarter.active&charges>=1&!variable.fire_blast_pooling&(!action.fireball.executing&!action.pyroblast.in_flight&buff.heating_up.react|action.fireball.executing&!buff.hot_streak.react|action.pyroblast.in_flight&buff.heating_up.react&!buff.hot_streak.react)" );
  default_->add_action( "call_action_list,name=standard_rotation,if=(variable.time_to_combustion>0|variable.disable_combustion)&buff.rune_of_power.down" );

  active_talents->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down&(variable.time_to_combustion>cooldown.living_bomb.duration|variable.time_to_combustion<=0|variable.disable_combustion)" );
  active_talents->add_action( "meteor,if=!variable.disable_combustion&variable.time_to_combustion<=0|(cooldown.meteor.duration<variable.time_to_combustion&!talent.rune_of_power.enabled)|talent.rune_of_power.enabled&buff.rune_of_power.up&variable.time_to_combustion>action.meteor.cooldown|target.time_to_die<variable.time_to_combustion|variable.disable_combustion" );
  active_talents->add_action( "dragons_breath,if=talent.alexstraszas_fury.enabled&(buff.combustion.down&!buff.hot_streak.react)" );

  rop_phase->add_action( "flamestrike,if=(active_enemies>=variable.hot_streak_flamestrike&(time-buff.combustion.last_expire>variable.delay_flamestrike|variable.disable_combustion))&buff.hot_streak.react" );
  rop_phase->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time" );
  rop_phase->add_action( "pyroblast,if=buff.firestorm.react" );
  rop_phase->add_action( "pyroblast,if=buff.hot_streak.react" );
  rop_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=buff.sun_kings_blessing_ready.down&!(active_enemies>=variable.hard_cast_flamestrike&(time-buff.combustion.last_expire>variable.delay_flamestrike|variable.disable_combustion))&!firestarter.active&(!buff.heating_up.react&!buff.hot_streak.react&!prev_off_gcd.fire_blast&(action.fire_blast.charges>=2|(talent.alexstraszas_fury.enabled&cooldown.dragons_breath.ready)|(talent.searing_touch.enabled&target.health.pct<=30)))" );
  rop_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&(((action.fireball.executing|action.pyroblast.executing)&buff.heating_up.react)|(talent.searing_touch.enabled&target.health.pct<=30&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))" );
  rop_phase->add_action( "call_action_list,name=active_talents" );
  rop_phase->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.pyroclasm.remains&cast_time<buff.rune_of_power.remains&(buff.pyroclasm.react=buff.pyroclasm.max_stack|buff.pyroclasm.remains<cast_time+action.fireball.execute_time|buff.alexstraszas_fury.up|!runeforge.sun_kings_blessing.equipped)" );
  rop_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&talent.searing_touch.enabled&target.health.pct<=30&!(active_enemies>=variable.hot_streak_flamestrike&(time-buff.combustion.last_expire>variable.delay_flamestrike|variable.disable_combustion))" );
  rop_phase->add_action( "phoenix_flames,if=!variable.phoenix_pooling&buff.heating_up.react&!buff.hot_streak.react&(active_dot.ignite<2|active_enemies>=variable.hard_cast_flamestrike|active_enemies>=variable.hot_streak_flamestrike)" );
  rop_phase->add_action( "scorch,if=target.health.pct<=30&talent.searing_touch.enabled" );
  rop_phase->add_action( "dragons_breath,if=active_enemies>2" );
  rop_phase->add_action( "flamestrike,if=(active_enemies>=variable.hard_cast_flamestrike&(time-buff.combustion.last_expire>variable.delay_flamestrike|variable.disable_combustion))" );
  rop_phase->add_action( "fireball" );

  combustion_phase->add_action( "lights_judgment,if=buff.combustion.down" );
  combustion_phase->add_action( "variable,name=extended_combustion_remains,op=set,value=buff.combustion.remains+buff.combustion.duration*(cooldown.combustion.remains<buff.combustion.remains)" );
  combustion_phase->add_action( "variable,name=extended_combustion_remains,op=add,value=6,if=buff.sun_kings_blessing_ready.up|variable.extended_combustion_remains>1.5*gcd.max*(buff.sun_kings_blessing.max_stack-buff.sun_kings_blessing.stack)" );
  combustion_phase->add_action( "bag_of_tricks,if=buff.combustion.down" );
  combustion_phase->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down" );
  combustion_phase->add_action( "mirrors_of_torment,if=buff.combustion.down&buff.rune_of_power.down" );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=(active_enemies<=active_dot.ignite|!cooldown.phoenix_flames.ready)&conduit.infernal_cascade.enabled&charges>=1&((action.fire_blast.charges_fractional+(variable.extended_combustion_remains-buff.infernal_cascade.duration)%cooldown.fire_blast.duration-variable.extended_combustion_remains%(buff.infernal_cascade.duration-0.5))>=0|variable.extended_combustion_remains<=buff.infernal_cascade.duration|buff.infernal_cascade.remains<0.5)&buff.combustion.up&!buff.firestorm.react&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react<2" );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=(active_enemies<=active_dot.ignite|!cooldown.phoenix_flames.ready)&!conduit.infernal_cascade.enabled&charges>=1&buff.combustion.up&!buff.firestorm.react&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react<2" );
  combustion_phase->add_action( "counterspell,if=runeforge.disciplinary_command.equipped&buff.disciplinary_command.down&buff.disciplinary_command_arcane.down&cooldown.buff_disciplinary_command.ready" );
  combustion_phase->add_action( "arcane_explosion,if=runeforge.disciplinary_command.equipped&buff.disciplinary_command.down&buff.disciplinary_command_arcane.down&cooldown.buff_disciplinary_command.ready" );
  combustion_phase->add_action( "frostbolt,if=runeforge.disciplinary_command.equipped&buff.disciplinary_command.down&buff.disciplinary_command_frost.down" );
  combustion_phase->add_action( "call_action_list,name=active_talents" );
  combustion_phase->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&(runeforge.disciplinary_command.equipped=buff.disciplinary_command.up)&(action.meteor.in_flight&action.meteor.in_flight_remains<=0.5|action.scorch.executing&action.scorch.execute_remains<0.5|action.fireball.executing&action.fireball.execute_remains<0.5|action.pyroblast.executing&action.pyroblast.execute_remains<0.5)" );
  combustion_phase->add_action( "potion,if=buff.combustion.last_expire<=action.combustion.last_used" );
  combustion_phase->add_action( "blood_fury,if=buff.combustion.last_expire<=action.combustion.last_used" );
  combustion_phase->add_action( "berserking,if=buff.combustion.last_expire<=action.combustion.last_used" );
  combustion_phase->add_action( "fireblood,if=buff.combustion.last_expire<=action.combustion.last_used" );
  combustion_phase->add_action( "ancestral_call,if=buff.combustion.last_expire<=action.combustion.last_used" );
  combustion_phase->add_action( "use_items,if=buff.combustion.last_expire<=action.combustion.last_used" );
  combustion_phase->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.combustion_flamestrike" );
  combustion_phase->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time" );
  combustion_phase->add_action( "pyroblast,if=buff.firestorm.react" );
  combustion_phase->add_action( "pyroblast,if=buff.pyroclasm.react&buff.pyroclasm.remains>cast_time&(buff.combustion.remains>cast_time|buff.combustion.down)" );
  combustion_phase->add_action( "pyroblast,if=buff.hot_streak.react&buff.combustion.up" );
  combustion_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  combustion_phase->add_action( "phoenix_flames,if=buff.combustion.up" );
  combustion_phase->add_action( "fireball,if=buff.combustion.down&cooldown.combustion.remains<cast_time&!conduit.flame_accretion.enabled" );
  combustion_phase->add_action( "scorch,if=buff.combustion.remains>cast_time&buff.combustion.up|buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "living_bomb,if=buff.combustion.remains<gcd.max&active_enemies>1" );
  combustion_phase->add_action( "dragons_breath,if=buff.combustion.remains<gcd.max&buff.combustion.up" );
  combustion_phase->add_action( "scorch,if=target.health.pct<=30&talent.searing_touch.enabled" );

  standard_rotation->add_action( "flamestrike,if=(active_enemies>=variable.hot_streak_flamestrike&(time-buff.combustion.last_expire>variable.delay_flamestrike|variable.disable_combustion))&buff.hot_streak.react" );
  standard_rotation->add_action( "pyroblast,if=buff.firestorm.react" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&buff.hot_streak.remains<action.fireball.execute_time" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&(prev_gcd.1.fireball|firestarter.active|action.pyroblast.in_flight)" );
  standard_rotation->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&(cooldown.rune_of_power.remains+action.rune_of_power.execute_time+cast_time>buff.sun_kings_blessing_ready.remains|!talent.rune_of_power.enabled)&variable.time_to_combustion+cast_time>buff.sun_kings_blessing_ready.remains" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&target.health.pct<=30&talent.searing_touch.enabled" );
  standard_rotation->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.pyroclasm.remains&(buff.pyroclasm.react=buff.pyroclasm.max_stack|buff.pyroclasm.remains<cast_time+action.fireball.execute_time|buff.alexstraszas_fury.up|!runeforge.sun_kings_blessing.equipped)" );
  standard_rotation->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&!variable.fire_blast_pooling&(((action.fireball.executing|action.pyroblast.executing)&buff.heating_up.react)|(talent.searing_touch.enabled&target.health.pct<=30&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))" );
  standard_rotation->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&talent.searing_touch.enabled&target.health.pct<=30&!(active_enemies>=variable.hot_streak_flamestrike&(time-buff.combustion.last_expire>variable.delay_flamestrike|variable.disable_combustion))" );
  standard_rotation->add_action( "phoenix_flames,if=!variable.phoenix_pooling&(!talent.from_the_ashes.enabled|active_enemies>1)&(active_dot.ignite<2|active_enemies>=variable.hard_cast_flamestrike|active_enemies>=variable.hot_streak_flamestrike)" );
  standard_rotation->add_action( "call_action_list,name=active_talents" );
  standard_rotation->add_action( "dragons_breath,if=active_enemies>1" );
  standard_rotation->add_action( "scorch,if=target.health.pct<=30&talent.searing_touch.enabled" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike&(time-buff.combustion.last_expire>variable.delay_flamestrike|variable.disable_combustion)" );
  standard_rotation->add_action( "fireball" );
  standard_rotation->add_action( "scorch" );
}

void mage_t::apl_frost()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* default_ = get_action_priority_list( "default" );
  action_priority_list_t* cds = get_action_priority_list( "cds" );
  action_priority_list_t* essences = get_action_priority_list( "essences" );
  action_priority_list_t* st = get_action_priority_list( "st" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );
  action_priority_list_t* movement = get_action_priority_list( "movement" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "summon_water_elemental" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "potion" );
  precombat->add_action( "frostbolt" );

  default_->add_action( "counterspell" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=essences" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=5" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<5" );
  default_->add_action( "call_action_list,name=movement" );

  cds->add_action( "mirrors_of_torment,if=soulbind.wasteland_propriety.enabled" );
  cds->add_action( "deathborne" );
  cds->add_action( "rune_of_power,if=cooldown.icy_veins.remains>15&buff.rune_of_power.down" );
  cds->add_action( "icy_veins,if=buff.rune_of_power.down" );
  cds->add_action( "time_warp,if=runeforge.temporal_warp.equipped&time>10&(prev_off_gcd.icy_veins|target.time_to_die<30)" );
  cds->add_action( "potion,if=prev_off_gcd.icy_veins|target.time_to_die<30" );
  cds->add_action( "use_items" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "bag_of_tricks" );

  essences->add_action( "guardian_of_azeroth" );
  essences->add_action( "focused_azerite_beam" );
  essences->add_action( "memory_of_lucid_dreams" );
  essences->add_action( "blood_of_the_enemy" );
  essences->add_action( "purifying_blast" );
  essences->add_action( "ripple_in_space" );
  essences->add_action( "concentrated_flame,line_cd=6" );
  essences->add_action( "reaping_flames" );
  essences->add_action( "the_unbound_force,if=buff.reckless_force.up" );
  essences->add_action( "worldvein_resonance" );

  st->add_action( "flurry,if=(remaining_winters_chill=0|debuff.winters_chill.down)&(prev_gcd.1.ebonbolt|buff.brain_freeze.react&(prev_gcd.1.radiant_spark|prev_gcd.1.glacial_spike|prev_gcd.1.frostbolt|(debuff.mirrors_of_torment.up|buff.expanded_potential.react|buff.freezing_winds.up)&buff.fingers_of_frost.react=0))" );
  st->add_action( "frozen_orb" );
  st->add_action( "blizzard,if=buff.freezing_rain.up|active_enemies>=3|active_enemies>=2&!runeforge.cold_front.equipped" );
  st->add_action( "ray_of_frost,if=remaining_winters_chill=1&debuff.winters_chill.remains" );
  st->add_action( "glacial_spike,if=remaining_winters_chill&debuff.winters_chill.remains>cast_time+travel_time" );
  st->add_action( "ice_lance,if=remaining_winters_chill&remaining_winters_chill>buff.fingers_of_frost.react&debuff.winters_chill.remains>travel_time" );
  st->add_action( "comet_storm" );
  st->add_action( "ice_nova" );
  st->add_action( "radiant_spark,if=buff.freezing_winds.up&active_enemies=1" );
  st->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time" );
  st->add_action( "ebonbolt" );
  st->add_action( "radiant_spark,if=(!runeforge.freezing_winds.equipped|active_enemies>=2)&(buff.brain_freeze.react|soulbind.combat_meditation.enabled)" );
  st->add_action( "shifting_power,if=active_enemies>=3" );
  st->add_action( "shifting_power,line_cd=60,if=(soulbind.field_of_blossoms.enabled|soulbind.grove_invigoration.enabled)&(!talent.rune_of_power.enabled|buff.rune_of_power.down&cooldown.rune_of_power.remains>16)" );
  st->add_action( "mirrors_of_torment" );
  st->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&target.level<=level&debuff.frozen.down" );
  st->add_action( "arcane_explosion,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_arcane.down" );
  st->add_action( "fire_blast,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down" );
  st->add_action( "glacial_spike,if=buff.brain_freeze.react" );
  st->add_action( "frostbolt" );

  aoe->add_action( "frozen_orb" );
  aoe->add_action( "blizzard" );
  aoe->add_action( "flurry,if=(remaining_winters_chill=0|debuff.winters_chill.down)&(prev_gcd.1.ebonbolt|buff.brain_freeze.react&buff.fingers_of_frost.react=0)" );
  aoe->add_action( "ice_nova" );
  aoe->add_action( "comet_storm" );
  aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time|remaining_winters_chill&debuff.winters_chill.remains>travel_time" );
  aoe->add_action( "radiant_spark" );
  aoe->add_action( "shifting_power" );
  aoe->add_action( "mirrors_of_torment" );
  aoe->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&target.level<=level&debuff.frozen.down" );
  aoe->add_action( "fire_blast,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down" );
  aoe->add_action( "arcane_explosion,if=mana.pct>30&!runeforge.cold_front.equipped" );
  aoe->add_action( "ebonbolt" );
  aoe->add_action( "frostbolt" );

  movement->add_action( "blink_any,if=movement.distance>10" );
  movement->add_action( "ice_floes,if=buff.ice_floes.down" );
  movement->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=2" );
  movement->add_action( "fire_blast" );
  movement->add_action( "ice_lance" );
}

double mage_t::resource_regen_per_second( resource_e rt ) const
{
  double reg = player_t::resource_regen_per_second( rt );

  if ( specialization() == MAGE_ARCANE && rt == RESOURCE_MANA )
  {
    reg *= 1.0 + 0.01 * spec.arcane_mage->effectN( 4 ).average( this );
    reg *= 1.0 + cache.mastery() * spec.savant->effectN( 1 ).mastery_value();
    reg *= 1.0 + buffs.enlightened_mana->check_value();

    if ( player_t::buffs.memory_of_lucid_dreams->check() )
      reg *= 1.0 + player_t::buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();
  }

  return reg;
}

void mage_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_MASTERY && spec.savant->ok() )
    recalculate_resource_max( RESOURCE_MANA );
}

double mage_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* a )
{
  double g = player_t::resource_gain( resource_type, amount, source, a );

  if ( resource_type == RESOURCE_MANA && talents.enlightened->ok() && a )
    update_enlightened();

  return g;
}

double mage_t::resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* a )
{
  double l = player_t::resource_loss( resource_type, amount, source, a );

  if ( resource_type == RESOURCE_MANA && talents.enlightened->ok() && a )
    update_enlightened();

  return l;
}

void mage_t::recalculate_resource_max( resource_e rt, gain_t* source )
{
  double max = resources.max[ rt ];
  double pct = resources.pct( rt );

  player_t::recalculate_resource_max( rt, source );

  if ( specialization() == MAGE_ARCANE && rt == RESOURCE_MANA )
  {
    resources.max[ rt ] *= 1.0 + cache.mastery() * spec.savant->effectN( 1 ).mastery_value();
    resources.max[ rt ] *= 1.0 + buffs.arcane_familiar->check_value();

    resources.current[ rt ] = resources.max[ rt ] * pct;
    sim->print_debug( "{} adjusts maximum mana from {} to {} ({}%)", name(), max, resources.max[ rt ], 100.0 * pct );
  }
}

double mage_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  m += state.from_the_ashes_mastery;

  return m;
}

double mage_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  m *= 1.0 + buffs.fevered_incantation->check_stack_value();
  m *= 1.0 + buffs.disciplinary_command->check_value();

  return m;
}

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( dbc::is_school( school, SCHOOL_ARCANE ) )
    m *= 1.0 + buffs.enlightened_damage->check_value();

  if ( dbc::is_school( school, SCHOOL_FIRE ) )
    m *= 1.0 + buffs.infernal_cascade->check_stack_value();

  return m;
}

double mage_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  m *= 1.0 + spec.arcane_mage->effectN( 3 ).percent();
  m *= 1.0 + spec.fire_mage->effectN( 3 ).percent();
  m *= 1.0 + spec.frost_mage->effectN( 3 ).percent();

  m *= 1.0 + buffs.bone_chilling->check_stack_value();
  m *= 1.0 + buffs.incanters_flow->check_stack_value();
  m *= 1.0 + buffs.rune_of_power->check_value();

  return m;
}

double mage_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  if ( auto td = target_data[ target ] )
  {
    if ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_FIRE ) || dbc::is_school( school, SCHOOL_FROST ) )
      m *= 1.0 + td->debuffs.grisly_icicle->check_value();
  }

  return m;
}

double mage_t::composite_rating_multiplier( rating_e r ) const
{
  double rm = player_t::composite_rating_multiplier( r );

  switch ( r )
  {
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
    case RATING_SPELL_CRIT:
      rm *= 1.0 + spec.critical_mass_2->effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return rm;
}

double mage_t::composite_spell_crit_chance() const
{
  double c = player_t::composite_spell_crit_chance();

  c += spec.critical_mass->effectN( 1 ).percent();
  c += buffs.focus_magic_crit->check_value();

  return c;
}

double mage_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

void mage_t::reset()
{
  player_t::reset();

  icicles.clear();
  events = events_t();
  last_bomb_target = nullptr;
  last_frostbolt_target = nullptr;
  burn_phase.reset();
  ground_aoe_expiration = std::array<timespan_t, AOE_MAX>();
  state = state_t();
}

void mage_t::update_movement( timespan_t duration )
{
  player_t::update_movement( duration );
  update_rune_distance( duration.total_seconds() * cache.run_speed() );
}

void mage_t::teleport( double distance, timespan_t duration )
{
  player_t::teleport( distance, duration );
  update_rune_distance( distance );
}

double mage_t::passive_movement_modifier() const
{
  double pmm = player_t::passive_movement_modifier();

  pmm += buffs.chrono_shift->check_value();
  pmm += buffs.frenetic_speed->check_value();

  return pmm;
}

void mage_t::arise()
{
  player_t::arise();

  buffs.incanters_flow->trigger();

  if ( talents.enlightened->ok() )
  {
    update_enlightened();

    timespan_t first_tick = rng().real() * options.enlightened_interval;
    events.enlightened = make_event<events::enlightened_event_t>( *sim, *this, first_tick );
  }

  if ( talents.focus_magic->ok() && options.focus_magic_interval > 0_ms )
  {
    timespan_t period = options.focus_magic_interval;
    period = std::max( 1_ms, rng().gauss( period, period * options.focus_magic_stddev ) );
    events.focus_magic = make_event<events::focus_magic_event_t>( *sim, *this, period );
  }

  if ( talents.from_the_ashes->ok() )
  {
    update_from_the_ashes();

    timespan_t first_tick = rng().real() * options.from_the_ashes_interval;
    events.from_the_ashes = make_event<events::from_the_ashes_event_t>( *sim, *this, first_tick );
  }

  if ( talents.time_anomaly->ok() )
  {
    timespan_t first_tick = rng().real() * talents.time_anomaly->effectN( 1 ).period();
    events.time_anomaly = make_event<events::time_anomaly_tick_event_t>( *sim, *this, first_tick );
  }
}

void mage_t::combat_begin()
{
  player_t::combat_begin();

  if ( specialization() == MAGE_ARCANE )
  {
    // When combat starts, any Arcane Charge stacks above one are
    // removed.
    int ac_stack = buffs.arcane_charge->check();
    if ( ac_stack > 1 )
      buffs.arcane_charge->decrement( ac_stack - 1 );

    uptime.burn_phase->update( false, sim->current_time() );
    uptime.conserve_phase->update( true, sim->current_time() );
  }
}

void mage_t::combat_end()
{
  player_t::combat_end();

  if ( specialization() == MAGE_ARCANE )
  {
    uptime.burn_phase->update( false, sim->current_time() );
    uptime.conserve_phase->update( false, sim->current_time() );
  }
}

/**
 * Mage specific action expressions
 *
 * Use this function for expressions which are bound to some action property (eg. target, cast_time, etc.) and not just
 * to the player itself. For those use the normal mage_t::create_expression override.
 */
std::unique_ptr<expr_t> mage_t::create_action_expression( action_t& action, util::string_view name )
{
  auto splits = util::string_split<util::string_view>( name, "." );

  // Firestarter expressions ==================================================
  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "firestarter" ) )
  {
    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( name_str, [ this, &action ]
      {
        if ( !talents.firestarter->ok() )
          return false;

        if ( options.firestarter_time > 0_ms )
          return sim->current_time() < options.firestarter_time;
        else
          return action.target->health_percentage() > talents.firestarter->effectN( 1 ).base_value();
      } );
    }

    if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      return make_fn_expr( name_str, [ this, &action ]
      {
        if ( !talents.firestarter->ok() )
          return 0.0;

        if ( options.firestarter_time > 0_ms )
          return std::max( options.firestarter_time - sim->current_time(), 0_ms ).total_seconds();
        else
          return action.target->time_to_percent( talents.firestarter->effectN( 1 ).base_value() ).total_seconds();
      } );
    }

    throw std::invalid_argument( fmt::format( "Unknown firestarer operation '{}'", splits[ 1 ] ) );
  }

  return player_t::create_action_expression( action, name );
}

std::unique_ptr<expr_t> mage_t::create_expression( util::string_view name )
{
  if ( util::str_compare_ci( name, "mana_gem_charges" ) )
  {
    return make_fn_expr( name, [ this ]
    { return state.mana_gem_charges; } );
  }

  // Incanters flow direction
  // Evaluates to:  0.0 if IF talent not chosen or IF stack unchanged
  //                1.0 if next IF stack increases
  //               -1.0 if IF stack decreases
  if ( util::str_compare_ci( name, "incanters_flow_dir" ) )
  {
    return make_fn_expr( name, [ this ]
    {
      if ( !talents.incanters_flow->ok() )
        return 0.0;

      if ( buffs.incanters_flow->reverse )
        return buffs.incanters_flow->check() == 1 ? 0.0 : -1.0;
      else
        return buffs.incanters_flow->check() == 5 ? 0.0 : 1.0;
    } );
  }

  if ( util::str_compare_ci( name, "burn_phase" ) )
  {
    return make_fn_expr( name, [ this ]
    { return burn_phase.on(); } );
  }

  if ( util::str_compare_ci( name, "burn_phase_duration" ) )
  {
    return make_fn_expr( name, [ this ]
    { return burn_phase.duration( sim->current_time() ).total_seconds(); } );
  }

  if ( util::str_compare_ci( name, "shooting_icicles" ) )
  {
    return make_fn_expr( name, [ this ]
    { return events.icicle != nullptr; } );
  }

  if ( util::str_compare_ci( name, "remaining_winters_chill" ) )
  {
    return make_fn_expr( name, [ this ]
    { return remaining_winters_chill; } );
  }

  if ( util::str_compare_ci( name, "hot_streak_spells_in_flight" ) )
  {
    auto is_hss = [] ( action_t* a )
    {
      if ( auto m = dynamic_cast<actions::mage_spell_t*>( a ) )
        return m->triggers.hot_streak != TT_NONE;
      else
        return false;
    };

    std::vector<action_t*> in_flight_list;
    for ( auto a : action_list )
    {
      if ( is_hss( a ) || range::find_if( a->child_action, is_hss ) != a->child_action.end() )
        in_flight_list.push_back( a );
    }

    return make_fn_expr( name, [ in_flight_list ]
    {
      size_t spells = 0;
      for ( auto a : in_flight_list )
        spells += a->num_travel_events();
      return spells;
    } );
  }

  auto splits = util::string_split<util::string_view>( name, "." );

  if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "ground_aoe" ) )
  {
    auto to_string = [] ( ground_aoe_type_e type )
    {
      switch ( type )
      {
        case AOE_BLIZZARD:    return "blizzard";
        case AOE_COMET_STORM: return "comet_storm";
        case AOE_FLAME_PATCH: return "flame_patch";
        case AOE_FROZEN_ORB:  return "frozen_orb";
        case AOE_METEOR_BURN: return "meteor_burn";
        case AOE_MAX:         return "unknown";
      }
      return "unknown";
    };

    auto type = AOE_MAX;
    for ( auto i = static_cast<ground_aoe_type_e>( 0 ); i < AOE_MAX; i++ )
    {
      if ( util::str_compare_ci( splits[ 1 ], to_string( i ) ) )
      {
        type = i;
        break;
      }
    }

    if ( type == AOE_MAX )
      throw std::invalid_argument( fmt::format( "Unknown ground_aoe type '{}'", splits[ 1 ] ) );

    if ( util::str_compare_ci( splits[ 2 ], "remains" ) )
    {
      return make_fn_expr( name, [ this, type ]
      { return std::max( ground_aoe_expiration[ type ] - sim->current_time(), 0_ms ).total_seconds(); } );
    }

    throw std::invalid_argument( fmt::format( "Unknown ground_aoe operation '{}'", splits[ 2 ] ) );
  }

  if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "incanters_flow_time_to" ) )
  {
    int expr_stack = util::to_int( splits[ 1 ] );
    if ( expr_stack < 1 || expr_stack > buffs.incanters_flow->max_stack() )
      throw std::invalid_argument( fmt::format( "Invalid incanters_flow_time_to stack number '{}'", splits[ 1 ] ) );

    // Number of ticks in one full cycle.
    int tick_cycle = buffs.incanters_flow->max_stack() * 2;
    int expr_pos_lo;
    int expr_pos_hi;

    if ( util::str_compare_ci( splits[ 2 ], "up" ) )
    {
      expr_pos_lo = expr_pos_hi = expr_stack;
    }
    else if ( util::str_compare_ci( splits[ 2 ], "down" ) )
    {
      expr_pos_lo = expr_pos_hi = tick_cycle - expr_stack + 1;
    }
    else if ( util::str_compare_ci( splits[ 2 ], "any" ) )
    {
      expr_pos_lo = expr_stack;
      expr_pos_hi = tick_cycle - expr_stack + 1;
    }
    else
    {
      throw std::invalid_argument( fmt::format( "Unknown incanters_flow_time_to stack type '{}'", splits[ 2 ] ) );
    }

    return make_fn_expr( name, [ this, tick_cycle, expr_pos_lo, expr_pos_hi ]
    {
      if ( !talents.incanters_flow->ok() || !buffs.incanters_flow->tick_event )
        return 0.0;

      int buff_stack = buffs.incanters_flow->check();
      int buff_pos = buffs.incanters_flow->reverse ? tick_cycle - buff_stack + 1 : buff_stack;
      if ( expr_pos_lo == buff_pos || expr_pos_hi == buff_pos )
        return 0.0;

      // Number of ticks required to reach the desired position.
      int ticks_lo = ( tick_cycle + expr_pos_lo - buff_pos ) % tick_cycle;
      int ticks_hi = ( tick_cycle + expr_pos_hi - buff_pos ) % tick_cycle;

      double tick_time = buffs.incanters_flow->tick_time().total_seconds();
      double tick_rem = buffs.incanters_flow->tick_event->remains().total_seconds();
      double value = tick_rem + tick_time * ( std::min( ticks_lo, ticks_hi ) - 1 );

      sim->print_debug( "incanters_flow_time_to: buff_position={} ticks_low={} ticks_high={} value={}",
                        buff_pos, ticks_lo, ticks_hi, value );

      return value;
    } );
  }

  return player_t::create_expression( name );
}

stat_e mage_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
    case STAT_STR_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
    case STAT_SPIRIT:
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

void mage_t::vision_of_perfection_proc()
{
  if ( vision_of_perfection_multiplier <= 0.0 )
    return;

  buff_t* primary = nullptr;
  buff_t* secondary = nullptr;

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      primary = buffs.arcane_power;
      break;
    case MAGE_FIRE:
      primary = buffs.combustion;
      secondary = buffs.wildfire;
      break;
    case MAGE_FROST:
      primary = buffs.icy_veins;
      secondary = buffs.frigid_grasp;
      break;
    default:
      return;
  }

  // Hotfixed to use the base duration of the buffs.
  timespan_t primary_duration = vision_of_perfection_multiplier * primary->data().duration();
  timespan_t secondary_duration = secondary ? vision_of_perfection_multiplier * secondary->data().duration() : 0_ms;

  if ( primary->check() )
  {
    primary->extend_duration( this, primary_duration );
    if ( secondary )
      secondary->extend_duration( this, secondary_duration );
  }
  else
  {
    primary->trigger( primary_duration );
    if ( secondary )
    {
      // For some reason, Frigid Grasp activates at a full duration.
      // TODO: This might be a bug.
      if ( specialization() == MAGE_FROST )
      {
        secondary->trigger();
      }
      else
      {
        secondary->trigger( secondary_duration );
      }
    }
    // TODO: This probably isn't intended
    buffs.rune_of_power->trigger();
  }
}

void mage_t::update_rune_distance( double distance )
{
  distance_from_rune += distance;

  if ( buffs.rune_of_power->check() && distance_from_rune > talents.rune_of_power->effectN( 2 ).radius() )
  {
    buffs.rune_of_power->expire();
    sim->print_debug( "{} moved out of Rune of Power.", name() );
  }
}

void mage_t::update_enlightened()
{
  if ( !talents.enlightened->ok() )
    return;

  if ( resources.pct( RESOURCE_MANA ) > talents.enlightened->effectN( 1 ).percent() )
  {
    buffs.enlightened_damage->trigger();
    buffs.enlightened_mana->expire();
  }
  else
  {
    buffs.enlightened_damage->expire();
    buffs.enlightened_mana->trigger();
  }
}

void mage_t::update_from_the_ashes()
{
  if ( !talents.from_the_ashes->ok() )
    return;

  state.from_the_ashes_mastery = talents.from_the_ashes->effectN( 3 ).base_value() * cooldowns.phoenix_flames->charges_fractional();
  invalidate_cache( CACHE_MASTERY );

  if ( sim->debug )
    sim->print_debug( "{} updates mastery from From the Ashes, new value: {}", name_str, cache.mastery() );
}

action_t* mage_t::get_icicle()
{
  action_t* a = nullptr;

  if ( !icicles.empty() )
  {
    event_t::cancel( icicles.front().expiration );
    a = icicles.front().action;
    icicles.erase( icicles.begin() );
  }

  return a;
}

bool mage_t::trigger_delayed_buff( buff_t* buff, double chance, timespan_t delay )
{
  if ( buff->max_stack() == 0 || buff->cooldown->down() )
    return false;

  bool success;
  if ( chance < 0.0 )
  {
    if ( buff->rppm )
      success = buff->rppm->trigger();
    else
      success = rng().roll( buff->default_chance );
  }
  else
  {
    success = rng().roll( chance );
  }

  if ( success )
  {
    if ( buff->check() )
      make_event( sim, delay, [ buff ] { buff->execute(); } );
    else
      buff->execute();
  }

  return success;
}

void mage_t::trigger_brain_freeze( double chance, proc_t* source, timespan_t delay )
{
  assert( source );

  bool success = trigger_delayed_buff( buffs.brain_freeze, chance, delay );
  if ( success )
  {
    source->occur();
    procs.brain_freeze->occur();
  }
}

void mage_t::trigger_fof( double chance, int stacks, proc_t* source )
{
  assert( source );

  bool success = buffs.fingers_of_frost->trigger( stacks, buff_t::DEFAULT_VALUE(), chance );
  if ( success )
  {
    if ( chance >= 1.0 )
      buffs.fingers_of_frost->predict();

    for ( int i = 0; i < stacks; i++ )
    {
      source->occur();
      procs.fingers_of_frost->occur();
    }
  }
}

void mage_t::trigger_icicle( player_t* icicle_target, bool chain )
{
  assert( icicle_target );

  if ( !spec.icicles->ok() )
    return;

  if ( chain && !events.icicle )
  {
    events.icicle = make_event<events::icicle_event_t>( *sim, *this, icicle_target, true );
    sim->print_debug( "{} icicle use on {} (chained), total={}", name(), icicle_target->name(), icicles.size() );
  }

  if ( !chain )
  {
    action_t* icicle_action = get_icicle();
    if ( !icicle_action )
      return;

    icicle_action->set_target( icicle_target );
    icicle_action->execute();
    sim->print_debug( "{} icicle use on {}, total={}", name(), icicle_target->name(), icicles.size() );
  }
}

void mage_t::trigger_icicle_gain( player_t* icicle_target, action_t* icicle_action )
{
  if ( !spec.icicles->ok() )
    return;

  unsigned max_icicles = as<unsigned>( spec.icicles->effectN( 2 ).base_value() );

  // Shoot one if capped
  if ( icicles.size() == max_icicles )
    trigger_icicle( icicle_target );

  buffs.icicles->trigger();
  icicles.push_back( { icicle_action, make_event( sim, buffs.icicles->buff_duration(), [ this ]
  {
    buffs.icicles->decrement();
    icicles.erase( icicles.begin() );
  } ) } );

  assert( icicle_action && icicles.size() <= max_icicles );
}

void mage_t::trigger_evocation( timespan_t duration_override, bool hasted )
{
  double mana_regen_multiplier = 1.0 + buffs.evocation->default_value;

  timespan_t duration = duration_override;
  if ( duration <= 0_ms )
    duration = buffs.evocation->buff_duration();

  if ( hasted )
  {
    mana_regen_multiplier /= cache.spell_speed();
    duration *= cache.spell_speed();
  }

  buffs.evocation->trigger( 1, mana_regen_multiplier, -1.0, duration );
}

void mage_t::trigger_lucid_dreams( player_t* trigger_target, double cost )
{
  if ( lucid_dreams_refund <= 0.0 )
    return;

  if ( cost <= 0.0 )
    return;

  double proc_chance =
    ( specialization() == MAGE_ARCANE ) ? options.lucid_dreams_proc_chance_arcane :
    ( specialization() == MAGE_FIRE   ) ? options.lucid_dreams_proc_chance_fire :
                                          options.lucid_dreams_proc_chance_frost;

  if ( rng().roll( proc_chance ) )
  {
    switch ( specialization() )
    {
      case MAGE_ARCANE:
        resource_gain( RESOURCE_MANA, lucid_dreams_refund * cost, gains.lucid_dreams );
        break;
      case MAGE_FIRE:
        cooldowns.fire_blast->adjust( -lucid_dreams_refund * cooldown_t::cooldown_duration( cooldowns.fire_blast ) );
        break;
      case MAGE_FROST:
        trigger_icicle_gain( trigger_target, action.icicle.lucid_dreams );
        break;
      default:
        break;
    }

    player_t::buffs.lucid_dreams->trigger();
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class mage_report_t : public player_report_extension_t
{
public:
  mage_report_t( mage_t& player ) :
    p( player )
  { }

  void html_customsection_cd_waste( report::sc_html_stream& os )
  {
    if ( p.cooldown_waste_data_list.empty() )
      return;

    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Cooldown waste</h3>\n"
          "<div class=\"toggle-content\">\n"
          "<table class=\"sc sort even\">\n"
          "<thead>\n"
          "<tr>\n"
          "<th></th>\n"
          "<th colspan=\"3\">Seconds per Execute</th>\n"
          "<th colspan=\"3\">Seconds per Iteration</th>\n"
          "</tr>\n"
          "<tr>\n"
          "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n"
          "<th class=\"toggle-sort\">Average</th>\n"
          "<th class=\"toggle-sort\">Minimum</th>\n"
          "<th class=\"toggle-sort\">Maximum</th>\n"
          "<th class=\"toggle-sort\">Average</th>\n"
          "<th class=\"toggle-sort\">Minimum</th>\n"
          "<th class=\"toggle-sort\">Maximum</th>\n"
          "</tr>\n"
          "</thead>\n";

    for ( const cooldown_waste_data_t* data : p.cooldown_waste_data_list )
    {
      if ( !data->active() )
        continue;

      std::string name = data->cd->name_str;
      if ( action_t* a = p.find_action( name ) )
        name = report_decorators::decorated_action(*a);
      else
        name = util::encode_html( name );

      os << "<tr>";
      fmt::print( os, "<td class=\"left\">{}</td>", name );
      fmt::print( os, "<td class=\"right\">{:.3f}</td>", data->normal.mean() );
      fmt::print( os, "<td class=\"right\">{:.3f}</td>", data->normal.min() );
      fmt::print( os, "<td class=\"right\">{:.3f}</td>", data->normal.max() );
      fmt::print( os, "<td class=\"right\">{:.3f}</td>", data->cumulative.mean() );
      fmt::print( os, "<td class=\"right\">{:.3f}</td>", data->cumulative.min() );
      fmt::print( os, "<td class=\"right\">{:.3f}</td>", data->cumulative.max() );
      os << "</tr>\n";
    }

    os << "</table>\n"
          "</div>\n"
          "</div>\n";
  }

  void html_customsection_burn_phases( report::sc_html_stream& os )
  {
    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Burn Phases</h3>\n"
          "<div class=\"toggle-content\">\n"
          "<p>Burn phase duration tracks the amount of time spent in each burn phase. This is defined as the time between a "
          "start_burn_phase and stop_burn_phase action being executed. Note that \"execute\" burn phases, i.e., the "
          "final burn of a fight, is also included.</p>\n"
          "<div class=\"flexwrap\">\n"
          "<table class=\"sc even\">\n"
          "<thead>\n"
          "<tr>\n"
          "<th>Burn Phase Duration</th>\n"
          "</tr>\n"
          "</thead>\n"
          "<tbody>\n";

    auto print_sample_data = [ &os ] ( extended_sample_data_t& s )
    {
      fmt::print( os, "<tr><td class=\"left\">Count</td><td>{}</td></tr>\n", s.count() );
      fmt::print( os, "<tr><td class=\"left\">Minimum</td><td>{:.3f}</td></tr>\n", s.min() );
      fmt::print( os, "<tr><td class=\"left\">5<sup>th</sup> percentile</td><td>{:.3f}</td></tr>\n", s.percentile( 0.05 ) );
      fmt::print( os, "<tr><td class=\"left\">Mean</td><td>{:.3f}</td></tr>\n", s.mean() );
      fmt::print( os, "<tr><td class=\"left\">95<sup>th</sup> percentile</td><td>{:.3f}</td></tr>\n", s.percentile( 0.95 ) );
      fmt::print( os, "<tr><td class=\"left\">Max</td><td>{:.3f}</td></tr>\n", s.max() );
      fmt::print( os, "<tr><td class=\"left\">Variance</td><td>{:.3f}</td></tr>\n", s.variance );
      fmt::print( os, "<tr><td class=\"left\">Mean Variance</td><td>{:.3f}</td></tr>\n", s.mean_variance );
      fmt::print( os, "<tr><td class=\"left\">Mean Std. Dev</td><td>{:.3f}</td></tr>\n", s.mean_std_dev );
    };

    print_sample_data( *p.sample_data.burn_duration_history );

    os << "</tbody>\n"
          "</table>\n";

    auto& h = *p.sample_data.burn_duration_history;
    highchart::histogram_chart_t chart( highchart::build_id( p, "burn_duration" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, h.distribution, "Burn Duration", h.mean(), h.min(), h.max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", "575" );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "</div>\n"
          "<p>Mana at burn start is the mana level recorded (in percentage of total mana) when a start_burn_phase command is executed.</p>\n"
          "<table class=\"sc even\">\n"
          "<thead>\n"
          "<tr>\n"
          "<th>Mana at Burn Start</th>\n"
          "</tr>\n"
          "</thead>\n"
          "<tbody>\n";

    print_sample_data( *p.sample_data.burn_initial_mana );

    os << "</tbody>\n"
          "</table>\n"
          "</div>\n"
          "</div>\n";
  }

  void html_customsection_icy_veins( report::sc_html_stream& os )
  {
    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Icy Veins</h3>\n"
          "<div class=\"toggle-content\">\n";

    auto& d = *p.sample_data.icy_veins_duration;
    int num_buckets = std::min( 70, static_cast<int>( d.max() - d.min() ) + 1 );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "icy_veins_duration" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Icy Veins Duration", d.mean(), d.min(), d.max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 13 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "</div>\n"
          "</div>\n";
  }

  void html_customsection_shatter( report::sc_html_stream& os )
  {
    if ( p.shatter_source_list.empty() )
      return;

    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Shatter</h3>\n"
          "<div class=\"toggle-content\">\n"
          "<table class=\"sc sort even\">\n"
          "<thead>\n"
          "<tr>\n"
          "<th></th>\n"
          "<th colspan=\"2\">None</th>\n"
          "<th colspan=\"3\">Winter's Chill</th>\n"
          "<th colspan=\"2\">Fingers of Frost</th>\n"
          "<th colspan=\"2\">Other effects</th>\n"
          "</tr>\n"
          "<tr>\n"
          "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n"
          "<th class=\"toggle-sort\">Count</th>\n"
          "<th class=\"toggle-sort\">Percent</th>\n"
          "<th class=\"toggle-sort\">Count</th>\n"
          "<th class=\"toggle-sort\">Percent</th>\n"
          "<th class=\"toggle-sort\">Utilization</th>\n"
          "<th class=\"toggle-sort\">Count</th>\n"
          "<th class=\"toggle-sort\">Percent</th>\n"
          "<th class=\"toggle-sort\">Count</th>\n"
          "<th class=\"toggle-sort\">Percent</th>\n"
          "</tr>\n"
          "</thead>\n";

    double bff = p.procs.brain_freeze_used->count.pretty_mean();

    for ( const shatter_source_t* data : p.shatter_source_list )
    {
      if ( !data->active() )
        continue;

      auto nonzero = [] ( const char* fmt, double d ) { return d != 0.0 ? fmt::format( fmt, d ) : ""; };
      auto cells = [ &, total = data->count_total() ] ( double mean, bool util = false )
      {
        std::string format_str = "<td class=\"right\">{}</td><td class=\"right\">{}</td>";
        if ( util ) format_str += "<td class=\"right\">{}</td>";

        fmt::print( os, format_str,
          nonzero( "{:.1f}", mean ),
          nonzero( "{:.1f}%", 100.0 * mean / total ),
          nonzero( "{:.1f}%", bff > 0.0 ? 100.0 * mean / bff : 0.0 ) );
      };

      std::string name = data->name_str;
      if ( action_t* a = p.find_action( name ) )
        name = report_decorators::decorated_action( *a );
      else
        name = util::encode_html( name );

      os << "<tr>";
      fmt::print( os, "<td class=\"left\">{}</td>", name );
      cells( data->count( FROZEN_NONE ) );
      cells( data->count( FROZEN_WINTERS_CHILL ), true );
      cells( data->count( FROZEN_FINGERS_OF_FROST ) );
      cells( data->count( FROZEN_ROOT ) );
      os << "</tr>\n";
    }

    os << "</table>\n"
          "</div>\n"
          "</div>\n";
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.sim->report_details == 0 )
      return;

    html_customsection_cd_waste( os );
    switch ( p.specialization() )
    {
      case MAGE_ARCANE:
        html_customsection_burn_phases( os );
        break;
      case MAGE_FROST:
        html_customsection_shatter( os );
        if ( p.talents.thermal_void->ok() )
          html_customsection_icy_veins( os );
        break;
      default:
        break;
    }
  }
private:
  mage_t& p;
};

// MAGE MODULE INTERFACE ====================================================

struct mage_module_t : public module_t
{
public:
  mage_module_t() :
    module_t( MAGE )
  { }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new mage_t( sim, name, r );
    p->report_extension = std::make_unique<mage_report_t>( *p );
    return p;
  }

  void register_hotfixes() const override
  {
    hotfix::register_spell( "Mage", "2017-03-20", "Manually set Frozen Orb's travel speed.", 84714 )
      .field( "prj_speed" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 20.0 )
      .verification_value( 0.0 );

    hotfix::register_spell( "Mage", "2017-06-21", "Ice Lance is slower than spell data suggests.", 30455 )
      .field( "prj_speed" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 47.0 )
      .verification_value( 50.0 );

    hotfix::register_spell( "Mage", "2018-12-28", "Manually set Arcane Orb's travel speed.", 153626 )
      .field( "prj_speed" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 20.0 )
      .verification_value( 0.0 );
  }

  bool valid() const override { return true; }
  void init( player_t* ) const override {}
  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};

}  // UNNAMED NAMESPACE

const module_t* module_t::mage()
{
  static mage_module_t m;
  return &m;
}

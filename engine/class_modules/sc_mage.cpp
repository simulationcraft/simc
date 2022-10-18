// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "player/covenant.hpp"
#include "util/util.hpp"
#include "class_modules/apl/mage.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"

namespace {

// ==========================================================================
// Mage
// ==========================================================================

// Forward declarations
struct mage_t;

// Finds an action with the given name. If no action exists, a new one will
// be created.
//
// Use this with secondary background actions to ensure the player only has
// one copy of the action.
template <typename Action, typename Actor, typename... Args>
action_t* get_action( std::string_view name, Actor* actor, Args&&... args )
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

struct icicle_tuple_t
{
  action_t* action; // Icicle action corresponding to the source action
  event_t*  expiration;
};

struct mage_td_t final : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* nether_tempest;
    dot_t* radiant_spark;
  } dots;

  struct debuffs_t
  {
    buff_t* frozen;
    buff_t* grisly_icicle;
    buff_t* improved_scorch;
    buff_t* radiant_spark_vulnerability;
    buff_t* touch_of_the_magi;
    buff_t* winters_chill;

    // Covenant Abilities
    buff_t* mirrors_of_torment;
  } debuffs;

  mage_td_t( player_t* target, mage_t* mage );
};

// Generalization of buff benefit tracking (up(), value(), etc).
// Keeps a track of the benefit for each stack separately.
struct buff_stack_benefit_t
{
  const buff_t* buff;
  std::vector<benefit_t*> buff_stack_benefit;

  buff_stack_benefit_t( const buff_t* _buff, std::string_view prefix ) :
    buff( _buff ),
    buff_stack_benefit()
  {
    for ( int i = 0; i <= buff->max_stack(); i++ )
    {
      auto benefit_name = fmt::format( "{} {} {}", prefix, buff->data().name_cstr(), i );
      buff_stack_benefit.push_back( buff->player->get_benefit( benefit_name ) );
    }
  }

  void update()
  {
    auto stack = as<unsigned>( buff->check() );
    for ( unsigned i = 0; i < buff_stack_benefit.size(); i++ )
      buff_stack_benefit[ i ]->update( i == stack );
  }
};

// Generalization of proc tracking (proc_t).
// Keeps a track of multiple related effects at once.
//
// See shatter_source_t for an example of its use.
template <size_t N>
struct effect_source_t : private noncopyable
{
  const std::string name_str;
  std::array<simple_sample_data_t, N> counts;
  std::array<int, N> iteration_counts;

  effect_source_t( std::string_view name ) :
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

struct mage_t final : public player_t
{
public:
  // Icicles
  std::vector<icicle_tuple_t> icicles;

  // Time Manipulation
  std::vector<cooldown_t*> time_manipulation_cooldowns;

  // Events
  struct events_t
  {
    event_t* enlightened;
    event_t* icicle;
    event_t* from_the_ashes;
    event_t* time_anomaly;
  } events;

  // State switches for rotation selection
  state_switch_t burn_phase;

  // Ground AoE tracking
  std::array<timespan_t, AOE_MAX> ground_aoe_expiration;

  // Data collection
  auto_dispose<std::vector<shatter_source_t*> > shatter_source_list;

  // Cached actions
  struct actions_t
  {
    action_t* agonizing_backlash;
    action_t* arcane_assault;
    action_t* arcane_echo;
    action_t* cold_front_frozen_orb;
    action_t* conflagration_flare_up;
    action_t* firefall_meteor;
    action_t* glacial_assault;
    action_t* harmonic_echo;
    action_t* ignite;
    action_t* living_bomb_dot;
    action_t* living_bomb_dot_spread;
    action_t* living_bomb_explosion;
    action_t* orb_barrage_arcane_orb;
    action_t* pet_freeze;
    action_t* pet_water_jet;
    action_t* tormenting_backlash;
    action_t* touch_of_the_magi_explosion;

    struct icicles_t
    {
      action_t* frostbolt;
      action_t* flurry;
      action_t* ice_lance;
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
  } benefits;

  // Buffs
  struct buffs_t
  {
    // Arcane
    buff_t* arcane_charge;
    buff_t* arcane_familiar;
    buff_t* arcane_harmony;
    buff_t* arcane_surge;
    buff_t* arcane_tempo;
    buff_t* chrono_shift;
    buff_t* clearcasting;
    buff_t* clearcasting_channel; // Hidden buff which governs tick and channel time
    buff_t* concentration;
    buff_t* enlightened_damage;
    buff_t* enlightened_mana;
    buff_t* evocation;
    buff_t* foresight;
    buff_t* foresight_icd;
    buff_t* impetus;
    buff_t* invigorating_powder;
    buff_t* nether_precision;
    buff_t* orb_barrage;
    buff_t* orb_barrage_ready;
    buff_t* presence_of_mind;
    buff_t* rule_of_threes;
    buff_t* siphon_storm;


    // Fire
    buff_t* combustion;
    buff_t* feel_the_burn;
    buff_t* fevered_incantation;
    buff_t* fiery_rush;
    buff_t* firefall;
    buff_t* firefall_ready;
    buff_t* firemind;
    buff_t* flame_accelerant;
    buff_t* flame_accelerant_icd;
    buff_t* heating_up;
    buff_t* hot_streak;
    buff_t* hyperthermia;
    buff_t* pyroclasm;
    buff_t* pyrotechnics;
    buff_t* sun_kings_blessing;
    buff_t* sun_kings_blessing_ready;
    buff_t* wildfire;


    // Frost
    buff_t* bone_chilling;
    buff_t* brain_freeze;
    buff_t* chain_reaction;
    buff_t* cold_front;
    buff_t* cold_front_ready;
    buff_t* fingers_of_frost;
    buff_t* freezing_rain;
    buff_t* freezing_winds;
    buff_t* icicles;
    buff_t* icy_veins;
    buff_t* ray_of_frost;
    buff_t* slick_ice;
    buff_t* snowstorm;


    // Shared
    buff_t* ice_floes;
    buff_t* incanters_flow;
    buff_t* overflowing_energy;
    buff_t* rune_of_power;
    buff_t* temporal_warp;
    buff_t* time_warp;


    // Runeforge Legendaries
    buff_t* runeforge_fevered_incantation;
    buff_t* firestorm;
    buff_t* molten_skyfall;
    buff_t* molten_skyfall_ready;
    buff_t* runeforge_sun_kings_blessing;
    buff_t* runeforge_sun_kings_blessing_ready;

    buff_t* disciplinary_command;
    buff_t* disciplinary_command_arcane; // Hidden buff
    buff_t* disciplinary_command_frost; // Hidden buff
    buff_t* disciplinary_command_fire; // Hidden buff
    buff_t* expanded_potential;
    buff_t* heart_of_the_fae;


    // Covenant Abilities
    buff_t* deathborne;
    buff_t* lead_by_example;


    // Soulbind Conduits
    buff_t* nether_precision_conduit;

    buff_t* flame_accretion;
    buff_t* infernal_cascade;

    buff_t* siphoned_malice;

    // Set Bonuses
    buff_t* bursting_energy;

    buff_t* touch_of_ice;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* combustion;
    cooldown_t* cone_of_cold;
    cooldown_t* fervent_flickering;
    cooldown_t* fire_blast;
    cooldown_t* flurry;
    cooldown_t* from_the_ashes;
    cooldown_t* frost_nova;
    cooldown_t* frozen_orb;
    cooldown_t* icy_veins;
    cooldown_t* incendiary_eruptions;
    cooldown_t* mirrors_of_torment;
    cooldown_t* phoenix_flames;
    cooldown_t* phoenix_reborn;
    cooldown_t* presence_of_mind;
    cooldown_t* snowstorm;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* arcane_surge;
    gain_t* mana_gem;
    gain_t* arcane_barrage;
  } gains;

  // Options
  struct options_t
  {
    double firestarter_duration_multiplier = 1.0;
    double searing_touch_duration_multiplier = 1.0;
    timespan_t frozen_duration = 1.0_s;
    timespan_t scorch_delay = 15_ms;
    timespan_t mirrors_of_torment_interval = 1.5_s;
    timespan_t arcane_missiles_chain_delay = 200_ms;
    double arcane_missiles_chain_relstddev = 0.1;
  } options;

  // Pets
  struct pets_t
  {
    pet_t* water_elemental = nullptr;
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

    proc_t* infernal_cascade_expires; // IC buffs expired during Combustion

    proc_t* brain_freeze;
    proc_t* brain_freeze_mirrors;
    proc_t* brain_freeze_snap_freeze;
    proc_t* brain_freeze_water_jet;
    proc_t* fingers_of_frost;
    proc_t* fingers_of_frost_flash_freeze;
    proc_t* fingers_of_frost_freezing_winds;
    proc_t* fingers_of_frost_snap_freeze;
    proc_t* fingers_of_frost_time_anomaly;
    proc_t* fingers_of_frost_wasted;
    proc_t* flurry_cast;
    proc_t* winters_chill_applied;
    proc_t* winters_chill_consumed;
  } procs;

  struct rppm_t
  {
    real_ppm_t* deaths_fathom;
  } rppm;

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
    const spell_data_t* arcane_charge;
    const spell_data_t* arcane_mage;
    const spell_data_t* savant;

    // Fire
    const spell_data_t* fire_mage;
    const spell_data_t* ignite;
    const spell_data_t* pyroblast_clearcasting_driver;

    // Frost
    const spell_data_t* frost_mage;
    const spell_data_t* icicles;
    const spell_data_t* icicles_2;
  } spec;

  // State
  struct state_t
  {
    bool brain_freeze_active;
    bool fingers_of_frost_active;
    int mana_gem_charges;
    int inactive_frozen_orbs;
    int active_frozen_orbs;
    double distance_from_rune;
    double from_the_ashes_mastery;
    timespan_t last_enlightened_update;
    player_t* last_bomb_target;
    int frostbolt_counter;
  } state;

  struct expression_support_t
  {
    timespan_t kindling_reduction; // Cumulative reduction from Kindling not counting guaranteed crits from Combustion
    int remaining_winters_chill; // Estimation of remaining Winter's Chill stacks, accounting for travel time
  } expression_support;

  // Talents
  struct talents_list_t
  {
    // Mage
    // Row 1
    player_talent_t blazing_barrier;
    player_talent_t ice_barrier;
    player_talent_t prismatic_barrier;

    // Row 2
    player_talent_t ice_block;
    player_talent_t overflowing_energy;
    player_talent_t invisibility;

    // Row 3
    player_talent_t winters_protection;
    player_talent_t spellsteal;
    player_talent_t tempest_barrier;
    player_talent_t incantation_of_swiftness;
    player_talent_t remove_curse;
    player_talent_t arcane_warding;

    // Row 4
    player_talent_t mirror_image;
    player_talent_t incanters_flow;
    player_talent_t rune_of_power;
    player_talent_t alter_time;

    // Row 5
    player_talent_t cryofreeze;
    player_talent_t reabsorption;
    player_talent_t reduplication;
    player_talent_t quick_witted;
    player_talent_t mass_polymorph;
    player_talent_t slow;
    player_talent_t master_of_time;
    player_talent_t diverted_energy;

    // Row 6
    player_talent_t ring_of_frost;
    player_talent_t ice_nova;
    player_talent_t ice_floes;
    player_talent_t shimmer;
    player_talent_t mass_slow;
    player_talent_t blast_wave;

    // Row 7
    player_talent_t improved_frost_nova;
    player_talent_t rigid_ice;
    player_talent_t tome_of_rhonin;
    player_talent_t tome_of_antonidas;
    player_talent_t volatile_detonation;
    player_talent_t energized_barriers;

    // Row 8
    player_talent_t frigid_winds;
    player_talent_t flow_of_time;
    player_talent_t temporal_velocity;

    // Row 9
    player_talent_t ice_ward;
    player_talent_t time_manipulation;
    player_talent_t greater_invisibility;
    player_talent_t accumulative_shielding;
    player_talent_t dragons_breath;

    // Row 10
    player_talent_t shifting_power;
    player_talent_t freezing_cold;
    player_talent_t temporal_warp;
    player_talent_t time_anomaly;
    player_talent_t displacement;
    player_talent_t meteor;

    // Arcane
    // Row 1
    player_talent_t arcane_barrage;

    // Row 2
    player_talent_t arcane_missiles;

    // Row 3
    player_talent_t arcane_orb;
    player_talent_t clearcasting;

    // Row 4
    player_talent_t arcane_tempo;
    player_talent_t improved_arcane_missiles;
    player_talent_t arcane_surge;
    player_talent_t crackling_energy;
    player_talent_t impetus;

    // Row 5
    player_talent_t arcane_familiar;
    player_talent_t rule_of_threes;
    player_talent_t charged_orb;
    player_talent_t arcing_cleave;
    player_talent_t arcane_power;
    player_talent_t mana_adept;
    player_talent_t nether_precision;
    player_talent_t amplification;

    // Row 6
    player_talent_t presence_of_mind;
    player_talent_t foresight;
    player_talent_t resonance;
    player_talent_t nether_tempest;
    player_talent_t reverberate;
    player_talent_t improved_prismatic_barrier;
    player_talent_t slipstream;
    player_talent_t improved_clearcasting;

    // Row 7
    player_talent_t chrono_shift;
    player_talent_t touch_of_the_magi;
    player_talent_t supernova;

    // Row 8
    player_talent_t evocation;
    player_talent_t enlightened;
    player_talent_t arcane_echo;
    player_talent_t arcane_bombardment;
    player_talent_t illuminated_thoughts;
    player_talent_t conjure_mana_gem;

    // Row 9
    player_talent_t siphon_storm;
    player_talent_t prodigious_savant;
    player_talent_t radiant_spark;
    player_talent_t concentration;
    player_talent_t cascading_power;

    // Row 10
    player_talent_t orb_barrage;
    player_talent_t harmonic_echo;
    player_talent_t arcane_harmony;

    // Fire
    // Row 1
    player_talent_t pyroblast;

    // Row 2
    player_talent_t fire_blast;
    player_talent_t fervent_flickering;

    // Row 3
    player_talent_t scorch;
    player_talent_t phoenix_flames;
    player_talent_t flamestrike;

    // Row 4
    player_talent_t searing_touch;
    player_talent_t firestarter;
    player_talent_t pyrotechnics;
    player_talent_t improved_flamestrike;

    // Row 5
    player_talent_t improved_scorch;
    player_talent_t critical_mass;
    player_talent_t cauterize;
    player_talent_t flame_on;
    player_talent_t flame_patch;

    // Row 6
    player_talent_t alexstraszas_fury;
    player_talent_t from_the_ashes;
    player_talent_t combustion;
    player_talent_t living_bomb;
    player_talent_t incendiary_eruptions;

    // Row 7
    player_talent_t call_of_the_sun_king;
    player_talent_t firemind;
    player_talent_t improved_combustion;
    player_talent_t tempered_flames;
    player_talent_t feel_the_burn;
    player_talent_t conflagration;

    // Row 8
    player_talent_t phoenix_reborn;
    player_talent_t pyroclasm;
    player_talent_t flame_accelerant;
    player_talent_t kindling;
    player_talent_t wildfire;
    player_talent_t firefall;
    player_talent_t pyromaniac;
    player_talent_t master_of_flame;

    // Row 9
    player_talent_t controlled_destruction;
    player_talent_t fevered_incantation;

    // Row 10
    player_talent_t sun_kings_blessing;
    player_talent_t fiery_rush;
    player_talent_t hyperthermia;

    // Frost
    // Row 1
    player_talent_t ice_lance;

    // Row 2
    player_talent_t frozen_orb;
    player_talent_t blizzard;
    player_talent_t fingers_of_frost;

    // Row 3
    player_talent_t flurry;
    player_talent_t shatter;

    // Row 4
    player_talent_t brain_freeze;
    player_talent_t everlasting_frost;
    player_talent_t cold_snap;
    player_talent_t frostbite;
    player_talent_t piercing_cold;

    // Row 5
    player_talent_t perpetual_winter;
    player_talent_t lonely_winter;
    player_talent_t summon_water_elemental;
    player_talent_t ice_caller;
    player_talent_t bone_chilling;

    // Row 6
    player_talent_t glacial_assault;
    player_talent_t ebonbolt;
    player_talent_t frozen_touch;
    player_talent_t wintertide;
    player_talent_t snowstorm;
    player_talent_t flash_freeze;
    player_talent_t subzero;

    // Row 7
    player_talent_t deep_shatter;
    player_talent_t icy_veins;
    player_talent_t splintering_cold;

    // Row 8
    player_talent_t comet_storm;
    player_talent_t freezing_rain;
    player_talent_t icy_propulsion;
    player_talent_t ray_of_frost;
    player_talent_t splitting_ice;

    // Row 9
    player_talent_t fractured_frost;
    player_talent_t snap_freeze;
    player_talent_t slick_ice;
    player_talent_t chain_reaction;
    player_talent_t hailstones;

    // Row 10
    player_talent_t cold_front;
    player_talent_t freezing_winds;
    player_talent_t thermal_void;
    player_talent_t glacial_spike;
  } talents;

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
    item_runeforge_t deaths_fathom;
    item_runeforge_t disciplinary_command;
    item_runeforge_t expanded_potential;
    item_runeforge_t grisly_icicle;
    item_runeforge_t harmonic_echo;
    item_runeforge_t heart_of_the_fae;
    item_runeforge_t sinful_delight;
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


  mage_t( sim_t* sim, std::string_view name, race_e r = RACE_NONE );

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void create_buffs() override;
  void create_options() override;
  void init_action_list() override;
  std::string default_potion() const override { return mage_apl::potion( this ); }
  std::string default_flask() const override { return mage_apl::flask( this ); }
  std::string default_food() const override { return mage_apl::food( this ); }
  std::string default_rune() const override { return mage_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return mage_apl::temporary_enchant( this ); }
  void init_gains() override;
  void init_procs() override;
  void init_benefits() override;
  void init_uptimes() override;
  void init_rng() override;
  void init_finished() override;
  void add_precombat_buff_state( buff_t*, int, double, timespan_t ) override;
  void invalidate_cache( cache_e ) override;
  void init_resources( bool ) override;
  void do_dynamic_regen( bool = false ) override;
  void recalculate_resource_max( resource_e, gain_t* = nullptr ) override;
  void reset() override;
  std::unique_ptr<expr_t> create_expression( std::string_view ) override;
  std::unique_ptr<expr_t> create_action_expression( action_t&, std::string_view ) override;
  action_t* create_action( std::string_view, std::string_view ) override;
  void create_actions() override;
  void create_pets() override;
  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_SPELL; }
  stat_e convert_hybrid_stat( stat_e ) const override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_mastery() const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_target_multiplier( player_t*, school_e ) const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_rating_multiplier( rating_e ) const override;
  double matching_gear_multiplier( attribute_e ) const override;
  void update_movement( timespan_t ) override;
  void teleport( double, timespan_t ) override;
  double passive_movement_modifier() const override;
  void arise() override;
  void combat_begin() override;
  void combat_end() override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;
  void analyze( sim_t& ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void regen( timespan_t ) override;
  void moving() override;

  target_specific_t<mage_td_t> target_data;

  const mage_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  mage_td_t* get_target_data( player_t* target ) const override
  {
    mage_td_t*& td = target_data[ target ];
    if ( !td )
      td = new mage_td_t( target, const_cast<mage_t*>( this ) );
    return td;
  }

  shatter_source_t* get_shatter_source( std::string_view name )
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
  void      update_enlightened( bool double_regen = false );
  void      update_from_the_ashes();
  action_t* get_icicle();
  bool      trigger_delayed_buff( buff_t* buff, double chance = -1.0, timespan_t delay = 0.15_s );
  bool      trigger_brain_freeze( double chance, proc_t* source, timespan_t delay = 0.15_s );
  bool      trigger_fof( double chance, proc_t* source, int stacks = 1 );
  void      trigger_icicle( player_t* icicle_target, bool chain = false );
  void      trigger_icicle_gain( player_t* icicle_target, action_t* icicle_action, timespan_t duration = timespan_t::min() );
  void      trigger_arcane_charge( int stacks = 1 );
  bool      trigger_crowd_control( const action_state_t* s, spell_mechanic type, timespan_t duration = timespan_t::min() );
  void      trigger_disciplinary_command( school_e );
  void      trigger_sinful_delight( specialization_e );
  void      trigger_time_manipulation();
};

namespace pets {

struct mage_pet_t : public pet_t
{
  mage_pet_t( sim_t* sim, mage_t* owner, std::string_view pet_name, bool guardian = false, bool dynamic = false ) :
    pet_t( sim, owner, pet_name, guardian, dynamic )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  const mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  mage_t* o()
  { return static_cast<mage_t*>( owner ); }
};

struct mage_pet_spell_t : public spell_t
{
  mage_pet_spell_t( std::string_view n, mage_pet_t* p, const spell_data_t* s ) :
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

struct water_elemental_pet_t final : public mage_pet_t
{
  water_elemental_pet_t( sim_t* sim, mage_t* owner ) :
    mage_pet_t( sim, owner, "water_elemental" )
  {
    owner_coeff.sp_from_sp = 0.75;
  }

  void init_action_list() override
  {
    action_list_str = "waterbolt";
    mage_pet_t::init_action_list();
  }

  action_t* create_action( std::string_view, std::string_view ) override;
  void      create_actions() override;
};

struct waterbolt_t final : public mage_pet_spell_t
{
  waterbolt_t( std::string_view n, water_elemental_pet_t* p, std::string_view options_str ) :
    mage_pet_spell_t( n, p, p->find_pet_spell( "Waterbolt" ) )
  {
    parse_options( options_str );
  }
};

struct freeze_t final : public mage_pet_spell_t
{
  freeze_t( std::string_view n, water_elemental_pet_t* p ) :
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

struct water_jet_t final : public mage_pet_spell_t
{
  water_jet_t( std::string_view n, water_elemental_pet_t* p ) :
    mage_pet_spell_t( n, p, p->find_pet_spell( "Water Jet" ) )
  {
    channeled = true;
  }

  void execute() override
  {
    mage_pet_spell_t::execute();
    o()->trigger_brain_freeze( 1.0, o()->procs.brain_freeze_water_jet );
  }
};

action_t* water_elemental_pet_t::create_action( std::string_view name, std::string_view options_str )
{
  if ( name == "waterbolt" ) return new waterbolt_t( name, this, options_str );

  return mage_pet_t::create_action( name, options_str );
}

void water_elemental_pet_t::create_actions()
{
  o()->action.pet_freeze = get_action<freeze_t>( "freeze", this );

  // /!\ WARNING /!\
  // This is a foreground action (and thus get_action shouldn't be used). Handle with care.
  // TODO: Do we want autocast option just like in Legion?
  o()->action.pet_water_jet = new water_jet_t( "water_jet", this );

  mage_pet_t::create_actions();
}

}  // water_elemental

namespace mirror_image {

// ==========================================================================
// Pet Mirror Image
// ==========================================================================

struct mirror_image_pet_t final : public mage_pet_t
{
  mirror_image_pet_t( sim_t* sim, mage_t* owner ) :
    mage_pet_t( sim, owner, "mirror_image", true, true )
  {
    owner_coeff.sp_from_sp = 0.55;
  }

  action_t* create_action( std::string_view, std::string_view ) override;

  void init_action_list() override
  {
    action_list_str = "frostbolt";
    mage_pet_t::init_action_list();
  }
};

struct frostbolt_t final : public mage_pet_spell_t
{
  frostbolt_t( std::string_view n, mirror_image_pet_t* p, std::string_view options_str ) :
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

action_t* mirror_image_pet_t::create_action( std::string_view name, std::string_view options_str )
{
  if ( name == "frostbolt" ) return new frostbolt_t( name, this, options_str );

  return mage_pet_t::create_action( name, options_str );
}

}  // mirror_image

}  // pets

namespace buffs {

// Custom buffs =============================================================

// Touch of the Magi debuff =================================================

struct touch_of_the_magi_t final : public buff_t
{
  touch_of_the_magi_t( mage_td_t* td ) :
    buff_t( *td, "touch_of_the_magi", td->source->find_spell( 210824 ) )
  {
    set_default_value( 0.0 );
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    auto p = debug_cast<mage_t*>( source );
    double damage_fraction = p->talents.touch_of_the_magi->effectN( 1 ).percent();
    damage_fraction += p->conduits.magis_brand.percent();
    p->action.touch_of_the_magi_explosion->execute_on_target( player, damage_fraction * current_value );
  }
};

struct combustion_t final : public buff_t
{
  double current_amount; // Amount of mastery rating granted by the buff
  double multiplier;

  combustion_t( mage_t* p ) :
    buff_t( p, "combustion", p->find_spell( 190319 ) ),
    current_amount(),
    multiplier( p->talents.improved_combustion->effectN( 2 ).percent() )
  {
    set_cooldown( 0_ms );
    set_default_value_from_effect( 1 );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    modify_duration( p->talents.improved_combustion->effectN( 1 ).time_value() );
    modify_duration( base_buff_duration * p->talents.tempered_flames->effectN( 3 ).percent() );

    set_stack_change_callback( [ this, p ] ( buff_t*, int old, int cur )
    {
      if ( old == 0 )
      {
        p->buffs.fiery_rush->trigger();
      }
      else if ( cur == 0 )
      {
        player->stat_loss( STAT_MASTERY_RATING, current_amount );
        current_amount = 0.0;
        p->buffs.fiery_rush->expire();
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

struct expanded_potential_buff_t : public buff_t
{
  mage_t* mage;

  expanded_potential_buff_t( mage_t* p, std::string_view name, const spell_data_t* spell_data ) :
    buff_t( p, name, spell_data ), mage( p )
  { }

  void decrement( int stacks, double value ) override
  {
    if ( check() )
    {
      mage->trigger_sinful_delight( MAGE_ARCANE );
      mage->trigger_sinful_delight( MAGE_FROST );
    }

    if ( check() && mage->buffs.expanded_potential->check() )
      mage->buffs.expanded_potential->expire();
    else
      buff_t::decrement( stacks, value );
  }
};

struct clearcasting_buff_t : public expanded_potential_buff_t
{
  clearcasting_buff_t( mage_t* p ) :
    expanded_potential_buff_t( p, "clearcasting", p->find_spell( 263725 ) )
  {
    set_default_value_from_effect( 1 );
    modify_max_stack( as<int>( p->talents.improved_clearcasting->effectN( 1 ).base_value() ) );
  }

  // TODO: Check everything here in game, especially the interaction with Expanded Potential
  void decrement( int stacks, double value ) override
  {
    if ( check() )
      mage->trigger_sinful_delight( MAGE_ARCANE );

    if ( check() && mage->buffs.concentration->check() )
      mage->buffs.concentration->expire();
    else
      expanded_potential_buff_t::decrement( stacks, value );
  }
};

struct ice_floes_t final : public buff_t
{
  ice_floes_t( mage_t* p ) :
    buff_t( p, "ice_floes", p->talents.ice_floes )
  { }

  void decrement( int stacks, double value ) override
  {
    if ( !check() )
      return;

    if ( sim->current_time() - last_trigger > 0.5_s )
      buff_t::decrement( stacks, value );
    else
      sim->print_debug( "Ice Floes removal ignored due to 500 ms protection" );
  }
};

struct icy_veins_t final : public buff_t
{
  icy_veins_t( mage_t* p ) :
    buff_t( p, "icy_veins", p->find_spell( 12472 ) )
  {
    set_default_value_from_effect( 1 );
    set_cooldown( 0_ms );
    set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    modify_duration( p->talents.thermal_void->effectN( 2 ).time_value() );
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    auto p = debug_cast<mage_t*>( player );
    if ( p->talents.thermal_void.ok() && duration == 0_ms )
      p->sample_data.icy_veins_duration->add( elapsed( sim->current_time() ).total_seconds() );

    p->buffs.slick_ice->expire();
  }
};

struct incanters_flow_t final : public buff_t
{
  incanters_flow_t( mage_t* p ) :
    buff_t( p, "incanters_flow", p->find_spell( 116267 ) )
  {
    set_duration( 0_ms );
    set_period( p->talents.incanters_flow->effectN( 1 ).period() );
    set_chance( p->talents.incanters_flow.ok() );
    set_default_value_from_effect( 1 );
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

    // The hidden "Flowing Thoughts" buff counts as an actual Arcane spell for DC.
    set_stack_change_callback( [ p ] ( buff_t*, int old, int cur )
    { if ( old == 3 && cur == 4 ) p->trigger_disciplinary_command( SCHOOL_ARCANE ); } );
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

struct rune_of_power_t final : public buff_t
{
  rune_of_power_t( mage_t* p ) :
    buff_t( p, "rune_of_power", p->find_spell( 116014 ) )
  {
    set_default_value_from_effect( 1 );
    set_chance( p->talents.rune_of_power.ok() );
    // Rune of Power has a tiny application delay in game. However, the delay
    // would add quite a lot of complexity to the APLs. It also rarely matters
    // in terms of modeling, so we treat Rune of Power as a normal activated buff.
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    auto p = debug_cast<mage_t*>( player );
    p->state.distance_from_rune = 0.0;
    p->trigger_disciplinary_command( data().get_school_type() );

    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    // When the Rune of Power buff fades at the same time as its area trigger, there is a
    // chance that the buff will fade first and the area trigger will reapply the buff for
    // an instant, which counts as executing an Arcane spell.
    if ( duration == 0_ms && rng().roll( 0.5 ) )
      debug_cast<mage_t*>( player )->trigger_disciplinary_command( data().get_school_type() );
  }
};

struct mirrors_of_torment_t final : public buff_t
{
  cooldown_t* icd;
  int successful_triggers;

  // Spec-specific effects
  timespan_t reduction;

  mirrors_of_torment_t( mage_td_t* td ) :
    buff_t( *td, "mirrors_of_torment", td->source->find_spell( 314793 ) ),
    icd( td->source->get_cooldown( "mirrors_of_torment_icd" ) ),
    successful_triggers(),
    reduction()
  {
    set_cooldown( 0_ms );
    set_reverse( true );
    // Stacks are handled manually by the tick callback.
    set_freeze_stacks( true );
    icd->duration = data().internal_cooldown();

    auto p = debug_cast<mage_t*>( source );
    if ( p->options.mirrors_of_torment_interval <= 0_ms )
      return;

    switch ( p->specialization() )
    {
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
        p->action.tormenting_backlash->execute_on_target( player );
      else
        p->action.agonizing_backlash->execute_on_target( player );

      p->buffs.siphoned_malice->trigger();

      switch ( p->specialization() )
      {
        case MAGE_ARCANE:
          p->trigger_delayed_buff( p->buffs.clearcasting, 1.0, 0_ms );
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

  // Damage multiplier that must be factored out when storing Touch of the Magi damage.
  double totm_factor;

  mage_spell_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ),
    frozen(),
    frozen_multiplier( 1.0 ),
    totm_factor( 1.0 )
  { }

  void initialize() override
  {
    action_state_t::initialize();
    frozen = 0U;
    frozen_multiplier = 1.0;
    totm_factor = 1.0;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " frozen=";

    std::streamsize ss = s.precision();
    s.precision( 4 );

    if ( frozen )
    {
      std::string str;

      auto concat_flag_str = [ this, &str ] ( std::string_view flag_str, frozen_flag_e flag )
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
    totm_factor       = mss->totm_factor;
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
    bool arcane_surge = true;
    bool bone_chilling = true;
    bool incanters_flow = true;
    bool invigorating_powder = true;
    bool rune_of_power = true;
    bool savant = false;

    bool deathborne = true;
    bool siphoned_malice = true;
    bool touch_of_ice = true;

    // Misc
    bool combustion = true;
    bool ice_floes = false;
    bool overflowing_energy = true;
    bool radiant_spark = true;
    bool shatter = true;
    bool shifting_power = true;
    bool time_manipulation = false;

    bool deathborne_cleave = false;
  } affected_by;

  struct triggers_t
  {
    bool chill = false;
    bool from_the_ashes = false;
    bool icy_propulsion = false;
    bool ignite = false;
    bool overflowing_energy = false;
    bool radiant_spark = false;
    bool touch_of_the_magi = true;

    target_trigger_type_e hot_streak = TT_NONE;
    target_trigger_type_e kindling = TT_NONE;

    bool icy_propulsion_conduit = true;
  } triggers;

  bool track_cd_waste;
  cooldown_waste_data_t* cd_waste;

public:
  mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
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

  const mage_td_t* find_td( const player_t* t ) const
  { return p()->find_target_data( t ); }

  mage_td_t* get_td( player_t* t )
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

    if ( affected_by.overflowing_energy )
      crit_bonus_multiplier *= 1.0 + p()->talents.overflowing_energy->effectN( 1 ).percent();

    // Save some CPU time by not computing frozen flags/frozen multiplier for Arcane and Fire.
    if ( harmful && affected_by.shatter && p()->specialization() == MAGE_FROST )
    {
      snapshot_flags |= STATE_FROZEN | STATE_FROZEN_MUL;
      update_flags   |= STATE_FROZEN | STATE_FROZEN_MUL;
    }

    if ( affected_by.time_manipulation && !range::contains( p()->time_manipulation_cooldowns, cooldown ) )
      p()->time_manipulation_cooldowns.push_back( cooldown );
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

    if ( affected_by.arcane_surge && p()->buffs.arcane_surge->check() )
      m *= 1.0 + p()->buffs.arcane_surge->data().effectN( 1 ).percent() + p()->talents.arcane_power->effectN( 2 ).percent();

    if ( affected_by.bone_chilling )
      m *= 1.0 + p()->buffs.bone_chilling->check_stack_value();

    if ( affected_by.incanters_flow )
      m *= 1.0 + p()->buffs.incanters_flow->check_stack_value();

    if ( affected_by.invigorating_powder )
      m *= 1.0 + p()->buffs.invigorating_powder->check_value();

    if ( affected_by.rune_of_power )
      m *= 1.0 + p()->buffs.rune_of_power->check_value();

    if ( affected_by.deathborne )
      m *= 1.0 + p()->buffs.deathborne->check_value();

    if ( affected_by.siphoned_malice )
      m *= 1.0 + p()->buffs.siphoned_malice->check_stack_value();

    if ( affected_by.touch_of_ice )
      m *= 1.0 + p()->buffs.touch_of_ice->check_value();

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

    if ( auto td = find_td( target ) )
    {
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

    if ( affected_by.overflowing_energy )
      c += p()->buffs.overflowing_energy->check_value();

    return c;
  }

  // Returns all currently active frozen effects as a bitfield (see frozen_flag_e).
  virtual unsigned frozen( const action_state_t* s ) const
  {
    const mage_td_t* td = find_td( s->target );

    if ( !td )
      return 0U;

    unsigned source = 0U;

    if ( td->debuffs.winters_chill->check() )
      source |= FF_WINTERS_CHILL;

    if ( td->debuffs.frozen->check() )
      source |= FF_ROOT;

    return source;
  }

  // Damage multiplier that applies only if the target is frozen.
  virtual double frozen_multiplier( const action_state_t* s ) const
  {
    double fm = 1.0;

    if ( get_school() == SCHOOL_FROST && cast_state( s )->frozen & FF_ROOT )
      fm *= 1.0 + p()->talents.subzero->effectN( 1 ).percent();

    return fm;
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    spell_t::snapshot_internal( s, flags, rt );

    if ( flags & STATE_FROZEN )
      cast_state( s )->frozen = frozen( s );

    if ( flags & STATE_FROZEN_MUL )
      cast_state( s )->frozen_multiplier = frozen_multiplier( s );

    if ( flags & ( STATE_TGT_MUL_DA | STATE_TGT_MUL_TA ) && p()->talents.touch_of_the_magi.ok() )
      cast_state( s )->totm_factor = composite_target_damage_vulnerability( s->target );
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

    if ( p()->buffs.foresight->check() )
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

    if ( p()->talents.clearcasting.ok() && harmful && current_resource() == RESOURCE_MANA )
    {
      // Mana spending required for 1% chance.
      double mana_step = p()->talents.clearcasting->cost( POWER_MANA ) * p()->resources.base[ RESOURCE_MANA ];
      mana_step /= p()->talents.clearcasting->effectN( 1 ).percent();
      double chance = 0.01 * last_resource_cost / mana_step;
      chance *= 1.0 + p()->talents.illuminated_thoughts->effectN( 1 ).percent();
      p()->trigger_delayed_buff( p()->buffs.clearcasting, chance );
    }

    if ( !background && affected_by.ice_floes && time_to_execute > 0_ms )
      p()->buffs.ice_floes->decrement();

    p()->trigger_disciplinary_command( get_school() );
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( s->result_total <= 0.0 )
      return;

    if ( p()->talents.icy_propulsion.ok() )
    {
      if ( triggers.icy_propulsion && s->result == RESULT_CRIT )
        p()->cooldowns.icy_veins->adjust( -p()->talents.icy_propulsion->effectN( 1 ).time_value() );
    }
    else
    {
      if ( triggers.icy_propulsion_conduit && s->result == RESULT_CRIT && p()->buffs.icy_veins->check() )
        p()->cooldowns.icy_veins->adjust( -0.1 * p()->conduits.icy_propulsion.time_value( conduit_data_t::S ) );
    }

    if ( callbacks && p()->talents.overflowing_energy.ok() && s->result_type == result_amount_type::DMG_DIRECT )
    {
      // TODO: should we use events here just like with Fevered Incantation? OE doesn't trigger from AoE spells
      // so multiple simultaneous triggers happen rather rarely
      if ( s->result == RESULT_CRIT )
        p()->buffs.overflowing_energy->expire();
      else if ( triggers.overflowing_energy )
        p()->buffs.overflowing_energy->trigger();
    }

    if ( s->result_type == result_amount_type::DMG_DIRECT )
    {
      if ( p()->talents.fevered_incantation.ok() )
      {
        if ( s->result == RESULT_CRIT )
          make_event( *sim, [ this ] { p()->buffs.fevered_incantation->trigger(); } );
        else
          make_event( *sim, [ this ] { p()->buffs.fevered_incantation->expire(); } );
      }
      else if ( p()->runeforge.fevered_incantation.ok() )
      {
        if ( s->result == RESULT_CRIT )
          make_event( *sim, [ this ] { p()->buffs.runeforge_fevered_incantation->trigger(); } );
        else
          make_event( *sim, [ this ] { p()->buffs.runeforge_fevered_incantation->expire(); } );
      }
    }
  }

  void assess_damage( result_amount_type rt, action_state_t* s ) override
  {
    spell_t::assess_damage( rt, s );

    if ( s->result_total <= 0.0 )
      return;

    if ( auto td = find_td( s->target ) )
    {
      auto spark_dot = td->dots.radiant_spark;
      if ( triggers.radiant_spark && spark_dot->is_ticking() )
      {
        auto spark_debuff = td->debuffs.radiant_spark_vulnerability;

        // Handle Harmonic Echo before changing the stack number
        if ( ( p()->talents.harmonic_echo.ok() || p()->runeforge.harmonic_echo.ok() ) && spark_debuff->check() )
        {
          double echo_pct = p()->talents.harmonic_echo.ok() ? p()->talents.harmonic_echo->effectN( 1 ).percent() : p()->runeforge.harmonic_echo->effectN( 1 ).percent();
          p()->action.harmonic_echo->execute_on_target( s->target, echo_pct * s->result_total );
        }

        if ( spark_debuff->at_max_stacks() )
        {
          spark_debuff->expire( p()->bugs ? 30_ms : 0_ms );
          // Prevent new applications of the vulnerability debuff until the DoT finishes ticking.
          spark_debuff->cooldown->start( spark_dot->remains() );
        }
        else
        {
          spark_debuff->trigger();
        }
      }

      auto totm = td->debuffs.touch_of_the_magi;
      if ( totm->check() && triggers.touch_of_the_magi )
      {
        // Touch of the Magi factors out debuffs with effect subtype 87 (Modify Damage Taken%), but only
        // if they increase damage taken. It does not factor out debuffs with effect subtype 270
        // (Modify Damage Taken% from Caster) or 271 (Modify Damage Taken% from Caster's Spells).
        totm->current_value += s->result_total / std::max( cast_state( s )->totm_factor, 1.0 );

        // Arcane Echo doesn't use the normal callbacks system (both in simc and in game). To prevent
        // loops, we need to explicitly check that the triggering action wasn't Arcane Echo.
        if ( p()->talents.arcane_echo.ok() && this != p()->action.arcane_echo )
          make_event( *sim, [ this, t = s->target ] { p()->action.arcane_echo->execute_on_target( t ); } );
      }
    }
  }

  void last_tick( dot_t* d ) override
  {
    spell_t::last_tick( d );

    if ( channeled && affected_by.ice_floes )
      p()->buffs.ice_floes->decrement();
  }

  void trigger_tracking_buff( buff_t* counter, buff_t* ready, int offset = 1, int stacks = 1 )
  {
    if ( ready->check() )
      return;

    if ( counter->at_max_stacks( offset + stacks - 1 ) )
    {
      counter->expire();
      ready->trigger();
    }
    else
    {
      counter->trigger( stacks );
    }
  }

  void trigger_deaths_fathom()
  {
    if ( p()->buffs.deathborne->check() )
      p()->buffs.deathborne->current_value += p()->runeforge.deaths_fathom->effectN( 2 ).percent();

    if ( p()->rppm.deaths_fathom->trigger() )
    {
      timespan_t d = p()->runeforge.deaths_fathom->effectN( 1 ).time_value();
      if ( p()->buffs.lead_by_example && !p()->buffs.deathborne->check() )
        p()->buffs.lead_by_example->trigger( d );

      p()->buffs.deathborne->extend_duration_or_trigger( d );
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

    if ( a->affected_by.shatter && p->talents.shatter.ok() )
    {
      // Multiplier is not in spell data, apparently.
      c *= 1.5;
      c += p->talents.shatter->effectN( 2 ).percent();
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

  // Tier 29 4pc support
  int num_targets_crit;

  arcane_mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    cost_reductions(),
    num_targets_crit()
  { }

  void execute() override
  {
    num_targets_crit = 0;
    mage_spell_t::execute();
  }

  void schedule_travel( action_state_t* s ) override
  {
    mage_spell_t::schedule_travel( s );
    if ( s->result == RESULT_CRIT ) num_targets_crit++;
  }

  void consume_cost_reductions() override
  {
    // Consume first applicable buff and then stop.
    for ( auto cr : cost_reductions )
    {
      int before = cr->check();
      if ( before )
      {
        cr->decrement();
        // Nether Precision is only triggered if the buff was actually decremented.
        // This is relevant when the player uses Expanded Potential.
        if ( cr == p()->buffs.clearcasting && cr->check() < before )
        {
          p()->buffs.nether_precision->trigger();
          p()->buffs.nether_precision_conduit->trigger();
        }
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
    double base = p()->buffs.arcane_charge->data().effectN( arcane_barrage ? 2 : 1 ).percent();

    double mastery = p()->cache.mastery() * p()->spec.savant->effectN( arcane_barrage ? 3 : 2 ).mastery_value();
    mastery *= 1.0 + p()->talents.prodigious_savant->effectN( arcane_barrage ? 2 : 1 ).percent();

    return 1.0 + p()->buffs.arcane_charge->check() * (base + mastery);
  }
};


// ==========================================================================
// Fire Mage Spell
// ==========================================================================

struct fire_mage_spell_t : public mage_spell_t
{
  fire_mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
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
        && p()->talents.kindling.ok() )
      {
        timespan_t amount = p()->talents.kindling->effectN( 1 ).time_value();
        p()->cooldowns.combustion->adjust( -amount );
        if ( !p()->buffs.combustion->check() )
          p()->expression_support.kindling_reduction += amount;
      }

      if ( triggers.from_the_ashes
        && s->result == RESULT_CRIT
        && p()->talents.from_the_ashes.ok()
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
    if ( !p->spec.pyroblast_clearcasting_driver->ok() )
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
          // If the player knew about Heating Up and converted to Hot Streak
          // with a guaranteed crit, let them react to the Hot Streak instantly.
          if ( guaranteed && hu_react )
            p->buffs.hot_streak->predict();

          // If Scorch generates Hot Streak and the actor is currently casting Pyroblast
          // or Flamestrike, the game will immediately finish the cast. This is presumably
          // done to work around the buff application delay inside Combustion or with
          // Searing Touch active. The following code is a huge hack.
          if ( id == 2948 && p->executing && ( p->executing->id == 11366 || p->executing->id == 2120 ) )
          {
            assert( p->executing->execute_event );
            p->current_execute_type = execute_type::FOREGROUND;
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
  {
    double m = 1.0;

    m *= 1.0 + p()->talents.wildfire->effectN( 2 ).percent();

    if ( !p()->buffs.combustion->check() )
      m *= 1.0 + p()->talents.master_of_flame->effectN( 1 ).percent();

    return m;
  }

  void trigger_ignite( action_state_t* s )
  {
    if ( !p()->spec.ignite->ok() )
      return;

    double m = s->target_da_multiplier;
    if ( m <= 0.0 )
      return;

    double trigger_dmg = s->result_total;

    // The extra crit damage from Overflowing Energy does not contribute to Ignite, factor it out.
    if ( p()->bugs && s->result == RESULT_CRIT )
    {
      double spell_bonus = composite_crit_damage_bonus_multiplier() * composite_target_crit_damage_bonus_multiplier( s->target );
      trigger_dmg /= 1.0 + s->result_crit_bonus;
      trigger_dmg *= 1.0 + s->result_crit_bonus / spell_bonus;
    }

    double amount = trigger_dmg / m * p()->cache.mastery_value();
    if ( amount <= 0.0 )
      return;

    amount *= composite_ignite_multiplier( s );

    if ( !p()->action.ignite->get_dot( s->target )->is_ticking() )
      p()->procs.ignite_applied->occur();

    residual_action::trigger( p()->action.ignite, s->target, amount );
  }

  bool firestarter_active( player_t* target ) const
  {
    if ( !p()->talents.firestarter.ok() )
      return false;

    return target->health_percentage() > 100.0 - ( 100.0 - p()->talents.firestarter->effectN( 1 ).base_value() ) * p()->options.firestarter_duration_multiplier;
  }

  bool searing_touch_active( player_t* target ) const
  {
    if ( !p()->talents.searing_touch.ok() )
      return false;

    return target->health_percentage() < p()->talents.searing_touch->effectN( 1 ).base_value() * p()->options.searing_touch_duration_multiplier;
  }

  bool improved_scorch_active( player_t* target ) const
  {
    if ( !p()->talents.improved_scorch.ok() )
      return false;

    // Because this is currently the same as Searing Touch, mage.searing_touch_duration_multiplier is applied here.
    return target->health_percentage() < p()->talents.improved_scorch->effectN( 2 ).base_value() * p()->options.searing_touch_duration_multiplier;
  }

  void trigger_molten_skyfall()
  {
    trigger_tracking_buff( p()->buffs.molten_skyfall, p()->buffs.molten_skyfall_ready, 2 );
  }

  bool consume_molten_skyfall( player_t* target )
  {
    if ( !p()->buffs.molten_skyfall_ready->check() )
      return false;
    p()->buffs.molten_skyfall_ready->expire();
    p()->action.firefall_meteor->execute_on_target( target );
    return true;
  }

  void trigger_firefall()
  {
    trigger_tracking_buff( p()->buffs.firefall, p()->buffs.firefall_ready, 2 );
  }

  bool consume_firefall( player_t* target )
  {
    if ( !p()->buffs.firefall_ready->check() )
      return false;
    p()->buffs.firefall_ready->expire();
    p()->action.firefall_meteor->execute_on_target( target );
    return true;
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

  hot_streak_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
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
    if ( p()->buffs.hot_streak->check() || p()->buffs.hyperthermia->check() || p()->buffs.firestorm->check() )
      return 0_ms;

    return fire_mage_spell_t::execute_time();
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    debug_cast<hot_streak_state_t*>( s )->hot_streak = last_hot_streak;
    fire_mage_spell_t::snapshot_state( s, rt );
  }

  double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    c += p()->buffs.hyperthermia->check_value();
    c += p()->buffs.firestorm->check_value();

    return c;
  }

  double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( last_hot_streak )
      am *= 1.0 + p()->sets->set( MAGE_FIRE, T29, B2 )->effectN( 1 ).percent();

    return am;
  }

  double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_ignite_multiplier( s );

    if ( debug_cast<const hot_streak_state_t*>( s )->hot_streak )
      m *= 2.0;

    return m;
  }

  void schedule_execute( action_state_t* s ) override
  {
    fire_mage_spell_t::schedule_execute( s );
    last_hot_streak = p()->buffs.hot_streak->up();
  }

  void execute() override
  {
    // TODO: instant non-HS flamestrike seems to actually trigger SKB combustion, pyroblast doesn't
    if ( time_to_execute > 0_ms && p()->buffs.sun_kings_blessing_ready->check() )
    {
      p()->buffs.sun_kings_blessing_ready->expire( p()->bugs ? 30_ms : 0_ms );
      p()->buffs.combustion->extend_duration_or_trigger( 1000 * p()->talents.sun_kings_blessing->effectN( 2 ).time_value() );
    }

    if ( time_to_execute > 0_ms && p()->buffs.runeforge_sun_kings_blessing_ready->check() )
    {
      p()->buffs.runeforge_sun_kings_blessing_ready->expire( p()->bugs ? 30_ms : 0_ms );
      p()->buffs.combustion->extend_duration_or_trigger( 1000 * p()->runeforge.sun_kings_blessing->effectN( 2 ).time_value() );
    }

    fire_mage_spell_t::execute();

    if ( last_hot_streak )
    {
      p()->buffs.hot_streak->decrement();
      p()->buffs.pyroclasm->trigger();
      p()->buffs.firemind->trigger();

      trigger_tracking_buff( p()->buffs.sun_kings_blessing, p()->buffs.sun_kings_blessing_ready );
      trigger_tracking_buff( p()->buffs.runeforge_sun_kings_blessing, p()->buffs.runeforge_sun_kings_blessing_ready );

      if ( rng().roll( p()->talents.pyromaniac->effectN( 1 ).percent() ) )
      {
        p()->procs.hot_streak->occur();
        p()->procs.hot_streak_pyromaniac->occur();
        p()->buffs.hot_streak->trigger();
      }
    }
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

  frost_mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    calculate_on_impact(),
    consumes_winters_chill(),
    proc_brain_freeze(),
    proc_fof(),
    proc_winters_chill_consumed(),
    track_shatter(),
    shatter_source(),
    impact_flags()
  { }

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
      proc_winters_chill_consumed = p()->get_proc( fmt::format( "Winter's Chill stacks consumed by {}", data().name_cstr() ) );

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
      p()->expression_support.remaining_winters_chill = std::max( p()->expression_support.remaining_winters_chill - 1, 0 );
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

      // Spells that calculate on impact actually execute a second spell when they impact.
      p()->trigger_disciplinary_command( get_school() );
    }

    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( s->chain_target == 0 )
        record_shatter_source( s, shatter_source );

      // TODO: should this be moved to mage_spell_t?
      if ( triggers.chill )
        trigger_chill_effect( s );

      if ( auto td = find_td( s->target ) )
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

  void trigger_chill_effect( const action_state_t* s )
  {
    // TODO: double check if frostbite and bone chilling trigger from the same spells
    p()->buffs.bone_chilling->trigger();
    if ( p()->rng().roll( p()->talents.frostbite->proc_chance() ) )
      p()->trigger_crowd_control( s, MECHANIC_ROOT, 0.5_s ); // Frostbite only has the initial grace period
  }

  void trigger_cold_front()
  {
    trigger_tracking_buff( p()->buffs.cold_front, p()->buffs.cold_front_ready, 2 );
  }

  bool consume_cold_front( player_t* target )
  {
    if ( !p()->buffs.cold_front_ready->check() )
      return false;
    p()->buffs.cold_front_ready->expire();
    p()->action.cold_front_frozen_orb->execute_on_target( target );
    return true;
  }
};

// Icicles ==================================================================

struct icicle_t final : public frost_mage_spell_t
{
  icicle_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 148022 ) )
  {
    background = track_shatter = triggers.icy_propulsion = true;
    callbacks = false;
    // TODO: This base damage may have been removed, but this should be verified not just using tooltip values.
    base_dd_min = base_dd_max = 1.0;
    base_multiplier *= 1.0 + p->talents.flash_freeze->effectN( 2 ).percent();
    crit_bonus_multiplier *= 1.0 + p->talents.piercing_cold->effectN( 1 ).percent();

    if ( p->talents.splitting_ice.ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_multiplier *= 1.0 + p->talents.splitting_ice->effectN( 3 ).percent();
      base_aoe_multiplier *= p->talents.splitting_ice->effectN( 2 ).percent();
    }
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
      p()->trigger_fof( p()->talents.flash_freeze->effectN( 1 ).percent(), p()->procs.fingers_of_frost_flash_freeze );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  { return frost_mage_spell_t::spell_direct_power_coefficient( s ) + icicle_sp_coefficient(); }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t final : public arcane_mage_spell_t
{
  presence_of_mind_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.presence_of_mind )
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
    p()->buffs.presence_of_mind->trigger();
  }
};

// Ignite Spell =============================================================

// TODO: 2022-10-02 Phoenix Reborn is not flagged to scale with spec auras, Rune of Power, etc.
// If this is fixed, change spell_t to mage_spell_t or fire_mage_spell_t.
struct phoenix_reborn_t final : public spell_t
{
  phoenix_reborn_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 383479 ) )
  {
    background = true;
    callbacks = false;
  }
};

struct ignite_t final : public residual_action_t
{
  action_t* phoenix_reborn;

  ignite_t( std::string_view n, mage_t* p ) :
    residual_action_t( n, p, p->find_spell( 12654 ) )
  {
    callbacks = true;
    affected_by.radiant_spark = false;

    if ( p->talents.phoenix_reborn.ok() )
      phoenix_reborn = get_action<phoenix_reborn_t>( "phoenix_reborn", p );
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
      p()->action.conflagration_flare_up->execute_on_target( d->target );

    if ( p()->cooldowns.fervent_flickering->up() && rng().roll( p()->talents.fervent_flickering->proc_chance() ) )
    {
      p()->cooldowns.fervent_flickering->start( p()->talents.fervent_flickering->internal_cooldown() );
      cooldown_t* cd = p()->cooldowns.fire_blast;
      if ( cd->action )
        cd->adjust( -1000 * p()->talents.fervent_flickering->effectN( 1 ).time_value() / cd->action->recharge_rate_multiplier( *cd ) );
    }

    if ( p()->cooldowns.phoenix_reborn->up() && rng().roll( p()->talents.phoenix_reborn->proc_chance() ) )
    {
      p()->cooldowns.phoenix_reborn->start( p()->talents.phoenix_reborn->internal_cooldown() );
      cooldown_t* cd = p()->cooldowns.phoenix_flames;
      if ( cd->action )
        cd->adjust( -1000 * p()->talents.fervent_flickering->effectN( 1 ).time_value() / cd->action->recharge_rate_multiplier( *cd ) );
      phoenix_reborn->execute_on_target( d->target );
    }
  }

  void impact( action_state_t* s ) override
  {
    // Residual periodic actions + tick_zero does not work
    assert( !tick_zero );

    dot_t* dot = get_dot( s->target );
    double current_amount = 0.0;
    double old_amount = 0.0;
    double ticks_left = 0.0;
    auto dot_state = debug_cast<residual_action::residual_periodic_state_t*>( dot->state );

    // If dot is ticking get current residual pool before we overwrite it
    if ( dot->is_ticking() )
    {
      old_amount = dot_state->tick_amount;
      ticks_left = dot->ticks_left_fractional();
      current_amount = old_amount * ticks_left;
    }

    // Add new amount to residual pool
    current_amount += s->result_amount;

    // Trigger the dot, refreshing its duration or starting it
    trigger_dot( s );

    if ( !dot_state )
      dot_state = debug_cast<residual_action::residual_periodic_state_t*>( dot->state );

    // After a refresh the number of ticks should always be an integer, but Ignite uses the fractional value here too for consistency.
    dot_state->tick_amount = current_amount / dot->ticks_left_fractional();

    sim->print_debug( "{} {} impact amount={} old_total={} old_ticks={} old_tick={} current_total={} current_ticks={} current_tick={}",
      player->name(), name(), s->result_amount, old_amount * ticks_left, ticks_left, ticks_left > 0 ? old_amount : 0,
      current_amount, dot->ticks_left_fractional(), dot_state->tick_amount );
  }
};

// Arcane Barrage Spell =====================================================

struct arcane_barrage_t final : public arcane_mage_spell_t
{
  int artifice_of_the_archmage_charges;

  arcane_barrage_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.arcane_barrage ),
    artifice_of_the_archmage_charges( as<int>( p->find_spell( 337244 )->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    cooldown->hasted = true;
    base_aoe_multiplier *= data().effectN( 2 ).percent();
    triggers.radiant_spark = triggers.overflowing_energy = true;
  }

  int n_targets() const override
  {
    int charges = p()->buffs.arcane_charge->check();
    return p()->talents.arcing_cleave.ok() && charges > 0 ? charges + 1 : 0;
  }

  void execute() override
  {
    p()->benefits.arcane_charge.arcane_barrage->update();

    arcane_mage_spell_t::execute();

    double mana_pct = p()->buffs.arcane_charge->check() * 0.01 * p()->talents.mana_adept->effectN( 1 ).percent();
    p()->resource_gain( RESOURCE_MANA, p()->resources.max[ RESOURCE_MANA ] * mana_pct, p()->gains.arcane_barrage, this );

    p()->buffs.arcane_tempo->trigger();
    p()->buffs.arcane_charge->expire();
    p()->buffs.arcane_harmony->expire();
    p()->buffs.bursting_energy->expire();
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.chrono_shift->trigger();
      // Multiply by 0.1 because for this data a value of 100 means 10%.
      if ( rng().roll( 0.1 * p()->conduits.artifice_of_the_archmage.percent() ) )
        p()->trigger_arcane_charge( artifice_of_the_archmage_charges );
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = arcane_mage_spell_t::composite_da_multiplier( s );

    m *= 1.0 + s->n_targets * p()->talents.resonance->effectN( 1 ).percent();

    // TODO: check what happens when you have both the talent and the legendary
    // the legendary is missing from the SpellReplacement table, so presumably you get both
    if ( s->target->health_percentage() < p()->talents.arcane_bombardment->effectN( 1 ).base_value() )
      m *= 1.0 + p()->talents.arcane_bombardment->effectN( 2 ).percent();
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

  double composite_crit_chance() const override
  {
    double c = arcane_mage_spell_t::composite_crit_chance();

    c += p()->buffs.bursting_energy->check_stack_value();

    return c;
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t final : public arcane_mage_spell_t
{
  arcane_blast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Blast" ) )
  {
    parse_options( options_str );
    cost_reductions = { p->buffs.rule_of_threes };
    triggers.radiant_spark = triggers.overflowing_energy = true;
    affected_by.deathborne_cleave = true;
    base_multiplier *= 1.0 + p->talents.crackling_energy->effectN( 1 ).percent();
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
  }

  double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    c *= 1.0 + p()->buffs.arcane_charge->check()
             * p()->buffs.arcane_charge->data().effectN( 5 ).percent();

    return c;
  }

  void execute() override
  {
    p()->benefits.arcane_charge.arcane_blast->update();

    arcane_mage_spell_t::execute();

    p()->trigger_arcane_charge();

    if ( rng().roll( p()->talents.impetus->effectN( 1 ).percent() ) )
    {
      if ( p()->buffs.arcane_charge->at_max_stacks() )
        p()->buffs.impetus->trigger();
      else
        p()->trigger_arcane_charge();
    }

    if ( p()->buffs.presence_of_mind->up() )
      p()->buffs.presence_of_mind->decrement();

    p()->buffs.concentration->trigger();
    p()->buffs.expanded_potential->trigger();

    if ( num_targets_crit > 0 )
      p()->buffs.bursting_energy->trigger();
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
    // TODO: This was fixed with the talent. Does it still happen with the conduit?
    if ( result_is_hit( s->result ) )
    {
      trigger_deaths_fathom();

      p()->buffs.nether_precision->decrement();

      if ( p()->conduits.nether_precision.ok() && !p()->talents.nether_precision.ok() )
        make_event( *sim, 15_ms, [ this ] { p()->buffs.nether_precision_conduit->decrement(); } );
    }
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_multiplier();
    am *= 1.0 + p()->buffs.nether_precision->check_value();
    am *= 1.0 + p()->buffs.nether_precision_conduit->check_value();

    return am;
  }

  double composite_crit_chance() const override
  {
    double c = arcane_mage_spell_t::composite_crit_chance();

    c += p()->buffs.arcane_charge->check() * p()->sets->set( MAGE_ARCANE, T29, B2 )->effectN( 1 ).percent();

    return c;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.presence_of_mind->check() )
      return 0_ms;

    timespan_t t = arcane_mage_spell_t::execute_time();

    t *= 1.0 + p()->buffs.arcane_charge->check()
             * p()->buffs.arcane_charge->data().effectN( 4 ).percent();

    return t;
  }
};

// Arcane Explosion Spell ===================================================

struct arcane_explosion_t final : public arcane_mage_spell_t
{
  arcane_explosion_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_class_spell( "Arcane Explosion" ) )
  {
    parse_options( options_str );
    aoe = -1;
    cost_reductions = { p->buffs.clearcasting };
    affected_by.savant = triggers.radiant_spark = true;
    base_multiplier *= 1.0 + p->talents.crackling_energy->effectN( 1 ).percent();
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( !target_list().empty() )
      p()->trigger_arcane_charge();

    if ( num_targets_hit >= as<int>( p()->talents.reverberate->effectN( 2 ).base_value() )
      && rng().roll( p()->talents.reverberate->effectN( 1 ).percent() ) )
    {
      p()->trigger_arcane_charge();
    }

    if ( num_targets_crit > 0 )
      p()->buffs.bursting_energy->trigger();
  }

  double composite_crit_chance() const override
  {
    double c = arcane_mage_spell_t::composite_crit_chance();

    c += p()->buffs.arcane_charge->check() * p()->sets->set( MAGE_ARCANE, T29, B2 )->effectN( 2 ).percent();

    return c;
  }
};

// Arcane Familiar Spell ====================================================

struct arcane_assault_t final : public arcane_mage_spell_t
{
  arcane_assault_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 225119 ) )
  {
    background = true;
    callbacks = false;
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
  }
};

struct arcane_familiar_t final : public arcane_mage_spell_t
{
  arcane_familiar_t( std::string_view n, mage_t* p, std::string_view options_str ) :
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

struct arcane_intellect_t final : public mage_spell_t
{
  arcane_intellect_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Arcane Intellect" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;

    if ( sim->overrides.arcane_intellect )
      background = true;
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( !sim->overrides.arcane_intellect )
      sim->auras.arcane_intellect->trigger();
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t final : public arcane_mage_spell_t
{
  arcane_missiles_tick_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 7268 ) )
  {
    background = true;
    affected_by.savant = triggers.radiant_spark = triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->talents.improved_arcane_missiles->effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.arcane_harmony->trigger();
      if ( p()->buffs.orb_barrage_ready->check() )
      {
        // TODO: Does Arcane Missiles have to hit to trigger Orb Barrage?
        p()->buffs.orb_barrage_ready->expire();
        p()->action.orb_barrage_arcane_orb->execute_on_target( s->target );
      }
    }
  }
};

struct am_state_t final : public mage_spell_state_t
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

struct arcane_missiles_t final : public arcane_mage_spell_t
{
  double cc_duration_reduction;
  double cc_tick_time_reduction;

  arcane_missiles_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.arcane_missiles )
  {
    parse_options( options_str );
    may_miss = false;
    // In the game, the tick zero of Arcane Missiles actually happens after 100 ms
    tick_zero = channeled = true;
    tick_action = get_action<arcane_missiles_tick_t>( "arcane_missiles_tick", p );
    cost_reductions = { p->buffs.clearcasting, p->buffs.rule_of_threes };

    const auto& cc_data = p->buffs.clearcasting_channel->data();
    cc_duration_reduction  = cc_data.effectN( 1 ).percent();
    cc_tick_time_reduction = cc_data.effectN( 2 ).percent() + p->talents.amplification->effectN( 1 ).percent();
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
    debug_cast<am_state_t*>( s )->tick_time_multiplier = p()->buffs.clearcasting_channel->check() ? 1.0 + cc_tick_time_reduction : 1.0;
    arcane_mage_spell_t::snapshot_state( s, rt );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t full_duration = dot_duration * s->haste;

    if ( p()->buffs.clearcasting_channel->check() )
      full_duration *= 1.0 + cc_duration_reduction;

    return full_duration;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = arcane_mage_spell_t::tick_time( s );

    t *= debug_cast<const am_state_t*>( s )->tick_time_multiplier;

    return t;
  }

  void execute() override
  {
    int stacks = p()->buffs.clearcasting->check() ? as<int>( p()->talents.orb_barrage->effectN( 2 ).base_value() ) : 1;
    trigger_tracking_buff( p()->buffs.orb_barrage, p()->buffs.orb_barrage_ready, 2, stacks );

    // Set up the hidden Clearcasting buff before executing the spell
    // so that tick time and dot duration have the correct values.
    if ( p()->buffs.clearcasting->check() )
    {
      p()->buffs.clearcasting_channel->trigger();
      p()->trigger_time_manipulation();
    }
    else
    {
      p()->buffs.clearcasting_channel->expire();
    }

    arcane_mage_spell_t::execute();
  }

  void trigger_dot( action_state_t* s ) override
  {
    dot_t* d = get_dot( s->target );
    timespan_t tick_remains = d->time_to_next_full_tick();
    timespan_t tt = tick_time( s );
    int ticks = 0;

    // There is a bug when chaining where instead of using the base
    // duration to calculate the rounded number of ticks, the time
    // left in the previous tick can add extra ticks if there is
    // more than the new tick time remaining before that tick.
    if ( p()->bugs && tick_remains > 0_ms )
    {
      timespan_t mean_delay = p()->options.arcane_missiles_chain_delay;
      timespan_t delay = rng().gauss( mean_delay, mean_delay * p()->options.arcane_missiles_chain_relstddev );
      timespan_t chain_remains = tick_remains - clamp( delay, 0_ms, tick_remains - 1_ms );
      // If tick_remains == 0_ms, this would subtract 1 from ticks.
      // This is not implemented in simc, but this actually appears
      // to happen in game, which can result in missing ticks if
      // the player refreshes the cast too quickly after a tick.
      ticks += as<int>( std::ceil( chain_remains / tt ) - 1 );
    }

    arcane_mage_spell_t::trigger_dot( s );

    // AM channel duration is a bit fuzzy, it will go above or below the
    // standard duration to make sure it has the correct number of ticks.
    ticks += as<int>( std::round( ( d->remains() - tick_remains ) / tt ) );
    timespan_t new_remains = ticks * tt + tick_remains;

    d->adjust_duration( new_remains - d->remains(), timespan_t::min(), 0, false );
  }

  bool usable_moving() const override
  {
    if ( p()->talents.slipstream.ok() && ( p()->buffs.clearcasting->check() || p()->buffs.clearcasting_channel->check() ) )
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

struct arcane_orb_bolt_t final : public arcane_mage_spell_t
{
  arcane_orb_bolt_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 153640 ) )
  {
    background = true;
    affected_by.savant = triggers.radiant_spark = true;
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    // AC is triggered even if the spell misses.
    p()->trigger_arcane_charge();
  }
};

struct arcane_orb_t final : public arcane_mage_spell_t
{
  arcane_orb_t( std::string_view n, mage_t* p, std::string_view options_str, bool orb_barrage = false ) :
    arcane_mage_spell_t( n, p, orb_barrage ? p->find_spell( 153626 ) : p->talents.arcane_orb )
  {
    parse_options( options_str );
    may_miss = may_crit = false;
    aoe = -1;
    cooldown->charges += as<int>( p->talents.charged_orb->effectN( 1 ).base_value() );

    impact_action = get_action<arcane_orb_bolt_t>( orb_barrage ? "orb_barrage_arcane_orb_bolt" : "arcane_orb_bolt", p );
    add_child( impact_action );

    if ( orb_barrage )
    {
      background = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();
    p()->trigger_arcane_charge();
  }
};

// Arcane Surge Spell =======================================================

struct arcane_surge_t final : public arcane_mage_spell_t
{
  double energize_pct;

  arcane_surge_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.arcane_surge ),
    energize_pct( p->find_spell( 365265 )->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    triggers.radiant_spark = true;
    // TODO: Arcane Surge is currently fully capped at 5 targets instead of dealing reduced damage beyond 5 targets like the tooltip says.
    if ( !p->bugs )
    {
      aoe = -1;
      reduced_aoe_targets = as<double>( data().max_targets() );
    }
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->resources.pct( RESOURCE_MANA ) * ( data().effectN( 2 ).base_value() - 1.0 );

    return am;
  }

  void execute() override
  {
    p()->buffs.arcane_surge->trigger();
    p()->buffs.rune_of_power->trigger();

    arcane_mage_spell_t::execute();

    p()->resource_gain( RESOURCE_MANA, p()->resources.max[ RESOURCE_MANA ] * energize_pct, p()->gains.arcane_surge, this );
  }
};

// Blast Wave Spell =========================================================

struct blast_wave_t final : public fire_mage_spell_t
{
  blast_wave_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.blast_wave )
  {
    parse_options( options_str );
    aoe = -1;
    triggers.radiant_spark = true;
    cooldown->duration += p->talents.volatile_detonation->effectN( 1 ).time_value();
  }
};

// Blink Spell ==============================================================

struct blink_t final : public mage_spell_t
{
  blink_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Blink" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    base_teleport_distance = data().effectN( 1 ).radius_max();
    movement_directionality = movement_direction_type::OMNI;

    if ( p->talents.flow_of_time.ok() )
      cooldown->duration += p->talents.flow_of_time->effectN( 1 ).time_value();
    else
      cooldown->duration += p->conduits.flow_of_time.time_value();

    if ( p->talents.shimmer.ok() )
      background = true;
  }
};

// Blizzard Spell ===========================================================

struct blizzard_shard_t final : public frost_mage_spell_t
{
  blizzard_shard_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 190357 ) )
  {
    aoe = -1;
    background = ground_aoe = triggers.chill = true;
    triggers.icy_propulsion_conduit = false;
    base_multiplier *= 1.0 + p->conduits.shivering_core.percent();
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
      timespan_t reduction = -10 * num_targets_hit * p()->talents.ice_caller->effectN( 1 ).time_value();
      p()->cooldowns.frozen_orb->adjust( reduction, false );
    }
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    // TODO: should this be moved to execute?
    if ( result_is_hit( s->result ) && p()->cooldowns.snowstorm->up() && rng().roll( p()->talents.snowstorm->proc_chance() ) )
    {
      p()->cooldowns.snowstorm->start( p()->talents.snowstorm->internal_cooldown() );
      p()->buffs.snowstorm->trigger();
    }
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.freezing_rain->check_value();

    return am;
  }
};

struct blizzard_t final : public frost_mage_spell_t
{
  action_t* blizzard_shard;

  blizzard_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.blizzard ),
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

struct cold_snap_t final : public frost_mage_spell_t
{
  cold_snap_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.cold_snap )
  {
    parse_options( options_str );
    harmful = false;
  };

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->cooldowns.cone_of_cold->reset( false );
    p()->cooldowns.frost_nova->reset( false );
  }
};

// Combustion Spell =========================================================

struct combustion_t final : public fire_mage_spell_t
{
  combustion_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.combustion )
  {
    parse_options( options_str );
    dot_duration = 0_ms;
    harmful = false;
    usable_while_casting = true;
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    p()->buffs.combustion->trigger();
    p()->buffs.rune_of_power->trigger();
    p()->buffs.wildfire->trigger();
    p()->expression_support.kindling_reduction = 0_ms;
  }
};

// Comet Storm Spell ========================================================

struct comet_storm_projectile_t final : public frost_mage_spell_t
{
  comet_storm_projectile_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 153596 ) )
  {
    aoe = -1;
    background = triggers.radiant_spark = true;
  }
};

struct comet_storm_t final : public frost_mage_spell_t
{
  static constexpr int pulse_count = 7;
  static constexpr timespan_t pulse_time = 0.2_s;

  action_t* projectile;

  comet_storm_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.comet_storm ),
    projectile( get_action<comet_storm_projectile_t>( "comet_storm_projectile", p ) )
  {
    parse_options( options_str );
    may_miss = may_crit = affected_by.shatter = false;
    add_child( projectile );
    travel_delay = p->find_spell( 228601 )->missile_speed();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    p()->ground_aoe_expiration[ AOE_COMET_STORM ] = sim->current_time() + pulse_count * pulse_time;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( pulse_time )
      .target( s->target )
      .n_pulses( pulse_count )
      .action( projectile ) );
  }
};

// Cone of Cold Spell =======================================================

struct cone_of_cold_t final : public frost_mage_spell_t
{
  cone_of_cold_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_class_spell( "Cone of Cold" ) )
  {
    parse_options( options_str );
    aoe = -1;
    consumes_winters_chill = triggers.radiant_spark = true;
    triggers.chill = !p->talents.freezing_cold.ok();
    affected_by.time_manipulation = p->talents.freezing_cold.ok();
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.snowstorm->check_stack_value();

    return am;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();
    p()->buffs.snowstorm->expire();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( p()->talents.freezing_cold.ok() )
      p()->trigger_crowd_control( s, MECHANIC_ROOT, 0.5_s ); // Freezing Cold only has the initial grace period
  }
};

// Conflagration Spell ======================================================

struct conflagration_t final : public fire_mage_spell_t
{
  conflagration_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 226757 ) )
  {
    background = true;
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
  }
};

struct conflagration_flare_up_t final : public fire_mage_spell_t
{
  conflagration_flare_up_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 205345 ) )
  {
    background = true;
    callbacks = false;
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
    aoe = -1;
  }
};

// Conjure Mana Gem Spell ===================================================

// Technically, this spell cannot be used when a Mana Gem is in the player's inventory,
// but we assume the player would just delete it before conjuring a new one.
struct conjure_mana_gem_t final : public arcane_mage_spell_t
{
  conjure_mana_gem_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.conjure_mana_gem )
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

struct use_mana_gem_t final : public mage_spell_t
{
  use_mana_gem_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_spell( 5405 ) )
  {
    parse_options( options_str );
    harmful = callbacks = may_crit = may_miss = false;
    affected_by.shifting_power = true;
    target = player;

    if ( p->specialization() != MAGE_ARCANE )
      background = true;
  }

  bool ready() override
  {
    if ( p()->state.mana_gem_charges <= 0 || p()->resources.pct( RESOURCE_MANA ) >= 1.0 )
      return false;

    return mage_spell_t::ready();
  }

  void execute() override
  {
    mage_spell_t::execute();

    p()->resource_gain( RESOURCE_MANA, p()->resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent(), p()->gains.mana_gem, this );
    p()->buffs.invigorating_powder->trigger();
    // TODO: In game, this is bugged and will not properly apply buffs.clearcasting_channel when triggered from 0 stacks.
    if ( p()->talents.cascading_power.ok() )
      p()->buffs.clearcasting->trigger( as<int>( p()->talents.cascading_power->effectN( 1 ).base_value() ) );

    p()->state.mana_gem_charges--;
    assert( p()->state.mana_gem_charges >= 0 );
  }

  // Needed to satisfy normal execute conditions
  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }
};

// Counterspell Spell =======================================================

struct counterspell_t final : public mage_spell_t
{
  counterspell_t( std::string_view n, mage_t* p, std::string_view options_str ) :
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
    if ( success && p()->conduits.grounding_surge.ok() )
      // At this point, Counterspell's cooldown hasn't started yet. Do the CDR in a separate event.
      make_event( *sim, [ this ] { cooldown->adjust( -100 * p()->conduits.grounding_surge.time_value() ); } );
    if ( success && p()->talents.quick_witted.ok() )
      make_event( *sim, [ this ] { cooldown->adjust( -p()->talents.quick_witted->effectN( 1 ).time_value() ); } );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return mage_spell_t::target_ready( candidate_target );
  }
};

// Dragon's Breath Spell ====================================================

struct dragons_breath_t final : public fire_mage_spell_t
{
  dragons_breath_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.dragons_breath )
  {
    parse_options( options_str );
    aoe = -1;
    triggers.from_the_ashes = triggers.radiant_spark = affected_by.time_manipulation = true;
    base_crit += p->talents.alexstraszas_fury->effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + p->talents.alexstraszas_fury->effectN( 2 ).percent();

    if ( p->talents.alexstraszas_fury.ok() )
      triggers.hot_streak = TT_MAIN_TARGET;
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );
    p()->trigger_crowd_control( s, MECHANIC_DISORIENT );
  }
};

// Evocation Spell ==========================================================

struct evocation_t final : public arcane_mage_spell_t
{
  int siphon_storm_charges;

  evocation_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.evocation ),
    siphon_storm_charges()
  {
    parse_options( options_str );
    channeled = ignore_false_positive = tick_zero = true;
    harmful = false;
    target = player;

    if ( p->talents.siphon_storm.ok() )
      siphon_storm_charges = as<int>( p->find_spell( 384265 )->effectN( 1 ).base_value() );
    else if ( p->runeforge.siphon_storm.ok() )
      siphon_storm_charges = as<int>( p->find_spell( 332929 )->effectN( 1 ).base_value() );
  }

  void trigger_dot( action_state_t* s ) override
  {
    // When Evocation is used from the precombat action list, do not start the channel.
    // Just trigger the appropriate buffs and bail out.
    if ( is_precombat )
    {
      int ticks = as<int>( tick_zero ) + static_cast<int>( dot_duration / base_tick_time );
      for ( int i = 0; i < ticks; i++ )
        p()->buffs.siphon_storm->trigger();
    }
    else
    {
      arcane_mage_spell_t::trigger_dot( s );

      double mana_regen_multiplier = 1.0 + p()->buffs.evocation->default_value;
      timespan_t duration = p()->buffs.evocation->buff_duration();

      double speed = p()->cache.spell_speed();
      mana_regen_multiplier /= speed;
      duration *= speed;

      p()->buffs.evocation->trigger( 1, mana_regen_multiplier - 1.0, -1.0, duration );
    }
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( siphon_storm_charges > 0 )
      p()->trigger_arcane_charge( siphon_storm_charges );

    if ( is_precombat && execute_state )
      cooldown->adjust( -composite_dot_duration( execute_state ) );
  }

  void tick( dot_t* d ) override
  {
    arcane_mage_spell_t::tick( d );
    p()->buffs.siphon_storm->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    arcane_mage_spell_t::last_tick( d );
    p()->buffs.evocation->expire();
  }

  bool usable_moving() const override
  {
    if ( p()->talents.slipstream.ok() )
      return true;

    return arcane_mage_spell_t::usable_moving();
  }
};

// Ebonbolt Spell ===========================================================

struct ebonbolt_t final : public frost_mage_spell_t
{
  ebonbolt_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ebonbolt )
  {
    parse_options( options_str );
    parse_effect_data( p->find_spell( 257538 )->effectN( 1 ) );
    calculate_on_impact = track_shatter = consumes_winters_chill = triggers.radiant_spark = true;
    triggers.icy_propulsion_conduit = triggers.icy_propulsion = !p->bugs;

    if ( p->talents.splitting_ice.ok() )
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

struct fireball_t final : public fire_mage_spell_t
{
  fireball_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->find_specialization_spell( "Fireball" ) )
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.kindling = TT_ALL_TARGETS;
    triggers.ignite = triggers.from_the_ashes = triggers.radiant_spark = triggers.overflowing_energy = true;
    affected_by.deathborne_cleave = true;
    base_multiplier *= 1.0 + p->sets->set( MAGE_FIRE, T29, B4 )->effectN( 1 ).percent();
    base_crit += p->sets->set( MAGE_FIRE, T29, B4 )->effectN( 3 ).percent();

    if ( p->talents.conflagration.ok() )
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

  timespan_t execute_time() const override
  {
    timespan_t t = fire_mage_spell_t::execute_time();

    if ( p()->buffs.flame_accelerant->check() )
      t *= 1.0 + p()->talents.flame_accelerant->effectN( 2 ).percent();

    return t;
  }

  double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( p()->buffs.flame_accelerant->check() )
      am *= 1.0 + p()->talents.flame_accelerant->effectN( 1 ).percent();

    return am;
  }

  void execute() override
  {
    fire_mage_spell_t::execute();
    p()->buffs.expanded_potential->trigger();
    p()->buffs.flame_accelerant_icd->trigger();
  }

  void impact( action_state_t* s ) override
  {
    // Flame Accretion conduit (which is tied to the Fireball buff), has some weird
    // interactions with Ignite from Fireball. When Fireball hits, Ignite is triggered
    // with the current mastery value and then the buff is incremented. However,
    // when Fireball crits, the buff expires first and Ignite is triggered with the
    // new mastery value.
    //
    // Here we model that by triggering the buff after impact, but expiring it before
    // impact (the crit bonus was already factored in during execute).
    if ( s->result == RESULT_CRIT )
      p()->buffs.pyrotechnics->expire();

    fire_mage_spell_t::impact( s );

    if ( s->result == RESULT_HIT )
      p()->buffs.pyrotechnics->trigger();

    if ( result_is_hit( s->result ) )
    {
      trigger_deaths_fathom();
      if ( !consume_molten_skyfall( s->target ) )
        trigger_molten_skyfall();
      if ( !consume_firefall( s->target ) )
        trigger_firefall();
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

    c += p()->buffs.pyrotechnics->check_stack_value();

    return c;
  }
};

// Flame Patch Spell ========================================================

struct flame_patch_t final : public fire_mage_spell_t
{
  flame_patch_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 205472 ) )
  {
    aoe = -1;
    ground_aoe = background = true;
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) && p()->cooldowns.incendiary_eruptions->up() && rng().roll( p()->talents.incendiary_eruptions->proc_chance() ) )
    {
      p()->cooldowns.incendiary_eruptions->start( p()->talents.incendiary_eruptions->internal_cooldown() );
      // TODO: Rather than reusing actions.living_bomb_dot, it would be better to split the actions so that stats are nicely split in reports.
      p()->action.living_bomb_dot->execute_on_target( s->target );
    }
  }
};

// Flamestrike Spell ========================================================

struct flamestrike_t final : public hot_streak_spell_t
{
  action_t* flame_patch;
  timespan_t flame_patch_duration;

  flamestrike_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    hot_streak_spell_t( n, p, p->talents.flamestrike ),
    flame_patch()
  {
    parse_options( options_str );
    triggers.ignite = triggers.radiant_spark = true;
    aoe = -1;
    base_execute_time += p->talents.improved_flamestrike->effectN( 1 ).time_value();
    base_multiplier *= 1.0 + p->conduits.master_flame.percent();

    if ( p->talents.flame_patch.ok() )
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
      // Flame Patch does not gain its extra ticks at exact haste breakpoints. Instead, extra
      // ticks occur with an increasing probability as haste approaches the expected breakpoint
      // with a 100% probability after the breakpoint is reached. This is likely due to technical
      // details of how Flame Patch or ground effects in general are implemented. For Flame Patch,
      // adding uniform delay to the duration between 5 ms and 105 ms gives an average number of
      // ticks that closely matches the observed values at haste levels near the breakpoints.
      timespan_t ground_aoe_duration = flame_patch_duration + rng().range( 5_ms, 105_ms );
      p()->ground_aoe_expiration[ AOE_FLAME_PATCH ] = sim->current_time() + ground_aoe_duration;

      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( target )
        .duration( ground_aoe_duration )
        .action( flame_patch )
        .hasted( ground_aoe_params_t::SPELL_SPEED ) );
    }
  }
};

// Flurry Spell =============================================================

struct glacial_assault_t final : public frost_mage_spell_t
{
  glacial_assault_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 379029 ) )
  {
    background = true;
    aoe = -1;
  }
};

struct flurry_bolt_t final : public frost_mage_spell_t
{
  flurry_bolt_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 228354 ) )
  {
    background = triggers.chill = triggers.radiant_spark = triggers.icy_propulsion = triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    auto wc = get_td( s->target )->debuffs.winters_chill;
    wc->trigger( wc->max_stack() );
    for ( int i = 0; i < wc->max_stack(); i++ )
      p()->procs.winters_chill_applied->occur();

    if ( rng().roll( p()->talents.glacial_assault->effectN( 1 ).percent() ) )
    {
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( 1.0_s )
        .target( s->target )
        .n_pulses( 1 )
        .action( p()->action.glacial_assault ) );
    }
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    if ( p()->state.brain_freeze_active )
      am *= 1.0 + p()->buffs.brain_freeze->data().effectN( 2 ).percent();

    return am;
  }
};

struct flurry_t final : public frost_mage_spell_t
{
  action_t* flurry_bolt;

  flurry_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.flurry ),
    flurry_bolt( get_action<flurry_bolt_t>( "flurry_bolt", p ) )
  {
    parse_options( options_str );
    may_miss = may_crit = affected_by.shatter = false;
    cooldown->charges += as<int>( p->talents.perpetual_winter->effectN( 1 ).base_value() );

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

  void execute() override
  {
    frost_mage_spell_t::execute();

    int icicle_count = p()->rng().roll( p()->talents.splintering_cold->effectN( 2 ).percent() ) ? 2 : 1;
    for (int i = 0; i < icicle_count; i++ ) p()->trigger_icicle_gain( target, p()->action.icicle.flurry );
    p()->expression_support.remaining_winters_chill = 2;

    p()->state.brain_freeze_active = p()->buffs.brain_freeze->up();
    p()->buffs.brain_freeze->decrement();

    p()->procs.flurry_cast->occur();
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

    consume_cold_front( s->target );
    trigger_cold_front();
  }
};

// Frostbolt Spell ==========================================================

struct frostbolt_t final : public frost_mage_spell_t
{
  double fof_chance;
  double bf_chance;

  bool fractured_frost_active = true;

  frostbolt_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_class_spell( "Frostbolt" ) ),
    fof_chance(),
    bf_chance()
  {
    parse_options( options_str );
    parse_effect_data( p->find_spell( 228597 )->effectN( 1 ) );
    calculate_on_impact = track_shatter = consumes_winters_chill = affected_by.deathborne_cleave = true;
    triggers.chill = triggers.radiant_spark = triggers.icy_propulsion = triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->talents.wintertide->effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + p->talents.piercing_cold->effectN( 1 ).percent();

    double ft_multiplier = 1.0 + p->talents.frozen_touch->effectN( 1 ).percent();
    // Because of the additional procs gained from the bad luck protection
    // system, the base proc chances are reduced so that the overall average
    // is not significantly changed by the system.
    // TODO: How does this reduction work for low level Mages without BLP?
    // TODO: Need to verify that this BLP system still exists in Dragonflight.
    if ( p->talents.fingers_of_frost.ok() )
      fof_chance = ft_multiplier * p->talents.fingers_of_frost->effectN( 1 ).percent() - 0.005;
    if ( p->talents.brain_freeze.ok() )
      bf_chance = ft_multiplier * p->talents.brain_freeze->effectN( 1 ).percent() - 0.01;

    if ( p->spec.icicles->ok() )
      add_child( p->action.icicle.frostbolt );
  }

  void init_finished() override
  {
    proc_brain_freeze = p()->get_proc( "Brain Freeze from Frostbolt" );
    proc_fof = p()->get_proc( "Fingers of Frost from Frostbolt" );

    frost_mage_spell_t::init_finished();
  }

  int n_targets() const override
  {
    if ( p()->talents.fractured_frost.ok() && fractured_frost_active )
      return as<int>( p()->talents.fractured_frost->effectN( 3 ).base_value() );
    else
      return frost_mage_spell_t::n_targets();
  }

  timespan_t gcd() const override
  {
    timespan_t t = frost_mage_spell_t::gcd();

    t *= 1.0 + p()->buffs.slick_ice->check_stack_value();

    return std::max( t, min_gcd );
  }

  timespan_t execute_time() const override
  {
    timespan_t t = frost_mage_spell_t::execute_time();

    t *= 1.0 + p()->buffs.slick_ice->check_stack_value();

    return t;
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.slick_ice->check() * p()->buffs.slick_ice->data().effectN( 3 ).percent();

    return am;
  }

  double frozen_multiplier( const action_state_t* s ) const override
  {
    double fm = frost_mage_spell_t::frozen_multiplier( s );

    fm *= 1.0 + p()->talents.deep_shatter->effectN( 1 ).percent();

    return fm;
  }

  void execute() override
  {
    // We treat Fractured Frost as always active outside of the spell execute, this makes sure
    // that simc properly invalidates target caches etc.
    fractured_frost_active = p()->rng().roll( p()->talents.fractured_frost->effectN( 2 ).percent() );
    frost_mage_spell_t::execute();
    fractured_frost_active = true;

    int icicle_count = p()->rng().roll( p()->talents.splintering_cold->effectN( 2 ).percent() ) ? 2 : 1;
    for ( int i = 0; i < icicle_count; i++ ) p()->trigger_icicle_gain( target, p()->action.icicle.frostbolt );

    handle_procs();

    p()->buffs.expanded_potential->trigger();

    if ( p()->buffs.icy_veins->check() )
      p()->buffs.slick_ice->trigger();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      trigger_deaths_fathom();
      consume_cold_front( s->target );
      // Cold Front and Deathborne cleave have some unusual interactions.
      // After casting a Frostbolt that hits 3 targets, the following occurs:
      // * 26-28 cold_front: FO is triggered, 0 stacks after
      // * cold_front_ready: FO is triggered, 3 stacks after
      // Other outcomes are also possible, although rare. This is most
      // likely due to batching.
      // TODO: double check if this is still the case
      if ( s->chain_target == 0 )
      {
        for ( unsigned i = 0; i < s->n_targets; i++ )
          trigger_cold_front();
      }
    }
  }

  void handle_procs()
  {
    if ( p()->specialization() != MAGE_FROST )
      return;

    bool fof_triggered = p()->trigger_fof( fof_chance, proc_fof );
    bool bf_triggered = p()->trigger_brain_freeze( bf_chance, proc_brain_freeze );

    if ( fof_chance == 0.0 || bf_chance == 0.0 )
      return;

    if ( !bf_triggered && !fof_triggered )
    {
      p()->state.frostbolt_counter++;
      // On the 6th consecutive Frostbolt where no Fingers of Frost or Brain Freeze
      // proc has occurred, there is an extra 25% chance to gain a Brain Freeze proc.
      if ( p()->state.frostbolt_counter == 6 )
        bf_triggered = p()->trigger_brain_freeze( 0.25, proc_brain_freeze );
      // On the 7th consecutive Frostbolt where no Fingers of Frost or Brain Freeze
      // proc has occurred, a proc will occur. The chances that this proc will be a
      // Fingers of Frost or a Brain Freeze are equal.
      else if ( p()->state.frostbolt_counter >= 7 )
      {
        // Try to trigger the Fingers of Frost proc with a 50% chance, because if it
        // is triggered with a 100% chance then the actor will not have to react to it.
        // Brain Freeze does not have this feature, so triggering it below with a 100%
        // chance is not a problem.
        fof_triggered = p()->trigger_fof( 0.5, proc_fof );
        if ( !fof_triggered )
          bf_triggered = p()->trigger_brain_freeze( 1.0, proc_brain_freeze );
      }
    }

    if ( fof_triggered || bf_triggered )
      p()->state.frostbolt_counter = 0;
  }
};

// Frost Nova Spell =========================================================

struct frost_nova_t final : public mage_spell_t
{
  frost_nova_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Frost Nova" ) )
  {
    parse_options( options_str );
    aoe = -1;
    triggers.radiant_spark = affected_by.time_manipulation = true;
    cooldown->charges += as<int>( p->talents.ice_ward->effectN( 1 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    p()->trigger_crowd_control( s, MECHANIC_ROOT );
    if ( result_is_hit( s->result ) && p()->runeforge.grisly_icicle.ok() )
      get_td( s->target )->debuffs.grisly_icicle->trigger();
  }
};

// Frozen Orb Spell =========================================================

struct frozen_orb_bolt_t final : public frost_mage_spell_t
{
  frozen_orb_bolt_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 84721 ) )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
    base_multiplier *= 1.0 + p->talents.everlasting_frost->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->conduits.unrelenting_cold.percent();
    base_multiplier *= 1.0 + p->sets->set( MAGE_FROST, T29, B2 )->effectN( 1 ).percent();
    background = triggers.chill = true;
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
      p()->trigger_fof( p()->talents.fingers_of_frost->effectN( 2 ).percent(), proc_fof );
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->cache.mastery() * p()->spec.icicles_2->effectN( 1 ).mastery_value();

    return am;
  }
};

struct frozen_orb_t final : public frost_mage_spell_t
{
  action_t* frozen_orb_bolt;

  frozen_orb_t( std::string_view n, mage_t* p, std::string_view options_str, bool cold_front = false ) :
    frost_mage_spell_t( n, p, cold_front ? p->find_spell( 84714 ) : p->talents.frozen_orb ),
    frozen_orb_bolt( get_action<frozen_orb_bolt_t>( cold_front ? "cold_front_frozen_orb_bolt" : "frozen_orb_bolt", p ) )
  {
    parse_options( options_str );
    may_miss = may_crit = affected_by.shatter = false;
    add_child( frozen_orb_bolt );

    if ( cold_front )
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

  void adjust_orb_count( timespan_t duration, bool active )
  {
    int& counter = active ? p()->state.active_frozen_orbs : p()->state.inactive_frozen_orbs;
    counter++;
    make_event( *sim, duration, [ this, &counter, active ]
    {
      counter--;
      if ( p()->state.active_frozen_orbs + p()->state.inactive_frozen_orbs == 0 || ( active && p()->bugs ) )
        p()->buffs.freezing_winds->expire();
    } );
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    adjust_orb_count( travel_time(), false );

    p()->buffs.freezing_winds->trigger();
    if ( !background ) p()->buffs.freezing_rain->trigger();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    p()->trigger_fof( 1.0, proc_fof );

    int pulse_count = 20;
    timespan_t pulse_time = 0.5_s;
    pulse_count += as<int>( p()->talents.everlasting_frost->effectN( 2 ).time_value() / pulse_time );
    timespan_t duration = ( pulse_count - 1 ) * pulse_time;
    p()->ground_aoe_expiration[ AOE_FROZEN_ORB ] = sim->current_time() + duration;

    adjust_orb_count( duration, true );

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( pulse_time )
      .target( s->target )
      .n_pulses( pulse_count )
      .action( frozen_orb_bolt ), true );
  }
};

// Glacial Spike Spell ======================================================

struct glacial_spike_t final : public frost_mage_spell_t
{
  glacial_spike_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.glacial_spike )
  {
    parse_options( options_str );
    parse_effect_data( p->find_spell( 228600 )->effectN( 1 ) );
    calculate_on_impact = track_shatter = consumes_winters_chill = true;
    triggers.radiant_spark = triggers.icy_propulsion = triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->talents.flash_freeze->effectN( 2 ).percent();
    crit_bonus_multiplier *= 1.0 + p->talents.piercing_cold->effectN( 1 ).percent();

    if ( p->talents.splitting_ice.ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_aoe_multiplier *= p->talents.splitting_ice->effectN( 2 ).percent();
    }
  }

  bool ready() override
  {
    // Glacial Spike doesn't check the Icicles buff after it started executing.
    if ( p()->executing != this && !p()->buffs.icicles->at_max_stacks() )
      return false;

    return frost_mage_spell_t::ready();
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

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

    // Consume all Icicles by expiring the buff and cleaning up the
    // Icicles vector. Note that this also includes canceling the
    // expiration events.
    p()->buffs.icicles->expire();
    while ( !p()->icicles.empty() )
    {
      p()->get_icicle();
      p()->trigger_fof( p()->talents.flash_freeze->effectN( 1 ).percent(), p()->procs.fingers_of_frost_flash_freeze );
      // TODO: check if splitting ice doubles the procs
      // TODO: what happens if buff stacks and icicle count don't match up
    }
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( s->result == RESULT_CRIT )
    {
      timespan_t cdr = -p()->talents.icy_propulsion->effectN( 1 ).time_value();
      p()->cooldowns.icy_veins->adjust( p()->spec.icicles->effectN( 2 ).base_value() * cdr );
    }

    p()->trigger_crowd_control( s, MECHANIC_ROOT );
  }
};

// Ice Floes Spell ==========================================================

struct ice_floes_t final : public mage_spell_t
{
  ice_floes_t( std::string_view n, mage_t* p, std::string_view options_str ) :
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

struct glacial_fragments_t final : public frost_mage_spell_t
{
  glacial_fragments_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 327498 ) )
  {
    aoe = -1;
    reduced_aoe_targets = p->runeforge.glacial_fragments->effectN( 3 ).base_value();
    background = true;
    affected_by.shatter = triggers.icy_propulsion_conduit = false; // TODO: does this work with Shatter in DF?
  }
};

struct ice_lance_state_t final : public mage_spell_state_t
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

struct ice_lance_t final : public frost_mage_spell_t
{
  shatter_source_t* extension_source;
  shatter_source_t* cleave_source;

  action_t* glacial_fragments;

  ice_lance_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ice_lance ),
    extension_source(),
    cleave_source(),
    glacial_fragments()
  {
    parse_options( options_str );
    parse_effect_data( p->find_spell( 228598 )->effectN( 1 ) );
    calculate_on_impact = track_shatter = consumes_winters_chill = true;
    triggers.radiant_spark = triggers.icy_propulsion = triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->sets->set( MAGE_FROST, T29, B2 )->effectN( 1 ).percent();

    // TODO: Cleave distance for SI seems to be 8 + hitbox size.
    if ( p->talents.splitting_ice.ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_multiplier *= 1.0 + p->talents.splitting_ice->effectN( 3 ).percent();
      // Hardcoded in the talent description.
      base_aoe_multiplier *= p->talents.splitting_ice->effectN( 2 ).percent() - 0.15;
    }

    if ( p->runeforge.glacial_fragments.ok() )
    {
      glacial_fragments = get_action<glacial_fragments_t>( "glacial_fragments", p );
      add_child( glacial_fragments );
    }
    if ( p->talents.hailstones.ok() )
      add_child( p->action.icicle.ice_lance );
  }

  void init_finished() override
  {
    frost_mage_spell_t::init_finished();

    if ( sim->report_details != 0 && p()->talents.splitting_ice.ok() )
      cleave_source = p()->get_shatter_source( "Ice Lance cleave" );
    if ( sim->report_details != 0 && p()->talents.thermal_void.ok() )
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

  void schedule_travel( action_state_t* s ) override
  {
    frost_mage_spell_t::schedule_travel( s );

    // We need access to the action state corresponding to the main target so that we
    // can use mage_spell_t::frozen to figure out the frozen state at the moment of cast.
    // TODO: this is almost surely a bug since you can shatter Ice Lance without getting Icicle/TM and vice versa
    if ( s->chain_target == 0 && frozen( s ) )
    {
      p()->trigger_time_manipulation();
      if ( p()->rng().roll( p()->talents.hailstones->effectN( 1 ).percent() ) )
        p()->trigger_icicle_gain( s->target, p()->action.icicle.ice_lance );
    }
  }

  void execute() override
  {
    p()->state.fingers_of_frost_active = p()->buffs.fingers_of_frost->up();

    frost_mage_spell_t::execute();

    if ( p()->state.fingers_of_frost_active )
      p()->buffs.touch_of_ice->trigger();

    p()->buffs.fingers_of_frost->decrement();

    // Begin casting all Icicles at the target, beginning 0.25 seconds after the
    // Ice Lance cast with remaining Icicles launching at intervals of 0.4
    // seconds, the latter adjusted by haste. Casting continues until all
    // Icicles are gone, including new ones that accumulate while they're being
    // fired. If target dies, Icicles stop.
    if ( !p()->talents.glacial_spike.ok() )
      p()->trigger_icicle( target, true );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    debug_cast<ice_lance_state_t*>( s )->fingers_of_frost = p()->buffs.fingers_of_frost->check();
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
      if ( p()->talents.thermal_void.ok() && p()->buffs.icy_veins->check() )
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

    if ( glacial_fragments )
    {
      double chance = p()->ground_aoe_expiration[ AOE_BLIZZARD ] > sim->current_time()
        ? p()->runeforge.glacial_fragments->effectN( 2 ).percent()
        : p()->runeforge.glacial_fragments->effectN( 1 ).percent();

      if ( rng().roll( chance ) )
        glacial_fragments->execute_on_target( s->target );
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
    double fm = frost_mage_spell_t::frozen_multiplier( s );

    fm *= 3.0;
    fm *= 1.0 + p()->conduits.ice_bite.percent();

    unsigned frozen = cast_state( s )->frozen;
    if ( frozen &  FF_FINGERS_OF_FROST
      && frozen & ~FF_FINGERS_OF_FROST)
    {
      fm *= 1.0 + p()->talents.wintertide->effectN( 2 ).percent();
    }

    return fm;
  }
};

// Ice Nova Spell ===========================================================

struct ice_nova_t final : public frost_mage_spell_t
{
  ice_nova_t( std::string_view n, mage_t* p, std::string_view options_str ) :
     frost_mage_spell_t( n, p, p->talents.ice_nova )
  {
    parse_options( options_str );
    aoe = -1;
    // TODO: currently deals full damage to all targets, probably a bug
    if ( !p->bugs )
    {
      reduced_aoe_targets = 1.0;
      full_amount_targets = 1;
    }
    consumes_winters_chill = triggers.radiant_spark = affected_by.time_manipulation = true;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    p()->trigger_crowd_control( s, MECHANIC_ROOT );
  }
};

// Icy Veins Spell ==========================================================

struct icy_veins_t final : public frost_mage_spell_t
{
  icy_veins_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.icy_veins )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    // TODO: check if this is the case with the talent as well
    p()->buffs.slick_ice->expire();
    p()->buffs.icy_veins->trigger();
    p()->buffs.rune_of_power->trigger();

    if ( p()->talents.snap_freeze.ok() )
    {
      // TODO: check what the delay is if BF is already up
      p()->trigger_brain_freeze( 1.0, p()->procs.brain_freeze_snap_freeze );
      p()->trigger_fof( 1.0, p()->procs.fingers_of_frost_snap_freeze );
    }
  }
};

// Fire Blast Spell =========================================================

struct fire_blast_t final : public fire_mage_spell_t
{
  int max_spread_targets;

  fire_blast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.fire_blast.ok() ? p->talents.fire_blast : p->find_class_spell( "Fire Blast" ) )
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.kindling = TT_MAIN_TARGET;
    triggers.ignite = triggers.from_the_ashes = triggers.radiant_spark = triggers.icy_propulsion = triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->sets->set( MAGE_FIRE, T29, B4 )->effectN( 1 ).percent();
    base_crit += p->sets->set( MAGE_FIRE, T29, B4 )->effectN( 3 ).percent();

    cooldown->charges += as<int>( p->talents.flame_on->effectN( 1 ).base_value() );
    cooldown->duration -= 1000 * p->talents.flame_on->effectN( 3 ).time_value();
    cooldown->hasted = true;

    if ( p->talents.fire_blast.ok() )
    {
      base_crit += 1.0;
      usable_while_casting = true;
    }

    // Ignite is bugged and spreads to one fewer target than it should.
    max_spread_targets = as<int>( p->spec.ignite->effectN( 4 ).base_value() + p->talents.master_of_flame->effectN( 2 ).base_value() ) - ( p->bugs ? 1 : 0 );
  }

  // TODO: When an Ignite has a partial tick, how is the bank amount calculated to determine valid spread targets?
  static double ignite_bank( dot_t* ignite )
  {
    if ( !ignite->is_ticking() )
      return 0.0;

    auto ignite_state = debug_cast<residual_action::residual_periodic_state_t*>( ignite->state );
    return ignite_state->tick_amount * ignite->ticks_left_fractional();
  }

  void spread_ignite( player_t* primary )
  {
    auto source = primary->get_dot( "ignite", player );
    if ( source->is_ticking() )
    {
      std::vector<dot_t*> ignites;

      // Collect the Ignite DoT objects of all targets that are in range.
      for ( auto t : target_list() )
        ignites.push_back( t->get_dot( "ignite", player ) );

      // Sort candidate Ignites by descending bank size.
      std::stable_sort( ignites.begin(), ignites.end(), [] ( dot_t* a, dot_t* b )
      { return ignite_bank( a ) > ignite_bank( b ); } );

      auto source_bank = ignite_bank( source );
      auto targets_remaining = max_spread_targets;
      auto source_tick_amount = debug_cast<residual_action::residual_periodic_state_t*>( source->state )->tick_amount;
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
        {
          p()->procs.ignite_overwrite->occur();

          // If Ignite is already active on the target, the copied Ignite is applied as if it were refreshing the active one.
          source->copy( destination->target, DOT_COPY_CLONE );
        }
        else
        {
          p()->procs.ignite_new_spread->occur();

          // If Ignite is not active, we need to apply a new Ignite, but the full state is not copied (i.e., time to tick).
          source->copy( destination->target, DOT_COPY_START );
        }

        // Regardless of existing Ignites, the tick amount is directly copied when spreading an Ignite.
        // This can sometimes result in the newly spread Ignites having a larger than expected bank.
        debug_cast<residual_action::residual_periodic_state_t*>( destination->state )->tick_amount = source_tick_amount;
      }
    }
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    p()->trigger_sinful_delight( MAGE_FIRE );
    if ( p()->specialization() == MAGE_FIRE )
      p()->trigger_time_manipulation();
  }

  void impact( action_state_t* s ) override
  {
    spread_ignite( s->target );

    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.feel_the_burn->trigger();
      if ( p()->buffs.combustion->check() )
        p()->buffs.infernal_cascade->trigger();
    }
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = fire_mage_spell_t::recharge_rate_multiplier( cd );

    if ( &cd == cooldown )
      m /= 1.0 + p()->buffs.fiery_rush->check_value();

    return m;
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_dot_t final : public fire_mage_spell_t
{
  // The game has two spells for the DoT, one for pre-spread one and one for
  // post-spread one. This allows two copies of the DoT to be up on one target.
  const bool primary;

  static unsigned dot_spell_id( bool primary )
  { return primary ? 217694 : 244813; }

  living_bomb_dot_t( std::string_view n, mage_t* p, bool primary_ ) :
    fire_mage_spell_t( n, p, p->find_spell( dot_spell_id( primary_ ) ) ),
    primary( primary_ )
  {
    background = true;
    if ( !p->talents.radiant_spark.ok() )
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

        p()->action.living_bomb_dot_spread->execute_on_target( t );
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

struct living_bomb_explosion_t final : public fire_mage_spell_t
{
  living_bomb_explosion_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 44461 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    background = true;
    callbacks = false;
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
  }
};

struct living_bomb_t final : public fire_mage_spell_t
{
  living_bomb_t( std::string_view n, mage_t* p, std::string_view options_str ) :
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

// Old implementation details from Celestalon:
// http://blue.mmo-champion.com/topic/318876-warlords-of-draenor-theorycraft-discussion/#post301
// Meteor is split over a number of spell IDs
// - Meteor (id=153561) is the talent spell, the driver
// - Meteor (id=153564) is the initial impact damage
// - Meteor Burn (id=155158) is the ground effect tick damage
// - Meteor Burn (id=175396) provides the tooltip's burn duration
// - Meteor (id=177345) contains the time between cast and impact
// 2021-02-23 PTR: Meteor now snapshots damage on impact.
// - time until impact is unchanged.
// - Meteor (id=351140) is the new initial impact damage.
struct meteor_burn_t final : public fire_mage_spell_t
{
  meteor_burn_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 155158 ) )
  {
    background = ground_aoe = true;
    aoe = -1;
    radius = p->find_spell( 153564 )->effectN( 1 ).radius_max();

    // Meteor Burn is actually some sort of area DoT. We simulate it
    // by using ground_aoe_event_t and a DoT that does a single
    // tick on each pulse.
    dot_duration = base_tick_time = 1_ms;
    hasted_ticks = false;
  }
};

struct meteor_impact_t final : public fire_mage_spell_t
{
  action_t* meteor_burn;

  timespan_t meteor_burn_duration;
  timespan_t meteor_burn_pulse_time;

  meteor_impact_t( std::string_view n, mage_t* p, action_t* burn ) :
    fire_mage_spell_t( n, p, p->find_spell( 351140 ) ),
    meteor_burn( burn ),
    meteor_burn_duration( p->find_spell( 175396 )->duration() ),
    meteor_burn_pulse_time( p->find_spell( 155158 )->effectN( 1 ).period() )
  {
    background = split_aoe_damage = true;
    aoe = -1;
    triggers.ignite = triggers.radiant_spark = true;

    if ( p->specialization() != MAGE_FIRE )
      base_multiplier *= 1.2; // There's currently no spell data for this
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    p()->ground_aoe_expiration[ AOE_METEOR_BURN ] = sim->current_time() + meteor_burn_duration;
    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( meteor_burn_pulse_time )
      .target( target )
      .duration( meteor_burn_duration )
      .action( meteor_burn ) );
  }
};

struct meteor_t final : public fire_mage_spell_t
{
  timespan_t meteor_delay;

  meteor_t( std::string_view n, mage_t* p, std::string_view options_str, bool firefall = false ) :
    fire_mage_spell_t( n, p, firefall ? p->find_spell( 153561 ) : p->talents.meteor ),
    meteor_delay( p->find_spell( 177345 )->duration() )
  {
    parse_options( options_str );

    if ( !data().ok() )
      return;

    action_t* meteor_burn = get_action<meteor_burn_t>( firefall ? "firefall_meteor_burn" : "meteor_burn", p );
    impact_action = get_action<meteor_impact_t>( firefall ? "firefall_meteor_impact" : "meteor_impact", p, meteor_burn );

    add_child( meteor_burn );
    add_child( impact_action );

    if ( firefall )
    {
      background = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  }

  timespan_t travel_time() const override
  {
    // Travel time cannot go lower than 1 second to give time for Meteor to visually fall.
    return std::max( meteor_delay * p()->cache.spell_speed(), 1.0_s );
  }
};

// Mirror Image Spell =======================================================

struct mirror_image_t final : public mage_spell_t
{
  mirror_image_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.mirror_image )
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

struct nether_tempest_aoe_t final : public arcane_mage_spell_t
{
  nether_tempest_aoe_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 114954 ) )
  {
    aoe = -1;
    background = true;
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }
};

struct nether_tempest_t final : public arcane_mage_spell_t
{
  action_t* nether_tempest_aoe;

  nether_tempest_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.nether_tempest ),
    nether_tempest_aoe( get_action<nether_tempest_aoe_t>( "nether_tempest_aoe", p ) )
  {
    parse_options( options_str );
    add_child( nether_tempest_aoe );
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
  }

  void execute() override
  {
    p()->benefits.arcane_charge.nether_tempest->update();

    arcane_mage_spell_t::execute();

    if ( hit_any_target )
    {
      if ( p()->state.last_bomb_target && p()->state.last_bomb_target != target )
        get_td( p()->state.last_bomb_target )->dots.nether_tempest->cancel();
      p()->state.last_bomb_target = target;
    }
  }

  void tick( dot_t* d ) override
  {
    arcane_mage_spell_t::tick( d );

    // Since the Nether Tempest AoE action inherits persistent multiplier and tick
    // factor of the DoT, we need to manually create an action state, set the
    // relevant values and pass it to the AoE action's schedule_execute.
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

// TODO: Phoenix Flames is currently bugged and spreads Ignite, check later in beta to make sure this was fixed.
struct phoenix_flames_splash_t final : public fire_mage_spell_t
{
  phoenix_flames_splash_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 257542 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    background = true;
    callbacks = false;
    triggers.hot_streak = triggers.kindling = TT_MAIN_TARGET;
    triggers.ignite = triggers.radiant_spark = true;
    base_multiplier *= 1.0 + p->sets->set( MAGE_FIRE, T29, B4 )->effectN( 1 ).percent();
    base_crit += p->talents.alexstraszas_fury->effectN( 3 ).percent();
    base_crit += p->sets->set( MAGE_FIRE, T29, B4 )->effectN( 3 ).percent();
  }
};

struct phoenix_flames_t final : public fire_mage_spell_t
{
  phoenix_flames_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.phoenix_flames )
  {
    parse_options( options_str );

    if ( !data().ok() )
      return;

    cooldown->charges += as<int>( p->talents.call_of_the_sun_king->effectN( 1 ).base_value() );

    impact_action = get_action<phoenix_flames_splash_t>( "phoenix_flames_splash", p );
    add_child( impact_action );
  }

  timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( t, 0.75_s );
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = fire_mage_spell_t::recharge_rate_multiplier( cd );

    if ( &cd == cooldown )
      m /= 1.0 + p()->buffs.fiery_rush->check_value();

    return m;
  }
};

// Pyroblast Spell ==========================================================

struct pyroblast_t final : public hot_streak_spell_t
{
  pyroblast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    hot_streak_spell_t( n, p, p->talents.pyroblast )
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.kindling = TT_MAIN_TARGET;
    triggers.ignite = triggers.from_the_ashes = triggers.radiant_spark = triggers.overflowing_energy = true;
    base_execute_time *= 1.0 + p->talents.tempered_flames->effectN( 1 ).percent();
    base_crit += p->talents.tempered_flames->effectN( 2 ).percent();

    if ( !p->talents.controlled_destruction.ok() )
      base_multiplier *= 1.0 + p->conduits.controlled_destruction.percent();
  }

  double action_multiplier() const override
  {
    double am = hot_streak_spell_t::action_multiplier();

    if ( time_to_execute > 0_ms )
      am *= 1.0 + p()->buffs.pyroclasm->check_value();

    return am;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hot_streak_spell_t::composite_da_multiplier( s );

    if ( s->target->health_percentage() < p()->talents.controlled_destruction->effectN( 2 ).base_value()
      || s->target->health_percentage() > p()->talents.controlled_destruction->effectN( 3 ).base_value() )
    {
      m *= 1.0 + p()->talents.controlled_destruction->effectN( 1 ).percent();
    }

    return m;
  }

  void execute() override
  {
    hot_streak_spell_t::execute();

    if ( time_to_execute > 0_ms )
      p()->buffs.pyroclasm->decrement();
  }

  timespan_t travel_time() const override
  {
    timespan_t t = hot_streak_spell_t::travel_time();
    return std::min( t, 0.75_s );
  }

  void impact( action_state_t* s ) override
  {
    hot_streak_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( !consume_molten_skyfall( s->target ) )
        trigger_molten_skyfall();
      if ( !consume_firefall( s->target ) )
        trigger_firefall();
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

struct ray_of_frost_t final : public frost_mage_spell_t
{
  ray_of_frost_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ray_of_frost )
  {
    parse_options( options_str );
    channeled = triggers.chill = triggers.radiant_spark = true;
    triggers.icy_propulsion_conduit = false;
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
    trigger_chill_effect( d->state );

    // TODO: Now happens at 2.5 and 5 through a hidden buff (spell_id 269748).
    if ( d->current_tick == 3 || d->current_tick == 5 )
      p()->trigger_fof( 1.0, proc_fof );
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

struct rune_of_power_t final : public mage_spell_t
{
  rune_of_power_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.rune_of_power )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.rune_of_power->trigger();
  }
};

// Scorch Spell =============================================================

struct scorch_t final : public fire_mage_spell_t
{
  scorch_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.scorch )
  {
    parse_options( options_str );
    triggers.hot_streak = TT_MAIN_TARGET;
    triggers.ignite = triggers.from_the_ashes = triggers.radiant_spark = triggers.overflowing_energy = true;
    // There is a tiny delay between Scorch dealing damage and Hot Streak
    // state being updated. Here we model it as a tiny travel time.
    travel_delay = p->options.scorch_delay.total_seconds();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( searing_touch_active( s->target ) )
      m *= 1.0 + p()->talents.searing_touch->effectN( 2 ).percent();

    return m;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = fire_mage_spell_t::composite_target_crit_chance( target );

    if ( searing_touch_active( target ) )
      c += 1.0;

    return c;
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) && improved_scorch_active( s->target ) )
      get_td( s->target )->debuffs.improved_scorch->trigger();
  }

  bool usable_moving() const override
  { return true; }
};

// Shimmer Spell ============================================================

struct shimmer_t final : public mage_spell_t
{
  shimmer_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.shimmer )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = usable_while_casting = true;
    base_teleport_distance = data().effectN( 1 ).radius_max();
    movement_directionality = movement_direction_type::OMNI;

    if ( p->talents.flow_of_time.ok() )
      cooldown->duration += p->talents.flow_of_time->effectN( 2 ).time_value();
    else
      cooldown->duration += p->conduits.flow_of_time.time_value();
  }
};

// Slow Spell ===============================================================

struct slow_t final : public arcane_mage_spell_t
{
  slow_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.slow )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }
};

// Supernova Spell ==========================================================

struct supernova_t final : public arcane_mage_spell_t
{
  supernova_t( std::string_view n, mage_t* p, std::string_view options_str ) :
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

struct summon_water_elemental_t final : public frost_mage_spell_t
{
  summon_water_elemental_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.summon_water_elemental )
  {
    parse_options( options_str );
    harmful = track_cd_waste = false;
    ignore_false_positive = true;
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

struct time_warp_t final : public mage_spell_t
{
  bool temporal_warp = false;

  time_warp_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Time Warp" ) )
  {
    parse_options( options_str );
    harmful = false;
    temporal_warp = p->talents.temporal_warp.ok() || p->runeforge.temporal_warp.ok();

    if ( sim->overrides.bloodlust && !temporal_warp )
      background = true;
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( player->buffs.exhaustion->check() )
      p()->buffs.temporal_warp->trigger();
    else if ( p()->talents.temporal_warp.ok() )
      cooldown->reset( false );

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
    if ( player->buffs.exhaustion->check() && !temporal_warp )
      return false;

    return mage_spell_t::ready();
  }
};

// Touch of the Magi Spell ==================================================

struct touch_of_the_magi_t final : public arcane_mage_spell_t
{
  touch_of_the_magi_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.touch_of_the_magi )
  {
    parse_options( options_str );

    if ( data().ok() )
      add_child( p->action.touch_of_the_magi_explosion );
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();
    p()->trigger_arcane_charge( as<int>( data().effectN( 2 ).base_value() ) );
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      auto debuff = get_td( s->target )->debuffs.touch_of_the_magi;
      debuff->expire();
      debuff->trigger();
    }
  }

  // Touch of the Magi will trigger procs that occur only from casting damaging spells.
  bool has_amount_result() const override
  { return true; }
};

struct touch_of_the_magi_explosion_t final : public arcane_mage_spell_t
{
  touch_of_the_magi_explosion_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 210833 ) )
  {
    background = true;
    may_miss = may_crit = callbacks = false;
    affected_by.radiant_spark = triggers.touch_of_the_magi = false;
    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    base_dd_min = base_dd_max = 1.0;
  }

  void init() override
  {
    arcane_mage_spell_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  double composite_target_da_multiplier( player_t* target ) const override
  {
    // Touch of the Magi explosion ignores debuffs with effect subtype 270 (Modify
    // Damage Taken% from Caster) or 271 (Modify Damage Taken% from Caster's Spells).
    double m = composite_target_damage_vulnerability( target );

    // For some reason, Touch of the Magi triple dips damage reductions.
    return m * std::min( m, 1.0 );
  }
};

struct arcane_echo_t final : public arcane_mage_spell_t
{
  arcane_echo_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 342232 ) )
  {
    aoe = -1;
    reduced_aoe_targets = p->talents.arcane_echo->effectN( 1 ).base_value();
    background = affected_by.savant = true;
    callbacks = false;
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
  }
};

// ==========================================================================
// Mage Covenant Abilities
// ==========================================================================

// Deathborne Spell =========================================================

struct deathborne_t final : public mage_spell_t
{
  deathborne_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_covenant_spell( "Deathborne" ) )
  {
    parse_options( options_str );
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

struct agonizing_backlash_t final : public mage_spell_t
{
  agonizing_backlash_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 320035 ) )
  {
    background = true;
    callbacks = false;
  }
};

struct tormenting_backlash_t final : public mage_spell_t
{
  tormenting_backlash_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 317589 ) )
  {
    background = true;
    callbacks = false;
  }
};

struct mirrors_of_torment_t final : public mage_spell_t
{
  mirrors_of_torment_t( std::string_view n, mage_t* p, std::string_view options_str ) :
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

    if ( result_is_hit( s->result ) )
      get_td( s->target )->debuffs.mirrors_of_torment->trigger();
  }
};

// Radiant Spark Spell ======================================================

struct harmonic_echo_t final : public mage_spell_t
{
  harmonic_echo_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->talents.harmonic_echo.ok() ? p->find_spell( 384685 ) : p->find_spell( 354189 ) )
  {
    background = true;
    may_miss = may_crit = callbacks = false;
    affected_by.radiant_spark = triggers.touch_of_the_magi = false;
    base_dd_min = base_dd_max = 1.0;
  }

  void init() override
  {
    mage_spell_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    // Ignore Positive Damage Taken Modifiers (321)
    return std::min( mage_spell_t::composite_target_multiplier( target ), 1.0 );
  }
};

struct radiant_spark_t final : public mage_spell_t
{
  radiant_spark_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.radiant_spark.ok() ? p->talents.radiant_spark : p->find_covenant_spell( "Radiant Spark" ) )
  {
    parse_options( options_str );
    affected_by.ice_floes = affected_by.savant = true;
    if ( !p->talents.radiant_spark.ok() )
      affected_by.radiant_spark = false;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // Create the vulnerability debuff for this target if it doesn't exist yet.
      // This is necessary because mage_spell_t::assess_damage does not create the
      // target data by itself.
      auto td = get_td( s->target );
      // If Radiant Spark is refreshed, the vulnerability debuff can be
      // triggered once again. Any previous stacks of the debuff are removed.
      td->debuffs.radiant_spark_vulnerability->cooldown->reset( false );
      td->debuffs.radiant_spark_vulnerability->expire();
    }
  }

  void last_tick( dot_t* d ) override
  {
    mage_spell_t::last_tick( d );

    if ( auto td = find_td( d->target ) )
      td->debuffs.radiant_spark_vulnerability->expire();
  }
};

// Shifting Power Spell =====================================================

struct shifting_power_pulse_t final : public mage_spell_t
{
  shifting_power_pulse_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 382445 ) ) // TODO: do we want to do anything aobut the covenant one? (id 325130)
  {
    background = true;
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    // TODO: Check what happens if the spell misses.
    if ( !p()->talents.shifting_power.ok() )
      p()->buffs.heart_of_the_fae->trigger();
  }
};

struct shifting_power_t final : public mage_spell_t
{
  std::vector<cooldown_t*> shifting_power_cooldowns;
  timespan_t reduction;

  shifting_power_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.shifting_power.ok() ? p->talents.shifting_power : p->find_covenant_spell( "Shifting Power" ) ),
    shifting_power_cooldowns(),
    reduction( data().effectN( 2 ).time_value() + ( p->talents.shifting_power.ok() ? 0_ms : p->conduits.discipline_of_the_grove.time_value() ) )
  {
    parse_options( options_str );
    channeled = affected_by.ice_floes = true;
    affected_by.shifting_power = false;
    tick_action = get_action<shifting_power_pulse_t>( "shifting_power_pulse", p );
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }

  void init_finished() override
  {
    mage_spell_t::init_finished();

    for ( auto a : p()->action_list )
    {
      auto m = dynamic_cast<mage_spell_t*>( a );
      if ( m && m->affected_by.shifting_power && !range::contains( shifting_power_cooldowns, m->cooldown ) )
        shifting_power_cooldowns.push_back( m->cooldown );
    }
  }

  bool usable_moving() const override
  {
    if ( !p()->talents.shifting_power.ok() && p()->runeforge.heart_of_the_fae.ok() )
      return true;

    return mage_spell_t::usable_moving();
  }

  void tick( dot_t* d ) override
  {
    mage_spell_t::tick( d );

    for ( auto cd : shifting_power_cooldowns )
      cd->adjust( reduction, false );
  }

  std::unique_ptr<expr_t> create_expression( std::string_view name ) override
  {
    if ( util::str_compare_ci( name, "tick_reduction" ) )
      return expr_t::create_constant( name, data().ok() ? -reduction.total_seconds() : 0.0 );

    if ( util::str_compare_ci( name, "full_reduction" ) )
      return expr_t::create_constant( name, data().ok() ? -reduction.total_seconds() * dot_duration / base_tick_time : 0.0 );

    return mage_spell_t::create_expression( name );
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

struct start_burn_phase_t final : public action_t
{
  start_burn_phase_t( std::string_view n, mage_t* p, std::string_view options_str ) :
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

struct stop_burn_phase_t final : public action_t
{
  stop_burn_phase_t( std::string_view n, mage_t* p, std::string_view options_str ) :
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

// Proxy Water Elemental Actions ======================================================

struct freeze_t final : public action_t
{
  freeze_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    action_t( ACTION_OTHER, n, p )
  {
    parse_options( options_str );
    trigger_gcd = 0_ms;
    may_miss = may_crit = callbacks = false;
    dual = usable_while_casting = ignore_false_positive = true;

    if ( !p->talents.summon_water_elemental.ok() )
      background = true;
  }

  void execute() override
  {
    debug_cast<mage_t*>( player )->action.pet_freeze->execute_on_target( target );
  }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );
    if ( !p->pets.water_elemental || p->pets.water_elemental->is_sleeping() )
      return false;

    // Make sure the cooldown is actually ready and not just within cooldown tolerance.
    if ( !p->action.pet_freeze->cooldown->up() || !p->action.pet_freeze->ready() )
      return false;

    return action_t::ready();
  }
};

struct water_jet_t final : public action_t
{
  water_jet_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    action_t( ACTION_OTHER, n, p )
  {
    parse_options( options_str );
    trigger_gcd = 0_ms;
    may_miss = may_crit = callbacks = false;
    dual = usable_while_casting = ignore_false_positive = true;

    if ( !p->talents.summon_water_elemental.ok() )
      background = true;
  }

  void execute() override
  {
    mage_t* mage = debug_cast<mage_t*>( player );
    pet_t* pet = mage->pets.water_elemental;

    pet->interrupt();
    event_t::cancel( pet->readying );

    mage->action.pet_water_jet->set_target( target );
    mage->action.pet_water_jet->schedule_execute();
  }

  bool ready() override
  {
    mage_t* mage = debug_cast<mage_t*>( player );
    pet_t* pet = mage->pets.water_elemental;

    if ( !pet || pet->is_sleeping() )
      return false;

    // Make sure the cooldown is actually ready and not just within cooldown tolerance.
    if ( !mage->action.pet_water_jet->cooldown->up() || !mage->action.pet_water_jet->ready() )
      return false;

    // Prevent recasting if Water Elemental is already executing Water Jet
    if ( pet->executing == mage->action.pet_water_jet )
      return false;

    return action_t::ready();
  }
};

}  // namespace actions


namespace events {

struct enlightened_event_t final : public event_t
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
    // Do a non-forced regen first to figure out if we have enough mana to swap the buffs.
    mage->do_dynamic_regen();
    mage->update_enlightened( true );
    mage->events.enlightened = make_event<enlightened_event_t>( sim(), *mage, 2.0_s );
  }
};

struct icicle_event_t final : public event_t
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

    icicle_action->execute_on_target( target );

    if ( !mage->icicles.empty() )
    {
      mage->events.icicle = make_event<icicle_event_t>( sim(), *mage, target );
      sim().print_debug( "{} icicle use on {} (chained), total={}", mage->name(), target->name(), mage->icicles.size() );
    }
  }
};

struct from_the_ashes_event_t final : public event_t
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
    mage->events.from_the_ashes = make_event<from_the_ashes_event_t>( sim(), *mage, 2.0_s );
  }
};

struct time_anomaly_tick_event_t final : public event_t
{
  mage_t* mage;

  enum ta_proc_type_e
  {
    TA_ARCANE_SURGE,
    TA_CLEARCASTING,
    TA_COMBUSTION,
    TA_FIRE_BLAST,
    TA_ICY_VEINS,
    TA_FINGERS_OF_FROST,
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

      auto spec = mage->specialization();

      // TODO: these conditions haven't been tested; also check what happens if relevant talents like Icy Veins are not selected
      if ( spec == MAGE_ARCANE && !mage->buffs.arcane_surge->check() )
        possible_procs.push_back( TA_ARCANE_SURGE );
      if ( spec == MAGE_ARCANE && !mage->buffs.clearcasting->at_max_stacks() )
        possible_procs.push_back( TA_CLEARCASTING );
      if ( spec == MAGE_FIRE && !mage->buffs.combustion->check() )
        possible_procs.push_back( TA_COMBUSTION );
      if ( spec == MAGE_FIRE && mage->cooldowns.fire_blast->current_charge != mage->cooldowns.fire_blast->charges )
        possible_procs.push_back( TA_FIRE_BLAST );
      if ( spec == MAGE_FROST && !mage->buffs.icy_veins->check() )
        possible_procs.push_back( TA_ICY_VEINS );
      if ( spec == MAGE_FROST && !mage->buffs.fingers_of_frost->at_max_stacks() )
        possible_procs.push_back( TA_FINGERS_OF_FROST );
      if ( !mage->buffs.time_warp->check() )
        possible_procs.push_back( TA_TIME_WARP );

      if ( !possible_procs.empty() )
      {
        auto proc = possible_procs[ rng().range( possible_procs.size() ) ];
        switch ( proc )
        {
          case TA_ARCANE_SURGE:
            mage->buffs.arcane_surge->trigger( 1000 * mage->talents.time_anomaly->effectN( 1 ).time_value() );
            break;
          case TA_CLEARCASTING:
            mage->buffs.clearcasting->trigger(); // TODO: does this need delayed trigger?
            break;
          case TA_COMBUSTION:
            mage->buffs.combustion->trigger( 1000 * mage->talents.time_anomaly->effectN( 4 ).time_value() );
            break;
          case TA_FIRE_BLAST:
            mage->cooldowns.fire_blast->reset( true );
            break;
          case TA_FINGERS_OF_FROST:
            mage->trigger_fof( 1.0, mage->procs.fingers_of_frost_time_anomaly );
            break;
          case TA_ICY_VEINS:
            mage->buffs.icy_veins->trigger( 1000 * mage->talents.time_anomaly->effectN( 5 ).time_value() );
            break;
          case TA_TIME_WARP:
            mage->buffs.time_warp->trigger();
            break;
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
  dots.radiant_spark  = target->get_dot( "radiant_spark", mage );

  debuffs.frozen                      = make_buff( *this, "frozen" )
                                          ->set_duration( mage->options.frozen_duration );
  debuffs.improved_scorch             = make_buff( *this, "improved_scorch", mage->find_spell( 383608 ) )
                                          ->set_schools_from_effect( 1 )
                                          ->set_default_value( mage->talents.improved_scorch->effectN( 3 ).percent() );
  debuffs.radiant_spark_vulnerability = make_buff( *this, "radiant_spark_vulnerability", mage->talents.radiant_spark.ok() ? mage->find_spell( 376104 ) : mage->find_spell( 307454 ) )
                                          ->set_activated( false )
                                          ->set_default_value_from_effect( 1 )
                                          ->modify_default_value( mage->conduits.ire_of_the_ascended.percent() ) // TODO: Confirm that Ire of the Ascended affects the talent.
                                          ->set_refresh_behavior( buff_refresh_behavior::DISABLED );
  debuffs.touch_of_the_magi           = make_buff<buffs::touch_of_the_magi_t>( this );
  debuffs.winters_chill               = make_buff( *this, "winters_chill", mage->find_spell( 228358 ) );

  // Runeforge Legendaries
  debuffs.grisly_icicle = make_buff( *this, "grisly_icicle", mage->find_spell( 348007 ) )
                            ->set_default_value_from_effect( 1 )
                            ->set_chance( mage->runeforge.grisly_icicle.ok() );

  // Covenant Abilities
  debuffs.mirrors_of_torment = make_buff<buffs::mirrors_of_torment_t>( this );
}

mage_t::mage_t( sim_t* sim, std::string_view name, race_e r ) :
  player_t( sim, MAGE, name, r ),
  events(),
  ground_aoe_expiration(),
  action(),
  benefits(),
  buffs(),
  cooldowns(),
  gains(),
  options(),
  pets(),
  procs(),
  rppm(),
  shuffled_rng(),
  sample_data(),
  spec(),
  state(),
  expression_support(),
  talents(),
  runeforge(),
  conduits(),
  uptime()
{
  // Cooldowns
  cooldowns.combustion           = get_cooldown( "combustion"           );
  cooldowns.cone_of_cold         = get_cooldown( "cone_of_cold"         );
  cooldowns.fervent_flickering   = get_cooldown( "fervent_flickering"   );
  cooldowns.fire_blast           = get_cooldown( "fire_blast"           );
  cooldowns.flurry               = get_cooldown( "flurry"               );
  cooldowns.from_the_ashes       = get_cooldown( "from_the_ashes"       );
  cooldowns.frost_nova           = get_cooldown( "frost_nova"           );
  cooldowns.frozen_orb           = get_cooldown( "frozen_orb"           );
  cooldowns.icy_veins            = get_cooldown( "icy_veins"            );
  cooldowns.incendiary_eruptions = get_cooldown( "incendiary_eruptions" );
  cooldowns.mirrors_of_torment   = get_cooldown( "mirrors_of_torment"   );
  cooldowns.phoenix_flames       = get_cooldown( "phoenix_flames"       );
  cooldowns.phoenix_reborn       = get_cooldown( "phoenix_reborn"       );
  cooldowns.presence_of_mind     = get_cooldown( "presence_of_mind"     );
  cooldowns.snowstorm            = get_cooldown( "snowstorm"            );

  // Options
  resource_regeneration = regen_type::DYNAMIC;
}

bool mage_t::trigger_crowd_control( const action_state_t* s, spell_mechanic type, timespan_t d )
{
  if ( type == MECHANIC_INTERRUPT )
    return s->target->debuffs.casting->check();

  if ( action_t::result_is_hit( s->result )
    && ( s->target->is_add() || s->target->level() < sim->max_player_level + 3 ) )
  {
    if ( type == MECHANIC_ROOT )
      get_target_data( s->target )->debuffs.frozen->trigger( d );

    return true;
  }

  return false;
}

void mage_t::trigger_disciplinary_command( school_e school )
{
  if ( !runeforge.disciplinary_command.ok() || buffs.disciplinary_command->cooldown->down() )
    return;

  // Only one school is triggered for Disciplinary Command if multiple are present.
  // Schools are checked in order from the largest school mask to smallest.
  if ( dbc::is_school( school, SCHOOL_ARCANE ) )
    buffs.disciplinary_command_arcane->trigger();
  else if ( dbc::is_school( school, SCHOOL_FROST ) )
    buffs.disciplinary_command_frost->trigger();
  else if ( dbc::is_school( school, SCHOOL_FIRE ) )
    buffs.disciplinary_command_fire->trigger();
  if ( buffs.disciplinary_command_arcane->check() && buffs.disciplinary_command_frost->check() && buffs.disciplinary_command_fire->check() )
  {
    buffs.disciplinary_command->trigger();
    buffs.disciplinary_command_arcane->expire();
    buffs.disciplinary_command_frost->expire();
    buffs.disciplinary_command_fire->expire();
  }
}

void mage_t::trigger_sinful_delight( specialization_e spec )
{
  if ( runeforge.sinful_delight.ok() && specialization() == spec )
    cooldowns.mirrors_of_torment->adjust( -runeforge.sinful_delight->effectN( 1 ).time_value() );
}

void mage_t::trigger_time_manipulation()
{
  if ( !talents.time_manipulation.ok() )
    return;

  timespan_t t = talents.time_manipulation->effectN( 1 ).time_value();
  for ( auto cd : time_manipulation_cooldowns ) cd->adjust( t, false );
}

action_t* mage_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace actions;

  // Arcane
  if ( name == "arcane_barrage"         ) return new         arcane_barrage_t( name, this, options_str );
  if ( name == "arcane_blast"           ) return new           arcane_blast_t( name, this, options_str );
  if ( name == "arcane_familiar"        ) return new        arcane_familiar_t( name, this, options_str );
  if ( name == "arcane_missiles"        ) return new        arcane_missiles_t( name, this, options_str );
  if ( name == "arcane_orb"             ) return new             arcane_orb_t( name, this, options_str );
  if ( name == "arcane_surge"           ) return new           arcane_surge_t( name, this, options_str );
  if ( name == "conjure_mana_gem"       ) return new       conjure_mana_gem_t( name, this, options_str );
  if ( name == "evocation"              ) return new              evocation_t( name, this, options_str );
  if ( name == "nether_tempest"         ) return new         nether_tempest_t( name, this, options_str );
  if ( name == "presence_of_mind"       ) return new       presence_of_mind_t( name, this, options_str );
  if ( name == "radiant_spark"          ) return new          radiant_spark_t( name, this, options_str );
  if ( name == "supernova"              ) return new              supernova_t( name, this, options_str );
  if ( name == "touch_of_the_magi"      ) return new      touch_of_the_magi_t( name, this, options_str );
  if ( name == "use_mana_gem"           ) return new           use_mana_gem_t( name, this, options_str );

  if ( name == "start_burn_phase"       ) return new       start_burn_phase_t( name, this, options_str );
  if ( name == "stop_burn_phase"        ) return new        stop_burn_phase_t( name, this, options_str );

  // Fire
  if ( name == "combustion"             ) return new             combustion_t( name, this, options_str );
  if ( name == "fireball"               ) return new               fireball_t( name, this, options_str );
  if ( name == "flamestrike"            ) return new            flamestrike_t( name, this, options_str );
  if ( name == "living_bomb"            ) return new            living_bomb_t( name, this, options_str );
  if ( name == "phoenix_flames"         ) return new         phoenix_flames_t( name, this, options_str );
  if ( name == "pyroblast"              ) return new              pyroblast_t( name, this, options_str );
  if ( name == "scorch"                 ) return new                 scorch_t( name, this, options_str );

  // Frost
  if ( name == "blizzard"               ) return new               blizzard_t( name, this, options_str );
  if ( name == "cold_snap"              ) return new              cold_snap_t( name, this, options_str );
  if ( name == "comet_storm"            ) return new            comet_storm_t( name, this, options_str );
  if ( name == "ebonbolt"               ) return new               ebonbolt_t( name, this, options_str );
  if ( name == "flurry"                 ) return new                 flurry_t( name, this, options_str );
  if ( name == "frozen_orb"             ) return new             frozen_orb_t( name, this, options_str );
  if ( name == "glacial_spike"          ) return new          glacial_spike_t( name, this, options_str );
  if ( name == "ice_lance"              ) return new              ice_lance_t( name, this, options_str );
  if ( name == "icy_veins"              ) return new              icy_veins_t( name, this, options_str );
  if ( name == "ray_of_frost"           ) return new           ray_of_frost_t( name, this, options_str );
  if ( name == "summon_water_elemental" ) return new summon_water_elemental_t( name, this, options_str );

  if ( name == "freeze"                 ) return new                 freeze_t( name, this, options_str );
  if ( name == "water_jet"              ) return new              water_jet_t( name, this, options_str );

  // Shared
  if ( name == "arcane_explosion"       ) return new       arcane_explosion_t( name, this, options_str );
  if ( name == "arcane_intellect"       ) return new       arcane_intellect_t( name, this, options_str );
  if ( name == "blast_wave"             ) return new             blast_wave_t( name, this, options_str );
  if ( name == "blink"                  ) return new                  blink_t( name, this, options_str );
  if ( name == "cone_of_cold"           ) return new           cone_of_cold_t( name, this, options_str );
  if ( name == "counterspell"           ) return new           counterspell_t( name, this, options_str );
  if ( name == "dragons_breath"         ) return new         dragons_breath_t( name, this, options_str );
  if ( name == "fire_blast"             ) return new             fire_blast_t( name, this, options_str );
  if ( name == "frost_nova"             ) return new             frost_nova_t( name, this, options_str );
  if ( name == "frostbolt"              ) return new              frostbolt_t( name, this, options_str );
  if ( name == "ice_floes"              ) return new              ice_floes_t( name, this, options_str );
  if ( name == "ice_nova"               ) return new               ice_nova_t( name, this, options_str );
  if ( name == "meteor"                 ) return new                 meteor_t( name, this, options_str );
  if ( name == "mirror_image"           ) return new           mirror_image_t( name, this, options_str );
  if ( name == "rune_of_power"          ) return new          rune_of_power_t( name, this, options_str );
  if ( name == "shifting_power"         ) return new         shifting_power_t( name, this, options_str );
  if ( name == "shimmer"                ) return new                shimmer_t( name, this, options_str );
  if ( name == "slow"                   ) return new                   slow_t( name, this, options_str );
  if ( name == "time_warp"              ) return new              time_warp_t( name, this, options_str );

  // Covenant Abilities
  if ( name == "deathborne"             ) return new             deathborne_t( name, this, options_str );
  if ( name == "mirrors_of_torment"     ) return new     mirrors_of_torment_t( name, this, options_str );

  // Special
  if ( name == "blink_any" || name == "any_blink" )
    return create_action( talents.shimmer.ok() ? "shimmer" : "blink", options_str );

  return player_t::create_action( name, options_str );
}

void mage_t::create_actions()
{
  using namespace actions;

  if ( spec.ignite->ok() )
    action.ignite = get_action<ignite_t>( "ignite", this );

  if ( spec.icicles->ok() )
  {
    action.icicle.frostbolt = get_action<icicle_t>( "frostbolt_icicle", this );
    action.icicle.flurry    = get_action<icicle_t>( "flurry_icicle", this );
    action.icicle.ice_lance = get_action<icicle_t>( "ice_lance_icicle", this );
  }

  if ( talents.arcane_familiar.ok() )
    action.arcane_assault = get_action<arcane_assault_t>( "arcane_assault", this );

  if ( talents.arcane_echo.ok() )
    action.arcane_echo = get_action<arcane_echo_t>( "arcane_echo", this );

  if ( talents.conflagration.ok() )
    action.conflagration_flare_up = get_action<conflagration_flare_up_t>( "conflagration_flare_up", this );

  if ( talents.living_bomb.ok() || talents.incendiary_eruptions.ok() )
  {
    action.living_bomb_dot        = get_action<living_bomb_dot_t>( "living_bomb_dot", this, true );
    action.living_bomb_dot_spread = get_action<living_bomb_dot_t>( "living_bomb_dot_spread", this, false );
    action.living_bomb_explosion  = get_action<living_bomb_explosion_t>( "living_bomb_explosion", this );
  }

  if ( talents.glacial_assault.ok() )
    action.glacial_assault = get_action<glacial_assault_t>( "glacial_assault", this );

  if ( talents.touch_of_the_magi.ok() )
    action.touch_of_the_magi_explosion = get_action<touch_of_the_magi_explosion_t>( "touch_of_the_magi_explosion", this );

  if ( talents.orb_barrage.ok() )
    action.orb_barrage_arcane_orb = get_action<arcane_orb_t>( "orb_barrage_arcane_orb", this, "", true );

  if ( talents.firefall.ok() || runeforge.molten_skyfall.ok() )
    action.firefall_meteor = get_action<meteor_t>( "firefall_meteor", this, "", true );

  if ( talents.cold_front.ok() || runeforge.cold_front.ok() )
    action.cold_front_frozen_orb = get_action<frozen_orb_t>( "cold_front_frozen_orb", this, "", true );

  if ( talents.harmonic_echo.ok() || runeforge.harmonic_echo.ok() )
    action.harmonic_echo = get_action<harmonic_echo_t>( "harmonic_echo", this );

  if ( find_covenant_spell( "Mirrors of Torment" )->ok() )
  {
    action.agonizing_backlash  = get_action<agonizing_backlash_t>( "agonizing_backlash", this );
    action.tormenting_backlash = get_action<tormenting_backlash_t>( "tormenting_backlash", this );
  }

  player_t::create_actions();

  // Ensure the cooldown of Phoenix Flames is properly initialized.
  if ( talents.from_the_ashes.ok() && !find_action( "phoenix_flames" ) )
    create_action( "phoenix_flames", "" );
}

void mage_t::create_options()
{
  add_option( opt_float( "mage.firestarter_duration_multiplier", options.firestarter_duration_multiplier ) );
  add_option( opt_float( "mage.searing_touch_duration_multiplier", options.searing_touch_duration_multiplier ) );
  add_option( opt_timespan( "mage.frozen_duration", options.frozen_duration ) );
  add_option( opt_timespan( "mage.scorch_delay", options.scorch_delay ) );
  add_option( opt_timespan( "mage.mirrors_of_torment_interval", options.mirrors_of_torment_interval, 1_ms, timespan_t::max() ) );
  add_option( opt_timespan( "mage.arcane_missiles_chain_delay", options.arcane_missiles_chain_delay, 0_ms, timespan_t::max() ) );
  add_option( opt_float( "mage.arcane_missiles_chain_relstddev", options.arcane_missiles_chain_relstddev, 0.0, std::numeric_limits<double>::max() ) );

  player_t::create_options();
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

  for ( size_t i = 0; i < shatter_source_list.size(); i++ )
  {
    auto ours = shatter_source_list[ i ];
    auto theirs = mage.shatter_source_list[ i ];
    assert( ours->name_str == theirs->name_str );
    ours->merge( *theirs );
  }

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      sample_data.burn_duration_history->merge( *mage.sample_data.burn_duration_history );
      sample_data.burn_initial_mana->merge( *mage.sample_data.burn_initial_mana );
      break;
    case MAGE_FROST:
      if ( talents.thermal_void.ok() )
        sample_data.icy_veins_duration->merge( *mage.sample_data.icy_veins_duration );
      break;
    default:
      break;
  }
}

void mage_t::analyze( sim_t& s )
{
  player_t::analyze( s );

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      sample_data.burn_duration_history->analyze();
      sample_data.burn_initial_mana->analyze();
      break;
    case MAGE_FROST:
      if ( talents.thermal_void.ok() )
        sample_data.icy_veins_duration->analyze();
      break;
    default:
      break;
  }
}

void mage_t::datacollection_begin()
{
  player_t::datacollection_begin();

  range::for_each( shatter_source_list, std::mem_fn( &shatter_source_t::datacollection_begin ) );
}

void mage_t::datacollection_end()
{
  player_t::datacollection_end();

  range::for_each( shatter_source_list, std::mem_fn( &shatter_source_t::datacollection_end ) );
}

void mage_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( resources.is_active( RESOURCE_MANA ) && buffs.arcane_surge->check() )
  {
    double base = resource_regen_per_second( RESOURCE_MANA );
    if ( base )
    {
      // Base regen was already done, so we don't need to add 1.0 to Arcane Surge's mana regen multiplier.
      double amount = buffs.arcane_surge->check_value() * base * periodicity.total_seconds();
      resource_gain( RESOURCE_MANA, amount, gains.arcane_surge );
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

  if ( talents.summon_water_elemental.ok() && find_action( "summon_water_elemental" ) )
    pets.water_elemental = new pets::water_elemental::water_elemental_pet_t( sim, this );

  if ( talents.mirror_image.ok() && find_action( "mirror_image" ) )
  {
    for ( int i = 0; i < as<int>( talents.mirror_image->effectN( 2 ).base_value() ); i++ )
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

  // Mage Talents
  // Row 1
  talents.blazing_barrier          = find_talent_spell( talent_tree::CLASS, "Blazing Barrier"          );
  talents.ice_barrier              = find_talent_spell( talent_tree::CLASS, "Ice Barrier"              );
  talents.prismatic_barrier        = find_talent_spell( talent_tree::CLASS, "Prismatic Barrier"        );
  // Row 2
  talents.ice_block                = find_talent_spell( talent_tree::CLASS, "Ice Block"                );
  talents.overflowing_energy       = find_talent_spell( talent_tree::CLASS, "Overflowing Energy"       );
  talents.invisibility             = find_talent_spell( talent_tree::CLASS, "Invisibility"             );
  // Row 3
  talents.winters_protection       = find_talent_spell( talent_tree::CLASS, "Winter's Protection"      );
  talents.spellsteal               = find_talent_spell( talent_tree::CLASS, "Spellsteal"               );
  talents.tempest_barrier          = find_talent_spell( talent_tree::CLASS, "Tempest Barrier"          );
  talents.incantation_of_swiftness = find_talent_spell( talent_tree::CLASS, "Incantation of Swiftness" );
  talents.remove_curse             = find_talent_spell( talent_tree::CLASS, "Remove Curse"             );
  talents.arcane_warding           = find_talent_spell( talent_tree::CLASS, "Arcane Warding"           );
  // Row 4
  talents.mirror_image             = find_talent_spell( talent_tree::CLASS, "Mirror Image"             );
  talents.incanters_flow           = find_talent_spell( talent_tree::CLASS, "Incanter's Flow"          );
  talents.rune_of_power            = find_talent_spell( talent_tree::CLASS, "Rune of Power"            );
  talents.alter_time               = find_talent_spell( talent_tree::CLASS, "Alter Time"               );
  // Row 5
  talents.cryofreeze               = find_talent_spell( talent_tree::CLASS, "Cryo-Freeze"              );
  talents.reabsorption             = find_talent_spell( talent_tree::CLASS, "Reabsorption"             );
  talents.reduplication            = find_talent_spell( talent_tree::CLASS, "Reduplication"            );
  talents.quick_witted             = find_talent_spell( talent_tree::CLASS, "Quick Witted"             );
  talents.mass_polymorph           = find_talent_spell( talent_tree::CLASS, "Mass Polymorph"           );
  talents.slow                     = find_talent_spell( talent_tree::CLASS, "Slow"                     );
  talents.master_of_time           = find_talent_spell( talent_tree::CLASS, "Master of Time"           );
  talents.diverted_energy          = find_talent_spell( talent_tree::CLASS, "Diverted Energy"          );
  // Row 6
  talents.ring_of_frost            = find_talent_spell( talent_tree::CLASS, "Ring of Frost"            );
  talents.ice_nova                 = find_talent_spell( talent_tree::CLASS, "Ice Nova"                 );
  talents.ice_floes                = find_talent_spell( talent_tree::CLASS, "Ice Floes"                );
  talents.shimmer                  = find_talent_spell( talent_tree::CLASS, "Shimmer"                  );
  talents.mass_slow                = find_talent_spell( talent_tree::CLASS, "Mass Slow"                );
  talents.blast_wave               = find_talent_spell( talent_tree::CLASS, "Blast Wave"               );
  // Row 7
  talents.improved_frost_nova      = find_talent_spell( talent_tree::CLASS, "Improved Frost Nova"      );
  talents.rigid_ice                = find_talent_spell( talent_tree::CLASS, "Rigid Ice"                );
  talents.tome_of_rhonin           = find_talent_spell( talent_tree::CLASS, "Tome of Rhonin"           );
  talents.tome_of_antonidas        = find_talent_spell( talent_tree::CLASS, "Tome of Antonidas"        );
  talents.volatile_detonation      = find_talent_spell( talent_tree::CLASS, "Volatile Detonation"      );
  talents.energized_barriers       = find_talent_spell( talent_tree::CLASS, "Energized Barriers"       );
  // Row 8
  talents.frigid_winds             = find_talent_spell( talent_tree::CLASS, "Frigid Winds"             );
  talents.flow_of_time             = find_talent_spell( talent_tree::CLASS, "Flow of Time"             );
  talents.temporal_velocity        = find_talent_spell( talent_tree::CLASS, "Temporal Velocity"        );
  // Row 9
  talents.ice_ward                 = find_talent_spell( talent_tree::CLASS, "Ice Ward"                 );
  talents.time_manipulation        = find_talent_spell( talent_tree::CLASS, "Time Manipulation"        );
  talents.greater_invisibility     = find_talent_spell( talent_tree::CLASS, "Greater Invisibility"     );
  talents.accumulative_shielding   = find_talent_spell( talent_tree::CLASS, "Accumulative Shielding"   );
  talents.dragons_breath           = find_talent_spell( talent_tree::CLASS, "Dragon's Breath"          );
  // Row 10
  talents.shifting_power           = find_talent_spell( talent_tree::CLASS, "Shifting Power"           );
  talents.freezing_cold            = find_talent_spell( talent_tree::CLASS, "Freezing Cold"            );
  talents.temporal_warp            = find_talent_spell( talent_tree::CLASS, "Temporal Warp"            );
  talents.time_anomaly             = find_talent_spell( talent_tree::CLASS, "Time Anomaly"             );
  talents.displacement             = find_talent_spell( talent_tree::CLASS, "Displacement"             );
  talents.meteor                   = find_talent_spell( talent_tree::CLASS, "Meteor"                   );

  // Arcane
  // Row 1
  talents.arcane_barrage             = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Barrage"             );
  // Row 2
  talents.arcane_missiles            = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Missiles"            );
  // Row 3
  talents.arcane_orb                 = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Orb"                 );
  talents.clearcasting               = find_talent_spell( talent_tree::SPECIALIZATION, "Clearcasting"               );
  // Row 4
  talents.arcane_tempo               = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Tempo"               );
  talents.improved_arcane_missiles   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Arcane Missiles"   );
  talents.arcane_surge               = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Surge"               );
  talents.crackling_energy           = find_talent_spell( talent_tree::SPECIALIZATION, "Crackling Energy"           );
  talents.impetus                    = find_talent_spell( talent_tree::SPECIALIZATION, "Impetus"                    );
  // Row 5
  talents.arcane_familiar            = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Familiar"            );
  talents.rule_of_threes             = find_talent_spell( talent_tree::SPECIALIZATION, "Rule of Threes"             );
  talents.charged_orb                = find_talent_spell( talent_tree::SPECIALIZATION, "Charged Orb"                );
  talents.arcing_cleave              = find_talent_spell( talent_tree::SPECIALIZATION, "Arcing Cleave"              );
  talents.arcane_power               = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Power"               );
  talents.mana_adept                 = find_talent_spell( talent_tree::SPECIALIZATION, "Mana Adept"                 );
  talents.nether_precision           = find_talent_spell( talent_tree::SPECIALIZATION, "Nether Precision"           );
  talents.amplification              = find_talent_spell( talent_tree::SPECIALIZATION, "Amplification"              );
  // Row 6
  talents.presence_of_mind           = find_talent_spell( talent_tree::SPECIALIZATION, "Presence of Mind"           );
  talents.foresight                  = find_talent_spell( talent_tree::SPECIALIZATION, "Foresight"                  );
  talents.resonance                  = find_talent_spell( talent_tree::SPECIALIZATION, "Resonance"                  );
  talents.nether_tempest             = find_talent_spell( talent_tree::SPECIALIZATION, "Nether Tempest"             );
  talents.reverberate                = find_talent_spell( talent_tree::SPECIALIZATION, "Reverberate"                );
  talents.improved_prismatic_barrier = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Prismatic Barrier" );
  talents.slipstream                 = find_talent_spell( talent_tree::SPECIALIZATION, "Slipstream"                 );
  talents.improved_clearcasting      = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Clearcasting"      );
  // Row 7
  talents.chrono_shift               = find_talent_spell( talent_tree::SPECIALIZATION, "Chrono Shift"               );
  talents.touch_of_the_magi          = find_talent_spell( talent_tree::SPECIALIZATION, "Touch of the Magi"          );
  talents.supernova                  = find_talent_spell( talent_tree::SPECIALIZATION, "Supernova"                  );
  // Row 8
  talents.evocation                  = find_talent_spell( talent_tree::SPECIALIZATION, "Evocation"                  );
  talents.enlightened                = find_talent_spell( talent_tree::SPECIALIZATION, "Enlightened"                );
  talents.arcane_echo                = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Echo"                );
  talents.arcane_bombardment         = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Bombardment"         );
  talents.illuminated_thoughts       = find_talent_spell( talent_tree::SPECIALIZATION, "Illuminated Thoughts"       );
  talents.conjure_mana_gem           = find_talent_spell( talent_tree::SPECIALIZATION, "Conjure Mana Gem"           );
  // Row 9
  talents.siphon_storm               = find_talent_spell( talent_tree::SPECIALIZATION, "Siphon Storm"               );
  talents.prodigious_savant          = find_talent_spell( talent_tree::SPECIALIZATION, "Prodigious Savant"          );
  talents.radiant_spark              = find_talent_spell( talent_tree::SPECIALIZATION, "Radiant Spark"              );
  talents.concentration              = find_talent_spell( talent_tree::SPECIALIZATION, "Concentration"              );
  talents.cascading_power            = find_talent_spell( talent_tree::SPECIALIZATION, "Cascading Power"            );
  // Row 10
  talents.orb_barrage                = find_talent_spell( talent_tree::SPECIALIZATION, "Orb Barrage"                );
  talents.harmonic_echo              = find_talent_spell( talent_tree::SPECIALIZATION, "Harmonic Echo"              );
  talents.arcane_harmony             = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Harmony"             );

  // Fire
  // Row 1
  talents.pyroblast              = find_talent_spell( talent_tree::SPECIALIZATION, "Pyroblast"              );
  // Row 2
  talents.fire_blast             = find_talent_spell( talent_tree::SPECIALIZATION, "Fire Blast"             );
  talents.fervent_flickering     = find_talent_spell( talent_tree::SPECIALIZATION, "Fervent Flickering"     );
  // Row 3
  talents.scorch                 = find_talent_spell( talent_tree::SPECIALIZATION, "Scorch"                 );
  talents.phoenix_flames         = find_talent_spell( talent_tree::SPECIALIZATION, "Phoenix Flames"         );
  talents.flamestrike            = find_talent_spell( talent_tree::SPECIALIZATION, "Flamestrike"            );
  // Row 4
  talents.searing_touch          = find_talent_spell( talent_tree::SPECIALIZATION, "Searing Touch"          );
  talents.firestarter            = find_talent_spell( talent_tree::SPECIALIZATION, "Firestarter"            );
  talents.pyrotechnics           = find_talent_spell( talent_tree::SPECIALIZATION, "Pyrotechnics"           );
  talents.improved_flamestrike   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Flamestrike"   );
  // Row 5
  talents.improved_scorch        = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Scorch"        );
  talents.critical_mass          = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Mass"          );
  talents.cauterize              = find_talent_spell( talent_tree::SPECIALIZATION, "Cauterize"              );
  talents.flame_on               = find_talent_spell( talent_tree::SPECIALIZATION, "Flame On"               );
  talents.flame_patch            = find_talent_spell( talent_tree::SPECIALIZATION, "Flame Patch"            );
  // Row 6
  talents.alexstraszas_fury      = find_talent_spell( talent_tree::SPECIALIZATION, "Alexstrasza's Fury"     );
  talents.from_the_ashes         = find_talent_spell( talent_tree::SPECIALIZATION, "From the Ashes"         );
  talents.combustion             = find_talent_spell( talent_tree::SPECIALIZATION, "Combustion"             );
  talents.living_bomb            = find_talent_spell( talent_tree::SPECIALIZATION, "Living Bomb"            );
  talents.incendiary_eruptions   = find_talent_spell( talent_tree::SPECIALIZATION, "Incendiary Eruptions"   );
  // Row 7
  talents.call_of_the_sun_king   = find_talent_spell( talent_tree::SPECIALIZATION, "Call of the Sun King"   );
  talents.firemind               = find_talent_spell( talent_tree::SPECIALIZATION, "Firemind"               );
  talents.improved_combustion    = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Combustion"    );
  talents.tempered_flames        = find_talent_spell( talent_tree::SPECIALIZATION, "Tempered Flames"        );
  talents.feel_the_burn          = find_talent_spell( talent_tree::SPECIALIZATION, "Feel the Burn"          );
  talents.conflagration          = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagration"          );
  // Row 8
  talents.phoenix_reborn         = find_talent_spell( talent_tree::SPECIALIZATION, "Phoenix Reborn"         );
  talents.pyroclasm              = find_talent_spell( talent_tree::SPECIALIZATION, "Pyroclasm"              );
  talents.flame_accelerant       = find_talent_spell( talent_tree::SPECIALIZATION, "Flame Accelerant"       );
  talents.kindling               = find_talent_spell( talent_tree::SPECIALIZATION, "Kindling"               );
  talents.wildfire               = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire"               );
  talents.firefall               = find_talent_spell( talent_tree::SPECIALIZATION, "Firefall"               );
  talents.pyromaniac             = find_talent_spell( talent_tree::SPECIALIZATION, "Pyromaniac"             );
  talents.master_of_flame        = find_talent_spell( talent_tree::SPECIALIZATION, "Master of Flame"        );
  // Row 9
  talents.controlled_destruction = find_talent_spell( talent_tree::SPECIALIZATION, "Controlled Destruction" );
  talents.fevered_incantation    = find_talent_spell( talent_tree::SPECIALIZATION, "Fevered Incantation"    );
  // Row 10
  talents.sun_kings_blessing     = find_talent_spell( talent_tree::SPECIALIZATION, "Sun King's Blessing"    );
  talents.fiery_rush             = find_talent_spell( talent_tree::SPECIALIZATION, "Fiery Rush"             );
  talents.hyperthermia           = find_talent_spell( talent_tree::SPECIALIZATION, "Hyperthermia"           );

  // Frost
  // Row 1
  talents.ice_lance              = find_talent_spell( talent_tree::SPECIALIZATION, "Ice Lance"              );
  // Row 2
  talents.frozen_orb             = find_talent_spell( talent_tree::SPECIALIZATION, "Frozen Orb"             );
  talents.blizzard               = find_talent_spell( talent_tree::SPECIALIZATION, "Blizzard"               );
  talents.fingers_of_frost       = find_talent_spell( talent_tree::SPECIALIZATION, "Fingers of Frost"       );
  // Row 3
  talents.flurry                 = find_talent_spell( talent_tree::SPECIALIZATION, "Flurry"                 );
  talents.shatter                = find_talent_spell( talent_tree::SPECIALIZATION, "Shatter"                );
  // Row 4
  talents.brain_freeze           = find_talent_spell( talent_tree::SPECIALIZATION, "Brain Freeze"           );
  talents.everlasting_frost      = find_talent_spell( talent_tree::SPECIALIZATION, "Everlasting Frost"      );
  talents.cold_snap              = find_talent_spell( talent_tree::SPECIALIZATION, "Cold Snap"              );
  talents.frostbite              = find_talent_spell( talent_tree::SPECIALIZATION, "Frostbite"              );
  talents.piercing_cold          = find_talent_spell( talent_tree::SPECIALIZATION, "Piercing Cold"          );
  // Row 5
  talents.perpetual_winter       = find_talent_spell( talent_tree::SPECIALIZATION, "Perpetual Winter"       );
  talents.lonely_winter          = find_talent_spell( talent_tree::SPECIALIZATION, "Lonely Winter"          );
  talents.summon_water_elemental = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Water Elemental" );
  talents.ice_caller             = find_talent_spell( talent_tree::SPECIALIZATION, "Ice Caller"             );
  talents.bone_chilling          = find_talent_spell( talent_tree::SPECIALIZATION, "Bone Chilling"          );
  // Row 6
  talents.glacial_assault        = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Assault"        );
  talents.ebonbolt               = find_talent_spell( talent_tree::SPECIALIZATION, "Ebonbolt"               );
  talents.frozen_touch           = find_talent_spell( talent_tree::SPECIALIZATION, "Frozen Touch"           );
  talents.wintertide             = find_talent_spell( talent_tree::SPECIALIZATION, "Wintertide"             );
  talents.snowstorm              = find_talent_spell( talent_tree::SPECIALIZATION, "Snowstorm"              );
  talents.flash_freeze           = find_talent_spell( talent_tree::SPECIALIZATION, "Flash Freeze"           );
  talents.subzero                = find_talent_spell( talent_tree::SPECIALIZATION, "Subzero"                );
  // Row 7
  talents.deep_shatter           = find_talent_spell( talent_tree::SPECIALIZATION, "Deep Shatter"           );
  talents.icy_veins              = find_talent_spell( talent_tree::SPECIALIZATION, "Icy Veins"              );
  talents.splintering_cold       = find_talent_spell( talent_tree::SPECIALIZATION, "Splintering Cold"       );
  // Row 8
  talents.comet_storm            = find_talent_spell( talent_tree::SPECIALIZATION, "Comet Storm"            );
  talents.freezing_rain          = find_talent_spell( talent_tree::SPECIALIZATION, "Freezing Rain"          );
  talents.icy_propulsion         = find_talent_spell( talent_tree::SPECIALIZATION, "Icy Propulsion"         );
  talents.ray_of_frost           = find_talent_spell( talent_tree::SPECIALIZATION, "Ray of Frost"           );
  talents.splitting_ice          = find_talent_spell( talent_tree::SPECIALIZATION, "Splitting Ice"          );
  // Row 9
  talents.fractured_frost        = find_talent_spell( talent_tree::SPECIALIZATION, "Fractured Frost"        );
  talents.snap_freeze            = find_talent_spell( talent_tree::SPECIALIZATION, "Snap Freeze"            );
  talents.slick_ice              = find_talent_spell( talent_tree::SPECIALIZATION, "Slick Ice"              );
  talents.chain_reaction         = find_talent_spell( talent_tree::SPECIALIZATION, "Chain Reaction"         );
  talents.hailstones             = find_talent_spell( talent_tree::SPECIALIZATION, "Hailstones"             );
  // Row 10
  talents.cold_front             = find_talent_spell( talent_tree::SPECIALIZATION, "Cold Front"             );
  talents.freezing_winds         = find_talent_spell( talent_tree::SPECIALIZATION, "Freezing Winds"         );
  talents.thermal_void           = find_talent_spell( talent_tree::SPECIALIZATION, "Thermal Void"           );
  talents.glacial_spike          = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Spike"          );

  // Spec Spells
  spec.arcane_charge                 = find_specialization_spell( "Arcane Charge"                 );
  spec.arcane_mage                   = find_specialization_spell( "Arcane Mage"                   );
  spec.fire_mage                     = find_specialization_spell( "Fire Mage"                     );
  spec.pyroblast_clearcasting_driver = find_specialization_spell( "Pyroblast Clearcasting Driver" );
  spec.frost_mage                    = find_specialization_spell( "Frost Mage"                    );

  // Mastery
  spec.savant    = find_mastery_spell( MAGE_ARCANE );
  spec.ignite    = find_mastery_spell( MAGE_FIRE );
  spec.icicles   = find_mastery_spell( MAGE_FROST );
  spec.icicles_2 = find_specialization_spell( "Mastery: Icicles", "Rank 2" );

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

  runeforge.deaths_fathom        = find_runeforge_legendary( "Death's Fathom"       );
  runeforge.disciplinary_command = find_runeforge_legendary( "Disciplinary Command" );
  runeforge.expanded_potential   = find_runeforge_legendary( "Expanded Potential"   );
  runeforge.grisly_icicle        = find_runeforge_legendary( "Grisly Icicle"        );
  runeforge.harmonic_echo        = find_runeforge_legendary( "Harmonic Echo"        );
  runeforge.heart_of_the_fae     = find_runeforge_legendary( "Heart of the Fae"     );
  runeforge.sinful_delight       = find_runeforge_legendary( "Sinful Delight"       );

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
  buffs.arcane_charge        = make_buff( this, "arcane_charge", find_spell( 36032 ) )
                                 ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.arcane_familiar      = make_buff( this, "arcane_familiar", find_spell( 210126 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_period( 3.0_s )
                                 ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
                                 ->set_tick_callback( [ this ] ( buff_t*, int, timespan_t )
                                   { action.arcane_assault->execute_on_target( target ); } )
                                 ->set_stack_change_callback( [ this ] ( buff_t*, int, int )
                                   { recalculate_resource_max( RESOURCE_MANA ); } );
  buffs.arcane_harmony       = make_buff( this, "arcane_harmony", talents.arcane_harmony.ok() ? find_spell( 384455 ) : find_spell( 332777 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_chance( talents.arcane_harmony.ok() || runeforge.arcane_harmony.ok() )
                                 ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.arcane_surge         = make_buff( this, "arcane_surge", find_spell( 365362 ) )
                                 ->set_default_value_from_effect( 3 )
                                 ->set_affects_regen( true )
                                 ->modify_duration( talents.arcane_power->effectN( 1 ).time_value() );
  buffs.arcane_tempo         = make_buff( this, "arcane_tempo", find_spell( 383997 ) )
                                 ->set_default_value( talents.arcane_tempo->effectN( 1 ).percent() )
                                 ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                 ->set_chance( talents.arcane_tempo.ok() );
  buffs.chrono_shift         = make_buff( this, "chrono_shift", find_spell( 236298 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->add_invalidate( CACHE_RUN_SPEED )
                                 ->set_chance( talents.chrono_shift.ok() );
  buffs.clearcasting         = make_buff<buffs::clearcasting_buff_t>( this );
  buffs.clearcasting_channel = make_buff( this, "clearcasting_channel", find_spell( 277726 ) )
                                 ->set_quiet( true );
  buffs.concentration        = make_buff( this, "concentration", find_spell( 384379 ) )
                                 ->set_activated( false )
                                 ->set_trigger_spell( talents.concentration );
  buffs.enlightened_damage   = make_buff( this, "enlightened_damage", find_spell( 321388 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.enlightened_mana     = make_buff( this, "enlightened_mana", find_spell( 321390 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_affects_regen( true );
  buffs.evocation            = make_buff( this, "evocation", find_spell( 12051 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_cooldown( 0_ms )
                                 ->set_affects_regen( true );
  buffs.foresight            = make_buff( this, "foresight", find_spell( 384865 ) )
                                 ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                   { if ( cur == 0 && player_t::buffs.movement->check() ) moving(); } )
                                 ->set_chance( talents.foresight.ok() );
  buffs.foresight_icd        = make_buff( this, "foresight_icd", find_spell( 384863 ) )
                                 ->set_can_cancel( false )
                                 ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                   { if ( cur == 0 ) buffs.foresight->trigger(); } );
  buffs.impetus              = make_buff( this, "impetus", find_spell( 393939 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.invigorating_powder  = make_buff( this, "invigorating_powder", find_spell( 384280 ) )
                                 ->set_default_value_from_effect( 1 );
  buffs.nether_precision     = make_buff( this, "nether_precision", find_spell( 383783 ) )
                                 ->set_default_value( talents.nether_precision->effectN( 1 ).percent() )
                                 ->set_chance( talents.nether_precision.ok() );
  buffs.orb_barrage          = make_buff( this, "orb_barrage", find_spell( 384859 ) )
                                 ->set_chance( talents.orb_barrage.ok() );
  buffs.orb_barrage_ready    = make_buff( this, "orb_barrage_ready", find_spell( 384860 ) );
  buffs.presence_of_mind     = make_buff( this, "presence_of_mind", find_spell( 205025 ) )
                                 ->set_cooldown( 0_ms )
                                 ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                   { if ( cur == 0 ) cooldowns.presence_of_mind->start( cooldowns.presence_of_mind->action ); } );
  buffs.rule_of_threes       = make_buff( this, "rule_of_threes", find_spell( 264774 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_chance( talents.rule_of_threes.ok() );
  buffs.siphon_storm         = make_buff( this, "siphon_storm", find_spell( talents.siphon_storm.ok() ? 384267 : 332934 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                                 ->set_chance( talents.siphon_storm.ok() || runeforge.siphon_storm.ok() );


  // Fire
  buffs.combustion               = make_buff<buffs::combustion_t>( this );
  buffs.feel_the_burn            = make_buff( this, "feel_the_burn", find_spell( 383395 ) )
                                     ->set_default_value( talents.feel_the_burn->effectN( 1 ).base_value() )
                                     ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
                                     ->set_chance( talents.feel_the_burn.ok() );
  buffs.fevered_incantation      = make_buff( this, "fevered_incantation", find_spell( 383811 ) )
                                     ->set_default_value( talents.fevered_incantation->effectN( 1 ).percent() )
                                     ->set_schools_from_effect( 1 )
                                     ->set_chance( talents.fevered_incantation.ok() );
  buffs.fiery_rush               = make_buff( this, "fiery_rush", find_spell( 383637 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_stack_change_callback( [ this ] ( buff_t*, int, int )
                                       {
                                         cooldowns.fire_blast->adjust_recharge_multiplier();
                                         cooldowns.phoenix_flames->adjust_recharge_multiplier();
                                       } )
                                     ->set_chance( talents.fiery_rush.ok() );
  buffs.firefall                 = make_buff( this, "firefall", find_spell( 384035 ) )
                                     ->set_chance( talents.firefall.ok() );
  buffs.firefall_ready           = make_buff( this, "firefall_ready", find_spell( 384038 ) );
  // TODO: 2022-10-02 Firemind currently affects base intellect and not current intellect.
  // This needs to be reimplemented if that bug is not fixed.
  buffs.firemind                 = make_buff( this, "firemind", find_spell( 383501 ) )
                                     ->set_default_value( talents.firemind->effectN( 3 ).percent() )
                                     ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                                     ->set_chance( talents.firemind.ok() );
  // TODO: 2022-10-02 Flame Accelerant has several bugs that are not implemented in simc.
  // 1. Casting a Fireball as the ICD is expiring can result in that Fireball gaining the
  // bonus damage without stopping the flame_accelerant buff from being applied just after.
  // 2. Fireball's base cast time without flame_accelerant is 4% too slow with 1 point of
  // the talent and is 22% too fast with 2 points of the talent.
  buffs.flame_accelerant         = make_buff( this, "flame_accelerant", find_spell( 203277 ) )
                                     ->set_can_cancel( false )
                                     ->set_chance( talents.flame_accelerant.ok() )
                                     ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.flame_accelerant_icd     = make_buff( this, "flame_accelerant_icd", find_spell( 203278 ) )
                                     ->set_can_cancel( false )
                                     ->set_quiet( true )
                                     ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                       {
                                         if ( cur == 0 )
                                           buffs.flame_accelerant->trigger();
                                         else
                                           buffs.flame_accelerant->expire();
                                      } );
  buffs.heating_up               = make_buff( this, "heating_up", find_spell( 48107 ) );
  buffs.hot_streak               = make_buff<buffs::expanded_potential_buff_t>( this, "hot_streak", find_spell( 48108 ) )
                                     ->set_stack_change_callback( [ this ] ( buff_t*, int old, int )
                                       {
                                         if ( old == 0 )
                                         {
                                           buffs.hyperthermia->trigger();
                                           if ( !talents.hyperthermia.ok() )
                                             buffs.firestorm->trigger();
                                         }
                                       } );
  buffs.hyperthermia             = make_buff( this, "hyperthermia", find_spell( 383874 ) )
                                    ->set_default_value_from_effect( 2 )
                                    ->set_trigger_spell( talents.hyperthermia );
  buffs.pyroclasm                = make_buff( this, "pyroclasm", find_spell( 269651 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_chance( talents.pyroclasm->effectN( 1 ).percent() );
  buffs.pyrotechnics             = make_buff( this, "pyrotechnics", find_spell( 157644 ) )
                                     ->set_chance( talents.pyrotechnics.ok() )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_stack_change_callback( [ this ] ( buff_t*, int old, int cur )
                                       {
                                         if ( cur > old )
                                           buffs.flame_accretion->trigger( cur - old );
                                         else
                                           buffs.flame_accretion->decrement( old - cur );
                                       } );
  buffs.sun_kings_blessing       = make_buff( this, "sun_kings_blessing", find_spell( 333314 ) )
                                     ->set_chance( talents.sun_kings_blessing.ok() );
  buffs.sun_kings_blessing_ready = make_buff( this, "sun_kings_blessing_ready", find_spell( 333315 ) );
  buffs.wildfire                 = make_buff( this, "wildfire", find_spell( 383492 ) )
                                     ->set_default_value( talents.wildfire->effectN( 3 ).percent() )
                                     ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                                     ->set_chance( talents.wildfire.ok() );


  // Frost
  buffs.bone_chilling    = make_buff( this, "bone_chilling", find_spell( 205766 ) )
                             ->set_default_value( 0.1 * talents.bone_chilling->effectN( 1 ).percent() )
                             ->set_chance( talents.bone_chilling.ok() );
  buffs.chain_reaction   = make_buff( this, "chain_reaction", find_spell( 278310 ) )
                             ->set_default_value( talents.chain_reaction->effectN( 1 ).percent() )
                             ->set_chance( talents.chain_reaction.ok() );
  buffs.brain_freeze     = make_buff<buffs::expanded_potential_buff_t>( this, "brain_freeze", find_spell( 190446 ) );
  // TODO: some of the talent + legendary interactions are currently bugged
  // casting Frozen Orb gives 2 different Freezing Winds buffs, casting Frostbolt during Icy Veins gives 2 different Slick Ice buffs
  buffs.cold_front       = make_buff( this, "cold_front", find_spell( talents.cold_front.ok() ? 382113 : 327327 ) )
                             ->set_chance( talents.cold_front.ok() || runeforge.cold_front.ok() );
  buffs.cold_front_ready = make_buff( this, "cold_front_ready", find_spell( talents.cold_front.ok() ? 382114 : 327330 ) );
  buffs.fingers_of_frost = make_buff( this, "fingers_of_frost", find_spell( 44544 ) );
  buffs.freezing_rain    = make_buff( this, "freezing_rain", find_spell( 270232 ) )
                             ->set_default_value_from_effect( 2 )
                             ->set_chance( talents.freezing_rain.ok() );
  buffs.freezing_winds   = make_buff( this, "freezing_winds", find_spell( talents.freezing_winds.ok() ? 382106 : 327478 ) )
                             ->set_tick_callback( [ this ] ( buff_t*, int, timespan_t )
                               { trigger_fof( 1.0, procs.fingers_of_frost_freezing_winds ); } )
                             ->set_chance( talents.freezing_winds.ok() || runeforge.freezing_winds.ok() );
  buffs.icicles          = make_buff( this, "icicles", find_spell( 205473 ) );
  buffs.icy_veins        = make_buff<buffs::icy_veins_t>( this );
  buffs.ray_of_frost     = make_buff( this, "ray_of_frost", find_spell( 208141 ) )
                             ->set_default_value_from_effect( 1 );
  buffs.slick_ice        = make_buff( this, "slick_ice", find_spell( talents.slick_ice.ok() ? 382148 : 327509 ) )
                             ->set_default_value_from_effect( 1 )
                             ->set_chance( talents.slick_ice.ok() || runeforge.slick_ice.ok() );
  buffs.snowstorm        = make_buff( this, "snowstorm", find_spell( 381522 ) )
                             ->set_default_value( talents.snowstorm->effectN( 2 ).percent() );


  // Shared
  buffs.ice_floes          = make_buff<buffs::ice_floes_t>( this );
  buffs.incanters_flow     = make_buff<buffs::incanters_flow_t>( this );
  buffs.overflowing_energy = make_buff( this, "overflowing_energy", find_spell( 394195 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_chance( talents.overflowing_energy.ok() );
  buffs.rune_of_power      = make_buff<buffs::rune_of_power_t>( this );
  buffs.temporal_warp      = make_buff( this, "temporal_warp", find_spell( talents.temporal_warp.ok() ? 386540 : 327355 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                               ->set_chance( talents.temporal_warp.ok() || runeforge.temporal_warp.ok() );
  buffs.time_warp          = make_buff( this, "time_warp", find_spell( 342242 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );


  // Runeforge Legendaries
  buffs.runeforge_fevered_incantation      = make_buff( this, "runeforge_fevered_incantation", find_spell( 333049 ) )
                                               ->set_default_value_from_effect( 1 )
                                               ->set_chance( runeforge.fevered_incantation.ok() && !talents.fevered_incantation.ok() );
  buffs.firestorm                          = make_buff( this, "firestorm", find_spell( 333100 ) )
                                               ->set_default_value_from_effect( 2 )
                                               ->set_trigger_spell( runeforge.firestorm );
  buffs.molten_skyfall                     = make_buff( this, "molten_skyfall", find_spell( 333170 ) )
                                               ->set_chance( runeforge.molten_skyfall.ok() && !talents.firefall.ok() );
  buffs.molten_skyfall_ready               = make_buff( this, "molten_skyfall_ready", find_spell( 333182 ) );
  buffs.runeforge_sun_kings_blessing       = make_buff( this, "runeforge_sun_kings_blessing", find_spell( 333314 ) )
                                               ->set_chance( runeforge.sun_kings_blessing.ok() && !talents.sun_kings_blessing.ok() );
  buffs.runeforge_sun_kings_blessing_ready = make_buff( this, "runeforge_sun_kings_blessing_ready", find_spell( 333315 ) );

  buffs.disciplinary_command        = make_buff( this, "disciplinary_command", find_spell( 327371 ) )
                                        ->set_default_value_from_effect( 1 );
  buffs.disciplinary_command_arcane = make_buff( this, "disciplinary_command_arcane", find_spell( 327369 ) )
                                        ->set_quiet( true )
                                        ->set_chance( runeforge.disciplinary_command.ok() );
  buffs.disciplinary_command_frost  = make_buff( this, "disciplinary_command_frost", find_spell( 327366 ) )
                                        ->set_quiet( true )
                                        ->set_chance( runeforge.disciplinary_command.ok() );
  buffs.disciplinary_command_fire   = make_buff( this, "disciplinary_command_fire", find_spell( 327368 ) )
                                        ->set_quiet( true )
                                        ->set_chance( runeforge.disciplinary_command.ok() );
  buffs.expanded_potential          = make_buff( this, "expanded_potential", find_spell( 327495 ) )
                                        ->set_activated( false )
                                        ->set_trigger_spell( runeforge.expanded_potential );
  buffs.heart_of_the_fae            = make_buff( this, "heart_of_the_fae", find_spell( 356881 ) )
                                        ->set_default_value_from_effect( 1 )
                                        ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                                        ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                        ->set_chance( runeforge.heart_of_the_fae.ok() );


  // Covenant Abilities
  buffs.deathborne = make_buff( this, "deathborne", find_spell( 324220 ) )
                       ->set_cooldown( 0_ms )
                       ->set_default_value_from_effect( 2 )
                       ->set_chance( find_covenant_spell( "Deathborne" )->ok() )
                       ->modify_duration( conduits.gift_of_the_lich.time_value() );


  // Soulbind Conduits
  buffs.nether_precision_conduit = make_buff( this, "nether_precision_conduit", find_spell( 336889 ) )
                                     ->set_default_value( conduits.nether_precision.percent() )
                                     ->set_chance( conduits.nether_precision.ok() && !talents.nether_precision.ok() );

  buffs.flame_accretion  = make_buff( this, "flame_accretion", find_spell( 157644 ) )
                             ->set_default_value( conduits.flame_accretion.value() )
                             ->set_chance( conduits.flame_accretion.ok() )
                             ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
  buffs.infernal_cascade = make_buff( this, "infernal_cascade", find_spell( 336832 ) )
                             ->set_default_value( conduits.infernal_cascade.percent() )
                             ->set_schools_from_effect( 1 )
                             ->set_chance( conduits.infernal_cascade.ok() )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                             ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                               { if ( cur == 0 && buffs.combustion->check() ) procs.infernal_cascade_expires->occur(); } );

  buffs.siphoned_malice = make_buff( this, "siphoned_malice", find_spell( 337090 ) )
                            ->set_default_value( conduits.siphoned_malice.percent() )
                            ->set_chance( conduits.siphoned_malice.ok() );


  // Set Bonuses
  buffs.bursting_energy = make_buff( this, "bursting_energy", find_spell( 395006 ) )
                            ->set_default_value_from_effect( 1 )
                            ->set_chance( sets->has_set_bonus( MAGE_ARCANE, T29, B4 ) );

  buffs.touch_of_ice = make_buff( this, "touch_of_ice", find_spell( 394994 ) )
                         ->set_default_value_from_effect( 1 )
                         ->set_chance( sets->has_set_bonus( MAGE_FROST, T29, B4 ) );


  // Foresight support
  if ( talents.foresight.ok() )
  {
    assert( !player_t::buffs.movement->stack_change_callback );
    player_t::buffs.movement->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
    {
      if ( cur == 0 )
      {
        buffs.foresight_icd->trigger();
      }
      else
      {
        buffs.foresight_icd->trigger( 0_ms );
        buffs.foresight->expire( buffs.foresight->data().effectN( 2 ).time_value() );
      }
    } );
  }
}

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.arcane_surge   = get_gain( "Arcane Surge"   );
  gains.mana_gem       = get_gain( "Mana Gem"       );
  gains.arcane_barrage = get_gain( "Arcane Barrage" );
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

      procs.infernal_cascade_expires = get_proc( "Infernal Cascade expires during Combustion" );
      break;
    case MAGE_FROST:
      procs.brain_freeze                    = get_proc( "Brain Freeze" );
      procs.brain_freeze_mirrors            = get_proc( "Brain Freeze from Mirrors of Torment" );
      procs.brain_freeze_snap_freeze        = get_proc( "Brain Freeze from Snap Freeze" );
      procs.brain_freeze_water_jet          = get_proc( "Brain Freeze from Water Jet" );
      procs.fingers_of_frost                = get_proc( "Fingers of Frost" );
      procs.fingers_of_frost_flash_freeze   = get_proc( "Fingers of Frost from Flash Freeze" );
      procs.fingers_of_frost_freezing_winds = get_proc( "Fingers of Frost from Freezing Winds" );
      procs.fingers_of_frost_snap_freeze    = get_proc( "Fingers of Frost from Snap Freeze" );
      procs.fingers_of_frost_time_anomaly   = get_proc( "Fingers of Frost from Time Anomaly" );
      procs.fingers_of_frost_wasted         = get_proc( "Fingers of Frost wasted due to Winter's Chill" );
      procs.flurry_cast                     = get_proc( "Flurry cast" );
      procs.winters_chill_applied           = get_proc( "Winter's Chill stacks applied" );
      procs.winters_chill_consumed          = get_proc( "Winter's Chill stacks consumed" );
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
      if ( talents.nether_tempest.ok() )
        benefits.arcane_charge.nether_tempest = std::make_unique<buff_stack_benefit_t>( buffs.arcane_charge, "Nether Tempest" );
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
      if ( talents.thermal_void.ok() )
        sample_data.icy_veins_duration = std::make_unique<extended_sample_data_t>( "Icy Veins duration", false );
      break;
    default:
      break;
  }
}

void mage_t::init_rng()
{
  player_t::init_rng();

  rppm.deaths_fathom = get_rppm( "deaths_fathom", runeforge.deaths_fathom, runeforge.deaths_fathom.item() );

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

  buffs.lead_by_example = buff_t::find( this, "lead_by_example" );
}

void mage_t::add_precombat_buff_state( buff_t* buff, int stacks, double value, timespan_t duration )
{
  if ( buff == buffs.icicles )
  {
    if ( stacks < 0 )
      stacks = 1;

    int max_icicles = as<int>( spec.icicles->effectN( 2 ).base_value() );
    register_precombat_begin( [ this, stacks, duration, max_icicles ] ( player_t* )
    {
      int new_icicles = std::min( stacks, max_icicles ) - buffs.icicles->check();
      for ( int i = 0; i < new_icicles; i++ )
        trigger_icicle_gain( target, action.icicle.frostbolt, duration );
    });

    return;
  }

  player_t::add_precombat_buff_state( buff, stacks, value, duration );
}

void mage_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
      case MAGE_ARCANE:
        mage_apl::arcane( this );
        break;
      case MAGE_FIRE:
        mage_apl::fire( this );
        break;
      case MAGE_FROST:
        mage_apl::frost( this );
        break;
      default:
        break;
    }

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

double mage_t::resource_regen_per_second( resource_e rt ) const
{
  double reg = player_t::resource_regen_per_second( rt );

  if ( specialization() == MAGE_ARCANE && rt == RESOURCE_MANA )
  {
    reg *= 1.0 + 0.01 * spec.arcane_mage->effectN( 4 ).average( this );
    reg *= 1.0 + cache.mastery() * spec.savant->effectN( 1 ).mastery_value();
    reg *= 1.0 + buffs.enlightened_mana->check_value();
    reg *= 1.0 + buffs.evocation->check_value();
  }

  return reg;
}

void mage_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_MASTERY && spec.savant->ok() )
    recalculate_resource_max( RESOURCE_MANA );
}

void mage_t::do_dynamic_regen( bool forced )
{
  player_t::do_dynamic_regen( forced );

  // Only update Enlightened buffs on resource updates that actually occur in game.
  if ( forced && talents.enlightened.ok() )
    make_event( *sim, [ this ] { update_enlightened(); } );
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

  school_e school = s->action->get_school();

  if ( buffs.fevered_incantation->has_common_school( school ) )
    m *= 1.0 + buffs.fevered_incantation->check_stack_value();
  if ( buffs.runeforge_fevered_incantation->has_common_school( school ) )
    m *= 1.0 + buffs.runeforge_fevered_incantation->check_stack_value();
  if ( buffs.disciplinary_command->has_common_school( school ) )
    m *= 1.0 + buffs.disciplinary_command->check_value();

  return m;
}

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.enlightened_damage->has_common_school( school ) )
    m *= 1.0 + buffs.enlightened_damage->check_value();
  if ( buffs.impetus->has_common_school( school ) )
    m *= 1.0 + buffs.impetus->check_value();
  if ( buffs.infernal_cascade->has_common_school( school ) )
    m *= 1.0 + buffs.infernal_cascade->check_stack_value();

  return m;
}

double mage_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

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

  if ( auto td = find_target_data( target ) )
  {
    if ( td->debuffs.grisly_icicle->has_common_school( school ) )
      m *= 1.0 + td->debuffs.grisly_icicle->check_value();

    if ( td->debuffs.improved_scorch->has_common_school( school ) )
      m *= 1.0 + td->debuffs.improved_scorch->check_stack_value();
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
      rm *= 1.0 + talents.critical_mass->effectN( 2 ).percent();
      break;
    default:
      break;
  }

  return rm;
}

double mage_t::composite_melee_crit_chance() const
{
  double c = player_t::composite_melee_crit_chance();

  c += talents.tome_of_rhonin->effectN( 1 ).percent();

  return c;
}

double mage_t::composite_spell_crit_chance() const
{
  double c = player_t::composite_spell_crit_chance();

  c += talents.tome_of_rhonin->effectN( 1 ).percent();
  c += talents.critical_mass->effectN( 1 ).percent();

  return c;
}

double mage_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  h /= 1.0 + talents.tome_of_antonidas->effectN( 1 ).percent();

  return h;
}

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  h /= 1.0 + talents.tome_of_antonidas->effectN( 1 ).percent();

  return h;
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
  burn_phase.reset();
  ground_aoe_expiration = std::array<timespan_t, AOE_MAX>();
  state = state_t();
  expression_support = expression_support_t();
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

  return pmm;
}

void mage_t::arise()
{
  player_t::arise();

  buffs.flame_accelerant->trigger();
  buffs.foresight->trigger();
  buffs.incanters_flow->trigger();

  if ( talents.enlightened.ok() )
  {
    update_enlightened();

    timespan_t first_tick = rng().real() * 2.0_s;
    events.enlightened = make_event<events::enlightened_event_t>( *sim, *this, first_tick );
  }

  if ( talents.from_the_ashes.ok() )
  {
    update_from_the_ashes();

    timespan_t first_tick = rng().real() * 2.0_s;
    events.from_the_ashes = make_event<events::from_the_ashes_event_t>( *sim, *this, first_tick );
  }

  if ( talents.time_anomaly.ok() )
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
    // When combat starts, any Arcane Charge stacks above one are removed.
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
std::unique_ptr<expr_t> mage_t::create_action_expression( action_t& action, std::string_view name )
{
  auto splits = util::string_split<std::string_view>( name, "." );

  // Firestarter expressions ==================================================
  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "firestarter" ) )
  {
    double firestarter_pct = 100.0 - ( 100.0 - talents.firestarter->effectN( 1 ).base_value() ) * options.firestarter_duration_multiplier;

    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      if ( !talents.firestarter.ok() )
        return expr_t::create_constant( name_str, false );

      return make_fn_expr( name_str, [ &action, firestarter_pct ]
      { return action.target->health_percentage() > firestarter_pct; } );
    }

    if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      if ( !talents.firestarter.ok() )
        return expr_t::create_constant( name_str, 0.0 );

      return make_fn_expr( name_str, [ &action, firestarter_pct ]
      { return action.target->time_to_percent( firestarter_pct ).total_seconds(); } );
    }

    throw std::invalid_argument( fmt::format( "Unknown firestarer operation '{}'", splits[ 1 ] ) );
  }

  // Searing Touch expressions ==================================================
  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "searing_touch" ) )
  {
    double searing_touch_pct = talents.searing_touch->effectN( 1 ).base_value() * options.searing_touch_duration_multiplier;

    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      if ( !talents.searing_touch.ok() )
        return expr_t::create_constant( name_str, false );

      return make_fn_expr( name_str, [ &action, searing_touch_pct ]
      { return action.target->health_percentage() < searing_touch_pct; } );
    }

    if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      if ( !talents.searing_touch.ok() )
        return expr_t::create_constant( name_str, std::numeric_limits<double>::max() );

      return make_fn_expr( name_str, [ &action, searing_touch_pct ]
      { return action.target->time_to_percent( searing_touch_pct ).total_seconds(); } );
    }

    throw std::invalid_argument( fmt::format( "Unknown searing_touch operation '{}'", splits[ 1 ] ) );
  }

  return player_t::create_action_expression( action, name );
}

std::unique_ptr<expr_t> mage_t::create_expression( std::string_view name )
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
      if ( !talents.incanters_flow.ok() )
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
    { return expression_support.remaining_winters_chill; } );
  }

  if ( util::str_compare_ci( name, "comet_storm_remains" ) )
  {
    std::vector<action_t*> in_flight_list;
    for ( auto a : action_list )
    {
      if ( a->id == 153595 )
        in_flight_list.push_back( a );
    }

    return make_fn_expr( name, [ this, in_flight_list ]
    {
      timespan_t remains = 0_ms;

      if ( ground_aoe_expiration[ AOE_COMET_STORM ] > sim->current_time() )
        remains = ground_aoe_expiration[ AOE_COMET_STORM ] - sim->current_time();

      for ( auto a : in_flight_list )
        if ( a->has_travel_events() )
          remains = std::max( remains, a->shortest_travel_event() + actions::comet_storm_t::pulse_count * actions::comet_storm_t::pulse_time );

      return remains.total_seconds();
    } );
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
      if ( is_hss( a ) || range::any_of( a->child_action, is_hss ) )
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

  if ( util::str_compare_ci( name, "expected_kindling_reduction" ) )
  {
    return make_fn_expr( name, [ this ]
    {
      if ( !talents.kindling.ok() || cooldowns.combustion->last_start < 0_ms )
        return 1.0;

      timespan_t t = sim->current_time() - cooldowns.combustion->last_start - buffs.combustion->buff_duration();
      if ( t <= 0_ms )
        return 1.0;

      return t / ( t + expression_support.kindling_reduction );
    } );
  }

  auto splits = util::string_split<std::string_view>( name, "." );

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

  // Time remaining until the specified Incanter's Flow stack.
  // Format: incanters_flow_time_to.<stack>.<type> where
  // type can be one of "up", "down", or "any".
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
      if ( !talents.incanters_flow.ok() || !buffs.incanters_flow->tick_event )
        return 0.0;

      int buff_stack = buffs.incanters_flow->check();
      // Current position in the stack cycle.
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

void mage_t::update_rune_distance( double distance )
{
  state.distance_from_rune += distance;

  if ( buffs.rune_of_power->check() && state.distance_from_rune > talents.rune_of_power->effectN( 2 ).radius() )
  {
    buffs.rune_of_power->expire();
    sim->print_debug( "{} moved out of Rune of Power.", name() );
  }
}

void mage_t::update_enlightened( bool double_regen )
{
  if ( !talents.enlightened.ok() )
    return;

  bool damage_buff = resources.pct( RESOURCE_MANA ) > talents.enlightened->effectN( 1 ).percent();
  if ( damage_buff && !buffs.enlightened_damage->check() )
  {
    // Periodic mana regen happens twice whenever the mana regen buff from Enlightened expires.
    if ( bugs && double_regen && sim->current_time() > state.last_enlightened_update )
      regen( sim->current_time() - state.last_enlightened_update );

    buffs.enlightened_damage->trigger();
    buffs.enlightened_mana->expire();
  }
  else if ( !damage_buff && !buffs.enlightened_mana->check() )
  {
    buffs.enlightened_damage->expire();
    buffs.enlightened_mana->trigger();
  }

  state.last_enlightened_update = sim->current_time();
}

void mage_t::update_from_the_ashes()
{
  if ( !talents.from_the_ashes.ok() )
    return;

  state.from_the_ashes_mastery = talents.from_the_ashes->effectN( 3 ).base_value() * static_cast<int>( cooldowns.phoenix_flames->charges_fractional() );
  invalidate_cache( CACHE_MASTERY );

  sim->print_debug( "{} updates mastery from From the Ashes, new value: {}", name_str, state.from_the_ashes_mastery );
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

// Triggers a buff. If the buff was already active, the new application
// is delayed by the specified amount.
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

bool mage_t::trigger_brain_freeze( double chance, proc_t* source, timespan_t delay )
{
  assert( source );

  bool success = rng().roll( chance );
  if ( success )
  {
    if ( buffs.brain_freeze->check() )
    {
      make_event( sim, delay, [ this, chance ]
      {
        buffs.brain_freeze->execute();
        cooldowns.flurry->reset( chance < 1.0 );
      } );
    }
    else
    {
      buffs.brain_freeze->execute();
      cooldowns.flurry->reset( chance < 1.0 );
    }

    source->occur();
    procs.brain_freeze->occur();
  }

  return success;
}

bool mage_t::trigger_fof( double chance, proc_t* source, int stacks )
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

  return success;
}

void mage_t::trigger_icicle( player_t* icicle_target, bool chain )
{
  assert( icicle_target );

  if ( !spec.icicles->ok() )
    return;

  // If Icicles are already being launched, don't start a new chain.
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

    icicle_action->execute_on_target( icicle_target );
    sim->print_debug( "{} icicle use on {}, total={}", name(), icicle_target->name(), icicles.size() );
  }
}

void mage_t::trigger_icicle_gain( player_t* icicle_target, action_t* icicle_action, timespan_t duration )
{
  if ( !spec.icicles->ok() )
    return;

  unsigned max_icicles = as<unsigned>( spec.icicles->effectN( 2 ).base_value() );

  // Shoot one if capped
  if ( icicles.size() == max_icicles )
    trigger_icicle( icicle_target );

  buffs.icicles->trigger( duration );
  icicles.push_back( { icicle_action, make_event( sim, buffs.icicles->remains(), [ this ]
  {
    buffs.icicles->decrement();
    icicles.erase( icicles.begin() );
  } ) } );

  assert( icicle_action && icicles.size() <= max_icicles );
}

void mage_t::trigger_arcane_charge( int stacks )
{
  if ( !spec.arcane_charge->ok() || stacks <= 0 )
    return;

  int before = buffs.arcane_charge->check();
  buffs.arcane_charge->trigger( stacks );

  if ( before < 3 && buffs.arcane_charge->check() >= 3 )
    buffs.rule_of_threes->trigger();
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

    for ( const auto& data : p.cooldown_waste_data_list )
    {
      if ( !data->active() )
        continue;

      std::string name = data->cd->name_str;
      if ( action_t* a = p.find_action( name ) )
        name = report_decorators::decorated_action( *a );
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

    double flurry = p.procs.flurry_cast->count.pretty_mean();

    for ( const shatter_source_t* data : p.shatter_source_list )
    {
      if ( !data->active() )
        continue;

      auto nonzero = [] ( double d, std::string_view suffix ) { return d != 0.0 ? fmt::format( "{:.1f}{}", d, suffix ) : ""; };
      auto cells = [ &, total = data->count_total() ] ( double mean, bool util = false )
      {
        fmt::print( os, "<td class=\"right\">{}</td>", nonzero( mean, "" ) );
        fmt::print( os, "<td class=\"right\">{}</td>", nonzero( 100.0 * mean / total, "%" ) );
        if ( util ) fmt::print( os, "<td class=\"right\">{}</td>", nonzero( flurry > 0.0 ? 100.0 * mean / flurry : 0.0, "%" ) );
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
        if ( p.talents.thermal_void.ok() )
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

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
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

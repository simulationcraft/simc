// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
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

enum trigger_override_e
{
  TO_DEFAULT,
  TO_ALWAYS,
  TO_NEVER
};

enum hot_streak_trigger_type_e
{
  HS_HIT,
  HS_CRIT,
  HS_CUSTOM
};

enum class ae_type
{
  NORMAL,
  ECHO,
  ENERGY_RECON
};

enum class ao_type
{
  NORMAL,
  ORB_BARRAGE,
  SPELLFROST
};

enum class meteor_type
{
  NORMAL,
  FIREFALL,
  ISOTHERMIC
};

enum class arcane_phoenix_rotation
{
  DEFAULT,
  ST,
  AOE
};

struct icicle_tuple_t
{
  action_t* action; // Icicle action corresponding to the source action
  event_t*  expiration;
};

struct buff_adjust_info_t
{
  buff_t* buff;
  bool expire;
  int stacks;
};

struct mage_td_t final : public actor_target_data_t
{
  struct debuffs_t
  {
    buff_t* arcane_debilitation;
    buff_t* controlled_destruction;
    buff_t* controlled_instincts;
    buff_t* frozen;
    buff_t* improved_scorch;
    buff_t* magis_spark;
    buff_t* magis_spark_ab;
    buff_t* magis_spark_abar;
    buff_t* magis_spark_am;
    buff_t* molten_fury;
    buff_t* nether_munitions;
    buff_t* numbing_blast;
    buff_t* touch_of_the_magi;
    buff_t* winters_chill;
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

  // Buffs waiting to be triggered/expired
  std::vector<buff_adjust_info_t> buff_queue;

  // Time Manipulation
  std::vector<cooldown_t*> time_manipulation_cooldowns;

  // Splinters
  std::vector<dot_t*> embedded_splinters;

  // Mana Cascade expiration events
  std::vector<event_t*> mana_cascade_expiration;

  // Events
  struct events_t
  {
    event_t* enlightened;
    event_t* flame_accelerant;
    event_t* icicle;
    event_t* merged_buff_execute;
    event_t* meteor_burn;
    event_t* splinterstorm;
    event_t* time_anomaly;
  } events;

  // Ground AoE tracking
  std::array<timespan_t, AOE_MAX> ground_aoe_expiration;

  // Winter's Chill tracking
  std::vector<action_t*> winters_chill_consumers;

  // Data collection
  auto_dispose<std::vector<shatter_source_t*> > shatter_source_list;

  // Cached actions
  struct actions_t
  {
    action_t* arcane_assault;
    action_t* arcane_echo;
    action_t* arcane_explosion_energy_recon;
    action_t* cold_front_frozen_orb;
    action_t* dematerialize;
    action_t* excess_ice_nova;
    action_t* excess_living_bomb;
    action_t* firefall_meteor;
    action_t* frostfire_empowerment;
    action_t* frostfire_infusion;
    action_t* glacial_assault;
    action_t* ignite;
    action_t* isothermic_comet_storm;
    action_t* isothermic_meteor;
    action_t* leydrinker_echo;
    action_t* living_bomb;
    action_t* magis_spark;
    action_t* magis_spark_echo;
    action_t* meteorite;
    action_t* pet_freeze;
    action_t* pet_water_jet;
    action_t* spellfrost_arcane_orb;
    action_t* splinter;
    action_t* splinter_dot;
    action_t* splinter_recall;
    action_t* splinterstorm;
    action_t* touch_of_the_magi_explosion;
    action_t* volatile_magic;

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
    } arcane_charge;
  } benefits;

  // Buffs
  struct buffs_t
  {
    // Arcane
    buff_t* aether_attunement;
    buff_t* aether_attunement_counter;
    buff_t* arcane_charge;
    buff_t* arcane_familiar;
    buff_t* arcane_harmony;
    buff_t* arcane_surge;
    buff_t* arcane_tempo;
    buff_t* big_brained;
    buff_t* clearcasting;
    buff_t* clearcasting_channel; // Hidden buff which governs tick and channel time
    buff_t* concentration;
    buff_t* enlightened_damage;
    buff_t* enlightened_mana;
    buff_t* evocation;
    buff_t* high_voltage;
    buff_t* impetus;
    buff_t* leydrinker;
    buff_t* nether_precision;
    buff_t* presence_of_mind;
    buff_t* siphon_storm;
    buff_t* static_cloud;


    // Fire
    buff_t* calefaction;
    buff_t* combustion;
    buff_t* feel_the_burn;
    buff_t* fevered_incantation;
    buff_t* fiery_rush;
    buff_t* firefall;
    buff_t* firefall_ready;
    buff_t* flame_accelerant;
    buff_t* flames_fury;
    buff_t* frenetic_speed;
    buff_t* fury_of_the_sun_king;
    buff_t* heat_shimmer;
    buff_t* heating_up;
    buff_t* hot_streak;
    buff_t* hyperthermia;
    buff_t* lit_fuse;
    buff_t* majesty_of_the_phoenix;
    buff_t* pyrotechnics;
    buff_t* sparking_cinders;
    buff_t* sun_kings_blessing;
    buff_t* wildfire;


    // Frost
    buff_t* bone_chilling;
    buff_t* brain_freeze;
    buff_t* chain_reaction;
    buff_t* cold_front;
    buff_t* cold_front_ready;
    buff_t* cryopathy;
    buff_t* deaths_chill;
    buff_t* fingers_of_frost;
    buff_t* freezing_rain;
    buff_t* freezing_winds;
    buff_t* frigid_empowerment;
    buff_t* icicles;
    buff_t* icy_veins;
    buff_t* permafrost_lances;
    buff_t* ray_of_frost;
    buff_t* slick_ice;


    // Frostfire
    buff_t* excess_fire;
    buff_t* excess_frost;
    buff_t* fire_mastery;
    buff_t* frost_mastery;
    buff_t* frostfire_empowerment;
    buff_t* severe_temperatures;


    // Spellslinger
    buff_t* spellfrost_teachings;
    buff_t* unerring_proficiency;


    // Sunfury
    buff_t* arcane_soul;
    buff_t* burden_of_power;
    buff_t* glorious_incandescence;
    buff_t* lingering_embers;
    buff_t* mana_cascade;
    buff_t* spellfire_sphere;
    buff_t* spellfire_spheres;


    // Shared
    buff_t* ice_floes;
    buff_t* incanters_flow;
    buff_t* overflowing_energy;
    buff_t* time_warp;


    // Set Bonuses
    buff_t* intuition;

    buff_t* blessing_of_the_phoenix;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* arcane_echo;
    cooldown_t* blast_wave;
    cooldown_t* combustion;
    cooldown_t* comet_storm;
    cooldown_t* cone_of_cold;
    cooldown_t* dragons_breath;
    cooldown_t* fire_blast;
    cooldown_t* flurry;
    cooldown_t* from_the_ashes;
    cooldown_t* frost_nova;
    cooldown_t* frozen_orb;
    cooldown_t* meteor;
    cooldown_t* phoenix_flames;
    cooldown_t* presence_of_mind;
    cooldown_t* pyromaniac;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* arcane_surge;
    gain_t* arcane_barrage;
    gain_t* energized_familiar;
  } gains;

  // Options
  struct options_t
  {
    timespan_t frozen_duration = 1.0_s;
    timespan_t scorch_delay = 15_ms;
    timespan_t arcane_missiles_chain_delay = 200_ms;
    double arcane_missiles_chain_relstddev = 0.1;
    timespan_t glacial_spike_delay = 100_ms;
    bool treat_bloodlust_as_time_warp = false;
    unsigned initial_spellfire_spheres = 3;
    arcane_phoenix_rotation arcane_phoenix_rotation_override = arcane_phoenix_rotation::DEFAULT;
  } options;

  // Pets
  struct pets_t
  {
    pet_t* water_elemental = nullptr;
    std::vector<pet_t*> mirror_images;
    pet_t* arcane_phoenix = nullptr;
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* heating_up_generated;         // Crits without HU/HS
    proc_t* heating_up_removed;           // Non-crits with HU >200ms after application
    proc_t* heating_up_ib_converted;      // IBs used on HU
    proc_t* hot_streak;                   // Total HS generated
    proc_t* hot_streak_spell;             // HU/HS spell impacts
    proc_t* hot_streak_spell_crit;        // HU/HS spell crits
    proc_t* hot_streak_spell_crit_wasted; // HU/HS spell crits with HS

    proc_t* ignite_applied;    // Direct ignite applications
    proc_t* ignite_new_spread; // Spread to new target
    proc_t* ignite_overwrite;  // Spread to target with existing ignite

    proc_t* brain_freeze;
    proc_t* brain_freeze_excess_fire;
    proc_t* brain_freeze_time_anomaly;
    proc_t* brain_freeze_water_jet;
    proc_t* fingers_of_frost;
    proc_t* fingers_of_frost_flash_freeze;
    proc_t* fingers_of_frost_freezing_winds;
    proc_t* fingers_of_frost_wasted;
    proc_t* flurry_cast;
    proc_t* winters_chill_applied;
    proc_t* winters_chill_consumed;

    proc_t* icicles_generated;
    proc_t* icicles_fired;
    proc_t* icicles_overflowed;
  } procs;

  struct shuffled_rngs_t
  {
    shuffled_rng_t* time_anomaly;
  } shuffled_rng;

  struct rppms_t
  {
    real_ppm_t* energy_reconstitution;
    real_ppm_t* frostfire_infusion;
  } rppm;

  struct accumulated_rngs_t
  {
    accumulated_rng_t* pyromaniac;
    accumulated_rng_t* spellfrost_teachings;
  } accumulated_rng;

  // Sample data
  struct sample_data_t
  {
    std::unique_ptr<extended_sample_data_t> icy_veins_duration;
    std::unique_ptr<simple_sample_data_t> low_mana_iteration;
  } sample_data;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_charge;
    const spell_data_t* arcane_mage;
    const spell_data_t* clearcasting;
    const spell_data_t* mana_adept;
    const spell_data_t* savant;

    // Fire
    const spell_data_t* fire_mage;
    const spell_data_t* fuel_the_fire;
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
    timespan_t last_enlightened_update;
    timespan_t gained_full_icicles;
    bool had_low_mana;
    bool trigger_dematerialize;
    bool trigger_leydrinker;
    bool trigger_ff_empowerment;
    bool trigger_flash_freezeburn;
    bool trigger_glorious_incandescence;
    int embedded_splinters;
    int magis_spark_spells;
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
    player_talent_t incanters_flow;

    // Row 3
    player_talent_t winters_protection;
    player_talent_t spellsteal;
    player_talent_t tempest_barrier;
    player_talent_t incantation_of_swiftness;
    player_talent_t remove_curse;
    player_talent_t arcane_warding;

    // Row 4
    player_talent_t mirror_image;
    player_talent_t shifting_power;
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
    player_talent_t ice_nova;
    player_talent_t ring_of_frost;
    player_talent_t ice_floes;
    player_talent_t shimmer;
    player_talent_t blast_wave;

    // Row 7
    player_talent_t improved_frost_nova;
    player_talent_t rigid_ice;
    player_talent_t tome_of_rhonin;
    player_talent_t dragons_breath;
    player_talent_t supernova;
    player_talent_t tome_of_antonidas;
    player_talent_t volatile_detonation;
    player_talent_t energized_barriers;

    // Row 8
    player_talent_t frigid_winds;
    player_talent_t flow_of_time;
    player_talent_t temporal_velocity;

    // Row 9
    player_talent_t ice_ward;
    player_talent_t freezing_cold;
    player_talent_t time_manipulation;
    player_talent_t displacement;
    player_talent_t accumulative_shielding;
    player_talent_t greater_invisibility;
    player_talent_t barrier_diffusion;

    // Row 10
    player_talent_t ice_cold;
    player_talent_t inspired_intellect;
    player_talent_t time_anomaly;
    player_talent_t mass_barrier;
    player_talent_t mass_invisibility;


    // Arcane
    // Row 1
    player_talent_t arcane_missiles;

    // Row 2
    player_talent_t amplification;
    player_talent_t nether_precision;

    // Row 3
    player_talent_t charged_orb;
    player_talent_t arcane_tempo;
    player_talent_t concentrated_power;
    player_talent_t consortiums_bauble;
    player_talent_t arcing_cleave;

    // Row 4
    player_talent_t arcane_familiar;
    player_talent_t arcane_surge;
    player_talent_t improved_clearcasting;

    // Row 5
    player_talent_t big_brained;
    player_talent_t energized_familiar;
    player_talent_t presence_of_mind;
    player_talent_t surging_urge;
    player_talent_t slipstream;
    player_talent_t static_cloud;
    player_talent_t resonance;

    // Row 6
    player_talent_t impetus;
    player_talent_t touch_of_the_magi;
    player_talent_t dematerialize;

    // Row 7
    player_talent_t resonant_orbs;
    player_talent_t illuminated_thoughts;
    player_talent_t evocation;
    player_talent_t improved_touch_of_the_magi;
    player_talent_t eureka;
    player_talent_t energy_reconstitution;
    player_talent_t reverberate;

    // Row 8
    player_talent_t arcane_debilitation;
    player_talent_t arcane_echo;
    player_talent_t prodigious_savant;

    // Row 9
    player_talent_t time_loop;
    player_talent_t aether_attunement;
    player_talent_t enlightened;
    player_talent_t arcane_bombardment;
    player_talent_t leysight;
    player_talent_t concentration;

    // Row 10
    player_talent_t high_voltage;
    player_talent_t arcane_harmony;
    player_talent_t magis_spark;
    player_talent_t nether_munitions;
    player_talent_t orb_barrage;
    player_talent_t leydrinker;


    // Fire
    // Row 1
    player_talent_t pyroblast;

    // Row 2
    player_talent_t fire_blast;
    player_talent_t firestarter;
    player_talent_t phoenix_flames;

    // Row 3
    player_talent_t pyrotechnics;
    player_talent_t fervent_flickering;
    player_talent_t call_of_the_sun_king;
    player_talent_t majesty_of_the_phoenix;

    // Row 4
    player_talent_t scorch;
    player_talent_t surging_blaze;
    player_talent_t lit_fuse;

    // Row 5
    player_talent_t improved_scorch;
    player_talent_t scald;
    player_talent_t heat_shimmer;
    player_talent_t flame_on;
    player_talent_t flame_accelerant;
    player_talent_t critical_mass;
    player_talent_t sparking_cinders;
    player_talent_t explosive_ingenuity;

    // Row 6
    player_talent_t inflame;
    player_talent_t alexstraszas_fury;
    player_talent_t ashen_feather;
    player_talent_t intensifying_flame;
    player_talent_t combustion;
    player_talent_t controlled_destruction;
    player_talent_t flame_patch;
    player_talent_t quickflame;
    player_talent_t convection;

    // Row 7
    player_talent_t master_of_flame;
    player_talent_t wildfire;
    player_talent_t improved_combustion;
    player_talent_t spontaneous_combustion;
    player_talent_t feel_the_burn;
    player_talent_t mark_of_the_firelord;

    // Row 8
    player_talent_t fevered_incantation;
    player_talent_t kindling;
    player_talent_t fires_ire;

    // Row 9
    player_talent_t pyromaniac;
    player_talent_t molten_fury;
    player_talent_t from_the_ashes;
    player_talent_t fiery_rush;
    player_talent_t meteor;
    player_talent_t firefall;
    player_talent_t explosivo;

    // Row 10
    player_talent_t hyperthermia;
    player_talent_t phoenix_reborn;
    player_talent_t sun_kings_blessing;
    player_talent_t unleashed_inferno;
    player_talent_t deep_impact;
    player_talent_t blast_zone;


    // Frost
    // Row 1
    player_talent_t ice_lance;

    // Row 2
    player_talent_t frozen_orb;
    player_talent_t fingers_of_frost;

    // Row 3
    player_talent_t flurry;
    player_talent_t shatter;

    // Row 4
    player_talent_t brain_freeze;
    player_talent_t everlasting_frost;
    player_talent_t winters_blessing;
    player_talent_t frostbite;
    player_talent_t piercing_cold;

    // Row 5
    player_talent_t perpetual_winter;
    player_talent_t lonely_winter;
    player_talent_t permafrost_lances;
    player_talent_t bone_chilling;

    // Row 6
    player_talent_t comet_storm;
    player_talent_t frozen_touch;
    player_talent_t wintertide;
    player_talent_t ice_caller;
    player_talent_t flash_freeze;
    player_talent_t subzero;

    // Row 7
    player_talent_t deep_shatter;
    player_talent_t icy_veins;
    player_talent_t splintering_cold;

    // Row 8
    player_talent_t glacial_assault;
    player_talent_t freezing_rain;
    player_talent_t thermal_void;
    player_talent_t splitting_ice;
    player_talent_t chain_reaction;

    // Row 9
    player_talent_t fractured_frost;
    player_talent_t freezing_winds;
    player_talent_t slick_ice;
    player_talent_t hailstones;
    player_talent_t ray_of_frost;

    // Row 10
    player_talent_t deaths_chill;
    player_talent_t cold_front;
    player_talent_t coldest_snap;
    player_talent_t glacial_spike;
    player_talent_t cryopathy;
    player_talent_t splintering_ray;


    // Frostfire
    // Row 1
    player_talent_t frostfire_mastery;

    // Row 2
    player_talent_t imbued_warding;
    player_talent_t meltdown;
    player_talent_t frostfire_bolt;
    player_talent_t elemental_affinity;
    player_talent_t flame_and_frost;

    // Row 3
    player_talent_t isothermic_core;
    player_talent_t severe_temperatures;
    player_talent_t thermal_conditioning;
    player_talent_t frostfire_infusion;

    // Row 4
    player_talent_t excess_frost;
    player_talent_t frostfire_empowerment;
    player_talent_t excess_fire;

    // Row 5
    player_talent_t flash_freezeburn;


    // Spellslinger
    // Row 1
    player_talent_t splintering_sorcery;

    // Row 2
    player_talent_t augury_abounds;
    player_talent_t controlled_instincts;
    player_talent_t splintering_orbs;

    // Row 3
    player_talent_t look_again;
    player_talent_t slippery_slinging;
    player_talent_t phantasmal_image;
    player_talent_t reactive_barrier;
    player_talent_t unerring_proficiency;
    player_talent_t volatile_magic;

    // Row 4
    player_talent_t shifting_shards;
    player_talent_t force_of_will;
    player_talent_t spellfrost_teachings;

    // Row 5
    player_talent_t splinterstorm;


    // Sunfury
    // Row 1
    player_talent_t spellfire_spheres;

    // Row 2
    player_talent_t mana_cascade;
    player_talent_t invocation_arcane_phoenix;
    player_talent_t burden_of_power;

    // Row 3
    player_talent_t merely_a_setback;
    player_talent_t gravity_lapse;
    player_talent_t lessons_in_debilitation;
    player_talent_t glorious_incandescence;

    // Row 4
    player_talent_t savor_the_moment;
    player_talent_t sunfury_execution;
    player_talent_t codex_of_the_sunstriders;
    player_talent_t ignite_the_future;
    player_talent_t rondurmancy;

    // Row 5
    player_talent_t memory_of_alar;
  } talents;

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
  double stacking_movement_modifier() const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_target_pet_damage_multiplier( player_t*, bool ) const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_rating_multiplier( rating_e ) const override;
  double composite_attribute_multiplier( attribute_e ) const override;
  double matching_gear_multiplier( attribute_e ) const override;
  void arise() override;
  void combat_begin() override;
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

  action_t* get_icicle();
  void trigger_arcane_charge( int stacks = 1 );
  bool trigger_brain_freeze( double chance, proc_t* source, timespan_t delay = 0.15_s );
  bool trigger_crowd_control( const action_state_t* s, spell_mechanic type, timespan_t adjust = 0_ms );
  bool trigger_clearcasting( double chance, timespan_t delay = 0.15_s );
  bool trigger_fof( double chance, proc_t* source, int stacks = 1 );
  void trigger_icicle( player_t* icicle_target, bool chain = false );
  void trigger_icicle_gain( player_t* icicle_target, action_t* icicle_action, double chance = 1.0, timespan_t duration = timespan_t::min() );
  void trigger_lit_fuse();
  void trigger_mana_cascade();
  void trigger_merged_buff( buff_t* buff, bool trigger );
  void trigger_meteor_burn( action_t* action, player_t* target, timespan_t pulse_time, timespan_t duration );
  void trigger_flash_freezeburn( bool ffb = false );
  void trigger_spellfire_spheres();
  void consume_burden_of_power();
  void trigger_splinter( player_t* target, int count = -1 );
  void trigger_time_manipulation();
  void update_enlightened( bool double_regen = false );
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
    action_list_str = "water_jet/waterbolt";
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

  void impact( action_state_t* s ) override
  {
    mage_pet_spell_t::impact( s );

    if ( result_is_hit( s->result ) && o()->buffs.icy_veins->check() )
      o()->buffs.frigid_empowerment->trigger();
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

    bool success = o()->trigger_crowd_control( s, MECHANIC_FREEZE );
    if ( success && o()->buffs.icy_veins->check() )
      o()->buffs.frigid_empowerment->trigger( o()->buffs.frigid_empowerment->max_stack() );
  }
};

struct water_jet_t final : public mage_pet_spell_t
{
  water_jet_t( std::string_view n, water_elemental_pet_t* p, std::string_view options_str ) :
    mage_pet_spell_t( n, p, p->find_pet_spell( "Water Jet" ) )
  {
    parse_options( options_str );
    channeled = true;
  }

  void execute() override
  {
    mage_pet_spell_t::execute();
    o()->trigger_brain_freeze( 1.0, o()->procs.brain_freeze_water_jet, 0_ms );
  }

  void impact( action_state_t* s ) override
  {
    mage_pet_spell_t::impact( s );

    if ( result_is_hit( s->result ) && o()->buffs.icy_veins->check() )
      o()->buffs.frigid_empowerment->trigger();
  }

  void tick( dot_t* d ) override
  {
    mage_pet_spell_t::tick( d );

    if ( o()->buffs.icy_veins->check() )
      o()->buffs.frigid_empowerment->trigger();
  }
};

action_t* water_elemental_pet_t::create_action( std::string_view name, std::string_view options_str )
{
  if ( name == "water_jet" ) return new water_jet_t( name, this, options_str );
  if ( name == "waterbolt" ) return new waterbolt_t( name, this, options_str );

  return mage_pet_t::create_action( name, options_str );
}

void water_elemental_pet_t::create_actions()
{
  o()->action.pet_freeze = get_action<freeze_t>( "freeze", this );

  // Create Water Jet that can be used by the proxy action
  o()->action.pet_water_jet = create_action( "water_jet", "" );

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

namespace arcane_phoenix {

// ==========================================================================
// Pet Arcane Phoenix
// ==========================================================================

struct arcane_phoenix_spell_t : public mage_pet_spell_t
{
  bool is_mage_spell; // TODO: Check if these spells also scale with target multipliers.
  bool exceptional;

  arcane_phoenix_spell_t( std::string_view n, mage_pet_t* p, const spell_data_t* s, bool exceptional_ = false ) :
    mage_pet_spell_t( n, p, s ),
    is_mage_spell( false ),
    exceptional( exceptional_ )
  {
    background = true;
    cooldown->duration = 0_ms;
    base_costs[ RESOURCE_MANA ] = 0;
  }

  void init() override
  {
    if ( initialized )
      return;

    mage_pet_spell_t::init();

    if ( !is_mage_spell )
      return;

    base_multiplier *= 1.0 + o()->spec.arcane_mage->effectN( 1 ).percent();
    base_multiplier *= 1.0 + o()->spec.fire_mage->effectN( 1 ).percent();
    // TODO: Uncomment when this modifier is fixed to be properly applied.
    // base_dd_multiplier *= 1.0 + o()->spec.arcane_mage->effectN( 9 ).percent();
    crit_bonus_multiplier *= 1.0 + o()->talents.overflowing_energy->effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + o()->talents.wildfire->effectN( 2 ).percent();
  }

  double action_multiplier() const override
  {
    double m = mage_pet_spell_t::action_multiplier();

    if ( is_mage_spell )
    {
      if ( o()->buffs.arcane_surge->check() )
        m *= 1.0 + o()->buffs.arcane_surge->data().effectN( 1 ).percent();

      m *= 1.0 + o()->buffs.incanters_flow->check_stack_value();
      m *= 1.0 + o()->buffs.lingering_embers->check_stack_value();
      m *= 1.0 + o()->buffs.spellfire_sphere->check_stack_value();
    }

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = mage_pet_spell_t::composite_da_multiplier( s );

    if ( is_mage_spell )
      m *= 1.0 + o()->buffs.blessing_of_the_phoenix->check_value();

    return m;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = mage_pet_spell_t::composite_crit_damage_bonus_multiplier();

    if ( is_mage_spell )
    {
      if ( o()->buffs.combustion->check() )
      {
        // TODO: The value here comes from spell 453385 effect#2, which is then adjusted based on the talent rank.
        // For now, just use effect#3, which is what Blizzard is using for the tooltip.
        double value = 0.001 * o()->talents.fires_ire->effectN( 3 ).base_value();
        if ( o()->bugs )
          value = std::floor( value );
        m *= 1.0 + value * 0.01;
      }

      m *= 1.0 + o()->buffs.wildfire->check_value();
    }

    return m;
  }

  void assess_damage( result_amount_type rt, action_state_t* s ) override
  {
    mage_pet_spell_t::assess_damage( rt, s );

    if ( !is_mage_spell || s->result_total <= 0.0 )
      return;

    auto td = o()->find_target_data( s->target );
    if ( td && td->debuffs.touch_of_the_magi->check() && o()->talents.arcane_echo.ok() && o()->cooldowns.arcane_echo->up() )
    {
      make_event( *sim, [ this, t = s->target ] { o()->action.arcane_echo->execute_on_target( t ); } );
      o()->cooldowns.arcane_echo->start();
    }
  }
};

struct arcane_phoenix_pet_t final : public mage_pet_t
{
  event_t* cast_event;
  std::vector<action_t*> st_actions;
  std::vector<action_t*> aoe_actions;
  std::vector<action_t*> exceptional_actions;
  timespan_t cast_period;
  int spells_used;
  int exceptional_spells_used;
  int exceptional_spells_remaining;
  bool exceptional_meteor_used;

  arcane_phoenix_pet_t( sim_t* sim, mage_t* owner ) :
    mage_pet_t( sim, owner, "arcane_phoenix", true, true ),
    cast_event(),
    st_actions(),
    aoe_actions(),
    exceptional_actions(),
    cast_period( owner->find_spell( 448659 )->effectN( 2 ).period() ),
    spells_used(),
    exceptional_spells_used(),
    exceptional_spells_remaining()
  {
    can_dismiss = true;
    owner_coeff.sp_from_sp = 1.0;
  }

  void schedule_cast()
  {
    cast_event = nullptr;
    action_t* action;
    const auto& tl = sim->target_non_sleeping_list;

    if ( spells_used % 2 == 1 && exceptional_spells_remaining > 0 )
    {
      action = exceptional_actions[ rng().range( exceptional_actions.size() ) ];
      // TODO: What happens with Ignite the Future and without Codex of the Sunstriders?
      o()->buffs.spellfire_sphere->decrement();
      o()->buffs.lingering_embers->trigger();
      exceptional_spells_used++;
      exceptional_spells_remaining--;
    }
    else
    {
      if ( o()->options.arcane_phoenix_rotation_override == arcane_phoenix_rotation::DEFAULT && tl.size() > 1
        || o()->options.arcane_phoenix_rotation_override == arcane_phoenix_rotation::AOE )
      {
        action = aoe_actions[ rng().range( aoe_actions.size() ) ];
      }
      else
      {
        action = st_actions[ rng().range( st_actions.size() ) ];
      }
    }

    spells_used++;
    player_t* t = tl[ rng().range( tl.size() ) ];
    action->execute_on_target( t );
    cast_event = make_event( *sim, cast_period, [ this ] { schedule_cast(); } );
  }

  void arise() override
  {
    mage_pet_t::arise();

    exceptional_meteor_used = false;
    spells_used = 0;
    exceptional_spells_used = 0;
    exceptional_spells_remaining = o()->talents.codex_of_the_sunstriders.ok() ? o()->buffs.spellfire_sphere->check() : 0;

    assert( !cast_event );
    schedule_cast();
  };

  void demise() override
  {
    mage_pet_t::demise();

    event_t::cancel( cast_event );

    if ( !o()->talents.memory_of_alar.ok() )
      return;

    timespan_t buff_duration;
    if ( o()->specialization() == MAGE_FIRE )
    {
      buff_duration = o()->talents.memory_of_alar->effectN( 3 ).time_value()
                    + exceptional_spells_used * o()->talents.memory_of_alar->effectN( 4 ).time_value();
      o()->buffs.hyperthermia->execute( -1, buff_t::DEFAULT_VALUE(), buff_duration );
      o()->buffs.hyperthermia->predict();
    }
    else
    {
      buff_duration = o()->talents.memory_of_alar->effectN( 1 ).time_value()
                    + exceptional_spells_used * o()->talents.memory_of_alar->effectN( 2 ).time_value();
      o()->buffs.arcane_soul->trigger( buff_duration );
    }
  };

  void create_actions() override;
};

struct arcane_barrage_t final : public arcane_phoenix_spell_t
{
  arcane_barrage_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 450499 ) )
  {}
};

struct pyroblast_t final : public arcane_phoenix_spell_t
{
  pyroblast_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 450461 ) )
  {}
};

struct flamestrike_t final : public arcane_phoenix_spell_t
{
  flamestrike_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 450462 ) )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value(); // TODO: Verify this
    is_mage_spell = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = arcane_phoenix_spell_t::composite_da_multiplier( s );

    if ( o()->buffs.combustion->check() )
      m *= 1.0 + o()->talents.unleashed_inferno->effectN( 4 ).percent();

    if ( o()->buffs.sparking_cinders->check() )
      m *= 1.0 + o()->talents.sparking_cinders->effectN( 2 ).percent();

    if ( o()->buffs.majesty_of_the_phoenix->check() )
      m *= 1.0 + o()->buffs.majesty_of_the_phoenix->data().effectN( 1 ).percent();

    // TODO: Double check that this actually applies and check whether it gets consumed.
    if ( o()->buffs.burden_of_power->check() )
      m *= 1.0 + o()->buffs.burden_of_power->data().effectN( 3 ).percent();

    return m;
  }
};

struct arcane_surge_t final : public arcane_phoenix_spell_t
{
  arcane_surge_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 453326 ), true )
  {
    reduced_aoe_targets = data().effectN( 3 ).base_value(); // TODO: Verify this
    is_mage_spell = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = arcane_phoenix_spell_t::composite_da_multiplier( s );

    m *= 1.0 + o()->cache.mastery() * o()->spec.savant->effectN( 5 ).mastery_value();

    return m;
  }
};

struct greater_pyroblast_t final : public arcane_phoenix_spell_t
{
  greater_pyroblast_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 450421 ), true )
  {}
};

struct meteorite_impact_t final : public arcane_phoenix_spell_t
{
  meteorite_impact_t( std::string_view n, arcane_phoenix_pet_t* p, bool exceptional = false ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( exceptional ? 456139 : 449569 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 8; // TODO: Verify this
    is_mage_spell = !exceptional;
  }
};

struct meteorite_t final : public arcane_phoenix_spell_t
{
  action_t* damage_action = nullptr;
  action_t* damage_action_exceptional = nullptr;
  timespan_t fall_time;
  timespan_t meteor_delay;

  meteorite_t( std::string_view n, arcane_phoenix_pet_t* p, bool exceptional_ = false ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 449559 ), exceptional_ ),
    fall_time( timespan_t::from_seconds( p->find_spell( exceptional_ ? 456137 : 449560 )->missile_speed() ) ),
    meteor_delay( p->find_spell( 449562 )->duration() )
  {
    damage_action = damage_action_exceptional = get_action<meteorite_impact_t>( "meteorite_exceptional_impact", p, true );
    if ( !exceptional )
      damage_action = get_action<meteorite_impact_t>( "meteorite_impact", p );
  }

  const arcane_phoenix_pet_t* p() const
  { return static_cast<arcane_phoenix_pet_t*>( player ); }

  arcane_phoenix_pet_t* p()
  { return static_cast<arcane_phoenix_pet_t*>( player ); }

  timespan_t travel_time() const override
  {
    return std::max( meteor_delay * p()->cache.spell_cast_speed(), fall_time ) - fall_time;
  }

  void execute() override
  {
    arcane_phoenix_spell_t::execute();

    if ( exceptional )
    {
      // TODO: Test the delay more rigorously
      make_repeating_event( *sim, 75_ms, [ this, t = target ] { target = t; arcane_phoenix_spell_t::execute(); }, 3 );
    }
  }

  void impact( action_state_t* s ) override
  {
    arcane_phoenix_spell_t::impact( s );

    // The Meteorite fizzles if it does not spawn in the sky before the Arcane Phoenix expires.
    // TODO: Check this later
    if ( !p()->is_sleeping() )
    {
      // Once an instance of the pet has dealt damage with one exceptional Meteorite,
      // all subsequent regular meteorites it casts will use the exceptional spell ID.
      // TODO: Check this later
      action_t* a = p()->exceptional_meteor_used ? damage_action_exceptional : damage_action;
      make_event( *sim, fall_time, [ a, t = s->target ] { a->execute_on_target( t ); } );
    }
  }
};

void arcane_phoenix_pet_t::create_actions()
{
  mage_pet_t::create_actions();

  st_actions.push_back( get_action<arcane_barrage_t>( "arcane_barrage", this ) );
  st_actions.push_back( get_action<pyroblast_t>( "pyroblast", this ) );

  aoe_actions.push_back( get_action<arcane_barrage_t>( "arcane_barrage", this ) );
  aoe_actions.push_back( get_action<flamestrike_t>( "flamestrike", this ) );
  aoe_actions.push_back( get_action<meteorite_t>( "meteorite", this ) );

  if ( o()->talents.codex_of_the_sunstriders.ok() )
  {
    exceptional_actions.push_back( get_action<arcane_surge_t>( "arcane_surge", this ) );
    exceptional_actions.push_back( get_action<greater_pyroblast_t>( "greater_pyroblast", this ) );
    exceptional_actions.push_back( get_action<meteorite_t>( "meteorite_exceptional", this, true ) );
  }
};

}  // arcane_phoenix

}  // pets

namespace buffs {

// Custom buffs =============================================================

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
    double pct = p->talents.touch_of_the_magi->effectN( 1 ).percent() + p->talents.improved_touch_of_the_magi->effectN( 1 ).percent();
    p->action.touch_of_the_magi_explosion->execute_on_target( player, pct * current_value );
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

    if ( p->talents.fires_ire.ok() )
      add_invalidate( CACHE_CRIT_CHANCE );

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

    p->buffs.frigid_empowerment->expire();
    p->buffs.deaths_chill->expire();
    p->buffs.slick_ice->expire();

    if ( !p->pets.water_elemental->is_sleeping() )
      p->pets.water_elemental->dismiss();
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

// Some Frost(fire) spells snapshot on impact (rather than execute). This is handled via
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
    bool arcane_debilitation = false;
    bool arcane_surge = true;
    bool bone_chilling = true;
    bool deaths_chill = true;
    bool frigid_empowerment = true;
    bool icicles_aoe = false;
    bool icicles_st = false;
    bool improved_scorch = true;
    bool incanters_flow = true;
    bool lingering_embers = true;
    bool molten_fury = true;
    bool nether_munitions = true;
    bool numbing_blast = true;
    bool savant = false;
    bool spellfire_sphere = true;
    bool unleashed_inferno = false;

    bool blessing_of_the_phoenix = true;

    // Misc
    bool combustion = true;
    bool fires_ire = true;
    bool flame_accelerant = false;
    bool force_of_will = true;
    bool ice_floes = false;
    bool overflowing_energy = true;
    bool shatter = true;
    bool shifting_power = true;
    bool time_manipulation = false;
    bool wildfire = true;
  } affected_by;

  struct triggers_t
  {
    bool chill = false;
    bool clearcasting = false;
    bool from_the_ashes = false;
    bool ignite = false;
    bool overflowing_energy = false;
    bool touch_of_the_magi = true;

    target_trigger_type_e calefaction = TT_NONE;
    target_trigger_type_e hot_streak = TT_NONE;
    target_trigger_type_e kindling = TT_NONE;
    target_trigger_type_e unleashed_inferno = TT_NONE;
  } triggers;

  bool calculate_on_impact;
  unsigned impact_flags;

public:
  mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    affected_by(),
    triggers(),
    calculate_on_impact(),
    impact_flags()
  {
    weapon_multiplier = 0.0;
    affected_by.ice_floes = data().affected_by( p->talents.ice_floes->effectN( 1 ) );
    track_cd_waste = data().cooldown() > 0_ms || data().charge_cooldown() > 0_ms;
    energize_type = action_energize::NONE;

    const auto& ea = p->talents.elemental_affinity;
    if ( p->specialization() == MAGE_FIRE )
      for ( int ix : { 1, 2, 5 } )
        apply_affecting_effect( ea->effectN( ix ) );

    if ( p->specialization() == MAGE_FROST )
      apply_affecting_effect( ea->effectN( 3 ) );
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

    if ( affected_by.force_of_will )
      crit_bonus_multiplier *= 1.0 + p()->talents.force_of_will->effectN( 2 ).percent();

    if ( affected_by.overflowing_energy )
      crit_bonus_multiplier *= 1.0 + p()->talents.overflowing_energy->effectN( 1 ).percent();

    if ( affected_by.wildfire )
      crit_bonus_multiplier *= 1.0 + p()->talents.wildfire->effectN( 2 ).percent();

    // Save some CPU time by not computing frozen flags/frozen multiplier for Arcane and Fire.
    if ( harmful && affected_by.shatter && p()->specialization() == MAGE_FROST )
    {
      snapshot_flags |= STATE_FROZEN | STATE_FROZEN_MUL;
      update_flags   |= STATE_FROZEN | STATE_FROZEN_MUL;
    }

    if ( calculate_on_impact )
      std::swap( snapshot_flags, impact_flags );

    if ( !harmful )
      target = player;

    if ( affected_by.time_manipulation && !range::contains( p()->time_manipulation_cooldowns, cooldown ) )
      p()->time_manipulation_cooldowns.push_back( cooldown );
  }

  double action_multiplier() const override
  {
    double m = spell_t::action_multiplier();

    if ( affected_by.arcane_surge && p()->buffs.arcane_surge->check() )
      m *= 1.0 + p()->buffs.arcane_surge->data().effectN( 1 ).percent();

    if ( affected_by.bone_chilling )
      m *= 1.0 + p()->buffs.bone_chilling->check_stack_value();

    if ( affected_by.deaths_chill )
      m *= 1.0 + p()->buffs.deaths_chill->check_stack_value();

    if ( affected_by.frigid_empowerment )
      m *= 1.0 + p()->buffs.frigid_empowerment->check_stack_value();

    if ( affected_by.icicles_aoe )
      m *= 1.0 + p()->cache.mastery() * p()->spec.icicles_2->effectN( 1 ).mastery_value();

    if ( affected_by.icicles_st )
      m *= 1.0 + p()->cache.mastery() * p()->spec.icicles_2->effectN( 3 ).mastery_value();

    if ( affected_by.incanters_flow )
      m *= 1.0 + p()->buffs.incanters_flow->check_stack_value();

    if ( affected_by.lingering_embers )
      m *= 1.0 + p()->buffs.lingering_embers->check_stack_value();

    if ( affected_by.spellfire_sphere )
      m *= 1.0 + p()->buffs.spellfire_sphere->check_stack_value();

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = spell_t::composite_da_multiplier( s );

    if ( affected_by.savant )
      m *= 1.0 + p()->cache.mastery() * p()->spec.savant->effectN( 5 ).mastery_value();

    if ( affected_by.unleashed_inferno && p()->buffs.combustion->check() )
      m *= 1.0 + p()->talents.unleashed_inferno->effectN( 1 ).percent();

    if ( affected_by.blessing_of_the_phoenix )
      m *= 1.0 + p()->buffs.blessing_of_the_phoenix->check_value();

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = spell_t::composite_target_multiplier( target );

    if ( auto td = find_td( target ) )
    {
      if ( affected_by.arcane_debilitation )
        m *= 1.0 + td->debuffs.arcane_debilitation->check_stack_value();
      if ( affected_by.improved_scorch )
        m *= 1.0 + td->debuffs.improved_scorch->check_stack_value();
      if ( affected_by.molten_fury )
        m *= 1.0 + td->debuffs.molten_fury->check_value();
      if ( affected_by.nether_munitions )
        m *= 1.0 + td->debuffs.nether_munitions->check_value();
      if ( affected_by.numbing_blast )
        m *= 1.0 + td->debuffs.numbing_blast->check_value();
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = spell_t::composite_crit_chance();

    if ( affected_by.combustion )
      c += p()->buffs.combustion->check_value();

    if ( affected_by.overflowing_energy )
      c += p()->buffs.overflowing_energy->check_stack_value();

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = spell_t::composite_crit_damage_bonus_multiplier();

    if ( affected_by.fires_ire && p()->buffs.combustion->check() )
    {
      // TODO: The value here comes from spell 453385 effect#2, which is then adjusted based on the talent rank.
      // For now, just use effect#3, which is what Blizzard is using for the tooltip.
      double value = 0.001 * p()->talents.fires_ire->effectN( 3 ).base_value();
      if ( p()->bugs )
        value = std::floor( value );
      m *= 1.0 + value * 0.01;
    }

    if ( affected_by.wildfire )
      m *= 1.0 + p()->buffs.wildfire->check_value();

    return m;
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

  virtual void snapshot_impact_state( action_state_t* s, result_amount_type rt )
  { snapshot_internal( s, impact_flags, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  { return calculate_on_impact ? 0.0 : spell_t::calculate_direct_amount( s ); }

  virtual double calculate_impact_direct_amount( action_state_t* s ) const
  { return spell_t::calculate_direct_amount( s ); }

  result_e calculate_result( action_state_t* s ) const override
  { return calculate_on_impact ? RESULT_NONE : spell_t::calculate_result( s ); }

  virtual result_e calculate_impact_result( action_state_t* s ) const
  { return spell_t::calculate_result( s ); }

  void enable_calculate_on_impact( unsigned spell_id )
  {
    calculate_on_impact = true;
    auto spell = player->find_spell( spell_id );
    for ( const auto& eff : spell->effects() )
      parse_effect_data( eff );
    may_crit = !spell->flags( SX_CANNOT_CRIT );
    tick_may_crit = spell->flags( SX_TICK_MAY_CRIT );
  }

  bool usable_moving() const override
  {
    if ( p()->buffs.ice_floes->check() && affected_by.ice_floes )
      return true;

    return spell_t::usable_moving();
  }

  virtual void consume_cost_reductions()
  { }

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( last_resource_cost > 0
      && !p()->resources.is_infinite( RESOURCE_MANA )
      && p()->resources.pct( RESOURCE_MANA ) < 0.1 )
    {
      p()->state.had_low_mana = true;
    }
  }

  void execute() override
  {
    spell_t::execute();

    // Make sure we remove all cost reduction buffs before we trigger new ones.
    // This will prevent for example Arcane Missiles consuming its own Clearcasting proc.
    consume_cost_reductions();

    if ( p()->spec.clearcasting->ok() && triggers.clearcasting )
    {
      // TODO: implement the hidden BLP
      double chance = p()->spec.clearcasting->effectN( 2 ).percent();
      chance += p()->talents.illuminated_thoughts->effectN( 1 ).percent();
      // Arcane Blast gets an additional 5% chance. Not mentioned in the spell data (or even the description).
      if ( id == 30451 )
        chance += 0.05;
      p()->trigger_clearcasting( chance );
    }

    if ( !background && affected_by.ice_floes && time_to_execute > 0_ms )
      p()->buffs.ice_floes->decrement();

    // TODO: This might need a more fine grained control since some spells behave weirdly
    // (Blast Wave doesn't trigger Fire mastery, for example).
    if ( harmful && !background )
      trigger_frostfire_mastery();
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

    spell_t::impact( s );

    if ( s->result_total <= 0.0 )
      return;

    // TODO: OE now triggers from procs but expires from non-procs. We'll need to
    // redo triggers.overflowing_energy and properly mark every proc spell in the module.
    if ( callbacks && p()->talents.overflowing_energy.ok() && s->result_type == result_amount_type::DMG_DIRECT && ( s->result == RESULT_CRIT || triggers.overflowing_energy ) )
      p()->trigger_merged_buff( p()->buffs.overflowing_energy, s->result != RESULT_CRIT );

    if ( p()->talents.fevered_incantation.ok() && s->result_type == result_amount_type::DMG_DIRECT )
      p()->trigger_merged_buff( p()->buffs.fevered_incantation, s->result == RESULT_CRIT );

    if ( tt_applicable( s, triggers.calefaction ) )
      trigger_calefaction( s->target );

    // TODO: Test the exact behavior of the hidden Molten Fury debuff.
    if ( p()->talents.molten_fury.ok() )
    {
      if ( target->health_percentage() <= p()->talents.molten_fury->effectN( 1 ).base_value() )
        get_td( s->target )->debuffs.molten_fury->trigger();
      else
        get_td( s->target )->debuffs.molten_fury->expire();
    }

    if ( callbacks && dbc::has_common_school( get_school(), SCHOOL_FROSTFIRE ) && s->result_type == result_amount_type::DMG_DIRECT )
    {
      if ( p()->rppm.frostfire_infusion->trigger() )
        p()->action.frostfire_infusion->execute_on_target( s->target );
      p()->buffs.frostfire_empowerment->trigger();
    }
  }

  void assess_damage( result_amount_type rt, action_state_t* s ) override
  {
    spell_t::assess_damage( rt, s );

    if ( s->result_total <= 0.0 )
      return;

    if ( auto td = find_td( s->target ) )
    {
      auto totm = td->debuffs.touch_of_the_magi;
      if ( totm->check() && triggers.touch_of_the_magi )
      {
        // Touch of the Magi factors out debuffs with effect subtype 87 (Modify Damage Taken%), but only
        // if they increase damage taken. It does not factor out debuffs with effect subtype 270
        // (Modify Damage Taken% from Caster) or 271 (Modify Damage Taken% from Caster's Spells).
        totm->current_value += s->result_total / std::max( cast_state( s )->totm_factor, 1.0 );

        // Arcane Echo doesn't use the normal callbacks system (both in simc and in game). To prevent
        // loops, we need to explicitly check that the triggering action wasn't Arcane Echo.
        if ( p()->talents.arcane_echo.ok() && this != p()->action.arcane_echo && p()->cooldowns.arcane_echo->up() )
        {
          make_event( *sim, [ this, t = s->target ] { p()->action.arcane_echo->execute_on_target( t ); } );
          p()->cooldowns.arcane_echo->start();
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

  void trigger_winters_chill( const action_state_t* s, int stacks = -1 )
  {
    if ( !result_is_hit( s->result ) )
      return;

    auto wc = get_td( s->target )->debuffs.winters_chill;
    if ( stacks < 0 )
      stacks = wc->max_stack();
    wc->trigger( stacks );
    for ( int i = 0; i < stacks; i++ )
      p()->procs.winters_chill_applied->occur();
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

  void trigger_calefaction( player_t* /*target*/ )
  {
    if ( !p()->talents.phoenix_reborn.ok() )
      return;

    p()->buffs.calefaction->trigger();
    if ( p()->buffs.calefaction->at_max_stacks() )
    {
      p()->buffs.calefaction->expire();
      // Trigger the buff outside of impact processing so that Phoenix Flames
      // doesn't benefit from the buff it just triggered.
      make_event( *sim, [ b = p()->buffs.flames_fury ] { b->trigger( b->max_stack() ); } );
    }
  }

  void trigger_frostfire_mastery( bool empowerment = false )
  {
    if ( !p()->talents.frostfire_mastery.ok() || ( empowerment && !p()->talents.flash_freezeburn.ok() ) )
      return;

    auto s = get_school();
    bool is_fire = dbc::is_school( s, SCHOOL_FIRE );
    bool is_frost = dbc::is_school( s, SCHOOL_FROST );
    if ( !is_fire && !is_frost )
      return;

    buff_t* fire = p()->buffs.fire_mastery;
    buff_t* frost = p()->buffs.frost_mastery;

    if ( empowerment )
    {
      fire->expire();
      frost->expire();
    }

    int fire_before = fire->check();
    int frost_before = frost->check();

    if ( is_fire )
      fire->trigger( empowerment ? fire->max_stack() : -1 );

    if ( is_frost )
      frost->trigger( empowerment ? frost->max_stack() : -1 );

    if ( fire_before < fire->check() && fire->at_max_stacks() )
      p()->buffs.excess_fire->trigger();
    if ( frost_before < frost->check() && frost->at_max_stacks() )
      p()->buffs.excess_frost->trigger();

    // Frostfire spells don't seem to trigger Severe Temperatures
    if ( !( is_fire && is_frost ) )
      p()->buffs.severe_temperatures->trigger();
  }

  void trigger_glorious_incandescence( player_t* t )
  {
    if ( !p()->talents.glorious_incandescence.ok() || !p()->state.trigger_glorious_incandescence )
      return;

    // TODO: Test the delay more rigorously
    p()->action.meteorite->execute_on_target( t );
    make_repeating_event( *sim, 75_ms, [ this, t ] { p()->action.meteorite->execute_on_target( t ); },
      as<int>( p()->talents.glorious_incandescence->effectN( 1 ).base_value() ) - 1 );

    p()->state.trigger_glorious_incandescence = false;
  }
};

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

  arcane_mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
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
        if ( cr == p()->buffs.clearcasting )
        {
          p()->buffs.nether_precision->trigger();
          // Technically, the buff disappears immediately when it reaches max stacks
          // and the Attunement buff is applied with a delay. Here, we just use
          // max stacks of the buff to track the delay.
          p()->buffs.aether_attunement_counter->trigger();
        }
        break;
      }
    }
  }

  double cost_pct_multiplier() const override
  {
    double c = mage_spell_t::cost_pct_multiplier();

    for ( auto cr : cost_reductions )
      c *= 1.0 + cr->check_value();

    return c;
  }

  double arcane_charge_multiplier( bool arcane_barrage = false ) const
  {
    double base = p()->buffs.arcane_charge->data().effectN( arcane_barrage ? 2 : 1 ).percent();

    double mastery = p()->cache.mastery() * p()->spec.savant->effectN( arcane_barrage ? 3 : 2 ).mastery_value();
    mastery *= 1.0 + p()->talents.prodigious_savant->effectN( arcane_barrage ? 2 : 1 ).percent();

    return 1.0 + p()->buffs.arcane_charge->check() * ( base + mastery );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( auto td = find_td( s->target ) )
    {
      buff_t* debuff;
      switch ( id )
      {
        case 7268:  debuff = td->debuffs.magis_spark_am;   break;
        case 30451: debuff = td->debuffs.magis_spark_ab;   break;
        case 44425: debuff = td->debuffs.magis_spark_abar; break;
        default:    debuff = nullptr;                      break;
      }

      bool trigger_echo = false;
      if ( debuff && debuff->check() )
      {
        trigger_echo = true;
        debuff->decrement();

        if ( ++p()->state.magis_spark_spells == 3 )
          p()->action.magis_spark->execute_on_target( s->target );
      }

      // Special handling for AM's 2 sec grace period
      // TODO: Currently bugged and only applies to the first AM hit (again).
      // Blizz also doesn't seem to be using the Magi's Spark debuff for this.
      if ( id == 7268 && td->debuffs.magis_spark->check() )
      {
        trigger_echo = true;
        td->debuffs.magis_spark->expire( p()->bugs ? 1_ms : 2.0_s );
      }

      if ( trigger_echo )
        p()->action.magis_spark_echo->execute_on_target( s->target, p()->talents.magis_spark->effectN( 1 ).percent() * s->result_total );
    }
  }

  void consume_nether_precision()
  {
    if ( !p()->buffs.nether_precision->check() )
      return;

    p()->buffs.nether_precision->decrement();
    p()->buffs.leydrinker->trigger();
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

  void execute() override
  {
    mage_spell_t::execute();

    // TODO: Flame Accelerant is currently bugged and expires after the queued spell has started casting.
    if ( affected_by.flame_accelerant && time_to_execute > 0_ms )
      make_event( *sim, 15_ms, [ this ] { p()->buffs.flame_accelerant->expire(); } );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( triggers.ignite )
        trigger_ignite( s );

      if ( tt_applicable( s, triggers.hot_streak ) )
        handle_hot_streak( s->composite_crit_chance(), s->result == RESULT_CRIT ? HS_CRIT : HS_HIT );

      if ( tt_applicable( s, triggers.unleashed_inferno ) && p()->buffs.combustion->check() )
        p()->cooldowns.combustion->adjust( -p()->talents.unleashed_inferno->effectN( 2 ).time_value() );

      if ( tt_applicable( s, triggers.kindling )
        && s->result == RESULT_CRIT
        && p()->talents.kindling.ok() )
      {
        timespan_t amount = p()->talents.kindling->effectN( 1 ).time_value();
        p()->cooldowns.combustion->adjust( -amount );
        if ( !p()->buffs.combustion->check() )
          p()->expression_support.kindling_reduction += amount;
      }

      if ( triggers.from_the_ashes && p()->talents.from_the_ashes.ok() && p()->cooldowns.from_the_ashes->up() )
      {
        p()->cooldowns.from_the_ashes->start( p()->talents.from_the_ashes->internal_cooldown() );
        p()->cooldowns.phoenix_flames->adjust( p()->talents.from_the_ashes->effectN( 1 ).time_value() );
      }
    }
  }

  void handle_hot_streak( double chance, hot_streak_trigger_type_e tt )
  {
    mage_t* p = this->p();
    if ( !p->spec.pyroblast_clearcasting_driver->ok() )
      return;

    p->procs.hot_streak_spell->occur();

    if ( tt == HS_CRIT || ( tt == HS_CUSTOM && rng().roll( chance ) ) )
    {
      bool guaranteed = chance >= 1.0;
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
            if ( p->executing->target->is_sleeping() )
            {
              make_event( *sim, [ p ] { p->interrupt(); } );
            }
            else
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
        }
        // Crit without HU => generate HU
        else
        {
          p->procs.heating_up_generated->occur();
          p->buffs.heating_up->trigger( p->buffs.heating_up->buff_duration() * p->cache.spell_cast_speed() );
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

  double execute_time_pct_multiplier() const override
  {
    double mul = mage_spell_t::execute_time_pct_multiplier();

    if ( affected_by.flame_accelerant )
      mul *= 1.0 + p()->buffs.flame_accelerant->check_value();

    return mul;
  }

  virtual double composite_ignite_multiplier( const action_state_t* s ) const
  {
    double m = 1.0;

    if ( !p()->buffs.combustion->check() )
      m *= 1.0 + p()->talents.master_of_flame->effectN( 1 ).percent();

    if ( auto td = find_td( s->target ) )
      m *= 1.0 + td->debuffs.controlled_destruction->check_stack_value();

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

    if ( p()->bugs && s->result == RESULT_CRIT )
    {
      double spell_bonus  = composite_crit_damage_bonus_multiplier() * composite_target_crit_damage_bonus_multiplier( s->target );
      double global_bonus = composite_player_critical_multiplier( s );
      trigger_dmg /= 1.0 + s->result_crit_bonus;
      trigger_dmg *= ( 1.0 + spell_bonus ) * global_bonus;
      // TODO: This calculation is incomplete because it doesn't take into
      // account crit_bonus or the pvp rules. However, in normal situations
      // it's pretty close to what happens in game.
    }

    double amount = trigger_dmg / m * p()->cache.mastery_value();
    if ( amount <= 0.0 )
      return;

    amount *= composite_ignite_multiplier( s );

    if ( !p()->action.ignite->get_dot( s->target )->is_ticking() )
      p()->procs.ignite_applied->occur();

    residual_action::trigger( p()->action.ignite, s->target, amount );
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
    if ( !p()->action.ignite )
      return;

    auto source = p()->action.ignite->get_dot( primary );
    if ( source->is_ticking() )
    {
      std::vector<dot_t*> ignites;

      // Collect the Ignite DoT objects of all targets that are in range.
      for ( auto t : target_list() )
        ignites.push_back( p()->action.ignite->get_dot( t ) );

      // Sort candidate Ignites by ascending bank size.
      std::stable_sort( ignites.begin(), ignites.end(), [] ( dot_t* a, dot_t* b )
      { return ignite_bank( a ) < ignite_bank( b ); } );

      auto source_bank = ignite_bank( source );
      auto source_tick_amount = debug_cast<residual_action::residual_periodic_state_t*>( source->state )->tick_amount;
      auto targets_remaining = as<int>( p()->spec.ignite->effectN( 4 ).base_value() );
      if ( p()->buffs.combustion->check() )
        targets_remaining += as<int>( p()->talents.master_of_flame->effectN( 2 ).base_value() );

      for ( auto destination : ignites )
      {
        // The original spread source doesn't count towards the spread target limit.
        if ( source == destination )
          continue;

        // Target cap was reached, stop.
        if ( targets_remaining-- <= 0 )
          break;

        // Source Ignite cannot spread to targets with higher Ignite bank.
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
        // This usually results in the newly spread Ignites having an incorrect bank size.
        debug_cast<residual_action::residual_periodic_state_t*>( destination->state )->tick_amount = source_tick_amount;
      }
    }
  }

  bool firestarter_active( player_t* target ) const
  {
    if ( !p()->talents.firestarter.ok() )
      return false;

    return target->health_percentage() >= p()->talents.firestarter->effectN( 1 ).base_value();
  }

  bool scorch_execute_active( player_t* target ) const
  {
    if ( !p()->talents.scorch.ok() )
      return false;

    if ( p()->buffs.heat_shimmer->check() )
      return true;

    return target->health_percentage() <= p()->talents.scorch->effectN( 2 ).base_value() + p()->talents.sunfury_execution->effectN( 2 ).base_value();
  }

  bool improved_scorch_active( player_t* target ) const
  {
    if ( !p()->talents.improved_scorch.ok() )
      return false;

    if ( p()->buffs.heat_shimmer->check() )
      return true;

    return target->health_percentage() <= p()->talents.improved_scorch->effectN( 1 ).base_value() + p()->talents.sunfury_execution->effectN( 2 ).base_value();
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

struct hot_streak_state_t final : public mage_spell_state_t
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
  action_t* pyromaniac_action;
  // Last available Hot Streak state.
  bool last_hot_streak;

  hot_streak_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    fire_mage_spell_t( n, p, s ),
    pyromaniac_action(),
    last_hot_streak()
  {
    affected_by.flame_accelerant = true;
    base_execute_time += p->talents.surging_blaze->effectN( 1 ).time_value();
    base_multiplier *= 1.0 + p->talents.surging_blaze->effectN( 2 ).percent();
  }

  action_state_t* new_state() override
  { return new hot_streak_state_t( this, target ); }

  void reset() override
  {
    fire_mage_spell_t::reset();
    last_hot_streak = false;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.hot_streak->check() || p()->buffs.hyperthermia->check() )
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

    return c;
  }

  double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( time_to_execute > 0_ms && !p()->buffs.hyperthermia->check() )
      am *= 1.0 + p()->buffs.fury_of_the_sun_king->check_value();

    return am;
  }

  double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_ignite_multiplier( s );

    if ( debug_cast<const hot_streak_state_t*>( s )->hot_streak )
    {
      m *= 2.0; // base Hot Streak! multiplier (not in spell data)
      m *= 1.0 + p()->talents.inflame->effectN( 1 ).percent();
    }

    return m;
  }

  void schedule_execute( action_state_t* s ) override
  {
    fire_mage_spell_t::schedule_execute( s );
    last_hot_streak = p()->buffs.hot_streak->up();
  }

  void execute() override
  {
    bool expire_skb = false;

    if ( time_to_execute > 0_ms && !p()->buffs.hyperthermia->check() && p()->buffs.fury_of_the_sun_king->check() )
    {
      expire_skb = true;
      // Extending Combustion doesn't trigger Frostfire Empowerment
      if ( !p()->buffs.combustion->check() )
        p()->trigger_flash_freezeburn();
      p()->buffs.combustion->extend_duration_or_trigger( 1000 * p()->talents.sun_kings_blessing->effectN( 2 ).time_value() );
    }

    if ( last_hot_streak )
    {
      // For Fire, Spellfire Spheres are triggered before the spell actually casts.
      p()->trigger_spellfire_spheres();
    }

    fire_mage_spell_t::execute();

    p()->buffs.sparking_cinders->decrement();

    if ( expire_skb )
      p()->buffs.fury_of_the_sun_king->expire();

    p()->consume_burden_of_power();

    if ( last_hot_streak )
    {
      p()->buffs.hot_streak->decrement();

      if ( !p()->buffs.combustion->check() )
        p()->buffs.hyperthermia->trigger();

      trigger_tracking_buff( p()->buffs.sun_kings_blessing, p()->buffs.fury_of_the_sun_king );
      p()->trigger_lit_fuse();
      p()->trigger_mana_cascade();

      // TODO: Test the proc chance and whether this works with Hyperthermia and Lit Fuse.
      if ( p()->cooldowns.pyromaniac->up() && p()->accumulated_rng.pyromaniac->trigger() )
      {
        p()->cooldowns.pyromaniac->start( p()->talents.pyromaniac->internal_cooldown() );

        trigger_tracking_buff( p()->buffs.sun_kings_blessing, p()->buffs.fury_of_the_sun_king );
        p()->trigger_lit_fuse();
        p()->trigger_spellfire_spheres();
        p()->trigger_mana_cascade();

        assert( pyromaniac_action );
        // Pyromaniac Pyroblast actually casts on the Mage's target, but that is probably a bug.
        make_event( *sim, 500_ms, [ this, t = target ]
        {
          pyromaniac_action->execute_on_target( t );
          p()->buffs.sparking_cinders->decrement();
        } );
      }
    }
  }
};


// ==========================================================================
// Frost Mage Spell
// ==========================================================================

struct frost_mage_spell_t : public mage_spell_t
{
  bool consumes_winters_chill;

  proc_t* proc_brain_freeze;
  proc_t* proc_fof;
  proc_t* proc_winters_chill_consumed;

  bool track_shatter;
  shatter_source_t* shatter_source;

  frost_mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    consumes_winters_chill(),
    proc_brain_freeze(),
    proc_fof(),
    proc_winters_chill_consumed(),
    track_shatter(),
    shatter_source()
  { }

  void init_finished() override
  {
    if ( consumes_winters_chill )
    {
      proc_winters_chill_consumed = p()->get_proc( fmt::format( "Winter's Chill stacks consumed by {}", data().name_cstr() ) );
      assert( !range::contains( p()->winters_chill_consumers, this ) );
      p()->winters_chill_consumers.push_back( this );
    }

    if ( track_shatter && sim->report_details != 0 )
      shatter_source = p()->get_shatter_source( name_str );

    mage_spell_t::init_finished();
  }

  double icicle_sp_coefficient() const
  {
    return p()->cache.mastery() * p()->spec.icicles->effectN( 3 ).sp_coeff();
  }

  void record_shatter_source( const action_state_t* s, shatter_source_t* source )
  {
    if ( !source )
      return;

    unsigned frozen = cast_state( s )->frozen;

    if ( frozen & FF_WINTERS_CHILL )
      source->occur( FROZEN_WINTERS_CHILL );
    else if ( frozen & FF_ROOT )
      source->occur( FROZEN_ROOT );
    else if ( frozen & FF_FINGERS_OF_FROST )
      source->occur( FROZEN_FINGERS_OF_FROST );
    else
      source->occur( FROZEN_NONE );
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( !background && consumes_winters_chill )
      p()->expression_support.remaining_winters_chill = std::max( p()->expression_support.remaining_winters_chill - 1, 0 );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( s->chain_target == 0 )
        record_shatter_source( s, shatter_source );

      if ( triggers.chill )
        trigger_chill_effect( s );

      if ( auto td = find_td( s->target ) )
      {
        if ( consumes_winters_chill && td->debuffs.winters_chill->check() )
        {
          td->debuffs.winters_chill->decrement();
          p()->trigger_splinter( p()->target );

          proc_winters_chill_consumed->occur();
          p()->procs.winters_chill_consumed->occur();
        }
      }
    }
  }

  void trigger_chill_effect( const action_state_t* s )
  {
    p()->trigger_merged_buff( p()->buffs.bone_chilling, true );
    if ( p()->rng().roll( p()->talents.frostbite->proc_chance() ) )
      p()->trigger_crowd_control( s, MECHANIC_FREEZE, -0.5_s ); // Frostbite only has the initial grace period
  }

  void trigger_cold_front( int stacks = 1 )
  {
    trigger_tracking_buff( p()->buffs.cold_front, p()->buffs.cold_front_ready, 2, stacks );
  }

  // Behavior for various initial states and trigger spells (format: spell, initial state -> final state, ...)
  // Flurry,     26 -> 27, 27 -> 28, 28 -> 0,     ready -> 1
  // Frostbolt1, 26 -> 27, 27 -> 28, 28 -> ready, ready -> 1
  // Frostbolt2, 26 -> 28, 27 -> 0,  28 -> 1,     ready -> 2
  // Frostbolt3, 26 -> 0,  27 -> 0,  28 -> 1,     ready -> 3
  bool consume_cold_front( player_t* target )
  {
    if ( !p()->buffs.cold_front_ready->check() )
      return false;
    p()->buffs.cold_front_ready->expire();
    p()->action.cold_front_frozen_orb->execute_on_target( target );
    return true;
  }
};

struct icicle_t final : public frost_mage_spell_t
{
  icicle_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 148022 ) )
  {
    background = track_shatter = true;
    callbacks = false;
    base_dd_min = base_dd_max = 1.0;
    base_multiplier *= 1.0 + p->talents.flash_freeze->effectN( 2 ).percent();
    crit_bonus_multiplier *= 1.0 + p->talents.piercing_cold->effectN( 1 ).percent();

    if ( p->talents.splitting_ice.ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_multiplier *= 1.0 + p->talents.splitting_ice->effectN( 3 ).percent();
      base_aoe_multiplier *= data().effectN( 1 ).chain_multiplier();
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

struct intensifying_flame_t final : public spell_t
{
  intensifying_flame_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 419800 ) )
  {
    background = true;
    base_dd_min = base_dd_max = 1.0;
  }

  void init() override
  {
    spell_t::init();

    // Despite only being flagged with Ignore Positive Damage Taken Modifiers (321),
    // this spell does not appear to double dip negative damage taken multipliers.
    snapshot_flags &= STATE_NO_MULTIPLIER;
  }
};

struct ignite_t final : public residual_action::residual_periodic_action_t<spell_t>
{
  action_t* intensifying_flame = nullptr;

  ignite_t( std::string_view n, mage_t* p ) :
    residual_action_t( n, p, p->find_spell( 12654 ) )
  {
    if ( p->talents.intensifying_flame.ok() )
      intensifying_flame = get_action<intensifying_flame_t>( "intensifying_flame", p );
  }

  void tick( dot_t* d ) override
  {
    residual_action_t::tick( d );

    auto p = debug_cast<mage_t*>( player );
    p->buffs.heat_shimmer->trigger();

    if ( p->get_active_dots( d ) <= p->talents.intensifying_flame->effectN( 1 ).base_value() )
    {
      // 2024-05-19: Intensifying Flames deals a percentage of Ignite's base tick damage and not the damage it actually ticked for.
      double tick_amount = p->bugs ? base_ta( d->state ) : d->state->result_total;
      intensifying_flame->execute_on_target( d->target, p->talents.intensifying_flame->effectN( 2 ).percent() * tick_amount );
    }
  }
};

struct arcane_orb_bolt_t final : public arcane_mage_spell_t
{
  arcane_orb_bolt_t( std::string_view n, mage_t* p, ao_type type ) :
    arcane_mage_spell_t( n, p, p->find_spell( type == ao_type::SPELLFROST ? 463357 : 153640 ) )
  {
    background = true;
    affected_by.savant = true;
    base_multiplier *= 1.0 + p->talents.resonant_orbs->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->talents.splintering_orbs->effectN( 3 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    // AC is triggered even if the spell misses.
    p()->trigger_arcane_charge();

    if ( result_is_hit( s->result ) && p()->talents.controlled_instincts.ok() )
      get_td( s->target )->debuffs.controlled_instincts->trigger();
  }
};

struct arcane_orb_t final : public arcane_mage_spell_t
{
  arcane_orb_t( std::string_view n, mage_t* p, std::string_view options_str, ao_type type = ao_type::NORMAL ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Orb" ) )
  {
    parse_options( options_str );
    may_miss = false;
    aoe = -1;
    cooldown->charges += as<int>( p->talents.charged_orb->effectN( 1 ).base_value() );
    triggers.clearcasting = type == ao_type::NORMAL;

    std::string_view bolt_name;
    switch ( type )
    {
      case ao_type::NORMAL:
        bolt_name = "arcane_orb_bolt";
        break;
      case ao_type::ORB_BARRAGE:
        bolt_name = "orb_barrage_arcane_orb_bolt";
        break;
      case ao_type::SPELLFROST:
        bolt_name = "spellfrost_arcane_orb_bolt";
        break;
      default:
        assert( false );
        break;
    }

    impact_action = get_action<arcane_orb_bolt_t>( bolt_name, p, type );
    add_child( impact_action );

    if ( type != ao_type::NORMAL )
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

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    // TODO: AO still seems to give 4 splinters, but needs to hit 2 targets now
    if ( s->chain_target == 0 || ( p()->bugs && s->chain_target == 1 ) )
      p()->trigger_splinter( s->target, as<int>( p()->talents.splintering_orbs->effectN( 4 ).base_value() ) );
  }
};

struct arcane_barrage_t final : public arcane_mage_spell_t
{
  action_t* orb_barrage = nullptr;
  int snapshot_charges = -1;
  int glorious_incandescence_charges = 0;
  int arcane_soul_charges = 0;
  int intuition_charges = 0;

  arcane_barrage_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Barrage" ) )
  {
    parse_options( options_str );
    base_aoe_multiplier *= data().effectN( 2 ).percent();
    affected_by.arcane_debilitation = true;
    triggers.overflowing_energy = triggers.clearcasting = true;
    base_multiplier *= 1.0 + p->sets->set( MAGE_ARCANE, TWW1, B2 )->effectN( 1 ).percent();
    glorious_incandescence_charges = as<int>( p->find_spell( 451223 )->effectN( 1 ).base_value() );
    arcane_soul_charges = as<int>( p->find_spell( 453413 )->effectN( 1 ).base_value() );
    intuition_charges = as<int>( p->find_spell( 455683 )->effectN( 1 ).base_value() );

    if ( p->talents.orb_barrage.ok() )
    {
      orb_barrage = get_action<arcane_orb_t>( "orb_barrage_arcane_orb", p, "", ao_type::ORB_BARRAGE );
      add_child( orb_barrage );
    }
  }

  int n_targets() const override
  {
    int charges = snapshot_charges != -1 ? snapshot_charges : p()->buffs.arcane_charge->check();
    return p()->talents.arcing_cleave.ok() && charges > 0 ? charges + 1 : 0;
  }

  void execute() override
  {
    // Arcane Orb from Orb Barrage executes before Arcane Barrage does. The extra
    // Arcane Charge from the Orb cast increases Barrage damage, but does not change
    // how many targets it hits. Snapshot the buff stacks before executing the Orb.
    snapshot_charges = p()->buffs.arcane_charge->check();
    if ( rng().roll( snapshot_charges * p()->talents.orb_barrage->effectN( 1 ).percent() ) )
      orb_barrage->execute_on_target( target );

    p()->benefits.arcane_charge.arcane_barrage->update();

    arcane_mage_spell_t::execute();

    double mana_pct = p()->buffs.arcane_charge->check() * 0.01 * p()->spec.mana_adept->effectN( 1 ).percent();
    p()->resource_gain( RESOURCE_MANA, p()->resources.max[ RESOURCE_MANA ] * mana_pct, p()->gains.arcane_barrage, this );

    p()->buffs.arcane_tempo->trigger();
    p()->buffs.arcane_charge->expire();
    p()->buffs.arcane_harmony->expire();

    if ( p()->buffs.arcane_soul->check() )
    {
      p()->trigger_clearcasting( 1.0, 0_ms );
      p()->trigger_arcane_charge( arcane_soul_charges );
    }

    if ( p()->buffs.nether_precision->check() )
    {
      consume_nether_precision();
      if ( p()->talents.dematerialize.ok() )
        p()->state.trigger_dematerialize = true;
      p()->trigger_splinter( target );
    }

    if ( p()->buffs.leydrinker->check() )
    {
      p()->buffs.leydrinker->decrement();
      p()->state.trigger_leydrinker = true;
    }

    p()->consume_burden_of_power();
    p()->trigger_spellfire_spheres();
    p()->trigger_mana_cascade();

    if ( p()->buffs.glorious_incandescence->check() )
    {
      p()->buffs.glorious_incandescence->decrement();
      p()->trigger_arcane_charge( glorious_incandescence_charges );
      p()->state.trigger_glorious_incandescence = true;
    }

    if ( p()->buffs.intuition->check() )
    {
      p()->buffs.intuition->decrement();
      p()->trigger_arcane_charge( intuition_charges );
    }
    p()->buffs.intuition->trigger();

    snapshot_charges = -1;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = arcane_mage_spell_t::composite_da_multiplier( s );

    m *= 1.0 + s->n_targets * p()->talents.resonance->effectN( 1 ).percent();

    if ( s->target->health_percentage() <= p()->talents.arcane_bombardment->effectN( 1 ).base_value() )
      m *= 1.0 + p()->talents.arcane_bombardment->effectN( 2 ).percent() + p()->talents.sunfury_execution->effectN( 1 ).percent();

    if ( p()->buffs.burden_of_power->check() )
      m *= 1.0 + p()->buffs.burden_of_power->data().effectN( 4 ).percent();

    return m;
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_multiplier( true );
    am *= 1.0 + p()->buffs.arcane_harmony->check_stack_value();
    am *= 1.0 + p()->buffs.nether_precision->check_value();
    am *= 1.0 + p()->buffs.intuition->check_value();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->state.trigger_dematerialize )
    {
      p()->state.trigger_dematerialize = false;
      residual_action::trigger( p()->action.dematerialize, s->target, p()->talents.dematerialize->effectN( 1 ).percent() * s->result_total );
    }

    if ( p()->state.trigger_leydrinker )
    {
      p()->state.trigger_leydrinker = false;
      p()->action.leydrinker_echo->execute_on_target( s->target, p()->talents.leydrinker->effectN( 2 ).percent() * s->result_total );
    }

    trigger_glorious_incandescence( s->target );
  }
};

struct arcane_blast_t final : public arcane_mage_spell_t
{
  arcane_blast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Blast" ) )
  {
    parse_options( options_str );
    affected_by.arcane_debilitation = true;
    triggers.overflowing_energy = triggers.clearcasting = true;
    base_multiplier *= 1.0 + p->talents.consortiums_bauble->effectN( 2 ).percent();
    base_multiplier *= 1.0 + p->sets->set( MAGE_ARCANE, TWW1, B2 )->effectN( 1 ).percent();
    base_costs[ RESOURCE_MANA ] *= 1.0 + p->talents.consortiums_bauble->effectN( 1 ).percent();
    cost_reductions = { p->buffs.concentration };
  }

  timespan_t travel_time() const override
  {
    // Add a small amount of travel time so that Arcane Blast's damage can be stored
    // in a Touch of the Magi cast immediately afterwards. Because simc has a default
    // sim_t::queue_delay of 5_ms, this needs to be consistently longer than that.
    return std::max( arcane_mage_spell_t::travel_time(), 6_ms );
  }

  double cost_pct_multiplier() const override
  {
    double c = arcane_mage_spell_t::cost_pct_multiplier();

    c *= 1.0 + p()->buffs.arcane_charge->check() * p()->buffs.arcane_charge->data().effectN( 5 ).percent();

    return c;
  }

  void execute() override
  {
    p()->benefits.arcane_charge.arcane_blast->update();

    arcane_mage_spell_t::execute();

    p()->consume_burden_of_power();
    p()->trigger_arcane_charge();
    p()->trigger_spellfire_spheres();
    p()->trigger_mana_cascade();

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

    if ( p()->buffs.nether_precision->check() )
    {
      // Nether Precision is slightly delayed, allowing two spells to benefit from the
      // last stack. Technically, the delay should be on Arcane Barrage as well, but
      // because it's an instant, it cannot be taken advantage of.
      // TODO: Check if AB -> PoM AB works (with low latency).
      make_event( *sim, 15_ms, [ this ] { consume_nether_precision(); } );
      if ( p()->talents.dematerialize.ok() )
        p()->state.trigger_dematerialize = true;
      p()->trigger_splinter( target );
    }

    if ( p()->buffs.leydrinker->check() )
    {
      p()->buffs.leydrinker->decrement();
      p()->state.trigger_leydrinker = true;
    }

    p()->buffs.intuition->trigger();
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_multiplier();
    am *= 1.0 + p()->buffs.nether_precision->check_value();

    return am;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = arcane_mage_spell_t::composite_da_multiplier( s );

    if ( p()->buffs.burden_of_power->check() )
      m *= 1.0 + p()->buffs.burden_of_power->data().effectN( 2 ).percent();

    return m;
  }

  double execute_time_pct_multiplier() const override
  {
    if ( p()->buffs.presence_of_mind->check() )
      return 0.0;

    double mul = arcane_mage_spell_t::execute_time_pct_multiplier();

    mul *= 1.0 + p()->buffs.arcane_charge->check() * p()->buffs.arcane_charge->data().effectN( 4 ).percent();

    return mul;
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->state.trigger_dematerialize )
    {
      p()->state.trigger_dematerialize = false;
      residual_action::trigger( p()->action.dematerialize, s->target, p()->talents.dematerialize->effectN( 1 ).percent() * s->result_total );
    }

    if ( p()->state.trigger_leydrinker )
    {
      p()->state.trigger_leydrinker = false;
      p()->action.leydrinker_echo->execute_on_target( s->target, p()->talents.leydrinker->effectN( 2 ).percent() * s->result_total );
    }
  }
};

struct arcane_explosion_t final : public arcane_mage_spell_t
{
  static const spell_data_t* spell( mage_t* p, ae_type type )
  {
    switch ( type )
    {
      case ae_type::NORMAL:       return p->find_class_spell( "Arcane Explosion" );
      case ae_type::ECHO:         return p->find_spell( 414381 );
      case ae_type::ENERGY_RECON: return p->find_spell( 461508 );
      default:                    return nullptr;
    }
  }

  action_t* echo = nullptr;
  const ae_type type;

  arcane_explosion_t( std::string_view n, mage_t* p, std::string_view options_str, ae_type type_ = ae_type::NORMAL ) :
    arcane_mage_spell_t( n, p, spell( p, type_ ) ),
    type( type_ )
  {
    parse_options( options_str );
    aoe = -1;
    affected_by.savant = true;
    triggers.clearcasting = type != ae_type::ENERGY_RECON;

    if ( type == ae_type::NORMAL )
    {
      cost_reductions = { p->buffs.clearcasting };
      if ( p->talents.concentrated_power.ok() )
      {
        echo = get_action<arcane_explosion_t>( "arcane_explosion_echo", p, "", ae_type::ECHO );
        add_child( echo );
      }
    }
    else
    {
      background = true;
    }
  }

  void execute() override
  {
    if ( echo && p()->buffs.clearcasting->check() )
      make_event( *sim, 500_ms, [ this, t = target ] { echo->execute_on_target( t ); } );

    arcane_mage_spell_t::execute();

    if ( p()->buffs.static_cloud->at_max_stacks() )
      p()->buffs.static_cloud->expire();
    p()->buffs.static_cloud->trigger();

    if ( type == ae_type::ENERGY_RECON )
      return;

    if ( !target_list().empty() )
      p()->trigger_arcane_charge();

    if ( num_targets_hit >= as<int>( p()->talents.reverberate->effectN( 2 ).base_value() )
      && rng().roll( p()->talents.reverberate->effectN( 1 ).percent() ) )
    {
      p()->trigger_arcane_charge();
    }

    if ( !background && p()->buffs.aether_attunement_counter->at_max_stacks() )
    {
      p()->buffs.aether_attunement_counter->expire();
      p()->buffs.aether_attunement->trigger();
    }
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.static_cloud->check_stack_value();

    // Seems to affect the echo as well, but only if the mage has CC at that time.
    if ( p()->buffs.clearcasting->check() && type != ae_type::ENERGY_RECON )
      am *= 1.0 + p()->talents.eureka->effectN( 1 ).percent();

    return am;
  }
};

struct arcane_assault_t final : public arcane_mage_spell_t
{
  double energize_pct;

  arcane_assault_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 225119 ) ),
    energize_pct( p->find_spell( 454020 )->effectN( 1 ).percent() )
  {
    background = true;
    callbacks = false;
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    // TODO: Proc rate isn't listed anywhere, update as we get more data
    if ( p()->talents.energized_familiar.ok() && rng().roll( 0.05 ) )
      p()->resource_gain( RESOURCE_MANA, p()->resources.max[ RESOURCE_MANA ] * energize_pct, p()->gains.energized_familiar, this );
  }
};

struct arcane_intellect_t final : public mage_spell_t
{
  arcane_intellect_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Arcane Intellect" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;

    if ( sim->overrides.arcane_intellect && !p->talents.arcane_familiar.ok() )
      background = true;
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( !sim->overrides.arcane_intellect )
      sim->auras.arcane_intellect->trigger();

    p()->buffs.arcane_familiar->trigger();
  }
};

struct am_state_t final : public mage_spell_state_t
{
  double tick_time_multiplier;
  int targets;

  am_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    tick_time_multiplier( 1.0 ),
    targets( 0 )
  { }

  void initialize() override
  {
    mage_spell_state_t::initialize();
    tick_time_multiplier = 1.0;
    targets = 0;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s ) << " tick_time_multiplier=" << tick_time_multiplier << " targets=" << targets;
    return s;
  }

  void copy_state( const action_state_t* s ) override
  {
    mage_spell_state_t::copy_state( s );

    auto ams = debug_cast<const am_state_t*>( s );
    tick_time_multiplier = ams->tick_time_multiplier;
    targets              = ams->targets;
  }
};

struct arcane_missiles_tick_t final : public arcane_mage_spell_t
{
  arcane_missiles_tick_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 7268 ) )
  {
    background = true;
    affected_by.savant = affected_by.arcane_debilitation = triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->talents.eureka->effectN( 1 ).percent();

    const auto& aa = p->buffs.aether_attunement->data();
    base_aoe_multiplier *= ( 1.0 + aa.effectN( 4 ).percent() ) / ( 1.0 + aa.effectN( 1 ).percent() );
  }

  action_state_t* new_state() override
  { return new am_state_t( this, target ); }

  int n_targets() const override
  {
    // If pre_execute_state exists, we need to respect the n_targets amount
    // as it existed when the state was updated.
    if ( pre_execute_state )
      return debug_cast<am_state_t*>( pre_execute_state )->targets;

    return p()->buffs.aether_attunement->check()
      ? as<int>( p()->buffs.aether_attunement->data().effectN( 2 ).base_value() )
      : arcane_mage_spell_t::n_targets();
  }

  void update_state( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    arcane_mage_spell_t::update_state( s, flags, rt );
    debug_cast<am_state_t*>( s )->targets = n_targets();
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.arcane_harmony->trigger();

      if ( p()->talents.arcane_debilitation.ok() )
      {
        auto debuff = get_td( s->target )->debuffs.arcane_debilitation;
        debuff->trigger();
        while ( rng().roll( p()->talents.time_loop->effectN( 1 ).percent() ) )
          debuff->trigger();
      }

      if ( p()->talents.high_voltage.ok() )
      {
        double chance = p()->talents.high_voltage->effectN( 1 ).percent();
        chance += p()->buffs.high_voltage->check_stack_value();
        if ( rng().roll( chance ) )
        {
          p()->trigger_arcane_charge();
          p()->buffs.high_voltage->expire();
        }
        else
        {
          p()->buffs.high_voltage->trigger();
        }
      }
    }
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.aether_attunement->check_value();

    return am;
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
    triggers.clearcasting = true;
    tick_action = get_action<arcane_missiles_tick_t>( "arcane_missiles_tick", p );
    cost_reductions = { p->buffs.clearcasting };

    // TODO (10.1.5 PTR): the tick time reduction is in CC while the duration reduction
    // is in Concentrated Power, which doesn't make sense and will presumably be fixed
    const auto& cc_data = p->buffs.clearcasting_channel->data();
    cc_duration_reduction  = cc_data.effectN( 1 ).percent() + p->talents.concentrated_power->effectN( 2 ).percent();
    cc_tick_time_reduction = cc_data.effectN( 2 ).percent() + p->talents.amplification->effectN( 1 ).percent();
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }

  action_state_t* new_state() override
  { return new am_state_t( this, target ); }

  // We need to snapshot any tick time reduction effect here so that it correctly affects the whole channel.
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

  double tick_time_pct_multiplier( const action_state_t* s ) const override
  {
    auto mul = arcane_mage_spell_t::tick_time_pct_multiplier( s );

    mul *= debug_cast<const am_state_t*>( s )->tick_time_multiplier;

    return mul;
  }

  void channel_finish()
  {
    p()->buffs.clearcasting_channel->expire();
    p()->buffs.aether_attunement->expire();

    if ( p()->buffs.aether_attunement_counter->at_max_stacks() )
    {
      p()->buffs.aether_attunement_counter->expire();
      p()->buffs.aether_attunement->trigger();
    }
  }

  bool ready() override
  {
    if ( !p()->buffs.clearcasting->check() )
      return false;

    return arcane_mage_spell_t::ready();
  }

  void execute() override
  {
    if ( get_dot( target )->is_ticking() )
      channel_finish();

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
      timespan_t delay = rng().gauss_ab( mean_delay, mean_delay * p()->options.arcane_missiles_chain_relstddev, 0_ms, tick_remains - 1_ms );
      timespan_t chain_remains = tick_remains - delay;
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

    d->adjust_duration( new_remains - d->remains() );
  }

  bool usable_moving() const override
  {
    if ( p()->talents.slipstream.ok() )
      return true;

    return arcane_mage_spell_t::usable_moving();
  }

  void last_tick( dot_t* d ) override
  {
    arcane_mage_spell_t::last_tick( d );
    channel_finish();
  }
};

struct arcane_surge_t final : public arcane_mage_spell_t
{
  double energize_pct;

  arcane_surge_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.arcane_surge ),
    energize_pct( p->find_spell( 365265 )->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    aoe = -1;
    affected_by.savant = true;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
  }

  timespan_t travel_time() const override
  {
    // Add a small amount of travel time so that Arcane Surge's damage can be stored
    // in a Touch of the Magi cast immediately afterwards. Because simc has a default
    // sim_t::queue_delay of 5_ms, this needs to be consistently longer than that.
    return std::max( arcane_mage_spell_t::travel_time(), 6_ms );
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->resources.pct( RESOURCE_MANA ) * ( data().effectN( 2 ).base_value() - 1.0 );

    if ( p()->talents.surging_urge.ok() )
    {
      double per_charge = p()->talents.surging_urge->effectN( 2 ).percent() / p()->talents.surging_urge->effectN( 1 ).base_value();
      am *= 1.0 + p()->buffs.arcane_charge->check() * per_charge;
    }

    return am;
  }

  void execute() override
  {
    p()->trigger_splinter( target, as<int>( p()->talents.augury_abounds->effectN( 1 ).base_value() ) );

    // Clear any existing surge buffs to trigger the T30 4pc buff.
    p()->buffs.arcane_surge->expire();
    timespan_t bonus_duration = p()->buffs.spellfire_sphere->check() * p()->talents.savor_the_moment->effectN( 3 ).time_value();
    timespan_t arcane_surge_duration = p()->buffs.arcane_surge->buff_duration() + bonus_duration;
    p()->buffs.arcane_surge->trigger( arcane_surge_duration );

    p()->trigger_clearcasting( 1.0, 0_ms );

    if ( p()->pets.arcane_phoenix )
      p()->pets.arcane_phoenix->summon( arcane_surge_duration ); // TODO: The extra random pet duration can sometimes result in an extra cast.

    arcane_mage_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( s->chain_target == 0 )
      p()->resource_gain( RESOURCE_MANA, p()->resources.max[ RESOURCE_MANA ] * energize_pct, p()->gains.arcane_surge, this );
  }
};

struct blast_wave_t final : public fire_mage_spell_t
{
  blast_wave_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.blast_wave )
  {
    parse_options( options_str );
    aoe = -1;
    cooldown->duration += p->talents.volatile_detonation->effectN( 1 ).time_value();
  }
};

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
    cooldown->duration += p->talents.flow_of_time->effectN( 1 ).time_value();

    if ( p->talents.shimmer.ok() )
      background = true;
  }
};

struct blizzard_shard_t final : public frost_mage_spell_t
{
  blizzard_shard_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 190357 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 8;
    background = ground_aoe = triggers.chill = true;
    affected_by.icicles_aoe = true;
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

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.freezing_rain->check_value();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) && p()->talents.controlled_instincts.ok() )
      get_td( s->target )->debuffs.controlled_instincts->trigger();
  }
};

struct blizzard_t final : public frost_mage_spell_t
{
  action_t* blizzard_shard;

  blizzard_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Blizzard" ) ),
    blizzard_shard( get_action<blizzard_shard_t>( "blizzard_shard", p ) )
  {
    parse_options( options_str );
    add_child( blizzard_shard );
    cooldown->hasted = true;
    may_miss = affected_by.shatter = false;
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

    timespan_t ground_aoe_duration = data().duration() * player->cache.spell_cast_speed();
    p()->ground_aoe_expiration[ AOE_BLIZZARD ] = sim->current_time() + ground_aoe_duration;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( target )
      .duration( ground_aoe_duration )
      .action( blizzard_shard )
      .hasted( ground_aoe_params_t::SPELL_CAST_SPEED ), true );
  }
};

struct cold_snap_t final : public frost_mage_spell_t
{
  cold_snap_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_specialization_spell( "Cold Snap" ) )
  {
    parse_options( options_str );
    harmful = false;
  };

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->cooldowns.frost_nova->reset( false );
    if ( !p()->talents.coldest_snap.ok() )
      p()->cooldowns.cone_of_cold->reset( false );

    if ( p()->talents.flame_and_frost.ok() )
    {
      p()->cooldowns.blast_wave->reset( false );
      p()->cooldowns.dragons_breath->reset( false );
      p()->cooldowns.fire_blast->reset( false );
    }
  }
};

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

    timespan_t bonus_duration = p()->buffs.spellfire_sphere->check() * p()->talents.savor_the_moment->effectN( 1 ).time_value();
    timespan_t combustion_duration = p()->buffs.combustion->buff_duration() + bonus_duration;
    p()->buffs.combustion->trigger( combustion_duration );
    p()->buffs.wildfire->trigger();
    p()->cooldowns.fire_blast->reset( false, as<int>( p()->talents.spontaneous_combustion->effectN( 1 ).base_value() ) );
    p()->cooldowns.phoenix_flames->reset( false, as<int>( p()->talents.spontaneous_combustion->effectN( 2 ).base_value() ) );
    p()->trigger_flash_freezeburn();
    if ( p()->talents.explosivo.ok() )
    {
      p()->buffs.lit_fuse->trigger();
      p()->buffs.lit_fuse->predict();
    }
    if ( p()->pets.arcane_phoenix )
      p()->pets.arcane_phoenix->summon( combustion_duration ); // TODO: The extra random pet duration can sometimes result in an extra cast.

    p()->expression_support.kindling_reduction = 0_ms;
  }
};

struct comet_storm_projectile_t final : public frost_mage_spell_t
{
  comet_storm_projectile_t( std::string_view n, mage_t* p, bool isothermic_ = false ) :
    frost_mage_spell_t( n, p, p->find_spell( isothermic_ ? 438609 : 153596 ) )
  {
    aoe = -1;
    background = true;
    affected_by.icicles_aoe = true;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) && p()->talents.glacial_assault.ok() )
      get_td( s->target )->debuffs.numbing_blast->trigger();
  }
};

struct comet_storm_t final : public frost_mage_spell_t
{
  static constexpr int pulse_count = 7;
  static constexpr timespan_t pulse_time = 0.2_s;

  action_t* projectile;
  const bool isothermic;

  comet_storm_t( std::string_view n, mage_t* p, std::string_view options_str, bool isothermic_ = false ) :
    frost_mage_spell_t( n, p, isothermic_ ? p->find_spell( 153595 ) : p->talents.comet_storm ),
    projectile( get_action<comet_storm_projectile_t>( isothermic_ ? "isothermic_comet_storm_projectile" : "comet_storm_projectile", p, isothermic_ ) ),
    isothermic( isothermic_ )
  {
    parse_options( options_str );
    may_miss = affected_by.shatter = false;
    add_child( projectile );
    travel_delay = p->find_spell( 228601 )->missile_speed();

    if ( isothermic )
    {
      background = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
    }
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

    if ( !p()->bugs && isothermic )
      trigger_frostfire_mastery();
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    if ( p()->action.isothermic_meteor )
      p()->action.isothermic_meteor->execute_on_target( target );
  }
};

struct cone_of_cold_t final : public frost_mage_spell_t
{
  cone_of_cold_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_class_spell( "Cone of Cold" ) )
  {
    parse_options( options_str );
    aoe = -1;
    consumes_winters_chill = true;
    triggers.chill = !p->talents.freezing_cold.ok();
    affected_by.time_manipulation = p->talents.freezing_cold.ok() && !p->talents.coldest_snap.ok();
    affected_by.shifting_power = !p->talents.coldest_snap.ok();
    base_multiplier *= 1.0 + p->spec.frost_mage->effectN( 10 ).percent();
    cooldown->duration += p->talents.coldest_snap->effectN( 1 ).time_value();
    // Since impact needs to know how many targets were actually hit, we
    // delay all impact events by 1 ms so that they occur after execute is done.
    travel_delay = 0.001;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    if ( hit_any_target )
    {
      if ( p()->talents.coldest_snap.ok() && num_targets_hit >= as<int>( p()->talents.coldest_snap->effectN( 3 ).base_value() ) )
      {
        p()->cooldowns.comet_storm->reset( false );
        p()->cooldowns.frozen_orb->reset( false );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    if ( p()->talents.coldest_snap.ok() && num_targets_hit >= as<int>( p()->talents.coldest_snap->effectN( 3 ).base_value() ) )
      trigger_winters_chill( s );

    frost_mage_spell_t::impact( s );

    if ( p()->talents.freezing_cold.ok() )
      p()->trigger_crowd_control( s, MECHANIC_FREEZE, -0.5_s ); // Freezing Cold only has the initial grace period
  }
};

struct counterspell_t final : public mage_spell_t
{
  counterspell_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Counterspell" ) )
  {
    parse_options( options_str );
    ignore_false_positive = is_interrupt = true;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    bool success = p()->trigger_crowd_control( s, MECHANIC_INTERRUPT );
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

struct dragons_breath_t final : public fire_mage_spell_t
{
  dragons_breath_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.dragons_breath )
  {
    parse_options( options_str );
    aoe = -1;
    triggers.from_the_ashes = affected_by.time_manipulation = true;
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

struct evocation_t final : public arcane_mage_spell_t
{
  evocation_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.evocation )
  {
    parse_options( options_str );
    channeled = ignore_false_positive = tick_zero = true;
    harmful = false;
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
      mana_regen_multiplier /= p()->cache.spell_cast_speed();

      p()->buffs.evocation->trigger( 1, mana_regen_multiplier - 1.0, -1.0, composite_dot_duration( s ) );
    }
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    p()->trigger_clearcasting( 1.0, 0_ms );
    p()->trigger_arcane_charge();

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

struct fireball_t final : public fire_mage_spell_t
{
  const bool frostfire;

  fireball_t( std::string_view n, mage_t* p, std::string_view options_str, bool frostfire_ = false ) :
    fire_mage_spell_t( n, p, frostfire_ ? p->talents.frostfire_bolt : p->find_specialization_spell( "Fireball" ) ),
    frostfire( frostfire_ )
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.kindling = TT_ALL_TARGETS;
    triggers.calefaction = triggers.unleashed_inferno = TT_MAIN_TARGET;
    triggers.ignite = triggers.from_the_ashes = triggers.overflowing_energy = true;
    affected_by.unleashed_inferno = affected_by.flame_accelerant = true;

    if ( frostfire )
    {
      base_execute_time *= 1.0 + p->talents.thermal_conditioning->effectN( 1 ).percent();
      base_dd_multiplier *= 1.0 + p->spec.fire_mage->effectN( 6 ).percent();
      enable_calculate_on_impact( 468655 );
    }
  }

  timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    // TODO: Frostfire Bolt currently doesn't respect the max travel time
    return frostfire && p()->bugs ? t : std::min( t, 0.75_s );
  }

  timespan_t execute_time() const override
  {
    if ( frostfire && p()->buffs.frostfire_empowerment->check() )
      return 0_ms;

    return fire_mage_spell_t::execute_time();
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    if ( frostfire )
    {
      if ( p()->buffs.frostfire_empowerment->check() )
      {
        // Buff is decremented with a short delay, allowing two spells to benefit.
        // TODO: Double check this later
        make_event( *sim, 15_ms, [ this ] { p()->buffs.frostfire_empowerment->decrement(); } );
        p()->state.trigger_ff_empowerment = true;
        trigger_frostfire_mastery( true );
      }

      p()->trigger_flash_freezeburn( true );
    }
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( s->result == RESULT_CRIT )
        p()->buffs.pyrotechnics->expire();
      else
        p()->buffs.pyrotechnics->trigger();

      if ( !consume_firefall( s->target ) )
        trigger_firefall();

      if ( frostfire )
      {
        p()->buffs.severe_temperatures->expire();

        if ( p()->state.trigger_ff_empowerment )
        {
          p()->state.trigger_ff_empowerment = false;
          p()->action.frostfire_empowerment->execute_on_target( s->target, p()->talents.frostfire_empowerment->effectN( 2 ).percent() * s->result_total );
        }
      }
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

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( frostfire )
    {
      m *= 1.0 + p()->buffs.severe_temperatures->check_stack_value();
      if ( p()->state.trigger_ff_empowerment )
        m *= 1.0 + p()->buffs.frostfire_empowerment->data().effectN( 3 ).percent();
    }

    return m;
  }

  double composite_crit_chance_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_chance_multiplier();

    if ( frostfire && p()->state.trigger_ff_empowerment )
      m *= 1.0 + p()->buffs.frostfire_empowerment->data().effectN( 1 ).percent();

    return m;
  }
};

struct flame_patch_t final : public fire_mage_spell_t
{
  flame_patch_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 205472 ) )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
    ground_aoe = background = true;
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }
};

struct flamestrike_pyromaniac_t final : public fire_mage_spell_t
{
  flamestrike_pyromaniac_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 460476 ) )
  {
    background = true;
    triggers.ignite = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value(); // TODO: Check this
    base_multiplier *= 1.0 + p->talents.surging_blaze->effectN( 2 ).percent();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( p()->buffs.combustion->check() )
      m *= 1.0 + p()->talents.unleashed_inferno->effectN( 4 ).percent();

    if ( p()->buffs.sparking_cinders->check() )
      m *= 1.0 + p()->talents.sparking_cinders->effectN( 2 ).percent();

    if ( p()->buffs.majesty_of_the_phoenix->check() )
      m *= 1.0 + p()->buffs.majesty_of_the_phoenix->data().effectN( 1 ).percent();

    if ( p()->buffs.burden_of_power->check() )
      m *= 1.0 + p()->buffs.burden_of_power->data().effectN( 3 ).percent();

    return m;
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    p()->consume_burden_of_power();
  }
};

struct flamestrike_t final : public hot_streak_spell_t
{
  action_t* flame_patch = nullptr;
  timespan_t flame_patch_duration;
  size_t num_targets_crit;

  flamestrike_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    hot_streak_spell_t( n, p, p->find_specialization_spell( "Flamestrike" ) )
  {
    parse_options( options_str );
    triggers.ignite = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
    base_dd_multiplier *= 1.0 + p->talents.quickflame->effectN( 1 ).percent();

    if ( p->talents.pyromaniac.ok() )
      pyromaniac_action = get_action<flamestrike_pyromaniac_t>( "flamestrike_pyromaniac", p );

    if ( p->talents.flame_patch.ok() )
    {
      flame_patch = get_action<flame_patch_t>( "flame_patch", p );
      flame_patch_duration = p->find_spell( 205470 )->duration();
      add_child( flame_patch );
    }
  }

  timespan_t execute_time_flat_modifier() const override
  {
    auto add = hot_streak_spell_t::execute_time_flat_modifier();

    if ( p()->buffs.majesty_of_the_phoenix->check() )
      add += p()->buffs.majesty_of_the_phoenix->data().effectN( 2 ).time_value();

    return add;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( p()->buffs.combustion->check() )
      m *= 1.0 + p()->talents.unleashed_inferno->effectN( 4 ).percent();

    if ( p()->buffs.sparking_cinders->check() )
      m *= 1.0 + p()->talents.sparking_cinders->effectN( 2 ).percent();

    if ( p()->buffs.majesty_of_the_phoenix->check() )
      m *= 1.0 + p()->buffs.majesty_of_the_phoenix->data().effectN( 1 ).percent();

    if ( p()->buffs.burden_of_power->check() )
      m *= 1.0 + p()->buffs.burden_of_power->data().effectN( 3 ).percent();

    return m;
  }

  double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    double m = hot_streak_spell_t::composite_ignite_multiplier( s );

    m *= 1.0 + p()->talents.mark_of_the_firelord->effectN( 1 ).percent();

    return m;
  }

  void execute() override
  {
    num_targets_crit = 0;

    hot_streak_spell_t::execute();

    p()->buffs.majesty_of_the_phoenix->decrement();

    if ( p()->buffs.combustion->check() )
    {
      double max_targets = p()->talents.unleashed_inferno->effectN( 3 ).base_value();
      p()->cooldowns.combustion->adjust( -p()->talents.unleashed_inferno->effectN( 2 ).time_value() * std::min( num_targets_crit / max_targets, 1.0 ) );
    }

    double max_targets = p()->talents.kindling->effectN( 2 ).base_value();
    timespan_t amount = p()->talents.kindling->effectN( 1 ).time_value() * std::min( num_targets_crit / max_targets, 1.0 );
    p()->cooldowns.combustion->adjust( -amount );
    if ( !p()->buffs.combustion->check() )
      p()->expression_support.kindling_reduction += amount;

    if ( hit_any_target )
      handle_hot_streak( execute_state->crit_chance, p()->spec.fuel_the_fire->ok() ? HS_CUSTOM : HS_HIT );

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
        .hasted( ground_aoe_params_t::SPELL_CAST_SPEED ) );
    }
  }

  void schedule_travel( action_state_t* s ) override
  {
    hot_streak_spell_t::schedule_travel( s );
    if ( s->result == RESULT_CRIT ) num_targets_crit++;
  }
};

struct glacial_assault_t final : public frost_mage_spell_t
{
  glacial_assault_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 379029 ) )
  {
    background = true;
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      get_td( s->target )->debuffs.numbing_blast->trigger();
  }
};

struct flurry_bolt_t final : public frost_mage_spell_t
{
  flurry_bolt_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 228354 ) )
  {
    background = triggers.chill = triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( s->chain_target == 0 && p()->buffs.excess_frost->check() )
    {
      p()->action.excess_ice_nova->execute_on_target( s->target );
      p()->buffs.excess_frost->decrement();
    }

    trigger_winters_chill( s );
    consume_cold_front( s->target );

    if ( rng().roll( p()->talents.glacial_assault->effectN( 1 ).percent() ) )
      make_event( *sim, 1.0_s, [ this, t = s->target ] { p()->action.glacial_assault->execute_on_target( t ); } );
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
    may_miss = affected_by.shatter = false;
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

    p()->trigger_icicle_gain( target, p()->action.icicle.flurry );
    p()->trigger_icicle_gain( target, p()->action.icicle.flurry, p()->talents.splintering_cold->effectN( 2 ).percent() );
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

    // Flurry only triggers one stack of Cold Front, but it happens after the first
    // Flurry bolt attempts to consume Cold Front buff, let it execute first.
    if ( p()->talents.cold_front.ok() )
      make_event( *sim, 1_ms, [ this ] { trigger_cold_front(); } );
  }
};

struct frostbolt_t final : public frost_mage_spell_t
{
  const bool frostfire;

  double fof_chance;
  double bf_chance;
  double fractured_frost_mul;

  frostbolt_t( std::string_view n, mage_t* p, std::string_view options_str, bool frostfire_ = false ) :
    frost_mage_spell_t( n, p, frostfire_ ? p->talents.frostfire_bolt : p->find_class_spell( "Frostbolt" ) ),
    frostfire( frostfire_ ),
    fof_chance(),
    bf_chance(),
    fractured_frost_mul()
  {
    // TODO Frostfire
    // * triggers an additional Frost/Fire Mastery on hit rather than on cast
    // * Fractured Frost makes the first projectile hit 3 nearby targets, doesn't care about where the other projectiles are going
    // * extra hits from Fractured Frost don't trigger Bone Chilling

    parse_options( options_str );
    if ( frostfire )
      base_execute_time *= 1.0 + p->talents.thermal_conditioning->effectN( 1 ).percent();
    enable_calculate_on_impact( frostfire ? 468655 : 228597 );

    track_shatter = consumes_winters_chill = true;
    triggers.chill = triggers.overflowing_energy = true;
    base_dd_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();
    base_dd_multiplier *= 1.0 + p->talents.wintertide->effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + p->talents.piercing_cold->effectN( 1 ).percent();

    const auto& ft = p->talents.frozen_touch;
    fof_chance = ( 1.0 + ft->effectN( 1 ).percent() ) * p->talents.fingers_of_frost->effectN( 1 ).percent();
    bf_chance = ( 1.0 + ft->effectN( 2 ).percent() ) * p->talents.brain_freeze->effectN( 1 ).percent();

    fractured_frost_mul = p->find_spell( 378445 )->effectN( 3 ).percent();

    if ( data().ok() && p->spec.icicles->ok() )
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
    if ( p()->talents.fractured_frost.ok() && p()->buffs.icy_veins->check() )
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

  double execute_time_pct_multiplier() const override
  {
    if ( frostfire && p()->buffs.frostfire_empowerment->check() )
      return 0.0;

    double mul = frost_mage_spell_t::execute_time_pct_multiplier();

    mul *= 1.0 + p()->buffs.slick_ice->check_stack_value();

    return mul;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = frost_mage_spell_t::composite_da_multiplier( s );

    m *= 1.0 + p()->buffs.slick_ice->check() * p()->buffs.slick_ice->data().effectN( 3 ).percent();

    if ( frostfire )
    {
      m *= 1.0 + p()->buffs.severe_temperatures->check_stack_value();
      if ( p()->state.trigger_ff_empowerment )
        m *= 1.0 + p()->buffs.frostfire_empowerment->data().effectN( 3 ).percent();
    }

    if ( p()->talents.fractured_frost.ok() && p()->buffs.icy_veins->check() )
      m *= 1.0 + fractured_frost_mul;

    return m;
  }

  double composite_crit_chance_multiplier() const override
  {
    double m = frost_mage_spell_t::composite_crit_chance_multiplier();

    if ( frostfire && p()->state.trigger_ff_empowerment )
      m *= 1.0 + p()->buffs.frostfire_empowerment->data().effectN( 1 ).percent();

    return m;
  }

  double frozen_multiplier( const action_state_t* s ) const override
  {
    double fm = frost_mage_spell_t::frozen_multiplier( s );

    fm *= 1.0 + p()->talents.deep_shatter->effectN( 1 ).percent();

    return fm;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->trigger_icicle_gain( target, p()->action.icicle.frostbolt );
    p()->trigger_icicle_gain( target, p()->action.icicle.frostbolt, p()->talents.splintering_cold->effectN( 2 ).percent() );

    p()->trigger_fof( fof_chance, proc_fof );
    p()->trigger_brain_freeze( bf_chance, proc_brain_freeze );

    if ( p()->buffs.icy_veins->check() )
      p()->buffs.slick_ice->trigger();

    if ( frostfire )
    {
      if ( p()->buffs.frostfire_empowerment->check() )
      {
        // Buff is decremented with a short delay, allowing two spells to benefit.
        // TODO: Double check this later
        make_event( *sim, 15_ms, [ this ] { p()->buffs.frostfire_empowerment->decrement(); } );
        p()->state.trigger_ff_empowerment = true;
        trigger_frostfire_mastery( true );
      }

      p()->trigger_flash_freezeburn( true );
    }
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p()->buffs.icy_veins->check() )
        p()->buffs.deaths_chill->trigger();

      if ( frostfire )
      {
        p()->buffs.severe_temperatures->expire();

        if ( p()->state.trigger_ff_empowerment )
        {
          p()->state.trigger_ff_empowerment = false;
          p()->action.frostfire_empowerment->execute_on_target( s->target, p()->talents.frostfire_empowerment->effectN( 2 ).percent() * s->result_total );
        }
      }
    }

    if ( s->chain_target != 0 )
      return;

    // See frost_mage_spell_t::consume_cold_front
    if ( p()->buffs.cold_front_ready->check() )
    {
      consume_cold_front( s->target );
      trigger_cold_front( s->n_targets );
    }
    else if ( p()->buffs.cold_front->check() >= 28 )
    {
      trigger_cold_front();
      if ( s->n_targets > 1 )
      {
        consume_cold_front( s->target );
        trigger_cold_front();
      }
    }
    else
    {
      trigger_cold_front( s->n_targets );
      consume_cold_front( s->target );
    }
  }
};

struct frost_nova_t final : public mage_spell_t
{
  frost_nova_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Frost Nova" ) )
  {
    parse_options( options_str );
    aoe = -1;
    affected_by.time_manipulation = true;
    cooldown->charges += as<int>( p->talents.ice_ward->effectN( 1 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );
    p()->trigger_crowd_control( s, MECHANIC_FREEZE );
  }
};

struct frozen_orb_bolt_t final : public frost_mage_spell_t
{
  frozen_orb_bolt_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 84721 ) )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
    base_multiplier *= 1.0 + p->talents.everlasting_frost->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->talents.splintering_orbs->effectN( 3 ).percent();
    background = triggers.chill = true;
    affected_by.icicles_aoe = true;
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

    am *= 1.0 + p()->buffs.spellfrost_teachings->check_value();

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
    may_miss = affected_by.shatter = false;
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

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->buffs.permafrost_lances->trigger();
    if ( !background ) p()->buffs.freezing_rain->trigger();
    // The Cold Front Frozen Orb seems to trigger Freezing Winds and then (almost always) immediately
    // expire it. However, if Freezing Winds is already up, it refreshes the buff as normal.
    // TODO: double check that this is the case, maybe quantify the fail chance as well?
    if ( !background || !p()->bugs || p()->buffs.freezing_winds->check() ) p()->buffs.freezing_winds->trigger();
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

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( pulse_time )
      .target( s->target )
      .n_pulses( pulse_count )
      .action( frozen_orb_bolt ), true );

    if ( p()->talents.splintering_orbs.ok() )
    {
      p()->trigger_splinter( nullptr );
      int count = as<int>( p()->talents.splintering_orbs->effectN( 1 ).base_value() ) - 1;
      make_repeating_event( *sim, pulse_time, [ this ] { p()->trigger_splinter( nullptr ); }, count );
    }
  }
};

struct glacial_spike_t final : public frost_mage_spell_t
{
  shatter_source_t* cleave_source = nullptr;

  glacial_spike_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.glacial_spike )
  {
    parse_options( options_str );
    enable_calculate_on_impact( 228600 );
    track_shatter = consumes_winters_chill = true;
    triggers.overflowing_energy = true;
    base_multiplier *= 1.0 + p->talents.flash_freeze->effectN( 2 ).percent();
    crit_bonus_multiplier *= 1.0 + p->talents.piercing_cold->effectN( 1 ).percent();

    if ( p->talents.splitting_ice.ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_aoe_multiplier *= p->talents.splitting_ice->effectN( 4 ).percent();
    }
  }

  void init_finished() override
  {
    proc_brain_freeze = p()->get_proc( "Brain Freeze from Glacial Spike" );

    frost_mage_spell_t::init_finished();

    if ( sim->report_details != 0 && p()->talents.splitting_ice.ok() )
      cleave_source = p()->get_shatter_source( "Glacial Spike cleave" );
  }

  bool ready() override
  {
    // Glacial Spike doesn't check the Icicles buff after it started executing.
    if ( p()->executing != this && !p()->buffs.icicles->at_max_stacks() )
      return false;

    return frost_mage_spell_t::ready();
  }

  timespan_t execute_time() const override
  {
    timespan_t t = frost_mage_spell_t::execute_time();

    // If the Mage just gained the final Icicle, add a small delay to approximate the lack of spell queueing.
    t += std::max( p()->state.gained_full_icicles + p()->options.glacial_spike_delay - sim->current_time(), 0_ms );

    return t;
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    double icicle_coef = icicle_sp_coefficient();
    icicle_coef *=       p()->spec.icicles->effectN( 2 ).base_value();
    icicle_coef *= 1.0 + p()->talents.splitting_ice->effectN( 3 ).percent();

    // Since the Icicle portion doesn't double dip mastery, the mastery multiplier has to be applied manually.
    am *= 1.0 + p()->cache.mastery() * p()->spec.icicles_2->effectN( 3 ).mastery_value()
              + icicle_coef / spell_power_mod.direct;

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
    }
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( s->chain_target == 0 && p()->talents.thermal_void.ok() && p()->buffs.icy_veins->check() )
      p()->buffs.icy_veins->extend_duration( p(), p()->talents.thermal_void->effectN( 3 ).time_value() );

    p()->trigger_crowd_control( s, MECHANIC_FREEZE );

    if ( s->chain_target != 0 )
      record_shatter_source( s, cleave_source );
  }
};

struct ice_floes_t final : public mage_spell_t
{
  ice_floes_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.ice_floes )
  {
    parse_options( options_str );
    may_miss = harmful = false;
    usable_while_casting = true;
    internal_cooldown->duration = data().internal_cooldown();
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.ice_floes->trigger();
  }
};

struct frigid_pulse_t final : public mage_spell_t
{
  frigid_pulse_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 460623 ) )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = p->sets->set( MAGE_FROST, TWW1, B4 )->effectN( 1 ).base_value();
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
  shatter_source_t* extension_source = nullptr;
  shatter_source_t* cleave_source = nullptr;
  action_t* frigid_pulse = nullptr;

  ice_lance_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ice_lance )
  {
    parse_options( options_str );
    enable_calculate_on_impact( 228598 );
    track_shatter = consumes_winters_chill = true;
    triggers.overflowing_energy = true;
    affected_by.icicles_st = true;
    base_multiplier *= 1.0 + p->talents.lonely_winter->effectN( 1 ).percent();
    base_multiplier *= 1.0 + p->sets->set( MAGE_FROST, TWW1, B2 )->effectN( 1 ).percent();

    // TODO: Cleave distance for SI seems to be 8 + hitbox size.
    if ( p->talents.splitting_ice.ok() )
    {
      aoe = 1 + as<int>( p->talents.splitting_ice->effectN( 1 ).base_value() );
      base_multiplier *= 1.0 + p->talents.splitting_ice->effectN( 3 ).percent();
      base_aoe_multiplier *= p->find_spell( 228598 )->effectN( 1 ).chain_multiplier();
    }

    if ( p->talents.hailstones.ok() )
      add_child( p->action.icicle.ice_lance );

    if ( p->sets->has_set_bonus( MAGE_FROST, TWW1, B4 ) )
    {
      frigid_pulse = get_action<frigid_pulse_t>( "frigid_pulse", p );
      add_child( frigid_pulse );
    }
  }

  void init_finished() override
  {
    proc_brain_freeze = p()->get_proc( "Brain Freeze from Ice Lance" );

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
      p()->trigger_icicle_gain( s->target, p()->action.icicle.ice_lance, p()->talents.hailstones->effectN( 1 ).percent() );
    }
  }

  void execute() override
  {
    p()->state.fingers_of_frost_active = p()->buffs.fingers_of_frost->up();

    frost_mage_spell_t::execute();

    if ( p()->state.fingers_of_frost_active )
      p()->buffs.cryopathy->trigger();

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
        p()->buffs.icy_veins->extend_duration( p(), p()->talents.thermal_void->effectN( 1 ).time_value() );
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

    if ( p()->buffs.excess_fire->check() )
    {
      p()->action.excess_living_bomb->execute_on_target( s->target );
      p()->buffs.excess_fire->decrement();
    }

    if ( frozen & FF_FINGERS_OF_FROST && frigid_pulse )
      frigid_pulse->execute_on_target( s->target );
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.chain_reaction->check_stack_value();
    am *= 1.0 + p()->buffs.permafrost_lances->check_value();

    return am;
  }

  double frozen_multiplier( const action_state_t* s ) const override
  {
    double fm = frost_mage_spell_t::frozen_multiplier( s );

    fm *= 3.0;

    unsigned frozen = cast_state( s )->frozen;
    if ( frozen &  FF_FINGERS_OF_FROST
      && frozen & ~FF_FINGERS_OF_FROST )
    {
      fm *= 1.0 + p()->talents.wintertide->effectN( 2 ).percent();
    }

    return fm;
  }
};

struct ice_nova_t final : public frost_mage_spell_t
{
  const bool excess;

  ice_nova_t( std::string_view n, mage_t* p, std::string_view options_str, bool excess_ = false ) :
    frost_mage_spell_t( n, p, excess_ ? p->find_spell( 157997 ) : p->talents.ice_nova ),
    excess( excess_ )
  {
    parse_options( options_str );
    aoe = -1;
    // TODO: currently deals full damage to all targets, probably a bug
    if ( !p->bugs )
    {
      reduced_aoe_targets = 1.0;
      full_amount_targets = 1;
    }

    if ( excess )
    {
      background = true;
      cooldown->duration = 0_ms;
      base_multiplier *= p->talents.excess_frost->effectN( 1 ).percent();
    }
    else
    {
      affected_by.time_manipulation = true;
    }
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    if ( excess )
    {
      trigger_frostfire_mastery();
      timespan_t t = -1000 * p()->talents.excess_frost->effectN( 2 ).time_value();
      p()->cooldowns.comet_storm->adjust( t );
      p()->cooldowns.meteor->adjust( t );
    }

    if ( p()->specialization() == MAGE_FROST )
      p()->buffs.unerring_proficiency->expire();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( !excess )
      p()->trigger_crowd_control( s, MECHANIC_FREEZE );
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    if ( p()->specialization() == MAGE_FROST )
      am *= 1.0 + p()->buffs.unerring_proficiency->check_stack_value();

    return am;
  }
};

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
    p()->trigger_splinter( nullptr, as<int>( p()->talents.augury_abounds->effectN( 1 ).base_value() ) );

    frost_mage_spell_t::execute();

    p()->buffs.icy_veins->trigger();
    p()->buffs.cryopathy->trigger( p()->buffs.cryopathy->max_stack() );
    p()->trigger_flash_freezeburn();
    if ( p()->pets.water_elemental->is_sleeping() )
      p()->pets.water_elemental->summon();
  }
};

struct fire_blast_t final : public fire_mage_spell_t
{
  size_t lit_fuse_targets;

  fire_blast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.fire_blast.ok() ? p->talents.fire_blast : p->find_class_spell( "Fire Blast" ) )
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.kindling = triggers.calefaction = triggers.unleashed_inferno = TT_MAIN_TARGET;
    affected_by.unleashed_inferno = triggers.ignite = triggers.from_the_ashes = triggers.overflowing_energy = true;

    cooldown->charges += as<int>( p->talents.flame_on->effectN( 1 ).base_value() );
    cooldown->duration -= 1000 * p->talents.fervent_flickering->effectN( 2 ).time_value();
    cooldown->hasted = true;
    // Data comes from talents.lit_fuse but needs to be available even when the talent isn't taken
    lit_fuse_targets = as<size_t>( p->find_spell( 450716 )->effectN( 2 ).base_value() );
    lit_fuse_targets += as<size_t>( p->talents.blast_zone->effectN( 3 ).base_value() );

    if ( p->talents.fire_blast.ok() )
    {
      base_crit += 1.0;
      usable_while_casting = true;
    }
  }

  void execute() override
  {
    if ( p()->buffs.glorious_incandescence->check() )
    {
      p()->buffs.glorious_incandescence->decrement();
      p()->state.trigger_glorious_incandescence = true;
    }

    fire_mage_spell_t::execute();

    if ( p()->specialization() == MAGE_FIRE )
      p()->trigger_time_manipulation();

    if ( p()->buffs.lit_fuse->check() )
    {
      p()->buffs.lit_fuse->decrement();
      std::vector<player_t*> tl = target_list(); // Make a copy.
      if ( !tl.empty() )
      {
        // TODO: Check for targeting conditions.
        rng().shuffle( tl.begin(), tl.end() );
        for ( size_t i = 0; i < lit_fuse_targets && i < tl.size(); i++ )
          p()->action.living_bomb->execute_on_target( tl[ i ] );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    spread_ignite( s->target );

    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.feel_the_burn->trigger();

      if ( p()->buffs.excess_fire->check() )
      {
        p()->action.excess_living_bomb->execute_on_target( s->target );
        p()->buffs.excess_fire->decrement();
      }

      trigger_glorious_incandescence( s->target );
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

struct living_bomb_explosion_t final : public fire_mage_spell_t
{
  const bool excess;

  static unsigned explosion_spell_id( bool excess, bool primary )
  {
    if ( excess )
      return primary ? 438674 : 464884;
    else
      return primary ? 44461 : 453251;
  }

  living_bomb_explosion_t( std::string_view n, mage_t* p, bool excess_, bool primary_ ) :
    fire_mage_spell_t( n, p, p->find_spell( explosion_spell_id( excess_, primary_ ) ) ),
    excess( excess_ )
  {
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    background = triggers.ignite = true;
    base_dd_multiplier *= 1.0 + p->talents.explosive_ingenuity->effectN( 2 ).percent();
    if ( excess )
      base_dd_multiplier *= 1.0 + p->spec.frost_mage->effectN( 11 ).percent();
    // TODO: This is scripted to only apply to Frost, keep an eye on it
    if ( p->specialization() == MAGE_FROST )
      base_dd_multiplier *= 1.0 + p->talents.excess_fire->effectN( 3 ).percent();
  }

  void init() override
  {
    fire_mage_spell_t::init();

    action_t* parent = excess ? p()->action.excess_living_bomb : p()->action.living_bomb;
    if ( parent && this != parent )
      parent->add_child( this );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = fire_mage_spell_t::composite_da_multiplier( s );

    if ( p()->buffs.combustion->check() )
      // TODO: This is currently "Add Flat Multiplier" in the data. Verify the
      // specific numbers in game, especially because scripting is involved.
      // There is also a zeroed "Add Percent Multiplier" on Combustion for Living Bomb.
      am *= 1.0 + p()->talents.explosivo->effectN( 2 ).percent();

    return am;
  }

  double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_ignite_multiplier( s );

    m *= 1.0 + p()->talents.mark_of_the_firelord->effectN( 1 ).percent();

    return m;
  }
};

struct living_bomb_dot_t final : public fire_mage_spell_t
{
  // The game has two spells for the DoT, one for pre-spread one and one for
  // post-spread one. This allows two copies of the DoT to be up on one target.
  const bool excess;
  const bool primary;
  size_t max_spread_targets;

  action_t* dot_spread = nullptr;
  action_t* explosion = nullptr;

  static unsigned dot_spell_id( bool excess, bool primary )
  {
    if ( excess )
      return primary ? 438672 : 438671;
    else
      return primary ? 217694 : 244813;
  }

  static std::string spell_name( bool explosion, bool excess, bool primary )
  {
    return fmt::format( "{}living_bomb{}{}", excess ? "excess_" : "", primary ? "" : "_spread", explosion ? "_explosion" : "" );
  }

  living_bomb_dot_t( std::string_view n, mage_t* p, bool excess_ = false, bool primary_ = true ) :
    fire_mage_spell_t( n, p, p->find_spell( dot_spell_id( excess_, primary_ ) ) ),
    excess( excess_ ),
    primary( primary_ ),
    max_spread_targets()
  {
    background = true;
    // Data comes from talents.lit_fuse but needs to be available even when the talent isn't taken
    max_spread_targets = as<size_t>( p->find_spell( 450716 )->effectN( 3 ).base_value() );
    max_spread_targets += as<size_t>( p->talents.blast_zone->effectN( 4 ).base_value() );

    explosion = get_action<living_bomb_explosion_t>( spell_name( true, excess, primary ), p, excess, primary );
    if ( primary )
      dot_spread = get_action<living_bomb_dot_t>( spell_name( false, excess, false ), p, excess, false );
  }

  void init() override
  {
    fire_mage_spell_t::init();
    update_flags &= ~STATE_HASTE;

    action_t* parent = excess ? p()->action.excess_living_bomb : p()->action.living_bomb;
    if ( parent && this != parent )
      parent->add_child( this );
  }

  void trigger_explosion( player_t* target )
  {
    // Don't trigger explosions when the iteration ends
    if ( sim->event_mgr.canceled )
      return;

    explosion->set_target( target );

    if ( primary )
    {
      // We're gonna be manipulating the vector, so make a copy
      std::vector<player_t*> targets = explosion->target_list();
      // TODO: Living Bomb can currently spread to the original target, but only
      // if another target is nearby
      if ( !p()->bugs || targets.size() == 1 )
        range::erase_remove( targets, target );
      rng().shuffle( targets.begin(), targets.end() );

      size_t spread = std::min( max_spread_targets, targets.size() );
      for ( size_t i = 0; i < spread; i++ )
        dot_spread->execute_on_target( targets[ i ] );

      if ( p()->talents.convection.ok() && spread == 0 )
        dot_spread->execute_on_target( target );

      if ( excess )
      {
        p()->cooldowns.phoenix_flames->adjust( -1000 * p()->talents.excess_fire->effectN( 2 ).time_value() );
        p()->trigger_brain_freeze( 1.0, p()->procs.brain_freeze_excess_fire, 0_ms );
      }
    }

    explosion->execute();
    p()->buffs.sparking_cinders->trigger();
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    if ( primary && excess )
      trigger_frostfire_mastery();
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
  const meteor_type type;
  action_t* meteor_burn;

  timespan_t meteor_burn_duration;
  timespan_t meteor_burn_pulse_time;

  meteor_impact_t( std::string_view n, mage_t* p, action_t* burn, meteor_type type_ ) :
    fire_mage_spell_t( n, p, p->find_spell( type_ == meteor_type::ISOTHERMIC ? 438607 : 351140 ) ),
    type( type_ ),
    meteor_burn( burn ),
    meteor_burn_duration( p->find_spell( 175396 )->duration() ),
    meteor_burn_pulse_time( p->find_spell( 155158 )->effectN( 1 ).period() )
  {
    aoe = -1;
    reduced_aoe_targets = 8;
    background = triggers.ignite = true;
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    p()->ground_aoe_expiration[ AOE_METEOR_BURN ] = sim->current_time() + meteor_burn_duration;
    p()->trigger_meteor_burn( meteor_burn, target, meteor_burn_pulse_time, meteor_burn_duration );

    if ( type == meteor_type::ISOTHERMIC )
      trigger_frostfire_mastery();

    if ( p()->talents.deep_impact.ok() )
    {
      const auto& tl = target_list();
      if ( !tl.empty() )
      {
        player_t* t = tl[ rng().range( tl.size() ) ];
        p()->action.living_bomb->execute_on_target( t );
      }
    }
  }
};

struct meteor_t final : public fire_mage_spell_t
{
  timespan_t meteor_delay;

  meteor_t( std::string_view n, mage_t* p, std::string_view options_str, meteor_type type = meteor_type::NORMAL ) :
    fire_mage_spell_t( n, p, type == meteor_type::NORMAL ? p->talents.meteor : p->find_spell( 153561 ) ),
    meteor_delay( p->find_spell( 177345 )->duration() )
  {
    parse_options( options_str );

    if ( !data().ok() )
      return;

    cooldown->duration += p->talents.deep_impact->effectN( 1 ).time_value();

    std::string_view burn_name;
    std::string_view impact_name;

    switch ( type )
    {
      case meteor_type::NORMAL:
        burn_name = "meteor_burn";
        impact_name = "meteor_impact";
        break;
      case meteor_type::FIREFALL:
        burn_name = "firefall_meteor_burn";
        impact_name = "firefall_meteor_impact";
        break;
      case meteor_type::ISOTHERMIC:
        burn_name = "isothermic_meteor_burn";
        impact_name = "isothermic_meteor_impact";
        break;
      default:
        assert( false );
        break;
    }

    action_t* meteor_burn = get_action<meteor_burn_t>( burn_name, p );
    impact_action = get_action<meteor_impact_t>( impact_name, p, meteor_burn, type );

    add_child( meteor_burn );
    add_child( impact_action );

    if ( type != meteor_type::NORMAL )
    {
      background = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  }

  timespan_t travel_time() const override
  {
    // Travel time cannot go lower than 1 second to give time for Meteor to visually fall.
    return std::max( meteor_delay * p()->cache.spell_cast_speed(), 1.0_s );
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    if ( p()->action.isothermic_comet_storm )
      p()->action.isothermic_comet_storm->execute_on_target( target );
  }
};

struct meteorite_impact_t final : public mage_spell_t
{
  meteorite_impact_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 449569 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 8; // TODO: Verify this
    background = triggers.ignite = true;
  }

  void execute() override
  {
    mage_spell_t::execute();

    p()->cooldowns.fire_blast->adjust( -p()->talents.glorious_incandescence->effectN( 2 ).time_value(), true, false );
  }
};

struct meteorite_t final : public mage_spell_t
{
  timespan_t meteor_delay;

  meteorite_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 449559 ) ),
    meteor_delay( p->find_spell( 449562 )->duration() )
  {
    background = true;
    impact_action = get_action<meteorite_impact_t>( "meteorite_impact", p );

    add_child( impact_action );
  }

  timespan_t travel_time() const override
  {
    // Travel time cannot go lower than 1 second to give time for Meteorite to visually fall.
    return std::max( meteor_delay * p()->cache.spell_cast_speed(), 1.0_s );
  }
};

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

struct phoenix_flames_splash_t final : public fire_mage_spell_t
{
  phoenix_flames_splash_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 257542 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    background = affected_by.unleashed_inferno = triggers.ignite = true;
    triggers.hot_streak = triggers.kindling = triggers.calefaction = triggers.unleashed_inferno = TT_MAIN_TARGET;
    base_multiplier *= 1.0 + p->talents.from_the_ashes->effectN( 2 ).percent();
    base_multiplier *= 1.0 + p->sets->set( MAGE_FIRE, TWW1, B2 )->effectN( 1 ).percent();
    base_crit += p->talents.call_of_the_sun_king->effectN( 2 ).percent();
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    if ( num_targets_hit >= as<int>( p()->talents.majesty_of_the_phoenix->effectN( 1 ).base_value() ) )
      p()->buffs.majesty_of_the_phoenix->trigger( p()->buffs.majesty_of_the_phoenix->max_stack() );
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buffs.feel_the_burn->trigger();

      if ( s->chain_target == 0 && p()->buffs.excess_frost->check() )
      {
        p()->action.excess_ice_nova->execute_on_target( s->target );
        p()->buffs.excess_frost->decrement();
      }
    }
  }

  double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.flames_fury->check_value();

    return am;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( s->n_targets <= as<unsigned>( p()->talents.ashen_feather->effectN( 1 ).base_value() ) )
      m *= 1.0 + p()->talents.ashen_feather->effectN( 2 ).percent();

    if ( s->chain_target == 0 )
      m *= 1.0 + p()->sets->set( MAGE_FIRE, TWW1, B2 )->effectN( 2 ).percent();

    return m;
  }

  double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_ignite_multiplier( s );

    if ( s->n_targets <= as<unsigned>( p()->talents.ashen_feather->effectN( 1 ).base_value() ) )
      m *= p()->talents.ashen_feather->effectN( 3 ).percent();

    return m;
  }
};

struct phoenix_flames_t final : public fire_mage_spell_t
{
  double blessing_speed;

  phoenix_flames_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.phoenix_flames ),
    blessing_speed( p->find_spell( 455137 )->missile_speed() )
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

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = fire_mage_spell_t::recharge_multiplier( cd );

    if ( &cd == cooldown )
      m /= 1.0 + p()->buffs.fiery_rush->check_value();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( p()->buffs.flames_fury->check() )
    {
      make_event( *sim, [ this ]
      {
        cooldown->reset( false );
        p()->buffs.flames_fury->decrement();
      } );
    }

    const spell_data_t* set = p()->sets->set( MAGE_FIRE, TWW1, B4 );
    if ( rng().roll( set->effectN( 1 ).percent() ) )
    {
      timespan_t delay = timespan_t::from_seconds( p()->get_player_distance( *s->target ) / blessing_speed );
      make_event( *sim, delay, [ this, set ]
      {
        p()->buffs.blessing_of_the_phoenix->trigger();
        cooldown->adjust( -set->effectN( 2 ).percent() * cooldown_t::cooldown_duration( cooldown ), false, false );
      } );
    }
  }
};

struct pyroblast_pyromaniac_t final : public fire_mage_spell_t
{
  pyroblast_pyromaniac_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 460475 ) )
  {
    background = true;
    triggers.ignite = true;
    base_multiplier *= 1.0 + p->talents.surging_blaze->effectN( 2 ).percent();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( p()->buffs.sparking_cinders->check() )
      m *= 1.0 + p()->talents.sparking_cinders->effectN( 1 ).percent();

    if ( p()->buffs.burden_of_power->check() )
      m *= 1.0 + p()->buffs.burden_of_power->data().effectN( 1 ).percent();

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    c += p()->buffs.hyperthermia->check_value();

    return c;
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    p()->consume_burden_of_power();
  }
};

struct pyroblast_t final : public hot_streak_spell_t
{
  pyroblast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    hot_streak_spell_t( n, p, p->talents.pyroblast )
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.kindling = triggers.calefaction = triggers.unleashed_inferno = TT_MAIN_TARGET;
    affected_by.unleashed_inferno = triggers.ignite = triggers.from_the_ashes = triggers.overflowing_energy = true;

    if ( p->talents.pyromaniac.ok() )
      pyromaniac_action = get_action<pyroblast_pyromaniac_t>( "pyroblast_pyromaniac", p );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hot_streak_spell_t::composite_da_multiplier( s );

    if ( p()->buffs.sparking_cinders->check() )
      m *= 1.0 + p()->talents.sparking_cinders->effectN( 1 ).percent();

    if ( p()->buffs.burden_of_power->check() )
      m *= 1.0 + p()->buffs.burden_of_power->data().effectN( 1 ).percent();

    return m;
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
      get_td( s->target )->debuffs.controlled_destruction->trigger();

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

struct splintering_ray_t final : public spell_t
{
  splintering_ray_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 418735 ) )
  {
    background = true;
    base_dd_min = base_dd_max = 1.0;
  }

  void init() override
  {
    spell_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    // Ignore Positive Damage Taken Modifiers (321)
    return std::min( spell_t::composite_target_multiplier( target ), 1.0 );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    spell_t::available_targets( tl );

    range::erase_remove( tl, target );

    return tl.size();
  }
};

struct ray_of_frost_t final : public frost_mage_spell_t
{
  action_t* splintering_ray = nullptr;

  ray_of_frost_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ray_of_frost )
  {
    parse_options( options_str );
    channeled = triggers.chill = true;
    affected_by.icicles_st = true;

    if ( p->talents.splintering_ray.ok() )
    {
      splintering_ray = get_action<splintering_ray_t>( "splintering_ray", p );
      add_child( splintering_ray );
    }
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

    if ( splintering_ray )
      splintering_ray->execute_on_target( d->target, p()->talents.splintering_ray->effectN( 1 ).percent() * d->state->result_total );
  }

  void last_tick( dot_t* d ) override
  {
    frost_mage_spell_t::last_tick( d );

    p()->buffs.ray_of_frost->expire();
    p()->buffs.cryopathy->expire();
    // Technically, both of these buffs should also be expired when Ray of Frost is refreshed.
    // The hidden FoF buff (see above) is expired but not reapplied, breaking the effect.
    // Currently not relevant as refreshing RoF is only possible through spell data overrides.
  }

  double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.ray_of_frost->check_stack_value();
    am *= 1.0 + p()->buffs.cryopathy->check_stack_value();

    return am;
  }
};

struct scorch_t final : public fire_mage_spell_t
{
  scorch_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.scorch )
  {
    parse_options( options_str );
    triggers.hot_streak = triggers.calefaction = triggers.unleashed_inferno = triggers.kindling = TT_MAIN_TARGET;
    affected_by.unleashed_inferno = triggers.ignite = triggers.from_the_ashes = triggers.overflowing_energy = true;
    // There is a tiny delay between Scorch dealing damage and Hot Streak
    // state being updated. Here we model it as a tiny travel time.
    travel_delay = p->options.scorch_delay.total_seconds();
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.heat_shimmer->check() )
      return 0_ms;

    return fire_mage_spell_t::execute_time();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( scorch_execute_active( s->target ) )
      m *= 1.0 + p()->talents.scald->effectN( 1 ).percent();

    return m;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = fire_mage_spell_t::composite_target_crit_chance( target );

    if ( scorch_execute_active( target ) )
      c += 1.0;

    return c;
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( scorch_execute_active( s->target ) )
        p()->buffs.frenetic_speed->trigger();
      if ( improved_scorch_active( s->target ) )
        get_td( s->target )->debuffs.improved_scorch->trigger();
      if ( p()->buffs.heat_shimmer->check() )
        // The currently casting Scorch and a queued Scorch can both fully benefit from Heat Shimmer.
        make_event( *sim, 15_ms, [ this ] { p()->buffs.heat_shimmer->decrement(); } );
    }
  }

  bool usable_moving() const override
  { return true; }
};

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
    cooldown->duration += p->talents.flow_of_time->effectN( 2 ).time_value();
  }
};

struct slow_t final : public arcane_mage_spell_t
{
  slow_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.slow )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }
};

struct supernova_t final : public mage_spell_t
{
  supernova_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.supernova )
  {
    parse_options( options_str );
    aoe = -1;
    affected_by.time_manipulation = affected_by.savant = true;
    triggers.clearcasting = true;

    double sn_mult = 1.0 + p->talents.supernova->effectN( 1 ).percent();
    base_multiplier     *= sn_mult;
    base_aoe_multiplier /= sn_mult;

    if ( p->talents.gravity_lapse.ok() )
      background = true;
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( p()->specialization() == MAGE_ARCANE )
      p()->buffs.unerring_proficiency->expire();
  }

  double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    if ( p()->specialization() == MAGE_ARCANE )
      am *= 1.0 + p()->buffs.unerring_proficiency->check_stack_value();

    return am;
  }
};

// Gravity Lapse's damage does not count as a Mage spell in most ways.
// TODO: Check this later
struct gravity_lapse_impact_t final : public spell_t
{
  gravity_lapse_impact_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 449715 ) )
  {
    background = true;
  }

  mage_t* p() const
  { return debug_cast<mage_t*>( player ); }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = spell_t::composite_da_multiplier( s );

    m *= 1.0 + p()->cache.mastery() * p()->spec.savant->effectN( 5 ).mastery_value();

    return m;
  }
};

struct gravity_lapse_t final : public mage_spell_t
{
  action_t* damage_action = nullptr;

  gravity_lapse_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_spell( 449700 ) )
  {
    parse_options( options_str );
    affected_by.time_manipulation = triggers.clearcasting = true;

    if ( !p->talents.gravity_lapse.ok() || !p->talents.supernova.ok() )
      background = true;

    damage_action = get_action<gravity_lapse_impact_t>( "gravity_lapse_impact", p );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( p()->trigger_crowd_control( s, MECHANIC_ROOT ) && s->chain_target == 0 )
      make_event( *sim, 4_s, [ this, t = s->target ] { damage_action->execute_on_target( t ); } );
  }
};

struct time_warp_t final : public mage_spell_t
{
  time_warp_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Time Warp" ) )
  {
    parse_options( options_str );
    harmful = false;

    if ( sim->overrides.bloodlust )
      background = true;
  }

  void execute() override
  {
    mage_spell_t::execute();

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
    if ( player->buffs.exhaustion->check() )
      return false;

    return mage_spell_t::ready();
  }
};

struct touch_of_the_magi_t final : public arcane_mage_spell_t
{
  touch_of_the_magi_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->talents.touch_of_the_magi )
  {
    parse_options( options_str );
    triggers.clearcasting = true;

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
      const auto& td = get_td( s->target )->debuffs;
      td.touch_of_the_magi->expire();
      td.touch_of_the_magi->trigger();

      if ( p()->talents.magis_spark.ok() )
      {
        p()->state.magis_spark_spells = 0;
        td.magis_spark->trigger();
        td.magis_spark_ab->trigger();
        td.magis_spark_abar->trigger();
        td.magis_spark_am->trigger();
      }
    }
  }

  // Touch of the Magi will trigger procs that occur only from casting damaging spells.
  bool has_amount_result() const override
  { return true; }
};

struct touch_of_the_magi_explosion_t final : public spell_t
{
  touch_of_the_magi_explosion_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 210833 ) )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    base_dd_min = base_dd_max = 1.0;
  }

  void init() override
  {
    spell_t::init();

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

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    auto mage = debug_cast<mage_t*>( player );
    if ( result_is_hit( s->result ) && mage->talents.nether_munitions.ok() )
      mage->get_target_data( s->target )->debuffs.nether_munitions->trigger();
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
  }
};

struct magis_spark_t final : public arcane_mage_spell_t
{
  magis_spark_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 453925 ) )
  {
    aoe = -1;
    background = true;
  }
};

struct magis_spark_echo_t final : public spell_t
{
  magis_spark_echo_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 458375 ) )
  {
    background = true;
    base_dd_min = base_dd_max = 1.0;
  }
};

struct shifting_power_pulse_t final : public mage_spell_t
{
  shifting_power_pulse_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 382445 ) )
  {
    background = true;
    aoe = -1;
  }
};

struct shifting_power_t final : public mage_spell_t
{
  std::vector<cooldown_t*> shifting_power_cooldowns;
  timespan_t reduction;

  shifting_power_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.shifting_power ),
    shifting_power_cooldowns(),
    reduction( data().effectN( 2 ).time_value() )
  {
    parse_options( options_str );
    channeled = affected_by.ice_floes = true;
    affected_by.shifting_power = false;
    triggers.clearcasting = true;
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

  void tick( dot_t* d ) override
  {
    mage_spell_t::tick( d );

    for ( auto cd : shifting_power_cooldowns )
      cd->adjust( reduction, false );

    int splinters = as<int>( p()->talents.shifting_shards->effectN( 1 ).base_value() / ( dot_duration / base_tick_time ) );
    p()->trigger_splinter( nullptr, splinters );
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

struct leydrinker_echo_t final : public spell_t
{
  leydrinker_echo_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 453770 ) )
  {
    background = true;
    base_dd_min = base_dd_max = 1.0;
    // The delay is probably on action execution rather than travel time,
    // but because the echo doesn't snapshot any multipliers, this doesn't matter.
    travel_delay = 0.7;
  }
};

struct dematerialize_t final : residual_action::residual_periodic_action_t<spell_t>
{
  dematerialize_t( std::string_view n, mage_t* p ) :
    residual_action_t( n, p, p->find_spell( 461498 ) )
  {
    // TODO: seems to benefit from both player and target mods at the moment,
    // which is very unusual for a residual like that. It double dips stuff like
    // versatility.
  }

  void tick( dot_t* d ) override
  {
    residual_action_t::tick( d );

    auto mage = debug_cast<mage_t*>( player );
    if ( mage->rppm.energy_reconstitution->trigger() )
      mage->action.arcane_explosion_energy_recon->execute_on_target( d->target );
  }
};

struct frostfire_infusion_t final : public mage_spell_t
{
  frostfire_infusion_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 431171 ) )
  {
    background = true;
  }

  void execute() override
  {
    mage_spell_t::execute();
    trigger_frostfire_mastery();
  }
};

struct frostfire_empowerment_t final : public spell_t
{
  frostfire_empowerment_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 431186 ) )
  {
    background = true;
    aoe = -1;
    base_dd_min = base_dd_max = 1.0;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    spell_t::available_targets( tl );

    range::erase_remove( tl, target );

    return tl.size();
  }
};

// TODO: Doesn't have the usual mage family flags, so it isn't affected by most mage effects
struct volatile_magic_t final : public spell_t
{
  volatile_magic_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( p->specialization() == MAGE_FROST ? 444967 : 444966 ) )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = p->talents.volatile_magic->effectN( 2 ).base_value();
  }
};

struct controlled_instincts_t final : public spell_t
{
  controlled_instincts_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( p->specialization() == MAGE_FROST ? 444487 : 444720 ) )
  {
    background = true;
    // Only hits 5 targets despite max_targets being 6
    aoe -= 1;
    // TODO: The tooltip still mentions this, but it's untestable at the moment since it can't hit 6 or more targets
    reduced_aoe_targets = p->talents.controlled_instincts->effectN( 5 ).base_value();
    base_dd_min = base_dd_max = 1.0;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    spell_t::available_targets( tl );

    range::erase_remove( tl, target );

    return tl.size();
  }
};

struct splinter_recall_t final : public spell_t
{
  splinter_recall_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( p->specialization() == MAGE_FROST ? 443934 : 444736 ) )
  {
    background = true;
    base_dd_min = base_dd_max = 1.0;
  }
};

struct embedded_splinter_t final : public mage_spell_t
{
  embedded_splinter_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( p->specialization() == MAGE_FROST ? 443740 : 444735 ) )
  {
    background = true;
    dot_max_stack += as<int>( p->talents.splinterstorm->effectN( 3 ).base_value() );
  }

  timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t duration ) const override
  {
    return duration;
  }

  double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p()->cache.mastery() * p()->spec.savant->effectN( 6 ).mastery_value();
    am *= 1.0 + p()->cache.mastery() * p()->spec.icicles_2->effectN( 5 ).mastery_value();

    return am;
  }

  void trigger_dot( action_state_t* s ) override
  {
    dot_t* d = get_dot( s->target );
    int before = d->current_stack();
    mage_spell_t::trigger_dot( s );
    int after = d->current_stack();

    if ( !range::contains( p()->embedded_splinters, d ) )
      p()->embedded_splinters.push_back( d );

    p()->state.embedded_splinters += after - before;
    sim->print_debug( "Embedded Splinters: {} (added {})", p()->state.embedded_splinters, after - before );
  }

  void last_tick( dot_t* d ) override
  {
    mage_spell_t::last_tick( d );
    int stack = d->current_stack();
    assert( stack > 0 );

    range::erase_remove( p()->embedded_splinters, d );

    p()->state.embedded_splinters -= stack;
    sim->print_debug( "Embedded Splinters: {} (removed {})", p()->state.embedded_splinters, stack );
    assert( p()->state.embedded_splinters >= 0 );

    if ( sim->event_mgr.canceled )
      return;

    if ( auto vm = p()->action.volatile_magic )
    {
      double old_mult = vm->base_multiplier;
      vm->base_multiplier *= stack;
      vm->execute_on_target( d->target );
      vm->base_multiplier = old_mult;
    }

    // If the dot ended due to the target dying, transfer the splinters to a nearby target.
    if ( d->target->is_sleeping() )
      make_event( *sim, [ this, stack ] { p()->trigger_splinter( nullptr, stack ); } );
  }
};

struct splinter_t final : public mage_spell_t
{
  const bool splinterstorm;
  action_t* controlled_instincts = nullptr;

  static unsigned spell_id( specialization_e spec, bool splinterstorm )
  {
    if ( spec == MAGE_FROST )
      return splinterstorm ? 443747 : 443722;
    else
      return splinterstorm ? 444713 : 443763;
  }

  splinter_t( std::string_view n, mage_t* p, bool splinterstorm_ = false ) :
    mage_spell_t( n, p, p->find_spell( spell_id( p->specialization(), splinterstorm_ ) ) ),
    splinterstorm( splinterstorm_ )
  {
    background = true;

    if ( p->talents.controlled_instincts.ok() )
      controlled_instincts = get_action<controlled_instincts_t>( "controlled_instincts", p );

    if ( splinterstorm )
      return;

    impact_action = p->action.splinter_dot;
    add_child( impact_action );

    if ( controlled_instincts )
      add_child( controlled_instincts );
    if ( p->action.volatile_magic )
      add_child( p->action.volatile_magic );
    if ( p->action.splinter_recall )
      add_child( p->action.splinter_recall );
    if ( p->action.splinterstorm )
      add_child( p->action.splinterstorm );
  }

  double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p()->cache.mastery() * p()->spec.savant->effectN( 7 ).mastery_value();
    am *= 1.0 + p()->cache.mastery() * p()->spec.icicles_2->effectN( 4 ).mastery_value();

    return am;
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( splinterstorm && p()->specialization() == MAGE_FROST )
    {
      // Update remaining_winters_chill exactly when the remaining
      // travel time matches that of Flurry.
      double distance = player->get_player_distance( *target );
      if ( execute_state && execute_state->target )
        distance += execute_state->target->height;
      timespan_t delay = travel_time();
      delay -= timespan_t::from_seconds( std::max( distance, 0.0 ) / 50.0 );
      make_event( *sim, std::max( delay, 0_ms ), [ this, t = target ]
      {
        int wc = 2;
        // TODO: Only consider spells that impact after the splinter does
        for ( auto a : p()->winters_chill_consumers )
          wc -= as<int>( a->has_travel_events_for( t ) );
        p()->expression_support.remaining_winters_chill = std::max( 0, wc );
      } );
    }
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( controlled_instincts )
    {
      if ( auto td = find_td( s->target ); td && td->debuffs.controlled_instincts->check() )
      {
        double pct = p()->talents.controlled_instincts->effectN( p()->specialization() == MAGE_FROST ? 4 : 1 ).percent();
        controlled_instincts->execute_on_target( s->target, pct * s->result_total );
      }
    }

    if ( p()->accumulated_rng.spellfrost_teachings->trigger() )
    {
      p()->cooldowns.frozen_orb->reset( true );
      if ( p()->action.spellfrost_arcane_orb && p()->target )
        p()->action.spellfrost_arcane_orb->execute_on_target( p()->target );
      if ( p()->specialization() == MAGE_FROST )
        p()->buffs.spellfrost_teachings->trigger();
    }

    if ( splinterstorm && p()->specialization() == MAGE_FROST )
      trigger_winters_chill( s );
  }
};

// ==========================================================================
// Mage Custom Actions
// ==========================================================================

struct proxy_action_t : public action_t
{
  action_t*& action;

  proxy_action_t( std::string_view n, mage_t* p, std::string_view options_str, action_t*& a ) :
    action_t( ACTION_OTHER, n, p ),
    action( a )
  {
    parse_options( options_str );
    trigger_gcd = 0_ms;
    may_miss = may_crit = callbacks = false;
    dual = usable_while_casting = ignore_false_positive = true;
  }

  mage_t* p() const
  { return debug_cast<mage_t*>( player ); }

  pet_t* pet() const
  { return p()->pets.water_elemental; }

  bool ready() override
  {
    if ( !pet() || pet()->is_sleeping() || pet()->buffs.stunned->check() )
      return false;

    // Pet is about to die, don't let it start a new cast.
    if ( p()->buffs.icy_veins->remains() == 0_ms )
      return false;

    // Make sure the cooldown is actually ready and not just within cooldown tolerance.
    if ( !action->cooldown->up() || !action->ready() )
      return false;

    return action_t::ready();
  }

  void execute() override = 0;
};

struct freeze_t final : public proxy_action_t
{
  freeze_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    proxy_action_t( n, p, options_str, p->action.pet_freeze )
  { }

  void execute() override
  {
    if ( pet()->is_sleeping() || pet()->buffs.stunned->check() )
      return;

    action->execute_on_target( target );
  }
};

struct water_jet_t final : public proxy_action_t
{
  water_jet_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    proxy_action_t( n, p, options_str, p->action.pet_water_jet )
  { }

  void init_finished() override
  {
    proxy_action_t::init_finished();

    // Prevent the pet from casting Water Jet on its own
    if ( pet() )
    {
      for ( auto a : pet()->action_list )
        if ( a != action && a->name_str == "water_jet" ) a->background = true;
    }
  }

  void execute() override
  {
    if ( target->is_sleeping() || pet()->is_sleeping() || pet()->buffs.stunned->check() )
      return;

    pet()->interrupt();
    event_t::cancel( pet()->readying );

    action->set_target( target );
    action->schedule_execute();
  }

  bool ready() override
  {
    // Prevent recasting if Water Elemental is already executing Water Jet
    return proxy_action_t::ready() && pet()->executing != action;
  }
};

}  // namespace actions


namespace events {

struct mage_event_t : public event_t
{
  mage_t* mage;
  mage_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ),
    mage( &m )
  { }
};

struct enlightened_event_t final : public mage_event_t
{
  enlightened_event_t( mage_t& m, timespan_t delta_time ) :
    mage_event_t( m, delta_time )
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

struct icicle_event_t final : public mage_event_t
{
  player_t* target;

  icicle_event_t( mage_t& m, player_t* t, bool first = false ) :
    mage_event_t( m, first ? 0.25_s : 0.4_s * m.cache.spell_cast_speed() ),
    target( t )
  { }

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
    mage->procs.icicles_fired->occur();

    if ( !mage->icicles.empty() )
    {
      mage->events.icicle = make_event<icicle_event_t>( sim(), *mage, target );
      sim().print_debug( "{} icicle use on {} (chained), total={}", mage->name(), target->name(), mage->icicles.size() );
    }
  }
};

struct flame_accelerant_event_t final : public mage_event_t
{
  flame_accelerant_event_t( mage_t& m, timespan_t delta_time ) :
    mage_event_t( m, delta_time )
  { }

  const char* name() const override
  { return "flame_accelerant_event"; }

  void execute() override
  {
    mage->events.flame_accelerant = nullptr;
    mage->buffs.flame_accelerant->trigger();
    mage->events.flame_accelerant = make_event<flame_accelerant_event_t>( sim(), *mage, mage->talents.flame_accelerant->effectN( 1 ).period() );
  }
};

struct merged_buff_execute_event_t final : public mage_event_t
{
  merged_buff_execute_event_t( mage_t& m ) :
    mage_event_t( m, 0_ms )
  { }

  const char* name() const override
  { return "merged_buff_execute_event"; }

  void execute() override
  {
    mage->events.merged_buff_execute = nullptr;
    for ( const auto& b : mage->buff_queue )
    {
      if ( b.expire )
        b.buff->expire();
      if ( b.stacks > 0 )
        b.buff->trigger( b.stacks );
    }
    mage->buff_queue.clear();
  }
};

struct time_anomaly_tick_event_t final : public mage_event_t
{
  enum ta_proc_type_e
  {
    TA_ARCANE_SURGE,
    TA_CLEARCASTING,
    TA_COMBUSTION,
    TA_FIRE_BLAST,
    TA_ICY_VEINS,
    TA_BRAIN_FREEZE,
    TA_TIME_WARP
  };

  time_anomaly_tick_event_t( mage_t& m, timespan_t delta_time ) :
    mage_event_t( m, delta_time )
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

      // TODO: these conditions haven't been tested
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
      if ( spec == MAGE_FROST && !mage->buffs.brain_freeze->check() )
        possible_procs.push_back( TA_BRAIN_FREEZE );
      if ( !mage->buffs.time_warp->check() && ( !mage->player_t::buffs.bloodlust->check() || !mage->options.treat_bloodlust_as_time_warp ) )
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
            mage->trigger_clearcasting( 1.0, 0_ms );
            break;
          case TA_COMBUSTION:
            mage->buffs.combustion->trigger( 1000 * mage->talents.time_anomaly->effectN( 4 ).time_value() );
            mage->trigger_flash_freezeburn();
            break;
          case TA_FIRE_BLAST:
            mage->cooldowns.fire_blast->reset( true );
            break;
          case TA_BRAIN_FREEZE:
            // TODO: figure out the delay
            mage->trigger_brain_freeze( 1.0, mage->procs.brain_freeze_time_anomaly );
            break;
          case TA_ICY_VEINS:
            mage->buffs.icy_veins->trigger( 1000 * mage->talents.time_anomaly->effectN( 5 ).time_value() );
            mage->buffs.cryopathy->trigger( mage->buffs.cryopathy->max_stack() );
            mage->trigger_flash_freezeburn();
            if ( mage->pets.water_elemental->is_sleeping() )
              mage->pets.water_elemental->summon();
            break;
          case TA_TIME_WARP:
            mage->buffs.time_warp->trigger();
            break;
          default:
            assert( false );
            break;
        }
      }
    }

    mage->events.time_anomaly = make_event<time_anomaly_tick_event_t>(
      sim(), *mage, mage->talents.time_anomaly->effectN( 1 ).period() );
  }
};

struct splinterstorm_event_t final : public mage_event_t
{
  splinterstorm_event_t( mage_t& m, timespan_t delta_time ) :
    mage_event_t( m, delta_time )
  { }

  const char* name() const override
  { return "splinterstorm_event"; }

  void execute() override
  {
    mage->events.splinterstorm = nullptr;

    if ( mage->target && !mage->target->is_sleeping() && mage->target->is_enemy()
      && mage->state.embedded_splinters >= as<int>( mage->talents.splinterstorm->effectN( 1 ).base_value() ) )
    {
      [[maybe_unused]] int splinters_state = mage->state.embedded_splinters;
      int splinters = 0;
      while ( !mage->embedded_splinters.empty() )
      {
        dot_t* d = mage->embedded_splinters.back();
        assert( d->is_ticking() );

        // calculate_tick_amount destructively modifies the state, make a copy and exclude crit damage
        auto new_state = d->current_action->get_state( d->state );
        new_state->result = RESULT_HIT;
        double tick_damage = d->current_action->calculate_tick_amount( new_state, d->current_stack() );
        action_state_t::release( new_state );

        double ticks_left = d->ticks_left_fractional();
        sim().print_debug( "Recalling splinter, tick damage: {}, remaining ticks: {}", tick_damage, ticks_left );
        mage->action.splinter_recall->execute_on_target( d->target, ticks_left * tick_damage );
        splinters += d->current_stack();
        d->cancel();
      }
      assert( mage->state.embedded_splinters == 0 );
      assert( splinters == splinters_state );

      make_repeating_event( sim(), 100_ms, [ a = mage->action.splinterstorm, t = mage->target ] { a->execute_on_target( t ); }, splinters );
    }

    mage->events.splinterstorm = make_event<splinterstorm_event_t>(
      sim(), *mage, mage->talents.splinterstorm->effectN( 2 ).period() );
  }
};

struct meteor_burn_event_t final : public mage_event_t
{
  action_t* action;
  player_t* target;
  timespan_t pulse_time;
  timespan_t expiration;

  meteor_burn_event_t( mage_t& m, action_t* action_, player_t* target_, timespan_t pulse_time_, timespan_t expiration_ ) :
    mage_event_t( m, pulse_time_ ),
    action( action_ ),
    target( target_ ),
    pulse_time( pulse_time_ ),
    expiration( expiration_ )
  { }

  const char* name() const override
  { return "meteor_burn_event"; }

  void execute()
  {
    mage->events.meteor_burn = nullptr;

    action->execute_on_target( target );

    if ( sim().current_time() + pulse_time <= expiration )
      mage->events.meteor_burn = make_event<meteor_burn_event_t>( sim(), *mage, action, target, pulse_time, expiration );
  }
};

}  // namespace events

// ==========================================================================
// Mage Character Definition
// ==========================================================================

mage_td_t::mage_td_t( player_t* target, mage_t* mage ) :
  actor_target_data_t( target, mage ),
  debuffs()
{
  // Baseline
  // TODO: For some reason, the debuff has a base value of 0.5 and then the talent modifies the
  // effect by adding 0.5/1.0 on top (depending on the rank). The value is then rounded, resulting
  // in 1% damage increase with 1 rank and 2% damage increase with 2 ranks.
  debuffs.arcane_debilitation    = make_buff( *this, "arcane_debilitation", mage->find_spell( 453599 ) )
                                     ->set_default_value( ( mage->bugs ? 2.0 : 1.0 ) * mage->talents.arcane_debilitation->effectN( 2 ).percent() )
                                     ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                                     ->set_chance( mage->talents.arcane_debilitation.ok() );
  // TODO: The 0.5 from the talent is rounded to 1, increasing Ignite damage by 50% at max stacks.
  debuffs.controlled_destruction = make_buff( *this, "controlled_destruction", mage->find_spell( 453268 ) )
                                     ->set_default_value( util::round( mage->talents.controlled_destruction->effectN( 1 ).percent(), mage->bugs ? 2 : 3 ) )
                                     ->set_chance( mage->talents.controlled_destruction.ok() );
  debuffs.controlled_instincts   = make_buff( *this, "controlled_instincts", mage->find_spell( mage->specialization() == MAGE_FROST ? 463192 : 454214 ) )
                                     ->set_chance( mage->talents.controlled_instincts.ok() );
  debuffs.frozen                 = make_buff( *this, "frozen" )
                                     ->set_refresh_behavior( buff_refresh_behavior::MAX );
  debuffs.improved_scorch        = make_buff( *this, "improved_scorch", mage->find_spell( 383608 ) )
                                     ->set_default_value_from_effect( 1 );
  debuffs.magis_spark            = make_buff( *this, "magis_spark", mage->find_spell( 450004 ) );
  debuffs.magis_spark_ab         = make_buff( *this, "magis_spark_arcane_blast", mage->find_spell( 453912 ) );
  debuffs.magis_spark_abar       = make_buff( *this, "magis_spark_arcane_barrage", mage->find_spell( 453911 ) );
  debuffs.magis_spark_am         = make_buff( *this, "magis_spark_arcane_missiles", mage->find_spell( 453898 ) );
  debuffs.molten_fury            = make_buff( *this, "molten_fury", mage->find_spell( 458910 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_chance( mage->talents.molten_fury.ok() );
  debuffs.nether_munitions       = make_buff( *this, "nether_munitions", mage->find_spell( 454004 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_chance( mage->talents.nether_munitions.ok() );
  debuffs.numbing_blast          = make_buff( *this, "numbing_blast", mage->find_spell( 417490 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_chance( mage->talents.glacial_assault.ok() );
  debuffs.touch_of_the_magi      = make_buff<buffs::touch_of_the_magi_t>( this );
  debuffs.winters_chill          = make_buff( *this, "winters_chill", mage->find_spell( 228358 ) );
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
  shuffled_rng(),
  rppm(),
  accumulated_rng(),
  sample_data(),
  spec(),
  state(),
  expression_support(),
  talents()
{
  // Cooldowns
  cooldowns.arcane_echo        = get_cooldown( "arcane_echo_icd"    );
  cooldowns.blast_wave         = get_cooldown( "blast_wave"         );
  cooldowns.combustion         = get_cooldown( "combustion"         );
  cooldowns.comet_storm        = get_cooldown( "comet_storm"        );
  cooldowns.cone_of_cold       = get_cooldown( "cone_of_cold"       );
  cooldowns.dragons_breath     = get_cooldown( "dragons_breath"     );
  cooldowns.fire_blast         = get_cooldown( "fire_blast"         );
  cooldowns.flurry             = get_cooldown( "flurry"             );
  cooldowns.from_the_ashes     = get_cooldown( "from_the_ashes"     );
  cooldowns.frost_nova         = get_cooldown( "frost_nova"         );
  cooldowns.frozen_orb         = get_cooldown( "frozen_orb"         );
  cooldowns.meteor             = get_cooldown( "meteor"             );
  cooldowns.phoenix_flames     = get_cooldown( "phoenix_flames"     );
  cooldowns.presence_of_mind   = get_cooldown( "presence_of_mind"   );
  cooldowns.pyromaniac         = get_cooldown( "pyromaniac"         );

  // Options
  resource_regeneration = regen_type::DYNAMIC;
}

action_t* mage_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace actions;

  if ( talents.frostfire_bolt.ok() && ( name == "fireball" || name == "frostbolt" ) )
    return create_action( "frostfire_bolt", options_str );

  if ( talents.gravity_lapse.ok() && name == "supernova" )
    return create_action( "gravity_lapse", options_str );

  // Arcane
  if ( name == "arcane_barrage"    ) return new    arcane_barrage_t( name, this, options_str );
  if ( name == "arcane_blast"      ) return new      arcane_blast_t( name, this, options_str );
  if ( name == "arcane_missiles"   ) return new   arcane_missiles_t( name, this, options_str );
  if ( name == "arcane_orb"        ) return new        arcane_orb_t( name, this, options_str );
  if ( name == "arcane_surge"      ) return new      arcane_surge_t( name, this, options_str );
  if ( name == "evocation"         ) return new         evocation_t( name, this, options_str );
  if ( name == "presence_of_mind"  ) return new  presence_of_mind_t( name, this, options_str );
  if ( name == "touch_of_the_magi" ) return new touch_of_the_magi_t( name, this, options_str );

  // Fire
  if ( name == "combustion"        ) return new        combustion_t( name, this, options_str );
  if ( name == "fireball"          ) return new          fireball_t( name, this, options_str );
  if ( name == "flamestrike"       ) return new       flamestrike_t( name, this, options_str );
  if ( name == "meteor"            ) return new            meteor_t( name, this, options_str );
  if ( name == "phoenix_flames"    ) return new    phoenix_flames_t( name, this, options_str );
  if ( name == "pyroblast"         ) return new         pyroblast_t( name, this, options_str );
  if ( name == "scorch"            ) return new            scorch_t( name, this, options_str );

  // Frost
  if ( name == "blizzard"          ) return new          blizzard_t( name, this, options_str );
  if ( name == "cold_snap"         ) return new         cold_snap_t( name, this, options_str );
  if ( name == "comet_storm"       ) return new       comet_storm_t( name, this, options_str );
  if ( name == "flurry"            ) return new            flurry_t( name, this, options_str );
  if ( name == "frozen_orb"        ) return new        frozen_orb_t( name, this, options_str );
  if ( name == "glacial_spike"     ) return new     glacial_spike_t( name, this, options_str );
  if ( name == "ice_lance"         ) return new         ice_lance_t( name, this, options_str );
  if ( name == "icy_veins"         ) return new         icy_veins_t( name, this, options_str );
  if ( name == "ray_of_frost"      ) return new      ray_of_frost_t( name, this, options_str );

  if ( name == "freeze"            ) return new            freeze_t( name, this, options_str );
  if ( name == "water_jet"         ) return new         water_jet_t( name, this, options_str );

  // Shared
  if ( name == "arcane_explosion"  ) return new  arcane_explosion_t( name, this, options_str );
  if ( name == "arcane_intellect"  ) return new  arcane_intellect_t( name, this, options_str );
  if ( name == "blast_wave"        ) return new        blast_wave_t( name, this, options_str );
  if ( name == "blink"             ) return new             blink_t( name, this, options_str );
  if ( name == "cone_of_cold"      ) return new      cone_of_cold_t( name, this, options_str );
  if ( name == "counterspell"      ) return new      counterspell_t( name, this, options_str );
  if ( name == "dragons_breath"    ) return new    dragons_breath_t( name, this, options_str );
  if ( name == "fire_blast"        ) return new        fire_blast_t( name, this, options_str );
  if ( name == "frost_nova"        ) return new        frost_nova_t( name, this, options_str );
  if ( name == "frostbolt"         ) return new         frostbolt_t( name, this, options_str );
  if ( name == "gravity_lapse"     ) return new     gravity_lapse_t( name, this, options_str );
  if ( name == "ice_floes"         ) return new         ice_floes_t( name, this, options_str );
  if ( name == "ice_nova"          ) return new          ice_nova_t( name, this, options_str );
  if ( name == "mirror_image"      ) return new      mirror_image_t( name, this, options_str );
  if ( name == "shifting_power"    ) return new    shifting_power_t( name, this, options_str );
  if ( name == "shimmer"           ) return new           shimmer_t( name, this, options_str );
  if ( name == "slow"              ) return new              slow_t( name, this, options_str );
  if ( name == "supernova"         ) return new         supernova_t( name, this, options_str );
  if ( name == "time_warp"         ) return new         time_warp_t( name, this, options_str );

  // Special
  if ( name == "blink_any" || name == "any_blink" )
    return create_action( talents.shimmer.ok() ? "shimmer" : "blink", options_str );

  if ( name == "frostfire_bolt" )
    return specialization() == MAGE_FIRE
         ? static_cast<action_t*>( new  fireball_t( name, this, options_str, true ) )
         : static_cast<action_t*>( new frostbolt_t( name, this, options_str, true ) );

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

  if ( talents.lit_fuse.ok() || talents.explosivo.ok() || talents.deep_impact.ok() )
    action.living_bomb = get_action<living_bomb_dot_t>( "living_bomb", this );

  if ( talents.glacial_assault.ok() )
    action.glacial_assault = get_action<glacial_assault_t>( "glacial_assault", this );

  if ( talents.touch_of_the_magi.ok() )
    action.touch_of_the_magi_explosion = get_action<touch_of_the_magi_explosion_t>( "touch_of_the_magi_explosion", this );

  if ( talents.firefall.ok() )
    action.firefall_meteor = get_action<meteor_t>( "firefall_meteor", this, "", meteor_type::FIREFALL );

  if ( talents.cold_front.ok() )
    action.cold_front_frozen_orb = get_action<frozen_orb_t>( "cold_front_frozen_orb", this, "", true );

  if ( talents.magis_spark.ok() )
  {
    action.magis_spark = get_action<magis_spark_t>( "magis_spark", this );
    action.magis_spark_echo = get_action<magis_spark_echo_t>( "magis_spark_echo", this );
  }

  if ( talents.leydrinker.ok() )
    action.leydrinker_echo = get_action<leydrinker_echo_t>( "leydrinker_echo", this );

  if ( talents.dematerialize.ok() )
    action.dematerialize = get_action<dematerialize_t>( "dematerialize", this );

  if ( talents.energy_reconstitution.ok() )
    action.arcane_explosion_energy_recon = get_action<arcane_explosion_t>( "arcane_explosion_energy_reconstitution", this, "", ae_type::ENERGY_RECON );

  if ( talents.frostfire_infusion.ok() )
    action.frostfire_infusion = get_action<frostfire_infusion_t>( "frostfire_infusion", this );

  if ( talents.frostfire_empowerment.ok() || talents.flash_freezeburn.ok() )
    action.frostfire_empowerment = get_action<frostfire_empowerment_t>( "frostfire_empowerment", this );

  if ( talents.excess_frost.ok() )
    action.excess_ice_nova = get_action<ice_nova_t>( "excess_ice_nova", this, "", true );

  if ( specialization() == MAGE_ARCANE && talents.spellfrost_teachings.ok() )
    action.spellfrost_arcane_orb = get_action<arcane_orb_t>( "spellfrost_arcane_orb", this, "", ao_type::SPELLFROST );

  if ( talents.excess_fire.ok() )
    action.excess_living_bomb = get_action<living_bomb_dot_t>( "excess_living_bomb", this, true );

  if ( talents.isothermic_core.ok() )
  {
    if ( specialization() == MAGE_FIRE )
      action.isothermic_comet_storm = get_action<comet_storm_t>( "isothermic_comet_storm", this, "", true );
    if ( specialization() == MAGE_FROST )
      action.isothermic_meteor = get_action<meteor_t>( "isothermic_meteor", this, "", meteor_type::ISOTHERMIC );
  }

  if ( talents.volatile_magic.ok() )
    action.volatile_magic = get_action<volatile_magic_t>( "volatile_magic", this );

  if ( talents.splinterstorm.ok() )
    action.splinter_recall = get_action<splinter_recall_t>( "splinter_recall", this );

  // Always create the splinterstorm action so that it can be referenced by the APL.
  if ( specialization() != MAGE_FIRE )
    action.splinterstorm = get_action<splinter_t>( "splinterstorm", this, true );

  // Create Splinters last so that the previous actions can be easily added as children
  if ( talents.splintering_sorcery.ok() )
  {
    action.splinter_dot = get_action<embedded_splinter_t>( specialization() == MAGE_FROST ? "embedded_frost_splinter" : "embedded_arcane_splinter", this );
    action.splinter = get_action<splinter_t>( specialization() == MAGE_FROST ? "frost_splinter" : "arcane_splinter", this );
  }

  if ( talents.glorious_incandescence.ok() )
    action.meteorite = get_action<meteorite_t>( "meteorite", this );

  player_t::create_actions();

  // Ensure the cooldown of Phoenix Flames is properly initialized.
  if ( talents.from_the_ashes.ok() && !find_action( "phoenix_flames" ) )
    create_action( "phoenix_flames", "" );
}

void mage_t::create_options()
{
  add_option( opt_timespan( "mage.frozen_duration", options.frozen_duration ) );
  add_option( opt_timespan( "mage.scorch_delay", options.scorch_delay ) );
  add_option( opt_timespan( "mage.arcane_missiles_chain_delay", options.arcane_missiles_chain_delay, 0_ms, timespan_t::max() ) );
  add_option( opt_float( "mage.arcane_missiles_chain_relstddev", options.arcane_missiles_chain_relstddev, 0.0, std::numeric_limits<double>::max() ) );
  add_option( opt_timespan( "mage.glacial_spike_delay", options.glacial_spike_delay, 0_ms, timespan_t::max() ) );
  add_option( opt_bool( "mage.treat_bloodlust_as_time_warp", options.treat_bloodlust_as_time_warp ) );
  add_option( opt_uint( "mage.initial_spellfire_spheres", options.initial_spellfire_spheres ) );
  add_option( opt_func( "mage.arcane_phoenix_rotation_override", [ this ] ( sim_t*, util::string_view name, util::string_view value )
              {
                if ( value.empty() || value == "default" )
                  options.arcane_phoenix_rotation_override = arcane_phoenix_rotation::DEFAULT;
                else if ( value == "st" )
                  options.arcane_phoenix_rotation_override = arcane_phoenix_rotation::ST;
                else if ( value == "aoe" )
                  options.arcane_phoenix_rotation_override = arcane_phoenix_rotation::AOE;
                else
                  throw std::invalid_argument( "valid options are 'default', 'st', and 'aoe'." );
                return true;
              } ) );

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
    case MAGE_FIRE:
      sample_data.low_mana_iteration->merge( *mage.sample_data.low_mana_iteration );
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
    case MAGE_FIRE:
      if ( double low_mana_mean = sample_data.low_mana_iteration->mean(); low_mana_mean > 0.1 )
        sim->error( "{}: Actor went below 10% mana in a significant fraction of iterations ({:.1f}%)", *this,
                    100.0 * low_mana_mean );
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

  if ( specialization() == MAGE_FIRE )
    sample_data.low_mana_iteration->add( as<double>( state.had_low_mana ) );
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

  if ( ( talents.icy_veins.ok() && find_action( "icy_veins" ) ) || ( specialization() == MAGE_FROST && talents.time_anomaly.ok() ) )
    pets.water_elemental = new pets::water_elemental::water_elemental_pet_t( sim, this );

  if ( talents.mirror_image.ok() && find_action( "mirror_image" ) )
  {
    int images = as<int>( talents.mirror_image->effectN( 2 ).base_value() );
    images += as<int>( talents.phantasmal_image->effectN( 1 ).base_value() );
    for ( int i = 0; i < images; i++ )
    {
      auto image = new pets::mirror_image::mirror_image_pet_t( sim, this );
      if ( i > 0 )
        image->quiet = true;
      pets.mirror_images.push_back( image );
    }
  }

  if ( talents.invocation_arcane_phoenix.ok() && ( find_action( "arcane_surge" ) || find_action( "combustion") ) )
    pets.arcane_phoenix = new pets::arcane_phoenix::arcane_phoenix_pet_t( sim, this );
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
  talents.incanters_flow           = find_talent_spell( talent_tree::CLASS, "Incanter's Flow"          );
  // Row 3
  talents.winters_protection       = find_talent_spell( talent_tree::CLASS, "Winter's Protection"      );
  talents.spellsteal               = find_talent_spell( talent_tree::CLASS, "Spellsteal"               );
  talents.tempest_barrier          = find_talent_spell( talent_tree::CLASS, "Tempest Barrier"          );
  talents.incantation_of_swiftness = find_talent_spell( talent_tree::CLASS, "Incantation of Swiftness" );
  talents.remove_curse             = find_talent_spell( talent_tree::CLASS, "Remove Curse"             );
  talents.arcane_warding           = find_talent_spell( talent_tree::CLASS, "Arcane Warding"           );
  // Row 4
  talents.mirror_image             = find_talent_spell( talent_tree::CLASS, "Mirror Image"             );
  talents.shifting_power           = find_talent_spell( talent_tree::CLASS, "Shifting Power"           );
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
  talents.ice_nova                 = find_talent_spell( talent_tree::CLASS, "Ice Nova"                 );
  talents.ring_of_frost            = find_talent_spell( talent_tree::CLASS, "Ring of Frost"            );
  talents.ice_floes                = find_talent_spell( talent_tree::CLASS, "Ice Floes"                );
  talents.shimmer                  = find_talent_spell( talent_tree::CLASS, "Shimmer"                  );
  talents.blast_wave               = find_talent_spell( talent_tree::CLASS, "Blast Wave"               );
  // Row 7
  talents.improved_frost_nova      = find_talent_spell( talent_tree::CLASS, "Improved Frost Nova"      );
  talents.rigid_ice                = find_talent_spell( talent_tree::CLASS, "Rigid Ice"                );
  talents.tome_of_rhonin           = find_talent_spell( talent_tree::CLASS, "Tome of Rhonin"           );
  talents.dragons_breath           = find_talent_spell( talent_tree::CLASS, "Dragon's Breath"          );
  talents.supernova                = find_talent_spell( talent_tree::CLASS, "Supernova"                );
  talents.tome_of_antonidas        = find_talent_spell( talent_tree::CLASS, "Tome of Antonidas"        );
  talents.volatile_detonation      = find_talent_spell( talent_tree::CLASS, "Volatile Detonation"      );
  talents.energized_barriers       = find_talent_spell( talent_tree::CLASS, "Energized Barriers"       );
  // Row 8
  talents.frigid_winds             = find_talent_spell( talent_tree::CLASS, "Frigid Winds"             );
  talents.flow_of_time             = find_talent_spell( talent_tree::CLASS, "Flow of Time"             );
  talents.temporal_velocity        = find_talent_spell( talent_tree::CLASS, "Temporal Velocity"        );
  // Row 9
  talents.ice_ward                 = find_talent_spell( talent_tree::CLASS, "Ice Ward"                 );
  talents.freezing_cold            = find_talent_spell( talent_tree::CLASS, "Freezing Cold"            );
  talents.time_manipulation        = find_talent_spell( talent_tree::CLASS, "Time Manipulation"        );
  talents.displacement             = find_talent_spell( talent_tree::CLASS, "Displacement"             );
  talents.accumulative_shielding   = find_talent_spell( talent_tree::CLASS, "Accumulative Shielding"   );
  talents.greater_invisibility     = find_talent_spell( talent_tree::CLASS, "Greater Invisibility"     );
  talents.barrier_diffusion        = find_talent_spell( talent_tree::CLASS, "Barrier Diffusion"        );
  // Row 10
  talents.ice_cold                 = find_talent_spell( talent_tree::CLASS, "Ice Cold"                 );
  talents.inspired_intellect       = find_talent_spell( talent_tree::CLASS, "Inspired Intellect"       );
  talents.time_anomaly             = find_talent_spell( talent_tree::CLASS, "Time Anomaly"             );
  talents.mass_barrier             = find_talent_spell( talent_tree::CLASS, "Mass Barrier"             );
  talents.mass_invisibility        = find_talent_spell( talent_tree::CLASS, "Mass Invisibility"        );

  // Arcane
  // Row 1
  talents.arcane_missiles            = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Missiles"            );
  // Row 2
  talents.amplification              = find_talent_spell( talent_tree::SPECIALIZATION, "Amplification"              );
  talents.nether_precision           = find_talent_spell( talent_tree::SPECIALIZATION, "Nether Precision"           );
  // Row 3
  talents.charged_orb                = find_talent_spell( talent_tree::SPECIALIZATION, "Charged Orb"                );
  talents.arcane_tempo               = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Tempo"               );
  talents.concentrated_power         = find_talent_spell( talent_tree::SPECIALIZATION, "Concentrated Power"         );
  talents.consortiums_bauble         = find_talent_spell( talent_tree::SPECIALIZATION, "Consortium's Bauble"        );
  talents.arcing_cleave              = find_talent_spell( talent_tree::SPECIALIZATION, "Arcing Cleave"              );
  // Row 4
  talents.arcane_familiar            = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Familiar"            );
  talents.arcane_surge               = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Surge"               );
  talents.improved_clearcasting      = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Clearcasting"      );
  // Row 5
  talents.big_brained                = find_talent_spell( talent_tree::SPECIALIZATION, "Big Brained"                );
  talents.energized_familiar         = find_talent_spell( talent_tree::SPECIALIZATION, "Energized Familiar"         );
  talents.presence_of_mind           = find_talent_spell( talent_tree::SPECIALIZATION, "Presence of Mind"           );
  talents.surging_urge               = find_talent_spell( talent_tree::SPECIALIZATION, "Surging Urge"               );
  talents.slipstream                 = find_talent_spell( talent_tree::SPECIALIZATION, "Slipstream"                 );
  talents.static_cloud               = find_talent_spell( talent_tree::SPECIALIZATION, "Static Cloud"               );
  talents.resonance                  = find_talent_spell( talent_tree::SPECIALIZATION, "Resonance"                  );
  // Row 6
  talents.impetus                    = find_talent_spell( talent_tree::SPECIALIZATION, "Impetus"                    );
  talents.touch_of_the_magi          = find_talent_spell( talent_tree::SPECIALIZATION, "Touch of the Magi"          );
  talents.dematerialize              = find_talent_spell( talent_tree::SPECIALIZATION, "Dematerialize"              );
  // Row 7
  talents.resonant_orbs              = find_talent_spell( talent_tree::SPECIALIZATION, "Resonant Orbs"              );
  talents.illuminated_thoughts       = find_talent_spell( talent_tree::SPECIALIZATION, "Illuminated Thoughts"       );
  talents.evocation                  = find_talent_spell( talent_tree::SPECIALIZATION, "Evocation"                  );
  talents.improved_touch_of_the_magi = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Touch of the Magi" );
  talents.eureka                     = find_talent_spell( talent_tree::SPECIALIZATION, "Eureka"                     );
  talents.energy_reconstitution      = find_talent_spell( talent_tree::SPECIALIZATION, "Energy Reconstitution"      );
  talents.reverberate                = find_talent_spell( talent_tree::SPECIALIZATION, "Reverberate"                );
  // Row 8
  talents.arcane_debilitation        = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Debilitation"        );
  talents.arcane_echo                = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Echo"                );
  talents.prodigious_savant          = find_talent_spell( talent_tree::SPECIALIZATION, "Prodigious Savant"          );
  // Row 9
  talents.time_loop                  = find_talent_spell( talent_tree::SPECIALIZATION, "Time Loop"                  );
  talents.aether_attunement          = find_talent_spell( talent_tree::SPECIALIZATION, "Aether Attunement"          );
  talents.enlightened                = find_talent_spell( talent_tree::SPECIALIZATION, "Enlightened"                );
  talents.arcane_bombardment         = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Bombardment"         );
  talents.leysight                   = find_talent_spell( talent_tree::SPECIALIZATION, "Leysight"                   );
  talents.concentration              = find_talent_spell( talent_tree::SPECIALIZATION, "Concentration"              );
  // Row 10
  talents.high_voltage               = find_talent_spell( talent_tree::SPECIALIZATION, "High Voltage"               );
  talents.arcane_harmony             = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Harmony"             );
  talents.magis_spark                = find_talent_spell( talent_tree::SPECIALIZATION, "Magi's Spark"               );
  talents.nether_munitions           = find_talent_spell( talent_tree::SPECIALIZATION, "Nether Munitions"           );
  talents.orb_barrage                = find_talent_spell( talent_tree::SPECIALIZATION, "Orb Barrage"                );
  talents.leydrinker                 = find_talent_spell( talent_tree::SPECIALIZATION, "Leydrinker"                 );

  // Fire
  // Row 1
  talents.pyroblast              = find_talent_spell( talent_tree::SPECIALIZATION, "Pyroblast"              );
  // Row 2
  talents.fire_blast             = find_talent_spell( talent_tree::SPECIALIZATION, "Fire Blast"             );
  talents.firestarter            = find_talent_spell( talent_tree::SPECIALIZATION, "Firestarter"            );
  talents.phoenix_flames         = find_talent_spell( talent_tree::SPECIALIZATION, "Phoenix Flames"         );
  // Row 3
  talents.pyrotechnics           = find_talent_spell( talent_tree::SPECIALIZATION, "Pyrotechnics"           );
  talents.fervent_flickering     = find_talent_spell( talent_tree::SPECIALIZATION, "Fervent Flickering"     );
  talents.call_of_the_sun_king   = find_talent_spell( talent_tree::SPECIALIZATION, "Call of the Sun King"   );
  talents.majesty_of_the_phoenix = find_talent_spell( talent_tree::SPECIALIZATION, "Majesty of the Phoenix" );
  // Row 4
  talents.scorch                 = find_talent_spell( talent_tree::SPECIALIZATION, "Scorch"                 );
  talents.surging_blaze          = find_talent_spell( talent_tree::SPECIALIZATION, "Surging Blaze"          );
  talents.lit_fuse               = find_talent_spell( talent_tree::SPECIALIZATION, "Lit Fuse"               );
  // Row 5
  talents.improved_scorch        = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Scorch"        );
  talents.scald                  = find_talent_spell( talent_tree::SPECIALIZATION, "Scald"                  );
  talents.heat_shimmer           = find_talent_spell( talent_tree::SPECIALIZATION, "Heat Shimmer"           );
  talents.flame_on               = find_talent_spell( talent_tree::SPECIALIZATION, "Flame On"               );
  talents.flame_accelerant       = find_talent_spell( talent_tree::SPECIALIZATION, "Flame Accelerant"       );
  talents.critical_mass          = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Mass"          );
  talents.sparking_cinders       = find_talent_spell( talent_tree::SPECIALIZATION, "Sparking Cinders"       );
  talents.explosive_ingenuity    = find_talent_spell( talent_tree::SPECIALIZATION, "Explosive Ingenuity"    );
  // Row 6
  talents.inflame                = find_talent_spell( talent_tree::SPECIALIZATION, "Inflame"                );
  talents.alexstraszas_fury      = find_talent_spell( talent_tree::SPECIALIZATION, "Alexstrasza's Fury"     );
  talents.ashen_feather          = find_talent_spell( talent_tree::SPECIALIZATION, "Ashen Feather"          );
  talents.intensifying_flame     = find_talent_spell( talent_tree::SPECIALIZATION, "Intensifying Flame"     );
  talents.combustion             = find_talent_spell( talent_tree::SPECIALIZATION, "Combustion"             );
  talents.controlled_destruction = find_talent_spell( talent_tree::SPECIALIZATION, "Controlled Destruction" );
  talents.flame_patch            = find_talent_spell( talent_tree::SPECIALIZATION, "Flame Patch"            );
  talents.quickflame             = find_talent_spell( talent_tree::SPECIALIZATION, "Quickflame"             );
  talents.convection             = find_talent_spell( talent_tree::SPECIALIZATION, "Convection"             );
  // Row 7
  talents.master_of_flame        = find_talent_spell( talent_tree::SPECIALIZATION, "Master of Flame"        );
  talents.wildfire               = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire"               );
  talents.improved_combustion    = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Combustion"    );
  talents.spontaneous_combustion = find_talent_spell( talent_tree::SPECIALIZATION, "Spontaneous Combustion" );
  talents.feel_the_burn          = find_talent_spell( talent_tree::SPECIALIZATION, "Feel the Burn"          );
  talents.mark_of_the_firelord   = find_talent_spell( talent_tree::SPECIALIZATION, "Mark of the Firelord"   );
  // Row 8
  talents.fevered_incantation    = find_talent_spell( talent_tree::SPECIALIZATION, "Fevered Incantation"    );
  talents.kindling               = find_talent_spell( talent_tree::SPECIALIZATION, "Kindling"               );
  talents.fires_ire              = find_talent_spell( talent_tree::SPECIALIZATION, "Fire's Ire"             );
  // Row 9
  talents.pyromaniac             = find_talent_spell( talent_tree::SPECIALIZATION, "Pyromaniac"             );
  talents.molten_fury            = find_talent_spell( talent_tree::SPECIALIZATION, "Molten Fury"            );
  talents.from_the_ashes         = find_talent_spell( talent_tree::SPECIALIZATION, "From the Ashes"         );
  talents.fiery_rush             = find_talent_spell( talent_tree::SPECIALIZATION, "Fiery Rush"             );
  talents.meteor                 = find_talent_spell( talent_tree::SPECIALIZATION, "Meteor"                 );
  talents.firefall               = find_talent_spell( talent_tree::SPECIALIZATION, "Firefall"               );
  talents.explosivo              = find_talent_spell( talent_tree::SPECIALIZATION, "Explosivo"              );
  // Row 10
  talents.hyperthermia           = find_talent_spell( talent_tree::SPECIALIZATION, "Hyperthermia"           );
  talents.phoenix_reborn         = find_talent_spell( talent_tree::SPECIALIZATION, "Phoenix Reborn"         );
  talents.sun_kings_blessing     = find_talent_spell( talent_tree::SPECIALIZATION, "Sun King's Blessing"    );
  talents.unleashed_inferno      = find_talent_spell( talent_tree::SPECIALIZATION, "Unleashed Inferno"      );
  talents.deep_impact            = find_talent_spell( talent_tree::SPECIALIZATION, "Deep Impact"            );
  talents.blast_zone             = find_talent_spell( talent_tree::SPECIALIZATION, "Blast Zone"             );

  // Frost
  // Row 1
  talents.ice_lance         = find_talent_spell( talent_tree::SPECIALIZATION, "Ice Lance"         );
  // Row 2
  talents.frozen_orb        = find_talent_spell( talent_tree::SPECIALIZATION, "Frozen Orb"        );
  talents.fingers_of_frost  = find_talent_spell( talent_tree::SPECIALIZATION, "Fingers of Frost"  );
  // Row 3
  talents.flurry            = find_talent_spell( talent_tree::SPECIALIZATION, "Flurry"            );
  talents.shatter           = find_talent_spell( talent_tree::SPECIALIZATION, "Shatter"           );
  // Row 4
  talents.brain_freeze      = find_talent_spell( talent_tree::SPECIALIZATION, "Brain Freeze"      );
  talents.everlasting_frost = find_talent_spell( talent_tree::SPECIALIZATION, "Everlasting Frost" );
  talents.winters_blessing  = find_talent_spell( talent_tree::SPECIALIZATION, "Winter's Blessing" );
  talents.frostbite         = find_talent_spell( talent_tree::SPECIALIZATION, "Frostbite"         );
  talents.piercing_cold     = find_talent_spell( talent_tree::SPECIALIZATION, "Piercing Cold"     );
  // Row 5
  talents.perpetual_winter  = find_talent_spell( talent_tree::SPECIALIZATION, "Perpetual Winter"  );
  talents.lonely_winter     = find_talent_spell( talent_tree::SPECIALIZATION, "Lonely Winter"     );
  talents.permafrost_lances = find_talent_spell( talent_tree::SPECIALIZATION, "Permafrost Lances" );
  talents.bone_chilling     = find_talent_spell( talent_tree::SPECIALIZATION, "Bone Chilling"     );
  // Row 6
  talents.comet_storm       = find_talent_spell( talent_tree::SPECIALIZATION, "Comet Storm"       );
  talents.frozen_touch      = find_talent_spell( talent_tree::SPECIALIZATION, "Frozen Touch"      );
  talents.wintertide        = find_talent_spell( talent_tree::SPECIALIZATION, "Wintertide"        );
  talents.ice_caller        = find_talent_spell( talent_tree::SPECIALIZATION, "Ice Caller"        );
  talents.flash_freeze      = find_talent_spell( talent_tree::SPECIALIZATION, "Flash Freeze"      );
  talents.subzero           = find_talent_spell( talent_tree::SPECIALIZATION, "Subzero"           );
  // Row 7
  talents.deep_shatter      = find_talent_spell( talent_tree::SPECIALIZATION, "Deep Shatter"      );
  talents.icy_veins         = find_talent_spell( talent_tree::SPECIALIZATION, "Icy Veins"         );
  talents.splintering_cold  = find_talent_spell( talent_tree::SPECIALIZATION, "Splintering Cold"  );
  // Row 8
  talents.glacial_assault   = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Assault"   );
  talents.freezing_rain     = find_talent_spell( talent_tree::SPECIALIZATION, "Freezing Rain"     );
  talents.thermal_void      = find_talent_spell( talent_tree::SPECIALIZATION, "Thermal Void"      );
  talents.splitting_ice     = find_talent_spell( talent_tree::SPECIALIZATION, "Splitting Ice"     );
  talents.chain_reaction    = find_talent_spell( talent_tree::SPECIALIZATION, "Chain Reaction"    );
  // Row 9
  talents.fractured_frost   = find_talent_spell( talent_tree::SPECIALIZATION, "Fractured Frost"   );
  talents.freezing_winds    = find_talent_spell( talent_tree::SPECIALIZATION, "Freezing Winds"    );
  talents.slick_ice         = find_talent_spell( talent_tree::SPECIALIZATION, "Slick Ice"         );
  talents.hailstones        = find_talent_spell( talent_tree::SPECIALIZATION, "Hailstones"        );
  talents.ray_of_frost      = find_talent_spell( talent_tree::SPECIALIZATION, "Ray of Frost"      );
  // Row 10
  talents.deaths_chill      = find_talent_spell( talent_tree::SPECIALIZATION, "Death's Chill"     );
  talents.cold_front        = find_talent_spell( talent_tree::SPECIALIZATION, "Cold Front"        );
  talents.coldest_snap      = find_talent_spell( talent_tree::SPECIALIZATION, "Coldest Snap"      );
  talents.glacial_spike     = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Spike"     );
  talents.cryopathy         = find_talent_spell( talent_tree::SPECIALIZATION, "Cryopathy"         );
  talents.splintering_ray   = find_talent_spell( talent_tree::SPECIALIZATION, "Splintering Ray"   );

  // Frostfire
  // Row 1
  talents.frostfire_mastery     = find_talent_spell( talent_tree::HERO, "Frostfire Mastery"     );
  // Row 2
  talents.imbued_warding        = find_talent_spell( talent_tree::HERO, "Imbued Warding"        );
  talents.meltdown              = find_talent_spell( talent_tree::HERO, "Meltdown"              );
  talents.frostfire_bolt        = find_talent_spell( talent_tree::HERO, "Frostfire Bolt"        );
  talents.elemental_affinity    = find_talent_spell( talent_tree::HERO, "Elemental Affinity"    );
  talents.flame_and_frost       = find_talent_spell( talent_tree::HERO, "Flame and Frost"       );
  // Row 3
  talents.isothermic_core       = find_talent_spell( talent_tree::HERO, "Isothermic Core"       );
  talents.severe_temperatures   = find_talent_spell( talent_tree::HERO, "Severe Temperatures"   );
  talents.thermal_conditioning  = find_talent_spell( talent_tree::HERO, "Thermal Conditioning"  );
  talents.frostfire_infusion    = find_talent_spell( talent_tree::HERO, "Frostfire Infusion"    );
  // Row 4
  talents.excess_frost          = find_talent_spell( talent_tree::HERO, "Excess Frost"          );
  talents.frostfire_empowerment = find_talent_spell( talent_tree::HERO, "Frostfire Empowerment" );
  talents.excess_fire           = find_talent_spell( talent_tree::HERO, "Excess Fire"           );
  // Row 5
  talents.flash_freezeburn      = find_talent_spell( talent_tree::HERO, "Flash Freezeburn"      );

  // Spellslinger
  // Row 1
  talents.splintering_sorcery  = find_talent_spell( talent_tree::HERO, "Splintering Sorcery"  );
  // Row 2
  talents.augury_abounds       = find_talent_spell( talent_tree::HERO, "Augury Abounds"       );
  talents.controlled_instincts = find_talent_spell( talent_tree::HERO, "Controlled Instincts" );
  talents.splintering_orbs     = find_talent_spell( talent_tree::HERO, "Splintering Orbs"     );
  // Row 3
  talents.look_again           = find_talent_spell( talent_tree::HERO, "Look Again"           );
  talents.slippery_slinging    = find_talent_spell( talent_tree::HERO, "Slippery Slinging"    );
  talents.phantasmal_image     = find_talent_spell( talent_tree::HERO, "Phantasmal Image"     );
  talents.reactive_barrier     = find_talent_spell( talent_tree::HERO, "Reactive Barrier"     );
  talents.unerring_proficiency = find_talent_spell( talent_tree::HERO, "Unerring Proficiency" );
  talents.volatile_magic       = find_talent_spell( talent_tree::HERO, "Volatile Magic"       );
  // Row 4
  talents.shifting_shards      = find_talent_spell( talent_tree::HERO, "Shifting Shards"      );
  talents.force_of_will        = find_talent_spell( talent_tree::HERO, "Force of Will"        );
  talents.spellfrost_teachings = find_talent_spell( talent_tree::HERO, "Spellfrost Teachings" );
  // Row 5
  talents.splinterstorm        = find_talent_spell( talent_tree::HERO, "Splinterstorm"        );

  // Sunfury
  // Row 1
  talents.spellfire_spheres         = find_talent_spell( talent_tree::HERO, "Spellfire Spheres"          );
  // Row 2
  talents.mana_cascade              = find_talent_spell( talent_tree::HERO, "Mana Cascade"               );
  talents.invocation_arcane_phoenix = find_talent_spell( talent_tree::HERO, "Invocation: Arcane Phoenix" );
  talents.burden_of_power           = find_talent_spell( talent_tree::HERO, "Burden of Power"            );
  // Row 3
  talents.merely_a_setback          = find_talent_spell( talent_tree::HERO, "Merely a Setback"           );
  talents.gravity_lapse             = find_talent_spell( talent_tree::HERO, "Gravity Lapse"              );
  talents.lessons_in_debilitation   = find_talent_spell( talent_tree::HERO, "Lessons in Debilitation"    );
  talents.glorious_incandescence    = find_talent_spell( talent_tree::HERO, "Glorious Incandescence"     );
  // Row 4
  talents.savor_the_moment          = find_talent_spell( talent_tree::HERO, "Savor the Moment"           );
  talents.sunfury_execution         = find_talent_spell( talent_tree::HERO, "Sunfury Execution"          );
  talents.codex_of_the_sunstriders  = find_talent_spell( talent_tree::HERO, "Codex of the Sunstriders"   );
  talents.ignite_the_future         = find_talent_spell( talent_tree::HERO, "Ignite the Future"          );
  talents.rondurmancy               = find_talent_spell( talent_tree::HERO, "Rondurmancy"                );
  // Row 5
  talents.memory_of_alar            = find_talent_spell( talent_tree::HERO, "Memory of Al'ar"            );

  // Spec Spells
  spec.arcane_charge                 = find_specialization_spell( "Arcane Charge"                 );
  spec.arcane_mage                   = find_specialization_spell( "Arcane Mage"                   );
  spec.clearcasting                  = find_specialization_spell( "Clearcasting"                  );
  spec.mana_adept                    = find_specialization_spell( "Mana Adept"                    );
  spec.fire_mage                     = find_specialization_spell( "Fire Mage"                     );
  spec.fuel_the_fire                 = find_specialization_spell( "Fuel the Fire"                 );
  spec.pyroblast_clearcasting_driver = find_specialization_spell( "Pyroblast Clearcasting Driver" );
  spec.frost_mage                    = find_specialization_spell( "Frost Mage"                    );

  // Mastery
  spec.savant    = find_mastery_spell( MAGE_ARCANE );
  spec.ignite    = find_mastery_spell( MAGE_FIRE );
  spec.icicles   = find_mastery_spell( MAGE_FROST );
  spec.icicles_2 = find_specialization_spell( "Mastery: Icicles", "Rank 2" );

  // Misc
  cooldowns.arcane_echo->duration = find_spell( 464515 )->internal_cooldown();
}

void mage_t::init_base_stats()
{
  if ( base.distance < 1.0 )
    base.distance = 10.0;

  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  // Mana Attunement
  resources.base_regen_per_second[ RESOURCE_MANA ] *= 1.0 + find_spell( 121039 )->effectN( 1 ).percent();

  for ( auto rt = RESOURCE_RAGE; rt < RESOURCE_MAX; rt++ )
    resources.active_resource[ rt ] = false;

  if ( specialization() == MAGE_ARCANE )
    regen_caches[ CACHE_MASTERY ] = true;
}

void mage_t::create_buffs()
{
  player_t::create_buffs();

  // Arcane
  buffs.aether_attunement         = make_buff( this, "aether_attunement", find_spell( 453601 ) )
                                      ->set_default_value_from_effect( 1 );
  buffs.aether_attunement_counter = make_buff( this, "aether_attunement_counter", find_spell( 458388 ) )
                                      ->set_chance( talents.aether_attunement.ok() );
  buffs.arcane_charge             = make_buff( this, "arcane_charge", find_spell( 36032 ) )
                                      ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.arcane_familiar           = make_buff( this, "arcane_familiar", find_spell( 210126 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_period( 3.0_s )
                                      ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
                                      ->set_tick_callback( [ this ] ( buff_t*, int, timespan_t )
                                        {
                                          action.arcane_assault->execute_on_target( target );
                                          if ( talents.energized_familiar.ok() && buffs.arcane_surge->check() )
                                          {
                                            // TODO: talent says it does 4 instead of 1, but seems to just be +4 in game
                                            int count = as<int>( talents.energized_familiar->effectN( 1 ).base_value() );
                                            make_repeating_event( *sim, 75_ms, [ this ] { action.arcane_assault->execute_on_target( target ); }, count );
                                          }
                                        } )
                                      ->set_stack_change_callback( [ this ] ( buff_t*, int, int )
                                        { recalculate_resource_max( RESOURCE_MANA ); } )
                                      ->set_chance( talents.arcane_familiar.ok() );
  buffs.arcane_harmony            = make_buff( this, "arcane_harmony", find_spell( 384455 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_chance( talents.arcane_harmony.ok() )
                                      ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.arcane_surge              = make_buff( this, "arcane_surge", find_spell( 365362 ) )
                                      ->set_default_value_from_effect( 3 )
                                      ->set_affects_regen( true );
  buffs.arcane_tempo              = make_buff( this, "arcane_tempo", find_spell( 383997 ) )
                                      ->set_default_value( talents.arcane_tempo->effectN( 1 ).percent() )
                                      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                      ->set_chance( talents.arcane_tempo.ok() );
  buffs.big_brained               = make_buff( this, "big_brained", find_spell( 461531 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                                      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                                      ->set_chance( talents.big_brained.ok() );
  buffs.clearcasting              = make_buff( this, "clearcasting", find_spell( 263725 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->modify_max_stack( as<int>( talents.improved_clearcasting->effectN( 1 ).base_value() ) )
                                      ->set_chance( spec.clearcasting->ok() ) ;
  buffs.clearcasting_channel      = make_buff( this, "clearcasting_channel", find_spell( 277726 ) )
                                      ->set_quiet( true );
  buffs.concentration             = make_buff( this, "concentration", find_spell( 384379 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_activated( false )
                                      ->set_trigger_spell( talents.concentration );
  buffs.enlightened_damage        = make_buff( this, "enlightened_damage", find_spell( 321388 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.enlightened_mana          = make_buff( this, "enlightened_mana", find_spell( 321390 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_affects_regen( true );
  buffs.evocation                 = make_buff( this, "evocation", find_spell( 12051 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_cooldown( 0_ms )
                                      ->set_affects_regen( true );
  buffs.high_voltage              = make_buff( this, "high_voltage", find_spell( 461525 ) )
                                      ->set_default_value_from_effect( 1, 0.01 );
  buffs.impetus                   = make_buff( this, "impetus", find_spell( 393939 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.leydrinker                = make_buff( this, "leydrinker", find_spell( 453758 ) )
                                      ->set_chance( talents.leydrinker->effectN( 1 ).percent() );
  buffs.nether_precision          = make_buff( this, "nether_precision", find_spell( 383783 ) )
                                      ->set_default_value( talents.nether_precision->effectN( 1 ).percent() )
                                      ->modify_default_value( talents.leysight->effectN( 1 ).percent() )
                                      ->set_activated( false )
                                      ->set_chance( talents.nether_precision.ok() );
  buffs.presence_of_mind          = make_buff( this, "presence_of_mind", find_spell( 205025 ) )
                                      ->set_cooldown( 0_ms )
                                      ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                        { if ( cur == 0 ) cooldowns.presence_of_mind->start( cooldowns.presence_of_mind->action ); } );
  buffs.siphon_storm              = make_buff( this, "siphon_storm", find_spell( 384267 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );
  buffs.static_cloud              = make_buff( this, "static_cloud", find_spell( 461515 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_chance( talents.static_cloud.ok() );


  // Fire
  buffs.calefaction              = make_buff( this, "calefaction", find_spell( 408673 ) )
                                     ->set_chance( talents.phoenix_reborn.ok() );
  buffs.combustion               = make_buff<buffs::combustion_t>( this );
  buffs.feel_the_burn            = make_buff( this, "feel_the_burn", find_spell( 383395 ) )
                                     ->set_default_value( talents.feel_the_burn->effectN( 1 ).base_value() )
                                     ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
                                     ->set_cooldown( find_spell( 383394 )->internal_cooldown() )
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
  buffs.flame_accelerant         = make_buff( this, "flame_accelerant", find_spell( 453283 ) )
                                     ->set_default_value_from_effect( 2 )
                                     ->set_chance( talents.flame_accelerant.ok() )
                                     ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.flames_fury              = make_buff( this, "flames_fury", find_spell( 409964 ) )
                                     ->set_default_value_from_effect( 1 );
  buffs.frenetic_speed           = make_buff( this, "frenetic_speed", find_spell( 236060 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->add_invalidate( CACHE_RUN_SPEED )
                                     ->set_chance( talents.scorch.ok() );
  buffs.fury_of_the_sun_king     = make_buff( this, "fury_of_the_sun_king", find_spell( 383883 ) )
                                     ->set_default_value_from_effect( 2 );
  buffs.heat_shimmer             = make_buff( this, "heat_shimmer", find_spell( 458964 ) )
                                     ->set_trigger_spell( talents.heat_shimmer );
  buffs.heating_up               = make_buff( this, "heating_up", find_spell( 48107 ) );
  buffs.hot_streak               = make_buff( this, "hot_streak", find_spell( 48108 ) );
  buffs.hyperthermia             = make_buff( this, "hyperthermia", find_spell( 383874 ) )
                                     ->set_default_value_from_effect( 2 )
                                     ->set_trigger_spell( talents.hyperthermia );
  buffs.lit_fuse                 = make_buff( this, "lit_fuse", find_spell( 453207 ) )
                                     ->set_chance( talents.lit_fuse.ok() || talents.explosivo.ok() );
  buffs.majesty_of_the_phoenix   = make_buff( this, "majesty_of_the_phoenix", find_spell( 453329 ) )
                                     ->set_chance( talents.majesty_of_the_phoenix.ok() );
  buffs.pyrotechnics             = make_buff( this, "pyrotechnics", find_spell( 157644 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_chance( talents.pyrotechnics.ok() );
  buffs.sparking_cinders         = make_buff( this, "sparking_cinders", find_spell( 457729 ) )
                                     ->set_trigger_spell( talents.sparking_cinders );
  buffs.sun_kings_blessing       = make_buff( this, "sun_kings_blessing", find_spell( 383882 ) )
                                     ->set_chance( talents.sun_kings_blessing.ok() );
  buffs.wildfire                 = make_buff( this, "wildfire", find_spell( 383492 ) )
                                     ->set_default_value( talents.wildfire->effectN( 3 ).percent() )
                                     ->set_chance( talents.wildfire.ok() );


  // Frost
  buffs.bone_chilling      = make_buff( this, "bone_chilling", find_spell( 205766 ) )
                               ->set_default_value( 0.1 * talents.bone_chilling->effectN( 1 ).percent() )
                               ->set_chance( talents.bone_chilling.ok() );
  buffs.chain_reaction     = make_buff( this, "chain_reaction", find_spell( 278310 ) )
                               ->set_default_value( talents.chain_reaction->effectN( 1 ).percent() )
                               ->set_chance( talents.chain_reaction.ok() );
  buffs.brain_freeze       = make_buff( this, "brain_freeze", find_spell( 190446 ) );
  buffs.cold_front         = make_buff( this, "cold_front", find_spell( 382113 ) )
                               ->set_chance( talents.cold_front.ok() );
  buffs.cold_front_ready   = make_buff( this, "cold_front_ready", find_spell( 382114 ) );
  buffs.cryopathy          = make_buff( this, "cryopathy", find_spell( 417492 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_chance( talents.cryopathy.ok() );
  buffs.deaths_chill       = make_buff( this, "deaths_chill", find_spell( 454371 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_chance( talents.deaths_chill.ok() );
  buffs.fingers_of_frost   = make_buff( this, "fingers_of_frost", find_spell( 44544 ) );
  buffs.freezing_rain      = make_buff( this, "freezing_rain", find_spell( 270232 ) )
                               ->set_default_value_from_effect( 2 )
                               ->set_chance( talents.freezing_rain.ok() );
  buffs.freezing_winds     = make_buff( this, "freezing_winds", find_spell( 382106 ) )
                               ->modify_duration( talents.everlasting_frost->effectN( 3 ).time_value() )
                               ->set_tick_callback( [ this ] ( buff_t*, int, timespan_t )
                                 { trigger_fof( 1.0, procs.fingers_of_frost_freezing_winds ); } )
                               ->set_partial_tick( true )
                               ->set_tick_behavior( buff_tick_behavior::REFRESH )
                               ->set_refresh_duration_callback( [ this ] ( const buff_t* b, timespan_t new_duration )
                                 {
                                   auto rem = b->tick_event ? b->tick_event->remains() : 0_ms;
                                   if ( !talents.everlasting_frost.ok() && rem < 2.0_s ) rem -= 1.0_s;
                                   return new_duration + rem;
                                 } )
                               ->set_chance( talents.freezing_winds.ok() );
  buffs.frigid_empowerment = make_buff( this, "frigid_empowerment", find_spell( 417488 ) )
                               ->set_default_value_from_effect( 1 );
  buffs.icicles            = make_buff( this, "icicles", find_spell( 205473 ) );
  buffs.icy_veins          = make_buff<buffs::icy_veins_t>( this );
  buffs.permafrost_lances  = make_buff( this, "permafrost_lances", find_spell( 455122 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_chance( talents.permafrost_lances.ok() );
  buffs.ray_of_frost       = make_buff( this, "ray_of_frost", find_spell( 208141 ) )
                               ->set_default_value_from_effect( 1 );
  buffs.slick_ice          = make_buff( this, "slick_ice", find_spell( 382148 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_chance( talents.slick_ice.ok() );


  // Frostfire
  buffs.excess_fire           = make_buff( this, "excess_fire", find_spell( 438624 ) )
                                  ->set_chance( talents.excess_fire.ok() );
  buffs.excess_frost          = make_buff( this, "excess_frost", find_spell( 438611 ) )
                                  ->set_chance( talents.excess_frost.ok() );
  buffs.fire_mastery          = make_buff( this, "fire_mastery", find_spell( 431040 ) )
                                  ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                  ->set_default_value_from_effect( 1 )
                                  ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
                                  ->set_chance( talents.frostfire_mastery.ok() );
  buffs.frost_mastery         = make_buff( this, "frost_mastery", find_spell( 431039 ) )
                                  ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
                                  ->set_default_value_from_effect( 1 )
                                  ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
                                  ->set_chance( talents.frostfire_mastery.ok() );
  buffs.frostfire_empowerment = make_buff( this, "frostfire_empowerment", find_spell( 431177 ) )
                                  ->set_trigger_spell( talents.frostfire_empowerment );
  buffs.severe_temperatures   = make_buff( this, "severe_temperatures", find_spell( 431190 ) )
                                  ->set_default_value_from_effect( 1 )
                                  ->set_trigger_spell( talents.severe_temperatures );


  // Spellslinger
  buffs.spellfrost_teachings = make_buff( this, "spellfrost_teachings", find_spell( 458411 ) )
                                 ->set_default_value_from_effect( 3 )
                                 ->set_chance( talents.spellfrost_teachings.ok() );
  buffs.unerring_proficiency = make_buff( this, "unerring_proficiency", find_spell( specialization() == MAGE_FROST ? 444976 : 444981 ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_chance( talents.unerring_proficiency.ok() );


  // Sunfury
  buffs.arcane_soul            = make_buff( this, "arcane_soul", find_spell( 451038 ) )
                                   ->set_chance( specialization() == MAGE_ARCANE && talents.memory_of_alar.ok() );
  buffs.burden_of_power        = make_buff( this, "burden_of_power", find_spell( 451049 ) )
                                   ->set_chance( talents.burden_of_power.ok() );
  buffs.glorious_incandescence = make_buff( this, "glorious_incandescence", find_spell( 451073 ) )
                                   ->set_chance( talents.glorious_incandescence.ok() );
  buffs.lingering_embers       = make_buff( this, "lingering_embers", find_spell( 461145 ) )
                                   ->set_default_value( find_spell( 448604 )->effectN( specialization() == MAGE_FIRE ? 6 : 1 ).percent() )
                                   ->set_chance( talents.codex_of_the_sunstriders.ok() );
  buffs.mana_cascade           = make_buff( this, "mana_cascade", find_spell( specialization() == MAGE_FIRE ? 449314 : 449322 ) )
                                   ->set_default_value( specialization() == MAGE_FIRE || bugs
                                     ? find_spell( 449314 )->effectN( 2 ).base_value() * 0.001
                                     : find_spell( 449322 )->effectN( 1 ).percent() )
                                   ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                   ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                     {
                                       if ( cur == 0 )
                                       {
                                         for ( auto e : mana_cascade_expiration )
                                           event_t::cancel( e );

                                         mana_cascade_expiration.clear();
                                       }
                                     } )
                                   ->set_chance( talents.mana_cascade.ok() );
  buffs.spellfire_sphere       = make_buff( this, "spellfire_sphere", find_spell( 448604 ) )
                                   ->set_default_value_from_effect( specialization() == MAGE_FIRE ? 6 : 1, 0.01 )
                                   ->modify_max_stack( as<int>( talents.rondurmancy->effectN( 1 ).base_value() ) )
                                   ->set_chance( talents.spellfire_spheres.ok() )
                                   ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.spellfire_spheres      = make_buff( this, "spellfire_spheres", find_spell( 449400 ) )
                                   ->set_chance( talents.spellfire_spheres.ok() );


  // Shared
  buffs.ice_floes          = make_buff<buffs::ice_floes_t>( this );
  buffs.incanters_flow     = make_buff<buffs::incanters_flow_t>( this );
  buffs.overflowing_energy = make_buff( this, "overflowing_energy", find_spell( 394195 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_chance( talents.overflowing_energy.ok() );
  buffs.time_warp          = make_buff( this, "time_warp", find_spell( 342242 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );


  // Set Bonuses
  buffs.intuition = make_buff( this, "intuition", find_spell( 455681 ) )
                      ->set_default_value_from_effect( 1 )
                      ->set_chance( sets->set( MAGE_ARCANE, TWW1, B4 )->effectN( 1 ).percent() );

  buffs.blessing_of_the_phoenix = make_buff( this, "blessing_of_the_phoenix", find_spell( 455134 ) )
                                    ->set_default_value_from_effect( 1 )
                                    ->set_chance( sets->has_set_bonus( MAGE_FIRE, TWW1, B4 ) );


  // Buffs that use stack_react or may_react need to be reactable regardless of what the APL does
  buffs.heating_up->reactable = true;

  // Frostfire Empowerment can be activated through Flash Freezeburn and doesn't need the previous talent
  if ( talents.flash_freezeburn.ok() )
    buffs.frostfire_empowerment->default_chance = -1.0;

  // Hyperthermia can be activated through Memory of Al'ar and doesn't need to be talented
  if ( talents.memory_of_alar.ok() )
    buffs.hyperthermia->default_chance = -1.0;
}

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.arcane_surge       = get_gain( "Arcane Surge"       );
  gains.arcane_barrage     = get_gain( "Arcane Barrage"     );
  gains.energized_familiar = get_gain( "Energized Familiar" );
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
      procs.hot_streak_spell             = get_proc( "Hot Streak spells used" );
      procs.hot_streak_spell_crit        = get_proc( "Hot Streak spell crits" );
      procs.hot_streak_spell_crit_wasted = get_proc( "Hot Streak spell crits wasted" );

      procs.ignite_applied    = get_proc( "Direct Ignite applications" );
      procs.ignite_new_spread = get_proc( "Ignites spread to new targets" );
      procs.ignite_overwrite  = get_proc( "Ignites spread to targets with existing Ignite" );
      break;
    case MAGE_FROST:
      procs.brain_freeze                    = get_proc( "Brain Freeze" );
      procs.brain_freeze_excess_fire        = get_proc( "Brain Freeze from Excess Fire" );
      procs.brain_freeze_time_anomaly       = get_proc( "Brain Freeze from Time Anomaly" );
      procs.brain_freeze_water_jet          = get_proc( "Brain Freeze from Water Jet" );
      procs.fingers_of_frost                = get_proc( "Fingers of Frost" );
      procs.fingers_of_frost_flash_freeze   = get_proc( "Fingers of Frost from Flash Freeze" );
      procs.fingers_of_frost_freezing_winds = get_proc( "Fingers of Frost from Freezing Winds" );
      procs.fingers_of_frost_wasted         = get_proc( "Fingers of Frost wasted due to Winter's Chill" );
      procs.flurry_cast                     = get_proc( "Flurry cast" );
      procs.winters_chill_applied           = get_proc( "Winter's Chill stacks applied" );
      procs.winters_chill_consumed          = get_proc( "Winter's Chill stacks consumed" );

      procs.icicles_generated  = get_proc( "Icicles generated" );
      procs.icicles_fired      = get_proc( "Icicles fired" );
      procs.icicles_overflowed = get_proc( "Icicles overflowed" );
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
      benefits.arcane_charge.arcane_blast = std::make_unique<buff_stack_benefit_t>( buffs.arcane_charge, "Arcane Blast" );
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
    case MAGE_FIRE:
      sample_data.low_mana_iteration = std::make_unique<simple_sample_data_t>();
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

  // TODO: There's no data about this in game. Keep an eye out in case Blizzard
  // changes this behind the scenes.
  shuffled_rng.time_anomaly = get_shuffled_rng( "time_anomaly", 1, 16 );
  rppm.energy_reconstitution = get_rppm( "energy_reconstitution", talents.energy_reconstitution );
  rppm.frostfire_infusion = get_rppm( "frostfire_infusion", talents.frostfire_infusion );
  // Accumulated RNG is also not present in the game data.
  accumulated_rng.pyromaniac = get_accumulated_rng( "pyromaniac", talents.pyromaniac.ok() ? 0.00605 : 0.0 );
  accumulated_rng.spellfrost_teachings = get_accumulated_rng( "spellfrost_teachings", talents.spellfrost_teachings.ok() ? 0.0004 : 0.0 );
}

void mage_t::init_finished()
{
  player_t::init_finished();

  // Sort the procs to put the proc sources next to each other.
  if ( specialization() == MAGE_FROST )
    range::sort( proc_list, [] ( proc_t* a, proc_t* b ) { return a->name_str < b->name_str; } );
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
        trigger_icicle_gain( target, action.icicle.frostbolt, 1.0, duration );
    } );

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
    reg *= 1.0 + cache.mastery() * spec.savant->effectN( 4 ).mastery_value();
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

double mage_t::stacking_movement_modifier() const
{
  double ms = player_t::stacking_movement_modifier();

  ms += buffs.frenetic_speed->check_value();

  return ms;
}

double mage_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  school_e school = s->action->get_school();

  if ( buffs.fevered_incantation->has_common_school( school ) )
    m *= 1.0 + buffs.fevered_incantation->check_stack_value();

  return m;
}

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.enlightened_damage->has_common_school( school ) )
    m *= 1.0 + buffs.enlightened_damage->check_value();
  if ( buffs.impetus->has_common_school( school ) )
    m *= 1.0 + buffs.impetus->check_value();

  return m;
}

double mage_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  m *= 1.0 + spec.arcane_mage->effectN( 3 ).percent();
  m *= 1.0 + spec.fire_mage->effectN( 3 ).percent();
  m *= 1.0 + spec.frost_mage->effectN( 3 ).percent();

  if ( !guardian )
  {
    m *= 1.0 + buffs.bone_chilling->check_stack_value();
    m *= 1.0 + buffs.incanters_flow->check_stack_value();
  }

  return m;
}

double mage_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  if ( auto td = find_target_data( target ) )
  {
    if ( !guardian )
      m *= 1.0 + td->debuffs.numbing_blast->check_value();
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
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
    case RATING_SPELL_HASTE:
      rm *= 1.0 + talents.winters_blessing->effectN( 2 ).percent();
      break;
    default:
      break;
  }

  return rm;
}

double mage_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double mul = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_INTELLECT && sim->auras.arcane_intellect->check() )
  {
    double ai_val = sim->auras.arcane_intellect->current_value;
    double ii_val = talents.inspired_intellect->effectN( 1 ).percent();
    mul /= 1.0 + ai_val;
    mul *= 1.0 + ai_val + ii_val;
  }

  return mul;
}

double mage_t::composite_melee_crit_chance() const
{
  double c = player_t::composite_melee_crit_chance();

  c += talents.tome_of_rhonin->effectN( 1 ).percent();
  c += talents.force_of_will->effectN( 1 ).percent();

  return c;
}

double mage_t::composite_spell_crit_chance() const
{
  double c = player_t::composite_spell_crit_chance();

  c += talents.tome_of_rhonin->effectN( 1 ).percent();
  c += talents.critical_mass->effectN( 1 ).percent();
  c += talents.force_of_will->effectN( 1 ).percent();

  if ( !buffs.combustion->check() && talents.fires_ire.ok() )
  {
    if ( bugs )
    {
      int rank = talents.fires_ire.rank();
      double rank_value = std::round( talents.fires_ire->effectN( 1 ).base_value() / rank );
      c += rank * rank_value * 0.01;
    }
    else
    {
      c += talents.fires_ire->effectN( 1 ).percent();
    }
  }

  return c;
}

double mage_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  h /= 1.0 + talents.tome_of_antonidas->effectN( 1 ).percent();
  h /= 1.0 + talents.winters_blessing->effectN( 1 ).percent();

  return h;
}

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  h /= 1.0 + talents.tome_of_antonidas->effectN( 1 ).percent();
  h /= 1.0 + talents.winters_blessing->effectN( 1 ).percent();

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
  buff_queue.clear();
  embedded_splinters.clear();
  mana_cascade_expiration.clear();
  events = events_t();
  ground_aoe_expiration = std::array<timespan_t, AOE_MAX>();
  state = state_t();
  expression_support = expression_support_t();
}

void mage_t::arise()
{
  player_t::arise();

  buffs.flame_accelerant->trigger();
  buffs.incanters_flow->trigger();

  if ( options.initial_spellfire_spheres > 0 )
    buffs.spellfire_sphere->trigger( options.initial_spellfire_spheres );

  if ( talents.enlightened.ok() )
  {
    update_enlightened();

    timespan_t first_tick = rng().real() * 2.0_s;
    events.enlightened = make_event<events::enlightened_event_t>( *sim, *this, first_tick );
  }

  if ( talents.flame_accelerant.ok() )
  {
    timespan_t first_tick = rng().real() * talents.flame_accelerant->effectN( 1 ).period();
    events.flame_accelerant = make_event<events::flame_accelerant_event_t>( *sim, *this, first_tick );
  }

  if ( talents.time_anomaly.ok() )
  {
    timespan_t first_tick = rng().real() * talents.time_anomaly->effectN( 1 ).period();
    events.time_anomaly = make_event<events::time_anomaly_tick_event_t>( *sim, *this, first_tick );
  }

  if ( talents.splinterstorm.ok() )
  {
    timespan_t first_tick = rng().real() * talents.splinterstorm->effectN( 2 ).period();
    events.splinterstorm = make_event<events::splinterstorm_event_t>( *sim, *this, first_tick );
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
  if ( util::str_compare_ci( name, "freezable" ) )
  {
    return make_fn_expr( name, [ &action ]
    {
      player_t* t = action.get_expression_target();
      return !t->is_boss() || t->level() < action.sim->max_player_level + 3;
    } );
  }

  auto splits = util::string_split<std::string_view>( name, "." );

  // Helper for health percentage based effects
  auto hp_pct_expr = [ & ] ( bool active, double actual_pct, bool execute )
  {
    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      if ( !active )
        return expr_t::create_constant( name, false );

      return make_fn_expr( name, [ &action, actual_pct, execute ]
      {
        double pct = action.get_expression_target()->health_percentage();
        return execute ? pct <= actual_pct : pct >= actual_pct;
      } );
    }

    if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      if ( !active )
        return expr_t::create_constant( name, execute ? std::numeric_limits<double>::max() : 0.0 );

      return make_fn_expr( name, [ &action, actual_pct ]
      { return action.get_expression_target()->time_to_percent( actual_pct ).total_seconds(); } );
    }

    throw std::invalid_argument( fmt::format( "Unknown {} operation '{}'", splits[ 0 ], splits[ 1 ] ) );
  };

  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "firestarter" ) )
    return hp_pct_expr( talents.firestarter.ok(), talents.firestarter->effectN( 1 ).base_value(), false );

  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "scorch_execute" ) )
    return hp_pct_expr( talents.scorch.ok(), talents.scorch->effectN( 2 ).base_value() + talents.sunfury_execution->effectN( 2 ).base_value(), true );

  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "improved_scorch" ) )
    return hp_pct_expr( talents.improved_scorch.ok(), talents.improved_scorch->effectN( 1 ).base_value() + talents.sunfury_execution->effectN( 2 ).base_value(), true );

  return player_t::create_action_expression( action, name );
}

std::unique_ptr<expr_t> mage_t::create_expression( std::string_view name )
{
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

  if ( util::str_compare_ci( name, "embedded_splinters" ) )
  {
    return make_fn_expr( name, [ this ]
    { return state.embedded_splinters; } );
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

  // Let action.frostbolt/fireball refer to frostfire_bolt
  if ( talents.frostfire_bolt.ok() && splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "action" ) )
  {
    // TODO: update this once blizz finalizes which spell replaces which
    if ( util::str_compare_ci( splits[ 1 ], "fireball" ) || util::str_compare_ci( splits[ 1 ], "frostbolt" ) )
    {
      if ( auto a = find_action( "frostfire_bolt" ) )
        return a->create_expression( splits[ 2 ] );
    }
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

bool mage_t::trigger_crowd_control( const action_state_t* s, spell_mechanic type, timespan_t adjust )
{
  if ( type == MECHANIC_INTERRUPT )
    return s->target->debuffs.casting->check();

  if ( action_t::result_is_hit( s->result )
    && ( !s->target->is_boss() || s->target->level() < sim->max_player_level + 3 ) )
  {
    if ( type == MECHANIC_FREEZE && options.frozen_duration + adjust > 0_ms )
      get_target_data( s->target )->debuffs.frozen->trigger( options.frozen_duration + adjust );

    return true;
  }

  return false;
}

void mage_t::trigger_time_manipulation()
{
  if ( !talents.time_manipulation.ok() )
    return;

  timespan_t t = talents.time_manipulation->effectN( 1 ).time_value();
  for ( auto cd : time_manipulation_cooldowns ) cd->adjust( t, false );
}

void mage_t::trigger_mana_cascade()
{
  if ( !talents.mana_cascade.ok() )
    return;

  int stacks = pets.arcane_phoenix && !pets.arcane_phoenix->is_sleeping() && talents.memory_of_alar.ok() ? 2 : 1;
  auto trigger_buff = [ this, s = std::min( buffs.mana_cascade->max_stack() - buffs.mana_cascade->check(), stacks ) ]
  {
    buffs.mana_cascade->trigger( s );
    mana_cascade_expiration.push_back( make_event( *sim, buffs.mana_cascade->buff_duration(), [ this, s ]
    {
      mana_cascade_expiration.erase( mana_cascade_expiration.begin() );
      buffs.mana_cascade->decrement( s );
    } ) );
  };

  if ( buffs.mana_cascade->check() < buffs.mana_cascade->max_stack() )
  {
    // If this is triggered twice within a small enough time frame,
    // erroneous expiration events can be scheduled. This currently
    // only happens with Pyromaniac.
    if ( bugs )
      make_event( *sim, trigger_buff );
    else
      trigger_buff();
  }
}

void mage_t::trigger_merged_buff( buff_t* buff, bool trigger )
{
  if ( !events.merged_buff_execute )
    events.merged_buff_execute = make_event<events::merged_buff_execute_event_t>( *sim, *this );

  auto it = range::find( buff_queue, buff, [] ( const auto& i ) { return i.buff; } );
  if ( it == buff_queue.end() )
  {
    buff_queue.push_back( { buff, !trigger, as<int>( trigger ) } );
  }
  else if ( trigger )
  {
    it->stacks++;
  }
  else
  {
    it->expire = true;
    it->stacks = 0;
  }
}

void mage_t::trigger_meteor_burn( action_t* action, player_t* target, timespan_t pulse_time, timespan_t duration )
{
  timespan_t expiration = sim->current_time() + duration;

  if ( !events.meteor_burn )
  {
    events.meteor_burn = make_event<events::meteor_burn_event_t>( *sim, *this, action, target, pulse_time, expiration );
    return;
  }

  auto e = debug_cast<events::meteor_burn_event_t*>( events.meteor_burn );
  e->action = action;
  e->target = target;
  e->pulse_time = pulse_time;
  e->expiration = expiration;
}

void mage_t::trigger_flash_freezeburn( bool ffb )
{
  if ( !talents.flash_freezeburn.ok() )
    return;

  bool schedule_event = false;
  // Attempt to apply a "banked" proc
  if ( ffb )
  {
    if ( state.trigger_flash_freezeburn )
    {
      state.trigger_flash_freezeburn = false;
      schedule_event = true;
    }
  }
  else
  {
    // If the player is currently casting a Frostfire Bolt, the proc isn't
    // applied immediately and is instead "banked," to be applied when
    // a Frostfire Bolt executes (generally the one that's currently being
    // cast, but could be a different one if the player is interrupted).
    // TODO: You can also do this with Icy Veins + FFB macro, implying there's
    // some small delay on this trigger as well.
    if ( executing && executing->id == 431044 )
      state.trigger_flash_freezeburn = true;
    else
      schedule_event = true;
  }

  if ( schedule_event )
    // The buff is applied with a small delay, which kinda defeats the
    // purpose of this whole mechanic.
    // TODO: double check this later
    make_event( *sim, 15_ms, [ this ] { buffs.frostfire_empowerment->execute(); } );
}

void mage_t::trigger_spellfire_spheres()
{
  if ( !talents.spellfire_spheres.ok() )
    return;

  int max_stacks = as<int>( talents.spellfire_spheres->effectN( specialization() == MAGE_FIRE ? 3 : 2 ).base_value() );

  buffs.spellfire_spheres->trigger();

  auto check_stacks = [ this, s = max_stacks ]
  {
    if ( buffs.spellfire_spheres->check() >= s )
    {
      if ( talents.ignite_the_future.ok() && pets.arcane_phoenix && !pets.arcane_phoenix->is_sleeping() && !buffs.spellfire_sphere->at_max_stacks() )
        debug_cast<pets::arcane_phoenix::arcane_phoenix_pet_t*>( pets.arcane_phoenix )->exceptional_spells_remaining++;
      buffs.spellfire_sphere->trigger();
      buffs.spellfire_spheres->expire();
      buffs.burden_of_power->trigger();
    }
  };

  // For Arcane, casting Arcane Blast and Arcane Barrage together results in both stacks of spellfire_spheres
  // being applied before they are consumed. This can be handled with a delay here. This does not work for Firek
  // because Pyroblast will consume the Burden of Power that was applied by the Hot Streak that it just consumed.
  if ( specialization() == MAGE_FIRE )
    check_stacks();
  else
    make_event( *sim, 15_ms, check_stacks );
}

void mage_t::consume_burden_of_power()
{
  if ( !buffs.burden_of_power->check() )
    return;

  buffs.burden_of_power->decrement();
  buffs.glorious_incandescence->trigger();
}

// If the target isn't specified, picks a random target.
void mage_t::trigger_splinter( player_t* target, int count )
{
  if ( !talents.splintering_sorcery.ok() || count == 0 )
    return;

  // Splinters don't fire if the target isn't a valid enemy
  if ( target && ( !target->is_enemy() || target->is_sleeping() ) )
    return;

  if ( !target && sim->target_non_sleeping_list.empty() )
    return;

  if ( count < 0 )
    count = specialization() == MAGE_FROST ? 1 : as<int>( talents.splintering_sorcery->effectN( 2 ).base_value() );

  double chance = talents.augury_abounds->effectN( 2 ).percent();
  for ( int i = 0; i < count; i++ )
  {
    player_t* t_ = target;
    if ( !t_ )
    {
      const auto& tl = sim->target_non_sleeping_list;
      t_ = tl[ rng().range( tl.size() ) ];
    }

    int per_conjure = ( buffs.icy_veins->check() || buffs.arcane_surge->check() ) && rng().roll( chance ) ? 2 : 1;
    for ( int j = 0; j < per_conjure; j++ )
    {
      make_event( *sim, [ this, t = t_ ] { action.splinter->execute_on_target( t ); } );
      buffs.unerring_proficiency->trigger();
    }
  }
}

bool mage_t::trigger_clearcasting( double chance, timespan_t delay )
{
  if ( specialization() != MAGE_ARCANE )
    return false;

  bool success = rng().roll( chance );
  if ( success )
  {
    if ( delay > 0_ms && buffs.clearcasting->check() )
      make_event( *sim, delay, [ this ] { buffs.clearcasting->trigger(); } );
    else
      buffs.clearcasting->trigger();

    if ( chance >= 1.0 )
      buffs.clearcasting->predict();
    buffs.big_brained->trigger();
  }

  return success;
}

bool mage_t::trigger_brain_freeze( double chance, proc_t* source, timespan_t delay )
{
  if ( specialization() != MAGE_FROST )
    return false;

  assert( source );

  bool success = rng().roll( chance );
  if ( success )
  {
    if ( delay > 0_ms && buffs.brain_freeze->check() )
    {
      make_event( *sim, delay, [ this, chance ]
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
    if ( procs.brain_freeze )
      procs.brain_freeze->occur();
  }

  return success;
}

bool mage_t::trigger_fof( double chance, proc_t* source, int stacks )
{
  if ( specialization() != MAGE_FROST )
    return false;

  assert( source );

  bool success = buffs.fingers_of_frost->trigger( stacks, buff_t::DEFAULT_VALUE(), chance );
  if ( success )
  {
    if ( chance >= 1.0 )
      buffs.fingers_of_frost->predict();

    for ( int i = 0; i < stacks; i++ )
    {
      source->occur();
      if ( procs.fingers_of_frost )
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
    procs.icicles_overflowed->occur();
    sim->print_debug( "{} icicle use on {}, total={}", name(), icicle_target->name(), icicles.size() );
  }
}

void mage_t::trigger_icicle_gain( player_t* icicle_target, action_t* icicle_action, double chance, timespan_t duration )
{
  if ( !spec.icicles->ok() || !rng().roll( chance ) )
    return;

  size_t max_icicles = as<size_t>( spec.icicles->effectN( 2 ).base_value() );
  size_t old_count = icicles.size();

  // Shoot one if capped
  if ( old_count == max_icicles )
    trigger_icicle( icicle_target );

  buffs.icicles->trigger( duration );
  icicles.push_back( { icicle_action, make_event( *sim, buffs.icicles->remains(), [ this ]
  {
    buffs.icicles->decrement();
    icicles.erase( icicles.begin() );
  } ) } );

  procs.icicles_generated->occur();

  if ( old_count == max_icicles || chance >= 1.0 )
    buffs.icicles->predict();

  if ( icicles.size() == max_icicles )
    state.gained_full_icicles = sim->current_time();

  assert( icicle_action && icicles.size() <= max_icicles );
}

void mage_t::trigger_arcane_charge( int stacks )
{
  if ( !spec.arcane_charge->ok() || stacks <= 0 )
    return;

  buffs.arcane_charge->trigger( stacks );
}

void mage_t::trigger_lit_fuse()
{
  if ( !talents.lit_fuse.ok() )
    return;

  // TODO: Verify the proc chance with every combination of the relevant talents.
  double chance = talents.lit_fuse->effectN( 4 ).percent() + talents.explosive_ingenuity->effectN( 1 ).percent();
  if ( buffs.combustion->check() )
    chance += talents.explosivo->effectN( 1 ).percent();
  if ( rng().roll( chance ) )
    buffs.lit_fuse->trigger();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class mage_report_t final : public player_report_extension_t
{
public:
  mage_report_t( mage_t& player ) :
    p( player )
  { }

  void html_customsection_icy_veins( report::sc_html_stream& os )
  {
    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Icy Veins</h3>\n"
          "<div class=\"toggle-content\">\n";

    auto& d = *p.sample_data.icy_veins_duration;
    int num_buckets = std::min( 70, static_cast<int>( 2 * ( d.max() - d.min() ) ) + 1 );
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

    switch ( p.specialization() )
    {
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

struct mage_module_t final : public module_t
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

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

enum ground_aoe_type_e
{
  AOE_BLIZZARD = 0,
  AOE_COMET_STORM,
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

enum class ao_type
{
  NORMAL,
  ORB_BARRAGE,
  ORB_MASTERY
};

enum class meteor_type
{
  NORMAL,
  ISOTHERMIC
};

enum class arcane_phoenix_rotation
{
  DEFAULT,
  ST,
  AOE
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
    buff_t* controlled_destruction;
    buff_t* freezing;
    buff_t* molten_fury;
    buff_t* touch_of_the_archmage;
    buff_t* touch_of_the_magi;
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

struct shatter_source_t : private noncopyable
{
  const std::string name_str;
  const int max_stack;
  std::vector<simple_sample_data_t> counts;
  std::vector<int> iteration_counts;

  shatter_source_t( std::string_view name, int max_stack_ ) :
    name_str( name ),
    max_stack( max_stack_ ),
    counts(),
    iteration_counts()
  {
    assert( max_stack >= 0 );
    counts.resize( max_stack + 1 );
    iteration_counts.resize( max_stack + 1 );
  }

  void occur( int stack )
  {
    assert( stack >= 0 && stack <= max_stack );
    iteration_counts[ stack ]++;
  }

  double count( int stack ) const
  {
    assert( stack >= 0 && stack <= max_stack );
    return counts[ stack ].pretty_mean();
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

  void merge( const shatter_source_t& other )
  {
    assert( max_stack == other.max_stack );
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

struct mage_t final : public player_t
{
public:
  // Buffs waiting to be triggered/expired
  std::vector<buff_adjust_info_t> buff_queue;

  // Splinters
  std::vector<dot_t*> embedded_splinters;

  // Mana Cascade expiration events
  std::vector<event_t*> mana_cascade_expiration;

  // Events
  struct events_t
  {
    event_t* icicle;
    event_t* merged_buff_execute;
    event_t* meteor_burn;
    event_t* splinterstorm;
  } events;

  // Ground AoE tracking
  std::array<timespan_t, AOE_MAX> ground_aoe_expiration;

  // Data collection
  auto_dispose<std::vector<shatter_source_t*> > shatter_source_list;

  // Cached actions
  struct actions_t
  {
    action_t* arcane_assault;
    action_t* arcane_echo;
    action_t* flash_freezeburn;
    action_t* frostfire_empowerment;
    action_t* glacial_assault;
    action_t* hand_of_frost;
    action_t* ignite;
    action_t* isothermic_comet_storm;
    action_t* isothermic_meteor;
    action_t* meteorite;
    action_t* molten_chill_ignite;
    action_t* pet_freeze;
    action_t* pet_water_jet;
    action_t* splinter;
    action_t* splinter_dot;
    action_t* splinter_recall;
    action_t* splinterstorm;
    action_t* touch_of_the_archmage;
    action_t* touch_of_the_magi_explosion;

    struct shatter_actions_t
    {
      action_t* comet_storm;
      action_t* ice_lance;
      action_t* meteor;
    } shatter;
  } action;

  // Benefits
  struct benefits_t
  {
    struct arcane_charge_benefits_t
    {
      std::unique_ptr<buff_stack_benefit_t> arcane_barrage;
      std::unique_ptr<buff_stack_benefit_t> arcane_blast;
      std::unique_ptr<buff_stack_benefit_t> arcane_pulse;
    } arcane_charge;
  } benefits;

  // Buffs
  struct buffs_t
  {
    // Arcane
    buff_t* arcane_charge;
    buff_t* arcane_familiar;
    buff_t* arcane_salvo;
    buff_t* arcane_surge;
    buff_t* clearcasting;
    buff_t* enlightened;
    buff_t* evocation;
    buff_t* overpowered_missiles;
    buff_t* presence_of_mind;


    // Fire
    buff_t* combustion;
    buff_t* feel_the_burn;
    buff_t* fevered_incantation;
    buff_t* fiery_rush;
    buff_t* frenetic_speed;
    buff_t* heating_up;
    buff_t* hot_streak;
    buff_t* hyperthermia;
    buff_t* hyperthermia_damage;
    buff_t* wildfire;


    // Frost
    buff_t* brain_freeze;
    buff_t* comet_storm;
    buff_t* fingers_of_frost;
    buff_t* freezing_rain;
    buff_t* glacial_spike;
    buff_t* hand_of_frost;
    buff_t* permafrost_lances;
    buff_t* thermal_void;


    // Frostfire
    buff_t* frostfire_empowerment;


    // Spellslinger
    buff_t* augury_abounds;


    // Sunfury
    buff_t* arcane_soul;
    buff_t* arcane_soul_damage;
    buff_t* burden_of_power;
    buff_t* glorious_incandescence;
    buff_t* lingering_embers;
    buff_t* mana_cascade;
    buff_t* spellfire_sphere;
    buff_t* spellfire_spheres;


    // Shared
    buff_t* brainstorm;
    buff_t* overflowing_energy;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* arcane_echo;
    cooldown_t* arcane_orb;
    cooldown_t* combustion;
    cooldown_t* cone_of_cold;
    cooldown_t* dragons_breath;
    cooldown_t* fire_blast;
    cooldown_t* flurry;
    cooldown_t* from_the_ashes;
    cooldown_t* frost_nova;
    cooldown_t* frozen_orb;
    cooldown_t* meteor;
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
    timespan_t arcane_missiles_chain_delay = 200_ms;
    double arcane_missiles_chain_relstddev = 0.1;
    timespan_t arcane_missiles_delay = 100_ms;
    unsigned initial_spellfire_spheres = 5;
    unsigned initial_icicles = 5;
    arcane_phoenix_rotation arcane_phoenix_rotation_override = arcane_phoenix_rotation::DEFAULT;
    double clearcasting_chance = 0.0068;
    double it_clearcasting_chance = 0.0938;
    double blast_clearcasting_chance = 0.0938;
    double blast_it_clearcasting_chance = 0.1618;
    bool fof_requires_freezing = true;
    bool il_requires_freezing = true;
    bool il_sort_by_freezing = false;
    bool randomize_si_target = false;
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
    proc_t* salvo_applied;
    proc_t* salvo_overflow;

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
    proc_t* brain_freeze_splinterstorm;
    proc_t* fingers_of_frost;
    proc_t* freezing_applied;
    proc_t* freezing_expired;
    proc_t* freezing_overflow;
  } procs;

  struct accumulated_rngs_t
  {
    accumulated_rng_t* pyromaniac;
  } accumulated_rng;

  // Sample data
  struct sample_data_t
  {
    std::unique_ptr<simple_sample_data_t> low_mana_iteration;
  } sample_data;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_charge;
    const spell_data_t* arcane_mage;
    const spell_data_t* clearcasting;
    const spell_data_t* savant;

    // Fire
    const spell_data_t* fire_mage;
    const spell_data_t* hot_streak;
    const spell_data_t* ignite;
    const spell_data_t* pyroblast_clearcasting_driver;

    // Frost
    const spell_data_t* frost_mage;
    const spell_data_t* freeze_and_shatter;
    const spell_data_t* shatter;
  } spec;

  // State
  struct state_t
  {
    bool brain_freeze_active;
    bool fingers_of_frost_active;
    bool had_low_mana;
    bool trigger_ff_empowerment;
    bool trigger_glorious_incandescence;
    bool trigger_overpowered_missiles;
    bool gained_initial_clearcasting; // Used to prevent queueing Arcane Missiles immediately after gaining the first stack Clearclasting.
    bool eureka;
    int embedded_splinters;
    int remaining_splinterstorm;
    int clearcasting_blp_count;
    int icicles;
  } state;

  // Talents
  struct talents_list_t
  {
    // Mage
    // Row 1
    player_talent_t blazing_barrier;
    player_talent_t ice_barrier;
    player_talent_t prismatic_barrier;

    // Row 2
    player_talent_t alter_time;
    player_talent_t ice_block;

    // Row 3
    player_talent_t temporal_realignment;
    player_talent_t time_walk;
    player_talent_t master_of_time;
    player_talent_t winters_protection;
    player_talent_t frost_conditioning;

    // Row 4
    player_talent_t arcane_warding;
    player_talent_t inspired_intellect;
    player_talent_t mirror_image;

    // Row 5
    player_talent_t spellsteal;
    player_talent_t quick_witted;
    player_talent_t dragons_breath;
    player_talent_t supernova;
    player_talent_t remove_curse;
    player_talent_t improved_conjuration;

    // Row 6
    player_talent_t improved_spellsteal;
    player_talent_t improved_blink;
    player_talent_t shimmer;
    player_talent_t improved_counterspell;
    player_talent_t overflowing_energy;
    player_talent_t improved_remove_curse;
    player_talent_t greater_invisibility;

    // Row 7
    player_talent_t ice_ward;
    player_talent_t improved_frost_nova;
    player_talent_t captured_thoughts;
    player_talent_t tome_of_rhonin;
    player_talent_t tome_of_antonidas;
    player_talent_t incantation_of_swiftness;
    player_talent_t master_of_escape;

    // Row 8
    player_talent_t charm_of_aegwynn;
    player_talent_t brainstorm;
    player_talent_t flow_of_time;
    player_talent_t mana_confluence;
    player_talent_t charm_of_medivh;

    // Row 9
    player_talent_t permafrost_bauble;
    player_talent_t freezing_cold;
    player_talent_t ice_nova;
    player_talent_t time_manipulation;
    player_talent_t mass_polymorph;
    player_talent_t ring_of_frost;
    player_talent_t energized_barriers;
    player_talent_t mass_invisibility;
    player_talent_t barrier_diffusion;

    // Row 10
    player_talent_t ice_cold;
    player_talent_t reflection;
    player_talent_t spatial_manipulation;
    player_talent_t improved_blazing_barrier;
    player_talent_t improved_ice_barrier;
    player_talent_t improved_prismatic_barrier;

    // Arcane
    // Row 1
    player_talent_t arcane_missiles;

    // Row 2
    player_talent_t concentrated_power;
    player_talent_t arcane_salvo;

    // Row 3
    player_talent_t amplification;
    player_talent_t improved_clearcasting;
    player_talent_t arcing_cleave;

    // Row 4
    player_talent_t arcane_pulse;
    player_talent_t arcane_surge;
    player_talent_t arcane_orb;

    // Row 5
    player_talent_t reverberate;
    player_talent_t presence_of_mind;
    player_talent_t slipstream;
    player_talent_t mana_bomb;
    player_talent_t arcane_familiar;
    player_talent_t charged_orb;

    // Row 6
    player_talent_t arcane_tempo;
    player_talent_t touch_of_the_magi;
    player_talent_t energized_familiar;
    player_talent_t expanded_mind;

    // Row 7
    player_talent_t consortiums_bauble;
    player_talent_t aether_attunement;
    player_talent_t aegwynns_technique;
    player_talent_t arcane_echo;
    player_talent_t resonance;
    player_talent_t impetus;

    // Row 8
    player_talent_t touch_of_the_archmage_1;
    player_talent_t evocation;
    player_talent_t mana_adept;
    player_talent_t enlightened;
    player_talent_t illuminated_thoughts;
    player_talent_t focusing_crystal;

    // Row 9
    player_talent_t touch_of_the_archmage_2;
    player_talent_t prodigious_savant;
    player_talent_t eureka;
    player_talent_t arcane_singularity;

    // Row 10
    player_talent_t touch_of_the_archmage_3;
    player_talent_t charged_missiles;
    player_talent_t high_voltage;
    player_talent_t overflowing_insight;
    player_talent_t overpowered_missiles;
    player_talent_t orb_mastery;
    player_talent_t orb_barrage;

    // Fire
    // Row 1
    player_talent_t pyroblast;

    // Row 2
    player_talent_t fire_blast;
    player_talent_t flamestrike_1;
    player_talent_t flamestrike_2;

    // Row 3
    player_talent_t ignition;
    player_talent_t combustion;
    player_talent_t fuel_the_fire;

    // Row 4
    player_talent_t fervent_flickering;
    player_talent_t cauterize;
    player_talent_t meteor;

    // Row 5
    player_talent_t firestarter;
    player_talent_t scorch;
    player_talent_t flame_on;
    player_talent_t kindling;
    player_talent_t critical_mass;
    player_talent_t deep_impact;

    // Row 6
    player_talent_t scald;
    player_talent_t frenetic_speed;
    player_talent_t inflame;
    player_talent_t controlled_destruction;
    player_talent_t mote_of_flame;
    player_talent_t blast_zone;

    // Row 7
    player_talent_t conflagration;
    player_talent_t intensifying_flame;
    player_talent_t spontaneous_combustion;
    player_talent_t molten_fury;
    player_talent_t mark_of_the_firelord;
    player_talent_t pyromastery;

    // Row 8
    player_talent_t fired_up_1;
    player_talent_t wildfire;
    player_talent_t fevered_incantation;
    player_talent_t fires_ire;

    // Row 9
    player_talent_t fired_up_2;
    player_talent_t master_of_flame;
    player_talent_t feel_the_burn;
    player_talent_t fiery_rush;
    player_talent_t flame_accelerant;
    player_talent_t pyromaniac;

    // Row 10
    player_talent_t fired_up_3;
    player_talent_t burnout;
    player_talent_t from_the_ashes;
    player_talent_t burn_it_all;
    player_talent_t slow_burn;
    player_talent_t pyroclasm;
    player_talent_t cinderstorm;

    // Frost
    // Row 1
    player_talent_t ice_lance;

    // Row 2
    player_talent_t blizzard_1;
    player_talent_t blizzard_2;
    player_talent_t fingers_of_frost;

    // Row 3
    player_talent_t frostbite;
    player_talent_t icicles;

    // Row 4
    player_talent_t flurry;
    player_talent_t cold_snap;
    player_talent_t glacial_bulwark;
    player_talent_t frozen_orb;

    // Row 5
    player_talent_t brain_freeze;
    player_talent_t icy_hand;
    player_talent_t ray_of_frost;
    player_talent_t wintertide;
    player_talent_t permafrost_lances;

    // Row 6
    player_talent_t frozen_touch;
    player_talent_t splitting_ice;
    player_talent_t improved_shatter;
    player_talent_t frigid_focus;
    player_talent_t splintering_ray;
    player_talent_t winters_blessing;
    player_talent_t everlasting_frost;
    player_talent_t freezing_rain;

    // Row 7
    player_talent_t fractured_frost;
    player_talent_t piercing_cold;
    player_talent_t summon_water_elemental;
    player_talent_t deep_shatter;
    player_talent_t cone_of_frost;

    // Row 8
    player_talent_t hand_of_frost_1;
    player_talent_t glacial_attunement;
    player_talent_t heart_of_ice;
    player_talent_t rimecaster;

    // Row 9
    player_talent_t hand_of_frost_2;
    player_talent_t glacial_assault;
    player_talent_t freezing_winds;
    player_talent_t improved_flurry;
    player_talent_t flash_freeze;
    player_talent_t crystalline_refraction;
    player_talent_t hailstones;

    // Row 10
    player_talent_t hand_of_frost_3;
    player_talent_t comet_storm;
    player_talent_t thermal_void;
    player_talent_t glaciate;

    // Frostfire
    // Row 1
    player_talent_t frostfire_bolt;

    // Row 2
    player_talent_t imbued_warding;
    player_talent_t meltdown;
    player_talent_t frostfire_empowerment;
    player_talent_t elemental_affinity;
    player_talent_t flame_and_frost;
    player_talent_t duality;

    // Row 3
    player_talent_t heat_sink;
    player_talent_t severe_temperatures;
    player_talent_t thermal_conditioning;
    player_talent_t dualcasting_adept;
    player_talent_t molten_chill;

    // Row 4
    player_talent_t frostfire_infusion;
    player_talent_t flash_freezeburn;
    player_talent_t blast_radius;
    player_talent_t elemental_conduit;

    // Row 5
    player_talent_t isothermic_core;

    // Spellslinger
    // Row 1
    player_talent_t splintering_sorcery;

    // Row 2
    player_talent_t augury_abounds;
    player_talent_t force_of_will;
    player_talent_t splintering_orbs;
    player_talent_t attuned_familiar;
    player_talent_t shifting_shards;

    // Row 3
    player_talent_t look_again;
    player_talent_t slippery_slinging;
    player_talent_t controlled_instincts;
    player_talent_t phantasmal_image;
    player_talent_t reactive_barrier;
    player_talent_t infused_splinters;

    // Row 4
    player_talent_t archmages_wrath;
    player_talent_t signature_spell;
    player_talent_t spellfrost_teachings;
    player_talent_t polished_focus;

    // Row 5
    player_talent_t splinterstorm;

    // Sunfury
    // Row 1
    player_talent_t spellfire_spheres;

    // Row 2
    player_talent_t mana_cascade;
    player_talent_t invocation_arcane_phoenix;
    player_talent_t burden_of_power;
    player_talent_t glorious_incandescence;

    // Row 3
    player_talent_t merely_a_setback;
    player_talent_t time_twist;
    player_talent_t codex_of_the_sunstriders;
    player_talent_t explosive_potential;
    player_talent_t lessons_in_debilitation;
    player_talent_t pyrocosm;

    // Row 4
    player_talent_t savor_the_moment;
    player_talent_t sunfury_execution;
    player_talent_t ashes_of_inspiration;
    player_talent_t rondurmancy;
    player_talent_t spellfire_salvo;

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
  void init_blizzard_action_list() override;
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
  void invalidate_cache( cache_e ) override;
  void init_resources( bool ) override;
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
  double composite_player_critical_damage_multiplier( const action_state_t*, school_e school ) const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_spell_crit_chance() const override;
  double composite_attribute_multiplier( attribute_e ) const override;
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

  shatter_source_t* get_shatter_source( std::string_view name, int max_stack )
  {
    for ( auto ss : shatter_source_list )
    {
      if ( ss->name_str == name )
        return ss;
    }

    auto ss = new shatter_source_t( name, max_stack );
    shatter_source_list.push_back( ss );
    return ss;
  }

  void trigger_arcane_charge( int stacks = 1 );
  bool trigger_brain_freeze( double chance, proc_t* source, timespan_t delay = 0.15_s );
  bool trigger_crowd_control( const action_state_t* s, spell_mechanic type );
  bool trigger_clearcasting( double chance = 1.0, timespan_t delay = 0_ms, bool never_predictable = false );
  bool trigger_fof( double chance, proc_t* source, int stacks = 1 );
  void trigger_mana_cascade();
  void trigger_merged_buff( buff_t* buff, bool trigger );
  void trigger_meteor_burn( action_t* action, player_t* target, timespan_t pulse_time, timespan_t duration );
  void trigger_spellfire_spheres();
  void consume_burden_of_power();
  void trigger_splinter( player_t* target, int count = -1 );
  void trigger_freezing( player_t* target, int stacks, proc_t* source, double chance = 1.0 );
  int  trigger_shatter( player_t* target, action_t* action, int max_consumption, shatter_source_t* source, bool fof = false );
  void trigger_icicle( int count = 1, bool grant_buff = true );
  void trigger_arcane_salvo( proc_t* source, int stacks = 1, double chance = 1.0 );
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

  void execute() override
  {
    mage_pet_spell_t::execute();

    if ( rng().roll( o()->talents.attuned_familiar->effectN( 2 ).percent() ) )
      o()->trigger_splinter( target );
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
    o()->trigger_crowd_control( s, MECHANIC_FREEZE );
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
  }

  double action_multiplier() const override
  {
    double m = mage_pet_spell_t::action_multiplier();

    if ( is_mage_spell )
    {
      if ( o()->buffs.arcane_surge->check() )
        m *= 1.0 + o()->buffs.arcane_surge->data().effectN( 1 ).percent();

      m *= 1.0 + o()->buffs.lingering_embers->check_stack_value();
      m *= 1.0 + o()->buffs.spellfire_sphere->check_stack_value();
    }

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

  double composite_crit_chance() const override
  {
    double c = mage_pet_spell_t::composite_crit_chance();

    if ( is_mage_spell )
      c += o()->buffs.combustion->check_value();

    return c;
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
    // TODO: Check what actually happens when there are no valid targets for part of the Phoenix duration.
    if ( !tl.empty() )
    {
      if ( spells_used % 2 == 1 && exceptional_spells_remaining > 0 )
      {
        action = rng().range( exceptional_actions );
        // TODO: What happens with Ignite the Future and without Codex of the Sunstriders?
        o()->buffs.spellfire_sphere->decrement();
        o()->buffs.lingering_embers->trigger();
        exceptional_spells_used++;
        exceptional_spells_remaining--;
      }
      else
      {
        if ( ( o()->options.arcane_phoenix_rotation_override == arcane_phoenix_rotation::DEFAULT && tl.size() > 1 )
          || o()->options.arcane_phoenix_rotation_override == arcane_phoenix_rotation::AOE )
        {
          action = rng().range( aoe_actions );
        }
        else
        {
          action = rng().range( st_actions );
        }
      }

      action->execute_on_target( rng().range( tl ) );
    }

    spells_used++;
    cast_event = make_event( *sim, cast_period, [ this ] { schedule_cast(); } );
  }

  void arise() override
  {
    mage_pet_t::arise();

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

    auto spec = o()->specialization();
    timespan_t buff_duration = o()->talents.memory_of_alar->effectN( spec == MAGE_FIRE ? 3 : 1 ).time_value();
    timespan_t per_spell = o()->talents.memory_of_alar->effectN( spec == MAGE_FIRE ? 4 : 2 ).time_value();
    buff_duration += exceptional_spells_used * per_spell;

    if ( spec == MAGE_FIRE )
    {
      if ( o()->buffs.hyperthermia->check() )
        // TODO: Verify that this works as expected in game.
        o()->buffs.hyperthermia->extend_duration( o(), buff_duration );
      else
        o()->buffs.hyperthermia->execute( -1, buff_t::DEFAULT_VALUE(), buff_duration );
      o()->buffs.hyperthermia->predict();
    }
    else
    {
      o()->buffs.arcane_soul->trigger( buff_duration );
    }
  };

  void create_actions() override;
};

struct arcane_barrage_t final : public arcane_phoenix_spell_t
{
  arcane_barrage_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 450499 ) )
  {
    is_mage_spell = true;
  }
};

struct pyroblast_t final : public arcane_phoenix_spell_t
{
  pyroblast_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 450461 ) )
  {
    is_mage_spell = true;
  }
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
};

struct arcane_surge_t final : public arcane_phoenix_spell_t
{
  arcane_surge_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 453326 ), true )
  {
    is_mage_spell = true;
  }
};

struct greater_pyroblast_t final : public arcane_phoenix_spell_t
{
  greater_pyroblast_t( std::string_view n, arcane_phoenix_pet_t* p ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 450421 ), true )
  {
    is_mage_spell = true;
  }
};

struct meteorite_impact_t final : public arcane_phoenix_spell_t
{
  meteorite_impact_t( std::string_view n, arcane_phoenix_pet_t* p, bool bug = false ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( bug ? 449569 : 456139 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 8; // TODO: Verify this
    is_mage_spell = true;
  }
};

struct meteorite_t final : public arcane_phoenix_spell_t
{
  action_t* damage_action = nullptr;
  action_t* damage_action_bug = nullptr;
  int exceptional_impact_counter;

  meteorite_t( std::string_view n, arcane_phoenix_pet_t* p, bool exceptional_ = false ) :
    arcane_phoenix_spell_t( n, p, p->find_spell( 449559 ), exceptional_ ), exceptional_impact_counter( 0 )
  {
    damage_action = get_action<meteorite_impact_t>( "meteorite_impact", p );
    damage_action_bug = get_action<meteorite_impact_t>( "meteorite_bug_impact", p, true );
    travel_delay = p->find_spell( exceptional_ ? 456137 : 449560 )->missile_speed();
  }

  const arcane_phoenix_pet_t* p() const
  { return static_cast<arcane_phoenix_pet_t*>( player ); }

  void execute() override
  {
    arcane_phoenix_spell_t::execute();

    if ( exceptional )
      // TODO: Test the delay more rigorously
      make_repeating_event( *sim, 75_ms, [ this, t = target ] { target = t; arcane_phoenix_spell_t::execute(); }, 3 );
  }

  void impact( action_state_t* s ) override
  {
    arcane_phoenix_spell_t::impact( s );

    // When an exceptional Meteorite proc happens, the first Meteorite will use
    // the correct spell ID and the remaining three will use the bugged spell ID.
    // TODO: Check this later
    bool bugged = exceptional && o()->bugs && ( exceptional_impact_counter % 4 != 0 );
    action_t* a = bugged ? damage_action_bug : damage_action;
    a->execute_on_target( s->target );
    if ( exceptional )
      exceptional_impact_counter++;
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
    // TODO: This *should* be affected by Touch of the Archmage and provide 25%, but
    // it currently doesn't seem to work ingame.
    double damage = current_value * p->talents.touch_of_the_magi->effectN( 1 ).percent();
    p->action.touch_of_the_magi_explosion->execute_on_target( player, damage );
    if ( p->talents.touch_of_the_archmage_3.ok() )
    {
      auto* debuff = p->get_target_data( player )->debuffs.touch_of_the_archmage;
      double ticks = std::round( debuff->buff_duration() / debuff->buff_period );
      // TODO: This seems to actually be using effect2, causing it to do 4 times as much damage
      double total = damage * p->talents.touch_of_the_archmage_3->effectN( 1 ).percent();
      debuff->trigger( -1, total / ticks );
    }
  }
};

struct combustion_t final : public buff_t
{
  double current_amount; // Amount of mastery rating granted by the buff
  double multiplier;

  combustion_t( mage_t* p ) :
    buff_t( p, "combustion", p->find_spell( 190319 ) ),
    current_amount(),
    multiplier() // TODO: slow burn
  {
    set_cooldown( 0_ms );
    set_default_value_from_effect( 1 );
    set_refresh_behavior( buff_refresh_behavior::DURATION );

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
}  // buffs


namespace actions {

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_state_t : public action_state_t
{
  // Damage multiplier that must be factored out when storing Touch of the Magi damage.
  double totm_factor;

  mage_spell_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ),
    totm_factor( 1.0 )
  { }

  void initialize() override
  {
    action_state_t::initialize();
    totm_factor = 1.0;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    totm_factor = debug_cast<const mage_spell_state_t*>( s )->totm_factor;
  }
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
  struct affected_by_t
  {
    // Temporary damage increase
    bool arcane_surge = true;
    bool freeze_and_shatter_1 = false;
    bool freeze_and_shatter_2 = false;
    bool hand_of_frost = true;
    bool lingering_embers = true;
    bool molten_fury = true;
    bool savant = false;
    bool spellfire_sphere = true;

    // Misc
    bool combustion = true;
    bool fires_ire = true;
    bool overflowing_energy = false;
    bool wildfire = true;
  } affected_by;

  struct triggers_t
  {
    bool clearcasting = false;
    bool from_the_ashes = false;
    bool frostfire_empowerment = false;
    bool ignite = false;
    bool molten_chill_ignite = false;
    bool touch_of_the_magi = true;

    target_trigger_type_e hot_streak = TT_NONE;
  } triggers;

  bool calculate_on_impact;
  unsigned impact_flags;
  double base_ignite_multiplier = 1.0;

  double freezing_chance = 1.0;
  int freezing_stacks = 0;
  int freezing_targets = -1;
  proc_t* freezing_source = nullptr;

  proc_t* salvo_source = nullptr;

public:
  mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    affected_by(),
    triggers(),
    calculate_on_impact(),
    impact_flags()
  {
    weapon_multiplier = 0.0;
    track_cd_waste = data().cooldown() > 0_ms || data().charge_cooldown() > 0_ms;
    energize_type = action_energize::NONE;
    // TODO: This could be a bit more robust; also the on-impact version
    affected_by.freeze_and_shatter_1 = data().affected_by( p->spec.freeze_and_shatter->effectN( 1 ) );
    affected_by.freeze_and_shatter_2 = data().affected_by( p->spec.freeze_and_shatter->effectN( 2 ) );

    if ( p->talents.molten_chill.ok() && school == SCHOOL_FROSTFIRE )
    {
      base_ignite_multiplier *= 1.0 + p->talents.molten_chill->effectN( 2 ).percent();
      // TODO: Looks like Flurry applies it even without Heat Sink
      triggers.molten_chill_ignite = true;
    }
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

    if ( calculate_on_impact )
      std::swap( snapshot_flags, impact_flags );

    if ( !harmful )
      target = player;
  }

  void init_finished() override
  {
    spell_t::init_finished();

    if ( p()->spec.shatter->ok() )
      freezing_source = p()->get_proc( fmt::format( "Freezing applied ({})", data().name_cstr() ) );

    if ( p()->talents.arcane_salvo.ok() )
      salvo_source = p()->get_proc( fmt::format( "Arcane Salvo applied ({})", data().name_cstr() ) );
  }

  double action_multiplier() const override
  {
    double m = spell_t::action_multiplier();

    if ( affected_by.arcane_surge && p()->buffs.arcane_surge->check() )
      m *= 1.0 + p()->buffs.arcane_surge->data().effectN( 1 ).percent();

    if ( affected_by.hand_of_frost )
      m *= 1.0 + p()->buffs.hand_of_frost->check_stack_value();

    if ( affected_by.lingering_embers )
      m *= 1.0 + p()->buffs.lingering_embers->check_stack_value();

    if ( affected_by.spellfire_sphere )
      m *= 1.0 + p()->buffs.spellfire_sphere->check_stack_value();

    if ( affected_by.freeze_and_shatter_1 )
      m *= 1.0 + p()->cache.mastery() * p()->spec.freeze_and_shatter->effectN( 1 ).mastery_value();

    if ( affected_by.freeze_and_shatter_2 )
      m *= 1.0 + p()->cache.mastery() * p()->spec.freeze_and_shatter->effectN( 2 ).mastery_value();

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
      if ( affected_by.molten_fury )
        m *= 1.0 + td->debuffs.molten_fury->check_value();
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

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    spell_t::snapshot_internal( s, flags, rt );

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

    affected_by.freeze_and_shatter_1 = spell->affected_by( p()->spec.freeze_and_shatter->effectN( 1 ) );
    affected_by.freeze_and_shatter_2 = spell->affected_by( p()->spec.freeze_and_shatter->effectN( 2 ) );

    // handle parsed base damage modifiers
    auto base_dd_mod = player->get_passive_value( *spell, "direct_damage" );
    base_dd_adder = base_dd_mod[ 1 ];
    base_dd_multiplier = base_dd_mod[ 2 ];

    auto base_td_mod = player->get_passive_value( *spell, "periodic_damage" );
    base_td_adder = base_td_mod[ 1 ];
    base_td_multiplier = base_td_mod[ 2 ];

    // handle parsed crit modifiers
    auto crit_mod = player->get_passive_value( *spell, "crit" );
    base_crit = crit_mod[ 1 ];
    crit_chance_multiplier = crit_mod[ 2 ];

    auto crit_bonus_mod = player->get_passive_value( *spell, "crit_bonus" );
    base_crit_bonus = 1.0 + crit_bonus_mod[ 1 ];
    crit_bonus_multiplier = crit_bonus_mod[ 2 ];
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

  double cost_pct_multiplier() const override
  {
    double c = spell_t::cost_pct_multiplier();

    c *= 1.0 + p()->talents.mana_confluence->effectN( 1 ).percent();

    return c;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    spell_t::available_targets( tl );

    if ( tl.size() > 2 && p()->options.randomize_si_target
      && data().affected_by( p()->talents.splitting_ice->effectN( 1 ) ) )
    {
      std::swap( tl[ 1 ], tl[ rng().range<size_t>( 1, tl.size() ) ] );
    }

    return tl.size();
  }

  void execute() override
  {
    spell_t::execute();

    bool snapshot_clearcasting = p()->buffs.clearcasting->check();
    // Make sure we remove all cost reduction buffs before we trigger new ones.
    // This will prevent for example Arcane Missiles consuming its own Clearcasting proc.
    consume_cost_reductions();

    if ( p()->spec.clearcasting->ok() && triggers.clearcasting )
    {
      // TODO: remove me
      p()->trigger_clearcasting( p()->spec.clearcasting->effectN( 2 ).percent(), 100_ms );
      return;
      // TODO: Adjust this with the new BLP data
      constexpr int cc_blp_threshold = 13;
      timespan_t delay = 100_ms;
      // The tooltip chance present on Clearcasting/Illuminated Thoughts is the total expected outcome of Clearcasting applications, not it's random proc chance.
      // Whenever combining both the proc chance and its bad luck protection, the final application rate is equal to its tooltip chance.
      double proc_chance = p()->options.clearcasting_chance; 
      if ( p()->talents.illuminated_thoughts.ok() )
        proc_chance = p()->options.it_clearcasting_chance;
      // Arcane Blast has an unmentioned 5% increase in total expected Clearcasting applications -- same BLP threshold, but higher proc chance.
      if ( id == 30451 )
      {
        proc_chance = p()->options.blast_clearcasting_chance;
        if ( p()->talents.illuminated_thoughts.ok() )
          proc_chance = p()->options.blast_it_clearcasting_chance;
      }

      p()->state.clearcasting_blp_count += 1;
      if ( p()->state.clearcasting_blp_count >= cc_blp_threshold )
        proc_chance = 1.0;
      // Arcane Explosion, if consuming Clearcasting, has the random proc chance occur precisely whenever the Echo is executed.
      if ( proc_chance != 1.0 && id == 1449 && snapshot_clearcasting )
        delay = 500_ms;

      if ( proc_chance == 1.0 || !background )
      {
        if ( p()->trigger_clearcasting( proc_chance, delay, background ) )
          p()->state.clearcasting_blp_count = 0;
      }
    }

    if ( triggers.frostfire_empowerment && rng().roll( p()->talents.frostfire_empowerment->effectN( 3 ).percent() ) )
      make_event( *sim, [ this ] { p()->buffs.frostfire_empowerment->trigger(); } );
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

    if ( freezing_targets == -1 || s->chain_target < freezing_targets )
      p()->trigger_freezing( s->target, freezing_stacks, freezing_source, freezing_chance );

    if ( triggers.ignite )
      trigger_ignite( s );

    if ( triggers.molten_chill_ignite )
      trigger_molten_chill_ignite( s );

    if ( affected_by.overflowing_energy )
    {
      // TODO: For Frost, this is currently triggered by Ice Lance but consumed by FB/FFB/GS
      p()->trigger_merged_buff( p()->buffs.overflowing_energy, s->result != RESULT_CRIT );
    }

    if ( p()->talents.fevered_incantation.ok() && s->result_type == result_amount_type::DMG_DIRECT )
      p()->trigger_merged_buff( p()->buffs.fevered_incantation, s->result == RESULT_CRIT );

    // TODO: Test the exact behavior of the hidden Molten Fury debuff.
    if ( p()->talents.molten_fury.ok() )
    {
      if ( target->health_percentage() <= p()->talents.molten_fury->effectN( 1 ).base_value() )
        get_td( s->target )->debuffs.molten_fury->trigger();
      else
        get_td( s->target )->debuffs.molten_fury->expire();
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

  virtual double composite_ignite_multiplier( const action_state_t* s ) const
  {
    double m = base_ignite_multiplier;

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

  // Simplified version of trigger_ignite for the Frostfire Frost version.
  void trigger_molten_chill_ignite( action_state_t* s )
  {
    if ( !p()->action.molten_chill_ignite )
      return;

    double m = s->target_da_multiplier;
    if ( m <= 0.0 )
      return;

    // TODO: Description says 15% (effect 1), but it seems to be doing 10% in game
    double amount = s->result_total / m * p()->talents.molten_chill->effectN( 1 ).percent();
    if ( amount <= 0.0 )
      return;

    // Note that composite_ignite_multiplier could be different from 1.0. Don't apply it here.
    residual_action::trigger( p()->action.molten_chill_ignite, s->target, amount );
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

template <typename Data>
struct mss_with_data_t final : public mage_spell_state_t
{
  Data data;

  mss_with_data_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    data()
  { }

  void initialize() override
  {
    mage_spell_state_t::initialize();
    data = Data();
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s );
    data.debug( s );
    return s;
  }

  void copy_state( const action_state_t* s ) override
  {
    mage_spell_state_t::copy_state( s );
    data = debug_cast<const mss_with_data_t*>( s )->data;
  }
};

template <typename Base, typename Data>
struct custom_state_spell_t : public Base
{
  using custom_state_t = mss_with_data_t<Data>;

  template <typename... Args>
  custom_state_spell_t( Args&&... args )
    : Base( std::forward<Args>( args )... )
  { }

  custom_state_t* cast_state( action_state_t* s )
  { return debug_cast<custom_state_t*>( s ); }

  const custom_state_t* cast_state( const action_state_t* s ) const
  { return debug_cast<const custom_state_t*>( s ); }

  action_state_t* new_state() override
  { return new custom_state_t( this, this->target ); }
};

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
    double per_charge = p()->buffs.arcane_charge->data().effectN( arcane_barrage ? 2 : 1 ).percent();
    per_charge += p()->cache.mastery() * p()->spec.savant->effectN( arcane_barrage ? 3 : 2 ).mastery_value() *
                  ( 1.0 + p()->talents.prodigious_savant->effectN( arcane_barrage ? 2 : 1 ).percent() );

    return 1.0 + p()->buffs.arcane_charge->check() * per_charge;
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
      if ( tt_applicable( s, triggers.hot_streak ) )
        handle_hot_streak( s->composite_crit_chance(), s->result == RESULT_CRIT ? HS_CRIT : HS_HIT );

      if ( triggers.from_the_ashes && p()->talents.from_the_ashes.ok() && p()->cooldowns.from_the_ashes->up() )
      {
        p()->cooldowns.from_the_ashes->start( p()->talents.from_the_ashes->internal_cooldown() );
        // TODO: Fire Blast cdr
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
          p->buffs.brainstorm->trigger();
          // If the player knew about Heating Up and converted to Hot Streak
          // with a guaranteed crit, let them react to the Hot Streak instantly.
          if ( guaranteed && hu_react )
            p->buffs.hot_streak->predict();
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

    return target->health_percentage() <= p()->talents.scorch->effectN( 2 ).base_value() + p()->talents.sunfury_execution->effectN( 2 ).base_value();
  }
};

struct hot_streak_data_t
{
  bool hot_streak = false;
  void debug( std::ostringstream& s ) const { s << " hot_streak=" << hot_streak; }
};

struct hot_streak_spell_t : public custom_state_spell_t<fire_mage_spell_t, hot_streak_data_t>
{
  action_t* pyromaniac_action;
  // Last available Hot Streak state.
  bool last_hot_streak;

  hot_streak_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    custom_state_spell_t( n, p, s ),
    pyromaniac_action(),
    last_hot_streak()
  { }

  void reset() override
  {
    custom_state_spell_t::reset();
    last_hot_streak = false;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.hot_streak->check() || p()->buffs.hyperthermia->check() )
      return 0_ms;

    return custom_state_spell_t::execute_time();
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    cast_state( s )->data.hot_streak = last_hot_streak;
    custom_state_spell_t::snapshot_state( s, rt );
  }

  double composite_crit_chance() const override
  {
    double c = custom_state_spell_t::composite_crit_chance();

    c += p()->buffs.hyperthermia->check_value();

    return c;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = custom_state_spell_t::composite_da_multiplier( s );

    m *= 1.0 + p()->buffs.hyperthermia_damage->check_stack_value();

    return m;
  }

  double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    double m = custom_state_spell_t::composite_ignite_multiplier( s );

    if ( cast_state( s )->data.hot_streak )
    {
      m *= 1.0 + p()->spec.hot_streak->effectN( 1 ).percent();
      m *= 1.0 + p()->talents.inflame->effectN( 1 ).percent();
    }

    return m;
  }

  void schedule_execute( action_state_t* s ) override
  {
    custom_state_spell_t::schedule_execute( s );
    last_hot_streak = p()->buffs.hot_streak->up();
  }

  void execute() override
  {
    custom_state_spell_t::execute();

    p()->consume_burden_of_power();

    if ( last_hot_streak )
    {
      p()->buffs.hot_streak->decrement();

      if ( !p()->buffs.combustion->check() && !p()->buffs.hyperthermia->check() )
        p()->buffs.hyperthermia->trigger();

      p()->trigger_spellfire_spheres();
      p()->trigger_mana_cascade();
    }

    // TODO: Test the proc chance and whether this works with Hyperthermia and Lit Fuse.
    // TODO: Pyromaniac seems to proc regardless of Hot Streak state
    if ( ( last_hot_streak || p()->bugs ) && p()->cooldowns.pyromaniac->up() && p()->accumulated_rng.pyromaniac->trigger() )
    {
      p()->cooldowns.pyromaniac->start( p()->talents.pyromaniac->internal_cooldown() );

      p()->trigger_spellfire_spheres();
      p()->trigger_mana_cascade();

      assert( pyromaniac_action );
      // Pyromaniac Pyroblast actually casts on the Mage's target, but that is probably a bug.
      make_event( *sim, 500_ms, [ this, t = target ]
      { pyromaniac_action->execute_on_target( t ); } );
    }

    if ( p()->buffs.hyperthermia->check() )
      p()->buffs.hyperthermia_damage->trigger();
  }
};


// ==========================================================================
// Frost Mage Spell
// ==========================================================================

struct frost_mage_spell_t : public mage_spell_t
{
  proc_t* proc_brain_freeze;
  proc_t* proc_fof;

  frost_mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    proc_brain_freeze(),
    proc_fof()
  { }
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
    background = proc = true;
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
    proc = true;
    if ( p->talents.intensifying_flame.ok() )
      intensifying_flame = get_action<intensifying_flame_t>( "intensifying_flame", p );
  }

  void tick( dot_t* d ) override
  {
    residual_action_t::tick( d );

    auto p = debug_cast<mage_t*>( player );
    if ( p->get_active_dots( d ) <= p->talents.intensifying_flame->effectN( 1 ).base_value() )
    {
      // 2024-05-19: Intensifying Flames deals a percentage of Ignite's base tick damage and not the damage it actually ticked for.
      double tick_amount = p->bugs ? base_ta( d->state ) : d->state->result_total;
      intensifying_flame->execute_on_target( d->target, p->talents.intensifying_flame->effectN( 2 ).percent() * tick_amount );
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = residual_action_t::composite_target_multiplier( target );

    if ( auto td = debug_cast<mage_t*>( player )->find_target_data( target ) )
      m *= 1.0 + td->debuffs.molten_fury->check_value();

    return m;
  }
};

struct molten_chill_ignite_t final : public residual_action::residual_periodic_action_t<spell_t>
{
  molten_chill_ignite_t( std::string_view n, mage_t* p ) :
    residual_action_t( n, p, p->find_spell( 1262887 ) )
  {
    proc = true;
  }
};

struct arcane_orb_bolt_t final : public arcane_mage_spell_t
{
  arcane_orb_bolt_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 153640 ) )
  {
    background = proc = true;
    affected_by.savant = true;
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    // AC is triggered even if the spell misses.
    p()->trigger_arcane_charge();
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    if ( p()->state.eureka )
      am *= 1.0 + p()->talents.eureka->effectN( 1 ).percent();

    return am;
  }
};

struct arcane_orb_data_t
{
  bool eureka = false;
  void debug( std::ostringstream& s ) const { s << " eureka=" << eureka; }
};

struct arcane_orb_t final : public custom_state_spell_t<arcane_mage_spell_t, arcane_orb_data_t>
{
  const ao_type type;
  bool clearcasting_snapshot = false;
  action_t* orb_mastery = nullptr;

  arcane_orb_t( std::string_view n, mage_t* p, std::string_view options_str, ao_type type_ = ao_type::NORMAL ) :
    custom_state_spell_t( n, p, type_ == ao_type::NORMAL ? p->talents.arcane_orb : p->find_spell( 153626 ) ),
    type( type_ )
  {
    parse_options( options_str );
    may_miss = false;
    aoe = -1;
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
      case ao_type::ORB_MASTERY:
        bolt_name = "orb_mastery_arcane_orb_bolt";
        break;
      default:
        assert( false );
        break;
    }

    impact_action = get_action<arcane_orb_bolt_t>( bolt_name, p );
    add_child( impact_action );

    if ( type != ao_type::NORMAL )
    {
      background = proc = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
      return;
    }

    if ( p->talents.orb_mastery.ok() )
    {
      cost_reductions = { p->buffs.clearcasting };
      orb_mastery = get_action<arcane_orb_t>( "orb_mastery_arcane_orb", p, "", ao_type::ORB_MASTERY );
      add_child( orb_mastery );
    }
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    cast_state( s )->data.eureka = p()->talents.orb_mastery.ok() && p()->talents.eureka.ok() && clearcasting_snapshot;
    custom_state_spell_t::snapshot_state( s, rt );
  }

  void execute() override
  {
    if ( orb_mastery && p()->buffs.clearcasting->check() )
    {
      int count = as<int>( p()->talents.orb_mastery->effectN( 1 ).base_value() );
      make_repeating_event( *sim, 150_ms, [ this, t = target ] { orb_mastery->execute_on_target( t ); }, count );
      clearcasting_snapshot = true;
    }

    custom_state_spell_t::execute();

    p()->trigger_arcane_charge();
    clearcasting_snapshot = false;
  }

  void impact( action_state_t* s ) override
  {
    // TODO: There's probably a nicer way to do this without having to give up on impact_spell
    p()->state.eureka = cast_state( s )->data.eureka;
    custom_state_spell_t::impact( s );
    p()->state.eureka = false;

    if ( p()->talents.splintering_orbs.ok() )
    {
      int count = as<int>( p()->talents.splintering_orbs->effectN( 4 ).base_value() );
      int max_count = as<int>( p()->talents.splintering_orbs->effectN( 1 ).base_value() );
      assert( count > 0 );
      if ( s->chain_target < max_count / count )
        p()->trigger_splinter( s->target, count );
    }
  }

  double cost_pct_multiplier() const override
  {
    // TODO: Clearcasting is the only cost reduction now and it applies
    // to a single spell. Perhaps we can remove the cost_reduction machinery
    // and avoid this silly hack.
    return mage_spell_t::cost_pct_multiplier();
  }
};

struct arcane_barrage_t final : public arcane_mage_spell_t
{
  action_t* orb_barrage = nullptr;
  int snapshot_charges = -1;
  int glorious_incandescence_charges = 0;
  int arcane_soul_charges = 0;

  arcane_barrage_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Barrage" ) )
  {
    parse_options( options_str );
    base_aoe_multiplier *= p->talents.arcing_cleave->effectN( 2 ).percent();
    affected_by.overflowing_energy = true;
    triggers.clearcasting = true;
    glorious_incandescence_charges = as<int>( p->find_spell( 451223 )->effectN( 1 ).base_value() );
    arcane_soul_charges = as<int>( p->find_spell( 453413 )->effectN( 1 ).base_value() );

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
    if ( p()->talents.orb_barrage->ok() )
    {
      triggers.clearcasting = true;
      if ( rng().roll( p()->buffs.arcane_salvo->check() * p()->talents.orb_barrage->effectN( 1 ).percent() ) )
      {
        orb_barrage->execute_on_target( target );
        // Likely a bug: Arcane Orb procs from Orb Barrage uniquely prevent Barrage from rolling Clearcasting's proc chance, and incrementing its BLP.
        triggers.clearcasting = false;
      }
    }

    p()->benefits.arcane_charge.arcane_barrage->update();

    arcane_mage_spell_t::execute();

    double mana_pct = p()->buffs.arcane_charge->check() * 0.01 * p()->talents.mana_adept->effectN( 1 ).percent();
    p()->resource_gain( RESOURCE_MANA, p()->resources.max[ RESOURCE_MANA ] * mana_pct, p()->gains.arcane_barrage, this );

    p()->buffs.arcane_charge->expire();
    int salvo = p()->buffs.arcane_salvo->check();
    if ( salvo && p()->talents.force_of_will.ok() )
    {
      // TODO: Force of Will seems to conjure one additional splinter if at
      // least one stack of Arcane Salvo is present
      p()->trigger_splinter( target, 1 + salvo / as<int>( p()->talents.force_of_will->effectN( 1 ).base_value() ) );
    }
    p()->buffs.arcane_salvo->expire();
    if ( salvo >= as<int>( p()->talents.polished_focus->effectN( 1 ).base_value() ) )
      p()->trigger_arcane_salvo( salvo_source, as<int>( p()->talents.polished_focus->effectN( 2 ).base_value() ) );

    if ( p()->buffs.arcane_soul->check() )
    {
      p()->trigger_clearcasting();
      p()->trigger_arcane_charge( arcane_soul_charges );
      p()->buffs.arcane_soul_damage->trigger();
    }

    p()->trigger_spellfire_spheres();
    p()->trigger_mana_cascade();

    if ( p()->buffs.glorious_incandescence->check() )
    {
      p()->buffs.glorious_incandescence->decrement();
      p()->trigger_arcane_charge( glorious_incandescence_charges );
      p()->state.trigger_glorious_incandescence = true;
    }
    p()->consume_burden_of_power();

    snapshot_charges = -1;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = arcane_mage_spell_t::composite_da_multiplier( s );

    if ( s->n_targets > 1 )
      m *= 1.0 + ( s->n_targets - 1 ) * p()->talents.resonance->effectN( 1 ).percent();

    if ( p()->buffs.burden_of_power->check() )
      m *= 1.0 + p()->buffs.burden_of_power->data().effectN( 4 ).percent();

    if ( p()->buffs.glorious_incandescence->check() )
      m *= 1.0 + p()->buffs.glorious_incandescence->data().effectN( 2 ).percent();

    return m;
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_multiplier( true );
    am *= 1.0 + p()->buffs.arcane_salvo->check_stack_value();
    am *= 1.0 + p()->buffs.arcane_soul_damage->check_stack_value();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );
    if ( result_is_hit( s->result ) )
      trigger_glorious_incandescence( s->target );
  }
};

struct arcane_blast_t final : public arcane_mage_spell_t
{
  arcane_blast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_specialization_spell( "Arcane Blast" ) )
  {
    parse_options( options_str );
    triggers.clearcasting = true;
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
    p()->trigger_arcane_charge( as<int>( data().effectN( 2 ).base_value() ) );
    p()->trigger_arcane_salvo( salvo_source, as<int>( p()->talents.expanded_mind->effectN( 1 ).base_value() ) );
    p()->trigger_splinter( p()->target );
    p()->trigger_spellfire_spheres();
    p()->trigger_mana_cascade();

    if ( p()->buffs.presence_of_mind->up() )
      p()->buffs.presence_of_mind->decrement();
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_multiplier();

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
};

struct arcane_explosion_t final : public arcane_mage_spell_t
{
  arcane_explosion_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    arcane_mage_spell_t( n, p, p->find_class_spell( "Arcane Explosion" ) )
  {
    parse_options( options_str );
    aoe = -1;
    affected_by.savant = true;
    triggers.clearcasting = true;
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( !target_list().empty() )
      p()->trigger_arcane_charge( as<int>( data().effectN( 1 ).base_value() ) );
  }
};

struct arcane_pulse_t final : public arcane_mage_spell_t
{
  action_t* arcane_pulse_echo = nullptr;

  arcane_pulse_t( std::string_view n, mage_t* p, std::string_view options_str, bool echo = false ) :
    arcane_mage_spell_t( n, p, echo ? p->find_spell( 1243460 ) : p->talents.arcane_pulse )
  {
    parse_options( options_str );
    aoe = -1;
    // TODO: Can the echo also trigger CC?
    triggers.clearcasting = true;
    reduced_aoe_targets = data().effectN( 3 ).base_value();

    if ( echo )
    {
      background = proc = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
      // TODO: This is probably a bug
      affected_by.savant = true;
      return;
    }

    if ( p->talents.reverberate.ok() )
    {
      arcane_pulse_echo = get_action<arcane_pulse_t>( "arcane_pulse_echo", p, "", true );
      add_child( arcane_pulse_echo );
    }
  }

  double cost_pct_multiplier() const override
  {
    double c = arcane_mage_spell_t::cost_pct_multiplier();

    c *= 1.0 + p()->buffs.arcane_charge->check() * p()->buffs.arcane_charge->data().effectN( 5 ).percent();

    return c;
  }

  void execute() override
  {
    p()->benefits.arcane_charge.arcane_pulse->update();

    // TODO: radius increase?
    arcane_mage_spell_t::execute();

    p()->trigger_arcane_charge( as<int>( data().effectN( 2 ).base_value() ) );
    p()->trigger_splinter( p()->target ); // Also triggers from echo
    if ( !background )
      p()->trigger_arcane_salvo( salvo_source, as<int>( p()->talents.expanded_mind->effectN( 1 ).base_value() ) );

    if ( arcane_pulse_echo && rng().roll( p()->talents.reverberate->effectN( 1 ).percent() ) )
      make_event( *sim, 500_ms, [ this, t = target ] { arcane_pulse_echo->execute_on_target( t ); } );
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_multiplier();

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
    background = proc = true;
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( rng().roll( p()->talents.energized_familiar->effectN( 2 ).percent() ) )
      p()->resource_gain( RESOURCE_MANA, p()->resources.max[ RESOURCE_MANA ] * energize_pct, p()->gains.energized_familiar, this );

    if ( rng().roll( p()->talents.attuned_familiar->effectN( 1 ).percent() ) )
      p()->trigger_splinter( target );
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

struct am_data_t
{
  int targets = 0;
  void debug( std::ostringstream& s ) const { s << " targets=" << targets; }
};

struct arcane_missiles_tick_t final : public custom_state_spell_t<arcane_mage_spell_t, am_data_t>
{
  int high_voltage_energize;

  arcane_missiles_tick_t( std::string_view n, mage_t* p ) :
    custom_state_spell_t( n, p, p->find_spell( 7268 ) ),
    high_voltage_energize( as<int>( p->find_spell( 461524 )->effectN( 1 ).base_value() ) )
  {
    background = proc = true;
    affected_by.savant = true;

    // The mage could have Overpowered Missiles without Aether Attunement,
    // so we can't use talents.aether_attunement here.
    base_aoe_multiplier *= p->find_spell( 1243307 )->effectN( 1 ).percent();
  }

  int n_targets() const override
  {
    // If pre_execute_state exists, we need to respect the n_targets amount
    // as it existed when the state was updated.
    if ( pre_execute_state )
      return cast_state( pre_execute_state )->data.targets;

    assert( custom_state_spell_t::n_targets() == 0 );
    int targets = 1;
    targets += as<int>( p()->talents.aether_attunement->effectN( 2 ).base_value() );
    if ( p()->buffs.overpowered_missiles->check() )
      targets += as<int>( p()->buffs.overpowered_missiles->data().effectN( 2 ).base_value() );
    return targets == 1 ? 0 : targets;
  }

  void update_state( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    custom_state_spell_t::update_state( s, flags, rt );
    cast_state( s )->data.targets = n_targets();
  }

  void execute() override
  {
    custom_state_spell_t::execute();

    p()->trigger_arcane_salvo( salvo_source );
    p()->trigger_arcane_salvo( salvo_source, as<int>( p()->talents.focusing_crystal->effectN( 2 ).base_value() ),
                               p()->talents.focusing_crystal->effectN( 1 ).percent() );

    if ( rng().roll( p()->talents.high_voltage->effectN( 1 ).percent() ) )
      p()->trigger_arcane_charge( high_voltage_energize );

    if ( p()->talents.charged_missiles.ok() )
      p()->buffs.arcane_charge->decrement();
  }

  double action_multiplier() const override
  {
    double am = custom_state_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.overpowered_missiles->check_value();
    if ( p()->buffs.arcane_charge->check() )
      am *= 1.0 + p()->talents.charged_missiles->effectN( 1 ).percent();

    return am;
  }
};

struct arcane_missiles_t final : public custom_state_spell_t<arcane_mage_spell_t, am_data_t>
{
  bool allow_arcane_missiles_delay = false;

  arcane_missiles_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    custom_state_spell_t( n, p, p->talents.arcane_missiles )
  {
    add_option( opt_bool( "allow_arcane_missiles_delay", allow_arcane_missiles_delay ) );
    parse_options( options_str );
    may_miss = false;
    tick_zero = channeled = true;
    triggers.clearcasting = true;
    tick_action = get_action<arcane_missiles_tick_t>( "arcane_missiles_tick", p );
    cost_reductions = { p->buffs.clearcasting };
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }

  void channel_finish()
  {
    p()->buffs.overpowered_missiles->expire();

    // Apply a banked proc
    if ( p()->state.trigger_overpowered_missiles )
    {
      p()->state.trigger_overpowered_missiles = false;
      p()->buffs.overpowered_missiles->trigger();
    }
  }

  bool ready() override
  {
    if ( !p()->buffs.clearcasting->check() )
      return false;

    // Arcane Missiles cannot be queued immediately after gaining the first stack of Clearcasting.
    if ( p()->state.gained_initial_clearcasting && !allow_arcane_missiles_delay )
      return false;

    return custom_state_spell_t::ready();
  }

  timespan_t execute_time() const override
  {
    timespan_t t = custom_state_spell_t::execute_time();

    // Arcane Missiles cannot be queued immediately after gaining the first stack of Clearcasting.
    // If used in this situation, add a small extra delay to account for this lack of queueing.
    if ( p()->state.gained_initial_clearcasting && allow_arcane_missiles_delay )
      t += p()->options.arcane_missiles_delay;

    return t;
  }

  void execute() override
  {
    if ( get_dot( target )->is_ticking() )
      channel_finish();

    custom_state_spell_t::execute();
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

    custom_state_spell_t::trigger_dot( s );

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

    return custom_state_spell_t::usable_moving();
  }

  void last_tick( dot_t* d ) override
  {
    custom_state_spell_t::last_tick( d );
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

    double max_mana_mult = data().effectN( 2 ).base_value();
    // TODO: Doesn't seem to work ingame, best guess at how it should work
    max_mana_mult *= 1.0 + p()->talents.mana_bomb->effectN( 1 ).percent();
    am *= 1.0 + p()->resources.pct( RESOURCE_MANA ) * ( max_mana_mult - 1.0 );

    return am;
  }

  void execute() override
  {
    p()->trigger_splinter( target, as<int>( p()->talents.augury_abounds->effectN( 1 ).base_value() ) );

    // Clear any existing surge buffs to trigger the DF2 4pc buff.
    p()->buffs.arcane_surge->expire();
    // TODO: fixme
    timespan_t bonus_duration = 0_ms; // p()->buffs.spellfire_sphere->check() * p()->buffs.spellfire_sphere->data().effectN( 5 ).time_value();
    timespan_t arcane_surge_duration = p()->buffs.arcane_surge->buff_duration() + bonus_duration;
    p()->buffs.arcane_surge->trigger( arcane_surge_duration );

    p()->trigger_clearcasting();

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
  }
};

struct blizzard_shard_t final : public frost_mage_spell_t
{
  blizzard_shard_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 190357 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 8; // TODO: check if this is still the case
    background = proc = ground_aoe = true;

    if ( p->talents.frostbite.ok() )
    {
      freezing_chance = p->talents.frostbite->proc_chance();
      freezing_stacks = as<int>( p->talents.frostbite->effectN( 2 ).base_value() );
    }
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }
};

struct blizzard_t final : public frost_mage_spell_t
{
  action_t* blizzard_shard;

  blizzard_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.blizzard_1.ok() ? p->talents.blizzard_1 : p->talents.blizzard_2 ),
    blizzard_shard( get_action<blizzard_shard_t>( "blizzard_shard", p ) )
  {
    parse_options( options_str );
    add_child( blizzard_shard );
    may_miss = false;
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
    frost_mage_spell_t( n, p, p->talents.cold_snap )
  {
    parse_options( options_str );
    harmful = false;
  };

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->cooldowns.frost_nova->reset( false );
    p()->cooldowns.cone_of_cold->reset( false );
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

    timespan_t bonus_duration =
      p()->buffs.spellfire_sphere->check() * p()->buffs.spellfire_sphere->data().effectN( 5 ).time_value();
    timespan_t combustion_duration = p()->buffs.combustion->buff_duration() + bonus_duration;
    p()->buffs.combustion->trigger( combustion_duration );
    p()->buffs.wildfire->trigger();
    p()->cooldowns.fire_blast->reset( false, as<int>( p()->talents.spontaneous_combustion->effectN( 1 ).base_value() ) );
    if ( p()->pets.arcane_phoenix )
      p()->pets.arcane_phoenix->summon( combustion_duration ); // TODO: The extra random pet duration can sometimes result in an extra cast.
  }
};

struct comet_storm_projectile_t final : public frost_mage_spell_t
{
  int freezing_consume;
  shatter_source_t* shatter_source;

  comet_storm_projectile_t( std::string_view n, mage_t* p, bool isothermic_ = false ) :
    frost_mage_spell_t( n, p, p->find_spell( isothermic_ ? 438609 : 153596 ) ),
    freezing_consume( as<int>( p->spec.shatter->effectN( 5 ).base_value() ) ),
    shatter_source( p->get_shatter_source( name_str, freezing_consume ) )
  {
    aoe = -1;
    background = proc = true;
    if ( p->talents.elemental_conduit.ok() )
      triggers.ignite = true;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) && p()->action.shatter.comet_storm )
      p()->trigger_shatter( s->target, p()->action.shatter.comet_storm, freezing_consume, shatter_source );
  }
};

struct comet_storm_t final : public frost_mage_spell_t
{
  static constexpr int pulse_count = 7;
  static constexpr timespan_t pulse_time = 0.2_s;

  action_t* projectile;
  const bool isothermic;

  comet_storm_t( std::string_view n, mage_t* p, std::string_view options_str, bool isothermic_ = false ) :
    frost_mage_spell_t( n, p, p->find_spell( 153595 ) ),
    projectile( get_action<comet_storm_projectile_t>( isothermic_ ? "isothermic_comet_storm_projectile" : "comet_storm_projectile", p, isothermic_ ) ),
    isothermic( isothermic_ )
  {
    parse_options( options_str );
    may_miss = false;
    add_child( projectile );
    travel_delay = p->find_spell( 228601 )->missile_speed();

    if ( isothermic )
    {
      background = proc = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
      return;
    }

    if ( p->spec.shatter->ok() )
      add_child( p->action.shatter.comet_storm );
    if ( p->talents.isothermic_core.ok() )
      add_child( p->action.isothermic_meteor );
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

  void execute() override
  {
    frost_mage_spell_t::execute();
    p()->buffs.comet_storm->decrement();

    if ( p()->action.isothermic_meteor )
      p()->action.isothermic_meteor->execute_on_target( target );
  }

  bool ready() override
  {
    if ( !p()->buffs.comet_storm->check() )
      return false;

    return frost_mage_spell_t::ready();
  }
};

struct cone_of_cold_t final : public frost_mage_spell_t
{
  cone_of_cold_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_class_spell( "Cone of Cold" ) )
  {
    parse_options( options_str );
    aoe = -1;
    if ( p->talents.cone_of_frost.ok() )
    {
      freezing_stacks = as<int>( p->talents.cone_of_frost->effectN( 1 ).base_value() );
      freezing_targets = as<int>( p->talents.cone_of_frost->effectN( 2 ).base_value() );
    }
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( p()->talents.freezing_cold.ok() )
      p()->trigger_crowd_control( s, MECHANIC_FREEZE );
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
    p()->trigger_crowd_control( s, MECHANIC_INTERRUPT );
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
    triggers.from_the_ashes = true;
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
    arcane_mage_spell_t::trigger_dot( s );

    double mana_regen_multiplier = 1.0 + p()->buffs.evocation->default_value;
    mana_regen_multiplier /= p()->cache.spell_cast_speed();

    p()->buffs.evocation->trigger( 1, mana_regen_multiplier - 1.0, -1.0, composite_dot_duration( s ) );
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
  double master_of_flame_mult;

  fireball_t( std::string_view n, mage_t* p, std::string_view options_str, bool frostfire_ = false ) :
    fire_mage_spell_t( n, p, frostfire_ ? p->talents.frostfire_bolt : p->find_specialization_spell( "Fireball" ) ),
    frostfire( frostfire_ ),
    master_of_flame_mult( 1.0 )
  {
    parse_options( options_str );
    if ( frostfire )
      enable_calculate_on_impact( 468655 );
    affected_by.overflowing_energy = true;
    triggers.hot_streak = TT_ALL_TARGETS;
    triggers.ignite = triggers.from_the_ashes = triggers.frostfire_empowerment = true;

    if ( p->talents.master_of_flame.ok() )
      master_of_flame_mult *= 1.0 + p->find_spell( 1217750 )->effectN( 1 ).percent();

    if ( data().ok() && p->talents.frostfire_empowerment.ok() )
      add_child( p->action.frostfire_empowerment );
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

    if ( frostfire && p()->buffs.frostfire_empowerment->check() )
    {
      // Buff is decremented with a short delay, allowing two spells to benefit.
      // TODO: Double check this later
      make_event( *sim, 15_ms, [ this ] { p()->buffs.frostfire_empowerment->decrement(); } );
      p()->state.trigger_ff_empowerment = true;
    }
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      get_td( s->target )->debuffs.controlled_destruction->trigger();

      if ( frostfire && p()->state.trigger_ff_empowerment )
      {
        p()->state.trigger_ff_empowerment = false;
        p()->action.frostfire_empowerment->execute_on_target( s->target, p()->talents.frostfire_empowerment->effectN( 2 ).percent() * s->result_total );
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

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    if ( !p()->buffs.combustion->check() )
      m *= master_of_flame_mult;

    if ( frostfire && p()->state.trigger_ff_empowerment )
      m *= 1.0 + p()->buffs.frostfire_empowerment->data().effectN( 3 ).percent();

    return m;
  }
};

struct flamestrike_pyromaniac_t final : public fire_mage_spell_t
{
  flamestrike_pyromaniac_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 460476 ) )
  {
    background = proc = true;
    triggers.ignite = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value(); // TODO: Check this
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

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
  flamestrike_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    hot_streak_spell_t( n, p, p->talents.flamestrike_1.ok() ? p->talents.flamestrike_1 : p->talents.flamestrike_2 )
  {
    parse_options( options_str );
    triggers.ignite = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
    // 1 ms travel delay to handle Majesty of the Phoenix correctly
    travel_delay = 0.001;

    if ( p->talents.pyromaniac.ok() )
      pyromaniac_action = get_action<flamestrike_pyromaniac_t>( "flamestrike_pyromaniac", p );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

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
    hot_streak_spell_t::execute();

    if ( hit_any_target )
      handle_hot_streak( execute_state->crit_chance, p()->talents.fuel_the_fire->ok() ? HS_CUSTOM : HS_HIT );
  }
};

struct glacial_assault_t final : public frost_mage_spell_t
{
  glacial_assault_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 379029 ) )
  {
    background = proc = true;
    aoe = -1;
    freezing_stacks = as<int>( p->talents.glacial_assault->effectN( 2 ).base_value() );
    affected_by.freeze_and_shatter_1 = true; // TODO: Technically, this is effect 5 that does it by label
  }
};

struct flurry_bolt_t final : public frost_mage_spell_t
{
  flurry_bolt_t( std::string_view n, mage_t* p ) :
    frost_mage_spell_t( n, p, p->find_spell( 228354 ) )
  {
    background = proc = true;
    freezing_stacks = as<int>( p->spec.shatter->effectN( 2 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

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

  const int pulses;
  const timespan_t pulse_time = 0.4_s;

  flurry_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.flurry ),
    flurry_bolt( get_action<flurry_bolt_t>( "flurry_bolt", p ) ),
    pulses( as<int>( data().effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    may_miss = false;
    triggers.frostfire_empowerment = true; // Doesn't seem to need Heat Sink

    add_child( flurry_bolt );
    if ( p->talents.glacial_assault.ok() )
      add_child( p->action.glacial_assault );
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->state.brain_freeze_active = p()->buffs.brain_freeze->up();
    p()->buffs.brain_freeze->decrement();
    if ( p()->state.brain_freeze_active )
      p()->buffs.thermal_void->trigger();
    p()->trigger_splinter( p()->target );
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    // TODO: Only fires 3 instead of 4 bolts at the secondary target
    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( pulse_time )
      .target( s->target )
      .n_pulses( pulses )
      .action( flurry_bolt ), true );
  }
};

struct frostbolt_t final : public frost_mage_spell_t
{
  const bool frostfire;

  double fof_chance = 0.0;
  double bf_chance = 0.0;

  frostbolt_t( std::string_view n, mage_t* p, std::string_view options_str, bool frostfire_ = false ) :
    frost_mage_spell_t( n, p, frostfire_ ? p->talents.frostfire_bolt : p->find_class_spell( "Frostbolt" ) ),
    frostfire( frostfire_ )
  {
    parse_options( options_str );
    enable_calculate_on_impact( frostfire ? 468655 : 228597 );
    affected_by.overflowing_energy = true;
    triggers.frostfire_empowerment = true;

    fof_chance = p->talents.fingers_of_frost->effectN( 1 ).percent();
    bf_chance = p->talents.brain_freeze->effectN( 1 ).percent();
    freezing_stacks = as<int>( p->spec.shatter->effectN( 1 ).base_value() );

    if ( data().ok() && p->talents.frostfire_empowerment.ok() )
      add_child( p->action.frostfire_empowerment );
  }

  void init_finished() override
  {
    proc_brain_freeze = p()->get_proc( "Brain Freeze from Frostbolt" );
    proc_fof = p()->get_proc( "Fingers of Frost from Frostbolt" );

    frost_mage_spell_t::init_finished();
  }

  timespan_t execute_time() const override
  {
    if ( frostfire && p()->buffs.frostfire_empowerment->check() )
      return 0_ms;

    return frost_mage_spell_t::execute_time();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = frost_mage_spell_t::composite_da_multiplier( s );

    // TODO: This can get consumed by a Glacial Spike that hits before FFB does.
    // The GS benefits (and consumes) only from the extra damage, the explosion
    // still happens on FFB hit.
    // TODO: Seems to be doing +100% damage rather than the +60% mentioned in desc
    // Possibly talent's effect 1? Same goes for the Fire version
    if ( frostfire && p()->state.trigger_ff_empowerment )
      m *= 1.0 + p()->buffs.frostfire_empowerment->data().effectN( 3 ).percent();

    return m;
  }

  void execute() override
  {
    frost_mage_spell_t::execute();

    p()->trigger_fof( fof_chance, proc_fof );
    p()->trigger_brain_freeze( bf_chance, proc_brain_freeze );
    p()->trigger_splinter( p()->target );

    if ( frostfire && p()->buffs.frostfire_empowerment->check() )
    {
      // Buff is decremented with a short delay, allowing two spells to benefit.
      // TODO: Double check this later
      make_event( *sim, 15_ms, [ this ] { p()->buffs.frostfire_empowerment->decrement(); } );
      p()->state.trigger_ff_empowerment = true;
    }
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( s->result == RESULT_CRIT && p()->talents.frostbite.ok() )
      p()->trigger_freezing( s->target, as<int>( p()->talents.frostbite->effectN( 1 ).base_value() ), freezing_source );

    if ( result_is_hit( s->result ) && frostfire && p()->state.trigger_ff_empowerment )
    {
      p()->state.trigger_ff_empowerment = false;
      p()->action.frostfire_empowerment->execute_on_target( s->target, p()->talents.frostfire_empowerment->effectN( 2 ).percent() * s->result_total );
    }
  }

  bool ready() override
  {
    // Buff only needs to be missing at the start of the cast
    if ( p()->buffs.glacial_spike->check() && p()->executing != this )
      return false;

    return frost_mage_spell_t::ready();
  }
};

struct frost_nova_t final : public mage_spell_t
{
  frost_nova_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->find_class_spell( "Frost Nova" ) )
  {
    parse_options( options_str );
    aoe = -1;
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
    background = proc = true;
    if ( p->talents.wintertide.ok() )
    {
      freezing_chance = p->talents.wintertide->proc_chance();
      freezing_stacks = 1; // Not in spell data
    }
  }
};

struct frozen_orb_t final : public frost_mage_spell_t
{
  action_t* frozen_orb_bolt;

  frozen_orb_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.frozen_orb ),
    frozen_orb_bolt( get_action<frozen_orb_bolt_t>( "frozen_orb_bolt", p ) )
  {
    parse_options( options_str );
    may_miss = false;
    add_child( frozen_orb_bolt );
  }

  void init_finished() override
  {
    proc_fof = p()->get_proc( "Fingers of Frost from Frozen Orb" );
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
    p()->buffs.freezing_rain->trigger();
    if ( p()->talents.everlasting_frost.ok() )
      p()->trigger_fof( 1.0, proc_fof );
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    int pulse_count = 30;
    timespan_t pulse_time = 0.5_s;
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
      int count = as<int>( p()->talents.splintering_orbs->effectN( 6 ).base_value() ) - 1;
      make_repeating_event( *sim, pulse_time, [ this ] { p()->trigger_splinter( nullptr ); }, count );
    }
  }
};

struct duality_pyroblast_t final : public mage_spell_t
{
  duality_pyroblast_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 1262863 ) )
  {
    // TODO: This might actually be consuming mana
    background = proc = true;
    if ( p->talents.elemental_conduit.ok() )
      triggers.molten_chill_ignite = true;
  }
};

struct glacial_spike_t final : public frost_mage_spell_t
{
  double fof_chance = 0.0;
  double bf_chance = 0.0;
  action_t* duality_pyroblast = nullptr;

  glacial_spike_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->find_spell( 199786 ) )
  {
    parse_options( options_str );
    enable_calculate_on_impact( 228600 );
    affected_by.overflowing_energy = true;

    fof_chance = p->talents.fingers_of_frost->effectN( 1 ).percent();
    bf_chance = p->talents.brain_freeze->effectN( 1 ).percent();
    freezing_stacks = as<int>( p->spec.shatter->effectN( 3 ).base_value() );
    chain_multiplier = 1.0; // The spell data value isn't used
    // TODO: GS seems to autocast if FFB hits while the GS buff is up, not sure what causes this

    if ( p->talents.duality.ok() )
    {
      duality_pyroblast = get_action<duality_pyroblast_t>( "duality_pyroblast", p );
      add_child( duality_pyroblast );
    }

    if ( p->specialization() == MAGE_FROST && p->talents.flash_freezeburn.ok() )
      add_child( p->action.flash_freezeburn );
  }

  void init_finished() override
  {
    proc_brain_freeze = p()->get_proc( "Brain Freeze from Glacial Spike" );
    proc_fof = p()->get_proc( "Fingers of Frost from Glacial Spike" );

    frost_mage_spell_t::init_finished();
  }

  void execute() override
  {
    frost_mage_spell_t::execute();
    p()->buffs.glacial_spike->decrement();
    p()->state.icicles = 0;

    p()->trigger_fof( fof_chance, proc_fof );
    p()->trigger_fof( p()->talents.flash_freeze->effectN( 1 ).percent(), proc_fof );
    p()->trigger_brain_freeze( bf_chance, proc_brain_freeze );
    p()->trigger_splinter( p()->target );
    // TODO: Currently doesn't work
    p()->trigger_splinter( p()->target, as<int>( p()->talents.signature_spell->effectN( 2 ).base_value() ) );

    if ( duality_pyroblast )
      duality_pyroblast->execute_on_target( target );

    if ( p()->talents.flash_freezeburn.ok() )
      p()->buffs.frostfire_empowerment->trigger();
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( s->result == RESULT_CRIT && p()->talents.frostbite.ok() )
      p()->trigger_freezing( s->target, as<int>( p()->talents.frostbite->effectN( 1 ).base_value() ), freezing_source );

    if ( result_is_hit( s->result ) && p()->action.flash_freezeburn )
      p()->action.flash_freezeburn->execute_on_target( s->target, p()->talents.flash_freezeburn->effectN( 2 ).percent() * s->result_total );
  }

  bool ready() override
  {
    // Buff needs to be present at the start and also at the end of the cast
    if ( !p()->buffs.glacial_spike->check() )
      return false;

    return frost_mage_spell_t::ready();
  }
};

struct shatter_t final : public mage_spell_t
{
  shatter_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 1246949 ) )
  {
    background = proc = true;
  }

  double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p()->buffs.permafrost_lances->check_value();
    if ( p()->state.fingers_of_frost_active )
      am *= 1.0 + p()->sets->set( MAGE_FROST, MID1, B4 )->effectN( 1 ).percent();

    return am;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = mage_spell_t::composite_crit_damage_bonus_multiplier();

    // Technically, there's a ticking effect that updates the multiplier every second, but this is be close enough.
    cm *= 1.0 + p()->talents.deep_shatter->effectN( 1 ).percent() * p()->cache.spell_crit_chance();

    return cm;
  }
};

struct ice_lance_t final : public frost_mage_spell_t
{
  struct ice_lance_impact_t final : public frost_mage_spell_t
  {
    int freezing_consume;
    shatter_source_t* shatter_source;
    shatter_source_t* shatter_source_cleave;

    ice_lance_impact_t( std::string_view n, mage_t* p ) :
      frost_mage_spell_t( n, p, p->find_spell( 228598 ) ),
      freezing_consume( as<int>( p->spec.shatter->effectN( 4 ).base_value() ) ),
      shatter_source( p->get_shatter_source( name_str, freezing_consume ) ),
      shatter_source_cleave( p->get_shatter_source( "Ice Lance cleave", freezing_consume ) )
    {
      background = true;
      // Spell data contains the AoE effect which is disabled unless you pick Fractured Frost
      // Fix the spell power mod and use base_aoe_multiplier for the cleave
      double primary_coef = data().effectN( 1 ).sp_coeff();
      double secondary_coef = data().effectN( 2 ).sp_coeff();
      spell_power_mod.direct = primary_coef;
      base_aoe_multiplier = secondary_coef / primary_coef;

      if ( p->talents.fractured_frost.ok() )
        aoe = 1 + as<int>( p->talents.fractured_frost->effectN( 1 ).base_value() );
    }

    void impact( action_state_t* s ) override
    {
      frost_mage_spell_t::impact( s );

      if ( result_is_hit( s->result ) && p()->action.shatter.ice_lance )
      {
        int stacks = p()->trigger_shatter( s->target, p()->action.shatter.ice_lance, freezing_consume,
                                           s->chain_target == 0 ? shatter_source : shatter_source_cleave, p()->state.fingers_of_frost_active );
        if ( s->chain_target == 0 && p()->talents.force_of_will.ok() )
          p()->trigger_splinter( s->target, stacks / as<int>( p()->talents.force_of_will->effectN( 3 ).base_value() ) );
        if ( stacks == freezing_consume )
          p()->trigger_freezing( s->target, as<int>( p()->talents.polished_focus->effectN( 3 ).base_value() ), freezing_source );
      }
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      frost_mage_spell_t::available_targets( tl );

      if ( p()->state.fingers_of_frost_active && !p()->options.fof_requires_freezing )
        return tl.size();

      if ( p()->options.il_requires_freezing )
      {
        range::erase_remove( tl, [ this ] ( player_t* t )
        {
          if ( t == target ) return false;
          if ( auto td = find_td( t ) ) return td->debuffs.freezing->check() == 0;
          return true;
        } );
      }

      if ( p()->options.il_sort_by_freezing )
      {
        auto value = [ this ] ( player_t* t )
        {
          if ( t == target ) return std::numeric_limits<int>::max();
          if ( auto td = find_td( t ) ) return td->debuffs.freezing->check();
          return 0;
        };

        range::sort( tl, [ value ] ( player_t* a, player_t* b ) { return value( a ) > value( b ); } );
      }

      return tl.size();
    }

    std::vector<player_t*>& target_list() const override
    {
      // Can't cache valid targets as they could change at any moment.
      target_cache.is_valid = false;
      return frost_mage_spell_t::target_list();
    }
  };

  action_t* thermal_void_il = nullptr;

  ice_lance_t( std::string_view n, mage_t* p, std::string_view options_str, bool thermal_void = false ) :
    frost_mage_spell_t( n, p, p->talents.ice_lance )
  {
    parse_options( options_str );
    impact_action = get_action<ice_lance_impact_t>( "ice_lance_impact", p );

    if ( thermal_void )
    {
      background = proc = true;
      cooldown->duration = 0_ms;
      // TODO: Seems to actually consume mana despite being a proc
      // base_costs[ RESOURCE_MANA ] = 0;
      return;
    }

    add_child( impact_action );
    if ( p->spec.shatter->ok() )
      add_child( p->action.shatter.ice_lance );
    if ( p->talents.thermal_void.ok() )
      thermal_void_il = get_action<ice_lance_t>( "thermal_void_ice_lance", p, "", true );
  }

  void execute() override
  {
    frost_mage_spell_t::execute();
    if ( background )
      return;

    p()->state.fingers_of_frost_active = p()->buffs.fingers_of_frost->up();
    p()->buffs.fingers_of_frost->decrement();
    if ( thermal_void_il && p()->buffs.thermal_void->check() )
    {
      p()->buffs.thermal_void->decrement();
      make_event( *sim, 0.8_s, [ this, t = target ] { thermal_void_il->execute_on_target( t ); } );
    }
  }
};

struct ice_nova_t final : public frost_mage_spell_t
{
  ice_nova_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ice_nova )
  {
    parse_options( options_str );
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
    if ( p->talents.cone_of_frost.ok() )
    {
      // TODO: Currently seems to ignore the main target if possible
      freezing_stacks = as<int>( p->talents.cone_of_frost->effectN( 1 ).base_value() );
      freezing_targets = as<int>( p->talents.cone_of_frost->effectN( 2 ).base_value() );
    }
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    p()->trigger_crowd_control( s, MECHANIC_FREEZE );
  }
};

struct fire_blast_t final : public fire_mage_spell_t
{
  fire_blast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    fire_mage_spell_t( n, p, p->talents.fire_blast.ok() ? p->talents.fire_blast : p->find_class_spell( "Fire Blast" ) )
  {
    parse_options( options_str );
    triggers.hot_streak = TT_ALL_TARGETS;
    triggers.ignite = triggers.from_the_ashes = triggers.frostfire_empowerment = true;

    if ( p->talents.fire_blast.ok() )
    {
      base_crit += 1.0;
      usable_while_casting = true;
    }
  }

  int n_targets() const override
  {
    if ( p()->buffs.glorious_incandescence->check() )
      return as<int>( p()->buffs.glorious_incandescence->data().effectN( 3 ).base_value() );
    else
      return fire_mage_spell_t::n_targets();
  }

  void execute() override
  {
    if ( p()->buffs.glorious_incandescence->check() )
      p()->state.trigger_glorious_incandescence = true;

    fire_mage_spell_t::execute();

    p()->buffs.glorious_incandescence->decrement();
    p()->buffs.feel_the_burn->trigger();
  }

  void impact( action_state_t* s ) override
  {
    if ( s->chain_target == 0 )
      spread_ignite( s->target );

    fire_mage_spell_t::impact( s );

    // As of 11.1, only triggers from Fire Blasts cast by Fire Mages.
    if ( result_is_hit( s->result ) && s->chain_target == 0 && p()->specialization() == MAGE_FIRE )
      trigger_glorious_incandescence( s->target );
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = fire_mage_spell_t::recharge_rate_multiplier( cd );

    if ( &cd == cooldown )
      m /= 1.0 + p()->buffs.fiery_rush->check_value();

    return m;
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
    background = proc = ground_aoe = true;
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
  int freezing_consume;
  shatter_source_t* shatter_source;

  meteor_impact_t( std::string_view n, mage_t* p, action_t* burn, meteor_type type_ ) :
    fire_mage_spell_t( n, p, p->find_spell( type_ == meteor_type::ISOTHERMIC ? 438607 : 351140 ) ),
    type( type_ ),
    meteor_burn( burn ),
    meteor_burn_duration( p->find_spell( 175396 )->duration() ),
    meteor_burn_pulse_time( p->find_spell( 155158 )->effectN( 1 ).period() ),
    freezing_consume( as<int>( p->spec.shatter->effectN( 5 ).base_value() ) ),
    shatter_source( p->get_shatter_source( name_str, freezing_consume ) )
  {
    aoe = -1;
    reduced_aoe_targets = 8;
    background = proc = triggers.ignite = true;
    if ( p->talents.elemental_conduit.ok() )
      triggers.molten_chill_ignite = true;

    // With Deep Impact, Meteor deals extra damage to the target closest to the impact point.
    // For simplicity, we assume that will be the main target.
    double m = 1.0 + p->talents.deep_impact->effectN( 1 ).percent();
    base_multiplier     *= m;
    base_aoe_multiplier /= m;
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    p()->ground_aoe_expiration[ AOE_METEOR_BURN ] = sim->current_time() + meteor_burn_duration;
    p()->trigger_meteor_burn( meteor_burn, target, meteor_burn_pulse_time, meteor_burn_duration );
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) && p()->action.shatter.meteor )
      p()->trigger_shatter( s->target, p()->action.shatter.meteor, freezing_consume, shatter_source );
  }
};

struct meteor_t final : public fire_mage_spell_t
{
  timespan_t meteor_delay;
  const meteor_type type;

  meteor_t( std::string_view n, mage_t* p, std::string_view options_str, meteor_type type_ = meteor_type::NORMAL ) :
    fire_mage_spell_t( n, p, type_ == meteor_type::NORMAL ? p->talents.meteor : p->find_spell( 153561 ) ),
    meteor_delay( p->find_spell( 177345 )->duration() ),
    type( type_ )
  {
    parse_options( options_str );

    if ( !data().ok() )
      return;

    std::string_view burn_name;
    std::string_view impact_name;

    switch ( type )
    {
      case meteor_type::NORMAL:
        burn_name = "meteor_burn";
        impact_name = "meteor_impact";
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

    if ( p->spec.shatter->ok() )
      add_child( p->action.shatter.meteor );

    if ( type != meteor_type::NORMAL )
    {
      background = proc = true;
      cooldown->duration = 0_ms;
      base_costs[ RESOURCE_MANA ] = 0;
      return;
    }

    if ( p->talents.isothermic_core.ok() )
      add_child( p->action.isothermic_comet_storm );
  }

  timespan_t travel_time() const override
  {
    // Travel time cannot go lower than 1 second to give time for Meteor to visually fall.
    return std::max( meteor_delay * p()->cache.spell_cast_speed(), 1.0_s );
  }

  void execute() override
  {
    fire_mage_spell_t::execute();

    if ( !background && p()->talents.flash_freezeburn.ok() )
      p()->buffs.frostfire_empowerment->trigger();

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
    background = proc = triggers.ignite = true;
  }

  void execute() override
  {
    mage_spell_t::execute();

    if ( p()->specialization() == MAGE_FIRE )
      p()->cooldowns.fire_blast->adjust( -p()->talents.glorious_incandescence->effectN( 2 ).time_value(), true, false );
  }
};

struct meteorite_t final : public mage_spell_t
{
  meteorite_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 449559 ) )
  {
    background = proc = true;
    impact_action = get_action<meteorite_impact_t>( "meteorite_impact", p );
    travel_delay = p->find_spell( 449560 )->missile_speed();

    add_child( impact_action );
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

struct duality_glacial_spike_t final : public mage_spell_t
{
  // TODO: Still seems to be using the old TWW set bonus GS
  // TODO: Also affected by Flash Freezeburn
  duality_glacial_spike_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 1236209 ) )
  {
    enable_calculate_on_impact( 1236211 );
    background = proc = true;
    if ( p->talents.elemental_conduit.ok() )
      triggers.ignite = true;

    if ( p->specialization() == MAGE_FIRE && p->talents.flash_freezeburn.ok() )
      add_child( p->action.flash_freezeburn );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    // TODO: This is probably a bug
    if ( result_is_hit( s->result ) && p()->action.flash_freezeburn )
      p()->action.flash_freezeburn->execute_on_target( s->target, p()->talents.flash_freezeburn->effectN( 2 ).percent() * s->result_total );
  }
};

struct pyroblast_pyromaniac_t final : public fire_mage_spell_t
{
  action_t* duality_gs = nullptr;

  pyroblast_pyromaniac_t( std::string_view n, mage_t* p ) :
    fire_mage_spell_t( n, p, p->find_spell( 460475 ) )
  {
    background = proc = true;
    triggers.ignite = true;
    if ( p->talents.duality.ok() )
      duality_gs = get_action<duality_glacial_spike_t>( "duality_glacial_spike", p );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = fire_mage_spell_t::composite_da_multiplier( s );

    m *= 1.0 + p()->buffs.hyperthermia_damage->check_stack_value();

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
    if ( p()->buffs.hyperthermia->check() )
      p()->buffs.hyperthermia_damage->trigger();
    if ( rng().roll( p()->talents.duality->effectN( 1 ).percent() ) )
      duality_gs->execute_on_target( target );
  }
};

struct pyroblast_t final : public hot_streak_spell_t
{
  action_t* duality_gs = nullptr;

  pyroblast_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    hot_streak_spell_t( n, p, p->talents.pyroblast )
  {
    parse_options( options_str );
    triggers.hot_streak = TT_MAIN_TARGET;
    triggers.ignite = triggers.from_the_ashes = true;

    if ( p->talents.pyromaniac.ok() )
      pyromaniac_action = get_action<pyroblast_pyromaniac_t>( "pyroblast_pyromaniac", p );

    if ( p->talents.duality.ok() )
    {
      duality_gs = get_action<duality_glacial_spike_t>( "duality_glacial_spike", p );
      add_child( duality_gs );
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hot_streak_spell_t::composite_da_multiplier( s );

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
      get_td( s->target )->debuffs.controlled_destruction->trigger();
  }

  void execute() override
  {
    hot_streak_spell_t::execute();

    if ( rng().roll( p()->talents.duality->effectN( 1 ).percent() ) )
      duality_gs->execute_on_target( target );
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
  proc_t* freezing_source;

  splintering_ray_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 418735 ) ),
    freezing_source( p->get_proc( "Freezing applied (Splintering Ray)" ) )
  {
    background = proc = true;
    target_filter_callback = secondary_targets_only();
    base_dd_min = base_dd_max = 1.0;
    // TODO: Seems to hit 1 fewer target
    aoe--;
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

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      debug_cast<mage_t*>( player )->trigger_freezing( s->target, 1, freezing_source );
  }
};

struct ray_of_frost_t final : public frost_mage_spell_t
{
  action_t* splintering_ray = nullptr;

  ray_of_frost_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.ray_of_frost )
  {
    parse_options( options_str );
    channeled = true;

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

    p()->trigger_freezing( d->target, 1, freezing_source ); // Not in spell data
    p()->trigger_splinter( d->target, as<int>( p()->talents.augury_abounds->effectN( 3 ).base_value() ) );

    // TODO: FoF is granted through spell 269748. Unfortunately, Blizzard forgot to change its
    // period to 2 sec when Ray of Frost was changed to 4 sec channel, so now it only grants a single FoF.
    if ( p()->talents.crystalline_refraction.ok() && ( d->current_tick == 4 || d->current_tick == 8 ) )
      p()->trigger_fof( 1.0, proc_fof );

    if ( p()->talents.glaciate.ok() )
    {
      // Seems to trigger an Icicle each tick
      // TODO: Doesn't trigger the GS buff on its own, needs the natural Icicle regen to happen
      p()->trigger_icicle( 1, !p()->bugs );
      timespan_t cdr = -p()->talents.glaciate->effectN( 2 ).time_value();
      p()->cooldowns.flurry->adjust( cdr, false );
      p()->cooldowns.frozen_orb->adjust( cdr, false );
    }

    if ( splintering_ray )
      splintering_ray->execute_on_target( d->target, p()->talents.splintering_ray->effectN( 1 ).percent() * d->state->result_total );

    // TODO: use the spell data in some way
    if ( p()->action.hand_of_frost && p()->talents.hand_of_frost_3.ok() && d->current_tick % 2 == 1 )
      p()->action.hand_of_frost->execute_on_target( d->target );
  }

  void execute() override
  {
    frost_mage_spell_t::execute();
    p()->buffs.comet_storm->trigger();
    p()->buffs.augury_abounds->trigger();
  }

  bool ready() override
  {
    if ( p()->buffs.comet_storm->check() )
      return false;

    return frost_mage_spell_t::ready();
  }
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
  }
};

struct supernova_t final : public mage_spell_t
{
  supernova_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    mage_spell_t( n, p, p->talents.supernova )
  {
    parse_options( options_str );
    aoe = -1;
    affected_by.savant = true;
    triggers.clearcasting = true;

    double sn_mult = 1.0 + p->talents.supernova->effectN( 1 ).percent();
    base_multiplier     *= sn_mult;
    base_aoe_multiplier /= sn_mult;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );
    p()->trigger_crowd_control( s, MECHANIC_KNOCKBACK );
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

    // use indices since it's possible to spawn new actors when bloodlust is triggered
    for ( size_t i = 0; i < sim->player_non_sleeping_list.size(); i++ )
    {
      auto* p = sim->player_non_sleeping_list[ i ];
      if ( p->is_pet() || p->buffs.exhaustion->check() )
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

    if ( p->talents.touch_of_the_archmage_3.ok() )
      add_child( p->action.touch_of_the_archmage );
  }

  void execute() override
  {
    arcane_mage_spell_t::execute();
    p()->trigger_arcane_charge( as<int>( data().effectN( 2 ).base_value() ) );
    p()->trigger_splinter( target, as<int>( p()->talents.signature_spell->effectN( 1 ).base_value() ) );
    if ( p()->talents.aegwynns_technique.ok() )
      p()->trigger_clearcasting();
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      const auto& td = get_td( s->target )->debuffs;
      td.touch_of_the_magi->expire();
      td.touch_of_the_magi->trigger();
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
    background = proc = true;
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
};

struct touch_of_the_archmage_t final : public spell_t
{
  touch_of_the_archmage_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 1258036 ) )
  {
    background = proc = true;
    aoe = -1;
    base_dd_min = base_dd_max = 1.0;
    double m = 1.0 + p->talents.touch_of_the_archmage_3->effectN( 2 ).percent();
    base_multiplier     *= m;
    base_aoe_multiplier /= m;
  }
};

struct summon_water_elemental_t final : public frost_mage_spell_t
{
  summon_water_elemental_t( std::string_view n, mage_t* p, std::string_view options_str ) :
    frost_mage_spell_t( n, p, p->talents.summon_water_elemental )
  {
    parse_options( options_str );
    harmful = false;
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

struct arcane_echo_t final : public arcane_mage_spell_t
{
  arcane_echo_t( std::string_view n, mage_t* p ) :
    arcane_mage_spell_t( n, p, p->find_spell( 342232 ) )
  {
    aoe = -1;
    reduced_aoe_targets = p->talents.arcane_echo->effectN( 1 ).base_value();
    background = proc = true;
  }
};

struct frostfire_empowerment_t final : public spell_t
{
  proc_t* freezing_source;

  frostfire_empowerment_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 431186 ) ),
    freezing_source( p->get_proc( "Freezing applied (Frostfire Empowerment)" ) )
  {
    background = proc = true;
    target_filter_callback = secondary_targets_only();
    aoe = -1;
    base_dd_min = base_dd_max = 1.0;
    // TODO: Check how it behaves wrt the excluded main target
    reduced_aoe_targets = p->talents.frostfire_empowerment->effectN( 5 ).base_value();
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    mage_t* p = debug_cast<mage_t*>( player );
    if ( result_is_hit( s->result ) )
      p->trigger_freezing( s->target, as<int>( p->talents.frostfire_empowerment->effectN( 4 ).base_value() ), freezing_source );
  }
};

struct flash_freezeburn_t final : public spell_t
{
  proc_t* freezing_source;

  flash_freezeburn_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( 1278079 ) ),
    freezing_source( p->get_proc( "Freezing applied (Flash Freezeburn)" ) )
  {
    background = proc = true;
    target_filter_callback = secondary_targets_only();
    base_dd_min = base_dd_max = 1.0;
    // TODO: Usually hits one fewer target
    // It's possible it picks 5 random targets and if one of them happens to be
    // the main target, it simply skips it. See also: Splintering Ray
    aoe--;
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      debug_cast<mage_t*>( player )->trigger_freezing( s->target, 1, freezing_source );
  }
};

struct controlled_instincts_t final : public spell_t
{
  controlled_instincts_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( p->specialization() == MAGE_FROST ? 444487 : 444720 ) )
  {
    background = proc = true;
    target_filter_callback = secondary_targets_only();
    // Only hits 5 targets despite max_targets being 6
    aoe -= 1;
    // TODO: The tooltip still mentions this, but it's untestable at the moment since it can't hit 6 or more targets
    reduced_aoe_targets = p->talents.controlled_instincts->effectN( 5 ).base_value();
    base_dd_min = base_dd_max = 1.0;
  }
};

struct splinter_recall_t final : public spell_t
{
  splinter_recall_t( std::string_view n, mage_t* p ) :
    spell_t( n, p, p->find_spell( p->specialization() == MAGE_FROST ? 443934 : 444736 ) )
  {
    background = proc = true;
    base_dd_min = base_dd_max = 1.0;
  }
};

struct embedded_splinter_t final : public mage_spell_t
{
  embedded_splinter_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( p->specialization() == MAGE_FROST ? 443740 : 444735 ) )
  {
    background = proc = true;
  }

  timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t duration ) const override
  {
    return duration;
  }

  double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p()->cache.mastery() * p()->spec.savant->effectN( 6 ).mastery_value();

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

    // If the dot ended due to the target dying, transfer a random portion of the splinters to a nearby target.
    if ( d->target->is_sleeping() )
    {
      int transfer = 1 + rng().range( stack );
      make_event( *sim, [ this, transfer ] { p()->trigger_splinter( nullptr, transfer ); } );
    }
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
    background = proc = true;

    if ( p->talents.controlled_instincts.ok() )
      controlled_instincts = get_action<controlled_instincts_t>( "controlled_instincts", p );

    freezing_chance = p->talents.infused_splinters->effectN( 2 ).percent();
    freezing_stacks = as<int>( p->talents.infused_splinters->effectN( 4 ).base_value() );

    if ( splinterstorm )
      return;

    impact_action = p->action.splinter_dot;
    add_child( impact_action );

    if ( controlled_instincts )
      add_child( controlled_instincts );
    if ( p->action.splinter_recall )
      add_child( p->action.splinter_recall );
    if ( p->action.splinterstorm )
      add_child( p->action.splinterstorm );
  }

  double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p()->cache.mastery() * p()->spec.savant->effectN( 7 ).mastery_value();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( controlled_instincts )
    {
      double pct = p()->talents.controlled_instincts->effectN( p()->specialization() == MAGE_FROST ? 4 : 1 ).percent();
      controlled_instincts->execute_on_target( s->target, pct * s->result_total );
    }

    p()->trigger_arcane_salvo( salvo_source, as<int>( p()->talents.infused_splinters->effectN( 3 ).base_value() ),
                               p()->talents.infused_splinters->effectN( 1 ).percent() );

    auto cd = p()->specialization() == MAGE_FROST ? p()->cooldowns.frozen_orb : p()->cooldowns.arcane_orb;
    cd->adjust( -p()->talents.spellfrost_teachings->effectN( p()->specialization() == MAGE_FROST ? 2 : 1 ).time_value(), false );
  }

  timespan_t travel_time() const override
  {
    timespan_t t = mage_spell_t::travel_time();

    // Spread the splinter impacts around a bit. Note that we have to use gauss( double, double )
    // here because the timespan one doesn't produce negative values.
    if ( !splinterstorm )
      t += timespan_t::from_millis( rng().gauss( 0.0, 5.0 ) );

    return std::max( t, 0_ms );
  }
};

struct hand_of_frost_t final : public mage_spell_t
{
  hand_of_frost_t( std::string_view n, mage_t* p ) :
    mage_spell_t( n, p, p->find_spell( 1262769 ) )
  {
    background = proc = true;
    freezing_stacks = as<int>( p->talents.hand_of_frost_1->effectN( 2 ).base_value() );
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.hand_of_frost->trigger();
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

struct icicle_event_t final : public mage_event_t
{
  icicle_event_t( mage_t& m, timespan_t delta_time ) :
    mage_event_t( m, delta_time )
  { }

  const char* name() const override
  { return "icicle_event"; }

  static void schedule_next( mage_t* p, bool randomize = false )
  {
    timespan_t next = p->talents.icicles->effectN( 1 ).period();
    next *= p->cache.spell_haste();  // Does not use spell speed
    if ( randomize ) next *= p->rng().real();
    p->events.icicle = make_event<icicle_event_t>( *p->sim, *p, next );
  }

  void execute() override
  {
    mage->events.icicle = nullptr;
    mage->trigger_icicle();
    schedule_next( mage );
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

struct splinterstorm_event_t final : public mage_event_t
{
  splinterstorm_event_t( mage_t& m, timespan_t delta_time ) :
    mage_event_t( m, delta_time )
  { }

  const char* name() const override
  { return "splinterstorm_event"; }

  static void schedule_next( mage_t* p, bool randomize = false )
  {
    timespan_t next = p->talents.splinterstorm->effectN( 2 ).period();
    if ( randomize ) next *= p->rng().real();
    p->events.splinterstorm = make_event<splinterstorm_event_t>( *p->sim, *p, next );
  }

  void execute() override
  {
    mage->events.splinterstorm = nullptr;

    player_t* t = nullptr;
    if ( mage->target && !mage->target->is_sleeping() && mage->target->is_enemy() )
      t = mage->target;
    else if ( const auto& tl = sim().target_non_sleeping_list; !tl.empty() )
      t = rng().range( tl );

    if ( t && mage->state.embedded_splinters >= as<int>( mage->talents.splinterstorm->effectN( 1 ).base_value() ) )
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

      mage->state.remaining_splinterstorm += splinters;
      make_repeating_event( sim(), 100_ms, [ m = mage, a = mage->action.splinterstorm, t ]
        { a->execute_on_target( t ); m->state.remaining_splinterstorm--; }, splinters );

      if ( mage->specialization() == MAGE_FROST )
        mage->trigger_brain_freeze( mage->talents.splinterstorm->effectN( 5 ).percent(), mage->procs.brain_freeze_splinterstorm, 0_ms );
      else
        // Doesn't seem to be affected by Illuminated Thoughts.
        // TODO: get more data and double check
        mage->trigger_clearcasting( mage->talents.splinterstorm->effectN( 4 ).percent() );
    }

    schedule_next( mage );
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

  void execute() override
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
  debuffs.controlled_destruction = make_buff( *this, "controlled_destruction", mage->find_spell( 453268 ) )
                                     ->set_default_value( 0.1 * mage->talents.controlled_destruction->effectN( 1 ).percent() )
                                     ->set_chance( mage->talents.controlled_destruction.ok() );
  debuffs.freezing               = make_buff( *this, "freezing", mage->find_spell( 1221389 ) )
                                     ->set_expire_callback( [ mage ] ( buff_t*, int stacks, timespan_t duration )
                                       {
                                         if ( duration != 0_ms ) return;
                                         for ( int i = 0; i < stacks; i++ )
                                           mage->procs.freezing_expired->occur();
                                       } )
                                     ->set_chance( mage->spec.shatter->ok() );
  debuffs.molten_fury            = make_buff( *this, "molten_fury", mage->find_spell( 458910 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->set_chance( mage->talents.molten_fury.ok() );
  debuffs.touch_of_the_archmage  = make_buff( *this, "touch_of_the_archmage", mage->find_spell( 1258134 ) )
                                     ->set_tick_callback( [ mage ] ( buff_t* b, int, timespan_t )
                                       { mage->action.touch_of_the_archmage->execute_on_target( b->player, b->check_value() ); } )
                                     ->set_chance( mage->talents.touch_of_the_archmage_3.ok() );
  debuffs.touch_of_the_magi      = make_buff<buffs::touch_of_the_magi_t>( this );
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
  accumulated_rng(),
  sample_data(),
  spec(),
  state(),
  talents()
{
  // Cooldowns
  cooldowns.arcane_echo        = get_cooldown( "arcane_echo_icd"  );
  cooldowns.arcane_orb         = get_cooldown( "arcane_orb"       );
  cooldowns.combustion         = get_cooldown( "combustion"       );
  cooldowns.cone_of_cold       = get_cooldown( "cone_of_cold"     );
  cooldowns.dragons_breath     = get_cooldown( "dragons_breath"   );
  cooldowns.fire_blast         = get_cooldown( "fire_blast"       );
  cooldowns.flurry             = get_cooldown( "flurry"           );
  cooldowns.from_the_ashes     = get_cooldown( "from_the_ashes"   );
  cooldowns.frost_nova         = get_cooldown( "frost_nova"       );
  cooldowns.frozen_orb         = get_cooldown( "frozen_orb"       );
  cooldowns.meteor             = get_cooldown( "meteor"           );
  cooldowns.presence_of_mind   = get_cooldown( "presence_of_mind" );
  cooldowns.pyromaniac         = get_cooldown( "pyromaniac"       );

  // Options
  resource_regeneration = regen_type::DYNAMIC;
}

action_t* mage_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace actions;

  if ( talents.frostfire_bolt.ok() && ( name == "fireball" || name == "frostbolt" ) )
    return create_action( "frostfire_bolt", options_str );

  // Arcane
  if ( name == "arcane_barrage"    ) return new    arcane_barrage_t( name, this, options_str );
  if ( name == "arcane_blast"      ) return new      arcane_blast_t( name, this, options_str );
  if ( name == "arcane_missiles"   ) return new   arcane_missiles_t( name, this, options_str );
  if ( name == "arcane_pulse"      ) return new      arcane_pulse_t( name, this, options_str );
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
  if ( name == "pyroblast"         ) return new         pyroblast_t( name, this, options_str );

  // Frost
  if ( name == "blizzard"          ) return new          blizzard_t( name, this, options_str );
  if ( name == "cold_snap"         ) return new         cold_snap_t( name, this, options_str );
  if ( name == "comet_storm"       ) return new       comet_storm_t( name, this, options_str );
  if ( name == "flurry"            ) return new            flurry_t( name, this, options_str );
  if ( name == "frozen_orb"        ) return new        frozen_orb_t( name, this, options_str );
  if ( name == "glacial_spike"     ) return new     glacial_spike_t( name, this, options_str );
  if ( name == "ice_lance"         ) return new         ice_lance_t( name, this, options_str );
  if ( name == "ray_of_frost"      ) return new      ray_of_frost_t( name, this, options_str );

  // Shared
  if ( name == "arcane_explosion"  ) return new  arcane_explosion_t( name, this, options_str );
  if ( name == "arcane_intellect"  ) return new  arcane_intellect_t( name, this, options_str );
  if ( name == "blink"             ) return new             blink_t( name, this, options_str );
  if ( name == "cone_of_cold"      ) return new      cone_of_cold_t( name, this, options_str );
  if ( name == "counterspell"      ) return new      counterspell_t( name, this, options_str );
  if ( name == "dragons_breath"    ) return new    dragons_breath_t( name, this, options_str );
  if ( name == "fire_blast"        ) return new        fire_blast_t( name, this, options_str );
  if ( name == "frost_nova"        ) return new        frost_nova_t( name, this, options_str );
  if ( name == "frostbolt"         ) return new         frostbolt_t( name, this, options_str );
  if ( name == "ice_nova"          ) return new          ice_nova_t( name, this, options_str );
  if ( name == "mirror_image"      ) return new      mirror_image_t( name, this, options_str );
  if ( name == "shimmer"           ) return new           shimmer_t( name, this, options_str );
  if ( name == "supernova"         ) return new         supernova_t( name, this, options_str );
  if ( name == "time_warp"         ) return new         time_warp_t( name, this, options_str );

  // Special
  if ( name == "blink_any" || name == "any_blink" )
    return create_action( talents.shimmer.ok() ? "shimmer" : "blink", options_str );

  if ( name == "frostfire_bolt" )
    return specialization() == MAGE_FIRE
         ? static_cast<action_t*>( new  fireball_t( name, this, options_str, true ) )
         : static_cast<action_t*>( new frostbolt_t( name, this, options_str, true ) );

  // Pet spells
  if ( name == "summon_water_elemental" ) return new summon_water_elemental_t( name, this, options_str );
  if ( name == "freeze"                 ) return new                 freeze_t( name, this, options_str );
  if ( name == "water_jet"              ) return new              water_jet_t( name, this, options_str );

  return player_t::create_action( name, options_str );
}

void mage_t::create_actions()
{
  using namespace actions;

  if ( spec.shatter->ok() )
  {
    action.shatter.comet_storm = get_action<shatter_t>( "shatter_comet_storm", this );
    action.shatter.ice_lance   = get_action<shatter_t>( "shatter_ice_lance",   this );
    action.shatter.meteor      = get_action<shatter_t>( "shatter_meteor",      this );
  }

  if ( spec.ignite->ok() )
    action.ignite = get_action<ignite_t>( "ignite", this );

  if ( talents.arcane_familiar.ok() )
    action.arcane_assault = get_action<arcane_assault_t>( "arcane_assault", this );

  if ( talents.arcane_echo.ok() )
    action.arcane_echo = get_action<arcane_echo_t>( "arcane_echo", this );

  if ( talents.glacial_assault.ok() )
    action.glacial_assault = get_action<glacial_assault_t>( "glacial_assault", this );

  if ( talents.touch_of_the_magi.ok() )
    action.touch_of_the_magi_explosion = get_action<touch_of_the_magi_explosion_t>( "touch_of_the_magi_explosion", this );

  if ( talents.touch_of_the_archmage_3.ok() )
    action.touch_of_the_archmage = get_action<touch_of_the_archmage_t>( "touch_of_the_archmage", this );

  if ( talents.hand_of_frost_1.ok() )
    action.hand_of_frost = get_action<hand_of_frost_t>( "hand_of_frost", this );

  if ( talents.frostfire_empowerment.ok() )
    action.frostfire_empowerment = get_action<frostfire_empowerment_t>( "frostfire_empowerment", this );

  if ( talents.flash_freezeburn.ok() )
    action.flash_freezeburn = get_action<flash_freezeburn_t>( "flash_freezeburn", this );

  if ( talents.isothermic_core.ok() )
  {
    if ( specialization() == MAGE_FIRE )
      action.isothermic_comet_storm = get_action<comet_storm_t>( "isothermic_comet_storm", this, "", true );
    if ( specialization() == MAGE_FROST )
      action.isothermic_meteor = get_action<meteor_t>( "isothermic_meteor", this, "", meteor_type::ISOTHERMIC );
  }

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

  if ( specialization() == MAGE_FROST && talents.molten_chill.ok() )
    action.molten_chill_ignite = get_action<molten_chill_ignite_t>( "molten_chill_ignite", this );

  player_t::create_actions();
}

void mage_t::create_options()
{
  add_option( opt_timespan( "mage.arcane_missiles_chain_delay", options.arcane_missiles_chain_delay, 0_ms, timespan_t::max() ) );
  add_option( opt_float( "mage.arcane_missiles_chain_relstddev", options.arcane_missiles_chain_relstddev, 0.0, std::numeric_limits<double>::max() ) );
  add_option( opt_timespan( "mage.arcane_missiles_delay", options.arcane_missiles_delay, 0_ms, timespan_t::max() ) );
  add_option( opt_uint( "mage.initial_spellfire_spheres", options.initial_spellfire_spheres ) );
  add_option( opt_uint( "mage.initial_icicles", options.initial_icicles ) );
  add_option( opt_func( "mage.arcane_phoenix_rotation_override", [ this ] ( sim_t*, util::string_view, util::string_view value )
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
  add_option( opt_float( "mage.clearcasting_chance", options.clearcasting_chance ) );
  add_option( opt_float( "mage.it_clearcasting_chance", options.it_clearcasting_chance ) );
  add_option( opt_float( "mage.blast_clearcasting_chance", options.blast_clearcasting_chance ) );
  add_option( opt_float( "mage.blast_it_clearcasting_chance", options.blast_it_clearcasting_chance ) );
  add_option( opt_bool( "mage.fof_requires_freezing", options.fof_requires_freezing ) );
  add_option( opt_bool( "mage.il_requires_freezing", options.il_requires_freezing ) );
  add_option( opt_bool( "mage.il_sort_by_freezing", options.il_sort_by_freezing ) );
  add_option( opt_bool( "mage.randomize_si_target", options.randomize_si_target ) );
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

  if ( talents.summon_water_elemental.ok() && find_action( "summon_water_elemental" ) )
    pets.water_elemental = new pets::water_elemental::water_elemental_pet_t( sim, this );

  if ( talents.mirror_image.ok() && find_action( "mirror_image" ) )
  {
    int images = as<int>( talents.mirror_image->effectN( 2 ).base_value() );
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
  talents.blazing_barrier            = find_talent_spell( talent_tree::CLASS, "Blazing Barrier"            );
  talents.ice_barrier                = find_talent_spell( talent_tree::CLASS, "Ice Barrier"                );
  talents.prismatic_barrier          = find_talent_spell( talent_tree::CLASS, "Prismatic Barrier"          );
  // Row 2
  talents.alter_time                 = find_talent_spell( talent_tree::CLASS, "Alter Time"                 );
  talents.ice_block                  = find_talent_spell( talent_tree::CLASS, "Ice Block"                  );
  // Row 3
  talents.temporal_realignment       = find_talent_spell( talent_tree::CLASS, "Temporal Realignment"       );
  talents.time_walk                  = find_talent_spell( talent_tree::CLASS, "Time Walk"                  );
  talents.master_of_time             = find_talent_spell( talent_tree::CLASS, "Master of Time"             );
  talents.winters_protection         = find_talent_spell( talent_tree::CLASS, "Winter's Protection"        );
  talents.frost_conditioning         = find_talent_spell( talent_tree::CLASS, "Frost Conditioning"         );
  // Row 4
  talents.arcane_warding             = find_talent_spell( talent_tree::CLASS, "Arcane Warding"             );
  talents.inspired_intellect         = find_talent_spell( talent_tree::CLASS, "Inspired Intellect"         );
  talents.mirror_image               = find_talent_spell( talent_tree::CLASS, "Mirror Image"               );
  // Row 5
  talents.spellsteal                 = find_talent_spell( talent_tree::CLASS, "Spellsteal"                 );
  talents.quick_witted               = find_talent_spell( talent_tree::CLASS, "Quick Witted"               );
  talents.dragons_breath             = find_talent_spell( talent_tree::CLASS, "Dragon's Breath"            );
  talents.supernova                  = find_talent_spell( talent_tree::CLASS, "Supernova"                  );
  talents.remove_curse               = find_talent_spell( talent_tree::CLASS, "Remove Curse"               );
  talents.improved_conjuration       = find_talent_spell( talent_tree::CLASS, "Improved Conjuration"       );
  // Row 6
  talents.improved_spellsteal        = find_talent_spell( talent_tree::CLASS, "Improved Spellsteal"        );
  talents.improved_blink             = find_talent_spell( talent_tree::CLASS, "Improved Blink"             );
  talents.shimmer                    = find_talent_spell( talent_tree::CLASS, "Shimmer"                    );
  talents.improved_counterspell      = find_talent_spell( talent_tree::CLASS, "Improved Counterspell"      );
  talents.overflowing_energy         = find_talent_spell( talent_tree::CLASS, "Overflowing Energy"         );
  talents.improved_remove_curse      = find_talent_spell( talent_tree::CLASS, "Improved Remove Curse"      );
  talents.greater_invisibility       = find_talent_spell( talent_tree::CLASS, "Greater Invisibility"       );
  // Row 7
  talents.ice_ward                   = find_talent_spell( talent_tree::CLASS, "Ice Ward"                   );
  talents.improved_frost_nova        = find_talent_spell( talent_tree::CLASS, "Improved Frost Nova"        );
  talents.captured_thoughts          = find_talent_spell( talent_tree::CLASS, "Captured Thoughts"          );
  talents.tome_of_rhonin             = find_talent_spell( talent_tree::CLASS, "Tome of Rhonin"             );
  talents.tome_of_antonidas          = find_talent_spell( talent_tree::CLASS, "Tome of Antonidas"          );
  talents.incantation_of_swiftness   = find_talent_spell( talent_tree::CLASS, "Incantation of Swiftness"   );
  talents.master_of_escape           = find_talent_spell( talent_tree::CLASS, "Master of Escape"           );
  // Row 8
  talents.charm_of_aegwynn           = find_talent_spell( talent_tree::CLASS, "Charm of Aegwynn"           );
  talents.brainstorm                 = find_talent_spell( talent_tree::CLASS, "Brainstorm"                 );
  talents.flow_of_time               = find_talent_spell( talent_tree::CLASS, "Flow of Time"               );
  talents.mana_confluence            = find_talent_spell( talent_tree::CLASS, "Mana Confluence"            );
  talents.charm_of_medivh            = find_talent_spell( talent_tree::CLASS, "Charm of Medivh"            );
  // Row 9
  talents.permafrost_bauble          = find_talent_spell( talent_tree::CLASS, "Permafrost Bauble"          );
  talents.freezing_cold              = find_talent_spell( talent_tree::CLASS, "Freezing Cold"              );
  talents.ice_nova                   = find_talent_spell( talent_tree::CLASS, "Ice Nova"                   );
  talents.time_manipulation          = find_talent_spell( talent_tree::CLASS, "Time Manipulation"          );
  talents.mass_polymorph             = find_talent_spell( talent_tree::CLASS, "Mass Polymorph"             );
  talents.ring_of_frost              = find_talent_spell( talent_tree::CLASS, "Ring of Frost"              );
  talents.energized_barriers         = find_talent_spell( talent_tree::CLASS, "Energized Barriers"         );
  talents.mass_invisibility          = find_talent_spell( talent_tree::CLASS, "Mass Invisibility"          );
  talents.barrier_diffusion          = find_talent_spell( talent_tree::CLASS, "Barrier Diffusion"          );
  // Row 10
  talents.ice_cold                   = find_talent_spell( talent_tree::CLASS, "Ice Cold"                   );
  talents.reflection                 = find_talent_spell( talent_tree::CLASS, "Reflection"                 );
  talents.spatial_manipulation       = find_talent_spell( talent_tree::CLASS, "Spatial Manipulation"       );
  talents.improved_blazing_barrier   = find_talent_spell( talent_tree::CLASS, "Improved Blazing Barrier"   );
  talents.improved_ice_barrier       = find_talent_spell( talent_tree::CLASS, "Improved Ice Barrier"       );
  talents.improved_prismatic_barrier = find_talent_spell( talent_tree::CLASS, "Improved Prismatic Barrier" );

  // Arcane
  // Row 1
  talents.arcane_missiles         = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Missiles"       );
  // Row 2
  talents.concentrated_power      = find_talent_spell( talent_tree::SPECIALIZATION, "Concentrated Power"    );
  talents.arcane_salvo            = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Salvo"          );
  // Row 3
  talents.amplification           = find_talent_spell( talent_tree::SPECIALIZATION, "Amplification"         );
  talents.improved_clearcasting   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Clearcasting" );
  talents.arcing_cleave           = find_talent_spell( talent_tree::SPECIALIZATION, "Arcing Cleave"         );
  // Row 4
  talents.arcane_pulse            = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Pulse"          );
  talents.arcane_surge            = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Surge"          );
  talents.arcane_orb              = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Orb"            );
  // Row 5
  talents.reverberate             = find_talent_spell( talent_tree::SPECIALIZATION, "Reverberate"           );
  talents.presence_of_mind        = find_talent_spell( talent_tree::SPECIALIZATION, "Presence of Mind"      );
  talents.slipstream              = find_talent_spell( talent_tree::SPECIALIZATION, "Slipstream"            );
  talents.mana_bomb               = find_talent_spell( talent_tree::SPECIALIZATION, "Mana Bomb"             );
  talents.arcane_familiar         = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Familiar"       );
  talents.charged_orb             = find_talent_spell( talent_tree::SPECIALIZATION, "Charged Orb"           );
  // Row 6
  talents.arcane_tempo            = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Tempo"          );
  talents.touch_of_the_magi       = find_talent_spell( talent_tree::SPECIALIZATION, "Touch of the Magi"     );
  talents.energized_familiar      = find_talent_spell( talent_tree::SPECIALIZATION, "Energized Familiar"    );
  talents.expanded_mind           = find_talent_spell( talent_tree::SPECIALIZATION, "Expanded Mind"         );
  // Row 7
  talents.consortiums_bauble      = find_talent_spell( talent_tree::SPECIALIZATION, "Consortium's Bauble"   );
  talents.aether_attunement       = find_talent_spell( talent_tree::SPECIALIZATION, "Aether Attunement"     );
  talents.aegwynns_technique      = find_talent_spell( talent_tree::SPECIALIZATION, "Aegwynn's Technique"   );
  talents.arcane_echo             = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Echo"           );
  talents.resonance               = find_talent_spell( talent_tree::SPECIALIZATION, "Resonance"             );
  talents.impetus                 = find_talent_spell( talent_tree::SPECIALIZATION, "Impetus"               );
  // Row 8
  talents.touch_of_the_archmage_1 = find_talent_spell( talent_tree::SPECIALIZATION, 1257942                 );
  talents.evocation               = find_talent_spell( talent_tree::SPECIALIZATION, "Evocation"             );
  talents.mana_adept              = find_talent_spell( talent_tree::SPECIALIZATION, "Mana Adept"            );
  talents.enlightened             = find_talent_spell( talent_tree::SPECIALIZATION, "Enlightened"           );
  talents.illuminated_thoughts    = find_talent_spell( talent_tree::SPECIALIZATION, "Illuminated Thoughts"  );
  talents.focusing_crystal        = find_talent_spell( talent_tree::SPECIALIZATION, "Focusing Crystal"      );
  // Row 9
  talents.touch_of_the_archmage_2 = find_talent_spell( talent_tree::SPECIALIZATION, 1257947                 );
  talents.prodigious_savant       = find_talent_spell( talent_tree::SPECIALIZATION, "Prodigious Savant"     );
  talents.eureka                  = find_talent_spell( talent_tree::SPECIALIZATION, "Eureka"                );
  talents.arcane_singularity      = find_talent_spell( talent_tree::SPECIALIZATION, "Arcane Singularity"    );
  // Row 1
  talents.touch_of_the_archmage_3 = find_talent_spell( talent_tree::SPECIALIZATION, 1257950                 );
  talents.charged_missiles        = find_talent_spell( talent_tree::SPECIALIZATION, "Charged Missiles"      );
  talents.high_voltage            = find_talent_spell( talent_tree::SPECIALIZATION, "High Voltage"          );
  talents.overflowing_insight     = find_talent_spell( talent_tree::SPECIALIZATION, "Overflowing Insight"   );
  talents.overpowered_missiles    = find_talent_spell( talent_tree::SPECIALIZATION, "Overpowered Missiles"  );
  talents.orb_mastery             = find_talent_spell( talent_tree::SPECIALIZATION, "Orb Mastery"           );
  talents.orb_barrage             = find_talent_spell( talent_tree::SPECIALIZATION, "Orb Barrage"           );

  // Fire
  // Row 1
  talents.pyroblast              = find_talent_spell( talent_tree::SPECIALIZATION, "Pyroblast"              );
  // Row 2
  talents.fire_blast             = find_talent_spell( talent_tree::SPECIALIZATION, "Fire Blast"             );
  talents.flamestrike_1          = find_talent_spell( talent_tree::SPECIALIZATION, 2120                     );
  talents.flamestrike_2          = find_talent_spell( talent_tree::SPECIALIZATION, 1254851                  );
  // Row 3
  talents.ignition               = find_talent_spell( talent_tree::SPECIALIZATION, "Ignition"               );
  talents.combustion             = find_talent_spell( talent_tree::SPECIALIZATION, "Combustion"             );
  talents.fuel_the_fire          = find_talent_spell( talent_tree::SPECIALIZATION, "Fuel the Fire"          );
  // Row 4
  talents.fervent_flickering     = find_talent_spell( talent_tree::SPECIALIZATION, "Fervent Flickering"     );
  talents.cauterize              = find_talent_spell( talent_tree::SPECIALIZATION, "Cauterize"              );
  talents.meteor                 = find_talent_spell( talent_tree::SPECIALIZATION, "Meteor"                 );
  // Row 5
  talents.firestarter            = find_talent_spell( talent_tree::SPECIALIZATION, "Firestarter"            );
  talents.scorch                 = find_talent_spell( talent_tree::SPECIALIZATION, "Scorch"                 );
  talents.flame_on               = find_talent_spell( talent_tree::SPECIALIZATION, "Flame On"               );
  talents.kindling               = find_talent_spell( talent_tree::SPECIALIZATION, "Kindling"               );
  talents.critical_mass          = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Mass"          );
  talents.deep_impact            = find_talent_spell( talent_tree::SPECIALIZATION, "Deep Impact"            );
  // Row 6
  talents.scald                  = find_talent_spell( talent_tree::SPECIALIZATION, "Scald"                  );
  talents.frenetic_speed         = find_talent_spell( talent_tree::SPECIALIZATION, "Frenetic Speed"         );
  talents.inflame                = find_talent_spell( talent_tree::SPECIALIZATION, "Inflame"                );
  talents.controlled_destruction = find_talent_spell( talent_tree::SPECIALIZATION, "Controlled Destruction" );
  talents.mote_of_flame          = find_talent_spell( talent_tree::SPECIALIZATION, "Mote of Flame"          );
  talents.blast_zone             = find_talent_spell( talent_tree::SPECIALIZATION, "Blast Zone"             );
  // Row 7
  talents.conflagration          = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagration"          );
  talents.intensifying_flame     = find_talent_spell( talent_tree::SPECIALIZATION, "Intensifying Flame"     );
  talents.spontaneous_combustion = find_talent_spell( talent_tree::SPECIALIZATION, "Spontaneous Combustion" );
  talents.molten_fury            = find_talent_spell( talent_tree::SPECIALIZATION, "Molten Fury"            );
  talents.mark_of_the_firelord   = find_talent_spell( talent_tree::SPECIALIZATION, "Mark of the Firelord"   );
  talents.pyromastery            = find_talent_spell( talent_tree::SPECIALIZATION, "Pyromastery"            );
  // Row 8
  talents.fired_up_1             = find_talent_spell( talent_tree::SPECIALIZATION, 1257343                  );
  talents.wildfire               = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire"               );
  talents.fevered_incantation    = find_talent_spell( talent_tree::SPECIALIZATION, "Fevered Incantation"    );
  talents.fires_ire              = find_talent_spell( talent_tree::SPECIALIZATION, "Fire's Ire"             );
  // Row 9
  talents.fired_up_2             = find_talent_spell( talent_tree::SPECIALIZATION, 1257349                  );
  talents.master_of_flame        = find_talent_spell( talent_tree::SPECIALIZATION, "Master of Flame"        );
  talents.feel_the_burn          = find_talent_spell( talent_tree::SPECIALIZATION, "Feel the Burn"          );
  talents.fiery_rush             = find_talent_spell( talent_tree::SPECIALIZATION, "Fiery Rush"             );
  talents.flame_accelerant       = find_talent_spell( talent_tree::SPECIALIZATION, "Flame Accelerant"       );
  talents.pyromaniac             = find_talent_spell( talent_tree::SPECIALIZATION, "Pyromaniac"             );
  // Row 10
  talents.fired_up_3             = find_talent_spell( talent_tree::SPECIALIZATION, 1257348                  );
  talents.burnout                = find_talent_spell( talent_tree::SPECIALIZATION, "Burnout"                );
  talents.from_the_ashes         = find_talent_spell( talent_tree::SPECIALIZATION, "From the Ashes"         );
  talents.burn_it_all            = find_talent_spell( talent_tree::SPECIALIZATION, "Burn It All"            );
  talents.slow_burn              = find_talent_spell( talent_tree::SPECIALIZATION, "Slow Burn"              );
  talents.pyroclasm              = find_talent_spell( talent_tree::SPECIALIZATION, "Pyroclasm"              );
  talents.cinderstorm            = find_talent_spell( talent_tree::SPECIALIZATION, "Cinderstorm"            );

  // Frost
  // Row 1
  talents.ice_lance              = find_talent_spell( talent_tree::SPECIALIZATION, "Ice Lance"              );
  // Row 2
  talents.blizzard_1             = find_talent_spell( talent_tree::SPECIALIZATION, 190356                   );
  talents.blizzard_2             = find_talent_spell( talent_tree::SPECIALIZATION, 1248829                  );
  talents.fingers_of_frost       = find_talent_spell( talent_tree::SPECIALIZATION, "Fingers of Frost"       );
  // Row 3
  talents.frostbite              = find_talent_spell( talent_tree::SPECIALIZATION, "Frostbite"              );
  talents.icicles                = find_talent_spell( talent_tree::SPECIALIZATION, "Icicles"                );
  // Row 4
  talents.flurry                 = find_talent_spell( talent_tree::SPECIALIZATION, "Flurry"                 );
  talents.cold_snap              = find_talent_spell( talent_tree::SPECIALIZATION, "Cold Snap"              );
  talents.glacial_bulwark        = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Bulwark"        );
  talents.frozen_orb             = find_talent_spell( talent_tree::SPECIALIZATION, "Frozen Orb"             );
  // Row 5
  talents.brain_freeze           = find_talent_spell( talent_tree::SPECIALIZATION, "Brain Freeze"           );
  talents.icy_hand               = find_talent_spell( talent_tree::SPECIALIZATION, "Icy Hand"               );
  talents.ray_of_frost           = find_talent_spell( talent_tree::SPECIALIZATION, "Ray of Frost"           );
  talents.wintertide             = find_talent_spell( talent_tree::SPECIALIZATION, "Wintertide"             );
  talents.permafrost_lances      = find_talent_spell( talent_tree::SPECIALIZATION, "Permafrost Lances"      );
  // Row 6
  talents.frozen_touch           = find_talent_spell( talent_tree::SPECIALIZATION, "Frozen Touch"           );
  talents.splitting_ice          = find_talent_spell( talent_tree::SPECIALIZATION, "Splitting Ice"          );
  talents.improved_shatter       = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Shatter"       );
  talents.frigid_focus           = find_talent_spell( talent_tree::SPECIALIZATION, "Frigid Focus"           );
  talents.splintering_ray        = find_talent_spell( talent_tree::SPECIALIZATION, "Splintering Ray"        );
  talents.winters_blessing       = find_talent_spell( talent_tree::SPECIALIZATION, "Winter's Blessing"      );
  talents.everlasting_frost      = find_talent_spell( talent_tree::SPECIALIZATION, "Everlasting Frost"      );
  talents.freezing_rain          = find_talent_spell( talent_tree::SPECIALIZATION, "Freezing Rain"          );
  // Row 7
  talents.fractured_frost        = find_talent_spell( talent_tree::SPECIALIZATION, "Fractured Frost"        );
  talents.piercing_cold          = find_talent_spell( talent_tree::SPECIALIZATION, "Piercing Cold"          );
  talents.summon_water_elemental = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Water Elemental" );
  talents.deep_shatter           = find_talent_spell( talent_tree::SPECIALIZATION, "Deep Shatter"           );
  talents.cone_of_frost          = find_talent_spell( talent_tree::SPECIALIZATION, "Cone of Frost"          );
  // Row 8
  talents.hand_of_frost_1        = find_talent_spell( talent_tree::SPECIALIZATION, 1262935                  );
  talents.glacial_attunement     = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Attunement"     );
  talents.heart_of_ice           = find_talent_spell( talent_tree::SPECIALIZATION, "Heart of Ice"           );
  talents.rimecaster             = find_talent_spell( talent_tree::SPECIALIZATION, "Rimecaster"             );
  // Row 9
  talents.hand_of_frost_2        = find_talent_spell( talent_tree::SPECIALIZATION, 1262981                  );
  talents.glacial_assault        = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Assault"        );
  talents.freezing_winds         = find_talent_spell( talent_tree::SPECIALIZATION, "Freezing Winds"         );
  talents.improved_flurry        = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Flurry"        );
  talents.flash_freeze           = find_talent_spell( talent_tree::SPECIALIZATION, "Flash Freeze"           );
  talents.crystalline_refraction = find_talent_spell( talent_tree::SPECIALIZATION, "Crystalline Refraction" );
  talents.hailstones             = find_talent_spell( talent_tree::SPECIALIZATION, "Hailstones"             );
  // Row 10
  talents.hand_of_frost_3        = find_talent_spell( talent_tree::SPECIALIZATION, 1263249                  );
  talents.comet_storm            = find_talent_spell( talent_tree::SPECIALIZATION, "Comet Storm"            );
  talents.thermal_void           = find_talent_spell( talent_tree::SPECIALIZATION, "Thermal Void"           );
  talents.glaciate               = find_talent_spell( talent_tree::SPECIALIZATION, "Glaciate"               );

  // Frostfire
  // Row 1
  talents.frostfire_bolt        = find_talent_spell( talent_tree::HERO, 431044                  ); 
  // Row 2
  talents.imbued_warding        = find_talent_spell( talent_tree::HERO, "Imbued Warding"        );
  talents.meltdown              = find_talent_spell( talent_tree::HERO, "Meltdown"              );
  talents.frostfire_empowerment = find_talent_spell( talent_tree::HERO, "Frostfire Empowerment" );
  talents.elemental_affinity    = find_talent_spell( talent_tree::HERO, "Elemental Affinity"    );
  talents.flame_and_frost       = find_talent_spell( talent_tree::HERO, "Flame and Frost"       );
  talents.duality               = find_talent_spell( talent_tree::HERO, "Duality"               );
  // Row 3
  talents.heat_sink             = find_talent_spell( talent_tree::HERO, "Heat Sink"             );
  talents.severe_temperatures   = find_talent_spell( talent_tree::HERO, "Severe Temperatures"   );
  talents.thermal_conditioning  = find_talent_spell( talent_tree::HERO, "Thermal Conditioning"  );
  talents.dualcasting_adept     = find_talent_spell( talent_tree::HERO, "Dualcasting Adept"     );
  talents.molten_chill          = find_talent_spell( talent_tree::HERO, "Molten Chill"          );
  // Row 4
  talents.frostfire_infusion    = find_talent_spell( talent_tree::HERO, "Frostfire Infusion"    );
  talents.flash_freezeburn      = find_talent_spell( talent_tree::HERO, "Flash Freezeburn"      );
  talents.blast_radius          = find_talent_spell( talent_tree::HERO, "Blast Radius"          );
  talents.elemental_conduit     = find_talent_spell( talent_tree::HERO, "Elemental Conduit"     );
  // Row 5
  talents.isothermic_core       = find_talent_spell( talent_tree::HERO, "Isothermic Core"       );

  // Spellslinger
  // Row 1
  talents.splintering_sorcery  = find_talent_spell( talent_tree::HERO, "Splintering Sorcery"  );
  // Row 2
  talents.augury_abounds       = find_talent_spell( talent_tree::HERO, "Augury Abounds"       );
  talents.force_of_will        = find_talent_spell( talent_tree::HERO, "Force of Will"        );
  talents.splintering_orbs     = find_talent_spell( talent_tree::HERO, "Splintering Orbs"     );
  talents.attuned_familiar     = find_talent_spell( talent_tree::HERO, "Attuned Familiar"     );
  talents.shifting_shards      = find_talent_spell( talent_tree::HERO, "Shifting Shards"      );
  // Row 3
  talents.look_again           = find_talent_spell( talent_tree::HERO, "Look Again"           );
  talents.slippery_slinging    = find_talent_spell( talent_tree::HERO, "Slippery Slinging"    );
  talents.controlled_instincts = find_talent_spell( talent_tree::HERO, "Controlled Instincts" );
  talents.phantasmal_image     = find_talent_spell( talent_tree::HERO, "Phantasmal Image"     );
  talents.reactive_barrier     = find_talent_spell( talent_tree::HERO, "Reactive Barrier"     );
  talents.infused_splinters    = find_talent_spell( talent_tree::HERO, "Infused Splinters"    );
  // Row 4
  talents.archmages_wrath      = find_talent_spell( talent_tree::HERO, "Archmage's Wrath"     );
  talents.signature_spell      = find_talent_spell( talent_tree::HERO, "Signature Spell"      );
  talents.spellfrost_teachings = find_talent_spell( talent_tree::HERO, "Spellfrost Teachings" );
  talents.polished_focus       = find_talent_spell( talent_tree::HERO, "Polished Focus"       );
  // Row 5
  talents.splinterstorm        = find_talent_spell( talent_tree::HERO, "Splinterstorm"        );

  // Sunfury
  // Row 1
  talents.spellfire_spheres         = find_talent_spell( talent_tree::HERO, "Spellfire Spheres"          );
  // Row 2
  talents.mana_cascade              = find_talent_spell( talent_tree::HERO, "Mana Cascade"               );
  talents.invocation_arcane_phoenix = find_talent_spell( talent_tree::HERO, "Invocation: Arcane Phoenix" );
  talents.burden_of_power           = find_talent_spell( talent_tree::HERO, "Burden of Power"            );
  talents.glorious_incandescence    = find_talent_spell( talent_tree::HERO, "Glorious Incandescence"     );
  // Row 3
  talents.merely_a_setback          = find_talent_spell( talent_tree::HERO, "Merely a Setback"           );
  talents.time_twist                = find_talent_spell( talent_tree::HERO, "Time Twist"                 );
  talents.codex_of_the_sunstriders  = find_talent_spell( talent_tree::HERO, "Codex of the Sunstriders"   );
  talents.explosive_potential       = find_talent_spell( talent_tree::HERO, "Explosive Potential"        );
  talents.lessons_in_debilitation   = find_talent_spell( talent_tree::HERO, "Lessons in Debilitation"    );
  talents.pyrocosm                  = find_talent_spell( talent_tree::HERO, "Pyrocosm"                   );
  // Row 4
  talents.savor_the_moment          = find_talent_spell( talent_tree::HERO, "Savor the Moment"           );
  talents.sunfury_execution         = find_talent_spell( talent_tree::HERO, "Sunfury Execution"          );
  talents.ashes_of_inspiration      = find_talent_spell( talent_tree::HERO, "Ashes of Inspiration"       );
  talents.rondurmancy               = find_talent_spell( talent_tree::HERO, "Rondurmancy"                );
  talents.spellfire_salvo           = find_talent_spell( talent_tree::HERO, "Spellfire Salvo"            );
  // Row 5
  talents.memory_of_alar            = find_talent_spell( talent_tree::HERO, "Memory of Al'ar"            );


  // Spec Spells
  spec.arcane_charge                 = find_specialization_spell( "Arcane Charge"                 );
  spec.arcane_mage                   = find_specialization_spell( "Arcane Mage"                   );
  spec.clearcasting                  = find_specialization_spell( "Clearcasting"                  );
  spec.fire_mage                     = find_specialization_spell( "Fire Mage"                     );
  spec.hot_streak                    = find_specialization_spell( "Hot Streak"                    );
  spec.pyroblast_clearcasting_driver = find_specialization_spell( "Pyroblast Clearcasting Driver" );
  spec.frost_mage                    = find_specialization_spell( "Frost Mage"                    );
  spec.shatter                       = find_specialization_spell( "Shatter"                       );

  // Mastery
  spec.savant             = find_mastery_spell( MAGE_ARCANE );
  spec.ignite             = find_mastery_spell( MAGE_FIRE );
  spec.freeze_and_shatter = find_mastery_spell( MAGE_FROST );

  // Misc
  cooldowns.arcane_echo->duration = find_spell( 464515 )->internal_cooldown();

  // Register passives
  // Arcane aura mana regen includes points per level adjustment, handled manually in mage_t::resource_regen_per_second
  deregister_passive_effect( spec.arcane_mage->effectN( 5 ) );

  register_passive_effect_mask( talents.elemental_affinity,
    specialization() == MAGE_FIRE ? effect_mask_t( true ).disable( 3 ) : effect_mask_t( false ).enable( 3 ) );

  // TODO: Double check that the fire effect doesn't apply as frost
  register_passive_effect_mask( talents.frostfire_infusion,
    specialization() == MAGE_FIRE ? effect_mask_t( true ).disable( 1 ) : effect_mask_t( true ).disable( 2 ) );

  // TODO: Technically, Frost can (and does) benefit from the Pyroblast effect
  register_passive_effect_mask( talents.dualcasting_adept,
    specialization() == MAGE_FIRE ? effect_mask_t( true ).disable( 1 ) : effect_mask_t( true ).disable( 2 ) );

  // TODO: The effects aren't properly disabled in game, so both CmS and Meteor get 44% extra damage
  // register_passive_effect_mask( talents.blast_radius,
  //   specialization() == MAGE_FIRE ? effect_mask_t( true ).disable( 1, 2 ) : effect_mask_t( true ).disable( 3, 4 ) );

  // TODO: The effects aren't properly disabled in game, Fire gets extra GS damage and Frost gets extra Meteor damage
  // register_passive_effect_mask( talents.flash_freezeburn,
  //   specialization() == MAGE_FIRE ? effect_mask_t( true ).disable( 1 ) : effect_mask_t( true ).disable( 4, 5 ) );

  // TODO: Remove these when Midnight releases
  register_passive_effect_mask( sets->set( HERO_FROSTFIRE, TWW3, B2 ),
    specialization() == MAGE_FIRE ? effect_mask_t( true ).disable( 5, 6 ) : effect_mask_t( true ).disable( 3, 4 ) );

  register_passive_effect_mask( sets->set( HERO_SPELLSLINGER, TWW3, B2 ),
    specialization() == MAGE_FROST ? effect_mask_t( true ).disable( 3 ) : effect_mask_t( true ).disable( 4 ) );

  register_passive_effect_mask( sets->set( HERO_SPELLSLINGER, TWW3, B4 ),
    specialization() == MAGE_FROST ? effect_mask_t( true ).disable( 5, 6 ) : effect_mask_t( true ) );

  register_passive_effect_mask( sets->set( HERO_SUNFURY, TWW3, B2 ),
    specialization() == MAGE_ARCANE ? effect_mask_t( true ).disable( 2, 4, 8 ) : effect_mask_t( true ).disable( 1, 3, 7 ) );

  register_passive_effect_mask( sets->set( HERO_SUNFURY, TWW3, B4 ),
    specialization() == MAGE_ARCANE ? effect_mask_t( true ).disable( 2 ) : effect_mask_t( true ).disable( 1 ) );

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();

  // Wizardry
  parse_passive_effects( find_spell( 89744 ) );
  // Mana Attunement
  parse_passive_effects( find_spell( 121039 ) );
}

void mage_t::init_base_stats()
{
  if ( base.distance < 1.0 )
    base.distance = 10.0;

  base.spell_power_per_intellect = 1.0;

  if ( specialization() == MAGE_ARCANE )
    regen_caches[ CACHE_MASTERY ] = true;

  player_t::init_base_stats();
}

void mage_t::create_buffs()
{
  player_t::create_buffs();

  // Arcane
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
  buffs.arcane_salvo              = make_buff( this, "arcane_salvo", find_spell( 1242974 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_chance( talents.arcane_salvo.ok() );
  buffs.arcane_surge              = make_buff( this, "arcane_surge", find_spell( 365362 ) )
                                      ->set_default_value_from_effect( 3 )
                                      ->set_affects_regen( true );
  buffs.clearcasting              = make_buff( this, "clearcasting", find_spell( 263725 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_chance( spec.clearcasting->ok() ) ;
  buffs.enlightened               = make_buff( this, "enlightened", find_spell( 1217242 ) )
                                      ->set_schools_from_effect( 4 )
                                      ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                      ->set_affects_regen( true )
                                      ->set_freeze_stacks( true ) // We want to bump the buff manually
                                      ->set_tick_callback( [ this ] ( buff_t* b, int, timespan_t )
                                        { b->bump( 0, resources.pct( RESOURCE_MANA ) ); } )
                                      ->set_chance( talents.enlightened.ok() );
  buffs.evocation                 = make_buff( this, "evocation", find_spell( 12051 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_cooldown( 0_ms )
                                      ->set_affects_regen( true );
  buffs.overpowered_missiles      = make_buff( this, "overpowered_missiles", find_spell( 1277009 ) )
                                      ->set_default_value_from_effect( 1 )
                                      ->set_chance( talents.overpowered_missiles.ok() );
  buffs.presence_of_mind          = make_buff( this, "presence_of_mind", find_spell( 205025 ) )
                                      ->set_cooldown( 0_ms )
                                      ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                        { if ( cur == 0 ) cooldowns.presence_of_mind->start( cooldowns.presence_of_mind->action ); } );


  // Fire
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
                                       { cooldowns.fire_blast->adjust_recharge_multiplier(); } )
                                     ->set_chance( talents.fiery_rush.ok() );
  buffs.frenetic_speed           = make_buff( this, "frenetic_speed", find_spell( 236060 ) )
                                     ->set_default_value_from_effect( 1 )
                                     ->add_invalidate( CACHE_RUN_SPEED )
                                     ->set_chance( talents.frenetic_speed.ok() );
  buffs.heating_up               = make_buff( this, "heating_up", find_spell( 48107 ) );
  buffs.hot_streak               = make_buff( this, "hot_streak", find_spell( 48108 ) );
  buffs.hyperthermia             = make_buff( this, "hyperthermia", find_spell( 383874 ) )
                                     ->set_default_value_from_effect( 2 )
                                     // ->set_trigger_spell( talents.hyperthermia ) // TODO: fix for hero talents
                                     ->set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                       { if ( cur == 0 ) buffs.hyperthermia_damage->expire(); } );
  buffs.hyperthermia_damage      = make_buff( this, "hyperthermia_damage", find_spell( 1242220 ) )
                                     ->set_default_value_from_effect( 1 );
  buffs.wildfire                 = make_buff( this, "wildfire", find_spell( 383492 ) )
                                     ->set_default_value( talents.wildfire->effectN( 3 ).percent() )
                                     ->set_chance( talents.wildfire.ok() );


  // Frost
  buffs.brain_freeze       = make_buff( this, "brain_freeze", find_spell( 190446 ) );
  buffs.comet_storm        = make_buff( this, "comet_storm", find_spell( 1247778 ) )
                               ->set_chance( talents.comet_storm.ok() );
  buffs.fingers_of_frost   = make_buff( this, "fingers_of_frost", find_spell( 44544 ) );
  buffs.freezing_rain      = make_buff( this, "freezing_rain", find_spell( 270232 ) )
                               ->set_chance( talents.freezing_rain.ok() );
  buffs.glacial_spike      = make_buff( this, "glacial_spike", find_spell( 1222865 ) )
                               ->set_chance( talents.icicles.ok() );
  buffs.hand_of_frost      = make_buff( this, "hand_of_frost", find_spell( 1263263 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                               ->set_chance( talents.hand_of_frost_2.ok() );
  buffs.permafrost_lances  = make_buff( this, "permafrost_lances", find_spell( 455122 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_chance( talents.permafrost_lances.ok() );
  buffs.thermal_void       = make_buff( this, "thermal_void", find_spell( 1247730 ) )
                               ->set_chance( talents.thermal_void->effectN( 1 ).percent() );


  // Frostfire
  buffs.frostfire_empowerment = make_buff( this, "frostfire_empowerment", find_spell( 431177 ) )
                                  ->set_chance( talents.frostfire_empowerment.ok() );


  // Spellslinger
  buffs.augury_abounds = make_buff( this, "augury_abounds", find_spell( 1247908 ) )
                           ->set_chance( talents.augury_abounds.ok() );

  // Sunfury
  buffs.arcane_soul            = make_buff( this, "arcane_soul", find_spell( 451038 ) )
                                   ->set_stack_change_callback( [ this ] ( buff_t*, int, int ) { buffs.arcane_soul_damage->expire(); } )
                                   ->set_chance( specialization() == MAGE_ARCANE && talents.memory_of_alar.ok() );
  buffs.arcane_soul_damage     = make_buff( this, "arcane_soul_damage", find_spell( 1223522 ) )
                                   ->set_default_value_from_effect( 1 );
  buffs.burden_of_power        = make_buff( this, "burden_of_power", find_spell( 451049 ) )
                                   ->set_chance( talents.burden_of_power.ok() );
  buffs.glorious_incandescence = make_buff( this, "glorious_incandescence", find_spell( 451073 ) )
                                   ->set_chance( talents.glorious_incandescence.ok() );
  buffs.lingering_embers       = make_buff( this, "lingering_embers", find_spell( 461145 ) )
                                   ->set_default_value( find_spell( 448604 )->effectN( specialization() == MAGE_FIRE ? 2 : 1 ).percent() )
                                   ->set_chance( talents.codex_of_the_sunstriders.ok() );
  buffs.mana_cascade           = make_buff( this, "mana_cascade", find_spell( specialization() == MAGE_FIRE ? 449314 : 449322 ) )
                                   ->set_default_value_from_effect( 2,  0.001 )
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
                                   ->set_default_value_from_effect( specialization() == MAGE_FIRE ? 2 : 1 )
                                   ->set_chance( talents.spellfire_spheres.ok() )
                                   ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.spellfire_spheres      = make_buff( this, "spellfire_spheres", find_spell( 449400 ) )
                                   ->set_chance( talents.spellfire_spheres.ok() );


  // Shared
  buffs.brainstorm         = make_buff( this, "brainstorm", find_spell( 461531 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                               ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                               ->set_chance( talents.brainstorm.ok() );
  buffs.overflowing_energy = make_buff( this, "overflowing_energy", find_spell( 394195 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_chance( talents.overflowing_energy.ok() );


  // Buffs that use stack_react or may_react need to be reactable regardless of what the APL does
  buffs.heating_up->reactable = true;

  // Hyperthermia can be activated through Memory of Al'ar and doesn't need to be talented
  // TODO: adjust this
  if ( talents.memory_of_alar.ok() )
    buffs.hyperthermia->default_chance = -1.0;

  if ( sets->has_set_bonus( MAGE_ARCANE, MID1, B2 ) )
  {
    assert( buffs.arcane_charge->default_value == buff_t::DEFAULT_VALUE() );
    buffs.arcane_charge->set_default_value( sets->set( MAGE_ARCANE, MID1, B2 )->effectN( 1 ).percent() )
                       ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );
  }
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
    case MAGE_ARCANE:
      procs.salvo_applied  = get_proc( "Arcane Salvo applied" );
      procs.salvo_overflow = get_proc( "Arcane Salvo overflow" );
      break;
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
      procs.brain_freeze               = get_proc( "Brain Freeze" );
      procs.brain_freeze_splinterstorm = get_proc( "Brain Freeze from Splinterstorm" );
      procs.fingers_of_frost           = get_proc( "Fingers of Frost" );
      procs.freezing_applied           = get_proc( "Freezing applied" );
      procs.freezing_expired           = get_proc( "Freezing expired" );
      procs.freezing_overflow          = get_proc( "Freezing overflow" );
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
      benefits.arcane_charge.arcane_pulse = std::make_unique<buff_stack_benefit_t>( buffs.arcane_charge, "Arcane Pulse" );
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
    default:
      break;
  }
}

void mage_t::init_rng()
{
  player_t::init_rng();

  // Accumulated RNG is also not present in the game data.
  accumulated_rng.pyromaniac = get_accumulated_rng( "pyromaniac", talents.pyromaniac.ok() ? 0.00605 : 0.0 );
}

void mage_t::init_finished()
{
  player_t::init_finished();

  // Sort the procs to put the proc sources next to each other.
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

void mage_t::init_blizzard_action_list()
{
  player_t::init_blizzard_action_list();

  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      cooldowns->add_action( "evocation,if=cooldown.arcane_surge.remains<3*gcd.max&cooldown.touch_of_the_magi.remains<5*gcd.max" );
      cooldowns->add_action( "arcane_surge,if=cooldown.touch_of_the_magi.remains<action.arcane_surge.execute_time" );
      break;
    case MAGE_FIRE:
      cooldowns->add_action( "combustion" );
      break;
    case MAGE_FROST:
      cooldowns->add_action( "icy_veins" );
      break;
    default:
      break;
  }
}

double mage_t::resource_regen_per_second( resource_e rt ) const
{
  double reg = player_t::resource_regen_per_second( rt );

  if ( specialization() == MAGE_ARCANE && rt == RESOURCE_MANA )
  {
    reg *= 1.0 + 0.01 * spec.arcane_mage->effectN( 5 ).average( this );
    reg *= 1.0 + cache.mastery() * spec.savant->effectN( 4 ).mastery_value();
    reg *= 1.0 + buffs.evocation->check_value();
    if ( buffs.enlightened->check() )
      reg *= 1.0 + ( 1.0 - buffs.enlightened->check_value() ) * buffs.enlightened->data().effectN( 3 ).percent();
  }

  return reg;
}

void mage_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_MASTERY && spec.savant->ok() )
    recalculate_resource_max( RESOURCE_MANA );
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

double mage_t::composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s, school );

  if ( buffs.fevered_incantation->has_common_school( school ) )
    m *= 1.0 + buffs.fevered_incantation->check_stack_value();

  return m;
}

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.enlightened->check() && buffs.enlightened->has_common_school( school ) )
    m *= 1.0 + buffs.enlightened->check_value() * buffs.enlightened->data().effectN( 2 ).percent();

  return m;
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

double mage_t::composite_spell_crit_chance() const
{
  double c = player_t::composite_spell_crit_chance();

  // TODO: Check the passive parsing and make sure we don't apply it twice
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

void mage_t::reset()
{
  player_t::reset();

  buff_queue.clear();
  embedded_splinters.clear();
  mana_cascade_expiration.clear();
  events = events_t();
  ground_aoe_expiration = std::array<timespan_t, AOE_MAX>();
  state = state_t();
}

void mage_t::arise()
{
  player_t::arise();

  buffs.enlightened->trigger( -1, resources.pct( RESOURCE_MANA ) );

  if ( options.initial_spellfire_spheres > 0 )
    buffs.spellfire_sphere->trigger( options.initial_spellfire_spheres );

  if ( talents.icicles.ok() )
  {
    trigger_icicle( options.initial_icicles );
    events::icicle_event_t::schedule_next( this, true );
  }

  if ( talents.splinterstorm.ok() )
    events::splinterstorm_event_t::schedule_next( this, true );
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

    throw sc_invalid_apl_argument( fmt::format( "Unknown {} operation '{}'.", splits[ 0 ], splits[ 1 ] ) );
  };

  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "firestarter" ) )
    return hp_pct_expr( talents.firestarter.ok(), talents.firestarter->effectN( 1 ).base_value(), false );

  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "scorch_execute" ) )
    return hp_pct_expr( talents.scorch.ok(), talents.scorch->effectN( 2 ).base_value() + talents.sunfury_execution->effectN( 2 ).base_value(), true );

  return player_t::create_action_expression( action, name );
}

std::unique_ptr<expr_t> mage_t::create_expression( std::string_view name )
{
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

  if ( util::str_compare_ci( name, "icicles" ) )
  {
    return make_fn_expr( name, [ this ]
    { return state.icicles; } );
  }

  if ( util::str_compare_ci( name, "embedded_splinters" ) )
  {
    return make_fn_expr( name, [ this ]
    { return state.embedded_splinters; } );
  }

  if ( util::str_compare_ci( name, "remaining_splinterstorm" ) )
  {
    return make_fn_expr( name, [ this ]
    { return state.remaining_splinterstorm; } );
  }

  if ( util::str_compare_ci( name, "clearcasting_blp_remains" ) )
  {
    return make_fn_expr( name, [ this ]
    { return 13 - state.clearcasting_blp_count; } );
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
      throw sc_invalid_apl_argument( fmt::format( "Unknown ground_aoe type '{}'.", splits[ 1 ] ) );

    if ( util::str_compare_ci( splits[ 2 ], "remains" ) )
    {
      return make_fn_expr( name, [ this, type ]
      { return std::max( ground_aoe_expiration[ type ] - sim->current_time(), 0_ms ).total_seconds(); } );
    }

    throw sc_invalid_apl_argument( fmt::format( "Unknown ground_aoe operation '{}'.", splits[ 2 ] ) );
  }

  // Let action.frostbolt/fireball refer to frostfire_bolt
  if ( talents.frostfire_bolt.ok() && splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "action" ) )
  {
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

bool mage_t::trigger_crowd_control( const action_state_t* s, spell_mechanic type )
{
  if ( type == MECHANIC_INTERRUPT )
    return s->target->debuffs.casting->check();

  if ( action_t::result_is_hit( s->result )
    && ( !s->target->is_boss() || s->target->level() < sim->max_player_level + 3 ) )
  {
    return true;
  }

  return false;
}

void mage_t::trigger_freezing( player_t* target, int stacks, proc_t* source, double chance )
{
  if ( !spec.shatter->ok() || stacks <= 0 )
    return;

  if ( rng().roll( chance ) )
  {
    auto debuff = get_target_data( target )->debuffs.freezing;
    int old_stacks = debuff->check();
    debuff->trigger( stacks );
    int new_stacks = debuff->check();

    assert( source );
    for ( int i = 0; i < stacks; i++ )
    {
      source->occur();
      procs.freezing_applied->occur();
    }
    int overflow = stacks - ( new_stacks - old_stacks );
    for ( int i = 0; i < overflow; i++ )
      procs.freezing_overflow->occur();
  }
}

int mage_t::trigger_shatter( player_t* target, action_t* action, int max_consumption, shatter_source_t* source, bool fof )
{
  if ( !spec.shatter->ok() || max_consumption <= 0 )
    return 0;

  buff_t* debuff = nullptr;
  if ( auto td = find_target_data( target ) )
    debuff = td->debuffs.freezing;
  int stacks = debuff ? debuff->check() : 0;

  int shatter_stacks = fof ? max_consumption : std::min( max_consumption, stacks );
  int consume_stacks = fof ? 0 : std::min( max_consumption, stacks );

  assert( source );
  source->occur( shatter_stacks );

  // TODO: With FoF, Shatter should happen even if the target has 0 Freezing stacks, this
  // is currently not the case.
  if ( options.fof_requires_freezing && stacks == 0 )
    return 0;

  if ( shatter_stacks > 0 )
  {
    double old_mult = action->base_multiplier;
    action->base_multiplier *= shatter_stacks;
    action->execute_on_target( target );
    action->base_multiplier = old_mult;

    action_t* hof = this->action.hand_of_frost;
    double hof_chance = talents.hand_of_frost_1->effectN( 1 ).percent();
    // TODO: Seems to be based on actually consumed stacks, so Fingers of Frost doesn't increase
    // the proc chance at all
    hof_chance += consume_stacks * 0.1 * talents.hand_of_frost_2->effectN( 1 ).percent();
    if ( hof && rng().roll( hof_chance ) )
      hof->execute_on_target( target );
  }

  if ( debuff )
  {
    if ( consume_stacks > 0 )
      debuff->decrement( consume_stacks );
    if ( debuff->check() )
      debuff->refresh();
  }

  return shatter_stacks;
}

void mage_t::trigger_icicle( int count, bool grant_buff )
{
  if ( !talents.icicles.ok() || count <= 0 )
    return;

  int max_icicles = as<int>( talents.icicles->effectN( 2 ).base_value() );
  state.icicles = std::min( state.icicles + count, max_icicles );
  if ( grant_buff && state.icicles == max_icicles )
    buffs.glacial_spike->trigger();
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

void mage_t::trigger_arcane_salvo( proc_t* source, int stacks, double chance )
{
  if ( !talents.arcane_salvo->ok() || stacks <= 0 )
    return;

  if ( rng().roll( chance ) )
  {
    auto buff = buffs.arcane_salvo;
    int old_stacks = buff->check();
    buff->trigger( stacks );
    int new_stacks = buff->check();

    assert( source );
    for ( int i = 0; i < stacks; i++ )
    {
      source->occur();
      procs.salvo_applied->occur();
    }
    int overflow = stacks - ( new_stacks - old_stacks );
    for ( int i = 0; i < overflow; i++ )
      procs.salvo_overflow->occur();
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

void mage_t::trigger_spellfire_spheres()
{
  if ( !talents.spellfire_spheres.ok() )
    return;

  int max_stacks = buffs.spellfire_spheres->max_stack();

  buffs.spellfire_spheres->trigger();

  auto check_stacks = [ this, s = max_stacks ]
  {
    if ( buffs.spellfire_spheres->check() >= s )
    {
      buffs.spellfire_sphere->trigger();
      buffs.spellfire_spheres->expire();
      buffs.burden_of_power->trigger();
    }
  };

  // For Arcane, casting Arcane Blast and Arcane Barrage together results in both stacks of spellfire_spheres
  // being applied before they are consumed. This can be handled with a delay here. This does not work for Fire
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
    count = 1; // TODO: Effect 2 of Splintering Sorcery? Unclear

  double chance = talents.augury_abounds->effectN( 2 ).percent();
  for ( int i = 0; i < count; i++ )
  {
    player_t* t_ = target;
    if ( !t_ )
      t_ = rng().range( sim->target_non_sleeping_list );

    int per_conjure = ( buffs.augury_abounds->check() || buffs.arcane_surge->check() ) && rng().roll( chance ) ? 2 : 1;
    for ( int j = 0; j < per_conjure; j++ )
    {
      make_event( *sim, [ this, t = t_ ] { action.splinter->execute_on_target( t ); } );
    }
  }
}

bool mage_t::trigger_clearcasting( double chance, timespan_t delay, bool never_predictable )
{
  if ( specialization() != MAGE_ARCANE )
    return false;

  bool success = rng().roll( chance );
  if ( success )
  {
    if ( !buffs.clearcasting->check() )
    {
      state.gained_initial_clearcasting = true;
      make_event( *sim, 50_ms, [ this ] { state.gained_initial_clearcasting = false; } );
    }
    if ( delay > 0_ms && buffs.clearcasting->check() )
      make_event( *sim, delay, [ this ] { buffs.clearcasting->trigger(); } );
    else
      buffs.clearcasting->trigger();
    if ( chance >= 1.0 && !never_predictable )
      buffs.clearcasting->predict();

    // TODO: double check timing
    buffs.brainstorm->trigger();
    trigger_splinter( target, as<int>( talents.shifting_shards->effectN( 1 ).base_value() ) );

    if ( rng().roll( talents.overpowered_missiles->effectN( 1 ).percent() ) )
    {
      // If Overpowered Missiles triggers during AM channel, the buff application
      // is delayed until the channel ends (or is refreshed).
      // TODO: Should we use the delay param here?
      if ( channeling && channeling->id == 5143 )
        state.trigger_overpowered_missiles = true;
      else
        buffs.overpowered_missiles->trigger();
    }
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

    buffs.brainstorm->trigger();
    trigger_splinter( target, as<int>( talents.shifting_shards->effectN( 1 ).base_value() ) );
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

void mage_t::trigger_arcane_charge( int stacks )
{
  if ( !spec.arcane_charge->ok() || stacks <= 0 )
    return;

  buffs.arcane_charge->trigger( stacks );
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

  void html_customsection_shatter( report::sc_html_stream& os )
  {
    if ( p.shatter_source_list.empty() )
      return;

    int data_cols = 0;
    for ( const auto* source : p.shatter_source_list )
      data_cols = std::max( data_cols, source->max_stack + 1 );

    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Shatter</h3>\n"
          "<div class=\"toggle-content\">\n"
          "<table class=\"sc sort even\">\n"
          "<thead>\n"
          "<tr>\n"
          "<th></th>\n"
          "<th colspan=\"" << data_cols + 1 << "\">Count</th>\n"
          "</tr>\n"
          "<tr>\n"
          "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n";

    for ( int i = 0; i < data_cols; i++ )
      os << "<th class=\"toggle-sort\">" << i << "</th>\n";

    os << "<th class=\"toggle-sort\">Total</th>\n"
          "</tr>\n"
          "</thead>\n";

    std::vector<double> totals( data_cols, 0.0 );
    auto nonzero = [] ( double d ) { return d != 0.0 ? fmt::format( "{:.1f}", d ) : ""; };
    for ( const auto* data : p.shatter_source_list )
    {
      if ( !data->active() )
        continue;

      std::string name = data->name_str;
      if ( action_t* a = p.find_action( name ) )
        name = report_decorators::decorated_action( *a );
      else
        name = util::encode_html( name );

      os << "<tr>";
      fmt::print( os, "<td class=\"left\">{}</td>", name );
      for ( int i = 0; i < data_cols; i++ )
      {
        double value = i <= data->max_stack ? data->count( i ) : 0.0;
        fmt::print( os, "<td class=\"right\">{}</td>", nonzero( value ) );
        totals[ i ] += value;
      }
      fmt::print( os, "<td class=\"right\">{}</td>", nonzero( data->count_total() ) );
      os << "</tr>\n";
    }

    os << "<td class=\"left\">All abilities</td>";
    for ( int i = 0; i < data_cols; i++ )
      fmt::print( os, "<td class=\"right\">{}</td>", nonzero( totals[ i ] ) );
    fmt::print( os, "<td class=\"right\">{}</td>", nonzero( range::accumulate( totals, 0.0 ) ) );

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

    hotfix::register_spell( "Mage", "2025-06-28", "Manually set Arcane Orb's travel speed.", 153626 )
      .field( "prj_speed" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 30.0 )
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

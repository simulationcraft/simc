// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"
#include "simulationcraft.hpp"
#include "player/covenant.hpp"
#include "player/pet_spawner.hpp"

namespace
{  // UNNAMED NAMESPACE
// ==========================================================================
// Druid
// ==========================================================================

// Forward declarations
struct druid_t;

namespace pets
{
struct denizen_of_the_dream_t;
}

enum form_e : unsigned int
{
  CAT_FORM       = 0x1,
  NO_FORM        = 0x2,
  TRAVEL_FORM    = 0x4,
  AQUATIC_FORM   = 0x8,  // Legacy
  BEAR_FORM      = 0x10,
  DIRE_BEAR_FORM = 0x40,  // Legacy
  MOONKIN_FORM   = 0x40000000,
};

enum moon_stage_e
{
  NEW_MOON,
  HALF_MOON,
  FULL_MOON,
  MAX_MOON,
};

enum eclipse_state_e
{
  ANY_NEXT,
  IN_SOLAR,
  IN_LUNAR,
  IN_BOTH,
  MAX_STATE
};

enum free_spell_e
{
  NONE = 0,
  // free procs
  CONVOKE,     // convoke_the_spirits night_fae covenant ability
  FIRMAMENT,   // sundered firmament talent
  FLASHING,    // flashing claws talent
  GALACTIC,    // galactic guardian talent
  LYCARAS,     // lycaras fleeting glimpse legendary
  NATURAL,     // natural orders will legendary
  ORBIT,       // orbit breaker talent
  // free casts
  APEX,        // apex predators's craving
  COSMOS,      // touch the cosmos 4t29
  STARWEAVER,  // starweaver talent
};

struct druid_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* adaptive_swarm_damage;
    dot_t* astral_smolder;
    dot_t* feral_frenzy;
    dot_t* frenzied_assault;
    dot_t* fungal_growth;
    dot_t* lunar_inspiration;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* rip;
    dot_t* stellar_flare;
    dot_t* sunfire;
    dot_t* tear;
    dot_t* thrash_bear;
    dot_t* thrash_cat;
  } dots;

  struct hots_t
  {
    dot_t* cenarion_ward;
    dot_t* cultivation;
    dot_t* frenzied_regeneration;
    dot_t* germination;
    dot_t* lifebloom;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* spring_blossoms;
    dot_t* wild_growth;
  } hots;

  struct debuffs_t
  {
    buff_t* moonfire;
    buff_t* sunfire;
    buff_t* pulverize;
    buff_t* tooth_and_claw;
  } debuff;

  struct buffs_t
  {
    buff_t* ironbark;
  } buff;

  druid_td_t( player_t& target, druid_t& source );

  int dots_ticking() const;

  int hots_ticking() const;
};

struct snapshot_counter_t
{
  simple_sample_data_t execute;
  simple_sample_data_t tick;
  simple_sample_data_t waste;
  std::vector<buff_t*> buffs;
  const sim_t* sim;
  stats_t* stats;
  bool is_snapped;

  snapshot_counter_t( player_t* p, stats_t* s, buff_t* b )
    : execute(), tick(), waste(), sim( p->sim ), stats( s ), is_snapped( false )
  {
    buffs.push_back( b );
  }

  bool check_all()
  {
    double n_up = 0;

    for ( const auto& b : buffs )
      if ( b->check() )
        n_up++;

    if ( n_up == 0 )
    {
      is_snapped = false;
      return false;
    }

    waste.add( n_up - 1 );
    is_snapped = true;
    return true;
  }

  void add_buff( buff_t* b ) { buffs.push_back( b ); }

  void count_execute()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim->current_iteration == 0 && sim->iterations > sim->threads && !sim->debug && !sim->log )
      return;

    execute.add( check_all() );
  }

  void count_tick()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim->current_iteration == 0 && sim->iterations > sim->threads && !sim->debug && !sim->log )
      return;

    tick.add( is_snapped );
  }

  void merge( const snapshot_counter_t& other )
  {
    execute.merge( other.execute );
    tick.merge( other.tick );
    waste.merge( other.waste );
  }
};

struct eclipse_handler_t
{
  // final entry in data_array holds # of iterations
  using data_array = std::array<double, eclipse_state_e::MAX_STATE + 1>;
  using iter_array = std::array<unsigned, eclipse_state_e::MAX_STATE>;

  struct
  {
    std::vector<data_array> arrays;
    data_array* wrath;
    data_array* starfire;
    data_array* starsurge;
    data_array* starfall;
    data_array* fury_of_elune;
    data_array* new_moon;
    data_array* half_moon;
    data_array* full_moon;
  } data;

  struct
  {
    std::vector<iter_array> arrays;
    iter_array* wrath;
    iter_array* starfire;
    iter_array* starsurge;
    iter_array* starfall;
    iter_array* fury_of_elune;
    iter_array* new_moon;
    iter_array* half_moon;
    iter_array* full_moon;
  } iter;

  druid_t* p;
  unsigned wrath_counter;
  unsigned wrath_counter_base;
  unsigned starfire_counter;
  unsigned starfire_counter_base;
  eclipse_state_e state;
  bool enabled_;

  eclipse_handler_t( druid_t* player )
    : data(),
      iter(),
      p( player ),
      wrath_counter( 2 ),
      wrath_counter_base( 2 ),
      starfire_counter( 2 ),
      starfire_counter_base( 2 ),
      state( ANY_NEXT ),
      enabled_( false )
  {}

  bool enabled() { return enabled_; }
  void init();

  void cast_wrath();
  void cast_starfire();
  void cast_starsurge();
  void cast_ca_inc();
  void cast_moon( moon_stage_e );
  void tick_starfall();
  void tick_fury_of_elune();

  void advance_eclipse();

  void trigger_both( timespan_t );
  void extend_both( timespan_t );
  void expire_both();

  void reset_stacks();
  void reset_state();

  void datacollection_begin();
  void datacollection_end();
  void merge( const eclipse_handler_t& );
};

template <typename Data, typename Base = action_state_t>
struct druid_action_state_t : public Base, public Data
{
  static_assert( std::is_base_of_v<action_state_t, Base> );
  static_assert( std::is_default_constructible_v<Data> );  // required for initialize
  static_assert( std::is_copy_assignable_v<Data> );  // required for copy_state

  using Base::Base;

  void initialize() override
  {
    Base::initialize();
    *static_cast<Data*>( this ) = Data{};
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    Base::debug_str( s );
    if constexpr ( fmt::is_formattable<Data>::value )
      fmt::print( s, " {}", *static_cast<const Data*>( this ) );
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    Base::copy_state( o );
    *static_cast<Data*>( this ) = *static_cast<const Data*>( debug_cast<const druid_action_state_t*>( o ) );
  }
};

struct druid_t : public player_t
{
private:
  form_e form;  // Active druid form
public:
  eclipse_handler_t eclipse_handler;
  // counters for snapshot tracking
  std::vector<std::unique_ptr<snapshot_counter_t>> counters;
  double expected_max_health;  // For Bristling Fur calculations.
  std::vector<std::tuple<unsigned, unsigned, timespan_t, timespan_t, double>> prepull_swarm;

  // !!!==========================================================================!!!
  // !!! Runtime variables NOTE: these MUST be properly reset in druid_t::reset() !!!
  // !!!==========================================================================!!!
  moon_stage_e moon_stage;
  double after_the_wildfire_counter;
  std::vector<event_t*> persistent_event_delay;
  event_t* lycaras_event;
  timespan_t lycaras_event_remains;
  std::vector<event_t*> swarm_tracker;  // 'friendly' targets for healing swarm
  // !!!==========================================================================!!!

  // Options
  struct options_t
  {
    // General
    bool affinity_resources = false;  // activate resources tied to affinities
    bool no_cds = false;
    bool raid_combat = true;
    timespan_t thorns_attack_period = 2_s;
    double thorns_hit_chance = 0.75;

    // Multi-Spec
    double adaptive_swarm_jump_distance_melee = 5.0;
    double adaptive_swarm_jump_distance_ranged = 25.0;
    double adaptive_swarm_jump_distance_stddev = 1.0;
    unsigned adaptive_swarm_friendly_targets = 20;
    std::string adaptive_swarm_prepull_setup = "";
    int convoke_the_spirits_deck = 5;

    // Balance
    double initial_astral_power = 0.0;
    int initial_moon_stage = static_cast<int>( moon_stage_e::NEW_MOON );
    double initial_pulsar_value = 0.0;
    bool delay_berserking = false;

    // Feral
    double predator_rppm_rate = 0.0;
    bool owlweave_cat = true;

    // Guardian
    bool catweave_bear = false;
    bool owlweave_bear = false;

    // Restoration
    double time_spend_healing = 0.0;

    // Covenant options
    double celestial_spirits_exceptional_chance = 0.85;
    std::string kindred_affinity_covenant = "night_fae";
    double kindred_spirits_absorbed = 0.15;
    double kindred_spirits_dps_pool_multiplier = 0.85;
    double kindred_spirits_non_dps_pool_multiplier = 1.2;
    std::string kindred_spirits_target_str;
    player_t* kindred_spirits_target;
    bool lone_empowerment = false;
  } options;

  // spec-based spell/attack power overrides
  struct spec_override_t
  {
    double attack_power;
    double spell_power;
  } spec_override;

  // RPPM objects
  struct rppms_t
  {
    // Feral
    real_ppm_t* predator;    // Optional RPPM approximation
  } rppm;

  struct active_actions_t
  {
    // General
    action_t* shift_to_caster;
    action_t* shift_to_bear;
    action_t* shift_to_cat;
    action_t* shift_to_moonkin;
    action_t* lycaras_fleeting_glimpse;  // fake holder action for reporting

    // Balance
    action_t* astral_smolder;
    action_t* denizen_of_the_dream;      // placeholder action
    action_t* orbit_breaker;
    action_t* shooting_stars;
    action_t* starfall_cosmos;           // free starfall from 4t29
    action_t* starsurge_cosmos;          // free starsurge fromm 4t29
    action_t* starfall_starweaver;       // free starfall from starweaver's warp
    action_t* starsurge_starweaver;      // free starsurge from starweaver's weft
    action_t* sundered_firmament;
    action_t* syzygy;

    // Feral
    action_t* ferocious_bite_apex;       // free bite from apex predator's crazing
    action_t* frenzied_assault;

    // Guardian
    action_t* after_the_wildfire_heal;
    action_t* brambles;
    action_t* elunes_favored_heal;
    action_t* galactic_guardian;
    action_t* natures_guardian;
    action_t* rage_of_the_sleeper_reflect;
    action_t* the_natural_orders_will;   // fake holder action for reporting
    action_t* thrash_bear_flashing;

    // Restoration
    action_t* yseras_gift;

    // Covenant
    spell_t* kindred_empowerment;
    spell_t* kindred_empowerment_partner;
  } active;

  // Pets
  struct pets_t
  {
    spawner::pet_spawner_t<pets::denizen_of_the_dream_t, druid_t> denizen_of_the_dream;
    std::array<pet_t*, 3> force_of_nature;

    pets_t( druid_t* p ) : denizen_of_the_dream( "denizen_of_the_dream", p ) {}
  } pets;

  // Auto-attacks
  weapon_t caster_form_weapon;
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* caster_melee_attack;
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* barkskin;
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* prowl;
    buff_t* thorns;
    buff_t* oath_of_the_elder_druid;
    buff_t* lycaras_fleeting_glimpse;  // lycara's '5s warning' buff

    // Class
    buff_t* dash;
    buff_t* heart_of_the_wild;
    buff_t* innervate;
    buff_t* ironfur;
    buff_t* lycaras_teachings_haste;  // no form
    buff_t* lycaras_teachings_crit;   // cat form
    buff_t* lycaras_teachings_vers;   // bear form
    buff_t* lycaras_teachings_mast;   // moonkin form
    buff_t* lycaras_teachings;        // placeholder buff
    buff_t* matted_fur;
    buff_t* moonkin_form;
    buff_t* natures_vigil;
    buff_t* protector_of_the_pack_moonfire;
    buff_t* protector_of_the_pack_regrowth;
    buff_t* tiger_dash;
    buff_t* tireless_pursuit;
    buff_t* ursine_vigor;
    buff_t* wild_charge_movement;

    // Multi-Spec
    buff_t* survival_instincts;

    // Balance
    buff_t* balance_of_all_things_arcane;
    buff_t* balance_of_all_things_nature;
    buff_t* celestial_alignment;
    buff_t* eclipse_lunar;
    buff_t* eclipse_solar;
    buff_t* friend_of_the_fae;
    buff_t* fury_of_elune;  // AP ticks
    buff_t* gathering_starstuff;  // 2t29
    buff_t* incarnation_moonkin;
    buff_t* natures_balance;
    buff_t* natures_grace;
    buff_t* orbit_breaker;
    buff_t* owlkin_frenzy;
    buff_t* parting_skies;  // sundered firmament tracker
    buff_t* starweavers_warp;  // free starfall
    buff_t* starweavers_weft;  // free starsurge
    buff_t* primordial_arcanic_pulsar;
    buff_t* solstice;
    buff_t* starfall;
    buff_t* starlord;  // talent
    buff_t* sundered_firmament;  // AP ticks
    buff_t* touch_the_cosmos;  // 4t29
    buff_t* touch_the_cosmos_starfall;  // hidden buff to track buffed starfall
    buff_t* rattled_stars;
    buff_t* umbral_embrace;
    buff_t* warrior_of_elune;

    // Feral
    buff_t* apex_predators_craving;
    buff_t* berserk_cat;
    buff_t* bloodtalons;
    buff_t* bt_rake;          // dummy buff
    buff_t* bt_shred;         // dummy buff
    buff_t* bt_swipe;         // dummy buff
    buff_t* bt_thrash;        // dummy buff
    buff_t* bt_moonfire;      // dummy buff
    buff_t* bt_brutal_slash;  // dummy buff
    buff_t* clearcasting_cat;
    buff_t* frantic_momentum;
    buff_t* incarnation_cat;
    buff_t* incarnation_cat_prowl;
    buff_t* predatory_swiftness;
    buff_t* protective_growth;
    buff_t* sabertooth;
    buff_t* sharpened_claws_bloodied_fangs;  // 4t29
    buff_t* sudden_ambush;
    buff_t* tigers_fury;
    buff_t* tigers_tenacity;

    // Guardian
    buff_t* berserk_bear;
    buff_t* bristling_fur;
    buff_t* dream_of_cenarius;
    buff_t* earthwarden;
    buff_t* galactic_guardian;
    buff_t* gore;
    buff_t* gory_fur;
    buff_t* guardian_of_elune;
    buff_t* incarnation_bear;
    buff_t* overpowering_aura;  // 2t29
    buff_t* rage_of_the_sleeper;
    buff_t* tooth_and_claw;
    buff_t* ursocs_fury;
    buff_t* vicious_cycle_mangle;
    buff_t* vicious_cycle_maul;
    // Guardian Conduits
    buff_t* savage_combatant;

    // Restoration
    buff_t* abundance;
    buff_t* cenarion_ward;
    buff_t* clearcasting_tree;
    buff_t* critical_growth;  // 4t29
    buff_t* flourish;
    buff_t* incarnation_tree;
    buff_t* natures_swiftness;
    buff_t* soul_of_the_forest_tree;
    buff_t* yseras_gift;

    // Covenants
    buff_t* convoke_the_spirits;  // dummy buff for conduit
    buff_t* kindred_empowerment;
    buff_t* kindred_empowerment_partner;
    buff_t* kindred_empowerment_energize;
    buff_t* kindred_affinity;
    buff_t* lone_empowerment;
    buff_t* ravenous_frenzy;
    buff_t* sinful_hysteria;

    // Helper pointers
    buff_t* clearcasting;  // clearcasting_cat or clearcasting_tree
    buff_t* b_inc_cat;     // berserk_cat or incarnation_cat
    buff_t* b_inc_bear;    // berserk_bear or incarnation_bear
    buff_t* ca_inc;        // celestial_alignment or incarnation_moonkin
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk_bear;
    cooldown_t* berserk_cat;
    cooldown_t* incarnation;
    cooldown_t* mangle;
    cooldown_t* moon_cd;  // New / Half / Full Moon
    cooldown_t* natures_swiftness;
    cooldown_t* thrash_bear;
    cooldown_t* tigers_fury;
    cooldown_t* warrior_of_elune;
    cooldown_t* rage_from_melees;
  } cooldown;

  // Gains
  struct gains_t
  {
    // Multiple Specs / Forms
    gain_t* clearcasting;        // Feral & Restoration
    gain_t* soul_of_the_forest;  // Feral & Guardian

    // Balance
    gain_t* natures_balance;
    gain_t* stellar_innervation;

    // Feral (Cat)
    gain_t* berserk;
    gain_t* cats_curiosity;
    gain_t* energy_refund;
    gain_t* tigers_tenacity;
    gain_t* incessant_hunter;
    gain_t* primal_claws;
    gain_t* primal_fury;

    // Guardian (Bear)
    gain_t* bear_form;
    gain_t* blood_frenzy;
    gain_t* brambles;
    gain_t* bristling_fur;
    gain_t* gore;
    gain_t* rage_from_melees;
  } gain;

  // Masteries
  struct masteries_t
  {
    const spell_data_t* harmony;
    const spell_data_t* natures_guardian;
    const spell_data_t* natures_guardian_AP;
    const spell_data_t* razor_claws;
    const spell_data_t* astral_invocation;
  } mastery;

  // Procs
  struct procs_t
  {
    // Balance
    proc_t* pulsar;

    // Feral
    proc_t* clearcasting;
    proc_t* clearcasting_wasted;
    proc_t* predator;
    proc_t* predator_wasted;
    proc_t* primal_claws;
    proc_t* primal_fury;

    // Guardian
    proc_t* gore;
  } proc;

  // Talents
  struct talents_t
  {
    // Class tree
    player_talent_t rake;  // Row 1
    player_talent_t frenzied_regeneration;
    player_talent_t rejuvenation;
    player_talent_t starfire;

    player_talent_t thrash;  // Row 2
    player_talent_t improved_barkskin;
    player_talent_t swiftmend;
    player_talent_t starsurge;

    player_talent_t rip;  // Row 3
    player_talent_t swipe;
    player_talent_t verdant_heart;
    player_talent_t remove_corruption;
    player_talent_t moonkin_form;

    player_talent_t maim;  // Row 4
    player_talent_t killer_instinct;
    player_talent_t ironfur;
    player_talent_t nurturing_instinct;
    player_talent_t hibernate;

    player_talent_t feline_swiftness;  // Row 5
    player_talent_t skull_bash;
    player_talent_t thick_hide;
    player_talent_t tiger_dash;
    player_talent_t wild_charge;
    player_talent_t natural_recovery;
    player_talent_t cyclone;
    player_talent_t astral_influence;

    player_talent_t tireless_pursuit;  // Row 6
    player_talent_t soothe;
    player_talent_t sunfire;
    player_talent_t typhoon;

    player_talent_t primal_fury;  // Row 7
    player_talent_t matted_fur;
    player_talent_t stampeding_roar;
    player_talent_t wild_growth;
    player_talent_t improved_sunfire;

    player_talent_t mighty_bash;  // Row 8
    player_talent_t incapacitating_roar;
    player_talent_t ursine_vigor;
    player_talent_t lycaras_teachings;
    player_talent_t improved_rejuvenation;
    player_talent_t ursols_vortex;
    player_talent_t mass_entanglement;

    player_talent_t wellhoned_instincts;  // Row 9
    player_talent_t improved_stampeding_roar;
    player_talent_t renewal;
    player_talent_t innervate;

    player_talent_t protector_of_the_pack;  // Row 10
    player_talent_t heart_of_the_wild;
    player_talent_t natures_vigil;

    // Multi-spec
    player_talent_t adaptive_swarm;
    player_talent_t unbridled_swarm;
    player_talent_t circle_of_life_and_death;
    player_talent_t convoke_the_spirits;
    player_talent_t survival_instincts;

    // Balance
    player_talent_t eclipse;  // Row 1

    player_talent_t shooting_stars;  // Row 2
    player_talent_t warrior_of_elune;
    player_talent_t starfall;

    player_talent_t solstice;  // Row 3
    player_talent_t umbral_intensity;
    player_talent_t umbral_embrace;
    player_talent_t aetherial_kindling;
    player_talent_t lunar_shrapnel;

    player_talent_t stellar_flare;  // Row 4
    player_talent_t twin_moons;
    player_talent_t solar_beam;
    player_talent_t force_of_nature;

    player_talent_t orbit_breaker;  // Row 5
    player_talent_t light_of_the_sun;
    player_talent_t natures_balance;
    player_talent_t rattle_the_stars;
    player_talent_t starweaver;

    player_talent_t waning_twilight;  // Row 6
    player_talent_t celestial_alignment;
    player_talent_t starlord;

    player_talent_t astral_communion;  // Row 7
    player_talent_t natures_grace;
    player_talent_t syzygy;
    player_talent_t primordial_arcanic_pulsar;
    player_talent_t soul_of_the_forest_moonkin;
    player_talent_t wild_mushroom;

    // player_talent_t circle_of_life_and_death_moonkin;  // Row 8
    player_talent_t incarnation_moonkin;
    // player_talent_t convoke_the_spirits_moonkin;
    player_talent_t power_of_goldrinn;
    player_talent_t fungal_growth;

    player_talent_t denizen_of_the_dream;  // Row 9
    player_talent_t astral_smolder;
    player_talent_t elunes_guidance;
    player_talent_t balance_of_all_things;
    player_talent_t fury_of_elune;
    player_talent_t new_moon;

    player_talent_t friend_of_the_fae;  // Row 10
    player_talent_t sundered_firmament;
    player_talent_t stellar_innervation;
    player_talent_t radiant_moonlight;

    // Feral
    player_talent_t tigers_fury;  // Row 1

    player_talent_t omen_of_clarity_cat;  // Row 2
    player_talent_t primal_claws;
    player_talent_t primal_wrath;

    player_talent_t merciless_strikes;  // Row 3
    player_talent_t predator;
    player_talent_t doubleclawed_rake;

    player_talent_t protective_growth;  // Row 4
    player_talent_t sabertooth;
    player_talent_t tireless_energy;
    player_talent_t pouncing_strikes;
    player_talent_t lunar_inspiration;

    player_talent_t sudden_ambush;  // Row 5
    player_talent_t rampant_ferocity;
    player_talent_t berserk;
    player_talent_t tear_open_wounds;
    player_talent_t taste_for_blood;

    player_talent_t infected_wounds_cat;  // Row 6
    player_talent_t predatory_swiftness;
    player_talent_t moment_of_clarity;
    player_talent_t dreadful_bleeding;
    player_talent_t relentless_predator;

    player_talent_t raging_fury;  // Row 7
    player_talent_t tigers_tenacity;
    player_talent_t berserk_heart_of_the_lion;
    // player_talent_t survival_instincts_cat;
    player_talent_t berserk_frenzy;
    player_talent_t brutal_slash;
    player_talent_t wild_slashes;

    player_talent_t carnivorous_instinct;  // Row 8
    player_talent_t frantic_momentum;
    player_talent_t cats_curiosity;

    player_talent_t lions_strength;  // Row 9
    player_talent_t bloodtalons;
    // player_talent_t adaptive_swarm_cat;
    player_talent_t incarnation_cat;
    // player_talent_t convoke_the_spirits_cat;
    player_talent_t soul_of_the_forest_cat;
    player_talent_t veinripper;
    player_talent_t rip_and_tear;

    player_talent_t feral_frenzy;  // row 10
    // player_talent_t unbridled_swarm_cat;
    player_talent_t ashamanes_guidance; 
    // player_talent_t circle_of_life_and_death_cat;
    player_talent_t apex_predators_craving;

    // Guardian
    player_talent_t maul;  // Row 1

    player_talent_t gore;  // Row 2
    // player_talent_t survival_instincts_bear;

    player_talent_t bristling_fur;  // Row 3
    player_talent_t brambles;
    player_talent_t ursine_adept;
    player_talent_t improved_survival_instincts;
    player_talent_t mangle;

    player_talent_t innate_resolve;  // Row 4
    player_talent_t infected_wounds_bear;
    player_talent_t berserk_ravage;
    player_talent_t ursocs_endurance;
    player_talent_t gory_fur;

    player_talent_t flashing_claws;  // Row 5
    player_talent_t tooth_and_claw;
    player_talent_t layered_mane;
    player_talent_t scintillating_moonlight;

    player_talent_t earthwarden;  // Row 6
    player_talent_t vicious_cycle;
    player_talent_t vulnerable_flesh;
    player_talent_t front_of_the_pack;
    player_talent_t reinforced_fur;
    player_talent_t galactic_guardian;
    player_talent_t twin_moonfire;

    player_talent_t berserk_unchecked_aggression;  // Row 7
    player_talent_t fury_of_nature;
    player_talent_t berserk_persistence;

    player_talent_t reinvigoration;  // Row 8
    // player_talent_t circle_of_life_and_death_bear;
    player_talent_t elunes_favored;
    player_talent_t survival_of_the_fittest;

    player_talent_t ursocs_fury;  // Row 9
    player_talent_t dream_of_cenarius;
    player_talent_t pulverize;
    player_talent_t incarnation_bear;
    // player_talent_t convoke_the_spirits_bear;
    player_talent_t blood_frenzy;
    player_talent_t soul_of_the_forest_bear;
    player_talent_t after_the_wildfire;
    player_talent_t guardian_of_elune;

    player_talent_t rend_and_tear;  // Row 10
    player_talent_t untamed_savagery;
    player_talent_t ursocs_guidance;
    player_talent_t rage_of_the_sleeper;

    // Restoration
    player_talent_t lifebloom;  // Row 1

    player_talent_t yseras_gift;  // Row 2
    player_talent_t natures_swiftness;
    player_talent_t omen_of_clarity_tree;

    player_talent_t grove_tending;  // Row 3
    player_talent_t natures_splendor;
    player_talent_t passing_seasons;
    player_talent_t flash_of_clarity;

    player_talent_t waking_dream;  // Row 4
    player_talent_t improved_regrowth;
    player_talent_t abundance;
    player_talent_t cenarion_ward;
    player_talent_t nourish;

    player_talent_t efflorescence;  // Row 5
    player_talent_t tranquility;
    player_talent_t ironbark;

    player_talent_t soul_of_the_forest_tree;  // Row 6
    player_talent_t cultivation;
    player_talent_t inner_peace;
    player_talent_t dreamstate;
    player_talent_t improved_wild_growth;
    player_talent_t stonebark;
    player_talent_t improved_ironbark;

    player_talent_t verdancy;  // Row 7
    player_talent_t rampant_growth;
    player_talent_t regenesis;
    player_talent_t harmonious_blooming;
    player_talent_t unstoppable_growth;
    player_talent_t regenerative_heartwood;

    player_talent_t spring_blossoms;  // Row 8
    player_talent_t overgrowth;
    player_talent_t incarnation_tree;
    // player_talent_t convoke_the_spirits_tree;
    // player_talent_t adaptive_swarm_tree;
    player_talent_t verdant_infusion;
    player_talent_t flourish;

    // player_talent_t circle_of_life_and_death_tree;  // Row 9
    player_talent_t budding_leaves;
    player_talent_t cenarius_guidance;
    player_talent_t embrace_of_the_dream;
    // player_talent_t unbridled_swarm_tree;
    player_talent_t luxuriant_soil;
    player_talent_t natural_wisdom;
    player_talent_t nurturing_dormancy;

    player_talent_t photosynthesis;  // Row 10
    player_talent_t undergrowth;
    player_talent_t germination;
    player_talent_t reforestation;
    player_talent_t power_of_the_archdruid;
    player_talent_t invigorate;
  } talent;

  // Class Specializations
  struct specializations_t
  {
    // Passive Auras
    const spell_data_t* druid;
    const spell_data_t* critical_strikes;  // Feral & Guardian
    const spell_data_t* leather_specialization;

    // Baseline
    const spell_data_t* bear_form_override;  // swipe/thrash
    const spell_data_t* bear_form_passive;
    const spell_data_t* cat_form_override;  // swipe/thrash
    const spell_data_t* cat_form_passive;
    const spell_data_t* cat_form_speed;
    const spell_data_t* feral_affinity;
    const spell_data_t* improved_prowl;
    const spell_data_t* moonfire_2;
    const spell_data_t* moonfire_dmg;
    const spell_data_t* wrath;

    // Class
    const spell_data_t* sunfire_dmg;
    const spell_data_t* thrash_bear_dot;

    // Multi-Spec
    const spell_data_t* adaptive_swarm_damage;
    const spell_data_t* adaptive_swarm_heal;

    // Balance
    const spell_data_t* balance;
    const spell_data_t* astral_power;
    const spell_data_t* celestial_alignment;
    const spell_data_t* eclipse_lunar;
    const spell_data_t* eclipse_solar;
    const spell_data_t* full_moon;
    const spell_data_t* half_moon;
    const spell_data_t* incarnation_moonkin;
    const spell_data_t* shooting_stars_dmg;

    // Feral
    const spell_data_t* feral;
    const spell_data_t* feral_overrides;
    const spell_data_t* ashamanes_guidance;
    const spell_data_t* berserk_cat;  // berserk cast/buff spell

    // Guardian
    const spell_data_t* guardian;
    const spell_data_t* bear_form_2;
    const spell_data_t* berserk_bear;  // berserk cast/buff spell
    const spell_data_t* galactic_guardian_buff;
    const spell_data_t* incarnation_bear;
    const spell_data_t* lightning_reflexes;

    // Resto
    const spell_data_t* restoration;
  } spec;

  struct uptimes_t
  {
    uptime_t* combined_ca_inc;
    uptime_t* eclipse_solar;
    uptime_t* eclipse_lunar;
    uptime_t* eclipse_none;
    uptime_t* friend_of_the_fae;
    uptime_t* primordial_arcanic_pulsar;
  } uptime;

  // Covenant
  struct covenant_t
  {
    // Kyrian
    const spell_data_t* kyrian;        // kindred spirits
    const spell_data_t* empower_bond;  // the actual action
    const spell_data_t* kindred_empowerment;
    const spell_data_t* kindred_empowerment_energize;
    const spell_data_t* kindred_empowerment_damage;

    // Night Fae
    const spell_data_t* night_fae;  // convoke the spirits

    // Venthyr
    const spell_data_t* venthyr;  // ravenous frenzy

    // Necrolord
    const spell_data_t* necrolord;  // adaptive swarm
  } cov;

  // Conduits
  struct conduit_t
  {
    conduit_data_t stellar_inspiration;
    conduit_data_t precise_alignment;
    conduit_data_t fury_of_the_skies;
    conduit_data_t umbral_intensity;

    conduit_data_t taste_for_blood;
    conduit_data_t incessant_hunter;
    conduit_data_t sudden_ambush;
    conduit_data_t carnivorous_instinct;

    conduit_data_t unchecked_aggression;
    conduit_data_t savage_combatant;
    conduit_data_t layered_mane;

    conduit_data_t deep_allegiance;
    conduit_data_t evolved_swarm;
    conduit_data_t conflux_of_elements;
    conduit_data_t endless_thirst;

    conduit_data_t tough_as_bark;
    conduit_data_t ursine_vigor;
    conduit_data_t innate_resolve;

    conduit_data_t front_of_the_pack;
    conduit_data_t born_of_the_wilds;
  } conduit;

  struct legendary_t
  {
    // General
    item_runeforge_t oath_of_the_elder_druid;   // 7084
    item_runeforge_t circle_of_life_and_death;  // 7085
    item_runeforge_t draught_of_deep_focus;     // 7086
    item_runeforge_t lycaras_fleeting_glimpse;  // 7110

    // Covenant
    item_runeforge_t kindred_affinity;   // 7477
    item_runeforge_t unbridled_swarm;    // 7472
    item_runeforge_t celestial_spirits;  // 7571
    item_runeforge_t sinful_hysteria;    // 7474

    // Balance
    item_runeforge_t oneths_clear_vision;        // 7087
    item_runeforge_t primordial_arcanic_pulsar;  // 7088
    item_runeforge_t balance_of_all_things;      // 7107
    item_runeforge_t timeworn_dreambinder;       // 7108

    // Feral
    item_runeforge_t cateye_curio;             // 7089
    item_runeforge_t eye_of_fearful_symmetry;  // 7090
    item_runeforge_t apex_predators_craving;   // 7091
    item_runeforge_t frenzyband;               // 7109

    // Guardian
    item_runeforge_t luffainfused_embrace;     // 7092
    item_runeforge_t the_natural_orders_will;  // 7093
    item_runeforge_t ursocs_fury_remembered;   // 7094
    item_runeforge_t legacy_of_the_sleeper;    // 7095
  } legendary;

  druid_t( sim_t* sim, std::string_view name, race_e r = RACE_NIGHT_ELF )
    : player_t( sim, DRUID, name, r ),
      form( form_e::NO_FORM ),
      eclipse_handler( this ),
      options(),
      spec_override( spec_override_t() ),
      active( active_actions_t() ),
      pets( this ),
      caster_form_weapon(),
      caster_melee_attack( nullptr ),
      cat_melee_attack( nullptr ),
      bear_melee_attack( nullptr ),
      buff( buffs_t() ),
      cooldown( cooldowns_t() ),
      gain( gains_t() ),
      mastery( masteries_t() ),
      proc( procs_t() ),
      talent( talents_t() ),
      spec( specializations_t() ),
      uptime( uptimes_t() ),
      cov( covenant_t() ),
      conduit( conduit_t() ),
      legendary( legendary_t() )
  {
    cooldown.berserk_bear       = get_cooldown( "berserk_bear" );
    cooldown.berserk_cat        = get_cooldown( "berserk_cat" );
    cooldown.incarnation        = get_cooldown( "incarnation" );
    cooldown.mangle             = get_cooldown( "mangle" );
    cooldown.moon_cd            = get_cooldown( "moon_cd" );
    cooldown.natures_swiftness  = get_cooldown( "natures_swiftness" );
    cooldown.thrash_bear        = get_cooldown( "thrash_bear" );
    cooldown.tigers_fury        = get_cooldown( "tigers_fury" );
    cooldown.warrior_of_elune   = get_cooldown( "warrior_of_elune" );
    cooldown.rage_from_melees   = get_cooldown( "rage_from_melees" );
    cooldown.rage_from_melees->duration = 1_s;

    resource_regeneration = regen_type::DYNAMIC;

    regen_caches[ CACHE_HASTE ]        = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  void activate() override;
  void init() override;
  bool validate_fight_style( fight_style_e ) const override;
  void init_absorb_priority() override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_gains() override;
  void init_procs() override;
  void init_uptimes() override;
  void init_resources( bool ) override;
  void init_rng() override;
  void init_special_effects() override;
  void init_spells() override;
  void init_scaling() override;
  void init_assessors() override;
  void init_finished() override;
  void create_buffs() override;
  void create_actions() override;
  std::string default_flask() const override;
  std::string default_potion() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;
  void invalidate_cache( cache_e ) override;
  void arise() override;
  void reset() override;
  void combat_begin() override;
  void merge( player_t& other ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void analyze( sim_t& ) override;
  timespan_t available() const override;
  double composite_melee_attack_power() const override;
  double composite_melee_attack_power_by_type( attack_power_type type ) const override;
  double composite_attack_power_multiplier() const override;
  double composite_armor() const override;
  double composite_armor_multiplier() const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double composite_block() const override { return 0; }
  double composite_crit_avoidance() const override;
  double composite_dodge_rating() const override;
  double composite_leech() const override;
  double composite_parry() const override { return 0; }
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_melee_expertise( const weapon_t* ) const override;
  double temporary_movement_modifier() const override;
  double passive_movement_modifier() const override;
  double composite_spell_power( school_e school ) const override;
  double composite_spell_power_multiplier() const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  std::unique_ptr<expr_t> create_action_expression(action_t& a, std::string_view name_str) override;
  std::unique_ptr<expr_t> create_expression( std::string_view name ) override;
  action_t* create_action( std::string_view name, std::string_view options ) override;
  pet_t* create_pet( std::string_view name, std::string_view type ) override;
  void create_pets() override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double resource_regen_per_second( resource_e ) const override;
  //double resource_gain( resource_e, double, gain_t*, action_t* a = nullptr ) override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void assess_damage_imminent_pre_absorb( school_e, result_amount_type, action_state_t* ) override;
  void assess_heal( school_e, result_amount_type, action_state_t* ) override;
  void recalculate_resource_max( resource_e, gain_t* source = nullptr ) override;
  void create_options() override;
  std::string create_profile( save_e type ) override;
  const druid_td_t* find_target_data( const player_t* target ) const override;
  druid_td_t* get_target_data( player_t* target ) const override;
  void copy_from( player_t* ) override;
  form_e get_form() const { return form; }
  void shapeshift( form_e );
  void init_beast_weapon( weapon_t&, double );
  void moving() override;
  void trigger_natures_guardian( const action_state_t* );
  double calculate_expected_max_health() const;
  const spelleffect_data_t* query_aura_effect( const spell_data_t* aura_spell,
                                               effect_subtype_t subtype           = A_ADD_PCT_MODIFIER,
                                               int misc_value                     = P_GENERIC,
                                               const spell_data_t* affected_spell = spell_data_t::nil(),
                                               effect_type_t type                 = E_APPLY_AURA ) const;
  const spell_data_t* apply_override( const spell_data_t* base, const spell_data_t* passive );
  void apply_affecting_auras( action_t& ) override;
  bool check_astral_power( action_t* a, int over );

  // secondary actions
  std::vector<action_t*> secondary_action_list;

  template <typename T, typename... Ts>
  T* get_secondary_action( std::string_view n, Ts&&... args )
  {
    auto it = range::find( secondary_action_list, n, &action_t::name_str );
    if ( it != secondary_action_list.cend() )
      return dynamic_cast<T*>( *it );

    auto a = new T( this, std::forward<Ts>( args )... );
    secondary_action_list.push_back( a );
    return a;
  }

  // pass along the name as first argument to new action construction
  template <typename T, typename... Ts>
  T* get_secondary_action_n( std::string_view n, Ts&&... args )
  {
    auto it = range::find( secondary_action_list, n, &action_t::name_str );
    if ( it != secondary_action_list.cend() )
      return dynamic_cast<T*>( *it );

    auto a = new T( this, n, std::forward<Ts>( args )... );
    secondary_action_list.push_back( a );
    return a;
  }

  template <typename T>
  bool match_dot_id( action_t* a, std::vector<int>& ids, std::vector<T*>& actions )
  {
    auto matched = dynamic_cast<T*>( a );
    if ( matched )
    {
      actions.push_back( matched );
      if ( !range::contains( ids, matched->internal_id ) )
        ids.push_back( matched->internal_id );
      return true;
    }
    return false;
  }

  template <typename T, typename U = T>
  void setup_dot_ids()
  {
    std::vector<int> ids;
    std::tuple<std::vector<T*>, std::vector<U*>> actions;
    for ( const auto& a : action_list )
    {
      if ( match_dot_id( a, ids, std::get<0>( actions ) ) )
        continue;
      if ( !std::is_same_v<T, U> )
        match_dot_id( a, ids, std::get<1>( actions ) );
    }
    if ( !ids.empty() )
    {
      for ( const auto& a : std::get<0>( actions ) )
        a->dot_ids = ids;
      if ( !std::is_same_v<T, U> )
        for ( const auto& a : std::get<1>( actions ) )
          a->dot_ids = ids;
    }
  }

private:
  void apl_precombat();
  void apl_default();
  void apl_feral();
  void apl_balance();
  void apl_guardian();
  void apl_restoration();

  target_specific_t<druid_td_t> target_data;
};

namespace pets
{
// ==========================================================================
// Pets and Guardians
// ==========================================================================

// Denizen of the Dream =============================================
struct denizen_of_the_dream_t : public pet_t
{
  struct fey_missile_t : public spell_t
  {
    fey_missile_t( pet_t* p ) : spell_t( "fey_missile", p, p->find_spell( 188046 ) )
    {
      // From ExpectedStat.db2
      double base_dam = 127552;  // @70

      switch( p->owner->true_level )
      {
        case 70: break;
        case 69: base_dam = 122190; break;
        case 68: base_dam = 114924; break;
        case 67: base_dam = 104111; break;
        case 66: base_dam = 89147;  break;
        case 65: base_dam = 76320;  break;
        case 64: base_dam = 65318;  break;
        case 63: base_dam = 55870;  break;
        case 62: base_dam = 47749;  break;
        case 61: base_dam = 40760;  break;
        default: base_dam = 23406;  break;
      }

      base_dd_min = base_dd_max = base_dam / 100;

      name_str_reporting = "fey_missile";

      auto proxy = debug_cast<druid_t*>( p->owner )->active.denizen_of_the_dream;
      auto it = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }

    void execute() override
    {
      // TODO: seems to have a random cooldown, narrow this further and see if hasted
      cooldown->duration = rng().range( 400_ms, 600_ms );

      spell_t::execute();
    }
  };

  denizen_of_the_dream_t( druid_t* p ) : pet_t( p->sim, p, "denizen_of_the_dream", true, true )
  {
    owner_coeff.sp_from_sp = 0;  // TODO: bugged ATM

    action_list_str = "fey_missile";
  }

  action_t* create_action( std::string_view n, std::string_view opt ) override
  {
    if ( n == "fey_missile" ) return new fey_missile_t( this );

    return pet_t::create_action( n, opt );
  }

  void arise() override
  {
    pet_t::arise();

    if ( o()->talent.friend_of_the_fae.ok() )
      o()->buff.friend_of_the_fae->trigger();
  }

  druid_t* o() { return static_cast<druid_t*>( owner ); }
};

// Force of Nature ==================================================
struct force_of_nature_t : public pet_t
{
  struct fon_melee_t : public melee_attack_t
  {
    bool first_attack;
    fon_melee_t( pet_t* p, const char* name = "Melee" )
      : melee_attack_t( name, p, spell_data_t::nil() ), first_attack( true )
    {
      school            = SCHOOL_PHYSICAL;
      weapon            = &( p->main_hand_weapon );
      weapon_multiplier = 1.0;
      base_execute_time = weapon->swing_time;
      may_crit = background = repeating = true;
    }

    timespan_t execute_time() const override
    {
      if ( first_attack )
      {
        return timespan_t::zero();
      }
      else
      {
        return melee_attack_t::execute_time();
      }
    }

    void cancel() override
    {
      melee_attack_t::cancel();
      first_attack = true;
    }

    void schedule_execute( action_state_t* s ) override
    {
      melee_attack_t::schedule_execute( s );
      first_attack = false;
    }
  };

  struct auto_attack_t : public melee_attack_t
  {
    auto_attack_t( pet_t* player ) : melee_attack_t( "auto_attack", player )
    {
      assert( player->main_hand_weapon.type != WEAPON_NONE );
      player->main_hand_attack = new fon_melee_t( player );
      trigger_gcd              = timespan_t::zero();
    }

    void execute() override { player->main_hand_attack->schedule_execute(); }

    bool ready() override { return ( player->main_hand_attack->execute_event == nullptr ); }
  };

  druid_t* o() { return static_cast<druid_t*>( owner ); }

  force_of_nature_t( sim_t* sim, druid_t* owner ) : pet_t( sim, owner, "Treant", true /*GUARDIAN*/, true )
  {
    // Treants have base weapon damage + ap from player's sp.
    owner_coeff.ap_from_sp = 0.6;

    // From ExpectedStat.db2
    double base_dps = 2834;  // @70

    switch ( o()->true_level )
    {
      case 70: break;
      case 69: base_dps = 2715; break;
      case 68: base_dps = 2553; break;
      case 67: base_dps = 2313; break;
      case 66: base_dps = 1981; break;
      case 65: base_dps = 1696; break;
      case 64: base_dps = 1451; break;
      case 63: base_dps = 1241; break;
      case 62: base_dps = 1061; break;
      case 61: base_dps = 905;  break;
      default: base_dps = 520;  break;
    }

    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = base_dps * main_hand_weapon.swing_time.total_seconds() / 1000;

    resource_regeneration = regen_type::DISABLED;
    main_hand_weapon.type = WEAPON_BEAST;

    action_list_str = "auto_attack";
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner->resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA ]   = 0;

    initial.stats.attribute[ ATTR_INTELLECT ] = 0;
    initial.spell_power_per_intellect         = 0;
    intellect_per_owner                       = 0;
    stamina_per_owner                         = 0;
  }

  resource_e primary_resource() const override { return RESOURCE_NONE; }

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this );

    return pet_t::create_action( name, options_str );
  }
};
}  // end namespace pets

namespace buffs
{
template <typename Base>
struct druid_buff_t : public Base
{
protected:
  using base_t = druid_buff_t<buff_t>;

  // Used when shapeshifting to switch to a new attack & schedule it to occur
  // when the current swing timer would have ended.
  void swap_melee( attack_t* new_attack, weapon_t& new_weapon )
  {
    if ( p().main_hand_attack && p().main_hand_attack->execute_event )
    {
      new_attack->base_execute_time = new_weapon.swing_time;
      new_attack->execute_event =
          new_attack->start_action_execute_event( p().main_hand_attack->execute_event->remains() );
      p().main_hand_attack->cancel();
    }
    new_attack->weapon   = &new_weapon;
    p().main_hand_attack = new_attack;
    p().main_hand_weapon = new_weapon;
  }

public:
  druid_buff_t( druid_t& p, std::string_view name, const spell_data_t* s = spell_data_t::nil(),
                const item_t* item = nullptr )
    : Base( &p, name, s, item )
  {}

  druid_buff_t( druid_td_t& td, std::string_view name, const spell_data_t* s = spell_data_t::nil(),
                const item_t* item = nullptr )
    : Base( td, name, s, item )
  {}

  druid_t& p() { return *debug_cast<druid_t*>( Base::source ); }

  const druid_t& p() const { return *debug_cast<druid_t*>( Base::source ); }
};

// Shapeshift Form Buffs ====================================================

// Bear Form ================================================================
struct bear_form_buff_t : public druid_buff_t<buff_t>
{
  double rage_gain;

  bear_form_buff_t( druid_t& p )
    : base_t( p, "bear_form", p.find_class_spell( "Bear Form" ) ),
      rage_gain( p.find_spell( 17057 )->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_CRIT_AVOIDANCE );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_STAMINA );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( p().caster_melee_attack, p().caster_form_weapon );

    p().resource_loss( RESOURCE_RAGE, p().resources.current[ RESOURCE_RAGE ] );
    p().recalculate_resource_max( RESOURCE_HEALTH );

    p().buff.ironfur->expire();
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    p().buff.tigers_fury->expire();  // Mar 03 2016: Tiger's Fury ends when you enter bear form.

    swap_melee( p().bear_melee_attack, p().bear_weapon );

    base_t::start( stacks, value, duration );

    p().resource_gain( RESOURCE_RAGE, rage_gain, p().gain.bear_form );
    p().recalculate_resource_max( RESOURCE_HEALTH );
  }
};

// Cat Form =================================================================
struct cat_form_buff_t : public druid_buff_t<buff_t>
{
  cat_form_buff_t( druid_t& p ) : base_t( p, "cat_form", p.find_class_spell( "Cat Form" ) )
  {
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( p().caster_melee_attack, p().caster_form_weapon );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    swap_melee( p().cat_melee_attack, p().cat_weapon );

    base_t::start( stacks, value, duration );
  }
};

// Moonkin Form =============================================================
struct moonkin_form_buff_t : public druid_buff_t<buff_t>
{
  moonkin_form_buff_t( druid_t& p ) : base_t( p, "moonkin_form", p.talent.moonkin_form )
  {
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

// Druid Buffs ==============================================================

// Berserk (Guardian) / Incarn Buff =========================================
struct berserk_bear_buff_t : public druid_buff_t<buff_t>
{
  bool inc;
  double hp_mul;

  berserk_bear_buff_t( druid_t& p, std::string_view n, const spell_data_t* s, bool b = false )
    : base_t( p, n, s ), inc( b ), hp_mul( 1.0 )
  {
    set_cooldown( 0_ms );
    set_refresh_behavior( buff_refresh_behavior::EXTEND );

    if ( !inc && p.specialization() == DRUID_GUARDIAN )
      name_str_reporting = "berserk";

    if ( p.talent.berserk_unchecked_aggression.ok() )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      apply_affecting_aura( p.talent.berserk_unchecked_aggression );
      set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    }

    if ( p.legendary.legacy_of_the_sleeper->ok() )
      add_invalidate( CACHE_LEECH );

    if ( inc )
      hp_mul += p.query_aura_effect( s, A_MOD_INCREASE_HEALTH_PERCENT )->percent();
  }

  void start( int s, double v, timespan_t d ) override
  {
    base_t::start( s, v, d );

    if ( inc )
    {
      p().resources.max[ RESOURCE_HEALTH ] *= hp_mul;
      p().resources.current[ RESOURCE_HEALTH ] *= hp_mul;
      p().recalculate_resource_max( RESOURCE_HEALTH );
    }
  }

  void expire_override( int s, timespan_t d ) override
  {
    if ( inc )
    {
      p().resources.max[ RESOURCE_HEALTH ] /= hp_mul;
      p().resources.current[ RESOURCE_HEALTH ] /= hp_mul;
      p().recalculate_resource_max( RESOURCE_HEALTH );
    }

    base_t::expire_override( s, d );
  }
};

// Berserk (Feral) / Incarn Buff ============================================
struct berserk_cat_buff_t : public druid_buff_t<buff_t>
{
  bool inc;

  berserk_cat_buff_t( druid_t& p, std::string_view n, const spell_data_t* s, bool b = false )
    : base_t( p, n, s ), inc( b )
  {
    set_cooldown( 0_ms );
    if ( !inc && p.specialization() == DRUID_FERAL )
      name_str_reporting = "berserk";

    if ( inc )
      set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST );
  }
};

// Bloodtalons Tracking Buff ================================================
struct bt_dummy_buff_t : public druid_buff_t<buff_t>
{
  int count;

  bt_dummy_buff_t( druid_t& p, std::string_view n )
    : base_t( p, n ), count( as<int>( p.talent.bloodtalons->effectN( 2 ).base_value() ) )
  {
    // The counting starts from the end of the triggering ability gcd.
    set_duration( timespan_t::from_seconds( p.talent.bloodtalons->effectN( 1 ).base_value() ) + 1_s );
    set_quiet( true );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    if ( !p().talent.bloodtalons.ok() )
      return false;

    if ( p().buff.bt_rake->check() + p().buff.bt_shred->check() + p().buff.bt_swipe->check() +
         p().buff.bt_thrash->check() + p().buff.bt_moonfire->check() + p().buff.bt_brutal_slash->check() + !this->check() < count )
    {
      return base_t::trigger( s, v, c, d );
    }

    p().buff.bt_rake->expire();
    p().buff.bt_shred->expire();
    p().buff.bt_swipe->expire();
    p().buff.bt_thrash->expire();
    p().buff.bt_moonfire->expire();
    p().buff.bt_brutal_slash->expire();

    p().buff.bloodtalons->trigger();

    return true;
  }
};

// Celestial Alignment / Incarn Buff ========================================
struct celestial_alignment_buff_t : public druid_buff_t<buff_t>
{
  bool inc;

  celestial_alignment_buff_t( druid_t& p, std::string_view n, const spell_data_t* s, bool b = false )
    : base_t( p, n, p.apply_override( s, p.talent.syzygy ) ), inc( b )
  {
    set_cooldown( 0_ms );

    if ( p.talent.celestial_alignment.ok() )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    }

    if ( inc )
    {
      add_invalidate( CACHE_CRIT_CHANCE );
    }
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    bool ret = base_t::trigger( s, v, c, d );

    if ( ret )
    {
      p().eclipse_handler.trigger_both( remains() );
      p().uptime.combined_ca_inc->update( true, sim->current_time() );

      if ( p().active.syzygy )
        p().active.syzygy->execute_on_target( p().target );
    }

    return ret;
  }

  void extend_duration( player_t* player, timespan_t d ) override
  {
    base_t::extend_duration( player, d );

    p().eclipse_handler.extend_both( d );
  }

  void expire_override( int s, timespan_t d ) override
  {
    base_t::expire_override( s, d );

    p().eclipse_handler.expire_both();
    p().uptime.primordial_arcanic_pulsar->update( false, sim->current_time() );
    p().uptime.combined_ca_inc->update( false, sim->current_time() );
  }

};

// Eclipse Buff =============================================================
struct eclipse_buff_t : public druid_buff_t<buff_t>
{
  bool is_lunar;
  bool is_solar;

  eclipse_buff_t( druid_t& p, std::string_view n, const spell_data_t* s )
    : base_t( p, n, s ),
      is_lunar( data().id() == p.spec.eclipse_lunar->id() ),
      is_solar( data().id() == p.spec.eclipse_solar->id() )
  {
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE );
  }

  void trigger_sundered_firmament()
  {
    if ( p().active.sundered_firmament && p().buff.parting_skies->check() )
    {
      p().active.sundered_firmament->execute_on_target( p().target );
      p().buff.parting_skies->expire();
    }
    else
    {
      p().buff.parting_skies->trigger();
    }
  }

  void execute( int s, double v, timespan_t d ) override
  {
    bool was_active = check();

    base_t::execute( s, v, d );

    if ( !was_active)
    {
      if ( p().talent.sundered_firmament.ok() )
        trigger_sundered_firmament();

      if ( p().sets->has_set_bonus( DRUID_BALANCE, T29, B4 ) )
        p().buff.touch_the_cosmos->trigger();
    }
  }

  /* TODO: re-enable if this comes back, otherwise remove by launch
  void extend_duration( player_t* player, timespan_t d ) override
  {
    base_t::extend_duration( player, d );

    trigger_sundered_firmament();
  }*/

  void expire_override( int s, timespan_t d ) override
  {
    base_t::expire_override( s, d );

    if ( p().talent.natures_grace.ok() )
      p().buff.natures_grace->trigger();

    p().eclipse_handler.advance_eclipse();
  }
};

// Fury of Elune AP =========================================================
struct fury_of_elune_buff_t : public druid_buff_t<buff_t>
{
  fury_of_elune_buff_t( druid_t& p, std::string_view n, const spell_data_t* s ) : base_t( p, n, s )
  {
    set_cooldown( 0_ms );
    set_refresh_behavior( buff_refresh_behavior::DURATION );

    auto eff = p.query_aura_effect( &data(), A_PERIODIC_ENERGIZE, RESOURCE_ASTRAL_POWER );
    auto ap = eff->resource( RESOURCE_ASTRAL_POWER );
    set_default_value( ap / eff->period().total_seconds() );

    auto gain = p.get_gain( n );
    set_tick_callback( [ ap, gain, this ]( buff_t*, int, timespan_t ) {
      player->resource_gain( RESOURCE_ASTRAL_POWER, ap, gain );
    } );
  }
};

// Kindred Empowerment ======================================================
struct kindred_empowerment_buff_t : public druid_buff_t<buff_t>
{
  spell_t** damage;  // actions are created after buffs, so use double pointer
  double pool;
  double pool_mul;
  double add_pct;
  double use_pct;
  bool partner;

  kindred_empowerment_buff_t( druid_t& p, std::string_view n, bool b = false )
    : base_t( p, n, p.cov.kindred_empowerment ),
      pool( 1.0 ), pool_mul( 1.0 ),
      add_pct( p.cov.kindred_empowerment_energize->effectN( 1 ).percent() ),
      use_pct( p.cov.kindred_empowerment->effectN( 2 ).percent() ),
      partner( b )
  {
    set_refresh_behavior( buff_refresh_behavior::DURATION );

    if ( partner )
      damage = &p.active.kindred_empowerment_partner;
    else
      damage = &p.active.kindred_empowerment;

    if ( !p.options.kindred_spirits_target )
    {
      if ( p.specialization() == DRUID_BALANCE || p.specialization() == DRUID_FERAL )
        pool_mul = p.options.kindred_spirits_dps_pool_multiplier;
      else
        pool_mul = p.options.kindred_spirits_non_dps_pool_multiplier;
    }
  }

  void expire_override( int s, timespan_t d ) override
  {
    base_t::expire_override( s, d );

    pool = 1.0;
  }

  void add_pool( const action_state_t* s )
  {
    trigger();

    double raw    = s->result_amount * add_pct * pool_mul;
    double lost   = raw * p().options.kindred_spirits_absorbed;
    double amount = raw - lost;

    sim->print_debug( "KINDRED EMPOWERMENT: Adding {} ({} lost) from {}-{} to {} pool of {}", amount, lost,
                    s->action->player->name_str, s->action->name_str, partner ? "partner" : "self", pool );

    pool += amount;
  }

  void use_pool( const action_state_t* s )
  {
    if ( pool <= 1.0 )  // minimum pool value of 1
      return;

    double amount = s->result_amount * use_pct;
    if ( amount == 0 )
      return;

    sim->print_debug( "KINDRED EMPOWERMENT: Using {} from {} pool of {} on {}-{}", amount, partner ? "partner" : "self",
                    pool, s->action->player->name_str, s->action->name_str );

    amount = std::min( amount, pool - 1.0 );
    pool -= amount;

    ( *damage )->execute_on_target( s->target, amount );
  }
};

// Protector of the Pack =====================================================
struct protector_of_the_pack_buff_t : public druid_buff_t<buff_t>
{
  double mul;
  double cap_coeff;  // from tooltip desc

  protector_of_the_pack_buff_t( druid_t& p, std::string_view n, const spell_data_t* s )
    : base_t( p, n, s ), mul( p.talent.protector_of_the_pack->effectN( 1 ).percent() ), cap_coeff( 1.2 )
  {
    set_name_reporting( "protector_of_the_pack" );
  }

  void add_amount( double amt )
  {
    auto cap = p().cache.spell_power( SCHOOL_MAX ) * cap_coeff;

    if ( current_value >= cap )
      return;

    current_value = std::min( cap, current_value + amt * mul );
  }
};

// Tiger Dash Buff ===========================================================
struct tiger_dash_buff_t : public druid_buff_t<buff_t>
{
  tiger_dash_buff_t( druid_t& p ) : base_t( p, "tiger_dash", p.talent.tiger_dash )
  {
    set_cooldown( 0_ms );
    set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );
    set_freeze_stacks( true );
    set_tick_callback( []( buff_t* b, int, timespan_t ) { b->current_value -= b->data().effectN( 2 ).percent(); } );
  }
};

// Umbral Embrace ===========================================================
struct umbral_embrace_buff_t : public druid_buff_t<buff_t>
{
  std::vector<action_t*> actions;

  umbral_embrace_buff_t( druid_t& p ) : base_t( p, "umbral_embrace", p.talent.umbral_embrace->effectN( 2 ).trigger() )
  {
    set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
      if ( !old_ )
      {
        for ( auto a : actions )
          a->set_school_override( SCHOOL_ASTRAL );
      }
      else if ( !new_ )
      {
        for ( auto a : actions )
          a->clear_school_override();
      }
    } );
  }
};

// Ursine Vigor =============================================================
struct ursine_vigor_buff_t : public druid_buff_t<buff_t>
{
  double hp_mul;

  ursine_vigor_buff_t( druid_t& p )
    : base_t( p, "ursine_vigor", p.talent.ursine_vigor->effectN( 1 ).trigger() ), hp_mul( 1.0 )
  {
    set_default_value( p.talent.ursine_vigor->effectN( 1 ).percent() );
    add_invalidate( CACHE_ARMOR );

    hp_mul += default_value;
  }

  void start( int s, double v, timespan_t d ) override
  {
    base_t::start( s, v, d );

    p().resources.max[ RESOURCE_HEALTH ] *= hp_mul;
    p().resources.current[ RESOURCE_HEALTH ] *= hp_mul;
    p().recalculate_resource_max( RESOURCE_HEALTH );
  }

  void expire_override( int s, timespan_t d ) override
  {
    p().resources.max[ RESOURCE_HEALTH ] /= hp_mul;
    p().resources.current[ RESOURCE_HEALTH ] /= hp_mul;
    p().recalculate_resource_max( RESOURCE_HEALTH );

    base_t::expire_override( s, d );
  }
};

// External Buffs (not derived from druid_buff_t) ==========================

// Kindred Affinity =========================================================
// Note the base is stat_buff_t to allow for easier handling of the Kyrian case, which gives 100 mastery rating instead
// of stat %
struct kindred_affinity_base_t : public stat_buff_t
{
  kindred_affinity_base_t( player_t* p, std::string_view n ) : stat_buff_t( p, n, p->find_spell( 357564 ) )
  {
    set_max_stack( 2 );  // artificially allow second stack to simulate doubling during kindred empowerment
  }

  void init_cov( covenant_e cov, double m = 1.0 )
  {
    if ( cov == covenant_e::KYRIAN )
    {
      // Kyrian uses modify_rating(189) subtype for mastery rating, which is automatically parsed in stat_buff_t ctor
      for ( auto& s : stats )
        s.amount *= m;

      name_str_reporting += "_mastery";
      return;
    }

    double multiplier = m * 0.01;

    stats.clear();

    if ( cov == covenant_e::NECROLORD )
    {
      set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT, P_MAX, multiplier );
      set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );
      name_str_reporting += "_vers";
    }
    else if ( cov == covenant_e::NIGHT_FAE )
    {
      set_default_value_from_effect_type( A_HASTE_ALL, P_MAX, multiplier );
      set_pct_buff_type( STAT_PCT_BUFF_HASTE );
      name_str_reporting += "_haste";
    }
    else if ( cov == covenant_e::VENTHYR )
    {
      set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE, P_MAX, multiplier );
      set_pct_buff_type( STAT_PCT_BUFF_CRIT );
      name_str_reporting += "_crit";
    }
    else
    {
      sim->error( "invalid bonded covenant for Kindred Affinity" );
    }
  }
};

struct kindred_affinity_buff_t : public kindred_affinity_base_t
{
  kindred_affinity_buff_t( druid_t& p ) : kindred_affinity_base_t( &p, "kindred_affinity" )
  {
    if ( !p.legendary.kindred_affinity->ok() )
      return;

    covenant_e cov;

    if ( p.options.kindred_spirits_target )
    {
      if ( p.options.kindred_spirits_target->covenant->type() == covenant_e::INVALID )
        throw std::invalid_argument( fmt::format( "Kindred Spirits target {} does not have a valid covenant.",
                                                  p.options.kindred_spirits_target->name_str ) );
      cov = p.options.kindred_spirits_target->covenant->type();
    }
    else if ( util::str_compare_ci( p.options.kindred_affinity_covenant, "kyrian" ) ||
              util::str_compare_ci( p.options.kindred_affinity_covenant, "mastery" ) ||
              p.options.lone_empowerment )
    {
      cov = covenant_e::KYRIAN;
    }
    else if ( util::str_compare_ci( p.options.kindred_affinity_covenant, "necrolord" ) ||
              util::str_compare_ci( p.options.kindred_affinity_covenant, "versatility" ) )
    {
      cov = covenant_e::NECROLORD;
    }
    else if ( util::str_compare_ci( p.options.kindred_affinity_covenant, "night_fae" ) ||
              util::str_compare_ci( p.options.kindred_affinity_covenant, "haste" ) )
    {
      cov = covenant_e::NIGHT_FAE;
    }
    else if ( util::str_compare_ci( p.options.kindred_affinity_covenant, "venthyr" ) ||
              util::str_compare_ci( p.options.kindred_affinity_covenant, "crit" ) )
    {
      cov = covenant_e::VENTHYR;
    }
    else
    {
      throw std::invalid_argument(
          "Invalid option for druid.kindred_affinity_covenent. Valid options are 'kyrian' 'necrolord' 'night_fae' "
          "'venthyr' 'mastery' 'haste' 'versatility' 'crit'" );
    }

    init_cov( cov );

    assert( p.buff.kindred_empowerment_energize );
    assert( p.buff.lone_empowerment );

    buff_t* buff_on_cast = p.buff.kindred_empowerment_energize;

    if ( p.options.lone_empowerment )
      buff_on_cast = p.buff.lone_empowerment;

    buff_on_cast->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( new_ )
        increment();
      else
        decrement();
    } );
  }
};

struct kindred_affinity_external_buff_t : public kindred_affinity_base_t
{
  kindred_affinity_external_buff_t( player_t* p ) : kindred_affinity_base_t( p, "kindred_affinity_external" )
  {
    init_cov( p->covenant->type(), 0.5 );
  }
};
}  // end namespace buffs

using bfun = std::function<bool()>;
struct buff_effect_t
{
  buff_t* buff;
  double value;
  bool use_stacks;
  bool mastery;
  bfun func;

  buff_effect_t( buff_t* b, double v, bool s = true, bool m = false, bfun f = nullptr )
    : buff( b ), value( v ), use_stacks( s ), mastery( m ), func( std::move( f ) )
  {}
};

using dfun = std::function<dot_t*( druid_td_t* )>;
struct dot_debuff_t
{
  dfun func;
  double value;
  bool use_stacks;

  dot_debuff_t( dfun f, double v, bool b )
    : func( std::move( f ) ), value( v ), use_stacks( b )
  {}
};

// Template for common druid action code. See priest_action_t.
template <class Base>
struct druid_action_t : public Base
{
private:
  using ab = Base;  // action base, eg. spell_t

public:
  using base_t = druid_action_t<Base>;

  // auto parsed dynamic effects
  std::vector<buff_effect_t> ta_multiplier_buffeffects;
  std::vector<buff_effect_t> da_multiplier_buffeffects;
  std::vector<buff_effect_t> execute_time_buffeffects;
  std::vector<buff_effect_t> recharge_multiplier_buffeffects;
  std::vector<buff_effect_t> cost_buffeffects;
  std::vector<buff_effect_t> flat_cost_buffeffects;
  std::vector<buff_effect_t> crit_chance_buffeffects;
  std::vector<dot_debuff_t> target_multiplier_dotdebuffs;
  // list of action_ids that triggers the same dot as this action
  std::vector<int> dot_ids;
  // Name to be used by get_dot() instead of action name
  std::string dot_name;
  // form spell to automatically cast
  action_t* autoshift;
  // Action is cast as a proc or replaces an existing action with a no-cost/no-cd version
  free_spell_e free_spell;
  // Restricts use of a spell based on form.
  unsigned form_mask;
  // Allows a spell that may be cast in NO_FORM but not in current form to be cast by exiting form.
  bool may_autounshift;
  bool triggers_galactic_guardian;
  bool is_auto_attack;
  bool break_stealth;

  druid_action_t( std::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      dot_name( n ),
      autoshift( nullptr ),
      free_spell( free_spell_e::NONE ),
      form_mask( ab::data().stance_mask() ),
      may_autounshift( true ),
      triggers_galactic_guardian( true ),
      is_auto_attack( false ),
      break_stealth( !ab::data().flags( spell_attribute::SX_NO_STEALTH_BREAK ) )
  {
    // WARNING: auto attacks will NOT get processed here since they have no spell data
    if ( ab::data().ok() )
    {
      apply_buff_effects();
      apply_dot_debuffs();
    }
  }

  std::unique_ptr<expr_t> create_expression( std::string_view ) override;

  druid_t* p() { return static_cast<druid_t*>( ab::player ); }

  const druid_t* p() const { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const { return p()->get_target_data( t ); }

  base_t* set_free_cast( free_spell_e f )
  {
    free_spell = f;
    ab::cooldown->duration = 0_ms;
    return this;
  }

  bool is_free() const
  {
    return free_spell;
  }

  bool is_free_proc() const
  {
    switch ( free_spell )
    {
      case free_spell_e::APEX:
      case free_spell_e::COSMOS:
      case free_spell_e::STARWEAVER:
      case free_spell_e::NONE:
        return false;
      default:
        return true;
    }
  }

  bool is_free_cast() const
  {
    switch ( free_spell )
    {
      case free_spell_e::APEX:
      case free_spell_e::COSMOS:
      case free_spell_e::STARWEAVER:
        return true;
      default:
        return false;
    }
  }

  static std::string get_suffix( std::string_view name, std::string_view base )
  {
    return std::string( name.substr( std::min( name.size(), name.find( base ) + base.size() ) ) );
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t )
      t = ab::target;
    if ( !t )
      return nullptr;

    dot_t*& dot = ab::target_specific_dot[ t ];
    if ( !dot )
      dot = t->get_dot( dot_name, ab::player );
    return dot;
  }

  unsigned get_dot_count() const
  {
    return range::accumulate( dot_ids, 0U, [ this ]( int add ) {
      return ab::player->get_active_dots( add );
    } );
  }

  bool ready() override
  {
    if ( !check_form_restriction() && !( ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM ) || autoshift ) )
    {
      ab::sim->print_debug( "{} ready() failed due to wrong form. form={:#010x} form_mask={:#010x}", ab::name(),
                            static_cast<unsigned int>( p()->get_form() ), form_mask );

      return false;
    }

    return ab::ready();
  }

  void snapshot_and_execute( const action_state_t* s, bool is_dot,
                             std::function<void( action_state_t* )> pre = nullptr,
                             std::function<void( action_state_t* )> post = nullptr )
  {
    auto state = this->get_state();
    state->target = s->target;

    if ( pre )
      pre( state );

    this->snapshot_state( state, this->amount_type( state, is_dot ) );

    if ( post )
      post( state );

    this->schedule_execute( state );
  }

  void schedule_execute( action_state_t* s = nullptr ) override
  {
    check_autoshift();

    ab::schedule_execute( s );
  }

  void execute() override
  {
    ab::execute();

    trigger_ravenous_frenzy( free_spell );

    if ( break_stealth )
    {
      p()->buff.prowl->expire();
      p()->buffs.shadowmeld->expire();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    trigger_galactic_guardian( s );
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    trigger_galactic_guardian( d->state );
  }

  void assess_damage( result_amount_type t, action_state_t* s ) override
  {
    ab::assess_damage( t, s );

    if ( !s->result_amount || !( t == result_amount_type::DMG_DIRECT || t == result_amount_type::DMG_OVER_TIME ) )
      return;

    if ( p()->buff.natures_vigil->check() && s->n_targets == 1 )
      p()->buff.natures_vigil->current_value += s->result_amount;

    if ( p()->active.elunes_favored_heal && p()->get_form() == BEAR_FORM &&
         dbc::has_common_school( s->action->get_school(), SCHOOL_ARCANE ) )
    {
      p()->active.elunes_favored_heal->execute_on_target(
          p(), p()->talent.elunes_favored->effectN( 1 ).percent() * s->result_amount );
    }

    if ( p()->talent.protector_of_the_pack.ok() && p()->specialization() != DRUID_RESTORATION )
    {
      auto b = p()->buff.protector_of_the_pack_regrowth;

      if ( !b->check() )
        b->trigger();

      debug_cast<buffs::protector_of_the_pack_buff_t*>( b )->add_amount( s->result_amount );
    }
  }

  virtual void trigger_ravenous_frenzy( free_spell_e f )
  {
    if ( ab::background || ab::trigger_gcd == 0_ms || !p()->buff.ravenous_frenzy->check() )
      return;

    // an instant executed immediately after casting RF does not grant a stack
    if ( p()->buff.ravenous_frenzy->elapsed( p()->sim->current_time() ) < 50_ms )
      return;

    // trigger on non-free_cast or free_cast that requires you to actually cast (or UFR)
    if ( !f || is_free_cast() || f == free_spell_e::FLASHING )
      p()->buff.ravenous_frenzy->trigger();
  }

  void trigger_gore()  // need this here instead of bear_attack_t because of moonfire
  {
    if ( !p()->talent.gore.ok() )
      return;

    if ( p()->buff.gore->trigger() )
    {
      p()->proc.gore->occur();
      p()->cooldown.mangle->reset( true );
    }
  }

  void trigger_galactic_guardian( action_state_t* s )
  {
    if ( !p()->talent.galactic_guardian.ok() )
      return;

    if ( !triggers_galactic_guardian || ab::proc || !ab::harmful )
      return;

    if ( s->target == p() || !ab::result_is_hit( s->result ) || s->result_total <= 0 )
      return;

    if ( !p()->buff.galactic_guardian->cooldown->up() )
      return;

    if ( !ab::rng().roll( p()->talent.galactic_guardian->proc_chance() ) )
      return;

    p()->active.galactic_guardian->execute_on_target( s->target );
    make_event( ab::sim, [ this ]() { p()->buff.galactic_guardian->trigger(); } );  // schedule buff after damage execute
  }

  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  double mod_spell_effects_value( const conduit_data_t& c, const spelleffect_data_t& ) { return c.value(); }

  template <typename T>
  void parse_spell_effects_mods( double& val, bool& mastery, const spell_data_t* base, size_t idx, T mod )
  {
    for ( size_t i = 1; i <= mod->effect_count(); i++ )
    {
      const auto& eff = mod->effectN( i );

      if ( eff.type() != E_APPLY_AURA )
        continue;

      if ( ( base->affected_by_all( eff ) &&
             ( ( eff.misc_value1() == P_EFFECT_1 && idx == 1 ) || ( eff.misc_value1() == P_EFFECT_2 && idx == 2 ) ||
               ( eff.misc_value1() == P_EFFECT_3 && idx == 3 ) || ( eff.misc_value1() == P_EFFECT_4 && idx == 4 ) ||
               ( eff.misc_value1() == P_EFFECT_5 && idx == 5 ) ) ) ||
           ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE && eff.trigger_spell_id() == base->id() && idx == 1 ) )
      {
        double pct = mod_spell_effects_value( mod, eff );

        if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
          val += pct;
        else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
          val *= 1.0 + pct / 100;
        else if ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE )
          val = pct;
        else
          continue;

        if ( p()->find_mastery_spell( p()->specialization() ) == mod )
          mastery = true;
      }
    }
  }

  void parse_spell_effects_mods( double&, bool&, const spell_data_t*, size_t ) {}

  template <typename T, typename... Ts>
  void parse_spell_effects_mods( double& val, bool& m, const spell_data_t* base, size_t idx, T mod, Ts... mods )
  {
    parse_spell_effects_mods( val, m, base, idx, mod );
    parse_spell_effects_mods( val, m, base, idx, mods... );
  }

  // Will parse simple buffs that ONLY target the caster and DO NOT have multiple ranks
  // 1: Add Percent Modifier to Spell Direct Amount
  // 2: Add Percent Modifier to Spell Periodic Amount
  // 3: Add Percent Modifier to Spell Cast Time
  // 4: Add Percent Modifier to Spell Cooldown
  // 5: Add Percent Modifier to Spell Resource Cost
  // 6: Add Flat Modifier to Spell Critical Chance
  template <typename... Ts>
  void parse_buff_effect( buff_t* buff, bfun f, const spell_data_t* s_data, size_t i, bool use_stacks, bool use_default,
                          Ts... mods )
  {
    const auto& eff = s_data->effectN( i );
    double val      = eff.base_value();
    double val_mul  = 0.01;
    bool mastery    = p()->find_mastery_spell( p()->specialization() ) == s_data;

    // TODO: more robust logic around 'party' buffs with radius
    if ( !( eff.type() == E_APPLY_AURA || eff.type() == E_APPLY_AREA_AURA_PARTY ) || eff.radius() ) return;

    if ( i <= 5 )
      parse_spell_effects_mods( val, mastery, s_data, i, mods... );

    auto debug_message = [ & ]( std::string_view type ) {
      if ( buff )
      {
        p()->sim->print_debug( "buff-effects: {} ({}) {} modified by {}{} with buff {} ({}#{})", ab::name(), ab::id,
                               type, val * val_mul, mastery ? "+mastery" : "", buff->name(), buff->data().id(), i );
      }
      else if ( mastery && !f )
      {
        p()->sim->print_debug( "mastery-effects: {} ({}) {} modified by {}+mastery from {} ({}#{})", ab::name(), ab::id,
                               type, val * val_mul, s_data->name_cstr(), s_data->id(), i );
      }
      else if ( f )
      {
        p()->sim->print_debug( "conditional-effects: {} ({}) {} modified by {} with condition from {} ({}#{})",
                               ab::name(), ab::id, type, val * val_mul, s_data->name_cstr(), s_data->id(), i );
      }
      else
      {
        p()->sim->print_debug( "passive-effects: {} ({}) {} modified by {} from {} ({}#{})", ab::name(), ab::id, type,
                               val * val_mul, s_data->name_cstr(), s_data->id(), i );
      }
    };

    if ( is_auto_attack && eff.subtype() == A_MOD_AUTO_ATTACK_PCT )
    {
      da_multiplier_buffeffects.emplace_back( buff, val * val_mul );
      debug_message( "auto attack" );
      return;
    }

    if ( !ab::data().affected_by_all( eff ) )
      return;

    if ( use_default && buff)
    {
      val = buff->default_value;
      val_mul = 1.0;
    }

    if ( !mastery && !val )
      return;

    if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
    {
      switch ( eff.misc_value1() )
      {
        case P_GENERIC:
          da_multiplier_buffeffects.emplace_back( buff, val * val_mul, use_stacks, mastery, f );
          debug_message( "direct damage" );
          break;
        case P_TICK_DAMAGE:
          ta_multiplier_buffeffects.emplace_back( buff, val * val_mul, use_stacks, mastery, f );
          debug_message( "tick damage" );
          break;
        case P_CAST_TIME:
          execute_time_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "cast time" );
          break;
        case P_COOLDOWN:
          recharge_multiplier_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "cooldown" );
          break;
        case P_RESOURCE_COST:
          cost_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "cost percent" );
          break;
        default:
          return;
      }
    }
    else if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
    {
      switch ( eff.misc_value1() )
      {
        case P_CRIT:
          crit_chance_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "crit chance" );
          break;
        case P_RESOURCE_COST:
          val_mul = eff.resource_multiplier( ab::current_resource() );
          flat_cost_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "flat cost" );
          break;
        default:
          return;
      }
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, unsigned ignore_mask, bool use_stacks, bool use_default, Ts... mods )
  {
    if ( !buff )
      return;

    const spell_data_t* s_data = &buff->data();

    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( buff, nullptr, s_data, i, use_stacks, use_default, mods... );
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, unsigned ignore_mask, Ts... mods )
  {
    parse_buff_effects<Ts...>( buff, ignore_mask, true, false, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, bool stack, bool use_default, Ts... mods )
  {
    parse_buff_effects<Ts...>( buff, 0U, stack, use_default, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, bool stack, Ts... mods )
  {
    parse_buff_effects<Ts...>( buff, 0U, stack, false, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, Ts... mods )
  {
    parse_buff_effects<Ts...>( buff, 0U, true, false, mods... );
  }

  void parse_conditional_effects( const spell_data_t* spell, bfun f, unsigned ignore_mask = 0U )
  {
    if ( !spell || !spell->ok() )
      return;

    for ( size_t i = 1 ; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( nullptr, f, spell, i, false, false );
    }
  }

  void parse_passive_effects( const spell_data_t* spell, unsigned ignore_mask = 0U )
  {
    parse_conditional_effects( spell, nullptr, ignore_mask );
  }

  double get_buff_effects_value( const std::vector<buff_effect_t>& buffeffects, bool flat = false,
                                 bool benefit = true ) const
  {
    double return_value = flat ? 0.0 : 1.0;

    for ( const auto& i : buffeffects )
    {
      double eff_val = i.value;
      int mod = 1;

      if ( i.func && !i.func() )
          continue;  // continue to next effect if conditional effect function is false

      if ( i.buff )
      {
        auto stack = benefit ? i.buff->stack() : i.buff->check();

        if ( !stack )
          continue;  // continue to next effect if stacks == 0 (buff is down)

        mod = i.use_stacks ? stack : 1;
      }

      if ( i.mastery )
        eff_val += p()->cache.mastery_value();

      if ( flat )
        return_value += eff_val * mod;
      else
        return_value *= 1.0 + eff_val * mod;
    }

    return return_value;
  }

  // Syntax: parse_buff_effects[<S|C[,...]>]( buff[, ignore_mask|use_stacks[, use_default]][, spell|conduit][,...] )
  //  buff = buff to be checked for to see if effect applies
  //  ignore_mask = optional bitmask to skip effect# n corresponding to the n'th bit
  //  use_stacks = optional, default true, whether to multiply value by stacks, mutually exclusive with ignore parameters
  //  use_default = optional, default false, whether to use buff's default value over effect's value
  //  S/C = optional list of template parameter to indicate spell or conduit with redirect effects
  //  spell/conduit = optional list of spell or conduit with redirect effects that modify the effects on the buff
  void apply_buff_effects()
  {
    using S = const spell_data_t*;
    using C = const conduit_data_t&;

    parse_buff_effects( p()->buff.ravenous_frenzy );
    parse_buff_effects( p()->buff.sinful_hysteria );
    parse_buff_effects<C>( p()->buff.convoke_the_spirits, p()->conduit.conflux_of_elements );
    parse_buff_effects( p()->buff.lone_empowerment );

    // Class
    parse_buff_effects( p()->buff.cat_form );
    parse_buff_effects( p()->buff.moonkin_form );
    parse_buff_effects( p()->buff.protector_of_the_pack_moonfire, false, true );
    parse_buff_effects( p()->buff.protector_of_the_pack_regrowth, false, true );

    switch( p()->specialization() )
    {
      case DRUID_BALANCE:     parse_buff_effects( p()->buff.heart_of_the_wild, 0b000000000111U ); break;
      case DRUID_FERAL:       parse_buff_effects( p()->buff.heart_of_the_wild, 0b000000111000U ); break;
      case DRUID_GUARDIAN:    parse_buff_effects( p()->buff.heart_of_the_wild, 0b000111000000U ); break;
      case DRUID_RESTORATION: parse_buff_effects( p()->buff.heart_of_the_wild, 0b111000000000U ); break;
      default: break;
    }

    // Balance
    parse_buff_effects<S>( p()->buff.balance_of_all_things_arcane, p()->talent.balance_of_all_things );
    parse_buff_effects<S>( p()->buff.balance_of_all_things_nature, p()->talent.balance_of_all_things );
    parse_buff_effects<S>( p()->buff.eclipse_lunar, p()->talent.umbral_intensity );
    parse_buff_effects<S>( p()->buff.eclipse_solar, p()->talent.umbral_intensity );
    parse_buff_effects( p()->buff.gathering_starstuff );
    parse_buff_effects<S>( p()->buff.incarnation_moonkin, p()->talent.elunes_guidance );
    parse_buff_effects( p()->buff.owlkin_frenzy );
    parse_buff_effects( p()->buff.rattled_stars );
    parse_buff_effects( p()->buff.starweavers_warp );
    parse_buff_effects( p()->buff.starweavers_weft );
    parse_buff_effects( p()->buff.touch_the_cosmos );
    parse_buff_effects<S>( p()->buff.umbral_embrace, p()->talent.umbral_embrace );
    parse_buff_effects( p()->buff.warrior_of_elune, false );

    // Feral
    parse_buff_effects( p()->buff.apex_predators_craving );
    parse_buff_effects( p()->buff.incarnation_cat );
    parse_buff_effects( p()->buff.predatory_swiftness );
    parse_buff_effects( p()->buff.sabertooth, true, true );
    parse_buff_effects( p()->buff.sharpened_claws_bloodied_fangs );

    // Guardian
    parse_buff_effects( p()->buff.bear_form );
    unsigned berserk_ignore_mask = p()->talent.berserk_persistence.ok() ? 0b11111100000U
                                                                        : 0b11111100110U;
                                                                          //10987654321
    parse_buff_effects<S, S>( p()->buff.berserk_bear, berserk_ignore_mask,
                              p()->talent.berserk_ravage,
                              p()->talent.berserk_unchecked_aggression );
    parse_buff_effects<S, S>( p()->buff.incarnation_bear, berserk_ignore_mask,  // TODO fix fix fix once data is not a disaster
                              p()->talent.berserk_ravage,
                              p()->talent.berserk_unchecked_aggression );
    parse_buff_effects( p()->buff.gory_fur );
    parse_buff_effects( p()->buff.overpowering_aura );
    parse_passive_effects( p()->talent.reinvigoration, p()->talent.innate_resolve.ok() ? 0b01U : 0b10U );
    parse_buff_effects( p()->buff.tooth_and_claw, false );
    parse_buff_effects( p()->buff.vicious_cycle_mangle, false, true );
    parse_buff_effects( p()->buff.vicious_cycle_maul, false, true );
    parse_buff_effects<C>( p()->buff.savage_combatant, p()->conduit.savage_combatant );

    // Restoration
    parse_buff_effects( p()->buff.abundance );
    parse_buff_effects<S>( p()->buff.clearcasting_tree, p()->talent.flash_of_clarity );
    parse_buff_effects( p()->buff.incarnation_tree );
    parse_buff_effects<S>( p()->buff.natures_swiftness, p()->talent.natures_splendor );
  }

  template <typename... Ts>
  void parse_dot_debuffs( const dfun& func, bool use_stacks, const spell_data_t* s_data, Ts... mods )
  {
    if ( !s_data->ok() )
      return;

    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      const auto& eff = s_data->effectN( i );
      double val      = eff.percent();
      bool mastery;  // dummy

      if ( eff.type() != E_APPLY_AURA )
        continue;

      if ( i <= 5 )
        parse_spell_effects_mods( val, mastery, s_data, i, mods... );

      if ( !val )
        continue;

      if ( !( eff.subtype() == A_MOD_DAMAGE_FROM_CASTER_SPELLS && ab::data().affected_by_all( eff ) ) &&
           !( eff.subtype() == A_MOD_AUTO_ATTACK_FROM_CASTER && is_auto_attack ) )
        continue;

      p()->sim->print_debug( "dot-debuffs: {} ({}) damage modified by {}% on targets with dot {} ({}#{})", ab::name(),
                             ab::id, val * 100.0, s_data->name_cstr(), s_data->id(), i );
      target_multiplier_dotdebuffs.emplace_back( func, val, use_stacks  );
    }
  }

  template <typename... Ts>
  void parse_dot_debuffs( dfun func, const spell_data_t* s_data, Ts... mods )
  {
    parse_dot_debuffs( func, true, s_data, mods... );
  }

  double get_dot_debuffs_value( druid_td_t* t ) const
  {
    double return_value = 1.0;

    for ( const auto& i : target_multiplier_dotdebuffs )
    {
      auto dot = i.func( t );

      if ( dot->is_ticking() )
        return_value *= 1.0 + i.value * ( i.use_stacks ? dot->current_stack() : 1.0 );
    }

    return return_value;
  }

  // Syntax: parse_dot_debuffs[<S[,...]>]( func, spell_data_t* dot[, spell_data_t* spell][,...] )
  //  func = function returning the dot_t* of the dot
  //  dot = spell data of the dot
  //  S = optional list of template parameter to indicate spell with redirect effects
  //  spell = optional list of spell with redirect effects that modify the effects on the dot
  void apply_dot_debuffs()
  {
    using S = const spell_data_t*;

    parse_dot_debuffs( []( druid_td_t* t ) -> dot_t* { return t->dots.adaptive_swarm_damage; },
                       false, p()->spec.adaptive_swarm_damage );
    parse_dot_debuffs<S>( []( druid_td_t* t ) -> dot_t* { return t->dots.thrash_bear; },
                          p()->spec.thrash_bear_dot, p()->talent.rend_and_tear );
  }

  double cost() const override
  {
    double c = ab::cost();

    c += get_buff_effects_value( flat_cost_buffeffects, true, false );

    c *= std::max( 0.0, get_buff_effects_value( cost_buffeffects, false, false ) );

    if ( is_free() || ( p()->specialization() == DRUID_RESTORATION && p()->buff.innervate->up() ) )
      return 0.0;

    return c;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t ) * get_dot_debuffs_value( td( t ) );

    return tm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double ta = ab::composite_ta_multiplier( s ) * get_buff_effects_value( ta_multiplier_buffeffects );

    return ta;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = ab::composite_da_multiplier( s ) * get_buff_effects_value( da_multiplier_buffeffects );

    return da;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance() + get_buff_effects_value( crit_chance_buffeffects, true );

    return cc;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = ab::execute_time() * get_buff_effects_value( execute_time_buffeffects );

    return std::max( 0_ms, et );
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = ab::recharge_multiplier( cd ) * get_buff_effects_value( recharge_multiplier_buffeffects );

    return rm;
  }

  // Override this function for temporary effects that change the normal form restrictions of the spell. eg: Predatory
  // Swiftness
  virtual bool check_form_restriction()
  {
    return !form_mask || ( form_mask & p()->get_form() ) == p()->get_form() ||
           ( p()->talent.ursine_adept.ok() && p()->buff.bear_form->check() &&
             ab::data().affected_by( p()->buff.bear_form->data().effectN( 2 ) ) );
  }

  void check_autoshift()
  {
    if ( !check_form_restriction() )
    {
      if ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM )
        p()->active.shift_to_caster->execute();
      else if ( autoshift )
        autoshift->execute();
      else
        assert( false && "Action executed in wrong form with no valid form to shift to!" );
    }
  }

  bool verify_actor_spec() const override
  {
    if ( range::contains( p()->secondary_action_list, this ) )
      return true;
#ifndef NDEBUG
    else
      p()->sim->print_log( "{} failed verification", ab::name() );
#endif

    return ab::verify_actor_spec();
  }
};

// Druid melee attack base for cat_attack_t and bear_attack_t
template <class Base>
struct druid_attack_t : public druid_action_t<Base>
{
private:
  using ab = druid_action_t<Base>;

public:
  using base_t = druid_attack_t<Base>;

  bool direct_bleed;
  double bleed_mul;

  druid_attack_t( std::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ), direct_bleed( false ), bleed_mul( 0.0 )
  {
    ab::may_glance = false;
    ab::special    = true;

    parse_special_effect_data();
  }

  void parse_special_effect_data()
  {
    for ( size_t i = 1; i <= this->data().effect_count(); i++ )
    {
      const spelleffect_data_t& ed = this->data().effectN( i );
      effect_type_t type           = ed.type();

      // Check for bleed flag at effect or spell level.
      if ( ( type == E_SCHOOL_DAMAGE || type == E_WEAPON_PERCENT_DAMAGE ) &&
           ( ed.mechanic() == MECHANIC_BLEED || this->data().mechanic() == MECHANIC_BLEED ) )
      {
        direct_bleed = true;
      }
    }
  }

  double composite_target_armor( player_t* t ) const override
  {
    if ( direct_bleed )
      return 0.0;
    else
      return ab::composite_target_armor( t );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    if ( bleed_mul && t->debuffs.bleeding && t->debuffs.bleeding->up() )
      tm *= 1.0 + bleed_mul;

    return tm;
  }
};

// Druid "Spell" Base for druid_spell_t, druid_heal_t ( and potentially druid_absorb_t )
template <class Base>
struct druid_spell_base_t : public druid_action_t<Base>
{
private:
  using ab = druid_action_t<Base>;

public:
  using base_t = druid_spell_base_t<Base>;

  bool reset_melee_swing;  // TRUE(default) to reset swing timer on execute (as most cast time spells do)

  druid_spell_base_t( std::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ), reset_melee_swing( true )
  {}

  void execute() override
  {
    if ( ab::trigger_gcd > 0_ms && !ab::proc && !ab::background && reset_melee_swing &&
         ab::p()->main_hand_attack && ab::p()->main_hand_attack->execute_event )
    {
      ab::p()->main_hand_attack->execute_event->reschedule( ab::p()->main_hand_weapon.swing_time *
                                                            ab::p()->cache.attack_speed() );
    }

    ab::execute();
  }
};

// for child actions that do damage based on % of parent action
struct druid_residual_data_t
{
  double total_amount = 0.0;

  friend void sc_format_to( const druid_residual_data_t& data, fmt::format_context::iterator out ) {
    fmt::format_to( out, "total_amount={}", data.total_amount );
  }
};

template <class Base, class Data = druid_residual_data_t>
struct druid_residual_action_t : public Base
{
  using base_t = druid_residual_action_t<Base, Data>;
  using state_t = druid_action_state_t<Data>;

  double residual_mul;

  druid_residual_action_t( std::string_view n, druid_t* p, const spell_data_t* s )
    : Base( n, p, s ), residual_mul( 0.0 )
  {
    Base::background = true;
    Base::round_base_dmg = false;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, Base::target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return debug_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return debug_cast<const state_t*>( s );
  }

  void set_amount( action_state_t* s, double v )
  {
    cast_state( s )->total_amount = v;
  }

  virtual double get_amount( const action_state_t* s ) const
  {
    return cast_state( s )->total_amount * residual_mul;
  }

  void init() override
  {
    Base::init();
    Base::update_flags &= ~( STATE_MUL_TA );
  }
};

namespace spells
{
/* druid_spell_t ============================================================
  Early definition of druid_spell_t. Only spells that MUST for use by other
  actions should go here, otherwise they can go in the second spells
  namespace.
========================================================================== */
struct druid_spell_t : public druid_spell_base_t<spell_t>
{
private:
  using ab = druid_spell_base_t<spell_t>;

public:
  druid_spell_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                 std::string_view opt = {} )
    : ab( n, p, s )
  {
    parse_options( opt );
  }

  void consume_resource() override
  {
    ab::consume_resource();

    if ( resource_current == RESOURCE_ASTRAL_POWER )
    {
      if ( is_free_proc() )
        return;

      if ( p()->talent.primordial_arcanic_pulsar.ok() )
      {
        auto p_cap = p()->talent.primordial_arcanic_pulsar->effectN( 1 ).base_value();
        auto p_buff = p()->buff.primordial_arcanic_pulsar;
        auto p_time = timespan_t::from_seconds( p()->talent.primordial_arcanic_pulsar->effectN( 2 ).base_value() );

        if ( !p_buff->check() )
          p_buff->trigger();

        // pulsar accumulate based on base_cost not last_resource_cost
        p_buff->current_value += base_cost();

        if ( p_buff->check_value() >= p_cap )
        {
          p_buff->current_value -= p_cap;

          if ( p()->talent.incarnation_moonkin.ok() && p()->get_form() != form_e::MOONKIN_FORM &&
               !p()->buff.incarnation_moonkin->check() )
          {
            p()->active.shift_to_moonkin->execute();
          }

          p()->buff.ca_inc->extend_duration_or_trigger( p_time, p() );
          p()->proc.pulsar->occur();

          p()->uptime.primordial_arcanic_pulsar->update( true, sim->current_time() );
          make_event( *sim, p_time, [ this ]() {
            p()->uptime.primordial_arcanic_pulsar->update( false, sim->current_time() );
          } );
        }
      }
    }
  }

  void trigger_shooting_stars( player_t* t )
  {
    if ( !p()->active.shooting_stars )
      return;

    double chance = p()->talent.shooting_stars->effectN( 1 ).percent();
    chance /= std::sqrt( get_dot_count() );

    if ( p()->buff.solstice->up() )
      chance *= 1.0 + p()->buff.solstice->check_value();

    if ( rng().roll( chance ) )
      p()->active.shooting_stars->execute_on_target( t );
  }

  std::unique_ptr<expr_t> create_expression( std::string_view name_str ) override
  {
    auto splits = util::string_split<std::string_view>( name_str, "." );

    if ( p()->specialization() == DRUID_BALANCE && util::str_compare_ci( splits[ 0 ], "ap_check" ) )
    {
      int over = splits.size() > 1 ? util::to_int( splits[ 1 ] ) : 0;

      return make_fn_expr( name_str, [ this, over ]() { return p()->check_astral_power( this, over ); } );
    }

    return ab::create_expression( name_str );
  }
};  // end druid_spell_t

struct druid_interrupt_t : public druid_spell_t
{
  druid_interrupt_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view options_str )
    : druid_spell_t( n, p, s, options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = harmful = false;
    ignore_false_positive = use_off_gcd = is_interrupt = true;
  }

  bool target_ready( player_t* t ) override
  {
    if ( !t->debuffs.casting->check() )
      return false;

    return druid_spell_t::target_ready( t );
  }
};

// Form Spells ==============================================================
struct druid_form_t : public druid_spell_t
{
  form_e form;

  druid_form_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt, form_e f )
    : druid_spell_t( n, p, s, opt ), form( f )
  {
    harmful = may_autounshift = reset_melee_swing = false;
    ignore_false_positive = true;

    form_mask = ( NO_FORM | BEAR_FORM | CAT_FORM | MOONKIN_FORM ) & ~form;
  }

  void trigger_ravenous_frenzy( free_spell_e ) override { return; }

  void execute() override
  {
    druid_spell_t::execute();

    p()->shapeshift( form );

    if ( p()->legendary.oath_of_the_elder_druid->ok() && !p()->buff.oath_of_the_elder_druid->check() &&
         !p()->buff.heart_of_the_wild->check() )
    {
      p()->buff.oath_of_the_elder_druid->trigger();
      p()->buff.heart_of_the_wild->trigger(
          timespan_t::from_seconds( p()->legendary.oath_of_the_elder_druid->effectN( 2 ).base_value() ) );
    }
  }
};

// Bear Form Spell ==========================================================
struct bear_form_t : public druid_form_t
{
  bear_form_t( druid_t* p, std::string_view opt )
    : druid_form_t( "bear_form", p, p->find_class_spell( "Bear Form" ), opt, BEAR_FORM )
  {}

  void execute() override
  {
    druid_form_t::execute();

    if ( p()->talent.ursine_vigor.ok() )
      p()->buff.ursine_vigor->trigger();
  }
};

// Cat Form Spell ===========================================================
struct cat_form_t : public druid_form_t
{
  cat_form_t( druid_t* p, std::string_view opt )
    : druid_form_t( "cat_form", p, p->find_class_spell( "Cat Form" ), opt, CAT_FORM )
  {}
};

// Moonkin Form Spell =======================================================
struct moonkin_form_t : public druid_form_t
{
  moonkin_form_t( druid_t* p, std::string_view opt )
    : druid_form_t( "moonkin_form", p, p->talent.moonkin_form, opt, MOONKIN_FORM )
  {}
};

// Cancelform (revert to caster form)========================================
struct cancel_form_t : public druid_form_t
{
  cancel_form_t( druid_t* p, std::string_view opt ) : druid_form_t( "cancelform", p, spell_data_t::nil(), opt, NO_FORM )
  {
    trigger_gcd = 0_ms;
  }
};

// Shooting Stars ===========================================================
struct shooting_stars_t : public druid_spell_t
{
  shooting_stars_t( druid_t* player ) : druid_spell_t( "shooting_stars", player, player->spec.shooting_stars_dmg )
  {
    background = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p()->talent.orbit_breaker.ok() )
      p()->buff.orbit_breaker->trigger();
  }
};
}  // namespace spells

namespace heals
{
// ==========================================================================
// Druid Heal
// ==========================================================================
struct druid_heal_t : public druid_spell_base_t<heal_t>
{
  struct
  {
    bool flourish;  // true if tick_rate is affected by flourish, NOTE: extension is handled in flourish_t
    bool soul_of_the_forest;
  } affected_by;

  double imp_fr_mul;  // % healing from improved frenzied regeneration
  double iron_mul;    // % healing from hots with ironbark
  double photo_mul;   // tick rate multiplier when lb is on self
  double photo_pct;   // % chance to bloom when lb is on other
  bool target_self;

  druid_heal_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), std::string_view opt = {} )
    : base_t( n, p, s ),
      affected_by(),
      imp_fr_mul( p->talent.verdant_heart->effectN( 1 ).percent() ),
      iron_mul( 0.0 ),
      photo_mul( p->talent.photosynthesis->effectN( 1 ).percent() ),
      photo_pct( p->talent.photosynthesis->effectN( 2 ).percent() ),
      target_self( false )
  {
    add_option( opt_bool( "target_self", target_self ) );
    parse_options( opt );

    if ( target_self )
      target = p;

    may_miss = harmful = false;
    ignore_false_positive = true;

    if ( p->talent.stonebark.ok() &&
         p->query_aura_effect( p->talent.ironbark, A_MOD_HEALING_RECEIVED, 0, &data() )->ok() )
    {
      iron_mul = p->talent.stonebark->effectN( 1 ).percent();
    }

    if ( p->talent.flourish.ok() )
      affected_by.flourish = p->query_aura_effect( p->talent.flourish, A_ADD_PCT_MODIFIER, P_TICK_TIME, &data() )->ok();
  }

  virtual double harmony_multiplier( player_t* t ) const
  {
    return p()->cache.mastery_value() * td( t )->hots_ticking();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = base_t::composite_target_multiplier( t );

    if ( p()->mastery.harmony->ok() )
      ctm *= 1.0 + harmony_multiplier( t );

    if ( iron_mul && td( t )->buff.ironbark->up() )
      ctm *= 1.0 + iron_mul;

    if ( t == player )
    {
      if ( imp_fr_mul && ( p()->buff.barkskin->check() || td( t )->hots.frenzied_regeneration->is_ticking() ) )
        ctm *= 1.0 + imp_fr_mul;

      ctm *= 1.0 + p()->talent.natural_recovery->effectN( 3 ).percent();
    }

    return ctm;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    auto tt = base_t::tick_time( s );

    // flourish effect is a negative percent modifier, so multiply here
    if ( affected_by.flourish && p()->buff.flourish->check() )
      tt *= 1.0 + p()->buff.flourish->default_value;

    // photo effect is a positive dummy value, so divide here
    if ( photo_mul && td( player )->hots.lifebloom->is_ticking() )
      tt /= 1.0 + photo_mul;

    return tt;
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( photo_pct && d->target != player && rng().roll( photo_pct ) )
    {
      auto lb = td( d->target )->hots.lifebloom;

      if ( lb->is_ticking() )
        lb->current_action->last_tick( lb );
    }
  }

  void execute() override
  {
    base_t::execute();

    if ( affected_by.soul_of_the_forest && p()->buff.soul_of_the_forest_tree->up() )
      p()->buff.soul_of_the_forest_tree->expire();
  }

  void assess_damage( result_amount_type t, action_state_t* s ) override
  {
    base_t::assess_damage( t, s );

    if ( !s->result_amount || !( t == result_amount_type::HEAL_DIRECT || t == result_amount_type::HEAL_OVER_TIME ) )
      return;

    if ( p()->talent.protector_of_the_pack.ok() && p()->specialization() == DRUID_RESTORATION )
    {
      auto b = p()->buff.protector_of_the_pack_moonfire;

      if ( !b->check() )
        b->trigger();

      debug_cast<buffs::protector_of_the_pack_buff_t*>( b )->add_amount( s->result_amount );
    }
  }
};
}  // namespace heals

namespace cat_attacks
{
// ==========================================================================
// Druid Cat Attack
// ==========================================================================
struct cat_attack_t : public druid_attack_t<melee_attack_t>
{
protected:
  bool attack_critical;

public:
  std::vector<buff_effect_t> persistent_multiplier_buffeffects;

  struct
  {
    bool tigers_fury;
    bool bloodtalons;
    bool clearcasting;
  } snapshots;

  snapshot_counter_t* bt_counter;
  snapshot_counter_t* tf_counter;

  double berserk_cp;
  double primal_claws_cp;
  double primal_fury_cp;
  bool consumes_combo_points;

  cat_attack_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), std::string_view opt = {} )
    : base_t( n, p, s ),
      snapshots(),
      bt_counter( nullptr ),
      tf_counter( nullptr ),
      berserk_cp( 0.0 ),
      primal_claws_cp( p->talent.primal_claws->effectN( 2 ).base_value() ),
      primal_fury_cp( p->talent.primal_fury->effectN( 1 ).trigger()->effectN( 1 ).base_value() ),
      consumes_combo_points( false )
  {
    parse_options( opt );

    if ( data().cost( POWER_COMBO_POINT ) )
    {
      consumes_combo_points = true;
      form_mask |= CAT_FORM;

      if ( p->talent.berserk.ok() )
        berserk_cp = p->spec.berserk_cat->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_COMBO_POINT );
    }

    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
      ap_type = attack_power_type::NO_WEAPON;

    if ( data().ok() )
    {
      using S = const spell_data_t*;

      snapshots.bloodtalons =
          parse_persistent_buff_effects( p->buff.bloodtalons, 0U, false );

      snapshots.tigers_fury =
          parse_persistent_buff_effects<S>( p->buff.tigers_fury, 0U, true, p->talent.carnivorous_instinct );

      snapshots.clearcasting =
          parse_persistent_buff_effects<S>( p->buff.clearcasting_cat, 0U, false, p->talent.moment_of_clarity );

      parse_passive_effects( p->mastery.razor_claws );
    }
  }

  virtual bool stealthed() const  // For effects that require any form of stealth.
  {
    // Make sure we call all for accurate benefit tracking. Berserk/Incarn/Sudden Assault handled in shred_t & rake_t -
    // move here if buff while stealthed becomes more widespread
    bool shadowmeld = p()->buffs.shadowmeld->up();
    bool prowl = p()->buff.prowl->up();

    return prowl || shadowmeld;
  }

  void consume_resource() override
  {
    double eff_cost = base_cost();

    base_t::consume_resource();

    // Treat Omen of Clarity energy savings like an energy gain for tracking purposes.
    if ( !is_free() && snapshots.clearcasting && current_resource() == RESOURCE_ENERGY &&
         p()->buff.clearcasting_cat->up() )
    {
      p()->buff.clearcasting_cat->decrement();

      // cat's curiosity refunds energy based on base_cost
      if ( p()->talent.cats_curiosity.ok() )
      {
        p()->resource_gain( RESOURCE_ENERGY, eff_cost * p()->talent.cats_curiosity->effectN( 1 ).percent(),
                            p()->gain.cats_curiosity );
      }

      // Base cost doesn't factor in but Omen of Clarity does net us less energy during it, so account for that here.
      eff_cost *= 1.0 + p()->buff.incarnation_cat->check_value();

      p()->gain.clearcasting->add( RESOURCE_ENERGY, eff_cost );
    }
  }

  template <typename... Ts>
  bool parse_persistent_buff_effects( buff_t* buff, unsigned ignore_mask, bool use_stacks, Ts... mods )
  {
    size_t ta_old   = ta_multiplier_buffeffects.size();
    size_t da_old   = da_multiplier_buffeffects.size();
    size_t cost_old = cost_buffeffects.size();

    parse_buff_effects( buff, ignore_mask, use_stacks, false, mods... );

    // If there is a new entry in the ta_mul table, move it to the pers_mul table.
    if ( ta_multiplier_buffeffects.size() > ta_old )
    {
      double &ta_val = ta_multiplier_buffeffects.back().value;
      double da_val = 0;

      // Any corresponding increases in the da_mul table can be removed as pers_mul covers da_mul & ta_mul
      if ( da_multiplier_buffeffects.size() > da_old )
      {
        da_val = da_multiplier_buffeffects.back().value;
        da_multiplier_buffeffects.pop_back();

        if ( da_val != ta_val )
        {
          p()->sim->print_debug(
              "WARNING: {} (id={}) spell data has inconsistent persistent multiplier benefit for {}.", buff->name(),
              buff->data().id(), this->name() );
        }
      }

      // snapshotting is done via custom scripting, so data can have different da/ta_mul values but the highest will apply
      if ( da_val > ta_val )
        ta_val = da_val;

      persistent_multiplier_buffeffects.push_back( ta_multiplier_buffeffects.back() );
      ta_multiplier_buffeffects.pop_back();

      p()->sim->print_debug( "persistent-buffs: {} ({}) damage modified by {}% with buff {} ({}), tick table has {} entries.", name(), id,
                             ta_val * 100.0, buff->name(), buff->data().id(), ta_multiplier_buffeffects.size() );


      return true;
    }
    // no persistent multiplier, but does snapshot & consume the buff
    if ( da_multiplier_buffeffects.size() > da_old || cost_buffeffects.size() > cost_old )
      return true;

    return false;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm =
        base_t::composite_persistent_multiplier( s ) * get_buff_effects_value( persistent_multiplier_buffeffects );

    return pm;
  }

  snapshot_counter_t* get_counter( buff_t* buff )
  {
    auto st = stats;
    while ( st->parent )
    {
      if ( !st->parent->action_list.front()->harmful )
        return nullptr;
      st = st->parent;
    }

    for ( const auto& c : p()->counters )
      if ( c->stats == st)
        for ( const auto& b : c->buffs )
          if ( b == buff )
            return c.get();

    return p()->counters.emplace_back( std::make_unique<snapshot_counter_t>( p(), st, buff ) ).get();
  }

  void init() override
  {
    base_t::init();

    if ( !is_auto_attack && !data().ok() )
      return;

    if ( snapshots.bloodtalons )
      bt_counter = get_counter( p()->buff.bloodtalons );

    if ( snapshots.tigers_fury )
      tf_counter = get_counter( p()->buff.tigers_fury );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( s->result == RESULT_CRIT )
        attack_critical = true;

      if ( p()->talent.berserk_frenzy->ok() && energize_resource == RESOURCE_COMBO_POINT && energize_amount > 0 &&
           ( p()->buff.berserk_cat->check() || p()->buff.incarnation_cat->check() ) )
      {
        trigger_berserk_frenzy( s->target, s->result_amount );
      }
    }
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    trigger_predator();

    if ( snapshots.bloodtalons && bt_counter )
      bt_counter->count_tick();

    if ( snapshots.tigers_fury && tf_counter )
      tf_counter->count_tick();
  }

  void execute() override
  {
    attack_critical = false;

    base_t::execute();

    if ( energize_resource == RESOURCE_COMBO_POINT && energize_amount > 0 && hit_any_target )
    {
      trigger_primal_fury();
      trigger_primal_claws();
    }

    if ( hit_any_target )
    {
      if ( snapshots.bloodtalons )
      {
        if ( bt_counter )
          bt_counter->count_execute();

        p()->buff.bloodtalons->decrement();
      }

      if ( snapshots.tigers_fury && tf_counter )
        tf_counter->count_execute();
    }

    if ( !hit_any_target )
      trigger_energy_refund();

    if ( harmful )
    {
      if ( special && base_costs[ RESOURCE_ENERGY ] > 0 )
        p()->buff.incarnation_cat->up();
    }
  }

  bool ready() override
  {
    if ( consumes_combo_points && p()->resources.current[ RESOURCE_COMBO_POINT ] < 1 )
      return false;

    return base_t::ready();
  }

  void trigger_energy_refund()
  {
    player->resource_gain( RESOURCE_ENERGY, last_resource_cost * 0.80, p()->gain.energy_refund );
  }

  void trigger_primal_claws()
  {
    if ( proc || !p()->talent.primal_claws.ok() )
      return;

    if ( rng().roll( p()->talent.primal_claws->effectN( 1 ).percent() ) )
    {
      p()->proc.primal_claws->occur();
      p()->resource_gain( RESOURCE_COMBO_POINT, primal_claws_cp, p()->gain.primal_claws );
    }
  }

  virtual void trigger_primal_fury()
  {
    if ( proc || !p()->talent.primal_fury.ok() || !attack_critical )
      return;

    p()->proc.primal_fury->occur();
    p()->resource_gain( RESOURCE_COMBO_POINT, primal_fury_cp, p()->gain.primal_fury );
  }

  void trigger_predator()
  {
    if ( !( p()->talent.predator.ok() && p()->options.predator_rppm_rate > 0 ) )
      return;

    if ( !p()->rppm.predator->trigger() )
      return;

    if ( !p()->cooldown.tigers_fury->down() )
      p()->proc.predator_wasted->occur();

    p()->cooldown.tigers_fury->reset( true );
    p()->proc.predator->occur();
  }

  void trigger_berserk_frenzy( player_t* t, double d )
  {
    if ( !special || !harmful || !d )
      return;

    residual_action::trigger( p()->active.frenzied_assault, t, p()->talent.berserk_frenzy->effectN( 1 ).percent() * d );
  }
};  // end druid_cat_attack_t

struct cat_finisher_data_t
{
  int combo_points = 0;

  friend void sc_format_to( const cat_finisher_data_t& data, fmt::format_context::iterator out ) {
    fmt::format_to( out, "combo_points={}", data.combo_points );
  }
};

template <class Data = cat_finisher_data_t>
struct cat_finisher_t : public cat_attack_t
{
  using state_t = druid_action_state_t<Data>;

  cat_finisher_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt = {} )
    : cat_attack_t( n, p, s, opt )
  {}

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return debug_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return debug_cast<const state_t*>( s );
  }

  virtual int _combo_points() const
  {
    if ( is_free() )
      return as<int>( p()->resources.max[ RESOURCE_COMBO_POINT ] );
    else
      return as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    // snapshot the combo point first so composite_X calculations can correctly refer to it
    cast_state( s )->combo_points = _combo_points();

    cat_attack_t::snapshot_state( s, rt );
  }

  void consume_resource() override
  {
    cat_attack_t::consume_resource();

    if ( background || !hit_any_target )
      return;

    auto consumed = _combo_points();

    if ( p()->talent.berserk_heart_of_the_lion.ok() )
    {
      auto dur_ = timespan_t::from_seconds( p()->talent.berserk_heart_of_the_lion->effectN( 1 ).base_value() *
                                            consumed * -0.1 );

      if ( p()->talent.incarnation_cat.ok() )
        p()->cooldown.incarnation->adjust( dur_ );
      else
        p()->cooldown.berserk_cat->adjust( dur_ );
    }

    if ( p()->talent.frantic_momentum.ok() )
    {
      p()->buff.frantic_momentum->trigger( 1, buff_t::DEFAULT_VALUE(),
                                           consumed * p()->talent.frantic_momentum->effectN( 1 ).percent() );
    }

    if ( p()->talent.predatory_swiftness.ok() )
    {
      p()->buff.predatory_swiftness->trigger( 1, buff_t::DEFAULT_VALUE(),
                                              consumed * p()->talent.predatory_swiftness->effectN( 3 ).percent() );
    }

    // TODO: implement 0.1s ICD
    if ( p()->talent.raging_fury.ok() )
    {
      auto dur_ = timespan_t::from_seconds( p()->talent.raging_fury->effectN( 1 ).base_value() * consumed * 0.1 );

      p()->buff.tigers_fury->extend_duration( p(), dur_ );
    }

    if ( p()->talent.soul_of_the_forest_cat.ok() )
    {
      p()->resource_gain( RESOURCE_ENERGY,
                          consumed * p()->talent.soul_of_the_forest_cat->effectN( 1 ).resource( RESOURCE_ENERGY ),
                          p()->gain.soul_of_the_forest );
    }

    if ( p()->talent.sudden_ambush.ok() )
    {
      p()->buff.sudden_ambush->trigger( 1, buff_t::DEFAULT_VALUE(),
                                        consumed * p()->talent.sudden_ambush->effectN( 1 ).percent() );
    }

    if ( free_spell == free_spell_e::CONVOKE )  // further effects are not processed for convoke fb
      return;

    if ( p()->sets->has_set_bonus( DRUID_FERAL, T29, B4 ) )
      p()->buff.sharpened_claws_bloodied_fangs->trigger( consumed );

    if ( !is_free() )
    {
      p()->resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );

      sim->print_log( "{} consumes {} {} for {} (0)", player->name(), consumed,
                      util::resource_type_string( RESOURCE_COMBO_POINT ), name() );

      stats->consume_resource( RESOURCE_COMBO_POINT, consumed );

      if ( ( p()->buff.berserk_cat->check() || p()->buff.incarnation_cat->check() ) && berserk_cp &&
           rng().roll( consumed * p()->talent.berserk->effectN( 1 ).percent() ) )
      {
        p()->resource_gain( RESOURCE_COMBO_POINT, berserk_cp, p()->gain.berserk );
      }
    }

    if ( p()->buff.tigers_tenacity->check() )
    {
      p()->resource_gain( RESOURCE_COMBO_POINT, p()->buff.tigers_tenacity->value(), p()->gain.tigers_tenacity );
      p()->buff.tigers_tenacity->decrement();
    }
  }
};

// Berserk (Cat) ==============================================================
struct berserk_cat_base_t : public cat_attack_t
{
  buff_t* buff;

  berserk_cat_base_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt )
    : cat_attack_t( n, p, s, opt ), buff( p->buff.berserk_cat )
  {
    harmful   = false;
    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  void execute() override
  {
    cat_attack_t::execute();

    buff->trigger();
  }
};

struct berserk_cat_t : public berserk_cat_base_t
{
  berserk_cat_t( druid_t* p, std::string_view opt ) : berserk_cat_base_t( "berserk_cat", p, p->spec.berserk_cat, opt )
  {
    name_str_reporting = "berserk";
  }

  bool ready() override
  {
    return p()->talent.incarnation_cat.ok() ? false : berserk_cat_base_t::ready();
  }
};

struct incarnation_cat_t : public berserk_cat_base_t
{
  incarnation_cat_t( druid_t* p, std::string_view opt )
    : berserk_cat_base_t( "incarnation_avatar_of_ashamane", p, p->talent.incarnation_cat, opt )
  {
    buff = p->buff.incarnation_cat;
  }

  void execute() override
  {
    berserk_cat_base_t::execute();

    p()->buff.incarnation_cat_prowl->trigger();
  }
};

// Brutal Slash =============================================================
struct brutal_slash_t : public cat_attack_t
{
  brutal_slash_t( druid_t* p, std::string_view opt ) : cat_attack_t( "brutal_slash", p, p->talent.brutal_slash, opt )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();

    if ( p->talent.merciless_strikes.ok() )
      bleed_mul = p->talent.merciless_strikes->effectN( 1 ).percent();
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.bt_brutal_slash->trigger();
  }
};

// Feral Frenzy =============================================================
struct feral_frenzy_t : public cat_attack_t
{
  struct feral_frenzy_data_t
  {
    double tick_amount = 0.0;
    double tick_mul = 1.0;

    friend void sc_format_to( const feral_frenzy_data_t& data, fmt::format_context::iterator out )
    {
      fmt::format_to( out, "tick_amount={} tick_mul={}", data.tick_amount, data.tick_mul );
    }
  };

  struct feral_frenzy_state_t : public action_state_t
  {
    feral_frenzy_state_t( action_t* a, player_t* t ) : action_state_t( a, t ) {}

    // dot damage is entirely overwritten by feral_frenzy_tick_t::base_ta()
    double composite_ta_multiplier() const override
    {
      return 1.0;
    }

    // what the multiplier would have been
    double base_composite_ta_multiplier() const
    {
      return action_state_t::composite_ta_multiplier();
    }
  };

  using state_t = druid_action_state_t<feral_frenzy_data_t, feral_frenzy_state_t>;

  struct feral_frenzy_tick_t : public cat_attack_t
  {
    bool is_direct_damage;

    feral_frenzy_tick_t( druid_t* p, std::string_view n )
      : cat_attack_t( n, p, p->find_spell( 274838 ) ), is_direct_damage( false )
    {
      background = dual = true;
      direct_bleed = false;
      dot_behavior = dot_behavior_e::DOT_REFRESH_DURATION;

      dot_name = "feral_frenzy_tick";
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    state_t* cast_state( action_state_t* s )
    {
      return debug_cast<state_t*>( s );
    }

    const state_t* cast_state( const action_state_t* s ) const
    {
      return debug_cast<const state_t*>( s );
    }

    // Small hack to properly distinguish instant ticks from the driver, from actual periodic ticks from the bleed
    result_amount_type report_amount_type( const action_state_t* s ) const override
    {
      if ( is_direct_damage )
        return result_amount_type::DMG_DIRECT;

      return s->result_type;
    }

    void execute() override
    {
      is_direct_damage = true;
      cat_attack_t::execute();
      is_direct_damage = false;
    }

    void trigger_primal_fury() override {}

    // the dot is 'snapshotted' so we directly use the tick_amount
    double base_ta( const action_state_t* s ) const override
    {
      if ( is_direct_damage )
        return cat_attack_t::base_ta( s );

      auto ff_s = cast_state( s );

      return ff_s->tick_amount * ff_s->tick_mul;
    }

    // dot damage is entirely overwritten by feral_frenzy_tick_t::base_ta()
    double attack_tick_power_coefficient( const action_state_t* ) const override
    {
      return 0.0;
    }

    double base_attack_tick_power_coefficient( const action_state_t* s ) const
    {
      return cat_attack_t::attack_tick_power_coefficient( s );
    }

    void trigger_dot( action_state_t* s ) override
    {
      // calculate the expected total dot amount from the existing state before the state is replaced in trigger_dot()
      auto stored_amount = 0.0;
      auto ff_d = get_dot( target );
      auto ff_s = cast_state( ff_d->state );

      if ( ff_s )
        stored_amount = ff_s->tick_amount * ff_d->ticks_left_fractional();

      cat_attack_t::trigger_dot( s );

      ff_s = cast_state( ff_d->state );

      // calculate base per tick damage: ap * coeff
      auto tick_damage = ff_s->composite_attack_power() * base_attack_tick_power_coefficient( ff_s );
      // calculate expected damage over full duration: tick damage * base duration / hasted tick time
      auto full_damage = tick_damage * ( composite_dot_duration( ff_s ) / tick_time( ff_s ) );

      stored_amount += full_damage;

      // calculate the full expected duration of the dot: remaining duration + elapsed time
      auto full_duration = ff_d->duration() + tick_time( ff_s ) - ff_d->time_to_next_full_tick();
      // damage per tick is proportional to the ratio of the the tick duration to the full duration
      auto tick_amount = stored_amount * tick_time( ff_s ) / full_duration;

      ff_s->tick_amount = tick_amount;

      // the multiplier on the latest hit overwrites multipliers from previous hits and applies to the entire dot
      ff_s->tick_mul = ff_s->base_composite_ta_multiplier();
    }
  };

  feral_frenzy_t( druid_t* p, std::string_view opt ) : feral_frenzy_t( p, "feral_frenzy", p->talent.feral_frenzy, opt )
  {}

  feral_frenzy_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : cat_attack_t( n, p, s, opt )
  {
    if ( data().ok() )
    {
      tick_action = p->get_secondary_action_n<feral_frenzy_tick_t>( name_str + "_tick" );
      tick_action->stats = stats;
      dynamic_tick_action = true;
    }
  }

  void init() override
  {
    cat_attack_t::init();

    if ( tick_action )
      tick_action->gain = gain;
  }
};

// Ferocious Bite ===========================================================
struct ferocious_bite_t : public cat_finisher_t<>
{
  struct rampant_ferocity_t : public druid_residual_action_t<cat_attack_t>
  {
    rampant_ferocity_t( druid_t* p, std::string_view n ) : base_t( n, p, p->find_spell( 391710 ) )
    {
      aoe = -1;
      reduced_aoe_targets = p->talent.rampant_ferocity->effectN( 2 ).base_value();
      name_str_reporting = "rampant_ferocity";

      residual_mul = p->talent.rampant_ferocity->effectN( 1 ).percent();
    }

    double base_da_min( const action_state_t* s ) const override { return get_amount( s ); }
    double base_da_max( const action_state_t* s ) const override { return get_amount( s ); }

    std::vector<player_t*>& target_list() const override
    {
      target_cache.is_valid = false;

      std::vector<player_t*>& tl = cat_attack_t::target_list();

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* t ) {
        return !td( t )->dots.rip->is_ticking() || t == target;
      } ), tl.end() );

      return tl;
    }
  };

  rampant_ferocity_t* rampant_ferocity;
  double excess_energy;
  double max_excess_energy;
  bool max_energy;

  ferocious_bite_t( druid_t* p, std::string_view opt ) : ferocious_bite_t( p, "ferocious_bite", opt ) {}

  ferocious_bite_t( druid_t* p, std::string_view n, std::string_view opt )
    : cat_finisher_t( n, p, p->find_class_spell( "Ferocious Bite" ) ),
      rampant_ferocity( nullptr ),
      excess_energy( 0.0 ),
      max_excess_energy( p->query_aura_effect( &data(), A_NONE, 0, spell_data_t::nil(), E_POWER_BURN )->base_value() ),
      max_energy( false )
  {
    add_option( opt_bool( "max_energy", max_energy ) );
    parse_options( opt );

    if ( p->talent.rampant_ferocity.ok() )
    {
      auto suf = get_suffix( name_str, "ferocious_bite" );
      rampant_ferocity = p->get_secondary_action_n<rampant_ferocity_t>( "rampant_ferocity" + suf );
      rampant_ferocity->background = true;
      add_child( rampant_ferocity );
    }

    if ( p->talent.relentless_predator.ok() )
      max_excess_energy *= 1.0 + p->talent.rampant_ferocity->effectN( 2 ).percent();
  }

  double maximum_energy() const
  {
    double req = base_costs[ RESOURCE_ENERGY ] + max_excess_energy;

    req *= 1.0 + p()->buff.incarnation_cat->check_value();

    if ( p()->buff.apex_predators_craving->check() )
      req *= 1.0 + p()->buff.apex_predators_craving->data().effectN( 1 ).percent();

    return req;
  }

  int _combo_points() const override
  {
    // special handling for convoke FB
    if ( free_spell == free_spell_e::CONVOKE )
      return 4;
    else
      return cat_finisher_t::_combo_points();
  }

  bool ready() override
  {
    if ( max_energy && p()->resources.current[ RESOURCE_ENERGY ] < maximum_energy() )
      return false;

    return cat_finisher_t::ready();
  }

  void execute() override
  {
    if ( !is_free() && p()->buff.apex_predators_craving->up() )
    {
      p()->active.ferocious_bite_apex->execute_on_target( target );
      return;
    }

    // Incarn does affect the additional energy consumption.
    double _max_used = max_excess_energy * ( 1.0 + p()->buff.incarnation_cat->check_value() );

    excess_energy = std::min( _max_used, ( p()->resources.current[ RESOURCE_ENERGY ] - cat_finisher_t::cost() ) );
    
    cat_finisher_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    cat_finisher_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->talent.sabertooth.ok() )
    {
      p()->buff.sabertooth->expire();  // existing buff is replaced with new buff, regardless of CP
      p()->buff.sabertooth->trigger( cast_state( s )->combo_points );
    }

    if ( rampant_ferocity && s->result_amount > 0 && !rampant_ferocity->target_list().empty() )
    {
      rampant_ferocity->snapshot_and_execute( s, false, [ this, s ]( action_state_t* new_ ) {
        rampant_ferocity->set_amount( new_, s->result_amount );
      } );
    }
  }

  void consume_resource() override
  {
    // Extra energy consumption happens first. In-game it happens before the skill even casts but let's not do that
    // because its dumb.
    if ( hit_any_target && !is_free() )
    {
      player->resource_loss( current_resource(), excess_energy );
      stats->consume_resource( current_resource(), excess_energy );
    }

    cat_finisher_t::consume_resource();

    if ( hit_any_target && free_spell == free_spell_e::APEX )
      p()->buff.apex_predators_craving->expire();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_finisher_t::composite_target_multiplier( t );

    if ( p()->talent.taste_for_blood.ok() )
    {
      auto t_td  = td( t );
      int bleeds = t_td->dots.rake->is_ticking() +
                   t_td->dots.rip->is_ticking() +
                   t_td->dots.thrash_bear->is_ticking() +
                   t_td->dots.thrash_cat->is_ticking() +
                   t_td->dots.frenzied_assault->is_ticking() +
                   t_td->dots.feral_frenzy->is_ticking() +
                   t_td->dots.tear->is_ticking();

      tm *= 1.0 + p()->talent.taste_for_blood->effectN( 1 ).percent() * bleeds;
    }

    return tm;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto dam = cat_finisher_t::composite_da_multiplier( s );
    auto energy_mul = is_free() ? 2.0 : 1.0 + excess_energy / max_excess_energy;
    // base spell coeff is for 5CP, so we reduce if lower than 5.
    auto combo_mul = cast_state( s )->combo_points / p()->resources.max[ RESOURCE_COMBO_POINT ];

    // ferocious_bite_max.damage expr calls action_t::calculate_direct_amount, so we must have a separate check for
    // buff.apex_predators_craving, as the free FB from apex is redirected upon execute() which would not have happened
    // when the expr is evaluated
    if ( p()->buff.apex_predators_craving->check() )
    {
      energy_mul = 2.0;
      combo_mul = 1.0;
    }

    return dam * energy_mul * combo_mul;
  }
};

struct frenzied_assault_t : public residual_action::residual_periodic_action_t<cat_attack_t>
{
  frenzied_assault_t( druid_t* p )
    : residual_action::residual_periodic_action_t<cat_attack_t>( "frenzied_assault", p, p->find_spell( 391140 ) )
  {
    background = proc = true;
    may_miss = may_dodge = may_parry = false;
  }
};

// Lunar Inspiration ========================================================
struct lunar_inspiration_t : public cat_attack_t
{
  lunar_inspiration_t( druid_t* p, std::string_view opt ) : lunar_inspiration_t( p, "lunar_inspiration", opt ) {}

  lunar_inspiration_t( druid_t* p, std::string_view n, std::string_view opt )
    : cat_attack_t( n, p, p->find_spell( 155625 ), opt )
  {
    may_dodge = may_parry = may_block = may_glance = false;

    energize_type = action_energize::ON_HIT;
    gcd_type      = gcd_haste_type::ATTACK_HASTE;

    s_data_reporting = p->find_spell( 155580 );  // find by id since you can cast LI without talent
    dot_name = "lunar_inspiration";
  }


  void execute() override
  {
    // Force invalidate target cache so that it will impact on the correct targets.
    target_cache.is_valid = false;

    cat_attack_t::execute();

    if ( hit_any_target )
      p()->buff.bt_moonfire->trigger();
  }

  void trigger_dot( action_state_t* s ) override
  {
    // If existing moonfire duration is longer, lunar inspiration dot is not applied
    if ( td( s->target )->dots.moonfire->remains() > composite_dot_duration( s ) )
      return;

    cat_attack_t::trigger_dot( s );
  }

  bool ready() override
  {
    if ( !p()->talent.lunar_inspiration.ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Maim =====================================================================
struct maim_t : public cat_attack_t
{
  maim_t( druid_t* p, std::string_view options_str ) : cat_attack_t( "maim", p, p->talent.maim, options_str ) {}

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    am *= p()->resources.current[ RESOURCE_COMBO_POINT ];

    return am;
  }
};

// Rake =====================================================================
struct rake_t : public cat_attack_t
{
  struct rake_bleed_t : public cat_attack_t
  {
    rake_bleed_t( druid_t* p, std::string_view n, const spell_data_t* s ) : cat_attack_t( n, p, s->effectN( 3 ).trigger() )
    {
      background = dual = hasted_ticks = true;
      may_miss = may_parry = may_dodge = false;
      // override for convoke. since this is only ever executed from rake_t, form checking is unnecessary.
      form_mask = 0;

      dot_name = "rake";
    }
  };

  rake_bleed_t* bleed;
  double stealth_mul;

  rake_t( druid_t* p, std::string_view opt ) : rake_t( p, "rake", p->talent.rake, opt ) {}

  rake_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : cat_attack_t( n, p, s, opt ), stealth_mul( 0.0 )
  {
    if ( p->talent.pouncing_strikes.ok() || p->spec.improved_prowl->ok() )
      stealth_mul = data().effectN( 4 ).percent();

    aoe = std::max( aoe, 1 ) + as<int>( p->talent.doubleclawed_rake->effectN( 1 ).base_value() );

    bleed = p->get_secondary_action_n<rake_bleed_t>( name_str + "_bleed", &data() );
    bleed->stats = stats;

    dot_name = "rake";
  }

  bool stealthed() const override
  {
    return p()->buff.berserk_cat->check() || p()->buff.incarnation_cat->check() || cat_attack_t::stealthed();
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = cat_attack_t::composite_persistent_multiplier( s );

    if ( stealth_mul && ( stealthed() || p()->buff.sudden_ambush->check() ) )
      pm *= 1.0 + stealth_mul;

    return pm;
  }
  
  bool has_amount_result() const override { return bleed->has_amount_result(); }

  std::vector<player_t*>& target_list() const override
  {
    auto& tl = cat_attack_t::target_list();

    // When Double-Clawed Rake is active, this is an AoE action meaning it will impact onto the first 2 targets in the
    // target list. Instead, we want it to impact on the target of the action and 1 additional, so we'll override the
    // target_list to make it so.
    if ( is_aoe() && as<int>( tl.size() ) > aoe )
    {
      // always hit the target, so if it exists make sure it's first
      auto start_it = tl.begin() + ( tl[ 0 ] == target ? 1 : 0 );

      // randomize remaining targets
      rng().shuffle( start_it, tl.end() );

      // sort by remaining duration
      std::sort( start_it, tl.end(), [ this ]( player_t* a, player_t* b ) {
        return td( a )->dots.rake->remains() < td( b )->dots.rake->remains();
      } );
    }

    return tl;    
  }

  void execute() override
  {
    // Force invalidate target cache so that it will impact on the correct targets.
    target_cache.is_valid = false;

    cat_attack_t::execute();

    if ( hit_any_target )
    {
      p()->buff.bt_rake->trigger();
      
      if ( !stealthed() )
      	p()->buff.sudden_ambush->expire();
    }
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    bleed->snapshot_and_execute( s, true, nullptr, [ s ]( action_state_t* new_ ) {
      // Copy persistent multipliers from the direct attack.
      new_->persistent_multiplier = s->persistent_multiplier;
    } );
  }
};

// Rip ======================================================================
struct rip_t : public cat_finisher_t<>
{
  struct tear_t : public druid_residual_action_t<cat_attack_t>
  {
    tear_t( druid_t* p, std::string_view n ) : base_t( n, p, p->find_spell( 391356 ) )
    {
      name_str_reporting = "tear";

      residual_mul = p->talent.rip_and_tear->effectN( 1 ).percent();
    }

    // TODO: currently bugged to be 6% per tick, rather than 6% over the entire dot
    double base_ta( const action_state_t* s ) const override
    {
      if ( p()->bugs )
        return get_amount( s );
      else
        return get_amount( s ) * tick_time( s ) / composite_dot_duration( s );
    }
  };

  tear_t* tear;

  rip_t( druid_t* p, std::string_view opt ) : rip_t( p, "rip", p->talent.rip, opt ) {}

  rip_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : cat_finisher_t( n, p, s, opt ), tear( nullptr )
  {
    dot_name = "rip";

    if ( p->talent.rip_and_tear.ok() )
    {
      auto suf = get_suffix( name_str, "rip" );
      tear = p->get_secondary_action_n<tear_t>( "tear" + suf );
    }
  }

  void init_finished() override
  {
    cat_finisher_t::init_finished();

    if ( tear )
      add_child( tear );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t t = cat_finisher_t::composite_dot_duration( s );

    return t *= cast_state( s )->combo_points + 1;
  }

  // Newly applied rip will not lower existing duration
  timespan_t calculate_dot_refresh_duration( const dot_t* d, timespan_t dur ) const override
  {
    return std::max( cat_finisher_t::calculate_dot_refresh_duration( d, dur ), d->remains() );
  }

  void impact( action_state_t* s ) override
  {
    cat_finisher_t::impact( s );

    if ( tear && result_is_hit( s->result ) )
    {
      auto dot_total = calculate_tick_amount( s, 1.0 ) * find_dot( s->target )->remains() / tick_time( s );

      if ( p()->talent.sabertooth.ok() )
        dot_total /= 1.0 + p()->buff.sabertooth->check_value();

      tear->snapshot_and_execute( s, true, [ this, dot_total ]( action_state_t* new_ ) {
        tear->set_amount( new_, dot_total );
      } );
    }
  }

  void tick( dot_t* d ) override
  {
    cat_finisher_t::tick( d );

    p()->buff.apex_predators_craving->trigger();

    if ( p()->conduit.incessant_hunter->ok() && rng().roll( p()->conduit.incessant_hunter.percent() ) )
    {
      p()->resource_gain( RESOURCE_ENERGY,
                          p()->conduit.incessant_hunter->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_ENERGY ),
                          p()->gain.incessant_hunter );
    }
  }
};

// Primal Wrath =============================================================
// NOTE: must be defined AFTER rip_T
struct primal_wrath_t : public cat_finisher_t<>
{
  struct tear_open_wounds_t : public cat_attack_t
  {
    timespan_t rip_dur;
    double rip_mul;

    tear_open_wounds_t( druid_t* p )
      : cat_attack_t( "tear_open_wounds", p, p->find_spell( 391786 ) ),
        rip_dur( timespan_t::from_seconds( p->talent.tear_open_wounds->effectN( 1 ).base_value() ) ),
        rip_mul( p->talent.tear_open_wounds->effectN( 2 ).percent() )
    {
      background = true;
      round_base_dmg = false;
    }

    double _get_amount( const action_state_t* s ) const
    {
      auto rip = td( s->target )->dots.rip;
      auto dur = std::min( rip_dur, rip->remains() );
      auto tic = dur / rip->current_action->tick_time( rip->state );
      auto amt = rip->state->result_total;

      return tic * amt * rip_mul;
    }

    double base_da_min( const action_state_t* s ) const override { return _get_amount( s ); }
    double base_da_max( const action_state_t* s ) const override { return _get_amount( s ); }

    void impact( action_state_t* s ) override
    {
      cat_attack_t::impact( s );

      td( s->target )->dots.rip->adjust_duration( -4_s, 0_ms, -1, false );
    }
  };

  rip_t* rip;
  action_t* wounds;
  timespan_t base_dur;

  primal_wrath_t( druid_t* p, std::string_view opt ) : primal_wrath_t( p, "primal_wrath", p->talent.primal_wrath, opt )
  {}  // TODO: remove ctor overload when no longer needed

  primal_wrath_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : cat_finisher_t( n, p, s, opt ),
      wounds( nullptr ),
      base_dur( timespan_t::from_seconds( data().effectN( 2 ).base_value() ) )
  {
    aoe = -1;

    dot_name = "rip";
    // Manually set true so bloodtalons is decremented and we get proper snapshot reporting
    snapshots.bloodtalons = true;

    rip = p->get_secondary_action_n<rip_t>( "rip_primal", p->find_spell( 1079 ), "" );
    rip->dot_duration = timespan_t::from_seconds( data().effectN( 2 ).base_value() );
    rip->dual = rip->background = true;
    rip->stats = stats;
    rip->base_costs[ RESOURCE_ENERGY ] = 0;
    rip->consumes_combo_points = false;
    // mods are parsed on construction so set to false so the rip execute doesn't decrement
    debug_cast<rip_t*>( rip )->snapshots.bloodtalons = false;

    if ( p->talent.tear_open_wounds.ok() )
    {
      wounds = p->get_secondary_action<tear_open_wounds_t>( "tear_open_wounds" );
      wounds->background = true;
      add_child( wounds );
    }

    if ( p->talent.circle_of_life_and_death.ok() )
    {
      base_dur *=
          1.0 + p->query_aura_effect( p->talent.circle_of_life_and_death, A_ADD_PCT_MODIFIER, P_EFFECT_2, &data() )
                    ->percent();
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dam = cat_attack_t::composite_da_multiplier( s );

    dam *= 1.0 + cast_state( s )->combo_points;

    return dam;
  }

  void impact( action_state_t* s ) override
  {
    if ( wounds && td( s->target )->dots.rip->is_ticking() )
      wounds->execute_on_target( s->target );

    rip->snapshot_and_execute( s, true );

    cat_finisher_t::impact( s );
  }
};

// Shred ====================================================================
struct shred_t : public cat_attack_t
{
  double stealth_mul;
  double stealth_cp;

  shred_t( druid_t* p, std::string_view opt ) : shred_t( p, "shred", opt ) {}

  shred_t( druid_t* p, std::string_view n, std::string_view opt )
    : cat_attack_t( n, p, p->find_class_spell( "Shred" ), opt ), stealth_mul( 0.0 ), stealth_cp( 0.0 )
  {
    if ( p->talent.merciless_strikes.ok() )
      bleed_mul = p->talent.merciless_strikes->effectN( 2 ).percent();

    if ( p->talent.pouncing_strikes.ok() )
    {
      stealth_mul = data().effectN( 3 ).percent();
      stealth_cp = p->find_spell( 343232 )->effectN( 1 ).base_value();
    }
  }

  bool stealthed() const override
  {
    return p()->buff.berserk_cat->check() || p()->buff.incarnation_cat->check() || cat_attack_t::stealthed();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = cat_attack_t::composite_energize_amount( s );

    if ( stealthed() || p()->buff.sudden_ambush->check() )
      e += stealth_cp;

    return e;
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( hit_any_target )
    {
      p()->buff.bt_shred->trigger();

      if ( !stealthed() )
        p()->buff.sudden_ambush->expire();
    }
  }

  double composite_crit_chance_multiplier() const override
  {
    double cm = cat_attack_t::composite_crit_chance_multiplier();

    if ( stealth_mul && ( stealthed() || p()->buff.sudden_ambush->check() ) )
      cm *= 2.0;

    return cm;
  }

  double action_multiplier() const override
  {
    double m = cat_attack_t::action_multiplier();

    if ( stealth_mul && ( stealthed() || p()->buff.sudden_ambush->check() ) )
      m *= 1.0 + stealth_mul;

    return m;
  }
};

// Swipe (Cat) ====================================================================
struct swipe_cat_t : public cat_attack_t
{
  double berserk_cp;

  swipe_cat_t( druid_t* p, std::string_view opt )
    : cat_attack_t( "swipe_cat", p, p->apply_override( p->talent.swipe, p->spec.cat_form_override ), opt ),
      berserk_cp( p->spec.berserk_cat->effectN( 3 ).base_value() )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 4 ).base_value();

    if ( p->talent.merciless_strikes.ok() )
      bleed_mul = p->talent.merciless_strikes->effectN( 1 ).percent();

    if ( p->specialization() == DRUID_FERAL )
      name_str_reporting = "swipe";
  }

  bool ready() override
  {
    if ( p()->talent.brutal_slash.ok() )
      return false;

    return cat_attack_t::ready();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    auto ea = cat_attack_t::composite_energize_amount( s );

    if ( p()->buff.b_inc_cat->check() )
      ea += berserk_cp;

    return ea;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.bt_swipe->trigger();
  }
};

// Tiger's Fury =============================================================
struct tigers_fury_t : public cat_attack_t
{
  tigers_fury_t( druid_t* p, std::string_view opt ) : cat_attack_t( "tigers_fury", p, p->talent.tigers_fury, opt )
  {
    harmful = false;
    energize_type = action_energize::ON_CAST;

    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.tigers_fury->trigger();

    if ( !is_free() && p()->talent.tigers_tenacity.ok() )
      p()->buff.tigers_tenacity->trigger();
  }
};

// Thrash (Cat) =============================================================
struct thrash_cat_t : public cat_attack_t
{
  thrash_cat_t( druid_t* p, std::string_view opt )
    : thrash_cat_t( p, "thrash_cat", p->apply_override( p->talent.thrash, p->spec.cat_form_override ), opt )
  {}

  thrash_cat_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : cat_attack_t( n, p, s, opt )
  {
    aoe    = -1;
    radius = data().effectN( 1 ).resource();

    // For some reason this is in a different spell
    energize_amount   = p->find_spell( 211141 )->effectN( 1 ).base_value();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type     = action_energize::ON_HIT;

    dot_name = "thrash_cat";

    if ( p->specialization() == DRUID_FERAL )
      name_str_reporting = "thrash";
  }

  void trigger_dot( action_state_t* s ) override
  {
    auto bear_thrash = td( s->target )->dots.thrash_bear;

    // Cat thrash will not overwrite bear thrash if bear thrash is on cooldown and has more than one stack
    if ( p()->cooldown.thrash_bear->down() && bear_thrash->current_stack() > 1 )
      return;

    bear_thrash->cancel();

    cat_attack_t::trigger_dot( s );
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.bt_thrash->trigger();
  }
};

}  // end namespace cat_attacks

namespace bear_attacks
{
// ==========================================================================
// Druid Bear Attack
// ==========================================================================
struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
  bool proc_gore;

  bear_attack_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                 std::string_view opt = {} )
    : base_t( n, p, s ), proc_gore( false )
  {
    parse_options( opt );

    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
      ap_type = attack_power_type::NO_WEAPON;
  }

  void consume_resource() override
  {
    base_t::consume_resource();

    if ( is_free() || !last_resource_cost )
      return;

    if ( resource_current == RESOURCE_RAGE )
    {
      if ( p()->talent.after_the_wildfire.ok() )
      {
        auto r_cap = p()->talent.after_the_wildfire->effectN( 2 ).base_value();

        p()->after_the_wildfire_counter += last_resource_cost;

        if ( p()->after_the_wildfire_counter >= r_cap )
        {
          p()->after_the_wildfire_counter -= r_cap;
          p()->active.after_the_wildfire_heal->execute();
        }
      }

      if ( p()->talent.ursocs_guidance.ok() )
      {
        auto cdr = timespan_t::from_seconds( last_resource_cost / -20 );

        if ( p()->talent.incarnation_bear.ok() )
          p()->cooldown.incarnation->adjust( cdr );
        else
          p()->cooldown.berserk_cat->adjust( cdr );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) && proc_gore )
      trigger_gore();
  }
};  // end bear_attack_t

// Berserk (Bear) ===========================================================
struct berserk_bear_base_t : public bear_attack_t
{
  buff_t* buff;

  berserk_bear_base_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt )
    : bear_attack_t( n, p, s, opt ), buff( p->buff.berserk_bear )
  {
    harmful   = false;
    form_mask = BEAR_FORM;
    autoshift = p->active.shift_to_bear;
  }

  void execute() override
  {
    bear_attack_t::execute();

    buff->trigger();
  }
};

struct berserk_bear_t : public berserk_bear_base_t
{
  berserk_bear_t( druid_t* p, std::string_view opt )
    : berserk_bear_base_t( "berserk_bear", p, p->spec.berserk_bear, opt )
  {
    name_str_reporting = "berserk";
  }

  bool ready() override
  {
    return p()->talent.incarnation_bear.ok() ? false : berserk_bear_base_t::ready();
  }
};

struct incarnation_bear_t : public berserk_bear_base_t
{
  incarnation_bear_t( druid_t* p, std::string_view opt )
    : berserk_bear_base_t( "incarnation_guardian_of_ursoc", p, p->spec.incarnation_bear, opt )
  {
    buff = p->buff.incarnation_bear;
  }
};

// Brambles =================================================================
struct brambles_t : public bear_attack_t
{
  brambles_t( druid_t* p ) : bear_attack_t( "brambles", p, p->find_spell( 203958 ) )
  {
    // Ensure reflect scales with damage multipliers
    snapshot_flags |= STATE_VERSATILITY | STATE_TGT_MUL_DA | STATE_MUL_DA;
    background = proc = may_miss = true;
  }
};

struct brambles_pulse_t : public bear_attack_t
{
  brambles_pulse_t( druid_t* p, std::string_view n ) : bear_attack_t( n, p, p->find_spell( 213709 ) )
  {
    background = dual = true;
    aoe               = -1;
  }
};

// Bristling Fur Spell ======================================================
struct bristling_fur_t : public bear_attack_t
{
  bristling_fur_t( druid_t* player, std::string_view opt )
    : bear_attack_t( "bristling_fur", player, player->talent.bristling_fur, opt )
  {
    harmful     = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    bear_attack_t::execute();

    p()->buff.bristling_fur->trigger();
  }
};

// Growl ====================================================================
struct growl_t : public bear_attack_t
{
  growl_t( druid_t* player, std::string_view opt )
    : bear_attack_t( "growl", player, player->find_class_spell( "Growl" ), opt )
  {
    ignore_false_positive = use_off_gcd = true;
    may_miss = may_parry = may_dodge = may_block = false;
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
      target->taunt( player );

    bear_attack_t::impact( s );
  }
};

// Incapacitating Roar ======================================================
struct incapacitating_roar_t : public bear_attack_t
{
  incapacitating_roar_t( druid_t* p, std::string_view opt )
    : bear_attack_t( "incapacitating_roar", p, p->talent.incapacitating_roar, opt )
  {
    harmful = false;

    form_mask = BEAR_FORM;
    autoshift = p->active.shift_to_bear;
  }
};

// Ironfur ==================================================================
struct ironfur_t : public bear_attack_t
{
  ironfur_t( druid_t* p, std::string_view opt ) : ironfur_t( p, "ironfur", p->talent.ironfur, opt ) {}

  ironfur_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : bear_attack_t( n, p, s, opt )
  {
    use_off_gcd = true;
    harmful = may_miss = may_parry = may_dodge = false;
  }

  timespan_t composite_buff_duration()
  {
    timespan_t bd = p()->buff.ironfur->buff_duration();

    if ( p()->buff.guardian_of_elune->check() )
      bd += p()->buff.guardian_of_elune->data().effectN( 1 ).time_value();

    return bd;
  }

  void execute() override
  {
    bear_attack_t::execute();

    int stack = 1;

    // TODO: does guardian of elune also apply to the extra application from layered mane?
    if ( p()->talent.layered_mane.ok() && rng().roll( p()->talent.layered_mane->effectN( 1 ).percent() ) )
      stack++;

    p()->buff.ironfur->trigger( stack, composite_buff_duration() );
    p()->buff.guardian_of_elune->expire();
  }
};

// Mangle ===================================================================
struct mangle_t : public bear_attack_t
{
  struct swiping_mangle_t : public druid_residual_action_t<bear_attack_t>
  {
    swiping_mangle_t( druid_t* p, std::string_view n ) : base_t( n, p, p->find_spell( 395942 ) )
    {
      auto set_ = p->sets->set( DRUID_GUARDIAN, T29, B2 );

      aoe = -1;
      reduced_aoe_targets = set_->effectN( 2 ).base_value();
      name_str_reporting = "swiping_mangle";

      residual_mul = set_->effectN( 1 ).percent();
    }

    double base_da_min( const action_state_t* s ) const override { return get_amount( s ); }
    double base_da_max( const action_state_t* s ) const override { return get_amount( s ); }

    std::vector<player_t*>& target_list() const override
    {
      target_cache.is_valid = false;

      std::vector<player_t*>& tl = base_t::target_list();

      tl.erase( std::remove( tl.begin(), tl.end(), target ), tl.end() );

      return tl;
    }
  };

  struct bloody_healing_t : public heals::druid_heal_t
  {
    bloody_healing_t( druid_t* p ) : heals::druid_heal_t( "bloody_healing", p, p->find_spell( 394504 ) ) {}
  };

  swiping_mangle_t* swiping;
  action_t* healing;
  int inc_targets;

  mangle_t( druid_t* p, std::string_view opt ) : mangle_t( p, "mangle", opt ) {}

  mangle_t( druid_t* p, std::string_view n, std::string_view opt )
    : bear_attack_t( n, p, p->find_class_spell( "Mangle" ), opt ),
      swiping( nullptr ),
      healing( nullptr ),
      inc_targets( 0 )
  {
    if ( p->talent.mangle.ok() )
      bleed_mul = data().effectN( 3 ).percent();

    energize_amount += p->talent.soul_of_the_forest_bear->effectN( 1 ).resource( RESOURCE_RAGE );

    if ( p->talent.incarnation_bear.ok() )
    {
      inc_targets = as<int>(
          p->query_aura_effect( p->spec.incarnation_bear, A_ADD_FLAT_MODIFIER, P_TARGET, s_data )->base_value() );
    }

    if ( p->sets->has_set_bonus( DRUID_GUARDIAN, T29, B2 ) )
    {
      auto suf = get_suffix( name_str, "mangle" );
      swiping = p->get_secondary_action_n<swiping_mangle_t>( "swiping_mangle" + suf );
      swiping->background = true;
      add_child( swiping );
    }

    if ( p->sets->has_set_bonus( DRUID_GUARDIAN, T29, B4 ) )
      healing = p->get_secondary_action<bloody_healing_t>( "bloody_healing" );
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double em = bear_attack_t::composite_energize_amount( s );

    em += p()->buff.gore->check_value();

    return em;
  }

  int n_targets() const override
  {
    auto n = bear_attack_t::n_targets();

    if ( p()->buff.incarnation_bear->check() )
      n += inc_targets;

    return n;
  }

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( is_free_proc() )
      return;

    if ( swiping && p()->buff.gore->check() && s->result_amount > 0 && s->chain_target == 0 &&
         !swiping->target_list().empty() )
    {
      swiping->snapshot_and_execute( s, false, [ this, s ]( action_state_t* new_ ) {
        swiping->set_amount( new_, s->result_amount );
      } );
    }
  }

  void execute() override
  {
    // this is proc'd before the cast and thus benefits the cast
    if ( p()->sets->has_set_bonus( DRUID_GUARDIAN, T29, B2 ) )
      p()->buff.overpowering_aura->trigger();

    if ( p()->sets->has_set_bonus( DRUID_GUARDIAN, T29, B4 ) )
      healing->execute();

    bear_attack_t::execute();

    if ( !hit_any_target )
      return;

    if ( !is_free_proc() )
      p()->buff.gore->expire();

    p()->buff.vicious_cycle_mangle->expire();

    if ( p()->talent.gory_fur.ok() )
      p()->buff.gory_fur->trigger();

    if ( p()->talent.vicious_cycle.ok() )
      p()->buff.vicious_cycle_maul->trigger();

    if ( p()->talent.guardian_of_elune.ok() )
      p()->buff.guardian_of_elune->trigger();

    if ( p()->conduit.savage_combatant->ok() )
      p()->buff.savage_combatant->trigger();
  }
};

// Maul =====================================================================
struct maul_t : public bear_attack_t
{
  maul_t( druid_t* p, std::string_view opt ) : bear_attack_t( "maul", p, p->talent.maul, opt )
  {
    proc_gore = true;
  }

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->buff.tooth_and_claw->up() )
    {
      td( s->target )->debuff.tooth_and_claw->trigger();
      p()->buff.tooth_and_claw->decrement();
    }

    if ( p()->talent.ursocs_fury.ok() )
      p()->buff.ursocs_fury->trigger( 1, s->result_amount * p()->talent.ursocs_fury->effectN( 1 ).percent() );
  }

  void execute() override
  {
    bear_attack_t::execute();

    if ( !hit_any_target )
      return;

    p()->buff.savage_combatant->expire();
    p()->buff.vicious_cycle_maul->decrement();

    if ( p()->talent.vicious_cycle.ok() )
      p()->buff.vicious_cycle_mangle->trigger();
  }
};

// Pulverize ================================================================
struct pulverize_t : public bear_attack_t
{
  int consume;

  pulverize_t( druid_t* p, std::string_view opt ) : pulverize_t( p, "pulverize", p->talent.pulverize, opt ) {}

  pulverize_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : bear_attack_t( n, p, s, opt ), consume( as<int>( data().effectN( 3 ).base_value() ) )
  {}

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( !is_free() )
      td( s->target )->dots.thrash_bear->decrement( consume );

    td( s->target )->debuff.pulverize->trigger();
  }

  bool target_ready( player_t* t ) override
  {
    // Call bear_attack_t::ready() first for proper targeting support.
    // Hardcode stack max since it may not be set if this code runs before Thrash is cast.
    return bear_attack_t::target_ready( t ) && ( is_free() || td( t )->dots.thrash_bear->current_stack() >= consume );
  }
};

// Rage of the Sleeper ======================================================
struct rage_of_the_sleeper_reflect_t : public bear_attack_t
{
  rage_of_the_sleeper_reflect_t( druid_t* p )
    : bear_attack_t( "rage_of_the_sleeper_reflect", p, p->find_spell( 219432 ) )
  {
    background = dual = true;
  }
};

struct rage_of_the_sleeper_t : public bear_attack_t
{
  rage_of_the_sleeper_t( druid_t* p, std::string_view opt )
    : bear_attack_t( "rage_of_the_sleeper", p, p->talent.rage_of_the_sleeper, opt )
  {
    harmful = may_miss = may_parry = may_dodge = false;

    if ( p->active.rage_of_the_sleeper_reflect )
      p->active.rage_of_the_sleeper_reflect->stats = stats;
  }

  void execute() override
  {
    bear_attack_t::execute();

    p()->buff.rage_of_the_sleeper->trigger();
  }
};

// Swipe (Bear) =============================================================
struct swipe_bear_t : public bear_attack_t
{
  swipe_bear_t( druid_t* p, std::string_view opt )
    : bear_attack_t( "swipe_bear", p, p->apply_override( p->talent.swipe, p->spec.bear_form_override ), opt )
  {
    aoe = -1;
    // target hit data stored in cat swipe
    reduced_aoe_targets = p->apply_override( p->talent.swipe, p->spec.cat_form_override )->effectN( 4 ).base_value();
    proc_gore = true;

    if ( p->specialization() == DRUID_GUARDIAN )
      name_str_reporting = "swipe";
  }
};

// Thrash (Bear) ============================================================
struct thrash_bear_t : public bear_attack_t
{
  struct thrash_bear_dot_t : public bear_attack_t
  {
    double bf_energize;

    thrash_bear_dot_t( druid_t* p, std::string_view n )
      : bear_attack_t( n, p, p->spec.thrash_bear_dot ), bf_energize( 0.0 )
    {
      dual = background = true;
      aoe = -1;

      dot_name = "thrash_bear";

      // energize amount is not stored in talent spell
      if ( p->talent.blood_frenzy.ok() )
        bf_energize = p->find_spell( 203961 )->effectN( 1 ).resource( RESOURCE_RAGE );
    }

    void trigger_dot( action_state_t* s ) override
    {
      td( s->target )->dots.thrash_cat->cancel();

      bear_attack_t::trigger_dot( s );
    }

    void tick( dot_t* d ) override
    {
      bear_attack_t::tick( d );

      p()->resource_gain( RESOURCE_RAGE, bf_energize, p()->gain.blood_frenzy );
    }
  };

  action_t* dot;

  thrash_bear_t( druid_t* p, std::string_view opt )
    : thrash_bear_t( p, "thrash_bear", p->apply_override( p->talent.thrash, p->spec.bear_form_override ), opt )
  {}

  thrash_bear_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : bear_attack_t( n, p, s, opt )
  {
    aoe = -1;
    proc_gore = true;

    dot = p->get_secondary_action_n<thrash_bear_dot_t>( name_str + "_dot" );
    dot->stats = stats;
    dot->radius = radius;

    dot_name = "thrash_bear";

    if ( p->specialization() == DRUID_GUARDIAN )
      name_str_reporting = "thrash";
  }

  bool has_amount_result() const override { return dot->has_amount_result(); }

  void execute() override
  {
    bear_attack_t::execute();

    dot->target = target;
    dot->schedule_execute();

    if ( p()->talent.flashing_claws.ok() && rng().roll( p()->talent.flashing_claws->effectN( 1 ).percent() ) )
      make_event( *sim, 500_ms, [ this ]() { p()->active.thrash_bear_flashing->execute_on_target( target ); } );
  }

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( p()->talent.earthwarden.ok() && result_is_hit( s->result ) )
      p()->buff.earthwarden->trigger();
  }
};
} // end namespace bear_attacks

namespace heals
{
// After the Wildfire =======================================================
struct after_the_wildfire_heal_t : public druid_heal_t
{
  after_the_wildfire_heal_t( druid_t* p ) : druid_heal_t( "after_the_wildfire", p, p->find_spell( 371982 ) )
  {
    aoe = -1;
  }
};

// Cenarion Ward ============================================================
struct cenarion_ward_t : public druid_heal_t
{
  struct cenarion_ward_hot_t : public druid_heal_t
  {
    cenarion_ward_hot_t( druid_t* p )
      : druid_heal_t( "cenarion_ward_hot", p, p->talent.cenarion_ward->effectN( 1 ).trigger() )
    {
      background = true;

      dot_name = "cenarion_ward";
    }
  };

  cenarion_ward_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "cenarion_ward", p, p->talent.cenarion_ward, opt )
  {
    auto hot = p->get_secondary_action<cenarion_ward_hot_t>( "cenarion_ward_hot" );

    p->buff.cenarion_ward->set_stack_change_callback( [ hot ]( buff_t* b, int, int new_ ) {
      if ( !new_ )
        hot->execute_on_target( b->player );
    } );
  }

  void execute() override
  {
    druid_heal_t::execute();

    p()->buff.cenarion_ward->trigger();
  }
};

// Efflorescence ============================================================
struct efflorescence_t : public druid_heal_t
{
  struct spring_blossoms_t : public druid_heal_t
  {
    spring_blossoms_t( druid_t* p ) : druid_heal_t( "spring_blossoms", p, p->find_spell( 207386 ) ) {}
  };

  struct efflorescence_tick_t : public druid_heal_t
  {
    action_t* spring;

    efflorescence_tick_t( druid_t* p )
      : druid_heal_t( "efflorescence_tick", p, p->find_spell( 81269 ) ), spring( nullptr )
    {
      background = dual = ground_aoe = true;
      aoe = 3;  // hardcoded
      name_str_reporting = "efflorescence";

      if ( p->talent.spring_blossoms.ok() )
        spring = p->get_secondary_action<spring_blossoms_t>( "spring_blossoms" );
    }

    std::vector<player_t*>& target_list() const override
    {
      // get the full list
      auto& tl = druid_heal_t::target_list();

      rng().shuffle( tl.begin(), tl.end() );

      // sort by targets without spring blossoms
      if ( spring )
      {
        range::sort( tl, [ this ]( player_t* l, player_t* r ) {
          return td( l )->hots.spring_blossoms->is_ticking() < td( r )->hots.spring_blossoms->is_ticking();
        } );
      }
      else
      {
        range::sort( tl, []( const player_t* l, const player_t* r ) {
          return l->health_percentage() < r->health_percentage();
        } );
      }

      return tl;
    }

    void execute() override
    {
      // force re-evaluation of targets every tick
      target_cache.is_valid = false;

      druid_heal_t::execute();
    }


    void impact( action_state_t* s ) override
    {
      druid_heal_t::impact( s );

      if ( spring )
        spring->execute_on_target( s->target );
    }
  };

  action_t* heal;
  timespan_t duration;
  timespan_t period;

  efflorescence_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "efflorescence", p, p->talent.efflorescence, opt )
  {
    auto efflo_data = p->find_spell( 81262 );
    duration = efflo_data->duration();
    period = p->query_aura_effect( efflo_data, A_PERIODIC_DUMMY )->period();

    heal = p->get_secondary_action<efflorescence_tick_t>( "efflorescence_tick" );
    heal->stats = stats;
  }

  void execute() override
  {
    druid_heal_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( target )
                                        .hasted( ground_aoe_params_t::hasted_with::SPELL_HASTE )
                                        .pulse_time( period )
                                        .duration( duration )
                                        .action( heal ) );
  }
};

// Elune's Favored ==========================================================
struct elunes_favored_heal_t : public druid_heal_t
{
  elunes_favored_heal_t( druid_t* p ) : druid_heal_t( "elunes_favored", p, p->find_spell( 370602 ) )
  {
    background = true;
  }
};

// Frenzied Regeneration ====================================================
struct frenzied_regeneration_t : public druid_heal_t
{
  cooldown_t* dummy_cd;
  cooldown_t* orig_cd;
  double goe_mul;

  frenzied_regeneration_t( druid_t* p, std::string_view opt )
    : frenzied_regeneration_t( p, "frenzied_regeneration", p->talent.frenzied_regeneration, opt )
  {}

  frenzied_regeneration_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : druid_heal_t( n, p, s, opt ), dummy_cd( p->get_cooldown( "dummy_cd" ) ), orig_cd( cooldown ), goe_mul( 0.0 )
  {
    target = p;

    if ( p->talent.guardian_of_elune.ok() )
      goe_mul = p->buff.guardian_of_elune->data().effectN( 2 ).percent();
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags = STATE_MUL_TA | STATE_VERSATILITY | STATE_MUL_PERSISTENT | STATE_TGT_MUL_TA;
  }

  void execute() override
  {
    if ( p()->talent.layered_mane.ok() && rng().roll( p()->talent.layered_mane->effectN( 2 ).percent() ) )
      cooldown = dummy_cd;

    druid_heal_t::execute();

    cooldown = orig_cd;

    p()->buff.guardian_of_elune->expire();
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = druid_heal_t::composite_persistent_multiplier( s );

    if ( p()->buff.guardian_of_elune->check() )
      pm *= 1.0 + goe_mul;

    return pm;
  }
};

// Flourish =================================================================
struct flourish_t : public druid_heal_t
{
  timespan_t ext;

  flourish_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "flourish", p, p->talent.flourish, opt ),
      ext( timespan_t::from_seconds( data().effectN( 1 ).base_value() ) )
  {
    dot_duration = 0_ms;
  }

  void execute() override
  {
    druid_heal_t::execute();

    p()->buff.flourish->trigger();

    for ( const auto& t : sim->player_non_sleeping_list )
    {
      auto hots = td( t )->hots;
      hots.cenarion_ward->adjust_duration( ext, 0_ms, -1, false );
      hots.cultivation->adjust_duration( ext, 0_ms, -1, false );
      hots.germination->adjust_duration( ext, 0_ms, -1, false );
      hots.lifebloom->adjust_duration( ext, 0_ms, -1, false );
      hots.regrowth->adjust_duration( ext, 0_ms, -1, false );
      hots.rejuvenation->adjust_duration( ext, 0_ms, -1, false );
      hots.spring_blossoms->adjust_duration( ext, 0_ms, -1, false );
      hots.wild_growth->adjust_duration( ext, 0_ms, -1, false );
    }
  }
};

// Ironbark =================================================================
struct ironbark_t : public druid_heal_t
{
  ironbark_t( druid_t* p, std::string_view opt ) : druid_heal_t( "ironbark", p, p->talent.ironbark, opt ) {}

  void impact( action_state_t* s ) override
  {
    druid_heal_t::impact( s );

    td( s->target )->buff.ironbark->trigger();
  }
};

// Lifebloom ================================================================
struct lifebloom_t : public druid_heal_t
{
  struct lifebloom_bloom_t : public druid_heal_t
  {
    lifebloom_bloom_t( druid_t* p ) : druid_heal_t( "lifebloom_bloom", p, p->find_spell( 33778 ) )
    {
      background = dual = true;
    }
  };

  lifebloom_bloom_t* bloom;

  lifebloom_t( druid_t* p, std::string_view opt ) : druid_heal_t( "lifebloom", p, p->talent.lifebloom, opt )
  {
    bloom = p->get_secondary_action<lifebloom_bloom_t>( "lifebloom_bloom" );
    bloom->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    // Cancel dot on all targets other than the one we impact on
    for ( const auto& t : sim->player_non_sleeping_list )
    {
      if ( t == target )
        continue;

      auto d = get_dot( t );
      sim->print_debug( "{} fades from {}", *d, *t );
      d->reset();  // we don't want last_tick() because there is no bloom on target swap
    }

    auto lb = get_dot( target );
    if ( lb->is_ticking() && lb->remains() <= dot_duration * 0.3 )
      bloom->execute_on_target( target );

    druid_heal_t::impact( s );
  }

  void trigger_dot( action_state_t* s ) override
  {
    auto lb = find_dot( s->target );
    if ( lb && lb->remains() <= composite_dot_duration( lb->state ) * 0.3 )
      bloom->execute_on_target( s->target );

    druid_heal_t::trigger_dot( s );
  }

  void tick( dot_t* d ) override
  {
    druid_heal_t::tick( d );

    p()->buff.clearcasting_tree->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    if ( !d->state->target->is_sleeping() )  // Prevent crash at end of simulation
      bloom->execute_on_target( d->target );

    druid_heal_t::last_tick( d );
  }
};

// Nature's Cure ============================================================
struct natures_cure_t : public druid_heal_t
{
  natures_cure_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "natures_cure", p, p->find_specialization_spell( "Nature's Cure" ), opt )
  {}
};

// Nature's Guardian ========================================================
struct natures_guardian_t : public druid_heal_t
{
  natures_guardian_t( druid_t* p ) : druid_heal_t( "natures_guardian", p, p->find_spell( 227034 ) )
  {
    background = true;
    target = p;
  }

  void init() override
  {
    druid_heal_t::init();

    // Not affected by multipliers of any sort.
    snapshot_flags &= ~STATE_NO_MULTIPLIER;
  }
};

// Nature's Swiftness =======================================================
struct natures_swiftness_t : public druid_heal_t
{
  natures_swiftness_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "natures_swiftness", p, p->talent.natures_swiftness, opt )
  {}

  timespan_t cooldown_duration() const override
  {
    return 0_ms;  // cooldown handled by buff.natures_swiftness
  }

  void execute() override
  {
    druid_heal_t::execute();

    p()->buff.natures_swiftness->trigger();
  }

  bool ready() override
  {
    if ( p()->buff.natures_swiftness->check() )
      return false;

    return druid_heal_t::ready();
  }
};

// Nourish ==================================================================
struct nourish_t : public druid_heal_t
{
  nourish_t( druid_t* p, std::string_view opt ) : druid_heal_t( "nourish", p, p->talent.nourish, opt ) {}

  double harmony_multiplier( player_t* t ) const override
  {
    return druid_heal_t::harmony_multiplier( t ) * 3.0;  // multiplier not in any spell data
  }
};

// Regrowth =================================================================
struct regrowth_t : public druid_heal_t
{
  timespan_t gcd_add;
  double bonus_crit;
  double sotf_mul;
  bool is_direct;

  regrowth_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "regrowth", p, p->find_class_spell( "Regrowth" ), opt ),
      gcd_add( p->query_aura_effect( p->spec.cat_form_passive, A_ADD_FLAT_MODIFIER, P_GCD, &data() )->time_value() ),
      bonus_crit( p->talent.improved_regrowth->effectN( 1 ).percent() ),
      sotf_mul( p->buff.soul_of_the_forest_tree->data().effectN( 1 ).percent() ),
      is_direct( false )
  {
    form_mask = NO_FORM | MOONKIN_FORM;
    may_autounshift = true;

    affected_by.soul_of_the_forest = true;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_heal_t::gcd();

    if ( p()->buff.cat_form->check() )
      g += gcd_add;

    return g;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buff.incarnation_tree->check() )
      return 0_ms;

    return druid_heal_t::execute_time();
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double tcc = druid_heal_t::composite_target_crit_chance( t );

    if ( is_direct && td( t )->hots.regrowth->is_ticking() )
      tcc += bonus_crit;

    return tcc;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pm = druid_heal_t::composite_persistent_multiplier( s );

    if ( p()->buff.soul_of_the_forest_tree->check() )
      pm *= 1.0 + sotf_mul;

    return pm;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    auto ctm = druid_heal_t::composite_target_multiplier( t );

    if ( t == player )
      ctm *= 1.0 + p()->talent.innate_resolve->effectN( 1 ).percent();

    return ctm;
  }

  bool check_form_restriction() override
  {
    if ( p()->buff.predatory_swiftness->check() )
      return true;

    return druid_heal_t::check_form_restriction();
  }

  void execute() override
  {
    is_direct = true;
    druid_heal_t::execute();
    is_direct = false;

    p()->buff.predatory_swiftness->expire();

    if ( target == p() && p()->talent.protective_growth.ok() )
      p()->buff.protective_growth->trigger();

    if ( !is_free() )
    {
      p()->buff.natures_swiftness->expire();
      p()->buff.protector_of_the_pack_regrowth->expire();
    }
  }

  void last_tick( dot_t* d ) override
  {
    druid_heal_t::last_tick( d );

    if ( d->target == p() )
      p()->buff.protective_growth->expire();
  }
};

// Rejuvenation =============================================================
struct rejuvenation_base_t : public druid_heal_t
{
  struct cultivation_t : public druid_heal_t
  {
    cultivation_t( druid_t* p ) : druid_heal_t( "cultivation", p, p->find_spell( 200389 ) ) {}
  };

  action_t* cult_hot;
  double cult_pct;  // NOTE: this is base_value, NOT percent()
  double sotf_mul;

  rejuvenation_base_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt )
    : druid_heal_t( n, p, s, opt ),
      cult_hot( nullptr ),
      cult_pct( p->talent.cultivation->effectN( 1 ).base_value() ),
      sotf_mul( p->buff.soul_of_the_forest_tree->data().effectN( 1 ).percent() )
  {
    apply_affecting_aura( p->talent.improved_rejuvenation );
    apply_affecting_aura( p->talent.germination );

    affected_by.soul_of_the_forest = true;

    if ( p->talent.cultivation.ok() )
      cult_hot = p->get_secondary_action<cultivation_t>( "cultivation" );
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pm = druid_heal_t::composite_persistent_multiplier( s );

    if ( p()->buff.soul_of_the_forest_tree->check() )
      pm *= 1.0 + sotf_mul;

    return pm;
  }

  void impact( action_state_t* s ) override
  {
    if ( !td( target )->hots.rejuvenation->is_ticking() )
      p()->buff.abundance->increment();

    druid_heal_t::impact( s );
  }

  void tick( dot_t* d ) override
  {
    druid_heal_t::tick( d );

    if ( cult_hot && d->target->health_percentage() <= cult_pct )
      cult_hot->execute_on_target( d->target );
  }

  void last_tick( dot_t* d ) override
  {
    druid_heal_t::last_tick( d );

    p()->buff.abundance->decrement();
  }
};

struct rejuvenation_t : public rejuvenation_base_t
{
  struct germination_t : public rejuvenation_base_t
  {
    germination_t( druid_t* p, std::string_view opt )
      : rejuvenation_base_t( "germination", p, p->find_spell( 155777 ), opt )
    {}
  };

  action_t* germination;

  rejuvenation_t( druid_t* p, std::string_view opt )
    : rejuvenation_base_t( "rejuvenation", p, p->talent.rejuvenation, opt ), germination( nullptr )
  {
    if ( p->talent.germination.ok() )
      germination = p->get_secondary_action<germination_t>( "germination", opt );
  }

  void execute() override
  {
    if ( p()->talent.germination.ok() )
    {
      auto hots = td( target )->hots;

      if ( ( hots.rejuvenation->is_ticking() && !hots.germination->is_ticking() ) ||
           ( hots.germination->remains() < hots.rejuvenation->remains() ) )
      {
        germination->execute_on_target( target );
        return;
      }
    }

    druid_heal_t::execute();
  }
};

// Remove Corruption ========================================================
struct remove_corruption_t : public druid_heal_t
{
  remove_corruption_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "remove_corruption", p, p->talent.remove_corruption, opt )
  {}
};

// Renewal ==================================================================
struct renewal_t : public druid_heal_t
{
  renewal_t( druid_t* p, std::string_view opt ) : druid_heal_t( "renewal", p, p->talent.renewal, opt ) {}

  void execute() override
  {
    base_dd_min = p()->resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    druid_heal_t::execute();
  }
};

// Swiftmend ================================================================
struct swiftmend_t : public druid_heal_t
{
  swiftmend_t( druid_t* p, std::string_view opt ) : druid_heal_t( "swiftmend", p, p->talent.swiftmend, opt ) {}

  bool target_ready( player_t* t ) override
  {
    auto hots = td( t )->hots;

    if ( hots.regrowth->is_ticking() || hots.wild_growth->is_ticking() || hots.rejuvenation->is_ticking() )
      return druid_heal_t::target_ready( t );

    return false;
  }

  void execute() override
  {
    druid_heal_t::execute();

    if ( p()->talent.soul_of_the_forest_tree.ok() )
      p()->buff.soul_of_the_forest_tree->trigger();
  }

  void impact( action_state_t* s ) override
  {
    auto t_td = td( s->target );

    if ( t_td->hots.regrowth->is_ticking() )
      t_td->hots.regrowth->cancel();
    else if ( t_td->hots.wild_growth->is_ticking() )
      t_td->hots.wild_growth->cancel();
    else if ( t_td->hots.rejuvenation->is_ticking() )
      t_td->hots.rejuvenation->cancel();
    else
      sim->error( "Swiftmend impact with no HoT ticking" );

    druid_heal_t::impact( s );
  }
};

// Tranquility ==============================================================
struct tranquility_t : public druid_heal_t
{
  struct tranquility_tick_t : public druid_heal_t
  {
    tranquility_tick_t( druid_t* p )
      : druid_heal_t( "tranquility_tick", p, p->talent.tranquility->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = -1;
    }
  };

  tranquility_t( druid_t* p, std::string_view opt ) : druid_heal_t( "tranquility", p, p->talent.tranquility, opt )
  {
    channeled = true;

    tick_action = p->get_secondary_action<tranquility_tick_t>( "tranquility_tick" );
    tick_action->stats = stats;
  }

  void init() override
  {
    druid_heal_t::init();

    // necessary because action_t::init() sets all tick_action::direct_tick to true
    tick_action->direct_tick = false;
  }
};

// Wild Growth ==============================================================
// The spellpower coefficient c of a tick of WG is given by:
//   c(t) = 0.175 - 0.07 * t / D
// where:
//   t = time of tick = current_time - start_time
//   D = full duration of WG
struct wild_growth_t : public druid_heal_t
{
  double decay_coeff;
  double sotf_mul;
  int inc_mod;

  wild_growth_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "wild_growth", p, p->talent.wild_growth, opt ),
      decay_coeff( 0.07 * ( 1.0 - p->talent.unstoppable_growth->effectN( 1 ).percent() ) ),
      sotf_mul( p->buff.soul_of_the_forest_tree->data().effectN( 2 ).percent() ),
      inc_mod( as<int>( p->talent.incarnation_tree->effectN( 3 ).base_value() ) )
  {
    aoe = as<int>( data().effectN( 2 ).base_value() ) +
          as<int>( p->talent.improved_wild_growth->effectN( 1 ).base_value() );

    // '0-tick' coeff, also unknown if this is hard set to 0.175 or based on a formula as below
    spell_power_mod.tick += decay_coeff / 2.0;

    affected_by.soul_of_the_forest = true;
  }

  double spell_tick_power_coefficient( const action_state_t* s ) const override
  {
    auto c = druid_heal_t::spell_tick_power_coefficient( s );
    auto dot = find_dot( s->target );

    c -= decay_coeff * ( 1.0 - dot->remains() / dot->duration() );

    return c;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pm = druid_heal_t::composite_persistent_multiplier( s );

    if ( p()->buff.soul_of_the_forest_tree->check() )
      pm *= 1.0 + sotf_mul;

    return pm;
  }

  int n_targets() const override
  {
    int n = druid_heal_t::n_targets();

    if ( n && p()->buff.incarnation_tree->check() )
      n += inc_mod;

    return n;
  }
};

// Ysera's Gift =============================================================
struct yseras_gift_t : public druid_heal_t
{
  yseras_gift_t( druid_t* p ) : druid_heal_t( "yseras_gift", p, p->find_spell( 145109 ) )
  {
    background = dual = true;
    base_pct_heal = p->talent.yseras_gift->effectN( 1 ).percent();
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_VERSATILITY;  // Is not affected by versatility.
  }

  void execute() override
  {
    if ( p()->health_percentage() < 100 )
      target = p();
    else
      target = smart_target();

    druid_heal_t::execute();
  }
};

// Overgrowth ===============================================================
// NOTE: this must come last since it requires other spell definitions
struct overgrowth_t : public druid_heal_t
{
  std::vector<action_t*> spell_list;

  overgrowth_t( druid_t* p, std::string_view opt ) : druid_heal_t( "overgrowth", p, p->talent.overgrowth, opt )
  {
    get_overgrowth_action<lifebloom_t>( "lifebloom" );
    get_overgrowth_action<rejuvenation_t>( "rejuvenation" );
    get_overgrowth_action<regrowth_t>( "regrowth" )
      ->base_dd_multiplier = 0.0;
    get_overgrowth_action<wild_growth_t>( "wild_growth" )
      ->aoe = 0;
  }

  template <typename T>
  T* get_overgrowth_action( std::string n )
  {
    auto a = p()->get_secondary_action_n<T>( n + "_overgrowth" );
    a->name_str_reporting = n;
    a->dot_name = n;
    add_child( a );
    spell_list.push_back( a );
    return a;
  }

  void execute() override
  {
    druid_heal_t::execute();

    for ( const auto& a : spell_list )
      a->execute_on_target( target );
  }
};
}  // end namespace heals

namespace spells
{
// ==========================================================================
// Druid Spells
// ==========================================================================

// Adaptive Swarm ===========================================================
struct adaptive_swarm_t : public druid_spell_t
{
  struct swarm_target_t
  {
    player_t* player;
    event_t* event;

    swarm_target_t( player_t* p, event_t* e = nullptr ) : player( p ), event( e ) {}
    swarm_target_t() : player(), event() {}

    operator player_t*() { return player; }
    operator event_t*() { return event; }
    bool operator !() { return !player && !event; }
  };

  struct adaptive_swarm_state_t : public action_state_t
  {
  private:
    int default_stacks;

  public:
    swarm_target_t swarm_target;
    double range;
    int stacks;
    bool jump;

    adaptive_swarm_state_t( action_t* a, player_t* t )
      : action_state_t( a, t ),
        default_stacks( as<int>( debug_cast<druid_t*>( a->player )->talent.adaptive_swarm->effectN( 1 ).base_value() ) ),
        range( 0.0 ),
        stacks( 0 ),
        jump( false )
    {}

    void initialize() override
    {
      action_state_t::initialize();

      swarm_target = {};
      range = 0.0;
      stacks = default_stacks;
      jump = false;
    }

    void copy_state( const action_state_t* s ) override
    {
      action_state_t::copy_state( s );
      auto swarm_s = debug_cast<const adaptive_swarm_state_t*>( s );

      swarm_target = swarm_s->swarm_target;
      range = swarm_s->range;
      stacks = swarm_s->stacks;
      jump = swarm_s->jump;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );

      s << " range=" << range;
      s << " swarm_stacks=" << stacks;

      if ( jump )
        s << " (jumped)";

      return s;
    }
  };

  struct adaptive_swarm_base_t : public druid_spell_t
  {
    adaptive_swarm_base_t* other;
    double tf_mul;
    bool heal;

    adaptive_swarm_base_t( druid_t* p, std::string_view n, const spell_data_t* s )
      : druid_spell_t( n, p, s ),
        other( nullptr ),
        tf_mul( p->query_aura_effect( p->talent.tigers_fury, A_ADD_PCT_MODIFIER, P_TICK_DAMAGE, &data() )->percent() ),
        heal( false )
    {
      dual = background = true;
      dot_behavior = dot_behavior_e::DOT_CLIP;
    }

    action_state_t* new_state() override
    {
      return new adaptive_swarm_state_t( this, target );
    }

    adaptive_swarm_state_t* state( action_state_t* s )
    {
      return debug_cast<adaptive_swarm_state_t*>( s );
    }

    const adaptive_swarm_state_t* state( const action_state_t* s ) const
    {
      return debug_cast<const adaptive_swarm_state_t*>( s );
    }

    timespan_t travel_time() const override
    {
      auto s_ = state( execute_state );

      if ( s_->jump )
        return timespan_t::from_seconds( s_->range / travel_speed );

      if ( target == player )
        return 0_ms;

      return druid_spell_t::travel_time();
    }

    virtual swarm_target_t new_swarm_target( swarm_target_t = {} ) = 0;

    void send_swarm( swarm_target_t swarm_target, int stack, double range )
    {
      auto new_state = state( get_state() );

      // healing swarm get a new randomized ranged. damage swarms replicate the range of the preceding healing swarm.
      if ( heal )
      {
        new_state->range = rng().roll( 0.5 ) ? rng().gauss( p()->options.adaptive_swarm_jump_distance_melee,
                                                            p()->options.adaptive_swarm_jump_distance_stddev, true )
                                             : rng().gauss( p()->options.adaptive_swarm_jump_distance_ranged,
                                                            p()->options.adaptive_swarm_jump_distance_stddev, true );
      }
      else
      {
        new_state->range = range;
      }

      new_state->stacks = stack;
      new_state->jump = true;
      new_state->swarm_target = swarm_target;
      new_state->target = swarm_target;

      sim->print_debug( "ADAPTIVE_SWARM: jumping {} {} stacks to {} ({} yd)", stack, heal ? "heal" : "damage",
                        heal ? swarm_target.event ? "existing target" : "new target" : swarm_target.player->name(),
                        new_state->range );

      schedule_execute( new_state );
    }

    void jump_swarm( int s, double r )
    {
      auto stacks = s - 1;

      if ( stacks <= 0 )
      {
        sim->print_debug( "ADAPTIVE_SWARM: {} expiring on final stack", heal ? "heal" : "damage" );
        return;
      }

      auto new_target = other->new_swarm_target();
      if ( !new_target )
      {
        new_target = new_swarm_target();
        assert( !new_target == false );
        send_swarm( new_target, stacks, r );
      }
      else
      {
        other->send_swarm( new_target, stacks, r );
      }

      if ( p()->talent.unbridled_swarm.ok() && rng().roll( p()->talent.unbridled_swarm->effectN( 1 ).percent() ) )
      {
        auto second_target = other->new_swarm_target( new_target );
        if ( !second_target )
        {
          second_target = new_swarm_target();
          assert( !second_target == false );
          sim->print_debug( "ADAPTIVE_SWARM: splitting off {} swarm", heal ? "heal" : "damage" );
          send_swarm( second_target, stacks, r );
        }
        else
        {
          sim->print_debug( "ADAPTIVE_SWARM: splitting off {} swarm", other->heal ? "heal" : "damage" );
          other->send_swarm( second_target, stacks, r );
        }
      }
    }
  };

  struct adaptive_swarm_damage_t : public adaptive_swarm_base_t
  {
    adaptive_swarm_damage_t( druid_t* p )
      : adaptive_swarm_base_t( p, "adaptive_swarm_damage", p->spec.adaptive_swarm_damage )
    {}

    bool dots_ticking( const druid_td_t* td ) const
    {
      return td->dots.moonfire->is_ticking() || td->dots.rake->is_ticking() || td->dots.rip->is_ticking() ||
             td->dots.stellar_flare->is_ticking() || td->dots.sunfire->is_ticking() ||
             td->dots.thrash_bear->is_ticking() || td->dots.thrash_cat->is_ticking();
    }

    swarm_target_t new_swarm_target( swarm_target_t exclude ) override
    {
      auto tl = target_list();

      // because action_t::available_targets() explicitly adds the current action_t::target to the target_cache, we need
      // to explicitly remove it here as swarm should not pick an invulnerable target whenignore_invulnerable_targets is
      // enabled.
      if ( sim->ignore_invulnerable_targets && target->debuffs.invulnerable->check() )
        tl.erase( std::remove( tl.begin(), tl.end(), target ), tl.end() );

      if ( exclude.player )
        tl.erase( std::remove( tl.begin(), tl.end(), exclude.player ), tl.end() );

      if ( tl.empty() )
        return {};

      if ( tl.size() == 1 )
        return tl[ 0 ];

      rng().shuffle( tl.begin(), tl.end() );

      // prioritize unswarmed over swarmed
      auto it = range::partition( tl, [ this ]( player_t* t ) {
        return !p()->get_target_data( t )->dots.adaptive_swarm_damage->is_ticking();
      } );

      // if unswarmed exists, prioritize dotted over undotted
      if ( it != tl.begin() )
      {
        std::partition( tl.begin(), it, [ this ]( player_t* t ) {
          return dots_ticking( p()->get_target_data( t ) );
        } );
      }
      // otherwise if swarmed exists, prioritize dotted over undotted
      else if ( it != tl.end() )
      {
        std::partition( it, tl.end(), [ this ]( player_t* t ) {
          return dots_ticking( p()->get_target_data( t ) );
        } );
      }

      return tl[ 0 ];
    }

    void impact( action_state_t* s ) override
    {
      auto incoming = state( s )->stacks;
      auto existing = get_dot( s->target )->current_stack();

      adaptive_swarm_base_t::impact( s );

      auto d = get_dot( s->target );
      d->increment( incoming + existing - 1 );

      sim->print_debug( "ADAPTIVE_SWARM: damage incoming:{} existing:{} final:{}", incoming, existing,
                        d->current_stack() );
    }

    void last_tick( dot_t* d ) override
    {
      adaptive_swarm_base_t::last_tick( d );

      // last_tick() is called via dot_t::cancel() when a new swarm CLIPs an existing swarm. As dot_t::last_tick() is
      // called before dot_t::reset(), check the remaining time on the dot to see if we're dealing with a true last_tick
      // (and thus need to check for a new jump) or just a refresh.

      // NOTE: if demise() is called on the target d->remains() has time left, so assume that a target that is sleeping
      // with time remaining has been demised()'d and jump to a new target
      if ( d->remains() > 0_ms && !d->target->is_sleeping() )
        return;

      jump_swarm( d->current_stack(), state( d->state )->range );
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      double pm = adaptive_swarm_base_t::composite_persistent_multiplier( s );

      // inherits from druid_spell_t so does not get automatic persistent multiplier parsing
      if ( !state( s )->jump && p()->buff.tigers_fury->up() )
        pm *= 1.0 + tf_mul;

      return pm;
    }

    double calculate_tick_amount( action_state_t* s, double m ) const override
    {
      auto stack = find_dot( s->target )->current_stack();
      assert( stack );

      return adaptive_swarm_base_t::calculate_tick_amount( s, m / stack );
    }
  };

  struct adaptive_swarm_heal_t : public adaptive_swarm_base_t
  {
    struct adaptive_swarm_heal_event_t : public event_t
    {
      adaptive_swarm_heal_t* swarm;
      druid_t* druid;
      double range;
      int stacks;

      adaptive_swarm_heal_event_t( druid_t* p, adaptive_swarm_heal_t* a, double r, int s, timespan_t d )
        : event_t( *p, d ), swarm( a ), druid( p ), range( r ), stacks( s )
      {}

      const char* name() const override
      {
        return "adaptive swarm heal event";
      }

      void execute() override
      {
        auto& tracker = druid->swarm_tracker;
        tracker.erase( std::remove( tracker.begin(), tracker.end(), this ), tracker.end() );

        swarm->jump_swarm( stacks, range );
      }
    };

    adaptive_swarm_heal_t( druid_t* p ) : adaptive_swarm_base_t( p, "adaptive_swarm_heal", p->spec.adaptive_swarm_heal )
    {
      quiet = heal = true;
      harmful = may_miss = false;
      base_td = base_td_multiplier = 0.0;

      parse_effect_period( data().effectN( 1 ) );
    }

    // TODO: add exclude functionality
    swarm_target_t new_swarm_target( swarm_target_t /* exclude */) override
    {
      if ( p()->swarm_tracker.size() < p()->options.adaptive_swarm_friendly_targets )
        return target;

      return { target, p()->swarm_tracker[ rng().range( p()->swarm_tracker.size() ) ] };
    }

    void impact( action_state_t* s ) override
    {
      auto incoming = state( s )->stacks;
      int existing = 0;
      int final = incoming;
      auto swarm_target = state( s )->swarm_target;

      adaptive_swarm_base_t::impact( s );

      adaptive_swarm_heal_event_t* swarm_event = nullptr;

      if ( swarm_target.event && swarm_target.event->remains() > 0_ms )
        swarm_event = dynamic_cast<adaptive_swarm_heal_event_t*>( swarm_target.event );

      if ( swarm_event )
      {
        existing = swarm_event->stacks;
        final = std::min( dot_max_stack, final + existing );
        swarm_event->stacks = final;
        swarm_event->reschedule( dot_duration );
      }
      else
      {
        // TODO: fix edge case where a healing swarm will be sent out while another is in-flight allowing you to end up
        // with swarm_tracker.size() > adaptive_swarm_friendly_targets
        p()->swarm_tracker.push_back(
            make_event<adaptive_swarm_heal_event_t>( *sim, p(), this, state( s )->range, final, dot_duration ) );
      }

      final = std::min( dot_max_stack, final );
      sim->print_debug( "ADAPTIVE_SWARM: heal incoming:{} existing:{} final:{} # of heal swarms:{}", incoming, existing,
                        final, p()->swarm_tracker.size() );
    }
  };

  adaptive_swarm_base_t* damage;
  adaptive_swarm_base_t* heal;
  timespan_t gcd_add;
  bool target_self;

  adaptive_swarm_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "adaptive_swarm", p, p->talent.adaptive_swarm ),
      gcd_add( p->query_aura_effect( p->spec.cat_form_passive, A_ADD_FLAT_LABEL_MODIFIER, P_GCD, &data() )->time_value() ),
      target_self( false )
  {
    add_option( opt_deprecated( "precombat_seconds", "druid.adaptive_swarm_prepull_setup" ) );
    add_option( opt_deprecated( "precombat_stacks", "druid.adaptive_swarm_prepull_setup" ) );
    add_option( opt_bool( "target_self", target_self ) );
    parse_options( opt );

    // These are always necessary to allow APL parsing of dot.adaptive_swarm expressions
    damage = p->get_secondary_action<adaptive_swarm_damage_t>( "adaptive_swarm_damage" );
    heal = p->get_secondary_action<adaptive_swarm_heal_t>( "adaptive_swarm_heal" );

    if ( !p->talent.adaptive_swarm.ok() )
      return;

    may_miss = harmful = false;
    base_costs[ RESOURCE_MANA ] = 0.0;  // remove mana cost so we don't need to enable mana regen

    damage->other = heal;
    damage->stats = stats;
    heal->other = damage;
    add_child( damage );
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p()->buff.cat_form->check() )
      g += gcd_add;

    return g;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( target_self )
    {
      heal->set_target( player );
      heal->schedule_execute();
    }
    else
    {
      damage->set_target( target );
      damage->schedule_execute();
    }
  }
};

// Astral Communion =========================================================
struct astral_communion_t : public druid_spell_t
{
  astral_communion_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "astral_communion", p, p->talent.astral_communion, opt )
  {
    harmful = false;
  }
};

// Astral Smolder ===========================================================
struct astral_smolder_t : public druid_residual_action_t<druid_spell_t>
{
  astral_smolder_t( druid_t* p ) : base_t( "astral_smolder", p, p->find_spell( 394061 ) )
  {
    residual_mul = p->talent.astral_smolder->effectN( 1 ).percent();
  }

  double base_ta( const action_state_t* s ) const override
  {
    return get_amount( s ) * tick_time( s ) / composite_dot_duration( s );
  }
};

// Barkskin =================================================================
struct barkskin_t : public druid_spell_t
{
  action_t* brambles;

  barkskin_t( druid_t* p, std::string_view opt ) : barkskin_t( p, "barkskin", opt ) {}

  barkskin_t( druid_t* p, std::string_view n, std::string_view opt )
    : druid_spell_t( n, p, p->find_class_spell( "Barkskin" ), opt ), brambles( nullptr )
  {
    harmful = false;
    use_off_gcd = true;
    dot_duration = 0_ms;

    if ( p->talent.brambles.ok() )
    {
      brambles = p->get_secondary_action_n<bear_attacks::brambles_pulse_t>( name_str + "brambles" );
      name_str += "+brambles";
      brambles->stats = stats;
    }
  }

  void init() override
  {
    druid_spell_t::init();

    if ( brambles && !name_str_reporting.empty() )
      name_str_reporting += "+brambles";
  }

  void execute() override
  {
    // since barkskin can be used off gcd, it can bypass schedule_execute() so we check for autoshift here
    check_autoshift();

    druid_spell_t::execute();

    if ( brambles )
    {
      p()->buff.barkskin->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
        brambles->execute();
      } );
    }

    p()->buff.barkskin->trigger();

    if ( p()->talent.matted_fur.ok() )
      p()->buff.matted_fur->trigger();
  }
};

// Celestial Alignment ======================================================
struct celestial_alignment_base_t : public druid_spell_t
{
  buff_t* buff;

  celestial_alignment_base_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt )
    : druid_spell_t( n, p, p->apply_override( s, p->talent.syzygy ), opt ), buff( p->buff.celestial_alignment )
  {
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    buff->trigger();
    p()->uptime.primordial_arcanic_pulsar->update( false, sim->current_time() );
    p()->eclipse_handler.cast_ca_inc();
  }
};

struct celestial_alignment_t : public celestial_alignment_base_t
{
  celestial_alignment_t( druid_t* p, std::string_view opt )
    : celestial_alignment_base_t( "celestial_alignment", p, p->spec.celestial_alignment, opt )
  {}

  bool ready() override
  {
    return p()->talent.incarnation_moonkin.ok() ? false : celestial_alignment_base_t::ready();
  }
};

struct incarnation_moonkin_t : public celestial_alignment_base_t
{
  incarnation_moonkin_t( druid_t* p, std::string_view opt )
    : celestial_alignment_base_t( "incarnation_chosen_of_elune", p, p->spec.incarnation_moonkin, opt )
  {
    form_mask = MOONKIN_FORM;
    autoshift = p->active.shift_to_moonkin;
    buff      = p->buff.incarnation_moonkin;
  }
};

// Dash =====================================================================
struct dash_t : public druid_spell_t
{
  buff_t* buff_on_cast;
  double gcd_mul;

  dash_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "dash", p, p->talent.tiger_dash.ok() ? p->talent.tiger_dash : p->find_class_spell( "Dash" ), opt ),
      buff_on_cast( p->talent.tiger_dash.ok() ? p->buff.tiger_dash : p->buff.dash ),
      gcd_mul( p->query_aura_effect( &p->buff.cat_form->data(), A_ADD_PCT_MODIFIER, P_GCD, &data() )->percent() )
  {
    harmful = may_miss = false;
    ignore_false_positive = true;

    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p()->buff.cat_form->check() )
      g *= 1.0 + gcd_mul;

    return g;
  }

  void execute() override
  {
    druid_spell_t::execute();

    buff_on_cast->trigger();
  }
};

// Entangling Roots =========================================================
struct entangling_roots_t : public druid_spell_t
{
  timespan_t gcd_add;

  entangling_roots_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "entangling_roots", p, p->find_class_spell( "Entangling Roots" ), opt ),
      gcd_add( p->query_aura_effect( p->spec.cat_form_passive, A_ADD_FLAT_MODIFIER, P_GCD, &data() )->time_value() )
  {
    form_mask = NO_FORM | MOONKIN_FORM;
    harmful   = false;
    // workaround so that we do not need to enable mana regen
    base_costs[ RESOURCE_MANA ] = 0.0;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p()->buff.cat_form->check() )
      g += gcd_add;

    return g;
  }

  bool check_form_restriction() override
  {
    if ( p()->buff.predatory_swiftness->check() )
      return true;

    return druid_spell_t::check_form_restriction();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.predatory_swiftness->expire();
  }
};

// Force of Nature ==========================================================
struct force_of_nature_t : public druid_spell_t
{
  timespan_t summon_duration;

  force_of_nature_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "force_of_nature", p, p->talent.force_of_nature, opt ),
      summon_duration( p->talent.force_of_nature->effectN( 2 ).trigger()->duration() + 1_ms )
  {
    harmful = false;
  }

  void init_finished() override
  {
    if ( p()->talent.force_of_nature.ok() )
    {
      for ( const auto& treant : p()->pets.force_of_nature )
      {
        for ( auto a : treant->action_list )
        {
          auto it = range::find( child_action, a->name_str, &action_t::name_str );
          if ( it != child_action.end() )
            a->stats = ( *it )->stats;
          else
            add_child( a );
        }
      }
    }

    druid_spell_t::init_finished();
  }

  void execute() override
  {
    druid_spell_t::execute();

    for ( const auto& treant : p()->pets.force_of_nature )
      if ( treant->is_sleeping() )
        treant->summon( summon_duration );
  }
};

// Fury of Elune =========================================================
struct fury_of_elune_t : public druid_spell_t
{
  struct fury_of_elune_tick_t : public druid_spell_t
  {
    fury_of_elune_tick_t( druid_t* p, std::string_view n, const spell_data_t* s ) : druid_spell_t( n, p, s )
    {
      background = dual = ground_aoe = true;
      aoe = -1;
      reduced_aoe_targets = 1.0;
      full_amount_targets = 1;
    }

    void impact( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      p()->eclipse_handler.tick_fury_of_elune();
    }
  };

  action_t* damage;
  buff_t* energize;
  timespan_t tick_period;

  fury_of_elune_t( druid_t* p, std::string_view opt )
    : fury_of_elune_t( p, "fury_of_elune", p->talent.fury_of_elune, p->find_spell( 211545 ), p->buff.fury_of_elune,
                       opt )
  {}

  fury_of_elune_t( druid_t* p, std::string_view n, const spell_data_t* s, const spell_data_t* s_damage, buff_t* b,
                   std::string_view opt )
    : druid_spell_t( n, p, s, opt ),
      energize( b ),
      tick_period( p->query_aura_effect( &b->data(), A_PERIODIC_ENERGIZE, RESOURCE_ASTRAL_POWER )->period() )
  {
    form_mask |= NO_FORM; // can be cast without form
    dot_duration = 0_ms;  // AP gain handled via buffs

    damage = p->get_secondary_action_n<fury_of_elune_tick_t>( name_str + "_tick", s_damage );
    damage->stats = stats;
  }

  bool has_amount_result() const override { return damage->has_amount_result(); }

  void execute() override
  {
    druid_spell_t::execute();

    energize->trigger();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( target )
      .hasted( ground_aoe_params_t::hasted_with::SPELL_HASTE )
      .pulse_time( tick_period )
      .duration( data().duration() )
      .action( damage ) );
  }
};

// Heart of the Wild ========================================================
struct heart_of_the_wild_t : public druid_spell_t
{
  heart_of_the_wild_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "heart_of_the_wild", p, p->talent.heart_of_the_wild, opt )
  {
    harmful = may_miss = reset_melee_swing = false;

    // Although the effect is coded as modify cooldown time (341) which takes a flat value in milliseconds, the actual
    // effect in-game works as a percent reduction.
    cooldown->duration *= 1.0 + p->conduit.born_of_the_wilds.percent();
  }

  void schedule_execute( action_state_t* s ) override
  {
    druid_spell_t::schedule_execute( s );
  }

  void trigger_ravenous_frenzy( free_spell_e ) override { return; }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.heart_of_the_wild->trigger();
  }
};

// Incarnation (Tree) =========================================================
struct incarnation_tree_t : public druid_spell_t
{
  incarnation_tree_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "incarnation_tree_of_life", p, p->talent.incarnation_tree, opt )
  {
    harmful   = false;
    form_mask = NO_FORM;
    autoshift = p->active.shift_to_caster;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.incarnation_tree->trigger();
  }
};

// Innervate ================================================================
struct innervate_t : public druid_spell_t
{
  innervate_t( druid_t* p, std::string_view opt ) : druid_spell_t( "innervate", p, p->talent.innervate, opt )
  {
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.innervate->trigger();
  }
};

// Mark of the Wild =========================================================
struct mark_of_the_wild_t : public druid_spell_t
{
  mark_of_the_wild_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "mark_of_the_wild", p, p->find_class_spell( "Mark of the Wild" ), opt )
  {
    harmful = false;
    ignore_false_positive = true;

    if ( sim->overrides.mark_of_the_wild )
      background = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( !sim->overrides.mark_of_the_wild )
      sim->auras.mark_of_the_wild->trigger();
  }
};

// Moon Spells ==============================================================
struct moon_base_t : public druid_spell_t
{
  moon_stage_e stage;

  moon_base_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt, moon_stage_e moon )
    : druid_spell_t( n, p, s, opt ), stage( moon )
  {}

  void init() override
  {
    if ( !is_free_proc() )
      cooldown = p()->cooldown.moon_cd;

    druid_spell_t::init();

    if ( !is_free() && data().ok() )
    {
      if ( cooldown->duration != 0_ms && cooldown->duration != data().charge_cooldown() )
      {
        p()->sim->error( "Moon CD: {} ({}) cooldown of {} doesn't match existing cooldown of {}", name_str, data().id(),
                         data().charge_cooldown(), cooldown->duration );
      }
      cooldown->duration = data().charge_cooldown();

      if ( cooldown->charges != 1 && cooldown->charges != as<int>( data().charges() ) )
      {
        p()->sim->error( "Moon CD: {} ({}) charges of {} doesn't match existing charges of {}", name_str, data().id(),
                         data().charges(), cooldown->charges );
      }
      cooldown->charges = data().charges();
    }
  }

  virtual bool check_stage() const
  {
    return p()->moon_stage == stage;
  }

  virtual void advance_stage()
  {
    p()->moon_stage++;
  }

  bool ready() override
  {
    return !check_stage() ? false : druid_spell_t::ready();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->eclipse_handler.cast_moon( stage );

    if ( is_free_proc() )
      return;

    advance_stage();
  }
};

// New Moon Spell ===========================================================
struct new_moon_t : public moon_base_t
{
  new_moon_t( druid_t* p, std::string_view opt )
    : moon_base_t( "new_moon", p, p->talent.new_moon, opt, moon_stage_e::NEW_MOON )
  {}
};

// Half Moon Spell ==========================================================
struct half_moon_t : public moon_base_t
{
  half_moon_t( druid_t* p, std::string_view opt )
    : moon_base_t( "half_moon", p, p->spec.half_moon, opt, moon_stage_e::HALF_MOON )
  {}
};

// Full Moon Spell ==========================================================
struct full_moon_t : public moon_base_t
{
  full_moon_t( druid_t* p, std::string_view opt ) : full_moon_t( p, "full_moon", p->spec.full_moon, opt ) {}

  full_moon_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : moon_base_t( n, p, s, opt, moon_stage_e::FULL_MOON )
  {
    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;

    // Since this can be free_cast, only energize for Balance
    if ( !p->spec.astral_power->ok() )
      energize_type = action_energize::NONE;
  }

  bool check_stage() const override
  {
    return p()->moon_stage >= stage;
  }

  void advance_stage() override
  {
    auto max_stage = p()->talent.radiant_moonlight.ok() ? moon_stage_e::MAX_MOON : moon_stage_e::FULL_MOON;

    if ( p()->moon_stage == max_stage )
      p()->moon_stage = moon_stage_e::NEW_MOON;
    else
      moon_base_t::advance_stage();
  }

  void execute() override
  {
    moon_base_t::execute();

    // if the extra charge of FM from RM is available, orbit breaker will eat it
    if ( p()->bugs && free_spell == free_spell_e::ORBIT && p()->talent.radiant_moonlight.ok() &&
         p()->moon_stage == moon_stage_e::FULL_MOON )
    {
      p()->moon_stage = moon_stage_e::MAX_MOON;
    }
  }
};

// Proxy Moon Spell =========================================================
struct moon_proxy_t : public druid_spell_t
{
  action_t* new_moon;
  action_t* half_moon;
  action_t* full_moon;

  moon_proxy_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "moons", p, p->talent.new_moon.ok() ? spell_data_t::nil() : spell_data_t::not_found(), opt ),
      new_moon( new new_moon_t( p, opt ) ),
      half_moon( new half_moon_t( p, opt ) ),
      full_moon( new full_moon_t( p, opt ) )
  {}

  void schedule_execute( action_state_t* s ) override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  new_moon->schedule_execute( s ); return;
      case HALF_MOON: half_moon->schedule_execute( s ); return;
      case FULL_MOON: 
      case MAX_MOON:  full_moon->schedule_execute( s ); return;
      default: break;
    }

    if ( s )
      action_state_t::release( s );
  }

  void execute() override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  new_moon->execute(); return;
      case HALF_MOON: half_moon->execute(); return;
      case FULL_MOON:
      case MAX_MOON:  full_moon->execute(); return;
      default: break;
    }

    if ( pre_execute_state )
      action_state_t::release( pre_execute_state );
  }

  bool action_ready() override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  return new_moon->action_ready();
      case HALF_MOON: return half_moon->action_ready();
      case FULL_MOON:
      case MAX_MOON:  return full_moon->action_ready();
      default: return false;
    }
  }

  bool target_ready( player_t* t ) override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  return new_moon->target_ready( t );
      case HALF_MOON: return half_moon->target_ready( t );
      case FULL_MOON:
      case MAX_MOON:  return full_moon->target_ready( t );
      default: return false;
    }
  }

  bool ready() override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  return new_moon->ready();
      case HALF_MOON: return half_moon->ready();
      case FULL_MOON:
      case MAX_MOON:  return full_moon->ready();
      default: return false;
    }
  }
};

// Moonfire Spell ===========================================================
struct moonfire_t : public druid_spell_t
{
  struct moonfire_damage_t : public druid_spell_t
  {
    double gg_mul;
    double feral_override_da;
    double feral_override_ta;

    moonfire_damage_t( druid_t* p, std::string_view n )
      : druid_spell_t( n, p, p->spec.moonfire_dmg ), gg_mul( 0.0 ), feral_override_da( 0.0 ), feral_override_ta( 0.0 )
    {
      may_miss = false;
      dual = background = true;

      triggers_galactic_guardian = false;
      dot_name = "moonfire";

      if ( p->talent.twin_moons.ok() )
      {
        // The increased target number has been removed from spell data
        aoe = std::max( aoe, 1 ) + 1;
        radius = p->talent.twin_moons->effectN( 1 ).base_value();
      }

      if ( p->talent.twin_moonfire.ok() )
      {
        // uses balance twin moons talent for radius
        aoe = std::max( aoe, 1 ) + 1;
        radius = p->find_spell( 279620 )->effectN( 1 ).base_value();
      }

      if ( p->spec.astral_power->ok() )
      {
        energize_resource = RESOURCE_ASTRAL_POWER;
        energize_type = action_energize::PER_HIT;
        energize_amount = p->spec.astral_power->effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
      }
      else
      {
        energize_type = action_energize::NONE;
      }

      if ( p->talent.galactic_guardian.ok() )
        gg_mul = p->buff.galactic_guardian->data().effectN( 3 ).percent();

      // Always applies when you are a feral druid unless you go into moonkin form
      if ( p->specialization() == DRUID_FERAL )
      {
        feral_override_da =
            p->query_aura_effect( p->spec.feral_overrides, A_ADD_PCT_MODIFIER, P_GENERIC, &data() )->percent();
        feral_override_ta =
            p->query_aura_effect( p->spec.feral_overrides, A_ADD_PCT_MODIFIER, P_TICK_DAMAGE, &data() )->percent();
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double dam = druid_spell_t::composite_da_multiplier( s );

      // MF proc'd by gg is affected by any existing gg buff.
      if ( p()->buff.galactic_guardian->check() )
        dam *= 1.0 + gg_mul;

      if ( feral_override_da && !p()->buff.moonkin_form->check() )
        dam *= 1.0 + feral_override_da;

      return dam;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double tam = druid_spell_t::composite_ta_multiplier( s );

      if ( feral_override_ta && !p()->buff.moonkin_form->check() )
        tam *= 1.0 + feral_override_ta;

      return tam;
    }

    std::vector<player_t*>& target_list() const override
    {
      auto& tl = druid_spell_t::target_list();

      // When Twin Moons is active, this is an AoE action meaning it will impact onto the first 2 targets in the target
      // list. Instead, we want it to impact on the target of the action and 1 additional, so we'll override the
      // target_list to make it so.
      if ( is_aoe() && as<int>( tl.size() ) > aoe )
      {
        // always hit the target, so if it exists make sure it's first
        auto start_it = tl.begin() + ( tl[ 0 ] == target ? 1 : 0 );

        // randomize remaining targets
        rng().shuffle( start_it, tl.end() );

        // prioritize undotted over dotted
        std::partition( start_it, tl.end(), [ this ]( player_t* t ) {
          return !td( t )->dots.moonfire->is_ticking();
        } );
      }

      return tl;
    }

    void gain_energize_resource( resource_e rt, double amt, gain_t* gain ) override
    {
      druid_spell_t::gain_energize_resource( rt, amt, gain );

      if ( p()->talent.stellar_innervation.ok() && p()->buff.eclipse_lunar->check() )
        p()->resource_gain( rt, amt, p()->gain.stellar_innervation, this );
    }

    void execute() override
    {
      // Force invalidate target cache so that it will impact on the correct targets.
      target_cache.is_valid = false;

      druid_spell_t::execute();

      if ( hit_any_target )
      {
        trigger_gore();

        p()->buff.protector_of_the_pack_moonfire->expire();

        if ( !is_free() )
          p()->buff.galactic_guardian->expire();
      }
    }

    void impact( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      p()->resource_gain( RESOURCE_RAGE, p()->buff.galactic_guardian->check_value(), gain );
    }

    void trigger_dot( action_state_t* s ) override
    {
      druid_spell_t::trigger_dot( s );

      td( s->target )->debuff.moonfire->trigger( -1, p()->cache.mastery_value(), -1.0, timespan_t::min() );

      // moonfire will completely replace lunar inspiration if the new moonfire duration is greater
      auto li_dot = td( s->target )->dots.lunar_inspiration;

      if ( get_dot( s->target )->remains() > li_dot->remains() )
        li_dot->cancel();
    }

    void tick( dot_t* d ) override
    {
      // Moonfire damage is supressed while lunar inspiration is also on the target. Note that it is not cancelled and
      // continues to tick down it's duration. If there is any duration remaining after lunar inspiration expires,
      // moonfire will resume ticking for damage.
      // Note that moonfire CAN still proc shooting stars while suppressed
      if ( td( d->target )->dots.lunar_inspiration->is_ticking() )
      {
        trigger_shooting_stars( d->target );
        return;
      }

      druid_spell_t::tick( d );

      trigger_shooting_stars( d->target );
    }

    void last_tick( dot_t* d ) override
    {
      druid_spell_t::last_tick( d );

      td( d->target )->debuff.moonfire->expire();
    }
  };

  moonfire_damage_t* damage;  // Add damage modifiers in moonfire_damage_t, not moonfire_t

  moonfire_t( druid_t* p, std::string_view opt ) : moonfire_t( p, "moonfire", opt ) {}

  moonfire_t( druid_t* p, std::string_view n, std::string_view opt )
    : druid_spell_t( n, p, p->find_class_spell( "Moonfire" ), opt )
  {
    may_miss = triggers_galactic_guardian = reset_melee_swing = false;

    damage = p->get_secondary_action_n<moonfire_damage_t>( name_str + "_dmg" );
    damage->stats = stats;
  }

  bool has_amount_result() const override { return damage->has_amount_result(); }

  void init() override
  {
    druid_spell_t::init();

    damage->gain = gain;
  }

  void execute() override
  {
    druid_spell_t::execute();

    damage->target = target;
    damage->schedule_execute();
  }
};

// Nature's Vigil =============================================================
template <class Base>
struct natures_vigil_t : public Base
{
  struct natures_vigil_tick_t : public Base
  {
    double mul;

    natures_vigil_tick_t( druid_t* p )
      : Base( "natures_vigil_tick", p, p->find_spell( p->specialization() == DRUID_RESTORATION ? 124988 : 124991 ) ),
        mul( p->talent.natures_vigil->effectN( 3 ).percent() )
    {
      Base::background = Base::dual = true;
    }

    double get_amount() const { return Base::p()->buff.natures_vigil->check_value() * mul; }
    double base_da_min( const action_state_t* ) const override { return get_amount(); }
    double base_da_max( const action_state_t* ) const override { return get_amount(); }

    void impact( action_state_t* s ) override
    {
      Base::impact( s );

      Base::p()->buff.natures_vigil->current_value = 0;
    }
  };

  natures_vigil_t( druid_t* p, std::string_view opt ) : Base( "natures_vigil", p, p->talent.natures_vigil, opt )
  {
    Base::dot_duration = 0_ms;
    Base::reset_melee_swing = false;

    if ( p->talent.natures_vigil.ok() )
    {
      auto tick = p->get_secondary_action<natures_vigil_tick_t>( "natures_vigil_tick" );
      tick->stats = Base::stats;

      p->buff.natures_vigil->set_tick_callback( [ tick ]( buff_t* b, int, timespan_t ) {
        if ( b->check_value() )
          tick->execute();
      } );
    }
  }

  void execute() override
  {
    Base::execute();

    Base::p()->buff.natures_vigil->trigger();
  }
};

// Prowl ====================================================================
struct prowl_t : public druid_spell_t
{
  prowl_t( druid_t* p, std::string_view opt ) : druid_spell_t( "prowl", p, p->find_class_spell( "Prowl" ), opt )
  {
    use_off_gcd = ignore_false_positive = true;
    harmful = break_stealth = false;

    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  void execute() override
  {
    sim->print_log( "{} performs {}", player->name(), name() );

    // since prowl can be used off gcd, it can bypass schedule_execute() so we check for autoshift again here
    check_autoshift();

    p()->buff.incarnation_cat_prowl->expire();
    p()->buff.prowl->trigger();

    druid_spell_t::execute();
  }

  bool ready() override
  {
    if ( p()->buff.prowl->check() )
      return false;

    if ( p()->in_combat )
    {
      if ( p()->buff.incarnation_cat_prowl->check() )
        return druid_spell_t::ready();

      if ( p()->sim->fight_style == FIGHT_STYLE_DUNGEON_SLICE && p()->player_t::buffs.shadowmeld->check() && target->type == ENEMY_ADD )
        return druid_spell_t::ready();

      if ( p()->sim->target_non_sleeping_list.empty() )
        return druid_spell_t::ready();

      return false;
    }

    return druid_spell_t::ready();
  }
};

// Proxy Swipe Spell ========================================================
struct swipe_proxy_t : public druid_spell_t
{
  action_t* swipe_cat;
  action_t* swipe_bear;

  swipe_proxy_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "swipe", p, p->talent.swipe, opt ),
      swipe_cat( new cat_attacks::swipe_cat_t( p, opt ) ),
      swipe_bear( new bear_attacks::swipe_bear_t( p, opt ) )
  {}

  timespan_t gcd() const override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->gcd();
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->gcd();

    return druid_spell_t::gcd();
  }

  void execute() override
  {
    if ( p()->buff.cat_form->check() )
      swipe_cat->execute();
    else if ( p()->buff.bear_form->check() )
      swipe_bear->execute();

    if ( pre_execute_state )
      action_state_t::release( pre_execute_state );
  }

  bool action_ready() override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->action_ready();
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->action_ready();

    return false;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->target_ready( candidate_target );
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->ready();
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->ready();

    return false;
  }

  double cost() const override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->cost();
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->cost();

    return 0;
  }
};

// Proxy Thrash =============================================================
struct thrash_proxy_t : public druid_spell_t
{
  action_t* thrash_bear;
  action_t* thrash_cat;

  thrash_proxy_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "thrash", p, p->talent.thrash, opt ),
      thrash_bear( new bear_attacks::thrash_bear_t( p, opt ) ),
      thrash_cat( new cat_attacks::thrash_cat_t( p, opt ) )
  {
    // At the moment, both versions of these spells are the same (and only the cat version has a talent that changes
    // this) so we use this spell data here. If this changes we will have to think of something more robust. These two
    // are important for the "ticks_gained_on_refresh" expression to work
    dot_duration = thrash_cat->dot_duration;
    base_tick_time = thrash_cat->base_tick_time;
  }

  timespan_t gcd() const override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->gcd();
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->gcd();

    return druid_spell_t::gcd();
  }

  void execute() override
  {
    if ( p()->buff.cat_form->check() )
      thrash_cat->execute();
    else if ( p()->buff.bear_form->check() )
      thrash_bear->execute();

    if ( pre_execute_state )
      action_state_t::release( pre_execute_state );
  }

  bool action_ready() override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->action_ready();
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->action_ready();

    return false;
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->get_dot( t );
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->get_dot( t );

    return nullptr;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->target_ready( candidate_target );
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->ready();
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->ready();

    return false;
  }

  double cost() const override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->cost();
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->cost();

    return 0;
  }
};

// Skull Bash ===============================================================
struct skull_bash_t : public druid_interrupt_t
{
  skull_bash_t( druid_t* p, std::string_view opt ) : druid_interrupt_t( "skull_bash", p, p->talent.skull_bash, opt ) {}
};

// Solar Beam ===============================================================
struct solar_beam_t : public druid_interrupt_t
{
  solar_beam_t( druid_t* p, std::string_view opt ) : druid_interrupt_t( "solar_beam", p, p->talent.solar_beam, opt )
  {
    base_costs[ RESOURCE_MANA ] = 0.0;  // remove mana cost so we don't need to enable mana regen

    // since simc interrupts only happen when the target is casting, it will always count as successful
    if ( p->talent.light_of_the_sun.ok() )
      cooldown->duration -= timespan_t::from_seconds( p->talent.light_of_the_sun->effectN( 1 ).base_value() );
  }
};

// Stampeding Roar ==========================================================
struct stampeding_roar_t : public druid_spell_t
{
  timespan_t form_gcd;  // lower gcd for roar while in bear/cat form
  timespan_t cast_gcd;  // gcd for caster->bear roar

  stampeding_roar_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "stampeding_roar", p, p->talent.stampeding_roar, opt ),
      form_gcd( p->find_spell( 77764 )->gcd() ),
      cast_gcd( trigger_gcd )
  {
    harmful = false;

    form_mask = BEAR_FORM | CAT_FORM;
    autoshift = p->active.shift_to_bear;
  }

  void init_finished() override
  {
    druid_spell_t::init_finished();

    if ( !p()->talent.front_of_the_pack.ok() )
      return;

    for ( const auto &actor : sim->actor_list )
      if ( actor->buffs.stampeding_roar )
        actor->buffs.stampeding_roar->apply_affecting_aura( p()->talent.front_of_the_pack );
  }

  // form->form stampeding roar (id=77764) is properly given hasted gcd via the druid aura (id=137009 eff#4), but the
  // caster->form stampeding roar (id=106898) is missing from the whitelist and does not have a hasted gcd.
  // find_class_spell() returns 106898, so we need to adjust the gcd_type before schedule_execute()
  void schedule_execute( action_state_t* s ) override
  {
    if ( p()->get_form() == form_e::BEAR_FORM || p()->get_form() == form_e::CAT_FORM )
    {
      gcd_type = gcd_haste_type::ATTACK_HASTE;
      trigger_gcd = form_gcd;
    }
    else
    {
      gcd_type = gcd_haste_type::NONE;
      trigger_gcd = cast_gcd;
    }

    druid_spell_t::schedule_execute( s );
  }

  void execute() override
  {
    druid_spell_t::execute();

    // TODO: add target eligibility via radius
    for ( const auto& actor : sim->player_non_sleeping_list )
      actor->buffs.stampeding_roar->trigger();
  }
};

// Starfall Spell ===========================================================
struct starfall_t : public druid_spell_t
{
  struct starfall_data_t
  {
    player_t* shrapnel_target = nullptr;
    double total_amount = 0.0;

    friend void sc_format_to( const starfall_data_t& data, fmt::format_context::iterator out )
    {
      fmt::format_to( out, "total_amount={} shrapnel_target={}", data.total_amount,
                      data.shrapnel_target ? data.shrapnel_target->name() : "none" );
    }
  };

  using starfall_base_t = druid_residual_action_t<druid_spell_t, starfall_data_t>;

  struct lunar_shrapnel_t : public starfall_base_t
  {
    lunar_shrapnel_t( druid_t* p, std::string_view n ) : base_t( n, p, p->find_spell( 393869 ) )
    {
      aoe = -1;
      reduced_aoe_targets = p->talent.lunar_shrapnel->effectN( 2 ).base_value();
      name_str_reporting = "lunar_shrapnel";

      residual_mul = p->talent.lunar_shrapnel->effectN( 1 ).percent();
    }
    double base_da_min( const action_state_t* s ) const override { return get_amount( s ); }
    double base_da_max( const action_state_t* s ) const override { return get_amount( s ); }
  };

  struct starfall_damage_t : public starfall_base_t
  {
    lunar_shrapnel_t* shrapnel;
    double cosmos_mul;

    starfall_damage_t( druid_t* p, std::string_view n )
      : starfall_base_t( n, p, p->find_spell( 191037 ) ),
        shrapnel( nullptr ),
        cosmos_mul( p->sets->set( DRUID_BALANCE, T29, B4 )->effectN( 1 ).trigger()->effectN( 2 ).percent() )
    {
      background = dual = true;

      if ( p->talent.lunar_shrapnel.ok() )
      {
        auto first = name_str.find_first_of( '_' );
        auto last = name_str.find_last_of( '_' );
        auto suf = name_str.substr( first, last - first );
        shrapnel = p->get_secondary_action_n<lunar_shrapnel_t>( "lunar_shrapnel" + suf );
      }
    }

    double action_multiplier() const override
    {
      auto stack = as<double>( p()->buff.starfall->check() );

      if ( p()->buff.touch_the_cosmos_starfall->check() )
        stack += cosmos_mul;

      return starfall_base_t::action_multiplier() * std::max( 1.0, stack );
    }

    void impact( action_state_t* s ) override
    {
      starfall_base_t::impact( s );

      if ( cast_state( s )->shrapnel_target && cast_state( s )->shrapnel_target == s->target )
      {
        shrapnel->snapshot_and_execute( s, false, [ this, s ]( action_state_t* new_ ) {
          shrapnel->set_amount( new_, s->result_amount );
        } );
      }
    }
  };

  struct starfall_driver_t : public starfall_base_t
  {
    starfall_damage_t* damage;
    double shrapnel_chance;

    starfall_driver_t( druid_t* p, std::string_view n )
      : starfall_base_t( n, p, p->buff.starfall->data().effectN( 3 ).trigger() ), shrapnel_chance( 0.0 )
    {
      background = dual = true;

      auto pre = name_str.substr( 0, name_str.find_last_of( '_' ) );
      damage = p->get_secondary_action_n<starfall_damage_t>( pre + "_damage" );

      // TODO: early estimate, test moar
      if ( p->talent.lunar_shrapnel.ok() )
        shrapnel_chance = 0.2;
    }

    // fake travel time to simulate execution delay for individual stars
    timespan_t travel_time() const override
    {
      // seems to have random discrete intervals. guesstimating at 66ms.
      return ( rng().range<int>( 14 ) + 1 ) * 66_ms;
    }

    void trigger_lunar_shrapnel( action_state_t* s )
    {
      auto tl = starfall_base_t::target_list();

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* t ) {
        return !td( t )->dots.moonfire->is_ticking();
      } ), tl.end() );

      if ( tl.empty() )
        return;

      auto chance = shrapnel_chance * std::cbrt( tl.size() );

      // Shrapnel triggers on the lowest GUID. For simc purposes we trigger this on the first target with moonfire.
      if ( rng().roll( chance ) )
        cast_state( s )->shrapnel_target = tl.front();
    }

    void execute() override
    {
      pre_execute_state = get_state();
      trigger_lunar_shrapnel( pre_execute_state );

      starfall_base_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      starfall_base_t::impact( s );

      damage->snapshot_and_execute( s, false, [ s ]( action_state_t* new_ ) {
        new_->copy_state( s );
      } );
    }
  };

  starfall_driver_t* driver;
  timespan_t max_ext;

  starfall_t( druid_t* p, std::string_view opt ) : starfall_t( p, "starfall", p->talent.starfall, opt ) {}

  starfall_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : druid_spell_t( n, p, s, opt ),
      max_ext( timespan_t::from_seconds( p->talent.aetherial_kindling->effectN( 2 ).base_value() ) )
  {
    may_miss = false;
    queue_failed_proc = p->get_proc( "starfall queue failed" );
    dot_duration = 0_ms;
    form_mask |= NO_FORM;

    driver = p->get_secondary_action_n<starfall_driver_t>( name_str + "_driver" );
    assert( driver->damage );
    driver->stats = stats;
    driver->damage->stats = stats;

    if ( p->talent.lunar_shrapnel.ok() )
      add_child( driver->damage->shrapnel );
  }

  void execute() override
  {
    if ( p()->buff.touch_the_cosmos->up() && !is_free_cast() )
    {
      p()->active.starfall_cosmos->execute_on_target( target );
      p()->buff.touch_the_cosmos->expire();

      if ( p()->bugs && !is_free_proc() )
        p()->buff.starweavers_warp->expire();

      return;
    }

    if ( !is_free() && p()->buff.starweavers_warp->up() )
    {
      p()->active.starfall_starweaver->execute_on_target( target );
      p()->buff.starweavers_warp->expire();
      return;
    }

    if ( p()->talent.aetherial_kindling.ok() )
    {
      auto ext = p()->talent.aetherial_kindling->effectN( 1 ).base_value();

      std::vector<player_t*>& tl = target_list();
      auto dur = timespan_t::from_seconds( ext );

      for ( const auto& t : tl )
      {
        td( t )->dots.moonfire->adjust_duration( dur, max_ext, -1, false );
        td( t )->dots.sunfire->adjust_duration( dur, max_ext, -1, false );
      }
    }

    druid_spell_t::execute();

    p()->buff.starfall->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      driver->execute();
      p()->eclipse_handler.tick_starfall();
    } );

    p()->buff.starfall->trigger();

    if ( p()->buff.touch_the_cosmos->check() )
      p()->buff.touch_the_cosmos_starfall->trigger();

    if ( p()->sets->has_set_bonus( DRUID_BALANCE, T29, B2 ) )
      p()->buff.gathering_starstuff->trigger();

    if ( is_free_proc() )
      return;

    if ( p()->talent.starlord.ok() )
      p()->buff.starlord->trigger();

    if ( p()->talent.rattle_the_stars.ok() )
      p()->buff.rattled_stars->trigger();

    if ( p()->talent.starweaver.ok() )
      p()->buff.starweavers_weft->trigger();
  }
};

// Starfire =============================================================
struct starfire_t : public druid_spell_t
{
  double sotf_mul;

  starfire_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "starfire", p, p->talent.starfire, opt ),
      sotf_mul( p->talent.soul_of_the_forest_moonkin->effectN( 2 ).percent() )
  {
    aoe = -1;
    base_aoe_multiplier = data().effectN( p->specialization() == DRUID_BALANCE ? 3 : 2 ).percent();
  }

  void init() override
  {
    druid_spell_t::init();

    if ( p()->talent.umbral_embrace.ok() )
      debug_cast<buffs::umbral_embrace_buff_t*>( p()->buff.umbral_embrace )->actions.push_back( this );
  }

  void init_finished() override
  {
    druid_spell_t::init_finished();

    // for precombat we hack it to manually energize 100ms later to get around AP capping on combat start
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER )
      energize_type = action_energize::NONE;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = druid_spell_t::composite_energize_amount( s );

    if ( p()->buff.warrior_of_elune->check() )
      e *= 1.0 + p()->talent.warrior_of_elune->effectN( 2 ).percent();

    return e;
  }

  void execute() override
  {
    druid_spell_t::execute();

    // for precombat we hack it to advance eclipse and manually energize 100ms later to get around the eclipse stack
    // reset & AP capping on combat start
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER )
    {
      make_event( *sim, 100_ms, [ this ]() {
        p()->eclipse_handler.cast_starfire();
        p()->resource_gain( RESOURCE_ASTRAL_POWER, composite_energize_amount( execute_state ),
                            energize_gain( execute_state ) );
      } );

      return;
    }

    if ( p()->buff.owlkin_frenzy->up() )
      p()->buff.owlkin_frenzy->expire();
    else if ( p()->buff.warrior_of_elune->up() )
      p()->buff.warrior_of_elune->decrement();

    p()->buff.umbral_embrace->expire();
    p()->buff.gathering_starstuff->expire();

    if ( is_free_proc() )
      return;

    p()->eclipse_handler.cast_starfire();
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( p()->active.astral_smolder && s->result_amount > 0 && result_is_hit( s->result ) && s->result == RESULT_CRIT )
    {
      auto smolder = debug_cast<astral_smolder_t*>( p()->active.astral_smolder );

      smolder->snapshot_and_execute( s, true, [ smolder, s ]( action_state_t* new_ ) {
        smolder->set_amount( new_, s->result_amount );
      } );
    }
  }

  double composite_aoe_multiplier( const action_state_t* s ) const override
  {
    double cam = druid_spell_t::composite_aoe_multiplier( s );

    if ( s->target != target && sotf_mul && p()->buff.eclipse_lunar->check() )
      cam *= 1.0 + sotf_mul;

    return cam;
  }
};

// Starsurge Spell ==========================================================
struct starsurge_offspec_t : public druid_spell_t
{
  starsurge_offspec_t( druid_t* p, std::string_view opt ) : druid_spell_t( "starsurge", p, p->talent.starsurge, opt )
  {
    form_mask = NO_FORM | MOONKIN_FORM;
    base_costs[ RESOURCE_MANA ] = 0.0;  // so we don't need to enable mana regen
    may_autounshift = true;
  }
};

struct starsurge_t : public druid_spell_t
{
  struct goldrinns_fang_t : public druid_spell_t
  {
    goldrinns_fang_t( druid_t* p, std::string_view n )
      : druid_spell_t( n, p, p->talent.power_of_goldrinn->effectN( 1 ).trigger() )
    {
      background = true;
      name_str_reporting = "goldrinns_fang";
    }
  };

  action_t* goldrinn;

  bool moonkin_form_in_precombat;

  starsurge_t( druid_t* p, std::string_view opt ) : starsurge_t( p, "starsurge", p->talent.starsurge, opt ) {}

  starsurge_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : druid_spell_t( n, p, s, opt ), goldrinn( nullptr ), moonkin_form_in_precombat( false )
  {
    form_mask |= NO_FORM; // spec version can be cast with no form despite spell data form mask

    if ( p->talent.power_of_goldrinn.ok() )
    {
      auto suf = get_suffix( name_str, "starsurge" );
      goldrinn = p->get_secondary_action_n<goldrinns_fang_t>( "goldrinns_fang" + suf );
      add_child( goldrinn );
    }
  }

  void init() override
  {
    // Need to set is_precombat first to bypass action_t::init() checks
    if ( action_list && action_list->name_str == "precombat" )
      is_precombat = true;

    druid_spell_t::init();

    if ( is_precombat )
    {
      moonkin_form_in_precombat = range::any_of( p()->precombat_action_list, []( action_t* a ) {
        return util::str_compare_ci( a->name(), "moonkin_form" );
      } );
    }
  }

  timespan_t travel_time() const override
  {
    return is_precombat ? 100_ms : druid_spell_t::travel_time();
  }

  bool ready() override
  {
    if ( !is_precombat )
      return druid_spell_t::ready();

    // in precombat, so hijack standard ready() procedure
    // emulate performing check_form_restriction()
    if ( !moonkin_form_in_precombat )
      return false;

    // emulate performing resource_available( current_resource(), cost() )
    if ( !p()->talent.natures_balance.ok() && p()->options.initial_astral_power < cost() )
      return false;

    return true;
  }

  void schedule_travel( action_state_t* s ) override
  {
    druid_spell_t::schedule_travel( s );

    // do this here immediately after hit_any_target is set so that empowerments are triggered early enough that any
    // effect that follows, such as proccing pulsar, can correctly reset empowerments
    if ( hit_any_target )
      p()->eclipse_handler.cast_starsurge();
  }

  void execute() override
  {
    if ( p()->buff.touch_the_cosmos->up() && !is_free_cast() )
    {
      p()->active.starsurge_cosmos->execute_on_target( target );
      p()->buff.touch_the_cosmos->expire();

      if ( p()->bugs && !is_free_proc() )
        p()->buff.starweavers_weft->expire();

      return;
    }

    if ( !is_free() && p()->buff.starweavers_weft->up() )
    {
      p()->active.starsurge_starweaver->execute_on_target( target );
      p()->buff.starweavers_weft->expire();
      return;
    }

    druid_spell_t::execute();

    if ( goldrinn && rng().roll( p()->talent.power_of_goldrinn->proc_chance() ) )
      goldrinn->execute_on_target( target );

    if ( p()->sets->has_set_bonus( DRUID_BALANCE, T29, B2 ) )
      p()->buff.gathering_starstuff->trigger();

    if ( is_free_proc() )
      return;

    if ( p()->talent.starlord.ok() )
      p()->buff.starlord->trigger();

    if ( p()->talent.rattle_the_stars.ok() )
      p()->buff.rattled_stars->trigger();

    if ( p()->talent.starweaver.ok() )
      p()->buff.starweavers_warp->trigger();
  }
};

// Stellar Flare ============================================================
struct stellar_flare_t : public druid_spell_t
{
  stellar_flare_t( druid_t* p, std::string_view opt )
    : stellar_flare_t( p, "stellar_flare", p->talent.stellar_flare, opt )
  {}

  stellar_flare_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : druid_spell_t( n, p, s, opt )
  {
    dot_name = "stellar_flare";
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    trigger_shooting_stars( d->target );
  }
};

// Sunfire Spell ============================================================
struct sunfire_t : public druid_spell_t
{
  struct sunfire_damage_t : public druid_spell_t
  {
    sunfire_damage_t( druid_t* p ) : druid_spell_t( "sunfire_dmg", p, p->spec.sunfire_dmg )
    {
      dual = background   = true;
      aoe                 = p->talent.improved_sunfire.ok() ? -1 : 0;
      base_aoe_multiplier = 0;
      radius              = data().effectN( 2 ).radius();

      dot_name = "sunfire";
    }

    void trigger_dot( action_state_t* s ) override
    {
      druid_spell_t::trigger_dot( s );

      td( s->target )->debuff.sunfire->trigger( -1, p()->cache.mastery_value(), -1.0, timespan_t::min() );
    }

    void tick( dot_t* d ) override
    {
      druid_spell_t::tick( d );

      trigger_shooting_stars( d->target );
    }

    void last_tick( dot_t* d ) override
    {
      druid_spell_t::last_tick( d );

      td( d->target )->debuff.sunfire->expire();
    }
  };

  action_t* damage;  // Add damage modifiers in sunfire_damage_t, not sunfire_t

  sunfire_t( druid_t* p, std::string_view options_str ) : druid_spell_t( "sunfire", p, p->talent.sunfire, options_str )
  {
    may_miss = false;

    damage = p->get_secondary_action<sunfire_damage_t>( "sunfire_dmg" );
    damage->stats = stats;

    if ( p->spec.astral_power->ok() )
    {
      energize_resource = RESOURCE_ASTRAL_POWER;
      energize_amount = p->spec.astral_power->effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
    }
    else
      energize_type = action_energize::NONE;
  }

  bool has_amount_result() const override { return damage->has_amount_result(); }

  void gain_energize_resource( resource_e rt, double amt, gain_t* gain ) override
  {
    druid_spell_t::gain_energize_resource( rt, amt, gain );

    if ( p()->talent.stellar_innervation.ok() && p()->buff.eclipse_solar->check() )
      p()->resource_gain( rt, amt, p()->gain.stellar_innervation, this );
  }

  void execute() override
  {
    druid_spell_t::execute();

    damage->target = target;
    damage->schedule_execute();
  }
};

// Survival Instincts =======================================================
struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "survival_instincts", p, p->talent.survival_instincts, opt )
  {
    harmful     = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.survival_instincts->trigger();

    if ( p()->talent.matted_fur.ok() )
      p()->buff.matted_fur->trigger();
  }
};

// Syzygy ===================================================================
struct syzygy_t : public druid_spell_t
{
  action_t* flare;

  syzygy_t( druid_t* p ) : druid_spell_t( "syzygy", p, p->find_spell( 361237 ) )
  {
    aoe = -1;
    travel_speed = 75.0;  // guesstimate

    flare = p->get_secondary_action_n<stellar_flare_t>( "stellar_flare_syzygy", p->find_spell( 202347 ), "" );
    flare->name_str_reporting = "stellar_flare";
    add_child( flare );
  }

  void impact( action_state_t* s ) override
  {
    flare->execute_on_target( s->target );  // flare is applied before impact damage

    druid_spell_t::impact( s );
  }
};

// Thorns ===================================================================
struct thorns_t : public druid_spell_t
{
  struct thorns_proc_t : public druid_spell_t
  {
    thorns_proc_t( druid_t* p ) : druid_spell_t( "thorns_hit", p, p->find_spell( 305496 ) )
    {
      background = true;
    }
  };

  struct thorns_attack_event_t : public event_t
  {
    action_t* thorns;
    player_t* target_actor;
    druid_t* source;
    timespan_t attack_period;
    double hit_chance;
    bool randomize_first;

    thorns_attack_event_t( druid_t* player, action_t* thorns_proc, player_t* source, bool randomize = false )
      : event_t( *player ),
        thorns( thorns_proc ),
        target_actor( source ),
        source( player ),
        attack_period( player->options.thorns_attack_period ),
        hit_chance( player->options.thorns_hit_chance ),
        randomize_first( randomize )
    {
      // this will delay the first psudo autoattack by a random amount between 0 and a full attack period
      if ( randomize_first )
        schedule( rng().real() * attack_period );
      else
        schedule( attack_period );
    }

    const char* name() const override { return "Thorns auto attack event"; }

    void execute() override
    {
      // Terminate the rescheduling if the target is dead, or if thorns would run out before next attack

      if ( target_actor->is_sleeping() )
        return;

      thorns->target = target_actor;
      if ( thorns->ready() && thorns->cooldown->up() && rng().roll( hit_chance ) )
        thorns->execute();

      if ( source->buff.thorns->remains() >= attack_period )
        make_event<thorns_attack_event_t>( *source->sim, source, thorns, target_actor, false );
    }
  };

  thorns_proc_t* thorns_proc;

  thorns_t( druid_t* p, std::string_view opt ) : druid_spell_t( "thorns", p, p->find_spell( 305497 ), opt )
  {
    // workaround so that we do not need to enable mana regen
    base_costs[ RESOURCE_MANA ] = 0.0;

    thorns_proc = p->get_secondary_action<thorns_proc_t>( "thorns_hit" );
    thorns_proc->stats = stats;
  }

  bool ready() override
  {
    return false;  // unavailable for now. need to manually allow later
  }

  void execute() override
  {
    p()->buff.thorns->trigger();

    for ( player_t* target : p()->sim->target_non_sleeping_list )
      make_event<thorns_attack_event_t>( *sim, p(), thorns_proc, target, true );

    druid_spell_t::execute();
  }
};

// Warrior of Elune =========================================================
struct warrior_of_elune_t : public druid_spell_t
{
  warrior_of_elune_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "warrior_of_elune", p, p->talent.warrior_of_elune, opt )
  {
    harmful = may_miss = false;
  }

  timespan_t cooldown_duration() const override
  {
    return 0_ms;  // cooldown handled by buff.warrior_of_elune
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.warrior_of_elune->trigger();
  }

  bool ready() override
  {
    if ( p()->buff.warrior_of_elune->check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Wild Charge ==============================================================
struct wild_charge_t : public druid_spell_t
{
  double movement_speed_increase;

  wild_charge_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "wild_charge", p, p->talent.wild_charge, opt ), movement_speed_increase( 5.0 )
  {
    harmful = may_miss = false;
    ignore_false_positive = true;
    range = data().max_range();
    movement_directionality = movement_direction_type::OMNI;
    trigger_gcd = 0_ms;
  }

  void schedule_execute( action_state_t* execute_state ) override
  {
    druid_spell_t::schedule_execute( execute_state );

    // Since Cat/Bear charge is limited to moving towards a target, cancel form if the druid wants to move away. Other
    // forms can already move in any direction they want so they're fine.
    if ( p()->current.movement_direction == movement_direction_type::AWAY )
      p()->shapeshift( NO_FORM );
  }

  void execute() override
  {
    p()->buff.wild_charge_movement->trigger(
        1, movement_speed_increase, 1,
        timespan_t::from_seconds(
            p()->current.distance_to_move /
            ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) ) );

    druid_spell_t::execute();
  }

  bool ready() override
  {
    if ( p()->current.distance_to_move < data().min_range() )  // Cannot charge if the target is too close.
      return false;

    return druid_spell_t::ready();
  }
};

// Wild Mushroom ============================================================
struct wild_mushroom_t : public druid_spell_t
{
  struct fungal_growth_t : public druid_residual_action_t<druid_spell_t>
  {
    fungal_growth_t( druid_t* p ) : base_t( "fungal_growth", p, p->find_spell( 81281 ) )
    {
      residual_mul = p->talent.fungal_growth->effectN( 1 ).percent();
    }

    double base_ta( const action_state_t* s ) const override
    {
      return get_amount( s ) * tick_time( s ) / composite_dot_duration( s );
    }
  };

  struct wild_mushroom_damage_t : public druid_spell_t
  {
    action_t* driver;
    fungal_growth_t* fungal;
    double ap_per;
    double ap_max;

    wild_mushroom_damage_t( druid_t* p, action_t* a )
      : druid_spell_t( "wild_mushroom_damage", p, p->find_spell( 88751 ) ),
        driver( a ),
        fungal( nullptr ),
        ap_per( 5 ),
        ap_max( data().effectN( 2 ).base_value() )
    {
      background = true;
      aoe = -1;

      if ( p->talent.fungal_growth.ok() )
      {
        fungal = p->get_secondary_action<fungal_growth_t>( "fungal_growth" );
        driver->add_child( fungal );
      }
    }

    void execute() override
    {
      druid_spell_t::execute();

      gain_energize_resource( RESOURCE_ASTRAL_POWER, std::min( ap_max, ap_per * num_targets_hit ),
                              driver->energize_gain( execute_state ) );
    }

    void impact( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      if ( fungal && s->result_amount > 0 && result_is_hit( s->result ) )
      {
        fungal->snapshot_and_execute( s, true, [ this, s ]( action_state_t* new_ ) {
          fungal->set_amount( new_, s->result_amount );
        } );
      }
    }
  };

  action_t* damage;
  timespan_t delay;

  wild_mushroom_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "wild_mushroom", p, p->talent.wild_mushroom, opt ), delay( data().duration() )
  {
    harmful = false;

    damage = p->get_secondary_action<wild_mushroom_damage_t>( "wild_mushroom_damage", this );
    damage->stats = stats;
  }

  void execute() override
  {
    druid_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( target )
      .x( target->x_position )
      .y( target->y_position )
      .pulse_time( delay )
      .duration( delay )
      .action( damage ) );
  }
};

// Wrath ====================================================================
struct wrath_t : public druid_spell_t
{
  double gcd_mul;
  double sotf_mul;
  unsigned count;

  wrath_t( druid_t* p, std::string_view opt ) : wrath_t( p, "wrath", opt ) {}

  wrath_t( druid_t* p, std::string_view n, std::string_view opt )
    : druid_spell_t( n, p, p->spec.wrath, opt ),
      gcd_mul( p->query_aura_effect( p->spec.eclipse_solar, A_ADD_PCT_MODIFIER, P_GCD, &data() )->percent() ),
      sotf_mul( p->talent.soul_of_the_forest_moonkin->effectN( 1 ).percent() ),
      count( 0 )
  {
    form_mask = NO_FORM | MOONKIN_FORM;

    if ( energize_resource_() == RESOURCE_ASTRAL_POWER )
      energize_amount = p->spec.astral_power->effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
  }

  void init() override
  {
    druid_spell_t::init();

    if ( p()->talent.umbral_embrace.ok() )
      debug_cast<buffs::umbral_embrace_buff_t*>( p()->buff.umbral_embrace )->actions.push_back( this );
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = druid_spell_t::composite_energize_amount( s );

    if ( energize_amount && sotf_mul && p()->buff.eclipse_solar->check() )
      e *= 1.0 + sotf_mul;

    return e;
  }

  timespan_t travel_time() const override
  {
    if ( !count )
      return druid_spell_t::travel_time();

    // for each additional wrath in precombat apl, reduce the travel time by the cast time
    player->invalidate_cache( CACHE_SPELL_HASTE );
    return std::max( 1_ms, druid_spell_t::travel_time() - base_execute_time * composite_haste() * count );
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    // Move this into parse_buff_effects if it becomes more prevalent. Currently only used for wrath & dash
    if ( p()->buff.eclipse_solar->up() )
      g *= 1.0 + gcd_mul;

    g = std::max( min_gcd, g );

    return g;
  }

  // return false on invulnerable targets as this action is !harmful to allow for self-healing, thus will pass the
  // invulnerable check in action_t::target_ready()
  bool target_ready( player_t* t ) override
  {
    if ( t->debuffs.invulnerable->check() && sim->ignore_invulnerable_targets )
      return false;

    return druid_spell_t::target_ready( t );
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.umbral_embrace->expire();
    p()->buff.gathering_starstuff->expire();

    if ( is_free_proc() )
      return;

    // in druid_t::init_finished(), we set the final wrath of the precombat to have energize type of NONE, so that
    // we can handle the delayed enerigze & eclipse stack triggering here.
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER && energize_type == action_energize::NONE )
    {
      make_event( *sim, 100_ms, [ this ]() {
        p()->eclipse_handler.cast_wrath();
        p()->resource_gain( RESOURCE_ASTRAL_POWER, composite_energize_amount( execute_state ),
                            energize_gain( execute_state ) );
      } );
    }
    else
    {
      p()->eclipse_handler.cast_wrath();
    }
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( p()->active.astral_smolder && s->result_amount > 0 && result_is_hit( s->result ) && s->result == RESULT_CRIT )
    {
      auto smolder = debug_cast<astral_smolder_t*>( p()->active.astral_smolder );

      smolder->snapshot_and_execute( s, false, [ smolder, s ]( action_state_t* new_ ) {
        smolder->set_amount( new_, s->result_amount );
      } );
    }
  }
};

// Convoke the Spirits ======================================================
// NOTE must be defined after all other spells
struct convoke_the_spirits_t : public druid_spell_t
{
  enum convoke_cast_e
  {
    CAST_NONE = 0,
    CAST_OFFSPEC,
    CAST_SPEC,
    CAST_HEAL,
    CAST_MAIN,
    CAST_WRATH,
    CAST_MOONFIRE,
    CAST_RAKE,
    CAST_THRASH_BEAR,
    CAST_FULL_MOON,
    CAST_STARSURGE,
    CAST_STARFALL,
    CAST_PULVERIZE,
    CAST_IRONFUR,
    CAST_MANGLE,
    CAST_FERAL_FRENZY,
    CAST_FEROCIOUS_BITE,
    CAST_THRASH_CAT,
    CAST_SHRED
  };

  struct
  {
    // Multi-spec
    action_t* conv_wrath;
    action_t* conv_moonfire;
    action_t* conv_rake;
    action_t* conv_thrash_bear;
    // Moonkin Form
    action_t* conv_full_moon;
    action_t* conv_starsurge;
    action_t* conv_starfall;
    // Bear Form
    action_t* conv_ironfur;
    action_t* conv_mangle;
    action_t* conv_pulverize;
    // Cat Form
    action_t* conv_feral_frenzy;
    action_t* conv_ferocious_bite;
    action_t* conv_thrash_cat;
    action_t* conv_shred;
    action_t* conv_lunar_inspiration;
  } actions;

  std::vector<convoke_cast_e> cast_list;
  std::vector<convoke_cast_e> offspec_list;
  std::vector<std::pair<convoke_cast_e, double>> chances;

  shuffled_rng_t* deck;

  int max_ticks;
  unsigned main_count;
  unsigned filler_count;
  unsigned dot_count;
  unsigned off_count;
  bool guidance;

  convoke_the_spirits_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "convoke_the_spirits", p, p->talent.convoke_the_spirits, opt ),
      actions(),
      deck( nullptr ),
      max_ticks( 0 ),
      main_count( 0 ),
      filler_count( 0 ),
      dot_count( 0 ),
      off_count( 0 ),
      guidance( p->talent.elunes_guidance.ok() || p->talent.ursocs_guidance.ok() || p->talent.ashamanes_guidance.ok() ||
                p->talent.cenarius_guidance.ok() )
  {
    if ( !p->talent.convoke_the_spirits.ok() )
      return;

    channeled = true;
    harmful = may_miss = false;

    max_ticks = as<int>( util::floor( dot_duration / base_tick_time ) );

    // create deck for exceptional spell cast
    deck = p->get_shuffled_rng( "convoke_the_spirits", 1, guidance ? 2 : p->options.convoke_the_spirits_deck );

    using namespace bear_attacks;
    using namespace cat_attacks;

    // Create actions used by all specs
    actions.conv_wrath       = get_convoke_action<wrath_t>( "wrath", "" );
    actions.conv_moonfire    = get_convoke_action<moonfire_t>( "moonfire", "" );
    actions.conv_rake        = get_convoke_action<rake_t>( "rake", p->find_spell( 1822 ), "" );
    actions.conv_thrash_bear = get_convoke_action<thrash_bear_t>( "thrash_bear", p->find_spell( 77758 ), "" );

    // Call form-specific initialization to create necessary actions & setup variables
    if ( p->find_action( "moonkin_form" ) )
      _init_moonkin();

    if ( p->find_action( "bear_form" ) )
      _init_bear();

    if ( p->find_action( "cat_form" ) )
      _init_cat();
  }

  template <typename T, typename... Ts>
  T* get_convoke_action( std::string n, Ts&&... args )
  {
    auto a = p()->get_secondary_action_n<T>( n + "_convoke", std::forward<Ts>( args )... );
    if ( a->name_str_reporting.empty() )
      a->name_str_reporting = n;
    a->set_free_cast( free_spell_e::CONVOKE );
    stats->add_child( a->stats );
    a->gain = gain;
    a->proc = true;
    return a;
  }

  action_t* convoke_action_from_type( convoke_cast_e type )
  {
    switch ( type )
    {
      case CAST_MOONFIRE:
        if ( p()->buff.cat_form->check() )
          return actions.conv_lunar_inspiration;
        else
          return actions.conv_moonfire;
      case CAST_WRATH:          return actions.conv_wrath;
      case CAST_RAKE:           return actions.conv_rake;
      case CAST_THRASH_BEAR:    return actions.conv_thrash_bear;
      case CAST_FULL_MOON:      return actions.conv_full_moon;
      case CAST_STARSURGE:      return actions.conv_starsurge;
      case CAST_STARFALL:       return actions.conv_starfall;
      case CAST_PULVERIZE:      return actions.conv_pulverize;
      case CAST_IRONFUR:        return actions.conv_ironfur;
      case CAST_MANGLE:         return actions.conv_mangle;
      case CAST_FERAL_FRENZY:   return actions.conv_feral_frenzy;
      case CAST_FEROCIOUS_BITE: return actions.conv_ferocious_bite;
      case CAST_THRASH_CAT:     return actions.conv_thrash_cat;
      case CAST_SHRED:          return actions.conv_shred;
      default: return nullptr;  // heals will fall through and return as null
    }
  }

  convoke_cast_e get_cast_from_dist( std::vector<std::pair<convoke_cast_e, double>> chances )
  {
    auto _sum = []( double a, std::pair<convoke_cast_e, double> b ) { return a + b.second; };

    auto roll = rng().range( 0.0, std::accumulate( chances.begin(), chances.end(), 0.0, _sum ) );

    for ( auto it = chances.begin(); it != chances.end(); it++ )
      if ( roll < std::accumulate( chances.begin(), it + 1, 0.0, _sum ) )
        return it->first;

    return CAST_NONE;
  }

  double composite_haste() const override { return 1.0; }

  void _init_cat()
  {
    using namespace cat_attacks;

    actions.conv_feral_frenzy   = get_convoke_action<feral_frenzy_t>( "feral_frenzy", p()->find_spell( 274837 ), "" );
    actions.conv_ferocious_bite = get_convoke_action<ferocious_bite_t>( "ferocious_bite", "" );
    actions.conv_thrash_cat     = get_convoke_action<thrash_cat_t>( "thrash_cat", p()->find_spell( 106830 ), "" );
    actions.conv_shred          = get_convoke_action<shred_t>( "shred", "" );
    // LI is a talent but the spell id is hardcoded into the constructor, so we don't need to explictly pass it here
    actions.conv_lunar_inspiration = get_convoke_action<lunar_inspiration_t>( "lunar_inspiration", "" );
  }

  void _init_moonkin()
  {
    actions.conv_full_moon = get_convoke_action<full_moon_t>( "full_moon", p()->find_spell( 274283 ), "" );
    actions.conv_starfall  = get_convoke_action<starfall_t>( "starfall", p()->find_spell( 191034 ), "" );
    actions.conv_starsurge = get_convoke_action<starsurge_t>( "starsurge", p()->find_spell( 78674 ), "" );
  }

  void _init_bear()
  {
    using namespace bear_attacks;

    actions.conv_ironfur   = get_convoke_action<ironfur_t>( "ironfur", p()->find_spell( 192081 ), "" );
    actions.conv_mangle    = get_convoke_action<mangle_t>( "mangle", "" );
    actions.conv_pulverize = get_convoke_action<pulverize_t>( "pulverize", p()->find_spell( 80313 ), "" );
  }

  void insert_exceptional( convoke_cast_e cast )
  {
    if ( !deck->trigger() )
      return;

    // Restoration with CS (only 9 ticks) seem to only succeed ~85% of the time
    if ( max_ticks <= 9 && !rng().roll( p()->options.celestial_spirits_exceptional_chance ) )
      return;

    cast_list.push_back( cast );
  }

  void _execute_bear()
  {
    main_count   = 0;
    offspec_list = { CAST_HEAL, CAST_HEAL, CAST_RAKE, CAST_WRATH };
    chances      = { { CAST_THRASH_BEAR, guidance ? 0.85 : 0.95 },
                     { CAST_IRONFUR, 1.0 },
                     { CAST_MANGLE, 1.0 }
                   };

    cast_list.insert( cast_list.end(),
                      static_cast<int>( rng().range( guidance ? 3.5 : 5, guidance ? 6 : 7 ) ),
                      CAST_OFFSPEC );

    insert_exceptional( CAST_PULVERIZE );
  }

  convoke_cast_e _tick_bear( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type;

    // convoke will not cast pulverize if it's already up on the target
    if ( type_ == CAST_PULVERIZE && td( target )->debuff.pulverize->check() )
      type_ = CAST_SPEC;

    if ( type_ == CAST_OFFSPEC && !offspec_list.empty() )
      type_ = offspec_list.at( rng().range( offspec_list.size() ) );
    else if ( type_ == CAST_SPEC )
    {
      auto dist = chances;  // local copy of distribution

      std::vector<player_t*> mf_tl;  // separate list for mf targets without a dot;
      for ( const auto& t : tl )
        if ( !td( t )->dots.moonfire->is_ticking() )
          mf_tl.push_back( t );

      if ( !mf_tl.empty() )
        dist.emplace_back( std::make_pair( CAST_MOONFIRE, ( main_count ? 0.25 : 1.0 ) / ( guidance ? 2.0 : 1.0 ) ) );

      type_ = get_cast_from_dist( dist );

      if ( type_ == CAST_MOONFIRE )
        conv_tar = mf_tl.at( rng().range( mf_tl.size() ) );  // mf has it's own target list
    }

    if ( !conv_tar )
      conv_tar = tl.at( rng().range( tl.size() ) );

    if ( type_ == CAST_RAKE && td( conv_tar )->dots.rake->is_ticking() )
      type_ = CAST_WRATH;

    if ( type_ == CAST_MOONFIRE )
      main_count++;

    return type_;
  }

  inline static size_t _clamp_and_cast( double x, size_t min, size_t max )
  {
    if ( x < min )
      return min;
    if ( x > max )
      return max;
    return static_cast<size_t>( x );
  }

  void _execute_cat()
  {
    offspec_list = { CAST_HEAL, CAST_HEAL, CAST_WRATH, CAST_MOONFIRE };
    chances      = { { CAST_SHRED, 0.10 },
                     { CAST_THRASH_CAT, 0.0588 },
                     { CAST_RAKE, 0.22 }
                   };

    cast_list.insert( cast_list.end(),
                      static_cast<size_t>( rng().range( guidance ? 2.5 : 4, guidance ? 7.5 : 9 ) ),
                      CAST_OFFSPEC );

    insert_exceptional( CAST_FERAL_FRENZY );

    cast_list.insert(
        cast_list.end(),
        _clamp_and_cast( rng().gauss( guidance ? 3 : 4.2, guidance ? 0.8 : 0.9360890055, true ), 0,
                         max_ticks - cast_list.size() ),
        CAST_MAIN );
  }

  convoke_cast_e _tick_cat( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type;

    if ( base_type == CAST_OFFSPEC && !offspec_list.empty() )
      type_ = offspec_list.at( rng().range( offspec_list.size() ) );
    else if ( base_type == CAST_MAIN )
      type_ = CAST_FEROCIOUS_BITE;
    else if ( base_type == CAST_SPEC )
    {
      auto dist = chances;

      type_ = get_cast_from_dist( dist );
    }

    conv_tar = tl.at( rng().range( tl.size() ) );

    auto target_data = td( conv_tar );

    if ( type_ == CAST_MOONFIRE && target_data->dots.lunar_inspiration->is_ticking() )
      type_ = CAST_WRATH;
    else if ( type_ == CAST_RAKE && target_data->dots.rake->is_ticking() )
      type_ = CAST_SHRED;

    return type_;
  }

  void _execute_moonkin()
  {
    cast_list.insert( cast_list.end(), 5 - ( guidance ? 1 : 0 ), CAST_HEAL );
    off_count    = 0;
    main_count   = 0;
    dot_count    = 0;
    filler_count = 0;

    insert_exceptional( CAST_FULL_MOON );
  }

  convoke_cast_e _tick_moonkin( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type;
    std::vector<std::pair<convoke_cast_e, double>> dist;
    unsigned adjust = guidance ? 1 : 0;

    conv_tar = tl.at( rng().range( tl.size() ) );

    if ( type_ == CAST_SPEC )
    {
      bool add_more = true;

      if ( !p()->buff.starfall->check() )
      {
        dist.emplace_back( std::make_pair( CAST_STARFALL, 3 + adjust ) );
        add_more = false;
      }

      std::vector<player_t*> mf_tl;  // separate list for mf targets without a dot;
      for ( const auto& t : tl )
        if ( !td( t )->dots.moonfire->is_ticking() )
          mf_tl.push_back( t );

      if ( !mf_tl.empty() )
      {
        if ( dot_count < ( 4 - adjust ) )
          dist.emplace_back( std::make_pair( CAST_MOONFIRE, 5.5 ) );
        else if ( dot_count < ( 5 - adjust ) )
          dist.emplace_back( std::make_pair( CAST_MOONFIRE, 1.0 ) );
      }

      if ( add_more )
      {
        if ( filler_count < ( 3 - adjust ) )
          dist.emplace_back( std::make_pair( CAST_WRATH, 5.5 ) );
        else if ( filler_count < ( 4 - adjust ) )
          dist.emplace_back( std::make_pair( CAST_WRATH, 3.5 ) );
        else if ( filler_count < ( 5 - adjust ) )
          dist.emplace_back( std::make_pair( CAST_WRATH, 1.0 ) );
      }

      if ( main_count < 3 - adjust )
        dist.emplace_back( std::make_pair( CAST_STARSURGE, 6.0 ) );
      else if ( main_count < 4 - adjust )
        dist.emplace_back( std::make_pair( CAST_STARSURGE, 3.0 ) );
      else if ( main_count < 5 - adjust )
        dist.emplace_back( std::make_pair( CAST_STARSURGE, 1.0 ) );
      else if ( main_count < 6 - adjust )
        dist.emplace_back( std::make_pair( CAST_STARSURGE, 0.5 ) );

      if ( filler_count < ( 4 - adjust ) )
        dist.emplace_back( std::make_pair( CAST_WRATH, 4.0 ) );
      else if ( filler_count < ( 5 - adjust ) )
        dist.emplace_back( std::make_pair( CAST_WRATH, 2.0 ) );
      else if ( filler_count < ( 6 - adjust ) )
        dist.emplace_back( std::make_pair( CAST_WRATH, 1.0 ) );
      else if ( filler_count < ( 7 - adjust ) )
        dist.emplace_back( std::make_pair( CAST_WRATH, 0.2 ) );

      if ( off_count < ( 6 - adjust ) )
        dist.emplace_back( std::make_pair( CAST_HEAL, 0.8 ) );
      else if ( off_count < ( 7 - adjust ) )
        dist.emplace_back( std::make_pair( CAST_HEAL, 0.4 ) );

      type_ = get_cast_from_dist( dist );

      if ( type_ == CAST_MOONFIRE )
        conv_tar = mf_tl.at( rng().range( mf_tl.size() ) );
    }

    if ( type_ == CAST_STARSURGE )
      main_count++;
    else if ( type_ == CAST_WRATH )
      filler_count++;
    else if ( type_ == CAST_MOONFIRE )
      dot_count++;
    else if ( type_ == CAST_HEAL )
      off_count++;

    return type_;
  }

  void execute() override
  {
    // Generic routine
    druid_spell_t::execute();
    p()->buff.convoke_the_spirits->trigger();

    cast_list.clear();
    main_count = 0;

    // form-specific execute setup
    if ( p()->buff.bear_form->check() )
      _execute_bear();
    else if ( p()->buff.moonkin_form->check() )
      _execute_moonkin();
    else if ( p()->buff.cat_form->check() )
      _execute_cat();

    cast_list.insert( cast_list.end(), max_ticks - cast_list.size(), CAST_SPEC );
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    // The last partial tick does nothing
    if ( d->time_to_tick() < base_tick_time )
      return;

    // form-agnostic
    action_t* conv_cast = nullptr;
    player_t* conv_tar  = nullptr;

    auto it   = cast_list.begin() + rng().range( cast_list.size() );
    auto type = *it;
    cast_list.erase( it );

    std::vector<player_t*> tl = target_list();
    if ( tl.empty() )
      return;

    // Do form-specific spell selection
    if ( p()->buff.moonkin_form->check() )
      type = _tick_moonkin( type, tl, conv_tar );
    else if ( p()->buff.bear_form->check() )
      type = _tick_bear( type, tl, conv_tar );
    else if ( p()->buff.cat_form->check() )
      type = _tick_cat( type, tl, conv_tar );

    conv_cast = convoke_action_from_type( type );
    if ( !conv_cast )
      return;

    conv_cast->execute_on_target( conv_tar );
  }

  void last_tick( dot_t* d ) override
  {
    druid_spell_t::last_tick( d );

    p()->buff.convoke_the_spirits->expire();
  }

  bool usable_moving() const override { return true; }
};

// Covenant Spells ==========================================================

// Kindred Spirits ==========================================================
struct kindred_empowerment_t : public druid_spell_t
{
  kindred_empowerment_t( druid_t* p, std::string_view n )
    : druid_spell_t( n, p, p->cov.kindred_empowerment_damage )
  {
    background = dual = true;
    may_miss = callbacks = false;
  }
};

struct kindred_spirits_t : public druid_spell_t
{
  buff_t* buff_on_cast;

  kindred_spirits_t( druid_t* p, std::string_view options_str )
    : druid_spell_t( "empower_bond", p, p->cov.empower_bond, options_str ),
      buff_on_cast( p->buff.kindred_empowerment_energize )
  {
    if ( !p->cov.kyrian->ok() )
      return;

    harmful = false;

    if ( !p->active.kindred_empowerment )
    {
      p->active.kindred_empowerment =
          p->get_secondary_action_n<kindred_empowerment_t>( "kindred_empowerment" );
      add_child( p->active.kindred_empowerment );

      p->active.kindred_empowerment_partner =
          p->get_secondary_action_n<kindred_empowerment_t>( "kindred_empowerment_partner" );
      add_child( p->active.kindred_empowerment_partner );
    }

    if ( p->conduit.deep_allegiance->ok() )
      cooldown->duration *= 1.0 + p->conduit.deep_allegiance.percent();

    if ( p->options.lone_empowerment )
    {
      switch ( p->specialization() )
      {
        case DRUID_BALANCE:
        case DRUID_FERAL:
          buff_on_cast = p->buff.lone_empowerment;
          break;
        default:
          sim->error( "Lone empowerment is only supported for DPS specializations." );
          break;
      }
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    buff_on_cast->trigger();
  }
};

// Ravenous Frenzy ==========================================================
struct ravenous_frenzy_t : public druid_spell_t
{
  ravenous_frenzy_t( druid_t* player, std::string_view opt )
    : druid_spell_t( "ravenous_frenzy", player, player->cov.venthyr, opt )
  {
    if ( !player->cov.venthyr->ok() )
      return;

    harmful      = false;
    dot_duration = 0_ms;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.ravenous_frenzy->trigger();
  }
};
}  // end namespace spells

namespace auto_attacks
{
template <typename Base>
struct druid_melee_t : public Base
{
  using ab = Base;
  using base_t = druid_melee_t<Base>;

  double ooc_chance;

  druid_melee_t( std::string_view n, druid_t* p ) : Base( n, p ), ooc_chance( 0.0 )
  {
    ab::may_crit = ab::may_glance = ab::background = ab::repeating = ab::is_auto_attack = true;
    ab::allow_class_ability_procs = ab::not_a_proc = true;

    ab::school = SCHOOL_PHYSICAL;
    ab::trigger_gcd = 0_ms;
    ab::special = false;
    ab::weapon_multiplier = 1.0;

    ab::apply_buff_effects();
    ab::apply_dot_debuffs();

    // Auto attack mods
    ab::parse_passive_effects( p->spec.guardian );
    ab::parse_passive_effects( p->talent.killer_instinct );
    ab::range += p->talent.astral_influence->effectN( 2 ).base_value();

    // 7.00 PPM via community testing (~368k auto attacks)
    // https://docs.google.com/spreadsheets/d/1vMvlq1k3aAuwC1iHyDjqAneojPZusdwkZGmatuWWZWs/edit#gid=1097586165
    if ( p->talent.omen_of_clarity_cat.ok() )
      ooc_chance = 7.00;

    if ( p->talent.moment_of_clarity.ok() )
      ooc_chance *= 1.0 + p->talent.moment_of_clarity->effectN( 2 ).percent();
  }

  timespan_t execute_time() const override
  {
    if ( !ab::player->in_combat )
      return 10_ms;

    return ab::execute_time();
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ooc_chance && ab::result_is_hit( s->result ) )
    {
      int active = ab::p()->buff.clearcasting_cat->check();
      double chance = ab::weapon->proc_chance_on_swing( ooc_chance );

      // Internal cooldown is handled by buff.
      if ( ab::p()->buff.clearcasting_cat->trigger( 1, buff_t::DEFAULT_VALUE(), chance ) )
      {
        ab::p()->proc.clearcasting->occur();

        for ( int i = 0; i < active; i++ )
          ab::p()->proc.clearcasting_wasted->occur();
      }
    }
  }
};

// Auto Attack ==============================================================
struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( druid_t* player, std::string_view opt ) : melee_attack_t( "auto_attack", player, spell_data_t::nil() )
  {
    parse_options( opt );

    trigger_gcd = 0_ms;
    ignore_false_positive = use_off_gcd = true;
  }

  void execute() override
  {
    player->main_hand_attack->weapon = &( player->main_hand_weapon );
    player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    player->main_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;

    if ( !player->main_hand_attack )
      return false;

    return ( player->main_hand_attack->execute_event == nullptr );  // not swinging
  }
};

// Caster Melee Attack ======================================================
struct caster_melee_t : public druid_melee_t<druid_attack_t<melee_attack_t>>
{
  caster_melee_t( druid_t* p ) : base_t( "caster_melee", p ) {}
};

// Bear Melee Attack ========================================================
struct bear_melee_t : public druid_melee_t<bear_attacks::bear_attack_t>
{
  bear_melee_t( druid_t* p ) : base_t( "bear_melee", p )
  {
    base_t::form_mask = form_e::BEAR_FORM;

    base_t::energize_type = action_energize::ON_HIT;
    base_t::energize_resource = resource_e::RESOURCE_RAGE;
    base_t::energize_amount = 4.0;
  }

  void execute() override
  {
    base_t::execute();

    if ( base_t::hit_any_target && base_t::p()->talent.tooth_and_claw.ok() )
      base_t::p()->buff.tooth_and_claw->trigger();
  }
};

// Cat Melee Attack =========================================================
struct cat_melee_t : public druid_melee_t<cat_attacks::cat_attack_t>
{
  cat_melee_t( druid_t* p ) : base_t( "cat_melee", p )
  {
    base_t::form_mask = form_e::CAT_FORM;

    // parse this here so AA modifier gets applied after is_auto_attack is set true
    base_t::snapshots.tigers_fury = base_t::parse_persistent_buff_effects<const spell_data_t*>(
        p->buff.tigers_fury, 0U, true, p->talent.carnivorous_instinct );
  }
};
}  // namespace auto_attacks

// ==========================================================================
// Druid Helper Functions & Structures
// ==========================================================================

// Brambles Absorb/Reflect Handler ==========================================
double brambles_handler( const action_state_t* s )
{
  auto p = static_cast<druid_t*>( s->target );
  assert( p->talent.brambles.ok() );

  /* Calculate the maximum amount absorbed. This is not affected by
     versatility (and likely other player modifiers). */
  double weapon_ap = 0.0;

  if ( p->buff.cat_form->check() )
    weapon_ap = p->cat_weapon.dps * WEAPON_POWER_COEFFICIENT;
  else if ( p->buff.bear_form->check() )
    weapon_ap = p->bear_weapon.dps * WEAPON_POWER_COEFFICIENT;
  else
    weapon_ap = p->main_hand_weapon.dps * WEAPON_POWER_COEFFICIENT;

  double attack_power = ( p->cache.attack_power() + weapon_ap ) * p->composite_attack_power_multiplier();

  // Brambles coefficient is not in spelldata :(
  double absorb_cap = attack_power * 0.06;

  // Calculate actual amount absorbed.
  double amount_absorbed = std::min( s->result_mitigated, absorb_cap );

  // Prevent self-harm
  if ( s->action->player != p )
  {
    // Schedule reflected damage.
    p->active.brambles->base_dd_min = p->active.brambles->base_dd_max = amount_absorbed;
    action_state_t* ref_s = p->active.brambles->get_state();
    ref_s->target         = s->action->player;
    p->active.brambles->snapshot_state( ref_s, result_amount_type::DMG_DIRECT );
    p->active.brambles->schedule_execute( ref_s );
  }

  return amount_absorbed;
}

// Earthwarden Absorb Handler ===============================================
double earthwarden_handler( const action_state_t* s )
{
  if ( s->action->special )
    return 0;

  auto p = static_cast<druid_t*>( s->target );
  assert( p->talent.earthwarden.ok() );

  if ( !p->buff.earthwarden->up() )
    return 0;

  double absorb = s->result_amount * p->buff.earthwarden->check_value();
  p->buff.earthwarden->decrement();

  return absorb;
}

// Kindred Empowerment Pool Absorb Handler ==================================
double kindred_empowerment_handler( const action_state_t* s )
{
  auto p = static_cast<druid_t*>( s->target );
  auto b = debug_cast<buffs::kindred_empowerment_buff_t*>( p->buff.kindred_empowerment );

  if ( !b->check() )
    return 0.0;

  auto absorb = s->result_amount * b->use_pct;

  p->sim->print_debug( "KINDRED_EMPOWERMENT: Absorbing {} from pool of {}", absorb, b->pool );

  absorb = std::min( absorb, b->pool - 1 );
  b->pool -= absorb;

  return absorb;
}

// Rage of the Sleeper Absorb/Reflect Handler ===============================
double rage_of_the_sleeper_handler( const action_state_t* s )
{
  auto p = static_cast<druid_t*>( s->target );

  if ( !p->buff.rage_of_the_sleeper->up() )
    return 0;

  double absorb = s->result_amount * p->buff.rage_of_the_sleeper->check_value();

  if ( s->action->player != p )
    p->active.rage_of_the_sleeper_reflect->execute_on_target( s->action->player );

  return absorb;
}

// Persistent Delay Event ===================================================
// Delay triggering the event a random amount. This prevents fixed-period drivers from ticking at the exact same times
// on every iteration. Buffs that use the event to activate should implement tick_zero-like behavior.
struct persistent_delay_event_t : public event_t
{
  std::function<void()> exec_fn;

  persistent_delay_event_t( druid_t* p, buff_t* b )
    : persistent_delay_event_t( p, [ b ]() { b->execute(); }, b->buff_period )
  {}

  persistent_delay_event_t( druid_t* p, std::function<void()> fn, timespan_t d )
    : event_t( *p ), exec_fn( std::move( fn ) )
  {
    schedule( rng().real() * d );
  }

  const char* name() const override { return "persistent_event_delay"; }

  void execute() override { exec_fn(); }
};

// Denizen of the Dream Proxy Action ========================================
struct denizen_of_the_dream_t : public action_t
{
  druid_t* druid;
  timespan_t dur;

  denizen_of_the_dream_t( druid_t* p )
    : action_t( action_e::ACTION_OTHER, "denizen_of_the_dream", p, p->talent.denizen_of_the_dream ),
      druid( p ),
      dur( p->find_spell( 394076 )->duration() )
  {}

  result_e calculate_result( action_state_t* ) const override
  {
    return result_e::RESULT_NONE;
  }

  void execute() override
  {
    action_t::execute();

    druid->pets.denizen_of_the_dream.spawn( dur );
  }
};

// Lycara's Fleeting Glimpse Proxy Action ===================================
struct lycaras_fleeting_glimpse_t : public action_t
{
  druid_t* druid;
  action_t* moonkin;
  action_t* cat;
  action_t* bear;
  action_t* tree;

  lycaras_fleeting_glimpse_t( druid_t* p )
    : action_t( action_e::ACTION_OTHER, "lycaras_fleeting_glimpse", p, p->legendary.lycaras_fleeting_glimpse ),
      druid( p ),
      moonkin( nullptr ),
      cat( nullptr ),
      bear( nullptr ),
      tree( nullptr )
  {
    moonkin = get_lycaras_action<spells::starfall_t>( "starfall", druid->find_spell( 191034 ), "" );
    cat     = get_lycaras_action<cat_attacks::primal_wrath_t>( "primal_wrath", druid->find_spell( 285381 ), "" );
    bear    = get_lycaras_action<spells::barkskin_t>( "barkskin", "" );
  }

  template <typename T, typename... Ts>
  T* get_lycaras_action( std::string n, Ts&&... args )
  {
    auto a = druid->get_secondary_action_n<T>( n + "_lycaras", std::forward<Ts>( args )... );
    a->name_str_reporting = n;
    a->set_free_cast( free_spell_e::LYCARAS );
    add_child( a );
    return a;
  }

  void execute() override
  {
    action_t* a;

    if ( druid->buff.moonkin_form->check() )
      a = moonkin;
    else if ( druid->buff.cat_form->check() )
      a = cat;
    else if ( druid->buff.bear_form->check() )
      a = bear;
    else
      a = tree;

    if ( a && druid->target )
      a->execute_on_target( druid->target );

    druid->lycaras_event =
        make_event( *sim, timespan_t::from_seconds( druid->buff.lycaras_fleeting_glimpse->default_value ),
                    [ this ]() { druid->buff.lycaras_fleeting_glimpse->trigger(); } );
  }
};

// The Natural Order's Will Proxy Action ====================================
struct the_natural_orders_will_t : public action_t
{
  action_t* ironfur;
  action_t* frenzied;

  the_natural_orders_will_t( druid_t* p )
    : action_t( action_e::ACTION_OTHER, "the_natural_orders_will", p, p->legendary.the_natural_orders_will )
  {
    ironfur = p->get_secondary_action_n<bear_attacks::ironfur_t>( "ironfur_natural", p->find_spell( 192081 ), "" )
                  ->set_free_cast( free_spell_e::NATURAL );
    ironfur->name_str_reporting = "ironfur";

    frenzied = p->get_secondary_action_n<heals::frenzied_regeneration_t>( "frenzied_regeneration_natural",
                                                                          p->find_spell( 22842 ), "" )
                   ->set_free_cast( free_spell_e::NATURAL );
    frenzied->name_str_reporting = "frenzied_regeneration";
  }

  void execute() override
  {
    ironfur->execute();
    frenzied->execute();
  }
};

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::activate ========================================================
void druid_t::activate()
{
  if ( talent.predator.ok() )
  {
    register_on_kill_callback( [ this ]( player_t* t ) {
      auto td = get_target_data( t );
      if ( td->dots.thrash_cat->is_ticking() || td->dots.rip->is_ticking() || td->dots.rake->is_ticking() )
      {
        if ( !cooldown.tigers_fury->down() )
          proc.predator_wasted->occur();

        cooldown.tigers_fury->reset( true );
        proc.predator->occur();
      }
    } );
  }

  if ( sim->ignore_invulnerable_targets )
  {
    if ( legendary.lycaras_fleeting_glimpse->ok() )
    {
      sim->target_non_sleeping_list.register_callback( [ this ]( player_t* ) {
        if ( sim->target_non_sleeping_list.empty() )
        {
          if ( buff.lycaras_fleeting_glimpse->check() )
          {
            lycaras_event_remains = buff.lycaras_fleeting_glimpse->remains();
            buff.lycaras_fleeting_glimpse->expire();
          }
          else if ( lycaras_event )
          {
            // If only the event is up (and not the buff) add the base buff duration so we can determine whether to
            // trigger the event or the buff when a target becomes active and we back 'in combat'
            lycaras_event_remains = lycaras_event->remains() + buff.lycaras_fleeting_glimpse->buff_duration();
            event_t::cancel( lycaras_event );
          }
        }
        else
        {
          if ( lycaras_event_remains > buff.lycaras_fleeting_glimpse->buff_duration() )  // trigger the event
          {
            lycaras_event =
                make_event( *sim, lycaras_event_remains - buff.lycaras_fleeting_glimpse->buff_duration(), [ this ]() {
                  buff.lycaras_fleeting_glimpse->trigger();
                } );
            lycaras_event_remains = 0_ms;
          }
          else if ( lycaras_event_remains > 0_ms )  // trigger the buff
          {
            buff.lycaras_fleeting_glimpse->trigger( lycaras_event_remains );
            lycaras_event_remains = 0_ms;
          }
        }
      } );
    }

    if ( talent.natures_balance.ok() )
    {
      // TODO: make this actually work since buff.nb's tick_callback uses hard coded value instead of default_value()
      sim->target_non_sleeping_list.register_callback( [ this ]( player_t* ) {
        if ( sim->target_non_sleeping_list.empty() )
          buff.natures_balance->current_value *= 3.0;
        else
          buff.natures_balance->current_value = buff.natures_balance->default_value;
      } );
    }
  }

  player_t::activate();
}

// druid_t::create_action  ==================================================
action_t* druid_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace auto_attacks;
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  // Baseline Abilities
  if ( name == "auto_attack"           ) return new           auto_attack_t( this, options_str );
  if ( name == "bear_form"             ) return new             bear_form_t( this, options_str );
  if ( name == "cat_form"              ) return new              cat_form_t( this, options_str );
  if ( name == "cancelform"            ) return new           cancel_form_t( this, options_str );
  if ( name == "entangling_roots"      ) return new      entangling_roots_t( this, options_str );
  if ( name == "ferocious_bite"        ) return new        ferocious_bite_t( this, options_str );
  if ( name == "growl"                 ) return new                 growl_t( this, options_str );
  if ( name == "mangle"                ) return new                mangle_t( this, options_str );
  if ( name == "mark_of_the_wild"      ) return new      mark_of_the_wild_t( this, options_str );
  if ( name == "moonfire"              ) return new              moonfire_t( this, options_str );
  if ( name == "prowl"                 ) return new                 prowl_t( this, options_str );
  if ( name == "regrowth"              ) return new              regrowth_t( this, options_str );
  if ( name == "shred"                 ) return new                 shred_t( this, options_str );
  if ( name == "wrath"                 ) return new                 wrath_t( this, options_str );

  // Class Talents
  if ( name == "barkskin"              ) return new              barkskin_t( this, options_str );
  if ( name == "dash" ||
       name == "tiger_dash"            ) return new                  dash_t( this, options_str );
  if ( name == "frenzied_regeneration" ) return new frenzied_regeneration_t( this, options_str );
  if ( name == "heart_of_the_wild"     ) return new     heart_of_the_wild_t( this, options_str );
  if ( name == "incapacitating_roar"   ) return new   incapacitating_roar_t( this, options_str );
  if ( name == "innervate"             ) return new             innervate_t( this, options_str );
  if ( name == "ironfur"               ) return new               ironfur_t( this, options_str );
  if ( name == "maim"                  ) return new                  maim_t( this, options_str );
  if ( name == "moonkin_form"          ) return new          moonkin_form_t( this, options_str );
  if ( name == "natures_vigil"         )
  {
    if ( specialization() == DRUID_RESTORATION )
      return new natures_vigil_t<druid_spell_t>( this, options_str );
    else
      return new natures_vigil_t<druid_heal_t>( this, options_str );
  }
  if ( name == "rake"                  ) return new                  rake_t( this, options_str );
  if ( name == "rejuvenation"          ) return new          rejuvenation_t( this, options_str );
  if ( name == "remove_corruption"     ) return new     remove_corruption_t( this, options_str );
  if ( name == "renewal"               ) return new               renewal_t( this, options_str );
  if ( name == "rip"                   ) return new                   rip_t( this, options_str );
  if ( name == "skull_bash"            ) return new            skull_bash_t( this, options_str );
  if ( name == "stampeding_roar"       ) return new       stampeding_roar_t( this, options_str );
  if ( name == "starfire"              ) return new              starfire_t( this, options_str );
  if ( name == "starsurge"             )
  {
    if ( specialization() == DRUID_BALANCE )
      return new starsurge_t( this, options_str );
    else
      return new starsurge_offspec_t( this, options_str );
  }
  if ( name == "sunfire"               ) return new               sunfire_t( this, options_str );
  if ( name == "swipe"                 ) return new           swipe_proxy_t( this, options_str );
  if ( name == "swipe_bear"            ) return new            swipe_bear_t( this, options_str );
  if ( name == "swipe_cat"             ) return new             swipe_cat_t( this, options_str );
  if ( name == "thrash"                ) return new          thrash_proxy_t( this, options_str );
  if ( name == "thrash_bear"           ) return new           thrash_bear_t( this, options_str );
  if ( name == "thrash_cat"            ) return new            thrash_cat_t( this, options_str );
  if ( name == "wild_charge"           ) return new           wild_charge_t( this, options_str );
  if ( name == "wild_growth"           ) return new           wild_growth_t( this, options_str );

  // Multispec Talents
  if ( name == "adaptive_swarm"        ) return new        adaptive_swarm_t( this, options_str );
  if ( name == "berserk" )
  {
    if ( specialization() == DRUID_GUARDIAN )
      return new berserk_bear_t( this, options_str );
    else if ( specialization() == DRUID_FERAL )
      return new berserk_cat_t( this, options_str );
  }
  if ( name == "convoke_the_spirits"   ) return new   convoke_the_spirits_t( this, options_str );
  if ( name == "incarnation"           )
  {
    switch ( specialization() )
    {
      case DRUID_BALANCE:                return new   incarnation_moonkin_t( this, options_str );
      case DRUID_FERAL:                  return new       incarnation_cat_t( this, options_str );
      case DRUID_GUARDIAN:               return new      incarnation_bear_t( this, options_str );
      case DRUID_RESTORATION:            return new      incarnation_tree_t( this, options_str );
      default: break;
    }
  }
  if ( name == "survival_instincts"    ) return new    survival_instincts_t( this, options_str );

  // Balance
  if ( name == "astral_communion"      ) return new      astral_communion_t( this, options_str );
  if ( name == "celestial_alignment"   ) return new   celestial_alignment_t( this, options_str );
  if ( name == "force_of_nature"       ) return new       force_of_nature_t( this, options_str );
  if ( name == "fury_of_elune"         ) return new         fury_of_elune_t( this, options_str );
  if ( name == "new_moon"              ) return new              new_moon_t( this, options_str );
  if ( name == "half_moon"             ) return new             half_moon_t( this, options_str );
  if ( name == "full_moon"             ) return new             full_moon_t( this, options_str );
  if ( name == "moons"                 ) return new            moon_proxy_t( this, options_str );
  if ( name == "solar_beam"            ) return new            solar_beam_t( this, options_str );
  if ( name == "starfall"              ) return new              starfall_t( this, options_str );
  if ( name == "stellar_flare"         ) return new         stellar_flare_t( this, options_str );
  if ( name == "warrior_of_elune"      ) return new      warrior_of_elune_t( this, options_str );
  if ( name == "wild_mushroom"         ) return new         wild_mushroom_t( this, options_str );

  // Feral
  if ( name == "berserk_cat"           ) return new           berserk_cat_t( this, options_str );
  if ( name == "brutal_slash"          ) return new          brutal_slash_t( this, options_str );
  if ( name == "feral_frenzy"          ) return new          feral_frenzy_t( this, options_str );
  if ( name == "moonfire_cat" ||
       name == "lunar_inspiration"     ) return new     lunar_inspiration_t( this, options_str );
  if ( name == "primal_wrath"          ) return new          primal_wrath_t( this, options_str );
  if ( name == "tigers_fury"           ) return new           tigers_fury_t( this, options_str );

  // Guardian
  if ( name == "berserk_bear"          ) return new          berserk_bear_t( this, options_str );
  if ( name == "bristling_fur"         ) return new         bristling_fur_t( this, options_str );
  if ( name == "maul"                  ) return new                  maul_t( this, options_str );
  if ( name == "pulverize"             ) return new             pulverize_t( this, options_str );
  if ( name == "rage_of_the_sleeper"   ) return new   rage_of_the_sleeper_t( this, options_str );

  // Restoration
  if ( name == "cenarion_ward"         ) return new         cenarion_ward_t( this, options_str );
  if ( name == "efflorescence"         ) return new         efflorescence_t( this, options_str );
  if ( name == "flourish"              ) return new              flourish_t( this, options_str );
  if ( name == "lifebloom"             ) return new             lifebloom_t( this, options_str );
  if ( name == "natures_cure"          ) return new          natures_cure_t( this, options_str );
  if ( name == "nourish"               ) return new               nourish_t( this, options_str );
  if ( name == "overgrowth"            ) return new            overgrowth_t( this, options_str );
  if ( name == "swiftmend"             ) return new             swiftmend_t( this, options_str );
  if ( name == "tranquility"           ) return new           tranquility_t( this, options_str );

  // Covenant
  if ( name == "kindred_spirits" ||
       name == "empower_bond"          ) return new       kindred_spirits_t( this, options_str );
  if ( name == "ravenous_frenzy"       ) return new       ravenous_frenzy_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================
pet_t* druid_t::create_pet( std::string_view pet_name, std::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  using namespace pets;

  return nullptr;
}

// druid_t::create_pets =====================================================
void druid_t::create_pets()
{
  player_t::create_pets();

  if ( talent.denizen_of_the_dream.ok() )
  {
    pets.denizen_of_the_dream.set_creation_callback( []( druid_t* p ) {
      return new pets::denizen_of_the_dream_t( p );
    } );
  }

  if ( talent.force_of_nature.ok() )
  {
    for ( pet_t*& pet : pets.force_of_nature )
      pet = new pets::force_of_nature_t( sim, this );
  }
}

// druid_t::init_spells =====================================================
void druid_t::init_spells()
{
  auto check = [ this ]( bool b, auto arg ) {
    if ( b )
    {
      if constexpr ( std::is_same_v<decltype( arg ), int> )
        return find_spell( arg );
      else if constexpr ( std::is_same_v<decltype( arg ), const spell_data_t*> )
        return arg;
    }
    return spell_data_t::not_found();
  };

  player_t::init_spells();

  // Talents ================================================================

  auto CT  = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };
  auto ST  = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };
  auto STS = [ this ]( std::string_view n, specialization_e s ) {
    return find_talent_spell( talent_tree::SPECIALIZATION, n, s );
  };

  // Class tree
  sim->print_debug( "Initializing class talents..." );
  talent.astral_influence               = CT( "Astral Influence" );
  talent.cyclone                        = CT( "Cyclone" );
  talent.feline_swiftness               = CT( "Feline Swiftness" );
  talent.frenzied_regeneration          = CT( "Frenzied Regeneration" );
  talent.heart_of_the_wild              = CT( "Heart of the Wild" );
  talent.hibernate                      = CT( "Hibernate" );
  talent.improved_barkskin              = CT( "Improved Barkskin" );
  talent.improved_rejuvenation          = CT( "Improved Rejuvenation" );
  talent.improved_stampeding_roar       = CT( "Improved Stampeding Roar");
  talent.improved_sunfire               = CT( "Improved Sunfire" );
  talent.incapacitating_roar            = CT( "Incapacitating Roar" );
  talent.innervate                      = CT( "Innervate" );
  talent.ironfur                        = CT( "Ironfur" );
  talent.killer_instinct                = CT( "Killer Instinct" );
  talent.lycaras_teachings              = CT( "Lycara's Teachings" );
  talent.maim                           = CT( "Maim" );
  talent.mass_entanglement              = CT( "Mass Entanglement" );
  talent.matted_fur                     = CT( "Matted Fur" );
  talent.mighty_bash                    = CT( "Mighty Bash" );
  talent.moonkin_form                   = CT( "Moonkin Form" );
  talent.natural_recovery               = CT( "Natural Recovery" );
  talent.natures_vigil                  = CT( "Nature's Vigil" );
  talent.nurturing_instinct             = CT( "Nurturing Instinct" );
  talent.primal_fury                    = CT( "Primal Fury" );
  talent.protector_of_the_pack          = CT( "Protector of the Pack" );
  talent.rake                           = CT( "Rake" );
  talent.rejuvenation                   = CT( "Rejuvenation" );
  talent.remove_corruption              = CT( "Remove Corruption" );
  talent.renewal                        = CT( "Renewal" );
  talent.rip                            = CT( "Rip" );
  talent.skull_bash                     = CT( "Skull Bash" );
  talent.soothe                         = CT( "Soothe" );
  talent.stampeding_roar                = CT( "Stampeding Roar" );
  talent.starfire                       = CT( "Starfire" );
  talent.starsurge                      = CT( "Starsurge" );
  talent.sunfire                        = CT( "Sunfire" );
  talent.swiftmend                      = CT( "Swiftmend" );
  talent.swipe                          = CT( "Swipe" );
  talent.thick_hide                     = CT( "Thick Hide" );
  talent.thrash                         = CT( "Thrash" );
  talent.tireless_pursuit               = CT( "Tireless Pursuit" );
  talent.typhoon                        = CT( "Typhoon" );
  talent.ursine_vigor                   = CT( "Ursine Vigor" );
  talent.ursols_vortex                  = CT( "Ursol's Vortex" );
  talent.verdant_heart                  = CT( "Verdant Heart" );
  talent.wellhoned_instincts            = CT( "Well-Honed Instincts" );
  talent.wild_charge                    = CT( "Wild Charge" );
  talent.wild_growth                    = CT( "Wild Growth" );

  // Multi-Spec
  sim->print_debug( "Initializing multi-spec talents..." );
  talent.adaptive_swarm                 = ST( "Adaptive Swarm" );
  talent.circle_of_life_and_death       = ST( "Circle of Life and Death" );
  talent.convoke_the_spirits            = ST( "Convoke the Spirits" );
  talent.survival_instincts             = ST( "Survival Instincts" );
  talent.unbridled_swarm                = ST( "Unbridled Swarm" );

  // Balance
  sim->print_debug( "Initializing balance talents..." );
  talent.aetherial_kindling             = ST( "Aetherial Kindling" );
  talent.astral_communion               = ST( "Astral Communion" );
  talent.astral_smolder                 = ST( "Astral Smolder" );
  talent.balance_of_all_things          = ST( "Balance of All Things" );
  talent.celestial_alignment            = ST( "Celestial Alignment" );
  talent.denizen_of_the_dream           = ST( "Denizen of the Dream" );
  talent.eclipse                        = ST( "Eclipse" );
  talent.elunes_guidance                = ST( "Elune's Guidance" );
  talent.force_of_nature                = ST( "Force of Nature" );
  talent.friend_of_the_fae              = ST( "Friend of the Fae" );
  talent.fungal_growth                  = ST( "Fungal Growth" );
  talent.fury_of_elune                  = ST( "Fury of Elune" );
  talent.incarnation_moonkin            = ST( "Incarnation: Chosen of Elune" );
  talent.light_of_the_sun               = ST( "Light of the Sun" );
  talent.lunar_shrapnel                 = ST( "Lunar Shrapnel" );
  talent.natures_balance                = ST( "Nature's Balance" );
  talent.natures_grace                  = ST( "Nature's Grace" );
  talent.new_moon                       = ST( "New Moon" );
  talent.orbit_breaker                  = ST( "Orbit Breaker" );
  talent.power_of_goldrinn              = ST( "Power of Goldrinn" );
  talent.primordial_arcanic_pulsar      = ST( "Primordial Arcanic Pulsar" );
  talent.radiant_moonlight              = ST( "Radiant Moonlight" );
  talent.rattle_the_stars               = ST( "Rattle the Stars" );
  talent.shooting_stars                 = ST( "Shooting Stars" );
  talent.solar_beam                     = ST( "Solar Beam" );
  talent.solstice                       = ST( "Solstice" );
  talent.soul_of_the_forest_moonkin     = STS( "Soul of the Forest", DRUID_BALANCE );
  talent.starfall                       = ST( "Starfall" );
  talent.starlord                       = ST( "Starlord" );
  talent.starweaver                     = ST( "Starweaver" );
  talent.stellar_flare                  = ST( "Stellar Flare" );
  talent.stellar_innervation            = ST( "Stellar Innervation" );
  talent.sundered_firmament             = ST( "Sundered Firmament" );
  talent.syzygy                         = ST( "Syzygy" );
  talent.twin_moons                     = ST( "Twin Moons" );
  talent.umbral_embrace                 = ST( "Umbral Embrace" );
  talent.umbral_intensity               = ST( "Umbral Intensity" );
  talent.waning_twilight                = ST( "Waning Twilight" );
  talent.warrior_of_elune               = ST( "Warrior of Elune" );
  talent.wild_mushroom                  = ST( "Wild Mushroom" );

  // Feral
  sim->print_debug( "Initializing feral talents..." );
  talent.apex_predators_craving         = ST( "Apex Predator's Craving" );
  talent.ashamanes_guidance             = ST( "Ashamane's Guidance" );
  talent.berserk                        = ST( "Berserk" );
  talent.berserk_frenzy                 = ST( "Berserk: Frenzy" );
  talent.berserk_heart_of_the_lion      = ST( "Berserk: Heart of the Lion" );
  talent.bloodtalons                    = ST( "Bloodtalons" );
  talent.brutal_slash                   = ST( "Brutal Slash" );
  talent.carnivorous_instinct           = ST( "Carnivorous Instinct" );
  talent.cats_curiosity                 = ST( "Cat's Curiosity" );
  talent.doubleclawed_rake              = ST( "Double-Clawed Rake" );
  talent.dreadful_bleeding              = ST( "Dreadful Bleeding" );
  talent.feral_frenzy                   = ST( "Feral Frenzy" );
  talent.frantic_momentum               = ST( "Frantic Momentum" );
  talent.incarnation_cat                = ST( "Incarnation: Avatar of Ashamane" );
  talent.infected_wounds_cat            = STS( "Infected Wounds", DRUID_FERAL );
  talent.lions_strength                 = ST( "Lion's Strength" );
  talent.lunar_inspiration              = ST( "Lunar Inspiration" );
  talent.merciless_strikes              = ST( "Merciless STrikes" );
  talent.moment_of_clarity              = ST( "Moment of Clarity" );
  talent.omen_of_clarity_cat            = STS( "Omen of Clarity", DRUID_FERAL );
  talent.pouncing_strikes               = ST( "Pouncing Strikes" );
  talent.predatory_swiftness            = ST( "Predatory Swiftness" );
  talent.primal_claws                   = ST( "Primal Claws" );
  talent.primal_wrath                   = ST( "Primal Wrath" );
  talent.predator                       = ST( "Predator" );
  talent.protective_growth              = ST( "Protective Growth" );
  talent.relentless_predator            = ST( "Relentless Predator" );
  talent.raging_fury                    = ST( "Raging Fury" );
  talent.rampant_ferocity               = ST( "Rampant Ferocity" );
  talent.rip_and_tear                   = ST( "Rip and Tear" );
  talent.sabertooth                     = ST( "Sabertooth" );
  talent.soul_of_the_forest_cat         = STS( "Soul of the Forest", DRUID_FERAL );
  talent.sudden_ambush                  = ST( "Sudden Ambush" );
  talent.taste_for_blood                = ST( "Taste for Blood" );
  talent.tear_open_wounds               = ST( "Tear Open Wounds" );
  talent.tigers_fury                    = ST( "Tiger's Fury" );
  talent.tigers_tenacity                = ST( "Tiger's Tenacity" );
  talent.tireless_energy                = ST( "Tireless Energy" );
  talent.veinripper                     = ST( "Veinripper" );
  talent.wild_slashes                   = ST( "Wild Slashes" );

  // Guardian
  sim->print_debug( "Initializing guardian talents..." );
  talent.after_the_wildfire             = ST( "After the Wildfire" );
  talent.berserk_persistence            = ST( "Berserk: Persistence" );
  talent.berserk_ravage                 = ST( "Berserk: Ravage" );
  talent.berserk_unchecked_aggression   = ST( "Berserk: Unchecked Aggression" );
  talent.blood_frenzy                   = ST( "Blood Frenzy" );
  talent.brambles                       = ST( "Brambles" );
  talent.bristling_fur                  = ST( "Bristling Fur" );
  talent.dream_of_cenarius              = ST( "Dream of Cenarius" );
  talent.earthwarden                    = ST( "Earthwarden" );
  talent.elunes_favored                 = ST( "Elune's Favored" );
  talent.flashing_claws                 = ST( "Flashing Claws" );
  talent.front_of_the_pack              = ST( "Front of the Pack" );
  talent.fury_of_nature                 = ST( "Fury of Nature" );
  talent.galactic_guardian              = ST( "Galactic Guardian" );
  talent.gore                           = ST( "Gore" );
  talent.gory_fur                       = ST( "Gory Fur" );
  talent.guardian_of_elune              = ST( "Guardian of Elune" );
  talent.improved_survival_instincts    = ST( "Improved Survival Instincts" );
  talent.incarnation_bear               = ST( "Incarnation: Guardian of Ursoc" );
  talent.infected_wounds_bear           = STS( "Infected Wounds", DRUID_GUARDIAN );
  talent.innate_resolve                 = ST( "Innate Resolve" );
  talent.layered_mane                   = ST( "Layered Mane" );
  talent.mangle                         = ST( "Mangle" );
  talent.maul                           = ST( "Maul" );
  talent.pulverize                      = ST( "Pulverize" );
  talent.rage_of_the_sleeper            = ST( "Rage of the Sleeper" );
  talent.reinforced_fur                 = ST( "Reinforced Fur" );
  talent.reinvigoration                 = ST( "Reinvigoration" );
  talent.rend_and_tear                  = ST( "Rend and Tear" );
  talent.scintillating_moonlight        = ST( "Scintillating Moonlight" );
  talent.soul_of_the_forest_bear        = STS( "Soul of the Forest", DRUID_GUARDIAN );
  talent.survival_of_the_fittest        = ST( "Survival of the Fittest" );
  talent.tooth_and_claw                 = ST( "Tooth and Claw" );
  talent.twin_moonfire                  = ST( "Twin Moonfire" );
  talent.ursine_adept                   = ST( "Ursine Adept" );
  talent.ursocs_guidance                = ST( "Ursoc's Guidance" );
  talent.ursocs_endurance               = ST( "Ursoc's Endurance" );
  talent.vicious_cycle                  = ST( "Vicious Cycle" );
  talent.vulnerable_flesh               = ST( "Vulnerable Flesh" );
  talent.untamed_savagery               = ST( "Untamed Savagery" );
  talent.ursocs_fury                    = ST( "Ursoc's Fury" );

  // Restoration
  sim->print_debug( "Initializing restoration talents..." );
  talent.abundance                      = ST( "Abundance" );
  talent.budding_leaves                 = ST( "Budding Leaves" );  // TODO: NYI
  talent.cenarion_ward                  = ST( "Cenarion Ward" );
  talent.cenarius_guidance              = ST( "Cenarius' Guidance" );  // TODO: Incarn bonus NYI
  talent.cultivation                    = ST( "Cultivation" );
  talent.dreamstate                     = ST( "Dreamstate" );  // TODO: NYI
  talent.efflorescence                  = ST( "Efflorescence" );
  talent.embrace_of_the_dream           = ST( "Embrace of the Dream" );  // TODO: NYI
  talent.flash_of_clarity               = ST( "Flash of Clarity" );
  talent.flourish                       = ST( "Flourish" );
  talent.germination                    = ST( "Germination" );
  talent.grove_tending                  = ST( "Grove Tending" );
  talent.harmonious_blooming            = ST( "Harmonious Blooming" );
  talent.improved_ironbark              = ST( "Improved Ironbark" );
  talent.improved_regrowth              = ST( "Improved Regrowth" );
  talent.improved_wild_growth           = ST( "Improved Wild Growth" );
  talent.incarnation_tree               = ST( "Incarnation: Tree of Life" );
  talent.inner_peace                    = ST( "Inner Peace" );
  talent.invigorate                     = ST( "Invigorate" );  // TODO: NYI
  talent.ironbark                       = ST( "Ironbark" );
  talent.lifebloom                      = ST( "Lifebloom" );
  talent.luxuriant_soil                 = ST( "Luxuriant Soil" );  // TODO: NYI
  talent.natural_wisdom                 = ST( "Natural Wisdom" );  // TODO: NYI
  talent.natures_splendor               = ST( "Nature's Splendor" );
  talent.natures_swiftness              = ST( "Nature's Swiftness" );
  talent.nourish                        = ST( "Nourish" );
  talent.nurturing_dormancy             = ST( "Nurturing Dormancy" );  // TODO: NYI
  talent.omen_of_clarity_tree           = STS( "Omen of Clarity", DRUID_RESTORATION );
  talent.overgrowth                     = ST( "Overgrowth" );
  talent.passing_seasons                = ST( "Passing Seasons" );
  talent.photosynthesis                 = ST( "Photosynthesis" );
  talent.power_of_the_archdruid         = ST( "Power of the Archdruid" );  // TODO: NYI
  talent.rampant_growth                 = ST( "Rampant Growth" );  // TODO: copy on lb target NYI
  talent.reforestation                  = ST( "Reforestation" );  // TODO: NYI
  talent.regenesis                      = ST( "Regenesis" );  // TODO: NYI
  talent.regenerative_heartwood         = ST( "Regenerative Heartwood" );  // TODO: NYI
  talent.soul_of_the_forest_tree        = STS( "Soul of the Forest", DRUID_RESTORATION );
  talent.spring_blossoms                = ST( "Spring Blossoms" );
  talent.stonebark                      = ST( "Stonebark" );
  talent.tranquility                    = ST( "Tranquility" );
  talent.undergrowth                    = ST( "Undergrowth" );  // TODO: NYI
  talent.unstoppable_growth             = ST( "Unstoppable Growth" );
  talent.verdancy                       = ST( "Verdancy" );  // TODO: NYI
  talent.verdant_infusion               = ST( "Verdant Infusion" );  // TODO: NYI
  talent.waking_dream                   = ST( "Waking Dream" );  // TODO: increased healing per rejuv NYI
  talent.yseras_gift                    = ST( "Ysera's Gift" );

  // Covenants
  cov.kyrian                       = find_covenant_spell( "Kindred Spirits" );
  cov.empower_bond                 = check( cov.kyrian->ok(), 326446 );
  cov.kindred_empowerment          = check( cov.kyrian->ok(), 327022 );
  cov.kindred_empowerment_energize = check( cov.kyrian->ok(), 327139 );
  cov.kindred_empowerment_damage   = check( cov.kyrian->ok(), 338411 );
  cov.night_fae                    = find_covenant_spell( "Convoke the Spirits" );
  cov.venthyr                      = find_covenant_spell( "Ravenous Frenzy" );
  cov.necrolord                    = find_covenant_spell( "Adaptive Swarm" );

  // Conduits

  // Balance
  conduit.stellar_inspiration  = find_conduit_spell( "Stellar Inspiration" );
  conduit.precise_alignment    = find_conduit_spell( "Precise Alignment" );
  conduit.fury_of_the_skies    = find_conduit_spell( "Fury of the Skies" );
  conduit.umbral_intensity     = find_conduit_spell( "Umbral Intensity" );

  // Feral
  conduit.taste_for_blood      = find_conduit_spell( "Taste for Blood" );
  conduit.incessant_hunter     = find_conduit_spell( "Incessant Hunter" );
  conduit.sudden_ambush        = find_conduit_spell( "Sudden Ambush" );
  conduit.carnivorous_instinct = find_conduit_spell( "Carnivorous Instinct" );

  // Guardian
  conduit.unchecked_aggression = find_conduit_spell( "Unchecked Aggression" );
  conduit.savage_combatant     = find_conduit_spell( "Savage Combatant" );
  conduit.layered_mane         = find_conduit_spell( "Layered Mane" );

  // Covenant
  conduit.deep_allegiance      = find_conduit_spell( "Deep Allegiance" );
  conduit.evolved_swarm        = find_conduit_spell( "Evolved Swarm" );
  conduit.conflux_of_elements  = find_conduit_spell( "Conflux of Elements" );
  conduit.endless_thirst       = find_conduit_spell( "Endless Thirst" );

  // Endurance
  conduit.tough_as_bark        = find_conduit_spell( "Tough as Bark" );
  conduit.ursine_vigor         = find_conduit_spell( "Ursine Vigor" );
  conduit.innate_resolve       = find_conduit_spell( "Innate Resolve" );

  // Finesse
  conduit.front_of_the_pack    = find_conduit_spell( "Front of the Pack" );
  conduit.born_of_the_wilds    = find_conduit_spell( "Born of the Wilds" );

  // Runeforge Legendaries

  // General
  legendary.oath_of_the_elder_druid   = find_runeforge_legendary( "Oath of the Elder Druid" );
  legendary.circle_of_life_and_death  = find_runeforge_legendary( "Circle of Life and Death" );
  legendary.draught_of_deep_focus     = find_runeforge_legendary( "Draught of Deep Focus" );
  legendary.lycaras_fleeting_glimpse  = find_runeforge_legendary( "Lycara's Fleeting Glimpse" );

  // Covenant
  legendary.kindred_affinity          = find_runeforge_legendary( "Kindred Affinity" );
  legendary.unbridled_swarm           = find_runeforge_legendary( "Locust Swarm" );
  legendary.celestial_spirits         = find_runeforge_legendary( "Celestial Spirits" );
  legendary.sinful_hysteria           = find_runeforge_legendary( "Sinful Hysteria" );

  // Balance
  legendary.oneths_clear_vision       = find_runeforge_legendary( "Oneth's Clear Vision" );
  legendary.primordial_arcanic_pulsar = find_runeforge_legendary( "Primordial Arcanic Pulsar" );
  legendary.timeworn_dreambinder      = find_runeforge_legendary( "Timeworn Dreambinder" );
  legendary.balance_of_all_things     = find_runeforge_legendary( "Balance of All Things" );

  // Feral
  legendary.cateye_curio              = find_runeforge_legendary( "Cat-eye Curio" );
  legendary.eye_of_fearful_symmetry   = find_runeforge_legendary( "Eye of Fearful Symmetry" );
  legendary.apex_predators_craving    = find_runeforge_legendary( "Apex Predator's Craving" );
  legendary.frenzyband                = find_runeforge_legendary( "Frenzyband" );

  // Guardian
  legendary.luffainfused_embrace      = find_runeforge_legendary( "Luffa-Infused Embrace" );
  legendary.the_natural_orders_will   = find_runeforge_legendary( "The Natural Order's Will" );
  legendary.ursocs_fury_remembered    = find_runeforge_legendary( "Ursoc's Fury Remembered" );
  legendary.legacy_of_the_sleeper     = find_runeforge_legendary( "Legacy of the Sleeper" );

  // Restoration

  // Passive Auras
  spec.druid                    = find_spell( 137009 );
  spec.critical_strikes         = find_specialization_spell( "Critical Strikes" );
  spec.leather_specialization   = find_specialization_spell( "Leather Specialization" );

  // Baseline
  spec.bear_form_override       = find_spell( 106829 );
  spec.bear_form_passive        = find_spell( 1178 );
  spec.cat_form_override        = find_spell( 48629 );
  spec.cat_form_passive         = find_spell( 3025 );
  spec.cat_form_speed           = find_spell( 113636 );
  spec.feral_affinity           = find_specialization_spell( "Feral Affinity" );
  spec.improved_prowl           = check( talent.rake.ok(), 231052 );
  spec.moonfire_2               = find_rank_spell( "Moonfire", "Rank 2" );
  spec.moonfire_dmg             = find_spell( 164812 );
  spec.wrath                    = find_specialization_spell( "Wrath" );
  if ( !spec.wrath->ok() )
    spec.wrath                  = find_class_spell( "Wrath" );

  // Class Abilities
  spec.sunfire_dmg              = check( talent.sunfire.ok(), 164815 );
  spec.thrash_bear_dot          = check( talent.thrash.ok(), 192090 );

  // Multi-Spec Abilities
  spec.adaptive_swarm_damage    = check( talent.adaptive_swarm.ok(), 391889 );
  spec.adaptive_swarm_heal      = check( talent.adaptive_swarm.ok(), 391891 );

  // Balance Abilities
  spec.balance                  = find_specialization_spell( "Balance Druid" );
  spec.astral_power             = find_specialization_spell( "Astral Power" );
  spec.celestial_alignment      = talent.celestial_alignment.find_override_spell();
  spec.eclipse_lunar            = check( talent.eclipse.ok(), 48518 );
  spec.eclipse_solar            = check( talent.eclipse.ok(), 48517 );
  spec.full_moon                = check( talent.new_moon.ok(), 274283 );
  spec.half_moon                = check( talent.new_moon.ok(), 274282 );
  spec.incarnation_moonkin      = check( talent.incarnation_moonkin.ok(), 102560 );
  spec.shooting_stars_dmg       = check( talent.shooting_stars.ok(), 202497 );  // shooting stars damage

  // Feral Abilities
  spec.feral                    = find_specialization_spell( "Feral Druid" );
  spec.feral_overrides          = find_specialization_spell( "Feral Overrides Passive" );
  spec.ashamanes_guidance       = check( talent.ashamanes_guidance.ok(), talent.convoke_the_spirits.ok() ? 391538 : 391475 );
  spec.berserk_cat              = talent.berserk.find_override_spell();

  // Guardian Abilities
  spec.guardian                 = find_specialization_spell( "Guardian Druid" );
  spec.bear_form_2              = find_rank_spell( "Bear Form", "Rank 2" );
  spec.berserk_bear             = check( talent.berserk_ravage.ok() ||
                                         talent.berserk_unchecked_aggression.ok() ||
                                         talent.berserk_persistence.ok(), 50334 );
  spec.incarnation_bear         = check( talent.incarnation_bear.ok(), 102558 );
  spec.lightning_reflexes       = find_specialization_spell( "Lightning Reflexes" );

  // Restoration Abilities
  spec.restoration              = find_specialization_spell( "Restoration Druid" );

  // Masteries ==============================================================
  mastery.harmony             = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian    = find_mastery_spell( DRUID_GUARDIAN );
  mastery.natures_guardian_AP = check( mastery.natures_guardian->ok(), 159195 );
  mastery.razor_claws         = find_mastery_spell( DRUID_FERAL );
  mastery.astral_invocation   = find_mastery_spell( DRUID_BALANCE );

  eclipse_handler.init();  // initialize this here since we need talent info to properly init
}

// druid_t::init_base =======================================================
void druid_t::init_base_stats()
{
  // Set base distance based on spec
  if ( base.distance < 1 )
    base.distance = specialization() == DRUID_BALANCE ? 30 : 5;

  player_t::init_base_stats();

  base.attack_power_per_agility = specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ? 1.0 : 0.0;
  base.spell_power_per_intellect = specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION ? 1.0 : 0.0;

  // Resources
  resources.base[ RESOURCE_RAGE ]         = 100;
  resources.base[ RESOURCE_COMBO_POINT ]  = 5;
  resources.base[ RESOURCE_ASTRAL_POWER ] = 100;
  resources.base[ RESOURCE_ENERGY ]       = 100 + talent.tireless_energy->effectN( 1 ).base_value();

  // only activate other resources if you have the affinity and affinity_resources = true
  resources.active_resource[ RESOURCE_HEALTH ]       = specialization() == DRUID_GUARDIAN;
  resources.active_resource[ RESOURCE_RAGE ]         = specialization() == DRUID_GUARDIAN || options.affinity_resources;
  resources.active_resource[ RESOURCE_MANA ]         = specialization() == DRUID_RESTORATION
                                                    || ( specialization() == DRUID_GUARDIAN && options.owlweave_bear )
                                                    || ( specialization() == DRUID_FERAL && options.owlweave_cat )
                                                    || options.affinity_resources;
  resources.active_resource[ RESOURCE_COMBO_POINT ]  = specialization() == DRUID_FERAL || specialization() == DRUID_RESTORATION
                                                    || ( specialization() == DRUID_GUARDIAN && options.catweave_bear )
                                                    || options.affinity_resources;
  resources.active_resource[ RESOURCE_ENERGY ]       = specialization() == DRUID_FERAL
                                                    || ( specialization() == DRUID_RESTORATION && role == ROLE_ATTACK )
                                                    || ( specialization() == DRUID_GUARDIAN && options.catweave_bear )
                                                    || options.affinity_resources;
  resources.active_resource[ RESOURCE_ASTRAL_POWER ] = specialization() == DRUID_BALANCE;

  // Energy Regen
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *=
      1.0 + query_aura_effect( spec.feral_affinity, A_MOD_POWER_REGEN_PERCENT, POWER_ENERGY )->percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *=
      1.0 + query_aura_effect( spec.feral, A_MOD_POWER_REGEN_PERCENT, POWER_ENERGY )->percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + talent.tireless_energy->effectN( 2 ).percent();

  base_gcd = 1.5_s;
}

void druid_t::init_assessors()
{
  player_t::init_assessors();

  if ( cov.kyrian->ok() && active.kindred_empowerment )
  {
    if ( options.kindred_spirits_target )
    {
      auto partner = options.kindred_spirits_target;

      buff.kindred_affinity->set_stack_change_callback( [ partner ]( buff_t*, int old_, int new_ ) {
        if ( new_ > old_ )
          partner->buffs.kindred_affinity->increment();
        else
          partner->buffs.kindred_affinity->decrement();
      } );
    }

    using emp_buff_t = buffs::kindred_empowerment_buff_t;

    auto handler = []( druid_t* p, emp_buff_t* add, emp_buff_t* use, action_state_t* s ) {
      if ( !s->action->harmful || ( s->action->type != ACTION_SPELL && s->action->type != ACTION_ATTACK ) )
        return;

      if ( s->result_amount == 0 )
        return;

      if ( s->action->id == p->active.kindred_empowerment->id ||
           s->action->id == p->active.kindred_empowerment_partner->id )
        return;

      if ( add && p->buff.kindred_empowerment_energize->up() )
        add->add_pool( s );

      if ( use && use->up() )
        use->use_pool( s );
    };

    // if we don't have an explicit target, emulate bonding with an identical damage profile 
    auto partner = options.kindred_spirits_target ? options.kindred_spirits_target : this;
    auto self_buff = debug_cast<emp_buff_t*>( buff.kindred_empowerment );
    auto bond_buff = debug_cast<emp_buff_t*>( buff.kindred_empowerment_partner );

    // only dps specs give a pool to the partner
    if ( specialization() != DRUID_BALANCE && specialization() != DRUID_FERAL )
      bond_buff = nullptr;

    // druid's assessor: add to bond buff, use from self buff
    assessor_out_damage.add( assessor::TARGET_DAMAGE + 1,
      [ &handler, self_buff, bond_buff, this ]( result_amount_type, action_state_t* s ) {
        handler( this, bond_buff, self_buff, s );
        return assessor::CONTINUE;
      } );

    // partner's assessor: add to self buff, use from bond buff
    partner->assessor_out_damage.add( assessor::TARGET_DAMAGE + 1,
      [ &handler, self_buff, bond_buff, this ]( result_amount_type, action_state_t* s ) {
        handler( this, self_buff, bond_buff, s );
        return assessor::CONTINUE;
      } );
  }
}

void druid_t::init_finished()
{
  player_t::init_finished();

  if ( specialization() == DRUID_BALANCE )
    spec_override.attack_power = query_aura_effect( spec.balance, A_404 )->percent();
  else if ( specialization() == DRUID_FERAL )
    spec_override.spell_power = query_aura_effect( spec.feral, A_366 )->percent();
  else if ( specialization() == DRUID_GUARDIAN )
    spec_override.spell_power = query_aura_effect( spec.guardian, A_366 )->percent();
  else if ( specialization() == DRUID_RESTORATION )
    spec_override.attack_power = query_aura_effect( spec.restoration, A_404 )->percent();

  // PRECOMBAT WRATH SHENANIGANS
  // we do this here so all precombat actions have gone throught init() and init_finished() so if-expr are properly
  // parsed and we can adjust wrath travel times accordingly based on subsequent precombat actions that will sucessfully
  // cast
  for ( auto pre = precombat_action_list.begin(); pre != precombat_action_list.end(); pre++ )
  {
    auto wr = dynamic_cast<spells::wrath_t*>( *pre );

    if ( wr )
    {
      std::for_each( pre + 1, precombat_action_list.end(), [ wr ]( action_t* a ) {
        // unnecessary offspec resources are disabled by default, so evaluate any if-expr on the candidate action first
        // so we don't call action_ready() on possible offspec actions that will require off-spec resources to be
        // enabled
        if ( a->harmful && ( !a->if_expr || a->if_expr->success() ) && a->action_ready() )
          wr->harmful = false;  // more harmful actions exist, set current wrath to non-harmful so we can keep casting

        if ( a->name_str == wr->name_str )
          wr->count++;  // see how many wrath casts are left, so we can adjust travel time when combat begins
      } );

      // if wrath is still harmful then it is the final precast spell, so we set the energize type to NONE, which will
      // then be accounted for in wrath_t::execute()
      if ( wr->harmful )
        wr->energize_type = action_energize::NONE;
    }
  }
}

// druid_t::init_buffs ======================================================
void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Baseline
  buff.barkskin = make_buff( this, "barkskin", find_class_spell( "Barkskin" ) )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_tick_behavior( buff_tick_behavior::NONE )
    ->apply_affecting_aura( talent.improved_barkskin )
    ->apply_affecting_aura( talent.reinforced_fur )
    ->apply_affecting_aura( talent.ursocs_endurance );
  if ( talent.brambles.ok() )
    buff.barkskin->set_tick_behavior( buff_tick_behavior::REFRESH );
  if ( legendary.the_natural_orders_will->ok() )
  {
    buff.barkskin->apply_affecting_aura( legendary.the_natural_orders_will )
      ->set_stack_change_callback( [ this ]( buff_t*, int, int ) { active.the_natural_orders_will->execute(); } );
  }

  buff.bear_form = make_buff<bear_form_buff_t>( *this );

  buff.cat_form = make_buff<cat_form_buff_t>( *this );

  buff.dash = make_buff( this, "dash", find_class_spell( "Dash" ) )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );

  buff.thorns = make_buff( this, "thorns", find_spell( 305497 ) );

  buff.wild_charge_movement = make_buff( this, "wild_charge_movement" );

  // Generic legendaries & Conduits
  buff.oath_of_the_elder_druid = make_buff( this, "oath_of_the_elder_druid", find_spell( 338643 ) )
    ->set_quiet( true );

  buff.lycaras_fleeting_glimpse = make_buff( this, "lycaras_fleeting_glimpse", find_spell( 340060 ) )
    ->set_period( 0_ms )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( !new_ && lycaras_event_remains == 0_ms )
        active.lycaras_fleeting_glimpse->execute();
    } );
  // default value used as interval for event
  buff.lycaras_fleeting_glimpse->set_default_value( legendary.lycaras_fleeting_glimpse->effectN( 1 ).base_value() -
                                                    buff.lycaras_fleeting_glimpse->buff_duration().total_seconds() );

  // Class
  buff.heart_of_the_wild = make_buff( this, "heart_of_the_wild", talent.heart_of_the_wild )
    ->set_cooldown( 0_ms );

  buff.ironfur = make_buff( this, "ironfur", talent.ironfur )
    ->set_default_value_from_effect_type( A_MOD_ARMOR_BY_PRIMARY_STAT_PCT )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->apply_affecting_aura( talent.reinforced_fur )
    ->apply_affecting_aura( talent.ursine_adept )
    ->apply_affecting_aura( talent.ursocs_endurance )
    ->add_invalidate( CACHE_AGILITY )
    ->add_invalidate( CACHE_ARMOR );

  buff.innervate = make_buff( this, "innervate", talent.innervate );

  buff.lycaras_teachings_haste = make_buff( this, "lycaras_teachings_haste", find_spell( 378989 ) )
    ->set_default_value( talent.lycaras_teachings->effectN( 1 ).percent() )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->set_name_reporting( "Haste" );

  buff.lycaras_teachings_crit = make_buff( this, "lycaras_teachings_crit", find_spell( 378990 ) )
    ->set_default_value( talent.lycaras_teachings->effectN( 1 ).percent() )
    ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
    ->set_name_reporting( "Crit" );

  buff.lycaras_teachings_vers = make_buff( this, "lycaras_teachings_vers", find_spell( 378991 ) )
    ->set_default_value( talent.lycaras_teachings->effectN( 1 ).percent() )
    ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY )
    ->set_name_reporting( "Vers" );

  buff.lycaras_teachings_mast = make_buff( this, "lycaras_teachings_mast", find_spell( 378992 ) )
    ->set_default_value( talent.lycaras_teachings->effectN( 1 ).percent() )
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
    ->set_name_reporting( "Mastery" );

  buff.lycaras_teachings = make_buff( this, "lycaras_teachings", talent.lycaras_teachings )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_period( 5.25_s )
    ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      buff_t* new_buff;

      switch( get_form() )
      {
        case NO_FORM:      new_buff = buff.lycaras_teachings_haste; break;
        case CAT_FORM:     new_buff = buff.lycaras_teachings_crit;  break;
        case BEAR_FORM:    new_buff = buff.lycaras_teachings_vers;  break;
        case MOONKIN_FORM: new_buff = buff.lycaras_teachings_mast;  break;
        default: return;
      }

      if ( !new_buff->check() )
      {
        buff.lycaras_teachings_haste->expire();
        buff.lycaras_teachings_crit->expire();
        buff.lycaras_teachings_vers->expire();
        buff.lycaras_teachings_mast->expire();

        new_buff->trigger();
      }
    } );

  buff.matted_fur =
      make_buff<absorb_buff_t>( this, "matted_fur", talent.matted_fur->effectN( 1 ).trigger() )
          ->set_default_value( talent.matted_fur->effectN( 1 ).average( this ) );

  buff.moonkin_form = make_buff<moonkin_form_buff_t>( *this );

  buff.natures_vigil = make_buff( this, "natures_vigil", talent.natures_vigil )
    ->set_cooldown( 0_ms )
    ->set_freeze_stacks( true );

  buff.protector_of_the_pack_moonfire =
      make_buff<protector_of_the_pack_buff_t>( *this, "protector_of_the_pack_moonfire", find_spell( 378987 ) );

  buff.protector_of_the_pack_regrowth =
      make_buff<protector_of_the_pack_buff_t>( *this, "protector_of_the_pack_regrowth", find_spell( 395336 ) );

  buff.tiger_dash = make_buff<tiger_dash_buff_t>( *this );

  buff.tireless_pursuit =
      make_buff( this, "tireless_pursuit", find_spell( 340546 ) )
          ->set_default_value( spec.cat_form_speed->effectN( 1 ).percent() )  // only switching from cat form supported
          ->set_duration( talent.tireless_pursuit->effectN( 1 ).time_value() );

  buff.ursine_vigor = make_buff<ursine_vigor_buff_t>( *this );

  // Multi-spec
  // The buff ID in-game is same as the talent, 61336, but the buff effects (as well as tooltip reference) is in 50322
  buff.survival_instincts = make_buff( this, "survival_instincts", talent.survival_instincts )
    ->set_cooldown( 0_ms )
    ->set_default_value( query_aura_effect( find_spell( 50322 ), A_MOD_DAMAGE_PERCENT_TAKEN )->percent() );

  // Balance buffs
  buff.balance_of_all_things_arcane = make_buff( this, "balance_of_all_things_arcane", find_spell( 394050 ) )
    ->set_reverse( true )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_name_reporting( "Arcane" );

  buff.balance_of_all_things_nature = make_buff( this, "balance_of_all_things_nature", find_spell( 394049 ) )
    ->set_reverse( true )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_name_reporting( "Nature" );

  buff.celestial_alignment =
      make_buff<celestial_alignment_buff_t>( *this, "celestial_alignment", spec.celestial_alignment );

  buff.incarnation_moonkin =
      make_buff<celestial_alignment_buff_t>( *this, "incarnation_chosen_of_elune", spec.incarnation_moonkin, true );

  buff.eclipse_lunar = make_buff<eclipse_buff_t>( *this, "eclipse_lunar", spec.eclipse_lunar );

  buff.eclipse_solar = make_buff<eclipse_buff_t>( *this, "eclipse_solar", spec.eclipse_solar );

  buff.friend_of_the_fae = make_buff( this, "friend_of_the_fae", find_spell( 394083 ) )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
    ->set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
      if ( !old_ )
        uptime.friend_of_the_fae->update( true, sim->current_time() );
      else if ( !new_ )
        uptime.friend_of_the_fae->update( false, sim->current_time() );
    } );

  buff.fury_of_elune = make_buff<fury_of_elune_buff_t>( *this, "fury_of_elune", talent.fury_of_elune );

  buff.gathering_starstuff =
      make_buff( this, "gathering_starstuff", sets->set( DRUID_BALANCE, T29, B2 )->effectN( 1 ).trigger() );

  buff.sundered_firmament = make_buff<fury_of_elune_buff_t>( *this, "sundered_firmament", find_spell( 394108 ) )
    ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  buff.natures_balance = make_buff( this, "natures_balance", talent.natures_balance );
  auto nb_eff = query_aura_effect( &buff.natures_balance->data(), A_PERIODIC_ENERGIZE, RESOURCE_ASTRAL_POWER );
  auto nb_ap = nb_eff->resource( RESOURCE_ASTRAL_POWER );
  buff.natures_balance->set_quiet( true )
    ->set_default_value( nb_ap / nb_eff->period().total_seconds() )
    ->set_tick_callback( [ nb_ap, this ]( buff_t*, int, timespan_t ) {
      resource_gain( RESOURCE_ASTRAL_POWER, nb_ap, gain.natures_balance );
    } );

  buff.natures_grace = make_buff( this, "natures_grace", find_spell( 393959 ) )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.orbit_breaker = make_buff( this, "orbit_breaker" )->set_quiet( true );
  if ( talent.orbit_breaker.ok() )
  {
    buff.orbit_breaker->set_max_stack( as<int>( talent.orbit_breaker->effectN( 1 ).base_value() ) )
      ->set_expire_at_max_stack( true )
      ->set_stack_change_callback( [ this ]( buff_t* b, int, int ) {
        if ( b->at_max_stacks() )
          active.orbit_breaker->execute_on_target( active.shooting_stars->target );
      } );
  }

  buff.owlkin_frenzy = make_buff( this, "owlkin_frenzy", find_spell( 157228 ) )
    ->set_chance( find_specialization_spell( "Owlkin Frenzy" )->effectN( 1 ).percent() );

  buff.parting_skies = make_buff( this, "parting_skies", find_spell( 395110 ) );

  buff.primordial_arcanic_pulsar = make_buff( this, "primordial_arcanic_pulsar", find_spell( 393961 ) ) 
    ->set_default_value( options.initial_pulsar_value );

  buff.rattled_stars = make_buff( this, "rattled_stars", talent.rattle_the_stars->effectN( 1 ).trigger() )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  buff.solstice = make_buff( this, "solstice", talent.solstice->effectN( 1 ).trigger() )
    ->set_default_value( talent.solstice->effectN( 1 ).percent() );

  buff.starfall = make_buff( this, "starfall", find_spell( 191034 ) )  // lookup via spellid for convoke
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_tick_on_application( true )  // TODO: confirm true?
    ->set_freeze_stacks( true )
    ->set_partial_tick( true )  // TODO: confirm true?
    ->set_tick_behavior( buff_tick_behavior::REFRESH );  // TODO: confirm true?

  buff.starlord = make_buff( this, "starlord", find_spell( 279709 ) )
    ->set_default_value( talent.starlord->effectN( 1 ).percent() )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.starweavers_warp = make_buff( this, "starweavers_warp", find_spell( 393942 ) )
    ->set_chance( talent.starweaver->effectN( 1 ).percent() );

  buff.starweavers_weft = make_buff( this, "starweavers_weft", find_spell( 393944 ) )
    ->set_chance( talent.starweaver->effectN( 2 ).percent() );

  buff.touch_the_cosmos =
      make_buff( this, "touch_the_cosmos", sets->set( DRUID_BALANCE, T29, B4 )->effectN( 1 ).trigger() );

  buff.touch_the_cosmos_starfall = make_buff( this, "touch_the_cosmos_starfall" )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_duration( buff.starfall->buff_duration() )
    ->set_max_stack( buff.starfall->max_stack() )
    ->set_quiet( true );

  buff.umbral_embrace = make_buff<umbral_embrace_buff_t>( *this );

  buff.warrior_of_elune = make_buff( this, "warrior_of_elune", talent.warrior_of_elune )
    ->set_cooldown( 0_ms )
    ->set_reverse( true )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( !new_ )
        cooldown.warrior_of_elune->start( cooldown.warrior_of_elune->action );
    } );

  // Feral buffs
  buff.apex_predators_craving =
      make_buff( this, "apex_predators_craving", talent.apex_predators_craving->effectN( 1 ).trigger() )
          ->set_chance( talent.apex_predators_craving->effectN( 1 ).percent() );

  buff.berserk_cat =
      make_buff<berserk_cat_buff_t>( *this, "berserk_cat", spec.berserk_cat );

  buff.incarnation_cat =
      make_buff<berserk_cat_buff_t>( *this, "incarnation_king_of_the_jungle", talent.incarnation_cat, true );

  buff.bloodtalons     = make_buff( this, "bloodtalons", find_spell( 145152 ) );
  buff.bt_rake         = make_buff<bt_dummy_buff_t>( *this, "bt_rake" );
  buff.bt_shred        = make_buff<bt_dummy_buff_t>( *this, "bt_shred" );
  buff.bt_swipe        = make_buff<bt_dummy_buff_t>( *this, "bt_swipe" );
  buff.bt_thrash       = make_buff<bt_dummy_buff_t>( *this, "bt_thrash" );
  buff.bt_moonfire     = make_buff<bt_dummy_buff_t>( *this, "bt_moonfire" );
  buff.bt_brutal_slash = make_buff<bt_dummy_buff_t>( *this, "bt_brutal_slash" );

  // 1.05s ICD per https://github.com/simulationcraft/simc/commit/b06d0685895adecc94e294f4e3fcdd57ac909a10
  buff.clearcasting_cat = make_buff( this, "clearcasting_cat", talent.omen_of_clarity_cat->effectN( 1 ).trigger() )
    ->set_cooldown( 1.05_s )
    ->apply_affecting_aura( talent.moment_of_clarity );
  buff.clearcasting_cat->name_str_reporting = "clearcasting";

  buff.tigers_tenacity = make_buff( this, "tigers_tenacity", talent.tigers_tenacity->effectN( 2 ).trigger() )
    ->set_reverse( true );
  buff.tigers_tenacity->set_default_value(
      buff.tigers_tenacity->data().effectN( 1 ).trigger()->effectN( 1 ).base_value() );

  buff.frantic_momentum = make_buff( this, "frantic_momentum", talent.frantic_momentum->effectN( 1 ).trigger() )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.incarnation_cat_prowl =
      make_buff( this, "incarnation_avatar_of_ashamane_prowl", talent.incarnation_cat->effectN( 2 ).trigger() );

  buff.predatory_swiftness = make_buff( this, "predatory_swiftness", find_spell( 69369 ) );

  buff.protective_growth = make_buff( this, "protective_growth", find_spell( 391955 ) )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN );

  buff.prowl = make_buff( this, "prowl", find_class_spell( "Prowl" ) );

  buff.sabertooth = make_buff( this, "sabertooth", find_spell( 391722 ) )
    ->set_default_value( talent.sabertooth->effectN( 2 ).percent() )
    ->set_max_stack( as<int>( resources.base[ RESOURCE_COMBO_POINT ] ) );

  buff.sharpened_claws_bloodied_fangs = make_buff( this, "sharpened_claws_bloodied_fangs", find_spell( 394465 ) );

  buff.sudden_ambush = make_buff( this, "sudden_ambush", talent.sudden_ambush->effectN( 1 ).trigger() );

  buff.tigers_fury = make_buff( this, "tigers_fury", talent.tigers_fury )
    ->set_cooldown( 0_ms )
    ->apply_affecting_aura( talent.predator );

  // Guardian buffs
  buff.berserk_bear =
      make_buff<berserk_bear_buff_t>( *this, "berserk_bear", spec.berserk_bear );

  buff.incarnation_bear =
      make_buff<berserk_bear_buff_t>( *this, "incarnation_guardian_of_ursoc", spec.incarnation_bear, true );

  buff.bristling_fur = make_buff( this, "bristling_fur", talent.bristling_fur )
    ->set_cooldown( 0_ms );

  buff.dream_of_cenarius = make_buff( this, "dream_of_cenarius", talent.dream_of_cenarius->effectN( 1 ).trigger() )
    ->set_cooldown( find_spell( 372523 )->duration() );

  buff.earthwarden = make_buff( this, "earthwarden", find_spell( 203975 ) )
    ->set_default_value( talent.earthwarden->effectN( 1 ).percent() );

  buff.galactic_guardian = make_buff( this, "galactic_guardian", find_spell( 213708 ) )
    ->set_cooldown( talent.galactic_guardian->internal_cooldown() )
    ->set_default_value_from_effect( 1, 0.1 /*RESOURCE_RAGE*/ );

  buff.gore = make_buff( this, "gore", find_spell( 93622 ) )
    ->set_chance( talent.gore->effectN( 1 ).percent() + sets->set( DRUID_GUARDIAN, T29, B4 )->effectN( 1 ).percent() )
    ->set_cooldown( talent.gore->internal_cooldown() )
    ->set_default_value_from_effect( 1, 0.1 /*RESOURCE_RAGE*/ );

  buff.gory_fur = make_buff( this, "gory_fur", talent.gory_fur->effectN( 1 ).trigger() )
    ->set_chance( talent.gory_fur->proc_chance() );

  buff.guardian_of_elune = make_buff( this, "guardian_of_elune", talent.guardian_of_elune->effectN( 1 ).trigger() );

  buff.overpowering_aura = make_buff( this, "overpowering_aura", find_spell( 395944 ) )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN );

  buff.rage_of_the_sleeper = make_buff( this, "rage_of_the_sleeper", talent.rage_of_the_sleeper )
    ->set_default_value_from_effect( 4, 0.01 );

  buff.tooth_and_claw = make_buff( this, "tooth_and_claw", talent.tooth_and_claw->effectN( 1 ).trigger() )
    ->set_chance( talent.tooth_and_claw->effectN( 1 ).percent() );

  buff.ursocs_fury = make_buff<absorb_buff_t>( this, "ursocs_fury", find_spell( 372505 ) );

  buff.vicious_cycle_mangle = make_buff( this, "vicious_cycle_mangle", find_spell( 372019) )
    ->set_default_value( talent.vicious_cycle->effectN( 1 ).percent() )
    ->set_name_reporting( "Vicious Cycle (Mangle)" );

  buff.vicious_cycle_maul = make_buff( this, "vicious_cycle_maul", find_spell( 372015 ) )
    ->set_default_value( talent.vicious_cycle->effectN( 1 ).percent() )
    ->set_name_reporting( "Vicious Cycle (Maul)" );

  // Guardian Conduits
  buff.savage_combatant = make_buff( this, "savage_combatant", conduit.savage_combatant->effectN( 1 ).trigger() )
    ->set_default_value( conduit.savage_combatant.percent() );

  // Restoration buffs
  buff.abundance = make_buff( this, "abundance", find_spell( 207640 ) )
    ->set_duration( 0_ms );

  buff.cenarion_ward = make_buff( this, "cenarion_ward", talent.cenarion_ward );

  buff.clearcasting_tree = make_buff( this, "clearcasting_tree", talent.omen_of_clarity_tree->effectN( 1 ).trigger() )
    ->set_chance( talent.omen_of_clarity_tree->effectN( 1 ).percent() );
  buff.clearcasting_tree->name_str_reporting = "clearcasting";

  buff.flourish = make_buff( this, "flourish", talent.flourish )
    ->set_default_value_from_effect( 2 );

  buff.incarnation_tree = make_buff( this, "incarnation_tree_of_life", find_spell( 5420 ) )
    ->set_duration( find_spell( 117679 )->duration() )  // 117679 is the generic incarnation shift proxy spell
    ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buff.natures_swiftness = make_buff( this, "natures_swiftness", talent.natures_swiftness )
    ->set_cooldown( 0_ms )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( !new_ )
        cooldown.natures_swiftness->start();
    } );

  buff.soul_of_the_forest_tree = make_buff( this, "soul_of_the_forest_tree", find_spell( 114108 ) );
  buff.soul_of_the_forest_tree->name_str_reporting = "soul_of_the_forest";

  if ( talent.yseras_gift.ok() )
  {
    buff.yseras_gift = make_buff( this, "yseras_gift_driver", talent.yseras_gift )
      ->apply_affecting_aura( talent.waking_dream->effectN( 1 ).trigger() )
      ->set_quiet( true )
      ->set_tick_zero( true )
      ->set_tick_callback( [this]( buff_t*, int, timespan_t ) {
          active.yseras_gift->schedule_execute();
        } );
  }

  // Covenant
  buff.convoke_the_spirits = make_buff( this, "convoke_the_spirits", talent.convoke_the_spirits )
    ->set_cooldown( 0_ms )
    ->set_period( 0_ms );
  if ( conduit.conflux_of_elements->ok() )
    buff.convoke_the_spirits->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.kindred_empowerment =
      make_buff<kindred_empowerment_buff_t>( *this, "kindred_empowerment" );

  buff.kindred_empowerment_partner =
      make_buff<kindred_empowerment_buff_t>( *this, "kindred_empowerment_partner", true );

  buff.kindred_empowerment_energize =
      make_buff( this, "kindred_empowerment_energize", cov.kindred_empowerment_energize )->set_period( 0_ms );

  buff.lone_empowerment = make_buff( this, "lone_empowerment", find_spell( 338142 ) )
    ->set_cooldown( 0_ms );

  // NOTE: this must come AFTER buff.kindred_empowerment_energize & buff.lone_empowerment
  buff.kindred_affinity = make_buff<kindred_affinity_buff_t>( *this );

  buff.ravenous_frenzy = make_buff( this, "ravenous_frenzy", cov.venthyr )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_period( 0_ms )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  if ( conduit.endless_thirst->ok() )
    buff.ravenous_frenzy->add_invalidate( CACHE_CRIT_CHANCE );

  buff.sinful_hysteria = make_buff( this, "ravenous_frenzy_sinful_hysteria", find_spell( 355315 ) )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  if ( conduit.endless_thirst->ok() )
    buff.sinful_hysteria->add_invalidate( CACHE_CRIT_CHANCE );
  if ( legendary.sinful_hysteria->ok() )
  {
    buff.sinful_hysteria->s_data_reporting = legendary.sinful_hysteria;
    buff.sinful_hysteria->name_str_reporting = "sinful_hysteria";
    buff.ravenous_frenzy->set_stack_change_callback( [ this ]( buff_t* b, int old_, int new_ ) {
      // spell data hasn't changed and still indicates 0.2s, but tooltip says 0.1s
      if ( old_ && new_ )
        b->extend_duration( this, 100_ms );
      else if ( old_ )
        buff.sinful_hysteria->trigger( old_ );
    } );
  }

  // Note you cannot replace buff checking with these, as it's possible to gain incarnation without the talent
  buff.b_inc_cat  = talent.incarnation_cat.ok()     ? buff.incarnation_cat     : buff.berserk_cat;
  buff.b_inc_bear = talent.incarnation_bear.ok()    ? buff.incarnation_bear    : buff.berserk_bear;
  buff.ca_inc     = talent.incarnation_moonkin.ok() ? buff.incarnation_moonkin : buff.celestial_alignment;
}

// Create active actions ====================================================
void druid_t::create_actions()
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace spells;
  using namespace heals;
  using namespace auto_attacks;

  // Melee Attacks
  if ( !caster_melee_attack )
    caster_melee_attack = new caster_melee_t( this );

  if ( !cat_melee_attack )
  {
    init_beast_weapon( cat_weapon, 1.0 );
    cat_melee_attack = new cat_melee_t( this );
  }

  if ( !bear_melee_attack )
  {
    init_beast_weapon( bear_weapon, 2.5 );
    bear_melee_attack = new bear_melee_t( this );
  }

  // General
  active.shift_to_caster = get_secondary_action<cancel_form_t>( "cancel_form_shift", "" );
  active.shift_to_caster->dual = true;
  active.shift_to_caster->background = true;
  active.shift_to_bear = get_secondary_action<bear_form_t>( "bear_form_shift", "" );
  active.shift_to_bear->dual = true;
  active.shift_to_cat = get_secondary_action<cat_form_t>( "cat_form_shift", "" );
  active.shift_to_cat->dual = true;
  active.shift_to_moonkin = get_secondary_action<moonkin_form_t>( "moonkin_form_shift", "" );
  active.shift_to_moonkin->dual = true;

  if ( legendary.lycaras_fleeting_glimpse->ok() )
    active.lycaras_fleeting_glimpse = new lycaras_fleeting_glimpse_t( this );

  // Balance
  if ( talent.astral_smolder.ok() )
    active.astral_smolder = get_secondary_action<astral_smolder_t>( "astral_smolder" );

  if ( talent.denizen_of_the_dream.ok() )
    active.denizen_of_the_dream = new denizen_of_the_dream_t( this );

  if ( talent.orbit_breaker.ok() )
  {
    auto fm = get_secondary_action_n<full_moon_t>( "orbit breaker", find_spell( 274283 ), "" );
    fm->s_data_reporting = talent.orbit_breaker;
    fm->base_multiplier = talent.orbit_breaker->effectN( 2 ).percent();
    fm->set_free_cast( free_spell_e::ORBIT );
    active.orbit_breaker = fm;
  }

  if ( talent.shooting_stars.ok() )
    active.shooting_stars = get_secondary_action<shooting_stars_t>( "shooting_stars" );

  if ( sets->has_set_bonus( DRUID_BALANCE, T29, B4 ) )
  {
    auto ss = get_secondary_action_n<starsurge_t>( "starsurge_cosmos", talent.starsurge, "" );
    ss->name_str_reporting = "touch_the_cosmos";
    ss->s_data_reporting = &buff.touch_the_cosmos->data();
    ss->set_free_cast( free_spell_e::COSMOS );
    active.starsurge_cosmos = ss;

    auto sf = get_secondary_action_n<starfall_t>( "starfall_cosmos", talent.starfall, "" );
    sf->name_str_reporting = "touch_the_cosmos";
    sf->s_data_reporting = &buff.touch_the_cosmos->data();
    sf->set_free_cast( free_spell_e::COSMOS );
    active.starfall_cosmos = sf;
  }

  if ( talent.starweaver.ok() )
  {
    auto ss = get_secondary_action_n<starsurge_t>( "starsurge_starweaver", talent.starsurge, "" );
    ss->name_str_reporting = "starweavers_weft";
    ss->s_data_reporting = &buff.starweavers_weft->data();
    ss->set_free_cast( free_spell_e::STARWEAVER );
    active.starsurge_starweaver = ss;

    auto sf = get_secondary_action_n<starfall_t>( "starfall_starweaver", talent.starfall, "" );
    sf->name_str_reporting = "starweavers_warp";
    sf->s_data_reporting = &buff.starweavers_warp->data();
    sf->set_free_cast( free_spell_e::STARWEAVER );
    active.starfall_starweaver = sf;
  }

  if ( talent.sundered_firmament.ok() )
  {
    auto firmament = get_secondary_action_n<fury_of_elune_t>( "sundered_firmament", find_spell( 394106 ),
                                                           find_spell( 394111 ), buff.sundered_firmament, "" );
    firmament->damage->base_multiplier = talent.sundered_firmament->effectN( 1 ).percent();
    firmament->s_data_reporting = talent.sundered_firmament;
    firmament->set_free_cast( free_spell_e::FIRMAMENT );
    active.sundered_firmament = firmament;
  }

  if ( talent.syzygy.ok() )
    active.syzygy = get_secondary_action<syzygy_t>( "syzygy" );
  
  // Feral
  if ( talent.apex_predators_craving.ok() )
  {
    auto apex = get_secondary_action_n<ferocious_bite_t>( "apex_predators_craving", "" );
    apex->s_data_reporting = talent.apex_predators_craving;
    apex->set_free_cast( free_spell_e::APEX );
    active.ferocious_bite_apex = apex;
  }

  if ( talent.berserk_frenzy.ok() )
    active.frenzied_assault = get_secondary_action<frenzied_assault_t>( "frenzied_assault" );

  // Guardian
  if ( talent.after_the_wildfire.ok() )
    active.after_the_wildfire_heal = get_secondary_action<after_the_wildfire_heal_t>( "after_the_wildfire" );

  if ( talent.brambles.ok() )
    active.brambles = get_secondary_action<brambles_t>( "brambles" );

  if ( talent.elunes_favored.ok() )
    active.elunes_favored_heal = get_secondary_action<elunes_favored_heal_t>( "elunes_favored" );

  if ( talent.galactic_guardian.ok() )
  {
    auto gg = get_secondary_action_n<moonfire_t>( "galactic_guardian", "" );
    gg->s_data_reporting = talent.galactic_guardian;
    gg->set_free_cast( free_spell_e::GALACTIC );
    gg->damage->set_free_cast( free_spell_e::GALACTIC );
    active.galactic_guardian = gg;
  }

  if ( mastery.natures_guardian->ok() )
    active.natures_guardian = new natures_guardian_t( this );

  if ( legendary.the_natural_orders_will->ok() )
    active.the_natural_orders_will = new the_natural_orders_will_t( this );

  if ( talent.flashing_claws.ok() )
  {
    auto flash = get_secondary_action_n<thrash_bear_t>( "flashing_claws",
                                                        apply_override( talent.thrash, spec.bear_form_override ), "" );
    flash->s_data_reporting = talent.flashing_claws;
    flash->name_str_reporting = "pawsitive_outlook";
    flash->set_free_cast( free_spell_e::FLASHING );
    active.thrash_bear_flashing = flash;
  }

  if ( talent.rage_of_the_sleeper.ok() )
  {
    auto rots = get_secondary_action<rage_of_the_sleeper_reflect_t>( "rage_of_the_sleeper_reflect" );
    rots->s_data_reporting = talent.rage_of_the_sleeper;
    rots->name_str_reporting = "rage_of_the_sleeper";
    active.rage_of_the_sleeper_reflect = rots;
  }

  // Restoration
  if ( talent.yseras_gift.ok() )
    active.yseras_gift = new yseras_gift_t( this );

  player_t::create_actions();

  // stat parent/child hookups
  auto find_parent = [ this ]( action_t* action, std::string_view n ) {
    if ( action )
    {
      if ( auto stat = find_stats( n ) )
      {
        for ( const auto& a : stat->action_list )
        {
          if ( a->data().ok() )
          {
            stat->add_child( action->stats );
            return;
          }
        }
      }
    }
  };

  find_parent( active.ferocious_bite_apex, "ferocious_bite" );
  find_parent( active.galactic_guardian, "moonfire" );
  find_parent( active.orbit_breaker, "full_moon" );
  find_parent( active.starsurge_cosmos, "starsurge" );
  find_parent( active.starfall_cosmos, "starfall" );
  find_parent( active.starsurge_starweaver, "starsurge" );
  find_parent( active.starfall_starweaver, "starfall" );
  find_parent( active.thrash_bear_flashing, "thrash_bear" );

  // setup dot_ids used by druid_action_t::get_dot_count()
  setup_dot_ids<sunfire_t::sunfire_damage_t>();
  setup_dot_ids<moonfire_t::moonfire_damage_t, lunar_inspiration_t>();
  setup_dot_ids<stellar_flare_t>();
  setup_dot_ids<rake_t::rake_bleed_t>();
  setup_dot_ids<rip_t>();
}

// Default Consumables ======================================================
std::string druid_t::default_flask() const
{
  if      ( true_level >= 60 ) return "spectral_flask_of_power";
  else if ( true_level < 40 )  return "disabled";

  switch ( specialization() )
  {
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
      return "greater_flask_of_endless_fathoms";
    case DRUID_FERAL:
    case DRUID_GUARDIAN:
      return "greater_flask_of_the_currents";
    default:
      return "disabled";
  }
}

std::string druid_t::default_potion() const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
      if      ( true_level >= 60 ) return "spectral_intellect";
      else if ( true_level >= 40 ) return "superior_battle_potion_of_intellect";
      SC_FALLTHROUGH;
    case DRUID_FERAL:
      if      ( true_level >= 60 ) return "spectral_agility";
      else if ( true_level >= 40 ) return "superior_battle_potion_of_agility";
      SC_FALLTHROUGH;
    case DRUID_GUARDIAN:
      if      ( true_level >= 60 ) return "phantom_fire";
      else if ( true_level >= 40 ) return "superior_battle_potion_of_agility";
      SC_FALLTHROUGH;
    default:
      return "disabled";
  }
}

std::string druid_t::default_food() const
{
  if      ( true_level >= 60 ) return "feast_of_gluttonous_hedonism";
  else if ( true_level >= 55 ) return "surprisingly_palatable_feast";
  else if ( true_level >= 45 ) return "famine_evaluator_and_snack_table";
  else return "disabled";
}

std::string druid_t::default_rune() const
{
  if      ( true_level >= 60 ) return "veiled";
  else if ( true_level >= 50 ) return "battle_scarred";
  else if ( true_level >= 45 ) return "defiled";
  else return "disabled";
}

std::string druid_t::default_temporary_enchant() const
{
  if ( true_level != 60 ) return "disabled";

  switch ( specialization() )
  {
    case DRUID_BALANCE: return "main_hand:shadowcore_oil";
    case DRUID_RESTORATION: return "main_hand:shadowcore_oil";
    case DRUID_GUARDIAN: return "main_hand:shadowcore_oil";
    case DRUID_FERAL: return "main_hand:shaded_sharpening_stone";
    default: return "disabled";
  }
}

// ALL Spec Pre-Combat Action Priority List =================================
void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Consumables
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
}

// NO Spec Combat Action Priority List ======================================
void druid_t::apl_default()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // Assemble Racials / On-Use Items / Professions
  for ( const auto& action_str : get_racial_actions() )
  {
    def->add_action( action_str );
  }
  for ( const auto& action_str : get_item_actions() )
  {
    def->add_action( action_str );
  }
  for ( const auto& action_str : get_profession_actions() )
  {
    def->add_action( action_str );
  }
}

// Action Priority Lists ========================================
void druid_t::apl_feral()
{
//    Good                 Okay        Stinky
//   ------               ----         -----
//  Night Fae  /\_/\     Kyrian       Venthyr (yuckers)
//           _| . . |_  Necrolords
//           >_  W  _<
//             |   |
// @Kotacat4, updated by @GuiltyasFeral
#include "class_modules/apl/feral_apl.inc"
}

void druid_t::apl_balance()
{
// Annotated APL can be found at https://balance-simc.github.io/Balance-SimC/md.html?file=balance.txt
#include "class_modules/apl/balance_apl.inc"
}

void druid_t::apl_guardian()
{
#include "class_modules/apl/guardian_apl.inc"
}

void druid_t::apl_restoration()
{
#include "class_modules/apl/restoration_druid_apl.inc"
}

// druid_t::init_scaling ====================================================
void druid_t::init_scaling()
{
  player_t::init_scaling();

  scaling->disable( STAT_STRENGTH );

  // workaround for resto dps scaling
  if ( specialization() == DRUID_RESTORATION )
  {
    scaling->disable( STAT_AGILITY );
    scaling->disable( STAT_MASTERY_RATING );
    scaling->disable( STAT_WEAPON_DPS );
    scaling->enable( STAT_INTELLECT );
  }

  // Save a copy of the weapon
  caster_form_weapon = main_hand_weapon;

  // Bear/Cat form weapons need to be scaled up if we are calculating scale factors for the weapon
  // dps. The actual cached cat/bear form weapons are created before init_scaling is called, so the
  // adjusted values for the "main hand weapon" have not yet been added.
  if ( sim->scaling->scale_stat == STAT_WEAPON_DPS )
  {
    if ( cat_weapon.damage > 0 )
    {
      auto coeff = sim->scaling->scale_value * cat_weapon.swing_time.total_seconds();
      cat_weapon.damage += coeff;
      cat_weapon.min_dmg += coeff;
      cat_weapon.max_dmg += coeff;
      cat_weapon.dps += sim->scaling->scale_value;
    }

    if ( bear_weapon.damage > 0 )
    {
      auto coeff = sim->scaling->scale_value * bear_weapon.swing_time.total_seconds();
      bear_weapon.damage += coeff;
      bear_weapon.min_dmg += coeff;
      bear_weapon.max_dmg += coeff;
      bear_weapon.dps += sim->scaling->scale_value;
    }
  }
}

// druid_t::init ============================================================
void druid_t::init()
{
  player_t::init();

  switch ( specialization() )
  {
    case DRUID_BALANCE:
      action_list_information +=
        "\n# Annotated Balance APL can be found at https://balance-simc.github.io/Balance-SimC/md.html?file=balance.txt\n";
      break;
    case DRUID_GUARDIAN:
      action_list_information +=
        "\n# Guardian APL can be found at https://www.dreamgrove.gg/sims/bear/guardian.txt\n";
      break;
    case DRUID_RESTORATION:
      action_list_information +=
        "\n# Restoration DPS APL can be found at https://www.dreamgrove.gg/sims/tree/restoration.txt\n";
      break;
    default:
      break;
  }

  if ( sim->fight_style == FIGHT_STYLE_DUNGEON_SLICE || sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE )
  {
    options.adaptive_swarm_friendly_targets = 5;
  }

  if ( covenant->type() == covenant_e::KYRIAN && !options.kindred_spirits_target_str.empty() )
  {
    auto it = range::find( sim->player_no_pet_list, options.kindred_spirits_target_str, &player_t::name_str );
    if ( it == sim->player_no_pet_list.end() )
      throw std::invalid_argument( "Kindred Spirits target player not found." );

    if ( *it == this )
      throw std::invalid_argument( "Cannot bond with yourself. Use 'druid.lone_empowerment=1'" );
    else
      options.kindred_spirits_target = *it;
  }
}

bool druid_t::validate_fight_style( fight_style_e style ) const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:
      if ( style == FIGHT_STYLE_PATCHWERK || style == FIGHT_STYLE_CASTING_PATCHWERK )
        return true;
      else
        return false;
    case DRUID_FERAL:
    case DRUID_GUARDIAN:
    case DRUID_RESTORATION:
    default:
      break;
  }

  return true;
}

// druid_t::init_gains ======================================================
void druid_t::init_gains()
{
  player_t::init_gains();

  if ( specialization() == DRUID_BALANCE )
  {
    gain.natures_balance     = get_gain( "Natures Balance" );
    gain.stellar_innervation = get_gain( "Stellar Innervation" );
  }
  else if ( specialization() == DRUID_FERAL )
  {
    gain.energy_refund       = get_gain( "Energy Refund" );
    gain.primal_claws        = get_gain( "Primal Claws" );
    gain.primal_fury         = get_gain( "Primal Fury" );
    gain.berserk             = get_gain( "Berserk" );
    gain.cats_curiosity      = get_gain( "Cat's Curiosity" );
    gain.incessant_hunter    = get_gain( "Incessant Hunter" );
    gain.tigers_tenacity     = get_gain( "Tiger's Tenacity" );
  }
  else if ( specialization() == DRUID_GUARDIAN )
  {
    gain.bear_form           = get_gain( "Bear Form" );
    gain.blood_frenzy        = get_gain( "Blood Frenzy" );
    gain.brambles            = get_gain( "Brambles" );
    gain.bristling_fur       = get_gain( "Bristling Fur" );
    gain.gore                = get_gain( "Gore" );
    gain.rage_from_melees    = get_gain( "Rage from Melees" );
  }
  // Multi-spec
  if ( specialization() == DRUID_FERAL || specialization() == DRUID_RESTORATION )
  {
    gain.clearcasting = get_gain( "Clearcasting" );  // Feral & Restoration
  }
  if ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN )
  {
    gain.soul_of_the_forest = get_gain( "Soul of the Forest" );  // Feral & Guardian
  }
}

// druid_t::init_procs ======================================================
void druid_t::init_procs()
{
  player_t::init_procs();

  // Balance
  proc.pulsar                 = get_proc( "Primordial Arcanic Pulsar" )->collect_interval();

  // Feral
  proc.predator               = get_proc( "Predator" );
  proc.predator_wasted        = get_proc( "Predator (Wasted)" );
  proc.primal_claws           = get_proc( "Primal Claws" );
  proc.primal_fury            = get_proc( "Primal Fury" );
  proc.clearcasting           = get_proc( "Clearcasting" );
  proc.clearcasting_wasted    = get_proc( "Clearcasting (Wasted)" );

  // Guardian
  proc.gore = get_proc( "Gore" );
}

// druid_t::init_uptimes ====================================================
void druid_t::init_uptimes()
{
  player_t::init_uptimes();

  std::string ca_inc_str;

  if ( talent.incarnation_moonkin.ok() )
    ca_inc_str = "Incarnation";
  else
    ca_inc_str = "Celestial Alignment";

  uptime.combined_ca_inc           = get_uptime( ca_inc_str + " (Total)" )->collect_uptime( *sim )->collect_duration( *sim );
  uptime.primordial_arcanic_pulsar = get_uptime( ca_inc_str + " (Pulsar)" )->collect_uptime( *sim );
  uptime.friend_of_the_fae         = get_uptime( "Friend of the Fae" )->collect_uptime( *sim )->collect_duration( *sim );
  uptime.eclipse_lunar             = get_uptime( "Lunar Eclipse Only" )->collect_uptime( *sim )->collect_duration( *sim );
  uptime.eclipse_solar             = get_uptime( "Solar Eclipse Only" )->collect_uptime( *sim );
  uptime.eclipse_none              = get_uptime( "No Eclipse" )->collect_uptime( *sim )->collect_duration( *sim );
}

// druid_t::init_resources ==================================================
void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ]         = 0;
  resources.current[ RESOURCE_COMBO_POINT ]  = 0;
  if ( options.initial_astral_power == 0.0 && talent.natures_balance.ok() )
    resources.current[RESOURCE_ASTRAL_POWER] = 50.0;
  else
    resources.current[RESOURCE_ASTRAL_POWER] = options.initial_astral_power;
  expected_max_health = calculate_expected_max_health();
}

// druid_t::init_rng ========================================================
void druid_t::init_rng()
{
  player_t::init_rng();

  // RPPM objects
  rppm.predator = get_rppm( "predator", options.predator_rppm_rate );  // Predator: optional RPPM approximation.
}

// druid_t::init_special_effects ============================================
void druid_t::init_special_effects()
{
  player_t::init_special_effects();

  if ( talent.denizen_of_the_dream.ok() )
  {
    struct denizen_of_the_dream_cb_t : public dbc_proc_callback_t
    {
      timespan_t dur;

      denizen_of_the_dream_cb_t( druid_t* p, const special_effect_t& e )
        : dbc_proc_callback_t( p, e ), dur( p->find_spell( 394076 )->duration() )
      {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( a->id == p()->spec.moonfire_dmg->id() || a->id == p()->spec.sunfire_dmg->id() )
          dbc_proc_callback_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* ) override
      {
        p()->active.denizen_of_the_dream->execute();
      }

      druid_t* p() { return static_cast<druid_t*>( listener ); }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.denizen_of_the_dream->name_cstr();
    driver->spell_id = talent.denizen_of_the_dream->id();
    special_effects.push_back( driver );

    auto cb = new denizen_of_the_dream_cb_t( this, *driver );
    cb->initialize();
  }

  if ( talent.umbral_embrace.ok() )
  {
    struct umbral_embrace_cb_t : public dbc_proc_callback_t
    {
      umbral_embrace_cb_t( druid_t* p, const special_effect_t& e ) : dbc_proc_callback_t( p, e ) {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( a->get_school() == SCHOOL_ASTRAL )
          dbc_proc_callback_t::trigger( a, s );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.umbral_embrace->name_cstr();
    driver->spell_id = talent.umbral_embrace->id();
    driver->custom_buff = buff.umbral_embrace;
    special_effects.push_back( driver );

    auto cb = new umbral_embrace_cb_t( this, *driver );
    cb->initialize();
  }

  if ( talent.ashamanes_guidance.ok() && talent.incarnation_cat.ok() )
  {
    struct ashamanes_guidance_cb_t : public dbc_proc_callback_t
    {
      druid_t* druid;
      timespan_t dur;

      ashamanes_guidance_cb_t( druid_t* p, const special_effect_t& e )
        : dbc_proc_callback_t( p, e ),
          druid( p ),
          dur( timespan_t::from_seconds( p->spec.ashamanes_guidance->effectN( 1 ).base_value() ) )
      {}

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a , s );

        druid->buff.incarnation_cat->trigger( dur );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = "ashamanes_guidance_driver";
    driver->spell_id = spec.ashamanes_guidance->id();
    driver->proc_flags_ = PF_MELEE;
    driver->ppm_ = -0.95;  // TODO: determine what this is. for now guesstimating at old vision of perfection rppm
    special_effects.push_back( driver );

    auto cb = new ashamanes_guidance_cb_t( this, *driver );
    cb->initialize();
  }
}

// druid_t::init_actions ====================================================
void druid_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  apl_precombat();  // PRE-COMBAT

  switch ( specialization() )
  {
    case DRUID_FERAL:       apl_feral();       break;
    case DRUID_BALANCE:     apl_balance();     break;
    case DRUID_GUARDIAN:    apl_guardian();    break;
    case DRUID_RESTORATION: apl_restoration(); break;
    default: apl_default(); break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// druid_t::reset ===========================================================
void druid_t::reset()
{
  player_t::reset();

  // Reset druid_t variables to their original state.
  form = NO_FORM;
  base_gcd = 1.5_s;
  eclipse_handler.reset_stacks();
  eclipse_handler.reset_state();

  // Restore main hand attack / weapon to normal state
  main_hand_attack = caster_melee_attack;
  main_hand_weapon = caster_form_weapon;

  if ( mastery.natures_guardian->ok() )
    recalculate_resource_max( RESOURCE_HEALTH );

  // Reset runtime variables
  moon_stage = static_cast<moon_stage_e>( options.initial_moon_stage );
  after_the_wildfire_counter = 0.0;
  persistent_event_delay.clear();
  lycaras_event = nullptr;
  lycaras_event_remains = 0_ms;
  swarm_tracker.clear();
}

// druid_t::merge ===========================================================
void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0; i < counters.size(); i++ )
    counters[ i ]->merge( *od.counters[ i ] );

  eclipse_handler.merge( od.eclipse_handler );
}

void druid_t::datacollection_begin()
{
  player_t::datacollection_begin();

  eclipse_handler.datacollection_begin();
}

void druid_t::datacollection_end()
{
  player_t::datacollection_end();

  eclipse_handler.datacollection_end();
}

void druid_t::analyze( sim_t& sim )
{
  if ( active.denizen_of_the_dream )
    range::for_each( active.denizen_of_the_dream->child_action, [ this ]( action_t* a ) { a->stats->player = this; } );

  player_t::analyze( sim );
}

// druid_t::mana_regen_per_second ===========================================
double druid_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  if ( r == RESOURCE_MANA )
  {
    if ( specialization() == DRUID_BALANCE && buff.moonkin_form->check() )
      reg *= ( 1.0 + buff.moonkin_form->data().effectN( 5 ).percent() ) / cache.spell_haste();
  }

  return reg;
}

// druid_t::available =======================================================
timespan_t druid_t::available() const
{
  if ( primary_resource() != RESOURCE_ENERGY )
    return player_t::available();

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 )
    return 100_ms;

  return std::max( timespan_t::from_seconds( ( 25 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) ), 100_ms );
}

// druid_t::arise ===========================================================
void druid_t::arise()
{
  player_t::arise();

  if ( talent.lycaras_teachings.ok() )
    persistent_event_delay.push_back( make_event<persistent_delay_event_t>( *sim, this, buff.lycaras_teachings ) );

  if ( talent.natures_balance.ok() )
    buff.natures_balance->trigger();

  if ( buff.yseras_gift )
    persistent_event_delay.push_back( make_event<persistent_delay_event_t>( *sim, this, buff.yseras_gift ) );
}

void druid_t::combat_begin()
{
  player_t::combat_begin();

  if ( spec.astral_power->ok() )
  {
    eclipse_handler.reset_stacks();

    switch ( eclipse_handler.state )
    {
      case eclipse_state_e::IN_BOTH:
        uptime.eclipse_lunar->update( true, sim->current_time() );
        uptime.eclipse_solar->update( true, sim->current_time() );
        break;
      case eclipse_state_e::IN_LUNAR:
        uptime.eclipse_lunar->update( true, sim->current_time() );
        break;
      case eclipse_state_e::IN_SOLAR:
        uptime.eclipse_solar->update( true, sim->current_time() );
        break;
      default:
        uptime.eclipse_none->update( true, sim->current_time() );
        break;
    }

    if ( options.raid_combat )
    {
      double cap = talent.natures_balance.ok() ? 50.0 : 20.0;
      double curr = resources.current[ RESOURCE_ASTRAL_POWER ];

      resources.current[ RESOURCE_ASTRAL_POWER ] = std::min( cap, curr );
      if ( curr > cap )
        sim->print_debug( "Astral Power capped at combat start to {} (was {})", cap, curr );
    }
  }

  if ( talent.adaptive_swarm.ok() && !prepull_swarm.empty() && find_action( "adaptive_swarm" ) )
  {
    using swarm_t = spells::adaptive_swarm_t::adaptive_swarm_heal_t;

    auto heal = get_secondary_action<swarm_t>( "adaptive_swarm_heal" );

    for ( auto [ min_stack, max_stack, min_dur, max_dur, chance ] : prepull_swarm )
    {
      if ( !rng().roll( chance ) )
        continue;

      auto stacks   = rng().range( min_stack, max_stack );
      auto duration = rng().range( min_dur, max_dur );
      auto range    = rng().roll( 0.5 ) ? rng().gauss( options.adaptive_swarm_jump_distance_melee,
                                                       options.adaptive_swarm_jump_distance_stddev, true )
                                        : rng().gauss( options.adaptive_swarm_jump_distance_ranged,
                                                       options.adaptive_swarm_jump_distance_stddev, true );

      swarm_tracker.push_back(
          make_event<swarm_t::adaptive_swarm_heal_event_t>( *sim, this, heal, range, stacks, duration ) );
    }
  }

  // Legendary-related buffs & events
  if ( legendary.lycaras_fleeting_glimpse->ok() )
  {
    lycaras_event = make_event( *sim, timespan_t::from_seconds( buff.lycaras_fleeting_glimpse->default_value ),
                                [ this ]() { buff.lycaras_fleeting_glimpse->trigger(); } );
  }

  if ( legendary.kindred_affinity->ok() )
  {
    buff.kindred_affinity->increment();  // this is a persistent buff with no proc chance, so trigger() will return false
  }
}

// druid_t::recalculate_resource_max ========================================
void druid_t::recalculate_resource_max( resource_e rt, gain_t* source )
{
  double pct_health = 0;
  double current_health = 0;
  bool adjust_natures_guardian_health = mastery.natures_guardian->ok() && rt == RESOURCE_HEALTH;

  if ( adjust_natures_guardian_health )
  {
    current_health = resources.current[ rt ];
    pct_health     = resources.pct( rt );
  }

  player_t::recalculate_resource_max( rt, source );

  if ( adjust_natures_guardian_health )
  {
    resources.max[ rt ] *= 1.0 + cache.mastery_value();
    // Maintain current health pct.
    resources.current[ rt ] = resources.max[ rt ] * pct_health;

    sim->print_log( "{} recalculates maximum health. old_current={:.0f} new_current={:.0f} net_health={:.0f}", name(),
                    current_health, resources.current[ rt ], resources.current[ rt ] - current_health );
  }
}

// druid_t::invalidate_cache ================================================
void druid_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_ATTACK_POWER:
      if ( specialization() == DRUID_GUARDIAN || specialization() == DRUID_FERAL )
        invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_SPELL_POWER:
      if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
        invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_MASTERY:
      if ( mastery.natures_guardian->ok() )
      {
        invalidate_cache( CACHE_ATTACK_POWER );
        recalculate_resource_max( RESOURCE_HEALTH );
      }
      break;
    case CACHE_CRIT_CHANCE:
      if ( specialization() == DRUID_GUARDIAN )
        invalidate_cache( CACHE_DODGE );
      break;
    case CACHE_AGILITY:
      if ( buff.ironfur->check() )
        invalidate_cache( CACHE_ARMOR );
      break;
    default: break;
  }
}

// Composite combat stat override functions =================================

// Attack Power =============================================================
double druid_t::composite_melee_attack_power() const
{
  if ( spec_override.attack_power )
    return spec_override.attack_power * composite_spell_power_multiplier() * cache.spell_power( SCHOOL_MAX );

  return player_t::composite_melee_attack_power();
}

double druid_t::composite_melee_attack_power_by_type( attack_power_type type ) const
{
  if ( spec_override.attack_power )
    return spec_override.attack_power * composite_spell_power_multiplier() * cache.spell_power( SCHOOL_MAX );

  return player_t::composite_melee_attack_power_by_type( type );
}

double druid_t::composite_attack_power_multiplier() const
{
  if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
    return 1.0;

  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.natures_guardian->ok() )
    ap *= 1.0 + cache.mastery() * mastery.natures_guardian_AP->effectN( 1 ).mastery_value();

  return ap;
}

// Armor ====================================================================
double druid_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buff.ironfur->up() )
    a += ( buff.ironfur->check_stack_value() * cache.agility() );

  return a;
}

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form->check() )
    a *= 1.0 + buff.bear_form->data().effectN( 4 ).percent();

  if ( buff.ursine_vigor->check() )
    a *= 1.0 + buff.ursine_vigor->check_value();

  return a;
}

// Critical Strike ==========================================================
double druid_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spec.critical_strikes->effectN( 1 ).percent();

  crit += buff.ravenous_frenzy->check() * conduit.endless_thirst.percent() / 10.0;
  crit += buff.sinful_hysteria->check() * conduit.endless_thirst.percent() / 10.0;

  return crit;
}

double druid_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spec.critical_strikes->effectN( 1 ).percent();

  crit += buff.ravenous_frenzy->check() * conduit.endless_thirst.percent() / 10.0;
  crit += buff.sinful_hysteria->check() * conduit.endless_thirst.percent() / 10.0;

  return crit;
}

// Defense ==================================================================
double druid_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  if ( buff.bear_form->check() )
    c += buff.bear_form->data().effectN( 3 ).percent();

  return c;
}

double druid_t::composite_dodge_rating() const
{
  double dr = player_t::composite_dodge_rating();

  if ( spec.lightning_reflexes->ok() )
    dr += composite_rating( RATING_MELEE_CRIT ) * spec.lightning_reflexes->effectN( 1 ).percent();

  return dr;
}

double druid_t::composite_leech() const
{
  double l = player_t::composite_leech();

  if ( legendary.legacy_of_the_sleeper->ok() && ( buff.berserk_bear->check() || buff.incarnation_bear->check() ) )
    l *= 1.0 + spec.berserk_bear->effectN( 8 ).percent();

  return l;
}

// Miscellaneous ============================================================
double druid_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  switch ( attr )
  {
    case ATTR_STAMINA:
      if ( buff.bear_form->check() )
        m *= 1.0 + spec.bear_form_passive->effectN( 2 ).percent() + spec.bear_form_2->effectN( 1 ).percent();
      break;
    default: break;
  }

  return m;
}

double druid_t::matching_gear_multiplier( attribute_e attr ) const
{
  unsigned idx;

  switch ( attr )
  {
    case ATTR_AGILITY: idx = 1; break;
    case ATTR_INTELLECT: idx = 2; break;
    case ATTR_STAMINA: idx = 3; break;
    default: return 0;
  }

  return spec.leather_specialization->effectN( idx ).percent();
}

double druid_t::composite_melee_expertise( const weapon_t* ) const
{
  double exp = player_t::composite_melee_expertise();

  if ( buff.bear_form->check() )
    exp += buff.bear_form->data().effectN( 6 ).base_value();

  return exp;
}

// Movement =================================================================
double druid_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

  if ( buff.dash->up() && buff.cat_form->check() )
    active = std::max( active, buff.dash->check_value() );

  if ( buff.wild_charge_movement->check() )
    active = std::max( active, buff.wild_charge_movement->check_value() );

  if ( buff.tiger_dash->up() && buff.cat_form->check() )
    active = std::max( active, buff.tiger_dash->check_value() );

  return active;
}

double druid_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buff.cat_form->up() )
    ms += spec.cat_form_speed->effectN( 1 ).percent();

  ms += talent.feline_swiftness->effectN( 1 ).percent();

  ms += talent.front_of_the_pack->effectN( 3 ).percent();

  return ms;
}

// Spell Power ==============================================================
double druid_t::composite_spell_power( school_e school ) const
{
  if ( spec_override.spell_power )
  {
    return composite_melee_attack_power_by_type( attack_power_type::WEAPON_MAINHAND ) *
           composite_attack_power_multiplier() * spec_override.spell_power;
  }

  return player_t::composite_spell_power( school );
}

double druid_t::composite_spell_power_multiplier() const
{
  if ( specialization() == DRUID_GUARDIAN || specialization() == DRUID_FERAL )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

double druid_t::composite_player_multiplier( school_e school ) const
{
  auto cpm = player_t::composite_player_multiplier( school );

  // TODO: bugged so that they don't stack, but in-game other instances of multiple buffs of the same school do stack
  if ( dbc::has_common_school( school, SCHOOL_ARCANE ) && get_form() == BEAR_FORM )
  {
    if ( bugs )
    {
      auto ef = talent.elunes_favored->effectN( 1 ).percent();
      auto fn = talent.fury_of_nature->effectN( 1 ).percent();

      cpm *= 1.0 + std::max( ef, fn );
    }
    else
    {
      cpm *= 1.0 + talent.elunes_favored->effectN( 1 ).percent();
      cpm *= 1.0 + talent.fury_of_nature->effectN( 1 ).percent();
    }
  }

  // NOTE: working as intended that multi school spells only take the highest buff and don't stack.
  // TODO: check if workaround for this is created
  if ( buff.eclipse_lunar->check() && buff.eclipse_lunar->has_common_school( school ) )
    cpm *= 1.0 + buff.eclipse_lunar->value();

  if ( buff.eclipse_solar->check() && buff.eclipse_solar->has_common_school( school ) )
    cpm *= 1.0 + buff.eclipse_solar->value();

  if ( buff.friend_of_the_fae->check() && buff.friend_of_the_fae->has_common_school( school ) )
    cpm *= 1.0 + buff.friend_of_the_fae->value();

  return cpm;
}

double druid_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  auto cptm = player_t::composite_player_target_multiplier( target, school );

  if ( specialization() == DRUID_BALANCE )
  {
    auto td = get_target_data( target );

    if ( bugs )
    {
      if ( dbc::has_common_school( school, SCHOOL_ARCANE ) )
        cptm *= 1.0 + td->debuff.moonfire->check_value();

      if ( dbc::has_common_school( school, SCHOOL_NATURE ) )
        cptm *= 1.0 + td->debuff.sunfire->check_value();
    }
    else
    {
      if ( dbc::has_common_school( school, SCHOOL_ARCANE ) && td->dots.moonfire->is_ticking() )
        cptm *= 1.0 + cache.mastery_value();

      if ( dbc::has_common_school( school, SCHOOL_NATURE ) && td->dots.sunfire->is_ticking() )
        cptm *= 1.0 + cache.mastery_value();
    }

    if ( talent.waning_twilight.ok() &&
         td->dots_ticking() >= as<int>( talent.waning_twilight->effectN( 3 ).base_value() ) )
    {
      cptm *= 1.0 + talent.waning_twilight->effectN( 1 ).percent();
    }
  }

  return cptm;
}

// Expressions ==============================================================
template <class Base>
std::unique_ptr<expr_t> druid_action_t<Base>::create_expression( std::string_view name_str )
{
  auto splits = util::string_split<std::string_view>( name_str, "." );

  if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "dot" ) &&
       util::str_compare_ci( splits[ 1 ], "adaptive_swarm_heal" ) )
  {
    using sw_event_t = spells::adaptive_swarm_t::adaptive_swarm_heal_t::adaptive_swarm_heal_event_t;

    struct adaptive_swarm_heal_expr_t : public expr_t
    {
      druid_t* player;
      std::function<double( sw_event_t* )> func;

      adaptive_swarm_heal_expr_t( druid_t* p, std::function<double( sw_event_t* )> f )
        : expr_t( "dot.adaptive_swarm_heal" ), player( p ), func( std::move( f ) )
      {}

      double evaluate() override
      {
        if ( player->swarm_tracker.empty() )
          return 0.0;

        auto tracker = player->swarm_tracker;
        range::sort( tracker, []( const event_t* l, const event_t* r ) {
          return l->remains() < r->remains();
        } );

        auto sw_event = dynamic_cast<sw_event_t*>( tracker.front() );
        if ( sw_event )
          return func( sw_event );
        else
          return 0.0;
      }
    };

    if ( util::str_compare_ci( splits[ 2 ], "count" ) )
    {
      return make_fn_expr( name_str, [ this ]() {
        return p()->swarm_tracker.size();
      } );
    }
    else if ( util::str_compare_ci( splits[ 2 ], "remains" ) )
    {
      return std::make_unique<adaptive_swarm_heal_expr_t>( p(), []( const sw_event_t* e ) {
        return e->remains().total_seconds();
      } );
    }
    else if ( util::str_compare_ci( splits[ 2 ], "stack" ) )
    {
      return std::make_unique<adaptive_swarm_heal_expr_t>( p(), []( const sw_event_t* e ) {
        return as<double>( e->stacks );
      } );
    }
  }

  return ab::create_expression( name_str );
}

std::unique_ptr<expr_t> druid_t::create_action_expression( action_t& a, std::string_view name_str )
{
  auto splits = util::string_split<std::string_view>( name_str, "." );

  if ( splits[ 0 ] == "ticks_gained_on_refresh" ||
       ( splits.size() > 2 && ( splits[ 0 ] == "druid" || splits[ 0 ] == "dot" ) &&
         splits[ 2 ] == "ticks_gained_on_refresh" ) )
  {
    bool pmul = false;
    if ( ( splits.size() > 1 && splits[ 1 ] == "pmult" ) || ( splits.size() > 4 && splits[ 3 ] == "pmult" ) )
      pmul = true;

    action_t* dot_action = nullptr;

    if ( splits.size() > 2 )
    {
      if ( splits[ 1 ] == "moonfire_cat" )
        dot_action = find_action( "lunar_inspiration" );
      else if ( splits[ 1 ] == "rake" )
        dot_action = find_action( "rake_bleed" );
      else
        dot_action = find_action( splits[ 1 ] );

      if ( !dot_action )
        throw std::invalid_argument( "invalid action specified in ticks_gained_on_refresh" );
    }
    else
      dot_action = &a;

    action_t* source_action = &a;
    double multiplier = 1.0;

    if ( dot_action->name_str == "primal_wrath" )
    {
      dot_action = find_action( "rip" );
      source_action = find_action( "primal_wrath" );
      multiplier = 0.5;
    }

    if ( dot_action->name_str == "thrash_cat" )
    {
      source_action = find_action( "thrash_cat" );
    }

    return make_fn_expr( name_str, [ dot_action, source_action, multiplier, pmul ]() -> double {
      auto ticks_gained_func = []( double mod, action_t* dot_action, player_t* target, bool pmul ) -> double {
        action_state_t* state = dot_action->get_state();
        state->target = target;
        dot_action->snapshot_state( state, result_amount_type::DMG_OVER_TIME );

        dot_t* dot = dot_action->get_dot( target );
        timespan_t ttd = target->time_to_percent( 0 );
        timespan_t duration = dot_action->composite_dot_duration( state ) * mod;

        double remaining_ticks = std::min( dot->remains(), ttd ) / dot_action->tick_time( state ) *
                                 ( ( pmul && dot->state ) ? dot->state->persistent_multiplier : 1.0 );
        double new_ticks = std::min( dot_action->calculate_dot_refresh_duration( dot, duration ), ttd ) /
                           dot_action->tick_time( state ) * ( pmul ? state->persistent_multiplier : 1.0 );

        action_state_t::release( state );
        return new_ticks - remaining_ticks;
      };

      if ( source_action->aoe == -1 )
      {
        double accum = 0.0;
        for ( player_t* target : source_action->targets_in_range_list( source_action->target_list() ) )
          accum += ticks_gained_func( multiplier, dot_action, target, pmul );

        return accum;
      }

      return ticks_gained_func( multiplier, dot_action, source_action->target, pmul );
    } );
  }

  return player_t::create_action_expression( a, name_str );
}

std::unique_ptr<expr_t> druid_t::create_expression( std::string_view name_str )
{
  auto splits = util::string_split<std::string_view>( name_str, "." );

  if ( util::str_compare_ci( splits[ 0 ], "druid" ) && splits.size() > 1 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "catweave_bear" ) && splits.size() == 2 )
      return make_fn_expr( "catweave_bear", [ this ]() {
        return options.catweave_bear;
      } );
    if ( util::str_compare_ci( splits[ 1 ], "owlweave_bear" ) && splits.size() == 2 )
      return make_fn_expr( "owlweave_bear", [ this ]() {
        return options.owlweave_bear;
      } );
    if ( util::str_compare_ci( splits[ 1 ], "owlweave_cat" ) && splits.size() == 2 )
      return make_fn_expr( "owlweave_cat", [ this ]() {
        return options.owlweave_cat;
      } );
    if ( util::str_compare_ci( splits[ 1 ], "no_cds" ) && splits.size() == 2 )
      return make_fn_expr( "no_cds", [ this ]() {
        return options.no_cds;
      } );
    if ( util::str_compare_ci( splits[ 1 ], "time_spend_healing" ) && splits.size() == 2 )
      return make_fn_expr( "time_spend_healing", [ this ]() {
        return options.time_spend_healing;
      } );
    if ( util::str_compare_ci( splits[1], "delay_berserking" ) && splits.size() == 2 )
      return make_fn_expr( "delay_berserking", [this]() {
      return options.delay_berserking;
    } );
  }

  if ( splits[ 0 ] == "action" && splits[ 1 ] == "ferocious_bite_max" && splits[ 2 ] == "damage" )
  {
    action_t* action = find_action( "ferocious_bite" );

    return make_fn_expr( name_str, [ action, this ]() -> double {
      action_state_t* state = action->get_state();
      state->n_targets      = 1;
      state->chain_target   = 0;
      state->result         = RESULT_HIT;

      action->snapshot_state( state, result_amount_type::DMG_DIRECT );
      state->target = action->target;
      //  (p()->resources.current[RESOURCE_ENERGY] - cat_attack_t::cost()));

      // combo_points = p()->resources.current[RESOURCE_COMBO_POINT];
      double current_energy                           = this->resources.current[ RESOURCE_ENERGY ];
      double current_cp                               = this->resources.current[ RESOURCE_COMBO_POINT ];
      this->resources.current[ RESOURCE_ENERGY ]      = 50;
      this->resources.current[ RESOURCE_COMBO_POINT ] = 5;

      double amount;
      state->result_amount = action->calculate_direct_amount( state );
      state->target->target_mitigation( action->get_school(), result_amount_type::DMG_DIRECT, state );
      amount = state->result_amount;
      amount *= 1.0 + clamp( state->crit_chance + state->target_crit_chance, 0.0, 1.0 ) *
                          action->composite_player_critical_multiplier( state );

      this->resources.current[ RESOURCE_ENERGY ]      = current_energy;
      this->resources.current[ RESOURCE_COMBO_POINT ] = current_cp;

      action_state_t::release( state );
      return amount;
    } );
  }

  if ( util::str_compare_ci( name_str, "active_bt_triggers" ) )
  {
    return make_fn_expr( "active_bt_triggers", [ this ]() {
      return buff.bt_rake->check() + buff.bt_shred->check() + buff.bt_swipe->check() + buff.bt_thrash->check() +
             buff.bt_moonfire->check() + buff.bt_brutal_slash->check();
    } );
  }

  if ( util::str_compare_ci( name_str, "bt_trigger_remains" ) )
  {
    return make_fn_expr( "bt_trigger_remains", [ this ]() {
      return std::min( std::initializer_list<double>( {
          buff.bt_rake->check() ? buff.bt_rake->remains().total_seconds() : 6.0,
          buff.bt_shred->check() ? buff.bt_shred->remains().total_seconds() : 6.0,
          buff.bt_moonfire->check() ? buff.bt_moonfire->remains().total_seconds() : 6.0,
          buff.bt_swipe->check() ? buff.bt_swipe->remains().total_seconds() : 6.0,
          buff.bt_thrash->check() ? buff.bt_thrash->remains().total_seconds() : 6.0,
          buff.bt_brutal_slash->check() ? buff.bt_brutal_slash->remains().total_seconds() : 6.0,
      } ) );
    } );
  }

  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "bs_inc" ) )
  {
    // special case since incarnation is handled via the proxy action and the cooldown obj is not explicitly named for
    // the spec variant
    if ( util::str_compare_ci( splits[ 0 ], "cooldown" ) &&
         ( talent.incarnation_cat.ok() || talent.incarnation_bear.ok() ) )
    {
      splits[ 1 ] = "incarnation";
    }
    else if ( specialization() == DRUID_FERAL )
    {
      if ( talent.incarnation_cat.ok() )
        splits[ 1 ] = "incarnation_king_of_the_jungle";
      else
        splits[ 1 ] = "berserk_cat";
    }
    else if ( specialization() == DRUID_GUARDIAN )
    {
      if ( talent.incarnation_bear.ok() )
        splits[ 1 ] = "incarnation_guardian_of_ursoc";
      else
        splits[ 1 ] = "berserk_bear";
    }
    else
    {
      splits[ 1 ] = "berserk";
    }

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "clearcasting" ) )
  {
    if ( specialization() == DRUID_FERAL )
      splits[ 1 ] = "clearcasting_cat";
    else
      splits[ 1 ] = "clearcasting_tree";

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 0 ], "cooldown" ) &&
       ( util::str_compare_ci( splits[ 1 ], "kindred_spirits" ) ||
         util::str_compare_ci( splits[ 1 ], "kindred_empowerment" ) ||
         util::str_compare_ci( splits[ 1 ], "lone_empowerment" ) ) )
  {
    splits[ 1 ] = "empower_bond";

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  if ( util::str_compare_ci( name_str, "combo_points" ) )
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );

  if ( specialization() == DRUID_BALANCE )
  {
    if ( util::str_compare_ci( name_str, "astral_power" ) )
      return make_ref_expr( name_str, resources.current[ RESOURCE_ASTRAL_POWER ] );

    // New Moon stage related expressions
    if ( util::str_compare_ci( name_str, "new_moon" ) )
      return make_fn_expr( name_str, [ this ]() { return moon_stage == NEW_MOON; } );
    else if ( util::str_compare_ci( name_str, "half_moon" ) )
      return make_fn_expr( name_str, [ this ]() { return moon_stage == HALF_MOON; } );
    else if ( util::str_compare_ci( name_str, "full_moon" ) )
      return make_fn_expr( name_str, [ this ]() { return moon_stage >= FULL_MOON; } );

    // automatic resolution of Celestial Alignment vs talented Incarnation
    if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "ca_inc" ) )
    {
      if ( util::str_compare_ci( splits[ 0 ], "cooldown" ) )
        splits[ 1 ] = talent.incarnation_moonkin.ok() ? "incarnation" : "celestial_alignment";
      else
        splits[ 1 ] = talent.incarnation_moonkin.ok() ? "incarnation_chosen_of_elune" : "celestial_alignment";

      return druid_t::create_expression( util::string_join( splits, "." ) );
    }

    // check for AP overcap on action other than current action. check for current action handled in
    // druid_spell_t::create_expression syntax: <action>.ap_check.<allowed overcap = 0>
    if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "ap_check" ) )
    {
      action_t* action = find_action( splits[ 0 ] );
      if ( action )
      {
        int over = splits.size() > 2 ? util::to_int( splits[ 2 ] ) : 0;

        return make_fn_expr( name_str, [ this, action, over ]() { return check_astral_power( action, over ); } );
      }

      throw std::invalid_argument( "invalid action" );
    }

    if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "buff" ) && util::str_compare_ci( splits[ 1 ], "fury_of_elune" ) )
    {
      if ( util::str_compare_ci( splits[ 2 ], "stack" ) )
      {
        return make_fn_expr( name_str, [ this ]() {
          return buff.fury_of_elune->check() + buff.sundered_firmament->check();
        } );
      }
      else if ( util::str_compare_ci( splits[ 2 ], "remains" ) )
      {
        return make_fn_expr( name_str, [ this ]() {
          return std::max( buff.fury_of_elune->remains(), buff.sundered_firmament->remains() );
        } );
      }
    }
  }

  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "eclipse" ) )
  {
    if ( util::str_compare_ci( splits[ 1 ], "state" ) )
    {
      return make_fn_expr( name_str, [ this ]() {
        eclipse_state_e state = eclipse_handler.state;
        if ( state == ANY_NEXT )
          return 0;
        if ( eclipse_handler.state == IN_SOLAR )
          return 1;
        if ( eclipse_handler.state == IN_LUNAR )
          return 2;
        if ( eclipse_handler.state == IN_BOTH )
          return 3;
        return 4;
      } );
    }
    else if ( util::str_compare_ci( splits[ 1 ], "any_next" ) )
      return make_fn_expr( name_str, [ this ]() {
        return eclipse_handler.state == ANY_NEXT;
      } );
    else if ( util::str_compare_ci( splits[ 1 ], "in_any" ) )
      return make_fn_expr( name_str, [ this ]() {
        return eclipse_handler.state == IN_SOLAR || eclipse_handler.state == IN_LUNAR ||
               eclipse_handler.state == IN_BOTH;
      } );
    else if ( util::str_compare_ci( splits[ 1 ], "in_solar" ) )
      return make_fn_expr( name_str, [ this ]() {
        return eclipse_handler.state == IN_SOLAR;
      } );
    else if ( util::str_compare_ci( splits[ 1 ], "in_lunar" ) )
      return make_fn_expr( name_str, [ this ]() {
        return eclipse_handler.state == IN_LUNAR;
      } );
    else if ( util::str_compare_ci( splits[ 1 ], "in_both" ) )
      return make_fn_expr( name_str, [ this ]() {
        return eclipse_handler.state == IN_BOTH;
      } );
  }

  if ( util::str_compare_ci( name_str, "buff.starfall.refreshable" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      return !buff.starfall->check() || buff.starfall->remains() <= buff.starfall->buff_duration() * 0.3;
    } );
  }

  // Convert talent.incarnation.* & buff.incarnation.* to spec-based incarnations. cooldown.incarnation.* doesn't need
  // name conversion.
  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "incarnation" ) &&
       ( util::str_compare_ci( splits[ 0 ], "buff" ) || util::str_compare_ci( splits[ 0 ], "talent" ) ) )
  {
    switch ( specialization() )
    {
      case DRUID_BALANCE:     splits[ 1 ] = "incarnation_chosen_of_elune";    break;
      case DRUID_FERAL:       splits[ 1 ] = "incarnation_king_of_the_jungle"; break;
      case DRUID_GUARDIAN:    splits[ 1 ] = "incarnation_guardian_of_ursoc";  break;
      case DRUID_RESTORATION: splits[ 1 ] = "incarnation_tree_of_life";       break;
      default: break;
    }

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "berserk" ) )
  {
    if ( specialization() == DRUID_FERAL )
      splits[ 1 ] = "berserk_cat";
    else if ( specialization() == DRUID_GUARDIAN )
      splits[ 1 ] = "berserk_bear";
    else
      splits[ 1 ] = "berserk";

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  return player_t::create_expression( name_str );
}

static bool parse_swarm_setup( sim_t* sim, std::string_view, std::string_view setup_str )
{
  auto entries = util::string_split<std::string_view>( setup_str, "/" );

  for ( auto entry : entries )
  {
    auto values = util::string_split<std::string_view>( entry, ":" );

    try
    {
      if ( values.size() != 5 )
        throw std::invalid_argument( "Missing value." );

      auto min_stack = std::clamp( util::to_unsigned( values[ 0 ] ), 1U, 5U );
      auto max_stack = std::clamp( util::to_unsigned( values[ 1 ] ), 1U, 5U );
      auto min_dur = timespan_t::from_seconds( std::clamp( util::to_double( values[ 2 ] ), 0.0, 12.0 ) );
      auto max_dur = timespan_t::from_seconds( std::clamp( util::to_double( values[ 3 ] ), 0.0, 12.0 ) );
      auto chance = std::clamp( util::to_double( values[ 4 ] ), 0.0, 1.0 );

      debug_cast<druid_t*>( sim->active_player )
          ->prepull_swarm.emplace_back( min_stack, max_stack, min_dur, max_dur, chance );
    }
    catch ( const std::invalid_argument& msg )
    {
      throw std::invalid_argument(
          fmt::format( "\n\tInvalid entry '{}' for druid.adaptive_swarm_prepull_setup. {}"
                       "\n\tFormat is <min stacks>:<max stacks>:<min duration>:<max duration>:<chance>/...",
                       entry, msg.what() ) );
    }
  }

  return true;
}

void druid_t::create_options()
{
  player_t::create_options();

  // General
  add_option( opt_bool( "druid.affinity_resources", options.affinity_resources ) );
  add_option( opt_bool( "druid.no_cds", options.no_cds ) );
  add_option( opt_bool( "druid.raid_combat", options.raid_combat ) );
  add_option( opt_timespan( "druid.thorns_attack_period", options.thorns_attack_period ) );
  add_option( opt_float( "druid.thorns_hit_chance", options.thorns_hit_chance ) );

  add_option( opt_deprecated( "druid.adaptive_swarm_jump_distance_min", "druid.adaptive_swarm_jump_distance_melee" ) );
  add_option( opt_deprecated( "druid.adaptive_swarm_jump_distance_max", "druid.adaptive_swarm_jump_distance_ranged" ) );
  add_option( opt_float( "druid.adaptive_swarm_jump_distance_melee", options.adaptive_swarm_jump_distance_melee ) );
  add_option( opt_float( "druid.adaptive_swarm_jump_distance_ranged", options.adaptive_swarm_jump_distance_ranged ) );
  add_option( opt_float( "druid.adaptive_swarm_jump_distance_stddev", options.adaptive_swarm_jump_distance_stddev ) );
  add_option( opt_uint( "druid.adaptive_swarm_friendly_targets", options.adaptive_swarm_friendly_targets, 1U, 20U ) );
  add_option( opt_func( "druid.adaptive_swarm_prepull_setup", parse_swarm_setup ) );

  // Balance
  add_option( opt_float( "druid.initial_astral_power", options.initial_astral_power ) );
  add_option( opt_int( "druid.initial_moon_stage", options.initial_moon_stage ) );
  add_option( opt_float( "druid.initial_pulsar_value", options.initial_pulsar_value ) );
  add_option( opt_bool( "druid.delay_berserking", options.delay_berserking ) );

  // Feral
  add_option( opt_float( "druid.predator_rppm", options.predator_rppm_rate ) );
  add_option( opt_bool( "druid.owlweave_cat", options.owlweave_cat ) );

  // Guardian
  add_option( opt_bool( "druid.catweave_bear", options.catweave_bear ) );
  add_option( opt_bool( "druid.owlweave_bear", options.owlweave_bear ) );

  // Restoration
  add_option( opt_float( "druid.time_spend_healing", options.time_spend_healing ) );

  // Covenant
  add_option( opt_float( "druid.celestial_spirits_exceptional_chance", options.celestial_spirits_exceptional_chance ) );
  add_option( opt_int( "druid.convoke_the_spirits_deck", options.convoke_the_spirits_deck ) );
  add_option( opt_string( "druid.kindred_affinity_covenant", options.kindred_affinity_covenant ) );
  add_option( opt_float( "druid.kindred_spirits_absorbed", options.kindred_spirits_absorbed ) );
  add_option( opt_float( "druid.kindred_spirits_dps_pool_multiplier", options.kindred_spirits_dps_pool_multiplier ) );
  add_option( opt_float( "druid.kindred_spirits_non_dps_pool_multiplier", options.kindred_spirits_non_dps_pool_multiplier ) );
  add_option( opt_string( "druid.kindred_spirits_target", options.kindred_spirits_target_str ) );
  add_option( opt_bool( "druid.lone_empowerment", options.lone_empowerment ) );
}

std::string druid_t::create_profile( save_e type )
{
  std::string profile = player_t::create_profile( type );

  if ( type & SAVE_PLAYER )
  {
    if ( !options.adaptive_swarm_prepull_setup.empty() )
      profile += fmt::format( "druid.adaptive_swarm_prepull_setup={}\n", options.adaptive_swarm_prepull_setup );
  }

  return profile;
}

role_e druid_t::primary_role() const
{
  // First, check for the user-specified role
  switch ( player_t::primary_role() )
  {
    case ROLE_TANK:
    case ROLE_ATTACK:
    case ROLE_SPELL:
      return player_t::primary_role();
      break;
    default:
      break;
  }

  // Else, fall back to spec
  switch ( specialization() )
  {
    case DRUID_BALANCE:
      return ROLE_SPELL; break;
    case DRUID_GUARDIAN:
      return ROLE_TANK; break;
    default:
      return ROLE_ATTACK;
      break;
  }
}

stat_e druid_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work for certain specs into the appropriate
  // "basic" stats
  switch ( s )
  {
    case STAT_STR_AGI_INT:
      switch ( specialization() )
      {
        case DRUID_BALANCE:
        case DRUID_RESTORATION: return STAT_INTELLECT;
        case DRUID_FERAL:
        case DRUID_GUARDIAN: return STAT_AGILITY;
        default: return STAT_NONE;
      }
    case STAT_AGI_INT:
      if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
        return STAT_INTELLECT;
      else
        return STAT_AGILITY;
    case STAT_STR_AGI: return STAT_AGILITY;
    case STAT_STR_INT: return STAT_INTELLECT;
    case STAT_SPIRIT:
      if ( specialization() == DRUID_RESTORATION )
        return s;
      else
        return STAT_NONE;
    case STAT_BONUS_ARMOR:
      if ( specialization() == DRUID_GUARDIAN )
        return s;
      else
        return STAT_NONE;
    default: return s;
  }
}

resource_e druid_t::primary_resource() const
{
  if ( specialization() == DRUID_BALANCE )
    return RESOURCE_ASTRAL_POWER;

  if ( specialization() == DRUID_GUARDIAN )
    return RESOURCE_RAGE;

  if ( primary_role() == ROLE_HEAL || primary_role() == ROLE_SPELL )
    return RESOURCE_MANA;

  return RESOURCE_ENERGY;
}

void druid_t::init_absorb_priority()
{
  if ( talent.brambles.ok() )
  {
    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>( talent.brambles->id(),
      instant_absorb_t( this, talent.brambles, "brambles_absorb",
        &brambles_handler ) ) );
  }

  if ( talent.earthwarden.ok() )
  {
    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>( talent.earthwarden->id(),
      instant_absorb_t( this, &buff.earthwarden->data(), "earthwarden_absorb",
        &earthwarden_handler ) ) );
  }

  if ( talent.rage_of_the_sleeper.ok() )
  {
    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>( talent.rage_of_the_sleeper->id(),
      instant_absorb_t( this, talent.rage_of_the_sleeper, "rage_of_the_sleeper_absorb",
        &rage_of_the_sleeper_handler ) ) );
  }

  if ( cov.kindred_empowerment->ok() )
  {
    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>( cov.kindred_empowerment->id(),
      instant_absorb_t( this, cov.kindred_empowerment, "kindred_empowerment_absorb",
        &kindred_empowerment_handler ) ) );
  }

  player_t::init_absorb_priority();

  absorb_priority.push_back( talent.earthwarden->id() );
  absorb_priority.push_back( talent.rage_of_the_sleeper->id() );  // TODO confirm priority
  absorb_priority.push_back( talent.brambles->id() );
  absorb_priority.push_back( cov.kindred_empowerment->id() );
}

void druid_t::target_mitigation( school_e school, result_amount_type type, action_state_t* s )
{
  s->result_amount *= 1.0 + buff.barkskin->value();

  s->result_amount *= 1.0 + buff.survival_instincts->value();

  s->result_amount *= 1.0 + talent.thick_hide->effectN( 1 ).percent();

  if ( sets->has_set_bonus( DRUID_GUARDIAN, T29, B2 ) )
    s->result_amount *= 1.0 + buff.overpowering_aura->check_value();

  if ( talent.protective_growth.ok() )
    s->result_amount *= 1.0 + buff.protective_growth->value();

  if ( talent.pulverize.ok() )
    s->result_amount *= 1.0 + get_target_data( s->action->player )->debuff.pulverize->check_value();

  if ( talent.rend_and_tear.ok() )
  {
    s->result_amount *= 1.0 + talent.rend_and_tear->effectN( 3 ).percent() *
                                  get_target_data( s->action->player )->dots.thrash_bear->current_stack();
  }

  if ( talent.scintillating_moonlight.ok() && get_target_data( s->action->player )->dots.moonfire->is_ticking() )
    s->result_amount *= 1.0 + talent.scintillating_moonlight->effectN( 1 ).percent();

  if ( talent.tooth_and_claw.ok() )
    s->result_amount *= 1.0 + get_target_data( s->action->player )->debuff.tooth_and_claw->check_value();

  if ( talent.ursine_adept.ok() && buff.bear_form->check() )
    s->result_amount *= 1.0 + talent.ursine_adept->effectN( 2 ).percent();

  player_t::target_mitigation( school, type, s );
}

void druid_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && dtype == result_amount_type::DMG_DIRECT && s->result == RESULT_HIT )
    buff.ironfur->up();

  player_t::assess_damage( school, dtype, s );
}

// Trigger effects based on being hit or taking damage.
void druid_t::assess_damage_imminent_pre_absorb( school_e school, result_amount_type dmg, action_state_t* s )
{
  player_t::assess_damage_imminent_pre_absorb( school, dmg, s );

  if ( action_t::result_is_hit( s->result ) && s->result_amount > 0 )
  {
    // Guardian rage from melees
    if ( specialization() == DRUID_GUARDIAN && !s->action->special && cooldown.rage_from_melees->up() )
    {
      resource_gain( RESOURCE_RAGE, spec.bear_form_passive->effectN( 3 ).base_value(), gain.rage_from_melees );
      cooldown.rage_from_melees->start( cooldown.rage_from_melees->duration );
    }

    if ( buff.moonkin_form->check() )
      buff.owlkin_frenzy->trigger();

    buff.cenarion_ward->expire();

    if ( buff.bristling_fur->up() )  // 1 rage per 1% of maximum health taken
      resource_gain( RESOURCE_RAGE, s->result_amount / expected_max_health * 100, gain.bristling_fur );

    if ( talent.dream_of_cenarius.ok() && rng().roll( cache.attack_crit_chance() ) )
      buff.dream_of_cenarius->trigger();
  }
}

void druid_t::assess_heal( school_e school, result_amount_type dmg_type, action_state_t* s )
{
  player_t::assess_heal( school, dmg_type, s );

  trigger_natures_guardian( s );
}

void druid_t::shapeshift( form_e f )
{
  if ( get_form() == f )
    return;

  buff.bear_form->expire();
  buff.cat_form->expire();
  buff.moonkin_form->expire();

  switch ( f )
  {
    case BEAR_FORM:    buff.bear_form->trigger();    break;
    case CAT_FORM:     buff.cat_form->trigger();     break;
    case MOONKIN_FORM: buff.moonkin_form->trigger(); break;
    case NO_FORM:                                    break;
    default: assert( 0 ); break;
  }

  form = f;
}

// Target Data ==============================================================
druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_target_data_t( &target, &source ), dots(), hots(), debuff(), buff()
{
  dots.adaptive_swarm_damage = target.get_dot( "adaptive_swarm_damage", &source );
  dots.astral_smolder        = target.get_dot( "astral_smolder", &source );
  dots.feral_frenzy          = target.get_dot( "feral_frenzy_tick", &source );  // damage dot is triggered by feral_frenzy_tick_t
  dots.frenzied_assault      = target.get_dot( "frenzied_assault", &source );
  dots.fungal_growth         = target.get_dot( "fungal_growth", &source );
  dots.lunar_inspiration     = target.get_dot( "lunar_inspiration", &source );
  dots.moonfire              = target.get_dot( "moonfire", &source );
  dots.rake                  = target.get_dot( "rake", &source );
  dots.rip                   = target.get_dot( "rip", &source );
  dots.stellar_flare         = target.get_dot( "stellar_flare", &source );
  dots.sunfire               = target.get_dot( "sunfire", &source );
  dots.tear                  = target.get_dot( "tear", &source );
  dots.thrash_bear           = target.get_dot( "thrash_bear", &source );
  dots.thrash_cat            = target.get_dot( "thrash_cat", &source );

  hots.cenarion_ward         = target.get_dot( "cenarion_ward", &source );
  hots.cultivation           = target.get_dot( "cultivation", &source );
  hots.frenzied_regeneration = target.get_dot( "frenzied_regeneration", &source );
  hots.germination           = target.get_dot( "germination", &source );
  hots.lifebloom             = target.get_dot( "lifebloom", &source );
  hots.regrowth              = target.get_dot( "regrowth", &source );
  hots.rejuvenation          = target.get_dot( "rejuvenation", &source );
  hots.spring_blossoms       = target.get_dot( "spring_blossoms", &source );
  hots.wild_growth           = target.get_dot( "wild_growth", &source );

  // proxy debuffs for balance mastery
  debuff.moonfire = make_buff( *this, "moonfire_debuff" )->set_quiet( true );
  debuff.sunfire  = make_buff( *this, "sunfire_debuff" )->set_quiet( true );

  debuff.pulverize = make_buff( *this, "pulverize_debuff", source.talent.pulverize )
    ->set_cooldown( 0_ms )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_TO_CASTER );

  debuff.tooth_and_claw = make_buff( *this, "tooth_and_claw_debuff",
    source.talent.tooth_and_claw->effectN( 1 ).trigger()->effectN( 2 ).trigger() )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_TO_CASTER );

  buff.ironbark = make_buff( *this, "ironbark", source.talent.ironbark )
    ->set_cooldown( 0_ms );
}

int druid_td_t::dots_ticking() const
{
  auto count = dots.astral_smolder->is_ticking() +
               dots.fungal_growth->is_ticking() +
               dots.moonfire->is_ticking() +
               dots.rake->is_ticking() +
               dots.rip->is_ticking() +
               dots.stellar_flare->is_ticking() +
               dots.sunfire->is_ticking() +
               dots.thrash_bear->is_ticking() +
               dots.thrash_cat->is_ticking();

  // TODO: do these 'non-standard' dots also count?
  // count += dots.feral_frenzy->is_ticking();
  // count += dots.frenzied_assault->is_ticking();
  // count += dots.lunar_inspiration->is_ticking();

  return count;
}

int druid_td_t::hots_ticking() const
{
  auto count = hots.cenarion_ward->is_ticking() + hots.cultivation->is_ticking() +
               hots.frenzied_regeneration->is_ticking() + hots.germination->is_ticking() + hots.regrowth->is_ticking() +
               hots.rejuvenation->is_ticking() + hots.spring_blossoms->is_ticking() + hots.wild_growth->is_ticking();

  auto lb_mul = 1 + as<int>( debug_cast<druid_t*>( source )->talent.harmonious_blooming->effectN( 1 ).base_value() );
  count += lb_mul * hots.lifebloom->is_ticking();

  return count;
}

const druid_td_t* druid_t::find_target_data( const player_t* target ) const
{
  assert( target );
  return target_data[ target ];
}

druid_td_t* druid_t::get_target_data( player_t* target ) const
{
  assert( target );
  druid_td_t*& td = target_data[ target ];
  if ( !td )
    td = new druid_td_t( *target, const_cast<druid_t&>( *this ) );

  return td;
}

void druid_t::init_beast_weapon( weapon_t& w, double swing_time )
{
  // use main hand weapon as base
  w = main_hand_weapon;

  if ( w.type == WEAPON_NONE )
  {
    // if main hand weapon is empty, use unarmed damage unarmed base beast weapon damage range is 1-1 Jul 25 2018
    w.min_dmg = w.max_dmg = w.damage = 1;
  }
  else
  {
    // Otherwise normalize the main hand weapon's damage to the beast weapon's speed.
    double normalizing_factor = swing_time / w.swing_time.total_seconds();
    w.min_dmg *= normalizing_factor;
    w.max_dmg *= normalizing_factor;
    w.damage *= normalizing_factor;
  }

  w.type       = WEAPON_BEAST;
  w.school     = SCHOOL_PHYSICAL;
  w.swing_time = timespan_t::from_seconds( swing_time );
}

void druid_t::moving()
{
  if ( ( executing && !executing->usable_moving() ) || ( channeling && !channeling->usable_moving() ) )
    player_t::interrupt();
}

void druid_t::trigger_natures_guardian( const action_state_t* s )
{
  if ( !mastery.natures_guardian->ok() )
    return;

  // don't display amount from tank_heal & other raid events, as we're mainly interested in benefit from actions
  if ( s->action->id <= 0 )
    return;

  if ( s->result_total <= 0 )
    return;

  if ( s->action == active.natures_guardian || s->action == active.yseras_gift ||
       s->action->id == 22842 )  // Frenzied Regeneration
    return;

  active.natures_guardian->base_dd_min = active.natures_guardian->base_dd_max = s->result_total * cache.mastery_value();

  action_state_t* new_state = active.natures_guardian->get_state();
  new_state->target = this;

  active.natures_guardian->snapshot_state( new_state, result_amount_type::HEAL_DIRECT );
  active.natures_guardian->schedule_execute( new_state );
}

double druid_t::calculate_expected_max_health() const
{
  double slot_weights = 0;
  double prop_values  = 0;

  for ( const auto& item : items )
  {
    if ( item.slot == SLOT_SHIRT || item.slot == SLOT_RANGED || item.slot == SLOT_TABARD ||
         item.item_level() <= 0 )
      continue;

    const random_prop_data_t item_data = dbc->random_property( item.item_level() );
    int index                          = item_database::random_suffix_type( item.parsed.data );
    if ( item_data.p_epic[ 0 ] == 0 )
      continue;

    slot_weights += item_data.p_epic[ index ] / item_data.p_epic[ 0 ];

    if ( !item.active() )
      continue;

    prop_values += item_data.p_epic[ index ];
  }

  double expected_health = ( prop_values / slot_weights ) * 8.318556;
  expected_health += base.stats.attribute[ STAT_STAMINA ];
  expected_health *= 1.0 + matching_gear_multiplier( ATTR_STAMINA );
  expected_health *= 1.0 + spec.bear_form_passive->effectN( 2 ).percent();
  expected_health *= current.health_per_stamina;

  return expected_health;
}

// Utility function to search spell data for matching effect.
// NOTE: This will return the FIRST effect that matches parameters.
const spelleffect_data_t* druid_t::query_aura_effect( const spell_data_t* aura_spell, effect_subtype_t subtype,
                                                      int misc_value, const spell_data_t* affected_spell,
                                                      effect_type_t type ) const
{
  for ( size_t i = 1; i <= aura_spell->effect_count(); i++ )
  {
    const spelleffect_data_t& effect = aura_spell->effectN( i );

    if ( affected_spell != spell_data_t::nil() && !affected_spell->affected_by_all( effect ) )
      continue;

    if ( effect.type() == type && effect.subtype() == subtype )
    {
      if ( misc_value != 0 )
      {
        if ( effect.misc_value1() == misc_value )
          return &effect;
      }
      else
        return &effect;
    }
  }

  return &spelleffect_data_t::nil();
}

const spell_data_t* druid_t::apply_override( const spell_data_t* base, const spell_data_t* passive )
{
  if ( !passive->ok() )
    return base;

  return find_spell( as<unsigned>( query_aura_effect( passive, A_OVERRIDE_ACTION_SPELL, 0, base )->base_value() ) );
}

// Eclipse Handler ==========================================================
void eclipse_handler_t::init()
{
  if ( !p->talent.eclipse.ok() )
    return;

  enabled_ = true;

  wrath_counter_base = starfire_counter_base = as<unsigned>( p->talent.eclipse->effectN( 1 ).base_value() );

  size_t res = 4;
  bool foe = p->talent.fury_of_elune.ok();
  bool nm = p->talent.new_moon.ok();
  bool hm = nm;
  bool fm = nm || p->talent.convoke_the_spirits.ok();

  data.arrays.reserve( res + foe + nm + hm + fm );
  data.wrath = &data.arrays.emplace_back( data_array() );
  data.starfire = &data.arrays.emplace_back( data_array() );
  data.starsurge = &data.arrays.emplace_back( data_array() );
  data.starfall = &data.arrays.emplace_back( data_array() );
  if ( foe )
    data.fury_of_elune = &data.arrays.emplace_back( data_array() );
  if ( nm )
    data.new_moon = &data.arrays.emplace_back( data_array() );
  if ( hm )
    data.half_moon = &data.arrays.emplace_back( data_array() );
  if ( fm )
    data.full_moon = &data.arrays.emplace_back( data_array() );

  iter.arrays.reserve( res + foe + nm + hm + fm );
  iter.wrath = &iter.arrays.emplace_back( iter_array() );
  iter.starfire = &iter.arrays.emplace_back( iter_array() );
  iter.starsurge = &iter.arrays.emplace_back( iter_array() );
  iter.starfall = &iter.arrays.emplace_back( iter_array() );
  if ( foe )
    iter.fury_of_elune = &iter.arrays.emplace_back( iter_array() );
  if ( nm )
    iter.new_moon = &iter.arrays.emplace_back( iter_array() );
  if ( hm )
    iter.half_moon = &iter.arrays.emplace_back( iter_array() );
  if ( fm )
    iter.full_moon = &iter.arrays.emplace_back( iter_array() );
}

void eclipse_handler_t::cast_wrath()
{
  if ( !enabled() ) return;

  if ( iter.wrath && p->in_combat )
    ( *iter.wrath )[ state ]++;

  if ( state == ANY_NEXT )
  {
    wrath_counter--;
    advance_eclipse();
  }
}

void eclipse_handler_t::cast_starfire()
{
  if ( !enabled() ) return;

  if ( iter.starfire && p->in_combat )
    ( *iter.starfire )[ state ]++;

  if ( state == ANY_NEXT )
  {
    starfire_counter--;
    advance_eclipse();
  }
}

void eclipse_handler_t::cast_starsurge()
{
  if ( !enabled() ) return;

  if ( iter.starsurge && p->in_combat )
    ( *iter.starsurge )[ state ]++;
}

void eclipse_handler_t::cast_ca_inc() {}

void eclipse_handler_t::cast_moon( moon_stage_e moon )
{
  if ( moon == moon_stage_e::NEW_MOON && iter.new_moon )
    ( *iter.new_moon )[ state ]++;
  else if ( moon == moon_stage_e::HALF_MOON && iter.half_moon )
    ( *iter.half_moon )[ state ]++;
  else if ( moon == moon_stage_e::FULL_MOON && iter.full_moon )
    ( *iter.full_moon )[ state ]++;
}

void eclipse_handler_t::tick_starfall()
{
  if ( iter.starfall )
    ( *iter.starfall )[ state ]++;
}

void eclipse_handler_t::tick_fury_of_elune()
{
  if ( iter.fury_of_elune )
    ( *iter.fury_of_elune )[ state ]++;
}

void eclipse_handler_t::advance_eclipse()
{
  switch ( state )
  {
    case ANY_NEXT:
      if ( !starfire_counter )
      {
        p->buff.eclipse_solar->trigger();

        if ( p->talent.solstice.ok() )
          p->buff.solstice->trigger();

        if ( p->talent.balance_of_all_things.ok() )
          p->buff.balance_of_all_things_nature->trigger();

        state = IN_SOLAR;
        p->uptime.eclipse_none->update( false, p->sim->current_time() );
        p->uptime.eclipse_solar->update( true, p->sim->current_time() );
        reset_stacks();
      }
      else if ( !wrath_counter )
      {
        p->buff.eclipse_lunar->trigger();

        if ( p->talent.solstice.ok() )
          p->buff.solstice->trigger();

        if ( p->legendary.balance_of_all_things.ok() )
          p->buff.balance_of_all_things_arcane->trigger();

        state = IN_LUNAR;
        p->uptime.eclipse_none->update( false, p->sim->current_time() );
        p->uptime.eclipse_lunar->update( true, p->sim->current_time() );
        reset_stacks();
      }
      break;

    case IN_SOLAR:
      p->uptime.eclipse_solar->update( false, p->sim->current_time() );
      p->uptime.eclipse_none->update( true, p->sim->current_time() );
      state = ANY_NEXT;
      break;

    case IN_LUNAR:
      p->uptime.eclipse_lunar->update( false, p->sim->current_time() );
      p->uptime.eclipse_none->update( true, p->sim->current_time() );
      state = ANY_NEXT;
      break;

    case IN_BOTH:
      p->uptime.eclipse_solar->update( false, p->sim->current_time() );
      p->uptime.eclipse_lunar->update( false, p->sim->current_time() );
      p->uptime.eclipse_none->update( true, p->sim->current_time() );
      state = ANY_NEXT;
      break;

    default:
      assert( true && "eclipse_handler_t::advance_eclipse() at unknown state" );
      break;
  }
}

void eclipse_handler_t::trigger_both( timespan_t d = 0_ms )
{
  if ( p->talent.balance_of_all_things.ok() )
  {
    p->buff.balance_of_all_things_arcane->trigger();
    p->buff.balance_of_all_things_nature->trigger();
  }

  if ( p->talent.solstice.ok() )
    p->buff.solstice->trigger();

  p->buff.eclipse_lunar->trigger( d );
  p->buff.eclipse_solar->trigger( d );

  state = IN_BOTH;
  p->uptime.eclipse_none->update( false, p->sim->current_time() );
  p->uptime.eclipse_lunar->update( false, p->sim->current_time() );
  p->uptime.eclipse_solar->update( false, p->sim->current_time() );
  reset_stacks();
}

void eclipse_handler_t::extend_both( timespan_t d )
{
  p->buff.eclipse_solar->extend_duration( p, d );
  p->buff.eclipse_lunar->extend_duration( p, d );
}

void eclipse_handler_t::expire_both()
{
  p->buff.eclipse_solar->expire();
  p->buff.eclipse_lunar->expire();
}

void eclipse_handler_t::reset_stacks()
{
  wrath_counter    = wrath_counter_base;
  starfire_counter = wrath_counter_base;
}

void eclipse_handler_t::reset_state()
{
  state = ANY_NEXT;
}

void eclipse_handler_t::datacollection_begin()
{
  if ( !enabled() || p->specialization() != DRUID_BALANCE )
    return;

  iter.wrath->fill( 0 );
  iter.starfire->fill( 0 );
  iter.starsurge->fill( 0 );
  iter.starfall->fill( 0 );
  if ( iter.fury_of_elune )
    iter.fury_of_elune->fill( 0 );
  if ( iter.new_moon )
    iter.new_moon->fill( 0 );
  if ( iter.half_moon )
    iter.half_moon->fill( 0 );
  if ( iter.full_moon )
    iter.full_moon->fill( 0 );
}

void eclipse_handler_t::datacollection_end()
{
  if ( !enabled() || p->specialization() != DRUID_BALANCE )
    return;

  auto end = []( auto& from, auto& to ) {
    static_assert( std::tuple_size_v<std::remove_reference_t<decltype( from )>> <
                       std::tuple_size_v<std::remove_reference_t<decltype( to )>>,
                   "array size mismatch" );

    size_t i = 0;

    for ( ; i < from.size(); i++ )
      to[ i ] += from[ i ];

    to[ i ]++;
  };

  end( *iter.wrath, *data.wrath );
  end( *iter.starfire, *data.starfire );
  end( *iter.starsurge, *data.starsurge );
  end( *iter.starfall, *data.starfall );
  if ( iter.fury_of_elune )
    end( *iter.fury_of_elune, *data.fury_of_elune );
  if ( iter.new_moon )
    end( *iter.new_moon, *data.new_moon );
  if ( iter.half_moon )
    end( *iter.half_moon, *data.half_moon );
  if ( iter.full_moon )
    end( *iter.full_moon, *data.full_moon );
}

void eclipse_handler_t::merge( const eclipse_handler_t& other )
{
  if ( !enabled() || p->specialization() != DRUID_BALANCE )
    return;

  auto merge = []( auto& from, auto& to ) {
    static_assert( std::tuple_size_v<std::remove_reference_t<decltype( from )>> ==
                       std::tuple_size_v<std::remove_reference_t<decltype( to )>>,
                   "array size mismatch" );

    for ( size_t i = 0; i < from.size(); i++ )
      to[ i ] += from[ i ];
  };

  merge( *other.data.wrath, *data.wrath );
  merge( *other.data.starfire, *data.starfire );
  merge( *other.data.starsurge, *data.starsurge );
  merge( *other.data.starfall, *data.starfall );
  if ( data.fury_of_elune )
    merge( *other.data.fury_of_elune, *data.fury_of_elune );
  if ( data.new_moon )
    merge( *other.data.new_moon, *data.new_moon );
  if ( data.half_moon )
    merge( *other.data.half_moon, *data.half_moon );
  if ( data.full_moon )
    merge( *other.data.full_moon, *data.full_moon );
}

void druid_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  options = debug_cast<druid_t*>( source )->options;
  prepull_swarm = debug_cast<druid_t*>( source )->prepull_swarm;
}

void druid_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  // Spec-wide auras
  action.apply_affecting_aura( spec.druid );
  action.apply_affecting_aura( spec.balance );
  action.apply_affecting_aura( spec.feral );
  action.apply_affecting_aura( spec.guardian );
  action.apply_affecting_aura( spec.restoration );

  // Rank spells
  action.apply_affecting_aura( spec.moonfire_2 );

  // Class
  action.apply_affecting_aura( talent.astral_influence );
  action.apply_affecting_aura( talent.improved_rejuvenation );
  action.apply_affecting_aura( talent.improved_stampeding_roar );
  action.apply_affecting_aura( talent.killer_instinct );
  action.apply_affecting_aura( talent.nurturing_instinct );

  // Multi-spec
  action.apply_affecting_aura( talent.circle_of_life_and_death );

  // Balance
  action.apply_affecting_aura( talent.elunes_guidance );
  action.apply_affecting_aura( talent.radiant_moonlight );
  action.apply_affecting_aura( talent.twin_moons );
  
  // Feral
  action.apply_affecting_aura( spec.ashamanes_guidance );
  action.apply_affecting_aura( talent.dreadful_bleeding );
  action.apply_affecting_aura( talent.infected_wounds_cat );
  action.apply_affecting_aura( talent.lions_strength );
  action.apply_affecting_aura( talent.relentless_predator );
  action.apply_affecting_aura( talent.veinripper );
  action.apply_affecting_aura( talent.wild_slashes );
  action.apply_affecting_aura( sets->set( DRUID_FERAL, T29, B2 ) );

  // Guardian
  action.apply_affecting_aura( talent.improved_survival_instincts );
  action.apply_affecting_aura( talent.innate_resolve );
  action.apply_affecting_aura( talent.reinvigoration );
  action.apply_affecting_aura( talent.soul_of_the_forest_bear );
  action.apply_affecting_aura( talent.survival_of_the_fittest );
  action.apply_affecting_aura( talent.twin_moonfire );
  action.apply_affecting_aura( talent.untamed_savagery );
  action.apply_affecting_aura( talent.ursocs_guidance );
  action.apply_affecting_aura( talent.vulnerable_flesh );
  action.apply_affecting_aura( sets->set( DRUID_GUARDIAN, T29, B4 ) );

  // Restoration
  action.apply_affecting_aura( talent.germination );
  action.apply_affecting_aura( talent.improved_ironbark );
  action.apply_affecting_aura( talent.inner_peace );
  action.apply_affecting_aura( talent.natural_recovery );
  action.apply_affecting_aura( talent.passing_seasons );
  action.apply_affecting_aura( talent.rampant_growth );
  action.apply_affecting_aura( talent.sabertooth );
  action.apply_affecting_aura( talent.soul_of_the_forest_cat );

  // Legendaries
  action.apply_affecting_aura( legendary.celestial_spirits );
  action.apply_affecting_aura( legendary.legacy_of_the_sleeper );
  action.apply_affecting_aura( legendary.luffainfused_embrace );

  // Conduits
  action.apply_affecting_conduit( conduit.innate_resolve );
  action.apply_affecting_conduit( conduit.tough_as_bark );
}

// check for AP overcap on current action. syntax: ap_check.<allowed overcap = 0>
bool druid_t::check_astral_power( action_t* a, int over )
{
  double ap = resources.current[ RESOURCE_ASTRAL_POWER ];

  auto aps = buff.natures_balance->check_value() +
             buff.fury_of_elune->check_value() +
             buff.sundered_firmament->check_value();

  ap += a->composite_energize_amount( nullptr );
  ap += a->time_to_execute.total_seconds() * aps;
  ap += spec.shooting_stars_dmg->effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );

  return ap <= resources.max[ RESOURCE_ASTRAL_POWER ] + over;
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
private:
  struct feral_counter_data_t
  {
    action_t* action = nullptr;
    double tf_exe = 0.0;
    double tf_tick = 0.0;
    double bt_exe = 0.0;
    double bt_tick = 0.0;
  };

public:
  druid_report_t( druid_t& player ) : p( player ) {}

  void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.specialization() == DRUID_FERAL )
      feral_snapshot_table( os );

    if ( p.specialization() == DRUID_BALANCE && p.eclipse_handler.enabled() )
      balance_eclipse_table( os );
  }

  void balance_print_data( report::sc_html_stream& os, const spell_data_t* spell,
                           const eclipse_handler_t::data_array& data )
  {
    double iter  = data[ eclipse_state_e::MAX_STATE ];
    double none  = data[ eclipse_state_e::ANY_NEXT ];
    double solar = data[ eclipse_state_e::IN_SOLAR ];
    double lunar = data[ eclipse_state_e::IN_LUNAR ];
    double both  = data[ eclipse_state_e::IN_BOTH ];
    double total = none + solar + lunar + both;

    if ( !total )
      return;

    os.format( "<tr><td class=\"left\">{}</td>"
               "<td class=\"right\">{:.2f}</td><td class=\"right\">{:.1f}%</td>"
               "<td class=\"right\">{:.2f}</td><td class=\"right\">{:.1f}%</td>"
               "<td class=\"right\">{:.2f}</td><td class=\"right\">{:.1f}%</td>"
               "<td class=\"right\">{:.2f}</td><td class=\"right\">{:.1f}%</td></tr>",
               report_decorators::decorated_spell_data( p.sim, spell ),
               none / iter, none / total * 100,
               solar / iter, solar / total * 100,
               lunar / iter, lunar / total * 100,
               both / iter, both / total * 100 );
  }

  void balance_eclipse_table( report::sc_html_stream& os )
  {
    os << "<div class=\"player-section custom_section\">\n"
       << "<h3 class=\"toggle open\">Eclipse Utilization</h3>\n"
       << "<div class=\"toggle-content\">\n"
       << "<table class=\"sc even\">\n"
       << "<thead><tr><th></th>\n"
       << "<th colspan=\"2\">None</th><th colspan=\"2\">Solar</th><th colspan=\"2\">Lunar</th><th colspan=\"2\">Both</th>\n"
       << "</tr></thead>\n";

    balance_print_data( os, p.spec.wrath, *p.eclipse_handler.data.wrath );
    balance_print_data( os, p.talent.starfire, *p.eclipse_handler.data.starfire );
    balance_print_data( os, p.talent.starsurge, *p.eclipse_handler.data.starsurge );
    balance_print_data( os, p.talent.starfall, *p.eclipse_handler.data.starfall );
    if ( p.eclipse_handler.data.fury_of_elune )
      balance_print_data( os, p.find_spell( 202770 ), *p.eclipse_handler.data.fury_of_elune );
    if ( p.eclipse_handler.data.new_moon )
      balance_print_data( os, p.find_spell( 274281 ), *p.eclipse_handler.data.new_moon );
    if ( p.eclipse_handler.data.half_moon )
      balance_print_data( os, p.find_spell( 274282 ), *p.eclipse_handler.data.half_moon );
    if ( p.eclipse_handler.data.full_moon )
      balance_print_data( os, p.find_spell( 274283 ), *p.eclipse_handler.data.full_moon );

    os << "</table>\n"
       << "</div>\n"
       << "</div>\n";
  }

  void feral_parse_counter( snapshot_counter_t* counter, feral_counter_data_t& data )
  {
    if ( range::contains( counter->buffs, p.buff.tigers_fury ) )
    {
      data.tf_exe += counter->execute.mean();

      if ( counter->tick.count() )
        data.tf_tick += counter->tick.mean();
      else
        data.tf_tick += counter->execute.mean();
    }
    else if ( range::contains( counter->buffs, p.buff.bloodtalons ) )
    {
      data.bt_exe += counter->execute.mean();

      if ( counter->tick.count() )
        data.bt_tick += counter->tick.mean();
      else
        data.bt_tick += counter->execute.mean();
    }
  }

  void feral_snapshot_table( report::sc_html_stream& os )
  {
    // Write header
    os << "<div class=\"player-section custom_section\">\n"
       << "<h3 class=\"toggle open\">Snapshot Table</h3>\n"
       << "<div class=\"toggle-content\">\n"
       << "<table class=\"sc sort even\">\n"
       << "<thead><tr><th></th>\n"
       << "<th colspan=\"2\">Tiger's Fury</th>\n";

    if ( p.talent.bloodtalons.ok() )
    {
      os << "<th colspan=\"2\">Bloodtalons</th>\n";
    }
    os << "</tr>\n";

    os << "<tr>\n"
       << "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability Name</th>\n"
       << "<th class=\"toggle-sort\">Execute %</th>\n"
       << "<th class=\"toggle-sort\">Benefit %</th>\n";

    if ( p.talent.bloodtalons.ok() )
    {
      os << "<th class=\"toggle-sort\">Execute %</th>\n"
         << "<th class=\"toggle-sort\">Benefit %</th>\n";
    }
    os << "</tr></thead>\n";

    std::vector<feral_counter_data_t> data_list;

    // Compile and Write Contents
    for ( size_t i = 0; i < p.counters.size(); i++ )
    {
      auto counter = p.counters[ i ].get();
      feral_counter_data_t data;

      for ( const auto& a : counter->stats->action_list )
      {
        if ( a->s_data->ok() )
        {
          data.action = a;
          break;
        }
      }

      if ( !data.action )
        data.action = counter->stats->action_list.front();

      // We can change the action's reporting name here since we shouldn't need to access it again later in the report
      std::string suf = "_convoke";
      if ( suf.size() <= data.action->name_str.size() &&
           std::equal( suf.rbegin(), suf.rend(), data.action->name_str.rbegin() ) )
      {
        data.action->name_str_reporting += "Convoke";
      }

      feral_parse_counter( counter, data );

      // since the BT counter is created immediately following the TF counter for the same stat in
      // cat_attacks_t::init(), check the next counter and add in the data if it's for the same stat
      if ( i + 1 < p.counters.size() )
      {
        auto next_counter = p.counters[ i + 1 ].get();

        if ( counter->stats == next_counter->stats )
        {
          feral_parse_counter( next_counter, data );
          i++;
        }
      }

      if ( data.tf_exe + data.tf_tick + data.bt_exe + data.bt_tick == 0.0 )
        continue;

      data_list.push_back( std::move( data ) );
    }

    range::sort( data_list, []( const feral_counter_data_t& l, const feral_counter_data_t& r ) {
      return l.action->name_str < r.action->name_str;
    } );

    for ( const auto& data : data_list )
    {
      os.format( "<tr><td class=\"left\">{}</td><td class=\"right\">{:.2f}%</td><td class=\"right\">{:.2f}%</td>",
                 report_decorators::decorated_action( *data.action ), data.tf_exe * 100, data.tf_tick * 100 );

      if ( p.talent.bloodtalons.ok() )
        os.format( "<td class=\"right\">{:.2f}%</td><td class=\"right\">{:.2f}</td>", data.bt_exe * 100, data.bt_tick * 100 );

      os << "</tr>\n";
    }

    // Write footer
    os << "</table>\n"
       << "</div>\n"
       << "</div>\n";
  }

private:
  druid_t& p;
};

// ==========================================================================
// Druid Special Effects
// ==========================================================================

using namespace unique_gear;
using namespace spells;
using namespace cat_attacks;
using namespace bear_attacks;
using namespace heals;
using namespace buffs;

// General

// Feral

// Guardian

// Druid Special Effects ====================================================
#if 0
static void init_special_effect( druid_t*                 player,
                                 specialization_e         spec,
                                 const special_effect_t*& ptr,
                                 const special_effect_t&  effect )
{
  /* Ensure we have the spell data. This will prevent the trinket effect from
     working on live Simulationcraft. Also ensure correct specialization. */
  if ( ! player -> find_spell( effect.spell_id ) -> ok() ||
       ( player -> specialization() != spec && spec != SPEC_NONE ) )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}
#endif

// DRUID MODULE INTERFACE ===================================================
struct druid_module_t : public module_t
{
  druid_module_t() : module_t( DRUID ) {}

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new druid_t( sim, name, r );
    p->report_extension = std::make_unique<druid_report_t>( *p );
    return p;
  }
  bool valid() const override { return true; }

  void init( player_t* p ) const override
  {
    p->buffs.stampeding_roar = make_buff( p, "stampeding_roar", p->find_spell( 106898 ) )
      ->set_cooldown( 0_ms )
      ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );

    if ( p->covenant && p->covenant->enabled() )
      p->buffs.kindred_affinity = make_buff<buffs::kindred_affinity_external_buff_t>( p );
  }

  void static_init() const override {}

  void register_hotfixes() const override
  {
    hotfix::register_spell( "Druid", "", "Adjust bear thrash periodic damage spell level requirement", 192090 )
      .field( "spell_level" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 11 )
      .verification_value( 18 ) ;
  }

  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};
}  // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}

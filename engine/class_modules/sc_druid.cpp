// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"
#include "simulationcraft.hpp"
#include "action/parse_buff_effects.hpp"
#include "player/covenant.hpp"
#include "player/pet_spawner.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"

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

enum form_e : unsigned
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
  DISABLED,
  ANY_NEXT,
  IN_SOLAR,
  IN_LUNAR,
  IN_BOTH,
  MAX_STATE
};

enum free_spell_e : unsigned
{
  NONE       = 0x0000,
  // free procs
  CONVOKE    = 0x0001,  // convoke_the_spirits night_fae covenant ability
  FIRMAMENT  = 0x0002,  // sundered firmament talent
  FLASHING   = 0x0004,  // flashing claws talent
  GALACTIC   = 0x0008,  // galactic guardian talent
  ORBIT      = 0x0040,  // orbit breaker talent
  // free casts
  APEX       = 0x0080,  // apex predators's craving
  STARWEAVER = 0x0100,  // starweaver talent
  TOOTH      = 0x0200,  // tooth and claw talent

  PROCS = CONVOKE | FIRMAMENT | FLASHING | GALACTIC | ORBIT,
  CASTS = APEX | STARWEAVER | TOOTH
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
    dot_t* adaptive_swarm_heal;
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
    buff_t* dire_fixation;
    buff_t* pulverize;
    buff_t* tooth_and_claw;
    buff_t* waning_twilight;
  } debuff;

  struct buffs_t
  {
    buff_t* ironbark;
  } buff;

  druid_td_t( player_t& target, druid_t& source );

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
  bool is_snapped = false;

  snapshot_counter_t( player_t* p, stats_t* s, buff_t* b )
    : execute(), tick(), waste(), sim( p->sim ), stats( s )
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
  unsigned wrath_counter = 2;
  unsigned wrath_counter_base = 2;
  unsigned starfire_counter = 2;
  unsigned starfire_counter_base = 2;
  eclipse_state_e state = eclipse_state_e::DISABLED;

  eclipse_handler_t( druid_t* player ) : data(), iter(), p( player ) {}

  bool enabled() { return state != eclipse_state_e::DISABLED; }
  void init();

  void cast_wrath();
  void cast_starfire();
  void cast_starsurge();
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
    *static_cast<Data*>( this ) = *static_cast<const Data*>( static_cast<const druid_action_state_t*>( o ) );
  }
};

// Static helper functions
template <typename V>
static const spell_data_t* resolve_spell_data( V data )
{
  if constexpr( std::is_invocable_v<decltype( &spell_data_t::ok ), V> )
    return data;
  else if constexpr( std::is_invocable_v<decltype( &buff_t::data ), V> )
    return &data->data();
  else if constexpr( std::is_invocable_v<decltype( &action_t::data ), V> )
    return &data->data();

  assert( false && "Could not resolve find_effect argument to spell data." );
  return nullptr;
}

// finds a spell effect
// 1) first argument can be either player_talent_t, spell_data_t*, buff_t*, action_t*
// 2) if the second argument is player_talent_t, spell_data_t*, buff_t*, or action_t* then only effects that affect it are returned
// 3) if the third (or second if the above does not apply) argument is effect subtype, then the type is assumed to be E_APPLY_AURA
// further arguments can be given to filter for full type + subtype + property
template <typename T, typename U, typename... Ts>
static const spelleffect_data_t& find_effect( T val, U type, Ts&&... args )
{
  const spell_data_t* data = resolve_spell_data<T>( val );

  if constexpr( std::is_same_v<U, effect_subtype_t> )
    return spell_data_t::find_spelleffect( *data, E_APPLY_AURA, type, std::forward<Ts>( args )... );
  else if constexpr( std::is_same_v<U, effect_type_t> )
    return spell_data_t::find_spelleffect( *data, type, std::forward<Ts>( args )... );
  else
  {
    const spell_data_t* affected = resolve_spell_data<U>( type );
 
    if constexpr( std::is_same_v<std::tuple_element_t<0, std::tuple<Ts...>>, effect_subtype_t> )
      return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA, std::forward<Ts>( args )... );
    else if constexpr( std::is_same_v<std::tuple_element_t<0, std::tuple<Ts...>>, effect_type_t> )
      return spell_data_t::find_spelleffect( *data, *affected, std::forward<Ts>( args )... );
   else
     return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA );
  }

  assert( false && "Could not resolve find_effect argument to type/subtype." );
  return spelleffect_data_t::nil();
}

template <typename T, typename U, typename... Ts>
static size_t find_effect_index( T val, U type, Ts&&... args )
{
  return find_effect( val, type, std::forward<Ts>( args )... ).index() + 1;
}

// finds the first effect with a trigger spell
// argument can be either player_talent_t, spell_data_t*, buff_t*, action_t*
template <typename T>
static const spelleffect_data_t& find_trigger( T val )
{
  const spell_data_t* data = resolve_spell_data<T>( val );

  for ( const auto& eff : data->effects() )
  {
    switch ( eff.type() )
    {
      case E_TRIGGER_SPELL:
      case E_TRIGGER_SPELL_WITH_VALUE:
        return eff;
      case E_APPLY_AURA:
      case E_APPLY_AREA_AURA_PARTY:
        switch( eff.subtype() )
        {
          case A_PROC_TRIGGER_SPELL:
          case A_PROC_TRIGGER_SPELL_WITH_VALUE:
          case A_PERIODIC_TRIGGER_SPELL:
          case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
            return eff;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }

  return spelleffect_data_t::nil();
}

struct druid_t : public player_t
{
private:
  form_e form = form_e::NO_FORM;  // Active druid form
public:
  eclipse_handler_t eclipse_handler;
  // counters for snapshot tracking
  std::vector<std::unique_ptr<snapshot_counter_t>> counters;
  // Tier 30 stacks tracking
  std::unique_ptr<extended_sample_data_t> predator_revealed_stacks;
  double expected_max_health;  // For Bristling Fur calculations.
  std::vector<std::tuple<unsigned, unsigned, timespan_t, timespan_t, double>> prepull_swarm;
  std::vector<player_t*> swarm_targets;

  // !!!==========================================================================!!!
  // !!! Runtime variables NOTE: these MUST be properly reset in druid_t::reset() !!!
  // !!!==========================================================================!!!
  moon_stage_e moon_stage;
  double cache_mastery_snapshot;  // for balance mastery snapshot
  std::vector<event_t*> persistent_event_delay;
  event_t* astral_power_decay;
  struct dot_list_t
  {
    std::vector<dot_t*> moonfire;
    std::vector<dot_t*> sunfire;
  } dot_list;
  player_t* dire_fixation_target;
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
    unsigned adaptive_swarm_melee_targets = 7;
    unsigned adaptive_swarm_ranged_targets = 12;
    std::string adaptive_swarm_prepull_setup = "";
    int convoke_the_spirits_deck = 5;
    double cenarius_guidance_exceptional_chance = 0.85;

    // Balance
    double initial_astral_power = 0.0;
    int initial_moon_stage = static_cast<int>( moon_stage_e::NEW_MOON );
    double initial_pulsar_value = 0.0;
    int initial_orbit_breaker_stacks = -1;

    // Feral
    double predator_rppm_rate = 0.0;
    bool owlweave_cat = true;

    // Guardian
    bool catweave_bear = false;
    bool owlweave_bear = false;

    // Restoration
    double time_spend_healing = 0.0;
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
    real_ppm_t* predator;  // Optional RPPM approximation
  } rppm;

  struct active_actions_t
  {
    // General
    action_t* shift_to_caster;
    action_t* shift_to_bear;
    action_t* shift_to_cat;
    action_t* shift_to_moonkin;

    // Balance
    action_t* astral_smolder;
    action_t* denizen_of_the_dream;          // placeholder action
    action_t* orbit_breaker;
    action_t* shooting_stars;                // placeholder action
    action_t* shooting_stars_moonfire;
    action_t* shooting_stars_sunfire;
    action_t* crashing_stars;                // 4t30
    action_t* starfall_starweaver;           // free starfall from starweaver's warp
    action_t* starsurge_starweaver;          // free starsurge from starweaver's weft
    action_t* sundered_firmament;
    action_t* orbital_strike;

    // Feral
    action_t* ferocious_bite_apex;           // free bite from apex predator's crazing
    action_t* frenzied_assault;
    action_t* thrashing_claws;

    // Guardian
    action_t* after_the_wildfire_heal;
    action_t* brambles;
    action_t* elunes_favored_heal;
    action_t* furious_regeneration;
    action_t* galactic_guardian;
    action_t* maul_tooth_and_claw;
    action_t* raze_tooth_and_claw;
    action_t* moonless_night;
    action_t* natures_guardian;
    action_t* rage_of_the_sleeper_reflect;
    action_t* thorns_of_iron;
    action_t* thrash_bear_flashing;

    // Restoration
    action_t* yseras_gift;
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
  melee_attack_t* caster_melee_attack = nullptr;
  melee_attack_t* cat_melee_attack = nullptr;
  melee_attack_t* bear_melee_attack = nullptr;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* barkskin;
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* prowl;
    buff_t* thorns;

    // Class
    buff_t* dash;
    buff_t* forestwalk;
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
    buff_t* protector_of_the_pack;
    buff_t* rising_light_falling_night_day;
    buff_t* rising_light_falling_night_night;
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
    buff_t* denizen_of_the_dream;  // proxy buff to track stack uptime
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
    buff_t* shooting_stars_moonfire;
    buff_t* shooting_stars_sunfire;
    buff_t* shooting_stars_stellar_flare;
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
    buff_t* bt_feral_frenzy;  // dummy buff
    buff_t* clearcasting_cat;
    buff_t* frantic_momentum;
    buff_t* incarnation_cat;
    buff_t* incarnation_cat_prowl;
    buff_t* overflowing_power;
    buff_t* predator_revealed;  // 4t30
    buff_t* predatory_swiftness;
    buff_t* protective_growth;
    buff_t* sabertooth;
    buff_t* shadows_of_the_predator;  // 2t30
    buff_t* sharpened_claws;  // 4t29
    buff_t* sudden_ambush;
    buff_t* tigers_fury;
    buff_t* tigers_tenacity;

    // Guardian
    buff_t* after_the_wildfire;
    buff_t* berserk_bear;
    buff_t* bristling_fur;
    buff_t* dream_of_cenarius;
    buff_t* earthwarden;
    buff_t* elunes_favored;
    buff_t* furious_regeneration;  // 2t30
    buff_t* galactic_guardian;
    buff_t* gore;
    buff_t* gory_fur;
    buff_t* guardian_of_elune;
    buff_t* incarnation_bear;
    buff_t* indomitable_guardian;  // 4t30
    buff_t* lunar_beam;
    buff_t* overpowering_aura;  // 2t29
    buff_t* rage_of_the_sleeper;
    buff_t* tooth_and_claw;
    buff_t* ursocs_fury;
    buff_t* vicious_cycle_mangle;
    buff_t* vicious_cycle_maul;

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
    cooldown_t* frenzied_regeneration;
    cooldown_t* incarnation_bear;
    cooldown_t* incarnation_cat;
    cooldown_t* incarnation_moonkin;
    cooldown_t* incarnation_tree;
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
    gain_t* cats_curiosity;
    gain_t* energy_refund;
    gain_t* incessant_hunter;
    gain_t* overflowing_power;
    gain_t* primal_claws;  // TODO remove in 10.1.5
    gain_t* primal_fury;
    gain_t* tigers_tenacity;

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
    proc_t* denizen_of_the_dream;
    proc_t* pulsar;

    // Feral
    proc_t* ashamanes_guidance;
    proc_t* clearcasting;
    proc_t* clearcasting_wasted;
    proc_t* predator;
    proc_t* predator_wasted;
    proc_t* primal_claws;  // TODO: remove in 10.1.5
    proc_t* primal_fury;

    // Guardian
    proc_t* galactic_guardian;
    proc_t* gore;
    proc_t* tooth_and_claw;
  } proc;

  // Talents
  struct talents_t
  {
    // Class tree
    player_talent_t astral_influence;
    player_talent_t cyclone;
    player_talent_t feline_swiftness;
    player_talent_t forestwalk;
    player_talent_t frenzied_regeneration;
    player_talent_t gale_winds;
    player_talent_t heart_of_the_wild;
    player_talent_t hibernate;
    player_talent_t improved_barkskin;
    player_talent_t improved_rejuvenation;
    player_talent_t improved_stampeding_roar;
    player_talent_t improved_sunfire;
    player_talent_t improved_swipe;
    player_talent_t incapacitating_roar;
    player_talent_t incessant_tempest;
    player_talent_t innervate;
    player_talent_t ironfur;
    player_talent_t killer_instinct;
    player_talent_t lycaras_teachings;
    player_talent_t maim;
    player_talent_t matted_fur;
    player_talent_t mass_entanglement;
    player_talent_t mighty_bash;
    player_talent_t moonkin_form;
    player_talent_t natural_recovery;
    player_talent_t natures_vigil;
    player_talent_t nurturing_instinct;
    player_talent_t primal_fury;
    player_talent_t protector_of_the_pack;
    player_talent_t rake;
    player_talent_t rejuvenation;
    player_talent_t remove_corruption;
    player_talent_t renewal;
    player_talent_t rising_light_falling_night;
    player_talent_t rip;
    player_talent_t skull_bash;
    player_talent_t soothe;
    player_talent_t stampeding_roar;
    player_talent_t starfire;
    player_talent_t starsurge;
    player_talent_t sunfire;
    player_talent_t swiftmend;
    player_talent_t thick_hide;
    player_talent_t thrash;
    player_talent_t tiger_dash;
    player_talent_t tireless_pursuit;
    player_talent_t typhoon;
    player_talent_t ursine_vigor;
    player_talent_t ursols_vortex;
    player_talent_t verdant_heart;
    player_talent_t wellhoned_instincts;
    player_talent_t wild_charge;
    player_talent_t wild_growth;

    // Multi-spec
    player_talent_t adaptive_swarm;
    player_talent_t unbridled_swarm;
    player_talent_t circle_of_life_and_death;
    player_talent_t convoke_the_spirits;
    player_talent_t survival_instincts;

    // Balance
    player_talent_t aetherial_kindling;
    player_talent_t astral_communion;
    player_talent_t astral_smolder;
    player_talent_t balance_of_all_things;
    player_talent_t celestial_alignment;
    player_talent_t cosmic_rapidity;
    player_talent_t denizen_of_the_dream;
    player_talent_t eclipse;
    player_talent_t elunes_guidance;
    player_talent_t force_of_nature;
    player_talent_t friend_of_the_fae;
    player_talent_t fungal_growth;
    player_talent_t fury_of_elune;
    player_talent_t incarnation_moonkin;
    player_talent_t light_of_the_sun;
    player_talent_t lunar_shrapnel;
    player_talent_t natures_balance;
    player_talent_t natures_grace;
    player_talent_t new_moon;
    player_talent_t orbit_breaker;
    player_talent_t orbital_strike;
    player_talent_t power_of_goldrinn;
    player_talent_t primordial_arcanic_pulsar;
    player_talent_t radiant_moonlight;
    player_talent_t rattle_the_stars;
    player_talent_t shooting_stars;
    player_talent_t solar_beam;
    player_talent_t solstice;
    player_talent_t soul_of_the_forest_moonkin;
    player_talent_t starfall;
    player_talent_t starlord;
    player_talent_t starweaver;
    player_talent_t stellar_flare;
    player_talent_t stellar_innervation;
    player_talent_t sundered_firmament;
    player_talent_t twin_moons;
    player_talent_t umbral_embrace;
    player_talent_t umbral_intensity;
    player_talent_t waning_twilight;
    player_talent_t warrior_of_elune;
    player_talent_t wild_mushroom;
    player_talent_t wild_surges;

    // Feral
    player_talent_t apex_predators_craving;
    player_talent_t ashamanes_guidance;
    player_talent_t berserk;
    player_talent_t berserk_frenzy;
    player_talent_t berserk_heart_of_the_lion;
    player_talent_t bloodtalons;
    player_talent_t brutal_slash;
    player_talent_t carnivorous_instinct;
    player_talent_t cats_curiosity;
    player_talent_t dire_fixation;
    player_talent_t doubleclawed_rake;
    player_talent_t dreadful_bleeding;
    player_talent_t feral_frenzy;
    player_talent_t frantic_momentum;
    player_talent_t incarnation_cat;
    player_talent_t infected_wounds_cat;
    player_talent_t lions_strength;
    player_talent_t lunar_inspiration;
    player_talent_t merciless_claws;
    player_talent_t moment_of_clarity;
    player_talent_t omen_of_clarity_cat;
    player_talent_t pouncing_strikes;
    player_talent_t predator;
    player_talent_t predatory_swiftness;
    player_talent_t primal_claws;  // TODO: remove in 10.1.5
    player_talent_t primal_wrath;
    player_talent_t protective_growth;
    player_talent_t raging_fury;
    player_talent_t rampant_ferocity;
    player_talent_t relentless_predator;
    player_talent_t rip_and_tear;
    player_talent_t sabertooth;
    player_talent_t soul_of_the_forest_cat;
    player_talent_t sudden_ambush;
    player_talent_t taste_for_blood;
    player_talent_t tear_open_wounds;
    player_talent_t thrashing_claws;
    player_talent_t tigers_fury;
    player_talent_t tigers_tenacity;
    player_talent_t tireless_energy;
    player_talent_t veinripper;
    player_talent_t wild_slashes;

    // Guardian
    player_talent_t after_the_wildfire;
    player_talent_t berserk_persistence;
    player_talent_t berserk_ravage;
    player_talent_t berserk_unchecked_aggression;
    player_talent_t blood_frenzy;
    player_talent_t brambles;
    player_talent_t bristling_fur;
    player_talent_t dream_of_cenarius;
    player_talent_t earthwarden;
    player_talent_t elunes_favored;
    player_talent_t flashing_claws;
    player_talent_t front_of_the_pack;  // TODO: remove in 10.1.5
    player_talent_t fury_of_nature;
    player_talent_t galactic_guardian;
    player_talent_t gore;
    player_talent_t gory_fur;
    player_talent_t guardian_of_elune;
    player_talent_t improved_survival_instincts;
    player_talent_t incarnation_bear;
    player_talent_t infected_wounds_bear;
    player_talent_t innate_resolve;
    player_talent_t layered_mane;
    player_talent_t lunar_beam;
    player_talent_t mangle;
    player_talent_t maul;
    player_talent_t moonless_night;
    player_talent_t pulverize;
    player_talent_t rage_of_the_sleeper;
    player_talent_t raze;
    player_talent_t reinforced_fur;
    player_talent_t reinvigoration;
    player_talent_t rend_and_tear;
    player_talent_t scintillating_moonlight;
    player_talent_t soul_of_the_forest_bear;
    player_talent_t survival_of_the_fittest;
    player_talent_t thorns_of_iron;
    player_talent_t tooth_and_claw;
    player_talent_t twin_moonfire;
    player_talent_t ursocs_endurance;
    player_talent_t ursocs_fury;
    player_talent_t vicious_cycle;
    player_talent_t vulnerable_flesh;
    player_talent_t untamed_savagery;
    player_talent_t ursocs_guidance;

    // Restoration
    player_talent_t abundance;
    player_talent_t budding_leaves;
    player_talent_t cenarion_ward;
    player_talent_t cenarius_guidance;
    player_talent_t cultivation;
    player_talent_t deep_focus;
    player_talent_t dreamstate;
    player_talent_t efflorescence;
    player_talent_t embrace_of_the_dream;
    player_talent_t flash_of_clarity;
    player_talent_t flourish;
    player_talent_t germination;
    player_talent_t grove_tending;
    player_talent_t harmonious_blooming;
    player_talent_t improved_ironbark;
    player_talent_t improved_regrowth;
    player_talent_t improved_wild_growth;
    player_talent_t incarnation_tree;
    player_talent_t inner_peace;
    player_talent_t invigorate;
    player_talent_t ironbark;
    player_talent_t lifebloom;
    player_talent_t luxuriant_soil;
    player_talent_t natures_splendor;
    player_talent_t natures_swiftness;
    player_talent_t nourish;
    player_talent_t nurturing_dormancy;
    player_talent_t omen_of_clarity_tree;
    player_talent_t overgrowth;
    player_talent_t rampant_growth;
    player_talent_t reforestation;
    player_talent_t regenerative_heartwood;
    player_talent_t regenesis;
    player_talent_t passing_seasons;
    player_talent_t photosynthesis;
    player_talent_t power_of_the_archdruid;
    player_talent_t soul_of_the_forest_tree;
    player_talent_t spring_blossoms;
    player_talent_t stonebark;
    player_talent_t tranquility;
    player_talent_t tranquil_mind;
    player_talent_t undergrowth;
    player_talent_t unstoppable_growth;
    player_talent_t verdancy;
    player_talent_t verdant_infusion;
    player_talent_t waking_dream;
    player_talent_t yseras_gift;
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
    const spell_data_t* improved_prowl;  // stealth rake
    const spell_data_t* moonfire_2;
    const spell_data_t* moonfire_dmg;
    const spell_data_t* swipe;
    const spell_data_t* wrath;

    // Class
    const spell_data_t* moonfire;
    const spell_data_t* sunfire_dmg;
    const spell_data_t* thrash_bear_dot;
    const spell_data_t* thrash_cat_dot;

    // Multi-Spec
    const spell_data_t* adaptive_swarm_damage;
    const spell_data_t* adaptive_swarm_heal;

    // Balance
    const spell_data_t* astral_communion;
    const spell_data_t* balance;
    const spell_data_t* astral_power;
    const spell_data_t* celestial_alignment;
    const spell_data_t* eclipse_lunar;
    const spell_data_t* eclipse_solar;
    const spell_data_t* full_moon;
    const spell_data_t* half_moon;
    const spell_data_t* incarnation_moonkin;
    const spell_data_t* shooting_stars_dmg;
    const spell_data_t* waning_twilight;
    const spell_data_t* starfall;

    // Feral
    const spell_data_t* feral;
    const spell_data_t* feral_overrides;
    const spell_data_t* ashamanes_guidance;
    const spell_data_t* berserk_cat;  // berserk cast/buff spell

    // Guardian
    const spell_data_t* guardian;
    const spell_data_t* bear_form_2;
    const spell_data_t* berserk_bear;  // berserk cast/buff spell
    const spell_data_t* elunes_favored;
    const spell_data_t* furious_regeneration;  // 2t30
    const spell_data_t* incarnation_bear;
    const spell_data_t* lightning_reflexes;
    const spell_data_t* ursine_adept;

    // Resto
    const spell_data_t* restoration;
    const spell_data_t* cenarius_guidance;
  } spec;

  struct uptimes_t
  {
    uptime_t* astral_smolder;
    uptime_t* combined_ca_inc;
    uptime_t* eclipse_solar;
    uptime_t* eclipse_lunar;
    uptime_t* eclipse_none;
    uptime_t* friend_of_the_fae;
    uptime_t* incarnation_cat;
    uptime_t* primordial_arcanic_pulsar;
    uptime_t* tooth_and_claw_debuff;
  } uptime;

  druid_t( sim_t* sim, std::string_view name, race_e r = RACE_NIGHT_ELF )
    : player_t( sim, DRUID, name, r ),
      eclipse_handler( this ),
      options(),
      spec_override(),
      active(),
      pets( this ),
      caster_form_weapon(),
      buff(),
      cooldown(),
      gain(),
      mastery(),
      proc(),
      talent(),
      spec(),
      uptime()
  {
    cooldown.berserk_bear          = get_cooldown( "berserk_bear" );
    cooldown.berserk_cat           = get_cooldown( "berserk_cat" );
    cooldown.frenzied_regeneration = get_cooldown( "frenzied_regeneration" );
    cooldown.incarnation_bear      = get_cooldown( "incarnation_guardian_of_ursoc" );
    cooldown.incarnation_cat       = get_cooldown( "incarnation_avatar_of_ashamane" );
    cooldown.incarnation_moonkin   = get_cooldown( "incarnation_chosen_of_elune" );
    cooldown.incarnation_tree      = get_cooldown( "incarnation_tree_of_life" );
    cooldown.mangle                = get_cooldown( "mangle" );
    cooldown.moon_cd               = get_cooldown( "moon_cd" );
    cooldown.natures_swiftness     = get_cooldown( "natures_swiftness" );
    cooldown.thrash_bear           = get_cooldown( "thrash_bear" );
    cooldown.tigers_fury           = get_cooldown( "tigers_fury" );
    cooldown.warrior_of_elune      = get_cooldown( "warrior_of_elune" );
    cooldown.rage_from_melees      = get_cooldown( "rage_from_melees" );
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
  std::unique_ptr<expr_t> create_action_expression(action_t& a, std::string_view name_str) override;
  std::unique_ptr<expr_t> create_expression( std::string_view name ) override;
  action_t* create_action( std::string_view name, std::string_view options ) override;
  pet_t* create_pet( std::string_view name, std::string_view type ) override;
  void create_pets() override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double resource_regen_per_second( resource_e ) const override;
  double resource_gain( resource_e, double, gain_t*, action_t* a = nullptr ) override;
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
  const spell_data_t* apply_override( const spell_data_t* base, const spell_data_t* passive );
  void apply_affecting_auras( action_t& ) override;
  bool check_astral_power( action_t* a, int over );
  double cache_mastery_value() const;  // for balance mastery
  void snapshot_mastery();

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
    if ( auto matched = dynamic_cast<T*>( a ) )
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
  void apl_balance_ptr();
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
    druid_t* o;

    fey_missile_t( pet_t* p )
      : spell_t( "fey_missile", p, p->find_spell( 188046 ) ), o( static_cast<druid_t*>( p->owner ) )
    {
      name_str_reporting = "fey_missile";

      auto proxy = o->active.denizen_of_the_dream;
      auto it = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }

    void execute() override
    {
      // TODO: has server batching behavior, using a random value for now
      cooldown->duration = rng().range( 0_ms, 600_ms );

      spell_t::execute();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto da = spell_t::composite_da_multiplier( s );

      da *= 1.0 + o->buff.eclipse_lunar->check_value();
      da *= 1.0 + o->buff.eclipse_solar->check_value();

      return da;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      auto tm = spell_t::composite_target_multiplier( t );
      auto td = o->get_target_data( t );

      if ( td->dots.moonfire->is_ticking() )
        tm *= 1.0 + o->cache_mastery_value();

      if ( td->dots.sunfire->is_ticking() )
        tm *= 1.0 + o->cache_mastery_value();

      return tm;
    }
  };

  denizen_of_the_dream_t( druid_t* p ) : pet_t( p->sim, p, "denizen_of_the_dream", true, true )
  {
    owner_coeff.sp_from_sp = 1.0;

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

    o()->buff.friend_of_the_fae->trigger();
  }

  druid_t* o() { return static_cast<druid_t*>( owner ); }
};

// Force of Nature ==================================================
struct force_of_nature_t : public pet_t
{
  struct fon_melee_t : public melee_attack_t
  {
    bool first_attack = true;

    fon_melee_t( pet_t* pet, const char* name = "Melee" ) : melee_attack_t( name, pet, spell_data_t::nil() )
    {
      school            = SCHOOL_PHYSICAL;
      weapon            = &( pet->main_hand_weapon );
      weapon_multiplier = 1.0;
      base_execute_time = weapon->swing_time;
      may_crit = background = repeating = true;
    }

    timespan_t execute_time() const override
    {
      return first_attack ? 0_ms : melee_attack_t::execute_time();
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
    auto_attack_t( pet_t* pet ) : melee_attack_t( "auto_attack", pet )
    {
      assert( pet->main_hand_weapon.type != WEAPON_NONE );
      pet->main_hand_attack = new fon_melee_t( pet );
      trigger_gcd = 0_ms;
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
    double base_dps = 3706;  // @70

    switch ( o()->true_level )
    {
      case 70: break;
      case 69: base_dps = 3447; break;
      case 68: base_dps = 3143; break;
      case 67: base_dps = 2774; break;
      case 66: base_dps = 2341; break;
      case 65: base_dps = 1973; break;
      case 64: base_dps = 1660; break;
      case 63: base_dps = 1394; break;
      case 62: base_dps = 1169; break;
      case 61: base_dps = 977;  break;
      default: base_dps = 523;  break;
    }

    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = base_dps * main_hand_weapon.swing_time.total_seconds() / 1000;

    resource_regeneration = regen_type::DISABLED;
    main_hand_weapon.type = WEAPON_BEAST;

    action_list_str = "auto_attack";
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    // TODO: confirm these values
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
    if ( name == "auto_attack" ) return new auto_attack_t( this );

    return pet_t::create_action( name, options_str );
  }
};
}  // end namespace pets

namespace buffs
{
template <typename Base = buff_t>
struct druid_buff_base_t : public Base
{
  static_assert( std::is_base_of_v<buff_t, Base> );

protected:
  using base_t = druid_buff_base_t<Base>;

public:
  druid_buff_base_t( druid_t* p, std::string_view name, const spell_data_t* s = spell_data_t::nil(),
                     const item_t* item = nullptr )
    : Base( p, name, s, item )
  {}

  druid_buff_base_t( druid_td_t& td, std::string_view name, const spell_data_t* s = spell_data_t::nil(),
                     const item_t* item = nullptr )
    : Base( td, name, s, item )
  {}

  druid_t* p() { return static_cast<druid_t*>( Base::source ); }

  const druid_t* p() const { return static_cast<druid_t*>( Base::source ); }
};

using druid_buff_t = druid_buff_base_t<>;

// Shapeshift Form Buffs ====================================================

struct swap_melee_t
{
private:
  druid_t* p_;

public:
  swap_melee_t( druid_t* p ) : p_( p ) {}

  // Used when shapeshifting to switch to a new attack & schedule it to occur
  // when the current swing timer would have ended.
  void swap_melee( attack_t* new_attack, weapon_t& new_weapon )
  {
    if ( p_->main_hand_attack && p_->main_hand_attack->execute_event )
    {
      new_attack->base_execute_time = new_weapon.swing_time;
      new_attack->execute_event =
          new_attack->start_action_execute_event( p_->main_hand_attack->execute_event->remains() );
      p_->main_hand_attack->cancel();
    }
    new_attack->weapon = &new_weapon;
    p_->main_hand_attack = new_attack;
    p_->main_hand_weapon = new_weapon;
  }
};

// Bear Form ================================================================
struct bear_form_buff_t : public druid_buff_t, public swap_melee_t
{
  double rage_gain;

  bear_form_buff_t( druid_t* p )
    : base_t( p, "bear_form", p->find_class_spell( "Bear Form" ) ),
      swap_melee_t( p ),
      rage_gain( find_effect( p->find_spell( 17057 ), E_ENERGIZE ).resource( RESOURCE_RAGE ) )
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

    swap_melee( p()->caster_melee_attack, p()->caster_form_weapon );

    p()->resource_loss( RESOURCE_RAGE, p()->resources.current[ RESOURCE_RAGE ] );
    p()->recalculate_resource_max( RESOURCE_HEALTH );

    p()->buff.ironfur->expire();
    p()->buff.rage_of_the_sleeper->expire();
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    swap_melee( p()->bear_melee_attack, p()->bear_weapon );

    base_t::start( stacks, value, duration );

    p()->resource_gain( RESOURCE_RAGE, rage_gain, p()->gain.bear_form );
    p()->recalculate_resource_max( RESOURCE_HEALTH );
  }
};

// Cat Form =================================================================
struct cat_form_buff_t : public druid_buff_t, public swap_melee_t
{
  cat_form_buff_t( druid_t* p ) : base_t( p, "cat_form", p->find_class_spell( "Cat Form" ) ), swap_melee_t( p )
  {
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( p()->caster_melee_attack, p()->caster_form_weapon );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    swap_melee( p()->cat_melee_attack, p()->cat_weapon );

    base_t::start( stacks, value, duration );
  }
};

// Moonkin Form =============================================================
struct moonkin_form_buff_t : public druid_buff_t
{
  moonkin_form_buff_t( druid_t* p ) : base_t( p, "moonkin_form", p->talent.moonkin_form )
  {
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
  }
};

// Druid Buffs ==============================================================

// Berserk (Guardian) / Incarn Buff =========================================
struct berserk_bear_buff_t : public druid_buff_t
{
  double hp_mul = 1.0;
  bool inc;

  berserk_bear_buff_t( druid_t* p, std::string_view n, const spell_data_t* s, bool b = false )
    : base_t( p, n, s ), inc( b )
  {
    set_cooldown( 0_ms );
    set_refresh_behavior( buff_refresh_behavior::EXTEND );

    if ( !inc && p->specialization() == DRUID_GUARDIAN )
      name_str_reporting = "berserk";

    if ( p->talent.berserk_unchecked_aggression.ok() )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      apply_affecting_aura( p->talent.berserk_unchecked_aggression );
      set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    }

    if ( inc )
      hp_mul += find_effect( this, A_MOD_INCREASE_HEALTH_PERCENT ).percent();
  }

  void start( int s, double v, timespan_t d ) override
  {
    base_t::start( s, v, d );

    if ( inc )
    {
      p()->resources.max[ RESOURCE_HEALTH ] *= hp_mul;
      p()->resources.current[ RESOURCE_HEALTH ] *= hp_mul;
      p()->recalculate_resource_max( RESOURCE_HEALTH );
    }
  }

  void expire_override( int s, timespan_t d ) override
  {
    if ( inc )
    {
      p()->resources.max[ RESOURCE_HEALTH ] /= hp_mul;
      p()->resources.current[ RESOURCE_HEALTH ] /= hp_mul;
      p()->recalculate_resource_max( RESOURCE_HEALTH );
    }

    base_t::expire_override( s, d );
  }
};

// Berserk (Feral) / Incarn Buff ============================================
struct berserk_cat_buff_t : public druid_buff_t
{
  bool inc;

  berserk_cat_buff_t( druid_t* p, std::string_view n, const spell_data_t* s, bool b = false )
    : base_t( p, n, s ), inc( b )
  {
    set_cooldown( 0_ms );
    if ( !inc && p->specialization() == DRUID_FERAL )
      name_str_reporting = "berserk";

    if ( inc )
      set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST );

    auto cp = find_effect( find_trigger( s ).trigger(), E_ENERGIZE ).resource( RESOURCE_COMBO_POINT );
    auto gain = p->get_gain( n );

    set_tick_callback( [ cp, gain, this ]( buff_t*, int, timespan_t ) {
      player->resource_gain( RESOURCE_COMBO_POINT, cp, gain );
    } );
  }

  void start( int s, double v, timespan_t d ) override
  {
    base_t::start( s, v, d );

    if ( inc )
      p()->uptime.incarnation_cat->update( true, sim->current_time() );
  }

  void expire_override( int s, timespan_t d ) override
  {
    base_t::expire_override( s, d );

    p()->gain.overflowing_power->overflow[ RESOURCE_COMBO_POINT ]+= p()->buff.overflowing_power->check();
    p()->buff.overflowing_power->expire();

    if ( inc )
      p()->uptime.incarnation_cat->update( false, sim->current_time() );
  }
};

// Bloodtalons Tracking Buff ================================================
struct bt_dummy_buff_t : public druid_buff_t
{
  int count;

  bt_dummy_buff_t( druid_t* p, std::string_view n )
    : base_t( p, n ), count( as<int>( p->talent.bloodtalons->effectN( 2 ).base_value() ) )
  {
    // The counting starts from the end of the triggering ability gcd.
    set_duration( timespan_t::from_seconds( p->talent.bloodtalons->effectN( 1 ).base_value() ) );
    set_quiet( true );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    set_activated( false );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    if ( !p()->talent.bloodtalons.ok() )
      return false;

    if ( ( p()->buff.bt_rake->check() + p()->buff.bt_shred->check() + p()->buff.bt_swipe->check() +
           p()->buff.bt_thrash->check() + p()->buff.bt_moonfire->check() + p()->buff.bt_brutal_slash->check() +
           p()->buff.bt_feral_frenzy->check() + !this->check() ) < count )
    {
      return base_t::trigger( s, v, c, d );
    }

    p()->buff.bt_rake->expire();
    p()->buff.bt_shred->expire();
    p()->buff.bt_swipe->expire();
    p()->buff.bt_thrash->expire();
    p()->buff.bt_moonfire->expire();
    p()->buff.bt_brutal_slash->expire();
    p()->buff.bt_feral_frenzy->expire();

    p()->buff.bloodtalons->trigger();

    return true;
  }
};

// Celestial Alignment / Incarn Buff ========================================
struct celestial_alignment_buff_t : public druid_buff_t
{
  celestial_alignment_buff_t( druid_t* p, std::string_view n, const spell_data_t* s )
    : base_t( p, n, p->apply_override( s, p->talent.orbital_strike ) )
  {
    set_cooldown( 0_ms );

    if ( p->talent.celestial_alignment.ok() )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    }
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    bool ret = base_t::trigger( s, v, c, d );

    if ( ret )
    {
      p()->eclipse_handler.trigger_both( remains() );
      p()->uptime.combined_ca_inc->update( true, sim->current_time() );

      if ( p()->active.orbital_strike )
        p()->active.orbital_strike->execute_on_target( p()->target );
    }

    return ret;
  }

  void extend_duration( player_t* player, timespan_t d ) override
  {
    base_t::extend_duration( player, d );

    p()->eclipse_handler.extend_both( d );
  }

  void expire_override( int s, timespan_t d ) override
  {
    base_t::expire_override( s, d );

    p()->eclipse_handler.expire_both();
    p()->uptime.primordial_arcanic_pulsar->update( false, sim->current_time() );
    p()->uptime.combined_ca_inc->update( false, sim->current_time() );
  }

};

// Fury of Elune AP =========================================================
struct fury_of_elune_buff_t : public druid_buff_t
{
  fury_of_elune_buff_t( druid_t* p, std::string_view n, const spell_data_t* s ) : base_t( p, n, s )
  {
    set_cooldown( 0_ms );
    set_refresh_behavior( buff_refresh_behavior::DURATION );

    const auto& eff = find_effect( this, A_PERIODIC_ENERGIZE, RESOURCE_ASTRAL_POWER );
    auto ap = eff.resource( RESOURCE_ASTRAL_POWER );
    set_default_value( ap / eff.period().total_seconds() );

    auto gain = p->get_gain( n );
    set_tick_callback( [ ap, gain, this ]( buff_t*, int, timespan_t ) {
      player->resource_gain( RESOURCE_ASTRAL_POWER, ap, gain );
    } );
  }
};

// Matted Fur ================================================================
struct matted_fur_buff_t : public druid_buff_base_t<absorb_buff_t>
{
  double mul;

  matted_fur_buff_t( druid_t* p ) : base_t( p, "matted_fur", find_trigger( p->talent.matted_fur ).trigger() )
  {
    mul = find_trigger( p->talent.matted_fur ).percent() * 1.25;
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    v = p()->composite_melee_attack_power() * mul * ( 1.0 + p()->composite_heal_versatility() );

    return base_t::trigger( s, v, c, d );
  }
};

// Protector of the Pack =====================================================
struct protector_of_the_pack_buff_t : public druid_buff_t
{
  double mul;
  double cap_coeff;

  protector_of_the_pack_buff_t( druid_t* p, std::string_view n, const spell_data_t* s )
    : base_t( p, n, s ),
      mul( p->talent.protector_of_the_pack->effectN( 1 ).percent() ),
      cap_coeff( p->specialization() == DRUID_RESTORATION ? 3.0 : 5.0 )
  {
    set_trigger_spell( p->talent.protector_of_the_pack );
    set_name_reporting( "protector_of_the_pack" );
  }

  void add_amount( double amt )
  {
    auto cap = p()->cache.spell_power( SCHOOL_MAX ) * cap_coeff;

    if ( current_value >= cap )
      return;

    current_value = std::min( cap, current_value + amt * mul );
  }
};

// Shadows of the Predator (Tier 30 2pc) =====================================
struct shadows_of_the_predator_buff_t : public druid_buff_t
{
  int cutoff;
  int reset;

  shadows_of_the_predator_buff_t( druid_t* p, const spell_data_t* s )
    : base_t( p, "shadows_of_the_predator", find_trigger( s ).trigger() ),
      cutoff( as<int>( s->effectN( 2 ).base_value() ) ),
      reset( as<int>( s->effectN( 3 ).base_value() ) )
  {
    set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE );
    set_pct_buff_type( STAT_PCT_BUFF_AGILITY );
  }

  void increment( int s, double v, timespan_t d ) override
  {
    if ( current_stack >= cutoff && rng().roll( 0.125 * ( current_stack - ( cutoff - 1 ) ) ) )
    {
      p()->predator_revealed_stacks->add( current_stack );

      base_t::decrement( current_stack - reset, v );

      // as we are using decrement, manually reschedule buff expiration
      expiration.front()->reschedule( refresh_duration( buff_duration() ) );

      p()->buff.predator_revealed->trigger();
    }
    else
    {
      base_t::increment( s, v, d );
    }
  }
};

// Shooting Stars ============================================================
struct shooting_stars_buff_t : public druid_buff_t
{
  std::vector<dot_t*>& dot_list;
  action_t*& damage;
  double base_chance;
  double crash_chance;

  shooting_stars_buff_t( druid_t* p, std::string_view n, std::vector<dot_t*>& d, action_t*& a )
    : base_t( p, n, p->talent.shooting_stars ),
      dot_list( d ),
      damage( a ),
      base_chance( find_effect( p->talent.shooting_stars, A_PERIODIC_DUMMY ).percent() ),
      crash_chance( p->sets->set( DRUID_BALANCE, T30, B4 )->effectN( 1 ).percent() )
  {
    set_quiet( true );
    set_tick_zero( true );
    apply_affecting_aura( p->talent.cosmic_rapidity );
    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { trigger_shooting_stars(); } );
  }

  void trigger_shooting_stars()
  {
    auto n = dot_list.size();
    if ( !n )
      return;

    double c = base_chance;
    c *= 1.0 + p()->buff.solstice->value();

    if ( n > 1 )
    {
      c *= std::sqrt( n );
      rng().shuffle( dot_list.begin(), dot_list.end() );
    }

    double procs;
    auto f = std::modf( c, &procs );
    if ( rng().roll( f ) )
      procs++;

    assert( procs <= n );

    for ( size_t i = 0; i < procs; i++ )
    {
      if ( rng().roll( crash_chance ) )
        p()->active.crashing_stars->execute_on_target( dot_list[ i ]->target );
      else
        damage->execute_on_target( dot_list[ i ]->target );
    }
  }
};

// Ursine Vigor =============================================================
struct ursine_vigor_buff_t : public druid_buff_t
{
  double hp_mul = 1.0;

  ursine_vigor_buff_t( druid_t* p )
    : base_t( p, "ursine_vigor", find_trigger( p->talent.ursine_vigor ).trigger() )
  {
    set_default_value( find_trigger( p->talent.ursine_vigor ).percent() );
    add_invalidate( CACHE_ARMOR );

    hp_mul += default_value;
  }

  void start( int s, double v, timespan_t d ) override
  {
    base_t::start( s, v, d );

    p()->resources.max[ RESOURCE_HEALTH ] *= hp_mul;
    p()->resources.current[ RESOURCE_HEALTH ] *= hp_mul;
    p()->recalculate_resource_max( RESOURCE_HEALTH );
  }

  void expire_override( int s, timespan_t d ) override
  {
    p()->resources.max[ RESOURCE_HEALTH ] /= hp_mul;
    p()->resources.current[ RESOURCE_HEALTH ] /= hp_mul;
    p()->recalculate_resource_max( RESOURCE_HEALTH );

    base_t::expire_override( s, d );
  }
};

// External Buffs (not derived from druid_buff_t) ==========================
}  // end namespace buffs

// Template for common druid action code. See priest_action_t.
template <class Base>
struct druid_action_t : public Base, public parse_buff_effects_t<druid_td_t>
{
private:
  using ab = Base;  // action base, eg. spell_t

public:
  using base_t = druid_action_t<Base>;

  // list of action_ids that triggers the same dot as this action
  std::vector<int> dot_ids;
  // Name to be used by get_dot() instead of action name
  std::string dot_name;
  // form spell to automatically cast
  action_t* autoshift = nullptr;
  // Action is cast as a proc or replaces an existing action with a no-cost/no-cd version
  unsigned free_spell = free_spell_e::NONE;
  // Restricts use of a spell based on form.
  unsigned form_mask;
  // Allows a spell that may be cast in NO_FORM but not in current form to be cast by exiting form.
  bool may_autounshift = true;
  bool is_auto_attack = false;
  bool break_stealth;

  druid_action_t( std::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      parse_buff_effects_t( this ),
      dot_name( n ),
      form_mask( ab::data().stance_mask() ),
      break_stealth( !ab::data().flags( spell_attribute::SX_NO_STEALTH_BREAK ) )
  {
    // WARNING: auto attacks will NOT get processed here since they have no spell data
    if ( ab::data().ok() )
    {
      apply_buff_effects();
      apply_debuffs_effects();

      if ( ab::data().flags( spell_attribute::SX_ABILITY ) || ab::trigger_gcd > 0_ms )
        ab::not_a_proc = true;

      if ( p()->spec.ursine_adept->ok() &&
           ab::data().affected_by( find_effect( p()->buff.bear_form, A_MOD_IGNORE_SHAPESHIFT ) ) )
      {
        form_mask |= form_e::BEAR_FORM;
      }
    }
  }

  druid_t* p() { return static_cast<druid_t*>( ab::player ); }

  const druid_t* p() const { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const { return p()->get_target_data( t ); }

  base_t* set_free_cast( free_spell_e f )
  {
    free_spell |= f;
    ab::cooldown->duration = 0_ms;
    return this;
  }

  bool is_free() const { return free_spell; }
  bool is_free( free_spell_e f ) const { return ( free_spell & f ) == f; }
  bool is_free_proc() const { return free_spell & free_spell_e::PROCS; }
  bool is_free_cast() const { return free_spell & free_spell_e::CASTS; }

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
                             std::function<void( const action_state_t*, action_state_t* )> pre = nullptr,
                             std::function<void( const action_state_t*, action_state_t* )> post = nullptr )
  {
    auto state = this->get_state();
    state->target = s->target;

    if ( pre )
      pre( s, state );

    this->snapshot_state( state, this->amount_type( state, is_dot ) );

    if ( post )
      post( s, state );

    this->schedule_execute( state );
  }

  void schedule_execute( action_state_t* s = nullptr ) override;
  void execute() override;
  void impact( action_state_t* s ) override;

  virtual bool can_proc_moonless_night() const
  {
    return ab::special && !ab::background && !ab::dual && !ab::proc && ( ab::aoe == 0 || ab::aoe == 1 );
  }

  void apply_buff_effects()
  {
    // Class
    parse_buff_effects( p()->buff.cat_form );
    parse_buff_effects( p()->buff.moonkin_form );
    parse_buff_effects( p()->buff.rising_light_falling_night_day );

    switch( p()->specialization() )
    {
      case DRUID_BALANCE:     parse_buff_effects( p()->buff.heart_of_the_wild, 0b000000000111U ); break;
      case DRUID_FERAL:       parse_buff_effects( p()->buff.heart_of_the_wild, 0b000000111000U ); break;
      case DRUID_GUARDIAN:    parse_buff_effects( p()->buff.heart_of_the_wild, 0b000111000000U ); break;
      case DRUID_RESTORATION: parse_buff_effects( p()->buff.heart_of_the_wild, 0b111000000000U ); break;
      default: break;
    }

    // Balance
    parse_buff_effects( p()->buff.balance_of_all_things_arcane, p()->talent.balance_of_all_things );
    parse_buff_effects( p()->buff.balance_of_all_things_nature, p()->talent.balance_of_all_things );
    parse_buff_effects( p()->buff.eclipse_lunar, p()->talent.umbral_intensity );
    parse_buff_effects( p()->buff.eclipse_solar, p()->talent.umbral_intensity );
    parse_buff_effects( p()->buff.friend_of_the_fae );
    parse_buff_effects( p()->buff.gathering_starstuff );
    parse_buff_effects( p()->buff.incarnation_moonkin, p()->talent.elunes_guidance );
    parse_buff_effects( p()->buff.owlkin_frenzy );
    parse_buff_effects( p()->buff.rattled_stars );
    parse_buff_effects( p()->buff.starweavers_warp );
    parse_buff_effects( p()->buff.starweavers_weft );
    parse_buff_effects( p()->buff.touch_the_cosmos );
    parse_buff_effects( p()->buff.warrior_of_elune, false );

    // Feral
    parse_buff_effects( p()->buff.apex_predators_craving );
    parse_buff_effects( p()->buff.berserk_cat );
    parse_buff_effects( p()->buff.incarnation_cat );
    parse_buff_effects( p()->buff.predatory_swiftness );
    parse_buff_effects( p()->buff.sabertooth, true, true );
    parse_buff_effects( p()->buff.sharpened_claws );

    // Guardian
    parse_buff_effects( p()->buff.bear_form );

    unsigned berserk_ignore_mask = p()->talent.berserk_persistence.ok() ? 0b11111100000U
                                                                        : 0b11111100110U;
                                                                          //10987654321
    parse_buff_effects( p()->buff.berserk_bear, berserk_ignore_mask,
                        p()->talent.berserk_ravage,
                        p()->talent.berserk_unchecked_aggression );
    parse_buff_effects( p()->buff.incarnation_bear, berserk_ignore_mask,
                        p()->talent.berserk_ravage,
                        p()->talent.berserk_unchecked_aggression );
    parse_buff_effects( p()->buff.dream_of_cenarius );
    parse_buff_effects( p()->buff.furious_regeneration );
    parse_buff_effects( p()->buff.gory_fur );
    parse_buff_effects( p()->buff.overpowering_aura );
    // rage of the sleeper also applies to moonfire tick via hidden script. see moonfire_damage_t
    // note that it DOES NOT apply to thrash ticks
    parse_buff_effects( p()->buff.rage_of_the_sleeper );
    parse_passive_effects( p()->talent.reinvigoration, p()->talent.innate_resolve.ok() ? 0b01U : 0b10U );
    parse_buff_effects( p()->buff.tooth_and_claw, false );
    parse_buff_effects( p()->buff.vicious_cycle_mangle, true, true );
    parse_buff_effects( p()->buff.vicious_cycle_maul, true, true );

    // Restoration
    parse_buff_effects( p()->buff.abundance );
    parse_buff_effects( p()->buff.clearcasting_tree, p()->talent.flash_of_clarity );
    parse_buff_effects( p()->buff.incarnation_tree );
    parse_buff_effects( p()->buff.natures_swiftness, p()->talent.natures_splendor );
  }

  void apply_debuffs_effects()
  {
    parse_dot_effects( &druid_td_t::dots_t::moonfire, p()->spec.moonfire_dmg, p()->mastery.astral_invocation );
    parse_dot_effects( &druid_td_t::dots_t::sunfire, p()->spec.sunfire_dmg, p()->mastery.astral_invocation );
    parse_dot_effects( &druid_td_t::dots_t::adaptive_swarm_damage,
                       p()->spec.adaptive_swarm_damage, false, p()->spec.restoration );
    parse_dot_effects( &druid_td_t::dots_t::thrash_bear, p()->spec.thrash_bear_dot, p()->talent.rend_and_tear );

    parse_debuff_effects( []( druid_td_t* td ) { return td->debuff.dire_fixation->check(); },
                          find_trigger( p()->talent.dire_fixation ).trigger() );
    parse_debuff_effects( []( druid_td_t* td ) { return td->debuff.waning_twilight->check(); },
                          p()->spec.waning_twilight, p()->talent.waning_twilight );
  }

  template <typename DOT, typename... Ts>
  void parse_dot_effects( DOT dot, const spell_data_t* spell, bool stacks, Ts... mods )
  {
    if ( stacks )
    {
      parse_debuff_effects( [ dot ]( druid_td_t* t ) {
        return std::invoke( dot, t->dots )->current_stack();
      }, spell, mods... );
    }
    else
    {
      parse_debuff_effects( [ dot ]( druid_td_t* t ) {
        return std::invoke( dot, t->dots )->is_ticking();
      }, spell, mods... );
    }
  }

  template <typename DOT, typename... Ts>
  void parse_dot_effects( DOT dot, const spell_data_t* spell, Ts... mods )
  {
    parse_dot_effects( dot, spell, true, mods... );
  }

  template <typename DOT, typename... Ts>
  void force_dot_effect( DOT dot, const spell_data_t* spell, unsigned idx, Ts... mods )
  {
    if ( ab::data().affected_by_all( spell->effectN( idx ) ) )
      return;

    parse_debuff_effect( [ dot ]( druid_td_t* t ) {
      return std::invoke( dot, t->dots )->is_ticking();
    }, spell, idx, true, mods... );
  }

  double get_debuff_effects_value( druid_td_t* t ) const override
  {
    double return_value = 1.0;

    for ( const auto& i : target_multiplier_dotdebuffs )
    {
      if ( auto check = i.func( t ) )
      {
        auto eff_val = i.value;

        if ( i.mastery )
          eff_val += p()->cache_mastery_value();

        return_value *= 1.0 + eff_val * check;
      }
    }

    return return_value;
  }

  double cost() const override
  {
    if ( is_free() || ( p()->specialization() == DRUID_RESTORATION && p()->buff.innervate->up() ) )
      return 0.0;

    double c = ab::cost();

    c += get_buff_effects_value( flat_cost_buffeffects, true, false );

    c *= get_buff_effects_value( cost_buffeffects, false, false );

    return std::max( 0.0, c );
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    return ab::composite_ta_multiplier( s ) * get_buff_effects_value( ta_multiplier_buffeffects );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return ab::composite_da_multiplier( s ) * get_buff_effects_value( da_multiplier_buffeffects );
  }

  double composite_crit_chance() const override
  {
    return ab::composite_crit_chance() + get_buff_effects_value( crit_chance_buffeffects, true );
  }

  timespan_t execute_time() const override
  {
    return std::max( 0_ms, ab::execute_time() * get_buff_effects_value( execute_time_buffeffects ) );

  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return ab::composite_dot_duration( s ) * get_buff_effects_value( dot_duration_buffeffects );
  }

  timespan_t cooldown_duration() const override
  {
    return ab::cooldown_duration() * get_buff_effects_value( recharge_multiplier_buffeffects );
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    return ab::recharge_multiplier( cd ) * get_buff_effects_value( recharge_multiplier_buffeffects );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    return ab::composite_target_multiplier( t ) * get_debuff_effects_value( td( t ) );
  }

  // Override this function for temporary effects that change the normal form restrictions of the spell. eg: Predatory
  // Swiftness
  virtual bool check_form_restriction()
  {
    if ( !form_mask || ( form_mask & p()->get_form() ) == p()->get_form() )
      return true;

    return false;
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

template <typename BASE>
struct trigger_deep_focus_t : public BASE
{
  using base_t = trigger_deep_focus_t<BASE>;

  double mul;

  trigger_deep_focus_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ), mul( p->talent.deep_focus->effectN( 1 ).percent() )
  {}

  double action_multiplier() const override
  {
    auto am = BASE::action_multiplier();

    if ( mul && BASE::get_dot_count() <= 1 )
      am *= 1.0 + mul;

    return am;
  }
};

template <typename BASE>
struct trigger_gore_t : public BASE
{
private:
  druid_t* p_;

public:
  using base_t = trigger_gore_t<BASE>;

  trigger_gore_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ), p_( p )
  {}

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( action_t::result_is_hit ( s->result ) )
    {
      if ( p_->buff.gore->trigger() )
      {
        p_->proc.gore->occur();
        p_->cooldown.mangle->reset( true );
      }
    }
  }
};

template <typename BASE>
struct trigger_waning_twilight_t : public BASE
{
private:
  druid_t* p_;
  int num_dots;

public:
  using base_t = trigger_waning_twilight_t<BASE>;

  trigger_waning_twilight_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ), p_( p ), num_dots( as<int>( p->talent.waning_twilight->effectN( 3 ).base_value() ) )
  {}

  void update_waning_twilight( player_t* t )
  {
    if ( !p_->talent.waning_twilight.ok() )
      return;

    auto td_ = BASE::td( t );
    int count = td_->dots.astral_smolder->is_ticking() +
                td_->dots.fungal_growth->is_ticking() +
                td_->dots.moonfire->is_ticking() +
                td_->dots.rake->is_ticking() +
                td_->dots.rip->is_ticking() +
                td_->dots.stellar_flare->is_ticking() +
                td_->dots.sunfire->is_ticking() +
                td_->dots.thrash_bear->is_ticking() +
                td_->dots.thrash_cat->is_ticking();

    if ( count < num_dots )
      td_->debuff.waning_twilight->expire();
    else if ( !td_->debuff.waning_twilight->check() )
      td_->debuff.waning_twilight->trigger();
  }

  void trigger_dot( action_state_t* s ) override
  {
    BASE::trigger_dot( s );

    update_waning_twilight( s->target );
  }

  void last_tick( dot_t* d ) override
  {
    BASE::last_tick( d );

    update_waning_twilight( d->target );
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

  double bleed_mul = 0.0;
  bool direct_bleed = false;

  druid_attack_t( std::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s )
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
    return direct_bleed ? 0.0 : ab::composite_target_armor( t );
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

  bool reset_melee_swing = true;  // TRUE(default) to reset swing timer on execute (as most cast time spells do)

  druid_spell_base_t( std::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s )
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

  double residual_mul = 0.0;

  druid_residual_action_t( std::string_view n, druid_t* p, const spell_data_t* s ) : Base( n, p, s )
  {
    Base::background = true;
    Base::round_base_dmg = false;

    Base::attack_power_mod.direct = Base::attack_power_mod.tick = 0;
    Base::spell_power_mod.direct = Base::spell_power_mod.tick = 0;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, Base::target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
  }

  void set_amount( action_state_t* s, double v )
  {
    cast_state( s )->total_amount = v;
  }

  virtual double get_amount( const action_state_t* s ) const
  {
    return cast_state( s )->total_amount * residual_mul;
  }

  double base_da_min( const action_state_t* s ) const override
  {
    return get_amount( s );
  }

  double base_da_max( const action_state_t* s ) const override
  {
    return get_amount( s );
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

struct astral_power_spender_t : public druid_spell_t
{
protected:
  using base_t = astral_power_spender_t;

public:
  buff_t* p_buff;
  timespan_t p_time;
  double p_cap;

  astral_power_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : druid_spell_t( n, p, s, o ),
      p_buff( p->buff.primordial_arcanic_pulsar ),
      p_time( timespan_t::from_seconds( p->talent.primordial_arcanic_pulsar->effectN( 2 ).base_value() ) ),
      p_cap( p->talent.primordial_arcanic_pulsar->effectN( 1 ).base_value() )
  {}

  void consume_resource() override
  {
    druid_spell_t::consume_resource();

    if ( is_free_proc() || !p()->talent.primordial_arcanic_pulsar.ok() )
      return;

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

      // Touch the cosmos will proc again after being used shortly after Pulsar is consumed in game
      make_event( *sim, [ this ]() { p()->buff.touch_the_cosmos->trigger(); } );

      p()->uptime.primordial_arcanic_pulsar->update( true, sim->current_time() );

      make_event( *sim, p_time, [ this ]() {
        p()->uptime.primordial_arcanic_pulsar->update( false, sim->current_time() );
      } );
    }
  }

  void post_execute()
  {
    p()->buff.touch_the_cosmos->expire();
    p()->buff.gathering_starstuff->trigger();

    if ( is_free_proc() )
      return;

    p()->buff.starlord->trigger();
    p()->buff.rattled_stars->trigger();
  }
};

template <typename BASE>
struct consume_umbral_embrace_t : public BASE
{
private:
  druid_t* p_;

public:
  using base_t = consume_umbral_embrace_t<BASE>;

  consume_umbral_embrace_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ), p_( p ) {}

  void init_umbral_embrace( const spell_data_t* ecl, dot_t* druid_td_t::dots_t::*dot, const spell_data_t* dmg )
  {
    // Umbral embrace is heavily scripted so we do all the auto parsing within the action itself
    if ( p_->talent.umbral_embrace.ok() )
    {
      BASE::da_multiplier_buffeffects.emplace_back( nullptr, p_->buff.umbral_embrace->default_value, false, false,
                                                    [ this ] { return umbral_embrace_check(); } );

      BASE::sim->print_debug( "buff-effects: {} ({}) direct_damage modified by {} with buff {} ({})", BASE::name(),
                              BASE::id, p_->buff.umbral_embrace->default_value, p_->buff.umbral_embrace->name(),
                              p_->buff.umbral_embrace->data().id() );

      BASE::force_conditional_effect( ecl, [ this ] { return umbral_embrace_check(); }, 1 );

      BASE::force_debuff_effect( [ this, dot ]( druid_td_t* t ) {
        return umbral_embrace_check() && std::invoke( dot, t->dots )->is_ticking();
      }, dmg, as<unsigned>( dmg->effect_count() ), p_->mastery.astral_invocation );
    }
  }

  bool umbral_embrace_check()
  {
    return p_->buff.umbral_embrace->check() &&
           ( p_->eclipse_handler.state == IN_SOLAR || p_->eclipse_handler.state == IN_LUNAR ||
             p_->eclipse_handler.state == IN_BOTH );
  }

  void execute() override
  {
    auto embrace = umbral_embrace_check();
    if ( embrace )
      BASE::set_school_override( SCHOOL_ASTRAL );

    BASE::execute();

    if ( embrace )
    {
      BASE::clear_school_override();
      p_->buff.umbral_embrace->expire();
    }
  }
};

template <typename BASE>
struct trigger_astral_smolder_t : public BASE
{
private:
  druid_t* p_;
  buff_t* other_ecl = nullptr;
  dot_t* druid_td_t::dots_t::* other_dot = nullptr;
  double mul;

public:
  using base_t = trigger_astral_smolder_t<BASE>;

  trigger_astral_smolder_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ),
      p_( p ),
      mul( p->talent.astral_smolder->effectN( 1 ).percent() )
  {}

  void init_astral_smolder( buff_t* b, dot_t* druid_td_t::dots_t::*d )
  {
    other_ecl = b;
    other_dot = d;
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( !p_->active.astral_smolder || !s->result_amount || s->result != RESULT_CRIT || BASE::is_free_proc() )
      return;

    assert( other_ecl && other_dot );
    auto amount = s->result_amount * mul;
    amount *= 1.0 + other_ecl->check_value();

    auto dot = std::invoke( other_dot, BASE::td( s->target )->dots );
    if ( !p_->bugs && dot->is_ticking() )
      amount *= 1.0 + p_->cache_mastery_value();

    residual_action::trigger( p_->active.astral_smolder, s->target, amount );
  }
};

template <typename BASE>
struct trigger_shooting_stars_t : public BASE
{
private:
  druid_t* p_;

public:
  using base_t = trigger_shooting_stars_t<BASE>;

  std::vector<dot_t*>* dot_list = nullptr;

  trigger_shooting_stars_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ), p_( p )
  {}

  void init() override
  {
    assert( dot_list );

    BASE::init();
  }

  void trigger_dot( action_state_t* s ) override
  {
    dot_t* d = BASE::get_dot( s->target );
    if( !d->is_ticking() )
    {
      assert( !range::contains( *dot_list, d ) );
      dot_list->push_back( d );
    }

    BASE::trigger_dot( s );
  }

  void last_tick( dot_t* d ) override
  {
    assert( range::contains( *dot_list, d ) );
    dot_list->erase( std::remove( dot_list->begin(), dot_list->end(), d ), dot_list->end() );

    BASE::last_tick( d );
  }
};

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

  void execute() override
  {
    druid_spell_t::execute();

    p()->shapeshift( form );
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

  moonkin_form_t( druid_t* p, const spell_data_t* s )
    : druid_form_t( "moonkin_form", p, s, "", MOONKIN_FORM )
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

  double forestwalk_mul;  // % healing from forestwalk
  double imp_fr_mul;      // % healing from improved frenzied regeneration
  double photo_mul;       // tick rate multiplier when lb is on self
  double photo_pct;       // % chance to bloom when lb is on other
  double iron_mul = 0.0;  // % healing from hots with ironbark
  bool target_self = false;

  druid_heal_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), std::string_view opt = {} )
    : base_t( n, p, s ),
      affected_by(),
      forestwalk_mul( find_effect( p->buff.forestwalk, A_MOD_HEALING_PCT ).percent() ),
      imp_fr_mul( find_effect( p->talent.verdant_heart, A_ADD_FLAT_MODIFIER, P_EFFECT_2 ).percent() ),
      photo_mul( p->talent.photosynthesis->effectN( 1 ).percent() ),
      photo_pct( p->talent.photosynthesis->effectN( 2 ).percent() )
  {
    add_option( opt_bool( "target_self", target_self ) );
    parse_options( opt );

    if ( target_self )
      target = p;

    may_miss = harmful = false;
    ignore_false_positive = true;

    if ( p->talent.stonebark.ok() && find_effect( p->talent.ironbark, this, A_MOD_HEALING_RECEIVED ).ok() )
      iron_mul = find_effect( p->talent.stonebark, A_ADD_FLAT_MODIFIER, P_EFFECT_2 ).percent();

    if ( p->talent.flourish.ok() )
      affected_by.flourish = find_effect( p->talent.flourish, this, A_ADD_PCT_MODIFIER, P_TICK_TIME ).ok();
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
      if ( p()->buff.forestwalk->check() )
        ctm *= 1.0 + forestwalk_mul;

      if ( p()->buff.barkskin->check() || td( t )->hots.frenzied_regeneration->is_ticking() )
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

  snapshot_counter_t* bt_counter = nullptr;
  snapshot_counter_t* tf_counter = nullptr;

  double berserk_cp;
  double primal_claws_cp;  // TODO: remove in 10.1.5
  double primal_fury_cp;

  cat_attack_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), std::string_view opt = {} )
    : base_t( n, p, s ),
      snapshots(),
      berserk_cp( p->spec.berserk_cat->effectN( 2 ).base_value() ),
      primal_claws_cp( p->talent.primal_claws->effectN( 2 ).base_value() ),
      primal_fury_cp( find_effect( find_trigger( p->talent.primal_fury ).trigger(), E_ENERGIZE ).resource( RESOURCE_COMBO_POINT ) )
  {
    parse_options( opt );

    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
      ap_type = attack_power_type::NO_WEAPON;

    if ( data().ok() )
    {
      snapshots.bloodtalons =
          parse_persistent_buff_effects( p->buff.bloodtalons, 0U, false );

      snapshots.tigers_fury =
          parse_persistent_buff_effects( p->buff.tigers_fury, 0U, true, p->talent.carnivorous_instinct );

      snapshots.clearcasting =
          parse_persistent_buff_effects( p->buff.clearcasting_cat, 0U, false, p->talent.moment_of_clarity );

      parse_passive_effects( p->mastery.razor_claws );
    }
  }

  virtual bool stealthed() const  // For effects that require any form of stealth.
  {
    // Make sure we call all for accurate benefit tracking. Berserk/Incarn/Sudden Assault handled in shred_t & rake_t -
    // move here if buff while stealthed becomes more widespread
    return p()->buff.prowl->up() || p()->buffs.shadowmeld->up();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    auto ea = base_t::composite_energize_amount( s );

    if ( energize_resource == RESOURCE_COMBO_POINT && energize_amount > 0 && p()->buff.b_inc_cat->check() )
      ea += berserk_cp;

    return ea;
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

      p()->sim->print_debug(
          "persistent-buffs: {} ({}) damage modified by {}% with buff {} ({}), tick table has {} entries.", name(), id,
          persistent_multiplier_buffeffects.back().value * 100.0, buff->name(), buff->data().id(),
          ta_multiplier_buffeffects.size() );

      return true;
    }
    // no persistent multiplier, but does snapshot & consume the buff
    if ( da_multiplier_buffeffects.size() > da_old || cost_buffeffects.size() > cost_old )
      return true;

    return false;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    return base_t::composite_persistent_multiplier( s ) * get_buff_effects_value( persistent_multiplier_buffeffects );
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
           p()->buff.b_inc_cat->check() )
      {
        trigger_berserk_frenzy( s->target, s );
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

  void trigger_energy_refund()
  {
    player->resource_gain( RESOURCE_ENERGY, last_resource_cost * 0.80, p()->gain.energy_refund );
  }

  void trigger_primal_claws()  // TODO: remove in 10.1.5
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

  void trigger_berserk_frenzy( player_t* t, const action_state_t* s )
  {
    if ( !special || !harmful || !s->result_amount )
      return;

    auto d = s->result_amount * p()->talent.berserk_frenzy->effectN( 1 ).percent();

    residual_action::trigger( p()->active.frenzied_assault, t, d );
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
struct combo_point_spender_t : public cat_attack_t
{
protected:
  using base_t = combo_point_spender_t<Data>;
  using state_t = druid_action_state_t<Data>;

public:
  bool consumes_combo_points = true;

  combo_point_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt = {} )
    : cat_attack_t( n, p, s, opt )
  {}

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
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

  bool ready() override
  {
    if ( consumes_combo_points && p()->resources.current[ RESOURCE_COMBO_POINT ] < 1 )
      return false;

    return cat_attack_t::ready();
  }

  void consume_resource() override
  {
    cat_attack_t::consume_resource();

    if ( background || !hit_any_target || !consumes_combo_points )
      return;

    auto consumed = _combo_points();

    if ( p()->talent.berserk_heart_of_the_lion.ok() )
    {
      auto dur_ = timespan_t::from_seconds( p()->talent.berserk_heart_of_the_lion->effectN( 1 ).base_value() *
                                            consumed * -0.1 );

      if ( p()->talent.incarnation_cat.ok() )
        p()->cooldown.incarnation_cat->adjust( dur_ );
      else
        p()->cooldown.berserk_cat->adjust( dur_ );
    }

    p()->buff.frantic_momentum->trigger( 1, buff_t::DEFAULT_VALUE(),
                                         consumed * p()->talent.frantic_momentum->effectN( 1 ).percent() );

    p()->buff.predatory_swiftness->trigger( 1, buff_t::DEFAULT_VALUE(),
                                            consumed * p()->talent.predatory_swiftness->effectN( 3 ).percent() );

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

    p()->buff.sudden_ambush->trigger( 1, buff_t::DEFAULT_VALUE(),
                                      consumed * p()->talent.sudden_ambush->effectN( 1 ).percent() );

    if ( free_spell & free_spell_e::CONVOKE )  // further effects are not processed for convoke fb
      return;

    p()->buff.sharpened_claws->trigger( consumed );

    if ( !is_free() )
    {
      p()->resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );

      sim->print_log( "{} consumes {} {} for {} (0)", player->name(), consumed,
                      util::resource_type_string( RESOURCE_COMBO_POINT ), name() );

      stats->consume_resource( RESOURCE_COMBO_POINT, consumed );
    }

    if ( p()->buff.tigers_tenacity->check() )
    {
      p()->resource_gain( RESOURCE_COMBO_POINT, p()->buff.tigers_tenacity->value(), p()->gain.tigers_tenacity );
      p()->buff.tigers_tenacity->decrement();
    }
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( !consumes_combo_points )
      return;

    p()->resource_gain( RESOURCE_COMBO_POINT, p()->buff.overflowing_power->check(), p()->gain.overflowing_power );
    p()->buff.overflowing_power->expire();
  }
};

using cat_finisher_t = combo_point_spender_t<>;

template <typename BASE>
struct trigger_thrashing_claws_t : public BASE
{
  using base_t = trigger_thrashing_claws_t<BASE>;

  trigger_thrashing_claws_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o )
  {
    if ( p->talent.thrashing_claws.ok() )
      BASE::impact_action = p->active.thrashing_claws;
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

    buff->extend_duration_or_trigger();
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
struct brutal_slash_t : public trigger_thrashing_claws_t<cat_attack_t>
{
  double berserk_swipe_cp;

  brutal_slash_t( druid_t* p, std::string_view opt )
    : base_t( "brutal_slash", p, p->talent.brutal_slash, opt ),
      berserk_swipe_cp( p->spec.berserk_cat->effectN( 2 ).base_value() )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
    track_cd_waste = true;

    if ( p->talent.merciless_claws.ok() )
      bleed_mul = p->talent.merciless_claws->effectN( 1 ).percent();
  }

  resource_e current_resource() const override
  {
    return p()->buff.cat_form->check() ? RESOURCE_ENERGY : RESOURCE_NONE;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    auto ea = base_t::composite_energize_amount( s );

    if ( p()->buff.b_inc_cat->check() )
      ea += berserk_swipe_cp;

    return ea;
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.bt_brutal_slash->trigger();
  }
};

// Feral Frenzy =============================================================
struct feral_frenzy_t : public cat_attack_t
{
  struct feral_frenzy_tick_t : public cat_attack_t
  {
    bool is_direct_damage = false;

    feral_frenzy_tick_t( druid_t* p, std::string_view n ) : cat_attack_t( n, p, p->find_spell( 274838 ) )
    {
      background = dual = true;
      direct_bleed = false;

      dot_name = "feral_frenzy_tick";
      berserk_cp = 0;  // feral frenzy does not count as a cp generator for berserk extra cp
    }

    // Small hack to properly distinguish instant ticks from the driver, from actual periodic ticks from the bleed
    result_amount_type report_amount_type( const action_state_t* s ) const override
    {
      return is_direct_damage ? result_amount_type::DMG_DIRECT : s->result_type;
    }

    void execute() override
    {
      is_direct_damage = true;
      cat_attack_t::execute();
      is_direct_damage = false;
    }

    void trigger_primal_fury() override {}
  };

  feral_frenzy_t( druid_t* p, std::string_view opt ) : feral_frenzy_t( p, "feral_frenzy", p->talent.feral_frenzy, opt )
  {}

  feral_frenzy_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : cat_attack_t( n, p, s, opt )
  {
    track_cd_waste = true;

    if ( data().ok() )
    {
      tick_action = p->get_secondary_action_n<feral_frenzy_tick_t>( name_str + "_tick" );
      tick_action->stats = stats;
    }
  }

  void init() override
  {
    cat_attack_t::init();

    if ( tick_action )
      tick_action->gain = gain;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.bt_feral_frenzy->trigger();
  }
};

// Ferocious Bite ===========================================================
struct ferocious_bite_t : public cat_finisher_t
{
  struct rampant_ferocity_old_t : public druid_residual_action_t<cat_attack_t>  // TODO: remove in 10.1.5
  {
    rampant_ferocity_old_t( druid_t* p, std::string_view n ) : base_t( n, p, p->find_spell( 391710 ) )
    {
      aoe = -1;
      reduced_aoe_targets = p->talent.rampant_ferocity->effectN( 2 ).base_value();
      name_str_reporting = "rampant_ferocity";

      residual_mul = p->talent.rampant_ferocity->effectN( 1 ).percent();
    }

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

  struct rampant_ferocity_t : public cat_attack_t
  {
    using state_t = druid_action_state_t<cat_finisher_data_t>;

    rampant_ferocity_t( druid_t* p, std::string_view n ) : cat_attack_t( n, p, p->find_spell( 391710 ) )
    {
      aoe = -1;
      reduced_aoe_targets = p->talent.rampant_ferocity->effectN( 2 ).base_value();
      name_str_reporting = "rampant_ferocity";
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    state_t* cast_state( action_state_t* s )
    {
      return static_cast<state_t*>( s );
    }

    const state_t* cast_state( const action_state_t* s ) const
    {
      return static_cast<const state_t*>( s );
    }

    std::vector<player_t*>& target_list() const override
    {
      target_cache.is_valid = false;

      std::vector<player_t*>& tl = cat_attack_t::target_list();

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* t ) {
        return !td( t )->dots.rip->is_ticking() || t == target;
      } ), tl.end() );

      return tl;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      return cat_attack_t::composite_da_multiplier( s ) * cast_state( s )->combo_points;
    }
  };

  cat_attack_t* rampant_ferocity = nullptr;
  double excess_energy = 0.0;
  double max_excess_energy;
  bool max_energy = false;

  ferocious_bite_t( druid_t* p, std::string_view opt ) : ferocious_bite_t( p, "ferocious_bite", opt ) {}

  ferocious_bite_t( druid_t* p, std::string_view n, std::string_view opt )
    : cat_finisher_t( n, p, p->find_class_spell( "Ferocious Bite" ) )
  {
    add_option( opt_bool( "max_energy", max_energy ) );
    parse_options( opt );

    if ( p->talent.rampant_ferocity.ok() )
    {
      auto suf = get_suffix( name_str, "ferocious_bite" );

      if ( !p->is_ptr() )
        rampant_ferocity = p->get_secondary_action_n<rampant_ferocity_old_t>( "rampant_ferocity" + suf );
      else
        rampant_ferocity = p->get_secondary_action_n<rampant_ferocity_t>( "rampant_ferocity" + suf );

      rampant_ferocity->background = true;
      add_child( rampant_ferocity );
    }

    parse_effect_modifiers( p->talent.relentless_predator );
    max_excess_energy = modified_effect( find_effect_index( this, E_POWER_BURN ) ).base_value();
  }

  double maximum_energy() const
  {
    double req = base_costs[ RESOURCE_ENERGY ] + max_excess_energy;

    req *= 1.0 + p()->buff.incarnation_cat->check_value();

    if ( p()->buff.apex_predators_craving->check() )
      req *= 1.0 + p()->buff.apex_predators_craving->data().effectN( 1 ).percent();

    return req;
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

    p()->buff.sabertooth->expire();  // existing buff is replaced with new buff, regardless of CP
    p()->buff.sabertooth->trigger( cast_state( s )->combo_points );

    if ( rampant_ferocity && s->result_amount > 0 && !rampant_ferocity->target_list().empty() )
    {
      if ( !p()->is_ptr() )
      {
        rampant_ferocity->snapshot_and_execute( s, false, [ this ]( const action_state_t* from, action_state_t* to ) {
          debug_cast<rampant_ferocity_old_t*>( rampant_ferocity )->set_amount( to, from->result_amount );
        } );
      }
      else
      {
        rampant_ferocity->snapshot_and_execute( s, false, [ this ]( const action_state_t* from, action_state_t* to ) {
          debug_cast<rampant_ferocity_t*>( rampant_ferocity )->cast_state( to )->combo_points = cast_state( from )->combo_points;
        } );
      }
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

    if ( hit_any_target && free_spell & free_spell_e::APEX )
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
  frenzied_assault_t( druid_t* p ) : residual_action_t( "frenzied_assault", p, p->find_spell( 391140 ) )
  {
    proc = true;
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
    return p()->talent.lunar_inspiration.ok() ? cat_attack_t::ready() : false;
  }
};

// Maim =====================================================================
struct maim_t : public cat_finisher_t
{
  maim_t( druid_t* p, std::string_view options_str ) : cat_finisher_t( "maim", p, p->talent.maim, options_str ) {}

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return cat_finisher_t::composite_da_multiplier( s ) * cast_state( s )->combo_points;
  }
};

// Rake =====================================================================
struct rake_t : public trigger_deep_focus_t<cat_attack_t>
{
  struct rake_bleed_t : public trigger_deep_focus_t<trigger_waning_twilight_t<cat_attack_t>>
  {
    rake_bleed_t( druid_t* p, std::string_view n, const spell_data_t* s ) : base_t( n, p, s )
    {
      background = dual = true;
      // override for convoke. since this is only ever executed from rake_t, form checking is unnecessary.
      form_mask = 0;

      dot_name = "rake";
    }
  };

  rake_bleed_t* bleed;
  double stealth_mul = 0.0;

  rake_t( druid_t* p, std::string_view opt ) : rake_t( p, "rake", p->talent.rake, opt ) {}

  rake_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt ) : base_t( n, p, s, opt )
  {
    if ( p->talent.pouncing_strikes.ok() || p->spec.improved_prowl->ok() )
      stealth_mul = data().effectN( 4 ).percent();

    aoe = std::max( aoe, 1 ) + as<int>( p->talent.doubleclawed_rake->effectN( 1 ).base_value() );

    bleed = p->get_secondary_action_n<rake_bleed_t>( name_str + "_bleed", find_trigger( this ).trigger() );
    bleed->stats = stats;

    dot_name = "rake";
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = base_t::composite_persistent_multiplier( s );

    if ( stealth_mul && ( stealthed() || p()->buff.sudden_ambush->check() ) )
      pm *= 1.0 + stealth_mul;

    return pm;
  }
  
  bool has_amount_result() const override { return bleed->has_amount_result(); }

  std::vector<player_t*>& target_list() const override
  {
    if ( !target_cache.is_valid )
      bleed->target_cache.is_valid = false;

    auto& tl = base_t::target_list();

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

    base_t::execute();

    if ( hit_any_target )
    {
      p()->buff.bt_rake->trigger();
      
      if ( !stealthed() )
      	p()->buff.sudden_ambush->expire();
    }
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    bleed->snapshot_and_execute( s, true, nullptr, []( const action_state_t* from, action_state_t* to ) {
      // Copy persistent multipliers from the direct attack.
      to->persistent_multiplier = from->persistent_multiplier;
    } );
  }
};

// Rip ======================================================================
struct rip_t : public trigger_deep_focus_t<trigger_waning_twilight_t<cat_finisher_t>>
{
  struct tear_t : public druid_residual_action_t<cat_attack_t>
  {
    tear_t( druid_t* p, std::string_view n ) : base_t( n, p, p->find_spell( 391356 ) )
    {
      name_str_reporting = "tear";

      // Rip and Tear is talent value is the amount over full duration
      residual_mul = p->talent.rip_and_tear->effectN( 1 ).percent() *
                     find_effect( this, A_PERIODIC_DAMAGE ).period() / dot_duration;
    }

    double base_da_min( const action_state_t* ) const override
    {
      return 0.0;
    }

    double base_da_max( const action_state_t* ) const override
    {
      return 0.0;
    }

    double base_ta( const action_state_t* s ) const override
    {
      return get_amount( s );
    }
  };

  tear_t* tear;

  rip_t( druid_t* p, std::string_view opt ) : rip_t( p, "rip", p->talent.rip, opt ) {}

  rip_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : base_t( n, p, s, opt ), tear( nullptr )
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
    base_t::init_finished();

    if ( tear )
      add_child( tear );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t t = base_t::composite_dot_duration( s );

    return t *= cast_state( s )->combo_points + 1;
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( tear && result_is_hit( s->result ) )
    {
      auto dot_total = calculate_tick_amount( s, 1.0 ) * find_dot( s->target )->remains() / tick_time( s );

      // increased damage from sabertooth is not counted for tear calculations
      dot_total /= 1.0 + p()->buff.sabertooth->check_stack_value();

      tear->snapshot_and_execute( s, true, [ this, dot_total ]( const action_state_t*, action_state_t* to ) {
        tear->set_amount( to, dot_total );
      } );
    }
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    p()->buff.apex_predators_craving->trigger();
  }
};

// Primal Wrath =============================================================
// NOTE: must be defined AFTER rip_T
struct primal_wrath_t : public cat_finisher_t
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
      aoe = -1;
      reduced_aoe_targets = p->talent.tear_open_wounds->effectN( 3 ).base_value();
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

    std::vector<player_t*>& target_list() const override
    {
      target_cache.is_valid = false;

      std::vector<player_t*>& tl = cat_attack_t::target_list();

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* t ) {
        return !td( t )->dots.rip->is_ticking();
      } ), tl.end() );

      return tl;
    }

    void impact( action_state_t* s ) override
    {
      cat_attack_t::impact( s );

      td( s->target )->dots.rip->adjust_duration( -4_s );
    }
  };

  rip_t* rip;
  action_t* wounds = nullptr;

  primal_wrath_t( druid_t* p, std::string_view opt ) : cat_finisher_t( "primal_wrath", p, p->talent.primal_wrath, opt )
  {
    aoe = -1;

    dot_name = "rip";
    // Manually set true so bloodtalons is decremented and we get proper snapshot reporting
    snapshots.bloodtalons = true;

    parse_effect_modifiers( p->talent.circle_of_life_and_death );
    parse_effect_modifiers( p->talent.veinripper );

    rip = p->get_secondary_action_n<rip_t>( "rip_primal", p->find_spell( 1079 ), "" );
    rip->dot_duration = timespan_t::from_seconds( modified_effect( 2 ).base_value() );
    rip->dual = rip->background = true;
    rip->stats = stats;
    rip->base_costs[ RESOURCE_ENERGY ] = 0;
    rip->consumes_combo_points = false;
    // mods are parsed on construction so set to false so the rip execute doesn't decrement
    rip->snapshots.bloodtalons = false;

    if ( p->talent.tear_open_wounds.ok() )
    {
      wounds = p->get_secondary_action<tear_open_wounds_t>( "tear_open_wounds" );
      add_child( wounds );
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return cat_attack_t::composite_da_multiplier( s ) * ( 1.0 + cast_state( s )->combo_points );
  }

  void impact( action_state_t* s ) override
  {
    if ( wounds && s->chain_target == 0 )
      wounds->execute_on_target( s->target );

    rip->snapshot_and_execute( s, true );

    cat_finisher_t::impact( s );
  }
};

// Shred ====================================================================
struct shred_t : public trigger_thrashing_claws_t<cat_attack_t>
{
  double stealth_mul;
  double stealth_cp;

  shred_t( druid_t* p, std::string_view opt ) : shred_t( p, "shred", opt ) {}

  shred_t( druid_t* p, std::string_view n, std::string_view opt )
    : base_t( n, p, p->find_class_spell( "Shred" ), opt ), stealth_mul( 0.0 ), stealth_cp( 0.0 )
  {
    // TODO adjust if it becomes possible to take both
    bleed_mul = std::max( p->talent.merciless_claws->effectN( 2 ).percent(),
                          p->talent.thrashing_claws->effectN( 1 ).percent() );

    if ( p->talent.pouncing_strikes.ok() )
    {
      stealth_mul = data().effectN( 3 ).percent();
      stealth_cp = p->find_spell( 343232 )->effectN( 1 ).base_value();
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = base_t::composite_energize_amount( s );

    if ( stealthed() || p()->buff.sudden_ambush->check() )
      e += stealth_cp;

    return e;
  }

  void execute() override
  {
    base_t::execute();

    if ( hit_any_target )
    {
      p()->buff.bt_shred->trigger();

      if ( !stealthed() )
        p()->buff.sudden_ambush->expire();
    }
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) && p()->talent.dire_fixation.ok() )
    {
      if ( p()->dire_fixation_target != s->target )
      {
        if ( p()->dire_fixation_target )
          td( p()->dire_fixation_target )->debuff.dire_fixation->expire();

        p()->dire_fixation_target = s->target;
      }

      td( p()->dire_fixation_target )->debuff.dire_fixation->trigger();
    }
  }

  double composite_crit_chance_multiplier() const override
  {
    double cm = base_t::composite_crit_chance_multiplier();

    if ( stealth_mul && ( stealthed() || p()->buff.sudden_ambush->check() ) )
      cm *= 2.0;

    return cm;
  }

  double action_multiplier() const override
  {
    double m = base_t::action_multiplier();

    if ( stealth_mul && ( stealthed() || p()->buff.sudden_ambush->check() ) )
      m *= 1.0 + stealth_mul;

    return m;
  }
};

// Swipe (Cat) ====================================================================
struct swipe_cat_t : public trigger_thrashing_claws_t<cat_attack_t>
{
  swipe_cat_t( druid_t* p, std::string_view opt )
    : base_t( "swipe_cat", p, p->apply_override( p->spec.swipe, p->spec.cat_form_override ), opt )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 4 ).base_value();

    if ( p->talent.merciless_claws.ok() )
      bleed_mul = p->talent.merciless_claws->effectN( 1 ).percent();

    if ( p->specialization() == DRUID_FERAL )
      name_str_reporting = "swipe";
  }

  bool ready() override
  {
    return p()->talent.brutal_slash.ok() ? false : base_t::ready();
  }

  void execute() override
  {
    base_t::execute();

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
    track_cd_waste = true;

    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.tigers_fury->trigger();
    
    p()->buff.tigers_tenacity->trigger();
  }
};

// Thrash (Cat) =============================================================
struct thrash_cat_dot_t : public trigger_waning_twilight_t<cat_attack_t>
{
  thrash_cat_dot_t( druid_t* p, std::string_view n ) : base_t( n, p, p->spec.thrash_cat_dot )
  {
    dual = background = true;

    dot_name = "thrash_cat";
  }

  void trigger_dot( action_state_t* s ) override
  {
    // cat thrash is not applied if existing bear thrash has longer duration
    auto thrash_bear = td( s->target )->dots.thrash_bear;
    if ( thrash_bear->remains() > composite_dot_duration( s ) )
      return;

    thrash_bear->cancel();

    base_t::trigger_dot( s );
  }
};

struct thrash_cat_t : public cat_attack_t
{
  thrash_cat_t( druid_t* p, std::string_view opt )
    : thrash_cat_t( p, "thrash_cat", p->apply_override( p->talent.thrash, p->spec.cat_form_override ), opt )
  {}

  thrash_cat_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : cat_attack_t( n, p, s, opt )
  {
    aoe = -1;

    impact_action = p->get_secondary_action_n<thrash_cat_dot_t>( name_str + "_dot" );
    impact_action->stats = stats;

    dot_name = "thrash_cat";

    if ( p->specialization() == DRUID_FERAL )
      name_str_reporting = "thrash";

    if ( p->bugs )
      berserk_cp = 0;  // thrash does not grant extra CP during berserk
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
  bear_attack_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                 std::string_view opt = {} )
    : base_t( n, p, s )
  {
    parse_options( opt );

    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
      ap_type = attack_power_type::NO_WEAPON;
  }
};  // end bear_attack_t

struct rage_spender_t : public bear_attack_t
{
protected:
  using base_t = rage_spender_t;

public:
  buff_t* atw_buff;
  double ug_cdr;

  rage_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o= {} )
    : bear_attack_t( n, p, s, o ),
      atw_buff( p->buff.after_the_wildfire ),
      ug_cdr( p->talent.ursocs_guidance->effectN( 5 ).base_value() )
  {}

  void consume_resource() override
  {
    bear_attack_t::consume_resource();

    if ( is_free() || !last_resource_cost )
      return;

    if ( p()->talent.after_the_wildfire.ok() )
    {
      if ( !atw_buff->check() )
        atw_buff->trigger();

      atw_buff->current_value -= last_resource_cost;

      if ( atw_buff->current_value <= 0 )
      {
        atw_buff->current_value += atw_buff->default_value;

        p()->active.after_the_wildfire_heal->execute();
      }
    }

    if ( p()->talent.ursocs_guidance.ok() && p()->talent.incarnation_bear.ok() )
      p()->cooldown.incarnation_bear->adjust( timespan_t::from_seconds( last_resource_cost / -ug_cdr ) );
  }
};

template <typename BASE>
struct trigger_ursocs_fury_t : public BASE
{
private:
  druid_t* p_;
  double cap = 0.3;  // not in spell data
  double mul;

public:
  using base_t = trigger_ursocs_fury_t<BASE>;

  trigger_ursocs_fury_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ), p_( p ), mul( p->talent.ursocs_fury->effectN( 1 ).percent() )
  {}

  void trigger_ursocs_fury( const action_state_t* s )
  {
    if ( !s->result_amount )
      return;

    p_->buff.ursocs_fury->trigger( 1, s->result_amount * ( 1.0 + mul ) );
    p_->buff.ursocs_fury->current_value = std::min( p_->buff.ursocs_fury->current_value, cap * p_->max_health() );
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    trigger_ursocs_fury( s );
  }

  void tick( dot_t* d ) override
  {
    BASE::tick( d );

    trigger_ursocs_fury( d->state );
  }
};

template <typename BASE>
struct trigger_indomitable_guardian_t : public BASE
{
private:
  druid_t* p_;

public:
  using base_t = trigger_indomitable_guardian_t<BASE>;

  trigger_indomitable_guardian_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ), p_( p )
  {}

  void execute() override
  {
    BASE::execute();

    p_->buff.indomitable_guardian->trigger();
  }
};

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

    // Always resets mangle & thrash CDs, even without berserk: ravage
    p()->cooldown.mangle->reset( true );
    p()->cooldown.thrash_bear->reset( true );

    if ( p()->talent.berserk_persistence.ok() )
      p()->cooldown.frenzied_regeneration->reset( true );
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
    background = proc = dual = true;
    aoe = -1;
  }
};

// Bristling Fur Spell ======================================================
struct bristling_fur_t : public bear_attack_t
{
  bristling_fur_t( druid_t* p, std::string_view opt )
    : bear_attack_t( "bristling_fur", p, p->talent.bristling_fur, opt )
  {
    harmful = false;
    gcd_type = gcd_haste_type::ATTACK_HASTE;
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
struct ironfur_t : public trigger_indomitable_guardian_t<rage_spender_t>
{
  timespan_t goe_ext;
  double lm_chance;

  ironfur_t( druid_t* p, std::string_view opt ) : ironfur_t( p, "ironfur", p->talent.ironfur, opt ) {}

  ironfur_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : base_t( n, p, s, opt ),
      goe_ext( find_effect( p->buff.guardian_of_elune, A_ADD_FLAT_MODIFIER, P_DURATION ).time_value() ),
      lm_chance( p->talent.layered_mane->effectN( 1 ).percent() )
  {
    use_off_gcd = true;
    harmful = may_miss = may_parry = may_dodge = false;
  }

  void execute() override
  {
    base_t::execute();

    int stack = 1;

    // TODO: does guardian of elune also apply to the extra application from layered mane?
    if ( p()->talent.layered_mane.ok() && rng().roll( lm_chance ) )
      stack++;

    auto dur = p()->buff.ironfur->buff_duration();

    if ( p()->buff.guardian_of_elune->check() )
      dur += goe_ext;

    p()->buff.ironfur->trigger( stack, dur );

    p()->buff.guardian_of_elune->expire();

    if ( !is_free_proc() )
    {
      p()->buff.gory_fur->expire();

      if ( p()->active.thorns_of_iron )
        p()->active.thorns_of_iron->execute();
    }
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

  swiping_mangle_t* swiping = nullptr;
  action_t* healing = nullptr;
  int inc_targets = 0;

  mangle_t( druid_t* p, std::string_view opt ) : mangle_t( p, "mangle", opt ) {}

  mangle_t( druid_t* p, std::string_view n, std::string_view opt )
    : bear_attack_t( n, p, p->find_class_spell( "Mangle" ), opt )
  {
    track_cd_waste = true;

    if ( p->talent.mangle.ok() )
      bleed_mul = data().effectN( 3 ).percent();

    parse_effect_modifiers( p->talent.soul_of_the_forest_bear );
    energize_amount = modified_effect( find_effect_index( this, E_ENERGIZE ) ).resource( RESOURCE_RAGE );

    if ( p->talent.incarnation_bear.ok() )
    {
      inc_targets =
          as<int>( find_effect( p->spec.incarnation_bear, this, A_ADD_FLAT_MODIFIER, P_TARGET ).base_value() );
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
    double e = bear_attack_t::composite_energize_amount( s );

    e += p()->buff.gore->check_value();

    e *= 1.0 + p()->buff.furious_regeneration->check_value();

    return e;
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

    if ( swiping && p()->buff.gore->check() && s->result_amount > 0 && s->chain_target == 0 &&
         !swiping->target_list().empty() )
    {
      // 2pc expires gore even on free_procs
      p()->buff.gore->expire();

      swiping->snapshot_and_execute( s, false, [ this ]( const action_state_t* from, action_state_t* to ) {
        swiping->set_amount( to, from->result_amount );
      } );
    }
  }

  void execute() override
  {
    // this is proc'd before the cast and thus benefits the cast
    if ( p()->buff.gore->check() )
      p()->buff.overpowering_aura->trigger();

    bear_attack_t::execute();

    if ( !hit_any_target )
      return;

    if ( !is_free_proc() )
    {
      p()->buff.gore->expire();

      if ( healing )
        healing->execute();
    }

    p()->buff.gory_fur->trigger();
    p()->buff.guardian_of_elune->trigger();

    p()->buff.vicious_cycle_mangle->expire();
    p()->buff.vicious_cycle_maul->trigger( num_targets_hit );
  }
};

// Maul =====================================================================
struct maul_t : public trigger_indomitable_guardian_t<trigger_ursocs_fury_t<trigger_gore_t<rage_spender_t>>>
{
  maul_t( druid_t* p, std::string_view opt ) : maul_t( p, "maul", opt ) {}

  maul_t( druid_t* p, std::string_view n, std::string_view opt ) : base_t( n, p, p->talent.maul, opt ) {}

  bool ready() override
  {
    return ( !p()->is_ptr() && p()->talent.raze.ok() ) ? false : base_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->buff.tooth_and_claw->check() )
      td( s->target )->debuff.tooth_and_claw->trigger();
  }

  void execute() override
  {
    if ( !is_free() && p()->buff.tooth_and_claw->up() )
    {
      p()->active.maul_tooth_and_claw->execute_on_target( target );
      return;
    }

    base_t::execute();

    if ( !hit_any_target )
      return;

    p()->buff.vicious_cycle_maul->expire();
    p()->buff.vicious_cycle_mangle->trigger();

    p()->buff.tooth_and_claw->decrement();
  }
};

// Moonless Night ===========================================================
struct moonless_night_t : public druid_residual_action_t<bear_attack_t>
{
  moonless_night_t( druid_t* p ) : base_t( "moonless_night", p, p->find_spell( 400360 ) )
  {
    proc = true;
    residual_mul = p->talent.moonless_night->effectN( 1 ).percent();
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
    background = proc = dual = true;
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

// Raze =====================================================================
struct raze_t : public trigger_indomitable_guardian_t<trigger_ursocs_fury_t<trigger_gore_t<rage_spender_t>>>
{
  raze_t( druid_t* p, std::string_view opt ) : raze_t( p, "raze", opt ) {}

  raze_t( druid_t* p, std::string_view n, std::string_view opt ) : base_t( n, p, p->talent.raze, opt )
  {
    aoe = -1;  // actually a frontal cone
    reduced_aoe_targets = 5.0;  // PTR not in spell data
  }

  bool can_proc_moonless_night() const override
  {
    return true;
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->buff.tooth_and_claw->check() )
      td( s->target )->debuff.tooth_and_claw->trigger();
  }

  void execute() override
  {
    if ( !is_free() && p()->buff.tooth_and_claw->up() )
    {
      p()->active.raze_tooth_and_claw->execute_on_target( target );
      return;
    }

    base_t::execute();

    if ( !hit_any_target )
      return;

    p()->buff.vicious_cycle_maul->expire();
    // raze triggers vicious cycle once per target hit
    p()->buff.vicious_cycle_mangle->trigger( num_targets_hit );

    p()->buff.tooth_and_claw->decrement();
  }
};

// Swipe (Bear) =============================================================
struct swipe_bear_t : public trigger_gore_t<bear_attack_t>
{
  swipe_bear_t( druid_t* p, std::string_view opt )
    : base_t( "swipe_bear", p, p->apply_override( p->spec.swipe, p->spec.bear_form_override ), opt )
  {
    aoe = -1;
    // target hit data stored in cat swipe
    reduced_aoe_targets = p->apply_override( p->spec.swipe, p->spec.cat_form_override )->effectN( 4 ).base_value();

    if ( p->specialization() == DRUID_GUARDIAN )
      name_str_reporting = "swipe";
  }
};

// Thorns of Iron ===========================================================
struct thorns_of_iron_t : public bear_attack_t
{
  double mul;

  thorns_of_iron_t( druid_t* p )
    : bear_attack_t( "thorns_of_iron", p, find_trigger( p->talent.thorns_of_iron ).trigger() ),
      mul( find_trigger( p->talent.thorns_of_iron ).percent() )
  {
    background = proc = split_aoe_damage = true;
    aoe = -1;

    // 1 point to allow proper snapshot/update flag parsing
    base_dd_min = base_dd_max = 1.0;
  }

  double base_da_min( const action_state_t* ) const override
  {
    return p()->cache.armor() * mul;
  }

  double base_da_max( const action_state_t* ) const override
  {
    return p()->cache.armor() * mul;
  }

  double action_multiplier() const override
  {
    auto am = bear_attack_t::action_multiplier();
    auto lm = p()->buff.ironfur->check();

    if ( lm > 4 )
      am *= std::pow( 0.955, lm - 4 );  // approx. from testing

    return am;
  }
};

// Thrash (Bear) ============================================================
struct thrash_bear_t : public trigger_ursocs_fury_t<trigger_gore_t<bear_attack_t>>
{
  struct thrash_bear_dot_t : public trigger_ursocs_fury_t<trigger_waning_twilight_t<bear_attack_t>>
  {
    double bf_energize = 0.0;

    thrash_bear_dot_t( druid_t* p, std::string_view n ) : base_t( n, p, p->spec.thrash_bear_dot )
    {
      dual = background = true;

      dot_name = "thrash_bear";

      // energize amount is not stored in talent spell
      if ( p->talent.blood_frenzy.ok() )
        bf_energize = find_effect( p->find_spell( 203961 ), E_ENERGIZE ).resource( RESOURCE_RAGE );
    }

    void trigger_dot( action_state_t* s ) override
    {
      // existing cat thrash is cancelled only if it's shorter than bear thrash duration
      auto thrash_cat = td( s->target )->dots.thrash_cat;
      if ( thrash_cat->remains() < composite_dot_duration( s ) )
        thrash_cat->cancel();

      base_t::trigger_dot( s );
    }

    void tick( dot_t* d ) override
    {
      p()->resource_gain( RESOURCE_RAGE, bf_energize, p()->gain.blood_frenzy );

      // if both cat thrash and bear thrash is up (due to cat thrash having a longer duration) then bear thrash damage
      // is suppressed
      if ( td( d->target )->dots.thrash_cat->is_ticking() )
        return;

      base_t::tick( d );
    }
  };

  thrash_bear_t( druid_t* p, std::string_view opt )
    : thrash_bear_t( p, "thrash_bear", p->apply_override( p->talent.thrash, p->spec.bear_form_override ), opt )
  {}

  thrash_bear_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : base_t( n, p, s, opt )
  {
    aoe = -1;
    impact_action = p->get_secondary_action_n<thrash_bear_dot_t>( name_str + "_dot" );
    impact_action->stats = stats;
    track_cd_waste = true;

    dot_name = "thrash_bear";

    if ( p->specialization() == DRUID_GUARDIAN )
      name_str_reporting = "thrash";
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = base_t::composite_energize_amount( s );

    e *= 1.0 + p()->buff.furious_regeneration->check_value();

    return e;
  }

  void execute() override
  {
    base_t::execute();

    if ( p()->talent.flashing_claws.ok() && rng().roll( p()->talent.flashing_claws->effectN( 1 ).percent() ) )
      make_event( *sim, 500_ms, [ this ]() { p()->active.thrash_bear_flashing->execute_on_target( target ); } );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) )
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
    proc = true;
    aoe = 5;  // TODO: not in known spell data
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
    action_t* spring = nullptr;

    efflorescence_tick_t( druid_t* p ) : druid_heal_t( "efflorescence_tick", p, p->find_spell( 81269 ) )
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
    period = find_effect( efflo_data, A_PERIODIC_DUMMY ).period();

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
  elunes_favored_heal_t( druid_t* p )
    : druid_heal_t( "elunes_favored", p, p->find_spell( 370602 ) )
  {
    background = proc = true;
  }

  double base_da_min( const action_state_t* ) const override { return p()->buff.elunes_favored->check_value(); }
  double base_da_max( const action_state_t* ) const override { return p()->buff.elunes_favored->check_value(); }
};

// Frenzied Regeneration ====================================================
struct frenzied_regeneration_t : public druid_heal_t
{
  cooldown_t* dummy_cd;
  cooldown_t* orig_cd;
  double goe_mul = 0.0;
  double ir_mul = 0.0;

  frenzied_regeneration_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "frenzied_regeneration", p, p->talent.frenzied_regeneration, opt ),
      dummy_cd( p->get_cooldown( "dummy_cd" ) ),
      orig_cd( cooldown )
  {
    target = p;

    if ( p->talent.guardian_of_elune.ok() )
      goe_mul = p->buff.guardian_of_elune->data().effectN( 2 ).percent();

    if ( p->is_ptr() )
      ir_mul = p->talent.innate_resolve->effectN( 1 ).percent();
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

    // assume the innate resolve multiplier is snapshot
    pm *= 1.0 + p()->health_percentage() * ir_mul;

    return pm;
  }
};

// Furious Regeneration (Guardian Tier 30 2pc) ==============================
struct furious_regeneration_t : public residual_action::residual_periodic_action_t<druid_heal_t>
{
  furious_regeneration_t( druid_t* p ) : residual_action_t( "furious_regeneration", p, p->spec.furious_regeneration )
  {
    proc = true;
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
      hots.cenarion_ward->adjust_duration( ext );
      hots.cultivation->adjust_duration( ext );
      hots.germination->adjust_duration( ext );
      hots.lifebloom->adjust_duration( ext );
      hots.regrowth->adjust_duration( ext );
      hots.rejuvenation->adjust_duration( ext );
      hots.spring_blossoms->adjust_duration( ext );
      hots.wild_growth->adjust_duration( ext );
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
    snapshot_flags &= STATE_NO_MULTIPLIER;
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
  struct protector_of_the_pack_regrowth_t : public druid_heal_t
  {
    protector_of_the_pack_regrowth_t( druid_t* p ) : druid_heal_t( "protector_of_the_pack_regrowth", p, p->find_spell( 400204 ) )
    {
      // 1 point to allow proper snapshot/update flag parsing
      base_dd_min = base_dd_max = 1.0;
      proc = true;

      name_str_reporting = "protector_of_the_pack";
    }

    double base_da_min( const action_state_t* ) const override
    {
      return p()->buff.protector_of_the_pack->check_value();
    }

    double base_da_max( const action_state_t* ) const override
    {
      return p()->buff.protector_of_the_pack->check_value();
    }
  };

  action_t* potp = nullptr;
  timespan_t gcd_add;
  double bonus_crit;
  double sotf_mul;
  bool is_direct = false;

  regrowth_t( druid_t* p, std::string_view opt ) : regrowth_t( p, "regrowth", opt ) {}

  regrowth_t( druid_t* p, std::string_view n, std::string_view opt )
    : druid_heal_t( n, p, p->find_class_spell( "Regrowth" ), opt ),
      gcd_add( find_effect( p->spec.cat_form_passive, this, A_ADD_FLAT_MODIFIER, P_GCD ).time_value() ),
      bonus_crit( p->talent.improved_regrowth->effectN( 1 ).percent() ),
      sotf_mul( p->buff.soul_of_the_forest_tree->data().effectN( 1 ).percent() )
  {
    form_mask = NO_FORM | MOONKIN_FORM;
    may_autounshift = true;

    affected_by.soul_of_the_forest = true;

    if ( p->specialization() != DRUID_RESTORATION &&  p->talent.protector_of_the_pack.ok() )
    {
      potp = p->get_secondary_action<protector_of_the_pack_regrowth_t>( "protector_of_the_pack_regrowth" );
      add_child( potp );
    }
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

    if ( t == player && !p()->is_ptr() )
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

    p()->buff.forestwalk->trigger();
    p()->buff.predatory_swiftness->expire();

    if ( target == p() )
      p()->buff.protective_growth->trigger();

    if ( potp && p()->buff.protector_of_the_pack->check() )
    {
      potp->execute_on_target( target );
      p()->buff.protector_of_the_pack->expire();
    }

    if ( is_free() )
      return;

    p()->buff.dream_of_cenarius->expire();
    p()->buff.natures_swiftness->expire();
  }

  void last_tick( dot_t* d ) override
  {
    druid_heal_t::last_tick( d );

    if ( d->target == p() )
      p()->buff.protective_growth->expire();
  }
};

// Rejuvenation =============================================================
struct rejuvenation_base_t : public trigger_deep_focus_t<druid_heal_t>
{
  struct cultivation_t : public druid_heal_t
  {
    cultivation_t( druid_t* p ) : druid_heal_t( "cultivation", p, p->find_spell( 200389 ) ) {}
  };

  action_t* cult_hot = nullptr;
  double cult_pct;  // NOTE: this is base_value, NOT percent()
  double sotf_mul;

  rejuvenation_base_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt )
    : base_t( n, p, s, opt ),
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
    auto pm = base_t::composite_persistent_multiplier( s );

    if ( p()->buff.soul_of_the_forest_tree->check() )
      pm *= 1.0 + sotf_mul;

    return pm;
  }

  void impact( action_state_t* s ) override
  {
    if ( !td( target )->hots.rejuvenation->is_ticking() )
      p()->buff.abundance->increment();

    base_t::impact( s );
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( cult_hot && d->target->health_percentage() <= cult_pct )
      cult_hot->execute_on_target( d->target );
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );

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

  action_t* germination = nullptr;

  rejuvenation_t( druid_t* p, std::string_view opt ) : rejuvenation_t( p, "rejuvenation", p->talent.rejuvenation, opt )
  {}

  rejuvenation_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : rejuvenation_base_t( n, p, s, opt )
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
  double mul;

  renewal_t( druid_t* p, std::string_view opt )
    : druid_heal_t( "renewal", p, p->talent.renewal, opt ), mul( find_effect( this, E_HEAL_PCT ).percent() )
  {}

  void execute() override
  {
    base_dd_min = p()->resources.max[ RESOURCE_HEALTH ] * mul;

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
      : druid_heal_t( "tranquility_tick", p, find_trigger( p->talent.tranquility ).trigger() )
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
    parse_effect_modifiers( p->talent.improved_wild_growth );
    aoe = as<int>( modified_effect( 2 ).base_value() );

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
  double mul;

  yseras_gift_t( druid_t* p )
    : druid_heal_t( "yseras_gift", p, p->find_spell( 145109 ) ), mul( p->talent.yseras_gift->effectN( 1 ).percent() )
  {
    background = dual = true;
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_VERSATILITY;  // Is not affected by versatility.
  }

  double bonus_da( const action_state_t* ) const override
  {
    return p()->resources.max[ RESOURCE_HEALTH ] * mul;
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

template <class Base>
void druid_action_t<Base>::schedule_execute( action_state_t* s )
{
  check_autoshift();

  ab::schedule_execute( s );
}

template <class Base>
void druid_action_t<Base>::execute()
{
  ab::execute();

  if ( break_stealth )
  {
    p()->buff.prowl->expire();
    p()->buffs.shadowmeld->expire();
  }
}

template <class Base>
void druid_action_t<Base>::impact( action_state_t* s )
{
  ab::impact( s );

  if ( p()->active.moonless_night && s->result_amount > 0 && can_proc_moonless_night() )
  {
    auto moonless = debug_cast<druid_residual_action_t<bear_attacks::bear_attack_t>*>( p()->active.moonless_night );

    moonless->snapshot_and_execute( s, false, [ moonless ]( const action_state_t* from, action_state_t* to ) {
      moonless->set_amount( to, from->result_amount );
    } );
  }
}

namespace spells
{
// ==========================================================================
// Druid Spells
// ==========================================================================

// Adaptive Swarm ===========================================================
struct adaptive_swarm_t : public druid_spell_t
{
  struct adaptive_swarm_state_t : public action_state_t
  {
    double range = 0.0;
    int stacks = 0;
    bool jump = false;

    adaptive_swarm_state_t( action_t* a, player_t* t ) : action_state_t( a, t ) {}

    void initialize() override
    {
      action_state_t::initialize();

      range = 0.0;
      stacks = as<int>( static_cast<druid_t*>( action->player )->talent.adaptive_swarm->effectN( 1 ).base_value() );
      jump = false;
    }

    void copy_state( const action_state_t* s ) override
    {
      action_state_t::copy_state( s );
      auto s_ = static_cast<const adaptive_swarm_state_t*>( s );

      range = s_->range;
      stacks = s_->stacks;
      jump = s_->jump;
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

  template <typename BASE, typename OTHER>
  struct adaptive_swarm_base_t : public BASE
  {
    adaptive_swarm_base_t<OTHER, BASE>* other = nullptr;
    double tf_mul;
    bool heal = false;

    adaptive_swarm_base_t( druid_t* p, std::string_view n, const spell_data_t* s )
      : BASE( n, p, s ),
        tf_mul( find_effect( p->talent.tigers_fury, this, A_ADD_PCT_MODIFIER, P_TICK_DAMAGE ).percent() )
    {
      BASE::dual = BASE::background = true;
      BASE::dot_behavior = dot_behavior_e::DOT_CLIP;
    }

    action_state_t* new_state() override
    { return new adaptive_swarm_state_t( this, BASE::target ); }

    adaptive_swarm_state_t* state( action_state_t* s )
    { return static_cast<adaptive_swarm_state_t*>( s ); }

    const adaptive_swarm_state_t* state( const action_state_t* s ) const
    { return static_cast<const adaptive_swarm_state_t*>( s ); }

    timespan_t travel_time() const override
    {
      auto s_ = state( BASE::execute_state );

      if ( s_->jump )
        return timespan_t::from_seconds( s_->range / BASE::travel_speed );

      if ( BASE::target == BASE::player )
        return 0_ms;

      return BASE::travel_time();
    }

    virtual player_t* new_swarm_target( player_t* = nullptr ) const = 0;

    virtual double new_swarm_range( const action_state_t*, player_t* ) const = 0;

    void send_swarm( player_t* t, dot_t* d )
    {
      auto new_state = state( BASE::get_state() );

      new_state->target = t;
      new_state->range = new_swarm_range( d->state, t );
      new_state->stacks = d->current_stack() - 1;
      new_state->jump = true;

      BASE::sim->print_log( "ADAPTIVE_SWARM: jumping {} {} stacks to {} ({} yd)", new_state->stacks,
                            heal ? "heal" : "damage", new_state->target->name(), new_state->range );

      BASE::schedule_execute( new_state );
    }

    void jump_swarm( dot_t* d )
    {
      auto new_target = other->new_swarm_target();
      if ( !new_target )
      {
        new_target = new_swarm_target();
        assert( new_target );
        send_swarm( new_target, d );
      }
      else
      {
        other->send_swarm( new_target, d );
      }

      if ( BASE::p()->talent.unbridled_swarm.ok() &&
           BASE::rng().roll( BASE::p()->talent.unbridled_swarm->effectN( 1 ).percent() ) )
      {
        auto second_target = other->new_swarm_target( new_target );
        if ( !second_target )
        {
          second_target = new_swarm_target( new_target );
          if ( !second_target )
            return;

          BASE::sim->print_log( "ADAPTIVE_SWARM: splitting off {} swarm", heal ? "heal" : "damage" );
          send_swarm( second_target, d );
        }
        else
        {
          BASE::sim->print_log( "ADAPTIVE_SWARM: splitting off {} swarm", other->heal ? "heal" : "damage" );
          other->send_swarm( second_target, d );
        }
      }
    }

    void impact( action_state_t* s ) override
    {
      auto incoming = state( s )->stacks;
      auto existing = BASE::get_dot( s->target )->current_stack();

      BASE::impact( s );

      auto d = BASE::get_dot( s->target );
      d->increment( incoming + existing - 1 );

      BASE::sim->print_log( "ADAPTIVE_SWARM: {} incoming:{} existing:{} final:{}", heal ? "heal" : "damage", incoming,
                            existing, d->current_stack() );
    }

    void last_tick( dot_t* d ) override
    {
      BASE::last_tick( d );

      // last_tick() is called via dot_t::cancel() when a new swarm CLIPs an existing swarm. As dot_t::last_tick() is
      // called before dot_t::reset(), check the remaining time on the dot to see if we're dealing with a true last_tick
      // (and thus need to check for a new jump) or just a refresh.

      // NOTE: if demise() is called on the target d->remains() has time left, so assume that a target that is sleeping
      // with time remaining has been demised()'d and jump to a new target
      if ( d->remains() > 0_ms && !d->target->is_sleeping() )
        return;

      if ( d->current_stack() > 1 )
        jump_swarm( d );
      else
        BASE::sim->print_log( "ADAPTIVE_SWARM: {} expiring on final stack", heal ? "heal" : "damage" );
    }

    double calculate_tick_amount( action_state_t* s, double m ) const override
    {
      auto stack = BASE::find_dot( s->target )->current_stack();

      return BASE::calculate_tick_amount( s, m / stack );
    }
  };

  using damage_swarm_t = adaptive_swarm_base_t<druid_spell_t, heals::druid_heal_t>;
  using healing_swarm_t = adaptive_swarm_base_t<heals::druid_heal_t, druid_spell_t>;

  struct adaptive_swarm_damage_t : public damage_swarm_t
  {
    adaptive_swarm_damage_t( druid_t* p ) : damage_swarm_t( p, "adaptive_swarm_damage", p->spec.adaptive_swarm_damage )
    {}

    bool dots_ticking( const druid_td_t* td ) const
    {
      return td->dots.moonfire->is_ticking() || td->dots.rake->is_ticking() || td->dots.rip->is_ticking() ||
             td->dots.stellar_flare->is_ticking() || td->dots.sunfire->is_ticking() ||
             td->dots.thrash_bear->is_ticking() || td->dots.thrash_cat->is_ticking();
    }

    player_t* new_swarm_target( player_t* exclude ) const override
    {
      auto tl = target_list();

      // because action_t::available_targets() explicitly adds the current action_t::target to the target_cache, we need
      // to explicitly remove it here as swarm should not pick an invulnerable target whenignore_invulnerable_targets is
      // enabled.
      if ( sim->ignore_invulnerable_targets && target->debuffs.invulnerable->check() )
        tl.erase( std::remove( tl.begin(), tl.end(), target ), tl.end() );

      if ( exclude )
        tl.erase( std::remove( tl.begin(), tl.end(), exclude ), tl.end() );

      if ( tl.empty() )
        return nullptr;

      if ( tl.size() > 1 )
      {
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
      }

      return tl.front();
    }

    double new_swarm_range( const action_state_t* s, player_t* ) const override
    {
      return state( s )->range;
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      double pm = adaptive_swarm_base_t::composite_persistent_multiplier( s );

      // inherits from druid_spell_t so does not get automatic persistent multiplier parsing
      if ( !state( s )->jump && p()->buff.tigers_fury->up() )
        pm *= 1.0 + tf_mul;

      return pm;
    }
  };

  struct adaptive_swarm_heal_t : public healing_swarm_t
  {
    adaptive_swarm_heal_t( druid_t* p ) : healing_swarm_t( p, "adaptive_swarm_heal", p->spec.adaptive_swarm_heal )
    {
      quiet = heal = true;
      harmful = may_miss = false;

      parse_effect_period( data().effectN( 1 ) );
    }

    player_t* new_swarm_target( player_t* exclude ) const override
    {
      auto tl = target_list();

      if ( exclude )
        tl.erase( std::remove( tl.begin(), tl.end(), exclude ), tl.end() );

      if ( tl.empty() )
        return nullptr;

      if ( tl.size() > 1 )
      {
        rng().shuffle( tl.begin(), tl.end() );

        // prioritize unswarmed over swarmed
        range::partition( tl, [ this ]( player_t* t ) {
          return !p()->get_target_data( t )->hots.adaptive_swarm_heal->is_ticking();
        } );
      }

      return tl.front();
    }

    double new_swarm_range( const action_state_t*, player_t* t ) const override
    {
      double dis = 0.0;

      if ( t->role == ROLE_ATTACK )
        dis = p()->options.adaptive_swarm_jump_distance_melee;
      else if ( t->role == ROLE_SPELL )
        dis = p()->options.adaptive_swarm_jump_distance_ranged;

      return rng().gauss( dis, p()->options.adaptive_swarm_jump_distance_stddev, true );
    }
  };

  adaptive_swarm_damage_t* damage;
  adaptive_swarm_heal_t* heal;
  timespan_t gcd_add;
  bool target_self = false;

  adaptive_swarm_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "adaptive_swarm", p, p->talent.adaptive_swarm ),
      gcd_add( find_effect( p->spec.cat_form_passive, this, A_ADD_FLAT_LABEL_MODIFIER, P_GCD ).time_value() )
  {
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
    : druid_spell_t( "astral_communion", p, p->spec.astral_communion, opt )
  {
    harmful = false;
  }
};

// Astral Smolder ===========================================================
struct astral_smolder_t
  : public residual_action::residual_periodic_action_t<trigger_waning_twilight_t<druid_spell_t>>
{
  astral_smolder_t( druid_t* p ) : residual_action_t( "astral_smolder", p, p->find_spell( 394061 ) )
  {
    proc = true;
  }

  void trigger_dot( action_state_t* s ) override
  {
    residual_action_t::trigger_dot( s );

    p()->uptime.astral_smolder->update( true, sim->current_time() );
  }

  void last_tick( dot_t* d ) override
  {
    residual_action_t::last_tick( d );

    p()->uptime.astral_smolder->update( false, sim->current_time() );
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

    p()->buff.matted_fur->trigger();
  }
};

// Celestial Alignment ======================================================
struct celestial_alignment_base_t : public druid_spell_t
{
  buff_t* buff;

  celestial_alignment_base_t( std::string_view n, druid_t* p, const spell_data_t* s, std::string_view opt )
    : druid_spell_t( n, p, p->apply_override( s, p->talent.orbital_strike ), opt ), buff( p->buff.celestial_alignment )
  {
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    buff->trigger();

    p()->uptime.primordial_arcanic_pulsar->update( false, sim->current_time() );
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
    buff = p->buff.incarnation_moonkin;
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
      gcd_mul( find_effect( p->buff.cat_form, this, A_ADD_PCT_MODIFIER, P_GCD ).percent() )
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
      gcd_add( find_effect( p->spec.cat_form_passive, this, A_ADD_FLAT_MODIFIER, P_GCD ).time_value() )
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
    return p()->buff.predatory_swiftness->check() || druid_spell_t::check_form_restriction();
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
      summon_duration( find_trigger( p->talent.force_of_nature ).trigger()->duration() + 1_ms )
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
      tick_period( find_effect( b, A_PERIODIC_ENERGIZE, RESOURCE_ASTRAL_POWER ).period() )
  {
    track_cd_waste = true;

    form_mask |= NO_FORM; // can be cast without form
    dot_duration = 0_ms;  // AP gain handled via buffs

    damage = p->get_secondary_action_n<fury_of_elune_tick_t>( name_str + "_tick", s_damage );
    damage->stats = stats;
  }

  // needed to allow on-cast procs
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
  }

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

// Lunar Beam ===============================================================
struct lunar_beam_t : public druid_spell_t
{
  struct lunar_beam_heal_t : public heals::druid_heal_t
  {
    lunar_beam_heal_t( druid_t* p ) : druid_heal_t( "lunar_beam_heal", p, p->find_spell( 204069 ) )
    {
      background = true;
      name_str_reporting = "Heal";

      target = p;
    }
  };

  struct lunar_beam_tick_t : public druid_spell_t
  {
    lunar_beam_tick_t( druid_t* p )
      : druid_spell_t( "lunar_beam_tick", p, p->find_spell( p->is_ptr() ? 414613 : 204069 ) )
    {
      background = dual = ground_aoe = true;
      aoe = -1;

      execute_action = p->get_secondary_action<lunar_beam_heal_t>( "lunar_beam_heal" );
    }
  };

  action_t* damage;

  lunar_beam_t( druid_t* p, std::string_view opt ) : druid_spell_t( "lunar_beam", p, p->talent.lunar_beam, opt )
  {
    damage = p->get_secondary_action<lunar_beam_tick_t>( "lunar_beam_tick" );
    damage->stats = stats;
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override { return damage->has_amount_result(); }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p()->is_ptr() )
      p()->buff.lunar_beam->trigger();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( target )
      .x( p()->x_position )
      .y( p()->y_position )
      .pulse_time( 1_s )
      .duration( data().duration() )
      .action( damage ) );
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
    {
      cooldown = p()->cooldown.moon_cd;
      track_cd_waste = true;
    }

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
  struct moonfire_damage_t
    : public trigger_deep_focus_t<trigger_gore_t<trigger_shooting_stars_t<trigger_waning_twilight_t<druid_spell_t>>>>
  {
    struct protector_of_the_pack_moonfire_t : public druid_spell_t
    {
      protector_of_the_pack_moonfire_t( druid_t* p )
        : druid_spell_t( "protector_of_the_pack_moonfire", p, p->find_spell( 400202 ) )
      {
        // 1 point to allow proper snapshot/update flag parsing
        base_dd_min = base_dd_max = 1.0;
        proc = true;

        name_str_reporting = "protector_of_the_pack";
      }

      double base_da_min( const action_state_t* ) const override
      {
        return p()->buff.protector_of_the_pack->check_value();
      }

      double base_da_max( const action_state_t* ) const override
      {
        return p()->buff.protector_of_the_pack->check_value();
      }
    };

    action_t* potp = nullptr;
    double gg_mul = 0.0;
    double feral_override_da = 0.0;
    double feral_override_ta = 0.0;

    moonfire_damage_t( druid_t* p, std::string_view n ) : base_t( n, p, p->spec.moonfire_dmg )
    {
      may_miss = false;
      dual = background = true;

      dot_name = "moonfire";
      dot_list = &p->dot_list.moonfire;

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

      if ( p->talent.galactic_guardian.ok() )
        gg_mul = p->buff.galactic_guardian->data().effectN( 3 ).percent();

      // Always applies when you are a feral druid unless you go into moonkin form
      if ( p->specialization() == DRUID_FERAL )
      {
        feral_override_da = find_effect( p->spec.feral_overrides, this, A_ADD_PCT_MODIFIER, P_GENERIC ).percent();
        feral_override_ta = find_effect( p->spec.feral_overrides, this, A_ADD_PCT_MODIFIER, P_TICK_DAMAGE ).percent();
      }

      if ( p->specialization() == DRUID_RESTORATION && p->talent.protector_of_the_pack.ok() )
      {
        potp = p->get_secondary_action<protector_of_the_pack_moonfire_t>( "protector_of_the_pack_moonfire" );
        add_child( potp );
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double dam = base_t::composite_da_multiplier( s );

      // MF proc'd by gg is not affected by any existing gg buff.
      if ( ( !is_free( free_spell_e::GALACTIC ) || !p()->is_ptr() ) && p()->buff.galactic_guardian->check() )
        dam *= 1.0 + gg_mul;

      if ( feral_override_da && !p()->buff.moonkin_form->check() )
        dam *= 1.0 + feral_override_da;

      return dam;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double tam = base_t::composite_ta_multiplier( s );

      if ( feral_override_ta && !p()->buff.moonkin_form->check() )
        tam *= 1.0 + feral_override_ta;

      // on PTR rage of the sleeper applies to ticks via hidden script, so we manually adjust here
      if ( p()->is_ptr() && p()->buff.rage_of_the_sleeper->check() )
        tam *= 1.0 + p()->talent.rage_of_the_sleeper->effectN( 5 ).percent();

      return tam;
    }

    std::vector<player_t*>& target_list() const override
    {
      auto& tl = base_t::target_list();

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

    void execute() override
    {
      // Force invalidate target cache so that it will impact on the correct targets.
      target_cache.is_valid = false;

      base_t::execute();

      if ( hit_any_target )
      {
        if ( potp && p()->buff.protector_of_the_pack->check() )
        {
          potp->execute_on_target( target );
          p()->buff.protector_of_the_pack->expire();
        }

        if ( !is_free( free_spell_e::GALACTIC ) || !p()->is_ptr() )
        {
          auto rage = p()->buff.galactic_guardian->check_value() * num_targets_hit;
          p()->resource_gain( RESOURCE_RAGE, rage, gain );
        }

        if ( !is_free() )
          p()->buff.galactic_guardian->expire();
      }
    }

    void trigger_dot( action_state_t* s ) override
    {
      base_t::trigger_dot( s );

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
        return;

      base_t::tick( d );
    }
  };

  moonfire_damage_t* damage;  // Add damage modifiers in moonfire_damage_t, not moonfire_t

  moonfire_t( druid_t* p, std::string_view opt ) : moonfire_t( p, "moonfire", opt ) {}

  moonfire_t( druid_t* p, std::string_view n, std::string_view opt )
    : druid_spell_t( n, p, p->spec.moonfire, opt )
  {
    damage = p->get_secondary_action_n<moonfire_damage_t>( name_str + "_dmg" );
    damage->stats = stats;

    if ( p->spec.astral_power->ok() )
    {
      parse_effect_modifiers( p->spec.astral_power );
      energize_resource = RESOURCE_ASTRAL_POWER;
      energize_amount =
          modified_effect( find_effect_index( this, E_ENERGIZE, A_MAX, RESOURCE_ASTRAL_POWER ) ).resource( RESOURCE_ASTRAL_POWER );
    }
    else
    {
      energize_type = action_energize::NONE;
    }
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override { return damage->has_amount_result(); }

  // force invalidate damage target cache on target change. baseline code always invalidate child_action target caches,
  // but we only need to do this on proxy action target change.
  bool select_target() override
  {
    auto old_target = target;
    auto ret = druid_spell_t::select_target();

    if ( ret && old_target != target && damage->is_aoe() )
      damage->target_cache.is_valid = false;

    return ret;
  }

  void init() override
  {
    druid_spell_t::init();

    damage->gain = gain;
  }

  void execute() override
  {
    p()->cache.invalidate( CACHE_MASTERY );
    p()->snapshot_mastery();

    druid_spell_t::execute();

    damage->target = target;
    damage->schedule_execute();
  }

  void gain_energize_resource( resource_e rt, double amt, gain_t* gain ) override
  {
    base_t::gain_energize_resource( rt, amt, gain );

    if ( p()->talent.stellar_innervation.ok() && p()->buff.eclipse_lunar->check() )
      p()->resource_gain( rt, amt, p()->gain.stellar_innervation, this );
  }
};

// Nature's Vigil =============================================================
template <class Base>
struct natures_vigil_t : public Base
{
  struct natures_vigil_tick_t : public Base
  {
    natures_vigil_tick_t( druid_t* p )
      : Base( "natures_vigil_tick", p, p->find_spell( p->specialization() == DRUID_RESTORATION ? 124988 : 124991 ) )
    {
      Base::dual = Base::proc = true;
    }

    double base_da_min( const action_state_t* ) const override
    {
      return Base::p()->buff.natures_vigil->check_value();
    }

    double base_da_max( const action_state_t* ) const override
    {
      return Base::p()->buff.natures_vigil->check_value();
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
        {
          tick->execute();
          b->current_value = 0;
        }
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

    p()->cancel_auto_attacks();

    if ( !p()->in_boss_encounter )
      p()->leave_combat();
  }

  bool ready() override
  {
    if ( p()->buff.prowl->check() || ( p()->in_combat && !p()->buff.incarnation_cat_prowl->check() ) )
      return false;

    return druid_spell_t::ready();
  }
};

// Proxy Swipe Spell ========================================================
struct swipe_proxy_t : public druid_spell_t
{
  action_t* swipe_cat;
  action_t* swipe_bear;

  swipe_proxy_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "swipe", p, p->spec.swipe, opt ),
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

// Shooting Stars ===========================================================
struct shooting_stars_t : public druid_spell_t
{
  buff_t* buff;

  shooting_stars_t( druid_t* p, std::string_view n, const spell_data_t* s )
    : druid_spell_t( n, p, s ), buff( p->buff.orbit_breaker )
  {
    background = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    buff->trigger();

    if ( buff->at_max_stacks() )
    {
      p()->active.orbit_breaker->execute_on_target( target );
      buff->expire();
    }
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
struct starfall_t : public astral_power_spender_t
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

  using starfall_state_t = druid_action_state_t<starfall_data_t>;

  // TODO: remove in 10.1.5
  struct lunar_shrapnel_t : public druid_residual_action_t<druid_spell_t, starfall_data_t>
  {
    lunar_shrapnel_t( druid_t* p, std::string_view n ) : base_t( n, p, p->find_spell( 393869 ) )
    {
      aoe = -1;
      reduced_aoe_targets = p->talent.lunar_shrapnel->effectN( 2 ).base_value();
      name_str_reporting = "lunar_shrapnel";

      residual_mul = p->talent.lunar_shrapnel->effectN( 1 ).percent();
    }
  };

  struct starfall_base_t : public druid_spell_t
  {
    starfall_base_t( std::string_view n, druid_t* p, const spell_data_t* s ) : druid_spell_t( n, p, s ) {}

    action_state_t* new_state() override
    {
      return new starfall_state_t( this, target );
    }

    starfall_state_t* cast_state( action_state_t* s )
    {
      return static_cast<starfall_state_t*>( s );
    }
  };

  struct starfall_damage_t : public starfall_base_t
  {
    lunar_shrapnel_t* shrapnel = nullptr;  // TODO: remove in 10.1.5
    double cosmos_mul;

    starfall_damage_t( druid_t* p, std::string_view n )
      : starfall_base_t( n, p, p->find_spell( 191037 ) ),
        cosmos_mul( p->sets->set( DRUID_BALANCE, T29, B4 )->effectN( 1 ).trigger()->effectN( 2 ).percent() )
    {
      background = dual = true;

      if ( !p->is_ptr() && p->talent.lunar_shrapnel.ok() )
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
        shrapnel->snapshot_and_execute( s, false, [ this ]( const action_state_t* from, action_state_t* to ) {
          shrapnel->set_amount( to, from->result_amount );
        } );
      }
    }
  };

  struct starfall_driver_t : public starfall_base_t
  {
    starfall_damage_t* damage;
    double shrapnel_chance = 0.0;  // TODO: remove in 10.1.5

    starfall_driver_t( druid_t* p, std::string_view n )
      : starfall_base_t( n, p, find_trigger( p->buff.starfall ).trigger() )
    {
      background = dual = true;

      auto pre = name_str.substr( 0, name_str.find_last_of( '_' ) );
      damage = p->get_secondary_action_n<starfall_damage_t>( pre + "_damage" );

      if ( !p->is_ptr() && p->talent.lunar_shrapnel.ok() )
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
      if ( p()->is_ptr() ) return;

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

      damage->snapshot_and_execute( s, false, []( const action_state_t* from, action_state_t* to ) {
        to->copy_state( from );
      } );
    }
  };

  starfall_driver_t* driver;
  timespan_t dot_ext;
  timespan_t max_ext;

  starfall_t( druid_t* p, std::string_view opt ) : starfall_t( p, "starfall", p->spec.starfall, opt ) {}

  starfall_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : base_t( n, p, s, opt ),
      dot_ext( timespan_t::from_seconds( p->talent.aetherial_kindling->effectN( 1 ).base_value() ) ),
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

    if ( !p->is_ptr() && p->talent.lunar_shrapnel.ok() )
      add_child( driver->damage->shrapnel );
  }

  void execute() override
  {
    if ( !is_free() && p()->buff.starweavers_warp->up() && p()->active.starfall_starweaver )
    {
      p()->active.starfall_starweaver->execute_on_target( target );
      p()->buff.starweavers_warp->expire();
      return;
    }

    if ( p()->talent.aetherial_kindling.ok() )
    {
      for ( auto t : p()->sim->target_non_sleeping_list )
      {
        td( t )->dots.moonfire->adjust_duration( dot_ext, max_ext );
        td( t )->dots.sunfire->adjust_duration( dot_ext, max_ext );
      }
    }

    base_t::execute();

    p()->buff.starfall->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      driver->execute();
      p()->eclipse_handler.tick_starfall();
    } );

    p()->buff.starfall->trigger();

    if ( p()->buff.touch_the_cosmos->check() )
      p()->buff.touch_the_cosmos_starfall->trigger();

    post_execute();

    if ( !is_free_proc() )
      p()->buff.starweavers_weft->trigger();
  }
};

// Starfire =============================================================
struct starfire_t : public trigger_astral_smolder_t<consume_umbral_embrace_t<druid_spell_t>>
{
  size_t aoe_idx;
  double aoe_base;
  double aoe_eclipse;
  double ap_base;
  double ap_woe;
  double smolder_mul;
  double sotf_mul;
  unsigned sotf_cap;

  starfire_t( druid_t* p, std::string_view opt )
    : base_t( "starfire", p, p->talent.starfire, opt ),
      aoe_idx( p->specialization() == DRUID_BALANCE ? 3 : 2 ),
      aoe_base( data().effectN( p->specialization() == DRUID_BALANCE ? 3 : 2 ).percent() ),
      smolder_mul( p->talent.astral_smolder->effectN( 1 ).percent() ),
      sotf_mul( p->talent.soul_of_the_forest_moonkin->effectN( 2 ).percent() ),
      sotf_cap( as<unsigned>( p->talent.soul_of_the_forest_moonkin->effectN( 3 ).base_value() ) )
  {
    aoe = -1;
    reduced_aoe_targets = p->is_ptr() ? data().effectN( p->specialization() == DRUID_BALANCE ? 5 : 3 ).base_value() : 8;

    init_umbral_embrace( p->spec.eclipse_solar, &druid_td_t::dots_t::sunfire, p->spec.sunfire_dmg );
    init_astral_smolder( p->buff.eclipse_solar, &druid_td_t::dots_t::sunfire );

    parse_effect_modifiers( p->spec.eclipse_lunar, p->talent.umbral_intensity );
    aoe_base = data().effectN( aoe_idx ).percent();
    aoe_eclipse = modified_effect( aoe_idx ).percent();

    parse_effect_modifiers( p->talent.wild_surges );
    ap_base = modified_effect( find_effect_index( this, E_ENERGIZE ) ).resource( RESOURCE_ASTRAL_POWER );

    parse_effect_modifiers( p->talent.warrior_of_elune );
    ap_woe = modified_effect( find_effect_index( this, E_ENERGIZE ) ).resource( RESOURCE_ASTRAL_POWER );
  }

  void init_finished() override
  {
    base_t::init_finished();

    // for precombat we hack it to manually energize 100ms later to get around AP capping on combat start
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER )
      energize_type = action_energize::NONE;
  }

  double sotf_multiplier( const action_state_t* s ) const
  {
    auto mul = 1.0;

    if ( sotf_mul && p()->buff.eclipse_lunar->check() && s->n_targets > 1 )
      mul += sotf_mul * std::min( sotf_cap, s->n_targets - 1 );

    return mul;
  }

  // WARNING: this overrides any adjustments done in parent classes
  double composite_energize_amount( const action_state_t* s ) const override
  {
    return ( p()->buff.warrior_of_elune->check() ? ap_woe : ap_base ) * sotf_multiplier( s );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = base_t::composite_da_multiplier( s );

    da *= sotf_multiplier( s );

    return da;
  }

  void execute() override
  {
    base_t::execute();

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

    p()->buff.gathering_starstuff->expire();

    if ( is_free_proc() )
      return;

    p()->eclipse_handler.cast_starfire();
  }

  // TODO: we do this all in composite_aoe_multiplier() as base_aoe_multiplier is not a virtual function. If necessary
  // in the future, base_aoe_multiplier may need to be made into one.
  double composite_aoe_multiplier( const action_state_t* s ) const override
  {
    double cam = base_t::composite_aoe_multiplier( s );

    if ( s->chain_target )
      cam *= p()->buff.eclipse_lunar->check() ? aoe_eclipse : aoe_base;

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

struct starsurge_t : public astral_power_spender_t
{
  struct goldrinns_fang_t : public druid_spell_t
  {
    goldrinns_fang_t( druid_t* p, std::string_view n )
      : druid_spell_t( n, p, find_trigger( p->talent.power_of_goldrinn ).trigger() )
    {
      background = true;
      name_str_reporting = "goldrinns_fang";

      force_buff_effect( p->buff.eclipse_lunar, 1 );
      force_buff_effect( p->buff.eclipse_solar, 1 );
      force_dot_effect( &druid_td_t::dots_t::moonfire, p->spec.moonfire_dmg, 5, p->mastery.astral_invocation );
      force_dot_effect( &druid_td_t::dots_t::sunfire, p->spec.sunfire_dmg, 4, p->mastery.astral_invocation );
    }
  };

  action_t* goldrinn = nullptr;
  bool moonkin_form_in_precombat = false;

  starsurge_t( druid_t* p, std::string_view opt ) : starsurge_t( p, "starsurge", p->talent.starsurge, opt ) {}

  starsurge_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt ) : base_t( n, p, s, opt )
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

    base_t::init();

    if ( is_precombat )
    {
      moonkin_form_in_precombat = range::any_of( p()->precombat_action_list, []( action_t* a ) {
        return util::str_compare_ci( a->name(), "moonkin_form" );
      } );
    }
  }

  timespan_t travel_time() const override
  {
    return is_precombat ? 100_ms : base_t::travel_time();
  }

  bool ready() override
  {
    if ( !is_precombat )
      return base_t::ready();

    // in precombat, so hijack standard ready() procedure
    // emulate performing check_form_restriction()
    if ( !moonkin_form_in_precombat )
      return false;

    // emulate performing resource_available( current_resource(), cost() )
    if ( !p()->talent.natures_balance.ok() && p()->options.initial_astral_power < cost() )
      return false;

    return true;
  }

  void execute() override
  {
    if ( !is_free() && p()->buff.starweavers_weft->up() && p()->active.starsurge_starweaver )
    {
      p()->active.starsurge_starweaver->execute_on_target( target );
      p()->buff.starweavers_weft->expire();
      return;
    }

    base_t::execute();

    if ( goldrinn && rng().roll( p()->talent.power_of_goldrinn->proc_chance() ) )
      goldrinn->execute_on_target( target );

    post_execute();

    if ( !is_free_proc() )
    {
      p()->buff.starweavers_warp->trigger();
      p()->eclipse_handler.cast_starsurge();
    }
  }
};

// Stellar Flare ============================================================
struct stellar_flare_t : public trigger_waning_twilight_t<druid_spell_t>
{
  stellar_flare_t( druid_t* p, std::string_view opt )
    : stellar_flare_t( p, "stellar_flare", p->talent.stellar_flare, opt )
  {}

  stellar_flare_t( druid_t* p, std::string_view n, const spell_data_t* s, std::string_view opt )
    : base_t( n, p, s, opt )
  {
    dot_name = "stellar_flare";
  }
};

// Sunfire Spell ============================================================
struct sunfire_t : public druid_spell_t
{
  struct sunfire_damage_t : public trigger_shooting_stars_t<trigger_waning_twilight_t<druid_spell_t>>
  {
    sunfire_damage_t( druid_t* p ) : base_t( "sunfire_dmg", p, p->spec.sunfire_dmg )
    {
      dual = background = true;
      aoe = p->talent.improved_sunfire.ok() ? -1 : 0;
      base_aoe_multiplier = 0;

      dot_name = "sunfire";
      dot_list = &p->dot_list.sunfire;
    }
  };

  action_t* damage;  // Add damage modifiers in sunfire_damage_t, not sunfire_t

  sunfire_t( druid_t* p, std::string_view opt ) : druid_spell_t( "sunfire", p, p->talent.sunfire, opt )
  {
    may_miss = false;

    damage = p->get_secondary_action<sunfire_damage_t>( "sunfire_dmg" );
    damage->stats = stats;

    if ( p->spec.astral_power->ok() )
    {
      parse_effect_modifiers( p->spec.astral_power );
      energize_amount = modified_effect( find_effect_index( this, E_ENERGIZE ) ).resource( RESOURCE_ASTRAL_POWER );
    }
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override { return damage->has_amount_result(); }

  // force invalidate damage target cache on target change. baseline code always invalidate child_action target caches,
  // but we only need to do this on proxy action target change.
  bool select_target() override
  {
    auto old_target = target;
    auto ret = druid_spell_t::select_target();

    if ( ret && old_target != target && damage->is_aoe() )
      damage->target_cache.is_valid = false;

    return ret;
  }

  void gain_energize_resource( resource_e rt, double amt, gain_t* gain ) override
  {
    druid_spell_t::gain_energize_resource( rt, amt, gain );

    if ( p()->talent.stellar_innervation.ok() && p()->buff.eclipse_solar->check() )
      p()->resource_gain( rt, amt, p()->gain.stellar_innervation, this );
  }

  void execute() override
  {
    p()->cache.invalidate( CACHE_MASTERY );
    p()->snapshot_mastery();

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

    p()->buff.matted_fur->trigger();
  }
};

// Orbital Strike ===========================================================
struct orbital_strike_t : public druid_spell_t
{
  action_t* flare;

  orbital_strike_t( druid_t* p ) : druid_spell_t( "orbital_strike", p, p->find_spell( 361237 ) )
  {
    aoe = -1;
    travel_speed = 75.0;  // guesstimate

    flare = p->get_secondary_action_n<stellar_flare_t>( "stellar_flare_orbital_strike", p->find_spell( 202347 ), "" );
    flare->name_str_reporting = "stellar_flare";
    add_child( flare );

    force_buff_effect( p->buff.eclipse_lunar, 1 );
    force_buff_effect( p->buff.eclipse_solar, 1 );
    force_dot_effect( &druid_td_t::dots_t::moonfire, p->spec.moonfire_dmg, 5, p->mastery.astral_invocation );
    force_dot_effect( &druid_td_t::dots_t::sunfire, p->spec.sunfire_dmg, 4, p->mastery.astral_invocation );
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
    track_cd_waste = true;
  }

  timespan_t cooldown_duration() const override
  {
    if ( !p()->is_ptr() ) return 0_ms;  // cooldown handled by buff.warrior_of_elune
    return druid_spell_t::cooldown_duration();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.warrior_of_elune->trigger();
  }

  bool ready() override
  {
    if ( !p()->is_ptr() ) return p()->buff.warrior_of_elune->check() ? false : druid_spell_t::ready();
    return druid_spell_t::ready();
  }
};

// Wild Charge ==============================================================
struct wild_charge_t : public druid_spell_t
{
  double movement_speed_increase = 5.0;

  wild_charge_t( druid_t* p, std::string_view opt ) : druid_spell_t( "wild_charge", p, p->talent.wild_charge, opt )
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
  struct fungal_growth_t : public residual_action::residual_periodic_action_t<trigger_waning_twilight_t<druid_spell_t>>
  {
    fungal_growth_t( druid_t* p ) : residual_action_t( "fungal_growth", p, p->find_spell( 81281 ) ) {}
  };

  struct wild_mushroom_damage_t : public druid_spell_t
  {
    action_t* driver;
    action_t* fungal = nullptr;
    double fungal_mul;
    double ap_per = 5.0;  // not in spell data
    double ap_max;

    wild_mushroom_damage_t( druid_t* p, action_t* a )
      : druid_spell_t( "wild_mushroom_damage", p, find_trigger( p->talent.wild_mushroom ).trigger() ),
        driver( a ),
        fungal_mul( p->talent.fungal_growth->effectN( 1 ).percent() ),
        ap_max( data().effectN( 2 ).base_value() )
    {
      background = dual = true;
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
        residual_action::trigger( fungal, s->target, s->result_amount * fungal_mul );
    }
  };

  action_t* damage;
  timespan_t delay;

  wild_mushroom_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "wild_mushroom", p, p->talent.wild_mushroom, opt ),
      delay( timespan_t::from_millis( find_trigger( this ).misc_value1() ) )
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
struct wrath_t : public trigger_astral_smolder_t<consume_umbral_embrace_t<druid_spell_t>>
{
  double ap_base;
  double ap_eclipse;
  double gcd_mul;
  double smolder_mul;
  unsigned count = 0;

  wrath_t( druid_t* p, std::string_view opt ) : wrath_t( p, "wrath", opt ) {}

  wrath_t( druid_t* p, std::string_view n, std::string_view opt )
    : base_t( n, p, p->spec.wrath, opt ),
      gcd_mul( find_effect( p->spec.eclipse_solar, this, A_ADD_PCT_MODIFIER, P_GCD ).percent() ),
      smolder_mul( p->talent.astral_smolder->effectN( 1 ).percent() )
  {
    form_mask = NO_FORM | MOONKIN_FORM;

    init_umbral_embrace( p->spec.eclipse_lunar, &druid_td_t::dots_t::moonfire, p->spec.moonfire_dmg );
    init_astral_smolder( p->buff.eclipse_lunar, &druid_td_t::dots_t::moonfire );

    parse_effect_modifiers( p->spec.astral_power );
    parse_effect_modifiers( p->talent.wild_surges );
    ap_base = modified_effect( find_effect_index( this, E_ENERGIZE ) ).resource( RESOURCE_ASTRAL_POWER );

    parse_effect_modifiers( p->spec.eclipse_solar, p->talent.soul_of_the_forest_moonkin );
    ap_eclipse = modified_effect( find_effect_index( this, E_ENERGIZE ) ).resource( RESOURCE_ASTRAL_POWER );
  }

  // WARNING: this overrides any adjustments done in parent classes
  double composite_energize_amount( const action_state_t* ) const override
  {
    return p()->buff.eclipse_solar->check() ? ap_eclipse : ap_base;
  }

  timespan_t travel_time() const override
  {
    if ( !count )
      return base_t::travel_time();

    // for each additional wrath in precombat apl, reduce the travel time by the cast time
    player->invalidate_cache( CACHE_SPELL_HASTE );
    return std::max( 1_ms, base_t::travel_time() - base_execute_time * composite_haste() * count );
  }

  timespan_t gcd() const override
  {
    timespan_t g = base_t::gcd();

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

    return base_t::target_ready( t );
  }

  void execute() override
  {
    base_t::execute();

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
    // Heals
    action_t* conv_regrowth;
    action_t* conv_rejuvenation;
  } actions;

  std::vector<convoke_cast_e> cast_list;
  std::vector<convoke_cast_e> offspec_list;
  std::vector<std::pair<convoke_cast_e, double>> chances;

  shuffled_rng_t* deck = nullptr;

  int max_ticks = 0;
  unsigned main_count = 0;
  unsigned filler_count = 0;
  unsigned dot_count = 0;
  unsigned off_count = 0;
  bool guidance;

  convoke_the_spirits_t( druid_t* p, std::string_view opt )
    : druid_spell_t( "convoke_the_spirits", p, p->talent.convoke_the_spirits, opt ),
      actions(),
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
  }

  void init() override
  {
    druid_spell_t::init();

    using namespace bear_attacks;
    using namespace cat_attacks;
    using namespace heals;

    // Create actions used by all specs
    actions.conv_wrath        = get_convoke_action<wrath_t>( "wrath", "" );
    actions.conv_moonfire     = get_convoke_action<moonfire_t>( "moonfire", "" );
    actions.conv_rake         = get_convoke_action<rake_t>( "rake", p()->find_spell( 1822 ), "" );
    actions.conv_thrash_bear  = get_convoke_action<thrash_bear_t>( "thrash_bear", p()->find_spell( 77758 ), "" );
    actions.conv_regrowth     = get_convoke_action<regrowth_t>( "regrowth", "" );
    actions.conv_rejuvenation = get_convoke_action<rejuvenation_t>( "rejuvenation", p()->find_spell( 774 ), "" );

    // Call form-specific initialization to create necessary actions & setup variables
    if ( p()->find_action( "moonkin_form" ) )
      _init_moonkin();

    if ( p()->find_action( "bear_form" ) )
      _init_bear();

    if ( p()->find_action( "cat_form" ) )
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
      case CAST_HEAL:           return rng().roll( 0.5 ) ? actions.conv_regrowth : actions.conv_rejuvenation;
      default: return nullptr;
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
    if ( max_ticks <= 9 && !rng().roll( p()->options.cenarius_guidance_exceptional_chance ) )
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
        dist.emplace_back( CAST_MOONFIRE, ( main_count ? 0.25 : 1.0 ) / ( guidance ? 2.0 : 1.0 ) );

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
        dist.emplace_back( CAST_STARFALL, 3 + adjust );
        add_more = false;
      }

      std::vector<player_t*> mf_tl;  // separate list for mf targets without a dot;
      for ( const auto& t : tl )
        if ( !td( t )->dots.moonfire->is_ticking() )
          mf_tl.push_back( t );

      if ( !mf_tl.empty() )
      {
        if      ( dot_count < ( 4 - adjust ) ) dist.emplace_back( CAST_MOONFIRE, 5.5 );
        else if ( dot_count < ( 5 - adjust ) ) dist.emplace_back( CAST_MOONFIRE, 1.0 );
      }

      if ( add_more )
      {
        if      ( filler_count < ( 3 - adjust ) ) dist.emplace_back( CAST_WRATH, 5.5 );
        else if ( filler_count < ( 4 - adjust ) ) dist.emplace_back( CAST_WRATH, 3.5 );
        else if ( filler_count < ( 5 - adjust ) ) dist.emplace_back( CAST_WRATH, 1.0 );
      }

      if      ( main_count < 3 - adjust ) dist.emplace_back( CAST_STARSURGE, 6.0 );
      else if ( main_count < 4 - adjust ) dist.emplace_back( CAST_STARSURGE, 3.0 );
      else if ( main_count < 5 - adjust ) dist.emplace_back( CAST_STARSURGE, 1.0 );
      else if ( main_count < 6 - adjust ) dist.emplace_back( CAST_STARSURGE, 0.5 );

      if      ( filler_count < ( 4 - adjust ) ) dist.emplace_back( CAST_WRATH, 4.0 );
      else if ( filler_count < ( 5 - adjust ) ) dist.emplace_back( CAST_WRATH, 2.0 );
      else if ( filler_count < ( 6 - adjust ) ) dist.emplace_back( CAST_WRATH, 1.0 );
      else if ( filler_count < ( 7 - adjust ) ) dist.emplace_back( CAST_WRATH, 0.2 );

      if      ( off_count < ( 6 - adjust ) ) dist.emplace_back( CAST_HEAL, 0.8 );
      else if ( off_count < ( 7 - adjust ) ) dist.emplace_back( CAST_HEAL, 0.4 );

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

  bool usable_moving() const override { return true; }
};
}  // end namespace spells

namespace auto_attacks
{
template <typename Base>
struct druid_melee_t : public Base
{
  using ab = Base;
  using base_t = druid_melee_t<Base>;

  double ooc_chance = 0.0;

  druid_melee_t( std::string_view n, druid_t* p ) : Base( n, p )
  {
    ab::may_crit = ab::may_glance = ab::background = ab::repeating = ab::is_auto_attack = true;
    ab::allow_class_ability_procs = ab::not_a_proc = true;

    ab::school = SCHOOL_PHYSICAL;
    ab::trigger_gcd = 0_ms;
    ab::special = false;
    ab::weapon_multiplier = 1.0;

    ab::apply_buff_effects();
    ab::apply_debuffs_effects();

    // Auto attack mods
    ab::parse_passive_effects( p->spec.guardian );
    ab::parse_passive_effects( p->talent.killer_instinct );
    ab::range += find_effect( p->talent.astral_influence, A_MOD_AUTO_ATTACK_RANGE ).base_value();
    if ( p->specialization() != DRUID_BALANCE )
      ab::range -= 2;

    // Manually add to da_multiplier as Tiger's Fury + Carnivorious Instinct effect on auto attacks is scripted
    auto val = find_effect( p->buff.tigers_fury, A_MOD_AUTO_ATTACK_PCT ).percent();
    // Carnivorous Instinct has no curvepoint for effect#3 which modifies AA, so we use effect#1 value instead
    val += p->talent.carnivorous_instinct->effectN( 1 ).percent();

    ab::da_multiplier_buffeffects.emplace_back( p->buff.tigers_fury, val, false, false, nullptr );

    ab::sim->print_debug( "buff-effects: {} ({}) direct_damage modified by {} with buff {} ({})", ab::name(), ab::id,
                          val, p->buff.tigers_fury->name(), p->buff.tigers_fury->data().id() );

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
    form_mask = form_e::BEAR_FORM;

    energize_type = action_energize::ON_HIT;
    energize_resource = resource_e::RESOURCE_RAGE;
    energize_amount = 4.0;
  }

  void execute() override
  {
    base_t::execute();

    if ( hit_any_target )
      if ( p()->buff.tooth_and_claw->trigger() )
        p()->proc.tooth_and_claw->occur();
  }
};

// Cat Melee Attack =========================================================
struct cat_melee_t : public druid_melee_t<cat_attacks::cat_attack_t>
{
  cat_melee_t( druid_t* p ) : base_t( "cat_melee", p )
  {
    form_mask = form_e::CAT_FORM;

    snapshots.tigers_fury = true;
  }
};

// Auto Attack (Action)========================================================
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
  double absorb_cap = attack_power * 0.075;

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

// Rage of the Sleeper Absorb/Reflect Handler ===============================
double rage_of_the_sleeper_handler( const action_state_t* s )
{
  auto p = static_cast<druid_t*>( s->target );

  if ( !p->buff.rage_of_the_sleeper->up() )
    return 0;

  if ( s->action->player != p )
    p->active.rage_of_the_sleeper_reflect->execute_on_target( s->action->player );

  return s->result_amount * p->talent.rage_of_the_sleeper->effectN( 3 ).percent();
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

struct astral_power_decay_event_t : public event_t
{
private:
  druid_t* p_;
  double nb_cap;

public:
  astral_power_decay_event_t( druid_t* p )
    : event_t( *p, 500_ms ),
      p_( p ),
      nb_cap( p->resources.base[ RESOURCE_ASTRAL_POWER ] * p->talent.natures_balance->effectN( 2 ).percent() )
  {}

  const char* name() const override { return "astral_power_decay"; }

  void execute() override
  {
    if ( !p_->in_combat )
    {
      auto cur = p_->resources.current[ RESOURCE_ASTRAL_POWER ];
      if ( cur > nb_cap )
      {
        p_->resource_loss( RESOURCE_ASTRAL_POWER, std::min( 5.0, cur - nb_cap ) );
        make_event<astral_power_decay_event_t>( sim(), p_ );
      }
    }
  }
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

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::activate ========================================================
void druid_t::activate()
{
  if ( talent.dire_fixation.ok() )
  {
    register_on_kill_callback( [ this ]( player_t* t ) {
      if ( t == dire_fixation_target )
        dire_fixation_target = nullptr;
    } );
  }

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

  if ( spec.astral_power->ok() )
  {
    // Create repeating resource_loss event once OOC for 20s
    register_on_combat_state_callback( [ this ]( player_t*, bool c ) {
      if ( !c )
      {
        astral_power_decay = make_event( *sim, 20_s, [ this ]() {
          astral_power_decay = nullptr;
          make_event<astral_power_decay_event_t>( *sim, this );
        } );
      }
      else
      {
        event_t::cancel( astral_power_decay );
      }
    } );
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
  if ( name == "lunar_beam"            ) return new            lunar_beam_t( this, options_str );
  if ( name == "maul"                  ) return new                  maul_t( this, options_str );
  if ( name == "pulverize"             ) return new             pulverize_t( this, options_str );
  if ( name == "raze"                  ) return new                  raze_t( this, options_str );
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

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================
pet_t* druid_t::create_pet( std::string_view pet_name, std::string_view )
{
  if ( auto pet = find_pet( pet_name ) )
    return pet;

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

  // not actually pets, but this stage is a good place to create these as all spells & actions have been created
  if ( talent.adaptive_swarm.ok() )
  {
    swarm_targets.push_back( this );

    size_t i = 0;
    while ( i < options.adaptive_swarm_melee_targets )
    {
      auto t = swarm_targets.emplace_back(
          module_t::heal_enemy()->create_player( sim, "swarm_melee_target_" + util::to_string( ++i ), RACE_NONE ) );
      t->quiet = true;
      t->role = ROLE_ATTACK;
      sim->init_actor( t );
    }

    while ( i < options.adaptive_swarm_melee_targets + options.adaptive_swarm_ranged_targets )
    {
      auto t = swarm_targets.emplace_back(
          module_t::heal_enemy()->create_player( sim, "swarm_ranged_target_" + util::to_string( ++i ), RACE_NONE ) );
      t->quiet = true;
      t->role = ROLE_SPELL;
      sim->init_actor( t );
    }
  }
}

// druid_t::init_spells =====================================================
void druid_t::init_spells()
{
  auto check = [ this ]( auto check, auto arg ) {
    bool b;

    if constexpr ( std::is_invocable_v<decltype( &player_talent_t::ok ), decltype( check )> )
      b = check.ok();
    else if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), decltype( check )> )
      b = check->ok();
    else b = check;

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
  talent.forestwalk                     = CT( "Forestwalk" );
  talent.frenzied_regeneration          = CT( "Frenzied Regeneration" );
  talent.gale_winds                     = CT( "Gale Winds" );
  talent.heart_of_the_wild              = CT( "Heart of the Wild" );
  talent.hibernate                      = CT( "Hibernate" );
  talent.improved_barkskin              = CT( "Improved Barkskin" );
  talent.improved_rejuvenation          = CT( "Improved Rejuvenation" );
  talent.improved_stampeding_roar       = CT( "Improved Stampeding Roar");
  talent.improved_sunfire               = CT( "Improved Sunfire" );
  talent.improved_swipe                 = CT( "Improved Swipe" );
  talent.incapacitating_roar            = CT( "Incapacitating Roar" );
  talent.incessant_tempest              = CT( "Incessant Tempest" );
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
  talent.rising_light_falling_night     = CT( "Rising Light, Falling Night" );
  talent.rip                            = CT( "Rip" );
  talent.skull_bash                     = CT( "Skull Bash" );
  talent.soothe                         = CT( "Soothe" );
  talent.stampeding_roar                = CT( "Stampeding Roar" );
  talent.starfire                       = CT( "Starfire" );
  talent.starsurge                      = CT( "Starsurge" );
  talent.sunfire                        = CT( "Sunfire" );
  talent.swiftmend                      = CT( "Swiftmend" );
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
  talent.cosmic_rapidity                = ST( "Cosmic Rapidity" );
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
  talent.orbital_strike                 = ST( "Orbital Strike" );
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
  talent.twin_moons                     = ST( "Twin Moons" );
  talent.umbral_embrace                 = ST( "Umbral Embrace" );
  talent.umbral_intensity               = ST( "Umbral Intensity" );
  talent.waning_twilight                = ST( "Waning Twilight" );
  talent.warrior_of_elune               = ST( "Warrior of Elune" );
  talent.wild_mushroom                  = ST( "Wild Mushroom" );
  talent.wild_surges                    = ST( "Wild Surges" );

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
  talent.dire_fixation                  = ST( "Dire Fixation" );
  talent.doubleclawed_rake              = ST( "Double-Clawed Rake" );
  talent.dreadful_bleeding              = ST( "Dreadful Bleeding" );
  talent.feral_frenzy                   = ST( "Feral Frenzy" );
  talent.frantic_momentum               = ST( "Frantic Momentum" );
  talent.incarnation_cat                = ST( "Incarnation: Avatar of Ashamane" );
  talent.infected_wounds_cat            = STS( "Infected Wounds", DRUID_FERAL );
  talent.lions_strength                 = ST( "Lion's Strength" );
  talent.lunar_inspiration              = ST( "Lunar Inspiration" );
  talent.merciless_claws                = ST( "Merciless Claws" );
  talent.moment_of_clarity              = ST( "Moment of Clarity" );
  talent.omen_of_clarity_cat            = STS( "Omen of Clarity", DRUID_FERAL );
  talent.pouncing_strikes               = ST( "Pouncing Strikes" );
  talent.predatory_swiftness            = ST( "Predatory Swiftness" );
  talent.primal_claws                   = ST( "Primal Claws" );  // TODO: remove in 10.1.5
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
  talent.thrashing_claws                = ST( "Thrashing Claws" );
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
  talent.lunar_beam                     = ST( "Lunar Beam" );
  talent.mangle                         = ST( "Mangle" );
  talent.maul                           = ST( "Maul" );
  talent.moonless_night                 = ST( "Moonless Night" );
  talent.pulverize                      = ST( "Pulverize" );
  talent.rage_of_the_sleeper            = ST( "Rage of the Sleeper" );
  talent.raze                           = ST( "Raze" );
  talent.reinforced_fur                 = ST( "Reinforced Fur" );
  talent.reinvigoration                 = ST( "Reinvigoration" );
  talent.rend_and_tear                  = ST( "Rend and Tear" );
  talent.scintillating_moonlight        = ST( "Scintillating Moonlight" );
  talent.soul_of_the_forest_bear        = STS( "Soul of the Forest", DRUID_GUARDIAN );
  talent.survival_of_the_fittest        = ST( "Survival of the Fittest" );
  talent.thorns_of_iron                 = ST( "Thorns of Iron" );
  talent.tooth_and_claw                 = ST( "Tooth and Claw" );
  talent.twin_moonfire                  = ST( "Twin Moonfire" );
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
  talent.deep_focus                     = ST( "Deep Focus" );
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
  talent.tranquil_mind                  = ST( "Tranquil Mind" );
  talent.tranquility                    = ST( "Tranquility" );
  talent.undergrowth                    = ST( "Undergrowth" );  // TODO: NYI
  talent.unstoppable_growth             = ST( "Unstoppable Growth" );
  talent.verdancy                       = ST( "Verdancy" );  // TODO: NYI
  talent.verdant_infusion               = ST( "Verdant Infusion" );  // TODO: NYI
  talent.waking_dream                   = ST( "Waking Dream" );  // TODO: increased healing per rejuv NYI
  talent.yseras_gift                    = ST( "Ysera's Gift" );

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
  spec.improved_prowl           = find_specialization_spell( "Improved Prowl" );
  spec.moonfire_2               = find_rank_spell( "Moonfire", "Rank 2" );
  spec.moonfire_dmg             = find_spell( 164812 );
  spec.swipe                    = find_class_spell( "Swipe" );
  spec.wrath                    = find_specialization_spell( "Wrath" );
  if ( !spec.wrath->ok() )
    spec.wrath                  = find_class_spell( "Wrath" );

  // Class Abilities
  spec.moonfire                 = find_class_spell( "Moonfire" );
  spec.sunfire_dmg              = check( talent.sunfire, 164815 );
  spec.thrash_bear_dot          = check( talent.thrash, 192090 );
  spec.thrash_cat_dot           = find_spell( 405233 );

  // Multi-Spec Abilities
  spec.adaptive_swarm_damage    = check( talent.adaptive_swarm, 391889 );
  spec.adaptive_swarm_heal      = check( talent.adaptive_swarm, 391891 );

  // Balance Abilities
  spec.balance                  = find_specialization_spell( "Balance Druid" );
  spec.astral_communion         = check( talent.astral_communion, 202359 );
  spec.astral_power             = find_specialization_spell( "Astral Power" );
  spec.celestial_alignment      = talent.celestial_alignment.find_override_spell();
  spec.eclipse_lunar            = check( talent.eclipse, 48518 );
  spec.eclipse_solar            = check( talent.eclipse, 48517 );
  spec.full_moon                = check( talent.new_moon, 274283 );
  spec.half_moon                = check( talent.new_moon, 274282 );
  spec.incarnation_moonkin      = check( talent.incarnation_moonkin, 102560 );
  spec.shooting_stars_dmg       = check( talent.shooting_stars, 202497 );  // shooting stars damage
  spec.waning_twilight          = check( talent.waning_twilight, 393957 );
  spec.starfall                 = check( talent.starfall, 191034 );

  // Feral Abilities
  spec.feral                    = find_specialization_spell( "Feral Druid" );
  spec.feral_overrides          = find_specialization_spell( "Feral Overrides Passive" );
  spec.ashamanes_guidance       = check( talent.ashamanes_guidance, talent.convoke_the_spirits.ok() ? 391538 : 391475 );
  spec.berserk_cat              = talent.berserk.find_override_spell();

  // Guardian Abilities
  spec.guardian                 = find_specialization_spell( "Guardian Druid" );
  spec.bear_form_2              = find_rank_spell( "Bear Form", "Rank 2" );
  spec.berserk_bear             = check( talent.berserk_ravage ||
                                         talent.berserk_unchecked_aggression ||
                                         talent.berserk_persistence, 50334 );
  spec.elunes_favored           = find_spell( 370588 );
  spec.furious_regeneration     = check( sets->set( DRUID_GUARDIAN, T30, B2 ), 408504 );
  spec.incarnation_bear         = check( talent.incarnation_bear, 102558 );
  spec.lightning_reflexes       = find_specialization_spell( "Lightning Reflexes" );
  spec.ursine_adept             = find_specialization_spell( "Ursine Adept" );

  // Restoration Abilities
  spec.restoration              = find_specialization_spell( "Restoration Druid" );
  spec.cenarius_guidance        = check( talent.cenarius_guidance, talent.convoke_the_spirits.ok() ? 393374 : 393381 );

  // Masteries ==============================================================
  mastery.harmony             = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian    = find_mastery_spell( DRUID_GUARDIAN );
  mastery.natures_guardian_AP = check( mastery.natures_guardian, 159195 );
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
  base.armor_multiplier *= 1.0 + find_effect( talent.killer_instinct, A_MOD_BASE_RESISTANCE_PCT ).percent();

  // Resources
  resources.base[ RESOURCE_RAGE ]         = 100;
  resources.base[ RESOURCE_COMBO_POINT ]  = 5;
  resources.base[ RESOURCE_ASTRAL_POWER ] = 100 + find_effect( talent.astral_communion, A_MOD_MAX_RESOURCE ).resource( RESOURCE_ASTRAL_POWER );
  resources.base[ RESOURCE_ENERGY ]       = 100 + find_effect( talent.tireless_energy, A_MOD_INCREASE_ENERGY ).resource( RESOURCE_ENERGY );

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
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + find_effect( spec.feral_affinity, A_MOD_POWER_REGEN_PERCENT ).percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + find_effect( spec.feral, A_MOD_POWER_REGEN_PERCENT ).percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + find_effect( talent.tireless_energy, A_MOD_POWER_REGEN_PERCENT ).percent();

  base_gcd = 1.5_s;
}

void druid_t::init_finished()
{
  player_t::init_finished();

  if ( specialization() == DRUID_BALANCE )
    spec_override.attack_power = find_effect( spec.balance, A_404 ).percent();
  else if ( specialization() == DRUID_FERAL )
    spec_override.spell_power = find_effect( spec.feral, A_366 ).percent();
  else if ( specialization() == DRUID_GUARDIAN )
    spec_override.spell_power = find_effect( spec.guardian, A_366 ).percent();
  else if ( specialization() == DRUID_RESTORATION )
    spec_override.attack_power = find_effect( spec.restoration, A_404 ).percent();

  // PRECOMBAT WRATH SHENANIGANS
  // we do this here so all precombat actions have gone throught init() and init_finished() so if-expr are properly
  // parsed and we can adjust wrath travel times accordingly based on subsequent precombat actions that will sucessfully
  // cast
  for ( auto pre = precombat_action_list.begin(); pre != precombat_action_list.end(); pre++ )
  {
    if ( auto wr = dynamic_cast<spells::wrath_t*>( *pre ) )
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

  buff.bear_form = make_buff<bear_form_buff_t>( this );

  buff.cat_form = make_buff<cat_form_buff_t>( this );

  buff.dash = make_buff( this, "dash", find_class_spell( "Dash" ) )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );

  buff.thorns = make_buff( this, "thorns", find_spell( 305497 ) );

  buff.wild_charge_movement = make_buff( this, "wild_charge_movement" );

  // Class
  buff.forestwalk = make_buff( this, "forestwalk", find_trigger( talent.forestwalk ).trigger() )
    ->apply_affecting_aura( talent.forestwalk )
    ->set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS, P_MAX, 0.0, E_APPLY_AREA_AURA_PARTY );

  buff.heart_of_the_wild = make_buff( this, "heart_of_the_wild", talent.heart_of_the_wild )
    ->set_cooldown( 0_ms );

  buff.ironfur = make_buff( this, "ironfur", talent.ironfur )
    ->set_default_value_from_effect_type( A_MOD_ARMOR_BY_PRIMARY_STAT_PCT )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_cooldown( 0_ms )
    ->apply_affecting_aura( talent.reinforced_fur )
    ->apply_affecting_aura( spec.ursine_adept )
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
    ->set_default_value( talent.lycaras_teachings->effectN( 1 ).base_value() )
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

  buff.matted_fur = make_buff<matted_fur_buff_t>( this );

  buff.moonkin_form = make_buff<moonkin_form_buff_t>( this );

  buff.natures_vigil = make_buff( this, "natures_vigil", talent.natures_vigil )
    ->set_default_value( 0 )
    ->set_cooldown( 0_ms )
    ->set_freeze_stacks( true );

  buff.protector_of_the_pack =
      make_buff( this, "protector_of_the_pack", find_spell( specialization() == DRUID_RESTORATION ? 378987 : 395336 ) )
          ->set_trigger_spell( talent.protector_of_the_pack );

  buff.rising_light_falling_night_day = make_buff( this, "rising_light_falling_night__day", find_spell( 417714 ) )
    ->set_trigger_spell( talent.rising_light_falling_night );

  buff.rising_light_falling_night_night =
      make_buff<stat_buff_t>( this, "rising_light_falling_night__night", find_spell( 417715 ) )
          ->set_trigger_spell( talent.rising_light_falling_night )
          ->set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT )
          ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );

  buff.tiger_dash = make_buff( this, "tiger_dash", talent.tiger_dash )
    ->set_cooldown( 0_ms )
    ->set_freeze_stacks( true )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED )
    ->set_tick_callback( []( buff_t* b, int, timespan_t ) {
      b->current_value -= b->data().effectN( 2 ).percent();
    } );

  buff.tireless_pursuit =
      make_buff( this, "tireless_pursuit", find_spell( 340546 ) )
          ->set_default_value( spec.cat_form_speed->effectN( 1 ).percent() )  // only switching from cat form supported
          ->set_duration( talent.tireless_pursuit->effectN( 1 ).time_value() );

  buff.ursine_vigor = make_buff<ursine_vigor_buff_t>( this );

  // Multi-spec
  // The buff ID in-game is same as the talent, 61336, but the buff effects (as well as tooltip reference) is in 50322
  buff.survival_instincts = make_buff( this, "survival_instincts", talent.survival_instincts )
    ->set_cooldown( 0_ms )
    ->set_default_value( find_effect( find_spell( 50322 ), A_MOD_DAMAGE_PERCENT_TAKEN ).percent() );

  // Balance buffs
  buff.balance_of_all_things_arcane = make_buff( this, "balance_of_all_things_arcane", find_spell( 394050 ) )
    ->set_trigger_spell( talent.balance_of_all_things )
    ->set_reverse( true )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_name_reporting( "Arcane" );

  buff.balance_of_all_things_nature = make_buff( this, "balance_of_all_things_nature", find_spell( 394049 ) )
    ->set_trigger_spell( talent.balance_of_all_things )
    ->set_reverse( true )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_name_reporting( "Nature" );

  buff.celestial_alignment =
      make_buff<celestial_alignment_buff_t>( this, "celestial_alignment", spec.celestial_alignment );

  buff.incarnation_moonkin =
      make_buff<celestial_alignment_buff_t>( this, "incarnation_chosen_of_elune", spec.incarnation_moonkin )
          ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.denizen_of_the_dream = make_buff( this, "denizen_of_the_dream", find_spell( 394076 ) )
    ->set_trigger_spell( talent.denizen_of_the_dream )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_max_stack( 10 )
    ->set_rppm( rppm_scale_e::RPPM_DISABLE );

  buff.eclipse_lunar = make_buff( this, "eclipse_lunar", spec.eclipse_lunar )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
    ->set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
      if ( old_ && !new_ )
        eclipse_handler.advance_eclipse();
    } );

  buff.eclipse_solar = make_buff( this, "eclipse_solar", spec.eclipse_solar )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
    ->set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
      if ( old_ && !new_ )
        eclipse_handler.advance_eclipse();
    } );

  buff.friend_of_the_fae = make_buff( this, "friend_of_the_fae", find_spell( 394083 ) )
    ->set_trigger_spell( talent.friend_of_the_fae )
    ->set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
      if ( !old_ )
        uptime.friend_of_the_fae->update( true, sim->current_time() );
      else if ( !new_ )
        uptime.friend_of_the_fae->update( false, sim->current_time() );
    } );

  buff.fury_of_elune = make_buff<fury_of_elune_buff_t>( this, "fury_of_elune", talent.fury_of_elune );

  buff.sundered_firmament = make_buff<fury_of_elune_buff_t>( this, "sundered_firmament", find_spell( 394108 ) )
    ->set_trigger_spell( talent.sundered_firmament )
    ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  buff.parting_skies = make_buff( this, "parting_skies", find_spell( 395110 ) )
    ->set_trigger_spell( talent.sundered_firmament )
    ->set_reverse( true )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ){
      if ( !new_ )
        active.sundered_firmament->execute_on_target( target );
    } );

  buff.gathering_starstuff =
      make_buff( this, "gathering_starstuff", find_trigger( sets->set( DRUID_BALANCE, T29, B2 ) ).trigger() );

  buff.natures_balance = make_buff( this, "natures_balance", talent.natures_balance )
    ->set_quiet( true )
    ->set_freeze_stacks( true );
  const auto& nb_eff = find_effect( buff.natures_balance, A_PERIODIC_ENERGIZE );
  buff.natures_balance
    ->set_default_value( nb_eff.resource( RESOURCE_ASTRAL_POWER ) / nb_eff.period().total_seconds() )
    ->set_tick_callback(
        [ ap = nb_eff.resource( RESOURCE_ASTRAL_POWER ),
          cap = talent.natures_balance->effectN( 2 ).percent(),
          this ]
        ( buff_t*, int, timespan_t ) mutable {
          if ( !in_combat )
          {
            if ( resources.current[ RESOURCE_ASTRAL_POWER ] < resources.base[ RESOURCE_ASTRAL_POWER ] * cap )
              ap *= 3.0;
            else
              ap = 0;
          }
          resource_gain( RESOURCE_ASTRAL_POWER, ap, gain.natures_balance );
        } );

  buff.natures_grace = make_buff( this, "natures_grace", find_spell( 393959 ) )
    ->set_trigger_spell( talent.natures_grace )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.orbit_breaker = make_buff( this, "orbit_breaker" )
    ->set_quiet( true )
    ->set_trigger_spell( talent.orbit_breaker )
    ->set_max_stack( std::max( 1, as<int>( talent.orbit_breaker->effectN( 1 ).base_value() ) ) );

  buff.owlkin_frenzy = make_buff( this, "owlkin_frenzy", find_spell( 157228 ) )
    ->set_chance( find_effect( find_specialization_spell( "Owlkin Frenzy" ), A_ADD_FLAT_MODIFIER, P_PROC_CHANCE ).percent() );

  buff.primordial_arcanic_pulsar = make_buff( this, "primordial_arcanic_pulsar", find_spell( 393961 ) )
    ->set_trigger_spell( talent.primordial_arcanic_pulsar );

  buff.rattled_stars = make_buff( this, "rattled_stars", find_trigger( talent.rattle_the_stars ).trigger() )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  buff.shooting_stars_moonfire = make_buff<shooting_stars_buff_t>( this, "shooting_stars_moonfire",
      dot_list.moonfire, active.shooting_stars_moonfire );

  buff.shooting_stars_sunfire = make_buff<shooting_stars_buff_t>( this, "shooting_stars_sunfire",
      dot_list.sunfire, active.shooting_stars_sunfire );

  buff.solstice = make_buff( this, "solstice", find_trigger( talent.solstice ).trigger() )
    ->set_default_value( find_trigger( talent.solstice ).percent() );

  buff.starfall = make_buff( this, "starfall", find_spell( 191034 ) )  // lookup via spellid for convoke
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_freeze_stacks( true )
    ->set_partial_tick( true )  // TODO: confirm true?
    ->set_tick_behavior( buff_tick_behavior::REFRESH );  // TODO: confirm true?

  buff.starlord = make_buff( this, "starlord", find_spell( 279709 ) )
    ->set_trigger_spell( talent.starlord )
    ->set_default_value( talent.starlord->effectN( 1 ).percent() )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.starweavers_warp = make_buff( this, "starweavers_warp", find_spell( 393942 ) )
    ->set_trigger_spell( talent.starweaver )
    ->set_chance( talent.starweaver->effectN( 1 ).percent() );

  buff.starweavers_weft = make_buff( this, "starweavers_weft", find_spell( 393944 ) )
    ->set_trigger_spell( talent.starweaver )
    ->set_chance( talent.starweaver->effectN( 2 ).percent() );

  buff.touch_the_cosmos =
      make_buff( this, "touch_the_cosmos", sets->set( DRUID_BALANCE, T29, B4 )->effectN( 1 ).trigger() );

  buff.touch_the_cosmos_starfall = make_buff( this, "touch_the_cosmos_starfall" )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_duration( buff.starfall->buff_duration() )
    ->set_max_stack( buff.starfall->max_stack() )
    ->set_quiet( true );

  buff.umbral_embrace = make_buff( this, "umbral_embrace", find_trigger( talent.umbral_embrace ).trigger() )
    ->set_default_value( find_effect( talent.umbral_embrace, A_ADD_FLAT_LABEL_MODIFIER, P_EFFECT_1 ).percent() );

  buff.warrior_of_elune = make_buff( this, "warrior_of_elune", talent.warrior_of_elune )
    ->set_reverse( true );
  if ( !is_ptr() )
  {
    buff.warrior_of_elune->set_cooldown( 0_ms )
      ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
        if ( !new_ )
          cooldown.warrior_of_elune->start( cooldown.warrior_of_elune->action );
      } );
  }

  // Feral buffs
  buff.apex_predators_craving =
      make_buff( this, "apex_predators_craving", find_trigger( talent.apex_predators_craving ).trigger() )
          ->set_chance( find_trigger( talent.apex_predators_craving ).percent() );

  buff.berserk_cat =
      make_buff<berserk_cat_buff_t>( this, "berserk_cat", spec.berserk_cat );

  buff.incarnation_cat =
      make_buff<berserk_cat_buff_t>( this, "incarnation_avatar_of_ashamane", talent.incarnation_cat, true );

  buff.bloodtalons = make_buff( this, "bloodtalons", find_spell( 145152 ) )
    ->set_trigger_spell( talent.bloodtalons );
  buff.bt_rake         = make_buff<bt_dummy_buff_t>( this, "bt_rake" );
  buff.bt_shred        = make_buff<bt_dummy_buff_t>( this, "bt_shred" );
  buff.bt_swipe        = make_buff<bt_dummy_buff_t>( this, "bt_swipe" );
  buff.bt_thrash       = make_buff<bt_dummy_buff_t>( this, "bt_thrash" );
  buff.bt_moonfire     = make_buff<bt_dummy_buff_t>( this, "bt_moonfire" );
  buff.bt_brutal_slash = make_buff<bt_dummy_buff_t>( this, "bt_brutal_slash" );
  buff.bt_feral_frenzy = make_buff<bt_dummy_buff_t>( this, "bt_feral_frenzy" );

  // 1.05s ICD per https://github.com/simulationcraft/simc/commit/b06d0685895adecc94e294f4e3fcdd57ac909a10
  buff.clearcasting_cat = make_buff( this, "clearcasting_cat", find_trigger( talent.omen_of_clarity_cat ).trigger() )
    ->set_cooldown( 1.05_s )
    ->apply_affecting_aura( talent.moment_of_clarity );
  buff.clearcasting_cat->name_str_reporting = "clearcasting";

  buff.tigers_tenacity = make_buff( this, "tigers_tenacity", find_trigger( talent.tigers_tenacity ).trigger() )
    ->set_reverse( true );
  buff.tigers_tenacity->set_default_value(
      find_effect( find_trigger( buff.tigers_tenacity ).trigger(), E_ENERGIZE ).resource( RESOURCE_COMBO_POINT ) );

  buff.frantic_momentum = make_buff( this, "frantic_momentum", find_trigger( talent.frantic_momentum ).trigger() )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.incarnation_cat_prowl = 
      make_buff( this, "incarnation_avatar_of_ashamane_prowl", find_effect( talent.incarnation_cat, E_TRIGGER_SPELL ).trigger() )
          ->set_name_reporting( "Prowl" );

  buff.overflowing_power = make_buff( this, "overflowing_power", find_spell( 405189 ) )
    ->set_trigger_spell( talent.berserk );

  buff.predator_revealed = make_buff( this, "predator_revealed", find_spell( 408468 ) )
    ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
    ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
    ->set_trigger_spell( sets->set( DRUID_FERAL, T30, B4 ) );
  buff.predator_revealed->set_tick_callback(
      [ cp = find_effect( find_trigger( buff.predator_revealed ).trigger(), E_ENERGIZE ).resource( RESOURCE_COMBO_POINT ),
        gain = get_gain( buff.predator_revealed->name() ),
        this ]
      ( buff_t*, int, timespan_t ) {
        resource_gain( RESOURCE_COMBO_POINT, cp, gain );
      } );

  buff.predatory_swiftness = make_buff( this, "predatory_swiftness", find_spell( 69369 ) )
    ->set_trigger_spell( talent.predatory_swiftness );

  buff.protective_growth = make_buff( this, "protective_growth", find_spell( 391955 ) )
    ->set_trigger_spell( talent.protective_growth )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN );

  buff.prowl = make_buff( this, "prowl", find_class_spell( "Prowl" ) );

  buff.sabertooth = make_buff( this, "sabertooth", find_spell( 391722 ) )
    ->set_trigger_spell( talent.sabertooth )
    ->set_default_value( talent.sabertooth->effectN( 2 ).percent() )
    ->set_max_stack( as<int>( resources.base[ RESOURCE_COMBO_POINT ] ) );

  buff.shadows_of_the_predator = make_buff<shadows_of_the_predator_buff_t>( this, sets->set( DRUID_FERAL, T30, B2 ) );

  buff.sharpened_claws = make_buff( this, "sharpened_claws", find_spell( 394465 ) )
    ->set_trigger_spell( sets->set( DRUID_FERAL, T29, B4 ) );

  buff.sudden_ambush = make_buff( this, "sudden_ambush", find_trigger( talent.sudden_ambush ).trigger() );

  buff.tigers_fury = make_buff( this, "tigers_fury", talent.tigers_fury )
    ->set_cooldown( 0_ms )
    ->apply_affecting_aura( talent.predator )
    // TODO: hack for bug where frenzied assault ignores benefit from tigers fury
    ->set_default_value_from_effect( 1 )
    ->apply_affecting_aura( talent.carnivorous_instinct );

  // Guardian buffs
  buff.after_the_wildfire = make_buff( this, "after_the_wildfire", talent.after_the_wildfire->effectN( 1 ).trigger() )
    ->set_default_value( talent.after_the_wildfire->effectN( 2 ).base_value() );

  buff.berserk_bear =
      make_buff<berserk_bear_buff_t>( this, "berserk_bear", spec.berserk_bear );

  buff.incarnation_bear =
      make_buff<berserk_bear_buff_t>( this, "incarnation_guardian_of_ursoc", spec.incarnation_bear, true );

  buff.bristling_fur = make_buff( this, "bristling_fur", talent.bristling_fur )
    ->set_cooldown( 0_ms );

  buff.dream_of_cenarius = make_buff( this, "dream_of_cenarius", talent.dream_of_cenarius->effectN( 1 ).trigger() )
    ->set_cooldown( find_spell( 372523 )->duration() );

  buff.earthwarden = make_buff( this, "earthwarden", find_spell( 203975 ) )
    ->set_trigger_spell( talent.earthwarden )
    ->set_default_value( talent.earthwarden->effectN( 1 ).percent() );

  buff.elunes_favored = make_buff( this, "elunes_favored", spec.elunes_favored )
    ->set_trigger_spell( talent.elunes_favored )
    ->set_quiet( true )
    ->set_freeze_stacks( true )
    ->set_default_value( 0 )
    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
      if ( b->check_value() )
      {
        active.elunes_favored_heal->execute();
        b->current_value = 0;
      }
    } );

  buff.furious_regeneration = make_buff( this, "furious_regeneration", spec.furious_regeneration )
    ->set_trigger_spell( sets->set( DRUID_GUARDIAN, T30, B2 ) )
    ->set_default_value_from_effect( 5 );

  buff.galactic_guardian = make_buff( this, "galactic_guardian", find_spell( 213708 ) )
    ->set_default_value_from_effect( 1, 0.1 /*RESOURCE_RAGE*/ );

  buff.gore = make_buff( this, "gore", find_spell( 93622 ) )
    ->set_trigger_spell( talent.gore )
    ->set_chance( talent.gore->effectN( 1 ).percent() + sets->set( DRUID_GUARDIAN, T29, B4 )->effectN( 1 ).percent() )
    ->set_cooldown( talent.gore->internal_cooldown() )
    ->set_default_value_from_effect( 1, 0.1 /*RESOURCE_RAGE*/ );

  buff.gory_fur = make_buff( this, "gory_fur", find_trigger( talent.gory_fur ).trigger() )
    ->set_chance( talent.gory_fur->proc_chance() );

  buff.guardian_of_elune = make_buff( this, "guardian_of_elune", find_trigger( talent.guardian_of_elune ).trigger() );

  buff.indomitable_guardian = make_buff( this, "indomitable_guardian", find_spell( 408522 ) )
    ->set_trigger_spell( sets->set( DRUID_GUARDIAN, T30, B4 ) )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_HEALTH_PERCENT )
    ->set_stack_change_callback( [ this ]( buff_t* b, int old_, int new_ ) {
      auto old_mul = 1.0 + old_ * b->default_value;
      auto new_mul = 1.0 + new_ * b->default_value;
      auto hp_mul = new_mul / old_mul;

      resources.max[ RESOURCE_HEALTH ] *= hp_mul;
      resources.current[ RESOURCE_HEALTH ] *= hp_mul;
      recalculate_resource_max( RESOURCE_HEALTH );
    } );

  buff.lunar_beam = make_buff( this, "lunar_beam", talent.lunar_beam )
    ->set_cooldown( 0_ms );
  if ( is_ptr() )
  {
    buff.lunar_beam->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
      ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
  }

  buff.overpowering_aura = make_buff( this, "overpowering_aura", find_spell( 395944 ) )
    ->set_trigger_spell( sets->set( DRUID_GUARDIAN, T29, B2 ) )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN );

  buff.rage_of_the_sleeper = make_buff( this, "rage_of_the_sleeper", talent.rage_of_the_sleeper )
    ->set_cooldown( 0_ms )
    ->add_invalidate( CACHE_LEECH );

  buff.tooth_and_claw = make_buff( this, "tooth_and_claw", find_trigger( talent.tooth_and_claw ).trigger() )
    ->set_chance( find_trigger( talent.tooth_and_claw ).percent() );

  buff.ursocs_fury = make_buff<absorb_buff_t>( this, "ursocs_fury", find_spell( 372505 ) )
    ->set_cumulative( true )
    ->set_trigger_spell( talent.ursocs_fury );

  buff.vicious_cycle_mangle = make_buff( this, "vicious_cycle_mangle", find_spell( 372019) )
    ->set_trigger_spell( talent.vicious_cycle )
    ->set_default_value( talent.vicious_cycle->effectN( 1 ).percent() )
    ->set_name_reporting( "Mangle" );

  buff.vicious_cycle_maul = make_buff( this, "vicious_cycle_maul", find_spell( 372015 ) )
    ->set_trigger_spell( talent.vicious_cycle )
    ->set_default_value( talent.vicious_cycle->effectN( 1 ).percent() )
    ->set_name_reporting( "Maul" );

  // Restoration buffs
  buff.abundance = make_buff( this, "abundance", find_spell( 207640 ) )
    ->set_duration( 0_ms );

  buff.cenarion_ward = make_buff( this, "cenarion_ward", talent.cenarion_ward );

  buff.clearcasting_tree = make_buff( this, "clearcasting_tree", find_trigger( talent.omen_of_clarity_tree ).trigger() )
    ->set_chance( find_trigger( talent.omen_of_clarity_tree ).percent() );
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

  buff.soul_of_the_forest_tree = make_buff( this, "soul_of_the_forest_tree", find_spell( 114108 ) )
    ->set_trigger_spell( talent.soul_of_the_forest_tree )
    ->set_name_reporting( "soul_of_the_forest" );

  buff.yseras_gift = make_buff( this, "yseras_gift_driver", talent.yseras_gift )
    ->apply_affecting_aura( talent.waking_dream->effectN( 1 ).trigger() )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_tick_callback( [this]( buff_t*, int, timespan_t ) {
        active.yseras_gift->schedule_execute();
      } );

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

  if ( talent.incarnation_moonkin.ok() )
  {
    active.shift_to_moonkin = get_secondary_action<moonkin_form_t>( "moonkin_form_shift", find_spell( 24858 ) );
    active.shift_to_moonkin->dual = true;
  }

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
    fm->energize_amount *= talent.orbit_breaker->effectN( 2 ).percent();
    fm->set_free_cast( free_spell_e::ORBIT );
    fm->background = true;
    active.orbit_breaker = fm;
  }

  if ( talent.shooting_stars.ok() )
  {
    active.shooting_stars = new action_t( action_e::ACTION_OTHER, "shooting_stars", this, talent.shooting_stars );

    auto mf = get_secondary_action_n<shooting_stars_t>( "shooting_stars_moonfire", spec.shooting_stars_dmg );
    mf->name_str_reporting = "Moonfire";
    active.shooting_stars->add_child( mf );
    active.shooting_stars_moonfire = mf;

    auto sf = get_secondary_action_n<shooting_stars_t>( "shooting_stars_sunfire", spec.shooting_stars_dmg );
    sf->name_str_reporting = "Sunfire";
    active.shooting_stars->add_child( sf );
    active.shooting_stars_sunfire = sf;

    if ( sets->has_set_bonus( DRUID_BALANCE, T30, B4 ) )
    {
      active.crashing_stars = get_secondary_action_n<shooting_stars_t>( "crashing_star", find_spell( 408310 ) );
      active.shooting_stars->add_child( active.crashing_stars );
    }
  }

  if ( talent.starweaver.ok() )
  {
    if ( talent.starsurge.ok() )
    {
      auto ss = get_secondary_action_n<starsurge_t>( "starsurge_starweaver", talent.starsurge, "" );
      ss->name_str_reporting = "starweavers_weft";
      ss->s_data_reporting = &buff.starweavers_weft->data();
      ss->set_free_cast( free_spell_e::STARWEAVER );
      active.starsurge_starweaver = ss;
    }

    if ( talent.starfall.ok() )
    {
      auto sf = get_secondary_action_n<starfall_t>( "starfall_starweaver", talent.starfall, "" );
      sf->name_str_reporting = "starweavers_warp";
      sf->s_data_reporting = &buff.starweavers_warp->data();
      sf->set_free_cast( free_spell_e::STARWEAVER );
      active.starfall_starweaver = sf;
    }
  }

  if ( talent.sundered_firmament.ok() )
  {
    auto firmament = get_secondary_action_n<fury_of_elune_t>( "sundered_firmament", find_spell( 394106 ),
                                                           find_spell( 394111 ), buff.sundered_firmament, "" );
    firmament->damage->base_multiplier = talent.sundered_firmament->effectN( 1 ).percent();
    firmament->s_data_reporting = talent.sundered_firmament;
    firmament->set_free_cast( free_spell_e::FIRMAMENT );
    firmament->background = true;
    active.sundered_firmament = firmament;
  }

  if ( talent.orbital_strike.ok() )
    active.orbital_strike = get_secondary_action<orbital_strike_t>( "orbital_strike" );
  
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

  if ( talent.thrashing_claws.ok() )
  {
    auto tc = get_secondary_action_n<thrash_cat_dot_t>( "thrashing_claws" );
    tc->s_data_reporting = talent.thrashing_claws;
    tc->aoe = 0;
    active.thrashing_claws = tc;
  }

  // Guardian
  if ( talent.after_the_wildfire.ok() )
    active.after_the_wildfire_heal = get_secondary_action<after_the_wildfire_heal_t>( "after_the_wildfire" );

  if ( talent.brambles.ok() )
    active.brambles = get_secondary_action<brambles_t>( "brambles" );

  if ( talent.elunes_favored.ok() )
    active.elunes_favored_heal = get_secondary_action<elunes_favored_heal_t>( "elunes_favored" );

  if ( sets->has_set_bonus( DRUID_GUARDIAN, T30, B2 ) )
    active.furious_regeneration = get_secondary_action<furious_regeneration_t>( "furious_regeneration" );

  if ( talent.galactic_guardian.ok() )
  {
    auto gg = get_secondary_action_n<moonfire_t>( "galactic_guardian", "" );
    gg->s_data_reporting = talent.galactic_guardian;
    gg->set_free_cast( free_spell_e::GALACTIC );
    gg->damage->set_free_cast( free_spell_e::GALACTIC );
    active.galactic_guardian = gg;
  }

  if ( talent.moonless_night.ok() )
    active.moonless_night = get_secondary_action<moonless_night_t>( "moonless_night" );

  if ( talent.tooth_and_claw.ok() )
  {
    if ( talent.maul.ok() )
    {
      auto tnc = get_secondary_action_n<maul_t>( "maul_tooth_and_claw", "" );
      tnc->s_data_reporting = talent.tooth_and_claw;
      tnc->name_str_reporting = "tooth_and_claw";
      tnc->set_free_cast( free_spell_e::TOOTH );
      active.maul_tooth_and_claw = tnc;
    }

    if ( talent.raze.ok() )
    {
      auto tnc = get_secondary_action_n<raze_t>( "raze_tooth_and_claw", "" );
      tnc->s_data_reporting = talent.tooth_and_claw;
      tnc->name_str_reporting = "tooth_and_claw";
      tnc->set_free_cast( free_spell_e::TOOTH );
      active.raze_tooth_and_claw = tnc;
    }
  }

  if ( mastery.natures_guardian->ok() )
    active.natures_guardian = new natures_guardian_t( this );

  if ( talent.flashing_claws.ok() )
  {
    auto flash = get_secondary_action_n<thrash_bear_t>( "flashing_claws",
                                                        apply_override( talent.thrash, spec.bear_form_override ), "" );
    flash->s_data_reporting = talent.flashing_claws;
    flash->name_str_reporting = "flashing_claws";
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

  if ( talent.thorns_of_iron.ok() )
  {
    active.thorns_of_iron = get_secondary_action<thorns_of_iron_t>( "thorns_of_iron" );
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
  find_parent( active.maul_tooth_and_claw, "maul" );
  find_parent( active.raze_tooth_and_claw, "raze" );
  find_parent( active.orbit_breaker, "full_moon" );
  find_parent( active.starsurge_starweaver, "starsurge" );
  find_parent( active.starfall_starweaver, "starfall" );
  find_parent( active.thrash_bear_flashing, "thrash_bear" );

  // setup dot_ids used by druid_action_t::get_dot_count()
  setup_dot_ids<sunfire_t::sunfire_damage_t>();
  setup_dot_ids<moonfire_t::moonfire_damage_t>();
  setup_dot_ids<lunar_inspiration_t>();
  setup_dot_ids<stellar_flare_t>();
  setup_dot_ids<rake_t::rake_bleed_t>();
  setup_dot_ids<rip_t>();
}

// Default Consumables ======================================================
std::string druid_t::default_flask() const
{
  if ( true_level >= 70 )
  {
    switch ( specialization() )
    {
      case DRUID_BALANCE:
      case DRUID_GUARDIAN:
      case DRUID_RESTORATION:
        return "phial_of_elemental_chaos_3";
      case DRUID_FERAL:
        return "phial_of_tepid_versatility_3";
      default:
        return "disabled";
    }
  }

  if ( true_level >= 60 )
    return "spectral_flask_of_power";
  return "disabled";
}

std::string druid_t::default_potion() const
{
  if ( true_level >= 70 ) return "elemental_potion_of_ultimate_power_3";

  switch ( specialization() )
  {
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
      return true_level >= 60 ? "spectral_intellect" : "superior_battle_potion_of_intellect";
    case DRUID_FERAL:
    case DRUID_GUARDIAN:
      return true_level >= 60 ? "spectral_agility" : "superior_battle_potion_of_agility";
    default: return "disabled";
  }
}

std::string druid_t::default_food() const
{
  if ( true_level >= 70 )
  {
    switch ( specialization() )
    {
      case DRUID_BALANCE:
        return "fated_fortune_cookie";
      case DRUID_GUARDIAN:
        return "fated_fortune_cookie";
      case DRUID_RESTORATION:
        return "fated_fortune_cookie";
      case DRUID_FERAL:
        return "thousandbone_tongueslicer";
      default:
        return "disabled";
    }
  }
  else if ( true_level >= 60 ) return "feast_of_gluttonous_hedonism";
  else if ( true_level >= 55 ) return "surprisingly_palatable_feast";
  else if ( true_level >= 45 ) return "famine_evaluator_and_snack_table";
  else return "disabled";
}

std::string druid_t::default_rune() const
{
  if      ( true_level >= 70 ) return "draconic";
  else if ( true_level >= 60 ) return "veiled";
  else if ( true_level >= 50 ) return "battle_scarred";
  else if ( true_level >= 45 ) return "defiled";
  else return "disabled";
}

std::string druid_t::default_temporary_enchant() const
{
  if ( true_level < 60 ) return "disabled";

  switch ( specialization() )
  {
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
      return true_level >= 70 ? "main_hand:howling_rune_3" : "main_hand:shadowcore_oil";
    case DRUID_FERAL:
      return true_level >= 70 ? "main_hand:buzzing_rune_3" : "main_hand:shaded_sharpening_stone";
    case DRUID_GUARDIAN:
      return true_level >= 70 ? "main_hand:primal_weightstone_3" : "main_hand:shadowcore_oil";
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
    def->add_action( action_str );

  for ( const auto& action_str : get_item_actions() )
    def->add_action( action_str );

  for ( const auto& action_str : get_profession_actions() )
    def->add_action( action_str );
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

void druid_t::apl_balance_ptr()
{
#include "class_modules/apl/balance_apl_ptr.inc"
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
    if ( options.adaptive_swarm_melee_targets == 9 )
      options.adaptive_swarm_melee_targets = 3;
    if ( options.adaptive_swarm_ranged_targets == 14 )
      options.adaptive_swarm_ranged_targets = 1;
  }
}

bool druid_t::validate_fight_style( fight_style_e /* style */ ) const
{
  /* uncomment if certain fight styles prove problematic again
    switch ( specialization() )
    {
      case DRUID_BALANCE:
      case DRUID_FERAL:
      case DRUID_GUARDIAN:
      case DRUID_RESTORATION:
      default:
        break;
    }
  */
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
    gain.cats_curiosity      = get_gain( "Cat's Curiosity" );
    gain.energy_refund       = get_gain( "Energy Refund" );
    gain.incessant_hunter    = get_gain( "Incessant Hunter" );
    gain.overflowing_power   = get_gain( "Overflowing Power" );
    gain.primal_claws        = get_gain( "Primal Claws" );  // TODO: remove in 10.1.5
    gain.primal_fury         = get_gain( "Primal Fury" );
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

  // TODO: temporarily remove advanced data collection until issues with profilesets & merge() showing one sim with
  // simple=true and other with simple=false is resolved

  // Balance
  proc.denizen_of_the_dream = get_proc( "Denizen of the Dream" )->collect_count();
  proc.pulsar               = get_proc( "Primordial Arcanic Pulsar" )->collect_interval();

  // Feral
  proc.ashamanes_guidance   = get_proc( "Ashamane's Guidance" )->collect_count();
  proc.predator             = get_proc( "Predator" );
  proc.predator_wasted      = get_proc( "Predator (Wasted)" );
  proc.primal_claws         = get_proc( "Primal Claws" );  // TODO: remove in 10.1.5
  proc.primal_fury          = get_proc( "Primal Fury" );
  proc.clearcasting         = get_proc( "Clearcasting" );
  proc.clearcasting_wasted  = get_proc( "Clearcasting (Wasted)" );

  if ( sets->has_set_bonus( DRUID_FERAL, T30, B2 ) )
    predator_revealed_stacks = std::make_unique<extended_sample_data_t>( "Predator Revealed Stack", false );

  // Guardian
  proc.galactic_guardian    = get_proc( "Galactic Guardian" )->collect_count();
  proc.gore                 = get_proc( "Gore" )->collect_interval();
  proc.tooth_and_claw       = get_proc( "Tooth and Claw" )->collect_interval();
}

// druid_t::init_uptimes ====================================================
void druid_t::init_uptimes()
{
  player_t::init_uptimes();

  // TODO: temporarily remove advanced data collection until issues with profilesets & merge() showing one sim with
  // simple=true and other with simple=false is resolved

  std::string ca_inc_str = talent.incarnation_moonkin.ok() ? "Incarnation" : "Celestial Alignment";

  uptime.astral_smolder            = get_uptime( "Astral Smolder" )->collect_uptime( *sim );
  uptime.combined_ca_inc           = get_uptime( ca_inc_str + " (Total)" )->collect_uptime( *sim )->collect_duration( *sim );
  uptime.primordial_arcanic_pulsar = get_uptime( ca_inc_str + " (Pulsar)" )->collect_uptime( *sim );
  uptime.eclipse_lunar             = get_uptime( "Lunar Eclipse Only" )->collect_uptime( *sim );
  uptime.eclipse_solar             = get_uptime( "Solar Eclipse Only" )->collect_uptime( *sim );
  uptime.eclipse_none              = get_uptime( "No Eclipse" )->collect_uptime( *sim );
  uptime.friend_of_the_fae         = get_uptime( "Friend of the Fae" )->collect_uptime( *sim );
  uptime.incarnation_cat           = get_uptime( "Incarnation: Avatar of Ashamane" )->collect_uptime( *sim );
  uptime.tooth_and_claw_debuff     = get_uptime( "Tooth and Claw Debuff" )->collect_uptime( *sim );
}

// druid_t::init_resources ==================================================
void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ]         = 0;
  resources.current[ RESOURCE_COMBO_POINT ]  = 0;
  if ( options.initial_astral_power == 0.0 && talent.natures_balance.ok() )
  {
    resources.current[ RESOURCE_ASTRAL_POWER ] =
        resources.base[ RESOURCE_ASTRAL_POWER ] * talent.natures_balance->effectN( 2 ).percent();
  }
  else
  {
    resources.current[ RESOURCE_ASTRAL_POWER ] = options.initial_astral_power;
  }
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
  struct druid_cb_t : public dbc_proc_callback_t
  {
    druid_cb_t( druid_t* p, const special_effect_t& e ) : dbc_proc_callback_t( p, e ) {}

    druid_t* p() { return static_cast<druid_t*>( listener ); }
  };

  // General
  if ( unique_gear::find_special_effect( this, 388069 ) )
  {
    callbacks.register_callback_execute_function( 388069,
      []( const dbc_proc_callback_t* cb, action_t* a, action_state_t* s ) {
        if ( a->special )
        {
          switch ( a->data().id() )
          {
            case 190984:  // wrath
            case 394111:  // sundered firmament
            case 191034:  // starfall
            case 78674:   // starsurge
            case 274281:  // new moon
            case 274282:  // half moon
            case 274283:  // full moon
              break;
            default:
              return;
          }
        }
        cb->proc_action->execute_on_target( s->target );
      } );
  }

  if ( talent.natures_vigil.ok() )
  {
    struct natures_vigil_cb_t : public druid_cb_t
    {
      double mul;

      natures_vigil_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ), mul( p->talent.natures_vigil->effectN( 3 ).percent() )
      {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        // TODO: whitelist this if need be
        if ( s->n_targets != 1 )
          return;

        if ( !s->result_total )
          return;

        druid_cb_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        // counts overheal
        p()->buff.natures_vigil->current_value += s->result_total * mul;
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.natures_vigil->name_cstr();
    driver->spell_id = talent.natures_vigil->id();
    driver->cooldown_ = 0_ms;
    driver->proc_flags2_ = PF2_ALL_HIT;

    if ( specialization() == DRUID_RESTORATION )
    {
      driver->proc_flags_ = ( PF_MAGIC_HEAL | PF_HELPFUL_PERIODIC );
      driver->proc_flags2_ |= PF2_PERIODIC_HEAL;
    }
    else
    {
      driver->proc_flags_ = talent.natures_vigil->proc_flags() & ~( PF_MAGIC_HEAL | PF_HELPFUL_PERIODIC );
      driver->proc_flags2_ |= PF2_PERIODIC_DAMAGE;
    }

    special_effects.push_back( driver );

    auto cb = new natures_vigil_cb_t( this, *driver );
    cb->initialize();
    cb->deactivate();

    buff.natures_vigil->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
      if ( new_  )
        cb->activate();
      else
        cb->deactivate();
    } );
  }

  if ( talent.protector_of_the_pack.ok() )
  {
    using amt_fn = std::function<double( action_state_t* )>;

    struct protector_of_the_pack_cb_t : public druid_cb_t
    {
      amt_fn amount;
      buff_t* buff;
      double mul;
      double cap_coeff = 0.0;

      protector_of_the_pack_cb_t( druid_t* p, const special_effect_t& e, amt_fn f )
        : druid_cb_t( p, e ),
          amount( std::move( f ) ),
          buff( p->buff.protector_of_the_pack ),
          mul( p->talent.protector_of_the_pack->effectN( 1 ).percent() )
      {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( !amount( s ) )
          return;

        druid_cb_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        if ( !buff->check() )
          buff->trigger();

        auto cap = p()->cache.spell_power( SCHOOL_MAX ) * cap_coeff;
        auto cur = buff->check_value();

        if ( cur < cap )
          buff->current_value = std::min( cap, cur + amount( s ) * mul );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.protector_of_the_pack->name_cstr();
    driver->spell_id = talent.protector_of_the_pack->id();
    driver->proc_flags2_ = PF2_ALL_HIT;

    amt_fn func = []( action_state_t* s ) { return s->result_amount; };

    if ( specialization() == DRUID_RESTORATION )
    {
      driver->proc_flags_ = ( PF_MAGIC_HEAL | PF_HELPFUL_PERIODIC );
      driver->proc_flags2_ |= PF2_PERIODIC_HEAL;

      if ( sim->count_overheal_as_heal )
      {
        func = []( action_state_t* s ) { return s->result_total; };
      }
    }
    else
    {
      driver->proc_flags_ = talent.protector_of_the_pack->proc_flags() & ~( PF_MAGIC_HEAL | PF_HELPFUL_PERIODIC );
      driver->proc_flags2_ |= PF2_PERIODIC_DAMAGE;
    }

    special_effects.push_back( driver );

    new protector_of_the_pack_cb_t( this, *driver, func );
  }

  // Balance
  if ( talent.denizen_of_the_dream.ok() )
  {
    struct denizen_of_the_dream_cb_t : public druid_cb_t
    {
      denizen_of_the_dream_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e ) {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( a->id == p()->spec.moonfire_dmg->id() || a->id == p()->spec.sunfire_dmg->id() )
          dbc_proc_callback_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* ) override
      {
        p()->active.denizen_of_the_dream->execute();
        p()->buff.denizen_of_the_dream->trigger();
        p()->proc.denizen_of_the_dream->occur();
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.denizen_of_the_dream->name_cstr();
    driver->spell_id = talent.denizen_of_the_dream->id();
    special_effects.push_back( driver );

    new denizen_of_the_dream_cb_t( this, *driver );
  }

  if ( talent.umbral_embrace.ok() )
  {
    struct umbral_embrace_cb_t : public druid_cb_t
    {
      umbral_embrace_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e ) {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( a->get_school() == SCHOOL_ASTRAL )
          druid_cb_t::trigger( a, s );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.umbral_embrace->name_cstr();
    driver->spell_id = talent.umbral_embrace->id();
    driver->custom_buff = buff.umbral_embrace;
    special_effects.push_back( driver );

    new umbral_embrace_cb_t( this, *driver );
  }

  // Feral
  if ( talent.ashamanes_guidance.ok() && talent.incarnation_cat.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = "ashamanes_guidance_driver";
    driver->spell_id = spec.ashamanes_guidance->id();
    driver->proc_flags_ = PF_MELEE;
    driver->ppm_ = -0.95;
    special_effects.push_back( driver );

    auto cb = new dbc_proc_callback_t( this, *driver );

    callbacks.register_callback_execute_function( driver->spell_id,
        [ dur = timespan_t::from_seconds( spec.ashamanes_guidance->effectN( 1 ).base_value() ),
          this ]
        ( const dbc_proc_callback_t*, action_t*, action_state_t* ) {
          buff.incarnation_cat->trigger( dur );
          proc.ashamanes_guidance->occur();
        } );

    buff.incarnation_cat->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
      if ( new_ )
        cb->deactivate();
      else
        cb->activate();
    } );
  }

  if ( sets->has_set_bonus( DRUID_FERAL, T30, B2 ) )
  {
    auto spell = sets->set( DRUID_FERAL, T30, B2 );

    const auto driver = new special_effect_t( this );
    driver->name_str = spell->name_cstr();
    driver->spell_id = spell->id();
    driver->custom_buff = buff.shadows_of_the_predator;
    special_effects.push_back( driver );

    new dbc_proc_callback_t( this, *driver );
  }

  // Guardian
  if ( talent.elunes_favored.ok() )
  {
    struct elunes_favored_cb_t : public druid_cb_t
    {
      double mul;

      elunes_favored_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ), mul( p->talent.elunes_favored->effectN( 1 ).percent() )
      {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( p()->get_form() != BEAR_FORM || !s->result_amount )
          return;

        // Elune's Favored heals off both arcane & nature damage
        if ( !dbc::has_common_school( a->get_school(), SCHOOL_ASTRAL ) )
          return;

        druid_cb_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        p()->buff.elunes_favored->current_value += s->result_amount * mul;
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.elunes_favored->name_cstr();
    driver->spell_id = spec.elunes_favored->id();
    special_effects.push_back( driver );

    new elunes_favored_cb_t( this, *driver );
  }

  if ( talent.galactic_guardian.ok() )
  {
    struct galactic_guardian_cb_t : public druid_cb_t
    {
      galactic_guardian_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e ) {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( a->id != p()->spec.moonfire_dmg->id() && a->id != p()->spec.moonfire->id() )
          dbc_proc_callback_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        p()->active.galactic_guardian->execute_on_target( s->target );
        p()->proc.galactic_guardian->occur();
        make_event( p()->sim, [ this ]() { p()->buff.galactic_guardian->trigger(); } );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.galactic_guardian->name_cstr();
    driver->spell_id = talent.galactic_guardian->id();
    special_effects.push_back( driver );

    new galactic_guardian_cb_t( this, *driver );
  }

  // blanket proc callback initialization happens here. anything further needs to be individually initialized
  player_t::init_special_effects();
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
    case DRUID_FERAL:       apl_feral();                                  break;
    case DRUID_BALANCE:     is_ptr() ? apl_balance_ptr() : apl_balance(); break;
    case DRUID_GUARDIAN:    apl_guardian();                               break;
    case DRUID_RESTORATION: apl_restoration();                            break;
    default:                apl_default();                                break;
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
  snapshot_mastery();
  persistent_event_delay.clear();
  astral_power_decay = nullptr;
  dot_list.moonfire.clear();
  dot_list.sunfire.clear();
  dire_fixation_target = nullptr;
}

// druid_t::merge ===========================================================
void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0; i < counters.size(); i++ )
    counters[ i ]->merge( *od.counters[ i ] );

  if ( predator_revealed_stacks )
    predator_revealed_stacks->merge( *od.predator_revealed_stacks );

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

  if ( predator_revealed_stacks )
    predator_revealed_stacks->analyze();

  // GG is a major portion of guardian druid damage but skews moonfire reporting because gg has no execute time. We
  // adjust by removing the gg amount from mf stat and re-calculating dpe and dpet for moonfire.
  if ( talent.galactic_guardian.ok() )
  {
    auto mf = find_action( "moonfire" );
    auto gg = find_action( "galactic_guardian" );
    if ( mf && gg )
    {
      auto mf_amt = mf->stats->compound_amount;
      auto gg_amt = gg->stats->compound_amount;
      auto mod = ( mf_amt - gg_amt ) / mf_amt;

      mf->stats->ape *= mod;
      mf->stats->apet *= mod;
    }
  }
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

double druid_t::resource_gain( resource_e r, double amount, gain_t* g, action_t* a )
{
  auto actual = player_t::resource_gain( r, amount, g, a );
  auto over = amount - actual;

  if ( r == RESOURCE_COMBO_POINT && g != gain.overflowing_power && over > 0 && buff.b_inc_cat->check() )
  {
    auto avail = std::min( over, as<double>( buff.overflowing_power->max_stack() - buff.overflowing_power->check() ) );
    if ( avail > 0 )
      g->overflow[ r ] -= avail;

    buff.overflowing_power->trigger( as<int>( over ) );
  }

  return actual;
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
  {
    persistent_event_delay.push_back( make_event<persistent_delay_event_t>( *sim, this, buff.lycaras_teachings ) );

    register_combat_begin( [ this ]( player_t* ) { buff.lycaras_teachings->trigger(); } );
  }

  if ( timeofday == timeofday_e::DAY_TIME )
    buff.rising_light_falling_night_day->trigger();
  else
    buff.rising_light_falling_night_night->trigger();

  buff.elunes_favored->trigger();
  buff.natures_balance->trigger();

  if ( talent.orbit_breaker.ok() )
  {
    auto stacks = options.initial_orbit_breaker_stacks >= 0
                      ? options.initial_orbit_breaker_stacks
                      : rng().range( 0, as<int>( talent.orbit_breaker->effectN( 1 ).base_value() ) );

    if ( stacks )
      buff.orbit_breaker->trigger( stacks );
  }

  if ( active.shooting_stars )
  {
    persistent_event_delay.push_back( make_event<persistent_delay_event_t>( *sim, this, buff.shooting_stars_moonfire ) );

    if ( talent.sunfire.ok() )
      persistent_event_delay.push_back( make_event<persistent_delay_event_t>( *sim, this, buff.shooting_stars_sunfire ) );
  }

  if ( active.yseras_gift )
    persistent_event_delay.push_back( make_event<persistent_delay_event_t>( *sim, this, buff.yseras_gift ) );

  if ( specialization() == DRUID_BALANCE )
  {
    if ( options.initial_pulsar_value > 0 )
    {
      register_combat_begin( [ this ]( player_t* p ) {
        buff.primordial_arcanic_pulsar->trigger();
        buff.primordial_arcanic_pulsar->current_value = options.initial_pulsar_value;
      } );
    }

    persistent_event_delay.push_back( make_event<persistent_delay_event_t>( *sim, this, [ this ]() {
      snapshot_mastery();
      make_repeating_event( *sim, 5.25_s, [ this ]() { snapshot_mastery(); } );
    }, 5.25_s ) );

    // This MUST come last so that any previous changes from other precombat events, such as lycaras update based on
    // form, happens first
    register_combat_begin( [ this ]( player_t* ) { snapshot_mastery(); } );
  }
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

    if ( !options.raid_combat )
      in_boss_encounter = 0;

    if ( in_boss_encounter )
    {
      double cap =
          std::max( resources.base[ RESOURCE_ASTRAL_POWER ] * talent.natures_balance->effectN( 2 ).percent(), 20.0 );
      double curr = resources.current[ RESOURCE_ASTRAL_POWER ];

      resources.current[ RESOURCE_ASTRAL_POWER ] = std::min( cap, curr );
      if ( curr > cap )
        sim->print_debug( "Astral Power capped at combat start to {} (was {})", cap, curr );
    }
  }
/*
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
    }
  }*/
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

  a *= 1.0 + buff.ursine_vigor->check_value();

  return a;
}

// Critical Strike ==========================================================
double druid_t::composite_melee_crit_chance() const
{
  return player_t::composite_melee_crit_chance() + spec.critical_strikes->effectN( 1 ).percent();
}

double druid_t::composite_spell_crit_chance() const
{
  return player_t::composite_spell_crit_chance() + spec.critical_strikes->effectN( 1 ).percent();
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

  if ( buff.rage_of_the_sleeper->check() )
    l *= 1.0 + talent.rage_of_the_sleeper->effectN( 3 ).percent();

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

  active += buff.forestwalk->check_value();

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

// Expressions ==============================================================
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

  if ( util::str_compare_ci( splits[ 0 ], "druid" ) && splits.size() == 2 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "catweave_bear" ) )
      return expr_t::create_constant( "catweave_bear", options.catweave_bear );
    if ( util::str_compare_ci( splits[ 1 ], "owlweave_bear" ) )
      return expr_t::create_constant( "owlweave_bear", options.owlweave_bear );
    if ( util::str_compare_ci( splits[ 1 ], "owlweave_cat" ) )
      return expr_t::create_constant( "owlweave_cat", options.owlweave_cat );
    if ( util::str_compare_ci( splits[ 1 ], "no_cds" ) )
      return expr_t::create_constant( "no_cds", options.no_cds );
    if ( util::str_compare_ci( splits[ 1 ], "time_spend_healing" ) )
      return expr_t::create_constant( "time_spend_healing", options.time_spend_healing );
    if ( util::str_compare_ci( splits[ 1 ], "initial_orbit_breaker_stacks" ) )
      return expr_t::create_constant( "initial_orbit_breaker_stacks", options.initial_orbit_breaker_stacks );
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
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.state == ANY_NEXT; } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_any" ) )
      {
        return make_fn_expr( name_str, [ this ]() {
          return eclipse_handler.state == IN_SOLAR || eclipse_handler.state == IN_LUNAR ||
                 eclipse_handler.state == IN_BOTH;
        } );
      }
      else if ( util::str_compare_ci( splits[ 1 ], "in_solar" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.state == IN_SOLAR; } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_lunar" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.state == IN_LUNAR; } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_both" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.state == IN_BOTH; } );
      else if ( util::str_compare_ci( splits[ 1 ], "starfire_counter" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.starfire_counter; } );
      else if ( util::str_compare_ci( splits[ 1 ], "wrath_counter" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.wrath_counter; } );
    }
  }
  else if ( specialization() == DRUID_FERAL )
  {
    if ( splits[ 0 ] == "action" && splits[ 1 ] == "ferocious_bite_max" && splits[ 2 ] == "damage" )
    {
      action_t* action = find_action( "ferocious_bite" );

      return make_fn_expr( name_str, [ action, this ]() -> double {
        action_state_t* state = action->get_state();
        state->n_targets = 1;
        state->chain_target = 0;
        state->result = RESULT_HIT;

        action->snapshot_state( state, result_amount_type::DMG_DIRECT );
        state->target = action->target;
        //  (p()->resources.current[RESOURCE_ENERGY] - cat_attack_t::cost()));

        // combo_points = p()->resources.current[RESOURCE_COMBO_POINT];
        double current_energy = this->resources.current[ RESOURCE_ENERGY ];
        double current_cp = this->resources.current[ RESOURCE_COMBO_POINT ];
        this->resources.current[ RESOURCE_ENERGY ] = 50;
        this->resources.current[ RESOURCE_COMBO_POINT ] = 5;

        double amount;
        state->result_amount = action->calculate_direct_amount( state );
        state->target->target_mitigation( action->get_school(), result_amount_type::DMG_DIRECT, state );
        amount = state->result_amount;
        amount *= 1.0 + clamp( state->crit_chance + state->target_crit_chance, 0.0, 1.0 ) *
                            action->composite_player_critical_multiplier( state );

        this->resources.current[ RESOURCE_ENERGY ] = current_energy;
        this->resources.current[ RESOURCE_COMBO_POINT ] = current_cp;

        action_state_t::release( state );
        return amount;
      } );
    }

    if ( util::str_compare_ci( name_str, "active_bt_triggers" ) )
    {
      return make_fn_expr( "active_bt_triggers", [ this ]() {
        return buff.bt_rake->check() + buff.bt_shred->check() + buff.bt_swipe->check() + buff.bt_thrash->check() +
               buff.bt_moonfire->check() + buff.bt_brutal_slash->check() + buff.bt_feral_frenzy->check();
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
            buff.bt_feral_frenzy->check() ? buff.bt_feral_frenzy->remains().total_seconds() : 6.0
        } ) );
      } );
    }

    if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "moonfire_cat" ) )
    {
      splits[ 1 ] = "lunar_inspiration";
      return druid_t::create_expression( util::string_join( splits, "." ) );
    }
  }

  // Convert [talent/buff/cooldown].incarnation.* to spec-based incarnations
  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "incarnation" ) &&
       ( util::str_compare_ci( splits[ 0 ], "buff" ) || util::str_compare_ci( splits[ 0 ], "talent" ) ||
         util::str_compare_ci( splits[ 0 ], "cooldown" ) ) )
  {
    switch ( specialization() )
    {
      case DRUID_BALANCE:     splits[ 1 ] = "incarnation_chosen_of_elune";    break;
      case DRUID_FERAL:       splits[ 1 ] = "incarnation_avatar_of_ashamane"; break;
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

  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "bs_inc" ) )
  {
    if ( specialization() == DRUID_FERAL )
    {
      if ( talent.incarnation_cat.ok() )
        splits[ 1 ] = "incarnation_avatar_of_ashamane";
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

  return player_t::create_expression( name_str );
}

static bool parse_swarm_setup( sim_t* sim, std::string_view, std::string_view setup_str )
{
  sim->error( "druid.adaptive_swarm_prepull_setup is temporarily disabled." );
  return false;

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

      static_cast<druid_t*>( sim->active_player )
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

  add_option( opt_float( "druid.adaptive_swarm_jump_distance_melee", options.adaptive_swarm_jump_distance_melee ) );
  add_option( opt_float( "druid.adaptive_swarm_jump_distance_ranged", options.adaptive_swarm_jump_distance_ranged ) );
  add_option( opt_float( "druid.adaptive_swarm_jump_distance_stddev", options.adaptive_swarm_jump_distance_stddev ) );
  add_option( opt_uint( "druid.adaptive_swarm_melee_targets", options.adaptive_swarm_melee_targets, 1U, 29U ) );
  add_option( opt_uint( "druid.adaptive_swarm_ranged_targets", options.adaptive_swarm_ranged_targets, 1U, 29U ) );
  add_option( opt_func( "druid.adaptive_swarm_prepull_setup", parse_swarm_setup ) );

  // Balance
  add_option( opt_float( "druid.initial_astral_power", options.initial_astral_power ) );
  add_option( opt_int( "druid.initial_moon_stage", options.initial_moon_stage ) );
  add_option( opt_float( "druid.initial_pulsar_value", options.initial_pulsar_value ) );
  add_option( opt_int( "druid.initial_orbit_breaker_stacks", options.initial_orbit_breaker_stacks ) );

  // Feral
  add_option( opt_float( "druid.predator_rppm", options.predator_rppm_rate ) );
  add_option( opt_bool( "druid.owlweave_cat", options.owlweave_cat ) );

  // Guardian
  add_option( opt_bool( "druid.catweave_bear", options.catweave_bear ) );
  add_option( opt_bool( "druid.owlweave_bear", options.owlweave_bear ) );

  // Restoration
  add_option( opt_float( "druid.time_spend_healing", options.time_spend_healing ) );
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

  player_t::init_absorb_priority();

  absorb_priority.push_back( talent.earthwarden->id() );
  absorb_priority.push_back( talent.rage_of_the_sleeper->id() );  // TODO confirm priority
  absorb_priority.push_back( talent.brambles->id() );
}

void druid_t::target_mitigation( school_e school, result_amount_type type, action_state_t* s )
{
  s->result_amount *= 1.0 + buff.barkskin->value();

  s->result_amount *= 1.0 + buff.survival_instincts->value();

  s->result_amount *= 1.0 + talent.thick_hide->effectN( 1 ).percent();

  s->result_amount *= 1.0 + buff.overpowering_aura->check_value();

  if ( talent.protective_growth.ok() )
    s->result_amount *= 1.0 + buff.protective_growth->value();

  if ( spec.ursine_adept->ok() && buff.bear_form->check() )
    s->result_amount *= 1.0 + spec.ursine_adept->effectN( 2 ).percent();

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

    if ( buff.moonkin_form->check() && s->result_type == result_amount_type::DMG_DIRECT )
      buff.owlkin_frenzy->trigger();

    buff.cenarion_ward->expire();

    if ( buff.bristling_fur->up() )  // 1 rage per 1% of maximum health taken
      resource_gain( RESOURCE_RAGE, s->result_amount / expected_max_health * 100, gain.bristling_fur );

    if ( talent.dream_of_cenarius.ok() && s->result_type == result_amount_type::DMG_DIRECT )
      buff.dream_of_cenarius->trigger( -1, buff_t::DEFAULT_VALUE(), cache.attack_crit_chance() );

    if ( active.furious_regeneration )
    {
      buff.furious_regeneration->trigger();

      auto heal_pct = sets->set( DRUID_GUARDIAN, T30, B2 )->effectN( 1 ).percent();

      residual_action::trigger( active.furious_regeneration, this, s->result_amount * heal_pct );
    }
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

  hots.adaptive_swarm_heal   = target.get_dot( "adaptive_swarm_heal", &source );
  hots.cenarion_ward         = target.get_dot( "cenarion_ward", &source );
  hots.cultivation           = target.get_dot( "cultivation", &source );
  hots.frenzied_regeneration = target.get_dot( "frenzied_regeneration", &source );
  hots.germination           = target.get_dot( "germination", &source );
  hots.lifebloom             = target.get_dot( "lifebloom", &source );
  hots.regrowth              = target.get_dot( "regrowth", &source );
  hots.rejuvenation          = target.get_dot( "rejuvenation", &source );
  hots.spring_blossoms       = target.get_dot( "spring_blossoms", &source );
  hots.wild_growth           = target.get_dot( "wild_growth", &source );

  debuff.dire_fixation = make_buff( *this, "dire_fixation", find_trigger( source.talent.dire_fixation ).trigger() );

  debuff.pulverize = make_buff( *this, "pulverize_debuff", source.talent.pulverize )
    ->set_cooldown( 0_ms )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_TO_CASTER )
    ->apply_affecting_aura( source.talent.circle_of_life_and_death );

  debuff.tooth_and_claw = make_buff( *this, "tooth_and_claw_debuff",
        find_trigger( find_trigger( source.talent.tooth_and_claw ).trigger() ).trigger() )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_TO_CASTER )
    ->set_stack_change_callback( [ & ]( buff_t* b, int, int new_ ) {
      source.uptime.tooth_and_claw_debuff->update( new_, b->sim->current_time() );
    } );

  debuff.waning_twilight = make_buff( *this, "waning_twilight", source.spec.waning_twilight )
    ->set_chance( 1.0 )
    ->set_duration( 0_ms );

  buff.ironbark = make_buff( *this, "ironbark", source.talent.ironbark )
    ->set_cooldown( 0_ms );
}

int druid_td_t::hots_ticking() const
{
  auto count = hots.adaptive_swarm_heal->is_ticking() + hots.cenarion_ward->is_ticking() +
               hots.cultivation->is_ticking() + hots.germination->is_ticking() + hots.regrowth->is_ticking() +
               hots.rejuvenation->is_ticking() + hots.spring_blossoms->is_ticking() + hots.wild_growth->is_ticking();

  auto lb_mul = 1 + as<int>( static_cast<druid_t*>( source )->talent.harmonious_blooming->effectN( 1 ).base_value() );
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

const spell_data_t* druid_t::apply_override( const spell_data_t* base, const spell_data_t* passive )
{
  if ( !passive->ok() )
    return base;

  return find_spell( as<unsigned>( find_effect( passive, base, A_OVERRIDE_ACTION_SPELL ).base_value() ) );
}

// Eclipse Handler ==========================================================
void eclipse_handler_t::init()
{
  if ( !p->talent.eclipse.ok() )
    return;

  state = ANY_NEXT;

  wrath_counter_base = starfire_counter_base = as<unsigned>( p->talent.eclipse->effectN( 1 ).base_value() );

  size_t res = 4;
  bool foe = p->talent.fury_of_elune.ok();
  bool nm = p->talent.new_moon.ok();
  bool hm = nm;
  bool fm = nm || p->talent.convoke_the_spirits.ok();

  data.arrays.reserve( res + foe + nm + hm + fm );
  data.wrath = &data.arrays.emplace_back();
  data.starfire = &data.arrays.emplace_back();
  data.starsurge = &data.arrays.emplace_back();
  data.starfall = &data.arrays.emplace_back();
  if ( foe )
    data.fury_of_elune = &data.arrays.emplace_back();
  if ( nm )
    data.new_moon = &data.arrays.emplace_back();
  if ( hm )
    data.half_moon = &data.arrays.emplace_back();
  if ( fm )
    data.full_moon = &data.arrays.emplace_back();

  iter.arrays.reserve( res + foe + nm + hm + fm );
  iter.wrath = &iter.arrays.emplace_back();
  iter.starfire = &iter.arrays.emplace_back();
  iter.starsurge = &iter.arrays.emplace_back();
  iter.starfall = &iter.arrays.emplace_back();
  if ( foe )
    iter.fury_of_elune = &iter.arrays.emplace_back();
  if ( nm )
    iter.new_moon = &iter.arrays.emplace_back();
  if ( hm )
    iter.half_moon = &iter.arrays.emplace_back();
  if ( fm )
    iter.full_moon = &iter.arrays.emplace_back();
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

void eclipse_handler_t::cast_moon( moon_stage_e moon )
{
  if ( !enabled() ) return;

  if ( moon == moon_stage_e::NEW_MOON && iter.new_moon )
    ( *iter.new_moon )[ state ]++;
  else if ( moon == moon_stage_e::HALF_MOON && iter.half_moon )
    ( *iter.half_moon )[ state ]++;
  else if ( moon == moon_stage_e::FULL_MOON && iter.full_moon )
    ( *iter.full_moon )[ state ]++;
}

void eclipse_handler_t::tick_starfall()
{
  if ( !enabled() ) return;

  if ( iter.starfall )
    ( *iter.starfall )[ state ]++;
}

void eclipse_handler_t::tick_fury_of_elune()
{
  if ( !enabled() ) return;

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
        p->buff.balance_of_all_things_nature->trigger();
        p->buff.touch_the_cosmos->trigger();
        p->buff.parting_skies->trigger();
        p->buff.solstice->trigger();

        state = IN_SOLAR;
        p->uptime.eclipse_none->update( false, p->sim->current_time() );
        p->uptime.eclipse_solar->update( true, p->sim->current_time() );
        reset_stacks();
      }
      else if ( !wrath_counter )
      {
        p->buff.eclipse_lunar->trigger();
        p->buff.balance_of_all_things_arcane->trigger();
        p->buff.touch_the_cosmos->trigger();
        p->buff.parting_skies->trigger();
        p->buff.solstice->trigger();

        state = IN_LUNAR;
        p->uptime.eclipse_none->update( false, p->sim->current_time() );
        p->uptime.eclipse_lunar->update( true, p->sim->current_time() );
        reset_stacks();
      }
      break;

    case IN_SOLAR:
      p->buff.natures_grace->trigger();

      state = ANY_NEXT;
      p->uptime.eclipse_solar->update( false, p->sim->current_time() );
      p->uptime.eclipse_none->update( true, p->sim->current_time() );
      break;

    case IN_LUNAR:
      p->buff.natures_grace->trigger();

      state = ANY_NEXT;
      p->uptime.eclipse_lunar->update( false, p->sim->current_time() );
      p->uptime.eclipse_none->update( true, p->sim->current_time() );
      break;

    case IN_BOTH:
      p->buff.natures_grace->trigger();

      state = ANY_NEXT;
      p->uptime.eclipse_solar->update( false, p->sim->current_time() );
      p->uptime.eclipse_lunar->update( false, p->sim->current_time() );
      p->uptime.eclipse_none->update( true, p->sim->current_time() );
      break;

    default:
      assert( true && "eclipse_handler_t::advance_eclipse() at unknown state" );
      break;
  }
}

void eclipse_handler_t::trigger_both( timespan_t d = 0_ms )
{
  if ( !enabled() ) return;

  p->buff.eclipse_lunar->trigger( d );
  p->buff.eclipse_solar->trigger( d );
  p->buff.balance_of_all_things_arcane->trigger();
  p->buff.balance_of_all_things_nature->trigger();
  p->buff.touch_the_cosmos->trigger();
  p->buff.parting_skies->trigger();
  p->buff.parting_skies->trigger();
  p->buff.solstice->trigger();
  if ( state != ANY_NEXT && !p->is_ptr() )
    p->buff.natures_grace->trigger();

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

  p->buff.balance_of_all_things_arcane->trigger();
  p->buff.balance_of_all_things_nature->trigger();
  p->buff.touch_the_cosmos->trigger();
  p->buff.parting_skies->trigger();
  p->buff.parting_skies->trigger();
  p->buff.solstice->trigger();
  if ( !p->is_ptr() )
    p->buff.natures_grace->trigger();
}

void eclipse_handler_t::expire_both()
{
  p->buff.eclipse_solar->expire();
  p->buff.eclipse_lunar->expire();
}

void eclipse_handler_t::reset_stacks()
{
  wrath_counter    = wrath_counter_base;
  starfire_counter = starfire_counter_base;
}

void eclipse_handler_t::reset_state()
{
  if ( !enabled() ) return;

  state = ANY_NEXT;
}

void eclipse_handler_t::datacollection_begin()
{
  if ( !enabled() ) return;

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
  if ( !enabled() ) return;

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
  if ( !enabled() ) return;

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

  options = static_cast<druid_t*>( source )->options;
  prepull_swarm = static_cast<druid_t*>( source )->prepull_swarm;
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
  action.apply_affecting_aura( talent.improved_swipe );
  action.apply_affecting_aura( talent.incessant_tempest );
  action.apply_affecting_aura( talent.killer_instinct );
  action.apply_affecting_aura( talent.nurturing_instinct );

  // Multi-spec
  action.apply_affecting_aura( talent.circle_of_life_and_death );

  // Balance
  action.apply_affecting_aura( talent.cosmic_rapidity );
  action.apply_affecting_aura( talent.elunes_guidance );
  action.apply_affecting_aura( talent.lunar_shrapnel );
  action.apply_affecting_aura( talent.orbital_strike );
  action.apply_affecting_aura( talent.power_of_goldrinn );
  action.apply_affecting_aura( talent.radiant_moonlight );
  action.apply_affecting_aura( talent.twin_moons );
  action.apply_affecting_aura( talent.wild_surges );
  action.apply_affecting_aura( sets->set( DRUID_BALANCE, T30, B2 ) );
  
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
  action.apply_affecting_aura( talent.flashing_claws );
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
  action.apply_affecting_aura( sets->set( DRUID_GUARDIAN, T30, B4 ) );

  // elune's favored applies to periodic effects via script, and fury of nature only applies to those spells that are
  // affected by elune's favored
  if ( action.data().affected_by_all( spec.elunes_favored->effectN( 1 ) ) )
  {
    auto apply_effect_ = [ &action ]( const spell_data_t* s, double v ) {
      action.base_dd_multiplier *= 1.0 + v;
      action.sim->print_debug( "{} base_dd_multiplier modified by {} from {}", action.name(), v, s->name_cstr() );

      action.base_td_multiplier *= 1.0 + v;
      action.sim->print_debug( "{} base_td_multiplier modified by {} from {}", action.name(), v, s->name_cstr() );
    };

    if ( talent.elunes_favored.ok() )
      apply_effect_( spec.elunes_favored, spec.elunes_favored->effectN( 1 ).percent() );

    if ( talent.fury_of_nature.ok() )
      apply_effect_( talent.fury_of_nature, talent.fury_of_nature->effectN( 1 ).percent() );
  }

  // Restoration
  action.apply_affecting_aura( spec.cenarius_guidance );
  action.apply_affecting_aura( talent.germination );
  action.apply_affecting_aura( talent.improved_ironbark );
  action.apply_affecting_aura( talent.inner_peace );
  action.apply_affecting_aura( talent.natural_recovery );
  action.apply_affecting_aura( talent.passing_seasons );
  action.apply_affecting_aura( talent.rampant_growth );
  action.apply_affecting_aura( talent.sabertooth );
  action.apply_affecting_aura( talent.soul_of_the_forest_cat );
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

// snapshotted mastery for balance druids
double druid_t::cache_mastery_value() const
{
  return cache_mastery_snapshot;
}

void druid_t::snapshot_mastery()
{
  cache_mastery_snapshot = cache.mastery_value();
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
    {
      os << "<div class=\"player-section custom_section\">\n";

      feral_snapshot_table( os );

      if ( p.predator_revealed_stacks )
        predator_revealed_histogram( os );

      os << "</div>\n";
    }

    if ( p.specialization() == DRUID_BALANCE && p.eclipse_handler.enabled() )
    {
      os << "<div class=\"player-section custom_section\">\n";

      balance_eclipse_table( os );

      os << "</div>\n";
    }
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
    os << "<h3 class=\"toggle open\">Eclipse Utilization</h3>\n"
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

  void predator_revealed_histogram( report::sc_html_stream& os )
  {
    // Write header
    os << "<h3 class=\"toggle open\">Predator Revealed Chart</h3>\n"
       << "<div class=\"toggle-content\">\n";

    auto& d = *p.predator_revealed_stacks;
    int num_buckets = 8;
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "predator_revealed_stacks" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "When Predator Revealed", d.mean(), 5, 12 ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> stacks<br/>" );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    // Write footer
    os << "</div>\n";
  }

  void feral_snapshot_table( report::sc_html_stream& os )
  {
    // Write header
    os << "<h3 class=\"toggle open\">Snapshot Table</h3>\n"
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
      os.format( R"(<tr><td class="left">{}</td><td class="right">{:.2f}%</td><td class="right">{:.2f}%</td>)",
                 report_decorators::decorated_action( *data.action ), data.tf_exe * 100, data.tf_tick * 100 );

      if ( p.talent.bloodtalons.ok() )
        os.format( R"(<td class="right">{:.2f}%</td><td class="right">{:.2f}</td>)", data.bt_exe * 100, data.bt_tick * 100 );

      os << "</tr>\n";
    }

    // Write footer
    os << "</table>\n"
       << "</div>\n";
  }

private:
  druid_t& p;
};

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

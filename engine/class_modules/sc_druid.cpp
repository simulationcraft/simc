// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "player/covenant.hpp"

namespace { // UNNAMED NAMESPACE
// ==========================================================================
// Druid
// ==========================================================================

/*
  Shadowlands TODO:

  https://docs.google.com/spreadsheets/d/e/2PACX-1vTpZmGwHpYIxSuKBuU8xilbWW6g0JErLbcxWYfPx_7bY6VSdVzjzCx29xtD7JgSlm2qJ7trlir_tatN/pubhtml
*/

// Forward declarations
struct druid_t;

enum form_e
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
};

// Azerite Trait
enum streaking_stars_e
{
  SS_NONE,
  // Spells
  SS_STARFIRE,
  SS_WRATH,
  SS_MOONFIRE,
  SS_SUNFIRE,
  SS_STARSURGE,
  SS_STELLAR_FLARE,
  SS_NEW_MOON,
  SS_HALF_MOON,
  SS_FULL_MOON,
  // These target the last hit enemy in game
  SS_STARFALL,
  SS_FORCE_OF_NATURE,
  SS_FURY_OF_ELUNE,
  SS_CELESTIAL_ALIGNMENT,
};

enum eclipse_state_e
{
  ANY_NEXT,
  IN_SOLAR,
  IN_LUNAR,
  IN_BOTH,
  SOLAR_NEXT,
  LUNAR_NEXT,
};

enum free_cast_e
{
  NONE = 0,
  CONVOKE,  // convoke_the_spirits night_fae covenant ability
  LYCARAS,  // lycaras fleeting glimpse legendary
  ONETHS,   // oneths clear vision legedary
};

struct druid_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* fury_of_elune;
    dot_t* lifebloom;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* rip;
    dot_t* stellar_flare;
    dot_t* sunfire;
    dot_t* starfall;
    dot_t* thrash_cat;
    dot_t* thrash_bear;
    dot_t* wild_growth;

    dot_t* adaptive_swarm_damage;
    dot_t* adaptive_swarm_heal;

    dot_t* frenzied_assault;
  } dots;

  struct buffs_t
  {
    buff_t* lifebloom;
  } buff;

  druid_td_t( player_t& target, druid_t& source );

  bool hot_ticking()
  {
    return dots.regrowth->is_ticking() || dots.rejuvenation->is_ticking() || dots.lifebloom->is_ticking() ||
           dots.wild_growth->is_ticking();
  }

  bool dot_ticking()
  {
    return dots.moonfire->is_ticking() || dots.rake->is_ticking() || dots.rip->is_ticking() ||
           dots.stellar_flare->is_ticking() || dots.sunfire->is_ticking() || dots.thrash_bear->is_ticking() ||
           dots.thrash_cat->is_ticking();
  }
};

struct druid_action_state_t : public action_state_t
{
  free_cast_e free_cast;

  druid_action_state_t( action_t* a, player_t* t, free_cast_e f = free_cast_e::NONE )
    : action_state_t( a, t ), free_cast( f )
  {}

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    const druid_action_state_t* druid_s = debug_cast<const druid_action_state_t*>( s );

    free_cast = druid_s->free_cast;
  }
};

struct snapshot_counter_t
{
  const sim_t* sim;
  druid_t* p;
  std::vector<buff_t*> b;
  double exe_up;
  double exe_down;
  double tick_up;
  double tick_down;
  bool is_snapped;
  double wasted_buffs;

  snapshot_counter_t( druid_t* player, buff_t* buff );

  bool check_all()
  {
    double n_up = 0;
    for ( auto& elem : b )
    {
      if ( elem->check() ) n_up++;
    }
    if ( n_up == 0 ) return false;

    wasted_buffs += n_up - 1;
    return true;
  }

  void add_buff( buff_t* buff ) { b.push_back( buff ); }

  void count_execute()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim->current_iteration == 0 && sim->iterations > sim->threads && !sim->debug && !sim->log ) return;

    check_all() ? ( exe_up++, is_snapped = true ) : ( exe_down++, is_snapped = false );
  }

  void count_tick()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim->current_iteration == 0 && sim->iterations > sim->threads && !sim->debug && !sim->log ) return;

    is_snapped ? tick_up++ : tick_down++;
  }

  double divisor() const
  {
    if ( !sim->debug && !sim->log && sim->iterations > sim->threads )
      return sim->iterations - sim->threads;
    else
      return std::min( sim->iterations, sim->threads );
  }

  double mean_exe_up() const { return exe_up / divisor(); }

  double mean_exe_down() const { return exe_down / divisor(); }

  double mean_tick_up() const { return tick_up / divisor(); }

  double mean_tick_down() const { return tick_down / divisor(); }

  double mean_exe_total() const { return ( exe_up + exe_down ) / divisor(); }

  double mean_tick_total() const { return ( tick_up + tick_down ) / divisor(); }

  double mean_waste() const { return wasted_buffs / divisor(); }

  void merge( const snapshot_counter_t& other )
  {
    exe_up += other.exe_up;
    exe_down += other.exe_down;
    tick_up += other.tick_up;
    tick_down += other.tick_down;
    wasted_buffs += other.wasted_buffs;
  }
};

struct eclipse_handler_t
{
  druid_t* p;
  unsigned wrath_counter;
  unsigned starfire_counter;
  eclipse_state_e state;

  eclipse_handler_t( druid_t* player ) : p( player ), wrath_counter( 2 ), starfire_counter( 2 ), state( ANY_NEXT ) {}

  void cast_wrath();
  void cast_starfire();
  void cast_starsurge();

  void advance_eclipse();

  void trigger_both( timespan_t );
  void expire_both();

  void reset_stacks();
  void reset_state();
};

struct druid_t : public player_t
{
private:
  form_e form;  // Active druid form
public:
  moon_stage_e moon_stage;
  eclipse_handler_t eclipse_handler;
  // counters for snapshot tracking
  std::vector<snapshot_counter_t*> counters;

  double expected_max_health;  // For Bristling Fur calculations.

  // Azerite
  streaking_stars_e previous_streaking_stars;
  double lucid_dreams_proc_chance_balance;
  double lucid_dreams_proc_chance_feral;
  double lucid_dreams_proc_chance_guardian;
  double vision_of_perfection_dur;
  double vision_of_perfection_cdr;

  // RPPM objects
  struct rppms_t
  {
    // Feral
    real_ppm_t* predator;    // Optional RPPM approximation
    real_ppm_t* blood_mist;  // Azerite trait

    // Balance
    real_ppm_t* power_of_the_moon;
  } rppm;

  // Options
  double predator_rppm_rate;
  double initial_astral_power;
  double thorns_attack_period;
  double thorns_hit_chance;
  int initial_moon_stage;
  int lively_spirit_stacks;  // to set how many spells a healer will cast during Innervate
  bool catweave_bear;
  bool affinity_resources;  // activate resources tied to affinities
  double kindred_empowerment_ratio;
  double convoke_the_spirits_heals;
  double convoke_the_spirits_ultimate;

  struct active_actions_t
  {
    heal_t* cenarion_ward_hot;
    action_t* brambles;
    action_t* brambles_pulse;
    spell_t* galactic_guardian;
    action_t* natures_guardian;
    spell_t* shooting_stars;
    action_t* yseras_gift;

    // Azerite
    spell_t* lunar_shrapnel;
    spell_t* streaking_stars;

    // Covenant
    spell_t* kindred_empowerment;
    spell_t* kindred_empowerment_partner;

    // Legendary
    action_t* frenzied_assault;
    action_t* lycaras_fleeting_glimpse;  // fake holder action for reporting
    action_t* oneths_clear_vision;       // fake holder action for reporting
  } active;

  // Pets
  std::array<pet_t*, 3> force_of_nature;

  // Auto-attacks
  weapon_t caster_form_weapon;
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* caster_melee_attack;
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;
  double equipped_weapon_dps;

  // Druid Events
  std::vector<event_t*> persistent_buff_delay;

  // Azerite
  struct azerite_t
  {
    // Balance
    azerite_power_t lively_spirit;
    azerite_power_t dawning_sun;
    azerite_power_t lunar_shrapnel;
    azerite_power_t power_of_the_moon;
    azerite_power_t high_noon;
    azerite_power_t streaking_stars;
    azerite_power_t arcanic_pulsar;
    // Feral
    azerite_power_t blood_mist;           // check spelldata
    azerite_power_t gushing_lacerations;  // check spelldata
    azerite_power_t iron_jaws;            //-||-
    azerite_power_t primordial_rage;      //-||-
    azerite_power_t raking_ferocity;      //-||-
    azerite_power_t shredding_fury;       //-||-
    azerite_power_t wild_fleshrending;    //-||-
    azerite_power_t jungle_fury;
    azerite_power_t untamed_ferocity;

    // Guardian
    azerite_power_t guardians_wrath;
    azerite_power_t layered_mane;  // TODO: check if second Ironfur benefits from Guardian of Elune
    azerite_power_t masterful_instincts;
    azerite_power_t twisted_claws;
    azerite_power_t burst_of_savagery;

    // essences
    azerite_essence_t conflict_and_strife;
  } azerite;

  // azerite essence
  const spell_data_t* lucid_dreams; // Memory of Lucid Dreams R1 MINOR BASE

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* dash;
    buff_t* tiger_dash;
    buff_t* cenarion_ward;
    buff_t* clearcasting;
    buff_t* prowl;
    buff_t* stampeding_roar;
    buff_t* wild_charge_movement;
    buff_t* innervate;
    buff_t* thorns;
    buff_t* heart_of_the_wild;

    buff_t* oath_of_the_elder_druid;

    // Covenants
    buff_t* kindred_empowerment;
    buff_t* kindred_empowerment_energize;
    buff_t* ravenous_frenzy;
    buff_t* convoke_the_spirits;  // dummy buff for conduit

    // Balance
    buff_t* natures_balance;
    buff_t* celestial_alignment;
    buff_t* incarnation_moonkin;
    buff_t* moonkin_form;
    buff_t* warrior_of_elune;
    buff_t* owlkin_frenzy;
    buff_t* starfall;
    buff_t* starlord;  // talent
    buff_t* eclipse_solar;
    buff_t* eclipse_lunar;
    buff_t* starsurge;  // stacking eclipse empowerment
    buff_t* solstice;
    // Balance Legendaries
    buff_t* primordial_arcanic_pulsar;
    buff_t* oneths_free_starsurge;
    buff_t* oneths_free_starfall;
    buff_t* balance_of_all_things_nature;
    buff_t* balance_of_all_things_arcane;
    buff_t* timeworn_dreambinder;

    // Feral
    buff_t* berserk_cat;
    buff_t* bloodtalons;
    buff_t* bt_rake;          // dummy buff
    buff_t* bt_shred;         // dummy buff
    buff_t* bt_swipe;         // dummy buff
    buff_t* bt_thrash;        // dummy buff
    buff_t* bt_moonfire;      // dummy buff
    buff_t* bt_brutal_slash;  // dummy buff
    buff_t* incarnation_cat;
    buff_t* predatory_swiftness;
    buff_t* savage_roar;
    buff_t* scent_of_blood;
    buff_t* tigers_fury;
    buff_t* jungle_stalker;
    // Feral Legendaries
    buff_t* eye_of_fearful_symmetry;
    buff_t* apex_predators_craving;
    buff_t* sudden_ambush;

    // Guardian
    buff_t* barkskin;
    buff_t* bristling_fur;
    buff_t* berserk_bear;
    buff_t* earthwarden;
    buff_t* earthwarden_driver;
    buff_t* galactic_guardian;
    buff_t* gore;
    buff_t* guardian_of_elune;
    buff_t* incarnation_bear;
    buff_t* ironfur;
    buff_t* pulverize;
    buff_t* survival_instincts;
    buff_t* guardians_wrath;
    buff_t* masterful_instincts;
    buff_t* twisted_claws;
    buff_t* burst_of_savagery;
    buff_t* sharpened_claws;
    buff_t* savage_combatant;

    // Restoration
    buff_t* incarnation_tree;
    buff_t* soul_of_the_forest;  // needs checking
    buff_t* yseras_gift;
    buff_t* harmony;  // NYI

    // Azerite
    buff_t* dawning_sun;
    buff_t* lively_spirit;
    buff_t* shredding_fury;
    buff_t* iron_jaws;
    buff_t* raking_ferocity;
    buff_t* arcanic_pulsar;
    buff_t* jungle_fury;
    buff_t* strife_doubled;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk;
    cooldown_t* celestial_alignment;
    cooldown_t* innervate;
    cooldown_t* growl;
    cooldown_t* incarnation;
    cooldown_t* mangle;
    cooldown_t* thrash_bear;
    cooldown_t* maul;
    cooldown_t* moon_cd;  // New / Half / Full Moon
    cooldown_t* swiftmend;
    cooldown_t* tigers_fury;
    cooldown_t* warrior_of_elune;
    cooldown_t* barkskin;
    cooldown_t* rage_from_melees;
  } cooldown;

  // Gains
  struct gains_t
  {
    // Multiple Specs / Forms
    gain_t* clearcasting;        // Feral & Restoration
    gain_t* soul_of_the_forest;  // Feral & Guardian
    gain_t* lucid_dreams;

    // Balance
    gain_t* natures_balance;  // talent
    gain_t* force_of_nature;
    gain_t* warrior_of_elune;
    gain_t* fury_of_elune;
    gain_t* stellar_flare;
    gain_t* starfire;
    gain_t* moonfire;
    gain_t* shooting_stars;
    gain_t* wrath;
    gain_t* sunfire;
    gain_t* celestial_alignment;
    gain_t* incarnation;
    gain_t* primordial_arcanic_pulsar;
    gain_t* arcanic_pulsar;
    gain_t* vision_of_perfection;

    // Feral (Cat)
    gain_t* brutal_slash;
    gain_t* energy_refund;
    gain_t* feral_frenzy;
    gain_t* primal_fury;
    gain_t* rake;
    gain_t* shred;
    gain_t* swipe_cat;
    gain_t* tigers_fury;
    gain_t* berserk;
    gain_t* gushing_lacerations;
    gain_t* cateye_curio;
    gain_t* eye_of_fearful_symmetry;
    gain_t* incessant_hunter;

    // Guardian (Bear)
    gain_t* bear_form;
    gain_t* blood_frenzy;
    gain_t* brambles;
    gain_t* bristling_fur;
    gain_t* galactic_guardian;
    gain_t* gore;
    gain_t* rage_refund;
    gain_t* rage_from_melees;
  } gain;

  // Masteries
  struct masteries_t
  {
    // Done
    const spell_data_t* natures_guardian;
    const spell_data_t* natures_guardian_AP;
    const spell_data_t* razor_claws;
    const spell_data_t* total_eclipse;

    // NYI / TODO!
    const spell_data_t* harmony;
  } mastery;

  // Procs
  struct procs_t
  {
    // General
    proc_t* vision_of_perfection;

    // Feral & Resto
    proc_t* clearcasting;
    proc_t* clearcasting_wasted;

    // Feral
    proc_t* predator;
    proc_t* predator_wasted;
    proc_t* primal_fury;
    proc_t* blood_mist;
    proc_t* gushing_lacerations;

    // Balance
    proc_t* power_of_the_moon;
    proc_t* streaking_star;
    proc_t* wasted_streaking_star;
    proc_t* arcanic_pulsar;

    // Guardian
    proc_t* gore;
  } proc;

  // Class Specializations
  struct specializations_t
  {
    // Generic
    const spell_data_t* druid;
    const spell_data_t* critical_strikes;        // Feral & Guardian
    const spell_data_t* leather_specialization;  // All Specializations
    const spell_data_t* omen_of_clarity;         // Feral & Restoration
    const spell_data_t* innervate;               // Balance & Restoration
    const spell_data_t* entangling_roots;
    const spell_data_t* barkskin;
    const spell_data_t* ironfur;

    // Feral
    const spell_data_t* feral;
    const spell_data_t* feral_overrides;
    const spell_data_t* cat_form;  // Cat form hidden effects
    const spell_data_t* cat_form_speed;
    const spell_data_t* feline_swiftness;  // Feral Affinity
    const spell_data_t* predatory_swiftness;
    const spell_data_t* primal_fury;
    const spell_data_t* rip;
    const spell_data_t* sharpened_claws;
    const spell_data_t* swipe_cat;
    const spell_data_t* thrash_cat;
    const spell_data_t* berserk_cat;
    const spell_data_t* rake_dmg;
    const spell_data_t* tigers_fury;
    const spell_data_t* shred;
    const spell_data_t* savage_roar;  // talent buff spell, holds composite_multiplier data
    const spell_data_t* bloodtalons;  // talent buff spell, holds composite_multiplier data

    // Balance
    const spell_data_t* balance;
    const spell_data_t* astral_influence;  // Balance Affinity
    const spell_data_t* astral_power;
    const spell_data_t* celestial_alignment;
    const spell_data_t* moonkin_form;
    const spell_data_t* eclipse;
    const spell_data_t* eclipse_2;
    const spell_data_t* eclipse_solar;
    const spell_data_t* eclipse_lunar;
    const spell_data_t* starfall;
    const spell_data_t* starfall_dmg;
    const spell_data_t* starfall_2;
    const spell_data_t* starsurge;
    const spell_data_t* starsurge_2;
    const spell_data_t* stellar_drift;  // stellar drift mobility affected by list ID 202461 Effect 1
    const spell_data_t* owlkin_frenzy;
    const spell_data_t* sunfire_dmg;
    const spell_data_t* moonfire_dmg;
    const spell_data_t* shooting_stars;
    const spell_data_t* shooting_stars_dmg;
    const spell_data_t* half_moon;
    const spell_data_t* full_moon;
    const spell_data_t* fury_of_elune;
    const spell_data_t* moonfire_2;
    const spell_data_t* moonfire_3;

    // Guardian
    const spell_data_t* guardian;
    const spell_data_t* guardian_overrides;
    const spell_data_t* bear_form;
    const spell_data_t* bear_form_2;  // Rank 2
    const spell_data_t* gore;
    const spell_data_t* swipe_bear;
    const spell_data_t* thrash_bear;
    const spell_data_t* berserk_bear;
    const spell_data_t* berserk_bear_2;   // Rank 2
    const spell_data_t* thick_hide;       // Guardian Affinity
    const spell_data_t* thrash_bear_dot;  // For Rend and Tear modifier
    const spell_data_t* lightning_reflexes;

    // Resto
    const spell_data_t* restoration;
    const spell_data_t* yseras_gift;  // Restoration Affinity
  } spec;

  // Talents
  struct talents_t
  {
    // Multiple Specs
    const spell_data_t* tiger_dash;
    const spell_data_t* renewal;
    const spell_data_t* wild_charge;

    const spell_data_t* balance_affinity;
    const spell_data_t* feral_affinity;
    const spell_data_t* guardian_affinity;
    const spell_data_t* restoration_affinity;

    const spell_data_t* mighty_bash;
    const spell_data_t* mass_entanglement;
    const spell_data_t* heart_of_the_wild;

    const spell_data_t* soul_of_the_forest;

    // Feral
    const spell_data_t* predator;
    const spell_data_t* sabertooth;
    const spell_data_t* lunar_inspiration;

    const spell_data_t* soul_of_the_forest_cat;
    const spell_data_t* savage_roar;
    const spell_data_t* incarnation_cat;

    const spell_data_t* scent_of_blood;
    const spell_data_t* brutal_slash;
    const spell_data_t* primal_wrath;

    const spell_data_t* moment_of_clarity;
    const spell_data_t* bloodtalons;
    const spell_data_t* feral_frenzy;

    // Balance
    const spell_data_t* natures_balance;
    const spell_data_t* warrior_of_elune;
    const spell_data_t* force_of_nature;

    const spell_data_t* soul_of_the_forest_moonkin;
    const spell_data_t* starlord;
    const spell_data_t* incarnation_moonkin;

    const spell_data_t* stellar_drift;
    const spell_data_t* twin_moons;
    const spell_data_t* stellar_flare;

    const spell_data_t* solstice;
    const spell_data_t* fury_of_elune;
    const spell_data_t* new_moon;

    // Guardian
    const spell_data_t* brambles;
    const spell_data_t* blood_frenzy;
    const spell_data_t* bristling_fur;

    const spell_data_t* soul_of_the_forest_bear;
    const spell_data_t* galactic_guardian;
    const spell_data_t* incarnation_bear;

    const spell_data_t* earthwarden;
    const spell_data_t* survival_of_the_fittest;
    const spell_data_t* guardian_of_elune;

    const spell_data_t* rend_and_tear;
    const spell_data_t* lunar_beam;
    const spell_data_t* pulverize;

    // PvP Talents
    const spell_data_t* sharpened_claws;

    // Restoration
    const spell_data_t* cenarion_ward;

    const spell_data_t* soul_of_the_forest_tree;
    const spell_data_t* cultivation;
    const spell_data_t* incarnation_tree;

    const spell_data_t* inner_peace;

    const spell_data_t* germination;
    const spell_data_t* flourish;
  } talent;

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
    const spell_data_t* adaptive_swarm_damage;
    const spell_data_t* adaptive_swarm_heal;
  } covenant;

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

    conduit_data_t deep_allegiance;
    conduit_data_t evolved_swarm;
    conduit_data_t conflux_of_elements;
    conduit_data_t endless_thirst;
  } conduit;

  struct uptimes_t
  {
    uptime_t* arcanic_pulsar;
    uptime_t* vision_of_perfection;
    uptime_t* combined_ca_inc;
    uptime_t* eclipse;
  } uptime;

  struct legendary_t
  {
    // General
    item_runeforge_t oath_of_the_elder_druid;   // 7084
    item_runeforge_t circle_of_life_and_death;  // 7085
    item_runeforge_t draught_of_deep_focus;     // 7086
    item_runeforge_t lycaras_fleeting_glimpse;  // 7110

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
    item_runeforge_t ursocs_lingering_spirit;  // 7094
    item_runeforge_t legacy_of_the_sleeper;    // 7095
  } legendary;

  druid_t( sim_t* sim, util::string_view name, race_e r = RACE_NIGHT_ELF )
    : player_t( sim, DRUID, name, r ),
      form( NO_FORM ),
      eclipse_handler( this ),
      previous_streaking_stars( SS_NONE ),
      lucid_dreams_proc_chance_balance( 0.15 ),
      lucid_dreams_proc_chance_feral( 0.15 ),
      lucid_dreams_proc_chance_guardian( 0.15 ),
      predator_rppm_rate( 0.0 ),
      initial_astral_power( 0 ),
      thorns_attack_period( 2.0 ),
      thorns_hit_chance( 0.75 ),
      initial_moon_stage( NEW_MOON ),
      lively_spirit_stacks( 9 ),  // set a usually fitting default value
      catweave_bear( false ),
      affinity_resources( false ),
      kindred_empowerment_ratio( 1.0 ),
      convoke_the_spirits_heals( 3.5 ),
      convoke_the_spirits_ultimate( 0.01 ),
      active( active_actions_t() ),
      force_of_nature(),
      caster_form_weapon(),
      caster_melee_attack( nullptr ),
      cat_melee_attack( nullptr ),
      bear_melee_attack( nullptr ),
      lucid_dreams( spell_data_t::not_found() ),
      buff( buffs_t() ),
      cooldown( cooldowns_t() ),
      gain( gains_t() ),
      mastery( masteries_t() ),
      proc( procs_t() ),
      spec( specializations_t() ),
      talent( talents_t() ),
      covenant( covenant_t() ),
      conduit( conduit_t() ),
      uptime( uptimes_t() ),
      legendary( legendary_t() )
  {
    cooldown.berserk             = get_cooldown( "berserk" );
    cooldown.celestial_alignment = get_cooldown( "celestial_alignment" );
    cooldown.growl               = get_cooldown( "growl" );
    cooldown.incarnation         = get_cooldown( "incarnation" );
    cooldown.mangle              = get_cooldown( "mangle" );
    cooldown.thrash_bear         = get_cooldown( "thrash_bear" );
    cooldown.maul                = get_cooldown( "maul" );
    cooldown.moon_cd             = get_cooldown( "moon_cd" );
    cooldown.swiftmend           = get_cooldown( "swiftmend" );
    cooldown.tigers_fury         = get_cooldown( "tigers_fury" );
    cooldown.warrior_of_elune    = get_cooldown( "warrior_of_elune" );
    cooldown.barkskin            = get_cooldown( "barkskin" );
    cooldown.innervate           = get_cooldown( "innervate" );
    cooldown.rage_from_melees    = get_cooldown( "rage_from_melees" );

    cooldown.rage_from_melees->duration = timespan_t::from_seconds( 1.0 );

    equipped_weapon_dps   = 0;
    resource_regeneration = regen_type::DYNAMIC;

    regen_caches[ CACHE_HASTE ]        = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  ~druid_t() override;

  // Character Definition
  void activate() override;
  void init() override;
  void init_absorb_priority() override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_gains() override;
  void init_procs() override;
  void init_uptimes() override;
  void init_resources( bool ) override;
  void init_rng() override;
  void init_spells() override;
  void init_scaling() override;
  void init_assessors() override;
  void create_buffs() override;
  void create_actions() override;
  std::string default_flask() const override;
  std::string default_potion() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  void invalidate_cache( cache_e ) override;
  void arise() override;
  void reset() override;
  void combat_begin() override;
  void merge( player_t& other ) override;
  timespan_t available() const override;
  double composite_armor() const override;
  double composite_armor_multiplier() const override;
  double composite_melee_attack_power() const override;
  double composite_melee_attack_power( attack_power_type type ) const override;
  double composite_attack_power_multiplier() const override;
  double composite_attribute( attribute_e attr ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_block() const override { return 0; }
  double composite_crit_avoidance() const override;
  double composite_dodge() const override;
  double composite_dodge_rating() const override;
  double composite_damage_versatility_rating() const override;
  double composite_heal_versatility_rating() const override;
  double composite_mitigation_versatility_rating() const override;
  double composite_leech() const override;
  double composite_melee_crit_chance() const override;
  double composite_melee_haste() const override;
  double composite_melee_expertise( const weapon_t* ) const override;
  double composite_parry() const override { return 0; }
  double composite_player_multiplier( school_e school ) const override;
  double composite_rating_multiplier( rating_e ) const override;
  double composite_spell_crit_chance() const override;
  double composite_spell_haste() const override;
  double composite_spell_power( school_e school ) const override;
  double composite_spell_power_multiplier() const override;
  double temporary_movement_modifier() const override;
  double passive_movement_modifier() const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;
  action_t* create_action( util::string_view name, const std::string& options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type ) override;
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
  void recalculate_resource_max( resource_e, gain_t* g = nullptr ) override;
  void create_options() override;
  std::string create_profile( save_e type ) override;
  druid_td_t* get_target_data( player_t* target ) const override;
  void copy_from( player_t* ) override;
  void output_json_report( js::JsonOutput& /* root */ ) const override;
  form_e get_form() const { return form; }
  void shapeshift( form_e );
  void init_beast_weapon( weapon_t&, double );
  const spell_data_t* find_affinity_spell( const std::string& ) const;
  specialization_e get_affinity_spec() const;
  void trigger_natures_guardian( const action_state_t* );
  double calculate_expected_max_health() const;
  const spelleffect_data_t* query_aura_effect( const spell_data_t* aura_spell,
                                               effect_subtype_t subtype           = A_ADD_PCT_MODIFIER,
                                               int misc_value                     = P_GENERIC,
                                               const spell_data_t* affected_spell = spell_data_t::nil(),
                                               effect_type_t type                 = E_APPLY_AURA ) const;
  void vision_of_perfection_proc() override;
  void apply_affecting_auras( action_t& ) override;

  // secondary actions
  std::vector<action_t*> secondary_action_list;

  template <typename T, typename... Ts>
  T* get_secondary_action( util::string_view n, Ts&&... args )
  {
    auto it = range::find( secondary_action_list, n, &action_t::name_str );
    if ( it != secondary_action_list.cend() )
      return dynamic_cast<T*>( *it );

    auto a = new T( this, std::forward<Ts>( args )... );
    secondary_action_list.push_back( a );
    return a;
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

druid_t::~druid_t()
{
  range::dispose( counters );
}

// eclipse handler function definitions
void eclipse_handler_t::cast_wrath()
{
  if ( state == ANY_NEXT || state == LUNAR_NEXT )
  {
    wrath_counter--;
    advance_eclipse();
  }
}

void eclipse_handler_t::cast_starfire()
{
  if ( state == ANY_NEXT || state == SOLAR_NEXT )
  {
    starfire_counter--;
    advance_eclipse();
  }
}

void eclipse_handler_t::advance_eclipse()
{
  if ( !starfire_counter && state != IN_SOLAR )
  {
    p->buff.eclipse_solar->trigger();

    if ( p->talent.solstice->ok() )
      p->buff.solstice->trigger();

    if ( p->legendary.balance_of_all_things->ok() )
      p->buff.balance_of_all_things_nature->trigger();

    state = IN_SOLAR;
    reset_stacks();

    p->uptime.eclipse->update( true, p->sim->current_time() );

    return;
  }

  if ( !wrath_counter && state != IN_LUNAR )
  {
    p->buff.eclipse_lunar->trigger();

    if ( p->talent.solstice->ok() )
      p->buff.solstice->trigger();

    if ( p->legendary.balance_of_all_things->ok() )
      p->buff.balance_of_all_things_arcane->trigger();

    state = IN_LUNAR;
    reset_stacks();

    p->uptime.eclipse->update( true, p->sim->current_time() );

    return;
  }

  if ( state == IN_SOLAR || state == IN_LUNAR || state == IN_BOTH )
    p->uptime.eclipse->update( false, p->sim->current_time() );

  if ( state == IN_SOLAR )
    state = LUNAR_NEXT;

  if ( state == IN_LUNAR )
    state = SOLAR_NEXT;

  if ( state == IN_BOTH )
    state = ANY_NEXT;
}

void eclipse_handler_t::cast_starsurge()
{
  if ( state == IN_SOLAR || state == IN_LUNAR || state == IN_BOTH )
  p->buff.starsurge->trigger();

  if ( p->conduit.stellar_inspiration->ok() && p->rng().roll( p->conduit.stellar_inspiration.percent() ) )
    p->buff.starsurge->trigger();
}

void eclipse_handler_t::trigger_both( timespan_t d = 0_ms )
{
  state = IN_BOTH;
  reset_stacks();

  if ( p->legendary.balance_of_all_things->ok() )
  {
    p->buff.balance_of_all_things_arcane->trigger();
    p->buff.balance_of_all_things_nature->trigger();
  }

  if ( p->talent.solstice->ok() )
    p->buff.solstice->trigger();

  p->buff.starsurge->expire();
  p->buff.eclipse_lunar->trigger( d );
  p->buff.eclipse_solar->trigger( d );

  p->uptime.eclipse->update( true, p->sim->current_time() );
}

void eclipse_handler_t::expire_both()
{
  p->buff.eclipse_solar->expire();
  p->buff.eclipse_lunar->expire();

  p->uptime.eclipse->update( false, p->sim->current_time() );
}

void eclipse_handler_t::reset_stacks()
{
  wrath_counter    = 2;
  starfire_counter = 2;
}

void eclipse_handler_t::reset_state()
{
  state = ANY_NEXT;
}

snapshot_counter_t::snapshot_counter_t( druid_t* player, buff_t* buff )
  : sim( player->sim ),
    p( player ),
    b( 0 ),
    exe_up( 0 ),
    exe_down( 0 ),
    tick_up( 0 ),
    tick_down( 0 ),
    is_snapped( false ),
    wasted_buffs( 0 )
{
  b.push_back( buff );
  p->counters.push_back( this );
}

namespace pets
{
// ==========================================================================
// Pets and Guardians
// ==========================================================================

// Force of Nature ==================================================

struct force_of_nature_t : public pet_t
{
  struct fon_melee_t : public melee_attack_t
  {
    bool first_attack;
    fon_melee_t( pet_t* p, const char* name = "melee" )
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

  force_of_nature_t( sim_t* sim, druid_t* owner ) : pet_t( sim, owner, "treant", true /*GUARDIAN*/, true )
  {
    // Treants have base weapon damage + ap from player's sp.
    // @50-57: 3 base + 0.6 SP
    //    @58: 3 base + 0.375 SP (needs more testing)
    //    @59: 3 base + 0.333 SP (needs more testing)
    //    @60: 3.5 base + 0.28 SP

    owner_coeff.ap_from_sp = 0.6;
    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = 3.0;

    if ( o()->level() == 58 )
      owner_coeff.ap_from_sp = 0.375;
    else if ( o()->level() == 59 )
      owner_coeff.ap_from_sp = 0.333;
    else if ( o()->level() == 60 )
    {
      owner_coeff.ap_from_sp = 0.28;
      main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = 3.5;
    }

    resource_regeneration = regen_type::DISABLED;
    main_hand_weapon.type = WEAPON_BEAST;
  }

  void init_action_list() override
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "auto_attack" );

    pet_t::init_action_list();
  }

  double composite_player_multiplier( school_e school ) const override
  {
    return owner->cache.player_multiplier( school );
  }

  double composite_melee_crit_chance() const override { return owner->cache.spell_crit_chance(); }

  double composite_spell_haste() const override { return owner->cache.spell_haste(); }

  double composite_damage_versatility() const override { return owner->cache.damage_versatility(); }

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

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this );

    return pet_t::create_action( name, options_str );
  }
};
}  // end namespace pets

namespace buffs
{
template <typename BuffBase>
struct druid_buff_t : public BuffBase
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
  druid_buff_t( druid_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                const item_t* item = nullptr )
    : BuffBase( &p, name, s, item ) {}

  druid_buff_t( druid_td_t& td, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                const item_t* item = nullptr )
    : BuffBase( td, name, s, item ) {}

  druid_t& p() { return *debug_cast<druid_t*>( BuffBase::source ); }
  const druid_t& p() const { return *debug_cast<druid_t*>( BuffBase::source ); }
};

// Bear Form ================================================================
struct bear_form_t : public druid_buff_t<buff_t>
{
private:
  const spell_data_t* rage_spell;

public:
  bear_form_t( druid_t& p )
    : base_t( p, "bear_form", p.find_class_spell( "Bear Form" ) ), rage_spell( p.find_spell( 17057 ) )
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
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    p().buff.cat_form->expire();
    p().buff.tigers_fury->expire();  // Mar 03 2016: Tiger's Fury ends when you enter bear form.
    p().buff.moonkin_form->expire();

    swap_melee( p().bear_melee_attack, p().bear_weapon );

    base_t::start( stacks, value, duration );

    p().resource_gain( RESOURCE_RAGE, rage_spell->effectN( 1 ).base_value() / 10.0, p().gain.bear_form );
    p().recalculate_resource_max( RESOURCE_HEALTH );
  }
};

// Cat Form =================================================================
struct cat_form_t : public druid_buff_t<buff_t>
{
  cat_form_t( druid_t& p ) : base_t( p, "cat_form", p.find_class_spell( "Cat Form" ) )
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
    p().buff.bear_form->expire();
    p().buff.moonkin_form->expire();

    swap_melee( p().cat_melee_attack, p().cat_weapon );

    base_t::start( stacks, value, duration );
  }
};

// Moonkin Form =============================================================
struct moonkin_form_t : public druid_buff_t<buff_t>
{
  moonkin_form_t( druid_t& p ) : base_t( p, "moonkin_form", p.spec.moonkin_form )
  {
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    p().buff.bear_form->expire();
    p().buff.cat_form->expire();

    base_t::start( stacks, value, duration );
  }
};

// Tiger Dash Buff ===========================================================
struct tiger_dash_buff_t : public druid_buff_t<buff_t>
{
  tiger_dash_buff_t( druid_t& p ) : base_t( p, "tiger_dash", p.talent.tiger_dash )
  {
    set_cooldown( 0_ms );
    set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );
    set_tick_callback( []( buff_t* b, int, timespan_t ) { b->current_value -= b->data().effectN( 2 ).percent(); } );
  }

  bool freeze_stacks() override { return true; }
};

//Innervate Buff ============================================================
struct innervate_buff_t : public druid_buff_t<buff_t>
{
  innervate_buff_t( druid_t& p ) : base_t( p, "innervate", p.spec.innervate ) {}

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    druid_t* p = debug_cast<druid_t*>( player );

    if ( p->azerite.lively_spirit.enabled() )
    {
      p->buff.lively_spirit->trigger( p->lively_spirit_stacks );
    }
  }
};

// Eclipse Buff =============================================================
struct eclipse_buff_t : public druid_buff_t<buff_t>
{
  double empowerment;

  eclipse_buff_t( druid_t& p, util::string_view n, const spell_data_t* s ) : base_t( p, n, s ), empowerment( 0.0 )
  {
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    set_default_value_from_effect( 2, 0.01 * ( 1.0 + p.conduit.umbral_intensity.percent() ) );

    if ( s == p.spec.eclipse_solar )
      empowerment = p.spec.starsurge->effectN( 2 ).percent() + p.spec.starsurge_2->effectN( 1 ).percent();
    else if ( s == p.spec.eclipse_lunar )
      empowerment = p.spec.starsurge->effectN( 3 ).percent() + p.spec.starsurge_2->effectN( 2 ).percent();
  }

  double value() override
  {
    double v = base_t::value();

    v += empowerment * p().buff.starsurge->stack();

    return v;
  }

  void expire_override( int s, timespan_t d ) override
  {
    base_t::expire_override( s, d );

    p().eclipse_handler.advance_eclipse();
    p().buff.starsurge->expire();
  }
};

// Warrior of Elune Buff ====================================================
struct warrior_of_elune_buff_t : public druid_buff_t<buff_t>
{
  warrior_of_elune_buff_t( druid_t& p ) : base_t( p, "warrior_of_elune", p.talent.warrior_of_elune ) {}

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    p().cooldown.warrior_of_elune->start();
  }
};

// Bloodtalons Tracking Buff ================================================
struct bt_dummy_buff_t : public druid_buff_t<buff_t>
{
  int count;

  bt_dummy_buff_t( druid_t& p, util::string_view n )
    : base_t( p, n ), count( as<int>( p.talent.bloodtalons->effectN( 2 ).base_value() ) )
  {
    set_duration( timespan_t::from_seconds( p.talent.bloodtalons->effectN( 1 ).base_value() ) );
    set_max_stack( 1 );
    set_quiet( true );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    if ( !p().talent.bloodtalons->ok() || p().buff.bloodtalons->check() )
      return false;

    if ( p().buff.bt_rake->check() + p().buff.bt_shred->check() + p().buff.bt_swipe->check() +
         p().buff.bt_thrash->check() + p().buff.bt_moonfire->check() + p().buff.bt_brutal_slash->check() + 1 < count )
    {
      return druid_buff_t<buff_t>::trigger( s, v, c, d );
    }

    p().buff.bt_rake->expire();
    p().buff.bt_shred->expire();
    p().buff.bt_swipe->expire();
    p().buff.bt_thrash->expire();
    p().buff.bt_moonfire->expire();
    p().buff.bt_brutal_slash->expire();

    p().buff.bloodtalons->trigger( p().buff.bloodtalons->max_stack() );

    return true;
  }
};

// Tiger's Fury =============================================================
struct tigers_fury_buff_t : public druid_buff_t<buff_t>
{
  tigers_fury_buff_t( druid_t& p ) : base_t( p, "tigers_fury", p.spec.tigers_fury )
  {
    set_cooldown( 0_ms );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    if ( p().azerite.jungle_fury.enabled() )
      p().buff.jungle_fury->trigger( duration );

    base_t::start( stacks, value, duration );
  }

  void refresh( int stacks, double value, timespan_t duration ) override
  {
    if ( p().azerite.jungle_fury.enabled() )
      p().buff.jungle_fury->refresh( 0, DEFAULT_VALUE(), duration );

    base_t::refresh( stacks, value, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    p().buff.jungle_fury->expire();

    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// Incarnation: Guardian of Ursoc ===========================================
struct incarnation_bear_buff_t : public druid_buff_t<buff_t>
{
  double hp_mul;

  incarnation_bear_buff_t( druid_t& p )
    : base_t( p, "incarnation_guardian_of_ursoc",
              p.talent.incarnation_bear->ok() ? p.talent.incarnation_bear : p.find_spell( 102558 ) )
  {
    set_cooldown( 0_ms );
    hp_mul = 1.0 + data().effectN( 5 ).percent();

    if ( p.conduit.unchecked_aggression->ok() )
      add_invalidate( CACHE_HASTE );

    if ( p.legendary.legacy_of_the_sleeper->ok() )
      add_invalidate( CACHE_LEECH );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool refresh = !check();
    bool success = druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );

    if ( !refresh && success )
    {
      player->resources.max[ RESOURCE_HEALTH ] *= hp_mul;
      player->resources.current[ RESOURCE_HEALTH ] *= hp_mul;
      p().recalculate_resource_max( RESOURCE_HEALTH );
    }

    return success;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    player->resources.max[ RESOURCE_HEALTH ] /= hp_mul;
    player->resources.current[ RESOURCE_HEALTH ] /= hp_mul;
    p().recalculate_resource_max( RESOURCE_HEALTH );

    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );
  }
};

// Survival Instincts =======================================================
struct survival_instincts_buff_t : public druid_buff_t<buff_t>
{
  survival_instincts_buff_t( druid_t& p )
    : base_t( p, "survival_instincts", p.find_specialization_spell( "Survival Instincts" ) )
  {
    set_cooldown( 0_ms );
    // Tooltip data redirects to another spell even though the defensive buff in logs is the specialization spell
    set_default_value( p.find_spell( 50322 )->effectN( 1 ).percent() );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    p().buff.masterful_instincts->trigger();
  }
};

// Kindred Empowerment ======================================================
struct kindred_empowerment_buff_t : public druid_buff_t<buff_t>
{
  double pool;
  double cumul_pool;

  kindred_empowerment_buff_t( druid_t& p )
    : base_t( p, "kindred_empowerment", p.covenant.kindred_empowerment ), pool( 1.0 ), cumul_pool( 0.0 )
  {
    set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  void expire_override( int s, timespan_t d ) override
  {
    base_t::expire_override( s, d );

    pool       = 1.0;
    cumul_pool = 0.0;
  }

  void add_pool( const action_state_t* s )
  {
    trigger();

    double amount = s->result_amount * p().covenant.kindred_empowerment_energize->effectN( 1 ).percent() *
                    p().kindred_empowerment_ratio;

    if ( sim->debug )
      sim->print_debug( "Kindred Empowerment: Adding {} from {} to pool of {}", amount, s->action->name(), pool );

    pool += amount;
    cumul_pool += amount;
  }

  void use_pool( const action_state_t* s )
  {
    if ( pool <= 1 )  // minimum pool value of 1
      return;

    double amount = std::min( s->result_amount * p().covenant.kindred_empowerment->effectN( 2 ).percent(), pool - 1);

    if ( amount == 0 )
      return;

    if ( sim->debug )
      sim->print_debug( "Kindred Empowerment: Using {} from pool of {} on {}", amount, pool, s->action->name() );

    auto damage = p().active.kindred_empowerment;
    damage->set_target( s->target );
    damage->base_dd_min = damage->base_dd_max = amount;
    damage->execute();

    pool -= amount;
  }

  void snapshot()
  {
    if ( cumul_pool > 0 )
    {
      auto partner = p().active.kindred_empowerment_partner;
      partner->set_target( p().target );
      partner->base_dd_min = partner->base_dd_max = cumul_pool;
      partner->execute();
    }
  }
};

}  // end namespace buffs

struct buff_effect_t
{
  buff_t* buff;
  double value;
  bool mastery;

  buff_effect_t( buff_t* b, double v, bool m = false ) : buff( b ), value( v ), mastery( m ) {}
};

struct dot_debuff_t
{
  std::function<dot_t*( druid_td_t* )> func;
  double value;
  bool use_stacks;

  dot_debuff_t( std::function<dot_t*( druid_td_t* )> f, double v, bool b ) : func( f ), value( v ), use_stacks( b ) {}
};

struct free_cast_stats_t
{
  free_cast_e type;
  stats_t* stats;

  free_cast_stats_t( free_cast_e f, stats_t* s ) : type( f ), stats( s ) {}
};

// Template for common druid action code. See priest_action_t.
template <class Base>
struct druid_action_t : public Base
{
  unsigned form_mask; // Restricts use of a spell based on form.
  bool may_autounshift; // Allows a spell that may be cast in NO_FORM but not in current form to be cast by exiting form.
  unsigned autoshift; // Allows a spell that may not be cast in the current form to be cast by automatically changing to the specified form.
private:
  using ab = Base;  // action base, eg. spell_t
public:
  using base_t = druid_action_t<Base>;

  bool triggers_galactic_guardian;
  double lucid_dreams_multiplier;
  bool is_auto_attack;

  free_cast_e free_cast;
  std::vector<free_cast_stats_t> free_cast_stats;
  stats_t* orig_stats;

  std::vector<buff_effect_t> ta_multiplier_buffeffects;
  std::vector<buff_effect_t> da_multiplier_buffeffects;
  std::vector<buff_effect_t> execute_time_buffeffects;
  std::vector<buff_effect_t> recharge_multiplier_buffeffects;
  std::vector<buff_effect_t> cost_buffeffects;
  std::vector<buff_effect_t> crit_chance_buffeffects;

  std::vector<dot_debuff_t> target_multiplier_dotdebuffs;

  druid_action_t( util::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      form_mask( ab::data().stance_mask() ),
      may_autounshift( true ),
      autoshift( 0 ),
      triggers_galactic_guardian( true ),
      lucid_dreams_multiplier( p()->lucid_dreams->effectN( 1 ).percent() ),
      is_auto_attack( false ),
      free_cast( free_cast_e::NONE ),
      free_cast_stats(),
      orig_stats( ab::stats ),
      ta_multiplier_buffeffects(),
      da_multiplier_buffeffects(),
      execute_time_buffeffects(),
      recharge_multiplier_buffeffects(),
      cost_buffeffects(),
      crit_chance_buffeffects(),
      target_multiplier_dotdebuffs()
  {
    ab::may_crit      = true;
    ab::tick_may_crit = true;

    // Ugly Ugly Ugly hack for now.
    if ( util::str_in_str_ci( n, "_melee" ) )
      is_auto_attack = true;

    apply_buff_effects();
    apply_dot_debuffs();
  }

  druid_t* p() { return static_cast<druid_t*>( ab::player ); }

  const druid_t* p() const { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const { return p()->get_target_data( t ); }

  action_state_t* new_state() override { return new druid_action_state_t( this, ab::target, free_cast ); }

  free_cast_e get_state_free_cast( action_state_t* s ) { return debug_cast<druid_action_state_t*>( s )->free_cast; }

  void set_state_free_cast( action_state_t* s ) { debug_cast<druid_action_state_t*>( s )->free_cast = free_cast; }

  std::string free_cast_string( free_cast_e f ) const
  {
    if ( f == free_cast_e::CONVOKE )
      return ab::name_str + "_convoke";

    if ( f == free_cast_e::LYCARAS )
      return ab::name_str + "_lycaras";

    if ( f == free_cast_e::ONETHS )
      return ab::name_str + "_oneths";

    return ab::name_str;
  }

  stats_t* init_free_cast_stats( free_cast_e f )
  {
    for ( auto s : free_cast_stats )
      if ( s.type == f )
        return s.stats;

    auto fc_stats = p()->get_stats( free_cast_string( f ), this );
    free_cast_stats.push_back( free_cast_stats_t( f, fc_stats ) );

    return fc_stats;
  }

  stats_t* get_free_cast_stats( free_cast_e f )
  {
    for ( auto s : free_cast_stats )
      if ( s.type == f )
        return s.stats;

    return ab::stats;
  }

  void execute() override
  {
    if ( free_cast )
    {
      ab::stats = get_free_cast_stats( free_cast );
      ab::execute();
      ab::stats = orig_stats;
      free_cast = free_cast_e::NONE;
    }
    else
      ab::execute();

    if ( !ab::background && ab::trigger_gcd > 0_ms && p()->buff.ravenous_frenzy->check() )
      p()->buff.ravenous_frenzy->trigger();
  }

  void schedule_travel( action_state_t* s ) override
  {
    set_state_free_cast( s );

    ab::schedule_travel( s );
  }

  void impact( action_state_t* s ) override
  {
    auto f = get_state_free_cast( s );

    if ( f && ab::stats == orig_stats )
    {
      ab::stats = get_free_cast_stats( f );
      ab::impact( s );
      ab::stats = orig_stats;
    }
    else
      ab::impact( s );

    trigger_galactic_guardian( s );
  }

  double mod_spell_effects_percent( const spell_data_t* s, const spelleffect_data_t* e ) { return e->percent(); }

  double mod_spell_effects_percent( const conduit_data_t& c, const spelleffect_data_t* )
  {
    // HOTFIX HACK to reflect server-side scripting
    if ( c == p()->conduit.endless_thirst )
      return c.percent() / 10.0;

    return c.percent();
  }

  template <typename T>
  void parse_spell_effects_mods( double& val, bool& mastery, const spell_data_t* base, size_t idx, T mod )
  {
    for ( size_t i = 1; i <= mod->effect_count(); i++ )
    {
      auto eff = &mod->effectN( i );

      if ( eff->type() != E_APPLY_AURA || !base->affected_by_all( &ab::player->dbc, *eff ) )
        continue;

      if ( ( eff->misc_value1() == P_EFFECT_1 && idx == 1 ) || ( eff->misc_value1() == P_EFFECT_2 && idx == 2 ) ||
           ( eff->misc_value1() == P_EFFECT_3 && idx == 3 ) || ( eff->misc_value1() == P_EFFECT_4 && idx == 4 ) ||
           ( eff->misc_value1() == P_EFFECT_5 && idx == 5 ) )
      {
        double pct = mod_spell_effects_percent( mod, eff );

        if ( eff->subtype() == A_ADD_FLAT_MODIFIER )
          val += pct;
        else if ( eff->subtype() == A_ADD_PCT_MODIFIER )
          val *= 1.0 + pct;
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
  void parse_buff_effect( buff_t* buff, const spell_data_t* s_data, size_t i, Ts... mods )
  {
    auto eff     = &s_data->effectN( i );
    double val   = eff->percent();
    bool mastery = false;

    // TODO: rework party_aura & target checks
    if ( !( eff->type() == E_APPLY_AURA || eff->type() == E_APPLY_AREA_AURA_PARTY ) || eff->target_1() != 1 )
      return;

    if ( i <= 5 )
      parse_spell_effects_mods( val, mastery, s_data, i, mods... );

    if ( is_auto_attack && eff->subtype() == A_MOD_AUTO_ATTACK_PCT )
    {
      da_multiplier_buffeffects.push_back( buff_effect_t( buff, val ) );
      return;
    }

    if ( !ab::data().affected_by_all( &ab::player->dbc, *eff ) )
      return;

    if ( !mastery && !val )
      return;

    if ( eff->subtype() == A_ADD_PCT_MODIFIER )
    {
      switch ( eff->misc_value1() )
      {
        case P_GENERIC:
          da_multiplier_buffeffects.push_back( buff_effect_t( buff, val, mastery ) );
          p()->sim->print_debug( "buff-effects: {} direct damage modified by {}%{} with buff {}", ab::name(),
                                 val * 100.0, mastery ? "+mastery" : "" , buff->name() );
          break;
        case P_TICK_DAMAGE:
          ta_multiplier_buffeffects.push_back( buff_effect_t( buff, val, mastery ) );
          p()->sim->print_debug( "buff-effects: {} tick damage modified by {}%{} with buff {}", ab::name(),
                                 val * 100.0, mastery ? "+mastery" : "" , buff->name() );
          break;
        case P_CAST_TIME:
          execute_time_buffeffects.push_back( buff_effect_t( buff, val ) );
          p()->sim->print_debug( "buff-effects: {} cast time modified by {}% with buff {}", ab::name(),
                                 val * 100.0, buff->name() );
          break;
        case P_COOLDOWN:
          recharge_multiplier_buffeffects.push_back( buff_effect_t( buff, val ) );
          p()->sim->print_debug( "buff-effects: {} cooldown modified by {}% with buff {}", ab::name(),
                                 val * 100.0, buff->name() );
          break;
        case P_RESOURCE_COST:
          cost_buffeffects.push_back( buff_effect_t( buff, val ) );
          p()->sim->print_debug( "buff-effects: {} cost modified by {}% with buff {}", ab::name(),
                                 val * 100.0, buff->name() );
          break;
        default:
          break;
      }
    }
    else if ( eff->subtype() == A_ADD_FLAT_MODIFIER && eff->misc_value1() == P_CRIT )
    {
      crit_chance_buffeffects.push_back( buff_effect_t( buff, val ) );
      p()->sim->print_debug( "buff-effects: {} crit chance modified by {}% with buff {}", ab::name(),
                             val * 100.0, buff->name() );
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, unsigned ignore, Ts... mods )
  {
    if ( !buff )
      return;

    const spell_data_t* s_data = &buff->data();

    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      if ( i == as<size_t>( ignore ) )
        continue;

      parse_buff_effect( buff, s_data, i, mods... );
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, Ts... mods )
  {
    if ( !buff )
      return;

    const spell_data_t* s_data = &buff->data();

    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      parse_buff_effect( buff, s_data, i, mods... );
    }
  }

  double get_buff_effects_value( const std::vector<buff_effect_t>& buffeffects, bool flat = false, bool benefit = true,
                                 bool stack = true ) const
  {
    double return_value = flat ? 0.0 : 1.0;

    for ( auto i : buffeffects )
    {
      double eff_val = i.value;

      if ( i.mastery )
      {
        eff_val += p()->cache.mastery_value();
      }

      if ( !i.buff )
      {
        return_value *= 1.0 + eff_val;
      }
      else if ( ( benefit && i.buff->up() ) || i.buff->check() )
      {
        if ( flat )
          return_value += eff_val * ( stack ? i.buff->check() : 1 );
        else
          return_value *= 1.0 + eff_val * ( stack ? i.buff->check() : 1 );
      }
    }

    return return_value;
  }

  // Syntax: parse_buff_effects[<S|C[,...]>]( buff_t* buff[, unsigned ignore][, spell_data_t* spell|conduit_data_t conduit][,...] )
  //  buff = buff to be checked for to see if effect applies
  //  ignore = optional effect index of buff to ignore, for effects hard-coded manually elsewhere
  //  S/C = optional list of template parameter to indicate spell or conduit with redirect effects
  //  spell/conduit = optional list of spell or conduit with redirect effects that modify the effects on the buff
  virtual void apply_buff_effects()
  {
    using S = const spell_data_t*;
    using C = const conduit_data_t&;

    parse_buff_effects<C>( p()->buff.ravenous_frenzy,
                           p()->conduit.endless_thirst );
    parse_buff_effects( p()->buff.heart_of_the_wild );
    parse_buff_effects<C>( p()->buff.convoke_the_spirits,
                           p()->conduit.conflux_of_elements );

    // Balance
    parse_buff_effects( p()->buff.moonkin_form );
    //parse_buff_effects( p()->buff.celestial_alignment );
    parse_buff_effects( p()->buff.incarnation_moonkin );
    parse_buff_effects( p()->buff.owlkin_frenzy );
    parse_buff_effects( p()->buff.warrior_of_elune );
    parse_buff_effects( p()->buff.balance_of_all_things_nature );
    parse_buff_effects( p()->buff.balance_of_all_things_arcane );
    parse_buff_effects( p()->buff.oneths_free_starfall );
    parse_buff_effects( p()->buff.oneths_free_starsurge );
    parse_buff_effects( p()->buff.timeworn_dreambinder );
    parse_buff_effects<S, S, S>( p()->buff.eclipse_lunar, 2u,
                                 p()->mastery.total_eclipse,
                                 p()->spec.eclipse_2,
                                 p()->talent.soul_of_the_forest_moonkin );
    parse_buff_effects<S, S>( p()->buff.eclipse_solar, 2u,
                              p()->mastery.total_eclipse,
                              p()->spec.eclipse_2 );

    // Guardian
    parse_buff_effects<S>( p()->buff.berserk_bear,
                           p()->spec.berserk_bear_2 );
    parse_buff_effects<S>( p()->buff.incarnation_bear,
                           p()->spec.berserk_bear_2 );
    parse_buff_effects( p()->buff.sharpened_claws );

    // Feral
    parse_buff_effects( p()->buff.cat_form );
    parse_buff_effects( p()->buff.incarnation_cat );
    parse_buff_effects( p()->buff.predatory_swiftness );
    parse_buff_effects( p()->buff.savage_roar );
    parse_buff_effects( p()->buff.apex_predators_craving );
  }

  template <typename... Ts>
  void parse_dot_debuffs( std::function<dot_t*( druid_td_t* )> func, bool use_stacks, const spell_data_t* s_data,
                          Ts... mods )
  {
    if ( !s_data->ok() )
      return;

    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      auto eff   = &s_data->effectN( i );
      double val = eff->percent();
      bool mastery;  // dummy

      if ( eff->type() != E_APPLY_AURA )
        continue;

      if ( i <= 5 )
        parse_spell_effects_mods( val, mastery, s_data, i, mods... );

      if ( !val )
        continue;

      if ( !( eff->subtype() == A_MOD_DAMAGE_FROM_CASTER_SPELLS && ab::data().affected_by_all( &ab::player->dbc, *eff ) ) &&
           !( eff->subtype() == A_MOD_AUTO_ATTACK_FROM_CASTER && is_auto_attack ) )
        continue;

      p()->sim->print_debug( "dot-debuffs: {} damage modified by {}% on targets with dot {}", ab::name(),
                             val * 100.0, s_data->name_cstr() );
      target_multiplier_dotdebuffs.push_back( dot_debuff_t( func, val, use_stacks ) );
    }
  }

  template <typename... Ts>
  void parse_dot_debuffs( std::function<dot_t*( druid_td_t* )> func, const spell_data_t* s_data, Ts... mods )
  {
    parse_dot_debuffs( func, true, s_data, mods... );
  }

  double get_dot_debuffs_value( druid_td_t* t ) const
  {
    double return_value = 1.0;

    for ( auto i : target_multiplier_dotdebuffs )
    {
      auto dot = i.func( t );

      if ( dot->is_ticking() )
        return_value *= 1.0 + i.value * ( i.use_stacks ? dot->current_stack() : 1.0 );
    }

    return return_value;
  }

  // Syntax: parse_dot_debuffs[<S|C[,...]>]( func, spell_data_t* dot[, spell_data_t* spell|conduit_data_t conduit][,...] )
  //  func = function returning the dot_t* of the dot
  //  dot = spell data of the dot
  //  S/C = optional list of template parameter to indicate spell or conduit with redirect effects
  //  spell/conduit = optional list of spell or conduit with redirect effects that modify the effects on the dot
  void apply_dot_debuffs()
  {
    using S = const spell_data_t*;
    using C = const conduit_data_t&;

    parse_dot_debuffs<C>( []( druid_td_t* t ) -> dot_t* { return t->dots.adaptive_swarm_damage; }, false,
                          p()->covenant.adaptive_swarm_damage,
                          p()->conduit.evolved_swarm );
    parse_dot_debuffs<S>( []( druid_td_t* t ) -> dot_t* { return t->dots.thrash_bear; },
                          p()->spec.thrash_bear_dot,
                          p()->talent.rend_and_tear );
    parse_dot_debuffs<C>( []( druid_td_t* t ) -> dot_t* { return t->dots.moonfire; },
                          p()->spec.moonfire_dmg,
                          p()->conduit.fury_of_the_skies );
    parse_dot_debuffs<C>( []( druid_td_t* t ) -> dot_t* { return t->dots.sunfire; },
                          p()->spec.sunfire_dmg,
                          p()->conduit.fury_of_the_skies );
  }

  double cost() const override
  {
    double c = ab::cost() * get_buff_effects_value( cost_buffeffects, false, false );

    if ( ( p()->buff.innervate->up() && p()->specialization() == DRUID_RESTORATION ) || free_cast )
      c *= 0;

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

    return et;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = ab::recharge_multiplier( cd ) * get_buff_effects_value( recharge_multiplier_buffeffects );

    return rm;
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    trigger_galactic_guardian( d->state );
  }

  void schedule_execute( action_state_t* s = nullptr ) override
  {
    if ( !check_form_restriction() )
    {
      if ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM )
        p()->shapeshift( NO_FORM );
      else if ( autoshift )
        p()->shapeshift( (form_e)autoshift );
      else
        assert( false && "Action executed in wrong form with no valid form to shift to!" );
    }

    ab::schedule_execute( s );
  }

  // Override this function for temporary effects that change the normal form restrictions of the spell. eg: Predatory
  // Swiftness
  virtual bool check_form_restriction()
  {
    return !form_mask || ( form_mask & p()->get_form() ) == p()->get_form() ||
           ( p()->specialization() == DRUID_GUARDIAN && p()->buff.bear_form->check() &&
             ab::data().affected_by( p()->buff.bear_form->data().effectN( 2 ) ) );
  }

  bool ready() override
  {
    if ( !check_form_restriction() && !( ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM ) || autoshift ) )
    {
      if ( ab::sim->debug )
        ab::sim->out_debug.printf( "%s ready() failed due to wrong form. form=%#.8x form_mask=%#.8x", ab::name(),
                                   p()->get_form(), form_mask );

      return false;
    }

    return ab::ready();
  }

  void trigger_lucid_dreams()
  {
    if ( !p()->lucid_dreams->ok() )
      return;

    if ( ab::last_resource_cost <= 0.0 )
      return;

    double proc_chance = 0.0;

    switch ( p()->specialization() )
    {
      case DRUID_BALANCE:
        proc_chance = p()->lucid_dreams_proc_chance_balance;
        break;
      case DRUID_FERAL:
        proc_chance = p()->lucid_dreams_proc_chance_feral;
        break;
      case DRUID_GUARDIAN:
        proc_chance = p()->lucid_dreams_proc_chance_guardian;
        break;
      default:
        break;
    }

    if ( ab::rng().roll( proc_chance ) )
    {
      switch ( p()->specialization() )
      {
        case DRUID_BALANCE:
          p()->resource_gain( RESOURCE_ASTRAL_POWER, lucid_dreams_multiplier * ab::last_resource_cost,
                              p()->gain.lucid_dreams );
          break;
        case DRUID_FERAL:
          p()->resource_gain( RESOURCE_ENERGY, lucid_dreams_multiplier * ab::last_resource_cost,
                              p()->gain.lucid_dreams );
          break;
        case DRUID_GUARDIAN:
          p()->resource_gain( RESOURCE_RAGE, lucid_dreams_multiplier * ab::last_resource_cost, p()->gain.lucid_dreams );
          break;
        default:
          break;
      }

      p()->player_t::buffs.lucid_dreams->trigger();
    }
  }

  void consume_resource() override
  {
    ab::consume_resource();

    trigger_lucid_dreams();
  }

  void trigger_gore()  // need this here instead of bear_attack_t because of moonfire
  {
    if ( !p()->spec.gore->ok() )
      return;

    if ( p()->buff.gore->trigger() )
    {
      p()->proc.gore->occur();
      p()->cooldown.mangle->reset( true );
    }
  }

  virtual void trigger_galactic_guardian( action_state_t* s )
  {
    if ( !( triggers_galactic_guardian && !ab::proc && ab::harmful ) )
      return;
    if ( !p()->talent.galactic_guardian->ok() )
      return;
    if ( s->target == p() )
      return;
    if ( !ab::result_is_hit( s->result ) )
      return;
    if ( s->result_total <= 0 )
      return;

    if ( ab::rng().roll( p()->talent.galactic_guardian->proc_chance() ) )
    {
      // Trigger Moonfire
      action_state_t* gg_s = p()->active.galactic_guardian->get_state();
      gg_s->target         = s->target;
      p()->active.galactic_guardian->snapshot_state( gg_s, result_amount_type::DMG_DIRECT );
      p()->active.galactic_guardian->schedule_execute( gg_s );

      // Buff is triggered in galactic_guardian_damage_t::execute()
    }
  }

  virtual bool compare_previous_streaking_stars( streaking_stars_e new_ability )
  {
    if ( p()->previous_streaking_stars == new_ability )
    {
      p()->proc.wasted_streaking_star->occur();
      return true;
    }
    return new_ability == SS_CELESTIAL_ALIGNMENT;
  }

  virtual void streaking_stars_trigger( streaking_stars_e new_ability, action_state_t* s )
  {
    if ( p()->azerite.streaking_stars.ok() &&
         ( p()->buff.celestial_alignment->check() || p()->buff.incarnation_moonkin->check() ) )
    {
      if ( !compare_previous_streaking_stars( new_ability ) )
      {
        action_state_t* ss_s = p()->active.streaking_stars->get_state();

        if ( s != nullptr )
          ss_s->target = s->target;
        else
          ss_s->target = ab::target;

        p()->active.streaking_stars->snapshot_state( ss_s, result_amount_type::DMG_DIRECT );
        p()->active.streaking_stars->schedule_execute( ss_s );
        p()->proc.streaking_star->occur();
      }
      p()->previous_streaking_stars = new_ability;
    }
  }

  bool verify_actor_spec() const override
  {
    if ( p()->find_affinity_spell( ab::name() )->found() || free_cast )
      return true;
#ifndef NDEBUG
    else if ( p()->sim->debug || p()->sim->log )
      p()->sim->print_log( "{} failed verification", ab::name() );
#endif

    return ab::verify_actor_spec();
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    auto splits = util::string_split( name_str, "." );

    if ( splits[ 0 ] == "dot" &&
         ( splits[ 2 ] == "ticks_gained_on_refresh" || splits[ 2 ] == "ticks_gained_on_refresh_pmultiplier" ) )
    {
      // Since we know some action names don't map to the actual dot portion, lets add some exceptions
      // this may have to be made more robust if other specs are interested in using it, but for now lets
      // default any ambiguity to what would make most sense for ferals.
      if ( splits[ 1 ] == "rake" )
        splits[ 1 ] = "rake_bleed";
      if ( p()->specialization() == DRUID_FERAL && splits[ 1 ] == "moonfire" )
        splits[ 1 ] = "lunar_inspiration";

      bool pmult_adjusted = false;
      if ( splits[ 2 ] == "ticks_gained_on_refresh_pmultiplier" )
        pmult_adjusted = true;

      if ( splits[ 1 ] == "primal_wrath" )  // special case since pw applies in aoe
      {
        action_t* rip = p()->find_action( "rip" );
        action_t* pw  = p()->find_action( "primal_wrath" );

        return make_fn_expr( name_str, [pw, rip, pmult_adjusted]() -> double {
          int cp = as<int>( pw->player->resources.current[ RESOURCE_COMBO_POINT ] );

          double gained_ticks  = 0;
          timespan_t duration  = rip->dot_duration * 0.5 * ( 1 + cp );
          timespan_t tick_time = rip->base_tick_time * rip->composite_haste();

          for ( player_t* pw_target : pw->targets_in_range_list( pw->target_list() ) )
          {
            timespan_t ttd         = pw_target->time_to_percent( 0 );
            dot_t* dot             = pw_target->get_dot( "rip", pw->player );
            double remaining_ticks = 0;
            double potential_ticks = 0;
            timespan_t durrem      = timespan_t::zero();
            double pmult           = 0;

            if ( dot->is_ticking() )
            {
              remaining_ticks = std::min( dot->remains(), ttd ) / tick_time;
              durrem          = rip->calculate_dot_refresh_duration( dot, duration );
              remaining_ticks *= pmult_adjusted ? dot->state->persistent_multiplier : 1.0;
            }

            if ( pmult_adjusted )
            {
              action_state_t* state = rip->get_state();

              pw->snapshot_state( state, result_amount_type::NONE );
              state->target = pw_target;

              pmult = pw->composite_persistent_multiplier( state );

              action_state_t::release( state );
            }

            potential_ticks = std::min( std::max( durrem, duration ), ttd ) / tick_time;
            potential_ticks *= pmult_adjusted ? pmult : 1.0;
            gained_ticks += potential_ticks - remaining_ticks;
          }
          return gained_ticks;
        } );
      }
      else if ( splits[ 1 ] == "thrash_cat" )  // special case since pw applies in aoe
      {
        action_t* tc = p()->find_action( "thrash_cat" );

        return make_fn_expr( name_str, [tc, pmult_adjusted]() -> double {
          double gained_ticks  = 0;
          timespan_t duration  = tc->dot_duration;
          timespan_t tick_time = tc->base_tick_time * tc->composite_haste();

          for ( player_t* tc_target : tc->targets_in_range_list( tc->target_list() ) )
          {
            timespan_t ttd         = tc_target->time_to_percent( 0 );
            dot_t* dot             = tc_target->get_dot( "Thrash", tc->player );
            double remaining_ticks = 0;
            double potential_ticks = 0;
            timespan_t durrem      = timespan_t::zero();
            double pmult           = 0;

            if ( dot->is_ticking() )
            {
              remaining_ticks = std::min( dot->remains(), ttd ) / tick_time;
              durrem          = tc->calculate_dot_refresh_duration( dot, duration );
              remaining_ticks *= pmult_adjusted ? dot->state->persistent_multiplier : 1.0;
            }

            if ( pmult_adjusted )
            {
              action_state_t* state = tc->get_state();

              tc->snapshot_state( state, result_amount_type::NONE );
              state->target = tc_target;

              pmult = tc->composite_persistent_multiplier( state );

              action_state_t::release( state );
            }

            potential_ticks = std::min( std::max( durrem, duration ), ttd ) / tick_time;
            potential_ticks *= pmult_adjusted ? pmult : 1.0;
            gained_ticks += potential_ticks - remaining_ticks;
          }
          return gained_ticks;
        } );
      }

      action_t* action = p()->find_action( splits[ 1 ] );
      if ( action )
        return make_fn_expr( name_str, [action, pmult_adjusted]() -> double {
          dot_t* dot             = action->get_dot();
          double remaining_ticks = 0;
          double potential_ticks = 0;
          action_state_t* state  = action->get_state( dot->state );
          timespan_t duration    = action->composite_dot_duration( state );
          timespan_t ttd         = action->target->time_to_percent( 0 );
          double pmult           = 0;

          if ( dot->is_ticking() )
          {
            remaining_ticks = std::min( dot->remains(), ttd ) / dot->current_action->tick_time( dot->state );
            duration        = action->calculate_dot_refresh_duration( dot, duration );
            remaining_ticks *= pmult_adjusted ? dot->state->persistent_multiplier : 1.0;
          }

          if ( pmult_adjusted )
          {
            action->snapshot_state( state, result_amount_type::NONE );
            state->target = action->target;

            pmult = action->composite_persistent_multiplier( state );
          }

          potential_ticks = std::min( duration, ttd ) / action->tick_time( state );
          potential_ticks *= pmult_adjusted ? pmult : 1.0;
          action_state_t::release( state );
          return potential_ticks - remaining_ticks;
        } );
      throw std::invalid_argument( "invalid action" );
    }
    else if ( splits[ 0 ] == "ticks_gained_on_refresh" )
    {
      return make_fn_expr( name_str, [this]() -> double {
        dot_t* dot             = this->get_dot();
        double remaining_ticks = 0;
        double potential_ticks = 0;
        timespan_t duration    = this->dot_duration;
        timespan_t ttd         = this->target->time_to_percent( 0 );

        if ( dot->is_ticking() )
        {
          remaining_ticks = std::min( dot->remains(), ttd ) / dot->current_action->tick_time( dot->state );
          duration        = this->calculate_dot_refresh_duration( dot, duration );
        }

        potential_ticks = std::min( duration, ttd ) / ( this->base_tick_time * this->composite_haste() );
        return potential_ticks - remaining_ticks;
      } );
    }

    return ab::create_expression( name_str );
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
  double ooc_chance;
  double bleed_mul;

  druid_attack_t( util::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ), direct_bleed( false ), ooc_chance( 0.0 ), bleed_mul( 0.0 )
  {
    ab::may_glance = false;
    ab::special    = true;

    parse_special_effect_data();

    // 7.00 PPM via community testing (~368k auto attacks)
    // https://docs.google.com/spreadsheets/d/1vMvlq1k3aAuwC1iHyDjqAneojPZusdwkZGmatuWWZWs/edit#gid=1097586165
    if ( player->spec.omen_of_clarity->ok() )
      ooc_chance = 7.00;

    if ( player->talent.moment_of_clarity->ok() )
      ooc_chance *= 1.0 + player->talent.moment_of_clarity->effectN( 2 ).percent();
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

  timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == timespan_t::zero() )
      return g;

    if ( g < ab::min_gcd )
      return ab::min_gcd;
    else
      return g;
  }

  double target_armor( player_t* t ) const override
  {
    if ( direct_bleed )
      return 0.0;
    else
      return ab::target_armor( t );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    if ( bleed_mul && t->debuffs.bleeding && t->debuffs.bleeding->up() )
      tm *= 1.0 + bleed_mul;

    return tm;
  }

  void trigger_clearcasting( action_state_t* s )
  {
    int active    = ab::p()->buff.clearcasting->check();
    double chance = ab::weapon->proc_chance_on_swing( ooc_chance );

    // Internal cooldown is handled by buff.
    if ( ab::p()->buff.clearcasting->trigger( 1, buff_t::DEFAULT_VALUE(), chance,
                                              ab::p()->buff.clearcasting->buff_duration() ) )
    {
      ab::p()->proc.clearcasting->occur();

      for ( int i = 0; i < active; i++ )
        ab::p()->proc.clearcasting_wasted->occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::is_auto_attack && ooc_chance && ab::result_is_hit( s->result ) )
      trigger_clearcasting( s );
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

  druid_spell_base_t( util::string_view n, druid_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ), reset_melee_swing( true )
  {}

  timespan_t gcd() const override
  {
    timespan_t g = ab::trigger_gcd;

    if ( g == timespan_t::zero() )
      return g;

    g *= ab::composite_haste();

    if ( g < ab::min_gcd )
      return ab::min_gcd;
    else
      return g;
  }

  void execute() override
  {
    if ( ab::time_to_execute > timespan_t::zero() && !ab::proc && !ab::background && reset_melee_swing )
    {
      if ( ab::p()->main_hand_attack && ab::p()->main_hand_attack->execute_event )
        ab::p()->main_hand_attack->execute_event->reschedule( ab::p()->main_hand_weapon.swing_time );
      // Nothing for OH, as druids don't DW
    }

    ab::execute();
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
  druid_spell_t( util::string_view token, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() )
    : base_t( token, p, s )
  {
    parse_options( options );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = ab::composite_da_multiplier( s );

    if ( ( &ab::data() == p()->spec.full_moon || &ab::data() == p()->spec.half_moon ||
           &ab::data() == p()->talent.new_moon ) &&
         p()->buff.eclipse_solar->up() )
    {
      da *= 1.0 + p()->cache.mastery_value();
    }

    return da;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance();

    if ( ( &ab::data() == p()->spec.full_moon || &ab::data() == p()->spec.half_moon ||
           &ab::data() == p()->talent.new_moon ) &&
         p()->buff.balance_of_all_things_nature->up() )
    {
      // Use the base_value stored for APL purposes for now until moons are properly whitelisted
      cc += p()->buff.balance_of_all_things_nature->stack_value() / 100.0;
    }

    return cc;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = ab::composite_energize_amount( s );

    if ( energize_resource_() == RESOURCE_ASTRAL_POWER )
    {
      if ( p()->buffs.memory_of_lucid_dreams->up() )
        e *= 1.0 + p()->buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();
    }

    return e;
  }

  void consume_resource() override
  {
    ab::consume_resource();

    trigger_astral_power_consumption_effects();
  }

  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    bool ab = ab::consume_cost_per_tick( dot );

    if ( ab::consume_per_tick_ )
      trigger_astral_power_consumption_effects();

    return ab;
  }

  bool usable_moving() const override
  {
    if ( p()->talent.stellar_drift->ok() && p()->buff.starfall->check() &&
         data().affected_by( p()->spec.stellar_drift->effectN( 1 ) ) )
      return true;

    return spell_t::usable_moving();
  }

  void proc_shooting_stars( player_t* t )
  {
    if ( !p()->spec.shooting_stars->ok() || !p()->active.shooting_stars )
      return;

    double chance = p()->spec.shooting_stars->effectN( 1 ).percent();
    chance /= std::sqrt( p()->get_active_dots( internal_id ) );

    if ( p()->buff.solstice->up() )
      chance *= 1.0 + p()->buff.solstice->data().effectN( 1 ).percent();

    if ( rng().roll( chance ) )
    {
      p()->active.shooting_stars->set_target( t );
      p()->active.shooting_stars->execute();
    }
  }

  virtual void trigger_astral_power_consumption_effects()
  {
    if ( p()->legendary.primordial_arcanic_pulsar->ok() )
    {
      if ( resource_current == RESOURCE_ASTRAL_POWER && last_resource_cost > 0.0 )
      {
        if ( !p()->buff.primordial_arcanic_pulsar->check() )
          p()->buff.primordial_arcanic_pulsar->trigger();

        p()->buff.primordial_arcanic_pulsar->current_value += last_resource_cost;

        if ( p()->buff.primordial_arcanic_pulsar->value() >=
             p()->legendary.primordial_arcanic_pulsar->effectN( 1 ).base_value() )
        {
          p()->buff.primordial_arcanic_pulsar->current_value -=
              p()->legendary.primordial_arcanic_pulsar->effectN( 1 ).base_value();
          timespan_t pulsar_dur =
              timespan_t::from_seconds( p()->legendary.primordial_arcanic_pulsar->effectN( 2 ).base_value() );
          buff_t* proc_buff =
              p()->talent.incarnation_moonkin->ok() ? p()->buff.incarnation_moonkin : p()->buff.celestial_alignment;

          if ( proc_buff->check() )
          {
            proc_buff->extend_duration( p(), pulsar_dur );
            p()->buff.eclipse_lunar->extend_duration( p(), pulsar_dur );
            p()->buff.eclipse_solar->extend_duration( p(), pulsar_dur );
          }
          else
            proc_buff->trigger( pulsar_dur );
        }
      }
    }
  };

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    auto splits = util::string_split<util::string_view>( name_str, "." );

    // check for AP overcap on current action. check for other action handled in druid_t::create_expression
    // syntax: ap_check.<allowed overcap = 0>
    if ( p()->specialization() == DRUID_BALANCE && util::str_compare_ci( splits[ 0 ], "ap_check" ) )
    {
      return make_fn_expr( name_str, [this, splits]() {
        action_state_t* state = this->get_state();
        double ap             = p()->resources.current[ RESOURCE_ASTRAL_POWER ];

        ap += composite_energize_amount( state );
        ap += p()->spec.shooting_stars_dmg->effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
        ap += p()->talent.natures_balance->ok() ? std::ceil( time_to_execute / 2_s ) : 0;

        action_state_t::release( state );
        return ap <=
               p()->resources.base[ RESOURCE_ASTRAL_POWER ] + ( splits.size() >= 2 ? util::to_int( splits[ 1 ] ) : 0 );
      } );
    }

    return ab::create_expression( name_str );
  }
};  // end druid_spell_t

// Shooting Stars ===========================================================

struct shooting_stars_t : public druid_spell_t
{
  shooting_stars_t( druid_t* player ) : druid_spell_t( "shooting_stars", player, player->spec.shooting_stars_dmg )
  {
    background = true;
  }

  timespan_t travel_time() const override
  {
    // has a set travel time since it spawns on the target
    return timespan_t::from_seconds( data().missile_speed() );
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  struct moonfire_damage_t : public druid_spell_t
  {
  private:
    double galactic_guardian_dd_multiplier;

  protected:
    bool benefits_from_galactic_guardian;

  public:
    moonfire_damage_t( druid_t* p ) : moonfire_damage_t( p, "moonfire_dmg" ) {}

    moonfire_damage_t( druid_t* p, util::string_view n ) : druid_spell_t( n, p, p->spec.moonfire_dmg )
    {
      if ( p->spec.astral_power->ok() )
      {
        energize_resource = RESOURCE_ASTRAL_POWER;
        energize_amount   = p->spec.astral_power->effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
      }
      else
        energize_type = action_energize::NONE;

      if ( p->spec.shooting_stars->ok() && !p->active.shooting_stars )
        p->active.shooting_stars = new shooting_stars_t( p );

      if ( p->talent.galactic_guardian->ok() )
        galactic_guardian_dd_multiplier = p->find_spell( 213708 )->effectN( 3 ).percent();

      aoe      = 1;
      may_miss = false;
      dual = background = true;

      triggers_galactic_guardian      = false;
      benefits_from_galactic_guardian = true;

      if ( p->talent.twin_moons->ok() )
      {
        // The increased target number has been removed from spell data
        aoe += 1;
        // Twin Moons seems to fire off another MF spell - sometimes concurrently, sometimes up to 100ms later. While
        // there are very limited cases where this could have an effect, those cases do exist. Possibly worth
        // investigating further in the future.
        radius = p->talent.twin_moons->effectN( 1 ).base_value();
      }

      // June 2016: This hotfix is negated if you shift into Moonkin Form (ever), so only apply it if the druid does not
      // have balance affinity. */
      if ( !p->talent.balance_affinity->ok() )
      {
        base_dd_multiplier *= 1.0 + p->spec.feral_overrides->effectN( 1 ).percent();
        base_td_multiplier *= 1.0 + p->spec.feral_overrides->effectN( 2 ).percent();
      }
    }

    double action_multiplier() const override
    {
      double am = druid_spell_t::action_multiplier();

      if ( p()->legendary.draught_of_deep_focus->ok() && p()->get_active_dots( internal_id ) == 1 )
        am *= 1.0 + p()->legendary.draught_of_deep_focus->effectN( 1 ).percent();

      return am;
    }

    double bonus_ta( const action_state_t* s ) const override
    {
      double ta = druid_spell_t::bonus_ta( s );

      ta += p()->azerite.power_of_the_moon.value( 2 );

      return ta;
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( !t )
        t = target;
      if ( !t )
        return nullptr;

      return td( t )->dots.moonfire;
    }

    double composite_target_da_multiplier( player_t* t ) const override
    {
      double tdm = druid_spell_t::composite_target_da_multiplier( t );

      // IN-GAME BUG (Jan 20 2018): Lady and the Child Galactic Guardian procs benefit from both the DD modifier and the
      // rage gain.
      if ( ( benefits_from_galactic_guardian || ( p()->bugs && t != target ) ) && p()->buff.galactic_guardian->check() )
      {
        // Galactic Guardian 7.1 Damage Buff
        tdm *= 1.0 + galactic_guardian_dd_multiplier;
      }

      return tdm;
    }

    void tick( dot_t* d ) override
    {
      druid_spell_t::tick( d );

      proc_shooting_stars( d->target );

      if ( !p()->azerite.power_of_the_moon.ok() )
        return;

      if ( !p()->rppm.power_of_the_moon->trigger() )
        return;

      p()->proc.power_of_the_moon->occur();
    }

    void impact( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      // The buff needs to be handled with the damage handler since 7.1 since it impacts Moonfire DD
      // IN-GAME BUG (Jan 20 2018): Lady and the Child Galactic Guardian procs benefit from both the DD modifier and the
      // rage gain.
      if ( ( benefits_from_galactic_guardian || ( p()->bugs && s->chain_target > 0 ) ) && result_is_hit( s->result ) &&
           p()->buff.galactic_guardian->check() )
      {
        p()->resource_gain( RESOURCE_RAGE, p()->buff.galactic_guardian->value(), p()->gain.galactic_guardian );

        // buff is not consumed when bug occurs
        if ( benefits_from_galactic_guardian )
          p()->buff.galactic_guardian->expire();
      }
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      /* When Twin Moons is active, this is an AoE action meaning it will impact onto the
      first 2 targets in the target list. Instead, we want it to impact on the target of the action
      and 1 additional, so we'll override the target_list to make it so. */
      if ( is_aoe() )
      {
        // Get the full target list.
        druid_spell_t::available_targets( tl );
        std::vector<player_t*> full_list = tl;

        tl.clear();
        // Add the target of the action.
        tl.push_back( target );

        // Loop through the full list and sort into afflicted/unafflicted.
        std::vector<player_t*> afflicted;
        std::vector<player_t*> unafflicted;

        for ( size_t i = 0; i < full_list.size(); i++ )
        {
          if ( full_list[ i ] == target || full_list[i]->debuffs.invulnerable->up() )
            continue;

          if ( td( full_list[ i ] )->dots.moonfire->is_ticking() )
            afflicted.push_back( full_list[ i ] );
          else
            unafflicted.push_back( full_list[ i ] );
        }

        // Fill list with random unafflicted targets.
        while ( tl.size() < as<size_t>( aoe ) && unafflicted.size() > 0 )
        {
          // Random target
          size_t i = static_cast<size_t>( p()->rng().range( 0, as<double>( unafflicted.size() ) ) );

          tl.push_back( unafflicted[ i ] );
          unafflicted.erase( unafflicted.begin() + i );
        }

        // Fill list with random afflicted targets.
        while ( tl.size() < as<size_t>( aoe ) && afflicted.size() > 0 )
        {
          // Random target
          size_t i = static_cast<size_t>( p()->rng().range( 0, as<double>( afflicted.size() ) ) );

          tl.push_back( afflicted[ i ] );
          afflicted.erase( afflicted.begin() + i );
        }

        return tl.size();
      }

      return druid_spell_t::available_targets( tl );
    }

    void execute() override
    {
      // Force invalidate target cache so that it will impact on the correct targets.
      target_cache.is_valid = false;

      druid_spell_t::execute();
    }
  };

  struct galactic_guardian_damage_t : public moonfire_damage_t
  {
    galactic_guardian_damage_t( druid_t* p ) : moonfire_damage_t( p, "galactic_guardian" )
    {
      benefits_from_galactic_guardian = false;
    }

    void execute() override
    {
      moonfire_damage_t::execute();

      p()->buff.galactic_guardian->trigger();
    }
  };

  action_t* damage;  // Add damage modifiers in moonfire_damage_t, not moonfire_t

  moonfire_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "moonfire", p, p->find_class_spell( "Moonfire" ), options_str )
  {
    may_miss = may_crit = false;
    damage = p->get_secondary_action<moonfire_damage_t>( "moonfire_dmg" );
    damage->stats = stats;

    if ( p->active.galactic_guardian )
      add_child( p->active.galactic_guardian );

    if ( p->spec.astral_power->ok() )
    {
      energize_resource = RESOURCE_ASTRAL_POWER;
      energize_amount   = p->spec.astral_power->effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
      energize_type = action_energize::ON_HIT;
    }
    else
      energize_type = action_energize::NONE;
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    streaking_stars_trigger( SS_MOONFIRE, s );

    if ( result_is_hit( s->result ) )
      trigger_gore();
  }

  void execute() override
  {
    if ( free_cast )
      damage->stats = get_free_cast_stats( free_cast );
    else
      damage->stats = orig_stats;

    druid_spell_t::execute();

    damage->target = execute_state->target;
    damage->schedule_execute();
  }
};
}  // namespace spells

namespace heals
{
struct druid_heal_t : public druid_spell_base_t<heal_t>
{
  bool target_self;

  druid_heal_t( util::string_view token, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                const std::string& options_str = std::string() )
    : base_t( token, p, s ), target_self( false )
  {
    add_option( opt_bool( "target_self", target_self ) );
    parse_options( options_str );

    if ( target_self )
      target = p;

    may_miss          = false;
    weapon_multiplier = 0;
    harmful           = false;
  }

public:
  void execute() override
  {
    base_t::execute();

    if ( p()->mastery.harmony->ok() && spell_power_mod.direct > 0 && !background )
      p()->buff.harmony->trigger( 1, p()->mastery.harmony->ok() ? p()->cache.mastery_value() : 0.0 );
  }

  double action_da_multiplier() const override
  {
    double adm = base_t::action_da_multiplier();

    adm *= 1.0 + p()->buff.incarnation_tree->check_value();

    if ( p()->mastery.harmony->ok() )
      adm *= 1.0 + p()->cache.mastery_value();

    return adm;
  }

  double action_ta_multiplier() const override
  {
    double atm = base_t::action_da_multiplier();

    atm *= 1.0 + p()->buff.incarnation_tree->check_value();

    if ( p()->mastery.harmony->ok() )
      atm *= 1.0 + p()->cache.mastery_value();

    return atm;
  }
};  // end druid_heal_t

}  // namespace heals

namespace caster_attacks
{
// Caster Form Melee Attack =================================================

struct caster_attack_t : public druid_attack_t<melee_attack_t>
{
  caster_attack_t( util::string_view token, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                   const std::string& options = std::string() )
    : base_t( token, p, s )
  {
    parse_options( options );
  }
};  // end druid_caster_attack_t

struct druid_melee_t : public caster_attack_t
{
  druid_melee_t( druid_t* p ) : caster_attack_t( "caster_melee", p )
  {
    may_glance = background = repeating = is_auto_attack = true;

    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = timespan_t::zero();
    special           = false;
    weapon_multiplier = 1.0;
  }

  timespan_t execute_time() const override
  {
    if ( !player->in_combat )
      return timespan_t::from_seconds( 0.01 );

    return caster_attack_t::execute_time();
  }
};
}  // namespace caster_attacks

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
  bool requires_stealth;
  bool consumes_combo_points;
  bool trigger_untamed_ferocity;
  double berserk_cp;

  struct has_snapshot_t
  {
    bool tigers_fury;
    bool bloodtalons;
    bool clearcasting;
  } snapshots;

  snapshot_counter_t* bt_counter;
  snapshot_counter_t* tf_counter;

  std::vector<buff_effect_t> persistent_multiplier_buffeffects;

  cat_attack_t( util::string_view token, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() )
    : base_t( token, p, s ),
      requires_stealth( false ),
      consumes_combo_points( false ),
      trigger_untamed_ferocity( data().affected_by( p->azerite.untamed_ferocity.spell()->effectN( 2 ) ) ),
      berserk_cp( 0.0 ),
      snapshots( has_snapshot_t() ),
      bt_counter( nullptr ),
      tf_counter( nullptr ),
      persistent_multiplier_buffeffects()
  {
    parse_options( options );

    if ( data().cost( POWER_COMBO_POINT ) )
    {
      consumes_combo_points = true;
      form_mask |= CAT_FORM;
      berserk_cp = p->spec.berserk_cat->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_COMBO_POINT ) +
                   p->find_rank_spell( "Berserk", "Rank 2" )->effectN( 1 ).resource( RESOURCE_COMBO_POINT );
    }

    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
      ap_type = attack_power_type::NO_WEAPON;

    if ( trigger_untamed_ferocity && !p->azerite.untamed_ferocity.ok() )
      trigger_untamed_ferocity = false;

    using S = const spell_data_t*;
    using C = const conduit_data_t&;

    snapshots.bloodtalons  = parse_persistent_buff_effects( p->buff.bloodtalons );
    snapshots.tigers_fury  = parse_persistent_buff_effects<C>( p->buff.tigers_fury,
                                                               p->conduit.carnivorous_instinct );
    snapshots.clearcasting = parse_persistent_buff_effects<S>( p->buff.clearcasting,
                                                               p->talent.moment_of_clarity );

    if ( data().affected_by( p->mastery.razor_claws->effectN( 1 ) ) )
    {
      auto val = p->mastery.razor_claws->effectN( 1 ).percent();
      da_multiplier_buffeffects.push_back( buff_effect_t( nullptr, val, true ) );
      p->sim->print_debug( "buff-effects: {} direct damage modified by {}%+mastery", name(), val * 100.0 );
    }

    if ( data().affected_by( p->mastery.razor_claws->effectN( 2 ) ) )
    {
      auto val = p->mastery.razor_claws->effectN( 2 ).percent();
      ta_multiplier_buffeffects.push_back( buff_effect_t( nullptr, val , true ) );
      p->sim->print_debug( "buff-effects: {} tick damage modified by {}%+mastery", name(), val * 100.0 );
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

    // Treat Omen of Clarity energy savings like an energy gain for tracking purposes.
    if ( snapshots.clearcasting && current_resource() == RESOURCE_ENERGY && p()->buff.clearcasting->up() )
    {
      // Base cost doesn't factor in but Omen of Clarity does net us less energy during it, so account for that here.
      eff_cost *= 1.0 + p()->buff.incarnation_cat->check_value();

      p()->gain.clearcasting->add( RESOURCE_ENERGY, eff_cost );
    }

    base_t::consume_resource();

    if ( snapshots.clearcasting && current_resource() == RESOURCE_ENERGY )
    {
      p()->buff.clearcasting->decrement();

      if ( p()->legendary.cateye_curio->ok() )
      {
        // TODO: check if we need to modify by eff_cost for berserk/incarn
        p()->resource_gain( RESOURCE_ENERGY, eff_cost * p()->legendary.cateye_curio->effectN( 1 ).percent(),
                            p()->gain.cateye_curio );
      }
    }

    if ( consumes_combo_points && hit_any_target )
    {
      int consumed = as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] );

      p()->resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );

      if ( sim->log )
      {
        sim->print_log( "{} consumes {} {} for {} (0)", player->name(), consumed,
                        util::resource_type_string( RESOURCE_COMBO_POINT ), name() );
      }

      stats->consume_resource( RESOURCE_COMBO_POINT, consumed );

      if ( p()->buff.berserk_cat->check() || p()->buff.incarnation_cat->check() )
      {
        if ( rng().roll( consumed * p()->spec.berserk_cat->effectN( 1 ).percent() ) )
        {
          p()->resource_gain( RESOURCE_COMBO_POINT, berserk_cp, p()->gain.berserk );
        }
      }

      if ( p()->spec.predatory_swiftness->ok() )
        p()->buff.predatory_swiftness->trigger( 1, 1, consumed * 0.20 );

      if ( p()->talent.soul_of_the_forest_cat->ok() )
      {
        p()->resource_gain( RESOURCE_ENERGY,
                            consumed * p()->talent.soul_of_the_forest_cat->effectN( 1 ).resource( RESOURCE_ENERGY ),
                            p()->gain.soul_of_the_forest );
      }

      if ( p()->buff.eye_of_fearful_symmetry->up() )
      {
        p()->resource_gain( RESOURCE_COMBO_POINT, p()->buff.eye_of_fearful_symmetry->check_value(),
                            p()->gain.eye_of_fearful_symmetry );
      }

      if ( p()->conduit.sudden_ambush->ok() && rng().roll( p()->conduit.sudden_ambush.percent() * consumed ) )
        p()->buff.sudden_ambush->trigger();
    }
  }

  template <typename... Ts>
  bool parse_persistent_buff_effects( buff_t* buff, Ts... mods )
  {
    size_t ta_old   = ta_multiplier_buffeffects.size();
    size_t da_old   = da_multiplier_buffeffects.size();
    size_t cost_old = cost_buffeffects.size();

    parse_buff_effects( buff, mods... );

    // If there is a new entry in the ta_mul table, move it to the pers_mul table.
    if ( ta_multiplier_buffeffects.size() > ta_old )
    {
      double ta_val = ta_multiplier_buffeffects.back().value;
      double da_val = 0;

      p()->sim->print_debug( "persistent-buffs: {} damage modified by {}% with buff {}", name(),
                             ta_val * 100.0, buff->name() );
      persistent_multiplier_buffeffects.push_back( ta_multiplier_buffeffects.back() );
      ta_multiplier_buffeffects.pop_back();

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

      return true;
    }
    // no persistent multiplier, but does snapshot & consume the buff
    if ( da_multiplier_buffeffects.size() > da_old || cost_buffeffects.size() > cost_old )
      return true;

    return false;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = base_t::composite_persistent_multiplier( s ) *
                get_buff_effects_value( persistent_multiplier_buffeffects, false, true, false );

    return pm;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = base_t::bonus_da( s );

    if ( trigger_untamed_ferocity )
      da += p()->azerite.untamed_ferocity.value( 2 );

    return da;
  }

  void init() override
  {
    base_t::init();

    if ( snapshots.bloodtalons )
      bt_counter = new snapshot_counter_t( p(), p()->buff.bloodtalons );
    if ( snapshots.tigers_fury )
      tf_counter = new snapshot_counter_t( p(), p()->buff.tigers_fury );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( s->result == RESULT_CRIT )
        attack_critical = true;

      if ( p()->legendary.frenzyband->ok() && ( p()->buff.berserk_cat->check() || p()->buff.incarnation_cat->check() ) )
        trigger_frenzyband( s->target, s->result_amount );
    }
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    trigger_predator();

    if ( d->state->persistent_multiplier > 1.0 )
    {
      if ( snapshots.bloodtalons )
        bt_counter->count_tick();

      if ( snapshots.tigers_fury )
        tf_counter->count_tick();
    }
  }

  void execute() override
  {
    attack_critical = false;

    base_t::execute();

    if ( energize_resource == RESOURCE_COMBO_POINT && energize_amount > 0 && hit_any_target && attack_critical )
    {
      if ( p()->specialization() == DRUID_FERAL ||
           ( p()->talent.feral_affinity->ok() && p()->buff.heart_of_the_wild->check() ) )
      {
        trigger_primal_fury();
      }
    }

    if ( hit_any_target )
    {
      if ( snapshots.bloodtalons )
      {
        bt_counter->count_execute();
        p()->buff.bloodtalons->decrement();
      }

      if ( snapshots.tigers_fury )
        tf_counter->count_execute();

      if ( trigger_untamed_ferocity )
      {
        p()->cooldown.berserk->adjust( -p()->azerite.untamed_ferocity.time_value( 3 ), false );
        p()->cooldown.incarnation->adjust( -p()->azerite.untamed_ferocity.time_value( 4 ), false );
      }

      if ( p()->legendary.frenzyband->ok() )
      {
        p()->cooldown.berserk->adjust( -p()->legendary.frenzyband->effectN( 1 ).time_value(), false );
        p()->cooldown.incarnation->adjust( -p()->legendary.frenzyband->effectN( 1 ).time_value(), false );
      }
    }

    if ( !hit_any_target )
      trigger_energy_refund();

    if ( harmful )
    {
      p()->buff.prowl->expire();
      p()->buffs.shadowmeld->expire();

      // Track benefit of damage buffs
      p()->buff.tigers_fury->up();
      p()->buff.savage_roar->up();

      if ( special && base_costs[ RESOURCE_ENERGY ] > 0 )
        p()->buff.incarnation_cat->up();
    }
  }

  bool ready() override
  {
    if ( requires_stealth && !stealthed() )
      return false;

    if ( consumes_combo_points && p()->resources.current[ RESOURCE_COMBO_POINT ] < 1 )
      return false;

    return base_t::ready();
  }

  void trigger_energy_refund()
  {
    player->resource_gain( RESOURCE_ENERGY, last_resource_cost * 0.80, p()->gain.energy_refund );
  }

  virtual void trigger_primal_fury()
  {
    if ( !p()->spec.primal_fury->ok() )
      return;

    p()->proc.primal_fury->occur();
    p()->resource_gain( RESOURCE_COMBO_POINT, p()->spec.primal_fury->effectN( 1 ).base_value(), p()->gain.primal_fury );
  }

  void trigger_predator()
  {
    if ( !( p()->talent.predator->ok() && p()->predator_rppm_rate > 0 ) )
      return;

    if ( !p()->rppm.predator->trigger() )
      return;

    if ( !p()->cooldown.tigers_fury->down() )
      p()->proc.predator_wasted->occur();

    p()->cooldown.tigers_fury->reset( true );
    p()->proc.predator->occur();
  }

  void trigger_frenzyband( player_t* t, double d )
  {
    if ( !special || !harmful || !d )
      return;

    residual_action::trigger( p()->active.frenzied_assault, t, p()->legendary.frenzyband->effectN( 2 ).percent() * d );
  }
};  // end druid_cat_attack_t

// Cat Melee Attack =========================================================

struct cat_melee_t : public cat_attack_t
{
  cat_melee_t( druid_t* player ) : cat_attack_t( "cat_melee", player, spell_data_t::nil(), "" )
  {
    form_mask  = CAT_FORM;
    may_glance = background = repeating = is_auto_attack = true;

    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = timespan_t::zero();
    special           = false;
    weapon_multiplier = 1.0;
  }

  timespan_t execute_time() const override
  {
    if ( !player->in_combat )
      return timespan_t::from_seconds( 0.01 );

    return cat_attack_t::execute_time();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( p()->talent.rend_and_tear->ok() )
      tm *= 1.0 + p()->talent.rend_and_tear->effectN( 3 ).percent() * td( t )->dots.thrash_bear->current_stack();

    return tm;
  }
};

// Rip State ================================================================

struct rip_state_t : public druid_action_state_t
{
  int combo_points;
  druid_t* druid;

  rip_state_t( druid_t* p, action_t* a, player_t* t ) : druid_action_state_t( a, t ), combo_points( 0 ), druid( p ) {}

  void initialize() override
  {
    druid_action_state_t::initialize();

    combo_points = as<int>( druid->resources.current[ RESOURCE_COMBO_POINT ] );
  }

  void copy_state( const action_state_t* state ) override
  {
    druid_action_state_t::copy_state( state );
    const rip_state_t* rip_state = debug_cast<const rip_state_t*>( state );

    combo_points = rip_state->combo_points;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    druid_action_state_t::debug_str( s );

    s << " combo_points=" << combo_points;

    return s;
  }
};

// Berserk ==================================================================

struct berserk_cat_t : public cat_attack_t
{
  berserk_cat_t( druid_t* player, const std::string& options_str )
    : cat_attack_t( "berserk", player, player->spec.berserk_cat, options_str )
  {
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
    cooldown->duration *= 1.0 + player->vision_of_perfection_cdr;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.berserk_cat->trigger();
  }

  bool ready() override
  {
    if ( p()->talent.incarnation_cat->ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Brutal Slash =============================================================

struct brutal_slash_t : public cat_attack_t
{
  brutal_slash_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "brutal_slash", p, p->talent.brutal_slash, options_str )
  {
    aoe               = as<int>( data().effectN( 3 ).base_value() );
    energize_amount   = data().effectN( 2 ).base_value();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type     = action_energize::ON_HIT;
    cooldown->hasted  = true;
  }

  double cost() const override
  {
    double c = cat_attack_t::cost();

    c -= p()->buff.scent_of_blood->check_stack_value();

    return c;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = cat_attack_t::bonus_da( s );

    if ( td( s->target )->dots.thrash_cat->is_ticking() || td( s->target )->dots.thrash_bear->is_ticking() )
    {
      b += p()->azerite.wild_fleshrending.value( 2 );
    }

    return b;
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( hit_any_target )
      p()->buff.bt_brutal_slash->trigger();
  }
};

//==== Feral Frenzy ==============================================

//Currently incorrect TODO(Xanzara)
struct feral_frenzy_driver_t : public cat_attack_t
{
  struct feral_frenzy_dot_t : public cat_attack_t
  {
    bool is_direct_damage;

    feral_frenzy_dot_t( druid_t* p ) : cat_attack_t( "feral_frenzy_tick", p, p->find_spell( 274838 ) )
    {
      background = dual = true;
      is_direct_damage  = false;
      direct_bleed      = false;
      dot_max_stack     = 5;
      // dot_behavior = DOT_CLIP;
    }

    // Refreshes, but doesn't pandemic
    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
    {
      return dot->time_to_next_tick() + triggered_duration;
    }

    // Small hack to properly distinguish instant ticks from the driver, from actual periodic ticks from the bleed
    result_amount_type report_amount_type( const action_state_t* state ) const override
    {
      if ( is_direct_damage )
        return result_amount_type::DMG_DIRECT;

      return state->result_type;
    }

    void execute() override
    {
      is_direct_damage = true;
      cat_attack_t::execute();
      is_direct_damage = false;
    }

    void impact( action_state_t* s ) override { cat_attack_t::impact( s ); }

    void trigger_primal_fury() override {}
  };

  double tick_ap_ratio;

  feral_frenzy_driver_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "feral_frenzy", p, p->find_spell( 274837 ) )
  {
    parse_options( options_str );

    tick_action = p->get_secondary_action<feral_frenzy_dot_t>( "feral_frenzy_tick" );
    tick_action->stats = stats;
    dynamic_tick_action = true;
    tick_ap_ratio = p->find_spell( 274838 )->effectN( 3 ).ap_coeff();
  }

  bool ready() override
  {
    if ( !p()->talent.feral_frenzy->ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Ferocious Bite ===========================================================

struct ferocious_bite_t : public cat_attack_t
{
  double excess_energy;
  double max_excess_energy;
  double combo_points;
  bool max_energy;
  timespan_t max_sabertooth_refresh;

  ferocious_bite_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "ferocious_bite", p, p->find_class_spell( "Ferocious Bite" ) ),
      excess_energy( 0 ),
      max_excess_energy( 0 ),
      max_energy( false ),
      max_sabertooth_refresh( p->spec.rip->duration() * p->resources.max[ RESOURCE_COMBO_POINT ] * 1.3 )
  {
    add_option( opt_bool( "max_energy", max_energy ) );
    parse_options( options_str );

    max_excess_energy = 1 * data().effectN( 2 ).base_value();
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

    return cat_attack_t::ready();
  }

  void execute() override
  {
    // Incarn does affect the additional energy consumption.
    max_excess_energy *= 1.0 + p()->buff.incarnation_cat->check_value();

    excess_energy = std::min( max_excess_energy, ( p()->resources.current[ RESOURCE_ENERGY ] - cat_attack_t::cost() ) );
    combo_points  = p()->resources.current[ RESOURCE_COMBO_POINT ];

    cat_attack_t::execute();

    max_excess_energy = 1 * data().effectN( 2 ).base_value();

    p()->buff.iron_jaws->trigger( 1,
                                  p()->azerite.iron_jaws.value( 1 ) * ( 0.5 + 0.5 / p()->azerite.iron_jaws.n_items() ),
                                  p()->azerite.iron_jaws.spell()->effectN( 2 ).percent() * combo_points );

    p()->buff.raking_ferocity->expire();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = cat_attack_t::bonus_da( s );

    da += p()->buff.raking_ferocity->value();

    return da;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s->result ) && p()->talent.sabertooth->ok() )
    {
      td( s->target )
          ->dots.rip->extend_duration(
              timespan_t::from_seconds( p()->talent.sabertooth->effectN( 2 ).base_value() * combo_points ),
              max_sabertooth_refresh, 0 );
    }
  }

  void ApexPredatorResource()
  {
    if ( p()->spec.predatory_swiftness->ok() )
      p()->buff.predatory_swiftness->trigger( 1, 1, 5 * 0.20 );

    if ( p()->talent.soul_of_the_forest_cat->ok() && p()->specialization() == DRUID_FERAL )
    {
      p()->resource_gain( RESOURCE_ENERGY,
                          5 * p()->talent.soul_of_the_forest_cat->effectN( 1 ).resource( RESOURCE_ENERGY ),
                          p()->gain.soul_of_the_forest );
    }

    p()->buff.apex_predators_craving->expire();
  }

  void consume_resource() override
  {
    // TODO: check if consumed on miss
    if ( p()->buff.apex_predators_craving->check() )
    {
      ApexPredatorResource();
      return;
    }

    // Extra energy consumption happens first. In-game it happens before the skill even casts but let's not do that
    // because its dumb.
    if ( hit_any_target )
    {
      player->resource_loss( current_resource(), excess_energy );
      stats->consume_resource( current_resource(), excess_energy );
    }

    cat_attack_t::consume_resource();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( p()->conduit.taste_for_blood->ok() )
    {
      auto t_td  = td( t );
      int bleeds = t_td->dots.rake->is_ticking() + t_td->dots.rip->is_ticking() + t_td->dots.thrash_cat->is_ticking();

      tm *= 1.0 + p()->conduit.taste_for_blood.percent() * bleeds;
    }

    return tm;
  }

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    if ( p()->buff.apex_predators_craving->up() )
      return am * 2.0;

    am *= p()->resources.current[ RESOURCE_COMBO_POINT ] / p()->resources.max[ RESOURCE_COMBO_POINT ];
    am *= 1.0 + excess_energy / max_excess_energy;

    return am;
  }
};

struct frenzied_assault_t : public residual_action::residual_periodic_action_t<cat_attack_t>
{
  frenzied_assault_t( druid_t* p )
    : residual_action::residual_periodic_action_t<cat_attack_t>( "frenzied_assault", p, p->find_spell( 340056 ) )
  {
    background = dual = proc = true;
    may_miss = may_dodge = may_parry = false;
  }
};

// Lunar Inspiration ========================================================

struct lunar_inspiration_t : public cat_attack_t
{
  lunar_inspiration_t( druid_t* player, const std::string& options_str )
    : cat_attack_t( "lunar_inspiration", player, player->find_spell( 155625 ), options_str )
  {
    may_dodge = may_parry = may_block = may_glance = false;

    hasted_ticks  = true;
    energize_type = action_energize::ON_HIT;

    gcd_type = gcd_haste_type::ATTACK_HASTE;
  }

  double bonus_ta( const action_state_t* s ) const override
  {
    double ta = cat_attack_t::bonus_ta( s );

    ta += p()->azerite.power_of_the_moon.value( 2 );

    return ta;
  }

  void execute() override
  {
    // Force invalidate target cache so that it will impact on the correct targets.
    target_cache.is_valid = false;

    cat_attack_t::execute();

    if ( hit_any_target )
      p()->buff.bt_moonfire->trigger();
  }

  bool ready() override
  {
    if ( !p()->talent.lunar_inspiration->ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Maim =====================================================================

struct maim_t : public cat_attack_t
{
  maim_t( druid_t* player, const std::string& options_str )
    : cat_attack_t( "maim", player, player->find_affinity_spell( "Maim" ), options_str ) {}

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    am *= p()->resources.current[ RESOURCE_COMBO_POINT ];

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = cat_attack_t::bonus_da( s );

    if ( p()->buff.iron_jaws->up() )
      da += p()->buff.iron_jaws->check_value();

    return da;
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( p()->buff.iron_jaws->up() )
      p()->buff.iron_jaws->expire();
  }
};

// Rake =====================================================================

struct rake_t : public cat_attack_t
{
  struct rake_bleed_t : public cat_attack_t
  {
    rake_bleed_t( druid_t* p ) : cat_attack_t( "rake_bleed", p, p->spec.rake_dmg )
    {
      background = dual = hasted_ticks = true;
      may_miss = may_parry = may_dodge = may_crit = false;
    }

    double action_multiplier() const override
    {
      double am = cat_attack_t::action_multiplier();

      if ( p()->legendary.draught_of_deep_focus->ok() && p()->get_active_dots( internal_id ) == 1 )
        am *= 1.0 + p()->legendary.draught_of_deep_focus->effectN( 1 ).percent();

      return am;
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( !t )
        t = target;
      if ( !t )
        return nullptr;

      return td( t )->dots.rake;
    }

    double bonus_ta( const action_state_t* s ) const override
    {
      double ta = cat_attack_t::bonus_ta( s );

      ta += p()->azerite.blood_mist.value( 2 );

      return ta;
    }

    void tick( dot_t* d ) override
    {
      cat_attack_t::tick( d );

      if ( !p()->azerite.blood_mist.ok() )
        return;

      if ( p()->buff.berserk_cat->check() || p()->buff.incarnation_cat->check() )
        return;

      if ( !p()->rppm.blood_mist->trigger() )
        return;

      p()->proc.blood_mist->occur();
      p()->buff.berserk_cat->trigger( 6_s );
    }
  };

  action_t* bleed;
  bool stealth_mul;

  rake_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "rake", p, p->find_affinity_spell( "Rake" ) ), stealth_mul( 0.0 )
  {
    parse_options( options_str );

    if ( p->find_rank_spell( "Rake", "Rank 2" )->ok() )
      stealth_mul = data().effectN( 4 ).percent();

    bleed = p->get_secondary_action<rake_bleed_t>( "rank_bleed" );
    bleed->stats = stats;
  }

  bool stealthed() const override
  {
    return p()->buff.berserk_cat->up() || p()->buff.incarnation_cat->up() || p()->buff.sudden_ambush->up() ||
           cat_attack_t::stealthed();
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t )
      t = target;
    if ( !t )
      return nullptr;

    return td( t )->dots.rake;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = cat_attack_t::composite_persistent_multiplier( s );

    if ( stealth_mul && stealthed() )
      pm *= 1.0 + stealth_mul;

    return pm;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    action_state_t* b_state = bleed->get_state();
    b_state->target         = s->target;
    bleed->snapshot_state( b_state, result_amount_type::DMG_OVER_TIME );
    // Copy persistent multipliers from the direct attack.
    b_state->persistent_multiplier = s->persistent_multiplier;
    bleed->schedule_execute( b_state );
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( hit_any_target )
      p()->buff.bt_rake->trigger();

    if ( p()->azerite.raking_ferocity.ok() )
      p()->buff.raking_ferocity->trigger( 1, p()->azerite.raking_ferocity.value() );

    // TODO: check if consumed on miss
    if ( p()->buff.sudden_ambush->up() )
      p()->buff.sudden_ambush->decrement();
  }
};

// Rip ======================================================================

struct rip_t : public cat_attack_t
{
  double combo_point_on_tick_proc_rate;

  rip_t( druid_t* p, const std::string& options_str ) : cat_attack_t( "rip", p, p->spec.rip, options_str )
  {
    special      = true;
    may_crit     = false;
    hasted_ticks = true;

    combo_point_on_tick_proc_rate = 0.0;

    if ( p->azerite.gushing_lacerations.ok() )
      combo_point_on_tick_proc_rate = p->find_spell( 279468 )->proc_chance();
  }

  action_state_t* new_state() override { return new rip_state_t( p(), this, target ); }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t t = cat_attack_t::composite_dot_duration( s );

    return t *= ( p()->resources.current[ RESOURCE_COMBO_POINT ] + 1 );
  }

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    if ( p()->legendary.draught_of_deep_focus->ok() && p()->get_active_dots( internal_id ) == 1 )
      am *= 1.0 + p()->legendary.draught_of_deep_focus->effectN( 1 ).percent();

    return am;
  }

  double bonus_ta( const action_state_t* s ) const override
  {
    double ta = cat_attack_t::bonus_ta( s );

    ta += p()->azerite.gushing_lacerations.value( 2 );

    if ( p()->buff.berserk_cat->up() || p()->buff.incarnation_cat->up() )
      ta += p()->azerite.primordial_rage.value();

    return ta;
  }

  void tick( dot_t* d ) override
  {
    cat_attack_t::tick( d );

    p()->buff.apex_predators_craving->trigger();

    if ( p()->rng().roll( combo_point_on_tick_proc_rate ) )
    {
      p()->resource_gain( RESOURCE_COMBO_POINT, 1, p()->gain.gushing_lacerations );
      p()->proc.gushing_lacerations->occur();
    }

    if ( p()->conduit.incessant_hunter->ok() && rng().roll( p()->conduit.incessant_hunter.percent() ) )
    {
      p()->resource_gain( RESOURCE_ENERGY,
                          p()->conduit.incessant_hunter->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_ENERGY ),
                          p()->gain.incessant_hunter );
    }
  }
};

// Primal Wrath =============================================================
struct primal_wrath_t : public cat_attack_t
{
  int combo_points;
  rip_t* rip;

  primal_wrath_t( druid_t* p, const std::string& options_str )
    : primal_wrath_t( p, p->talent.primal_wrath, options_str )
  {}

  primal_wrath_t( druid_t* p, const spell_data_t* sd, const std::string& options_str )
    : cat_attack_t( "primal_wrath", p, sd, options_str ), combo_points( 0 )
  {
    parse_options( options_str );

    special = true;
    aoe     = -1;

    // TODO: consolidate into single object instead of new rip_t for every time
    rip = new rip_t( p, "" );
    rip->stats = stats;
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t )
      t = target;
    if ( !t )
      return nullptr;

    return td( t )->dots.rip;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    double adpc = cat_attack_t::attack_direct_power_coefficient( s );

    adpc *= ( 1ll + combo_points );

    return adpc;
  }

  void impact( action_state_t* s ) override
  {
    if ( free_cast )
      rip->stats = get_free_cast_stats( free_cast );
    else
      rip->stats = orig_stats;

    cat_attack_t::impact( s );

    auto b_state    = rip->get_state();
    b_state->target = s->target;
    rip->snapshot_state( b_state, result_amount_type::DMG_OVER_TIME );
    // Copy persistent multipliers from the direct attack.
    b_state->persistent_multiplier = s->persistent_multiplier;

    if ( !td( s->target )->dots.rip->state )
      td( s->target )->dots.rip->state = rip->get_state();

    // changes stat object to primal wraths for consistency in case of a refresh
    td( s->target )->dots.rip->current_action = rip;
    td( s->target )->dots.rip->state->copy_state( b_state );
    td( s->target )->dots.rip->trigger( rip->dot_duration * 0.5 * ( combo_points + 1 ) );  // this seems to be hardcoded

    action_state_t::release( b_state );
  }

  void execute() override
  {
    if ( free_cast )
      combo_points = as<int>( p()->resources.max[ RESOURCE_COMBO_POINT ] );
    else
      combo_points = as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] );

    cat_attack_t::execute();
  }
};

// Savage Roar ==============================================================

struct savage_roar_t : public cat_attack_t
{
  savage_roar_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "savage_roar", p, p->talent.savage_roar, options_str )
  {
    may_crit = may_miss = harmful = false;
    dot_duration = base_tick_time = timespan_t::zero();
  }

  // We need a custom implementation of Pandemic refresh mechanics since our ready() method relies on having the correct
  // final value.
  timespan_t duration( int combo_points = -1 )
  {
    if ( combo_points == -1 )
      combo_points = as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] );

    timespan_t d = data().duration() * ( combo_points + 1 );

    // Maximum duration is 130% of the raw duration of the new Savage Roar.
    if ( p()->buff.savage_roar->check() )
      d += std::min( p()->buff.savage_roar->remains(), d * 0.3 );

    return d;
  }

  void execute() override
  {
    // Grab duration before we go and spend all of our combo points.
    timespan_t d = duration();

    cat_attack_t::execute();

    p()->buff.savage_roar->trigger( d );
  }

  bool ready() override
  {
    // Savage Roar may not be cast if the new duration is less than that of the current
    if ( duration() < p()->buff.savage_roar->remains() )
      return false;

    return cat_attack_t::ready();
  }
};

// Shred ====================================================================

struct shred_t : public cat_attack_t
{
  double stealth_mul;
  double stealth_cp;

  shred_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "shred", p, p->find_class_spell( "Shred" ), options_str ), stealth_mul( 0.0 ), stealth_cp( 0.0 )
  {
    if ( p->find_rank_spell( "Shred", "Rank 2" )->ok() )
      bleed_mul = data().effectN( 4 ).percent();

    if ( p->find_rank_spell( "Shred", "Rank 3" )->ok() )
      stealth_mul = data().effectN( 3 ).percent();

    stealth_cp = p->find_rank_spell( "Shred", "Rank 4" )->effectN( 1 ).base_value();
  }

  bool stealthed() const override
  {
    return p()->buff.berserk_cat->up() || p()->buff.incarnation_cat->up() || p()->buff.sudden_ambush->up() ||
           cat_attack_t::stealthed();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = cat_attack_t::composite_energize_amount( s );

    if ( stealth_cp && stealthed() )
      e += stealth_cp;

    return e;
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( hit_any_target )
      p()->buff.bt_shred->trigger();

    if ( p()->buff.shredding_fury->up() )
      p()->buff.shredding_fury->decrement();

    // TODO: Check if consumed on miss
    if ( p()->buff.sudden_ambush->up() )
      p()->buff.sudden_ambush->decrement();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = cat_attack_t::bonus_da( s );

    if ( td( s->target )->dots.thrash_cat->is_ticking() || td( s->target )->dots.thrash_bear->is_ticking() )
      b += p()->azerite.wild_fleshrending.value( 1 );

    if ( p()->buff.shredding_fury->check() )
      b += p()->buff.shredding_fury->value();

    return b;
  }

  double composite_crit_chance_multiplier() const override
  {
    double cm = cat_attack_t::composite_crit_chance_multiplier();

    if ( stealth_mul && stealthed() )
      cm *= 2.0;

    return cm;
  }

  double action_multiplier() const override
  {
    double m = cat_attack_t::action_multiplier();

    if ( stealth_mul && stealthed() )
      m *= 1.0 + stealth_mul;

    return m;
  }
};

// Swipe (Cat) ====================================================================

struct swipe_cat_t : public cat_attack_t
{
  swipe_cat_t( druid_t* player, const std::string& options_str )
    : cat_attack_t( "swipe_cat", player, player->spec.swipe_cat, options_str )
  {
    aoe               = as<int>( data().effectN( 4 ).base_value() );
    energize_amount   = data().effectN( 1 ).base_value();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type     = action_energize::ON_HIT;

    if ( player->find_rank_spell( "Swipe", "Rank 2" )->ok() )
      bleed_mul = player->spec.swipe_cat->effectN( 2 ).percent();
  }

  double cost() const override
  {
    double c = cat_attack_t::cost();

    c -= p()->buff.scent_of_blood->check_stack_value();

    return c;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = cat_attack_t::bonus_da( s );

    if ( td( s->target )->dots.thrash_cat->is_ticking() || td( s->target )->dots.thrash_bear->is_ticking() )
    {
      b += p()->azerite.wild_fleshrending.value( 2 );
    }

    return b;
  }

  bool ready() override
  {
    if ( p()->talent.brutal_slash->ok() )
      return false;

    return cat_attack_t::ready();
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( hit_any_target )
      p()->buff.bt_swipe->trigger();
  }
};

// Tiger's Fury =============================================================

struct tigers_fury_t : public cat_attack_t
{
  timespan_t duration;

  tigers_fury_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "tigers_fury", p, p->spec.tigers_fury, options_str ), duration( p->buff.tigers_fury->buff_duration() )
  {
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
    autoshift = form_mask = CAT_FORM;
    energize_type         = action_energize::ON_CAST;
    energize_amount += p->find_rank_spell( "Tiger's Fury", "Rank 2" )->effectN( 1 ).resource( RESOURCE_ENERGY );

    if ( p->talent.predator->ok() )
      duration += p->talent.predator->effectN( 1 ).time_value();

    if ( p->azerite.jungle_fury.enabled() )
    {
      /*if ( p->talent.predator->ok() )
        duration = p->azerite.jungle_fury.time_value( 1, azerite_power_t::S );
      else
        duration = p->azerite.jungle_fury.time_value( 2, azerite_power_t::S );*/
      duration += timespan_t::from_millis( 2000 );
    }
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.tigers_fury->trigger( duration );

    if ( p()->legendary.eye_of_fearful_symmetry->ok() )
      p()->buff.eye_of_fearful_symmetry->trigger();

    // p()->buff.jungle_fury->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
  }
};

// Thrash (Cat) =============================================================

struct thrash_cat_t : public cat_attack_t
{
  thrash_cat_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "thrash_cat", p, p->spec.thrash_cat, options_str )
  {
    aoe                    = -1;
    spell_power_mod.direct = 0;
    // amount_delta = 0.25;
    hasted_ticks = true;

    // For some reason this is in a different spell
    energize_amount   = p->find_spell( 211141 )->effectN( 1 ).base_value();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type     = action_energize::ON_HIT;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( p()->azerite.twisted_claws.ok() )
      p()->buff.twisted_claws->trigger();

    if ( p()->talent.scent_of_blood->ok() )
      p()->buff.scent_of_blood->trigger();
  }

  void execute() override
  {
    // Remove potential existing scent of blood buff here
    p()->buff.scent_of_blood->expire();

    cat_attack_t::execute();

    if ( hit_any_target )
      p()->buff.bt_thrash->trigger();
  }
};

}  // end namespace cat_attacks

namespace bear_attacks {

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
  bool proc_gore;

  bear_attack_t( util::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options_str = std::string() )
    : base_t( n, p, s ), proc_gore( false )
  {
    parse_options( options_str );

    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
      ap_type = attack_power_type::NO_WEAPON;
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) && proc_gore )
      trigger_gore();
  }
};  // end bear_attack_t

// Bear Melee Attack ========================================================

struct bear_melee_t : public bear_attack_t
{
  bear_melee_t( druid_t* player ) : bear_attack_t( "bear_melee", player )
  {
    form_mask  = BEAR_FORM;
    may_glance = background = repeating = is_auto_attack = true;

    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = timespan_t::zero();
    special           = false;
    weapon_multiplier = 1.0;

    energize_type     = action_energize::ON_HIT;
    energize_resource = RESOURCE_RAGE;
    energize_amount   = 4;
  }

  timespan_t execute_time() const override
  {
    if ( !player->in_combat )
      return timespan_t::from_seconds( 0.01 );

    return bear_attack_t::execute_time();
  }
};

struct berserk_bear_t : public bear_attack_t
{
  berserk_bear_t( druid_t* p, const std::string& o )
    : bear_attack_t( "berserk", p, p->find_specialization_spell( "berserk" ), o )
  {
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
  }

  void execute() override
  {
    bear_attack_t::execute();

    p()->buff.berserk_bear->trigger();
  }

  bool ready() override
  {
    if ( p()->talent.incarnation_bear->ok() )
      return false;

    return bear_attack_t::ready();
  }
};

// Growl ====================================================================

struct growl_t : public bear_attack_t
{
  growl_t( druid_t* player, const std::string& options_str )
    : bear_attack_t( "growl", player, player->find_class_spell( "Growl" ), options_str )
  {
    ignore_false_positive = use_off_gcd =true;
    may_miss = may_parry = may_dodge = may_block = may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
      target->taunt( player );

    bear_attack_t::impact( s );
  }
};

// Mangle ===================================================================

struct mangle_t : public bear_attack_t
{
  mangle_t( druid_t* player, const std::string& options_str )
    : bear_attack_t( "mangle", player, player->find_class_spell( "Mangle" ), options_str )
  {
    if ( p()->find_rank_spell( "Mangle", "Rank 2" )->ok() )
      bleed_mul = data().effectN( 3 ).percent();

    energize_amount += player->talent.soul_of_the_forest_bear->effectN( 1 ).resource( RESOURCE_RAGE );
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double em = bear_attack_t::composite_energize_amount( s );

    em += p()->buff.gore->value();

    return em;
  }

  int n_targets() const override
  {
    if ( p()->buff.incarnation_bear->check() )
      return as<int>( p()->talent.incarnation_bear->effectN( 4 ).base_value() );

    return bear_attack_t::n_targets();
  }

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->buff.guardian_of_elune->trigger();

      if ( p()->conduit.savage_combatant->ok() )
        p()->buff.savage_combatant->trigger();
    }
  }

  void execute() override
  {
    bear_attack_t::execute();

    if ( p()->buff.gore->up() )
    {
      p()->buff.gore->expire();

      if ( p()->azerite.burst_of_savagery.ok() )
      {
        p()->buff.burst_of_savagery->trigger();
        trigger_gore();
      }
    }
  }
};

// Maul =====================================================================

struct maul_t : public bear_attack_t
{
  maul_t( druid_t* player, const std::string& options_str )
    : bear_attack_t( "maul", player, player->find_specialization_spell( "Maul" ), options_str )
  {
    proc_gore = true;
  }

  void execute() override
  {
    bear_attack_t::execute();

    if ( p()->buff.savage_combatant->up() )
      p()->buff.savage_combatant->decrement();
  }

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p()->azerite.guardians_wrath.ok() )
      {
        p()->buff.guardians_wrath->up();  // benefit tracking
        p()->buff.guardians_wrath->trigger();
      }

      if ( p()->azerite.conflict_and_strife.is_major() && p()->talent.sharpened_claws->ok() )
      {
        p()->buff.sharpened_claws->up();  // benefit tracking
        p()->buff.sharpened_claws->trigger();
      }
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = bear_attack_t::composite_da_multiplier( s );

    if ( p()->buff.savage_combatant->check() )
      da *= 1.0 + p()->buff.savage_combatant->stack_value();

    return da;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = bear_attack_t::bonus_da( s );

    if ( p()->azerite.guardians_wrath.ok() )
      da += p()->azerite.guardians_wrath.value( 2 );

    return da;
  }
};

// Pulverize ================================================================

struct pulverize_t : public bear_attack_t
{
  pulverize_t( druid_t* player, const std::string& options_str )
    : bear_attack_t( "pulverize", player, player->talent.pulverize, options_str ) {}

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // consumes x stacks of Thrash on the target
      td( s->target )->dots.thrash_bear->decrement( (int)data().effectN( 3 ).base_value() );

      // and reduce damage taken by x% for y sec.
      p()->buff.pulverize->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Call bear_attack_t::ready() first for proper targeting support.
    // Hardcode stack max since it may not be set if this code runs before Thrash is cast.
    return bear_attack_t::target_ready( candidate_target ) &&
           td( candidate_target )->dots.thrash_bear->current_stack() >= data().effectN( 3 ).base_value();
  }
};

// Swipe (Bear) =============================================================

struct swipe_bear_t : public bear_attack_t
{
  swipe_bear_t( druid_t* p, const std::string& options_str )
    : bear_attack_t( "swipe_bear", p, p->spec.swipe_bear, options_str )
  {
    // target hit data stored in spec.swipe_cat
    aoe  = as<int>( p->spec.swipe_cat->effectN( 4 ).base_value() );
    proc_gore = true;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = bear_attack_t::bonus_da( s );

    if ( td( s->target )->dots.thrash_cat->is_ticking() || td( s->target )->dots.thrash_bear->is_ticking() )
      b += p()->azerite.wild_fleshrending.value( 2 );

    return b;
  }
};

// Thrash (Bear) ============================================================

struct thrash_bear_t : public bear_attack_t
{
  double blood_frenzy_amount;

  thrash_bear_t( druid_t* p, const std::string& options_str )
    : bear_attack_t( "thrash_bear", p, p->spec.thrash_bear, options_str ), blood_frenzy_amount( 0 )
  {
    aoe  = -1;
    proc_gore = hasted_ticks = true;
    dot_duration             = p->spec.thrash_bear_dot->duration();
    base_tick_time           = p->spec.thrash_bear_dot->effectN( 1 ).period();
    spell_power_mod.direct   = 0;
    attack_power_mod.tick    = p->spec.thrash_bear_dot->effectN( 1 ).ap_coeff();
    dot_max_stack =
        p->spec.thrash_bear_dot->max_stacks() + as<int>( p->legendary.luffainfused_embrace->effectN( 4 ).base_value() );
    radius *= 1.0 + p->legendary.luffainfused_embrace->effectN( 3 ).percent();

    // Bear Form cost modifier
    base_costs[ RESOURCE_RAGE ] *= 1.0 + p->buff.bear_form->data().effectN( 7 ).percent();

    if ( p->talent.blood_frenzy->ok() )
      blood_frenzy_amount = p->find_spell( 203961 )->effectN( 1 ).resource( RESOURCE_RAGE );
  }

  void tick( dot_t* d ) override
  {
    bear_attack_t::tick( d );

    p()->resource_gain( RESOURCE_RAGE, blood_frenzy_amount, p()->gain.blood_frenzy );
  }

  void execute() override
  {
    bear_attack_t::execute();

    if ( p()->legendary.ursocs_lingering_spirit->ok() &&
         rng().roll( p()->legendary.ursocs_lingering_spirit->effectN( 1 ).percent() ) )
    {
      schedule_execute();
    }
  }

  void impact( action_state_t* state ) override
  {
    bear_attack_t::impact( state );

    if ( p()->azerite.twisted_claws.ok() )
      p()->buff.twisted_claws->trigger();
  }
};

} // end namespace bear_attacks

namespace heals {

// ==========================================================================
// Druid Heal
// ==========================================================================

// Cenarion Ward ============================================================

struct cenarion_ward_hot_t : public druid_heal_t
{
  cenarion_ward_hot_t( druid_t* p ) : druid_heal_t( "cenarion_ward_hot", p, p->find_spell( 102352 ) )
  {
    target     = p;
    background = proc = true;
    harmful = hasted_ticks = false;
  }

  void execute() override
  {
    heal_t::execute();

    p()->buff.cenarion_ward->expire();
  }
};

struct cenarion_ward_t : public druid_heal_t
{
  cenarion_ward_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "cenarion_ward", p, p->talent.cenarion_ward, options_str ) {}
  
  void execute() override
  {
    druid_heal_t::execute();

    p()->buff.cenarion_ward->trigger();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target != p() )
      return false;
  
    return druid_heal_t::target_ready( candidate_target );
  }
};

// Frenzied Regeneration ====================================================

struct frenzied_regeneration_t : public heals::druid_heal_t
{
  frenzied_regeneration_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "frenzied_regeneration", p, p->find_affinity_spell( "Frenzied Regeneration" ), options_str )
  {
    /* use_off_gcd = quiet = true; */
    target           = p;
    cooldown->hasted = true;
    tick_zero        = true;
    may_crit = tick_may_crit = hasted_ticks = false;

    cooldown->charges += as<int>( p->find_rank_spell( "Frenzied Regeneration", "Rank 2" )->effectN( 1 ).base_value() );
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags = STATE_MUL_TA | STATE_VERSATILITY | STATE_MUL_PERSISTENT | STATE_TGT_MUL_TA;
  }

  void execute() override
  {
    druid_heal_t::execute();

    if ( p()->buff.guardian_of_elune->up() )
      p()->buff.guardian_of_elune->expire();
  }

  double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    am *= 1.0 + p()->buff.guardian_of_elune->check() * p()->buff.guardian_of_elune->data().effectN( 2 ).percent();

    return am;
  }
};

// Lifebloom ================================================================

struct lifebloom_bloom_t : public druid_heal_t
{
  lifebloom_bloom_t( druid_t* p ) : druid_heal_t( "lifebloom_bloom", p, p->find_class_spell( "Lifebloom" ) )
  {
    background = dual = true;
    dot_duration      = timespan_t::zero();
    base_td = attack_power_mod.tick = 0;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= td( target )->buff.lifebloom->check();

    return ctm;
  }
};

struct lifebloom_t : public druid_heal_t
{
  lifebloom_bloom_t* bloom;

  lifebloom_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "lifebloom", p, p->find_class_spell( "Lifebloom" ), options_str ),
      bloom( new lifebloom_bloom_t( p ) )
  {
    may_crit              = false;
    ignore_false_positive = true;
  }

  void impact( action_state_t* state ) override
  {
    // Cancel Dot/td-buff on all targets other than the one we impact on
    for ( size_t i = 0; i < sim->actor_list.size(); ++i )
    {
      player_t* t = sim->actor_list[ i ];

      if ( state->target == t )
        continue;

      get_dot( t )->cancel();
      td( t )->buff.lifebloom->expire();
    }

    druid_heal_t::impact( state );

    if ( result_is_hit( state->result ) )
      td( state->target )->buff.lifebloom->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    if ( !d->state->target->is_sleeping() )  // Prevent crash at end of simulation
      bloom->execute();

    td( d->state->target )->buff.lifebloom->expire();

    druid_heal_t::last_tick( d );
  }
};

// Nature's Guardian ========================================================

struct natures_guardian_t : public druid_heal_t
{
  natures_guardian_t( druid_t* p ) : druid_heal_t( "natures_guardian", p, p->find_spell( 227034 ) )
  {
    background = true;
    may_crit   = false;
    target     = p;
  }

  void init() override
  {
    druid_heal_t::init();

    // Not affected by multipliers of any sort.
    snapshot_flags &= ~STATE_NO_MULTIPLIER;
  }
};

// Regrowth =================================================================

struct regrowth_t : public druid_heal_t
{
  regrowth_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "regrowth", p, p->find_class_spell( "Regrowth" ), options_str )
  {
    form_mask             = NO_FORM | MOONKIN_FORM;
    may_autounshift       = true;
    ignore_false_positive = true;  // Prevents cat/bear from failing a skill check and going into caster form.

    // Hack for feral to be able to use target-related expressions on this action
    // Disables healing entirely
    if ( p->specialization() == DRUID_FERAL )
    {
      target          = sim->target;
      base_multiplier = 0;
    }
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_heal_t::gcd();

    if ( p()->buff.cat_form->check() )
      g -= p()->query_aura_effect( p()->spec.cat_form, A_ADD_FLAT_MODIFIER, P_GCD, &data() )->time_value();

    return g;
  }

  void consume_resource() override
  {
    // Prevent from consuming Omen of Clarity unnecessarily
    if ( p()->buff.predatory_swiftness->check() )
      return;

    druid_heal_t::consume_resource();
  }

  double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    return am;
  }

  bool check_form_restriction() override
  {
    if ( p()->buff.predatory_swiftness->check() )
      return true;

    return druid_heal_t::check_form_restriction();
  }

  void execute() override
  {
    druid_heal_t::execute();

    p()->buff.predatory_swiftness->decrement( 1 );
  }
};

// Rejuvenation =============================================================

struct rejuvenation_t : public druid_heal_t
{
  rejuvenation_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "rejuvenation", p, p->find_class_spell( "Rejuvenation" ), options_str )
  {
    tick_zero = true;
  }
};

// Renewal ==================================================================

struct renewal_t : public druid_heal_t
{
  renewal_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "renewal", p, p->find_talent_spell( "Renewal" ), options_str )
  {
    may_crit = false;
  }

  void execute() override
  {
    base_dd_min = p()->resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    druid_heal_t::execute();
  }
};

// Swiftmend ================================================================

// TODO: in game, you can swiftmend other druids' hots, which is not supported here
struct swiftmend_t : public druid_heal_t
{
  swiftmend_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "swiftmend", p, p->find_class_spell( "Swiftmend" ), options_str ) {}

  void impact( action_state_t* state ) override
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state->result ) )
    {
      if ( p()->talent.soul_of_the_forest_tree->ok() )
        p()->buff.soul_of_the_forest->trigger();
    }
  }
};

// Tranquility ==============================================================

struct tranquility_t : public druid_heal_t
{
  tranquility_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "tranquility", p, p->find_specialization_spell( "Tranquility" ), options_str )
  {
    aoe               = -1;
    base_execute_time = data().duration();
    channeled         = true;

    // Healing is in spell effect 1
    parse_spell_data( ( *player->dbc->spell( data().effectN( 1 ).trigger_spell_id() ) ) );
  }
};

// Wild Growth ==============================================================

struct wild_growth_t : public druid_heal_t
{
  wild_growth_t( druid_t* p, const std::string& options_str )
    : druid_heal_t( "wild_growth", p, p->find_class_spell( "Wild Growth" ), options_str )
  {
    ignore_false_positive = true;
  }

  int n_targets() const override
  {
    int n = druid_heal_t::n_targets();

    if ( p()->buff.incarnation_tree->check() )
      n += 2;

    return n;
  }
};

// Ysera's Gift =============================================================

struct yseras_gift_t : public druid_heal_t
{
  yseras_gift_t( druid_t* p ) : druid_heal_t( "yseras_gift", p, p->find_spell( 145110 ) )
  {
    may_crit   = false;
    background = dual = true;
    base_pct_heal     = p->spec.yseras_gift->effectN( 1 ).percent();
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_VERSATILITY;  // Is not affected by versatility.
  }

  double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    if ( p()->buff.bear_form->check() )
      am *= 1.0 + p()->buff.bear_form->data().effectN( 8 ).percent();

    return am;
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

}  // end namespace heals

namespace spells
{
// ==========================================================================
// Druid Spells
// ==========================================================================

// Auto Attack ==============================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( druid_t* player, const std::string& options_str )
    : melee_attack_t( "auto_attack", player, spell_data_t::nil() )
  {
    parse_options( options_str );

    trigger_gcd           = timespan_t::zero();
    ignore_false_positive = true;
    use_off_gcd           = true;
  }

  void execute() override
  {
    player->main_hand_attack->weapon            = &( player->main_hand_weapon );
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

// Barkskin =================================================================

struct barkskin_t : public druid_spell_t
{
  barkskin_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "barkskin", player, player->find_specialization_spell( "Barkskin" ), options_str )
  {
    harmful     = false;
    use_off_gcd = true;

    cooldown->duration *= 1.0 + player->talent.survival_of_the_fittest->effectN( 1 ).percent();
    dot_duration = timespan_t::zero();

    if ( player->talent.brambles->ok() )
      add_child( player->active.brambles_pulse );
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.barkskin->trigger();
  }
};

// Bear Form Spell ==========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "bear_form", player, player->find_class_spell( "Bear Form" ), options_str )
  {
    form_mask       = NO_FORM | CAT_FORM | MOONKIN_FORM;
    may_autounshift = false;

    harmful               = false;
    min_gcd               = timespan_t::from_seconds( 1.5 );
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->shapeshift( BEAR_FORM );

    if ( p()->legendary.oath_of_the_elder_druid->ok() && !p()->buff.oath_of_the_elder_druid->check() &&
         p()->talent.guardian_affinity->ok() )
    {
      p()->buff.heart_of_the_wild->trigger(
          1, buff_t::DEFAULT_VALUE(), 1.0,
          timespan_t::from_seconds( p()->legendary.oath_of_the_elder_druid->effectN( 2 ).base_value() ) );
    }
  }
};
// Brambles =================================================================

struct brambles_t : public druid_spell_t
{
  brambles_t( druid_t* p ) : druid_spell_t( "brambles_reflect", p, p->find_spell( 203958 ) )
  {
    // Ensure reflect scales with damage multipliers
    snapshot_flags |= STATE_VERSATILITY | STATE_TGT_MUL_DA | STATE_MUL_DA;
    background = may_crit = proc = may_miss = true;
  }
};

struct brambles_pulse_t : public druid_spell_t
{
  brambles_pulse_t( druid_t* p ) : druid_spell_t( "brambles_pulse", p, p->find_spell( 213709 ) )
  {
    background = dual = true;
    aoe               = -1;
  }
};

// Bristling Fur Spell ======================================================

struct bristling_fur_t : public druid_spell_t
{
  bristling_fur_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "bristling_fur", player, player->talent.bristling_fur, options_str )
  {
    harmful     = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.bristling_fur->trigger();
  }
};

// Cat Form Spell ===========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "cat_form", player, player->find_class_spell( "Cat Form" ), options_str )
  {
    form_mask       = NO_FORM | BEAR_FORM | MOONKIN_FORM;
    may_autounshift = false;

    harmful               = false;
    min_gcd               = timespan_t::from_seconds( 1.5 );
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->shapeshift( CAT_FORM );

    if ( p()->legendary.oath_of_the_elder_druid->ok() && !p()->buff.oath_of_the_elder_druid->check() &&
         p()->talent.feral_affinity->ok() )
    {
      p()->buff.heart_of_the_wild->trigger(
          1, buff_t::DEFAULT_VALUE(), 1.0,
          timespan_t::from_seconds( p()->legendary.oath_of_the_elder_druid->effectN( 2 ).base_value() ) );
    }
  }
};

// Celestial Alignment ======================================================

struct celestial_alignment_t : public druid_spell_t
{
  celestial_alignment_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "celestial_alignment", player, player->spec.celestial_alignment, options_str )
  {
    harmful = false;
    cooldown->duration *= 1.0 + player->vision_of_perfection_cdr;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.celestial_alignment->trigger();
    p()->uptime.arcanic_pulsar->update( false, sim->current_time() );
    p()->uptime.vision_of_perfection->update( false, sim->current_time() );

    // Trigger after triggering the buff so the cast procs the spell
    streaking_stars_trigger( SS_CELESTIAL_ALIGNMENT, nullptr );
  }

  bool ready() override
  {
    if ( p()->talent.incarnation_moonkin->ok() )
      return false;

    return druid_spell_t::ready();
  }
};

// Streaking Stars =======================================================

struct streaking_stars_t : public druid_spell_t
{
  streaking_stars_t( druid_t* p ) : druid_spell_t( "streaking_star", p, p->find_spell( 272873 ) )
  {
    background  = true;
    base_dd_min = base_dd_max = p->azerite.streaking_stars.value( 1 );

    if ( p->talent.incarnation_moonkin->ok() )
    {
      // This spell deals less damage when incarnation is talented which is not found in the spelldata 8/10/18
      base_dd_min *= 0.6667;
      base_dd_max *= 0.6667;
    }
  }
};

struct kindred_empowerment_t : public druid_spell_t
{
  kindred_empowerment_t( druid_t* p, util::string_view n )
    : druid_spell_t( n, p, p->covenant.kindred_empowerment_damage )
  {
    background = true;
    may_miss = may_crit = callbacks = false;
    snapshot_flags |= STATE_TGT_MUL_DA;
    update_flags |= STATE_TGT_MUL_DA;
  }
};

// Fury of Elune =========================================================

struct fury_of_elune_t : public druid_spell_t
{
  struct fury_of_elune_tick_t : public druid_spell_t
  {
    fury_of_elune_tick_t( druid_t* p ) : druid_spell_t( "fury_of_elune_tick", p, p->spec.fury_of_elune )
    {
      background = dual = ground_aoe = reduced_aoe_damage = true;
      aoe = -1;
    }
  };

  action_t* damage;

  fury_of_elune_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "fury_of_elune", p, p->talent.fury_of_elune, options_str )
  {
    damage = p->get_secondary_action<fury_of_elune_tick_t>( "fury_of_elune_tick" );
    damage->stats = stats;
  }

  void execute() override
  {
    druid_spell_t::execute();

    streaking_stars_trigger( SS_FURY_OF_ELUNE, nullptr );

    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .hasted( ground_aoe_params_t::hasted_with::SPELL_HASTE )
                                        .pulse_time( data().effectN( 3 ).period() )
                                        .duration( data().duration() )
                                        .action( damage ) );
  }
};

// Dash =====================================================================

struct dash_t : public druid_spell_t
{
  dash_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "dash", player,
                     player->talent.tiger_dash->ok() ? player->talent.tiger_dash : player->find_class_spell( "Dash" ),
                     options_str )
  {
    autoshift = form_mask = CAT_FORM;

    harmful = may_crit = may_miss = false;
    ignore_false_positive         = true;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p()->buff.cat_form->check() )
      g *= 1.0 + p()->query_aura_effect( &p()->buff.cat_form->data(), A_ADD_PCT_MODIFIER, P_GCD, &data() )->percent();

    return g;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p()->talent.tiger_dash->ok() )
      p()->buff.tiger_dash->trigger();
    else
      p()->buff.dash->trigger();
  }
};

// Tiger Dash =====================================================================

struct tiger_dash_t : public druid_spell_t
{
  tiger_dash_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "tiger_dash", player, player->talent.tiger_dash, options_str )
  {
    autoshift = form_mask = CAT_FORM;

    harmful = may_crit = may_miss = false;
    ignore_false_positive         = true;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p()->buff.cat_form->check() )
      g *= 1.0 + p()->query_aura_effect( &p()->buff.cat_form->data(), A_ADD_PCT_MODIFIER, P_GCD, &data() )->percent();

    return g;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.tiger_dash->trigger();
  }
};

// Full Moon Spell ==========================================================

struct full_moon_t : public druid_spell_t
{
  cooldown_t* orig_cd;

  full_moon_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "full_moon", p, p->spec.full_moon, options_str )
  {
    aoe                = -1;
    reduced_aoe_damage = true;
    cooldown = orig_cd = p->cooldown.moon_cd;

    if ( !p->spec.astral_power->ok() )
      energize_type = action_energize::NONE;
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    streaking_stars_trigger( SS_FULL_MOON, s );  // proc munching shenanigans, munch tracking NYI
  }

  void execute() override
  {
    if ( free_cast )
    {
      cooldown = nullptr;
      druid_spell_t::execute();
      cooldown = orig_cd;
    }
    else
    {
      druid_spell_t::execute();
      p()->moon_stage = moon_stage_e::NEW_MOON;
    }
  }

  timespan_t travel_time() const override
  {
    // has a set travel time since it spawns on the target
    return timespan_t::from_seconds( data().missile_speed() );
  }

  bool ready() override
  {
    if ( !p()->talent.new_moon || p()->moon_stage != FULL_MOON || !p()->cooldown.moon_cd->up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Half Moon Spell ==========================================================

struct half_moon_t : public druid_spell_t
{
  half_moon_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "half_moon", player, player->spec.half_moon, options_str )
  {
    cooldown = player->cooldown.moon_cd;
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );
    streaking_stars_trigger( SS_HALF_MOON, s );  // proc munching shenanigans, munch tracking NYI
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->moon_stage++;
  }

  bool ready() override
  {
    if ( !p()->talent.new_moon || p()->moon_stage != HALF_MOON || !p()->cooldown.moon_cd->up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Thrash ===================================================================

struct thrash_proxy_t : public druid_spell_t
{
  action_t* thrash_cat;
  action_t* thrash_bear;

  thrash_proxy_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "thrash", p, p->find_specialization_spell( "Thrash" ), options_str )
  {
    thrash_cat  = new cat_attacks::thrash_cat_t( p, options_str );
    thrash_bear = new bear_attacks::thrash_bear_t( p, options_str );

    // At the moment, both versions of these spells are the same (and only the cat version has a talent that changes
    // this) so we use this spell data here. If this changes we will have to think of something more robust. These two
    // are important for the "ticks_gained_on_refresh" expression to work
    dot_duration   = thrash_cat->dot_duration;
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

struct swipe_proxy_t : public druid_spell_t
{
  action_t* swipe_cat;
  action_t* swipe_bear;

  swipe_proxy_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "swipe", p, p->find_class_spell( "Swipe" ), options_str )
  {
    swipe_cat  = new cat_attacks::swipe_cat_t( p, options_str );
    swipe_bear = new bear_attacks::swipe_bear_t( p, options_str );
  }

  void execute() override
  {
    if ( p()->buff.cat_form->check() )
      swipe_cat->execute();
    else if ( p()->buff.bear_form->check() )
      swipe_bear->execute();
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

// Incarnation ==============================================================

struct incarnation_t : public druid_spell_t
{
  buff_t* spec_buff;

  incarnation_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "incarnation", p,
      p->specialization() == DRUID_BALANCE ? p->talent.incarnation_moonkin :
      p->specialization() == DRUID_FERAL ? p->talent.incarnation_cat :
      p->specialization() == DRUID_GUARDIAN ? p->talent.incarnation_bear :
      p->specialization() == DRUID_RESTORATION ? p->talent.incarnation_tree :
      spell_data_t::nil(), options_str )
  {
    switch ( p->specialization() )
    {
      case DRUID_BALANCE:
        spec_buff = p->buff.incarnation_moonkin;
        cooldown->duration *= 1.0 + p->vision_of_perfection_cdr;
        break;
      case DRUID_FERAL:
        spec_buff = p->buff.incarnation_cat;
        cooldown->duration *= 1.0 + p->vision_of_perfection_cdr;
        break;
      case DRUID_GUARDIAN:
        spec_buff = p->buff.incarnation_bear;
        break;
      case DRUID_RESTORATION:
        spec_buff = p->buff.incarnation_tree;
        break;
      default:
        assert( false && "Actor attempted to create incarnation action with no specialization." );
    }

    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    spec_buff->trigger();

    if ( p()->buff.incarnation_cat->check() )
      p()->buff.jungle_stalker->trigger();

    if ( p()->buff.incarnation_moonkin->check() )
    {
      p()->uptime.arcanic_pulsar->update( false, sim->current_time() );
      p()->uptime.vision_of_perfection->update( false, sim->current_time() );

      streaking_stars_trigger( SS_CELESTIAL_ALIGNMENT, nullptr );
    }
  }
};

struct heart_of_the_wild_t : public druid_spell_t
{
  heart_of_the_wild_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "heart_of_the_wild", p, p->talent.heart_of_the_wild, options_str )
  {
    harmful = may_crit = may_miss = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.heart_of_the_wild->trigger();
  }
};

// Entangling Roots =========================================================
struct entangling_roots_t : public druid_spell_t
{
  entangling_roots_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "entangling_roots", p, p->spec.entangling_roots, options_str )
  {
    form_mask = NO_FORM | MOONKIN_FORM;
    harmful   = false;
    // workaround so that we do not need to enable mana regen
    base_costs[ RESOURCE_MANA ] = 0.0;
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

    if ( p()->buff.strife_doubled )  // Check for essence happens in arise(). If null here means we don't have it.
      p()->buff.strife_doubled->trigger();
  }
};

// Innervate ================================================================
struct innervate_t : public druid_spell_t
{
  innervate_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "innervate", p, p->spec.innervate, options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.innervate->trigger();
  }
};

// Ironfur ==================================================================

struct ironfur_t : public druid_spell_t
{
  ironfur_t( druid_t* p, const std::string& options_str ) : druid_spell_t( "ironfur", p, p->spec.ironfur, options_str )
  {
    use_off_gcd = true;
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
  }

  timespan_t composite_buff_duration()
  {
    timespan_t bd = p()->buff.ironfur->buff_duration();

    bd += timespan_t::from_seconds( p()->buff.guardian_of_elune->value() );

    return bd;
  }

  double cost() const override
  {
    double c = druid_spell_t::cost();

    c += p()->buff.guardians_wrath->check_stack_value();

    return c;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.ironfur->trigger( composite_buff_duration() );

    if ( p()->buff.guardian_of_elune->up() )
      p()->buff.guardian_of_elune->expire();

    if ( p()->buff.guardians_wrath->up() )
      p()->buff.guardians_wrath->expire();

    if ( p()->azerite.layered_mane.ok() && rng().roll( p()->azerite.layered_mane.spell()->effectN( 2 ).percent() ) )
      p()->buff.ironfur->trigger( composite_buff_duration() );
  }
};

// Lunar Beam ===============================================================

struct lunar_beam_t : public druid_spell_t
{
  struct lunar_beam_heal_t : public heals::druid_heal_t
  {
    lunar_beam_heal_t( druid_t* p ) : heals::druid_heal_t( "lunar_beam_heal", p, p->find_spell( 204069 ) )
    {
      target                  = p;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
      background              = true;
    }
  };

  struct lunar_beam_damage_t : public druid_spell_t
  {
    action_t* heal;

    lunar_beam_damage_t( druid_t* p ) : druid_spell_t( "lunar_beam_damage", p, p->find_spell( 204069 ) )
    {
      aoe  = -1;
      dual = background = ground_aoe = true;
      attack_power_mod.direct        = data().effectN( 2 ).ap_coeff();

      heal = p->get_secondary_action<lunar_beam_heal_t>( "lunar_beam_heal" );
    }

    void execute() override
    {
      druid_spell_t::execute();

      heal->schedule_execute();
    }
  };

  action_t* damage;

  lunar_beam_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "lunar_beam", p, p->talent.lunar_beam, options_str )
  {
    may_crit = tick_may_crit = may_miss = hasted_ticks = false;
    base_tick_time = timespan_t::from_seconds( 1.0 );  // TODO: Find in spell data... somewhere!
    dot_duration   = timespan_t::zero();

    damage = p->get_secondary_action<lunar_beam_damage_t>( "lunar_beam_damage" );
    damage->stats = stats;
  }

  void execute() override
  {
    druid_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( base_tick_time )
                                        .duration( data().duration() )
                                        .start_time( sim->current_time() )
                                        .action( damage ),
                                    false );
  }
};

// Starfire =============================================================

struct starfire_t : public druid_spell_t
{
  starfire_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "starfire", player, player->find_affinity_spell( "Starfire" ), options_str )
  {
    aoe = -1;

    if ( player->specialization() == DRUID_BALANCE )
      base_aoe_multiplier = data().effectN( 3 ).percent();
    else
      base_aoe_multiplier = data().effectN( 2 ).percent();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = druid_spell_t::composite_energize_amount( s );

    if ( energize_resource_() == RESOURCE_ASTRAL_POWER && p()->buff.warrior_of_elune->check() )
      e *= 1.0 + p()->talent.warrior_of_elune->effectN( 2 ).percent();

    return e;
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );
    streaking_stars_trigger( SS_STARFIRE, s );
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p()->buff.owlkin_frenzy->up() )
      p()->buff.owlkin_frenzy->decrement();
    else if ( p()->buff.warrior_of_elune->up() )
      p()->buff.warrior_of_elune->decrement();

    p()->eclipse_handler.cast_starfire();

    if ( p()->azerite.dawning_sun.ok() )
      p()->buff.dawning_sun->trigger( 1, p()->azerite.dawning_sun.value() );
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = druid_spell_t::composite_aoe_multiplier( state );

    if ( state->target != target && p()->buff.eclipse_lunar->up() )
      cam *= 1.0 + p()->buff.eclipse_lunar->value();

    return cam;
  }
};

// New Moon Spell ===========================================================

struct new_moon_t : public druid_spell_t
{
  new_moon_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "new_moon", player, player->talent.new_moon, options_str )
  {
    cooldown           = player->cooldown.moon_cd;
    cooldown->duration = data().charge_cooldown();
    cooldown->charges  = data().charges();
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    streaking_stars_trigger( SS_NEW_MOON, s );  // proc munching shenanigans, munch tracking NYI
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->moon_stage++;
  }

  bool ready() override
  {
    if ( !p()->talent.new_moon || p()->moon_stage != NEW_MOON || !p()->cooldown.moon_cd->up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Sunfire Spell ============================================================

struct sunfire_t : public druid_spell_t
{
  struct sunfire_damage_t : public druid_spell_t
  {
    int sunfire_action_id;

    sunfire_damage_t( druid_t* p ) : druid_spell_t( "sunfire_dmg", p, p->spec.sunfire_dmg ), sunfire_action_id( 0 )
    {
      if ( p->spec.shooting_stars->ok() && !p->active.shooting_stars )
        p->active.shooting_stars = new shooting_stars_t( p );

      dual = background   = true;
      aoe                 = p->find_rank_spell( "Sunfire", "Rank 2" )->ok() ? -1 : 0;
      base_aoe_multiplier = 0;

      if ( p->azerite.high_noon.ok() )
        radius += p->azerite.high_noon.value();
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( !t )
        t = target;
      if ( !t )
        return nullptr;

      return td( t )->dots.sunfire;
    }

    void tick( dot_t* d ) override
    {
      druid_spell_t::tick( d );

      proc_shooting_stars( d->target );
    }

    double bonus_ta( const action_state_t* s ) const override
    {
      double ta = druid_spell_t::bonus_ta( s );

      ta += p()->azerite.high_noon.value( 2 );

      return ta;
    }
  };

  action_t* damage;  // Add damage modifiers in sunfire_damage_t, not sunfire_t

  sunfire_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "sunfire", p, p->find_affinity_spell( "Sunfire" ), options_str )
  {
    may_miss = may_crit = false;
    damage = p->get_secondary_action<sunfire_damage_t>( "sunfire_dmg" );
    damage->stats = stats;

    if ( p->spec.astral_power->ok() )
    {
      energize_resource = RESOURCE_ASTRAL_POWER;
      energize_amount   = p->spec.astral_power->effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
    }
    else
      energize_type = action_energize::NONE;

  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    streaking_stars_trigger( SS_SUNFIRE, s );
  }

  void execute() override
  {
    druid_spell_t::execute();

    damage->target = execute_state->target;
    damage->schedule_execute();
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "moonkin_form", player, player->spec.moonkin_form, options_str )
  {
    form_mask       = NO_FORM | CAT_FORM | BEAR_FORM;
    may_autounshift = false;

    harmful               = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->shapeshift( MOONKIN_FORM );

    if ( p()->legendary.oath_of_the_elder_druid->ok() && !p()->buff.oath_of_the_elder_druid->check() &&
         p()->talent.balance_affinity->ok() )
    {
      p()->buff.heart_of_the_wild->trigger(
          1, buff_t::DEFAULT_VALUE(), 1.0,
          timespan_t::from_seconds( p()->legendary.oath_of_the_elder_druid->effectN( 2 ).base_value() ) );
    }
  }
};

// Prowl ====================================================================

struct prowl_t : public druid_spell_t
{
  prowl_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "prowl", player, player->find_class_spell( "Prowl" ), options_str )
  {
    autoshift = form_mask = CAT_FORM;

    trigger_gcd           = timespan_t::zero();
    use_off_gcd           = true;
    harmful               = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    if ( sim->log )
      sim->print_log( "{} performs {}", player->name(), name() );

    p()->buff.jungle_stalker->expire();
    p()->buff.prowl->trigger();
  }

  bool ready() override
  {
    if ( p()->buff.prowl->check() )
      return false;

    if ( p()->in_combat && !( p()->specialization() == DRUID_FERAL && p()->buff.jungle_stalker->check() ) )
      return false;

    return druid_spell_t::ready();
  }
};

// Skull Bash ===============================================================

struct skull_bash_t : public druid_spell_t
{
  skull_bash_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "skull_bash", player, player->find_specialization_spell( "Skull Bash" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = use_off_gcd = true;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return druid_spell_t::target_ready( candidate_target );
  }
};

struct wrath_t : public druid_spell_t
{
  unsigned count;
  double gcd_mul;

  wrath_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "wrath", p, p->find_affinity_spell( "Wrath" ), options_str )
  {
    form_mask       = NO_FORM | MOONKIN_FORM;
    energize_amount = p->spec.astral_power->effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
    gcd_mul = p->query_aura_effect( &p->buff.eclipse_solar->data(), A_ADD_PCT_MODIFIER, P_GCD, &data() )->percent();
  }

  void init_finished() override
  {
    druid_spell_t::init_finished();

    if ( p()->specialization() == DRUID_BALANCE && action_list && action_list->name_str == "precombat" )
    {
      auto apl = player->precombat_action_list;

      auto it = range::find( apl, this );
      if ( it != apl.end() )
      {
        std::for_each( it + 1, apl.end(), [this]( action_t* a ) {
          if ( harmful && a->harmful && a->action_ready() )
            harmful = false;  // another harmful action exists; set current to non-harmful so we can keep casting

          if ( a->name_str == name_str )
            count++;  // see how many wrath casts are left, so we can adjust travel time when combat begins
        } );
      }
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = druid_spell_t::composite_energize_amount( s );

    if ( energize_resource_() == RESOURCE_ASTRAL_POWER && p()->talent.soul_of_the_forest_moonkin->ok() &&
         p()->buff.eclipse_solar->check() )
    {
      e *= 1.0 + p()->talent.soul_of_the_forest_moonkin->effectN( 1 ).percent();
    }

    return e;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = druid_spell_t::composite_target_da_multiplier( t );

    if ( p()->buff.eclipse_solar->up() )
      tdm *= 1.0 + p()->buff.eclipse_solar->value();

    return tdm;
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

  void execute() override
  {
    druid_spell_t::execute();

    streaking_stars_trigger( SS_WRATH, execute_state );

    if ( !free_cast )
      p()->eclipse_handler.cast_wrath();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = druid_spell_t::bonus_da( s );

    da += p()->buff.dawning_sun->value();

    return da;
  }
};

// Stampeding Roar ==========================================================

struct stampeding_roar_t : public druid_spell_t
{
  stampeding_roar_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "stampeding_roar", p, p->find_class_spell( "Stampeding Roar" ), options_str )
  {
    form_mask = BEAR_FORM | CAT_FORM;
    autoshift = BEAR_FORM;

    harmful = false;

    cooldown->duration -= p->find_rank_spell( "Stampeding Roar", "Rank 2" )->effectN( 1 ).time_value();
  }

  void execute() override
  {
    druid_spell_t::execute();

    for ( size_t i = 0; i < sim->player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim->player_non_sleeping_list[ i ];

      if ( p->type == PLAYER_GUARDIAN )
        continue;

      p->buffs.stampeding_roar->trigger();
    }
  }
};

// Starfall Spell ===========================================================

struct lunar_shrapnel_t : public druid_spell_t
{
  lunar_shrapnel_t( druid_t* p ) : druid_spell_t( "lunar_shrapnel", p, p->find_spell( 279641 ) )
  {
    background  = true;
    aoe         = -1;
    base_dd_min = base_dd_max = p->azerite.lunar_shrapnel.value( 1 );
  }
};

struct starfall_tick_t : public druid_spell_t
{
  starfall_tick_t( druid_t* p ) : druid_spell_t( "starfall_tick", p, p->spec.starfall_dmg )
  {
    background = dual = direct_tick = true;
    aoe    = -1;
    // TODO: pull hardcoded id out of here and into spec.X
    radius = p->find_spell( 50286 )->effectN( 1 ).radius();
  }

  timespan_t travel_time() const override
  {
    // seems to have a random travel time between 1x - 2x missile speed
    return timespan_t::from_seconds( data().missile_speed() * rng().range( 1.0, 2.0 ) );
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );
    if ( p()->azerite.lunar_shrapnel.ok() && td( target )->dots.moonfire->is_ticking() )
    {
      p()->active.lunar_shrapnel->set_target( s->target );
      p()->active.lunar_shrapnel->execute();
    }
  }
};

struct starfall_t : public druid_spell_t
{
  action_t* starfall_tick;

  starfall_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "starfall", p, p->spec.starfall, options_str )
  {
    may_miss = may_crit = false;
    starfall_tick = p->get_secondary_action<starfall_tick_t>( "starfall_tick" );
    starfall_tick->stats = stats;

    if ( p->azerite.lunar_shrapnel.ok() )
      add_child( p->active.lunar_shrapnel );

    if ( p->legendary.oneths_clear_vision->ok() )
      p->active.oneths_clear_vision->stats->add_child( init_free_cast_stats( free_cast_e::ONETHS ) );
  }

  bool ready() override
  {
    if ( p()->specialization() == DRUID_BALANCE )
      return druid_spell_t::ready();

    return false;
  }

  void execute() override
  {
    if ( !free_cast && p()->buff.oneths_free_starfall->up() )
      free_cast = free_cast_e::ONETHS;

    if ( free_cast )
      starfall_tick->stats = get_free_cast_stats( free_cast );
    else
      starfall_tick->stats = orig_stats;

    if ( p()->spec.starfall_2->ok() )
    {
      int ext = as<int>( p()->spec.starfall_2->effectN( 1 ).base_value() );

      if ( p()->conduit.stellar_inspiration->ok() && rng().roll( p()->conduit.stellar_inspiration.percent() ) )
        ext *= 2;

      std::vector<player_t*>& tl = target_list();

      for ( size_t i = 0, actors = tl.size(); i < actors; i++ )
      {
        player_t* t = tl[ i ];

        td( t )->dots.moonfire->extend_duration( timespan_t::from_seconds( ext ), 0_ms, -1, false );
        td( t )->dots.sunfire->extend_duration( timespan_t::from_seconds( ext ), 0_ms, -1, false );
      }
    }

    druid_spell_t::execute();

    streaking_stars_trigger( SS_STARFALL, nullptr );

    p()->buff.starfall->set_tick_callback( [this]( buff_t*, int, timespan_t ) { starfall_tick->schedule_execute(); } );
    p()->buff.starfall->trigger();

    if ( get_state_free_cast( execute_state ) == free_cast_e::CONVOKE )
      return;  // convoke doesn't process any further

    if ( p()->buff.oneths_free_starfall->check() )
      p()->buff.oneths_free_starfall->expire();

    if ( p()->talent.starlord->ok() )
      p()->buff.starlord->trigger();

    if ( p()->legendary.timeworn_dreambinder->ok() )
      p()->buff.timeworn_dreambinder->trigger();

    if ( p()->legendary.oneths_clear_vision->ok() )
      p()->buff.oneths_free_starsurge->trigger();
  }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  starsurge_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "starsurge", p, p->find_affinity_spell( "Starsurge" ), options_str )
  {
    if ( p->legendary.oneths_clear_vision->ok() )
      p->active.oneths_clear_vision->stats->add_child( init_free_cast_stats( free_cast_e::ONETHS ) );
  }

  timespan_t travel_time() const override  // hack to allow bypassing of action_t::init() precombat check
  {
    return action_list && action_list->name_str == "precombat" ? 100_ms : druid_spell_t::travel_time();
  }

  bool ready() override
  {
    if ( p()->in_combat )
      return druid_spell_t::ready();

    // in precombat, so hijack standard ready() procedure
    auto apl = player->precombat_action_list;

    // emulate performing check_form_restriction()
    auto it = range::find_if( apl, []( action_t* a ) { return util::str_compare_ci( a->name(), "moonkin_form" ); } );
    if ( it == apl.end() )
      return false;

    // emulate performing resource_available( current_resource(), cost() )
    if ( !p()->talent.natures_balance->ok() && p()->initial_astral_power < cost() )
      return false;

    return true;
  }

  void execute() override
  {
    if ( !free_cast && p()->buff.oneths_free_starsurge->up() ) 
      free_cast = free_cast_e::ONETHS;

    druid_spell_t::execute();

    p()->eclipse_handler.cast_starsurge();

    if ( get_state_free_cast( execute_state ) == free_cast_e::CONVOKE )
      return;  // convoke doesn't process any further

    if ( p()->buff.oneths_free_starsurge->check() )
      p()->buff.oneths_free_starsurge->expire();

    if ( p()->talent.starlord->ok() )
      p()->buff.starlord->trigger();

    if ( p()->legendary.timeworn_dreambinder->ok() )
      p()->buff.timeworn_dreambinder->trigger();

    if ( p()->legendary.oneths_clear_vision->ok() )
      p()->buff.oneths_free_starfall->trigger();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = druid_spell_t::bonus_da( s );

    if ( p()->azerite.arcanic_pulsar.ok() )
      da += p()->azerite.arcanic_pulsar.value( 2 );

    return da;
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( p()->azerite.arcanic_pulsar.ok() )
    {
      p()->buff.arcanic_pulsar->trigger();

      if ( p()->buff.arcanic_pulsar->check() == p()->buff.arcanic_pulsar->max_stack() )
      {
        timespan_t pulsar_dur =
            timespan_t::from_seconds( p()->azerite.arcanic_pulsar.spell()->effectN( 3 ).base_value() );
        buff_t* proc_buff =
            p()->talent.incarnation_moonkin->ok() ? p()->buff.incarnation_moonkin : p()->buff.celestial_alignment;

        if ( proc_buff->check() )
          proc_buff->extend_duration( p(), pulsar_dur );
        else
          proc_buff->trigger( pulsar_dur );

        // hardcoded 12AP because 6s / 20s * 40AP = 12AP
        p()->resource_gain( RESOURCE_ASTRAL_POWER, 12, p()->gain.arcanic_pulsar );
        p()->buff.arcanic_pulsar->expire();
        p()->proc.arcanic_pulsar->occur();
        p()->uptime.arcanic_pulsar->update( true, sim->current_time() );
        make_event( *sim, pulsar_dur, [this]() { p()->uptime.arcanic_pulsar->update( false, sim->current_time() ); } );

        streaking_stars_trigger( SS_CELESTIAL_ALIGNMENT, nullptr );
      }
    }

    streaking_stars_trigger( SS_STARSURGE, s );
  }
};

// Stellar Flare ============================================================

struct stellar_flare_t : public druid_spell_t
{
  stellar_flare_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "stellar_flare", player, player->talent.stellar_flare, options_str )
  {
    energize_amount = player->talent.stellar_flare->effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    streaking_stars_trigger( SS_STELLAR_FLARE, s );
  }
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "survival_instincts", player, player->find_specialization_spell( "Survival Instincts" ),
                     options_str )
  {
    harmful     = false;
    use_off_gcd = true;

    cooldown->charges +=
        as<int>( player->find_rank_spell( "Survival Instincts", "Rank 2" )->effectN( 1 ).base_value() );

    // Because vision of perfection does exist, but does not affect this spell for feral.
    if ( player->specialization() == DRUID_GUARDIAN )
    {
      cooldown->duration *= 1.0 + player->talent.survival_of_the_fittest->effectN( 2 ).percent();
      cooldown->duration *= 1.0 + player->vision_of_perfection_cdr;
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.survival_instincts->trigger();
  }
};

// Thorns ===================================================================

struct thorns_t : public druid_spell_t
{
  struct thorns_proc_t : public druid_spell_t
  {
    thorns_proc_t( druid_t* player ) : druid_spell_t( "thorns_hit", player, player->find_spell( 305496 ) )
    {
      background = true;
      if ( p()->specialization() == DRUID_FERAL )
      {  // a little gnarly, TODO(xan): clean this up
        attack_power_mod.direct = 1.2 * p()->query_aura_effect( p()->spec.feral, A_366 )->percent();
        spell_power_mod.direct  = 0;
      }
    }
  };

  struct thorns_attack_event_t : public event_t
  {
    action_t* thorns;
    player_t* target_actor;
    timespan_t attack_period;
    druid_t* source;
    bool randomize_first;
    double hit_chance;

    thorns_attack_event_t( druid_t* player, action_t* thorns_proc, player_t* source, bool randomize = false )
      : event_t( *player ),
        thorns( thorns_proc ),
        target_actor( source ),
        attack_period( timespan_t::from_seconds( player->thorns_attack_period ) ),
        source( player ),
        randomize_first( randomize ),
        hit_chance( player->thorns_hit_chance )
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

  bool available             = false;
  thorns_proc_t* thorns_proc = nullptr;

  thorns_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "thorns", player, player->find_spell( 305497 ), options_str )
  {
    available = p()->azerite.conflict_and_strife.is_major();
    // workaround so that we do not need to enable mana regen
    base_costs[ RESOURCE_MANA ] = 0.0;

    if ( !thorns_proc )
    {
      thorns_proc        = new thorns_proc_t( player );
      thorns_proc->stats = stats;
    }
  }

  bool ready() override
  {
    if ( !available )
      return false;

    return druid_spell_t::ready();
  }

  void execute() override
  {
    p()->buff.thorns->trigger();

    for ( player_t* target : p()->sim->target_non_sleeping_list )
    {
      make_event<thorns_attack_event_t>( *sim, p(), thorns_proc, target, true );
    }

    druid_spell_t::execute();
  }
};

// Warrior of Elune =========================================================

struct warrior_of_elune_t : public druid_spell_t
{
  warrior_of_elune_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "warrior_of_elune", player, player->talent.warrior_of_elune, options_str )
  {
    harmful = may_crit = may_miss = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->cooldown.warrior_of_elune->reset( false );

    p()->buff.warrior_of_elune->trigger( p()->talent.warrior_of_elune->initial_stacks() );
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

  wild_charge_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "wild_charge", p, p->talent.wild_charge, options_str ), movement_speed_increase( 5.0 )
  {
    harmful = may_crit = may_miss = false;
    ignore_false_positive         = true;
    range                         = data().max_range();
    movement_directionality       = movement_direction_type::OMNI;
    trigger_gcd                   = timespan_t::zero();
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

struct force_of_nature_t : public druid_spell_t
{
  timespan_t summon_duration;
  force_of_nature_t( druid_t* p, const std::string options )
    : druid_spell_t( "force_of_nature", p, p->talent.force_of_nature ), summon_duration( timespan_t::zero() )
  {
    parse_options( options );
    harmful = may_crit = false;
    summon_duration    = p->talent.force_of_nature->effectN( 2 ).trigger()->duration() + 1_ms;
    energize_amount    = p->talent.force_of_nature->effectN( 5 ).resource( RESOURCE_ASTRAL_POWER );
  }

  void execute() override
  {
    druid_spell_t::execute();

    for ( size_t i = 0; i < p()->force_of_nature.size(); i++ )
    {
      if ( p()->force_of_nature[ i ]->is_sleeping() )
        p()->force_of_nature[ i ]->summon( summon_duration );
    }
  }
};

struct kindred_spirits_t : public druid_spell_t
{
  kindred_spirits_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "empower_bond", player, player->covenant.empower_bond, options_str )
  {
    if ( !player->covenant.kyrian->ok() )
      return;

    harmful = false;

    if ( player->active.kindred_empowerment )
      add_child( player->active.kindred_empowerment );

    if ( player->active.kindred_empowerment_partner )
      add_child( player->active.kindred_empowerment_partner );

    if ( player->conduit.deep_allegiance->ok() )
      cooldown->duration *= 1.0 + player->conduit.deep_allegiance.percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.kindred_empowerment_energize->trigger();
  }
};

struct convoke_the_spirits_t : public druid_spell_t
{
  shuffled_rng_t* deck;
  bool heal_cast;
  double heal_chance;

  // Balance
  action_t* conv_fm;
  action_t* conv_ss;
  action_t* conv_wr;
  action_t* conv_sf;
  action_t* conv_mf;
  bool mf_cast;
  int mf_count;
  int ss_count;
  int wr_count;

  // Feral
  // Guardian
  // Restoration

  convoke_the_spirits_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "convoke_the_spirits", p, p->covenant.night_fae, options_str )
  {
    if ( !p->covenant.night_fae->ok() )
      return;

    parse_options( options_str );

    harmful = channeled = true;
    may_miss = may_crit = false;

    int heals_int = as<int>( util::ceil( p->convoke_the_spirits_heals ) );
    heal_chance = heals_int - p->convoke_the_spirits_heals;
    deck = p->get_shuffled_rng( "convoke_the_spirits", heals_int, static_cast<int>( dot_duration / base_tick_time ) );

    // Balance
    conv_fm = get_convoke_action<full_moon_t>( "full_moon", options_str );
    conv_wr = get_convoke_action<wrath_t>( "wrath", options_str );
    conv_ss = get_convoke_action<starsurge_t>( "starsurge", options_str );
    conv_sf = get_convoke_action<starfall_t>( "starfall", options_str );
    conv_mf = get_convoke_action<moonfire_t>( "moonfire",options_str );

    // Feral
    // Guardian
    // Restoration
  }

  template <typename T, typename... Ts>
  T* get_convoke_action( util::string_view n, Ts&&... args )
  {
    auto a = p()->get_secondary_action<T>( n, std::forward<Ts>( args )... );
    stats->add_child( a->init_free_cast_stats( free_cast_e::CONVOKE ) );
    return a;
  }

  double composite_haste() const override { return 1.0; }

  void execute() override
  {
    druid_spell_t::execute();
    p()->reset_auto_attacks( composite_dot_duration( execute_state ) );

    p()->buff.convoke_the_spirits->trigger();

    deck->reset();

    heal_cast = false;

    // Balance
    mf_cast  = false;
    mf_count = 0;
    ss_count = 0;
    wr_count = 0;

    // Feral
    // Guardian
    // Restoration
  }

  void tick( dot_t* d ) override
  {
    action_t* conv_cast = nullptr;
    player_t* conv_tar  = nullptr;

    druid_spell_t::tick( d );

    if ( deck->trigger() )  // success == HEAL cast
    {
      if ( heal_cast )
        return;

      heal_cast = true;

      if ( !rng().roll( heal_chance ) )
        return;
    }

    std::vector<player_t*> tl = target_list();
    if ( !tl.size() )
      return;

    conv_tar = tl[ static_cast<size_t>( rng().range( 0, as<double>( tl.size() ) ) ) ];

    if ( p()->buff.moonkin_form->check() )
    {
      if ( rng().roll( p()->convoke_the_spirits_ultimate ) )  // assume 1% 'ultimate' overrides spell selection logic
        conv_cast = conv_fm;
      else
      {
        if ( !p()->buff.starfall->check() )  // always starfall if it isn't up
          conv_cast = conv_sf;
        else  // randomly decide on damage spell
        {
          std::vector<player_t*> mf_tl;  // separate list for mf targets without a dot;
          for ( auto i : tl )
          {
            if ( !td( i )->dots.moonfire->is_ticking() )
              mf_tl.push_back( i );
          }

          player_t* mf_tar = nullptr;
          if ( mf_tl.size() )
            mf_tar = mf_tl[ static_cast<size_t>( rng().range( 0, as<double>( mf_tl.size() ) ) ) ];

          if ( !mf_cast && mf_tar )  // always mf once if at least one eligible target
          {
            conv_cast = conv_mf;
            mf_cast = true;
          }
          else  // try to keep spell count balanced
          {
            std::vector<action_t*> dam;

            if ( mf_tar && mf_count <= ss_count && mf_count <= wr_count )  // balance mf against ss & wr
              dam.push_back( conv_mf );

            if ( ss_count <= wr_count )  // balance ss only against wr
              dam.push_back( conv_ss );

            if ( wr_count <= ss_count )  // balance wr only against ss
              dam.push_back( conv_wr );

            if ( !dam.size() )  // sanity check
              return;

            conv_cast = dam[ static_cast<size_t>( rng().range( 0, as<double>( dam.size() ) ) ) ];

            if ( conv_cast == conv_mf )
              mf_count++;
            else if ( conv_cast == conv_ss )
              ss_count++;
            else if ( conv_cast == conv_wr )
              wr_count++;
          }

          if ( conv_cast == conv_mf )
            conv_tar = mf_tar;
        }
      }
      debug_cast<druid_action_t<spell_t>*>( conv_cast )->free_cast = free_cast_e::CONVOKE;
    }

    if ( !conv_cast || !conv_tar )
      return;

    conv_cast->set_target( conv_tar );
    conv_cast->execute();
  }

  void last_tick( dot_t* d ) override
  {
    druid_spell_t::last_tick( d );

    if ( p()->buff.convoke_the_spirits->check() )
      p()->buff.convoke_the_spirits->expire();
  }

  bool usable_moving() const override { return true; }
};

struct ravenous_frenzy_t : public druid_spell_t
{
  ravenous_frenzy_t( druid_t* player, const std::string& options_str )
    : druid_spell_t( "ravenous_frenzy", player, player->covenant.venthyr, options_str )
  {
    if ( !player->covenant.venthyr->ok() )
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

struct adaptive_swarm_t : public druid_spell_t
{
  struct adaptive_swarm_state_t : public druid_action_state_t
  {
  private:
    int default_stacks;

  public:
    int stacks;

    adaptive_swarm_state_t( action_t* a, player_t* t ) : druid_action_state_t( a, t )
    {
      default_stacks = as<int>( debug_cast<druid_t*>( a->player )->covenant.necrolord->effectN( 1 ).base_value() );
    }

    void initialize() override
    {
      action_state_t::initialize();

      stacks = default_stacks;
    }

    void copy_state( const action_state_t* s ) override
    {
      druid_action_state_t::copy_state( s );
      const adaptive_swarm_state_t* swarm_s = debug_cast<const adaptive_swarm_state_t*>( s );

      stacks = swarm_s->stacks;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      druid_action_state_t::debug_str( s );

      s << " swarm_stacks=" << stacks;

      return s;
    }
  };

  struct adaptive_swarm_base_t : public druid_spell_t
  {
    adaptive_swarm_base_t* other;
    bool heal;

    adaptive_swarm_base_t( druid_t* p, util::string_view n, const spell_data_t* sd )
      : druid_spell_t( n, p, sd ), other( nullptr )
    {
      dual = background = true;
      dot_behavior      = dot_behavior_e::DOT_CLIP;
    }

    action_state_t* new_state() override { return new adaptive_swarm_state_t( this, target ); }

    player_t* new_swarm_target()
    {
      auto tl = other->target_list();
      if ( !tl.size() )
        return nullptr;

      player_t* tar = nullptr;

      // Target priority
      std::vector<player_t*> tl_1;  // 1: has a dot, but no swarm
      std::vector<player_t*> tl_2;  // 2: has no dot and not swarm
      std::vector<player_t*> tl_3;  // 3: has a dot, but also swarm
      std::vector<player_t*> tl_4;  // 4: rest

      for ( const auto& t : tl )
      {
        auto t_td = other->td( t );

        if ( other->heal )
        {
          if ( !t_td->dots.adaptive_swarm_heal->is_ticking() )
          {
            if ( t_td->hot_ticking() ) tl_1.push_back( t );
            else                       tl_2.push_back( t );
          }
          else
          {
            if ( t_td->hot_ticking() ) tl_3.push_back( t );
            else                       tl_4.push_back( t );
          }
        }
        else
        {
          if ( !t_td->dots.adaptive_swarm_damage->is_ticking() )
          {
            if ( t_td->dot_ticking() ) tl_1.push_back( t );
            else                       tl_2.push_back( t );
          }
          else
          {
            if ( t_td->dot_ticking() ) tl_3.push_back( t );
            else                       tl_4.push_back( t );
          }
        }
      }

      if      ( tl_1.size() ) tar = tl_1[ static_cast<size_t>( rng().range( 0, as<double>( tl_1.size() ) ) ) ];
      else if ( tl_2.size() ) tar = tl_2[ static_cast<size_t>( rng().range( 0, as<double>( tl_2.size() ) ) ) ];
      else if ( tl_3.size() ) tar = tl_3[ static_cast<size_t>( rng().range( 0, as<double>( tl_3.size() ) ) ) ];
      else if ( tl_4.size() ) tar = tl_4[ static_cast<size_t>( rng().range( 0, as<double>( tl_4.size() ) ) ) ];

      return tar;
    }

    void impact( action_state_t* s ) override
    {
      auto incoming = debug_cast<adaptive_swarm_state_t*>( s )->stacks;
      auto existing = get_dot( s->target )->current_stack();

      druid_spell_t::impact( s );

      auto d = get_dot( s->target );
      d->increment( incoming + existing - 1 );
      sim->print_debug( "adaptive_swarm stacks: {} incoming:{} existing:{} final:{}", heal ? "heal" : "damage",
                        incoming, existing, d->current_stack() );
    }

    void last_tick( dot_t* d ) override
    {
      druid_spell_t::last_tick( d );

      // last_tick() is called via dot_t::cancel() when a new swarm CLIPs an existing swarm. As dot_t::last_tick() is
      // called before dot_t::reset(), check the remaining time on the dot to see if we're dealing with a true last_tick
      // (and thus need to check for a new jump) or just a refresh.

      // NOTE: if demise() is called on the target d->remains() has time left, so assume that a target that is sleeping
      // with time remaining has been demised()'d and jump to a new target
      if ( d->remains() > 0_ms && !d->target->is_sleeping() )
        return;

      int stacks = d->current_stack();
      if ( stacks <= 1 )
      {
        sim->print_debug( "adaptive_swarm stacks: expiring on final stack" );
        return;
      }

      auto new_target = new_swarm_target();
      if ( !new_target )
      {
        sim->print_debug( "adaptive_swarm stacks: expiring {} stacks with no target found", stacks );
        return;
      }

      stacks--;

      sim->print_debug( "adaptive_swarm stacks: jumping {} stacks of {} to {}", stacks, heal ? "damage" : "heal",
                        new_target->name() );

      auto new_state = get_state();
      debug_cast<adaptive_swarm_state_t*>( new_state )->stacks = stacks;

      new_state->target = new_target;
      other->set_target( new_target );
      other->schedule_execute( new_state );
    }
  };

  struct adaptive_swarm_damage_t : public adaptive_swarm_base_t
  {
    adaptive_swarm_damage_t( druid_t* p )
      : adaptive_swarm_base_t( p, "adaptive_swarm_damage", p->covenant.adaptive_swarm_damage )
    {
      heal = false;
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( !t ) t = target;
      if ( !t ) return nullptr;

      return td( t )->dots.adaptive_swarm_damage;
    }

    double calculate_tick_amount( action_state_t* s, double m ) const override
    {
      auto stack = td( s->target )->dots.adaptive_swarm_damage->current_stack();
      assert( stack );

      return adaptive_swarm_base_t::calculate_tick_amount( s, m / stack );
    }
  };

  struct adaptive_swarm_heal_t : public adaptive_swarm_base_t
  {
    adaptive_swarm_heal_t( druid_t* p )
      : adaptive_swarm_base_t( p, "adaptive_swarm_heal", p->covenant.adaptive_swarm_heal )
    {
      quiet = heal = true;
      may_miss = may_crit = false;
      base_td = base_td_multiplier = 0.0;
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( !t ) t = target;
      if ( !t ) return nullptr;

      return td( t )->dots.adaptive_swarm_heal;
    }
  };

  adaptive_swarm_base_t* damage;
  adaptive_swarm_base_t* heal;

  adaptive_swarm_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "adaptive_swarm", p, p->covenant.necrolord, options_str )
  {
    parse_options( options_str );

    // These are always necessary to allow APL parsing of dot.adaptive_swarm expressions
    damage = p->get_secondary_action<adaptive_swarm_damage_t>( "adaptive_swarm_damage" );
    heal   = p->get_secondary_action<adaptive_swarm_heal_t>( "adaptive_swarm_heal" );

    if ( !p->covenant.necrolord->ok() )
      return;

    may_miss = may_crit = false;
    base_costs[ RESOURCE_MANA ] = 0.0;  // remove mana cost so we don't need to enable mana regen

    damage->other = heal;
    damage->stats = stats;
    heal->other   = damage;
    add_child( damage );
  }

  void execute() override
  {
    druid_spell_t::execute();

    damage->set_target( target );
    damage->schedule_execute();
  }
};
}  // end namespace spells

// ==========================================================================
// Druid Helper Functions & Structures
// ==========================================================================

// Brambles Absorb/Reflect Handler ==========================================

double brambles_handler( const action_state_t* s )
{
  druid_t* p = static_cast<druid_t*>( s->target );
  assert( p->talent.brambles->ok() );
  assert( s );

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

  druid_t* p = static_cast<druid_t*>( s->target );
  assert( p->talent.earthwarden->ok() );

  if ( !p->buff.earthwarden->up() )
    return 0;

  double absorb = s->result_amount * p->buff.earthwarden->check_value();
  p->buff.earthwarden->decrement();

  return absorb;
}

// Persistent Buff Delay Event ==============================================

struct persistent_buff_delay_event_t : public event_t
{
  buff_t* buff;

  persistent_buff_delay_event_t( druid_t* p, buff_t* b ) : event_t( *p ), buff( b )
  {
    // Delay triggering the buff a random amount between 0 and buff_period. This prevents fixed-period driver buffs from
    // ticking at the exact same times on every iteration. Buffs that use the event to activate should implement
    // tick_zero-like behavior.
    schedule( rng().real() * b->buff_period );
  }

  const char* name() const override { return "persistent_buff_delay"; }

  void execute() override { buff->trigger(); }
};

struct lycaras_fleeting_glimpse_t : public action_t
{
  druid_t* d;
  action_t* moonkin;
  action_t* cat;
  action_t* bear;
  action_t* tree;

  lycaras_fleeting_glimpse_t( druid_t* p )
    : action_t( action_e::ACTION_OTHER, "lycaras_fleeting_glimpse", p, p->legendary.lycaras_fleeting_glimpse ), d( p ),
      moonkin( nullptr ), cat( nullptr ), bear( nullptr ), tree( nullptr )
  {
    moonkin = get_lycaras_action<spells::starfall_t>( "starfall", "" );
    cat = get_lycaras_action<cat_attacks::primal_wrath_t>( "primal_wrath", d->find_spell( 285381 ), "" );
  }

  template <typename T, typename... Ts>
  T* get_lycaras_action( util::string_view n, Ts&&... args )
  {
    auto a = d->get_secondary_action<T>( n, std::forward<Ts>( args )... );
    stats->add_child( a->init_free_cast_stats( free_cast_e::LYCARAS ) );
    return a;
  }

  void execute() override
  {
    action_t* a;

    if ( d->buff.moonkin_form->check() )
    {
      a = moonkin;
      debug_cast<spells::starfall_t*>( a )->free_cast = free_cast_e::LYCARAS;
    }
    else if ( d->buff.cat_form->check() )
    {
      a = cat;
      debug_cast<cat_attacks::primal_wrath_t*>( a )->free_cast = free_cast_e::LYCARAS;
    }
    else if ( d->buff.bear_form->check() )
      a = bear;
    else
      a = tree;

    if ( a )
    {
      a->set_target( d->target );
      a->execute();
    }
  }
};

struct lycaras_fleeting_glimpse_event_t : public event_t
{
  druid_t* d;
  timespan_t interval;

  lycaras_fleeting_glimpse_event_t( druid_t* p, timespan_t tm ) : event_t( *p, tm ), d( p ), interval( tm ) {}

  const char* name() const override { return "lycaras_fleeting_glimpse"; }

  void execute() override
  {
    d->active.lycaras_fleeting_glimpse->execute();
    make_event<lycaras_fleeting_glimpse_event_t>( sim(), d, interval );
  }
};

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::activate ========================================================
void druid_t::activate()
{
  if ( talent.predator->ok() )
  {
    range::for_each( sim->actor_list, [this]( player_t* target ) -> void {
      if ( !target->is_enemy() )
        return;

      target->callbacks_on_demise.emplace_back( [this]( player_t* target ) -> void {
        auto p = this;
        if ( p->specialization() != DRUID_FERAL )
          return;

        if ( get_target_data( target )->dots.thrash_cat->is_ticking() ||
             get_target_data( target )->dots.rip->is_ticking() ||
             get_target_data( target )->dots.rake->is_ticking() )
        {
          if ( !p->cooldown.tigers_fury->down() )
            p->proc.predator_wasted->occur();

          p->cooldown.tigers_fury->reset( true );
          p->proc.predator->occur();
        }
      } );
    } );

    callbacks_on_demise.emplace_back( [this]( player_t* target ) -> void {
      if ( sim->active_player->specialization() != DRUID_FERAL )
        return;

      if ( get_target_data( target )->dots.thrash_cat->is_ticking() ||
           get_target_data( target )->dots.rip->is_ticking() ||
           get_target_data( target )->dots.rake->is_ticking() )
      {
        auto p = ( (druid_t*)sim->active_player );

        if ( !p->cooldown.tigers_fury->down() )
          p->proc.predator_wasted->occur();

        p->cooldown.tigers_fury->reset( true );
        p->proc.predator->occur();
      }
    } );
  }

   player_t::activate();
}

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( util::string_view name, const std::string& options_str )
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  // Generic
  if ( name == "auto_attack"            ) return new            auto_attack_t( this, options_str );
  if ( name == "bear_form"              ) return new      spells::bear_form_t( this, options_str );
  if ( name == "cat_form"               ) return new       spells::cat_form_t( this, options_str );
  if ( name == "moonkin_form"           ) return new   spells::moonkin_form_t( this, options_str );
  if ( name == "dash"                   ) return new                   dash_t( this, options_str );
  if ( name == "entangling_roots"       ) return new       entangling_roots_t( this, options_str );
  if ( name == "heart_of_the_wild"      ) return new      heart_of_the_wild_t( this, options_str );
  if ( name == "moonfire"               ) return new               moonfire_t( this, options_str );
  if ( name == "prowl"                  ) return new                  prowl_t( this, options_str );
  if ( name == "renewal"                ) return new                renewal_t( this, options_str );
  if ( name == "stampeding_roar"        ) return new        stampeding_roar_t( this, options_str );
  if ( name == "tiger_dash"             ) return new             tiger_dash_t( this, options_str );
  if ( name == "wild_charge"            ) return new            wild_charge_t( this, options_str );

  // Multispec
  if ( name == "berserk" )
  {
    if ( specialization() == DRUID_GUARDIAN )   return new     berserk_bear_t( this, options_str );
    else if ( specialization() == DRUID_FERAL ) return new      berserk_cat_t( this, options_str );
  }
  if ( name == "incarnation"            ) return new            incarnation_t( this, options_str );
  if ( name == "swipe"                  ) return new            swipe_proxy_t( this, options_str );
  if ( name == "thrash"                 ) return new           thrash_proxy_t( this, options_str );

  // Balance
  if ( name == "celestial_alignment"    ) return new    celestial_alignment_t( this, options_str );
  if ( name == "force_of_nature"        ) return new        force_of_nature_t( this, options_str );
  if ( name == "fury_of_elune"          ) return new          fury_of_elune_t( this, options_str );
  if ( name == "innervate"              ) return new              innervate_t( this, options_str );
  if ( name == "new_moon"               ) return new               new_moon_t( this, options_str );
  if ( name == "half_moon"              ) return new              half_moon_t( this, options_str );
  if ( name == "full_moon"              ) return new              full_moon_t( this, options_str );
  if ( name == "starfall"               ) return new               starfall_t( this, options_str );
  if ( name == "starfire"               ) return new               starfire_t( this, options_str );
  if ( name == "starsurge"              ) return new              starsurge_t( this, options_str );
  if ( name == "stellar_flare"          ) return new          stellar_flare_t( this, options_str );
  if ( name == "sunfire"                ) return new                sunfire_t( this, options_str );
  if ( name == "warrior_of_elune"       ) return new       warrior_of_elune_t( this, options_str );
  if ( name == "wrath"                  ) return new                  wrath_t( this, options_str );

  // Feral
  if ( name == "berserk_cat"            ) return new            berserk_cat_t( this, options_str );
  if ( name == "brutal_slash"           ) return new           brutal_slash_t( this, options_str );
  if ( name == "feral_frenzy"           ) return new    feral_frenzy_driver_t( this, options_str );
  if ( name == "ferocious_bite"         ) return new         ferocious_bite_t( this, options_str );
  if ( name == "maim"                   ) return new                   maim_t( this, options_str );
  if ( name == "moonfire_cat" ||
       name == "lunar_inspiration"      ) return new      lunar_inspiration_t( this, options_str );
  if ( name == "primal_wrath"           ) return new           primal_wrath_t( this, options_str );
  if ( name == "rake"                   ) return new                   rake_t( this, options_str );
  if ( name == "rip"                    ) return new                    rip_t( this, options_str );
  if ( name == "savage_roar"            ) return new            savage_roar_t( this, options_str );
  if ( name == "shred"                  ) return new                  shred_t( this, options_str );
  if ( name == "skull_bash"             ) return new             skull_bash_t( this, options_str );
  if ( name == "swipe_cat"              ) return new              swipe_cat_t( this, options_str );
  if ( name == "thrash_cat"             ) return new             thrash_cat_t( this, options_str );
  if ( name == "tigers_fury"            ) return new            tigers_fury_t( this, options_str );

  // Guardian
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk_bear"           ) return new           berserk_bear_t( this, options_str );
  if ( name == "bristling_fur"          ) return new          bristling_fur_t( this, options_str );
  if ( name == "frenzied_regeneration"  ) return new  frenzied_regeneration_t( this, options_str );
  if ( name == "growl"                  ) return new                  growl_t( this, options_str );
  if ( name == "ironfur"                ) return new                ironfur_t( this, options_str );
  if ( name == "lunar_beam"             ) return new             lunar_beam_t( this, options_str );
  if ( name == "mangle"                 ) return new                 mangle_t( this, options_str );
  if ( name == "maul"                   ) return new                   maul_t( this, options_str );
  if ( name == "pulverize"              ) return new              pulverize_t( this, options_str );
  if ( name == "survival_instincts"     ) return new     survival_instincts_t( this, options_str );
  if ( name == "swipe_bear"             ) return new             swipe_bear_t( this, options_str );
  if ( name == "thrash_bear"            ) return new            thrash_bear_t( this, options_str );

  // Restoration
  if ( name == "cenarion_ward"          ) return new          cenarion_ward_t( this, options_str );
  if ( name == "lifebloom"              ) return new              lifebloom_t( this, options_str );
  if ( name == "regrowth"               ) return new               regrowth_t( this, options_str );
  if ( name == "rejuvenation"           ) return new           rejuvenation_t( this, options_str );
  if ( name == "swiftmend"              ) return new              swiftmend_t( this, options_str );
  if ( name == "thorns"                 ) return new                 thorns_t( this, options_str );
  if ( name == "tranquility"            ) return new            tranquility_t( this, options_str );
  if ( name == "wild_growth"            ) return new            wild_growth_t( this, options_str );

  // Covenant
  if ( name == "adaptive_swarm"         ) return new         adaptive_swarm_t( this, options_str );
  if ( name == "convoke_the_spirits"    ) return new    convoke_the_spirits_t( this, options_str );
  if ( name == "kindred_spirits" ||
       name == "empower_bond"           ) return new        kindred_spirits_t( this, options_str );
  if ( name == "ravenous_frenzy"        ) return new        ravenous_frenzy_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( util::string_view pet_name, util::string_view /* pet_type */ )
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

  if ( talent.force_of_nature->ok() )
  {
    for ( pet_t*& pet : force_of_nature )
      pet = new pets::force_of_nature_t( sim, this );
  }
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  auto check_spell = [this]( bool b, unsigned id ) -> const spell_data_t* {
    return b ? find_spell( id ) : spell_data_t::not_found();
  };

  player_t::init_spells();

  // Talents ================================================================

  // Multiple Specs
  talent.tiger_dash              = find_talent_spell( "Tiger Dash" );
  talent.renewal                 = find_talent_spell( "Renewal" );
  talent.wild_charge             = find_talent_spell( "Wild Charge" );
  
  talent.balance_affinity        = find_talent_spell( "Balance Affinity" );
  talent.feral_affinity          = find_talent_spell( "Feral Affinity" );
  talent.guardian_affinity       = find_talent_spell( "Guardian Affinity" );
  talent.restoration_affinity    = find_talent_spell( "Restoration Affinity" );
  
  talent.mighty_bash             = find_talent_spell( "Mighty Bash" );
  talent.mass_entanglement       = find_talent_spell( "Mass Entanglement" );
  talent.heart_of_the_wild       = find_talent_spell( "Heart of the Wild" );
  
  talent.soul_of_the_forest      = find_talent_spell( "Soul of the Forest" );
  talent.soul_of_the_forest_moonkin =
      specialization() == DRUID_BALANCE ? talent.soul_of_the_forest : spell_data_t::not_found();
  talent.soul_of_the_forest_cat =
      specialization() == DRUID_FERAL ? talent.soul_of_the_forest : spell_data_t::not_found();
  talent.soul_of_the_forest_bear =
      specialization() == DRUID_GUARDIAN ? talent.soul_of_the_forest : spell_data_t::not_found();
  talent.soul_of_the_forest_tree =
      specialization() == DRUID_RESTORATION ? talent.soul_of_the_forest : spell_data_t::not_found();

  // Feral
  talent.predator                = find_talent_spell( "Predator" );
  talent.sabertooth              = find_talent_spell( "Sabertooth" );
  talent.lunar_inspiration       = find_talent_spell( "Lunar Inspiration" );
  
  talent.savage_roar             = find_talent_spell( "Savage Roar" );
  talent.incarnation_cat         = find_talent_spell( "Incarnation: King of the Jungle" );
  talent.scent_of_blood          = find_talent_spell( "Scent of Blood" );
  
  talent.brutal_slash            = find_talent_spell( "Brutal Slash" );
  talent.primal_wrath            = find_talent_spell( "Primal Wrath" );
  
  talent.moment_of_clarity       = find_talent_spell( "Moment of Clarity" );
  talent.bloodtalons             = find_talent_spell( "Bloodtalons" );
  talent.feral_frenzy            = find_talent_spell( "Feral Frenzy" );

  // Balance
  talent.natures_balance         = find_talent_spell( "Nature's Balance" );
  talent.warrior_of_elune        = find_talent_spell( "Warrior of Elune" );
  talent.force_of_nature         = find_talent_spell( "Force of Nature" );

  talent.starlord                = find_talent_spell( "Starlord" );
  talent.incarnation_moonkin     = find_talent_spell( "Incarnation: Chosen of Elune" );

  talent.stellar_drift           = find_talent_spell( "Stellar Drift" );
  talent.twin_moons              = find_talent_spell( "Twin Moons" );
  talent.stellar_flare           = find_talent_spell( "Stellar Flare" );

  talent.solstice                = find_talent_spell( "Solstice" );
  talent.fury_of_elune           = find_talent_spell( "Fury of Elune" );
  talent.new_moon                = find_talent_spell( "New Moon" );

  // Guardian
  talent.brambles                = find_talent_spell( "Brambles" );
  talent.bristling_fur           = find_talent_spell( "Bristling Fur" );
  talent.blood_frenzy            = find_talent_spell( "Blood Frenzy" );
  
  talent.galactic_guardian       = find_talent_spell( "Galactic Guardian" );
  talent.incarnation_bear        = find_talent_spell( "Incarnation: Guardian of Ursoc" );
  
  talent.earthwarden             = find_talent_spell( "Earthwarden" );
  talent.survival_of_the_fittest = find_talent_spell( "Survival of the Fittest" );
  talent.guardian_of_elune       = find_talent_spell( "Guardian of Elune" );
  
  talent.rend_and_tear           = find_talent_spell( "Rend and Tear" );
  talent.lunar_beam              = find_talent_spell( "Lunar Beam" );
  talent.pulverize               = find_talent_spell( "Pulverize" );
  
  talent.sharpened_claws         = find_spell( 202110, DRUID_GUARDIAN );

  // Restoration
  talent.cenarion_ward           = find_talent_spell( "Cenarion Ward" );

  talent.cultivation             = find_talent_spell( "Cultivation" );
  talent.incarnation_tree        = find_talent_spell( "Incarnation: Tree of Life" );

  talent.inner_peace             = find_talent_spell( "Inner Peace" );

  talent.germination             = find_talent_spell( "Germination" );
  talent.flourish                = find_talent_spell( "Flourish" );

  if ( talent.earthwarden->ok() )
  {
    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>(
        talent.earthwarden->id(),
        instant_absorb_t( this, find_spell( 203975 ), "earthwarden", &earthwarden_handler ) ) );
  }

  // Covenants
  covenant.kyrian                       = find_covenant_spell( "Kindred Spirits" );
  covenant.empower_bond                 = check_spell( covenant.kyrian->ok(), 326446 );
  covenant.kindred_empowerment          = check_spell( covenant.kyrian->ok(), 327022 );
  covenant.kindred_empowerment_energize = check_spell( covenant.kyrian->ok(), 327139 );
  covenant.kindred_empowerment_damage   = check_spell( covenant.kyrian->ok(), 338411 );
  covenant.night_fae                    = find_covenant_spell( "Convoke the Spirits" );
  covenant.venthyr                      = find_covenant_spell( "Ravenous Frenzy" );
  covenant.necrolord                    = find_covenant_spell( "Adaptive Swarm" );
  covenant.adaptive_swarm_damage        = check_spell( covenant.necrolord->ok(), 325733 );
  covenant.adaptive_swarm_heal          = check_spell( covenant.necrolord->ok(), 325748 );

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

  // Covenant
  conduit.deep_allegiance      = find_conduit_spell( "Deep Allegiance" );
  conduit.evolved_swarm        = find_conduit_spell( "Evolved Swarm" );
  conduit.conflux_of_elements  = find_conduit_spell( "Conflux of Elements" );
  conduit.endless_thirst       = find_conduit_spell( "Endless Thirst" );

  // Runeforge Legendaries

  // General
  legendary.oath_of_the_elder_druid   = find_runeforge_legendary( "Oath of the Elder Druid" );
  legendary.circle_of_life_and_death  = find_runeforge_legendary( "Circle of Life and Death" );
  legendary.draught_of_deep_focus     = find_runeforge_legendary( "Draught of Deep Focus" );
  legendary.lycaras_fleeting_glimpse  = find_runeforge_legendary( "Lycara's Fleeting Glimpse" );

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
  legendary.ursocs_lingering_spirit   = find_runeforge_legendary( "Ursoc's Lingering Spirit" );
  legendary.legacy_of_the_sleeper     = find_runeforge_legendary( "Legacy of the Sleeper" );

  // Restoration

  // Specializations ========================================================

  // Generic / Multiple specs
  spec.druid                  = find_spell( 137009 );
  spec.critical_strikes       = find_specialization_spell( "Critical Strikes" );
  spec.leather_specialization = find_specialization_spell( "Leather Specialization" );
  spec.omen_of_clarity        = find_specialization_spell( "Omen of Clarity" );
  spec.entangling_roots       = find_class_spell( "Entangling Roots" );
  spec.barkskin               = find_class_spell( "Barkskin" );
  spec.ironfur                = find_class_spell( "Ironfur" );

  // Balance
  spec.balance                = find_specialization_spell( "Balance Druid" );
  spec.astral_power           = find_specialization_spell( "Astral Power" );
  spec.moonkin_form           = find_affinity_spell( "Moonkin Form" );
  spec.owlkin_frenzy          = check_spell( spec.moonkin_form->ok(), 157228 );  // Owlkin Frenzy RAWR
  spec.celestial_alignment    = find_specialization_spell( "Celestial Alignment" );
  spec.innervate              = find_specialization_spell( "Innervate" );
  spec.eclipse                = find_specialization_spell( "Eclipse" );
  spec.eclipse_2              = find_rank_spell( "Eclipse", "Rank 2" );
  spec.eclipse_solar          = find_spell( 48517 );
  spec.eclipse_lunar          = find_spell( 48518 );
  spec.sunfire_dmg            = check_spell( find_affinity_spell( "Sunfire" )->ok(), 164815 );  // dot debuff for sunfire
  spec.moonfire_dmg           = check_spell( find_class_spell( "Moonfire" )->ok(), 164812 );    // dot debuff for moonfire
  spec.starsurge              = find_spell( 78674 );  // do NOT use find_affinity_spell. eclipse buff is held within the balance version.
  spec.starsurge_2            = find_rank_spell( "Starsurge", "Rank 2" );  // Adds bigger eclipse buff
  spec.starfall               = find_affinity_spell( "Starfall" );
  spec.starfall_2             = find_rank_spell( "Starfall", "Rank 2" );
  spec.starfall_dmg           = check_spell( spec.starfall->ok(), 191037 );
  spec.stellar_drift          = check_spell( talent.stellar_drift->ok(), 202461 );   // stellar drift mobility buff
  spec.shooting_stars         = find_specialization_spell( "Shooting Stars" );
  spec.shooting_stars_dmg     = check_spell( spec.shooting_stars->ok(), 202497 );  // shooting stars damage
  spec.fury_of_elune          = check_spell( talent.fury_of_elune->ok(), 211545 );   // fury of elune tick damage
  spec.half_moon              = check_spell( talent.new_moon->ok(), 274282 );
  spec.full_moon              = check_spell( talent.new_moon->ok() || covenant.night_fae->ok(), 274283 );
  spec.moonfire_2             = find_rank_spell( "Moonfire", "Rank 2" );
  spec.moonfire_3             = find_rank_spell( "Moonfire", "Rank 3" );

  // Feral
  spec.feral                  = find_specialization_spell( "Feral Druid" );
  spec.feral_overrides        = find_specialization_spell( "Feral Overrides Passive" );
  spec.cat_form               = check_spell( find_class_spell( "Cat Form" )->ok(), 3025 );
  spec.cat_form_speed         = check_spell( find_class_spell( "Cat Form" )->ok(), 113636 );
  spec.predatory_swiftness    = find_specialization_spell( "Predatory Swiftness" );
  spec.primal_fury            = find_affinity_spell( "Primary Fury" )->effectN( 1 ).trigger();
  spec.rip                    = find_affinity_spell( "Rip" );
  spec.sharpened_claws        = find_specialization_spell( "Sharpened Claws" );
  spec.swipe_cat              = check_spell( find_affinity_spell( "Swipe" )->ok(), 106785 );
  spec.thrash_cat             = check_spell( find_specialization_spell( "Thrash" )->ok(), 106830 );
  spec.berserk_cat            = find_specialization_spell( "Berserk" );
  spec.rake_dmg               = find_affinity_spell( "Rake" )->effectN( 3 ).trigger();
  spec.tigers_fury            = find_specialization_spell( "Tiger's Fury" );
  spec.shred                  = find_class_spell( "Shred" );
  spec.savage_roar            = check_spell( talent.savage_roar->ok(), 62071 );
  spec.bloodtalons            = check_spell( talent.bloodtalons->ok(), 145152 );

  // Guardian
  spec.guardian               = find_specialization_spell( "Guardian Druid" );
  spec.lightning_reflexes     = find_specialization_spell( "Lightning Reflexes" );
  spec.bear_form              = check_spell( find_class_spell( "Bear Form" )->ok(), 1178 );
  spec.bear_form_2            = find_rank_spell( "Bear Form", "Rank 2" );
  spec.gore                   = find_specialization_spell( "Gore" );
  spec.swipe_bear             = check_spell( find_specialization_spell( "Swipe" )->ok(), 213771 );
  spec.thrash_bear            = check_spell( find_affinity_spell( "Thrash" )->ok(), 77758 );
  spec.thrash_bear_dot        = check_spell( spec.thrash_bear->ok(), 192090 );
  spec.berserk_bear           = check_spell( find_specialization_spell( "Berserk" )->ok(), 50334 );
  spec.berserk_bear_2         = check_spell( spec.berserk_bear->ok(), 343240 );

  // Restoration
  spec.restoration            = find_specialization_spell( "Restoration Druid" );

  // Azerite ================================================================
  // Balance
  azerite.dawning_sun = find_azerite_spell("Dawning Sun");
  azerite.high_noon = find_azerite_spell("High Noon");
  azerite.lively_spirit = find_azerite_spell("Lively Spirit");
  azerite.lunar_shrapnel = find_azerite_spell("Lunar Shrapnel");
  azerite.power_of_the_moon = find_azerite_spell("Power of the Moon");
  azerite.streaking_stars = find_azerite_spell("Streaking Stars");
  azerite.arcanic_pulsar = find_azerite_spell("Arcanic Pulsar");

  // Feral
  azerite.blood_mist = find_azerite_spell("Blood Mist");
  azerite.gushing_lacerations = find_azerite_spell("Gushing Lacerations");
  azerite.iron_jaws = find_azerite_spell("Iron Jaws");
  azerite.primordial_rage = find_azerite_spell("Primordial Rage");
  azerite.raking_ferocity = find_azerite_spell("Raking Ferocity");
  azerite.shredding_fury = find_azerite_spell("Shredding Fury");
  azerite.wild_fleshrending = find_azerite_spell("Wild Fleshrending");
  azerite.jungle_fury = find_azerite_spell("Jungle Fury");
  azerite.untamed_ferocity = find_azerite_spell("Untamed Ferocity");

  // Guardian
  azerite.guardians_wrath = find_azerite_spell("Guardian's Wrath");
  azerite.layered_mane = find_azerite_spell("Layered Mane");
  azerite.masterful_instincts = find_azerite_spell("Masterful Instincts");
  azerite.twisted_claws = find_azerite_spell("Twisted Claws");
  azerite.burst_of_savagery = find_azerite_spell("Burst of Savagery");

  //Azerite essences
  auto essence_vop = find_azerite_essence("Vision of Perfection");
  vision_of_perfection_cdr = azerite::vision_of_perfection_cdr(essence_vop);
  vision_of_perfection_dur = essence_vop.spell(1u, essence_type::MAJOR)->effectN(1).percent()
    + essence_vop.spell(2u, essence_spell::UPGRADE, essence_type::MAJOR)->effectN(1).percent();

  lucid_dreams = find_azerite_essence("Memory of Lucid Dreams").spell(1u, essence_type::MINOR);

  azerite.conflict_and_strife = find_azerite_essence("Conflict and Strife");

  // Affinities =============================================================

  spec.feline_swiftness = find_affinity_spell( "Feline Swiftness" );
  spec.astral_influence = find_affinity_spell( "Astral Influence" );
  spec.thick_hide       = find_affinity_spell( "Thick Hide" );
  spec.yseras_gift      = find_affinity_spell( "Ysera's Gift" );

  // Masteries ==============================================================

  mastery.razor_claws         = find_mastery_spell( DRUID_FERAL );
  mastery.harmony             = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian    = find_mastery_spell( DRUID_GUARDIAN );
  mastery.natures_guardian_AP = check_spell( mastery.natures_guardian->ok(), 159195 );
  mastery.total_eclipse       = find_mastery_spell( DRUID_BALANCE );
}

// druid_t::init_base =======================================================

void druid_t::init_base_stats()
{
  // Set base distance based on spec
  if ( base.distance < 1 )
    base.distance = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? 5 : 30;

  player_t::init_base_stats();

  base.attack_power_per_agility = specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ? 1.0 : 0.0;
  base.spell_power_per_intellect = specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION ? 1.0 : 0.0;

  // Resources
  resources.base[ RESOURCE_RAGE ]         = 100;
  resources.base[ RESOURCE_COMBO_POINT ]  = 5;
  resources.base[ RESOURCE_ASTRAL_POWER ] = 100;
  resources.base[ RESOURCE_ENERGY ]       = 100 + talent.moment_of_clarity->effectN( 3 ).resource( RESOURCE_ENERGY ) +
                                      legendary.cateye_curio->effectN( 2 ).base_value();

  // only activate other resources if you have the affinity and affinity_resources = true
  resources.active_resource[ RESOURCE_HEALTH ]       = specialization() == DRUID_GUARDIAN ||
                                                     ( talent.guardian_affinity->ok() && affinity_resources );
  resources.active_resource[ RESOURCE_RAGE ]         = specialization() == DRUID_GUARDIAN ||
                                                     ( talent.guardian_affinity->ok() && affinity_resources );
  resources.active_resource[ RESOURCE_MANA ]         = specialization() == DRUID_RESTORATION ||
                                                     ( talent.restoration_affinity->ok() && affinity_resources );
  resources.active_resource[ RESOURCE_COMBO_POINT ]  = specialization() == DRUID_FERAL || specialization() == DRUID_RESTORATION ||
                                                     ( talent.feral_affinity->ok() && ( affinity_resources || catweave_bear ) );
  resources.active_resource[ RESOURCE_ENERGY ]       = specialization() == DRUID_FERAL || specialization() == DRUID_RESTORATION ||
                                                     ( talent.feral_affinity->ok() && ( affinity_resources || catweave_bear ) );
  resources.active_resource[ RESOURCE_ASTRAL_POWER ] = specialization() == DRUID_BALANCE;

  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;
  if ( specialization() == DRUID_FERAL )
  {
    resources.base_regen_per_second[ RESOURCE_ENERGY ] *=
        1.0 + query_aura_effect( spec.feral, A_MOD_POWER_REGEN_PERCENT, P_EFFECT_1 )->percent();
  }
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + talent.feral_affinity->effectN( 2 ).percent();

  base_gcd = 1.5_s;
}

void druid_t::init_assessors()
{
  player_t::init_assessors();

  if ( covenant.kyrian->ok() )
  {
    assessor_out_damage.add( assessor::TARGET_DAMAGE + 1, [this]( result_amount_type, action_state_t* s ) {
      auto pool = debug_cast<buffs::kindred_empowerment_buff_t*>( buff.kindred_empowerment );
      const auto s_action = s->action;

      // TODO: Confirm damage must come from an active action
      if ( !s_action->harmful || ( s_action->type != ACTION_SPELL && s_action->type != ACTION_ATTACK ) )
        return assessor::CONTINUE;

      if ( s->result_amount == 0 )
        return assessor::CONTINUE;

      // TODO: Confirm added damage doesn't add to the pool
      if ( s_action == active.kindred_empowerment )
        return assessor::CONTINUE;

      if ( buff.kindred_empowerment_energize->up() )
        pool->add_pool( s );

      if ( buff.kindred_empowerment->up() )
        pool->use_pool( s );

      return assessor::CONTINUE;
    } );
  }
}

// druid_t::init_buffs ======================================================

void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Generic druid buffs
  buff.bear_form    = make_buff<buffs::bear_form_t>( *this );
  buff.cat_form     = make_buff<buffs::cat_form_t>( *this );
  buff.moonkin_form = make_buff<buffs::moonkin_form_t>( *this );

  buff.dash = make_buff( this, "dash", find_class_spell( "Dash" ) )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );

  buff.tiger_dash = make_buff<tiger_dash_buff_t>( *this );

  buff.heart_of_the_wild = make_buff( this, "heart_of_the_wild",
      find_spell( as<unsigned>( query_aura_effect( talent.balance_affinity->ok() ? talent.balance_affinity
                                                : talent.feral_affinity->ok() ? talent.feral_affinity
                                                : talent.guardian_affinity->ok() ? talent.guardian_affinity
                                                : talent.restoration_affinity,
      A_OVERRIDE_ACTION_SPELL, talent.heart_of_the_wild->id() )->base_value() ) ) )
    ->set_cooldown( 0_ms );

  buff.innervate = make_buff<innervate_buff_t>( *this );

  buff.prowl = make_buff( this, "prowl", find_class_spell( "Prowl" ) );

  buff.thorns = make_buff( this, "thorns", find_spell( 305497 ) );

  buff.wild_charge_movement = make_buff( this, "wild_charge_movement" );

  // Generic legendaries
  buff.oath_of_the_elder_druid = make_buff( this, "oath_of_the_elder_druid", find_spell( 338643 ) )
    ->set_quiet( true );

  // Balance buffs
  // Default value is ONLY used for APL expression, so set via base_value() and not percent()
  buff.balance_of_all_things_arcane = make_buff( this, "balance_of_all_things_arcane", find_spell( 339946 ) )
    ->set_default_value_from_effect( 1, 1.0 )
    ->set_reverse( true );

  buff.balance_of_all_things_nature = make_buff( this, "balance_of_all_things_nature", find_spell( 339943 ) )
    ->set_default_value_from_effect( 1, 1.0 )
    ->set_reverse( true );

  buff.celestial_alignment = make_buff( this, "celestial_alignment", spec.celestial_alignment )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->modify_duration( conduit.precise_alignment.time_value() )
    ->add_invalidate( CACHE_HASTE )
    ->set_stack_change_callback( [this] ( buff_t* b, int, int new_ ) {
        if ( new_ )
        {
          this->eclipse_handler.trigger_both( b->buff_duration() );
          uptime.combined_ca_inc->update( true, sim->current_time() );
        }
        else
        {
          this->eclipse_handler.expire_both();
          uptime.combined_ca_inc->update( false, sim->current_time() );
        }
      } );

  buff.incarnation_moonkin = make_buff( this, "incarnation_chosen_of_elune", talent.incarnation_moonkin )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->modify_duration( conduit.precise_alignment.time_value() )
    ->add_invalidate( CACHE_CRIT_CHANCE )
    ->add_invalidate( CACHE_HASTE )
    ->set_stack_change_callback( [this] ( buff_t* b, int, int new_ ) {
        if ( new_ )
        {
          this->eclipse_handler.trigger_both( b->buff_duration() );
          uptime.combined_ca_inc->update( true, sim->current_time() );
        }
        else
        {
          this->eclipse_handler.expire_both();
          uptime.combined_ca_inc->update( false, sim->current_time() );
      }
      } );

  buff.eclipse_solar = make_buff<eclipse_buff_t>( *this, "eclipse_solar", spec.eclipse_solar );

  buff.eclipse_lunar = make_buff<eclipse_buff_t>( *this, "eclipse_lunar", spec.eclipse_lunar );

  buff.natures_balance = make_buff( this, "natures_balance", talent.natures_balance )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_tick_callback( [this]( buff_t*, int, timespan_t ) {
        resource_gain( RESOURCE_ASTRAL_POWER, talent.natures_balance->effectN( 1 ).resource( RESOURCE_ASTRAL_POWER ),
                        gain.natures_balance );
      } );

  buff.oneths_free_starsurge = make_buff( this, "oneths_clear_vision", find_spell( 339797 ) )
    ->set_chance( legendary.oneths_clear_vision->effectN( 1 ).percent() );

  buff.oneths_free_starfall = make_buff( this, "oneths_perception", find_spell( 339800 ) )
    ->set_chance( legendary.oneths_clear_vision->effectN( 1 ).percent() );

  buff.owlkin_frenzy = make_buff( this, "owlkin_frenzy", spec.owlkin_frenzy )
    ->set_chance( find_rank_spell( "Moonkin Form", "Rank 2" )->effectN( 1 ).percent() );

  buff.primordial_arcanic_pulsar = make_buff( this, "primordial_arcanic_pulsar", find_spell( 338825 ) );

  buff.solstice = make_buff( this, "solstice", talent.solstice->effectN( 1 ).trigger() );

  buff.starfall = make_buff( this, "starfall", spec.starfall )
    ->apply_affecting_aura( talent.stellar_drift )
    ->set_period( 1_s )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
    ->set_tick_zero( true );

  buff.starlord = make_buff( this, "starlord", find_spell( 279709 ) )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->add_invalidate( CACHE_HASTE );

  buff.starsurge = make_buff( this, "starsurge_empowerment", find_affinity_spell( "Starsurge" ) )
    ->set_cooldown( 0_ms )
    ->set_duration( 0_ms )
    ->set_max_stack( 30 );  // Arbitrary cap. Current max eclipse duration is 45s (15s base + 30s inc). Adjust if needed.

  buff.timeworn_dreambinder = make_buff( this, "timeworn_dreambinder", legendary.timeworn_dreambinder->effectN( 1 ).trigger() )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  buff.warrior_of_elune = make_buff<warrior_of_elune_buff_t>( *this );

  // Feral buffs
  buff.apex_predators_craving =
      make_buff( this, "apex_predators_craving", legendary.apex_predators_craving->effectN( 1 ).trigger() )
          ->set_chance( legendary.apex_predators_craving->effectN( 1 ).percent() );

  buff.berserk_cat = make_buff( this, "berserk_cat", spec.berserk_cat )
    ->set_cooldown( 0_ms );

  buff.incarnation_cat = make_buff( this, "incarnation_king_of_the_jungle", talent.incarnation_cat )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST );

  buff.jungle_stalker = make_buff( this, "jungle_stalker", talent.incarnation_cat->effectN( 3 ).trigger() );

  buff.bloodtalons     = make_buff( this, "bloodtalons", spec.bloodtalons );
  buff.bt_rake         = make_buff<bt_dummy_buff_t>( *this, "bt_rake" );
  buff.bt_shred        = make_buff<bt_dummy_buff_t>( *this, "bt_shred" );
  buff.bt_swipe        = make_buff<bt_dummy_buff_t>( *this, "bt_swipe" );
  buff.bt_thrash       = make_buff<bt_dummy_buff_t>( *this, "bt_thrash" );
  buff.bt_moonfire     = make_buff<bt_dummy_buff_t>( *this, "bt_moonfire" );
  buff.bt_brutal_slash = make_buff<bt_dummy_buff_t>( *this, "bt_brutal_slash" );

  // 1.05s ICD per https://github.com/simulationcraft/simc/commit/b06d0685895adecc94e294f4e3fcdd57ac909a10
  buff.clearcasting = make_buff( this, "clearcasting", spec.omen_of_clarity->effectN( 1 ).trigger() )
    ->set_chance( spec.omen_of_clarity->proc_chance() )
    ->set_cooldown( 1.05_s )
    ->modify_max_stack( as<int>( talent.moment_of_clarity->effectN( 1 ).base_value() ) );

  buff.eye_of_fearful_symmetry =
      make_buff( this, "eye_of_fearful_symmetry", legendary.eye_of_fearful_symmetry->effectN( 1 ).trigger() );
  buff.eye_of_fearful_symmetry->set_default_value(
      buff.eye_of_fearful_symmetry->data().effectN( 1 ).trigger()->effectN( 1 ).base_value() );

  buff.predatory_swiftness = make_buff( this, "predatory_swiftness", find_spell( 69369 ) )
    ->set_chance( spec.predatory_swiftness->ok() );

  buff.savage_roar = make_buff( this, "savage_roar", spec.savage_roar )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )  // Pandemic refresh is done by the action
    ->set_tick_behavior( buff_tick_behavior::NONE );

  buff.scent_of_blood = make_buff( this, "scent_of_blood", find_spell( 285646 ) )
    ->set_default_value( talent.scent_of_blood->effectN( 1 ).base_value() )
    ->set_max_stack( 10 );

  buff.sudden_ambush = make_buff( this, "sudden_ambush", conduit.sudden_ambush->effectN( 1 ).trigger() );

  buff.tigers_fury = make_buff<tigers_fury_buff_t>( *this );

  // Guardian buffs
  buff.berserk_bear = make_buff( this, "berserk_bear", spec.berserk_bear )
    ->set_cooldown( 0_ms );
  if ( legendary.legacy_of_the_sleeper->ok() )
    buff.berserk_bear->add_invalidate( CACHE_LEECH );
  if ( conduit.unchecked_aggression->ok() )
    buff.berserk_bear->add_invalidate( CACHE_HASTE );

  buff.incarnation_bear = make_buff<incarnation_bear_buff_t>( *this );

  buff.barkskin = make_buff( this, "barkskin", spec.barkskin )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
    ->set_tick_behavior( talent.brambles->ok() ? buff_tick_behavior::REFRESH : buff_tick_behavior::NONE )
    ->apply_affecting_aura( find_rank_spell( "Barkskin", "Rank 2") )
    ->set_tick_callback( [this]( buff_t*, int, timespan_t ) {
      if ( talent.brambles->ok() )
        active.brambles_pulse->execute();
      } );

  buff.bristling_fur = make_buff( this, "bristling_fur", talent.bristling_fur )
    ->set_cooldown( 0_ms );

  buff.earthwarden = make_buff( this, "earthwarden", find_spell( 203975 ) )
    ->set_default_value( talent.earthwarden->effectN( 1 ).percent() );

  buff.earthwarden_driver = make_buff( this, "earthwarden_driver", talent.earthwarden )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_tick_callback( [this]( buff_t*, int, timespan_t ) {
        buff.earthwarden->trigger();
      } );

  buff.galactic_guardian = make_buff( this, "galactic_guardian", find_spell( 213708 ) )
    ->set_chance( talent.galactic_guardian->ok() )
    ->set_default_value_from_effect( 1, 0.1 /*RESOURCE_RAGE*/ );

  buff.gore = make_buff( this, "gore", find_spell( 93622 ) )
    ->set_chance( spec.gore->effectN( 1 ).percent() )
    ->set_default_value_from_effect( 1, 0.1 /*RESOURCE_RAGE*/ );

  buff.guardian_of_elune = make_buff( this, "guardian_of_elune", talent.guardian_of_elune->effectN( 1 ).trigger() )
    ->set_chance( talent.guardian_of_elune->ok() )
    ->set_default_value( talent.guardian_of_elune->effectN( 1 ).trigger()->effectN( 1 ).time_value().total_seconds() );

  buff.ironfur = make_buff( this, "ironfur", spec.ironfur )
    ->set_default_value_from_effect_type( A_MOD_ATTACK_POWER_OF_STAT_PERCENT )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->apply_affecting_aura( find_rank_spell( "Ironfur", "Rank 2" ) )
    ->add_invalidate( CACHE_AGILITY )
    ->add_invalidate( CACHE_ARMOR );

  buff.pulverize = make_buff( this, "pulverize", talent.pulverize )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_IGNORE_DAMAGE_REDUCTION_SCHOOL )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.savage_combatant = make_buff( this, "savage_combatant", conduit.savage_combatant->effectN( 1 ).trigger() )
    ->set_default_value( conduit.savage_combatant.percent() );

  buff.sharpened_claws = make_buff( this, "sharpened_claws", find_spell( 279943 ) )
    ->set_default_value_from_effect( 1 );

  buff.survival_instincts = make_buff<survival_instincts_buff_t>( *this );

  // Restoration buffs
  buff.cenarion_ward = make_buff( this, "cenarion_ward", talent.cenarion_ward );

  buff.incarnation_tree = make_buff( this, "incarnation_tree_of_life", talent.incarnation_tree )
    ->set_cooldown( 0_ms )
    ->set_duration( 30_s )
    ->set_default_value_from_effect( 1 )
    ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buff.harmony = make_buff( this, "harmony", find_spell( 100977 ) );

  buff.soul_of_the_forest = make_buff( this, "soul_of_the_forest", find_spell( 114108 ) )
    ->set_default_value_from_effect( 1 );

  if ( specialization() == DRUID_RESTORATION || talent.restoration_affinity -> ok() )
  {
    buff.yseras_gift = make_buff( this, "yseras_gift_driver", spec.yseras_gift )
      ->set_quiet( true )
      ->set_tick_zero( true )
      ->set_tick_callback( [this]( buff_t*, int, timespan_t ) {
          active.yseras_gift->schedule_execute();
        } );
  }

  // Covenant
  buff.kindred_empowerment = make_buff<kindred_empowerment_buff_t>( *this );

  buff.kindred_empowerment_energize =
      make_buff( this, "kindred_empowerment_energize", covenant.kindred_empowerment_energize )
          ->set_stack_change_callback( [this]( buff_t*, int, int new_ ) {
            if ( !new_ )
              debug_cast<buffs::kindred_empowerment_buff_t*>( buff.kindred_empowerment )->snapshot();
          } );

  buff.convoke_the_spirits = make_buff( this, "convoke_the_spirits", covenant.night_fae )
    ->set_cooldown( 0_ms )
    ->set_period( 0_ms );
  if ( conduit.conflux_of_elements->ok() )
    buff.convoke_the_spirits->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.ravenous_frenzy = make_buff( this, "ravenous_frenzy", covenant.venthyr )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_period( 0_ms )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->add_invalidate( CACHE_HASTE )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  if ( conduit.endless_thirst->ok() )
    buff.ravenous_frenzy->add_invalidate( CACHE_CRIT_CHANCE );

  // Azerite
  buff.shredding_fury =
      make_buff( this, "shredding_fury", find_spell( 274426 ) )->set_default_value( azerite.shredding_fury.value() );

  buff.jungle_fury = make_buff<stat_buff_t>( this, "jungle_fury", find_spell( 274425 ) )
                         ->add_stat( STAT_CRIT_RATING, azerite.jungle_fury.value( 1 ) )
                         ->set_chance( azerite.jungle_fury.enabled() ? 1.0 : 0.0 );

  buff.iron_jaws = make_buff( this, "iron_jaws", find_spell( 276026 ) );

  buff.raking_ferocity = make_buff( this, "raking_ferocity", find_spell( 273340 ) );

  buff.dawning_sun =
      make_buff( this, "dawning_sun", azerite.dawning_sun.spell()->effectN( 1 ).trigger()->effectN( 1 ).trigger() );

  buff.lively_spirit = make_buff<stat_buff_t>( this, "lively_spirit", find_spell( 279648 ) )
                           ->add_stat( STAT_INTELLECT, azerite.lively_spirit.value() );

  buff.arcanic_pulsar = make_buff( this, "arcanic_pulsar",
                                   azerite.arcanic_pulsar.spell()->effectN( 1 ).trigger()->effectN( 1 ).trigger() );

  buff.guardians_wrath = make_buff( this, "guardians_wrath", find_spell( 279541 ) )
                             ->set_default_value( find_spell( 279541 )->effectN( 1 ).resource( RESOURCE_RAGE ) );

  buff.masterful_instincts = make_buff<stat_buff_t>( this, "masterful_instincts", find_spell( 273349 ) )
                                 ->add_stat( STAT_MASTERY_RATING, azerite.masterful_instincts.value( 1 ) )
                                 ->add_stat( STAT_ARMOR, azerite.masterful_instincts.value( 2 ) );

  buff.twisted_claws = make_buff<stat_buff_t>( this, "twisted_claws", find_spell( 275909 ) )
                           ->add_stat( STAT_AGILITY, azerite.twisted_claws.value( 1 ) )
                           ->set_chance( find_spell( 275908 )->proc_chance() );

  buff.burst_of_savagery = make_buff<stat_buff_t>( this, "burst_of_savagery", find_spell( 289315 ) )
                               ->add_stat( STAT_MASTERY_RATING, azerite.burst_of_savagery.value( 1 ) );

  player_t::buffs.memory_of_lucid_dreams->set_affects_regen( true );
}

void druid_t::create_actions()
{
  caster_melee_attack = new caster_attacks::druid_melee_t( this );

  if ( !cat_melee_attack )
  {
    init_beast_weapon( cat_weapon, 1.0 );
    cat_melee_attack = new cat_attacks::cat_melee_t( this );
  }

  if ( !bear_melee_attack )
  {
    init_beast_weapon( bear_weapon, 2.5 );
    bear_melee_attack = new bear_attacks::bear_melee_t( this );
  }

  if ( talent.cenarion_ward->ok() )
    active.cenarion_ward_hot = new heals::cenarion_ward_hot_t( this );

  if ( spec.yseras_gift )
    active.yseras_gift = new heals::yseras_gift_t( this );

  if ( talent.brambles->ok() )
  {
    active.brambles       = new spells::brambles_t( this );
    active.brambles_pulse = new spells::brambles_pulse_t( this );

    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>(
        talent.brambles->id(), instant_absorb_t( this, talent.brambles, "brambles", &brambles_handler ) ) );
  }

  if ( talent.galactic_guardian->ok() )
  {
    active.galactic_guardian        = new spells::moonfire_t::galactic_guardian_damage_t( this );
    active.galactic_guardian->stats = get_stats( "moonfire" );
  }

  if ( mastery.natures_guardian->ok() )
    active.natures_guardian = new heals::natures_guardian_t( this );

  if ( azerite.lunar_shrapnel.ok() )
    active.lunar_shrapnel = new spells::lunar_shrapnel_t( this );

  if ( azerite.streaking_stars.ok() )
    active.streaking_stars = new spells::streaking_stars_t( this );

  if ( covenant.kyrian->ok() )
  {
    active.kindred_empowerment         = new spells::kindred_empowerment_t( this, "kindred_empowerment" );
    active.kindred_empowerment_partner = new spells::kindred_empowerment_t( this, "kindred_empowerment_partner" );
  }

  if ( legendary.frenzyband->ok() )
    active.frenzied_assault = new cat_attacks::frenzied_assault_t( this );

  if ( legendary.oneths_clear_vision->ok() )
    active.oneths_clear_vision =
        new action_t( action_e::ACTION_OTHER, "oneths_clear_vision", this, legendary.oneths_clear_vision );

  if ( legendary.lycaras_fleeting_glimpse->ok() )
    active.lycaras_fleeting_glimpse = new lycaras_fleeting_glimpse_t( this );

  player_t::create_actions();
}

// ALL Spec Pre-Combat Action Priority List =================================
std::string druid_t::default_flask() const
{
  switch ( specialization() )
  {
    case DRUID_FERAL:
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
    case DRUID_GUARDIAN:
    default: return "disabled";
  }
}
std::string druid_t::default_potion() const
{
  switch ( specialization() )
  {
    case DRUID_FERAL:
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
    case DRUID_GUARDIAN:
    default: return "disabled";
  }
}

std::string druid_t::default_food() const
{
  switch ( specialization() )
  {
    case DRUID_FERAL:
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
    case DRUID_GUARDIAN:
    default: return "disabled";
  }
}

std::string druid_t::default_rune() const
{
  switch ( specialization() )
  {
    case DRUID_FERAL:
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
    case DRUID_GUARDIAN:
    default: return "disabled";
  }
}

void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Consumables
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( specialization() == DRUID_FERAL )
  {
    precombat->add_action( this, "Cat Form" );
  }

  if ( specialization() == DRUID_BALANCE )
  {
    precombat->add_action( this, "Moonkin Form" );
  }

  if ( specialization() == DRUID_GUARDIAN )
  {
    if ( catweave_bear && talent.feral_affinity->ok() )
    {
      precombat->add_action( this, "Cat Form" );
    }
    else
    {
      precombat->add_action( this, "Bear Form" );
    }
  }

  if ( specialization() == DRUID_RESTORATION )
  {
    if ( talent.feral_affinity->ok() )
    {
      precombat->add_action( this, "Cat Form" );
    }
    if ( talent.balance_affinity->ok() )
    {
      precombat->add_action( this, "Moonkin Form" );
    }
  }

  precombat->add_action( "potion" );
}

// NO Spec Combat Action Priority List ======================================

void druid_t::apl_default()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // Assemble Racials / On-Use Items / Professions
  std::string extra_actions = "";

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    extra_actions += add_action( racial_actions[ i ] );

  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    extra_actions += add_action( item_actions[ i ] );

  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    extra_actions += add_action( profession_actions[ i ] );

  def->add_action( extra_actions );
}

// Feral Combat Action Priority List ========================================

void druid_t::apl_feral()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  def->add_action( "auto_attack" );
  def->add_action( this, "Shred" );
}

// Balance Combat Action Priority List ======================================

void druid_t::apl_balance()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  def->add_action( this, "Wrath" );
}

// Guardian Combat Action Priority List =====================================

void druid_t::apl_guardian()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  def->add_action( "auto_attack" );
  def->add_action( this, "Swipe" );
}

// Restoration Combat Action Priority List ==================================

void druid_t::apl_restoration()
{
  action_priority_list_t* def     = get_action_priority_list( "default" );
  action_priority_list_t* balance = get_action_priority_list( "balance" );

  def->add_action( "run_action_list,name=balance,if=talent.balance_affinity.enabled" );

  balance->add_action( this, "Moonfire", "target_if=refreshable" );
  balance->add_action( this, "Sunfire", "target_if=refreshable" );
  balance->add_talent( this, "Heart of the Wild" );
  balance->add_action( "convoke_the_spirits,if=buff.eclipse_solar.up" );
  balance->add_action( this, "Starsurge" );
  balance->add_action( this, "Wrath", "if=buff.eclipse_solar.up|eclipse.lunar_next" );
  balance->add_action( this, "Starfire" );
}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  scaling->disable( STAT_STRENGTH );

  // workaround for resto dps scaling
  if ( specialization() == DRUID_RESTORATION && role == ROLE_ATTACK )
  {
    scaling->disable( STAT_AGILITY );
    scaling->disable( STAT_MASTERY_RATING );
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

  // if ( specialization() == DRUID_RESTORATION )
  // sim -> errorf( "%s is using an unsupported spec.", name() );
}

// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  // General
  gain.lucid_dreams = get_gain( "lucid_dreams" );

  if ( specialization() == DRUID_BALANCE )
  {
    gain.natures_balance           = get_gain( "natures_balance" );
    gain.force_of_nature           = get_gain( "force_of_nature" );
    gain.fury_of_elune             = get_gain( "fury_of_elune" );
    gain.stellar_flare             = get_gain( "stellar_flare" );
    gain.starfire                  = get_gain( "starfire" );
    gain.moonfire                  = get_gain( "moonfire" );
    gain.shooting_stars            = get_gain( "shooting_stars" );
    gain.wrath                     = get_gain( "wrath" );
    gain.sunfire                   = get_gain( "sunfire" );
    gain.celestial_alignment       = get_gain( "celestial_alignment" );
    gain.incarnation               = get_gain( "incarnation" );
    gain.primordial_arcanic_pulsar = get_gain( "primordial_arcanic_pulsar" );
    gain.arcanic_pulsar            = get_gain( "arcanic_pulsar" );
    gain.vision_of_perfection      = get_gain( "vision_of_perfection" );
  }
  else if ( specialization() == DRUID_FERAL )
  {
    gain.brutal_slash            = get_gain( "brutal_slash" );
    gain.energy_refund           = get_gain( "energy_refund" );
    gain.feral_frenzy            = get_gain( "feral_frenzy" );
    gain.primal_fury             = get_gain( "primal_fury" );
    gain.rake                    = get_gain( "rake" );
    gain.shred                   = get_gain( "shred" );
    gain.swipe_cat               = get_gain( "swipe_cat" );
    gain.tigers_fury             = get_gain( "tigers_fury" );
    gain.berserk                 = get_gain( "berserk" );
    gain.cateye_curio            = get_gain( "cateye_curio" );
    gain.eye_of_fearful_symmetry = get_gain( "eye_of_fearful_symmetry" );
  }
  else if ( specialization() == DRUID_GUARDIAN )
  {
    gain.bear_form         = get_gain( "bear_form" );
    gain.blood_frenzy      = get_gain( "blood_frenzy" );
    gain.brambles          = get_gain( "brambles" );
    gain.bristling_fur     = get_gain( "bristling_fur" );
    gain.galactic_guardian = get_gain( "galactic_guardian" );
    gain.gore              = get_gain( "gore" );
    gain.rage_refund       = get_gain( "rage_refund" );
    gain.rage_from_melees  = get_gain( "rage_from_melees" );
  }

  // Multi-spec
  if ( specialization() == DRUID_FERAL || specialization() == DRUID_RESTORATION )
  {
    gain.clearcasting = get_gain( "clearcasting" );  // Feral & Restoration
  }
  if ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN )
  {
    gain.soul_of_the_forest = get_gain( "soul_of_the_forest" );  // Feral & Guardian
  }
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  // General
  proc.vision_of_perfection  = get_proc( "Vision of Perfection" );  //->collect_count()->collect_interval();

  // Balance
  proc.power_of_the_moon     = get_proc( "Power of the Moon" );     //->collect_count();
  proc.arcanic_pulsar        = get_proc( "Arcanic Pulsar Proc" );   //->collect_interval();
  proc.streaking_star        = get_proc( "Streaking Stars" );       //->collect_count();
  proc.wasted_streaking_star = get_proc( "Wasted Streaking Stars" );

  // Feral
  proc.predator              = get_proc( "predator" );
  proc.predator_wasted       = get_proc( "predator_wasted" );
  proc.primal_fury           = get_proc( "primal_fury" );
  proc.blood_mist            = get_proc( "blood_mist" );
  proc.gushing_lacerations   = get_proc( "gushing_lacerations" );
  proc.clearcasting          = get_proc( "clearcasting" );
  proc.clearcasting_wasted   = get_proc( "clearcasting_wasted" );

  // Guardian
  proc.gore                  = get_proc( "gore" );
}

// druid_t::init_uptimes ====================================================

void druid_t::init_uptimes()
{
  player_t::init_uptimes();

  uptime.eclipse = get_uptime( "Eclipse" );

  if ( talent.incarnation_moonkin->ok() )
  {
    uptime.arcanic_pulsar       = get_uptime( "Incarnation (Pulsar)" );
    uptime.vision_of_perfection = get_uptime( "Incarnation (Vision)" );//->collect_uptime();
    uptime.combined_ca_inc      = get_uptime( "Incarnation (Total)" );//->collect_uptime()->collect_duration();
  }
  else
  {
    uptime.arcanic_pulsar       = get_uptime( "Celestial Alignment (Pulsar)" );
    uptime.vision_of_perfection = get_uptime( "Celestial Alignment (Vision)" );//->collect_uptime();
    uptime.combined_ca_inc      = get_uptime( "Celestial Alignment (Total)" );//->collect_uptime()->collect_duration();
  }
}

// druid_t::init_resources ==================================================

void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ]         = 0;
  resources.current[ RESOURCE_COMBO_POINT ]  = 0;
  resources.current[ RESOURCE_ASTRAL_POWER ] =
      std::max( talent.natures_balance->ok() ? 50.0 : 0.0, initial_astral_power );
  expected_max_health = calculate_expected_max_health();
}

// druid_t::init_rng ========================================================

void druid_t::init_rng()
{
  // RPPM objects
  rppm.predator = get_rppm( "predator", predator_rppm_rate );  // Predator: optional RPPM approximation.
  rppm.blood_mist =
      get_rppm( "blood_mist", find_spell( azerite.blood_mist.spell()->effectN( 1 ).trigger_spell_id() )->real_ppm() );
  rppm.power_of_the_moon =
      get_rppm( "power_of_the_moon", azerite.power_of_the_moon.spell()->effectN( 1 ).trigger()->real_ppm() );

  player_t::init_rng();
}

// druid_t::init_actions ====================================================

void druid_t::init_action_list()
{
#ifdef NDEBUG  // Only restrict on release builds.
  // Restoration isn't fully supported atm
  if ( specialization() == DRUID_RESTORATION && role != ROLE_ATTACK )
  {
    if ( !quiet )
      sim->errorf( "Druid restoration healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }
#endif
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  apl_precombat();  // PRE-COMBAT

  switch ( specialization() )
  {
    case DRUID_FERAL: apl_feral(); break;
    case DRUID_BALANCE: apl_balance(); break;
    case DRUID_GUARDIAN: apl_guardian(); break;
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
  form                     = NO_FORM;
  moon_stage               = (moon_stage_e)initial_moon_stage;
  previous_streaking_stars = SS_NONE;
  eclipse_handler.reset_stacks();
  eclipse_handler.reset_state();

  base_gcd = timespan_t::from_seconds( 1.5 );

  // Restore main hand attack / weapon to normal state
  main_hand_attack = caster_melee_attack;
  main_hand_weapon = caster_form_weapon;

  // Reset any custom events to be safe.
  persistent_buff_delay.clear();

  if ( mastery.natures_guardian->ok() )
    recalculate_resource_max( RESOURCE_HEALTH );
}

// druid_t::merge ===========================================================

void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ]->merge( *od.counters[ i ] );
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

  if ( r == RESOURCE_ENERGY )
  {
    if ( buff.savage_roar->check() )
      reg *= 1.0 + spec.savage_roar->effectN( 3 ).percent();

    if ( player_t::buffs.memory_of_lucid_dreams->check() )
      reg *= 1.0 + player_t::buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();
  }

  return reg;
}

// druid_t::resource_gain ===================================================

double druid_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  if ( resource_type == RESOURCE_RAGE && player_t::buffs.memory_of_lucid_dreams->up() )
    amount *= 1.0 + player_t::buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();

  return player_t::resource_gain( resource_type, amount, source, action );
}

// druid_t::available =======================================================

timespan_t druid_t::available() const
{
  if ( primary_resource() != RESOURCE_ENERGY )
    return timespan_t::from_seconds( 0.1 );

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 )
    return timespan_t::from_seconds( 0.1 );

  return std::max( timespan_t::from_seconds( ( 25 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) ),
                   timespan_t::from_seconds( 0.1 ) );
}

// druid_t::arise ===========================================================

void druid_t::arise()
{
  player_t::arise();

  if ( talent.earthwarden->ok() )
    buff.earthwarden->trigger( buff.earthwarden->max_stack() );

  if ( talent.natures_balance->ok() )
    buff.natures_balance->trigger();

  // Trigger persistent buffs
  if ( buff.yseras_gift )
    persistent_buff_delay.push_back( make_event<persistent_buff_delay_event_t>( *sim, this, buff.yseras_gift ) );

  if ( talent.earthwarden->ok() )
    persistent_buff_delay.push_back( make_event<persistent_buff_delay_event_t>( *sim, this, buff.earthwarden_driver ) );

  // Conflict major rank 3 buff to double the minor vers buff
  if ( azerite.conflict_and_strife.is_major() && azerite.conflict_and_strife.rank() >= 3 )
    buff.strife_doubled = buff_t::find( this, "conflict_vers" );
}

void druid_t::combat_begin()
{
  player_t::combat_begin();

  if ( legendary.lycaras_fleeting_glimpse->ok() )
  {
    make_event<lycaras_fleeting_glimpse_event_t>(
        *sim, this, timespan_t::from_seconds( legendary.lycaras_fleeting_glimpse->effectN( 1 ).base_value() ) );
  }
}

// druid_t::recalculate_resource_max ========================================

void druid_t::recalculate_resource_max( resource_e rt, gain_t* source )
{
  double pct_health = 0, current_health = 0;
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

    if ( sim->log )
      sim->out_log.printf( "%s recalculates maximum health. old_current=%.0f new_current=%.0f net_health=%.0f", name(),
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

// druid_t::composite_melee_attack_power ===============================

double druid_t::composite_melee_attack_power() const
{
  const spell_data_t* spec_spell = nullptr;

  if ( specialization() == DRUID_BALANCE )
    spec_spell = spec.balance;
  else if ( specialization() == DRUID_RESTORATION )
    spec_spell = spec.restoration;

  if ( spec_spell )
    return query_aura_effect( spec_spell, A_404 )->percent() * cache.spell_power( SCHOOL_MAX ) *
           composite_spell_power_multiplier();

  return player_t::composite_melee_attack_power();
}

double druid_t::composite_melee_attack_power( attack_power_type type ) const
{
  const spell_data_t* spec_spell = nullptr;

  if ( specialization() == DRUID_BALANCE )
    spec_spell = spec.balance;
  else if ( specialization() == DRUID_RESTORATION )
    spec_spell = spec.restoration;

  if ( spec_spell )
    return query_aura_effect( spec_spell, A_404 )->percent() * cache.spell_power( SCHOOL_MAX ) *
           composite_spell_power_multiplier();

  return player_t::composite_melee_attack_power( type );
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier() const
{
  if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
    return 1.0;

  double ap = player_t::composite_attack_power_multiplier();

  // All modifiers MUST invalidate CACHE_ATTACK_POWER or Nurturing Instinct will break.

  if ( mastery.natures_guardian->ok() )
    ap *= 1.0 + cache.mastery() * mastery.natures_guardian_AP->effectN( 1 ).mastery_value();

  return ap;
}

// druid_t::composite_armor =================================================

double druid_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buff.ironfur->up() )
    a += ( buff.ironfur->check_stack_value() * cache.agility() );

  return a;
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form->check() )
    a *= 1.0 + buff.bear_form->data().effectN( 4 ).percent();

  return a;
}

// druid_t::temporary_movement_modifier =====================================

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

// druid_t::passive_movement_modifier =======================================

double druid_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buff.cat_form->up() )
    ms += spec.cat_form_speed->effectN( 1 ).percent();

  ms += spec.feline_swiftness->effectN( 1 ).percent();

  return ms;
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

// druid_t::composite_rating_multiplier ====================================

double druid_t::composite_rating_multiplier( rating_e rating ) const
{
  double rm = player_t::composite_rating_multiplier( rating );

  return rm;
}

// druid_t::composite_melee_expertise( weapon_t* ) ==========================

double druid_t::composite_melee_expertise( const weapon_t* ) const
{
  double exp = player_t::composite_melee_expertise();

  if ( buff.bear_form->check() )
    exp += buff.bear_form->data().effectN( 6 ).base_value();

  return exp;
}

// druid_t::composite_melee_crit_chance ============================================

double druid_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spec.critical_strikes->effectN( 1 ).percent();

  return crit;
}

// druid_t::composite_spell_crit_chance ============================================

double druid_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spec.critical_strikes->effectN( 1 ).percent();

  return crit;
}

// druid_t::composite_spell_haste ===========================================

double druid_t::composite_spell_haste() const
{
  double sh = player_t::composite_spell_haste();

  sh *= 1.0 / ( 1.0 + buff.starlord->stack_value() );

  sh *= 1.0 / ( 1.0 + buff.ravenous_frenzy->stack_value() );

  sh *= 1.0 / ( 1.0 + buff.celestial_alignment->stack_value() );

  sh *= 1.0 / ( 1.0 + buff.incarnation_moonkin->stack_value() );

  if ( conduit.unchecked_aggression->ok() && buff.berserk_bear->check() )
    sh *= 1.0 / ( 1.0 + conduit.unchecked_aggression.percent() );

  return sh;
}

// druid_t::composite_meele_haste ===========================================

double druid_t::composite_melee_haste() const
{
  double mh = player_t::composite_melee_haste();

  mh *= 1.0 / ( 1.0 + buff.starlord->stack_value() );

  mh *= 1.0 / ( 1.0 + buff.ravenous_frenzy->stack_value() );

  mh *= 1.0 / ( 1.0 + buff.celestial_alignment->stack_value() );

  mh *= 1.0 / ( 1.0 + buff.incarnation_moonkin->stack_value() );

  if ( conduit.unchecked_aggression->ok() && buff.berserk_bear->check() )
    mh *= 1.0 / ( 1.0 + conduit.unchecked_aggression.percent() );

  return mh;
}

// druid_t::composite_spell_power ===========================================

double druid_t::composite_spell_power( school_e school ) const
{
  double sp       = 0.0;
  double ap_coeff = 0.0;

  if ( specialization() == DRUID_GUARDIAN )
    ap_coeff = query_aura_effect( spec.guardian, A_366 )->percent();
  else if ( specialization() == DRUID_FERAL )
    ap_coeff = query_aura_effect( spec.feral, A_366 )->percent();

  if ( ap_coeff > 0 )
  {
    double weapon_sp = 0.0;

    if ( buff.cat_form->check() )
      weapon_sp = cat_weapon.dps * WEAPON_POWER_COEFFICIENT;
    else if ( buff.bear_form->check() )
      weapon_sp = bear_weapon.dps * WEAPON_POWER_COEFFICIENT;
    else
      weapon_sp = main_hand_weapon.dps * WEAPON_POWER_COEFFICIENT;

    sp += composite_attack_power_multiplier() * ( cache.attack_power() + weapon_sp ) * ap_coeff;
  }

  sp += player_t::composite_spell_power( school );

  return sp;
}

// druid_t::composite_spell_power_multiplier ================================

double druid_t::composite_spell_power_multiplier() const
{
  if ( specialization() == DRUID_GUARDIAN || specialization() == DRUID_FERAL)
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// druid_t::composite_attribute =============================================

double druid_t::composite_attribute( attribute_e attr ) const
{
  double a = player_t::composite_attribute( attr );

  switch ( attr )
  {
    case ATTR_AGILITY:
      if ( buff.ironfur->up() && azerite.layered_mane.ok() )
        a += azerite.layered_mane.value( 1 ) * buff.ironfur->stack();
      break;
    default: break;
  }

  return a;
}

// druid_t::composite_attribute_multiplier ==================================

double druid_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  switch ( attr )
  {
    case ATTR_STAMINA:
      if ( buff.bear_form->check() )
        m *= 1.0 + spec.bear_form->effectN( 2 ).percent() + spec.bear_form_2->effectN( 1 ).percent();
      break;
    default: break;
  }

  return m;
}

// druid_t::matching_gear_multiplier ========================================

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

// druid_t::composite_crit_avoidance ========================================

double druid_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  if ( buff.bear_form->check() )
    c += buff.bear_form->data().effectN( 3 ).percent();

  return c;
}

// druid_t::composite_dodge =================================================

double druid_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  return d;
}

// druid_t::composite_dodge_rating ==========================================

double druid_t::composite_dodge_rating() const
{
  double dr = player_t::composite_dodge_rating();

  // FIXME: Spell dataify me.
  if ( specialization() == DRUID_GUARDIAN )
    dr += composite_rating( RATING_MELEE_CRIT );

  return dr;
}

// druid_t::composite_damage_versatility_rating ==========================================

double druid_t::composite_damage_versatility_rating() const
{
  double cdvr = player_t::composite_damage_versatility_rating();

  return cdvr;
}

// druid_t::composite_heal_versatility_rating ==========================================

double druid_t::composite_heal_versatility_rating() const
{
  double chvr = player_t::composite_heal_versatility_rating();

  return chvr;
}

// druid_t::composite_mitigation_versatility_rating ==========================================

double druid_t::composite_mitigation_versatility_rating() const
{
  double cmvr = player_t::composite_mitigation_versatility_rating();

  return cmvr;
}

// druid_t::composite_leech =================================================

double druid_t::composite_leech() const
{
  double l = player_t::composite_leech();

  if ( legendary.legacy_of_the_sleeper->ok() && ( buff.berserk_bear->check() || buff.incarnation_bear->check() ) )
    l *= 1.0 + spec.berserk_bear->effectN( 8 ).percent();

  return l;
}

// druid_t::create_expression ===============================================

std::unique_ptr<expr_t> druid_t::create_expression( util::string_view name_str )
{
  auto splits = util::string_split<util::string_view>( name_str, "." );

  if ( splits[ 0 ] == "druid" &&
       ( splits[ 2 ] == "ticks_gained_on_refresh" || splits[ 2 ] == "ticks_gained_on_refresh_pmultiplier" ) )
  {
    // Since we know some action names don't map to the actual dot portion, lets add some exceptions
    // this may have to be made more robust if other specs are interested in using it, but for now lets
    // default any ambiguity to what would make most sense for ferals.
    if ( splits[ 1 ] == "rake" )
      splits[ 1 ] = "rake_bleed";
    if ( specialization() == DRUID_FERAL && splits[ 1 ] == "moonfire" )
      splits[ 1 ] = "lunar_inspiration";

    bool pmult_adjusted = false;

    if ( splits[ 2 ] == "ticks_gained_on_refresh_pmultiplier" )
      pmult_adjusted = true;

    action_t* action = find_action( splits[ 1 ] );
    if ( action )
      return make_fn_expr( name_str, [action, pmult_adjusted]() -> double {
        dot_t* dot             = action->get_dot();
        double remaining_ticks = 0;
        double potential_ticks = 0;
        action_state_t* state  = action->get_state( dot->state );
        timespan_t duration    = action->composite_dot_duration( state );
        state->target = action->target = action->player->target;
        timespan_t ttd                 = action->target->time_to_percent( 0 );
        double pmult                   = 0;
        // action->snapshot_state(state, result_amount_type::DMG_OVER_TIME);

        if ( dot->is_ticking() )
        {
          remaining_ticks = std::min( dot->remains(), ttd ) / dot->current_action->tick_time( dot->state );
          duration        = action->calculate_dot_refresh_duration( dot, duration );
          remaining_ticks *= pmult_adjusted ? dot->state->persistent_multiplier : 1.0;
        }

        if ( pmult_adjusted )
        {
          action->snapshot_state( state, result_amount_type::NONE );
          state->target = action->target;

          pmult = action->composite_persistent_multiplier( state );
        }

        potential_ticks = std::min( duration, ttd ) / ( action->tick_time( state ) );
        potential_ticks *= pmult_adjusted ? pmult : 1.0;

        action_state_t::release( state );
        return potential_ticks - remaining_ticks;
      } );
    throw std::invalid_argument( "invalid action" );
  }

  if ( splits[ 0 ] == "action" && splits[ 1 ] == "ferocious_bite_max" && splits[ 2 ] == "damage" )
  {
    action_t* action = find_action( "ferocious_bite" );

    return make_fn_expr( name_str, [action, this]() -> double {
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

  if ( util::str_compare_ci( name_str, "combo_points" ) )
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );

  if ( specialization() == DRUID_BALANCE )
  {
    if ( util::str_compare_ci( name_str, "astral_power" ) )
      return make_ref_expr( name_str, resources.current[ RESOURCE_ASTRAL_POWER ] );

    // New Moon stage related expressions
    if ( util::str_compare_ci( name_str, "new_moon" ) )
      return make_fn_expr( name_str, [this]() { return moon_stage == NEW_MOON; } );
    else if ( util::str_compare_ci( name_str, "half_moon" ) )
      return make_fn_expr( name_str, [this]() { return moon_stage == HALF_MOON; } );
    else if ( util::str_compare_ci( name_str, "full_moon" ) )
      return make_fn_expr( name_str, [this]() { return moon_stage == FULL_MOON; } );

    // automatic resolution of Celestial Alignment vs talented Incarnation
    if ( splits.size() == 3 && util::str_compare_ci( splits[ 1 ], "ca_inc" ) )
    {
      std::string replacement;

      if ( util::str_compare_ci( splits[ 0 ], "cooldown" ) )
        replacement = talent.incarnation_moonkin->ok() ? ".incarnation." : ".celestial_alignment.";
      else
        replacement = talent.incarnation_moonkin->ok() ? ".incarnation_chosen_of_elune." : ".celestial_alignment.";

      return player_t::create_expression( fmt::format( "{}{}{}", splits[ 0 ], replacement, splits[ 2 ] ) );
    }

    // check for AP overcap on action other than current action. check for current action handled in
    // druid_spell_t::create_expression syntax: <action>.ap_check.<allowed overcap = 0>
    if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "ap_check" ) )
    {
      action_t* action = find_action( splits[ 0 ] );
      if ( action )
      {
        return make_fn_expr( name_str, [action, this, splits]() {
          action_state_t* state = action->get_state();
          double ap             = resources.current[ RESOURCE_ASTRAL_POWER ];
          ap += action->composite_energize_amount( state );
          ap += spec.shooting_stars_dmg->effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
          ap += talent.natures_balance->ok() ? std::ceil( action->time_to_execute / 2_s ) : 0;
          action_state_t::release( state );
          return ap <=
                 resources.base[ RESOURCE_ASTRAL_POWER ] + ( splits.size() >= 3 ? util::to_int( splits[ 2 ] ) : 0 );
        } );
      }

      throw std::invalid_argument( "invalid action" );
    }
  }

  if ( specialization() == DRUID_BALANCE || talent.balance_affinity->ok() )
  {
    if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "eclipse" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "state" ) )
      {
        return make_fn_expr( name_str, [this]() {
          eclipse_state_e state = eclipse_handler.state;
          if ( state == ANY_NEXT )
            return 0;
          if ( eclipse_handler.state == SOLAR_NEXT )
            return 1;
          if ( eclipse_handler.state == LUNAR_NEXT )
            return 2;
          else
            return 3;
        } );
      }
      else if ( util::str_compare_ci( splits[ 1 ], "any_next" ) )
        return make_fn_expr( name_str, [this]() { return eclipse_handler.state == ANY_NEXT; } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_solar" ) )
        return make_fn_expr( name_str, [this]() { return eclipse_handler.state == IN_SOLAR; } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_lunar" ) )
        return make_fn_expr( name_str, [this]() { return eclipse_handler.state == IN_LUNAR; } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_both" ) )
        return make_fn_expr( name_str, [this]() { return eclipse_handler.state == IN_BOTH; } );
      else if ( util::str_compare_ci( splits[ 1 ], "solar_next" ) )
        return make_fn_expr( name_str, [this]() { return eclipse_handler.state == SOLAR_NEXT; } );
      else if ( util::str_compare_ci( splits[ 1 ], "lunar_next" ) )
        return make_fn_expr( name_str, [this]() { return eclipse_handler.state == LUNAR_NEXT; } );
    }
  }

  if ( util::str_compare_ci( name_str, "buff.starfall.refreshable" ) )
  {
    return make_fn_expr( name_str, [this]() {
      return !buff.starfall->check() || buff.starfall->remains() <= spec.starfall->duration() * 0.3;
    } );
  }

  // Convert talent.incarnation.* & buff.incarnation.* to spec-based incarnations. cooldown.incarnation.* doesn't need
  // name conversion.
  if ( splits.size() == 3 && util::str_compare_ci( splits[ 1 ], "incarnation" ) &&
       ( util::str_compare_ci( splits[ 0 ], "buff" ) || util::str_compare_ci( splits[ 0 ], "talent" ) ) )
  {
    std::string replacement_name;
    switch ( specialization() )
    {
      case DRUID_BALANCE: replacement_name = ".incarnation_chosen_of_elune."; break;
      case DRUID_FERAL: replacement_name = ".incarnation_king_of_the_jungle."; break;
      case DRUID_GUARDIAN: replacement_name = ".incarnation_guardian_of_ursoc."; break;
      case DRUID_RESTORATION: replacement_name = ".incarnation_tree_of_life."; break;
      default: break;
    }
    return player_t::create_expression( fmt::format("{}{}{}", splits[ 0 ], replacement_name, splits[ 2 ]) );
  }

  if ( splits.size() == 3 && util::str_compare_ci( splits[ 1 ], "berserk" ) )
  {
    std::string replacement;
    switch ( specialization() )
    {
      case DRUID_FERAL: replacement = ".berserk_cat."; break;
      case DRUID_GUARDIAN: replacement = ".berserk_bear."; break;
      default: break;
    }
    return player_t::create_expression( fmt::format( "{}{}{}", splits[ 0 ], replacement, splits[ 2 ] ) );
  }

  return player_t::create_expression( name_str );
}

// druid_t::create_options ==================================================

void druid_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "predator_rppm", predator_rppm_rate ) );
  add_option( opt_float( "initial_astral_power", initial_astral_power ) );
  add_option( opt_int( "initial_moon_stage", initial_moon_stage ) );
  add_option( opt_int( "lively_spirit_stacks", lively_spirit_stacks ) );
  add_option( opt_bool( "catweave_bear", catweave_bear ) );
  add_option( opt_bool( "affinity_resources", affinity_resources ) );
  add_option( opt_float( "thorns_attack_period", thorns_attack_period ) );
  add_option( opt_float( "thorns_hit_chance", thorns_hit_chance ) );
  add_option( opt_float( "kindred_empowerment_ratio", kindred_empowerment_ratio ) );
  add_option( opt_float( "convoke_the_spirits_heals", convoke_the_spirits_heals ) );
  add_option( opt_float( "convoke_the_spirits_ultimate", convoke_the_spirits_ultimate ) );
}

// druid_t::create_profile ==================================================

std::string druid_t::create_profile( save_e type )
{
  return player_t::create_profile( type );
}

// druid_t::primary_role ====================================================

role_e druid_t::primary_role() const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE: return ROLE_SPELL; break;
    case DRUID_FERAL:
    case DRUID_GUARDIAN: return ROLE_ATTACK; break;
    case DRUID_RESTORATION:
      if ( player_t::primary_role() == ROLE_SPELL )
        return ROLE_SPELL;
      else
        return ROLE_ATTACK;
      break;
    default: return player_t::primary_role(); break;
  }
}

// druid_t::convert_hybrid_stat =============================================

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
    // This is a guess at how AGI/STR gear will work for Balance/Resto, TODO: confirm
    case STAT_STR_AGI: return STAT_AGILITY;
    // This is a guess at how STR/INT gear will work for Feral/Guardian, TODO: confirm
    // This should probably never come up since druids can't equip plate, but....
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

// druid_t::primary_resource ================================================

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

// druid_t::init_absorb_priority ============================================

void druid_t::init_absorb_priority()
{
  player_t::init_absorb_priority();

  absorb_priority.push_back( talent.brambles->id() );     // Brambles
  absorb_priority.push_back( talent.earthwarden->id() );  // Earthwarden - Legion TOCHECK
}

// druid_t::target_mitigation ===============================================

void druid_t::target_mitigation( school_e school, result_amount_type type, action_state_t* s )
{
  s->result_amount *= 1.0 + buff.barkskin->value();

  s->result_amount *= 1.0 + buff.survival_instincts->value();

  s->result_amount *= 1.0 + buff.pulverize->value();

  if ( spec.thick_hide )
    s->result_amount *= 1.0 + spec.thick_hide->effectN( 1 ).percent();

  if ( talent.rend_and_tear->ok() )
    s->result_amount *= 1.0 + talent.rend_and_tear->effectN( 3 ).percent() *
                                  get_target_data( s->action->player )->dots.thrash_bear->current_stack();

  player_t::target_mitigation( school, type, s );
}

// druid_t::assess_damage ===================================================

void druid_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && dtype == result_amount_type::DMG_DIRECT && s->result == RESULT_HIT )
    buff.ironfur->up();

  player_t::assess_damage( school, dtype, s );
}

// druid_t::assess_damage_imminent_pre_absorb ===============================

// Trigger effects based on being hit or taking damage.
void druid_t::assess_damage_imminent_pre_absorb( school_e school, result_amount_type dmg, action_state_t* s )
{
  player_t::assess_damage_imminent_pre_absorb( school, dmg, s );

  if ( action_t::result_is_hit( s->result ) && s->result_amount > 0 )
  {
    // Guardian rage from melees
    if ( specialization() == DRUID_GUARDIAN && !s->action->special && cooldown.rage_from_melees->up() )
    {
      resource_gain( RESOURCE_RAGE, spec.bear_form->effectN( 3 ).base_value(), gain.rage_from_melees );
      cooldown.rage_from_melees->start( cooldown.rage_from_melees->duration );
    }

    if ( buff.moonkin_form->check() )
      buff.owlkin_frenzy->trigger();

    if ( buff.cenarion_ward->up() )
      active.cenarion_ward_hot->execute();

    if ( buff.bristling_fur->up() )  // 1 rage per 1% of maximum health taken
      resource_gain( RESOURCE_RAGE, s->result_amount / expected_max_health * 100, gain.bristling_fur );
  }
}

// druid_t::assess_heal =====================================================

void druid_t::assess_heal( school_e school, result_amount_type dmg_type, action_state_t* s )
{
  player_t::assess_heal( school, dmg_type, s );

  trigger_natures_guardian( s );
}

// druid_t::shapeshift ======================================================

void druid_t::shapeshift( form_e f )
{
  if ( get_form() == f )
    return;

  buff.cat_form->expire();
  buff.bear_form->expire();
  buff.moonkin_form->expire();

  switch ( f )
  {
    case CAT_FORM: buff.cat_form->trigger(); break;
    case BEAR_FORM: buff.bear_form->trigger(); break;
    case MOONKIN_FORM: buff.moonkin_form->trigger(); break;
    case NO_FORM: break;
    default: assert( 0 ); break;
  }

  form = f;
}

// druid_t::get_target_data =================================================

druid_td_t* druid_t::get_target_data( player_t* target ) const
{
  assert( target );
  druid_td_t*& td = target_data[ target ];
  if ( !td )
    td = new druid_td_t( *target, const_cast<druid_t&>( *this ) );

  return td;
}

// druid_t::init_beast_weapon ===============================================

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

// druid_t::get_affinity_spec ===============================================
specialization_e druid_t::get_affinity_spec() const
{
  if ( talent.balance_affinity->ok() )
    return DRUID_BALANCE;

  if ( talent.restoration_affinity->ok() )
    return DRUID_RESTORATION;

  if ( talent.feral_affinity->ok() )
    return DRUID_FERAL;

  if ( talent.guardian_affinity->ok() )
    return DRUID_GUARDIAN;

  return SPEC_NONE;
}

// druid_t::find_affinity_spell =============================================

struct affinity_spells_t
{
  const char* name;
  unsigned int spell_id;
  specialization_e spec;
};

const affinity_spells_t affinity_spells[] =
{
    { "Moonkin Form", 197625, DRUID_BALANCE },
    { "Starfire", 197628, DRUID_BALANCE },
    { "Starsurge", 197626, DRUID_BALANCE },
    { "Wrath", 5176, SPEC_NONE },
    { "Frenzied Regeneration", 22842, DRUID_GUARDIAN },
    { "Thrash", 77758, DRUID_GUARDIAN },
    { "Rake", 1822, DRUID_FERAL },
    { "Rip", 1079, DRUID_FERAL },
    { "Maim", 22570, DRUID_FERAL },
    { "Swipe", 106785, DRUID_FERAL },
    { "Primal Fury", 159286, DRUID_FERAL }
};

const spell_data_t* druid_t::find_affinity_spell( const std::string& name ) const
{
  const spell_data_t* spec_spell = find_specialization_spell( name );

  if ( spec_spell->found() )
    return spec_spell;

  for ( const auto& e : affinity_spells )
  {
    if ( util::str_compare_ci( name, e.name ) && ( e.spec == get_affinity_spec() || e.spec == SPEC_NONE ) )
      return find_spell( e.spell_id );
  }

  spec_spell =
      find_spell( dbc->specialization_ability_id( get_affinity_spec(), util::inverse_tokenize( name ).c_str() ) );

  if ( spec_spell->found() )
    return spec_spell;

  return spell_data_t::not_found();
}

// druid_t::trigger_natures_guardian ========================================

void druid_t::trigger_natures_guardian( const action_state_t* trigger_state )
{
  if ( !mastery.natures_guardian->ok() )
    return;
  if ( trigger_state->result_total <= 0 )
    return;
  if ( trigger_state->action == active.natures_guardian || trigger_state->action == active.yseras_gift ||
       trigger_state->action->id == 22842 )  // Frenzied Regeneration
    return;

  active.natures_guardian->base_dd_min = active.natures_guardian->base_dd_max =
      trigger_state->result_total * cache.mastery_value();
  action_state_t* s = active.natures_guardian->get_state();
  s->target         = this;
  active.natures_guardian->snapshot_state( s, result_amount_type::HEAL_DIRECT );
  active.natures_guardian->schedule_execute( s );
}

// druid_t::calculate_expected_max_health ===================================

double druid_t::calculate_expected_max_health() const
{
  double slot_weights = 0;
  double prop_values  = 0;

  for ( size_t i = 0; i < items.size(); i++ )
  {
    const item_t* item = &items[ i ];
    if ( !item || item->slot == SLOT_SHIRT || item->slot == SLOT_RANGED || item->slot == SLOT_TABARD ||
         item->item_level() <= 0 )
      continue;

    const random_prop_data_t item_data = dbc->random_property( item->item_level() );
    int index                          = item_database::random_suffix_type( item->parsed.data );
    if ( item_data.p_epic[ 0 ] == 0 )
      continue;

    slot_weights += item_data.p_epic[ index ] / item_data.p_epic[ 0 ];

    if ( !item->active() )
      continue;

    prop_values += item_data.p_epic[ index ];
  }

  double expected_health = ( prop_values / slot_weights ) * 8.318556;
  expected_health += base.stats.attribute[ STAT_STAMINA ];
  expected_health *= 1.0 + matching_gear_multiplier( ATTR_STAMINA );
  expected_health *= 1.0 + spec.bear_form -> effectN( 2 ).percent();
  expected_health *= current.health_per_stamina;

  return expected_health;
}

// NOTE: This will return the FIRST effect that matches parameters.
const spelleffect_data_t* druid_t::query_aura_effect( const spell_data_t* aura_spell, effect_subtype_t subtype,
                                                      int misc_value, const spell_data_t* affected_spell,
                                                      effect_type_t type ) const
{
  for ( size_t i = 1; i <= aura_spell->effect_count(); i++ )
  {
    const spelleffect_data_t& effect = aura_spell->effectN( i );

    if ( affected_spell != spell_data_t::nil() && !affected_spell->affected_by_all( *dbc, effect ) )
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

void druid_t::vision_of_perfection_proc()
{
  buff_t* vp_buff;
  timespan_t vp_dur;

  switch ( specialization() )
  {
    case DRUID_BALANCE:
      if ( talent.incarnation_moonkin->ok() )
        vp_buff = buff.incarnation_moonkin;
      else
        vp_buff = buff.celestial_alignment;

      resource_gain( RESOURCE_ASTRAL_POWER, 40 * vision_of_perfection_dur, gain.vision_of_perfection );

      if ( azerite.streaking_stars.ok() )
        previous_streaking_stars = SS_CELESTIAL_ALIGNMENT;
      break;

    case DRUID_GUARDIAN:
      vp_buff = buff.incarnation_bear;
      cooldown.mangle->reset( false );
      cooldown.thrash_bear->reset( false );
      cooldown.growl->reset( false );
      cooldown.maul->reset( false );
      break;

    case DRUID_FERAL:
      if ( talent.incarnation_cat->ok() )
      {
        vp_buff = buff.incarnation_cat;
        buff.jungle_stalker->trigger();
      }
      else
      {
        vp_buff = buff.berserk_cat;
      }
      break;

    default:
      return;
  }

  vp_dur = vp_buff->data().duration() * vision_of_perfection_dur;

  if ( vp_buff->check() )
    vp_buff->extend_duration( this, vp_dur );
  else
    vp_buff->trigger( vp_dur );

  proc.vision_of_perfection->occur();

  if ( specialization() == DRUID_BALANCE )
  {
    uptime.vision_of_perfection->update( true, sim->current_time() );
    make_event( *sim, vp_dur, [this]() {
      uptime.vision_of_perfection->update( false, sim->current_time() );
    } );
  }
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_target_data_t( &target, &source ), dots( dots_t() ), buff( buffs_t() )
{
  dots.fury_of_elune         = target.get_dot( "fury_of_elune", &source );
  dots.lifebloom             = target.get_dot( "lifebloom", &source );
  dots.moonfire              = target.get_dot( "moonfire", &source );
  dots.stellar_flare         = target.get_dot( "stellar_flare", &source );
  dots.rake                  = target.get_dot( "rake", &source );
  dots.regrowth              = target.get_dot( "regrowth", &source );
  dots.rejuvenation          = target.get_dot( "rejuvenation", &source );
  dots.rip                   = target.get_dot( "rip", &source );
  dots.sunfire               = target.get_dot( "sunfire", &source );
  dots.starfall              = target.get_dot( "starfall", &source );
  dots.thrash_bear           = target.get_dot( "thrash_bear", &source );
  dots.thrash_cat            = target.get_dot( "thrash_cat", &source );
  dots.wild_growth           = target.get_dot( "wild_growth", &source );
  dots.adaptive_swarm_damage = target.get_dot( "adaptive_swarm_damage", &source );
  dots.adaptive_swarm_heal   = target.get_dot( "adaptive_swarm_heal", &source );
  dots.frenzied_assault      = target.get_dot( "frenzied_assault", &source );

  buff.lifebloom = make_buff( *this, "lifebloom", source.find_class_spell( "Lifebloom" ) );
}

// Copypasta for reporting
bool has_amount_results( const std::array<stats_t::stats_results_t, FULLTYPE_MAX>& res )
{
  return ( res[ FULLTYPE_HIT ].actual_amount.mean() > 0 || res[ FULLTYPE_CRIT ].actual_amount.mean() > 0 );
}

// druid_t::copy_from =====================================================

void druid_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  druid_t* p = debug_cast<druid_t*>( source );

  predator_rppm_rate           = p->predator_rppm_rate;
  initial_astral_power         = p->initial_astral_power;
  initial_moon_stage           = p->initial_moon_stage;
  lively_spirit_stacks         = p->lively_spirit_stacks;
  affinity_resources           = p->affinity_resources;
  kindred_empowerment_ratio    = p->kindred_empowerment_ratio;
  convoke_the_spirits_heals    = p->convoke_the_spirits_heals;
  convoke_the_spirits_ultimate = p->convoke_the_spirits_ultimate;
  thorns_attack_period         = p->thorns_attack_period;
  thorns_hit_chance            = p->thorns_hit_chance;
}

void druid_t::output_json_report( js::JsonOutput& /*root*/ ) const
{
  return;  // NYI.

  if ( specialization() != DRUID_FERAL )
    return;

  /* snapshot_stats:
   *    savage_roar:
   *        [ name: Shred Exec: 15% Benefit: 98% ]
   *        [ name: Rip Exec: 88% Benefit: 35% ]
   *    bloodtalons:
   *        [ name: Rip Exec: 99% Benefit: 99% ]
   *    tigers_fury:
   */
  for ( size_t i = 0, end = stats_list.size(); i < end; i++ )
  {
    stats_t* stats   = stats_list[ i ];
    double tf_exe_up = 0, tf_exe_total = 0;
    double tf_benefit_up = 0, tf_benefit_total = 0;
    double bt_exe_up = 0, bt_exe_total = 0;
    double bt_benefit_up = 0, bt_benefit_total = 0;
    // int n = 0;

    for ( size_t j = 0, end2 = stats->action_list.size(); j < end2; j++ )
    {
      cat_attacks::cat_attack_t* a = dynamic_cast<cat_attacks::cat_attack_t*>( stats->action_list[ j ] );
      if ( !a )
        continue;

      if ( !a->snapshots.tigers_fury )
        continue;

      tf_exe_up += a->tf_counter->mean_exe_up();
      tf_exe_total += a->tf_counter->mean_exe_total();
      tf_benefit_up += a->tf_counter->mean_tick_up();
      tf_benefit_total += a->tf_counter->mean_tick_total();
      if ( has_amount_results( stats->direct_results ) )
      {
        tf_benefit_up += a->tf_counter->mean_exe_up();
        tf_benefit_total += a->tf_counter->mean_exe_total();
      }

      if ( a->snapshots.bloodtalons )
      {
        bt_exe_up += a->bt_counter->mean_exe_up();
        bt_exe_total += a->bt_counter->mean_exe_total();
        bt_benefit_up += a->bt_counter->mean_tick_up();
        bt_benefit_total += a->bt_counter->mean_tick_total();
        if ( has_amount_results( stats->direct_results ) )
        {
          bt_benefit_up += a->bt_counter->mean_exe_up();
          bt_benefit_total += a->bt_counter->mean_exe_total();
        }
      }

      /*    if ( tf_exe_total > 0 || bt_exe_total > 0 )
            {
              // auto snapshot = root["snapshot_stats"].add();
              if ( talent.savage_roar->ok() )
              {
                // auto sr = snapshot["savage_roar"].make_array();
              }
              if ( talent.bloodtalons->ok() ) {}
            }*/
    }
  }
}

void druid_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  // Spec-wide auras
  action.apply_affecting_aura( spec.druid );
  action.apply_affecting_aura( spec.feral );
  action.apply_affecting_aura( spec.restoration );
  action.apply_affecting_aura( spec.balance );
  action.apply_affecting_aura( spec.guardian );

  // Rank spells
  action.apply_affecting_aura( spec.moonfire_2 );
  action.apply_affecting_aura( spec.moonfire_3 );

  // Talents
  action.apply_affecting_aura( talent.feral_affinity );
  action.apply_affecting_aura( talent.stellar_drift );
  action.apply_affecting_aura( talent.twin_moons );
  action.apply_affecting_aura( talent.sabertooth );
  action.apply_affecting_aura( talent.soul_of_the_forest_cat );
  action.apply_affecting_aura( talent.soul_of_the_forest_bear );

  // Legendaries
  action.apply_affecting_aura( legendary.luffainfused_embrace );
  action.apply_affecting_aura( legendary.legacy_of_the_sleeper );
  action.apply_affecting_aura( legendary.circle_of_life_and_death );
}

//void druid_t::output_json_report(js::JsonOutput& root) const
//{
//   if (specialization() == DRUID_FERAL)
//   {
//      auto snapshot = root["snapshot_stats"].add();
//
//      if ( talent.savage_roar -> ok() )
//      {
//         snapshot["savage_roar"].make_array();
//         *(void*)0 == 0;
//      }
//
//   }
//}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
public:
  druid_report_t( druid_t& player ) : p( player ) {}

  void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.specialization() == DRUID_FERAL )
      feral_snapshot_table( os );
  }

  void feral_snapshot_table( report::sc_html_stream& os )
  {
    // Write header
    os << "<div class=\"player-section custom_section\">\n"
       << "<h3 class=\"toggle open\">Snapshot Table</h3>\n"
       << "<div class=\"toggle-content\">\n"
       << "<table class=\"sc sort even\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th colspan=\"2\">Tiger's Fury</th>\n";

    if ( p.talent.bloodtalons->ok() )
    {
      os << "<th colspan=\"2\">Bloodtalons</th>\n";
    }
    os << "</tr>\n";

    os << "<tr>\n"
       << "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability Name</th>\n"
       << "<th class=\"toggle-sort\">Execute %</th>\n"
       << "<th class=\"toggle-sort\">Benefit %</th>\n";

    if ( p.talent.bloodtalons->ok() )
    {
      os << "<th class=\"toggle-sort\">Execute %</th>\n"
         << "<th class=\"toggle-sort\">Benefit %</th>\n";
    }
    os << "</tr>\n"
       << "</thead>\n";

    // Compile and Write Contents
    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats   = p.stats_list[ i ];
      double tf_exe_up = 0, tf_exe_total = 0;
      double tf_benefit_up = 0, tf_benefit_total = 0;
      double bt_exe_up = 0, bt_exe_total = 0;
      double bt_benefit_up = 0, bt_benefit_total = 0;

      for ( size_t j = 0, end2 = stats->action_list.size(); j < end2; j++ )
      {
        cat_attacks::cat_attack_t* a = dynamic_cast<cat_attacks::cat_attack_t*>( stats->action_list[ j ] );
        if ( !a )
          continue;

        if ( !a->snapshots.tigers_fury )
          continue;

        tf_exe_up += a->tf_counter->mean_exe_up();
        tf_exe_total += a->tf_counter->mean_exe_total();
        tf_benefit_up += a->tf_counter->mean_tick_up();
        tf_benefit_total += a->tf_counter->mean_tick_total();
        if ( has_amount_results( stats->direct_results ) )
        {
          tf_benefit_up += a->tf_counter->mean_exe_up();
          tf_benefit_total += a->tf_counter->mean_exe_total();
        }

        if ( a->snapshots.bloodtalons )
        {
          bt_exe_up += a->bt_counter->mean_exe_up();
          bt_exe_total += a->bt_counter->mean_exe_total();
          bt_benefit_up += a->bt_counter->mean_tick_up();
          bt_benefit_total += a->bt_counter->mean_tick_total();
          if ( has_amount_results( stats->direct_results ) )
          {
            bt_benefit_up += a->bt_counter->mean_exe_up();
            bt_benefit_total += a->bt_counter->mean_exe_total();
          }
        }
      }

      if ( tf_exe_total > 0 || bt_exe_total > 0 )
      {
        const action_t& action = *( stats->action_list[ 0 ] );
        std::string name_str   = report_decorators::decorated_action( action );

        // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
        os.printf( "<tr><td class=\"left\">%s</td><td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
                   name_str.c_str(), util::round( tf_exe_total ? ( tf_exe_up / tf_exe_total * 100 ) : 0, 2 ),
                   util::round( tf_benefit_total ? ( tf_benefit_up / tf_benefit_total * 100 ) : 0, 2 ) );

        if ( p.talent.bloodtalons->ok() )
        {
          // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
          os.printf( "<td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
                     util::round( bt_exe_total ? ( bt_exe_up / bt_exe_total * 100 ) : 0, 2 ),
                     util::round( bt_benefit_total ? ( bt_benefit_up / bt_benefit_total * 100 ) : 0, 2 ) );
        }
        os << "</tr>";
      }
    }
    os << "</tr>";

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

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new druid_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new druid_report_t( *p ) );
    return p;
  }
  bool valid() const override { return true; }
  void init( player_t* p ) const override
  {
    p->buffs.stampeding_roar = make_buff( p, "stampeding_roar", p->find_spell( 77764 ) )
      ->set_max_stack( 1 )
      ->set_duration( timespan_t::from_seconds( 8.0 ) );
  }

  void static_init() const override {}
  void register_hotfixes() const override {}
  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}

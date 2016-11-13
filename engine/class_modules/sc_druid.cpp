// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE
// ==========================================================================
// Druid
// ==========================================================================

/*
  Legion TODO:

  Astral Influence
  Affinity active components
  Artifact 20 rank traits?
  Ekowraith, Creator of Worlds legendary
  Weapon Damage scaling issue

  Feral =====================================================================
  Predator vs. adds
  Artifact utility traits
  Check Luffa-Wrapped Grips (what procs it)
  Check Blood Scent crit
  Jagged Wounds partial ticks

  Balance ===================================================================
  Stellar Drift cast while moving
  Force of Nature
  Shooting Stars AsP react
  Check Fury of Elune

  Touch of the Moon
  Light of the Sun
  Rejuvenating Innervation

  Guardian ==================================================================
  Statistics?
  Incarnation CD modifier rework
  Check Galactic Guardian proc sources

  Resto =====================================================================
  All the things

  Needs Documenting =========================================================
  predator_rppm option
  initial_astral_power option
  initial_moon_stage option
  moon stage expressions
*/

// Forward declarations
struct druid_t;

// Active actions
struct stalwart_guardian_t;
namespace spells {
struct moonfire_t;
struct starshards_t;
struct shooting_stars_t;
}
namespace heals {
struct cenarion_ward_hot_t;
}
namespace cat_attacks {
struct gushing_wound_t;
}
namespace bear_attacks {
struct bear_attack_t;
}

enum form_e {
  CAT_FORM       = 0x1,
  NO_FORM        = 0x2,
  TRAVEL_FORM    = 0x4,
  AQUATIC_FORM   = 0x8, // Legacy
  BEAR_FORM      = 0x10,
  DIRE_BEAR_FORM = 0x40, // Legacy
  MOONKIN_FORM   = 0x40000000,
};

enum moon_stage_e {
  NEW_MOON,
  HALF_MOON,
  FULL_MOON,
};

struct druid_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* ashamanes_frenzy;
    dot_t* fury_of_elune;
    dot_t* gushing_wound;
    dot_t* lifebloom;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* rip;
    dot_t* shadow_rake;
    dot_t* shadow_rip;
    dot_t* shadow_thrash;
    dot_t* stellar_flare;
    dot_t* sunfire;
    dot_t* starfall;
    dot_t* thrash_cat;
    dot_t* thrash_bear;
    dot_t* wild_growth;
  } dots;

  struct buffs_t
  {
    buff_t* lifebloom;
  } buff;

  struct debuffs_t
  {
    buff_t* bloodletting;
    buff_t* open_wounds;
    buff_t* stellar_empowerment;
  } debuff;

  druid_td_t( player_t& target, druid_t& source );

  bool hot_ticking()
  {
    return dots.regrowth      -> is_ticking() ||
           dots.rejuvenation  -> is_ticking() ||
           dots.lifebloom     -> is_ticking() ||
           dots.wild_growth   -> is_ticking();
  }

  unsigned feral_tier19_4pc_bleeds() // TOCHECK
  {
    return dots.rip -> is_ticking()
           + dots.rake -> is_ticking()
           + dots.thrash_cat -> is_ticking();
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

  snapshot_counter_t( druid_t* player , buff_t* buff );

  bool check_all()
  {
    double n_up = 0;
    for ( auto& elem : b )
    {
      if ( elem -> check() )
        n_up++;
    }
    if ( n_up == 0 )
      return false;

    wasted_buffs += n_up - 1;
    return true;
  }

  void add_buff( buff_t* buff )
  {
    b.push_back( buff );
  }

  void count_execute()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    check_all() ? ( exe_up++ , is_snapped = true ) : ( exe_down++ , is_snapped = false );
  }

  void count_tick()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    is_snapped ? tick_up++ : tick_down++;
  }

  double divisor() const
  {
    if ( ! sim -> debug && ! sim -> log && sim -> iterations > sim -> threads )
      return sim -> iterations - sim -> threads;
    else
      return std::min( sim -> iterations, sim -> threads );
  }

  double mean_exe_up() const
  { return exe_up / divisor(); }

  double mean_exe_down() const
  { return exe_down / divisor(); }

  double mean_tick_up() const
  { return tick_up / divisor(); }

  double mean_tick_down() const
  { return tick_down / divisor(); }

  double mean_exe_total() const
  { return ( exe_up + exe_down ) / divisor(); }

  double mean_tick_total() const
  { return ( tick_up + tick_down ) / divisor(); }

  double mean_waste() const
  { return wasted_buffs / divisor(); }

  void merge( const snapshot_counter_t& other )
  {
    exe_up += other.exe_up;
    exe_down += other.exe_down;
    tick_up += other.tick_up;
    tick_down += other.tick_down;
    wasted_buffs += other.wasted_buffs;
  }
};

struct druid_t : public player_t
{
private:
  form_e form; // Active druid form
public:
  moon_stage_e moon_stage;
  struct {
    double x, y;
  } fury_of_elune_position;

  // counters for snapshot tracking
  std::vector<snapshot_counter_t*> counters;

  // Active
  action_t* t16_2pc_starfall_bolt;
  action_t* t16_2pc_sun_bolt;

  double starshards;
  double expected_max_health; // For Bristling Fur calculations.

  // RPPM objects
  struct rppms_t
  {
    // Feral
    real_ppm_t* predator; // Optional RPPM approximation
    real_ppm_t* shadow_thrash;

    // Balance
    real_ppm_t* balance_tier18_2pc;
  } rppm;

  // Options
  double predator_rppm_rate;
  double initial_astral_power;
  int    initial_moon_stage;

  struct active_actions_t
  {
    stalwart_guardian_t*            stalwart_guardian;
    action_t* gushing_wound;
    heals::cenarion_ward_hot_t*     cenarion_ward_hot;
    attack_t* ashamanes_rip;
    action_t* brambles;
    action_t* brambles_pulse;
    spell_t*  echoing_stars;
    spell_t*  galactic_guardian;
    spell_t*  goldrinns_fang;
    action_t* natures_guardian;
    spell_t*  rage_of_the_sleeper;
    attack_t* shadow_thrash;
    spell_t*  shooting_stars;
    spell_t*  starfall;
    spell_t*  starshards;
    action_t* yseras_gift;
  } active;

  // Pets
  std::array<pet_t*, 11> pet_fey_moonwing; // 30 second duration, 3 second internal icd... create 11 to be safe.

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

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* dash;
    buff_t* displacer_beast;
    buff_t* cenarion_ward;
    buff_t* clearcasting;
    buff_t* incarnation_proxy;
    buff_t* prowl;
    buff_t* stampeding_roar;
    buff_t* wild_charge_movement;

    // Balance
    buff_t* blessing_of_anshe;
    buff_t* blessing_of_elune;
    buff_t* celestial_alignment;
    buff_t* fury_of_elune_up; // Tracking buff for APL
    buff_t* incarnation_moonkin;
    buff_t* lunar_empowerment;
    buff_t* moonkin_form;
    buff_t* oneths_intuition; // Legion Legendary
    buff_t* oneths_overconfidence; // Legion Legendary
    buff_t* power_of_elune; // Legion Legendary
    buff_t* solar_empowerment;
    buff_t* star_power; // Moon and Stars artifact medal
    buff_t* the_emerald_dreamcatcher; // Legion Legendary
    buff_t* warrior_of_elune;
    buff_t* balance_tier18_4pc; // T18 4P Balance

    // Feral
    buff_t* ashamanes_energy;
    buff_t* berserk;
    buff_t* bloodtalons;
    buff_t* elunes_guidance;
    buff_t* feral_instinct;
    buff_t* incarnation_cat;
    buff_t* predatory_swiftness;
    buff_t* protection_of_ashamane;
    buff_t* savage_roar;
    buff_t* scent_of_blood;
    buff_t* tigers_fury;
    buff_t* feral_tier17_4pc;

    // Guardian
    buff_t* adaptive_fur;
    buff_t* barkskin;
    buff_t* bristling_fur;
    buff_t* earthwarden;
    buff_t* earthwarden_driver;
    buff_t* galactic_guardian;
    buff_t* gore;
    buff_t* gory_fur;
    buff_t* guardian_of_elune;
    buff_t* incarnation_bear;
    buff_t* ironfur;
    buff_t* mark_of_ursol;
    buff_t* pulverize;
    buff_t* rage_of_the_sleeper;
    buff_t* survival_instincts;
    buff_t* guardian_tier17_4pc;
    buff_t* guardian_tier19_4pc;

    // Restoration
    buff_t* incarnation_tree;
    buff_t* soul_of_the_forest; // needs checking
    buff_t* yseras_gift;
    buff_t* harmony; // NYI
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk;
    cooldown_t* celestial_alignment;
    cooldown_t* growl;
    cooldown_t* incarnation;
    cooldown_t* mangle;
    cooldown_t* maul;
    cooldown_t* moon_cd; // New / Half / Full Moon
    cooldown_t* wod_pvp_4pc_melee;
    cooldown_t* swiftmend;
    cooldown_t* tigers_fury;
    cooldown_t* warrior_of_elune;
  } cooldown;

  // Gains
  struct gains_t
  {
    // Multiple Specs / Forms
    gain_t* clearcasting;       // Feral & Restoration
    gain_t* soul_of_the_forest; // Feral & Guardian
    
    // Balance
    gain_t* astral_communion;
    gain_t* blessing_of_anshe;
    gain_t* lunar_strike;
    gain_t* moonfire;
    gain_t* shooting_stars;
    gain_t* solar_wrath;
    gain_t* sunfire;

    // Feral (Cat)
    gain_t* ashamanes_energy;
    gain_t* ashamanes_frenzy;
    gain_t* brutal_slash;
    gain_t* energy_refund;
    gain_t* elunes_guidance;
    gain_t* primal_fury;
    gain_t* rake;
    gain_t* shred;
    gain_t* swipe_cat;
    gain_t* tigers_fury;
    gain_t* feral_tier17_2pc;
    gain_t* feral_tier18_4pc;
    gain_t* feral_tier19_2pc;

    // Guardian (Bear)
    gain_t* bear_form;
    gain_t* blood_frenzy;
    gain_t* brambles;
    gain_t* bristling_fur;
    gain_t* galactic_guardian;
    gain_t* gore;
    gain_t* stalwart_guardian;
    gain_t* rage_refund;
    gain_t* guardian_tier17_2pc;
    gain_t* guardian_tier18_2pc;
  } gain;

  // Masteries
  struct masteries_t
  {
    // Done
    const spell_data_t* natures_guardian;
    const spell_data_t* natures_guardian_AP;
    const spell_data_t* razor_claws;
    const spell_data_t* starlight;

    // NYI / TODO!
    const spell_data_t* harmony;
  } mastery;

  // Procs
  struct procs_t
  {
    // Feral & Resto
    proc_t* clearcasting;
    proc_t* clearcasting_wasted;

    // Feral
    proc_t* ashamanes_bite;
    proc_t* the_wildshapers_clutch;
    proc_t* predator;
    proc_t* predator_wasted;
    proc_t* primal_fury;
    proc_t* tier17_2pc_melee;

    // Balance
    proc_t* starshards;

    // Guardian
    proc_t* gore;
  } proc;

  // Class Specializations
  struct specializations_t
  {
    // Generic
    const spell_data_t* druid;
    const spell_data_t* critical_strikes;       // Feral & Guardian
    const spell_data_t* killer_instinct;        // Feral & Guardian
    const spell_data_t* nurturing_instinct;     // Balance & Restoration
    const spell_data_t* leather_specialization; // All Specializations
    const spell_data_t* omen_of_clarity;        // Feral & Restoration

    // Feral
    const spell_data_t* feral;
    const spell_data_t* feral_overrides;
    const spell_data_t* feral_overrides2;
    const spell_data_t* cat_form; // Cat form hidden effects
    const spell_data_t* cat_form_speed;
    const spell_data_t* feline_swiftness; // Feral Affinity
    const spell_data_t* gushing_wound; // tier17_4pc
    const spell_data_t* predatory_swiftness;
    const spell_data_t* primal_fury;
    const spell_data_t* rip;
    const spell_data_t* sharpened_claws;
    const spell_data_t* swipe_cat;
    const spell_data_t* rake_2;
    const spell_data_t* tigers_fury_2;
    const spell_data_t* ferocious_bite_2;
    const spell_data_t* shred;
    const spell_data_t* shred_2;
    const spell_data_t* shred_3;
    const spell_data_t* swipe_2;

    // Balance
    const spell_data_t* balance;
    const spell_data_t* balance_overrides;
    const spell_data_t* astral_influence; // Balance Affinity
    const spell_data_t* astral_power;
    const spell_data_t* blessing_of_anshe;
    const spell_data_t* blessing_of_elune;
    const spell_data_t* celestial_alignment;
    const spell_data_t* moonkin_form;
    const spell_data_t* starfall;
    const spell_data_t* stellar_empowerment;
    const spell_data_t* balance_tier19_2pc;
    const spell_data_t* starsurge_2;
    const spell_data_t* moonkin_2;
    const spell_data_t* starfall_2;
    const spell_data_t* sunfire_2;

    // Guardian
    const spell_data_t* guardian;
    const spell_data_t* guardian_overrides;
    const spell_data_t* bear_form;
    const spell_data_t* gore;
    const spell_data_t* ironfur;
    const spell_data_t* thick_hide; // Guardian Affinity
    const spell_data_t* thrash_bear_dot; // For Rend and Tear modifier
    const spell_data_t* mangle_2;
    const spell_data_t* lightning_reflexes;
    const spell_data_t* ironfur_2;

    // Resto
    const spell_data_t* yseras_gift; // Restoration Affinity
  } spec;

  // Talents
  struct talents_t
  {
    // Multiple Specs
    const spell_data_t* renewal;
    const spell_data_t* displacer_beast;
    const spell_data_t* wild_charge;

    const spell_data_t* balance_affinity;
    const spell_data_t* feral_affinity;
    const spell_data_t* guardian_affinity;
    const spell_data_t* restoration_affinity;

    const spell_data_t* mighty_bash;
    const spell_data_t* mass_entanglement;
    const spell_data_t* typhoon;

    const spell_data_t* soul_of_the_forest;
    const spell_data_t* moment_of_clarity;

    // Feral
    const spell_data_t* predator;
    const spell_data_t* blood_scent;
    const spell_data_t* lunar_inspiration;

    const spell_data_t* incarnation_cat;
    const spell_data_t* savage_roar;

    const spell_data_t* sabertooth;
    const spell_data_t* jagged_wounds;
    const spell_data_t* elunes_guidance;

    const spell_data_t* brutal_slash;
    const spell_data_t* bloodtalons;

    // Balance
    const spell_data_t* shooting_stars;
    const spell_data_t* warrior_of_elune;
    const spell_data_t* starlord;

    const spell_data_t* incarnation_moonkin;
    const spell_data_t* stellar_flare;

    const spell_data_t* stellar_drift;
    const spell_data_t* full_moon;
    const spell_data_t* natures_balance;

    const spell_data_t* fury_of_elune;
    const spell_data_t* astral_communion;
    const spell_data_t* blessing_of_the_ancients;

    // Guardian
    const spell_data_t* brambles;
    const spell_data_t* pulverize;
    const spell_data_t* blood_frenzy;

    const spell_data_t* gutteral_roars;

    const spell_data_t* incarnation_bear;
    const spell_data_t* galactic_guardian;

    const spell_data_t* earthwarden;
    const spell_data_t* guardian_of_elune;
    const spell_data_t* survival_of_the_fittest;

    const spell_data_t* rend_and_tear;
    const spell_data_t* lunar_beam;
    const spell_data_t* bristling_fur;

    // Restoration
    const spell_data_t* verdant_growth;
    const spell_data_t* cenarion_ward;
    const spell_data_t* germination;

    const spell_data_t* incarnation_tree;
    const spell_data_t* cultivation;

    const spell_data_t* prosperity;
    const spell_data_t* inner_peace;
    const spell_data_t* profusion;

    const spell_data_t* stonebark;
    const spell_data_t* flourish;
  } talent;

  // Artifacts
  struct artifact_spell_data_t
  {
    // Balance -- Scythe of Elune
    artifact_power_t bladed_feathers;
    artifact_power_t dark_side_of_the_moon;
    artifact_power_t empowerment;
    artifact_power_t falling_star;
    artifact_power_t moon_and_stars;
    artifact_power_t new_moon;
    artifact_power_t power_of_goldrinn;
    artifact_power_t scythe_of_the_stars;
    artifact_power_t skywrath;
    artifact_power_t solar_stabbing;
    artifact_power_t sunfire_burns;
    artifact_power_t twilight_glow;
    artifact_power_t echoing_stars;
    artifact_power_t sunblind;

    // NYI
    artifact_power_t light_of_the_sun;
    artifact_power_t rejuvenating_innervation;
    artifact_power_t touch_of_the_moon;

    // Feral -- Fangs of Ashamane
    artifact_power_t ashamanes_bite;
    artifact_power_t ashamanes_energy;
    artifact_power_t ashamanes_frenzy;
    artifact_power_t attuned_to_nature;
    artifact_power_t feral_instinct;
    artifact_power_t feral_power;
    artifact_power_t honed_instincts;
    artifact_power_t open_wounds;
    artifact_power_t powerful_bite;
    artifact_power_t protection_of_ashamane;
    artifact_power_t razor_fangs;
    artifact_power_t scent_of_blood;
    artifact_power_t shadow_thrash;
    artifact_power_t sharpened_claws;
    artifact_power_t shredder_fangs;
    artifact_power_t tear_the_flesh;

    // NYI
    artifact_power_t hardened_roots;

    // Guardian -- Claws of Ursoc
    artifact_power_t adaptive_fur;
    artifact_power_t bear_hug;
    artifact_power_t embrace_of_the_nightmare;
    artifact_power_t rage_of_the_sleeper;
    artifact_power_t reinforced_fur;
    artifact_power_t mauler;
    artifact_power_t jagged_claws;
    artifact_power_t bestial_fortitude;
    artifact_power_t perpetual_spring;
    artifact_power_t ursocs_endurance;
    artifact_power_t sharpened_instincts;
    artifact_power_t vicious_bites;
    artifact_power_t wildflesh;
    artifact_power_t gory_fur;
    artifact_power_t iron_claws;

    // NYI
    artifact_power_t bloody_paws;
    artifact_power_t roar_of_the_crowd;
  } artifact;

  struct legendary_t
  {
    // Balance
    timespan_t impeccable_fel_essence;

    // Feral
    double the_wildshapers_clutch;
    timespan_t ailuro_pouncers;
  } legendary;

  druid_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DRUID, name, r ),
    form( NO_FORM ),
    t16_2pc_starfall_bolt( nullptr ),
    t16_2pc_sun_bolt( nullptr ),
    starshards( 0.0 ),
    predator_rppm_rate( 0.0 ),
    initial_astral_power( 0 ),
    initial_moon_stage( NEW_MOON ),
    active( active_actions_t() ),
    pet_fey_moonwing(),
    caster_form_weapon(),
    caster_melee_attack( nullptr ),
    cat_melee_attack( nullptr ),
    bear_melee_attack( nullptr ),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    mastery( masteries_t() ),
    proc( procs_t() ),
    spec( specializations_t() ),
    talent( talents_t() ),
    legendary( legendary_t() )
  {
    cooldown.berserk             = get_cooldown( "berserk"             );
    cooldown.celestial_alignment = get_cooldown( "celestial_alignment" );
    cooldown.growl               = get_cooldown( "growl"               );
    cooldown.incarnation         = get_cooldown( "incarnation"         );
    cooldown.mangle              = get_cooldown( "mangle"              );
    cooldown.maul                = get_cooldown( "maul"                );
    cooldown.moon_cd             = get_cooldown( "moon_cd"             );
    cooldown.wod_pvp_4pc_melee   = get_cooldown( "wod_pvp_4pc_melee"   );
    cooldown.swiftmend           = get_cooldown( "swiftmend"           );
    cooldown.tigers_fury         = get_cooldown( "tigers_fury"         );
    cooldown.warrior_of_elune    = get_cooldown( "warrior_of_elune"    );

    cooldown.wod_pvp_4pc_melee -> duration = timespan_t::from_seconds( 30.0 );

    legendary.the_wildshapers_clutch = 0.0;

    equipped_weapon_dps = 0;

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  virtual           ~druid_t();

  // Character Definition
  virtual void      init() override;
  virtual void      init_absorb_priority() override;
  virtual void      init_action_list() override;
  virtual void      init_base_stats() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_resources( bool ) override;
  virtual void      init_rng() override;
  virtual void      init_spells() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual void      arise() override;
  virtual void      reset() override;
  virtual void      merge( player_t& other ) override;
  virtual timespan_t available() const override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_attribute( attribute_e attr ) const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_block() const override { return 0; }
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_dodge() const override;
  virtual double    composite_dodge_rating() const override;
  virtual double    composite_leech() const override;
  virtual double    composite_melee_crit_chance() const override;
  virtual double    composite_melee_expertise( const weapon_t* ) const override;
  virtual double    composite_parry() const override { return 0; }
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_rating_multiplier( rating_e ) const override;
  virtual double    composite_spell_crit_chance() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_spell_power( school_e school ) const override;
  virtual double    composite_spell_power_multiplier() const override;
  virtual double    temporary_movement_modifier() const override;
  virtual double    passive_movement_modifier() const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual resource_e primary_resource() const override;
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    mana_regen_per_second() const override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  virtual void      recalculate_resource_max( resource_e ) override;
  virtual void      create_options() override;
  virtual std::string      create_profile( save_e type = SAVE_ALL ) override;
  virtual druid_td_t* get_target_data( player_t* target ) const override;

  form_e get_form() const { return form; }
  void shapeshift( form_e );
  void init_beast_weapon( weapon_t&, double );
  const spell_data_t* find_affinity_spell( const std::string& );
  void trigger_natures_guardian( const action_state_t* );
  double calculate_expected_max_health() const;

private:
  void              apl_precombat();
  void              apl_default();
  void              apl_feral();
  void              apl_balance();
  void              apl_guardian();
  void              apl_restoration();

  target_specific_t<druid_td_t> target_data;
};

druid_t::~druid_t()
{
  range::dispose( counters );
}

snapshot_counter_t::snapshot_counter_t( druid_t* player , buff_t* buff ) :
  sim( player -> sim ), p( player ), b( 0 ),
  exe_up( 0 ), exe_down( 0 ), tick_up( 0 ), tick_down( 0 ), is_snapped( false ), wasted_buffs( 0 )
{
  b.push_back( buff );
  p -> counters.push_back( this );
}

// Stalwart Guardian ( 6.2 T18 Guardian Trinket) ============================

struct stalwart_guardian_t : public absorb_t
{
  struct stalwart_guardian_reflect_t : public attack_t
  {
    stalwart_guardian_reflect_t( druid_t* p ) :
      attack_t( "stalwart_guardian_reflect", p, p -> find_spell( 185321 ) )
    {
      may_block = may_dodge = may_parry = may_miss = true;
      may_crit = true;
    }
  };

  double incoming_damage;
  double absorb_limit;
  double absorb_size;
  player_t* triggering_enemy;
  stalwart_guardian_reflect_t* reflect;

  stalwart_guardian_t( const special_effect_t& effect ) :
    absorb_t( "stalwart_guardian_bg", effect.player, effect.player -> find_spell( 185321 ) ),
    incoming_damage( 0 ), absorb_size( 0 )
  {
    background = quiet = true;
    may_crit = false;
    target = player;
    harmful = false;
    attack_power_mod.direct = effect.driver() -> effectN( 1 ).average( effect.item ) / 100.0;
    absorb_limit = effect.driver() -> effectN( 2 ).percent();

    reflect = new stalwart_guardian_reflect_t( p() );
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  void init() override
  {
    absorb_t::init();

    snapshot_flags &= ~STATE_VERSATILITY; // Is not affected by versatility.
  }

  void execute() override
  {
    absorb_t::execute();

    reflect -> base_dd_min = absorb_size;
    reflect -> target = triggering_enemy;
    reflect -> execute();
  }

  void impact( action_state_t* s ) override
  {
    s -> result_amount = std::min( s -> result_total, absorb_limit * incoming_damage );
    absorb_size = s -> result_amount;
  }
};

namespace pets {


// ==========================================================================
// Pets and Guardians
// ==========================================================================

// T18 2PC Balance Fairies ==================================================

struct fey_moonwing_t: public pet_t
{
  struct fey_missile_t: public spell_t
  {
    fey_missile_t( fey_moonwing_t* player ):
      spell_t( "fey_missile", player, player -> find_spell( 188046 ) )
    {
      if ( player -> o() -> pet_fey_moonwing[0] )
        stats = player -> o() -> pet_fey_moonwing[0] -> get_stats( "fey_missile" );
      may_crit = true;

      // Casts have a delay that decreases with haste. This is a very rough approximation.
      cooldown -> duration = timespan_t::from_millis( 600 );
      cooldown -> hasted = true;
    }
  };
  druid_t* o() { return static_cast<druid_t*>( owner ); }

  fey_moonwing_t( sim_t* sim, druid_t* owner ):
    pet_t( sim, owner, "fey_moonwing", true /*GUARDIAN*/, true )
  {
    owner_coeff.sp_from_sp = 0.75;
    regen_type = REGEN_DISABLED;
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    resources.base[RESOURCE_HEALTH] = owner -> resources.max[RESOURCE_HEALTH] * 0.4;
    resources.base[RESOURCE_MANA] = 0;

    initial.stats.attribute[ATTR_INTELLECT] = 0;
    initial.spell_power_per_intellect = 0;
    intellect_per_owner = 0;
    stamina_per_owner = 0;
    action_list_str = "fey_missile";
  }

  void summon( timespan_t duration ) override
  {
    pet_t::summon( duration );
    o() -> buff.balance_tier18_4pc -> trigger();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "fey_missile"  ) return new fey_missile_t( this );
    return pet_t::create_action( name, options_str );
  }
};
} // end namespace pets

namespace buffs {

template <typename BuffBase>
struct druid_buff_t : public BuffBase
{
protected:
  typedef druid_buff_t base_t;
  druid_t& druid;

  // Used when shapeshifting to switch to a new attack & schedule it to occur
  // when the current swing timer would have ended.
  void swap_melee( attack_t* new_attack, weapon_t& new_weapon )
  {
    if ( druid.main_hand_attack && druid.main_hand_attack -> execute_event )
    {
      new_attack -> base_execute_time = new_weapon.swing_time;
      new_attack -> execute_event = new_attack -> start_action_execute_event(
                                      druid.main_hand_attack -> execute_event -> remains() );
      druid.main_hand_attack -> cancel();
    }
    new_attack -> weapon = &new_weapon;
    druid.main_hand_attack = new_attack;
    druid.main_hand_weapon = new_weapon;
  }

public:
  druid_buff_t( druid_t& p, const buff_creator_basics_t& params ) :
    BuffBase( params ),
    druid( p )
  { }

  druid_t& p() const { return druid; }
};

// Adaptive Fur =============================================================
/* Custom benefit implementation since we need to be able to increment
   down_count even when the buff is active. */

struct adaptive_fur_t : public druid_buff_t< buff_t >
{
  adaptive_fur_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "adaptive_fur", p.find_spell( 200945 ) )
      .chance( p.artifact.adaptive_fur.data().proc_chance() )
      .default_value( p.find_spell( 200945 ) -> effectN( 1 ).percent() ) )
  {}

  void no_benefit()
  { down_count++; }
};

// Bear Form ================================================================

struct bear_form_t : public druid_buff_t< buff_t >
{
public:
  bear_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "bear_form", p.find_class_spell( "Bear Form" ) ) ),
    rage_spell( p.find_spell( 17057 ) )
  {
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_STAMINA );
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_EXP );

    if ( p.spec.killer_instinct -> ok() )
      add_invalidate( CACHE_AGILITY );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( druid.caster_melee_attack, druid.caster_form_weapon );

    druid.recalculate_resource_max( RESOURCE_HEALTH );
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    druid.buff.moonkin_form -> expire();
    druid.buff.cat_form -> expire();

    druid.buff.tigers_fury -> expire(); // Mar 03 2016: Tiger's Fury ends when you enter bear form.

    swap_melee( druid.bear_melee_attack, druid.bear_weapon );

    // Set rage to 0 and then gain rage to 10
    druid.resource_loss( RESOURCE_RAGE, druid.resources.current[ RESOURCE_RAGE ] );
    druid.resource_gain( RESOURCE_RAGE, rage_spell -> effectN( 1 ).base_value() / 10.0, druid.gain.bear_form );
    // TODO: Clear rage on bear form exit instead of entry.

    base_t::start( stacks, value, duration );

    druid.recalculate_resource_max( RESOURCE_HEALTH );
  }
private:
  const spell_data_t* rage_spell;
};

// Berserk Buff Template ====================================================

struct berserk_buff_base_t : public druid_buff_t<buff_t>
{
  double increased_max_energy;

  berserk_buff_base_t( druid_t& p, const std::string& name, const spell_data_t* spell ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, name, spell )
                          .cd( timespan_t::zero() ) ), increased_max_energy(0)
  // Cooldown handled by ability
  {}

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool refresh = check() != 0;
    bool success = druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );

    if ( ! refresh && success )
      player -> resources.max[ RESOURCE_ENERGY ] += increased_max_energy;

    return success;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    player -> resources.max[ RESOURCE_ENERGY ] -= increased_max_energy;
    // Force energy down to cap if it's higher.
    player -> resources.current[ RESOURCE_ENERGY ] = std::min( player -> resources.current[ RESOURCE_ENERGY ],
        player -> resources.max[ RESOURCE_ENERGY ] );

    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );
  }
};

// Berserk Buff =============================================================

struct berserk_buff_t : public berserk_buff_base_t
{
  berserk_buff_t( druid_t& p ) :
    berserk_buff_base_t( p, "berserk", p.find_spell( 106951 ) )
  {
    default_value        = data().effectN( 1 ).percent(); // cost modifier
    increased_max_energy = data().effectN( 3 ).resource( RESOURCE_ENERGY );
  }
};

// Incarnation: King of the Jungle ==========================================

struct incarnation_cat_buff_t : public berserk_buff_base_t
{
  incarnation_cat_buff_t( druid_t& p ) :
    berserk_buff_base_t( p, "incarnation_king_of_the_jungle", p.talent.incarnation_cat )
  {
    default_value        = data().effectN( 2 ).percent(); // cost modifier
    increased_max_energy = data().effectN( 3 ).resource( RESOURCE_ENERGY );
  }
};

// Incarnation: Chosen of Elune ==========================================

struct incarnation_moonkin_buff_t : public druid_buff_t< buff_t >
{
  incarnation_moonkin_buff_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "incarnation_chosen_of_elune", p.talent.incarnation_moonkin )
      .default_value( p.talent.incarnation_moonkin -> effectN( 1 ).percent() )
      .cd( timespan_t::zero() )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  {}

  void expire_override( int stacks, timespan_adl_barrier::timespan_t duration ) override
  {
    druid_buff_t<buff_t>::expire_override( stacks, duration );

    druid.buff.star_power -> expire();
  }
};

// Cat Form =================================================================

struct cat_form_t : public druid_buff_t< buff_t >
{
  cat_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "cat_form", p.find_class_spell( "Cat Form" ) ) )
  {
    add_invalidate( CACHE_ATTACK_POWER );

    if ( p.spec.killer_instinct -> ok() )
      add_invalidate( CACHE_AGILITY );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( druid.caster_melee_attack, druid.caster_form_weapon );
    druid.buff.protection_of_ashamane -> trigger();
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    druid.buff.bear_form -> expire();
    druid.buff.moonkin_form -> expire();

    swap_melee( druid.cat_melee_attack, druid.cat_weapon );

    base_t::start( stacks, value, duration );
  }
};

// Celestial Alignment Buff =================================================

struct celestial_alignment_buff_t : public druid_buff_t < buff_t >
{
  celestial_alignment_buff_t( druid_t& p ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, "celestial_alignment", p.spec.celestial_alignment )
                          .cd( timespan_t::zero() ) // handled by spell
                          .default_value( p.spec.celestial_alignment -> effectN( 1 ).percent() )
                          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  {}

  void expire_override( int stacks, timespan_adl_barrier::timespan_t duration ) override
  {
    druid_buff_t<buff_t>::expire_override( stacks, duration );

    druid.buff.star_power -> expire();
  }
};

// Moonkin Form =============================================================

struct moonkin_form_t : public druid_buff_t< buff_t >
{
  moonkin_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "moonkin_form", p.spec.moonkin_form )
            .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
            .add_invalidate( CACHE_ARMOR )
            .chance( 1.0 ) )
  {}

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    druid.buff.bear_form -> expire();
    druid.buff.cat_form  -> expire();

    base_t::start( stacks, value, duration );
  }
};

// Warrior of Elune Buff ====================================================

struct warrior_of_elune_buff_t : public druid_buff_t<buff_t>
{
  warrior_of_elune_buff_t( druid_t& p ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, "warrior_of_elune", p.talent.warrior_of_elune ) )
  {}

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    // disabled for now since they'll probably institute this behavior later.
    // druid.cooldown.warrior_of_elune -> start();
  }
};

} // end namespace buffs

// Template for common druid action code. See priest_action_t.
template <class Base>
struct druid_action_t : public Base
{
  unsigned form_mask; // Restricts use of a spell based on form.
  bool may_autounshift; // Allows a spell that may be cast in NO_FORM but not in current form to be cast by exiting form.
  unsigned autoshift; // Allows a spell that may not be cast in the current form to be cast by automatically changing to the specified form.
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef druid_action_t base_t;

  bool rend_and_tear;
  bool hasted_gcd;
  double gore_chance;
  bool triggers_galactic_guardian;

  druid_action_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    form_mask( ab::data().stance_mask() ), may_autounshift( true ), autoshift( 0 ),
    rend_and_tear( ab::data().affected_by( player -> spec.thrash_bear_dot -> effectN( 2 ) ) ),
    hasted_gcd( ab::data().affected_by( player -> spec.druid -> effectN( 4 ) ) ),
    gore_chance( player -> spec.gore -> proc_chance() ), triggers_galactic_guardian( true )
  {
    ab::may_crit      = true;
    ab::tick_may_crit = true;
    ab::cooldown -> hasted = ab::data().affected_by( player -> spec.druid -> effectN( 3 ) );

    gore_chance += p() -> artifact.bear_hug.percent();
    gore_chance += p() -> sets.set( DRUID_GUARDIAN, T19, B2 ) -> effectN( 1 ).percent();
  }

  druid_t* p()
  { return static_cast<druid_t*>( ab::player ); }
  const druid_t* p() const
  { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double target_armor( player_t* t ) const override
  {
    double a = ab::target_armor( t );
    
    a *= 1.0 - td( t ) -> debuff.open_wounds -> value();

    return a;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    if ( rend_and_tear )
      tm *= 1.0 + p() -> talent.rend_and_tear -> effectN( 2 ).percent() * td( t ) -> dots.thrash_bear -> current_stack();

    return tm;
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( s -> target, s -> result_amount );

    trigger_galactic_guardian( s );
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( d -> target, d -> state -> result_amount );

    trigger_galactic_guardian( d -> state );
  }

  virtual void schedule_execute( action_state_t* s = nullptr ) override
  {
    if ( ! check_form_restriction() )
    {
      if ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM )
        p() -> shapeshift( NO_FORM );
      else if ( autoshift )
        p() -> shapeshift( ( form_e ) autoshift );
      else
        assert( "Action executed in wrong form with no valid form to shift to!" );
    }

    ab::schedule_execute( s );
  }

  /* Override this function for temporary effects that change the normal
     form restrictions of the spell. eg: Predatory Swiftness */
  virtual bool check_form_restriction()
  {
    return ! form_mask || ( form_mask & p() -> get_form() ) == p() -> get_form();
  }

  virtual bool ready() override
  {
    if ( ! check_form_restriction() && ! ( ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM ) || autoshift ) )
    {
      if ( ab::sim -> debug )
        ab::sim -> out_debug.printf( "%s ready() failed due to wrong form. form=%#.8x form_mask=%#.8x", ab::name(), p() -> get_form(), form_mask );

      return false;
    }

    return ab::ready();
  }

  void trigger_gushing_wound( player_t* t, double dmg )
  {
    if ( ! ( ab::special && ab::harmful && dmg > 0 ) )
      return;

    residual_action::trigger(
      p() -> active.gushing_wound, // ignite spell
      t, // target
      p() -> spec.gushing_wound -> effectN( 1 ).percent() * dmg );
  }

  bool trigger_gore()
  {
    if ( ab::rng().roll( gore_chance ) )
    {
      p() -> proc.gore -> occur();
      p() -> cooldown.mangle -> reset( true );
      p() -> buff.gore -> trigger();

      return true;
    }
    return false;
  }

  virtual void trigger_galactic_guardian( action_state_t* s )
  {
    if ( ! ( triggers_galactic_guardian && ! ab::proc && ab::harmful ) )
      return;
    if ( ! p() -> talent.galactic_guardian -> ok() )
      return;
    if ( s -> target == p() )
      return;
    if ( ! ab::result_is_hit( s -> result ) )
      return;
    if ( s -> result_total <= 0 )
      return;

    if ( ab::rng().roll( p() -> talent.galactic_guardian -> proc_chance() ) )
    {
      // Trigger Moonfire
      action_state_t* gg_s = p() -> active.galactic_guardian -> get_state();
      gg_s -> target = s -> target;
      p() -> active.galactic_guardian -> snapshot_state( gg_s, DMG_DIRECT );
      p() -> active.galactic_guardian -> schedule_execute( gg_s );

      // Trigger buff
      p() -> buff.galactic_guardian -> trigger();
    }
  }
};

// Druid melee attack base for cat_attack_t and bear_attack_t
template <class Base>
struct druid_attack_t : public druid_action_t< Base >
{
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_attack_t base_t;

  bool consumes_bloodtalons;
  snapshot_counter_t* bt_counter;
  snapshot_counter_t* tf_counter;
  bool direct_bleed;

  druid_attack_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ), consumes_bloodtalons( false ),
    bt_counter( nullptr ), tf_counter( nullptr ), direct_bleed( false )
  {
    ab::may_glance    = false;
    ab::special       = true;

    parse_special_effect_data();
  }

  void parse_special_effect_data()
  {
    for ( size_t i = 1; i <= this -> data().effect_count(); i++ )
    {
      const spelleffect_data_t& ed = this -> data().effectN( i );
      effect_type_t type = ed.type();

      // Check for bleed flag at effect or spell level.
      if ( ( type == E_SCHOOL_DAMAGE || type == E_WEAPON_PERCENT_DAMAGE )
           && ( ed.mechanic() == MECHANIC_BLEED || this -> data().mechanic() == MECHANIC_BLEED ) )
      {
        direct_bleed = true;
      }
    }
  }

  void init() override
  {
    ab::init();

    consumes_bloodtalons = ab::harmful && ab::special && ab::trigger_gcd > timespan_t::zero();

    if ( consumes_bloodtalons )
    {
      bt_counter = new snapshot_counter_t( ab::p(), ab::p() -> buff.bloodtalons );
      tf_counter = new snapshot_counter_t( ab::p(), ab::p() -> buff.tigers_fury );
    }
  }

  virtual timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == timespan_t::zero() )
      return g;

    if ( ab::hasted_gcd )
      g *= ab::p() -> cache.attack_haste();

    if ( g < ab::min_gcd )
      return ab::min_gcd;
    else
      return g;
  }

  void execute() override
  {
    ab::execute();

    if ( consumes_bloodtalons && ab::hit_any_target )
    {
      bt_counter -> count_execute();
      tf_counter -> count_execute();

      ab::p() -> buff.bloodtalons -> decrement();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::result_is_hit( s -> result ) && ! ab::special )
      trigger_clearcasting();
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    if ( consumes_bloodtalons )
    {
      bt_counter -> count_tick();
      tf_counter -> count_tick();
    }
  }

  virtual double target_armor( player_t* t ) const override
  {
    if ( direct_bleed )
      return 0.0;
    else
      return ab::target_armor( t );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    /* Assume that any action that deals physical and applies a dot deals all bleed damage, so
       that it scales direct "bleed" damage. This is a bad assumption if there is an action
       that applies a dot but does plain physical direct damage, but there are none of those. */
    if ( dbc::is_school( ab::school, SCHOOL_PHYSICAL ) && ab::dot_duration > timespan_t::zero() )
      tm *= 1.0 + ab::td( t ) -> debuff.bloodletting -> value();

    return tm;
  }

  void trigger_clearcasting()
  {
    if ( ab::proc )
      return;
    if ( ! ( ab::p() -> specialization() == DRUID_FERAL && ab::p() -> spec.omen_of_clarity -> ok() ) )
      return;
    
    // 5.25 PPM via http://us.battle.net/wow/en/forum/topic/20747154889#1
    double chance = ab::weapon -> proc_chance_on_swing( 5.25 );

    if ( ab::p() -> sets.has_set_bonus( DRUID_FERAL, T18, B2 ) )
      chance *= 1.0 + ab::p() -> sets.set( DRUID_FERAL, T18, B2 ) -> effectN( 1 ).percent();

    int active = ab::p() -> buff.clearcasting -> check();

    if ( ab::p() -> buff.clearcasting -> trigger(
           ab::p() -> buff.clearcasting -> max_stack(),
           buff_t::DEFAULT_VALUE(),
           chance,
           ab::p() -> buff.clearcasting -> buff_duration ) ) {
      ab::p() -> proc.clearcasting -> occur();

      for ( int i = 0; i < active; i++ )
        ab::p() -> proc.clearcasting_wasted -> occur();
    }
  }

};

// Druid "Spell" Base for druid_spell_t, druid_heal_t ( and potentially druid_absorb_t )
template <class Base>
struct druid_spell_base_t : public druid_action_t< Base >
{
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_spell_base_t base_t;
  
  bool cat_form_gcd;

  druid_spell_base_t( const std::string& n, druid_t* player,
                      const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    cat_form_gcd( ab::data().affected_by( player -> spec.cat_form -> effectN( 4 ) ) )
  {}

  virtual timespan_t gcd() const override
  {
    timespan_t g = ab::trigger_gcd;

    if ( g == timespan_t::zero() )
      return g;

    if ( cat_form_gcd && ab::p() -> buff.cat_form -> check() )
      g += ab::p() -> spec.cat_form -> effectN( 4 ).time_value();

    g *= ab::composite_haste();

    if ( g < ab::min_gcd )
      return ab::min_gcd;
    else
      return g;
  }
};

namespace spells {

/* druid_spell_t ============================================================
  Early definition of druid_spell_t. Only spells that MUST for use by other
  actions should go here, otherwise they can go in the second spells
  namespace.
========================================================================== */

struct druid_spell_t : public druid_spell_base_t<spell_t>
{
private:
  typedef druid_spell_base_t<spell_t> ab;
public:
  bool incarnation;
  bool celestial_alignment;
  bool blessing_of_elune;
  bool stellar_empowerment;

  druid_spell_t( const std::string& token, druid_t* p,
                 const spell_data_t* s      = spell_data_t::nil(),
                 const std::string& options = std::string() )
    : base_t( token, p, s ),
      incarnation( data().affected_by( p -> talent.incarnation_moonkin -> effectN( 4 ) ) ),
      celestial_alignment( data().affected_by( p -> spec.celestial_alignment -> effectN( 3 ) ) ),
      blessing_of_elune( data().affected_by( p -> spec.blessing_of_elune -> effectN( 1 ) ) ),
      stellar_empowerment( data().affected_by( p -> spec.stellar_empowerment -> effectN( 1 ) ) )
  {
    parse_options( options );
  }

  virtual double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = ab::composite_energize_amount( s );

    if ( energize_resource_() == RESOURCE_ASTRAL_POWER )
    {
      if ( blessing_of_elune && p() -> buff.blessing_of_elune -> up() )
        e *= 1.0 + p() -> spec.blessing_of_elune -> effectN( 1 ).percent();

      if ( celestial_alignment && p() -> buff.celestial_alignment -> check() )
        e *= 1.0 + p() -> spec.celestial_alignment -> effectN( 3 ).percent();

      if ( incarnation && p() -> buff.incarnation_moonkin -> check() )
        e *= 1.0 + p() -> talent.incarnation_moonkin -> effectN( 4 ).percent();
    }

    return e;
  }

  virtual void consume_resource() override
  {
    ab::consume_resource();

    trigger_impeccable_fel_essence();
  }

  virtual bool consume_cost_per_tick( const dot_t& dot ) override
  {
    bool ab = ab::consume_cost_per_tick( dot );
    trigger_impeccable_fel_essence();
    return ab;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    if ( stellar_empowerment && td( t ) -> debuff.stellar_empowerment -> up() )
    {
      tm *= 1.0 + composite_stellar_empowerment( t );
    }

    return tm;
  }

  virtual void execute() override
  {
    // Adjust buffs and cooldowns if we're in precombat.
    if ( ! p() -> in_combat )
    {
      if ( p() -> buff.incarnation_moonkin -> check() )
      {
        timespan_t time = std::max( std::max( min_gcd, trigger_gcd * composite_haste() ), base_execute_time * composite_haste() );
        p() -> buff.incarnation_moonkin -> extend_duration( p(), -time );
        p() -> buff.incarnation_proxy -> extend_duration( p(), -time );
        p() -> cooldown.incarnation -> adjust( -time );
      }
    }

    if ( harmful && trigger_gcd > timespan_t::zero() )
      p() -> buff.star_power -> up();

    ab::execute();

    trigger_star_power();
  }

  virtual void tick( dot_t* d ) override
  {
    ab::tick( d );

    // Benefit tracking
    if ( d -> state -> result_amount > 0 )
    {
      if ( hasted_ticks )
        p() -> buff.star_power -> up();
    }
  }

  virtual void trigger_balance_tier18_2pc()
  {
    if ( ! p() -> sets.has_set_bonus( DRUID_BALANCE, T18, B2 ) )
      return;

    if ( ! p() -> rppm.balance_tier18_2pc -> trigger() )
      return;

    for ( pet_t* pet : p() -> pet_fey_moonwing )
    {
      if ( pet -> is_sleeping() )
      {
        pet -> summon( timespan_t::from_seconds( 30 ) );
        return;
      }
    }
  }

  virtual void trigger_shooting_stars( action_state_t* s )
  {
    if ( ! p() -> talent.shooting_stars -> ok() )
      return;

    if ( rng().roll( p() -> talent.shooting_stars -> effectN( 1 ).percent() ) )
    {
      p() -> active.shooting_stars -> target = s -> target;
      p() -> active.shooting_stars -> execute();
    }
  }

  virtual void trigger_impeccable_fel_essence()
  {
    if ( p() -> legendary.impeccable_fel_essence == timespan_t::zero() )
      return;
    if ( resource_current != RESOURCE_ASTRAL_POWER )
      return;
    if ( resource_consumed <= 0.0 )
      return;

    timespan_t reduction = resource_consumed * p() -> legendary.impeccable_fel_essence;
    p() -> cooldown.celestial_alignment -> adjust( reduction );
    p() -> cooldown.incarnation -> adjust( reduction );
  }

  virtual void trigger_star_power()
  {
    if ( ! p() -> artifact.moon_and_stars.rank() )
      return;
    if ( ! ( ! background && trigger_gcd> timespan_t::zero() ) ) // TOCHECK
      return;
    if ( ! ( p() -> buff.celestial_alignment -> check() || p() -> buff.incarnation_moonkin -> check() ) )
      return;

    p() -> buff.star_power -> trigger();
  }

  // Empowerment multiplier functions to prevent putting this mess in several places.

  virtual double composite_stellar_empowerment( player_t* t ) const
  {
    double se = td( t ) -> debuff.stellar_empowerment -> check_value();
         
    se += p() -> mastery.starlight -> ok() * p() -> cache.mastery_value();
            
    se *= 1.0 + p() -> artifact.falling_star.percent();

    return se;
  }

  virtual double composite_solar_empowerment() const
  {
    double se = p() -> buff.solar_empowerment -> check_value();

    se += p() -> mastery.starlight -> ok() * p() -> cache.mastery_value();

    return se;
  }

  virtual double composite_lunar_empowerment() const
  {
    double le = p() -> buff.lunar_empowerment -> check_value();

    le += p() -> mastery.starlight -> ok() * p() -> cache.mastery_value();

    return le;
  }
}; // end druid_spell_t

// Shooting Stars ===========================================================

struct shooting_stars_t : public druid_spell_t
{
  shooting_stars_t( druid_t* player ) :
    druid_spell_t( "shooting_stars", player, player -> find_spell( 202497 ) )
  {
    background = true;
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
    moonfire_damage_t( druid_t* p ) :
      druid_spell_t( "moonfire_dmg", p, p -> find_spell( 164812 ) )
    {
      if ( p -> spec.astral_power -> ok() )
      {
        energize_resource = RESOURCE_ASTRAL_POWER;
        energize_amount   = p -> spec.astral_power -> effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
      }
      else
      {
        energize_type = ENERGIZE_NONE;
      }

      if ( p -> talent.shooting_stars -> ok() && ! p -> active.shooting_stars )
      {
        p -> active.shooting_stars = new shooting_stars_t( p );
      }

      if ( p -> talent.galactic_guardian->ok())
      {
        galactic_guardian_dd_multiplier = p->find_spell(213708)->effectN(3).percent();
      }

      may_miss = false;
      triggers_galactic_guardian = false;
      benefits_from_galactic_guardian = true;
      dual = background = true;
      dot_duration       += p -> spec.balance_overrides -> effectN( 4 ).time_value();
      base_dd_multiplier *= 1.0 + p -> spec.guardian -> effectN( 6 ).percent();

      /* June 2016: This hotfix is negated if you shift into Moonkin Form (ever),
        so only apply it if the druid does not have balance affinity. */
      if ( ! p -> talent.balance_affinity -> ok() )
      {
        base_dd_multiplier *= 1.0 + p -> spec.feral_overrides -> effectN( 1 ).percent();
        base_td_multiplier *= 1.0 + p -> spec.feral_overrides -> effectN( 2 ).percent();
      }

      base_multiplier *= 1.0 + p -> artifact.twilight_glow.percent();

      if ( !( p->specialization() == DRUID_FERAL ) )
      {
        // HOTFIX
        base_multiplier *= 1.1;
      }
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( ! t ) t = target;
      if ( ! t ) return nullptr;

      return td( t ) -> dots.moonfire;
    }

    virtual double action_da_multiplier() const override
    {
      double adm = druid_spell_t::action_da_multiplier();

      if ( benefits_from_galactic_guardian && p()->buff.galactic_guardian->check() )
      {
        // Galactic Guardian 7.1 Damage Buff
        adm *= 1.0 + galactic_guardian_dd_multiplier;
      }

      return adm;
    }

    void tick( dot_t* d ) override
    {
      druid_spell_t::tick( d );

      trigger_shooting_stars( d -> state );

      trigger_balance_tier18_2pc();
    }

    void impact ( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      // The buff needs to be handled with the damage handler since 7.1 since it impacts Moonfire DD
      if ( benefits_from_galactic_guardian && result_is_hit( s -> result ) && p() -> buff.galactic_guardian -> check() )
      {
        p() -> resource_gain( RESOURCE_RAGE, p() -> buff.galactic_guardian -> value(), p() -> gain.galactic_guardian );
        p() -> buff.galactic_guardian -> expire();
      }
    }
  };

  struct galactic_guardian_damage_t : public moonfire_damage_t
  {
    galactic_guardian_damage_t( druid_t* p ) :
      moonfire_damage_t( p )
    {
      benefits_from_galactic_guardian = false;
    }
  };

  moonfire_damage_t* damage;

  moonfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonfire", player, player -> find_class_spell( "Moonfire" ), options_str )
  {
    may_miss = false;
    damage = new moonfire_damage_t( player );
    damage -> stats = stats;

    if ( player -> spec.astral_power -> ok() )
    {
      energize_resource = RESOURCE_ASTRAL_POWER;
      energize_amount = player -> spec.astral_power -> effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
    }
    else
    {
      energize_type = ENERGIZE_NONE;
    }

    // Add damage modifiers in moonfire_damage_t, not here.
  }

  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      trigger_gore();
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    damage -> target = execute_state -> target;
    damage -> schedule_execute();
  }
};

}

namespace heals {

struct druid_heal_t : public druid_spell_base_t<heal_t>
{
  action_t* living_seed;
  bool target_self;

  druid_heal_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options_str = std::string() ) :
    base_t( token, p, s ),
    living_seed( nullptr ),
    target_self( 0 )
  {
    add_option( opt_bool( "target_self", target_self ) );
    parse_options( options_str );

    if ( target_self )
      target = p;

    may_miss          = false;
    weapon_multiplier = 0;
    harmful           = false;
  }

protected:
  void init_living_seed();

public:
  virtual void execute() override
  {
    base_t::execute();

    if ( base_execute_time > timespan_t::zero() )
      p() -> buff.soul_of_the_forest -> expire();

    if ( p() -> mastery.harmony -> ok() && spell_power_mod.direct > 0 && ! background )
      p() -> buff.harmony -> trigger( 1, p() -> mastery.harmony -> ok() ? p() -> cache.mastery_value() : 0.0 );
  }

  virtual double composite_haste() const override
  {
    double h = base_t::composite_haste();

    h *= 1.0 / ( 1.0 + p() -> buff.soul_of_the_forest -> value() );

    return h;
  }

  virtual double action_da_multiplier() const override
  {
    double adm = base_t::action_da_multiplier();

    adm *= 1.0 + p() -> buff.incarnation_tree -> check_value();

    if ( p() -> mastery.harmony -> ok() )
      adm *= 1.0 + p() -> cache.mastery_value();

    return adm;
  }

  virtual double action_ta_multiplier() const override
  {
    double adm = base_t::action_ta_multiplier();

    adm += p() -> buff.incarnation_tree -> check_value();

    adm += p() -> buff.harmony -> check_value();

    return adm;
  }

  void trigger_lifebloom_refresh( action_state_t* s )
  {
    druid_td_t& td = *this -> td( s -> target );

    if ( td.dots.lifebloom -> is_ticking() )
    {
      td.dots.lifebloom -> refresh_duration();

      if ( td.buff.lifebloom -> check() )
        td.buff.lifebloom -> refresh();
    }
  }

  void trigger_living_seed( action_state_t* s )
  {
    // Technically this should be a buff on the target, then bloom when they're attacked
    // For simplicity we're going to assume it always heals the target
    if ( living_seed )
    {
      living_seed -> base_dd_min = s -> result_amount;
      living_seed -> base_dd_max = s -> result_amount;
      living_seed -> execute();
    }
  }

  void trigger_clearcasting()
  {
    if ( ! proc && p() -> specialization() == DRUID_RESTORATION && p() -> spec.omen_of_clarity -> ok() )
      p() -> buff.clearcasting -> trigger(); // Proc chance is handled by buff chance
  }
}; // end druid_heal_t

}

namespace caster_attacks {

// Caster Form Melee Attack =================================================

struct caster_attack_t : public druid_attack_t < melee_attack_t >
{
  caster_attack_t( const std::string& token, druid_t* p,
                   const spell_data_t* s = spell_data_t::nil(),
                   const std::string& options = std::string() ) :
    base_t( token, p, s )
  {
    parse_options( options );
  }
}; // end druid_caster_attack_t

struct druid_melee_t : public caster_attack_t
{
  druid_melee_t( druid_t* p ) :
    caster_attack_t( "melee", p )
  {
    school      = SCHOOL_PHYSICAL;
    may_glance  = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special     = false;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return caster_attack_t::execute_time();
  }
};

}

namespace cat_attacks {

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

struct cat_attack_t : public druid_attack_t < melee_attack_t >
{
protected:
  bool    attack_critical;
public:
  bool    requires_stealth;
  bool    consumes_combo_points;
  bool    consumes_clearcasting;
  bool    trigger_tier17_2pc;
  struct {
    bool direct, tick;
  } razor_claws;
  bool    snapshots_tf;
  bool    snapshots_sr;

  cat_attack_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s ),
    requires_stealth( false ),
    consumes_combo_points( false ),
    consumes_clearcasting( true ),
    trigger_tier17_2pc( false ),
    snapshots_tf( true ),
    snapshots_sr( true )
  {
    parse_options( options );
    
    if ( data().cost( POWER_COMBO_POINT ) )
    {
      consumes_combo_points = true;
      form_mask |= CAT_FORM;
    }

    razor_claws.direct = data().affected_by( p -> mastery.razor_claws -> effectN( 1 ) );
    razor_claws.tick = data().affected_by( p -> mastery.razor_claws -> effectN( 2 ) );

    // Apply all Feral Affinity damage modifiers.
    if ( p -> talent.feral_affinity -> ok() )
    {
      for ( size_t i = 1; i <= p -> talent.feral_affinity -> effect_count(); i++ )
      {
        const spelleffect_data_t& effect = p -> talent.feral_affinity -> effectN( i );

        if ( ! ( effect.type() == E_APPLY_AURA && effect.subtype() == A_ADD_PCT_MODIFIER ) )
          continue;

        if ( data().affected_by( effect ) )
          base_multiplier *= 1.0 + effect.percent();
      }
    }
  }

  // For effects that specifically trigger only when "prowling."
  virtual bool prowling() const
  {
    // Make sure we call all three methods for accurate benefit tracking.
    bool prowl = p() -> buff.prowl -> up(),
         inc   = p() -> buff.incarnation_cat -> up();

    return prowl || inc;
  }

  virtual bool stealthed() const // For effects that require any form of stealth.
  {
    // Make sure we call all three methods for accurate benefit tracking.
    bool shadowmeld = p() -> buffs.shadowmeld -> up(),
         prowl = prowling();

    return prowl || shadowmeld;
  }

  virtual double cost() const override
  {
    double c = base_t::cost();

    if ( c == 0 )
      return 0;

    if ( consumes_clearcasting && current_resource() == RESOURCE_ENERGY &&
      p() -> buff.clearcasting -> check() )
      return 0;

    c *= 1.0 + p() -> buff.berserk -> check_value();
    c *= 1.0 + p() -> buff.incarnation_cat -> check_value();

    return c;
  }

  virtual void consume_resource() override
  {
    // Treat Omen of Clarity energy savings like an energy gain for tracking purposes.
    if ( consumes_clearcasting && current_resource() == RESOURCE_ENERGY &&
      base_t::cost() > 0 && p() -> buff.clearcasting -> up() )
    {
      // Base cost doesn't factor in Berserk, but Omen of Clarity does net us less energy during it, so account for that here.
      double eff_cost = base_t::cost();
      eff_cost *= 1.0 + p() -> buff.berserk -> value();
      eff_cost *= 1.0 + p() -> buff.incarnation_cat -> check_value();

      p() -> gain.clearcasting -> add( RESOURCE_ENERGY, eff_cost );

      // Feral tier18 4pc occurs before the base cost is consumed.
      if ( p() -> sets.has_set_bonus( DRUID_FERAL, T18, B4 ) )
      {
        p() -> resource_gain( RESOURCE_ENERGY,
          base_t::cost() * p() -> sets.set( DRUID_FERAL, T18, B4 ) -> effectN( 1 ).percent(),
          p() -> gain.feral_tier18_4pc );
      }
    }

    base_t::consume_resource();

    if ( consumes_clearcasting && current_resource() == RESOURCE_ENERGY && base_t::cost() > 0 )
      p() -> buff.clearcasting -> decrement();

    if ( consumes_combo_points && hit_any_target )
    {
      int consumed = ( int ) p() -> resources.current[ RESOURCE_COMBO_POINT ];

      p() -> resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );

      if ( sim -> log )
      {
        sim -> out_log.printf( "%s consumes %d %s for %s (0)",
          player -> name(),
          consumed,
          util::resource_type_string( RESOURCE_COMBO_POINT ),
          name() );
      }

      stats -> consume_resource( RESOURCE_COMBO_POINT, consumed );

      if ( p() -> spec.predatory_swiftness -> ok() )
        p() -> buff.predatory_swiftness -> trigger( 1, 1, consumed * 0.20 );

      if ( p() -> talent.soul_of_the_forest -> ok() && p() -> specialization() == DRUID_FERAL )
      {
        p() -> resource_gain( RESOURCE_ENERGY,
          consumed * p() -> talent.soul_of_the_forest -> effectN( 1 ).resource( RESOURCE_ENERGY ),
          p() -> gain.soul_of_the_forest );
      }
    }
  }

  virtual double composite_target_crit_chance( player_t* t ) const override
  {
    double tc = base_t::composite_target_crit_chance( t );

    if ( special && t -> debuffs.bleeding -> check() )
      tc += p() -> talent.blood_scent -> effectN( 1 ).percent();

    return tc;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    // Cancel out the multipliers from druid_t::composite_player_multiplier.
    if ( snapshots_tf )
      am /= 1.0 + p() -> buff.tigers_fury -> check_value();

    if ( snapshots_sr )
      am /= 1.0 + p() -> buff.savage_roar -> check_value();

    return am;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = base_t::composite_persistent_multiplier( s );

    if ( consumes_bloodtalons )
      pm *= 1.0 + p() -> buff.bloodtalons -> check_value();
    
    if ( snapshots_tf )
      pm *= 1.0 + p() -> buff.tigers_fury -> check_value();

    if ( snapshots_sr )
      pm *= 1.0 + p() -> buff.savage_roar -> check_value();

    return pm;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double dm = base_t::composite_da_multiplier( state );

    /* Modifiers for direct bleed damage. */
    if ( razor_claws.direct )
      dm *= 1.0 + p() -> cache.mastery_value();

    return dm;
  }

  virtual double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double tm = base_t::composite_ta_multiplier( s );

    if ( razor_claws.tick ) // Don't need to check school, it modifies all periodic damage.
      tm *= 1.0 + p() -> cache.mastery_value();

    return tm;
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) && s -> result == RESULT_CRIT )
      attack_critical = true;
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    trigger_predator();
    trigger_wildshapers_clutch( d -> state );

    if ( trigger_tier17_2pc )
    {
      p() -> resource_gain( RESOURCE_ENERGY,
        p() -> sets.set( DRUID_FERAL, T17, B2 ) -> effectN( 1 ).base_value(),
        p() -> gain.feral_tier17_2pc );
    }
  }

  virtual void execute() override
  {
    attack_critical = false;

    base_t::execute();

    if ( energize_resource == RESOURCE_COMBO_POINT && energize_amount > 0 && hit_any_target )
    {
      trigger_ashamanes_rip();

      if ( attack_critical )
      {
        trigger_primal_fury();
      }
    }

    if ( ! hit_any_target )
      trigger_energy_refund();

    if ( harmful )
    {
      p() -> buff.prowl -> expire();
      p() -> buffs.shadowmeld -> expire();

      // Track benefit of damage buffs
      p() -> buff.tigers_fury -> up();
      p() -> buff.savage_roar -> up();
      p() -> buff.feral_instinct -> up();
      if ( special && base_costs[ RESOURCE_ENERGY ] > 0 )
        p() -> buff.berserk -> up();
    }
  }

  virtual bool ready() override
  {
    if ( requires_stealth && ! stealthed() )
      return false;

    if ( consumes_combo_points && p() -> resources.current[ RESOURCE_COMBO_POINT ] < 1 )
      return false;

    return base_t::ready();
  }

  void trigger_energy_refund()
  {
    player -> resource_gain( RESOURCE_ENERGY, resource_consumed * 0.80,
      p() -> gain.energy_refund );
  }

  virtual void trigger_primal_fury()
  {
    if ( ! p() -> spec.primal_fury -> ok() )
      return;

    p() -> proc.primal_fury -> occur();
    p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spec.primal_fury ->
      effectN( 1 ).base_value(), p() -> gain.primal_fury );
  }

  void trigger_predator()
  {
    if ( ! ( p() -> talent.predator -> ok() && p() -> predator_rppm_rate > 0 ) )
      return;
    if ( ! p() -> rppm.predator -> trigger() )
      return;

    if ( ! p() -> cooldown.tigers_fury -> down() )
      p() -> proc.predator_wasted -> occur();

    p() -> cooldown.tigers_fury -> reset( true );
    p() -> proc.predator -> occur();
  }

  void trigger_wildshapers_clutch( action_state_t* s )
  {
    if ( p() -> legendary.the_wildshapers_clutch == 0.0 )
      return;
    if ( ! dbc::is_school( school, SCHOOL_PHYSICAL ) ) // bleeds only
      return;
    if ( s -> result != RESULT_CRIT )
      return;
    if ( s -> result_amount <= 0 )
      return;

    if ( p() -> rng().roll( p() -> legendary.the_wildshapers_clutch ) )
    {
      p() -> proc.the_wildshapers_clutch -> occur();
      trigger_primal_fury();
    }
  }

  void trigger_ashamanes_rip()
  {
    if ( ! p() -> artifact.ashamanes_bite.rank() )
      return;
    if ( ! ( energize_resource == RESOURCE_COMBO_POINT && energize_amount > 0 ) )
      return;
    if ( ! hit_any_target )
      return;

    if ( p() -> rng().roll( p() -> artifact.ashamanes_bite.data().proc_chance() ) )
    {
      if ( td( execute_state -> target ) -> dots.rip -> is_ticking() )
      {
        p() -> active.ashamanes_rip -> target = execute_state -> target;
        p() -> active.ashamanes_rip -> schedule_execute();
      }
    }
  }
}; // end druid_cat_attack_t

// Cat Melee Attack =========================================================

struct cat_melee_t : public cat_attack_t
{
  cat_melee_t( druid_t* player ) :
    cat_attack_t( "cat_melee", player, spell_data_t::nil(), "" )
  {
    form_mask = CAT_FORM;

    school = SCHOOL_PHYSICAL;
    may_glance = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special = false;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return cat_attack_t::execute_time();
  }

  virtual double action_multiplier() const override
  {
    double cm = cat_attack_t::action_multiplier();

    if ( p() -> buff.cat_form -> check() )
      cm *= 1.0 + p() -> buff.cat_form -> data().effectN( 3 ).percent();

    return cm;
  }
};

// Rip State ================================================================

struct rip_state_t : public action_state_t
{
  int combo_points;
  druid_t* druid;

  rip_state_t( druid_t* p, action_t* a, player_t* target ) :
    action_state_t( a, target ), combo_points( 0 ), druid( p )
  { }

  void initialize() override
  {
    action_state_t::initialize();

    combo_points = ( int ) druid -> resources.current[ RESOURCE_COMBO_POINT ];
  }

  void copy_state( const action_state_t* state ) override
  {
    action_state_t::copy_state( state );
    const rip_state_t* rip_state = debug_cast<const rip_state_t*>( state );
    combo_points = rip_state -> combo_points;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );

    s << " combo_points=" << combo_points;

    return s;
  }
};

// Ashamane's Frenzy ========================================================

struct ashamanes_frenzy_driver_t : public cat_attack_t
{
  struct ashamanes_frenzy_t : public cat_attack_t
  {
    struct ashamanes_frenzy_ignite_t : public residual_action::residual_periodic_action_t<cat_attack_t>
    {
      ashamanes_frenzy_ignite_t( druid_t* p ) :
        base_t( "ashamanes_frenzy_ignite", p, p -> find_spell( 210723 ) )
      {
        dual = true;
        tick_may_crit = true; // Sep 22 2016
      }

      void init() override
      {
        base_t::init();

        snapshot_flags |= STATE_CRIT | STATE_TGT_CRIT;
      }
    };

    ashamanes_frenzy_ignite_t* ignite;
    double ignite_multiplier;

    ashamanes_frenzy_t( druid_t* p ) :
      cat_attack_t( "ashamanes_frenzy_hit", p, p -> find_spell( 210723 ) ),
      ignite( new ashamanes_frenzy_ignite_t( p ) )
    {
      background = dual = true;
      snapshots_sr = snapshots_tf = false; // Sep 22 2016
      consumes_bloodtalons = false; // BT is applied by driver.
      ignite_multiplier = dot_duration / data().effectN( 3 ).period();
      dot_duration = timespan_t::zero(); // DoT is applied by ignite action.
    }

    void impact( action_state_t* s ) override
    {
      cat_attack_t::impact( s );

      residual_action::trigger( ignite, s -> target, s -> result_raw * ignite_multiplier );
    }
  };

  ashamanes_frenzy_t* damage;

  ashamanes_frenzy_driver_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "ashamanes_frenzy", p, &p -> artifact.ashamanes_frenzy.data(), options_str ),
    damage( new ashamanes_frenzy_t( p ) )
  {
    may_miss = may_parry = may_dodge = may_crit = tick_may_crit = false;
    damage -> stats = damage -> ignite -> stats = stats;
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  { ( void ) s; assert( s -> result_amount == 0.0 ); }

  // Assure the only persistent multiplier here is Bloodtalons.
  double composite_persistent_multiplier( const action_state_t* ) const override
  { return 1.0 + p() -> buff.bloodtalons -> check_value(); }

  void tick( dot_t* d ) override
  {
    cat_attack_t::tick( d );

    // Create & snapshot state
    action_state_t* s = damage -> get_state();
    s -> target = d -> state -> target;
    damage -> snapshot_state( s, DMG_DIRECT );
    // Apply Bloodtalons from driver.
    s -> persistent_multiplier *= d -> state -> persistent_multiplier;
    damage -> schedule_execute( s );
  }

  // Does not trigger primal fury.
  void trigger_primal_fury() override {}
};

// Ashamane's Rip ===========================================================

struct ashamanes_rip_t : public cat_attack_t
{
  ashamanes_rip_t( druid_t* p ) :
    cat_attack_t( "ashamanes_rip", p, p -> find_spell( 210705 ) )
  {
    background = true;
    may_miss = may_block = may_dodge = may_parry = false;
    // Copies benefit from rip, so need to flag this as snapshotting so its damage doesn't get modified dynamically.
    snapshots_tf = snapshots_sr = true;
      
    base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + p -> artifact.razor_fangs.percent();
  }

  action_state_t* new_state() override
  { return new rip_state_t( p(), this, target ); }

  void snapshot_state( action_state_t* s, dmg_e rt ) override
  {
    assert( td( s -> target ) -> dots.rip -> is_ticking() );

    cat_attack_t::snapshot_state( s, rt );

    rip_state_t* shadow_rip = debug_cast<rip_state_t*>( s );
    rip_state_t* rip = debug_cast<rip_state_t*>( td( s -> target ) -> dots.rip -> state );

    shadow_rip -> combo_points = rip -> combo_points;
    shadow_rip -> persistent_multiplier = rip -> persistent_multiplier;
  }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    rip_state_t* s_rip = debug_cast<rip_state_t*>( td( s -> target ) -> dots.shadow_rip -> state );

    if ( ! s_rip )
      return 0;

    return cat_attack_t::attack_tick_power_coefficient( s ) * s_rip -> combo_points;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  { return td( s -> target ) -> dots.rip -> remains(); }

  virtual void execute() override
  {
    assert( td( target ) -> dots.rip -> is_ticking() );

    cat_attack_t::execute();
  }
};

// Berserk ==================================================================

struct berserk_t : public cat_attack_t
{
  berserk_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "berserk", player, player -> find_specialization_spell( "Berserk" ), options_str )
  {
    harmful = consumes_clearcasting = may_miss = may_parry = may_dodge = may_crit = false;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p() -> buff.berserk -> trigger();
    p() -> buff.feral_instinct -> trigger();

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
      p() -> buff.feral_tier17_4pc -> trigger();
  }

  bool ready() override
  {
    if ( p() -> talent.incarnation_cat -> ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Brutal Slash =============================================================

struct brutal_slash_t : public cat_attack_t
{
  brutal_slash_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "brutal_slash", p, p -> talent.brutal_slash, options_str )
  {
    aoe = -1;
    energize_amount = data().effectN( 1 ).base_value();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type = ENERGIZE_ON_HIT;
    cooldown -> hasted = true;

    base_multiplier *= 1.0 + p -> artifact.sharpened_claws.percent();
  }

  double cost() const override
  {
    double c = cat_attack_t::cost();

    // TOCHECK
    double reduction = p() -> buff.scent_of_blood -> check_value();
    reduction *= 1.0 + p() -> buff.berserk -> check_value();
    reduction *= 1.0 + p() -> buff.incarnation_cat -> check_value();
    c += reduction;

    return c;
  }
};

// Ferocious Bite ===========================================================

struct ferocious_bite_t : public cat_attack_t
{
  double excess_energy;
  double max_excess_energy;
  bool max_energy;

  ferocious_bite_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "ferocious_bite", p, p -> find_affinity_spell( "Ferocious Bite" ) ),
    excess_energy( 0 ), max_excess_energy( 0 ), max_energy( false )
  {
    add_option( opt_bool( "max_energy" , max_energy ) );
    parse_options( options_str );

    max_excess_energy  = -1 * data().effectN( 2 ).base_value();
    special            = true;
    energize_type      = ENERGIZE_NONE; // disable negative energy gain in spell data

    crit_bonus_multiplier *= 1.0 + p -> artifact.powerful_bite.percent();
  } 

  double maximum_energy() const
  {
    double req = base_costs[ RESOURCE_ENERGY ] + max_excess_energy;

    req *= 1.0 + p() -> buff.berserk -> check_value();
    req *= 1.0 + p() -> buff.incarnation_cat -> check_value();

    if ( p() -> buff.clearcasting -> check() )
      req /= 2.0;

    return req;
  }

  bool ready() override
  {
    if ( max_energy && p() -> resources.current[ RESOURCE_ENERGY ] < maximum_energy() )
      return false;

    return cat_attack_t::ready();
  }

  void execute() override
  {
    // Berserk does affect the additional energy consumption.
    max_excess_energy *= 1.0 + p() -> buff.berserk -> value();
    max_excess_energy *= 1.0 + p() -> buff.incarnation_cat -> check_value();

    excess_energy = std::min( max_excess_energy,
      ( p() -> resources.current[ RESOURCE_ENERGY ] - cat_attack_t::cost() ) );

    cat_attack_t::execute();

    max_excess_energy = -1 * data().effectN( 2 ).base_value();
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( s -> target -> health_percentage() <= 25 || p() -> talent.sabertooth -> ok() )
      {
        td( s -> target ) -> dots.rip -> refresh_duration( 0 );
      }
    }
  }

  void consume_resource() override
  {
    /* Extra energy consumption happens first. In-game it happens before the
       skill even casts but let's not do that because its dumb. */
    if ( hit_any_target )
    {
      player -> resource_loss( current_resource(), excess_energy );
      stats -> consume_resource( current_resource(), excess_energy );
    }

    cat_attack_t::consume_resource();
  }

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    am *= p() -> resources.current[ RESOURCE_COMBO_POINT ]
      / p() -> resources.max[ RESOURCE_COMBO_POINT ];

    am *= 1.0 + excess_energy / max_excess_energy;

    return am;
  }
};

// Gushing Wound (tier17_feral_4pc) =========================================

struct gushing_wound_t : public residual_action::residual_periodic_action_t<cat_attack_t>
{
  gushing_wound_t( druid_t* p ) :
    residual_action::residual_periodic_action_t<cat_attack_t>( "gushing_wound", p, p -> find_spell( 166638 ) )
  {
    background = dual = proc = true;
    consumes_clearcasting = may_miss = may_dodge = may_parry = false;
    trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );
  }
};

// Lunar Inspiration ========================================================

struct lunar_inspiration_t : public cat_attack_t
{
  lunar_inspiration_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "lunar_inspiration", player, player -> find_spell( 155625 ), options_str )
  {
    may_dodge = may_parry = may_block = may_glance = false;
    snapshots_sr = snapshots_tf = false; // June 6 2016
    hasted_ticks = true;
  }

  void init() override
  {
    cat_attack_t::init();

    consumes_bloodtalons = false;
  }

  virtual bool ready() override
  {
    if ( ! p() -> talent.lunar_inspiration -> ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Maim =====================================================================

struct maim_t : public cat_attack_t
{
  maim_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "maim", player, player -> find_specialization_spell( "Maim" ), options_str )
  {
    weapon_multiplier = data().effectN( 3 ).pp_combo_points() / 100.0;
  }

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    am *= p() -> resources.current[ RESOURCE_COMBO_POINT ];

    return am;
  }
};

// Rake =====================================================================

struct rake_t : public cat_attack_t
{
  struct rake_bleed_t : public cat_attack_t
  {
    rake_bleed_t( druid_t* p ) :
      cat_attack_t( "rake_bleed", p, p -> find_spell( 155722 ) )
    {
      background = dual = true;
      may_miss = may_parry = may_dodge = may_crit = false;

      base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
      dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();
      
      // Direct damage modifiers go in rake_t!
      trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );
      base_multiplier *= 1.0 + p -> artifact.tear_the_flesh.percent();
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( ! t ) t = target;
      if ( ! t ) return nullptr;

      return td( t ) -> dots.rake;
    }
  };

  action_t* bleed;

  rake_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rake", p, p -> find_affinity_spell( "Rake" ) )
  {
    parse_options( options_str );

    bleed = p -> find_action( "rake_bleed" );

    if ( ! bleed )
    {
      bleed = new rake_bleed_t( p );
      bleed -> stats = stats;
    }

    // Periodic damage modifiers go in rake_bleed_t!
    trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );
    base_multiplier *= 1.0 + p -> artifact.tear_the_flesh.percent();
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = cat_attack_t::composite_persistent_multiplier( s );

    if ( stealthed() )
      pm *= 1.0 + data().effectN( 4 ).percent();

    return pm;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    action_state_t* b_state = bleed -> get_state();
    b_state -> target = s -> target;
    bleed -> snapshot_state( b_state, DMG_OVER_TIME );
    // Copy persistent multipliers from the direct attack.
    b_state -> persistent_multiplier = s -> persistent_multiplier;
    bleed -> schedule_execute( b_state );
  }
};

// Rip ======================================================================

struct rip_t : public cat_attack_t
{
  rip_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rip", p, p -> find_affinity_spell( "Rip" ), options_str )
  {
    special      = true;
    may_crit     = false;

    trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );

    base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
    dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> artifact.razor_fangs.percent();
  }

  action_state_t* new_state() override
  { return new rip_state_t( p(), this, target ); }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    /* FIXME: Does this even work correctly for tick_damage expression?
     probably just uses the CP of the Rip already on the target */
    rip_state_t* rip_state = debug_cast<rip_state_t*>( td( s -> target ) -> dots.rip -> state );

    if ( ! rip_state )
      return 0;

    return cat_attack_t::attack_tick_power_coefficient( s ) * rip_state -> combo_points;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> debuff.open_wounds -> trigger();
  }

  void last_tick( dot_t* d ) override
  {
    cat_attack_t::last_tick( d );

    td( d -> target ) -> debuff.open_wounds -> expire();
  }
};

// Savage Roar ==============================================================

struct savage_roar_t : public cat_attack_t
{
  savage_roar_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "savage_roar", p, p -> talent.savage_roar, options_str )
  {
    consumes_combo_points = true; // Missing from spell data.
    may_crit = may_miss = harmful = false;
    dot_duration = base_tick_time = timespan_t::zero();
  }

  /* We need a custom implementation of Pandemic refresh mechanics since our ready()
     method relies on having the correct final value. */
  timespan_t duration( int combo_points = -1 )
  {
    if ( combo_points == -1 )
      combo_points = ( int ) p() -> resources.current[ RESOURCE_COMBO_POINT ];

    timespan_t d = data().duration() + timespan_t::from_seconds( 4.0 ) * combo_points;

    // Maximum duration is 130% of the raw duration of the new Savage Roar.
    if ( p() -> buff.savage_roar -> check() )
      d += std::min( p() -> buff.savage_roar -> remains(), d * 0.3 );

    return d;
  }

  virtual void execute() override
  {
    // Grab duration before we go and spend all of our combo points.
    timespan_t d = duration();

    cat_attack_t::execute();

    p() -> buff.savage_roar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, d );
  }

  virtual bool ready() override
  {
    // Savage Roar may not be cast if the new duration is less than that of the current
    if ( duration() < p() -> buff.savage_roar -> remains() )
      return false;

    return cat_attack_t::ready();
  }
};

// Shred ====================================================================

struct shred_t : public cat_attack_t
{
  shred_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "shred", p, p -> find_affinity_spell( "Shred" ), options_str )
  {
    base_crit += p -> artifact.feral_power.percent();
    base_multiplier *= 1.0 + p -> artifact.shredder_fangs.percent();
  }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT && p() -> sets.has_set_bonus( DRUID_FERAL, PVP, B4 ) )
      td( s -> target ) -> debuff.bloodletting -> trigger(); // Druid module debuff
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> spec.shred -> effectN( 5 ).percent();

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, T19, B4 ) )
    {
      tm *= 1.0 + td( t ) -> feral_tier19_4pc_bleeds() *
        p() -> sets.set( DRUID_FERAL, T19, B4 ) -> effectN( 1 ).percent();
    }

    return tm;
  }

  double composite_crit_chance_multiplier() const override
  {
    double cm = cat_attack_t::composite_crit_chance_multiplier();

    if ( stealthed() )
      cm *= 2.0;

    return cm;
  }

  double action_multiplier() const override
  {
    double m = cat_attack_t::action_multiplier();

    if ( stealthed() )
      m *= 1.0 + data().effectN( 4 ).percent();

    return m;
  }
};

// Swipe (Cat) ====================================================================

struct swipe_cat_t : public cat_attack_t
{
public:
  swipe_cat_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "swipe_cat", player, player -> find_affinity_spell( "Swipe" ) -> ok() ?
      player -> spec.swipe_cat : spell_data_t::not_found(), options_str )
  {
    aoe = -1;
    energize_amount = data().effectN( 1 ).percent();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type = ENERGIZE_ON_HIT;

    base_multiplier *= 1.0 + player -> artifact.sharpened_claws.percent();
  }

  double cost() const override
  {
    double c = cat_attack_t::cost();

    // TOCHECK
    double reduction = p() -> buff.scent_of_blood -> check_value();
    reduction *= 1.0 + p() -> buff.berserk -> check_value();
    reduction *= 1.0 + p() -> buff.incarnation_cat -> check_value();
    c += reduction;

    return c;
  }

  virtual void execute() override
  {
    cat_attack_t::execute();

    p() -> buff.scent_of_blood -> up();
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 2 ).percent();

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, T19, B4 ) )
    {
      tm *= 1.0 + td( t ) -> feral_tier19_4pc_bleeds() *
        p() -> sets.set( DRUID_FERAL, T19, B4 ) -> effectN( 1 ).percent();
    }

    return tm;
  }

  virtual bool ready() override
  {
    if ( p() -> talent.brutal_slash -> ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Tiger's Fury =============================================================

struct tigers_fury_t : public cat_attack_t
{
  timespan_t duration;

  tigers_fury_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "tigers_fury", p, p -> find_specialization_spell( "Tiger's Fury" ), options_str ),
    duration( p -> buff.tigers_fury -> buff_duration )
  {
    harmful = consumes_clearcasting = may_miss = may_parry = may_dodge = may_crit = false;
    autoshift = form_mask = CAT_FORM;
    energize_type = ENERGIZE_ON_CAST;
    energize_amount += p -> spec.tigers_fury_2 -> effectN( 1 ).resource( RESOURCE_ENERGY );
  }

  void execute() override
  {
    cat_attack_t::execute();

    p() -> buff.tigers_fury -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );

    p() -> buff.ashamanes_energy -> trigger();
  }
};

// Thrash (Cat) =============================================================

struct thrash_cat_t : public cat_attack_t
{
  struct shadow_thrash_t : public cat_attack_t
  {
    struct shadow_thrash_tick_t : public cat_attack_t
    {
      shadow_thrash_tick_t( druid_t* p ) :
        cat_attack_t( "shadow_thrash", p, p -> find_spell( 210687 ) )
      {
        background = dual = true;
        aoe = -1;
      }
    };

    shadow_thrash_t( druid_t* p ) :
      cat_attack_t( "shadow_thrash", p, p -> artifact.shadow_thrash.data().effectN( 1 ).trigger() )
    {
      background = true;
      tick_action = new shadow_thrash_tick_t( p );
    
      base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
      dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();
    }
  };

  thrash_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "thrash_cat", p, p -> find_spell( 106830 ), options_str )
  {
    aoe = -1;
    spell_power_mod.direct = 0;
    
    trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );

    if ( p -> sets.has_set_bonus( DRUID_FERAL, T19, B2 ) )
    {
      // No value in spell data.
      energize_amount = 1;
      energize_resource = RESOURCE_COMBO_POINT;
      energize_type = ENERGIZE_ON_HIT;
    }

    if ( p -> artifact.shadow_thrash.rank() && ! p -> active.shadow_thrash )
    {
      p -> active.shadow_thrash = new shadow_thrash_t( p );
      add_child( p -> active.shadow_thrash );
    }
    
    base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
    dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> artifact.jagged_claws.percent();
  }

  void execute() override
  {
    cat_attack_t::execute();

    p() -> buff.scent_of_blood -> trigger( 1,
      num_targets_hit * p() -> buff.scent_of_blood -> default_value );

    if ( rng().roll( p() -> artifact.shadow_thrash.data().proc_chance() ) )
      p() -> active.shadow_thrash -> schedule_execute();
  }
};

} // end namespace cat_attacks

namespace bear_attacks {

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
  bool gore;

  bear_attack_t( const std::string& n, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options_str = std::string() ) :
    base_t( n, p, s ), gore( false )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    base_t::execute();

    if ( hit_any_target && gore )
      trigger_gore();
  }
}; // end bear_attack_t

// Bear Melee Attack ========================================================

struct bear_melee_t : public bear_attack_t
{
  bear_melee_t( druid_t* player ) :
    bear_attack_t( "bear_melee", player )
  {
    form_mask = BEAR_FORM;

    school      = SCHOOL_PHYSICAL;
    may_glance  = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special     = false;

    energize_type     = ENERGIZE_ON_HIT;
    energize_resource = RESOURCE_RAGE;
    energize_amount   = 7.875; // http://us.battle.net/wow/en/forum/topic/20743504316?page=13#248
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return bear_attack_t::execute_time();
  }
};

// Growl ====================================================================

struct growl_t: public bear_attack_t
{
  growl_t( druid_t* player, const std::string& options_str ):
    bear_attack_t( "growl", player, player -> find_class_spell( "Growl" ), options_str )
  {
    ignore_false_positive = true;
    may_miss = may_parry = may_dodge = may_block = may_crit = false;
    use_off_gcd = true;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation_bear -> up() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    bear_attack_t::impact( s );
  }
};

// Mangle ===================================================================

struct mangle_t : public bear_attack_t
{
  double bleeding_multiplier;

  mangle_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "mangle", player, player -> find_affinity_spell( "Mangle" ), options_str )
  {
    bleeding_multiplier = data().effectN( 3 ).percent();

    if ( p() -> specialization() == DRUID_GUARDIAN )
    {
      energize_amount += player -> talent.soul_of_the_forest -> effectN( 1 ).resource( RESOURCE_RAGE );
      base_multiplier *= 1.0 + player -> talent.soul_of_the_forest -> effectN( 2 ).percent();
    }

    base_multiplier *= 1.0 + player -> artifact.vicious_bites.percent();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double em = bear_attack_t::composite_energize_amount( s );

    em += p() -> buff.gore -> value();

    return em;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation_bear -> up() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = bear_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> check() )
      tm *= 1.0 + bleeding_multiplier;

    return tm;
  }

  int n_targets() const override
  {
    if ( p() -> buff.incarnation_bear -> up() )
      return p() -> buff.incarnation_bear -> data().effectN( 4 ).base_value();

    return bear_attack_t::n_targets();
  }

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> buff.guardian_of_elune -> trigger();
  }

  void execute() override
  {
    bear_attack_t::execute();

    p() -> buff.gore -> expire();

    p() -> buff.guardian_tier19_4pc -> trigger();
    p() -> buff.gory_fur -> trigger();
  }
};

// Maul =====================================================================

struct maul_t : public bear_attack_t
{
  maul_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "maul", player, player -> find_specialization_spell( "Maul" ), options_str )
  {
    weapon = &( player -> main_hand_weapon );
    use_off_gcd = true;

    base_crit += player -> artifact.mauler.percent();
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation_bear -> up() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }
};

// Pulverize ================================================================

struct pulverize_t : public bear_attack_t
{
  pulverize_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "pulverize", player, player -> talent.pulverize, options_str )
  {}

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // consumes x stacks of Thrash on the target
      td( s -> target ) -> dots.thrash_bear -> decrement( data().effectN( 3 ).base_value() );

      // and reduce damage taken by x% for y sec.
      p() -> buff.pulverize -> trigger();
    }
  }

  virtual bool ready() override
  {
    // Call bear_attack_t::ready() first for proper targeting support.
    // Hardcode stack max since it may not be set if this code runs before Thrash is cast.
    return bear_attack_t::ready() &&
      td( target ) -> dots.thrash_bear -> current_stack() >= data().effectN( 3 ).base_value();
  }
};

// Swipe (Bear) =============================================================

struct swipe_bear_t : public bear_attack_t
{
  swipe_bear_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( "swipe_bear", p, p -> find_spell( 213771 ), options_str )
  {
    aoe = -1;
    gore = true;
  }
};

// Thrash (Bear) ============================================================

struct thrash_bear_t : public bear_attack_t
{
  double blood_frenzy_amount;

  thrash_bear_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( "thrash_bear", p, p -> find_spell( 77758 ), options_str ),
    blood_frenzy_amount( 0 )
  {
    aoe = -1;
    spell_power_mod.direct = 0;
    gore = true;
    dot_duration = p -> spec.thrash_bear_dot -> duration();
    base_tick_time = p -> spec.thrash_bear_dot -> effectN( 1 ).period();
    attack_power_mod.tick = p -> spec.thrash_bear_dot -> effectN( 1 ).ap_coeff();
    dot_max_stack = 3; 
    // Apply hidden passive damage multiplier
    base_dd_multiplier *= 1.0 + p -> spec.guardian_overrides -> effectN( 6 ).percent();
    // Bear Form cost modifier
    base_costs[ RESOURCE_RAGE ] *= 1.0 + p -> buff.bear_form -> data().effectN( 7 ).percent();
    
    if ( p -> talent.blood_frenzy -> ok() )
      blood_frenzy_amount = p -> find_spell( 203961 ) -> effectN( 1 ).resource( RESOURCE_RAGE );

    base_multiplier *= 1.0 + p -> artifact.jagged_claws.percent();
  }

  virtual void tick( dot_t* d ) override
  {
    bear_attack_t::tick( d );

    p() -> resource_gain( RESOURCE_RAGE, blood_frenzy_amount, p() -> gain.blood_frenzy );
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
  cenarion_ward_hot_t( druid_t* p ) :
    druid_heal_t( "cenarion_ward_hot", p, p -> find_spell( 102352 ) )
  {
    target = p;
    background = proc = true;
    harmful = hasted_ticks = false;
  }

  virtual void execute() override
  {
    heal_t::execute();

    p() -> buff.cenarion_ward -> expire();
  }
};

struct cenarion_ward_t : public druid_heal_t
{
  cenarion_ward_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "cenarion_ward", p, p -> talent.cenarion_ward,  options_str )
  {}

  virtual void execute() override
  {
    druid_heal_t::execute();

    p() -> buff.cenarion_ward -> trigger();
  }

  virtual bool ready() override
  {
    if ( target != p() )
    {
      assert( "Cenarion Ward will not trigger on other players!" );
      return false;
    }

    return druid_heal_t::ready();
  }
};

// Living Seed ==============================================================

struct living_seed_t : public druid_heal_t
{
  living_seed_t( druid_t* player ) :
    druid_heal_t( "living_seed", player, player -> find_specialization_spell( "Living Seed" ) )
  {
    background = true;
    may_crit   = false;
    proc       = true;
    school     = SCHOOL_NATURE;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  { return data().effectN( 1 ).percent(); }
};

void druid_heal_t::init_living_seed()
{
  if ( p() -> specialization() == DRUID_RESTORATION )
    living_seed = new living_seed_t( p() );
}

// Frenzied Regeneration ====================================================
// TOCHECK: Verify healing calculations match alpha.

struct frenzied_regeneration_t : public druid_heal_t
{
  struct frenzied_regeneration_ignite_t : public residual_action::residual_periodic_action_t<druid_heal_t>
  {
    frenzied_regeneration_ignite_t( druid_t* p, const spell_data_t* s ) :
      residual_action::residual_periodic_action_t<druid_heal_t>( "frenzied_regeneration", p, spell_data_t::nil() )
    {
      background = true;
      target = p;

      dot_duration = s -> duration();
      base_tick_time = s -> effectN( 1 ).period();
      school = s -> get_school_type();
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      timespan_t t = residual_action::residual_periodic_action_t<druid_heal_t>::composite_dot_duration( s );

      t += timespan_t::from_seconds( p() -> buff.guardian_tier19_4pc -> check_stack_value() );

      return t;
    }
  };

  double heal_pct, min_pct;
  timespan_t time_window;
  frenzied_regeneration_ignite_t* ignite;
  heal_t* skysecs_hold;

  frenzied_regeneration_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "frenzied_regeneration_driver", p,
      p -> find_specialization_spell( "Frenzied Regeneration" ), options_str ),
      skysecs_hold( nullptr )
  {
    use_off_gcd = quiet = true;
    may_crit = tick_may_crit = false;
    target = p;
    cooldown -> hasted = true;

    // % of damage taken in the last x seconds to heal.
    heal_pct = data().effectN( 2 ).percent();
    // Length of the incoming damage window.
    time_window = timespan_t::from_seconds( data().effectN( 3 ).base_value() );
    // Minimum base heal amount as a % of max health.
    min_pct = data().effectN( 4 ).percent();
    ignite = new frenzied_regeneration_ignite_t( p, &data() );
    dot_duration = timespan_t::zero();

    base_multiplier *= 1.0 + p -> artifact.wildflesh.percent();
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags = STATE_MUL_TA | STATE_VERSATILITY | STATE_MUL_PERSISTENT | STATE_TGT_MUL_TA;
  }

  double calculate_base_amount()
  {
    double dt = p() -> compute_incoming_damage( time_window ) * heal_pct;
  
    return std::max( dt, p() -> resources.max[ RESOURCE_HEALTH ] * min_pct );
  }

  void execute() override
  {
    base_dd_min = base_dd_max = calculate_base_amount();

    druid_heal_t::execute();

    if ( skysecs_hold )
      skysecs_hold -> schedule_execute();

    if ( p() -> buff.guardian_of_elune -> up() )
      p() -> buff.guardian_of_elune -> expire();

    if ( p() -> buff.guardian_tier19_4pc -> up() )
      p() -> buff.guardian_tier19_4pc -> expire();
  }

  double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    am *= 1.0 + p() -> buff.guardian_of_elune -> check()
      * p() -> buff.guardian_of_elune -> data().effectN( 2 ).percent();

    am *= 1.0 + timespan_t::from_seconds( p() -> buff.guardian_tier19_4pc -> check_stack_value() ) / ignite -> dot_duration;

    return am;
  }

  void impact( action_state_t* s ) override
  { residual_action::trigger( ignite, s -> target, s -> result_amount ); }
};

// Healing Touch ============================================================

struct healing_touch_t : public druid_heal_t
{
  healing_touch_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "healing_touch", p, p -> find_class_spell( "Healing Touch" ), options_str )
  {
    form_mask = NO_FORM | MOONKIN_FORM; // DBC has no mask

    init_living_seed();
    ignore_false_positive = true; // Prevents cat/bear from failing a skill check and going into caster form.
    base_multiplier *= 1.0 + p -> spec.feral -> effectN( 2 ).percent()
      + p -> spec.balance -> effectN( 2 ).percent()
      + p -> spec.guardian -> effectN( 2 ).percent();

    // redirect to self if not specified
    /* if ( target -> is_enemy() || ( target -> type == HEALING_ENEMY && p -> specialization() == DRUID_GUARDIAN ) )
      target = p; */

    target = sim -> target;
    base_multiplier = 0;

    base_multiplier *= 1.0 + p -> artifact.attuned_to_nature.percent();
  }

  virtual double cost() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  virtual void consume_resource() override
  {
    // Prevent from consuming Omen of Clarity unnecessarily
    if ( p() -> buff.predatory_swiftness -> check() )
      return;

    druid_heal_t::consume_resource();
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return timespan_t::zero();

    timespan_t et = druid_heal_t::execute_time();

    et *= 1.0 + p() -> buff.power_of_elune -> current_stack
      * p() -> buff.power_of_elune -> data().effectN( 2 ).percent();

    return et;
  }

  virtual double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    am *= 1.0 + p() -> buff.power_of_elune -> current_stack
      * p() -> buff.power_of_elune -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void impact( action_state_t* state ) override
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_lifebloom_refresh( state );

      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );
    }
  }

  virtual bool check_form_restriction() override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return true;

    return druid_heal_t::check_form_restriction();
  }

  virtual void execute() override
  {
    druid_heal_t::execute();

    if ( p() -> talent.bloodtalons -> ok() )
      p() -> buff.bloodtalons -> trigger( 2 );

    p() -> buff.predatory_swiftness -> expire();

    if ( p() -> buff.power_of_elune -> up() )
      p() -> buff.power_of_elune -> expire();
  }
};

// Lifebloom ================================================================

struct lifebloom_bloom_t : public druid_heal_t
{
  lifebloom_bloom_t( druid_t* p ) :
    druid_heal_t( "lifebloom_bloom", p, p -> find_class_spell( "Lifebloom" ) )
  {
    background       = true;
    dual             = true;
    dot_duration        = timespan_t::zero();
    base_td          = 0;
    attack_power_mod.tick   = 0;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= td( target ) -> buff.lifebloom -> check();

    return ctm;
  }
};

struct lifebloom_t : public druid_heal_t
{
  lifebloom_bloom_t* bloom;

  lifebloom_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "lifebloom", p, p -> find_class_spell( "Lifebloom" ), options_str ),
    bloom( new lifebloom_bloom_t( p ) )
  {
    may_crit   = false;
    ignore_false_positive = true;
  }

  virtual void impact( action_state_t* state ) override
  {
    // Cancel Dot/td-buff on all targets other than the one we impact on
    for ( size_t i = 0; i < sim -> actor_list.size(); ++i )
    {
      player_t* t = sim -> actor_list[ i ];
      if ( state -> target == t )
        continue;
      get_dot( t ) -> cancel();
      td( t ) -> buff.lifebloom -> expire();
    }

    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
      td( state -> target ) -> buff.lifebloom -> trigger();
  }

  virtual void last_tick( dot_t* d ) override
  {
    if ( ! d -> state -> target -> is_sleeping() ) // Prevent crash at end of simulation
      bloom -> execute();
    td( d -> state -> target ) -> buff.lifebloom -> expire();

    druid_heal_t::last_tick( d );
  }

  virtual void tick( dot_t* d ) override
  {
    druid_heal_t::tick( d );

    trigger_clearcasting();
  }
};

// Nature's Guardian ========================================================

struct natures_guardian_t : public druid_heal_t
{
  natures_guardian_t( druid_t* p ) :
    druid_heal_t( "natures_guardian", p, p -> find_spell( 227034 ) )
  {
    background = true;
    may_crit = false;
    target = p;
  }

  void init() override
  {
    druid_heal_t::init();

    // Not affected by multipliers of any sort.
    snapshot_flags &= ~STATE_NO_MULTIPLIER;
  }
};

// Regrowth =================================================================

struct regrowth_t: public druid_heal_t
{
  regrowth_t( druid_t* p, const std::string& options_str ):
    druid_heal_t( "regrowth", p, p -> find_class_spell( "Regrowth" ), options_str )
  {
    ignore_false_positive = true;


    ignore_false_positive = true; // Prevents cat/bear from failing a skill check and going into caster form.
    base_multiplier *= 1.0 + p -> spec.feral -> effectN( 2 ).percent()
      + p -> spec.balance -> effectN( 2 ).percent()
      + p -> spec.guardian -> effectN( 2 ).percent();

    // redirect to self if not specified
    /* if ( target -> is_enemy() || ( target -> type == HEALING_ENEMY && p -> specialization() == DRUID_GUARDIAN ) )
    target = p; */

    target = sim -> target;
    base_multiplier = 0;

    base_multiplier *= 1.0 + p -> artifact.attuned_to_nature.percent();
  }

  double cost() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  void consume_resource() override
  {
    // Prevent from consuming Omen of Clarity unnecessarily
    if ( p() -> buff.predatory_swiftness -> check() )
      return;

    druid_heal_t::consume_resource();
  }

  timespan_t execute_time() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return timespan_t::zero();

    timespan_t et = druid_heal_t::execute_time();

    et *= 1.0 + p() -> buff.power_of_elune -> current_stack
      * p() -> buff.power_of_elune -> data().effectN( 2 ).percent();

    return et;
  }

  double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    am *= 1.0 + p() -> buff.power_of_elune -> current_stack
      * p() -> buff.power_of_elune -> data().effectN( 1 ).percent();

    return am;
  }

  void impact( action_state_t* state ) override
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_lifebloom_refresh( state );

      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );
    }
  }

  bool check_form_restriction() override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return true;

    return druid_heal_t::check_form_restriction();
  }

  void execute() override
  {
    druid_heal_t::execute();

    if ( p() -> talent.bloodtalons -> ok() )
      p() -> buff.bloodtalons -> trigger( 2 );

    p() -> buff.predatory_swiftness -> decrement( 1 );

    if ( p() -> buff.power_of_elune -> up() )
      p() -> buff.power_of_elune -> expire();
  }
};

// Rejuvenation =============================================================

struct rejuvenation_t : public druid_heal_t
{
  rejuvenation_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "rejuvenation", p, p -> find_class_spell( "Rejuvenation" ), options_str )
  {
    tick_zero = true;
  }
};

// Renewal ==================================================================

struct renewal_t : public druid_heal_t
{
  renewal_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "renewal", p, p -> find_talent_spell( "Renewal" ), options_str )
  {
    may_crit = false;
  }

  virtual void execute() override
  {
    base_dd_min = p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    druid_heal_t::execute();
  }
};

// Swiftmend ================================================================

// TODO: in game, you can swiftmend other druids' hots, which is not supported here
struct swiftmend_t : public druid_heal_t
{
  struct swiftmend_aoe_heal_t : public druid_heal_t
  {
    swiftmend_aoe_heal_t( druid_t* p, const spell_data_t* s ) :
      druid_heal_t( "swiftmend_aoe", p, s )
    {
      aoe            = 3;
      background     = true;
      base_tick_time = timespan_t::from_seconds( 1.0 );
      hasted_ticks   = true;
      may_crit       = false;
      proc           = true;
      tick_may_crit  = false;
    }
  };

  swiftmend_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "swiftmend", p, p -> find_class_spell( "Swiftmend" ), options_str )
  {
    init_living_seed();

    impact_action = new swiftmend_aoe_heal_t( p, &data() );
  }

  virtual void impact( action_state_t* state ) override
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );

      if ( p() -> talent.soul_of_the_forest -> ok() )
        p() -> buff.soul_of_the_forest -> trigger();
    }
  }

  virtual bool ready() override
  {
    player_t* t = ( execute_state ) ? execute_state -> target : target;

    if ( ! ( td( t ) -> dots.regrowth -> is_ticking() ||
             td( t ) -> dots.rejuvenation -> is_ticking() ) )
      return false;

    return druid_heal_t::ready();
  }
};

// Tranquility ==============================================================

struct tranquility_t : public druid_heal_t
{
  tranquility_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "tranquility", p, p -> find_specialization_spell( "Tranquility" ), options_str )
  {
    aoe               = data().effectN( 3 ).base_value(); // Heals 5 targets
    base_execute_time = data().duration();
    channeled         = true;

    // Healing is in spell effect 1
    parse_spell_data( ( *player -> dbc.spell( data().effectN( 1 ).trigger_spell_id() ) ) );
  }
};

// Wild Growth ==============================================================

struct wild_growth_t : public druid_heal_t
{
  wild_growth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "wild_growth", p, p -> find_class_spell( "Wild Growth" ), options_str )
  {
    ignore_false_positive = true;
  }

  int n_targets() const override
  {
    int n = druid_heal_t::n_targets();

    if ( p() -> buff.incarnation_tree -> check() )
      n += 2;

    return n;
  }
};

// Ysera's Gift =============================================================

struct yseras_gift_t : public druid_heal_t
{
  yseras_gift_t( druid_t* p ) :
    druid_heal_t( "yseras_gift", p, p -> find_spell( 145110 ) )
  {
    may_crit = false;
    background = dual = true;
    base_pct_heal = p -> spec.yseras_gift -> effectN( 1 ).percent();
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_VERSATILITY; // Is not affected by versatility.
  }

  virtual double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    if ( p() -> buff.bear_form -> check() )
      am *= 1.0 + p() -> buff.bear_form -> data().effectN( 8 ).percent();

    return am;
  }

  virtual void execute() override
  {
    if ( p() -> health_percentage() < 100 )
      target = p();
    else
      target = smart_target();

    druid_heal_t::execute();
  }
};

} // end namespace heals

namespace spells {

// ==========================================================================
// Druid Spells
// ==========================================================================

// Auto Attack ==============================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( druid_t* player, const std::string& options_str ) :
    melee_attack_t( "auto_attack", player, spell_data_t::nil() )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    ignore_false_positive = true;
    use_off_gcd = true;
  }

  virtual void execute() override
  {
    player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
    player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;
    player -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    if ( ! player -> main_hand_attack )
      return false;

    return ( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Astral Communion =========================================================

struct astral_communion_t : public druid_spell_t
{
  astral_communion_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "astral_communion", player, player -> talent.astral_communion, options_str )
  {
    may_miss = may_crit = callbacks = harmful = false;
    energize_type = ENERGIZE_ON_CAST;
  }
};

// Barkskin =================================================================

struct barkskin_t : public druid_spell_t
{
  barkskin_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "barkskin", player, player -> find_specialization_spell( "Barkskin" ), options_str )
  {
    harmful = false;
    use_off_gcd = true;

    cooldown -> duration *= 1.0 + player -> spec.guardian -> effectN( 1 ).percent();
    cooldown -> duration *= 1.0 + player -> talent.survival_of_the_fittest -> effectN( 1 ).percent();
    dot_duration = timespan_t::zero();

    if ( player -> talent.brambles -> ok() )
      add_child( player -> active.brambles_pulse );

    cooldown -> duration *= 1.0 + player -> artifact.perpetual_spring.percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.barkskin -> trigger();
  }
};

// Bear Form Spell ==========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bear_form", player, player -> find_class_spell( "Bear Form" ), options_str )
  {
    form_mask = NO_FORM | CAT_FORM | MOONKIN_FORM;
    may_autounshift = false;

    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );
    ignore_false_positive = true;

    if ( ! player -> bear_melee_attack )
    {
      player -> init_beast_weapon( player -> bear_weapon, 2.5 );
      player -> bear_melee_attack = new bear_attacks::bear_melee_t( player );
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( BEAR_FORM );
  }
};

// Blessing of An'she =======================================================

struct blessing_of_anshe_t : public druid_spell_t
{
  blessing_of_anshe_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "blessing_of_anshe", player, player -> spec.blessing_of_anshe, options_str )
  {
    harmful = may_crit = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.blessing_of_elune -> expire();
    p() -> buff.blessing_of_anshe -> start();
  }

  virtual bool ready() override
  {
    if ( ! p() -> talent.blessing_of_the_ancients -> ok() )
      return false;
    if ( p() -> buff.blessing_of_anshe -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Blessing of Elune ========================================================

struct blessing_of_elune_t : public druid_spell_t
{
  blessing_of_elune_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "blessing_of_elune", player, player -> spec.blessing_of_elune, options_str )
  {
    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.blessing_of_anshe -> expire();
    p() -> buff.blessing_of_elune -> start();
  }

  virtual bool ready() override
  {
    if ( ! p() -> talent.blessing_of_the_ancients -> ok() )
      return false;
    if ( p() -> buff.blessing_of_elune -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Brambles =================================================================

struct brambles_t : public druid_spell_t
{
  brambles_t( druid_t* p ) :
    druid_spell_t( "brambles_reflect", p, p -> find_spell( 203958 ) )
  {
    background = may_crit = proc = may_miss = true;
  }
};

struct brambles_pulse_t : public druid_spell_t
{
  brambles_pulse_t( druid_t* p ) :
    druid_spell_t( "brambles_pulse", p, p -> find_spell( 213709 ) )
  {
    background = dual = true;
    aoe = -1;
  }
};

// Bristling Fur Spell ======================================================

struct bristling_fur_t : public druid_spell_t
{
  bristling_fur_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bristling_fur", player, player -> talent.bristling_fur, options_str  )
  {
    harmful = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.bristling_fur -> trigger();
  }
};

// Cat Form Spell ===========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "cat_form", player, player -> find_class_spell( "Cat Form" ), options_str )
  {
    form_mask = NO_FORM | BEAR_FORM | MOONKIN_FORM;
    may_autounshift = false;

    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );
    ignore_false_positive = true;

    if ( ! player -> cat_melee_attack )
    {
      player -> init_beast_weapon( player -> cat_weapon, 1.0 );
      player -> cat_melee_attack = new cat_attacks::cat_melee_t( player );
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( CAT_FORM );
  }
};


// Celestial Alignment ======================================================

struct celestial_alignment_t : public druid_spell_t
{
  celestial_alignment_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "celestial_alignment", player, player -> spec.celestial_alignment , options_str )
  {
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    druid_spell_t::execute(); // Do not change the order here.

    p() -> buff.celestial_alignment -> trigger();
  }

  virtual bool ready() override
  {
    if ( p() -> talent.incarnation_moonkin -> ok() )
      return false;

    return druid_spell_t::ready();
  }
};

// Fury of Elune =========================================================

struct fury_of_elune_t : public druid_spell_t
{
  struct fury_of_elune_tick_t : public druid_spell_t
  {
    fury_of_elune_tick_t( druid_t* player ) :
      druid_spell_t( "fury_of_elune_tick", player, player -> find_spell( 211545 ) )
    {
      background = dual = ground_aoe = true;
    } 
    
    void snapshot_internal( action_state_t* state, unsigned flags, dmg_e rt ) override
    {
      druid_spell_t::snapshot_internal( state, flags, rt );

      state -> original_x = p() -> fury_of_elune_position.x;
      state -> original_y = p() -> fury_of_elune_position.y;
    }
  };

  fury_of_elune_tick_t* fury_of_elune;

  fury_of_elune_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "fury_of_elune", player, player -> talent.fury_of_elune, options_str ),
    fury_of_elune( new fury_of_elune_tick_t( player ) )
  {
    ground_aoe = true;
    add_child( fury_of_elune );

    dot_duration = sim -> expected_iteration_time > timespan_t::zero() ?
      2 * sim -> expected_iteration_time :
      2 * sim -> max_time * ( 1.0 + sim -> vary_combat_length ); // "infinite" duration

    // Tick cost is proportional to base tick time.
    base_costs_per_tick[ RESOURCE_ASTRAL_POWER ] *= base_tick_time.total_seconds();
  }

  void impact( action_state_t* s ) override
  {
    bool refresh = get_dot( s -> target ) -> is_ticking();

    druid_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && ! refresh )
      p() -> buff.fury_of_elune_up -> trigger();

    p() -> fury_of_elune_position.x = s -> original_x;
    p() -> fury_of_elune_position.y = s -> original_y;
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    fury_of_elune -> execute();
  }

  void cancel() override
  {
    druid_spell_t::cancel();

    if ( dot_t* dot = find_dot( target ) )
      dot -> cancel();

    p() -> buff.fury_of_elune_up -> decrement();
  }
};

struct fury_of_elune_move_t : public druid_spell_t
{
  fury_of_elune_move_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "move_fury_of_elune", player, player -> find_spell( 211547 ), options_str )
  {}

  void execute() override
  {
    druid_spell_t::execute();

    p() -> fury_of_elune_position.x = execute_state -> original_x;
    p() -> fury_of_elune_position.y = execute_state -> original_y;
  }

  bool ready() override
  {
    if ( ! p() -> buff.fury_of_elune_up -> check() )
      return false;

    if ( ! sim -> distance_targeting_enabled )
      return false;

    bool r = druid_spell_t::ready();

    if ( target -> x_position == p() -> fury_of_elune_position.x &&
      target -> y_position == p() -> fury_of_elune_position.y )
    {
      return false;
    }

    return r;
  }
};

// Dash =====================================================================

struct dash_t : public druid_spell_t
{
  dash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "dash", player, player -> find_class_spell( "Dash" ), options_str )
  {
    autoshift = form_mask = CAT_FORM;

    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.dash -> trigger();
  }
};

// Displacer Beast ==========================================================

struct displacer_beast_t : public druid_spell_t
{
  displacer_beast_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "displacer_beast", p, p -> talent.displacer_beast, options_str )
  {
    autoshift = form_mask = CAT_FORM;

    harmful = may_crit = may_miss = false;
    ignore_false_positive = true;
    base_teleport_distance = radius;
    movement_directionality = MOVEMENT_OMNI;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.displacer_beast -> trigger();
  }
};

// Elune's Guidance =========================================================

struct elunes_guidance_t : public druid_spell_t
{
  elunes_guidance_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "elunes_guidance", p, p -> talent.elunes_guidance, options_str )
  {
    harmful = false;
    energize_type = ENERGIZE_ON_CAST;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.elunes_guidance -> trigger();
  }
};

// Full Moon Spell ==========================================================

struct full_moon_t : public druid_spell_t
{
  full_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "full_moon", player, player -> find_spell( 202771 ), options_str )
  {
    aoe = -1;
    split_aoe_damage = true;
    cooldown = player -> cooldown.moon_cd;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    double da = druid_spell_t::calculate_direct_amount( state );
    
    // Sep 3 2016: Main target takes full damage, this reverses the divisor in action_t::calculate_direct_amount (hotfixed).
    if ( state -> chain_target == 0 )
    {
      da *= state -> n_targets;
    }

    return da;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> moon_stage = NEW_MOON; // TOCHECK: Requires hit?
  }

  bool ready() override
  {
    if ( ! p() -> artifact.new_moon.rank() )
      return false;
    if ( p() -> moon_stage != FULL_MOON )
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Half Moon Spell ==========================================================

struct half_moon_t : public druid_spell_t
{
  half_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "half_moon", player, player -> find_spell( 202768 ), options_str )
  {
    cooldown = player -> cooldown.moon_cd;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> moon_stage++;
  }

  bool ready() override
  {
    if ( ! p() -> artifact.new_moon.rank() )
      return false;
    if ( p() -> moon_stage != HALF_MOON )
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Incarnation ==============================================================

struct incarnation_t : public druid_spell_t
{
  buff_t* spec_buff;

  incarnation_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "incarnation", p,
                   p -> specialization() == DRUID_BALANCE     ? p -> talent.incarnation_moonkin :
                   p -> specialization() == DRUID_FERAL       ? p -> talent.incarnation_cat     :
                   p -> specialization() == DRUID_GUARDIAN    ? p -> talent.incarnation_bear    :
                   p -> specialization() == DRUID_RESTORATION ? p -> talent.incarnation_tree    :
                   spell_data_t::nil(), options_str )
  {
    switch ( p -> specialization() )
    {
    case DRUID_BALANCE:
      spec_buff = p -> buff.incarnation_moonkin;
      break;
    case DRUID_FERAL:
      spec_buff = p -> buff.incarnation_cat;
      break;
    case DRUID_GUARDIAN:
      spec_buff = p -> buff.incarnation_bear;
      break;
    case DRUID_RESTORATION:
      spec_buff = p -> buff.incarnation_tree;
      break;
    default:
      assert( "Actor attempted to create incarnation action with no specialization." );
    }

    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    spec_buff -> trigger();
    p() -> buff.feral_instinct -> trigger();

    if ( ! p() -> in_combat )
    {
      timespan_t time = std::max( min_gcd, trigger_gcd * composite_haste() );

      spec_buff -> extend_duration( p(), -time );
      cooldown -> adjust( -time );

      // King of the Jungle raises energy cap, so manually trigger some regen so that the actor starts with the correct amount of energy.
      if ( p() -> buff.incarnation_cat -> check() )
        p() -> regen( time );
    }
    
    // Proxy buff for APL laziness.
    p() -> buff.incarnation_proxy -> trigger( 1, 0, -1.0, spec_buff -> remains() );

    if ( p() -> buff.incarnation_bear -> check() )
    {
      p() -> cooldown.mangle -> reset( false );
      p() -> cooldown.growl  -> reset( false );
      p() -> cooldown.maul   -> reset( false );
    }
  }
};

// Innervate ================================================================

struct innervate_t : public druid_spell_t
{
  innervate_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "innervate", p, p -> find_specialization_spell( "Innervate" ), options_str )
  {
    harmful = false;
  }
};

// Ironfur ==================================================================

struct ironfur_t : public druid_spell_t
{
  ironfur_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "ironfur", p, p -> spec.ironfur, options_str )
  {
    use_off_gcd = true;
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
  }

  double cost() const override
  {
    double c = druid_spell_t::cost();

    c *= 1.0 + p() -> buff.gory_fur -> check_value();

    return c;
  }

  timespan_t composite_buff_duration()
  {
    timespan_t bd = p() -> buff.ironfur -> buff_duration;

    bd += timespan_t::from_seconds( p() -> buff.guardian_of_elune -> value() );
    bd += timespan_t::from_seconds( p() -> buff.guardian_tier19_4pc -> check_stack_value() );

    return bd;
  }

  virtual void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.ironfur -> trigger( 1, buff_t::DEFAULT_VALUE(), -1, composite_buff_duration() );

    if ( p() -> buff.guardian_of_elune -> up() )
      p() -> buff.guardian_of_elune -> expire();

    if ( p() -> buff.guardian_tier19_4pc -> up() )
      p() -> buff.guardian_tier19_4pc -> expire();

    if ( p() -> buff.gory_fur -> up() )
      p() -> buff.gory_fur -> expire();
  }
};

// Lunar Beam ===============================================================

struct lunar_beam_t : public druid_spell_t
{
  struct lunar_beam_heal_t : public heals::druid_heal_t
  {
    lunar_beam_heal_t( druid_t* p ) :
      heals::druid_heal_t( "lunar_beam_heal", p, p -> find_spell( 204069 ) )
    {
      target = p;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
      background = true;
    }
  };

  struct lunar_beam_damage_t : public druid_spell_t
  {
    action_t* heal;

    lunar_beam_damage_t( druid_t* p ) :
      druid_spell_t( "lunar_beam_damage", p, p -> find_spell( 204069 ) )
    {
      aoe = -1;
      dual = background = ground_aoe = true;
      attack_power_mod.direct = data().effectN( 2 ).ap_coeff();

      heal = new lunar_beam_heal_t( p );
    }

    void execute() override
    {
      druid_spell_t::execute();

      heal -> schedule_execute();
    }
  };

  action_t* damage;

  lunar_beam_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "lunar_beam", p, p -> talent.lunar_beam, options_str )
  {
    may_crit = tick_may_crit = may_miss = hasted_ticks = false;
    dot_duration = timespan_t::zero();
    base_tick_time = timespan_t::from_seconds( 1.0 ); // TODO: Find in spell data... somewhere!

    damage = p -> find_action( "lunar_beam_damage" );

    if ( ! damage )
    {
      damage = new lunar_beam_damage_t( p );
      damage -> stats = stats;
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        .x( execute_state -> target -> x_position )
        .y( execute_state -> target -> y_position )
        .pulse_time( base_tick_time )
        .duration( data().duration() )
        .start_time( sim -> current_time() )
        .action( damage ), false );
  }
};

// Lunar Strike =============================================================

struct lunar_strike_t : public druid_spell_t
{
  timespan_t natures_balance;
  lunar_strike_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "lunar_strike", player,
      player -> find_affinity_spell( "Lunar Strike" ), options_str )
  {
    aoe = -1;
    base_aoe_multiplier = data().effectN( 3 ).percent();

    natures_balance    = timespan_t::from_seconds( player -> talent.natures_balance -> effectN( 1 ).base_value() );
    
    base_execute_time *= 1.0 + player -> sets.set( DRUID_BALANCE, T17, B2 ) -> effectN( 1 ).percent();
    base_crit         += player -> artifact.dark_side_of_the_moon.percent();
    base_multiplier   *= 1.0 + player -> artifact.skywrath.percent();
  }

  double composite_crit_chance() const override
  {
    double cc = druid_spell_t::composite_crit_chance();

    if ( p() -> buff.lunar_empowerment -> check() )
      cc += p() -> spec.balance_tier19_2pc -> effectN( 1 ).percent();

    return cc;
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.lunar_empowerment -> check() )
      am *= 1.0 + composite_lunar_empowerment();

    return am;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p() -> talent.starlord -> ok() && p() -> buff.lunar_empowerment -> check() )
      g *= 1 - p() -> talent.starlord -> effectN( 1 ).percent();

    return g;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = druid_spell_t::execute_time();

    if ( p() -> talent.starlord -> ok() && p() -> buff.lunar_empowerment -> check() )
      et *= 1 - p() -> talent.starlord -> effectN( 1 ).percent();

    if ( p() -> buff.warrior_of_elune -> check() )
      et *= 1 + p() -> talent.warrior_of_elune -> effectN( 1 ).percent();

    return et;
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );
    // Nature's Balance only extends Moonfire on the primary target. FIXME: Actually extends the duration of ALL sunfires.
    if ( natures_balance > timespan_t::zero() && hit_any_target )
    {
      td( s -> target ) -> dots.moonfire -> extend_duration( natures_balance, timespan_t::from_seconds( 20.0 ) );
    }
  }

  void execute() override
  {
    p() -> buff.lunar_empowerment -> up();
    p() -> buff.warrior_of_elune -> up();

    druid_spell_t::execute();

    p() -> buff.lunar_empowerment -> decrement();
    p() -> buff.warrior_of_elune -> decrement();

    p() -> buff.power_of_elune -> trigger();

    if ( rng().roll( p() -> sets.set( DRUID_BALANCE, T19, B4 ) -> effectN( 1 ).percent() ) )
      p() -> buff.solar_empowerment -> trigger();
  }
};

// Mark of Ursol ============================================================

struct mark_of_ursol_t : public druid_spell_t
{
  mark_of_ursol_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "mark_of_ursol", player,
      player -> find_specialization_spell( "Mark of Ursol" ), options_str )
  {
    harmful = false;
    may_crit = may_miss = false;
    dot_duration = timespan_t::zero();
  }

  double cost() const override
  {
    double c = druid_spell_t::cost();

    c *= 1.0 + p() -> buff.gory_fur -> check_value();

    return c;
  }

  timespan_t composite_buff_duration()
  {
    return p() -> buff.mark_of_ursol -> buff_duration +
      timespan_t::from_seconds( p() -> buff.guardian_of_elune -> value() );
  }

  virtual void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.mark_of_ursol -> trigger( 1, buff_t::DEFAULT_VALUE(),
      -1, composite_buff_duration() );

    p() -> buff.guardian_of_elune -> expire();

    if ( p() -> buff.gory_fur -> up() )
      p() -> buff.gory_fur -> expire();
  }
};

// New Moon Spell ===========================================================

struct new_moon_t : public druid_spell_t
{
  new_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "new_moon", player, player -> artifact.new_moon, options_str )
  {
    cooldown = player -> cooldown.moon_cd;
    cooldown -> duration = data().charge_cooldown();
    cooldown -> charges  = data().charges();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> moon_stage++;
  }

  bool ready() override
  {
    if ( ! p() -> artifact.new_moon.rank() )
      return false;
    if ( p() -> moon_stage != NEW_MOON )
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Rage of the Sleeper ======================================================

struct rage_of_the_sleeper_t : public druid_spell_t
{
  struct rage_of_the_sleeper_reflect_t : public druid_spell_t
  {
    rage_of_the_sleeper_reflect_t( druid_t* p ) :
      druid_spell_t( "rage_of_the_sleeper_reflect", p, p -> find_spell( 219432 ) )
    {
      background = proc = true;
      may_miss = may_crit = false;
    }
  };

  rage_of_the_sleeper_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "rage_of_the_sleeper", p, &p -> artifact.rage_of_the_sleeper.data(), options_str )
  {
    use_off_gcd = true;
    harmful = may_miss = may_crit = false;
    base_dd_min = base_dd_max = 0;

    if ( ! p -> active.rage_of_the_sleeper )
      p -> active.rage_of_the_sleeper = new rage_of_the_sleeper_reflect_t( p );
  }

  virtual void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.rage_of_the_sleeper -> trigger();
  }
};

// Sunfire Spell ============================================================

struct sunfire_t : public druid_spell_t
{
  struct sunfire_damage_t : public druid_spell_t
  {
    sunfire_damage_t( druid_t* p ) : 
      druid_spell_t( "sunfire_dmg", p, p -> find_spell( 164815 ) )
    {
      if ( p -> talent.shooting_stars -> ok() && ! p -> active.shooting_stars )
      {
        p -> active.shooting_stars = new shooting_stars_t( p );
      }
      
      dual = background = true;
      aoe = -1;
      dot_duration += p -> spec.balance_overrides -> effectN( 4 ).time_value();

      base_multiplier *= 1.0 + p -> artifact.sunfire_burns.percent();
      radius += p -> artifact.sunblind.value();
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( ! t ) t = target;
      if ( ! t ) return nullptr;

      return td( t ) -> dots.sunfire;
    }

    void tick( dot_t* d ) override
    {
      druid_spell_t::tick( d );

      trigger_shooting_stars( d -> state );

      trigger_balance_tier18_2pc();
    }
  };

  sunfire_damage_t* damage;

  sunfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "sunfire", player, player -> find_affinity_spell( "Sunfire" ), options_str )
  {
    may_miss = false;
    damage = new sunfire_damage_t( player );
    damage -> stats = stats;

    if ( player -> spec.astral_power -> ok() )
    {
      energize_resource = RESOURCE_ASTRAL_POWER;
      energize_amount = player -> spec.astral_power -> effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
    }
    else
    {
      energize_type = ENERGIZE_NONE;
    }

    // Add damage modifiers in sunfire_damage_t, not here.
  }

  void execute() override
  {
    druid_spell_t::execute();

    damage -> target = execute_state -> target;
    damage -> schedule_execute();
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonkin_form", player, player -> spec.moonkin_form, options_str )
  {
    form_mask = NO_FORM | CAT_FORM | BEAR_FORM;
    may_autounshift = false;

    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( MOONKIN_FORM );
  }
};

// Prowl ====================================================================

struct prowl_t : public druid_spell_t
{
  prowl_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "prowl", player, player -> find_class_spell( "Prowl" ), options_str )
  {
    autoshift = form_mask = CAT_FORM;

    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    p() -> buff.prowl -> trigger();
  }

  bool ready() override
  {
    if ( p() -> buff.prowl -> check() )
      return false;

    if ( p() -> in_combat && ! ( p() -> specialization() == DRUID_FERAL &&
      p() -> buff.incarnation_cat -> check() ) )
    {
      return false;
    }

    return druid_spell_t::ready();
  }
};

// Skull Bash ===============================================================

struct skull_bash_t : public druid_spell_t
{
  skull_bash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "skull_bash", player, player -> find_specialization_spell( "Skull Bash" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, PVP, B2 ) )
      p() -> cooldown.tigers_fury -> reset( false );
  }

  bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Solar Wrath ==============================================================

struct solar_wrath_t : public druid_spell_t
{
  timespan_t natures_balance;

  solar_wrath_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "solar_wrath", player, player -> find_affinity_spell( "Solar Wrath" ), options_str )
  {
    natures_balance    = timespan_t::from_seconds( player -> talent.natures_balance -> effectN( 2 ).base_value() );

    base_execute_time *= 1.0 + player -> sets.set( DRUID_BALANCE, T17, B2 ) -> effectN( 1 ).percent();
    base_multiplier   *= 1.0 + player -> artifact.skywrath.percent();
    base_multiplier   *= 1.0 + player -> artifact.solar_stabbing.percent();
    energize_amount = player -> spec.astral_power -> effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
  }

  double composite_crit_chance() const override
  {
    double cc = druid_spell_t::composite_crit_chance();

    if ( p() -> buff.solar_empowerment -> check() )
      cc += p() -> spec.balance_tier19_2pc -> effectN( 1 ).percent();

    return cc;
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.solar_empowerment -> check() )
      am *= 1.0 + composite_solar_empowerment();

    return am;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p() -> talent.starlord -> ok() && p() -> buff.solar_empowerment -> check() )
      g *= 1 - p() -> talent.starlord -> effectN( 1 ).percent();

    return g;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = druid_spell_t::execute_time();

    if ( p() -> talent.starlord -> ok() && p() -> buff.solar_empowerment -> check() )
      et *= 1 - p() -> talent.starlord -> effectN( 1 ).percent();

    return et;
  }

  void execute() override
  {
    p() -> buff.solar_empowerment -> up();

    druid_spell_t::execute();
    
    if ( natures_balance > timespan_t::zero() && hit_any_target )
    {
      td( execute_state -> target ) -> dots.sunfire ->
        extend_duration( natures_balance, timespan_t::from_seconds( 20.0 ) );
    }

    if ( p() -> sets.has_set_bonus( DRUID_BALANCE, T17, B4 ) )
    {
      p() -> cooldown.celestial_alignment ->
        adjust( -p() -> sets.set( DRUID_BALANCE, T17, B4 ) -> effectN( 1 ).time_value() );
    }

    p() -> buff.solar_empowerment -> decrement();

    p() -> buff.power_of_elune -> trigger();

    if ( rng().roll( p() -> sets.set( DRUID_BALANCE, T19, B4 ) -> effectN( 2 ).percent() ) )
      p() -> buff.lunar_empowerment -> trigger();
  }
};

// Stampeding Roar ==========================================================

struct stampeding_roar_t : public druid_spell_t
{
  stampeding_roar_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "stampeding_roar", p, p -> find_class_spell( "Stampeding Roar" ), options_str )
  {
    form_mask = BEAR_FORM | CAT_FORM;
    autoshift = BEAR_FORM;

    harmful = false;
    radius *= 1.0 + p -> talent.gutteral_roars -> effectN( 1 ).percent();
    cooldown -> duration *= 1.0 + p -> talent.gutteral_roars -> effectN( 2 ).percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> type == PLAYER_GUARDIAN )
        continue;

      p -> buffs.stampeding_roar -> trigger();
    }
  }
};

// Starfall Spell ===========================================================

struct starfall_t : public druid_spell_t
{
  struct starfall_tick_t : public druid_spell_t
  {
    bool echoing_stars;

    starfall_tick_t( const std::string& n, druid_t* p, const spell_data_t* s ) :
      druid_spell_t( n, p, s ),
      echoing_stars( false )
    {
      aoe = -1;
      background = dual = direct_tick = true; // Legion TOCHECK
      callbacks = false;
      radius  = p -> find_spell( 191034 ) -> effectN( 1 ).radius();
      radius *= 1.0 + p -> talent.stellar_drift -> effectN( 1 ).percent();

      base_multiplier *= 1.0 + p -> talent.stellar_drift -> effectN( 2 ).percent();
    }
    
    timespan_t travel_time() const override
    {
      // If this is an echo and distance targeting is on, no travel is needed.
      if ( echoing_stars && sim -> distance_targeting_enabled )
        return timespan_t::zero();

      /* Generate a travel time, 800 - 1800ms. FIXME: Travel time is a bit messed up with regards
      to echoes. "Chains" should impact as the same time as their source. */
      return rng().range( timespan_t::from_millis( 800 ), timespan_t::from_millis( 1800 ) );
    }

    double action_multiplier() const override
    {
      double am = druid_spell_t::action_multiplier();

      if ( p() -> mastery.starlight -> ok() )
        am *= 1.0 + p() -> cache.mastery_value();

      return am;
    }

    void impact( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      // Distance targeting: Execute a proper "chain" off of each target impacted.
      if ( p() -> artifact.echoing_stars.rank() && ! echoing_stars &&
        sim -> distance_targeting_enabled )
      {
        p() -> active.echoing_stars -> target = select_chain_target( s );

        if ( p() -> active.echoing_stars -> target )
          p() -> active.echoing_stars -> schedule_execute();
      }
    }

    void execute() override
    {
      druid_spell_t::execute();

      // Non-distance targeting: If we hit more than 1 target, simply trigger the echo as an AoE.
      if ( p() -> artifact.echoing_stars.rank() && ! echoing_stars &&
        ! sim -> distance_targeting_enabled && execute_state -> n_targets > 1 )
      {
        assert( ! p() -> active.echoing_stars -> pre_execute_state );
        p() -> active.echoing_stars -> schedule_execute();
      }
    }

    player_t* select_chain_target( action_state_t* s )
    {
      // Create target list
      std::vector<player_t*> targets;
      for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* t = sim -> target_non_sleeping_list[ i ];

        if ( t -> is_enemy() && t != s -> target &&
          t -> get_ground_aoe_distance( *s ) <= p() -> active.echoing_stars -> radius + t -> combat_reach )
          targets.push_back( t );
      }
        
      if ( targets.size() > 0 )
      {
        // Select a random target
        return targets[ p() -> rng().range( 0, targets.size() ) ];
      }

      return nullptr;
    }
  };

  starfall_t( druid_t* p, const std::string& options_str ):
    druid_spell_t( "starfall", p, p -> find_spell( 191034 ), options_str )
  {
    may_miss = may_crit = false;
    base_tick_time = data().duration() / 8.0; // ticks 9 times (missing from spell data)
    // TOCHECK: tick zero?

    if ( ! p -> active.starfall )
    {
      p -> active.starfall = new starfall_tick_t( "starfall_tick", p, p -> find_spell( 191037 ) );
      p -> active.starfall -> stats = stats;
    }

    if ( p -> artifact.echoing_stars.rank() && ! p -> active.echoing_stars )
    {
      /* Create echoing stars action. If distance targeting is off, we'll just cheat a bit and
      trigger a repeat AoE that hits for less damage. If it's on, then we'll do real chaining
      from each target impacted. */
      starfall_tick_t* echo = new starfall_tick_t( "echoing_stars", p, p -> find_spell( 226104 ) );
      // set bool so this action knows its the secondary and to not trigger itself
      echo -> echoing_stars = true;
        
      if ( sim -> distance_targeting_enabled )
      {
        echo -> aoe = 0;
        echo -> radius = p -> active.starfall -> data().effectN( 2 ).radius();
      }

      add_child( echo );
      p -> active.echoing_stars = echo;
    }

    base_costs[ RESOURCE_ASTRAL_POWER ] +=
      p -> talent.soul_of_the_forest -> effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
  }

  double cost() const override
  {
    if ( p() -> buff.oneths_overconfidence -> check() )
      return 0;

    return druid_spell_t::cost();
  }

  virtual void execute() override
  {
    druid_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        .x( execute_state -> target -> x_position )
        .y( execute_state -> target -> y_position )
        .pulse_time( base_tick_time )
        .duration( data().duration() )
        .start_time( sim -> current_time() )
        .action( p() -> active.starfall )
        .hasted( ground_aoe_params_t::SPELL_HASTE ), true );

    // Trigger starfall debuffs
    for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
    {
      player_t* t = sim -> target_non_sleeping_list[ i ];

      if ( t -> is_enemy() && ( ! sim -> distance_targeting_enabled ||
        t -> get_player_distance( *execute_state -> target ) <= p() -> active.starfall -> radius + t -> combat_reach ) )
      {
        td( t ) -> debuff.stellar_empowerment -> trigger();
      }
    }
    
    if ( p() -> buff.oneths_overconfidence -> up() ) // benefit tracking
      p() -> buff.oneths_overconfidence -> decrement();

    p() -> buff.oneths_intuition -> trigger();
  }
};

struct starshards_t : public starfall_t
{
  starshards_t( druid_t* player ) :
    starfall_t( player, "" )
  {
    background = true;
    target = sim -> target;
    radius = 40;
    // Does not cost astral power.
    base_costs.fill( 0 );
  }

  // Allow the action to execute even when Starfall is already active.
  bool ready() override
  { return druid_spell_t::ready(); }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  struct goldrinns_fang_t : public druid_spell_t
  {
    goldrinns_fang_t( druid_t* player ) :
      druid_spell_t( "goldrinns_fang", player, player -> artifact.power_of_goldrinn.data().effectN( 1 ).trigger() )
    {
      background = true;
      may_miss = false;
    }
  };

  starsurge_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "starsurge", p, p -> find_affinity_spell( "Starsurge" ), options_str )
  {
    base_crit += p -> artifact.scythe_of_the_stars.percent();

    if ( p -> artifact.power_of_goldrinn.rank() && ! p -> active.goldrinns_fang )
    {
      p -> active.goldrinns_fang = new goldrinns_fang_t( p );
      add_child( p -> active.goldrinns_fang );
    }
  }

  double cost() const override
  {
    if ( p() -> buff.oneths_intuition -> check() )
      return 0;

    double c = druid_spell_t::cost();

    c += p() -> buff.the_emerald_dreamcatcher -> check_stack_value();

    return c;
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> mastery.starlight -> ok() )
      am *= 1.0 + p() -> cache.mastery_value();

    return am;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( hit_any_target )
    {
      // Dec 3 2015: Starsurge must hit to grant empowerments, but grants them on cast not impact.
      p() -> buff.solar_empowerment -> trigger();
      p() -> buff.lunar_empowerment -> trigger();
    }

    if ( rng().roll( p() -> starshards ) )
    {
      p() -> proc.starshards -> occur();
      p() -> active.starshards -> execute();
    }

    // TOCHECK: Needs hit?
    if ( rng().roll( p() -> artifact.power_of_goldrinn.data().proc_chance() ) )
    {
      p() -> active.goldrinns_fang -> target = target;
      p() -> active.goldrinns_fang -> schedule_execute();
    }

    p() -> buff.the_emerald_dreamcatcher -> up(); // benefit tracking
    p() -> buff.the_emerald_dreamcatcher -> trigger();
    
    if ( p() -> buff.oneths_intuition -> up() ) // benefit tracking
      p() -> buff.oneths_intuition -> decrement();

    p() -> buff.oneths_overconfidence -> trigger();
  }
};

// Stellar Flare ============================================================

struct stellar_flare_t : public druid_spell_t
{
  stellar_flare_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "stellar_flare", player, player -> talent.stellar_flare, options_str )
  {}
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "survival_instincts", player, player -> find_specialization_spell( "Survival Instincts" ),
      options_str )
  {
    harmful = false;
    use_off_gcd = true;

    // Spec-based cooldown modifiers
    cooldown -> duration += player -> spec.feral_overrides2 -> effectN( 6 ).time_value();
    cooldown -> duration *= 1.0 + player -> spec.guardian -> effectN( 3 ).percent();

    cooldown -> duration *= 1.0 + player -> talent.survival_of_the_fittest -> effectN( 1 ).percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.survival_instincts -> trigger();
  }
};

// Typhoon ==================================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "typhoon", player, player -> talent.typhoon, options_str )
  {
    ignore_false_positive = true;
  }
};

// Warrior of Elune =========================================================

struct warrior_of_elune_t : public druid_spell_t
{
  warrior_of_elune_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "warrior_of_elune", player, player -> talent.warrior_of_elune, options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.warrior_of_elune -> trigger( 2 );
  }

  virtual bool ready() override
  {
    if ( p() -> buff.warrior_of_elune -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Wild Charge ==============================================================

struct wild_charge_t : public druid_spell_t
{
  double movement_speed_increase;

  wild_charge_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "wild_charge", p, p -> talent.wild_charge, options_str ),
    movement_speed_increase( 5.0 )
  {
    harmful = may_crit = may_miss = false;
    ignore_false_positive = true;
    range = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
    trigger_gcd = timespan_t::zero();
  }

  void schedule_execute( action_state_t* execute_state ) override
  {
    druid_spell_t::schedule_execute( execute_state );

    /* Since Cat/Bear charge is limited to moving towards a target,
       cancel form if the druid wants to move away.
       Other forms can already move in any direction they want so they're fine. */
    if ( p() -> current.movement_direction == MOVEMENT_AWAY )
      p() -> shapeshift( NO_FORM );
  }

  void execute() override
  {
    p() -> buff.wild_charge_movement -> trigger( 1, movement_speed_increase, 1,
        timespan_t::from_seconds( p() -> current.distance_to_move / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() +
                                  movement_speed_increase ) ) ) );

    druid_spell_t::execute();
  }

  bool ready() override
  {
    if ( p() -> current.distance_to_move < data().min_range() ) // Cannot charge if the target is too close.
      return false;

    return druid_spell_t::ready();
  }
};

} // end namespace spells

// ==========================================================================
// Druid Helper Functions & Structures
// ==========================================================================

// Brambles Absorb/Reflect Handler ==========================================

double brambles_handler( const action_state_t* s )
{
  druid_t* p = static_cast<druid_t*>( s -> target );
  assert( p -> talent.brambles -> ok() );
  assert( s );

  /* Calculate the maximum amount absorbed. This is not affected by
     versatility (and likely other player modifiers). */
  double absorb_cap = 0.24 * p -> cache.attack_power() *
    p -> composite_attack_power_multiplier();

  // Calculate actual amount absorbed.
  double amount_absorbed = std::min( s -> result_mitigated, absorb_cap );

  // Schedule reflected damage.
  p -> active.brambles -> base_dd_min = p -> active.brambles -> base_dd_max = 
    amount_absorbed;
  action_state_t* ref_s = p -> active.brambles -> get_state();
  ref_s -> target = s -> action -> player;
  p -> active.brambles -> snapshot_state( ref_s, DMG_DIRECT );
  p -> active.brambles -> schedule_execute( ref_s );

  return amount_absorbed;
}

// Earthwarden Absorb Handler ===============================================

double earthwarden_handler( const action_state_t* s )
{
  if ( s -> action -> special )
    return 0;

  druid_t* p = static_cast<druid_t*>( s -> target );
  assert( p -> talent.earthwarden -> ok() );

  if ( ! p -> buff.earthwarden -> up() )
    return 0;

  double absorb = s -> result_amount * p -> buff.earthwarden -> check_value();
  p -> buff.earthwarden -> decrement();

  return absorb;
}

// Persistent Buff Delay Event ==============================================

struct persistent_buff_delay_event_t : public event_t
{
  buff_t* buff;

  persistent_buff_delay_event_t( druid_t* p, buff_t* b ) :
    event_t( *p ), buff( b )
  {
    /* Delay triggering the buff a random amount between 0 and buff_period.
       This prevents fixed-period driver buffs from ticking at the exact same
       times on every iteration.

       Buffs that use the event to activate should implement tick_zero-like
       behavior. */
    schedule( rng().real() * b -> buff_period );
  }

  const char* name() const override
  { return "persistent_buff_delay"; }

  void execute() override
  { buff -> trigger(); }
};

// Ailuro Pouncers Event ====================================================

struct ailuro_pouncers_event_t : public event_t
{
  druid_t* druid;

  ailuro_pouncers_event_t( druid_t* p,
                           timespan_t set_delay = timespan_t::zero() )
    : event_t( *p, set_delay > timespan_t::zero()
                       ? set_delay
                       : p->legendary.ailuro_pouncers ),
      druid( p )
  {
  }

  const char* name() const override
  { return "ailuro_pouncers"; }

  void execute() override
  {
    druid -> buff.predatory_swiftness -> trigger();

    make_event<ailuro_pouncers_event_t>( sim(), druid );
  }
};

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  if ( name == "ashamanes_frenzy" ||
       name == "frenzy"                 ) return new ashamanes_frenzy_driver_t( this, options_str );
  if ( name == "astral_communion" ||
       name == "ac" )                     return new       astral_communion_t( this, options_str );
  if ( name == "auto_attack"            ) return new            auto_attack_t( this, options_str );
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk"                ) return new                berserk_t( this, options_str );
  if ( name == "bear_form"              ) return new      spells::bear_form_t( this, options_str );
  if ( name == "blessing_of_anshe"      ) return new      blessing_of_anshe_t( this, options_str );
  if ( name == "blessing_of_elune"      ) return new      blessing_of_elune_t( this, options_str );
  if ( name == "brutal_slash"           ) return new           brutal_slash_t( this, options_str );
  if ( name == "bristling_fur"          ) return new          bristling_fur_t( this, options_str );
  if ( name == "cat_form"               ) return new       spells::cat_form_t( this, options_str );
  if ( name == "celestial_alignment" ||
       name == "ca"                     ) return new    celestial_alignment_t( this, options_str );
  if ( name == "cenarion_ward"          ) return new          cenarion_ward_t( this, options_str );
  if ( name == "dash"                   ) return new                   dash_t( this, options_str );
  if ( name == "displacer_beast"        ) return new        displacer_beast_t( this, options_str );
  if ( name == "elunes_guidance"        ) return new        elunes_guidance_t( this, options_str );
  if ( name == "ferocious_bite"         ) return new         ferocious_bite_t( this, options_str );
  if ( name == "frenzied_regeneration"  ) return new  frenzied_regeneration_t( this, options_str );
  if ( name == "full_moon"              ) return new              full_moon_t( this, options_str );
  if ( name == "fury_of_elune"          ) return new          fury_of_elune_t( this, options_str );
  if ( name == "growl"                  ) return new                  growl_t( this, options_str );
  if ( name == "half_moon"              ) return new              half_moon_t( this, options_str );
  if ( name == "healing_touch"          ) return new          healing_touch_t( this, options_str );
  if ( name == "innervate"              ) return new              innervate_t( this, options_str );
  if ( name == "ironfur"                ) return new                ironfur_t( this, options_str );
  if ( name == "lifebloom"              ) return new              lifebloom_t( this, options_str );
  if ( name == "lunar_beam"             ) return new             lunar_beam_t( this, options_str );
  if ( name == "lunar_strike"           ) return new           lunar_strike_t( this, options_str );
  if ( name == "maim"                   ) return new                   maim_t( this, options_str );
  if ( name == "mangle"                 ) return new                 mangle_t( this, options_str );
  if ( name == "mark_of_ursol"          ) return new          mark_of_ursol_t( this, options_str );
  if ( name == "maul"                   ) return new                   maul_t( this, options_str );
  if ( name == "moonfire"               ) return new               moonfire_t( this, options_str );
  if ( name == "moonfire_cat" ||
       name == "lunar_inspiration" )      return new      lunar_inspiration_t( this, options_str );
  if ( name == "move_fury_of_elune"     ) return new     fury_of_elune_move_t( this, options_str );
  if ( name == "new_moon"               ) return new               new_moon_t( this, options_str );
  if ( name == "sunfire"                ) return new                sunfire_t( this, options_str );
  if ( name == "moonkin_form"           ) return new   spells::moonkin_form_t( this, options_str );
  if ( name == "pulverize"              ) return new              pulverize_t( this, options_str );
  if ( name == "rage_of_the_sleeper"    ) return new    rage_of_the_sleeper_t( this, options_str );
  if ( name == "rake"                   ) return new                   rake_t( this, options_str );
  if ( name == "renewal"                ) return new                renewal_t( this, options_str );
  if ( name == "regrowth"               ) return new               regrowth_t( this, options_str );
  if ( name == "rejuvenation"           ) return new           rejuvenation_t( this, options_str );
  if ( name == "rip"                    ) return new                    rip_t( this, options_str );
  if ( name == "savage_roar"            ) return new            savage_roar_t( this, options_str );
  if ( name == "shred"                  ) return new                  shred_t( this, options_str );
  if ( name == "skull_bash"             ) return new             skull_bash_t( this, options_str );
  if ( name == "solar_wrath"            ) return new            solar_wrath_t( this, options_str );
  if ( name == "stampeding_roar"        ) return new        stampeding_roar_t( this, options_str );
  if ( name == "starfall"               ) return new               starfall_t( this, options_str );
  if ( name == "starsurge"              ) return new              starsurge_t( this, options_str );
  if ( name == "stellar_flare"          ) return new          stellar_flare_t( this, options_str );
  if ( name == "prowl"                  ) return new                  prowl_t( this, options_str );
  if ( name == "survival_instincts"     ) return new     survival_instincts_t( this, options_str );
  if ( name == "swipe_cat"              ) return new              swipe_cat_t( this, options_str );
  if ( name == "swipe_bear"             ) return new             swipe_bear_t( this, options_str );
  if ( name == "swiftmend"              ) return new              swiftmend_t( this, options_str );
  if ( name == "tigers_fury"            ) return new            tigers_fury_t( this, options_str );
  if ( name == "thrash_bear"            ) return new            thrash_bear_t( this, options_str );
  if ( name == "thrash_cat"             ) return new             thrash_cat_t( this, options_str );
  if ( name == "tranquility"            ) return new            tranquility_t( this, options_str );
  if ( name == "typhoon"                ) return new                typhoon_t( this, options_str );
  if ( name == "warrior_of_elune"       ) return new       warrior_of_elune_t( this, options_str );
  if ( name == "wild_charge"            ) return new            wild_charge_t( this, options_str );
  if ( name == "wild_growth"            ) return new            wild_growth_t( this, options_str );
  if ( name == "incarnation"            ) return new            incarnation_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name,
                            const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  return nullptr;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  player_t::create_pets();

  if ( sets.has_set_bonus( DRUID_BALANCE, T18, B2 ) )
  {
    for ( pet_t*& pet : pet_fey_moonwing )
      pet = new pets::fey_moonwing_t( sim, this );
  }
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  player_t::init_spells();

  // Specializations ========================================================

  // Generic / Multiple specs
  spec.critical_strikes           = find_specialization_spell( "Critical Strikes" );
  spec.druid                      = find_spell( 137009 );
  spec.killer_instinct            = find_specialization_spell( "Killer Instinct" );
  spec.leather_specialization     = find_specialization_spell( "Leather Specialization" );
  spec.nurturing_instinct         = find_specialization_spell( "Nurturing Instinct" );
  spec.omen_of_clarity            = find_specialization_spell( "Omen of Clarity" );

  // Balance
  spec.astral_power               = find_specialization_spell( "Astral Power" );
  spec.balance                    = find_specialization_spell( "Balance Druid" );
  spec.balance_overrides          = find_specialization_spell( "Balance Overrides Passive" );
  spec.blessing_of_anshe          = find_talent_spell( "Blessing of the Ancients" ) -> ok() ? find_spell( 202739 ) : spell_data_t::not_found();
  spec.blessing_of_elune          = find_talent_spell( "Blessing of the Ancients" ) -> ok() ? find_spell( 202737 ) : spell_data_t::not_found();
  spec.celestial_alignment        = find_specialization_spell( "Celestial Alignment" );
  spec.moonkin_form               = find_specialization_spell( "Moonkin Form" );
  spec.starfall                   = find_specialization_spell( "Starfall" );
  spec.stellar_empowerment        = spec.starfall -> ok() ? find_spell( 197637 ) : spell_data_t::not_found();
  spec.balance_tier19_2pc         = sets.has_set_bonus( DRUID_BALANCE, T19, B2 ) ? find_spell( 211089 ) : spell_data_t::not_found();
  spec.starsurge_2 = find_specialization_spell( 231021 );
  spec.moonkin_2 = find_specialization_spell( 231042 );
  spec.starfall_2 = find_specialization_spell( 231049 );
  spec.sunfire_2 = find_specialization_spell( 231050) ;

  // Feral
  spec.cat_form                   = find_class_spell( "Cat Form" ) -> ok() ? find_spell( 3025   ) : spell_data_t::not_found();
  spec.cat_form_speed             = find_class_spell( "Cat Form" ) -> ok() ? find_spell( 113636 ) : spell_data_t::not_found();
  spec.feral                      = find_specialization_spell( "Feral Druid" );
  spec.feral_overrides            = specialization() == DRUID_FERAL ? find_spell( 197692 ) : spell_data_t::not_found();
  spec.feral_overrides2           = specialization() == DRUID_FERAL ? find_spell( 106733 ) : spell_data_t::not_found();
  spec.gushing_wound              = sets.has_set_bonus( DRUID_FERAL, T17, B4 ) ? find_spell( 165432 ) : spell_data_t::not_found();
  spec.nurturing_instinct         = find_specialization_spell( "Nurturing Instinct" );
  spec.predatory_swiftness        = find_specialization_spell( "Predatory Swiftness" );
  spec.primal_fury                = find_spell( 16953 );
  spec.rip                        = find_specialization_spell( "Rip" );
  spec.sharpened_claws            = find_specialization_spell( "Sharpened Claws" );
  spec.swipe_cat                  = find_spell( 106785 );
  spec.rake_2 = find_specialization_spell( 231052 );
  spec.tigers_fury_2 = find_specialization_spell( 231055 );
  spec.ferocious_bite_2 = find_specialization_spell( 231056 );
  spec.shred = find_specialization_spell( "Shred" );
  spec.shred_2 = find_specialization_spell( 231063 );
  spec.shred_3 = find_specialization_spell( 231057 );
  spec.swipe_2 = find_specialization_spell( 231283 );


  // Guardian
  spec.bear_form                  = find_class_spell( "Bear Form" ) -> ok() ? find_spell( 1178 ) : spell_data_t::not_found();
  spec.gore                       = find_specialization_spell( "Gore" );
  spec.guardian                   = find_specialization_spell( "Guardian Druid" );
  spec.guardian_overrides         = find_specialization_spell( "Guardian Overrides Passive" );
  spec.ironfur                    = find_specialization_spell( "Ironfur" );
  spec.thrash_bear_dot            = find_specialization_spell( "Thrash" ) -> ok() ? find_spell( 192090 ) : spell_data_t::not_found();
  spec.mangle_2 = find_specialization_spell( 231064 );
  spec.lightning_reflexes = find_specialization_spell( 231064 );
  spec.ironfur_2 = find_specialization_spell( 231070 );
  
  // Talents ================================================================

  // Multiple Specs
  talent.renewal                        = find_talent_spell( "Renewal" );
  talent.displacer_beast                = find_talent_spell( "Displacer Beast" );
  talent.wild_charge                    = find_talent_spell( "Wild Charge" );

  talent.balance_affinity               = find_talent_spell( "Balance Affinity" );
  talent.feral_affinity                 = find_talent_spell( "Feral Affinity" );
  talent.guardian_affinity              = find_talent_spell( "Guardian Affinity" );
  talent.restoration_affinity           = find_talent_spell( "Restoration Affinity" );

  talent.mighty_bash                    = find_talent_spell( "Mighty Bash" );
  talent.mass_entanglement              = find_talent_spell( "Mass Entanglement" );
  talent.typhoon                        = find_talent_spell( "Typhoon" );

  talent.soul_of_the_forest             = find_talent_spell( "Soul of the Forest" );
  talent.moment_of_clarity              = find_talent_spell( "Moment of Clarity" );

  // Feral
  talent.predator                       = find_talent_spell( "Predator" );
  talent.blood_scent                    = find_talent_spell( "Blood Scent" );
  talent.lunar_inspiration              = find_talent_spell( "Lunar Inspiration" );

  talent.incarnation_cat                = find_talent_spell( "Incarnation: King of the Jungle" );
  talent.brutal_slash                   = find_talent_spell( "Brutal Slash" );

  talent.sabertooth                     = find_talent_spell( "Sabertooth" );
  talent.jagged_wounds                  = find_talent_spell( "Jagged Wounds" );
  talent.elunes_guidance                = find_talent_spell( "Elune's Guidance" );

  talent.savage_roar                    = find_talent_spell( "Savage Roar" );
  talent.bloodtalons                    = find_talent_spell( "Bloodtalons" );

  // Balance
  talent.shooting_stars                 = find_talent_spell( "Shooting Stars" );
  talent.warrior_of_elune               = find_talent_spell( "Warrior of Elune" );
  talent.starlord                       = find_talent_spell( "Starlord" );

  talent.incarnation_moonkin            = find_talent_spell( "Incarnation: Chosen of Elune" );
  talent.stellar_flare                  = find_talent_spell( "Stellar Flare" );

  talent.stellar_drift                  = find_talent_spell( "Stellar Drift" );
  talent.full_moon                      = find_talent_spell( "Full Moon" );
  talent.natures_balance                = find_talent_spell( "Nature's Balance" );

  talent.fury_of_elune                  = find_talent_spell( "Fury of Elune" );
  talent.astral_communion               = find_talent_spell( "Astral Communion" );
  talent.blessing_of_the_ancients       = find_talent_spell( "Blessing of the Ancients" );

  // Guardian
  talent.brambles                       = find_talent_spell( "Brambles" );
  talent.pulverize                      = find_talent_spell( "Pulverize" );
  talent.blood_frenzy                   = find_talent_spell( "Blood Frenzy" );

  talent.gutteral_roars                 = find_talent_spell( "Gutteral Roars" );

  talent.incarnation_bear               = find_talent_spell( "Incarnation: Guardian of Ursoc" );
  talent.galactic_guardian              = find_talent_spell( "Galactic Guardian" );

  talent.earthwarden                    = find_talent_spell( "Earthwarden" );
  talent.guardian_of_elune              = find_talent_spell( "Guardian of Elune" );
  talent.survival_of_the_fittest        = find_talent_spell( "Survival of the Fittest" );

  talent.rend_and_tear                  = find_talent_spell( "Rend and Tear" );
  talent.lunar_beam                     = find_talent_spell( "Lunar Beam" );
  talent.bristling_fur                  = find_talent_spell( "Bristling Fur" );

  // Restoration
  talent.verdant_growth                 = find_talent_spell( "Verdant Growth" );
  talent.cenarion_ward                  = find_talent_spell( "Cenarion Ward" );
  talent.germination                    = find_talent_spell( "Germination" );

  talent.incarnation_tree               = find_talent_spell( "Incarnation: Tree of Life" );
  talent.cultivation                    = find_talent_spell( "Cultivation" );

  talent.prosperity                     = find_talent_spell( "Prosperity" );
  talent.inner_peace                    = find_talent_spell( "Inner Peace" );
  talent.profusion                      = find_talent_spell( "Profusion" );

  talent.stonebark                      = find_talent_spell( "Stonebark" );
  talent.flourish                       = find_talent_spell( "Flourish" );

  if ( talent.earthwarden -> ok() )
    instant_absorb_list[ talent.earthwarden -> id() ] =
      new instant_absorb_t( this, find_spell( 203975 ), "earthwarden", &earthwarden_handler );

  // Affinities =============================================================

  spec.feline_swiftness = specialization() == DRUID_FERAL || talent.feral_affinity -> ok() ?
    find_spell( 131768 ) : spell_data_t::not_found();

  spec.astral_influence = specialization() == DRUID_BALANCE || talent.balance_affinity -> ok() ?
    find_spell( 197524 ) : spell_data_t::not_found();
  
  spec.thick_hide = specialization() == DRUID_GUARDIAN || talent.guardian_affinity -> ok() ?
    find_spell( 16931 ) : spell_data_t::not_found();
  
  spec.yseras_gift = specialization() == DRUID_RESTORATION || talent.restoration_affinity -> ok() ?
    find_spell( 145108 ) : spell_data_t::not_found();

  // Masteries ==============================================================

  mastery.razor_claws         = find_mastery_spell( DRUID_FERAL );
  mastery.harmony             = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian    = find_mastery_spell( DRUID_GUARDIAN );
  mastery.natures_guardian_AP = mastery.natures_guardian -> ok() ?
    find_spell( 159195 ) : spell_data_t::not_found();
  mastery.starlight           = find_mastery_spell( DRUID_BALANCE );

  // Artifact ===============================================================

  // Balance -- Scythe of Elune
  artifact.new_moon                     = find_artifact_spell( "New Moon" );
  artifact.moon_and_stars               = find_artifact_spell( "Moon and Stars" );
  artifact.power_of_goldrinn            = find_artifact_spell( "Power of Goldrinn" );
  artifact.echoing_stars                = find_artifact_spell( "Echoing Stars" );
  artifact.falling_star                 = find_artifact_spell( "Falling Star" );
  artifact.touch_of_the_moon            = find_artifact_spell( "Touch of the Moon" );
  artifact.bladed_feathers              = find_artifact_spell( "Bladed Feathers" );
  artifact.sunfire_burns                = find_artifact_spell( "Sunfire Burns" );
  artifact.solar_stabbing               = find_artifact_spell( "Solar Stabbing" );
  artifact.dark_side_of_the_moon        = find_artifact_spell( "Dark Side of the Moon" );
  artifact.rejuvenating_innervation     = find_artifact_spell( "Rejuvenating Innervation" );
  artifact.twilight_glow                = find_artifact_spell( "Twilight Glow" );
  artifact.scythe_of_the_stars          = find_artifact_spell( "Scythe of the Stars" );
  artifact.skywrath                     = find_artifact_spell( "Skywrath" );
  artifact.sunblind                     = find_artifact_spell( "Sunblind" );
  artifact.light_of_the_sun             = find_artifact_spell( "Light of the Sun" );
  artifact.empowerment                  = find_artifact_spell( "Empowerment" );

  // Feral -- Fangs of Ashamane
  artifact.ashamanes_frenzy             = find_artifact_spell( "Ashamane's Frenzy" );
  artifact.open_wounds                  = find_artifact_spell( "Open Wounds" );
  artifact.ashamanes_bite               = find_artifact_spell( "Ashamane's Bite" );
  artifact.shadow_thrash                = find_artifact_spell( "Shadow Thrash" );
  artifact.ashamanes_energy             = find_artifact_spell( "Ashamane's Energy" );
  artifact.razor_fangs                  = find_artifact_spell( "Razor Fangs" );
  artifact.attuned_to_nature            = find_artifact_spell( "Attuned to Nature" );
  artifact.powerful_bite                = find_artifact_spell( "Powerful Bite" );
  artifact.feral_power                  = find_artifact_spell( "Feral Power" );
  artifact.sharpened_claws              = find_artifact_spell( "Sharpened Claws" );
  artifact.shredder_fangs               = find_artifact_spell( "Shredder Fangs" );
  artifact.tear_the_flesh               = find_artifact_spell( "Tear the Flesh" );
  artifact.honed_instincts              = find_artifact_spell( "Honed Instincts" );
  artifact.protection_of_ashamane       = find_artifact_spell( "Protection of Ashamane" );
  artifact.scent_of_blood               = find_artifact_spell( "Scent of Blood" );
  artifact.feral_instinct               = find_artifact_spell( "Feral Instinct" );
  artifact.hardened_roots               = find_artifact_spell( "Hardened Roots" );

  // Guardian -- Claws of Ursoc
  artifact.rage_of_the_sleeper          = find_artifact_spell( "Rage of the Sleeper" );
  artifact.adaptive_fur                 = find_artifact_spell( "Adaptive Fur" );
  artifact.embrace_of_the_nightmare     = find_artifact_spell( "Embrace of the Nightmare" );
  artifact.reinforced_fur               = find_artifact_spell( "Reinforced Fur" );
  artifact.mauler                       = find_artifact_spell( "Mauler" );
  artifact.jagged_claws                 = find_artifact_spell( "Jagged Claws" );
  artifact.bestial_fortitude            = find_artifact_spell( "Bestial Fortitude" );
  artifact.perpetual_spring             = find_artifact_spell( "Perpetual Spring" );
  artifact.bear_hug                     = find_artifact_spell( "Bear Hug" );
  artifact.ursocs_endurance             = find_artifact_spell( "Ursoc's Endurance" );
  artifact.sharpened_instincts          = find_artifact_spell( "Sharpened Instincts" );
  artifact.vicious_bites                = find_artifact_spell( "Vicious Bites" );
  artifact.wildflesh                    = find_artifact_spell( "Wildflesh" );
  artifact.gory_fur                     = find_artifact_spell( "Gory Fur" );
  artifact.iron_claws                   = find_artifact_spell( "Iron Claws" );
  artifact.roar_of_the_crowd            = find_artifact_spell( "Roar of the Crowd" );

  // Active Actions =========================================================

  caster_melee_attack = new caster_attacks::druid_melee_t( this );

  if ( talent.cenarion_ward -> ok() )
    active.cenarion_ward_hot  = new heals::cenarion_ward_hot_t( this );
  if ( spec.yseras_gift )
    active.yseras_gift        = new heals::yseras_gift_t( this );
  if ( sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
    active.gushing_wound      = new cat_attacks::gushing_wound_t( this );
  if ( talent.brambles -> ok() )
  {
    active.brambles           = new spells::brambles_t( this );
    active.brambles_pulse     = new spells::brambles_pulse_t( this );

    instant_absorb_list[ talent.brambles -> id() ] =
      new instant_absorb_t( this, talent.brambles, "brambles", &brambles_handler );
  }
  if ( talent.galactic_guardian -> ok() )
  {
    active.galactic_guardian  = new spells::moonfire_t::galactic_guardian_damage_t( this );
    active.galactic_guardian -> stats = get_stats( "moonfire" );
  }
  if ( artifact.ashamanes_bite.rank() )
    active.ashamanes_rip = new cat_attacks::ashamanes_rip_t( this );
  if ( mastery.natures_guardian -> ok() )
    active.natures_guardian = new heals::natures_guardian_t( this );
}

// druid_t::init_base =======================================================

void druid_t::init_base_stats()
{
  player_t::init_base_stats();

  // Set base distance based on spec
  base.distance = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? 3 : 30;

  // All specs get benefit from both agi and intellect.
  // Nurturing Instinct overrides this behavior in composite_spell_power.
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Resources
  resources.base[ RESOURCE_RAGE         ] = 100;
  resources.base[ RESOURCE_COMBO_POINT  ] = 5;
  resources.base[ RESOURCE_ASTRAL_POWER ] = 100;
  resources.base[ RESOURCE_ENERGY       ] = 100
      + sets.set( DRUID_FERAL, T18, B2 ) -> effectN( 2 ).resource( RESOURCE_ENERGY )
      + talent.moment_of_clarity -> effectN( 3 ).percent();

  resources.active_resource[ RESOURCE_ASTRAL_POWER ] = specialization() == DRUID_BALANCE;
  resources.active_resource[ RESOURCE_HEALTH       ] = primary_role() == ROLE_TANK || talent.guardian_affinity -> ok();
  resources.active_resource[ RESOURCE_MANA         ] = primary_role() == ROLE_HEAL || talent.restoration_affinity -> ok()
      || talent.balance_affinity -> ok() || specialization() == DRUID_GUARDIAN;
  resources.active_resource[ RESOURCE_ENERGY       ] = primary_role() == ROLE_ATTACK || talent.feral_affinity -> ok();
  resources.active_resource[ RESOURCE_COMBO_POINT  ] = primary_role() == ROLE_ATTACK || talent.feral_affinity -> ok();
  resources.active_resource[ RESOURCE_RAGE         ] = primary_role() == ROLE_TANK || talent.guardian_affinity -> ok();

  base_energy_regen_per_second = 10;
  base_energy_regen_per_second *= 1.0 + talent.feral_affinity -> effectN( 2 ).percent();

  // Max Mana & Mana Regen modifiers
  resources.base_multiplier[ RESOURCE_MANA ] *= 1.0 + spec.druid -> effectN( 5 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.druid -> effectN( 5 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// druid_t::init_buffs ======================================================

void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Generic / Multi-spec druid buffs

  buff.bear_form             = new buffs::bear_form_t( *this );

  buff.cat_form              = new buffs::cat_form_t( *this );

  buff.clearcasting          = buff_creator_t( this, "clearcasting", spec.omen_of_clarity -> effectN( 1 ).trigger() )
                               .chance( specialization() == DRUID_RESTORATION ? find_spell( 113043 ) -> proc_chance()
                                        : find_spell( 16864 ) -> proc_chance() )
                               .cd( timespan_t::zero() )
                               .max_stack( 1 + talent.moment_of_clarity -> effectN( 1 ).base_value() );

  buff.dash                  = buff_creator_t( this, "dash", find_class_spell( "Dash" ) )
                               .cd( timespan_t::zero() )
                               .default_value( find_class_spell( "Dash" ) -> effectN( 1 ).percent() );

  buff.prowl                 = buff_creator_t( this, "prowl", find_class_spell( "Prowl" ) );

  // Talent buffs

  buff.displacer_beast       = buff_creator_t( this, "displacer_beast", talent.displacer_beast -> effectN( 2 ).trigger() )
                               .default_value( talent.displacer_beast -> effectN( 2 ).trigger() -> effectN( 1 ).percent() );

  buff.wild_charge_movement  = buff_creator_t( this, "wild_charge_movement" );

  buff.cenarion_ward         = buff_creator_t( this, "cenarion_ward", find_talent_spell( "Cenarion Ward" ) );

  buff.incarnation_proxy     = buff_creator_t( this, "incarnation", spell_data_t::nil() )
                               .quiet( true )
                               .chance( 1 );

  buff.incarnation_moonkin   = new incarnation_moonkin_buff_t( *this );

  buff.incarnation_cat       = new incarnation_cat_buff_t( *this );

  buff.incarnation_bear      = buff_creator_t( this, "incarnation_guardian_of_ursoc", talent.incarnation_bear )
                               .cd( timespan_t::zero() );

  buff.incarnation_tree      = buff_creator_t( this, "incarnation_tree_of_life", talent.incarnation_tree )
                               .duration( timespan_t::from_seconds( 30 ) )
                               .default_value( find_spell( 5420 ) -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
                               .cd( timespan_t::zero() );

  buff.bloodtalons           = buff_creator_t( this, "bloodtalons", talent.bloodtalons -> ok() ? find_spell( 145152 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 145152 ) -> effectN( 1 ).percent() )
                               .max_stack( 2 );

  buff.elunes_guidance       = buff_creator_t( this, "elunes_guidance", talent.elunes_guidance )
                               .tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
                                 resource_gain( RESOURCE_COMBO_POINT,
                                   talent.elunes_guidance -> effectN( 2 ).trigger() -> effectN( 1 ).resource( RESOURCE_COMBO_POINT ),
                                   gain.elunes_guidance ); } )
                               .cd( timespan_t::zero() )
                               .period( talent.elunes_guidance -> effectN( 2 ).period() );
 
  buff.feral_instinct        = buff_creator_t( this, "feral_instinct", find_spell( 210649 ) )
                               .chance( artifact.feral_instinct.rank() > 0 )
                               .default_value( artifact.feral_instinct.percent() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.galactic_guardian     = buff_creator_t( this, "galactic_guardian", find_spell( 213708 ) )
                               .chance( talent.galactic_guardian -> ok() )
                               .default_value( find_spell( 213708 ) -> effectN( 1 ).resource( RESOURCE_RAGE ) );

  buff.gore                  = buff_creator_t( this, "mangle", find_spell( 93622 ) )
                               .default_value( find_spell( 93622 ) -> effectN( 1 ).resource( RESOURCE_RAGE ) );

  buff.gory_fur              = buff_creator_t( this, "gory_fur", artifact.gory_fur.data().effectN( 1 ).trigger() )
                               .trigger_spell( artifact.gory_fur )
                               .chance( artifact.gory_fur.data().proc_chance() )
                               .default_value( -artifact.gory_fur.data().effectN( 1 ).percent() );

  buff.guardian_of_elune     = buff_creator_t( this, "guardian_of_elune", talent.guardian_of_elune -> effectN( 1 ).trigger() )
                               .chance( talent.guardian_of_elune -> ok() )
                               .default_value( talent.guardian_of_elune -> effectN( 1 ).trigger() -> effectN( 1 ).time_value().total_seconds() );

  // Balance

  buff.blessing_of_anshe     = buff_creator_t( this, "blessing_of_anshe", spec.blessing_of_anshe )
                               .tick_time_behavior( BUFF_TICK_TIME_HASTED )
                               .tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
                                 resource_gain( RESOURCE_ASTRAL_POWER, spec.blessing_of_anshe -> effectN( 1 ).resource( RESOURCE_ASTRAL_POWER ),
                                 gain.blessing_of_anshe ); } );

  buff.blessing_of_elune     = buff_creator_t( this, "blessing_of_elune", spec.blessing_of_elune );

  buff.celestial_alignment   = new celestial_alignment_buff_t( *this );

  buff.fury_of_elune_up      = buff_creator_t( this, "fury_of_elune_up", spell_data_t::nil() )
                               .max_stack( 10 ); // Tracking buff for APL use

  buff.lunar_empowerment     = buff_creator_t( this, "lunar_empowerment", find_spell( 164547 ) )
                               .default_value( find_spell( 164547 ) -> effectN( 1 ).percent()
                                 + talent.soul_of_the_forest -> effectN( 1 ).percent()
                                 + artifact.empowerment.percent() )
                               .max_stack( find_spell( 164547 ) -> max_stacks() + spec.starsurge_2 -> effectN( 1 ).base_value() );

  buff.moonkin_form          = new buffs::moonkin_form_t( *this );

  buff.solar_empowerment     = buff_creator_t( this, "solar_empowerment", find_spell( 164545 ) )
                               .default_value( find_spell( 164545 ) -> effectN( 1 ).percent()
                                 + talent.soul_of_the_forest -> effectN( 1 ).percent()
                                 + artifact.empowerment.percent() )
                               .max_stack( find_spell( 164545 ) -> max_stacks() + spec.starsurge_2 -> effectN( 1 ).base_value() );

  buff.star_power            = buff_creator_t( this, "star_power", find_spell( 202942 ) )
                               .default_value( find_spell( 202942 ) -> effectN( 1 ).percent() / ( talent.incarnation_moonkin -> ok() ? 3 : 1 ) )
                               .add_invalidate( CACHE_SPELL_HASTE );

  buff.warrior_of_elune      = new warrior_of_elune_buff_t( *this );

  // Feral

  buff.ashamanes_energy      = buff_creator_t( this, "ashamanes_energy", find_spell( 210583 ) )
                               .chance( artifact.ashamanes_energy.rank() > 0 )
                               .default_value( artifact.ashamanes_energy.value() )
                               .tick_callback( [ this ]( buff_t* b , int, const timespan_t& ) {
                                  resource_gain( RESOURCE_ENERGY, b -> check_value(), gain.ashamanes_energy ); } );

  buff.berserk               = new berserk_buff_t( *this );

  buff.predatory_swiftness   = buff_creator_t( this, "predatory_swiftness", find_spell( 69369 ) )
                               .chance( spec.predatory_swiftness -> ok() );

  buff.protection_of_ashamane = buff_creator_t( this, "protection_of_ashamane", find_spell( 210655 ) )
                               .chance( artifact.protection_of_ashamane.rank() > 0 )
                               .cd( find_spell( 213557 ) -> duration() )
                               .default_value( find_spell( 210655 ) -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_DODGE )
                               .add_invalidate( CACHE_ARMOR );

  buff.savage_roar           = buff_creator_t( this, "savage_roar", talent.savage_roar )
                               .default_value( talent.savage_roar -> effectN( 2 ).percent() )
                               .refresh_behavior( BUFF_REFRESH_DURATION ) // Pandemic refresh is done by the action
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                               .tick_behavior( BUFF_TICK_NONE );

  buff.scent_of_blood        = buff_creator_t( this, "scent_of_blood", find_spell( 210664 ) )
                               .chance( artifact.scent_of_blood.rank() > 0 )
                               .default_value( -artifact.scent_of_blood.data().effectN( 1 ).resource( RESOURCE_ENERGY ) );

  buff.tigers_fury           = buff_creator_t( this, "tigers_fury", find_specialization_spell( "Tiger's Fury" ) )
                               .default_value( find_specialization_spell( "Tiger's Fury" ) -> effectN( 1 ).percent() )
                               .cd( timespan_t::zero() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ); 

  // Guardian
  buff.adaptive_fur          = new adaptive_fur_t( *this );

  buff.barkskin              = buff_creator_t( this, "barkskin", find_specialization_spell( "Barkskin" ) )
                               .cd( timespan_t::zero() )
                               .default_value( find_specialization_spell( "Barkskin" ) -> effectN( 2 ).percent() )
                               .duration( find_specialization_spell( "Barkskin" ) -> duration() + artifact.ursocs_endurance.time_value() )
                               .tick_behavior( talent.brambles -> ok() ? BUFF_TICK_REFRESH : BUFF_TICK_NONE )
                               .tick_callback( [ this ] ( buff_t*, int, const timespan_t& ) {
                                 if ( talent.brambles -> ok() )
                                   active.brambles_pulse -> execute();
                               } );

  buff.bristling_fur         = buff_creator_t( this, "bristling_fur", talent.bristling_fur )
                               .cd( timespan_t::zero() );

  buff.earthwarden           = buff_creator_t( this, "earthwarden", find_spell( 203975 ) )
                               .default_value( talent.earthwarden -> effectN( 1 ).percent() );

  buff.earthwarden_driver    = buff_creator_t( this, "earthwarden_driver", talent.earthwarden )
                               .quiet( true )
                               .tick_callback( [ this ] ( buff_t*, int, const timespan_t& ) { buff.earthwarden -> trigger(); } )
                               .tick_zero( true );

  buff.ironfur               = buff_creator_t( this, "ironfur", spec.ironfur )
                               .duration( spec.ironfur -> duration() + artifact.ursocs_endurance.time_value() )
                               .default_value( spec.ironfur -> effectN( 1 ).percent() + artifact.reinforced_fur.percent() )
                               .add_invalidate( CACHE_ARMOR )
                               .max_stack( 20 ) // many stacks, handle it
                               .stack_behavior( BUFF_STACK_ASYNCHRONOUS )
                               .cd( timespan_t::zero() );

  buff.mark_of_ursol         = buff_creator_t( this, "mark_of_ursol", find_specialization_spell( "Mark of Ursol" ) )
                               .default_value( find_specialization_spell( "Mark of Ursol" ) -> effectN( 1 ).percent() )
                               .cd( timespan_t::zero() ) // cooldown handled by spell
                               .duration( find_specialization_spell( "Mark of Ursol" ) -> duration() + artifact.ursocs_endurance.time_value() );

  buff.pulverize             = buff_creator_t( this, "pulverize", find_spell( 158792 ) )
                               .default_value( find_spell( 158792 ) -> effectN( 1 ).percent() )
                               .refresh_behavior( BUFF_REFRESH_PANDEMIC );

  buff.rage_of_the_sleeper   = buff_creator_t( this, "rage_of_the_sleeper", &artifact.rage_of_the_sleeper.data() )
                               .cd( timespan_t::zero() )
                               .default_value( -artifact.rage_of_the_sleeper.data().effectN( 1 ).percent() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                               .add_invalidate( CACHE_LEECH );

  buff.survival_instincts    = buff_creator_t( this, "survival_instincts", find_specialization_spell( "Survival Instincts" ) )
                               .cd( timespan_t::zero() )
                               .default_value( -find_specialization_spell( "Survival Instincts" ) -> effectN( 1 ).percent() + artifact.sharpened_instincts.percent() )
                               .duration( find_specialization_spell( "Survival Instincts" ) -> duration() + artifact.honed_instincts.time_value() );

  // Restoration
  buff.harmony               = buff_creator_t( this, "harmony", mastery.harmony -> ok() ? find_spell( 100977 ) : spell_data_t::not_found() );

  buff.soul_of_the_forest    = buff_creator_t( this, "soul_of_the_forest",
                               talent.soul_of_the_forest -> ok() ? find_spell( 114108 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 114108 ) -> effectN( 1 ).percent() );

  if ( specialization() == DRUID_RESTORATION || talent.restoration_affinity -> ok() )
  {
    buff.yseras_gift         = buff_creator_t( this, "yseras_gift_driver", spec.yseras_gift )
                               .quiet( true )
                               .tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
                                 active.yseras_gift -> schedule_execute(); } )
                               .tick_zero( true );
  }

  // Set Bonuses

  buff.balance_tier18_4pc    = buff_creator_t( this, "faerie_blessing", find_spell( 188086 ) )
                               .default_value( find_spell( 188086 ) -> effectN( 1 ).percent() )
                               .chance( sets.has_set_bonus( DRUID_BALANCE, T18, B4 ) )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.feral_tier17_4pc      = buff_creator_t( this, "feral_tier17_4pc", find_spell( 166639 ) )
                               .quiet( true );

  buff.guardian_tier17_4pc   = buff_creator_t( this, "guardian_tier17_4pc", find_spell( 177969 ) )
                               .default_value( find_spell( 177969 ) -> effectN( 1 ).percent() );

  buff.guardian_tier19_4pc   = buff_creator_t( this, "natural_defenses",
                                 sets.set( DRUID_GUARDIAN, T19, B4 ) -> effectN( 1 ).trigger() )
                               .trigger_spell( sets.set( DRUID_GUARDIAN, T19, B4 ) )
                               .default_value( sets.set( DRUID_GUARDIAN, T19, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() / 1000.0 );
}

// ALL Spec Pre-Combat Action Priority List =================================

void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask or Elixir
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask, elixir1, elixir2;

    if ( primary_role() == ROLE_TANK ) // Guardian
    {
      if ( true_level > 100 )
        flask = "flask_of_the_seventh_demon";
      else if ( true_level > 90 )
        flask = "greater_draenic_agility_flask";
      else if ( true_level > 85 )
        flask = "winds";
      else
        flask = "steelskin";
    }
    else if ( primary_role() == ROLE_ATTACK ) // Feral
    {
      if ( true_level > 100 )
        flask = "flask_of_the_seventh_demon";
      else if ( true_level > 90 )
        flask = "greater_draenic_agility_flask";
      else
        flask = "winds";
    }
    else // Balance & Restoration
    {
      if ( true_level > 100 )
        flask = "flask_of_the_whispered_pact";
      else if ( true_level > 90 )
        flask = "greater_draenic_intellect_flask";
      else if ( true_level > 85 )
        flask = "warm_sun";
      else
        flask = "draconic_mind";
    }

    if ( ! flask.empty() )
      precombat -> add_action( "flask,type=" + flask );
    else
    {
      if ( ! elixir1.empty() )
        precombat -> add_action( "elixir,type=" + elixir1 );
      if ( ! elixir2.empty() )
        precombat -> add_action( "elixir,type=" + elixir2 );
    }
  }

  // Food
  if ( sim -> allow_food && true_level > 80 )
  {
    std::string food;

    if ( true_level > 100 )
    {
      switch ( specialization() )
      {
        case DRUID_FERAL:
          food = "nightborne_delicacy_platter";
          break;
        default: // placeholder
          food = "azshari_salad";
          break;
      }
    }
    else if ( true_level > 90 )
    {
      switch ( specialization() )
      {
        case DRUID_FERAL:
          food = "pickled_eel";
          break;
        case DRUID_BALANCE:
        case DRUID_GUARDIAN:
          food = "sleeper_sushi";
          break;
        default:
          food = "buttered_sturgeon";
          break;
      }
    }
    else
      food = "seafood_magnifique_feast";

    if ( ! food.empty() )
      precombat -> add_action( "food,type=" + food );
  }

  // Augmentation Rune
  if ( true_level > 100 )
    precombat -> add_action( "augmentation,type=defiled" );

  // Mark of the Wild
  precombat -> add_action( this, "Mark of the Wild", "if=!aura.str_agi_int.up" );

  // Feral: Bloodtalons
  if ( specialization() == DRUID_FERAL && true_level >= 100 )
    precombat -> add_action( this, "Regrowth", "if=talent.bloodtalons.enabled" );

  // Forms
  if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) || primary_role() == ROLE_ATTACK )
  {
    precombat -> add_action( this, "Cat Form" );
    precombat -> add_action( this, "Prowl" );
  }
  else if ( primary_role() == ROLE_TANK )
  {
    precombat -> add_action( this, "Bear Form" );
  }
  else if ( specialization() == DRUID_BALANCE && ( primary_role() == ROLE_DPS || primary_role() == ROLE_SPELL ) )
  {
    precombat -> add_action( this, "Moonkin Form" );
    precombat -> add_action( "blessing_of_elune" );
  }

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-Potion
  if ( sim -> allow_potions && true_level >= 80 )
  {
    std::string potion_action = "potion,name=";
    if ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK )
    {
      if ( true_level > 100 )
        potion_action += "old_war";
      else if ( true_level > 90 )
        potion_action += "draenic_agility";
      else if ( true_level > 85 )
        potion_action += "tolvir";
      else
        potion_action += "tolvir";
      precombat -> add_action( potion_action );
    }
    else if ( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION ) && ( primary_role() == ROLE_SPELL
              || primary_role() == ROLE_HEAL ) )
    {
      if ( true_level > 100 )
        potion_action += "deadly_grace";
      else if ( true_level > 90 )
        potion_action += "draenic_intellect";
      else if ( true_level > 85 )
        potion_action += "jade_serpent";
      else
        potion_action += "volcanic";
      precombat -> add_action( potion_action );
    }
  }

  // Spec Specific Optimizations
  if ( specialization() == DRUID_BALANCE )
  {
    precombat -> add_action( this, "New Moon" );
  }
  else if ( specialization() == DRUID_GUARDIAN )
    precombat -> add_talent( this, "Cenarion Ward" );
  else if ( specialization() == DRUID_FERAL && ( find_item( "soul_capacitor" ) || find_item( "maalus_the_blood_drinker" ) ) )
    precombat -> add_action( "incarnation" );
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

  if ( primary_role() == ROLE_ATTACK )
  {
    def -> add_action( this, "Faerie Fire", "if=debuff.weakened_armor.stack<3" );
    def -> add_action( extra_actions );
    def -> add_action( this, "Rake", "if=remains<=duration*0.3" );
    def -> add_action( this, "Shred" );
    def -> add_action( this, "Ferocious Bite", "if=combo_points>=5" );
  }
  // Specless (or speced non-main role) druid who has a primary role of a healer
  else if ( primary_role() == ROLE_HEAL )
  {
    def -> add_action( extra_actions );
    def -> add_action( this, "Rejuvenation", "if=remains<=duration*0.3" );
    def -> add_action( this, "Healing Touch", "if=mana.pct>=30" );
  }
}

// Feral Combat Action Priority List ========================================

void druid_t::apl_feral()
{
  action_priority_list_t* def      = get_action_priority_list( "default"    );
  action_priority_list_t* finish   = get_action_priority_list( "finisher"   );
  action_priority_list_t* generate = get_action_priority_list( "generator"  );
  action_priority_list_t* sbt      = get_action_priority_list( "sbt_opener" );

  std::string              potion_action  = "potion,name=";
  if ( true_level > 100 )
    potion_action += "old_war";
  else if ( true_level > 90 )
    potion_action += "draenic_agility";
  else
    potion_action += "tolvir";

  // Main List ==============================================================
  
  def -> add_action( this, "Dash", "if=!buff.cat_form.up" );
  def -> add_action( this, "Cat Form" );
  def -> add_talent( this, "Wild Charge" );
  def -> add_talent( this, "Displacer Beast", "if=movement.distance>10" );
  def -> add_action( this, "Dash", "if=movement.distance&buff.displacer_beast.down&buff.wild_charge_movement.down" );
  if ( race == RACE_NIGHT_ELF )
    def -> add_action( this, "Rake", "if=buff.prowl.up|buff.shadowmeld.up" );
  else
    def -> add_action( this, "Rake", "if=buff.prowl.up" );
  def -> add_action( "auto_attack" );
  def -> add_action( this, "Skull Bash" );
  def -> add_action( this, "Berserk", "if=buff.tigers_fury.up" );
  def -> add_action( "incarnation,if=cooldown.tigers_fury.remains<gcd" );

  // On-Use Items
  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].has_use_special_effect() )
    {
      std::string line = std::string( "use_item,slot=" ) + items[ i ].slot_name();
      if ( items[ i ].name_str == "mirror_of_the_blademaster" )
        line += ",if=raid_event.adds.in>60|!raid_event.adds.exists|spell_targets.swipe_cat>desired_targets";
      else if ( items[ i ].name_str != "maalus_the_blood_drinker" )
        line += ",if=(buff.tigers_fury.up&(target.time_to_die>trinket.stat.any.cooldown|target.time_to_die<45))|buff.incarnation.remains>20";

      def -> add_action( line );
    }
  }

  if ( sim -> allow_potions && true_level >= 80 )
    def -> add_action( potion_action +
                       ",if=((buff.berserk.remains>10|buff.incarnation.remains>20)&(target.time_to_die<180|(trinket.proc.all.react&target.health.pct<25)))|target.time_to_die<=40" );

  // Racials
  if ( race == RACE_TROLL )
    def -> add_action( "berserking,if=buff.tigers_fury.up" );

  def -> add_action( this, "Tiger's Fury",
                     "if=(!buff.clearcasting.react&energy.deficit>=60)|energy.deficit>=80|(t18_class_trinket&buff.berserk.up&buff.tigers_fury.down)" );
  def -> add_action( "incarnation,if=energy.time_to_max>1&energy>=35" );
  def -> add_action( this, "Ferocious Bite", "cycle_targets=1,if=dot.rip.ticking&dot.rip.remains<3&target.time_to_die>3&(target.health.pct<25|talent.sabertooth.enabled)",
                     "Keep Rip from falling off during execute range." );
  def -> add_action( this, "Regrowth",
                     "if=talent.bloodtalons.enabled&buff.predatory_swiftness.up&(combo_points>=5|buff.predatory_swiftness.remains<1.5"
                     "|(talent.bloodtalons.enabled&combo_points=2&buff.bloodtalons.down&cooldown.ashamanes_frenzy.remains<gcd)|"
                     "(talent.elunes_guidance.enabled&((cooldown.elunes_guidance.remains<gcd&combo_points=0)|(buff.elunes_guidance.up&combo_points>=4))))",
                     "Use Healing Touch at 5 Combo Points, if Predatory Swiftness is about to fall off, at 2 Combo Points before Ashamane's Frenzy, "
                     "before Elune's Guidance is cast or before the Elune's Guidance buff gives you a 5th Combo Point." );
  def -> add_action( "call_action_list,name=sbt_opener,if=talent.sabertooth.enabled&time<20" );
  def -> add_action( this, "Regrowth", "if=equipped.ailuro_pouncers&talent.bloodtalons.enabled&buff.predatory_swiftness.stack>1&buff.bloodtalons.down",
    "Special logic for Ailuro Pouncers legendary." );
  def -> add_action( "call_action_list,name=finisher" );
  def -> add_action( "call_action_list,name=generator" );

  // Finishers
  finish -> add_action( "pool_resource,for_next=1",
    "Use Savage Roar if it's expired and you're at 5 combo points or are about to use Brutal Slash" );
  finish -> add_talent( this, "Savage Roar",
    "if=!buff.savage_roar.up&(combo_points=5|(talent.brutal_slash.enabled&spell_targets.brutal_slash>desired_targets&action.brutal_slash.charges>0))" );
  finish -> add_action( "pool_resource,for_next=1",
    "Thrash has higher priority than finishers at 5 targets" );
  finish -> add_action( "thrash_cat,cycle_targets=1,if=remains<=duration*0.3&spell_targets.thrash_cat>=5" );
  finish -> add_action( "pool_resource,for_next=1",
    "Replace Rip with Swipe at 8 targets" );
  finish -> add_action( "swipe_cat,if=spell_targets.swipe_cat>=8" );
  finish -> add_action( this, "Rip", "cycle_targets=1,if=(!ticking|(remains<8&target.health.pct>25&!talent.sabertooth.enabled)|"
    "persistent_multiplier>dot.rip.pmultiplier)&target.time_to_die-remains>tick_time*4&combo_points=5&"
    "(energy.time_to_max<1|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3|"
    "set_bonus.tier18_4pc|buff.clearcasting.react|talent.soul_of_the_forest.enabled|!dot.rip.ticking|(dot.rake.remains<1.5&spell_targets.swipe_cat<6))",
    "Refresh Rip at 8 seconds or for a stronger Rip" );
  finish -> add_talent( this, "Savage Roar", "if=(buff.savage_roar.remains<=10.5|(buff.savage_roar.remains<=7.2&!talent.jagged_wounds.enabled))&"
    "combo_points=5&(energy.time_to_max<1|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3|"
    "set_bonus.tier18_4pc|buff.clearcasting.react|talent.soul_of_the_forest.enabled|!dot.rip.ticking|(dot.rake.remains<1.5&spell_targets.swipe_cat<6))",
    "Refresh Savage Roar early with Jagged Wounds" );
  finish -> add_action( "swipe_cat,if=combo_points=5&(spell_targets.swipe_cat>=6|(spell_targets.swipe_cat>=3&!talent.bloodtalons.enabled))&"
    "combo_points=5&(energy.time_to_max<1|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3|"
    "set_bonus.tier18_4pc|(talent.moment_of_clarity.enabled&buff.clearcasting.react))",
    "Replace FB with Swipe at 6 targets for Bloodtalons or 3 targets otherwise." );
  finish -> add_action( this, "Ferocious Bite", "max_energy=1,cycle_targets=1,if=combo_points=5&"
    "(energy.time_to_max<1|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3|"
    "set_bonus.tier18_4pc|(talent.moment_of_clarity.enabled&buff.clearcasting.react))" );

  // Generators
  generate -> add_talent( this, "Brutal Slash", "if=spell_targets.brutal_slash>desired_targets&combo_points<5",
    "Brutal Slash if there's adds up" );
  generate -> add_action( this, artifact.ashamanes_frenzy, "ashamanes_frenzy",
    "if=combo_points<=2&buff.elunes_guidance.down&(buff.bloodtalons.up|!talent.bloodtalons.enabled)&(buff.savage_roar.up|!talent.savage_roar.enabled)" );
  generate -> add_action( "pool_resource,if=talent.elunes_guidance.enabled&combo_points=0&energy<action.ferocious_bite.cost+25-energy.regen*cooldown.elunes_guidance.remains",
    "Pool energy for Elune's Guidance when it's coming off cooldown." );
  generate -> add_talent( this, "Elune's Guidance", "if=talent.elunes_guidance.enabled&combo_points=0&energy>=action.ferocious_bite.cost+25" );
  generate -> add_action( "pool_resource,for_next=1",
    "Spam Thrash over Rake or Moonfire at 9 targets with Brutal Slash talent." );
  generate -> add_action( "thrash_cat,if=talent.brutal_slash.enabled&spell_targets.thrash_cat>=9" );
  generate -> add_action( "pool_resource,for_next=1",
    "Use Swipe over Rake or Moonfire at 6 targets." );
  generate -> add_action( "swipe_cat,if=spell_targets.swipe_cat>=6" );
  if ( race == RACE_NIGHT_ELF )
    generate -> add_action( "shadowmeld,if=combo_points<5&energy>=action.rake.cost&dot.rake.pmultiplier<2.1&buff.tigers_fury.up&"
    "(buff.bloodtalons.up|!talent.bloodtalons.enabled)&(!talent.incarnation.enabled|cooldown.incarnation.remains>18)&!buff.incarnation.up",
    "Shadowmeld to buff Rake" );
  generate -> add_action( "pool_resource,for_next=1", "Refresh Rake early with Bloodtalons" );
  generate -> add_action( this, "Rake", "cycle_targets=1,if=combo_points<5&(!ticking|(!talent.bloodtalons.enabled&remains<duration*0.3)|"
    "(talent.bloodtalons.enabled&buff.bloodtalons.up&(!talent.soul_of_the_forest.enabled&remains<=7|remains<=5)&"
    "persistent_multiplier>dot.rake.pmultiplier*0.80))&target.time_to_die-remains>tick_time" );
  generate -> add_action( "moonfire_cat,cycle_targets=1,if=combo_points<5&remains<=4.2&target.time_to_die-remains>tick_time*2" );
  generate -> add_action( "pool_resource,for_next=1" );
  generate -> add_action( "thrash_cat,cycle_targets=1,if=remains<=duration*0.3&spell_targets.swipe_cat>=2" );
  generate -> add_talent( this, "Brutal Slash",
    "if=combo_points<5&((raid_event.adds.exists&raid_event.adds.in>(1+max_charges-charges_fractional)*15)|(!raid_event.adds.exists&(charges_fractional>2.66&time>10)))",
    "Brutal Slash if you would cap out charges before the next adds spawn" );
  generate -> add_action( "swipe_cat,if=combo_points<5&spell_targets.swipe_cat>=3" );
  generate -> add_action( this, "Shred", "if=combo_points<5&(spell_targets.swipe_cat<3|talent.brutal_slash.enabled)" );

  // Sabertooth Opener
  sbt -> add_action( this, "Regrowth", "if=talent.bloodtalons.enabled&combo_points=5&!buff.bloodtalons.up&!dot.rip.ticking",
    "Hard-cast a Healing Touch for Bloodtalons buff. Use Dash to re-enter Cat Form." );
  sbt -> add_action( this, "Tiger's Fury", "if=!dot.rip.ticking&combo_points=5",
    "Force use of Tiger's Fury before applying Rip." );
}

// Balance Combat Action Priority List ======================================

void druid_t::apl_balance()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  std::vector<std::string> item_actions   = get_item_actions();
  std::string              potion_action  = "potion,name=";
  if ( true_level > 100 )
    potion_action += "deadly_grace";
  else if ( true_level > 90 )
    potion_action += "draenic_intellect";
  else if ( true_level > 85 )
    potion_action += "jade_serpent";
  else
    potion_action += "volcanic";

  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  action_priority_list_t* FoE                 = get_action_priority_list( "fury_of_elune"  );
  action_priority_list_t* CA                  = get_action_priority_list( "celestial_alignment_phase");
  action_priority_list_t* ST                  = get_action_priority_list( "single_target" );
  action_priority_list_t* ED                  = get_action_priority_list( "ed" );

  if ( sim -> allow_potions && true_level >= 80 )
    default_list -> add_action( potion_action + ",if=buff.celestial_alignment.up|buff.incarnation.up" );

  default_list -> add_action( "blessing_of_elune,if=active_enemies<=2&talent.blessing_of_the_ancients.enabled&buff.blessing_of_elune.down" );
  default_list -> add_action( "blessing_of_elune,if=active_enemies>=3&talent.blessing_of_the_ancients.enabled&buff.blessing_of_anshe.down" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] + ",if=buff.celestial_alignment.up|buff.incarnation.up" );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list -> add_action( "call_action_list,name=fury_of_elune,if=talent.fury_of_elune.enabled&cooldown.fury_of_elue.remains<target.time_to_die" );
  default_list -> add_action( "call_action_list,name=ed,if=equipped.the_emerald_dreamcatcher" );
  default_list -> add_action( this, "New Moon", "if=(charges=2&recharge_time<5)|charges=3" );
  default_list -> add_action( this, "Half Moon", "if=(charges=2&recharge_time<5)|charges=3|(target.time_to_die<15&charges=2)");
  default_list -> add_action( this, "Full Moon", "if=(charges=2&recharge_time<5)|charges=3|target.time_to_die<15");
  default_list -> add_talent( this, "Stellar Flare", "cycle_targets=1,max_cycle_targets=4,if=active_enemies<4&remains<7.2&astral_power>=15");
  default_list -> add_action( this, "Moonfire", "if=(talent.natures_balance.enabled&remains<3)|(remains<6.6&!talent.natures_balance.enabled)" );
  default_list -> add_action( this, "Sunfire", "if=(talent.natures_balance.enabled&remains<3)|(remains<5.4&!talent.natures_balance.enabled)" );
  default_list -> add_talent( this, "Astral Communion", "if=astral_power.deficit>=75" );
  default_list -> add_action( "incarnation,if=astral_power>=40" );
  default_list -> add_action( this, "Celestial Alignment", "if=astral_power>=40" );
  default_list -> add_action( this, "Starfall", "if=buff.oneths_overconfidence.up" );
  default_list -> add_action( this, "Solar Wrath", "if=buff.solar_empowerment.stack=3" );
  default_list -> add_action( this, "Lunar Strike", "if=buff.lunar_empowerment.stack=3" );
  
  default_list -> add_action( "call_action_list,name=celestial_alignment_phase,if=buff.celestial_alignment.up|buff.incarnation.up" );
  default_list -> add_action( "call_action_list,name=single_target" );
  
  CA -> add_action( this, "Starfall", "if=(active_enemies>=2&talent.stellar_flare.enabled|active_enemies>=3)&((talent.fury_of_elune.enabled&cooldown.fury_of_elune.remains>12&buff.fury_of_elune_up.down)|!talent.fury_of_elune.enabled)" );
  CA -> add_action( this, "Starsurge", "if=active_enemies<=2" );
  CA -> add_talent( this, "Warrior of Elune" );
  CA -> add_action( this, "Lunar Strike", "if=buff.warrior_of_elune.up" );
  CA -> add_action( this, "Solar Wrath", "if=buff.solar_empowerment.up" );
  CA -> add_action( this, "Lunar Strike", "if=buff.lunar_empowerment.up" );
  CA -> add_action( this, "Solar Wrath", "if=talent.natures_balance.enabled&dot.sunfire_dmg.remains<5&cast_time<dot.sunfire_dmg.remains" );
  CA -> add_action( this, "Lunar Strike", "if=(talent.natures_balance.enabled&dot.moonfire_dmg.remains<5&cast_time<dot.moonfire_dmg.remains)|active_enemies>=2" );  
  CA -> add_action( this, "Solar Wrath" );
  
  ST -> add_action( this, "New Moon", "if=astral_power<=90" );
  ST -> add_action( this, "Half Moon", "if=astral_power<=80" );
  ST -> add_action( this, "Full Moon", "if=astral_power<=60" );
  ST -> add_action( this, "Starfall", "if=(active_enemies>=2&talent.stellar_flare.enabled|active_enemies>=3)&((talent.fury_of_elune.enabled&cooldown.fury_of_elune.remains>12&buff.fury_of_elune_up.down)|!talent.fury_of_elune.enabled)" );
  ST -> add_action( this, "Starsurge", "if=active_enemies<=2" );
  ST -> add_talent( this, "Warrior of Elune" );
  ST -> add_action( this, "Lunar Strike", "if=buff.warrior_of_elune.up" );
  ST -> add_action( this, "Solar Wrath", "if=buff.solar_empowerment.up" );
  ST -> add_action( this, "Lunar Strike", "if=buff.lunar_empowerment.up" );
  ST->  add_action( this, "Solar Wrath", "if=talent.natures_balance.enabled&dot.sunfire_dmg.remains<5&cast_time<dot.sunfire_dmg.remains" );
  ST->  add_action( this, "Lunar Strike", "if=(talent.natures_balance.enabled&dot.moonfire_dmg.remains<5&cast_time<dot.moonfire_dmg.remains)|active_enemies>=2" );
  ST -> add_action( this, "Solar Wrath" );
  
  FoE -> add_action( "incarnation,if=astral_power>=95&cooldown.fury_of_elune.remains<=gcd" );
  FoE -> add_action( this, "astral_power>=95&cooldown.fury_of_elune.remains<=gcd" );
  FoE -> add_talent( this, "Fury of Elune", "if=astral_power>=95" );
  FoE -> add_action( this, "New Moon", "if=((charges=2&recharge_time<5)|charges=3)&&(buff.fury_of_elune_up.up|(cooldown.fury_of_elune.remains>gcd*3&astral_power<=90))" );
  FoE -> add_action( this, "Half Moon", "if=((charges=2&recharge_time<5)|charges=3)&&(buff.fury_of_elune_up.up|(cooldown.fury_of_elune.remains>gcd*3&astral_power<=80))" );
  FoE -> add_action( this, "Full Moon", "if=((charges=2&recharge_time<5)|charges=3)&&(buff.fury_of_elune_up.up|(cooldown.fury_of_elune.remains>gcd*3&astral_power<=60))" );
  FoE -> add_talent( this, "Astral Communion", "if=buff.fury_of_elune_up.up&astral_power<=25" );
  FoE -> add_talent( this, "Warrior of Elune", "if=buff.fury_of_elune_up.up|(cooldown.fury_of_elune.remains>=35&buff.lunar_empowerment.up)" );
  FoE -> add_action( this, "Lunar Strike", "if=buff.warrior_of_elune.up&(astral_power<=90|(astral_power<=85&buff.incarnation.up))" );
  FoE -> add_action( this, "New Moon", "if=astral_power<=90&buff.fury_of_elune_up.up" );
  FoE -> add_action( this, "Half Moon", "if=astral_power<=80&buff.fury_of_elune_up.up&astral_power>cast_time*12" );
  FoE -> add_action( this, "Full Moon", "if=astral_power<=60&buff.fury_of_elune_up.up&astral_power>cast_time*12" );
  FoE -> add_action( this, "Moonfire", "if=buff.fury_of_elune_up.down&remains<=6.6" );
  FoE -> add_action( this, "Sunfire", "if=buff.fury_of_elune_up.down&remains<5.4" );
  FoE -> add_talent( this, "Stellar Flare", "if=remains<7.2&active_enemies=1" );
  FoE -> add_action( this, "Starfall", "if=(active_enemies>=2&talent.stellar_flare.enabled|active_enemies>=3)&buff.fury_of_elune_up.down&cooldown.fury_of_elune.remains>10" );
  FoE -> add_action( this, "Starsurge", "if=active_enemies<=2&buff.fury_of_elune_up.down&cooldown.fury_of_elune.remains>7" );
  FoE -> add_action( this, "Starsurge", "if=buff.fury_of_elune_up.down&((astral_power>=92&cooldown.fury_of_elune.remains>gcd*3)|(cooldown.warrior_of_elune.remains<=5&cooldown.fury_of_elune.remains>=35&buff.lunar_empowerment.stack<2))" );
  FoE -> add_action( this, "Solar Wrath", "if=buff.solar_empowerment.up" );
  FoE -> add_action( this, "Lunar Strike", "if=buff.lunar_empowerment.stack=3|(buff.lunar_empowerment.remains<5&buff.lunar_empowerment.up)|active_enemies>=2" );
  FoE -> add_action( this, "Solar Wrath" );

  ED -> add_talent( this, "Astral Communion", "if=astral_power.deficit>=75&buff.the_emerald_dreamcatcher.up" );
  ED -> add_action( "incarnation,if=astral_power>=85&!buff.the_emerald_dreamcatcher.up" );
  ED -> add_action( this, "Celestial Alignment", "if=astral_power>=85&!buff.the_emerald_dreamcatcher.up" );
  ED -> add_action( this, "Starsurge", "if=(buff.the_emerald_dreamcatcher.up&buff.the_emerald_dreamcatcher.remains<gcd.max)|astral_power>=90|((buff.celestial_alignment.up|buff.incarnation.up)&astral_power>=85)" );
  ED -> add_talent( this, "Stellar Flare", "cycle_targets=1,max_cycle_targets=4,if=active_enemies<4&remains<7.2&astral_power>=15" );
  ED -> add_action( this, "Moonfire", "if=(talent.natures_balance.enabled&remains<3)|(remains<6.6&!talent.natures_balance.enabled)" );
  ED -> add_action( this, "Sunfire", "if=(talent.natures_balance.enabled&remains<3)|(remains<5.4&!talent.natures_balance.enabled)" );
  ED -> add_action( this, "Solar Wrath", "if=buff.solar_empowerment.up&buff.the_emerald_dreamcatcher.remains>execute_time&astral_power>=12&dot.sunfire.remains<5.4&dot.moonfire.remains>6.6" );
  ED -> add_action( this, "Lunar Strike", "if=buff.lunar_empowerment.up&buff.the_emerald_dreamcatcher.remains>execute_time&astral_power>=8&(!(buff.celestial_alignment.up|buff.incarnation.up)|(buff.celestial_alignment.up|buff.incarnation.up)&astral_power<=77)" );
  ED -> add_action( this, "New Moon", "if=astral_power<=90" );
  ED -> add_action( this, "Half Moon", "if=astral_power<=80" );
  ED -> add_action( this, "Full Moon", "if=astral_power<=60" );
  ED -> add_action( this, "Solar Wrath", "if=buff.solar_empowerment.up" );
  ED -> add_action( this, "Lunar Strike", "if=buff.lunar_empowerment.up" );
  ED -> add_action( this, "Solar Wrath" );

}

// Guardian Combat Action Priority List =====================================

void druid_t::apl_guardian()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  default_list -> add_action( "auto_attack" );

  for (size_t i = 0; i < racial_actions.size(); i++)
    default_list->add_action(racial_actions[i]);
  for (size_t i = 0; i < item_actions.size(); i++)
    default_list->add_action(item_actions[i]);

  default_list->add_action("incarnation");
  default_list->add_action("rage_of_the_sleeper");
  default_list->add_action("lunar_beam");
  default_list->add_action(this, "Frenzied Regeneration", "if=incoming_damage_5s%health.max>=0.5|health<=health.max*0.4");
  default_list->add_action( "bristling_fur,if=buff.ironfur.stack=1|buff.ironfur.down" );
  default_list->add_action(this, "Ironfur", "if=(buff.ironfur.up=0)|(buff.gory_fur.up=1)|(rage>=80)");
  default_list->add_action(this, "Moonfire", "if=buff.incarnation.up=1&dot.moonfire.remains<=4.8");
  default_list->add_action("thrash_bear,if=buff.incarnation.up=1&dot.thrash.remains<=4.5");
  default_list->add_action(this, "Mangle");
  default_list->add_action("thrash_bear");
  default_list->add_action("pulverize,if=buff.pulverize.up=0|buff.pulverize.remains<=6");
  default_list->add_action(this, "Moonfire", "if=buff.galactic_guardian.up=1&(!ticking|dot.moonfire.remains<=4.8)");
  default_list->add_action(this, "Moonfire", "if=buff.galactic_guardian.up=1");
  default_list->add_action(this, "Moonfire", "if=dot.moonfire.remains<=4.8");
  default_list->add_action("swipe_bear");
}

// Restoration Combat Action Priority List ==================================

void druid_t::apl_restoration()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list -> add_action( this, "Natures Swiftness" );
  default_list -> add_talent( this, "Incarnation" );
  default_list -> add_action( this, "Healing Touch", "if=buff.clearcasting.up" );
  default_list -> add_action( this, "Rejuvenation", "if=remains<=duration*0.3" );
  default_list -> add_action( this, "Lifebloom", "if=debuff.lifebloom.down" );
  default_list -> add_action( this, "Swiftmend" );
  default_list -> add_action( this, "Healing Touch" );
}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  scales_with[ STAT_STRENGTH ] = false;

  // Save a copy of the weapon
  caster_form_weapon = main_hand_weapon;

  // Bear/Cat form weapons need to be scaled up if we are calculating scale factors for the weapon
  // dps. The actual cached cat/bear form weapons are created before init_scaling is called, so the
  // adjusted values for the "main hand weapon" have not yet been added.
  if ( sim -> scaling -> scale_stat == STAT_WEAPON_DPS )
  {
    if ( cat_weapon.damage > 0 )
    {
      auto coeff = sim -> scaling -> scale_value * cat_weapon.swing_time.total_seconds();
      cat_weapon.damage  += coeff;
      cat_weapon.min_dmg += coeff;
      cat_weapon.max_dmg += coeff;
    }

    if ( bear_weapon.damage > 0 )
    {
      auto coeff = sim -> scaling -> scale_value * bear_weapon.swing_time.total_seconds();
      bear_weapon.damage  += coeff;
      bear_weapon.min_dmg += coeff;
      bear_weapon.max_dmg += coeff;
    }
  }
}

// druid_t::init ============================================================

void druid_t::init()
{
  player_t::init();

  if ( specialization() == DRUID_RESTORATION )
    sim -> errorf( "%s is using an unsupported spec.", name() );
}

// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  // Multiple Specs / Forms
  gain.clearcasting          = get_gain( "clearcasting" );       // Feral & Restoration
  gain.soul_of_the_forest    = get_gain( "soul_of_the_forest" ); // Feral & Guardian

  // Balance
  gain.astral_communion      = get_gain( "astral_communion"      );
  gain.blessing_of_anshe     = get_gain( "blessing_of_anshe"     );
  gain.lunar_strike          = get_gain( "lunar_strike"          );
  gain.moonfire              = get_gain( "moonfire"              );
  gain.shooting_stars        = get_gain( "shooting_stars"        );
  gain.solar_wrath           = get_gain( "solar_wrath"           );
  gain.sunfire               = get_gain( "sunfire"               );


  // Feral
  gain.ashamanes_energy      = get_gain( "ashamanes_energy"      );
  gain.ashamanes_frenzy      = get_gain( "ashamanes_frenzy"      );
  gain.brutal_slash          = get_gain( "brutal_slash"          );
  gain.energy_refund         = get_gain( "energy_refund"         );
  gain.elunes_guidance       = get_gain( "elunes_guidance"       );
  gain.primal_fury           = get_gain( "primal_fury"           );
  gain.rake                  = get_gain( "rake"                  );
  gain.shred                 = get_gain( "shred"                 );
  gain.swipe_cat             = get_gain( "swipe_cat"             );
  gain.tigers_fury           = get_gain( "tigers_fury"           );

  // Guardian
  gain.bear_form             = get_gain( "bear_form"             );
  gain.blood_frenzy          = get_gain( "blood_frenzy"          );
  gain.brambles              = get_gain( "brambles"              );
  gain.bristling_fur         = get_gain( "bristling_fur"         );
  gain.galactic_guardian     = get_gain( "galactic_guardian"     );
  gain.gore                  = get_gain( "gore"                  );
  gain.rage_refund           = get_gain( "rage_refund"           );
  gain.stalwart_guardian     = get_gain( "stalwart_guardian"     );

  // Set Bonuses
  gain.feral_tier17_2pc      = get_gain( "feral_tier17_2pc"      );
  gain.feral_tier18_4pc      = get_gain( "feral_tier18_4pc"      );
  gain.feral_tier19_2pc      = get_gain( "feral_tier19_2pc"      );
  gain.guardian_tier17_2pc   = get_gain( "guardian_tier17_2pc"   );
  gain.guardian_tier18_2pc   = get_gain( "guardian_tier18_2pc"   );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  proc.ashamanes_bite           = get_proc( "ashamanes_bite"         );
  proc.clearcasting             = get_proc( "clearcasting"           );
  proc.clearcasting_wasted      = get_proc( "clearcasting_wasted"    );
  proc.gore                     = get_proc( "gore"                   );
  proc.the_wildshapers_clutch   = get_proc( "the_wildshapers_clutch" );
  proc.predator                 = get_proc( "predator"               );
  proc.predator_wasted          = get_proc( "predator_wasted"        );
  proc.primal_fury              = get_proc( "primal_fury"            );
  proc.starshards               = get_proc( "Starshards"             );
  proc.tier17_2pc_melee         = get_proc( "tier17_2pc_melee"       );
}

// druid_t::init_resources ==================================================

void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ] = 0;
  resources.current[ RESOURCE_COMBO_POINT ] = 0;
  resources.current[ RESOURCE_ASTRAL_POWER ] = initial_astral_power;
  expected_max_health = calculate_expected_max_health();
}

// druid_t::init_rng ========================================================

void druid_t::init_rng()
{
  // RPPM objects
  rppm.balance_tier18_2pc = get_rppm( "balance_tier18_2pc", sets.set( DRUID_BALANCE, T18, B2 ) );
  rppm.predator           = get_rppm( "predator", predator_rppm_rate ); // Predator: optional RPPM approximation.
  rppm.shadow_thrash      = get_rppm( "shadow_thrash", artifact.shadow_thrash );

  player_t::init_rng();
}

// druid_t::init_actions ====================================================

void druid_t::init_action_list()
{
#ifdef NDEBUG // Only restrict on release builds.
  // Restoration isn't fully supported atm
  if ( specialization() == DRUID_RESTORATION )
  {
    if ( ! quiet )
      sim -> errorf( "Druid restoration healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }
#endif
  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat(); // PRE-COMBAT

  switch ( specialization() )
  {
  case DRUID_FERAL:
    apl_feral(); // FERAL
    break;
  case DRUID_BALANCE:
    apl_balance();  // BALANCE
    break;
  case DRUID_GUARDIAN:
    apl_guardian(); // GUARDIAN
    break;
  case DRUID_RESTORATION:
    apl_restoration(); // RESTORATION
    break;
  default:
    apl_default(); // DEFAULT
    break;
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
  moon_stage = ( moon_stage_e ) initial_moon_stage;

  base_gcd = timespan_t::from_seconds( 1.5 );

  // Restore main hand attack / weapon to normal state
  main_hand_attack = caster_melee_attack;
  main_hand_weapon = caster_form_weapon;

  // Reset any custom events to be safe.
  persistent_buff_delay.clear();

  if ( mastery.natures_guardian -> ok() )
    recalculate_resource_max( RESOURCE_HEALTH );
}

// druid_t::merge ===========================================================

void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ] -> merge( *od.counters[ i ] );
}

// druid_t::mana_regen_per_second ===========================================

double druid_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  if ( buff.moonkin_form -> check() ) // Boomkins get 150% increased mana regeneration, scaling with haste. Legion TOCHECK
    mp5 *= 1.0 + buff.moonkin_form -> data().effectN( 5 ).percent() + ( 1 / cache.spell_haste() );

  return mp5;
}

// druid_t::available =======================================================

timespan_t druid_t::available() const
{
  if ( primary_resource() != RESOURCE_ENERGY )
    return timespan_t::from_seconds( 0.1 );

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 ) return timespan_t::from_seconds( 0.1 );

  return std::max(
           timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
           timespan_t::from_seconds( 0.1 )
         );
}

// druid_t::arise ===========================================================

void druid_t::arise()
{
  player_t::arise();

  if ( talent.earthwarden -> ok() )
    buff.earthwarden -> trigger( buff.earthwarden -> max_stack() );

  // Trigger persistent buffs
  if ( buff.yseras_gift )
    persistent_buff_delay.push_back( make_event<persistent_buff_delay_event_t>( *sim, this, buff.yseras_gift ) );

  if ( talent.earthwarden -> ok() )
    persistent_buff_delay.push_back( make_event<persistent_buff_delay_event_t>( *sim, this, buff.earthwarden_driver ) );

  if ( legendary.ailuro_pouncers > timespan_t::zero() )
  {
    timespan_t preproc = legendary.ailuro_pouncers * rng().real();
    if ( preproc < buff.predatory_swiftness -> buff_duration )
    {
      buff.predatory_swiftness -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0,
        buff.predatory_swiftness -> buff_duration - preproc );
    }

    make_event<ailuro_pouncers_event_t>( *sim, this, timespan_t::from_seconds( 15 ) - preproc );
  }
}

// druid_t::recalculate_resource_max ========================================

void druid_t::recalculate_resource_max( resource_e rt )
{
  double pct_health = 0, current_health = 0;
  bool adjust_natures_guardian_health = mastery.natures_guardian -> ok() && rt == RESOURCE_HEALTH;
  if ( adjust_natures_guardian_health )
  {
    current_health = resources.current[ rt ];
    pct_health = resources.pct( rt );
  }

  player_t::recalculate_resource_max( rt );

  if ( adjust_natures_guardian_health )
  {
    resources.max[ rt ] *= 1.0 + cache.mastery_value();
    // Maintain current health pct.
    resources.current[ rt ] = resources.max[ rt ] * pct_health;

    if ( sim -> log )
      sim -> out_log.printf( "%s recalculates maximum health. old_current=%.0f new_current=%.0f net_health=%.0f",
                             name(), current_health, resources.current[ rt ], resources.current[ rt ] - current_health );
  }
}

// druid_t::invalidate_cache ================================================

void druid_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_ATTACK_POWER:
    if ( spec.nurturing_instinct -> ok() )
      invalidate_cache( CACHE_SPELL_POWER );
    break;
  case CACHE_INTELLECT:
    if ( spec.killer_instinct -> ok() && ( buff.cat_form -> check() || buff.bear_form -> check() ) )
      invalidate_cache( CACHE_AGILITY );
    break;
  case CACHE_MASTERY:
    if ( mastery.natures_guardian -> ok() )
    {
      invalidate_cache( CACHE_ATTACK_POWER );
      recalculate_resource_max( RESOURCE_HEALTH );
    }
    break;
  case CACHE_CRIT_CHANCE:
    if ( specialization() == DRUID_GUARDIAN )
      invalidate_cache( CACHE_DODGE );
  default:
    break;
  }
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  // All modifiers MUST invalidate CACHE_ATTACK_POWER or Nurturing Instinct will break.

  if ( mastery.natures_guardian -> ok() )
    ap *= 1.0 + cache.mastery() * mastery.natures_guardian_AP -> effectN( 1 ).mastery_value();

  return ap;
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form -> check() )
    a *= 1.0 + buff.bear_form -> data().effectN( 3 ).percent();

  if ( buff.moonkin_form -> check() )
    a *= 1.0 + buff.moonkin_form -> data().effectN( 3 ).percent() + artifact.bladed_feathers.percent();

  a *= 1.0 + buff.ironfur -> check_stack_value();

  a *= 1.0 + buff.protection_of_ashamane -> check() * buff.protection_of_ashamane -> data().effectN( 2 ).percent();

  a *= 1.0 + artifact.iron_claws.percent();

  return a;
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) )
  {
    m *= 1.0 + buff.balance_tier18_4pc -> check_value();

    if ( buff.moonkin_form -> check() )
      m *= 1.0 + buff.moonkin_form -> data().effectN( 8 ).percent();
  }

  // Tiger's Fury and Savage Roar are player multipliers. Their "snapshotting" for cat abilities is handled in cat_attack_t.
  m *= 1.0 + buff.tigers_fury -> check_value();
  m *= 1.0 + buff.savage_roar -> check_value();

  // Damage modifier applies to all "magic" damage.
  if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
  {
    m *= 1.0 + buff.celestial_alignment -> check_value();
    m *= 1.0 + buff.incarnation_moonkin -> check_value();
  }

  m *= 1.0 + buff.feral_instinct -> check_value();

  if ( artifact.embrace_of_the_nightmare.rank() )
    m *= 1.0 + buff.rage_of_the_sleeper -> check() * buff.rage_of_the_sleeper -> data().effectN( 5 ).percent();

  return m;
}

// druid_t::composite_rating_multiplier ====================================

double druid_t::composite_rating_multiplier( rating_e rating ) const
{
  double rm = player_t::composite_rating_multiplier( rating );

  switch( rating )
  {
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      rm *= 1.0 + spec.feral -> effectN( 3 ).percent();
      break;
    default:
      break;
  }

  return rm;
}

// druid_t::composite_melee_expertise( weapon_t* ) ==========================

double druid_t::composite_melee_expertise( const weapon_t* ) const
{
  double exp = player_t::composite_melee_expertise();

  if ( buff.bear_form -> check() )
    exp += buff.bear_form -> data().effectN( 6 ).base_value();

  return exp;
}

// druid_t::composite_melee_crit_chance ============================================

double druid_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::temporary_movement_modifier =====================================

double druid_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

  if ( buff.dash -> up() )
    active = std::max( active, buff.dash -> value() );

  if ( buff.wild_charge_movement -> up() )
    active = std::max( active, buff.wild_charge_movement -> value() );

  if ( buff.displacer_beast -> up() )
    active = std::max( active, buff.displacer_beast -> value() );

  return active;
}

// druid_t::passive_movement_modifier =======================================

double druid_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buff.cat_form -> up() )
    ms += spec.cat_form_speed -> effectN( 1 ).percent();

  ms += spec.feline_swiftness -> effectN( 1 ).percent();

  return ms;
}

// druid_t::composite_spell_crit_chance ============================================

double druid_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::composite_spell_haste ===========================================

double druid_t::composite_spell_haste() const
{
  double sh = player_t::composite_spell_haste();

  sh /= 1.0 + buff.star_power -> check_stack_value();

  return sh;
}

// druid_t::composite_spell_power ===========================================

double druid_t::composite_spell_power( school_e school ) const
{
  // Nurturing Instinct overrides SP from other sources.
  if ( spec.nurturing_instinct -> ok() )
  {
    return spec.nurturing_instinct -> effectN( 1 ).percent() * cache.attack_power() *
      composite_attack_power_multiplier();
  }

  return player_t::composite_spell_power( school );
}

// druid_t::composite_spell_power_multiplier ================================

double druid_t::composite_spell_power_multiplier() const
{
  if ( spec.nurturing_instinct -> ok() )
  {
    return 1.0;
  }

  return player_t::composite_spell_power_multiplier();
}

// druid_t::composite_attribute =============================================

double druid_t::composite_attribute( attribute_e attr ) const
{
  double a = player_t::composite_attribute( attr );

  switch ( attr )
  {
  case ATTR_AGILITY:
    if ( spec.killer_instinct -> ok() && ( buff.bear_form -> up() || buff.cat_form -> up() ) )
    {
      // 100% of non-base Intellect added as Agility
      a += spec.killer_instinct -> effectN( 1 ).percent() * ( cache.intellect() - base.stats.attribute[ STAT_INTELLECT ] );
    }
    break;
  default:
    break;
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
    if ( buff.bear_form -> check() )
      m *= 1.0 + spec.bear_form -> effectN( 2 ).percent();
    m *= 1.0 + artifact.bestial_fortitude.percent();
    break;
  default:
    break;
  }

  return m;
}

// druid_t::matching_gear_multiplier ========================================

double druid_t::matching_gear_multiplier( attribute_e attr ) const
{
  unsigned idx;

  switch ( attr )
  {
  case ATTR_AGILITY:
    idx = 1;
    break;
  case ATTR_INTELLECT:
    idx = 2;
    break;
  case ATTR_STAMINA:
    idx = 3;
    break;
  default:
    return 0;
  }

  return spec.leather_specialization -> effectN( idx ).percent();
}

// druid_t::composite_crit_avoidance ========================================

double druid_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  if ( buff.bear_form -> check() )
    c += buff.bear_form -> data().effectN( 2 ).percent();

  return c;
}

// druid_t::composite_dodge =================================================

double druid_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  d += buff.protection_of_ashamane -> check_value();

  return d;
}

// druid_t::composite_dodge_rating ==========================================

double druid_t::composite_dodge_rating() const
{
  double dr = player_t::composite_dodge_rating();

  // FIXME: Spell dataify me.
  if ( specialization() == DRUID_GUARDIAN )
  {
    dr += composite_rating( RATING_MELEE_CRIT );
  }

  return dr;
}

// druid_t::composite_leech =================================================

double druid_t::composite_leech() const
{
  double l = player_t::composite_leech();

  if ( artifact.embrace_of_the_nightmare.rank() )
    l += buff.rage_of_the_sleeper -> check() * buff.rage_of_the_sleeper -> data().effectN( 4 ).percent();

  return l;
}

// druid_t::create_expression ===============================================

expr_t* druid_t::create_expression( action_t* a, const std::string& name_str )
{
  struct druid_expr_t : public expr_t
  {
    druid_t& druid;
    druid_expr_t( const std::string& n, druid_t& p ) :
      expr_t( n ), druid( p )
    {}
  };

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( util::str_compare_ci( name_str, "astral_power" ) )
  {
    return make_ref_expr( name_str, resources.current[ RESOURCE_ASTRAL_POWER ] );
  }
  else if ( util::str_compare_ci( name_str, "combo_points" ) )
  {
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );
  }
  else if ( util::str_compare_ci( name_str, "new_moon" )
            || util::str_compare_ci( name_str, "half_moon" )
            || util::str_compare_ci( name_str, "full_moon" )  )
  {
    struct moon_stage_expr_t : public druid_expr_t
    {
      int stage;

      moon_stage_expr_t( druid_t& p, const std::string& name_str ) :
        druid_expr_t( name_str, p )
      {
        if ( util::str_compare_ci( name_str, "new_moon" ) )
          stage = 0;
        else if ( util::str_compare_ci( name_str, "half_moon" ) )
          stage = 1;
        else if ( util::str_compare_ci( name_str, "full_moon" ) )
          stage = 2;
        else
          assert( "Bad name_str passed to moon_stage_expr_t" );
      }

      virtual double evaluate() override
      { return druid.moon_stage == stage; }
    };

    return new moon_stage_expr_t( *this, name_str );
  }

  return player_t::create_expression( a, name_str );
}

// druid_t::create_options ==================================================

void druid_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "predator_rppm", predator_rppm_rate ) );
  add_option( opt_float( "initial_astral_power", initial_astral_power ) );
  add_option( opt_int( "initial_moon_stage", initial_moon_stage ) );
}

// druid_t::create_profile ==================================================

std::string druid_t::create_profile( save_e type )
{
  return player_t::create_profile( type );
}

// druid_t::primary_role ====================================================

role_e druid_t::primary_role() const
{
  if ( specialization() == DRUID_BALANCE )
  {
    if ( player_t::primary_role() == ROLE_HEAL )
      return ROLE_HEAL;

    return ROLE_SPELL;
  }

  else if ( specialization() == DRUID_FERAL )
  {
    if ( player_t::primary_role() == ROLE_TANK )
      return ROLE_TANK;

    return ROLE_ATTACK;
  }

  else if ( specialization() == DRUID_GUARDIAN )
  {
    if ( player_t::primary_role() == ROLE_ATTACK )
      return ROLE_ATTACK;

    return ROLE_TANK;
  }

  else if ( specialization() == DRUID_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_HEAL;
  }

  return player_t::primary_role();
}

// druid_t::convert_hybrid_stat =============================================

stat_e druid_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
    switch ( specialization() )
    {
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
      return STAT_INTELLECT;
    case DRUID_FERAL:
    case DRUID_GUARDIAN:
      return STAT_AGILITY;
    default:
      return STAT_NONE;
    }
  case STAT_AGI_INT:
    if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
      return STAT_INTELLECT;
    else
      return STAT_AGILITY;
  // This is a guess at how AGI/STR gear will work for Balance/Resto, TODO: confirm
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Feral/Guardian, TODO: confirm
  // This should probably never come up since druids can't equip plate, but....
  case STAT_STR_INT:
    return STAT_INTELLECT;
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
  default:
    return s;
  }
}

// druid_t::primary_resource ================================================

resource_e druid_t::primary_resource() const
{
  if ( specialization() == DRUID_BALANCE && primary_role() == ROLE_SPELL )
    return RESOURCE_ASTRAL_POWER;

  if ( primary_role() == ROLE_HEAL || primary_role() == ROLE_SPELL )
    return RESOURCE_MANA;

  if ( primary_role() == ROLE_TANK )
    return RESOURCE_RAGE;

  return RESOURCE_ENERGY;
}

// druid_t::init_absorb_priority ============================================

void druid_t::init_absorb_priority()
{
  player_t::init_absorb_priority();

  absorb_priority.push_back( 184878 ); // Stalwart Guardian
  absorb_priority.push_back( talent.brambles -> id() ); // Brambles
  absorb_priority.push_back( talent.earthwarden -> id() ); // Earthwarden - Legion TOCHECK
}

// druid_t::target_mitigation ===============================================

void druid_t::target_mitigation( school_e school, dmg_e type, action_state_t* s )
{
  s -> result_amount *= 1.0 + buff.barkskin -> value();

  s -> result_amount *= 1.0 + buff.survival_instincts -> value();

  s -> result_amount *= 1.0 + buff.pulverize -> value();
  
  s -> result_amount *= 1.0 + buff.rage_of_the_sleeper -> value();

  if ( spec.thick_hide )
    s -> result_amount *= 1.0 + spec.thick_hide -> effectN( 1 ).percent();

  if ( talent.rend_and_tear -> ok() )
    s -> result_amount *= 1.0 - talent.rend_and_tear -> effectN( 2 ).percent()
      * get_target_data( s -> action -> player ) -> dots.thrash_bear -> current_stack();

  if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
  {
    s -> result_amount *= 1.0 + buff.mark_of_ursol -> value();
  
    if ( buff.adaptive_fur -> check() && dbc::is_school( school, ( school_e ) ( int ) buff.adaptive_fur -> check_value() ) ) // TOCHECK
    {
      s -> result_amount *= 1.0 + buff.adaptive_fur -> value();
    }
    else
    {
      debug_cast<buffs::adaptive_fur_t*>( buff.adaptive_fur ) -> no_benefit();
    }
  }

  player_t::target_mitigation( school, type, s );
}

// druid_t::assess_damage ===================================================

void druid_t::assess_damage( school_e school,
                             dmg_e    dtype,
                             action_state_t* s )
{
  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && dtype == DMG_DIRECT &&
    s -> result == RESULT_HIT )
  {
    buff.ironfur -> up();
  }

  player_t::assess_damage( school, dtype, s );

  if ( artifact.adaptive_fur.rank() && dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
  {
    if ( buff.adaptive_fur -> trigger( 1, school ) && sim -> log )
    {
      sim -> out_log.printf( "%s %s adapts to %s (%d).", name(), buff.adaptive_fur -> name(),
        util::school_type_string( school ), school );
    }
  }
}

// druid_t::assess_damage_imminent_pre_absorb ===============================

// Trigger effects based on being hit or taking damage.

void druid_t::assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* s )
{
  if ( action_t::result_is_hit( s -> result ) && s -> result_amount > 0 )
  {
    if ( buff.cenarion_ward -> up() )
      active.cenarion_ward_hot -> execute();

    if ( buff.bristling_fur -> up() )
    {
      // 1 rage per 1% of maximum health taken
      resource_gain( RESOURCE_RAGE,
                     s -> result_amount / expected_max_health * 100,
                     gain.bristling_fur );
    }

    if ( buff.rage_of_the_sleeper -> up() )
    {
      assert( s -> action -> player != this );

      active.rage_of_the_sleeper -> target = s -> action -> player;
      // Don't schedule to make sure to respect the set target.
      active.rage_of_the_sleeper -> execute();
    }
  }
}

// druid_t::assess_heal =====================================================

void druid_t::assess_heal( school_e school,
                           dmg_e    dmg_type,
                           action_state_t* s )
{
  if ( sets.has_set_bonus( DRUID_GUARDIAN, T18, B2 ) && buff.ironfur -> check() )
  {
    double pct = sets.set( DRUID_GUARDIAN, T18, B2 ) -> effectN( 1 ).percent();

    // Trigger a gain so we can track how much the set bonus helped.
    // The gain is 100% overflow so it doesn't distort charts.
    gain.guardian_tier18_2pc -> add( RESOURCE_HEALTH, 0, s -> result_total * pct );

    s -> result_total *= 1.0 + pct;
  }

  player_t::assess_heal( school, dmg_type, s );

  trigger_natures_guardian( s );
}

// druid_t::shapeshift ======================================================

void druid_t::shapeshift( form_e f )
{
  if ( get_form() == f )
    return;

  buff.cat_form     -> expire();
  buff.bear_form    -> expire();
  buff.moonkin_form -> expire();

  switch ( f )
  {
  case CAT_FORM:
    buff.cat_form -> trigger();
    break;
  case BEAR_FORM:
    buff.bear_form -> trigger();
    break;
  case MOONKIN_FORM:
    buff.moonkin_form -> trigger();
    break;
  default:
    assert( 0 );
    break;
  }

  form = f;
}

// druid_t::get_target_data =================================================

druid_td_t* druid_t::get_target_data( player_t* target ) const
{
  assert( target );
  druid_td_t*& td = target_data[ target ];
  if ( ! td )
  {
    td = new druid_td_t( *target, const_cast<druid_t&>( *this ) );
  }
  return td;
}

// druid_t::init_beast_weapon ===============================================

void druid_t::init_beast_weapon( weapon_t& w, double swing_time )
{
  w = main_hand_weapon;
  double mod = swing_time /  w.swing_time.total_seconds();
  w.type = WEAPON_BEAST;
  w.school = SCHOOL_PHYSICAL;
  w.min_dmg *= mod;
  w.max_dmg *= mod;
  w.damage *= mod;
  w.swing_time = timespan_t::from_seconds( swing_time );
}

// druid_t::find_affinity_spell =============================================

const spell_data_t* druid_t::find_affinity_spell( const std::string& name )
{
  const spell_data_t* spec_spell = find_specialization_spell( name );

  if ( spec_spell )
  {
    return spec_spell;
  }

  if ( talent.balance_affinity -> ok() )
  {
    if ( util::str_compare_ci( "Starsurge", name             ) ) return find_spell( 197626 );
    if ( util::str_compare_ci( "Lunar Strike", name          ) ) return find_spell( 197628 );
    if ( util::str_compare_ci( "Solar Wrath", name           ) ) return find_spell( 197629 );
    if ( util::str_compare_ci( "Sunfire", name               ) ) return find_spell( 197630 );
  }
  else if ( talent.feral_affinity -> ok() )
  {
    if ( util::str_compare_ci( "Rip", name                   ) ) return find_spell( 1079 );
    if ( util::str_compare_ci( "Rake", name                  ) ) return find_spell( 1822 );
    if ( util::str_compare_ci( "Shred", name                 ) ) return find_spell( 5221 );
    if ( util::str_compare_ci( "Ferocious Bite", name        ) ) return find_spell( 22568 );
    if ( util::str_compare_ci( "Swipe", name                 ) ) return find_spell( 213764 );
  }
  else if ( talent.guardian_affinity -> ok() )
  {
    if ( util::str_compare_ci( "Mangle", name                ) ) return find_spell( 33917 );
    if ( util::str_compare_ci( "Thrash", name                ) ) return find_spell( 106832 );
    if ( util::str_compare_ci( "Ironfur", name               ) ) return find_spell( 192081 );
    if ( util::str_compare_ci( "Frenzied Regeneration", name ) ) return find_spell( 22842 );
  }

  return spell_data_t::not_found();
}

// druid_t::trigger_natures_guardian ========================================

void druid_t::trigger_natures_guardian( const action_state_t* trigger_state )
{
  if ( ! mastery.natures_guardian -> ok() )
    return;
  if ( trigger_state -> result_total <= 0 )
    return;
  if ( trigger_state -> action == active.natures_guardian )
    return;

  active.natures_guardian -> base_dd_min = active.natures_guardian -> base_dd_max =
    trigger_state -> result_total * cache.mastery_value();
  action_state_t* s = active.natures_guardian -> get_state();
  s -> target = this;
  active.natures_guardian -> snapshot_state( s, HEAL_DIRECT );
  active.natures_guardian -> schedule_execute( s );
}

// druid_t::calculate_expected_max_health ===================================

double druid_t::calculate_expected_max_health() const
{
  double slot_weights = 0;
  double prop_values = 0;

  for ( size_t i = 0; i < items.size(); i++ )
  {
    const item_t* item = &items[ i ];
    if ( ! item || item -> slot == SLOT_SHIRT || item -> slot == SLOT_RANGED ||
      item -> slot == SLOT_TABARD || item -> item_level() <= 0 )
    {
      continue;
    }

    const random_prop_data_t item_data = dbc.random_property( item -> item_level() );
    int index = item_database::random_suffix_type( &item -> parsed.data );
    slot_weights += item_data.p_epic[ index ] / item_data.p_epic[ 0 ];

    if ( ! item -> active() )
    {
      continue;
    }

    prop_values += item_data.p_epic[ index ];
  }

  double expected_health = ( prop_values / slot_weights ) * 8.318556;
  expected_health += base.stats.attribute[ STAT_STAMINA ];
  expected_health *= 1.0 + matching_gear_multiplier( ATTR_STAMINA );
  expected_health *= 1.0 + spec.bear_form -> effectN( 2 ).percent();
  expected_health *= 1.0 + artifact.bestial_fortitude.percent();
  expected_health *= current.health_per_stamina;

  return expected_health;
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_target_data_t( &target, &source ),
    dots( dots_t() ),
    buff( buffs_t() ),
    debuff( debuffs_t() )
{
  dots.ashamanes_frenzy = target.get_dot( "ashamanes_frenzy", &source );
  dots.fury_of_elune    = target.get_dot( "fury_of_elune",    &source );
  dots.gushing_wound    = target.get_dot( "gushing_wound",    &source );
  dots.lifebloom        = target.get_dot( "lifebloom",        &source );
  dots.moonfire         = target.get_dot( "moonfire",         &source );
  dots.stellar_flare    = target.get_dot( "stellar_flare",    &source );
  dots.rake             = target.get_dot( "rake",             &source );
  dots.regrowth         = target.get_dot( "regrowth",         &source );
  dots.rejuvenation     = target.get_dot( "rejuvenation",     &source );
  dots.rip              = target.get_dot( "rip",              &source );
  dots.shadow_rake      = target.get_dot( "ashamanes_rake",   &source );
  dots.shadow_rip       = target.get_dot( "ashamanes_rip",    &source );
  dots.shadow_thrash    = target.get_dot( "shadow_thrash",    &source );
  dots.sunfire          = target.get_dot( "sunfire",          &source );
  dots.starfall         = target.get_dot( "starfall",         &source );
  dots.thrash_bear      = target.get_dot( "thrash_bear",      &source );
  dots.thrash_cat       = target.get_dot( "thrash_cat",       &source );
  dots.wild_growth      = target.get_dot( "wild_growth",      &source );

  buff.lifebloom = buff_creator_t( *this, "lifebloom", source.find_class_spell( "Lifebloom" ) );

  debuff.bloodletting        = buff_creator_t( *this, "bloodletting", source.find_spell( 165699 ) )
                               .default_value( source.find_spell( 165699 ) -> effectN( 1 ).percent() );
  debuff.open_wounds         = buff_creator_t( *this, "open_wounds", source.find_spell( 210670 ) )
                               .default_value( source.find_spell( 210670 ) -> effectN( 1 ).percent() )
                               .trigger_spell( source.artifact.open_wounds );
  debuff.stellar_empowerment = buff_creator_t( *this, "stellar_empowerment", source.spec.stellar_empowerment )
                                .default_value( source.spec.stellar_empowerment -> effectN( 1 ).percent() );
}

// Copypasta for reporting
bool has_amount_results( const std::array<stats_t::stats_results_t, RESULT_MAX>& res )
{
  return (
           res[ RESULT_HIT ].actual_amount.mean() > 0 ||
           res[ RESULT_CRIT ].actual_amount.mean() > 0
         );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
public:
  druid_report_t( druid_t& player ) :
    p( player )
  { }

  void feral_snapshot_table( report::sc_html_stream& os )
  {
    // Write header
    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th >Ability</th>\n"
       << "<th colspan=2>Tiger's Fury</th>\n";
    if ( p.talent.bloodtalons -> ok() )
    {
      os << "<th colspan=2>Bloodtalons</th>\n";
    }
    os << "</tr>\n";

    os << "<tr>\n"
       << "<th>Name</th>\n"
       << "<th>Execute %</th>\n"
       << "<th>Benefit %</th>\n";
    if ( p.talent.bloodtalons -> ok() )
    {
      os << "<th>Execute %</th>\n"
         << "<th>Benefit %</th>\n";
    }
    os << "</tr>\n";

    // Compile and Write Contents
    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats = p.stats_list[ i ];
      double tf_exe_up = 0, tf_exe_total = 0;
      double tf_benefit_up = 0, tf_benefit_total = 0;
      double bt_exe_up = 0, bt_exe_total = 0;
      double bt_benefit_up = 0, bt_benefit_total = 0;
      int n = 0;

      for ( size_t j = 0, end2 = stats -> action_list.size(); j < end2; j++ )
      {
        cat_attacks::cat_attack_t* a = dynamic_cast<cat_attacks::cat_attack_t*>( stats -> action_list[ j ] );
        if ( ! a )
          continue;

        if ( ! a -> consumes_bloodtalons )
          continue;

        tf_exe_up += a -> tf_counter -> mean_exe_up();
        tf_exe_total += a -> tf_counter -> mean_exe_total();
        tf_benefit_up += a -> tf_counter -> mean_tick_up();
        tf_benefit_total += a -> tf_counter -> mean_tick_total();
        if ( has_amount_results( stats -> direct_results ) )
        {
          tf_benefit_up += a -> tf_counter -> mean_exe_up();
          tf_benefit_total += a -> tf_counter -> mean_exe_total();
        }
        if ( p.talent.bloodtalons -> ok() )
        {
          bt_exe_up += a -> bt_counter -> mean_exe_up();
          bt_exe_total += a -> bt_counter -> mean_exe_total();
          bt_benefit_up += a -> bt_counter -> mean_tick_up();
          bt_benefit_total += a -> bt_counter -> mean_tick_total();
          if ( has_amount_results( stats -> direct_results ) )
          {
            bt_benefit_up += a -> bt_counter -> mean_exe_up();
            bt_benefit_total += a -> bt_counter -> mean_exe_total();
          }
        }
      }

      if ( tf_exe_total > 0 || bt_exe_total > 0 )
      {
        std::string name_str = report::decorated_action_name( stats -> action_list[ 0 ], stats -> name_str );
        std::string row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
        os.format( "<tr%s><td class=\"left\">%s</td><td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
                   row_class_str.c_str(),
                   name_str.c_str(),
                   util::round( tf_exe_up / tf_exe_total * 100, 2 ),
                   util::round( tf_benefit_up / tf_benefit_total * 100, 2 ) );

        if ( p.talent.bloodtalons -> ok() )
        {
          // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
          os.format( "<td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
                     util::round( bt_exe_up / bt_exe_total * 100, 2 ),
                     util::round( bt_benefit_up / bt_benefit_total * 100, 2 ) );
        }

        os << "</tr>";
      }

    }

    os << "</tr>";

    // Write footer
    os << "</table>\n";
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

// Warlords of Draenor ======================================================

struct wildcat_celerity_t : public scoped_buff_callback_t<buff_t>
{
  wildcat_celerity_t( const std::string& n ) :
    super( DRUID_FERAL, n )
  {}
  
  void manipulate( buff_t* b, const special_effect_t& e ) override
  {
    if ( b -> player -> true_level < 110 )
    {
      b -> buff_duration *= 1.0 + e.driver() -> effectN( 1 ).average( e.item ) / 100.0;
    }
  }
};

struct starshards_callback_t : public scoped_actor_callback_t<druid_t>
{
  starshards_callback_t() : super( DRUID_BALANCE )
  {}

  void manipulate( druid_t* p, const special_effect_t& e ) override
  {
    p -> starshards = e.driver() -> effectN( 1 ).average( e.item ) / 100.0;
    p -> active.starshards = new starshards_t( p );
  }
};


struct stalwart_guardian_callback_t : public scoped_actor_callback_t<druid_t>
{
  stalwart_guardian_callback_t() : super( DRUID_GUARDIAN )
  {}

  static double stalwart_guardian_handler( const action_state_t* s )
  {
    druid_t* p = static_cast<druid_t*>( s -> target );
    assert( p -> active.stalwart_guardian );
    assert( s );

    // Pass incoming damage value so the absorb can be calculated.
    // TOCHECK: Does this use result_amount or result_mitigated?
    p -> active.stalwart_guardian -> incoming_damage = s -> result_mitigated;
    // Pass the triggering enemy so that the damage reflect has a target;
    p -> active.stalwart_guardian -> triggering_enemy = s -> action -> player;
    p -> active.stalwart_guardian -> execute();

    return p -> active.stalwart_guardian -> absorb_size;
  }

  void manipulate( druid_t* p, const special_effect_t& /* e */ ) override
  {
    p -> active.stalwart_guardian = new stalwart_guardian_t( p );

    p -> instant_absorb_list[ 184878 ] =
      new instant_absorb_t( p, p -> find_spell( 184878 ), "stalwart_guardian", &stalwart_guardian_handler );
  }
};

// Legion Artifacts =========================================================

struct fangs_of_ashamane_t : public scoped_actor_callback_t<druid_t>
{
  fangs_of_ashamane_t() : super( DRUID )
  {}

  void manipulate( druid_t* p, const special_effect_t& ) override
  {
    // Fangs of Ashamane act as a 2 handed weapon when in Cat Form.
    unsigned ilevel = p -> items[ SLOT_MAIN_HAND ].item_level();
    double mod = p -> sim -> dbc.item_damage_2h( ilevel ).values[ ITEM_QUALITY_ARTIFACT ]
                / p -> sim -> dbc.item_damage_1h( ilevel ).values[ ITEM_QUALITY_ARTIFACT ];
    p -> cat_weapon.min_dmg *= mod;
    p -> cat_weapon.max_dmg *= mod;
    p -> cat_weapon.damage  *= mod;
  }
};

// Legion Legendaries =======================================================

// General

template<typename T>
struct luffa_wrappings_t : public scoped_action_callback_t<T>
{
  luffa_wrappings_t( const std::string& name ) :
    scoped_action_callback_t<T>( DRUID, name )
  {}

  void manipulate( T* a, const special_effect_t& e ) override
  {
    druid_t* p = debug_cast<druid_t*>( a -> player );

    // Feral Druid passive modifies the strength of the effect.
    a -> radius *= 1.0 + e.driver() -> effectN( 1 ).percent()
      + p -> spec.feral -> effectN( 4 ).percent();
    a -> base_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent()
      + p -> spec.feral -> effectN( 5 ).percent();
  }
};

struct dual_determination_t : public scoped_action_callback_t<survival_instincts_t>
{
  dual_determination_t() : super( DRUID, "survival_instincts" )
  {}

  void manipulate( survival_instincts_t* a, const special_effect_t& e ) override
  {
    a -> cooldown -> charges += e.driver() -> effectN( 1 ).base_value();
    a -> base_recharge_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent();
  }
};

// Feral

struct ailuro_pouncers_t : public scoped_buff_callback_t<buff_t>
{
  ailuro_pouncers_t() : super( DRUID_FERAL, "predatory_swiftness" )
  {}

  void manipulate( buff_t* b, const special_effect_t& e ) override
  {
    b -> set_max_stack( b -> max_stack() + e.driver() -> effectN( 1 ).base_value() );
  
    druid_t* p = debug_cast<druid_t*>( b -> player );
    p -> legendary.ailuro_pouncers = e.driver() -> effectN( 2 ).period();
  }
};

struct chatoyant_signet_t : public scoped_actor_callback_t<druid_t>
{
  chatoyant_signet_t() : super( DRUID )
  {}

  void manipulate( druid_t* p, const special_effect_t& e ) override
  {
    p -> resources.base[ RESOURCE_ENERGY ] += e.driver() -> effectN( 1 ).resource( RESOURCE_ENERGY );
  }
};

struct the_wildshapers_clutch_t : public scoped_actor_callback_t<druid_t>
{
  the_wildshapers_clutch_t() : super( DRUID )
  {}

  void manipulate( druid_t* p, const special_effect_t& e ) override
  {
    p -> legendary.the_wildshapers_clutch = e.driver() -> proc_chance();
  }
};

// Balance

struct impeccable_fel_essence_t : public scoped_actor_callback_t<druid_t>
{
  impeccable_fel_essence_t() : super( DRUID )
  {}

  void manipulate( druid_t* p, const special_effect_t& e ) override
  {
    p -> legendary.impeccable_fel_essence =
      - timespan_t::from_seconds( e.driver() -> effectN( 1 ).base_value() ) / e.driver() -> effectN( 2 ).base_value();
  }
};

struct promise_of_elune_t : public class_buff_cb_t<druid_t>
{
  promise_of_elune_t() : super( DRUID, "power_of_elune_the_moon_goddess" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return actor( e ) -> buff.power_of_elune; }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.driver() -> effectN( 1 ).trigger() );
  }
};

struct oneths_intuition_t : public class_buff_cb_t<druid_t>
{
  oneths_intuition_t() : super( DRUID, "oneths_intuition" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return actor( e ) -> buff.oneths_intuition; }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.player -> find_spell( 209406 ) )
      .chance( e.driver() -> proc_chance() );
  }
};

struct oneths_overconfidence_t : public class_buff_cb_t<druid_t>
{
  oneths_overconfidence_t() : super( DRUID, "oneths_overconfidence" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return actor( e ) -> buff.oneths_overconfidence; }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.player -> find_spell( 209407 ) )
      .chance( e.driver() -> proc_chance() );
  }
};

struct the_emerald_dreamcatcher_t : public class_buff_cb_t<druid_t>
{
  the_emerald_dreamcatcher_t() : super( DRUID, "the_emerald_dreamcatcher" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return actor( e ) -> buff.the_emerald_dreamcatcher; }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.driver() -> effectN( 1 ).trigger() )
      .default_value( e.driver() -> effectN( 1 ).trigger()
        -> effectN( 1 ).resource( RESOURCE_ASTRAL_POWER ) );
  }
};

// Guardian

struct elizes_everlasting_encasement_t : public scoped_action_callback_t<thrash_bear_t>
{
  elizes_everlasting_encasement_t() : super( DRUID, "thrash_bear" )
  {}

  void manipulate( thrash_bear_t* a, const special_effect_t& e ) override
  {
    a -> dot_max_stack += e.driver() -> effectN( 1 ).base_value();
  }
};

struct skysecs_hold_t : public scoped_action_callback_t<frenzied_regeneration_t>
{
  struct skysecs_hold_heal_t : public druid_heal_t
  {
    skysecs_hold_heal_t( const special_effect_t& effect ) :
      druid_heal_t( "skysecs_hold", debug_cast<druid_t*>( effect.player ),
      effect.driver() -> effectN( 1 ).trigger() )
    {
      background = true;
      hasted_ticks = may_crit = tick_may_crit = false;
      target = player;
      tick_pct_heal = data().effectN( 1 ).percent();
    }

    void init() override
    {
      druid_heal_t::init();

      snapshot_flags = update_flags = 0;
    }
  };

  skysecs_hold_t() : super( DRUID, "frenzied_regeneration_driver" )
  {}

  void manipulate( frenzied_regeneration_t* a, const special_effect_t& e ) override
  {
    a -> skysecs_hold = new skysecs_hold_heal_t( e );
  }
};

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

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new druid_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new druid_report_t( *p ) );
    return p;
  }
  virtual bool valid() const override { return true; }
  virtual void init( player_t* p ) const override
  {
    p -> buffs.stampeding_roar = buff_creator_t( p, "stampeding_roar", p -> find_spell( 77764 ) )
                                 .max_stack( 1 )
                                 .duration( timespan_t::from_seconds( 8.0 ) );
  }

  virtual void static_init() const override
  {
    // Warlords of Draenor
    register_special_effect( 184876, starshards_callback_t() );
    register_special_effect( 184877, wildcat_celerity_t( "incarnation_king_of_the_jungle" ) );
    register_special_effect( 184877, wildcat_celerity_t( "berserk" ) );
    register_special_effect( 184877, wildcat_celerity_t( "tigers_fury" ) );
    register_special_effect( 184878, stalwart_guardian_callback_t() );
    // register_special_effect( 184879, flourish_t() );

    // Legion Artifacts
    register_special_effect( 214843, fangs_of_ashamane_t() );

    // Legion Legendaries
    register_special_effect( 208228, dual_determination_t() );
    register_special_effect( 208342, elizes_everlasting_encasement_t() );
    register_special_effect( 208199, impeccable_fel_essence_t() );
    register_special_effect( 208319, the_wildshapers_clutch_t() );
    register_special_effect( 209405, oneths_intuition_t(), true );
    register_special_effect( 209405, oneths_overconfidence_t(), true );
    register_special_effect( 207523, chatoyant_signet_t() );
    register_special_effect( 208209, ailuro_pouncers_t() );
    register_special_effect( 208283, promise_of_elune_t(), true );
    register_special_effect( 208219, skysecs_hold_t() );
    register_special_effect( 208190, the_emerald_dreamcatcher_t(), true );
    register_special_effect( 208681, luffa_wrappings_t<thrash_cat_t>( "thrash_cat" ) );
    register_special_effect( 208681, luffa_wrappings_t<thrash_cat_t::shadow_thrash_t::shadow_thrash_tick_t>( "shadow_thrash" ) );
    register_special_effect( 208681, luffa_wrappings_t<thrash_bear_t>( "thrash_bear" ) );
    // register_special_effect( 208220, amanthuls_wisdom );
    // register_special_effect( 207943, edraith_bonds_of_aglaya );
    // register_special_effect( 210667, ekowraith_creator_of_worlds );
    // register_special_effect( 207932, tearstone_of_elune );
    // register_special_effect( 207271, the_dark_titans_advice );
    // register_special_effect( 208191, essence_of_infusion_t() );
  }

  virtual void register_hotfixes() const override 
  {
    /*
    hotfix::register_effect( "Druid", "2016-09-23", "Sunfire damage increased by 10%.", 232416 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.10 )
      .verification_value( 1.0 );

    hotfix::register_effect( "Druid", "2016-09-23", "Sunfire damage increased by 10%.-dot", 232417 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.10 )
      .verification_value( 0.5 );

    hotfix::register_effect( "Druid", "2016-09-23", "Starfall damage increased by 10%.", 280158 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.10 )
      .verification_value( 0.4 );

    hotfix::register_effect( "Druid", "2016-09-23", "Lunar Strike damage increased by 5%.", 284976 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 2.7 );

    hotfix::register_effect( "Druid", "2016-09-23", "Solar Wrath damage increased by 5%.", 280098 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 1.9 );
      */

  }

  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}

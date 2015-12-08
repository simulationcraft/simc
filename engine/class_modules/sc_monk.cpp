// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
TODO:

Add all buffs
- Crackling Jade Lightning
Change expel harm to heal later on.

GENERAL:
- Fortuitous Sphers - Finish implementing
- Break up Healing Elixers and Fortuitous into two spells; one for proc and one for heal
- Zen Meditation

WINDWALKER:
- Add the cooldown reduction while Serenity is active

MISTWEAVER: Pretty much everything. I have no plans of fixing mistweaver. -alex 8/26/14
Implement the following spells:
- Renewing Mist
- Revival
- Uplift
- Life Cocoon
- Teachings of the Monastery
- Focus and Harmony
* SCK healing
* BoK's cleave effect
- Non-glyphed Mana Tea

BREWMASTER:

- Add some form of cooldown for Expel harm below 35% to better model what is in-game

*/
#include "simulationcraft.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace { // UNNAMED NAMESPACE
// Forward declarations
namespace actions {
namespace spells {
struct stagger_self_damage_t;
}
}
namespace pets {
struct storm_earth_and_fire_pet_t;
}
struct monk_t;

enum combo_strikes_e {
  CS_NONE = -1,
  // Attacks begin here
  CS_TIGER_PALM,
  CS_BLACKOUT_KICK,
  CS_RISING_SUN_KICK,
  CS_FISTS_OF_FURY,
  CS_SPINNING_CRANE_KICK,
  CS_RUSHING_JADE_WIND,
  CS_SPINNING_DRAGON_STRIKE,
  CS_ATTACK_MAX,

  // Spells begin here
  CS_CHI_BURST,
  CS_CHI_WAVE,
  CS_SPELL_MAX,

  // Misc
  CS_SPELL_MIN = CS_CHI_BURST,
  CS_ATTACK_MIN = CS_TIGER_PALM,
  CS_MAX,
};

enum sef_pet_e { SEF_FIRE = 0, SEF_STORM, SEF_EARTH, SEF_PET_MAX };
enum sef_ability_e {
  SEF_NONE = -1,
  // Attacks begin here
  SEF_TIGER_PALM,
  SEF_BLACKOUT_KICK,
  SEF_RISING_SUN_KICK,
  SEF_RISING_SUN_KICK_TRINKET,
  SEF_FISTS_OF_FURY,
  SEF_SPINNING_CRANE_KICK,
  SEF_RUSHING_JADE_WIND,
  SEF_SPINNING_DRAGON_STRIKE,
  SEF_ATTACK_MAX,
  // Attacks end here

  // Spells begin here
  SEF_CHI_BURST,
  SEF_CHI_WAVE,
  SEF_SPELL_MAX,
  // Spells end here

  // Misc
  SEF_SPELL_MIN = SEF_CHI_BURST,
  SEF_ATTACK_MIN = SEF_TIGER_PALM,
  SEF_MAX
};

#define sef_spell_idx( x ) ( ( x ) - SEF_SPELL_MIN )

namespace monk_util
{
// Special Monk Attack Weapon damage collection, if the pointers mh or oh are
// set, instead of the classical action_t::weapon Damage is divided instead of
// multiplied by the weapon speed, AP portion is not multiplied by weapon
// speed.  Both MH and OH are directly weaved into one damage number
double monk_weapon_damage( const action_t* action,
                           const weapon_t* mh,
                           const weapon_t* oh,
                           double weapon_power_mod,
                           double ap )
{
  player_t* player = action -> player;
  sim_t* sim = player -> sim;

  double total_dmg = 0;
  // Main Hand
  if ( mh && mh -> type != WEAPON_NONE && weapon_power_mod > 0 )
  {
    assert( mh -> slot != SLOT_OFF_HAND );
    double dmg = sim -> averaged_range( mh -> min_dmg, mh -> max_dmg ) + mh -> bonus_dmg;
      
    dmg /= mh -> swing_time.total_seconds();
    total_dmg += dmg;

    if ( sim->debug )
    {
      sim -> out_debug.printf( "%s main hand weapon damage portion for %s: td=%.3f min_dmg=%.0f max_dmg=%.0f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
                               player -> name(), action -> name(),
                               total_dmg, mh -> min_dmg, mh -> max_dmg, dmg,
                               mh -> bonus_dmg, mh -> swing_time.total_seconds(), ap );
    }
  }

  // Off Hand
  if ( oh && oh -> type != WEAPON_NONE && weapon_power_mod > 0 && player -> specialization() != MONK_MISTWEAVER )
  {
    assert( oh -> slot == SLOT_OFF_HAND );
    double dmg = sim -> averaged_range( oh -> min_dmg, oh -> max_dmg ) + oh -> bonus_dmg;
      
    dmg /= oh -> swing_time.total_seconds();
    // OH penalty
    dmg *= 0.5;

    total_dmg += dmg;

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s off-hand weapon damage portion for %s: td=%.3f min_dmg=%.0f max_dmg=%.0f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
                               player -> name(), action -> name(), total_dmg, oh -> min_dmg, oh -> max_dmg, dmg, oh -> bonus_dmg, oh -> swing_time.total_seconds(), ap );
    }
  }

  if ( player -> dual_wield() )
    total_dmg *= 0.857143;

  total_dmg += weapon_power_mod * ap;

  return total_dmg;
}
} // Namespace 'monk_util' ends

struct monk_td_t: public actor_target_data_t
{
public:

  struct dots_t
  {
    dot_t* enveloping_mist;
    dot_t* renewing_mist;
    dot_t* soothing_mist;
    dot_t* zen_sphere;
  } dots;

  struct buffs_t
  {
    debuff_t* dizzing_kicks;
    debuff_t* keg_smash;
    debuff_t* storm_earth_and_fire;
  } debuff;

  monk_t& monk;
  monk_td_t( player_t* target, monk_t* p );
};

struct monk_t: public player_t
{
public:
  typedef player_t base_t;

  struct
  {
    luxurious_sample_data_t* stagger_tick_damage;
    luxurious_sample_data_t* stagger_total_damage;
    luxurious_sample_data_t* purified_damage;
    luxurious_sample_data_t* light_stagger_total_damage;
    luxurious_sample_data_t* moderate_stagger_total_damage;
    luxurious_sample_data_t* heavy_stagger_total_damage;
  } sample_datas;

  struct active_actions_t
  {
    action_t* healing_elixir;
    actions::spells::stagger_self_damage_t* stagger_self_damage;
  } active_actions;

  bool track_jade;

  combo_strikes_e previous_combo_strike;

  struct buffs_t
  {
    buff_t* bladed_armor;
    buff_t* channeling_soothing_mist;
    buff_t* chi_orbit;
    buff_t* chi_sphere;
    buff_t* chi_torpedo;
    buff_t* combo_breaker_bok;
    buff_t* cranes_zeal;
    buff_t* dampen_harm;
    buff_t* diffuse_magic;
    buff_t* forceful_winds;
    buff_t* fortifying_brew;
    buff_t* keg_smash_talent;
    buff_t* power_strikes;
    buff_t* rushing_jade_wind;
    buff_t* serenity;
    buff_t* storm_earth_and_fire;
    buff_t* gift_of_the_ox;
    buff_t* tigereye_brew;

    buff_t* zen_meditation;

    // Brewmaster
    buff_t* elusive_brawler;
    buff_t* elusive_dance;
    buff_t* ironskin_brew;

    // Mistweaver
    buff_t* teachings_of_the_monastery;
    buff_t* mana_tea;
    buff_t* mistweaving;
    buff_t* chi_jis_guidance;

    // Windwalker
    buff_t* dizzying_kicks;
    buff_t* swift_as_the_wind;
    buff_t* transfer_the_power;
    buff_t* vital_mists;

    // Legion changes
    buff_t* eye_of_the_tiger;
    buff_t* combo_strikes;
    buff_t* hit_combo;
  } buff;

public:

  struct gains_t
  {
    gain_t* black_ox_brew;
    gain_t* chi_refund;
    gain_t* power_strikes;
    gain_t* combo_breaker_bok;
    gain_t* crackling_jade_lightning;
    gain_t* energy_refund;
    gain_t* energizing_elixir;
    gain_t* keg_smash;
    gain_t* gift_of_the_ox;
    gain_t* mana_tea;
    gain_t* renewing_mist;
    gain_t* serenity;
    gain_t* soothing_mist;
    gain_t* spinning_crane_kick;
    gain_t* rushing_jade_wind;
    gain_t* effuse;
    gain_t* tier15_2pc_melee;
    gain_t* tier16_4pc_melee;
    gain_t* tier16_4pc_tank;
    gain_t* tier17_2pc_healer;
    gain_t* tiger_palm;
    gain_t* healing_elixirs;
    gain_t* fortuitous_spheres;
  } gain;

  struct procs_t
  {
    proc_t* combo_breaker_bok;
    proc_t* eye_of_the_tiger;
    proc_t* mana_tea;
    proc_t* tier15_2pc_melee;
    proc_t* tier15_4pc_melee;
    proc_t* tier17_4pc_heal;
  } proc;

  struct talents_t
  {
    // Tier 15 Talents
    const spell_data_t* chi_burst;
    const spell_data_t* eye_of_the_tiger; // Brewmaster & Windwalker
    const spell_data_t* chi_wave;
    // Mistweaver
    const spell_data_t* zen_pulse;
    const spell_data_t* mana_tea;

    // Tier 30 Talents
    const spell_data_t* chi_torpedo;
    const spell_data_t* tigers_lust;
    const spell_data_t* celerity;

    // Tier 45 Talents
    // Brewmaster
    const spell_data_t* light_brewing;
    const spell_data_t* black_ox_brew;
    const spell_data_t* secret_ingredients;
    // Windwalker
    const spell_data_t* energizing_elixir;
    const spell_data_t* ascension;
    const spell_data_t* power_strikes;
    // Mistweaver
    const spell_data_t* mist_wrap;
    const spell_data_t* cranes_grace;
    const spell_data_t* lifecycles;

    // Tier 60 Talents
    const spell_data_t* ring_of_peace;
    const spell_data_t* summon_black_ox_statue; // Brewmaster
    const spell_data_t* dizzying_kicks; // Windwalker
    const spell_data_t* song_of_chi_ji; // Mistweaver
    const spell_data_t* leg_sweep;

    // Tier 75 Talents
    const spell_data_t* healing_elixirs;
    const spell_data_t* dampen_harm;
    const spell_data_t* diffuse_magic;

    // Tier 90 Talents
    const spell_data_t* rushing_jade_wind; // Brewmaster & Windwalker
    // Brewmaster
    const spell_data_t* invoke_niuzao;
    const spell_data_t* special_delivery;
    // Windwalker
    const spell_data_t* invoke_xuen;
    const spell_data_t* hit_combo;
    // Mistweaver
    const spell_data_t* refreshing_jade_wind;
    const spell_data_t* invoke_chi_ji;
    const spell_data_t* summon_jade_serpent_statue;

    // Tier 100 Talents
    // Brewmaster
    const spell_data_t* elusive_dance;
    const spell_data_t* fortified_mind;
    const spell_data_t* high_tolerance;
    // Windwalker
    const spell_data_t* chi_orbit;
    const spell_data_t* spinning_dragon_strike;
    const spell_data_t* serenity;
    // Mistweaver
    const spell_data_t* mistwalk;
    const spell_data_t* focused_thunder;
    const spell_data_t* soothing_elegance;
  } talent;

  // Specialization
  struct specs_t
  {
    // GENERAL
    const spell_data_t* blackout_kick;
    const spell_data_t* crackling_jade_lightning;
    const spell_data_t* critical_strikes;
    const spell_data_t* healing_sphere;
    const spell_data_t* effuse;
    const spell_data_t* leather_specialization;
    const spell_data_t* legacy_of_the_white_tiger;
    const spell_data_t* legacy_of_the_emperor;
    const spell_data_t* rising_sun_kick;
    const spell_data_t* spinning_crane_kick;
    const spell_data_t* tiger_palm;
    const spell_data_t* touch_of_death;
//    const spell_data_t* way_of_the_monk_aa_damage;
//    const spell_data_t* way_of_the_monk_aa_speed;
    const spell_data_t* zen_meditaiton;
    const spell_data_t* fortifying_brew;

    // Brewmaster
    const spell_data_t* blackout_strike;
    const spell_data_t* bladed_armor;
    const spell_data_t* breath_of_fire;
    const spell_data_t* desperate_measures;
    const spell_data_t* dizzying_haze;
    const spell_data_t* ferment;
    const spell_data_t* gift_of_the_ox;
    const spell_data_t* ironskin_brew;
    const spell_data_t* keg_smash;
    const spell_data_t* provoke;
    const spell_data_t* purifying_brew;
    const spell_data_t* resolve;
    const spell_data_t* summon_black_ox_statue;
    const spell_data_t* stagger;
    const spell_data_t* light_stagger;
    const spell_data_t* moderate_stagger;
    const spell_data_t* heavy_stagger;

    // Mistweaver
    const spell_data_t* brewing_mana_tea;
    const spell_data_t* crane_style_techniques;
    const spell_data_t* detonate_chi;
    const spell_data_t* enveloping_mist;
    const spell_data_t* jade_mists;
    const spell_data_t* life_cocoon;
    const spell_data_t* mana_tea;
    const spell_data_t* mana_tea_driver;
    const spell_data_t* renewing_mist;
    const spell_data_t* renewing_mist_heal;
    const spell_data_t* revival;
    const spell_data_t* soothing_mist;
    const spell_data_t* soothing_mists_trigger;
    const spell_data_t* soothing_mist_statue;
    const spell_data_t* summon_jade_serpent_statue;
    const spell_data_t* teachings_of_the_monastery;
    const spell_data_t* teachings_of_the_monastery_buff;
    const spell_data_t* thunder_focus_tea;
    const spell_data_t* uplift;
    const spell_data_t* eminence;
    const spell_data_t* eminence_statue;
    const spell_data_t* breath_of_the_serpent_heal;
    const spell_data_t* extend_life;

    // Windwalker
    const spell_data_t* afterlife;
    const spell_data_t* battle_trance;
    const spell_data_t* combat_conditioning;
    const spell_data_t* combo_breaker;
    const spell_data_t* disable;
    const spell_data_t* fists_of_fury;
    const spell_data_t* flying_serpent_kick;
    const spell_data_t* rising_sun_kick_trinket;
    const spell_data_t* stance_of_the_fierce_tiger;
    const spell_data_t* storm_earth_and_fire;
    const spell_data_t* tigereye_brew;
    const spell_data_t* touch_of_karma;
    const spell_data_t* windflurry;
  } spec;

  // Artifact
  struct artifact_spell_data_t
  {
    // Windwalker Artifact
    artifact_power_t acrobatics;
    artifact_power_t crosswinds;
    artifact_power_t dark_skies;
    artifact_power_t death_art;
    artifact_power_t fists_of_the_wind;
    artifact_power_t gale_burst;
    artifact_power_t good_karma;
    artifact_power_t healing_winds;
    artifact_power_t inner_peace;
    artifact_power_t light_on_your_feet;
    artifact_power_t power_of_a_thousand_cranes;
    artifact_power_t rising_winds;
    artifact_power_t strike_of_the_skylord;
    artifact_power_t strength_of_xuen;
    artifact_power_t transfer_the_power;
    artifact_power_t tornado_kicks;
  } artifact;

  struct mastery_spells_t
  {
    const spell_data_t* combo_strikes;       // Windwalker
    const spell_data_t* elusive_brawler;     // Brewmaster
    const spell_data_t* gust_of_mists; // Mistweaver
  } mastery;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* brewmaster_attack;
    cooldown_t* brewmaster_active_mitigation;
    cooldown_t* desperate_measure;
    cooldown_t* fists_of_fury;
    cooldown_t* fortifying_brew;
    cooldown_t* healing_elixirs;
    cooldown_t* healing_sphere;
    cooldown_t* rising_sun_kick;
    cooldown_t* touch_of_death;
  } cooldown;

  struct passives_t
  {
    // General
    const spell_data_t* chi_burst_damage;
    const spell_data_t* chi_burst_heal;
    const spell_data_t* chi_torpedo;
    const spell_data_t* chi_wave_damage;
    const spell_data_t* chi_wave_heal;
    const spell_data_t* eye_of_the_tiger;
    const spell_data_t* healing_elixirs;
    const spell_data_t* rushing_jade_wind_damage;
    const spell_data_t* spinning_crane_kick_damage;
    // Brewmaster
    const spell_data_t* aura_brewmaster_monk;
    const spell_data_t* breath_of_fire_dot;
    const spell_data_t* elusive_brawler;
    const spell_data_t* elusive_dance;
    const spell_data_t* gift_of_the_ox_chance;
    const spell_data_t* gift_of_the_ox_heal;
    const spell_data_t* gift_of_the_ox_summon;
    const spell_data_t* keg_smash_buff;
    const spell_data_t* special_delivery;
    const spell_data_t* stagger_self_damage;
    const spell_data_t* tier17_2pc_tank;
    // Mistweaver
    const spell_data_t* aura_mistweaver_monk;
    const spell_data_t* tier17_2pc_heal;
    const spell_data_t* tier17_4pc_heal;
    // Windwalker
    const spell_data_t* aura_windwalker_monk;
    const spell_data_t* chi_orbit;
    const spell_data_t* chi_sphere;
    const spell_data_t* crackling_tiger_lightning;
    const spell_data_t* crackling_tiger_lightning_driver;
    const spell_data_t* dizzying_kicks;
    const spell_data_t* hit_combo;
    const spell_data_t* spinning_dragon_strike;
    const spell_data_t* touch_of_karma_tick;
    const spell_data_t* tier15_2pc_melee;
    const spell_data_t* tier17_4pc_melee;

  } passives;

  struct pets_t
  {
    pets::storm_earth_and_fire_pet_t* sef[ SEF_PET_MAX ];
  } pet;

  // Options
  struct options_t
  {
    int initial_chi;
    double goto_throttle;
    double ppm_below_35_percent_dm;
    double ppm_below_50_percent_dm;
  } user_options;

private:
  target_specific_t<monk_td_t> target_data;
public:

  monk_t( sim_t* sim, const std::string& name, race_e r ):
    player_t( sim, MONK, name, r ),
    active_actions( active_actions_t() ),
    buff( buffs_t() ),
    gain( gains_t() ),
    proc( procs_t() ),
    talent( talents_t() ),
    spec( specs_t() ),
    mastery( mastery_spells_t() ),
    cooldown( cooldowns_t() ),
    passives( passives_t() ),
    pet( pets_t() ),
    user_options( options_t() ),
    light_stagger_threshold( 0 ),
    moderate_stagger_threshold( 0.035 ),
    heavy_stagger_threshold( 0.065 ),
    eluding_movements( nullptr ),
    soothing_breeze( nullptr ),
    furious_sun( nullptr ),
    aburaq( nullptr )
  {
    // actives
    cooldown.brewmaster_attack            = get_cooldown( "brewmaster_attack" );
    cooldown.brewmaster_active_mitigation = get_cooldown( "brews" );
    cooldown.fortifying_brew              = get_cooldown( "fortifying_brew" );
    cooldown.fists_of_fury                = get_cooldown( "fists_of_fury" );
    cooldown.healing_elixirs              = get_cooldown( "healing_elixirs" );
    cooldown.healing_sphere               = get_cooldown( "healing_sphere" );
    cooldown.rising_sun_kick              = get_cooldown( "rising_sun_kick" );
    cooldown.touch_of_death               = get_cooldown( "touch_of_death" );

    regen_type = REGEN_DYNAMIC;
    if ( specialization() != MONK_MISTWEAVER )
    {
      regen_caches[CACHE_HASTE] = true;
      regen_caches[CACHE_ATTACK_HASTE] = true;
    }
    track_jade = false;
    previous_combo_strike = CS_NONE;
    user_options.initial_chi = 0;
    user_options.goto_throttle = 0;
    user_options.ppm_below_35_percent_dm = 0;
    user_options.ppm_below_50_percent_dm = 0;
  }

  // player_t overrides
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_melee_crit_multiplier() const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_spell_crit_multiplier() const override;
  virtual double    energy_regen_per_second() const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_heal_multiplier( const action_state_t* s ) const override;
  virtual double    composite_melee_expertise( const weapon_t* weapon ) const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_parry() const override;
  virtual double    composite_dodge() const override;
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      regen( timespan_t periodicity ) override;
  virtual void      reset() override;
  virtual void      interrupt() override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      create_options() override;
  virtual void      copy_from( player_t* ) override;
  virtual resource_e primary_resource() const override;
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual void      pre_analyze_hook() override;
  virtual void      combat_begin() override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage( school_e, dmg_e, action_state_t* s ) override;
  virtual void      assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* s ) override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual void      init_action_list() override;
  virtual bool      has_t18_class_trinket() const override;
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str ) override;
  virtual monk_td_t* get_target_data( player_t* target ) const override
  {
    monk_td_t*& td = target_data[target];
    if ( !td )
    {
      td = new monk_td_t( target, const_cast<monk_t*>( this ) );
    }
    return td;
  }

  // Monk specific
  void apl_combat_brewmaster();
  void apl_combat_mistweaver();
  void apl_combat_windwalker();
  void apl_pre_brewmaster();
  void apl_pre_mistweaver();
  void apl_pre_windwalker();

  // Custom Monk Functions
  double current_stagger_tick_dmg();
  double current_stagger_tick_dmg_percent();
  double current_stagger_dot_remains();
  double stagger_pct();
  // Blizzard rounds it's stagger damage; anything higher than half a percent beyond 
  // the threshold will switch to the next threshold
  const double light_stagger_threshold;
  const double moderate_stagger_threshold;
  const double heavy_stagger_threshold;
//  combo_strikes_e convert_expression_action_to_enum( action_t* a );
  double weapon_power_mod;
  double clear_stagger();
  bool has_stagger();

  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* eluding_movements;
  const special_effect_t* soothing_breeze;
  const special_effect_t* furious_sun;

  // Legion Artifact effects
  const special_effect_t* aburaq;
};

// ==========================================================================
// Monk Pets & Statues
// ==========================================================================

namespace pets {
struct statue_t: public pet_t
{
  statue_t( sim_t* sim, monk_t* owner, const std::string& n, pet_e pt, bool guardian = false ):
    pet_t( sim, owner, n, pt, guardian )
  { }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }
};

struct jade_serpent_statue_t: public statue_t
{
  typedef statue_t base_t;
  jade_serpent_statue_t( sim_t* sim, monk_t* owner, const std::string& n ):
    base_t( sim, owner, n, PET_NONE, true )
  { }
};

// ==========================================================================
// Storm Earth and Fire
// ==========================================================================

struct storm_earth_and_fire_pet_t : public pet_t
{
  struct sef_td_t: public actor_target_data_t
  {

    sef_td_t( player_t* target, storm_earth_and_fire_pet_t* source ) :
      actor_target_data_t( target, source )
    { }
  };

  // Storm, Earth, and Fire abilities begin =================================

  template <typename BASE>
  struct sef_action_base_t : public BASE
  {
    typedef BASE super_t;
    typedef sef_action_base_t<BASE> base_t;

    const action_t* source_action;

    sef_action_base_t( const std::string& n,
                       storm_earth_and_fire_pet_t* p,
                       const spell_data_t* data = spell_data_t::nil() ) :
      BASE( n, p, data ), source_action( nullptr )
    {
      // Make SEF attacks always background, so they do not consume resources
      // or do anything associated with "foreground actions".
      this -> background = this -> may_crit = true;
      this -> callbacks = false;

      // Cooldowns are handled automatically by the mirror abilities, the SEF specific ones need none.
      this -> cooldown -> duration = timespan_t::zero();

      // No costs are needed either
      this -> base_costs[ RESOURCE_ENERGY ] = 0;
      this -> base_costs[ RESOURCE_CHI ] = 0;
    }

    void init()
    {
      super_t::init();

      // Find source_action from the owner by matching the action name and
      // spell id with eachother. This basically means that by default, any
      // spell-data driven ability with 1:1 mapping of name/spell id will
      // always be chosen as the source action. In some cases this needs to be
      // overridden (see sef_zen_sphere_t for example).
      for ( size_t i = 0, end = o() -> action_list.size(); i < end; i++ )
      {
        action_t* a = o() -> action_list[ i ];

        if ( util::str_compare_ci( this -> name_str, a -> name_str ) && this -> id == a -> id )
        {
          source_action = a;
          break;
        }
      }

      if ( source_action )
      {
        this -> update_flags = source_action -> update_flags;
        this -> snapshot_flags = source_action -> snapshot_flags;
      }
    }

    sef_td_t* td( player_t* t ) const
    { return this -> p() -> get_target_data( t ); }

    monk_t* o()
    { return debug_cast<monk_t*>( this -> player -> cast_pet() -> owner ); }

    const monk_t* o() const
    { return debug_cast<const monk_t*>( this -> player -> cast_pet() -> owner ); }

    const storm_earth_and_fire_pet_t* p() const
    { return debug_cast<storm_earth_and_fire_pet_t*>( this -> player ); }

    storm_earth_and_fire_pet_t* p()
    { return debug_cast<storm_earth_and_fire_pet_t*>( this -> player ); }

    // Use SEF-specific override methods for target related multipliers as the
    // pets seem to have their own functionality relating to it. The rest of
    // the state-related stuff is actually mapped to the source (owner) action
    // below.

    double composite_target_multiplier( player_t* t ) const
    {
      double m = super_t::composite_target_multiplier( t );

      const sef_td_t* tdata = td( t );

      return m;
    }

    // Map the rest of the relevant state-related stuff into the source
    // action's methods. In other words, use the owner's data. Note that attack
    // power is not included here, as we will want to (just in case) snapshot
    // AP through the pet's own AP system. This allows us to override the
    // inheritance coefficient if need be in an easy way.

    double attack_direct_power_coefficient( const action_state_t* state ) const
    {
      return source_action -> attack_direct_power_coefficient( state );
    }

    double attack_tick_power_coefficient( const action_state_t* state ) const
    {
      return source_action -> attack_tick_power_coefficient( state );
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const
    {
      return source_action -> composite_dot_duration( s );
    }

    timespan_t tick_time( double haste ) const
    {
      return source_action -> tick_time( haste );
    }

    double composite_da_multiplier( const action_state_t* s ) const
    {
      return source_action -> composite_da_multiplier( s );
    }

    double composite_ta_multiplier( const action_state_t* s ) const
    {
      return source_action -> composite_ta_multiplier( s );
    }

    double composite_persistent_multiplier( const action_state_t* s ) const
    {
      return source_action -> composite_persistent_multiplier( s );
    }

    double composite_versatility( const action_state_t* s ) const
    {
      return source_action -> composite_versatility( s );
    }

    double composite_haste() const
    {
      return source_action -> composite_haste();
    }

    timespan_t travel_time() const
    {
      return source_action -> travel_time();
    }

    int n_targets() const
    { return source_action ? source_action -> n_targets() : super_t::n_targets(); }

    void schedule_execute( action_state_t* state = nullptr )
    {
      // Never execute an ability if there's no source action. Things will crash.
      if ( ! source_action )
      {
        action_state_t::release( state );
        return;
      }

      // Target always follows the SEF clone's target, which is assigned during
      // summon time
      this -> target = this -> player -> target;

      super_t::schedule_execute( state );
    }

    void snapshot_internal( action_state_t* state, uint32_t flags, dmg_e rt )
    {
      super_t::snapshot_internal( state, flags, rt );

      // TODO: Check-recheck-figure out some day
      // Apply a -5% modifier to all damage generated by the pets.
      state -> da_multiplier /= 1.05;
      state -> ta_multiplier /= 1.05;
    }
  };

  struct sef_melee_attack_t : public sef_action_base_t<melee_attack_t>
  {
    bool main_hand, off_hand;

    sef_melee_attack_t( const std::string& n,
                        storm_earth_and_fire_pet_t* p,
                        const spell_data_t* data = spell_data_t::nil(),
                        weapon_t* w = nullptr ) :
      base_t( n, p, data ),
      // For special attacks, the SEF pets always use the owner's weapons.
      main_hand( ! w ? true : false ), off_hand( ! w ? true : false )
    {
      school = SCHOOL_PHYSICAL;

      if ( w )
      {
        weapon = w;
      }
    }

    // SEF uses the "normal" monk weapon damage calculation, except for auto
    // attacks.
    double calculate_weapon_damage( double attack_power ) const override
    {
      // Actual weapon damage calculation is done with the OWNER weapons for
      // special attacks, not SEF specific ones.
//      if ( main_hand || ( main_hand && off_hand ) )
//        return monk_util::monk_weapon_damage( this, &( o() -> main_hand_weapon ), &( o() -> off_hand_weapon ), weapon_power_mod, attack_power );
//      else
        return base_t::calculate_weapon_damage( attack_power );
    }

    // Physical tick_action abilities need amount_type() override, so the
    // tick_action multistrikes are properly physically mitigated.
    dmg_e amount_type( const action_state_t* state, bool periodic ) const override
    {
      if ( tick_action && tick_action -> school == SCHOOL_PHYSICAL )
      {
        return DMG_DIRECT;
      }
      else
      {
        return base_t::amount_type( state, periodic );
      }
    }
  };

  struct sef_spell_t : public sef_action_base_t<spell_t>
  {
    sef_spell_t( const std::string& n,
                 storm_earth_and_fire_pet_t* p,
                 const spell_data_t* data = spell_data_t::nil() ) :
      base_t( n, p, data )
    { }
  };

  // Auto attack ============================================================

  struct melee_t: public sef_melee_attack_t
  {
    melee_t( const std::string& n, storm_earth_and_fire_pet_t* player, weapon_t* w ):
      sef_melee_attack_t( n, player, spell_data_t::nil(), w )
    {
      background = repeating = may_crit = may_glance = true;

      base_execute_time = w -> swing_time;
      trigger_gcd = timespan_t::zero();
      special = false;
      auto_attack = true;

      if ( player -> dual_wield() )
      {
        base_hit -= 0.19;
      }

      if ( w == &( player -> main_hand_weapon ) )
      {
        source_action = player -> owner -> find_action( "melee_main_hand" );
      }
      else
      {
        source_action = player -> owner -> find_action( "melee_off_hand" );
        // If owner is using a 2handed weapon, there's not going to be an
        // off-hand action for autoattacks, thus just use main hand one then.
        if ( ! source_action )
        {
          source_action = player -> owner -> find_action( "melee_main_hand" );
        }
      }

      // TODO: Can't really assert here, need to figure out a fallback if the
      // windwalker does not use autoattacks (how likely is that?)
      if ( sim -> debug )
      {
        sim -> errorf( "%s has no auto_attack in APL, Storm, Earth, and Fire pets cannot auto-attack.",
            o() -> name() );
      }
    }

    // A wild equation appears
    double composite_attack_power() const override
    {
      double ap = sef_melee_attack_t::composite_attack_power();

      if ( o() -> main_hand_weapon.group() == WEAPON_2H )
      {
        ap += o() -> main_hand_weapon.dps * 3.5;
      }
      // 1h/dual wield equation. Note, this formula is slightly off (~3%) for
      // owner dw/pet dw variation.
      else
      {
        double total_dps = o() -> main_hand_weapon.dps;
        double dw_mul = 1.0;
        if ( o() -> off_hand_weapon.group() != WEAPON_NONE )
        {
          total_dps += o() -> off_hand_weapon.dps * 0.5;
          dw_mul = 0.898882275;
        }

        ap += total_dps * 3.5 * dw_mul;
      }

      return ap;
    }

    // Since we use owner multipliers, we need to apply (or remove!) the auto
    // attack Way of the Monk multiplier from the AA damage.
    void snapshot_internal( action_state_t* state, uint32_t flags, dmg_e rt ) override
    {
      sef_melee_attack_t::snapshot_internal( state, flags, rt );

/*
      if ( ! o() -> dual_wield() && player -> dual_wield() )
      {
        state -> da_multiplier *= 1.0 + o() -> spec.way_of_the_monk_aa_damage -> effectN( 1 ).percent();
      }
      else if ( o() -> dual_wield() && ! player -> dual_wield() )
      {
        state -> da_multiplier /= 1.0 + o() -> spec.way_of_the_monk_aa_damage -> effectN( 1 ).percent();
      }
*/
    }

    virtual timespan_t execute_time() const override
    {
      timespan_t t = sef_melee_attack_t::execute_time();

//      if ( ! player -> dual_wield() )
//        t *= 1.0 / ( 1.0 + o() -> spec.way_of_the_monk_aa_speed -> effectN( 1 ).percent() );

      return t;
    }

    void execute() override
    {
      if ( time_to_execute > timespan_t::zero() && player -> executing )
      {
        if ( sim -> debug )
        {
          sim -> out_debug.printf( "%s Executing '%s' during melee (%s).",
              player -> name(),
              player -> executing -> name(),
              util::slot_type_string( weapon -> slot ) );
        }

        schedule_execute();
      }
      else
        sef_melee_attack_t::execute();
    }
  };

  struct auto_attack_t: public attack_t
  {
    auto_attack_t( storm_earth_and_fire_pet_t* player, const std::string& options_str ):
      attack_t( "auto_attack", player, spell_data_t::nil() )
    {
      parse_options( options_str );

      trigger_gcd = timespan_t::zero();

      melee_t* mh = new melee_t( "auto_attack_mh", player, &( player -> main_hand_weapon ) );
      if ( ! mh -> source_action )
      {
        background = true;
        return;
      }
      player -> main_hand_attack = mh;

      if ( player -> dual_wield() )
      {
        player -> off_hand_attack = new melee_t( "auto_attack_oh", player, &( player -> off_hand_weapon ) );
      }
    }

    virtual void execute() override
    {
      player -> main_hand_attack -> schedule_execute();

      if ( player -> off_hand_attack )
        player -> off_hand_attack -> schedule_execute();
    }

    virtual bool ready() override
    {
      if ( player -> is_moving() ) return false;

      return ( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
    }
  };

  // Special attacks ========================================================
  //
  // Note, these automatically use the owner's multipliers, so there's no need
  // to adjust anything here.

  struct sef_tiger_palm_t : public sef_melee_attack_t
  {
    sef_tiger_palm_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "tiger_palm", player, player -> o() -> spec.tiger_palm )
    { }
  };

  struct sef_blackout_kick_t : public sef_melee_attack_t
  {

    sef_blackout_kick_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "blackout_kick", player, player -> o() -> spec.blackout_kick )
    { }
  };

  struct sef_rising_sun_kick_t : public sef_melee_attack_t
  {

    sef_rising_sun_kick_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "rising_sun_kick", player, player -> o() -> spec.rising_sun_kick )
    { }
  };

  struct sef_rising_sun_kick_trinket_t : public sef_melee_attack_t
  {

    sef_rising_sun_kick_trinket_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "rising_sun_kick_trinket", player, player -> o() -> spec.rising_sun_kick_trinket )
    {
      player -> find_action( "rising_sun_kick" ) -> add_child( this );
    }
  };

  struct sef_tick_action_t : public sef_melee_attack_t
  {
    sef_tick_action_t( const std::string& name, storm_earth_and_fire_pet_t* p, const spell_data_t* data ) :
      sef_melee_attack_t( name, p, data )
    {
      aoe = -1;

      // Reset some variables to ensure proper execution
      dot_duration = timespan_t::zero();
      school = SCHOOL_PHYSICAL;
    }
  };

  struct sef_fists_of_fury_tick_t: public sef_tick_action_t
  {
    sef_fists_of_fury_tick_t( storm_earth_and_fire_pet_t* p ):
      sef_tick_action_t( "fists_of_fury_tick", p, p -> o() -> spec.fists_of_fury -> effectN( 3 ).trigger())
    { }

    // Damage must be divided on non-main target by the number of targets
    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      if ( state -> target != target )
      {
        return 1.0 / state -> n_targets;
      }

      return 1.0;
    }
  };

  struct sef_fists_of_fury_t : public sef_melee_attack_t
  {
    sef_fists_of_fury_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "fists_of_fury", player, player -> o() -> spec.fists_of_fury )
    {
      channeled = tick_zero = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_fists_of_fury_tick_t( player );
    }
  };

  struct sef_spinning_crane_kick_t : public sef_melee_attack_t
  {
    sef_spinning_crane_kick_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "spinning_crane_kick", player, player -> o() -> spec.spinning_crane_kick )
    {
      channeled = tick_zero = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_tick_action_t( "spinning_crane_kick_tick", player, &( data() ) );
    }
  };

  struct sef_rushing_jade_wind_t : public sef_melee_attack_t
  {
    sef_rushing_jade_wind_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "rushing_jade_wind", player, player -> o() -> talent.refreshing_jade_wind )
    {
      tick_zero = hasted_ticks = true;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_tick_action_t( "rushing_jade_wind_tick", player, &( data() ) );
    }
  };

  struct sef_spinning_dragon_strike_tick_t : public sef_tick_action_t
  {
    sef_spinning_dragon_strike_tick_t( storm_earth_and_fire_pet_t* p ):
      sef_tick_action_t( "spinning_dragon_strike_tick", p, p -> o() -> passives.spinning_dragon_strike )
    {
      aoe = -1;
    }
  };

  struct sef_spinning_dragon_strike_t : public sef_melee_attack_t
  {
    sef_spinning_dragon_strike_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "spinning_dragon_strike", player, player -> o() -> talent.spinning_dragon_strike )
    {
      channeled = true;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_spinning_dragon_strike_tick_t( player );
    }
  };

  struct sef_chi_wave_damage_t : public sef_spell_t
  {
    sef_chi_wave_damage_t( storm_earth_and_fire_pet_t* player ) :
      sef_spell_t( "chi_wave_damage", player, player -> o() -> passives.chi_wave_damage )
    {
      dual = true;
    }
  };

  // SEF Chi Wave skips the healing ticks, delivering damage on every second
  // tick of the ability for simplicity.
  struct sef_chi_wave_t : public sef_spell_t
  {
    sef_chi_wave_damage_t* wave;

    sef_chi_wave_t( storm_earth_and_fire_pet_t* player ) :
      sef_spell_t( "chi_wave", player, player -> o() -> talent.chi_wave ),
      wave( new sef_chi_wave_damage_t( player ) )
    {
      may_crit = may_miss = hasted_ticks = false;
      tick_zero = tick_may_crit = true;
    }

    void tick( dot_t* d ) override
    {
      if ( d -> current_tick % 2 == 0 )
      {
        sef_spell_t::tick( d );
        wave -> target = d -> target;
        wave -> schedule_execute();
      }
    }
  };

  struct sef_chi_burst_t : public sef_spell_t
  {
    sef_chi_burst_t( storm_earth_and_fire_pet_t* player ) :
      sef_spell_t( "chi_burst", player, player -> o() -> passives.chi_burst_damage )
    {
      interrupt_auto_attack = false;
    }
  };

  // Storm, Earth, and Fire abilities end ===================================

  std::vector<sef_melee_attack_t*> attacks;
  std::vector<sef_spell_t*> spells;

private:
  target_specific_t<sef_td_t> target_data;
public:

  storm_earth_and_fire_pet_t( const std::string& name, sim_t* sim, monk_t* owner, bool dual_wield ):
    pet_t( sim, owner, name, true ),
    attacks( SEF_ATTACK_MAX ), spells( SEF_SPELL_MAX - SEF_SPELL_MIN )
  {
    // Storm, Earth, and Fire pets have to become "Windwalkers", so we can get
    // around some sanity checks in the action execution code, that prevents
    // abilities meant for a certain specialization to be executed by actors
    // that do not have the specialization.
    _spec = MONK_WINDWALKER;

    main_hand_weapon.type = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( dual_wield ? 2.6 : 3.6 );

    if ( dual_wield )
    {
      off_hand_weapon.type = WEAPON_BEAST;
      off_hand_weapon.swing_time = timespan_t::from_seconds( 2.6 );
    }

    owner_coeff.ap_from_ap = 1.0;
  }

  timespan_t available() const override
  { return sim -> expected_iteration_time * 2; }

  monk_t* o()
  {
    return debug_cast<monk_t*>( owner );
  }

  const monk_t* o() const
  {
    return debug_cast<const monk_t*>( owner );
  }

  sef_td_t* get_target_data( player_t* target ) const override
  {
    sef_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new sef_td_t( target, const_cast< storm_earth_and_fire_pet_t*>( this ) );
    }
    return td;
  }

  void init_spells() override
  {
    pet_t::init_spells();

    attacks[ SEF_TIGER_PALM              ] = new sef_tiger_palm_t( this );
    attacks[ SEF_BLACKOUT_KICK           ] = new sef_blackout_kick_t( this );
    attacks[ SEF_RISING_SUN_KICK         ] = new sef_rising_sun_kick_t( this );
    attacks[ SEF_RISING_SUN_KICK_TRINKET ] = new sef_rising_sun_kick_trinket_t( this );
    attacks[ SEF_FISTS_OF_FURY           ] = new sef_fists_of_fury_t( this );
    attacks[ SEF_SPINNING_CRANE_KICK     ] = new sef_spinning_crane_kick_t( this );
    attacks[ SEF_RUSHING_JADE_WIND       ] = new sef_rushing_jade_wind_t( this );
    attacks[ SEF_SPINNING_DRAGON_STRIKE  ] = new sef_spinning_dragon_strike_t( this );

    spells[ sef_spell_idx( SEF_CHI_BURST )  ] = new sef_chi_burst_t( this );
    spells[ sef_spell_idx( SEF_CHI_WAVE )   ] = new sef_chi_wave_t( this );
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    pet_t::summon( duration );

    o() -> buff.storm_earth_and_fire -> trigger();
    // Note, storm_earth_fire_t (the summoning spell) has already set the
    // correct target
    monk_td_t* owner_td = o() -> get_target_data( target );
    assert( owner_td -> debuff.storm_earth_and_fire -> check() == 0 );
    owner_td -> debuff.storm_earth_and_fire -> trigger();
  }

  void dismiss( bool expired = false ) override
  {
    pet_t::dismiss( expired );

    if ( ! target -> is_sleeping() )
    {
      monk_td_t* owner_td = o() -> get_target_data( target );
      assert( owner_td -> debuff.storm_earth_and_fire -> check() == 1 );
      owner_td -> debuff.storm_earth_and_fire -> expire();
    }

    o() -> buff.storm_earth_and_fire -> decrement();
  }

  void trigger_attack( sef_ability_e ability, const action_t* source_action )
  {
    if ( ability >= SEF_SPELL_MIN )
    {
      size_t spell = static_cast<size_t>( ability - SEF_SPELL_MIN );
      assert( spells[ spell ] );

      // If the owner targeted this pet's target, don't mirror the ability
      if ( source_action -> target != target )
      {
        spells[ spell ] -> source_action = source_action;
        spells[ spell ] -> schedule_execute();
      }
    }
    else
    {
      assert( attacks[ ability ] );

      if ( source_action -> target != target )
      {
        attacks[ ability ] -> source_action = source_action;
        attacks[ ability ] -> schedule_execute();
      }
    }
  }
};


// ==========================================================================
// Xuen Pet
// ==========================================================================
struct xuen_pet_t: public pet_t
{
private:
  struct melee_t: public melee_attack_t
  {
    melee_t( const std::string& n, xuen_pet_t* player ):
      melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = repeating = may_crit = may_glance = true;
      school = SCHOOL_NATURE;

      // Use damage numbers from the level-scaled weapon
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      trigger_gcd = timespan_t::zero();
      special = false;
    }

    void execute() override
    {
      if ( time_to_execute > timespan_t::zero() && player -> executing )
      {
        if ( sim -> debug )
          sim -> out_debug.printf( "Executing '%s' during melee (%s).", player -> executing -> name(), util::slot_type_string( weapon -> slot ) );
        schedule_execute();
      }
      else
        attack_t::execute();
    }
  };

  struct crackling_tiger_lightning_tick_t: public spell_t
  {
    crackling_tiger_lightning_tick_t( xuen_pet_t *p ): spell_t( "crackling_tiger_lightning_tick", p, p -> find_spell( 123996 ) )
    {
      aoe = 3;
      dual = direct_tick = background = may_crit = may_miss = true;
      range = radius;
      radius = 0;
    }
  };

  struct crackling_tiger_lightning_driver_t: public spell_t
  {
    crackling_tiger_lightning_driver_t( xuen_pet_t *p, const std::string& options_str ): spell_t( "crackling_tiger_lightning_driver", p, p -> o() -> passives.crackling_tiger_lightning_driver )
    {
      parse_options( options_str );

      // for future compatibility, we may want to grab Xuen and our tick spell and build this data from those (Xuen summon duration, for example)
      dot_duration = p -> o() -> talent.invoke_xuen -> duration();
      hasted_ticks = may_miss = false;
      tick_zero = dynamic_tick_action = true; // trigger tick when t == 0
      base_tick_time = p -> o() -> passives.crackling_tiger_lightning_driver -> effectN( 1 ).period(); // trigger a tick every second
      cooldown -> duration = p -> o() -> talent.invoke_xuen -> duration(); // we're done after 45 seconds

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }
  };

  struct crackling_tiger_lightning_t: public melee_attack_t
  {
    crackling_tiger_lightning_t( xuen_pet_t* player, const std::string& options_str ): melee_attack_t( "crackling_tiger_lightning", player, player -> o() -> passives.crackling_tiger_lightning )
    {
      parse_options( options_str );

      // Looks like Xuen needs a couple fixups to work properly.  Let's do that now.
      aoe = 3;
      special = tick_may_crit = true;
      cooldown -> duration = timespan_t::from_seconds( 6.0 );
    }
  };

  struct auto_attack_t: public attack_t
  {
    auto_attack_t( xuen_pet_t* player, const std::string& options_str ):
      attack_t( "auto_attack", player, spell_data_t::nil() )
    {
      parse_options( options_str );

      player -> main_hand_attack = new melee_t( "melee_main_hand", player );
      player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute() override
    {
      player -> main_hand_attack -> schedule_execute();

      if ( player -> off_hand_attack )
        player -> off_hand_attack -> schedule_execute();
    }

    virtual bool ready() override
    {
      if ( player -> is_moving() ) return false;

      return ( player->main_hand_attack -> execute_event == nullptr ); // not swinging
    }
  };

public:
  xuen_pet_t( sim_t* sim, monk_t* owner ):
    pet_t( sim, owner, "xuen_the_white_tiger", true )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    main_hand_weapon.min_dmg = dbc.spell_scaling( o() -> type, level() );
    main_hand_weapon.max_dmg = dbc.spell_scaling( o() -> type, level() );
    main_hand_weapon.damage = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap = 1;
  }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }

  virtual void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "crackling_tiger_lightning" )
      return new crackling_tiger_lightning_driver_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};
} // end namespace pets

namespace actions {
// ==========================================================================
// Monk Abilities
// ==========================================================================

// Template for common monk action code. See priest_action_t.
template <class Base>
struct monk_action_t: public Base
{
  sef_ability_e sef_ability;
  bool hasted_gcd;
private:
  std::array < resource_e, MONK_WINDWALKER + 1 > _resource_by_stance;
  typedef Base ab; // action base, eg. spell_t
public:
  typedef monk_action_t base_t;

  monk_action_t( const std::string& n, monk_t* player,
                 const spell_data_t* s = spell_data_t::nil() ):
                 ab( n, player, s ),
                 sef_ability( SEF_NONE ),
                 hasted_gcd( ab::data().affected_by( player -> passives.aura_mistweaver_monk -> effectN( 4 ) ) )
  {
    ab::may_crit = true;
    range::fill( _resource_by_stance, RESOURCE_MAX );
    ab::min_gcd = timespan_t::from_seconds( 1.0 );
    ab::trigger_gcd = timespan_t::from_seconds( 1.5 );
    if ( player -> specialization() != MONK_MISTWEAVER )
    {
      if ( player -> spec.stagger -> ok() )
        ab::trigger_gcd -= timespan_t::from_millis( player -> spec.stagger -> effectN( 11 ).base_value() * -1 ); // Saved as -500 milliseconds
      if ( player -> spec.stance_of_the_fierce_tiger -> ok() )
        ab::trigger_gcd -= timespan_t::from_millis( player -> spec.stance_of_the_fierce_tiger -> effectN( 6 ).base_value() * -1); // Saved as -500 milliseconds
    }
  }
  virtual ~monk_action_t() {}

  monk_t* p()
  {
    return debug_cast<monk_t*>( ab::player );
  }
  const monk_t* p() const
  {
    return debug_cast<monk_t*>( ab::player );
  }
  monk_td_t* td( player_t* t ) const
  {
    return p() -> get_target_data( t );
  }

  virtual bool ready()
  {
    if ( !ab::ready() )
      return false;

    return true;
  }

  virtual void init()
  {
    ab::init();

    /* Iterate through power entries, and find if there are resources linked to one of our stances
    */
    for ( size_t i = 0; ab::data()._power && i < ab::data()._power -> size(); i++ )
    {
      const spellpower_data_t* pd = ( *ab::data()._power )[i];
      switch ( pd -> aura_id() )
      {
      case 137023:
        assert( _resource_by_stance[MONK_BREWMASTER] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[MONK_BREWMASTER] = pd -> resource();
        break;
      case 137024:
        assert( _resource_by_stance[MONK_MISTWEAVER] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[MONK_MISTWEAVER] = pd -> resource();
        break;
      case 137025:
        assert( _resource_by_stance[MONK_WINDWALKER] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[MONK_WINDWALKER] = pd -> resource();
        break;
      default: break;
      }
    }
  }

  void reset_swing()
  {
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
    {
      p() -> main_hand_attack -> cancel();
      p() -> main_hand_attack -> schedule_execute();
    }
    if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event )
    {
      p() -> off_hand_attack -> cancel();
      p() -> off_hand_attack -> schedule_execute();
    }
  }

  virtual resource_e current_resource() const
  {
    resource_e resource_by_stance = _resource_by_stance[p() -> specialization()];

    if ( resource_by_stance == RESOURCE_MAX )
      return ab::current_resource();

    return resource_by_stance;
  }

  // Make sure the current combo strike ability is not the same as the previous ability used
  virtual bool compare_previous_combo_strikes( combo_strikes_e new_ability )
  {
    return ( p() -> previous_combo_strike == new_ability );
  }

  // Used to trigger Windwalker's Combo Strike Mastery
  void combo_strikes_trigger( combo_strikes_e new_ability )
  {
    if ( !compare_previous_combo_strikes( new_ability ) && p() -> mastery.combo_strikes -> ok() )
    {
      p() -> buff.combo_strikes -> trigger();
      if ( p() -> talent.hit_combo -> ok() )
        p() -> buff.hit_combo -> trigger();
      p() -> previous_combo_strike = new_ability;
    }
    else
    {
      p() -> buff.combo_strikes -> expire();
      p() -> buff.hit_combo -> expire();
    }
  }

  double combo_breaker_chance( combo_strikes_e ability )
  {
    double cb_chance = 0;
    if ( p() -> specialization() == MONK_WINDWALKER )
    {
      if ( ability == CS_RISING_SUN_KICK )
      {
        if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T18, B2 ) )
          cb_chance += p() -> sets.set( MONK_WINDWALKER, T18, B2 ) -> effectN( 1 ).percent();
      }
      else
      {
        cb_chance += p() -> spec.combo_breaker -> effectN( 1 ).percent();
        if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T18, B4 ) )
          cb_chance += p() -> sets.set( MONK_WINDWALKER, T18, B4 ) -> effectN( 1 ).percent();
      }
    }
    return cb_chance;
  }

  double cost() const
  {
    double c = ab::cost();
    if ( c == 0 )
      return c;
    c *= 1.0 + cost_reduction();
    if ( c < 0 ) c = 0;
    return c;
  }

  virtual double cost_reduction() const
  {
    double c = 0.0;

    if ( p() -> specialization() == MONK_MISTWEAVER && p() -> buff.mana_tea -> up() )
      c += p() -> buff.mana_tea -> value();

    return c;
  }

  virtual void consume_resource()
  {
    ab::consume_resource();

    if ( !ab::execute_state ) // Fixes rare crashes at combat_end.
      return;

    // Chi Savings on Dodge & Parry & Miss
    if ( current_resource() == RESOURCE_CHI && ab::resource_consumed > 0 )
    {
      double chi_restored = ab::resource_consumed;
      if ( !ab::aoe && ab::result_is_miss( ab::execute_state -> result ) )
        p() -> resource_gain( RESOURCE_CHI, chi_restored, p() -> gain.chi_refund );
      else if ( p() -> buff.serenity -> up() )
        p() -> resource_gain( RESOURCE_CHI, chi_restored, p() -> gain.serenity );
    }

    // Energy refund, estimated at 80%
    if ( current_resource() == RESOURCE_ENERGY && ab::resource_consumed > 0 && ab::result_is_miss( ab::execute_state -> result ) )
    {
      double energy_restored = ab::resource_consumed * 0.8;

      p() -> resource_gain( RESOURCE_ENERGY, energy_restored, p() -> gain.energy_refund );
    }
  }

  virtual void execute()
  {
    ab::execute();

    trigger_storm_earth_and_fire( this );
  }

  virtual timespan_t gcd() const
  {
    timespan_t t = ab::action_t::gcd();

    if ( t == timespan_t::zero() )
      return t;

    if ( hasted_gcd && ab::player -> specialization() == MONK_MISTWEAVER )
      t *= ab::player -> cache.attack_haste();
    if ( t < ab::min_gcd )
      t = ab::min_gcd;

    return t;
  }

  void trigger_storm_earth_and_fire( const action_t* a )
  {
    if ( ! p() -> spec.storm_earth_and_fire -> ok() )
    {
      return;
    }

    if ( sef_ability == SEF_NONE )
    {
      return;
    }

    for ( sef_pet_e i = SEF_FIRE; i < SEF_PET_MAX; i++ )
    {
      if ( p() -> pet.sef[ i ] -> is_sleeping() )
      {
        continue;
      }
      p() -> pet.sef[ i ] -> trigger_attack( sef_ability, a );
    }
  }
};

struct monk_spell_t: public monk_action_t < spell_t >
{
  monk_spell_t( const std::string& n, monk_t* player,
                const spell_data_t* s = spell_data_t::nil() ):
                base_t( n, player, s )
  {
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double m = base_t::composite_target_multiplier( t );

    return m;
  }
};

struct monk_heal_t: public monk_action_t < heal_t >
{
  monk_heal_t( const std::string& n, monk_t& p,
               const spell_data_t* s = spell_data_t::nil() ):
               base_t( n, &p, s )
  {
    harmful = false;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = base_t::composite_target_multiplier( target );

    return m;
  }
};

namespace attacks {
struct monk_melee_attack_t: public monk_action_t < melee_attack_t >
{
  weapon_t* mh;
  weapon_t* oh;

  monk_melee_attack_t( const std::string& n, monk_t* player,
                       const spell_data_t* s = spell_data_t::nil() ):
                       base_t( n, player, s ),
                       mh( nullptr ), oh( nullptr )
  {
    special = true;
    may_glance = false;
  }

  virtual double target_armor( player_t* t ) const override
  {
    double a = base_t::target_armor( t );

    return a;
  }

  // Special Monk Attack Weapon damage collection, if the pointers mh or oh are set, instead of the classical action_t::weapon
  // Damage is divided instead of multiplied by the weapon speed, AP portion is not multiplied by weapon speed.
  // Both MH and OH are directly weaved into one damage number
  virtual double calculate_weapon_damage( double ap ) const override
  {
    // Use monk specific weapon damage calculation if mh or oh (monk specific weapons) are
    // specificed.
//    if ( mh || oh )
//      return monk_util::monk_weapon_damage( this, mh, oh, weapon_power_mod, ap );
    // Otherwise, use normal weapon damage calculation. It's only used for auto-attacks currently.
//    else
      return melee_attack_t::calculate_weapon_damage( ap );
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double m = base_t::composite_target_multiplier( t );

    return m;
  }

  // Physical tick_action abilities need amount_type() override, so the
  // tick_action are properly physically mitigated.
  dmg_e amount_type( const action_state_t* state, bool periodic ) const override
  {
    if ( tick_action && tick_action -> school == SCHOOL_PHYSICAL )
    {
      return DMG_DIRECT;
    }
    else
    {
      return base_t::amount_type( state, periodic );
    }
  }
};

// ==========================================================================
// Tiger Palm
// ==========================================================================

struct eye_of_the_tiger_heal_tick_t : public monk_heal_t
{
  eye_of_the_tiger_heal_tick_t( monk_t& p, const std::string& name ):
    monk_heal_t( name, p, p.passives.eye_of_the_tiger )
  {
    background = true;
    target = player;
  }
};

struct eye_of_the_tiger_dmg_tick_t: public monk_spell_t
{
  eye_of_the_tiger_dmg_tick_t( monk_t* player, const std::string& name ):
    monk_spell_t( name, player, player -> passives.eye_of_the_tiger )
  {
    background = true;
    attack_power_mod.direct = player -> passives.eye_of_the_tiger -> effectN( 2 ).ap_coeff();
  }
};

struct eye_of_the_tiger_t : public monk_spell_t
{
  heal_t* heal;
  spell_t* damage;
  eye_of_the_tiger_t( monk_t* player ) :
    monk_spell_t( "eye_of_the_tiger", player, player -> talent.eye_of_the_tiger ),
    heal( new eye_of_the_tiger_heal_tick_t( *player, "eye_of_the_tiger_heal" ) ),
    damage( new eye_of_the_tiger_dmg_tick_t( player, "eye_of_the_tiger_damage" ) )
  {
    background = true;
    hasted_ticks = harmful = false;
    dot_duration = player -> passives.eye_of_the_tiger -> duration();
    base_tick_time = player -> passives.eye_of_the_tiger -> effectN( 1 ).period();
    add_child( heal );
    add_child( damage );
    tick_zero = true;
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    p() -> buff.eye_of_the_tiger -> trigger();
  }

  void tick(dot_t* d) override
  {
    monk_spell_t::tick( d );

    damage -> execute();
    heal -> execute();
  }
};

struct tiger_palm_t: public monk_melee_attack_t
{
  eye_of_the_tiger_t* dot;
  tiger_palm_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "tiger_palm", p, p -> spec.tiger_palm ),
    dot( new eye_of_the_tiger_t( p ) )
  {
    parse_options( options_str );
    sef_ability = SEF_TIGER_PALM;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_costs[RESOURCE_ENERGY] *= 1 + ( p -> specialization() == MONK_BREWMASTER ? p -> spec.stagger -> effectN( 16 ).percent() : 0 ); // -50% for Brewmasters
    spell_power_mod.direct = 0.0;
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_TIGER_PALM );

    if ( p() -> talent.eye_of_the_tiger -> ok() )
      dot -> execute();

    if ( p() -> specialization() == MONK_MISTWEAVER)
      p() -> buff.teachings_of_the_monastery -> trigger();
    else if ( p() -> specialization () == MONK_WINDWALKER)
    {
      if ( p() -> aburaq )
        p() -> buff.swift_as_the_wind -> trigger();

      // Chi Gain
      double chi_gain = data().effectN( 2 ).base_value();
      chi_gain += p() -> spec.stance_of_the_fierce_tiger -> effectN( 4 ).base_value();

      if ( p() -> buff.power_strikes -> up() )
      {
        if ( p() -> resources.current[RESOURCE_CHI] + chi_gain < p() -> resources.max[RESOURCE_CHI] )
          player -> resource_gain( RESOURCE_CHI, p() -> buff.power_strikes -> default_value, p() -> gain.power_strikes, this );
        else
          p() -> buff.chi_sphere -> trigger();

        p() -> buff.power_strikes -> expire();
      }
      player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.tiger_palm, this );

      double cb_chance = combo_breaker_chance( CS_TIGER_PALM );
      if ( p() -> buff.combo_breaker_bok -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance ) )
        p() -> proc.combo_breaker_bok -> occur();

      if ( rng().roll( p() -> sets.set( SET_MELEE, T15, B2) -> proc_chance() ) )
      {
        p() -> resource_gain( RESOURCE_ENERGY, p() -> passives.tier15_2pc_melee -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc_melee );
        p() -> proc.tier15_2pc_melee -> occur();
      }
    }
    else if ( p() -> specialization() == MONK_BREWMASTER )
    {
          
      if ( p() -> cooldown.brewmaster_active_mitigation -> down() )
        p() -> cooldown.brewmaster_active_mitigation -> adjust( -1 * timespan_t::from_seconds( data().effectN( 3 ).base_value() ) );

      if ( p() -> talent.secret_ingredients -> ok() )
        p() -> buff.keg_smash_talent -> trigger();
    }
  }
};

// ==========================================================================
// Rising Sun Kick
// ==========================================================================

struct rising_sun_kick_proc_t : public monk_melee_attack_t
{
  rising_sun_kick_proc_t( monk_t* p, const spell_data_t* s ) :
    monk_melee_attack_t( "rising_sun_kick_trinket", p, s )
  {
    sef_ability = SEF_RISING_SUN_KICK_TRINKET;

    cooldown -> duration = timespan_t::from_millis( 250 );
    background = true;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    trigger_gcd = timespan_t::zero();

  }

  bool init_finished()
  {
    bool ret = monk_melee_attack_t::init_finished();
    action_t* rsk = player -> find_action( "rising_sun_kick" );
    if ( rsk )
    {
      base_multiplier = rsk -> base_multiplier;
      spell_power_mod.direct = rsk -> spell_power_mod.direct;

      rsk -> add_child( this );
    }

    return ret;
  }

  // Force 250 milliseconds for the animation, but not delay the overall GCD
  timespan_t execute_time() const override
  {
    return timespan_t::from_millis( 250 );
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    if ( p() -> specialization() == MONK_WINDWALKER )
      p() -> debuffs.mortal_wounds -> trigger();

    double cb_chance = combo_breaker_chance( CS_RISING_SUN_KICK );
    if ( cb_chance > 0 )
    {
      if ( p() -> buff.combo_breaker_bok -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance ) )
          p() -> proc.combo_breaker_bok -> occur();
    }
  }

  virtual double cost() const override
  {
    return 0;
  }
};

struct rising_sun_kick_t: public monk_melee_attack_t
{
  rising_sun_kick_proc_t* rsk_proc;

  rising_sun_kick_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "rising_sun_kick", p, p -> spec.rising_sun_kick )
  {
    parse_options( options_str );

    cooldown -> duration = data().cooldown();
    if ( p -> specialization() == MONK_MISTWEAVER )
      cooldown -> duration += p -> passives.aura_mistweaver_monk -> effectN( 8 ).time_value();

//    if( p -> talent.pool_of_mists -> ok() )
//      cooldown -> charges += p -> talent.pool_of_mists -> effectN( 2 ).base_value();

    sef_ability = SEF_RISING_SUN_KICK;

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    attack_power_mod.direct = p -> spec.rising_sun_kick_trinket -> effectN( 1 ).ap_coeff();
    spell_power_mod.direct = 0.0;

    if ( p -> furious_sun )
      rsk_proc = new rising_sun_kick_proc_t( p, p -> spec.rising_sun_kick_trinket );
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.serenity -> check() )
      cd *= 1 + p() -> talent.serenity -> effectN( 4 ).percent(); // saved as -50

    monk_melee_attack_t::update_ready( cd );
  }

  virtual double action_multiplier() const
  {
    double am = monk_melee_attack_t::action_multiplier();

//    if ( p() -> talent.pool_of_mists -> ok() )
//      am *= 1.0 + p() -> talent.pool_of_mists -> effectN( 4 ).percent();

    if ( p() -> artifact.rising_winds.rank() )
      am *= 1 + p() -> artifact.rising_winds.percent();

    if ( p() -> buff.teachings_of_the_monastery -> up() )
    {
      am *= 1 + p() -> buff.teachings_of_the_monastery -> value();
      p() -> buff.teachings_of_the_monastery -> expire();
    }

    return am;
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_RISING_SUN_KICK );

    if ( p() -> specialization() == MONK_WINDWALKER )
      p() -> debuffs.mortal_wounds -> trigger();

    if ( p() -> artifact.transfer_the_power.rank() )
      p() -> buff.transfer_the_power -> trigger();

    double cb_chance = combo_breaker_chance( CS_RISING_SUN_KICK );
    if ( cb_chance > 0 )
    {
      if ( p() -> buff.combo_breaker_bok -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance ) )
          p() -> proc.combo_breaker_bok -> occur();
    }
  }

  virtual void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    // Windwalker Tier 18 (WoD 6.2) trinket effect is in use, adjust Rising Sun Kick proc chance based on spell data
    // of the special effect.
    if ( p() -> furious_sun )
    {
      double proc_chance = p() -> furious_sun -> driver() -> effectN( 1 ).average( p() -> furious_sun -> item) / 100.0;

      if ( rng().roll( proc_chance ) )
        rsk_proc -> execute();
    }
  }
};

// ==========================================================================
// Blackout Kick
// ==========================================================================

struct blackout_kick_t: public monk_melee_attack_t
{
  rising_sun_kick_proc_t* rsk_proc;

  blackout_kick_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "blackout_kick", p, ( p -> specialization() == MONK_BREWMASTER ? spell_data_t::nil() : p -> spec.blackout_kick ) )
  {
    parse_options( options_str );

    if ( p -> furious_sun )
    {
      rsk_proc = new rising_sun_kick_proc_t( p, p -> spec.rising_sun_kick_trinket );
    }

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    spell_power_mod.direct = 0.0;
    cooldown -> duration = data().cooldown();
    if ( p -> specialization() == MONK_BREWMASTER )
      base_costs[RESOURCE_CHI] *= 1 + p -> spec.stagger -> effectN( 15 ).percent(); // -100% for Brewmasters
    else if ( p -> specialization() == MONK_BREWMASTER )
      cooldown -> duration *= 1 + p -> spec.combat_conditioning -> effectN( 2 ).percent(); // -100% for Windwalkers

    sef_ability = SEF_BLACKOUT_KICK;
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.serenity -> check() )
      cd *= 1 + p() -> talent.serenity -> effectN( 4 ).percent(); // saved as -50

    monk_melee_attack_t::update_ready( cd );
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p() -> talent.dizzying_kicks -> ok() )
      td( s -> target ) -> debuff.dizzing_kicks -> trigger();
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_BLACKOUT_KICK );

    if ( p() -> artifact.transfer_the_power.rank() )
      p() -> buff.transfer_the_power -> trigger();

    if ( p() -> specialization() == MONK_MISTWEAVER )
    {
      if ( p() -> buff.teachings_of_the_monastery -> up() )
      {
        if ( rng().roll( p() -> spec.teachings_of_the_monastery -> effectN( 2 ).percent() ) )
          p() -> cooldown.rising_sun_kick -> reset( true );
      }
    }
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p() -> buff.teachings_of_the_monastery -> up() )
    {
      am *= 1 + p() -> buff.teachings_of_the_monastery -> value();
      p() -> buff.teachings_of_the_monastery -> expire();
    }

    if ( p() -> artifact.dark_skies.rank() )
      am *= 1 + p() -> artifact.dark_skies.percent();

    // check for melee 2p and CB: BoK, for the 50% dmg bonus
    if ( p() -> sets.has_set_bonus( SET_MELEE, T16, B2 ) && p() -> buff.combo_breaker_bok -> up() ) {
      // damage increased by 40% for WW 2pc upon CB
      am *= 1 + ( p() -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).percent() );
    }
    return am;
  }

  virtual double cost() const override
  {
    if ( p() -> buff.combo_breaker_bok -> check() )
      return 0.0;

    return monk_melee_attack_t::cost();
  }

  virtual void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    double savings = base_costs[RESOURCE_CHI] - cost();

    if ( p() -> buff.combo_breaker_bok -> up() )
    {
      p() -> buff.combo_breaker_bok -> expire();
      p() -> gain.combo_breaker_bok -> add( RESOURCE_CHI, savings );
    }

    // Windwalker Tier 18 (WoD 6.2) trinket effect is in use, adjust Rising Sun Kick proc chance based on spell data
    // of the special effect.
    if ( p() -> furious_sun )
    {
      double proc_chance = p() -> furious_sun -> driver() -> effectN( 1 ).average( p() -> furious_sun -> item ) / 100.0;

      if ( rng().roll( proc_chance ) )
        rsk_proc -> execute();
    }
  }
};

// ==========================================================================
// Blackout Strike
// ==========================================================================

struct blackout_strike_t: public monk_melee_attack_t
{
  blackout_strike_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "blackout_strike", p, p -> spec.blackout_strike )
  {
    parse_options( options_str );

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    spell_power_mod.direct = 0.0;
    cooldown -> duration = data().cooldown();
    if ( p -> specialization() == MONK_BREWMASTER )
      base_costs[RESOURCE_CHI] *= 1 + p -> spec.stagger -> effectN( 15 ).percent(); // -100% for Brewmasters
    else if ( p -> specialization() == MONK_BREWMASTER )
      cooldown -> duration *= 1 + p -> spec.combat_conditioning -> effectN( 2 ).percent(); // -100% for Windwalkers
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.serenity -> check() )
      cd *= 1 + p() -> talent.serenity -> effectN( 4 ).percent(); // saved as -50

    monk_melee_attack_t::update_ready( cd );
  }
};

// ==========================================================================
// SCK/RJW Tick Info
// ==========================================================================

// Shared tick action for both abilities
struct tick_action_t : public monk_melee_attack_t
{
  tick_action_t( const std::string& name, monk_t* p, const spell_data_t* data ) :
    monk_melee_attack_t( name, p, data )
  {
    dual = background = true;
    aoe = -1;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    radius = ( p -> talent.rushing_jade_wind -> ok() ? p -> passives.rushing_jade_wind_damage -> effectN( 1 ).radius() : p -> passives.spinning_crane_kick_damage -> effectN( 1 ).radius() );

    // Reset some variables to ensure proper execution
    dot_duration = timespan_t::zero();
    school = SCHOOL_PHYSICAL;
    cooldown -> duration = timespan_t::zero();
    base_costs[ RESOURCE_ENERGY ] = 0;
  }
};


// Rushing Jade Wind ========================================================

struct rushing_jade_wind_t : public monk_melee_attack_t
{
  rushing_jade_wind_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "rushing_jade_wind", p, p -> talent.rushing_jade_wind )
  {
    sef_ability = SEF_RUSHING_JADE_WIND;

    parse_options( options_str );

    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
    tick_zero = hasted_ticks = true;

    attack_power_mod.direct = p -> passives.rushing_jade_wind_damage -> effectN( 1 ).ap_coeff();
    base_costs[RESOURCE_CHI] *= 1 + ( p -> specialization() == MONK_BREWMASTER ? p -> spec.stagger -> effectN( 15 ).percent() : 0 ); // -100% for Brewmasters
    spell_power_mod.direct = 0.0;
    dot_behavior = DOT_REFRESH;

    tick_action = new tick_action_t( "rushing_jade_wind_tick", p, p -> passives.rushing_jade_wind_damage );
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.serenity -> check() )
      cd *= 1 + p() -> talent.serenity -> effectN( 4 ).percent(); // saved as -50

    cd *= p() -> cache.attack_haste();

    monk_melee_attack_t::update_ready( cd );
  }

  // N full ticks, but never additional ones.
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s -> haste ) / base_tick_time );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_RUSHING_JADE_WIND );

    p() -> buff.rushing_jade_wind -> trigger( 1,
        buff_t::DEFAULT_VALUE(),
        1.0,
        composite_dot_duration( execute_state ) );
  }

  void init() override
  {
    monk_melee_attack_t::init();

    update_flags &= ~STATE_HASTE;
  }
};

// Spinning Crane Kick ======================================================

struct spinning_crane_kick_t: public monk_melee_attack_t
{
  spinning_crane_kick_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "spinning_crane_kick", p, p -> spec.spinning_crane_kick )
  {
    parse_options( options_str );

    sef_ability = SEF_SPINNING_CRANE_KICK;

    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
    tick_zero = channeled = true;

    attack_power_mod.direct = p -> passives.spinning_crane_kick_damage -> effectN( 1 ).ap_coeff();
    base_costs[RESOURCE_CHI] *= 1 + ( p -> specialization() == MONK_BREWMASTER ? p -> spec.stagger -> effectN( 15 ).percent() : 0 ); // -100% for Brewmasters
    spell_power_mod.direct = 0.0;

    dot_behavior = DOT_REFRESH;

    tick_action = new tick_action_t( "spinning_crane_kick_tick", p, p -> passives.spinning_crane_kick_damage );
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p() -> artifact.power_of_a_thousand_cranes.rank() )
      am *= 1 + p() -> artifact.power_of_a_thousand_cranes.percent();

    return am;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_SPINNING_CRANE_KICK );
  }
};

// ==========================================================================
// Fists of Fury
// ==========================================================================

struct fists_of_fury_tick_t: public monk_melee_attack_t
{
  fists_of_fury_tick_t( monk_t* p, const std::string& name ):
    monk_melee_attack_t( name, p, p -> spec.fists_of_fury -> effectN( 3 ).trigger() )
  {
    background = true;
    aoe = -1;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    base_costs[ RESOURCE_CHI ] = 0;
    dot_duration = timespan_t::zero();
    trigger_gcd = timespan_t::zero();
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    if ( state -> target != target )
    {
      return 1.0 / state -> n_targets;
    }

    return 1.0;
  }
};

struct fists_of_fury_t: public monk_melee_attack_t
{
  rising_sun_kick_proc_t* rsk_proc;

  fists_of_fury_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "fists_of_fury", p, p -> spec.fists_of_fury )
  {
    parse_options( options_str );

    sef_ability = SEF_FISTS_OF_FURY;

    channeled = tick_zero = true;
    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

    attack_power_mod.direct = p -> spec.fists_of_fury -> effectN( 5 ).ap_coeff();
    spell_power_mod.direct = 0.0;

    // T14 WW 2PC
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).time_value();

    tick_action = new fists_of_fury_tick_t( p, "fists_of_fury_tick" );

    if ( p -> furious_sun )
      rsk_proc = new rising_sun_kick_proc_t( p, p -> spec.rising_sun_kick_trinket );
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.serenity -> check() )
      cd *= 1 + p() -> talent.serenity -> effectN( 4 ).percent(); // saved as -50

    monk_melee_attack_t::update_ready( cd );
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p() -> buff.transfer_the_power -> up() )
    {
      am *= 1 + p() -> buff.transfer_the_power -> stack_value();
      p() -> buff.transfer_the_power -> expire();
    }

    if ( p() -> artifact.fists_of_the_wind.rank() )
      am *= 1 + p() -> artifact.fists_of_the_wind.percent();

    return am;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_FISTS_OF_FURY );
  }

  virtual void last_tick( dot_t* dot ) override
  {
    monk_melee_attack_t::last_tick( dot );
    // Windwalker Tier 18 (WoD 6.2) trinket effect is in use, adjust Rising Sun Kick proc chance based on spell data
    // of the special effect.
    if ( p() -> furious_sun )
    {
      double proc_chance = p() -> furious_sun -> driver() -> effectN( 1 ).average( p() -> furious_sun -> item ) / 100.0;

      if ( rng().roll( proc_chance ) )
        rsk_proc -> execute();
    }
  }
};

// ==========================================================================
// Spinning Dragon Strike
// ==========================================================================

struct spinning_dragon_strike_tick_t: public monk_melee_attack_t
{
  spinning_dragon_strike_tick_t(const std::string& name, monk_t* p, const spell_data_t* s) :
    monk_melee_attack_t( name, p, s )
  {
    background = true;
    aoe = -1;
    radius = p -> passives.spinning_dragon_strike -> effectN( 1 ).radius();

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::zero();
  }
};

struct spinning_dragon_strike_t: public monk_melee_attack_t
{
  rising_sun_kick_proc_t* rsk_proc;

  spinning_dragon_strike_t(monk_t* p, const std::string& options_str) :
    monk_melee_attack_t( "spinning_dragon_strike", p, p -> talent.spinning_dragon_strike )
  {
    sef_ability = SEF_SPINNING_DRAGON_STRIKE;

    parse_options( options_str );
    interrupt_auto_attack = callbacks = false;
    channeled = true;
    dot_duration = data().duration();
    tick_zero = false;

    attack_power_mod.direct = p -> passives.spinning_dragon_strike -> effectN( 1 ).ap_coeff();
    spell_power_mod.direct = 0.0;

    tick_action = new spinning_dragon_strike_tick_t( "spinning_dragon_strike_tick", p, p -> passives.spinning_dragon_strike );

    if ( p -> furious_sun )
      rsk_proc = new rising_sun_kick_proc_t( p, p -> spec.rising_sun_kick_trinket );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_SPINNING_DRAGON_STRIKE );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t tt = tick_time( s -> haste );
    return tt * 3;
  }

  virtual void last_tick(dot_t* dot) override
  {
    monk_melee_attack_t::last_tick(dot); 
    if ( p() -> furious_sun )
    {
      double proc_chance = p() -> furious_sun -> driver() -> effectN( 1 ).average( p() -> furious_sun -> item) / 100.0;
      // Hurricane Strike's proc chance is reduced in half
      proc_chance *= 0.50;

      // Each chi spent has a chance of proccing a Rising Sun Kick. Plausible to see up to 3 RSK procs; though highly unlikely
      for ( int i = 1; i <= 3; i++ ) // TODO: Hard code the 3 for the time being until final word on how this works
      {
        if ( rng().roll( proc_chance ) )
          rsk_proc -> execute();
      }
    }
  }

  virtual bool ready() override
  {
    if ( p() -> cooldown.fists_of_fury -> down() && p() -> cooldown.rising_sun_kick -> down() )
      return monk_melee_attack_t::ready();

    return false;
  }
};


// ==========================================================================
// Strike of the Skylord
// ==========================================================================

struct strike_of_the_skylord_off_hand_t: public monk_melee_attack_t
{
  strike_of_the_skylord_off_hand_t( monk_t* p, const char* name, const spell_data_t* s ):
    monk_melee_attack_t( name, p, s )
  {
    may_dodge = may_parry = may_block = may_miss = true;
    dual = true;
    weapon = &( p -> off_hand_weapon );
  }
};

struct strike_of_the_skylord_t: public monk_melee_attack_t
{
  strike_of_the_skylord_off_hand_t* oh_attack;
  strike_of_the_skylord_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "strike_of_the_skylord", p, &( p -> artifact.strike_of_the_skylord.data() ) ),
    oh_attack( nullptr )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = true;

    oh_attack = new strike_of_the_skylord_off_hand_t( p, "strike_of_the_skylord_offhand", data().effectN( 4 ).trigger() );
    add_child( oh_attack );
  }

  void execute() override
  {
    monk_melee_attack_t::execute(); // this is the MH attack

    if ( oh_attack && result_is_hit( execute_state -> result ) &&
         p() -> off_hand_weapon.type != WEAPON_NONE ) // If MH fails to land, OH does not execute.
      oh_attack -> execute();
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return monk_melee_attack_t::ready();
  }
};

// ==========================================================================
// Melee
// ==========================================================================

struct melee_t: public monk_melee_attack_t
{
  int sync_weapons;
  bool first;
  melee_t( const std::string& name, monk_t* player, int sw ):
    monk_melee_attack_t( name, player, spell_data_t::nil() ),
    sync_weapons( sw ), first( true )
  {
    background = repeating = may_glance = true;
    trigger_gcd = timespan_t::zero();
    special = false;
    school = SCHOOL_PHYSICAL;
    auto_attack = true;

    if ( player -> main_hand_weapon.group() == WEAPON_1H )
    {
//      base_multiplier *= 1.0 + player -> spec.way_of_the_monk_aa_damage -> effectN( 1 ).percent();
      if ( player -> specialization() == MONK_MISTWEAVER )
        base_multiplier *= 1.0 + player -> passives.aura_mistweaver_monk -> effectN( 3 ).percent();
      else
        base_hit -= 0.19;
    }
  }

  void reset() override
  {
    monk_melee_attack_t::reset();
    first = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = monk_melee_attack_t::execute_time();

/*    if ( p() -> main_hand_weapon.group() == WEAPON_2H )
      t *= 1.0 / ( 1.0 + p() -> spec.way_of_the_monk_aa_speed -> effectN( 1 ).percent() );
*/

    if ( p() -> buff.swift_as_the_wind -> up() )
      t * 1.0 / (1.0 + p() -> buff.swift_as_the_wind -> value() );

    if ( first )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    else
      return t;
  }

  void execute() override
  {
    // Prevent the monk from melee'ing while channeling soothing_mist.
    // FIXME: This is super hacky and spams up the APL sample sequence a bit.
    // Disabled since mistweaver doesn't work atm.
    // if ( p() -> buff.channeling_soothing_mist -> check() )
    // return;

    if ( first )
      first = false;

    if ( time_to_execute > timespan_t::zero() && player -> executing )
    {
      if ( sim -> debug ) sim -> out_debug.printf( "Executing '%s' during melee (%s).", player -> executing -> name(), util::slot_type_string( weapon -> slot ) );
      schedule_execute();
    }
    else
      monk_melee_attack_t::execute();
  }
};

// ==========================================================================
// Auto Attack
// ==========================================================================

struct auto_attack_t: public monk_melee_attack_t
{
  int sync_weapons;
  auto_attack_t( monk_t* player, const std::string& options_str ):
    monk_melee_attack_t( "auto_attack", player, spell_data_t::nil() ),
    sync_weapons( 0 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );
    ignore_false_positive = true;

    p() -> main_hand_attack = new melee_t( "melee_main_hand", player, sync_weapons );
    p() -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
    p() -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( !player -> dual_wield() ) return;

      p() -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      p() -> off_hand_attack -> weapon = &( player -> off_hand_weapon );
      p() -> off_hand_attack -> base_execute_time = player -> off_hand_weapon.swing_time;
      p() -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    if ( player -> main_hand_attack )
      p() -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
  }

  bool ready() override
  {
    if ( p() -> current.distance_to_move > 5 )
      return false;

    return( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// ==========================================================================
// Keg Smash
// ==========================================================================

struct keg_smash_t: public monk_melee_attack_t
{

  keg_smash_t( monk_t& p, const std::string& options_str ):
    monk_melee_attack_t( "keg_smash", &p, p.spec.keg_smash )
  {
    parse_options( options_str );

    aoe = -1;
    radius = p.spec.keg_smash -> effectN( 1 ).radius();
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    cooldown -> duration = p.spec.keg_smash -> cooldown();
  }

  virtual bool ready() override
  {
    if ( p() -> buff.keg_smash_talent -> check() )
      return true;

    return monk_melee_attack_t::ready();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    td( s -> target ) -> debuff.keg_smash -> trigger();
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p() -> buff.keg_smash_talent -> check() )
      p() -> buff.keg_smash_talent -> expire();
    
    if ( p() -> cooldown.brewmaster_active_mitigation -> down() )
      p() -> cooldown.brewmaster_active_mitigation -> adjust( -1 * timespan_t::from_seconds( p() -> spec.keg_smash -> effectN( 3 ).base_value() ) );
  }
};

// ==========================================================================
// Special Delivery
// ==========================================================================

struct special_delivery_t : public monk_melee_attack_t
{
  special_delivery_t( monk_t* p ) :
    monk_melee_attack_t( "special_delivery", p, p -> passives.special_delivery )
  {
    background = true;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    trigger_gcd = timespan_t::zero();
    aoe = -1;
    radius = p -> passives.special_delivery -> effectN( 1 ).radius();
  }

  virtual double cost() const override
  {
    return 0;
  }
};

// ==========================================================================
// Touch of Death
// ==========================================================================

struct touch_of_death_t: public monk_melee_attack_t
{
  touch_of_death_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "touch_of_death", p, p -> spec.touch_of_death )
  {
    parse_options( options_str );
    may_crit = may_miss = may_dodge = may_parry = may_block = false;
  }

  virtual double target_armor( player_t* ) const override { return 0; }

  virtual void impact( action_state_t* s ) override
  {
    s -> result_amount = p() -> resources.max[RESOURCE_HEALTH];
    monk_melee_attack_t::impact( s );
  }
};

// ==========================================================================
// Touch of Karma
// ==========================================================================

struct touch_of_karma_dot_t: public residual_action::residual_periodic_action_t < monk_melee_attack_t >
{
  touch_of_karma_dot_t( monk_t* p ):
    base_t( "touch_of_karma", p, p -> passives.touch_of_karma_tick )
  {
    may_miss = may_crit = false;
    dual = true;
  }
};

struct touch_of_karma_t: public monk_melee_attack_t
{
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double pct_health;
  touch_of_karma_dot_t* touch_of_karma_dot;
  touch_of_karma_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "touch_of_karma", p, p -> spec.touch_of_karma ),
    interval( 100 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), pct_health( 0.4 ),
    touch_of_karma_dot( new touch_of_karma_dot_t( p ) )
  {
    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "pct_health", pct_health ) );
    parse_options( options_str );
    cooldown -> duration = data().cooldown();
    base_dd_min = base_dd_max = 0;

    double max_pct = data().effectN( 3 ).percent();
    if ( pct_health > max_pct ) // Does a maximum of 50% of the monk's HP.
      pct_health = max_pct;

    if ( interval < cooldown -> duration.total_seconds() )
    {
      sim -> errorf( "%s minimum interval for Touch of Karma is 90 seconds.", player -> name() );
      interval = cooldown -> duration.total_seconds();
    }

    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;

    may_crit = may_miss = may_dodge = may_parry = false;
  }

  void execute() override
  {
    timespan_t new_cd = timespan_t::from_seconds( rng().gauss( interval, interval_stddev ) );
    timespan_t data_cooldown = data().cooldown();
    if ( new_cd < data_cooldown )
      new_cd = data_cooldown;

    cooldown -> duration = new_cd;

    monk_melee_attack_t::execute();

    if ( pct_health > 0 )
    {
      residual_action::trigger(
        touch_of_karma_dot, execute_state -> target,
        pct_health * player -> resources.max[RESOURCE_HEALTH] );
    }
  }
};

// ==========================================================================
// Provoke
// ==========================================================================

struct provoke_t: public monk_melee_attack_t
{
  provoke_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "provoke", p, p -> spec.provoke )
  {
    parse_options( options_str );
    use_off_gcd = true;
    ignore_false_positive = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    monk_melee_attack_t::impact( s );
  }
};
} // END melee_attacks NAMESPACE

namespace spells {

// ==========================================================================
// Tigereye Brew
// ==========================================================================

struct tigereye_brew_t: public monk_spell_t
{
  tigereye_brew_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "tigereye_brew", player, ( player -> talent.serenity -> ok() ? spell_data_t::nil() : player -> spec.tigereye_brew ) )
  {
    parse_options( options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> charges = data().charges();
    cooldown -> duration = data().duration();
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( p() -> talent.healing_elixirs -> ok() )
    {
      if ( p() -> cooldown.healing_elixirs -> up() )
        p() -> active_actions.healing_elixir -> execute();
    }

    p() -> buff.tigereye_brew -> trigger();
  }
};

// ==========================================================================
// Energizing Elixir
// ==========================================================================

struct energizing_elixir_t: public monk_spell_t
{
  energizing_elixir_t(monk_t* player, const std::string& options_str) :
    monk_spell_t( "energizing_elixir", player, player -> talent.energizing_elixir )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
    dot_duration = trigger_gcd;
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    int energy_max_refund = std::min( p() -> talent.energizing_elixir -> effectN( 1 ).base_value(), static_cast<int>( p() -> resources.max[RESOURCE_ENERGY] ) );
    int energy_refund = energy_max_refund - static_cast<int>( p() -> resources.current[RESOURCE_ENERGY] );

    int chi_max_refund = std::min( p() -> talent.energizing_elixir -> effectN( 2 ).base_value(), static_cast<int>( p() -> resources.max[RESOURCE_CHI] ) );
    int chi_refund = chi_max_refund - static_cast<int>( p() -> resources.current[RESOURCE_CHI] );

    p() -> resource_gain( RESOURCE_ENERGY, energy_refund, p() -> gain.energy_refund );
    p() -> resource_gain( RESOURCE_CHI, chi_refund, p() -> gain.chi_refund );

    if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T17, B4 ) )
      p() -> buff.forceful_winds -> trigger();
  }
};

// ==========================================================================
// Black Ox Brew
// ==========================================================================

struct black_ox_brew_t: public monk_spell_t
{
  black_ox_brew_t(monk_t* player, const std::string& options_str) :
    monk_spell_t( "black_ox_brew", player, player -> talent.black_ox_brew )
  {
    parse_options( options_str );

    harmful = false;
    cooldown -> duration = data().charge_cooldown();
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    int energy_max_refund = std::min( p() -> talent.black_ox_brew -> effectN( 1 ).base_value(), static_cast<int>( p() -> resources.max[RESOURCE_ENERGY] ) );
    int energy_refund = energy_max_refund - static_cast<int>( p() -> resources.current[RESOURCE_ENERGY] );

    p() -> resource_gain( RESOURCE_ENERGY, energy_refund, p() -> gain.energy_refund );

    p() -> cooldown.brewmaster_active_mitigation -> reset( true );
  }
};

// ==========================================================================
// Chi Wave
// ==========================================================================

struct chi_wave_heal_tick_t: public monk_heal_t
{
  chi_wave_heal_tick_t( monk_t& p, const std::string& name ):
    monk_heal_t( name, p, p.passives.chi_wave_heal )
  {
    background = direct_tick = true;
    attack_power_mod.direct = 0.500; // Hard code 09/09/14
    target = player;
  }
};

struct chi_wave_dmg_tick_t: public monk_spell_t
{
  chi_wave_dmg_tick_t( monk_t* player, const std::string& name ):
    monk_spell_t( name, player, player -> passives.chi_wave_damage )
  {
    background = direct_tick = true;
    attack_power_mod.direct = 0.500; // Hard code 09/09/14
  }
};

struct chi_wave_t: public monk_spell_t
{
  heal_t* heal;
  spell_t* damage;
  bool dmg;
  chi_wave_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "chi_wave", player, player -> talent.chi_wave ),
    heal( new chi_wave_heal_tick_t( *player, "chi_wave_heal" ) ),
    damage( new chi_wave_dmg_tick_t( player, "chi_wave_damage" ) ),
    dmg( true )
  {
    sef_ability = SEF_CHI_WAVE;

    parse_options( options_str );
    hasted_ticks = harmful = false;
    dot_duration = timespan_t::from_seconds( player -> talent.chi_wave -> effectN( 1 ).base_value() );
    base_tick_time = timespan_t::from_seconds( 1.0 );
    add_child( heal );
    add_child( damage );
    tick_zero = true;
    radius = player -> find_spell( 132466 ) -> effectN( 2 ).base_value();
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_CHI_WAVE );
  }

  void impact( action_state_t* s ) override
  {
    dmg = true; // Set flag so that the first tick does damage

    monk_spell_t::impact( s );
  }

  void tick( dot_t* d ) override
  {
    monk_spell_t::tick( d );
    // Select appropriate tick action
    if ( dmg )
      damage -> execute();
    else
      heal -> execute();

    dmg = !dmg; // Invert flag for next use
  }
};

// ==========================================================================
// Chi Burst
// ==========================================================================

struct chi_burst_heal_t: public monk_heal_t
{
  chi_burst_heal_t( monk_t& player ):
    monk_heal_t( "chi_burst_heal", player, player.passives.chi_burst_heal )
  {
    background = true;
    target = p();
    attack_power_mod.direct = 2.75;
  }
};

struct chi_burst_t: public monk_spell_t
{
  chi_burst_heal_t* heal;
  chi_burst_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "chi_burst", player, player -> passives.chi_burst_damage ),
    heal( nullptr )
  {
    sef_ability = SEF_CHI_BURST;

    parse_options( options_str );
    heal = new chi_burst_heal_t( *player );
    execute_action = heal;
    aoe = -1;
    interrupt_auto_attack = false;
    attack_power_mod.direct = 2.75; // hardcoded
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    combo_strikes_trigger( CS_CHI_BURST );
  }
};

// ==========================================================================
// Chi Torpedo
// ==========================================================================

struct chi_torpedo_t: public monk_spell_t
{
  chi_torpedo_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "chi_torpedo", player, player -> talent.chi_torpedo )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    cooldown -> duration = p() -> talent.chi_torpedo -> charge_cooldown();
    cooldown -> charges = p() -> talent.chi_torpedo -> charges();
    if ( p() -> artifact.acrobatics.rank() )
      cooldown -> charges += p() -> artifact.acrobatics.value();
  }
};

// ==========================================================================
// Serenity
// ==========================================================================

struct serenity_t: public monk_spell_t
{
  serenity_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "serenity", player, player -> talent.serenity )
  {
    parse_options( options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    monk_spell_t::execute();

    p() -> buff.serenity -> trigger();
  }
};

struct summon_pet_t: public monk_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, const std::string& pname, monk_t* p, const spell_data_t* sd = spell_data_t::nil() ):
    monk_spell_t( n, p, sd ),
    summoning_duration( timespan_t::zero() ), pet_name( pname ), pet( nullptr )
  {
    harmful = false;
  }

  bool init_finished() override
  {
    pet = player -> find_pet( pet_name );
    if ( ! pet )
    {
      background = true;
    }

    return monk_spell_t::init_finished();
  }

  virtual void execute() override
  {
    pet -> summon( summoning_duration );

    monk_spell_t::execute();
  }

  bool ready() override
  {
    if ( ! pet )
    {
      return false;
    }
    return monk_spell_t::ready();
  }
};

// ==========================================================================
// Invoke Xuen, the White Tiger
// ==========================================================================

struct xuen_spell_t: public summon_pet_t
{
  xuen_spell_t( monk_t* p, const std::string& options_str ):
    summon_pet_t( "invoke_xuen", "xuen_the_white_tiger", p, p -> talent.invoke_xuen )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
    summoning_duration = data().duration();
  }
};

// ==========================================================================
// Storm, Earth, and Fire
// ==========================================================================

struct storm_earth_and_fire_t;

struct sef_despawn_cb_t
{
  storm_earth_and_fire_t* action;

  sef_despawn_cb_t( storm_earth_and_fire_t* a );

  void operator()(player_t*);
};

struct storm_earth_and_fire_t: public monk_spell_t
{
  storm_earth_and_fire_t( monk_t* p, const std::string& options_str ):
    monk_spell_t( "storm_earth_and_fire", p, p -> spec.storm_earth_and_fire )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    callbacks = harmful = may_miss = may_crit = may_dodge = may_parry = may_block = false;
  }

  void init() override
  {
    monk_spell_t::init();

    sim -> target_non_sleeping_list.register_callback( sef_despawn_cb_t( this ) );
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    // No clone on target, check if we can spawn one or not
    if ( td( execute_state -> target ) -> debuff.storm_earth_and_fire -> check() == 0 )
    {
      // Already full clones, despawn both
      if ( p() -> buff.storm_earth_and_fire -> check() == 2 )
      {
        for ( size_t i = 0; i < sizeof_array( p() -> pet.sef ); i++ )
        {
          if ( p() -> pet.sef[ i ] -> is_sleeping() )
          {
            continue;
          }

          p() -> pet.sef[ i ] -> dismiss();
        }

        assert( p() -> buff.storm_earth_and_fire -> check() == 0 );
      }
      // Can fit a clone on the target, randomize which clone is spawned
      // TODO: Is this really random?
      else
      {
        std::vector<size_t> sef_idx;
        for ( size_t i = 0; i < sizeof_array( p() -> pet.sef ); i++ )
        {
          if ( ! p() -> pet.sef[ i ] -> is_sleeping() )
          {
            continue;
          }

          sef_idx.push_back( i );
        }

        unsigned rng_idx = static_cast<unsigned>( rng().range( 0.0, static_cast<double>( sef_idx.size() ) ) );
        assert( rng_idx < sef_idx.size() );
        size_t idx = sef_idx[ rng_idx ];

        p() -> pet.sef[ idx ] -> target = execute_state -> target;
        p() -> pet.sef[ idx ] -> summon();
      }
    }
    // Clone on target, despawn that specific clone
    else
    {
      for ( size_t i = 0; i < sizeof_array( p() -> pet.sef ); i++ )
      {
        if ( p() -> pet.sef[ i ] -> is_sleeping() )
        {
          continue;
        }

        if ( p() -> pet.sef[ i ] -> target != execute_state -> target )
        {
          continue;
        }

        p() -> pet.sef[ i ] -> dismiss();
      }
    }
  }
};

sef_despawn_cb_t::sef_despawn_cb_t( storm_earth_and_fire_t* a ) : action( a )
{ assert( action ); }

void sef_despawn_cb_t::operator()(player_t*)
{
  for ( size_t i = 0; i < sizeof_array( action -> p() -> pet.sef ); i++ )
  {
    pets::storm_earth_and_fire_pet_t* sef = action -> p() -> pet.sef[ i ];
    // Dormant clones don't care
    if ( sef -> is_sleeping() )
    {
      continue;
    }

    // If the active clone's target is sleeping, lets despawn it.
    if ( sef -> target -> is_sleeping() )
    {
      sef -> dismiss();
    }
  }
}

// ==========================================================================
// Chi Sphere
// ==========================================================================

struct chi_sphere_t: public monk_spell_t
{
  chi_sphere_t( monk_t* p, const std::string& options_str ):
    monk_spell_t( "chi_sphere", p, p -> passives.chi_sphere )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( p() -> buff.chi_sphere -> up() )
    {
      // Only use 1 Orb per execution
      player -> resource_gain( RESOURCE_CHI, p() -> buff.chi_sphere -> value(), p() -> gain.power_strikes, this );

      p() -> buff.chi_sphere -> decrement();
    }
  }
};

// ==========================================================================
// Chi Orbit
// ==========================================================================

struct chi_orbit_t: public monk_spell_t
{
  chi_orbit_t( monk_t* p, const std::string& options_str ):
    monk_spell_t( "chi_orbit", p, p -> talent.chi_orbit )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
    attack_power_mod.direct = p -> passives.chi_orbit -> effectN( 1 ).ap_coeff();
  }

  bool ready() override
  {
    return p() -> buff.chi_orbit -> up();
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( p() -> buff.chi_orbit -> up() )
      p() -> buff.chi_orbit -> decrement();
  }
};

// ==========================================================================
// Breath of Fire
// ==========================================================================

struct breath_of_fire_t: public monk_spell_t
{
  struct periodic_t: public monk_spell_t
  {
    periodic_t( monk_t& p ):
      monk_spell_t( "breath_of_fire_dot", &p, p.passives.breath_of_fire_dot )
    {
      background = true;
      tick_may_crit = may_crit = true;
      hasted_ticks = false;
    }
  };

  periodic_t* dot_action;
  breath_of_fire_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "breath_of_fire", &p, p.spec.breath_of_fire ),
    dot_action( new periodic_t( p ) )
  {
    parse_options( options_str );
    aoe = -1;
    base_costs[RESOURCE_CHI] *= 1 + p.spec.stagger -> effectN( 15 ).percent(); // -100% for Brewmasters
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    monk_td_t& td = *this -> td( s -> target );

    if ( td.debuff.keg_smash -> up() )
    {
      dot_action -> target = s -> target;
      dot_action -> execute();
    }
  }
};

// ==========================================================================
// Fortifying Brew
// ==========================================================================

struct fortifying_brew_t: public monk_spell_t
{
  fortifying_brew_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "fortifying_brew", &p, p.spec.fortifying_brew )
  {
    parse_options( options_str );

    harmful = may_crit = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration += p.sets.set( SET_TANK, T16, B2 ) -> effectN( 1 ).time_value();
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    p() -> buff.fortifying_brew -> trigger();
  }
};

// ==========================================================================
// Mana Tea
// ==========================================================================
// Manatee
//                   _.---.._
//     _        _.-'         ''-.
//   .'  '-,_.-'                 '''.
//  (       _                     o  :
//   '._ .-'  '-._         \  \-  ---]
//                 '-.___.-')  )..-'
//                          (_/lame

struct mana_tea_t: public monk_spell_t
{
  mana_tea_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "mana_tea", &p, p.talent.mana_tea )
  {
    parse_options( options_str );

    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    monk_spell_t::execute();

    p() -> buff.mana_tea -> trigger();

    if ( p() -> talent.healing_elixirs -> ok() )
    {
      if ( p() -> cooldown.healing_elixirs -> up() )
        p() -> active_actions.healing_elixir -> execute();
    }
  }
};

// ==========================================================================
// Stagger Damage
// ==========================================================================

struct stagger_self_damage_t : public residual_action::residual_periodic_action_t < monk_spell_t >
{
  stagger_self_damage_t( monk_t* p ):
    base_t( "stagger_self_damage", p, p -> passives.stagger_self_damage )
  {
    dot_duration = p -> spec.heavy_stagger -> duration();
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks = tick_may_crit = false;
    target = p;

    callbacks = false;
  }

  virtual void init() override
  {
    base_t::init();

    // We don't want this counted towards our dps
    stats -> type = STATS_NEUTRAL;
  }

  /* Clears the dot and all damage. Used by Purifying Brew
  * Returns amount purged
  */
  double clear_all_damage()
  {
    dot_t* d = get_dot();
    double damage_remaining = 0.0;
    if ( d -> is_ticking() )
      damage_remaining += d -> state -> result_amount; // Assumes base_td == damage, no modifiers or crits

    cancel();
    d -> cancel();

    return damage_remaining;
  }

  bool stagger_ticking()
  {
    dot_t* d = get_dot();
    return d -> is_ticking();
  }

  double tick_amount()
  {
    dot_t* d = get_dot();
    if ( d && d -> state )
      return calculate_tick_amount( d -> state, 1 );
    return 0;
  }
};

// ==========================================================================
// Ironskin Brew
// ==========================================================================

struct ironskin_brew_t : public monk_spell_t
{
//  special_delivery_t* delivery;

  ironskin_brew_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "ironskin_brew", &p, p.spec.ironskin_brew )
//    delivery( new special_delivery_t( &p ) )
  {
    parse_options( options_str );

    harmful = false;
    trigger_gcd = timespan_t::zero();

    cooldown             = p.cooldown.brewmaster_active_mitigation;
    cooldown -> duration = p.find_spell( id ) -> cooldown() + p.talent.light_brewing -> effectN( 1 ).time_value(); // Saved as -5000
    cooldown -> charges  = p.find_spell( id ) -> charges() + p.talent.light_brewing -> effectN( 2 ).base_value();
  }

  void execute() override
  {
    monk_spell_t::execute();

    p() -> buff.ironskin_brew -> trigger();

    // TODO: Get the actual amount that Fortified Mind reduces Fortifying Brew's cooldown by
    if ( p() -> talent.fortified_mind -> ok() )
      p() -> cooldown.fortifying_brew -> duration += timespan_t::zero();

    /*
    if ( p() -> talent.special_delivery -> ok() )
      if ( rng().roll( p() -> talent.special_delivery -> proc_chance() ) )
        delivery -> execute();
    */
  }
};

// ==========================================================================
// Purifying Brew
// ==========================================================================

struct purifying_brew_t: public monk_spell_t
{
//  special_delivery_t* delivery;

  purifying_brew_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "purifying_brew", &p, p.spec.purifying_brew )
//    delivery( new special_delivery_t( &p ) )
  {
    parse_options( options_str );

    harmful = false;
    trigger_gcd = timespan_t::zero();

    cooldown             = p.cooldown.brewmaster_active_mitigation;
    cooldown -> duration = p.find_spell( id ) -> cooldown() + p.talent.light_brewing -> effectN( 1 ).time_value(); // Saved as -5000
    cooldown -> charges  = p.find_spell( id ) -> charges() + p.talent.light_brewing -> effectN( 2 ).base_value();
  }

  void execute() override
  {
    monk_spell_t::execute();

    double stagger_pct = p() -> current_stagger_tick_dmg_percent();
    double stagger_dmg = p() -> clear_stagger();

    // Tier 16 4 pieces Brewmaster: Purifying Brew also heals you for 15% of the amount of staggered damage cleared.
    if ( p() -> sets.has_set_bonus( SET_TANK, T16, B4 ) )
    {
      double stagger_heal = stagger_dmg * p() -> sets.set( SET_TANK, T16, B4 ) -> effectN( 1 ).percent();
      player -> resource_gain( RESOURCE_HEALTH, stagger_heal, p() -> gain.tier16_4pc_tank, this );
    }

    // Optional addition: Track and report amount of damage cleared
    if ( stagger_pct > p() -> heavy_stagger_threshold )
    {
      if ( p() -> talent.elusive_dance -> ok() )
        p() -> buff.elusive_dance-> trigger( 3 );
      p() -> sample_datas.heavy_stagger_total_damage -> add( stagger_dmg );
    }
    else if ( stagger_pct > p() -> moderate_stagger_threshold )
    {
      if ( p() -> talent.elusive_dance -> ok() )
        p() -> buff.elusive_dance-> trigger( 2 );
      p() -> sample_datas.moderate_stagger_total_damage -> add( stagger_dmg );
    }
    else
    {
      if ( p() -> talent.elusive_dance -> ok() )
        p() -> buff.elusive_dance-> trigger();
      p() -> sample_datas.light_stagger_total_damage -> add( stagger_dmg );
    }

    p() -> sample_datas.purified_damage -> add( stagger_dmg );

    // TODO: Get the actual amount that Fortified Mind reduces Fortifying Brew's cooldown by
    if ( p() -> talent.fortified_mind -> ok() )
      p() -> cooldown.fortifying_brew -> duration += timespan_t::zero();

    /*
    if ( p() -> talent.special_delivery -> ok() )
      if ( rng().roll( p() -> talent.special_delivery -> proc_chance() ) )
        delivery -> execute();
    */
  }

  bool ready() override
  {
    // Irrealistic of in-game, but let's make sure stagger is actually present
    if ( !p() -> active_actions.stagger_self_damage -> stagger_ticking() )
      return false;

    return monk_spell_t::ready();
  }
};

// ==========================================================================
// Crackling Jade Lightning
// ==========================================================================

struct crackling_jade_lightning_t: public monk_spell_t
{
  crackling_jade_lightning_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "crackling_jade_lightning", &p, p.spec.crackling_jade_lightning )
  {
    parse_options( options_str );

    channeled = tick_may_crit = true;
    hasted_ticks = false; // Channeled spells always have hasted ticks. Use hasted_ticks = false to disable the increase in the number of ticks.
    procs_courageous_primal_diamond = false;
    base_tick_time *= 1.0 + p.spec.crane_style_techniques -> effectN( 4 ).percent();
    dot_duration *= 1.0 + p.spec.crane_style_techniques -> effectN( 5 ).percent();
    base_costs_per_tick[RESOURCE_ENERGY] = ( p.specialization() == MONK_MISTWEAVER ? 0 : 15 );
  }

  double cost() const override
  {
    if ( p() -> specialization() == MONK_MISTWEAVER )
      return 0;
    return monk_spell_t::cost();
  }

  void last_tick( dot_t* dot ) override
  {
    monk_spell_t::last_tick( dot );

    // Reset swing timer
    if ( player -> main_hand_attack )
    {
      player -> main_hand_attack -> cancel();
      player -> main_hand_attack -> schedule_execute();
    }

    if ( player -> off_hand_attack )
    {
      player -> off_hand_attack -> cancel();
      player -> off_hand_attack -> schedule_execute();
    }
  }
};

// ==========================================================================
// Dampen Harm
// ==========================================================================

struct dampen_harm_t: public monk_spell_t
{
  dampen_harm_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "dampen_harm", &p, p.talent.dampen_harm )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    p() -> buff.dampen_harm -> trigger( data().max_stacks() );
  }
};

// ==========================================================================
// Diffuse Magic
// ==========================================================================

struct diffuse_magic_t: public monk_spell_t
{
  diffuse_magic_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "diffuse_magic", &p, p.talent.diffuse_magic )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  virtual void execute() override
  {
    p() -> buff.diffuse_magic -> trigger();
    monk_spell_t::execute();
  }
};
} // END spells NAMESPACE

namespace heals {
// ==========================================================================
// Enveloping Mist
// ==========================================================================
/*
TODO: Verify healing values.
*/

struct enveloping_mist_t: public monk_heal_t
{
  enveloping_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "enveloping_mist", p, p.spec.enveloping_mist )
  {
    parse_options( options_str );

    may_miss = false;
  }
};

// ==========================================================================
// Renewing Mist
// ==========================================================================
/*
TODO: Verify healing values.
Add bouncing.
*/

struct renewing_mist_t: public monk_heal_t
{
  renewing_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "renewing_mist", p, p.spec.renewing_mist )
  {
    parse_options( options_str );
    may_crit = may_miss = false;
    dot_duration = p.spec.renewing_mist_heal -> duration();
  }

  virtual double cost() const override
  {
    double c = monk_heal_t::cost();

    if ( p() -> buff.chi_jis_guidance -> up() )
      c *= 1 + p() -> buff.chi_jis_guidance -> value(); // Saved as -50%
    return c;
  }


  virtual void execute() override
  {
    monk_heal_t::execute();
  }
};

// ==========================================================================
// Soothing Mist
// ==========================================================================
/*
DESC: Surging Mist and Enveloping Mist need to be able to be cast while
we're channeling Soothing Mist WITHOUT interrupting the channel. to
achieve this, soothing_mist applies a HoT to the target that is
removed whenever the monk executes an action that is not one of these
whitelised spells. Auto attack is also halted while the HoT is
active. The HoT is also removed any time the user moves, is stunned,
interrupted, etc.

TODO: Verify healing values.
Change cost from per tick to on use + per second.
Confirm the mana consumption is affected by lucidity.
*/

struct soothing_mist_t: public monk_heal_t
{
  int consecutive_failed_chi_procs;

  soothing_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "soothing_mist", p, p.spec.soothing_mist )
  {
    parse_options( options_str );

    tick_zero = true;
    consecutive_failed_chi_procs = 0;

    // Cost is handled in action_t::tick()
    base_costs[RESOURCE_MANA] = 0.0;
    base_costs_per_tick[RESOURCE_MANA] = 0.0;
  }

  virtual double action_ta_multiplier() const override
  {
    double tm = monk_heal_t::action_ta_multiplier();

    player_t* t = ( execute_state ) ? execute_state -> target : target;

    if ( td( t ) -> dots.enveloping_mist -> is_ticking() )
      tm *= 1.0 + p() -> spec.enveloping_mist -> effectN( 2 ).percent();

    return tm;
  }

  virtual void tick( dot_t* d ) override
  {
    // Deduct mana / fizzle if the caster does not have enough mana
    double tick_cost = data().cost( POWER_MANA ) * p() -> resources.base[RESOURCE_MANA] * p() -> composite_spell_haste();
    if ( p() -> resources.current[RESOURCE_MANA] >= tick_cost )
      p() -> resource_loss( RESOURCE_MANA, tick_cost, p() -> gain.soothing_mist, this );
    else
    {
      player_t* t = ( execute_state ) ? execute_state -> target : target;

      td( t ) -> dots.enveloping_mist -> cancel();
      p() -> buff.channeling_soothing_mist -> expire();
      return;
    }

    monk_heal_t::tick( d );

    if ( p() -> sets.has_set_bonus ( MONK_MISTWEAVER, T17, B2 ) )
      p() -> buff.mistweaving -> trigger();

  }

  virtual void impact( action_state_t* s ) override
  {
    monk_heal_t::impact( s );

    p() -> buff.channeling_soothing_mist -> trigger();
  }

  virtual void last_tick( dot_t* d ) override
  {
    monk_heal_t::last_tick( d );

    p() -> buff.channeling_soothing_mist -> expire();
    p() -> buff.mistweaving -> expire();
  }

  virtual bool ready() override
  {
    if ( p() -> buff.channeling_soothing_mist -> check() )
      return false;

    // Mana cost check
    double tick_cost = data().cost( POWER_MANA ) * p() -> resources.base[RESOURCE_MANA];
    if ( p() -> resources.current[RESOURCE_MANA] < tick_cost )
      return false;

    return monk_heal_t::ready();
  }
};

// ==========================================================================
// Effuse
// ==========================================================================

struct effuse_t: public monk_heal_t
{
  effuse_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "effuse", p, p.spec.effuse )
  {
    parse_options( options_str );

    hasted_gcd = true;

    if ( p.specialization() == MONK_MISTWEAVER )
    {
      resource_current = RESOURCE_MANA;
      base_costs[RESOURCE_MANA] = p.spec.effuse-> cost( POWER_MANA ) * p.resources.base[RESOURCE_MANA];
    }
    else
    {
      resource_current = RESOURCE_ENERGY;
      base_costs[RESOURCE_ENERGY] = p.spec.effuse-> cost( POWER_ENERGY );
    }

    may_miss = false;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t et = monk_heal_t::execute_time();

    return et;
  }

  virtual double cost() const override
  {
    double c = monk_heal_t::cost();

    return c;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( p() -> sets.has_set_bonus( MONK_MISTWEAVER, T17, B4 ) && s -> result == RESULT_CRIT )
      p() -> buff.chi_jis_guidance -> trigger();
  }
};

// ==========================================================================
// Zen Sphere (Heal)
// ==========================================================================
/*
struct zen_sphere_t: public monk_heal_t
{
  struct zen_sphere_dot_t: public monk_spell_t
  {
    zen_sphere_dot_t( monk_t& player ):
      monk_spell_t( "zen_sphere_dmg", &player, player.find_spell( 124098 ) )
    {
      background = dual = true;

      attack_power_mod.direct = 0.095;
    }
  };

  struct zen_sphere_explosion_heal_t : public monk_heal_t
  {
    zen_sphere_explosion_heal_t( monk_t& p ):
      monk_heal_t( "zen_sphere_detonate_heal", p, p.find_spell( 124101 ) )
    {
      background = dual = true;
      aoe = -1;

      attack_power_mod.direct = 1.25;
    }

  struct zen_sphere_explosion_dmg_t: public monk_spell_t
  {
    zen_sphere_explosion_dmg_t( monk_t& player ):
      monk_spell_t( "zen_sphere_detonate", &player, player.find_spell( 125033 ) )
    {
      background = dual = true;
      aoe = -1;

      attack_power_mod.direct = 1.25;
    }
  };

  zen_sphere_dot_t*            dot;
  zen_sphere_explosion_heal_t* explosion_heal;
  zen_sphere_explosion_dmg_t*  explosion_dmg;

  zen_sphere_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "zen_sphere", p, p.talent.zen_sphere ),
    dot( new zen_sphere_dot_t( p ) ),
    explosion_heal( new zen_sphere_explosion_heal_t( p ) ),
    explosion_dmg( new zen_sphere_explosion_dmg_t( p ) )
  {
    sef_ability = SEF_ZEN_SPHERE;

    parse_options( options_str );

    add_child( explosion_heal );
    dot -> add_child( explosion_dmg );

    attack_power_mod.tick = 0.095;
    hasted_ticks = false;

    cooldown -> duration = data().cooldown();
  }

  void last_tick( dot_t* d ) override
  {
    monk_heal_t::last_tick( d );

    if ( ! player -> is_sleeping() )
    {
      explosion_dmg -> execute();

      if ( player -> record_healing() )
      {
        explosion_heal -> execute();
      }
    }
  }
};
*/

// ==========================================================================
// Gift of the Ox
// ==========================================================================

struct gift_of_the_ox_t: public monk_heal_t
{
  gift_of_the_ox_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "gift_of_the_ox", p, p.passives.gift_of_the_ox_heal )
  {
    parse_options( options_str );
    harmful = false;
    background = true;
    target = &p;
    trigger_gcd = timespan_t::zero();
    base_pct_heal = p.passives.gift_of_the_ox_heal -> effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    if ( p() -> buff.gift_of_the_ox -> up() )
    {
      monk_heal_t::execute();

      p() -> buff.gift_of_the_ox -> decrement();
    }
  }
};

// ==========================================================================
// Healing Elixirs
// ==========================================================================

struct healing_elixirs_t: public monk_heal_t
{
  healing_elixirs_t( monk_t& p ):
    monk_heal_t( "healing_elixirs", p, p.talent.healing_elixirs )
  {
    harmful = may_crit = false;
    background = true;
    may_multistrike = 0;
    target = &p;
    trigger_gcd = timespan_t::zero();
    base_pct_heal = p.passives.healing_elixirs -> effectN( 1 ).percent();
    cooldown -> duration = data().effectN( 1 ).period();
  }
};
} // end namespace heals

namespace absorbs {
struct monk_absorb_t: public monk_action_t < absorb_t >
{
  monk_absorb_t( const std::string& n, monk_t& player,
                 const spell_data_t* s = spell_data_t::nil() ):
                 base_t( n, &player, s )
  {
  }
};
} // end namespace absorbs

using namespace attacks;
using namespace spells;
using namespace heals;
using namespace absorbs;
} // end namespace actions;

struct chi_orbit_event_t : public player_event_t
{
  chi_orbit_event_t( monk_t& player, timespan_t tick_time ):
    player_event_t( player )
  {
    tick_time = clamp( tick_time, timespan_t::zero(), player.talent.chi_orbit -> effectN( 1 ).period() );
    add_event( tick_time );
  }
  virtual const char* name() const override
  { return  "chi_orbit"; }
  virtual void execute() override
  {
    monk_t* p = debug_cast<monk_t*>( player() );

    p -> buff.chi_orbit -> trigger();

    new ( sim() ) chi_orbit_event_t( *p, p -> talent.chi_orbit -> effectN( 1 ).period() );
  }
};

struct power_strikes_event_t: public player_event_t
{
  power_strikes_event_t( monk_t& player, timespan_t tick_time ):
    player_event_t( player )
  {
    // Safety clamp
    tick_time = clamp( tick_time, timespan_t::zero(), player.talent.power_strikes -> effectN( 1 ).period() );
    add_event( tick_time );
  }
  virtual const char* name() const override
  { return  "power_strikes"; }
  virtual void execute() override
  {
    monk_t* p = debug_cast<monk_t*>( player() );

    p -> buff.power_strikes -> trigger();

    new ( sim() ) power_strikes_event_t( *p, p -> talent.power_strikes -> effectN( 1 ).period() );
  }
};

// ==========================================================================
// Monk Buffs
// ==========================================================================

namespace buffs
{
  template <typename Base>
  struct monk_buff_t: public Base
  {
    public:
    typedef monk_buff_t base_t;

    monk_buff_t( monk_td_t& p, const buff_creator_basics_t& params ):
      Base( params ), monk( p.monk )
    {}

    monk_buff_t( monk_t& p, const buff_creator_basics_t& params ):
      Base( params ), monk( p )
    {}

    monk_td_t& get_td( player_t* t ) const
    {
      return *(monk.get_target_data( t ));
    }

    protected:
    monk_t& monk;
  };

  struct fortifying_brew_t: public monk_buff_t < buff_t >
{
  int health_gain;
  fortifying_brew_t( monk_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s ).cd( timespan_t::zero() ) ), health_gain( 0 )
  {}

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    // Extra Health is set by current max_health, doesn't change when max_health changes.
    health_gain = static_cast<int>( monk.resources.max[RESOURCE_HEALTH] * ( monk.spec.fortifying_brew -> effectN( 1 ).percent() ) );
    monk.stat_gain( STAT_MAX_HEALTH, health_gain, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    monk.stat_gain( STAT_HEALTH, health_gain, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    return base_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );
    monk.stat_loss( STAT_MAX_HEALTH, health_gain, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    monk.stat_loss( STAT_HEALTH, health_gain, ( gain_t* )nullptr, ( action_t* )nullptr, true );
  }
};
}

// ==========================================================================
// Monk Character Definition
// ==========================================================================

monk_td_t::monk_td_t( player_t* target, monk_t* p ):
actor_target_data_t( target, p ),
dots( dots_t() ),
debuff( buffs_t() ),
monk( *p )
{
  debuff.dizzing_kicks = buff_creator_t( *this, "dizzying_kicks" )
    .spell( p -> passives.dizzying_kicks )
    .default_value( p-> passives.dizzying_kicks -> effectN( 1 ).percent() );
  debuff.keg_smash = buff_creator_t( *this, "keg_smash" )
    .spell( p -> spec.keg_smash )
    .default_value( p -> spec.keg_smash -> effectN( 2 ).percent() );
  debuff.storm_earth_and_fire = buff_creator_t( *this, "storm_earth_and_fire_target" )
    .cd( timespan_t::zero() );

  dots.enveloping_mist = target -> get_dot( "enveloping_mist", p );
  dots.renewing_mist = target -> get_dot( "renewing_mist", p );
  dots.soothing_mist = target -> get_dot( "soothing_mist", p );
  dots.zen_sphere = target -> get_dot( "zen_sphere", p );
}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  using namespace actions;
  // Melee Attacks
  if (name == "auto_attack") return new                 auto_attack_t(this, options_str);
  if ( name == "tiger_palm" ) return new                tiger_palm_t( this, options_str );
  if ( name == "blackout_kick" ) return new             blackout_kick_t( this, options_str );
  if ( name == "spinning_crane_kick" ) return new       spinning_crane_kick_t( this, options_str );
  if ( name == "fists_of_fury" ) return new             fists_of_fury_t( this, options_str );
  if ( name == "rising_sun_kick" ) return new           rising_sun_kick_t( this, options_str );
  if ( name == "tigereye_brew" ) return new             tigereye_brew_t( this, options_str );
  if ( name == "touch_of_karma" ) return new            touch_of_karma_t( this, options_str );
  if ( name == "energizing_elixir" ) return new         energizing_elixir_t( this, options_str );
  if ( name == "provoke" ) return new                   provoke_t( this, options_str );
  if ( name == "touch_of_death" ) return new            touch_of_death_t( this, options_str );
  if ( name == "storm_earth_and_fire" ) return new      storm_earth_and_fire_t( this, options_str );
  // Brewmaster
  if ( name == "blackout_strike" ) return new           blackout_strike_t( this, options_str );
  if ( name == "breath_of_fire" ) return new            breath_of_fire_t( *this, options_str );
  if ( name == "keg_smash" ) return new                 keg_smash_t( *this, options_str );
  if ( name == "fortifying_brew" ) return new           fortifying_brew_t( *this, options_str );
  if ( name == "ironskin_brew" ) return new             ironskin_brew_t( *this, options_str );
  if ( name == "purifying_brew" ) return new            purifying_brew_t( *this, options_str );
  if ( name == "gift_of_the_ox" ) return new            gift_of_the_ox_t( *this, options_str );
  // Mistweaver
  if ( name == "enveloping_mist" ) return new           enveloping_mist_t( *this, options_str );
  if ( name == "mana_tea" ) return new                  mana_tea_t( *this, options_str );
  if ( name == "renewing_mist" ) return new             renewing_mist_t( *this, options_str );
  if ( name == "soothing_mist" ) return new             soothing_mist_t( *this, options_str );
  if ( name == "effuse" ) return new                    effuse_t( *this, options_str );
  if ( name == "crackling_jade_lightning" ) return new  crackling_jade_lightning_t( *this, options_str );
  // Talents
  if ( name == "chi_wave" ) return new                  chi_wave_t( this, options_str );
  if ( name == "chi_burst" ) return new                 chi_burst_t( this, options_str );
  if ( name == "chi_sphere" ) return new                chi_sphere_t( this, options_str ); // For Power Strikes
  if ( name == "black_ox_brew" ) return new             black_ox_brew_t( this, options_str );
  if ( name == "dampen_harm" ) return new               dampen_harm_t( *this, options_str );
  if ( name == "diffuse_magic" ) return new             diffuse_magic_t( *this, options_str );
  if ( name == "rushing_jade_wind" ) return new         rushing_jade_wind_t( this, options_str );
  if ( name == "invoke_xuen" ) return new               xuen_spell_t( this, options_str );
  if ( name == "chi_torpedo" ) return new               chi_torpedo_t( this, options_str );
  if ( name == "chi_orbit" ) return new                 chi_orbit_t( this, options_str );
  if ( name == "spinning_dragon_strike" ) return new    spinning_dragon_strike_t( this, options_str );
  if ( name == "serenity" ) return new                  serenity_t( this, options_str );
  // Artifacts
  if ( name == "strike_of_the_skylord" ) return new     strike_of_the_skylord_t( this, options_str );
  return base_t::create_action( name, options_str );
}

// monk_t::create_pet =======================================================

pet_t* monk_t::create_pet( const std::string& name,
                           const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( name );

  if ( p ) return p;

  using namespace pets;
  if ( name == "xuen_the_white_tiger" ) return new xuen_pet_t( sim, this );

  return nullptr;
}

// monk_t::create_pets ======================================================

void monk_t::create_pets()
{
  base_t::create_pets();

  if ( talent.invoke_xuen -> ok() && find_action( "invoke_xuen" ) )
  {
    create_pet( "xuen_the_white_tiger" );
  }

  if ( specialization() == MONK_WINDWALKER && find_action( "storm_earth_and_fire" ) )
  {
    pet.sef[ SEF_FIRE ] = new pets::storm_earth_and_fire_pet_t( "fire_spirit", sim, this, true );
    pet.sef[ SEF_STORM ] = new pets::storm_earth_and_fire_pet_t( "storm_spirit", sim, this, true );
    pet.sef[ SEF_EARTH ] = new pets::storm_earth_and_fire_pet_t( "earth_spirit", sim, this, false );
  }
}

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  base_t::init_spells();

  //TALENTS
  // Tier 15 Talents
  talent.chi_burst                   = find_talent_spell( "Chi Burst" );
  talent.eye_of_the_tiger            = find_talent_spell( "Eye of the Tiger" ); // Brewmaster & Windwalker
  talent.chi_wave                    = find_talent_spell( "Chi Wave" );
  // Mistweaver
  talent.zen_pulse                   = find_talent_spell( "Zen Pulse" );
  talent.mana_tea                    = find_talent_spell( "Mana Tea" );

  // Tier 30 Talents
  talent.chi_torpedo                 = find_talent_spell( "Chi Torpedo" );
  talent.tigers_lust                 = find_talent_spell( "Tiger's Lust" );
  talent.celerity                    = find_talent_spell( "Celerity" );

  // Tier 45 Talents
  // Brewmaster
  talent.light_brewing               = find_talent_spell( "Light Brewing" );
  talent.black_ox_brew               = find_talent_spell( "Black Ox Brew" );
  talent.secret_ingredients          = find_talent_spell( "Secret Ingredients" );
  // Windwalker
  talent.energizing_elixir           = find_talent_spell( "Energizing Elixir" );
  talent.ascension                   = find_talent_spell( "Ascension" );
  talent.power_strikes               = find_talent_spell( "Power Strikes" );
  // Mistweaver
  talent.mist_wrap                   = find_talent_spell( "Mist Wrap" );
  talent.cranes_grace                = find_talent_spell(" Crane's Grace" );
  talent.lifecycles                  = find_talent_spell( "Lifecycles" );

  // Tier 60 Talents
  talent.ring_of_peace               = find_talent_spell( "Ring of Peace" );
  talent.summon_black_ox_statue      = find_talent_spell( "Summon Black Ox Statue" ); // Brewmaster
  talent.dizzying_kicks              = find_talent_spell( "Dizzying Kicks" ); // Windwalker
  talent.song_of_chi_ji              = find_talent_spell( "Song of Chi-Ji" ); // Mistweaver
  talent.leg_sweep                   = find_talent_spell( "Leg Sweep" );

  // Tier 75 Talents
  talent.healing_elixirs             = find_talent_spell( "Healing Elixirs" );
  talent.dampen_harm                 = find_talent_spell( "Dampen Harm" );
  talent.diffuse_magic               = find_talent_spell( "Diffuse Magic" );

  // Tier 90 Talents
  talent.rushing_jade_wind           = find_talent_spell( "Rushing Jade Wind" ); // Brewmaster & Windwalker
  // Brewmaster
  talent.invoke_niuzao               = find_talent_spell( "Invoke Niuzao, the Black Ox", "invoke_niuzao" );
  talent.special_delivery            = find_talent_spell( "Special Delivery" );
  // Windwalker
  talent.invoke_xuen                 = find_talent_spell( "Invoke Xuen, the White Tiger", "invoke_xuen" );
  talent.hit_combo                   = find_talent_spell("Hit Combo");
  // Mistweaver
  talent.refreshing_jade_wind        = find_talent_spell( "Refreshing Jade Wind" );
  talent.invoke_chi_ji               = find_talent_spell( "Invoke Chi-Ji, the Red Crane", "invoke_chi_ji" );
  talent.summon_jade_serpent_statue  = find_talent_spell( "Summon Jade Serpent Statue" );

  // Tier 100 Talents
  // Brewmaster
  talent.elusive_dance               = find_talent_spell( "Elusive Dance" );
  talent.fortified_mind              = find_talent_spell( "Fortified Mind" );
  talent.high_tolerance              = find_talent_spell( "High Tolerance" );
  // Windwalker
  talent.chi_orbit                   = find_talent_spell( "Chi Orbit" );
  talent.spinning_dragon_strike      = find_talent_spell( "Spinning Dragon Strike" );
  talent.serenity                    = find_talent_spell( "Serenity" );
  // Mistweaver
  talent.mistwalk                    = find_talent_spell ("Mistwalk" );
  talent.focused_thunder             = find_talent_spell( "Focused Thunder" );
  talent.soothing_elegance           = find_talent_spell( "Soothing Elegance" );

  // Artifact

  // Windwalker
  artifact.acrobatics                 = find_artifact_spell( "Acrobatics" );
  artifact.crosswinds                 = find_artifact_spell( "Crosswind" );
  artifact.dark_skies                 = find_artifact_spell( "Dark Skies" );
  artifact.death_art                  = find_artifact_spell( "Death Art" );
  artifact.fists_of_the_wind          = find_artifact_spell( "Fists of the Wind" );
  artifact.gale_burst                 = find_artifact_spell( "Gale Burst" );
  artifact.good_karma                 = find_artifact_spell( "Good Karma" );
  artifact.healing_winds              = find_artifact_spell( "Healing Winds" );
  artifact.inner_peace                = find_artifact_spell( "Inner Peace" );
  artifact.light_on_your_feet         = find_artifact_spell( "Light on Your Feet" );
  artifact.power_of_a_thousand_cranes = find_artifact_spell( "Power of a Thousand Cranes" );
  artifact.rising_winds               = find_artifact_spell( "Rising Winds" );
  artifact.strike_of_the_skylord      = find_artifact_spell( "Strike of the Skylord" );
  artifact.strength_of_xuen           = find_artifact_spell( "Strength of Xuen" );
  artifact.transfer_the_power         = find_artifact_spell( "Transfer the Power" );
  artifact.tornado_kicks              = find_artifact_spell( "Tornado Kicks" );

  // General Passives
  spec.blackout_kick                 = find_class_spell( "Blackout Kick" );
  spec.crackling_jade_lightning      = find_class_spell( "Crackling Jade Lightning" );
  spec.critical_strikes              = find_specialization_spell( "Critical Strikes" );
  spec.healing_sphere                = find_spell( 125355 );
  spec.effuse                        = find_class_spell( "Effuse" );
  spec.leather_specialization        = find_specialization_spell( "Leather Specialization" );
  spec.legacy_of_the_white_tiger     = find_specialization_spell( "Legacy of the White Tiger" );
  spec.rising_sun_kick               = find_specialization_spell( "Rising Sun Kick" );
  spec.spinning_crane_kick           = find_class_spell( "Spinning Crane Kick" );
  spec.tiger_palm                    = find_class_spell( "Tiger Palm" );
  spec.touch_of_death                = find_specialization_spell( "Touch of Death" );
//  spec.way_of_the_monk_aa_damage     = find_spell( 108977 );
//  spec.way_of_the_monk_aa_speed      = find_spell( 140737 );
  spec.fortifying_brew               = find_class_spell( "Fortifying Brew" );

  // Windwalker Passives
  spec.combo_breaker                 = find_specialization_spell( "Combo Breaker" );
  spec.fists_of_fury                 = find_specialization_spell( "Fists of Fury" );
  spec.flying_serpent_kick           = find_specialization_spell( "Flying Serpent Kick" );
  spec.afterlife                     = find_specialization_spell( "Afterlife" );
  spec.disable                       = find_specialization_spell( "Disable" );
  spec.tigereye_brew                 = find_specialization_spell( "Tigereye Brew" );
  spec.touch_of_karma                = find_specialization_spell( "Touch of Karma" );
  spec.combat_conditioning           = find_specialization_spell( "Combat Conditioning" );
  spec.stance_of_the_fierce_tiger    = find_specialization_spell( "Stance of the Fierce Tiger" );
  spec.storm_earth_and_fire          = find_specialization_spell( "Storm, Earth, and Fire" );
  spec.battle_trance                 = find_specialization_spell( "Battle Trance" );
  spec.windflurry                    = find_specialization_spell( "Windflurry" );
  spec.rising_sun_kick_trinket       = find_spell( 185099 );

  // Brewmaster Passives
  spec.blackout_strike               = find_specialization_spell( "Blackout Strike" );
  spec.desperate_measures            = find_specialization_spell( "Desperate Measures" );
  spec.breath_of_fire                = find_specialization_spell( "Breath of Fire" );
  spec.summon_black_ox_statue        = find_specialization_spell( "Summon Black Ox Statue" );
  spec.ironskin_brew                 = find_specialization_spell( "Ironskin Brew" );
  spec.provoke                       = find_specialization_spell( "Provoke" );
  spec.purifying_brew                = find_specialization_spell( "Purifying Brew" );
  spec.keg_smash                     = find_specialization_spell( "Keg Smash" );
  spec.gift_of_the_ox                = find_specialization_spell( "Gift of the Ox" );
  spec.resolve                       = find_specialization_spell( "Resolve" );
  spec.bladed_armor                  = find_specialization_spell( "Bladed Armor" );
  spec.ferment                       = find_specialization_spell( "Ferment" );
  spec.stagger                       = find_specialization_spell( "Stagger" );
  spec.light_stagger                 = find_spell( 124275 );
  spec.moderate_stagger              = find_spell( 124274 );
  spec.heavy_stagger                 = find_spell( 124273 );

  // Mistweaver Passives
  spec.brewing_mana_tea              = find_specialization_spell( "Brewing: Mana Tea" );
  spec.crane_style_techniques        = find_specialization_spell( "Crane Style Techniques" );
  spec.teachings_of_the_monastery    = find_specialization_spell( "Teachings of the Monastery" );
  spec.teachings_of_the_monastery_buff = find_spell( 202090 );
  spec.renewing_mist                 = find_specialization_spell( "Renewing Mist" );
  spec.renewing_mist_heal            = find_spell( 119611 );
  spec.soothing_mist                 = find_specialization_spell( "Soothing Mist" );
  spec.soothing_mist_statue          = find_spell( 125953 );
  spec.mana_tea                      = find_specialization_spell( "Mana Tea" );
  spec.revival                       = find_specialization_spell( "Revival" );
  spec.summon_jade_serpent_statue    = find_specialization_spell( "Summon Jade Serpent Statue" );
  spec.detonate_chi                  = find_specialization_spell( "Detonate Chi" );
  spec.mana_tea_driver               = find_specialization_spell( "Mana Tea Driver" );
  spec.legacy_of_the_emperor         = find_specialization_spell( "Legacy of the Emperor" );
  spec.soothing_mists_trigger        = find_specialization_spell( "Soothing Mists Trigger" );
  spec.uplift                        = find_specialization_spell( "Uplift" );
  spec.thunder_focus_tea             = find_specialization_spell( "Thunder Focus Tea" );
  spec.life_cocoon                   = find_specialization_spell( "Life Cocoon" );
  spec.enveloping_mist               = find_specialization_spell( "Enveloping Mist" );
  spec.jade_mists                    = find_specialization_spell( "Jade Mists" );
  spec.eminence                      = find_spell( 126890 );
  spec.eminence_statue               = find_spell( 117895 );
  spec.breath_of_the_serpent_heal    = find_spell( 157590 );
  spec.extend_life                   = find_spell( 185158 ); // Tier 18 bonus

  // General
  passives.chi_burst_damage           = find_spell( 148135 );
  passives.chi_burst_heal             = find_spell( 130654 );
  passives.chi_torpedo                = find_spell( 119085 );
  passives.chi_wave_damage            = find_spell( 132467 );
  passives.chi_wave_heal              = find_spell( 132463 );
  passives.eye_of_the_tiger           = find_spell( 196608 );
  passives.healing_elixirs            = find_spell( 122281 ); // talent.healing_elixirs -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() 
  passives.rushing_jade_wind_damage   = find_spell( 148187 );
  passives.spinning_crane_kick_damage = find_spell( 107270 );
  // Brewmaster
  passives.aura_brewmaster_monk       = find_spell( 137023 );
  passives.breath_of_fire_dot         = find_spell( 123725 );
  passives.elusive_brawler            = find_spell( 195630 );
  passives.elusive_dance              = find_spell( 196739 );
  passives.gift_of_the_ox_chance      = find_spell( 124502 );
  passives.gift_of_the_ox_heal        = find_spell( 124507 );
  passives.gift_of_the_ox_summon      = find_spell( 124503 );
  passives.keg_smash_buff             = find_spell( 196720 );
  passives.special_delivery           = find_spell( 196733 );
  passives.stagger_self_damage        = find_spell( 124255 );
  passives.tier17_2pc_tank            = find_spell( 165356 );
  // Mistweaver
  passives.aura_mistweaver_monk       = find_spell( 137024 );
  passives.tier17_2pc_heal            = find_spell( 167732 );
  passives.tier17_4pc_heal            = find_spell( 167717 );
  // Windwalker
  passives.aura_windwalker_monk       = find_spell( 137025 );
  passives.chi_orbit                  = find_spell( 196748 );
  passives.chi_sphere                 = find_spell( 121283 );
  passives.crackling_tiger_lightning  = find_spell( 123996 );
  passives.crackling_tiger_lightning_driver = find_spell( 123999 );
  passives.dizzying_kicks             = find_spell( 196723 );
  passives.hit_combo                  = find_spell( 196741 );
  passives.spinning_dragon_strike     = find_spell( 158221 );
  passives.touch_of_karma_tick        = find_spell( 124280 );
  passives.tier15_2pc_melee           = find_spell( 138311 );
  passives.tier17_4pc_melee           = find_spell( 166603 );

  //MASTERY
  mastery.combo_strikes              = find_mastery_spell( MONK_WINDWALKER );
  mastery.elusive_brawler            = find_mastery_spell( MONK_BREWMASTER );
  mastery.gust_of_mists              = find_mastery_spell( MONK_MISTWEAVER );

  // Sample Data
  sample_datas.stagger_total_damage           = get_sample_data("Total Stagger damage generated");
  sample_datas.stagger_tick_damage            = get_sample_data("Stagger damage that was not purified");
  sample_datas.purified_damage                = get_sample_data("Stagger damage that was purified");
  sample_datas.light_stagger_total_damage     = get_sample_data("Amount of damage purified while at light stagger");
  sample_datas.moderate_stagger_total_damage  = get_sample_data("Amount of damage purified while at moderate stagger");
  sample_datas.heavy_stagger_total_damage     = get_sample_data("Amount of damage purified while at heavy stagger");

  //SPELLS
  if ( talent.healing_elixirs -> ok() )
    active_actions.healing_elixir     = new actions::healing_elixirs_t( *this );

  if (specialization() == MONK_BREWMASTER)
    active_actions.stagger_self_damage = new actions::stagger_self_damage_t( this );
}

// monk_t::init_base ========================================================

void monk_t::init_base_stats()
{
  base_t::init_base_stats();

  base.distance = ( specialization() == MONK_MISTWEAVER ) ? 40 : 3;
  base_gcd = timespan_t::from_seconds( 1.5 );
  if ( spec.stagger -> ok() )
    base_gcd -= timespan_t::from_millis( spec.stagger -> effectN( 11 ).base_value() * -1 ); // Saved as -500 milliseconds
  if ( spec.stance_of_the_fierce_tiger -> ok() )
    base_gcd -= timespan_t::from_millis( spec.stance_of_the_fierce_tiger -> effectN( 6 ).base_value() * -1); // Saved as -500 milliseconds

  resources.base[RESOURCE_CHI] = 4 + talent.ascension -> effectN( 1 ).base_value();
  resources.base[RESOURCE_ENERGY] = 100 + sets.set( MONK_WINDWALKER, T18, B4 ) -> effectN( 2 ).base_value();
  if ( artifact.inner_peace.rank() )
    resources.base[RESOURCE_ENERGY] += artifact.inner_peace.value();

  base_chi_regen_per_second = 0;
  base_energy_regen_per_second = 10.0;

  if ( specialization() != MONK_MISTWEAVER )
    base.attack_power_per_agility = 1.0;
  if ( specialization() == MONK_MISTWEAVER )
    base.spell_power_per_intellect = 1.0;

  // initialize resolve for Berwmaster
  if ( specialization() == MONK_BREWMASTER )
    resolve_manager.init();
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  base_t::init_scaling();

  if ( specialization() != MONK_MISTWEAVER )
  {
    scales_with[STAT_INTELLECT] = false;
    scales_with[STAT_SPELL_POWER] = false;
    scales_with[STAT_AGILITY] = true;
    scales_with[STAT_WEAPON_DPS] = true;
  }
  else
  {
    scales_with[STAT_AGILITY] = false;
    scales_with[STAT_MASTERY_RATING] = false;
    scales_with[STAT_ATTACK_POWER] = false;
    scales_with[STAT_SPIRIT] = true;
  }
  scales_with[STAT_STRENGTH] = false;

  if ( specialization() == MONK_BREWMASTER )
    scales_with[ STAT_BONUS_ARMOR ] = true;

  if ( off_hand_weapon.type != WEAPON_NONE )
    scales_with[STAT_WEAPON_OFFHAND_DPS] = true;
}

// monk_t::init_buffs =======================================================

// monk_t::create_buffs =====================================================

void monk_t::create_buffs()
{
  base_t::create_buffs();

  // General
  buff.chi_torpedo = buff_creator_t( this, "chi_torpedo", passives.chi_torpedo )
    .default_value( passives.chi_torpedo -> effectN( 1 ).percent() );

  buff.fortifying_brew = new buffs::fortifying_brew_t( *this, "fortifying_brew", find_spell( 120954 ) );

  buff.power_strikes = buff_creator_t( this, "power_strikes", talent.power_strikes -> effectN( 1 ).trigger() )
    .default_value( talent.power_strikes -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() );

  buff.rushing_jade_wind = buff_creator_t( this, "rushing_jade_wind", talent.rushing_jade_wind )
    .cd( timespan_t::zero() );

  buff.dampen_harm = buff_creator_t( this, "dampen_harm", talent.dampen_harm )
    .cd( timespan_t::zero() );

  buff.diffuse_magic = buff_creator_t( this, "diffuse_magic", talent.diffuse_magic )
    .default_value( talent.diffuse_magic -> effectN( 1 ).percent() );

  buff.eye_of_the_tiger = buff_creator_t( this, "eye_of_the_tiger", passives.eye_of_the_tiger );

  // Brewmaster
  buff.bladed_armor = buff_creator_t( this, "bladed_armor", spec.bladed_armor )
    .default_value( spec.bladed_armor -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_ATTACK_POWER );

  buff.elusive_brawler = buff_creator_t( this, "elusive_brawler", passives.elusive_brawler )
    .max_stack( static_cast<int>( ceil( 1 / ( mastery.elusive_brawler -> effectN( 1 ).mastery_value() * 8 ) ) ) )
    .add_invalidate( CACHE_DODGE );

  buff.elusive_dance = buff_creator_t(this, "elusive_dance", passives.elusive_dance)
    .default_value( talent.elusive_dance -> effectN( 1 ).percent() ) // 5% per stack
    .max_stack( 3 ) // Cap of 15%
    .add_invalidate( CACHE_DODGE );

  buff.ironskin_brew = buff_creator_t(this, "ironskin_brew", spec.ironskin_brew )
    .default_value( spec.ironskin_brew -> effectN( 1 ).percent() )
    .refresh_behavior( BUFF_REFRESH_EXTEND );

  buff.keg_smash_talent = buff_creator_t( this, "keg_smash", passives.keg_smash_buff )
    .chance( talent.secret_ingredients -> proc_chance() ); 

  buff.gift_of_the_ox = buff_creator_t( this, "gift_of_the_ox" , passives.gift_of_the_ox_summon )
    .duration( passives.gift_of_the_ox_summon -> duration() )
    .refresh_behavior( BUFF_REFRESH_NONE )
    .max_stack( 99 );

  // Mistweaver
  buff.channeling_soothing_mist = buff_creator_t( this, "channeling_soothing_mist", spell_data_t::nil() );

  buff.cranes_zeal = buff_creator_t( this, "cranes_zeal", find_spell( 127722 ) )
    .add_invalidate( CACHE_CRIT );

  buff.mana_tea = buff_creator_t( this, "mana_tea", talent.mana_tea )
    .default_value( talent.mana_tea -> effectN( 1 ).percent() );

  buff.teachings_of_the_monastery = buff_creator_t( this, "teachings_of_the_monastery", spec.teachings_of_the_monastery_buff )
    .default_value( spec.teachings_of_the_monastery_buff -> effectN( 1 ).percent() );

  buff.vital_mists = buff_creator_t( this, "vital_mists", find_spell( 118674 ) ).max_stack( 5 );

  buff.mistweaving = buff_creator_t(this, "mistweaving", passives.tier17_2pc_heal )
    .default_value( passives.tier17_2pc_heal -> effectN( 1 ).percent() )
    .max_stack( 7 )
    .add_invalidate( CACHE_CRIT );

  buff.chi_jis_guidance = buff_creator_t( this, "chi_jis_guidance", passives.tier17_4pc_heal )
    .default_value( passives.tier17_4pc_heal -> effectN( 1 ).percent() ) ;

  // Windwalker
  buff.chi_orbit = buff_creator_t( this, "chi_orbit", talent.chi_orbit )
    .default_value( passives.chi_orbit -> effectN( 1 ).ap_coeff() )
    .max_stack( 4 );

  buff.chi_sphere = buff_creator_t( this, "chi_sphere", passives.chi_sphere )
    .default_value( passives.chi_sphere -> effectN( 1 ).base_value() )
    .max_stack( 8 );

  buff.combo_breaker_bok = buff_creator_t( this, "combo_breaker_bok", find_spell( 116768 ) );

  buff.combo_strikes = buff_creator_t(this, "combo_strikes")
    .duration( timespan_t::from_seconds( 30 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.forceful_winds = buff_creator_t( this, "forceful_winds", passives.tier17_4pc_melee )
    .default_value( passives.tier17_4pc_melee -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_CRIT )
    .add_invalidate( CACHE_SPELL_CRIT );

  buff.hit_combo = buff_creator_t( this, "hit_combo", passives.hit_combo )
    .default_value( passives.hit_combo -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.serenity = buff_creator_t( this, "serenity", talent.serenity )
    .default_value( talent.serenity -> effectN( 2 ).percent() )
    .duration( talent.serenity -> duration() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buff.storm_earth_and_fire = buff_creator_t( this, "storm_earth_and_fire", spec.storm_earth_and_fire )
                              .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                              .cd( timespan_t::zero() );

  buff.swift_as_the_wind = buff_creator_t( this, "swift_as_the_wind", aburaq -> driver() -> effectN( 1 ).trigger() )
    .chance( aburaq -> driver() -> effectN( 1 ).percent() )
    .default_value( aburaq -> driver() -> effectN( 1 ).trigger() -> effectN( 2 ).percent() );

  buff.tigereye_brew = buff_creator_t( this, "tigereye_brew", spec.tigereye_brew )
    .period( timespan_t::zero() ) // Tigereye Brew does not tick, despite what the spelldata implies.
    .duration( spec.tigereye_brew -> duration() + 
      ( artifact.strength_of_xuen.rank() ? timespan_t::from_seconds( artifact.strength_of_xuen.value() ) : timespan_t::zero() ) )
    .default_value( spec.tigereye_brew -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buff.transfer_the_power = buff_creator_t( this, "transfer_the_power", artifact.transfer_the_power.data().effectN( 1 ).trigger() )
    .duration( timespan_t::from_seconds( 20 ) ) // TODO: Get actual duration of buff
    .max_stack( 3 ) // TODO: Get the max stacks
    .default_value( artifact.transfer_the_power.rank() ? artifact.transfer_the_power.percent() : 0 ); 
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  base_t::init_gains();

  gain.black_ox_brew            = get_gain( "black_ox_brew" );
  gain.chi_refund               = get_gain( "chi_refund" );
  gain.power_strikes            = get_gain( "power_strikes" );
  gain.combo_breaker_bok        = get_gain( "combo_breaker_blackout_kick" );
  gain.crackling_jade_lightning = get_gain( "crackling_jade_lightning" );
  gain.energizing_elixir        = get_gain( "energizing_elixir" );
  gain.energy_refund            = get_gain( "energy_refund" );
  gain.keg_smash                = get_gain( "keg_smash" );
  gain.mana_tea                 = get_gain( "mana_tea" );
  gain.renewing_mist            = get_gain( "renewing_mist" );
  gain.serenity                 = get_gain( "serenity" );
  gain.soothing_mist            = get_gain( "soothing_mist" );
  gain.spinning_crane_kick      = get_gain( "spinning_crane_kick" );
  gain.rushing_jade_wind        = get_gain( "rushing_jade_wind" );
  gain.effuse                   = get_gain( "effuse" );
  gain.tier15_2pc_melee         = get_gain( "tier15_2pc_melee" );
  gain.tier16_4pc_melee         = get_gain( "tier16_4pc_melee" );
  gain.tier16_4pc_tank          = get_gain( "tier16_4pc_tank" );
  gain.tier17_2pc_healer        = get_gain( "tier17_2pc_healer" );
  gain.tiger_palm               = get_gain( "tiger_palm" );
  gain.gift_of_the_ox           = get_gain( "gift_of_the_ox" );
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.combo_breaker_bok          = get_proc( "combo_breaker_bok" );
  proc.eye_of_the_tiger           = get_proc( "eye_of_the_tiger" );
  proc.mana_tea                   = get_proc( "mana_tea" );
  proc.tier15_2pc_melee           = get_proc( "tier15_2pc_melee" );
  proc.tier15_4pc_melee           = get_proc( "tier15_4pc_melee" );
  proc.tier17_4pc_heal            = get_proc( "tier17_2pc_heal" );
}

// druid_t::has_t18_class_trinket ===========================================

bool monk_t::has_t18_class_trinket() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:   return eluding_movements != nullptr;
    case MONK_MISTWEAVER:   return soothing_breeze != nullptr;
    case MONK_WINDWALKER:   return furious_sun != nullptr;
    default:                return false;
  }
}

// monk_t::reset ============================================================

void monk_t::reset()
{
  base_t::reset();
}

// monk_t::regen (brews/teas)================================================

void monk_t::regen( timespan_t periodicity )
{
  resource_e resource_type = primary_resource();

  if ( resource_type == RESOURCE_MANA )
  {
    //TODO: add mana tea here
  }

  base_t::regen( periodicity );
}

// monk_t::interrupt =========================================================

void monk_t::interrupt()
{
  player_t::interrupt();
}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( attribute_e attr ) const
{
  switch ( specialization() )
  {
  case MONK_MISTWEAVER:
    if ( attr == ATTR_INTELLECT )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  case MONK_WINDWALKER:
    if ( attr == ATTR_AGILITY )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  case MONK_BREWMASTER:
    if ( attr == ATTR_STAMINA )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  default:
    break;
  }

  return 0.0;
}

// monk_t::has_stagger ====================================================

bool monk_t::has_stagger()
{
  return active_actions.stagger_self_damage -> stagger_ticking();
}
double monk_t::clear_stagger()
{
  return active_actions.stagger_self_damage -> clear_all_damage();
}

// monk_t::composite_attack_speed =========================================

// monk_t::composite_melee_crit ============================================

double monk_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  if ( buff.mistweaving -> check() )
    crit += buff.mistweaving -> stack_value();

  if ( buff.cranes_zeal -> check() )
    crit += buff.cranes_zeal -> data().effectN( 1 ).percent();

  return crit;
}

// monk_t::composte_melee_crit_multiplier===================================

double monk_t::composite_melee_crit_multiplier() const
{
  double crit = player_t::composite_melee_crit_multiplier();

  if ( buff.forceful_winds -> check() )
    crit += buff.forceful_winds -> value();

  return crit;
}

// monk_t::composite_spell_crit ============================================

double monk_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  if ( buff.cranes_zeal -> check() )
  {
    crit += buff.cranes_zeal -> data().effectN( 1 ).percent();
  }

  return crit;
}

// monk_t::composte_spell_crit_multiplier===================================

double monk_t::composite_spell_crit_multiplier() const
{
  double crit = player_t::composite_spell_crit_multiplier();

  if ( buff.forceful_winds -> check() )
    crit += buff.forceful_winds -> value();

  return crit;
}

// monk_t::composite_player_multiplier =====================================

double monk_t::composite_player_multiplier( school_e school ) const
{
  double m = base_t::composite_player_multiplier( school );

  m *= 1.0 + spec.stance_of_the_fierce_tiger -> effectN( 3 ).percent();

  if ( buff.combo_strikes -> up() )
  {
    m *= 1.0 + cache.mastery() * mastery.combo_strikes -> effectN( 1 ).mastery_value();
    m *= 1.0 + buff.hit_combo -> value();
  }

  if ( buff.tigereye_brew -> up() )
    m *= 1.0 + buff.tigereye_brew -> value();

  if ( buff.serenity -> up() )
    m *= 1.0 + buff.serenity -> value();

  if ( buff.storm_earth_and_fire -> up() )
    m *= 1.0 + spec.storm_earth_and_fire -> effectN( 1 ).percent();

  // Brewmaster Tier 18 (WoD 6.2) trinket effect is in use, Elusive Brew increases damage based on spell data of the special effect.
/*  if ( eluding_movements )
  {
    m *= 1.0 + ( eluding_movements -> driver() -> effectN( 2 ).average( eluding_movements -> item ) / 100 );
  }
*/
  return m;
}

// monk_t::composite_attribute_multiplier =====================================

double monk_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double cam = base_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA )
    cam *= 1.0 + spec.stagger ->effectN( 6 ).percent();

  return cam;
}

// monk_t::composite_player_heal_multiplier ==================================

double monk_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = base_t::composite_player_heal_multiplier( s );

  if ( buff.tigereye_brew -> up() )
    m *= 1.0 + spec.tigereye_brew -> effectN( 2 ).percent();

  if ( buff.serenity -> up() )
    m *= 1.0 + talent.serenity -> effectN( 3 ).percent();

  if ( buff.storm_earth_and_fire -> up() )
    m *= 1.0 + spec.storm_earth_and_fire -> effectN( 2 ).percent();

  return m;
}

// monk_t::composite_melee_expertise ========================================

double monk_t::composite_melee_expertise( const weapon_t* weapon ) const
{
  double e = base_t::composite_melee_expertise( weapon );

  e += spec.stagger -> effectN( 12 ).percent();

  return e;
}

// monk_t::composite_melee_attack_power ==================================

double monk_t::composite_melee_attack_power() const
{
  if ( specialization() == MONK_MISTWEAVER )
    return composite_spell_power( SCHOOL_MAX );

  double ap = player_t::composite_melee_attack_power();

  ap += buff.bladed_armor -> default_value * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// monk_t::composite_attack_power_multiplier() ==========================

double monk_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.elusive_brawler -> ok() )
    ap *= 1.0 + cache.mastery() * mastery.elusive_brawler -> effectN( 2 ).mastery_value();

  return ap;
}


// monk_t::composite_parry ==============================================

double monk_t::composite_parry() const
{
  double p = base_t::composite_parry();

  return p;
}

// monk_t::composite_dodge ==============================================

double monk_t::composite_dodge() const
{
  double d = base_t::composite_dodge();

  if ( buff.elusive_brawler -> up() )
    d += buff.elusive_brawler -> current_stack * ( cache.mastery() * mastery.elusive_brawler -> effectN( 1 ).mastery_value() );

  if ( buff.elusive_dance -> up() )
    d += buff.elusive_dance -> stack_value();

  if ( artifact.light_on_your_feet.rank() )
    d += artifact.light_on_your_feet.percent();

  return d;
}

// monk_t::composite_crit_avoidance =====================================

double monk_t::composite_crit_avoidance() const
{
  double c = base_t::composite_crit_avoidance();

  c += spec.stagger -> effectN( 8 ).percent();

  return c;
}

// monk_t::composite_rating_multiplier =================================

double monk_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = base_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_MELEE_CRIT:
    return m *= 1.0 + spec.ferment -> effectN( 1 ).percent();
  case RATING_SPELL_CRIT:
    return m *= 1.0 + spec.ferment -> effectN( 1 ).percent();
  case RATING_MULTISTRIKE:
    m *= 1.0 + spec.battle_trance -> effectN( 1 ).percent();
    m *= 1.0 + spec.jade_mists -> effectN( 1 ).percent();
    return m;
    break;
  default:
    break;
  }

  return m;
}

// monk_t::composite_armor_multiplier ===================================

double monk_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  a += spec.stagger -> effectN( 14 ).percent();

  return a;
}

// monk_t::invalidate_cache ==============================================

void monk_t::invalidate_cache( cache_e c )
{
  base_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_SPELL_POWER:
    if ( specialization() == MONK_MISTWEAVER )
      player_t::invalidate_cache( CACHE_ATTACK_POWER );
    break;
  case CACHE_BONUS_ARMOR:
    if ( spec.bladed_armor -> ok() )
      player_t::invalidate_cache( CACHE_ATTACK_POWER );
    break;
  default: break;
  }
}


// monk_t::create_options ===================================================

void monk_t::create_options()
{
  base_t::create_options();

  add_option( opt_int( "initial_chi", user_options.initial_chi ) );
  add_option( opt_float( "goto_throttle", user_options.goto_throttle ) );
  add_option( opt_float ( "ppm_below_35_percent_dm", user_options.ppm_below_35_percent_dm ) );
  add_option( opt_float ( "ppm_below_50_percent_dm", user_options.ppm_below_50_percent_dm ) );
}

// monk_t::copy_from =========================================================

void monk_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  monk_t* source_p = debug_cast<monk_t*>( source );

  user_options = source_p -> user_options;
}

// monk_t::primary_resource =================================================

resource_e monk_t::primary_resource() const
{
  if ( specialization() == MONK_MISTWEAVER )
    return RESOURCE_MANA;

  return RESOURCE_ENERGY;
}

// monk_t::primary_role =====================================================

role_e monk_t::primary_role() const
{
  if ( base_t::primary_role() == ROLE_DPS )
    return ROLE_HYBRID;

  if ( base_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( base_t::primary_role() == ROLE_HEAL )
    return ROLE_HYBRID;//To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == MONK_BREWMASTER )
    return ROLE_TANK;

  if ( specialization() == MONK_MISTWEAVER )
    return ROLE_HYBRID;//To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == MONK_WINDWALKER )
    return ROLE_DPS;

  return ROLE_HYBRID;
}

// monk_t::convert_hybrid_stat ==============================================

stat_e monk_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
    switch ( specialization() )
    {
    case MONK_MISTWEAVER:
      return STAT_INTELLECT;
    case MONK_BREWMASTER:
    case MONK_WINDWALKER:
      return STAT_AGILITY;
    default:
      return STAT_NONE;
    }
  case STAT_AGI_INT:
    if ( specialization() == MONK_MISTWEAVER )
      return STAT_INTELLECT;
    else
      return STAT_AGILITY;
  case STAT_STR_AGI:
    return STAT_AGILITY;
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
    if ( specialization() == MONK_MISTWEAVER )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
    if ( specialization() == MONK_BREWMASTER )
      return s;
    else
      return STAT_NONE;
  default: return s;
  }
}

// monk_t::pre_analyze_hook  ================================================

void monk_t::pre_analyze_hook()
{
  base_t::pre_analyze_hook();

  if ( stats_t* zen_sphere = find_stats( "zen_sphere" ) )
  {
    if ( stats_t* zen_sphere_dmg = find_stats( "zen_sphere_dmg" ) )
    {
      zen_sphere_dmg -> total_execute_time = zen_sphere -> total_execute_time;
      zen_sphere_dmg -> num_executes       = zen_sphere -> num_executes;
    }
  }
}

// monk_t::energy_regen_per_second ==========================================

double monk_t::energy_regen_per_second() const
{
  double r = base_t::energy_regen_per_second();

  r *= 1.0 + talent.ascension -> effectN( 3 ).percent();

  return r;
}

// monk_t::combat_begin ====================================================

void monk_t::combat_begin()
{
  base_t::combat_begin();

  resources.current[RESOURCE_CHI] = 0;

  if ( specialization() == MONK_WINDWALKER && !buffs.fierce_tiger_movement_aura -> up() )
  {
    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[i];
      if ( p -> type == PLAYER_GUARDIAN )
        continue;

      p -> buffs.fierce_tiger_movement_aura -> trigger();
    }
  }

  if ( spec.resolve -> ok() )
    resolve_manager.start();

  if ( talent.chi_orbit -> ok() )
  {
    // If Chi Orbit, start out with max stacks
    buff.chi_orbit -> trigger( 4 );
    new ( *sim ) chi_orbit_event_t( *this, timespan_t::zero() );
  }

  if ( talent.power_strikes -> ok() )
  {
    new ( *sim ) power_strikes_event_t( *this, timespan_t::zero() );
  }

  if ( spec.bladed_armor -> ok() )
    buff.bladed_armor -> trigger();
}

// monk_t::assess_damage ====================================================

void monk_t::assess_damage(school_e school,
  dmg_e    dtype,
  action_state_t* s)
{
  buff.fortifying_brew -> up();
  if ( specialization() == MONK_BREWMASTER )
  {
    // Brewmaster Tier 18 (WoD 6.2) trinket effect is in use, adjust Elusive Brew proc chance based on spell data of the special effect.
/*    if ( eluding_movements )
    {
    {
      double bm_trinket_proc = eluding_movements -> driver() -> effectN( 1 ).average( eluding_movements -> item );

      if ( health_percentage() < bm_trinket_proc )
      {
//        TODO: Figure out how to get trigger_brew to work from here
        if ( rng().roll( ( bm_trinket_proc / 100 ) / 2 ) )
          buff.elusive_brew_stacks -> trigger( 1 );
      }
    }
*/

    if ( s -> result == RESULT_DODGE )
    {
      if ( buff.elusive_brawler -> up() )
        buff.elusive_brawler -> expire();

      if ( sets.has_set_bonus( MONK_BREWMASTER, T17, B2 ) )
        resource_gain( RESOURCE_ENERGY, passives.tier17_2pc_tank -> effectN( 1 ).base_value(), gain.energy_refund );
    }

    if ( action_t::result_is_hit( s -> result ) && s -> action -> id != passives.stagger_self_damage -> id() )
    {
      // trigger the mastery if the player gets hit by a physical attack; but not from stagger
      if ( school == SCHOOL_PHYSICAL )
        buff.elusive_brawler -> trigger();

      // TODO: Check if 35% chance is baseline and increased by HP percent from there
      if ( rng().roll( fmax( passives.gift_of_the_ox_chance -> effectN( 1 ).percent(), 1 - fmax( resources.pct( RESOURCE_HEALTH ), 0 ) ) ) )
        buff.gift_of_the_ox -> trigger();
    }
  }

  base_t::assess_damage(school, dtype, s);
}

// monk_t::target_mitigation ====================================================

void monk_t::target_mitigation( school_e school,
                                dmg_e    dt,
                                action_state_t* s )
{
  // Stagger is not reduced by damage mitigation effects
  if ( s -> action -> id == passives.stagger_self_damage -> id() )
  {
    // Register the tick then exit
    sample_datas.stagger_tick_damage -> add( s -> result_amount );
    return;
  }

  // Passive sources (Sturdy Ox)
  if ( school != SCHOOL_PHYSICAL && specialization() == MONK_BREWMASTER )
    // TODO: Double Check that Brewmasters mitigate 15% of Magical Damage
    s -> result_amount *= 1.0 + spec.stagger -> effectN( 5 ).percent();

  // Damage Reduction Cooldowns
  if ( buff.fortifying_brew -> up() )
    s -> result_amount *= 1.0 - spec.fortifying_brew -> effectN( 1 ).percent();

  // Dampen Harm // Currently reduces hits below 15% hp as well
  double dampen_health = max_health() * buff.dampen_harm -> data().effectN( 1 ).percent();
  double dampen_result_amount = s -> result_amount;
  if ( buff.dampen_harm -> up() && dampen_result_amount >= dampen_health )
  {
    s -> result_amount *= 1.0 - buff.dampen_harm -> data().effectN( 2 ).percent(); // Dampen Harm reduction is stored as +50
    buff.dampen_harm -> decrement(); // A stack will only be removed if the reduction was applied.
  }

  // Diffuse Magic
  if ( buff.diffuse_magic -> up() && school != SCHOOL_PHYSICAL )
    s -> result_amount *= 1.0 + buff.diffuse_magic -> default_value; // Stored as -90%

  player_t::target_mitigation(school, dt, s);
}

// monk_t::assess_damage_imminent_pre_absorb ==============================

void monk_t::assess_damage_imminent_pre_absorb( school_e school,
                                                dmg_e    dtype,
                                                action_state_t* s )
{
  base_t::assess_damage_imminent_pre_absorb( school, dtype, s );

  if ( specialization() == MONK_BREWMASTER )
    return;

  // Stagger damage can't be staggered!
  if ( s -> action -> id == passives.stagger_self_damage -> id() )
    return;

  // Stagger Calculation
  double stagger_dmg = 0;

  if ( s -> result_amount > 0 )
  {
    if ( school == SCHOOL_PHYSICAL )
      stagger_dmg += s -> result_amount * stagger_pct();

//    if ( school != SCHOOL_PHYSICAL && talent.soul_dance )
//      stagger_dmg += s -> result_amount * ( stagger_pct() * talent.soul_dance -> effectN( 1 ).percent() );

    s -> result_amount -= stagger_dmg;
    s -> result_mitigated -= stagger_dmg;
  }
  // Hook up Stagger Mechanism
  if ( stagger_dmg > 0 )
  {
    sample_datas.stagger_total_damage -> add( stagger_dmg );
    residual_action::trigger( active_actions.stagger_self_damage, this, stagger_dmg );
  }
}

// Brewmaster Pre-Combat Action Priority List ============================

void monk_t::apl_pre_brewmaster()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  // Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    if ( true_level > 90 )
      pre -> add_action( "flask,type=greater_draenic_agility_flask" );
    else if ( true_level >= 85 )
      pre -> add_action( "flask,type=spring_blossoms" );
    else if ( true_level >= 80 )
      pre -> add_action( "flask,type=the_winds" );
    else if ( true_level >= 75 )
      pre -> add_action( "flask,type=endless_rage" );
    else
      pre -> add_action( "flask,type=relentless_assault" );
  }

  if ( sim -> allow_food && level() >= 80 )
  {
    if ( level() > 90)
      pre -> add_action( "food,type=sleeper_sushi" );
    else if ( level() >= 80 )
      pre -> add_action( "food,type=skewered_eel" );
    else if ( level() >= 70 )
      pre -> add_action( "food,type=blackened_dragonfin" );
    else
      pre -> add_action( "food,type=warp_burger" );
  }

  pre -> add_action( "legacy_of_the_white_tiger,if=!aura.str_agi_int.up|!aura.critical_strike.up" );
  pre -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( sim -> allow_potions && true_level >= 80 )
  {
    if ( true_level >= 90 )
      pre -> add_action( "potion,name=draenic_armor" );
    else if ( true_level >= 85 )
      pre -> add_action( "potion,name=virmens_bite" );
    else
      pre -> add_action( "potion,name=tolvir" );
  }

  pre -> add_action( "dampen_harm" );
}

// Windwalker Pre-Combat Action Priority List ==========================

void monk_t::apl_pre_windwalker()
{
  action_priority_list_t* pre = get_action_priority_list("precombat");
//  std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

  if ( sim -> allow_flasks )
  {
    // Flask
    if ( true_level > 90 )
      pre -> add_action( "flask,type=greater_draenic_agility_flask" );
    else if ( true_level >= 85 )
      pre -> add_action( "flask,type=spring_blossoms" );
    else if ( true_level >= 80 )
      pre -> add_action( "flask,type=the_winds" );
    else if ( true_level >= 75 )
      pre -> add_action( "flask,type=endless_rage" );
    else
      pre->add_action( "flask,type=relentless_assault" );
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( level() > 90 )
      pre -> add_action( "food,type=salty_squid_roll" );
    else if ( level() >= 85 )
      pre -> add_action( "food,type=sea_mist_rice_noodles" );
    else if ( level() >= 80 )
      pre -> add_action( "food,type=skewered_eel" );
    else if ( level() >= 70 )
      pre -> add_action( "food,type=blackened_dragonfin" );
    else
      pre -> add_action( "food,type=warp_burger" );
  }

  pre -> add_action( "legacy_of_the_white_tiger,if=!aura.str_agi_int.up|!aura.critical_strike.up" );
  pre -> add_action( "snapshot_stats" );

  if ( sim -> allow_potions )
  {
    // Prepotion
    if ( true_level > 90 )
      pre -> add_action( "potion,name=draenic_agility" );
    else if ( true_level >= 85 )
      pre -> add_action( "potion,name=virmens_bite" );
    else if ( true_level >= 80 )
      pre -> add_action( "potion,name=tolvir" );
    else
      pre -> add_action( "potion,name=potion_of_speed" );
  }
}

// Mistweaver Pre-Combat Action Priority List ==========================

void monk_t::apl_pre_mistweaver()
{
  action_priority_list_t* pre = get_action_priority_list("precombat");
//  std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

  if ( sim -> allow_flasks )
  {
    if ( true_level > 90 )
      pre -> add_action( "flask,type=greater_draenic_intellect_flask" );
    // Flask
    else if ( true_level >= 85 )
      pre -> add_action( "flask,type=warm_sun" );
    else if ( true_level > 80 )
      pre -> add_action( "flask,type=draconic_mind" );
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( level() > 90 )
      pre -> add_action( "food,type=salty_squid_roll" );
    else if ( level() >= 85 )
      pre -> add_action( "food,type=mogu_fish_stew" );
    else if ( level() > 80 )
      pre -> add_action( "food,type=seafood_magnifique_feast" );
  }

  pre -> add_action( "legacy_of_the_emperor,if=!aura.str_agi_int.up" );
  pre -> add_action( "snapshot_stats" );

  if ( sim -> allow_potions )
  {
    // Prepotion
    if ( true_level > 90 )
      pre -> add_action( "potion,name=draenic_intellect_potion" );
    else if ( true_level >= 85 )
      pre -> add_action( "potion,name=jade_serpent_potion" );
    else if ( true_level > 80 )
      pre -> add_action( "potion,name=volcanic_potion" );
  }
}

// Brewmaster Combat Action Priority List =========================

void monk_t::apl_combat_brewmaster()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* st = get_action_priority_list( "st" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );

  def -> add_action( "auto_attack" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      def -> add_action( racial_actions[i] + ",if=chi.max-chi>=1&energy<=40" );
    else
      def -> add_action( racial_actions[i] + ",if=energy<=40" );
  }

  def -> add_action( "chi_sphere,if=talent.power_strikes.enabled&buff.chi_sphere.react&chi<4" );
  def -> add_talent( this, "Chi Brew", "if=(chi<1&stagger.heavy)|(chi<2)" );
  def -> add_action( this, "Gift of the Ox", "if=buff.gift_of_the_ox.react&incoming_damage_1500ms" );
  def -> add_talent( this, "Diffuse Magic", "if=incoming_damage_1500ms&buff.fortifying_brew.down" );
  def -> add_talent( this, "Dampen Harm", "if=incoming_damage_1500ms&buff.fortifying_brew.down" );
  def -> add_action( this, "Fortifying Brew", "if=incoming_damage_1500ms&(buff.dampen_harm.down|buff.diffuse_magic.down)" );

  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      def -> add_action( "use_item,name=" + items[i].name_str + ",if=incoming_damage_1500ms&(buff.dampen_harm.down|buff.diffuse_magic.down)&buff.fortifying_brew.down" );
  }

//  def -> add_action( "invoke_niuzao,if=talent.invoke_niuzao.enabled&target.time_to_die>15&buff.serenity.down" );
  def -> add_talent( this, "Serenity", "if=talent.serenity.enabled&cooldown.keg_smash.remains>6" );
  
  if ( sim -> allow_potions )
  {
    if ( true_level >= 90 )
      def -> add_action( "potion,name=draenic_armor,if=(buff.fortifying_brew.down&(buff.dampen_harm.down|buff.diffuse_magic.down))" );
    else if ( true_level >= 85 )
      def -> add_action( "potion,name=virmens_bite,if=(buff.fortifying_brew.down&(buff.dampen_harm.down|buff.diffuse_magic.down))" );
  }

  def -> add_action( "call_action_list,name=st,if=active_enemies<3" );
  def -> add_action( "call_action_list,name=aoe,if=active_enemies>=3" );

  st -> add_action( this, "Purifying Brew", "if=stagger.heavy" );
  st -> add_action( this, "Purifying Brew", "if=buff.serenity.up" );
  st -> add_action( this, "Purifying Brew", "if=stagger.moderate" );
  st -> add_talent( this, "Black Ox Brew" );
  st -> add_action( this, "Keg Smash" );
  st -> add_action( this, "Blackout Kick" );  
  st -> add_talent( this, "Chi Burst", "if=energy.time_to_max>2&buff.serenity.down" );
  st -> add_talent( this, "Chi Wave", "if=energy.time_to_max>2&buff.serenity.down" );
  st -> add_action( this, "Tiger Palm", "if=cooldown.keg_smash.remains>=gcd&(energy+(energy.regen*(cooldown.keg_smash.remains)))>=80");

  aoe -> add_action( this, "Purifying Brew", "if=stagger.heavy" );
  aoe -> add_action( this, "Purifying Brew", "if=buff.serenity.up" );
  aoe -> add_action( this, "Purifying Brew", "if=stagger.moderate" );
  aoe -> add_talent( this, "Black Ox Brew" );
  aoe -> add_action( this, "Keg Smash" );
  aoe -> add_action( this, "Breath of Fire", "if=debuff.keg_smash.up" );  
  aoe -> add_talent( this, "Rushing Jade Wind" );
  aoe -> add_talent( this, "Chi Burst", "if=energy.time_to_max>2&buff.serenity.down" );
  aoe -> add_talent( this, "Chi Wave", "if=energy.time_to_max>2&buff.serenity.down" );
  aoe -> add_action( this, "Tiger Palm", "if=cooldown.keg_smash.remains>=gcd&(energy+(energy.regen*(cooldown.keg_smash.remains)))>=80" );
}

// Windwalker Combat Action Priority List ===============================

void monk_t::apl_combat_windwalker()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* opener = get_action_priority_list("opener");
  action_priority_list_t* st = get_action_priority_list("st");
  action_priority_list_t* aoe_norjw = get_action_priority_list("aoe_norjw");
  action_priority_list_t* aoe_rjw = get_action_priority_list("aoe_rjw");

  def -> add_action( "auto_attack" );

  def -> add_action( "invoke_xuen" );

// TODO: Will activate these lines once Storm, Earth, and Fire is finished; too spammy right now
  def -> add_action( "storm_earth_and_fire,target=2,if=debuff.storm_earth_and_fire_target.down" );
  def -> add_action( "storm_earth_and_fire,target=3,if=debuff.storm_earth_and_fire_target.down" );
  def -> add_action( "chi_orbit,if=talent.chi_orbit.enabled" );
  def -> add_action( "call_action_list,name=opener,if=talent.serenity.enabled&talent.chi_brew.enabled&cooldown.fists_of_fury.up&time<20" );
  def -> add_action( "chi_sphere,if=talent.power_strikes.enabled&buff.chi_sphere.react&chi<chi.max" );

  if ( sim -> allow_potions )
  {
    if ( true_level == 100 )
      def -> add_action( "potion,name=draenic_agility,if=buff.serenity.up|(!talent.serenity.enabled&(trinket.proc.agility.react|trinket.proc.multistrike.react))|buff.bloodlust.react|target.time_to_die<=60" );
    else if ( true_level >= 85 )
      def -> add_action( "potion,name=virmens_bite,if=buff.bloodlust.react|target.time_to_die<=60" );
  }

  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      def -> add_action( "use_item,name=" + items[i].name_str + ",if=buff.tigereye_brew.up|target.time_to_die<18" );
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      def -> add_action( racial_actions[i] + ",if=chi.max-chi>=1&(buff.tigereye_brew_use.up|target.time_to_die<18)" );
    else
      def -> add_action( racial_actions[i] + ",if=buff.tigereye_brew.up|target.time_to_die<18" );
  }

  def -> add_talent( this, "Chi Brew", "if=chi.max-chi>=2&((charges=1&recharge_time<=10)|charges=2|target.time_to_die<charges*10)&buff.tigereye_brew.stack<=16" );
  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down" );
  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down&buff.serenity.up" );
//  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down&buff.spirit_shift.up" );
//  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down&trinket.proc.agility.react" );
//  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down&trinket.proc.versatility.react" );
//  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down&trinket.proc.crit.react" );
  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down&cooldown.fists_of_fury.up&chi>=3" );
  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down&chi>=2&target.time_to_die<40" );
  def -> add_action( this, "Touch of Death" );
  def -> add_talent( this, "Serenity", "if=chi>=2" );
  def -> add_action( this, "Fists of Fury", "if=energy.time_to_max>cast_time&!buff.serenity.up" );
  def -> add_talent( this, "Spinning Dragon Strike", "if=energy.time_to_max>cast_time" );
  
  def -> add_action( "call_action_list,name=st,if=active_enemies<3&(level<100" );
  def -> add_action( "call_action_list,name=aoe_norjw,if=active_enemies>=3&!talent.rushing_jade_wind.enabled" );
  def -> add_action( "call_action_list,name=aoe_rjw,if=active_enemies>=3&talent.rushing_jade_wind.enabled" );

  // Single Target & Non-Chi Explosion Cleave
  st -> add_action( this, "Blackout Kick", "if=set_bonus.tier18_2pc=1&buff.combo_breaker_bok.react" );
  st -> add_action( this, "Rising Sun Kick" );
  st -> add_action( this, "Blackout Kick", "if=buff.combo_breaker_bok.react|buff.serenity.up" );
  st -> add_talent( this, "Chi Wave", "if=energy.time_to_max>2&buff.serenity.down" );
  st -> add_talent( this, "Chi Burst", "if=energy.time_to_max>2&buff.serenity.down" );
  st -> add_action( this, "Blackout Kick", "if=chi.max-chi<2" );
  st -> add_action( this, "Tiger Palm", "if=chi.max-chi>=2" );

  // AoE Non-Rushing Jade Wind
  aoe_norjw -> add_talent( this, "Chi Wave", "if=energy.time_to_max>2&buff.serenity.down" );
  aoe_norjw -> add_talent( this, "Chi Burst", "if=energy.time_to_max>2&buff.serenity.down" );
  aoe_norjw -> add_action( this, "Spinning Crane Kick", "if=buff.combo_breaker_bok.react|buff.serenity.up" );
  aoe_norjw -> add_action( this, "Spinning Crane Kick", "if=chi.max-chi<2&cooldown.fists_of_fury.remains>3" );
  aoe_norjw -> add_action( this, "Tiger Palm", "if=chi.max-chi>=2" );

  // AoE Rushing Jade Wind
  aoe_rjw -> add_talent( this, "Rushing Jade Wind" );
  aoe_rjw -> add_talent( this, "Chi Wave", "if=energy.time_to_max>2&buff.serenity.down" );
  aoe_rjw -> add_talent( this, "Chi Burst", "if=energy.time_to_max>2&buff.serenity.down" );
  aoe_rjw -> add_action( this, "Blackout Kick", "if=buff.combo_breaker_bok.react|buff.serenity.up" );
  aoe_rjw -> add_action( this, "Blackout Kick", "if=chi.max-chi<2&cooldown.fists_of_fury.remains>3" );
  aoe_rjw -> add_action( this, "Tiger Palm", "if=chi.max-chi>=2" );

  // Chi Brew & Serenity Opener
  opener -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew.down" );
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      opener -> add_action( racial_actions[i] + ",if=buff.tigereye_brew.up&chi.max-chi>=1" );
    else
      opener -> add_action( racial_actions[i] + ",if=buff.tigereye_brew.up" );
  }
  opener -> add_action( this, "Fists of Fury", "if=buff.serenity.up&buff.serenity.remains<1.5" );
  for (int i = 0; i < num_items; i++)
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      opener -> add_action( "use_item,name=" + items[i].name_str );
  }
  opener -> add_action( this, "Rising Sun Kick" );
  opener -> add_action( this, "Blackout Kick", "if=chi.max-chi<=1&cooldown.chi_brew.up|buff.serenity.up" );
  opener -> add_talent( this, "Chi Brew", "if=chi.max-chi>=2" );
  opener -> add_talent( this, "Serenity", "if=chi.max-chi<=2" );
  opener -> add_action( this, "Tiger Palm", "if=chi.max-chi>=2&!buff.serenity.up" );
}

// Mistweaver Combat Action Priority List ==================================

void monk_t::apl_combat_mistweaver()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  action_priority_list_t* def = get_action_priority_list("default");
  action_priority_list_t* st = get_action_priority_list( "st" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );

  def -> add_action( "auto_attack" );
  def -> add_action( "invoke_xuen" );
  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      def -> add_action( "use_item,name=" + items[i].name_str );
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      def -> add_action( racial_actions[i] + ",if=chi.max-chi>=1&target.time_to_die<18" );
    else
      def -> add_action( racial_actions[i] + ",if=target.time_to_die<18" );
  }


  if ( sim -> allow_potions )
  {
    if ( true_level == 100 )
      def -> add_action( "potion,name=draenic_intellect,if=buff.bloodlust.react|target.time_to_die<=60" );
    else if ( true_level >= 85 )
      def -> add_action( "potion,name=jade_serpent_potion,if=buff.bloodlust.react|target.time_to_die<=60" );
  }

  def -> add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
  def -> add_action( "call_action_list,name=st,if=active_enemies<3" );
  def -> add_action( "chi_sphere,if=buff.chi_sphere.react" );


  st -> add_action( this, "Rising Sun Kick", "if=buff.teachings_of_the_monastery.up" );
  st -> add_action( this, "Blackout Kick", "if=buff.teachings_of_the_monastery.up" );
  st -> add_talent( this, "Chi Wave" );
  st -> add_talent( this, "Chi Burst" );
  st -> add_action( this, "Tiger Palm", "if=buff.teachings_of_the_monastery.down" );

  aoe -> add_action( this, "Spinning Crane Kick", "if=!talent.refreshing_jade_wind.enabled" );
  aoe -> add_talent( this, "Refreshing Jade Wind" );
  aoe -> add_talent( this, "Chi Burst" );
  aoe -> add_action( this, "Blackout Kick" );
  aoe -> add_action( this, "Tiger Palm", "if=talent.rushing_jade_wind.enabled" );
}

// monk_t::init_actions =====================================================

void monk_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }
  if (  main_hand_weapon.group() == WEAPON_2H && off_hand_weapon.group() == WEAPON_1H )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has a 1-Hand weapon equipped in the Off-Hand while a 2-Hand weapon is equipped in the Main-Hand.", name() );
    quiet = true;
    return;
  }
  if ( specialization() == MONK_BREWMASTER && off_hand_weapon.group() == WEAPON_1H )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has a Brewmaster and has equipped a 1-Hand weapon equipped in the Off-Hand when they are unable to dual weld.", name() );
    quiet = true;
    return;
  }
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  // Precombat
  switch ( specialization() )
  {
  case MONK_BREWMASTER:
    apl_pre_brewmaster();
    break;
  case MONK_WINDWALKER:
    apl_pre_windwalker();
    break;
  case MONK_MISTWEAVER:
    apl_pre_mistweaver();
    break;
  default: break;
  }

  // Combat
  switch ( specialization() )
  {
  case MONK_BREWMASTER:
    apl_combat_brewmaster();
    break;
  case MONK_WINDWALKER:
    apl_combat_windwalker();
    break;
  case MONK_MISTWEAVER:
    apl_combat_mistweaver();
    break;
  default:
    add_action( "Tiger Palm" );
    break;
  }
  use_default_action_list = true;

  base_t::init_action_list();
}

// monk_t::stagger_pct ===================================================

double monk_t::stagger_pct()
{
  double stagger = 0.0;

  if ( specialization() == MONK_BREWMASTER ) // no stagger when not in Brewmaster Specialization
  {
    stagger += spec.stagger -> effectN( 9 ).percent(); //TODO: Effect says 10 but tooltip say 6%; double check

    if ( talent.high_tolerance -> ok() )
      stagger += talent.high_tolerance -> effectN( 1 ).percent();

    if ( specialization() == MONK_BREWMASTER && buff.fortifying_brew -> check() )
      stagger += spec.fortifying_brew -> effectN( 1 ).percent();

    if ( buff.ironskin_brew -> up() )
      stagger += buff.ironskin_brew -> value();
  }

  return stagger;
}

// monk_t::current_stagger_tick_dmg ==================================================

double monk_t::current_stagger_tick_dmg()
{
  double dmg = 0;
  if ( active_actions.stagger_self_damage )
    dmg = active_actions.stagger_self_damage -> tick_amount();
  return dmg;
}

// monk_t::current_stagger_dmg_percent ==================================================

double monk_t::current_stagger_tick_dmg_percent()
{
  return current_stagger_tick_dmg() / resources.max[RESOURCE_HEALTH];
}

// monk_t::current_stagger_dot_duration ==================================================

double monk_t::current_stagger_dot_remains()
{
  double remains = 0;
  if ( active_actions.stagger_self_damage )
  {
    dot_t* dot = active_actions.stagger_self_damage -> get_dot();

    remains = dot -> ticks_left();
  }
  return remains;
}

// monk_t::convert_expression_action_to_enum =============================================

/*combo_strikes_e monk_t::convert_expression_action_to_enum( action_t* action )
{
  std::string& action_name = action->name();
  switch (action->name())
  {
    case "tiger_palm": return CS_TIGER_PALM; break;
    default: return CS_NONE; break;
  }
}
*/
// TODO: Convert action_t* a to Combo Strike ENUM
// TODO: Compare Combo Strike

// monk_t::create_expression ==================================================

expr_t* monk_t::create_expression( action_t* a, const std::string& name_str )
{
  std::vector<std::string> splits = util::string_split( name_str, "." );
  if ( splits.size() == 2 && splits[0] == "stagger" )
  {
    struct stagger_threshold_expr_t: public expr_t
    {
      monk_t& player;
      double stagger_health_pct;
      stagger_threshold_expr_t( monk_t& p, double stagger_health_pct ):
        expr_t( "stagger_threshold_" + util::to_string( stagger_health_pct ) ),
        player( p ), stagger_health_pct( stagger_health_pct )
      { }

      virtual double evaluate() override
      {
        return player.current_stagger_tick_dmg_percent() > stagger_health_pct;
      }
    };
    struct stagger_amount_expr_t: public expr_t
    {
      monk_t& player;
      stagger_amount_expr_t( monk_t& p ):
        expr_t( "stagger_amount" ),
        player( p )
      { }

      virtual double evaluate() override
      {
        return player.current_stagger_tick_dmg();
      }
    };
    struct stagger_percent_expr_t : public expr_t
    {
      monk_t& player;
      stagger_percent_expr_t( monk_t& p ) :
        expr_t( "stagger_percent" ),
        player( p )
      { }

      virtual double evaluate() override
      {
        return player.current_stagger_tick_dmg_percent() * 100;
      }
    };
    struct stagger_remains_expr_t : public expr_t
    {
      monk_t& player;
      stagger_remains_expr_t(monk_t& p) :
        expr_t("stagger_remains"),
        player(p)
      { }

      virtual double evaluate() override
      {
        return player.current_stagger_dot_remains();
      }
    };

    if ( splits[1] == "light" )
      return new stagger_threshold_expr_t( *this, light_stagger_threshold );
    else if ( splits[1] == "moderate" )
      return new stagger_threshold_expr_t( *this, moderate_stagger_threshold );
    else if ( splits[1] == "heavy" )
      return new stagger_threshold_expr_t( *this, heavy_stagger_threshold );
    else if ( splits[1] == "amount" )
      return new stagger_amount_expr_t( *this );
    else if ( splits[1] == "pct" )
      return new stagger_percent_expr_t( *this );
    else if ( splits[1] == "remains" )
      return new stagger_remains_expr_t( *this );
  }

/*  if ( splits.size() == 2 && splits[0] == "combo_strikes")
  {

  }
  */

  return base_t::create_expression( a, name_str );
}

// monk_t::monk_report =================================================

/* Report Extension Class
* Here you can define class specific report extensions/overrides
*/
class monk_report_t: public player_report_extension_t
{
public:
  monk_report_t( monk_t& player ):
    p( player )
  {
  }

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    if (p.specialization() == MONK_BREWMASTER)
    {
      double stagger_tick_dmg = p.sample_datas.stagger_tick_damage -> sum();
      double purified_dmg = p.sample_datas.purified_damage -> sum();
      double stagger_total_dmg = stagger_tick_dmg + purified_dmg;

      os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Stagger Analysis</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      os << "\t\t\t\t\t\t<p style=\"color: red;\">This section is a work in progress</p>\n";

      os << "\t\t\t\t\t\t<p>Percent amount of stagger that was purified: "
       << ( ( purified_dmg / stagger_total_dmg ) * 100 ) << "%</p>\n"
       << "\t\t\t\t\t\t<p>Percent amount of stagger that directly damaged the player: "
       << ( ( stagger_tick_dmg / stagger_total_dmg ) * 100 ) << "%</p>\n\n";

      os << "\t\t\t\t\t\t<table class=\"sc\">\n"
        << "\t\t\t\t\t\t\t<tbody>\n"
        << "\t\t\t\t\t\t\t\t<tr>\n"
        << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Damage Stats</th>\n"
        << "\t\t\t\t\t\t\t\t\t<th>DTPS</th>\n"
//        << "\t\t\t\t\t\t\t\t\t<th>DTPS%</th>\n"
        << "\t\t\t\t\t\t\t\t\t<th>Execute</th>\n"
        << "\t\t\t\t\t\t\t\t</tr>\n";

      // Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
       << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"http://www.wowhead.com/spell=124255\" class = \" icontinyl icontinyl icontinyl\" "
       << "style = \"background: url(http://wowimg.zamimg.com/images/wow/icons/tiny/ability_rogue_cheatdeath.gif) 0% 50% no-repeat;\"> "
       << "<span style = \"margin - left: 18px; \">Stagger</span></a></span>\n"
       << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
        << (p.sample_datas.stagger_tick_damage -> mean() / 60) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
        << p.sample_datas.stagger_tick_damage -> count() << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Light Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
       << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"http://www.wowhead.com/spell=124275\" class = \" icontinyl icontinyl icontinyl\" "
       << "style = \"background: url(http://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra_green.gif) 0% 50% no-repeat;\"> "
       << "<span style = \"margin - left: 18px; \">Light Stagger</span></a></span>\n"
       << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
        << (p.sample_datas.light_stagger_total_damage -> mean() / 60) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
        << p.sample_datas.light_stagger_total_damage -> count() << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Moderate Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
        << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
        << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
        << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"http://www.wowhead.com/spell=124274\" class = \" icontinyl icontinyl icontinyl\" "
        << "style = \"background: url(http://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra.gif) 0% 50% no-repeat;\"> "
        << "<span style = \"margin - left: 18px; \">Moderate Stagger</span></a></span>\n"
        << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
        << (p.sample_datas.moderate_stagger_total_damage -> mean() / 60) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
        << p.sample_datas.moderate_stagger_total_damage -> count() << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Heavy Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
        << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
        << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
        << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"http://www.wowhead.com/spell=124273\" class = \" icontinyl icontinyl icontinyl\" "
        << "style = \"background: url(http://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra_red.gif) 0% 50% no-repeat;\"> "
        << "<span style = \"margin - left: 18px; \">Heavy Stagger</span></a></span>\n"
        << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
        << (p.sample_datas.heavy_stagger_total_damage -> mean() / 60) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
        << p.sample_datas.heavy_stagger_total_damage -> count() << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      os << "\t\t\t\t\t\t\t</tbody>\n"
       << "\t\t\t\t\t\t</table>\n";

      os << "\t\t\t\t\t\t</div>\n"
       << "\t\t\t\t\t</div>\n";
    }
    else
      ( void )p;
  }
private:
  monk_t& p;
};

// MONK MODULE INTERFACE ====================================================

static void do_trinket_init( monk_t*                  player,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( ! player -> find_spell( effect.spell_id ) -> ok() ||
       player -> specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

static void eluding_movements( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*>( effect.player );
  do_trinket_init( monk, MONK_BREWMASTER, monk -> eluding_movements, effect );
}

static void soothing_breeze( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*>( effect.player );
  do_trinket_init( monk, MONK_MISTWEAVER, monk -> soothing_breeze, effect );
}

static void furious_sun( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*>( effect.player );
  do_trinket_init( monk, MONK_WINDWALKER, monk -> furious_sun, effect );
}

static void aburaq( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*> ( effect.player );
  do_trinket_init( monk, MONK_WINDWALKER, monk -> aburaq, effect );
}

struct monk_module_t: public module_t
{
  monk_module_t(): module_t( MONK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new monk_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new monk_report_t( *p ) );
    return p;
  }
  virtual bool valid() const override { return true; }

  virtual void static_init() const override
  {
    // WoD's Legendary Rings
    unique_gear::register_special_effect( 184906, eluding_movements );
    unique_gear::register_special_effect( 184907, soothing_breeze );
    unique_gear::register_special_effect( 184908, furious_sun );

    // Legion Artifacts
    unique_gear::register_special_effect( 195599, aburaq );

    // TODO: Add the Legion Legendary effects
  }


  virtual void register_hotfixes() const override
  {
  }

  virtual void init( player_t* p ) const override
  {
    p -> buffs.fierce_tiger_movement_aura = buff_creator_t( p, "fierce_tiger_movement_aura",
                                                            p -> find_spell( 103985 ) )
      .duration( timespan_t::from_seconds( 0 ) );
  }
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};
} // UNNAMED NAMESPACE

const module_t* module_t::monk()
{
  static monk_module_t m;
  return &m;
}

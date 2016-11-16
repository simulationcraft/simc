// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
NOTES:
- to evaluate Combo Strikes in the APL, use "!prev_gcd.[ability]"
- To show CJL can be interupted in the APL, use "&!prev_gcd.crackling_jade_lightning,interrupt=1"

TODO:

GENERAL:
- Add Legendaries

WINDWALKER:
- Add Cyclone Strike Counter as an expression
- Redo how Gale Bursts works with Hidden master's Forbidden Touch

MISTWEAVER: 
- Gusts of Mists - Check calculations
- Vivify - Check the interation between Thunder Focus Tea and Lifecycles
- Essence Font - See if the implementation can be corrected to the intended design.
- Life Cocoon - Double check if the Enveloping Mists and Renewing Mists from Mists of Life proc the mastery or not.
- Not Modeled:
-- Crane's Grace
-- Invoke Chi-Ji
-- Summon Jade Serpent Statue

BREWMASTER:
- Celestial Fortune needs to be implemented.
- Change the intial midigation % of stagger into an absorb (spell id 115069)
- Fortuitous Sphers - Finish implementing
- Break up Healing Elixers and Fortuitous into two spells; one for proc and one for heal
- Gift of the Ox - Check if 35% chance is baseline and increased by HP percent from there
- Double Check that Brewmasters mitigate 15% of Magical Damage
- Stagger - Effect says 10 but tooltip say 6%; double check
- Not Modeled:
-- Summon Black Ox Statue
-- Invoke Niuzao
-- Zen Meditation
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
  CS_RISING_SUN_KICK_TRINKET,
  CS_FISTS_OF_FURY,
  CS_SPINNING_CRANE_KICK,
  CS_RUSHING_JADE_WIND,
  CS_WHIRLING_DRAGON_PUNCH,
  CS_STRIKE_OF_THE_WINDLORD,
  CS_ATTACK_MAX,

  // Spells begin here
  CS_CHI_BURST,
  CS_CHI_WAVE,
  CS_CRACKLING_JADE_LIGHTNING,
  CS_TOUCH_OF_DEATH,
  CS_FLYING_SERPENT_KICK,
  CS_SPELL_MAX,

  // Misc
  CS_SPELL_MIN = CS_CHI_BURST,
  CS_ATTACK_MIN = CS_TIGER_PALM,
  CS_MAX,
};

enum sef_pet_e { SEF_FIRE = 0, SEF_EARTH, SEF_PET_MAX }; //Player becomes storm spirit.
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
  SEF_WHIRLING_DRAGON_PUNCH,
  SEF_STRIKE_OF_THE_WINDLORD,
  SEF_STRIKE_OF_THE_WINDLORD_OH,
  SEF_ATTACK_MAX,
  // Attacks end here

  // Spells begin here
  SEF_CHI_BURST,
  SEF_CHI_WAVE,
  SEF_CRACKLING_JADE_LIGHTNING,
  SEF_SPELL_MAX,
  // Spells end here

  // Misc
  SEF_SPELL_MIN = SEF_CHI_BURST,
  SEF_ATTACK_MIN = SEF_TIGER_PALM,
  SEF_MAX
};

unsigned sef_spell_idx( unsigned x )
{
  return x - as<unsigned>(static_cast<int>( SEF_SPELL_MIN ));
}

struct monk_td_t: public actor_target_data_t
{
public:

  struct dots_t
  {
    dot_t* enveloping_mist;
    dot_t* renewing_mist;
    dot_t* soothing_mist;
    dot_t* touch_of_death;
  } dots;

  struct buffs_t
  {
    debuff_t* mark_of_the_crane;
    debuff_t* dizzing_kicks;
    debuff_t* gale_burst;
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

  // Active
  heal_t*   active_celestial_fortune_proc;

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
    action_t* chi_orbit;
    actions::spells::stagger_self_damage_t* stagger_self_damage;
  } active_actions;

  combo_strikes_e previous_combo_strike;

  double gift_of_the_ox_proc_chance;
  unsigned int internal_id;
  // Counter for when to start the trigger for the 19 4-piece Windwalker Combo Master buff
  double tier19_4pc_melee_counter;

  double weapon_power_mod;
  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* eluding_movements;
  const special_effect_t* soothing_breeze;
  const special_effect_t* furious_sun;

  // Legion Artifact effects
  const special_effect_t* fu_zan_the_wanderers_companion;
  const special_effect_t* sheilun_staff_of_the_mists;
  const special_effect_t* fists_of_the_heavens;

  struct buffs_t
  {
    // General
    buff_t* chi_torpedo;
    buff_t* dampen_harm;
    buff_t* diffuse_magic;
    buff_t* rushing_jade_wind;
    stat_buff_t* tier19_oh_8pc; // Tier 19 Order Hall 8-piece

    // Brewmaster
    buff_t* bladed_armor;
    buff_t* blackout_combo;
    buff_t* brew_stache;
    buff_t* dragonfire_brew;
    buff_t* elusive_brawler;
    buff_t* elusive_dance;
    buff_t* exploding_keg;
    buff_t* fortification;
    buff_t* fortifying_brew;
    buff_t* gift_of_the_ox;
    buff_t* gifted_student;
    buff_t* greater_gift_of_the_ox;
    buff_t* ironskin_brew;
    buff_t* keg_smash_talent;
    buff_t* swift_as_a_coursing_river;
    buff_t* zen_meditation;

    // Mistweaver
    absorb_buff_t* life_cocoon;
    buff_t* channeling_soothing_mist;
    buff_t* chi_jis_guidance;
    buff_t* extend_life;
    buff_t* lifecycles_enveloping_mist;
    buff_t* lifecycles_vivify;
    buff_t* mana_tea;
    buff_t* mistweaving;
    buff_t* refreshing_jade_wind;
    buff_t* the_mists_of_sheilun;
    buff_t* teachings_of_the_monastery;
    buff_t* thunder_focus_tea;
    buff_t* uplifting_trance;

    // Windwalker
    buff_t* chi_orbit;
    buff_t* bok_proc;
    buff_t* combo_master;
    buff_t* combo_strikes;
    buff_t* dizzying_kicks;
    buff_t* forceful_winds;
    buff_t* hit_combo;
    buff_t* light_on_your_feet;
    buff_t* masterful_strikes;
    buff_t* power_strikes;
    buff_t* spinning_crane_kick;
    buff_t* storm_earth_and_fire;
    buff_t* serenity;
    buff_t* transfer_the_power;

    // Legendaries
    buff_t* hidden_masters_forbidden_touch;
    buff_t* emperors_electric_charge;
  } buff;

public:

  struct gains_t
  {
    gain_t* black_ox_brew_energy;
    gain_t* chi_refund;
    gain_t* power_strikes;
    gain_t* bok_proc;
    gain_t* crackling_jade_lightning;
    gain_t* energy_refund;
    gain_t* energizing_elixir_chi;
    gain_t* energizing_elixir_energy;
    gain_t* keg_smash;
    gain_t* gift_of_the_ox;
    gain_t* mana_tea;
    gain_t* renewing_mist;
    gain_t* serenity;
    gain_t* soothing_mist;
    gain_t* spinning_crane_kick;
    gain_t* rushing_jade_wind;
    gain_t* effuse;
    gain_t* spirit_of_the_crane;
    gain_t* tier17_2pc_healer;
    gain_t* tiger_palm;
    gain_t* healing_elixirs;
    gain_t* fortuitous_spheres;
  } gain;

  struct procs_t
  {
    proc_t* bok_proc;
    proc_t* eye_of_the_tiger;
    proc_t* mana_tea;
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
    const spell_data_t* mistwalk;

    // Tier 30 Talents
    const spell_data_t* chi_torpedo;
    const spell_data_t* tigers_lust;
    const spell_data_t* celerity;

    // Tier 45 Talents
    // Brewmaster
    const spell_data_t* light_brewing;
    const spell_data_t* black_ox_brew;
    const spell_data_t* gift_of_the_mists;
    // Windwalker
    const spell_data_t* energizing_elixir;
    const spell_data_t* ascension;
    const spell_data_t* power_strikes;
    // Mistweaver
    const spell_data_t* lifecycles;
    const spell_data_t* spirit_of_the_crane;
    const spell_data_t* mist_wrap;

    // Tier 60 Talents
    const spell_data_t* ring_of_peace;
    const spell_data_t* summon_black_ox_statue; // Brewmaster
    const spell_data_t* dizzying_kicks; // Windwalker
    const spell_data_t* song_of_chi_ji; // Mistweaver
    const spell_data_t* leg_sweep;

    // Tier 75 Talents
    const spell_data_t* healing_elixirs;
    const spell_data_t* diffuse_magic;
    const spell_data_t* dampen_harm;

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
    const spell_data_t* blackout_combo;
    const spell_data_t* high_tolerance;
    // Windwalker
    const spell_data_t* chi_orbit;
    const spell_data_t* whirling_dragon_punch;
    const spell_data_t* serenity;
    // Mistweaver
    const spell_data_t* mana_tea;
    const spell_data_t* focused_thunder;
    const spell_data_t* rising_thunder;
  } talent;

  // Specialization
  struct specs_t
  {
    // GENERAL
    const spell_data_t* blackout_kick;
    const spell_data_t* crackling_jade_lightning;
    const spell_data_t* critical_strikes;
    const spell_data_t* effuse;
    const spell_data_t* effuse_2;
    const spell_data_t* leather_specialization;
    const spell_data_t* provoke;
    const spell_data_t* rising_sun_kick;
    const spell_data_t* roll;
    const spell_data_t* spinning_crane_kick;
    const spell_data_t* spear_hand_strike;
    const spell_data_t* tiger_palm;

    // Brewmaster
    const spell_data_t* blackout_strike;
    const spell_data_t* bladed_armor;
    const spell_data_t* breath_of_fire;
    const spell_data_t* celestial_fortune;
    const spell_data_t* expel_harm;
    const spell_data_t* fortifying_brew;
    const spell_data_t* gift_of_the_ox;
    const spell_data_t* ironskin_brew;
    const spell_data_t* keg_smash;
    const spell_data_t* purifying_brew;
    const spell_data_t* stagger;
    const spell_data_t* zen_meditation;
    const spell_data_t* light_stagger;
    const spell_data_t* moderate_stagger;
    const spell_data_t* heavy_stagger;

    // Mistweaver
    const spell_data_t* detox;
    const spell_data_t* enveloping_mist;
    const spell_data_t* envoloping_mist_2;
    const spell_data_t* essence_font;
    const spell_data_t* essence_font_2;
    const spell_data_t* life_cocoon;
    const spell_data_t* reawaken;
    const spell_data_t* renewing_mist;
    const spell_data_t* renewing_mist_2;
    const spell_data_t* resuscitate;
    const spell_data_t* revival;
    const spell_data_t* soothing_mist;
    const spell_data_t* teachings_of_the_monastery;
    const spell_data_t* thunder_focus_tea;
    const spell_data_t* thunger_focus_tea_2;
    const spell_data_t* vivify;

    // Windwalker
    const spell_data_t* afterlife;
    const spell_data_t* combat_conditioning; // Possibly will get removed
    const spell_data_t* combo_breaker;
    const spell_data_t* cyclone_strikes;
    const spell_data_t* disable;
    const spell_data_t* fists_of_fury;
    const spell_data_t* flying_serpent_kick;
    const spell_data_t* stance_of_the_fierce_tiger;
    const spell_data_t* storm_earth_and_fire;
    const spell_data_t* storm_earth_and_fire_2;
    const spell_data_t* touch_of_death;
    const spell_data_t* touch_of_karma;
    const spell_data_t* windwalker_monk;
    const spell_data_t* windwalking;
  } spec;

  // Artifact
  struct artifact_spell_data_t
  {
    // Brewmaster Artifact
    artifact_power_t hot_blooded;
    artifact_power_t brew_stache;
    artifact_power_t dark_side_of_the_moon;
    artifact_power_t dragonfire_brew;
    artifact_power_t face_palm;
    artifact_power_t exploding_keg;
    artifact_power_t fortification;
    artifact_power_t gifted_student;
    artifact_power_t healthy_appetite;
    artifact_power_t full_keg;
    artifact_power_t obsidian_fists;
    artifact_power_t obstinate_determination;
    artifact_power_t overflow;
    artifact_power_t potent_kick;
    artifact_power_t smashed;
    artifact_power_t staggering_around;
    artifact_power_t swift_as_a_coursing_river;

    // Mistweaver Artifact
    artifact_power_t blessings_of_yulon;
    artifact_power_t celestial_breath;
    artifact_power_t coalescing_mists;
    artifact_power_t dancing_mists;
    artifact_power_t essence_of_the_mists;
    artifact_power_t extended_healing;
    artifact_power_t infusion_of_life;
    artifact_power_t light_on_your_feet_mw;
    artifact_power_t mists_of_life;
    artifact_power_t mists_of_wisdom;
    artifact_power_t protection_of_shaohao;
    artifact_power_t sheiluns_gift;
    artifact_power_t shroud_of_mist;
    artifact_power_t soothing_remedies;
    artifact_power_t spirit_tether;
    artifact_power_t the_mists_of_sheilun;
    artifact_power_t way_of_the_mistweaver;

    // Windwalker Artifact
    artifact_power_t crosswinds;
    artifact_power_t dark_skies;
    artifact_power_t death_art;
    artifact_power_t fists_of_the_wind;
    artifact_power_t gale_burst;
    artifact_power_t good_karma;
    artifact_power_t healing_winds;
    artifact_power_t inner_peace;
    artifact_power_t light_on_your_feet_ww;
    artifact_power_t power_of_a_thousand_cranes;
    artifact_power_t rising_winds;
    artifact_power_t spiritual_focus;
    artifact_power_t strike_of_the_windlord;
    artifact_power_t strength_of_xuen;
    artifact_power_t tiger_claws;
    artifact_power_t tornado_kicks;
    artifact_power_t transfer_the_power;
  } artifact;

  struct mastery_spells_t
  {
    const spell_data_t* combo_strikes;       // Windwalker
    const spell_data_t* elusive_brawler;     // Brewmaster
    const spell_data_t* gust_of_mists;       // Mistweaver
  } mastery;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* blackout_kick;
    cooldown_t* blackout_strike;
    cooldown_t* black_ox_brew;
    cooldown_t* brewmaster_attack;
    cooldown_t* brewmaster_active_mitigation;
    cooldown_t* breath_of_fire;
    cooldown_t* desperate_measure;
    cooldown_t* fists_of_fury;
    cooldown_t* fortifying_brew;
    cooldown_t* healing_elixirs;
    cooldown_t* rising_sun_kick;
    cooldown_t* refreshing_jade_wind;
    cooldown_t* rushing_jade_wind;
    cooldown_t* strike_of_the_windlord;
    cooldown_t* thunder_focus_tea;
    cooldown_t* touch_of_death;
    cooldown_t* serenity;
  } cooldown;

  struct passives_t
  {
    // General
    const spell_data_t* aura_monk;
    const spell_data_t* chi_burst_damage;
    const spell_data_t* chi_burst_heal;
    const spell_data_t* chi_torpedo;
    const spell_data_t* chi_wave_damage;
    const spell_data_t* chi_wave_heal;
    const spell_data_t* healing_elixirs;
    // Brewmaster
    const spell_data_t* aura_brewmaster_monk;
    const spell_data_t* breath_of_fire_dot;
    const spell_data_t* celestial_fortune;
    const spell_data_t* dragonfire_brew_damage;
    const spell_data_t* elusive_brawler;
    const spell_data_t* elusive_dance;
    const spell_data_t* face_palm;
    const spell_data_t* gift_of_the_ox_heal;
    const spell_data_t* gift_of_the_ox_summon;
    const spell_data_t* greater_gift_of_the_ox_heal;
    const spell_data_t* ironskin_brew;
    const spell_data_t* keg_smash_buff;
    const spell_data_t* special_delivery;
    const spell_data_t* stagger_self_damage;
    const spell_data_t* tier17_2pc_tank;

    // Mistweaver
    const spell_data_t* aura_mistweaver_monk;
    const spell_data_t* blessings_of_yulon;
    const spell_data_t* celestial_breath_heal;
    const spell_data_t* lifecycles_enveloping_mist;
    const spell_data_t* lifecycles_vivify;
    const spell_data_t* renewing_mist_heal;
    const spell_data_t* shaohaos_mists_of_wisdom;
    const spell_data_t* soothing_mist_heal;
    const spell_data_t* soothing_mist_statue;
    const spell_data_t* spirit_of_the_crane;
    const spell_data_t* teachings_of_the_monastery_buff;
    const spell_data_t* totm_bok_proc;
    const spell_data_t* the_mists_of_sheilun_heal;
    const spell_data_t* uplifting_trance;
    const spell_data_t* zen_pulse_heal;
    const spell_data_t* tier17_2pc_heal;
    const spell_data_t* tier17_4pc_heal;
    const spell_data_t* tier18_2pc_heal;

    // Windwalker
    const spell_data_t* aura_windwalker_monk;
    const spell_data_t* chi_orbit;
    const spell_data_t* bok_proc;
    const spell_data_t* crackling_tiger_lightning;
    const spell_data_t* crackling_tiger_lightning_driver;
    const spell_data_t* crosswinds_dmg;
    const spell_data_t* crosswinds_trigger;
    const spell_data_t* dizzying_kicks;
    const spell_data_t* gale_burst;
    const spell_data_t* hit_combo;
    const spell_data_t* mark_of_the_crane;
    const spell_data_t* whirling_dragon_punch;
    const spell_data_t* touch_of_karma_tick;
    const spell_data_t* tier17_4pc_melee;
    const spell_data_t* tier18_2pc_melee;
    const spell_data_t* tier19_4pc_melee;

    // Legendaries
    const spell_data_t* hidden_masters_forbidden_touch;
    const spell_data_t* the_emperor_capacitor;
  } passives;

  struct legendary_t
  {
    // General
    const spell_data_t* archimondes_infinite_command;
    const spell_data_t* cinidaria_the_symbiote;
    const spell_data_t* kiljaedens_burning_wish;
    const spell_data_t* prydaz_xavarics_magnum_opus;
    const spell_data_t* sephuzs_secret;
    const spell_data_t* velens_future_sight;

    // Brewmaster
    const spell_data_t* anvil_hardened_wristwraps;
    const spell_data_t* firestone_walkers;
    const spell_data_t* fundamental_observation;
    const spell_data_t* gai_plins_soothing_sash;
    const spell_data_t* jewel_of_the_lost_abbey;
    const spell_data_t* salsalabims_lost_tunic;

    // Mistweaver
    const spell_data_t* eithas_lunar_glides_of_eramas;
    const spell_data_t* eye_of_collidus_the_warp_watcher;
    const spell_data_t* leggings_of_the_black_flame;
    const spell_data_t* ovyds_winter_wrap;
    const spell_data_t* petrichor_lagniappe;
    const spell_data_t* shelter_of_rin;
    const spell_data_t* unison_spaulders;

    // Windwalker
    const spell_data_t* cenedril_reflector_of_hatred; // The amount of damage that Touch of Karma can redirect is increased by x% of your maximum health.
    const spell_data_t* drinking_horn_cover; //The duration of Storm, Earth, and Fire is extended by x sec for every Chi you spend.
    const spell_data_t* hidden_masters_forbidden_touch; // Touch of Death can be used a second time within 3 sec before its cooldown is triggered.
    const spell_data_t* katsuos_eclipse; // Reduce the cost of Fists of Fury by x Chi.
    const spell_data_t* march_of_the_legion; // Increase the movement speed bonus of Windwalking by x%.
    const spell_data_t* the_emperors_capacitor; // Chi spenders increase the damage of your next Crackling Jade Lightning by X%. Stacks up to Y times.
  } legendary;

  struct pets_t
  {
    pets::storm_earth_and_fire_pet_t* sef[ SEF_PET_MAX ];
  } pet;

  // Options
  struct options_t
  {
    int initial_chi;
  } user_options;

  // Blizzard rounds it's stagger damage; anything higher than half a percent beyond
  // the threshold will switch to the next threshold
  const double light_stagger_threshold;
  const double moderate_stagger_threshold;
  const double heavy_stagger_threshold;
private:
  target_specific_t<monk_td_t> target_data;
public:
  monk_t( sim_t* sim, const std::string& name, race_e r )
    : player_t( sim, MONK, name, r ),
      active_actions( active_actions_t() ),
      previous_combo_strike(CS_NONE),
      gift_of_the_ox_proc_chance(),
      internal_id(),
      tier19_4pc_melee_counter(),
      weapon_power_mod(),
      eluding_movements( nullptr ),
      soothing_breeze( nullptr ),
      furious_sun( nullptr ),
      fu_zan_the_wanderers_companion( nullptr ),
      sheilun_staff_of_the_mists( nullptr ),
      fists_of_the_heavens( nullptr ),
      buff( buffs_t() ),
      gain( gains_t() ),
      proc( procs_t() ),
      talent( talents_t() ),
      spec( specs_t() ),
      mastery( mastery_spells_t() ),
      cooldown( cooldowns_t() ),
      passives( passives_t() ),
      legendary( legendary_t() ),
      pet( pets_t() ),
      user_options( options_t() ),
      light_stagger_threshold( 0 ),
      moderate_stagger_threshold( 0.01666 ), // Moderate transfers at 33.3% Stagger; 1.67% every 1/2 sec
      heavy_stagger_threshold( 0.03333 ) // Heavy transfers at 66.6% Stagger; 3.34% every 1/2 sec
  {
    // actives
    active_celestial_fortune_proc = nullptr;


    cooldown.blackout_kick                = get_cooldown( "blackout_kick" );
    cooldown.blackout_strike              = get_cooldown( "blackout_stike" );
    cooldown.black_ox_brew                = get_cooldown( "black_ox_brew" );
    cooldown.brewmaster_attack            = get_cooldown( "brewmaster_attack" );
    cooldown.brewmaster_active_mitigation = get_cooldown( "brews" );
    cooldown.breath_of_fire               = get_cooldown( "breath_of_fire" );
    cooldown.fortifying_brew              = get_cooldown( "fortifying_brew" );
    cooldown.fists_of_fury                = get_cooldown( "fists_of_fury" );
    cooldown.healing_elixirs              = get_cooldown( "healing_elixirs" );
    cooldown.rising_sun_kick              = get_cooldown( "rising_sun_kick" );
    cooldown.refreshing_jade_wind         = get_cooldown( "refreshing_jade_wind" );
    cooldown.rushing_jade_wind            = get_cooldown( "rushing_jade_wind" );
    cooldown.strike_of_the_windlord       = get_cooldown( "strike_of_the_windlord" );
    cooldown.thunder_focus_tea            = get_cooldown( "thunder_focus_tea" );
    cooldown.touch_of_death               = get_cooldown( "touch_of_death" );
    cooldown.serenity                     = get_cooldown( "serenity" );

    regen_type = REGEN_DYNAMIC;
    if ( specialization() != MONK_MISTWEAVER )
    {
      regen_caches[CACHE_HASTE] = true;
      regen_caches[CACHE_ATTACK_HASTE] = true;
    }
    user_options.initial_chi = 0;
  }

  // player_t overrides
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_melee_crit_chance() const override;
  virtual double    composite_melee_crit_chance_multiplier() const override;
  virtual double    composite_spell_crit_chance() const override;
  virtual double    composite_spell_crit_chance_multiplier() const override;
  virtual double    energy_regen_per_second() const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_heal_multiplier( const action_state_t* s ) const override;
  virtual double    composite_melee_expertise( const weapon_t* weapon ) const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_parry() const override;
  virtual double    composite_dodge() const override;
  virtual double    composite_mastery() const override;
  virtual double    composite_mastery_rating() const override;
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual double    temporary_movement_modifier() const override;
  virtual double    passive_movement_modifier() const override;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_rng() override;
  virtual void      init_resources( bool ) override;
  virtual void      regen( timespan_t periodicity ) override;
  virtual void      reset() override;
  virtual void      interrupt() override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      recalculate_resource_max( resource_e ) override;
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
  virtual void      assess_heal( school_e, dmg_e, action_state_t* s) override;
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
//  combo_strikes_e convert_expression_action_to_enum( action_t* a );
  void trigger_celestial_fortune( action_state_t* );
  void trigger_mark_of_the_crane( action_state_t* );
  player_t* next_mark_of_the_crane_target( action_state_t* );
  int mark_of_the_crane_counter();
  double clear_stagger();
  double partial_clear_stagger( double );
  bool has_stagger();

  // Storm Earth and Fire targeting logic
  std::vector<player_t*> create_storm_earth_and_fire_target_list() const;
  void retarget_storm_earth_and_fire( pet_t* pet, std::vector<player_t*>& targets, size_t n_targets ) const;
  void retarget_storm_earth_and_fire_pets() const;
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
  jade_serpent_statue_t( sim_t* sim, monk_t* owner, const std::string& n ):
    statue_t( sim, owner, n, PET_NONE, true )
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

        if ( ( this -> id > 0 && this -> id == a -> id ) ||
             util::str_compare_ci( this -> name_str, a -> name_str ) )
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

    timespan_t tick_time( const action_state_t* s ) const
    {
      return source_action -> tick_time( s );
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

    void execute()
    {
      // Target always follows the SEF clone's target, which is assigned during
      // summon time
      this -> target = this -> player -> target;

      super_t::execute();
    }

    void snapshot_internal( action_state_t* state, uint32_t flags, dmg_e rt )
    {
      super_t::snapshot_internal( state, flags, rt );

      // Take out the Owner's Hit Combo Multiplier, but only if the ability is going to snapshot
      // multipliers in the first place.
      if ( o() -> talent.hit_combo -> ok() )
      {
        if ( rt == DMG_DIRECT && ( flags & STATE_MUL_DA ) )
        {
          // Remove owner's Hit Combo
          state -> da_multiplier /= ( 1 + o() -> buff.hit_combo -> stack_value() );
          // .. aand add Pet's Hit Combo
          state -> da_multiplier *= 1 + p() -> sef_hit_combo -> stack_value();
        }

        if ( rt == DMG_OVER_TIME && ( flags & STATE_MUL_TA ) )
        {
          state -> ta_multiplier /= ( 1 + o() -> buff.hit_combo -> stack_value() );
          state -> ta_multiplier *= 1 + p() -> sef_hit_combo -> stack_value();
        }
      }
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
      if ( ! source_action && sim -> debug )
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
      else
      {
        // 1h/dual wield equation. Note, this formula is slightly off (~3%) for
        // owner dw/pet dw variation.
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
      {
        sef_melee_attack_t::execute();
      }
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

    virtual bool ready() override
    {
      if ( player -> is_moving() ) return false;

      return ( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
    }

    virtual void execute() override
    {
      player -> main_hand_attack -> schedule_execute();

      if ( player -> off_hand_attack )
        player -> off_hand_attack -> schedule_execute();
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

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
        o() -> trigger_mark_of_the_crane( state );
    }
  };

  struct sef_blackout_kick_t : public sef_melee_attack_t
  {

    sef_blackout_kick_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "blackout_kick", player, player -> o() -> spec.blackout_kick )
    { }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        o() -> trigger_mark_of_the_crane( state );
      }
    }
   };

  struct sef_rsk_tornado_kick_t : sef_melee_attack_t
  {
    sef_rsk_tornado_kick_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "rising_sun_kick_tornado_kick", player, player -> o() -> spec.rising_sun_kick -> effectN( 1 ).trigger() )
    {
      may_miss = may_dodge = may_parry = may_crit = may_block = true;

      cooldown -> duration = timespan_t::zero();
      background = dual = true;
      trigger_gcd = timespan_t::zero();
    }

    void init() override
    {
      sef_melee_attack_t::init();

      snapshot_flags &= STATE_NO_MULTIPLIER;
      snapshot_flags &= ~STATE_AP;
      snapshot_flags |= STATE_CRIT;
      snapshot_flags |= STATE_TGT_ARMOR;

      update_flags &= STATE_NO_MULTIPLIER;
      update_flags &= ~STATE_AP;
      update_flags |= STATE_CRIT;
      update_flags |= STATE_TGT_ARMOR;
    }

    // Force 250 milliseconds for the animation, but not delay the overall GCD
    timespan_t execute_time() const override
    {
      return timespan_t::from_millis( 250 );
    }

    virtual double cost() const override
    {
      return 0;
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        state -> target -> debuffs.mortal_wounds -> trigger();
        o() -> trigger_mark_of_the_crane( state );
      }
    }
  };


  struct sef_rising_sun_kick_t : public sef_melee_attack_t
  {
    sef_rsk_tornado_kick_t* rsk_tornado_kick;

    sef_rising_sun_kick_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "rising_sun_kick", player, player -> o() -> spec.rising_sun_kick ),
      rsk_tornado_kick( new sef_rsk_tornado_kick_t( player ) )
    {
      if ( player -> o() -> artifact.tornado_kicks.rank() )
        add_child( rsk_tornado_kick );
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        if ( o() -> spec.combat_conditioning )
          state -> target -> debuffs.mortal_wounds -> trigger();

        if ( o() -> spec.spinning_crane_kick )
          o() -> trigger_mark_of_the_crane( state );

        if ( o() -> artifact.tornado_kicks.rank() )
        {
          double raw = state -> result_raw * o() -> artifact.tornado_kicks.data().effectN( 1 ).percent();
          rsk_tornado_kick -> target = state -> target;
          rsk_tornado_kick -> base_dd_max = raw;
          rsk_tornado_kick -> base_dd_min = raw;
          rsk_tornado_kick -> execute();
        }
      }
    }
  };

  struct sef_rising_sun_kick_trinket_t : public sef_melee_attack_t
  {

    sef_rising_sun_kick_trinket_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "rising_sun_kick_trinket", player, player -> o() -> spec.rising_sun_kick -> effectN( 1 ).trigger() )
    {
      player -> find_action( "rising_sun_kick" ) -> add_child( this );
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
        state -> target -> debuffs.mortal_wounds -> trigger();
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
  };

  struct sef_fists_of_fury_t : public sef_melee_attack_t
  {
    sef_fists_of_fury_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "fists_of_fury", player, player -> o() -> spec.fists_of_fury )
    {
      channeled = tick_zero = interrupt_auto_attack = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_fists_of_fury_tick_t( player );
    }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = sef_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( o() -> buff.transfer_the_power -> up() )
    {
      pm /= 1 + o() -> buff.transfer_the_power -> stack_value();
    }

    return pm;
  }

//    timespan_t composite_dot_duration( const action_state_t* s ) const override { return timespan_t::from_millis( 4000 ); }

//    timespan_t tick_time( const action_state_t* ) const override { return timespan_t::from_millis( 1000 ); }
  };

  struct sef_spinning_crane_kick_t : public sef_melee_attack_t
  {
    sef_spinning_crane_kick_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "spinning_crane_kick", player, player -> o() -> spec.spinning_crane_kick )
    {
      tick_zero = interrupt_auto_attack = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_tick_action_t( "spinning_crane_kick_tick", player, 
          player -> o() -> spec.spinning_crane_kick -> effectN( 1 ).trigger() );
    }
  };

  struct sef_rushing_jade_wind_t : public sef_melee_attack_t
  {
    sef_rushing_jade_wind_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "rushing_jade_wind", player, player -> o() -> talent.rushing_jade_wind )
    {
      tick_zero = hasted_ticks = true;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_tick_action_t( "rushing_jade_wind_tick", player,
          player -> o() -> talent.rushing_jade_wind -> effectN( 1 ).trigger() );
    }
  };

  struct sef_whirling_dragon_punch_tick_t : public sef_tick_action_t
  {
    sef_whirling_dragon_punch_tick_t( storm_earth_and_fire_pet_t* p ):
      sef_tick_action_t( "whirling_dragon_punch_tick", p, p -> o() -> passives.whirling_dragon_punch )
    {
      aoe = -1;
    }
  };

  struct sef_whirling_dragon_punch_t : public sef_melee_attack_t
  {
    sef_whirling_dragon_punch_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "whirling_dragon_punch", player, player -> o() -> talent.whirling_dragon_punch )
    {
      channeled = true;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_whirling_dragon_punch_tick_t( player );
    }
  };

  struct sef_strike_of_the_windlord_oh_t : public sef_melee_attack_t
  {
    sef_strike_of_the_windlord_oh_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "strike_of_the_windlord_offhand", player, player -> o() -> artifact.strike_of_the_windlord.data().effectN( 4 ).trigger() )
    {
      may_dodge = may_parry = may_block = may_miss = true;
      dual = true;
      aoe = -1;
      radius = data().effectN( 2 ).base_value();
      // Unlike the player, SEF don't have a damage variance. Possible bug
      sim -> average_range = 1;
      weapon = &( player -> off_hand_weapon );
    }

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

  struct sef_strike_of_the_windlord_t : public sef_melee_attack_t
  {
    sef_strike_of_the_windlord_t( storm_earth_and_fire_pet_t* player ) :
      sef_melee_attack_t( "strike_of_the_windlord", player, player -> o() -> artifact.strike_of_the_windlord.data().effectN( 3 ).trigger() )
    {
      may_dodge = may_parry = may_block = may_miss = true;
      dual = true;
      aoe = -1;
      radius = data().effectN( 2 ).base_value();
      weapon = &( player -> main_hand_weapon );
      // Unlike the player, SEF don't have a damage variance. Possible bug
      sim -> average_range = 1;
      normalize_weapon_speed = true;
    }

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

  struct sef_chi_burst_damage_t: public sef_spell_t
  {
    sef_chi_burst_damage_t( storm_earth_and_fire_pet_t* player ):
      sef_spell_t( "chi_burst_damage", player, player -> o() -> passives.chi_burst_damage)
    {
      dual = true;
      aoe = -1;
    }
  };

  struct sef_chi_burst_t : public sef_spell_t
  {
    sef_chi_burst_damage_t* damage;
    sef_chi_burst_t( storm_earth_and_fire_pet_t* player ) :
      sef_spell_t( "chi_burst", player, player -> o() -> talent.chi_burst ),
      damage( new sef_chi_burst_damage_t( player ) )
    { 
    }

    virtual void execute() override
    {
      sef_spell_t::execute();

      damage -> execute();
    }

  };

  struct sef_crackling_jade_lightning_t : public sef_spell_t
  {
    sef_crackling_jade_lightning_t( storm_earth_and_fire_pet_t* player ) :
      sef_spell_t( "crackling_jade_lightning", player, player -> o() -> spec.crackling_jade_lightning )
    {
      tick_may_crit = true;
      hasted_ticks = false;
      interrupt_auto_attack = true;
    }
  };

  // Storm, Earth, and Fire abilities end ===================================


  std::vector<sef_melee_attack_t*> attacks;
  std::vector<sef_spell_t*> spells;

private:
  target_specific_t<sef_td_t> target_data;
public:
  // SEF applies the Cyclone Strike debuff as well

  bool sticky_target; // When enabled, SEF pets will stick to the target they have
  buff_t* sef_hit_combo;

  storm_earth_and_fire_pet_t( const std::string& name, sim_t* sim, monk_t* owner, bool dual_wield ):
    pet_t( sim, owner, name, true ),
    attacks( SEF_ATTACK_MAX ), spells( SEF_SPELL_MAX - SEF_SPELL_MIN ),
    sticky_target( false ), sef_hit_combo(nullptr)
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

  // Reset SEF target to default settings
  void reset_targeting()
  {
    target = owner -> target;
    sticky_target = false;
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

    attacks.at( SEF_TIGER_PALM )      = new sef_tiger_palm_t( this );
    attacks.at( SEF_BLACKOUT_KICK )   = new sef_blackout_kick_t( this );
    attacks.at( SEF_RISING_SUN_KICK ) = new sef_rising_sun_kick_t( this );
    attacks.at( SEF_RISING_SUN_KICK_TRINKET ) =
        new sef_rising_sun_kick_trinket_t( this );
    attacks.at( SEF_FISTS_OF_FURY ) = new sef_fists_of_fury_t( this );
    attacks.at( SEF_SPINNING_CRANE_KICK ) =
        new sef_spinning_crane_kick_t( this );
    attacks.at( SEF_RUSHING_JADE_WIND ) = new sef_rushing_jade_wind_t( this );
    attacks.at( SEF_WHIRLING_DRAGON_PUNCH ) =
        new sef_whirling_dragon_punch_t( this );
    attacks.at( SEF_STRIKE_OF_THE_WINDLORD ) = 
        new sef_strike_of_the_windlord_t( this );
    attacks.at( SEF_STRIKE_OF_THE_WINDLORD_OH ) = 
        new sef_strike_of_the_windlord_oh_t( this );

    spells.at( sef_spell_idx( SEF_CHI_BURST ) ) = new sef_chi_burst_t( this );
    spells.at( sef_spell_idx( SEF_CHI_WAVE ) )  = new sef_chi_wave_t( this );
    spells.at( sef_spell_idx( SEF_CRACKLING_JADE_LIGHTNING ) ) =
        new sef_crackling_jade_lightning_t( this );
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
  }

  void dismiss( bool expired = false ) override
  {
    pet_t::dismiss( expired );

    o() -> buff.storm_earth_and_fire -> decrement();
  }

  void create_buffs() override
  {
    pet_t::create_buffs();

    sef_hit_combo = buff_creator_t( this, "sef_hit_combo", o() -> passives.hit_combo )
                    .default_value( o() -> passives.hit_combo -> effectN( 1 ).percent() )
                    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void trigger_attack( sef_ability_e ability, const action_t* source_action )
  {
    if ( ability >= SEF_SPELL_MIN )
    {
      size_t spell = static_cast<size_t>( ability - SEF_SPELL_MIN );
      assert( spells[ spell ] );

      if ( o() -> buff.combo_strikes -> up() && o() -> talent.hit_combo -> ok() )
        sef_hit_combo -> trigger();

      spells[ spell ] -> source_action = source_action;
      spells[ spell ] -> execute();
    }
    else
    {
      assert( attacks[ ability ] );

      if ( o() -> buff.combo_strikes -> up() && o() -> talent.hit_combo -> ok() )
        sef_hit_combo -> trigger();

      attacks[ ability ] -> source_action = source_action;
      attacks[ ability ] -> execute();
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
      school = SCHOOL_PHYSICAL;

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
    crackling_tiger_lightning_driver_t( xuen_pet_t *p, const std::string& options_str ): 
      spell_t( "crackling_tiger_lightning_driver", p, p -> o() -> passives.crackling_tiger_lightning_driver )
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
    crackling_tiger_lightning_t( xuen_pet_t* player, const std::string& options_str ): 
      melee_attack_t( "crackling_tiger_lightning", player, player -> o() -> passives.crackling_tiger_lightning )
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

    virtual bool ready() override
    {
      if ( player -> is_moving() ) return false;

      return ( player->main_hand_attack -> execute_event == nullptr ); // not swinging
    }

    virtual void execute() override
    {
      player -> main_hand_attack -> schedule_execute();

      if ( player -> off_hand_attack )
        player -> off_hand_attack -> schedule_execute();
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
  std::array < resource_e, MONK_MISTWEAVER + 1 > _resource_by_stance;
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
    ab::trigger_gcd = timespan_t::from_seconds( 1.5 );
    switch ( player -> specialization() )
    {
      case MONK_BREWMASTER:
      {
        // Reduce GCD from 1.5 sec to 1 sec
        if ( ab::data().affected_by( player -> spec.stagger -> effectN( 11 ) ) )
          ab::trigger_gcd += player -> spec.stagger -> effectN( 11 ).time_value(); // Saved as -500 milliseconds
        // Technically minimum GCD is 750ms but nothing brings the GCD below 1 sec
        ab::min_gcd = timespan_t::from_seconds( 1.0 );
        // Brewmasters no longer use Chi so need to zero out chi cost
        if ( ab::data().affected_by( player -> spec.stagger -> effectN( 15 ) ) )
          ab::base_costs[RESOURCE_CHI] *= 1 + player -> spec.stagger -> effectN( 15 ).percent(); // -100% for Brewmasters
        // Hasted Cooldown
        ab::cooldown -> hasted = ab::data().affected_by( player -> passives.aura_brewmaster_monk -> effectN( 1 ) );
        break;
      }
      case MONK_MISTWEAVER:
      {
        // Hasted Cooldown
        ab::cooldown -> hasted = ab::data().affected_by( player -> passives.aura_mistweaver_monk -> effectN( 5 ) );
        break;
      }
      case MONK_WINDWALKER:
      {

        if ( ab::data().affected_by( player -> spec.stance_of_the_fierce_tiger -> effectN( 5 ) ) )
          ab::trigger_gcd += player -> spec.stance_of_the_fierce_tiger -> effectN( 5 ).time_value(); // Saved as -500 milliseconds
      // Technically minimum GCD is 750ms but nothing brings the GCD below 1 sec
        ab::min_gcd = timespan_t::from_seconds( 1.0 );
        // Hasted Cooldown
        ab::cooldown -> hasted = ab::data().affected_by( player -> passives.aura_monk -> effectN( 1 ) );
        // Cooldown reduction
        if ( ab::data().affected_by( player -> passives.aura_windwalker_monk -> effectN( 1 ) ) )
          ab::cooldown -> duration *= 1 + player -> passives.aura_windwalker_monk -> effectN( 1 ).percent(); // saved as -100
        break;
      }
      default: break;
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

  bool ready() override
  {
    if ( !ab::ready() )
      return false;

    return true;
  }

  void init() override
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
        assert( _resource_by_stance[ specdata::spec_idx( MONK_BREWMASTER ) ] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[ specdata::spec_idx( MONK_BREWMASTER ) ] = pd -> resource();
        break;
      case 137024:
        assert( _resource_by_stance[ specdata::spec_idx( MONK_MISTWEAVER ) ] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[ specdata::spec_idx( MONK_MISTWEAVER ) ] = pd -> resource();
        break;
      case 137025:
        assert( _resource_by_stance[ specdata::spec_idx( MONK_WINDWALKER ) ] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[ specdata::spec_idx( MONK_WINDWALKER ) ] = pd -> resource();
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

  resource_e current_resource() const override
  {
    resource_e resource_by_stance = _resource_by_stance[specdata::spec_idx( p() -> specialization() )];

    if ( resource_by_stance == RESOURCE_MAX )
      return ab::current_resource();

    return resource_by_stance;
  }

  // Make sure the current combo strike ability is not the same as the previous ability used
  virtual bool compare_previous_combo_strikes( combo_strikes_e new_ability )
  {
    return p() -> previous_combo_strike == new_ability;
  }

  // Used to trigger Windwalker's Combo Strike Mastery; Triggers prior to calculating damage
  void combo_strikes_trigger( combo_strikes_e new_ability )
  {
    if ( !compare_previous_combo_strikes( new_ability ) && p() -> mastery.combo_strikes -> ok() )
    {
      if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T19, B4 ) )
      {
        if ( p() -> tier19_4pc_melee_counter < 3 )
          p() -> tier19_4pc_melee_counter++;
        else
        {
          p() -> buff.combo_master -> trigger();
          p() -> tier19_4pc_melee_counter = 0;
        }
      }

      p() -> buff.combo_strikes -> trigger();
      if ( p() -> talent.hit_combo -> ok() )
        p() -> buff.hit_combo -> trigger();
    }
    else
    {
      p() -> buff.combo_strikes -> expire();
      p() -> buff.hit_combo -> expire();
      p() -> buff.combo_master -> expire();
      p() -> tier19_4pc_melee_counter = 0;
    }
    p() -> previous_combo_strike = new_ability;
  }

  double cost() const override
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

    if ( p() -> buff.mana_tea -> up() && ab::data().affected_by( p() -> talent.mana_tea -> effectN( 1 ) ) )
      c += p() -> buff.mana_tea -> value(); // saved as -50%

    if ( p() -> buff.serenity -> up() && ab::data().affected_by( p() -> talent.serenity -> effectN( 1 ) ) )
      c += p() -> talent.serenity -> effectN( 1 ).percent(); // Saved as -100

    if ( p() -> buff.bok_proc -> up() && ab::data().affected_by( p() -> passives.bok_proc -> effectN( 1 ) ) )
      c += p() -> passives.bok_proc -> effectN ( 1 ).percent(); // Saved as -100

    return c;
  }

  virtual void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    timespan_t cd = cd_duration;
    // Only adjust cooldown (through serenity) if it's non zero.
    if ( cd_duration == timespan_t::min() )
    {
      cd = ab::cooldown -> duration;
    }

    // Update the cooldown while Serenity is active
    if ( p() -> buff.serenity -> up() && ab::data().affected_by( p() -> talent.serenity -> effectN( 4 ) ) )
      cd *= 1 + p() -> talent.serenity -> effectN( 4 ).percent(); // saved as -50

    ab::update_ready( cd );
  }
  
  void consume_resource() override
  {
    ab::consume_resource();

    if ( !ab::execute_state ) // Fixes rare crashes at combat_end.
      return;

    
    if ( current_resource() == RESOURCE_CHI )
    {
      if ( ab::cost() > 0 )
      {
        // Drinking Horn Cover Legendary
        if ( p() -> legendary.drinking_horn_cover && p() -> buff.storm_earth_and_fire -> up() )
        {
          // Effect is saved as 6; duration is saved as 600 milliseconds
          double duration = p() -> legendary.drinking_horn_cover -> effectN( 1 ).base_value() * 100;
          double extension = duration * ab::cost();

          // Extend the duration of the buff
          p() -> buff.storm_earth_and_fire -> extend_duration( p(), timespan_t::from_millis( extension ) );

          // Extend the duration of pets
          if ( !p() -> pet.sef[SEF_EARTH] -> is_sleeping() )
            p() -> pet.sef[SEF_EARTH] -> expiration -> reschedule( p() -> pet.sef[SEF_EARTH] -> expiration -> remains() + timespan_t::from_millis( extension ) );
          if ( !p() -> pet.sef[SEF_FIRE] -> is_sleeping() )
            p() -> pet.sef[SEF_FIRE] -> expiration -> reschedule( p() -> pet.sef[SEF_FIRE] -> expiration -> remains() + timespan_t::from_millis( extension ) );
        }
        if ( p() -> legendary.the_emperors_capacitor )
          p() -> buff.emperors_electric_charge -> trigger( ab::cost() );
      }
      // Chi Savings on Dodge & Parry & Miss
      if ( ab::resource_consumed > 0 )
      {
        double chi_restored = ab::resource_consumed;
        if ( !ab::aoe && ab::result_is_miss( ab::execute_state -> result ) )
          p() -> resource_gain( RESOURCE_CHI, chi_restored, p() -> gain.chi_refund );
      }
    }

    // Energy refund, estimated at 80%
    if ( current_resource() == RESOURCE_ENERGY && ab::resource_consumed > 0 && ! ab::hit_any_target )
    {
      double energy_restored = ab::resource_consumed * 0.8;

      p() -> resource_gain( RESOURCE_ENERGY, energy_restored, p() -> gain.energy_refund );
    }

    if ( p() -> buff.serenity -> up() )
      p() -> gain.serenity -> add( RESOURCE_CHI, ab::base_costs[RESOURCE_CHI] - cost() );
  }

  void execute() override
  {
    ab::execute();

    trigger_storm_earth_and_fire( this );
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( p() -> artifact.gale_burst.rank() )
    {
      if ( td( s -> target ) -> dots.touch_of_death -> is_ticking() && s -> action -> name_str != "gale_burst" )
      {
        if ( s -> action -> harmful )
        {
          double gale_burst = s -> result_amount;

          if ( p() -> buff.storm_earth_and_fire -> up() )
            gale_burst *= 3;

          if ( td( s -> target ) -> debuff.gale_burst -> up() )
          {
            td( s -> target ) -> debuff.gale_burst -> current_value += gale_burst;

            if ( ab::sim -> debug )
            {
              ab::sim -> out_debug.printf( "%s added %.2f towards Gale Burst. Current Gale Burst amount that is saved up is %.2f.",
                  ab::player -> name(),
                  gale_burst,
                  td( s -> target ) -> debuff.gale_burst -> current_value );
            }
          }
        }
      }
    }
    ab::impact( s );
  }

  timespan_t gcd() const override
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

    if ( ! p() -> buff.storm_earth_and_fire -> up() )
    {
      return;
    }

    p() -> pet.sef[ SEF_EARTH ] -> trigger_attack( sef_ability, a );
    p() -> pet.sef[ SEF_FIRE  ] -> trigger_attack( sef_ability, a );
    // Trigger pet retargeting if sticky target is not defined, and the Monk used one of the Cyclone
    // Strike triggering abilities
    if ( ! p() -> pet.sef[ SEF_EARTH ] -> sticky_target &&
         ( sef_ability == SEF_TIGER_PALM || sef_ability == SEF_BLACKOUT_KICK ||
         sef_ability == SEF_RISING_SUN_KICK ) )
    {
      p() -> retarget_storm_earth_and_fire_pets();
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

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( p() -> specialization() == MONK_MISTWEAVER )
    {
      player_t* t = ( execute_state ) ? execute_state -> target : target;

      if ( td( t ) -> dots.enveloping_mist -> is_ticking() )
      {
        if ( p() -> internal_id != internal_id )
        {
          if ( p() -> talent.mist_wrap )
            am *= 1.0 + p() -> spec.enveloping_mist -> effectN( 2 ).percent() + p() -> talent.mist_wrap -> effectN( 2 ).percent();
          else
            am *= 1.0 + p() -> spec.enveloping_mist -> effectN( 2 ).percent();
        }
      }

      if ( p() -> buff.life_cocoon -> up() )
        am *= 1.0 + p() -> spec.life_cocoon -> effectN( 2 ).percent();

      if ( p() -> buff.extend_life -> up() )
        am *= 1.0 + p() -> buff.extend_life -> value();
    }

    return am;
  }

  double action_ta_multiplier() const override
  {
    double atm = base_t::action_ta_multiplier();

    if ( p() -> specialization() == MONK_MISTWEAVER )
    {
      if ( p() -> buff.the_mists_of_sheilun -> up() )
        atm *= 1.0 + p() -> buff.the_mists_of_sheilun -> value();
    }
    return atm;
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

// Eye of the Tiger ========================================================
struct eye_of_the_tiger_heal_tick_t : public monk_heal_t
{
  eye_of_the_tiger_heal_tick_t( monk_t& p, const std::string& name ):
    monk_heal_t( name, p, p.talent.eye_of_the_tiger -> effectN( 1 ).trigger() )
  {
    background = true;
    hasted_ticks = false;
    target = player;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p() -> specialization() == MONK_BREWMASTER )
    {
      am *= 1 + p() -> passives.aura_brewmaster_monk -> effectN( 5 ).percent();
    }

    return am;
  }
};

struct eye_of_the_tiger_dmg_tick_t: public monk_spell_t
{
  eye_of_the_tiger_dmg_tick_t( monk_t* player, const std::string& name ):
    monk_spell_t( name, player, player -> talent.eye_of_the_tiger -> effectN( 1 ).trigger() )
  {
    background = true;
    hasted_ticks = false;
    attack_power_mod.direct = 0;
    attack_power_mod.tick = data().effectN( 2 ).ap_coeff();
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    if ( p() -> specialization() == MONK_BREWMASTER )
    {
      am *= 1 + p() -> passives.aura_brewmaster_monk -> effectN( 5 ).percent();
    }
    return am;
  }
};

// Tiger Palm base ability ===================================================
struct tiger_palm_t: public monk_melee_attack_t
{
  heal_t* eye_of_the_tiger_heal;
  spell_t* eye_of_the_tiger_damage;
  tiger_palm_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "tiger_palm", p, p -> spec.tiger_palm ),
    eye_of_the_tiger_heal( new eye_of_the_tiger_heal_tick_t( *p, "eye_of_the_tiger_heal" ) ),
    eye_of_the_tiger_damage( new eye_of_the_tiger_dmg_tick_t( p, "eye_of_the_tiger_damage" ) )
  {
    parse_options( options_str );
    sef_ability = SEF_TIGER_PALM;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    add_child( eye_of_the_tiger_damage );
    add_child( eye_of_the_tiger_heal );

    if ( p -> specialization() == MONK_BREWMASTER )
      base_costs[RESOURCE_ENERGY] *= 1 + p -> spec.stagger -> effectN( 16 ).percent(); // -50% for Brewmasters

    if ( p -> specialization() == MONK_WINDWALKER )
    {
      energize_amount = p -> passives.aura_windwalker_monk -> effectN( 2 ).base_value();
    }
    else
      energize_type = ENERGIZE_NONE;

    spell_power_mod.direct = 0.0;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p() -> artifact.tiger_claws.rank() )
      am *= 1 + p() -> artifact.tiger_claws.percent();

    if ( p() -> specialization() == MONK_MISTWEAVER )
      am *= 1 + p() -> passives.aura_mistweaver_monk -> effectN( 11 ).percent();

    if ( p() -> specialization() == MONK_BREWMASTER )
    {
      am *= 1 + p() -> passives.aura_brewmaster_monk -> effectN( 4 ).percent();

      if ( p() -> artifact.face_palm.rank() )
      {
        if ( rng().roll( p() -> artifact.face_palm.percent() ) )
          am *= 1 + p() -> passives.face_palm -> effectN( 1 ).percent();
      }

      if ( p() -> buff.blackout_combo -> up() )
        am *= 1 + p() -> buff.blackout_combo -> data().effectN( 1 ).percent();
    }

    return am;
  }

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_TIGER_PALM );

    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    if ( p() -> talent.eye_of_the_tiger -> ok() )
    {
      eye_of_the_tiger_damage -> execute();
      eye_of_the_tiger_heal -> execute();
    }

    switch ( p() -> specialization() )
    {
      case MONK_MISTWEAVER:
      {
        p() -> buff.teachings_of_the_monastery -> trigger();
        break;
      }
      case MONK_WINDWALKER:
      {
        // Power Strike activation
        // Legion change = The buff will no longer summon a chi sphere at max chi. It will hold the buff until you can actually use the power strike
        // This means it will be used at 0, 1, or 2 chi
        if ( p() -> buff.power_strikes -> up() && ( p() -> resources.current[RESOURCE_CHI] < p() -> resources.max[RESOURCE_CHI] ) )
        {
          player -> resource_gain( RESOURCE_CHI, p() -> buff.power_strikes -> value(), p() -> gain.power_strikes, this );
          p() -> buff.power_strikes -> expire();
        }

        // Combo Breaker calculation
        if ( p() -> buff.bok_proc -> trigger() )
          p() -> proc.bok_proc -> occur();

        if ( p() -> buff.masterful_strikes -> up() )
          p() -> buff.masterful_strikes -> decrement();

        break;
      }
      case MONK_BREWMASTER:
      {
        // Reduces the remaining cooldown on your Brews by 1 sec
        double time_reduction = p() -> spec.tiger_palm -> effectN( 3 ).base_value();

        if ( p() -> artifact.face_palm.rank() )
        {
          if ( rng().roll( p() -> artifact.face_palm.percent() ) )
            time_reduction += p() -> passives.face_palm -> effectN( 2 ).base_value();
        }

        // 4 pieces (Brewmaster) : Tiger Palm reduces the remaining cooldown on your brews by an additional 1 sec.
        if ( p() -> sets.has_set_bonus( MONK_BREWMASTER, T19, B4 ) )
          time_reduction += p() -> sets.set( MONK_BREWMASTER, T19, B4 ) -> effectN( 1 ).base_value();

        // we need to adjust the cooldown time DOWNWARD instead of UPWARD so multiply the time_reduction by -1
        time_reduction *= -1;

        if ( p() -> cooldown.brewmaster_active_mitigation -> down() )
        {
          p() -> cooldown.brewmaster_active_mitigation -> adjust( timespan_t::from_seconds( time_reduction ), true );
        }

        if ( p() -> cooldown.fortifying_brew -> down() )
          p() -> cooldown.fortifying_brew -> adjust( timespan_t::from_seconds( time_reduction ), true );

        if ( p() -> cooldown.black_ox_brew -> down() )
          p() -> cooldown.black_ox_brew -> adjust( timespan_t::from_seconds( time_reduction ), true );

        if ( p() -> buff.blackout_combo -> up() )
          p() -> buff.blackout_combo -> expire();

        break;
      }
      default: break;
    }

    if ( p() -> sets.has_set_bonus( p() -> specialization(), T19OH, B8 ) )
      p() -> buff.tier19_oh_8pc -> trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    // Apply Mortal Wonds
    if ( p() -> specialization() == MONK_WINDWALKER && result_is_hit( s -> result ) && p() -> spec.spinning_crane_kick )
      p() -> trigger_mark_of_the_crane( s );
  }
};

// ==========================================================================
// Rising Sun Kick
// ==========================================================================

// Rising Sun Kick T18 Windwalker Trinket Proc ==============================
struct rising_sun_kick_proc_t : public monk_melee_attack_t
{
  rising_sun_kick_proc_t( monk_t* p ) :
    monk_melee_attack_t( "rising_sun_kick_trinket", p, p -> spec.rising_sun_kick -> effectN( 1 ).trigger() )
  {
    sef_ability = SEF_RISING_SUN_KICK_TRINKET;

    cooldown -> duration = timespan_t::zero();
    background = dual = true;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    trigger_gcd = timespan_t::zero();
  }

  bool init_finished() override
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

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p() -> artifact.rising_winds.rank() )
      am *= 1 + p() -> artifact.rising_winds.percent();

    return am;
  }

  virtual double cost() const override
  {
    return 0;
  }

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_RISING_SUN_KICK_TRINKET );

    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    // Activate A'Buraq's Trait
    if ( p() -> artifact.transfer_the_power.rank() )
      p() -> buff.transfer_the_power -> trigger();

    // TODO: This is a possible bug where it is removing a stack before adding 2 stacks
    if ( p() -> buff.masterful_strikes -> up() )
      p() -> buff.masterful_strikes -> decrement();

    if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T18, B2 ) )
      p() -> buff.masterful_strikes -> trigger( ( int ) p() -> sets.set( MONK_WINDWALKER,T18, B2 ) -> effect_count() );
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // Apply Mortal Wonds
      if ( p() -> spec.combat_conditioning )
        s -> target -> debuffs.mortal_wounds -> trigger();
      if ( p() -> spec.spinning_crane_kick )
        p() -> trigger_mark_of_the_crane( s );
    }
  }
};

// Rising Sun Kick Tornado Kick Windwalker Artifact Trait ==================
struct rising_sun_kick_tornado_kick_t : public monk_melee_attack_t
{

  rising_sun_kick_tornado_kick_t( monk_t* p ) :
    monk_melee_attack_t( "rising_sun_kick_tornado_kick", p, p -> spec.rising_sun_kick -> effectN( 1 ).trigger() )
  {
    may_miss = may_dodge = may_parry = may_crit = may_block = true;

    cooldown -> duration = timespan_t::zero();
    background = dual = true;
    trigger_gcd = timespan_t::zero();
  }

  void init() override
  {
    monk_melee_attack_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags &= ~STATE_AP;
    snapshot_flags |= STATE_CRIT;
    snapshot_flags |= STATE_TGT_ARMOR;

    update_flags &= STATE_NO_MULTIPLIER;
    update_flags &= ~STATE_AP;
    update_flags |= STATE_CRIT;
    update_flags |= STATE_TGT_ARMOR;
  }

  // Force 250 milliseconds for the animation, but not delay the overall GCD
  timespan_t execute_time() const override
  {
    return timespan_t::from_millis( 250 );
  }

  virtual double cost() const override
  {
    return 0;
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // Apply Mortal Wonds
      s -> target -> debuffs.mortal_wounds -> trigger();
      p() -> trigger_mark_of_the_crane( s );
    }
  }
};

// Rising Sun Kick Baseline ability =======================================
struct rising_sun_kick_t: public monk_melee_attack_t
{
  rising_sun_kick_proc_t* rsk_proc;
  rising_sun_kick_tornado_kick_t* rsk_tornado_kick;

  rising_sun_kick_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "rising_sun_kick", p, p -> spec.rising_sun_kick ),
    rsk_proc( new rising_sun_kick_proc_t( p ) ),
    rsk_tornado_kick( new rising_sun_kick_tornado_kick_t( p ) )
  {
    parse_options( options_str );

    if ( p -> specialization() == MONK_MISTWEAVER )
      cooldown -> duration += p -> passives.aura_mistweaver_monk -> effectN( 8 ).time_value();
    if ( p -> sets.has_set_bonus( MONK_WINDWALKER, T19, B2) )
      cooldown -> duration += p -> sets.set( MONK_WINDWALKER, T19, B2 ) -> effectN( 1 ).time_value();

    sef_ability = SEF_RISING_SUN_KICK;

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    attack_power_mod.direct = p -> spec.rising_sun_kick -> effectN( 1 ).trigger() -> effectN( 1 ).ap_coeff();
    spell_power_mod.direct = 0.0;

    if ( p -> artifact.tornado_kicks.rank() )
      add_child( rsk_tornado_kick );

    if ( p -> specialization() == MONK_WINDWALKER )
      add_child( rsk_proc );
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p() -> artifact.rising_winds.rank() )
      am *= 1 + p() -> artifact.rising_winds.percent();

    return am;
  }

  virtual void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    // Windwalker Tier 18 (WoD 6.2) trinket effect is in use, adjust Rising Sun Kick proc chance based on spell data
    // of the special effect.
    // Not usable at level 110
    if ( p() -> furious_sun && p() -> level() < 110 )
    {
      double proc_chance = p() -> furious_sun -> driver() -> effectN( 1 ).average( p() -> furious_sun -> item ) / 100.0;

      if ( rng().roll( proc_chance ) )
        rsk_proc -> execute();
    }
  }

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_RISING_SUN_KICK );

    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    switch ( p() -> specialization() )
    {
      case MONK_MISTWEAVER:
      {
        if ( p() -> talent.rising_thunder -> ok() )
          p() -> cooldown.thunder_focus_tea -> reset( true );
        break;
      }
      case MONK_WINDWALKER:
      {
        // Activate A'Buraq's Trait
        if ( p() -> artifact.transfer_the_power.rank() )
          p() -> buff.transfer_the_power -> trigger();

        // TODO: This is a possible bug where it is removing a stack before adding 2 stacks
        if ( p() -> buff.masterful_strikes -> up() )
          p() -> buff.masterful_strikes -> decrement();

        if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T18, B2 ) )
          p() -> buff.masterful_strikes -> trigger( ( int ) p() -> sets.set( MONK_WINDWALKER,T18, B2 ) -> effect_count() );
        break;
      }
      default: break;
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact(s);

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> buff.teachings_of_the_monastery -> up() )
      {
        p() -> buff.teachings_of_the_monastery -> expire();
        // Spirit of the Crane does not have a buff associated with it. Since
        // this is tied somewhat with Teachings of the Monastery, tacking
        // this onto the removal of that buff.
        if ( p() -> talent.spirit_of_the_crane -> ok() )
          p() -> resource_gain( RESOURCE_MANA, ( p() -> resources.max[RESOURCE_MANA] * p() -> passives.spirit_of_the_crane -> effectN( 1 ).percent() ), p() -> gain.spirit_of_the_crane );
      }

      // Apply Mortal Wonds
      if ( p() -> specialization() == MONK_WINDWALKER )
      {
        s -> target -> debuffs.mortal_wounds -> trigger();

        if ( p() -> artifact.tornado_kicks.rank() )
        {
          double raw = s -> result_raw * p() -> artifact.tornado_kicks.data().effectN( 1 ).percent();
          rsk_tornado_kick -> target = s -> target;
          rsk_tornado_kick -> base_dd_max = raw;
          rsk_tornado_kick -> base_dd_min = raw;
          rsk_tornado_kick -> execute();
        }
      }
    }
  }
};

// ==========================================================================
// Blackout Kick
// ==========================================================================

// Blackout Kick Proc from Teachings of the Monastery =======================
struct blackout_kick_totm_proc : public monk_melee_attack_t
{
  blackout_kick_totm_proc( monk_t* p ) :
    monk_melee_attack_t( "blackout_kick_totm_proc", p, p -> passives.totm_bok_proc )
  {
    cooldown -> duration = timespan_t::zero();
    background = dual = true;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    trigger_gcd = timespan_t::zero();
  }

  bool init_finished() override
  {
    bool ret = monk_melee_attack_t::init_finished();
    action_t* bok = player -> find_action( "blackout_kick" );
    if ( bok )
    {
      base_multiplier = bok -> base_multiplier;
      spell_power_mod.direct = bok -> spell_power_mod.direct;

      bok -> add_child( this );
    }

    return ret;
  }

  // Force 100 milliseconds for the animation, but not delay the overall GCD
  timespan_t execute_time() const override
  {
    return timespan_t::from_millis( 100 );
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p() -> passives.aura_mistweaver_monk -> effectN( 10 ).percent();

    return am;
  }

  virtual double cost() const override
  {
    return 0;
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    if ( rng().roll( p() -> spec.teachings_of_the_monastery -> effectN( 1 ).percent() ) )
        p() -> cooldown.rising_sun_kick -> reset( true );
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p() -> talent.spirit_of_the_crane -> ok() )
      p() -> resource_gain( RESOURCE_MANA, ( p() -> resources.max[RESOURCE_MANA] * p() -> passives.spirit_of_the_crane -> effectN( 1 ).percent() ), p() -> gain.spirit_of_the_crane );
  }
};

struct blackout_kick_t: public monk_melee_attack_t
{
  rising_sun_kick_proc_t* rsk_proc;
  blackout_kick_totm_proc* bok_totm_proc;

  blackout_kick_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "blackout_kick", p, ( p -> specialization() == MONK_BREWMASTER ? spell_data_t::nil() : p -> spec.blackout_kick ) )
  {
    parse_options( options_str );


    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    spell_power_mod.direct = 0.0;
    switch ( p -> specialization() )
    {
      case MONK_MISTWEAVER:
      {
        bok_totm_proc = new blackout_kick_totm_proc( p );
        break;
      }
      case MONK_WINDWALKER:
      {
        rsk_proc = new rising_sun_kick_proc_t( p );
        break;
      }
      default:
        break;
    }

    sef_ability = SEF_BLACKOUT_KICK;
  }

  virtual bool ready() override
  {
    if ( p() -> specialization() == MONK_BREWMASTER )
      return false;

    return monk_melee_attack_t::ready();
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    switch ( p() -> specialization() )
    {
      case MONK_MISTWEAVER:
      {
        am *= 1 + p() -> passives.aura_mistweaver_monk -> effectN( 10 ).percent();
        break;
      }
      case MONK_WINDWALKER:
      {
        if ( p() -> artifact.dark_skies.rank() )
          am *= 1 + p() -> artifact.dark_skies.percent();

        break;
      }
      default: break;
    }
    return am;
  }

  virtual void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    if ( p() -> buff.bok_proc -> up() )
    {
      p() -> buff.bok_proc -> expire();
      p() -> gain.bok_proc -> add( RESOURCE_CHI, base_costs[RESOURCE_CHI] );
    }

    // Windwalker Tier 18 (WoD 6.2) trinket effect is in use, adjust Rising Sun Kick proc chance based on spell data
    // of the special effect.
    // Not usable at level 110
    if ( p() -> furious_sun && p() -> level() < 110 )
    {
      double proc_chance = p() -> furious_sun -> driver() -> effectN( 1 ).average( p() -> furious_sun -> item ) / 100.0;

      if ( rng().roll( proc_chance ) )
        rsk_proc -> execute();
    }
  }

  void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_BLACKOUT_KICK );

    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    switch ( p() -> specialization() )
    {
      case MONK_MISTWEAVER:
      {
        if ( rng().roll( p() -> spec.teachings_of_the_monastery -> effectN( 1 ).percent() ) )
          p() -> cooldown.rising_sun_kick -> reset( true );
        break;
      }
      case MONK_WINDWALKER:
      {
        if ( p() -> buff.masterful_strikes -> up() )
          p() -> buff.masterful_strikes -> decrement();

        if ( p() -> artifact.transfer_the_power.rank() )
          p() -> buff.transfer_the_power -> trigger();
        break;
      }
      default: break;
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // Apply Dizzing Kick debuff onto the target if talented
      if ( p() -> talent.dizzying_kicks -> ok() )
        td( s -> target ) -> debuff.dizzing_kicks -> trigger();

      if ( p() -> specialization() == MONK_WINDWALKER && p() -> spec.spinning_crane_kick )
        p() -> trigger_mark_of_the_crane( s );

      if ( p() -> buff.teachings_of_the_monastery -> up() )
      {
        int stacks = p() -> buff.teachings_of_the_monastery -> current_stack;
        p() -> buff.teachings_of_the_monastery -> expire();

        for (int i = 0; i < stacks; i++ )
          bok_totm_proc -> execute();

      }
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

    switch ( p -> specialization() )
    {
      case MONK_BREWMASTER:
      {
        if ( p -> artifact.obsidian_fists.rank() )
          base_crit += p -> artifact.obsidian_fists.percent();

        break;
      }
      default: break;
    }
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    // Mistweavers cannot learn this spell. However the effect to adjust this spell is in the database.
    // Just being a completionist about this.
    if ( p() -> specialization() == MONK_MISTWEAVER )
    {
      am *= 1 + p() -> passives.aura_mistweaver_monk -> effectN( 10 ).percent();
    }
    return am;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p() -> talent.blackout_combo -> ok() )
      p() -> buff.blackout_combo -> execute();
  }
};

// ==========================================================================
// SCK/RJW Tick Info
// ==========================================================================

// Shared tick action for both abilities ====================================
struct tick_action_t : public monk_melee_attack_t
{
  tick_action_t( const std::string& name, monk_t* p, const spell_data_t* data ) :
    monk_melee_attack_t( name, p, data )
  {
    dual = background = true;
    aoe = -1;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    radius = data -> effectN( 1 ).radius();

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
    tick_zero = true;

    spell_power_mod.direct = 0.0;
    dot_behavior = DOT_REFRESH; // Spell uses Pandemic Mechanics.

    tick_action = new tick_action_t( "rushing_jade_wind_tick", p, p -> talent.rushing_jade_wind -> effectN( 1 ).trigger() );
  }

  void init() override
  {
    monk_melee_attack_t::init();

    update_flags &= ~STATE_HASTE;
  }

  // N full ticks, but never additional ones.
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    if ( p() -> specialization() == MONK_BREWMASTER )
    {
      pm *= 1 + p() -> passives.aura_brewmaster_monk -> effectN( 3 ).percent();
    }

    return pm;
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    return am;
  }

  void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_RUSHING_JADE_WIND );

    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    p() -> buff.rushing_jade_wind -> trigger( 1,
        buff_t::DEFAULT_VALUE(),
        1.0,
        composite_dot_duration( execute_state ) );

    if ( p() -> buff.masterful_strikes -> up() )
       p() -> buff.masterful_strikes -> decrement();

    if ( p() -> artifact.transfer_the_power.rank() )
      p() -> buff.transfer_the_power -> trigger();
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
    tick_zero = interrupt_auto_attack = true;

    spell_power_mod.direct = 0.0;
    dot_behavior = DOT_REFRESH; // Spell uses Pandemic Mechanics.

    tick_action = new tick_action_t( "spinning_crane_kick_tick", p, p -> spec.spinning_crane_kick -> effectN( 1 ).trigger() );
  }

  // N full ticks, but never additional ones.
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  int mark_of_the_crane_counter() const
  {
    std::vector<player_t*> targets = target_list();
    int mark_of_the_crane_counter = 0;

    if ( p() -> specialization() == MONK_WINDWALKER )
    {
      for ( player_t* target : targets )
      {
        if ( td( target ) -> debuff.mark_of_the_crane -> up() )
          mark_of_the_crane_counter++;
      }
    }
    return mark_of_the_crane_counter;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    pm *= 1 + ( mark_of_the_crane_counter() * p() -> spec.spinning_crane_kick -> effectN( 2 ).percent() );

    if ( p() -> artifact.power_of_a_thousand_cranes.rank() )
      pm *= 1 + p() -> artifact.power_of_a_thousand_cranes.percent();

    if ( p() -> specialization() == MONK_MISTWEAVER )
      pm *= 1 + p() -> passives.aura_mistweaver_monk -> effectN( 12 ).percent();

    return pm;
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();


    return am;
  }

  void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_SPINNING_CRANE_KICK );

    monk_melee_attack_t::execute();

    p() -> buff.spinning_crane_kick -> trigger( 1,
        buff_t::DEFAULT_VALUE(),
        1.0,
        composite_dot_duration( execute_state ) );
  }

  virtual void last_tick( dot_t* dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    if ( p() -> buff.masterful_strikes -> up() )
      p() -> buff.masterful_strikes -> decrement();
  }
};

// ==========================================================================
// Fists of Fury
// ==========================================================================

// Crosswinds Artifact Trait ===============================================
/* When you activate FoF, you get a hidden buff on yourself that shoots a wind spirit guy at a random 
   target that is being hit by your FoF, regardless of how many people are being hit by FoF. If you 
   FoF one target, they all hit that guy. If you FoF 5 targets, each wind spirit hits a random one 
   of the 5 guys. it just rolls Random(1, (number of targets)), and picks that guy.
*/
/* Crosswinds triggers 5 times over 2.5-3 second period; even though the duration of the buff is 4 
   seconds. Trigger happens after the first tick of Fists of Fury but does not have a zero tick.
*/
struct crosswinds_tick_t : public monk_melee_attack_t
{
  crosswinds_tick_t( monk_t* p ) :
    monk_melee_attack_t( "crosswinds_tick", p, p -> passives.crosswinds_dmg )
  {
    background = dual = true;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    base_costs[ RESOURCE_CHI ] = 0;
    dot_duration = timespan_t::zero();
    trigger_gcd = timespan_t::zero();
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p() -> buff.storm_earth_and_fire -> up() )
      am *= 3;

    return am;
  }
};

struct crosswinds_t : public monk_melee_attack_t
{
  crosswinds_t( monk_t* p ) :
    monk_melee_attack_t( "crosswinds", p, p -> passives.crosswinds_trigger )
  {
    background = dual = true; 
    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = hasted_ticks = tick_zero = false;
    channeled = false;

    tick_action = new crosswinds_tick_t( p );
  }
};

struct fists_of_fury_tick_t: public monk_melee_attack_t
{
  fists_of_fury_tick_t( monk_t* p, const std::string& name ):
    monk_melee_attack_t( name, p, p -> spec.fists_of_fury )
  {
    background = true;
    aoe = -1;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    attack_power_mod.direct = p -> spec.fists_of_fury -> effectN( 5 ).ap_coeff();
    base_costs[ RESOURCE_CHI ] = 0;
    dot_duration = timespan_t::zero();
    trigger_gcd = timespan_t::zero();
  }
};

struct fists_of_fury_t: public monk_melee_attack_t
{
  rising_sun_kick_proc_t* rsk_proc;
  crosswinds_t* crosswinds;

  fists_of_fury_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "fists_of_fury", p, p -> spec.fists_of_fury ),
    crosswinds( new crosswinds_t( p ) )
  {
    parse_options( options_str );

    sef_ability = SEF_FISTS_OF_FURY;

    channeled = tick_zero = true;
    interrupt_auto_attack = true;
    // Effect 1 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
    base_tick_time = dot_duration / 4;
    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

    attack_power_mod.direct = 0.0;
    spell_power_mod.direct = 0.0;

    tick_action = new fists_of_fury_tick_t( p, "fists_of_fury_tick" );

    rsk_proc = new rising_sun_kick_proc_t( p );

    if ( p -> artifact.crosswinds.rank() )
      add_child( crosswinds );
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    if ( p() -> buff.transfer_the_power -> up() )
    {
      pm *= 1 + p() -> buff.transfer_the_power -> stack_value();
    }

    if ( p() -> artifact.fists_of_the_wind.rank() )
      pm *= 1 + p() -> artifact.fists_of_the_wind.percent();

    return pm;
  }

  virtual bool ready() override
  {
    // Only usable with 1-handed weapons
    if ( p() -> main_hand_weapon.type > WEAPON_1H || p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;
    
    return monk_melee_attack_t::ready();
  }

  virtual double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p() -> legendary.katsuos_eclipse )
      c += p() -> legendary.katsuos_eclipse -> effectN( 1 ).base_value(); // saved as -2

    return c;
  }

  void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_FISTS_OF_FURY );

    monk_melee_attack_t::execute();

    if ( p() -> artifact.crosswinds.rank() )
      crosswinds -> execute();

    if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T17, B2 ) )
    {
      // Since Serenity replaces Tigereye Brew, adjust Serenity's cooldown first.
      if ( p() -> talent.serenity )
        p() -> cooldown.serenity -> adjust( timespan_t::from_seconds( p() -> sets.set( MONK_WINDWALKER, T17, B2 ) -> effectN( 1 ).base_value() ) );
    }

    if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T18, B4 ) )
      p() -> buff.masterful_strikes -> trigger( ( int ) p() -> sets.set( MONK_WINDWALKER,T18, B4 ) -> effect_count() - 1 );
  }

  virtual void last_tick( dot_t* dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    // This is not when this happens but putting this here so that it's able to be checked by SEF
    if ( p() -> buff.transfer_the_power -> up() )
      p() -> buff.transfer_the_power -> expire();

    // Windwalker Tier 18 (WoD 6.2) trinket effect is in use, adjust Rising Sun Kick proc chance based on spell data
    // of the special effect.
    // Not usable at level 110
    if ( p() -> furious_sun && p() -> level() < 110 )
    {
      double proc_chance = p() -> furious_sun -> driver() -> effectN( 1 ).average( p() -> furious_sun -> item ) / 100.0;

      if ( rng().roll( proc_chance ) )
        rsk_proc -> execute();
    }
  }
};

// ==========================================================================
// Whirling Dragon Punch
// ==========================================================================

struct whirling_dragon_punch_tick_t: public monk_melee_attack_t
{
  whirling_dragon_punch_tick_t(const std::string& name, monk_t* p, const spell_data_t* s) :
    monk_melee_attack_t( name, p, s )
  {
    background = true;
    aoe = -1;
    radius = s -> effectN( 1 ).radius();

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::zero();
  }
};

struct whirling_dragon_punch_t: public monk_melee_attack_t
{

  whirling_dragon_punch_t(monk_t* p, const std::string& options_str) :
    monk_melee_attack_t( "whirling_dragon_punch", p, p -> talent.whirling_dragon_punch )
  {
    sef_ability = SEF_WHIRLING_DRAGON_PUNCH;

    parse_options( options_str );
    interrupt_auto_attack = callbacks = false;
    channeled = true;
    dot_duration = data().duration();
    tick_zero = false;

    spell_power_mod.direct = 0.0;

    tick_action = new whirling_dragon_punch_tick_t( "whirling_dragon_punch_tick", p, p -> passives.whirling_dragon_punch );
  }

  virtual bool ready() override
  {
    // Only usable while Fists of Fury and Rising Sun Kick are on cooldown.
    if ( p() -> cooldown.fists_of_fury -> down() && p() -> cooldown.rising_sun_kick -> down() )
      return monk_melee_attack_t::ready();

    return false;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t tt = tick_time( s );
    return tt * 3;
  }

  void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_WHIRLING_DRAGON_PUNCH );

    monk_melee_attack_t::execute();
  }
};


// ==========================================================================
// Strike of the Windlord
// ==========================================================================
// Off hand hits first followed by main hand

struct strike_of_the_windlord_off_hand_t: public monk_melee_attack_t
{
  strike_of_the_windlord_off_hand_t( monk_t* p, const char* name, const spell_data_t* s ):
    monk_melee_attack_t( name, p, s )
  {
    sef_ability = SEF_STRIKE_OF_THE_WINDLORD_OH;
    may_dodge = may_parry = may_block = may_miss = true;
    dual = true;
    weapon = &( p -> off_hand_weapon );
    aoe = -1;
    radius = data().effectN( 2 ).base_value();
    // Force the Sim to use damage range
    sim -> average_range = 0;

    if ( sim -> pvp_crit )
      base_multiplier *= 0.70; // 08/03/2016
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    if ( state -> target != target )
    {
      return 1.0 / state -> n_targets;
    }

    return 1.0;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

};

struct strike_of_the_windlord_t: public monk_melee_attack_t
{
  strike_of_the_windlord_off_hand_t* oh_attack;
  strike_of_the_windlord_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "strike_of_the_windlord", p, &( p -> artifact.strike_of_the_windlord.data() ) ),
    oh_attack( nullptr )
  {
    sef_ability = SEF_STRIKE_OF_THE_WINDLORD;

    parse_options( options_str );
    may_dodge = may_parry = may_block = true;
    aoe = -1;
    weapon_multiplier = data().effectN( 3 ).trigger() -> effectN( 1 ).percent();
    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    radius = data().effectN( 3 ).trigger() -> effectN( 2 ).base_value();
    // Force the sim to use damage range
    sim -> average_range = 0;

    oh_attack = new strike_of_the_windlord_off_hand_t( p, "strike_of_the_windlord_offhand", data().effectN( 4 ).trigger() );
    add_child( oh_attack );

    if ( sim -> pvp_crit )
      base_multiplier *= 0.70; // 08/03/2016
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    if ( state -> target != target )
    {
      return 1.0 / state -> n_targets;
    }

    return 1.0;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_melee_attack_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return monk_melee_attack_t::ready();
  }

  void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_STRIKE_OF_THE_WINDLORD );

    monk_melee_attack_t::execute(); // this is the MH attack

    if ( oh_attack && result_is_hit( execute_state -> result ) &&
         p() -> off_hand_weapon.type != WEAPON_NONE ) // If MH fails to land, OH does not execute.
      oh_attack -> execute();
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

    if ( player -> main_hand_weapon.group() == WEAPON_1H )
    {
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

  bool ready() override
  {
    if ( p() -> current.distance_to_move > 5 )
      return false;

    return( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }

  virtual void execute() override
  {
    if ( player -> main_hand_attack )
      p() -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
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

    if ( p.artifact.smashed.rank() )
      radius += p.artifact.smashed.value();

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    cooldown -> duration = p.spec.keg_smash -> cooldown();
  }

  virtual bool ready() override
  {
    // Secret Ingredients allows Tiger Palm to have a 30% chance to reset the cooldown of Keg Smash
    if ( p() -> buff.keg_smash_talent -> check() )
      return true;

    return monk_melee_attack_t::ready();
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p() -> artifact.full_keg.rank() )
      am *= 1 + p() -> artifact.full_keg.percent();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    td( s -> target ) -> debuff.keg_smash -> trigger();
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    // If cooldown was reset by Secret Ingredients talent, to end the buff
    if ( p() -> buff.keg_smash_talent -> check() )
      p() -> buff.keg_smash_talent -> expire();
    
    // Reduces the remaining cooldown on your Brews by 4 sec.
    double time_reduction = p() -> spec.keg_smash -> effectN( 3 ).base_value();

    // Blackout Combo talent reduces Brew's cooldown by 2 sec.
    if ( p() -> buff.blackout_combo -> up() )
    {
      time_reduction += p() -> buff.blackout_combo -> data().effectN( 3 ).base_value();
      p() -> buff.blackout_combo -> expire();
    }

    // we need to adjust the cooldown time DOWNWARD instead of UPWARD so multiply the time_reduction by -1
    time_reduction *= -1;

    if ( p() -> cooldown.brewmaster_active_mitigation -> down() )
    {
      p() -> cooldown.brewmaster_active_mitigation -> adjust( timespan_t::from_seconds( time_reduction ), true );
    }

    if ( p() -> cooldown.fortifying_brew -> down() )
      p() -> cooldown.fortifying_brew -> adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p() -> cooldown.black_ox_brew -> down() )
      p() -> cooldown.black_ox_brew -> adjust( timespan_t::from_seconds( time_reduction ), true );
  }
};

// ==========================================================================
// Exploding Keg
// ==========================================================================

struct exploding_keg_t: public monk_melee_attack_t
{

  exploding_keg_t( monk_t& p, const std::string& options_str ):
    monk_melee_attack_t( "exploding_keg", &p, p.artifact.exploding_keg )
  {
    parse_options( options_str );

    aoe = -1;

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    p() -> buff.exploding_keg -> trigger();
  }
};

// ==========================================================================
// Touch of Death
// ==========================================================================
// Gale Burst will not show in the combat log. Damage will be added directly to Touch of Death
// However I am added Gale Burst as a child of Touch of Death for statistics reasons.

struct gale_burst_t: public monk_spell_t
{
  gale_burst_t( monk_t* p ) :
    monk_spell_t( "gale_burst", p, p -> artifact.gale_burst.data().effectN( 1 ).trigger() )
  {
    background = true;
    may_crit = false;
    school = SCHOOL_PHYSICAL;
  }

  void init() override
  {
    monk_spell_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct touch_of_death_t: public monk_spell_t
{
  gale_burst_t* gale_burst;
  touch_of_death_t( monk_t* p, const std::string& options_str ):
    monk_spell_t( "touch_of_death", p, p -> spec.touch_of_death ),
    gale_burst( new gale_burst_t( p ) )
  {
    may_crit = hasted_ticks = false;
    parse_options( options_str );
    school = SCHOOL_PHYSICAL;

    if ( p -> artifact.gale_burst.rank() )
      add_child( gale_burst );
  }

  virtual bool ready() override
  {
    // Cannot be used on a target that has Touch of Death on them already
    if ( td( p() -> target ) -> dots.touch_of_death -> is_ticking() )
      return false;

    return monk_spell_t::ready();
  }

  double target_armor( player_t* ) const override { return 0; }

  double calculate_tick_amount( action_state_t*, double /*dot_multiplier*/ ) const override
  {
    double amount = p() -> resources.max[RESOURCE_HEALTH];

    amount *= p() -> spec.touch_of_death -> effectN( 2 ).percent(); // 50% HP
 
    if ( p() -> buff.combo_strikes -> up() )
      amount *= 1 + p() -> cache.mastery_value();

    return amount;
  }

 void last_tick( dot_t* dot ) override
  {
    if ( p() -> artifact.gale_burst.rank() && td( p() -> target ) -> debuff.gale_burst -> up() )
    {
      gale_burst -> base_dd_min = td( p() -> target ) -> debuff.gale_burst -> current_value * p() -> artifact.gale_burst.percent();
      gale_burst -> base_dd_max = td( p() -> target ) -> debuff.gale_burst -> current_value * p() -> artifact.gale_burst.percent();

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s executed '%s'. Amount sent before modifiers is %.2f.",
            player -> name(),
            gale_burst -> name(),
            td( p() -> target ) -> debuff.gale_burst -> current_value );
      }

      gale_burst -> target = dot -> target;
      gale_burst -> execute();

      td( p() -> target ) -> debuff.gale_burst -> expire();
    }
    monk_spell_t::last_tick( dot );
}

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_TOUCH_OF_DEATH );

    monk_spell_t::execute();

    if ( p() -> legendary.hidden_masters_forbidden_touch )
    {
      if ( p() -> buff.hidden_masters_forbidden_touch -> up() )
        p() -> buff.hidden_masters_forbidden_touch -> expire();
      else
      {
        p() -> buff.hidden_masters_forbidden_touch -> execute();
        this -> cooldown -> reset( true );
      }
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> artifact.gale_burst.rank() )
      {
        td( s -> target ) -> debuff.gale_burst -> trigger();
        td( s -> target ) -> debuff.gale_burst -> current_value = 0;
      }
    }
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
    if ( p -> legendary.cenedril_reflector_of_hatred )
      max_pct += p -> legendary.cenedril_reflector_of_hatred -> effectN( 1 ).percent();
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

// Spear Hand Strike ========================================================

struct spear_hand_strike_t: public monk_melee_attack_t
{
  spear_hand_strike_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "spear_hand_strike", p, p -> spec.spear_hand_strike )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }
};
} // END melee_attacks NAMESPACE

namespace spells {

// ==========================================================================
// Energizing Elixir
// ==========================================================================

struct energizing_elixir_t: public monk_spell_t
{
  energizing_elixir_t(monk_t* player, const std::string& options_str) :
    monk_spell_t( "energizing_elixir", player, player -> talent.energizing_elixir )
  {
    parse_options( options_str );

    dot_duration = trigger_gcd = timespan_t::zero();
    may_miss = may_crit = harmful = false;
    energize_type = ENERGIZE_NONE; // disable resource gain from spell data
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    p() -> resource_gain( RESOURCE_ENERGY, p() -> talent.energizing_elixir -> effectN( 1 ).base_value(), p() -> gain.energizing_elixir_energy );
    p() -> resource_gain( RESOURCE_CHI, p() -> talent.energizing_elixir -> effectN( 2 ).base_value(), p() -> gain.energizing_elixir_chi );
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
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();
    energize_type = ENERGIZE_NONE; // disable resource gain from spell data
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    // Refill Ironskin Brew and Purifying Brew charges.
    p() -> cooldown.brewmaster_active_mitigation -> reset( true, true );

    p() -> resource_gain( RESOURCE_ENERGY, p() -> talent.black_ox_brew -> effectN( 1 ).base_value(), p() -> gain.black_ox_brew_energy );
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

    // Executing Serenity reduces any current cooldown by 50%
    // Have to manually adjust each of the affected spells
    double percent_adjust = p() -> talent.serenity -> effectN( 4 ).percent(); // saved as -50%

    cooldown_reduction( p() -> cooldown.blackout_kick, percent_adjust );

    cooldown_reduction( p() -> cooldown.blackout_strike, percent_adjust );

    cooldown_reduction( p() -> cooldown.rushing_jade_wind, percent_adjust );

    cooldown_reduction( p() -> cooldown.refreshing_jade_wind, percent_adjust );

    cooldown_reduction( p() -> cooldown.rising_sun_kick, percent_adjust );

    cooldown_reduction( p() -> cooldown.fists_of_fury, percent_adjust );

    cooldown_reduction( p() -> cooldown.strike_of_the_windlord, percent_adjust );
  }

  void cooldown_reduction(cooldown_t* cd, double percent_adjust )
  {
    if ( cd -> down() )
      cd -> adjust( cd -> remains() * percent_adjust);
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

struct storm_earth_and_fire_t: public monk_spell_t
{
  storm_earth_and_fire_t( monk_t* p, const std::string& options_str ):
    monk_spell_t( "storm_earth_and_fire", p, p -> spec.storm_earth_and_fire )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    callbacks = harmful = may_miss = may_crit = may_dodge = may_parry = may_block = false;
    cooldown -> charges += p -> spec.storm_earth_and_fire_2 -> effectN( 1 ).base_value();
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    // While pets are up, don't trigger cooldown since the sticky targeting does not consume charges
    if ( p() -> buff.storm_earth_and_fire -> check() )
    {
      cd_duration = timespan_t::zero();
    }

    monk_spell_t::update_ready( cd_duration );
  }

  bool ready() override
  {
    if ( p() -> talent.serenity -> ok() )
      return false;

    // Don't let user needlessly trigger SEF sticky targeting mode, if the user would just be
    // triggering it on the same sticky target
    if ( p() -> buff.storm_earth_and_fire -> check() &&
         ( p() -> pet.sef[ SEF_EARTH ] -> sticky_target &&
         target == p() -> pet.sef[ SEF_EARTH ] -> target ) )
    {
      return false;
    }

    return monk_spell_t::ready();
  }

  // Normal summon that summons the pets, they seek out proper targeets
  void normal_summon()
  {
    auto targets = p() -> create_storm_earth_and_fire_target_list();
    auto n_targets = targets.size();

    // Start targeting logic from "owner" always
    p() -> pet.sef[ SEF_EARTH ] -> reset_targeting();
    p() -> pet.sef[ SEF_EARTH ] -> target = p() -> target;
    p() -> retarget_storm_earth_and_fire( p() -> pet.sef[ SEF_EARTH ], targets, n_targets );
    p() -> pet.sef[ SEF_EARTH ] -> summon( data().duration() );

    // Start targeting logic from "owner" always
    p() -> pet.sef[ SEF_FIRE ] -> reset_targeting();
    p() -> pet.sef[ SEF_FIRE ] -> target = p() -> target;
    p() -> retarget_storm_earth_and_fire( p() -> pet.sef[ SEF_FIRE ], targets, n_targets );
    p() -> pet.sef[ SEF_FIRE ] -> summon( data().duration() );
  }

  // Monk used SEF while pets are up to sticky target them into an enemy
  void sticky_targeting()
  {
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s storm_earth_and_fire sticky target %s to %s (old=%s)",
        player -> name(), p() -> pet.sef[ SEF_EARTH ] -> name(), target -> name(),
        p() -> pet.sef[ SEF_EARTH ] -> target -> name() );
    }

    p() -> pet.sef[ SEF_EARTH ] -> target = target;
    p() -> pet.sef[ SEF_EARTH ] -> sticky_target = true;

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s storm_earth_and_fire sticky target %s to %s (old=%s)",
        player -> name(), p() -> pet.sef[ SEF_FIRE ] -> name(), target -> name(),
        p() -> pet.sef[ SEF_FIRE ] -> target -> name() );
    }

    p() -> pet.sef[ SEF_FIRE ] -> target = target;
    p() -> pet.sef[ SEF_FIRE ] -> sticky_target = true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( ! p() -> buff.storm_earth_and_fire -> check() )
    {
      normal_summon();
    }
    else
    {
      sticky_targeting();
    }
  }
};

// Callback to retarget Storm Earth and Fire pets when new target appear, or old targets depsawn
// (i.e., die).
struct sef_despawn_cb_t
{
  monk_t* monk;

  sef_despawn_cb_t( monk_t* m ) : monk( m )
  { }

  void operator()( player_t* )
  {
    // No pets up, don't do anything
    if ( ! monk -> buff.storm_earth_and_fire -> check() )
    {
      return;
    }

    auto targets = monk -> create_storm_earth_and_fire_target_list();
    auto n_targets = targets.size();

    // If the active clone's target is sleeping, reset it's targeting, and jump it to a new target.
    // Note that if sticky targeting is used, both targets will jump (since both are going to be
    // stickied to the dead target)
    range::for_each( monk -> pet.sef, [ this, &targets, &n_targets ]( pets::storm_earth_and_fire_pet_t* pet ) {

      // Arise time went negative, so the target is sleeping. Can't check "is_sleeping" here, because
      // the callback is called before the target goes to sleep.
      if ( pet -> target -> arise_time < timespan_t::zero() )
      {
        pet -> reset_targeting();
        monk -> retarget_storm_earth_and_fire( pet, targets, n_targets );
      }
      else
      {
        // Retarget pets otherwise (a new target has appeared). Note that if the pets are sticky
        // targeted, this will do nothing.
        monk -> retarget_storm_earth_and_fire( pet, targets, n_targets );
      }
    } );
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
    sef_ability = SEF_CRACKLING_JADE_LIGHTNING;

    parse_options( options_str );

    channeled = tick_may_crit = interrupt = true;
    hasted_ticks = false; // Channeled spells always have hasted ticks. Use hasted_ticks = false to disable the increase in the number of ticks.
    interrupt_auto_attack = true;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_spell_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  virtual double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    if ( p() -> specialization() == MONK_MISTWEAVER )
      am *= 1 + p() -> passives.aura_mistweaver_monk -> effectN( 13 ).percent();

    if ( p() -> buff.emperors_electric_charge -> up() )
    {
      am *= 1 + p() -> buff.emperors_electric_charge -> stack_value();
      p() -> buff.emperors_electric_charge -> expire();
    }

    return am;
  }

  virtual void execute() override
  {
    combo_strikes_trigger( CS_CRACKLING_JADE_LIGHTNING );

    monk_spell_t::execute();
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
// Chi Orbit
// ==========================================================================

struct chi_orbit_t: public monk_spell_t
{
  chi_orbit_t( monk_t* p ):
    monk_spell_t( "chi_orbit", p, p -> talent.chi_orbit )
  {
    background = true;
    attack_power_mod.direct = p -> passives.chi_orbit -> effectN( 1 ).ap_coeff();
    aoe = -1;
    school = p -> passives.chi_orbit -> get_school_type();
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

struct dragonfire_brew : public monk_spell_t
{
  struct dragonfire_brew_tick : public monk_spell_t
  {
    dragonfire_brew_tick( monk_t& p ) :
      monk_spell_t( "dragonfire_brew_tick", &p, p.passives.dragonfire_brew_damage )
    {
      background = true;
      tick_may_crit = may_crit = true;
      hasted_ticks = false;
      aoe = -1;
    }
  };

  dragonfire_brew( monk_t& p ) :
    monk_spell_t( "dragonfire_brew", &p, p.artifact.dragonfire_brew )
  {
    background = true;
    tick_may_crit = may_crit = false;
    hasted_ticks = false;
    // Placeholder stuff to get things working
    dot_duration = timespan_t::from_seconds( 3 ); // Hard code the duration to 3 seconds
    base_tick_time = dot_duration / p.find_spell( 213183 ) -> effectN( 1 ).base_value();
    tick_zero = hasted_ticks = false;

    tick_action = new dragonfire_brew_tick( p );
  }
};

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

    double action_multiplier() const override
    {
      double am = monk_spell_t::action_multiplier();

      if ( p() -> artifact.hot_blooded.rank() )
        am *= 1 + p() -> artifact.hot_blooded.percent();

      return am;
    }
  };

  dragonfire_brew* dragonfire;
  periodic_t* dot_action;

  breath_of_fire_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "breath_of_fire", &p, p.spec.breath_of_fire ),
    dragonfire( new dragonfire_brew( p ) ),
    dot_action( new periodic_t( p ) )
  {
    parse_options( options_str );

    add_child( dragonfire );
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    // Update the cooldown if Blackout Combo is up
    if ( p() -> buff.blackout_combo -> check() )
    {
      cd += p() -> buff.blackout_combo -> data().effectN( 2 ).time_value(); // saved as -6 seconds
      p() -> buff.blackout_combo -> expire();
    }

    monk_spell_t::update_ready( cd );
  }

 virtual void execute() override
  {
    monk_spell_t::execute();

    if ( p() -> artifact.dragonfire_brew.rank() )
      dragonfire -> execute();
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
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    p() -> buff.fortifying_brew -> trigger();

    if ( p() -> artifact.swift_as_a_coursing_river.rank() )
      p() -> buff.swift_as_a_coursing_river -> trigger();

    if ( p() -> artifact.brew_stache.rank() )
      p() -> buff.brew_stache -> trigger();

    if ( p() -> artifact.fortification.rank() )
      p() -> buff.fortification -> trigger();
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

    d -> cancel();
    cancel();

    return damage_remaining;
  }

  /* Clears part of the stagger dot. Used by Purifying Brew
  * Returns amount purged
  */
  double clear_partial_damage( double percent_amount )
  {
    dot_t* d = get_dot();
    double damage_remaining = 0.0;

    if ( d -> is_ticking() )
    {
      damage_remaining += d -> state -> result_amount; // Assumes base_td == damage, no modifiers or crits
      damage_remaining *= percent_amount;
      d -> state -> result_amount -= damage_remaining;
    }

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
      return calculate_tick_amount( d -> state, d -> current_stack() );
    return 0;
  }
};

// ==========================================================================
// Special Delivery
// ==========================================================================

struct special_delivery_t : public monk_spell_t
{
  special_delivery_t( monk_t& p ) :
    monk_spell_t( "special_delivery", &p, p.passives.special_delivery )
  {
    background = true;
    trigger_gcd = timespan_t::zero();
    aoe = -1;
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    radius = data().effectN( 1 ).radius();
  }

  virtual bool ready() override
  {
    return p() -> talent.special_delivery -> ok();
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( p() -> talent.special_delivery -> effectN( 1 ).base_value() );
  }

  virtual double cost() const override
  {
    return 0;
  }
};

// ==========================================================================
// Ironskin Brew
// ==========================================================================

struct ironskin_brew_t : public monk_spell_t
{
  special_delivery_t* delivery;

  ironskin_brew_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "ironskin_brew", &p, p.spec.ironskin_brew )
  {
    parse_options( options_str );

    harmful = false;
    trigger_gcd = timespan_t::zero();

    p.cooldown.brewmaster_active_mitigation -> duration = p.spec.ironskin_brew -> charge_cooldown();
    p.cooldown.brewmaster_active_mitigation -> charges  = p.spec.ironskin_brew -> charges();
    p.cooldown.brewmaster_active_mitigation -> duration += p.talent.light_brewing -> effectN( 1 ).time_value(); // Saved as -3000
    p.cooldown.brewmaster_active_mitigation -> charges  += p.talent.light_brewing -> effectN( 2 ).base_value();
    p.cooldown.brewmaster_active_mitigation -> hasted   = true;

    cooldown             = p.cooldown.brewmaster_active_mitigation;

    if ( p.talent.special_delivery -> ok() )
      delivery = new special_delivery_t( p );
  }

  void execute() override
  {
    monk_spell_t::execute();

    p() -> buff.ironskin_brew -> trigger();

    if ( p() -> talent.special_delivery -> ok() )
    {
      if ( rng().roll( p() -> talent.special_delivery -> proc_chance() ) )
        delivery -> execute();
    }

    if ( p() -> buff.blackout_combo -> up() )
    {
      p() -> active_actions.stagger_self_damage -> reschedule_execute( timespan_t::from_seconds( p() -> buff.blackout_combo -> data().effectN( 4 ).base_value() ) );
      p() -> buff.blackout_combo -> expire();
    }

    if ( p() -> artifact.swift_as_a_coursing_river.rank() )
      p() -> buff.swift_as_a_coursing_river -> trigger();

    if ( p() -> artifact.brew_stache.rank() )
      p() -> buff.brew_stache -> trigger();
  }
};

// ==========================================================================
// Purifying Brew
// ==========================================================================

struct purifying_brew_t: public monk_spell_t
{
  special_delivery_t* delivery;

  purifying_brew_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "purifying_brew", &p, p.spec.purifying_brew )
  {
    parse_options( options_str );

    harmful = false;
    trigger_gcd = timespan_t::zero();

    p.cooldown.brewmaster_active_mitigation -> duration = p.spec.purifying_brew -> charge_cooldown();
    p.cooldown.brewmaster_active_mitigation -> charges  = p.spec.purifying_brew -> charges();
    p.cooldown.brewmaster_active_mitigation -> duration += p.talent.light_brewing -> effectN( 1 ).time_value(); // Saved as -3000
    p.cooldown.brewmaster_active_mitigation -> charges  += p.talent.light_brewing -> effectN( 2 ).base_value();
    p.cooldown.brewmaster_active_mitigation -> hasted   = true;

    cooldown -> duration = p.spec.purifying_brew -> charge_cooldown();

    if ( p.talent.special_delivery -> ok() )
      delivery = new special_delivery_t( p );
  }

  bool ready() override
  {
    // Irrealistic of in-game, but let's make sure stagger is actually present
    if ( !p() -> active_actions.stagger_self_damage -> stagger_ticking() )
      return false;

    return monk_spell_t::ready();
  }

  void execute() override
  {
    monk_spell_t::execute();

    double stagger_pct = p() -> current_stagger_tick_dmg_percent();

    double purifying_brew_percent = p() -> spec.purifying_brew -> effectN( 1 ).percent();
    if ( p() -> talent.elusive_dance -> ok() )
      purifying_brew_percent += p() -> talent.elusive_dance -> effectN( 2 ).percent();

    //double stagger_dmg = p() -> partial_clear_stagger( purifying_brew_percent );

    // Optional addition: Track and report amount of damage cleared
    if ( stagger_pct > p() -> heavy_stagger_threshold )
    {
      if ( p() -> talent.elusive_dance -> ok() )
      {
        // cancel whatever level the previous Elusive Dance and start the new dance
        if ( p() -> buff.elusive_dance -> up() )
          p() -> buff.elusive_dance -> expire();
        p() -> buff.elusive_dance -> trigger( 3 );
      }
//      p() -> sample_datas.heavy_stagger_total_damage -> add( stagger_dmg );

      // When clearing Moderate Stagger with Purifying Brew, you generate 1 stack of Elusive Brawler.
      if ( p() -> sets.has_set_bonus( MONK_BREWMASTER, T17, B4 ) )
        p() -> buff.elusive_brawler -> trigger( p() -> sets.set( MONK_BREWMASTER, T17, B4) -> effectN( 1 ).base_value() );
    }
    else if ( stagger_pct > p() -> moderate_stagger_threshold )
    {
      if ( p() -> talent.elusive_dance -> ok() )
      {
        // cancel whatever level the previous Elusive Dance and start the new dance
        if ( p() -> buff.elusive_dance -> up() )
          p() -> buff.elusive_dance -> expire();
        p() -> buff.elusive_dance -> trigger( 2 );
      }
//      p() -> sample_datas.moderate_stagger_total_damage -> add( stagger_dmg );

      // When clearing Moderate Stagger with Purifying Brew, you generate 1 stack of Elusive Brawler.
      if ( p() -> sets.has_set_bonus( MONK_BREWMASTER, T17, B4 ) )
        p() -> buff.elusive_brawler -> trigger( p() -> sets.set( MONK_BREWMASTER, T17, B4) -> effectN( 1 ).base_value() );
    }
    else
    {
      if ( p() -> talent.elusive_dance -> ok() )
      {
        // cancel whatever level the previous Elusive Dance and start the new dance
        if ( p() -> buff.elusive_dance -> up() )
          p() -> buff.elusive_dance -> expire();
        p() -> buff.elusive_dance -> trigger();
      }
//      p() -> sample_datas.light_stagger_total_damage -> add( stagger_dmg );
    }

//    p() -> sample_datas.purified_damage -> add( stagger_dmg );

    if ( p() -> talent.healing_elixirs -> ok() )
    {
      if ( p() -> cooldown.healing_elixirs -> up() )
        p() -> active_actions.healing_elixir -> execute();
    }

    if ( p() -> talent.special_delivery -> ok() )
    {
      if ( rng().roll( p() -> talent.special_delivery -> proc_chance() ) )
        delivery -> execute();
    }

    if ( p() -> artifact.swift_as_a_coursing_river.rank() )
      p() -> buff.swift_as_a_coursing_river -> trigger();

    if ( p() -> artifact.brew_stache.rank() )
      p() -> buff.brew_stache -> trigger();

    if ( p() -> buff.blackout_combo -> up() )
    {
      p() -> buff.elusive_brawler -> trigger();
      p() -> buff.blackout_combo -> expire();
    }
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
// Thunder Focus Tea
// ==========================================================================

struct celestial_breath_heal_t : public monk_heal_t
{
  celestial_breath_heal_t( monk_t& p ) :
    monk_heal_t( "celestial_breath_heal", p, p.passives.celestial_breath_heal )
  {
    background = dual = true;
    may_miss = false;
    aoe = p.artifact.celestial_breath.data().effectN( 1 ).base_value();
  }
};

struct celestial_breath_t : public monk_spell_t
{
  celestial_breath_heal_t* heal;

  celestial_breath_t( monk_t& p ) :
    monk_spell_t( "celestial_breath", &p, p.artifact.celestial_breath.data().effectN( 1 ).trigger() )
  {
    background = dual = true;
    may_miss = false;
    radius = p.passives.celestial_breath_heal -> effectN( 1 ).radius();

    heal = new celestial_breath_heal_t( p );
  }

  void tick( dot_t* d ) override
  {
    monk_spell_t::tick( d );

    heal -> execute();
  }
};

struct thunder_focus_tea_t : public monk_spell_t
{
  celestial_breath_t* breath;

  thunder_focus_tea_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "Thunder_focus_tea", &p, p.spec.thunder_focus_tea )
  {
    parse_options( options_str );

    harmful = false;
    trigger_gcd = timespan_t::zero();

    if ( p.artifact.celestial_breath.rank() )
      breath = new celestial_breath_t( p );
  }

  void execute() override
  {
    monk_spell_t::execute();

    p() -> buff.thunder_focus_tea -> trigger( p() -> buff.thunder_focus_tea -> max_stack() );

    if ( p() -> talent.healing_elixirs -> ok() )
    {
      if ( p() -> cooldown.healing_elixirs -> up() )
        p() -> active_actions.healing_elixir -> execute();
    }

    if ( p() -> artifact.celestial_breath.rank() )
      breath -> execute();
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
// Soothing Mist
// ==========================================================================

struct soothing_mist_t: public monk_heal_t
{

  soothing_mist_t( monk_t& p ):
    monk_heal_t( "soothing_mist", p, p.passives.soothing_mist_heal )
  {
    background = dual = true;

    tick_zero = true;
  }

  virtual bool ready() override
  {
    if ( p() -> buff.channeling_soothing_mist -> check() )
      return false;

    return monk_heal_t::ready();
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p() -> artifact.soothing_remedies.rank() )
      am *= 1 + p() -> artifact.soothing_remedies.percent();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_heal_t::impact( s );

    p() -> buff.channeling_soothing_mist -> trigger();
  }

  virtual void tick( dot_t* d ) override
  {
    monk_heal_t::tick( d );

    if ( p() -> sets.has_set_bonus ( MONK_MISTWEAVER, T17, B2 ) )
      p() -> buff.mistweaving -> trigger();
  }

  virtual void last_tick( dot_t* d ) override
  {
    monk_heal_t::last_tick( d );

    p() -> buff.channeling_soothing_mist -> expire();
    p() -> buff.mistweaving -> expire();
  }
};

// ==========================================================================
// The Mists of Sheilun
// ==========================================================================

struct the_mists_of_sheilun_heal_t: public monk_heal_t
{
  the_mists_of_sheilun_heal_t( monk_t& p ):
    monk_heal_t( "the_mists_of_sheilun", p, p.passives.the_mists_of_sheilun_heal )
  {
    background = dual = true;
    aoe = -1;
  }
};

// The Mists of Sheilun Buff ==========================================================
struct the_mists_of_sheilun_buff_t : public buff_t
{
  the_mists_of_sheilun_heal_t* heal;

  the_mists_of_sheilun_buff_t( monk_t* p ) :
    buff_t( buff_creator_t( p, "the_mists_of_sheilun", p -> sheilun_staff_of_the_mists -> driver() -> effectN( 1 ).trigger() )
      .chance( p -> sheilun_staff_of_the_mists -> driver() -> effectN( 1 ).percent() )
      .default_value( p -> sheilun_staff_of_the_mists -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ).percent() ) )
  { 
    heal = new the_mists_of_sheilun_heal_t( *p );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    heal -> execute();
  }
};

// ==========================================================================
// Gust of Mists
// ==========================================================================
// The mastery actually affects the Spell Power Coefficient but I am not sure if that
// would work normally. Using Action Multiplier since it APPEARS to calculate the same.
//
// TODO: Double Check if this works.

struct gust_of_mists_t: public monk_heal_t
{
  gust_of_mists_t( monk_t& p ):
    monk_heal_t( "gust_of_mists", p, p.mastery.gust_of_mists -> effectN( 2 ).trigger() )
  {
    background = dual = true;
    spell_power_mod.direct = 1;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    am *= p() -> cache.mastery_value();

    // Mastery's Effect 3 gives a flat add modifier of 0.1 Spell Power co-efficient
    // TODO: Double check calculation

    return am;
  }
};

// ==========================================================================
// Effuse
// ==========================================================================

struct effuse_t: public monk_heal_t
{
  the_mists_of_sheilun_buff_t* artifact;
  gust_of_mists_t* mastery;

  effuse_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "effuse", p, p.spec.effuse )
  {
    parse_options( options_str );

    if ( p.sheilun_staff_of_the_mists )
      artifact = new the_mists_of_sheilun_buff_t( &p );

    mastery = new gust_of_mists_t( p );

    spell_power_mod.direct = data().effectN( 1 ).ap_coeff();

    may_miss = false;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

      if ( p() -> specialization() == MONK_BREWMASTER || p() -> specialization() == MONK_WINDWALKER )
        am *= 1 + p() -> spec.effuse_2 -> effectN( 1 ).percent();
      else
      {
        if ( p() -> buff.thunder_focus_tea -> up() )
          am *= 1 + p() -> spec.thunder_focus_tea -> effectN( 2 ).percent(); // saved as 200

        if ( p() -> artifact.coalescing_mists.rank() )
          am *= 1 + p() -> artifact.coalescing_mists.percent();
      }

    return am;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p() -> buff.thunder_focus_tea -> up() )
      p() -> buff.thunder_focus_tea -> decrement();

    if ( p() -> sheilun_staff_of_the_mists )
      artifact -> trigger();

    if ( p() -> sets.has_set_bonus( p() -> specialization(), T19OH, B8 ) )
      p() -> buff.tier19_oh_8pc -> trigger();
    
    mastery -> execute();
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( p() -> sets.has_set_bonus( MONK_MISTWEAVER, T17, B4 ) && s -> result == RESULT_CRIT )
      p() -> buff.chi_jis_guidance -> trigger();
  }
};

// ==========================================================================
// Enveloping Mist
// ==========================================================================

struct enveloping_mist_t: public monk_heal_t
{
  the_mists_of_sheilun_buff_t* artifact;
  gust_of_mists_t* mastery;

  enveloping_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "enveloping_mist", p, p.spec.enveloping_mist )
  {
    parse_options( options_str );

    may_miss = false;

    dot_duration = p.spec.enveloping_mist -> duration();
    if ( p.talent.mist_wrap )
      dot_duration += p.talent.mist_wrap -> effectN( 1 ).time_value();

    if ( p.sheilun_staff_of_the_mists )
      artifact = new the_mists_of_sheilun_buff_t( &p );

    mastery = new gust_of_mists_t( p );

    p.internal_id = internal_id;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p() -> artifact.mists_of_wisdom.rank() )
      am *= 1 + p() -> artifact.mists_of_wisdom.percent();

    if ( p() -> artifact.way_of_the_mistweaver.rank() )
      am *= 1 + p() -> artifact.way_of_the_mistweaver.percent();

    return am;
  }

  virtual double cost() const override
  {
    double c = monk_heal_t::cost();

    if ( p() -> buff.lifecycles_enveloping_mist -> check() )
      c *= 1 + p() -> buff.lifecycles_enveloping_mist -> value(); // saved as -20%

    return c;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t et = monk_heal_t::execute_time();

    if ( p() -> buff.thunder_focus_tea -> check() )
      et *= 1 + p() -> spec.thunder_focus_tea -> effectN( 3 ).percent(); // saved as -100

    return et;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p() -> talent.lifecycles )
    {
      if ( p() -> buff.lifecycles_enveloping_mist -> up() )
        p() -> buff.lifecycles_enveloping_mist -> expire();
      p() -> buff.lifecycles_vivify -> trigger();
    }

    if ( p() -> sheilun_staff_of_the_mists )
      artifact -> trigger();

    mastery -> execute();
  }
};

// ==========================================================================
// Renewing Mist
// ==========================================================================
/*
Bouncing only happens when overhealing, so not going to bother with bouncing
*/

struct extend_life_t: public monk_heal_t
{
  extend_life_t( monk_t& p ):
    monk_heal_t( "extend_life", p, p.passives.tier18_2pc_heal )
  {
    background = dual = true;
    dot_duration = timespan_t::zero();
  }

  virtual bool ready() override
  {
    return p() -> specialization() == MONK_MISTWEAVER;
  }

  virtual double cost() const override
  {
    return 0;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    p() -> buff.extend_life -> trigger();
  }

};

// Renewing Mist Dancing Mist Mistweaver Artifact Traits ====================
struct renewing_mist_dancing_mist_t: public monk_heal_t
{
  gust_of_mists_t* mastery;
  extend_life_t* tier18_2pc;

  renewing_mist_dancing_mist_t( monk_t& p ):
    monk_heal_t( "renewing_mist_dancing_mist", p, p.spec.renewing_mist )
  {
    background = dual = true;
    may_crit = may_miss = false;
    dot_duration = p.passives.renewing_mist_heal -> duration();

    if ( p.artifact.extended_healing.rank() )
      dot_duration += p.artifact.extended_healing.time_value();

    if ( p.sets.has_set_bonus( MONK_MISTWEAVER, T18, B2 ) )
      tier18_2pc = new extend_life_t( p ); 

    mastery = new gust_of_mists_t( p );
  }

  virtual double cost() const override
  {
    return 0;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p() -> sets.has_set_bonus( MONK_MISTWEAVER, T18, B4 ) )
      tier18_2pc -> execute();

    mastery -> execute();
  }
};

// Base Renewing Mist Heal ================================================
struct renewing_mist_t: public monk_heal_t
{
  the_mists_of_sheilun_buff_t* artifact;
  renewing_mist_dancing_mist_t* rem;
  gust_of_mists_t* mastery;
  extend_life_t* tier18_2pc;

  renewing_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "renewing_mist", p, p.spec.renewing_mist )
  {
    parse_options( options_str );
    may_crit = may_miss = false;
    dot_duration = p.passives.renewing_mist_heal -> duration();

    if ( p.sheilun_staff_of_the_mists )
      artifact = new the_mists_of_sheilun_buff_t( &p );

    if ( p.artifact.extended_healing.rank() )
      dot_duration += p.artifact.extended_healing.time_value();

    if ( p.artifact.dancing_mists.rank() )
      rem = new renewing_mist_dancing_mist_t( p );

    if ( p.sets.has_set_bonus( MONK_MISTWEAVER, T18, B4 ) )
      tier18_2pc = new extend_life_t( p );

    mastery = new gust_of_mists_t( p );
  }

  virtual double cost() const override
  {
    double c = monk_heal_t::cost();

    if ( p() -> buff.chi_jis_guidance -> check() )
      c *= 1 + p() -> buff.chi_jis_guidance -> value(); // Saved as -50%

    return c;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.thunder_focus_tea -> check() )
      cd *= 1 + p() -> spec.thunder_focus_tea -> effectN( 1 ).percent();

    monk_heal_t::update_ready( cd );
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p() -> sets.has_set_bonus( MONK_MISTWEAVER, T18, B2 ) )
      tier18_2pc -> execute();

    mastery -> execute();

    if ( p() -> buff.thunder_focus_tea -> up() )
      p() -> buff.thunder_focus_tea -> decrement();

    if ( p() -> sheilun_staff_of_the_mists )
      artifact -> trigger();

    if ( p() -> artifact.dancing_mists.rank() )
    {
      if ( rng().roll( p() -> artifact.dancing_mists.data().effectN( 1 ).percent() ) )
          rem -> execute();
    }
  }

  void tick( dot_t* d ) override
  {
    monk_heal_t::tick( d );

    p() -> buff.uplifting_trance -> trigger();
  }

};

// ==========================================================================
// Vivify
// ==========================================================================

struct vivify_t: public monk_heal_t
{
  the_mists_of_sheilun_buff_t* artifact;
  gust_of_mists_t* mastery;

  vivify_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "vivify", p, p.spec.vivify )
  {
    parse_options( options_str );

    // 1 for the primary target, plus the value of the effect
    aoe = 1 + data().effectN( 1 ).base_value();
    spell_power_mod.direct = data().effectN( 2 ).sp_coeff();

    if ( p.sheilun_staff_of_the_mists )
      artifact = new the_mists_of_sheilun_buff_t( &p );

    mastery = new gust_of_mists_t( p );

    may_miss = false;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p() -> buff.uplifting_trance -> up() )
      am *= 1 + p() -> buff.uplifting_trance -> value();

    if ( p() -> artifact.infusion_of_life.rank() )
      am *= 1 + p() -> artifact.infusion_of_life.percent();

    return am;
  }

  virtual double cost() const override
  {
    double c = monk_heal_t::cost();

    // TODO: check the interation between Thunder Focus Tea and Lifecycles
    if ( p() -> buff.thunder_focus_tea -> check() )
      c *= 1 + p() -> spec.thunder_focus_tea -> effectN( 5 ).percent(); // saved as -100

    if ( p() -> buff.lifecycles_vivify -> check() )
      c *= 1 + p() -> buff.lifecycles_vivify -> value(); // saved as -20%

    return c;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p() -> buff.thunder_focus_tea -> up() )
      p() -> buff.thunder_focus_tea -> decrement();

    if ( p() -> talent.lifecycles )
    {
      if ( p() -> buff.lifecycles_vivify -> up() )
        p() -> buff.lifecycles_vivify -> expire();

      p() -> buff.lifecycles_enveloping_mist -> trigger();
    }

    if ( p() -> sheilun_staff_of_the_mists )
      artifact -> trigger();

    if ( p() -> buff.uplifting_trance -> up() )
      p() -> buff.uplifting_trance -> expire();

    mastery -> execute();
  }
};

// ==========================================================================
// Essence Font
// ==========================================================================
// The spell only hits each player 3 times no matter how many players are in group
// The intended model is every 1/6 of a sec, it fires a bolt at a single target 
// that is randomly selected from the pool of [allies that are within range that 
// have not been the target of any of the 5 previous ticks]. If there only 3 
// potential allies, then that set is empty half the time, and a bolt doesn't fire.

struct essence_font_t: public monk_spell_t
{
  struct essence_font_heal_t : public monk_heal_t
  {
    essence_font_heal_t( monk_t& p ) :
      monk_heal_t( "essence_font_heal", p, p.spec.essence_font -> effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = p.spec.essence_font -> effectN( 1 ).base_value();
    }

    double action_multiplier() const override
    {
      double am = monk_heal_t::action_multiplier();

      if ( p() -> buff.refreshing_jade_wind -> up() )
        am *= 1 + p() -> buff.refreshing_jade_wind -> value();

      if ( p() -> artifact.essence_of_the_mists.rank() )
        am *= 1 + p() -> artifact.essence_of_the_mists.percent();

      return am;
    }
  };

  essence_font_heal_t* heal;
  the_mists_of_sheilun_buff_t* artifact;

  essence_font_t( monk_t* p, const std::string& options_str ) :
    monk_spell_t( "essence_font", p, p -> spec.essence_font ),
    heal( new essence_font_heal_t( *p ) )
  {
    parse_options( options_str );

    may_miss = hasted_ticks = false;
    tick_zero = true;

    base_tick_time = data().effectN( 1 ).base_value() * data().effectN(1).period();

    add_child( heal );

    if ( p -> sheilun_staff_of_the_mists )
      artifact = new the_mists_of_sheilun_buff_t( p );
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( p() -> sheilun_staff_of_the_mists )
      artifact -> trigger();
  }

  virtual void last_tick( dot_t* d ) override
  {
    monk_spell_t::last_tick( d );

    if ( p() -> artifact.light_on_your_feet_mw.rank() )
      p() -> buff.light_on_your_feet -> trigger();
  }
};

// ==========================================================================
// Revival
// ==========================================================================

struct blessings_of_yulon_t: public monk_heal_t
{
  blessings_of_yulon_t( monk_t& p ):
    monk_heal_t( "blessings_of_yulon", p, p.passives.blessings_of_yulon )
  {
    background = dual = false;
    may_miss = may_crit = false;
  }
};

struct revival_t: public monk_heal_t
{
  the_mists_of_sheilun_buff_t* artifact;
  blessings_of_yulon_t* yulon;

  revival_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "revival", p, p.spec.revival )
  {
    parse_options( options_str );

    may_miss = false;
    aoe = -1;

    if ( p.sheilun_staff_of_the_mists )
      artifact = new the_mists_of_sheilun_buff_t( &p );

    if ( p.artifact.blessings_of_yulon.rank() )
      yulon = new blessings_of_yulon_t( p );

    if ( sim -> pvp_crit )
      base_multiplier *= 2; // 08/03/2016
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p() -> sheilun_staff_of_the_mists )
      artifact -> trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_heal_t::impact( s );

    if ( p() -> artifact.blessings_of_yulon.rank() )
    {
      double percent = p() -> artifact.blessings_of_yulon.percent();
      yulon -> base_dd_min = s -> result_amount * percent;
      yulon -> base_dd_max = s -> result_amount * percent;
      yulon -> execute();
    }
  }
};

// ==========================================================================
// Sheilun's Gift
// ==========================================================================

struct sheiluns_gift_t: public monk_heal_t
{
  the_mists_of_sheilun_buff_t* artifact;

  sheiluns_gift_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "sheiluns_gift", p, &p.artifact.sheiluns_gift.data() )
  {
    parse_options( options_str );

    may_miss = false;

    artifact = new the_mists_of_sheilun_buff_t( &p );
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    artifact -> trigger();
  }
};

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
  }

  virtual bool ready() override
  {
    if ( p() -> specialization() != MONK_BREWMASTER )
      return false;

    return p() -> buff.gift_of_the_ox -> up();
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    p() -> buff.gift_of_the_ox -> decrement();

    if ( p() -> artifact.gifted_student.rank() )
      p() -> buff.gifted_student -> trigger();
  }
};

// ==========================================================================
// Greater Gift of the Ox
// ==========================================================================

struct greater_gift_of_the_ox_t: public monk_heal_t
{
  greater_gift_of_the_ox_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "greater_gift_of_the_ox", p, p.passives.greater_gift_of_the_ox_heal )
  {
    parse_options( options_str );
    harmful = false;
    background = true;
    target = &p;
    trigger_gcd = timespan_t::zero();
  }

  virtual bool ready() override
  {
    if ( p() -> specialization() != MONK_BREWMASTER )
      return false;

    return p() -> buff.greater_gift_of_the_ox -> up();
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p() -> artifact.gifted_student.rank() )
      am *= 1 + p() -> artifact.gifted_student.percent();

    return am;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    p() -> buff.greater_gift_of_the_ox -> decrement();

    if ( p() -> artifact.gifted_student.rank() )
      p() -> buff.gifted_student -> trigger();
  }
};

// ==========================================================================
// Zen Pulse
// ==========================================================================

struct zen_pulse_heal_t : public monk_heal_t
{
  zen_pulse_heal_t( monk_t& p ):
    monk_heal_t("zen_pulse_heal", p, p.passives.zen_pulse_heal )
  {
    background = true;
    may_crit = may_miss = false;
    target = &( p );
  }
};

struct zen_pulse_dmg_t: public monk_spell_t
{
  zen_pulse_heal_t* heal;
  zen_pulse_dmg_t( monk_t* player ):
    monk_spell_t( "zen_pulse_damage", player, player -> talent.zen_pulse )
  {
    background = true;
    aoe = -1;

    heal = new zen_pulse_heal_t( *player );
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    heal -> base_dd_min = s -> result_amount;
    heal -> base_dd_max = s -> result_amount;
    heal -> execute();
  }
};

struct zen_pulse_t : public monk_spell_t
{
  spell_t* damage;
  zen_pulse_t( monk_t* player ) :
    monk_spell_t( "zen_pulse", player, player -> talent.zen_pulse )
  {
    may_miss = may_dodge = may_parry = false;
    damage = new zen_pulse_dmg_t( player );
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    damage -> execute();
  }
};

// ==========================================================================
// Mistwalk
// ==========================================================================
// Not going to model the teleportation part of the spell

struct mistwalk_t: public monk_heal_t
{
  mistwalk_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "mistwalk", p, p.talent.mistwalk )
  {
    parse_options( options_str );

    may_miss = false;

    attack_power_mod.direct = p.talent.mistwalk -> effectN( 2 ).ap_coeff();
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
    attack_power_mod.direct = 0.867; // Hard code 07/12/16
    target = player;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p() -> buff.storm_earth_and_fire -> up() )
    {
      double sef_mult = p() -> spec.storm_earth_and_fire -> effectN( 3 ).percent();
      if ( p() -> artifact.spiritual_focus.rank() )
        sef_mult += p() -> artifact.spiritual_focus.data().effectN( 3 ).percent();
      am *= 1.0 + sef_mult;
    }
    am *= 1 + p() -> passives.aura_brewmaster_monk -> effectN( 4 ).percent();

    return am;
  }
};

struct chi_wave_dmg_tick_t: public monk_spell_t
{
  chi_wave_dmg_tick_t( monk_t* player, const std::string& name ):
    monk_spell_t( name, player, player -> passives.chi_wave_damage )
  {
    background = direct_tick = true;
    attack_power_mod.direct = 0.867; // Hard code 07/12/16
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_spell_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    if ( p() -> specialization() == MONK_BREWMASTER )
    {
      am *= 1 + p() -> passives.aura_brewmaster_monk -> effectN( 4 ).percent();
    }
    return am;
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
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_CHI_WAVE );

    monk_spell_t::execute();
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
    aoe = -1;
    attack_power_mod.direct = 4.125; // Hard code 06/21/16
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p() -> buff.storm_earth_and_fire -> up() )
    {
      double sef_mult = p() -> spec.storm_earth_and_fire -> effectN( 3 ).percent();
      if ( p() -> artifact.spiritual_focus.rank() )
        sef_mult += p() -> artifact.spiritual_focus.data().effectN( 3 ).percent();
      am *= 1.0 + sef_mult;
    }

    if ( p() -> specialization() == MONK_BREWMASTER )
    {
      am *= 1 + p() -> passives.aura_brewmaster_monk -> effectN( 4 ).percent();
    }

    return am;
  }
};

struct chi_burst_damage_t: public monk_spell_t
{
  chi_burst_damage_t( monk_t& player ):
    monk_spell_t( "chi_burst_damage", &player, player.passives.chi_burst_damage)
  {
    background = true;
    aoe = -1;
    attack_power_mod.direct = 4.125; // Hard code 06/21/16
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_spell_t::composite_persistent_multiplier( action_state );

    if ( p() -> buff.combo_strikes -> up() )
      pm *= 1 + p() -> cache.mastery_value();

    return pm;
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    if ( p() -> specialization() == MONK_BREWMASTER )
    {
      am *= 1 + p() -> passives.aura_brewmaster_monk -> effectN( 4 ).percent();
    }
    return am;
  }
};

struct chi_burst_t: public monk_spell_t
{
  chi_burst_heal_t* heal;
  chi_burst_damage_t* damage;
  chi_burst_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "chi_burst", player, player -> talent.chi_burst ),
    heal( nullptr )
  {
    sef_ability = SEF_CHI_BURST;

    parse_options( options_str );
    heal = new chi_burst_heal_t( *player );
    damage = new chi_burst_damage_t( *player );
    interrupt_auto_attack = false;
  }

  virtual bool ready() override
  {
    if ( p() -> talent.chi_burst -> ok() )
      return monk_spell_t::ready();

    return false;
  }

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_CHI_BURST );

    monk_spell_t::execute();

    heal -> execute();
    damage -> execute();
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
    target = &p;
    trigger_gcd = timespan_t::zero();
    base_pct_heal = p.passives.healing_elixirs -> effectN( 1 ).percent();
    cooldown -> duration = data().effectN( 1 ).period();
  }
};

// ==========================================================================
// Refreshing Jade Wind
// ==========================================================================

struct refreshing_jade_wind_heal_t: public monk_heal_t
{
  refreshing_jade_wind_heal_t( monk_t& player ):
    monk_heal_t( "refreshing_jade_wind_heal", player, player.talent.refreshing_jade_wind-> effectN( 1 ).trigger() )
  {
    background = true;
    aoe = 6;
  }
};

struct refreshing_jade_wind_t: public monk_spell_t
{
  refreshing_jade_wind_heal_t* heal;
  refreshing_jade_wind_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "refreshing_jade_wind", player, player -> talent.refreshing_jade_wind )
  {
    parse_options( options_str );
    heal = new refreshing_jade_wind_heal_t( *player );
  }

  void tick( dot_t* d ) override
  {
    monk_spell_t::tick( d );

    heal -> execute();
  }
};

// ==========================================================================
// Celestial Fortune
// ==========================================================================
// This is a Brewmaster-specific critical strike effect

struct celestial_fortune_t : public monk_heal_t
{
  proc_t* proc_tracker;

  celestial_fortune_t( monk_t& p )
    : monk_heal_t( "celestial_fortune", p, p.passives.celestial_fortune ),
    proc_tracker( p.get_proc( name_str ) )
  {
    background = true;
    proc = true;
    target = player;
    may_crit = false;
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything  
  virtual void init() override
  {
    monk_heal_t::init();
    // disable the snapshot_flags for all multipliers, but specifically allow 
    // action_multiplier() to be called so we can override.
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_MUL_DA;
  }

  virtual double action_multiplier() const override
  {
    double am = p() -> spec.celestial_fortune -> effectN( 1 ).percent();
        
    return am;
  }

  virtual bool ready() override
  {
    return p() -> specialization() == MONK_BREWMASTER;
  }

  virtual void execute() override
  {
    proc_tracker -> occur();

    monk_heal_t::execute();
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

// ==========================================================================
// Life Cocoon
// ==========================================================================
// TODO: Double check if the Enveloping Mists and Renewing Mists from Mists
// of life proc the mastery or not.

// Enveloping Mist Mists of Life Mistweaver Artifact Trait =======================
struct enveloping_mist_mists_of_life_t: public monk_heal_t
{
  enveloping_mist_mists_of_life_t( monk_t& p ):
    monk_heal_t( "enveloping_mist_mists_of_life", p, p.spec.enveloping_mist )
  {
    background = dual = true;
    may_miss = false;

    dot_duration = p.spec.enveloping_mist -> duration();
    if ( p.talent.mist_wrap )
      dot_duration += timespan_t::from_seconds( p.talent.mist_wrap -> effectN( 1 ).base_value() );
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p() -> artifact.mists_of_wisdom.rank() )
      am *= 1 + p() -> artifact.mists_of_wisdom.percent();

    if ( p() -> artifact.way_of_the_mistweaver.rank() )
      am *= 1 + p() -> artifact.way_of_the_mistweaver.percent();

    return am;
  }

  virtual double cost() const override
  {
    return 0;
  }
};

// Renewing Mist Mists of Life Mistweaver Artifact Traits ===================
struct renewing_mist_mists_of_life_t: public monk_heal_t
{
  renewing_mist_mists_of_life_t( monk_t& p ):
    monk_heal_t( "renewing_mist_dancing_mist", p, p.spec.renewing_mist )
  {
    background = dual = true;
    may_crit = may_miss = false;
    dot_duration = p.passives.renewing_mist_heal -> duration();

    if ( p.artifact.extended_healing.rank() )
      dot_duration += p.artifact.extended_healing.time_value();

    if ( p.artifact.extended_healing.rank() )
      dot_duration += p.artifact.extended_healing.time_value();
  }

  virtual double cost() const override
  {
    return 0;
  }
};

struct life_cocoon_t: public monk_absorb_t
{
  renewing_mist_mists_of_life_t* rem;
  enveloping_mist_mists_of_life_t* em;

  life_cocoon_t( monk_t& p, const std::string& options_str ):
    monk_absorb_t( "life_cocoon", p, p.spec.life_cocoon )
  {
    parse_options( options_str );
    harmful = may_crit = false;
    cooldown -> duration = data().charge_cooldown();
    spell_power_mod.direct = 31.164; // Hard Code 2015-Dec-29

    if ( p.artifact.mists_of_life.rank() )
    {
      rem = new renewing_mist_mists_of_life_t( p );
      em = new enveloping_mist_mists_of_life_t( p );
    }
  }

  double action_multiplier() const override
  {
    double am = monk_absorb_t::action_multiplier();

    if ( p() -> artifact.protection_of_shaohao.rank() )
      am *= 1 + p() -> artifact.protection_of_shaohao.percent();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    p() -> buff.life_cocoon -> trigger( 1, s -> result_amount );
    stats -> add_result( 0.0, s -> result_amount, ABSORB, s -> result, s -> block_result, s -> target );

    if ( p() -> artifact.mists_of_life.rank() )
    {
      rem -> execute();
      em -> execute();
    }
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
  chi_orbit_event_t( monk_t& player, timespan_t tick_time )
    : player_event_t( player,
                      clamp( tick_time, timespan_t::zero(),
                             player.talent.chi_orbit->effectN( 1 ).period() ) )
  {
  }

  virtual const char* name() const override
  { return  "chi_orbit_generate"; }

  virtual void execute() override
  {
    monk_t* p = debug_cast<monk_t*>( player() );

    p -> buff.chi_orbit -> trigger();

    make_event<chi_orbit_event_t>( sim(), *p, p -> talent.chi_orbit -> effectN( 1 ).period() );
  }
};

struct chi_orbit_trigger_event_t : public player_event_t
{
  chi_orbit_trigger_event_t( monk_t& player, timespan_t tick_time )
    : player_event_t( player, clamp( tick_time, timespan_t::zero(),
                                     timespan_t::from_seconds( 1 ) ) )
  {
  }

  virtual const char* name() const override
  { return  "chi_orbit_trigger"; }

  virtual void execute() override
  {
    monk_t* p = debug_cast<monk_t*>( player() );

    if ( p -> buff.chi_orbit -> up() )
      p -> active_actions.chi_orbit -> execute();

    make_event<chi_orbit_trigger_event_t>( sim(), *p, timespan_t::from_seconds( 1 ) );
  }
};

struct power_strikes_event_t: public player_event_t
{
  power_strikes_event_t( monk_t& player, timespan_t tick_time )
    : player_event_t(
          player, clamp( tick_time, timespan_t::zero(),
                         player.talent.power_strikes->effectN( 1 ).period() ) )
  {
  }

  virtual const char* name() const override
  { return  "power_strikes"; }

  virtual void execute() override
  {
    monk_t* p = debug_cast<monk_t*>( player() );

    p -> buff.power_strikes -> trigger();

    make_event<power_strikes_event_t>( sim(), *p, p -> talent.power_strikes -> effectN( 1 ).period() );
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
      return *( monk.get_target_data( t ) );
    }

    protected:
    monk_t& monk;    
  };

// Fortifying Brew Buff ==========================================================
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

// Hidden Master's Forbidden Touch Legendary
struct hidden_masters_forbidden_touch_t : public monk_buff_t < buff_t >
{
  hidden_masters_forbidden_touch_t( monk_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s ) )
  {
  }
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );
    cooldown_t* touch_of_death = source -> get_cooldown( "touch_of_death" );
    if ( touch_of_death -> up() )
      touch_of_death -> start();
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
  if ( p -> specialization() == MONK_WINDWALKER )
  {
    debuff.mark_of_the_crane = buff_creator_t( *this, "mark_of_the_crane" )
      .spell( p -> passives.mark_of_the_crane )
      .default_value( p -> passives.mark_of_the_crane -> effectN( 1 ).percent() );

    debuff.gale_burst = buff_creator_t( *this, "gale_burst" )
      .spell( p -> passives.gale_burst )
      .default_value( 0 )
      .quiet( true );
  }

  debuff.dizzing_kicks = buff_creator_t( *this, "dizzying_kicks" )
    .spell( p -> passives.dizzying_kicks )
    .default_value( p-> passives.dizzying_kicks -> effectN( 1 ).percent() );

  if ( p -> specialization() == MONK_BREWMASTER )
  {
    debuff.keg_smash = buff_creator_t( *this, "keg_smash" )
      .spell( p -> spec.keg_smash )
      .default_value( p -> spec.keg_smash -> effectN( 2 ).percent() );
  }

  debuff.storm_earth_and_fire = buff_creator_t( *this, "storm_earth_and_fire_target" )
    .cd( timespan_t::zero() );

  dots.enveloping_mist = target -> get_dot( "enveloping_mist", p );
  dots.renewing_mist = target -> get_dot( "renewing_mist", p );
  dots.soothing_mist = target -> get_dot( "soothing_mist", p );
  dots.touch_of_death = target -> get_dot( "touch_of_death", p );
}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  using namespace actions;
  // General
  if ( name == "auto_attack" ) return new               auto_attack_t( this, options_str );
  if ( name == "crackling_jade_lightning" ) return new  crackling_jade_lightning_t( *this, options_str );
  if ( name == "tiger_palm" ) return new                tiger_palm_t( this, options_str );
  if ( name == "blackout_kick" ) return new             blackout_kick_t( this, options_str );
  if ( name == "spinning_crane_kick" ) return new       spinning_crane_kick_t( this, options_str );
  if ( name == "rising_sun_kick" ) return new           rising_sun_kick_t( this, options_str );
  if ( name == "spear_hand_strike" ) return new         spear_hand_strike_t( this, options_str );
  if ( name == "vivify" ) return new                    vivify_t( *this, options_str );
  // Brewmaster
  if ( name == "blackout_strike" ) return new           blackout_strike_t( this, options_str );
  if ( name == "breath_of_fire" ) return new            breath_of_fire_t( *this, options_str );
  if ( name == "exploding_keg" ) return new             exploding_keg_t( *this, options_str );
  if ( name == "fortifying_brew" ) return new           fortifying_brew_t( *this, options_str );
  if ( name == "gift_of_the_ox" ) return new            gift_of_the_ox_t( *this, options_str );
  if ( name == "greater_gift_of_the_ox" ) return new    greater_gift_of_the_ox_t( *this, options_str );
  if ( name == "ironskin_brew" ) return new             ironskin_brew_t( *this, options_str );
  if ( name == "keg_smash" ) return new                 keg_smash_t( *this, options_str );
  if ( name == "purifying_brew" ) return new            purifying_brew_t( *this, options_str );
  if ( name == "provoke" ) return new                   provoke_t( this, options_str );
  // Mistweaver
  if ( name == "effuse" ) return new                    effuse_t( *this, options_str );
  if ( name == "enveloping_mist" ) return new           enveloping_mist_t( *this, options_str );
  if ( name == "essence_font" ) return new              essence_font_t( this, options_str );
  if ( name == "life_cocoon" ) return new               life_cocoon_t( *this, options_str );
  if ( name == "mana_tea" ) return new                  mana_tea_t( *this, options_str );
  if ( name == "renewing_mist" ) return new             renewing_mist_t( *this, options_str );
  if ( name == "revival" ) return new                   revival_t( *this, options_str );
  if ( name == "thunder_focus_tea" ) return new         thunder_focus_tea_t( *this, options_str );
  // Windwalker
  if ( name == "fists_of_fury" ) return new             fists_of_fury_t( this, options_str );
  if ( name == "touch_of_karma" ) return new            touch_of_karma_t( this, options_str );
  if ( name == "touch_of_death" ) return new            touch_of_death_t( this, options_str );
  if ( name == "storm_earth_and_fire" ) return new      storm_earth_and_fire_t( this, options_str );
  // Talents
  if ( name == "chi_burst" ) return new                 chi_burst_t( this, options_str );
//  if ( name == "chi_orbit" ) return new                 chi_orbit_t( this, options_str );
  if ( name == "chi_torpedo" ) return new               chi_torpedo_t( this, options_str );
  if ( name == "chi_wave" ) return new                  chi_wave_t( this, options_str );
  if ( name == "black_ox_brew" ) return new             black_ox_brew_t( this, options_str );
  if ( name == "dampen_harm" ) return new               dampen_harm_t( *this, options_str );
  if ( name == "diffuse_magic" ) return new             diffuse_magic_t( *this, options_str );
  if ( name == "energizing_elixir" ) return new         energizing_elixir_t( this, options_str );
  if ( name == "invoke_xuen" ) return new               xuen_spell_t( this, options_str );
  if ( name == "mistwalk" ) return new                  mistwalk_t( *this, options_str );
  if ( name == "refreshing_jade_wind" ) return new      refreshing_jade_wind_t( this, options_str );
  if ( name == "rushing_jade_wind" ) return new         rushing_jade_wind_t( this, options_str );
  if ( name == "whirling_dragon_punch" ) return new     whirling_dragon_punch_t( this, options_str );
  if ( name == "serenity" ) return new                  serenity_t( this, options_str );
  // Artifacts
  if ( name == "sheiluns_gift" ) return new             sheiluns_gift_t( *this, options_str );
  if ( name == "strike_of_the_windlord" ) return new    strike_of_the_windlord_t( this, options_str );
  return base_t::create_action( name, options_str );
}

void monk_t::trigger_celestial_fortune( action_state_t* s )
{
  if ( ! spec.celestial_fortune -> ok() || s -> action == active_celestial_fortune_proc || s -> result_raw == 0.0 )
    return;

  // flush out percent heals
  if ( s -> action -> type == ACTION_HEAL )
  {
    heal_t* heal_cast = debug_cast<heal_t*>( s -> action );
    if ( ( s -> result_type == HEAL_DIRECT && heal_cast -> base_pct_heal > 0 ) || ( s -> result_type == HEAL_OVER_TIME && heal_cast -> tick_pct_heal > 0 ) )
      return;
  }

  // Attempt to proc the heal
  if ( active_celestial_fortune_proc && rng().roll( composite_melee_crit_chance() ) )
  {
    active_celestial_fortune_proc -> base_dd_max = active_celestial_fortune_proc -> base_dd_min = s -> result_amount;
    active_celestial_fortune_proc -> schedule_execute();
  }
}

void monk_t::trigger_mark_of_the_crane( action_state_t* s )
{
  get_target_data( s -> target ) -> debuff.mark_of_the_crane -> trigger();
}

player_t* monk_t::next_mark_of_the_crane_target( action_state_t* state )
{
  std::vector<player_t*> targets = state -> action -> target_list();
  if ( targets.empty())
  {
    return nullptr;
  }
  if ( targets.size() > 1 )
  {
    // Have the SEF converge onto the the cleave target if there are only 2 targets
    if (targets.size() == 2)
      return targets[1];
    // Don't move the SEF if there is only 3 targets
    if (targets.size() == 3)
      return state -> target;
    
    // First of all find targets that do not have the cyclone strike debuff applied and send the SEF to those targets
    for ( player_t* target : targets )
    {
      if (  !get_target_data( target ) -> debuff.mark_of_the_crane -> up() &&
        !get_target_data( target ) -> debuff.storm_earth_and_fire -> up() )
      {
        // remove the current target as having an SEF on it
        get_target_data( state -> target ) -> debuff.storm_earth_and_fire -> expire();
        // make the new target show that a SEF is on the target
        get_target_data( target ) -> debuff.storm_earth_and_fire -> trigger();
        return target;
      }
    }

    // If all targets have the debuff, find the lowest duration of cyclone strike debuff as well as not have a SEF
    // debuff (indicating that an SEF is not already on the target and send the SEF to that new target.
    player_t* lowest_duration = targets[0];

    // They should never attack the player target
    for ( player_t* target : targets )
    {
      if ( !get_target_data( target ) -> debuff.storm_earth_and_fire -> up() )
      {
        if ( get_target_data( target ) -> debuff.mark_of_the_crane -> remains() <
          get_target_data( lowest_duration ) -> debuff.mark_of_the_crane -> remains() )
          lowest_duration = target;
      }
    }
    // remove the current target as having an SEF on it
    get_target_data( state -> target ) -> debuff.storm_earth_and_fire -> expire();
    // make the new target show that a SEF is on the target
    get_target_data( lowest_duration ) -> debuff.storm_earth_and_fire -> trigger();
    return lowest_duration;
  }
  // otherwise, target the same as the player
  return targets.front();
}

int monk_t::mark_of_the_crane_counter()
{
  auto targets = sim -> target_non_sleeping_list.data();
  int mark_of_the_crane_counter = 0;

  if ( specialization() == MONK_WINDWALKER )
  {
    for ( player_t* target : targets )
    {
      if ( get_target_data( target ) -> debuff.mark_of_the_crane -> up() )
        mark_of_the_crane_counter++;
    }
  }
  return mark_of_the_crane_counter;
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
    sim -> target_non_sleeping_list.register_callback( actions::sef_despawn_cb_t( this ) );

    pet.sef[ SEF_FIRE ] = new pets::storm_earth_and_fire_pet_t( "fire_spirit", sim, this, true );
    // The player BECOMES the Storm Spirit
    // SEF EARTH was changed from 2-handed user to dual welding in Legion
    pet.sef[ SEF_EARTH ] = new pets::storm_earth_and_fire_pet_t( "earth_spirit", sim, this, true );
  }
}

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  base_t::init_spells();
  // Talents spells =====================================
  // Tier 15 Talents
  talent.chi_burst                   = find_talent_spell( "Chi Burst" );
  talent.eye_of_the_tiger            = find_talent_spell( "Eye of the Tiger" ); // Brewmaster & Windwalker
  talent.chi_wave                    = find_talent_spell( "Chi Wave" );
  // Mistweaver
  talent.zen_pulse                   = find_talent_spell( "Zen Pulse" );
  talent.mistwalk                    = find_talent_spell( "Mistwalk" );

  // Tier 30 Talents
  talent.chi_torpedo                 = find_talent_spell( "Chi Torpedo" );
  talent.tigers_lust                 = find_talent_spell( "Tiger's Lust" );
  talent.celerity                    = find_talent_spell( "Celerity" );

  // Tier 45 Talents
  // Brewmaster
  talent.light_brewing               = find_talent_spell( "Light Brewing" );
  talent.black_ox_brew               = find_talent_spell( "Black Ox Brew" );
  talent.gift_of_the_mists           = find_talent_spell( "Gift of the Mists" );
  // Windwalker
  talent.energizing_elixir           = find_talent_spell( "Energizing Elixir" );
  talent.ascension                   = find_talent_spell( "Ascension" );
  talent.power_strikes               = find_talent_spell( "Power Strikes" );
  // Mistweaver
  talent.lifecycles                  = find_talent_spell( "Lifecycles" );
  talent.spirit_of_the_crane         = find_talent_spell( "Spirit of the Crane" );
  talent.mist_wrap                   = find_talent_spell( "Mist Wrap" );

  // Tier 60 Talents
  talent.ring_of_peace               = find_talent_spell( "Ring of Peace" );
  talent.summon_black_ox_statue      = find_talent_spell( "Summon Black Ox Statue" ); // Brewmaster
  talent.dizzying_kicks              = find_talent_spell( "Dizzying Kicks" ); // Windwalker
  talent.song_of_chi_ji              = find_talent_spell( "Song of Chi-Ji" ); // Mistweaver
  talent.leg_sweep                   = find_talent_spell( "Leg Sweep" );

  // Tier 75 Talents
  talent.healing_elixirs             = find_talent_spell( "Healing Elixirs" );
  talent.diffuse_magic               = find_talent_spell( "Diffuse Magic" );
  talent.dampen_harm                 = find_talent_spell( "Dampen Harm" );

  // Tier 90 Talents
  talent.rushing_jade_wind           = find_talent_spell( "Rushing Jade Wind" ); // Brewmaster & Windwalker
  // Brewmaster
  talent.invoke_niuzao               = find_talent_spell( "Invoke Niuzao, the Black Ox", "invoke_niuzao" );
  talent.special_delivery            = find_talent_spell( "Special Delivery" );
  // Windwalker
  talent.invoke_xuen                 = find_talent_spell( "Invoke Xuen, the White Tiger", "invoke_xuen" );
  talent.hit_combo                   = find_talent_spell( "Hit Combo" );
  // Mistweaver
  talent.refreshing_jade_wind        = find_talent_spell( "Refreshing Jade Wind" );
  talent.invoke_chi_ji               = find_talent_spell( "Invoke Chi-Ji, the Red Crane", "invoke_chi_ji" );
  talent.summon_jade_serpent_statue  = find_talent_spell( "Summon Jade Serpent Statue" );

  // Tier 100 Talents
  // Brewmaster
  talent.elusive_dance               = find_talent_spell( "Elusive Dance" );
  talent.blackout_combo              = find_talent_spell( "Blackout Combo" );
  talent.high_tolerance              = find_talent_spell( "High Tolerance" );
  // Windwalker
  talent.chi_orbit                   = find_talent_spell( "Chi Orbit" );
  talent.whirling_dragon_punch       = find_talent_spell( "Whirling Dragon Punch" );
  talent.serenity                    = find_talent_spell( "Serenity" );
  // Mistweaver
  talent.mana_tea                    = find_talent_spell( "Mana Tea" );
  talent.focused_thunder             = find_talent_spell( "Focused Thunder" );
  talent.rising_thunder              = find_talent_spell ("Rising Thunder");
  
  // Artifact spells ========================================
  // Brewmater
  artifact.hot_blooded                = find_artifact_spell( "Hot Blooded" );
  artifact.brew_stache                = find_artifact_spell( "Brew-Stache" );
  artifact.dark_side_of_the_moon      = find_artifact_spell( "Dark Side of the Moon" );
  artifact.dragonfire_brew            = find_artifact_spell( "Dragonfire Brew" );
  artifact.face_palm                  = find_artifact_spell( "Face Palm" );
  artifact.exploding_keg              = find_artifact_spell( "Exploding Keg" );
  artifact.fortification              = find_artifact_spell( "Fortification" );
  artifact.full_keg                   = find_artifact_spell( "Full Keg" );
  artifact.gifted_student             = find_artifact_spell( "Gifted Student" );
  artifact.healthy_appetite           = find_artifact_spell( "Healthy Appetite" );
  artifact.obsidian_fists             = find_artifact_spell( "Obsidian Fists" );
  artifact.obstinate_determination    = find_artifact_spell( "Obstinate Determination" );
  artifact.overflow                   = find_artifact_spell( "Overflow" );
  artifact.potent_kick                = find_artifact_spell( "Potent Kick" );
  artifact.smashed                    = find_artifact_spell( "Smashed" );
  artifact.staggering_around          = find_artifact_spell( "Staggering Around" );
  artifact.swift_as_a_coursing_river  = find_artifact_spell( "Swift as a Coursing River" );

  // Mistweaver
  artifact.blessings_of_yulon         = find_artifact_spell( "Blessings of Yu'lon" );
  artifact.celestial_breath           = find_artifact_spell( "Celestial Breath" );
  artifact.coalescing_mists           = find_artifact_spell( "Coalescing Mists" );
  artifact.dancing_mists              = find_artifact_spell( "Dancing Mists" );
  artifact.essence_of_the_mists       = find_artifact_spell( "Essence of the Mists" );
  artifact.extended_healing           = find_artifact_spell( "Extended Healing" );
  artifact.infusion_of_life           = find_artifact_spell( "Infusion of Life" );
  artifact.light_on_your_feet_mw      = find_artifact_spell( "Light on Your Feet" );
  artifact.mists_of_life              = find_artifact_spell( "Mists of Life" );
  artifact.mists_of_wisdom            = find_artifact_spell( "Mists of Wisdom" );
  artifact.protection_of_shaohao      = find_artifact_spell( "Protection of Shaohao" );
  artifact.sheiluns_gift              = find_artifact_spell( "Sheilun's Gift" );
  artifact.shroud_of_mist             = find_artifact_spell( "Shroud of Mist" );
  artifact.soothing_remedies          = find_artifact_spell( "Soothing Remedies" );
  artifact.spirit_tether              = find_artifact_spell( "Spirit Tether" );
  artifact.the_mists_of_sheilun       = find_artifact_spell( "The Mists of Sheilun" );
  artifact.way_of_the_mistweaver      = find_artifact_spell( "Way of the Mistweaver" );

  // Windwalker
  artifact.crosswinds                 = find_artifact_spell( "Crosswinds" );
  artifact.dark_skies                 = find_artifact_spell( "Dark Skies" );
  artifact.death_art                  = find_artifact_spell( "Death Art" );
  artifact.fists_of_the_wind          = find_artifact_spell( "Fists of the Wind" );
  artifact.gale_burst                 = find_artifact_spell( "Gale Burst" );
  artifact.good_karma                 = find_artifact_spell( "Good Karma" );
  artifact.healing_winds              = find_artifact_spell( "Healing Winds" );
  artifact.inner_peace                = find_artifact_spell( "Inner Peace" );
  artifact.light_on_your_feet_ww      = find_artifact_spell( "Light on Your Feet" );
  artifact.power_of_a_thousand_cranes = find_artifact_spell( "Power of a Thousand Cranes" );
  artifact.rising_winds               = find_artifact_spell( "Rising Winds" );
  artifact.spiritual_focus            = find_artifact_spell( "Spiritual Focus" );
  artifact.strike_of_the_windlord     = find_artifact_spell( "Strike of the Windlord" );
  artifact.strength_of_xuen           = find_artifact_spell( "Strength of Xuen" );
  artifact.tiger_claws                = find_artifact_spell( "Tiger Claws" );
  artifact.tornado_kicks              = find_artifact_spell( "Tornado Kicks" );
  artifact.transfer_the_power         = find_artifact_spell( "Transfer the Power" );

  // Specialization spells ====================================
  // Multi-Specialization & Class Spells
  spec.blackout_kick                 = find_class_spell( "Blackout Kick" );
  spec.crackling_jade_lightning      = find_class_spell( "Crackling Jade Lightning" );
  spec.critical_strikes              = find_specialization_spell( "Critical Strikes" );
  spec.effuse                        = find_specialization_spell( "Effuse" );
  spec.effuse_2                      = find_specialization_spell( 231602 );
  spec.leather_specialization        = find_specialization_spell( "Leather Specialization" );
  spec.provoke                       = find_class_spell( "Provoke" );
  spec.resuscitate                   = find_class_spell( "Resuscitate" );
  spec.rising_sun_kick               = find_specialization_spell( "Rising Sun Kick" );
  spec.roll                          = find_class_spell( "Roll" );
  spec.spear_hand_strike             = find_specialization_spell( "Spear Hand Strike" );
  spec.spinning_crane_kick           = find_specialization_spell( "Spinning Crane Kick" );
  spec.tiger_palm                    = find_class_spell( "Tiger Palm" );

  // Brewmaster Specialization
  spec.blackout_strike               = find_specialization_spell( "Blackout Strike" );
  spec.bladed_armor                  = find_specialization_spell( "Bladed Armor" );
  spec.breath_of_fire                = find_specialization_spell( "Breath of Fire" );
  spec.celestial_fortune             = find_specialization_spell( "Celestial Fortune" );
  spec.expel_harm                    = find_specialization_spell( "Expel Harm" );
  spec.fortifying_brew               = find_specialization_spell( "Fortifying Brew" );
  spec.gift_of_the_ox                = find_specialization_spell( "Gift of the Ox" );
  spec.ironskin_brew                 = find_specialization_spell( "Ironskin Brew" );
  spec.keg_smash                     = find_specialization_spell( "Keg Smash" );
  spec.purifying_brew                = find_specialization_spell( "Purifying Brew" );
  spec.stagger                       = find_specialization_spell( "Stagger" );
  spec.zen_meditation                = find_specialization_spell( "Zen Meditation" );
  spec.light_stagger                 = find_spell( 124275 );
  spec.moderate_stagger              = find_spell( 124274 );
  spec.heavy_stagger                 = find_spell( 124273 );

  // Mistweaver Specialization
  spec.detox                         = find_specialization_spell( "Detox" );
  spec.enveloping_mist               = find_specialization_spell( "Enveloping Mist" );
  spec.envoloping_mist_2             = find_specialization_spell( 231605 );
  spec.essence_font                  = find_specialization_spell( "Essence Font" );
  spec.life_cocoon                   = find_specialization_spell( "Life Cocoon" );
  spec.reawaken                      = find_specialization_spell( "Reawaken" );
  spec.renewing_mist                 = find_specialization_spell( "Renewing Mist" );
  spec.renewing_mist_2               = find_specialization_spell( 231606 );
  spec.revival                       = find_specialization_spell( "Revival" );
  spec.soothing_mist                 = find_specialization_spell( "Soothing Mist" );
  spec.teachings_of_the_monastery    = find_specialization_spell( "Teachings of the Monastery" );
  spec.thunder_focus_tea             = find_specialization_spell( "Thunder Focus Tea" );
  spec.thunger_focus_tea_2           = find_specialization_spell( 231876 );
  spec.vivify                        = find_specialization_spell( "Vivify" );

  // Windwalker Specialization
  spec.afterlife                     = find_specialization_spell( "Afterlife" );
  spec.combat_conditioning           = find_specialization_spell( "Combat Conditioning" );
  spec.combo_breaker                 = find_specialization_spell( "Combo Breaker" );
  spec.cyclone_strikes               = find_specialization_spell( "Cyclone Strikes" );
  spec.disable                       = find_specialization_spell( "Disable" );
  spec.fists_of_fury                 = find_specialization_spell( "Fists of Fury" );
  spec.flying_serpent_kick           = find_specialization_spell( "Flying Serpent Kick" );
  spec.stance_of_the_fierce_tiger    = find_specialization_spell( "Stance of the Fierce Tiger" );
  spec.storm_earth_and_fire          = find_specialization_spell( "Storm, Earth, and Fire" );
  spec.storm_earth_and_fire_2        = find_specialization_spell( 231627 );
  spec.touch_of_karma                = find_specialization_spell( "Touch of Karma" );
  spec.touch_of_death                = find_specialization_spell( "Touch of Death" );
  spec.windwalker_monk               = find_specialization_spell( "Windwalker Monk" );
  spec.windwalking                   = find_specialization_spell( "Windwalking" );

  // Passives =========================================
  // General
  passives.aura_monk                        = find_spell( 137022 );
  passives.chi_burst_damage                 = find_spell( 148135 );
  passives.chi_burst_heal                   = find_spell( 130654 );
  passives.chi_torpedo                      = find_spell( 119085 );
  passives.chi_wave_damage                  = find_spell( 132467 );
  passives.chi_wave_heal                    = find_spell( 132463 );
  passives.healing_elixirs                  = find_spell( 122281 ); // talent.healing_elixirs -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() 

  // Brewmaster
  passives.aura_brewmaster_monk             = find_spell( 137023 );
  passives.breath_of_fire_dot               = find_spell( 123725 );
  passives.celestial_fortune                = find_spell( 216521 );
  passives.dragonfire_brew_damage           = find_spell( 227681 ); 
  passives.elusive_brawler                  = find_spell( 195630 );
  passives.elusive_dance                    = find_spell( 196739 );
  passives.gift_of_the_ox_heal              = find_spell( 124507 );
  passives.gift_of_the_ox_summon            = find_spell( 124503 );
  passives.greater_gift_of_the_ox_heal      = find_spell( 214416 );
  passives.ironskin_brew                    = find_spell( 215479 );
  passives.keg_smash_buff                   = find_spell( 196720 );
  passives.face_palm                        = find_spell( 227679 );
  passives.special_delivery                 = find_spell( 196733 );
  passives.stagger_self_damage              = find_spell( 124255 );
  passives.tier17_2pc_tank                  = find_spell( 165356 );

  // Mistweaver
  passives.aura_mistweaver_monk             = find_spell( 137024 );
  passives.totm_bok_proc                    = find_spell( 228649 );
  passives.blessings_of_yulon               = find_spell( 199671 );
  passives.celestial_breath_heal            = find_spell( 199565 ); // artifact.celestial_breath.data().effectN( 1 ).trigger() -> effectN( 1 ).trigger()
  passives.lifecycles_enveloping_mist       = find_spell( 197919 );
  passives.lifecycles_vivify                = find_spell( 197916 );
  passives.renewing_mist_heal               = find_spell( 119611 );
  passives.shaohaos_mists_of_wisdom         = find_spell( 199877 ); // artifact.shaohaos_mists_of_wisdom.data().effectN( 1 ).trigger() -> effectN( 2 ).trigger()
  passives.soothing_mist_heal               = find_spell( 115175 );
  passives.soothing_mist_statue             = find_spell( 198533 );
  passives.spirit_of_the_crane              = find_spell( 210803 );
  passives.teachings_of_the_monastery_buff  = find_spell( 202090 );
  passives.the_mists_of_sheilun_heal        = find_spell( 199894 );
  passives.uplifting_trance                 = find_spell( 197206 );
  passives.zen_pulse_heal                   = find_spell( 198487 );
  passives.tier17_2pc_heal                  = find_spell( 167732 );
  passives.tier17_4pc_heal                  = find_spell( 167717 );
  passives.tier18_2pc_heal                  = find_spell( 185158 ); // Extend Life

  // Windwalker
  passives.aura_windwalker_monk             = find_spell( 137025 );
  passives.chi_orbit                        = find_spell( 196748 );
  passives.bok_proc                         = find_spell( 116768 );
  passives.crackling_tiger_lightning        = find_spell( 123996 );
  passives.crackling_tiger_lightning_driver = find_spell( 123999 );
  passives.crosswinds_dmg                   = find_spell( 196061 );
  passives.crosswinds_trigger               = find_spell( 195651 );
  passives.dizzying_kicks                   = find_spell( 196723 );
  passives.gale_burst                       = find_spell( 195403 );
  passives.hit_combo                        = find_spell( 196741 );
  passives.mark_of_the_crane                = find_spell( 228287 );
  passives.whirling_dragon_punch            = find_spell( 158221 );
  passives.touch_of_karma_tick              = find_spell( 124280 );
  passives.tier17_4pc_melee                 = find_spell( 166603 );
  passives.tier18_2pc_melee                 = find_spell( 216172 );
  passives.tier19_4pc_melee                 = find_spell( 211432 );

  // Legendaries
  passives.hidden_masters_forbidden_touch   = find_spell( 213114 );
  passives.the_emperor_capacitor            = find_spell( 235054 );

  // Mastery spells =========================================
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

  if ( talent.chi_orbit -> ok() )
    active_actions.chi_orbit = new actions::chi_orbit_t( this );

  if (specialization() == MONK_BREWMASTER)
    active_actions.stagger_self_damage = new actions::stagger_self_damage_t( this );
}

// monk_t::init_base ========================================================

void monk_t::init_base_stats()
{
  base_t::init_base_stats();

  base_gcd = timespan_t::from_seconds( 1.5 );

  switch( specialization() )
  {
    case MONK_BREWMASTER:
    {
      base.distance = 3;
      base_gcd += spec.stagger -> effectN( 11 ).time_value(); // Saved as -500 milliseconds
      base.attack_power_per_agility = 1.0;
      resources.base[RESOURCE_ENERGY] = 100;
      resources.base[RESOURCE_MANA] = 0;
      resources.base[RESOURCE_CHI] = 0;
      base_energy_regen_per_second = 10.0;
      break;
    }
    case MONK_MISTWEAVER:
    {
      base.distance = 40;
      base.spell_power_per_intellect = 1.0;
      resources.base[RESOURCE_ENERGY] = 0;
      resources.base[RESOURCE_CHI] = 0;
      base_energy_regen_per_second = 0;
      break;
    }
    case MONK_WINDWALKER:
    {
      base.distance = 3;
      base_gcd += spec.stance_of_the_fierce_tiger -> effectN( 6 ).time_value(); // Saved as -500 milliseconds
      base.attack_power_per_agility = 1.0;
      resources.base[RESOURCE_ENERGY] = 100;
      resources.base[RESOURCE_ENERGY] += sets.set(MONK_WINDWALKER, T18, B4) -> effectN( 2 ).base_value();
      resources.base[RESOURCE_MANA] = 0;
      resources.base[RESOURCE_CHI] = 4;
      resources.base[RESOURCE_CHI] += spec.stance_of_the_fierce_tiger -> effectN( 6 ).base_value();
      resources.base[RESOURCE_CHI] += talent.ascension -> effectN( 1 ).base_value();
      base_energy_regen_per_second = 10.0;

      if ( artifact.inner_peace.rank() )
        resources.base[RESOURCE_ENERGY] += artifact.inner_peace.value();
      break;
    }
    default: break;
  }

  base_chi_regen_per_second = 0;
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

  if ( specialization() == MONK_WINDWALKER )
  {
    // Touch of Death
    scales_with[ STAT_STAMINA ] = true;
  }
  if ( specialization() == MONK_BREWMASTER )
  {
    scales_with[ STAT_BONUS_ARMOR ] = true;
  }

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

  buff.tier19_oh_8pc = stat_buff_creator_t( this, "grandmasters_wisdom" )
    .spell( sets.set( specialization(), T19OH, B8 ) -> effectN( 1 ).trigger() );

  // Brewmaster
  buff.bladed_armor = buff_creator_t( this, "bladed_armor", spec.bladed_armor )
    .default_value( spec.bladed_armor -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_ATTACK_POWER );

  buff.blackout_combo = buff_creator_t( this, "blackout_combo", talent.blackout_combo -> effectN( 5 ).trigger() );

  buff.brew_stache = buff_creator_t( this, "brew_stache", artifact.brew_stache.data().effectN( 1 ).trigger() )
    .default_value( artifact.brew_stache.data().effectN( 1 ).trigger() -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_DODGE );

  buff.elusive_brawler = buff_creator_t( this, "elusive_brawler", mastery.elusive_brawler -> effectN( 3 ).trigger() )
    .max_stack( 100 ) // Hard code 100 stacks based on testing 07/20/16 https://twitter.com/Phazius/status/755956860583149568
    .add_invalidate( CACHE_DODGE );

  buff.elusive_dance = buff_creator_t(this, "elusive_dance", passives.elusive_dance)
    .default_value( talent.elusive_dance -> effectN( 1 ).percent() ) // 5% per stack
    .max_stack( 3 ) // Cap of 15%
    .add_invalidate( CACHE_DODGE );

  buff.exploding_keg = buff_creator_t( this, "exploding_keg", artifact.exploding_keg )
    .default_value( artifact.exploding_keg.data().effectN( 2 ).percent() );

  buff.fortification = buff_creator_t( this, "fortification", artifact.fortification.data().effectN( 1 ).trigger() )
    .default_value( artifact.fortification.data().effectN( 1 ).trigger() -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_DODGE );

  buff.ironskin_brew = buff_creator_t(this, "ironskin_brew", passives.ironskin_brew )
    .default_value( passives.ironskin_brew -> effectN( 1 ).percent() 
      + ( sets.has_set_bonus( MONK_BREWMASTER, T19, B2 ) ? sets.set( MONK_BREWMASTER, T19, B2 ) -> effectN( 1 ).percent() : 0 ) )
    .duration( passives.ironskin_brew -> duration() + ( artifact.potent_kick.rank() ? timespan_t::from_seconds( artifact.potent_kick.value() ) : timespan_t::zero() ) )
    .refresh_behavior( BUFF_REFRESH_EXTEND );

  buff.keg_smash_talent = buff_creator_t( this, "keg_smash", talent.gift_of_the_mists -> effectN( 1 ).trigger() )
    .chance( talent.gift_of_the_mists -> proc_chance() ); 

  buff.gift_of_the_ox = buff_creator_t( this, "gift_of_the_ox", passives.gift_of_the_ox_summon )
    .duration( passives.gift_of_the_ox_summon -> duration() )
    .refresh_behavior( BUFF_REFRESH_NONE )
    .max_stack( 99 );

  buff.gifted_student = buff_creator_t( this, "gifted_student", artifact.gifted_student.data().effectN( 1 ).trigger() )
    .default_value( artifact.gifted_student.rank() ? artifact.gifted_student.percent() : 0 )
    .add_invalidate( CACHE_CRIT_CHANCE );

  buff.greater_gift_of_the_ox = buff_creator_t( this, "greater_gift_of_the_ox" , passives.gift_of_the_ox_summon )
    .duration( passives.gift_of_the_ox_summon -> duration() )
    .refresh_behavior( BUFF_REFRESH_NONE )
    .max_stack( 99 );

  buff.swift_as_a_coursing_river = buff_creator_t( this, "swift_as_a_coursing_river", artifact.swift_as_a_coursing_river.data().effectN( 1 ).trigger() )
    .default_value( artifact.swift_as_a_coursing_river.data().effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  // Mistweaver
  buff.channeling_soothing_mist = buff_creator_t( this, "channeling_soothing_mist", passives.soothing_mist_heal );

  buff.life_cocoon = absorb_buff_creator_t( this, "life_cocoon", spec.life_cocoon )
    .source( get_stats( "life_cocoon" ) )
    .cd( timespan_t::zero() );

  buff.mana_tea = buff_creator_t( this, "mana_tea", talent.mana_tea )
    .default_value( talent.mana_tea -> effectN( 1 ).percent() );

  buff.lifecycles_enveloping_mist = buff_creator_t( this, "lifecycles_enveloping_mist", passives.lifecycles_enveloping_mist )
    .default_value( passives.lifecycles_enveloping_mist -> effectN( 1 ).percent() );

  buff.lifecycles_vivify = buff_creator_t( this, "lifecycles_vivify", passives.lifecycles_vivify )
    .default_value( passives.lifecycles_vivify -> effectN( 1 ).percent() );

  buff.light_on_your_feet = buff_creator_t( this, "light_on_your_feet", find_spell( 199407 ) )
    .default_value( artifact.light_on_your_feet_mw.rank() ? artifact.light_on_your_feet_mw.percent() : 0 );

  buff.refreshing_jade_wind = buff_creator_t( this, "refreshing_jade_wind", talent.refreshing_jade_wind )
    .default_value( talent.refreshing_jade_wind -> effectN( 2 ).percent() )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );

  buff.spinning_crane_kick = buff_creator_t( this, "spinning_crane_kick", spec.spinning_crane_kick )
    .default_value( spec.spinning_crane_kick -> effectN( 2 ).percent() )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );

  buff.teachings_of_the_monastery = buff_creator_t( this, "teachings_of_the_monastery", passives.teachings_of_the_monastery_buff )
    .default_value( passives.teachings_of_the_monastery_buff -> effectN( 1 ).percent() );

  buff.thunder_focus_tea = buff_creator_t( this, "thunder_focus_tea", spec.thunder_focus_tea )
    .max_stack( 1 + ( talent.focused_thunder ? talent.focused_thunder -> effectN( 1 ).base_value()  : 0 ) );

  buff.uplifting_trance = buff_creator_t( this, "uplifting_trance", passives.uplifting_trance )
    .chance( spec.renewing_mist -> effectN( 2 ).percent() 
      + ( sets.has_set_bonus( MONK_MISTWEAVER, T19, B2 ) ? sets.set( MONK_MISTWEAVER, T19, B2 ) -> effectN( 1 ).percent() : 0 ) )
    .default_value( passives.uplifting_trance -> effectN( 1 ).percent() );

  buff.mistweaving = buff_creator_t(this, "mistweaving", passives.tier17_2pc_heal )
    .default_value( passives.tier17_2pc_heal -> effectN( 1 ).percent() )
    .max_stack( 7 )
    .add_invalidate( CACHE_CRIT_CHANCE )
    .add_invalidate( CACHE_SPELL_CRIT_CHANCE );

  buff.chi_jis_guidance = buff_creator_t( this, "chi_jis_guidance", passives.tier17_4pc_heal )
    .default_value( passives.tier17_4pc_heal -> effectN( 1 ).percent() );

  buff.extend_life = buff_creator_t( this, "extend_life", passives.tier18_2pc_heal )
    .default_value( passives.tier18_2pc_heal -> effectN( 2 ).percent() );

  // Windwalker
  buff.chi_orbit = buff_creator_t( this, "chi_orbit", talent.chi_orbit )
    .default_value( passives.chi_orbit -> effectN( 1 ).ap_coeff() )
    .max_stack( 4 );

  buff.bok_proc = buff_creator_t( this, "bok_proc", passives.bok_proc )
    .chance( spec.combo_breaker -> effectN( 1 ).percent() + 
      ( artifact.strength_of_xuen.rank() ? artifact.strength_of_xuen.percent() : 0 ) );

  buff.combo_master = buff_creator_t( this, "combo_master", passives.tier19_4pc_melee )
    .default_value( passives.tier19_4pc_melee -> effectN( 1 ).base_value() )
    .add_invalidate( CACHE_MASTERY );

  buff.combo_strikes = buff_creator_t( this, "combo_strikes" )
    .duration( timespan_t::from_minutes( 60 ) )
    .quiet( true ) // In-game does not show this buff but I would like to use it for background stuff
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.forceful_winds = buff_creator_t( this, "forceful_winds", passives.tier17_4pc_melee )
    .default_value( passives.tier17_4pc_melee -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_CRIT_CHANCE )
    .add_invalidate( CACHE_SPELL_CRIT_CHANCE );

  buff.hit_combo = buff_creator_t( this, "hit_combo", passives.hit_combo )
    .default_value( passives.hit_combo -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.masterful_strikes = buff_creator_t(this, "masterful_strikes", passives.tier18_2pc_melee )
    .default_value( passives.tier18_2pc_melee -> effectN( 1 ).base_value() )
    .add_invalidate( CACHE_MASTERY );

  buff.serenity = buff_creator_t( this, "serenity", talent.serenity )
    .default_value( talent.serenity -> effectN( 2 ).percent() +
      ( artifact.spiritual_focus.rank() ? artifact.spiritual_focus.percent() : 0 ) )
    .duration( talent.serenity -> duration() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buff.storm_earth_and_fire = buff_creator_t( this, "storm_earth_and_fire", spec.storm_earth_and_fire )
                              .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                              .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
                              .cd( timespan_t::zero() );

  buff.transfer_the_power = buff_creator_t( this, "transfer_the_power", artifact.transfer_the_power.data().effectN( 1 ).trigger() )
    .default_value( artifact.transfer_the_power.rank() ? artifact.transfer_the_power.percent() : 0 ); 

  // Legendaries
  buff.hidden_masters_forbidden_touch = new buffs::hidden_masters_forbidden_touch_t( 
    *this, "hidden_masters_forbidden_touch", passives.hidden_masters_forbidden_touch );

  buff.emperors_electric_charge = buff_creator_t( this, "emperors_electric_charge", passives.the_emperor_capacitor )
    .default_value( passives.the_emperor_capacitor -> effectN( 1 ).percent() );
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  base_t::init_gains();

  gain.black_ox_brew_energy     = get_gain( "black_ox_brew_energy" );
  gain.chi_refund               = get_gain( "chi_refund" );
  gain.power_strikes            = get_gain( "power_strikes" );
  gain.bok_proc                 = get_gain( "blackout_kick_proc" );
  gain.crackling_jade_lightning = get_gain( "crackling_jade_lightning" );
  gain.energizing_elixir_energy = get_gain( "energizing_elixir_energy" );
  gain.energizing_elixir_chi    = get_gain( "energizing_elixir_chi" );
  gain.energy_refund            = get_gain( "energy_refund" );
  gain.keg_smash                = get_gain( "keg_smash" );
  gain.mana_tea                 = get_gain( "mana_tea" );
  gain.renewing_mist            = get_gain( "renewing_mist" );
  gain.serenity                 = get_gain( "serenity" );
  gain.soothing_mist            = get_gain( "soothing_mist" );
  gain.spinning_crane_kick      = get_gain( "spinning_crane_kick" );
  gain.spirit_of_the_crane      = get_gain( "spirit_of_the_crane" );
  gain.rushing_jade_wind        = get_gain( "rushing_jade_wind" );
  gain.effuse                   = get_gain( "effuse" );
  gain.tier17_2pc_healer        = get_gain( "tier17_2pc_healer" );
  gain.tiger_palm               = get_gain( "tiger_palm" );
  gain.gift_of_the_ox           = get_gain( "gift_of_the_ox" );
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.bok_proc                   = get_proc( "bok_proc" );
  proc.eye_of_the_tiger           = get_proc( "eye_of_the_tiger" );
  proc.mana_tea                   = get_proc( "mana_tea" );
  proc.tier17_4pc_heal            = get_proc( "tier17_2pc_heal" );
}

// monk_t::init_rng =======================================================

void monk_t::init_rng()
{
  player_t::init_rng();
}

// monk_t::init_resources ===================================================

void monk_t::init_resources( bool force )
{
  player_t::init_resources( force );

  if ( artifact.healthy_appetite.rank() )
  {
    recalculate_resource_max( RESOURCE_HEALTH );
    resources.initial[ RESOURCE_HEALTH ] = resources.current[ RESOURCE_HEALTH ]
      = resources.max[ RESOURCE_HEALTH ];
  }
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

  previous_combo_strike = CS_NONE;
}

// monk_t::regen (brews/teas)================================================

void monk_t::regen( timespan_t periodicity )
{
  // resource_e resource_type = primary_resource();

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

// monk_t::recalculate_resource_max =========================================

void monk_t::recalculate_resource_max( resource_e r )
{
  player_t::recalculate_resource_max( r );

  if ( r == RESOURCE_HEALTH )
  {
    resources.max[ RESOURCE_HEALTH ] *= 1.0 + artifact.healthy_appetite.percent();
  }
}

// monk_t::create_storm_earth_and_fire_target_list ====================================

std::vector<player_t*> monk_t::create_storm_earth_and_fire_target_list() const
{
  // Make a copy of the non sleeping target list
  auto l = sim -> target_non_sleeping_list.data();

  // Sort the list by selecting non-cyclone striked targets first, followed by ascending order of
  // the debuff remaining duration
  range::sort( l, [ this ]( player_t* l, player_t* r ) {
    auto lcs = get_target_data( l ) -> debuff.mark_of_the_crane;
    auto rcs = get_target_data( r ) -> debuff.mark_of_the_crane;
    // Neither has cyclone strike
    if ( ! lcs -> check() && ! rcs -> check() )
    {
      return false;
    }
    // Left side does not have cyclone strike, right side does
    else if ( ! lcs -> check() && rcs -> check() )
    {
      return true;
    }
    // Left side has cyclone strike, right side does not
    else if ( lcs -> check() && ! rcs -> check() )
    {
      return false;
    }

    // Both have cyclone strike, order by remaining duration
    return lcs -> remains() < rcs -> remains();
  } );

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s storm_earth_and_fire target list, n_targets=%u", name(), l.size() );
    range::for_each( l, [ this ]( player_t* t ) {
      sim -> out_debug.printf( "%s cs=%.3f",
        t -> name(),
        get_target_data( t ) -> debuff.mark_of_the_crane -> remains().total_seconds() );
    } );
  }

  return l;
}

// monk_t::retarget_storm_earth_and_fire ====================================

void monk_t::retarget_storm_earth_and_fire( pet_t* pet, std::vector<player_t*>& targets, size_t n_targets ) const
{
  player_t* original_target = pet -> target;

  // Everyone attacks the same (single) target
  if ( n_targets == 1 )
  {
    pet -> target = targets.front();
  }
  // Pets attack the target the owner is not attacking
  else if ( n_targets == 2 )
  {
    pet -> target = targets.front() == pet -> owner -> target ? targets.back() : targets.front();
  }
  // 3 targets, split evenly by skipping the owner's target and picking the first available target
  else if ( n_targets == 3 )
  {
    auto it = targets.begin();
    while ( it != targets.end() )
    {
      // Don't attack owner's target
      if ( *it == pet -> owner -> target )
      {
        it++;
        continue;
      }

      pet -> target = *it;
      // This target has been chosen, so remove from the list (so that the second pet can choose
      // something else)
      targets.erase( it );
      break;
    }
  }
  // More than 3 targets, choose suitable ones from the target list
  else
  {
    auto it = targets.begin();
    while ( it != targets.end() )
    {
      // Don't attack owner's target
      if ( *it == pet -> owner -> target )
      {
        it++;
        continue;
      }

      // Don't attack my own target
      if ( *it == pet -> target )
      {
        it++;
        continue;
      }

      pet -> target = *it;
      // This target has been chosen, so remove from the list (so that the second pet can choose
      // something else)
      targets.erase( it );
      break;
    }
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s storm_earth_and_fire %s (re)target=%s old_target=%s", name(),
        pet -> name(), pet -> target -> name(), original_target -> name() );
  }
}

// monk_t::retarget_storm_earth_and_fire_pets =======================================

void monk_t::retarget_storm_earth_and_fire_pets() const
{
  if ( pet.sef[ SEF_EARTH ] -> sticky_target == true )
  {
    return;
  }

  auto targets = create_storm_earth_and_fire_target_list();
  auto n_targets = targets.size();
  retarget_storm_earth_and_fire( pet.sef[ SEF_EARTH ], targets, n_targets );
  retarget_storm_earth_and_fire( pet.sef[ SEF_FIRE  ], targets, n_targets );
}

// monk_t::has_stagger ======================================================

bool monk_t::has_stagger()
{
  return active_actions.stagger_self_damage -> stagger_ticking();
}

// monk_t::partial_clear_stagger ====================================================

double monk_t::partial_clear_stagger( double clear_percent )
{
  return active_actions.stagger_self_damage -> clear_partial_damage( clear_percent );
}

// monk_t::clear_stagger ==================================================

double monk_t::clear_stagger()
{
  return active_actions.stagger_self_damage -> clear_all_damage();
}

// monk_t::composite_attack_speed =========================================

// monk_t::composite_melee_crit_chance ============================================

double monk_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  if ( buff.mistweaving -> check() )
    crit += buff.mistweaving -> stack_value();

  if ( buff.gifted_student -> check() )
    crit += buff.gifted_student -> value();

  return crit;
}

// monk_t::composite_melee_crit_chance_multiplier ===========================

double monk_t::composite_melee_crit_chance_multiplier() const
{
  double crit = player_t::composite_melee_crit_chance_multiplier();

  if ( buff.forceful_winds -> check() )
    crit += buff.forceful_winds -> value();

  return crit;
}

// monk_t::composite_spell_crit_chance ============================================

double monk_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// monk_t::composte_spell_crit_chance_multiplier===================================

double monk_t::composite_spell_crit_chance_multiplier() const
{
  double crit = player_t::composite_spell_crit_chance_multiplier();

  if ( buff.forceful_winds -> check() )
    crit += buff.forceful_winds -> value();

  return crit;
}

// monk_t::composite_player_multiplier =====================================

double monk_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  m *= 1.0 + spec.stance_of_the_fierce_tiger -> effectN( 3 ).percent();

  if ( buff.serenity -> up() )
  {
    double ser_mult = talent.serenity -> effectN( 2 ).percent();
    if ( artifact.spiritual_focus.rank() )
      ser_mult += artifact.spiritual_focus.data().effectN( 5 ).percent();
    m *= 1 + ser_mult;
  }

  if ( buff.storm_earth_and_fire -> up() )
  {
    double sef_mult = spec.storm_earth_and_fire -> effectN( 1 ).percent();
    if ( artifact.spiritual_focus.rank() )
      sef_mult += artifact.spiritual_focus.data().effectN( 1 ).percent();
    m *= 1.0 + sef_mult;
  }

  if ( talent.hit_combo -> ok() )
    m *= 1.0 + buff.hit_combo -> stack_value();

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
  double cam = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA )
    cam *= 1.0 + spec.stagger -> effectN( 6 ).percent();

  return cam;
}

// monk_t::composite_player_heal_multiplier ==================================

double monk_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  if ( buff.serenity -> up() )
  {
    double ser_mult = talent.serenity -> effectN( 3 ).percent();
    if ( artifact.spiritual_focus.rank() )
      ser_mult += artifact.spiritual_focus.data().effectN( 6 ).percent();
    m *= 1+ ser_mult;
  }

  if ( buff.storm_earth_and_fire -> up() )
  {
    double sef_mult = spec.storm_earth_and_fire -> effectN( 2 ).percent();
    if ( artifact.spiritual_focus.rank() )
      sef_mult += artifact.spiritual_focus.data().effectN( 2 ).percent();
    m *= 1.0 + sef_mult;
  }

  return m;
}

// monk_t::composite_melee_expertise ========================================

double monk_t::composite_melee_expertise( const weapon_t* weapon ) const
{
  double e = player_t::composite_melee_expertise( weapon );

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
  double p = player_t::composite_parry();

  return p;
}

// monk_t::composite_dodge ==============================================

double monk_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  if ( buff.brew_stache -> up() )
    d += buff.brew_stache -> value();

  if ( buff.elusive_brawler -> up() )
    d += buff.elusive_brawler -> current_stack * cache.mastery_value();

  if ( buff.elusive_dance -> up() )
    d += buff.elusive_dance -> stack_value();

  if ( buff.fortification -> up() )
    d += buff.fortification -> value();

  if ( artifact.light_on_your_feet_ww.rank() )
    d += artifact.light_on_your_feet_ww.percent();

  return d;
}

// monk_t::composite_crit_avoidance =====================================

double monk_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.stagger -> effectN( 8 ).percent();

  return c;
}

// monk_t::composite_mastery ===========================================

double monk_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  if ( buff.masterful_strikes -> up() )
    m += buff.masterful_strikes -> value();

  return m;
}

// monk_t::composite_mastery_rating ====================================

double monk_t::composite_mastery_rating() const
{
  double m = player_t::composite_mastery_rating();

  if ( buff.combo_master -> up() )
    m += buff.combo_master -> value();

  return m;
}

// monk_t::composite_rating_multiplier =================================

double monk_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  return m;
}

// monk_t::composite_armor_multiplier ===================================

double monk_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  a += spec.stagger -> effectN( 14 ).percent();

  if ( specialization() == MONK_BREWMASTER )
    a += passives.aura_brewmaster_monk -> effectN( 6 ).percent();

  return a;
}

// monk_t::temporary_movement_modifier =====================================

double monk_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

  return active;
}

// monk_t::passive_movement_modifier =======================================

double monk_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  return ms;
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
  case CACHE_MASTERY:
    if ( specialization() == MONK_WINDWALKER )
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    break;
  default: break;
  }
}


// monk_t::create_options ===================================================

void monk_t::create_options()
{
  base_t::create_options();

  add_option( opt_int( "initial_chi", user_options.initial_chi ) );
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
}

// monk_t::energy_regen_per_second ==========================================

double monk_t::energy_regen_per_second() const
{
  double r = base_t::energy_regen_per_second();

  r *= 1.0 + talent.ascension -> effectN( 2 ).percent();

  return r;
}

// monk_t::combat_begin ====================================================

void monk_t::combat_begin()
{
  base_t::combat_begin();

  if ( specialization() == MONK_WINDWALKER)
  {
    resources.current[RESOURCE_CHI] = 0;

    if ( !buffs.windwalking_movement_aura -> up() )
    {
      for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
      {
        player_t* p = sim -> player_non_sleeping_list[i];
        if ( p -> type == PLAYER_GUARDIAN )
          continue;

        p -> buffs.windwalking_movement_aura -> trigger();
      }
    }

    if ( talent.chi_orbit -> ok() )
    {
      // If Chi Orbit, start out with max stacks
      buff.chi_orbit -> trigger( buff.chi_orbit -> max_stack() );
      make_event<chi_orbit_event_t>( *sim, *this, timespan_t::zero() );
      make_event<chi_orbit_trigger_event_t>( *sim, *this, timespan_t::zero() );
    }

    if ( talent.power_strikes -> ok() )
      make_event<power_strikes_event_t>( *sim, *this, timespan_t::zero() );
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

      if ( artifact.gifted_student.rank() )
        buff.gifted_student -> trigger();
    }
  }

  if ( action_t::result_is_hit( s -> result ) && s -> action -> id != passives.stagger_self_damage -> id() )
  {
    // trigger the mastery if the player gets hit by a physical attack; but not from stagger
    if ( school == SCHOOL_PHYSICAL )
      buff.elusive_brawler -> trigger();
  }

  base_t::assess_damage( school, dtype, s );
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

  // Dampen Harm // Currently reduces hits below 15% hp as well
  double dampen_health = max_health() * buff.dampen_harm -> data().effectN( 1 ).percent();
  if ( buff.dampen_harm -> up() && s -> result_amount >= dampen_health )
  {
    s -> result_amount *= 1.0 - buff.dampen_harm -> data().effectN( 2 ).percent(); // Dampen Harm reduction is stored as +50
    buff.dampen_harm -> decrement(); // A stack will only be removed if the reduction was applied.
  }

  // Diffuse Magic
  if ( buff.diffuse_magic -> up() && school != SCHOOL_PHYSICAL )
    s -> result_amount *= 1.0 + buff.diffuse_magic -> default_value; // Stored as -60%

  // Damage Reduction Cooldowns
  if ( buff.fortifying_brew -> up() )
    s -> result_amount *= 1.0 - spec.fortifying_brew -> effectN( 1 ).percent();

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
    {
      if ( buff.exploding_keg -> up() ) 
        s -> result_amount *= 1.0 + buff.exploding_keg -> value();

      // Passive sources (Sturdy Ox)
      if ( school != SCHOOL_PHYSICAL )
        // TODO: Magical Damage reduction (currently set to zero, but effect is still in place)
        s -> result_amount *= 1.0 + spec.stagger -> effectN( 5 ).percent();
      break;
    }
    case MONK_MISTWEAVER:
    {
      if ( artifact.shroud_of_mist.rank() )
        s -> result_amount *= 1.0 + artifact.shroud_of_mist.value();
      break;
    }
    default: break;
  }

  double health_before_hit = resources.current[RESOURCE_HEALTH];

  player_t::target_mitigation( school, dt, s );

  // cap HP% at 0 HP since SimC can fall below 0 HP
  double health_percent_after_the_hit = fmax( ( resources.current[RESOURCE_HEALTH] - s -> result_amount ) / max_health(), 0 );

  if ( talent.healing_elixirs -> ok() )
  {
    // TODO: 35% HP for Healing Elixirs is hard-coded until otherwise changed
    if ( resources.pct(RESOURCE_HEALTH) > 0.35 && health_percent_after_the_hit <= 0.35 && cooldown.healing_elixirs -> up() )
      active_actions.healing_elixir -> execute();
  }

  // Gift of the Ox Trigger Calculations ===========================================================

  if ( specialization() == MONK_BREWMASTER )
  {
    // Obstinate Determination is a separate roll to the normal GotO chance to proc.
    // TODO: 35% HP for Obstinate Determination is hard-coded until otherwise changed
    if ( artifact.obstinate_determination.rank() && s -> result_amount > 0 
      && resources.pct(RESOURCE_HEALTH) > 0.35 && health_percent_after_the_hit <= 0.35 )
    {
      if ( artifact.overflow.rank() )
      { 
        if ( rng().roll( artifact.overflow.percent() ) )
          buff.greater_gift_of_the_ox -> trigger();
        else
          buff.gift_of_the_ox -> trigger();
      }
      else
        buff.gift_of_the_ox -> trigger();
    }

    // Gift of the Ox is no longer a random chance, under the hood. When you are hit, it increments a counter by (DamageTakenBeforeAbsorbsOrStagger / MaxHealth).
    // It now drops an orb whenever that reaches 1.0, and decrements it by 1.0. The tooltip still says ‘chance’, to keep it understandable.
    // Gift of the Mists multiplies that counter increment by (2 - (HealthBeforeDamage - DamageTakenBeforeAbsorbsOrStagger) / MaxHealth);
    double goto_proc_chance = s -> result_amount / max_health();

    if ( talent.gift_of_the_mists -> ok() )
      // Due to the fact that SimC can cause HP values to go into negative, force the cap to be 160% since the original formula can go above 160% with negative HP
      goto_proc_chance *= 1 + ( talent.gift_of_the_mists -> effectN( 1 ).percent() * ( 1 - fmax( ( health_before_hit - s -> result_amount ) / max_health(), 0 ) ) );

    gift_of_the_ox_proc_chance += goto_proc_chance;

    if ( gift_of_the_ox_proc_chance > 1.0 )
    {
      if ( artifact.overflow.rank() )
      { 
        if ( rng().roll( artifact.overflow.percent() ) )
          buff.greater_gift_of_the_ox -> trigger();
        else
          buff.gift_of_the_ox -> trigger();
      }
      else
        buff.gift_of_the_ox -> trigger();

      gift_of_the_ox_proc_chance -= 1.0;
    }
  }
}

// monk_t::assess_damage_imminent_pre_absorb ==============================

void monk_t::assess_damage_imminent_pre_absorb( school_e school,
                                                dmg_e    dtype,
                                                action_state_t* s )
{
  base_t::assess_damage_imminent_pre_absorb( school, dtype, s );

  if ( specialization() == MONK_BREWMASTER )
  {
    // Stagger damage can't be staggered!
    if ( s -> action -> id == passives.stagger_self_damage -> id() )
      return;

    // Stagger Calculation
    double stagger_dmg = 0;

    if ( s -> result_amount > 0 )
    {
      if ( school == SCHOOL_PHYSICAL )
        stagger_dmg += s -> result_amount * stagger_pct();

      else if ( school != SCHOOL_PHYSICAL )
        // hard code the 50% effectiveness of stagger
        stagger_dmg += s -> result_amount * ( stagger_pct() * 0.5 );

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
}

// monk_t::assess_heal ===================================================

void monk_t::assess_heal( school_e school, dmg_e dmg_type, action_state_t* s )
{
  // Celestial Fortune procs a heal every now and again
/*  if ( s -> action -> id != passives.healing_elixirs -> id() 
    || s -> action -> id != passives.gift_of_the_ox_heal -> id()
    || s -> action -> id != passives.greater_gift_of_the_ox_heal -> id() )
  {
  */
//    if ( spec.celestial_fortune -> ok() )
//      trigger_celestial_fortune( s );
//  }

  player_t::assess_heal( school, dmg_type, s );
}

// Brewmaster Pre-Combat Action Priority List ============================

void monk_t::apl_pre_brewmaster()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  // Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    
//    if ( true_level > 100)
//      pre -> add_action( "flask,type=flask_of_the_seventh_demon" );
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
    //if ( level() > 100 )
    //  pre -> add_action( "food,type=the_hungry_magister" ); //fixmewithproperfood
    if ( level() > 90)
      pre -> add_action( "food,type=sleeper_sushi" );
    else if ( level() >= 80 )
      pre -> add_action( "food,type=skewered_eel" );
    else if ( level() >= 70 )
      pre -> add_action( "food,type=blackened_dragonfin" );
    else
      pre -> add_action( "food,type=warp_burger" );
  }

//  if ( true_level > 100 )
//    pre -> add_action( "augmentation,type=defiled" );

  pre -> add_action( "snapshot_stats" );

  if ( sim -> allow_potions )
  {
    //if ( true_level > 100 )
    //  pre -> add_action( "potion,name=old_war" );
    if ( true_level > 90 )
      pre -> add_action( "potion,name=draenic_agility" );
    else if ( true_level >= 85 )
      pre -> add_action( "potion,name=virmens_bite" );
    else
      pre -> add_action( "potion,name=tolvir" );
  }

  pre -> add_talent( this, "Diffuse Magic" );
  pre -> add_talent( this, "Dampen Harm" );
  pre -> add_talent( this, "Chi Burst" );
  pre -> add_talent( this, "Chi Wave" );

}

// Windwalker Pre-Combat Action Priority List ==========================

void monk_t::apl_pre_windwalker()
{
  action_priority_list_t* pre = get_action_priority_list("precombat");
//  std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

  if ( sim -> allow_flasks )
  {
  // Flask
    if ( true_level > 100)
      pre -> add_action( "flask,type=flask_of_the_seventh_demon" );
    else if ( true_level > 90 )
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
    if ( level() > 100 )
      pre -> add_action( "food,type=fishbrul_special" );
    else if ( level() > 90 )
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

  if ( true_level > 100 )
    pre -> add_action( "augmentation,type=defiled" );

  pre -> add_action( "snapshot_stats" );

  if ( sim -> allow_potions )
  {
    // Prepotion
    if ( true_level > 100 )
      pre -> add_action( "potion,name=old_war" );
    else if ( true_level > 90 )
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
    // Flask
    if ( true_level > 100)
      pre -> add_action( "flask,type=flask_of_the_whispered_pact" );
    else if ( true_level > 90 )
      pre -> add_action( "flask,type=greater_draenic_intellect_flask" );
    else if ( true_level >= 85 )
      pre -> add_action( "flask,type=warm_sun" );
    else if ( true_level > 80 )
      pre -> add_action( "flask,type=draconic_mind" );
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( level() > 100 )
      pre -> add_action( "food,type=nightborne_delicacy_platter" );
    else if ( level() > 90 )
      pre -> add_action( "food,type=salty_squid_roll" );
    else if ( level() >= 85 )
      pre -> add_action( "food,type=mogu_fish_stew" );
    else if ( level() > 80 )
      pre -> add_action( "food,type=seafood_magnifique_feast" );
  }

  pre -> add_action( "snapshot_stats" );

  if ( sim -> allow_potions )
  {
    // Prepotion
    if ( true_level > 100 )
      pre -> add_action( "potion,name=deadly_grace" );
    else if ( true_level > 90 )
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
//  action_priority_list_t* aoe = get_action_priority_list( "aoe" );

  def -> add_action( "auto_attack" );

/*  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
      def -> add_action( racial_actions[i] + ",if=energy<=40" );
  }
*/

//  def -> add_talent( this, "Chi Brew", "if=(chi<1&stagger.heavy)|(chi<2)" );
//  def -> add_action( this, "Gift of the Ox", "if=buff.gift_of_the_ox.react&incoming_damage_1500ms" );
//  def -> add_talent( this, "Diffuse Magic", "if=incoming_damage_1500ms&buff.fortifying_brew.down" );
//  def -> add_talent( this, "Dampen Harm", "if=incoming_damage_1500ms&buff.fortifying_brew.down" );
//  def -> add_action( this, "Fortifying Brew", "if=incoming_damage_1500ms&(buff.dampen_harm.down|buff.diffuse_magic.down)" );

/*  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      def -> add_action( "use_item,name=" + items[i].name_str + ",if=incoming_damage_1500ms&(buff.dampen_harm.down|buff.diffuse_magic.down)&buff.fortifying_brew.down" );
  }
  */

//  def -> add_action( "invoke_niuzao,if=talent.invoke_niuzao.enabled&target.time_to_die>15&buff.serenity.down" );
//  def -> add_talent( this, "Serenity", "if=talent.serenity.enabled&cooldown.keg_smash.remains>6" );
  
/*  if ( sim -> allow_potions )
  {
    if ( true_level >= 85 )
      def -> add_action( "potion,name=virmens_bite,if=(buff.fortifying_brew.down&(buff.dampen_harm.down|buff.diffuse_magic.down))" );
  }
*/

  def -> add_action( "call_action_list,name=st,if=active_enemies<3" );
//  def -> add_action( "call_action_list,name=aoe,if=active_enemies>=3" );

//  st -> add_action( this, "Purifying Brew", "if=stagger.heavy" );
//  st -> add_action( this, "Purifying Brew", "if=buff.serenity.up" );
//  st -> add_action( this, "Purifying Brew", "if=stagger.moderate" );
  //  st->add_talent(this, "Black Ox Brew");
  st -> add_action( this, "Keg Smash" );
  st -> add_action( this, "Blackout Strike" );
  st -> add_action( this, "Exploding Keg" );
  st -> add_talent( this, "Chi Burst" );
  st -> add_talent( this, "Chi Wave" );
  st -> add_talent( this, "Rushing Jade Wind" );
  st -> add_action( this, "Breath of Fire");
  st -> add_action( this, "Tiger Palm" );//, "if=(energy+(energy.regen*cooldown.keg_smash.remains))>=40");

/*  aoe -> add_action( this, "Purifying Brew", "if=stagger.heavy" );
  aoe -> add_action( this, "Purifying Brew", "if=buff.serenity.up" );
  aoe -> add_action( this, "Purifying Brew", "if=stagger.moderate" );
  aoe -> add_talent( this, "Black Ox Brew" );
  aoe -> add_action( this, "Keg Smash" );
  aoe -> add_action( this, "Breath of Fire", "if=debuff.keg_smash.up" );  
  aoe -> add_talent( this, "Rushing Jade Wind" );
  aoe -> add_talent( this, "Chi Burst", "if=energy.time_to_max>2&buff.serenity.down" );
  aoe -> add_talent( this, "Chi Wave", "if=energy.time_to_max>2&buff.serenity.down" );
  aoe -> add_action( this, "Tiger Palm", "if=cooldown.keg_smash.remains>=gcd&(energy+(energy.regen*(cooldown.keg_smash.remains)))>=80" );
*/
}

// Windwalker Combat Action Priority List ===============================

void monk_t::apl_combat_windwalker()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* cd = get_action_priority_list( "cd" );
  action_priority_list_t* sef = get_action_priority_list("sef");
  action_priority_list_t* serenity = get_action_priority_list("serenity");
  action_priority_list_t* st = get_action_priority_list("st");

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Spear Hand Strike", "if=target.debuff.casting.react" );

  if ( sim -> allow_potions )
  {
    if ( true_level > 100 )
      def -> add_action( "potion,name=old_war,if=buff.serenity.up|buff.storm_earth_and_fire.up|(!talent.serenity.enabled&trinket.proc.agility.react)|buff.bloodlust.react|target.time_to_die<=60" );
    else if ( true_level > 90 )
      def -> add_action( "potion,name=draenic_agility,if=buff.serenity.up|buff.storm_earth_and_fire.up|(!talent.serenity.enabled&trinket.proc.agility.react)|buff.bloodlust.react|target.time_to_die<=60" );
    else if ( true_level >= 85 )
      def -> add_action( "potion,name=virmens_bite,if=buff.bloodlust.react|target.time_to_die<=60" );
  }

  def -> add_action( "call_action_list,name=serenity,if=(talent.serenity.enabled&cooldown.serenity.remains<=0)&((artifact.strike_of_the_windlord.enabled&cooldown.strike_of_the_windlord.remains<=14&cooldown.rising_sun_kick.remains<=4)|buff.serenity.up)" );
  def -> add_action( "call_action_list,name=sef,if=!talent.serenity.enabled&((artifact.strike_of_the_windlord.enabled&cooldown.strike_of_the_windlord.remains<=14&cooldown.fists_of_fury.remains<=6&cooldown.rising_sun_kick.remains<=6)|buff.storm_earth_and_fire.up)" );
  def -> add_action( "call_action_list,name=serenity,if=(talent.serenity.enabled&cooldown.serenity.remains<=0)&(!artifact.strike_of_the_windlord.enabled&cooldown.strike_of_the_windlord.remains<14&cooldown.fists_of_fury.remains<=15&cooldown.rising_sun_kick.remains<7)|buff.serenity.up" );
  def -> add_action( "call_action_list,name=sef,if=!talent.serenity.enabled&((!artifact.strike_of_the_windlord.enabled&cooldown.fists_of_fury.remains<=9&cooldown.rising_sun_kick.remains<=5)|buff.storm_earth_and_fire.up)" );
  def -> add_action( "call_action_list,name=st" );

  // Cooldowns
  cd -> add_action( "invoke_xuen" );
  int num_items = (int)items.size();

  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      if ( items[i].name_str == "unbridled_fury" )
      {
        cd -> add_action( "use_item,name=" + items[i].name_str + ",if=!artifact.gale_burst.enabled" );
        cd -> add_action( "use_item,name=" + items[i].name_str + ",if=(!artifact.strike_of_the_windlord.enabled&cooldown.strike_of_the_windlord.remains<14&cooldown.fists_of_fury.remains<=15&cooldown.rising_sun_kick.remains<7)|buff.serenity.up" );
      }
      else if ( items[i].name_str == "tiny_oozeling_in_a_jar" )
        cd -> add_action( "use_item,name=" + items[i].name_str + ",if=buff.congealing_goo.stack>=6" );
      else
        cd -> add_action( "use_item,name=" + items[i].name_str );
    }
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] != "arcane_torrent" )
      cd -> add_action( racial_actions[i]  );
  }
  cd -> add_action( this, "Touch of Death", "cycle_targets=1,max_cycle_targets=2,if=!artifact.gale_burst.enabled&equipped.137057&!prev_gcd.touch_of_death" );
  cd -> add_action( this, "Touch of Death", "if=!artifact.gale_burst.enabled&!equipped.137057" );
  cd -> add_action( this, "Touch of Death", "cycle_targets=1,max_cycle_targets=2,if=artifact.gale_burst.enabled&equipped.137057&cooldown.strike_of_the_windlord.remains<8&cooldown.fists_of_fury.remains<=4&cooldown.rising_sun_kick.remains<7&!prev_gcd.touch_of_death" );
  cd -> add_action( this, "Touch of Death", "if=artifact.gale_burst.enabled&!equipped.137057&cooldown.strike_of_the_windlord.remains<8&cooldown.fists_of_fury.remains<=4&cooldown.rising_sun_kick.remains<7" );

  // Storm, Earth, and Fire
  sef -> add_talent( this, "Energizing Elixir" );
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      sef -> add_action( racial_actions[i] + ",if=chi.max-chi>=1&energy.time_to_max>=0.5" );
  }
  sef -> add_action( "call_action_list,name=cd" );
  sef -> add_action( this, "Storm, Earth, and Fire" );
  sef -> add_action( "call_action_list,name=st" );

  // Serenity
  serenity -> add_talent( this, "Energizing Elixir" );
  serenity -> add_action( "call_action_list,name=cd" );
  serenity -> add_talent( this, "Serenity" );
  serenity -> add_action( this, "Strike of the Windlord" );
  serenity -> add_action( this, "Rising Sun Kick", "cycle_targets=1,if=active_enemies<3" );
  serenity -> add_action( this, "Fists of Fury" );
  serenity -> add_action( this, "Spinning Crane Kick", "if=active_enemies>=3&!prev_gcd.spinning_crane_kick" );
  serenity -> add_action( this, "Rising Sun Kick", "cycle_targets=1,if=active_enemies>=3" );
  serenity -> add_action( this, "Blackout Kick", "cycle_targets=1,if=!prev_gcd.blackout_kick" );
  serenity -> add_action( this, "Spinning Crane Kick", "if=!prev_gcd.spinning_crane_kick" );
  serenity -> add_talent( this, "Rushing Jade Wind", "if=!prev_gcd.rushing_jade_wind" );

  // Single Target
  st -> add_action( "call_action_list,name=cd" );
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      st -> add_action( racial_actions[i] + ",if=chi.max-chi>=1&energy.time_to_max>=0.5" );
  }
  st -> add_talent( this, "Energizing Elixir", "if=energy<energy.max&chi<=1" );
  st -> add_action( this, "Strike of the Windlord", "if=talent.serenity.enabled|active_enemies<6" );
  st -> add_action( this, "Fists of Fury" );
  st -> add_action( this, "Rising Sun Kick", "cycle_targets=1" );
  st -> add_talent( this, "Whirling Dragon Punch" );
  st -> add_action( this, "Spinning Crane Kick", "if=active_enemies>=3&!prev_gcd.spinning_crane_kick" );
  st -> add_talent( this, "Rushing Jade Wind", "if=chi.max-chi>1&!prev_gcd.rushing_jade_wind" );
  st -> add_action( this, "Blackout Kick", "cycle_targets=1,if=(chi>1|buff.bok_proc.up)&!prev_gcd.blackout_kick" );
  st -> add_talent( this, "Chi Wave", "if=energy.time_to_max>=2.25" );
  st -> add_talent( this, "Chi Burst", "if=energy.time_to_max>=2.25" );
  st -> add_action( this, "Tiger Palm", "cycle_targets=1,if=!prev_gcd.tiger_palm" );
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

  st -> add_action( this, "Rising Sun Kick", "if=buff.teachings_of_the_monastery.up" );
  st -> add_action( this, "Blackout Kick", "if=buff.teachings_of_the_monastery.up" );
  st -> add_talent( this, "Chi Wave" );
  st -> add_talent( this, "Chi Burst" );
  st -> add_action( this, "Tiger Palm", "if=buff.teachings_of_the_monastery.down" );

  aoe -> add_action( this, "Spinning Crane Kick" );
  aoe -> add_talent( this, "Refreshing Jade Wind" );
  aoe -> add_talent( this, "Chi Burst" );
  aoe -> add_action( this, "Blackout Kick" );
  aoe -> add_action( this, "Tiger Palm", "if=talent.rushing_jade_wind.enabled" );
}

// monk_t::init_actions =====================================================

void monk_t::init_action_list()
{
#ifdef NDEBUG // Only restrict on release builds.
  // Mistweaver isn't supported atm
  if ( specialization() == MONK_MISTWEAVER )
  {
    if ( ! quiet )
      sim -> errorf( "Monk mistweaver healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }
#endif
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

    if ( buff.fortifying_brew -> check() )
    {
      stagger += spec.fortifying_brew -> effectN( 1 ).percent();

      if ( artifact.staggering_around.rank() )
        stagger += artifact.staggering_around.percent();
    }

    if ( buff.ironskin_brew -> up() )
      stagger += buff.ironskin_brew -> value();
  }

  // TODO: Cap stagger at 100% stagger since one can currently hit 105% stagger
  return fmin( stagger, 1 );
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

  if ( splits.size() == 2 && splits[0] == "spinning_crane_kick" )
  {
    struct sck_stack_expr_t : public expr_t
    {
      monk_t& player;
      sck_stack_expr_t( monk_t& p ) :
        expr_t( "stack" ),
        player( p )
      { }

      virtual double evaluate() override
      {
        return player.mark_of_the_crane_counter();
      }
    };

  }

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

// 6.2 Monk Trinket Effects -----------------------------------------------------
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

// Legion Artifact Effects --------------------------------------------------------

// Brewmaster Legion Artifact
static void fu_zan_the_wanderers_companion( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*> ( effect.player );
  do_trinket_init( monk, MONK_BREWMASTER, monk -> fu_zan_the_wanderers_companion, effect );
}

// Mistweaver Legion Artifact
static void sheilun_staff_of_the_mists( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*> ( effect.player );
  do_trinket_init( monk, MONK_MISTWEAVER, monk -> sheilun_staff_of_the_mists, effect );
}

// Windwalker Legion Artifact
static void fists_of_the_heavens( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*> ( effect.player );
  do_trinket_init( monk, MONK_WINDWALKER, monk -> fists_of_the_heavens, effect );
}

// Legion Legendary Effects ---------------------------------------------------------
// General Legendary Effects
struct cinidaria_the_symbiote_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  cinidaria_the_symbiote_t() : super( MONK )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  { monk -> legendary.cinidaria_the_symbiote = e.driver(); }
};

struct prydaz_xavarics_magnum_opus_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  prydaz_xavarics_magnum_opus_t() : super( MONK )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.prydaz_xavarics_magnum_opus = e.driver();
  }
};

struct sephuzs_secret_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  sephuzs_secret_t() : super( MONK )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.sephuzs_secret = e.driver();
  }
};

// Brewmaster Legendary Effects
struct firestone_walkers_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  firestone_walkers_t() : super( MONK_BREWMASTER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.firestone_walkers = e.driver();
  }
};

struct fundamental_observation_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  fundamental_observation_t() : super( MONK_BREWMASTER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.fundamental_observation = e.driver();
  }
};

struct gai_plins_soothing_sash_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  gai_plins_soothing_sash_t() : super( MONK_BREWMASTER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.gai_plins_soothing_sash = e.driver();
  }
};

struct jewel_of_the_lost_abbey_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  jewel_of_the_lost_abbey_t() : super( MONK_BREWMASTER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.jewel_of_the_lost_abbey = e.driver();
  }
};

struct salsalabims_lost_tunic_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  salsalabims_lost_tunic_t() : super( MONK_BREWMASTER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.salsalabims_lost_tunic = e.driver();
  }
};

// Mistweaver
struct eithas_lunar_glides_of_eramas_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  eithas_lunar_glides_of_eramas_t() : super( MONK_MISTWEAVER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.eithas_lunar_glides_of_eramas = e.driver();
  }
};

struct eye_of_collidus_the_warp_watcher_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  eye_of_collidus_the_warp_watcher_t() : super( MONK_MISTWEAVER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.eye_of_collidus_the_warp_watcher = e.driver();
  }
};

struct leggings_of_the_black_flame_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  leggings_of_the_black_flame_t() : super( MONK_MISTWEAVER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.leggings_of_the_black_flame = e.driver();
  }
};

struct ovyds_winter_wrap_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  ovyds_winter_wrap_t() : super( MONK_MISTWEAVER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.ovyds_winter_wrap = e.driver();
  }
};

struct petrichor_lagniappe_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  petrichor_lagniappe_t() : super( MONK_MISTWEAVER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.petrichor_lagniappe = e.driver();
  }
};

struct unison_spaulders_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  unison_spaulders_t() : super( MONK_MISTWEAVER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk -> legendary.unison_spaulders = e.driver();
  }
};

// Windwalker Legendary Effects
struct cenedril_reflector_of_hatred_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  cenedril_reflector_of_hatred_t() : super( MONK_WINDWALKER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  { monk -> legendary.cenedril_reflector_of_hatred = e.driver(); }
};

struct drinking_horn_cover_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  drinking_horn_cover_t() : super( MONK_WINDWALKER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  { monk -> legendary.drinking_horn_cover = e.driver(); }
};

struct hidden_masters_forbidden_touch_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  hidden_masters_forbidden_touch_t() : super( MONK_WINDWALKER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  { monk -> legendary.hidden_masters_forbidden_touch = e.driver(); }
};

struct katsuos_eclipse_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  katsuos_eclipse_t() : super( MONK_WINDWALKER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  { monk -> legendary.katsuos_eclipse = e.driver(); }
};

struct march_of_the_legion_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  march_of_the_legion_t() : super( MONK_WINDWALKER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  { monk -> legendary.march_of_the_legion = e.driver(); }
};

struct the_emperors_capacitor_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  the_emperors_capacitor_t() : super( MONK_WINDWALKER )
  { }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  { monk -> legendary.the_emperors_capacitor = e.driver(); }
};

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
    // WoD's Archemonde Trinkets
    unique_gear::register_special_effect( 184906, eluding_movements );
    unique_gear::register_special_effect( 184907, soothing_breeze );
    unique_gear::register_special_effect( 184908, furious_sun );

    // Legion Artifacts
    unique_gear::register_special_effect( 214854, fists_of_the_heavens );
    unique_gear::register_special_effect( 214483, sheilun_staff_of_the_mists );
    unique_gear::register_special_effect( 214852, fu_zan_the_wanderers_companion );

    // Legion Legendary Effects
    // General
    unique_gear::register_special_effect( 207692, cinidaria_the_symbiote_t() );
    unique_gear::register_special_effect( 207428, prydaz_xavarics_magnum_opus_t() );
    unique_gear::register_special_effect( 208051, sephuzs_secret_t() );

    // Brewmaster
    unique_gear::register_special_effect( 224489, firestone_walkers_t() );
    unique_gear::register_special_effect( 208878, fundamental_observation_t() );
    unique_gear::register_special_effect( 208837, gai_plins_soothing_sash_t() );
    unique_gear::register_special_effect( 208881, jewel_of_the_lost_abbey_t() );
    unique_gear::register_special_effect( 212935, salsalabims_lost_tunic_t() );

    // Mistweaver
    unique_gear::register_special_effect( 217153, eithas_lunar_glides_of_eramas_t() );
    unique_gear::register_special_effect( 217473, eye_of_collidus_the_warp_watcher_t() );
    unique_gear::register_special_effect( 216506, leggings_of_the_black_flame_t() );
    unique_gear::register_special_effect( 217634, ovyds_winter_wrap_t() );
    unique_gear::register_special_effect( 206902, petrichor_lagniappe_t() );
    unique_gear::register_special_effect( 212123, unison_spaulders_t() );

    // Windwalker
    unique_gear::register_special_effect( 208842, cenedril_reflector_of_hatred_t() );
    unique_gear::register_special_effect( 209256, drinking_horn_cover_t() );
    unique_gear::register_special_effect( 213112, hidden_masters_forbidden_touch_t() );
    unique_gear::register_special_effect( 208045, katsuos_eclipse_t() );
    unique_gear::register_special_effect( 212132, march_of_the_legion_t() );
    unique_gear::register_special_effect( 235053, the_emperors_capacitor_t() );
  }


  virtual void register_hotfixes() const override
  {
  }

  virtual void init( player_t* p ) const override
  {
    p -> buffs.windwalking_movement_aura = buff_creator_t( p, "windwalking_movement_aura",
                                                            p -> find_spell( 166646 ) )
      .duration( timespan_t::from_seconds( 0 ) )
      .add_invalidate( CACHE_RUN_SPEED );
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

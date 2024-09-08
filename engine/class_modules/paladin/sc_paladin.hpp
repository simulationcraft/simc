#pragma once
#include "simulationcraft.hpp"

namespace paladin
{
// Forward declarations
typedef std::pair<std::string, simple_sample_data_with_min_max_t> data_t;
typedef std::pair<std::string, simple_sample_data_t> simple_data_t;
struct paladin_t;
struct blessing_of_sacrifice_redirect_t;
namespace buffs
{
struct avenging_wrath_buff_t;
struct crusade_buff_t;
struct holy_avenger_buff_t;
struct ardent_defender_buff_t;
struct forbearance_t;
struct shield_of_vengeance_buff_t;
struct redoubt_buff_t;
struct sentinel_buff_t;
struct sentinel_decay_buff_t;
struct execution_sentence_debuff_t;
}  // namespace buffs
const int MAX_START_OF_COMBAT_HOLY_POWER = 1;

enum season : unsigned int
{
  SUMMER      = 0,
  AUTUMN      = 1,
  WINTER      = 2,
  SPRING      = 3,
  NUM_SEASONS = 4,
};

enum armament : unsigned int
{
  HOLY_BULWARK  = 0,
  SACRED_WEAPON = 1,
  NUM_ARMAMENT  = 2,
};

enum consecration_source : unsigned int
{
  HARDCAST         = 0,
  BLADE_OF_JUSTICE = 1,
  SEARING_LIGHT    = 2,
};

enum grand_crusader_source : unsigned int
{
  GC_NORMAL   = 0,
  GC_JUDGMENT = 1,
};

// ==========================================================================
// Paladin Target Data
// ==========================================================================

struct paladin_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* expurgation;
    dot_t* truths_wake;
    dot_t* dawnlight;
  } dots;

  struct buffs_t
  {
    buffs::execution_sentence_debuff_t* execution_sentence;
    buff_t* judgment;
    buff_t* judgment_of_light;
    buff_t* blessed_hammer;
    buff_t* final_reckoning;
    buff_t* sanctify;
    buff_t* eye_of_tyr;
    buff_t* crusaders_resolve;
    buff_t* heartfire;  // T30 2p Prot
    buff_t* empyrean_hammer;
    buff_t* vanguard_of_justice;
    buff_t* holy_flames;
  } debuff;

  struct
  {
    buff_t* holy_bulwark;
    buff_t* sacred_weapon;
  } buffs;

  paladin_td_t( player_t* target, paladin_t* paladin );

  bool standing_in_consecration();
};

struct paladin_t : public player_t
{
public:
  // Active spells
  struct active_spells_t
  {
    heal_t* beacon_of_light;
    action_t* holy_shield_damage;
    action_t* empyrean_hammer;
    action_t* heartfire;
    action_t* judgment_of_light;
    action_t* shield_of_vengeance_damage;
    action_t* zeal;

    action_t* inner_light_damage;
    action_t* sanctified_wrath;
    action_t* virtuous_command;
    action_t* create_aw_expression;

    // Required for seraphim
    action_t* sotr;

    blessing_of_sacrifice_redirect_t* blessing_of_sacrifice_redirect;

    // Covenant stuff
    action_t* divine_toll;
    action_t* seasons[ NUM_SEASONS ];
    action_t* divine_resonance;

    // talent stuff
    action_t* background_cons;
    action_t* incandescence;
    action_t* empyrean_legacy;
    action_t* es_explosion;
    action_t* background_blessed_hammer;
    action_t* divine_arbiter;
    action_t* searing_light;
    action_t* searing_light_cons;

    // Tier stuff
    action_t* cleansing_flame;  // Prot Tier 31 4pc
    action_t* cleansing_flame_heal;

    action_t* expurgation;
    action_t* wrathful_sanction;

    action_t* divine_hammer;
    action_t* divine_hammer_tick;

    action_t* sacrosanct_crusade_heal;
    action_t* highlords_judgment;
    action_t* dawnlight;
    action_t* sun_sear;
    action_t* suns_avatar_dmg;
    action_t* armament[ NUM_ARMAMENT ];
    action_t* sacred_weapon_proc_damage;
    action_t* sacred_weapon_proc_heal;
  } active;

  // Buffs
  struct buffs_t
  {
    // Shared
    buffs::avenging_wrath_buff_t* avenging_wrath;
    buff_t* divine_purpose;
    buff_t* divine_shield;
    buff_t* divine_steed;
    buff_t* devotion_aura;
    buff_t* retribution_aura;
    buff_t* blessing_of_protection;
    buff_t* avenging_wrath_might;
    buff_t* faiths_armor;

    // Holy
    buff_t* divine_protection;
    buff_t* holy_avenger;
    buff_t* avenging_crusader;
    buff_t* infusion_of_light;

    // Prot
    absorb_buff_t* holy_shield_absorb;     // Dummy buff to trigger spell damage "blocking" absorb effect
    absorb_buff_t* blessed_hammer_absorb;  // ^
    absorb_buff_t* divine_bulwark_absorb;  // New Mastery absorb
    buffs::sentinel_buff_t* sentinel;
    buffs::sentinel_decay_buff_t* sentinel_decay;
    buff_t* bulwark_of_order_absorb;
    buff_t* seraphim;
    buff_t* ardent_defender;
    buff_t* grand_crusader;
    buff_t* guardian_of_ancient_kings;
    buff_t* redoubt;
    buff_t* shield_of_the_righteous;
    buff_t* moment_of_glory;
    buff_t* shining_light_stacks;
    buff_t* shining_light_free;
    buff_t* royal_decree;
    buff_t* bastion_of_light;
    buff_t* faith_in_the_light;
    buff_t* moment_of_glory_absorb;
    buff_t* blessing_of_spellwarding;
    buff_t* strength_in_adversity;

    buff_t* inner_light;
    buff_t* inspiring_vanguard;
    buff_t* barricade_of_faith;
    buff_t* ally_of_the_light;       // T29 2pc
    buff_t* deflecting_light;        // T29 4pc
    buff_t* sanctification;     // T31 2pc building
    buff_t* sanctification_empower;  // T31 2pc consecration effect
    buff_t* rising_wrath; // TWW1 4pc
    buff_t* heightened_wrath; // TWW1 4pc

    // Ret
    buffs::crusade_buff_t* crusade;
    buffs::shield_of_vengeance_buff_t* shield_of_vengeance;
    buff_t* empyrean_power;

    buff_t* rush_of_light;
    buff_t* inquisitors_ire;
    buff_t* inquisitors_ire_driver;
    buff_t* templar_strikes;

    buff_t* bulwark_of_righteous_fury;
    buff_t* blessing_of_dusk;
    buff_t* blessing_of_dawn;
    buff_t* final_verdict;
    buff_t* divine_resonance;

    buff_t* empyrean_legacy;
    buff_t* empyrean_legacy_cooldown;
    buff_t* relentless_inquisitor;
    buff_t* divine_arbiter;
    buff_t* judge_jury_and_executioner;
    buff_t* divine_hammer;

    buff_t* echoes_of_wrath;  // T31 4pc

    // TWW Hero Talents
    struct
    {
      buff_t* sacred_weapon;
      buff_t* holy_bulwark;
      buff_t* blessed_assurance;
      buff_t* divine_guidance;
      buff_t* rite_of_sanctification;
      buff_t* rite_of_adjuration;
      buff_t* blessing_of_the_forge;  // Sacred Weapon doodad, pseudo invisible buff
    } lightsmith;

    struct
    {
      buff_t* hammer_of_light_ready;
      buff_t* hammer_of_light_free;
      buff_t* for_whom_the_bell_tolls;
      buff_t* shake_the_heavens;
      buff_t* sacrosanct_crusade;
      buff_t* endless_wrath;
      buff_t* sanctification;
      buff_t* undisputed_ruling;
      buff_t* lights_deliverance;
    } templar;

    struct
    {
      buff_t* dawnlight;
      buff_t* blessing_of_anshe;
      buff_t* morning_star;
      buff_t* gleaming_rays;
      buff_t* solar_grace;
      buff_t* morning_star_driver;
      buff_t* suns_avatar;
    } herald_of_the_sun;

    buff_t* rise_from_ash; // Ret TWW1 4p
  } buffs;

  // Gains
  struct gains_t
  {
    // Healing/absorbs
    gain_t* holy_shield;
    gain_t* bulwark_of_order;
    gain_t* sacrosanct_crusade;
    gain_t* blessed_hammer;
    gain_t* moment_of_glory;

    // Mana
    gain_t* mana_beacon_of_light;

    // Holy Power
    gain_t* hp_templars_verdict_refund;
    gain_t* judgment;
    gain_t* hp_cs;
    gain_t* hp_divine_toll;
    gain_t* hp_vm;
    gain_t* hp_crusading_strikes;
    gain_t* hp_divine_auxiliary;
    gain_t* eye_of_tyr;
  } gains;

  // Spec Passives
  struct spec_t
  {
    const spell_data_t* judgment_3;
    const spell_data_t* judgment_4;
    const spell_data_t* consecration_2;
    const spell_data_t* consecration_3;
    const spell_data_t* shield_of_the_righteous;
    const spell_data_t* holy_paladin;
    const spell_data_t* protection_paladin;
    const spell_data_t* retribution_paladin;
    const spell_data_t* retribution_paladin_2;
    const spell_data_t* word_of_glory_2;
    const spell_data_t* holy_shock_2;
    const spell_data_t* improved_crusader_strike;
  } spec;

  // Cooldowns
  struct cooldowns_t
  {
    // Required to get various cooldown-reducing procs procs working
    cooldown_t* avenging_wrath;  // Righteous Protector (Prot)
    cooldown_t* sentinel;        // Righteous Protector (Prot)
    cooldown_t* hammer_of_justice;
    cooldown_t* judgment_of_light_icd;
    cooldown_t* blessing_of_protection;    // Blessing of Spellwarding Shared CD
    cooldown_t* blessing_of_spellwarding;  // Blessing of Protection Shared CD
    cooldown_t* divine_shield;             // Resolute Defender (Prot)
    cooldown_t* lay_on_hands;              // Tirion's Devotion

    cooldown_t* holy_shock;     // Crusader's Might, Divine Purpose
    cooldown_t* light_of_dawn;  // Divine Purpose

    cooldown_t* avengers_shield;  // Grand Crusader
    cooldown_t* consecration;     // Precombat shenanigans
    cooldown_t* inner_light_icd;
    cooldown_t* righteous_protector_icd;
    cooldown_t* judgment;                   // Crusader's Judgment
    cooldown_t* shield_of_the_righteous;    // Judgment
    cooldown_t* guardian_of_ancient_kings;  // Righteous Protector
    cooldown_t* ardent_defender;            // Resolute Defender

    cooldown_t* blade_of_justice;
    cooldown_t* final_reckoning;
    cooldown_t* hammer_of_wrath;
    cooldown_t* wake_of_ashes;

    cooldown_t* blessing_of_the_seasons;
    cooldown_t* holy_armaments;
    cooldown_t* ashen_hallow;  // Radiant Embers Legendary

    cooldown_t* ret_aura_icd;
    cooldown_t* consecrated_blade_icd;
    cooldown_t* searing_light_icd;
    cooldown_t* radiant_glory_icd;
    cooldown_t* righteous_cause_icd;

    cooldown_t* aurora_icd;
    cooldown_t* second_sunrise_icd;

    cooldown_t* eye_of_tyr;          // Light's Deliverance
    cooldown_t* endless_wrath_icd;   // Needed for many random hammer procs
    cooldown_t* hammerfall_icd;
    cooldown_t* art_of_war;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* paladin;
    const spell_data_t* plate_specialization;

    const spell_data_t* infusion_of_light;

    const spell_data_t* grand_crusader;
    const spell_data_t* riposte;
    const spell_data_t* sanctuary;

    const spell_data_t* aegis_of_light;
    const spell_data_t* aegis_of_light_2;

    const spell_data_t* boundless_conviction;

    const spell_data_t* art_of_war;
    const spell_data_t* art_of_war_2;
  } passives;

  struct mastery_t
  {
    const spell_data_t* divine_bulwark;    // Prot
    const spell_data_t* divine_bulwark_2;  // Rank 2 - consecration DR
    const spell_data_t* highlords_judgment; // Ret
    const spell_data_t* hand_of_light;     // Ret
    const spell_data_t* lightbringer;      // Holy
  } mastery;

  // Procs and RNG
  struct procs_t
  {
    proc_t* art_of_war;
    proc_t* righteous_cause;
    proc_t* divine_purpose;
    proc_t* final_reckoning;
    proc_t* empyrean_power;

    proc_t* as_grand_crusader;
    proc_t* as_grand_crusader_wasted;
    proc_t* as_engraved_sigil;
    proc_t* as_engraved_sigil_wasted;
    proc_t* as_moment_of_glory;
    proc_t* as_moment_of_glory_wasted;

    proc_t* divine_inspiration;
  } procs;

  // Spells
  struct spells_t
  {
    const spell_data_t* avenging_wrath;
    const spell_data_t* divine_purpose_buff;
    const spell_data_t* judgment_debuff;

    const spell_data_t* sotr_buff;

    const spell_data_t* sanctified_wrath_damage;

    const spell_data_t* judgment_2;
    const spell_data_t* improved_avenging_wrath;
    const spell_data_t* hammer_of_wrath_2;
    const spell_data_t* moment_of_glory;

    const spell_data_t* ashen_hallow_how;

    const spell_data_t* consecrated_blade;
    const spell_data_t* seraphim_buff;
    const spell_data_t* crusade;
    const spell_data_t* sentinel;

    struct
    {
      const spell_data_t* holy_bulwark;
      const spell_data_t* holy_bulwark_absorb;
      const spell_data_t* forges_reckoning; // Spell triggered by Blessing of the Forge (Shield of the Righteous)
      const spell_data_t* sacred_word;      // Spell triggered by Blessing of the Forge (Word of Glory)
    } lightsmith;

    struct
    {
      const spell_data_t* hammer_of_light;
      const spell_data_t* hammer_of_light_driver;
      const spell_data_t* empyrean_hammer;
      const spell_data_t* empyrean_hammer_wd; // Wrathful Descent triggered damage
    } templar;

    struct
    {
      const spell_data_t* gleaming_rays;
      const spell_data_t* dawnlight_aoe_metadata;
    } herald_of_the_sun;

    const spell_data_t* highlords_judgment_hidden;
  } spells;

  struct rppms_t {
    real_ppm_t* radiant_glory;
    real_ppm_t* judge_jury_and_executioner;
  } rppm;

  // Talents
  struct talents_t
  {
    // Duplicate names are commented out

    // Class

    // 0
    const spell_data_t* lay_on_hands;
    const spell_data_t* blessing_of_freedom;
    const spell_data_t* hammer_of_wrath;
    const spell_data_t* cleanse_toxins;
    const spell_data_t* auras_of_the_resolute;
    const spell_data_t* obduracy;
    const spell_data_t* auras_of_swift_vengeance;
    const spell_data_t* turn_evil;
    const spell_data_t* fist_of_justice;
    const spell_data_t* divine_steed;
    const spell_data_t* greater_judgment;
    const spell_data_t* repentance;
    const spell_data_t* blinding_light;
    const spell_data_t* cavalier;
    const spell_data_t* seasoned_warhorse;
    const spell_data_t* rebuke;

    // 8
    const spell_data_t* holy_aegis;
    const spell_data_t* avenging_wrath;  // Spell
    const spell_data_t* justification;
    const spell_data_t* punishment;
    const spell_data_t* golden_path;
    const spell_data_t* blessing_of_sacrifice;
    const spell_data_t* sanctified_plates;
    const spell_data_t* blessing_of_protection;
    const spell_data_t* unbound_freedom;
    const spell_data_t* lightforged_blessing;
    const spell_data_t* seal_of_mercy;
    const spell_data_t* afterimage;
    const spell_data_t* sacrifice_of_the_just;
    const spell_data_t* recompense;
    const spell_data_t* unbreakable_spirit;
    const spell_data_t* improved_blessing_of_protection;
    const spell_data_t* crusaders_reprieve;

    // 20
    const spell_data_t* strength_of_conviction;
    const spell_data_t* judgment_of_light;
    const spell_data_t* seal_of_might;
    const spell_data_t* divine_purpose;
    const spell_data_t* seal_of_alacrity;
    const spell_data_t* incandescence;
    const spell_data_t* touch_of_light;
    const spell_data_t* faiths_armor;
    const spell_data_t* of_dusk_and_dawn;
    const spell_data_t* divine_toll;
    const spell_data_t* seal_of_the_crusader;
    const spell_data_t* seal_of_order;
    const spell_data_t* fading_light;
    const spell_data_t* divine_resonance;
    const spell_data_t* quickened_invocation;
    const spell_data_t* zealots_paragon;  // Prot only
    const spell_data_t* vengeful_wrath;   // Ret only

    // Stuff that may come back
    const spell_data_t* seraphim;  // It'll come back :copium:
    const spell_data_t* holy_avenger;

    // Holy -- NYI, Not touching for now
    // T15
    const spell_data_t* crusaders_might;
    const spell_data_t* bestow_faith;
    const spell_data_t* lights_hammer;
    // T25
    const spell_data_t* saved_by_the_light;
    const spell_data_t* holy_prism;

    const spell_data_t* rule_of_law;
    const spell_data_t* avenging_crusader;
    const spell_data_t* awakening;
    // T50
    const spell_data_t* glimmer_of_light;
    const spell_data_t* beacon_of_faith;
    const spell_data_t* beacon_of_virtue;

    // Protection
    // 0
    const spell_data_t* avengers_shield;
    const spell_data_t* holy_shield;
    const spell_data_t* inner_light;
    const spell_data_t* redoubt;
    const spell_data_t* hammer_of_the_righteous;
    const spell_data_t* blessed_hammer;
    const spell_data_t* grand_crusader;
    const spell_data_t* shining_light;
    const spell_data_t* improved_holy_shield;
    const spell_data_t* sanctuary;
    const spell_data_t* inspiring_vanguard;
    const spell_data_t* ardent_defender;
    const spell_data_t* barricade_of_faith;
    const spell_data_t* consecration_in_flame;

    // 8
    const spell_data_t* consecrated_ground;
    const spell_data_t* tirions_devotion;
    const spell_data_t* bulwark_of_order;
    const spell_data_t* improved_ardent_defender;
    const spell_data_t* blessing_of_spellwarding;
    const spell_data_t* light_of_the_titans;
    const spell_data_t* strength_in_adversity;
    const spell_data_t* crusaders_resolve;
    const spell_data_t* tyrs_enforcer;
    const spell_data_t* relentless_inquisitor;
    const spell_data_t* avenging_wrath_might;
    const spell_data_t* sentinel;
    const spell_data_t* seal_of_charity;
    const spell_data_t* faith_in_the_light;
    const spell_data_t* seal_of_reprisal;
    const spell_data_t* bastion_of_light;
    const spell_data_t* guardian_of_ancient_kings;
    const spell_data_t* hand_of_the_protector;
    const spell_data_t* crusaders_judgment;

    // 20
    const spell_data_t* focused_enmity;
    const spell_data_t* soaring_shield;
    const spell_data_t* gift_of_the_golden_valkyr;
    const spell_data_t* uthers_counsel;
    const spell_data_t* sanctified_wrath;
    const spell_data_t* ferren_marcuss_fervor;
    const spell_data_t* eye_of_tyr;
    const spell_data_t* resolute_defender;
    const spell_data_t* moment_of_glory;
    const spell_data_t* bulwark_of_righteous_fury;
    const spell_data_t* inmost_light;
    const spell_data_t* final_stand;
    const spell_data_t* righteous_protector;

    // Retribution
    // 0
    const spell_data_t* blade_of_justice;
    const spell_data_t* divine_storm;

    const spell_data_t* swift_justice;
    const spell_data_t* light_of_justice;
    const spell_data_t* expurgation;
    const spell_data_t* judgment_of_justice;
    const spell_data_t* improved_blade_of_justice;
    const spell_data_t* holy_blade;

    const spell_data_t* final_verdict;
    const spell_data_t* justicars_vengeance;
    const spell_data_t* lights_celerity; // TODO: implement or drop
    const spell_data_t* guided_prayer; // TODO: implement or drop
    const spell_data_t* art_of_war;
    const spell_data_t* righteous_cause;

    const spell_data_t* jurisdiction;
    const spell_data_t* inquisitors_ire;
    const spell_data_t* tempest_of_the_lightbringer;
    const spell_data_t* crusade;
    // const spell_data_t* avenging_wrath_might; // available up in prot
    const spell_data_t* vanguards_momentum;
    const spell_data_t* sanctify;
    const spell_data_t* zealots_fervor;
    const spell_data_t* rush_of_light;
    const spell_data_t* consecrated_ground_ret; // TODO: implement or drop

    const spell_data_t* improved_judgment;
    const spell_data_t* boundless_judgment;
    const spell_data_t* crusading_strikes;
    const spell_data_t* templar_strikes;
    const spell_data_t* divine_wrath;
    const spell_data_t* divine_hammer;
    const spell_data_t* blade_of_vengeance;
    const spell_data_t* holy_flames;
    const spell_data_t* empyrean_legacy;

    const spell_data_t* vanguard_of_justice;
    const spell_data_t* heart_of_the_crusader;
    const spell_data_t* highlords_wrath;
    const spell_data_t* wake_of_ashes;
    const spell_data_t* blessed_champion;
    const spell_data_t* empyrean_power;
    const spell_data_t* judge_jury_and_executioner;

    const spell_data_t* adjudication;
    const spell_data_t* shield_of_vengeance;
    const spell_data_t* aegis_of_protection;
    const spell_data_t* penitence;

    const spell_data_t* blades_of_light;
    const spell_data_t* final_reckoning;
    const spell_data_t* execution_sentence;
    const spell_data_t* seething_flames;
    const spell_data_t* burning_crusade;

    const spell_data_t* divine_arbiter;
    const spell_data_t* executioners_will;
    const spell_data_t* divine_auxiliary;
    const spell_data_t* radiant_glory;
    const spell_data_t* burn_to_ash;
    const spell_data_t* searing_light;

    // Hero Talents
    struct
    {
      const spell_data_t* holy_armaments;

      const spell_data_t* rite_of_sanctification;
      const spell_data_t* rite_of_adjuration;
      const spell_data_t* solidarity;
      const spell_data_t* divine_guidance;
      const spell_data_t* blessed_assurance;

      const spell_data_t* laying_down_arms;
      const spell_data_t* divine_inspiration;
      const spell_data_t* forewarning;
      const spell_data_t* fear_no_evil;
      const spell_data_t* excoriation;

      const spell_data_t* shared_resolve;
      const spell_data_t* valiance;
      const spell_data_t* hammer_and_anvil;

      const spell_data_t* blessing_of_the_forge;
    } lightsmith;

    struct
    {
      const spell_data_t* lights_guidance;

      const spell_data_t* zealous_vindication;
      const spell_data_t* for_whom_the_bell_tolls;
      const spell_data_t* shake_the_heavens;
      const spell_data_t* wrathful_descent;

      const spell_data_t* sacrosanct_crusade;
      const spell_data_t* higher_calling;
      const spell_data_t* bonds_of_fellowship;
      const spell_data_t* unrelenting_charger;

      const spell_data_t* endless_wrath;
      const spell_data_t* sanctification;
      const spell_data_t* hammerfall;
      const spell_data_t* undisputed_ruling;

      const spell_data_t* lights_deliverance;
    } templar;

    struct {
      const spell_data_t* dawnlight;

      const spell_data_t* morning_star;
      const spell_data_t* gleaming_rays;
      const spell_data_t* eternal_flame;
      const spell_data_t* luminosity;

      const spell_data_t* illumine;
      const spell_data_t* will_of_the_dawn;
      const spell_data_t* blessing_of_anshe;
      const spell_data_t* lingering_radiance;
      const spell_data_t* sun_sear;

      const spell_data_t* aurora;
      const spell_data_t* solar_grace;
      const spell_data_t* second_sunrise;

      const spell_data_t* suns_avatar;
    } herald_of_the_sun;

  } talents;

  // Paladin options
  struct options_t
  {
    bool fake_sov                         = true;
    double proc_chance_ret_aura_sera      = 0.10;
    int min_dg_heal_targets               = 1;
    int max_dg_heal_targets               = 5;
    bool sacred_weapon_prefer_new_targets = false;
    bool fake_solidarity                  = false;
  } options;
  player_t* beacon_target;

  season next_season;
  armament next_armament;
  double radiant_glory_accumulator;

  bool lights_deliverance_triggered_during_ready;

  int holy_power_generators_used;
  int melee_swing_count;

  // Helper variables to not always RNG the correct target
  player_t* random_weapon_target;
  player_t* random_bulwark_target;
  int divine_inspiration_next;

  paladin_t( sim_t* sim, util::string_view name, race_e r = RACE_TAUREN );

  virtual void init_assessors() override;
  virtual void init_base_stats() override;
  virtual void init_gains() override;
  virtual void init_procs() override;
  virtual void init() override;
  virtual void init_scaling() override;
  virtual void create_buffs() override;
  virtual void init_special_effects() override;
  virtual void init_rng() override;
  virtual void init_spells() override;
  virtual void init_action_list() override;
  virtual bool validate_fight_style( fight_style_e style ) const override;
  virtual void reset() override;
  virtual std::unique_ptr<expr_t> create_expression( util::string_view name ) override;

  // player stat functions
  virtual double composite_player_multiplier( school_e ) const override;
  virtual double composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double composite_attack_power_multiplier() const override;
  virtual double composite_bonus_armor() const override;
  virtual double composite_melee_crit_chance() const override;
  virtual double composite_spell_crit_chance() const override;
  virtual double composite_damage_versatility() const override;
  virtual double composite_heal_versatility() const override;
  virtual double composite_mitigation_versatility() const override;
  virtual double composite_mastery() const override;
  virtual double composite_melee_haste() const override;
  virtual double composite_melee_auto_attack_speed() const override;
  virtual double composite_spell_haste() const override;
  virtual double composite_crit_avoidance() const override;
  virtual double composite_parry() const override;
  virtual double composite_parry_rating() const override;
  virtual double composite_block() const override;
  virtual double non_stacking_movement_modifier() const override;
  virtual double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  virtual double composite_base_armor_multiplier() const override;

  virtual double resource_gain( resource_e resource_type, double amount, gain_t* source = nullptr,
                                action_t* action = nullptr ) override;
  virtual double resource_loss( resource_e resource_type, double amount, gain_t* source = nullptr,
                                action_t* action = nullptr ) override;

  // combat outcome functions
  virtual void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  virtual void target_mitigation( school_e, result_amount_type, action_state_t* ) override;

  virtual void invalidate_cache( cache_e ) override;
  virtual void create_options() override;
  virtual double matching_gear_multiplier( attribute_e attr ) const override;
  virtual void create_actions() override;
  virtual action_t* create_action( util::string_view name, util::string_view options_str ) override;
  virtual resource_e primary_resource() const override;
  virtual role_e primary_role() const override;
  virtual stat_e convert_hybrid_stat( stat_e s ) const override;
  virtual void combat_begin() override;
  virtual void copy_from( player_t* ) override;

  void trigger_grand_crusader( grand_crusader_source source = GC_NORMAL );
  void trigger_holy_shield( action_state_t* s );
  void trigger_laying_down_arms();
  void trigger_empyrean_hammer( player_t* target, int number_to_trigger, timespan_t delay, bool random_after_first = false );
  void trigger_lights_deliverance(bool triggered_by_hol = false);
  void tww1_4p_prot();
  void heartfire( action_state_t* s );
  void t29_4p_prot();
  void t31_4p_prot( action_state_t* s );
  void t31_4p_prot_heal( action_state_t* s );
  void trigger_forbearance( player_t* target );
  void trigger_es_explosion( player_t* target );
  int get_local_enemies( double distance ) const;
  bool standing_in_consecration() const;
  bool standing_in_hallow() const;
  void adjust_health_percent();
  void cast_holy_armaments( player_t* target, armament usedArmament, bool changeArmament, bool random );

  // Returns true if AW/Crusade is up, or if the target is below 20% HP.
  // This isn't in HoW's target_ready() so it can be used in the time_to_hpg expression
  bool get_how_availability( player_t* t ) const;

  std::unique_ptr<expr_t> create_consecration_expression( util::string_view expr_str );
  std::unique_ptr<expr_t> create_ashen_hallow_expression( util::string_view expr_str );
  std::unique_ptr<expr_t> create_aw_expression( util::string_view expr_str );

  ground_aoe_event_t* active_consecration;
  ground_aoe_event_t* active_boj_cons;
  ground_aoe_event_t* active_searing_light_cons;
  std::set<ground_aoe_event_t*> all_active_consecrations;
  ground_aoe_event_t* active_hallow_damaging;
  ground_aoe_event_t* active_hallow_healing;
  buff_t* active_aura;

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  void apply_affecting_auras( action_t& action ) override;

  void create_buffs_retribution();
  void init_rng_retribution();
  void init_spells_retribution();
  void generate_action_prio_list_ret();
  void create_ret_actions();
  action_t* create_action_retribution( util::string_view name, util::string_view options_str );

  void create_buffs_protection();
  void init_spells_protection();
  void create_prot_actions();
  action_t* create_action_protection( util::string_view name, util::string_view options_str );

  void create_buffs_holy();
  void init_spells_holy();
  void create_holy_actions();
  action_t* create_action_holy( util::string_view name, util::string_view options_str );

  void generate_action_prio_list_prot();
  void generate_action_prio_list_holy();
  void generate_action_prio_list_holy_dps();

  void apply_avatar_dawnlights();
  void spread_expurgation( action_t* act, player_t* og );

  target_specific_t<paladin_td_t> target_data;

  virtual const paladin_td_t* find_target_data( const player_t* target ) const override;
  virtual paladin_td_t* get_target_data( player_t* target ) const override;

  bool may_benefit_from_skyfury() const override
  {
    return !( talents.crusading_strikes->ok() );
  }
  dbc_proc_callback_t* create_sacred_weapon_callback(paladin_t* source, player_t* target);
};

namespace buffs
{
struct avenging_wrath_buff_t : public buff_t
{
  avenging_wrath_buff_t( paladin_t* p );
  double get_damage_mod() const
  {
    return damage_modifier;
  }
  double get_healing_mod() const
  {
    return healing_modifier;
  }
  double get_crit_bonus() const
  {
    return crit_bonus;
  }

private:
  double damage_modifier;
  double healing_modifier;
  double crit_bonus;
};

struct crusade_buff_t : public buff_t
{
  crusade_buff_t( paladin_t* p );

  double get_damage_mod() const
  {
    return damage_modifier * ( this->check() );
  }

  double get_haste_bonus() const
  {
    return haste_bonus * ( this->check() );
  }

private:
  double damage_modifier;
  double haste_bonus;
};

struct execution_sentence_debuff_t : public buff_t
{
  execution_sentence_debuff_t( paladin_td_t* td )
    : buff_t( *td, "execution_sentence", debug_cast<paladin_t*>( td->source )->talents.execution_sentence ),
      accumulated_damage( 0.0 ),
      accum_percent( 0.0 ),
      extended_count( 0 )
  {
    set_cooldown( 0_ms );  // handled by the ability
    accum_percent = data().effectN( 2 ).percent();

    paladin_t* p = debug_cast<paladin_t*>( td->source );

    if ( p->talents.executioners_will->ok() )
    {
      modify_duration( timespan_t::from_millis( p->talents.executioners_will->effectN( 1 ).base_value() ) );
    }
  }

  void reset() override
  {
    buff_t::reset();
    accumulated_damage = 0.0;
    extended_count     = 0;
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    paladin_t* paladin = debug_cast<paladin_t*>( source );
    paladin->trigger_es_explosion( player );

    accumulated_damage = 0.0;
    extended_count     = 0;
  }

  void accumulate_damage( const action_state_t* s, double mult )
  {
    double amount_accumulated = s->result_total * mult;

    sim->print_debug( "{}'s {} accumulates {} additional damage: {} -> {}", player->name(), name(), amount_accumulated,
                      accumulated_damage, accumulated_damage + s->result_total );

    accumulated_damage += amount_accumulated;
  }

  double get_accumulated_damage() const
  {
    return accumulated_damage * accum_percent;
  }

private:
  double accumulated_damage;
  double accum_percent;
  int extended_count;
};

struct forbearance_t : public buff_t
{
  paladin_t* paladin;

  forbearance_t( player_t* p, const char* name ) : buff_t( p, name, p->find_spell( 25771 ) ), paladin( nullptr )
  {
  }

  forbearance_t( paladin_td_t* ap, const char* name )
    : buff_t( *ap, name, ap->source->find_spell( 25771 ) ), paladin( debug_cast<paladin_t*>( ap->source ) )
  {
  }
};

struct holy_bulwark_absorb_t : public absorb_buff_t
{
  paladin_t* caster;
  holy_bulwark_absorb_t( paladin_td_t* td )
    : absorb_buff_t( td->target, "holy_bulwark_absorb_" + td->source->name_str + "_" + td->target->name_str,
                     debug_cast<paladin_t*>( td->source )->spells.lightsmith.holy_bulwark_absorb )
  {
    caster                   = debug_cast<paladin_t*>( td->source );
    set_absorb_source( caster->get_stats( "holy_bulwark_absorb_"+td->target->name_str) );
  }
  holy_bulwark_absorb_t( paladin_t* p )
    : absorb_buff_t( p, "holy_bulwark_absorb", p->spells.lightsmith.holy_bulwark_absorb )
  {
    caster = p;
    set_absorb_source( caster->get_stats( "holy_bulwark_absorb" ) );
  }
  bool trigger(int stacks, double value, double chance, timespan_t duration) override
  {
    double total_value = this->value();
    if (value > 0)
    {
      total_value += value;
    }
    else
    {
      total_value += this->player->resources.max[ RESOURCE_HEALTH ] *
                     ( caster->spells.lightsmith.holy_bulwark->effectN( 4 ).percent() / 10.0 );
    }
    total_value = std::min( total_value, this->player->resources.max[ RESOURCE_HEALTH ] * caster->spells.lightsmith.holy_bulwark->effectN( 5 ).percent());
    return absorb_buff_t::trigger( stacks, total_value, chance, duration );
  }
};

struct holy_bulwark_buff_t : public buff_t
{
  holy_bulwark_absorb_t* absorb;
  paladin_t* caster;
  player_t* buff_owner;
  holy_bulwark_buff_t( paladin_td_t* td )
    : buff_t( *td, "holy_bulwark_ally_"+td->target->name_str, debug_cast<paladin_t*>( td->source )->spells.lightsmith.holy_bulwark),
      absorb( new holy_bulwark_absorb_t(td) ),
      caster( debug_cast<paladin_t*>( td->source ) ),
      buff_owner( td->target )
  {
    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { absorb->trigger( -1, 0, -1, timespan_t::min() ); } );
  }
  holy_bulwark_buff_t( paladin_t* p )
    : buff_t( p, "holy_bulwark", p->spells.lightsmith.holy_bulwark ),
      absorb( new holy_bulwark_absorb_t(p) ),
      caster( p ),
      buff_owner( p )
  {
    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { absorb->trigger( -1, 0, -1, timespan_t::min() ); } );
  }
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool result = buff_t::trigger( stacks, value, chance, duration );
    // Initial absorb scales with target's max HP, but with caster's stats
    double initial_absorb = buff_owner->resources.max[ RESOURCE_HEALTH ] * (caster->spells.lightsmith.holy_bulwark->effectN(2).percent()/10) * (1.0 + caster->composite_heal_versatility());
    absorb->trigger( -1, initial_absorb, -1, timespan_t::min() );
    return result;
  }
};

struct sentinel_buff_t : public buff_t
{
  sentinel_buff_t( paladin_t* p );

  double get_damage_mod() const
  {
    return damage_modifier;
  }
  double get_damage_reduction_mod() const
  {
    return damage_reduction_modifier * ( this->check() );
  }

  double get_health_bonus() const
  {
    return health_bonus * ( this->check() );
  }
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;

private:
  double damage_modifier;
  double damage_reduction_modifier;
  double health_bonus;
};

struct sentinel_decay_buff_t : public buff_t
{
  sentinel_decay_buff_t( paladin_t* p );
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

}  // namespace buffs

// ==========================================================================
// Paladin Ability Templates
// ==========================================================================

// Template for common paladin action code. See priest_action_t.
template <class Base>
struct paladin_action_t : public Base
{
private:
  using ab = Base;  // action base, eg. spell_t
public:
  using base_t = paladin_action_t;

  // Damage increase whitelists
  struct affected_by_t
  {
    bool avenging_wrath, judgment, blessing_of_dawn, seal_of_reprisal, seal_of_order, divine_purpose,
      divine_purpose_cost;                                                               // Shared
    bool crusade, highlords_judgment, highlords_judgment_hidden, final_reckoning_st, final_reckoning_aoe,
      blades_of_light, divine_hammer, ret_t29_2p, ret_t29_4p, rise_from_ash; // Ret
    bool avenging_crusader;                                                                // Holy
    bool bastion_of_light, sentinel, heightened_wrath;                                     // Prot
    bool gleaming_rays; // Herald of the Sun
  } affected_by;

  // haste scaling bools
  bool hasted_cd;
  bool hasted_gcd;

  bool searing_light_disabled;
  bool always_do_capstones;
  bool clears_judgment;

  bool triggers_higher_calling;
  bool skip_es_accum;

  paladin_action_t( util::string_view n, paladin_t* p, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, p, s ),
      affected_by( affected_by_t() ),
      hasted_cd( false ),
      hasted_gcd( false ),
      searing_light_disabled( false ),
      always_do_capstones(false),
      clears_judgment( false ),
      triggers_higher_calling( false ),
      skip_es_accum( false )
  {
    ab::track_cd_waste = s->cooldown() > 0_ms || s->charge_cooldown() > 0_ms;

    // Spec aura damage increase
    if ( p->specialization() == PALADIN_RETRIBUTION )
    {
      // Mastery
      this->affected_by.highlords_judgment = this->data().affected_by( p->mastery.highlords_judgment->effectN( 1 ) );
      this->affected_by.highlords_judgment_hidden = this->data().affected_by( p->spells.highlords_judgment_hidden->effectN( 1 ) ) ||
                                                    this->data().affected_by( p->spells.highlords_judgment_hidden->effectN( 3 ) );

      // Temporary damage modifiers
      this->affected_by.crusade         = this->data().affected_by( p->spells.crusade->effectN( 1 ) );
      this->affected_by.final_reckoning_st = this->data().affected_by( p->talents.final_reckoning->effectN( 3 ) );
      this->affected_by.final_reckoning_aoe = this->data().affected_by( p->talents.final_reckoning->effectN( 4 ) );
      this->affected_by.ret_t29_2p =
          this->data().affected_by( p->sets->set( PALADIN_RETRIBUTION, T29, B2 )->effectN( 1 ) );
      this->affected_by.ret_t29_4p =
          this->data().affected_by( p->sets->set( PALADIN_RETRIBUTION, T29, B4 )->effectN( 1 ) );
      this->affected_by.rise_from_ash =
          this->data().affected_by( p->find_spell( 454693 )->effectN( 1 ) );
      if ( p->talents.divine_hammer->ok() )
      {
        for ( auto i = 2; i < 5; i++ )
        {
          auto label = p->talents.divine_hammer->effectN( i );
          if ( this->data().affected_by( label ) || this->data().affected_by_category( label ) )
          {
            this->affected_by.divine_hammer = true;
            break;
          }
        }
      }
    }
    if ( p->specialization() == PALADIN_HOLY )
    {
      this->affected_by.avenging_crusader = this->data().affected_by( p->talents.avenging_crusader->effectN( 1 ) );
    }
    if ( p->specialization() == PALADIN_PROTECTION )
    {
      this->affected_by.bastion_of_light = this->data().affected_by( p->talents.bastion_of_light->effectN( 1 ) );
      this->affected_by.sentinel         = this->data().affected_by( p->talents.sentinel->effectN( 1 ) );
      this->affected_by.heightened_wrath = this->data().affected_by( p->buffs.heightened_wrath->data().effectN( 1 ) );
    }

    this->affected_by.judgment            = this->data().affected_by( p->spells.judgment_debuff->effectN( 1 ) );
    this->clears_judgment                 = this->affected_by.judgment;
    this->affected_by.avenging_wrath      = this->data().affected_by( p->spells.avenging_wrath->effectN( 2 ) );
    this->affected_by.divine_purpose_cost = this->data().affected_by( p->spells.divine_purpose_buff->effectN( 1 ) );
    this->affected_by.divine_purpose      = this->data().affected_by( p->spells.divine_purpose_buff->effectN( 2 ) );
    this->affected_by.seal_of_reprisal    = this->data().affected_by( p->talents.seal_of_reprisal->effectN( 1 ) );
    this->affected_by.blessing_of_dawn    = this->data().affected_by( p->find_spell( 385127 )->effectN( 1 ) );

    if ( p->talents.penitence->ok() )
    {
      if ( this->data().affected_by_label( p->talents.penitence->effectN( 1 ).misc_value2() ) )
      {
        ab::base_multiplier *= 1.0 + p->talents.penitence->effectN( 1 ).percent();
      }
      else if ( this->data().affected_by_label( p->talents.penitence->effectN( 2 ).misc_value2() ) )
      {
        ab::base_multiplier *= 1.0 + p->talents.penitence->effectN( 2 ).percent();
      }
    }

    if ( p->talents.adjudication->ok() && this->data().affected_by( p->talents.adjudication->effectN( 1 ) ) )
    {
      ab::crit_bonus_multiplier *= 1.0 + p->talents.adjudication->effectN( 1 ).percent();
    }

    if ( p->talents.blades_of_light->ok() && this->data().affected_by( p->talents.blades_of_light->effectN( 1 ) ) )
    {
      this->affected_by.blades_of_light = true;
      ab::school = SCHOOL_HOLYSTRIKE;
      ab::base_multiplier *= 1.0 + p->talents.blades_of_light->effectN( 2 ).percent();
    }
    else
    {
      this->affected_by.blades_of_light = false;
    }

    if ( p->talents.burning_crusade->ok() && this->data().affected_by( p->talents.burning_crusade->effectN( 1 ) ) )
    {
      ab::school = SCHOOL_RADIANT;
    }

    if ( p->talents.herald_of_the_sun.gleaming_rays->ok() )
    {
      this->affected_by.gleaming_rays = this->data().affected_by( p->spells.herald_of_the_sun.gleaming_rays->effectN( 1 ) );
    }
    else
    {
      this->affected_by.gleaming_rays = false;
    }
  }

  paladin_t* p()
  {
    return static_cast<paladin_t*>( ab::player );
  }
  const paladin_t* p() const
  {
    return static_cast<paladin_t*>( ab::player );
  }

  paladin_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  void init() override
  {
    ab::init();

    if ( hasted_cd )
    {
      ab::cooldown->hasted = hasted_cd;
    }
    if ( hasted_gcd )
    {
      if ( p()->specialization() == PALADIN_HOLY )
      {
        ab::gcd_type = gcd_haste_type::SPELL_HASTE;
      }
      else
      {
        ab::gcd_type = gcd_haste_type::ATTACK_HASTE;
      }
    }
  }

  void trigger_judgment_of_light( action_state_t* s )
  {
    // Don't activate if the player is at full HP
    if ( p()->resources.current[ RESOURCE_HEALTH ] < p()->resources.max[ RESOURCE_HEALTH ] )
    {
      p()->active.judgment_of_light->execute();
      td( s->target )->debuff.judgment_of_light->decrement();
    }
  }

  void execute() override
  {
    ab::execute();

    if ( ( this->affected_by.blades_of_light || always_do_capstones ) && p()->talents.divine_arbiter->ok() )
    {
      p()->buffs.divine_arbiter->trigger( 1 );
    }

    if ( ( ab::get_school() == SCHOOL_RADIANT || always_do_capstones ) && !searing_light_disabled && p()->talents.searing_light->ok() )
    {
      if ( ab::rng().roll( p()->talents.searing_light->proc_chance() ) && p()->cooldowns.searing_light_icd->up() )
      {
        p()->cooldowns.searing_light_icd->start();
        p()->active.searing_light->set_target( ab::target );
        p()->active.searing_light->schedule_execute();
      }
    }
    if ( triggers_higher_calling && p()->talents.templar.higher_calling->ok() && p()->buffs.templar.shake_the_heavens->up() )
    {
      timespan_t extension = timespan_t::from_seconds( p()->talents.templar.higher_calling->effectN( 1 ).base_value() );
      // If Crusading Strikes is triggering, extension is only 500ms
      if ( ab::id == 408385 )
        extension = 500_ms;
      p()->buffs.templar.shake_the_heavens->extend_duration( p(), extension );
    }

    if ( affected_by.divine_hammer && p()->buffs.divine_hammer->up() )
    {
      p()->buffs.divine_hammer->current_value = p()->buffs.divine_hammer->current_value * 1.15;
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( td( s->target )->debuff.judgment_of_light->up() && p()->cooldowns.judgment_of_light_icd->up() )
    {
      trigger_judgment_of_light( s );
      p()->cooldowns.judgment_of_light_icd->start();
    }

    if ( ab::result_is_hit( s->result ) )
    {
      if ( affected_by.judgment && clears_judgment )
      {
        paladin_td_t* td = this->td( s->target );
        if ( td->debuff.judgment->up() )
          td->debuff.judgment->decrement();
      }
    }
  }

  double action_multiplier() const override
  {
    double am = ab::action_multiplier();

    if ( affected_by.gleaming_rays && p()->buffs.herald_of_the_sun.gleaming_rays->up() )
    {
      am *= 1.0 + p()->spells.herald_of_the_sun.gleaming_rays->effectN( 1 ).percent();
    }

    if ( p()->specialization() == PALADIN_RETRIBUTION )
    {
      if ( affected_by.highlords_judgment )
      {
        double mastery_amount = p()->cache.mastery_value();
        if ( affected_by.highlords_judgment_hidden && p()->talents.highlords_wrath->ok() )
        {
          // TODO: this has gotta be wrong. Where's the actual spell data for this?
          mastery_amount *= 1.0 + (p()->talents.highlords_wrath->effectN( 3 ).percent() / p()->talents.highlords_wrath->effectN( 2 ).base_value());
        }
        am *= 1.0 + mastery_amount;
      }

      if ( affected_by.crusade && p()->buffs.crusade->up() )
      {
        am *= 1.0 + p()->buffs.crusade->get_damage_mod();
      }
    }

    // Class talent's Avenging Wrath damage multiplier affects only if base talent is talented (Could still use AW with
    // only Sentinel/AWM/Crusade/AC talented)
    if ( affected_by.avenging_wrath && p()->talents.avenging_wrath->ok() &&
         ( p()->buffs.avenging_wrath->up() || p()->buffs.sentinel->up() ) )
    {
      am *= 1.0 + p()->buffs.avenging_wrath->get_damage_mod();
    }

    if ( affected_by.rise_from_ash && p()->buffs.rise_from_ash->up() )
    {
      am *= 1.0 + p()->buffs.rise_from_ash->data().effectN( 1 ).percent();
    }

    // TWW1 Prot 4pc
    if ( affected_by.heightened_wrath && p()->buffs.heightened_wrath->up() )
    {
      am *= 1.0 + p()->buffs.heightened_wrath->value();
    }

    if ( affected_by.avenging_crusader )
    {
      am *= 1.0 + p()->buffs.avenging_crusader->check_value();
    }

    // Divine purpose damage increase handled here,
    // Cost handled in holy_power_consumer_t
    if ( affected_by.divine_purpose && p()->buffs.divine_purpose->up() )
    {
      am *= 1.0 + p()->spells.divine_purpose_buff->effectN( 2 ).percent();
    }

    if ( affected_by.seal_of_reprisal && p()->talents.seal_of_reprisal->ok() )
    {
      am *= 1.0 + p()->talents.seal_of_reprisal->effectN( 1 ).percent();
    }

    if ( affected_by.blessing_of_dawn && p()->buffs.blessing_of_dawn->up() )
    {
      // Buffs handled in holy_power_consumer_t
      // Get base multiplier
      double bod_mult = p()->buffs.blessing_of_dawn->value();
      // Increase base multiplier by SoO/FL increase
      if ( p()->talents.seal_of_order->ok() )
        bod_mult += p()->talents.seal_of_order->effectN( 1 ).percent();
      // Separating those, in case on of them gets changed. Both do exactly the same to Dawn, though.
      if ( p()->talents.fading_light->ok() )
        bod_mult += p()->talents.fading_light->effectN( 1 ).percent();
      // Multiply by stack count
      bod_mult *= p()->buffs.blessing_of_dawn->stack();
      am *= 1.0 + bod_mult;
    }

    return am;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = ab::composite_target_multiplier( t );

    paladin_td_t* td = this->td( t );

    // Handles both holy and ret judgment
    if ( affected_by.judgment && td->debuff.judgment->up() )
    {
      double judg_mul = 1.0 + td->debuff.judgment->default_value;
      ctm *= judg_mul;
    }

    if ( affected_by.final_reckoning_st && td->debuff.final_reckoning->up() )
    {
      ctm *= 1.0 + p()->talents.final_reckoning->effectN( 3 ).percent();
    }

    if ( affected_by.final_reckoning_aoe && td->debuff.final_reckoning->up() )
    {
      ctm *= 1.0 + p()->talents.final_reckoning->effectN( 4 ).percent();
    }

    return ctm;
  }

  virtual double composite_target_ta_multiplier( player_t* target ) const override
  {
    double cttm = ab::composite_target_ta_multiplier( target );

    paladin_td_t* td = this->td( target );
    if ( p()->talents.burn_to_ash->ok() && td->dots.truths_wake->is_ticking() && ab::id != 403695 )
      cttm *= 1.0 + p()->talents.burn_to_ash->effectN( 2 ).percent();

    return cttm;
  }

  virtual void assess_damage( result_amount_type typ, action_state_t* s ) override
  {
    ab::assess_damage( typ, s );

    paladin_td_t* td = this->td( s->target );

    if ( td->debuff.execution_sentence->check() && !skip_es_accum )
    {
      double mult = 1.0;

      // ES counts damage before wings & mastery, but after most other multipliers,
      // per bolas test Aug 17 2024
      if ( affected_by.avenging_wrath && p()->buffs.avenging_wrath->up() )
      {
        mult /= 1.0 + p()->buffs.avenging_wrath->get_damage_mod();
      }

      if ( affected_by.crusade && p()->buffs.crusade->up() )
      {
        mult /= 1.0 + p()->buffs.crusade->get_damage_mod();
      }

      if ( affected_by.highlords_judgment )
      {
        double mastery_amount = p()->cache.mastery_value();
        if ( affected_by.highlords_judgment_hidden && p()->talents.highlords_wrath->ok() )
        {
          // TODO: this has gotta be wrong. Where's the actual spell data for this?
          mastery_amount *= 1.0 + (p()->talents.highlords_wrath->effectN( 3 ).percent() / p()->talents.highlords_wrath->effectN( 2 ).base_value());
        }
        mult /= 1.0 + mastery_amount;
      }

      td->debuff.execution_sentence->accumulate_damage( s, mult );
    }

    if ( p()->buffs.moment_of_glory->up() )
    {
      double amount = s->result_amount * p()->talents.moment_of_glory->effectN( 3 ).percent();
      p()->buffs.moment_of_glory_absorb->trigger( 1, p()->buffs.moment_of_glory_absorb->value() + amount );
    }
  }
};

// paladin "Spell" Base for paladin_spell_t, paladin_heal_t and paladin_absorb_t

template <class Base>
struct paladin_spell_base_t : public paladin_action_t<Base>
{
private:
  typedef paladin_action_t<Base> ab;

public:
  typedef paladin_spell_base_t base_t;

  paladin_spell_base_t( util::string_view n, paladin_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s )
  {
  }
};

template <typename Data, typename Base = action_state_t>
struct paladin_action_state_t : public Base, public Data
{
  static_assert( std::is_base_of_v<action_state_t, Base> );
  static_assert( std::is_default_constructible_v<Data> );  // required for initialize
  static_assert( std::is_copy_assignable_v<Data> );        // required for copy_state

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
    *static_cast<Data*>( this ) = *static_cast<const Data*>( static_cast<const paladin_action_state_t*>( o ) );
  }
};

// ==========================================================================
// The damage formula in action_t::calculate_direct_amount in sc_action.cpp is documented here:
// https://github.com/simulationcraft/simc/wiki/DevelopersDocumentation#damage-calculations
// ==========================================================================

// ==========================================================================
// Paladin Spells, Heals, and Absorbs
// ==========================================================================

// paladin-specific spell_t, heal_t, and absorb_t classes for inheritance ===

struct paladin_spell_t : public paladin_spell_base_t<spell_t>
{
  paladin_spell_t( util::string_view n, paladin_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
  }
};

struct paladin_heal_t : public paladin_spell_base_t<heal_t>
{
  paladin_heal_t( util::string_view n, paladin_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
    may_crit      = true;
    tick_may_crit = true;
    harmful       = false;
    // WARNING: When harmful = false, if you try to cast at time=0
    // then the ability has no cost and no gcd, so it just spams it indefinitely

    weapon_multiplier = 0.0;
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( s->target != p()->beacon_target )
      trigger_beacon_of_light( s );
  }

  void trigger_beacon_of_light( action_state_t* s )
  {
    if ( !p()->beacon_target )
      return;

    if ( !p()->beacon_target->buffs.beacon_of_light->up() )
      return;

    if ( proc )
      return;

    assert( p()->active.beacon_of_light );

    p()->active.beacon_of_light->target = p()->beacon_target;

    double amount = s->result_amount;
    amount *= p()->beacon_target->buffs.beacon_of_light->data().effectN( 1 ).percent();

    p()->active.beacon_of_light->base_dd_min = amount;
    p()->active.beacon_of_light->base_dd_max = amount;

    p()->active.beacon_of_light->execute();
  }
};

struct paladin_absorb_t : public paladin_spell_base_t<absorb_t>
{
  paladin_absorb_t( util::string_view n, paladin_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
  }
};

struct paladin_melee_attack_t : public paladin_action_t<melee_attack_t>
{
  paladin_melee_attack_t( util::string_view n, paladin_t* p, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, p, s )
  {
    may_crit = true;
    special  = true;
    weapon   = &( p->main_hand_weapon );
  }
};

// holy power consumption

struct sanctified_wrath_t : public paladin_spell_t
{
  int last_holy_power_cost;

  sanctified_wrath_t( paladin_t* p )
    : paladin_spell_t( "sanctified_wrath", p, p->find_spell( 326731 ) ), last_holy_power_cost( 0 )
  {
    aoe        = -1;
    background = may_crit = true;
  }

  double action_multiplier() const override
  {
    return paladin_spell_t::action_multiplier() * last_holy_power_cost;
  }
};

template <class Base>
struct holy_power_consumer_t : public Base
{
private:
  using ab = Base;  // action base, eg. spell_t
public:
  using base_t = holy_power_consumer_t;
  bool is_divine_storm;
  bool is_wog;
  bool is_sotr;
  bool doesnt_consume_dp;
  bool is_hammer_of_light;
  bool is_hammer_of_light_driver;
  double hol_cost;
  holy_power_consumer_t( util::string_view n, paladin_t* player, const spell_data_t* s )
    : ab( n, player, s ),
      is_divine_storm( false ),
      is_wog( false ),
      is_sotr( false ),
      doesnt_consume_dp( false ),
      is_hammer_of_light( false ),
      is_hammer_of_light_driver( false ),
      hol_cost( 3.0 )
  {
  }

  double cost() const override
  {
    // paladin_t* p = paladin_action_t<Base>::template p();

    if ( ab::background )
    {
      return 0.0;
    }

    if ( ( is_divine_storm && ( ab::p()->buffs.empyrean_power->check() ) ) ||
         ( ab::affected_by.divine_purpose_cost && ab::p()->buffs.divine_purpose->check() ) )
    {
      return 0.0;
    }

    return ab::cost();
  }

  void impact( action_state_t* s ) override
  {
    paladin_t* p = ab::p();
    ab::impact( s );

    if ( ab::aoe == 0 && p->talents.rush_of_light->ok() && s->result == RESULT_CRIT )
    {
      p->buffs.rush_of_light->trigger();
    }

    if ( !is_hammer_of_light && p->talents.templar.hammerfall->ok() && p->cooldowns.hammerfall_icd->up() )
    {
      int additionalTargets = 0;
      if ( p->buffs.templar.shake_the_heavens->up() )
        additionalTargets += as<int>( p->talents.templar.hammerfall->effectN( 2 ).base_value() );
      p->trigger_empyrean_hammer( nullptr, 1 + additionalTargets,
                                  timespan_t::from_millis( p->talents.templar.hammerfall->effectN( 1 ).base_value() ),
                                  true );
      p->cooldowns.hammerfall_icd->start();
    }

    if ( !is_divine_storm && p->talents.vanguard_of_justice->ok() )
    {
      paladin_td_t* td = p->get_target_data( s->target );
      td->debuff.vanguard_of_justice->trigger();
    }

    if ( ab::result_is_hit( s->result ) &&  p->buffs.herald_of_the_sun.dawnlight->up() )
    {
      p->active.dawnlight->execute_on_target( s->target );
      p->buffs.herald_of_the_sun.dawnlight->decrement();
    }
  }

  void execute() override
  {
    // p variable just to make this look neater
    paladin_t* p = ab::p();

    ab::execute();

    // if this is a vanq-hammer-based DS, don't do this stuff
    if ( ab::background && is_divine_storm )
      return;

    bool isFreeSLDPSpender = p->buffs.divine_purpose->up() || ( is_wog && p->buffs.shining_light_free->up() ) ||
                             ( is_divine_storm && p->buffs.empyrean_power->up() );

    double num_hopo_spent = as<double>( holy_power_consumer_t::cost() );
    if ( is_hammer_of_light_driver && !p->buffs.templar.hammer_of_light_free->up() )
      num_hopo_spent = hol_cost;
    else if ( is_hammer_of_light )
      num_hopo_spent = 0.0;
    // Free spenders seem to count as 3 Holy Power, regardless the cost
    // Free Hammer of Light from Divine Purpose counts as 5 Holy Power spent, Free Hammer of Light from Light's
    // Deliverance counts as 0 Holy Power spent
    if ( isFreeSLDPSpender )
      num_hopo_spent = is_hammer_of_light_driver ? hol_cost : 3.0;

    if ( p->talents.righteous_cause->ok() && p->cooldowns.righteous_cause_icd->up() )
    {
      // TODO: verify that this is how this works
      unsigned base_cost = as<int>( ab::base_cost() );
      for ( unsigned i = 0; i < base_cost; i++ )
      {
        if ( ab::rng().roll( p->talents.righteous_cause->effectN( 1 ).percent() ) )
        {
          p->procs.righteous_cause->occur();
          p->cooldowns.blade_of_justice->reset( true );
          p->cooldowns.righteous_cause_icd->start();
          break;
        }
      }
    }

    if ( p->talents.relentless_inquisitor->ok() && !ab::background )
      p->buffs.relentless_inquisitor->trigger();

    /*
      Hammer of Light shenanigans, leaving these findings for later, not touching for now:
      Crusade, no RG:
      Execute gives 5 Stacks, if no DP active
      Damage gives 5 Stacks, if DP active
      (Can go to 10 stacks immediately, if HoL was the one to trigger DP, see
      https://www.warcraftlogs.com/reports/WjvLtYRhfq1PmQTF#fight=last&type=auras&ability=231895&view=events&pins=0%24Separate%24%23244F4B%24any%24-1%246015535.0.0.Unknown%240.0.0.Any%24true%240.0.0.Any%24true%24429826%7C427453%7C408458
      )

      Crusade, RG:
      Execute gives 5 Stacks, if no DP active
      Damage never gives stacks

      Light's Deliverance never gives Stacks
    */
    if ( p->buffs.crusade->check() )
    {
      double crusade_stacks_given = num_hopo_spent;

      if ( is_hammer_of_light_driver )
      {
        // 2024-08-05 The driver doesn't give stacks if it was free
        if ( ( p->buffs.divine_purpose->up() && p->bugs ) ||
             ( p->buffs.templar.hammer_of_light_free->up() && p->bugs ) )
        {
          crusade_stacks_given = 0.0;
        }
      }
      // Damage part of Hammer of Light
      else if ( is_hammer_of_light )
      {
        // 2024-08-05 The damage part gives stacks if Divine Purpose is up (Can go to 10 stacks immediately if the driver procced it). But only for hardcast Crusades
        if ( p->bugs && p->buffs.divine_purpose->up() && !p->talents.radiant_glory->ok() )
        {
          crusade_stacks_given = 5.0;
        }
      }
      if ( crusade_stacks_given > 0 )
        p->buffs.crusade->trigger( as<int>( crusade_stacks_given ) );
    }

    // Hammer of Light driver can proc it under all circumstances, but not the damage part
    if ( p->talents.radiant_glory->ok() && ( is_hammer_of_light_driver || !is_hammer_of_light ) )
    {
      // This is a bit of a hack. As far as we can tell from logs,
      // this agony-like accumulator logic matches the distribution
      // of procs pretty closely, tested on a couple thousand TV casts.
      // This will need periodic re-verification, but is good enough for beta
      // purposes.
      p->radiant_glory_accumulator += ab::rng().range( 0.0, 0.225 );
      if ( p->radiant_glory_accumulator >= 1.0 )
      {
        bool do_avatar = p->talents.herald_of_the_sun.suns_avatar->ok() && !( p->buffs.avenging_wrath->up() || p->buffs.crusade->up() );

        // TODO(mserrano): get this from spell data
        if ( p->talents.crusade->ok() )
        {
          p->buffs.crusade->extend_duration_or_trigger( timespan_t::from_seconds( 5 ) );
        }
        else if ( p->talents.avenging_wrath->ok() )
        {
          p->buffs.avenging_wrath->extend_duration_or_trigger( timespan_t::from_seconds( 4 ) );
        }
        p->radiant_glory_accumulator -= 1.0;

        if ( do_avatar )
        {
          p->apply_avatar_dawnlights();
        }
      }
    }

    // 2024-08-04 Currently, Hammer of Light doesn't affect Righteous Protector at all
    if ( p->talents.righteous_protector->ok() && ( ( is_hammer_of_light_driver && !p->bugs ) || !is_hammer_of_light ) )
    {
      // 23-03-23 Not sure when this bug was introduced, but free Holy Power Spenders ignore RP ICD
      if ( p->cooldowns.righteous_protector_icd->up() ||
           ( p->bugs && isFreeSLDPSpender ) )
      {
        timespan_t reduction = timespan_t::from_seconds(
            // Why is this in deciseconds?
            -1.0 * p->talents.righteous_protector->effectN( 1 ).base_value() / 100 );
        reduction *= num_hopo_spent;
        ab::sim->print_debug(
            "Righteous protector reduced the cooldown of Avenging Wrath and Guardian of Ancient Kings by {} sec",
            num_hopo_spent );

        p->cooldowns.avenging_wrath->adjust( reduction );
        p->cooldowns.sentinel->adjust( reduction );
        p->cooldowns.guardian_of_ancient_kings->adjust( reduction );

        p->cooldowns.righteous_protector_icd->start();
      }
    }

    // 2022-10-25 Resolute Defender, spend 3 HP to reduce AD/DS cooldown
    if ( p->talents.resolute_defender->ok() && !ab::background )
    {
      // Just like RP, value is in deciseconds, for whatever reasons
      timespan_t reduction =
          timespan_t::from_seconds( -1.0 * p->talents.resolute_defender->effectN( 1 ).base_value() / 10 );
      p->cooldowns.ardent_defender->adjust( reduction );
      p->cooldowns.divine_shield->adjust( reduction );
    }

    if ( p->talents.tirions_devotion->ok() && p->talents.lay_on_hands->ok() && !ab::background )
    {
      timespan_t reduction =
          timespan_t::from_seconds( -1.0 * p->talents.tirions_devotion->effectN( 1 ).base_value() * cost() );
      p->cooldowns.lay_on_hands->adjust( reduction );
    }

    // Consume Empyrean Power on Divine Storm, handled here for interaction with DP/FoJ
    // Cost reduction is still in divine_storm_t
    bool should_continue = true;
    if ( is_divine_storm && p->bugs )
    {
      if ( p->buffs.empyrean_power->up() )
      {
        p->buffs.empyrean_power->expire();
        should_continue = false;
      }
    }
    else if ( is_divine_storm )
    {
      if ( p->buffs.empyrean_power->up() )
      {
        p->buffs.empyrean_power->expire();
        should_continue = false;
      }
    }

    if ( is_wog && p->buffs.shining_light_free->check() )
    {
      should_continue = false;
      // Shining Light is now consumed before Divine Purpose 2020-11-01
      p->buffs.shining_light_free->decrement();
    }

    if ( p->buffs.sentinel->up() && p->buffs.sentinel_decay->up() && !ab::background )
    {
      // 2022-11-14 Free Holy Power spenders do not delay Sentinel's decay
      if ( !( p->bugs && isFreeSLDPSpender ) )
      {
        p->buffs.sentinel_decay->extend_duration( p, timespan_t::from_seconds( 1 ) );
      }
    }

    // We should only have should_continue false in the event that we're a divine storm
    // assert-check here for safety
    assert( is_divine_storm || should_continue || p->specialization() != PALADIN_RETRIBUTION );

    // Divine Purpose isn't consumed on DS if EP was consumed
    if ( should_continue )
    {
      if ( p->buffs.divine_purpose->up() && !doesnt_consume_dp )
      {
        p->buffs.divine_purpose->expire();
      }
    }

    // Roll for Divine Purpose
    // 2024-08-04 Damage event of Hammer of Light cannot proc Divine Purpose, if you're Ret
    // (Although it is also likely that the driver being able to proc Divine Purpose is also a bug, but who knows
    if ( !( p->bugs && !is_hammer_of_light_driver && is_hammer_of_light && p->specialization() == PALADIN_RETRIBUTION ) && p->talents.divine_purpose->ok() && this->rng().roll( p->talents.divine_purpose->effectN( 1 ).percent() ) )
    {
      p->buffs.divine_purpose->trigger();
      p->procs.divine_purpose->occur();
    }

    if ( p->talents.incandescence->ok() )
    {
      for ( int hopo = 0; hopo < num_hopo_spent; hopo++ )
      {
        // this appears to be hardcoded?
        if ( ab::rng().roll( 0.05 ) )
        {
          p->active.incandescence->schedule_execute();
        }
      }
    }

    if ( p->talents.faiths_armor->ok() )
    {
      if ( ( p->specialization() == PALADIN_RETRIBUTION && is_wog ) ||
           ( p->specialization() != PALADIN_RETRIBUTION && is_sotr ) )
      {
        p->buffs.faiths_armor->trigger();
      }
    }

    if ( p->buffs.blessing_of_dawn->up() && !is_hammer_of_light_driver )
    {
      p->buffs.blessing_of_dawn->expire();
      p->buffs.blessing_of_dusk->trigger();
    }
    if ( p->talents.lightsmith.divine_guidance->ok() )
    {
      p->buffs.lightsmith.divine_guidance->trigger();
    }
    if ( p->talents.lightsmith.blessed_assurance->ok() )
    {
      p->buffs.lightsmith.blessed_assurance->trigger();
    }
    if ( p->sets->has_set_bonus(PALADIN_PROTECTION, TWW1, B4) && !is_hammer_of_light && !is_hammer_of_light_driver )
    {
      p->buffs.rising_wrath->increment();
    }
  }
};

// Delayed Execute Event ====================================================

struct delayed_execute_event_t : public event_t
{
  action_t* action;
  player_t* target;

  delayed_execute_event_t( paladin_t* p, action_t* a, player_t* t, timespan_t delay )
    : event_t( *p->sim, delay ), action( a ), target( t )
  {
    assert( action->background );
  }

  const char* name() const override
  {
    return action->name();
  }

  void execute() override
  {
    if ( !target->is_sleeping() )
    {
      action->set_target( target );
      action->execute();
    }
  }
};

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, util::string_view options_str );
  void execute() override;
};

struct judgment_t : public paladin_melee_attack_t
{
  judgment_t( paladin_t* p, util::string_view name );

  proc_types proc_type() const override;
  void impact( action_state_t* s ) override;
  void execute() override;
  double action_multiplier() const override;
};

struct shield_of_the_righteous_buff_t : public buff_t
{
  shield_of_the_righteous_buff_t( paladin_t* p );
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

void empyrean_power( special_effect_t& effect );

}  // namespace paladin

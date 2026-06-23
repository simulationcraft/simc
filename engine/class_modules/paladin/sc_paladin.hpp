#pragma once
#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"

namespace paladin
{
// Forward declarations
typedef std::pair<std::string, simple_sample_data_with_min_max_t> data_t;
typedef std::pair<std::string, simple_sample_data_t> simple_data_t;
struct paladin_t;
struct blessing_of_sacrifice_redirect_t;
namespace buffs
{
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

enum armament : unsigned int
{
  HOLY_BULWARK  = 0,
  SACRED_WEAPON = 1,
  NUM_ARMAMENT  = 2,
};

enum armament_source : unsigned int
{
  LS_HARDCAST           = 0,
  LS_WINGS              = 1,
  LS_DIVINE_INSPIRATION = 2,
};

enum lesser_armament : unsigned int
{
  LESSER_WEAPON  = 0,
  LESSER_BULWARK = 1,
};

enum hammer_and_anvil_source : unsigned int
{
  HAA_JUDGMENT    = 0,
  HAA_DIVINE_TOLL = 1,
};

enum consecration_source : unsigned int
{
  HARDCAST         = 0,
  BLADE_OF_JUSTICE = 1,
  HAMMER_OF_LIGHT  = 2,
};

enum grand_crusader_source : unsigned int
{
  GC_NORMAL   = 0,
  GC_JUDGMENT = 1,
  GC_ROR      = 2,
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
  } dot;

  struct buffs_t
  {
    buffs::execution_sentence_debuff_t* execution_sentence;
    buff_t* execution_sentence_gather;
    buff_t* judgment;
    buff_t* blessed_hammer;
    buff_t* sanctify;
    buff_t* crusaders_resolve;
    buff_t* empyrean_hammer;
    buff_t* consecration;
    buff_t* seal_of_reprisal;
  } debuff;

  struct
  {
    buff_t* holy_bulwark;
    buff_t* sacred_weapon;
    buff_t* lesser_weapon;
    absorb_buff_t* lesser_bulwark;
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
    action_t* empyrean_hammer;
    action_t* shield_of_vengeance_damage;
    action_t* zeal;

    blessing_of_sacrifice_redirect_t* blessing_of_sacrifice_redirect;

    // Covenant stuff
    action_t* divine_toll;
    action_t* divine_toll_how;
    action_t* divine_resonance;
    action_t* divine_resonance_ret;
    action_t* divine_resonance_ret_how;
    action_t* divine_exaction_prot;

    // talent stuff
    action_t* background_cons;
    action_t* empyrean_legacy;
    action_t* es_explosion;
    action_t* background_blessed_hammer;
    action_t* hammer_of_light_cons;

    action_t* expurgation;

    action_t* divine_hammer_tick;

    action_t* sacrosanct_crusade_heal;
    action_t* highlords_judgment;
    action_t* dawnlight;
    action_t* sun_sear;
    action_t* blade_of_justice;
    action_t* armament[ NUM_ARMAMENT ];
    action_t* sacred_weapon_proc_damage;
    action_t* sacred_weapon_proc_heal;
    action_t* lesser_weapon_proc_damage;
    action_t* lesser_weapon_proc_heal;
    action_t* eye_for_an_eye;

    action_t* background_avenging_wrath;
    action_t* background_crusade;
  } active;

  // Buffs
  struct buffs_t
  {
    // Shared
    buff_t* avenging_wrath;
    buff_t* divine_purpose;
    buff_t* divine_shield;
    buff_t* divine_steed;
    buff_t* devotion_aura;
    buff_t* blessing_of_protection;
    buff_t* faiths_armor;
    buff_t* hammer_of_wrath;

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
    buff_t* ardent_defender;
    buff_t* guardian_of_ancient_kings;
    buff_t* redoubt;
    buff_t* shield_of_the_righteous;
    buff_t* shining_light_stacks;
    buff_t* shining_light_free;
    buff_t* blessing_of_spellwarding;
    buff_t* strength_in_adversity;

    // Apex
    buff_t* vanguard;
    buff_t* valor;

    // Ret
    buffs::shield_of_vengeance_buff_t* shield_of_vengeance;
    buff_t* empyrean_power;

    buff_t* art_of_war;
    buff_t* righteous_cause;

    buff_t* rush_of_light;
    buff_t* templar_strikes;

    buff_t* bulwark_of_righteous_fury;
    buff_t* blessing_of_dusk;
    buff_t* blessing_of_dawn;
    buff_t* divine_resonance;

    buff_t* empyrean_legacy;
    buff_t* empyrean_legacy_cooldown;
    buff_t* judge_jury_and_executioner;

    buff_t* execution_sentence;

    buff_t* echoes_of_wrath;  // DF3 4pc

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
      buff_t* fake_solidarity; // Stackable buff that fakes other people having a Sacred Weapon buff
      buff_t* fake_solidarity_bulwark; // Stackable buff that fakes other people having a Holy Bulwark Buff (Only for Reflection of Radiance)
      buff_t* masterwork_weapon;
      buff_t* masterwork_bulwark;
      buff_t* lesser_weapon;
      absorb_buff_t* lesser_bulwark;
    } lightsmith;

    struct
    {
      buff_t* hammer_of_light_ready;
      buff_t* hammer_of_light_free;
      buff_t* shake_the_heavens;
      buff_t* sacrosanct_crusade;
      buff_t* sanctification;
      buff_t* undisputed_ruling;
      buff_t* lights_deliverance;
      buff_t* divine_hammer;
    } templar;

    struct
    {
      buff_t* dawnlight;
      buff_t* blessing_of_anshe;
      buff_t* morning_star;
      buff_t* solar_grace;
      buff_t* morning_star_driver;
      buff_t* born_in_sunlight; // Hidden passive buff
    } herald_of_the_sun;

    buff_t* rise_from_ash; // Ret TWW1 4p
    buff_t* winning_streak; // Ret TWW2 2pc
    buff_t* all_in; // Ret TWW2 4pc

    buff_t* light_blessed_shield; // Prot Midnight 4p
  } buffs;

  // Gains
  struct gains_t
  {
    // Healing/absorbs
    gain_t* holy_shield;
    gain_t* bulwark_of_order;
    gain_t* sacrosanct_crusade;
    gain_t* blessed_hammer;
    gain_t* holy_bulwark;
    gain_t* lesser_bulwark;

    // Mana
    gain_t* mana_beacon_of_light;

    // Holy Power
    gain_t* hp_templars_verdict_refund;
    gain_t* judgment;
    gain_t* hp_cs;
    gain_t* hp_divine_toll;
    gain_t* hp_crusading_strikes;
    gain_t* hp_judge_jury_and_executioner_refund;
    gain_t* hp_glory_of_the_vanguard_2;
    gain_t* hp_walk_into_light;
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
    cooldown_t* blessing_of_protection;    // Blessing of Spellwarding Shared CD
    cooldown_t* blessing_of_spellwarding;  // Blessing of Protection Shared CD
    cooldown_t* lay_on_hands;              // Tirion's Devotion

    cooldown_t* holy_shock;     // Crusader's Might, Divine Purpose
    cooldown_t* light_of_dawn;  // Divine Purpose

    cooldown_t* avengers_shield;  // Grand Crusader
    cooldown_t* consecration;     // Precombat shenanigans
    cooldown_t* judgment;                   // Crusader's Judgment
    cooldown_t* guardian_of_ancient_kings;  // Righteous Protector

    cooldown_t* blade_of_justice;
    cooldown_t* hammer_of_wrath;
    cooldown_t* wake_of_ashes;
    cooldown_t* divine_toll;

    cooldown_t* holy_armaments;

    cooldown_t* ret_aura_icd;
    cooldown_t* consecrated_blade_icd;
    cooldown_t* righteous_cause_icd;

    cooldown_t* aurora_icd;
    cooldown_t* second_sunrise_icd;
    cooldown_t* walk_into_light_icd;

    cooldown_t* hammerfall_icd;
    cooldown_t* art_of_war;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* paladin;
    const spell_data_t* plate_specialization;

    const spell_data_t* infusion_of_light;

    const spell_data_t* riposte;
    const spell_data_t* sanctuary;

    const spell_data_t* aegis_of_light;

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
    proc_t* art_of_war_wasted;
    proc_t* righteous_cause;
    proc_t* divine_purpose;
    proc_t* empyrean_power;

    proc_t* as_grand_crusader;
    proc_t* as_grand_crusader_wasted;
    proc_t* as_moment_of_glory;
    proc_t* as_moment_of_glory_wasted;

    proc_t* divine_inspiration;

    proc_t* templar_lights_judicator;

    proc_t* grand_crusader_ror_sw;
    proc_t* grand_crusader_ror_hb;
  } procs;

  struct proc_data_entries_t
  {
    proc_data_t crusading_strikes_energize;
  } proc_data_entries;

  // Spells
  struct spells_t
  {
    const spell_data_t* avenging_wrath;
    const spell_data_t* divine_purpose_buff;
    const spell_data_t* judgment_debuff;
    const spell_data_t* sanctify;
    const spell_data_t* consecration;
    const spell_data_t* seal_of_reprisal;

    const spell_data_t* sotr_buff;
    const spell_data_t* standing_in_consecration_buff;

    const spell_data_t* consecrated_blade;
    const spell_data_t* crusade;
    const spell_data_t* sentinel;
    const spell_data_t* refining_fire_tick;
    const spell_data_t* expurgation;
    const spell_data_t* crusading_strikes_data;

    struct
    {
      const spell_data_t* holy_bulwark;
      const spell_data_t* holy_bulwark_absorb;
      const spell_data_t* forges_reckoning; // Spell triggered by Blessing of the Forge (Shield of the Righteous)
      const spell_data_t* sacred_word;      // Spell triggered by Blessing of the Forge (Word of Glory)
      const spell_data_t* lesser_bulwark; // TWW3 Absorb
      const spell_data_t* lesser_weapon; // TWW3 Damage
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
      const spell_data_t* dawnlight_aoe_metadata;
    } herald_of_the_sun;

    const spell_data_t* highlords_judgment_hidden;

    // Apex
    const spell_data_t* glory_of_the_vanguard;
    const spell_data_t* blaze_of_glory;
    const spell_data_t* light_within;

    // More Hammer of Wrath spell data for Ret
    const spell_data_t* judgment_ret;
    const spell_data_t* judgment_ret_dt;
    const spell_data_t* hammer_of_wrath_ret;
    const spell_data_t* hammer_of_wrath_ret_dt;

    // Tier stuff
    const spell_data_t* unrelenting_edict;
  } spells;

  struct rppms_t {
  } rppm;

  // Talents
  struct talents_t
  {
    // Duplicate names are commented out

    // Class

    // 0
    const spell_data_t* lay_on_hands;
    const spell_data_t* auras_of_the_resolute;
    const spell_data_t* hammer_of_wrath;

    const spell_data_t* cleanse_toxins;
    const spell_data_t* empyreal_ward;
    const spell_data_t* fist_of_justice;
    const spell_data_t* blinding_light;
    const spell_data_t* turn_evil;

    const spell_data_t* a_just_reward;
    const spell_data_t* afterimage;
    const spell_data_t* healing_hands; // Ret only
    const spell_data_t* guided_prayer;
    const spell_data_t* divine_steed;
    const spell_data_t* lights_countenance;
    const spell_data_t* greater_judgment;
    const spell_data_t* wrench_evil;
    const spell_data_t* stand_against_evil;

    const spell_data_t* holy_reprieve;
    const spell_data_t* shield_of_vengeance;
    const spell_data_t* cavalier;
    const spell_data_t* divine_spurs;
    const spell_data_t* steed_of_liberty;
    const spell_data_t* blessing_of_freedom;
    const spell_data_t* rebuke;

    // 8
    const spell_data_t* obduracy;
    const spell_data_t* divine_toll;
    const spell_data_t* unbound_freedom;
    const spell_data_t* sanctified_plates;
    const spell_data_t* punishment;

    const spell_data_t* divine_reach;
    const spell_data_t* brought_to_light;
    const spell_data_t* blessing_of_sacrifice;
    const spell_data_t* divine_resonance;
    const spell_data_t* quickened_invocation;
    const spell_data_t* blessing_of_protection;
    const spell_data_t* fear_no_evil;
    const spell_data_t* consecrated_ground;

    const spell_data_t* holy_aegis;
    const spell_data_t* sacrifice_of_the_just;
    const spell_data_t* recompense;
    const spell_data_t* sacred_strength;
    const spell_data_t* divine_purpose;
    const spell_data_t* improved_blessing_of_protection;
    const spell_data_t* unbreakable_spirit;

    // 20
    const spell_data_t* lightforged_blessing;
    const spell_data_t* lead_the_charge;
    const spell_data_t* worthy_sacrifice;
    const spell_data_t* righteous_protection;
    const spell_data_t* holy_ritual;
    const spell_data_t* blessed_calling;
    const spell_data_t* inspired_guard;
    const spell_data_t* lights_revocation;

    const spell_data_t* faiths_armor;
    const spell_data_t* stoicism;
    const spell_data_t* seal_of_might;
    const spell_data_t* vengeful_wrath;
    const spell_data_t* eye_for_an_eye;
    const spell_data_t* golden_path;
    const spell_data_t* selfless_healer;

    const spell_data_t* blessing_of_dawn;
    const spell_data_t* lightbearer;
    const spell_data_t* blessing_of_dusk;

    const spell_data_t* avenging_wrath;  // Keeping this because it still contains all the spell data needed

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

    const spell_data_t* shining_light;
    const spell_data_t* hammer_of_the_righteous;
    const spell_data_t* blessed_hammer;

    const spell_data_t* imbued_shield;
    const spell_data_t* redoubt;
    const spell_data_t* grand_crusader;
    const spell_data_t* seal_of_charity;

    const spell_data_t* refining_fire;
    const spell_data_t* valiant_crusade;
    const spell_data_t* ardent_defender;
    const spell_data_t* searing_sunlight;
    const spell_data_t* solace;

    // 8
    const spell_data_t* undying_embers;
    const spell_data_t* bulwark_of_order;
    const spell_data_t* improved_ardent_defender;
    const spell_data_t* blessing_of_spellwarding;
    const spell_data_t* light_of_the_titans;
    const spell_data_t* tirions_devotion;
    const spell_data_t* vision_of_sanctity;

    const spell_data_t* tyrs_enforcer;
    const spell_data_t* relentless_inquisitor;
    const spell_data_t* avenging_wrath_might;
    const spell_data_t* sentinel;
    const spell_data_t* crusaders_judgment;
    const spell_data_t* consecration_in_flame;

    const spell_data_t* soaring_shield;
    const spell_data_t* seal_of_reprisal;
    const spell_data_t* guardian_of_ancient_kings;
    const spell_data_t* hand_of_the_protector;
    const spell_data_t* sanctuary;

    // 20
    const spell_data_t* focused_enmity;
    const spell_data_t* gift_of_the_golden_valkyr;
    const spell_data_t* sanctified_wrath;
    const spell_data_t* uthers_counsel;

    const spell_data_t* strength_in_adversity;
    const spell_data_t* crusaders_resolve;
    const spell_data_t* ferren_marcuss_fervor;
    const spell_data_t* empyrean_authority;
    const spell_data_t* zealots_paragon;
    const spell_data_t* instrument_of_the_divine;

    const spell_data_t* sweeping_verdict;
    const spell_data_t* adjudication;
    const spell_data_t* bulwark_of_righteous_fury;
    const spell_data_t* final_stand;
    const spell_data_t* righteous_protector;

    const spell_data_t* glory_of_the_vanguard_1;
    const spell_data_t* glory_of_the_vanguard_2;
    const spell_data_t* glory_of_the_vanguard_3;

    // Retribution
    const spell_data_t* blade_of_justice;
    const spell_data_t* divine_storm;

    const spell_data_t* swift_justice;
    const spell_data_t* light_of_justice;
    const spell_data_t* expurgation;
    const spell_data_t* judgment_of_justice;
    // available up above
    // const spell_data_t* avenging_wrath;

    const spell_data_t* final_verdict;
    const spell_data_t* improved_blade_of_justice;
    const spell_data_t* holy_blade;
    const spell_data_t* art_of_war;
    const spell_data_t* righteous_cause;

    const spell_data_t* jurisdiction;
    const spell_data_t* tempest_of_the_lightbringer;
    const spell_data_t* rush_of_light;
    const spell_data_t* sanctify;
    const spell_data_t* holy_flames;

    const spell_data_t* improved_judgment;
    const spell_data_t* boundless_judgment;
    const spell_data_t* zealots_fervor;
    const spell_data_t* heart_of_the_crusader;
    const spell_data_t* blade_of_vengeance;

    const spell_data_t* empyrean_power;
    const spell_data_t* highlords_wrath;
    const spell_data_t* templar_strikes;
    const spell_data_t* crusading_strikes;
    const spell_data_t* blessed_champion;
    const spell_data_t* burning_crusade;

    const spell_data_t* blades_of_light;
    const spell_data_t* wake_of_ashes;
    const spell_data_t* divine_wrath;

    const spell_data_t* execution_sentence;
    const spell_data_t* seething_flames;
    const spell_data_t* empyrean_legacy;

    const spell_data_t* judge_jury_and_executioner;
    const spell_data_t* radiant_glory;
    const spell_data_t* burn_to_ash;
    const spell_data_t* crusade;

    // const spell_data_t* avenging_wrath_might; // available up in prot
    const spell_data_t* consecrated_ground_ret; // TODO: implement or drop

    const spell_data_t* light_within_1;
    const spell_data_t* light_within_2;
    const spell_data_t* light_within_3;

    // Hero Talents
    struct
    {
      const spell_data_t* holy_armaments;

      const spell_data_t* rite_of_sanctification;
      const spell_data_t* rite_of_adjuration;
      const spell_data_t* solidarity;
      const spell_data_t* divine_guidance;
      const spell_data_t* blessed_assurance;
      const spell_data_t* masterwork;

      const spell_data_t* laying_down_arms;
      const spell_data_t* divine_inspiration;
      const spell_data_t* forewarning;
      const spell_data_t* authoritative_rebuke;
      const spell_data_t* tempered_in_battle;
      const spell_data_t* hammer_and_anvil;

      const spell_data_t* shared_resolve;
      const spell_data_t* valiance;
      const spell_data_t* reflection_of_radiance;
      const spell_data_t* resounding_strike;

      const spell_data_t* blessing_of_the_forge;
    } lightsmith;

    struct
    {
      const spell_data_t* lights_guidance;

      const spell_data_t* zealous_vindication;
      const spell_data_t* shake_the_heavens;
      const spell_data_t* wrathful_descent;
      const spell_data_t* divine_hammer;

      const spell_data_t* sacrosanct_crusade;
      const spell_data_t* higher_calling;
      const spell_data_t* bonds_of_fellowship;
      const spell_data_t* unrelenting_charger;
      const spell_data_t* lights_judicator;

      const spell_data_t* endless_wrath;
      const spell_data_t* sanctification;
      const spell_data_t* hammerfall;
      const spell_data_t* undisputed_ruling;
      const spell_data_t* divine_exaction;
      const spell_data_t* seal_of_the_templar;

      const spell_data_t* lights_deliverance;
    } templar;

    struct {
      const spell_data_t* dawnlight;

      const spell_data_t* morning_star;
      const spell_data_t* gleaming_rays;
      const spell_data_t* eternal_flame;
      const spell_data_t* luminosity;
      const spell_data_t* endless_gleam;

      const spell_data_t* illumine;
      const spell_data_t* will_of_the_dawn;
      const spell_data_t* blessing_of_anshe;
      const spell_data_t* lingering_radiance;
      const spell_data_t* sun_sear;
      const spell_data_t* solar_grace;

      const spell_data_t* aurora;
      const spell_data_t* walk_into_light;
      const spell_data_t* second_sunrise;
      const spell_data_t* born_in_sunlight;

      const spell_data_t* suns_avatar;
    } herald_of_the_sun;
  } talents;

  // Paladin options
  struct options_t
  {
    bool fake_sov                         = true;
    int min_dg_heal_targets               = 1;
    int max_dg_heal_targets               = 5;
    bool fake_solidarity                  = true;
    double ror_bulwark_additional_proc_chance = .3;
    double blessed_hammer_strikes          = 2.0;
    std::string starting_armament             = "sacred_weapon";
  } options;
  player_t* beacon_target;

  armament next_armament;


  // Helper variables to not always RNG the correct target
  player_t* random_weapon_target;
  player_t* random_bulwark_target;
  int divine_inspiration_next;

  double reflection_of_radiance_proc_chance;

  paladin_t( sim_t* sim, util::string_view name, race_e r = RACE_TAUREN );

  void init_assessors() override;
  void init_base_stats() override;
  void init_gains() override;
  void init_procs() override;
  void init() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_special_effects() override;
  void init_rng() override;
  void init_spells() override;
  void init_action_list() override;
  void init_blizzard_action_list() override;
  void init_proc_data_entries();
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                            const assisted_combat_step_data_t& step ) const override;
  bool validate_fight_style( fight_style_e style ) const override;
  bool validate_actor() override;
  void reset() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;

  // player stat functions
  double composite_player_multiplier( school_e ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_attack_power_multiplier() const override;
  double composite_bonus_armor() const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double composite_damage_versatility() const override;
  double composite_heal_versatility() const override;
  double composite_mitigation_versatility() const override;
  double composite_mastery() const override;
  double composite_melee_haste() const override;
  double composite_melee_auto_attack_speed() const override;
  double composite_spell_haste() const override;
  double composite_crit_avoidance() const override;
  double composite_parry() const override;
  double composite_parry_rating() const override;
  double composite_block() const override;
  double composite_mitigation_multiplier( const action_state_t*, school_e, bool direct ) const override;
  double composite_mitigation_from_player_multiplier( player_t*, const action_state_t*, school_e, bool direct ) const override;
  double non_stacking_movement_modifier() const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_base_armor_multiplier() const override;

  double resource_gain( resource_e resource_type, double amount, gain_t* source = nullptr,
                                action_t* action = nullptr ) override;
  double resource_loss( resource_e resource_type, double amount, gain_t* source = nullptr,
                                action_t* action = nullptr ) override;

  // combat outcome functions
  void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  block_result_e target_block_resolution( const action_state_t* ) const override;

  void invalidate_cache( cache_e ) override;
  void create_options() override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void create_actions() override;
  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void combat_begin() override;
  void copy_from( player_t* ) override;

  void trigger_grand_crusader( grand_crusader_source source = GC_NORMAL );
  void trigger_laying_down_arms();
  void trigger_empyrean_hammer( player_t* target, int number_to_trigger, timespan_t delay, bool random_after_first = false );
  void trigger_lights_deliverance();
  void trigger_expurgation( player_t* target, double effectiveness );
  void trigger_forbearance( player_t* target );
  void accumulate_es_damage( action_state_t* s, double mult );
  void trigger_es_explosion( player_t* target );
  int get_local_enemies( double distance ) const;
  bool standing_in_consecration() const;
  void adjust_health_percent();
  void cast_holy_armaments( player_t* target, armament usedArmament, armament_source src );
  void cast_lesser_armament( int amount, lesser_armament usedArmament );
  void trigger_greater_judgment( paladin_td_t* targetdata );
  bool get_how_availability() const;
  bool wings_up() const;
  bool templar() const;
  bool lightsmith() const;
  bool herald_of_the_sun() const;

  std::unique_ptr<expr_t> create_consecration_expression( util::string_view expr_str );
  std::unique_ptr<expr_t> create_aw_expression( util::string_view expr_str );
  std::unique_ptr<expr_t> create_vw_expression( util::string_view expr_str );

  ground_aoe_event_t* active_consecration;
  ground_aoe_event_t* active_boj_cons;
  std::set<ground_aoe_event_t*> all_active_consecrations;
  buff_t* active_aura;

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

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

  void apply_action_effects( action_t* a );
  void apply_target_action_effects( action_t* a );

  void generate_action_prio_list_prot();
  void generate_action_prio_list_holy();
  void generate_action_prio_list_holy_dps();

  target_specific_t<paladin_td_t> target_data;

  virtual const paladin_td_t* find_target_data( const player_t* target ) const override;
  virtual paladin_td_t* get_target_data( player_t* target ) const override;

  dbc_proc_callback_t* create_sacred_weapon_callback( paladin_t* source, player_t* target );
  dbc_proc_callback_t* create_lesser_weapon_callback( paladin_t* source, player_t* target );

  std::vector<int> fake_lesser_weapon_set;
};

namespace buffs
{
struct execution_sentence_debuff_t : public buff_t
{
  execution_sentence_debuff_t( paladin_td_t* td )
    : buff_t( *td, "execution_sentence_debuff", debug_cast<paladin_t*>( td->source )->talents.execution_sentence )
  {
    set_cooldown( 0_ms );  // handled by the ability
  }

  void reset() override
  {
    buff_t::reset();
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );
    paladin_t* paladin = debug_cast<paladin_t*>( source );
    paladin->trigger_es_explosion( player );
  }
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
    caster = debug_cast<paladin_t*>( td->source );
    set_absorb_source( caster->get_stats( "holy_bulwark_absorb_" + td->target->name_str ) );
  }
  holy_bulwark_absorb_t( paladin_t* p )
    : absorb_buff_t( p, "holy_bulwark_absorb", p->spells.lightsmith.holy_bulwark_absorb )
  {
    caster = p;
    set_absorb_source( caster->get_stats( "holy_bulwark_absorb" ) );
  }
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    double total_value = this->value();
    if ( value > 0 )
    {
      total_value += value;
    }
    else
    {
      total_value += this->player->resources.max[ RESOURCE_HEALTH ] *
                     ( caster->spells.lightsmith.holy_bulwark->effectN( 4 ).percent() / 10.0 );
    }
    total_value = std::min( total_value, this->player->resources.max[ RESOURCE_HEALTH ] *
                                             caster->spells.lightsmith.holy_bulwark->effectN( 5 ).percent() );
    return absorb_buff_t::trigger( stacks, total_value, chance, duration );
  }
  void absorb_used( double absorbed, player_t* source ) override
  {
    absorb_buff_t::absorb_used( absorbed, source );
    double chance = caster->reflection_of_radiance_proc_chance;
    double stacks = caster->buffs.lightsmith.fake_solidarity_bulwark->stack();
    // Holy Bulwarks on the group don't trigger all that often, so it shouldn't be a 100% increased chance
    double increasedChance = stacks * caster->options.ror_bulwark_additional_proc_chance;  
    if ( caster->options.fake_solidarity )
      chance = 1.0 - ( std::pow( 1.0 - chance, increasedChance  + 1 ) );
    if ( caster->talents.lightsmith.reflection_of_radiance->ok() && caster->rng().roll( chance ) )
    {
      caster->trigger_grand_crusader( GC_ROR );
      caster->procs.grand_crusader_ror_hb->occur();
    }
  }
};

struct lesser_bulwark_buff_t : public absorb_buff_t
{
  paladin_t* caster;
  lesser_bulwark_buff_t(paladin_td_t* td)
    : absorb_buff_t(td->target, "lesser_bulwark_ally_" + td->source->name_str + "_" + td->target->name_str,
      debug_cast<paladin_t*>(td->source)->spells.lightsmith.lesser_bulwark)
  {
    caster = debug_cast<paladin_t*>( td->source );
    set_absorb_source( caster->get_stats( "lesser_bulwark_absorb_" + td->target->name_str ) );
  }
  lesser_bulwark_buff_t( paladin_t* p ) : absorb_buff_t( p, "lesser_bulwark", p->spells.lightsmith.lesser_bulwark )
  {
    caster = p;
    set_absorb_source( caster->get_stats( "lesser_bulwark_absorb" ) );
  }

  bool trigger(int stacks, double value, double chance, timespan_t duration) override
  {
    value      = value < 0 ? 0 : value;
    double MAP = caster->composite_melee_attack_power() *caster->spells.lightsmith.lesser_bulwark->effectN( 1 ).ap_coeff();
    MAP *= 1.0 + caster->composite_heal_versatility();
    return absorb_buff_t::trigger( stacks, value + MAP, chance, duration );
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
  double get_healing_mod() const
  {
    return healing_modifier;
  }
  double get_crit_bonus() const
  {
    return crit_bonus;
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
  double healing_modifier;
  double crit_bonus;
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
struct paladin_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;  // action base, eg. spell_t
public:
  using base_t = paladin_action_t;

  // Damage increase whitelists
  struct affected_by_t
  {
    bool avenging_wrath, divine_purpose, divine_purpose_cost;  // Shared
    bool crusade, highlords_judgment, highlords_judgment_hidden,
      rise_from_ash; // Ret
    bool avenging_crusader;                                                                // Holy
    bool sentinel;  // Prot
  } affected_by;

  // haste scaling bools
  bool hasted_cd;

  bool clears_judgment;

  bool triggers_higher_calling;

  paladin_action_t( util::string_view n, paladin_t* p, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, p, s ),
      affected_by( affected_by_t() ),
      hasted_cd( false ),
      clears_judgment( false ),
      triggers_higher_calling( false )
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
      this->affected_by.rise_from_ash =
          this->data().affected_by( p->find_spell( 454693 )->effectN( 1 ) );
    }
    if ( p->specialization() == PALADIN_HOLY )
    {
      this->affected_by.avenging_crusader = this->data().affected_by( p->talents.avenging_crusader->effectN( 1 ) );
    }
    if ( p->specialization() == PALADIN_PROTECTION )
    {
      this->affected_by.sentinel         = this->data().affected_by( p->talents.sentinel->effectN( 1 ) );
    }

    this->clears_judgment                 = this->data().affected_by( p->spells.judgment_debuff->effectN( 1 ) );
    this->affected_by.avenging_wrath      = this->data().affected_by( p->spells.avenging_wrath->effectN( 2 ) );
    this->affected_by.sentinel            = this->data().affected_by( p->spells.sentinel->effectN( 1 ) );
    this->affected_by.divine_purpose_cost = this->data().affected_by( p->spells.divine_purpose_buff->effectN( 1 ) );
    this->affected_by.divine_purpose      = this->data().affected_by( p->spells.divine_purpose_buff->effectN( 2 ) );

    if ( this->data().ok() )
    {
      p->apply_action_effects( this );
      if (this->type == action_e::ACTION_SPELL || this->type == action_e::ACTION_ATTACK)
      {
        p->apply_target_action_effects( this );
      }
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
  }

  void execute() override
  {
    ab::execute();

    if ( triggers_higher_calling && p()->talents.templar.higher_calling->ok() && p()->buffs.templar.shake_the_heavens->up() )
    {
      timespan_t extension = timespan_t::from_seconds( p()->talents.templar.higher_calling->effectN( 1 ).base_value() );
      // If Crusading Strikes is triggering, extension is only 500ms
      if ( ab::id == 408385 )
        extension = 500_ms;
      p()->buffs.templar.shake_the_heavens->extend_duration( extension );
    }

    if ( ab::current_resource() == RESOURCE_HOLY_POWER && ab::last_resource_cost > 0 && p()->buffs.judge_jury_and_executioner->up() )
    {
      p()->resource_gain( ab::current_resource(), ab::last_resource_cost, p()->gains.hp_judge_jury_and_executioner_refund );
      p()->buffs.judge_jury_and_executioner->decrement();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::result_is_hit( s->result ) )
    {
      if ( clears_judgment )
      {
        paladin_td_t* td = this->td( s->target );
        if ( td->debuff.judgment->up() )
          td->debuff.judgment->decrement();
      }
      if ( !p()->is_ptr() )
      {
        if ( p()->buffs.lightsmith.masterwork_weapon->up() )
        {
          p()->buffs.lightsmith.masterwork_weapon->decrement();
          p()->cast_lesser_armament( 1, LESSER_WEAPON );
        }
        if ( p()->buffs.lightsmith.masterwork_bulwark->up() )
        {
          p()->buffs.lightsmith.masterwork_bulwark->decrement();
          p()->cast_lesser_armament( 1, LESSER_BULWARK );
        }
      }
    }

    if ( ab::energize_resource_() == RESOURCE_HOLY_POWER && p()->talents.rush_of_light->ok() && s->result == RESULT_CRIT )
    {
      p()->buffs.rush_of_light->trigger();
    }
  }

  double action_multiplier() const override
  {
    double am = ab::action_multiplier();

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
    }

    if ( affected_by.rise_from_ash && p()->buffs.rise_from_ash->up() )
    {
      am *= 1.0 + p()->buffs.rise_from_ash->data().effectN( 1 ).percent();
    }

    if ( affected_by.avenging_crusader )
    {
      am *= 1.0 + p()->buffs.avenging_crusader->check_value();
    }

    return am;
  }

  virtual double composite_target_ta_multiplier( player_t* target ) const override
  {
    double cttm = ab::composite_target_ta_multiplier( target );

    paladin_td_t* td = this->td( target );
    if ( p()->talents.burn_to_ash->ok() && td->dot.truths_wake->is_ticking() && ab::id != 403695 )
      cttm *= 1.0 + p()->talents.burn_to_ash->effectN( 2 ).percent();

    return cttm;
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
  double beacon_pct;

  paladin_heal_t( util::string_view n, paladin_t* p, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, p, s ), beacon_pct( p->find_spell( 53563 )->effectN( 1 ).percent() )
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

    if ( proc )
      return;

    assert( p()->active.beacon_of_light );

    p()->active.beacon_of_light->target = p()->beacon_target;

    double amount = s->result_amount * beacon_pct;

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
  bool is_hammer_of_light_cleave;
  bool is_hammer_of_light_main;
  double hol_cost;

  // Because every different Divine Storm behaves differently
  bool triggers_endless_gleam;
  bool triggers_divine_purpose;
  bool triggers_crusade_stacks;
  bool triggers_righteous_cause;

  holy_power_consumer_t( util::string_view n, paladin_t* player, const spell_data_t* s )
    : ab( n, player, s ),
      is_divine_storm( false ),
      is_wog( false ),
      is_sotr( false ),
      doesnt_consume_dp( false ),
      is_hammer_of_light_cleave( false ),
      is_hammer_of_light_main( false ),
      hol_cost( 3.0 ),
      triggers_endless_gleam(true),
      triggers_divine_purpose(true),
      triggers_crusade_stacks(true),
      triggers_righteous_cause(true)
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
    dot_t* d     = this->td( s->target )->dot.dawnlight;
    if ( triggers_endless_gleam && d && d->is_ticking() )
    {
      d->adjust_duration(
          timespan_t::from_millis( p->talents.herald_of_the_sun.endless_gleam->effectN( 2 ).base_value() ) );
    }
    ab::impact( s );

    if ( !is_hammer_of_light_cleave && p->talents.templar.hammerfall->ok() && p->cooldowns.hammerfall_icd->up() )
    {
      int additionalTargets = 0;
      if ( p->buffs.templar.shake_the_heavens->up() )
        // Disappeared from spell data
        additionalTargets += 1; //as<int>( p->talents.templar.hammerfall->effectN( 2 ).base_value() );
      p->trigger_empyrean_hammer( nullptr, 1 + additionalTargets,
                                  timespan_t::from_millis( p->talents.templar.hammerfall->effectN( 1 ).base_value() ),
                                  true );
      p->cooldowns.hammerfall_icd->start();
    }

    // We only apply Dawnlight on the first target hit
    if ( ab::result_is_hit( s->result ) && p->buffs.herald_of_the_sun.dawnlight->up() && !ab::background && s->chain_target == 0 )
    {
      // We only apply Dawnlight if it is not already ticking on the target - Or if there is only 1 target
      if ( !d || !d->is_ticking() || ab::target_list().size() == 1 )
      {
        p->active.dawnlight->execute_on_target( s->target );
        p->buffs.herald_of_the_sun.dawnlight->decrement();
      }
      // If Dawnlight is already ticking on out first target, we look for another target
      else
      {
        for (auto& tl : ab::target_list())
        {
          if (!ab::td(tl)->dot.dawnlight->is_ticking())
          {
            p->active.dawnlight->execute_on_target( tl );
            p->buffs.herald_of_the_sun.dawnlight->decrement();
            break;
          }
        }
      }
    }
  }

  void execute() override
  {
    // p variable just to make this look neater
    paladin_t* p = ab::p();

    bool isFreeSLDPSpender = p->buffs.divine_purpose->up() || ( is_wog && p->buffs.shining_light_free->up() ) ||
                             ( is_divine_storm && p->buffs.empyrean_power->up() );

    [[maybe_unused]] double num_hopo_spent = as<double>( holy_power_consumer_t::cost() );
    if ( is_hammer_of_light_main && !p->buffs.templar.hammer_of_light_free->up() )
      num_hopo_spent = hol_cost;
    else if ( is_hammer_of_light_cleave )
      num_hopo_spent = 0.0;
    // Free spenders seem to count as 3 Holy Power, regardless the cost
    // Free Hammer of Light from Divine Purpose counts as 5 Holy Power spent, Free Hammer of Light from Light's
    // Deliverance counts as 0 Holy Power spent
    if ( isFreeSLDPSpender )
    {
      num_hopo_spent = is_hammer_of_light_main ? hol_cost : 3.0;
      // 2024-09-10 If Hammer of Light is affected by Divine Purpose, it counts as 5 Holy Power spent.
      if ( p->bugs && p->specialization() == PALADIN_PROTECTION && p->buffs.divine_purpose->up() &&
           is_hammer_of_light_main )
      {
        num_hopo_spent = 5.0;
      }
    }

    // For Holy Power spending stuff, SotR with Instrument always counts as 3 Holy Power spent
    if (p->bugs && is_sotr && p->talents.instrument_of_the_divine->ok() && cost() > 3.0)
    {
      num_hopo_spent = 3.0;
    }


    ab::execute();

    if ( triggers_endless_gleam && is_divine_storm && ab::execute_state->n_targets > 1 )
    {
      auto tl = ab::target_list();

      int count = 0;
      for (auto& t : tl)
      {
        if ( ab::td( t )->dot.dawnlight->is_ticking() )
          count++;
      }
      // Not in spell data
      if (count > 1)
      {
        timespan_t ad =
            timespan_t::from_millis( p->talents.herald_of_the_sun.endless_gleam->effectN( 3 ).base_value() );
        for ( auto& t : tl )
        {
          if ( ab::td( t )->dot.dawnlight->is_ticking() )
            ab::td( t )->dot.dawnlight->adjust_duration( ad );
        }
      }
    }

    if ( triggers_righteous_cause && p->talents.righteous_cause->ok() && p->cooldowns.righteous_cause_icd->up() )
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
          p->buffs.righteous_cause->trigger();
          break;
        }
      }
    }

    if ( triggers_crusade_stacks && p->talents.crusade->ok() && p->buffs.avenging_wrath->up() )
    {
      int crusade_stacks = as<int>( num_hopo_spent );
      // Hammer of Light always gives 5 Stacks, even if it's free
      if ( is_hammer_of_light_main )
      {
        crusade_stacks = as<int>( hol_cost );
        // 2025-12-24 Fluttershy: Currently, if HoL is cast with less then 5 stacks, you gain 10 Crusade Stacks
        if ( p->bugs && p->buffs.avenging_wrath->stack() < 5 )
          crusade_stacks *= 2;
      }
      if ( crusade_stacks > 0 )
        p->buffs.avenging_wrath->trigger( as<int>( crusade_stacks ) );
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
    if ( is_divine_storm )
    {
      if ( p->buffs.empyrean_power->up() )
      {
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
        p->buffs.sentinel_decay->extend_duration( timespan_t::from_seconds( 1 ) );
      }
      // 2025-12-18 Instrument of the Divine talented extends Sentinel's decay by double the time, regardless of Holy Power spent.
      if (p->bugs && p->talents.instrument_of_the_divine->ok())
      {
        p->buffs.sentinel_decay->extend_duration( timespan_t::from_seconds( 1 ) );
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
    if ( triggers_divine_purpose && !( p->bugs && !is_hammer_of_light_main && is_hammer_of_light_cleave && p->specialization() == PALADIN_RETRIBUTION ) && p->talents.divine_purpose->ok() && this->rng().roll( p->talents.divine_purpose->effectN( 1 ).percent() ) )
    {
      p->buffs.divine_purpose->trigger();
      p->procs.divine_purpose->occur();
    }

    if ( p->talents.faiths_armor->ok() )
    {
      if ( ( p->specialization() == PALADIN_RETRIBUTION && is_wog ) ||
           ( p->specialization() != PALADIN_RETRIBUTION && is_sotr ) )
      {
        p->buffs.faiths_armor->trigger();
      }
    }
    if ( p->talents.lightsmith.divine_guidance->ok() )
    {
      p->buffs.lightsmith.divine_guidance->trigger();
    }
    if ( p->talents.lightsmith.blessed_assurance->ok() )
    {
      p->buffs.lightsmith.blessed_assurance->trigger();
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

struct delayed_execute_on_target_event_t : public event_t
{
  action_t* action;
  player_t* target;
  double amount;

  delayed_execute_on_target_event_t(paladin_t* p, action_t* a, player_t* t, double amount, timespan_t delay)
    : event_t( *p->sim, delay ), action(a), target(t), amount(amount)
  {
    assert( action->background );
  }
  const char* name() const override
  {
    return action->name();
  }

  void execute() override
  {
    if (!target->is_sleeping())
    {
      action->execute_on_target( target, amount );
    }
  }
};

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p );
  avenging_wrath_t( paladin_t* p, util::string_view options_str );
  void execute() override;
  action_state_t* new_state() override;
};
struct unrelenting_edict_t : public paladin_spell_t
{
  unrelenting_edict_t( paladin_t* p, util::string_view name );
  void do_execute( action_state_t* s );
};
struct hammer_and_anvil_t : public paladin_spell_t
{
  hammer_and_anvil_t( paladin_t* p, util::string_view name );
};
struct judgment_base_t : public paladin_melee_attack_t
{
  hammer_and_anvil_t* hammer_and_anvil;
  int judge_holy_power;
  int sw_holy_power;
  bool triggers_highlords_judgment;
  judgment_base_t( paladin_t* p, util::string_view name, const spell_data_t* s = spell_data_t::nil() );
  judgment_base_t( paladin_t* p, util::string_view name, util::string_view options_str, const spell_data_t* s = spell_data_t::nil() );
  void impact( action_state_t* s ) override;
  void execute() override;
};

struct hammer_of_wrath_t : public judgment_base_t
{
private:
  hammer_of_wrath_t* echo;

public:
  bool triggers_second_sunrise   = false;
  bool triggers_divine_resonance = false;
  unrelenting_edict_t* ue;
  hammer_of_wrath_t( paladin_t* p, util::string_view name, const spell_data_t* s = spell_data_t::nil() );
  hammer_of_wrath_t( paladin_t* p, util::string_view name, util::string_view options_str,
                     const spell_data_t* s = spell_data_t::nil() );
  void impact( action_state_t* s ) override;
  double composite_target_multiplier( player_t* target ) const override;
  bool action_ready() override;
  void execute() override;
};

struct judgment_t : public judgment_base_t
{
  bool triggered_hammer_and_anvil;
  unrelenting_edict_t* ue;

  judgment_t( paladin_t* p, util::string_view name, const spell_data_t* s = spell_data_t::nil() );
  judgment_t( paladin_t* p, util::string_view name, util::string_view options_str,
              const spell_data_t* s = spell_data_t::nil() );

  proc_types proc_type() const override;
  void execute() override;
  void impact(action_state_t* s) override;
  bool action_ready() override;
};


struct shield_of_the_righteous_buff_t : public buff_t
{
  shield_of_the_righteous_buff_t( paladin_t* p );
};
bool trigger_hammer_and_anvil( paladin_t* p, player_t* target, hammer_and_anvil_t* haa,
                               hammer_and_anvil_source haas );
struct golden_path_t : public paladin_heal_t
{
  golden_path_t( paladin_t* p );
};
struct consecration_tick_t : public paladin_spell_t
{
  golden_path_t* heal_tick;
  consecration_tick_t( util::string_view name, paladin_t* p );
  double action_multiplier() const override;
  double composite_target_multiplier( player_t* target ) const override;
};
}  // namespace paladin

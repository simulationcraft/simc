// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc/specialization.hpp"
#include "simulationcraft.hpp"
#include "player/player_talent_points.hpp"
#include "class_modules/apl/apl_warrior.hpp"

namespace
{  // UNNAMED NAMESPACE
// ==========================================================================
// Warrior
// To Do: Clean up green text
// Fury - Gathering Storm tick behavior - Fury needs 2 more
// Arms - 
// ==========================================================================

struct warrior_t;

// Finds an action with the given name. If no action exists, a new one will
// be created.
//
// Use this with secondary background actions to ensure the player only has
// one copy of the action.
// Shamelessly borrowed from the mage module
template <typename Action, typename Actor, typename... Args>
action_t* get_action( util::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}

struct warrior_td_t : public actor_target_data_t
{
  dot_t* dots_deep_wounds;
  dot_t* dots_gushing_wound;
  dot_t* dots_ravager;
  dot_t* dots_rend;
  dot_t* dots_thunderous_roar;
  buff_t* debuffs_colossus_smash;
  buff_t* debuffs_concussive_blows;
  buff_t* debuffs_executioners_precision;
  buff_t* debuffs_exploiter;
  buff_t* debuffs_fatal_mark;
  buff_t* debuffs_siegebreaker;
  buff_t* debuffs_demoralizing_shout;
  buff_t* debuffs_taunt;
  buff_t* debuffs_punish;
  buff_t* debuffs_callous_reprisal;
  bool hit_by_fresh_meat;

  warrior_t& warrior;
  warrior_td_t( player_t* target, warrior_t& p );

  void target_demise();
};

using data_t        = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

template <typename T_CONTAINER, typename T_DATA>
T_CONTAINER* get_data_entry( util::string_view name, std::vector<T_DATA*>& entries )
{
  for ( size_t i = 0; i < entries.size(); i++ )
  {
    if ( entries[ i ]->first == name )
    {
      return &( entries[ i ]->second );
    }
  }

  entries.push_back( new T_DATA( name, T_CONTAINER() ) );
  return &( entries.back()->second );
}

struct warrior_t : public player_t
{
public:
  event_t *rampage_driver;
  std::vector<attack_t*> rampage_attacks;
  bool non_dps_mechanics, warrior_fixed_time;
  int into_the_fray_friends;
  int never_surrender_percentage;

  auto_dispose<std::vector<data_t*> > cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*> > cd_waste_iter;

  // Active
  struct active_t
  {
    action_t* natures_fury;
    action_t* ancient_aftershock_pulse;
    action_t* kyrian_spear_attack;
    action_t* spear_of_bastion_attack;
    action_t* deep_wounds_ARMS;
    action_t* deep_wounds_PROT;
    action_t* fatality;
    action_t* signet_avatar;
    action_t* signet_bladestorm_a;
    action_t* signet_bladestorm_f;
    action_t* signet_recklessness;
    action_t* torment_avatar;
    action_t* torment_bladestorm;
    action_t* torment_odyns_fury;
    action_t* torment_recklessness;
    action_t* tough_as_nails;
    action_t* iron_fortress; // Prot azerite trait
    action_t* bastion_of_might_ip; // 0 rage IP from Bastion of Might azerite trait
  } active;

  // Buffs
  struct buffs_t
  {
    buff_t* ashen_juggernaut;
    buff_t* avatar;
    buff_t* ayalas_stone_heart;
    buff_t* bastion_of_might; // the mastery buff
    buff_t* bastion_of_might_vop; // bastion of might proc from VoP
    buff_t* battle_stance;
    buff_t* battering_ram;
    buff_t* berserker_rage;
    buff_t* berserker_stance;
    buff_t* bladestorm;
    buff_t* bloodcraze;
    buff_t* bounding_stride;
    buff_t* brace_for_impact;
    buff_t* charge_movement;
    buff_t* concussive_blows;
    buff_t* dancing_blades;
    buff_t* defensive_stance;
    buff_t* die_by_the_sword;
    buff_t* elysian_might;
    buff_t* enrage;
    buff_t* frenzy;
    buff_t* heroic_leap_movement;
    buff_t* hurricane;
    buff_t* hurricane_driver;
    buff_t* ignore_pain;
    buff_t* intercept_movement;
    buff_t* intervene_movement;
    buff_t* into_the_fray;
    buff_t* juggernaut;
    buff_t* juggernaut_prot;
    buff_t* last_stand;
    buff_t* meat_cleaver;
    buff_t* martial_prowess;
    buff_t* merciless_bonegrinder;
    buff_t* ravager;
    buff_t* recklessness;
    buff_t* reckless_abandon;
    buff_t* revenge;
    buff_t* shield_block;
    buff_t* shield_charge_movement;
    buff_t* shield_wall;
    buff_t* slaughtering_strikes_rb;
    buff_t* slaughtering_strikes_an;
    buff_t* spell_reflection;
    buff_t* sudden_death;
    buff_t* sweeping_strikes;
    buff_t* test_of_might_tracker;  // Used to track rage gain from test of might.
    buff_t* test_of_might;
    //buff_t* vengeance_revenge;
    //buff_t* vengeance_ignore_pain;
    buff_t* whirlwind;
    buff_t* wild_strikes;
    buff_t* tornados_eye;

    buff_t* dance_of_death_prot;
    buff_t* seeing_red;
    buff_t* seeing_red_tracking;
    buff_t* violent_outburst;

    // Legion Legendary
    buff_t* fujiedas_fury;
    buff_t* xavarics_magnum_opus;
    buff_t* sephuzs_secret;
    buff_t* in_for_the_kill;
    // Azerite Traits
    buff_t* bloodsport;
    buff_t* brace_for_impact_az;
    buff_t* crushing_assault;
    buff_t* gathering_storm;
    buff_t* infinite_fury;
    buff_t* pulverizing_blows;
    buff_t* striking_the_anvil;
    buff_t* trample_the_weak;
    // Covenant
    buff_t* conquerors_banner;
    buff_t* conquerors_frenzy;
    buff_t* conquerors_mastery;
    buff_t* elysian_might_legendary;
    // Conduits
    buff_t* ashen_juggernaut_conduit;
    buff_t* merciless_bonegrinder_conduit;
    buff_t* harrowing_punishment;
    buff_t* veterans_repute;
    buff_t* show_of_force;
    buff_t* unnerving_focus;
    // Tier
    buff_t* strike_vulnerabilities;
    buff_t* vanguards_determination;
    buff_t* crushing_advance;
    buff_t* merciless_assault;
    buff_t* earthen_tenacity;  // T30 Protection 4PC

    // Shadowland Legendary
    buff_t* battlelord;
    buff_t* cadence_of_fujieda;
    buff_t* will_of_the_berserker;

  } buff;

  struct rppm_t
  {
    real_ppm_t* fatal_mark;
    real_ppm_t* revenge;
  } rppm;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* avatar;
    cooldown_t* recklessness;
    cooldown_t* berserker_rage;
    cooldown_t* bladestorm;
    cooldown_t* bloodthirst;
    cooldown_t* bloodbath;
    cooldown_t* cleave;
    cooldown_t* colossus_smash;
    cooldown_t* demoralizing_shout;
    cooldown_t* thunderous_roar;
    cooldown_t* enraged_regeneration;
    cooldown_t* execute;
    cooldown_t* heroic_leap;
    cooldown_t* iron_fortress_icd;
    cooldown_t* impending_victory;
    cooldown_t* last_stand;
    cooldown_t* mortal_strike;
    cooldown_t* odyns_fury;
    cooldown_t* onslaught;
    cooldown_t* overpower;
    cooldown_t* pummel;
    cooldown_t* rage_from_auto_attack;
    cooldown_t* rage_from_crit_block;
    cooldown_t* rage_of_the_valarjar_icd;
    cooldown_t* raging_blow;
    cooldown_t* crushing_blow;
    cooldown_t* ravager;
    cooldown_t* shield_slam;
    cooldown_t* shield_wall;
    cooldown_t* shockwave;
    cooldown_t* skullsplitter;
    cooldown_t* storm_bolt;
    cooldown_t* tough_as_nails_icd;
    cooldown_t* thunder_clap;
    cooldown_t* warbreaker;
    cooldown_t* ancient_aftershock;
    cooldown_t* condemn;
    cooldown_t* conquerors_banner;
    cooldown_t* kyrian_spear;
    cooldown_t* spear_of_bastion;
    cooldown_t* signet_of_tormented_kings;
    cooldown_t* berserkers_torment;
    cooldown_t* cold_steel_hot_blood_icd;
  } cooldown;

  // Gains
  struct gains_t
  {
    gain_t* archavons_heavy_hand;
    gain_t* avatar;
    gain_t* avatar_torment;
    gain_t* avoided_attacks;
    gain_t* bloodsurge;
    gain_t* charge;
    gain_t* critical_block;
    gain_t* execute;
    gain_t* frothing_berserker;
    gain_t* meat_cleaver;
    gain_t* melee_crit;
    gain_t* melee_main_hand;
    gain_t* melee_off_hand;
    gain_t* raging_blow;
    gain_t* revenge;
    gain_t* shield_charge;
    gain_t* shield_slam;
    gain_t* spear_of_bastion;
    gain_t* strength_of_arms;
    gain_t* whirlwind;
    gain_t* booming_voice;
    gain_t* thunder_clap;
    gain_t* endless_rage;
    gain_t* collateral_damage;
    gain_t* instigate;
    gain_t* war_machine_demise;

    // Legendarys, Azerite, and Special Effects
    gain_t* execute_refund;
    gain_t* rage_from_damage_taken;
    gain_t* ravager;
    gain_t* ceannar_rage;
    gain_t* cold_steel_hot_blood;
    gain_t* valarjar_berserking;
    gain_t* lord_of_war;
    gain_t* simmering_rage;
    gain_t* memory_of_lucid_dreams;
    gain_t* conquerors_banner;
    gain_t* merciless_assault;
  } gain;

  // Spells
  struct spells_t
  {
    // Core Class Spells
    const spell_data_t* battle_shout;
    const spell_data_t* charge;
    const spell_data_t* execute;
    const spell_data_t* hamstring;
    const spell_data_t* heroic_throw;
    const spell_data_t* pummel;
    const spell_data_t* shield_block;
    const spell_data_t* shield_slam;
    const spell_data_t* slam;
    const spell_data_t* taunt;
    const spell_data_t* victory_rush;
    const spell_data_t* whirlwind;

    // Class Passives
    const spell_data_t* warrior_aura;

    // Extra Spells To Make Things Work

    const spell_data_t* colossus_smash_debuff;
    const spell_data_t* executioners_precision_debuff;
    const spell_data_t* fatal_mark_debuff;
    const spell_data_t* concussive_blows_debuff;
    const spell_data_t* recklessness_buff;
    const spell_data_t* shield_block_buff;
    const spell_data_t* siegebreaker_debuff;
    const spell_data_t* whirlwind_buff;
    const spell_data_t* aftershock_duration;
    const spell_data_t* shield_wall;
  } spell;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* deep_wounds_ARMS;  // Arms
    const spell_data_t* critical_block;    // Protection
    const spell_data_t* unshackled_fury;   // Fury
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* delayed_auto_attack;
    proc_t* glory;
    proc_t* tactician;
  } proc;

  // Spec Passives
  struct spec_t
  {
    // Arms Spells
    const spell_data_t* arms_warrior;
    const spell_data_t* seasoned_soldier;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* deep_wounds_ARMS;

    // Fury Spells
    const spell_data_t* fury_warrior;
    const spell_data_t* enrage;
    const spell_data_t* execute;
    const spell_data_t* whirlwind;

    const spell_data_t* bloodbath; // BT replacement
    const spell_data_t* crushing_blow; // RB replacement

    // Protection Spells
    const spell_data_t* protection_warrior;
    const spell_data_t* devastate;
    const spell_data_t* riposte;
    const spell_data_t* thunder_clap_prot_hidden;
    const spell_data_t* vanguard;

    const spell_data_t* deep_wounds_PROT;
    const spell_data_t* revenge_trigger;
    const spell_data_t* shield_block_2;

  } spec;

  // Talents
  struct talents_t
  {
    struct class_talents_t
    {
      player_talent_t battle_stance;
      player_talent_t berserker_stance;
      player_talent_t defensive_stance;

      player_talent_t berserker_rage;
      player_talent_t impending_victory;
      player_talent_t war_machine;
      player_talent_t intervene;
      player_talent_t rallying_cry;

      player_talent_t berserker_shout;
      player_talent_t piercing_howl;
      player_talent_t fast_footwork;
      player_talent_t spell_reflection;
      player_talent_t leeching_strikes;
      player_talent_t inspiring_presence;
      player_talent_t second_wind;

      player_talent_t frothing_berserker;
      player_talent_t heroic_leap;
      player_talent_t intimidating_shout;
      player_talent_t thunder_clap;
      player_talent_t furious_blows;

      player_talent_t wrecking_throw;
      player_talent_t shattering_throw;
      player_talent_t crushing_force;
      player_talent_t pain_and_gain;
      player_talent_t cacophonous_roar;
      player_talent_t menace;
      player_talent_t storm_bolt;
      player_talent_t overwhelming_rage;
      player_talent_t barbaric_training;
      player_talent_t concussive_blows;

      player_talent_t reinforced_plates;
      player_talent_t bounding_stride;
      player_talent_t blood_and_thunder;
      player_talent_t crackling_thunder;
      player_talent_t sidearm;

      player_talent_t honed_reflexes;
      player_talent_t bitter_immunity;
      player_talent_t double_time;
      player_talent_t titanic_throw;
      player_talent_t seismic_reverberation;

      player_talent_t armored_to_the_teeth;
      player_talent_t wild_strikes;
      player_talent_t one_handed_weapon_specialization;
      player_talent_t two_handed_weapon_specialization;
      player_talent_t dual_wield_specialization;
      player_talent_t cruel_strikes;
      player_talent_t endurance_training;

      player_talent_t avatar;
      player_talent_t thunderous_roar;
      player_talent_t spear_of_bastion;
      player_talent_t shockwave;

      player_talent_t immovable_object;
      player_talent_t unstoppable_force;
      player_talent_t blademasters_torment;
      player_talent_t warlords_torment;
      player_talent_t berserkers_torment;
      player_talent_t titans_torment;
      player_talent_t uproar;
      player_talent_t thunderous_words;
      player_talent_t piercing_verdict;
      player_talent_t elysian_might;
      player_talent_t rumbling_earth;
      player_talent_t sonic_boom;

    } warrior;

    struct arms_talents_t
    {
      player_talent_t mortal_strike;

      player_talent_t overpower;

      player_talent_t martial_prowess;
      player_talent_t die_by_the_sword;
      player_talent_t improved_execute;

      player_talent_t improved_overpower;
      player_talent_t bloodsurge;
      player_talent_t fueled_by_violence;
      player_talent_t storm_wall;
      player_talent_t sudden_death;
      player_talent_t fervor_of_battle;

      player_talent_t tactician;
      player_talent_t colossus_smash;
      player_talent_t impale;

      player_talent_t skullsplitter;
      player_talent_t rend;
      player_talent_t exhilarating_blows;
      player_talent_t anger_management;
      player_talent_t massacre;
      player_talent_t cleave;

      player_talent_t tide_of_blood;
      player_talent_t bloodborne;
      player_talent_t dreadnaught;
      player_talent_t in_for_the_kill;
      player_talent_t test_of_might;
      player_talent_t blunt_instruments;
      player_talent_t warbreaker;
      player_talent_t storm_of_swords;
      player_talent_t collateral_damage;
      player_talent_t reaping_swings;

      player_talent_t deft_experience;
      player_talent_t valor_in_victory;
      player_talent_t critical_thinking;

      player_talent_t bloodletting;
      player_talent_t battlelord;
      player_talent_t bladestorm;
      player_talent_t sharpened_blades;
      player_talent_t executioners_precision;

      player_talent_t fatality;
      player_talent_t dance_of_death;
      player_talent_t unhinged;
      player_talent_t hurricane;
      player_talent_t merciless_bonegrinder;
      player_talent_t juggernaut;

      player_talent_t improved_slam;
      player_talent_t improved_sweeping_strikes;
      player_talent_t strength_of_arms;
      player_talent_t spiteful_serenity;

    } arms;

    struct fury_talents_t
    {
      player_talent_t bloodthirst;

      player_talent_t raging_blow;

      player_talent_t improved_enrage;
      player_talent_t enraged_regeneration;
      player_talent_t improved_execute;

      player_talent_t improved_bloodthirst;
      player_talent_t fresh_meat;
      player_talent_t warpaint;
      player_talent_t invigorating_fury;
      player_talent_t sudden_death;
      player_talent_t improved_raging_blow;

      player_talent_t focus_in_chaos;
      player_talent_t rampage;
      player_talent_t cruelty;

      player_talent_t single_minded_fury;
      player_talent_t cold_steel_hot_blood;
      player_talent_t vicious_contempt;
      player_talent_t frenzy;
      player_talent_t hack_and_slash;
      player_talent_t slaughtering_strikes;
      player_talent_t ashen_juggernaut;
      player_talent_t improved_whirlwind;
      player_talent_t wrath_and_fury;

      player_talent_t frenzied_flurry;
      player_talent_t bloodborne;
      player_talent_t bloodcraze;
      player_talent_t recklessness;
      player_talent_t massacre;
      player_talent_t meat_cleaver;
      player_talent_t raging_armaments;

      player_talent_t deft_experience;
      player_talent_t swift_strikes;
      player_talent_t critical_thinking;
      player_talent_t storm_of_swords;
      player_talent_t odyns_fury;
      player_talent_t anger_management;
      player_talent_t reckless_abandon;
      player_talent_t onslaught;
      player_talent_t ravager;

      player_talent_t annihilator;
      player_talent_t dancing_blades;
      player_talent_t titanic_rage;
      player_talent_t unbridled_ferocity;
      player_talent_t depths_of_insanity;
      player_talent_t tenderize;
      player_talent_t storm_of_steel;
      player_talent_t hurricane;

    } fury;

    struct protection_talents_t
    {
      player_talent_t ignore_pain;

      player_talent_t revenge;

      player_talent_t demoralizing_shout;
      player_talent_t devastator;
      player_talent_t last_stand;

      player_talent_t improved_heroic_throw;
      player_talent_t best_served_cold;
      player_talent_t strategist;
      player_talent_t brace_for_impact;
      player_talent_t unnerving_focus;

      player_talent_t challenging_shout;
      player_talent_t instigate;
      player_talent_t rend;
      player_talent_t bloodsurge;
      player_talent_t fueled_by_violence;
      player_talent_t brutal_vitality;

      player_talent_t disrupting_shout;
      player_talent_t show_of_force;
      player_talent_t sudden_death;
      player_talent_t thunderlord;
      player_talent_t shield_wall;
      player_talent_t bolster;
      player_talent_t tough_as_nails;
      player_talent_t spell_block;
      player_talent_t bloodborne;

      player_talent_t heavy_repercussions;
      player_talent_t into_the_fray;
      player_talent_t enduring_defenses;
      player_talent_t massacre;
      player_talent_t anger_management;
      player_talent_t defenders_aegis;
      player_talent_t impenetrable_wall;
      player_talent_t punish;
      player_talent_t juggernaut;

      player_talent_t focused_vigor;
      player_talent_t shield_specialization;
      player_talent_t enduring_alacrity;

      player_talent_t shield_charge;
      player_talent_t booming_voice;
      player_talent_t indomitable;
      player_talent_t violent_outburst;
      player_talent_t ravager;

      player_talent_t battering_ram;
      player_talent_t champions_bulwark;
      player_talent_t battle_scarred_veteran;
      player_talent_t dance_of_death;
      player_talent_t storm_of_steel;

    } protection;

    struct shared_talents_t
    {
      player_talent_t hurricane;
      player_talent_t ravager;
      player_talent_t bloodsurge;

    } shared;

  } talents;

  struct tier_set_t
  {
    const spell_data_t* t29_arms_2pc;
    const spell_data_t* t29_arms_4pc;
    const spell_data_t* t29_fury_2pc;
    const spell_data_t* t29_fury_4pc;
    const spell_data_t* t29_prot_2pc;
    const spell_data_t* t29_prot_4pc;
    const spell_data_t* t30_arms_2pc;
    const spell_data_t* t30_arms_4pc;
    const spell_data_t* t30_fury_2pc;
    const spell_data_t* t30_fury_4pc;
    const spell_data_t* t30_prot_2pc;
    const spell_data_t* t30_prot_4pc;
  } tier_set;

  struct legendary_t
  {
    const spell_data_t* sephuzs_secret;
    const spell_data_t* valarjar_berserkers;
    const spell_data_t* ceannar_charger;
    const spell_data_t* archavons_heavy_hand;
    const spell_data_t* kazzalax_fujiedas_fury;
    const spell_data_t* prydaz_xavarics_magnum_opus;
    const spell_data_t* najentuss_vertebrae;
    const spell_data_t* ayalas_stone_heart;
    const spell_data_t* raging_fury;
    const spell_data_t* the_great_storms_eye;
    const spell_data_t* the_wall;
    const spell_data_t* thunderlord;
    const spell_data_t* reprisal;

    legendary_t()
      : sephuzs_secret( spell_data_t::not_found() ),
        valarjar_berserkers( spell_data_t::not_found() ),
        ceannar_charger( spell_data_t::not_found() ),
        archavons_heavy_hand( spell_data_t::not_found() ),
        kazzalax_fujiedas_fury( spell_data_t::not_found() ),
        prydaz_xavarics_magnum_opus( spell_data_t::not_found() ),
        najentuss_vertebrae( spell_data_t::not_found() ),
        ayalas_stone_heart( spell_data_t::not_found() ),
        raging_fury( spell_data_t::not_found() ),
        the_great_storms_eye( spell_data_t::not_found() ),
        the_wall( spell_data_t::not_found() ),
        thunderlord( spell_data_t::not_found() ),
        reprisal(spell_data_t::not_found() )

    {
    }
    // General
    item_runeforge_t leaper;
    item_runeforge_t misshapen_mirror;
    item_runeforge_t seismic_reverberation;
    item_runeforge_t signet_of_tormented_kings;
    // Covenant
    item_runeforge_t elysian_might;
    item_runeforge_t glory;
    item_runeforge_t natures_fury;
    item_runeforge_t sinful_surge;
    double glory_counter = 0.0; // Track Glory rage spent
    // Arms
    item_runeforge_t battlelord;
    item_runeforge_t enduring_blow;
    item_runeforge_t exploiter;
    item_runeforge_t unhinged;
    // Fury
    item_runeforge_t cadence_of_fujieda;
    item_runeforge_t deathmaker;
    item_runeforge_t reckless_defense;
    item_runeforge_t will_of_the_berserker;

  } legendary;

  // Covenant Powers
  struct covenant_t
  {
    const spell_data_t* ancient_aftershock;
    const spell_data_t* condemn;
    const spell_data_t* condemn_driver;
    const spell_data_t* conquerors_banner;
    const spell_data_t* kyrian_spear;
  } covenant;

  // Conduits
  struct conduit_t
  {
    conduit_data_t ashen_juggernaut;
    conduit_data_t crash_the_ramparts;
    conduit_data_t depths_of_insanity;
    conduit_data_t destructive_reverberations;
    conduit_data_t hack_and_slash;
    conduit_data_t harrowing_punishment;
    conduit_data_t merciless_bonegrinder;
    conduit_data_t mortal_combo;
    conduit_data_t piercing_verdict;
    conduit_data_t veterans_repute;
    conduit_data_t vicious_contempt;
    conduit_data_t show_of_force;
    conduit_data_t unnerving_focus;
  } conduit;

  // Azerite traits
  struct
  {
    // All
    azerite_power_t breach;
    azerite_power_t moment_of_glory;
    azerite_power_t bury_the_hatchet;
    // Prot
    azerite_power_t iron_fortress;
    azerite_power_t deafening_crash;
    azerite_power_t callous_reprisal;
    azerite_power_t brace_for_impact;
    azerite_power_t bloodsport;
    azerite_power_t bastion_of_might;
    // Arms
    azerite_power_t test_of_might;
    azerite_power_t seismic_wave;
    azerite_power_t lord_of_war;
    azerite_power_t gathering_storm;
    azerite_power_t crushing_assault;
    azerite_power_t striking_the_anvil;
    // fury
    azerite_power_t trample_the_weak;
    azerite_power_t simmering_rage;
    azerite_power_t reckless_flurry;
    azerite_power_t pulverizing_blows;
    azerite_power_t infinite_fury;
    azerite_power_t cold_steel_hot_blood;
    azerite_power_t unbridled_ferocity;

    // Essences
    azerite_essence_t memory_of_lucid_dreams;
    azerite_essence_t vision_of_perfection;
    double vision_of_perfection_percentage;
  } azerite;

  struct azerite_spells_t
  {
    // General
    const spell_data_t* memory_of_lucid_dreams;
  } azerite_spells;

  struct warrior_options_t
  {
    double memory_of_lucid_dreams_proc_chance = 0.15;
  } options;

  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  warrior_t( sim_t* sim, util::string_view name, race_e r = RACE_NIGHT_ELF )
    : player_t( sim, WARRIOR, name, r ),
      rampage_driver( nullptr ),
      rampage_attacks( 0 ),
      active(),
      buff(),
      cooldown(),
      gain(),
      spell(),
      mastery(),
      proc(),
      spec(),
      talents(),
      legendary(),
      azerite()
  {
    non_dps_mechanics =
        true;  // When set to false, disables stuff that isn't important, such as second wind, bloodthirst heal, etc.
    warrior_fixed_time    = true;
    into_the_fray_friends = -1;
    never_surrender_percentage = 70;

    resource_regeneration = regen_type::DISABLED;

    talent_points->register_validity_fn( [this]( const spell_data_t* spell ) {
      // Soul of the Battlelord
      if ( find_item_by_id( 151650 ) )
      {
        switch ( specialization() )
        {
          case WARRIOR_FURY:
            return spell->id() == 206315;  // Massacre
          case WARRIOR_ARMS:
            return spell->id() == 152278;  // Anger Management
          case WARRIOR_PROTECTION:
            return spell->id() == 202572;  // Vengeance
          default:
            // This shouldn't happen
            break;
        }
      }
      return false;
    } );
  }

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_position() override;
  void init_procs() override;
  void init_resources( bool ) override;
  void arise() override;
  void combat_begin() override;
  void init_rng() override;
  double composite_attribute( attribute_e attr ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_rating_multiplier( rating_e rating ) const override;
  double composite_player_multiplier( school_e school ) const override;
  // double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_target_crit_chance( player_t* target ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_melee_speed() const override;
  double composite_melee_haste() const override;
  double composite_armor_multiplier() const override;
  double composite_bonus_armor() const override;
  double composite_base_armor_multiplier() const override;
  double composite_block() const override;
  double composite_block_reduction( action_state_t* s ) const override;
  double composite_parry_rating() const override;
  double composite_parry() const override;
  double composite_melee_expertise( const weapon_t* ) const override;
  double composite_attack_power_multiplier() const override;
  // double composite_melee_attack_power() const override;
  double composite_mastery() const override;
  double composite_damage_versatility() const override;
  double composite_crit_block() const override;
  double composite_crit_avoidance() const override;
  // double composite_melee_speed() const override;
  double composite_melee_crit_chance() const override;
  double composite_melee_crit_rating() const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double composite_leech() const override;
  double resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  void teleport( double yards, timespan_t duration ) override;
  void trigger_movement( double distance, movement_direction_type direction ) override;
  void interrupt() override;
  void reset() override;
  void moving() override;
  void create_options() override;
  std::string create_profile( save_e type ) override;
  void invalidate_cache( cache_e ) override;
  double temporary_movement_modifier() const override;
  void vision_of_perfection_proc() override;
  //void apply_affecting_auras(action_t& action) override;

  void trigger_tide_of_blood( dot_t* dot );

  void apl_default();
  void init_action_list() override;

  action_t* create_action( util::string_view name, util::string_view options ) override;
  void activate() override;
  resource_e primary_resource() const override
  {
    return RESOURCE_RAGE;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  // void assess_damage_imminent_pre_absorb( school_e, result_amount_type, action_state_t* s ) override;
  // void assess_damage_imminent( school_e, result_amount_type, action_state_t* s ) override;
  void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;
  void apply_affecting_auras( action_t& action ) override;

  void datacollection_begin() override;
  void datacollection_end() override;

  target_specific_t<warrior_td_t> target_data;

  const warrior_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  warrior_td_t* get_target_data( player_t* target ) const override
  {
    warrior_td_t*& td = target_data[ target ];

    if ( !td )
    {
      td = new warrior_td_t( target, const_cast<warrior_t&>( *this ) );
    }
    return td;
  }

  // Secondary Action Tracking - From Rogue
private:
  std::vector<action_t*> background_actions;

public:
  template <typename T, typename... Ts>
  T* get_background_action( util::string_view n, Ts&&... args )
  {
    auto it = range::find( background_actions, n, &action_t::name_str );
    if ( it != background_actions.cend() )
    {
      return dynamic_cast<T*>( *it );
    }

    auto action = new T( n, this, std::forward<Ts>( args )... );
    action->background = true;
    background_actions.push_back( action );
    return action;
  }


  void enrage()
  {
    buff.enrage->trigger();
    if ( legendary.ceannar_charger->found() )
    {
      resource_gain( RESOURCE_RAGE,
                     legendary.ceannar_charger->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ),
                     gain.ceannar_rage );
    }
  }
};

namespace
{  // UNNAMED NAMESPACE
// Template for common warrior action code. See priest_action_t.
template <class Base>
struct warrior_action_t : public Base
{
  struct affected_by_t
  {
    // mastery/buff damage increase.
    bool fury_mastery_direct, fury_mastery_dot, arms_mastery,
    siegebreaker;
    // talents
    bool avatar, sweeping_strikes, booming_voice, bloodcraze, executioners_precision,
    ashen_juggernaut, recklessness, slaughtering_strikes, colossus_smash,
    merciless_bonegrinder, juggernaut, juggernaut_prot;
    // tier
    bool t29_arms_4pc;
    bool t29_prot_2pc;
    bool t30_arms_2pc;
    bool t30_arms_4pc;
    bool t30_fury_4pc;
    // azerite & conduit
    bool crushing_assault, ashen_juggernaut_conduit;

    affected_by_t()
      : fury_mastery_direct( false ),
        fury_mastery_dot( false ),
        arms_mastery( false ),
        siegebreaker( false ),
        avatar( false ),
        sweeping_strikes( false ),
        booming_voice( false ),
        bloodcraze( false ),
        executioners_precision( false ),
        ashen_juggernaut( false ),
        recklessness( false ),
        slaughtering_strikes( false ),
        colossus_smash( false ),
        merciless_bonegrinder( false ),
        juggernaut( false ),
        juggernaut_prot( false ),
        t29_arms_4pc ( false ),
        t29_prot_2pc( false ),
        t30_arms_2pc( false ),
        t30_arms_4pc( false ),
        t30_fury_4pc( false ),
        crushing_assault( false ),
        ashen_juggernaut_conduit( false )
    {
    }
  } affected_by;

  bool usable_while_channeling;
  double tactician_per_rage;

private:
  using ab = Base;  // action base, eg. spell_t
public:
  using base_t = warrior_action_t<Base>;
  bool track_cd_waste;
  simple_sample_data_with_min_max_t *cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;
  bool initialized;
  warrior_action_t( util::string_view n, warrior_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      usable_while_channeling( false ),
      tactician_per_rage( 0 ),
      track_cd_waste( s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero() ),
      cd_wasted_exec( nullptr ),
      cd_wasted_cumulative( nullptr ),
      cd_wasted_iter( nullptr ),
      initialized( false )
  {
    ab::may_crit = true;
    if ( p()->talents.arms.deft_experience->ok() )
    {
      tactician_per_rage += ( ( player->talents.arms.tactician->effectN( 1 ).percent() +
                                player->talents.arms.deft_experience->effectN( 2 ).percent() ) / 100 );
    }
    else
    {
      tactician_per_rage += ( player->talents.arms.tactician->effectN( 1 ).percent() / 100 );
    }
  }

  void init() override
  {
    if ( initialized )
      return;

    ab::init();

    if ( track_cd_waste )
    {
      cd_wasted_exec = get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p()->cd_waste_exec );
      cd_wasted_cumulative =
          get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p()->cd_waste_cumulative );
      cd_wasted_iter = get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, p()->cd_waste_iter );
    }

    //if ( ab::data().affected_by( p()->spec.fury_warrior->effectN( 1 ) ) )
      //ab::base_dd_multiplier *= 1.0 + p()->spec.fury_warrior->effectN( 1 ).percent();
    //if ( ab::data().affected_by( p()->spec.fury_warrior->effectN( 2 ) ) )
      //ab::base_td_multiplier *= 1.0 + p()->spec.fury_warrior->effectN( 2 ).percent();

    //if ( ab::data().affected_by( p()->spec.arms_warrior->effectN( 2 ) ) )
      //ab::base_dd_multiplier *= 1.0 + p()->spec.arms_warrior->effectN( 2 ).percent();
    //if ( ab::data().affected_by( p()->spec.arms_warrior->effectN( 3 ) ) )
      //ab::base_td_multiplier *= 1.0 + p()->spec.arms_warrior->effectN( 3 ).percent();

    //if ( ab::data().affected_by( p()->spec.prot_warrior->effectN( 1 ) ) )
      //ab::base_dd_multiplier *= 1.0 + p()->spec.prot_warrior->effectN( 1 ).percent();
    //if ( ab::data().affected_by( p()->spec.prot_warrior->effectN( 2 ) ) )
      //ab::base_td_multiplier *= 1.0 + p()->spec.prot_warrior->effectN( 2 ).percent();

    if ( ab::data().affected_by( p()->spell.warrior_aura->effectN( 1 ) ) )
      ab::cooldown->hasted = true;
    if ( ab::data().affected_by( p()->spell.warrior_aura->effectN( 2 ) ) )
      ab::gcd_type = gcd_haste_type::ATTACK_HASTE;

    // Affecting Passive Conduits
    ab::apply_affecting_conduit( p()->conduit.ashen_juggernaut );
    ab::apply_affecting_conduit( p()->conduit.crash_the_ramparts );
    ab::apply_affecting_conduit( p()->conduit.depths_of_insanity );
    ab::apply_affecting_conduit( p()->conduit.destructive_reverberations );
    ab::apply_affecting_conduit( p()->conduit.piercing_verdict );

    // passive set bonuses

    // passive talents
    ab::apply_affecting_aura( p()->talents.arms.bloodborne );
    ab::apply_affecting_aura( p()->talents.arms.bloodletting );
    ab::apply_affecting_aura( p()->talents.arms.blunt_instruments ); // damage only
    ab::apply_affecting_aura( p()->talents.arms.impale );
    ab::apply_affecting_aura( p()->talents.arms.improved_overpower );
    ab::apply_affecting_aura( p()->talents.arms.improved_execute );
    ab::apply_affecting_aura( p()->talents.arms.improved_slam );
    ab::apply_affecting_aura( p()->talents.arms.reaping_swings );
    ab::apply_affecting_aura( p()->talents.arms.sharpened_blades );
    ab::apply_affecting_aura( p()->talents.arms.storm_of_swords );
    ab::apply_affecting_aura( p()->talents.arms.strength_of_arms ); // rage generation in spell
    ab::apply_affecting_aura( p()->talents.arms.valor_in_victory );
    ab::apply_affecting_aura( p()->talents.fury.bloodborne );
    ab::apply_affecting_aura( p()->talents.fury.critical_thinking );
    ab::apply_affecting_aura( p()->talents.fury.deft_experience );
    ab::apply_affecting_aura( p()->talents.fury.improved_bloodthirst );
    ab::apply_affecting_aura( p()->talents.fury.improved_raging_blow );
    ab::apply_affecting_aura( p()->talents.fury.raging_armaments );
    ab::apply_affecting_aura( p()->talents.fury.storm_of_steel );
    ab::apply_affecting_aura( p()->talents.fury.storm_of_swords ); // rage generation in spell
    ab::apply_affecting_aura( p()->talents.protection.storm_of_steel );
    ab::apply_affecting_aura( p()->talents.protection.bloodborne );
    ab::apply_affecting_aura( p()->talents.protection.defenders_aegis );
    ab::apply_affecting_aura( p()->talents.protection.battering_ram );
    ab::apply_affecting_aura( p()->talents.warrior.barbaric_training );
    ab::apply_affecting_aura( p()->talents.warrior.concussive_blows );
    ab::apply_affecting_aura( p()->talents.warrior.cruel_strikes );
    ab::apply_affecting_aura( p()->talents.warrior.crushing_force ); // crit portion not active
    ab::apply_affecting_aura( p()->talents.warrior.piercing_verdict );
    ab::apply_affecting_aura( p()->talents.warrior.honed_reflexes );
    ab::apply_affecting_aura( p()->talents.warrior.sonic_boom );
    ab::apply_affecting_aura( p()->talents.warrior.thunderous_words );
    ab::apply_affecting_aura( p()->talents.warrior.uproar );

   // set bonus
    ab::apply_affecting_aura( p()->tier_set.t29_arms_2pc );  
    ab::apply_affecting_aura( p()->tier_set.t29_fury_2pc );  
    ab::apply_affecting_aura( p()->tier_set.t30_fury_2pc );



    affected_by.ashen_juggernaut_conduit = ab::data().affected_by( p()->conduit.ashen_juggernaut->effectN( 1 ).trigger()->effectN( 1 ) );
    affected_by.slaughtering_strikes     = ab::data().affected_by( p()->find_spell( 393931 )->effectN( 1 ) );
    affected_by.ashen_juggernaut         = ab::data().affected_by( p()->talents.fury.ashen_juggernaut->effectN( 1 ).trigger()->effectN( 1 ) );
    affected_by.juggernaut               = ab::data().affected_by( p()->talents.arms.juggernaut->effectN( 1 ).trigger()->effectN( 1 ) );
    affected_by.juggernaut_prot          = ab::data().affected_by( p()->talents.protection.juggernaut->effectN( 1 ).trigger()->effectN( 1 ) );
    affected_by.bloodcraze               = ab::data().affected_by( p()->talents.fury.bloodcraze->effectN( 1 ).trigger()->effectN( 1 ) );
    affected_by.sweeping_strikes         = ab::data().affected_by( p()->spec.sweeping_strikes->effectN( 1 ) );
    affected_by.fury_mastery_direct      = ab::data().affected_by( p()->mastery.unshackled_fury->effectN( 1 ) );
    affected_by.fury_mastery_dot         = ab::data().affected_by( p()->mastery.unshackled_fury->effectN( 2 ) );
    affected_by.arms_mastery             = ab::data().affected_by( p()->mastery.deep_wounds_ARMS -> effectN( 3 ).trigger()->effectN( 2 ) );
    affected_by.booming_voice            = ab::data().affected_by( p()->talents.protection.demoralizing_shout->effectN( 3 ) );
    affected_by.colossus_smash           = ab::data().affected_by( p()->spell.colossus_smash_debuff->effectN( 1 ) );
    affected_by.executioners_precision   = ab::data().affected_by( p()->spell.executioners_precision_debuff->effectN( 1 ) );
    affected_by.merciless_bonegrinder    = ab::data().affected_by( p()->find_spell( 383316 )->effectN( 1 ) );
    affected_by.siegebreaker             = ab::data().affected_by( p()->spell.siegebreaker_debuff->effectN( 1 ) );
    affected_by.avatar                   = ab::data().affected_by( p()->talents.warrior.avatar->effectN( 1 ) );
    affected_by.recklessness             = ab::data().affected_by( p()->spell.recklessness_buff->effectN( 1 ) );
    affected_by.t29_arms_4pc             = ab::data().affected_by( p()->find_spell( 394173 )->effectN( 1 ) );
    affected_by.t29_prot_2pc             = ab::data().affected_by( p()->find_spell( 394056 )->effectN( 1 ) );
    affected_by.t30_arms_2pc             = ab::data().affected_by( p()->find_spell( 262115 )->effectN( 5 ) );
    affected_by.t30_arms_4pc             = ab::data().affected_by( p()->find_spell( 410138 )->effectN( 1 ) );
    affected_by.t30_fury_4pc             = ab::data().affected_by( p()->find_spell( 409983 )->effectN( 2 ) );

    initialized = true;
  }


  warrior_t* p()
  {
    return debug_cast<warrior_t*>( ab::player );
  }

  const warrior_t* p() const
  {
    return debug_cast<warrior_t*>( ab::player );
  }

  warrior_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  virtual double tactician_cost() const
  {
    if ( ab::sim->log )
    {
      ab::sim->out_debug.printf(
          "Rage used to calculate tactician chance from ability %s: %4.4f, actual rage used: %4.4f", ab::name(),
          ab::cost(), base_t::cost() );
    }
    return ab::cost();
  }

  int n_targets() const override
  {
    if ( affected_by.sweeping_strikes && p()->buff.sweeping_strikes->check() )
    {
      return static_cast<int>( 1 + p()->spec.sweeping_strikes->effectN( 1 ).base_value() );
    }

    return ab::n_targets();
  }

  double cost() const override
  {
    double c = ab::cost();

    return c;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    warrior_td_t* td = p()->get_target_data( target );

    if ( affected_by.colossus_smash && td->debuffs_colossus_smash->check() )
    {
      m *= 1.0 + ( td->debuffs_colossus_smash->value() );
    }

    if ( affected_by.executioners_precision && td->debuffs_executioners_precision->check() )
    {
      m *= 1.0 + ( td->debuffs_executioners_precision->stack_value() );
    }

    if ( affected_by.arms_mastery && td->dots_deep_wounds->is_ticking() )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.siegebreaker && td->debuffs_siegebreaker->check() )
    {
      m *= 1.0 + ( td->debuffs_siegebreaker->value() );
    }

    if ( td -> debuffs_demoralizing_shout -> up() && p()->talents.protection.booming_voice->ok() &&
         affected_by.booming_voice )
    {
      m *= 1.0 + p()->talents.protection.booming_voice->effectN( 2 ).percent();
    }

    if ( td->debuffs_concussive_blows->check() )
    {
      m *= 1.0 + ( td->debuffs_concussive_blows->value() );
    }

    return m;
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* target ) const override
  {
    double tcdbm = ab::composite_target_crit_damage_bonus_multiplier( target );

    warrior_td_t* td = p()->get_target_data( target );

    // needs adjusting - this is actually a TOTAL crit damage increase, not extra - current implementation does not support
    if ( p()->sets->has_set_bonus( WARRIOR_ARMS, T30, B2 ) && td->dots_deep_wounds->is_ticking() &&
         affected_by.t30_arms_2pc )
    {
      tcdbm *= 1.0 + ( p()->find_spell( 262115 )->effectN( 5 ).percent() );
    }

    return tcdbm;
  }

  double composite_crit_chance() const override
  {
    double c = ab::composite_crit_chance();

    if( affected_by.ashen_juggernaut )
    {
      c += p()->buff.ashen_juggernaut->stack_value();
    }
    if ( affected_by.ashen_juggernaut_conduit )
    {
      c += p()->buff.ashen_juggernaut_conduit->stack_value();
    }

    if( affected_by.t30_fury_4pc )
    {
      c += p()->buff.merciless_assault->stack() * p()->find_spell( 409983 )->effectN( 3 ).percent();
    }

    if( affected_by.bloodcraze )
    {
      c += p()->buff.bloodcraze->stack_value();
    }

    if ( affected_by.recklessness && p()->buff.recklessness->up() )
    {
      c += p()->buff.recklessness->check_value();
    }

    return c;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dm = ab::composite_da_multiplier( s );

    if ( affected_by.fury_mastery_direct && p()->buff.enrage->up() )
    {
      dm *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.avatar && p()->buff.avatar->up() )
    {
      //dm *= 1.0 + p()->buff.avatar->data().effectN( 1 ).percent() + p()->talents.arms.spiteful_serenity->effectN( 1 ).percent;
      dm *= 1.0 + p()->buff.avatar->check_value();
    }

    if ( affected_by.sweeping_strikes && s->chain_target > 0 )
    {
      dm *= p()->spec.sweeping_strikes->effectN( 2 ).percent();
    }

    if ( affected_by.slaughtering_strikes && p()->buff.slaughtering_strikes_an->up() )
    {
      dm *= 1.0 + p()->buff.slaughtering_strikes_an->stack_value();
    }

    if ( affected_by.slaughtering_strikes && p()->buff.slaughtering_strikes_rb->up() )
    {
      dm *= 1.0 + p()->buff.slaughtering_strikes_rb->stack_value();
    }

    if ( affected_by.merciless_bonegrinder && p()->buff.merciless_bonegrinder->up() )
    {
      dm *= 1.0 + p()->buff.merciless_bonegrinder->check_value();
    }

    if ( affected_by.juggernaut && p()->buff.juggernaut->up() )
    {
      dm *= 1.0 + p()->buff.juggernaut->stack_value();
    }

    if ( affected_by.juggernaut_prot && p()->buff.juggernaut_prot->up() )
    {
      dm *= 1.0 + p()->buff.juggernaut_prot->stack_value();
    }

    if ( affected_by.t29_arms_4pc && p()->buff.strike_vulnerabilities->up() )
    {
      dm *= 1.0 + p()->buff.strike_vulnerabilities->check_value();
    }

    if ( affected_by.t29_prot_2pc && p()->buff.vanguards_determination->up() )
    {
      dm *= 1.0 + p()->buff.vanguards_determination->check_value();
    }

    if ( affected_by.t30_arms_4pc && p()->buff.crushing_advance->up() )
    {
      dm *= 1.0 + p()->buff.crushing_advance->stack_value();
    }

    if ( affected_by.t30_fury_4pc && p()->buff.merciless_assault->up() )
    {
      dm *= 1.0 + p()->buff.merciless_assault->stack_value();
    }

    return dm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double tm = ab::composite_ta_multiplier( s );

    if ( affected_by.fury_mastery_dot && p()->buff.enrage->up() )
    {
      tm *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.avatar && p()->buff.avatar->up() )
    {
      double percent_increase = p()->buff.avatar->check_value();

      tm *= 1.0 + percent_increase;
    }

    if ( affected_by.t29_arms_4pc && p()->buff.strike_vulnerabilities->up() )
    {
      tm *= 1.0 + p()->buff.strike_vulnerabilities->check_value();
    }

    if ( affected_by.t29_prot_2pc && p()->buff.vanguards_determination->up() )
    {
      tm *= 1.0 + p()->buff.vanguards_determination->check_value();
    }

    return tm;
  }

  void execute() override
  {
    ab::execute();
  }

  bool ready() override
  {
    if ( !ab::ready() )
      return false;

    // -1 melee range implies that the ability can be used at any distance from the target.
    if ( p()->current.distance_to_move > ab::range && ab::range != -1 )
      return false;

    if ( ( p()->channeling || p()->buff.bladestorm->check() ) && !usable_while_channeling )
      return false;

    return true;
  }

  void update_ready( timespan_t cd ) override
  {
    if ( cd_wasted_exec &&
         ( cd > timespan_t::zero() || ( cd <= timespan_t::zero() && ab::cooldown->duration > timespan_t::zero() ) ) &&
         ab::cooldown->current_charge == ab::cooldown->charges && ab::cooldown->last_charged > timespan_t::zero() &&
         ab::cooldown->last_charged < ab::sim->current_time() )
    {
      double time_ = ( ab::sim->current_time() - ab::cooldown->last_charged ).total_seconds();
      if ( p()->sim->debug )
      {
        p()->sim->out_debug.printf( "%s %s cooldown waste tracking waste=%.3f exec_time=%.3f", p()->name(), ab::name(),
                                    time_, ab::time_to_execute.total_seconds() );
      }
      time_ -= ab::time_to_execute.total_seconds();

      if ( time_ > 0 )
      {
        cd_wasted_exec->add( time_ );
        cd_wasted_iter->add( time_ );
      }
    }
    ab::update_ready( cd );
  }

  bool usable_moving() const override
  {  // All warrior abilities are usable while moving, the issue is being in range.
    return true;
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::sim->log )
    {
      ab::sim->out_debug.printf(
          "Strength: %4.4f, AP: %4.4f, Crit: %4.4f%%, Crit Dmg Mult: %4.4f,  Mastery: %4.4f%%, Haste: %4.4f%%, "
          "Versatility: %4.4f%%, Bonus Armor: %4.4f, Tick Multiplier: %4.4f, Direct Multiplier: %4.4f, Action "
          "Multiplier: %4.4f",
          p()->cache.strength(), p()->cache.attack_power() * p()->composite_attack_power_multiplier(),
          p()->cache.attack_crit_chance() * 100, p()->composite_player_critical_damage_multiplier( s ),
          p()->cache.mastery_value() * 100, ( 1 / p()->cache.attack_haste() - 1 ) * 100,
          p()->cache.damage_versatility() * 100, p()->cache.bonus_armor(), s->composite_ta_multiplier(),
          s->composite_da_multiplier(), s->action->action_multiplier() );
    }

    if( p()->sets->has_set_bonus( WARRIOR_PROTECTION, T29, B4 ) )
    {
      double new_ip = s -> result_amount;

      new_ip *= p()->sets->set( WARRIOR_PROTECTION, T29, B4 )->effectN( 1 ).percent();

      double previous_ip = p() -> buff.ignore_pain -> current_value;

      // IP is capped to 30% of max health
      double ip_max_health_cap = p() -> max_health() * 0.3;

      if ( previous_ip + new_ip > ip_max_health_cap )
      {
        new_ip = ip_max_health_cap;
      }

      if ( new_ip > 0.0 )
      {
        p()->buff.ignore_pain->trigger( 1, new_ip );
      }
    }
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    if ( ab::sim->log )
    {
      ab::sim->out_debug.printf(
          "Strength: %4.4f, AP: %4.4f, Crit: %4.4f%%, Crit Dmg Mult: %4.4f,  Mastery: %4.4f%%, Haste: %4.4f%%, "
          "Versatility: %4.4f%%, Bonus Armor: %4.4f, Tick Multiplier: %4.4f, Direct Multiplier: %4.4f, Action "
          "Multiplier: %4.4f",
          p()->cache.strength(), p()->cache.attack_power() * p()->composite_attack_power_multiplier(),
          p()->cache.attack_crit_chance() * 100, p()->composite_player_critical_damage_multiplier( d->state ),
          p()->cache.mastery_value() * 100, ( 1 / p()->cache.attack_haste() - 1 ) * 100,
          p()->cache.damage_versatility() * 100, p()->cache.bonus_armor(), d->state->composite_ta_multiplier(),
          d->state->composite_da_multiplier(), d->state->action->action_ta_multiplier() );
    }
  }

  void consume_resource() override
  {
    //td = find_target_data( target );

    if ( tactician_per_rage )
    {
      tactician();
    }

    ab::consume_resource();

    double rage = ab::last_resource_cost;

    if ( p()->buff.test_of_might_tracker->check() )
      p()->buff.test_of_might_tracker->current_value +=
          rage;  // Uses rage cost before anything makes it cheaper.

    if ( p()->talents.arms.anger_management->ok() || p()->talents.fury.anger_management->ok() || p()->talents.protection.anger_management->ok() )
    {
      anger_management( rage );
    }
    if ( ab::result_is_miss( ab::execute_state->result ) && rage > 0 && !ab::aoe )
    {
      p()->resource_gain( RESOURCE_RAGE, rage * 0.8, p()->gain.avoided_attacks );
    }

    // Memory of Lucid Dreams Essence
    if ( p()->azerite.memory_of_lucid_dreams.enabled() && ab::last_resource_cost > 0 )
    {
      resource_e cr = ab::current_resource();
      if ( cr == RESOURCE_RAGE )
      {
        if ( p()->rng().roll( p()->options.memory_of_lucid_dreams_proc_chance ) )
        {
          // Gains are rounded up to the nearest whole value, which can be seen with the Lucid Dreams active up
          const double amount =
              ceil( ab::last_resource_cost * p()->azerite_spells.memory_of_lucid_dreams->effectN( 1 ).percent() );
          p()->resource_gain( cr, amount, p()->gain.memory_of_lucid_dreams );

          if ( p()->azerite.memory_of_lucid_dreams.rank() >= 3 )
          {
          p()->buffs.lucid_dreams->trigger();
          }
        }
      }
    }

    if ( ab::current_resource() == RESOURCE_RAGE && ab::last_resource_cost > 0 )
    {
      if ( p()->legendary.glory->ok() && p()->buff.conquerors_banner->check() )
      {
        p()->legendary.glory_counter += ab::last_resource_cost;
        if ( p()->legendary.glory_counter > ( p()->specialization() >= WARRIOR_PROTECTION ? 10 : 25 ) )
        {
          double times_over_threshold = floor( p()->legendary.glory_counter / (p()->specialization() == WARRIOR_PROTECTION ? 10 : 25) );
          p()->buff.conquerors_banner->extend_duration( p(), timespan_t::from_millis( p()->legendary.glory->effectN( 3 ).base_value() ) * times_over_threshold );
          p()->buff.conquerors_mastery->extend_duration( p(), timespan_t::from_millis( p()->legendary.glory->effectN( 3 ).base_value() ) * times_over_threshold );
          p()->buff.veterans_repute->extend_duration( p(), timespan_t::from_millis( p()->legendary.glory->effectN( 3 ).base_value() ) * times_over_threshold );
          p()->legendary.glory_counter -= (p()->specialization() == WARRIOR_PROTECTION ? 10 : 25) * times_over_threshold ;
          p()->proc.glory->occur();
        }
      }
    }

    // Protection Warrior Violent Outburst Seeing Red Tracking
    if ( p()->specialization() == WARRIOR_PROTECTION && p()->talents.protection.violent_outburst.ok() && rage > 0 )
    {
      // Trigger the buff if this is the first rage consumption of the iteration
      if ( !p()->buff.seeing_red_tracking->check() )
      {
        p()->buff.seeing_red_tracking->trigger();
      }

      double original_value = p()->buff.seeing_red_tracking->current_value;
      double rage_per_stack = p()->buff.seeing_red_tracking->data().effectN( 1 ).base_value();
      p()->buff.seeing_red_tracking->current_value += rage;
      p()->sim->print_debug( "{} increments seeing_red_tracking by {}. Old={} New={}", p()->name(), rage,
                             original_value, p()->buff.seeing_red_tracking->current_value );

      while ( p()->buff.seeing_red_tracking->current_value >= rage_per_stack )
      {
        p()->buff.seeing_red_tracking->current_value -= rage_per_stack;
        p()->sim->print_debug(
            "{} reaches seeing_red_tracking threshold, triggering seeing_red buff. New seeing_red_tracking value is {}",
            p()->name(), p()->buff.seeing_red_tracking->current_value );

        p()->buff.seeing_red->trigger();

        if( p()->buff.seeing_red->at_max_stacks() )
        {
          p()->buff.seeing_red->expire();
          p()->buff.violent_outburst->trigger();
        }

      }
    }
  }

  virtual void tactician()
  {
    double tact_rage = tactician_cost();  // Tactician resets based on cost before things make it cost less.
    double tactician_chance = tactician_per_rage;

    if ( ab::rng().roll( tactician_chance * tact_rage ) )
    {
      p()->cooldown.overpower->reset( true );
      p()->proc.tactician->occur();
      if ( p()->azerite.striking_the_anvil.ok() )
      {
        p()->buff.striking_the_anvil->trigger();
      }
    }
  }

  void anger_management( double rage )
  {
    if ( rage > 0 )
    {
      // Anger management reduces some cds by 1 sec per n rage spent (different per spec)
      double cd_time_reduction = -1.0 * rage;

      if ( p()->specialization() == WARRIOR_FURY )
      {
        cd_time_reduction /= p()->talents.fury.anger_management->effectN( 3 ).base_value();
        p()->cooldown.recklessness->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.ravager->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }

      else if ( p()->specialization() == WARRIOR_ARMS )
      {
        cd_time_reduction /= p()->talents.arms.anger_management->effectN( 1 ).base_value();  
                                                                                                                                                                       
        p()->cooldown.colossus_smash->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.warbreaker->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.bladestorm->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }

      else if ( p()->specialization() == WARRIOR_PROTECTION )
      {
        cd_time_reduction /= p()->talents.protection.anger_management->effectN( 2 ).base_value();
        p()->cooldown.shield_wall->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.avatar->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }
    }
  }
};

struct warrior_heal_t : public warrior_action_t<heal_t>
{  // Main Warrior Heal Class
  warrior_heal_t( util::string_view n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
    may_crit = tick_may_crit = hasted_ticks = false;
    target                                  = p;
  }
};

struct warrior_spell_t : public warrior_action_t<spell_t>
{  // Main Warrior Spell Class - Used for spells that deal no damage, usually buffs.
  warrior_spell_t( util::string_view n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }
};

struct warrior_attack_t : public warrior_action_t<melee_attack_t>
{  // Main Warrior Attack Class
  warrior_attack_t( util::string_view n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, p, s )
  {
    special = true;
  }

  void execute() override
  {
    base_t::execute();
    if ( !special )  // Procs below only trigger on special attacks, not autos
      return;

    if ( ( p()->talents.arms.sudden_death->ok() || p()->talents.fury.sudden_death->ok() || p()->talents.protection.sudden_death->ok() ) && p()->buff.sudden_death->trigger() )
    {
      p()->cooldown.execute->reset( true );
      p()->cooldown.condemn->reset( true );
    }

    if ( p()->buff.ayalas_stone_heart->trigger() )
    {
      p()->cooldown.execute->reset( true );
      p()->cooldown.condemn->reset( true );
    }

    if ( affected_by.crushing_assault )
    {
      p()->buff.crushing_assault->expire();
    }
    p()->buff.crushing_assault->trigger();
  }

  player_t* select_random_target() const
  {
    if ( sim->distance_targeting_enabled )
    {
      std::vector<player_t*> targets;
      range::for_each( sim->target_non_sleeping_list, [&targets, this]( player_t* t ) {
        if ( p()->get_player_distance( *t ) <= radius + t->combat_reach )
        {
          targets.push_back( t );
        }
      } );

      auto random_idx = rng().range( targets.size() );
      return !targets.empty() ? targets[ random_idx ] : nullptr;
    }
    else
    {
      auto random_idx = rng().range( size_t(), sim->target_non_sleeping_list.size() );
      return sim->target_non_sleeping_list[ random_idx ];
    }
  }
};

// Devastate ================================================================

struct devastate_t : public warrior_attack_t
{
  double shield_slam_reset;
  devastate_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "devastate", p, p->spec.devastate ),
      shield_slam_reset( p->talents.protection.strategist->effectN( 1 ).percent() )
  {
    weapon        = &( p->main_hand_weapon );
    impact_action = p->active.deep_wounds_PROT;
    parse_options( options_str );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) && rng().roll( shield_slam_reset ) )
    {
      p()->cooldown.shield_slam->reset( true );
    }

    if ( p() -> talents.protection.instigate.ok() )
      p() -> resource_gain( RESOURCE_RAGE, p() -> talents.protection.instigate -> effectN( 2 ).resource( RESOURCE_RAGE ), p() -> gain.instigate );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p() -> talents.protection.instigate -> effectN( 1 ).percent();

    return am;
  }

  bool ready() override
  {
    if ( !p()->has_shield_equipped() || p()->talents.protection.devastator->ok() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Melee Attack =============================================================

struct reckless_flurry_t : warrior_attack_t
{
  reckless_flurry_t( warrior_t* p )
    : warrior_attack_t( "reckless_flurry", p, p->find_spell( 283810 ) )
  {
    background  = true;
    base_dd_min = base_dd_max = p->azerite.reckless_flurry.value( 2 );
  }
};

struct annihilator_t : warrior_attack_t
{
  annihilator_t( warrior_t* p ) : warrior_attack_t( "annihilator", p, p->find_spell( 383915 ) )
  {
    background  = true;
    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.fury.cruelty->ok() && p()->buff.enrage->check() )
    {
      am *= 1.0 + p()->talents.fury.cruelty->effectN( 2 ).percent();
    }

    return am;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.fury.slaughtering_strikes->ok() )
    {
      p()->buff.slaughtering_strikes_an->trigger();
    }
  }
};


struct sidearm_t : warrior_attack_t
{
  sidearm_t( warrior_t* p ) : warrior_attack_t( "sidearm", p, p->find_spell( 384391 ) )
  {
    background = true;
  }
};

struct devastator_t : warrior_attack_t
{
  double shield_slam_reset;

  devastator_t( warrior_t* p ) : warrior_attack_t( "devastator", p, p->talents.protection.devastator->effectN( 1 ).trigger() ),
      shield_slam_reset( p->talents.protection.devastator->effectN( 2 ).percent() )
  {
    background = true;
    impact_action = p->active.deep_wounds_PROT;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( rng().roll( shield_slam_reset ) )
    {
      p() -> cooldown.shield_slam -> reset( true );
    }

    if ( p() -> talents.protection.instigate.ok() )
      p() -> resource_gain( RESOURCE_RAGE, p() -> talents.protection.instigate -> effectN( 4 ).resource( RESOURCE_RAGE ), p() -> gain.instigate );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p() -> talents.protection.instigate -> effectN( 3 ).percent();

    return am;
  }
};

struct melee_t : public warrior_attack_t
{
  warrior_attack_t* annihilator;
  warrior_attack_t* reckless_flurry;
  warrior_attack_t* sidearm;
  bool mh_lost_melee_contact, oh_lost_melee_contact;
  double base_rage_generation, arms_rage_multiplier, fury_rage_multiplier, seasoned_soldier_crit_mult;
  double sidearm_chance, enrage_chance;
  devastator_t* devastator;
  melee_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, spell_data_t::nil() ),
      annihilator( nullptr ),
      reckless_flurry( nullptr ),
      sidearm( nullptr),
      mh_lost_melee_contact( true ),
      oh_lost_melee_contact( true ),
      // arms and fury multipliers are both 1, adjusted by the spec scaling auras (x4 for Arms and x1 for Fury)
      base_rage_generation( 1.75 ),
      arms_rage_multiplier( 4.00 ),
      fury_rage_multiplier( 1.00 ),
      seasoned_soldier_crit_mult( p->spec.seasoned_soldier->effectN( 1 ).percent() ),
      sidearm_chance( p->talents.warrior.sidearm->proc_chance() ),
      enrage_chance( p->talents.fury.frenzied_flurry->proc_chance() ),
      devastator( nullptr )
  {
    background = repeating = may_glance = usable_while_channeling = true;
    allow_class_ability_procs = not_a_proc = true;
    special           = false;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = timespan_t::zero();
    weapon_multiplier = 1.0;
    if ( p->dual_wield() )
    {
      base_hit -= 0.19;
    }
    if ( p->talents.protection.devastator->ok() )
    {
      devastator = new devastator_t( p );
      add_child( devastator );
    }
    if ( p->azerite.reckless_flurry.ok() )
    {
      reckless_flurry = new reckless_flurry_t( p );
    }
    if ( p->talents.fury.annihilator->ok() )
    {
      annihilator = new annihilator_t( p );
    }
    if ( p->talents.warrior.sidearm->ok() )
    {
      sidearm = new sidearm_t( p );
    }
  }

  void init() override
  {
    warrior_attack_t::init();
    affected_by.fury_mastery_direct = p()->mastery.unshackled_fury->ok();
    affected_by.arms_mastery        = p()->mastery.deep_wounds_ARMS->ok();
    affected_by.colossus_smash      = p()->talents.arms.colossus_smash->ok();
    affected_by.siegebreaker        = p()->spell.siegebreaker_debuff->ok();
    affected_by.booming_voice       = p()->talents.protection.booming_voice->ok();
    affected_by.avatar = true;
    affected_by.t29_arms_4pc = true;
  }

  void reset() override
  {
    warrior_attack_t::reset();
    mh_lost_melee_contact = oh_lost_melee_contact = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = warrior_attack_t::execute_time();
    if ( weapon->slot == SLOT_MAIN_HAND && mh_lost_melee_contact )
    {
      return timespan_t::zero();  // If contact is lost, the attack is instant.
    }
    else if ( weapon->slot == SLOT_OFF_HAND && oh_lost_melee_contact )  // Also used for the first attack.
    {
      return timespan_t::zero();
    }
    else
    {
      return t;
    }
  }

  void schedule_execute( action_state_t* s ) override
  {
    warrior_attack_t::schedule_execute( s );
    if ( weapon->slot == SLOT_MAIN_HAND )
    {
      mh_lost_melee_contact = false;
    }
    else if ( weapon->slot == SLOT_OFF_HAND )
    {
      oh_lost_melee_contact = false;
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.berserker_stance->check_stack_value();

    am *= 1.0 + p()->buff.dancing_blades->check_value();

    am *= 1.0 + p()->buff.battering_ram->check_value();

    return am;
  }

  double composite_hit() const override
  {
    if ( p()->talents.fury.focus_in_chaos.ok() && p()->buff.enrage->check() )
    {
      return 1.0;
    }
    return warrior_attack_t::composite_hit();
  }

  void execute() override
  {
    if ( p()->current.distance_to_move > 5 )
    {  // Cancel autoattacks, auto_attack_t will restart them when we're back in range.
      if ( weapon->slot == SLOT_MAIN_HAND )
      {
        p()->proc.delayed_auto_attack->occur();
        mh_lost_melee_contact = true;
        player->main_hand_attack->cancel();
      }
      else
      {
        p()->proc.delayed_auto_attack->occur();
        oh_lost_melee_contact = true;
        player->off_hand_attack->cancel();
      }
    }
    else
    {
      warrior_attack_t::execute();

      if ( result_is_hit( execute_state->result ) )
      {
        if ( p()->main_hand_weapon.group() == WEAPON_1H && p()->off_hand_weapon.group() == WEAPON_1H 
          && p()->talents.fury.frenzied_flurry->ok() && rng().roll( enrage_chance ) )
        {
          p()->enrage();
        }
        if ( p()->talents.protection.devastator->ok() )
        {
          devastator->target = execute_state->target;
          devastator->schedule_execute();
        }
        trigger_rage_gain( execute_state );
        if ( p()->azerite.reckless_flurry.ok() )  // does this need a spec = fury check?
        {
          p()->cooldown.recklessness->adjust(
              ( -1 * p()->azerite.reckless_flurry.spell_ref().effectN( 1 ).time_value() ) );
        }
        if ( p()->buff.infinite_fury->check() )  // does this need a spec = fury check?
        {
          ( p()->buff.infinite_fury->trigger() ); // auto attacks refresh the buff but do not trigger it initially
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( reckless_flurry && result_is_hit( s->result ) )
    {
      reckless_flurry->set_target( s->target );
      reckless_flurry->execute();
    }

    if ( annihilator && result_is_hit( s->result ) )
    {
      annihilator->set_target( s->target );
      annihilator->execute();
    }

    if ( sidearm && result_is_hit( s->result ) && rng().roll( sidearm_chance ) )
    {
      sidearm->set_target( s->target );
      sidearm->execute();
    }

    if ( p()->talents.warrior.wild_strikes->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.wild_strikes->trigger();
    }
  }

  void trigger_rage_gain( const action_state_t* s )
  {
    double rage_gain = weapon->swing_time.total_seconds() * base_rage_generation;

    if ( p()->specialization() == WARRIOR_ARMS )
    {
      rage_gain *= arms_rage_multiplier;
      if ( s->result == RESULT_CRIT )
      {
        rage_gain *= 1.0 + seasoned_soldier_crit_mult;
      }
    }
    else if ( p() -> specialization() == WARRIOR_FURY )
    {
      rage_gain *= fury_rage_multiplier;
      if ( weapon->slot == SLOT_OFF_HAND )
      {
        rage_gain *= 0.5;
      }
    }
    else
    {
      // Protection generates a static 2 rage per successful auto attack landed
      rage_gain = 2.0;
    }
    rage_gain *= 1.0 + p()->talents.warrior.war_machine->effectN( 2 ).percent();

    rage_gain = util::round( rage_gain, 1 );

    if ( p()->specialization() == WARRIOR_ARMS && s->result == RESULT_CRIT )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_gain, p()->gain.melee_crit );
    }
    else
    {
      p()->resource_gain( RESOURCE_RAGE, rage_gain,
                          weapon->slot == SLOT_OFF_HAND ? p()->gain.melee_off_hand : p()->gain.melee_main_hand );
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public warrior_attack_t
{
  auto_attack_t( warrior_t* p, util::string_view options_str ) : warrior_attack_t( "auto_attack", p )
  {
    parse_options( options_str );
    assert( p->main_hand_weapon.type != WEAPON_NONE );
    ignore_false_positive = usable_while_channeling = true;
    range = 5;

    if ( p->main_hand_weapon.type == WEAPON_2H && p->has_shield_equipped() && p->specialization() != WARRIOR_FURY )
    {
      sim->errorf( "Player %s is using a 2 hander + shield while specced as Protection or Arms. Disabling autoattacks.",
                   name() );
    }
    else
    {
      p->main_hand_attack                    = new melee_t( "auto_attack_mh", p );
      p->main_hand_attack->weapon            = &( p->main_hand_weapon );
      p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;
    }

    if ( p->off_hand_weapon.type != WEAPON_NONE && p->specialization() == WARRIOR_FURY )
    {
      p->off_hand_attack                    = new melee_t( "auto_attack_oh", p );
      p->off_hand_attack->weapon            = &( p->off_hand_weapon );
      p->off_hand_attack->base_execute_time = p->off_hand_weapon.swing_time;
      p->off_hand_attack->id                = 1;
    }
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    if ( p()->main_hand_attack->execute_event == nullptr )
    {
      p()->main_hand_attack->schedule_execute();
    }
    if ( p()->off_hand_attack && p()->off_hand_attack->execute_event == nullptr )
    {
      p()->off_hand_attack->schedule_execute();
    }
  }

  bool ready() override
  {
    bool ready = warrior_attack_t::ready();
    if ( ready )  // Range check
    {
      if ( p()->main_hand_attack->execute_event == nullptr )
      {
        return ready;
      }
      if ( p()->off_hand_attack && p()->off_hand_attack->execute_event == nullptr )
      {
        // Don't check for execute_event if we don't have an offhand.
        return ready;
      }
    }
    return false;
  }
};

// Rend ==============================================================

struct rend_dot_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  rend_dot_t( warrior_t* p )
    : warrior_attack_t( "rend", p, p->find_spell( 388539 ) ),
      bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
      rage_from_bloodsurge(
          p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }
  }
};

struct rend_t : public warrior_attack_t
{
  warrior_attack_t* rend_dot;
  rend_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "rend", p, p->talents.arms.rend ),
      rend_dot( nullptr )
  {
    parse_options( options_str );
    tick_may_crit = true;
    hasted_ticks  = true;
    rend_dot      = new rend_dot_t( p );
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    rend_dot->set_target( s->target );
    rend_dot->execute();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Mortal Strike ============================================================
struct crushing_advance_t : warrior_attack_t
{
  crushing_advance_t( warrior_t* p ) : warrior_attack_t( "crushing_advance", p, p->find_spell( 411703 ) )
  {
    aoe                 = -1;
    reduced_aoe_targets = 5.0;
    background          = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->buff.crushing_advance->stack() > 1 )
    {
      am *= 1.0 + ( p()->buff.crushing_advance->stack() - 1 ) * 0.5;
    }
    // gains a 50% damage bonus for each stack beyond the first
    // 1 stack = base damage, 2 stack = +50%, 3 stack = +100%
    // Not in spell data

    return am;
  }
};

struct mortal_strike_unhinged_t : public warrior_attack_t
{
  mortal_strike_unhinged_t* mortal_combo_strike;
  bool from_mortal_combo;
  double enduring_blow_chance;
  double mortal_combo_chance;
  warrior_attack_t* crushing_advance;
  mortal_strike_unhinged_t( warrior_t* p, util::string_view name, bool mortal_combo = false )
    : warrior_attack_t( name, p, p->talents.arms.mortal_strike ), mortal_combo_strike( nullptr ),
      from_mortal_combo( mortal_combo ),
      enduring_blow_chance( p->legendary.enduring_blow->proc_chance() ),
      mortal_combo_chance( mortal_combo ? 0.0 : p->conduit.mortal_combo.percent() ),
      crushing_advance( nullptr )
  {
    background = true;
    if ( p->conduit.mortal_combo->ok() && !from_mortal_combo )
    {
      mortal_combo_strike                      = new mortal_strike_unhinged_t( p, "Mortal Combo", true );
      mortal_combo_strike->background          = true;
    }
    cooldown->duration = timespan_t::zero();
    weapon             = &( p->main_hand_weapon );

    if ( p->tier_set.t30_arms_4pc->ok() )
    {
      crushing_advance = new crushing_advance_t( p );
      add_child( crushing_advance );
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.martial_prowess->check_stack_value();

    return am;
  }

  double cost() const override
  {
    return 0;
  }

  double tactician_cost() const override
  {
    return 0;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( !sim->overrides.mortal_wounds && execute_state->target->debuffs.mortal_wounds )
      {
        execute_state->target->debuffs.mortal_wounds->trigger();
      }
    }

    if ( crushing_advance && p()->buff.crushing_advance->check() )
    {
      // crushing_advance->set_target( s->target );
      crushing_advance->execute();
    }

    p()->buff.crushing_advance->expire();
    p()->buff.martial_prowess->expire();

    warrior_td_t* td = this->td( execute_state->target );
    td->debuffs_exploiter->expire();
    td->debuffs_executioners_precision->expire();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( p()->legendary.enduring_blow->ok() && ( result_is_hit( s->result ) ) && rng().roll( enduring_blow_chance ) )
    {
      if ( td( s->target )->debuffs_colossus_smash->up() )
      {
        td( s-> target )->debuffs_colossus_smash->extend_duration( p(), timespan_t::from_millis( p()->legendary.enduring_blow->effectN( 1 ).base_value() ) );
      }
      else
      {
        td( s-> target )->debuffs_colossus_smash->trigger( timespan_t::from_millis( p()->legendary.enduring_blow->effectN( 1 ).base_value() ) );
      }
    }
    if ( mortal_combo_strike && rng().roll( mortal_combo_chance && !p()->talents.arms.exhilarating_blows->ok() ) ) // talent overrides conduit
    {
      mortal_combo_strike->execute();
    }
    if ( p()->talents.arms.fatality->ok() && p()->rppm.fatal_mark->trigger() && target->health_percentage() > 30 )
    {  // does this eat RPPM when switching from low -> high health target?
      td( s->target )->debuffs_fatal_mark->trigger();
    }
    if ( p()->tier_set.t29_arms_4pc->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.strike_vulnerabilities->trigger();
    }
  }

  // Override actor spec verification so it's usable with Unhinged legendary for non-Arms
  bool verify_actor_spec() const override
  {
    if ( p() -> legendary.unhinged -> ok() )
      return true;
    return warrior_attack_t::verify_actor_spec();
  }
};

struct mortal_strike_t : public warrior_attack_t
{
  mortal_strike_t* mortal_combo_strike;
  bool from_mortal_combo;
  double cost_rage;
  double enduring_blow_chance;
  double mortal_combo_chance;
  double exhilarating_blows_chance;
  double frothing_berserker_chance;
  double rage_from_frothing_berserker;
  warrior_attack_t* crushing_advance;
  warrior_attack_t* rend_dot;
  mortal_strike_t( warrior_t* p, util::string_view options_str, bool mortal_combo = false )
    : warrior_attack_t( "mortal_strike", p, p->talents.arms.mortal_strike ), mortal_combo_strike( nullptr ),
      from_mortal_combo( mortal_combo ),
      enduring_blow_chance( p->legendary.enduring_blow->proc_chance() ),
      mortal_combo_chance( mortal_combo ? 0.0 : p->conduit.mortal_combo.percent() ),
      exhilarating_blows_chance( p->talents.arms.exhilarating_blows->proc_chance() ),
      frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
      rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).base_value() / 100.0 ),
      crushing_advance( nullptr ),
      rend_dot( nullptr )
  {
    parse_options( options_str );

    if ( p->conduit.mortal_combo->ok() && !from_mortal_combo )
    {
      mortal_combo_strike                      = new mortal_strike_t( p, options_str, true );
      mortal_combo_strike->background          = true;
    }

    weapon           = &( p->main_hand_weapon );
    cooldown->hasted = true;  // Doesn't show up in spelldata for some reason.
    impact_action    = p->active.deep_wounds_ARMS;
    rend_dot = new rend_dot_t( p );

    if ( p->tier_set.t30_arms_4pc->ok() )
    {
      crushing_advance = new crushing_advance_t( p );
      add_child( crushing_advance );
    }
  }

  double cost() const override
  {
    if ( ( !from_mortal_combo ) && p()->buff.battlelord->check() )
    {
        return 20;
    }

    if ( from_mortal_combo )
      return 0;
    return warrior_attack_t::cost();
  }

  double tactician_cost() const override
  {
    if ( from_mortal_combo )
      return 0;
    return warrior_attack_t::cost();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.martial_prowess->check_stack_value();

    return am;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warrior_attack_t::composite_target_multiplier( target );

    m *= 1.0 + td( target )->debuffs_exploiter->check_value();

    return m;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    cost_rage = last_resource_cost;
    if ( p()->talents.warrior.frothing_berserker->ok() && rng().roll( frothing_berserker_chance ) )
    {
      p()->resource_gain(RESOURCE_RAGE, last_resource_cost * rage_from_frothing_berserker, p()->gain.frothing_berserker);
    }
    if ( result_is_hit( execute_state->result ) )
    {
      if ( !sim->overrides.mortal_wounds && execute_state->target->debuffs.mortal_wounds )
      {
        execute_state->target->debuffs.mortal_wounds->trigger();
      }
    }
    if ( !from_mortal_combo )
    {
      p()->buff.battlelord->expire();
    }
    if ( p()->talents.arms.exhilarating_blows->ok() && rng().roll( exhilarating_blows_chance ) )
    {
      p()->cooldown.mortal_strike->reset( true );
      p()->cooldown.cleave->reset( true );
    }
    if ( crushing_advance && p()->buff.crushing_advance->check() )
    {
      //crushing_advance->set_target( s->target );
      crushing_advance->execute();
    }

    p()->buff.crushing_advance->expire();
    p()->buff.martial_prowess->expire();

    warrior_td_t* td = this->td( execute_state->target );
    td->debuffs_exploiter->expire();
    td->debuffs_executioners_precision->expire();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( p()->legendary.enduring_blow->ok() && ( result_is_hit( s->result ) ) && rng().roll( enduring_blow_chance ) )
    {
      if ( td( s->target )->debuffs_colossus_smash->up() )
      {
        td( s-> target )->debuffs_colossus_smash->extend_duration( p(), timespan_t::from_millis( p()->legendary.enduring_blow->effectN( 1 ).base_value() ) );
      }
      else
      {
        td( s-> target )->debuffs_colossus_smash->trigger( timespan_t::from_millis( p()->legendary.enduring_blow->effectN( 1 ).base_value() ) );
      }
    }
    if ( mortal_combo_strike && rng().roll( mortal_combo_chance && !p()->talents.arms.exhilarating_blows->ok() ) ) // talent overrides conduit
    {
      mortal_combo_strike->execute();
    }
    if ( p()->talents.arms.fatality->ok() && p()->rppm.fatal_mark->trigger() && target->health_percentage() > 30 )
    { // does this eat RPPM when switching from low -> high health target?
      td( s->target )->debuffs_fatal_mark->trigger();
    }
    if ( p()->tier_set.t29_arms_4pc->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.strike_vulnerabilities->trigger();
    }
    if ( p()->talents.arms.bloodletting->ok() && ( target->health_percentage() < 35 ) )
    {
      rend_dot->set_target( s->target );
      rend_dot->execute();
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Bladestorm ===============================================================

struct bladestorm_tick_t : public warrior_attack_t
{
  bladestorm_tick_t( warrior_t* p, util::string_view name, const spell_data_t* spell )
    : warrior_attack_t( name, p, spell )

  {
    dual = true;
    aoe = -1;
    reduced_aoe_targets = 8.0;
    background = true;
    if ( p->specialization() == WARRIOR_ARMS )
    {
      impact_action = p->active.deep_wounds_ARMS;
    }
  }
    static const spell_data_t* get_correct_spell_data( warrior_t* p )
    {
      if (p->specialization() == WARRIOR_FURY)
      {
          if (p->legendary.signet_of_tormented_kings.enabled())
          {
            return p->find_spell( 46924 ) -> effectN( 1 ).trigger();
          }
      }
      else
      {
        return p->talents.arms.bladestorm->effectN( 1 ).trigger();
      }
    }

};

struct bladestorm_t : public warrior_attack_t
{
  attack_t *bladestorm_mh, *bladestorm_oh;
  mortal_strike_unhinged_t* mortal_strike;
  double signet_chance;
  bool signet_triggered;

  bladestorm_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell, bool signet_triggered = false )
    : warrior_attack_t( n, p, spell ),
    bladestorm_mh( new bladestorm_tick_t( p, fmt::format( "{}_mh", n ), spell->effectN( 1 ).trigger() ) ),
      bladestorm_oh( nullptr ),
      mortal_strike( nullptr ),
      signet_chance( 0.5 * p->legendary.signet_of_tormented_kings->proc_chance() ),
      signet_triggered( signet_triggered )
  {
    parse_options( options_str );
    channeled = false;
    tick_zero = true;
    interrupt_auto_attack = false;
    travel_speed                      = 0;

    bladestorm_mh->weapon             = &( player->main_hand_weapon );
    add_child( bladestorm_mh );
    if ( player->off_hand_weapon.type != WEAPON_NONE && player->specialization() == WARRIOR_FURY )
    {
    bladestorm_oh         = new bladestorm_tick_t( p, fmt::format( "{}_oh", n ), spell->effectN( 1 ).trigger() );
      bladestorm_oh->weapon = &( player->off_hand_weapon );
      add_child( bladestorm_oh );
    }

    // Unhinged DOES work w/ Torment and Signet
    if ( p->talents.arms.unhinged->ok() )
    {
      mortal_strike = new mortal_strike_unhinged_t( p, "bladestorm_mortal_strike" );
      add_child( mortal_strike );
    }

    // Vision of Perfection only reduces the cooldown for Arms
    if ( p->azerite.vision_of_perfection.enabled() && p->specialization() == WARRIOR_ARMS )
    {
      cooldown->duration *= 1.0 + azerite::vision_of_perfection_cdr( p->azerite.vision_of_perfection );
    }

    if ( signet_triggered )
    {
      dot_duration = p->legendary.signet_of_tormented_kings->effectN( 3 ).time_value();
    }
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    if ( signet_triggered )
      return warrior_attack_t::composite_dot_duration( s );
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if( signet_triggered )
    {
      p()->buff.bladestorm->trigger( dot_duration );
    }
    else
    {
      p()->buff.bladestorm->trigger();

      if ( p()->legendary.signet_of_tormented_kings.ok() )
      {
        action_t* signet_ability = p()->rng().roll( signet_chance ) ? p()->active.signet_avatar : p()->active.signet_recklessness;
        signet_ability->schedule_execute();
      }
      if ( p()->talents.warrior.blademasters_torment.ok() )
      {
        action_t* torment_ability = p()->active.torment_avatar;
        torment_ability->schedule_execute();
      }
    }

    if ( p()->talents.arms.hurricane->ok() || p()->talents.fury.hurricane->ok() )
    {
      p()->buff.hurricane_driver->trigger();
    }
  }

  void tick( dot_t* d ) override
  {
    // dont tick if BS buff not up
    // since first tick is instant the buff won't be up yet
    if ( d->ticks_left() < d->num_ticks() && !p()->buff.bladestorm->up() )
    {
      make_event( sim, [ d ] { d->cancel(); } );
      return;
    }

    if ( d->ticks_left() > 0 )
    {
      p()->buff.tornados_eye->trigger();
      p()->buff.gathering_storm->trigger();
    }
    else
    {
      // only duration is refreshed on last tick
      p()->buff.tornados_eye->trigger( 0 );
      p()->buff.gathering_storm->trigger( 0 );
    }

    warrior_attack_t::tick( d );
    bladestorm_mh->execute();

    if ( bladestorm_mh->result_is_hit( execute_state->result ) && bladestorm_oh )
    {
      bladestorm_oh->execute();
    }
    // As of Dragonflight, Unhinged triggers with tick 2 and 4
    if ( mortal_strike && ( d->current_tick == 2 || d->current_tick == 4 ) )
    {
      auto t = select_random_target();

      if ( t )
      {
        mortal_strike->target = t;
        mortal_strike->execute();
      }
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );
    p()->buff.bladestorm->expire();
    if ( p()->conduit.merciless_bonegrinder->ok() && !p()->talents.arms.merciless_bonegrinder->ok() ) // talent overrides conduit
    {
    p()->buff.merciless_bonegrinder_conduit->trigger( timespan_t::from_seconds( 9.0 ) );
    }
    if ( p()->talents.arms.merciless_bonegrinder->ok() )
    {
      p()->buff.merciless_bonegrinder->trigger();
    }
  }

// TODO: Mush Torment Bladestorm into regular Bladestorm reporting
//  void init() override
//  {
//    warrior_attack_t::init();
//    p()->active.torment_bladestorm->stats = stats;
//  }

  bool verify_actor_spec() const override
  {
    if ( signet_triggered )
      return true;

    return warrior_attack_t::verify_actor_spec();
  }
};

// Torment Bladestorm ===============================================================

struct torment_bladestorm_t : public warrior_attack_t
{
  attack_t *bladestorm_mh, *bladestorm_oh;
  mortal_strike_unhinged_t* mortal_strike;

  torment_bladestorm_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell,
                bool torment_triggered = false )
    : warrior_attack_t( n, p, spell ),
      bladestorm_mh( new bladestorm_tick_t( p, fmt::format( "{}_mh", n ), spell->effectN( 1 ).trigger() ) ),
      bladestorm_oh( nullptr ),
      mortal_strike( nullptr )
  {
    parse_options( options_str );
    channeled = false;
    tick_zero = true;
    interrupt_auto_attack = false;
    travel_speed = 0;
    dot_duration = p->talents.warrior.blademasters_torment->effectN( 2 ).time_value();
    bladestorm_mh->weapon = &( player->main_hand_weapon );
    add_child( bladestorm_mh );

    if ( p->talents.arms.unhinged->ok() )
    {
      mortal_strike = new mortal_strike_unhinged_t( p, "bladestorm_mortal_strike" );
      add_child( mortal_strike );
    }
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
      return warrior_attack_t::composite_dot_duration( s );
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.bladestorm->trigger();

    if ( p()->talents.arms.hurricane->ok() )
    {
      p()->buff.hurricane_driver->trigger();
    }
  }

  void tick( dot_t* d ) override
  {
    // dont tick if BS buff not up
    // since first tick is instant the buff won't be up yet
    if ( d->ticks_left() < d->num_ticks() && !p()->buff.bladestorm->up() )
    {
      make_event( sim, [ d ] { d->cancel(); } );
      return;
    }

    warrior_attack_t::tick( d );
    bladestorm_mh->execute();

    // As of Dragonflight, Unhinged triggers with tick 2 and 4
    if ( mortal_strike && ( d->current_tick == 2 || d->current_tick == 4 ) )
    {
      auto t = select_random_target();

      if ( t )
      {
        mortal_strike->target = t;
        mortal_strike->execute();
      }
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );
    p()->buff.bladestorm->expire();
    if ( p()->conduit.merciless_bonegrinder->ok() )
    {
      p()->buff.merciless_bonegrinder_conduit->trigger( timespan_t::from_seconds( 9.0 ) );
    }
    if ( p()->talents.arms.merciless_bonegrinder->ok() )
    {
      p()->buff.merciless_bonegrinder->trigger();
    }
  }
};

// Bloodthirst Heal =========================================================

struct bloodthirst_heal_t : public warrior_heal_t
{
  bloodthirst_heal_t( warrior_t* p ) : warrior_heal_t( "bloodthirst_heal", p, p->find_spell( 117313 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background    = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_heal_t::action_multiplier();

    return am;
  }
  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

// Bloodthirst ==============================================================

struct gushing_wound_dot_t : public warrior_attack_t
{
  gushing_wound_dot_t( warrior_t* p ) : warrior_attack_t( "gushing_wound", p, p->find_spell( 385042 ) ) //288080 & 4 is CB
  {
    background = tick_may_crit = true;
    hasted_ticks               = false;
    //base_td = p->talents.fury.cold_steel_hot_blood.value( 2 );
  }
};

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;
  warrior_attack_t* gushing_wound;
  int aoe_targets;
  double enrage_chance;
  double rage_from_cold_steel_hot_blood;
  double rage_from_merciless_assault;
  bloodthirst_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "bloodthirst", p, p->talents.fury.bloodthirst ),
      bloodthirst_heal( nullptr ),
      gushing_wound( nullptr ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      enrage_chance( p->spec.enrage->effectN( 2 ).percent() ),
      rage_from_cold_steel_hot_blood( p->find_spell( 383978 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_merciless_assault( p->find_spell( 409983 )->effectN( 1 ).base_value() / 10.0 )
  {
    parse_options( options_str );

    weapon = &( p->main_hand_weapon );
    radius = 5;
    if ( p->non_dps_mechanics )
    {
      bloodthirst_heal = new bloodthirst_heal_t( p );
    }
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();

    if ( p->talents.fury.fresh_meat->ok() )
    {
      enrage_chance += p->talents.fury.fresh_meat->effectN( 1 ).percent();
    }

    if ( p->talents.fury.cold_steel_hot_blood.ok() )
    {
      gushing_wound = new gushing_wound_dot_t( p );
    }
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->conduit.vicious_contempt->ok() && ( target->health_percentage() < 35 ) )
    {
      am *= 1.0 + ( p()->conduit.vicious_contempt.value() / 100.0 );
    }

    if ( p()->talents.fury.vicious_contempt->ok() && ( target->health_percentage() < 35 ) )
    {
      am *= 1.0 + ( p()->talents.fury.vicious_contempt->effectN( 1 ).percent() );
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    // Delayed event to cancel the stack on any crit results
    if ( p()->talents.fury.bloodcraze->ok() && s->result == RESULT_CRIT )
      make_event( *p()->sim, [ this ] { p()->buff.bloodcraze->expire(); } );

    if ( gushing_wound && s->result == RESULT_CRIT )
    {
      gushing_wound->set_target( s->target );
      gushing_wound->execute();
    }

    if ( p()->talents.fury.cold_steel_hot_blood.ok() && execute_state->result == RESULT_CRIT &&
         p()->cooldown.cold_steel_hot_blood_icd->up() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_cold_steel_hot_blood, p()->gain.cold_steel_hot_blood );
      p() -> cooldown.cold_steel_hot_blood_icd->start();
    }

    if ( p()->tier_set.t30_fury_4pc->ok() && target == s->target )
    {
      p()->resource_gain( RESOURCE_RAGE, p()->buff.merciless_assault->stack() * rage_from_merciless_assault,
                          p()->gain.merciless_assault );
    }

    p()->buff.fujiedas_fury->trigger( 1 );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.fury.bloodcraze->ok() )
    {
      p()->buff.bloodcraze->trigger();
    }

    p()->buff.meat_cleaver->decrement();
    p()->buff.merciless_assault->expire();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( bloodthirst_heal )
      {
        bloodthirst_heal->execute();
      }

      if ( rng().roll( enrage_chance ) )
      {
        p()->enrage();
      }

      if ( p()->legendary.cadence_of_fujieda->ok() )
      {
        p()->buff.cadence_of_fujieda->trigger( 1 );
      }
    }
    if( !td( execute_state->target )->hit_by_fresh_meat )
    {
      p()->buff.enrage->trigger();
      td( execute_state->target )->hit_by_fresh_meat = true;
    }
  }

  bool ready() override
  {
    if ( p()->buff.reckless_abandon->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Bloodbath ==============================================================

struct bloodbath_t : public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;
  warrior_attack_t* gushing_wound;
  int aoe_targets;
  double enrage_chance;
  double rage_from_cold_steel_hot_blood;
  double rage_from_merciless_assault;
  bloodbath_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "bloodbath", p, p->spec.bloodbath ),
      bloodthirst_heal( nullptr ),
      gushing_wound( nullptr ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      enrage_chance( p->spec.enrage->effectN( 2 ).percent() ),
      rage_from_cold_steel_hot_blood( p->find_spell( 383978 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_merciless_assault( p->find_spell( 409983 )->effectN( 1 ).base_value() / 10.0 )
  {
    parse_options( options_str );

    weapon = &( p->main_hand_weapon );
    radius = 5;
    if ( p->non_dps_mechanics )
    {
      bloodthirst_heal = new bloodthirst_heal_t( p );
    }
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();

    if ( p->talents.fury.fresh_meat->ok() )
    {
      enrage_chance += p->talents.fury.fresh_meat->effectN( 1 ).percent();
    }

    if ( p->talents.fury.cold_steel_hot_blood.ok() )
    {
      gushing_wound = new gushing_wound_dot_t( p );
    }
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->conduit.vicious_contempt->ok() && ( target->health_percentage() < 35 ) )
    {
      am *= 1.0 + ( p()->conduit.vicious_contempt.value() / 100.0 );
    }

    if ( p()->talents.fury.vicious_contempt->ok() && ( target->health_percentage() < 35 ) )
    {
      am *= 1.0 + ( p()->talents.fury.vicious_contempt->effectN( 1 ).percent() );
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    // Delayed event to cancel the stack on any crit results
    if ( p()->talents.fury.bloodcraze->ok() && s->result == RESULT_CRIT )
      make_event( *p()->sim, [ this ] { p()->buff.bloodcraze->expire(); } );


    if ( gushing_wound && s->result == RESULT_CRIT )
    {
      gushing_wound->set_target( s->target );
      gushing_wound->execute();
    }

    if ( p()->talents.fury.cold_steel_hot_blood.ok() && execute_state->result == RESULT_CRIT &&
         p()->cooldown.cold_steel_hot_blood_icd->up() )
    {
      p()->cooldown.cold_steel_hot_blood_icd->start();
    }

    if ( p()->tier_set.t30_fury_4pc->ok() && target == s->target )
    {
      p()->resource_gain( RESOURCE_RAGE, p()->buff.merciless_assault->stack() * rage_from_merciless_assault,
                          p()->gain.merciless_assault );
    }

    p()->buff.fujiedas_fury->trigger( 1 );
    if ( p()->legendary.cadence_of_fujieda->ok() )
    {
      p()->buff.cadence_of_fujieda->trigger( 1 );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.fury.bloodcraze->ok() )
    {
      p()->buff.bloodcraze->trigger();
    }

    p()->buff.reckless_abandon->decrement();

    p()->buff.meat_cleaver->decrement();
    p()->buff.merciless_assault->expire();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( bloodthirst_heal )
      {
        bloodthirst_heal->execute();
      }

      if ( rng().roll( enrage_chance ) )
      {
        p()->enrage();
      }
    }
  }

  bool ready() override
  {
    if ( !p()->buff.reckless_abandon->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Onslaught ==============================================================

struct onslaught_t : public warrior_attack_t
{
  double unbridled_chance;
  const spell_data_t* damage_spell;
  int aoe_targets;
  onslaught_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "onslaught", p, p->talents.fury.onslaught ),
      unbridled_chance( p->talents.fury.unbridled_ferocity->effectN( 1 ).base_value() / 100.0 ),
      damage_spell( p->find_spell( 396718U ) ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 2 ).base_value() ) )
  {
    parse_options( options_str );
    weapon              = &( p->main_hand_weapon );
    radius              = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
    attack_power_mod.direct = damage_spell->effectN( 1 ).ap_coeff();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  void execute() override
  {
    if ( p()->talents.fury.tenderize->ok() )
    {
      p()->enrage();
      p()->buff.slaughtering_strikes_rb->trigger( as<int>( p()->talents.fury.tenderize->effectN( 2 ).base_value() ) );
    }

    if ( p()->talents.fury.unbridled_ferocity.ok() && rng().roll( unbridled_chance ) )
    {
      const timespan_t trigger_duration = p()->talents.fury.unbridled_ferocity->effectN( 2 ).time_value();
      p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
    }

    warrior_attack_t::execute();

    p()->buff.meat_cleaver->decrement();

  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
      return false;
    return warrior_attack_t::ready();
  }
};

// Charge ===================================================================

struct charge_t : public warrior_attack_t
{
  // Split the damage, resource gen and other triggered effects from the movement part of Charge
  struct charge_impact_t : public warrior_attack_t
  {
    const spell_data_t* damage_spell;

    charge_impact_t( warrior_t* p ) :
      warrior_attack_t( "charge_impact", p, p -> spell.charge ),
      damage_spell( p->find_spell( 126664 ) )
    {
      background = true;

      energize_resource       = RESOURCE_RAGE;
      energize_type           = action_energize::ON_CAST;
      attack_power_mod.direct = damage_spell->effectN( 1 ).ap_coeff();

      //Reprisal extra rage gain for Charge
      if ( p->legendary.reprisal->ok() )
      {
        energize_amount +=  p->find_spell( 335734 )->effectN( 1 ).resource( RESOURCE_RAGE );
      }
    }

    void execute() override
    {
      warrior_attack_t::execute();

      if ( p()->legendary.reprisal->ok() )
      {
        if ( p()->buff.shield_block->check() )
        {
          // Even though it isn't mentionned anywhere in spelldata, reprisal only triggers shield block for 4s
          p()->buff.shield_block->extend_duration( p(), 4_s );
        }
        else
          p()->buff.shield_block->trigger( 4_s );

        p()->buff.revenge->trigger();
      }
    }
  };

  bool first_charge;
  double movement_speed_increase, min_range;
  charge_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "charge", p, p->spell.charge ),
      first_charge( true ),
      movement_speed_increase( 5.0 ),
      min_range( data().min_range() )
  {
    parse_options( options_str );
    movement_directionality = movement_direction_type::OMNI;

    impact_action = new charge_impact_t( p );

    // Rage gen is handled in the impact action
    energize_amount = 0;
    energize_resource = RESOURCE_NONE;

    if ( p->talents.warrior.double_time->ok() )
    {
      cooldown->charges += as<int>( p->talents.warrior.double_time->effectN( 1 ).base_value() );
      cooldown->duration += p->talents.warrior.double_time->effectN( 2 ).time_value();
    }
  }

  timespan_t calc_charge_time( double distance )
  {
    return timespan_t::from_seconds( distance /
      ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) );
  }

  void execute() override
  {
    if ( p()->current.distance_to_move > 5 )
    {
      p()->buff.charge_movement->trigger( 1, movement_speed_increase, 1, calc_charge_time( p()->current.distance_to_move ) );
      p()->current.moving_away = 0;
    }

    warrior_attack_t::execute();

  }

  void reset() override
  {
    warrior_attack_t::reset();
    first_charge = true;
  }

  bool ready() override
  {
    if ( first_charge )  // Assumes that we charge into combat, instead of setting initial distance to 20 yards.
    {
      return warrior_attack_t::ready();
    }
    if ( p()->current.distance_to_move < min_range )  // Cannot charge if too close to the target.
    {
      return false;
    }
    if ( p()->buff.charge_movement->check() || p()->buff.heroic_leap_movement->check() ||
         p()->buff.intervene_movement->check() || p() -> buff.shield_charge_movement->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Cleave ===================================================================

struct cleave_t : public warrior_attack_t
{
  double cost_rage;
  double frothing_berserker_chance;
  double rage_from_frothing_berserker;
  cleave_t( warrior_t* p, util::string_view options_str ) 
    : warrior_attack_t( "cleave", p, p->talents.arms.cleave ),
    frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
    rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).base_value() / 100.0 )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
    aoe = -1;
    reduced_aoe_targets = 5.0;
  }

  double cost() const override
  {
    if ( p()->buff.battlelord->check() )
    {
      return 10;
    }

    return warrior_attack_t::cost();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.martial_prowess->check_stack_value();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    if ( execute_state->n_targets >= 1 )
    {
      p()->active.deep_wounds_ARMS->set_target( s->target );
      p()->active.deep_wounds_ARMS->execute();
    }
    if ( p()->talents.arms.fatality->ok() && p()->rppm.fatal_mark->trigger() && target->health_percentage() > 30 )
    {  // does this eat RPPM when switching from low -> high health target?
      td( s->target )->debuffs_fatal_mark->trigger();
    }
    if ( p()->tier_set.t29_arms_4pc->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.strike_vulnerabilities->trigger();
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    cost_rage = last_resource_cost;
    if ( p()->talents.warrior.frothing_berserker->ok() && rng().roll( frothing_berserker_chance ) )
    {
      p()->resource_gain(RESOURCE_RAGE, last_resource_cost * rage_from_frothing_berserker, p()->gain.frothing_berserker);
    }
    p()->buff.battlelord->expire();
    p()->buff.martial_prowess->expire();
  }
};

// Colossus Smash ===========================================================

struct colossus_smash_t : public warrior_attack_t
{
  bool lord_of_war;
  double rage_from_lord_of_war;
  colossus_smash_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "colossus_smash", p, p->talents.arms.colossus_smash ),
      lord_of_war( false ),
      rage_from_lord_of_war(
          ( p->azerite.lord_of_war.spell()->effectN( 1 ).base_value() ) / 10.0 ) // 8.1 no extra rage with rank
  {
    if ( p->talents.arms.warbreaker->ok() )
    {
      background = true;  // Warbreaker replaces Colossus Smash for Arms
    }
    else
    {
      parse_options( options_str );
      weapon = &( player->main_hand_weapon );
      impact_action    = p->active.deep_wounds_ARMS;
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->azerite.lord_of_war.value( 2 );
    return b;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->azerite.lord_of_war.ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_lord_of_war, p()->gain.lord_of_war );
    }

    if ( result_is_hit( execute_state->result ) )
    {
      td( execute_state->target )->debuffs_colossus_smash->trigger();
      p()->buff.test_of_might_tracker->trigger();

      if ( p()->talents.arms.in_for_the_kill->ok() )
        p()->buff.in_for_the_kill->trigger();
    }

    if ( p()->talents.warrior.warlords_torment->ok() )
    {
      const timespan_t trigger_duration = p()->talents.warrior.warlords_torment->effectN( 2 ).time_value();
      p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
    }
  }
};

// Deep Wounds ARMS ==============================================================

struct deep_wounds_ARMS_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  deep_wounds_ARMS_t( warrior_t* p ) : warrior_attack_t( "deep_wounds", p, p->find_spell( 262115 ) ),
    bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
    rage_from_bloodsurge( p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }

    if ( p()->tier_set.t30_arms_4pc->ok() && d->state->result == RESULT_CRIT )
    {
      p()->buff.crushing_advance->trigger();
    }
  }
};

// Deep Wounds PROT ==============================================================

struct deep_wounds_PROT_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  deep_wounds_PROT_t( warrior_t* p )
    : warrior_attack_t( "deep_wounds", p, p->spec.deep_wounds_PROT->effectN( 1 ).trigger() ),
    bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
    rage_from_bloodsurge( p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }
  }
};

// Demoralizing Shout =======================================================

struct demoralizing_shout_t : public warrior_attack_t
{
  double rage_gain;
  demoralizing_shout_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "demoralizing_shout", p, p->talents.protection.demoralizing_shout ), rage_gain( 0 )
  {
    parse_options( options_str );
    usable_while_channeling = true;
    rage_gain += p->talents.protection.booming_voice->effectN( 1 ).resource( RESOURCE_RAGE );
    aoe = -1;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( rage_gain > 0 )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_gain, p()->gain.booming_voice );
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    td( s->target ) -> debuffs_demoralizing_shout -> trigger(
      1, data().effectN( 1 ).percent(), 1.0, p()->talents.protection.demoralizing_shout->duration()  );
  }
};

// Thunderous Roar ==============================================================

struct thunderous_roar_dot_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  thunderous_roar_dot_t( warrior_t* p )
    : warrior_attack_t( "thunderous_roar_dot", p, p->find_spell( 397364 ) ),
      bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
      rage_from_bloodsurge( p->find_spell( 384362 )->effectN( 1 ).base_value() / 10.0 )
  {
    background = tick_may_crit = true;
    //hasted_ticks               = false; //currently hasted in game - likely unintended
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }
  }
};

struct thunderous_roar_t : public warrior_attack_t
{
  warrior_attack_t* thunderous_roar_dot;
  thunderous_roar_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "thunderous_roar", p, p->talents.warrior.thunderous_roar ),
      thunderous_roar_dot( nullptr )
  {
    parse_options( options_str );
    aoe       = -1;
    may_dodge = may_parry = may_block = false;

    thunderous_roar_dot   = new thunderous_roar_dot_t( p );
    add_child( thunderous_roar_dot );

  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    thunderous_roar_dot->set_target( s->target );
    thunderous_roar_dot->execute();
  }
};

// Arms Execute ==================================================================

struct execute_damage_t : public warrior_attack_t
{
  double max_rage;
  double cost_rage;
  execute_damage_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "execute_damage", p, p->spell.execute->effectN( 1 ).trigger() ), max_rage( 40 )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
    background = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( cost_rage == 0 )  // If it was free, it's a full damage execute.
      am *= 2.0;
    else
      am *= 2.0 * ( std::min( max_rage, cost_rage ) / max_rage );
    return am;
  }
};

struct execute_arms_t : public warrior_attack_t
{
  execute_damage_t* trigger_attack;
  double max_rage;
  double execute_pct;
  double shield_slam_reset;
  execute_arms_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "execute", p, p->spell.execute ), max_rage( 40 ), execute_pct( 20 ),
    shield_slam_reset( p -> talents.protection.strategist -> effectN( 1 ).percent() )
  {
    parse_options( options_str );
    weapon        = &( p->main_hand_weapon );

    trigger_attack = new execute_damage_t( p, options_str );

    if ( p->talents.arms.massacre->ok() )
    {
      execute_pct = p->talents.arms.massacre->effectN( 2 )._base_value;
    }
    if ( p->talents.protection.massacre->ok() )
    {
      execute_pct = p->talents.protection.massacre->effectN( 2 ).base_value();
    }
  }

  double tactician_cost() const override
  {
    double c;

    if ( p()->buff.sudden_death->check() )
    {
      c = 0;
    }
    else
    {
      c = std::min( max_rage, p()->resources.current[ RESOURCE_RAGE ] );
      c = ( c / max_rage ) * 40;
    }
    if ( sim->log )
    {
      sim->out_debug.printf( "Rage used to calculate tactician chance from ability %s: %4.4f, actual rage used: %4.4f",
                             name(), c, cost() );
    }
    return c;
  }

  double cost() const override
  {
    double c = warrior_attack_t::cost();
    c        = std::min( max_rage, std::max( p()->resources.current[ RESOURCE_RAGE ], c ) );

    if ( p()->buff.ayalas_stone_heart->check() )
    {
      c *= 1.0 + p()->buff.ayalas_stone_heart->data().effectN( 2 ).percent();
    }
    if ( p()->buff.sudden_death->check() )
    {
      return 0;  // The cost reduction isn't in the spell data
    }
    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    trigger_attack->cost_rage = last_resource_cost;
    trigger_attack->execute();
    if ( p()->talents.arms.improved_execute->ok() && !p()->talents.arms.critical_thinking->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, last_resource_cost * p()->find_spell( 163201 )->effectN( 2 ).percent(),
                          p()->gain.execute_refund );  // Not worth the trouble to check if the target died.
    }
    if ( p()->talents.arms.improved_execute->ok() && p()->talents.arms.critical_thinking->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, last_resource_cost * ( p()->talents.arms.critical_thinking->effectN( 2 ).percent() +
                                                 p()->find_spell( 163201 )->effectN( 2 ).percent() ),
                          p()->gain.execute_refund );  // Not worth the trouble to check if the target died.
    }

    if (p()->buff.sudden_death->up())
    {
      p()->buff.sudden_death->expire();
    }
    if ( p()->talents.arms.executioners_precision->ok() && ( result_is_hit( execute_state->result ) ) )
    {
      td( execute_state->target )->debuffs_executioners_precision->trigger();
    }
    if ( p()->legendary.exploiter.ok() && !p()->talents.arms.executioners_precision->ok() && ( result_is_hit( execute_state->result ) ) )
    {
      td( execute_state->target )->debuffs_exploiter->trigger();
    }
    if ( p()->conduit.ashen_juggernaut.ok() && !p()->talents.arms.juggernaut->ok() )
    {
      p()->buff.ashen_juggernaut_conduit->trigger();
    }
    if ( p()->talents.arms.juggernaut.ok() )
    {
      p()->buff.juggernaut->trigger();
    }
    if ( p()->talents.protection.juggernaut.ok() )
    {
      p()->buff.juggernaut_prot->trigger();
    }

    if ( rng().roll( shield_slam_reset ) )
      p()->cooldown.shield_slam->reset( true );
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if( state->target->health_percentage() < 30 && td( state->target )->debuffs_fatal_mark->check() )
    {
      p()->active.fatality->set_target( state->target );
      p()->active.fatality->execute();
    }

    if ( p()->tier_set.t29_arms_4pc->ok() && state->result == RESULT_CRIT )
    {
      p()->buff.strike_vulnerabilities->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Ayala's Stone Heart and Sudden Death allow execution on any target
    bool always = p()->buff.ayalas_stone_heart->check() || p()->buff.sudden_death->check();

    if ( ! always && candidate_target->health_percentage() > execute_pct )
    {
      return false;
    }

    return warrior_attack_t::target_ready( candidate_target );
  }

  bool ready() override
  {
    if ( p()->covenant.condemn_driver->ok() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Fatality ===============================================================================
struct fatality_t : public warrior_attack_t
{
  fatality_t( warrior_t* p ) : warrior_attack_t( "fatality", p, p->find_spell( 383706 ) )
  {
    background = true;
    may_crit   = false;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warrior_attack_t::composite_target_multiplier( target );
    m *= td( target )->debuffs_fatal_mark->stack();
    return m;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );
    td( state->target )->debuffs_fatal_mark->expire();
  }
};

// Fury Execute ======================================================================
struct execute_main_hand_t : public warrior_attack_t
{
  int aoe_targets;
  execute_main_hand_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    background = true;
    dual   = true;
    weapon = &( p->main_hand_weapon );
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }
};

struct execute_off_hand_t : public warrior_attack_t
{
  int aoe_targets;
  execute_off_hand_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    background = true;
    dual     = true;
    may_miss = may_dodge = may_parry = may_block = false;
    weapon                                       = &( p->off_hand_weapon );
    //base_multiplier *= 1.0 + p->spec.execute_rank_2->effectN( 1 ).percent();
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }
};

struct fury_execute_parent_t : public warrior_attack_t
{
  execute_main_hand_t* mh_attack;
  execute_off_hand_t* oh_attack;
  bool improved_execute;
  double execute_pct;
  //double cost_rage;
  double max_rage;
  double rage_from_improved_execute;
  fury_execute_parent_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "execute", p, p->spec.execute ),
      improved_execute( false ),
      execute_pct( 20 ),
      max_rage( 40 ),
      rage_from_improved_execute(
      ( p->talents.fury.improved_execute->effectN( 3 ).base_value() ) / 10.0 )
  {
    parse_options( options_str );
    mh_attack = new execute_main_hand_t( p, "execute_mainhand", p->spec.execute->effectN( 1 ).trigger() );
    oh_attack = new execute_off_hand_t( p, "execute_offhand", p->spec.execute->effectN( 2 ).trigger() );
    add_child( mh_attack );
    add_child( oh_attack );

    if ( p->talents.fury.massacre->ok() )
    {
      execute_pct = p->talents.fury.massacre->effectN( 2 ).base_value();
      if ( cooldown->action == this )
      {
        cooldown -> duration -= timespan_t::from_millis( p -> talents.fury.massacre -> effectN( 3 ).base_value() );
      }
    }
  }

  double cost() const override
  {
    double c = warrior_attack_t::cost();
    c        = std::min( max_rage, std::max( p()->resources.current[ RESOURCE_RAGE ], c ) );

    if ( p()->talents.fury.improved_execute->ok() )
    {
      c *= 1.0 + p()->talents.fury.improved_execute->effectN( 1 ).percent();
    }
    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    mh_attack->execute();

    if ( p()->specialization() == WARRIOR_FURY && result_is_hit( execute_state->result ) &&
         p()->off_hand_weapon.type != WEAPON_NONE )
      // If MH fails to land, or if there is no OH weapon for Fury, oh attack does not execute.
      oh_attack->execute();

    p()->buff.meat_cleaver->decrement();
    p()->buff.sudden_death->expire();
    p()->buff.ayalas_stone_heart->expire();

    if ( p()->talents.fury.improved_execute->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_improved_execute, p()->gain.execute );
    }

    if ( p()->conduit.ashen_juggernaut.ok() && !p()->talents.fury.ashen_juggernaut->ok() ) // talent overrides conduit )
    {
      p()->buff.ashen_juggernaut_conduit->trigger();
    }
    if ( p()->talents.fury.ashen_juggernaut->ok() )
    {
      p()->buff.ashen_juggernaut->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Ayala's Stone Heart and Sudden Death allow execution on any target
    bool always = p()->buff.ayalas_stone_heart->check() || p()->buff.sudden_death->check();

    if ( ! always && candidate_target->health_percentage() > execute_pct )
    {
      return false;
    }

    return warrior_attack_t::target_ready( candidate_target );
  }

  bool ready() override
  {
    if ( p()->covenant.condemn_driver->ok() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Hamstring ==============================================================

struct hamstring_t : public warrior_attack_t
{
  hamstring_t( warrior_t* p, util::string_view options_str ) : warrior_attack_t( "hamstring", p, p->spell.hamstring )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
  }
};

// Piercing Howl ==============================================================

struct piercing_howl_t : public warrior_attack_t
{
  piercing_howl_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "piercing_howl", p, p->talents.warrior.piercing_howl )
  {
    parse_options( options_str );
  }
};

// Heroic Throw ==============================================================

struct heroic_throw_t : public warrior_attack_t
{
  heroic_throw_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "heroic_throw", p, p->find_class_spell( "Heroic Throw" ) )
  {
    parse_options( options_str );

    weapon    = &( player->main_hand_weapon );
    may_dodge = may_parry = may_block = false;

    if ( p -> talents.protection.improved_heroic_throw -> ok() )
    {
      impact_action = p->active.deep_wounds_PROT;
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.protection.improved_heroic_throw->ok() )
    {
      am *= 1.0 + p()->talents.protection.improved_heroic_throw->effectN( 2 ).percent();
    }

    return am;
  }

  bool ready() override
  {
    if ( p()->current.distance_to_move > range ||
         p()->current.distance_to_move < data().min_range() )  // Cannot heroic throw unless target is in range.
    {
      return false;
    }
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Heroic Leap ==============================================================

struct heroic_leap_t : public warrior_attack_t
{
  const spell_data_t* heroic_leap_damage;
  heroic_leap_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "heroic_leap", p, p->talents.warrior.heroic_leap ),
      heroic_leap_damage( p->find_spell( 52174 ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    aoe                   = -1;
    may_dodge = may_parry = may_miss = may_block = false;
    movement_directionality                      = movement_direction_type::OMNI;
    base_teleport_distance                       = data().max_range();
    range                                        = -1;
    attack_power_mod.direct                      = heroic_leap_damage->effectN( 1 ).ap_coeff();
    radius                                       = heroic_leap_damage->effectN( 1 ).radius();

    cooldown->duration = data().charge_cooldown();  // Fixes bug in spelldata for now.
    cooldown->duration += p->talents.warrior.bounding_stride->effectN( 1 ).time_value();
  }

  // TOCHECK: Shouldn't this scale with distance to travel?
  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 0.5 );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p()->current.distance_to_move > 0 && !p()->buff.heroic_leap_movement->check() )
    {
      double speed = std::min( p()->current.distance_to_move, base_teleport_distance ) /
                     ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() ) ) /
                     travel_time().total_seconds();
      p()->buff.heroic_leap_movement->trigger( 1, speed, 1, travel_time() );
    }
  }

  bool ready() override
  {
    if ( p()->buff.intervene_movement->check() || p()->buff.charge_movement->check() ||
         p()->buff.intercept_movement->check() || p()->buff.shield_charge_movement->check() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Impending Victory Heal ========================================================

struct impending_victory_heal_t : public warrior_heal_t
{
  impending_victory_heal_t( warrior_t* p ) : warrior_heal_t( "impending_victory_heal", p, p->find_spell( 202166 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background    = true;
  }

  proc_types proc_type() const override
  {
    return PROC1_NONE_HEAL;
  }

  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

// Impending Victory ==============================================================

struct impending_victory_t : public warrior_attack_t
{
  impending_victory_heal_t* impending_victory_heal;
  impending_victory_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "impending_victory", p, p->talents.warrior.impending_victory ), impending_victory_heal( nullptr )
  {
    parse_options( options_str );
    if ( p->non_dps_mechanics )
    {
      impending_victory_heal = new impending_victory_heal_t( p );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( impending_victory_heal )
    {
      impending_victory_heal->execute();
    }
  }
};

// Intervene ===============================================================
// Note: Conveniently ignores that you can only intervene a friendly target.
// For the time being, we're just going to assume that there is a friendly near the target
// that we can intervene to. Maybe in the future with a more complete movement system, we will
// fix this to work in a raid simulation that includes multiple melee.

struct intervene_t : public warrior_attack_t
{
  double movement_speed_increase;
  intervene_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "intervene", p, p->talents.warrior.intervene ), movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    ignore_false_positive   = true;
    movement_directionality = movement_direction_type::OMNI;

    // Reprisal rage gain for Intervene
    if ( p->legendary.reprisal->ok() )
    {
      energize_resource = RESOURCE_RAGE;
      energize_type     = action_energize::ON_CAST;
      energize_amount += p->find_spell( 335734 )->effectN( 1 ).resource( RESOURCE_RAGE );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p()->current.distance_to_move > 0 )
    {
      p()->buff.intervene_movement->trigger(
          1, movement_speed_increase, 1,
          timespan_t::from_seconds(
              p()->current.distance_to_move /
              ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) ) );
      p()->current.moving_away = 0;
    }
    
    //Reprisal Shield Block and Revenge! gain for Intervene.
    if ( p()->legendary.reprisal->ok() )
    {
      if ( p()->buff.shield_block->check() )
      {
        // Even though it isn't mentionned anywhere in spelldata, reprisal only triggers shield block for 4s
        p()->buff.shield_block->extend_duration( p(), 4_s );
      }
      else
        p()->buff.shield_block->trigger( 4_s );

      p()->buff.revenge->trigger();
    }
  }

  bool ready() override
  {
    if ( p()->buff.heroic_leap_movement->check() || p()->buff.charge_movement->check() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Iron Fortress azerite trait ==============================================

struct iron_fortress_t : public warrior_attack_t
{
  bool crit_blocked;

  iron_fortress_t( warrior_t* p ) :
    warrior_attack_t( "iron_fortress", p, p -> find_spell( 279142 ) ),
    crit_blocked( false )
  {
    base_dd_min = base_dd_max = p -> azerite.iron_fortress.value( 1 );

    background = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    // If the hit was critically blocked, deals twice as much damage (no spelldata for it)
    if ( crit_blocked )
    {
      am *= 2.0;
    }

    return am;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p() -> cooldown.iron_fortress_icd -> start();
  }
};

// Heroic Charge ============================================================

struct heroic_charge_t : public warrior_attack_t
{
  //TODO: do actual movement for distance targeting sims
  warrior_attack_t* heroic_leap;
  charge_t* charge;
  int heroic_leap_distance;

  heroic_charge_t( warrior_t* p, util::string_view options_str ) :
    warrior_attack_t( "heroic_charge", p, spell_data_t::nil() ),
    heroic_leap( new heroic_leap_t( p, "" ) ),
    charge( new charge_t( p, "" ) ),
    heroic_leap_distance( 10 )
  {
    // The user can set the distance that they want to travel with heroic leap before charging if they want
    add_option( opt_int( "distance", heroic_leap_distance ) );
    parse_options( options_str );

    if ( heroic_leap_distance > heroic_leap -> data().max_range() ||
         heroic_leap_distance < heroic_leap -> data().min_range() )
    {
      sim -> error( "{} has an invalid heroic leap distance of {}, defaulting to 10", p -> name(), heroic_leap_distance );
      heroic_leap_distance = 10;
    }
    callbacks = false;
  }

  timespan_t execute_time() const override
  {
    // Execute time is equal to the sum of heroic leap's travel time (a fixed 0.5s atm)
    // and charge's time to travel based on heroic leap's distance travelled
    return heroic_leap -> travel_time() + charge -> calc_charge_time( heroic_leap_distance );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    // At the end of the execution, start cooldowns, adjusted for execute time
    heroic_leap -> cooldown -> start( heroic_leap -> cooldown_duration() - execute_time() );
    charge -> cooldown -> start( charge -> cooldown_duration() - charge -> calc_charge_time( heroic_leap_distance ) );

    // Execute charge damage and whatever else it's supposed to proc
    charge -> impact_action -> execute();
  }

  bool ready() override
  {
    // TODO: Enable heroic leaping mid-movement while making sure it doesn't break anything
    if ( ! heroic_leap -> cooldown -> is_ready() || ! charge -> cooldown -> is_ready() ||
         p()->buffs.movement->check() || p() -> buff.heroic_leap_movement -> check() || p() -> buff.charge_movement -> check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Pummel ===================================================================

struct pummel_t : public warrior_attack_t
{
  pummel_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "pummel", p, p->find_class_spell( "Pummel" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
    is_interrupt = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.warrior.concussive_blows.ok() && result_is_hit( execute_state->result ) )
    {
      td( execute_state->target )->debuffs_concussive_blows->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return warrior_attack_t::target_ready( candidate_target );
  }
};

// Raging Blow ==============================================================

struct raging_blow_attack_t : public warrior_attack_t
{
  int aoe_targets;
  raging_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
    aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    dual                                         = true;
    background = true;

    //base_multiplier *= 1.0 + p->talents.cruelty->effectN( 1 ).percent();
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.fury.cruelty->ok() && p()->buff.enrage->check() )
    {
      am *= 1.0 + p()->talents.fury.cruelty->effectN( 1 ).percent();
    }
    if ( p()->talents.fury.wrath_and_fury->ok() )
    {
      am *= 1.0 + p()->find_spell( 386045 )->effectN( 1 ).percent();
    }

    return am;
  }
};

struct raging_blow_t : public warrior_attack_t
{
  raging_blow_attack_t* mh_attack;
  raging_blow_attack_t* oh_attack;
  double cd_reset_chance;
  double wrath_and_fury_reset_chance;
  raging_blow_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "raging_blow", p, p->talents.fury.raging_blow ),
      mh_attack( nullptr ),
      oh_attack( nullptr ),
      cd_reset_chance( p->talents.fury.raging_blow->effectN( 1 ).percent() ),
      wrath_and_fury_reset_chance( p->talents.fury.wrath_and_fury->proc_chance() )
  {
    parse_options( options_str );

    oh_attack         = new raging_blow_attack_t( p, "raging_blow_oh", p->talents.fury.raging_blow->effectN( 4 ).trigger() );
    oh_attack->weapon = &( p->off_hand_weapon );
    add_child( oh_attack );
    mh_attack         = new raging_blow_attack_t( p, "raging_blow_mh", p->talents.fury.raging_blow->effectN( 3 ).trigger() );
    mh_attack->weapon = &( p->main_hand_weapon );
    add_child( mh_attack );
    cooldown->reset( false );
    track_cd_waste = true;
    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }
  }

  void init() override
  {
    warrior_attack_t::init();
    cooldown->hasted = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      mh_attack->execute();
      oh_attack->execute();
    }
    if ( p()->talents.fury.improved_raging_blow->ok() && p()->talents.fury.wrath_and_fury->ok() &&
         p()->buff.enrage->check() )
    {
      if ( rng().roll( wrath_and_fury_reset_chance ) )
        {
          cooldown->reset( true );
        }
    }
    else if ( p()->talents.fury.improved_raging_blow->ok() )
    {
      if ( rng().roll( cd_reset_chance ) )
        {
          cooldown->reset( true );
        }
    }
    p()->buff.meat_cleaver->decrement();

    if ( p()->talents.fury.slaughtering_strikes->ok() )
    {
      p()->buff.slaughtering_strikes_rb->trigger();
    }
    if ( p()->azerite.pulverizing_blows.ok() )
    {
      p()->buff.pulverizing_blows->trigger();
    }
    if ( p()->buff.will_of_the_berserker->check() )
    {
      ( p()->buff.will_of_the_berserker->trigger() ); // RB refreshs, but does not initially trigger
    }
  }

  bool ready() override
  {
    // Needs weapons in both hands
    if ( p()->main_hand_weapon.type == WEAPON_NONE || p()->off_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    if ( p()->buff.reckless_abandon->check() )
    {
      return false;
    }
    if ( p()->talents.fury.annihilator->ok() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Crushing Blow ==============================================================

struct crushing_blow_attack_t : public warrior_attack_t
{
  int aoe_targets;
  crushing_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
    aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    dual                                         = true;
    background = true;

    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.fury.cruelty->ok() && p()->buff.enrage->check() )
    {
      am *= 1.0 + p()->talents.fury.cruelty->effectN( 1 ).percent();
    }
    if ( p()->talents.fury.wrath_and_fury->ok() )
    {
      am *= 1.0 + p()->find_spell( 386045 )->effectN( 1 ).percent();
    }

    return am;
  }
};

struct crushing_blow_t : public warrior_attack_t
{
  crushing_blow_attack_t* mh_attack;
  crushing_blow_attack_t* oh_attack;
  double cd_reset_chance, wrath_and_fury_reset_chance;
  crushing_blow_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "crushing_blow", p, p->spec.crushing_blow ),
      mh_attack( nullptr ),
      oh_attack( nullptr ),
      cd_reset_chance( p->spec.crushing_blow->effectN( 1 ).percent() ),
      wrath_and_fury_reset_chance( p->talents.fury.wrath_and_fury->proc_chance() )
  {
    parse_options( options_str );

    oh_attack         = new crushing_blow_attack_t( p, "crushing_blow_oh", p->spec.crushing_blow->effectN( 4 ).trigger() );
    oh_attack->weapon = &( p->off_hand_weapon );
    add_child( oh_attack );
    mh_attack         = new crushing_blow_attack_t( p, "crushing_blow_mh", p->spec.crushing_blow->effectN( 3 ).trigger() );
    mh_attack->weapon = &( p->main_hand_weapon );
    add_child( mh_attack );
    cooldown->reset( false );
    cooldown = p -> cooldown.raging_blow;
    track_cd_waste = true;

    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }
  }

  void init() override
  {
    warrior_attack_t::init();
    cooldown->hasted = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      mh_attack->execute();
      oh_attack->execute();
    }
    if ( p()->talents.fury.improved_raging_blow->ok() && p()->talents.fury.wrath_and_fury->ok() &&
         p()->buff.enrage->check() )
    {
      if ( rng().roll( wrath_and_fury_reset_chance ) )
      {
        cooldown->reset( true );
      }
    }
    else if ( p()->talents.fury.improved_raging_blow->ok() && rng().roll( cd_reset_chance ) )
    {
      cooldown->reset( true );
    }


    p()->buff.reckless_abandon->decrement();
    p()->buff.meat_cleaver->decrement();

    if ( p()->talents.fury.slaughtering_strikes->ok() )
    {
      p()->buff.slaughtering_strikes_rb->trigger();
    }
    if ( p()->azerite.pulverizing_blows.ok() )
    {
      p()->buff.pulverizing_blows->trigger();
    }
    if ( p()->buff.will_of_the_berserker->check() )
    {
      ( p()->buff.will_of_the_berserker->trigger() ); // CB refreshs, but does not initially trigger
    }
  }

  bool ready() override
  {
    // Needs weapons in both hands
    if ( p()->main_hand_weapon.type == WEAPON_NONE || p()->off_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    if ( !p()->buff.reckless_abandon->check() )
    {
      return false;
    }
    if ( p()->talents.fury.annihilator->ok() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Shattering Throw ========================================================

struct shattering_throw_t : public warrior_attack_t
{
  shattering_throw_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "shattering_throw", p, p->talents.warrior.shattering_throw )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
    attack_power_mod.direct = 1.0;
  }
  //add absorb shield bonus (are those even in SimC?), add cast time?
};

// Skullsplitter ===========================================================

struct skullsplitter_t : public warrior_attack_t
{
  skullsplitter_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "skullsplitter", p, p->talents.arms.skullsplitter )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
  }

  void trigger_tide_of_blood( dot_t* dot )
  {
    if ( !dot->is_ticking() )
      return;

    const int full_ticks = as<int>( std::floor( dot->ticks_left_fractional() ) );
    if ( full_ticks < 1 )
      return;

    p()->sim->print_log( "{} has {} ticks of {} remaining for Tide of Blood", *p(), full_ticks, *dot );
    for ( int i = 0; i < full_ticks; i++ )
    {
      dot->tick();
    }

    dot->cancel();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    warrior_td_t* td = p()->get_target_data( target );
    trigger_tide_of_blood( td->dots_deep_wounds );

    if ( p()->talents.arms.tide_of_blood->ok() )
    {
      trigger_tide_of_blood( td->dots_rend );
    }
  }
};

// Sweeping Strikes ===================================================================

struct sweeping_strikes_t : public warrior_spell_t
{
  sweeping_strikes_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "sweeping_strikes", p, p->spec.sweeping_strikes )
  {
    parse_options( options_str );
    callbacks = false;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.sweeping_strikes->trigger();
  }
};

// Odyn's Fury ==========================================================

struct odyns_fury_off_hand_t : public warrior_attack_t
{
  odyns_fury_off_hand_t( warrior_t* p, util::string_view name, const spell_data_t* spell )
    : warrior_attack_t( name, p, spell )
  {
    background          = true;
    aoe                 = -1;
    base_multiplier *= 1.0 + p->talents.fury.titanic_rage->effectN( 1 ).percent();
  }
};

struct odyns_fury_main_hand_t : public warrior_attack_t
{
  odyns_fury_main_hand_t( warrior_t* p, util::string_view name, const spell_data_t* spell )
    : warrior_attack_t( name, p, spell )
  {
    background = true;
    aoe        = -1;
    base_multiplier *= 1.0 + p->talents.fury.titanic_rage->effectN( 1 ).percent();
  }
};

struct odyns_fury_t : warrior_attack_t
{
  odyns_fury_main_hand_t* mh_attack;
  odyns_fury_main_hand_t* mh_attack2;
  odyns_fury_off_hand_t* oh_attack;
  odyns_fury_off_hand_t* oh_attack2;
  odyns_fury_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell )
    : warrior_attack_t( n, p, spell ),
      mh_attack( new odyns_fury_main_hand_t( p, fmt::format( "{}_mh", n ), spell->effectN( 1 ).trigger() ) ),
      mh_attack2( new odyns_fury_main_hand_t( p, fmt::format( "{}_mh", n ), spell->effectN( 3 ).trigger() ) ),
      oh_attack( new odyns_fury_off_hand_t( p, fmt::format( "{}_oh", n ), spell->effectN( 2 ).trigger() ) ),
      oh_attack2( new odyns_fury_off_hand_t( p, fmt::format( "{}_oh", n ), spell->effectN( 4 ).trigger() ) )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_attack->weapon = &( p->main_hand_weapon );
      add_child( mh_attack );
      mh_attack->radius = radius;
      mh_attack2->weapon = &( p->main_hand_weapon );
      mh_attack2->radius = radius;
      add_child( mh_attack2 );
      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        oh_attack->weapon = &( p->off_hand_weapon );
        add_child( oh_attack );
        oh_attack->radius = radius;
        oh_attack2->weapon = &( p->off_hand_weapon );
        add_child( oh_attack2 );
        oh_attack2->radius = radius;
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.fury.dancing_blades->ok() )
    {
      p()->buff.dancing_blades->trigger();
    } 

    if ( p()->talents.fury.titanic_rage->ok() )
    {
      p()->enrage();
      p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->max_stack() );
    }

    mh_attack->execute();
    oh_attack->execute();
    mh_attack2->execute();
    oh_attack2->execute();

    if ( p()->talents.warrior.titans_torment->ok() )
    {
      action_t* torment_ability = p()->active.torment_avatar;
      torment_ability->schedule_execute();
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Torment Odyn's Fury ==========================================================

struct torment_odyns_fury_t : warrior_attack_t
{
  odyns_fury_main_hand_t* mh_attack;
  odyns_fury_main_hand_t* mh_attack2;
  odyns_fury_off_hand_t* oh_attack;
  odyns_fury_off_hand_t* oh_attack2;
  torment_odyns_fury_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell, bool torment_triggered = false )
    : warrior_attack_t( n, p, spell ),
      mh_attack( new odyns_fury_main_hand_t( p, fmt::format( "{}_mh", n ), spell->effectN( 1 ).trigger() ) ),
      mh_attack2( new odyns_fury_main_hand_t( p, fmt::format( "{}_mh", n ), spell->effectN( 3 ).trigger() ) ),
      oh_attack( new odyns_fury_off_hand_t( p, fmt::format( "{}_oh", n ), spell->effectN( 2 ).trigger() ) ),
      oh_attack2( new odyns_fury_off_hand_t( p, fmt::format( "{}_oh", n ), spell->effectN( 4 ).trigger() ) )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_attack->weapon = &( p->main_hand_weapon );
      add_child( mh_attack );
      mh_attack->radius  = radius;
      mh_attack2->weapon = &( p->main_hand_weapon );
      mh_attack2->radius = radius;
      add_child( mh_attack2 );
      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        oh_attack->weapon = &( p->off_hand_weapon );
        add_child( oh_attack );
        oh_attack->radius  = radius;
        oh_attack2->weapon = &( p->off_hand_weapon );
        add_child( oh_attack2 );
        oh_attack2->radius = radius;
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.fury.dancing_blades->ok() )
    {
      p()->buff.dancing_blades->trigger();
    }

    if ( p()->talents.fury.titanic_rage->ok() )
    {
      p()->enrage();
      p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->max_stack() );
    }

    mh_attack->execute();
    oh_attack->execute();
    mh_attack2->execute();
    oh_attack2->execute();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Overpower ============================================================

struct seismic_wave_t : warrior_attack_t
{
  seismic_wave_t( warrior_t* p )
    : warrior_attack_t( "seismic_wave", p, p->find_spell( 278497 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 5.0;
    background  = true;
    base_dd_min = base_dd_max = p->azerite.seismic_wave.value( 1 );
  }
};

struct dreadnaught_t : warrior_attack_t
{
  dreadnaught_t( warrior_t* p )
    : warrior_attack_t( "dreadnaught", p, p->find_spell( 315961 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 5.0;
    background  = true;

    if ( p->talents.arms.battlelord->ok() )
    {
      base_multiplier *= 1.0 + p->talents.arms.battlelord->effectN( 1 ).percent();
    }
  }
};
struct overpower_t : public warrior_attack_t
{
  double battlelord_chance;
  double rage_from_strength_of_arms;
  warrior_attack_t* seismic_wave;
  warrior_attack_t* dreadnaught;

  overpower_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "overpower", p, p->talents.arms.overpower ),
      battlelord_chance( p->talents.arms.battlelord->proc_chance() ),
      rage_from_strength_of_arms( p->find_spell( 400806 )->effectN( 1 ).base_value() / 10.0 ),
      seismic_wave( nullptr ),
      dreadnaught( nullptr )
  {
    parse_options( options_str );
    may_block = may_parry = may_dodge = false;
    weapon                            = &( p->main_hand_weapon );
    if ( p->talents.arms.battlelord->ok() )
    {
      base_multiplier *= 1.0 + p->talents.arms.battlelord->effectN( 1 ).percent();
    }

    if ( p->talents.arms.dreadnaught->ok() )
    {
      cooldown->charges += as<int>( p->talents.arms.dreadnaught->effectN( 1 ).base_value() );
      dreadnaught = new dreadnaught_t( p );
      add_child( dreadnaught );
    }

    if ( p->azerite.seismic_wave.ok() )
    {
      seismic_wave = new seismic_wave_t( p );
      add_child( seismic_wave );
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    if ( p()->buff.striking_the_anvil->check() )
    {
      b += p()->buff.striking_the_anvil->value();
    }
    return b;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( seismic_wave && result_is_hit( s->result ) )
    {
      seismic_wave->set_target( s->target );
      seismic_wave->execute();
    }
    if ( dreadnaught && result_is_hit( s->result ) )
    {
      dreadnaught->set_target( s->target );
      dreadnaught->execute();
    }
    if ( p()->buff.striking_the_anvil->check() )
    {
      p()->cooldown.mortal_strike->adjust( - timespan_t::from_millis(
      p()->azerite.striking_the_anvil.spell()->effectN( 2 ).base_value() ) );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p()->talents.arms.battlelord->ok() && rng().roll( battlelord_chance ) )
    {
      p()->cooldown.mortal_strike->reset( true );
      p()->cooldown.cleave->reset( true );
      p() -> buff.battlelord -> trigger();
    }

    if ( p()->talents.arms.martial_prowess->ok() )
    {
    p()->buff.martial_prowess->trigger();
    }
    p()->buff.striking_the_anvil->expire();

    if ( p()->talents.arms.strength_of_arms->ok() && target->health_percentage() < 35 )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_strength_of_arms, p()->gain.strength_of_arms );
    }

  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Warbreaker ==============================================================================

struct warbreaker_t : public warrior_attack_t
{
  bool lord_of_war;
  double rage_from_lord_of_war;
  warbreaker_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "warbreaker", p, p->talents.arms.warbreaker ),
      lord_of_war( false ),
      rage_from_lord_of_war(
          //( p->azerite.lord_of_war.spell()->effectN( 1 ).base_value() * p->azerite.lord_of_war.n_items() ) / 10.0 )
          ( p->azerite.lord_of_war.spell()->effectN( 1 ).base_value() ) / 10.0 ) // 8.1 no extra rage with rank
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
    aoe    = -1;
    impact_action    = p->active.deep_wounds_ARMS;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->azerite.lord_of_war.value( 2 );
    return b;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->azerite.lord_of_war.ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_lord_of_war, p()->gain.lord_of_war );
    }

    if ( hit_any_target )
    {
      p()->buff.test_of_might_tracker->trigger();

      if ( p()->talents.arms.in_for_the_kill->ok() )
        p()->buff.in_for_the_kill->trigger();
    }

    if ( p()->talents.warrior.warlords_torment->ok() )
    {
      const timespan_t trigger_duration = p()->talents.warrior.warlords_torment->effectN( 2 ).time_value();
      p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs_colossus_smash->trigger();
    }
  }
};

// Rampage ================================================================

struct rampage_attack_t : public warrior_attack_t
{
  int aoe_targets;
  bool first_attack, first_attack_missed, valarjar_berserking, simmering_rage;
  double rage_from_valarjar_berserking;
  double rage_from_simmering_rage;
  double reckless_defense_chance;
  double hack_and_slash_chance;
  rampage_attack_t( warrior_t* p, const spell_data_t* rampage, util::string_view name )
    : warrior_attack_t( name, p, rampage ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      first_attack( false ),
      first_attack_missed( false ),
      valarjar_berserking( false ),
      simmering_rage( false ),
      rage_from_valarjar_berserking( p->find_spell( 248179 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_simmering_rage(
          ( p->azerite.simmering_rage.spell()->effectN( 1 ).base_value() ) / 10.0 ),
      reckless_defense_chance( p->legendary.reckless_defense->effectN( 2 ).percent() ),
      hack_and_slash_chance( p->talents.fury.hack_and_slash->proc_chance() )
  {
    background = true;
    dual = true;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
    if ( p->talents.fury.rampage->effectN( 3 ).trigger() == rampage )
      first_attack = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( first_attack && result_is_miss( execute_state->result ) )
      first_attack_missed = true;
    else if ( first_attack )
      first_attack_missed = false;

    if ( p()->talents.fury.hack_and_slash->ok() && rng().roll( hack_and_slash_chance ) )
    {
      p()->cooldown.raging_blow->reset( true );
      p()->cooldown.crushing_blow->reset( true );
    }
  }

  void impact( action_state_t* s ) override
  {
    if ( !first_attack_missed )
    {  // If the first attack misses, all of the rest do as well. However, if any other attack misses, the attacks after
       // continue. The animations and timing of everything else still occur, so we can't just cancel rampage.
      warrior_attack_t::impact( s );

       // s->target will only activate on strikes against the primary target, ignoring cleaved attacks.
      if ( p()->legendary.valarjar_berserkers != spell_data_t::not_found() && s->result == RESULT_CRIT &&
           target == s->target )
      {
        p()->resource_gain( RESOURCE_RAGE, rage_from_valarjar_berserking, p()->gain.valarjar_berserking );
      }
      if ( p()->azerite.simmering_rage.ok() && target == s->target )
      {
        p()->resource_gain( RESOURCE_RAGE, rage_from_simmering_rage, p()->gain.simmering_rage );
      }
      if ( p()->legendary.reckless_defense->ok() )
      {
        if ( target == s->target && execute_state->result == RESULT_HIT
        && rng().roll( reckless_defense_chance ) )
        {
          p()->cooldown.recklessness->adjust( - timespan_t::from_seconds( p()->legendary.reckless_defense->effectN( 1 ).base_value() ) );
        }
      }
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->buff.pulverizing_blows->stack_value();
    b += p()->azerite.simmering_rage.value( 2 );
    b += p()->azerite.unbridled_ferocity.value( 2 );
    return b;
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }
};

struct rampage_event_t : public event_t
{
  timespan_t duration;
  warrior_t* warrior;
  size_t attacks;
  rampage_event_t( warrior_t* p, size_t current_attack ) : event_t( *p->sim ), warrior( p ), attacks( current_attack )
  {
    duration = next_execute();
    schedule( duration );
    if ( sim().debug )
      sim().out_debug.printf( "New rampage event" );
  }

  timespan_t next_execute() const
  {
    timespan_t time_till_next_attack = timespan_t::zero();
    switch ( attacks )
    {
      case 0:
        break;  // First attack is instant.
      case 1:
        time_till_next_attack = timespan_t::from_millis( warrior->talents.fury.rampage->effectN( 3 ).misc_value1() );
        break;
      case 2:
        time_till_next_attack = timespan_t::from_millis( warrior->talents.fury.rampage->effectN( 4 ).misc_value1() -
                                                         warrior->talents.fury.rampage->effectN( 3 ).misc_value1() );
        break;
      case 3:
        time_till_next_attack = timespan_t::from_millis( warrior->talents.fury.rampage->effectN( 5 ).misc_value1() -
                                                         warrior->talents.fury.rampage->effectN( 4 ).misc_value1() );
        break;
    }
    return time_till_next_attack;
  }

  void execute() override
  {
    warrior->rampage_attacks[ attacks ]->execute();
    if ( attacks == 0 )
    {
      // Enrage/Frothing go here if not benefiting the first hit
    }
    attacks++;
    if ( attacks < warrior->rampage_attacks.size() )
    {
      warrior->rampage_driver = make_event<rampage_event_t>( sim(), warrior, attacks );
    }
    else
    {
      warrior->rampage_driver = nullptr;
      warrior->buff.meat_cleaver->decrement();
      warrior->buff.pulverizing_blows->expire();
      warrior->buff.slaughtering_strikes_rb->expire();
      warrior->buff.slaughtering_strikes_an->expire();
    }
  }
};

struct rampage_parent_t : public warrior_attack_t
{
  double cost_rage;
  double deathmaker_chance;
  double unbridled_chance;
  double frothing_berserker_chance;
  double hack_and_slash_conduit_chance;
  double rage_from_frothing_berserker;
  rampage_parent_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "rampage", p, p->talents.fury.rampage ),
    deathmaker_chance( p->legendary.deathmaker->proc_chance() ),
    unbridled_chance( p->talents.fury.unbridled_ferocity->effectN( 1 ).base_value() / 100.0 ),
    frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
    hack_and_slash_conduit_chance( p->conduit.hack_and_slash.percent() / 10.0 ),
    rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).base_value() / 100.0 )
  {
    parse_options( options_str );
    for ( auto* rampage_attack : p->rampage_attacks )
    {
      add_child( rampage_attack );
    }
    track_cd_waste = false;
    //base_costs[ RESOURCE_RAGE ] += p->talents.frothing_berserker->effectN( 1 ).resource( RESOURCE_RAGE );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    cost_rage = last_resource_cost;
    if ( p()->talents.warrior.frothing_berserker->ok() && rng().roll( frothing_berserker_chance ) )
    {
      p()->resource_gain(RESOURCE_RAGE, last_resource_cost * rage_from_frothing_berserker, p()->gain.frothing_berserker);
    }
    if ( p()->talents.fury.frenzy->ok() )
    {
      p()->buff.frenzy->trigger();
    }
    if ( p()->talents.fury.unbridled_ferocity.ok() && rng().roll( unbridled_chance ) )
    {
      const timespan_t trigger_duration = p()->talents.fury.unbridled_ferocity->effectN( 2 ).time_value();
      p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
    }
    if ( p()->talents.fury.reckless_abandon->ok() )
    {
      p()->buff.reckless_abandon->trigger();
    }
    if ( p()->legendary.deathmaker->ok() && ( result_is_hit( execute_state->result ) ) && rng().roll( deathmaker_chance ) )
    {
      if ( td( target )->debuffs_siegebreaker->up() )
      {
        td( target )->debuffs_siegebreaker->extend_duration( p(), timespan_t::from_millis( p()->legendary.deathmaker->effectN( 1 ).base_value() ) );
      }
      else
      {
        td( target )->debuffs_siegebreaker->trigger( timespan_t::from_millis( p()->legendary.deathmaker->effectN( 1 ).base_value() ) );
      }
    }
    if ( p()->conduit.hack_and_slash->ok() && rng().roll( hack_and_slash_conduit_chance && !p()->talents.fury.hack_and_slash->ok() ) ) // talent overrides conduit
    {
      p()->cooldown.raging_blow->reset( true );
      p()->cooldown.crushing_blow->reset( true );
    }
    if ( p()->azerite.trample_the_weak.ok() )
    {
      p()->buff.trample_the_weak->trigger();
    }
    if ( p()->tier_set.t30_fury_4pc->ok() )
    {
      p()->buff.merciless_assault->trigger();
    }

    p()->enrage();
    p()->rampage_driver = make_event<rampage_event_t>( *sim, p(), 0 );
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE || p()->off_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Ravager ==============================================================

struct ravager_tick_t : public warrior_attack_t
{
  double rage_from_ravager;
  ravager_tick_t( warrior_t* p, util::string_view name )
    : warrior_attack_t( name, p, p->find_spell( 156287 ) ), rage_from_ravager( 0.0 )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
    dual = ground_aoe = true;
    rage_from_ravager = p->find_spell( 334934 )->effectN( 1 ).resource( RESOURCE_RAGE );
    rage_from_ravager += p->talents.fury.storm_of_steel->effectN( 5 ).resource( RESOURCE_RAGE );
    rage_from_ravager += p->talents.protection.storm_of_steel->effectN( 5 ).resource( RESOURCE_RAGE );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( execute_state->n_targets > 0 )
      p()->resource_gain( RESOURCE_RAGE, rage_from_ravager, p()->gain.ravager );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();
    if( p() -> buff.dance_of_death_prot -> up() )
    {
      am *= 1.0 + p() -> buff.dance_of_death_prot -> value();
    }

    return am;
  }
};

struct ravager_t : public warrior_attack_t
{
  ravager_tick_t* ravager;
  // We have to use find_spell here, rather than use the talent lookup, as both fury and protection use the same spell_id
  ravager_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "ravager", p, p->talents.shared.ravager ),
      ravager( new ravager_tick_t( p, "ravager_tick" ) )
  {
    parse_options( options_str );
    ignore_false_positive   = true;
    hasted_ticks            = true;
    internal_cooldown->duration = 0_s; // allow Anger Management to reduce the cd properly due to having both charges and cooldown entries
    attack_power_mod.direct = attack_power_mod.tick = 0;
    add_child( ravager );

    // Vision of Perfection only reduces the cooldown for Arms
    if ( p->azerite.vision_of_perfection.enabled() && p->specialization() == WARRIOR_ARMS )
    {
      cooldown->duration *= 1.0 + azerite::vision_of_perfection_cdr( p->azerite.vision_of_perfection );
    }
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // Haste reduces dot time, as well as the tick time for Ravager
    timespan_t tt = tick_time( s );
    auto full_duration = dot_duration;

    if ( p() -> buff.dance_of_death_prot -> up () )
    {
      // Nov 2 2022 - Dance of death extends ravager by 2 seconds, but you do not get an extra tick.
      // As a result, not applying the time for now, as it doesn't work.
      if ( !p() -> bugs )
        full_duration += p() -> talents.protection.dance_of_death -> effectN( 1 ).trigger() -> effectN( 1 ).time_value();
    }

    return full_duration * ( tt / base_tick_time );
  }

  void execute() override
  {
    if ( p()->talents.shared.hurricane->ok() )
    {
      p()->buff.hurricane_driver->trigger();
    }

    warrior_attack_t::execute();
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    ravager->execute();
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );

    if ( p()->conduit.merciless_bonegrinder->ok() )
    {
      p()->buff.merciless_bonegrinder_conduit->trigger( timespan_t::from_seconds( 7.0 ) );
    }
  }
};

// Revenge ==================================================================

struct revenge_t : public warrior_attack_t
{
  double shield_slam_reset;
  action_t* seismic_action;
  double frothing_berserker_chance;
  double rage_from_frothing_berserker;
  revenge_t( warrior_t* p, util::string_view options_str, bool seismic = false )
    : warrior_attack_t( seismic ? "revenge_seismic_reverberation" : "revenge", p, p->talents.protection.revenge ),
      shield_slam_reset( p->talents.protection.strategist->effectN( 1 ).percent() ),
      seismic_action( nullptr ),
      frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
      rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).base_value() / 100.0 )
    {
      parse_options( options_str );
      aoe           = -1;
      impact_action = p->active.deep_wounds_PROT;
      base_multiplier *= 1.0 + p -> talents.protection.best_served_cold -> effectN( 1 ).percent();
      if ( seismic )
      {
        background = proc = true;
        base_multiplier *= 1.0 + p -> find_spell( 382956 ) -> effectN( 3 ).percent();
      }
      else if ( p -> talents.warrior.seismic_reverberation -> ok() )
      {
        seismic_action = new revenge_t( p, "", true );
        add_child( seismic_action );
      }
  }

  double cost() const override
  {
    double cost = warrior_attack_t::cost();
    cost *= 1.0 + p()->buff.revenge->check_value();
    //cost *= 1.0 + p()->buff.vengeance_revenge->check_value();
    return cost;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p()->buff.revenge->expire();

    if ( p()->talents.warrior.seismic_reverberation->ok() && !background &&
    execute_state->n_targets >= p()->talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      seismic_action->execute_on_target( target );
    }

    if ( rng().roll( shield_slam_reset ) )
      p()->cooldown.shield_slam->reset( true );

    if ( p()->talents.protection.show_of_force->ok() )
    {
      p()->buff.show_of_force->trigger();
    }

    if ( p()->talents.warrior.frothing_berserker->ok() && !background && rng().roll( frothing_berserker_chance ) )
    {
      p()->resource_gain(RESOURCE_RAGE, last_resource_cost * rage_from_frothing_berserker, p()->gain.frothing_berserker);
    }

    if ( p() -> sets->has_set_bonus( WARRIOR_PROTECTION, T29, B2 ) )
      p()->buff.vanguards_determination->trigger();
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );
    if ( p() -> azerite.callous_reprisal.enabled() )
    {
      td( state -> target ) -> debuffs_callous_reprisal -> trigger();
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->azerite.callous_reprisal.value( 2 );
    return b;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();
    if( p() -> buff.revenge -> up() && p() -> talents.protection.best_served_cold -> ok() )
    {
      am /= 1.0 + p()->talents.protection.best_served_cold->effectN( 1 ).percent();
      am *= 1.0 + p()->talents.protection.best_served_cold->effectN( 1 ).percent() +
            p()->buff.revenge->data().effectN( 2 ).percent();
    }

    am *= 1.0 + p() -> talents.protection.show_of_force -> effectN( 2 ).percent();

    return am;
  }
};

// Enraged Regeneration ===============================================

struct enraged_regeneration_t : public warrior_heal_t
{
  enraged_regeneration_t( warrior_t* p, util::string_view options_str )
    : warrior_heal_t( "enraged_regeneration", p, p->talents.fury.enraged_regeneration )
  {
    parse_options( options_str );
    range         = -1;
    base_pct_heal = data().effectN( 1 ).percent();
  }
};

// Prot Rend ==============================================================

struct rend_dot_prot_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  rend_dot_prot_t( warrior_t* p ) : warrior_attack_t( "rend", p, p->find_spell( 394063 ) ),
    bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
    rage_from_bloodsurge( p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }
  }
};

struct rend_prot_t : public warrior_attack_t
{
  warrior_attack_t* rend_dot;
  rend_prot_t( warrior_t* p, util::string_view options_str ) : warrior_attack_t( "rend", p, p->talents.protection.rend ),
  rend_dot( nullptr )
  {
    parse_options( options_str );
    tick_may_crit = true;
    hasted_ticks  = true;
    // Arma: 2022 Nov 4th.  The trigger spell triggers the arms version of rend dot, even though the tooltip references the prot version.
    if ( p -> bugs )
      rend_dot = new rend_dot_t( p );
    else
      rend_dot = new rend_dot_prot_t( p );
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

      rend_dot->set_target( s->target );
      rend_dot->execute();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Shield Charge ============================================================

struct shield_charge_damage_t : public warrior_attack_t
{
  double rage_gain;
  shield_charge_damage_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->find_spell( 385954 ) ),
    rage_gain( p->find_spell( 385954 )->effectN( 4 ).resource( RESOURCE_RAGE ) )
  {
    background = true;
    may_miss = false;
    energize_type = action_energize::NONE;

    aoe = 0;
    // this spell has both coefficients in it, force #1
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

    rage_gain += p->talents.protection.champions_bulwark->effectN( 2 ).resource( RESOURCE_RAGE );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.protection.champions_bulwark->ok() )
    {
      am *= 1.0 + p()->talents.protection.champions_bulwark->effectN( 3 ).percent();
    }
    return am;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.protection.champions_bulwark->ok() )
    {
    if ( p()->buff.shield_block->check() )
    {
      p()->buff.shield_block->extend_duration( p(), p() -> buff.shield_block->buff_duration() );
    }
    else
    {
      p()->buff.shield_block->trigger();
    }
      p()->buff.revenge->trigger();
    }

    if ( p()->talents.protection.battering_ram->ok() )
    {
      p()->buff.battering_ram->trigger();
    }

    p()->resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.shield_charge );
  }
};

struct shield_charge_damage_aoe_t : public warrior_attack_t
{
  shield_charge_damage_aoe_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->find_spell( 385954 ) )
  {
    background = true;
    may_miss = false;
    energize_type = action_energize::NONE;

    aoe = -1;

    // this spell has both coefficients in it, force #2
    attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    warrior_attack_t::available_targets( tl );
    // Remove our main target from the target list, as aoe hits all other mobs
    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.protection.champions_bulwark->ok() )
    {
      am *= 1.0 + p()->talents.protection.champions_bulwark->effectN( 3 ).percent();
    }
    return am;
  }
};

struct shield_charge_t : public warrior_attack_t
{
  double movement_speed_increase;
  action_t* shield_charge_damage;
  action_t* shield_charge_damage_aoe;
  shield_charge_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "shield_charge", p, p->talents.protection.shield_charge ),
      movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    energize_type = action_energize::NONE;
    movement_directionality = movement_direction_type::OMNI;

    shield_charge_damage = get_action<shield_charge_damage_t>( "shield_charge_main", p );
    shield_charge_damage_aoe = get_action<shield_charge_damage_aoe_t>( "shield_charge_aoe", p );
    add_child( shield_charge_damage );
    add_child( shield_charge_damage_aoe );
  }

  timespan_t calc_charge_time( double distance )
  {
    return timespan_t::from_seconds( distance /
      ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) );
  }

  void execute() override
  {
    if ( p()->current.distance_to_move > 0 )
    {
      p()->buff.shield_charge_movement->trigger( 1, movement_speed_increase, 1, calc_charge_time( p()->current.distance_to_move ) );
      p()->current.moving_away = 0;
    }
    warrior_attack_t::execute();

    shield_charge_damage->execute_on_target( target );
    // If we have more than one target, trigger aoe as well
    if ( sim -> target_non_sleeping_list.size() > 1 )
      shield_charge_damage_aoe->execute_on_target( target );
  }

  bool ready() override
  {
    if ( p()->buff.charge_movement->check() || p()->buff.heroic_leap_movement->check() ||
         p()->buff.intervene_movement->check() || p()->buff.shield_charge_movement->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Shield Slam ==============================================================

// Linked action for shield slam aoe with T30 Protection
struct earthen_smash_t : public warrior_attack_t
{
  earthen_smash_t( util::string_view name, warrior_t* p )
  : warrior_attack_t( name, p, p->find_spell( 410219 ) )
  {
    background = true;
    aoe = -1;
  }
};

struct shield_slam_t : public warrior_attack_t
{
  double rage_gain;
  action_t* earthen_smash;
  shield_slam_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "shield_slam", p, p->spell.shield_slam ),
    rage_gain( p->spell.shield_slam->effectN( 3 ).resource( RESOURCE_RAGE ) ),
    earthen_smash( get_action<earthen_smash_t>( "earthen_smash", p ))
  {
    parse_options( options_str );
    energize_type = action_energize::NONE;
    rage_gain += p->talents.protection.heavy_repercussions->effectN( 2 ).resource( RESOURCE_RAGE );
    rage_gain += p->talents.protection.impenetrable_wall->effectN( 2 ).resource( RESOURCE_RAGE );

    if ( p -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B2 ) )
        base_multiplier *= 1.0 + p -> sets -> set( WARRIOR_PROTECTION, T30, B2 ) -> effectN( 1 ).percent();
  }

    void init() override
  {
    warrior_attack_t::init();
    rage_gain += p()->legendary.the_wall->effectN( 2 ).resource( RESOURCE_RAGE );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.shield_block->up() )
    {
      double sb_increase = p() -> spell.shield_block_buff -> effectN( 2 ).percent();
      am *= 1.0 + sb_increase;
    }

    if ( p()->talents.protection.punish.ok() )
    {
      am *= 1.0 + p()->talents.protection.punish->effectN( 1 ).percent();
    }

    if ( p()->buff.violent_outburst->check() )
    {
      am *= 1.0 + p()->buff.violent_outburst->data().effectN( 1 ).percent();
    }

    if ( p() -> buff.brace_for_impact -> up() )
    {
      am *= 1.0 + p()->buff.brace_for_impact -> stack_value();
    }

    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B2 ) && p() -> buff.last_stand -> up() )
    {
        am *= 1.0 + p() -> talents.protection.last_stand -> effectN( 3 ).percent();
    }

    return am;
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double da = warrior_attack_t::bonus_da( state );

    da += p() -> buff.brace_for_impact_az -> check() * p()->azerite.brace_for_impact.value(2);

    return da;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->buff.shield_block->up() && p()->talents.protection.heavy_repercussions->ok() )
    {
      p () -> buff.shield_block -> extend_duration( p(),
          timespan_t::from_seconds( p() -> talents.protection.heavy_repercussions -> effectN( 1 ).percent() ) );
    }

    if ( p()->talents.protection.impenetrable_wall->ok() )
    {
      p()->cooldown.shield_wall->adjust( - timespan_t::from_seconds( p()->talents.protection.impenetrable_wall->effectN( 1 ).base_value() ) );
    }

    auto total_rage_gain = rage_gain;

    if ( p() -> azerite.brace_for_impact.enabled() )
    {
      p() -> buff.brace_for_impact_az -> trigger();
    }

    if ( p() -> talents.protection.brace_for_impact->ok() )
      p() -> buff.brace_for_impact -> trigger();

    if ( p()->buff.violent_outburst->check() )
    {
      p()->buff.ignore_pain->trigger();
      p()->buff.violent_outburst->expire();
      total_rage_gain *= 1.0 + p() -> buff.violent_outburst->data().effectN( 3 ).percent();
    }

    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B2 ) ) 
    {
      p()->cooldown.last_stand->adjust( - timespan_t::from_seconds( p() -> sets -> set(WARRIOR_PROTECTION, T30, B2 ) -> effectN( 2 ).base_value() ) );
      // Value is doubled with last stand up, so we apply the same effect twice.
      if ( p() -> buff.last_stand -> up() )
      {
        p()->cooldown.last_stand->adjust( - timespan_t::from_seconds( p() -> sets -> set(WARRIOR_PROTECTION, T30, B2 ) -> effectN( 2 ).base_value() ) );
      }
    }

    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B4 ) && p() -> buff.earthen_tenacity -> up() )
    {
      earthen_smash -> execute_on_target( target );
    }

    p()->resource_gain( RESOURCE_RAGE, total_rage_gain, p() -> gain.shield_slam );
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    warrior_td_t* td = p() -> get_target_data( state -> target );

    if ( p()->talents.protection.punish.ok() )
    {
      td -> debuffs_punish -> trigger();
    }
  }

  bool ready() override
  {
    if ( !p()->has_shield_equipped() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Slam =====================================================================

struct slam_t : public warrior_attack_t
{
  bool from_Fervor;
  int aoe_targets;
  slam_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "slam", p, p->spell.slam ), from_Fervor( false ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    weapon                       = &( p->main_hand_weapon );
    affected_by.crushing_assault = true;
    if ( p->talents.fury.storm_of_swords->ok() )
    {
      energize_amount += p->talents.fury.storm_of_swords->effectN( 6 ).resource( RESOURCE_RAGE );
    }
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    if ( p()->buff.crushing_assault->check() )
    {
      b += p()->buff.crushing_assault->value();
    }
    return b;
  }

  double cost() const override
  {
    if ( from_Fervor )
      return 0;

    if ( p()->buff.crushing_assault->check() && !from_Fervor )
      return 0;
    return warrior_attack_t::cost();
  }

  double tactician_cost() const override
  {
    if ( from_Fervor )
      return 0;
    return warrior_attack_t::cost();
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.meat_cleaver->decrement();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Shockwave ================================================================

struct shockwave_t : public warrior_attack_t
{
  shockwave_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "shockwave", p, p->talents.warrior.shockwave )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
    aoe                               = -1;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( state -> n_targets >= as<size_t>( p() -> talents.warrior.rumbling_earth->effectN( 1 ).base_value() ) )
    {
      p()->cooldown.shockwave->adjust(
          timespan_t::from_seconds( p()->talents.warrior.rumbling_earth->effectN( 2 ).base_value() ) );
    }
  }
};

// Storm Bolt ===============================================================

struct storm_bolt_t : public warrior_attack_t
{
  storm_bolt_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "storm_bolt", p, p->talents.warrior.storm_bolt )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Titanic Throw ============================================================

struct titanic_throw_t : public warrior_attack_t
{
  titanic_throw_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "titanic_throw", p, p->talents.warrior.titanic_throw )
    {
      parse_options( options_str );
      may_dodge = may_parry = may_block = false;
      aoe                               = as<int>( p->talents.warrior.titanic_throw->effectN( 2 ).base_value() );
    }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.protection.improved_heroic_throw->ok() )
    {
      am *= 1.0 + p()->talents.protection.improved_heroic_throw->effectN( 2 ).percent();
    }

    return am;
  }
};

// Tough as Nails ===========================================================

struct tough_as_nails_t : public warrior_attack_t
{
  tough_as_nails_t( warrior_t* p ) :
    warrior_attack_t( "tough_as_nails", p, p -> find_spell( 385890 ) )
  {
    may_crit = false;
    ignores_armor = true;

    background = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p() -> cooldown.tough_as_nails_icd -> start();
  }
};

// Thunder Clap =============================================================

struct thunder_clap_t : public warrior_attack_t
{
  double rage_gain;
  double shield_slam_reset;
  warrior_attack_t* blood_and_thunder;
  double blood_and_thunder_target_cap;
  double blood_and_thunder_targets_hit;
  thunder_clap_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "thunder_clap", p, p->talents.warrior.thunder_clap ),
      rage_gain( data().effectN( 4 ).resource( RESOURCE_RAGE ) ),
      shield_slam_reset( p->talents.protection.strategist->effectN( 1 ).percent() ),
      blood_and_thunder( nullptr ),
      blood_and_thunder_target_cap( 0 ),
      blood_and_thunder_targets_hit( 0 )
  {
    parse_options( options_str );
    aoe       = -1;
    may_dodge = may_parry = may_block = false;

    radius *= 1.0 + p->talents.warrior.crackling_thunder->effectN( 1 ).percent();
    energize_type = action_energize::NONE;

    if ( p->specialization() == WARRIOR_ARMS || p->specialization() == WARRIOR_FURY )
    {
    base_costs[ RESOURCE_RAGE ] += p->talents.warrior.blood_and_thunder->effectN( 2 ).resource( RESOURCE_RAGE );
    }
    if ( p -> spec.thunder_clap_prot_hidden )
      rage_gain += p -> spec.thunder_clap_prot_hidden -> effectN( 1 ).resource( RESOURCE_RAGE );

    if ( p->talents.warrior.blood_and_thunder.ok() )
    {
      blood_and_thunder_target_cap = p->talents.warrior.blood_and_thunder->effectN( 3 ).base_value();
      if ( p->talents.arms.rend->ok() )
        blood_and_thunder = new rend_dot_t( p );
      if ( p->talents.protection.rend->ok() )
      {
        // Arma: 2022 Nov 4th.  Even if you are prot, the arms rend dot is being applied.
        if ( p -> bugs )
          blood_and_thunder = new rend_dot_t( p );
        else
          blood_and_thunder = new rend_dot_prot_t( p );
      }
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->buff.avatar->up() && p()->talents.warrior.unstoppable_force->ok() )
    {
      am *= 1.0 + p()->talents.warrior.unstoppable_force->effectN( 1 ).percent();
    }

    if ( p()->buff.show_of_force->check() )
    {
      am *= 1.0 + ( p()-> buff.show_of_force -> stack_value() );
    }

    if ( p()->buff.violent_outburst->check() )
    {
      am *= 1.0 + p()->buff.violent_outburst->data().effectN( 1 ).percent();
    }

    if ( p()->talents.warrior.blood_and_thunder.ok() )
    {
      am *= 1.0 + p()->talents.warrior.blood_and_thunder->effectN( 1 ).percent();
    }

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = warrior_attack_t::bonus_da( s );

    if ( p() -> azerite.deafening_crash.enabled() )
    {
      da += p() -> azerite.deafening_crash.value( 2 );
    }

    return da;
  }

  void execute() override
  {
    blood_and_thunder_targets_hit = 0;

    warrior_attack_t::execute();

    if ( p()->buff.show_of_force->up() )
    {
      p()->buff.show_of_force->expire();
    }

    if ( rng().roll( shield_slam_reset ) )
    {
      p()->cooldown.shield_slam->reset( true );
    }

    if ( p()->legendary.thunderlord->ok() )
    {
     p() -> cooldown.demoralizing_shout -> adjust( - p() -> legendary.thunderlord -> effectN( 1 ).time_value() *
          std::min( execute_state->n_targets, as<unsigned int>( p()->legendary.thunderlord->effectN( 2 ).base_value() ) ) );
    }

    if ( p() -> talents.protection.thunderlord.ok() )
    {
      p() -> cooldown.demoralizing_shout -> adjust( - p() -> talents.protection.thunderlord -> effectN( 1 ).time_value() *
          std::min( execute_state->n_targets, as<unsigned int>( p() -> talents.protection.thunderlord -> effectN ( 2 ).base_value() ) ) );
    }

    auto total_rage_gain = rage_gain;

    if ( p()->buff.violent_outburst->check() )
    {
      p()->buff.ignore_pain->trigger();
      p()->buff.violent_outburst->expire();
      total_rage_gain *= 1.0 + p() -> buff.violent_outburst -> data().effectN( 4 ).percent();
    }

    p()->resource_gain( RESOURCE_RAGE, total_rage_gain, p() -> gain.thunder_clap );
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = warrior_attack_t::recharge_multiplier( cd );
    if ( p()->buff.avatar->up() && p()->talents.warrior.unstoppable_force->ok() )
    {
      rm *= 1.0 + ( p()->talents.warrior.unstoppable_force->effectN( 2 ).percent() );
    }
    return rm;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( p() -> azerite.deafening_crash.enabled() && td( state -> target ) -> debuffs_demoralizing_shout -> up() )
    {
      td( state -> target ) -> debuffs_demoralizing_shout -> extend_duration( p(), timespan_t::from_millis( p() -> azerite.deafening_crash.spell() -> effectN( 1 ).base_value() ) );
    }
    if ( ( p()->talents.arms.rend->ok() || p()->talents.protection.rend->ok() ) && p()->talents.warrior.blood_and_thunder.ok() )
    {
      if ( blood_and_thunder_targets_hit < blood_and_thunder_target_cap )
      {
        blood_and_thunder->execute_on_target( state->target );
        blood_and_thunder_targets_hit++;
      }
    }
  }
};

// Victory Rush =============================================================

struct victory_rush_heal_t : public warrior_heal_t
{
  victory_rush_heal_t( warrior_t* p ) : warrior_heal_t( "victory_rush_heal", p, p->find_spell( 118779 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background    = true;
  }
  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

struct victory_rush_t : public warrior_attack_t
{
  victory_rush_heal_t* victory_rush_heal;

  victory_rush_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "victory_rush", p, p->spell.victory_rush ), victory_rush_heal( new victory_rush_heal_t( p ) )
  {
    parse_options( options_str );
    if ( p->non_dps_mechanics )
    {
      execute_action = victory_rush_heal;
    }
    cooldown->duration = timespan_t::from_seconds( 1000.0 );
  }
};

// Whirlwind ================================================================

struct whirlwind_off_hand_t : public warrior_attack_t
{
  whirlwind_off_hand_t( warrior_t* p, const spell_data_t* whirlwind ) : warrior_attack_t( "whirlwind_off-hand", p, whirlwind )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = 5.0;


    base_multiplier *= 1.0 + p->talents.fury.meat_cleaver->effectN( 1 ).percent();
  }

  int current_tick;

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.warrior.seismic_reverberation->ok() && current_tick == 3 )
    {
      am *= 1.0 + p()->talents.warrior.seismic_reverberation->effectN( 3 ).percent();
    }
    if ( p()->buff.merciless_bonegrinder_conduit->check() )
    {
      am *= 1.0 + p()->conduit.merciless_bonegrinder.percent();
    }
    return am;
  }
};

struct fury_whirlwind_mh_t : public warrior_attack_t
{
  fury_whirlwind_mh_t( warrior_t* p, const spell_data_t* whirlwind ) : warrior_attack_t( "whirlwind_mh", p, whirlwind )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = 5.0;

    base_multiplier *= 1.0 + p->talents.fury.meat_cleaver->effectN(1).percent();
  }

  int current_tick;

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.warrior.seismic_reverberation->ok() && current_tick == 3 )
    {
      am *= 1.0 + p()->talents.warrior.seismic_reverberation->effectN( 3 ).percent();
    }
    if ( p()->buff.merciless_bonegrinder_conduit->check() )
    {
      am *= 1.0 + p()->conduit.merciless_bonegrinder.percent();
    }
    return am;
  }
};

struct fury_whirlwind_parent_t : public warrior_attack_t
{
  whirlwind_off_hand_t* oh_attack;
  fury_whirlwind_mh_t* mh_attack;
  timespan_t spin_time;
  double base_rage_gain;
  double additional_rage_gain_per_target;
  fury_whirlwind_parent_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "whirlwind", p, p->spec.whirlwind ),
      oh_attack( nullptr ),
      mh_attack( nullptr ),
      spin_time( timespan_t::from_millis( p->spec.whirlwind->effectN( 6 ).misc_value1() ) ),
      base_rage_gain( p->spec.whirlwind->effectN( 1 ).base_value() ),
      additional_rage_gain_per_target( p->spec.whirlwind->effectN( 2 ).base_value() )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_attack         = new fury_whirlwind_mh_t( p, data().effectN( 4 ).trigger() );
      mh_attack->weapon = &( p->main_hand_weapon );
      mh_attack->radius = radius;
      add_child( mh_attack );
      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        oh_attack         = new whirlwind_off_hand_t( p, data().effectN( 5 ).trigger() );
        oh_attack->weapon = &( p->off_hand_weapon );
        oh_attack->radius = radius;
        add_child( oh_attack );
      }
    }
    tick_zero = true;
    hasted_ticks = false;
    base_tick_time           = spin_time;
    dot_duration             = base_tick_time * 2;
  }

  timespan_t composite_dot_duration( const action_state_t* /* s */ ) const override
  {
    if ( p()->legendary.najentuss_vertebrae != spell_data_t::not_found() &&
         as<int>( target_list().size() ) >= p()->legendary.najentuss_vertebrae->effectN( 1 ).base_value() )
    {
      return dot_duration + ( base_tick_time * p()->legendary.najentuss_vertebrae->effectN( 2 ).base_value() );
    }
    if ( p()->talents.warrior.seismic_reverberation != spell_data_t::not_found() &&
         as<int>( target_list().size() ) >= p()->talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      return dot_duration + ( base_tick_time * p()->talents.warrior.seismic_reverberation->effectN( 2 ).base_value() );
    }

    return dot_duration;
  }

  void tick( dot_t* d ) override
  {
    mh_attack->current_tick = d->current_tick;
    if ( oh_attack )
    {
      oh_attack->current_tick = d->current_tick;
    }

    warrior_attack_t::tick( d );

    if ( mh_attack )
    {
      mh_attack->execute();
      if ( oh_attack )
      {
        oh_attack->execute();
      }
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );

    if ( p()->talents.fury.improved_whirlwind->ok() )
    {
      p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->max_stack() );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    const int num_available_targets = std::min( 5, as<int>( target_list().size() ));  // Capped to 5 targets 

    if ( p()->talents.fury.improved_whirlwind->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, ( base_rage_gain + additional_rage_gain_per_target * num_available_targets ),
                        p()->gain.whirlwind );
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Arms Whirlwind ========================================================

struct arms_whirlwind_mh_t : public warrior_attack_t
{
  arms_whirlwind_mh_t( warrior_t* p, const spell_data_t* whirlwind ) : warrior_attack_t( "whirlwind_mh", p, whirlwind )
  {
    aoe = -1;
    reduced_aoe_targets = 5.0;
    background = true;
  }

  int current_tick;

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.warrior.seismic_reverberation->ok() && current_tick == 3 )
    {
      am *= 1.0 + p()->talents.warrior.seismic_reverberation->effectN( 3 ).percent();
    }
    if ( p()->buff.merciless_bonegrinder_conduit->check() )
    {
      am *= 1.0 + p()->conduit.merciless_bonegrinder.percent();
    }
    return am;
  }

  double tactician_cost() const override
  {
    return 0;
  }
};

struct first_arms_whirlwind_mh_t : public warrior_attack_t
{
  first_arms_whirlwind_mh_t( warrior_t* p, const spell_data_t* whirlwind )
    : warrior_attack_t( "whirlwind_mh", p, whirlwind )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = 5.0;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->buff.merciless_bonegrinder_conduit->check() )
    {
      am *= 1.0 + p()->conduit.merciless_bonegrinder.percent();
    }
    return am;
  }

  double tactician_cost() const override
  {
    return 0;
  }
};

struct arms_whirlwind_parent_t : public warrior_attack_t
{
  double max_rage;
  slam_t* fervor_slam;
  first_arms_whirlwind_mh_t* first_mh_attack;
  arms_whirlwind_mh_t* mh_attack;
  timespan_t spin_time;
  arms_whirlwind_parent_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "whirlwind", p, p->spell.whirlwind ),
      fervor_slam( nullptr ),
      first_mh_attack( nullptr ),
      mh_attack( nullptr ),
      spin_time( timespan_t::from_millis( p->spell.whirlwind->effectN( 2 ).misc_value1() ) )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->talents.arms.fervor_of_battle->ok() )
    {
      fervor_slam                               = new slam_t( p, options_str );
      fervor_slam->from_Fervor                  = true;
      fervor_slam->affected_by.crushing_assault = true;
    }

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_attack         = new arms_whirlwind_mh_t( p, data().effectN( 2 ).trigger() );
      mh_attack->weapon = &( p->main_hand_weapon );
      mh_attack->radius = radius;
      add_child( mh_attack );
      first_mh_attack         = new first_arms_whirlwind_mh_t( p, data().effectN( 1 ).trigger() );
      first_mh_attack->weapon = &( p->main_hand_weapon );
      first_mh_attack->radius = radius;
      add_child( first_mh_attack );
    }
    tick_zero = true;
    hasted_ticks = false;
    base_tick_time           = spin_time;
    dot_duration             = base_tick_time * 2;
  }

  timespan_t composite_dot_duration( const action_state_t* /* s */ ) const override
  {
    if ( p()->legendary.najentuss_vertebrae != spell_data_t::not_found() &&
         as<int>( target_list().size() ) >= p()->legendary.najentuss_vertebrae->effectN( 1 ).base_value() )
    {
      return dot_duration + ( base_tick_time * p()->legendary.najentuss_vertebrae->effectN( 2 ).base_value() );
    }
    if ( p()->talents.warrior.seismic_reverberation != spell_data_t::not_found() &&
         as<int>( target_list().size() ) >= p()->talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      return dot_duration + ( base_tick_time * p()->talents.warrior.seismic_reverberation->effectN( 2 ).base_value() );
    }

    return dot_duration;
  }

  double cost() const override
  {
    if ( p()->talents.arms.fervor_of_battle->ok() && p()->buff.crushing_assault->check() )
      return 10;
    return warrior_attack_t::cost();
  }

  double tactician_cost() const override
  {
    double c = warrior_attack_t::cost();

    if ( sim->log )
    {
      sim->out_debug.printf( "Rage used to calculate tactician chance from ability %s: %4.4f, actual rage used: %4.4f",
                             name(), c, cost() );
    }
    return c;
  }

  void tick( dot_t* d ) override
  {
    mh_attack->current_tick = d->current_tick;
    warrior_attack_t::tick( d );

    if ( d->current_tick == 1 )
    {
      if ( p()->talents.arms.fervor_of_battle->ok() )
      {
        fervor_slam->execute();
      }
      first_mh_attack->execute();
    }
    else
    {
      mh_attack->execute();
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Wrecking Throw ========================================================

struct wrecking_throw_t : public warrior_attack_t
{
  wrecking_throw_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "wrecking_throw", p, p->talents.warrior.wrecking_throw )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
    attack_power_mod.direct = 1.0;
  }
  // add absorb shield bonus (are those even in SimC?)
};

// ==========================================================================
// Covenant Abilities
// ==========================================================================

// Ancient Aftershock========================================================

struct ancient_aftershock_pulse_t : public warrior_attack_t
{
  ancient_aftershock_pulse_t( warrior_t* p ) : warrior_attack_t( "ancient_aftershock_pulse", p, p->find_spell( 326062 ) )
  {
    background = true;
    aoe               = 5;
    energize_amount   = p->find_spell( 326076 )->effectN( 1 ).base_value() / 10.0;
    energize_type     = action_energize::PER_HIT;
    energize_resource = RESOURCE_RAGE;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( p()->active.natures_fury )
    {
      p()->active.natures_fury->set_target( s->target );
      p()->active.natures_fury->execute();
    }
   }
};

struct natures_fury_dot_t : public warrior_attack_t
{
  natures_fury_dot_t( warrior_t* p ) : warrior_attack_t( "natures_fury", p, p->find_spell( 354163 ) )
  {
    //background = tick_may_crit = true;
    //hasted_ticks               = false;
  }
};

struct ancient_aftershock_t : public warrior_attack_t
{
  ancient_aftershock_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "ancient_aftershock", p, p->covenant.ancient_aftershock )
  {
    parse_options( options_str );
    aoe       = -1;
    may_dodge = may_parry = may_block = false;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    timespan_t duration = p()->spell.aftershock_duration->duration();
    if ( p()->legendary.natures_fury->ok() )
      duration += p()->legendary.natures_fury->effectN( 1 ).time_value();

    make_event<ground_aoe_event_t>( *p()->sim, p(), ground_aoe_params_t()
      .target( execute_state->target )
      .pulse_time( timespan_t::from_seconds( 3.0 ) ) // hard coded by interns
      .duration( duration )
      .action( p()->active.ancient_aftershock_pulse ) );
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( p()->active.natures_fury )
    {
      p()->active.natures_fury->set_target( s->target );
      p()->active.natures_fury->execute();
    }
   }
};

// Arms Condemn==============================================================

struct condemn_damage_t : public warrior_attack_t
{
  double max_rage;
  double cost_rage;
  condemn_damage_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "condemn", p, p->covenant.condemn->effectN( 1 ).trigger() ), max_rage( 40 )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
    background = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( cost_rage == 0 )  // If it was free, it's a full damage condemn.
      am *= 2.0;
    else
      am *= 2.0 * ( std::min( max_rage, cost_rage ) / max_rage );

    if ( p()->conduit.harrowing_punishment->ok() )
    {
      size_t num_targets = std::min( p()->sim->target_non_sleeping_list.size(), (size_t) 5);
      am *= 1.0 + p()->conduit.harrowing_punishment.percent() * num_targets;
    }

    return am;
  }
};

struct condemn_arms_t : public warrior_attack_t
{
  condemn_damage_t* trigger_attack;
  double max_rage;
  double execute_pct_above;
  double execute_pct_below;
  condemn_arms_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "condemn", p, p->covenant.condemn ), max_rage( 40 ), execute_pct_above( 80 ), execute_pct_below( 20 )
  {
    parse_options( options_str );
    weapon        = &( p->main_hand_weapon );

    trigger_attack = new condemn_damage_t( p, options_str );

    if ( p->talents.arms.massacre->ok() )
    {
      execute_pct_below = p->talents.arms.massacre->effectN( 2 ).base_value();
    }
    if ( p->talents.protection.massacre->ok() )
    {
      execute_pct_below = p->talents.protection.massacre->effectN( 2 ).base_value();
    }
  }

  double tactician_cost() const override
  {
    double c;

    if ( !p()->buff.ayalas_stone_heart->check() && !p()->buff.sudden_death->check() )
    {
      c = std::min( max_rage, p()->resources.current[ RESOURCE_RAGE ] );
      c = ( c / max_rage ) * 40;
    }
    else
    {
      c = 0;
    }
    if ( sim->log )
    {
      sim->out_debug.printf( "Rage used to calculate tactician chance from ability %s: %4.4f, actual rage used: %4.4f",
                             name(), c, cost() );
    }
    return c;
  }

  double cost() const override
  {
    double c = warrior_attack_t::cost();
    c        = std::min( max_rage, std::max( p()->resources.current[ RESOURCE_RAGE ], c ) );

    if ( p()->buff.ayalas_stone_heart->check() )
    {
      c *= 1.0 + p()->buff.ayalas_stone_heart->data().effectN( 2 ).percent();
    }
    if ( p()->buff.sudden_death->check() )
    {
      return 0;  // The cost reduction isn't in the spell data
    }
    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    trigger_attack->cost_rage = last_resource_cost;
    trigger_attack->execute();
    if ( p()->talents.arms.improved_execute->ok() && !p()->talents.arms.critical_thinking->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, last_resource_cost * p()->find_spell( 163201 )->effectN( 2 ).percent(),
                          p()->gain.execute_refund );  // Not worth the trouble to check if the target died.
    }
    if ( p()->talents.arms.improved_execute->ok() && p()->talents.arms.critical_thinking->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, last_resource_cost * ( p()->talents.arms.critical_thinking->effectN( 2 ).percent() +
                                                 p()->find_spell( 163201 )->effectN( 2 ).percent() ),
                          p()->gain.execute_refund );  // Not worth the trouble to check if the target died.
    }

    if (p()->buff.sudden_death->up())
    {
      p()->buff.sudden_death->expire();
    }
    if ( p()->talents.arms.executioners_precision->ok() && ( result_is_hit( execute_state->result ) ) )
    {
      td( execute_state->target )->debuffs_executioners_precision->trigger();
    }
    if ( p()->legendary.exploiter.ok() && !p()->talents.arms.executioners_precision->ok() && ( result_is_hit( execute_state->result ) ) )
    {
      td( execute_state->target )->debuffs_exploiter->trigger();
    }
    if ( p()->conduit.ashen_juggernaut.ok() && !p()->talents.arms.juggernaut->ok() )
    {
      p()->buff.ashen_juggernaut_conduit->trigger();
    }
    if ( p()->talents.arms.juggernaut.ok() )
    {
      p()->buff.juggernaut->trigger();
    }
    if ( p()->talents.protection.juggernaut.ok() )
    {
      p()->buff.juggernaut_prot->trigger();
    }
    if ( p()->legendary.sinful_surge->ok() && td( execute_state->target )->debuffs_colossus_smash->check() )
    {
      td( execute_state->target )->debuffs_colossus_smash->extend_duration( p(), timespan_t::from_millis( p()->legendary.sinful_surge->effectN( 1 ).base_value() ) );
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Ayala's Stone Heart and Sudden Death allow execution on any target
    bool always = p()->buff.ayalas_stone_heart->check() || p()->buff.sudden_death->check();

if ( ! always && candidate_target->health_percentage() > execute_pct_below && candidate_target->health_percentage() < execute_pct_above )
    {
      return false;
    }

    return warrior_attack_t::target_ready( candidate_target );
  }

  bool ready() override
  {
    if ( !p()->covenant.condemn_driver->ok() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Fury Condemn ======================================================================

struct condemn_main_hand_t : public warrior_attack_t
{
  int aoe_targets;
  condemn_main_hand_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    background = true;
    dual   = true;
    weapon = &( p->main_hand_weapon );
    //base_multiplier *= 1.0 + p->spec.execute_rank_2->effectN( 1 ).percent();
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->conduit.harrowing_punishment->ok() )
    {
      size_t num_targets = std::min( p()->sim->target_non_sleeping_list.size(), (size_t) 5);
      am *= 1.0 + p()->conduit.harrowing_punishment.percent() * num_targets;
    }

    return am;
  }
};

struct condemn_off_hand_t : public warrior_attack_t
{
  int aoe_targets;
  condemn_off_hand_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    background = true;
    dual     = true;
    may_miss = may_dodge = may_parry = may_block = false;
    weapon                                       = &( p->off_hand_weapon );
    //base_multiplier *= 1.0 + p->spec.execute_rank_2->effectN( 1 ).percent();
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->conduit.harrowing_punishment->ok() )
    {
      size_t num_targets = std::min( p()->sim->target_non_sleeping_list.size(), (size_t) 5);
      am *= 1.0 + p()->conduit.harrowing_punishment.percent() * num_targets;
    }

    return am;
  }
};

struct fury_condemn_parent_t : public warrior_attack_t
{
  condemn_main_hand_t* mh_attack;
  condemn_off_hand_t* oh_attack;
  bool improved_execute;
  double execute_pct_above;
  double execute_pct_below;
  //double cost_rage;
  double max_rage;
  double rage_from_improved_execute;
  fury_condemn_parent_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "condemn", p, p->covenant.condemn ),
      improved_execute( false ), execute_pct_above( 80 ), execute_pct_below( 20 ), max_rage( 40 ),
      rage_from_improved_execute(
      ( p->talents.fury.improved_execute->effectN( 3 ).base_value() ) / 10.0 )
  {
    parse_options( options_str );
    mh_attack = new condemn_main_hand_t( p, "condemn_mainhand", p->covenant.condemn->effectN( 1 ).trigger() );
    oh_attack = new condemn_off_hand_t( p, "condemn_offhand", p->covenant.condemn->effectN( 2 ).trigger() );
    add_child( mh_attack );
    add_child( oh_attack );

    if ( p->talents.fury.massacre->ok() )
    {
      execute_pct_below = p->talents.fury.massacre->effectN( 2 )._base_value;
      if ( cooldown->action == this )
      {
        cooldown->duration -= timespan_t::from_millis( p->talents.fury.massacre->effectN( 3 ).base_value() );
      }
    }
  }

  double cost() const override
  {
    double c = warrior_attack_t::cost();
    c = std::min( max_rage, std::max( p()->resources.current[RESOURCE_RAGE], c ) );

    if ( p()->talents.fury.improved_execute->ok() )
    {
      c *= 1.0 + p()->talents.fury.improved_execute->effectN( 1 ).percent();
    }
    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    mh_attack->execute();

    if ( p()->specialization() == WARRIOR_FURY && result_is_hit( execute_state->result ) &&
         p()->off_hand_weapon.type != WEAPON_NONE )
      // If MH fails to land, or if there is no OH weapon for Fury, oh attack does not execute.
      oh_attack->execute();

    p()->buff.meat_cleaver->decrement();
    p()->buff.sudden_death->expire();
    p()->buff.ayalas_stone_heart->expire();

    if ( p()->talents.fury.improved_execute->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_improved_execute, p()->gain.execute );
    }

    if ( p()->conduit.ashen_juggernaut.ok() && !p()->talents.fury.ashen_juggernaut->ok() ) // talent overrides conduit )
    {
      p()->buff.ashen_juggernaut_conduit->trigger();
    }
    if ( p()->talents.fury.ashen_juggernaut->ok() )
    {
      p()->buff.ashen_juggernaut->trigger();
    }

    if ( p()->legendary.sinful_surge->ok() && p()->buff.recklessness->check() )
    {
      p()->buff.recklessness->extend_duration( p(), timespan_t::from_millis( p()->legendary.sinful_surge->effectN( 2 ).base_value() ) );
      p()->buff.reckless_abandon->extend_duration( p(), timespan_t::from_millis( p()->legendary.sinful_surge->effectN( 2 ).base_value() ) );
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Ayala's Stone Heart and Sudden Death allow execution on any target
    bool always = p()->buff.ayalas_stone_heart->check() || p()->buff.sudden_death->check();

    if ( ! always && candidate_target->health_percentage() > execute_pct_below && candidate_target->health_percentage() < execute_pct_above )
    {
      return false;
    }

    return warrior_attack_t::target_ready( candidate_target );
  }

  bool ready() override
  {
    if ( !p()->covenant.condemn_driver->ok() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Conquerors Banner=========================================================

struct conquerors_banner_t : public warrior_spell_t
{
  conquerors_banner_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "conquerors_banner", p, p->covenant.conquerors_banner )
  {
    parse_options( options_str );
    energize_resource       = RESOURCE_NONE;
    harmful = false;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.conquerors_banner->trigger();
    p()->buff.conquerors_mastery->trigger();

    if ( p()->conduit.veterans_repute->ok() )
    {
      p()->buff.veterans_repute->trigger();
    }
  }
};

// Spear of Bastion==========================================================

struct spear_of_bastion_attack_t : public warrior_attack_t
{
  double rage_gain;
  spear_of_bastion_attack_t( warrior_t* p ) : warrior_attack_t( "spear_of_bastion_attack", p, p->find_spell( 376080 ) ),
  rage_gain( p->find_spell( 376080 )->effectN( 3 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
    aoe                        = -1;
    reduced_aoe_targets        = 5.0;
    dual                       = true;
    allow_class_ability_procs  = true;
    energize_type     = action_energize::NONE;

    rage_gain *= 1.0 + p->talents.warrior.piercing_verdict->effectN( 2 ).percent();

    // dot_duration += timespan_t::from_millis( p -> find_spell( 357996 ) -> effectN( 1 ).base_value() );
    if ( p->talents.warrior.elysian_might->ok() )
    {
      dot_duration += timespan_t::from_millis( p->find_spell( 386284 )->effectN( 1 ).base_value() );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.spear_of_bastion );
  }
};

struct spear_of_bastion_t : public warrior_attack_t
{
  spear_of_bastion_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "spear_of_bastion", p, p->talents.warrior.spear_of_bastion )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
    aoe = -1;
    reduced_aoe_targets               = 5.0;
    if ( p->active.spear_of_bastion_attack )
    {
      execute_action = p->active.spear_of_bastion_attack;
      add_child( execute_action );
    }

    energize_type     = action_energize::NONE;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.warrior.elysian_might->ok() )
    {
      p()->buff.elysian_might->trigger();
    }
  }
};

// Kyrian Spear=================================================================

struct kyrian_spear_attack_t : public warrior_attack_t
{
  kyrian_spear_attack_t( warrior_t* p ) : warrior_attack_t( "kyrian_spear_attack", p, p->find_spell( 307871 ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
    aoe = -1;
    reduced_aoe_targets = 5.0;
    dual       = true;
//dot_duration += timespan_t::from_millis( p -> find_spell( 357996 ) -> effectN( 1 ).base_value() );
    if ( p->legendary.elysian_might->ok() )
    {
      dot_duration += timespan_t::from_millis( p -> find_spell( 357996 ) -> effectN( 1 ).base_value() );
    }
  }
};

struct kyrian_spear_t : public warrior_attack_t
{
  kyrian_spear_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "kyrian_spear", p, p->covenant.kyrian_spear )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
    if ( p->active.kyrian_spear_attack )
    {
      execute_action = p->active.kyrian_spear_attack;
      add_child( execute_action );
    }
    if ( p->conduit.piercing_verdict->ok() )
    {
      energize_amount = p->conduit.piercing_verdict.percent() * (1 + p->find_spell( 307871 )->effectN( 3 ).base_value() / 10.0 );
    }
    energize_type     = action_energize::ON_CAST;
    energize_resource = RESOURCE_RAGE;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->legendary.elysian_might->ok() )
      {
        p()->buff.elysian_might_legendary->trigger();
      }
  }
};


// ==========================================================================
// Warrior Spells
// ==========================================================================

// Avatar ===================================================================

struct avatar_t : public warrior_spell_t
{
  double signet_chance;
  bool signet_triggered;
  avatar_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell,
  bool signet_triggered = false )
    : warrior_spell_t( n, p, spell ),
    signet_chance( 0.5 * p->legendary.signet_of_tormented_kings->proc_chance() ),
    signet_triggered( signet_triggered )
  {

    parse_options( options_str );
    callbacks = false;
    harmful   = false;

    // Vision of Perfection doesn't reduce the cooldown for non-prot
    if ( p -> azerite.vision_of_perfection.enabled() && p -> specialization() == WARRIOR_PROTECTION )
    {
      cooldown -> duration *= 1.0 + azerite::vision_of_perfection_cdr( p -> azerite.vision_of_perfection );
    }
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p()->talents.warrior.immovable_object->ok() )
      p()->buff.shield_wall->trigger( p()->talents.warrior.immovable_object->effectN( 2 ).time_value() );

    if( signet_triggered )
    {
      if ( p()->buff.avatar->check() )
      {
        p()->buff.avatar->extend_duration( p(), timespan_t::from_millis( p()->legendary.signet_of_tormented_kings->effectN( 2 ).base_value() ) );
      }
      else
      {
        p()->buff.avatar->trigger( p()->legendary.signet_of_tormented_kings->effectN( 2 ).time_value() );
      }
    }
    else
    {
      if ( ! p()->bugs )
        p()->buff.avatar->extend_duration_or_trigger();
      else  // avatar always triggers to 20s duration when it's hard cast
      {
        auto extended_duration = p()->buff.avatar->buff_duration();
        if ( p()->buff.avatar->remains() + extended_duration > 20_s )
        {
          extended_duration = 20_s - p()->buff.avatar->remains();
        }
        p()->buff.avatar->extend_duration_or_trigger( extended_duration );
      }

      if ( p()->legendary.signet_of_tormented_kings.ok() && p()->specialization() == WARRIOR_FURY )
      {
        action_t* signet_ability = p()->rng().roll( signet_chance ) ? p()->active.signet_recklessness : p()->active.signet_bladestorm_f;
        signet_ability->schedule_execute();
      }
      else if ( p()->legendary.signet_of_tormented_kings.ok() && p()->specialization() == WARRIOR_ARMS )
      {
        action_t* signet_ability = p()->rng().roll( signet_chance ) ? p()->active.signet_recklessness : p()->active.signet_bladestorm_a;
        signet_ability->schedule_execute();
      }
      if ( p()->talents.warrior.berserkers_torment.ok() )
      {
        action_t* torment_ability = p()->active.torment_recklessness;
        torment_ability->schedule_execute();
      }
      if ( p()->talents.warrior.blademasters_torment.ok() )
      {
        action_t* torment_ability = p()->active.torment_bladestorm;
        torment_ability->schedule_execute();
      }
      if ( p()->talents.warrior.titans_torment->ok() )
      {
        action_t* torment_ability = p()->active.torment_odyns_fury;
        torment_ability->schedule_execute();
      }
    }

    if ( p()->talents.warrior.warlords_torment->ok() )
    {
      const timespan_t trigger_duration = p()->talents.warrior.warlords_torment->effectN( 2 ).time_value();
      p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
    }

    if ( p()->azerite.bastion_of_might.enabled() )
    {
      p()->buff.bastion_of_might->trigger();

      if ( p() -> specialization() == WARRIOR_PROTECTION )
        p() -> active.bastion_of_might_ip -> execute();
    }
  }

  bool verify_actor_spec() const override // no longer needed ?
  {
    if ( signet_triggered )
      return true;

    // Do not check spec if Arms talent avatar is available, so that spec check on the spell (required: protection) does not fail.
    if ( p()->talents.warrior.avatar->ok() && p()->specialization() == WARRIOR_ARMS )
      return true;

    return warrior_spell_t::verify_actor_spec();
  }
};

// Torment Avatar ===================================================================

struct torment_avatar_t : public warrior_spell_t
{
  torment_avatar_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell,
  bool torment_triggered = false ) : warrior_spell_t( n, p, spell )
  {
    parse_options( options_str );
    callbacks = false;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p()->talents.warrior.berserkers_torment->ok() )
    {
      const timespan_t trigger_duration = p()->talents.warrior.berserkers_torment->effectN( 2 ).time_value();
      p()->buff.avatar->extend_duration_or_trigger( trigger_duration );
    }
    if ( p()->talents.warrior.titans_torment->ok() )
    {
      const timespan_t trigger_duration = timespan_t::from_millis( 4000 ); // value not in spell data
      p()->buff.avatar->extend_duration_or_trigger( trigger_duration );   
    }
    if ( p()->talents.warrior.blademasters_torment->ok() )
    {
      if ( p()->talents.arms.spiteful_serenity->ok() )
      {
        const timespan_t trigger_duration = p()->talents.warrior.blademasters_torment->effectN( 2 ).time_value()
                                            + timespan_t::from_millis( 4000 ); // Spiteful doubles duration from 4->8s, not in data
        p()->buff.avatar->extend_duration_or_trigger( trigger_duration );
      }
      else
      {
        const timespan_t trigger_duration = p()->talents.warrior.blademasters_torment->effectN( 2 ).time_value();
        p()->buff.avatar->extend_duration_or_trigger( trigger_duration );
      }
    }
  }
};

// Battle Shout ===================================================================

struct battle_shout_t : public warrior_spell_t
{
  battle_shout_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "battle_shout", p, p->spell.battle_shout )
  {
    parse_options( options_str );
    usable_while_channeling = true;
    harmful = false;

    background = sim->overrides.battle_shout != 0;
  }

  void execute() override
  {
    sim->auras.battle_shout->trigger();
  }

  bool ready() override
  {
    return warrior_spell_t::ready() && !sim->auras.battle_shout->check();
  }
};

// Berserker Rage ===========================================================

struct berserker_rage_t : public warrior_spell_t
{
  berserker_rage_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "berserker_rage", p, p->talents.warrior.berserker_rage )
  {
    parse_options( options_str );
    callbacks   = false;
    use_off_gcd = true;
    range       = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.berserker_rage->trigger();
  }
};

// Battle Stance ===============================================================

struct battle_stance_t : public warrior_spell_t
{
  std::string onoff;
  bool onoffbool;
  battle_stance_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "battle_stance", p, p->talents.warrior.battle_stance ), onoff(), onoffbool( false )
  {
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );
    harmful = false;

    if ( onoff == "on" )
    {
      onoffbool = true;
    }
    else if ( onoff == "off" )
    {
      onoffbool = false;
    }
    else
    {
      sim->errorf( "Battle stance must use the option 'toggle=on' or 'toggle=off'" );
      background = true;
    }

    use_off_gcd = true;

    //background = sim->overrides.battle_stance != 0;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    if ( onoffbool )
    {
      p()->buff.battle_stance->trigger();
    }
    else
    {
      p()->buff.battle_stance->expire();
    }
  }

  bool ready() override
  {
    if ( onoffbool && p()->buff.battle_stance->check() )
    {
      return false;
    }
    else if ( !onoffbool && !p()->buff.battle_stance->check() )
    {
      return false;
    }

    return warrior_spell_t::ready(); // && !sim->auras.battle_stance->check();
  }
};

// Berserker Stance ===============================================================

struct berserker_stance_t : public warrior_spell_t
{
  std::string onoff;
  bool onoffbool;
  berserker_stance_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "berserker_stance", p, p->talents.warrior.berserker_stance ), onoff(), onoffbool( false )
  {
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );
    harmful = false;

    if ( onoff == "on" )
    {
      onoffbool = true;
    }
    else if ( onoff == "off" )
    {
      onoffbool = false;
    }
    else
    {
      sim->errorf( "Berserker stance must use the option 'toggle=on' or 'toggle=off'" );
      background = true;
    }

    use_off_gcd = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    if ( onoffbool )
    {
      p()->buff.berserker_stance->trigger();
    }
    else
    {
      p()->buff.berserker_stance->expire();
    }
  }

  bool ready() override
  {
    if ( onoffbool && p()->buff.berserker_stance->check() )
    {
      return false;
    }
    else if ( !onoffbool && !p()->buff.berserker_stance->check() )
    {
      return false;
    }

    return warrior_spell_t::ready();
  }
};

// Defensive Stance ===============================================================

struct defensive_stance_t : public warrior_spell_t
{
  std::string onoff;
  bool onoffbool;
  defensive_stance_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "defensive_stance", p, p->talents.warrior.defensive_stance ), onoff(), onoffbool( false )
  {
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );

    if ( onoff == "on" )
    {
      onoffbool = true;
    }
    else if ( onoff == "off" )
    {
      onoffbool = false;
    }
    else
    {
      sim->errorf( "Defensive stance must use the option 'toggle=on' or 'toggle=off'" );
      background = true;
    }

    use_off_gcd = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    if ( onoffbool )
    {
      p()->buff.defensive_stance->trigger();
    }
    else
    {
      p()->buff.defensive_stance->expire();
    }
  }

  bool ready() override
  {
    if ( onoffbool && p()->buff.defensive_stance->check() )
    {
      return false;
    }
    else if ( !onoffbool && !p()->buff.defensive_stance->check() )
    {
      return false;
    }

    return warrior_spell_t::ready();
  }
};

// Die By the Sword  ==============================================================

struct die_by_the_sword_t : public warrior_spell_t
{
  die_by_the_sword_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "die_by_the_sword", p, p->talents.arms.die_by_the_sword )
  {
    parse_options( options_str );
    callbacks = false;
    range     = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p()->buff.die_by_the_sword->trigger();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }

    return warrior_spell_t::ready();
  }
};

// In for the Kill ===================================================================

struct in_for_the_kill_t : public buff_t
{
  double base_value;
  double below_pct_increase_amount;
  double below_pct_increase;

  in_for_the_kill_t( warrior_t& p, util::string_view n, const spell_data_t* s )
    : buff_t( &p, n, s ),
      base_value( p.talents.arms.in_for_the_kill->effectN( 1 ).percent() ),
      below_pct_increase_amount( p.talents.arms.in_for_the_kill->effectN( 2 ).percent() ),
      below_pct_increase( p.talents.arms.in_for_the_kill->effectN( 3 ).base_value() )

  {
    add_invalidate( CACHE_ATTACK_HASTE );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( source->target->health_percentage() <= below_pct_increase )
    {
      default_value = below_pct_increase_amount;
    }
    else
    {
      default_value = base_value;
    }
    return buff_t::trigger( stacks, value, chance, duration );
  }
};

// Last Stand ===============================================================

struct last_stand_t : public warrior_spell_t
{
  last_stand_t( warrior_t* p, util::string_view options_str ) : warrior_spell_t( "last_stand", p, p->talents.protection.last_stand )
  {
    parse_options( options_str );
    range              = -1;
    cooldown->duration = data().cooldown();
    cooldown -> duration += p -> talents.protection.bolster -> effectN( 1 ).time_value();
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p()->talents.protection.unnerving_focus->ok() )
    {
      p()->buff.unnerving_focus->trigger();
    }

    if ( p() -> talents.protection.bolster -> ok() )
    {
      if ( p()->buff.shield_block->check() )
      {
        p()->buff.shield_block->extend_duration( p(), data().duration() );
      }
      else
      {
        p()->buff.shield_block->trigger( data().duration() ) ;
      }
    }
    p()->buff.last_stand->trigger();
  }
};

// Rallying Cry ===============================================================

struct rallying_cry_t : public warrior_spell_t
{
  rallying_cry_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "rallying_cry", p, p->talents.warrior.rallying_cry )
  {
    parse_options( options_str );
    callbacks = false;
    range     = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    player->buffs.rallying_cry->trigger();
  }
};

// Recklessness =============================================================

struct recklessness_t : public warrior_spell_t
{
  double bonus_crit;
  double signet_chance;
  bool signet_triggered;
  recklessness_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell,
  bool signet_triggered = false )
    : warrior_spell_t( n, p, spell ),
      bonus_crit( 0.0 ),
      signet_chance( 0.5 * p->legendary.signet_of_tormented_kings->proc_chance() ),
      signet_triggered( signet_triggered )
  {
    parse_options( options_str );
    bonus_crit = data().effectN( 1 ).percent();
    callbacks  = false;

    harmful = false;

    if ( p->talents.fury.reckless_abandon->ok() )
    {
      energize_amount   = p->talents.fury.reckless_abandon->effectN( 1 ).base_value() / 10.0;
      energize_type     = action_energize::ON_CAST;
      energize_resource = RESOURCE_RAGE;
    }

    if ( p->azerite.vision_of_perfection.enabled() )
    {
      cooldown->duration *= 1.0 + azerite::vision_of_perfection_cdr( p->azerite.vision_of_perfection );
    }
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if( signet_triggered )
    {
      if ( p()->buff.recklessness->check() )
      {
        p()->buff.recklessness->extend_duration( p(), timespan_t::from_millis( p()->legendary.signet_of_tormented_kings->effectN( 1 ).base_value() ) );
      }
      else
      {
        p()->buff.recklessness->trigger( p()->legendary.signet_of_tormented_kings->effectN( 1 ).time_value() );
      }
    }
    else
    {
      p()->buff.recklessness->extend_duration_or_trigger();

      if ( p()->legendary.signet_of_tormented_kings.ok() )
      {
        action_t* signet_ability = p()->rng().roll( signet_chance ) ? p()->active.signet_avatar : p()->active.signet_bladestorm_f;
        signet_ability->schedule_execute();
      }
      if ( p()->talents.warrior.berserkers_torment.ok() )
      {
        action_t* torment_ability = p()->active.torment_avatar;
        torment_ability->schedule_execute();
      }
    }
  }

  bool verify_actor_spec() const override
  {
    if ( signet_triggered )
      return true;

    if ( p()->talents.warrior.warlords_torment->ok() )
      return true;

    return warrior_spell_t::verify_actor_spec();
  }
};

// Torment Recklessness =============================================================

struct torment_recklessness_t : public warrior_spell_t
{
  double bonus_crit;
  torment_recklessness_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell,
    bool torment_triggered = false ) : warrior_spell_t( n, p, spell ), bonus_crit( 0.0 )
  {
    parse_options( options_str );
    bonus_crit = data().effectN( 1 ).percent();
    callbacks  = false;

    harmful = false;

  }

  void execute() override
  {
    warrior_spell_t::execute();

    const timespan_t trigger_duration = p()->talents.warrior.berserkers_torment->effectN( 2 ).time_value();
    p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
  }
};

// Ignore Pain =============================================================

struct ignore_pain_buff_t : public absorb_buff_t
{
  ignore_pain_buff_t( warrior_t* player ) : absorb_buff_t( player, "ignore_pain", player->talents.protection.ignore_pain )
  {
    cooldown->duration = 0_ms;
    set_absorb_source( player->get_stats( "ignore_pain" ) );
    set_absorb_gain( player->get_gain( "ignore_pain" ) );
  }

  // Custom consume implementation to allow minimum absorb amount.
  double consume( double amount ) override
  {
    // IP only absorbs up to 55% of the damage taken
    amount *= debug_cast< warrior_t* >( player ) -> talents.protection.ignore_pain -> effectN( 2 ).percent();
    double absorbed = absorb_buff_t::consume( amount );

    return absorbed;
  }
};

struct ignore_pain_t : public warrior_spell_t
{
  ignore_pain_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "ignore_pain", p, p->talents.protection.ignore_pain )
  {
    parse_options( options_str );
    may_crit     = false;
    range        = -1;
    target       = player;
    base_costs[ RESOURCE_RAGE ] = ( p->specialization() == WARRIOR_FURY ? 60 : p->specialization() == WARRIOR_ARMS ? 40 : 35);

    base_dd_max = base_dd_min = 0;
    resource_current = RESOURCE_RAGE;
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double da = warrior_spell_t::bonus_da( state );

    if ( p() -> azerite.bloodsport.enabled() )
    {
      da += p() -> azerite.bloodsport.value( 3 );
    }

    return da;
  }

  double cost() const override
  {
    double c = warrior_spell_t::cost();

    //c *= 1.0 + p() -> buff.vengeance_ignore_pain -> value();

    return c;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    //p()->buff.vengeance_ignore_pain->expire();
    //p()->buff.vengeance_revenge->trigger();
    if ( p() -> azerite.bloodsport.enabled() )
    {
      p() -> buff.bloodsport -> trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    double new_ip = s -> result_amount;

    double previous_ip = p() -> buff.ignore_pain -> current_value;

    // IP is capped to 30% of max health
    double ip_max_health_cap = p() -> max_health() * 0.3;

    if ( previous_ip + new_ip > ip_max_health_cap )
    {
      new_ip = ip_max_health_cap;
    }

    if ( new_ip > 0.0 )
    {
      p()->buff.ignore_pain->trigger( 1, new_ip );
    }
  }
};

// A free Ignore Pain from Bastion of Might when Avatar is cast
struct ignore_pain_bom_t : public ignore_pain_t
{
  ignore_pain_bom_t( warrior_t* p )
    : ignore_pain_t( p, "" )
  {
    may_crit     = false;
    range        = -1;
    target       = player;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    //p()->buff.vengeance_revenge->trigger(); // this IP does trigger vengeance but doesnt consume it
  }

  double cost( ) const override
  {
    return 0.0;
  }

};

// Shield Block =============================================================

struct shield_block_t : public warrior_spell_t
{
  shield_block_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "shield_block", p, p->spell.shield_block )
  {
    parse_options( options_str );
    cooldown->hasted = true;
    cooldown->charges += as<int>( p->spec.shield_block_2->effectN( 1 ).base_value() );
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p()->buff.shield_block->check() )
    {
      p()->buff.shield_block->extend_duration( p(), p() -> buff.shield_block->buff_duration() );
    }
    else
    {
      p()->buff.shield_block->trigger();
    }
  }

  bool ready() override
  {
    if ( !p()->has_shield_equipped() )
    {
      return false;
    }

    return warrior_spell_t::ready();
  }
};

// Shield Wall ==============================================================

struct shield_wall_t : public warrior_spell_t
{
  shield_wall_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "shield_wall", p, p->talents.protection.shield_wall )
  {
    parse_options( options_str );
    harmful = false;
    range   = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.shield_wall->trigger( 1, p()->buff.shield_wall->data().effectN( 1 ).percent() );

    if ( p()->talents.warrior.immovable_object->ok() )
      p()->buff.avatar->trigger( p()->talents.warrior.immovable_object->effectN( 2 ).time_value() );
  }
};

// Spell Reflection  ==============================================================

struct spell_reflection_t : public warrior_spell_t
{
  spell_reflection_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "spell_reflection", p, p->talents.warrior.spell_reflection )
  {
    parse_options( options_str );
    use_off_gcd = usable_while_channeling = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p()->buff.spell_reflection->trigger();
  }
};

// Taunt =======================================================================

struct taunt_t : public warrior_spell_t
{
  taunt_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "taunt", p, p->find_class_spell( "Taunt" ) )
  {
    parse_options( options_str );
    use_off_gcd = ignore_false_positive = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
    {
      target->taunt( player );
    }

    warrior_spell_t::impact( s );
  }
};

}  // UNNAMED NAMESPACE

// warrior_t::create_action  ================================================

action_t* warrior_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "ancient_aftershock" )
    return new ancient_aftershock_t( this, options_str );
  if ( name == "avatar" )
    return new avatar_t( this, options_str, name, talents.warrior.avatar );
  if ( name == "battle_shout" )
    return new battle_shout_t( this, options_str );
  if ( name == "recklessness" )
    return new recklessness_t( this, options_str, name, specialization() == WARRIOR_FURY ? talents.fury.recklessness : find_spell( 1719 ) );
  if ( name == "battle_stance" )
    return new battle_stance_t( this, options_str );
  if ( name == "berserker_rage" )
    return new berserker_rage_t( this, options_str );
  if ( name == "berserker_stance" )
    return new berserker_stance_t( this, options_str );
  if ( name == "bladestorm" )
    return new bladestorm_t( this, options_str, name, specialization() == WARRIOR_FURY ? find_spell( 46924 ) : talents.arms.bladestorm );
  if ( name == "bloodthirst" )
    return new bloodthirst_t( this, options_str );
  if ( name == "bloodbath" )
    return new bloodbath_t( this, options_str );
  if ( name == "charge" )
    return new charge_t( this, options_str );
  if ( name == "cleave" )
    return new cleave_t( this, options_str );
  if ( name == "colossus_smash" )
    return new colossus_smash_t( this, options_str );
  if ( name == "condemn" )
  {
    if ( specialization() == WARRIOR_FURY )
    {
     return new fury_condemn_parent_t( this, options_str );
    }
    else
    {
     return new condemn_arms_t( this, options_str );
    }
  }
  if ( name == "conquerors_banner" )
    return new conquerors_banner_t( this, options_str );
  if ( name == "defensive_stance" )
    return new defensive_stance_t( this, options_str );
  if ( name == "demoralizing_shout" )
    return new demoralizing_shout_t( this, options_str );
  if ( name == "devastate" )
    return new devastate_t( this, options_str );
  if ( name == "die_by_the_sword" )
    return new die_by_the_sword_t( this, options_str );
  if ( name == "thunderous_roar" )
    return new thunderous_roar_t( this, options_str );
  if ( name == "enraged_regeneration" )
    return new enraged_regeneration_t( this, options_str );
  if ( name == "execute" )
  {
    if ( specialization() == WARRIOR_FURY )
    {
      return new fury_execute_parent_t( this, options_str );
    }
    else
    {
      return new execute_arms_t( this, options_str );
    }
  }
  if ( name == "hamstring" )
    return new hamstring_t( this, options_str );
  if ( name == "heroic_charge" )
    return new heroic_charge_t( this, options_str );
  if ( name == "heroic_leap" )
    return new heroic_leap_t( this, options_str );
  if ( name == "heroic_throw" )
    return new heroic_throw_t( this, options_str );
  if ( name == "impending_victory" )
    return new impending_victory_t( this, options_str );
  if ( name == "intervene" )
    return new intervene_t( this, options_str );
  if ( name == "last_stand" )
    return new last_stand_t( this, options_str );
  if ( name == "mortal_strike" )
    return new mortal_strike_t( this, options_str );
  if ( name == "odyns_fury" )
    return new odyns_fury_t( this, options_str, name, talents.fury.odyns_fury );
  if ( name == "onslaught" )
    return new onslaught_t( this, options_str );
  if ( name == "overpower" )
    return new overpower_t( this, options_str );
  if ( name == "piercing_howl" )
    return new piercing_howl_t( this, options_str );
  if ( name == "pummel" )
    return new pummel_t( this, options_str );
  if ( name == "raging_blow" )
    return new raging_blow_t( this, options_str );
  if ( name == "crushing_blow" )
    return new crushing_blow_t( this, options_str );
  if ( name == "rallying_cry" )
    return new rallying_cry_t( this, options_str );
  if ( name == "rampage" )
    return new rampage_parent_t( this, options_str );
  if ( name == "ravager" )
    return new ravager_t( this, options_str );
  if ( name == "rend" )
  {
    if ( specialization() == WARRIOR_PROTECTION )
      return new rend_prot_t( this, options_str );
    else
      return new rend_t( this, options_str );
  }
  if ( name == "revenge" )
    return new revenge_t( this, options_str );
  if ( name == "shattering_throw" )
    return new shattering_throw_t( this, options_str );
  if ( name == "shield_block" )
    return new shield_block_t( this, options_str );
  if ( name == "shield_charge" )
    return new shield_charge_t( this, options_str );
  if ( name == "shield_slam" )
    return new shield_slam_t( this, options_str );
  if ( name == "shield_wall" )
    return new shield_wall_t( this, options_str );
  if ( name == "shockwave" )
    return new shockwave_t( this, options_str );
  if ( name == "skullsplitter" )
    return new skullsplitter_t( this, options_str );
  if ( name == "slam" )
    return new slam_t( this, options_str );
  if ( name == "spear_of_bastion" )
    return new spear_of_bastion_t( this, options_str );
  if ( name == "kyrian_spear" )
    return new kyrian_spear_t( this, options_str );
  if ( name == "spell_reflection" )
    return new spell_reflection_t( this, options_str );
  if ( name == "storm_bolt" )
    return new storm_bolt_t( this, options_str );
  if ( name == "sweeping_strikes" )
    return new sweeping_strikes_t( this, options_str );
  if ( name == "taunt" )
    return new taunt_t( this, options_str );
  if ( name == "thunder_clap" )
    return new thunder_clap_t( this, options_str );
  if ( name == "titanic_throw" )
    return new titanic_throw_t( this, options_str );
  if ( name == "victory_rush" )
    return new victory_rush_t( this, options_str );
  if ( name == "warbreaker" )
    return new warbreaker_t( this, options_str );
  if ( name == "ignore_pain" )
    return new ignore_pain_t( this, options_str );
  if ( name == "whirlwind" )
  {
    if ( specialization() == WARRIOR_FURY )
    {
      return new fury_whirlwind_parent_t( this, options_str );
    }
    else if ( specialization() == WARRIOR_ARMS )
    {
      return new arms_whirlwind_parent_t( this, options_str );
    }
  }
  if ( name == "wrecking_throw" )
    return new wrecking_throw_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  player_t::init_spells();

  // Core Class Spells
  spell.battle_shout            = find_class_spell( "Battle Shout" );
  spell.charge                  = find_class_spell( "Charge" );
  spell.execute                 = find_class_spell( "Execute" );
  spell.hamstring               = find_class_spell( "Hamstring" );
  spell.heroic_throw            = find_class_spell( "Heroic Throw" );
  spell.pummel                  = find_class_spell( "Pummel" );
  spell.shield_block            = find_class_spell( "Shield Block" );
  spell.shield_slam             = find_class_spell( "Shield Slam" );
  spell.slam                    = find_class_spell( "Slam" );
  spell.taunt                   = find_class_spell( "Taunt" );
  spell.victory_rush            = find_class_spell( "Victory Rush" );
  spell.whirlwind               = find_class_spell( "Whirlwind" );
  spell.shield_block_buff       = find_spell( 132404 );
  spell.concussive_blows_debuff = find_spell( 383116 ); 
  spell.recklessness_buff       = find_spell( 1719 ); // lookup to allow Warlord to use Reck

  // Class Passives
  spell.warrior_aura            = find_spell( 137047 );

  // Arms Spells
  mastery.deep_wounds_ARMS      = find_mastery_spell( WARRIOR_ARMS );
  spec.arms_warrior             = find_specialization_spell( "Arms Warrior" );
  spec.seasoned_soldier         = find_specialization_spell( "Seasoned Soldier" );
  spec.sweeping_strikes         = find_specialization_spell( "Sweeping Strikes" );
  spec.deep_wounds_ARMS         = find_specialization_spell("Mastery: Deep Wounds", WARRIOR_ARMS);
  spell.colossus_smash_debuff   = find_spell( 208086 );
  spell.executioners_precision_debuff = find_spell( 386633 );
  spell.fatal_mark_debuff       = find_spell( 383704 );

  // Fury Spells
  mastery.unshackled_fury       = find_mastery_spell( WARRIOR_FURY );
  spec.fury_warrior             = find_specialization_spell( "Fury Warrior" );
  spec.enrage                   = find_specialization_spell( "Enrage" );
  spec.execute                  = find_specialization_spell( "Execute" );
  spec.whirlwind                = find_specialization_spell( "Whirlwind" );
  spec.bloodbath                = find_spell(335096);
  spec.crushing_blow            = find_spell(335097);
  spell.whirlwind_buff          = find_spell( 85739, WARRIOR_FURY );  // Used to be called Meat Cleaver
  spell.siegebreaker_debuff     = find_spell( 280773 );

  // Protection Spells
  mastery.critical_block        = find_mastery_spell( WARRIOR_PROTECTION );
  spec.protection_warrior       = find_specialization_spell( "Protection Warrior" );
  spec.devastate                = find_specialization_spell( "Devastate" );
  spec.riposte                  = find_specialization_spell( "Riposte" );
  spec.thunder_clap_prot_hidden = find_specialization_spell( "Thunder Clap Prot Hidden" );
  spec.vanguard                 = find_specialization_spell( "Vanguard" );
  spec.deep_wounds_PROT         = find_specialization_spell("Deep Wounds", WARRIOR_PROTECTION);
  spec.revenge_trigger          = find_specialization_spell("Revenge Trigger");
  spec.shield_block_2           = find_specialization_spell( 231847 ); // extra charge
  spell.shield_wall             = find_spell( 871 );

  // Class Talents
  talents.warrior.battle_stance                    = find_talent_spell( talent_tree::CLASS, "Battle Stance" );
  talents.warrior.berserker_stance                 = find_talent_spell( talent_tree::CLASS, "Berserker Stance" );
  talents.warrior.defensive_stance                 = find_talent_spell( talent_tree::CLASS, "Defensive Stance" );

  talents.warrior.berserker_rage                   = find_talent_spell( talent_tree::CLASS, "Berserker Rage" );
  talents.warrior.impending_victory                = find_talent_spell( talent_tree::CLASS, "Impending Victory" );
  talents.warrior.war_machine                      = find_talent_spell( talent_tree::CLASS, "War Machine", specialization() );
  talents.warrior.intervene                        = find_talent_spell( talent_tree::CLASS, "Intervene" );
  talents.warrior.rallying_cry                     = find_talent_spell( talent_tree::CLASS, "Rallying Cry" );

  talents.warrior.piercing_howl                    = find_talent_spell( talent_tree::CLASS, "Piercing Howl" );
  talents.warrior.fast_footwork                    = find_talent_spell( talent_tree::CLASS, "Fast Footwork" );
  talents.warrior.spell_reflection                 = find_talent_spell( talent_tree::CLASS, "Spell Reflection" );
  talents.warrior.leeching_strikes                 = find_talent_spell( talent_tree::CLASS, "Leeching Strikes" );
  talents.warrior.inspiring_presence               = find_talent_spell( talent_tree::CLASS, "Inspiring Presence" );
  talents.warrior.second_wind                      = find_talent_spell( talent_tree::CLASS, "Second Wind" );

  talents.warrior.frothing_berserker               = find_talent_spell( talent_tree::CLASS, "Frothing Berserker", specialization() );
  talents.warrior.heroic_leap                      = find_talent_spell( talent_tree::CLASS, "Heroic Leap" );
  talents.warrior.intimidating_shout               = find_talent_spell( talent_tree::CLASS, "Intimidating Shout" );
  talents.warrior.thunder_clap                     = find_talent_spell( talent_tree::CLASS, "Thunder Clap", specialization() );
  talents.warrior.furious_blows                    = find_talent_spell( talent_tree::CLASS, "Furious Blows" );

  talents.warrior.wrecking_throw                   = find_talent_spell( talent_tree::CLASS, "Wrecking Throw" );
  talents.warrior.shattering_throw                 = find_talent_spell( talent_tree::CLASS, "Shattering Throw" );
  talents.warrior.crushing_force                   = find_talent_spell( talent_tree::CLASS, "Crushing Force" );
  talents.warrior.pain_and_gain                    = find_talent_spell( talent_tree::CLASS, "Pain and Gain" );
  talents.warrior.cacophonous_roar                 = find_talent_spell( talent_tree::CLASS, "Cacophonous Roar" );
  talents.warrior.menace                           = find_talent_spell( talent_tree::CLASS, "Menace" );
  talents.warrior.storm_bolt                       = find_talent_spell( talent_tree::CLASS, "Storm Bolt" );
  talents.warrior.overwhelming_rage                = find_talent_spell( talent_tree::CLASS, "Overwhelming Rage" );
  talents.warrior.barbaric_training                = find_talent_spell( talent_tree::CLASS, "Barbaric Training" );
  talents.warrior.concussive_blows                 = find_talent_spell( talent_tree::CLASS, "Concussive Blows" );

  talents.warrior.reinforced_plates                = find_talent_spell( talent_tree::CLASS, "Reinforced Plates" );
  talents.warrior.bounding_stride                  = find_talent_spell( talent_tree::CLASS, "Bounding Stride" );
  talents.warrior.blood_and_thunder                = find_talent_spell( talent_tree::CLASS, "Blood and Thunder" );
  talents.warrior.crackling_thunder                = find_talent_spell( talent_tree::CLASS, "Crackling Thunder" );
  talents.warrior.sidearm                          = find_talent_spell( talent_tree::CLASS, "Sidearm" );

  talents.warrior.honed_reflexes                   = find_talent_spell( talent_tree::CLASS, "Honed Reflexes" );
  talents.warrior.bitter_immunity                  = find_talent_spell( talent_tree::CLASS, "Bitter Immunity" );
  talents.warrior.double_time                      = find_talent_spell( talent_tree::CLASS, "Double Time" );
  talents.warrior.titanic_throw                    = find_talent_spell( talent_tree::CLASS, "Titanic Throw" );
  talents.warrior.seismic_reverberation            = find_talent_spell( talent_tree::CLASS, "Seismic Reverberation" );

  talents.warrior.armored_to_the_teeth             = find_talent_spell( talent_tree::CLASS, "Armored to the Teeth" );
  talents.warrior.wild_strikes                     = find_talent_spell( talent_tree::CLASS, "Wild Strikes" );
  talents.warrior.one_handed_weapon_specialization = find_talent_spell( talent_tree::CLASS, "One-Handed Weapon Specialization" );
  talents.warrior.two_handed_weapon_specialization = find_talent_spell( talent_tree::CLASS, "Two-Handed Weapon Specialization" );
  talents.warrior.dual_wield_specialization        = find_talent_spell( talent_tree::CLASS, "Dual Wield Specialization" );
  talents.warrior.cruel_strikes                    = find_talent_spell( talent_tree::CLASS, "Cruel Strikes" );
  talents.warrior.endurance_training               = find_talent_spell( talent_tree::CLASS, "Endurance Training", specialization() );

  talents.warrior.avatar                           = find_talent_spell( talent_tree::CLASS, "Avatar" );
  talents.warrior.thunderous_roar                  = find_talent_spell( talent_tree::CLASS, "Thunderous Roar" );
  talents.warrior.spear_of_bastion                 = find_talent_spell( talent_tree::CLASS, "Spear of Bastion" );
  talents.warrior.shockwave                        = find_talent_spell( talent_tree::CLASS, "Shockwave" );

  talents.warrior.immovable_object                 = find_talent_spell( talent_tree::CLASS, "Immovable Object" );
  talents.warrior.unstoppable_force                = find_talent_spell( talent_tree::CLASS, "Unstoppable Force" );
  talents.warrior.blademasters_torment             = find_talent_spell( talent_tree::CLASS, "Blademaster's Torment" );
  talents.warrior.warlords_torment                 = find_talent_spell( talent_tree::CLASS, "Warlord's Torment" );
  talents.warrior.berserkers_torment               = find_talent_spell( talent_tree::CLASS, "Berserker's Torment" );
  talents.warrior.titans_torment                   = find_talent_spell( talent_tree::CLASS, "Titan's Torment" );
  talents.warrior.uproar                           = find_talent_spell( talent_tree::CLASS, "Uproar" );
  talents.warrior.thunderous_words                 = find_talent_spell( talent_tree::CLASS, "Thunderous Words" );
  talents.warrior.piercing_verdict                 = find_talent_spell( talent_tree::CLASS, "Piercing Verdict" );
  talents.warrior.elysian_might                    = find_talent_spell( talent_tree::CLASS, "Elysian Might" );
  talents.warrior.rumbling_earth                   = find_talent_spell( talent_tree::CLASS, "Rumbling Earth" );
  talents.warrior.sonic_boom                       = find_talent_spell( talent_tree::CLASS, "Sonic Boom" );

  // Arms Talents
  talents.arms.mortal_strike                       = find_talent_spell( talent_tree::SPECIALIZATION, "Mortal Strike" );

  talents.arms.overpower                           = find_talent_spell( talent_tree::SPECIALIZATION, "Overpower" );

  talents.arms.martial_prowess                     = find_talent_spell( talent_tree::SPECIALIZATION, "Martial Prowess" );
  talents.arms.die_by_the_sword                    = find_talent_spell( talent_tree::SPECIALIZATION, "Die by the Sword" );
  talents.arms.improved_execute                    = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Execute", WARRIOR_ARMS );

  talents.arms.improved_overpower                  = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Overpower" );
  talents.arms.bloodsurge                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodsurge", WARRIOR_ARMS );
  talents.arms.fueled_by_violence                  = find_talent_spell( talent_tree::SPECIALIZATION, "Fueled by Violence", WARRIOR_ARMS );
  talents.arms.storm_wall                          = find_talent_spell( talent_tree::SPECIALIZATION, "Storm Wall" );
  talents.arms.sudden_death                        = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Death", WARRIOR_ARMS );
  talents.arms.fervor_of_battle                    = find_talent_spell( talent_tree::SPECIALIZATION, "Fervor of Battle" );

  talents.arms.tactician                            = find_talent_spell( talent_tree::SPECIALIZATION, "Tactician" );
  talents.arms.colossus_smash                      = find_talent_spell( talent_tree::SPECIALIZATION, "Colossus Smash" );
  talents.arms.impale                              = find_talent_spell( talent_tree::SPECIALIZATION, "Impale" );

  talents.arms.skullsplitter                       = find_talent_spell( talent_tree::SPECIALIZATION, "Skullsplitter" );
  talents.arms.rend                                = find_talent_spell( talent_tree::SPECIALIZATION, "Rend", WARRIOR_ARMS );
  talents.arms.exhilarating_blows                  = find_talent_spell( talent_tree::SPECIALIZATION, "Exhilarating Blows" );
  talents.arms.anger_management                    = find_talent_spell( talent_tree::SPECIALIZATION, "Anger Management" );
  talents.arms.massacre                            = find_talent_spell( talent_tree::SPECIALIZATION, "Massacre", WARRIOR_ARMS );
  talents.arms.cleave                              = find_talent_spell( talent_tree::SPECIALIZATION, "Cleave" );

  talents.arms.tide_of_blood                       = find_talent_spell( talent_tree::SPECIALIZATION, "Tide of Blood" );
  talents.arms.bloodborne                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodborne", WARRIOR_ARMS );
  talents.arms.dreadnaught                         = find_talent_spell( talent_tree::SPECIALIZATION, "Dreadnaught" );
  talents.arms.in_for_the_kill                     = find_talent_spell( talent_tree::SPECIALIZATION, "In for the Kill" );
  talents.arms.test_of_might                       = find_talent_spell( talent_tree::SPECIALIZATION, "Test of Might" );
  talents.arms.blunt_instruments                   = find_talent_spell( talent_tree::SPECIALIZATION, "Blunt Instruments" );
  talents.arms.warbreaker                          = find_talent_spell( talent_tree::SPECIALIZATION, "Warbreaker" );
  talents.arms.storm_of_swords                     = find_talent_spell( talent_tree::SPECIALIZATION, "Storm of Swords", WARRIOR_ARMS );
  talents.arms.collateral_damage                   = find_talent_spell( talent_tree::SPECIALIZATION, "Collateral Damage" );
  talents.arms.reaping_swings                      = find_talent_spell( talent_tree::SPECIALIZATION, "Reaping Swings" );

  talents.arms.deft_experience                     = find_talent_spell( talent_tree::SPECIALIZATION, "Deft Experience", WARRIOR_ARMS );
  talents.arms.valor_in_victory                    = find_talent_spell( talent_tree::SPECIALIZATION, "Valor in Victory" );
  talents.arms.critical_thinking                   = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Thinking", WARRIOR_ARMS );

  talents.arms.bloodletting                        = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodletting" );
  talents.arms.battlelord                          = find_talent_spell( talent_tree::SPECIALIZATION, "Battlelord" );
  talents.arms.bladestorm                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bladestorm" );
  talents.arms.sharpened_blades                    = find_talent_spell( talent_tree::SPECIALIZATION, "Sharpened Blades" );
  talents.arms.executioners_precision              = find_talent_spell( talent_tree::SPECIALIZATION, "Executioner's Precision" );

  talents.arms.fatality                            = find_talent_spell( talent_tree::SPECIALIZATION, "Fatality" );
  talents.arms.dance_of_death                      = find_talent_spell( talent_tree::SPECIALIZATION, "Dance of Death", WARRIOR_ARMS );
  talents.arms.unhinged                            = find_talent_spell( talent_tree::SPECIALIZATION, "Unhinged" );
  talents.arms.hurricane                           = find_talent_spell( talent_tree::SPECIALIZATION, "Hurricane", WARRIOR_ARMS );
  talents.arms.merciless_bonegrinder               = find_talent_spell( talent_tree::SPECIALIZATION, "Merciless Bonegrinder" );
  talents.arms.juggernaut                          = find_talent_spell( talent_tree::SPECIALIZATION, "Juggernaut", WARRIOR_ARMS );

  talents.arms.improved_slam                       = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Slam", WARRIOR_ARMS );
  talents.arms.improved_sweeping_strikes           = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Sweeping Strikes", WARRIOR_ARMS );
  talents.arms.strength_of_arms                    = find_talent_spell( talent_tree::SPECIALIZATION, "Strength of Arms", WARRIOR_ARMS );
  talents.arms.spiteful_serenity                   = find_talent_spell( talent_tree::SPECIALIZATION, "Spiteful Serenity", WARRIOR_ARMS );

  // Fury Talents
  talents.fury.bloodthirst          = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodthirst" );

  talents.fury.raging_blow          = find_talent_spell( talent_tree::SPECIALIZATION, "Raging Blow" );

  talents.fury.improved_enrage      = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Enrage" );
  talents.fury.enraged_regeneration = find_talent_spell( talent_tree::SPECIALIZATION, "Enraged Regeneration" );
  talents.fury.improved_execute     = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Execute", WARRIOR_FURY );

  talents.fury.improved_bloodthirst = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Bloodthirst" );
  talents.fury.fresh_meat           = find_talent_spell( talent_tree::SPECIALIZATION, "Fresh Meat" );
  talents.fury.warpaint             = find_talent_spell( talent_tree::SPECIALIZATION, "Warpaint" );
  talents.fury.invigorating_fury    = find_talent_spell( talent_tree::SPECIALIZATION, "Invigorating Fury" );
  talents.fury.sudden_death         = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Death", WARRIOR_FURY );
  talents.fury.improved_raging_blow = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Raging Blow" );

  talents.fury.focus_in_chaos       = find_talent_spell( talent_tree::SPECIALIZATION, "Focus in Chaos" );
  talents.fury.rampage              = find_talent_spell( talent_tree::SPECIALIZATION, "Rampage" );
  talents.fury.cruelty              = find_talent_spell( talent_tree::SPECIALIZATION, "Cruelty" );

  talents.fury.single_minded_fury   = find_talent_spell( talent_tree::SPECIALIZATION, "Single-Minded Fury" );
  talents.fury.cold_steel_hot_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Cold Steel, Hot Blood" );
  talents.fury.vicious_contempt     = find_talent_spell( talent_tree::SPECIALIZATION, "Vicious Contempt" );
  talents.fury.frenzy               = find_talent_spell( talent_tree::SPECIALIZATION, "Frenzy" );
  talents.fury.hack_and_slash       = find_talent_spell( talent_tree::SPECIALIZATION, "Hack and Slash" );
  talents.fury.slaughtering_strikes = find_talent_spell( talent_tree::SPECIALIZATION, "Slaughtering Strikes" );
  talents.fury.ashen_juggernaut     = find_talent_spell( talent_tree::SPECIALIZATION, "Ashen Juggernaut" );
  talents.fury.improved_whirlwind   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Whirlwind" );
  talents.fury.wrath_and_fury       = find_talent_spell( talent_tree::SPECIALIZATION, "Wrath and Fury" );

  talents.fury.frenzied_flurry      = find_talent_spell( talent_tree::SPECIALIZATION, "Frenzied Flurry" );
  talents.fury.bloodborne           = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodborne", WARRIOR_FURY );
  talents.fury.bloodcraze           = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodcraze" );
  talents.fury.recklessness         = find_talent_spell( talent_tree::SPECIALIZATION, "Recklessness" );
  talents.fury.massacre             = find_talent_spell( talent_tree::SPECIALIZATION, "Massacre", WARRIOR_FURY );
  talents.fury.meat_cleaver         = find_talent_spell( talent_tree::SPECIALIZATION, "Meat Cleaver" );
  talents.fury.raging_armaments     = find_talent_spell( talent_tree::SPECIALIZATION, "Raging Armaments" );

  talents.fury.deft_experience      = find_talent_spell( talent_tree::SPECIALIZATION, "Deft Experience", WARRIOR_FURY );
  talents.fury.swift_strikes        = find_talent_spell( talent_tree::SPECIALIZATION, "Swift Strikes" );
  talents.fury.critical_thinking    = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Thinking", WARRIOR_FURY );
  talents.fury.storm_of_swords      = find_talent_spell( talent_tree::SPECIALIZATION, "Storm of Swords", WARRIOR_FURY );
  talents.fury.odyns_fury           = find_talent_spell( talent_tree::SPECIALIZATION, "Odyn's Fury" );
  talents.fury.anger_management     = find_talent_spell( talent_tree::SPECIALIZATION, "Anger Management" );
  talents.fury.reckless_abandon     = find_talent_spell( talent_tree::SPECIALIZATION, "Reckless Abandon" );
  talents.fury.onslaught            = find_talent_spell( talent_tree::SPECIALIZATION, "Onslaught" );
  talents.fury.ravager              = find_talent_spell( talent_tree::SPECIALIZATION, "Ravager", WARRIOR_FURY );

  talents.fury.annihilator          = find_talent_spell( talent_tree::SPECIALIZATION, "Annihilator" );
  talents.fury.dancing_blades       = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing Blades" );
  talents.fury.titanic_rage         = find_talent_spell( talent_tree::SPECIALIZATION, "Titanic Rage" );
  talents.fury.unbridled_ferocity   = find_talent_spell( talent_tree::SPECIALIZATION, "Unbridled Ferocity" );
  talents.fury.depths_of_insanity   = find_talent_spell( talent_tree::SPECIALIZATION, "Depths of Insanity" );
  talents.fury.tenderize            = find_talent_spell( talent_tree::SPECIALIZATION, "Tenderize" );
  talents.fury.storm_of_steel       = find_talent_spell( talent_tree::SPECIALIZATION, "Storm of Steel", WARRIOR_FURY );
  talents.fury.hurricane            = find_talent_spell( talent_tree::SPECIALIZATION, "Hurricane", WARRIOR_FURY ); 

  // Protection Talents
  talents.protection.ignore_pain            = find_talent_spell( talent_tree::SPECIALIZATION, "Ignore Pain" );

  talents.protection.revenge                = find_talent_spell( talent_tree::SPECIALIZATION, "Revenge" );

  talents.protection.demoralizing_shout     = find_talent_spell( talent_tree::SPECIALIZATION, "Demoralizing Shout" );
  talents.protection.devastator             = find_talent_spell( talent_tree::SPECIALIZATION, "Devastator" );
  talents.protection.last_stand             = find_talent_spell( talent_tree::SPECIALIZATION, "Last Stand" );

  talents.protection.improved_heroic_throw  = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Heroic Throw" );
  talents.protection.best_served_cold       = find_talent_spell( talent_tree::SPECIALIZATION, "Best Served Cold" );
  talents.protection.strategist             = find_talent_spell( talent_tree::SPECIALIZATION, "Strategist" );
  talents.protection.brace_for_impact       = find_talent_spell( talent_tree::SPECIALIZATION, "Brace for Impact" );
  talents.protection.unnerving_focus        = find_talent_spell( talent_tree::SPECIALIZATION, "Unnerving Focus" );

  talents.protection.challenging_shout      = find_talent_spell( talent_tree::SPECIALIZATION, "Challenging Shout" );
  talents.protection.instigate              = find_talent_spell( talent_tree::SPECIALIZATION, "Instigate" );
  talents.protection.rend                   = find_talent_spell( talent_tree::SPECIALIZATION, "Rend", WARRIOR_PROTECTION );
  talents.protection.bloodsurge             = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodsurge", WARRIOR_PROTECTION );
  talents.protection.fueled_by_violence     = find_talent_spell( talent_tree::SPECIALIZATION, "Fueled by Violence", WARRIOR_PROTECTION );
  talents.protection.brutal_vitality        = find_talent_spell( talent_tree::SPECIALIZATION, "Brutal Vitality" ); // NYI

  talents.protection.disrupting_shout       = find_talent_spell( talent_tree::SPECIALIZATION, "Disrupting Shout" );
  talents.protection.show_of_force          = find_talent_spell( talent_tree::SPECIALIZATION, "Show of Force" );
  talents.protection.sudden_death           = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Death", WARRIOR_PROTECTION );
  talents.protection.thunderlord            = find_talent_spell( talent_tree::SPECIALIZATION, "Thunderlord" );
  talents.protection.shield_wall            = find_talent_spell( talent_tree::SPECIALIZATION, "Shield Wall" );
  talents.protection.bolster                = find_talent_spell( talent_tree::SPECIALIZATION, "Bolster" );
  talents.protection.tough_as_nails         = find_talent_spell( talent_tree::SPECIALIZATION, "Tough as Nails" );
  talents.protection.spell_block            = find_talent_spell( talent_tree::SPECIALIZATION, "Spell Block" );
  talents.protection.bloodborne             = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodborne", WARRIOR_PROTECTION );

  talents.protection.heavy_repercussions    = find_talent_spell( talent_tree::SPECIALIZATION, "Heavy Repercussions" );
  talents.protection.into_the_fray          = find_talent_spell( talent_tree::SPECIALIZATION, "Into the Fray" );
  talents.protection.enduring_defenses      = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Defenses" );
  talents.protection.massacre               = find_talent_spell( talent_tree::SPECIALIZATION, "Massacre", WARRIOR_PROTECTION );
  talents.protection.anger_management       = find_talent_spell( talent_tree::SPECIALIZATION, "Anger Management" );
  talents.protection.defenders_aegis        = find_talent_spell( talent_tree::SPECIALIZATION, "Defender's Aegis" );
  talents.protection.impenetrable_wall      = find_talent_spell( talent_tree::SPECIALIZATION, "Impenetrable Wall" );
  talents.protection.punish                 = find_talent_spell( talent_tree::SPECIALIZATION, "Punish" );
  talents.protection.juggernaut             = find_talent_spell( talent_tree::SPECIALIZATION, "Juggernaut", WARRIOR_PROTECTION );

  talents.protection.focused_vigor          = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Vigor" );
  talents.protection.shield_specialization  = find_talent_spell( talent_tree::SPECIALIZATION, "Shield Specialization" );
  talents.protection.enduring_alacrity      = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Alacrity" );

  talents.protection.shield_charge          = find_talent_spell( talent_tree::SPECIALIZATION, "Shield Charge" );
  talents.protection.booming_voice          = find_talent_spell( talent_tree::SPECIALIZATION, "Booming Voice" );
  talents.protection.indomitable            = find_talent_spell( talent_tree::SPECIALIZATION, "Indomitable" );
  talents.protection.violent_outburst       = find_talent_spell( talent_tree::SPECIALIZATION, "Violent Outburst" );
  talents.protection.ravager                = find_talent_spell( talent_tree::SPECIALIZATION, "Ravager", WARRIOR_PROTECTION );

  talents.protection.battering_ram          = find_talent_spell( talent_tree::SPECIALIZATION, "Battering Ram" );
  talents.protection.champions_bulwark      = find_talent_spell( talent_tree::SPECIALIZATION, "Champion's Bulwark" );
  talents.protection.battle_scarred_veteran = find_talent_spell( talent_tree::SPECIALIZATION, "Battle-Scarred Veteran" );
  talents.protection.dance_of_death         = find_talent_spell( talent_tree::SPECIALIZATION, "Dance of Death", WARRIOR_PROTECTION );
  talents.protection.storm_of_steel         = find_talent_spell( talent_tree::SPECIALIZATION, "Storm of Steel", WARRIOR_PROTECTION );

  // Shared Talents - needed when using the same spell data with a spec check (hurricane)

  auto find_shared_talent = []( std::vector<player_talent_t*> talents ) {
    for ( const auto t : talents )
    {
      if ( t->ok() )
      {
        return *t;
      }
    }
    return *talents[ 0 ];
  };

  talents.shared.hurricane = find_shared_talent( { &talents.arms.hurricane, &talents.fury.hurricane } );
  talents.shared.ravager = find_shared_talent( { &talents.fury.ravager, &talents.protection.ravager } );
  talents.shared.bloodsurge = find_shared_talent( { &talents.arms.bloodsurge, &talents.protection.bloodsurge } );

  // All
  azerite.breach           = find_azerite_spell( "Breach" );
  azerite.moment_of_glory  = find_azerite_spell( "Moment of Glory" );
  azerite.bury_the_hatchet = find_azerite_spell( "Bury the Hatchet" );
  // Prot
  azerite.iron_fortress      = find_azerite_spell( "Iron Fortress" );
  azerite.deafening_crash    = find_azerite_spell( "Deafening Crash" );
  azerite.callous_reprisal   = find_azerite_spell( "Callous Reprisal" );
  azerite.brace_for_impact   = find_azerite_spell( "Brace for Impact" );
  azerite.bloodsport         = find_azerite_spell( "Bloodsport" );
  azerite.bastion_of_might   = find_azerite_spell( "Bastion of Might" );
  // Arms
  azerite.test_of_might          = find_azerite_spell( "Test of Might" );
  azerite.seismic_wave           = find_azerite_spell( "Seismic Wave" );
  azerite.lord_of_war            = find_azerite_spell( "Lord of War" );
  azerite.gathering_storm        = find_azerite_spell( "Gathering Storm" );
  azerite.crushing_assault       = find_azerite_spell( "Crushing Assault" );
  azerite.striking_the_anvil     = find_azerite_spell( "Striking the Anvil" );
  // Fury
  azerite.trample_the_weak  = find_azerite_spell( "Trample the Weak" );
  azerite.simmering_rage    = find_azerite_spell( "Simmering Rage" );
  azerite.reckless_flurry   = find_azerite_spell( "Reckless Flurry" );
  azerite.pulverizing_blows = find_azerite_spell( "Pulverizing Blows" );
  azerite.infinite_fury     = find_azerite_spell( "Infinite Fury" );
  azerite.cold_steel_hot_blood   = find_azerite_spell( "Cold Steel, Hot Blood" );
  azerite.unbridled_ferocity     = find_azerite_spell( "Unbridled Ferocity" );
  // Essences
  azerite.memory_of_lucid_dreams = find_azerite_essence( "Memory of Lucid Dreams" );
  azerite_spells.memory_of_lucid_dreams = azerite.memory_of_lucid_dreams.spell( 1U, essence_type::MINOR );
  azerite.vision_of_perfection          = find_azerite_essence( "Vision of Perfection" );
  azerite.vision_of_perfection_percentage =
      azerite.vision_of_perfection.spell( 1U, essence_type::MAJOR )->effectN( 1 ).percent();
  azerite.vision_of_perfection_percentage +=
      azerite.vision_of_perfection.spell( 2U, essence_spell::UPGRADE, essence_type::MAJOR )->effectN( 1 ).percent();

  // Convenant Abilities
  covenant.ancient_aftershock    = find_covenant_spell( "Ancient Aftershock" );
  spell.aftershock_duration      = find_spell( 343607 );
  covenant.condemn_driver        = find_covenant_spell( "Condemn" );
  if ( specialization() == WARRIOR_FURY )
  covenant.condemn               = find_spell( as<unsigned>( covenant.condemn_driver->effectN( 2 ).base_value() ) );
  else
  covenant.condemn               = find_spell(as<unsigned>( covenant.condemn_driver->effectN( 1 ).base_value() ) );
  covenant.conquerors_banner     = find_covenant_spell( "Conqueror's Banner" );
  covenant.kyrian_spear          = find_covenant_spell( "Spear of Bastion" );

  // Conduits ===============================================================

  conduit.ashen_juggernaut            = find_conduit_spell( "Ashen Juggernaut" );
  conduit.crash_the_ramparts          = find_conduit_spell( "Crash the Ramparts" );
  conduit.merciless_bonegrinder       = find_conduit_spell( "Merciless Bonegrinder" );
  conduit.mortal_combo                = find_conduit_spell( "Mortal Combo");

  conduit.depths_of_insanity          = find_conduit_spell( "Depths of Insanity" );
  conduit.hack_and_slash              = find_conduit_spell( "Hack and Slash");
  conduit.vicious_contempt            = find_conduit_spell( "Vicious Contempt" );

  conduit.destructive_reverberations  = find_conduit_spell( "Destructive Reverberations");
  conduit.harrowing_punishment        = find_conduit_spell( "Harrowing Punishment" );
  conduit.piercing_verdict            = find_conduit_spell( "Piercing Verdict" );
  conduit.veterans_repute             = find_conduit_spell( "Veteran's Repute" );
  conduit.show_of_force               = find_conduit_spell( "Show of Force" );
  conduit.unnerving_focus             = find_conduit_spell( "Unnerving Focus" );

  // Tier Sets
  tier_set.t29_arms_2pc               = sets->set( WARRIOR_ARMS, T29, B2 );
  tier_set.t29_arms_4pc               = sets->set( WARRIOR_ARMS, T29, B4 );
  tier_set.t29_fury_2pc               = sets->set( WARRIOR_FURY, T29, B2 );
  tier_set.t29_fury_4pc               = sets->set( WARRIOR_FURY, T29, B4 );
  tier_set.t29_prot_2pc               = sets->set( WARRIOR_PROTECTION, T29, B2 );
  tier_set.t29_prot_4pc               = sets->set( WARRIOR_PROTECTION, T29, B4 );
  tier_set.t30_arms_2pc               = sets->set( WARRIOR_ARMS, T30, B2 );
  tier_set.t30_arms_4pc               = sets->set( WARRIOR_ARMS, T30, B4 );
  tier_set.t30_fury_2pc               = sets->set( WARRIOR_FURY, T30, B2 );
  tier_set.t30_fury_4pc               = sets->set( WARRIOR_FURY, T30, B4 );
  tier_set.t30_prot_2pc               = sets->set( WARRIOR_PROTECTION, T30, B2 );
  tier_set.t30_prot_4pc               = sets->set( WARRIOR_PROTECTION, T30, B4 );

  // Active spells
  //active.ancient_aftershock_pulse = nullptr;
  active.deep_wounds_ARMS = nullptr;
  active.deep_wounds_PROT = nullptr;
  active.fatality         = nullptr;

  // Runeforged Legendary Items
  legendary.elysian_might             = find_runeforge_legendary( "Elysian Might" );
  legendary.glory                     = find_runeforge_legendary( "Glory" );
  legendary.leaper                    = find_runeforge_legendary( "Leaper" );
  legendary.misshapen_mirror          = find_runeforge_legendary( "Misshapen Mirror" );
  legendary.natures_fury              = find_runeforge_legendary( "Nature's Fury" );
  legendary.seismic_reverberation     = find_runeforge_legendary( "Seismic Reverberation" );
  legendary.signet_of_tormented_kings = find_runeforge_legendary( "Signet of Tormented Kings" );
  legendary.sinful_surge              = find_runeforge_legendary( "Sinful Surge" );

  legendary.battlelord                = find_runeforge_legendary( "Battlelord" );
  legendary.enduring_blow             = find_runeforge_legendary( "Enduring Blow" );
  legendary.exploiter                 = find_runeforge_legendary( "Exploiter" );
  legendary.unhinged                  = find_runeforge_legendary( "Unhinged" );

  legendary.cadence_of_fujieda        = find_runeforge_legendary( "Cadence of Fujieda" );
  legendary.deathmaker                = find_runeforge_legendary( "Deathmaker" );
  legendary.reckless_defense          = find_runeforge_legendary( "Reckless Defense" );
  legendary.will_of_the_berserker     = find_runeforge_legendary( "Will of the Berserker" );

  legendary.the_wall                  = find_runeforge_legendary( "The Wall" );
  legendary.thunderlord               = find_runeforge_legendary( "Thunderlord" );
  legendary.reprisal                  = find_runeforge_legendary( "Reprisal" );

  // AA Mods Not Handled by affecting_aura
  if ( specialization() == WARRIOR_FURY )
  {
  auto_attack_multiplier *= 1.0 + spec.fury_warrior->effectN( 4 ).percent();
  }
  if ( specialization() == WARRIOR_ARMS )
  {
    auto_attack_multiplier *= 1.0 + spec.arms_warrior->effectN( 6 ).percent();
  }
  if ( specialization() == WARRIOR_FURY && main_hand_weapon.group() == WEAPON_1H &&
             off_hand_weapon.group() == WEAPON_1H && talents.fury.single_minded_fury->ok() )
  {
    auto_attack_multiplier *= 1.0 + talents.fury.single_minded_fury->effectN( 4 ).percent();
  }
  if ( specialization() == WARRIOR_FURY && main_hand_weapon.group() == WEAPON_1H &&
       off_hand_weapon.group() == WEAPON_1H && talents.fury.frenzied_flurry->ok() )
  {
    auto_attack_multiplier *= 1.0 + talents.fury.frenzied_flurry->effectN( 1 ).percent();
  }
  if ( specialization() == WARRIOR_FURY && talents.warrior.dual_wield_specialization->ok() )
  {
  auto_attack_multiplier *= 1.0 + talents.warrior.dual_wield_specialization->effectN( 3 ).percent();
  }
  if ( specialization() == WARRIOR_ARMS && talents.warrior.two_handed_weapon_specialization->ok() ) 
  {
    auto_attack_multiplier *= 1.0 + talents.warrior.two_handed_weapon_specialization->effectN( 3 ).percent();
  }
  if ( specialization() == WARRIOR_PROTECTION && talents.warrior.one_handed_weapon_specialization->ok() )
  {
    auto_attack_multiplier *= 1.0 + talents.warrior.one_handed_weapon_specialization->effectN( 3 ).percent();
  }

  if ( legendary.natures_fury->ok() )
    active.natures_fury = new natures_fury_dot_t( this );
  if ( covenant.ancient_aftershock->ok() )
    active.ancient_aftershock_pulse = new ancient_aftershock_pulse_t( this );
  if ( covenant.kyrian_spear->ok() )
    active.kyrian_spear_attack = new kyrian_spear_attack_t( this );
  if ( spec.deep_wounds_ARMS->ok() )
    active.deep_wounds_ARMS = new deep_wounds_ARMS_t( this );
  if ( spec.deep_wounds_PROT->ok() )
    active.deep_wounds_PROT = new deep_wounds_PROT_t( this );
  if ( talents.arms.fatality->ok() )
    active.fatality = new fatality_t( this );
  if ( talents.warrior.spear_of_bastion->ok() )
    active.spear_of_bastion_attack = new spear_of_bastion_attack_t( this );
  if ( talents.fury.rampage->ok() )
  {
    // rampage now hits 4 times instead of 5 and effect indexes shifted
    rampage_attack_t* first  = new rampage_attack_t( this, talents.fury.rampage->effectN( 2 ).trigger(), "rampage1" );
    rampage_attack_t* second = new rampage_attack_t( this, talents.fury.rampage->effectN( 3 ).trigger(), "rampage2" );
    rampage_attack_t* third  = new rampage_attack_t( this, talents.fury.rampage->effectN( 4 ).trigger(), "rampage3" );
    rampage_attack_t* fourth = new rampage_attack_t( this, talents.fury.rampage->effectN( 5 ).trigger(), "rampage4" );

    // the order for hits is now OH MH OH MH
    first->weapon  = &( this->off_hand_weapon );
    second->weapon = &( this->main_hand_weapon );
    third->weapon  = &( this->off_hand_weapon );
    fourth->weapon = &( this->main_hand_weapon );

    this->rampage_attacks.push_back( first );
    this->rampage_attacks.push_back( second );
    this->rampage_attacks.push_back( third );
    this->rampage_attacks.push_back( fourth );
  }
  if ( azerite.iron_fortress.enabled() )
  {
    active.iron_fortress = new iron_fortress_t( this );
  }
  if ( talents.protection.tough_as_nails->ok() )
  {
    active.tough_as_nails = new tough_as_nails_t( this );
  }
  if ( azerite.bastion_of_might.enabled() && specialization() == WARRIOR_PROTECTION )
  {
    active.bastion_of_might_ip = new ignore_pain_bom_t( this );
  }
  if ( legendary.signet_of_tormented_kings->ok() )
  {
    cooldown.signet_of_tormented_kings = get_cooldown( "signet_of_tormented_kings" );
    cooldown.signet_of_tormented_kings->duration = legendary.signet_of_tormented_kings->internal_cooldown();

    active.signet_bladestorm_a  = new bladestorm_t( this, "", "bladestorm_signet", find_spell( 227847 ), true );
    active.signet_bladestorm_f  = new bladestorm_t( this, "", "bladestorm_signet", find_spell( 46924 ), true );
    active.signet_recklessness  = new recklessness_t( this, "", "recklessness_signet", find_spell( 1719 ), true );
    active.signet_avatar        = new avatar_t( this, "", "avatar_signet", find_spell( 107574 ), true );
    for ( action_t* action : { active.signet_recklessness, active.signet_bladestorm_a, active.signet_bladestorm_f,
    active.signet_avatar } )
    {
      action->background = true;
      action->trigger_gcd = timespan_t::zero();
      action->cooldown = cooldown.signet_of_tormented_kings;
    }
  }
  if ( talents.warrior.berserkers_torment->ok() )
  {
    active.torment_recklessness = new torment_recklessness_t( this, "", "recklessness_torment", find_spell( 1719 ), true );
    active.torment_avatar       = new torment_avatar_t( this, "", "avatar_torment", find_spell( 107574 ), true );
    for ( action_t* action : { active.torment_recklessness, active.torment_avatar } )
    {
      action->background  = true;
      action->trigger_gcd = timespan_t::zero();
    }
  }
  if ( talents.warrior.blademasters_torment->ok() )
  {
    active.torment_avatar       = new torment_avatar_t( this, "", "avatar_torment", find_spell( 107574 ), true );
    active.torment_bladestorm   = new torment_bladestorm_t( this, "", "bladestorm_torment", find_spell( 227847 ), true );
    for ( action_t* action : { active.torment_avatar, active.torment_bladestorm } )
    {
      action->background  = true;
      action->trigger_gcd = timespan_t::zero();
    }
  }
  if ( talents.warrior.titans_torment->ok() )
  {
    active.torment_avatar       = new torment_avatar_t( this, "", "avatar_torment", find_spell( 107574 ), true );
    active.torment_odyns_fury   = new torment_odyns_fury_t( this, "", "odyns_fury_torment", find_spell( 385059 ), true );
    for ( action_t* action : { active.torment_avatar, active.torment_odyns_fury } )
    {
      action->background  = true;
      action->trigger_gcd = timespan_t::zero();
    }
  }

  // Cooldowns
  cooldown.avatar         = get_cooldown( "avatar" );
  cooldown.recklessness   = get_cooldown( "recklessness" );
  cooldown.berserker_rage = get_cooldown( "berserker_rage" );
  cooldown.bladestorm     = get_cooldown( "bladestorm" );
  cooldown.bloodthirst    = get_cooldown( "bloodthirst" );
  cooldown.bloodbath      = get_cooldown( "bloodbath" );

  cooldown.cleave                           = get_cooldown( "cleave" );
  cooldown.colossus_smash                   = get_cooldown( "colossus_smash" );
  cooldown.condemn                          = get_cooldown( "condemn" );
  cooldown.conquerors_banner                = get_cooldown( "conquerors_banner" );
  cooldown.demoralizing_shout               = get_cooldown( "demoralizing_shout" );
  cooldown.thunderous_roar                  = get_cooldown( "thunderous_roar" );
  cooldown.enraged_regeneration             = get_cooldown( "enraged_regeneration" );
  cooldown.execute                          = get_cooldown( "execute" );
  cooldown.heroic_leap                      = get_cooldown( "heroic_leap" );
  cooldown.iron_fortress_icd                = get_cooldown( "iron_fortress" );
  cooldown.iron_fortress_icd -> duration    = azerite.iron_fortress.spell() -> effectN( 1 ).trigger() -> internal_cooldown();
  cooldown.last_stand                       = get_cooldown( "last_stand" );
  cooldown.mortal_strike                    = get_cooldown( "mortal_strike" );
  cooldown.odyns_fury                       = get_cooldown( "odyns_fury" );
  cooldown.onslaught                        = get_cooldown( "onslaught" );
  cooldown.overpower                        = get_cooldown( "overpower" );
  cooldown.pummel                           = get_cooldown( "pummel" );
  cooldown.rage_from_auto_attack            = get_cooldown( "rage_from_auto_attack" );
  cooldown.rage_from_auto_attack->duration  = timespan_t::from_seconds ( 1.0 );
  cooldown.rage_from_crit_block             = get_cooldown( "rage_from_crit_block" );
  cooldown.rage_from_crit_block->duration   = timespan_t::from_seconds( 3.0 );
  cooldown.raging_blow                      = get_cooldown( "raging_blow" );
  cooldown.crushing_blow                    = get_cooldown( "raging_blow" );
  cooldown.ravager                          = get_cooldown( "ravager" );
  cooldown.shield_slam                      = get_cooldown( "shield_slam" );
  cooldown.shield_wall                      = get_cooldown( "shield_wall" );
  cooldown.skullsplitter                    = get_cooldown( "skullsplitter" );
  cooldown.shockwave                        = get_cooldown( "shockwave" );
  cooldown.storm_bolt                       = get_cooldown( "storm_bolt" );
  cooldown.tough_as_nails_icd               = get_cooldown( "tough_as_nails" );
  cooldown.tough_as_nails_icd -> duration   = talents.protection.tough_as_nails->effectN( 1 ).trigger() -> internal_cooldown();
  cooldown.thunder_clap                     = get_cooldown( "thunder_clap" );
  cooldown.warbreaker                       = get_cooldown( "warbreaker" );
  cooldown.cold_steel_hot_blood_icd         = get_cooldown( "cold_steel_hot_blood" );
  cooldown.cold_steel_hot_blood_icd -> duration = talents.fury.cold_steel_hot_blood->effectN( 2 ).trigger() -> internal_cooldown();
}

// warrior_t::init_base =====================================================

void warrior_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5.0;

  player_t::init_base_stats();

  resources.base[ RESOURCE_RAGE ] = 100;
  if ( talents.warrior.overwhelming_rage->ok() )
  {
    resources.base[ RESOURCE_RAGE ] += find_spell( 382767 )->effectN( 1 ).base_value() / 10.0;
  }
  resources.max[ RESOURCE_RAGE ]  = resources.base[ RESOURCE_RAGE ];

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base_gcd = timespan_t::from_seconds( 1.5 );

  resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1 + talents.protection.indomitable -> effectN( 3 ).percent();

  // Warriors gets +7% block from their class aura
  base.block += spell.warrior_aura -> effectN( 7 ).percent();

  // Protection Warriors have a +8% block chance in their spec aura
  base.block += spec.protection_warrior -> effectN( 9 ).percent();
}

// warrior_t::merge ==========================================================

void warrior_t::merge( player_t& other )
{
  player_t::merge( other );

  const warrior_t& s = static_cast<warrior_t&>( other );

  for ( size_t i = 0, end = cd_waste_exec.size(); i < end; i++ )
  {
    cd_waste_exec[ i ]->second.merge( s.cd_waste_exec[ i ]->second );
    cd_waste_cumulative[ i ]->second.merge( s.cd_waste_cumulative[ i ]->second );
  }
}

// warrior_t::datacollection_begin ===========================================

void warrior_t::datacollection_begin()
{
  if ( active_during_iteration )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_iter[ i ]->second.reset();
    }
  }

  player_t::datacollection_begin();
}

// warrior_t::datacollection_end =============================================

void warrior_t::datacollection_end()
{
  if ( requires_data_collection() )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_cumulative[ i ]->second.add( cd_waste_iter[ i ]->second.sum() );
    }
  }

  player_t::datacollection_end();
}

// NO Spec Combat Action Priority List

void warrior_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list->add_action( this, "Heroic Throw" );
}

// ==========================================================================
// Warrior Buffs
// ==========================================================================

namespace buffs
{
template <typename Base>
struct warrior_buff_t : public Base
{
public:
  using base_t = warrior_buff_t;


  warrior_buff_t( warrior_td_t& td, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( td, name, s, item )
  {
  }

  warrior_buff_t( warrior_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( &p, name, s, item )
  {
  }

protected:
  warrior_t& warrior()
  {
    return *debug_cast<warrior_t*>( Base::source );
  }
  const warrior_t& warrior() const
  {
    return *debug_cast<warrior_t*>( Base::source );
  }
};


// Rallying Cry ==============================================================

struct rallying_cry_t : public buff_t
{
  double health_change;
  rallying_cry_t( player_t* p ) :
    buff_t( p, "rallying_cry", p->find_spell( 97463 ) ), health_change( p->find_spell( 97462 )->effectN( 1 ).percent() )
  { }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    double old_health = player -> resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );
    player -> resources.current[ RESOURCE_HEALTH ] *= 1.0 + health_change; // Update health after the maximum is increased

    sim -> print_debug( "{} gains Rallying Cry: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }

  void expire_override( int, timespan_t ) override
  {
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];
    double old_health = player -> resources.current[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player -> resources.current[ RESOURCE_HEALTH ] /= 1.0 + health_change; // Update health before the maximum is reduced
    player -> recalculate_resource_max( RESOURCE_HEALTH );

    sim -> print_debug( "{} loses Rallying Cry: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }
};

// Last Stand ======================================================================

struct last_stand_buff_t : public warrior_buff_t<buff_t>
{
  double health_change;
  last_stand_buff_t( warrior_t& p, util::string_view n, const spell_data_t* s ) :
    base_t( p, n, s ), health_change( data().effectN( 1 ).percent() )
  {
    add_invalidate( CACHE_BLOCK );
    set_cooldown( timespan_t::zero() );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    warrior_buff_t<buff_t>::start( stacks, value, duration );

    double old_health = player -> resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );
    player -> resources.current[ RESOURCE_HEALTH ] *= 1.0 + health_change; // Update health after the maximum is increased

    sim -> print_debug( "{} gains Last Stand: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }

  void expire_override( int, timespan_t ) override
  {
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];
    double old_health = player -> resources.current[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    // Last Stand isn't like other health buffs, the player doesn't lose health when it expires (unless it's over the cap)
    // player -> resources.current[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );

    sim -> print_debug( "{} loses Last Stand: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );

    warrior_t* p = debug_cast< warrior_t* >( player );
    if ( ! p -> sim -> event_mgr.canceled && p -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B4 ) )
      p -> buff.earthen_tenacity -> trigger();
  }
};

// Demoralizing Shout ================================================================

struct debuff_demo_shout_t : public warrior_buff_t<buff_t>
{
  int extended;
  const int deafening_crash_cap;
  debuff_demo_shout_t( warrior_td_t& p, warrior_t* w )
    : base_t( p, "demoralizing_shout_debuff", w -> find_spell( 1160 ) ),
      extended( 0 ), deafening_crash_cap( as<int>( w -> azerite.deafening_crash.spell() -> effectN( 3 ).base_value() ) )
  {
    cooldown -> duration = timespan_t::zero(); // Cooldown handled by the action
    default_value = data().effectN( 1 ).percent();
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    extended = 0; // counts extra seconds gained on each debuff past the initial trigger
    return base_t::trigger( stacks, value, chance, duration );
  }

  // TODO? Change how the extension is coded with deafening crash cap
  // if another mechanic extends demo shout duration after the initial trigger
  void extend_duration( player_t* p, timespan_t extra_seconds ) override
  {
    if ( extended < deafening_crash_cap )
    {
      base_t::extend_duration( p, extra_seconds );
      extended += as<int>(extra_seconds.total_seconds() );
    }
  }
};

// Test of Might Tracker ===================================================================

struct test_of_might_t : public warrior_buff_t<buff_t>
{
  test_of_might_t( warrior_t& p, util::string_view n, const spell_data_t* s )
    : base_t( p, n, s )
  {
    quiet = true;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t* test_of_might = warrior().buff.test_of_might;
    test_of_might->expire();
    double strength = ( current_value / 10 ) * ( warrior().talents.arms.test_of_might->effectN( 1 ).percent() );
    test_of_might->trigger( 1, strength );
    current_value = 0;
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// That legendary crap ring ===========================================================

struct sephuzs_secret_buff_t : public buff_t
{
  cooldown_t* icd;
  sephuzs_secret_buff_t( warrior_t* p ) : buff_t( p, "sephuzs_secret", p->find_spell( 208052 ) )
  {
    set_default_value( p->find_spell( 208052 )->effectN( 2 ).percent() );
    add_invalidate( CACHE_ATTACK_HASTE );
    icd           = p->get_cooldown( "sephuzs_secret_cooldown" );
    icd->duration = p->find_spell( 226262 )->duration();
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    if ( icd->down() )
      return;
    buff_t::execute( stacks, value, duration );
    icd->start();
  }
};

}  // end namespace buffs

// ==========================================================================
// Warrior Character Definition
// ==========================================================================

warrior_td_t::warrior_td_t( player_t* target, warrior_t& p ) : actor_target_data_t( target, &p ), warrior( p )
{
  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
  using namespace buffs;

  hit_by_fresh_meat = false;
  dots_deep_wounds = target->get_dot( "deep_wounds", &p );
  dots_ravager     = target->get_dot( "ravager", &p );
  dots_rend        = target->get_dot( "rend", &p );
  dots_gushing_wound = target->get_dot( "gushing_wound", &p );
  dots_thunderous_roar = target->get_dot( "thunderous_roar", &p );

  debuffs_colossus_smash = make_buff( *this , "colossus_smash" )
                               ->set_default_value( p.spell.colossus_smash_debuff->effectN( 2 ).percent()
                                                    * ( 1.0 + p.talents.arms.spiteful_serenity->effectN( 7 ).percent() ) )
                               ->set_duration( p.spell.colossus_smash_debuff->duration()
                                               + p.talents.arms.spiteful_serenity->effectN( 8 ).time_value()
                                               + p.talents.arms.blunt_instruments->effectN( 2 ).time_value() )
                               ->set_cooldown( timespan_t::zero() );

  debuffs_concussive_blows = make_buff( *this , "concussive_blows" )
                               ->set_default_value( p.spell.concussive_blows_debuff->effectN( 1 ).percent() )
                               ->set_duration( p.spell.concussive_blows_debuff->duration() );

  debuffs_executioners_precision = make_buff( *this, "executioners_precision" ) 
          ->set_duration( p.spell.executioners_precision_debuff->duration() )
          ->set_max_stack( p.spell.executioners_precision_debuff->max_stacks() )
          ->set_default_value( p.talents.arms.executioners_precision->effectN( 1 ).percent() );

  debuffs_fatal_mark = make_buff( *this, "fatal_mark" ) 
          ->set_duration( p.spell.fatal_mark_debuff->duration() )
          ->set_max_stack( p.spell.fatal_mark_debuff->max_stacks() );

  debuffs_siegebreaker = make_buff( *this , "siegebreaker" )
    ->set_default_value( p.spell.siegebreaker_debuff->effectN( 2 ).percent() )
    ->set_duration( p.spell.siegebreaker_debuff->duration() )
    ->set_cooldown( timespan_t::zero() );

  debuffs_demoralizing_shout = new buffs::debuff_demo_shout_t( *this, &p );

  debuffs_punish = make_buff( *this, "punish", p.talents.protection.punish -> effectN( 2 ).trigger() )
    ->set_default_value( p.talents.protection.punish -> effectN( 2 ).trigger() -> effectN( 1 ).percent() );

  debuffs_callous_reprisal = make_buff( *this, "callous_reprisal",
                                             p.azerite.callous_reprisal.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
    ->set_default_value( p.azerite.callous_reprisal.spell() -> effectN( 1 ).percent() );

  debuffs_taunt = make_buff( *this, "taunt", p.find_class_spell( "Taunt" ) );

  debuffs_exploiter = make_buff( *this , "exploiter", p.find_spell( 335452 ) )
                               ->set_default_value( ( (player_t *)(&p))->covenant->id() == (unsigned int)covenant_e::VENTHYR
                                 ? p.find_spell( 335451 )->effectN( 1 ).percent() + p.covenant.condemn_driver->effectN( 6 ).percent()
                                 : p.find_spell( 335451 )->effectN( 1 ).percent() )
                               ->set_duration( p.find_spell( 335452 )->duration() )
                               ->set_cooldown( timespan_t::zero() );
}

void warrior_td_t::target_demise()
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source->sim->event_mgr.canceled )
    return;

  warrior_t* p = static_cast<warrior_t*>( source );

  if ( p -> talents.protection.dance_of_death.ok() && dots_ravager->is_ticking() )
  {
    p -> buff.dance_of_death_prot -> trigger();
  }

  if ( p -> talents.warrior.war_machine->ok() )
  {
    p->resource_gain( RESOURCE_RAGE, p -> talents.warrior.war_machine -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE ),
                          p->gain.war_machine_demise );
  }
}

// warrior_t::init_buffs ====================================================

void warrior_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  buff.ashen_juggernaut = make_buff( this, "ashen_juggernaut", find_spell( 392537 ) )
      ->set_default_value( find_spell( 392537 )->effectN( 1 ).percent() );

  buff.revenge =
      make_buff( this, "revenge", find_spell( 5302 ) )
      ->set_default_value( find_spell( 5302 )->effectN( 1 ).percent() )
      ->set_cooldown( spec.revenge_trigger -> internal_cooldown() );

  buff.avatar = make_buff( this, "avatar", talents.warrior.avatar )
      ->set_default_value( talents.warrior.avatar->effectN( 1 ).percent() 
                           * ( 1.0 + talents.arms.spiteful_serenity->effectN( 1 ).percent() ) )
      ->set_duration( talents.warrior.avatar->duration() + talents.arms.spiteful_serenity->effectN( 4 ).time_value() )
      ->set_chance(1)
      ->set_cooldown( timespan_t::zero() );

  buff.wild_strikes = make_buff( this, "wild_strikes", talents.warrior.wild_strikes )
      ->set_default_value( talents.warrior.wild_strikes->effectN( 2 ).base_value() / 100.0 )
      ->set_duration( find_spell( 392778 )->duration() )
      ->add_invalidate( CACHE_ATTACK_SPEED );

  buff.dancing_blades = make_buff( this, "dancing_blades", find_spell( 391688 ) )
      ->set_default_value( find_spell( 391688 )->effectN( 1 ).base_value() / 100.0 )
      ->add_invalidate( CACHE_ATTACK_SPEED )
      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC);

  buff.battering_ram = make_buff( this, "battering_ram", find_spell( 394313 ) )
      ->set_default_value( find_spell( 394313 )->effectN( 1 ).percent() )
      ->add_invalidate( CACHE_ATTACK_SPEED );

  if ( talents.warrior.unstoppable_force -> ok() )
    buff.avatar -> set_stack_change_callback( [ this ] ( buff_t*, int, int )
    { cooldown.thunder_clap -> adjust_recharge_multiplier(); } );

  buff.berserker_rage = make_buff( this, "berserker_rage", talents.warrior.berserker_rage )
      ->set_cooldown( timespan_t::zero() );

  buff.bounding_stride = make_buff( this, "bounding_stride", find_spell( 202164 ) )
    ->set_chance( talents.warrior.bounding_stride->ok() )
    ->set_default_value( find_spell( 202164 )->effectN( 1 ).percent() );

  buff.bladestorm =
      make_buff( this, "bladestorm", specialization() == WARRIOR_FURY ? find_spell( 46924 ) : talents.arms.bladestorm )
      ->set_period( timespan_t::zero() )
      ->set_cooldown( timespan_t::zero() );

  buff.battle_stance = make_buff( this, "battle_stance", talents.warrior.battle_stance )
    ->set_activated( true )
    ->set_default_value( talents.warrior.battle_stance->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.berserker_stance = make_buff( this, "berserker_stance", talents.warrior.berserker_stance )
    ->set_activated( true )
    ->set_default_value( talents.warrior.berserker_stance->effectN( 1 ).percent() );

  buff.defensive_stance = make_buff( this, "defensive_stance", talents.warrior.defensive_stance )
    ->set_activated( true )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.die_by_the_sword = make_buff( this, "die_by_the_sword", talents.arms.die_by_the_sword )
    ->set_default_value( talents.arms.die_by_the_sword->effectN( 2 ).percent() )
    ->set_cooldown( timespan_t::zero() )
    ->add_invalidate( CACHE_PARRY );


  buff.elysian_might = make_buff( this, "elysian_might", find_spell( 386286 ) )
     ->set_default_value( find_spell( 386286 )->effectN( 1 ).percent() )
     ->set_duration( find_spell( 376080 )->duration() +
                     talents.warrior.elysian_might->effectN( 1 ).trigger()->effectN( 1 ).time_value() );

  buff.enrage = make_buff( this, "enrage", find_spell( 184362 ) )
     ->add_invalidate( CACHE_ATTACK_HASTE )
     ->add_invalidate( CACHE_RUN_SPEED )
     ->set_default_value( find_spell( 184362 )->effectN( 1 ).percent() )
     ->set_duration( find_spell( 184362 )->duration() + talents.fury.tenderize->effectN( 1 ).time_value() );

  buff.frenzy = make_buff( this, "frenzy", find_spell(335082) )
                           ->set_default_value( find_spell( 335082 )->effectN( 1 ).percent() )
                           ->add_invalidate( CACHE_ATTACK_HASTE );

  buff.heroic_leap_movement   = make_buff( this, "heroic_leap_movement" );
  buff.charge_movement        = make_buff( this, "charge_movement" );
  buff.intervene_movement     = make_buff( this, "intervene_movement" );
  buff.intercept_movement     = make_buff( this, "intercept_movement" );
  buff.shield_charge_movement = make_buff( this, "shield_charge_movement" );

  buff.into_the_fray = make_buff( this, "into_the_fray", find_spell( 202602 ) )
    ->set_chance( talents.protection.into_the_fray->ok() )
    ->set_default_value( find_spell( 202602 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_HASTE );

  buff.juggernaut = make_buff( this, "juggernaut", talents.arms.juggernaut->effectN( 1 ).trigger() )
    ->set_default_value( talents.arms.juggernaut->effectN( 1 ).trigger()->effectN( 1 ).percent() )
    ->set_duration( talents.arms.juggernaut->effectN( 1 ).trigger()->duration() );

  buff.juggernaut_prot = make_buff( this, "juggernaut_prot", talents.protection.juggernaut->effectN( 1 ).trigger() )
    ->set_default_value( talents.protection.juggernaut->effectN( 1 ).trigger()->effectN( 1 ).percent() )
    ->set_duration( talents.protection.juggernaut->effectN( 1 ).trigger()->duration() )
    ->set_cooldown( talents.protection.juggernaut->internal_cooldown() );

  buff.last_stand = new buffs::last_stand_buff_t( *this, "last_stand", talents.protection.last_stand );

  buff.meat_cleaver = make_buff( this, "meat_cleaver", spell.whirlwind_buff );
  buff.meat_cleaver->set_max_stack(buff.meat_cleaver->max_stack() + as<int>( talents.fury.meat_cleaver->effectN( 2 ).base_value() ) );

  buff.martial_prowess =
    make_buff(this, "martial_prowess", talents.arms.martial_prowess)
    ->set_default_value(talents.arms.overpower->effectN(2).percent() );
  buff.martial_prowess->set_max_stack(buff.martial_prowess->max_stack() + as<int>( talents.arms.martial_prowess->effectN(2).base_value() ) );

  buff.merciless_bonegrinder = make_buff( this, "merciless_bonegrinder", find_spell( 383316 ) )
    ->set_default_value( find_spell( 383316 )->effectN( 1 ).percent() )
    ->set_duration( find_spell( 383316 )->duration() );

  buff.spell_reflection = make_buff( this, "spell_reflection", talents.warrior.spell_reflection )
    -> set_cooldown( 0_ms ); // handled by the ability

  buff.sweeping_strikes = make_buff(this, "sweeping_strikes", spec.sweeping_strikes)
    ->set_duration(spec.sweeping_strikes->duration() + talents.arms.improved_sweeping_strikes->effectN( 1 ).time_value() )
    ->set_cooldown(timespan_t::zero());

  buff.ignore_pain = new ignore_pain_buff_t( this );

  buff.recklessness = make_buff( this, "recklessness", spell.recklessness_buff )
    ->set_chance(1)
    ->set_duration( spell.recklessness_buff->duration() + talents.fury.depths_of_insanity->effectN( 1 ).time_value() )
    ->apply_affecting_conduit( conduit.depths_of_insanity )
    //->add_invalidate( CACHE_CRIT_CHANCE ) removed in favor of composite_crit_chance (line 1015)
    ->set_cooldown( timespan_t::zero() )
    ->set_default_value( spell.recklessness_buff->effectN( 1 ).percent() )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int after ) { if ( after == 0  && this->legendary.will_of_the_berserker->ok() ) buff.will_of_the_berserker->trigger(); });

  buff.reckless_abandon = make_buff( this, "reckless_abandon", find_spell( 396752 ) );

  buff.sudden_death = make_buff( this, "sudden_death", specialization() == WARRIOR_FURY ? talents.fury.sudden_death : specialization() == WARRIOR_ARMS ? talents.arms.sudden_death : talents.protection.sudden_death );
    if ( tier_set.t29_fury_4pc->ok() )
    buff.sudden_death->set_rppm( RPPM_NONE, -1, 2.5 ); // hardcode unsupported type 8 modifier

  buff.shield_block = make_buff( this, "shield_block", spell.shield_block_buff )
    ->set_duration( spell.shield_block_buff->duration() + talents.protection.enduring_defenses->effectN( 1 ).time_value() )
    ->set_cooldown( timespan_t::zero() )
    ->add_invalidate( CACHE_BLOCK );

  buff.shield_wall = make_buff( this, "shield_wall", spell.shield_wall )
    ->set_default_value( spell.shield_wall->effectN( 1 ).percent() )
    ->set_cooldown( timespan_t::zero() );

  buff.slaughtering_strikes_an = make_buff( this, "slaughtering_strikes_an", find_spell( 393943 ) )
    ->set_default_value( find_spell( 393943 )->effectN( 1 ).percent() );

  buff.slaughtering_strikes_rb = make_buff( this, "slaughtering_strikes_rb", find_spell( 393931 ) )
    ->set_default_value( find_spell( 393931 )->effectN( 1 ).percent() );

  const spell_data_t* test_of_might_tracker = talents.arms.test_of_might.spell()->effectN( 1 ).trigger()->effectN( 1 ).trigger();
  buff.test_of_might_tracker = new test_of_might_t( *this, "test_of_might_tracker", test_of_might_tracker );
  buff.test_of_might_tracker->set_duration( spell.colossus_smash_debuff->duration() + talents.arms.spiteful_serenity->effectN( 8 ).time_value() 
                                            + talents.arms.blunt_instruments->effectN( 2 ).time_value() );
  buff.test_of_might = make_buff( this, "test_of_might", find_spell( 385013 ) )
                              ->set_default_value( talents.arms.test_of_might->effectN( 1 ).percent() )
                              ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
                              ->set_trigger_spell( test_of_might_tracker );

  buff.bloodcraze = make_buff( this, "bloodcraze", find_spell( 393951 ) )
    ->set_default_value( talents.fury.bloodcraze->effectN( 1 ).percent() );

  const spell_data_t* hurricane_trigger = find_spell( 390577 );
  const spell_data_t* hurricane_buff   = find_spell( 390581 );
  buff.hurricane_driver =
      make_buff( this, "hurricane_driver", hurricane_trigger )
          ->set_quiet( true )
          ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
          ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { buff.hurricane->trigger(); } );

  buff.hurricane = make_buff( this, "hurricane", hurricane_buff )
    ->set_trigger_spell( hurricane_trigger )
    ->set_default_value( talents.shared.hurricane->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_STRENGTH );

  //buff.vengeance_ignore_pain = make_buff( this, "vengeance_ignore_pain", find_spell( 202574 ) )
    //->set_chance( talents.vengeance->ok() )
    //->set_default_value( find_spell( 202574 )->effectN( 1 ).percent() );

  //buff.vengeance_revenge = make_buff( this, "vengeance_revenge", find_spell( 202573 ) )
    //->set_chance( talents.vengeance->ok() )
    //->set_default_value( find_spell( 202573 )->effectN( 1 ).percent() );

  buff.sephuzs_secret = new buffs::sephuzs_secret_buff_t( this );

  buff.in_for_the_kill = new in_for_the_kill_t( *this, "in_for_the_kill", find_spell( 248622 ) );

  buff.whirlwind = make_buff( this, "whirlwind", find_spell( 85739 ) );

  buff.dance_of_death_prot = make_buff( this, "dance_of_death_prot", talents.protection.dance_of_death -> effectN( 1 ).trigger() )
                                          -> set_default_value( talents.protection.dance_of_death -> effectN( 1 ).trigger() -> effectN( 2 ).percent() );

  buff.seeing_red = make_buff( this, "seeing_red", find_spell( 386486 ) );

  buff.seeing_red_tracking =
      make_buff( this, "seeing_red_tracking", find_spell( 386477 ) )
          ->set_quiet( true )
          ->set_duration( timespan_t::zero() )
          ->set_max_stack( 100 )
          ->set_default_value( 0 );

  buff.violent_outburst = make_buff( this, "violent_outburst", find_spell( 386478 ) );

  buff.brace_for_impact = make_buff( this, "brace_for_impact", talents.protection.brace_for_impact->effectN( 1 ).trigger() )
                         -> set_default_value( talents.protection.brace_for_impact->effectN( 1 ).trigger()->effectN( 1 ).percent() )
                         -> set_initial_stack( 1 );

  // Azerite
  const spell_data_t* crushing_assault_trigger = azerite.crushing_assault.spell()->effectN( 1 ).trigger();
  const spell_data_t* crushing_assault_buff    = crushing_assault_trigger->effectN( 1 ).trigger();
  buff.crushing_assault                        = make_buff( this, "crushing_assault", crushing_assault_buff )
                              ->set_default_value( azerite.crushing_assault.value( 1 ) )
                              ->set_trigger_spell( crushing_assault_trigger );

  const spell_data_t* gathering_storm_trigger = azerite.gathering_storm.spell()->effectN( 1 ).trigger();
  //const spell_data_t* gathering_storm_buff    = gathering_storm_trigger->effectN( 1 ).trigger();
  buff.gathering_storm                        = make_buff<stat_buff_t>( this, "gathering_storm", find_spell( 273415 ) )
                           ->add_stat( STAT_STRENGTH, azerite.gathering_storm.value( 1 ) )
                           ->set_trigger_spell( gathering_storm_trigger );

  const spell_data_t* infinite_fury_trigger = azerite.infinite_fury.spell()->effectN( 1 ).trigger();
  const spell_data_t* infinite_fury_buff    = infinite_fury_trigger ->effectN( 1 ).trigger();
  buff.infinite_fury = make_buff( this, "infinite_fury", infinite_fury_buff    )
                               ->set_trigger_spell( infinite_fury_trigger  )
                               ->set_default_value( azerite.infinite_fury.value() )
                               ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.pulverizing_blows = make_buff( this, "pulverizing_blows", find_spell( 275672 ) )
                               ->set_trigger_spell( azerite.pulverizing_blows.spell_ref().effectN( 1 ).trigger() )
                               ->set_default_value( azerite.pulverizing_blows.value() );

  const spell_data_t* trample_the_weak_trigger = azerite.trample_the_weak.spell()->effectN( 1 ).trigger();
  const spell_data_t* trample_the_weak_buff    = trample_the_weak_trigger->effectN( 1 ).trigger();
  buff.trample_the_weak = make_buff<stat_buff_t>( this, "trample_the_weak", trample_the_weak_buff )
                              ->add_stat( STAT_STRENGTH, azerite.trample_the_weak.value( 1 ) )
                              ->add_stat( STAT_STAMINA, azerite.trample_the_weak.value( 1 ) )
                              ->set_trigger_spell( trample_the_weak_trigger );
  buff.bloodsport = make_buff<stat_buff_t>( this, "bloodsport", azerite.bloodsport.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
                   -> add_stat( STAT_LEECH_RATING, azerite.bloodsport.value( 2 ) );
  buff.brace_for_impact_az = make_buff( this, "brace_for_impact_az", azerite.brace_for_impact.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
                         -> set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                         -> set_default_value( azerite.brace_for_impact.value( 1 ) );
  buff.striking_the_anvil = make_buff( this, "striking_the_anvil", find_spell( 288452 ) )
                               ->set_trigger_spell( azerite.striking_the_anvil.spell_ref().effectN( 1 ).trigger() )
                               ->set_default_value( azerite.striking_the_anvil.value() );

  const spell_data_t* bastion_of_might_trigger = azerite.bastion_of_might.spell()->effectN( 1 ).trigger();
  const spell_data_t* bastion_of_might_buff = bastion_of_might_trigger->effectN( 1 ).trigger();
  buff.bastion_of_might = make_buff<stat_buff_t>( this, "bastion_of_might", bastion_of_might_buff)
                               ->add_stat( STAT_MASTERY_RATING, azerite.bastion_of_might.value( 1 ) )
                               ->set_trigger_spell( bastion_of_might_trigger );
  // Vision of Perfection procs a smaller mastery buff with a shorter duration
  buff.bastion_of_might_vop = make_buff<stat_buff_t>( this, "bastion_of_might_vop", bastion_of_might_buff )
    -> add_stat( STAT_MASTERY_RATING, azerite.bastion_of_might.value( 1 ) * azerite.vision_of_perfection_percentage )
    -> set_duration( bastion_of_might_buff -> duration() * azerite.vision_of_perfection_percentage );

  // Covenant Abilities====================================================================================================

  buff.conquerors_banner = make_buff( this, "conquerors_banner", covenant.conquerors_banner )
    ->set_default_value_from_effect_type( A_PERIODIC_ENERGIZE )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      resource_gain( RESOURCE_RAGE, (specialization() == WARRIOR_FURY ? 6 : 4), gain.conquerors_banner );
    } );

  buff.conquerors_frenzy    = make_buff( this, "conquerors_frenzy", find_spell( 325862 ) )
                               ->set_default_value( find_spell( 325862 )->effectN( 2 ).percent() )
                               ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.conquerors_mastery = make_buff<stat_buff_t>( this, "conquerors_mastery", find_spell( 325862 ) );

  buff.elysian_might_legendary = make_buff( this, "elysian_might_legendary", find_spell( 311193 ) )
                         ->set_default_value( find_spell( 311193 )->effectN( 1 ).percent() );

  // Conduits===============================================================================================================

  buff.ashen_juggernaut_conduit = make_buff( this, "ashen_juggernaut_conduit", conduit.ashen_juggernaut->effectN( 1 ).trigger() )
                           ->set_default_value( ( (player_t*)this )->covenant->id() == (unsigned int)covenant_e::VENTHYR
                             ? conduit.ashen_juggernaut.percent() * (1 + covenant.condemn_driver ->effectN( 7 ).percent())
                             :  conduit.ashen_juggernaut.percent());

  buff.merciless_bonegrinder_conduit = make_buff( this, "merciless_bonegrinder_conduit", find_spell( 335260 ) )
                                ->set_default_value( conduit.merciless_bonegrinder.percent() );

  buff.veterans_repute = make_buff( this, "veterans_repute", conduit.veterans_repute )
                          ->add_invalidate( CACHE_STRENGTH )
                          ->set_default_value( conduit.veterans_repute.percent() )
                          ->set_duration( covenant.conquerors_banner->duration() );

  buff.show_of_force = make_buff( this, "show_of_force", talents.protection.show_of_force -> effectN( 1 ).trigger() )
                           ->set_default_value( talents.protection.show_of_force -> effectN( 1 ).percent() );

  // Arma: 2022 Nov 4.  Unnerving focus seems to get the value from the parent, not the value set in the buff
  buff.unnerving_focus = make_buff( this, "unnerving_focus", talents.protection.unnerving_focus -> effectN( 1 ).trigger() )
                           ->set_default_value( talents.protection.unnerving_focus -> effectN( 1 ).percent() );
  // Runeforged Legendary Powers============================================================================================

  buff.battlelord = make_buff( this, "battlelord", find_spell( 386631 ) );

  buff.cadence_of_fujieda = make_buff( this, "cadence_of_fujieda", find_spell( 335558 ) )
                           ->set_default_value( find_spell( 335558 )->effectN( 1 ).percent() )
                           ->add_invalidate( CACHE_ATTACK_HASTE );

  buff.will_of_the_berserker = make_buff( this, "will_of_the_berserker", find_spell( 335597 ) )
                               ->set_default_value( find_spell( 335597 )->effectN( 1 ).percent() )
                               ->add_invalidate( CACHE_CRIT_CHANCE );

  // T29 Tier Effects ===============================================================================================================

  buff.strike_vulnerabilities = make_buff( this, "strike_vulnerabilities", tier_set.t29_arms_4pc->ok() ?
                                           find_spell( 394173 ) : spell_data_t::not_found() )
                               ->set_default_value( find_spell( 394173 )->effectN( 1 ).percent() )
                               ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.vanguards_determination = make_buff( this, "vanguards_determination", tier_set.t29_prot_2pc->ok() ?
                                            find_spell( 394056 ) : spell_data_t::not_found() )
                                    ->set_default_value( find_spell( 394056 )->effectN( 1 ).percent());

  // T30 Tier Effects ===============================================================================================================
  buff.crushing_advance = make_buff( this, "crushing_advance", tier_set.t30_arms_4pc->ok() ?
                               find_spell( 410138 ) : spell_data_t::not_found() )
                          ->set_default_value( find_spell( 410138 )->effectN( 1 ).percent() );

  buff.merciless_assault = make_buff( this, "merciless_assault", tier_set.t30_fury_4pc->ok() ? 
                                find_spell( 409983 ) : spell_data_t::not_found() )
                           ->set_default_value( find_spell( 409983 )->effectN( 2 ).percent() )
                           ->set_duration( find_spell( 409983 )->duration() );

  buff.earthen_tenacity = make_buff( this, "earthen_tenacity", tier_set.t30_prot_4pc -> ok() ?
                                find_spell( 410218 ) : spell_data_t::not_found() );
}
// warrior_t::init_rng ==================================================
void warrior_t::init_rng()
{
  player_t::init_rng();
  rppm.fatal_mark       = get_rppm( "fatal_mark", talents.arms.fatality );
  rppm.revenge        = get_rppm( "revenge_trigger", spec.revenge_trigger );
}
// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  player_t::init_scaling();

  if ( specialization() == WARRIOR_FURY )
  {
    scaling->enable( STAT_WEAPON_OFFHAND_DPS );
  }

  if ( specialization() == WARRIOR_PROTECTION )
  {
    scaling->enable( STAT_BONUS_ARMOR );
  }

  scaling->disable( STAT_AGILITY );
}

// warrior_t::init_gains ====================================================

void warrior_t::init_gains()
{
  player_t::init_gains();

  gain.archavons_heavy_hand             = get_gain( "archavons_heavy_hand" );
  gain.avatar                           = get_gain( "avatar" );
  gain.avatar_torment                   = get_gain( "avatar_torment" );
  gain.avoided_attacks                  = get_gain( "avoided_attacks" );
  gain.bloodsurge                       = get_gain( "bloodsurge" );
  gain.charge                           = get_gain( "charge" );
  gain.conquerors_banner                = get_gain( "conquerors_banner" );
  gain.critical_block                   = get_gain( "critical_block" );
  gain.execute                          = get_gain( "execute" );
  gain.frothing_berserker               = get_gain( "frothing_berserker" );
  gain.melee_crit                       = get_gain( "melee_crit" );
  gain.melee_main_hand                  = get_gain( "melee_main_hand" );
  gain.melee_off_hand                   = get_gain( "melee_off_hand" );
  gain.revenge                          = get_gain( "revenge" );
  gain.shield_charge                    = get_gain( "shield_charge" );
  gain.shield_slam                      = get_gain( "shield_slam" );
  gain.spear_of_bastion                 = get_gain( "spear_of_bastion" );
  gain.strength_of_arms                 = get_gain( "strength_of_arms" );
  gain.booming_voice                    = get_gain( "booming_voice" );
  gain.thunder_clap                     = get_gain( "thunder_clap" );
  gain.whirlwind                        = get_gain( "whirlwind" );
  gain.collateral_damage                = get_gain( "collateral_damage" );
  gain.instigate                        = get_gain( "instigate" );
  gain.war_machine_demise               = get_gain( "war_machine_demise" );

  gain.ceannar_rage           = get_gain( "ceannar_rage" );
  gain.cold_steel_hot_blood   = get_gain( "cold_steel_hot_blood" );
  gain.endless_rage           = get_gain( "endless_rage" );
  gain.lord_of_war            = get_gain( "lord_of_war" );
  gain.meat_cleaver           = get_gain( "meat_cleaver" );
  gain.valarjar_berserking    = get_gain( "valarjar_berserking" );
  gain.ravager                = get_gain( "ravager" );
  gain.rage_from_damage_taken = get_gain( "rage_from_damage_taken" );
  gain.simmering_rage         = get_gain( "simmering_rage" );
  gain.execute_refund         = get_gain( "execute_refund" );

  // Azerite
  gain.memory_of_lucid_dreams = get_gain( "memory_of_lucid_dreams_proc" );
  gain.merciless_assault      = get_gain( "merciless_assault" );
}

// warrior_t::init_position ====================================================

void warrior_t::init_position()
{
  player_t::init_position();
}

// warrior_t::init_procs ======================================================

void warrior_t::init_procs()
{
  player_t::init_procs();
  proc.delayed_auto_attack = get_proc( "delayed_auto_attack" );
  proc.glory               = get_proc( "glory" );
  proc.tactician           = get_proc( "tactician" );
}

// warrior_t::init_resources ================================================

void warrior_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ] = 0;  // By default, simc sets all resources to full. However, Warriors cannot
                                           // reliably start combat with more than 0 rage. This will also ensure that
                                           // the 20-40 rage from Charge is not overwritten.
}

// warrior_t::default_potion ================================================

std::string warrior_t::default_potion() const
{
  std::string fury_pot =
      ( true_level > 60 )
          ? "elemental_potion_of_ultimate_power_3"
          : ( true_level > 50 )
                ? "spectral_strength"
                : "disabled";

  std::string arms_pot =
      ( true_level > 60 )
          ? "elemental_potion_of_ultimate_power_3"
          : ( true_level > 50 )
                ? "spectral_strength"
                : "disabled";

  std::string protection_pot =
      ( true_level > 60 )
          ? "elemental_potion_of_ultimate_power_3"
          : ( true_level > 50 )
                ? "spectral_strength"
                : "disabled";

  switch ( specialization() )
  {
    case WARRIOR_FURY:
      return fury_pot;
    case WARRIOR_ARMS:
      return arms_pot;
    case WARRIOR_PROTECTION:
      return protection_pot;
    default:
      return "disabled";
  }
}

// warrior_t::default_flask =================================================

std::string warrior_t::default_flask() const
{
  if ( specialization() == WARRIOR_PROTECTION && true_level > 60 )
    return "phial_of_static_empowerment_3";

  return ( true_level > 60 )
             ? "phial_of_tepid_versatility_3"
             : ( true_level > 50 )
                   ? "spectral_flask_of_power"
                   : "disabled";
}

// warrior_t::default_food ==================================================

std::string warrior_t::default_food() const
{
  std::string fury_food = ( true_level > 60 )
                              ? "fated_fortune_cookie"
                              : ( true_level > 50 )
                                    ? "feast_of_gluttonous_hedonism"
                                    : "disabled";

  std::string arms_food = ( true_level > 60 )
                              ? "fated_fortune_cookie"
                              : ( true_level > 50 )
                                    ? "feast_of_gluttonous_hedonism"
                                    : "disabled";

  std::string protection_food = ( true_level > 60 )
                              ? "fated_fortune_cookie"
                              : ( true_level > 50 )
                                    ? "feast_of_gluttonous_hedonism"
                                    : "disabled";

  switch ( specialization() )
  {
    case WARRIOR_FURY:
      return fury_food;
    case WARRIOR_ARMS:
      return arms_food;
    case WARRIOR_PROTECTION:
      return protection_food;
    default:
      return "disabled";
  }
}

// warrior_t::default_rune ==================================================

std::string warrior_t::default_rune() const
{
  return ( true_level >= 60 ) ? "draconic"
                               : ( true_level >= 50 ) ? "veiled" : "disabled";
}

// warrior_t::default_temporary_enchant =====================================

std::string warrior_t::default_temporary_enchant() const
{
  std::string fury_temporary_enchant = ( true_level >= 60 )
                              ? "main_hand:hissing_rune_3/off_hand:hissing_rune_3"
                              : "disabled";

  std::string arms_temporary_enchant = ( true_level >= 60 )
                              ? "main_hand:howling_rune_3"
                              : "disabled";

  std::string protection_temporary_enchant = ( true_level >= 60 )
                              ? "main_hand:howling_rune_3"
                              : "disabled";
  switch ( specialization() )
  {
    case WARRIOR_FURY:
      return fury_temporary_enchant;
    case WARRIOR_ARMS:
      return arms_temporary_enchant;
    case WARRIOR_PROTECTION:
      return protection_temporary_enchant;
    default:
      return "disabled";
  }
}

// warrior_t::init_actions ==================================================

void warrior_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
    {
      sim->errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    }

    quiet = true;
    return;
  }

  clear_action_priority_lists();

  switch ( specialization() )
  {
    case WARRIOR_FURY:
      warrior_apl::fury( this );
      break;
    case WARRIOR_ARMS:
      warrior_apl::arms( this );
      break;
    case WARRIOR_PROTECTION:
      warrior_apl::protection( this );
      break;
    default:
      apl_default();  // DEFAULT
      break;
  }

  // Default
  use_default_action_list = true;
  player_t::init_action_list();
}

// warrior_t::arise() ======================================================

void warrior_t::arise()
{
  player_t::arise();
}

// warrior_t::combat_begin ==================================================

void warrior_t::combat_begin()
{
  if ( !sim->fixed_time )
  {
    if ( warrior_fixed_time )
    {
      for ( auto* p : sim->player_list )
      {
         if ( p->specialization() != WARRIOR_FURY && p->specialization() != WARRIOR_ARMS )
        {
          warrior_fixed_time = false;
          break;
        }
      }
      if ( warrior_fixed_time )
      {
        sim->fixed_time = true;
        sim->error(
            "To fix issues with the target exploding <20% range due to execute, fixed_time=1 has been enabled. This "
            "gives similar results" );
        sim->error(
            "to execute's usage in a raid sim, without taking an eternity to simulate. To disable this option, add "
            "warrior_fixed_time=0 to your sim." );
      }
    }
  }
  player_t::combat_begin();
  buff.into_the_fray -> trigger( into_the_fray_friends < 0 ? buff.into_the_fray -> max_stack() : into_the_fray_friends + 1 );
}

// Into the fray

struct into_the_fray_callback_t
{
  warrior_t* w;
  double fray_distance;
  into_the_fray_callback_t( warrior_t* p ) : w( p ), fray_distance( 0 )
  {
    fray_distance = p->talents.protection.into_the_fray->effectN( 1 ).base_value();
  }

  void operator()( player_t* )
  {
    size_t i            = w->sim->target_non_sleeping_list.size();
    size_t buff_stacks_ = w->into_the_fray_friends;
    while ( i > 0 && buff_stacks_ < w->buff.into_the_fray->data().max_stacks() )
    {
      i--;
      player_t* target_ = w->sim->target_non_sleeping_list[ i ];
      if ( w->get_player_distance( *target_ ) <= fray_distance )
      {
        buff_stacks_++;
      }
    }
    if ( w->buff.into_the_fray->current_stack != as<int>( buff_stacks_ ) )
    {
      w->buff.into_the_fray->expire();
      w->buff.into_the_fray->trigger( static_cast<int>( buff_stacks_ ) );
    }
  }
};

// warrior_t::create_actions ================================================

void warrior_t::activate()
{
  player_t::activate();

  // If the value is equal to -1, don't use the callback and just assume max stacks all the time
  if ( talents.protection.into_the_fray->ok() && into_the_fray_friends >= 0 )
  {
    sim->target_non_sleeping_list.register_callback( into_the_fray_callback_t( this ) );
  }
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  player_t::reset();

  rampage_driver = nullptr;
  legendary.glory_counter  = 0;
}

// Movement related overrides. =============================================

void warrior_t::moving()
{
}

void warrior_t::interrupt()
{
  buff.charge_movement->expire();
  buff.heroic_leap_movement->expire();
  buff.intervene_movement->expire();
  buff.intercept_movement->expire();
  buff.shield_charge_movement->expire();

  player_t::interrupt();
}

void warrior_t::teleport( double, timespan_t )
{
  // All movement "teleports" are modeled.
}

void warrior_t::trigger_movement( double distance, movement_direction_type direction )
{
  if ( talents.protection.into_the_fray->ok() && into_the_fray_friends >= 0 )
  {
    into_the_fray_callback_t( this );
  }

  else
  {
    player_t::trigger_movement( distance, direction );
  }
}

// warrior_t::composite_player_multiplier ===================================

double warrior_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buff.tornados_eye->check() )
  {
    m *= 1.0 + ( buff.tornados_eye->current_stack * buff.tornados_eye->data().effectN( 2 ).percent() );
  }

  if ( buff.defensive_stance->check() )
  {
    m *= 1.0 + talents.warrior.defensive_stance->effectN( 2 ).percent();
  }

  m *= 1.0 + buff.fujiedas_fury->check_stack_value();

  return m;
}

// rogue_t::composite_player_target_crit_chance =============================

double warrior_t::composite_player_target_crit_chance( player_t* target ) const
{
  double c = player_t::composite_player_target_crit_chance( target );

  auto td = get_target_data( target );

  // crit chance bonus is not currently whitelisted in data
  if ( sets->has_set_bonus( WARRIOR_ARMS, T30, B2 ) && td->dots_deep_wounds->is_ticking() )
  c += find_spell( 262115 )->effectN( 4 ).percent();

  return c;
}

// warrior_t::composite_attack_speed ===========================================

double warrior_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  if ( talents.warrior.furious_blows->ok() )
  {
    s *= 1.0 / ( 1.0 + talents.warrior.furious_blows->effectN( 1 ).percent() );
  }

  s *= 1.0 / ( 1.0 + buff.wild_strikes->check_value() );

  s *= 1.0 / ( 1.0 + buff.dancing_blades->check_value() );

  s *= 1.0 / ( 1.0 + buff.battering_ram->check_value() );

  return s;
}

// warrior_t::composite_melee_haste ===========================================

double warrior_t::composite_melee_haste() const
{
  double a = player_t::composite_melee_haste();

  if ( talents.fury.improved_enrage->ok() )
  {
  a *= 1.0 / ( 1.0 + buff.enrage->check_value() );
  }
  a *= 1.0 / ( 1.0 + buff.frenzy->check_stack_value() );

  a *= 1.0 / ( 1.0 + buff.cadence_of_fujieda->check_value() );

  a *= 1.0 / ( 1.0 + buff.into_the_fray->check_stack_value() );

  a *= 1.0 / ( 1.0 + buff.sephuzs_secret->check_value() );

  a *= 1.0 / ( 1.0 + buff.in_for_the_kill->check_value() );

  a *= 1.0 / ( 1.0 + talents.warrior.wild_strikes->effectN( 1 ).percent() );

  a *= 1.0 / ( 1.0 + talents.fury.swift_strikes->effectN( 1 ).percent() );

  a *= 1.0 / ( 1.0 + talents.protection.enduring_alacrity->effectN( 2 ).percent());

  return a;
}

// warrior_t::composite_melee_expertise =====================================

double warrior_t::composite_melee_expertise( const weapon_t* ) const
{
  double e = player_t::composite_melee_expertise();

  e += spec.protection_warrior->effectN( 7 ).percent();

  return e;
}

// warrior_t::composite_mastery =============================================

double warrior_t::composite_mastery() const
{
  double y = player_t::composite_mastery();

  if ( specialization() == WARRIOR_ARMS )
  {
    y += talents.arms.deft_experience->effectN( 1 ).base_value();
  }
  else
  {
    y += talents.fury.deft_experience->effectN( 1 ).base_value();
  }

  return y;
}

// warrior_t::composite_damage_versatility =============================================

double warrior_t::composite_damage_versatility() const
{
  double cdv = player_t::composite_damage_versatility();

  cdv += talents.arms.valor_in_victory->effectN( 1 ).percent();

  return cdv;
}

// warrior_t::composite_attribute ================================

double warrior_t::composite_attribute( attribute_e attr ) const
{
  double p = player_t::composite_attribute( attr );

  if ( attr == ATTR_STRENGTH )
  {
    // Arma 2022 Nov 13 Note that unless we handle str manually in the armor calcs Attt for prot ends up looping here.  
    // As Str and armor are both looping in the stat cache
    // As we have it implemented properly in the armor calcs, this can be globally enabled.
    // get_attribute -> composite_attribute -> bonus_armor -> composite_bonus_armor -> strength -> get_attribute
    //if ( specialization() != WARRIOR_PROTECTION )
    p += ( talents.warrior.armored_to_the_teeth->effectN( 2 ).percent() * cache.armor() );
  }

  return p;
}

// warrior_t::composite_attribute_multiplier ================================

double warrior_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + buff.veterans_repute->value();
    m *= 1.0 + buff.hurricane->check_stack_value();
    m *= 1.0 + talents.protection.focused_vigor->effectN( 1 ).percent();
  }

  // Protection has increased stamina from vanguard
  if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + spec.vanguard -> effectN( 2 ).percent();
    m *= 1.0 + talents.protection.enduring_alacrity -> effectN( 1 ).percent();
    m *= 1.0 + talents.warrior.endurance_training -> effectN( 1 ).percent();
  }

  return m;
}

// warrior_t::composite_rating_multiplier ===================================

double warrior_t::composite_rating_multiplier( rating_e rating ) const
{
  return player_t::composite_rating_multiplier( rating );
}

// warrior_t::matching_gear_multiplier ======================================

double warrior_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( ( attr == ATTR_STRENGTH ) && ( specialization() != WARRIOR_PROTECTION ) )
  {
    return 0.05;
  }

  if ( ( attr == ATTR_STAMINA ) && ( specialization() == WARRIOR_PROTECTION ) )
  {
    return 0.05;
  }

  return 0.0;
}

// warrior_t::composite_armor_multiplier ==========================================

double warrior_t::composite_armor_multiplier() const
{
  double ar = player_t::composite_armor_multiplier();

  // Arma 2022 Nov 10.  To avoid an infinite loop, we manually calculate the str benefit of armored to the teeth here, and apply the armor we would gain from it
  if ( talents.warrior.armored_to_the_teeth->ok() && specialization() == WARRIOR_PROTECTION )
  {
    auto dividend = spec.vanguard -> effectN( 1 ).percent() * talents.warrior.armored_to_the_teeth -> effectN( 2 ).percent() * (1 + talents.warrior.reinforced_plates->effectN( 1 ).percent()) * ( 1+talents.protection.focused_vigor->effectN( 3 ).percent());
    auto divisor = 1 - (spec.vanguard -> effectN( 1 ).percent() * talents.warrior.armored_to_the_teeth -> effectN( 2 ).percent() * (1 + talents.warrior.reinforced_plates->effectN( 1 ).percent()) * ( 1+talents.protection.focused_vigor->effectN( 3 ).percent()));
    ar *= 1 + (dividend / divisor);
  }

 // Generally Modify Armor% (101)

  ar *= 1.0 + talents.warrior.reinforced_plates->effectN( 1 ).percent();
  ar *= 1.0 + talents.protection.enduring_alacrity -> effectN( 3 ).percent();
  ar *= 1.0 + talents.protection.focused_vigor -> effectN( 3 ).percent();

  return ar;
}

// warrior_t::composite_bonus_armor ==========================================

double warrior_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  // Arma 2022 Nov 10.
  // We have to use base strength here, and then multiply by all the various STR buffs, while avoiding multiplying by Armored to the teeth.
  // This is to prevent an infinite loop between Attt using the cache.armor() and composite_bonus_armor using cache.strength()
  // We have to add all str buffs to this, if we want to avoid vanguard missing anything.  Not ideal.

  if ( specialization() == WARRIOR_PROTECTION )
  {
    // Pulls strength using the base player_t functions.  We call these directly to avoid using the warrior_t versions, as armor contribution from 
    // attt is calculated during composite_armor_multiplier
    auto current_str = util::floor( player_t::composite_attribute( ATTR_STRENGTH ) * player_t::composite_attribute_multiplier( ATTR_STRENGTH ) );
    // if there is anything else in warrior_t::composite_attribute_multiplier that applies to str, like focused_vigor for instance
    // it needs to be added here as well
    ba += spec.vanguard -> effectN( 1 ).percent() * current_str * (1.0 + talents.protection.focused_vigor->effectN( 1 ).percent());
  }

  // If in the future if blizz changes behavior, and we want to go back to using the two caches, we can use the below code
  //if( specialization() == WARRIOR_PROTECTION )
  //  ba += spec.vanguard -> effectN( 1 ).percent() * cache.strength();

  return ba;
}

// warrior_t::composite_base_armor_multiplier ================================
double warrior_t::composite_base_armor_multiplier() const
{
  // Generally Modify Base Resistance (142)
  double a = player_t::composite_base_armor_multiplier();

  return a;
}

// warrior_t::composite_block ================================================

double warrior_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * mastery.critical_block->effectN( 2 ).mastery_value();
  double b                   = player_t::composite_block_dr( block_subject_to_dr );

  // shield block adds 100% block chance
  if ( buff.shield_block -> up() )
  {
    b += spell.shield_block_buff -> effectN( 1 ).percent();
  }

  // Not affected by DR
  if ( talents.protection.shield_specialization->ok() )
    b += talents.protection.shield_specialization->effectN( 1 ).percent();

  return b;
}

// warrior_t::composite_block_reduction =====================================

double warrior_t::composite_block_reduction( action_state_t* s ) const
{
  double br = player_t::composite_block_reduction( s );

  if ( buff.brace_for_impact_az -> check() )
  {
    br += buff.brace_for_impact_az -> check_stack_value();
  }

  if ( buff.brace_for_impact -> check() )
  {
    br *= 1.0 + buff.brace_for_impact -> check() * talents.protection.brace_for_impact -> effectN( 2 ).percent();
  }

  if ( azerite.iron_fortress.enabled() )
  {
    br += azerite.iron_fortress.value( 2 );
  }

  if ( talents.protection.shield_specialization->ok() )
  {
      br += talents.protection.shield_specialization->effectN( 2 ).percent();
  }

  return br;
}

// warrior_t::composite_parry_rating() ========================================

double warrior_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // TODO: remove the spec check once riposte is pulled from spelldata
  if ( spec.riposte -> ok() || specialization() == WARRIOR_PROTECTION )
  {
    p += composite_melee_crit_rating();
  }
  return p;
}

// warrior_t::composite_parry =================================================

double warrior_t::composite_parry() const
{
  double parry = player_t::composite_parry();

  if ( buff.die_by_the_sword->check() )
  {
    parry += talents.arms.die_by_the_sword->effectN( 1 ).percent();
  }
  return parry;
}

// warrior_t::composite_attack_power_multiplier ==============================

double warrior_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.critical_block -> ok() )
  {
    ap *= 1.0 + mastery.critical_block -> effectN( 5 ).mastery_value() * cache.mastery();
  }
  return ap;
}

// warrior_t::composite_crit_block =====================================

double warrior_t::composite_crit_block() const
{

  double b = player_t::composite_crit_block();

  if ( mastery.critical_block->ok() )
  {
    b += cache.mastery() * mastery.critical_block->effectN( 1 ).mastery_value();
  }

  return b;
}

// warrior_t::composite_crit_avoidance ===========================================

double warrior_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();
  c += spec.protection_warrior->effectN( 7 ).percent();
  return c;
}

// warrior_t::composite_melee_speed ========================================
/*
double warrior_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  return s;
}
*/

// warrior_t::composite_melee_crit_chance =========================================

double warrior_t::composite_melee_crit_chance() const
{
  double c = player_t::composite_melee_crit_chance();

  //c += buff.recklessness->check_value(); removed in favor of composite_crit_chance (line 1015)
  c += buff.will_of_the_berserker->check_value();
  c += buff.conquerors_frenzy->check_value();
  c += talents.warrior.cruel_strikes->effectN( 1 ).percent();
  c += buff.battle_stance->check_value();
  c += buff.strike_vulnerabilities->check_value();

  if ( specialization() == WARRIOR_ARMS )
  {
    c += talents.arms.critical_thinking->effectN( 1 ).percent();
  }
  else if ( specialization() == WARRIOR_FURY )
  {
    c += talents.fury.critical_thinking->effectN( 1 ).percent();
  }

  c += talents.protection.focused_vigor->effectN( 2 ).percent();

  return c;
}

// warrior_t::composite_melee_crit_rating =========================================

double warrior_t::composite_melee_crit_rating() const
{
  double c = player_t::composite_melee_crit_rating();

  c += buff.infinite_fury->check_value();

  return c;
}

// warrior_t::composite_player_critical_damage_multiplier ==================

double warrior_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double cdm = player_t::composite_player_critical_damage_multiplier( s );

  cdm *= 1.0 + buff.elysian_might_legendary->check_value();

  cdm *= 1.0 + buff.elysian_might->check_value();

  return cdm;
}

// warrior_t::composite_spell_crit_chance =========================================
/*
double warrior_t::composite_spell_crit_chance() const
{
  return composite_melee_crit_chance();
}
*/

// warrior_t::composite_leech ==============================================

double warrior_t::composite_leech() const
{
  double m = player_t::composite_leech();

  m += talents.warrior.leeching_strikes->effectN( 1 ).percent();

  return m;
}


// warrior_t::resource_gain =================================================

double warrior_t::resource_gain( resource_e r, double a, gain_t* g, action_t* action )
{
  if ( buff.recklessness->check() && r == RESOURCE_RAGE )
  {
    bool do_not_double_rage = false;

    do_not_double_rage      = ( g == gain.ceannar_rage || g == gain.valarjar_berserking || g == gain.simmering_rage || 
                                  g == gain.memory_of_lucid_dreams || g == gain.frothing_berserker );

    if ( !do_not_double_rage )  // FIXME: remove this horror after BFA launches, keep Simmering Rage
      a *= 1.0 + spell.recklessness_buff->effectN( 4 ).percent();
  }
  // Memory of Lucid Dreams
  if ( buffs.memory_of_lucid_dreams->up() )
  {
    a *= 1.0 + buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();
  }

  if ( buff.unnerving_focus->up() )
  {
    a *= 1.0 + buff.unnerving_focus->stack_value();//Spell data lists all the abilities it provides rage gain to separately - currently it is all of our abilities.
  }
  return player_t::resource_gain( r, a, g, action );
}

// warrior_t::temporary_movement_modifier ==================================

double warrior_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  // These are ordered in the highest speed movement increase to the lowest, there's no reason to check the rest as they
  // will just be overridden. Also gives correct benefit numbers.
  if ( buff.heroic_leap_movement->up() )
  {
    temporary = std::max( buff.heroic_leap_movement->value(), temporary );
  }
  else if ( buff.charge_movement->up() )
  {
    temporary = std::max( buff.charge_movement->value(), temporary );
  }
  else if ( buff.intervene_movement->up() )
  {
    temporary = std::max( buff.intervene_movement->value(), temporary );
  }
  else if ( buff.intercept_movement->up() )
  {
    temporary = std::max( buff.intercept_movement->value(), temporary );
  }
  else if ( buff.shield_charge_movement->up() )
  {
    temporary = std::max( buff.shield_charge_movement->value(), temporary );
  }
  else if ( buff.bounding_stride->up() )
  {
    temporary = std::max( buff.bounding_stride->value(), temporary );
  }
  else if ( buff.tornados_eye->up() )
  {
    temporary =
        std::max( buff.tornados_eye->current_stack * buff.tornados_eye->data().effectN( 1 ).percent(), temporary );
  }
  return temporary;
}

// warrior_t::invalidate_cache ==============================================

void warrior_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( mastery.critical_block->ok() )
  {
    if ( c == CACHE_MASTERY )
    {
      player_t::invalidate_cache( CACHE_BLOCK );
      player_t::invalidate_cache( CACHE_CRIT_BLOCK );
      player_t::invalidate_cache( CACHE_ATTACK_POWER );
    }
    if ( c == CACHE_CRIT_CHANCE )
    {
      player_t::invalidate_cache( CACHE_PARRY );
    }
  }
  if ( c == CACHE_MASTERY && mastery.unshackled_fury->ok() )
  {
    player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
  if ( c == CACHE_STRENGTH && spec.vanguard -> ok() )
  {
    player_t::invalidate_cache( CACHE_BONUS_ARMOR );
  }
  if ( c == CACHE_ARMOR && talents.warrior.armored_to_the_teeth->ok() && spec.vanguard->ok() )
  {
    player_t::invalidate_cache( CACHE_STRENGTH );
  }
  if ( c == CACHE_BONUS_ARMOR && talents.warrior.armored_to_the_teeth->ok() && spec.vanguard->ok() )
  {
    player_t::invalidate_cache( CACHE_STRENGTH );
  }
}

// warrior_t::primary_role() ================================================

role_e warrior_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( specialization() == WARRIOR_PROTECTION )
  {
    return ROLE_TANK;
  }
  return ROLE_ATTACK;
}

// warrior_t::vision_of_perfection_proc ================================

void warrior_t::vision_of_perfection_proc()
{
  switch ( specialization() )
  {
    case WARRIOR_FURY:
    {
      const timespan_t duration =
          this->buff.recklessness->data().duration() * azerite.vision_of_perfection_percentage;
      if ( this->buff.recklessness->check() )
      {
        this->buff.recklessness->extend_duration( this, duration );
      }
      else
      {
        this->buff.recklessness->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
      }
      break;
    }

    case WARRIOR_ARMS:
    {
      const timespan_t duration =
          this->buff.bladestorm->data().duration() * azerite.vision_of_perfection_percentage;
      if ( this->buff.bladestorm->check() )
      {
        this->buff.bladestorm->extend_duration( this, duration );
      }
      else
      {
        this->buff.bladestorm->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
      }
      break;
    }

    // TODO: defensive only also proc a limited effectiveness free Ignore Pain
    case WARRIOR_PROTECTION:
    {
      timespan_t trigger_duration = this -> buff.avatar -> data().duration() * azerite.vision_of_perfection_percentage;
      // If Avatar is already active, just extend the buff(s)' durations
      if ( this -> buff.avatar -> check() )
      {
        this -> buff.avatar -> extend_duration( this, trigger_duration );
        if ( azerite.bastion_of_might.enabled() )
        {
          this -> buff.bastion_of_might -> extend_duration( this, trigger_duration );
        }
      }
      // Otherwise, activate avatar and trigger a bad BoM
      else
      {
        this -> buff.avatar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, trigger_duration );
        if ( azerite.bastion_of_might.enabled() )
        {
          this -> buff.bastion_of_might_vop -> trigger();
        }
      }
      break;
    }
    default:
      break;
  }
}


// warrior_t::convert_hybrid_stat ==============================================

stat_e warrior_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    case STAT_AGI_INT:
      return STAT_NONE;
    case STAT_STR_AGI_INT:
    case STAT_STR_AGI:
    case STAT_STR_INT:
      return STAT_STRENGTH;
    case STAT_SPIRIT:
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
        return s;
    default:
      return s;
  }
}

void warrior_t::assess_damage( school_e school, result_amount_type type, action_state_t* s )
{
  player_t::assess_damage( school, type, s );

  if ( s->result == RESULT_DODGE || s->result == RESULT_PARRY )
  {
   if ( rppm.revenge->trigger() )
    {
      buff.revenge->trigger();
    }
  }

  // Generate 3 Rage on auto-attack taken.
  // TODO: Update with spelldata once it actually exists
  // Arma Feb 1 2023 - We check that damage was actually done, as otherwise, flasks that did not do damage were
  // giving us rage
  else if ( ! s -> action -> special && cooldown.rage_from_auto_attack->up() && s->result_raw > 0.0 )
  {
    resource_gain( RESOURCE_RAGE, 3.0, gain.rage_from_damage_taken, s -> action );
    cooldown.rage_from_auto_attack->start( cooldown.rage_from_auto_attack->duration );
  }

  if ( azerite.iron_fortress.enabled() && cooldown.iron_fortress_icd -> up() &&
     ( s -> block_result == BLOCK_RESULT_BLOCKED || s -> block_result == BLOCK_RESULT_CRIT_BLOCKED ) &&
       s -> action -> player -> is_enemy() ) // sanity check - no friendly-fire
  {
    iron_fortress_t* iron_fortress_active = debug_cast<iron_fortress_t*>( active.iron_fortress );

    iron_fortress_active -> crit_blocked = s -> block_result == BLOCK_RESULT_CRIT_BLOCKED;
    iron_fortress_active -> target = s -> action -> player;
    iron_fortress_active -> execute();
  }

  if ( talents.protection.tough_as_nails->ok() && cooldown.tough_as_nails_icd -> up() &&
    ( s -> block_result == BLOCK_RESULT_BLOCKED || s -> block_result == BLOCK_RESULT_CRIT_BLOCKED ) &&
    s -> action -> player -> is_enemy() )
  {
    active.tough_as_nails -> execute_on_target( s -> action -> player );
  }
}

// warrior_t::target_mitigation ============================================

void warrior_t::target_mitigation( school_e school, result_amount_type dtype, action_state_t* s )
{
  player_t::target_mitigation( school, dtype, s );

  if ( s->result == RESULT_HIT || s->result == RESULT_CRIT || s->result == RESULT_GLANCE )
  {
    if ( buff.defensive_stance->up() )
    {
      s->result_amount *= 1.0 + buff.defensive_stance->data().effectN( 1 ).percent();
    }

    warrior_td_t* td = get_target_data( s->action->player );

    if ( td->debuffs_demoralizing_shout->up() )
    {
      s->result_amount *= 1.0 + td->debuffs_demoralizing_shout->value();
    }

    // 2018-10-21: Callous Reprisal's damage reduction is 0.5% per stack per equipped item with the trait selected
    if ( td -> debuffs_callous_reprisal -> up() )
    {
      s -> result_amount *= 1.0 + td -> debuffs_callous_reprisal -> stack_value() * azerite.callous_reprisal.n_items();
    }

    if ( td -> debuffs_punish -> up() )
    {
      s -> result_amount *= 1.0 + td -> debuffs_punish -> value();
    }

    if ( school != SCHOOL_PHYSICAL && buff.spell_reflection->up() )
    {
      s -> result_amount *= 1.0 + buff.spell_reflection->data().effectN( 2 ).percent();
      buff.spell_reflection->expire();
    }
    // take care of dmg reduction CDs
    if ( buff.shield_wall->up() )
    {
      s->result_amount *= 1.0 + buff.shield_wall->value();
    }
    else if ( buff.die_by_the_sword->up() )
    {
      s->result_amount *= 1.0 + buff.die_by_the_sword->default_value;
    }

    if ( specialization() == WARRIOR_PROTECTION )
      s->result_amount *= 1.0 + spec.vanguard -> effectN( 3 ).percent();

    if ( buff.vanguards_determination->up() )
      s->result_amount *= 1.0 + sets->set( WARRIOR_PROTECTION, T29, B2 )->effectN( 1 ).trigger()->effectN( 2 ).percent();
  }
}

// warrior_t::create_options ================================================

void warrior_t::create_options()
{
  player_t::create_options();

  add_option( opt_bool( "non_dps_mechanics", non_dps_mechanics ) );
  add_option( opt_bool( "warrior_fixed_time", warrior_fixed_time ) );
  add_option( opt_int( "into_the_fray_friends", into_the_fray_friends ) );
  add_option( opt_int( "never_surrender_percentage", never_surrender_percentage ) );
  add_option( opt_float( "memory_of_lucid_dreams_proc_chance", options.memory_of_lucid_dreams_proc_chance, 0.0, 1.0 ) );
}

// warrior_t::create_profile ================================================

std::string warrior_t::create_profile( save_e type )
{
  if ( specialization() == WARRIOR_PROTECTION && primary_role() == ROLE_TANK )
  {
    position_str = "front";
  }

  return player_t::create_profile( type );
}

// warrior_t::copy_from =====================================================

void warrior_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warrior_t* p = debug_cast<warrior_t*>( source );

  non_dps_mechanics     = p->non_dps_mechanics;
  warrior_fixed_time    = p->warrior_fixed_time;
  into_the_fray_friends = p->into_the_fray_friends;
  never_surrender_percentage = p -> never_surrender_percentage;
}

void warrior_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  action.apply_affecting_aura( spec.arms_warrior );
  action.apply_affecting_aura( spec.fury_warrior );
  action.apply_affecting_aura( spec.protection_warrior );
  if ( specialization() == WARRIOR_FURY && main_hand_weapon.group() == WEAPON_1H &&
             off_hand_weapon.group() == WEAPON_1H && talents.fury.single_minded_fury->ok() )
  {
  action.apply_affecting_aura( talents.fury.single_minded_fury );
  }
  if ( specialization() == WARRIOR_FURY && talents.warrior.dual_wield_specialization->ok() )
  {
    action.apply_affecting_aura( talents.warrior.dual_wield_specialization );
  }
  if ( specialization() == WARRIOR_ARMS && talents.warrior.two_handed_weapon_specialization->ok() )
  {
    action.apply_affecting_aura( talents.warrior.two_handed_weapon_specialization );
  }
  if ( specialization() == WARRIOR_PROTECTION && talents.warrior.one_handed_weapon_specialization->ok() )
  {
    action.apply_affecting_aura( talents.warrior.one_handed_weapon_specialization );
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warrior_report_t : public player_report_extension_t
{
public:
  warrior_report_t( warrior_t& player ) : p( player )
  {
  }

  void cdwaste_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th colspan=\"3\">Seconds per Execute</th>\n"
       << "<th colspan=\"3\">Seconds per Iteration</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Ability</th>\n"
       << "<th>Average</th>\n"
       << "<th>Minimum</th>\n"
       << "<th>Maximum</th>\n"
       << "<th>Average</th>\n"
       << "<th>Minimum</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";
  }

  void cdwaste_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void cdwaste_table_contents( report::sc_html_stream& os )
  {
    size_t n = 0;
    for ( size_t i = 0; i < p.cd_waste_exec.size(); i++ )
    {
      const data_t* entry = p.cd_waste_exec[ i ];
      if ( entry->second.count() == 0 )
      {
        continue;
      }

      const data_t* iter_entry = p.cd_waste_cumulative[ i ];

      action_t* a          = p.find_action( entry->first );
      std::string name_str = entry->first;
      if ( a )
      {
        name_str = report_decorators::decorated_action(*a);
      }
      else
      {
        name_str = util::encode_html( name_str );
      }

      std::string row_class_str;
      if ( ++n & 1 )
        row_class_str = " class=\"odd\"";

      os.printf( "<tr%s>", row_class_str.c_str() );
      os << "<td class=\"left\">" << name_str << "</td>";
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.min() );
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.max() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.min() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.max() );
      os << "</tr>\n";
    }
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
    if ( !p.cd_waste_exec.empty() )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown waste details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      cdwaste_table_header( os );
      cdwaste_table_contents( os );
      cdwaste_table_footer( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }

    os << "\t\t\t\t\t</div>\n";
  }

private:
  warrior_t& p;
};

namespace items
{
struct raging_fury_t : public unique_gear::scoped_action_callback_t<charge_t>
{
  raging_fury_t() : super( WARRIOR, "charge" )
  {
  }

  void manipulate( charge_t* action, const special_effect_t& e ) override
  {
    action->energize_amount *= 1.0 + e.driver()->effectN( 1 ).percent();
  }
};

struct ayalas_stone_heart_t : public unique_gear::class_buff_cb_t<warrior_t>
{
  ayalas_stone_heart_t() : super( WARRIOR, "stone_heart" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buff.ayalas_stone_heart;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.driver()->effectN( 1 ).trigger() )
        ->set_trigger_spell( e.driver() )
        ->set_rppm( RPPM_NONE );
  }
};

struct the_great_storms_eye_t : public unique_gear::class_buff_cb_t<warrior_t>
{
  the_great_storms_eye_t() : super( WARRIOR, "tornados_eye" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buff.tornados_eye;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    if ( e.player->talent_points->has_row_col( 6, 3 ) )
      return make_buff( e.player, buff_name, e.player->find_spell( 248145 ) )
          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          ->add_invalidate( CACHE_RUN_SPEED );
    else
      return make_buff( e.player, buff_name, e.player->find_spell( 248142 ) )
          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          ->add_invalidate( CACHE_RUN_SPEED );
  }
};

struct kazzalax_fujiedas_fury_t : public unique_gear::class_buff_cb_t<warrior_t>
{
  kazzalax_fujiedas_fury_t() : super( WARRIOR, "fujiedas_fury" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buff.fujiedas_fury;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.driver()->effectN( 1 ).trigger() )
        ->set_default_value( e.driver()->effectN( 1 ).trigger()->effectN( 1 ).percent() )
        ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct prydaz_xavarics_magnum_opus_t : public unique_gear::class_buff_cb_t<warrior_t>
{
  prydaz_xavarics_magnum_opus_t() : super( WARRIOR, "xavarics_magnum_opus" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buff.xavarics_magnum_opus;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.player->find_spell( 207472 ) );
  }
};

struct sephuzs_secret_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  sephuzs_secret_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.sephuzs_secret = e.driver();
  }
};

struct valarjar_berserkers_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  valarjar_berserkers_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.valarjar_berserkers = e.driver();
  }
};

struct archavons_heavy_hand_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  archavons_heavy_hand_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.archavons_heavy_hand = e.driver();
  }
};

struct ceannar_charger_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  ceannar_charger_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.ceannar_charger = e.driver();
  }
};

struct najentuss_vertebrae_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  najentuss_vertebrae_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.najentuss_vertebrae = e.driver();
  }
};

void init()
{
  unique_gear::register_special_effect( 205144, archavons_heavy_hand_t() );
  unique_gear::register_special_effect( 207779, ceannar_charger_t() );
  unique_gear::register_special_effect( 207775, kazzalax_fujiedas_fury_t(), true );
  unique_gear::register_special_effect( 207428, prydaz_xavarics_magnum_opus_t(), true );  // Not finished
  unique_gear::register_special_effect( 215096, najentuss_vertebrae_t() );
  unique_gear::register_special_effect( 207767, ayalas_stone_heart_t(), true );
  unique_gear::register_special_effect( 222266, raging_fury_t() );
  unique_gear::register_special_effect( 208051, sephuzs_secret_t() );
  unique_gear::register_special_effect( 248118, the_great_storms_eye_t(), true );
  unique_gear::register_special_effect( 248120, valarjar_berserkers_t() );
}

}  // items

struct warrior_module_t : public module_t
{
  warrior_module_t() : module_t( WARRIOR )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new warrior_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new warrior_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void static_init() const override
  {
    items::init();
  }

  void register_hotfixes() const override
  {
  }

  void init( player_t* p ) const override
  {
        p->buffs.conquerors_banner = make_buff<stat_buff_t>( p, "conquerors_banner_external", p->find_spell( 325862 ) );
        p->buffs.rallying_cry = make_buff<buffs::rallying_cry_t>( p );
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};
}  // UNNAMED NAMESPACE

const module_t* module_t::warrior()
{
  static warrior_module_t m;
  return &m;
}

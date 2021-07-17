// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "player/pet_spawner.hpp"
#include "action/action_callback.hpp"
#include "sc_enums.hpp"

#include "simulationcraft.hpp"

namespace monk
{
namespace actions
{
namespace spells
{
struct stagger_self_damage_t;
}
}  // namespace actions
namespace pets
{
struct storm_earth_and_fire_pet_t;
}

struct monk_t;

enum class sef_pet_e
{
  SEF_FIRE = 0,
  SEF_EARTH,
  SEF_PET_MAX
};  // Player becomes storm spirit.

enum class sef_ability_e
{
  SEF_NONE = -1,
  // Attacks begin here
  SEF_TIGER_PALM,
  SEF_BLACKOUT_KICK,
  SEF_RISING_SUN_KICK,
  SEF_FISTS_OF_FURY,
  SEF_SPINNING_CRANE_KICK,
  SEF_RUSHING_JADE_WIND,
  SEF_WHIRLING_DRAGON_PUNCH,
  SEF_FIST_OF_THE_WHITE_TIGER,
  SEF_FIST_OF_THE_WHITE_TIGER_OH,
  SEF_ATTACK_MAX,
  // Attacks end here

  // Spells begin here
  SEF_CHI_WAVE,
  SEF_CRACKLING_JADE_LIGHTNING,
  SEF_SPELL_MAX,
  // Spells end here

  // Misc
  SEF_SPELL_MIN  = SEF_CHI_WAVE,
  SEF_ATTACK_MIN = SEF_TIGER_PALM,
  SEF_MAX
};

inline int sef_spell_index( int x )
{
  return x - static_cast<int>( sef_ability_e::SEF_SPELL_MIN );
}

struct monk_td_t : public actor_target_data_t
{
public:
  struct
  {
    dot_t* breath_of_fire;
    dot_t* enveloping_mist;
    dot_t* eye_of_the_tiger_damage;
    dot_t* eye_of_the_tiger_heal;
    dot_t* renewing_mist;
    dot_t* rushing_jade_wind;
    dot_t* soothing_mist;
    dot_t* touch_of_death;
    dot_t* touch_of_karma;
  } dots;

  struct
  {
    // Brewmaster
    buff_t* exploding_keg;
    buff_t* keg_smash;

    // Windwalker
    buff_t* flying_serpent_kick;
    buff_t* empowered_tiger_lightning;
    buff_t* mark_of_the_crane;
    buff_t* storm_earth_and_fire;
    buff_t* touch_of_karma;

    // Covenant Abilities
    buff_t* bonedust_brew;
    buff_t* faeline_stomp;
    buff_t* fallen_monk_keg_smash;
    buff_t* weapons_of_order;

    // Shadowland Legendaries
    buff_t* fae_exposure;
    buff_t* keefers_skyreach;
    buff_t* skyreach_exhaustion;
  } debuff;

  monk_t& monk;
  monk_td_t( player_t* target, monk_t* p );
};

struct monk_t : public player_t
{
public:
  using base_t = player_t;

  // Active
  action_t* windwalking_aura;

  struct sample_data_t
  {
    sc_timeline_t stagger_effective_damage_timeline;
    sc_timeline_t stagger_damage_pct_timeline;
    sc_timeline_t stagger_pct_timeline;
    sample_data_helper_t* stagger_damage;
    sample_data_helper_t* stagger_total_damage;
    sample_data_helper_t* light_stagger_damage;
    sample_data_helper_t* moderate_stagger_damage;
    sample_data_helper_t* heavy_stagger_damage;
    sample_data_helper_t* purified_damage;
    sample_data_helper_t* staggering_strikes_cleared;
    double buffed_stagger_base;
    double buffed_stagger_pct_player_level, buffed_stagger_pct_target_level;
  } sample_datas;

  struct active_actions_t
  {
    action_t* rushing_jade_wind;

    // Brewmaster
    action_t* breath_of_fire;
    heal_t* celestial_fortune;
    heal_t* gift_of_the_ox_trigger;
    heal_t* gift_of_the_ox_expire;
    actions::spells::stagger_self_damage_t* stagger_self_damage;

    // Windwalker
    action_t* sunrise_technique;
    action_t* empowered_tiger_lightning;

    // Conduit
    heal_t* evasive_stride;

    // Covenant
    action_t* bonedust_brew_dmg;
    action_t* bonedust_brew_heal;

    // Legendary
    action_t* bountiful_brew;
  } active_actions;

  std::vector<action_t*> combo_strike_actions;
  double spiritual_focus_count;

  // Blurred time cooldown shenanigans
  std::vector<cooldown_t*> serenity_cooldowns;

  struct stagger_tick_entry_t
  {
    double value;
  };
  std::vector<stagger_tick_entry_t> stagger_tick_damage;  // record stagger tick damage for expression

  double gift_of_the_ox_proc_chance;

  struct buffs_t
  {
    // General
    buff_t* chi_torpedo;
    buff_t* dampen_harm;
    buff_t* diffuse_magic;
    buff_t* rushing_jade_wind;
    buff_t* spinning_crane_kick;

    // Brewmaster
    buff_t* bladed_armor;
    buff_t* blackout_combo;
    absorb_buff_t* celestial_brew;
    buff_t* celestial_flames;
    buff_t* elusive_brawler;
    buff_t* fortifying_brew;
    buff_t* gift_of_the_ox;
    buff_t* invoke_niuzao;
    buff_t* purified_chi;
    buff_t* shuffle;
    buff_t* spitfire;
    buff_t* zen_meditation;
    // niuzao r2 recent purifies fake buff
    buff_t* recent_purifies;

    buff_t* light_stagger;
    buff_t* moderate_stagger;
    buff_t* heavy_stagger;

    // Mistweaver
    absorb_buff_t* life_cocoon;
    buff_t* channeling_soothing_mist;
    buff_t* invoke_chiji;
    buff_t* invoke_chiji_evm;
    buff_t* lifecycles_enveloping_mist;
    buff_t* lifecycles_vivify;
    buff_t* mana_tea;
    buff_t* refreshing_jade_wind;
    buff_t* teachings_of_the_monastery;
    buff_t* touch_of_death_mw;
    buff_t* thunder_focus_tea;
    buff_t* uplifting_trance;

    // Windwalker
    buff_t* bok_proc;
    buff_t* combo_master;
    buff_t* combo_strikes;
    buff_t* dance_of_chiji;
    buff_t* dance_of_chiji_hidden;  // Used for trigger DoCJ ticks
    buff_t* dizzying_kicks;
    buff_t* flying_serpent_kick_movement;
    buff_t* hit_combo;
    buff_t* inner_stength;
    buff_t* invoke_xuen;
    buff_t* storm_earth_and_fire;
    buff_t* serenity;
    buff_t* touch_of_death_ww;
    buff_t* touch_of_karma;
    buff_t* windwalking_driver;
    buff_t* whirling_dragon_punch;

    // Covenant Abilities
    buff_t* bonedust_brew;
    buff_t* bonedust_brew_hidden;
    buff_t* weapons_of_order;
    buff_t* weapons_of_order_ww;
    buff_t* faeline_stomp;
    buff_t* faeline_stomp_brm;
    buff_t* faeline_stomp_reset;
    buff_t* fallen_order;

    // Covenant Conduits
    absorb_buff_t* fortifying_ingrediences;

    // Shadowland Legendary
    buff_t* chi_energy;
    buff_t* charred_passions;
    buff_t* fae_exposure;
    buff_t* invokers_delight;
    buff_t* mighty_pour;
    buff_t* pressure_point;
    buff_t* the_emperors_capacitor;
    buff_t* invoke_xuen_call_to_arms;
  } buff;

public:
  struct gains_t
  {
    gain_t* black_ox_brew_energy;
    gain_t* chi_refund;
    gain_t* bok_proc;
    gain_t* chi_burst;
    gain_t* crackling_jade_lightning;
    gain_t* energy_refund;
    gain_t* energizing_elixir_chi;
    gain_t* energizing_elixir_energy;
    gain_t* expel_harm;
    gain_t* fist_of_the_white_tiger;
    gain_t* focus_of_xuen;
    gain_t* fortuitous_spheres;
    gain_t* gift_of_the_ox;
    gain_t* healing_elixir;
    gain_t* rushing_jade_wind_tick;
    gain_t* serenity;
    gain_t* spirit_of_the_crane;
    gain_t* tiger_palm;
    gain_t* touch_of_death_ww;

    // Azerite Traits
    gain_t* glory_of_the_dawn;
    gain_t* open_palm_strikes;
    gain_t* memory_of_lucid_dreams;
    gain_t* lucid_dreams;

    // Covenants
    gain_t* bonedust_brew;
    gain_t* weapons_of_order;
  } gain;

  struct procs_t
  {
    proc_t* bok_proc;
    proc_t* boiling_brew_healing_sphere;
  } proc;

  struct talents_t
  {
    // Tier 15 Talents
    const spell_data_t* eye_of_the_tiger;  // Brewmaster & Windwalker
    const spell_data_t* chi_wave;
    const spell_data_t* chi_burst;
    // Mistweaver
    const spell_data_t* zen_pulse;

    // Tier 25 Talents
    const spell_data_t* celerity;
    const spell_data_t* chi_torpedo;
    const spell_data_t* tigers_lust;

    // Tier 30 Talents
    // Brewmaster
    const spell_data_t* light_brewing;
    const spell_data_t* spitfire;
    const spell_data_t* black_ox_brew;
    // Windwalker
    const spell_data_t* ascension;
    const spell_data_t* fist_of_the_white_tiger;
    const spell_data_t* energizing_elixir;
    // Mistweaver
    const spell_data_t* spirit_of_the_crane;
    const spell_data_t* mist_wrap;
    const spell_data_t* lifecycles;

    // Tier 35 Talents
    const spell_data_t* tiger_tail_sweep;
    const spell_data_t* summon_black_ox_statue;  // Brewmaster
    const spell_data_t* song_of_chi_ji;          // Mistweaver
    const spell_data_t* ring_of_peace;
    // Windwalker
    const spell_data_t* good_karma;

    // Tier 40 Talents
    // Windwalker
    const spell_data_t* inner_strength;
    // Mistweaver & Windwalker
    const spell_data_t* diffuse_magic;
    // Brewmaster
    const spell_data_t* bob_and_weave;
    const spell_data_t* healing_elixir;
    const spell_data_t* dampen_harm;

    // Tier 45 Talents
    // Brewmaster
    const spell_data_t* special_delivery;
    const spell_data_t* exploding_keg;
    // Windwalker
    const spell_data_t* hit_combo;
    const spell_data_t* dance_of_chiji;
    // Brewmaster & Windwalker
    const spell_data_t* rushing_jade_wind;
    // Mistweaver
    const spell_data_t* summon_jade_serpent_statue;
    const spell_data_t* refreshing_jade_wind;
    const spell_data_t* invoke_chi_ji;

    // Tier 50 Talents
    // Brewmaster
    const spell_data_t* high_tolerance;
    const spell_data_t* celestial_flames;
    const spell_data_t* blackout_combo;
    // Windwalker
    const spell_data_t* spirtual_focus;
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
    const spell_data_t* blackout_kick_2;
    const spell_data_t* blackout_kick_3;
    const spell_data_t* blackout_kick_brm;
    const spell_data_t* crackling_jade_lightning;
    const spell_data_t* critical_strikes;
    const spell_data_t* detox;
    const spell_data_t* expel_harm;
    const spell_data_t* expel_harm_2_brm;
    const spell_data_t* expel_harm_2_mw;
    const spell_data_t* expel_harm_2_ww;
    const spell_data_t* fortifying_brew_brm;
    const spell_data_t* fortifying_brew_2_brm;
    const spell_data_t* fortifying_brew_mw_ww;
    const spell_data_t* fortifying_brew_2_mw;
    const spell_data_t* fortifying_brew_2_ww;
    const spell_data_t* leather_specialization;
    const spell_data_t* leg_sweep;
    const spell_data_t* mystic_touch;
    const spell_data_t* paralysis;
    const spell_data_t* provoke;
    const spell_data_t* provoke_2;
    const spell_data_t* resuscitate;
    const spell_data_t* rising_sun_kick;
    const spell_data_t* rising_sun_kick_2;
    const spell_data_t* roll;
    const spell_data_t* roll_2;
    const spell_data_t* spear_hand_strike;
    const spell_data_t* spinning_crane_kick;
    const spell_data_t* spinning_crane_kick_brm;
    const spell_data_t* spinning_crane_kick_2_brm;
    const spell_data_t* spinning_crane_kick_2_ww;
    const spell_data_t* tiger_palm;
    const spell_data_t* touch_of_death;
    const spell_data_t* touch_of_death_2;
    const spell_data_t* touch_of_death_3_brm;
    const spell_data_t* touch_of_death_3_mw;
    const spell_data_t* touch_of_death_3_ww;
    const spell_data_t* two_hand_adjustment;
    const spell_data_t* vivify;
    const spell_data_t* vivify_2_brm;
    const spell_data_t* vivify_2_mw;
    const spell_data_t* vivify_2_ww;

    // Brewmaster
    const spell_data_t* bladed_armor;
    const spell_data_t* breath_of_fire;
    const spell_data_t* brewmasters_balance;
    const spell_data_t* brewmaster_monk;
    const spell_data_t* celestial_brew;
    const spell_data_t* celestial_brew_2;
    const spell_data_t* celestial_fortune;
    const spell_data_t* clash;
    const spell_data_t* gift_of_the_ox;
    const spell_data_t* invoke_niuzao;
    const spell_data_t* invoke_niuzao_2;
    const spell_data_t* keg_smash;
    const spell_data_t* purifying_brew;
    const spell_data_t* purifying_brew_2;
    const spell_data_t* shuffle;
    const spell_data_t* stagger;
    const spell_data_t* stagger_2;
    const spell_data_t* zen_meditation;

    // Mistweaver
    const spell_data_t* enveloping_mist;
    const spell_data_t* envoloping_mist_2;
    const spell_data_t* essence_font;
    const spell_data_t* essence_font_2;
    const spell_data_t* invoke_yulon;
    const spell_data_t* life_cocoon;
    const spell_data_t* life_cocoon_2;
    const spell_data_t* mistweaver_monk;
    const spell_data_t* reawaken;
    const spell_data_t* renewing_mist;
    const spell_data_t* renewing_mist_2;
    const spell_data_t* revival;
    const spell_data_t* revival_2;
    const spell_data_t* soothing_mist;
    const spell_data_t* teachings_of_the_monastery;
    const spell_data_t* thunder_focus_tea;
    const spell_data_t* thunder_focus_tea_2;

    // Windwalker
    const spell_data_t* afterlife;
    const spell_data_t* afterlife_2;
    const spell_data_t* combat_conditioning;  // Possibly will get removed
    const spell_data_t* combo_breaker;
    const spell_data_t* cyclone_strikes;
    const spell_data_t* disable;
    const spell_data_t* disable_2;
    const spell_data_t* fists_of_fury;
    const spell_data_t* flying_serpent_kick;
    const spell_data_t* flying_serpent_kick_2;
    const spell_data_t* invoke_xuen;
    const spell_data_t* invoke_xuen_2;
    const spell_data_t* reverse_harm;
    const spell_data_t* stance_of_the_fierce_tiger;
    const spell_data_t* storm_earth_and_fire;
    const spell_data_t* storm_earth_and_fire_2;
    const spell_data_t* touch_of_karma;
    const spell_data_t* windwalker_monk;
    const spell_data_t* windwalking;
  } spec;

  struct mastery_spells_t
  {
    const spell_data_t* combo_strikes;    // Windwalker
    const spell_data_t* elusive_brawler;  // Brewmaster
    const spell_data_t* gust_of_mists;    // Mistweaver
  } mastery;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* blackout_kick;
    cooldown_t* black_ox_brew;
    cooldown_t* brewmaster_attack;
    cooldown_t* breath_of_fire;
    cooldown_t* chi_torpedo;
    cooldown_t* celestial_brew;
    cooldown_t* desperate_measure;
    cooldown_t* expel_harm;
    cooldown_t* fist_of_the_white_tiger;
    cooldown_t* fists_of_fury;
    cooldown_t* flying_serpent_kick;
    cooldown_t* fortifying_brew;
    cooldown_t* healing_elixir;
    cooldown_t* invoke_niuzao;
    cooldown_t* invoke_xuen;
    cooldown_t* invoke_yulon;
    cooldown_t* keg_smash;
    cooldown_t* purifying_brew;
    cooldown_t* rising_sun_kick;
    cooldown_t* refreshing_jade_wind;
    cooldown_t* roll;
    cooldown_t* rushing_jade_wind_brm;
    cooldown_t* rushing_jade_wind_ww;
    cooldown_t* storm_earth_and_fire;
    cooldown_t* thunder_focus_tea;
    cooldown_t* touch_of_death;
    cooldown_t* serenity;

    // Covenants
    cooldown_t* weapons_of_order;
    cooldown_t* bonedust_brew;
    cooldown_t* faeline_stomp;
    cooldown_t* fallen_order;

    // Legendary
    cooldown_t* charred_passions;
    cooldown_t* bountiful_brew;
    cooldown_t* sinister_teachings;
  } cooldown;

  struct passives_t
  {
    // General
    const spell_data_t* aura_monk;
    const spell_data_t* chi_burst_damage;
    const spell_data_t* chi_burst_heal;
    const spell_data_t* chi_wave_damage;
    const spell_data_t* chi_wave_heal;
    const spell_data_t* fortifying_brew;
    const spell_data_t* healing_elixir;
    const spell_data_t* mystic_touch;

    // Brewmaster
    const spell_data_t* breath_of_fire_dot;
    const spell_data_t* celestial_fortune;
    const spell_data_t* elusive_brawler;
    const spell_data_t* gift_of_the_ox_heal;
    const spell_data_t* keg_smash_buff;
    const spell_data_t* shuffle;
    const spell_data_t* special_delivery;
    const spell_data_t* stagger_self_damage;
    const spell_data_t* heavy_stagger;
    const spell_data_t* stomp;

    // Mistweaver
    const spell_data_t* renewing_mist_heal;
    const spell_data_t* soothing_mist_heal;
    const spell_data_t* soothing_mist_statue;
    const spell_data_t* spirit_of_the_crane;
    const spell_data_t* totm_bok_proc;
    const spell_data_t* zen_pulse_heal;

    // Windwalker
    const spell_data_t* bok_proc;
    const spell_data_t* crackling_tiger_lightning;
    const spell_data_t* crackling_tiger_lightning_driver;
    const spell_data_t* cyclone_strikes;
    const spell_data_t* dance_of_chiji;
    const spell_data_t* dance_of_chiji_bug;
    const spell_data_t* dizzying_kicks;
    const spell_data_t* empowered_tiger_lightning;
    const spell_data_t* fists_of_fury_tick;
    const spell_data_t* flying_serpent_kick_damage;
    const spell_data_t* focus_of_xuen;
    const spell_data_t* hit_combo;
    const spell_data_t* mark_of_the_crane;
    const spell_data_t* touch_of_karma_tick;
    const spell_data_t* whirling_dragon_punch_tick;

    // Covenants
    const spell_data_t* bonedust_brew_dmg;
    const spell_data_t* bonedust_brew_heal;
    const spell_data_t* bonedust_brew_chi;
    const spell_data_t* faeline_stomp_damage;
    const spell_data_t* faeline_stomp_ww_damage;
    const spell_data_t* faeline_stomp_brm;
    const spell_data_t* fallen_monk_breath_of_fire;
    const spell_data_t* fallen_monk_clash;
    const spell_data_t* fallen_monk_enveloping_mist;
    const spell_data_t* fallen_monk_fists_of_fury;
    const spell_data_t* fallen_monk_fists_of_fury_tick;
    const spell_data_t* fallen_monk_keg_smash;
    const spell_data_t* fallen_monk_soothing_mist;
    const spell_data_t* fallen_monk_spec_duration;
    const spell_data_t* fallen_monk_spinning_crane_kick;
    const spell_data_t* fallen_monk_spinning_crane_kick_tick;
    const spell_data_t* fallen_monk_tiger_palm;

    // Conduits
    const spell_data_t* fortifying_ingredients;
    const spell_data_t* evasive_stride;

    // Shadowland Legendary
    const spell_data_t* chi_explosion;
    const spell_data_t* fae_exposure_dmg;
    const spell_data_t* fae_exposure_heal;
    const spell_data_t* shaohaos_might;
    const spell_data_t* charred_passions_dmg;
    const spell_data_t* call_to_arms_invoke_xuen;
    const spell_data_t* call_to_arms_invoke_niuzao;
    const spell_data_t* call_to_arms_invoke_yulon;
    const spell_data_t* call_to_arms_invoke_chiji;
  } passives;

  // RPPM objects
  struct rppms_t
  {
    // Shadowland Legendary
    real_ppm_t* bountiful_brew;
  } rppm;

  // Covenant
  struct covenant_t
  {
    // Kyrian:
    // Weapons of Order - Increases your Mastery by X% and your
    // Windwalker - Rising Sun Kick reduces the cost of your Chi Abilities by 1 for 5 sec
    // Mistweaver - Essence Font heals nearby allies for (30% of Spell power) health on channel start and end
    // Brewmaster - Keg Smash increases the damage you deal to those enemies by X%, up to 5*X% for 8 sec.
    const spell_data_t* kyrian;

    // Night Fae
    // Faeline Stomp - Strike the ground fiercely to expose a faeline for 30 sec, dealing (X% of Attack power) Nature
    // damage Brewmaster - and igniting enemies with Breath of Fire Mistweaver - and healing allies with an Essence Font
    // bolt Windwalker - and ripping Chi and Energy Spheres out of enemies Your abilities have a 6% chance of resetting
    // the cooldown of Faeline Stomp while fighting on a faeline.
    const spell_data_t* night_fae;

    // Venthyr
    // Fallen Order
    // Opens a mystic portal for 24 sec. Every 2 sec, it summons a spirit of your order's fallen Ox, Crane, or Tiger
    // adepts for 4 sec. Fallen [Ox][Crane][Tiger] adepts assist for an additional 2 sec, and will [attack your enemies
    // with Breath of Fire][heal with Enveloping Mist][assault with Fists of Fury].
    const spell_data_t* venthyr;

    // Necrolord
    // Bonedust Brew
    // Hurl a brew created from the bones of your enemies at the ground, coating all targets struck for 10 sec.  Your
    // abilities have a 35% chance to affect the target a second time at 35% effectiveness as Shadow damage or healing.
    // Mistweaver - Gust of Mists heals targets with your Bonedust Brew active for an additional (42% of Attack power)
    // Brewmaster - Tiger Palm and Keg Smash reduces the cooldown of your brews by an additional 1 sec. when striking
    // enemies with your Bonedust Brew active Windwalker - Spinning Crane Kick refunds 1 Chi when striking enemies with
    // your Bonedust Brew active
    const spell_data_t* necrolord;
  } covenant;

  // Conduits
  struct conduit_t
  {
    // General
    conduit_data_t dizzying_tumble;
    conduit_data_t fortifying_ingredients;
    conduit_data_t grounding_breath;
    conduit_data_t harm_denial;
    conduit_data_t lingering_numbness;
    conduit_data_t swift_transference;
    conduit_data_t tumbling_technique;

    // Brewmaster
    conduit_data_t celestial_effervescence;
    conduit_data_t evasive_stride;
    conduit_data_t scalding_brew;
    conduit_data_t walk_with_the_ox;

    // Mistweaver
    conduit_data_t jade_bond;
    conduit_data_t nourishing_chi;
    conduit_data_t rising_sun_revival;
    conduit_data_t resplendent_mist;

    // Windwalker
    conduit_data_t calculated_strikes;
    conduit_data_t coordinated_offensive;
    conduit_data_t inner_fury;
    conduit_data_t xuens_bond;

    // Covenant
    conduit_data_t strike_with_clarity;
    conduit_data_t imbued_reflections;
    conduit_data_t bone_marrow_hops;
    conduit_data_t way_of_the_fae;
  } conduit;

  struct legendary_t
  {
    // General
    item_runeforge_t fatal_touch;          // 7081
    item_runeforge_t invokers_delight;     // 7082
    item_runeforge_t swiftsure_wraps;      // 7080
    item_runeforge_t escape_from_reality;  // 7184

    // Brewmaster
    item_runeforge_t charred_passions;      // 7076
    item_runeforge_t celestial_infusion;    // 7078
    item_runeforge_t shaohaos_might;        // 7079
    item_runeforge_t stormstouts_last_keg;  // 7077

    // Mistweaver
    item_runeforge_t ancient_teachings_of_the_monastery;  // 7075
    item_runeforge_t clouded_focus;                       // 7074
    item_runeforge_t tear_of_morning;                     // 7072
    item_runeforge_t yulons_whisper;                      // 7073

    // Windwalker
    item_runeforge_t jade_ignition;            // 7071
    item_runeforge_t keefers_skyreach;         // 7068
    item_runeforge_t last_emperors_capacitor;  // 7069
    item_runeforge_t xuens_battlegear;         // 7070

    // Covenant
    item_runeforge_t bountiful_brew;       // 7707; Necrolord Covenant
    item_runeforge_t call_to_arms;         // 7718; Bastion Covenant
    item_runeforge_t faeline_harmony;      // 7721; Night Fae Covenant
    item_runeforge_t sinister_teachings;   // 7726; Venthyr Covenant
  } legendary;

  struct pets_t
  {
    std::array<pets::storm_earth_and_fire_pet_t*, (int)sef_pet_e::SEF_PET_MAX> sef;
    spawner::pet_spawner_t<pet_t, monk_t> xuen;
    spawner::pet_spawner_t<pet_t, monk_t> niuzao;
    spawner::pet_spawner_t<pet_t, monk_t> yulon;
    spawner::pet_spawner_t<pet_t, monk_t> chiji;
    spawner::pet_spawner_t<pet_t, monk_t> fallen_monk_ww;
    spawner::pet_spawner_t<pet_t, monk_t> fallen_monk_mw;
    spawner::pet_spawner_t<pet_t, monk_t> fallen_monk_brm;

    pet_t* bron;

    pets_t( monk_t* p );
  } pets;

  // Options
  struct options_t
  {
    int initial_chi;
    double memory_of_lucid_dreams_proc_chance = 0.15;
    double expel_harm_effectiveness;
    double faeline_stomp_uptime;
    int chi_burst_healing_targets;
  } user_options;

  // Blizzard rounds it's stagger damage; anything higher than half a percent beyond
  // the threshold will switch to the next threshold
  const double light_stagger_threshold;
  const double moderate_stagger_threshold;
  const double heavy_stagger_threshold;

private:
  target_specific_t<monk_td_t> target_data;

public:
  monk_t( sim_t* sim, util::string_view name, race_e r );

  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  // player_t overrides
  action_t* create_action( util::string_view name, const std::string& options ) override;
  double composite_base_armor_multiplier() const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_melee_expertise( const weapon_t* weapon ) const override;
  double composite_melee_attack_power() const override;
  double composite_melee_attack_power_by_type( attack_power_type type ) const override;
  double composite_spell_power( school_e school ) const override;
  double composite_spell_power_multiplier() const override;
  double composite_spell_haste() const override;
  double composite_melee_haste() const override;
  double composite_attack_power_multiplier() const override;
  double composite_dodge() const override;
  double composite_mastery() const override;
  double composite_mastery_rating() const override;
  double composite_crit_avoidance() const override;
  double temporary_movement_modifier() const override;
  double composite_player_dd_multiplier( school_e, const action_t* action ) const override;
  double composite_player_td_multiplier( school_e, const action_t* action ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool guardian ) const override;
  double composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  void create_pets() override;
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_assessors() override;
  void init_rng() override;
  void init_special_effects() override;
  void init_special_effect( special_effect_t& effect ) override;
  void reset() override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void create_options() override;
  void copy_from( player_t* ) override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void pre_analyze_hook() override;
  void combat_begin() override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void assess_damage( school_e, result_amount_type, action_state_t* s ) override;
  void assess_damage_imminent_pre_absorb( school_e, result_amount_type, action_state_t* s ) override;
  void assess_heal( school_e, result_amount_type, action_state_t* s ) override;
  void invalidate_cache( cache_e ) override;
  void init_action_list() override;
  void activate() override;
  void collect_resource_timeline_information() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  const monk_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }
  monk_td_t* get_target_data( player_t* target ) const override
  {
    monk_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new monk_td_t( target, const_cast<monk_t*>( this ) );
    }
    return td;
  }
  void apply_affecting_auras( action_t& ) override;
  void merge( player_t& other ) override;

  // Custom Monk Functions
  void stagger_damage_changed( bool last_tick = false );
  double current_stagger_tick_dmg();
  double current_stagger_tick_dmg_percent();
  double current_stagger_amount_remains_percent();
  double current_stagger_amount_remains();
  double current_stagger_amount_remains_to_total_percent();
  timespan_t current_stagger_dot_remains();
  double stagger_base_value();
  double stagger_pct( int target_level );
  double stagger_total();
  void trigger_celestial_fortune( action_state_t* );
  void trigger_bonedust_brew( const action_state_t* );
  void trigger_keefers_skyreach( action_state_t* );
  void trigger_mark_of_the_crane( action_state_t* );
  void trigger_empowered_tiger_lightning( action_state_t* );
  void trigger_bonedust_brew( action_state_t* );
  player_t* next_mark_of_the_crane_target( action_state_t* );
  int mark_of_the_crane_counter();
  double clear_stagger();
  double partial_clear_stagger_pct( double );
  double partial_clear_stagger_amount( double );
  bool has_stagger();
  double calculate_last_stagger_tick_damage( int n ) const;

  // Storm Earth and Fire targeting logic
  std::vector<player_t*> create_storm_earth_and_fire_target_list() const;
  void summon_storm_earth_and_fire( timespan_t duration );
  void retarget_storm_earth_and_fire( pet_t* pet, std::vector<player_t*>& targets, size_t n_targets ) const;
  void retarget_storm_earth_and_fire_pets() const;

  void bonedust_brew_assessor( action_state_t* );
  void trigger_storm_earth_and_fire( const action_t* a, sef_ability_e sef_ability );
  void storm_earth_and_fire_fixate( player_t* target );
  bool storm_earth_and_fire_fixate_ready( player_t* target );
  player_t* storm_earth_and_fire_fixate_target( sef_pet_e sef_pet );
  void trigger_storm_earth_and_fire_bok_proc( sef_pet_e sef_pet );
};
struct sef_despawn_cb_t
{
  monk_t* monk;

  sef_despawn_cb_t( monk_t* m ) : monk( m )
  {
  }

  void operator()( player_t* );
};
}  // namespace monk

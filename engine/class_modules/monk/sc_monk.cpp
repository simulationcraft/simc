﻿
// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
]NOTES:

- To evaluate Combo Strikes in the APL, use:
    if=combo_break    true if action is a repeat and can combo strike
    if=combo_strike   true if action is not a repeat or can't combo strike

- To show CJL can be interupted in the APL, use:
     &!prev_gcd.crackling_jade_lightning,interrupt=1

TODO:

GENERAL:
- Change Eye of the Tiger from a dot to an interaction with a buff

- Convert Pet base abilities to somethign more generic
- Trigger Bonedust Brew from SEF, Xuen, Niuzao, and Chiji

WINDWALKER:
- Implement Xuen Rank 2 (Enhanced Tiger Lightning)
- Implement Touch of Death Rank 3
- Add Cyclone Strike Counter as an expression

MISTWEAVER:
- Essence Font - See if the implementation can be corrected to the intended design.
- Life Cocoon - Double check if the Enveloping Mists and Renewing Mists from Mists of Life proc the mastery or not.
- Not Modeled:

BREWMASTER:
- Not Modeled:
*/
#include "simulationcraft.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace
{  // UNNAMED NAMESPACE
// Forward declarations
namespace actions
{
namespace spells
{
struct stagger_self_damage_t;
}
namespace pet_summon
{
struct storm_earth_and_fire_t;
}
}  // namespace actions
namespace pets
{
  struct storm_earth_and_fire_pet_t;
  struct xuen_pet_t;
  struct niuzao_pet_t;
  struct yulon_pet_t;
  struct chiji_pet_t;

  // Covenant
  struct fallen_monk_ww_pet_t;
  struct fallen_monk_mw_pet_t;
  struct fallen_monk_brm_pet_t;
}
namespace orbs
{
  struct chi_orb_t;
  struct energy_orb_t;
}  // namespace orbs
struct monk_t;

enum sef_pet_e
{
  SEF_FIRE = 0,
  SEF_EARTH,
  SEF_PET_MAX
};  // Player becomes storm spirit.

enum sef_ability_e
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

unsigned sef_spell_idx( unsigned x )
{
  return x - as<unsigned>( static_cast<int>( SEF_SPELL_MIN ) );
}

struct monk_td_t : public actor_target_data_t
{
public:
  struct dots_t
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

  struct buffs_t
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
    buff_t* keefers_skyreach;
    buff_t* recently_rushing_tiger_palm;
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

  struct
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
    buff_t* touch_of_death;
    buff_t* thunder_focus_tea;
    buff_t* uplifting_trance;

    // Windwalker
    buff_t* bok_proc;
    buff_t* combo_master;
    buff_t* combo_strikes;
    buff_t* dance_of_chiji;
    buff_t* dance_of_chiji_hidden; // Used for trigger DoCJ ticks
    buff_t* dizzying_kicks;
    buff_t* flying_serpent_kick_movement;
    buff_t* hit_combo;
    buff_t* inner_stength;
    buff_t* invoke_xuen;
    buff_t* storm_earth_and_fire;
    buff_t* serenity;
    buff_t* touch_of_karma;
    buff_t* windwalking_driver;
    buff_t* whirling_dragon_punch;

    // Covenant Abilities
    buff_t* bonedust_brew_hidden;
    buff_t* weapons_of_order;
    buff_t* weapons_of_order_ww;
    buff_t* faeline_stomp;
    buff_t* faeline_stomp_brm;

    // Covenant Conduits
    absorb_buff_t* fortifying_ingrediences;

    // Shadowland Legendary
    buff_t* chi_energy;
    buff_t* charred_passions;
    buff_t* invokers_delight;
    buff_t* mighty_pour;
    buff_t* pressure_point;
    buff_t* the_emperors_capacitor;
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
    const spell_data_t* face_palm;
    const spell_data_t* charred_passions_dmg;
  } passives;

  // RPPM objects
  struct rppms_t
  {
    // Azerite Traits
    real_ppm_t* boiling_brew;
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
    // Faeline Stomp - Strike the ground fiercely to expose a faeline for 30 sec, dealing (X% of Attack power) Nature damage
    // Brewmaster - and igniting enemies with Breath of Fire
    // Mistweaver - and healing allies with an Essence Font bolt
    // Windwalker - and ripping Chi and Energy Spheres out of enemies
    // Your abilities have a 6% chance of resetting the cooldown of Faeline Stomp while fighting on a faeline.
    const spell_data_t* night_fae;

    // Venthyr
    // Fallen Order
    // Opens a mystic portal for 24 sec. Every 2 sec, it summons a spirit of your order's fallen Ox, Crane, or Tiger adepts for 4 sec.
    // Fallen [Ox][Crane][Tiger] adepts assist for an additional 2 sec, and will [attack your enemies with Breath of Fire][heal with Enveloping Mist][assault with Fists of Fury].
    const spell_data_t* venthyr;

    // Necrolord
    // Bonedust Brew
    // Hurl a brew created from the bones of your enemies at the ground, coating all targets struck for 10 sec.  Your abilities have a 35% chance to affect the target a second time at 35% effectiveness as Shadow damage or healing.
    // Mistweaver - Gust of Mists heals targets with your Bonedust Brew active for an additional (42% of Attack power)
    // Brewmaster - Tiger Palm and Keg Smash reduces the cooldown of your brews by an additional 1 sec. when striking enemies with your Bonedust Brew active
    // Windwalker - Spinning Crane Kick refunds 1 Chi when striking enemies with your Bonedust Brew active
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
    item_runeforge_t fatal_touch;                        // 7081
    item_runeforge_t invokers_delight;                   // 7082
    item_runeforge_t swiftsure_wraps;                    // 7080
    item_runeforge_t escape_from_reality;                // 7184

    // Brewmaster
    item_runeforge_t charred_passions;                   // 7076
    item_runeforge_t celestial_infusion;                 // 7078
    item_runeforge_t shaohaos_might;                     // 7079
    item_runeforge_t stormstouts_last_keg;               // 7077

    // Mistweaver
    item_runeforge_t ancient_teachings_of_the_monastery; // 7075
    item_runeforge_t clouded_focus;                      // 7074
    item_runeforge_t tear_of_morning;                    // 7072
    item_runeforge_t yulons_whisper;                     // 7073

    // Windwalker
    item_runeforge_t jade_ignition;                      // 7071
    item_runeforge_t keefers_skyreach;                   // 7068
    item_runeforge_t last_emperors_capacitor;            // 7069
    item_runeforge_t xuens_treasure;                     // 7070
  } legendary;

  struct pets_t
  {
    pets::storm_earth_and_fire_pet_t* sef[ SEF_PET_MAX ];
    pets::xuen_pet_t* xuen = nullptr;
    pets::niuzao_pet_t* niuzao = nullptr;
    pets::yulon_pet_t* yulon   = nullptr;
    pets::chiji_pet_t* chiji   = nullptr;
    spawner::pet_spawner_t<pets::fallen_monk_ww_pet_t, monk_t> fallen_monk_ww;
    spawner::pet_spawner_t<pets::fallen_monk_mw_pet_t, monk_t> fallen_monk_mw;
    spawner::pet_spawner_t<pets::fallen_monk_brm_pet_t, monk_t> fallen_monk_brm;

    pets_t( monk_t* p )
      : fallen_monk_ww( "fallen_monk_windwalker", p ),
        fallen_monk_mw( "fallen_monk_mistweaver", p ),
        fallen_monk_brm( "fallen_monk_brewmaster", p )
   {}
  } pets;

  // Options
  struct options_t
  {
    int initial_chi;
    double memory_of_lucid_dreams_proc_chance = 0.15;
    double expel_harm_effectiveness;
    double faeline_stomp_uptime;
  } user_options;

  // Blizzard rounds it's stagger damage; anything higher than half a percent beyond
  // the threshold will switch to the next threshold
  const double light_stagger_threshold;
  const double moderate_stagger_threshold;
  const double heavy_stagger_threshold;

private:
  target_specific_t<monk_td_t> target_data;

public:
  monk_t( sim_t* sim, util::string_view name, race_e r )
    : player_t( sim, MONK, name, r ),
      active_actions(),
      spiritual_focus_count( 0 ),
      gift_of_the_ox_proc_chance(),
      buff(),
      gain(),
      proc(),
      talent(),
      spec(),
      mastery(),
      cooldown(),
      passives(),
      rppm(),
      covenant(),
      conduit(),
      legendary(),
      pets( pets_t( this ) ),
      user_options(),
      light_stagger_threshold( 0 ),
      moderate_stagger_threshold( 0.01666 ),  // Moderate transfers at 33.3% Stagger; 1.67% every 1/2 sec
      heavy_stagger_threshold( 0.03333 )      // Heavy transfers at 66.6% Stagger; 3.34% every 1/2 sec
  {
    // actives
    windwalking_aura = nullptr;

    cooldown.blackout_kick                = get_cooldown( "blackout_kick" );
    cooldown.black_ox_brew                = get_cooldown( "black_ox_brew" );
    cooldown.brewmaster_attack            = get_cooldown( "brewmaster_attack" );
    cooldown.breath_of_fire               = get_cooldown( "breath_of_fire" );
    cooldown.celestial_brew               = get_cooldown( "celestial_brew" );
    cooldown.chi_torpedo                  = get_cooldown( "chi_torpedo" );
    cooldown.expel_harm                   = get_cooldown( "expel_harm" );
    cooldown.fortifying_brew              = get_cooldown( "fortifying_brew" );
    cooldown.fist_of_the_white_tiger      = get_cooldown( "fist_of_the_white_tiger" );
    cooldown.fists_of_fury                = get_cooldown( "fists_of_fury" );
    cooldown.healing_elixir               = get_cooldown( "healing_elixir" );
    cooldown.invoke_niuzao                = get_cooldown( "invoke_niuzao_the_black_ox" );
    cooldown.invoke_xuen                  = get_cooldown( "invoke_xuen_the_white_tiger" );
    cooldown.keg_smash                    = get_cooldown( "keg_smash" );
    cooldown.purifying_brew               = get_cooldown( "purifying_brew" );
    cooldown.rising_sun_kick              = get_cooldown( "rising_sun_kick" );
    cooldown.refreshing_jade_wind         = get_cooldown( "refreshing_jade_wind" );
    cooldown.roll                         = get_cooldown( "roll" );
    cooldown.rushing_jade_wind_brm        = get_cooldown( "rushing_jade_wind" );
    cooldown.rushing_jade_wind_ww         = get_cooldown( "rushing_jade_wind" );
    cooldown.storm_earth_and_fire         = get_cooldown( "storm_earth_and_fire" );
    cooldown.thunder_focus_tea            = get_cooldown( "thunder_focus_tea" );
    cooldown.touch_of_death               = get_cooldown( "touch_of_death" );
    cooldown.serenity                     = get_cooldown( "serenity" );

    // Covenants
    cooldown.weapons_of_order             = get_cooldown( "weapnos_of_order" );
    cooldown.bonedust_brew                = get_cooldown( "bonedust_brew" );
    cooldown.faeline_stomp                = get_cooldown( "faeline_stomp" );
    cooldown.fallen_order                 = get_cooldown( "fallen_order" );

    resource_regeneration = regen_type::DYNAMIC;
    if ( specialization() != MONK_MISTWEAVER )
    {
      regen_caches[ CACHE_HASTE ]        = true;
      regen_caches[ CACHE_ATTACK_HASTE ] = true;
    }
    user_options.initial_chi = 1;
    user_options.expel_harm_effectiveness = 1.0;
    user_options.faeline_stomp_uptime = 1.0;
  }

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
  double composite_melee_crit_chance_multiplier() const override;
  double composite_spell_crit_chance() const override;
  double composite_spell_crit_chance_multiplier() const override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_heal_multiplier( const action_state_t* s ) const override;
  double composite_melee_expertise( const weapon_t* weapon ) const override;
  double composite_melee_attack_power() const override;
  double composite_melee_attack_power_by_type( attack_power_type ap_type ) const override;
  double composite_spell_haste() const override;
  double composite_melee_haste() const override;
  double composite_attack_power_multiplier() const override;
  double composite_parry() const override;
  double composite_dodge() const override;
  double composite_mastery() const override;
  double composite_mastery_rating() const override;
  double composite_crit_avoidance() const override;
  double composite_rating_multiplier( rating_e rating ) const override;
  double resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  double temporary_movement_modifier() const override;
  double passive_movement_modifier() const override;
  void create_pets() override;
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_assessors() override;
  void init_rng() override;
  void init_resources( bool ) override;
  void regen( timespan_t periodicity ) override;
  void reset() override;
  void interrupt() override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void recalculate_resource_max( resource_e, gain_t* g = nullptr ) override;
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

  // Monk specific
  void apl_combat_brewmaster();
  void apl_combat_mistweaver();
  void apl_combat_windwalker();
  void apl_pre_brewmaster();
  void apl_pre_mistweaver();
  void apl_pre_windwalker();

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
  void trigger_bonedust_brew ( const action_state_t* );
  void trigger_mark_of_the_crane( action_state_t* );
  void trigger_empowered_tiger_lightning( action_state_t*);
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

  void accumulate_gale_burst_damage( action_state_t* );
};

namespace orbs
{
}

// ==========================================================================
// Monk Pets & Statues
// ==========================================================================

namespace pets
{

// ==========================================================================
// Base Monk Pet Action
// ==========================================================================

struct monk_pet_t : public pet_t
{
  monk_pet_t( monk_t* owner, util::string_view name, pet_e pet_type, bool guardian, bool dynamic )
    : pet_t( owner->sim, owner, name, pet_type, guardian, dynamic )
  {
  }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }

  const monk_t* o() const
  {
    return static_cast<monk_t*>( owner );
  }
};

template <typename BASE, typename PET_TYPE = monk_pet_t>
struct pet_action_base_t : public BASE
{
  using super_t = BASE;
  using base_t  = pet_action_base_t<BASE>;

  pet_action_base_t( util::string_view n, PET_TYPE* p, const spell_data_t* data = spell_data_t::nil() )
    : BASE( n, p, data )
  {
    // No costs are needed either
    this->base_costs[ RESOURCE_ENERGY ] = 0;
    this->base_costs[ RESOURCE_CHI ]    = 0;
    this->base_costs[ RESOURCE_MANA ]   = 0;
  }

  void init() override
  {
    super_t::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it =
          range::find_if( o()->pet_list, [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }

  monk_t* o()
  {
    return p()->o();
  }

  const monk_t* o() const
  {
    return p()->o();
  }

  const PET_TYPE* p() const
  {
    return debug_cast<const PET_TYPE*>( this->player );
  }

  PET_TYPE* p()
  {
    return debug_cast<PET_TYPE*>( this->player );
  }

  void execute() override
  {
    this->target = this->player->target;

    super_t::execute();
  }
};

// ==========================================================================
// Base Monk Pet Melee Attack
// ==========================================================================

  struct pet_melee_attack_t : public pet_action_base_t<melee_attack_t>
{
  pet_melee_attack_t( util::string_view n, monk_pet_t* p,
                      const spell_data_t* data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }

  // Physical tick_action abilities need amount_type() override, so the
  // tick_action multistrikes are properly physically mitigated.
  result_amount_type amount_type( const action_state_t* state, bool periodic ) const override
  {
    if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
    {
      return result_amount_type::DMG_DIRECT;
    }
    else
    {
      return base_t::amount_type( state, periodic );
    }
  }
};

struct pet_melee_t : pet_melee_attack_t
{
  pet_melee_t( util::string_view name, monk_pet_t* player, weapon_t* weapon )
    : pet_melee_attack_t( name, player, spell_data_t::nil() )
  {
    background = repeating = may_crit = may_glance = true;
    school                                         = SCHOOL_PHYSICAL;
    weapon_multiplier                              = 1.0;
    this->weapon                                   = weapon;
    // Use damage numbers from the level-scaled weapon
    base_execute_time = weapon->swing_time;
    trigger_gcd       = timespan_t::zero();
    special           = false;

    // TODO: check if there should be a dual wield hit malus here.
  }

  void execute() override
  {
    if ( time_to_execute > timespan_t::zero() && player->executing )
    {
      sim->print_debug( "Executing {} during melee ({}).", *player->executing, weapon->slot );
      schedule_execute();
    }
    else
      pet_melee_attack_t::execute();
  }
};

// ==========================================================================
// Generalized Auto Attack Action
// ==========================================================================

struct pet_auto_attack_t : public melee_attack_t
{
  pet_auto_attack_t( monk_pet_t* player ) : melee_attack_t( "auto_attack", player )
  {
    assert( player->main_hand_weapon.type != WEAPON_NONE );
    player->main_hand_attack = nullptr;
    trigger_gcd              = 0_ms;
  }

  void init() override
  {
    melee_attack_t::init();

    assert( player->main_hand_attack && "Pet auto attack created without main hand attack" );
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();

    if ( player->off_hand_attack )
      player->off_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    return ( player->main_hand_attack->execute_event == nullptr );
  }
};

// ==========================================================================
// Base Monk Pet Spell
// ==========================================================================

struct pet_spell_t : public pet_action_base_t<spell_t>
{
  pet_spell_t( util::string_view n, monk_pet_t* p, const spell_data_t* data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }
};

// ==========================================================================
// Base Monk Heal Spell
// ==========================================================================

struct pet_heal_t : public pet_action_base_t<heal_t>
{
  pet_heal_t( util::string_view n, monk_pet_t* p, const spell_data_t* data = spell_data_t::nil() ) : base_t( n, p, data )
  {
  }
};


// ==========================================================================
// Monk Statues
// ==========================================================================

struct statue_t : public monk_pet_t
{
  statue_t( monk_t* owner, util::string_view n, pet_e pt, bool guardian = false )
    : monk_pet_t( owner, n, pt, guardian, false )
  {
  }
};

struct jade_serpent_statue_t : public statue_t
{
  jade_serpent_statue_t( monk_t* owner, util::string_view n ) : statue_t( owner, n, PET_NONE, true )
  {
  }
};

// ==========================================================================
// Storm Earth and Fire (SEF)
// ==========================================================================

struct storm_earth_and_fire_pet_t : public monk_pet_t
{
  // Storm, Earth, and Fire abilities begin =================================

  template <typename BASE>
  struct sef_action_base_t : public pet_action_base_t<BASE, storm_earth_and_fire_pet_t>
  {
    using super_t = pet_action_base_t<BASE, storm_earth_and_fire_pet_t>;
    using base_t  = sef_action_base_t<BASE>;

    const action_t* source_action;

    sef_action_base_t( util::string_view n, storm_earth_and_fire_pet_t* p,
                       const spell_data_t* data = spell_data_t::nil() )
      : super_t( n, p, data ), source_action( nullptr )
    {
      // Make SEF attacks always background, so they do not consume resources
      // or do anything associated with "foreground actions".
      this->background = this->may_crit = true;
      this->callbacks                   = false;

      // Cooldowns are handled automatically by the mirror abilities, the SEF specific ones need none.
      this->cooldown->duration = timespan_t::zero();

      // No costs are needed either
      this->base_costs[ RESOURCE_ENERGY ] = 0;
      this->base_costs[ RESOURCE_CHI ]    = 0;
    }

    void init() override
    {
      super_t::init();

      // Find source_action from the owner by matching the action name and
      // spell id with eachother. This basically means that by default, any
      // spell-data driven ability with 1:1 mapping of name/spell id will
      // always be chosen as the source action. In some cases this needs to be
      // overridden (see sef_zen_sphere_t for example).
      for ( const action_t* a : this->o()->action_list )
      {
        if ( ( this->id > 0 && this->id == a->id ) || util::str_compare_ci( this->name_str, a->name_str ) )
        {
          source_action = a;
          break;
        }
      }

      if ( source_action )
      {
        this->update_flags   = source_action->update_flags;
        this->snapshot_flags = source_action->snapshot_flags;
      }
    }

    // Use SEF-specific override methods for target related multipliers as the
    // pets seem to have their own functionality relating to it. The rest of
    // the state-related stuff is actually mapped to the source (owner) action
    // below.

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = super_t::composite_target_multiplier( t );

      return m;
    }

    // Map the rest of the relevant state-related stuff into the source
    // action's methods. In other words, use the owner's data. Note that attack
    // power is not included here, as we will want to (just in case) snapshot
    // AP through the pet's own AP system. This allows us to override the
    // inheritance coefficient if need be in an easy way.

    double attack_direct_power_coefficient( const action_state_t* state ) const override
    {
      return source_action->attack_direct_power_coefficient( state );
    }

    double attack_tick_power_coefficient( const action_state_t* state ) const override
    {
      return source_action->attack_tick_power_coefficient( state );
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      return source_action->composite_dot_duration( s );
    }

    timespan_t tick_time( const action_state_t* s ) const override
    {
      return source_action->tick_time( s );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      return source_action->composite_da_multiplier( s );
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      return source_action->composite_ta_multiplier( s );
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      return source_action->composite_persistent_multiplier( s );
    }

    double composite_versatility( const action_state_t* s ) const override
    {
      return source_action->composite_versatility( s );
    }

    double composite_haste() const override
    {
      return source_action->composite_haste();
    }

    timespan_t travel_time() const override
    {
      return source_action->travel_time();
    }

    int n_targets() const override
    {
      return source_action ? source_action->n_targets() : super_t::n_targets();
    }

    void execute() override
    {
      // Target always follows the SEF clone's target, which is assigned during
      // summon time
      this->target = this->player->target;

      super_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      auto owner = this->o();

      owner->trigger_empowered_tiger_lightning( s );
      owner->trigger_bonedust_brew( s );

      super_t::impact( s );
    }

    void snapshot_internal( action_state_t* state, uint32_t flags, result_amount_type rt ) override
    {
      super_t::snapshot_internal( state, flags, rt );

      if ( this->o()->conduit.coordinated_offensive->ok() && this->p()->sticky_target )
      {
         if ( rt == result_amount_type::DMG_DIRECT && ( flags & STATE_MUL_DA ) )
           state->da_multiplier += this->o()->conduit.coordinated_offensive.percent();

         if ( rt == result_amount_type::DMG_OVER_TIME && ( flags & STATE_MUL_TA ) )
           state->ta_multiplier += this->o()->conduit.coordinated_offensive.percent();
      }
    }
  };

  struct sef_melee_attack_t : public sef_action_base_t<melee_attack_t>
  {
    sef_melee_attack_t( util::string_view n, storm_earth_and_fire_pet_t* p,
                        const spell_data_t* data = spell_data_t::nil() )
      : base_t( n, p, data )
    {
      school = SCHOOL_PHYSICAL;
    }

    // Physical tick_action abilities need amount_type() override, so the
    // tick_action multistrikes are properly physically mitigated.
    result_amount_type amount_type( const action_state_t* state, bool periodic ) const override
    {
      if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
      {
        return result_amount_type::DMG_DIRECT;
      }
      else
      {
        return base_t::amount_type( state, periodic );
      }
    }
  };

  struct sef_spell_t : public sef_action_base_t<spell_t>
  {
    sef_spell_t( util::string_view n, storm_earth_and_fire_pet_t* p, const spell_data_t* data = spell_data_t::nil() )
      : base_t( n, p, data )
    {
    }
  };

  // Auto attack ============================================================

  struct melee_t : public sef_melee_attack_t
  {
    melee_t( util::string_view n, storm_earth_and_fire_pet_t* player, weapon_t* w )
      : sef_melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = repeating = may_crit = may_glance = true;
      weapon                                         = w;
      school                                         = SCHOOL_PHYSICAL;
      weapon_multiplier                              = 1.0;
      base_execute_time                              = w->swing_time;
      trigger_gcd                                    = timespan_t::zero();
      special                                        = false;

      if ( player->dual_wield() )
      {
        base_hit -= 0.19;
      }

      if ( w == &( player->main_hand_weapon ) )
      {
        source_action = player->owner->find_action( "melee_main_hand" );
      }
      else
      {
        source_action = player->owner->find_action( "melee_off_hand" );
        // If owner is using a 2handed weapon, there's not going to be an
        // off-hand action for autoattacks, thus just use main hand one then.
        if ( !source_action )
        {
          source_action = player->owner->find_action( "melee_main_hand" );
        }
      }

      // TODO: Can't really assert here, need to figure out a fallback if the
      // windwalker does not use autoattacks (how likely is that?)
      if ( !source_action && sim->debug )
      {
        sim->error( "{} has no auto_attack in APL, Storm, Earth, and Fire pets cannot auto-attack.", *o() );
      }
    }

    // A wild equation appears
    double composite_attack_power() const override
    {
      double ap = sef_melee_attack_t::composite_attack_power();

      if ( o()->main_hand_weapon.group() == WEAPON_2H )
      {
        ap += o()->main_hand_weapon.dps * 3.5;
      }
      else
      {
        // 1h/dual wield equation. Note, this formula is slightly off (~3%) for
        // owner dw/pet dw variation.
        double total_dps = o()->main_hand_weapon.dps;
        double dw_mul    = 1.0;
        if ( o()->off_hand_weapon.group() != WEAPON_NONE )
        {
          total_dps += o()->off_hand_weapon.dps * 0.5;
          dw_mul = 0.898882275;
        }

        ap += total_dps * 3.5 * dw_mul;
      }

      return ap;
    }

    void execute() override
    {
      if ( time_to_execute > timespan_t::zero() && ( player->channeling || player->executing ) )
      {
        sim->print_debug( "{} Executing {} during melee ({}).", *player,
                                 player->executing ? *player->executing : *player->channeling,
                                 weapon->slot );

        schedule_execute();
      }
      else
      {
        sef_melee_attack_t::execute();
      }
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( storm_earth_and_fire_pet_t* player, util::string_view options_str )
      : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      melee_t* mh = new melee_t( "auto_attack_mh", player, &( player->main_hand_weapon ) );
      if ( !mh->source_action )
      {
        background = true;
        return;
      }
      player->main_hand_attack = mh;

      if ( player->dual_wield() )
      {
        player->off_hand_attack = new melee_t( "auto_attack_oh", player, &( player->off_hand_weapon ) );
      }
    }
  };

  // Special attacks ========================================================
  //
  // Note, these automatically use the owner's multipliers, so there's no need
  // to adjust anything here.

  struct sef_tiger_palm_t : public sef_melee_attack_t
  {
    sef_tiger_palm_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "tiger_palm", player, player->o()->spec.tiger_palm )
    {
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) && o()->spec.spinning_crane_kick_2_ww->ok() )
        o()->trigger_mark_of_the_crane( state );
    }
  };

  struct sef_blackout_kick_t : public sef_melee_attack_t
  {
    sef_blackout_kick_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "blackout_kick", player, player->o()->spec.blackout_kick )
    {
      // Hard Code the divider
      base_dd_min = base_dd_max = 1;
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( o()->spec.spinning_crane_kick_2_ww->ok() )
          o()->trigger_mark_of_the_crane( state );

        if ( p()->buff.bok_proc_sef->up() )
          p()->buff.bok_proc_sef->expire();
      }
    }
  };

  struct sef_rising_sun_kick_dmg_t : public sef_melee_attack_t
  {
    sef_rising_sun_kick_dmg_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "rising_sun_kick_dmg", player, player->o()->spec.rising_sun_kick->effectN( 1 ).trigger() )
    {
      background = true;
    }

    double composite_crit_chance() const override
    {
      double c = sef_melee_attack_t::composite_crit_chance();

      if ( o()->buff.pressure_point->up() )
        c += o()->buff.pressure_point->value();

      return c;
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( o()->spec.combat_conditioning->ok() )
          state->target->debuffs.mortal_wounds->trigger();

        if ( o()->spec.spinning_crane_kick_2_ww->ok() )
          o()->trigger_mark_of_the_crane( state );
      }
    }
  };

  struct sef_rising_sun_kick_t : public sef_melee_attack_t
  {
    sef_rising_sun_kick_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "rising_sun_kick", player, player->o()->spec.rising_sun_kick )
    {
      execute_action = new sef_rising_sun_kick_dmg_t( player );
    }
  };

  struct sef_tick_action_t : public sef_melee_attack_t
  {
    sef_tick_action_t( const std::string& name, storm_earth_and_fire_pet_t* p, const spell_data_t* data )
      : sef_melee_attack_t( name, p, data )
    {
      aoe = -1;

      // Reset some variables to ensure proper execution
      dot_duration = timespan_t::zero();
      school       = SCHOOL_PHYSICAL;
    }
  };

  struct sef_fists_of_fury_tick_t : public sef_tick_action_t
  {
    sef_fists_of_fury_tick_t( storm_earth_and_fire_pet_t* p )
      : sef_tick_action_t( "fists_of_fury_tick", p, p->o()->passives.fists_of_fury_tick )
    {
      aoe = 1 + (int)p->o()->spec.fists_of_fury->effectN( 1 ).base_value();
    }
  };

  struct sef_fists_of_fury_t : public sef_melee_attack_t
  {
    sef_fists_of_fury_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "fists_of_fury", player, player->o()->spec.fists_of_fury )
    {
      channeled = tick_zero = interrupt_auto_attack = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
      // Hard code a 25% reduced cast time to not cause any clipping issues
      // https://us.battle.net/forums/en/wow/topic/20752377961?page=29#post-573
      dot_duration = data().duration() / 1.25;
      // Effect 1 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
      base_tick_time = ( dot_duration / 4 );

      weapon_power_mod = 0;

      tick_action = new sef_fists_of_fury_tick_t( player );
    }

    // Base tick_time(action_t) is somehow pulling the Owner's base_tick_time instead of the pet's
    // Forcing SEF to use it's own base_tick_time for tick_time.
    timespan_t tick_time( const action_state_t* state ) const override
    {
      timespan_t t = base_tick_time;
      if ( channeled || hasted_ticks )
      {
        t *= state->haste;
      }
      return t;
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      if ( channeled )
        return dot_duration * ( tick_time( s ) / base_tick_time );

      return dot_duration;
    }
  };

  struct sef_spinning_crane_kick_tick_t : public sef_tick_action_t
  {
    sef_spinning_crane_kick_tick_t( storm_earth_and_fire_pet_t* p )
      : sef_tick_action_t( "spinning_crane_kick_tick", p, p->o()->spec.spinning_crane_kick->effectN( 1 ).trigger() )
    {
      aoe = as<int>( p->o()->spec.spinning_crane_kick->effectN( 1 ).base_value() );
    }
  };

  struct sef_spinning_crane_kick_t : public sef_melee_attack_t
  {
    sef_spinning_crane_kick_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "spinning_crane_kick", player, player->o()->spec.spinning_crane_kick )
    {
      tick_zero = hasted_ticks = interrupt_auto_attack = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_spinning_crane_kick_tick_t( player );
    }
  };

  struct sef_rushing_jade_wind_tick_t : public sef_tick_action_t
  {
    sef_rushing_jade_wind_tick_t( storm_earth_and_fire_pet_t* p )
      : sef_tick_action_t( "rushing_jade_wind_tick", p, p->o()->talent.rushing_jade_wind->effectN( 1 ).trigger() )
    {
      aoe = -1;
    }
  };

  struct sef_rushing_jade_wind_t : public sef_melee_attack_t
  {
    sef_rushing_jade_wind_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "rushing_jade_wind", player, player->o()->talent.rushing_jade_wind )
    {
      dual = true;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      if ( !player->active_actions.rushing_jade_wind_sef )
      {
        player->active_actions.rushing_jade_wind_sef        = new sef_rushing_jade_wind_tick_t( player );
        player->active_actions.rushing_jade_wind_sef->stats = stats;
      }
    }

    void execute() override
    {
      sef_melee_attack_t::execute();

      p()->buff.rushing_jade_wind_sef->trigger();
    }
  };

  struct sef_whirling_dragon_punch_tick_t : public sef_tick_action_t
  {
    sef_whirling_dragon_punch_tick_t( storm_earth_and_fire_pet_t* p )
      : sef_tick_action_t( "whirling_dragon_punch_tick", p, p->o()->passives.whirling_dragon_punch_tick )
    {
      aoe = -1;
    }
  };

  struct sef_whirling_dragon_punch_t : public sef_melee_attack_t
  {
    sef_whirling_dragon_punch_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "whirling_dragon_punch", player, player->o()->talent.whirling_dragon_punch )
    {
      channeled = true;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_whirling_dragon_punch_tick_t( player );
    }
  };

  struct sef_fist_of_the_white_tiger_oh_t : public sef_melee_attack_t
  {
    sef_fist_of_the_white_tiger_oh_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "fist_of_the_white_tiger_offhand", player, player->o()->talent.fist_of_the_white_tiger )
    {
      may_dodge = may_parry = may_block = may_miss = true;
      dual                                         = true;

      energize_type = action_energize::NONE;
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( o()->spec.spinning_crane_kick_2_ww->ok() )
          o()->trigger_mark_of_the_crane( state );
      }
    }
  };

  struct sef_fist_of_the_white_tiger_t : public sef_melee_attack_t
  {
    sef_fist_of_the_white_tiger_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "fist_of_the_white_tiger", player,
                            player->o()->talent.fist_of_the_white_tiger->effectN( 2 ).trigger() )
    {
      may_dodge = may_parry = may_block = may_miss = true;
      dual                                         = true;
    }
  };

  struct sef_chi_wave_damage_t : public sef_spell_t
  {
    sef_chi_wave_damage_t( storm_earth_and_fire_pet_t* player )
      : sef_spell_t( "chi_wave_damage", player, player->o()->passives.chi_wave_damage )
    {
      dual = true;
    }
  };

  // SEF Chi Wave skips the healing ticks, delivering damage on every second
  // tick of the ability for simplicity.
  struct sef_chi_wave_t : public sef_spell_t
  {
    sef_chi_wave_damage_t* wave;

    sef_chi_wave_t( storm_earth_and_fire_pet_t* player )
      : sef_spell_t( "chi_wave", player, player->o()->talent.chi_wave ), wave( new sef_chi_wave_damage_t( player ) )
    {
      may_crit = may_miss = hasted_ticks = false;
      tick_zero = tick_may_crit = true;
    }

    void tick( dot_t* d ) override
    {
      if ( d->current_tick % 2 == 0 )
      {
        sef_spell_t::tick( d );
        wave->target = d->target;
        wave->schedule_execute();
      }
    }
  };

  struct sef_crackling_jade_lightning_t : public sef_spell_t
  {
    sef_crackling_jade_lightning_t( storm_earth_and_fire_pet_t* player )
      : sef_spell_t( "crackling_jade_lightning", player, player->o()->spec.crackling_jade_lightning )
    {
      tick_may_crit         = true;
      channeled = tick_zero = true;
      hasted_ticks          = false;
      interrupt_auto_attack = true;
      dot_duration          = data().duration();
    }

    // Base tick_time(action_t) is somehow pulling the Owner's base_tick_time instead of the pet's
    // Forcing SEF to use it's own base_tick_time for tick_time.
    timespan_t tick_time( const action_state_t* state ) const override
    {
      timespan_t t = base_tick_time;
      if ( channeled || hasted_ticks )
      {
        t *= state->haste;
      }
      return t;
    }

    double cost_per_tick( resource_e resource ) const override
    {
      double c = sef_spell_t::cost_per_tick( resource );

     c = 0;

      return c;
    }
  };

  // Storm, Earth, and Fire abilities end ===================================

  std::vector<sef_melee_attack_t*> attacks;
  std::vector<sef_spell_t*> spells;

public:
  // SEF applies the Cyclone Strike debuff as well

  bool sticky_target;  // When enabled, SEF pets will stick to the target they have

  struct active_actions_t
  {
    action_t* rushing_jade_wind_sef = nullptr;
  } active_actions;

  struct buffs_t
  {
    buff_t* bok_proc_sef          = nullptr;
    buff_t* hit_combo_sef         = nullptr;
    buff_t* pressure_point_sef    = nullptr;
    buff_t* rushing_jade_wind_sef = nullptr;
  } buff;

  storm_earth_and_fire_pet_t( util::string_view name, monk_t* owner, bool dual_wield,
                              weapon_e weapon_type )
    : monk_pet_t( owner, name, PET_NONE, true, true ),
      attacks( SEF_ATTACK_MAX ),
      spells( SEF_SPELL_MAX - SEF_SPELL_MIN ),
      sticky_target( false ),
      buff( buffs_t() )
  {
    // Storm, Earth, and Fire pets have to become "Windwalkers", so we can get
    // around some sanity checks in the action execution code, that prevents
    // abilities meant for a certain specialization to be executed by actors
    // that do not have the specialization.
    _spec = MONK_WINDWALKER;

    main_hand_weapon.type       = weapon_type;
    main_hand_weapon.swing_time = timespan_t::from_seconds( dual_wield ? 2.6 : 3.6 );

    if ( dual_wield )
    {
      off_hand_weapon.type       = weapon_type;
      off_hand_weapon.swing_time = timespan_t::from_seconds( 2.6 );
    }

    if ( name == "fire_spirit" )
      npc_id = 69791;
    else if ( name == "earth_spirit" )
      npc_id = 69792;

    owner_coeff.ap_from_ap = 1.0;
  }

  // Reset SEF target to default settings
  void reset_targeting()
  {
    target        = owner->target;
    sticky_target = false;
  }

  timespan_t available() const override
  {
    return sim->expected_iteration_time * 2;
  }

  void init_spells() override
  {
    monk_pet_t::init_spells();

    attacks.at( SEF_TIGER_PALM )                 = new sef_tiger_palm_t( this );
    attacks.at( SEF_BLACKOUT_KICK )              = new sef_blackout_kick_t( this );
    attacks.at( SEF_RISING_SUN_KICK )            = new sef_rising_sun_kick_t( this );
    attacks.at( SEF_FISTS_OF_FURY )              = new sef_fists_of_fury_t( this );
    attacks.at( SEF_SPINNING_CRANE_KICK )        = new sef_spinning_crane_kick_t( this );
    attacks.at( SEF_RUSHING_JADE_WIND )          = new sef_rushing_jade_wind_t( this );
    attacks.at( SEF_WHIRLING_DRAGON_PUNCH )      = new sef_whirling_dragon_punch_t( this );
    attacks.at( SEF_FIST_OF_THE_WHITE_TIGER )    = new sef_fist_of_the_white_tiger_t( this );
    attacks.at( SEF_FIST_OF_THE_WHITE_TIGER_OH ) = new sef_fist_of_the_white_tiger_oh_t( this );

    spells.at( sef_spell_idx( SEF_CHI_WAVE ) )                 = new sef_chi_wave_t( this );
    spells.at( sef_spell_idx( SEF_CRACKLING_JADE_LIGHTNING ) ) = new sef_crackling_jade_lightning_t( this );
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";

    monk_pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    monk_pet_t::summon( duration );

    o()->buff.storm_earth_and_fire->trigger( 1, buff_t::DEFAULT_VALUE(), 1, duration );

    if ( o()->buff.bok_proc->up() )
      buff.bok_proc_sef->trigger( 1, buff_t::DEFAULT_VALUE(), 1, o()->buff.bok_proc->remains() );

    if ( o()->buff.hit_combo->up() )
      buff.hit_combo_sef->trigger( o()->buff.hit_combo->stack() );

    if ( o()->buff.rushing_jade_wind->up() )
      buff.rushing_jade_wind_sef->trigger( 1, buff_t::DEFAULT_VALUE(), 1, o()->buff.rushing_jade_wind->remains() );

    sticky_target = false;
  }

  void dismiss( bool expired = false ) override
  {
    monk_pet_t::dismiss( expired );

    o()->buff.storm_earth_and_fire->decrement();
  }

  void create_buffs() override
  {
    monk_pet_t::create_buffs();

    buff.bok_proc_sef =
        make_buff( this, "bok_proc_sef", o()->passives.bok_proc )
            ->set_quiet( true );  // In-game does not show this buff but I would like to use it for background stuff;

    buff.rushing_jade_wind_sef = make_buff( this, "rushing_jade_wind_sef", o()->talent.rushing_jade_wind )
                                     ->set_can_cancel( true )
                                     ->set_tick_zero( true )
                                     ->set_cooldown( timespan_t::zero() )
                                     ->set_period( o()->talent.rushing_jade_wind->effectN( 1 ).period() )
                                     ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                                     ->set_duration( sim->expected_iteration_time * 2 )
                                     ->set_tick_behavior( buff_tick_behavior::CLIP )
                                     ->set_tick_callback( [this]( buff_t* d, int, timespan_t ) {
                                       if ( o()->buff.rushing_jade_wind->up() )
                                         active_actions.rushing_jade_wind_sef->execute();
                                       else
                                         d->expire( timespan_t::from_millis( 1 ) );
                                     } );

    buff.hit_combo_sef = make_buff( this, "hit_combo_sef", o()->passives.hit_combo )
                             ->set_default_value_from_effect( 1 )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void trigger_attack( sef_ability_e ability, const action_t* source_action )
  {
    if ( ability >= SEF_SPELL_MIN )
    {
      size_t spell = static_cast<size_t>( ability - SEF_SPELL_MIN );
      assert( spells[ spell ] );

      if ( o()->buff.combo_strikes->up() && o()->talent.hit_combo->ok() )
        buff.hit_combo_sef->trigger();

      spells[ spell ]->source_action = source_action;
      spells[ spell ]->execute();
    }
    else
    {
      assert( attacks[ ability ] );

      if ( o()->buff.combo_strikes->up() && o()->talent.hit_combo->ok() )
        buff.hit_combo_sef->trigger();

      attacks[ ability ]->source_action = source_action;
      attacks[ ability ]->execute();
    }
  }
};

// ==========================================================================
// Xuen Pet
// ==========================================================================
struct xuen_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, xuen_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );
      o()->trigger_bonedust_brew( s );

      pet_melee_attack_t::impact( s );
    }
  };

  struct crackling_tiger_lightning_tick_t : public pet_spell_t
  {
    crackling_tiger_lightning_tick_t( xuen_pet_t* p )
      : pet_spell_t( "crackling_tiger_lightning_tick", p, p->o()->passives.crackling_tiger_lightning )
    {
      dual = direct_tick = background = may_crit = true;
    }

    void impact( action_state_t* s ) override
    {
      auto owner = o();
      owner->trigger_empowered_tiger_lightning( s );
      owner->trigger_bonedust_brew( s );

      pet_spell_t::impact( s );
    }
  };

  struct crackling_tiger_lightning_t : public pet_spell_t
  {
    crackling_tiger_lightning_t( xuen_pet_t* p, util::string_view options_str )
      : pet_spell_t( "crackling_tiger_lightning", p, p->o()->passives.crackling_tiger_lightning )
    {
      parse_options( options_str );

      // for future compatibility, we may want to grab Xuen and our tick spell and build this data from those (Xuen
      // summon duration, for example)
      dot_duration        = p->o()->spec.invoke_xuen->duration();
      hasted_ticks        = true;
      may_miss            = false;
      dynamic_tick_action = true;
      base_tick_time =
          p->o()->passives.crackling_tiger_lightning_driver->effectN( 1 ).period();  // trigger a tick every second
      cooldown->duration      = p->o()->spec.invoke_xuen->cooldown();              // we're done after 25 seconds
      attack_power_mod.direct = 0.0;
      attack_power_mod.tick   = 0.0;

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( xuen_pet_t* player, util::string_view options_str )
      : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack                    = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  xuen_pet_t( monk_t* owner ) : monk_pet_t( owner, "xuen_the_white_tiger", PET_XUEN, true, true )
  {
    npc_id                      = 63508;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap      = 1.00;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = owner->cache.player_multiplier( school );

    if ( o()->conduit.xuens_bond->ok() )
      cpm *= 1 + o()->conduit.xuens_bond.percent();

    return cpm;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "crackling_tiger_lightning" )
      return new crackling_tiger_lightning_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Niuzao Pet
// ==========================================================================
struct niuzao_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, niuzao_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_bonedust_brew( s );

      pet_melee_t::impact( s );
    }
  };

  struct stomp_t : public pet_melee_attack_t
  {
    stomp_t( niuzao_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( "stomp", p, p->o()->passives.stomp )
    {
      parse_options( options_str );

      aoe          = -1;
      may_crit     = true;
      // technically the base damage doesn't split. practically, the base damage
      // is ass and totally irrelevant. the r2 hot trub effect (which does
      // split) is by far the dominating factor in any aoe sim.
      //
      // if i knew more about simc, i'd implement a separate effect for that,
      // but i'm not breaking something that (mostly) works in pursuit of that
      // goal.
      //
      //  - emallson
      split_aoe_damage = true;
    }

    double bonus_da( const action_state_t* s ) const override
    {
      double b = pet_melee_attack_t::bonus_da( s );

      niuzao_pet_t* p = static_cast<niuzao_pet_t*>( player );

      auto purify_amount = p->o()->buff.recent_purifies->value();
      auto actual_damage = purify_amount * p->o()->spec.invoke_niuzao_2->effectN( 1 ).percent();
      b += actual_damage;
      p->o()->sim->print_debug( "applying bonus purify damage (original: {}, reduced: {})", purify_amount,
                                actual_damage );

      return b;
    }

    double action_multiplier() const override
    {
      double am       = pet_melee_attack_t::action_multiplier();
      niuzao_pet_t* p = static_cast<niuzao_pet_t*>( player );

      if ( p->o()->conduit.walk_with_the_ox->ok() )
        am *= 1 + p->o()->conduit.walk_with_the_ox.percent();

      return am;
    }

    void execute() override
    {
      pet_melee_attack_t::execute();
      // canceling the purify buff goes here so that in aoe all hits see the
      // purified damage that needs to be split. this occurs after all damage
      // has been dealt
      niuzao_pet_t* p = static_cast<niuzao_pet_t*>( player );
      p->o()->buff.recent_purifies->cancel();
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_bonedust_brew( s );

      pet_melee_attack_t::impact( s );
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( niuzao_pet_t* player, util::string_view options_str )
      : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack                    = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  niuzao_pet_t( monk_t* owner ) : monk_pet_t( owner, "niuzao_the_black_ox", PET_NIUZAO, true, true )
  {
    npc_id                      = 73967;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_ap      = 1;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = pet_t::composite_player_multiplier( school );

    cpm *= 1 + o()->spec.brewmaster_monk->effectN( 3 ).percent();

    return cpm;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/stomp";

    pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "stomp" )
      return new stomp_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Chi-Ji Pet
// ==========================================================================
struct chiji_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, chiji_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( chiji_pet_t* player, util::string_view options_str )
      : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack                    = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  chiji_pet_t( monk_t* owner ) : monk_pet_t( owner, "chiji_the_red_crane", PET_CHIJI,  true, true )
  {
    npc_id                      = 166949;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
    owner_coeff.ap_from_ap      = o()->spec.mistweaver_monk->effectN( 4 ).percent();
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = monk_pet_t::composite_player_multiplier( school );

    return cpm;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";

    monk_pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Yu'lon Pet
// ==========================================================================
struct yulon_pet_t : public monk_pet_t
{
public:
  yulon_pet_t( monk_t* owner ) : monk_pet_t( owner, "yulon_the_jade_serpent", PET_YULON, true, true )
  {
    npc_id                      = 165374;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_ap      = o()->spec.mistweaver_monk->effectN( 4 ).percent();
  }
};

// ==========================================================================
// Fallen Monk - Windwalker (Venthyr)
// ==========================================================================
struct fallen_monk_ww_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, fallen_monk_ww_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
      // TODO: check why this is here
      base_hit -= 0.19;
    }

    // Copy melee code from Storm, Earth and Fire
    double composite_attack_power() const override
    {
      double ap = pet_melee_t::composite_attack_power();
      auto owner = o();

      if ( owner->main_hand_weapon.group() == WEAPON_2H )
      {
        ap += owner->main_hand_weapon.dps * 3.5;
      }
      else
      {
        // 1h/dual wield equation. Note, this formula is slightly off (~3%) for
        // owner dw/pet dw variation.
        double total_dps = owner->main_hand_weapon.dps;
        double dw_mul    = 1.0;
        if ( owner->off_hand_weapon.group() != WEAPON_NONE )
        {
          total_dps += owner->off_hand_weapon.dps * 0.5;
          dw_mul = 0.898882275;
        }

        ap += total_dps * 3.5 * dw_mul;
      }

      return ap;
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_t::impact( s );
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( fallen_monk_ww_pet_t* player, util::string_view options_str )
      : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack                    = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  struct buffs_t
  {
    buff_t* hit_combo_fm_ww     = nullptr;
  } buff;

  fallen_monk_ww_pet_t( monk_t* owner ) :
      monk_pet_t( owner, "fallen_monk_windwalker", PET_FALLEN_MONK, true, true ), buff( buffs_t() )
  {
    npc_id                      = 168033;
    main_hand_weapon.type       = WEAPON_1H;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2 );

    off_hand_weapon.type       = WEAPON_1H;
    off_hand_weapon.min_dmg     = dbc->spell_scaling( o()->type, level() );
    off_hand_weapon.max_dmg     = dbc->spell_scaling( o()->type, level() );
    off_hand_weapon.damage      = ( off_hand_weapon.min_dmg + off_hand_weapon.max_dmg ) / 2;
    off_hand_weapon.swing_time  = timespan_t::from_seconds( 2 );

    switch ( owner->specialization() )
    {
      case MONK_WINDWALKER:
      case MONK_BREWMASTER:
        owner_coeff.ap_from_ap = 0.3333;
        owner_coeff.sp_from_ap = 0.32;
        break;
      case MONK_MISTWEAVER:
      {
        owner_coeff.ap_from_ap = 0.3333;
        owner_coeff.sp_from_sp = 0.3333;
        break;
      }
      default:
        break;
    }
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = o()->cache.player_multiplier( school );

    if ( o()->conduit.imbued_reflections->ok() )
      cpm *= 1 + o()->conduit.imbued_reflections.percent();

    return cpm;
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    monk_pet_t::summon( duration );

    if ( o()->buff.hit_combo->up() )
      buff.hit_combo_fm_ww->trigger( o()->buff.hit_combo->stack() );
  }

  void create_buffs() override
  {
    monk_pet_t::create_buffs();

    buff.hit_combo_fm_ww = make_buff( this, "hit_combo_fo_ww", o()->passives.hit_combo )
                           ->set_default_value_from_effect( 1 )
                           ->set_quiet( true )
                           ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  struct fallen_monk_fists_of_fury_tick_t : public pet_melee_attack_t
  {
    fallen_monk_fists_of_fury_tick_t( fallen_monk_ww_pet_t* p )
      : pet_melee_attack_t( "fists_of_fury_tick_fo", p, p->o()->passives.fallen_monk_fists_of_fury_tick )
    {
      background              = true;
      aoe                     = 1 + (int)o()->passives.fallen_monk_fists_of_fury->effectN( 1 ).base_value();
      attack_power_mod.direct = o()->passives.fallen_monk_fists_of_fury->effectN( 5 ).ap_coeff();
      ap_type                 = attack_power_type::WEAPON_MAINHAND;
      dot_duration            = timespan_t::zero();
      trigger_gcd             = timespan_t::zero();
    }

    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double cam = pet_melee_attack_t::composite_aoe_multiplier( state );

      if ( state->target != target )
        return cam *= o()->passives.fallen_monk_fists_of_fury->effectN( 6 ).percent();

      return cam;
    }

    double action_multiplier() const override
    {
      double am = pet_melee_attack_t::action_multiplier();

      // monk_t* o = static_cast<monk_t*>( player );
      if ( o()->conduit.inner_fury->ok() )
        am *= 1 + o()->conduit.inner_fury.percent();

      return am;
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_attack_t::impact( s );
    }
  };

  struct fallen_monk_fists_of_fury_t : public pet_melee_attack_t
  {
    monk_t* owner;
    fallen_monk_fists_of_fury_t( fallen_monk_ww_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( "fists_of_fury_fo", p, p->o()->passives.fallen_monk_fists_of_fury ), owner( p->o() )
    {
      parse_options( options_str );

      channeled = tick_zero = true;
      interrupt_auto_attack = true;

      attack_power_mod.direct = 0;
      attack_power_mod.tick   = 0;
      weapon_power_mod        = 0;

      // Effect 2 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
      base_tick_time = dot_duration / 4;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      tick_action = new fallen_monk_fists_of_fury_tick_t( p );
    }

    double action_multiplier() const override
    {
      double am = pet_melee_attack_t::action_multiplier();

      return 0;
    }
  };

  struct fallen_monk_tiger_palm_t : public pet_melee_attack_t
  {
    fallen_monk_tiger_palm_t( fallen_monk_ww_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( "tiger_palm_fo", p, p->o()->passives.fallen_monk_tiger_palm )
    {
      parse_options( options_str );

      may_miss = may_block = may_dodge = may_parry = callbacks = false;

      // We only want the monk to cast Tiger Palm 2 times during the duration.
      // Increase the cooldown for non-windwalkers so that it only casts 2 times.
      if ( o()->specialization() == MONK_WINDWALKER )
        cooldown->duration = timespan_t::from_seconds( 2.5 );
      else
        cooldown->duration = timespan_t::from_seconds( 3.1 );
    }

    double cost() const override
    {
      return 0;
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_attack_t::impact( s );
    }
  };

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    // Only cast Fists of Fury for Windwalker specialization
    if ( owner->specialization() == MONK_WINDWALKER )
        action_list_str += "/fists_of_fury";
    action_list_str += "/tiger_palm";

    pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    if ( name == "fists_of_fury" )
      return new fallen_monk_fists_of_fury_t( this, options_str );

    if ( name == "tiger_palm" )
      return new fallen_monk_tiger_palm_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Fallen Monk - Brewmaster (Venthyr)
// ==========================================================================
struct fallen_monk_brm_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, fallen_monk_brm_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_t::impact( s );
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( fallen_monk_brm_pet_t* player, util::string_view options_str )
      : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack                    = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  struct buffs_t
  {
    buff_t* hit_combo_fm_brm    = nullptr;
  } buff;

  fallen_monk_brm_pet_t( monk_t* owner )
    : monk_pet_t( owner, "fallen_monk_brewmaster", PET_FALLEN_MONK, true, true ), buff()
  {
    npc_id                      = 168073;
    main_hand_weapon.type       = WEAPON_2H;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1 );

    switch ( owner->specialization() )
    {
      case MONK_WINDWALKER:
      case MONK_BREWMASTER:
        owner_coeff.ap_from_ap = 0.3333;
        owner_coeff.sp_from_ap = 0.32;
        break;
      case MONK_MISTWEAVER:
      {
        owner_coeff.ap_from_ap = 0.3333;
        owner_coeff.sp_from_sp = 0.3333;
        break;
      }
      default:
        break;
    }
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = o()->cache.player_multiplier( school );

    if ( o()->conduit.imbued_reflections->ok() )
      cpm *= 1 + o()->conduit.imbued_reflections.percent();

    return cpm;
  }

  struct fallen_monk_keg_smash_t : public pet_melee_attack_t
  {
    fallen_monk_keg_smash_t( fallen_monk_brm_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( "keg_smash_fo", p, p->o()->passives.fallen_monk_keg_smash )
    {
      parse_options( options_str );

      aoe                     = -1;
      attack_power_mod.direct = p->o()->passives.fallen_monk_keg_smash->effectN( 2 ).ap_coeff();
      radius                  = p->o()->passives.fallen_monk_keg_smash->effectN( 2 ).radius();

      if ( o()->specialization() == MONK_BREWMASTER )
        cooldown->duration = timespan_t::from_seconds( 6.0 );
      else
        cooldown->duration = timespan_t::from_seconds( 9.0 );
      trigger_gcd             = timespan_t::from_seconds( 1.5 );
    }

    // For more than 5 targets damage is based on a Sqrt(5/x)
    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double cam = pet_melee_attack_t::composite_aoe_multiplier( state );

      if ( state->n_targets > o()->spec.keg_smash->effectN( 7 ).base_value() )
        // this is the closest we can come up without Blizzard flat out giving us the function
        // Primary takes the 100% damage
        // Secondary targets get reduced damage
        if ( state->target != target )
          cam *= std::sqrt( 5 / state->n_targets );

      return cam;
    }

    double action_multiplier() const override
    {
      double am = pet_melee_attack_t::action_multiplier();

      if ( o()->legendary.stormstouts_last_keg->ok() )
        am *= 1 + o()->legendary.stormstouts_last_keg->effectN( 1 ).percent();

      if ( o()->conduit.scalding_brew->ok() )
        {
          if ( o()->get_target_data( player->target )->dots.breath_of_fire->is_ticking() )
            am *= 1 + o()->conduit.scalding_brew.percent();
        }

      return am;
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_attack_t::impact( s );

      o()->get_target_data( s->target )->debuff.fallen_monk_keg_smash->trigger();
    }
  };

  struct fallen_monk_breath_of_fire_t : public pet_spell_t
  {
    struct fallen_monk_breath_of_fire_tick_t : public pet_spell_t
    {
      fallen_monk_breath_of_fire_tick_t( fallen_monk_brm_pet_t* p )
        : pet_spell_t( "breath_of_fire_dot_fo", p, p->o()->passives.breath_of_fire_dot )
      {
        background    = true;
        tick_may_crit = may_crit = true;
        hasted_ticks  = false;
      }

      // Initial damage does Square Root damage
      double composite_aoe_multiplier( const action_state_t* state ) const override
      {
        double cam = pet_spell_t::composite_aoe_multiplier( state );

        if ( state->target != target )
          return cam / std::sqrt( state->n_targets );

        return cam;
      }

      void impact( action_state_t* s ) override
      {
        o()->trigger_empowered_tiger_lightning( s );

        pet_spell_t::impact( s );
      }
    };

    fallen_monk_breath_of_fire_tick_t* dot_action;
    fallen_monk_breath_of_fire_t( fallen_monk_brm_pet_t* p, util::string_view options_str )
      : pet_spell_t( "breath_of_fire_fo", p, p->o()->passives.fallen_monk_breath_of_fire ),
        dot_action( new fallen_monk_breath_of_fire_tick_t( p ) )
    {
      parse_options( options_str );
      cooldown->duration = timespan_t::from_seconds( 9 );
      trigger_gcd        = timespan_t::from_seconds( 2 );

      add_child( dot_action );
    }

    void impact( action_state_t* s ) override
    {
      pet_spell_t::impact( s );

      if ( o()->get_target_data( s->target )->debuff.keg_smash->up() ||
           o()->get_target_data( s->target )->debuff.fallen_monk_keg_smash->up() )
      {
        dot_action->target = s->target;
        dot_action->execute();
      }
    }
  };

  struct fallen_monk_clash_t : public pet_spell_t
  {
    fallen_monk_clash_t( fallen_monk_brm_pet_t* p, util::string_view options_str )
      : pet_spell_t( "clash_fo", p, p->o()->passives.fallen_monk_clash )
    {
      parse_options( options_str );
      gcd_type           = gcd_haste_type::NONE;

      cooldown->duration = timespan_t::from_seconds( 9 );
      trigger_gcd        = timespan_t::from_seconds( 2 );
    }
  };

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/clash";
    action_list_str += "/keg_smash";
    // Only cast Breath of Fire for Brewmaster specialization
    if ( o()->specialization() == MONK_BREWMASTER )
      action_list_str += "/breath_of_fire";

    monk_pet_t::init_action_list();
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    monk_pet_t::summon( duration );

    if ( o()->buff.hit_combo->up() )
      buff.hit_combo_fm_brm->trigger( o()->buff.hit_combo->stack() );
  }

  void create_buffs() override
  {
    monk_pet_t::create_buffs();

    buff.hit_combo_fm_brm = make_buff( this, "hit_combo_fo_brm", o()->passives.hit_combo )
                            ->set_default_value_from_effect( 1 )
                            ->set_quiet( true )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    if ( name == "clash" )
      return new fallen_monk_clash_t( this, options_str );

    if ( name == "keg_smash" )
      return new fallen_monk_keg_smash_t( this, options_str );

    if ( name == "breath_of_fire" )
      return new fallen_monk_breath_of_fire_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Fallen Monk - Mistweaver (Venthyr)
// ==========================================================================
struct fallen_monk_mw_pet_t : public monk_pet_t
{
public:
  fallen_monk_mw_pet_t( monk_t* owner )
    : monk_pet_t( owner, "fallen_monk_mistweaver", PET_FALLEN_MONK, true, true )
  {
    npc_id                      = 168074;
    main_hand_weapon.type       = WEAPON_1H;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1 );

    owner_coeff.sp_from_ap = 0.98;
    owner_coeff.ap_from_ap = 0.98;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = o()->cache.player_multiplier( school );

    if ( o()->conduit.imbued_reflections->ok() )
      cpm *= 1 + o()->conduit.imbued_reflections.percent();

    return cpm;
  }

  struct fallen_monk_enveloping_mist_t : public pet_heal_t
  {
    fallen_monk_enveloping_mist_t( fallen_monk_mw_pet_t* p, util::string_view options_str )
      : pet_heal_t( "enveloping_mist_fo", p, p->o()->passives.fallen_monk_enveloping_mist )
    {
      parse_options( options_str );

      may_miss = false;

      dot_duration = data().duration();
      target       = p->o();
    }

    double cost() const override
    {
      return 0;
    }
  };

  struct fallen_monk_soothing_mist_t : public pet_heal_t
  {
    fallen_monk_soothing_mist_t( fallen_monk_mw_pet_t* p, util::string_view options_str )
      : pet_heal_t( "soothing_mist_fo", p, p->o()->passives.fallen_monk_soothing_mist )
    {
      parse_options( options_str );

      may_miss = false;
      channeled = tick_zero = true;
      interrupt_auto_attack = true;

      dot_duration       = data().duration();
      trigger_gcd        = timespan_t::from_millis( 750 );
      cooldown->duration = timespan_t::from_seconds( 5 );
      cooldown->hasted   = true;
      target             = p->o();
    }
  };

  void init_action_list() override
  {
    action_list_str = "";
    // Only cast Enveloping Mist for Mistweaver specialization
    if ( o()->specialization() == MONK_MISTWEAVER )
      action_list_str += "/enveloping_mist";
    action_list_str += "/soothing_mist";

    monk_pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "enveloping_mist" )
      return new fallen_monk_enveloping_mist_t( this, options_str );

    if ( name == "soothing_mist" )
      return new fallen_monk_soothing_mist_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }
};
}  // end namespace pets

namespace actions
{
// ==========================================================================
// Monk Abilities
// ==========================================================================
// Template for common monk action code. See priest_action_t.

template <class Base>
struct monk_action_t : public Base
{
  sef_ability_e sef_ability;
  bool ww_mastery;
  bool may_combo_strike;
  bool trigger_chiji;

  // Affect flags for various dynamic effects
  struct
  {
    bool serenity;
  } affected_by;

private:
  std::array<resource_e, MONK_MISTWEAVER + 1> _resource_by_stance;
  using ab = Base;  // action base, eg. spell_t
public:
  using base_t = monk_action_t<Base>;

  monk_action_t( util::string_view n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ), sef_ability( SEF_NONE ), ww_mastery( false ), may_combo_strike( false ), trigger_chiji( false ), affected_by()
  {
    ab::may_crit = true;
    range::fill( _resource_by_stance, RESOURCE_MAX );
  }

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
    return p()->get_target_data( t );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    if ( name_str == "combo_strike" )
      return make_mem_fn_expr( name_str, *this, &monk_action_t::is_combo_strike );
    else if ( name_str == "combo_break" )
      return make_mem_fn_expr( name_str, *this, &monk_action_t::is_combo_break );
    return ab::create_expression( name_str );
  }

  bool ready() override
  {
    return ab::ready();
  }

  void init() override
  {
    ab::init();

    /* Iterate through power entries, and find if there are resources linked to one of our stances
     */
    for ( const spellpower_data_t& pd : ab::data().powers() )
    {
      switch ( pd.aura_id() )
      {
        case 137023:
          assert( _resource_by_stance[ dbc::spec_idx( MONK_BREWMASTER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ dbc::spec_idx( MONK_BREWMASTER ) ] = pd.resource();
          break;
        case 137024:
          assert( _resource_by_stance[ dbc::spec_idx( MONK_MISTWEAVER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ dbc::spec_idx( MONK_MISTWEAVER ) ] = pd.resource();
          break;
        case 137025:
          assert( _resource_by_stance[ dbc::spec_idx( MONK_WINDWALKER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ dbc::spec_idx( MONK_WINDWALKER ) ] = pd.resource();
          break;
        default:
          break;
      }
    }
  }

  void reset_swing()
  {
    if ( p()->main_hand_attack && p()->main_hand_attack->execute_event )
    {
      p()->main_hand_attack->cancel();
      p()->main_hand_attack->schedule_execute();
    }
    if ( p()->off_hand_attack && p()->off_hand_attack->execute_event )
    {
      p()->off_hand_attack->cancel();
      p()->off_hand_attack->schedule_execute();
    }
  }

  resource_e current_resource() const override
  {
    if ( p()->specialization() == SPEC_NONE )
    {
      return ab::current_resource();
    }

    resource_e resource_by_stance = _resource_by_stance[ dbc::spec_idx( p()->specialization() ) ];

    if ( resource_by_stance == RESOURCE_MAX )
      return ab::current_resource();

    return resource_by_stance;
  }

  // Check if the combo ability under consideration is different from the last
  bool is_combo_strike()
  {
    if ( !may_combo_strike )
      return false;

    // We don't know if the first attack is a combo or not, so assume it
    // is. If you change this, also change is_combo_break so that it
    // doesn't combo break on the first attack.
    if ( p()->combo_strike_actions.empty() )
      return true;

    if ( p()->combo_strike_actions.back()->id != this->id )
      return true;

    return false;
  }

  // This differs from combo_strike when the ability can't combo strike. In
  // that case both is_combo_strike and is_combo_break are false.
  bool is_combo_break()
  {
    if ( !may_combo_strike )
      return false;

    return !is_combo_strike();
  }

  // Trigger Windwalker's Combo Strike Mastery, the Hit Combo talent,
  // and other effects that trigger from combo strikes.
  // Triggers from execute() on abilities with may_combo_strike = true
  // Side effect: modifies combo_strike_actions
  void combo_strikes_trigger()
  {
    if ( !p()->mastery.combo_strikes->ok() )
      return;

    if ( is_combo_strike() )
    {
      p()->buff.combo_strikes->trigger();

      if ( p()->talent.hit_combo->ok() )
        p()->buff.hit_combo->trigger();

      if ( p()->conduit.xuens_bond->ok() )
        p()->cooldown.invoke_xuen->adjust( p()->conduit.xuens_bond->effectN( 2 ).time_value(), true ); // Saved as -100
    }
    else
    {
      p()->combo_strike_actions.clear();
      p()->buff.combo_strikes->expire();
      p()->buff.hit_combo->expire();
    }

    // Record the current action in the history.
    p()->combo_strike_actions.push_back( this );
  }

  // Reduces Brewmaster Brew cooldowns by the time given
  void brew_cooldown_reduction( double time_reduction )
  {
    // we need to adjust the cooldown time DOWNWARD instead of UPWARD so multiply the time_reduction by -1
    time_reduction *= -1;

    if ( p()->cooldown.purifying_brew->down() )
      p()->cooldown.purifying_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->cooldown.celestial_brew->down() )
      p()->cooldown.celestial_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->cooldown.fortifying_brew->down() )
      p()->cooldown.fortifying_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->cooldown.black_ox_brew->down() )
      p()->cooldown.black_ox_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->covenant.necrolord->ok() && p()->cooldown.bonedust_brew->down() )
      p()->cooldown.bonedust_brew->adjust( timespan_t::from_seconds( time_reduction ), true );
  }

  void trigger_shuffle( double time_extension )
  {
    if ( p()->specialization() == MONK_BREWMASTER && p()->spec.shuffle->ok() )
    {
      timespan_t base_time = timespan_t::from_seconds( time_extension );
      if ( p()->buff.shuffle->up() )
      {
        timespan_t max_time   = p()->buff.shuffle->buff_duration();
        timespan_t new_length = std::min( max_time, base_time + p()->buff.shuffle->remains() );
        p()->buff.shuffle->refresh_duration( new_length );
      }
      else
      {
        p()->buff.shuffle->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, base_time );
      }

      if ( p()->conduit.walk_with_the_ox->ok() && p()->cooldown.invoke_niuzao->down() )
        p()->cooldown.invoke_niuzao->adjust( p()->conduit.walk_with_the_ox->effectN( 2 ).time_value(), true );
    }
  }

  double cost() const override
  {
    double c = ab::cost();

    if ( c == 0 )
      return c;

    c *= 1.0 + cost_reduction();
    if ( c < 0 )
      c = 0;

    return c;
  }

  virtual double cost_reduction() const
  {
    double c = 0.0;

    if ( p()->buff.mana_tea->up() && ab::data().affected_by( p()->talent.mana_tea->effectN( 1 ) ) )
      c += p()->buff.mana_tea->value();  // saved as -50%

    else if ( p()->buff.serenity->up() && ab::data().affected_by( p()->talent.serenity->effectN( 1 ) ) )
      c += p()->talent.serenity->effectN( 1 ).percent();  // Saved as -100

    else if ( p()->buff.bok_proc->up() && ab::data().affected_by( p()->passives.bok_proc->effectN( 1 ) ) )
      c += p()->passives.bok_proc->effectN( 1 ).percent();  // Saved as -100

    return c;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = ab::recharge_multiplier( cd );

    // Update the cooldown while Serenity is active
    if ( p()->buff.serenity->up() && current_resource() == RESOURCE_CHI && ab::cost() > 0 )
    {
      rm *= 1.0 / ( 1 + p()->talent.serenity->effectN( 4 ).percent() );
    }

    return rm;
  }

  void consume_resource() override
  {
    ab::consume_resource();

    if ( !ab::execute_state )  // Fixes rare crashes at combat_end.
      return;

    if ( current_resource() == RESOURCE_CHI )
    {
      if ( ab::cost() > 0 )
      {
        if ( p()->talent.inner_strength->ok() )
          p()->buff.inner_stength->trigger( (int)ab::cost() );

        if ( p()->talent.spirtual_focus->ok() )
        {
          p()->spiritual_focus_count += ab::cost();

          if ( p()->spiritual_focus_count >= p()->talent.spirtual_focus->effectN( 1 ).base_value() )
          {
            p()->cooldown.storm_earth_and_fire->adjust( -1 * p()->talent.spirtual_focus->effectN( 2 ).time_value() );
            p()->spiritual_focus_count -= p()->talent.spirtual_focus->effectN( 1 ).base_value();
          }
        }
      }

      if ( p()->legendary.last_emperors_capacitor->ok() )
        p()->buff.the_emperors_capacitor->trigger();

      // Chi Savings on Dodge & Parry & Miss
      if ( ab::last_resource_cost > 0 )
      {
        double chi_restored = ab::last_resource_cost;
        if ( !ab::aoe && ab::result_is_miss( ab::execute_state->result ) )
          p()->resource_gain( RESOURCE_CHI, chi_restored, p()->gain.chi_refund );
      }
    }

    // Energy refund, estimated at 80%
    if ( current_resource() == RESOURCE_ENERGY && ab::last_resource_cost > 0 && !ab::hit_any_target )
    {
      double energy_restored = ab::last_resource_cost * 0.8;

      p()->resource_gain( RESOURCE_ENERGY, energy_restored, p()->gain.energy_refund );
    }
  }

  void execute() override
  {
    if ( may_combo_strike )
      combo_strikes_trigger();

    if ( trigger_chiji && p()->buff.invoke_chiji->up() )
      p()->buff.invoke_chiji_evm->trigger();

    ab::execute();

    trigger_storm_earth_and_fire( this );

    if ( p()->buff.faeline_stomp->up() && ab::background == false &&
         p()->rng().roll( p()->user_options.faeline_stomp_uptime ) )
      if ( p()->rng().roll( p()->buff.faeline_stomp->value() ) )
        p()->cooldown.faeline_stomp->reset( true, 1 );

  }

  void impact( action_state_t* s ) override
  {
    if ( s->action->school == SCHOOL_PHYSICAL )
    {
      trigger_mystic_touch( s );
    }

    // Don't want to cause the buff to be cast and then used up immediately.
    if ( current_resource() == RESOURCE_CHI )
    {
      // Bug: Dance of Chi-Ji cannot proc during Serenity
      if ( !p()->bugs )
      {
        // Dance of Chi-Ji talent triggers from spending chi
        if ( p()->talent.dance_of_chiji->ok() )
          p()->buff.dance_of_chiji->trigger();
      }
      else if ( !p()->buff.serenity->up() )
      {
        // Dance of Chi-Ji talent triggers from spending chi
        if ( p()->talent.dance_of_chiji->ok() )
          p()->buff.dance_of_chiji->trigger();
      }
    }

    p()->trigger_empowered_tiger_lightning( s );

    p()->trigger_bonedust_brew( s );

    ab::impact( s );
  }

  void trigger_storm_earth_and_fire( const action_t* a )
  {
    if ( !p()->spec.storm_earth_and_fire->ok() )
    {
      return;
    }

    if ( sef_ability == SEF_NONE )
    {
      return;
    }

    if ( !p()->buff.storm_earth_and_fire->up() )
    {
      return;
    }

    p()->pets.sef[ SEF_EARTH ]->trigger_attack( sef_ability, a );
    p()->pets.sef[ SEF_FIRE ]->trigger_attack( sef_ability, a );
    // Trigger pet retargeting if sticky target is not defined, and the Monk used one of the Cyclone
    // Strike triggering abilities
    if ( !p()->pets.sef[ SEF_EARTH ]->sticky_target &&
         ( sef_ability == SEF_TIGER_PALM || sef_ability == SEF_BLACKOUT_KICK || sef_ability == SEF_RISING_SUN_KICK ) )
    {
      p()->retarget_storm_earth_and_fire_pets();
    }
  }

  void trigger_mystic_touch( action_state_t* s )
  {
    if ( ab::sim->overrides.mystic_touch )
    {
      return;
    }

    if ( ab::result_is_miss( s->result ) )
    {
      return;
    }

    if ( s->result_amount == 0.0 )
    {
      return;
    }

    if ( s->target->debuffs.mystic_touch && p()->spec.mystic_touch->ok() )
    {
      s->target->debuffs.mystic_touch->trigger();
    }
  }
};

struct monk_spell_t : public monk_action_t<spell_t>
{
  monk_spell_t( util::string_view n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s )
  {
    ap_type = attack_power_type::WEAPON_MAINHAND;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = base_t::composite_target_multiplier( t );

    if ( td( t )->debuff.weapons_of_order->up() )
      m *= 1 + td( t )->debuff.weapons_of_order->stack_value();

    return m;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = base_t::composite_persistent_multiplier( action_state );

    if ( ww_mastery && p()->buff.combo_strikes->up() )
      pm *= 1 + p()->cache.mastery_value();

    return pm;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( p()->buff.storm_earth_and_fire->up() )
    {
      if ( base_t::data().affected_by( p()->spec.storm_earth_and_fire->effectN( 1 ) ) )
      {
        double sef_multiplier = p()->spec.storm_earth_and_fire->effectN( 1 ).percent();

        am *= 1 + sef_multiplier;
      }
    }

    if ( p()->buff.serenity->up() )
    {
      if ( base_t::data().affected_by( p()->talent.serenity->effectN( 2 ) ) )
      {
        double serenity_multiplier = p()->talent.serenity->effectN( 2 ).percent();

        if ( p()->conduit.coordinated_offensive->ok() )
          serenity_multiplier += p()->conduit.coordinated_offensive.percent();

        am *= 1 + serenity_multiplier;
      }
    }

    if ( p()->buff.hit_combo->up() )
    {
      if ( base_t::data().affected_by( p()->passives.hit_combo->effectN( 1 ) ) )
        am *= 1 + p()->buff.hit_combo->stack_value();
    }

    return am;
  }
};

struct monk_heal_t : public monk_action_t<heal_t>
{
  monk_heal_t( util::string_view n, monk_t& p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, &p, s )
  {
    harmful = false;
    ap_type = attack_power_type::WEAPON_MAINHAND;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = base_t::composite_target_multiplier( target );

    return m;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( p()->specialization() == MONK_MISTWEAVER )
    {
      player_t* t = ( execute_state ) ? execute_state->target : target;

      if ( td( t )->dots.enveloping_mist->is_ticking() )
      {
        if ( p()->talent.mist_wrap->ok() )
          am *= 1.0 + p()->spec.enveloping_mist->effectN( 2 ).percent() + p()->talent.mist_wrap->effectN( 2 ).percent();
        else
          am *= 1.0 + p()->spec.enveloping_mist->effectN( 2 ).percent();
      }

      if ( p()->buff.life_cocoon->up() )
        am *= 1.0 + p()->spec.life_cocoon->effectN( 2 ).percent();
    }

    if ( p()->buff.storm_earth_and_fire->up() )
    {
      if ( base_t::data().affected_by( p()->spec.storm_earth_and_fire->effectN( 1 ) ) )
        am *= 1 + p()->spec.storm_earth_and_fire->effectN( 1 ).percent();
    }

    if ( p()->buff.serenity->up() )
    {
      if ( base_t::data().affected_by( p()->talent.serenity->effectN( 2 ) ) )
        am *= 1 + p()->talent.serenity->effectN( 2 ).percent();
    }

    if ( p()->buff.hit_combo->up() )
    {
      if ( base_t::data().affected_by( p()->passives.hit_combo->effectN( 1 ) ) )
        am *= 1 + p()->buff.hit_combo->stack_value();
    }

    return am;
  }
};

struct monk_absorb_t : public monk_action_t<absorb_t>
{
  monk_absorb_t( const std::string& n, monk_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, &player, s )
  {
  }
};

struct monk_snapshot_stats_t : public snapshot_stats_t
{
  monk_snapshot_stats_t( monk_t* player, const std::string& options ) : snapshot_stats_t( player, options )
  {
  }

  void execute() override
  {
    snapshot_stats_t::execute();

    monk_t* monk = debug_cast<monk_t*>( player );

    monk->sample_datas.buffed_stagger_base             = monk->stagger_base_value();
    monk->sample_datas.buffed_stagger_pct_player_level = monk->stagger_pct( player->level() );
    monk->sample_datas.buffed_stagger_pct_target_level = monk->stagger_pct( target->level() );
  }
};

namespace pet_summon
{
struct summon_pet_t : public monk_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pet_t* pet;

public:
  summon_pet_t( util::string_view n, util::string_view pname, monk_t* p,
                const spell_data_t* sd = spell_data_t::nil() )
    : monk_spell_t( n, p, sd ), summoning_duration( timespan_t::zero() ), pet_name( pname ), pet( nullptr )
  {
    harmful = false;
  }

  void init_finished() override
  {
    pet = player->find_pet( pet_name );
    if ( !pet )
    {
      background = true;
    }

    monk_spell_t::init_finished();
  }

  void execute() override
  {
    pet->summon( summoning_duration );

    monk_spell_t::execute();
  }

  bool ready() override
  {
    if ( !pet )
    {
      return false;
    }
    return monk_spell_t::ready();
  }
};

// ==========================================================================
// Storm, Earth, and Fire
// ==========================================================================

struct storm_earth_and_fire_t : public monk_spell_t
{
  storm_earth_and_fire_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "storm_earth_and_fire", p, p->spec.storm_earth_and_fire )
  {
    parse_options( options_str );

    // Forcing the minimum GCD to 750 milliseconds
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_type  = gcd_haste_type::ATTACK_HASTE;
    callbacks = harmful = may_miss = may_crit = may_dodge = may_parry = may_block = false;

    cooldown->charges += (int)p->spec.storm_earth_and_fire_2->effectN( 1 ).base_value();
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    // While pets are up, don't trigger cooldown since the sticky targeting does not consume charges
    if ( p()->buff.storm_earth_and_fire->check() )
    {
      cd_duration = timespan_t::zero();
    }

    monk_spell_t::update_ready( cd_duration );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Don't let user needlessly trigger SEF sticky targeting mode, if the user would just be
    // triggering it on the same sticky target
    if ( p()->buff.storm_earth_and_fire->check() &&
         ( p()->pets.sef[ SEF_EARTH ]->sticky_target && candidate_target == p()->pets.sef[ SEF_EARTH ]->target ) )
    {
      return false;
    }

    return monk_spell_t::target_ready( candidate_target );
  }

  bool ready() override
  {
    if ( p()->talent.serenity->ok() )
      return false;

//    if ( p()->buff.storm_earth_and_fire->up() && (p()->pets.sef[ SEF_EARTH ]->sticky_target == false || p()->pets.sef[ SEF_FIRE ]->sticky_target == false))
//      return true;

    return monk_spell_t::ready();
  }

  // Monk used SEF while pets are up to sticky target them into an enemy
  void sticky_targeting()
  {
    sim->print_debug( "{} storm_earth_and_fire sticky target {} to {} (old={})", *player,
                             *p()->pets.sef[ SEF_EARTH ], *target,
                             *p()->pets.sef[ SEF_EARTH ]->target );

    p()->pets.sef[ SEF_EARTH ]->target        = target;
    p()->pets.sef[ SEF_EARTH ]->sticky_target = true;

    sim ->print_debug( "{} storm_earth_and_fire sticky target {} to {} (old={})", *player,
                             *p()->pets.sef[ SEF_FIRE ], *target,
                             *p()->pets.sef[ SEF_FIRE ]->target );

    p()->pets.sef[ SEF_FIRE ]->target        = target;
    p()->pets.sef[ SEF_FIRE ]->sticky_target = true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( !p()->buff.storm_earth_and_fire->check() )
    {
      p()->summon_storm_earth_and_fire( data().duration() );
    }
    else
    {
      sticky_targeting();
    }
  }
};

struct storm_earth_and_fire_fixate_t : public monk_spell_t
{
  storm_earth_and_fire_fixate_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "storm_earth_and_fire_fixate", p, p->spec.storm_earth_and_fire->effectN( 5 ).trigger() )
  {
    parse_options( options_str );

    callbacks = false;
    trigger_gcd = timespan_t::zero();
    cooldown->duration = timespan_t::zero();
  }

  bool ready() override
  {
    if ( p()->buff.storm_earth_and_fire->up() )
    {
      if ( p()->pets.sef[ SEF_EARTH ]->sticky_target == true || p()->pets.sef[ SEF_FIRE ]->sticky_target == true ) 
      {
        if (p()->pets.sef[ SEF_EARTH ]->target != target )
          return true;
        else if ( p()->pets.sef[ SEF_FIRE ]->target != target )
          return true;
      } 
      else if ( p()->pets.sef[ SEF_EARTH ]->sticky_target == false ||
                p()->pets.sef[ SEF_FIRE ]->sticky_target == false )
        return true;
    }

    return false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    sim->print_debug( "{} storm_earth_and_fire sticky target {} to {} (old={})", *player,
                      *p()->pets.sef[ SEF_EARTH ], *target, *p()->pets.sef[ SEF_EARTH ]->target );

    p()->pets.sef[ SEF_EARTH ]->target        = target;
    p()->pets.sef[ SEF_EARTH ]->sticky_target = true;

    sim->print_debug( "{} storm_earth_and_fire sticky target {} to {} (old={})", *player,
                      *p()->pets.sef[ SEF_FIRE ], *target, *p()->pets.sef[ SEF_FIRE ]->target );

    p()->pets.sef[ SEF_FIRE ]->target        = target;
    p()->pets.sef[ SEF_FIRE ]->sticky_target = true;
  }
};

// Callback to retarget Storm Earth and Fire pets when new target appear, or old targets depsawn
// (i.e., die).
struct sef_despawn_cb_t
{
  monk_t* monk;

  sef_despawn_cb_t( monk_t* m ) : monk( m )
  {
  }

  void operator()( player_t* )
  {
    // No pets up, don't do anything
    if ( !monk->buff.storm_earth_and_fire->check() )
    {
      return;
    }

    auto targets   = monk->create_storm_earth_and_fire_target_list();
    auto n_targets = targets.size();

    // If the active clone's target is sleeping, reset it's targeting, and jump it to a new target.
    // Note that if sticky targeting is used, both targets will jump (since both are going to be
    // stickied to the dead target)
    range::for_each( monk->pets.sef, [this, &targets, &n_targets]( pets::storm_earth_and_fire_pet_t* pet ) {
      // Arise time went negative, so the target is sleeping. Can't check "is_sleeping" here, because
      // the callback is called before the target goes to sleep.
      if ( pet->target->arise_time < timespan_t::zero() )
      {
        pet->reset_targeting();
        monk->retarget_storm_earth_and_fire( pet, targets, n_targets );
      }
      else
      {
        // Retarget pets otherwise (a new target has appeared). Note that if the pets are sticky
        // targeted, this will do nothing.
        monk->retarget_storm_earth_and_fire( pet, targets, n_targets );
      }
    } );
  }
};
}  // namespace pet_summon

namespace attacks
{
struct monk_melee_attack_t : public monk_action_t<melee_attack_t>
{
  weapon_t* mh;
  weapon_t* oh;

  monk_melee_attack_t( util::string_view n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s ), mh( nullptr ), oh( nullptr )
  {
    special    = true;
    may_glance = false;
  }

  void init_finished() override
  {
    if ( affected_by.serenity )
    {
      auto cooldowns = p()->serenity_cooldowns;
      if ( std::find( cooldowns.begin(), cooldowns.end(), cooldown ) == cooldowns.end() )
        p()->serenity_cooldowns.push_back( cooldown );
    }

    base_t::init_finished();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = base_t::composite_target_multiplier( t );

    if ( td( t )->debuff.weapons_of_order->up() )
      m *= 1 + td( t )->debuff.weapons_of_order->stack_value();

    return m;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = player->composite_player_target_crit_chance( target );

    if ( td( target )->debuff.keefers_skyreach->up() )
      c += td( target )->debuff.keefers_skyreach->value();

    return c;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = base_t::composite_persistent_multiplier( action_state );

    return pm;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( ww_mastery && p()->buff.combo_strikes->up() )
      am *= 1 + p()->cache.mastery_value();

    if ( p()->buff.storm_earth_and_fire->up() )
    {
      if ( base_t::data().affected_by( p()->spec.storm_earth_and_fire->effectN( 1 ) ) )
        am *= 1 + p()->spec.storm_earth_and_fire->effectN( 1 ).percent();
    }

    if ( p()->buff.serenity->up() )
    {
      if ( base_t::data().affected_by( p()->talent.serenity->effectN( 2 ) ) )
        am *= 1 + p()->talent.serenity->effectN( 2 ).percent();
    }

    if ( p()->buff.hit_combo->up() )
    {
      if ( base_t::data().affected_by( p()->passives.hit_combo->effectN( 1 ) ) )
        am *= 1 + p()->buff.hit_combo->stack_value();
    }

    // Increases just physical damage
    if ( p()->buff.touch_of_death->up() )
      am *= 1 + p()->buff.touch_of_death->value();

    return am;
  }

  // Physical tick_action abilities need amount_type() override, so the
  // tick_action are properly physically mitigated.
  result_amount_type amount_type( const action_state_t* state, bool periodic ) const override
  {
    if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
    {
      return result_amount_type::DMG_DIRECT;
    }
    else
    {
      return base_t::amount_type( state, periodic );
    }
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !sim->overrides.mystic_touch && s->action->result_is_hit( s->result ) && p()->passives.mystic_touch->ok() &&
         s->result_amount > 0.0 )
    {
      s->target->debuffs.mystic_touch->trigger();
    }
  }
};

// ==========================================================================
// Windwalking Aura Toggle
// ==========================================================================

struct windwalking_aura_t : public monk_spell_t
{
  windwalking_aura_t( monk_t* player ) : monk_spell_t( "windwalking_aura_toggle", player )
  {
    harmful     = false;
    background  = true;
    trigger_gcd = timespan_t::zero();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    tl.clear();

    for ( size_t i = 0, actors = sim->player_non_sleeping_list.size(); i < actors; i++ )
    {
      player_t* t = sim->player_non_sleeping_list[ i ];
      tl.push_back( t );
    }

    return tl.size();
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    size_t i = tl.size();
    while ( i > 0 )
    {
      i--;
      player_t* target_to_buff = tl[ i ];

      if ( p()->get_player_distance( *target_to_buff ) > 10.0 )
        tl.erase( tl.begin() + i );
    }

    return tl;
  }
};

// ==========================================================================
// Sunrise Technique
// ==========================================================================
struct sunrise_technique_t : public monk_melee_attack_t
{
  sunrise_technique_t( monk_t* p ) : monk_melee_attack_t( "sunrise_technique", p, p->find_spell( 275673 ) )
  {
    background  = true;
    may_crit    = true;
    trigger_gcd = timespan_t::zero();
    min_gcd     = timespan_t::zero();
  }
};

// ==========================================================================
// Tiger Palm
// ==========================================================================

// Eye of the Tiger ========================================================
struct eye_of_the_tiger_heal_tick_t : public monk_heal_t
{
  eye_of_the_tiger_heal_tick_t( monk_t& p, util::string_view name )
    : monk_heal_t( name, p, p.talent.eye_of_the_tiger->effectN( 1 ).trigger() )
  {
    background   = true;
    hasted_ticks = false;
    may_crit = tick_may_crit = true;
    target                   = player;
  }
};

struct eye_of_the_tiger_dmg_tick_t : public monk_spell_t
{
  eye_of_the_tiger_dmg_tick_t( monk_t* player, util::string_view name )
    : monk_spell_t( name, player, player->talent.eye_of_the_tiger->effectN( 1 ).trigger() )
  {
    background   = true;
    hasted_ticks = false;
    may_crit = tick_may_crit = true;
    attack_power_mod.direct  = 0;
    attack_power_mod.tick    = data().effectN( 2 ).ap_coeff();
  }
};

// Tiger Palm base ability ===================================================
struct tiger_palm_t : public monk_melee_attack_t
{
  heal_t* eye_of_the_tiger_heal;
  spell_t* eye_of_the_tiger_damage;
  bool shaohoas_might;

  tiger_palm_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "tiger_palm", p, p->spec.tiger_palm ),
      eye_of_the_tiger_heal( new eye_of_the_tiger_heal_tick_t( *p, "eye_of_the_tiger_heal" ) ),
      eye_of_the_tiger_damage( new eye_of_the_tiger_dmg_tick_t( p, "eye_of_the_tiger_damage" ) ),
      shaohoas_might( false )
  {
    parse_options( options_str );

    ww_mastery                    = true;
    may_combo_strike              = true;
    trigger_chiji                 = true;
    sef_ability                   = SEF_TIGER_PALM;

    add_child( eye_of_the_tiger_damage );
    add_child( eye_of_the_tiger_heal );

    if ( p->specialization() == MONK_WINDWALKER )
      energize_amount = p->spec.windwalker_monk->effectN( 4 ).base_value();
    else
      energize_type = action_energize::NONE;

    spell_power_mod.direct = 0.0;

    if ( p->legendary.keefers_skyreach->ok() )
      this->range += p->legendary.keefers_skyreach->effectN( 1 ).base_value();
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->buff.blackout_combo->check() )
      am *= 1 + p()->buff.blackout_combo->data().effectN( 1 ).percent();

    if ( shaohoas_might )
      am *= 1 + p()->passives.face_palm->effectN( 1 ).percent();

    return am;
  }

  void execute() override
  {
    if ( p()->legendary.shaohaos_might->ok() && rng().roll( p()->legendary.shaohaos_might->effectN( 1 ).percent() ) )
      shaohoas_might = true;

    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    if ( p()->talent.eye_of_the_tiger->ok() )
    {
      eye_of_the_tiger_damage->execute();
      eye_of_the_tiger_heal->execute();
    }

    switch ( p()->specialization() )
    {
      case MONK_MISTWEAVER:
      {
        p()->buff.teachings_of_the_monastery->trigger();
        break;
      }
      case MONK_WINDWALKER:
      {
        // Combo Breaker calculation
        if ( p()->spec.combo_breaker->ok() && p()->buff.bok_proc->trigger() )
        {
          p()->proc.bok_proc->occur();

          if ( p()->buff.storm_earth_and_fire->up() )
          {
            p()->pets.sef[ SEF_FIRE ]->buff.bok_proc_sef->trigger();
            p()->pets.sef[ SEF_EARTH ]->buff.bok_proc_sef->trigger();
          }
        }
        break;
      }
      case MONK_BREWMASTER:
      {
        if ( p()->talent.spitfire->ok() )
        {
          if ( rng().roll( p()->talent.spitfire->proc_chance() ) )
          {
            p()->cooldown.breath_of_fire->reset( true );
            p()->buff.spitfire->trigger();
          }
        }

        // Reduces the remaining cooldown on your Brews by 1 sec
        brew_cooldown_reduction( p()->spec.tiger_palm->effectN( 3 ).base_value() );

        if ( p()->buff.blackout_combo->up() )
          p()->buff.blackout_combo->expire();

        if ( shaohoas_might )
          brew_cooldown_reduction( p()->passives.face_palm->effectN( 2 ).base_value() );
        break;
      }
      default:
        break;
    }
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    // Apply Mark of the Crane
    if ( p()->specialization() == MONK_WINDWALKER && result_is_hit( s->result ) && p()->spec.spinning_crane_kick_2_ww->ok() )
      p()->trigger_mark_of_the_crane( s );

    // Bonedust Brew
    if ( p()->specialization() == MONK_BREWMASTER && td( s->target )->debuff.bonedust_brew->up() )
      brew_cooldown_reduction( p()->covenant.necrolord->effectN( 3 ).base_value() );

    if ( p()->legendary.keefers_skyreach->ok() )
    {
      if ( !td( s->target )->debuff.recently_rushing_tiger_palm->up() )
      {
        td( s->target )->debuff.keefers_skyreach->trigger();
        td( s->target )->debuff.recently_rushing_tiger_palm->trigger();
      }
    }
    shaohoas_might = false;
  }
};

// ==========================================================================
// Rising Sun Kick
// ==========================================================================

// Rising Sun Kick Damage Trigger ===========================================

struct rising_sun_kick_dmg_t : public monk_melee_attack_t
{
  rising_sun_kick_dmg_t( monk_t* p, const std::string& name )
    : monk_melee_attack_t( name, p, p->spec.rising_sun_kick->effectN( 1 ).trigger() )
  {
    ww_mastery = true;

    background = dual             = true;
    may_crit                      = true;
    trigger_chiji                 = true;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    // Rank 2 seems to be applied after Bonus Damage. Hence the reason for being in the Action Multiplier
    if ( p()->spec.rising_sun_kick_2->ok() )
      am *= 1 + p()->spec.rising_sun_kick_2->effectN( 1 ).percent();

    return am;
  }

  double composite_crit_chance() const override
  {
    double c = monk_melee_attack_t::composite_crit_chance();

    if ( p()->buff.pressure_point->up() )
      c += p()->buff.pressure_point->value();

    return c;
  }

  void init() override
  {
    monk_melee_attack_t::init();

    if ( p()->specialization() == MONK_WINDWALKER )
      ap_type = attack_power_type::WEAPON_BOTH;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    switch ( p()->specialization() )
    {
      case MONK_MISTWEAVER:
      {
        if ( p()->buff.thunder_focus_tea->up() )
        {
          if ( p()->spec.thunder_focus_tea_2->ok() )
            p()->cooldown.rising_sun_kick->adjust( p()->spec.thunder_focus_tea_2->effectN( 1 ).time_value(), true );

          p()->buff.thunder_focus_tea->decrement();
          break;
        }
      }
      default:
        break;
    }
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p()->buff.teachings_of_the_monastery->up() )
      {
        p()->buff.teachings_of_the_monastery->expire();
        // Spirit of the Crane does not have a buff associated with it. Since
        // this is tied somewhat with Teachings of the Monastery, tacking
        // this onto the removal of that buff.
        if ( p()->talent.spirit_of_the_crane->ok() )
          p()->resource_gain(
              RESOURCE_MANA,
              ( p()->resources.max[ RESOURCE_MANA ] * p()->passives.spirit_of_the_crane->effectN( 1 ).percent() ),
              p()->gain.spirit_of_the_crane );
      }

      // Apply Mortal Wonds
      if ( p()->specialization() == MONK_WINDWALKER )
      {
        if ( s->target->debuffs.mortal_wounds )
        {
          s->target->debuffs.mortal_wounds->trigger();
        }

        if ( p()->legendary.xuens_treasure->ok() && ( s->result == RESULT_CRIT ) )
          p()->cooldown.fists_of_fury->adjust( -1 * p()->legendary.xuens_treasure->effectN( 2 ).time_value() );

        // Apply Mark of the Crane
        if ( p()->spec.spinning_crane_kick_2_ww->ok() )
          p()->trigger_mark_of_the_crane( s );
      }
    }
  }
};

struct rising_sun_kick_t : public monk_melee_attack_t
{
  rising_sun_kick_dmg_t* trigger_attack;

  rising_sun_kick_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "rising_sun_kick", p, p->spec.rising_sun_kick )
  {
    parse_options( options_str );

    may_combo_strike     = true;
    sef_ability          = SEF_RISING_SUN_KICK;
    affected_by.serenity = true;

    attack_power_mod.direct = 0;

    trigger_attack        = new rising_sun_kick_dmg_t( p, "rising_sun_kick_dmg" );
    trigger_attack->stats = stats;
  }

  void init() override
  {
    monk_melee_attack_t::init();

    ap_type = attack_power_type::NONE;
  }

  virtual double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->buff.weapons_of_order_ww->up() )
      c += p()->buff.weapons_of_order_ww->value();  // saved as -1

    return c;
  }

  void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    // Register how much chi is saved without actually refunding the chi
    if ( p()->buff.serenity->up() )
    {
      if ( p()->buff.weapons_of_order_ww->up() )
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] - p()->buff.weapons_of_order_ww->value() );
      else
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
    }

  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    trigger_attack->set_target( target );
    trigger_attack->execute();

    if ( p()->talent.whirling_dragon_punch->ok() && p()->cooldown.fists_of_fury->down() )
    {
        if ( this->cooldown_duration() <= p()->cooldown.fists_of_fury->remains() )
            p()->buff.whirling_dragon_punch->set_duration( this->cooldown_duration() );
        else
          p()->buff.whirling_dragon_punch->set_duration( p()->cooldown.fists_of_fury->remains() );
        p()->buff.whirling_dragon_punch->trigger();
    }

    if ( p()->specialization() == MONK_WINDWALKER && p()->buff.weapons_of_order->up() )
      p()->buff.weapons_of_order_ww->trigger();
  }
};

// ==========================================================================
// Blackout Kick
// ==========================================================================

// Blackout Kick Proc from Teachings of the Monastery =======================
struct blackout_kick_totm_proc : public monk_melee_attack_t
{
  blackout_kick_totm_proc( monk_t* p ) : monk_melee_attack_t( "blackout_kick_totm_proc", p, p->passives.totm_bok_proc )
  {
    cooldown->duration = timespan_t::zero();
    background = dual             = true;
    trigger_chiji                 = true;
    trigger_gcd                   = timespan_t::zero();
  }

  void init_finished() override
  {
    monk_melee_attack_t::init_finished();
    action_t* bok = player->find_action( "blackout_kick" );
    if ( bok )
    {
      base_multiplier        = bok->base_multiplier;
      spell_power_mod.direct = bok->spell_power_mod.direct;

      bok->add_child( this );
    }
  }

  // Force 100 milliseconds for the animation, but not delay the overall GCD
  timespan_t execute_time() const override
  {
    return timespan_t::from_millis( 100 );
  }

  double cost() const override
  {
    return 0;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    if ( rng().roll( p()->spec.teachings_of_the_monastery->effectN( 1 ).percent() ) )
      p()->cooldown.rising_sun_kick->reset( true );

    if ( p()->conduit.tumbling_technique->ok() && rng().roll( p()->conduit.tumbling_technique->effectN( 1 ).percent() ) )
    {
      if ( p()->talent.chi_torpedo->ok() )
        p()->cooldown.chi_torpedo->reset( true, 1 );
      else
        p()->cooldown.roll->reset( true, 1 );
    }

    trigger_shuffle( p()->spec.blackout_kick->effectN( 2 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->talent.spirit_of_the_crane->ok() )
      p()->resource_gain(
          RESOURCE_MANA,
          ( p()->resources.max[ RESOURCE_MANA ] * p()->passives.spirit_of_the_crane->effectN( 1 ).percent() ),
          p()->gain.spirit_of_the_crane );
  }
};

// Charred Passions ============================================================
struct charred_passions_t : public monk_spell_t
{
  charred_passions_t( monk_t* p ) : monk_spell_t( "charred_passions", p, p->passives.charred_passions_dmg )
  {
    background = dual             = true;
    proc                          = true;
    may_crit                      = false;
  }
};

// Blackout Kick Baseline ability =======================================
struct blackout_kick_t : public monk_melee_attack_t
{
  blackout_kick_totm_proc* bok_totm_proc;
  charred_passions_t* charred_passions;

  blackout_kick_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "blackout_kick", p,
        ( p->specialization() == MONK_BREWMASTER ? p->spec.blackout_kick_brm : p->spec.blackout_kick ) ),
        charred_passions( new charred_passions_t( p ) )
  {
    ww_mastery = true;

    parse_options( options_str );
    sef_ability                   = SEF_BLACKOUT_KICK;
    may_combo_strike              = true;
    trigger_chiji                 = true;

    switch ( p->specialization() )
    {
      case MONK_MISTWEAVER:
      {
        bok_totm_proc = new blackout_kick_totm_proc( p );
        break;
      }
      case MONK_WINDWALKER:
      {
        if ( p->spec.blackout_kick_2 )
          // Saved as -1
          base_costs[ RESOURCE_CHI ] +=
              p->spec.blackout_kick_2->effectN( 1 ).base_value();  // Reduce base from 3 chi to 2
        break;
      }
      default:
        break;
    }

  }

  void init() override
  {
    monk_melee_attack_t::init();

    if ( p()->specialization() == MONK_WINDWALKER )
      ap_type = attack_power_type::WEAPON_BOTH;
  }

  double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->buff.weapons_of_order_ww->up() )
      c += p()->buff.weapons_of_order_ww->value();

    if ( c <= 0 )
      return 0;

    return c;
  }

  void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    // Register how much chi is saved without actually refunding the chi
    if ( p()->buff.serenity->up() )
    {
      if ( p()->buff.weapons_of_order_ww->up() )
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] - p()->buff.weapons_of_order_ww->value() );
      else
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
    }

    // Register how much chi is saved without actually refunding the chi
    if ( p()->buff.bok_proc->up() )
    {
      p()->buff.bok_proc->expire();
      if ( !p()->buff.serenity->up() )
        p()->gain.bok_proc->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
    }
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    switch ( p()->specialization() )
    {
      case MONK_BREWMASTER:
      {
        if ( p()->mastery.elusive_brawler->ok() )
          p()->buff.elusive_brawler->trigger();

        if ( p()->talent.blackout_combo->ok() )
          p()->buff.blackout_combo->trigger();

        trigger_shuffle( p()->spec.blackout_kick_brm->effectN( 2 ).base_value() );
        break;
      }
      case MONK_MISTWEAVER:
      {
        if ( rng().roll( p()->spec.teachings_of_the_monastery->effectN( 1 ).percent() ) )
          p()->cooldown.rising_sun_kick->reset( true );
        break;
      }
      case MONK_WINDWALKER:
      {
        if ( p()->spec.blackout_kick_3->ok() )
        {
          // Reduce the cooldown of Rising Sun Kick and Fists of Fury
          timespan_t cd_reduction = -1 * p()->spec.blackout_kick->effectN( 3 ).time_value();
          if ( p()->buff.weapons_of_order->up() )
            cd_reduction += ( -1 * p()->covenant.kyrian->effectN( 8 ).time_value() );

          // Reduction is getting halved during Serenity
          if ( p()->buff.serenity->up() )
            cd_reduction *= 1.0 / ( 1 + p()->talent.serenity->effectN( 4 ).percent() );

          p()->cooldown.rising_sun_kick->adjust( cd_reduction, true );
          p()->cooldown.fists_of_fury->adjust( cd_reduction, true );
        }
        break;
      }
      default:
        break;
    }

    if ( p()->conduit.tumbling_technique->ok() && rng().roll( p()->conduit.tumbling_technique.percent() ) )
    {
      if ( p()->talent.chi_torpedo->ok() )
        p()->cooldown.chi_torpedo->reset( true, 1 );
      else
        p()->cooldown.roll->reset( true, 1 );
    }
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p()->specialization() == MONK_WINDWALKER && p()->spec.spinning_crane_kick_2_ww->ok() )
        p()->trigger_mark_of_the_crane( s );

      if ( p()->buff.teachings_of_the_monastery->up() )
      {
        int stacks = p()->buff.teachings_of_the_monastery->current_stack;
        p()->buff.teachings_of_the_monastery->expire();

        for ( int i = 0; i < stacks; i++ )
          bok_totm_proc->execute();
      }

      if ( p()->buff.charred_passions->up() )
      {
        double dmg_percent         = p()->legendary.charred_passions->effectN( 1 ).percent();
        charred_passions->base_dd_min = s->result_amount * dmg_percent;
        charred_passions->base_dd_max = s->result_amount * dmg_percent;
        charred_passions->execute();

        if ( td( s->target )->dots.breath_of_fire->is_ticking() )
          td( s->target )->dots.breath_of_fire->refresh_duration();
      }
    }
  }
};

// ==========================================================================
// Rushing Jade Wind
// ==========================================================================

struct rjw_tick_action_t : public monk_melee_attack_t
{
  rjw_tick_action_t( const std::string& name, monk_t* p, const spell_data_t* data )
    : monk_melee_attack_t( name, p, data )
  {
    ww_mastery = true;

    dual = background = true;
    aoe               = p->talent.rushing_jade_wind->effectN( 1 ).base_value();
    radius            = data->effectN( 1 ).radius();

    // Reset some variables to ensure proper execution
    dot_duration       = timespan_t::zero();
    cooldown->duration = timespan_t::zero();
  }
};

struct rushing_jade_wind_t : public monk_melee_attack_t
{
  rushing_jade_wind_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "rushing_jade_wind", p, p->talent.rushing_jade_wind )
  {
    parse_options( options_str );
    sef_ability      = SEF_RUSHING_JADE_WIND;
    may_combo_strike = true;
    gcd_type         = gcd_haste_type::NONE;

    // Set dot data to 0, since we handle everything through the buff.
    base_tick_time = timespan_t::zero();
    dot_duration   = timespan_t::zero();

    if ( !p->active_actions.rushing_jade_wind )
    {
      p->active_actions.rushing_jade_wind =
          new rjw_tick_action_t( "rushing_jade_wind_tick", p, p->talent.rushing_jade_wind->effectN( 1 ).trigger() );
      p->active_actions.rushing_jade_wind->stats = stats;
    }
  }

  void init() override
  {
    monk_melee_attack_t::init();

    update_flags &= ~STATE_HASTE;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->buff.rushing_jade_wind->trigger();
  }
};

// ==========================================================================
// Spinning Crane Kick
// ==========================================================================

// Jade Ignition Legendary
struct chi_explosion_t : public monk_spell_t
{
  chi_explosion_t( monk_t* player )
    : monk_spell_t( "chi_explosion", player, player->passives.chi_explosion )
  {
    dual = background = true;
    aoe               = -1;
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    if ( p()->buff.chi_energy->up() )
        am += 1 + p()->buff.chi_energy->stack_value();

    return am;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.chi_energy->expire();
  }
};

struct sck_tick_action_t : public monk_melee_attack_t
{
  charred_passions_t* charred_passions;

  sck_tick_action_t( const std::string& name, monk_t* p, const spell_data_t* data )
    : monk_melee_attack_t( name, p, data ),
      charred_passions( new charred_passions_t( p ) )
  {
    ww_mastery                    = true;
    trigger_chiji                 = true;

    dual = background = true;
    aoe               = (int)p->spec.spinning_crane_kick->effectN( 1 ).base_value();
    radius            = data->effectN( 1 ).radius();

    // Reset some variables to ensure proper execution
    dot_duration                  = timespan_t::zero();
    school                        = SCHOOL_PHYSICAL;
    cooldown->duration            = timespan_t::zero();
    base_costs[ RESOURCE_ENERGY ] = 0;
  }

  void init() override
  {
    monk_melee_attack_t::init();

    if ( p()->specialization() == MONK_WINDWALKER )
      ap_type = attack_power_type::WEAPON_BOTH;
  }

  double cost() const override
  {
    return 0;
  }

  int mark_of_the_crane_counter() const
  {
    std::vector<player_t*> targets = target_list();
    int mark_of_the_crane_counter  = 0;

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      for ( player_t* target : targets )
      {
        if ( td( target )->debuff.mark_of_the_crane->up() )
          mark_of_the_crane_counter++;
      }
    }
    return mark_of_the_crane_counter;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    double motc_multiplier = p()->passives.cyclone_strikes->effectN( 1 ).percent();

    if ( p()->conduit.calculated_strikes->ok() )
      motc_multiplier += p()->conduit.calculated_strikes.percent();

    am *= 1 + ( mark_of_the_crane_counter() * motc_multiplier );

    if ( p()->buff.dance_of_chiji_hidden->up() )
      am *= 1 + p()->buff.dance_of_chiji_hidden->value();

    return am;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->spec.spinning_crane_kick_2_brm->ok() )
      trigger_shuffle( p()->spec.spinning_crane_kick_2_brm->effectN( 1 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->buff.charred_passions->up() )
    {
      double dmg_percent         = p()->legendary.charred_passions->effectN( 1 ).percent();
      charred_passions->base_dd_min = s->result_amount * dmg_percent;
      charred_passions->base_dd_max = s->result_amount * dmg_percent;
      charred_passions->execute();

      if ( td( s->target )->dots.breath_of_fire->is_ticking() )
        td( s->target )->dots.breath_of_fire->refresh_duration();
    }
  }
};

struct spinning_crane_kick_t : public monk_melee_attack_t
{
  chi_explosion_t* chi_x;

  spinning_crane_kick_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "spinning_crane_kick", p,
        ( p->specialization() == MONK_BREWMASTER ? p->spec.spinning_crane_kick_brm : p->spec.spinning_crane_kick ) ),
      chi_x( nullptr )
  {
    parse_options( options_str );

    sef_ability      = SEF_SPINNING_CRANE_KICK;
    may_combo_strike = true;

    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
    tick_zero = hasted_ticks = interrupt_auto_attack = true;

    spell_power_mod.direct = 0.0;
    dot_behavior           = DOT_REFRESH;  // Spell uses Pandemic Mechanics.

    tick_action =
        new sck_tick_action_t( "spinning_crane_kick_tick", p, p->spec.spinning_crane_kick->effectN( 1 ).trigger() );

    chi_x = new chi_explosion_t( p );
  }

  // N full ticks, but never additional ones.
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->buff.weapons_of_order_ww->up() )
      c += p()->buff.weapons_of_order_ww->value();

    if ( p()->buff.dance_of_chiji_hidden->up() )
      c += p()->passives.dance_of_chiji->effectN( 1 ).base_value();  // saved as -2

    if ( c < 0 )
      c = 0;

    return c;
  }

  void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    if ( p()->buff.serenity->up() )
    {
      double cost = base_costs[ RESOURCE_CHI ];

      if ( p()->buff.weapons_of_order_ww->up() )
        cost -= p()->buff.weapons_of_order_ww->value();

      if ( p()->buff.dance_of_chiji_hidden->up() )
        cost -= p()->buff.dance_of_chiji->value();

      if ( cost < 0 )
        cost = 0;

      p()->resource_gain( RESOURCE_CHI, cost, p()->gain.serenity );
    }
  }

  void execute() override
  {
    if ( p()->buff.dance_of_chiji->up() )
    {
      p()->buff.dance_of_chiji->expire();
      p()->buff.dance_of_chiji_hidden->trigger();
    }

    monk_melee_attack_t::execute();

    timespan_t buff_duration = composite_dot_duration( execute_state );

    p()->buff.spinning_crane_kick->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, buff_duration );

    if ( p()->buff.chi_energy->up() )
      chi_x->execute();

    if ( p()->buff.celestial_flames->up() )
    {
      p()->active_actions.breath_of_fire->target = execute_state->target;
      p()->active_actions.breath_of_fire->execute();
    }

    // Bonedust Brew
    // Chi refund is triggering once on the trigger spell and not from tick spells.
    if ( p()->covenant.necrolord->ok() )
      if ( p()->specialization() == MONK_WINDWALKER && td( execute_state->target )->debuff.bonedust_brew->up() &&
           !p()->buff.dance_of_chiji->up() )
        p()->resource_gain( RESOURCE_CHI, p()->passives.bonedust_brew_chi->effectN( 1 ).base_value(),
                            p()->gain.bonedust_brew );

  }

  void last_tick( dot_t* dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    if ( p()->buff.dance_of_chiji_hidden->up() )
      p()->buff.dance_of_chiji_hidden->expire();
  }
};

// ==========================================================================
// Fists of Fury
// ==========================================================================

struct fists_of_fury_tick_t : public monk_melee_attack_t
{
  fists_of_fury_tick_t( monk_t* p, const std::string& name )
    : monk_melee_attack_t( name, p, p->passives.fists_of_fury_tick )
  {
    background                    = true;
    aoe                           = 1 + (int)p->spec.fists_of_fury->effectN( 1 ).base_value();
    ww_mastery                    = true;

    attack_power_mod.direct    = p->spec.fists_of_fury->effectN( 5 ).ap_coeff();
    ap_type                    = attack_power_type::WEAPON_MAINHAND;
    base_costs[ RESOURCE_CHI ] = 0;
    dot_duration               = timespan_t::zero();
    trigger_gcd                = timespan_t::zero();
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = melee_attack_t::composite_aoe_multiplier( state );

    if ( state->target != target )
      return cam *= p()->spec.fists_of_fury->effectN( 6 ).percent();

    return cam;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->conduit.inner_fury->ok() )
      am *= 1 + p()->conduit.inner_fury.percent();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->legendary.jade_ignition->ok() )
      p()->buff.chi_energy->trigger();
  }
};

struct fists_of_fury_t : public monk_melee_attack_t
{
  fists_of_fury_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "fists_of_fury", p, p->spec.fists_of_fury )
  {
    parse_options( options_str );

    sef_ability          = SEF_FISTS_OF_FURY;
    may_combo_strike     = true;
    affected_by.serenity = true;

    channeled = tick_zero = true;
    interrupt_auto_attack = true;

    attack_power_mod.direct = 0;
    weapon_power_mod        = 0;

    // Effect 1 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
    base_tick_time = dot_duration / 4;
    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

    tick_action = new fists_of_fury_tick_t( p, "fists_of_fury_tick" );
  }

  double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->buff.weapons_of_order_ww->up() )
      c += p()->buff.weapons_of_order_ww->value();

    if ( c <= 0 )
      return 0;

    return c;
  }

  void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    // Register how much chi is saved without actually refunding the chi
    if ( p()->buff.serenity->up() )
    {
      if ( p()->buff.weapons_of_order_ww->up() )
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] - p()->buff.weapons_of_order_ww->value() );
      else
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
    }
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->talent.whirling_dragon_punch->ok() && p()->cooldown.rising_sun_kick->down() )
    {
      if ( this->cooldown_duration() <= p()->cooldown.rising_sun_kick->remains() )
        p()->buff.whirling_dragon_punch->set_duration( this->cooldown_duration() );
      else
        p()->buff.whirling_dragon_punch->set_duration( p()->cooldown.rising_sun_kick->remains() );
      p()->buff.whirling_dragon_punch->trigger();
    }
  }

  void last_tick( dot_t* dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    if ( p()->legendary.xuens_treasure->ok() )
      p()->buff.pressure_point->trigger();
  }
};

// ==========================================================================
// Whirling Dragon Punch
// ==========================================================================

struct whirling_dragon_punch_tick_t : public monk_melee_attack_t
{
  whirling_dragon_punch_tick_t( const std::string& name, monk_t* p, const spell_data_t* s )
    : monk_melee_attack_t( name, p, s )
  {
    ww_mastery                    = true;

    background = true;
    aoe        = -1;
    radius     = s->effectN( 1 ).radius();
  }
};

struct whirling_dragon_punch_t : public monk_melee_attack_t
{
  whirling_dragon_punch_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "whirling_dragon_punch", p, p->talent.whirling_dragon_punch )
  {
    sef_ability = SEF_WHIRLING_DRAGON_PUNCH;

    parse_options( options_str );
    interrupt_auto_attack = callbacks = false;
    channeled                         = true;
    dot_duration                      = data().duration();
    tick_zero                         = false;
    may_combo_strike                  = true;

    spell_power_mod.direct = 0.0;

    tick_action =
        new whirling_dragon_punch_tick_t( "whirling_dragon_punch_tick", p, p->passives.whirling_dragon_punch_tick );
  }

  bool ready() override
  {
    // Only usable while Fists of Fury and Rising Sun Kick are on cooldown.
    if ( p()->buff.whirling_dragon_punch->up() )
      return monk_melee_attack_t::ready();

    return false;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t tt = tick_time( s );
    return tt * 3;
  }
};

// ==========================================================================
// Fist of the White Tiger
// ==========================================================================
// Off hand hits first followed by main hand
// The ability does NOT require an off-hand weapon to be executed.
// The ability uses the main-hand weapon damage for both attacks

struct fist_of_the_white_tiger_main_hand_t : public monk_melee_attack_t
{
  fist_of_the_white_tiger_main_hand_t( monk_t* p, const char* name, const spell_data_t* s )
    : monk_melee_attack_t( name, p, s )
  {
    sef_ability                   = SEF_FIST_OF_THE_WHITE_TIGER;
    ww_mastery                    = true;

    may_dodge = may_parry = may_block = may_miss = true;
    dual                                         = true;
    // attack_power_mod.direct = p -> talent.fist_of_the_white_tiger -> effectN( 1 ).ap_coeff();
    weapon = &( player->main_hand_weapon );
  }
};

struct fist_of_the_white_tiger_t : public monk_melee_attack_t
{
  fist_of_the_white_tiger_main_hand_t* mh_attack;
  fist_of_the_white_tiger_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "fist_of_the_white_tiger_offhand", p, p->talent.fist_of_the_white_tiger ),
      mh_attack( nullptr )
  {
    sef_ability                   = SEF_FIST_OF_THE_WHITE_TIGER_OH;
    ww_mastery                    = true;
    may_combo_strike              = true;
    affected_by.serenity          = false;
    cooldown->hasted              = false;

    parse_options( options_str );
    may_dodge = may_parry = may_block = true;
    // This is the off-hand damage
    weapon      = &( player->off_hand_weapon );
    trigger_gcd = data().gcd();

    mh_attack = new fist_of_the_white_tiger_main_hand_t( p, "fist_of_the_white_tiger_mainhand",
                                                         p->talent.fist_of_the_white_tiger->effectN( 2 ).trigger() );
    add_child( mh_attack );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      p()->resource_gain( RESOURCE_CHI,
                          p()->talent.fist_of_the_white_tiger->effectN( 3 ).trigger()->effectN( 1 ).base_value(),
                          p()->gain.fist_of_the_white_tiger );

      mh_attack->execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    // Apply Mark of the Crane
    if ( result_is_hit( s->result ) && p()->spec.spinning_crane_kick_2_ww->ok() )
      p()->trigger_mark_of_the_crane( s );
  }
};

// ==========================================================================
// Melee
// ==========================================================================

struct melee_t : public monk_melee_attack_t
{
  int sync_weapons;
  bool first;
  melee_t( const std::string& name, monk_t* player, int sw )
    : monk_melee_attack_t( name, player, spell_data_t::nil() ), sync_weapons( sw ), first( true )
  {
    background = repeating = may_glance = true;
    trigger_gcd                         = timespan_t::zero();
    special                             = false;
    school                              = SCHOOL_PHYSICAL;
    weapon_multiplier                   = 1.0;

    if ( player->main_hand_weapon.group() == WEAPON_1H )
    {
      if ( player->specialization() != MONK_MISTWEAVER )
        base_hit -= 0.19;
    }
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->buff.storm_earth_and_fire->up() )
      am *= 1.0 + p()->spec.storm_earth_and_fire->effectN( 3 ).percent();

    if ( p()->buff.serenity->up() )
      am *= 1 + p()->talent.serenity->effectN( 7 ).percent();

    if ( p()->buff.hit_combo->up() )
      am *= 1 + p()->buff.hit_combo->stack_value();

    return am;
  }

  void reset() override
  {
    monk_melee_attack_t::reset();
    first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = monk_melee_attack_t::execute_time();

    if ( first )
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 )
                                               : timespan_t::zero();
    else
      return t;
  }

  void execute() override
  {
    if ( first )
      first = false;

    monk_melee_attack_t::execute();
  }
};

// ==========================================================================
// Auto Attack
// ==========================================================================

struct auto_attack_t : public monk_melee_attack_t
{
  int sync_weapons;
  auto_attack_t( monk_t* player, util::string_view options_str )
    : monk_melee_attack_t( "auto_attack", player, spell_data_t::nil() ), sync_weapons( 0 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );
    ignore_false_positive = true;

    p()->main_hand_attack                    = new melee_t( "melee_main_hand", player, sync_weapons );
    p()->main_hand_attack->weapon            = &( player->main_hand_weapon );
    p()->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;

    if ( player->off_hand_weapon.type != WEAPON_NONE )
    {
      if ( !player->dual_wield() )
        return;

      p()->off_hand_attack                    = new melee_t( "melee_off_hand", player, sync_weapons );
      p()->off_hand_attack->weapon            = &( player->off_hand_weapon );
      p()->off_hand_attack->base_execute_time = player->off_hand_weapon.swing_time;
      p()->off_hand_attack->id                = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  bool ready() override
  {
    if ( p()->current.distance_to_move > 5 )
      return false;

    return ( p()->main_hand_attack->execute_event == nullptr );  // not swinging
  }

  void execute() override
  {
    if ( player->main_hand_attack )
      p()->main_hand_attack->schedule_execute();

    if ( player->off_hand_attack )
      p()->off_hand_attack->schedule_execute();
  }
};

// ==========================================================================
// Keg Smash
// ==========================================================================
struct keg_smash_t : public monk_melee_attack_t
{
  keg_smash_t( monk_t& p, util::string_view options_str ) : monk_melee_attack_t( "keg_smash", &p, p.spec.keg_smash )
  {
    parse_options( options_str );

    aoe = -1;

    attack_power_mod.direct = p.spec.keg_smash->effectN( 2 ).ap_coeff();
    radius                  = p.spec.keg_smash->effectN( 2 ).radius();

    cooldown->duration = p.spec.keg_smash->cooldown();
    cooldown->duration = p.spec.keg_smash->charge_cooldown();

    if ( p.legendary.stormstouts_last_keg->ok() )
      cooldown->charges += p.legendary.stormstouts_last_keg->effectN( 2 ).base_value();

    // Keg Smash does not appear to be picking up the baseline Trigger GCD reduction
    // Forcing the trigger GCD to 1 second.
    trigger_gcd = timespan_t::from_seconds( 1 );
  }

  // For more than 5 targets damage is based on a Sqrt(5/x)
  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = monk_melee_attack_t::composite_aoe_multiplier( state );

    if ( state->n_targets > p()->spec.keg_smash->effectN( 7 ).base_value() )
      // this is the closest we can come up without Blizzard flat out giving us the function
      // Primary takes the 100% damage
      // Secondary targets get reduced damage
      if ( state->target != target )
        cam *= std::sqrt( 5 / state->n_targets );

    return cam;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->legendary.stormstouts_last_keg->ok() )
      am *= 1 + p()->legendary.stormstouts_last_keg->effectN( 1 ).percent();

    if ( p()->conduit.scalding_brew->ok() )
    {
      if ( td( p()->target )->dots.breath_of_fire->is_ticking() )
        am *= 1 + p()->conduit.scalding_brew.percent();
    }

    return am;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    // Reduces the remaining cooldown on your Brews by 4 sec.
    double time_reduction = p()->spec.keg_smash->effectN( 4 ).base_value();

    // Blackout Combo talent reduces Brew's cooldown by 2 sec.
    if ( p()->buff.blackout_combo->up() )
    {
      time_reduction += p()->buff.blackout_combo->data().effectN( 3 ).base_value();
      p()->buff.blackout_combo->expire();
    }

    trigger_shuffle( p()->spec.keg_smash->effectN( 6 ).base_value() );

    brew_cooldown_reduction( time_reduction );
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    td( s->target )->debuff.keg_smash->trigger();

    if ( p()->buff.weapons_of_order->up() )
      td( s->target )->debuff.weapons_of_order->trigger();

    // Bonedust Brew
    if ( td( s->target )->debuff.bonedust_brew->up() )
      brew_cooldown_reduction( p()->covenant.necrolord->effectN( 3 ).base_value() );
  }
};

// ==========================================================================
// Touch of Death
// ==========================================================================
struct touch_of_death_t : public monk_melee_attack_t
{

  touch_of_death_t( monk_t& p, util::string_view options_str )
    : monk_melee_attack_t( "touch_of_death", &p, p.spec.touch_of_death )
  {
    may_crit = hasted_ticks = false;
    may_combo_strike        = true;
    parse_options( options_str );
    cooldown->duration = data().cooldown();

    if ( p.legendary.fatal_touch.ok() )
      cooldown->duration -= p.legendary.fatal_touch->effectN( 1 ).time_value();
  }

  void init() override
  {
    monk_melee_attack_t::init();

    snapshot_flags = update_flags = 0;
  }

  double composite_target_armor( player_t* ) const override
  {
    return 0;
  }

  bool ready() override
  {
    if ( p()->spec.touch_of_death_2->ok() &&
         ( target->health_percentage() < p()->spec.touch_of_death->effectN( 2 ).base_value() ) )
      return monk_melee_attack_t::ready();
    if ( ( target->true_level <= p()->true_level ) &&
         ( target->current_health() <= p()->resources.max[ RESOURCE_HEALTH ] ) )
      return monk_melee_attack_t::ready();

    return false;

  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->specialization() == MONK_MISTWEAVER && p()->spec.touch_of_death_3_mw->ok() )
      p()->buff.touch_of_death->trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    // Damage is associated with the players non-buffed max HP
    // Meaning using Fortifying Brew does not affect ToD's damage
    double amount = p()->resources.initial[ RESOURCE_HEALTH ];

    if ( target->true_level > p()->true_level )
      amount *= p()->spec.touch_of_death->effectN( 3 ).percent();  // 35% HP

    amount *= 1 + p()->cache.damage_versatility();

    if ( p()->buff.combo_strikes->up() )
      amount *= 1 + p()->cache.mastery_value();

    s->result_total = s->result_raw = amount;
    monk_melee_attack_t::impact( s );

    if ( p()->spec.touch_of_death_3_brm->ok() )
      p()->partial_clear_stagger_amount( amount * p()->spec.touch_of_death_3_brm->effectN( 1 ).percent() );
  }
};

// ==========================================================================
// Touch of Karma
// ==========================================================================
// When Touch of Karma (ToK) is activated, two spells are placed. A buff on the player (id: 125174), and a
// debuff on the target (id: 122470). Whenever the player takes damage, a dot (id: 124280) is placed on
// the target that increases as the player takes damage. Each time the player takes damage, the dot is refreshed
// and recalculates the dot size based on the current dot size. Just to make it easier to code, I'll wait until
// the Touch of Karma buff expires before placing a dot on the target. Net result should be the same.

// 8.1 Good Karma - If the player still has the ToK buff on them, each time the target hits the player, the amount
// absorbed is immediatly healed by the Good Karma spell (id: 285594)

struct touch_of_karma_dot_t : public residual_action::residual_periodic_action_t<monk_melee_attack_t>
{
  touch_of_karma_dot_t( monk_t* p ) : base_t( "touch_of_karma", p, p->passives.touch_of_karma_tick )
  {
    may_miss = may_crit = false;
    dual                = true;
    ap_type             = attack_power_type::NO_WEAPON;
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    monk_melee_attack_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags = update_flags = 0;
    snapshot_flags |= STATE_VERSATILITY;
  }
};

struct touch_of_karma_t : public monk_melee_attack_t
{
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double pct_health;
  touch_of_karma_dot_t* touch_of_karma_dot;
  touch_of_karma_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "touch_of_karma", p, p->spec.touch_of_karma ),
      interval( 100 ),
      interval_stddev( 0.05 ),
      interval_stddev_opt( 0 ),
      pct_health( 0.5 ),
      touch_of_karma_dot( new touch_of_karma_dot_t( p ) )
  {
    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "pct_health", pct_health ) );
    parse_options( options_str );
    cooldown->duration = data().cooldown();
    base_dd_min = base_dd_max = 0;
    ap_type                   = attack_power_type::NO_WEAPON;

    double max_pct = data().effectN( 3 ).percent();

    if ( pct_health > max_pct )  // Does a maximum of 50% of the monk's HP.
      pct_health = max_pct;

    if ( interval < cooldown->duration.total_seconds() )
    {
      sim->error( "{} minimum interval for Touch of Karma is 90 seconds.", *player );
      interval = cooldown->duration.total_seconds();
    }

    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;

    trigger_gcd = timespan_t::zero();
    may_crit = may_miss = may_dodge = may_parry = false;
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    monk_melee_attack_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags = update_flags = 0;
  }

  void execute() override
  {
    timespan_t new_cd        = timespan_t::from_seconds( rng().gauss( interval, interval_stddev ) );
    timespan_t data_cooldown = data().cooldown();
    if ( new_cd < data_cooldown )
      new_cd = data_cooldown;

    cooldown->duration = new_cd;

    monk_melee_attack_t::execute();

    if ( pct_health > 0 )
    {
      double damage_amount = pct_health * player->resources.max[ RESOURCE_HEALTH ];

      damage_amount *= data().effectN( 4 ).percent();

      residual_action::trigger( touch_of_karma_dot, execute_state->target, damage_amount );
    }
  }
};

// ==========================================================================
// Provoke
// ==========================================================================

struct provoke_t : public monk_melee_attack_t
{
  provoke_t( monk_t* p, util::string_view options_str ) : monk_melee_attack_t( "provoke", p, p->spec.provoke )
  {
    parse_options( options_str );
    use_off_gcd           = true;
    ignore_false_positive = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
      target->taunt( player );

    monk_melee_attack_t::impact( s );
  }
};

// ==========================================================================
// Spear Hand Strike
// ==========================================================================

struct spear_hand_strike_t : public monk_melee_attack_t
{
  spear_hand_strike_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "spear_hand_strike", p, p->spec.spear_hand_strike )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    is_interrupt          = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

//    if ( p()->level() <= 50 )
//      p()->trigger_sephuzs_secret( execute_state, MECHANIC_INTERRUPT );
  }
};

// ==========================================================================
// Leg Sweep
// ==========================================================================

struct leg_sweep_t : public monk_melee_attack_t
{
  leg_sweep_t( monk_t* p, util::string_view options_str ) : monk_melee_attack_t( "leg_sweep", p, p->spec.leg_sweep )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

//    if ( p()->level() <= 50 )
//      p()->trigger_sephuzs_secret( execute_state, MECHANIC_STUN );
  }
};

// ==========================================================================
// Paralysis
// ==========================================================================

struct paralysis_t : public monk_melee_attack_t
{
  paralysis_t( monk_t* p, util::string_view options_str ) : monk_melee_attack_t( "paralysis", p, p->spec.paralysis )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

//    if ( p()->level() <= 50 )
//      p()->trigger_sephuzs_secret( execute_state, MECHANIC_INCAPACITATE );
  }
};

// ==========================================================================
// Flying Serpent Kick
// ==========================================================================

struct flying_serpent_kick_t : public monk_melee_attack_t
{
  bool first_charge;
  double movement_speed_increase;
  flying_serpent_kick_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "flying_serpent_kick", p, p->spec.flying_serpent_kick ),
      first_charge( true ),
      movement_speed_increase( p->spec.flying_serpent_kick->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    ww_mastery                      = true;
    may_combo_strike                = true;
    ignore_false_positive           = true;
    movement_directionality         = movement_direction_type::OMNI;
    attack_power_mod.direct         = p->passives.flying_serpent_kick_damage->effectN( 1 ).ap_coeff();
    aoe                             = -1;
    p->cooldown.flying_serpent_kick = cooldown;

    if ( p->spec.flying_serpent_kick_2 )
      p->cooldown.flying_serpent_kick->duration += p->spec.flying_serpent_kick_2->effectN( 1 ).time_value(); // Saved as -5000
  }

  void reset() override
  {
    action_t::reset();
    first_charge = true;
  }

  bool ready() override
  {
    if ( first_charge )  // Assumes that we fsk into combat, instead of setting initial distance to 20 yards.
      return monk_melee_attack_t::ready();

    return monk_melee_attack_t::ready();
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->spec.windwalker_monk->effectN( 1 ).percent();

    return am;
  }

  void execute() override
  {
    if ( p()->current.distance_to_move >= 0 )
    {
      p()->buff.flying_serpent_kick_movement->trigger(
          1, movement_speed_increase, 1,
          timespan_t::from_seconds(
              std::min( 1.5,
              p()->current.distance_to_move /
              ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) ) ) );
      p()->current.moving_away = 0;
    }

    monk_melee_attack_t::execute();

    if ( first_charge )
    {
      first_charge = !first_charge;
    }
  }

  void impact( action_state_t* state ) override
  {
    monk_melee_attack_t::impact( state );

    td( state->target )->debuff.flying_serpent_kick->trigger();
  }
};
}  // namespace attacks

namespace spells
{
// ==========================================================================
// Energizing Elixir
// ==========================================================================

struct energizing_elixir_t : public monk_spell_t
{
  energizing_elixir_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "energizing_elixir", player, player->talent.energizing_elixir )
  {
    parse_options( options_str );

    dot_duration = timespan_t::zero();
    may_miss = may_crit = harmful = false;
    energize_type                 = action_energize::NONE;  // disable resource gain from spell data
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->resource_gain( RESOURCE_ENERGY, p()->talent.energizing_elixir->effectN( 1 ).base_value(),
                        p()->gain.energizing_elixir_energy );
    p()->resource_gain( RESOURCE_CHI, p()->talent.energizing_elixir->effectN( 2 ).base_value(),
                        p()->gain.energizing_elixir_chi );
  }
};

// ==========================================================================
// Black Ox Brew
// ==========================================================================

struct black_ox_brew_t : public monk_spell_t
{
  black_ox_brew_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "black_ox_brew", player, player->talent.black_ox_brew )
  {
    parse_options( options_str );

    harmful       = false;
    use_off_gcd   = true;
    energize_type = action_energize::NONE;  // disable resource gain from spell data
  }

  void execute() override
  {
    monk_spell_t::execute();

    // Refill Purifying Brew charges.
    p()->cooldown.purifying_brew->reset( true, -1 );

    // Refills Celestial Brew charges
    p()->cooldown.celestial_brew->reset( true, -1 );

    p()->resource_gain( RESOURCE_ENERGY, p()->talent.black_ox_brew->effectN( 1 ).base_value(),
                        p()->gain.black_ox_brew_energy );

    if ( p()->talent.celestial_flames->ok() )
      p()->buff.celestial_flames->trigger();


  }
};

// ==========================================================================
// Roll
// ==========================================================================

struct roll_t : public monk_spell_t
{
  roll_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "roll", player, ( player->talent.chi_torpedo ? spell_data_t::nil() : player->spec.roll ) )
  {
    parse_options( options_str );

    cooldown->charges += (int)player->spec.roll_2->effectN( 1 ).base_value();

    if ( player->talent.celerity )
    {
      cooldown->duration += player->talent.celerity->effectN( 1 ).time_value();
      cooldown->charges += (int)player->talent.celerity->effectN( 2 ).base_value();
    }

    if ( player->legendary.swiftsure_wraps.ok() )
      cooldown->charges += (int)player->legendary.swiftsure_wraps->effectN( 1 ).base_value();
  }
};

// ==========================================================================
// Chi Torpedo
// ==========================================================================

struct chi_torpedo_t : public monk_spell_t
{
  chi_torpedo_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "chi_torpedo", player, player->talent.chi_torpedo )
  {
    parse_options( options_str );

    cooldown->charges += (int)player->spec.roll_2->effectN( 1 ).base_value();

    if ( player->legendary.swiftsure_wraps.ok() )
      cooldown->charges += (int)player->legendary.swiftsure_wraps->effectN( 1 ).base_value();
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.chi_torpedo->trigger();
  }
};

// ==========================================================================
// Serenity
// ==========================================================================

struct serenity_t : public monk_spell_t
{
  serenity_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "serenity", player, player->talent.serenity )
  {
    parse_options( options_str );
    harmful     = false;
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.serenity->trigger();
  }
};

// ==========================================================================
// Crackling Jade Lightning
// ==========================================================================

struct crackling_jade_lightning_t : public monk_spell_t
{
  crackling_jade_lightning_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "crackling_jade_lightning", &p, p.spec.crackling_jade_lightning )
  {
    sef_ability      = SEF_CRACKLING_JADE_LIGHTNING;
    may_combo_strike = true;

    parse_options( options_str );

    channeled = tick_zero = tick_may_crit = true;
    dot_duration                          = data().duration();
    hasted_ticks = false;  // Channeled spells always have hasted ticks. Use hasted_ticks = false to disable the
                           // increase in the number of ticks.
    interrupt_auto_attack = true;
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t t = base_tick_time;
    if ( channeled || hasted_ticks )
    {
      t *= state->haste;
    }
    return t;
  }

  double cost_per_tick( resource_e resource ) const override
  {
    double c = monk_spell_t::cost_per_tick( resource );

    if ( p()->buff.the_emperors_capacitor->up() && resource == RESOURCE_ENERGY )
      c *= 1 + ( p()->buff.the_emperors_capacitor->current_stack *
                 p()->buff.the_emperors_capacitor->data().effectN( 2 ).percent() );

    return c;
  }

  double cost() const override
  {
    double c = monk_spell_t::cost();

    if ( p()->buff.the_emperors_capacitor->up() )
      c *= 1 + ( p()->buff.the_emperors_capacitor->current_stack *
                 p()->buff.the_emperors_capacitor->data().effectN( 2 ).percent() );

    return c;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_spell_t::composite_persistent_multiplier( action_state );

    if ( p()->buff.the_emperors_capacitor->up() )
      pm *= 1 + p()->buff.the_emperors_capacitor->stack_value();

    return pm;
  }

  void last_tick( dot_t* dot ) override
  {
    monk_spell_t::last_tick( dot );

    if ( p()->buff.the_emperors_capacitor->up() )
      p()->buff.the_emperors_capacitor->expire();

    // Reset swing timer
    if ( player->main_hand_attack )
    {
      player->main_hand_attack->cancel();
      if ( !player->main_hand_attack->target->is_sleeping() )
      {
        player->main_hand_attack->schedule_execute();
      }
    }

    if ( player->off_hand_attack )
    {
      player->off_hand_attack->cancel();
      if ( !player->off_hand_attack->target->is_sleeping() )
      {
        player->off_hand_attack->schedule_execute();
      }
    }
  }
};

// ==========================================================================
// Breath of Fire
// ==========================================================================

  struct breath_of_fire_dot_t : public monk_spell_t
{
    breath_of_fire_dot_t( monk_t& p ) : monk_spell_t( "breath_of_fire_dot", &p, p.passives.breath_of_fire_dot )
  {
    background    = true;
    tick_may_crit = may_crit = true;
    hasted_ticks             = false;
  }
};

struct breath_of_fire_t : public monk_spell_t
{

  breath_of_fire_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "breath_of_fire", &p, p.spec.breath_of_fire )
  {
    parse_options( options_str );
    gcd_type = gcd_haste_type::NONE;

    aoe = 1;

    add_child( p.active_actions.breath_of_fire );
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown->duration;

    // Update the cooldown if Blackout Combo is up
    if ( p()->buff.blackout_combo->up() )
    {
      // Saved as 3 seconds
      cd += (-1 * timespan_t::from_seconds( p()->buff.blackout_combo->data().effectN( 2 ).base_value() ) );
      p()->buff.blackout_combo->expire();
    }

    monk_spell_t::update_ready( cd );
  }

  // Initial damage does Square Root damage
  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam  = monk_spell_t::composite_aoe_multiplier( state );

    if ( state->target != target )
      return cam / std::sqrt( state->n_targets );

    return cam;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->buff.spitfire->up() )
      p()->buff.spitfire->expire();

    if ( p()->legendary.charred_passions->ok() )
      p()->buff.charred_passions->trigger();
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    monk_td_t& td = *this->td( s->target );

    if ( td.debuff.keg_smash->up() || td.debuff.fallen_monk_keg_smash->up() )
    {
      p()->active_actions.breath_of_fire->target = s->target;
      p()->active_actions.breath_of_fire->execute();
    }
  }
};

// ==========================================================================
// Special Delivery
// ==========================================================================

struct special_delivery_t : public monk_spell_t
{
  special_delivery_t( monk_t& p ) : monk_spell_t( "special_delivery", &p, p.passives.special_delivery )
  {
    may_block = may_dodge = may_parry = true;
    background                        = true;
    trigger_gcd                       = timespan_t::zero();
    aoe                               = -1;
    attack_power_mod.direct           = data().effectN( 1 ).ap_coeff();
    radius                            = data().effectN( 1 ).radius();
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( p()->talent.special_delivery->effectN( 1 ).base_value() );
  }

  double cost() const override
  {
    return 0;
  }
};

// ==========================================================================
// Fortifying Brew
// ==========================================================================

struct fortifying_ingredients_t : public monk_absorb_t
{
  fortifying_ingredients_t( monk_t& p ) :
      monk_absorb_t( "fortifying_ingredients", p, p.passives.fortifying_ingredients )
  {
    harmful = may_crit = false;
  }
};

struct fortifying_brew_t : public monk_spell_t
{
  special_delivery_t* delivery;
  fortifying_ingredients_t* fortifying_ingredients;

  fortifying_brew_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "fortifying_brew", &p,
        ( p.specialization() == MONK_BREWMASTER ? p.spec.fortifying_brew_brm : p.spec.fortifying_brew_mw_ww ) ),
      fortifying_ingredients( new fortifying_ingredients_t( p ) )
  {
    parse_options( options_str );

    harmful = may_crit = false;

    if ( p.spec.fortifying_brew_2_mw )
      cooldown->duration += p.spec.fortifying_brew_2_mw->effectN( 1 ).time_value();

    if ( p.spec.fortifying_brew_2_ww )
      cooldown->duration += p.spec.fortifying_brew_2_ww->effectN( 1 ).time_value();

    if ( p.talent.special_delivery->ok() )
      delivery = new special_delivery_t( p );
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.fortifying_brew->trigger();

    if ( p()->talent.special_delivery->ok() )
    {
      delivery->set_target( target );
      delivery->execute();
    }

    if ( p()->talent.celestial_flames->ok() )
      p()->buff.celestial_flames->trigger();

    if ( p()->conduit.fortifying_ingredients->ok() )
    {
      double absorb_percent               = p()->conduit.fortifying_ingredients.percent();
      fortifying_ingredients->base_dd_min = p()->resources.max[ RESOURCE_HEALTH ] * absorb_percent;
      fortifying_ingredients->base_dd_max = p()->resources.max[ RESOURCE_HEALTH ] * absorb_percent;
      fortifying_ingredients->execute();
    }
  }
};

// ==========================================================================
// Exploding Keg
// ==========================================================================
struct exploding_keg_t : public monk_spell_t
{
  exploding_keg_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "exploding_keg", &p, p.talent.exploding_keg )
  {
    parse_options( options_str );

    aoe    = -1;
    radius = data().effectN( 1 ).radius();
    range  = data().max_range();
  }

  void impact( action_state_t* state ) override
  {
    monk_spell_t::impact( state );

    td( state->target )->debuff.exploding_keg->trigger();
  }

  timespan_t travel_time() const override
  {
    // Always has the same time to land regardless of distance, probably represented there.
    return timespan_t::from_seconds( data().missile_speed() );
  }

  // Ensuring that we can't cast on a target that is too close
  bool target_ready( player_t* candidate_target ) override
  {
    if ( player->get_player_distance( *candidate_target ) < data().min_range() )
    {
      return false;
    }

    return monk_spell_t::target_ready( candidate_target );
  }
};

// ==========================================================================
// Stagger Damage
// ==========================================================================

struct stagger_self_damage_t : public residual_action::residual_periodic_action_t<monk_spell_t>
{
  stagger_self_damage_t( monk_t* p ) : base_t( "stagger_self_damage", p, p->passives.stagger_self_damage )
  {
    // Just get dot duration from heavy stagger spell data
    auto s = p->find_spell( 124273 );
    assert( s );

    dot_duration = s->duration();
    dot_duration += timespan_t::from_seconds( p->talent.bob_and_weave->effectN( 1 ).base_value() / 10 );
    base_tick_time = timespan_t::from_millis( 500 );
    hasted_ticks = tick_may_crit = false;
    target                       = p;
  }

  void impact( action_state_t* s ) override
  {
    if ( p()->conduit.evasive_stride->ok() )
    {
      // Tooltip shows this as (Value / 10)%; ie: (25 / 10)% = 2.5%.
      // For roll purpose we need this to go to 0.025 or value / 1000
      if ( p()->buff.shuffle->up() && p()->buff.heavy_stagger->up() &&
           rng().roll( p()->conduit.evasive_stride.value() / 1000 ) )
      {
        p()->active_actions.evasive_stride->base_dd_min = s->result_amount;
        p()->active_actions.evasive_stride->base_dd_max = s->result_amount;
        p()->active_actions.evasive_stride->execute();
        s->result_amount = 0;
      }
    }

    base_t::impact( s );
    p()->buff.shuffle->up();  // benefit tracking
    p()->stagger_damage_changed();
  }

  void assess_damage( result_amount_type type, action_state_t* s ) override
  {
    base_t::assess_damage( type, s );

    p()->stagger_tick_damage.push_back( {s->result_amount} );

    p()->sample_datas.stagger_effective_damage_timeline.add( sim->current_time(), s->result_amount );
    p()->sample_datas.stagger_damage->add( s->result_amount );

    if ( p()->buff.light_stagger->check() )
    {
      p()->sample_datas.light_stagger_damage->add( s->result_amount );
    }
    else if ( p()->buff.moderate_stagger->check() )
    {
      p()->sample_datas.moderate_stagger_damage->add( s->result_amount );
    }
    else if ( p()->buff.heavy_stagger->check() )
    {
      p()->sample_datas.heavy_stagger_damage->add( s->result_amount );
    }
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );
    p()->stagger_damage_changed( true );
  }

  proc_types proc_type() const override
  {
    return PROC1_ANY_DAMAGE_TAKEN;
  }

  void init() override
  {
    base_t::init();

    // We don't want this counted towards our dps
    stats->type = stats_e::STATS_NEUTRAL;
  }

  void delay_tick( timespan_t seconds )
  {
    dot_t* d = get_dot();
    if ( d->is_ticking() )
    {
      if ( d->tick_event )
      {
        d->tick_event->reschedule( d->tick_event->remains() + seconds );
        if ( d->end_event )
        {
          d->end_event->reschedule( d->end_event->remains() + seconds );
        }
      }
    }
  }

  /* Clears the dot and all damage. Used by Purifying Brew
   * Returns amount purged
   */
  double clear_all_damage()
  {
    dot_t* d              = get_dot();
    double amount_cleared = amount_remaining();

    d->cancel();
    cancel();
    p()->stagger_damage_changed();

    return amount_cleared;
  }

  /* Clears part of the stagger dot. Used by Purifying Brew
   * Returns amount purged
   */
  double clear_partial_damage_pct( double percent_amount )
  {
    dot_t* d              = get_dot();
    double amount_cleared = 0.0;

    if ( d->is_ticking() )
    {
      debug_cast<residual_action::residual_periodic_state_t*>( d->state );

      auto ticks_left                   = d->ticks_left();
      auto damage_remaining_initial     = amount_remaining();
      auto damage_remaining_after_clear = damage_remaining_initial * ( 1.0 - percent_amount );
      amount_cleared                    = damage_remaining_initial - damage_remaining_after_clear;
      set_tick_amount( damage_remaining_after_clear / ticks_left );
      assert( std::fabs( amount_remaining() - damage_remaining_after_clear ) < 1.0 &&
              "stagger remaining amount after clear does not match" );

      sim->print_debug( "{} partially clears stagger by {} (requested:  {:.2f}%). Damage remaining is {}.",
                        player->name(), amount_cleared, percent_amount * 100.0, damage_remaining_after_clear );
    }
    else
    {
      sim->print_debug( "{} no active stagger to clear (requested pct: {}).", player->name(), percent_amount * 100.0 );
    }

    p()->stagger_damage_changed();

    return amount_cleared;
  }

  /* Clears part of the stagger dot. Used by Staggering Strikes Azerite Trait
   * Returns amount purged
   */
  double clear_partial_damage_amount( double amount )
  {
    dot_t* d              = get_dot();
    double amount_cleared = 0.0;

    if ( d->is_ticking() )
    {
      debug_cast<residual_action::residual_periodic_state_t*>( d->state );
      auto ticks_left                   = d->ticks_left();
      auto damage_remaining_initial     = amount_remaining();
      auto damage_remaining_after_clear = std::fmax( damage_remaining_initial - amount, 0 );
      amount_cleared                    = damage_remaining_initial - damage_remaining_after_clear;
      set_tick_amount( damage_remaining_after_clear / ticks_left );
      assert( std::fabs( amount_remaining() - damage_remaining_after_clear ) < 1.0 &&
              "stagger remaining amount after clear does not match" );

      sim->print_debug( "{} partially clears stagger by {} (requested: {}). Damage remaining is {}.", player->name(),
                        amount_cleared, amount, damage_remaining_after_clear );
    }
    else
    {
      sim->print_debug( "{} no active stagger to clear (requested amount: {}).", player->name(), amount );
    }

    p()->stagger_damage_changed();

    return amount_cleared;
  }

  // adds amount to residual actions tick amount
  void set_tick_amount( double amount )
  {
    dot_t* dot = get_dot();
    if ( dot )
    {
      auto rd_state         = debug_cast<residual_action::residual_periodic_state_t*>( dot->state );
      rd_state->tick_amount = amount;
    }
  }

  bool stagger_ticking()
  {
    dot_t* d = get_dot();
    return d->is_ticking();
  }

  double tick_amount()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return base_ta( d->state );
    return 0;
  }

  double tick_ratio_to_hp()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return base_ta( d->state ) / p()->resources.max[ RESOURCE_HEALTH ];
    return 0;
  }

  double amount_remaining()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return base_ta( d->state ) * d->ticks_left();
    return 0;
  }

  double stagger_total()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return base_ta( d->state ) * static_cast<double>( dot_duration / base_tick_time );
    return 0;
  }

  double amount_remaining_to_total()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return ( base_ta( d->state ) * d->ticks_left() ) / ( base_ta( d->state ) * static_cast<double>( dot_duration / base_tick_time ) );
    return 0;
  }
};

// ==========================================================================
// Purifying Brew
// ==========================================================================

struct purifying_brew_t : public monk_spell_t
{
  special_delivery_t* delivery;

  purifying_brew_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "purifying_brew", &p, p.spec.purifying_brew )
  {
    parse_options( options_str );

    harmful     = false;

    cooldown->charges += (int)p.spec.purifying_brew_2->effectN( 1 ).base_value();

    if ( p.talent.light_brewing->ok() )
      cooldown->duration *= 1 + p.talent.light_brewing->effectN( 2 ).percent(); // -20

    if ( p.talent.special_delivery->ok() )
      delivery = new special_delivery_t( p );
  }

  bool ready() override
  {
    // Irrealistic of in-game, but let's make sure stagger is actually present
    if ( !p()->active_actions.stagger_self_damage->stagger_ticking() )
      return false;

    return monk_spell_t::ready();
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->talent.special_delivery->ok() )
    {
      delivery->set_target( target );
      delivery->execute();
    }

    if ( p()->talent.celestial_flames->ok() )
      p()->buff.celestial_flames->trigger();

    if ( p()->buff.blackout_combo->up() )
    {
      p()->buff.elusive_brawler->trigger( 1 );
      p()->buff.blackout_combo->expire();
    }

    if ( p()->spec.celestial_brew_2->ok() )
    {
      double count = 1;
      for ( auto&& buff : { p()->buff.light_stagger, p()->buff.moderate_stagger, p()->buff.heavy_stagger } )
      {
        if ( buff && buff->check() )
        {
          p()->buff.purified_chi->trigger( count );
        }
        count++;
      }
    }

    if ( p()->legendary.celestial_infusion->ok() &&
         rng().roll( p()->legendary.celestial_infusion->effectN( 2 ).percent() ) )
      p()->cooldown.purifying_brew->reset( true, 1 );


    // Reduce stagger damage
    auto amount_cleared =
        p()->active_actions.stagger_self_damage->clear_partial_damage_pct( data().effectN( 1 ).percent() );
    p()->sample_datas.purified_damage->add( amount_cleared );
    p()->buff.recent_purifies->trigger( 1, amount_cleared );
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

struct mana_tea_t : public monk_spell_t
{
  mana_tea_t( monk_t& p, util::string_view options_str ) : monk_spell_t( "mana_tea", &p, p.talent.mana_tea )
  {
    parse_options( options_str );

    harmful     = false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.mana_tea->trigger();
  }
};

// ==========================================================================
// Thunder Focus Tea
// ==========================================================================

struct thunder_focus_tea_t : public monk_spell_t
{
  thunder_focus_tea_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "Thunder_focus_tea", &p, p.spec.thunder_focus_tea )
  {
    parse_options( options_str );

    harmful     = false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.thunder_focus_tea->trigger( p()->buff.thunder_focus_tea->max_stack() );
  }
};

// ==========================================================================
// Dampen Harm
// ==========================================================================

struct dampen_harm_t : public monk_spell_t
{
  dampen_harm_t( monk_t& p, util::string_view options_str ) : monk_spell_t( "dampen_harm", &p, p.talent.dampen_harm )
  {
    parse_options( options_str );
    harmful     = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.dampen_harm->trigger();
  }
};

// ==========================================================================
// Diffuse Magic
// ==========================================================================

struct diffuse_magic_t : public monk_spell_t
{
  diffuse_magic_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "diffuse_magic", &p, p.talent.diffuse_magic )
  {
    parse_options( options_str );
    harmful     = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  void execute() override
  {
    p()->buff.diffuse_magic->trigger();
    monk_spell_t::execute();
  }
};

// ==========================================================================
// Invoke Xuen, the White Tiger
// ==========================================================================

struct xuen_spell_t : public monk_spell_t
{
  xuen_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "invoke_xuen_the_white_tiger", p, p->spec.invoke_xuen )
  {
    parse_options( options_str );

    harmful = false;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.xuen->summon( p()->spec.invoke_xuen->duration() );

    p()->buff.invoke_xuen->trigger();

    if ( p()->legendary.invokers_delight->ok() )
      p()->buff.invokers_delight->trigger();
  }
};

struct empowered_tiger_lightning_t : public monk_spell_t
{
  empowered_tiger_lightning_t( monk_t& p ) 
      : monk_spell_t( "empowered_tiger_lightning", &p, p.passives.empowered_tiger_lightning )
  {
    background = true;
    may_crit   = false;
  }

  bool ready() override
  {
    if ( p()->spec.invoke_xuen_2->ok() )
      return true;

    return false;
  }
};

// ==========================================================================
// Invoke Niuzao, the Black Ox
// ==========================================================================

struct niuzao_spell_t : public monk_spell_t
{
  niuzao_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "invoke_niuzao_the_black_ox", p, p->spec.invoke_niuzao )
  {
    parse_options( options_str );

    harmful            = false;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.niuzao->summon( p()->spec.invoke_niuzao->duration() );

    p()->buff.invoke_niuzao->trigger();

    if ( p()->legendary.invokers_delight->ok() )
      p()->buff.invokers_delight->trigger();
  }
};

// ==========================================================================
// Invoke Chi-Ji, the Red Crane
// ==========================================================================

struct chiji_spell_t : public monk_spell_t
{
  chiji_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "invoke_chiji_the_red_crane", p, p->talent.invoke_chi_ji )
  {
    parse_options( options_str );

    harmful            = false;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.chiji->summon( p()->talent.invoke_chi_ji->duration() );

    p()->buff.invoke_chiji->trigger();

    if ( p()->legendary.invokers_delight->ok() )
      p()->buff.invokers_delight->trigger();
  }
};

// ==========================================================================
// Invoke Yu'lon, the Jade Serpent
// ==========================================================================

struct yulon_spell_t : public monk_spell_t
{
  yulon_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "invoke_yulon_the_jade_serpent", p, p->spec.invoke_yulon )
  {
    parse_options( options_str );

    harmful            = false;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  bool ready() override
  {
    if ( !p()->talent.invoke_chi_ji->ok() )
      return monk_spell_t::ready();

    return false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.yulon->summon( p()->spec.invoke_yulon->duration() );

    if ( p()->legendary.invokers_delight->ok() )
      p()->buff.invokers_delight->trigger();
  }
};

// ==========================================================================
// Weapons of Order - Kyrian Covenant Ability
// ==========================================================================

struct weapons_of_order_t : public monk_spell_t
{
  weapons_of_order_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "weapons_of_order", &p, p.covenant.kyrian )
  {
    parse_options( options_str );
    may_combo_strike = true;
    harmful     = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  void execute() override
  {
    p()->buff.weapons_of_order->trigger();

    if ( p()->specialization() == MONK_BREWMASTER )
      p()->cooldown.keg_smash->reset( true, 1 );

    monk_spell_t::execute();
  }
};

// ==========================================================================
// Bonedust Brew - Necrolord Covenant Ability
// ==========================================================================

struct bonedust_brew_t : public monk_spell_t
{
  bonedust_brew_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "bonedust_brew", &p, p.covenant.necrolord )
  {
    parse_options( options_str );
    may_combo_strike = true;
    harmful          = false;
    aoe              = -1;
    base_dd_min      = 0;
    base_dd_max      = 0;
  }

  void execute() override
  {
    p()->buff.bonedust_brew_hidden->trigger();
    monk_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    td( s->target )->debuff.bonedust_brew->trigger();
  }
};

// ==========================================================================
// Bonedust Brew - Necrolord Covenant Damage
// ==========================================================================

struct bonedust_brew_damage_t : public monk_spell_t
{
  bonedust_brew_damage_t( monk_t& p )
    : monk_spell_t( "bonedust_brew_dmg", &p, p.passives.bonedust_brew_dmg )
  {
    background = true;
    may_crit   = false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->conduit.bone_marrow_hops->ok() && p()->buff.bonedust_brew_hidden->up() )
    {
      // Saved at -500
      p()->cooldown.bonedust_brew->adjust( p()->conduit.bone_marrow_hops->effectN( 2 ).time_value(), true );

      p()->buff.bonedust_brew_hidden->decrement();
    }
  }
};

// ==========================================================================
// Bonedust Brew - Necrolord Covenant Heal
// ==========================================================================

struct bonedust_brew_heal_t : public monk_heal_t
{
  bonedust_brew_heal_t( monk_t& p ) :
      monk_heal_t( "bonedust_brew_heal", p, p.passives.bonedust_brew_heal )
  {
    background = true;
  }

  void execute() override
  {
    monk_heal_t::execute();

    if ( p()->conduit.bone_marrow_hops->ok() && p()->buff.bonedust_brew_hidden->up() )
    {
      // Saved at -500
      p()->cooldown.bonedust_brew->adjust( p()->conduit.bone_marrow_hops->effectN( 2 ).time_value(), true );

      p()->buff.bonedust_brew_hidden->decrement();
    }
  }
};

// ==========================================================================
// Faeline Stomp - Ardenweald Covenant
// ==========================================================================

struct faeline_stomp_ww_damage_t : public monk_spell_t
{
  faeline_stomp_ww_damage_t( monk_t& p ) :
      monk_spell_t( "faeline_stomp_ww_dmg", &p, p.passives.faeline_stomp_ww_damage )
  {
    background = true;
    ww_mastery = true;
  }
};

struct faeline_stomp_damage_t : public monk_spell_t
{
  faeline_stomp_damage_t( monk_t& p )
    : monk_spell_t( "faeline_stomp_dmg", &p, p.passives.faeline_stomp_damage )
  {
    background = true;
    ww_mastery = true;

    attack_power_mod.direct = p.passives.faeline_stomp_damage->effectN( 1 ).ap_coeff();
    spell_power_mod.direct  = 0;
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = monk_spell_t::composite_aoe_multiplier( state );

    const std::vector<player_t*>& targets = state->action->target_list();

    if ( p()->conduit.way_of_the_fae->ok() && !targets.empty() )
    {
      cam *= 1 + ( p()->conduit.way_of_the_fae.percent() *
             std::min( (double)targets.size(), p()->conduit.way_of_the_fae->effectN( 2 ).base_value() ) );
    }

    return cam;
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    td( s->target )->debuff.faeline_stomp->trigger();
  }
};

struct faeline_stomp_t : public monk_spell_t
{
  faeline_stomp_damage_t* damage;
  faeline_stomp_ww_damage_t* ww_damage;
  faeline_stomp_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "faeline_stomp", &p, p.covenant.night_fae ),
      damage( new faeline_stomp_damage_t( p ) ),
      ww_damage( new faeline_stomp_ww_damage_t( p ) )
  {
    parse_options( options_str );
    may_combo_strike = true;
    aoe = p.covenant.night_fae->effectN( 3 ).base_value();
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    damage->set_target( s->target );
    damage->execute();

    p()->buff.faeline_stomp->trigger();

    switch ( p()->specialization() )
    {
      case MONK_WINDWALKER:
      {
        ww_damage->set_target( s->target );
        ww_damage->execute();
        break;
      }
      case MONK_BREWMASTER:
      {
        p()->buff.faeline_stomp_brm->trigger();
        break;
      }
      default:
        break;
    }
  }
};

// ==========================================================================
// Fallen Order - Venthyr Covenant Ability
// ==========================================================================

struct fallen_order_t : public monk_spell_t
{
  struct fallen_order_event_t : public event_t
  {
    std::vector<std::pair<specialization_e, timespan_t>> fallen_monks;
    timespan_t summon_interval;
    monk_t* p;

    fallen_order_event_t( monk_t* monk, std::vector<std::pair<specialization_e, timespan_t>> fm,
        timespan_t interval )
      : event_t( *monk, interval ),
        fallen_monks( fm ),
        summon_interval( interval ),
        p( monk )
    {
    }

    virtual const char* name() const override
    {
      return "fallen_order_summon";
    }

    void execute() override
    {
      if ( fallen_monks.empty() )
        return;

      std::pair<specialization_e, timespan_t> fallen_monk_pair;

      fallen_monk_pair = fallen_monks.front();

      switch (fallen_monk_pair.first)
      {
        case MONK_WINDWALKER:
          p->pets.fallen_monk_ww.spawn( fallen_monk_pair.second, 1 );
          break;
        case MONK_BREWMASTER:
          p->pets.fallen_monk_brm.spawn( fallen_monk_pair.second, 1 );
          break;
        case MONK_MISTWEAVER:
          p->pets.fallen_monk_mw.spawn( fallen_monk_pair.second, 1 );
          break;
        default:
          break;
      }
      fallen_monks.erase( fallen_monks.begin() );

      if ( !fallen_monks.empty() )
        make_event<fallen_order_event_t>( sim(), p, fallen_monks, summon_interval );
    }
  };

  fallen_order_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "fallen_order", &p, p.covenant.venthyr )
  {
    parse_options( options_str );
    harmful     = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  void execute() override
  {
    monk_spell_t::execute();

    specialization_e spec       = p()->specialization();
    timespan_t summon_duration  = timespan_t::from_seconds( p()->covenant.venthyr->effectN( 4 ).base_value() );
    timespan_t primary_duration = p()->passives.fallen_monk_spec_duration->duration();
    std::vector<std::pair<specialization_e, timespan_t>> fallen_monks;

    // Monks alternate summoning primary spec and non-primary spec
    // 11 summons in total (6 primary and a mix of 5 non-primary)
    // for non-primary, there is a 50% chance one or the other non-primary is summoned
    switch ( spec )
    {
      case MONK_WINDWALKER:
        fallen_monks.push_back( std::make_pair( MONK_WINDWALKER, primary_duration ) );
        break;
      case MONK_BREWMASTER:
        fallen_monks.push_back( std::make_pair( MONK_BREWMASTER, primary_duration ) );
        break;
      case MONK_MISTWEAVER:
        fallen_monks.push_back( std::make_pair( MONK_MISTWEAVER, primary_duration ) );
        break;
      default:
        break;
    }

    for ( int i = 0; i < 7; i++ )
    {
      switch ( spec )
      {
        case MONK_WINDWALKER:
        {
          if ( i % 2 )
            fallen_monks.push_back( std::make_pair( MONK_WINDWALKER, primary_duration ) );
          else if ( rng().roll( 0.75 ) )
            fallen_monks.push_back( std::make_pair( MONK_BREWMASTER, summon_duration ) );
          else
            fallen_monks.push_back( std::make_pair( MONK_MISTWEAVER, summon_duration ) );
          break;
        }
        case MONK_BREWMASTER:
        {
          if ( i % 2 )
            fallen_monks.push_back( std::make_pair( MONK_BREWMASTER, primary_duration ) );
          else if ( rng().roll( 0.75 ) )
            fallen_monks.push_back( std::make_pair( MONK_WINDWALKER, summon_duration ) );
          else
            fallen_monks.push_back( std::make_pair( MONK_MISTWEAVER, summon_duration ) );
          break;
        }
        case MONK_MISTWEAVER:
        {
          if ( i % 2 )
            fallen_monks.push_back( std::make_pair( MONK_MISTWEAVER, primary_duration ) );
          else if ( rng().roll( 0.75 ) )
            fallen_monks.push_back( std::make_pair( MONK_WINDWALKER, summon_duration ) );
          else
            fallen_monks.push_back( std::make_pair( MONK_BREWMASTER, summon_duration ) );
          break;
        }
        default:
          break;
      }
    }

    make_event<fallen_order_event_t>(
        *sim, p(), fallen_monks, p()->covenant.venthyr->effectN( 1 ).period() * 3 );
  }
};
}  // namespace spells

namespace heals
{
// ==========================================================================
// Soothing Mist
// ==========================================================================

struct soothing_mist_t : public monk_heal_t
{
  soothing_mist_t( monk_t& p ) : monk_heal_t( "soothing_mist", p, p.passives.soothing_mist_heal )
  {
    background = dual = true;

    tick_zero = true;
  }

  bool ready() override
  {
    if ( p()->buff.channeling_soothing_mist->check() )
      return false;

    return monk_heal_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    monk_heal_t::impact( s );

    p()->buff.channeling_soothing_mist->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    monk_heal_t::last_tick( d );

    p()->buff.channeling_soothing_mist->expire();
  }
};

// ==========================================================================
// Gust of Mists
// ==========================================================================
// The mastery actually affects the Spell Power Coefficient but I am not sure if that
// would work normally. Using Action Multiplier since it APPEARS to calculate the same.
//
// TODO: Double Check if this works.

struct gust_of_mists_t : public monk_heal_t
{
  gust_of_mists_t( monk_t& p ) : monk_heal_t( "gust_of_mists", p, p.mastery.gust_of_mists->effectN( 2 ).trigger() )
  {
    background = dual      = true;
    spell_power_mod.direct = 1;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    am *= p()->cache.mastery_value();

    // Mastery's Effect 3 gives a flat add modifier of 0.1 Spell Power co-efficient
    // TODO: Double check calculation

    return am;
  }
};

// ==========================================================================
// Enveloping Mist
// ==========================================================================

struct enveloping_mist_t : public monk_heal_t
{
  gust_of_mists_t* mastery;

  enveloping_mist_t( monk_t& p, util::string_view options_str )
    : monk_heal_t( "enveloping_mist", p, p.spec.enveloping_mist )
  {
    parse_options( options_str );

    may_miss = false;

    dot_duration = p.spec.enveloping_mist->duration();
    if ( p.talent.mist_wrap )
      dot_duration += p.talent.mist_wrap->effectN( 1 ).time_value();

    mastery = new gust_of_mists_t( p );
  }

  double cost() const override
  {
    double c = monk_heal_t::cost();

    if ( p()->buff.lifecycles_enveloping_mist->check() )
      c *= 1 + p()->buff.lifecycles_enveloping_mist->value();  // saved as -20%

    return c;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = monk_heal_t::execute_time();

    if ( p()->buff.invoke_chiji_evm->up() )
      et *= 1 + p()->buff.invoke_chiji_evm->stack_value();

    if ( p()->buff.thunder_focus_tea->check() )
      et *= 1 + p()->spec.thunder_focus_tea->effectN( 3 ).percent();  // saved as -100

    return et;
  }

  void execute() override
  {
    monk_heal_t::execute();

    if ( p()->talent.lifecycles->ok() )
    {
      if ( p()->buff.lifecycles_enveloping_mist->up() )
        p()->buff.lifecycles_enveloping_mist->expire();
      p()->buff.lifecycles_vivify->trigger();
    }

    if ( p()->buff.thunder_focus_tea->up() )
    {
      p()->buff.thunder_focus_tea->decrement();
    }

    mastery->execute();
  }
};

// ==========================================================================
// Renewing Mist
// ==========================================================================
/*
Bouncing only happens when overhealing, so not going to bother with bouncing
*/
struct renewing_mist_t : public monk_heal_t
{
  gust_of_mists_t* mastery;

  renewing_mist_t( monk_t& p, util::string_view options_str ) : monk_heal_t( "renewing_mist", p, p.spec.renewing_mist )
  {
    parse_options( options_str );
    may_crit = may_miss = false;
    dot_duration        = p.passives.renewing_mist_heal->duration();

    mastery = new gust_of_mists_t( p );
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown->duration;

    if ( p()->buff.thunder_focus_tea->check() )
      cd *= 1 + p()->spec.thunder_focus_tea->effectN( 1 ).percent();

    monk_heal_t::update_ready( cd );
  }

  void execute() override
  {
    monk_heal_t::execute();

    mastery->execute();

    if ( p()->buff.thunder_focus_tea->up() )
    {
      p()->buff.thunder_focus_tea->decrement();
    }
  }

  void tick( dot_t* d ) override
  {
    monk_heal_t::tick( d );

    p()->buff.uplifting_trance->trigger();
  }
};

// ==========================================================================
// Vivify
// ==========================================================================

struct vivify_t : public monk_heal_t
{
  gust_of_mists_t* mastery;

  vivify_t( monk_t& p, util::string_view options_str ) : monk_heal_t( "vivify", p, p.spec.vivify )
  {
    parse_options( options_str );

    // 1 for the primary target, plus the value of the effect
    aoe                    = (int)( 1 + data().effectN( 1 ).base_value() );
    spell_power_mod.direct = data().effectN( 2 ).sp_coeff();

    mastery = new gust_of_mists_t( p );

    may_miss = false;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p()->buff.uplifting_trance->up() )
      am *= 1 + p()->buff.uplifting_trance->value();

    if ( p()->spec.vivify_2_brm->ok() )
      am *= 1 + p()->spec.vivify_2_brm->effectN( 1 ).percent();

    if ( p()->spec.vivify_2_ww->ok() )
      am *= 1 + p()->spec.vivify_2_ww->effectN( 1 ).percent();

    return am;
  }

  double cost() const override
  {
    double c = monk_heal_t::cost();

    if ( p()->buff.thunder_focus_tea->check() && p()->spec.thunder_focus_tea_2->ok() )
      c *= 1 + p()->spec.thunder_focus_tea_2->effectN( 2 ).percent();  // saved as -100

    if ( p()->buff.lifecycles_vivify->check() )
      c *= 1 + p()->buff.lifecycles_vivify->value();  // saved as -25%

    return c;
  }

  void execute() override
  {
    monk_heal_t::execute();

    if ( p()->buff.thunder_focus_tea->up() )
    {
      p()->buff.thunder_focus_tea->decrement();
    }

    if ( p()->talent.lifecycles )
    {
      if ( p()->buff.lifecycles_vivify->up() )
        p()->buff.lifecycles_vivify->expire();

      p()->buff.lifecycles_enveloping_mist->trigger();
    }

    if ( p()->buff.uplifting_trance->up() )
      p()->buff.uplifting_trance->expire();

    if ( p()->mastery.gust_of_mists->ok() )
        mastery->execute();
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

struct essence_font_t : public monk_spell_t
{
  struct essence_font_heal_t : public monk_heal_t
  {
    essence_font_heal_t( monk_t& p )
      : monk_heal_t( "essence_font_heal", p, p.spec.essence_font->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe               = (int)p.spec.essence_font->effectN( 1 ).base_value();
    }

    double action_multiplier() const override
    {
      double am = monk_heal_t::action_multiplier();

      if ( p()->buff.refreshing_jade_wind->up() )
        am *= 1 + p()->buff.refreshing_jade_wind->value();

      return am;
    }
  };

  essence_font_heal_t* heal;

  essence_font_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "essence_font", p, p->spec.essence_font ), heal( new essence_font_heal_t( *p ) )
  {
    parse_options( options_str );

    may_miss = hasted_ticks = false;
    tick_zero               = true;

    base_tick_time = data().effectN( 1 ).base_value() * data().effectN( 1 ).period();

    add_child( heal );
  }
};

// ==========================================================================
// Revival
// ==========================================================================

struct revival_t : public monk_heal_t
{
  revival_t( monk_t& p, util::string_view options_str ) : monk_heal_t( "revival", p, p.spec.revival )
  {
    parse_options( options_str );

    may_miss = false;
    aoe      = -1;

    if ( sim->pvp_crit )
      base_multiplier *= 2;  // 08/03/2016
  }
};

// ==========================================================================
// Gift of the Ox
// ==========================================================================

struct gift_of_the_ox_t : public monk_heal_t
{
  gift_of_the_ox_t( monk_t& p, util::string_view options_str )
    : monk_heal_t( "gift_of_the_ox", p, p.passives.gift_of_the_ox_heal )
  {
    parse_options( options_str );
    harmful     = false;
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }

  bool ready() override
  {
    if ( p()->specialization() != MONK_BREWMASTER )
      return false;

    return p()->buff.gift_of_the_ox->check();
  }

  void execute() override
  {
    monk_heal_t::execute();

    p()->buff.gift_of_the_ox->decrement();
  }
};

struct gift_of_the_ox_trigger_t : public monk_heal_t
{
  gift_of_the_ox_trigger_t( monk_t& p ) : monk_heal_t( "gift_of_the_ox_trigger", p, p.find_spell( 124507 ) )
  {
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }
};

struct gift_of_the_ox_expire_t : public monk_heal_t
{
  gift_of_the_ox_expire_t( monk_t& p ) : monk_heal_t( "gift_of_the_ox_expire", p, p.find_spell( 178173 ) )
  {
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }
};

// ==========================================================================
// Expel Harm
// ==========================================================================

struct niuzaos_blessing_t : public monk_heal_t
{
  niuzaos_blessing_t( monk_t& p ) : monk_heal_t( "niuzaos_blessing", p, p.find_spell( 278535 ) )
  {
    background = true;
    proc       = true;
    target     = player;
  }
};

struct expel_harm_dmg_t : public monk_spell_t
{
  expel_harm_dmg_t( monk_t* player ) : monk_spell_t( "expel_harm_damage", player, player->find_spell( 115129 ) )
  {
    background = true;
    may_crit   = false;
  }
 };

struct expel_harm_t : public monk_heal_t
{
  expel_harm_dmg_t* dmg;
  expel_harm_t( monk_t& p, util::string_view options_str )
    : monk_heal_t( "expel_harm", p, p.spec.expel_harm ),
      dmg( new expel_harm_dmg_t( &p ) )
  {
    parse_options( options_str );

    target           = player;
    may_combo_strike = true;

    if ( p.spec.expel_harm_2_brm->ok() )
      cooldown->duration += p.spec.expel_harm_2_brm->effectN( 1 ).time_value();

    add_child( dmg );
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p()->conduit.harm_denial->ok() )
      am *= 1 + p()->conduit.harm_denial.percent();

    return am;
  }

  void execute() override
  {
    monk_heal_t::execute();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      double chi_gain = 0;

      if ( p()->spec.expel_harm_2_ww->ok() )
        chi_gain += p()->spec.expel_harm_2_ww->effectN( 1 ).base_value();

      if (chi_gain > 0 )
        p()->resource_gain( RESOURCE_CHI, chi_gain, p()->gain.expel_harm );
    }
  }

  void impact( action_state_t* s ) override
  {
    monk_heal_t::impact( s );

    double result = s->result_total;

    result *= p()->spec.expel_harm->effectN( 2 ).percent();

    // Defaults to 1 but if someone wants to adjust the amount of damage
    result *= p()->user_options.expel_harm_effectiveness;

    // Have to manually set the combo strike mastery multiplier
    if ( p()->buff.combo_strikes->up() )
      result *= 1 + p()->cache.mastery_value();

    if ( p()->buff.gift_of_the_ox->up() && p()->spec.expel_harm_2_brm->ok() )
    {
      double goto_heal = p()->passives.gift_of_the_ox_heal->effectN( 1 ).ap_coeff();
      goto_heal *= p()->buff.gift_of_the_ox->stack();
      result += goto_heal;
    }

    dmg->base_dd_min = result;
    dmg->base_dd_max = result;
    dmg->execute();

    for ( int i = 0; i < p()->buff.gift_of_the_ox->stack(); i++ )
    {
      p()->buff.gift_of_the_ox->decrement();
    }
  }
};

// ==========================================================================
// Zen Pulse
// ==========================================================================

struct zen_pulse_heal_t : public monk_heal_t
{
  zen_pulse_heal_t( monk_t& p ) : monk_heal_t( "zen_pulse_heal", p, p.passives.zen_pulse_heal )
  {
    background = true;
    may_crit = may_miss = false;
    target              = &( p );
  }
};

struct zen_pulse_dmg_t : public monk_spell_t
{
  zen_pulse_heal_t* heal;
  zen_pulse_dmg_t( monk_t* player ) : monk_spell_t( "zen_pulse_damage", player, player->talent.zen_pulse )
  {
    background = true;
    aoe        = -1;

    heal = new zen_pulse_heal_t( *player );
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    heal->base_dd_min = s->result_amount;
    heal->base_dd_max = s->result_amount;
    heal->execute();
  }
};

struct zen_pulse_t : public monk_spell_t
{
  spell_t* damage;
  zen_pulse_t( monk_t* player ) : monk_spell_t( "zen_pulse", player, player->talent.zen_pulse )
  {
    may_miss = may_dodge = may_parry = false;
    damage                           = new zen_pulse_dmg_t( player );
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    damage->execute();
  }
};

// ==========================================================================
// Chi Wave
// ==========================================================================

struct chi_wave_heal_tick_t : public monk_heal_t
{
  chi_wave_heal_tick_t( monk_t& p, const std::string& name ) : monk_heal_t( name, p, p.passives.chi_wave_heal )
  {
    background = direct_tick = true;
    target                   = player;
  }
};

struct chi_wave_dmg_tick_t : public monk_spell_t
{
  chi_wave_dmg_tick_t( monk_t* player, const std::string& name )
    : monk_spell_t( name, player, player->passives.chi_wave_damage )
  {
    background              = true;
    ww_mastery              = true;
    attack_power_mod.direct = player->passives.chi_wave_damage->effectN( 1 ).ap_coeff();
    attack_power_mod.tick   = 0;
  }
};

struct chi_wave_t : public monk_spell_t
{
  heal_t* heal;
  spell_t* damage;
  bool dmg;
  chi_wave_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "chi_wave", player, player->talent.chi_wave ),
      heal( new chi_wave_heal_tick_t( *player, "chi_wave_heal" ) ),
      damage( new chi_wave_dmg_tick_t( player, "chi_wave_damage" ) ),
      dmg( true )
  {
    sef_ability      = SEF_CHI_WAVE;
    may_combo_strike = true;
    parse_options( options_str );
    hasted_ticks = harmful = false;
    cooldown->hasted       = false;
    dot_duration           = timespan_t::from_seconds( player->talent.chi_wave->effectN( 1 ).base_value() );
    base_tick_time         = timespan_t::from_seconds( 1.0 );
    add_child( heal );
    add_child( damage );
    tick_zero = true;
    radius    = player->find_spell( 132466 )->effectN( 2 ).base_value();
    gcd_type  = gcd_haste_type::SPELL_HASTE;
  }

  void impact( action_state_t* s ) override
  {
    dmg = true;  // Set flag so that the first tick does damage

    monk_spell_t::impact( s );
  }

  void tick( dot_t* d ) override
  {
    monk_spell_t::tick( d );
    // Select appropriate tick action
    if ( dmg )
      damage->execute();
    else
      heal->execute();

    dmg = !dmg;  // Invert flag for next use
  }
};

// ==========================================================================
// Chi Burst
// ==========================================================================

struct chi_burst_heal_t : public monk_heal_t
{
  chi_burst_heal_t( monk_t& player ) : monk_heal_t( "chi_burst_heal", player, player.passives.chi_burst_heal )
  {
    background = true;
    target     = p();
    aoe        = -1;
  }
};

struct chi_burst_damage_t : public monk_spell_t
{
  int num_hit;
  chi_burst_damage_t( monk_t& player ) : monk_spell_t( "chi_burst_damage", &player, player.passives.chi_burst_damage ),
      num_hit( 0 )
  {
    background = true;
    ww_mastery = true;
    aoe        = -1;
  }

  void execute() override
  {
    num_hit = 0;

    monk_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    num_hit++;

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( num_hit <= p()->talent.chi_burst->effectN( 3 ).base_value() )
        p()->resource_gain( RESOURCE_CHI, p()->find_spell( 261682 )->effectN( 1 ).base_value(), p()->gain.chi_burst );
    }
  }
};

struct chi_burst_t : public monk_spell_t
{
  chi_burst_heal_t* heal;
  chi_burst_damage_t* damage;
  chi_burst_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "chi_burst", player, player->talent.chi_burst ),
      heal( nullptr )
  {
    parse_options( options_str );
    may_combo_strike = true;
    heal             = new chi_burst_heal_t( *player );
    heal->stats      = stats;
    damage           = new chi_burst_damage_t( *player );
    damage->stats    = stats;
    cooldown->hasted = false;

    interrupt_auto_attack = false;
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  bool ready() override
  {
    if ( p()->talent.chi_burst->ok() )
      return monk_spell_t::ready();

    return false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    heal->execute();
    damage->execute();
  }
};

// ==========================================================================
// Healing Elixirs
// ==========================================================================

struct healing_elixir_t : public monk_heal_t
{
  healing_elixir_t( monk_t& p ) : monk_heal_t( "healing_elixir", p, p.talent.healing_elixir )
  {
    harmful = may_crit = false;
    target             = &p;
    base_pct_heal      = p.talent.healing_elixir->effectN( 1 ).percent();
  }
};

// ==========================================================================
// Refreshing Jade Wind
// ==========================================================================

struct refreshing_jade_wind_heal_t : public monk_heal_t
{
  refreshing_jade_wind_heal_t( monk_t& player )
    : monk_heal_t( "refreshing_jade_wind_heal", player, player.talent.refreshing_jade_wind->effectN( 1 ).trigger() )
  {
    background = true;
    aoe        = 6;
  }
};

struct refreshing_jade_wind_t : public monk_spell_t
{
  refreshing_jade_wind_heal_t* heal;
  refreshing_jade_wind_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "refreshing_jade_wind", player, player->talent.refreshing_jade_wind )
  {
    parse_options( options_str );
    heal = new refreshing_jade_wind_heal_t( *player );
  }

  void tick( dot_t* d ) override
  {
    monk_spell_t::tick( d );

    heal->execute();
  }
};

// ==========================================================================
// Celestial Fortune
// ==========================================================================
// This is a Brewmaster-specific critical strike effect

struct celestial_fortune_t : public monk_heal_t
{
  celestial_fortune_t( monk_t& p ) : monk_heal_t( "celestial_fortune", p, p.passives.celestial_fortune )
  {
    background = true;
    proc       = true;
    target     = player;
    may_crit   = false;

    base_multiplier = p.spec.celestial_fortune->effectN( 1 ).percent();
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    monk_heal_t::init();
    // disable the snapshot_flags for all multipliers, but specifically allow
    // action_multiplier() to be called so we can override.
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_MUL_DA;
  }
};

// ==========================================================================
// Evasive Stride Conduit
// ==========================================================================

struct evasive_stride_t : public monk_heal_t
{
  evasive_stride_t( monk_t& p ) : monk_heal_t( "evasive_stride", p, p.passives.evasive_stride )
  {
    background  = true;
    proc        = true;
    target      = player;
  }
};

}  // end namespace heals

namespace absorbs
{
// ==========================================================================
// Celestial Brew
// ==========================================================================

struct special_delivery_t : public monk_spell_t
{
  special_delivery_t( monk_t& p ) : monk_spell_t( "special_delivery", &p, p.passives.special_delivery )
  {
    may_block = may_dodge = may_parry = true;
    background                        = true;
    trigger_gcd                       = timespan_t::zero();
    aoe                               = -1;
    attack_power_mod.direct           = data().effectN( 1 ).ap_coeff();
    radius                            = data().effectN( 1 ).radius();
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( p()->talent.special_delivery->effectN( 1 ).base_value() );
  }

  double cost() const override
  {
    return 0;
  }
};

struct celestial_brew_t : public monk_absorb_t
{
  special_delivery_t* delivery;

  celestial_brew_t( monk_t& p, util::string_view options_str )
    : monk_absorb_t( "celestial_brew", p, p.spec.celestial_brew )
  {
    parse_options( options_str );
    harmful = may_crit = false;

    if ( p.talent.light_brewing->ok() )
      cooldown->duration *= 1 + p.talent.light_brewing->effectN( 2 ).percent();  // -20

    if ( p.talent.special_delivery->ok() )
      delivery = new special_delivery_t( p );
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( p()->buff.purified_chi->up() )
      am *= 1 + p()->buff.purified_chi->stack_value();

    return am;
  }

  void execute() override
  {

    monk_absorb_t::execute();

    if ( p()->talent.special_delivery->ok() )
    {
      delivery->set_target( target );
      delivery->execute();
    }

    if ( p()->talent.celestial_flames->ok() )
      p()->buff.celestial_flames->trigger();

    if ( p()->buff.blackout_combo->up() )
    {
      p()->active_actions.stagger_self_damage->delay_tick(
          timespan_t::from_seconds( p()->buff.blackout_combo->data().effectN( 4 ).base_value() ) );
      p()->buff.blackout_combo->expire();
    }

    if ( p()->legendary.celestial_infusion->ok() )
      p()->buff.mighty_pour->trigger();
  }
};

// ==========================================================================
// Life Cocoon
// ==========================================================================
struct life_cocoon_t : public monk_absorb_t
{
  life_cocoon_t( monk_t& p, util::string_view options_str ) : monk_absorb_t( "life_cocoon", p, p.spec.life_cocoon )
  {
    parse_options( options_str );
    harmful = may_crit     = false;

    base_dd_min = p.resources.max[ RESOURCE_HEALTH ] * p.spec.life_cocoon->effectN( 3 ).percent();
    base_dd_max = base_dd_min;
  }


  void impact( action_state_t* s ) override
  {
    p()->buff.life_cocoon->trigger( 1, s->result_amount );
    stats->add_result( 0.0, s->result_amount, result_amount_type::ABSORB, s->result, s->block_result, s->target );
  }
};
}  // end namespace absorbs

using namespace pets;
using namespace pet_summon;
using namespace attacks;
using namespace spells;
using namespace heals;
using namespace absorbs;
}  // namespace actions

// ==========================================================================
// Monk Buffs
// ==========================================================================

namespace buffs
{
template <typename buff_t>
struct monk_buff_t : public buff_t
{
public:
  using base_t = monk_buff_t;

  monk_buff_t( monk_td_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(),
               const item_t* item = nullptr )
    : buff_t( p, name, s, item )
  {
  }

  monk_buff_t( monk_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(),
               const item_t* item = nullptr )
    : buff_t( &p, name, s, item )
  {
  }

  monk_td_t& get_td( player_t* t )
  {
    return *( p().get_target_data( t ) );
  }

  const monk_td_t& get_td( player_t* t ) const
  {
    return *( p().get_target_data( t ) );
  }

  monk_t& p()
  {
    return *debug_cast<monk_t*>( buff_t::source );
  }

  const monk_t& p() const
  {
    return *debug_cast<monk_t*>( buff_t::source );
  }
};

// ===============================================================================
// Fortifying Brew Buff
// ===============================================================================
struct fortifying_brew_t : public monk_buff_t<buff_t>
{
  int health_gain;
  fortifying_brew_t( monk_t& p, const std::string& n, const spell_data_t* s ) : monk_buff_t( p, n, s ), health_gain( 0 )
  {
    cooldown->duration = timespan_t::zero();
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    double health_multiplier = p().spec.fortifying_brew_mw_ww->effectN( 1 ).percent();

    if ( p().specialization() == MONK_BREWMASTER && p().spec.fortifying_brew_2_brm->ok() )
    {
      // The tooltip is hard-coded with 20% if Brewmaster Rank 2 is activated
      // Currently it's bugged and giving 17.39% HP instead of the intended 20%
      // The intended calculation is:
      // health_multiplier = health_multiplier * ( 0.20 * ( 1 / (1 + health_multiplier ) ) );
      health_multiplier = 0.20 * ( 1 / (1 + health_multiplier) );
    }

    // Extra Health is set by current max_health, doesn't change when max_health changes.
    health_gain = static_cast<int>( p().resources.max[ RESOURCE_HEALTH ] * health_multiplier );

    p().stat_gain( STAT_MAX_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    p().stat_gain( STAT_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    p().stat_loss( STAT_MAX_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    p().stat_loss( STAT_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
  }
};

// ===============================================================================
// Serenity Buff 
// ===============================================================================
struct serenity_buff_t : public monk_buff_t<buff_t>
{
  monk_t& m;
  serenity_buff_t( monk_t& p, const std::string& n, const spell_data_t* s )
    : monk_buff_t( p, n, s ), m( p )
  {
    set_default_value_from_effect( 2 );
    set_cooldown( timespan_t::zero() );

    set_duration( s->duration() );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
    set_stack_change_callback( [ this ]( buff_t*, int, int ) {
        range::for_each( m.serenity_cooldowns, []( cooldown_t* cd ) { cd->adjust_recharge_multiplier(); } );
    } );
  }
};

// ===============================================================================
// Touch of Karma Buff 
// ===============================================================================
struct touch_of_karma_buff_t : public monk_buff_t<buff_t>
{
  touch_of_karma_buff_t( monk_t& p, const std::string& n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    default_value = 0;
    set_cooldown( timespan_t::zero() );

    set_duration( s->duration() );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    // Make sure the value is reset upon each trigger
    current_value = 0;

    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Rushing Jade Wind Buff 
// ===============================================================================
struct rushing_jade_wind_buff_t : public monk_buff_t<buff_t>
{
  // gonna assume this is 1 buff per monk combatant
  timespan_t _period;

  static void rjw_callback( buff_t* b, int, timespan_t )
  {
    monk_t* p = debug_cast<monk_t*>( b->player );

    p->active_actions.rushing_jade_wind->execute();
  }

  rushing_jade_wind_buff_t( monk_t& p, const std::string& n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_can_cancel( true );
    set_tick_zero( true );
    set_cooldown( timespan_t::zero() );

    set_period( s->effectN( 1 ).period() );
    set_tick_time_behavior( buff_tick_time_behavior::CUSTOM );
    set_tick_time_callback( [&]( const buff_t*, unsigned int ) { return _period; } );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    set_partial_tick( true );

    if ( p.specialization() == MONK_BREWMASTER )
      set_duration( s->duration() * ( 1 + p.spec.brewmaster_monk->effectN( 9 ).percent() ) );
    else
      set_duration( s->duration() );

    set_tick_callback( rjw_callback );
    set_tick_behavior( buff_tick_behavior::REFRESH );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    duration = ( duration >= timespan_t::zero() ? duration : this->buff_duration() ) * p().cache.spell_speed();
    // RJW snapshots the tick period on cast. this + the tick_time
    // callback represent that behavior
    _period = this->buff_period * p().cache.spell_speed();
    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Gift of the Ox Buff
// ===============================================================================
struct gift_of_the_ox_buff_t : public monk_buff_t<buff_t>
{
  gift_of_the_ox_buff_t( monk_t& p, const std::string& n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_can_cancel( false );
    set_cooldown( timespan_t::zero() );
    stack_behavior = buff_stack_behavior::ASYNCHRONOUS;

    set_refresh_behavior( buff_refresh_behavior::NONE );

    set_duration( p.find_spell( 124503 )->duration() );
    set_max_stack( 99 );
  }

  void decrement( int stacks, double value ) override
  {
    monk_t* p = debug_cast<monk_t*>( player );
    if ( stacks > 0 )
    {
      p->active_actions.gift_of_the_ox_trigger->execute();

      buff_t::decrement( stacks, value );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    monk_t* p = debug_cast<monk_t*>( player );

    p->active_actions.gift_of_the_ox_expire->execute();

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Xuen Rank 2 Empowered Tiger Lightning Buff
// ===============================================================================
struct invoke_xuen_the_white_tiger_buff_t : public monk_buff_t<buff_t>
{
  static void invoke_xuen_callback( buff_t* b, int, timespan_t )
  {
    monk_t* p = debug_cast<monk_t*>( b->player );
    double empowered_tiger_lightning_multiplier = p->spec.invoke_xuen_2->effectN( 2 ).percent();

    for ( auto target : p->sim->target_non_sleeping_list )
    {
      if ( p->get_target_data( target )->debuff.empowered_tiger_lightning->up() )
      {
        double value = p->get_target_data( target )->debuff.empowered_tiger_lightning->value();
        p->get_target_data( target )->debuff.empowered_tiger_lightning->current_value = 0;
        if ( value > 0 )
        {
          p->active_actions.empowered_tiger_lightning->set_target( target );
          p->active_actions.empowered_tiger_lightning->base_dd_min = value * empowered_tiger_lightning_multiplier;
          p->active_actions.empowered_tiger_lightning->base_dd_max = value * empowered_tiger_lightning_multiplier;
          p->active_actions.empowered_tiger_lightning->execute();
        }
      }
    }
  }

  invoke_xuen_the_white_tiger_buff_t( monk_t& p, const std::string& n, const spell_data_t* s ) 
      : monk_buff_t( p, n, s )
  {
    set_cooldown( timespan_t::zero() );
    set_refresh_behavior( buff_refresh_behavior::NONE );

    set_period( p.spec.invoke_xuen->effectN( 2 ).period() );

    set_tick_callback( invoke_xuen_callback );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Niuzao Rank 2 Purifying Buff
// ===============================================================================
struct purifying_buff_t : public monk_buff_t<buff_t>
{
  std::deque<double> values;
  // tracking variable for debug code
  bool ignore_empty;
  purifying_buff_t( monk_t& p, const std::string& n, const spell_data_t* s ) 
      : monk_buff_t( p, n, s )
  {
    set_can_cancel( true );
    set_quiet (true );
    set_cooldown( timespan_t::zero() );
    set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

    set_refresh_behavior( buff_refresh_behavior::NONE );

    set_duration( timespan_t::from_seconds( 6 ) );
    set_max_stack( 99 );

    ignore_empty = false;
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    ignore_empty = false;
    p().sim->print_debug("adding recent purify (amount: {})", value);
    // Make sure the value is reset upon each trigger
    current_value = 0;

    values.push_back( value );

    return buff_t::trigger( stacks, value, chance, duration );
  }

  double value() override
  {
    double total_value = 0;

    if ( !values.empty() )
    {
      for ( auto& i : values )
        total_value += i;
    }

    return total_value;
  }

  void decrement( int stacks, double value ) override
  {
    if (values.empty()) {
      // decrement ends up being called after expire_override sometimes. if this
      // debug msg is showing, then we have an error besides that that is
      // leading to stack/queue mismatches
      if (!ignore_empty) {
        p().sim->print_debug("purifying_buff decrement called with no values in queue!");
      }
    } else {
      values.pop_front();
    }
    buff_t::decrement( stacks, value );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    ignore_empty = true;
    values.clear();
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Windwalking Buff
// ===============================================================================
struct windwalking_driver_t : public monk_buff_t<buff_t>
{
  double movement_increase;
  windwalking_driver_t( monk_t& p, const std::string& n, const spell_data_t* s )
    : monk_buff_t( p, n, s ), movement_increase( 0 )
  {
    set_tick_callback( [&p, this]( buff_t*, int /* total_ticks */, timespan_t /* tick_time */ ) {
      range::for_each( p.windwalking_aura->target_list(), [&p, this]( player_t* target ) {
        target->buffs.windwalking_movement_aura->trigger(
            1,
            ( movement_increase  ),
            1, timespan_t::from_seconds( 10 ) );
      } );
    } );
    set_cooldown( timespan_t::zero() );
    set_duration( timespan_t::zero() );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_behavior( buff_tick_behavior::CLIP );
    movement_increase = p.buffs.windwalking_movement_aura->data().effectN( 1 ).percent();
  }
};

// ===============================================================================
// Stagger Buff
// ===============================================================================
struct stagger_buff_t : public monk_buff_t<buff_t>
{
  stagger_buff_t( monk_t& p, const std::string& n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    timespan_t stagger_duration = s->duration();
    stagger_duration += timespan_t::from_seconds( p.talent.bob_and_weave->effectN( 1 ).base_value() / 10 );

    // set_duration(stagger_duration);
    set_duration( timespan_t::zero() );
    if ( p.talent.high_tolerance->ok() )
    {
      add_invalidate( CACHE_HASTE );
    }
  }
};

}  // namespace buffs

namespace items
{
// MONK MODULE INTERFACE ====================================================

void do_trinket_init( monk_t* player, specialization_e spec, const special_effect_t*& ptr,
                      const special_effect_t& effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !player->find_spell( effect.spell_id )->ok() || player->specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

void init()
{
}
}  // namespace items

// ==========================================================================
// Monk Character Definition
// ==========================================================================

// Debuffs ==================================================================
monk_td_t::monk_td_t( player_t* target, monk_t* p )
  : actor_target_data_t( target, p ), dots( dots_t() ), debuff( buffs_t() ), monk( *p )
{
  if ( p->specialization() == MONK_WINDWALKER )
  {
    debuff.flying_serpent_kick = make_buff( *this, "flying_serpent_kick", p->passives.flying_serpent_kick_damage )
                                     ->set_default_value_from_effect( 2 );
    debuff.empowered_tiger_lightning = make_buff( *this, "empowered_tiger_lightning", spell_data_t::nil() )
                                           ->set_quiet( true )
                                           ->set_cooldown( timespan_t::zero() )
                                           ->set_refresh_behavior( buff_refresh_behavior::NONE )
                                           ->set_max_stack( 1 )
                                           ->set_default_value( 0 );

    debuff.mark_of_the_crane = make_buff( *this, "mark_of_the_crane", p->passives.mark_of_the_crane )
                                   ->set_default_value( p->passives.cyclone_strikes->effectN( 1 ).percent() )
                                   ->set_refresh_behavior( buff_refresh_behavior::DURATION );
    debuff.touch_of_karma = make_buff( *this, "touch_of_karma_debuff", p->spec.touch_of_karma )
                                // set the percent of the max hp as the default value.
                                ->set_default_value_from_effect( 3 );
    debuff.touch_of_karma->default_value_effect_multiplier *=
        1 + ( p->talent.good_karma->ok() ? p->talent.good_karma->effectN( 1 ).percent() : 0 );
  }

  if ( p->specialization() == MONK_BREWMASTER )
  {
    debuff.keg_smash = make_buff( *this, "keg_smash", p->spec.keg_smash )
                           ->set_cooldown( timespan_t::zero() )
                           ->set_default_value_from_effect( 3 );

    debuff.exploding_keg = make_buff( *this, "exploding_keg", p->talent.exploding_keg )
                               ->set_cooldown(timespan_t::zero());
  }

  // Covenant Abilities
  debuff.bonedust_brew = make_buff( *this, "bonedust_brew", p->covenant.necrolord )
                             ->set_cooldown( timespan_t::zero() )
                             ->set_chance( 1.0 )
                             ->set_default_value_from_effect( 3 );

  debuff.faeline_stomp = make_buff( *this, "faeline_stomp_debuff", p->find_spell( 327257 ) );

  debuff.fallen_monk_keg_smash = make_buff( *this, "fallen_monk_keg_smash", p->passives.fallen_monk_keg_smash )
                                     ->set_default_value_from_effect( 3 );

  debuff.weapons_of_order = make_buff( *this, "weapons_of_order_debuff", p->find_spell( 312106 ) )
                                ->set_default_value_from_effect( 1 );

  // Shadowland Legendary
  debuff.keefers_skyreach = make_buff( *this, "keefers_skyreach", p->find_spell( 344021 ) )
                                  ->set_default_value_from_effect( 1 )
                                  ->add_invalidate( CACHE_ATTACK_CRIT_CHANCE )
                                  ->set_refresh_behavior( buff_refresh_behavior::NONE );
  debuff.recently_rushing_tiger_palm = make_buff( *this, "recently_rushing_tiger_palm", p->find_spell( 337341 ) )
                                            ->set_refresh_behavior( buff_refresh_behavior::NONE );

  debuff.storm_earth_and_fire = make_buff( *this, "storm_earth_and_fire_target" )->set_cooldown( timespan_t::zero() );

  dots.breath_of_fire          = target->get_dot( "breath_of_fire_dot", p );
  dots.enveloping_mist         = target->get_dot( "enveloping_mist", p );
  dots.eye_of_the_tiger_damage = target->get_dot( "eye_of_the_tiger_damage", p );
  dots.eye_of_the_tiger_heal   = target->get_dot( "eye_of_the_tiger_heal", p );
  dots.renewing_mist           = target->get_dot( "renewing_mist", p );
  dots.rushing_jade_wind       = target->get_dot( "rushing_jade_wind", p );
  dots.soothing_mist           = target->get_dot( "soothing_mist", p );
  dots.touch_of_karma          = target->get_dot( "touch_of_karma", p );
}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( util::string_view name, const std::string& options_str )
{
  using namespace actions;
  // General
  if ( name == "snapshot_stats" )
    return new monk_snapshot_stats_t( this, options_str );
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "crackling_jade_lightning" )
    return new crackling_jade_lightning_t( *this, options_str );
  if ( name == "tiger_palm" )
    return new tiger_palm_t( this, options_str );
  if ( name == "blackout_kick" )
    return new blackout_kick_t( this, options_str );
  if ( name == "expel_harm" )
    return new expel_harm_t( *this, options_str );
  if ( name == "leg_sweep" )
    return new leg_sweep_t( this, options_str );
  if ( name == "paralysis" )
    return new paralysis_t( this, options_str );
  if ( name == "rising_sun_kick" )
    return new rising_sun_kick_t( this, options_str );
  if ( name == "roll" )
    return new roll_t( this, options_str );
  if ( name == "spear_hand_strike" )
    return new spear_hand_strike_t( this, options_str );
  if ( name == "spinning_crane_kick" )
    return new spinning_crane_kick_t( this, options_str );
  if ( name == "vivify" )
    return new vivify_t( *this, options_str );

  // Brewmaster
  if ( name == "breath_of_fire" )
    return new breath_of_fire_t( *this, options_str );
  if ( name == "celestial_brew" )
    return new celestial_brew_t( *this, options_str );
  if ( name == "exploding_keg" )
    return new exploding_keg_t( *this, options_str );
  if ( name == "fortifying_brew" )
    return new fortifying_brew_t( *this, options_str );
  if ( name == "gift_of_the_ox" )
    return new gift_of_the_ox_t( *this, options_str );
  if ( name == "invoke_niuzao" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "invoke_niuzao_the_black_ox" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "keg_smash" )
    return new keg_smash_t( *this, options_str );
  if ( name == "purifying_brew" )
    return new purifying_brew_t( *this, options_str );
  if ( name == "provoke" )
    return new provoke_t( this, options_str );

  // Mistweaver
  if ( name == "enveloping_mist" )
    return new enveloping_mist_t( *this, options_str );
  if ( name == "essence_font" )
    return new essence_font_t( this, options_str );
  if ( name == "invoke_chiji" )
    return new chiji_spell_t( this, options_str );
  if ( name == "invoke_chiji_the_red_crane" )
    return new chiji_spell_t( this, options_str );
  if ( name == "invoke_yulon" )
    return new yulon_spell_t( this, options_str );
  if ( name == "invoke_yulon_the_jade_serpent" )
    return new yulon_spell_t( this, options_str );
  if ( name == "life_cocoon" )
    return new life_cocoon_t( *this, options_str );
  if ( name == "mana_tea" )
    return new mana_tea_t( *this, options_str );
  if ( name == "renewing_mist" )
    return new renewing_mist_t( *this, options_str );
  if ( name == "revival" )
    return new revival_t( *this, options_str );
  if ( name == "thunder_focus_tea" )
    return new thunder_focus_tea_t( *this, options_str );

  // Windwalker
  if ( name == "fists_of_fury" )
    return new fists_of_fury_t( this, options_str );
  if ( name == "flying_serpent_kick" )
    return new flying_serpent_kick_t( this, options_str );
  if ( name == "touch_of_karma" )
    return new touch_of_karma_t( this, options_str );
  if ( name == "touch_of_death" )
    return new touch_of_death_t( *this, options_str );
  if ( name == "storm_earth_and_fire" )
    return new storm_earth_and_fire_t( this, options_str );
  if ( name == "storm_earth_and_fire_fixate" )
    return new storm_earth_and_fire_fixate_t( this, options_str );

  // Talents
  if ( name == "chi_burst" )
    return new chi_burst_t( this, options_str );
  if ( name == "chi_torpedo" )
    return new chi_torpedo_t( this, options_str );
  if ( name == "chi_wave" )
    return new chi_wave_t( this, options_str );
  if ( name == "black_ox_brew" )
    return new black_ox_brew_t( this, options_str );
  if ( name == "dampen_harm" )
    return new dampen_harm_t( *this, options_str );
  if ( name == "diffuse_magic" )
    return new diffuse_magic_t( *this, options_str );
  if ( name == "energizing_elixir" )
    return new energizing_elixir_t( this, options_str );
  if ( name == "fist_of_the_white_tiger" )
    return new fist_of_the_white_tiger_t( this, options_str );
  if ( name == "invoke_xuen" )
    return new xuen_spell_t( this, options_str );
  if ( name == "invoke_xuen_the_white_tiger" )
    return new xuen_spell_t( this, options_str );
  if ( name == "refreshing_jade_wind" )
    return new refreshing_jade_wind_t( this, options_str );
  if ( name == "rushing_jade_wind" )
    return new rushing_jade_wind_t( this, options_str );
  if ( name == "whirling_dragon_punch" )
    return new whirling_dragon_punch_t( this, options_str );
  if ( name == "serenity" )
    return new serenity_t( this, options_str );

  // Covenant Abilities
  if ( name == "bonedust_brew" )
    return new bonedust_brew_t( *this, options_str );
  if ( name == "faeline_stomp" )
    return new faeline_stomp_t( *this, options_str );
  if ( name == "fallen_order" )
    return new fallen_order_t( *this, options_str );
  if ( name == "weapons_of_order" )
    return new weapons_of_order_t( *this, options_str );

  return base_t::create_action( name, options_str );
}

void monk_t::trigger_celestial_fortune( action_state_t* s )
{
  if ( !spec.celestial_fortune->ok() || s->action == active_actions.celestial_fortune || s->result_raw == 0.0 )
  {
    return;
  }

  // flush out percent heals
  if ( s->action->type == ACTION_HEAL )
  {
    heal_t* heal_cast = debug_cast<heal_t*>( s->action );
    if ( ( s->result_type == result_amount_type::HEAL_DIRECT && heal_cast->base_pct_heal > 0 ) ||
         ( s->result_type == result_amount_type::HEAL_OVER_TIME && heal_cast->tick_pct_heal > 0 ) )
      return;
  }

  // Attempt to proc the heal
  if ( active_actions.celestial_fortune && rng().roll( composite_melee_crit_chance() ) )
  {
    active_actions.celestial_fortune->base_dd_max = active_actions.celestial_fortune->base_dd_min = s->result_amount;
    active_actions.celestial_fortune->schedule_execute();
  }
}

void monk_t::trigger_mark_of_the_crane( action_state_t* s )
{
  if ( get_target_data( s->target )->debuff.mark_of_the_crane->up() ||
       mark_of_the_crane_counter() < as<int>( passives.cyclone_strikes->max_stacks() ) )
    get_target_data( s->target )->debuff.mark_of_the_crane->trigger();
}

player_t* monk_t::next_mark_of_the_crane_target( action_state_t* state )
{
  std::vector<player_t*> targets = state->action->target_list();
  if ( targets.empty() )
  {
    return nullptr;
  }
  if ( targets.size() > 1 )
  {
    // Have the SEF converge onto the the cleave target if there are only 2 targets
    if ( targets.size() == 2 )
      return targets[ 1 ];
    // Don't move the SEF if there is only 3 targets
    if ( targets.size() == 3 )
      return state->target;

    // First of all find targets that do not have the cyclone strike debuff applied and send the SEF to those targets
    for ( player_t* target : targets )
    {
      if ( !get_target_data( target )->debuff.mark_of_the_crane->up() &&
           !get_target_data( target )->debuff.storm_earth_and_fire->up() )
      {
        // remove the current target as having an SEF on it
        get_target_data( state->target )->debuff.storm_earth_and_fire->expire();
        // make the new target show that a SEF is on the target
        get_target_data( target )->debuff.storm_earth_and_fire->trigger();
        return target;
      }
    }

    // If all targets have the debuff, find the lowest duration of cyclone strike debuff as well as not have a SEF
    // debuff (indicating that an SEF is not already on the target and send the SEF to that new target.
    player_t* lowest_duration = targets[ 0 ];

    // They should never attack the player target
    for ( player_t* target : targets )
    {
      if ( !get_target_data( target )->debuff.storm_earth_and_fire->up() )
      {
        if ( get_target_data( target )->debuff.mark_of_the_crane->remains() <
             get_target_data( lowest_duration )->debuff.mark_of_the_crane->remains() )
          lowest_duration = target;
      }
    }
    // remove the current target as having an SEF on it
    get_target_data( state->target )->debuff.storm_earth_and_fire->expire();
    // make the new target show that a SEF is on the target
    get_target_data( lowest_duration )->debuff.storm_earth_and_fire->trigger();
    return lowest_duration;
  }
  // otherwise, target the same as the player
  return targets.front();
}

int monk_t::mark_of_the_crane_counter()
{
  auto targets                  = sim->target_non_sleeping_list.data();
  int mark_of_the_crane_counter = 0;

  if ( specialization() == MONK_WINDWALKER )
  {
    for ( player_t* target : targets )
    {
      if ( get_target_data( target )->debuff.mark_of_the_crane->up() )
        mark_of_the_crane_counter++;
    }
  }
  return mark_of_the_crane_counter;
}

// monk_t::create_pets ======================================================

void monk_t::create_pets()
{
  base_t::create_pets();

  monk_t* p = this;

  if ( spec.invoke_xuen->ok() && ( find_action( "invoke_xuen" ) || find_action( "invoke_xuen_the_white_tiger" ) ) )
  {
    pets.xuen = new pets::xuen_pet_t( p );
  }

  if ( spec.invoke_niuzao->ok() && ( find_action( "invoke_niuzao" ) || find_action( "invoke_niuzao_the_black_ox" ) ) )
  {
    pets.niuzao = new pets::niuzao_pet_t( p );
  }

  if ( talent.invoke_chi_ji->ok() && ( find_action( "invoke_chiji" ) || find_action( "invoke_chiji_the_red_crane" ) ) )
  {
    pets.chiji = new pets::chiji_pet_t( p );
  }

  if ( spec.invoke_yulon->ok() && !talent.invoke_chi_ji->ok() &&
       ( find_action( "invoke_yulon" ) || find_action( "invoke_yulon_the_jade_serpent" ) ) )
  {
    pets.yulon = new pets::yulon_pet_t( p );
  }

  if ( specialization() == MONK_WINDWALKER && find_action( "storm_earth_and_fire" ) )
  {
    pets.sef[ SEF_FIRE ] = new pets::storm_earth_and_fire_pet_t( "fire_spirit", p, true, WEAPON_SWORD );
    // The player BECOMES the Storm Spirit
    // SEF EARTH was changed from 2-handed user to dual welding in Legion
    pets.sef[ SEF_EARTH ] = new pets::storm_earth_and_fire_pet_t( "earth_spirit", p, true, WEAPON_MACE );
  }

  if ( covenant.venthyr->ok() )
  {
    pets.fallen_monk_ww.set_creation_callback( []( monk_t* p ) { return new pets::fallen_monk_ww_pet_t( p ); } );
    pets.fallen_monk_mw.set_creation_callback( []( monk_t* p ) { return new pets::fallen_monk_mw_pet_t( p ); } );
    pets.fallen_monk_brm.set_creation_callback( []( monk_t* p ) { return new pets::fallen_monk_brm_pet_t( p ); } );
  }
}

// monk_t::activate =========================================================

void monk_t::activate()
{
  player_t::activate();

  if ( specialization() == MONK_WINDWALKER && find_action( "storm_earth_and_fire" ) )
  {
    sim->target_non_sleeping_list.register_callback( actions::sef_despawn_cb_t( this ) );
  }
}

void monk_t::collect_resource_timeline_information()
{
  base_t::collect_resource_timeline_information();

  sample_datas.stagger_damage_pct_timeline.add( sim->current_time(), current_stagger_amount_remains_percent() * 100.0 );

  auto stagger_pct_val = stagger_pct( target->level() );
  sample_datas.stagger_pct_timeline.add( sim->current_time(), stagger_pct_val * 100.0 );
}

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  base_t::init_spells();
  // Talents spells =====================================
  // Tier 15 Talents
  talent.eye_of_the_tiger = find_talent_spell( "Eye of the Tiger" );  // Brewmaster & Windwalker
  talent.chi_wave         = find_talent_spell( "Chi Wave" );
  talent.chi_burst        = find_talent_spell( "Chi Burst" );
  // Mistweaver
  talent.zen_pulse = find_talent_spell( "Zen Pulse" );

  // Tier 25 Talents
  talent.celerity    = find_talent_spell( "Celerity" );
  talent.chi_torpedo = find_talent_spell( "Chi Torpedo" );
  talent.tigers_lust = find_talent_spell( "Tiger's Lust" );

  // Tier 30 Talents
  // Brewmaster
  talent.light_brewing = find_talent_spell( "Light Brewing" );
  talent.spitfire      = find_talent_spell( "Spitfire" );
  talent.black_ox_brew = find_talent_spell( "Black Ox Brew" );
  // Windwalker
  talent.ascension               = find_talent_spell( "Ascension" );
  talent.fist_of_the_white_tiger = find_talent_spell( "Fist of the White Tiger" );
  talent.energizing_elixir       = find_talent_spell( "Energizing Elixir" );
  // Mistweaver
  talent.spirit_of_the_crane = find_talent_spell( "Spirit of the Crane" );
  talent.mist_wrap           = find_talent_spell( "Mist Wrap" );
  talent.lifecycles          = find_talent_spell( "Lifecycles" );

  // Tier 35 Talents
  talent.tiger_tail_sweep       = find_talent_spell( "Tiger Tail Sweep" );
  talent.summon_black_ox_statue = find_talent_spell( "Summon Black Ox Statue" );  // Brewmaster & Windwalker
  talent.song_of_chi_ji         = find_talent_spell( "Song of Chi-Ji" );          // Mistweaver
  talent.ring_of_peace          = find_talent_spell( "Ring of Peace" );
  // Windwalker
  talent.good_karma             = find_talent_spell( "Good Karma" );

  // Tier 40 Talents
  // Windwalker
  talent.inner_strength = find_talent_spell( "Inner Strength" );
  // Mistweaver & Windwalker
  talent.diffuse_magic  = find_talent_spell( "Diffuse Magic" );
  // Brewmaster
  talent.bob_and_weave  = find_talent_spell( "Bob and Weave" );
  talent.healing_elixir = find_talent_spell( "Healing Elixir" );
  talent.dampen_harm    = find_talent_spell( "Dampen Harm" );

  // Tier 45 Talents
  // Brewmaster
  talent.special_delivery           = find_talent_spell( "Special Delivery" );
  talent.exploding_keg              = find_talent_spell( "Exploding Keg" );
  // Windwalker
  talent.hit_combo                  = find_talent_spell( "Hit Combo" );
  talent.dance_of_chiji             = find_talent_spell( "Dance of Chi-Ji" );
  // Brewmaster & Windwalker
  talent.rushing_jade_wind          = find_talent_spell( "Rushing Jade Wind" );
  // Mistweaver
  talent.summon_jade_serpent_statue = find_talent_spell( "Summon Jade Serpent Statue" );
  talent.refreshing_jade_wind       = find_talent_spell( "Refreshing Jade Wind" );
  talent.invoke_chi_ji              = find_talent_spell( "Invoke Chi-Ji, the Red Crane" );

  // Tier 50 Talents
  // Brewmaster
  talent.high_tolerance   = find_talent_spell( "High Tolerance" );
  talent.celestial_flames = find_talent_spell( "Celestial Flames" );
  talent.blackout_combo   = find_talent_spell( "Blackout Combo" );
  // Windwalker
  talent.spirtual_focus        = find_talent_spell( "Spiritual Focus" );
  talent.whirling_dragon_punch = find_talent_spell( "Whirling Dragon Punch" );
  talent.serenity              = find_talent_spell( "Serenity" );
  // Mistweaver
  talent.mana_tea        = find_talent_spell( "Mana Tea" );
  talent.focused_thunder = find_talent_spell( "Focused Thunder" );
  talent.rising_thunder  = find_talent_spell( "Rising Thunder" );

  // Specialization spells ====================================
  // Multi-Specialization & Class Spells
  spec.blackout_kick             = find_class_spell( "Blackout Kick" );
  spec.blackout_kick_2           = find_rank_spell( "Blackout Kick", "Rank 2", MONK_WINDWALKER );
  spec.blackout_kick_3           = find_rank_spell( "Blackout Kick", "Rank 3", MONK_WINDWALKER );
  spec.blackout_kick_brm         = find_specialization_spell( "Blackout Kick" );
  spec.crackling_jade_lightning  = find_class_spell( "Crackling Jade Lightning" );
  spec.critical_strikes          = find_specialization_spell( "Critical Strikes" );
  spec.detox                     = find_specialization_spell( "Detox" );
  spec.expel_harm                = find_class_spell( "Expel Harm" );
  spec.expel_harm_2_brm          = find_rank_spell( "Expel Harm", "Rank 2", MONK_BREWMASTER );
  spec.expel_harm_2_mw           = find_rank_spell( "Expel Harm", "Rank 2", MONK_MISTWEAVER );
  spec.expel_harm_2_ww           = find_rank_spell( "Expel Harm", "Rank 2", MONK_WINDWALKER );
  spec.fortifying_brew_brm       = find_spell( 115203 );
  spec.fortifying_brew_2_brm     = find_rank_spell( "Fortifying Brew", "Rank 2", MONK_BREWMASTER );
  spec.fortifying_brew_mw_ww     = find_spell( 243435 );
  spec.fortifying_brew_2_mw      = find_rank_spell( "Fortifying Brew", "Rank 2", MONK_MISTWEAVER );
  spec.fortifying_brew_2_ww      = find_rank_spell( "Fortifying Brew", "Rank 2", MONK_WINDWALKER );
  spec.leather_specialization    = find_specialization_spell( "Leather Specialization" );
  spec.leg_sweep                 = find_class_spell( "Leg Sweep" );
  spec.mystic_touch              = find_class_spell( "Mystic Touch" );
  spec.paralysis                 = find_class_spell( "Paralysis" );
  spec.provoke                   = find_class_spell( "Provoke" );
  spec.provoke_2                 = find_rank_spell( "Provoke", "Rank 2" );
  spec.resuscitate               = find_class_spell( "Resuscitate" );
  spec.rising_sun_kick           = find_specialization_spell( "Rising Sun Kick" );
  spec.rising_sun_kick_2         = find_rank_spell( "Rising Sun Kick", "Rank 2" );
  spec.roll                      = find_class_spell( "Roll" );
  spec.roll_2                    = find_rank_spell( "Roll", "Rank 2" );
  spec.spear_hand_strike         = find_specialization_spell( "Spear Hand Strike" );
  spec.spinning_crane_kick       = find_class_spell( "Spinning Crane Kick" );
  spec.spinning_crane_kick_brm   = find_spell( 322729 );
  spec.spinning_crane_kick_2_brm = find_rank_spell( "Spinning Crane Kick", "Rank 2", MONK_BREWMASTER );
  spec.spinning_crane_kick_2_ww  = find_rank_spell( "Spinning Crane Kick", "Rank 2", MONK_WINDWALKER );
  spec.tiger_palm                = find_class_spell( "Tiger Palm" );
  spec.touch_of_death            = find_class_spell( "Touch of Death" );
  spec.touch_of_death_2          = find_rank_spell( "Touch of Death", "Rank 2" );
  spec.touch_of_death_3_brm      = find_rank_spell( "Touch of Death", "Rank 3", MONK_BREWMASTER );
  spec.touch_of_death_3_mw       = find_rank_spell( "Touch of Death", "Rank 3", MONK_MISTWEAVER );
  spec.touch_of_death_3_ww       = find_rank_spell( "Touch of Death", "Rank 3", MONK_WINDWALKER );
  spec.two_hand_adjustment       = find_spell( 346104 );
  spec.vivify                    = find_class_spell( "Vivify" );
  spec.vivify_2_brm              = find_rank_spell( "Vivify", "Rank 2", MONK_BREWMASTER );
  spec.vivify_2_mw               = find_rank_spell( "Vivify", "Rank 2", MONK_MISTWEAVER );
  spec.vivify_2_ww               = find_rank_spell( "Vivify", "Rank 2", MONK_WINDWALKER );

  // Brewmaster Specialization
  spec.bladed_armor        = find_specialization_spell( "Bladed Armor" );
  spec.breath_of_fire      = find_specialization_spell( "Breath of Fire" );
  spec.brewmasters_balance = find_specialization_spell( "Brewmaster's Balance" );
  spec.brewmaster_monk     = find_specialization_spell( "Brewmaster Monk" );
  spec.celestial_brew      = find_specialization_spell( "Celestial Brew" );
  spec.celestial_brew_2    = find_rank_spell( "Celestial Brew", "Rank 2" );
  spec.celestial_fortune   = find_specialization_spell( "Celestial Fortune" );
  spec.clash               = find_specialization_spell( "Clash" );
  spec.gift_of_the_ox      = find_specialization_spell( "Gift of the Ox" );
  spec.invoke_niuzao       = find_specialization_spell( "Invoke Niuzao, the Black Ox" );
  spec.invoke_niuzao_2     = find_specialization_spell( "Invoke Niuzao, the Black Ox", "Rank 2" );
  spec.keg_smash           = find_specialization_spell( "Keg Smash" );
  spec.purifying_brew      = find_specialization_spell( "Purifying Brew" );
  spec.purifying_brew_2    = find_rank_spell( "Purifying Brew", "Rank 2" );
  spec.shuffle             = find_specialization_spell( "Shuffle" );
  spec.stagger             = find_specialization_spell( "Stagger" );
  spec.stagger_2           = find_rank_spell( "Stagger", "Rank 2" );
  spec.zen_meditation      = find_specialization_spell( "Zen Meditation" );

  // Mistweaver Specialization
  spec.detox                      = find_specialization_spell( "Detox" );
  spec.enveloping_mist            = find_specialization_spell( "Enveloping Mist" );
  spec.envoloping_mist_2          = find_rank_spell( "Enveloping Mist", "Rank 2" );
  spec.essence_font               = find_specialization_spell( "Essence Font" );
  spec.essence_font_2             = find_rank_spell( "Essence Font", "Rank 2" );
  spec.invoke_yulon               = find_specialization_spell( "Invoke Yu'lon, the Jade Serpent" );
  spec.life_cocoon                = find_specialization_spell( "Life Cocoon" );
  spec.life_cocoon_2              = find_rank_spell( "Life Cocoon", "Rank 2" );
  spec.mistweaver_monk            = find_specialization_spell( "Mistweaver Monk" );
  spec.reawaken                   = find_specialization_spell( "Reawaken" );
  spec.renewing_mist              = find_specialization_spell( "Renewing Mist" );
  spec.renewing_mist_2            = find_rank_spell( "Renewing Mist", "Rank 2" );
  spec.revival                    = find_specialization_spell( "Revival" );
  spec.soothing_mist              = find_specialization_spell( "Soothing Mist" );
  spec.teachings_of_the_monastery = find_specialization_spell( "Teachings of the Monastery" );
  spec.thunder_focus_tea          = find_specialization_spell( "Thunder Focus Tea" );
  spec.thunder_focus_tea_2        = find_rank_spell( "Thunder Focus Tea", "Rank 2" );

  // Windwalker Specialization
  spec.afterlife                  = find_specialization_spell( "Afterlife" );
  spec.afterlife_2                = find_rank_spell( "Afterlife", "Rank 2" );
  spec.combat_conditioning        = find_specialization_spell( "Combat Conditioning" );
  spec.combo_breaker              = find_specialization_spell( "Combo Breaker" );
  spec.cyclone_strikes            = find_specialization_spell( "Cyclone Strikes" );
  spec.disable                    = find_specialization_spell( "Disable" );
  spec.disable_2                  = find_rank_spell( "Disable", "Rank 2" );
  spec.fists_of_fury              = find_specialization_spell( "Fists of Fury" );
  spec.flying_serpent_kick        = find_specialization_spell( "Flying Serpent Kick" );
  spec.flying_serpent_kick_2      = find_rank_spell( "Flying Serpent Kick", "Rank 2" );
  spec.invoke_xuen                = find_specialization_spell( "Invoke Xuen, the White Tiger" );
  spec.invoke_xuen_2              = find_rank_spell( "Invoke Xuen, the White Tiger", "Rank 2" );
  spec.reverse_harm               = find_spell( 342928 );
  spec.stance_of_the_fierce_tiger = find_specialization_spell( "Stance of the Fierce Tiger" );
  spec.storm_earth_and_fire       = find_specialization_spell( "Storm, Earth, and Fire" );
  spec.storm_earth_and_fire_2     = find_rank_spell( "Storm, Earth, and Fire", "Rank 2" );
  spec.touch_of_karma             = find_specialization_spell( "Touch of Karma" );
  spec.windwalker_monk            = find_specialization_spell( "Windwalker Monk" );
  spec.windwalking                = find_specialization_spell( "Windwalking" );

  // Covenant Abilities ================================

  covenant.kyrian                 = find_covenant_spell( "Weapons of Order" );
  covenant.night_fae              = find_covenant_spell( "Faeline Stomp" );;
  covenant.venthyr                = find_covenant_spell( "Fallen Order" );;
  covenant.necrolord              = find_covenant_spell( "Bonedust Brew" );;

  // Soulbind Conduits Abilities =======================

  // General
  conduit.dizzying_tumble         = find_conduit_spell( "Dizzying Tumble" );
  conduit.fortifying_ingredients  = find_conduit_spell( "Fortifying Ingredients" );
  conduit.grounding_breath        = find_conduit_spell( "Grounding Breath" );
  conduit.harm_denial             = find_conduit_spell( "Harm Denial" );
  conduit.lingering_numbness      = find_conduit_spell( "Lingering Numbness" );
  conduit.swift_transference      = find_conduit_spell( "Swift Transference" );
  conduit.tumbling_technique      = find_conduit_spell( "Tumbling Technique" );

  // Brewmaster
  conduit.celestial_effervescence = find_conduit_spell( "Celestial Effervescence" );
  conduit.evasive_stride          = find_conduit_spell( "Evasive Stride" );
  conduit.scalding_brew           = find_conduit_spell( "Scalding Brew" );
  conduit.walk_with_the_ox        = find_conduit_spell( "Walk with the Ox" );

  // Mistweaver
  conduit.jade_bond               = find_conduit_spell( "Jade Bond" );
  conduit.nourishing_chi          = find_conduit_spell( "Nourishing Chi" );
  conduit.rising_sun_revival      = find_conduit_spell( "Rising Sun Revival" );
  conduit.resplendent_mist        = find_conduit_spell( "Resplendent Mist" );

  // Windwalker
  conduit.calculated_strikes      = find_conduit_spell( "Calculated Strikes" );
  conduit.coordinated_offensive   = find_conduit_spell( "Coordinated Offensive" );
  conduit.inner_fury              = find_conduit_spell( "Inner Fury" );
  conduit.xuens_bond              = find_conduit_spell( "Xuen's Bond" );

  // Covenant
  conduit.strike_with_clarity     = find_conduit_spell( "Strike with Clarity" );
  conduit.imbued_reflections      = find_conduit_spell( "Imbued Reflections" );
  conduit.bone_marrow_hops        = find_conduit_spell( "Bone Marrow Hops" );
  conduit.way_of_the_fae          = find_conduit_spell( "Way of the Fae" );

  // Shadowland Legendaries ============================

  // General
  legendary.fatal_touch                        = find_runeforge_legendary( "Fatal Touch" );
  legendary.invokers_delight                   = find_runeforge_legendary( "Invoker's Delight" );
  legendary.swiftsure_wraps                    = find_runeforge_legendary( "Swiftsure Wraps" );
  legendary.escape_from_reality                = find_runeforge_legendary( "Escape from Reality" );

  // Brewmaster
  legendary.charred_passions                   = find_runeforge_legendary( "Charred Passions" );
  legendary.celestial_infusion                 = find_runeforge_legendary( "Celestial Infusion" );
  legendary.shaohaos_might                     = find_runeforge_legendary( "Shaohao's Might" );
  legendary.stormstouts_last_keg               = find_runeforge_legendary( "Stormstout's Last Keg" );

  // Mistweaver
  legendary.ancient_teachings_of_the_monastery = find_runeforge_legendary( "Ancient Teachings of the Monastery" );
  legendary.clouded_focus                      = find_runeforge_legendary( "Clouded Focus" );
  legendary.tear_of_morning                    = find_runeforge_legendary( "Tear of Morning" );
  legendary.yulons_whisper                     = find_runeforge_legendary( "Yu'lon's Whisper" );

  // Windwalker
  legendary.jade_ignition                      = find_runeforge_legendary( "Jade Ignition" );
  legendary.keefers_skyreach                   = find_runeforge_legendary( "Keefer's Skyreach" );
  legendary.last_emperors_capacitor            = find_runeforge_legendary( "Last Emperor's Capacitor" );
  legendary.xuens_treasure                     = find_runeforge_legendary( "Xuen's Treasure" );

  // Passives =========================================
  // General
  passives.aura_monk             = find_spell( 137022 );
  passives.chi_burst_damage      = find_spell( 148135 );
  passives.chi_burst_heal        = find_spell( 130654 );
  passives.chi_wave_damage       = find_spell( 132467 );
  passives.chi_wave_heal         = find_spell( 132463 );
  passives.fortifying_brew       = find_spell( 120954 );
  passives.healing_elixir        = find_spell( 122281 );  // talent.healing_elixir -> effectN( 1 ).trigger() -> effectN( 1 ).trigger()
  passives.mystic_touch          = find_spell( 8647 );

  // Brewmaster
  passives.breath_of_fire_dot  = find_spell( 123725 );
  passives.celestial_fortune   = find_spell( 216521 );
  passives.elusive_brawler     = find_spell( 195630 );
  passives.gift_of_the_ox_heal = find_spell( 124507 );
  passives.shuffle             = find_spell( 215479 );
  passives.keg_smash_buff      = find_spell( 196720 );
  passives.special_delivery    = find_spell( 196733 );
  passives.stagger_self_damage = find_spell( 124255 );
  passives.heavy_stagger       = find_spell( 124273 );
  passives.stomp               = find_spell( 227291 );

  // Mistweaver
  passives.totm_bok_proc        = find_spell( 228649 );
  passives.renewing_mist_heal   = find_spell( 119611 );
  passives.soothing_mist_heal   = find_spell( 115175 );
  passives.soothing_mist_statue = find_spell( 198533 );
  passives.spirit_of_the_crane  = find_spell( 210803 );
  passives.zen_pulse_heal       = find_spell( 198487 );

  // Windwalker
  passives.bok_proc                         = find_spell( 116768 );
  passives.crackling_tiger_lightning        = find_spell( 123996 );
  passives.crackling_tiger_lightning_driver = find_spell( 123999 );
  passives.cyclone_strikes                  = find_spell( 220358 );
  passives.dance_of_chiji                   = find_spell( 325202 );
  passives.dizzying_kicks                   = find_spell( 196723 );
  passives.empowered_tiger_lightning        = find_spell( 335913 );
  passives.fists_of_fury_tick               = find_spell( 117418 );
  passives.flying_serpent_kick_damage       = find_spell( 123586 );
  passives.focus_of_xuen                    = find_spell( 252768 );
  passives.hit_combo                        = find_spell( 196741 );
  passives.mark_of_the_crane                = find_spell( 228287 );
  passives.touch_of_karma_tick              = find_spell( 124280 );
  passives.whirling_dragon_punch_tick       = find_spell( 158221 );

  // Covenants
  passives.bonedust_brew_dmg                    = find_spell( 325217 );
  passives.bonedust_brew_heal                   = find_spell( 325218 );
  passives.bonedust_brew_chi                    = find_spell( 328296 );
  passives.faeline_stomp_damage                 = find_spell( 345727 );
  passives.faeline_stomp_ww_damage              = find_spell( 327264 );
  passives.faeline_stomp_brm                    = find_spell( 347480 );
  passives.fallen_monk_breath_of_fire           = find_spell( 330907 );
  passives.fallen_monk_clash                    = find_spell( 330909 );
  passives.fallen_monk_enveloping_mist          = find_spell( 344008 );
  passives.fallen_monk_fists_of_fury            = find_spell( 330898 );
  passives.fallen_monk_fists_of_fury_tick       = find_spell( 345714 );
  passives.fallen_monk_keg_smash                = find_spell( 330911 );
  passives.fallen_monk_soothing_mist            = find_spell( 328283 );
  passives.fallen_monk_spec_duration            = find_spell( 347826 );
  passives.fallen_monk_spinning_crane_kick      = find_spell( 330901 );
  passives.fallen_monk_spinning_crane_kick_tick = find_spell( 330903 );
  passives.fallen_monk_tiger_palm               = find_spell( 346602 );

  // Conduits
  passives.fortifying_ingredients     = find_spell( 336874 );
  passives.evasive_stride             = find_spell( 343764 );

  // Shadowland Legendary
  passives.chi_explosion              = find_spell( 337342 );
  passives.face_palm                  = find_spell( 227679 );
  passives.charred_passions_dmg          = find_spell( 338141 );

  // Mastery spells =========================================
  mastery.combo_strikes   = find_mastery_spell( MONK_WINDWALKER );
  mastery.elusive_brawler = find_mastery_spell( MONK_BREWMASTER );
  mastery.gust_of_mists   = find_mastery_spell( MONK_MISTWEAVER );

  // Sample Data
  sample_datas.stagger_total_damage       = get_sample_data( "Damage added to stagger pool" );
  sample_datas.stagger_damage             = get_sample_data( "Effective stagger damage" );
  sample_datas.light_stagger_damage       = get_sample_data( "Effective light stagger damage" );
  sample_datas.moderate_stagger_damage    = get_sample_data( "Effective moderate stagger damage" );
  sample_datas.heavy_stagger_damage       = get_sample_data( "Effective heavy stagger damage" );
  sample_datas.purified_damage            = get_sample_data( "Stagger damage that was purified" );
  sample_datas.staggering_strikes_cleared = get_sample_data( "Stagger damage that was cleared by Staggering Strikes" );

  // Active Action Spells
  // Brewmaster
  active_actions.breath_of_fire         = new actions::spells::breath_of_fire_dot_t( *this );
  active_actions.celestial_fortune      = new actions::heals::celestial_fortune_t( *this );
  active_actions.gift_of_the_ox_trigger = new actions::gift_of_the_ox_trigger_t( *this );
  active_actions.gift_of_the_ox_expire  = new actions::gift_of_the_ox_expire_t( *this );
  active_actions.stagger_self_damage    = new actions::stagger_self_damage_t( this );

  // Windwalker
  active_actions.sunrise_technique      = new actions::sunrise_technique_t( this );
  active_actions.empowered_tiger_lightning = new actions::empowered_tiger_lightning_t( *this );
  windwalking_aura                      = new actions::windwalking_aura_t( this );

  // Conduit
  active_actions.evasive_stride         = new actions::heals::evasive_stride_t( *this );

  // Covenant
  active_actions.bonedust_brew_dmg      = new actions::spells::bonedust_brew_damage_t( *this );
  active_actions.bonedust_brew_heal     = new actions::spells::bonedust_brew_heal_t( *this );
}

// monk_t::init_base ========================================================

void monk_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    if ( specialization() == MONK_MISTWEAVER )
      base.distance = 40;
    else
      base.distance = 5;
  }
  base_t::init_base_stats();

  base_gcd = timespan_t::from_seconds( 1.5 );

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
    {
      base_gcd += spec.brewmaster_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds
      base.attack_power_per_agility                      = 1.0;
      base.spell_power_per_attack_power                  = spec.brewmaster_monk->effectN( 18 ).percent();
      resources.base[ RESOURCE_ENERGY ]                  = 100;
      resources.base[ RESOURCE_MANA ]                    = 0;
      resources.base[ RESOURCE_CHI ]                     = 0;
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10.0;
      resources.base_regen_per_second[ RESOURCE_MANA ]   = 0;
      break;
    }
    case MONK_MISTWEAVER:
    {
      base.spell_power_per_intellect                     = 1.0;
      base.attack_power_per_spell_power                  = spec.mistweaver_monk->effectN( 4 ).percent();
      resources.base[ RESOURCE_ENERGY ]                  = 0;
      resources.base[ RESOURCE_CHI ]                     = 0;
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
      break;
    }
    case MONK_WINDWALKER:
    {
      if ( base.distance < 1 )
        base.distance = 5;
      //base_gcd += spec.windwalker_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds
      base.attack_power_per_agility     = 1.0;
      base.spell_power_per_attack_power = spec.windwalker_monk->effectN( 14 ).percent();
      resources.base[ RESOURCE_ENERGY ] = 100;
      resources.base[ RESOURCE_ENERGY ] += talent.ascension->effectN( 3 ).base_value();
      resources.base[ RESOURCE_MANA ] = 0;
      resources.base[ RESOURCE_CHI ]  = 4;
      resources.base[ RESOURCE_CHI ] += spec.windwalker_monk->effectN( 11 ).base_value();
      resources.base[ RESOURCE_CHI ] += talent.ascension->effectN( 1 ).base_value();
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10.0;
      resources.base_regen_per_second[ RESOURCE_MANA ]   = 0;
      break;
    }
    default:
      break;
  }

  resources.base_regen_per_second[ RESOURCE_CHI ] = 0;
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  base_t::init_scaling();

  if ( specialization() != MONK_MISTWEAVER )
  {
    scaling->disable( STAT_INTELLECT );
    scaling->disable( STAT_SPELL_POWER );
    scaling->enable( STAT_AGILITY );
    scaling->enable( STAT_WEAPON_DPS );
  }
  else
  {
    scaling->disable( STAT_AGILITY );
    scaling->disable( STAT_MASTERY_RATING );
    scaling->disable( STAT_ATTACK_POWER );
    scaling->enable( STAT_SPIRIT );
  }
  scaling->disable( STAT_STRENGTH );

  if ( specialization() == MONK_WINDWALKER )
  {
    // Touch of Death
    scaling->enable( STAT_STAMINA );
  }
  if ( specialization() == MONK_BREWMASTER )
  {
    scaling->enable( STAT_BONUS_ARMOR );
  }

  if ( off_hand_weapon.type != WEAPON_NONE )
    scaling->enable( STAT_WEAPON_OFFHAND_DPS );
}

// monk_t::init_buffs =======================================================

// monk_t::create_buffs =====================================================

void monk_t::create_buffs()
{
  base_t::create_buffs();

  // General
  buff.chi_torpedo = make_buff( this, "chi_torpedo", find_spell( 119085 ) )
                         ->set_default_value_from_effect( 1 );

  buff.fortifying_brew = new buffs::fortifying_brew_t( *this, "fortifying_brew",
      ( specialization() == MONK_BREWMASTER ? passives.fortifying_brew : spec.fortifying_brew_mw_ww ) );

  buff.rushing_jade_wind = new buffs::rushing_jade_wind_buff_t( *this, "rushing_jade_wind", talent.rushing_jade_wind );

  buff.dampen_harm = make_buff( this, "dampen_harm", talent.dampen_harm );

  buff.diffuse_magic = make_buff( this, "diffuse_magic", talent.diffuse_magic )
                           ->set_default_value_from_effect( 1 );

  buff.spinning_crane_kick = make_buff( this, "spinning_crane_kick", spec.spinning_crane_kick )
                                 ->set_default_value_from_effect( 2 )
                                 ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  // Brewmaster
  buff.bladed_armor = make_buff( this, "bladed_armor", spec.bladed_armor )
                          ->set_default_value_from_effect( 1 )
                          ->add_invalidate( CACHE_ATTACK_POWER );

  buff.blackout_combo = make_buff( this, "blackout_combo", talent.blackout_combo->effectN( 5 ).trigger() );

  buff.celestial_brew = make_buff<absorb_buff_t>( this, "celestial_brew", spec.celestial_brew );
  buff.celestial_brew->set_absorb_source( get_stats( "celestial_brew" ) )->set_cooldown( timespan_t::zero() );

  buff.celestial_flames = make_buff( this, "celestial_flames", talent.celestial_flames->effectN( 1 ).trigger() )
                              ->set_chance( talent.celestial_flames->proc_chance() )
                              ->set_default_value( talent.celestial_flames->effectN( 2 ).percent() );

  buff.elusive_brawler = make_buff( this, "elusive_brawler", mastery.elusive_brawler->effectN( 3 ).trigger() )
                             ->add_invalidate( CACHE_DODGE );

  buff.shuffle = make_buff( this, "shuffle", passives.shuffle )
                     ->set_duration_multiplier( 3 )
                     ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  buff.gift_of_the_ox = new buffs::gift_of_the_ox_buff_t( *this, "gift_of_the_ox", find_spell( 124503 ) );

  buff.invoke_niuzao = make_buff( this, "invoke_niuzao_the_black_ox", spec.invoke_niuzao )
                           ->set_default_value_from_effect( 2 );

  buff.purified_chi = make_buff( this, "purified_chi", find_spell( 325092 ) )
                          ->set_default_value_from_effect( 1 );

  buff.spitfire = make_buff( this, "spitfire", talent.spitfire->effectN( 1 ).trigger() );

  buff.light_stagger    = make_buff<buffs::stagger_buff_t>( *this, "light_stagger", find_spell( 124275 ) );
  buff.moderate_stagger = make_buff<buffs::stagger_buff_t>( *this, "moderate_stagger", find_spell( 124274 ) );
  buff.heavy_stagger    = make_buff<buffs::stagger_buff_t>( *this, "heavy_stagger", passives.heavy_stagger );
  buff.recent_purifies = new buffs::purifying_buff_t( *this, "recent_purifies", spell_data_t::nil() );

  // Mistweaver
  buff.channeling_soothing_mist = make_buff( this, "channeling_soothing_mist", passives.soothing_mist_heal );

  buff.invoke_chiji = make_buff( this, "invoke_chiji", find_spell( 343818 ) );

  buff.invoke_chiji_evm = make_buff( this, "invoke_chiji_evm", find_spell( 343820 ) )
                              ->set_default_value_from_effect( 1 );

  buff.life_cocoon = make_buff<absorb_buff_t>( this, "life_cocoon", spec.life_cocoon );
  buff.life_cocoon->set_absorb_source( get_stats( "life_cocoon" ) )->set_cooldown( timespan_t::zero() );

  buff.mana_tea = make_buff( this, "mana_tea", talent.mana_tea )
                      ->set_default_value_from_effect( 1 );

  buff.lifecycles_enveloping_mist = make_buff( this, "lifecycles_enveloping_mist", find_spell( 197919 ) )
                                        ->set_default_value_from_effect( 1 );

  buff.lifecycles_vivify = make_buff( this, "lifecycles_vivify", find_spell( 197916 ) )
                               ->set_default_value_from_effect( 1 );

  buff.refreshing_jade_wind =
      make_buff( this, "refreshing_jade_wind", talent.refreshing_jade_wind )
          ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.teachings_of_the_monastery = make_buff( this, "teachings_of_the_monastery", find_spell( 202090 ) )
                                        ->set_default_value_from_effect( 1 );

  buff.thunder_focus_tea =
      make_buff( this, "thunder_focus_tea", spec.thunder_focus_tea )
          ->modify_max_stack( (int)( talent.focused_thunder ? talent.focused_thunder->effectN( 1 ).base_value() : 0 ) );

  buff.touch_of_death = make_buff( this, "touch_of_death", find_spell( 344361 ) )
                            ->set_default_value_from_effect( 1 )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.uplifting_trance = make_buff( this, "uplifting_trance", find_spell( 197916 ) )
                              ->set_chance( spec.renewing_mist->effectN( 2 ).percent() )
                              ->set_default_value_from_effect( 1 );

  // Windwalker
  buff.bok_proc =
      make_buff( this, "bok_proc", passives.bok_proc )
          ->set_chance( spec.combo_breaker->effectN( 1 ).percent() );

  buff.combo_master = make_buff( this, "combo_master", find_spell( 211432 ) )
                          ->set_default_value_from_effect( 1 )
                          ->add_invalidate( CACHE_MASTERY );

  buff.combo_strikes =
      make_buff( this, "combo_strikes" )
          ->set_duration( timespan_t::from_minutes( 60 ) )
          ->set_quiet( true )  // In-game does not show this buff but I would like to use it for background stuff
          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.dance_of_chiji = make_buff( this, "dance_of_chiji", passives.dance_of_chiji )
                            ->set_trigger_spell( talent.dance_of_chiji );

  buff.dance_of_chiji_hidden = make_buff( this, "dance_of_chiji_hidden" )
                                   ->set_duration( timespan_t::from_seconds( 1.5 ) )
                                   ->set_quiet( true )
                                   ->set_default_value( talent.dance_of_chiji->effectN( 1 ).percent() );

  buff.flying_serpent_kick_movement = make_buff( this, "flying_serpent_kick_movement" );  // find_spell( 115057 )

  buff.hit_combo = make_buff( this, "hit_combo", passives.hit_combo )
                       ->set_default_value_from_effect( 1 )
                       ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.inner_stength = make_buff( this, "inner_strength", find_spell( 261769 ) )
                           ->set_default_value_from_effect( 1 )
                           ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  buff.invoke_xuen = new buffs::invoke_xuen_the_white_tiger_buff_t( *this, "invoke_xuen_the_white_tiger", spec.invoke_xuen );

  buff.serenity = new buffs::serenity_buff_t( *this, "serenity", talent.serenity );

  buff.storm_earth_and_fire =
      make_buff( this, "storm_earth_and_fire", spec.storm_earth_and_fire )
          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
          ->set_can_cancel( false )  // Undocumented hotfix 28/09/2018 - SEF can no longer be canceled.
          ->set_cooldown( timespan_t::zero() );

  buff.pressure_point = make_buff( this, "pressure_point", find_spell( 337481 ) )
                            ->set_default_value_from_effect( 1 )
                            ->set_refresh_behavior( buff_refresh_behavior::NONE );

  buff.touch_of_karma = new buffs::touch_of_karma_buff_t( *this, "touch_of_karma", find_spell( 125174 ) );

  buff.windwalking_driver = new buffs::windwalking_driver_t( *this, "windwalking_aura_driver", find_spell( 166646 ) );

  buff.whirling_dragon_punch = make_buff( this, "whirling_dragon_punch", find_spell( 196742 ) )
                                   ->set_refresh_behavior( buff_refresh_behavior::NONE );

  // Covenant Abilities
  buff.bonedust_brew_hidden = make_buff( this, "bonedust_brew_hidden" )
                                  ->set_quiet( true )
                                  ->set_duration( timespan_t::from_seconds( 10 ) )
                                  ->set_max_stack( 5 )
                                  ->set_reverse( true )
                                  ->set_reverse_stack_count( 5 );

  buff.weapons_of_order = make_buff( this, "weapons_of_order", find_spell( 310454 ) )
                        ->set_default_value( find_spell( 310454 )->effectN( 1 ).base_value() )
                        ->set_duration( find_spell( 310454 )->duration() + (
                            conduit.strike_with_clarity->ok() ? conduit.strike_with_clarity->effectN( 2 ).time_value() : timespan_t::zero() ) )
                        ->add_invalidate( CACHE_MASTERY );

  buff.weapons_of_order_ww = make_buff( this, "weapons_of_order_ww", find_spell( 311054 ) )
                                 ->set_default_value( find_spell( 311054 )->effectN( 1 ).base_value() );

  buff.faeline_stomp = make_buff( this, "faeline_stomp", covenant.night_fae )
                           ->set_default_value_from_effect( 2 );

  buff.faeline_stomp_brm = make_buff( this, "faeline_stomp_brm", passives.faeline_stomp_brm )
                               ->set_default_value_from_effect( 1 );

  // Covenant Conduits
  buff.fortifying_ingrediences = make_buff<absorb_buff_t>( this, "fortifying_ingredients", find_spell( 336874 ) );
  buff.fortifying_ingrediences->set_absorb_source( get_stats( "fortifying_ingredients" ) )->set_cooldown( timespan_t::zero() );

  // Shadowland Legendaries
  // General
  buff.charred_passions    = make_buff( this, "charred_passions", find_spell( 338140 ) );
  buff.invokers_delight = make_buff( this, "invokers_delight", legendary.invokers_delight->effectN( 1 ).trigger() )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
      ->add_invalidate( CACHE_ATTACK_HASTE )
      ->add_invalidate( CACHE_HASTE )
      ->add_invalidate( CACHE_SPELL_HASTE );

  // Brewmaster
  buff.mighty_pour = make_buff( this, "mighty_pour", find_spell( 337994 ) )
                         ->add_invalidate ( CACHE_ARMOR );

  // Mistweaver

  // Windwalker
  buff.chi_energy = make_buff( this, "chi_energy", find_spell( 337571 ) )
                        ->set_default_value_from_effect( 1 );

  buff.the_emperors_capacitor = make_buff( this, "the_emperors_capacitor", find_spell( 337291 ) )
                                    ->set_default_value_from_effect( 1 );

}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  base_t::init_gains();

  gain.black_ox_brew_energy     = get_gain( "black_ox_brew_energy" );
  gain.bok_proc                 = get_gain( "blackout_kick_proc" );
  gain.chi_refund               = get_gain( "chi_refund" );
  gain.chi_burst                = get_gain( "chi_burst" );
  gain.crackling_jade_lightning = get_gain( "crackling_jade_lightning" );
  gain.energizing_elixir_energy = get_gain( "energizing_elixir_energy" );
  gain.energizing_elixir_chi    = get_gain( "energizing_elixir_chi" );
  gain.energy_refund            = get_gain( "energy_refund" );
  gain.expel_harm               = get_gain( "expel_harm" );
  gain.fist_of_the_white_tiger  = get_gain( "fist_of_the_white_tiger" );
  gain.focus_of_xuen            = get_gain( "focus_of_xuen" );
  gain.gift_of_the_ox           = get_gain( "gift_of_the_ox" );
  gain.glory_of_the_dawn        = get_gain( "glory_of_the_dawn" );
  gain.rushing_jade_wind_tick   = get_gain( "rushing_jade_wind_tick" );
  gain.serenity                 = get_gain( "serenity" );
  gain.spirit_of_the_crane      = get_gain( "spirit_of_the_crane" );
  gain.tiger_palm               = get_gain( "tiger_palm" );

  // Azerite Traits
  gain.open_palm_strikes        = get_gain( "open_palm_strikes" );
  gain.memory_of_lucid_dreams   = get_gain( "memory_of_lucid_dreams_proc" );
  gain.lucid_dreams             = get_gain( "lucid_dreams" );

  // Covenants
  gain.bonedust_brew            = get_gain( "bonedust_brew" );
  gain.weapons_of_order         = get_gain( "weapons_of_order" );
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.bok_proc                    = get_proc( "bok_proc" );
  proc.boiling_brew_healing_sphere = get_proc( "Boiling Brew Healing Sphere" );
}

// monk_t::init_assessors ===================================================

void monk_t::init_assessors()
{
  base_t::init_assessors();

  assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, [this]( result_amount_type, action_state_t* s ) {
    accumulate_gale_burst_damage( s );
    return assessor::CONTINUE;
  } );
}

// monk_t::init_rng =======================================================

void monk_t::init_rng()
{
  player_t::init_rng();
  if ( specialization() == MONK_BREWMASTER )
    rppm.boiling_brew = get_rppm( "boiling_brew", find_spell( 272797 ) );
}

// monk_t::init_resources ===================================================

void monk_t::init_resources( bool force )
{
  player_t::init_resources( force );
}

// monk_t::reset ============================================================

void monk_t::reset()
{
  base_t::reset();

  spiritual_focus_count = 0;
  combo_strike_actions.clear();
  stagger_tick_damage.clear();
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
        return spec.leather_specialization->effectN( 1 ).percent();
      break;
    case MONK_WINDWALKER:
      if ( attr == ATTR_AGILITY )
        return spec.leather_specialization->effectN( 1 ).percent();
      break;
    case MONK_BREWMASTER:
      if ( attr == ATTR_STAMINA )
        return spec.leather_specialization->effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return 0.0;
}

// monk_t::recalculate_resource_max =========================================

void monk_t::recalculate_resource_max( resource_e r, gain_t* source )
{
  player_t::recalculate_resource_max( r, source );
}

// monk_t::summon_storm_earth_and_fire ================================================

void monk_t::summon_storm_earth_and_fire( timespan_t duration )
{
  auto targets   = create_storm_earth_and_fire_target_list();
  auto n_targets = targets.size();

  // Start targeting logic from "owner" always
  pets.sef[ SEF_EARTH ]->reset_targeting();
  pets.sef[ SEF_EARTH ]->target = target;
  pets.sef[ SEF_EARTH ]->sticky_target = false;
  retarget_storm_earth_and_fire( pets.sef[ SEF_EARTH ], targets, n_targets );
  pets.sef[ SEF_EARTH ]->summon( duration );

  // Start targeting logic from "owner" always
  pets.sef[ SEF_FIRE ]->reset_targeting();
  pets.sef[ SEF_FIRE ]->target = target;
  pets.sef[ SEF_FIRE ]->sticky_target = false;
  retarget_storm_earth_and_fire( pets.sef[ SEF_FIRE ], targets, n_targets );
  pets.sef[ SEF_FIRE ]->summon( duration );
}

// monk_t::create_storm_earth_and_fire_target_list ====================================

std::vector<player_t*> monk_t::create_storm_earth_and_fire_target_list() const
{
  // Make a copy of the non sleeping target list
  auto l = sim->target_non_sleeping_list.data();

  // Sort the list by selecting non-cyclone striked targets first, followed by ascending order of
  // the debuff remaining duration
  range::sort( l, [this]( player_t* l, player_t* r ) {
    auto lcs = get_target_data( l )->debuff.mark_of_the_crane;
    auto rcs = get_target_data( r )->debuff.mark_of_the_crane;
    // Neither has cyclone strike
    if ( !lcs->check() && !rcs->check() )
    {
      return false;
    }
    // Left side does not have cyclone strike, right side does
    else if ( !lcs->check() && rcs->check() )
    {
      return true;
    }
    // Left side has cyclone strike, right side does not
    else if ( lcs->check() && !rcs->check() )
    {
      return false;
    }

    // Both have cyclone strike, order by remaining duration, use actor index as a tiebreaker
    timespan_t lv = lcs->remains(), rv = rcs->remains();
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv < rv;
  } );

  if ( sim->debug )
  {
    sim->out_debug.print( "{} storm_earth_and_fire target list, n_targets={}", *this, l.size() );
    range::for_each( l, [this]( player_t* t ) {
      sim->out_debug.print( "{} cs={}", *t,
                             get_target_data( t )->debuff.mark_of_the_crane->remains().total_seconds() );
    } );
  }

  return l;
}

void monk_t::accumulate_gale_burst_damage( action_state_t* s )
{
  if ( !s->action->harmful )
    return;
}

// monk_t::retarget_storm_earth_and_fire ====================================

void monk_t::retarget_storm_earth_and_fire( pet_t* pet, std::vector<player_t*>& targets, size_t n_targets ) const
{
  player_t* original_target = pet->target;

  // Clones will now only re-target when you use an ability that applies Mark of the Crane, and their current target
  // already has Mark of the Crane. https://us.battle.net/forums/en/wow/topic/20752377961?page=29#post-573
  if ( !get_target_data( original_target )->debuff.mark_of_the_crane->up() )
    return;

  // Everyone attacks the same (single) target
  if ( n_targets == 1 )
  {
    pet->target = targets.front();
  }
  // Pets attack the target the owner is not attacking
  else if ( n_targets == 2 )
  {
    pet->target = targets.front() == pet->owner->target ? targets.back() : targets.front();
  }
  // 3 targets, split evenly by skipping the owner's target and picking the first available target
  else if ( n_targets == 3 )
  {
    auto it = targets.begin();
    while ( it != targets.end() )
    {
      // Don't attack owner's target
      if ( *it == pet->owner->target )
      {
        it++;
        continue;
      }

      pet->target = *it;
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
      if ( *it == pet->owner->target )
      {
        it++;
        continue;
      }

      // Don't attack my own target
      if ( *it == pet->target )
      {
        it++;
        continue;
      }

      // Clones will no longer target Immune enemies, or crowd-controlled enemies, or enemies you aren’t in combat with.
      // https://us.battle.net/forums/en/wow/topic/20752377961?page=29#post-573
      player_t* player = *it;
      if ( player->debuffs.invulnerable )
      {
        it++;
        continue;
      }

      pet->target = *it;
      // This target has been chosen, so remove from the list (so that the second pet can choose
      // something else)
      targets.erase( it );
      break;
    }
  }

  if ( sim->debug )
  {
    sim->out_debug.printf( "%s storm_earth_and_fire %s (re)target=%s old_target=%s", name(), pet->name(),
                           pet->target->name(), original_target->name() );
  }

  range::for_each( pet->action_list,
                   [pet]( action_t* a ) { a->acquire_target( retarget_source::SELF_ARISE, nullptr, pet->target ); } );
}

// monk_t::retarget_storm_earth_and_fire_pets =======================================

void monk_t::retarget_storm_earth_and_fire_pets() const
{
  if ( pets.sef[ SEF_EARTH ]->sticky_target == true )
  {
    return;
  }

  auto targets   = create_storm_earth_and_fire_target_list();
  auto n_targets = targets.size();
  retarget_storm_earth_and_fire( pets.sef[ SEF_EARTH ], targets, n_targets );
  retarget_storm_earth_and_fire( pets.sef[ SEF_FIRE ], targets, n_targets );
}

// monk_t::has_stagger ======================================================

bool monk_t::has_stagger()
{
  return active_actions.stagger_self_damage->stagger_ticking();
}

// monk_t::partial_clear_stagger_pct ====================================================

double monk_t::partial_clear_stagger_pct( double clear_percent )
{
  return active_actions.stagger_self_damage->clear_partial_damage_pct( clear_percent );
}

// monk_t::partial_clear_stagger_amount =================================================

double monk_t::partial_clear_stagger_amount( double clear_amount )
{
  return active_actions.stagger_self_damage->clear_partial_damage_amount( clear_amount );
}

// monk_t::clear_stagger ==================================================

double monk_t::clear_stagger()
{
  return active_actions.stagger_self_damage->clear_all_damage();
}

// monk_t::stagger_total ==================================================

double monk_t::stagger_total()
{
  return active_actions.stagger_self_damage->stagger_total();
}

/**
 * Haste modifiers affecting both melee_haste and spell_haste.
 */
double shared_composite_haste_modifiers( const monk_t& p, double h )
{
//  if ( p.buff.sephuzs_secret && p.buff.sephuzs_secret->check() )
//  {
//    h *= 1.0 / ( 1.0 + p.buff.sephuzs_secret->stack_value() );
//  }

  // 7.2 Sephuz's Secret passive haste. If the item is missing, default_chance will be set to 0 (by
  // the fallback buff creator).
//  if ( p.legendary.sephuzs_secret && p.level() < 120 )
//  {
//    h *= 1.0 / ( 1.0 + p.legendary.sephuzs_secret->effectN( 3 ).percent() );
//  }

  if ( p.talent.high_tolerance->ok() )
  {
    int effect_index = 2;  // Effect index of HT affecting each stagger buff
    for ( auto&& buff : {p.buff.light_stagger, p.buff.moderate_stagger, p.buff.heavy_stagger} )
    {
      if ( buff && buff->check() )
      {
        h *= 1.0 / ( 1.0 + p.talent.high_tolerance->effectN( effect_index ).percent() );
      }
      ++effect_index;
    }
  }

  return h;
}

// monk_t::composite_spell_haste =========================================

double monk_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  h = shared_composite_haste_modifiers( *this, h );

  return h;
}

// monk_t::composite_melee_haste =========================================

double monk_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  h = shared_composite_haste_modifiers( *this, h );

  return h;
}

// monk_t::composite_melee_crit_chance ============================================

double monk_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spec.critical_strikes->effectN( 1 ).percent();

  return crit;
}

// monk_t::composite_melee_crit_chance_multiplier ===========================

double monk_t::composite_melee_crit_chance_multiplier() const
{
  double crit = player_t::composite_melee_crit_chance_multiplier();

  return crit;
}

// monk_t::composite_spell_crit_chance ============================================

double monk_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spec.critical_strikes->effectN( 1 ).percent();

  return crit;
}

// monk_t::composte_spell_crit_chance_multiplier===================================

double monk_t::composite_spell_crit_chance_multiplier() const
{
  double crit = player_t::composite_spell_crit_chance_multiplier();

  return crit;
}

// monk_t::composite_player_multiplier =====================================

double monk_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

// monk_t::composite_attribute_multiplier =====================================

double monk_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double cam = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA )
  {
    cam *= 1.0 + spec.brewmaster_monk->effectN( 11 ).percent();
  }

  return cam;
}

// monk_t::composite_player_heal_multiplier ==================================

double monk_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  return m;
}

// monk_t::composite_melee_expertise ========================================

double monk_t::composite_melee_expertise( const weapon_t* weapon ) const
{
  double e = player_t::composite_melee_expertise( weapon );

  e += spec.brewmaster_monk->effectN( 15 ).percent();

  return e;
}

// monk_t::composite_melee_attack_power ==================================

double monk_t::composite_melee_attack_power() const
{
  if ( specialization() == MONK_MISTWEAVER )
    return composite_spell_power( SCHOOL_MAX );

  return player_t::composite_melee_attack_power();
}

double monk_t::composite_melee_attack_power_by_type( attack_power_type ap_type ) const
{
  return player_t::composite_melee_attack_power_by_type( ap_type );
}

// monk_t::composite_attack_power_multiplier() ==========================

double monk_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.elusive_brawler->ok() )
    ap *= 1.0 + cache.mastery() * mastery.elusive_brawler->effectN( 2 ).mastery_value();

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

  if ( buff.elusive_brawler->up() )
    d += buff.elusive_brawler->current_stack * cache.mastery_value();

  return d;
}

// monk_t::composite_crit_avoidance =====================================

double monk_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.brewmaster_monk->effectN( 13 ).percent();

  return c;
}

// monk_t::composite_mastery ===========================================

double monk_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  if ( buff.weapons_of_order->up() )
  {
    m += buff.weapons_of_order->value();
    if ( conduit.strike_with_clarity->ok() )
      m += conduit.strike_with_clarity.value();
  }

  return m;
}

// monk_t::composite_mastery_rating ====================================

double monk_t::composite_mastery_rating() const
{
  double m = player_t::composite_mastery_rating();

  if ( buff.combo_master->up() )
    m += buff.combo_master->value();

  return m;
}

// monk_t::composite_rating_multiplier =================================

double monk_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  return m;
}

// monk_t::composite_armor_multiplier ===================================

double monk_t::composite_base_armor_multiplier() const
{
  double a = player_t::composite_base_armor_multiplier();

  a *= 1 + spec.brewmasters_balance->effectN( 1 ).percent();

  if ( buff.mighty_pour->up() )
    a *= 1 + buff.mighty_pour->data().effectN( 1 ).percent();

  return a;
}

// monk_t::resource_gain ================================================

double monk_t::resource_gain( resource_e r, double a, gain_t* g, action_t* action )
{
  return player_t::resource_gain( r, a, g, action );
}

// monk_t::temporary_movement_modifier =====================================

double monk_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

//  if ( buff.sephuzs_secret->up() )
//    active = std::max( buff.sephuzs_secret->data().effectN( 1 ).percent(), active );

  if ( buff.chi_torpedo->up() )
    active = std::max( buff.chi_torpedo->stack_value(), active );

  if ( buff.flying_serpent_kick_movement->up() )
    active = std::max( buff.flying_serpent_kick_movement->value(), active );

  return active;
}

// monk_t::passive_movement_modifier =======================================

double monk_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  // 7.2 Sephuz's Secret passive movement speed. If the item is missing, default_chance will be set
  // to 0 (by the fallback buff creator).
//  if ( legendary.sephuzs_secret && level() < 120 )
//    ms += legendary.sephuzs_secret->effectN( 2 ).percent();

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
      if ( spec.bladed_armor->ok() )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_MASTERY:
      if ( specialization() == MONK_WINDWALKER )
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;
    default:
      break;
  }
}

// monk_t::create_options ===================================================

void monk_t::create_options()
{
  base_t::create_options();

  add_option( opt_int( "initial_chi", user_options.initial_chi ) );
  add_option(
      opt_float( "memory_of_lucid_dreams_proc_chance", user_options.memory_of_lucid_dreams_proc_chance, 0.0, 1.0 ) );
  add_option( opt_float( "expel_harm_effectiveness", user_options.expel_harm_effectiveness, 0.0, 1.0 ) );
  add_option( opt_float( "faeline_stomp_uptime", user_options.faeline_stomp_uptime, 0.0, 1.0 ) );
}

// monk_t::copy_from =========================================================

void monk_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  monk_t* source_p = debug_cast<monk_t*>( source );

  user_options = source_p->user_options;
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
    return ROLE_HYBRID;  // To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == MONK_BREWMASTER )
    return ROLE_TANK;

  if ( specialization() == MONK_MISTWEAVER )
    return ROLE_ATTACK;  // To prevent spawning healing_target, as there is no support for healing.

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
    default:
      return s;
  }
}

// monk_t::pre_analyze_hook  ================================================

void monk_t::pre_analyze_hook()
{
  sample_datas.stagger_effective_damage_timeline.adjust( *sim );
  sample_datas.stagger_damage_pct_timeline.adjust( *sim );
  sample_datas.stagger_pct_timeline.adjust( *sim );

  base_t::pre_analyze_hook();
}

// monk_t::energy_regen_per_second ==========================================

double monk_t::resource_regen_per_second( resource_e r ) const
{
  double reg = base_t::resource_regen_per_second( r );

  if ( r == RESOURCE_ENERGY )
  {
    reg *= 1.0 + talent.ascension->effectN( 2 ).percent();
  }
  else if ( r == RESOURCE_MANA )
  {
    reg *= 1.0 + spec.mistweaver_monk->effectN( 8 ).percent();
  }

  // Memory of Lucid Dreams
  if ( player_t::buffs.memory_of_lucid_dreams->check() )
    reg *= 1.0 + player_t::buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();

  return reg;
}

// monk_t::combat_begin ====================================================

void monk_t::combat_begin()
{
  base_t::combat_begin();

  if ( specialization() == MONK_WINDWALKER )
  {
    if ( sim->distance_targeting_enabled )
    {
      buff.windwalking_driver->trigger();
    }
    else
    {
      buffs.windwalking_movement_aura->trigger(
          1,
          buffs.windwalking_movement_aura->data().effectN( 1 ).percent(),
          1, timespan_t::zero() );
    }

    if ( user_options.initial_chi > 0 )
      resources.current[ RESOURCE_CHI ] =
          clamp( as<double>( user_options.initial_chi + resources.current[ RESOURCE_CHI ] ), 0.0,
                 resources.max[ RESOURCE_CHI ] );
    sim->print_debug( "Combat starting chi has been set to {}", resources.current[ RESOURCE_CHI ] );
  }

  if ( spec.bladed_armor->ok() )
    buff.bladed_armor->trigger();
}

// monk_t::assess_damage ====================================================

void monk_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  buff.fortifying_brew->up();
  if ( specialization() == MONK_BREWMASTER )
  {
    if ( s->result == RESULT_DODGE )
    {
      if ( buff.elusive_brawler->up() )
        buff.elusive_brawler->expire();
    }
  }

  if ( action_t::result_is_hit( s->result ) && s->action->id != passives.stagger_self_damage->id() )
  {
    // trigger the mastery if the player gets hit by a physical attack; but not from stagger
    if ( school == SCHOOL_PHYSICAL )
      buff.elusive_brawler->trigger();
  }

  base_t::assess_damage( school, dtype, s );
}

// monk_t::target_mitigation ====================================================

void monk_t::target_mitigation( school_e school, result_amount_type dt, action_state_t* s )
{
  // Gift of the Ox Trigger Calculations ===========================================================

  // Gift of the Ox is no longer a random chance, under the hood. When you are hit, it increments a counter by
  // (DamageTakenBeforeAbsorbsOrStagger / MaxHealth). It now drops an orb whenever that reaches 1.0, and decrements it
  // by 1.0. The tooltip still says ‘chance’, to keep it understandable.
  if ( s->action->id != passives.stagger_self_damage->id() )
  {
    double goto_proc_chance = s->result_amount / max_health();

    gift_of_the_ox_proc_chance += goto_proc_chance;

    if ( gift_of_the_ox_proc_chance > 1.0 )
    {
      buff.gift_of_the_ox->trigger();

      gift_of_the_ox_proc_chance -= 1.0;
    }
  }

  // Dampen Harm // Reduces hits by 20 - 50% based on the size of the hit
  // Works on Stagger
  if ( buff.dampen_harm->up() )
  {
    double dampen_max_percent = buff.dampen_harm->data().effectN( 3 ).percent();
    if ( s->result_amount >= max_health() )
      s->result_amount *= 1 - dampen_max_percent;
    else
    {
      double dampen_min_percent = buff.dampen_harm->data().effectN( 2 ).percent();
      s->result_amount *= 1 - ( dampen_min_percent +
                                ( ( dampen_max_percent - dampen_min_percent ) * ( s->result_amount / max_health() ) ) );
    }
  }

  // Stagger is not reduced by damage mitigation effects
  if ( s->action->id == passives.stagger_self_damage->id() )
  {
    return;
  }

  // Diffuse Magic
  if ( buff.diffuse_magic->up() && school != SCHOOL_PHYSICAL )
    s->result_amount *= 1.0 + buff.diffuse_magic->default_value;  // Stored as -60%

  // If Breath of Fire is ticking on the source target, the player receives 5% less damage
  if ( get_target_data( s->action->player )->dots.breath_of_fire->is_ticking() )
  {
    // Saved as -5
    double dmg_reduction = passives.breath_of_fire_dot->effectN( 2 ).percent();

    if ( buff.celestial_flames->up() )
      dmg_reduction -= buff.celestial_flames->value(); // Saved as 5
    s->result_amount *= 1.0 + dmg_reduction;
  }

  // Inner Strength
  if ( buff.inner_stength->up() )
    s->result_amount *= 1.0 + buff.inner_stength->stack_value();

  // Damage Reduction Cooldowns
  if ( buff.fortifying_brew->up() )
  {
    double reduction = spec.fortifying_brew_mw_ww->effectN( 2 ).percent(); // Saved as -15%
    if ( spec.fortifying_brew_2_brm->ok() )
      reduction -= 0.05;

    s->result_amount *= ( 1.0 + reduction );
  }

  // Touch of Karma Absorbtion
  if ( buff.touch_of_karma->up() )
  {
    double percent_HP = spec.touch_of_karma->effectN( 3 ).percent() * max_health();
    if ( ( buff.touch_of_karma->value() + s->result_amount ) >= percent_HP )
    {
      double difference = percent_HP - buff.touch_of_karma->value();
      buff.touch_of_karma->current_value += difference;
      s->result_amount -= difference;
      buff.touch_of_karma->expire();
    }
    else
    {
      buff.touch_of_karma->current_value += s->result_amount;
      s->result_amount = 0;
    }
  }

  player_t::target_mitigation( school, dt, s );
}

// monk_t::assess_damage_imminent_pre_absorb ==============================

void monk_t::assess_damage_imminent_pre_absorb( school_e school, result_amount_type dtype, action_state_t* s )
{
  base_t::assess_damage_imminent_pre_absorb( school, dtype, s );

  if ( specialization() == MONK_BREWMASTER )
  {
    // Stagger damage can't be staggered!
    if ( s->action->id == passives.stagger_self_damage->id() )
      return;

    // Stagger Calculation
    double stagger_dmg = 0;

    if ( s->result_amount > 0 )
    {
      if ( school == SCHOOL_PHYSICAL )
        stagger_dmg += s->result_amount * stagger_pct( s->target->level() );

      else if ( spec.stagger_2->ok() && school != SCHOOL_PHYSICAL )
      {
        double stagger_magic = stagger_pct( s->target->level() ) * spec.stagger_2->effectN( 1 ).percent();

        stagger_dmg += s->result_amount * stagger_magic;
      }

      s->result_amount -= stagger_dmg;
      s->result_mitigated -= stagger_dmg;
    }
    // Hook up Stagger Mechanism
    if ( stagger_dmg > 0 )
    {
      // Blizzard is putting a cap on how much damage can go into stagger
      double amount_remains = active_actions.stagger_self_damage->amount_remaining();
      double cap            = max_health() * spec.stagger->effectN( 4 ).percent();
      if ( amount_remains + stagger_dmg >= cap )
      {
        double diff = amount_remains - cap;
        s->result_amount += stagger_dmg - diff;
        s->result_mitigated += stagger_dmg - diff;
        stagger_dmg -= diff;
      }
      sample_datas.stagger_total_damage->add( stagger_dmg );
      residual_action::trigger( active_actions.stagger_self_damage, this, stagger_dmg );
    }
  }
}

// monk_t::assess_heal ===================================================

void monk_t::assess_heal( school_e school, result_amount_type dmg_type, action_state_t* s )
{
  player_t::assess_heal( school, dmg_type, s );

  trigger_celestial_fortune( s );
}

// =========================================================================
// Monk APL
// =========================================================================

// monk_t::default_flask ===================================================
std::string monk_t::default_flask() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      if ( true_level >= 60 )
        return "spectral_flask_of_power";
      else if ( true_level >= 50 )
        return "currents";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( true_level >= 60 )
        return "spectral_flask_of_power";
      else if ( true_level >= 50 )
        return "greater_flask_of_endless_fathoms";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( true_level >= 60 )
        return "spectral_flask_of_power";
      else if ( true_level >= 50 )
        return "greater_flask_of_the_currents";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

// monk_t::default_potion ==================================================
std::string monk_t::default_potion() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      if ( true_level >= 60 )
        return "phantom_fire";
      else if ( true_level >= 50 )
        return "unbridled_fury";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( true_level >= 60 )
        return "potion_of_spectral_intellect";
      else if ( true_level >= 50 )
        return "superior_battle_potion_of_intellect";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( true_level >= 60 )
        return "potion_of_spectral_agility";
      else if ( true_level >= 50 )
        return "unbridled_fury";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

// monk_t::default_food ====================================================
std::string monk_t::default_food() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      if ( true_level >= 60 )
        return "spinefin_souffle_and_fries";
      else if ( true_level >= 50 )
        return "biltong";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( true_level >= 60 )
        return "feast_of_gluttonous_hedonism";
      else if ( true_level >= 50 )
        return "famine_evaluator_and_snack_table";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( true_level >= 60 )
        return "feast_of_gluttonous_hedonism";
      else if ( true_level >= 50 )
        return "mechdowels_big_mech";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

// monk_t::default_rune ====================================================
std::string monk_t::default_rune() const
{
  if ( true_level >= 60 )
    return "veiled";
  else if ( true_level >= 50 )
    return "battle_scarred";
  else if ( true_level >= 45 )
    return "defiled";
  return "disabled";
}

// monk_t::temporary_enchant ===============================================
std::string monk_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      if ( true_level >= 60 )
        return "main_hand:shadowcore_oil/off_hand:shadowcore_oil";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( true_level >= 60 )
        return "main_hand:shadowcore_oil";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( true_level >= 60 )
        return "main_hand:shaded_weightstone/off_hand:shaded_weightstone";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}


// Brewmaster Pre-Combat Action Priority List ============================

void monk_t::apl_pre_brewmaster()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  // Flask
  pre->add_action( "flask" );

  // Food
  pre->add_action( "food" );

  // Rune
  pre->add_action( "augmentation" );

  // Snapshot stats
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  pre->add_action( "potion" );

  pre->add_talent( this, "Chi Burst" );
  pre->add_talent( this, "Chi Wave" );
}

// Windwalker Pre-Combat Action Priority List ==========================

void monk_t::apl_pre_windwalker()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  // Flask
  pre->add_action( "flask" );
  // Food
  pre->add_action( "food" );
  // Rune
  pre->add_action( "augmentation" );

  // Snapshot stats
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  pre->add_action( "potion" );
  pre->add_action( "variable,name=xuen_on_use_trinket,op=set,value=0" );
  pre->add_talent( this, "Chi Burst" );
  pre->add_talent( this, "Chi Wave", "if=!talent.energizing_elixir.enabled" );
}

// Mistweaver Pre-Combat Action Priority List ==========================

void monk_t::apl_pre_mistweaver()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  // Flask
  pre->add_action( "flask" );

  // Food
  pre->add_action( "food" );

  // Rune
  pre->add_action( "augmentation" );

  // Snapshot stats
  pre->add_action( "snapshot_stats" );

  pre->add_action( "potion" );

  pre->add_talent( this, "Chi Burst" );
  pre->add_talent( this, "Chi Wave" );
}

// Brewmaster Combat Action Priority List =========================

void monk_t::apl_combat_brewmaster()

{
  std::vector<std::string> racial_actions = get_racial_actions();
  action_priority_list_t* def             = get_action_priority_list( "default" );
  def->add_action( "auto_attack" );
  def->add_action( this, "Spear Hand Strike", "if=target.debuff.casting.react" );
  def->add_action( this, "Gift of the Ox", "if=health<health.max*0.65" );
  def->add_talent( this, "Dampen Harm", "if=incoming_damage_1500ms&buff.fortifying_brew.down" );
  def->add_action( this, "Fortifying Brew", "if=incoming_damage_1500ms&(buff.dampen_harm.down|buff.diffuse_magic.down)" );

  for ( size_t i = 0; i < items.size(); i++ )
  {
    std::string name_str = "";
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      def->add_action( "use_item,name=" + items[ i ].name_str );
    }
  }

  def->add_action( "potion" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] != "arcane_torrent" )
      def->add_action( racial_actions[ i ] );
  }

  def->add_action( this, spec.invoke_niuzao, "invoke_niuzao_the_black_ox", "if=target.time_to_die>25" );
  def->add_action( this, "Touch of Death", "if=target.health.pct<=15" );

  // Covenant Abilities
  def->add_action( "weapons_of_order" );
  def->add_action( "fallen_order" );
  def->add_action( "bonedust_brew" );

  // Purifying Brew
  def->add_action( this, "Purifying Brew" );

  // Black Ox Brew
  def->add_talent( this, "Black Ox Brew", "if=cooldown.purifying_brew.charges_fractional<0.5",
                   "Black Ox Brew is currently used to either replenish brews based on less than half a brew charge available, or low energy to enable Keg Smash" );
  def->add_talent( this, "Black Ox Brew", "if=(energy+(energy.regen*cooldown.keg_smash.remains))<40&buff.blackout_combo.down&cooldown.keg_smash.up" );

  def->add_action( this, "Keg Smash", "if=spell_targets>=2",
      "Offensively, the APL prioritizes KS on cleave, BoS else, with energy spenders and cds sorted below" );

  // Covenant Faeline Stomp
  def->add_action( "faeline_stomp,if=spell_targets>=2" );

  def->add_action( this, "Keg Smash", "if=buff.weapons_of_order.up",
      "cast KS at top prio during WoO buff" );

  // Celestial Brew
  def->add_action( this, "Celestial Brew", "if=buff.blackout_combo.down&incoming_damage_1999ms>(health.max*0.1+stagger.last_tick_damage_4)&buff.elusive_brawler.stack<2",
      "Celestial Brew priority whenever it took significant damage (adjust the health.max coefficient according to intensity of damage taken), and to dump excess charges before BoB." );

  def->add_action( this, "Tiger Palm", "if=talent.rushing_jade_wind.enabled&buff.blackout_combo.up&buff.rushing_jade_wind.up" );
  def->add_action( this, "Breath of Fire", "if=buff.charred_passions.down&runeforge.charred_passions.equipped" );
  def->add_action( this, "Blackout Kick" );
  def->add_action( this, "Keg Smash" );

  // Covenant Faeline Stomp
  def->add_action( "faeline_stomp" );

  def->add_action( this, "Expel Harm", "if=buff.gift_of_the_ox.stack>=3" );
  def->add_action( this, "Touch of Death" );
  def->add_talent( this, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down" );
  def->add_action( this, "Spinning Crane Kick", "if=buff.charred_passions.up" );
  def->add_action( this, "Breath of Fire", "if=buff.blackout_combo.down&(buff.bloodlust.down|(buff.bloodlust.up&dot.breath_of_fire_dot.refreshable))" );
  def->add_talent( this, "Chi Burst" );
  def->add_talent( this, "Chi Wave" );
  def->add_action( this, "Spinning Crane Kick", "if=active_enemies>=3&cooldown.keg_smash.remains>gcd&(energy+(energy.regen*(cooldown.keg_smash.remains+execute_time)))>=65&(!talent.spitfire.enabled|!runeforge.charred_passions.equipped)" );
  def->add_action( this, "Tiger Palm", "if=!talent.blackout_combo&cooldown.keg_smash.remains>gcd&(energy+(energy.regen*(cooldown.keg_smash.remains+gcd)))>=65" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      def->add_action( racial_actions[ i ] + ",if=energy<31" );
  }

  def->add_talent( this, "Rushing Jade Wind" );


//  def->add_action( this, "Expel Harm", "if=buff.gift_of_the_ox.stack>4" );

}

// Windwalker Combat Action Priority List ===============================

void monk_t::apl_combat_windwalker()
{
  std::vector<std::string> racial_actions  = get_racial_actions();
  action_priority_list_t* def              = get_action_priority_list( "default" );
  action_priority_list_t* opener           = get_action_priority_list( "opener" );
  action_priority_list_t* cd_sef           = get_action_priority_list( "cd_sef" );
  action_priority_list_t* cd_serenity      = get_action_priority_list( "cd_serenity" );
  action_priority_list_t* serenity         = get_action_priority_list( "serenity" );
  action_priority_list_t* weapons_of_order = get_action_priority_list( "weapons_of_order" );
  action_priority_list_t* aoe              = get_action_priority_list( "aoe" );
  action_priority_list_t* st               = get_action_priority_list( "st" );

  def->add_action( "auto_attack", "Executed every time the actor is available." );
  def->add_action( this, "Spear Hand Strike", "if=target.debuff.casting.react" );
  def->add_action(
      "variable,name=hold_xuen,op=set,value=cooldown.invoke_xuen_the_white_tiger.remains>fight_remains|fight_remains<120&fight_remains>cooldown.serenity.remains&cooldown.serenity.remains>10" );
  if ( sim->allow_potions )
  {
    if ( spec.invoke_xuen->ok() )
      def->add_action(
          "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&pet.xuen_the_white_tiger.active|fight_remains<=60" );
    else
      def->add_action( "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&fight_remains<=60" );
  }

  def->add_action( "call_action_list,name=serenity,if=buff.serenity.up" );
  def->add_action( "call_action_list,name=weapons_of_order,if=buff.weapons_of_order.up" );
  if ( spec.invoke_xuen->ok() )
    def->add_action( "call_action_list,name=opener,if=time<4&chi<5&!pet.xuen_the_white_tiger.active" );
  else
    def->add_action( "call_action_list,name=opener,if=time<4&chi<5" );
  def->add_talent( this, "Fist of the White Tiger",
      "target_if=min:debuff.mark_of_the_crane.remains,if=chi.max-chi>=3&(energy.time_to_max<1|energy.time_to_max<4&cooldown.fists_of_fury.remains<1.5|cooldown.weapons_of_order.remains<2)" );
  def->add_action( this, "Expel Harm",
      "if=chi.max-chi>=1&(energy.time_to_max<1|cooldown.serenity.remains<2|energy.time_to_max<4&cooldown.fists_of_fury.remains<1.5|cooldown.weapons_of_order.remains<2)" );
  def->add_action( this, "Tiger Palm",
      "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&chi.max-chi>=2&(energy.time_to_max<1|cooldown.serenity.remains<2|energy.time_to_max<4&cooldown.fists_of_fury.remains<1.5|cooldown.weapons_of_order.remains<2)" );
  def->add_action( "call_action_list,name=cd_sef,if=!talent.serenity" );
  def->add_action( "call_action_list,name=cd_serenity,if=talent.serenity" );
  def->add_action( "call_action_list,name=st,if=active_enemies<3" );
  def->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );

  // Opener
  opener->add_talent( this, "Fist of the White Tiger", "target_if=min:debuff.mark_of_the_crane.remains,if=chi.max-chi>=3" );
  opener->add_action( this, "Expel Harm", "if=talent.chi_burst.enabled&chi.max-chi>=3" );
  opener->add_action( this, "Tiger Palm", "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=combo_strike&chi.max-chi>=2" );
  opener->add_talent( this, "Chi Wave", "if=chi.max-chi=2" );
  opener->add_action( this, "Expel Harm" );
  opener->add_action( this, "Tiger Palm", "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=chi.max-chi>=2" );

  // AoE
  aoe->add_talent( this, "Whirling Dragon Punch" );
  aoe->add_talent( this, "Energizing Elixir", "if=chi.max-chi>=2&energy.time_to_max>2|chi.max-chi>=4" );
  aoe->add_action( this, "Spinning Crane Kick", "if=combo_strike&(buff.dance_of_chiji.up|debuff.bonedust_brew.up)" );
  aoe->add_action( this, "Fists of Fury", "if=energy.time_to_max>execute_time|chi.max-chi<=1" );
  aoe->add_action( this, "Rising Sun Kick",
      "target_if=min:debuff.mark_of_the_crane.remains,if=(talent.whirling_dragon_punch&cooldown.rising_sun_kick.duration>cooldown.whirling_dragon_punch.remains+4)&(cooldown.fists_of_fury.remains>3|chi>=5)" );
  aoe->add_talent( this, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down" );
  aoe->add_action( this, "Spinning Crane Kick",
      "if=combo_strike&((cooldown.bonedust_brew.remains>2&(chi>3|cooldown.fists_of_fury.remains>6)&(chi>=5|cooldown.fists_of_fury.remains>2))|energy.time_to_max<=3)" );
  aoe->add_action( this, "Expel Harm", "if=chi.max-chi>=1" );
  aoe->add_talent( this, "Fist of the White Tiger", "target_if=min:debuff.mark_of_the_crane.remains,if=chi.max-chi>=3" );
  aoe->add_talent( this, "Chi Burst", "if=chi.max-chi>=2" );
  aoe->add_action( this, "Crackling Jade Lightning", "if=buff.the_emperors_capacitor.stack>19&energy.time_to_max>execute_time-1&cooldown.fists_of_fury.remains>execute_time" );
  aoe->add_action( this, "Tiger Palm",
      "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=chi.max-chi>=2&(!talent.hit_combo|combo_strike)" );
  aoe->add_talent( this, "Chi Wave", "if=combo_strike" );
  aoe->add_action( this, "Flying Serpent Kick", "if=buff.bok_proc.down,interrupt=1" );
  aoe->add_action( this, "Blackout Kick",
      "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(buff.bok_proc.up|talent.hit_combo&prev_gcd.1.tiger_palm&chi=2&cooldown.fists_of_fury.remains<3|chi.max-chi<=1&prev_gcd.1.spinning_crane_kick&energy.time_to_max<3)" );

  // Serenity Cooldowns
  if ( spec.invoke_xuen->ok() )
    cd_serenity->add_action( "variable,name=serenity_burst,op=set,value=cooldown.serenity.remains<1|pet.xuen_the_white_tiger.active&cooldown.serenity.remains>30|fight_remains<20" );
  else
    cd_serenity->add_action( "variable,name=serenity_burst,op=set,value=cooldown.serenity.remains<1|cooldown.serenity.remains>30|fight_remains<20" );
  cd_serenity->add_action( this, "Invoke Xuen, the White Tiger", "if=!variable.hold_xuen|fight_remains<25" );

  // Serenity On-use items
  for ( size_t i = 0; i < items.size(); i++ )
  {
    std::string name_str = "";
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      cd_serenity->add_action( "use_item,name=" + items[ i ].name_str );
    }
  }

  // Serenity Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=chi.max-chi>=1" );
    else if ( racial_actions[ i ] == "ancestral_call" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] == "blood_fury" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] == "fireblood" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] == "berserking" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] == "bag_of_tricks" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else
      cd_serenity->add_action( racial_actions[ i ] );
  }

  if ( spec.invoke_xuen->ok() )
  {
    cd_serenity->add_action( this, "Touch of Death", "if=fight_remains>180|pet.xuen_the_white_tiger.active|fight_remains<10" );
    cd_serenity->add_action( this, "Touch of Karma", "if=fight_remains>90|pet.xuen_the_white_tiger.active|fight_remains<10" );
  }
  else
  {
    cd_serenity->add_action( this, "Touch of Death", "if=fight_remains>180|fight_remains<10" );
    cd_serenity->add_action( this, "Touch of Karma", "if=fight_remains>90|fight_remains<10" );
  }
   

  // Serenity Covenant Abilities
  cd_serenity->add_action( "weapons_of_order,if=cooldown.rising_sun_kick.remains<execute_time" );
  cd_serenity->add_action( "faeline_stomp" );
  cd_serenity->add_action( "fallen_order" );
  cd_serenity->add_action( "bonedust_brew" );

  cd_serenity->add_talent( this, "Serenity", "if=cooldown.rising_sun_kick.remains<2|fight_remains<15" );
  cd_serenity->add_action( "bag_of_tricks" );

  // Storm, Earth and Fire Cooldowns
  cd_sef->add_action( this, "Invoke Xuen, the White Tiger", "if=!variable.hold_xuen|fight_remains<25" );

  // Storm, Earth, and Fire w/ Arcane Torrent
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      cd_sef->add_action( racial_actions[ i ] + ",if=chi.max-chi>=1" );
  }

  if ( spec.invoke_xuen->ok() )
    cd_sef->add_action( this, "Touch of Death", "if=buff.storm_earth_and_fire.down&pet.xuen_the_white_tiger.active|fight_remains<10|fight_remains>180" );
  else
    cd_sef->add_action( this, "Touch of Death", "if=buff.storm_earth_and_fire.down|fight_remains<10|fight_remains>180" );
  
  // Storm, Earth, and Fire Covenant Abilities
  cd_sef->add_action( "weapons_of_order,if=(raid_event.adds.in>45|raid_event.adds.up)&cooldown.rising_sun_kick.remains<execute_time" );
  cd_sef->add_action( "faeline_stomp,if=combo_strike&(raid_event.adds.in>10|raid_event.adds.up)" );
  cd_sef->add_action( "fallen_order,if=raid_event.adds.in>30|raid_event.adds.up" );
  cd_sef->add_action( "bonedust_brew,if=raid_event.adds.in>50|raid_event.adds.up,line_cd=60" );

  cd_sef->add_action( "storm_earth_and_fire_fixate,if=conduit.coordinated_offensive.enabled" );
  cd_sef->add_action( this, "Storm, Earth, and Fire", "if=cooldown.storm_earth_and_fire.charges=2|fight_remains<20|(raid_event.adds.remains>15|!covenant.kyrian&"
      "((raid_event.adds.in>cooldown.storm_earth_and_fire.full_recharge_time|!raid_event.adds.exists)&"
      "(cooldown.invoke_xuen_the_white_tiger.remains>cooldown.storm_earth_and_fire.full_recharge_time|variable.hold_xuen))&"
      "cooldown.fists_of_fury.remains<=9&chi>=2&cooldown.whirling_dragon_punch.remains<=12)" );
  cd_sef->add_action( this, "Storm, Earth, and Fire", "if=covenant.kyrian&(buff.weapons_of_order.up|(fight_remains<cooldown.weapons_of_order.remains|"
      "cooldown.weapons_of_order.remains>cooldown.storm_earth_and_fire.full_recharge_time)&cooldown.fists_of_fury.remains<=9&chi>=2&cooldown.whirling_dragon_punch.remains<=12)" );


  // Storm, Earth, and Fire on-use trinkets
  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      cd_sef->add_action( "use_item,name=" + items[ i ].name_str );
    }
  }

  if ( spec.invoke_xuen->ok() )
    cd_sef->add_action( this, "Touch of Karma", "if=fight_remains>159|pet.xuen_the_white_tiger.active|variable.hold_xuen" );
  else
    cd_sef->add_action( this, "Touch of Karma", "if=fight_remains>159|variable.hold_xuen" );

  // Storm, Earth and Fire Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] != "arcane_torrent" )
    {
      if ( racial_actions[ i ] == "ancestral_call" )
        cd_sef->add_action( racial_actions[ i ] + ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<20" );
      else if ( racial_actions[ i ] == "blood_fury" )
        cd_sef->add_action( racial_actions[ i ] + ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<20" );
      else if ( racial_actions[ i ] == "fireblood" )
        cd_sef->add_action( racial_actions[ i ] + ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<10" );
      else if ( racial_actions[ i ] == "berserking" )
        cd_sef->add_action( racial_actions[ i ] + ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<15" );
      else if ( racial_actions[ i ] == "bag_of_tricks" )
        cd_sef->add_action( racial_actions[ i ] + ",if=buff.storm_earth_and_fire.down" );
      else
	    cd_sef->add_action( racial_actions[ i ] );
    }
  }

  // Serenity
  serenity->add_action( this, "Fists of Fury", "if=buff.serenity.remains<1" );

  // Serenity on-use trinkets
  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      serenity->add_action( "use_item,name=" + items[ i ].name_str );
    }
  }

  serenity->add_action( this, "Spinning Crane Kick", "if=combo_strike&(active_enemies>=3|active_enemies>1&!cooldown.rising_sun_kick.up)" );
  serenity->add_action( this, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );
  serenity->add_action( this, "Fists of Fury", "if=active_enemies>=3" );
  serenity->add_action( this, "Spinning Crane Kick", "if=combo_strike&buff.dance_of_chiji.up" );
  serenity->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.weapons_of_order_ww.up&cooldown.rising_sun_kick.remains>2" );
  serenity->add_action( this, "Fists of Fury", "interrupt_if=!cooldown.rising_sun_kick.up" );
  serenity->add_action( this, "Spinning Crane Kick", "if=combo_strike&debuff.bonedust_brew.up" );
  serenity->add_talent( this, "Fist of the White Tiger", "target_if=min:debuff.mark_of_the_crane.remains,if=chi<3" );
  serenity->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike|!talent.hit_combo" );
  serenity->add_action( this, "Spinning Crane Kick" );

  // Weapons of Order
  weapons_of_order->add_action( "call_action_list,name=cd_sef,if=!talent.serenity" );
  weapons_of_order->add_action( "call_action_list,name=cd_serenity,if=talent.serenity" );
  weapons_of_order->add_talent( this, "Energizing Elixir", "if=chi.max-chi>=2&energy.time_to_max>3" );
  weapons_of_order->add_action( this, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains" );
  weapons_of_order->add_action( this, "Spinning Crane Kick", "if=combo_strike&buff.dance_of_chiji.up" );
  weapons_of_order->add_action( this, "Fists of Fury", "if=active_enemies>=2&buff.weapons_of_order_ww.remains<1" );
  weapons_of_order->add_talent( this, "Whirling Dragon Punch", "if=active_enemies>=2" );
  weapons_of_order->add_action( this, "Spinning Crane Kick", "if=combo_strike&active_enemies>=3&buff.weapons_of_order_ww.up" );
  weapons_of_order->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&active_enemies<=2" );
  weapons_of_order->add_talent( this, "Whirling Dragon Punch" );
  weapons_of_order->add_action( this, "Fists of Fury", "interrupt=1,if=buff.storm_earth_and_fire.up&raid_event.adds.in>cooldown.fists_of_fury.duration*0.6" );
  weapons_of_order->add_action( this, "Spinning Crane Kick", "if=buff.chi_energy.stack>30-5*active_enemies" );
  weapons_of_order->add_talent( this, "Fist of the White Tiger", "target_if=min:debuff.mark_of_the_crane.remains,if=chi<3" );
  weapons_of_order->add_action( this, "Expel Harm", "if=chi.max-chi>=1" );
  weapons_of_order->add_talent( this, "Chi Burst", "if=chi.max-chi>=(1+active_enemies>1)" );
  weapons_of_order->add_action( this, "Tiger Palm", "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=(!talent.hit_combo|combo_strike)&chi.max-chi>=2" );
  weapons_of_order->add_talent( this, "Chi Wave" );
  weapons_of_order->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=chi>=3|buff.weapons_of_order_ww.up" );
  weapons_of_order->add_action( this, "Flying Serpent Kick", "interrupt=1" );

  // Single Target
  st->add_talent( this, "Whirling Dragon Punch", "if=raid_event.adds.in>cooldown.whirling_dragon_punch.duration*0.8|raid_event.adds.up" );
  st->add_talent( this, "Energizing Elixir", "if=chi.max-chi>=2&energy.time_to_max>3|chi.max-chi>=4&(energy.time_to_max>2|!prev_gcd.1.tiger_palm)" );
  st->add_action( this, "Spinning Crane Kick", "if=combo_strike&buff.dance_of_chiji.up&(raid_event.adds.in>buff.dance_of_chiji.remains-2|raid_event.adds.up)" );
  st->add_action( this, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=cooldown.serenity.remains>1|!talent.serenity" );
  st->add_action( this, "Fists of Fury", "if=(raid_event.adds.in>cooldown.fists_of_fury.duration*0.8|raid_event.adds.up)&(energy.time_to_max>execute_time-1|chi.max-chi<=1|buff.storm_earth_and_fire.remains<execute_time+1)|fight_remains<execute_time+1" );
  st->add_action( this, "Crackling Jade Lightning", "if=buff.the_emperors_capacitor.stack>19&energy.time_to_max>execute_time-1&cooldown.rising_sun_kick.remains>execute_time|buff.the_emperors_capacitor.stack>14&(cooldown.serenity.remains<5&talent.serenity|cooldown.weapons_of_order.remains<5&covenant.kyrian|fight_remains<5)" );
  st->add_talent( this, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down&active_enemies>1" );
  st->add_talent( this, "Fist of the White Tiger", "target_if=min:debuff.mark_of_the_crane.remains,if=chi<3" );
  st->add_action( this, "Expel Harm", "if=chi.max-chi>=1" );
  st->add_talent( this, "Chi Burst", "if=chi.max-chi>=1&active_enemies=1&raid_event.adds.in>20|chi.max-chi>=2&active_enemies>=2" );
  st->add_talent( this, "Chi Wave" );
  st->add_action( this, "Tiger Palm", "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=combo_strike&chi.max-chi>=2&buff.storm_earth_and_fire.down" );
  st->add_action( this, "Spinning Crane Kick", "if=buff.chi_energy.stack>30-5*active_enemies&buff.storm_earth_and_fire.down&(cooldown.rising_sun_kick.remains>2&cooldown.fists_of_fury.remains>2|cooldown.rising_sun_kick.remains<3&cooldown.fists_of_fury.remains>3&chi>3|cooldown.rising_sun_kick.remains>3&cooldown.fists_of_fury.remains<3&chi>4|chi.max-chi<=1&energy.time_to_max<2)|buff.chi_energy.stack>10&fight_remains<7" );
  st->add_action( this, "Blackout Kick",
                  "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(talent.serenity&cooldown.serenity.remains<3|cooldown.rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>1|cooldown.rising_sun_kick.remains<3&cooldown.fists_of_fury.remains>3&chi>2|cooldown.rising_sun_kick.remains>3&cooldown.fists_of_fury.remains<3&chi>3|chi>5|buff.bok_proc.up)" );
  st->add_action( this, "Tiger Palm", "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=combo_strike&chi.max-chi>=2" );
  st->add_action( this, "Flying Serpent Kick", "interrupt=1" );
  st->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains<3&chi=2&prev_gcd.1.tiger_palm&energy.time_to_50<1" );
  st->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<2&(chi.max-chi<=1|prev_gcd.1.tiger_palm)" );

}  // namespace

// Mistweaver Combat Action Priority List ==================================

void monk_t::apl_combat_mistweaver()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  action_priority_list_t* def             = get_action_priority_list( "default" );
  action_priority_list_t* st              = get_action_priority_list( "st" );
  action_priority_list_t* aoe             = get_action_priority_list( "aoe" );

  def->add_action( "auto_attack" );
  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      def->add_action( "use_item,name=" + items[ i ].name_str );
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      def->add_action( racial_actions[ i ] + ",if=chi.max-chi>=1&target.time_to_die<18" );
    else
      def->add_action( racial_actions[ i ] + ",if=target.time_to_die<18" );
  }

  def->add_action( "potion" );

  def->add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
  def->add_action( "call_action_list,name=st,if=active_enemies<3" );

  // Covenant Abilities
  def->add_action( "weapons_of_order" );
  def->add_action( "faeline_stomp" );
  def->add_action( "fallen_order" );
  def->add_action( "bonedust_brew" );

  st->add_action( this, "Thunder Focus Tea" );
  st->add_action( this, "Rising Sun Kick" );
  st->add_action( this, "Blackout Kick",
                  "if=buff.teachings_of_the_monastery.stack=1&cooldown.rising_sun_kick.remains<12" );
  st->add_talent( this, "Chi Wave" );
  st->add_talent( this, "Chi Burst" );
  st->add_action( this, "Tiger Palm",
                  "if=buff.teachings_of_the_monastery.stack<3|buff.teachings_of_the_monastery.remains<2" );

  aoe->add_action( this, "Spinning Crane Kick" );
  aoe->add_talent( this, "Chi Wave" );
  aoe->add_talent( this, "Chi Burst" );
}

// monk_t::init_actions =====================================================

void monk_t::init_action_list()
{
  // Mistweaver isn't supported atm
  if ( !sim->allow_experimental_specializations && specialization() == MONK_MISTWEAVER && role != ROLE_ATTACK )
  {
    if ( !quiet )
      sim->error( "Monk mistweaver healing for {} is not currently supported.", *this );

    quiet = true;
    return;
  }
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->error( "{} has no weapon equipped at the Main-Hand slot.", *this );
    quiet = true;
    return;
  }
  if ( main_hand_weapon.group() == WEAPON_2H && off_hand_weapon.group() == WEAPON_1H )
  {
    if ( !quiet )
      sim->error(
          "{} has a 1-Hand weapon equipped in the Off-Hand while a 2-Hand weapon is equipped in the Main-Hand.",
          *this );
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
    default:
      break;
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

double monk_t::stagger_base_value()
{
  double stagger_base = 0.0;

  if ( specialization() == MONK_BREWMASTER )  // no stagger when not in Brewmaster Specialization
  {
    stagger_base = agility() * spec.stagger->effectN( 1 ).percent();

    if ( talent.high_tolerance->ok() )
      stagger_base *= 1 + talent.high_tolerance->effectN( 5 ).percent();

    if ( spec.fortifying_brew_2_brm->ok() && buff.fortifying_brew->up() )
      stagger_base *= 1 + passives.fortifying_brew->effectN( 6 ).percent();

    if ( bugs )
      // Hard coding the 125% multiplier until Blizzard fixes this
      if ( buff.faeline_stomp_brm->up() )
        stagger_base *= 1.0 + 0.25;

    if ( buff.shuffle->check() )
      stagger_base *= 1.0 + passives.shuffle->effectN( 1 ).percent();
  }

  return stagger_base;
}

/**
 * BFA stagger formula
 *
 * See https://us.battle.net/forums/en/wow/topic/20765536748#post-10
 * or http://blog.askmrrobot.com/diminishing-returns-other-bfa-tank-formulas/
 */
double monk_t::stagger_pct( int target_level )
{
  double stagger_base = stagger_base_value();

  double stagger = stagger_base / ( stagger_base + dbc->armor_mitigation_constant( target_level ) );

  // Bug: This is right now hard-coding the 5% instead of the estimated 125% multiplier to stagger_base
  if (!bugs)
    if ( buff.faeline_stomp_brm->up() )
      stagger += buff.faeline_stomp_brm->value();

  return std::min( stagger, 0.99 );
}

// monk_t::current_stagger_tick_dmg ==================================================

double monk_t::current_stagger_tick_dmg()
{
  double dmg = 0;
  if ( active_actions.stagger_self_damage )
    dmg = active_actions.stagger_self_damage->tick_amount();

  if ( buff.invoke_niuzao->up() )
    dmg *= 1 - buff.invoke_niuzao->value(); // Saved as 25%

  return dmg;
}

void monk_t::stagger_damage_changed( bool last_tick )
{
  buff_t* previous_buff = nullptr;
  for ( auto&& b : {buff.light_stagger, buff.moderate_stagger, buff.heavy_stagger} )
  {
    if ( b->check() )
    {
      previous_buff = b;
      break;
    }
  }
  sim->print_debug( "Previous stagger buff was {}.", previous_buff ? previous_buff->name() : "none" );

  buff_t* new_buff = nullptr;
  dot_t* dot       = nullptr;
  int niuzao       = 0;
  if ( active_actions.stagger_self_damage )
    dot = active_actions.stagger_self_damage->get_dot();
  if ( !last_tick && dot && dot->is_ticking() )  // fake dot not active on last tick
  {
    auto current_tick_dmg                = current_stagger_tick_dmg();
    auto current_tick_dmg_per_max_health = current_stagger_tick_dmg_percent();
    sim->print_debug( "Stagger dmg: {} ({}%):", current_tick_dmg, current_tick_dmg_per_max_health * 100.0 );
    if ( current_tick_dmg_per_max_health > 0.045 )
    {
      new_buff = buff.heavy_stagger;
      niuzao   = 3;
    }
    else if ( current_tick_dmg_per_max_health > 0.03 )
    {
      new_buff = buff.moderate_stagger;
      niuzao   = 2;
    }
    else if ( current_tick_dmg_per_max_health > 0.0 )
    {
      new_buff = buff.light_stagger;
      niuzao   = 1;
    }
  }
  sim->print_debug( "Stagger new buff is {}.", new_buff ? new_buff->name() : "none" );

  if ( previous_buff && previous_buff != new_buff )
  {
    previous_buff->expire();
  }
  if ( new_buff && previous_buff != new_buff )
  {
    new_buff->trigger();
  }
}

// monk_t::current_stagger_total ==================================================

double monk_t::current_stagger_amount_remains()
{
  double dmg = 0;
  if ( active_actions.stagger_self_damage )
    dmg = active_actions.stagger_self_damage->amount_remaining();
  return dmg;
}

// monk_t::current_stagger_amount_remains_to_total_percent ==========================================

double monk_t::current_stagger_amount_remains_to_total_percent()
{
  return active_actions.stagger_self_damage->amount_remaining_to_total();
}

// monk_t::current_stagger_dmg_percent ==================================================

double monk_t::current_stagger_tick_dmg_percent()
{
  return current_stagger_tick_dmg() / resources.max[ RESOURCE_HEALTH ];
}

double monk_t::current_stagger_amount_remains_percent()
{
  return current_stagger_amount_remains() / resources.max[ RESOURCE_HEALTH ];
}

// monk_t::current_stagger_dot_duration ==================================================

timespan_t monk_t::current_stagger_dot_remains()
{
  if ( active_actions.stagger_self_damage )
  {
    dot_t* dot = active_actions.stagger_self_damage->get_dot();

    return dot->remains();
  }

  return timespan_t::zero();
}

/**
 * Accumulated stagger tick damage of the last n ticks.
 */
double monk_t::calculate_last_stagger_tick_damage( int n ) const
{
  double amount = 0.0;

  assert( n > 0 );

  for ( size_t i = stagger_tick_damage.size(), j = n; i-- && j--; )
  {
    amount += stagger_tick_damage[ i ].value;
  }

  return amount;
}

void monk_t::trigger_empowered_tiger_lightning( action_state_t* s )
{
  if ( spec.invoke_xuen_2->ok() )
  {
    // Make sure Xuen is up and the action is not the Empowered Tiger Lightning itself (335913)
    // Touch of Karma (id = 124280) does not contribute to Empowered Tiger Lightning
    if ( buff.invoke_xuen->check() && s->result_total > 0 && s->action->id != 335913 && s->action->id != 124280 )
    {
      auto td = get_target_data( s-> target);

      auto previous_value = td->debuff.empowered_tiger_lightning->check() ? td->debuff.empowered_tiger_lightning->current_value : 0;
      auto new_value = previous_value + s->result_total;
      
      td->debuff.empowered_tiger_lightning->trigger( -1, new_value , -1, buff.invoke_xuen->remains());
    }
  }
}

void monk_t::trigger_bonedust_brew( action_state_t* s )
{
  if ( covenant.necrolord->ok() && s->result_total > 0 && s->action->id != 325217 && s->action->id != 325218 )
  {
    if ( auto td = find_target_data( s->target ) )
    {
      if ( td->debuff.bonedust_brew->up() && rng().roll( covenant.necrolord->proc_chance() ) )
      {
        double damage = s->result_total * covenant.necrolord->effectN( 1 ).percent();
        // Bone Marrow Hops DOES NOT work with SEF or pets
        //          if ( o()->conduit.bone_marrow_hops->ok() )
        //            damage *= 1 + o()->conduit.bone_marrow_hops.percent();

        active_actions.bonedust_brew_dmg->base_dd_min = damage;
        active_actions.bonedust_brew_dmg->base_dd_max = damage;
        active_actions.bonedust_brew_dmg->execute();
      }
    }
  }
}

// monk_t::create_expression ==================================================

std::unique_ptr<expr_t> monk_t::create_expression( util::string_view name_str )
{
  auto splits = util::string_split<util::string_view>( name_str, "." );
  if ( splits.size() == 2 && splits[ 0 ] == "stagger" )
  {
    struct stagger_threshold_expr_t : public expr_t
    {
      monk_t& player;
      double stagger_health_pct;
      stagger_threshold_expr_t( monk_t& p, double stagger_health_pct )
        : expr_t( "stagger_threshold_" + util::to_string( stagger_health_pct ) ),
          player( p ),
          stagger_health_pct( stagger_health_pct )
      {
      }

      double evaluate() override
      {
        return player.current_stagger_tick_dmg_percent() > stagger_health_pct;
      }
    };
    // WoW API has this as the 16th node from UnitDebuff
    struct stagger_amount_expr_t : public expr_t
    {
      monk_t& player;
      stagger_amount_expr_t( monk_t& p ) : expr_t( "stagger_amount" ), player( p )
      {
      }

      double evaluate() override
      {
        return player.current_stagger_tick_dmg();
      }
    };
    struct stagger_percent_expr_t : public expr_t
    {
      monk_t& player;
      stagger_percent_expr_t( monk_t& p ) : expr_t( "stagger_percent" ), player( p )
      {
      }

      double evaluate() override
      {
        return player.current_stagger_tick_dmg_percent() * 100;
      }
    };
    // This is the current stagger amount remaining compared to the total amount of the stagger dot
    struct stagger_amount_to_total_percent_expr_t : public expr_t
    {
      monk_t& player;
      stagger_amount_to_total_percent_expr_t( monk_t& p )
        : expr_t( "stagger_amount_to_total_percent" ), player( p )
      {
      }

      double evaluate() override
      {
        return player.current_stagger_amount_remains_to_total_percent() * 100;
      }
    };

    if ( splits[ 1 ] == "light" )
      return std::make_unique<stagger_threshold_expr_t>( *this, light_stagger_threshold );
    else if ( splits[ 1 ] == "moderate" )
      return std::make_unique<stagger_threshold_expr_t>( *this, moderate_stagger_threshold );
    else if ( splits[ 1 ] == "heavy" )
      return std::make_unique<stagger_threshold_expr_t>( *this, heavy_stagger_threshold );
    else if ( splits[ 1 ] == "amount" )
      return std::make_unique<stagger_amount_expr_t>( *this );
    else if ( splits[ 1 ] == "pct" )
      return std::make_unique<stagger_percent_expr_t>( *this );
    else if ( splits[ 1 ] == "amounttototalpct" )
      return std::make_unique<stagger_amount_to_total_percent_expr_t>( *this );
    else if ( splits[ 1 ] == "remains" )
    {
      return make_fn_expr( name_str, [this]() { return current_stagger_dot_remains(); } );
    }
    else if ( splits[ 1 ] == "amount_remains" )
    {
      return make_fn_expr( name_str, [this]() { return current_stagger_amount_remains(); } );
    }
    else if ( splits[ 1 ] == "ticking" )
    {
      return make_fn_expr( name_str, [this]() { return has_stagger(); } );
    }

    if ( util::str_in_str_ci( splits[ 1 ], "last_tick_damage_" ) )
    {
      auto parts = util::string_split<util::string_view>( splits[ 1 ], "_" );
      int n = util::to_int( parts.back() );

      // skip construction if the duration is nonsensical
      if ( n > 0 )
      {
        return make_fn_expr( name_str, [this, n] { return calculate_last_stagger_tick_damage( n ); } );
      }
      else
      {
        throw std::invalid_argument( fmt::format( "Non-positive number of last stagger ticks '{}'.", n ) );
      }
    }
  }

  else if ( splits.size() == 2 && splits[ 0 ] == "spinning_crane_kick" )
  {
    struct sck_stack_expr_t : public expr_t
    {
      monk_t& player;
      sck_stack_expr_t( monk_t& p ) : expr_t( "sck_count" ), player( p )
      {
      }

      double evaluate() override
      {
        return player.mark_of_the_crane_counter();
      }
    };

    if ( splits[ 1 ] == "count" )
      return std::make_unique<sck_stack_expr_t>( *this );
  }

  return base_t::create_expression( name_str );
}

void monk_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  action.apply_affecting_aura( passives.aura_monk );
  action.apply_affecting_aura( spec.brewmaster_monk );
  action.apply_affecting_aura( spec.windwalker_monk );
  action.apply_affecting_aura( spec.mistweaver_monk );

  if ( ( specialization() == MONK_BREWMASTER || specialization() == MONK_WINDWALKER ) &&
      main_hand_weapon.group() == weapon_e::WEAPON_1H )
        action.apply_affecting_aura( spec.two_hand_adjustment );

  // if ( action.data().affected_by( spec.mistweaver_monk->effectN( 6 ) ) )
  //   action.gcd_type = gcd_haste_type::HASTE;

  // if ( action.data().affected_by( spec.windwalker_monk->effectN( 14 ) ) )
  //   action.trigger_gcd += spec.windwalker_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds

  // // Reduce GCD from 1.5 sec to 1.005 sec (33%)
  // if ( action.data().affected_by_label( spec.brewmaster_monk->effectN( 16 ) ) )
  // {
  //   action.trigger_gcd *= ( 100.0 + spec.brewmaster_monk->effectN( 16 ).base_value() ) / 100.0;
  //   action.gcd_type = gcd_haste_type::NONE;
  // }
}

void monk_t::merge( player_t& other )
{
  base_t::merge( other );

  auto& other_monk = static_cast<const monk_t&>( other );

  sample_datas.stagger_effective_damage_timeline.merge( other_monk.sample_datas.stagger_effective_damage_timeline );
  sample_datas.stagger_damage_pct_timeline.merge( other_monk.sample_datas.stagger_damage_pct_timeline );
  sample_datas.stagger_pct_timeline.merge( other_monk.sample_datas.stagger_pct_timeline );
}

// monk_t::monk_report =================================================

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class monk_report_t : public player_report_extension_t
{
public:
  monk_report_t( monk_t& player ) : p( player )
  {
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    if ( p.specialization() == MONK_BREWMASTER )
    {
      double stagger_tick_dmg   = p.sample_datas.stagger_damage->mean();
      double purified_dmg       = p.sample_datas.purified_damage->mean();
      double staggering_strikes = p.sample_datas.staggering_strikes_cleared->mean();
      double stagger_total_dmg  = p.sample_datas.stagger_total_damage->mean();

      os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
         << "\t\t\t\t\t<h3 class=\"toggle open\">Stagger Analysis</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      highchart::time_series_t chart_stagger_damage( highchart::build_id( p, "stagger_damage" ), *p.sim );
      chart::generate_actor_timeline( chart_stagger_damage, p, "Stagger effective damage",
                                      color::resource_color( RESOURCE_HEALTH ),
                                      p.sample_datas.stagger_effective_damage_timeline );
      chart_stagger_damage.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart_stagger_damage.set( "chart.width", "575" );
      os << chart_stagger_damage.to_target_div();
      p.sim->add_chart_data( chart_stagger_damage );

      highchart::time_series_t chart_stagger_damage_pct( highchart::build_id( p, "stagger_damage_pct" ), *p.sim );
      chart::generate_actor_timeline( chart_stagger_damage_pct, p, "Stagger pool / health %",
                                      color::resource_color( RESOURCE_HEALTH ),
                                      p.sample_datas.stagger_damage_pct_timeline );
      chart_stagger_damage_pct.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart_stagger_damage_pct.set( "chart.width", "575" );
      os << chart_stagger_damage_pct.to_target_div();
      p.sim->add_chart_data( chart_stagger_damage_pct );

      highchart::time_series_t chart_stagger_pct( highchart::build_id( p, "stagger_pct" ), *p.sim );
      chart::generate_actor_timeline( chart_stagger_pct, p, "Stagger %", color::resource_color( RESOURCE_HEALTH ),
                                      p.sample_datas.stagger_pct_timeline );
      chart_stagger_pct.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart_stagger_pct.set( "chart.width", "575" );
      os << chart_stagger_pct.to_target_div();
      p.sim->add_chart_data( chart_stagger_pct );

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger base Unbuffed: {} Raid Buffed: {}</p>\n", p.stagger_base_value(),
                  p.sample_datas.buffed_stagger_base );

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger pct (player level) Unbuffed: {:.2f}% Raid Buffed: {:.2f}%</p>\n",
                  100.0 * p.stagger_pct( p.level() ), 100.0 * p.sample_datas.buffed_stagger_pct_player_level );

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger pct (target level) Unbuffed: {:.2f}% Raid Buffed: {:.2f}%</p>\n",
                  100.0 * p.stagger_pct( p.target->level() ), 100.0 * p.sample_datas.buffed_stagger_pct_target_level );

      fmt::print( os, "\t\t\t\t\t\t<p>Total Stagger damage added: {} / {:.2f}%</p>\n", stagger_total_dmg, 100.0 );
      fmt::print( os, "\t\t\t\t\t\t<p>Stagger cleared by Purifying Brew: {} / {:.2f}%</p>\n", purified_dmg,
                  ( purified_dmg / stagger_total_dmg ) * 100.0 );
      fmt::print( os, "\t\t\t\t\t\t<p>Stagger cleared by Staggering Strikes: {} / {:.2f}%</p>\n", staggering_strikes,
                  ( staggering_strikes / stagger_total_dmg ) * 100.0 );
      fmt::print( os, "\t\t\t\t\t\t<p>Stagger that directly damaged the player: {} / {:.2f}%</p>\n", stagger_tick_dmg,
                  ( stagger_tick_dmg / stagger_total_dmg ) * 100.0 );

      os << "\t\t\t\t\t\t<table class=\"sc\">\n"
         << "\t\t\t\t\t\t\t<tbody>\n"
         << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Damage Stats</th>\n"
         << "\t\t\t\t\t\t\t\t\t<th>Total</th>\n"
         //        << "\t\t\t\t\t\t\t\t\t<th>DTPS%</th>\n"
         << "\t\t\t\t\t\t\t\t\t<th>Execute</th>\n"
         << "\t\t\t\t\t\t\t\t</tr>\n";

      // Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124255\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/ability_rogue_cheatdeath.gif) "
            "0% "
            "50% no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << ( p.sample_datas.stagger_damage->mean() )
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << p.sample_datas.stagger_damage->count()
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Light Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124275\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra_green.gif) "
            "0% "
            "50% no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Light Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( p.sample_datas.light_stagger_damage->mean() ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << p.sample_datas.light_stagger_damage->count()
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Moderate Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124274\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra.gif) 0% 50% "
            "no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Moderate Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( p.sample_datas.moderate_stagger_damage->mean() ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << p.sample_datas.moderate_stagger_damage->count() << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Heavy Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124273\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra_red.gif) 0% "
            "50% no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Heavy Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( p.sample_datas.heavy_stagger_damage->mean() ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << p.sample_datas.heavy_stagger_damage->count()
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      os << "\t\t\t\t\t\t\t</tbody>\n"
         << "\t\t\t\t\t\t</table>\n";

      os << "\t\t\t\t\t\t</div>\n"
         << "\t\t\t\t\t</div>\n";
    }
    else
      (void)p;
  }

private:
  monk_t& p;
};

struct monk_module_t : public module_t
{
  monk_module_t() : module_t( MONK )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new monk_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new monk_report_t( *p ) );
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
    hotfix::register_effect( "Monk", "2020-11-21", "Manually set Direct Damage Windwalker Monk Two-Hand Adjustment by 2%", 872417 )
          .field( "base_value" )
          .operation( hotfix::HOTFIX_ADD)
          .modifier( 2 )
          .verification_value( 0 );
    hotfix::register_effect( "Monk", "2020-11-21", "Manually set Periodic Damage Windwalker Monk Two-Hand Adjustment by 2%", 872418 )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_ADD )
        .modifier( 2 )
        .verification_value( 0 );
    /*    hotfix::register_effect( "Monk", "2017-03-29", "Split Personality cooldown reduction increased to 5 seconds
       per
       rank (was 3 seconds per rank). [SEF]", 739336) .field( "base_value" ) .operation( hotfix::HOTFIX_SET ) .modifier(
       -5000 ) .verification_value( -3000 ); hotfix::register_effect( "Monk", "2017-03-30", "Split Personality cooldown
       reduction increased to 5 seconds per rank (was 3 seconds per rank). [Serentiy]", 739336) .field( "base_value" )
          .operation( hotfix::HOTFIX_SET )
          .modifier( -5000 )
          .verification_value( -3000 );
    */
  }

  void init( player_t* p ) const override
  {
    p->buffs.windwalking_movement_aura =
        make_buff( p, "windwalking_movement_aura", p->find_spell( 166646 ) )->add_invalidate( CACHE_RUN_SPEED );
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};
}  // UNNAMED NAMESPACE

const module_t* module_t::monk()
{
  static monk_module_t m;
  return &m;
}

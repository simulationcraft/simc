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
namespace actions::spells
{
struct stagger_self_damage_t;
}  // namespace actions::spells
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
  SEF_STRIKE_OF_THE_WINDLORD,
  SEF_STRIKE_OF_THE_WINDLORD_OH,
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

enum class bonedust_brew_zone_results_e
{
  NONE = 0,
  TP_FILL1,
  TP_FILL2,
  NO_CAP,
  CAP,
  BLUE = TP_FILL1,
  GREEN = TP_FILL2,
  RED = NO_CAP,
  PURPLE = CAP
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
    propagate_const<dot_t*> breath_of_fire;
    propagate_const<dot_t*> enveloping_mist;
    propagate_const<dot_t*> eye_of_the_tiger_damage;
    propagate_const<dot_t*> eye_of_the_tiger_heal;
    propagate_const<dot_t*> renewing_mist;
    propagate_const<dot_t*> rushing_jade_wind;
    propagate_const<dot_t*> soothing_mist;
    propagate_const<dot_t*> touch_of_death;
    propagate_const<dot_t*> touch_of_karma;
  } dots;

  struct
  {
    // Brewmaster
    propagate_const<buff_t*> exploding_keg;
    propagate_const<buff_t*> keg_smash;

    // Windwalker
    propagate_const<buff_t*> flying_serpent_kick;
    propagate_const<buff_t*> empowered_tiger_lightning;
    propagate_const<buff_t*> mark_of_the_crane;
    propagate_const<buff_t*> storm_earth_and_fire;
    propagate_const<buff_t*> touch_of_karma;

    // Covenant Abilities
    propagate_const<buff_t*> bonedust_brew;
    propagate_const<buff_t*> faeline_stomp;
    propagate_const<buff_t*> faeline_stomp_brm;
    propagate_const<buff_t*> fallen_monk_keg_smash;
    propagate_const<buff_t*> weapons_of_order;

    // Shadowland Legendaries
    propagate_const<buff_t*> call_to_arms_empowered_tiger_lightning;
    propagate_const<buff_t*> fae_exposure;
    propagate_const<buff_t*> keefers_skyreach;
    propagate_const<buff_t*> sinister_teaching_fallen_monk_keg_smash;
    propagate_const<buff_t*> skyreach_exhaustion;
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
    propagate_const<action_t*> breath_of_fire;
    propagate_const<heal_t*> celestial_fortune;
    propagate_const<heal_t*> gift_of_the_ox_trigger;
    propagate_const<heal_t*> gift_of_the_ox_expire;
    propagate_const<actions::spells::stagger_self_damage_t*> stagger_self_damage;

    // Windwalker
    propagate_const<action_t*> empowered_tiger_lightning;

    // Conduit
    propagate_const<heal_t*> evasive_stride;

    // Covenant
    propagate_const<action_t*> bonedust_brew_dmg;
    propagate_const<action_t*> bonedust_brew_heal;

    // Legendary
    propagate_const<action_t*> bountiful_brew;
    propagate_const<action_t*> call_to_arms_empowered_tiger_lightning;
  } active_actions;

  struct passive_actions_t
  {
    action_t* thunderfist;
  } passive_actions;

  std::vector<action_t*> combo_strike_actions;
  double spiritual_focus_count;
  double shuffle_count_secs;

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
    propagate_const<buff_t*> chi_torpedo;
    propagate_const<buff_t*> dampen_harm;
    propagate_const<buff_t*> diffuse_magic;
    propagate_const<buff_t*> rushing_jade_wind;
    propagate_const<buff_t*> spinning_crane_kick;

    // Brewmaster
    propagate_const<buff_t*> bladed_armor;
    propagate_const<buff_t*> blackout_combo;
    propagate_const<absorb_buff_t*> celestial_brew;
    propagate_const<buff_t*> celestial_flames;
    propagate_const<buff_t*> elusive_brawler;
    propagate_const<buff_t*> fortifying_brew;
    propagate_const<buff_t*> gift_of_the_ox;
    propagate_const<buff_t*> hit_scheme;
    propagate_const<buff_t*> invoke_niuzao;
    propagate_const<buff_t*> purified_chi;
    propagate_const<buff_t*> shuffle;
    propagate_const<buff_t*> zen_meditation;
    // niuzao r2 recent purifies fake buff
    propagate_const<buff_t*> recent_purifies;

    propagate_const<buff_t*> light_stagger;
    propagate_const<buff_t*> moderate_stagger;
    propagate_const<buff_t*> heavy_stagger;

    // Mistweaver
    propagate_const<absorb_buff_t*> life_cocoon;
    propagate_const<buff_t*> channeling_soothing_mist;
    propagate_const<buff_t*> invoke_chiji;
    propagate_const<buff_t*> invoke_chiji_evm;
    propagate_const<buff_t*> lifecycles_enveloping_mist;
    propagate_const<buff_t*> lifecycles_vivify;
    propagate_const<buff_t*> mana_tea;
    propagate_const<buff_t*> refreshing_jade_wind;
    propagate_const<buff_t*> teachings_of_the_monastery;
    propagate_const<buff_t*> touch_of_death_mw;
    propagate_const<buff_t*> thunder_focus_tea;
    propagate_const<buff_t*> uplifting_trance;

    // Windwalker
    propagate_const<buff_t*> bok_proc;
    propagate_const<buff_t*> combo_master;
    propagate_const<buff_t*> combo_strikes;
    propagate_const<buff_t*> dance_of_chiji;
    propagate_const<buff_t*> dance_of_chiji_hidden;  // Used for trigger DoCJ ticks
    propagate_const<buff_t*> dizzying_kicks;
    propagate_const<buff_t*> flying_serpent_kick_movement;
    propagate_const<buff_t*> hidden_masters_forbidden_touch;
    propagate_const<buff_t*> invoke_xuen;
    propagate_const<buff_t*> storm_earth_and_fire;
    propagate_const<buff_t*> serenity;
    propagate_const<buff_t*> thunderfist;
    propagate_const<buff_t*> touch_of_death_ww;
    propagate_const<buff_t*> touch_of_karma;
    propagate_const<buff_t*> transfer_the_power;
    propagate_const<buff_t*> windwalking_driver;
    propagate_const<buff_t*> whirling_dragon_punch;
    propagate_const<buff_t*> power_strikes;
    propagate_const<buff_t*> fury_of_xuen_stacks;
    propagate_const<stat_buff_t*> fury_of_xuen_haste;

    // Covenant Abilities
    propagate_const<buff_t*> bonedust_brew;
    propagate_const<buff_t*> bonedust_brew_hidden;
    propagate_const<buff_t*> weapons_of_order;
    propagate_const<buff_t*> weapons_of_order_ww;
    propagate_const<buff_t*> faeline_stomp;
    propagate_const<buff_t*> faeline_stomp_brm;
    propagate_const<buff_t*> faeline_stomp_reset;
    propagate_const<buff_t*> fallen_order;
    propagate_const<buff_t*> windwalking_venthyr;

    // Covenant Conduits
    propagate_const<absorb_buff_t*> fortifying_ingrediences;

    // Shadowland Legendary
    propagate_const<buff_t*> chi_energy;
    propagate_const<buff_t*> charred_passions;
    propagate_const<buff_t*> fae_exposure;
    propagate_const<buff_t*> invokers_delight;
    propagate_const<buff_t*> mighty_pour;
    propagate_const<buff_t*> pressure_point;
    propagate_const<buff_t*> the_emperors_capacitor;
    propagate_const<buff_t*> invoke_xuen_call_to_arms;

    // T28 Set Bonus
    propagate_const<buff_t*> keg_of_the_heavens;
    propagate_const<buff_t*> primordial_potential;
    propagate_const<buff_t*> primordial_power;
    propagate_const<buff_t*> primordial_power_hidden_gcd;
    propagate_const<buff_t*> primordial_power_hidden_channel;
  } buff;

public:
  struct gains_t
  {
    propagate_const<gain_t*> black_ox_brew_energy;
    propagate_const<gain_t*> chi_refund;
    propagate_const<gain_t*> bok_proc;
    propagate_const<gain_t*> chi_burst;
    propagate_const<gain_t*> crackling_jade_lightning;
    propagate_const<gain_t*> energy_refund;
    propagate_const<gain_t*> energizing_elixir_chi;
    propagate_const<gain_t*> energizing_elixir_energy;
    propagate_const<gain_t*> expel_harm;
    propagate_const<gain_t*> focus_of_xuen;
    propagate_const<gain_t*> fortuitous_spheres;
    propagate_const<gain_t*> gift_of_the_ox;
    propagate_const<gain_t*> healing_elixir;
    propagate_const<gain_t*> rushing_jade_wind_tick;
    propagate_const<gain_t*> serenity;
    propagate_const<gain_t*> spirit_of_the_crane;
    propagate_const<gain_t*> tiger_palm;
    propagate_const<gain_t*> touch_of_death_ww;
    propagate_const<gain_t*> power_strikes;
    propagate_const<gain_t*> open_palm_strikes;

    // Azerite Traits
    propagate_const<gain_t*> glory_of_the_dawn;
    propagate_const<gain_t*> memory_of_lucid_dreams;
    propagate_const<gain_t*> lucid_dreams;

    // Covenants
    propagate_const<gain_t*> bonedust_brew;
    propagate_const<gain_t*> weapons_of_order;
  } gain;

  struct procs_t
  {
    propagate_const<proc_t*> blackout_kick_cdr_with_woo;
    propagate_const<proc_t*> blackout_kick_cdr;
    propagate_const<proc_t*> blackout_kick_cdr_serenity_with_woo;
    propagate_const<proc_t*> blackout_kick_cdr_serenity;
    propagate_const<proc_t*> boiling_brew_healing_sphere;
    propagate_const<proc_t*> bonedust_brew_reduction;
    propagate_const<proc_t*> bountiful_brew_proc;
    propagate_const<proc_t*> glory_of_the_dawn;
    propagate_const<proc_t*> rsk_reset_totm;
    propagate_const<proc_t*> sinister_teaching_reduction;
    propagate_const<proc_t*> tumbling_technique_chi_torpedo;
    propagate_const<proc_t*> tumbling_technique_roll;
    propagate_const<proc_t*> xuens_battlegear_reduction;
  } proc;

  struct talents_t
  {
    // General Talents
    struct
    {
      // Row 1
      player_talent_t soothing_mist;
      player_talent_t rising_sun_kick;
      player_talent_t tigers_lust;
      // Row 2
      player_talent_t roll;
      player_talent_t calming_presence;
      player_talent_t paralysis;
      // Row 3
      player_talent_t tiger_tail_sweep;
      player_talent_t heavy_air;
      player_talent_t vivify;
      player_talent_t detox;
      player_talent_t disable;
      player_talent_t paralysis_rank_2;
      // Row 4
      player_talent_t grace_of_the_crane;
      player_talent_t vivacious_vivification;
      player_talent_t ferocity_of_xuen;
      // 8 Required
      // Row 5
      player_talent_t elusive_mists;
      player_talent_t transcendence;
      player_talent_t spear_hand_strike;
      player_talent_t fortifying_brew;
      // Row 6
      player_talent_t chi_wave;
      player_talent_t chi_burst;
      player_talent_t provoke;
      player_talent_t ring_of_peace;
      player_talent_t fast_feet;
      player_talent_t celerity;
      player_talent_t chi_torpedo;
      player_talent_t fortifying_brew_dodge_armor;
      player_talent_t fortifying_brew_cooldown;
      // Row 7
      player_talent_t roll_out;
      player_talent_t diffuse_magic;
      player_talent_t eye_of_the_tiger;
      player_talent_t dampen_harm;
      player_talent_t touch_of_death;
      player_talent_t expel_harm;
      // 20 Required
      // Row 8
      player_talent_t close_to_heart;
      player_talent_t escape_from_reality;
      player_talent_t windwalking;
      player_talent_t fatal_touch;
      player_talent_t generous_pour;
      // Row 9
      player_talent_t save_them_all;
      player_talent_t resonant_fists;
      player_talent_t bounce_back;
      // Row 10
      player_talent_t summon_jade_serpent_statue;
      player_talent_t summon_white_tiger_statue;
      player_talent_t summon_black_ox_statue;
    } general;

    // Brewmaster
    struct
    {
      // Row 1
      player_talent_t keg_smash;
      // Row 2
      player_talent_t stagger;
      // Row 3
      player_talent_t purifying_brew;
      player_talent_t shuffle;
      // Row 4
      player_talent_t hit_scheme;
      player_talent_t gift_of_the_ox;
      player_talent_t healing_elixir;
      player_talent_t quick_sip;
      player_talent_t rushing_jade_wind;
      player_talent_t special_delivery;
      // 8 Required
      // Row 5
      player_talent_t celestial_flames;
      player_talent_t celestial_brew;
      player_talent_t staggering_strikes;
      player_talent_t graceful_exit;
      player_talent_t zen_meditation;
      player_talent_t clash;
      // Row 6
      player_talent_t breath_of_fire;
      player_talent_t celestial_brew_rank_2;
      player_talent_t purifying_brew_rank_2;
      player_talent_t strength_of_spirit;
      player_talent_t gai_plins_imperial_brew;
      player_talent_t fundamental_observation;
      player_talent_t face_palm;
      // Row 7
      player_talent_t scalding_brew;
      player_talent_t salsalabims_strength;
      player_talent_t fortifying_brew_stagger;
      player_talent_t black_ox_brew;
      player_talent_t bob_and_weave;
      player_talent_t invoke_niuzao_the_black_ox;
      player_talent_t light_brewing;
      player_talent_t training_of_niuzao;
      player_talent_t shocking_brew;
      player_talent_t shadowboxing_treads;
      player_talent_t fluidity_of_motion;
      // 20 Required
      // Row 8
      player_talent_t dragonfire_brew;
      player_talent_t charred_passions;
      player_talent_t high_tolerance;
      player_talent_t walk_with_the_ox;
      player_talent_t elusive_footwork;
      player_talent_t anvil_and_stave;
      player_talent_t counterstrike;
      // Row 9
      player_talent_t bonedust_brew;
      player_talent_t invoke_niuzao_the_black_ox_rank_2;
      player_talent_t exploding_keg;
      player_talent_t blackout_combo;
      player_talent_t weapons_of_order;
      // Row 10
      player_talent_t bountiful_brew;
      player_talent_t attenuation;
      player_talent_t stormstouts_last_keg;
      player_talent_t call_to_arms;
      player_talent_t effusive_anima_accelerator;
    } brewmaster;

    // Mistweaver
    struct
    {
      // Row 1
      player_talent_t enveloping_mist;
      // Row 2
      player_talent_t essence_font;
      player_talent_t renewing_mist;
      // Row 3
      player_talent_t life_cocoon;
      player_talent_t thunder_focus_tea;
      player_talent_t invigorating_mists;
      // Row 4
      player_talent_t teachings_of_the_monastery;
      player_talent_t revival;
      player_talent_t restoral;
      player_talent_t healing_elixir;
      player_talent_t mastery_of_mist;
      // 8 Required
      // Row 5
      player_talent_t spirit_of_the_crane;
      player_talent_t mists_of_life;
      player_talent_t uplifting_spirits;
      player_talent_t zen_pulse;
      player_talent_t lifecycles;
      player_talent_t mana_tea;
      // Row 6
      player_talent_t nourishing_chi;
      player_talent_t overflowing_mists;
      player_talent_t invoke_yulon_the_jade_serpent;
      player_talent_t invoke_chi_ji_the_red_crane;
      player_talent_t zen_reverberation;
      player_talent_t accumulating_mist;
      player_talent_t song_of_chi_ji;
      player_talent_t rapid_diffusion;
      // Row 7
      player_talent_t calming_coalescence;
      player_talent_t yulons_whisper;
      player_talent_t mist_wrap;
      player_talent_t refreshing_jade_wind;
      player_talent_t enveloping_breath;
      player_talent_t dancing_mists;
      player_talent_t font_of_life;
      // 20 Required
      // Row 8
      player_talent_t faeline_stomp;
      player_talent_t ancient_teachings_of_the_monastery;
      player_talent_t clouded_focus;
      player_talent_t jade_bond;
      player_talent_t gift_of_the_celestials;
      player_talent_t focused_thunder;
      player_talent_t upwelling;
      player_talent_t bonedust_brew;
      // Row 9
      player_talent_t ancient_concordance;
      player_talent_t peaceful_mending;
      player_talent_t secret_infusion;
      player_talent_t misty_peaks;
      player_talent_t resplendent_mist;
      // Row 10
      player_talent_t awakened_faeline;
      player_talent_t restorative_proliferation;
      player_talent_t tea_of_plenty;
      player_talent_t unison;
      player_talent_t invokers_delight;
      player_talent_t tear_of_morning;
      player_talent_t rising_mist;
      player_talent_t bountiful_brew;
      player_talent_t attenuation;
    } mistweaver;

    // Windwalker
    struct {
      // Row 1
      player_talent_t fists_of_fury;
      // Row 2
      player_talent_t touch_of_karma;
      player_talent_t ascension;
      player_talent_t power_strikes;
      // Row 3
      player_talent_t feathers_of_a_hundred_flocks;
      player_talent_t touch_of_the_tiger;
      player_talent_t flying_serpent_kick;
      player_talent_t flashing_fists;
      player_talent_t open_palm_strikes;
      // Row 4
      player_talent_t mark_of_the_crane;
      player_talent_t gale_burst;
      player_talent_t glory_of_the_dawn;
      // 8 Required
      // Row 5
      player_talent_t shadowboxing_treads;
      player_talent_t hit_scheme;
      player_talent_t storm_earth_and_fire;
      player_talent_t serenity;
      player_talent_t meridian_strikes;
      player_talent_t jade_ignition;
      // Row 6
      player_talent_t dance_of_chiji;
      player_talent_t inner_peace;
      player_talent_t drinking_horn_cover;
      player_talent_t spiritual_focus;
      player_talent_t strike_of_the_windlord;
      // Row 7
      player_talent_t rushing_jade_wind;
      player_talent_t hidden_masters_forbidden_touch;
      player_talent_t invoke_xuen_the_white_tiger;
      player_talent_t teachings_of_the_monastery;
      player_talent_t thunderfist;
      // 20 Required
      // Row 8
      player_talent_t crane_vortex;
      player_talent_t xuens_bond;
      player_talent_t fury_of_xuen;
      player_talent_t empowered_tiger_lightning;
      player_talent_t rising_star;
      // Row 9
      player_talent_t boneduest_brew;
      player_talent_t fatal_flying_guillotine;
      // player_talent_t NYI
      player_talent_t xuens_battlegear;
      player_talent_t transfer_the_power;
      player_talent_t whirling_dragon_punch;
      player_talent_t faeline_stomp;
      // Row 10
      player_talent_t calculated_strikes;
      player_talent_t attenuation;
      player_talent_t keefers_skyreach;
      player_talent_t invokers_delight;
      player_talent_t way_of_the_fae;
      player_talent_t last_emperors_capacitor;
    } windwalker;
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
    propagate_const<cooldown_t*> blackout_kick;
    propagate_const<cooldown_t*> black_ox_brew;
    propagate_const<cooldown_t*> brewmaster_attack;
    propagate_const<cooldown_t*> breath_of_fire;
    propagate_const<cooldown_t*> chi_torpedo;
    propagate_const<cooldown_t*> celestial_brew;
    propagate_const<cooldown_t*> desperate_measure;
    propagate_const<cooldown_t*> drinking_horn_cover;
    propagate_const<cooldown_t*> expel_harm;
    propagate_const<cooldown_t*> fists_of_fury;
    propagate_const<cooldown_t*> flying_serpent_kick;
    propagate_const<cooldown_t*> fortifying_brew;
    propagate_const<cooldown_t*> healing_elixir;
    propagate_const<cooldown_t*> invoke_niuzao;
    propagate_const<cooldown_t*> invoke_xuen;
    propagate_const<cooldown_t*> invoke_yulon;
    propagate_const<cooldown_t*> keg_smash;
    propagate_const<cooldown_t*> purifying_brew;
    propagate_const<cooldown_t*> rising_sun_kick;
    propagate_const<cooldown_t*> refreshing_jade_wind;
    propagate_const<cooldown_t*> roll;
    propagate_const<cooldown_t*> rushing_jade_wind_brm;
    propagate_const<cooldown_t*> rushing_jade_wind_ww;
    propagate_const<cooldown_t*> storm_earth_and_fire;
    propagate_const<cooldown_t*> strike_of_the_windlord;
    propagate_const<cooldown_t*> thunder_focus_tea;
    propagate_const<cooldown_t*> touch_of_death;
    propagate_const<cooldown_t*> serenity;

    // Covenants
    propagate_const<cooldown_t*> weapons_of_order;
    propagate_const<cooldown_t*> bonedust_brew;
    propagate_const<cooldown_t*> faeline_stomp;
    propagate_const<cooldown_t*> fallen_order;

    // Legendary
    propagate_const<cooldown_t*> charred_passions;
    propagate_const<cooldown_t*> bountiful_brew;
    propagate_const<cooldown_t*> sinister_teachings;
  } cooldown;

  struct passives_t
  {
    // General
    const spell_data_t* aura_monk;
    const spell_data_t* chi_burst_damage;
    const spell_data_t* chi_burst_energize;
    const spell_data_t* chi_burst_heal;
    const spell_data_t* chi_wave_damage;
    const spell_data_t* chi_wave_heal;
    const spell_data_t* claw_of_the_white_tiger;
    const spell_data_t* fortifying_brew;
    const spell_data_t* healing_elixir;
    const spell_data_t* mystic_touch;

    // Brewmaster
    const spell_data_t* breath_of_fire_dot;
    const spell_data_t* celestial_fortune;
    const spell_data_t* elusive_brawler;
    const spell_data_t* face_palm;
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
    const spell_data_t* hidden_masters_forbidden_touch;
    const spell_data_t* mark_of_the_crane;
    const spell_data_t* thunderfist;
    const spell_data_t* touch_of_karma_tick;
    const spell_data_t* whirling_dragon_punch_tick;
    const spell_data_t* fury_of_xuen_stacking_buff;
    const spell_data_t* fury_of_xuen_haste_buff;

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
    const spell_data_t* fallen_monk_fallen_brew;
    const spell_data_t* fallen_monk_fists_of_fury;
    const spell_data_t* fallen_monk_fists_of_fury_tick;
    const spell_data_t* fallen_monk_keg_smash;
    const spell_data_t* fallen_monk_soothing_mist;
    const spell_data_t* fallen_monk_spec_duration;
    const spell_data_t* fallen_monk_spinning_crane_kick;
    const spell_data_t* fallen_monk_spinning_crane_kick_tick;
    const spell_data_t* fallen_monk_tiger_palm;
    const spell_data_t* fallen_monk_windwalking;

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
    const spell_data_t* call_to_arms_empowered_tiger_lightning;

    // Tier 28
    const spell_data_t* keg_of_the_heavens_buff;
    const spell_data_t* keg_of_the_heavens_heal;
    const spell_data_t* primordial_potential;
    const spell_data_t* primordial_power;
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
    spawner::pet_spawner_t<pet_t, monk_t> tiger_adept;
    spawner::pet_spawner_t<pet_t, monk_t> crane_adept;
    spawner::pet_spawner_t<pet_t, monk_t> ox_adept;
    spawner::pet_spawner_t<pet_t, monk_t> white_tiger_statue;
    spawner::pet_spawner_t<pet_t, monk_t> fury_of_xuen_tiger;
    spawner::pet_spawner_t<pet_t, monk_t> call_to_arms_xuen;
    spawner::pet_spawner_t<pet_t, monk_t> call_to_arms_niuzao;
    spawner::pet_spawner_t<pet_t, monk_t> sinister_teaching_tiger_adept;
    spawner::pet_spawner_t<pet_t, monk_t> sinister_teaching_crane_adept;
    spawner::pet_spawner_t<pet_t, monk_t> sinister_teaching_ox_adept;

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
    int motc_override;
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
  action_t* create_action( util::string_view name, util::string_view options ) override;
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
  const spelleffect_data_t* find_spelleffect( const spell_data_t* spell, effect_subtype_t subtype,
                                              int misc_value               = P_GENERIC,
                                              const spell_data_t* affected = spell_data_t::nil(),
                                              effect_type_t type           = E_APPLY_AURA );
  const spell_data_t* find_spell_override( const spell_data_t* base, const spell_data_t* passive );
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
  void trigger_empowered_tiger_lightning( action_state_t*, bool trigger_invoke_xuen, bool trigger_call_to_arms );
  void trigger_bonedust_brew( action_state_t* );
  player_t* next_mark_of_the_crane_target( action_state_t* );
  int mark_of_the_crane_counter();
  double sck_modifier();
  double clear_stagger();
  double partial_clear_stagger_pct( double );
  double partial_clear_stagger_amount( double );
  bool has_stagger();
  double calculate_last_stagger_tick_damage( int n ) const;

  // Storm Earth and Fire targeting logic
  std::vector<player_t*> create_storm_earth_and_fire_target_list() const;
  void summon_storm_earth_and_fire( timespan_t duration );
  void retarget_storm_earth_and_fire( pet_t* pet, std::vector<player_t*>& targets ) const;
  void retarget_storm_earth_and_fire_pets() const;

  void bonedust_brew_assessor( action_state_t* );
  void trigger_storm_earth_and_fire( const action_t* a, sef_ability_e sef_ability );
  void storm_earth_and_fire_fixate( player_t* target );
  bool storm_earth_and_fire_fixate_ready( player_t* target );
  // Trigger Windwalker Tier 28 4-piece Primordial Power Buff
  void storm_earth_and_fire_trigger_primordial_power(); 
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

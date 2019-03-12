
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

WINDWALKER:
- Add Cyclone Strike Counter as an expression

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
- Not Modeled:
-- Damage part of Expel Harm
-- Summon Black Ox Statue
-- Guard
-- Zen Meditation
*/
#include "simulationcraft.hpp"
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
}  // namespace actions
namespace pets
{
struct storm_earth_and_fire_pet_t;
}
struct monk_t;

enum combo_strikes_e
{
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
  CS_FIST_OF_THE_WHITE_TIGER,
  CS_ATTACK_MAX,

  // Spells begin here
  CS_CHI_BURST,
  CS_CHI_WAVE,
  CS_CRACKLING_JADE_LIGHTNING,
  CS_TOUCH_OF_DEATH,
  CS_FLYING_SERPENT_KICK,
  CS_SPELL_MAX,

  // Misc
  CS_SPELL_MIN  = CS_CHI_BURST,
  CS_ATTACK_MIN = CS_TIGER_PALM,
  CS_MAX,
};

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
    buff_t* mark_of_the_crane;
    buff_t* flying_serpent_kick;
    buff_t* keg_smash;
    buff_t* storm_earth_and_fire;
    buff_t* sunrise_technique;
    buff_t* touch_of_karma;
    buff_t* touch_of_death_amplifier;
  } debuff;

  monk_t& monk;
  monk_td_t( player_t* target, monk_t* p );
};

struct monk_t : public player_t
{
public:
  typedef player_t base_t;

  // Active
  action_t* windwalking_aura;

  struct
  {
    luxurious_sample_data_t* stagger_tick_damage;
    luxurious_sample_data_t* stagger_total_damage;
    luxurious_sample_data_t* light_stagger_total_damage;
    luxurious_sample_data_t* moderate_stagger_total_damage;
    luxurious_sample_data_t* heavy_stagger_total_damage;
    luxurious_sample_data_t* purified_damage;
    double buffed_stagger_base;
    double buffed_stagger_pct_player_level, buffed_stagger_pct_target_level;
  } sample_datas;

  struct active_actions_t
  {
    action_t* rushing_jade_wind;

    // Brewmaster
    heal_t* celestial_fortune;
    heal_t* gift_of_the_ox_trigger;
    heal_t* gift_of_the_ox_expire;
    actions::spells::stagger_self_damage_t* stagger_self_damage;

    // Windwalker
    action_t* sunrise_technique;

    // Azerite Traits
    action_t* fit_to_burst;
} active_actions;

  combo_strikes_e previous_combo_strike;
  double spiritual_focus_count;

  // Blurred time cooldown shenanigans
  std::vector<cooldown_t*> serenity_cooldowns;

  struct stagger_tick_entry_t
  {
    double value;
  };
  std::vector<stagger_tick_entry_t> stagger_tick_damage;  // record stagger tick damage for expression

  double gift_of_the_ox_proc_chance;
  // Containers for when to start the trigger for the 19 4-piece Windwalker Combo Master buff
  combo_strikes_e t19_melee_4_piece_container_1;
  combo_strikes_e t19_melee_4_piece_container_2;
  combo_strikes_e t19_melee_4_piece_container_3;

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
    stat_buff_t* tier19_oh_8pc;  // Tier 19 Order Hall 8-piece

    // Brewmaster
    buff_t* bladed_armor;
    buff_t* blackout_combo;
    buff_t* elusive_brawler;
    buff_t* fortifying_brew;
    buff_t* gift_of_the_ox;
    buff_t* ironskin_brew;
    buff_t* spitfire;
    buff_t* zen_meditation;
    buff_t* light_stagger;
    buff_t* moderate_stagger;
    buff_t* heavy_stagger;

    // Mistweaver
    absorb_buff_t* life_cocoon;
    buff_t* channeling_soothing_mist;
    buff_t* lifecycles_enveloping_mist;
    buff_t* lifecycles_vivify;
    buff_t* mana_tea;
    buff_t* refreshing_jade_wind;
    buff_t* teachings_of_the_monastery;
    buff_t* thunder_focus_tea;
    buff_t* uplifting_trance;

    // Windwalker
    buff_t* bok_proc;
    buff_t* combo_master;
    buff_t* combo_strikes;
    buff_t* dizzying_kicks;
    buff_t* flying_serpent_kick_movement;
    buff_t* hit_combo;
    buff_t* inner_stength;
    buff_t* pressure_point;
    buff_t* spinning_crane_kick;
    buff_t* storm_earth_and_fire;
    buff_t* serenity;
    buff_t* touch_of_karma;
    buff_t* windwalking_driver;

    // Legendaries
    buff_t* hidden_masters_forbidden_touch;
    buff_t* sephuzs_secret;
    buff_t* the_emperors_capacitor;

    // Azerite Trait
    buff_t* dance_of_chiji;
    buff_t* fit_to_burst;
    buff_t* fury_of_xuen_stacks;
    stat_buff_t* fury_of_xuen_haste;
    stat_buff_t* iron_fists;
    stat_buff_t* training_of_niuzao;
    buff_t* sunrise_technique;
    buff_t* swift_roundhouse;
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
  } gain;

  struct procs_t
  {
    proc_t* bok_proc;
  } proc;

  struct talents_t
  {
    // Tier 15 Talents
    const spell_data_t* eye_of_the_tiger;  // Brewmaster & Windwalker
    const spell_data_t* chi_wave;
    const spell_data_t* chi_burst;
    // Mistweaver
    const spell_data_t* zen_pulse;

    // Tier 30 Talents
    const spell_data_t* celerity;
    const spell_data_t* chi_torpedo;
    const spell_data_t* tigers_lust;

    // Tier 45 Talents
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

    // Tier 60 Talents
    const spell_data_t* tiger_tail_sweep;
    const spell_data_t* summon_black_ox_statue;  // Brewmaster
    const spell_data_t* song_of_chi_ji;          // Mistweaver
    const spell_data_t* ring_of_peace;
    // Windwalker
    const spell_data_t* good_karma;

    // Tier 75 Talents
    // Windwalker
    const spell_data_t* inner_strength;
    // Mistweaver & Windwalker
    const spell_data_t* diffuse_magic;
    // Brewmaster
    const spell_data_t* bob_and_weave;
    const spell_data_t* healing_elixir;
    const spell_data_t* dampen_harm;

    // Tier 90 Talents
    // Brewmaster
    const spell_data_t* special_delivery;
    const spell_data_t* invoke_niuzao;
    // Windwalker
    const spell_data_t* hit_combo;
    const spell_data_t* invoke_xuen;
    // Brewmaster & Windwalker
    const spell_data_t* rushing_jade_wind;
    // Mistweaver
    const spell_data_t* summon_jade_serpent_statue;
    const spell_data_t* refreshing_jade_wind;
    const spell_data_t* invoke_chi_ji;

    // Tier 100 Talents
    // Brewmaster
    const spell_data_t* high_tolerance;
    const spell_data_t* guard;
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
    const spell_data_t* crackling_jade_lightning;
    const spell_data_t* critical_strikes;
    const spell_data_t* effuse;
    const spell_data_t* effuse_2;
    const spell_data_t* leather_specialization;
    const spell_data_t* leg_sweep;
    const spell_data_t* mystic_touch;
    const spell_data_t* paralysis;
    const spell_data_t* provoke;
    const spell_data_t* rising_sun_kick;
    const spell_data_t* rising_sun_kick_2;
    const spell_data_t* roll;
    const spell_data_t* spinning_crane_kick;
    const spell_data_t* spear_hand_strike;
    const spell_data_t* tiger_palm;

    // Brewmaster
    const spell_data_t* blackout_strike;
    const spell_data_t* bladed_armor;
    const spell_data_t* breath_of_fire;
    const spell_data_t* brewmasters_balance;
    const spell_data_t* brewmaster_monk;
    const spell_data_t* celestial_fortune;
    const spell_data_t* expel_harm;
    const spell_data_t* fortifying_brew;
    const spell_data_t* gift_of_the_ox;
    const spell_data_t* ironskin_brew;
    const spell_data_t* keg_smash;
    const spell_data_t* purifying_brew;
    const spell_data_t* stagger;
    const spell_data_t* zen_meditation;

    // Mistweaver
    const spell_data_t* detox;
    const spell_data_t* enveloping_mist;
    const spell_data_t* envoloping_mist_2;
    const spell_data_t* essence_font;
    const spell_data_t* essence_font_2;
    const spell_data_t* life_cocoon;
    const spell_data_t* mistweaver_monk;
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
    const spell_data_t* blackout_kick_2;
    const spell_data_t* blackout_kick_3;
    const spell_data_t* combat_conditioning;  // Possibly will get removed
    const spell_data_t* combo_breaker;
    const spell_data_t* cyclone_strikes;
    const spell_data_t* disable;
    const spell_data_t* fists_of_fury;
    const spell_data_t* flying_serpent_kick;
    const spell_data_t* stance_of_the_fierce_tiger;
    const spell_data_t* storm_earth_and_fire;
    const spell_data_t* storm_earth_and_fire_2;
    const spell_data_t* touch_of_death;
    const spell_data_t* touch_of_death_amplifier;
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
    cooldown_t* blackout_strike;
    cooldown_t* black_ox_brew;
    cooldown_t* brewmaster_attack;
    cooldown_t* brewmaster_active_mitigation;
    cooldown_t* breath_of_fire;
    cooldown_t* desperate_measure;
    cooldown_t* fist_of_the_white_tiger;
    cooldown_t* fists_of_fury;
    cooldown_t* flying_serpent_kick;
    cooldown_t* fortifying_brew;
    cooldown_t* healing_elixir;
    cooldown_t* keg_smash;
    cooldown_t* rising_sun_kick;
    cooldown_t* refreshing_jade_wind;
    cooldown_t* rushing_jade_wind_brm;
    cooldown_t* rushing_jade_wind_ww;
    cooldown_t* storm_earth_and_fire;
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
    const spell_data_t* chi_wave_damage;
    const spell_data_t* chi_wave_heal;
    const spell_data_t* healing_elixir;
    const spell_data_t* mystic_touch;
    // Brewmaster
    const spell_data_t* breath_of_fire_dot;
    const spell_data_t* celestial_fortune;
    const spell_data_t* elusive_brawler;
    const spell_data_t* fortifying_brew;
    const spell_data_t* gift_of_the_ox_heal;
    const spell_data_t* ironskin_brew;
    const spell_data_t* keg_smash_buff;
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
    const spell_data_t* dizzying_kicks;
    const spell_data_t* fists_of_fury_tick;
    const spell_data_t* flying_serpent_kick_damage;
    const spell_data_t* focus_of_xuen;
    const spell_data_t* hit_combo;
    const spell_data_t* mark_of_the_crane;
    const spell_data_t* touch_of_karma_tick;
    const spell_data_t* whirling_dragon_punch_tick;

    // Legendaries
    const spell_data_t* the_emperors_capacitor;
    const spell_data_t* the_wind_blows;

    // Azerite Traits
    const spell_data_t* fury_of_xuen_stacking_buff;
    const spell_data_t* fury_of_xuen_haste_buff;
    const spell_data_t* glory_of_the_dawn_dmg;
  } passives;

  // RPPM objects
  struct rppms_t
  {
    // Azerite Traits
    real_ppm_t* boiling_brew;
    real_ppm_t* dance_of_chiji;
  } rppm;

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
    const spell_data_t* stormstouts_last_gasp;

    // Mistweaver
    const spell_data_t* eithas_lunar_glides_of_eramas;
    const spell_data_t* eye_of_collidus_the_warp_watcher;
    const spell_data_t* leggings_of_the_black_flame;
    const spell_data_t* ovyds_winter_wrap;
    const spell_data_t* petrichor_lagniappe;
    const spell_data_t* shelter_of_rin;
    const spell_data_t* unison_spaulders;

    // Windwalker
    const spell_data_t* cenedril_reflector_of_hatred;  // The amount of damage that Touch of Karma can redirect is
                                                       // increased by x% of your maximum health.
    const spell_data_t*
        drinking_horn_cover;  // The duration of Storm, Earth, and Fire is extended by x sec for every Chi you spend.
    const spell_data_t* hidden_masters_forbidden_touch;  // Touch of Death can be used a second time within 3 sec before
                                                         // its cooldown is triggered.
    const spell_data_t* katsuos_eclipse;                 // Reduce the cost of Fists of Fury by x Chi.
    const spell_data_t* march_of_the_legion;             // Increase the movement speed bonus of Windwalking by x%.
    const spell_data_t* the_emperors_capacitor;          // Chi spenders increase the damage of your next Crackling Jade
                                                         // Lightning by X%. Stacks up to Y times.
    const spell_data_t* the_wind_blows;
  } legendary;

  struct azerite_powers_t
  {
    // General
    // When you Leg Sweep, your targets suffer 50% reduced movement speed, and you gain 0 Speed for 6 sec.
    azerite_power_t sweep_the_leg;

    // Multiple
    // Rising Sun Kick has a 25% chance to trigger a second time, dealing 4950 Physical damage and restoring 1 Chi.
    azerite_power_t glory_of_the_dawn;
    // While Fortifying Brew is active, heal for 92 every second.
    azerite_power_t strength_of_spirit;
    // Attacking a target with Rising Sun Kick causes your damaging melee abilities to deal 49 additional Physical
    // damage to that target for 15 sec.
    azerite_power_t sunrise_technique;

    // Brewmaster
    // Breath of Fire deals 60 additional damage over its duration, and has a chance to summon a Healing Sphere each
    // time it deals damage.
    azerite_power_t boiling_brew;
    // Blackout Strike deals an additional 169 damage. Blackout Strike critical hits grant an additional 1 stack of
    // Elusive Brawler.
    azerite_power_t elusive_footwork;
    // Drinking Purifying Brew while at Heavy Stagger causes your next 3 melee abilities to heal you for 360.
    azerite_power_t fit_to_burst;
    // Expel Harm restores 60 health over 6 sec for each Healing Sphere consumed.
    azerite_power_t niuzaos_blessing;
    // When you Blackout Strike, your Stagger is reduced by 62.
    azerite_power_t staggering_strikes;
    // Gain up to 153 Mastery based on your current level of Stagger.
    azerite_power_t training_of_niuzao;

    // Mistweaver
    // When Life Cocoon expires, it releases a burst of mist that restores 2476 health split among the target and nearby
    // allies.
    azerite_power_t burst_of_life;
    // Activating Thunder Focus Tea causes you to exhale the breath of Yu'lon, healing up to 6 allies within 15 yards
    // for ((40% of Spell power) * 3) over 2 sec.
    azerite_power_t celestial_breath;
    // Your Essence Font's initial heal is increased by 26 and has a chance to reduce the cooldown of Thunder Focus Tea
    // by 1 sec.
    azerite_power_t font_of_life;
    // Vivify does an additional 173 healing when empowered by Thunder Focus Tea.
    azerite_power_t invigorating_brew;
    // When your Renewing Mists heals an ally, you have a chance to gain 128 Haste for 10 seconds.
    azerite_power_t misty_peaks;
    // Your Enveloping Mists heal the target for 62 each time they take damage.
    azerite_power_t overflowing_mists;
    // Your Vivify heals for an additional 85. Vivify critical heals reduce the cooldown of your Revival by 1.0 sec.
    azerite_power_t uplifting_spirits;

    // Windwalker
    // Fists of Fury grants you 0 Critical Strike for 6 sec when it hits at least 4 enemies.
    azerite_power_t iron_fists;
    // Your Combo Strikes grant you the Fury of Xuen, giving your next Fists of Fury a 3 % chance to grant 384 Haste 
    // and invoke Xuen, The White Tiger for 8 sec.Stacks up to 33 times.
    azerite_power_t fury_of_xuen;
    // When you Combo Strike, the cooldown of Touch of Death is reduced by 0.1 sec. Touch of Death deals an additional
    // 1540 damage.
    azerite_power_t meridian_strikes;
    // When Fists of Fury deals damage, it has a 5% chance to refund 1 Chi, and it deals 12 additional damage.
    azerite_power_t open_palm_strikes;
    // Tiger Palm's chance to make your next Blackout Kick free is increased to 10% and Tiger Palm deals an additional
    // 15 damage.
    azerite_power_t pressure_point;
    // Blackout Kick increases the damage of your next Rising Sun Kick by 74, stacking up to 2 times.
    azerite_power_t swift_roundhouse;
    // Spending Chi has a chance to make your next Spinning Crane Kick free and deal 1274 additional damage.
    azerite_power_t dance_of_chiji;
  } azerite;

  struct pets_t
  {
    pets::storm_earth_and_fire_pet_t* sef[ SEF_PET_MAX ];
//    spawner::pet_spawner_t<pets::fury_of_xuen_pet_t, monk_t> fury_of_xuen;
//    pets_t( monk_t* m );
  } pet;

/*  struct pets_t
  {
    spawner::pet_spawner_t<pets::fury_of_xuen_pet_t, monk_t> fury_of_xuen;
  } pet_spawner;
  */

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
      previous_combo_strike( CS_NONE ),
      spiritual_focus_count( 0 ),
      gift_of_the_ox_proc_chance(),
      t19_melee_4_piece_container_1( CS_NONE ),
      t19_melee_4_piece_container_2( CS_NONE ),
      t19_melee_4_piece_container_3( CS_NONE ),
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
      azerite( azerite_powers_t() ),
      pet( pets_t() ),
//      pet_spawner( pets_t() ),
      user_options( options_t() ),
      light_stagger_threshold( 0 ),
      moderate_stagger_threshold( 0.01666 ),  // Moderate transfers at 33.3% Stagger; 1.67% every 1/2 sec
      heavy_stagger_threshold( 0.03333 )      // Heavy transfers at 66.6% Stagger; 3.34% every 1/2 sec
  {
    // actives
    windwalking_aura = nullptr;

    cooldown.blackout_kick                = get_cooldown( "blackout_kick" );
    cooldown.blackout_strike              = get_cooldown( "blackout_stike" );
    cooldown.black_ox_brew                = get_cooldown( "black_ox_brew" );
    cooldown.brewmaster_attack            = get_cooldown( "brewmaster_attack" );
    cooldown.brewmaster_active_mitigation = get_cooldown( "brews" );
    cooldown.breath_of_fire               = get_cooldown( "breath_of_fire" );
    cooldown.fortifying_brew              = get_cooldown( "fortifying_brew" );
    cooldown.fist_of_the_white_tiger      = get_cooldown( "fist_of_the_white_tiger" );
    cooldown.fists_of_fury                = get_cooldown( "fists_of_fury" );
    cooldown.healing_elixir               = get_cooldown( "healing_elixir" );
    cooldown.keg_smash                    = get_cooldown( "keg_smash" );
    cooldown.rising_sun_kick              = get_cooldown( "rising_sun_kick" );
    cooldown.refreshing_jade_wind         = get_cooldown( "refreshing_jade_wind" );
    cooldown.rushing_jade_wind_brm        = get_cooldown( "rushing_jade_wind" );
    cooldown.rushing_jade_wind_ww         = get_cooldown( "rushing_jade_wind" );
    cooldown.storm_earth_and_fire         = get_cooldown( "storm_earth_and_fire" );
    cooldown.thunder_focus_tea            = get_cooldown( "thunder_focus_tea" );
    cooldown.touch_of_death               = get_cooldown( "touch_of_death" );
    cooldown.serenity                     = get_cooldown( "serenity" );

    regen_type = REGEN_DYNAMIC;
    if ( specialization() != MONK_MISTWEAVER )
    {
      regen_caches[ CACHE_HASTE ]        = true;
      regen_caches[ CACHE_ATTACK_HASTE ] = true;
    }
    user_options.initial_chi = 0;

    talent_points.register_validity_fn( [this]( const spell_data_t* spell ) {
      if ( find_item_by_id( 151643 ) != nullptr && level() < 120 )  // Soul of the Grandmaster Legendary
      {
        switch ( specialization() )
        {
          case MONK_BREWMASTER:
            return spell->id() == 196737;  // High Tolerance
            break;
          case MONK_MISTWEAVER:
            return spell->id() == 197900;  // Mist Wrap
            break;
          case MONK_WINDWALKER:
            return spell->id() == 196607;  // Eye of the Tiger
            break;
          default:
            return false;
            break;
        }
      }

      return false;
    } );
  }

  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  // player_t overrides
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual double composite_base_armor_multiplier() const override;
  virtual double composite_melee_crit_chance() const override;
  virtual double composite_melee_crit_chance_multiplier() const override;
  virtual double composite_spell_crit_chance() const override;
  virtual double composite_spell_crit_chance_multiplier() const override;
  virtual double resource_regen_per_second( resource_e ) const override;
  virtual double composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double composite_player_multiplier( school_e school ) const override;
  virtual double composite_player_heal_multiplier( const action_state_t* s ) const override;
  virtual double composite_melee_expertise( const weapon_t* weapon ) const override;
  virtual double composite_melee_attack_power() const override;
  virtual double composite_spell_haste() const override;
  virtual double composite_melee_haste() const override;
  virtual double composite_attack_power_multiplier() const override;
  virtual double composite_parry() const override;
  virtual double composite_dodge() const override;
  virtual double composite_mastery() const override;
  virtual double composite_mastery_rating() const override;
  virtual double composite_crit_avoidance() const override;
  virtual double composite_rating_multiplier( rating_e rating ) const override;
  virtual double temporary_movement_modifier() const override;
  virtual double passive_movement_modifier() const override;
  virtual pet_t* create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void create_pets() override;
  virtual void init_spells() override;
  virtual void init_base_stats() override;
  virtual void init_scaling() override;
  virtual void create_buffs() override;
  virtual void init_gains() override;
  virtual void init_procs() override;
  virtual void init_rng() override;
  virtual void init_resources( bool ) override;
  virtual void regen( timespan_t periodicity ) override;
  virtual void reset() override;
  virtual void interrupt() override;
  virtual double matching_gear_multiplier( attribute_e attr ) const override;
  virtual void recalculate_resource_max( resource_e ) override;
  virtual void create_options() override;
  virtual void copy_from( player_t* ) override;
  virtual resource_e primary_resource() const override;
  virtual role_e primary_role() const override;
  virtual stat_e convert_hybrid_stat( stat_e s ) const override;
  virtual void pre_analyze_hook() override;
  virtual void combat_begin() override;
  virtual void target_mitigation( school_e, dmg_e, action_state_t* ) override;
  virtual void assess_damage( school_e, dmg_e, action_state_t* s ) override;
  virtual void assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* s ) override;
  virtual void assess_heal( school_e, dmg_e, action_state_t* s ) override;
  virtual void invalidate_cache( cache_e ) override;
  virtual void init_action_list() override;
  void activate() override;
  virtual expr_t* create_expression( const std::string& name_str ) override;
  virtual monk_td_t* get_target_data( player_t* target ) const override
  {
    monk_td_t*& td = target_data[ target ];
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
  void stagger_damage_changed();
  double current_stagger_tick_dmg();
  double current_stagger_tick_dmg_percent();
  double current_stagger_amount_remains();
  timespan_t current_stagger_dot_remains();
  double stagger_base_value();
  double stagger_pct( int target_level );
  void trigger_celestial_fortune( action_state_t* );
  void trigger_sephuzs_secret( const action_state_t* state, spell_mechanic mechanic, double proc_chance = -1.0 );
  void trigger_mark_of_the_crane( action_state_t* );
  player_t* next_mark_of_the_crane_target( action_state_t* );
  int mark_of_the_crane_counter();
  double clear_stagger();
  double partial_clear_stagger_pct( double );
  double partial_clear_stagger_amount( double );
  bool has_stagger();
  double calculate_last_stagger_tick_damage( int n ) const;

  // Storm Earth and Fire targeting logic
  std::vector<player_t*> create_storm_earth_and_fire_target_list() const;
  void retarget_storm_earth_and_fire( pet_t* pet, std::vector<player_t*>& targets, size_t n_targets ) const;
  void retarget_storm_earth_and_fire_pets() const;
};

// ==========================================================================
// Monk Pets & Statues
// ==========================================================================

namespace pets
{
struct statue_t : public pet_t
{
  statue_t( sim_t* sim, monk_t* owner, const std::string& n, pet_e pt, bool guardian = false )
    : pet_t( sim, owner, n, pt, guardian )
  {
  }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }
};

struct jade_serpent_statue_t : public statue_t
{
  jade_serpent_statue_t( sim_t* sim, monk_t* owner, const std::string& n ) : statue_t( sim, owner, n, PET_NONE, true )
  {
  }
};

// ==========================================================================
// Storm Earth and Fire
// ==========================================================================

struct storm_earth_and_fire_pet_t : public pet_t
{
  struct sef_td_t : public actor_target_data_t
  {
    sef_td_t( player_t* target, storm_earth_and_fire_pet_t* source ) : actor_target_data_t( target, source )
    {
    }
  };

  // Storm, Earth, and Fire abilities begin =================================

  template <typename BASE>
  struct sef_action_base_t : public BASE
  {
    typedef BASE super_t;
    typedef sef_action_base_t<BASE> base_t;

    const action_t* source_action;

    sef_action_base_t( const std::string& n, storm_earth_and_fire_pet_t* p,
                       const spell_data_t* data = spell_data_t::nil() )
      : BASE( n, p, data ), source_action( nullptr )
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

    void init()
    {
      super_t::init();

      // Find source_action from the owner by matching the action name and
      // spell id with eachother. This basically means that by default, any
      // spell-data driven ability with 1:1 mapping of name/spell id will
      // always be chosen as the source action. In some cases this needs to be
      // overridden (see sef_zen_sphere_t for example).
      for ( size_t i = 0, end = o()->action_list.size(); i < end; i++ )
      {
        action_t* a = o()->action_list[ i ];

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

    sef_td_t* td( player_t* t ) const
    {
      return this->p()->get_target_data( t );
    }

    monk_t* o()
    {
      return debug_cast<monk_t*>( this->player->cast_pet()->owner );
    }

    const monk_t* o() const
    {
      return debug_cast<const monk_t*>( this->player->cast_pet()->owner );
    }

    const storm_earth_and_fire_pet_t* p() const
    {
      return debug_cast<storm_earth_and_fire_pet_t*>( this->player );
    }

    storm_earth_and_fire_pet_t* p()
    {
      return debug_cast<storm_earth_and_fire_pet_t*>( this->player );
    }

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
      return source_action->attack_direct_power_coefficient( state );
    }

    double attack_tick_power_coefficient( const action_state_t* state ) const
    {
      return source_action->attack_tick_power_coefficient( state );
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const
    {
      return source_action->composite_dot_duration( s );
    }

    timespan_t tick_time( const action_state_t* s ) const
    {
      return source_action->tick_time( s );
    }

    double composite_da_multiplier( const action_state_t* s ) const
    {
      return source_action->composite_da_multiplier( s );
    }

    double composite_ta_multiplier( const action_state_t* s ) const
    {
      return source_action->composite_ta_multiplier( s );
    }

    double composite_persistent_multiplier( const action_state_t* s ) const
    {
      return source_action->composite_persistent_multiplier( s );
    }

    double composite_versatility( const action_state_t* s ) const
    {
      return source_action->composite_versatility( s );
    }

    double composite_haste() const
    {
      return source_action->composite_haste();
    }

    timespan_t travel_time() const
    {
      return source_action->travel_time();
    }

    int n_targets() const
    {
      return source_action ? source_action->n_targets() : super_t::n_targets();
    }

    void execute()
    {
      // Target always follows the SEF clone's target, which is assigned during
      // summon time
      this->target = this->player->target;

      super_t::execute();
    }

    void snapshot_internal( action_state_t* state, uint32_t flags, dmg_e rt )
    {
      super_t::snapshot_internal( state, flags, rt );

      // Take out the Owner's Hit Combo Multiplier, but only if the ability is going to snapshot
      // multipliers in the first place.
/*      if ( o() -> talent.hit_combo -> ok() )
      {
        if ( rt == DMG_DIRECT && ( flags & STATE_MUL_DA ) )
        {
          state -> da_multiplier /= ( 1 + o() -> buff.hit_combo -> stack_value() );
          state -> da_multiplier *= 1 + p() -> buff.hit_combo_sef -> stack_value();
        }

        if ( rt == DMG_OVER_TIME && ( flags & STATE_MUL_TA ) )
        {
          state -> ta_multiplier /= ( 1 + o() -> buff.hit_combo -> stack_value() );
          state -> ta_multiplier *= 1 + p() -> buff.hit_combo_sef -> stack_value();
        }
      }
      */
    }
  };

  struct sef_melee_attack_t : public sef_action_base_t<melee_attack_t>
  {
    bool main_hand, off_hand;

    sef_melee_attack_t( const std::string& n, storm_earth_and_fire_pet_t* p,
                        const spell_data_t* data = spell_data_t::nil(), weapon_t* w = nullptr )
      : base_t( n, p, data ),
        // For special attacks, the SEF pets always use the owner's weapons.
        main_hand( !w ? true : false ),
        off_hand( !w ? true : false )
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
      if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
      {
        return DMG_DIRECT;
      }
      else
      {
        return base_t::amount_type( state, periodic );
      }
    }

/*    double action_multiplier() const override
    {
      double am = base_t::action_multiplier();

      if (p()->buff.hit_combo_sef->up())
      {
        if (base_t::data().affected_by(o()->passives.hit_combo->effectN(1)))
        {
          // Remove owner's Hit Combo
          am /= 1 + o()->buff.hit_combo->stack_value();
          // .. aand add Pet's Hit Combo
          am *= 1 + p()->buff.hit_combo_sef->stack_value();
        }
      }
      return am;
    }
    */
  };

  struct sef_spell_t : public sef_action_base_t<spell_t>
  {
    sef_spell_t( const std::string& n, storm_earth_and_fire_pet_t* p, const spell_data_t* data = spell_data_t::nil() )
      : base_t( n, p, data )
    {
    }

/*    double action_multiplier() const override
    {
      double am = base_t::action_multiplier();

      if (p()->buff.hit_combo_sef->up())
      {
        if (base_t::data().affected_by(o()->passives.hit_combo->effectN(1)))
        {
          // Remove owner's Hit Combo
          am /= 1 + o()->buff.hit_combo->stack_value();
          // .. aand add Pet's Hit Combo
          am *= 1 + p()->buff.hit_combo_sef->stack_value();
        }
      }
      return am;
    }
    */
  };

  // Auto attack ============================================================

  struct melee_t : public sef_melee_attack_t
  {
    melee_t( const std::string& n, storm_earth_and_fire_pet_t* player, weapon_t* w )
      : sef_melee_attack_t( n, player, spell_data_t::nil(), w )
    {
      background = repeating = may_crit = may_glance = true;
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
        sim->errorf( "%s has no auto_attack in APL, Storm, Earth, and Fire pets cannot auto-attack.", o()->name() );
      }
    }

/*    double action_multiplier() const override
    {
      double am = sef_melee_attack_t::action_multiplier();

      am *= 1.0 + o()->spec.storm_earth_and_fire->effectN( 1 ).percent();

      if (p()->buff.hit_combo_sef->up())
      {
        // Remove owner's Hit Combo
        am /= 1 + o()->buff.hit_combo->stack_value();
        // .. aand add Pet's Hit Combo
        am *= 1 + p()->buff.hit_combo_sef->stack_value();
      }

      return am;
    }
    */

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
      if ( time_to_execute > timespan_t::zero() && player->executing )
      {
        if ( sim->debug )
        {
          sim->out_debug.printf( "%s Executing '%s' during melee (%s).", player->name(), player->executing->name(),
                                 util::slot_type_string( weapon->slot ) );
        }

        schedule_execute();
      }
      else
      {
        sef_melee_attack_t::execute();
      }
    }
  };

  struct auto_attack_t : public attack_t
  {
    auto_attack_t( storm_earth_and_fire_pet_t* player, const std::string& options_str )
      : attack_t( "auto_attack", player, spell_data_t::nil() )
    {
      parse_options( options_str );

      trigger_gcd = timespan_t::zero();

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

    virtual bool ready() override
    {
      if ( player->is_moving() )
        return false;

      return ( player->main_hand_attack->execute_event == nullptr );  // not swinging
    }

    virtual void execute() override
    {
      player->main_hand_attack->schedule_execute();

      if ( player->off_hand_attack )
        player->off_hand_attack->schedule_execute();
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

    double bonus_da( const action_state_t* s ) const override
    {
      double b = sef_melee_attack_t::bonus_da( s );

      if ( o()->azerite.pressure_point.ok() )
          b += o()->azerite.pressure_point.value();

      return b;
    }

  void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
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

    double composite_persistent_multiplier( const action_state_t* action_state ) const override
    {
      double pm = sef_melee_attack_t::composite_persistent_multiplier( action_state );

      if ( o()->sets->has_set_bonus( MONK_WINDWALKER, T21, B4 ) && p()->buff.bok_proc_sef->up() )
        pm *= 1 + o()->sets->set( MONK_WINDWALKER, T21, B4 )->effectN( 1 ).percent();

      return pm;
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
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

    double bonus_da( const action_state_t* s ) const override
    {
      double b = sef_melee_attack_t::bonus_da( s );

      if ( o()->buff.swift_roundhouse->up() )
        b += o()->buff.swift_roundhouse->stack_value();

      return b;
    }

    virtual double composite_crit_chance() const override
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
        if ( o()->spec.combat_conditioning )
          state->target->debuffs.mortal_wounds->trigger();

        if ( o()->spec.spinning_crane_kick )
          o()->trigger_mark_of_the_crane( state );
      }
    }
  };

  struct sef_rising_sun_kick_t : public sef_melee_attack_t
  {
    sef_rising_sun_kick_dmg_t* trigger;
    sef_rising_sun_kick_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "rising_sun_kick", player, player->o()->spec.rising_sun_kick ),
        trigger( new sef_rising_sun_kick_dmg_t( player ) )
    {
    }

    virtual void execute() override
    {
      sef_melee_attack_t::execute();

      trigger->execute();
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
    }

    double bonus_da( const action_state_t* s ) const override
    {
      double b = sef_tick_action_t::bonus_da( s );

      if ( o()->azerite.open_palm_strikes.ok() )
        b += o()->azerite.open_palm_strikes.value( 4 );

      return b;
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
      aoe = -1;
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

      energize_type = ENERGIZE_NONE;
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
      hasted_ticks          = false;
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

  storm_earth_and_fire_pet_t( const std::string& name, sim_t* sim, monk_t* owner, bool dual_wield, weapon_e weapon_type )
    : pet_t( sim, owner, name, true ),
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
    if ( !td )
    {
      td = new sef_td_t( target, const_cast<storm_earth_and_fire_pet_t*>( this ) );
    }
    return td;
  }

  void init_spells() override
  {
    pet_t::init_spells();

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

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    pet_t::summon( duration );

    o()->buff.storm_earth_and_fire->trigger();

    if ( o()->buff.bok_proc->up() )
      buff.bok_proc_sef->trigger( 1, buff_t::DEFAULT_VALUE(), 1, o()->buff.bok_proc->remains() );

    if ( o() -> buff.hit_combo -> up() )
      buff.hit_combo_sef -> trigger( o() -> buff.hit_combo -> stack() );

    if ( o()->buff.rushing_jade_wind->up() )
      buff.rushing_jade_wind_sef->trigger( 1, buff_t::DEFAULT_VALUE(), 1, o()->buff.rushing_jade_wind->remains() );
  }

  void dismiss( bool expired = false ) override
  {
    pet_t::dismiss( expired );

    o()->buff.storm_earth_and_fire->decrement();
  }

  void create_buffs() override
  {
    pet_t::create_buffs();

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
                                     ->set_tick_callback( [this]( buff_t* d, int, const timespan_t& ) {
                                       if ( o()->buff.rushing_jade_wind->up() )
                                         active_actions.rushing_jade_wind_sef->execute();
                                       else
                                         d->expire( timespan_t::from_millis( 1 ) );
                                     } );

    buff.hit_combo_sef = make_buff( this, "hit_combo_sef", o() -> passives.hit_combo )
                         -> set_default_value( o() -> passives.hit_combo -> effectN( 1 ).percent() )
                         -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void trigger_attack( sef_ability_e ability, const action_t* source_action )
  {
    if ( ability >= SEF_SPELL_MIN )
    {
      size_t spell = static_cast<size_t>( ability - SEF_SPELL_MIN );
      assert( spells[ spell ] );

      if ( o() -> buff.combo_strikes -> up() && o() -> talent.hit_combo -> ok() )
        buff.hit_combo_sef -> trigger();

      spells[ spell ]->source_action = source_action;
      spells[ spell ]->execute();
    }
    else
    {
      assert( attacks[ ability ] );

      if ( o() -> buff.combo_strikes -> up() && o() -> talent.hit_combo -> ok() )
        buff.hit_combo_sef -> trigger();

      attacks[ ability ]->source_action = source_action;
      attacks[ ability ]->execute();
    }
  }
};

// ==========================================================================
// Xuen Pet
// ==========================================================================
struct xuen_pet_t : public pet_t
{
private:
  struct melee_t : public melee_attack_t
  {
    melee_t( const std::string& n, xuen_pet_t* player ) : melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = repeating = may_crit = may_glance = true;
      school                                         = SCHOOL_PHYSICAL;
      weapon_multiplier                              = 1.0;
      // Use damage numbers from the level-scaled weapon
      weapon            = &( player->main_hand_weapon );
      base_execute_time = weapon->swing_time;
      trigger_gcd       = timespan_t::zero();
      special           = false;
    }

    void execute() override
    {
      if ( time_to_execute > timespan_t::zero() && player->executing )
      {
        if ( sim->debug )
          sim->out_debug.printf( "Executing '%s' during melee (%s).", player->executing->name(),
                                 util::slot_type_string( weapon->slot ) );
        schedule_execute();
      }
      else
        attack_t::execute();
    }
  };

  struct crackling_tiger_lightning_tick_t : public spell_t
  {
    crackling_tiger_lightning_tick_t( xuen_pet_t* p )
      : spell_t( "crackling_tiger_lightning_tick", p, p->o()->passives.crackling_tiger_lightning )
    {
      aoe  = 3;
      dual = direct_tick = background = may_crit = may_miss = true;
      //range                                                 = radius;
      //radius                                                = 0;
    }
  };

  struct crackling_tiger_lightning_t : public spell_t
  {
    crackling_tiger_lightning_t( xuen_pet_t* p, const std::string& options_str )
      : spell_t( "crackling_tiger_lightning", p, p->o()->passives.crackling_tiger_lightning )
    {
      parse_options( options_str );

      // for future compatibility, we may want to grab Xuen and our tick spell and build this data from those (Xuen
      // summon duration, for example)
      dot_duration = p->o()->talent.invoke_xuen->duration();
      hasted_ticks = true;
      may_miss = false;
      dynamic_tick_action = true;  // trigger tick when t == 0
      base_tick_time =
          p->o()->passives.crackling_tiger_lightning_driver->effectN( 1 ).period();  // trigger a tick every second
      cooldown->duration      = p->o()->talent.invoke_xuen->cooldown();              // we're done after 45 seconds
      attack_power_mod.direct = 0.0;
      attack_power_mod.tick   = 0.0;

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }
  };

  struct auto_attack_t : public attack_t
  {
    auto_attack_t( xuen_pet_t* player, const std::string& options_str )
      : attack_t( "auto_attack", player, spell_data_t::nil() )
    {
      parse_options( options_str );

      player->main_hand_attack                    = new melee_t( "melee_main_hand", player );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual bool ready() override
    {
      if ( player->is_moving() )
        return false;

      return ( player->main_hand_attack->execute_event == nullptr );  // not swinging
    }

    virtual void execute() override
    {
      player->main_hand_attack->schedule_execute();

      if ( player->off_hand_attack )
        player->off_hand_attack->schedule_execute();
    }
  };

public:
  xuen_pet_t( sim_t* sim, monk_t* owner, std::string name ) : pet_t( sim, owner, name, true )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap      = 1.00;
  }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }

  const monk_t* o() const
  {
    return static_cast<monk_t*>( owner );
  }

  virtual void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "crackling_tiger_lightning" )
      return new crackling_tiger_lightning_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Fury of Xuen Pet
// ==========================================================================
struct fury_of_xuen_pet_t : public pet_t
{
private:
  struct melee_t : public melee_attack_t
  {
    melee_t( const std::string& n, fury_of_xuen_pet_t* player ) : melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = repeating = may_crit = may_glance = true;
      school                                         = SCHOOL_PHYSICAL;
      weapon_multiplier                              = 1.0;
      // Use damage numbers from the level-scaled weapon
      weapon            = &( player->main_hand_weapon );
      base_execute_time = weapon->swing_time;
      trigger_gcd       = timespan_t::zero();
      special           = false;
    }

    void execute() override
    {
      if ( time_to_execute > timespan_t::zero() && player->executing )
      {
        if ( sim->debug )
          sim->out_debug.printf( "Executing '%s' during melee (%s).", player->executing->name(),
                                 util::slot_type_string( weapon->slot ) );
        schedule_execute();
      }
      else
        attack_t::execute();
    }
  };

  struct crackling_tiger_lightning_tick_t : public spell_t
  {
    crackling_tiger_lightning_tick_t( fury_of_xuen_pet_t* p )
      : spell_t( "crackling_tiger_lightning_tick", p, p->o()->passives.crackling_tiger_lightning )
    {
      aoe  = 3;
      dual = direct_tick = background = may_crit = may_miss = true;
      //range                                                 = radius;
      //radius                                                = 0;
    }
  };

  struct crackling_tiger_lightning_t : public spell_t
  {
    crackling_tiger_lightning_t( fury_of_xuen_pet_t* p, const std::string& options_str )
      : spell_t( "crackling_tiger_lightning", p, p->o()->passives.crackling_tiger_lightning )
    {
      parse_options( options_str );

      // for future compatibility, we may want to grab Xuen and our tick spell and build this data from those (Xuen
      // summon duration, for example)
      dot_duration = p->o()->buff.fury_of_xuen_haste->buff_duration;
      hasted_ticks = true;
      may_miss = false;
      dynamic_tick_action = true;  // trigger tick when t == 0
      base_tick_time =
          p->o()->passives.crackling_tiger_lightning_driver->effectN( 1 ).period();  // trigger a tick every second
      cooldown->duration      = p->o()->buff.fury_of_xuen_haste->buff_duration + timespan_t::from_seconds( 2 );      // we're done after 9 seconds
      attack_power_mod.direct = 0.0;
      attack_power_mod.tick   = 0.0;

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }
  };

  struct auto_attack_t : public attack_t
  {
    auto_attack_t( fury_of_xuen_pet_t* player, const std::string& options_str )
      : attack_t( "auto_attack", player, spell_data_t::nil() )
    {
      parse_options( options_str );

      player->main_hand_attack                    = new melee_t( "melee_main_hand", player );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual bool ready() override
    {
      if ( player->is_moving() )
        return false;

      return ( player->main_hand_attack->execute_event == nullptr );  // not swinging
    }

    virtual void execute() override
    {
      player->main_hand_attack->schedule_execute();

      if ( player->off_hand_attack )
        player->off_hand_attack->schedule_execute();
    }
  };

public:
  fury_of_xuen_pet_t( sim_t* sim, monk_t* owner, std::string name ) : pet_t( sim, owner, name, true )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap      = 1.00;
  }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }

  const monk_t* o() const
  {
    return static_cast<monk_t*>( owner );
  }

  virtual void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
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
struct niuzao_pet_t : public pet_t
{
private:
  struct melee_t : public melee_attack_t
  {
    melee_t( const std::string& n, niuzao_pet_t* player ) : melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = repeating = may_crit = may_glance = true;
      school                                         = SCHOOL_PHYSICAL;
      weapon_multiplier                              = 1.0;
      // Use damage numbers from the level-scaled weapon
      weapon            = &( player->main_hand_weapon );
      base_execute_time = weapon->swing_time;
      trigger_gcd       = timespan_t::zero();
      special           = false;
    }

    void execute() override
    {
      if ( time_to_execute > timespan_t::zero() && player->executing )
      {
        if ( sim->debug )
          sim->out_debug.printf( "Executing '%s' during melee (%s).", player->executing->name(),
                                 util::slot_type_string( weapon->slot ) );
        schedule_execute();
      }
      else
        attack_t::execute();
    }
  };

  struct stomp_tick_t : public melee_attack_t
  {
    stomp_tick_t( niuzao_pet_t* p ) : melee_attack_t( "stomp_tick", p, p->o()->passives.stomp )
    {
      aoe  = -1;
      dual = direct_tick = background = may_crit = may_miss = true;
      range                                                 = radius;
      radius                                                = 0;
      cooldown->duration                                    = timespan_t::zero();
    }
  };

  struct stomp_t : public spell_t
  {
    stomp_t( niuzao_pet_t* p, const std::string& options_str ) : spell_t( "stomp", p, p->o()->passives.stomp )
    {
      parse_options( options_str );

      // for future compatibility, we may want to grab Niuzao and our tick spell and build this data from those (Niuzao
      // summon duration, for example)
      dot_duration = p->o()->talent.invoke_niuzao->duration();
      hasted_ticks = may_miss = false;
      tick_zero = dynamic_tick_action = true;                                      // trigger tick when t == 0
      base_tick_time                  = p->o()->passives.stomp->cooldown();        // trigger a tick every second
      cooldown->duration              = p->o()->talent.invoke_niuzao->duration();  // we're done after 45 seconds
      attack_power_mod.direct         = 0.0;
      attack_power_mod.tick           = 0.0;

      tick_action = new stomp_tick_t( p );
    }
  };

  struct auto_attack_t : public attack_t
  {
    auto_attack_t( niuzao_pet_t* player, const std::string& options_str )
      : attack_t( "auto_attack", player, spell_data_t::nil() )
    {
      parse_options( options_str );

      player->main_hand_attack                    = new melee_t( "melee_main_hand", player );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual bool ready() override
    {
      if ( player->is_moving() )
        return false;

      return ( player->main_hand_attack->execute_event == nullptr );  // not swinging
    }

    virtual void execute() override
    {
      player->main_hand_attack->schedule_execute();

      if ( player->off_hand_attack )
        player->off_hand_attack->schedule_execute();
    }
  };

public:
  niuzao_pet_t( sim_t* sim, monk_t* owner ) : pet_t( sim, owner, "niuzao_the_black_ox", true )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_ap      = 1;
  }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    monk_t* o = static_cast<monk_t*>( owner );
    m *= 1 + o->spec.brewmaster_monk->effectN( 3 ).percent();

    return m;
  }

  virtual void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/stomp";

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "stomp" )
      return new stomp_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
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

  // Affect flags for various dynamic effects
  struct
  {
    struct
    {
      bool spell_da1;
      bool spell_da2;
      bool spell_da3;
      bool spell_da4;
      bool spell_ta1;
      bool spell_ta2;
    } brewmaster;

     struct
    {
      bool spell_da1;
      bool spell_ta1;
    } mistweaver;

    struct
    {
      bool spell_da1;
      bool spell_da2;
      bool spell_da3;
      bool spell_da4;
      bool spell_da5;
      bool spell_da6;
      bool spell_da7;
      bool spell_ta1;
      bool spell_ta2;
      bool spell_ta3;
      bool spell_ta4;
    } windwalker;

    bool hasted_cooldown;
    bool hasted_gcd;
    bool serenity;
    bool sunrise_technique;
  } affected_by;

private:
  std::array<resource_e, MONK_MISTWEAVER + 1> _resource_by_stance;
  typedef Base ab;  // action base, eg. spell_t
public:
  typedef monk_action_t base_t;

  monk_action_t( const std::string& n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      sef_ability( SEF_NONE ),
      ww_mastery( false ),

      affected_by()
  {
    init_affected_by();
    ab::may_crit = true;
    range::fill( _resource_by_stance, RESOURCE_MAX );
    ab::trigger_gcd = timespan_t::from_seconds( 1.5 );

    switch ( player->specialization() )
    {
      case MONK_BREWMASTER:
      {
        if ( affected_by.brewmaster.spell_da1 )
          ab::base_dd_multiplier *= 1.0 + player->spec.brewmaster_monk->effectN( 1 ).percent();
        if ( affected_by.brewmaster.spell_da2 ) // RJW uses direct hit for it's ticks
          ab::base_dd_multiplier *= 1.0 + player->spec.brewmaster_monk->effectN( 6 ).percent();
        if ( affected_by.brewmaster.spell_da3 )
          ab::base_dd_multiplier *= 1.0 + player->spec.brewmaster_monk->effectN( 7 ).percent();
        if ( affected_by.brewmaster.spell_da4 )
          ab::base_dd_multiplier *= 1.0 + player->spec.brewmaster_monk->effectN( 18 ).percent();

        if ( affected_by.brewmaster.spell_ta1 )
          ab::base_td_multiplier *= 1.0 + player->spec.brewmaster_monk->effectN( 2 ).percent();
        if ( affected_by.brewmaster.spell_ta2 )
          ab::base_td_multiplier *= 1.0 + player->spec.brewmaster_monk->effectN( 8 ).percent();

        // Reduce GCD from 1.5 sec to 1 sec
        if ( ab::data().affected_by( player->spec.brewmaster_monk->effectN( 14 ) ) )
          ab::trigger_gcd += player->spec.brewmaster_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds

        // Brewmasters no longer use Chi so need to zero out chi cost
        if ( ab::data().affected_by( player->spec.brewmaster_monk->effectN( 16 ) ) )
          ab::base_costs[ RESOURCE_CHI ] *=
              1 + player->spec.brewmaster_monk->effectN( 16 ).percent();  // -100% for Brewmasters
        break;
      }
      case MONK_MISTWEAVER:
      {
        if ( affected_by.mistweaver.spell_da1 )
          ab::base_dd_multiplier *= 1.0 + player->spec.mistweaver_monk->effectN( 1 ).percent();
        if ( affected_by.mistweaver.spell_ta1 )
          ab::base_td_multiplier *= 1.0 + player->spec.mistweaver_monk->effectN( 2 ).percent();

        break;
      }
      case MONK_WINDWALKER:
      {
        if ( affected_by.windwalker.spell_da1 )
        {
          ab::base_dd_multiplier *= 1.0 + player->spec.windwalker_monk->effectN( 1 ).percent();
        }
        if ( affected_by.windwalker.spell_da2 )
          ab::base_dd_multiplier *= 1.0 + player->spec.windwalker_monk->effectN( 5 ).percent();
        if ( affected_by.windwalker.spell_da3 )
        {
          ab::base_dd_multiplier *= 1.0 + player->spec.windwalker_monk->effectN( 6 ).percent();
        }
        if ( affected_by.windwalker.spell_da4 )
        {
          ab::base_dd_multiplier *= 1.0 + player->spec.windwalker_monk->effectN( 8 ).percent();
        }
        if ( affected_by.windwalker.spell_da5 )
          ab::base_dd_multiplier *= 1.0 + player->spec.windwalker_monk->effectN( 9 ).percent();
        if ( affected_by.windwalker.spell_da6 )
          ab::base_dd_multiplier *= 1.0 + player->spec.windwalker_monk->effectN( 11 ).percent();

        if ( affected_by.windwalker.spell_ta1 )
        {
          ab::base_td_multiplier *= 1.0 + player->spec.windwalker_monk->effectN( 2 ).percent();
        }
        if ( affected_by.windwalker.spell_ta2 )
          ab::base_dd_multiplier *= 1.0 + player->spec.windwalker_monk->effectN( 7 ).percent();

        if ( ab::data().affected_by( player->spec.windwalker_monk->effectN( 14 ) ) )
          ab::trigger_gcd += player->spec.windwalker_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds
        // Technically minimum GCD is 750ms but all but the level 15 spells have a minimum GCD of 1 sec
        ab::min_gcd = timespan_t::from_seconds( 1.0 );

        // Cooldown reduction
        if ( ab::data().affected_by( player->spec.windwalker_monk->effectN( 10 ) ) )
          ab::cooldown->duration *= 1 + player->spec.windwalker_monk->effectN( 10 ).percent();  // saved as -100
        break;
      }
      default:
        break;
    }
  }

  /**
   * Initialize all affected_by members and print out debug info
   */
  void init_affected_by()
  {
    struct affect_init_t
    {
      const spelleffect_data_t& effect;
      bool& affects;
    } affects[] = {
        {p()->spec.brewmaster_monk->effectN( 1 ), affected_by.brewmaster.spell_da1},
        {p()->spec.brewmaster_monk->effectN( 6 ), affected_by.brewmaster.spell_da2},
        {p()->spec.brewmaster_monk->effectN( 7 ), affected_by.brewmaster.spell_da3},
        {p()->spec.brewmaster_monk->effectN( 18 ), affected_by.brewmaster.spell_da4},
        {p()->spec.brewmaster_monk->effectN( 2 ), affected_by.brewmaster.spell_ta1},
        {p()->spec.brewmaster_monk->effectN( 8 ), affected_by.brewmaster.spell_ta2},
        {p()->spec.brewmaster_monk->effectN( 4 ), affected_by.hasted_cooldown},  // not yet working, see Keg Smash
        {p()->spec.brewmaster_monk->effectN( 5 ),
         affected_by.hasted_cooldown},  // not yet working, see Ironskin-/Purifying Brew
        {p()->passives.aura_monk->effectN( 1 ), affected_by.hasted_cooldown},
        {p()->spec.mistweaver_monk->effectN( 6 ), affected_by.hasted_gcd},
        {p()->spec.mistweaver_monk->effectN( 1 ), affected_by.mistweaver.spell_da1},
        {p()->spec.mistweaver_monk->effectN( 2 ), affected_by.mistweaver.spell_ta1},
        {p()->spec.windwalker_monk->effectN( 1 ), affected_by.windwalker.spell_da1},
        {p()->spec.windwalker_monk->effectN( 5 ), affected_by.windwalker.spell_da2},
        {p()->spec.windwalker_monk->effectN( 6 ), affected_by.windwalker.spell_da3},
        {p()->spec.windwalker_monk->effectN( 8 ), affected_by.windwalker.spell_da4},
        {p()->spec.windwalker_monk->effectN( 9 ), affected_by.windwalker.spell_da5},
        {p()->spec.windwalker_monk->effectN( 11 ), affected_by.windwalker.spell_da6},
        {p()->spec.windwalker_monk->effectN( 2 ), affected_by.windwalker.spell_ta1},
        {p()->spec.windwalker_monk->effectN( 7 ), affected_by.windwalker.spell_ta2},
    };

    for ( const auto& a : affects )
    {
      bool affects = base_t::data().affected_by( a.effect );
      if ( affects )
      {
        a.affects = true;
        ab::sim->print_debug( "Action {} ({}) affected by {} (idx={}).", ab::name(), ab::data().id(),
                              a.effect.spell()->name_cstr(), a.effect.spell_effect_num() + 1 );
      }
    }
  }

  virtual ~monk_action_t()
  {
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

  bool ready() override
  {
    if ( !ab::ready() )
      return false;

    return true;
  }

  void init() override
  {
    ab::init();

    if ( affected_by.hasted_cooldown )
    {
      ab::cooldown->hasted = true;
    }

    /* Iterate through power entries, and find if there are resources linked to one of our stances
     */
    for ( size_t i = 0; ab::data()._power && i < ab::data()._power->size(); i++ )
    {
      const spellpower_data_t* pd = ( *ab::data()._power )[ i ];
      switch ( pd->aura_id() )
      {
        case 137023:
          assert( _resource_by_stance[ specdata::spec_idx( MONK_BREWMASTER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ specdata::spec_idx( MONK_BREWMASTER ) ] = pd->resource();
          break;
        case 137024:
          assert( _resource_by_stance[ specdata::spec_idx( MONK_MISTWEAVER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ specdata::spec_idx( MONK_MISTWEAVER ) ] = pd->resource();
          break;
        case 137025:
          assert( _resource_by_stance[ specdata::spec_idx( MONK_WINDWALKER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ specdata::spec_idx( MONK_WINDWALKER ) ] = pd->resource();
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

    resource_e resource_by_stance = _resource_by_stance[ specdata::spec_idx( p()->specialization() ) ];

    if ( resource_by_stance == RESOURCE_MAX )
      return ab::current_resource();

    return resource_by_stance;
  }

  // Make sure the current combo strike ability is not the same as the previous ability used
  virtual bool compare_previous_combo_strikes( combo_strikes_e new_ability )
  {
    return p()->previous_combo_strike == new_ability;
  }

  // Used to trigger Windwalker's Combo Strike Mastery; Triggers prior to calculating damage
  void combo_strikes_trigger( combo_strikes_e new_ability )
  {
    if ( p()->mastery.combo_strikes->ok() )
    {
      if ( !compare_previous_combo_strikes( new_ability ) )
      {
        p()->buff.combo_strikes->trigger();

        if ( p()->talent.hit_combo->ok() )
          p()->buff.hit_combo->trigger();

        if ( p()->azerite.meridian_strikes.ok() && p()->cooldown.touch_of_death->remains() > timespan_t::zero() )
          p()->cooldown.touch_of_death->adjust(
              timespan_t::from_seconds( -1 *
                                        ( p()->azerite.meridian_strikes.spell_ref().effectN( 2 ).base_value() / 100 ) ),
              true );  // Saved as 10

        if ( p()->azerite.dance_of_chiji.ok() && p()->rppm.dance_of_chiji -> trigger() )
          p()->buff.dance_of_chiji->trigger();

        if ( p()->azerite.fury_of_xuen.ok() )
          p()->buff.fury_of_xuen_stacks->trigger();
      }
      else
      {
        p()->buff.combo_strikes->expire();
        p()->buff.hit_combo->expire();
      }
      p()->previous_combo_strike = new_ability;

      // The set bonus checks the last 3 unique combo strike triggering abilities before triggering a spell
      // This is an ongoing check; so theoretically it can trigger 2 times from 4 unique CS spells in a row
      // If a spell is used and it is one of the last 3 combo stirke saved, it will not trigger the buff
      // IE: Energizing Elixir -> Strike of the Windlord -> Fists of Fury -> Tiger Palm (trigger) -> Blackout Kick
      // (trigger) -> Tiger Palm -> Rising Sun Kick (trigger) The triggering CAN reset if the player casts the same
      // ability two times in a row. IE: Energizing Elixir -> Blackout Kick -> Blackout Kick -> Rising Sun Kick ->
      // Blackout Kick -> Tiger Palm (trigger)
      if ( p()->sets->has_set_bonus( MONK_WINDWALKER, T19, B4 ) )
      {
        if ( p()->t19_melee_4_piece_container_3 != CS_NONE )
        {
          // Check if the last two containers are not the same as the new ability
          if ( p()->t19_melee_4_piece_container_3 != new_ability )
          {
            if ( p()->t19_melee_4_piece_container_2 != new_ability )
            {
              // if they are not the same adjust containers and trigger the buff
              p()->t19_melee_4_piece_container_1 = p()->t19_melee_4_piece_container_2;
              p()->t19_melee_4_piece_container_2 = p()->t19_melee_4_piece_container_3;
              p()->t19_melee_4_piece_container_3 = new_ability;
              p()->buff.combo_master->trigger();
            }
            // Don't do anything if the second container is the same
          }
          // semi-reset if the last ability is the same as the new ability
          else
          {
            p()->t19_melee_4_piece_container_1 = new_ability;
            p()->t19_melee_4_piece_container_2 = CS_NONE;
            p()->t19_melee_4_piece_container_3 = CS_NONE;
          }
        }
        else if ( p()->t19_melee_4_piece_container_2 != CS_NONE )
        {
          // If the 3rd container is blank check if the first two containers are not the same
          if ( p()->t19_melee_4_piece_container_2 != new_ability )
          {
            if ( p()->t19_melee_4_piece_container_1 != new_ability )
            {
              // Assign the 3rd container and trigger the buff
              p()->t19_melee_4_piece_container_3 = new_ability;
              p()->buff.combo_master->trigger();
            }
            // Don't do anything if the first container is the same
          }
          // semi-reset if the last ability is the same as the new ability
          else
          {
            p()->t19_melee_4_piece_container_1 = new_ability;
            p()->t19_melee_4_piece_container_2 = CS_NONE;
            p()->t19_melee_4_piece_container_3 = CS_NONE;
          }
        }
        else if ( p()->t19_melee_4_piece_container_1 != CS_NONE )
        {
          // If the 2nd and 3rd container is blank, check if the first container is not the same
          if ( p()->t19_melee_4_piece_container_1 != new_ability )
            // Assign the second container
            p()->t19_melee_4_piece_container_2 = new_ability;
          // semi-reset if the last ability is the same as the new ability
          else
          {
            p()->t19_melee_4_piece_container_1 = new_ability;
            p()->t19_melee_4_piece_container_2 = CS_NONE;
            p()->t19_melee_4_piece_container_3 = CS_NONE;
          }
        }
        else
          p()->t19_melee_4_piece_container_1 = new_ability;
      }
    }
  }

  // Reduces Brewmaster Brew cooldowns by the time given
  void brew_cooldown_reduction( double time_reduction )
  {
    // we need to adjust the cooldown time DOWNWARD instead of UPWARD so multiply the time_reduction by -1
    time_reduction *= -1;

    if ( p()->cooldown.brewmaster_active_mitigation->down() )
      p()->cooldown.brewmaster_active_mitigation->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->cooldown.fortifying_brew->down() )
      p()->cooldown.fortifying_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->cooldown.black_ox_brew->down() )
      p()->cooldown.black_ox_brew->adjust( timespan_t::from_seconds( time_reduction ), true );
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

  virtual void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    timespan_t cd = cd_duration;
    // Only adjust cooldown (through serenity) if it's non zero.
    if ( cd_duration == timespan_t::min() )
    {
      cd = ab::cooldown->duration;
    }

    // Update the cooldown while Serenity is active
    if ( p()->buff.serenity->up() && ab::data().affected_by( p()->talent.serenity->effectN( 5 ) ) )
      cd *= ( 1 / ( 1 + p()->talent.serenity->effectN( 4 ).percent() ) );  // saved as 100
    ab::update_ready( cd );
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
        // Drinking Horn Cover Legendary
        if ( p()->legendary.drinking_horn_cover )
        {
          if ( p()->buff.storm_earth_and_fire->up() )
          {
            // Effect is saved as 4; duration is saved as 400 milliseconds
            double duration  = p()->legendary.drinking_horn_cover->effectN( 1 ).base_value() * 100;
            double extension = duration * ab::cost();

            // Extend the duration of the buff
            p()->buff.storm_earth_and_fire->extend_duration( p(), timespan_t::from_millis( extension ) );

            // Extend the duration of pets
            if ( !p()->pet.sef[ SEF_EARTH ]->is_sleeping() )
              p()->pet.sef[ SEF_EARTH ]->expiration->reschedule( p()->pet.sef[ SEF_EARTH ]->expiration->remains() +
                                                                 timespan_t::from_millis( extension ) );
            if ( !p()->pet.sef[ SEF_FIRE ]->is_sleeping() )
              p()->pet.sef[ SEF_FIRE ]->expiration->reschedule( p()->pet.sef[ SEF_FIRE ]->expiration->remains() +
                                                                timespan_t::from_millis( extension ) );
          }
          else if ( p()->buff.serenity->up() )
          {
            // Since this is extended based on chi spender instead of chi spent, extention is the duration
            // Effect is saved as 3; extension is saved as 300 milliseconds
            double extension = p()->legendary.drinking_horn_cover->effectN( 2 ).base_value() * 100;

            // Extend the duration of the buff
            p()->buff.serenity->extend_duration( p(), timespan_t::from_millis( extension ) );
          }
        }

        if ( p()->talent.inner_strength )
          p()->buff.inner_stength->trigger( (int)ab::cost() );

        if ( p()->talent.spirtual_focus )
        {
          p()->spiritual_focus_count += ab::cost();

          if ( p()->spiritual_focus_count >= p()->talent.spirtual_focus->effectN( 1 ).base_value() )
          {
            p()->cooldown.storm_earth_and_fire->adjust( -1 * p()->talent.spirtual_focus->effectN( 2 ).time_value() );
            p()->spiritual_focus_count -= p()->talent.spirtual_focus->effectN( 1 ).base_value();
          }
        }

        // The Emperor's Capacitor Legendary
        if ( p()->legendary.the_emperors_capacitor )
          p()->buff.the_emperors_capacitor->trigger();
      }
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
    ab::execute();

    trigger_storm_earth_and_fire( this );
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( s->action->school == SCHOOL_PHYSICAL )
    {
      trigger_mystic_touch( s );

      if ( p()->azerite.sunrise_technique.ok() )
      {
        if ( affected_by.sunrise_technique && p()->buff.sunrise_technique->up() &&
             td( s->target )->debuff.sunrise_technique->up() && s->result > 0 )
        {
          p()->active_actions.sunrise_technique->target = s->target;
          p()->active_actions.sunrise_technique->execute();
        }
      }
    }

    if ( td( s->target )->dots.touch_of_death->is_ticking() && s->action->name_str != "touch_of_death_amplifier" )
    {
      if ( s->action->harmful )
      {
        double touch_of_death_amplifier = s->result_amount;

        // Having Storm, Earth and Fire out increases the amplifier by 3 which is hard coded and not in any spell
        // effect. This is due to the fact that the SEF clones don't contribute to the amplifier.
        if ( p()->buff.storm_earth_and_fire->up() )
          touch_of_death_amplifier *= 3;

        if ( td( s->target )->debuff.touch_of_death_amplifier->up() )
        {
          td( s->target )->debuff.touch_of_death_amplifier->current_value += touch_of_death_amplifier;

          if ( ab::sim->debug )
          {
            ab::sim->out_debug.printf(
                "%s added %.2f towards Gale Burst. Current Gale Burst amount that is saved up is %.2f.",
                ab::player->name(), touch_of_death_amplifier,
                td( s->target )->debuff.touch_of_death_amplifier->current_value );
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

    if ( affected_by.hasted_gcd )
      t *= ab::player->cache.attack_haste();

    if ( t < ab::min_gcd )
      t = ab::min_gcd;

    return t;
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

    p()->pet.sef[ SEF_EARTH ]->trigger_attack( sef_ability, a );
    p()->pet.sef[ SEF_FIRE ]->trigger_attack( sef_ability, a );
    // Trigger pet retargeting if sticky target is not defined, and the Monk used one of the Cyclone
    // Strike triggering abilities
    if ( !p()->pet.sef[ SEF_EARTH ]->sticky_target &&
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
  monk_spell_t( const std::string& n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s )
  {
    ap_type = AP_WEAPON_MH;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double m = base_t::composite_target_multiplier( t );

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

    // The Wind blows increases damage by 3%
    if ( p()->legendary.the_wind_blows )
    {
      if ( base_t::data().affected_by( p()->passives.the_wind_blows->effectN( 1 ) ) )
        am *= 1.0 + p()->passives.the_wind_blows->effectN( 1 ).percent();
    }

    return am;
  }
};

struct monk_heal_t : public monk_action_t<heal_t>
{
  monk_heal_t( const std::string& n, monk_t& p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, &p, s )
  {
    harmful = false;
    ap_type = AP_WEAPON_MH;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
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
        if ( p()->talent.mist_wrap )
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

    // The Wind blows increases damage by 3%
    if ( p()->legendary.the_wind_blows )
    {
      if ( base_t::data().affected_by( p()->passives.the_wind_blows->effectN( 1 ) ) )
        am *= 1.0 + p()->passives.the_wind_blows->effectN( 1 ).percent();
    }

    return am;
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

namespace pet_summon {
  
struct summon_pet_t : public monk_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, const std::string& pname, monk_t* p,
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

  virtual void execute() override
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
// Invoke Xuen, the White Tiger
// ==========================================================================

struct xuen_spell_t : public summon_pet_t
{
  xuen_spell_t( monk_t* p, const std::string& options_str )
    : summon_pet_t( "invoke_xuen_the_white_tiger", "xuen_the_white_tiger", p, p->talent.invoke_xuen )
  {
    parse_options( options_str );

    harmful            = false;
    summoning_duration = data().duration();
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_SPELL;
  }
};

// ==========================================================================
// Fury of Xuen
// ==========================================================================

struct fury_of_xuen_spell_t : public summon_pet_t
{
  fury_of_xuen_spell_t( monk_t* p )
    : summon_pet_t( "fury_of_xuen", "fury_of_xuen", p, p->azerite.fury_of_xuen )
  {
    background = true;

    harmful = false;
    summoning_duration = p->passives.fury_of_xuen_haste_buff->duration();
    min_gcd = timespan_t::zero();
  }
};

// ==========================================================================
// Invoke Niuzao, the Black Ox
// ==========================================================================

struct niuzao_spell_t : public summon_pet_t
{
  niuzao_spell_t( monk_t* p, const std::string& options_str )
    : summon_pet_t( "invoke_niuzao_the_black_ox", "niuzao_the_black_ox", p, p->talent.invoke_niuzao )
  {
    parse_options( options_str );

    harmful            = false;
    summoning_duration = data().duration();
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_SPELL;
  }
};

// ==========================================================================
// Storm, Earth, and Fire
// ==========================================================================

struct storm_earth_and_fire_t;

struct storm_earth_and_fire_t : public monk_spell_t
{
  storm_earth_and_fire_t( monk_t* p, const std::string& options_str )
    : monk_spell_t( "storm_earth_and_fire", p, p->spec.storm_earth_and_fire )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::from_seconds( 1 );
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_ATTACK;
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
         ( p()->pet.sef[ SEF_EARTH ]->sticky_target &&
           candidate_target == p()->pet.sef[ SEF_EARTH ]->target ) )
    {
      return false;
    }

    return monk_spell_t::target_ready( candidate_target );
  }

  bool ready() override
  {
    if ( p()->talent.serenity->ok() )
      return false;

    return monk_spell_t::ready();
  }

  // Normal summon that summons the pets, they seek out proper targeets
  void normal_summon()
  {
    auto targets   = p()->create_storm_earth_and_fire_target_list();
    auto n_targets = targets.size();

    // Start targeting logic from "owner" always
    p()->pet.sef[ SEF_EARTH ]->reset_targeting();
    p()->pet.sef[ SEF_EARTH ]->target = p()->target;
    p()->retarget_storm_earth_and_fire( p()->pet.sef[ SEF_EARTH ], targets, n_targets );
    p()->pet.sef[ SEF_EARTH ]->summon( data().duration() );

    // Start targeting logic from "owner" always
    p()->pet.sef[ SEF_FIRE ]->reset_targeting();
    p()->pet.sef[ SEF_FIRE ]->target = p()->target;
    p()->retarget_storm_earth_and_fire( p()->pet.sef[ SEF_FIRE ], targets, n_targets );
    p()->pet.sef[ SEF_FIRE ]->summon( data().duration() );
  }

  // Monk used SEF while pets are up to sticky target them into an enemy
  void sticky_targeting()
  {
    if ( sim->debug )
    {
      sim->out_debug.printf( "%s storm_earth_and_fire sticky target %s to %s (old=%s)", player->name(),
                             p()->pet.sef[ SEF_EARTH ]->name(), target->name(),
                             p()->pet.sef[ SEF_EARTH ]->target->name() );
    }

    p()->pet.sef[ SEF_EARTH ]->target        = target;
    p()->pet.sef[ SEF_EARTH ]->sticky_target = true;

    if ( sim->debug )
    {
      sim->out_debug.printf( "%s storm_earth_and_fire sticky target %s to %s (old=%s)", player->name(),
                             p()->pet.sef[ SEF_FIRE ]->name(), target->name(),
                             p()->pet.sef[ SEF_FIRE ]->target->name() );
    }

    p()->pet.sef[ SEF_FIRE ]->target        = target;
    p()->pet.sef[ SEF_FIRE ]->sticky_target = true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( !p()->buff.storm_earth_and_fire->check() )
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
    range::for_each( monk->pet.sef, [this, &targets, &n_targets]( pets::storm_earth_and_fire_pet_t* pet ) {
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
}

namespace attacks
{
struct monk_melee_attack_t : public monk_action_t<melee_attack_t>
{
  weapon_t* mh;
  weapon_t* oh;

  monk_melee_attack_t( const std::string& n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
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

  double recharge_multiplier() const override
  {
    double rm = base_t::recharge_multiplier();
    if ( p()->buff.serenity->up() )
    {
      rm *= 1.0 / ( 1 + p()->talent.serenity->effectN( 5 ).percent() );
    }

    return rm;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double m = base_t::composite_target_multiplier( t );

    return m;
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

    // The Wind blows increases damage by 3%
    if ( p()->legendary.the_wind_blows )
    {
      if ( base_t::data().affected_by( p()->passives.the_wind_blows->effectN( 1 ) ) )
        am *= 1.0 + p()->passives.the_wind_blows->effectN( 1 ).percent();
    }

    return am;
  }

  // Physical tick_action abilities need amount_type() override, so the
  // tick_action are properly physically mitigated.
  dmg_e amount_type( const action_state_t* state, bool periodic ) const override
  {
    if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
    {
      return DMG_DIRECT;
    }
    else
    {
      return base_t::amount_type( state, periodic );
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !sim->overrides.mystic_touch && s->action->result_is_hit( s->result ) && p()->passives.mystic_touch->ok() &&
         s->result_amount > 0.0 )
    {
      s->target->debuffs.mystic_touch->trigger();
    }

    if ( result_is_hit( s->result ) )
    {
      if ( p()->buff.fit_to_burst->up() )
      {
        p()->active_actions.fit_to_burst->schedule_execute();

        p()->buff.fit_to_burst->decrement();
      }
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

    if ( p->azerite.sunrise_technique.ok() )
    {
      base_dd_min = p->azerite.sunrise_technique.value();
      base_dd_max = p->azerite.sunrise_technique.value();
    }
  }
};

// ==========================================================================
// Tiger Palm
// ==========================================================================

// Eye of the Tiger ========================================================
struct eye_of_the_tiger_heal_tick_t : public monk_heal_t
{
  eye_of_the_tiger_heal_tick_t( monk_t& p, const std::string& name )
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
  eye_of_the_tiger_dmg_tick_t( monk_t* player, const std::string& name )
    : monk_spell_t( name, player, player->talent.eye_of_the_tiger->effectN( 1 ).trigger() )
  {
    background   = true;
    ww_mastery   = true;
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

  tiger_palm_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "tiger_palm", p, p->spec.tiger_palm ),
      eye_of_the_tiger_heal( new eye_of_the_tiger_heal_tick_t( *p, "eye_of_the_tiger_heal" ) ),
      eye_of_the_tiger_damage( new eye_of_the_tiger_dmg_tick_t( p, "eye_of_the_tiger_damage" ) )
  {
    parse_options( options_str );

    ww_mastery                    = true;
    sef_ability                   = SEF_TIGER_PALM;
    affected_by.sunrise_technique = true;

    add_child( eye_of_the_tiger_damage );
    add_child( eye_of_the_tiger_heal );

    if ( p->specialization() == MONK_BREWMASTER )
      base_costs[ RESOURCE_ENERGY ] *= 1 + p->spec.brewmaster_monk->effectN( 17 ).percent();  // -50% for Brewmasters

    if ( p->specialization() == MONK_WINDWALKER )
      energize_amount = p->spec.windwalker_monk->effectN( 4 ).base_value();
    else
      energize_type = ENERGIZE_NONE;

    spell_power_mod.direct = 0.0;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->spec.mistweaver_monk->effectN( 13 ).percent();

    if ( p()->buff.blackout_combo->check() )
      am *= 1 + p()->buff.blackout_combo->data().effectN( 1 ).percent();

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = monk_melee_attack_t::bonus_da( s );

    if ( p()->azerite.pressure_point.ok() )
    {
      double pp = p() -> azerite.pressure_point.value();
      switch ( p() -> specialization() )
      {
        case MONK_BREWMASTER: b += pp * ( 1 + p()->spec.brewmaster_monk -> effectN( 21 ).percent() ); break; // saved as -60
        case MONK_MISTWEAVER: b += pp * ( 1 + p()->spec.mistweaver_monk -> effectN( 16 ).percent() ); break; // saved as -28.5
        default: b += pp; break;
      }
    }

    return b;
  }

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_TIGER_PALM );

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
        if ( p()->spec.combo_breaker && p()->buff.bok_proc->trigger() )
        {
          p()->proc.bok_proc->occur();

          if ( p()->buff.storm_earth_and_fire->up() )
          {
            p()->pet.sef[ SEF_FIRE ]->buff.bok_proc_sef->trigger();
            p()->pet.sef[ SEF_EARTH ]->buff.bok_proc_sef->trigger();
          }
        }
        break;
      }
      case MONK_BREWMASTER:
      {
        if ( p()->cooldown.blackout_strike->down() )
          p()->cooldown.blackout_strike->adjust(
              timespan_t::from_seconds( -1 * p()->spec.tiger_palm->effectN( 3 ).base_value() ) );

        if ( p()->talent.spitfire )
        {
          if ( rng().roll( p()->talent.spitfire->proc_chance() ) )
          {
            p()->cooldown.breath_of_fire->reset( true );
            p()->buff.spitfire->trigger();
          }
        }

        // Reduces the remaining cooldown on your Brews by 1 sec
        double time_reduction = p()->spec.tiger_palm->effectN( 3 ).base_value();

        // 4 pieces (Brewmaster) : Tiger Palm reduces the remaining cooldown on your brews by an additional 1 sec.
        if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T19, B4 ) )
          time_reduction += p()->sets->set( MONK_BREWMASTER, T19, B4 )->effectN( 1 ).base_value();

        brew_cooldown_reduction( time_reduction );

        if ( p()->buff.blackout_combo->up() )
          p()->buff.blackout_combo->expire();
        break;
      }
      default:
        break;
    }

    if ( p()->sets->has_set_bonus( p()->specialization(), T19OH, B8 ) )
      p()->buff.tier19_oh_8pc->trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    // Apply Mark of the Crane
    if ( p()->specialization() == MONK_WINDWALKER && result_is_hit( s->result ) && p()->spec.spinning_crane_kick )
      p()->trigger_mark_of_the_crane( s );
  }
};

// ==========================================================================
// Rising Sun Kick
// ==========================================================================

// Glory of the Dawn =================================================
struct glory_of_the_dawn_t : public monk_melee_attack_t
{
  glory_of_the_dawn_t( monk_t* p, const std::string& name )
    : monk_melee_attack_t( name, p, p -> passives.glory_of_the_dawn_dmg )
  {
    background = true;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    // Feb-14-2019
    // In 8.1.5, the Glory of the Dawn artifact trait will now deal 35% increased damage while Storm, Earth, and Fire is active.
    // https://www.wowhead.com/bluetracker?topic=95585&region=us
    // https://us.forums.blizzard.com/en/wow/t/ww-sef-bugs-and-more/95585/10
    // The 35% cannot be located in any effect (whether it's Windwalker aura, SEF's spell, or in either of GotD's spells)
    // Using SEF' damage reduction times 3 for future proofing
    // (1 + -55%) = 45%; 45% * 3 = 135%
    if ( p()->buff.storm_earth_and_fire->up() ) {
      am *= ( 1 + p() -> spec.storm_earth_and_fire -> effectN( 1 ).percent() ) * 3;
    }

    return am;
  }
};

// Rising Sun Kick Damage Trigger ===========================================

struct rising_sun_kick_dmg_t : public monk_melee_attack_t
{
  rising_sun_kick_dmg_t( monk_t* p, const std::string& name )
    : monk_melee_attack_t( name, p, p->spec.rising_sun_kick->effectN( 1 ).trigger() )
  {
    ww_mastery = true;

    background = true;
    may_crit = true;
    affected_by.sunrise_technique = true;
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    // Rank 2 seems to be applied after Bonus Damage. Hence the reason for being in the Action Multiplier
    if ( p()->spec.rising_sun_kick_2 )
      am *= 1 + p()->spec.rising_sun_kick_2->effectN( 1 ).percent();

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = monk_melee_attack_t::bonus_da( s );

    if ( p()->buff.swift_roundhouse->up() )
      b += p()->buff.swift_roundhouse->stack_value();

    return b;
  }

  void init() override
  {
    monk_melee_attack_t::init();

    if ( p()->specialization() == MONK_WINDWALKER )
      ap_type = AP_WEAPON_BOTH;
  }
  
  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    switch ( p()->specialization() )
    {
      case MONK_MISTWEAVER:
      {
        if ( p()->talent.rising_thunder->ok() )
          p()->cooldown.thunder_focus_tea->reset(true);
        break;
      }
      default:
        break;
    }
  }

    virtual void impact( action_state_t* s ) override
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

        if ( p()->sets->has_set_bonus( MONK_WINDWALKER, T20, B2 ) && ( s->result == RESULT_CRIT ) )
          // -1 to reduce the spell cooldown instead of increasing
          // saved as 3000
          p()->cooldown.fists_of_fury->adjust( -1 * p()->find_spell( 242260 )->effectN( 1 ).time_value() );
      }

      if ( p()->azerite.sunrise_technique.ok() )
      {
        p()->buff.sunrise_technique->trigger();
        td( s->target )->debuff.sunrise_technique->trigger();
      }  

      if (p()->buff.swift_roundhouse->up())
        p()->buff.swift_roundhouse->expire();
    }
  }
};

struct rising_sun_kick_t : public monk_melee_attack_t
{
  rising_sun_kick_dmg_t* trigger_attack;
  glory_of_the_dawn_t* gotd;

  rising_sun_kick_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "rising_sun_kick", p, p->spec.rising_sun_kick )
  {
    parse_options( options_str );

    cooldown->duration += p->spec.mistweaver_monk->effectN( 10 ).time_value();

    if ( p->sets->has_set_bonus( MONK_WINDWALKER, T19, B2 ) )
      cooldown->duration += p->sets->set( MONK_WINDWALKER, T19, B2 )->effectN( 1 ).time_value();

    sef_ability          = SEF_RISING_SUN_KICK;
    affected_by.serenity = true;

    attack_power_mod.direct = 0;

    trigger_attack        = new rising_sun_kick_dmg_t( p, "rising_sun_kick_dmg" );
    trigger_attack->stats = stats;

    gotd = new glory_of_the_dawn_t( p, "glory_of_the_dawn" );
    add_child( gotd );
  }

  void init() override
  {
    monk_melee_attack_t::init();

    ap_type = AP_NONE;
  }

  virtual double composite_crit_chance() const override
  {
    double c = monk_melee_attack_t::composite_crit_chance();

    if ( p()->buff.pressure_point->up() )
      c += p()->buff.pressure_point->value();

    return c;
  }

  virtual void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    if ( p()->buff.serenity->up() )
      p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
  }

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_RISING_SUN_KICK );

    monk_melee_attack_t::execute();

    trigger_attack->execute();

    if ( p()->azerite.glory_of_the_dawn.ok() )
    {
      if ( rng().roll( p()->azerite.glory_of_the_dawn.spell_ref().effectN( 3 ).percent() ) ) 
      {
        double raw = p()->azerite.glory_of_the_dawn.value();
        gotd -> target = p() -> target;
        gotd -> base_dd_max = raw;
        gotd -> base_dd_min = raw;
        gotd -> execute();

        if ( p()->specialization() == MONK_WINDWALKER )
          p()->resource_gain( RESOURCE_CHI, p()->passives.glory_of_the_dawn_dmg->effectN( 3 ).base_value(), p()->gain.glory_of_the_dawn );
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
  blackout_kick_totm_proc( monk_t* p ) : monk_melee_attack_t( "blackout_kick_totm_proc", p, p->passives.totm_bok_proc )
  {
    cooldown->duration = timespan_t::zero();
    background = dual             = true;
    affected_by.sunrise_technique = true;
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

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->spec.mistweaver_monk->effectN( 12 ).percent();

    return am;
  }

  virtual double cost() const override
  {
    return 0;
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    if ( rng().roll( p()->spec.teachings_of_the_monastery->effectN( 1 ).percent() ) )
      p()->cooldown.rising_sun_kick->reset( true );

    if ( p()->azerite.swift_roundhouse.ok() )
      p()->buff.swift_roundhouse->trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->talent.spirit_of_the_crane->ok() )
      p()->resource_gain(
          RESOURCE_MANA,
          ( p()->resources.max[ RESOURCE_MANA ] * p()->passives.spirit_of_the_crane->effectN( 1 ).percent() ),
          p()->gain.spirit_of_the_crane );
  }
};

// Blackout Kick Baseline ability =======================================
struct blackout_kick_t : public monk_melee_attack_t
{
  blackout_kick_totm_proc* bok_totm_proc;

  blackout_kick_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "blackout_kick", p, p->spec.blackout_kick )
  {
    ww_mastery = true;

    parse_options( options_str );
    sef_ability                   = SEF_BLACKOUT_KICK;
    affected_by.sunrise_technique = true;

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
        if ( p->spec.blackout_kick_3 )
          // Saved as -1
          base_costs[ RESOURCE_CHI ] +=
              p->spec.blackout_kick_3->effectN( 1 ).base_value();  // Reduce base from 2 chi to 1
        break;
      }
      default:
        break;
    }
    sef_ability = SEF_BLACKOUT_KICK;
  }

  void init() override
  {
    monk_melee_attack_t::init();

    if ( p()->specialization() == MONK_WINDWALKER )
      ap_type = AP_WEAPON_BOTH;
  }

  virtual double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( c <= 0 )
      return 0;

    return c;
  }

  virtual bool ready() override
  {
    if ( p()->specialization() == MONK_BREWMASTER )
      return false;

    return monk_melee_attack_t::ready();
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    switch ( p()->specialization() )
    {
      case MONK_MISTWEAVER:
      {
        am *= 1 + p()->spec.mistweaver_monk->effectN( 12 ).percent();
        break;
      }
      case MONK_WINDWALKER:
      {
        if ( p()->sets->has_set_bonus( MONK_WINDWALKER, T21, B4 ) && p()->buff.bok_proc->up() )
          am *= 1 + p()->sets->set( MONK_WINDWALKER, T21, B4 )->effectN( 1 ).percent();
        break;
      }
      default:
        break;
    }
    return am;
  }

  virtual void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    if ( p()->buff.serenity->up() )
      p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );

    if ( p()->buff.bok_proc->up() )
    {
      p()->buff.bok_proc->expire();
      if ( !p()->buff.serenity->up() )
        p()->gain.bok_proc->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );

      if ( p()->sets->has_set_bonus( MONK_WINDWALKER, T21, B2 ) )
        p()->resource_gain( RESOURCE_CHI, p()->passives.focus_of_xuen->effectN( 1 ).base_value(),
                            p()->gain.focus_of_xuen );
    }
  }

  void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_BLACKOUT_KICK );

    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    switch ( p()->specialization() )
    {
      case MONK_MISTWEAVER:
      {
        if ( rng().roll( p()->spec.teachings_of_the_monastery->effectN( 1 ).percent() ) )
          p()->cooldown.rising_sun_kick->reset( true );
        break;
      }
      case MONK_WINDWALKER:
      {
        p()->cooldown.rising_sun_kick->adjust( -1 * p()->spec.blackout_kick->effectN( 3 ).time_value() );
        p()->cooldown.fists_of_fury->adjust( -1 * p()->spec.blackout_kick->effectN( 3 ).time_value() );
        break;
      }
      default:
        break;
    }

    if ( p()->azerite.swift_roundhouse.ok() )
      p()->buff.swift_roundhouse->trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p()->specialization() == MONK_WINDWALKER && p()->spec.spinning_crane_kick )
        p()->trigger_mark_of_the_crane( s );

      if ( p()->buff.teachings_of_the_monastery->up() )
      {
        int stacks = p()->buff.teachings_of_the_monastery->current_stack;
        p()->buff.teachings_of_the_monastery->expire();

        for ( int i = 0; i < stacks; i++ )
          bok_totm_proc->execute();
      }
    }
  }
};

// ==========================================================================
// Blackout Strike
// ==========================================================================

struct blackout_strike_t : public monk_melee_attack_t
{
  blackout_strike_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "blackout_strike", p, p->spec.blackout_strike )
  {
    parse_options( options_str );

    spell_power_mod.direct = 0.0;
    cooldown->duration     = data().cooldown();
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    // Mistweavers cannot learn this spell. However the effect to adjust this spell is in the database.
    // Just being a completionist about this.
    am *= 1 + p()->spec.mistweaver_monk->effectN( 12 ).percent();

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = base_t::bonus_da( s );

    if ( p()->azerite.elusive_footwork.ok() )
      b += p()->azerite.elusive_footwork.value( 3 );

    return b;
  }

  virtual bool ready() override
  {
    // Only usable with 2-handed weapons
    if ( p()->main_hand_weapon.type <= WEAPON_1H || p()->main_hand_weapon.type == WEAPON_NONE )
      return false;

    return monk_melee_attack_t::ready();
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->talent.blackout_combo->ok() )
      p()->buff.blackout_combo->trigger();

    if ( p()->azerite.swift_roundhouse.ok() )
      p()->buff.swift_roundhouse->trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // if player level >= 78
      if ( p()->mastery.elusive_brawler )
      {
        p()->buff.elusive_brawler->trigger();

        if ( p()->azerite.elusive_footwork.ok() && s->result == RESULT_CRIT )
          p()->buff.elusive_brawler->trigger( p()->azerite.elusive_footwork.spell_ref().effectN( 2 ).base_value() );

        if ( p()->azerite.staggering_strikes.ok() )
          p()->partial_clear_stagger_amount( p()->azerite.staggering_strikes.value() );
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
    aoe               = -1;
    radius            = data->effectN( 1 ).radius();

    // Reset some variables to ensure proper execution
    dot_duration       = timespan_t::zero();
    cooldown->duration = timespan_t::zero();
  }
};

struct rushing_jade_wind_t : public monk_melee_attack_t
{
  rushing_jade_wind_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "rushing_jade_wind", p, p->talent.rushing_jade_wind )
  {
    parse_options( options_str );
    sef_ability = SEF_RUSHING_JADE_WIND;

    // Forcing the minimum GCD to 750 milliseconds
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_ATTACK;

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
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_RUSHING_JADE_WIND );

    monk_melee_attack_t::execute();

    p()->buff.rushing_jade_wind->trigger();
  }
};

// ==========================================================================
// Spinning Crane Kick
// ==========================================================================

struct sck_tick_action_t : public monk_melee_attack_t
{
  sck_tick_action_t( const std::string& name, monk_t* p, const spell_data_t* data )
    : monk_melee_attack_t( name, p, data )
  {
    affected_by.sunrise_technique = true;
    ww_mastery = true;

    dual = background = true;
    aoe               = -1;
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
      ap_type = AP_WEAPON_BOTH;
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

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + ( mark_of_the_crane_counter() * p()->passives.cyclone_strikes->effectN( 1 ).percent() );

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = monk_melee_attack_t::bonus_da( s );

    if ( p()->buff.dance_of_chiji->up() )
      // The amount return is the full amount. We need to divide this by 4 ticks to get the correct per-tick amount
      b += p()->azerite.dance_of_chiji.value() / 4;

    return b;
  }
};

struct spinning_crane_kick_t : public monk_melee_attack_t
{
  spinning_crane_kick_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "spinning_crane_kick", p, p->spec.spinning_crane_kick )
  {
    parse_options( options_str );

    sef_ability = SEF_SPINNING_CRANE_KICK;

    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
    tick_zero = hasted_ticks = interrupt_auto_attack = true;

    spell_power_mod.direct = 0.0;
    dot_behavior           = DOT_REFRESH;  // Spell uses Pandemic Mechanics.

    tick_action =
        new sck_tick_action_t( "spinning_crane_kick_tick", p, p->spec.spinning_crane_kick->effectN( 1 ).trigger() );

    if (p->specialization() == MONK_WINDWALKER)
      base_costs[RESOURCE_CHI] -= p->passives.cyclone_strikes->effectN(2).base_value();
  }

  // N full ticks, but never additional ones.
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  virtual double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->buff.dance_of_chiji->up() && !p()->buff.serenity->up() )
      c += p()->buff.dance_of_chiji->data().effectN( 3 ).base_value();  // saved as -2

    return c;
  }

  virtual void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    if ( p()->buff.serenity->up() )
      p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
  }

  void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_SPINNING_CRANE_KICK );

    monk_melee_attack_t::execute();

    p()->buff.spinning_crane_kick->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, composite_dot_duration( execute_state ) );
  }

  virtual void last_tick(dot_t* dot) override
  {
    monk_melee_attack_t::last_tick(dot);

    if ( p()->buff.dance_of_chiji->up() )
      p()->buff.dance_of_chiji->expire();
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
    aoe                           = -1;
    ww_mastery                    = true;
    affected_by.sunrise_technique = true;

    attack_power_mod.direct    = p->spec.fists_of_fury->effectN( 5 ).ap_coeff();
    ap_type                    = AP_WEAPON_MH;
    base_costs[ RESOURCE_CHI ] = 0;
    dot_duration               = timespan_t::zero();
    trigger_gcd                = timespan_t::zero();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = monk_melee_attack_t::bonus_da( s );

    if ( p()->azerite.open_palm_strikes.ok() )
      b += p()->azerite.open_palm_strikes.value( 4 );

    return b;
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    if ( state -> target != target )
      return p()->spec.fists_of_fury->effectN( 6 ).percent();

    return 1.0;
  }
  
  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->azerite.open_palm_strikes.ok() &&
         rng().roll( p()->azerite.open_palm_strikes.spell_ref().effectN( 2 ).percent() ) )
      p()->gain.open_palm_strikes->add( RESOURCE_CHI,
                                        p()->azerite.open_palm_strikes.spell_ref().effectN( 3 ).base_value() );
  }
};

struct fists_of_fury_t : public monk_melee_attack_t
{
  actions::pet_summon::fury_of_xuen_spell_t* xuen;

  fists_of_fury_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "fists_of_fury", p, p->spec.fists_of_fury )
  {
    parse_options( options_str );

    sef_ability           = SEF_FISTS_OF_FURY;
    affected_by.serenity  = true;

    channeled = tick_zero = true;
    interrupt_auto_attack = true;

    attack_power_mod.direct = 0;
    weapon_power_mod        = 0;

    // Effect 1 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
    base_tick_time = dot_duration / 4;
    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

    tick_action = new fists_of_fury_tick_t( p, "fists_of_fury_tick" );

    xuen = new actions::pet_summon::fury_of_xuen_spell_t( p );
  }

  virtual bool ready() override
  {
    // Only usable with 1-handed weapons
    if ( p()->main_hand_weapon.type > WEAPON_1H || p()->main_hand_weapon.type == WEAPON_NONE )
      return false;

    return monk_melee_attack_t::ready();
  }

  virtual double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->legendary.katsuos_eclipse && !p()->buff.serenity->up() )
      c += p()->legendary.katsuos_eclipse->effectN( 1 ).base_value();  // saved as -1

    return c;
  }

  virtual void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    if ( p()->buff.serenity->up() )
    {
      if ( p()->legendary.katsuos_eclipse )
        p()->gain.serenity->add(
            RESOURCE_CHI, base_costs[ RESOURCE_CHI ] + p()->legendary.katsuos_eclipse->effectN( 1 ).base_value() );
      else
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
    }
  }

  void execute() override
  {
    // Check Fury of Xuen stacks prior to checking Combo Strikes
    if ( p()->buff.fury_of_xuen_stacks->up() )
    {
      if ( rng().roll( p()->buff.fury_of_xuen_stacks->stack_value() ) )
      {
        p()->buff.fury_of_xuen_haste->trigger();
        xuen->execute();
//        p()->pet_spawner.fury_of_xuen.spawn( p()->passives.fury_of_xuen_haste_buff->duration() + timespan_t::from_seconds( 1 ), 1 );
        p()->buff.fury_of_xuen_stacks->expire();
      }
    }

    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_FISTS_OF_FURY );

    monk_melee_attack_t::execute();

    // Get the number of targets from the non sleeping target list
    auto targets = sim->target_non_sleeping_list.size();

    if ( p()->azerite.iron_fists.ok() && targets >= p()->azerite.iron_fists.spell_ref().effectN( 2 ).base_value() )
      p()->buff.iron_fists->trigger();
  }

  virtual void last_tick( dot_t* dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    if ( p()->sets->has_set_bonus( MONK_WINDWALKER, T20, B4 ) )
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
    affected_by.sunrise_technique = true;

    background = true;
    aoe        = -1;
    radius     = s->effectN( 1 ).radius();
  }
};

struct whirling_dragon_punch_t : public monk_melee_attack_t
{
  whirling_dragon_punch_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "whirling_dragon_punch", p, p->talent.whirling_dragon_punch )
  {
    sef_ability = SEF_WHIRLING_DRAGON_PUNCH;

    parse_options( options_str );
    interrupt_auto_attack = callbacks = false;
    channeled                         = true;
    dot_duration                      = data().duration();
    tick_zero                         = false;

    spell_power_mod.direct = 0.0;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_ATTACK;

    tick_action =
        new whirling_dragon_punch_tick_t( "whirling_dragon_punch_tick", p, p->passives.whirling_dragon_punch_tick );
  }

  virtual bool ready() override
  {
    // Only usable while Fists of Fury and Rising Sun Kick are on cooldown.
    if ( p()->cooldown.fists_of_fury->down() && p()->cooldown.rising_sun_kick->down() )
      return monk_melee_attack_t::ready();

    return false;
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
    affected_by.sunrise_technique = true;

    may_dodge = may_parry = may_block = may_miss = true;
    dual                                         = true;
    // attack_power_mod.direct = p -> talent.fist_of_the_white_tiger -> effectN( 1 ).ap_coeff();
    weapon = &( player->main_hand_weapon );
  }
};

struct fist_of_the_white_tiger_t : public monk_melee_attack_t
{
  fist_of_the_white_tiger_main_hand_t* mh_attack;
  fist_of_the_white_tiger_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "fist_of_the_white_tiger_offhand", p, p->talent.fist_of_the_white_tiger ),
      mh_attack( nullptr )
  {
    sef_ability                   = SEF_FIST_OF_THE_WHITE_TIGER_OH;
    ww_mastery                    = true;
    affected_by.sunrise_technique = true;
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
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_FIST_OF_THE_WHITE_TIGER );

    monk_melee_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      p()->resource_gain( RESOURCE_CHI,
                          p()->talent.fist_of_the_white_tiger->effectN( 3 ).trigger()->effectN( 1 ).base_value(),
                          p()->gain.fist_of_the_white_tiger );

      if ( p()->legendary.the_wind_blows )
        p()->buff.bok_proc->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );

      mh_attack->execute();
    }
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
      if ( player->specialization() == MONK_MISTWEAVER )
        base_multiplier *= 1.0 + player->spec.mistweaver_monk->effectN( 5 ).percent();
      else
        base_hit -= 0.19;
    }
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->buff.storm_earth_and_fire->up() )
      am *= 1.0 + p()->spec.storm_earth_and_fire->effectN( 3 ).percent();

    if ( p()->buff.serenity->up() )
      am *= 1 + p()->talent.serenity->effectN( 7 ).percent();

    if ( p()->buff.hit_combo->up() )
      am *= 1 + p()->buff.hit_combo->stack_value();

    if ( p()->legendary.the_wind_blows )
      am *= 1 + p()->passives.the_wind_blows->effectN( 3 ).percent();

    return am;
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
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 )
                                               : timespan_t::zero();
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

    if ( time_to_execute > timespan_t::zero() && player->executing )
    {
      if ( sim->debug )
        sim->out_debug.printf( "Executing '%s' during melee (%s).", player->executing->name(),
                               util::slot_type_string( weapon->slot ) );
      schedule_execute();
    }
    else
      monk_melee_attack_t::execute();
  }
};

// ==========================================================================
// Auto Attack
// ==========================================================================

struct auto_attack_t : public monk_melee_attack_t
{
  int sync_weapons;
  auto_attack_t( monk_t* player, const std::string& options_str )
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

  virtual void execute() override
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
  keg_smash_t( monk_t& p, const std::string& options_str ) : monk_melee_attack_t( "keg_smash", &p, p.spec.keg_smash )
  {
    parse_options( options_str );

    aoe = -1;

    attack_power_mod.direct = p.spec.keg_smash->effectN( 2 ).ap_coeff();
    radius                  = p.spec.keg_smash->effectN( 2 ).radius();

    cooldown->duration = p.spec.keg_smash->cooldown();
    cooldown->duration = p.spec.keg_smash->charge_cooldown();

    affected_by.hasted_cooldown = true;  // necessary since category based affected_by parsing is not a thing

    // Keg Smash does not appear to be picking up the baseline Trigger GCD reduction
    // Forcing the trigger GCD to 1 second.
    trigger_gcd = timespan_t::from_seconds( 1 );
  }

  virtual double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->legendary.stormstouts_last_gasp )
      am *= 1 + p()->legendary.stormstouts_last_gasp->effectN( 2 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    td( s->target )->debuff.keg_smash->trigger();
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->legendary.salsalabims_lost_tunic != nullptr )
      p()->cooldown.breath_of_fire->reset( true );

    // Reduces the remaining cooldown on your Brews by 4 sec.
    double time_reduction = p()->spec.keg_smash->effectN( 4 ).base_value();

    // Blackout Combo talent reduces Brew's cooldown by 2 sec.
    if ( p()->buff.blackout_combo->up() )
    {
      time_reduction += p()->buff.blackout_combo->data().effectN( 3 ).base_value();
      p()->buff.blackout_combo->expire();
    }

    brew_cooldown_reduction( time_reduction );
  }
};

// ==========================================================================
// Touch of Death
// ==========================================================================
// Touch of Death Amplifier will not show in the combat log. Damage will be added directly to Touch of Death
// However I am added Touch of Death Amplifier as a child of Touch of Death for statistics reasons.

struct touch_of_death_amplifier_t : public monk_spell_t
{
  touch_of_death_amplifier_t( monk_t* p )
    : monk_spell_t( "touch_of_death_amplifier", p, p->spec.touch_of_death_amplifier )
  {
    background                    = true;
    may_crit                      = false;
    school                        = SCHOOL_PHYSICAL;
    ap_type                       = AP_NO_WEAPON;
    affected_by.sunrise_technique = true;
  }

  void init() override
  {
    monk_spell_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct touch_of_death_t : public monk_spell_t
{
  touch_of_death_amplifier_t* touch_of_death_amplifier;

  touch_of_death_t( monk_t* p, const std::string& options_str )
    : monk_spell_t( "touch_of_death", p, p->spec.touch_of_death ),
      touch_of_death_amplifier( new touch_of_death_amplifier_t( p ) )
  {
    may_crit = hasted_ticks = false;
    parse_options( options_str );
    school             = SCHOOL_PHYSICAL;
    ap_type            = AP_NO_WEAPON;
    cooldown->duration = data().cooldown();

    add_child( touch_of_death_amplifier );
  }

  virtual bool target_ready( player_t* candidate_target ) override
  {
    // Cannot be used on a target that has Touch of Death on them already
    if ( td( candidate_target )->dots.touch_of_death->is_ticking() )
      return false;

    return monk_spell_t::target_ready( candidate_target );
  }

  void init() override
  {
    monk_spell_t::init();

    snapshot_flags = update_flags = 0;
  }

  double target_armor( player_t* ) const override
  {
    return 0;
  }

  double calculate_tick_amount( action_state_t* s, double /*dot_multiplier*/ ) const override
  {
    double amount = p()->resources.max[ RESOURCE_HEALTH ];

    amount *= p()->spec.touch_of_death->effectN( 2 ).percent();  // 35% HP

    // Bonus damage happens before any multipliers
    if ( p()->azerite.meridian_strikes.ok() )
      amount += p()->azerite.meridian_strikes.value();

    amount *= 1 + p()->cache.damage_versatility();

    if ( p()->legendary.hidden_masters_forbidden_touch )
      amount *= 1 + p()->legendary.hidden_masters_forbidden_touch->effectN( 2 ).percent();

    if ( p()->buff.combo_strikes->up() )
      amount *= 1 + p()->cache.mastery_value();

    s->result_raw   = amount;
    s->result_total = amount;

    return amount;
  }

  void last_tick( dot_t* dot ) override
  {
    if ( td( p()->target )->debuff.touch_of_death_amplifier->up() )
    {
      double touch_of_death_multiplier      = p()->spec.touch_of_death_amplifier->effectN( 1 ).percent();
      double tod_dmg                        = td( p()->target )->debuff.touch_of_death_amplifier->current_value;
      touch_of_death_amplifier->base_dd_min = tod_dmg * touch_of_death_multiplier;
      touch_of_death_amplifier->base_dd_max = tod_dmg * touch_of_death_multiplier;

      if ( sim->debug )
      {
        sim->out_debug.printf( "%s executed '%s'. Amount sent before modifiers is %.2f.", player->name(),
                               touch_of_death_amplifier->name(),
                               td( p()->target )->debuff.touch_of_death_amplifier->current_value );
      }

      touch_of_death_amplifier->target = dot->target;
      touch_of_death_amplifier->execute();

      td( p()->target )->debuff.touch_of_death_amplifier->expire();
    }

    monk_spell_t::last_tick( dot );
  }

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_TOUCH_OF_DEATH );

    monk_spell_t::execute();

    if ( p()->legendary.hidden_masters_forbidden_touch )
    {
      if ( p()->buff.hidden_masters_forbidden_touch->up() )
        p()->buff.hidden_masters_forbidden_touch->expire();
      else
      {
        p()->buff.hidden_masters_forbidden_touch->execute();
        this->cooldown->reset( true );
      }
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuff.touch_of_death_amplifier->trigger();
      td( s->target )->debuff.touch_of_death_amplifier->current_value = 0;
    }
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
    ap_type             = AP_NO_WEAPON;
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  virtual void init() override
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
  touch_of_karma_t( monk_t* p, const std::string& options_str )
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
    ap_type                   = AP_NO_WEAPON;

    double max_pct = data().effectN( 3 ).percent();

    if ( pct_health > max_pct )  // Does a maximum of 50% of the monk's HP.
      pct_health = max_pct;

    if ( interval < cooldown->duration.total_seconds() )
    {
      sim->errorf( "%s minimum interval for Touch of Karma is 90 seconds.", player->name() );
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
  virtual void init() override
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

      damage_amount *=  data().effectN( 4 ).percent();

      if ( p()->legendary.cenedril_reflector_of_hatred )
        damage_amount *= 1 + p()->legendary.cenedril_reflector_of_hatred->effectN( 1 ).percent();

      residual_action::trigger( touch_of_karma_dot, execute_state->target, damage_amount );
    }
  }
};

// ==========================================================================
// Provoke
// ==========================================================================

struct provoke_t : public monk_melee_attack_t
{
  provoke_t( monk_t* p, const std::string& options_str ) : monk_melee_attack_t( "provoke", p, p->spec.provoke )
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
  spear_hand_strike_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "spear_hand_strike", p, p->spec.spear_hand_strike )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
    may_miss = may_block = may_dodge = may_parry = false;
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->level() <= 115 )
      p()->trigger_sephuzs_secret( execute_state, MECHANIC_INTERRUPT );
  }
};

// ==========================================================================
// Leg Sweep
// ==========================================================================

struct leg_sweep_t : public monk_melee_attack_t
{
  leg_sweep_t( monk_t* p, const std::string& options_str ) : monk_melee_attack_t( "leg_sweep", p, p->spec.leg_sweep )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->level() <= 115 )
      p()->trigger_sephuzs_secret( execute_state, MECHANIC_STUN );
  }
};

// ==========================================================================
// Paralysis
// ==========================================================================

struct paralysis_t : public monk_melee_attack_t
{
  paralysis_t( monk_t* p, const std::string& options_str ) : monk_melee_attack_t( "paralysis", p, p->spec.paralysis )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }

  virtual void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->level() <= 115 )
      p()->trigger_sephuzs_secret( execute_state, MECHANIC_INCAPACITATE );
  }
};

// ==========================================================================
// Flying Serpent Kick
// ==========================================================================

struct flying_serpent_kick_t : public monk_melee_attack_t
{
  bool first_charge;
  double movement_speed_increase;
  flying_serpent_kick_t( monk_t* p, const std::string& options_str )
    : monk_melee_attack_t( "flying_serpent_kick", p, p->spec.flying_serpent_kick ),
      first_charge( true ),
      movement_speed_increase( p->spec.flying_serpent_kick->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    affected_by.sunrise_technique   = true;
    ignore_false_positive           = true;
    movement_directionality         = MOVEMENT_OMNI;
    attack_power_mod.direct         = p->passives.flying_serpent_kick_damage->effectN( 1 ).ap_coeff();
    aoe                             = -1;
    p->cooldown.flying_serpent_kick = cooldown;
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

  virtual double action_multiplier() const override
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
              p()->current.distance_to_move /
              ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) ) );
      p()->current.moving_away = 0;
    }

    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_FLYING_SERPENT_KICK );

    monk_melee_attack_t::execute();

    if ( p()->legendary.sephuzs_secret != spell_data_t::not_found() && execute_state->target->type == ENEMY_ADD )
    {
      p()->buff.sephuzs_secret->trigger();
    }
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
  energizing_elixir_t( monk_t* player, const std::string& options_str )
    : monk_spell_t( "energizing_elixir", player, player->talent.energizing_elixir )
  {
    parse_options( options_str );

    dot_duration = trigger_gcd = timespan_t::zero();
    may_miss = may_crit = harmful = false;
    energize_type                 = ENERGIZE_NONE;  // disable resource gain from spell data
  }

  virtual void execute() override
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
  black_ox_brew_t( monk_t* player, const std::string& options_str )
    : monk_spell_t( "black_ox_brew", player, player->talent.black_ox_brew )
  {
    parse_options( options_str );

    harmful       = false;
    use_off_gcd   = true;
    trigger_gcd   = timespan_t::zero();
    energize_type = ENERGIZE_NONE;  // disable resource gain from spell data
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    // Refill Ironskin Brew and Purifying Brew charges.
    p()->cooldown.brewmaster_active_mitigation->reset( true, true );

    p()->resource_gain( RESOURCE_ENERGY, p()->talent.black_ox_brew->effectN( 1 ).base_value(),
                        p()->gain.black_ox_brew_energy );
  }
};

// ==========================================================================
// Roll
// ==========================================================================

struct roll_t : public monk_spell_t
{
  roll_t( monk_t* player, const std::string& options_str )
    : monk_spell_t( "roll", player, ( player->talent.chi_torpedo ? spell_data_t::nil() : player->spec.roll ) )
  {
    parse_options( options_str );

    if ( player->talent.celerity )
    {
      cooldown->duration += player->talent.celerity->effectN( 1 ).time_value();
      cooldown->charges += (int)player->talent.celerity->effectN( 2 ).base_value();
    }
  }
};

// ==========================================================================
// Chi Torpedo
// ==========================================================================

struct chi_torpedo_t : public monk_spell_t
{
  chi_torpedo_t( monk_t* player, const std::string& options_str )
    : monk_spell_t( "chi_torpedo", player, player->talent.chi_torpedo )
  {
    parse_options( options_str );
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
  serenity_t( monk_t* player, const std::string& options_str )
    : monk_spell_t( "serenity", player, player->talent.serenity )
  {
    parse_options( options_str );
    harmful     = false;
    trigger_gcd = timespan_t::from_seconds( 1 );
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_SPELL;
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
  crackling_jade_lightning_t( monk_t& p, const std::string& options_str )
    : monk_spell_t( "crackling_jade_lightning", &p, p.spec.crackling_jade_lightning )
  {
    sef_ability = SEF_CRACKLING_JADE_LIGHTNING;

    parse_options( options_str );

    channeled = tick_may_crit = true;
    hasted_ticks = false;  // Channeled spells always have hasted ticks. Use hasted_ticks = false to disable the
                           // increase in the number of ticks.
    interrupt_auto_attack = true;
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_SPELL;
  }

  virtual double cost_per_tick( resource_e resource ) const override
  {
    double c = monk_spell_t::cost_per_tick( resource );

    if ( p()->buff.the_emperors_capacitor->up() && resource == RESOURCE_ENERGY )
      c *= 1 + ( p()->buff.the_emperors_capacitor->current_stack *
                 p()->passives.the_emperors_capacitor->effectN( 2 ).percent() );

    return c;
  }

  virtual double cost() const override
  {
    double c = monk_spell_t::cost();

    if ( p()->buff.the_emperors_capacitor->up() )
      c *= 1 + ( p()->buff.the_emperors_capacitor->current_stack *
                 p()->passives.the_emperors_capacitor->effectN( 2 ).percent() );

    return c;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_spell_t::composite_persistent_multiplier( action_state );

    if ( p()->buff.the_emperors_capacitor->up() )
      pm *= 1 + p()->buff.the_emperors_capacitor->stack_value();

    return pm;
  }

  virtual double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    am *= 1 + p()->spec.mistweaver_monk->effectN( 15 ).percent();

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

struct breath_of_fire_t : public monk_spell_t
{
  struct periodic_t : public monk_spell_t
  {
    periodic_t( monk_t& p ) : monk_spell_t( "breath_of_fire_dot", &p, p.passives.breath_of_fire_dot )
    {
      background    = true;
      tick_may_crit = may_crit = true;
      hasted_ticks             = false;
    }

    double bonus_ta( const action_state_t* s ) const override
    {
      double b = base_t::bonus_ta( s );

      if ( p()->azerite.boiling_brew.ok() )
        b += p()->azerite.boiling_brew.value( 2 );

      return b;
    }

    virtual void impact( action_state_t* s ) override
    {
      monk_spell_t::impact( s );

      if (p() -> azerite.boiling_brew.ok() && p() -> rppm.boiling_brew->trigger() )
        p()->buff.gift_of_the_ox->trigger();
    }
  };

  periodic_t* dot_action;

  breath_of_fire_t( monk_t& p, const std::string& options_str )
    : monk_spell_t( "breath_of_fire", &p, p.spec.breath_of_fire ), dot_action( new periodic_t( p ) )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::from_seconds( 1 );

    add_child( dot_action );
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown->duration;

    // Update the cooldown if Blackout Combo is up
    if ( p()->buff.blackout_combo->up() )
    {
      cd += p()->buff.blackout_combo->data().effectN( 2 ).time_value();  // saved as -6 seconds
      p()->buff.blackout_combo->expire();
    }

    monk_spell_t::update_ready( cd );
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if ( p()->buff.spitfire->up() )
      p()->buff.spitfire->expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    monk_td_t& td = *this->td( s->target );

    if ( td.debuff.keg_smash->up() )
    {
      dot_action->target = s->target;
      dot_action->execute();
    }

    // if player level >= 78
    if ( p()->mastery.elusive_brawler )
    {
      p()->buff.elusive_brawler->trigger();

      if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T21, B2 ) &&
           rng().roll( p()->sets->set( MONK_BREWMASTER, T21, B2 )->effectN( 1 ).percent() ) )
        p()->buff.elusive_brawler->trigger();
    }
  }
};

// ==========================================================================
// Fortifying Brew
// ==========================================================================

struct fortifying_brew_t : public monk_spell_t
{
  fortifying_brew_t( monk_t& p, const std::string& options_str )
    : monk_spell_t( "fortifying_brew", &p, p.spec.fortifying_brew )
  {
    parse_options( options_str );

    harmful = may_crit = false;
    trigger_gcd        = timespan_t::zero();
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    p()->buff.fortifying_brew->trigger();
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
    dot_duration   = s->duration();
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks = tick_may_crit = false;
    target                       = p;
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );
    p()->buff.ironskin_brew->up();  // benefit tracking
    p()->stagger_damage_changed();
  }

  void assess_damage( dmg_e type, action_state_t* s ) override
  {
    base_t::assess_damage( type, s );

    p()->stagger_tick_damage.push_back( {s->result_amount} );
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );
    p()->stagger_damage_changed();
  }
  proc_types proc_type() const override
  {
    return PROC1_ANY_DAMAGE_TAKEN;
  }

  virtual void init() override
  {
    base_t::init();

    // We don't want this counted towards our dps
    stats->type = STATS_NEUTRAL;
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
    dot_t* d                = get_dot();
    double damage_remaining = 0.0;
    if ( d->is_ticking() )
      damage_remaining += d->state->result_amount;  // Assumes base_td == damage, no modifiers or crits

    d->cancel();
    cancel();
    p()->stagger_damage_changed();

    return damage_remaining;
  }

  /* Clears part of the stagger dot. Used by Purifying Brew
   * Returns amount purged
   */
  double clear_partial_damage_pct( double percent_amount )
  {
    dot_t* d                = get_dot();
    double damage_remaining = 0.0;

    if ( d->is_ticking() )
    {
      auto dot_state = debug_cast<residual_action::residual_periodic_state_t*>( d->state );
      damage_remaining += dot_state -> tick_amount; // Assumes base_td == damage, no modifiers or crits
      damage_remaining *= percent_amount;
      dot_state -> tick_amount -= damage_remaining;
    }

    sim->print_debug( "{} partially clears stagger by {:.2f}% ({} / tick).", player->name(), percent_amount * 100.0,
                      damage_remaining );

    p()->stagger_damage_changed();

    return damage_remaining;
  }

  /* Clears part of the stagger dot. Used by Staggering Strikes Azerite Trait
   * Returns amount purged
   */
  double clear_partial_damage_amount( double amount )
  {
    dot_t* d                = get_dot();
    double damage_remaining = 0.0;

    if ( d->is_ticking() )
    {
      auto dot_state = debug_cast<residual_action::residual_periodic_state_t*>( d->state );
      damage_remaining += dot_state -> tick_amount; // Assumes base_td == damage, no modifiers or crits
      damage_remaining = std::fmax( damage_remaining - amount, 0 );
      dot_state -> tick_amount = damage_remaining;
    }

    sim->print_debug( "{} partially clears stagger by {} ({} / tick).", player->name(), amount, damage_remaining );

    p()->stagger_damage_changed();

    return damage_remaining;
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
      return calculate_tick_amount( d->state, d->current_stack() );
    return 0;
  }

  double amount_remaining()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return d->state->result_amount;
    return 0;
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

  virtual bool ready() override
  {
    return p()->talent.special_delivery->ok();
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( p()->talent.special_delivery->effectN( 1 ).base_value() );
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

  ironskin_brew_t( monk_t& p, const std::string& options_str )
    : monk_spell_t( "ironskin_brew", &p, p.spec.ironskin_brew ), delivery( new special_delivery_t( p ) )
  {
    parse_options( options_str );

    harmful     = false;
    trigger_gcd = timespan_t::zero();

    p.cooldown.brewmaster_active_mitigation->duration = p.spec.ironskin_brew->charge_cooldown();
    p.cooldown.brewmaster_active_mitigation->charges  = p.spec.ironskin_brew->charges();
    p.cooldown.brewmaster_active_mitigation->duration +=
        p.talent.light_brewing->effectN( 1 ).time_value();  // Saved as -3000
    p.cooldown.brewmaster_active_mitigation->charges += (int)p.talent.light_brewing->effectN( 2 ).base_value();
    p.cooldown.brewmaster_active_mitigation->hasted = true;

    cooldown = p.cooldown.brewmaster_active_mitigation;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->buff.ironskin_brew->check() )
    {
      timespan_t base_time     = p()->buff.ironskin_brew->buff_duration;
      timespan_t max_time      = p()->passives.ironskin_brew->effectN( 2 ).base_value() * base_time;
      timespan_t max_extension = max_time - p()->buff.ironskin_brew->remains();
      p()->buff.ironskin_brew->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, std::min( base_time, max_extension ) );
    }
    else
      p()->buff.ironskin_brew->trigger();

    if ( p()->talent.special_delivery->ok() )
    {
      delivery->target = target;
      delivery->execute();
    }

    if ( p()->buff.blackout_combo->up() )
    {
      p()->active_actions.stagger_self_damage->delay_tick(
          timespan_t::from_seconds( p()->buff.blackout_combo->data().effectN( 4 ).base_value() ) );
      p()->buff.blackout_combo->expire();
    }

    if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T20, B2 ) )
      p()->buff.gift_of_the_ox->trigger();
  }
};

// ==========================================================================
// Purifying Brew
// ==========================================================================

struct purifying_brew_t : public monk_spell_t
{
  special_delivery_t* delivery;

  purifying_brew_t( monk_t& p, const std::string& options_str )
    : monk_spell_t( "purifying_brew", &p, p.spec.purifying_brew )
  {
    parse_options( options_str );

    harmful     = false;
    trigger_gcd = timespan_t::zero();

    p.cooldown.brewmaster_active_mitigation->duration = p.spec.purifying_brew->charge_cooldown();
    p.cooldown.brewmaster_active_mitigation->charges  = p.spec.purifying_brew->charges();
    p.cooldown.brewmaster_active_mitigation->duration +=
        p.talent.light_brewing->effectN( 1 ).time_value();  // Saved as -3000
    p.cooldown.brewmaster_active_mitigation->charges += (int)p.talent.light_brewing->effectN( 2 ).base_value();
    p.cooldown.brewmaster_active_mitigation->hasted = true;

    cooldown = p.cooldown.brewmaster_active_mitigation;

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
      delivery->target = target;
      delivery->execute();
    }

    if ( p()->buff.blackout_combo->up() )
    {
      p()->buff.elusive_brawler->trigger( 1 );
      p()->buff.blackout_combo->expire();
    }

    if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T20, B2 ) )
      p()->buff.gift_of_the_ox->trigger();

    if ( p()->azerite.fit_to_burst.ok() && p()->buff.heavy_stagger->check() )
    {
      p()->buff.fit_to_burst->trigger( p()->buff.fit_to_burst->max_stack() );
    }

    // Reduce stagger damage
    p()->active_actions.stagger_self_damage->clear_partial_damage_pct( data().effectN( 1 ).percent() );
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
  mana_tea_t( monk_t& p, const std::string& options_str ) : monk_spell_t( "mana_tea", &p, p.talent.mana_tea )
  {
    parse_options( options_str );

    harmful     = false;
    trigger_gcd = timespan_t::zero();
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
  thunder_focus_tea_t( monk_t& p, const std::string& options_str )
    : monk_spell_t( "Thunder_focus_tea", &p, p.spec.thunder_focus_tea )
  {
    parse_options( options_str );

    harmful     = false;
    trigger_gcd = timespan_t::zero();
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
  dampen_harm_t( monk_t& p, const std::string& options_str ) : monk_spell_t( "dampen_harm", &p, p.talent.dampen_harm )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful     = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  virtual void execute() override
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
  diffuse_magic_t( monk_t& p, const std::string& options_str )
    : monk_spell_t( "diffuse_magic", &p, p.talent.diffuse_magic )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful     = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  virtual void execute() override
  {
    p()->buff.diffuse_magic->trigger();
    monk_spell_t::execute();
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

  virtual bool ready() override
  {
    if ( p()->buff.channeling_soothing_mist->check() )
      return false;

    return monk_heal_t::ready();
  }

  virtual void impact( action_state_t* s ) override
  {
    monk_heal_t::impact( s );

    p()->buff.channeling_soothing_mist->trigger();
  }

  virtual void last_tick( dot_t* d ) override
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
// Effuse
// ==========================================================================

struct effuse_t : public monk_heal_t
{
  gust_of_mists_t* mastery;

  effuse_t( monk_t& p, const std::string& options_str ) : monk_heal_t( "effuse", p, p.spec.effuse )
  {
    parse_options( options_str );

    mastery = new gust_of_mists_t( p );

    spell_power_mod.direct = data().effectN( 1 ).ap_coeff();

    may_miss = false;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p()->specialization() == MONK_BREWMASTER || p()->specialization() == MONK_WINDWALKER )
      am *= 1 + p()->spec.effuse_2->effectN( 1 ).percent();
    else
    {
      if ( p()->buff.thunder_focus_tea->up() )
        am *= 1 + p()->spec.thunder_focus_tea->effectN( 2 ).percent();  // saved as 200
    }

    return am;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p()->buff.thunder_focus_tea->up() )
      p()->buff.thunder_focus_tea->decrement();

    if ( p()->sets->has_set_bonus( p()->specialization(), T19OH, B8 ) )
      p()->buff.tier19_oh_8pc->trigger();

    mastery->execute();
  }
};

// ==========================================================================
// Enveloping Mist
// ==========================================================================

struct enveloping_mist_t : public monk_heal_t
{
  gust_of_mists_t* mastery;

  enveloping_mist_t( monk_t& p, const std::string& options_str )
    : monk_heal_t( "enveloping_mist", p, p.spec.enveloping_mist )
  {
    parse_options( options_str );

    may_miss = false;

    dot_duration = p.spec.enveloping_mist->duration();
    if ( p.talent.mist_wrap )
      dot_duration += p.talent.mist_wrap->effectN( 1 ).time_value();

    mastery = new gust_of_mists_t( p );
  }

  virtual double cost() const override
  {
    double c = monk_heal_t::cost();

    if ( p()->buff.lifecycles_enveloping_mist->check() )
      c *= 1 + p()->buff.lifecycles_enveloping_mist->value();  // saved as -20%

    return c;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t et = monk_heal_t::execute_time();

    if ( p()->buff.thunder_focus_tea->check() )
      et *= 1 + p()->spec.thunder_focus_tea->effectN( 3 ).percent();  // saved as -100

    return et;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p()->talent.lifecycles )
    {
      if ( p()->buff.lifecycles_enveloping_mist->up() )
        p()->buff.lifecycles_enveloping_mist->expire();
      p()->buff.lifecycles_vivify->trigger();
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

  renewing_mist_t( monk_t& p, const std::string& options_str ) : monk_heal_t( "renewing_mist", p, p.spec.renewing_mist )
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

  virtual void execute() override
  {
    monk_heal_t::execute();

    mastery->execute();

    if ( p()->buff.thunder_focus_tea->up() )
      p()->buff.thunder_focus_tea->decrement();
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

  vivify_t( monk_t& p, const std::string& options_str ) : monk_heal_t( "vivify", p, p.spec.vivify )
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

    return am;
  }

  virtual double cost() const override
  {
    double c = monk_heal_t::cost();

    // TODO: check the interation between Thunder Focus Tea and Lifecycles
    if ( p()->buff.thunder_focus_tea->check() )
      c *= 1 + p()->spec.thunder_focus_tea->effectN( 5 ).percent();  // saved as -100

    if ( p()->buff.lifecycles_vivify->check() )
      c *= 1 + p()->buff.lifecycles_vivify->value();  // saved as -20%

    return c;
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p()->buff.thunder_focus_tea->up() )
      p()->buff.thunder_focus_tea->decrement();

    if ( p()->talent.lifecycles )
    {
      if ( p()->buff.lifecycles_vivify->up() )
        p()->buff.lifecycles_vivify->expire();

      p()->buff.lifecycles_enveloping_mist->trigger();
    }

    if ( p()->buff.uplifting_trance->up() )
      p()->buff.uplifting_trance->expire();

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

  essence_font_t( monk_t* p, const std::string& options_str )
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
  revival_t( monk_t& p, const std::string& options_str ) : monk_heal_t( "revival", p, p.spec.revival )
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
  gift_of_the_ox_t( monk_t& p, const std::string& options_str )
    : monk_heal_t( "gift_of_the_ox", p, p.passives.gift_of_the_ox_heal )
  {
    parse_options( options_str );
    harmful     = false;
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }

  virtual bool ready() override
  {
    if ( p()->specialization() != MONK_BREWMASTER )
      return false;

    return p()->buff.gift_of_the_ox->check();
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    p()->buff.gift_of_the_ox->decrement();

    if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T20, B4 ) )
      p()->partial_clear_stagger_pct( p()->sets->set( MONK_BREWMASTER, T20, B4 )->effectN( 1 ).percent() );
  }
};

struct gift_of_the_ox_trigger_t : public monk_heal_t
{
  gift_of_the_ox_trigger_t( monk_t& p ) 
    : monk_heal_t( "gift_of_the_ox_trigger", p, p.find_spell( 124507 ) )
  {
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T20, B4 ) )
      p()->partial_clear_stagger_pct( p()->sets->set( MONK_BREWMASTER, T20, B4 )->effectN( 1 ).percent() );
  }
};

struct gift_of_the_ox_expire_t : public monk_heal_t
{
  gift_of_the_ox_expire_t( monk_t& p ) 
    : monk_heal_t( "gift_of_the_ox_expire", p, p.find_spell( 178173 ) )
  {
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    monk_heal_t::execute();

    if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T20, B4 ) )
      p()->partial_clear_stagger_pct( p()->sets->set( MONK_BREWMASTER, T20, B4 )->effectN( 1 ).percent() );
  }
};

// ==========================================================================
// Expel Harm
// ==========================================================================

struct niuzaos_blessing_t : public monk_heal_t
{
  niuzaos_blessing_t( monk_t& p ) : 
    monk_heal_t( "niuzaos_blessing", p, p.find_spell( 278535 ) )
  {
    background = true;
    proc = true;
    target = player;
  }
};

struct expel_harm_dmg_t : public monk_spell_t
{
  expel_harm_dmg_t( monk_t* player ) : 
    monk_spell_t( "expel_harm_damage", player, player->find_spell( 115129 ) )
  {
    background = true;
  }
};

struct expel_harm_t : public monk_spell_t
{
  expel_harm_dmg_t* dmg;
  niuzaos_blessing_t* niuzao;
  expel_harm_t( monk_t* p, const std::string& options_str ) :
    monk_spell_t( "expel_harm", p, p->spec.expel_harm ),
    dmg( new expel_harm_dmg_t( p ) ),
    niuzao( new niuzaos_blessing_t( *p ) )
  {
    parse_options( options_str );
  }

  virtual bool ready() override
  {
    return p()->buff.gift_of_the_ox->check();
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    if (p()->azerite.niuzaos_blessing.ok())
    {
      double nb_dmg = p() -> azerite.niuzaos_blessing.value() * p() -> buff.gift_of_the_ox->stack();
      niuzao->base_dd_min = nb_dmg;
      niuzao->base_dd_max = nb_dmg;
      niuzao->execute();
    }

    // Triggering the Gift of the ox heals, but not the damage
    // TODO damage part of Expel Harm
    //double mult = p()->spec.expel_harm->effectN( 2 ).percent();

    for ( int i=0; i < p() ->buff.gift_of_the_ox->stack(); i++ )
      p()->buff.gift_of_the_ox->decrement();
    
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

  virtual void impact( action_state_t* s ) override
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

  virtual void execute() override
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
  chi_wave_t( monk_t* player, const std::string& options_str )
    : monk_spell_t( "chi_wave", player, player->talent.chi_wave ),
      heal( new chi_wave_heal_tick_t( *player, "chi_wave_heal" ) ),
      damage( new chi_wave_dmg_tick_t( player, "chi_wave_damage" ) ),
      dmg( true )
  {
    sef_ability = SEF_CHI_WAVE;

    parse_options( options_str );
    hasted_ticks = harmful = false;
    cooldown->hasted       = false;
    dot_duration           = timespan_t::from_seconds( player->talent.chi_wave->effectN( 1 ).base_value() );
    base_tick_time         = timespan_t::from_seconds( 1.0 );
    add_child( heal );
    add_child( damage );
    tick_zero = true;
    radius    = player->find_spell( 132466 )->effectN( 2 ).base_value();
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_SPELL;
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
  chi_burst_damage_t( monk_t& player ) : monk_spell_t( "chi_burst_damage", &player, player.passives.chi_burst_damage )
  {
    background = true;
    ww_mastery = true;
    aoe        = -1;
  }
};

struct chi_burst_t : public monk_spell_t
{
  chi_burst_heal_t* heal;
  chi_burst_damage_t* damage;
  chi_burst_t( monk_t* player, const std::string& options_str )
    : monk_spell_t( "chi_burst", player, player->talent.chi_burst ), heal( nullptr )
  {
    parse_options( options_str );
    heal          = new chi_burst_heal_t( *player );
    heal->stats   = stats;
    damage        = new chi_burst_damage_t( *player );
    damage->stats = stats;
    cooldown->hasted      = false;

    interrupt_auto_attack = false;
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_haste = HASTE_SPELL;
  }

  virtual bool ready() override
  {
    if ( p()->talent.chi_burst->ok() )
      return monk_spell_t::ready();

    return false;
  }

  virtual void execute() override
  {
    // Trigger Combo Strikes
    // registers even on a miss
    combo_strikes_trigger( CS_CHI_BURST );

    monk_spell_t::execute();

    heal->execute();
    damage->execute();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( num_targets_hit > p()->talent.chi_burst->effectN( 3 ).base_value() )
        for ( int i = 0; i < p()->talent.chi_burst->effectN( 3 ).base_value(); i++ )
          p()->resource_gain( RESOURCE_CHI, p()->find_spell( 261682 )->effectN( 1 ).base_value(), p()->gain.chi_burst );
      else
        for ( int i = 0; i < num_targets_hit; i++ )
          p()->resource_gain( RESOURCE_CHI, p()->find_spell( 261682 )->effectN( 1 ).base_value(), p()->gain.chi_burst );
    }
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
    trigger_gcd        = timespan_t::zero();
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
  refreshing_jade_wind_t( monk_t* player, const std::string& options_str )
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
  virtual void init() override
  {
    monk_heal_t::init();
    // disable the snapshot_flags for all multipliers, but specifically allow
    // action_multiplier() to be called so we can override.
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_MUL_DA;
  }
};

// ==========================================================================
// Fit to Burst
// ==========================================================================

struct fit_to_burst_t : public monk_heal_t
{
  fit_to_burst_t( monk_t& p ) : monk_heal_t( "fit_to_burst", p, p.find_spell( 275894 ) )
  {
    background  = true;
    proc        = true;
    target      = player;
    base_dd_min = base_dd_max = p.azerite.fit_to_burst.value();
  }
};

}  // end namespace heals

namespace absorbs
{
struct monk_absorb_t : public monk_action_t<absorb_t>
{
  monk_absorb_t( const std::string& n, monk_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, &player, s )
  {
  }
};

// ==========================================================================
// Life Cocoon
// ==========================================================================
struct life_cocoon_t : public monk_absorb_t
{
  life_cocoon_t( monk_t& p, const std::string& options_str ) : monk_absorb_t( "life_cocoon", p, p.spec.life_cocoon )
  {
    parse_options( options_str );
    harmful = may_crit     = false;
    cooldown->duration     = data().charge_cooldown();
    spell_power_mod.direct = 31.164;  // Hard Code 2015-Dec-29
  }

  virtual void impact( action_state_t* s ) override
  {
    p()->buff.life_cocoon->trigger( 1, s->result_amount );
    stats->add_result( 0.0, s->result_amount, ABSORB, s->result, s->block_result, s->target );
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

// Fortifying Brew Buff ==========================================================
struct fortifying_brew_t : public monk_buff_t<buff_t>
{
  int health_gain;
  fortifying_brew_t( monk_t& p, const std::string& n, const spell_data_t* s ) : monk_buff_t( p, n, s ), health_gain( 0 )
  {
    cooldown->duration = timespan_t::zero();
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    // Extra Health is set by current max_health, doesn't change when max_health changes.
    health_gain =
        static_cast<int>( p().resources.max[ RESOURCE_HEALTH ] * ( p().spec.fortifying_brew->effectN( 1 ).percent() ) );
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

// Hidden Master's Forbidden Touch Legendary
struct hidden_masters_forbidden_touch_t : public monk_buff_t<buff_t>
{
  hidden_masters_forbidden_touch_t( monk_t& p, const std::string& n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
  }
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    cooldown_t* touch_of_death = source->get_cooldown( "touch_of_death" );
    if ( touch_of_death->up() )
      touch_of_death->start();
  }
};

// Serenity Buff ==========================================================
struct serenity_buff_t : public monk_buff_t<buff_t>
{
  double percent_adjust;
  monk_t& m;
  serenity_buff_t( monk_t& p, const std::string& n, const spell_data_t* s )
    : monk_buff_t( p, n, s ), percent_adjust( 0 ), m( p )
  {
    set_default_value( s->effectN( 2 ).percent() );
    set_cooldown( timespan_t::zero() );

    set_duration( s->duration() );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

    percent_adjust = s->effectN( 4 ).percent();  // saved as 100%
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    range::for_each( m.serenity_cooldowns, []( cooldown_t* cd ) { cd->adjust_recharge_multiplier(); } );
  }

  void expire( timespan_t delay ) override
  {
    bool expired = check() != 0;

    buff_t::expire( delay );

    if ( expired )
    {
      range::for_each( m.serenity_cooldowns, []( cooldown_t* cd ) { cd->adjust_recharge_multiplier(); } );
    }
  }
};

// Touch of Karma Buff ===================================================
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

// Rushing Jade Wind Buff ================================================
struct rushing_jade_wind_buff_t : public monk_buff_t<buff_t>
{
  static void rjw_callback( buff_t* b, int, const timespan_t& )
  {
    monk_t* p = debug_cast<monk_t*>( b -> player );

    p -> active_actions.rushing_jade_wind -> execute();
  }

  rushing_jade_wind_buff_t( monk_t& p, const std::string& n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_can_cancel( true );
    set_tick_zero( true );
    set_cooldown( timespan_t::zero() );

    set_period( s->effectN( 1 ).period() );
    set_tick_time_behavior( buff_tick_time_behavior::HASTED );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

    if ( p.specialization() == MONK_BREWMASTER )
      set_duration( s -> duration() * ( 1 + p.spec.brewmaster_monk->effectN( 9 ).percent() ) );
    else
      set_duration( s -> duration() );

    set_tick_callback( rjw_callback );
    set_tick_behavior( buff_tick_behavior::CLIP );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// Gift of the Ox Buff ===================================================
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
    if ( stacks > 0 && p->current_health() < p->max_health() )
    {
      p->active_actions.gift_of_the_ox_trigger->execute();

      buff_t::decrement ( stacks, value );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    monk_t* p = debug_cast<monk_t*>( player );

    p->active_actions.gift_of_the_ox_expire->execute();

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// Windwalking Buff ======================================================
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
            ( movement_increase +
              ( p.legendary.march_of_the_legion ? p.legendary.march_of_the_legion->effectN( 1 ).percent() : 0.0 ) ),
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

// Legion Artifact Effects --------------------------------------------------------

// Brewmaster Legion Artifact
void fu_zan_the_wanderers_companion( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*>( effect.player );
  do_trinket_init( monk, MONK_BREWMASTER, monk->fu_zan_the_wanderers_companion, effect );
}

// Mistweaver Legion Artifact
void sheilun_staff_of_the_mists( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*>( effect.player );
  do_trinket_init( monk, MONK_MISTWEAVER, monk->sheilun_staff_of_the_mists, effect );
}

// Windwalker Legion Artifact
void fists_of_the_heavens( special_effect_t& effect )
{
  monk_t* monk = debug_cast<monk_t*>( effect.player );
  do_trinket_init( monk, MONK_WINDWALKER, monk->fists_of_the_heavens, effect );
}

// Legion Legendary Effects ---------------------------------------------------------
// General Legendary Effects
struct cinidaria_the_symbiote_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  cinidaria_the_symbiote_t() : super( MONK )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.cinidaria_the_symbiote = e.driver();
  }
};

struct prydaz_xavarics_magnum_opus_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  prydaz_xavarics_magnum_opus_t() : super( MONK )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.prydaz_xavarics_magnum_opus = e.driver();
  }
};

struct sephuzs_secret_enabler_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  sephuzs_secret_enabler_t() : scoped_actor_callback_t( MONK )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.sephuzs_secret = e.driver();
  }
};

struct sephuzs_secret_t : public unique_gear::class_buff_cb_t<monk_t, buff_t>
{
  sephuzs_secret_t() : super( MONK, "sephuzs_secret" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<monk_t*>( e.player )->buff.sephuzs_secret;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    auto buff = make_buff( e.player, buff_name, e.trigger() );
    buff->set_cooldown( e.player->find_spell( 226262 )->duration() )
        ->set_default_value( e.trigger()->effectN( 2 ).percent() )
        ->add_invalidate( CACHE_RUN_SPEED )
        ->add_invalidate( CACHE_HASTE );
    return buff;
  }
};

// Brewmaster Legendary Effects
struct firestone_walkers_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  firestone_walkers_t() : super( MONK_BREWMASTER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.firestone_walkers = e.driver();
  }
};

struct fundamental_observation_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  fundamental_observation_t() : super( MONK_BREWMASTER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.fundamental_observation = e.driver();
  }
};

struct gai_plins_soothing_sash_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  gai_plins_soothing_sash_t() : super( MONK_BREWMASTER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.gai_plins_soothing_sash = e.driver();
  }
};

struct jewel_of_the_lost_abbey_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  jewel_of_the_lost_abbey_t() : super( MONK_BREWMASTER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.jewel_of_the_lost_abbey = e.driver();
  }
};

struct salsalabims_lost_tunic_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  salsalabims_lost_tunic_t() : super( MONK_BREWMASTER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.salsalabims_lost_tunic = e.driver();
  }
};

struct stormstouts_last_gasp_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  stormstouts_last_gasp_t() : super( MONK_BREWMASTER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.stormstouts_last_gasp = e.driver();
    monk->cooldown.keg_smash->charges += (int)monk->legendary.stormstouts_last_gasp->effectN( 1 ).base_value();
  }
};

// Mistweaver
struct eithas_lunar_glides_of_eramas_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  eithas_lunar_glides_of_eramas_t() : super( MONK_MISTWEAVER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.eithas_lunar_glides_of_eramas = e.driver();
  }
};

struct eye_of_collidus_the_warp_watcher_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  eye_of_collidus_the_warp_watcher_t() : super( MONK_MISTWEAVER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.eye_of_collidus_the_warp_watcher = e.driver();
  }
};

struct leggings_of_the_black_flame_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  leggings_of_the_black_flame_t() : super( MONK_MISTWEAVER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.leggings_of_the_black_flame = e.driver();
  }
};

struct ovyds_winter_wrap_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  ovyds_winter_wrap_t() : super( MONK_MISTWEAVER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.ovyds_winter_wrap = e.driver();
  }
};

struct petrichor_lagniappe_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  petrichor_lagniappe_t() : super( MONK_MISTWEAVER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.petrichor_lagniappe = e.driver();
  }
};

struct unison_spaulders_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  unison_spaulders_t() : super( MONK_MISTWEAVER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.unison_spaulders = e.driver();
  }
};

// Windwalker Legendary Effects
struct cenedril_reflector_of_hatred_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  cenedril_reflector_of_hatred_t() : super( MONK_WINDWALKER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.cenedril_reflector_of_hatred = e.driver();
  }
};

struct drinking_horn_cover_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  drinking_horn_cover_t() : super( MONK_WINDWALKER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.drinking_horn_cover = e.driver();
  }
};

struct hidden_masters_forbidden_touch_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  hidden_masters_forbidden_touch_t() : super( MONK_WINDWALKER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.hidden_masters_forbidden_touch = e.driver();
  }
};

struct katsuos_eclipse_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  katsuos_eclipse_t() : super( MONK_WINDWALKER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.katsuos_eclipse = e.driver();
  }
};

struct march_of_the_legion_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  march_of_the_legion_t() : super( MONK_WINDWALKER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.march_of_the_legion = e.driver();
  }
};

struct the_emperors_capacitor_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  the_emperors_capacitor_t() : super( MONK_WINDWALKER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.the_emperors_capacitor = e.driver();
  }
};

struct the_wind_blows_t : public unique_gear::scoped_actor_callback_t<monk_t>
{
  the_wind_blows_t() : super( MONK_WINDWALKER )
  {
  }

  void manipulate( monk_t* monk, const special_effect_t& e ) override
  {
    monk->legendary.the_wind_blows = e.driver();
    monk->cooldown.fist_of_the_white_tiger->duration *= 1 - monk->legendary.the_wind_blows->effectN( 1 ).percent();
  }
};

void init()
{
  // Legion Artifacts
  unique_gear::register_special_effect( 214854, fists_of_the_heavens );
  unique_gear::register_special_effect( 214483, sheilun_staff_of_the_mists );
  unique_gear::register_special_effect( 214852, fu_zan_the_wanderers_companion );

  // Legion Legendary Effects
  // General
  unique_gear::register_special_effect( 207692, cinidaria_the_symbiote_t() );
  unique_gear::register_special_effect( 207428, prydaz_xavarics_magnum_opus_t() );
  unique_gear::register_special_effect( 208051, sephuzs_secret_enabler_t() );
  unique_gear::register_special_effect( 208051, sephuzs_secret_t(), true );

  // Brewmaster
  unique_gear::register_special_effect( 224489, firestone_walkers_t() );
  unique_gear::register_special_effect( 208878, fundamental_observation_t() );
  unique_gear::register_special_effect( 208837, gai_plins_soothing_sash_t() );
  unique_gear::register_special_effect( 208881, jewel_of_the_lost_abbey_t() );
  unique_gear::register_special_effect( 212935, salsalabims_lost_tunic_t() );
  unique_gear::register_special_effect( 248044, stormstouts_last_gasp_t() );

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
  unique_gear::register_special_effect( 248101, the_wind_blows_t() );
}
}  // namespace items

// ==========================================================================
// Monk Character Definition
// ==========================================================================

monk_td_t::monk_td_t( player_t* target, monk_t* p )
  : actor_target_data_t( target, p ), dots( dots_t() ), debuff( buffs_t() ), monk( *p )
{
  if ( p->specialization() == MONK_WINDWALKER )
  {
    debuff.mark_of_the_crane = make_buff( *this, "mark_of_the_crane", p->passives.mark_of_the_crane )
                                   ->set_default_value( p->passives.cyclone_strikes->effectN( 1 ).percent() )
                                   ->set_refresh_behavior( buff_refresh_behavior::DURATION );
    debuff.flying_serpent_kick =
        make_buff( *this, "flying_serpent_kick", p->passives.flying_serpent_kick_damage )
            ->set_default_value( p->passives.flying_serpent_kick_damage->effectN( 2 ).percent() );
    debuff.touch_of_death_amplifier = make_buff( *this, "touch_of_death_amplifier", p->spec.touch_of_death_amplifier )
                                          ->set_duration( p->spec.touch_of_death->duration() )
                                          ->set_default_value( 0 )
                                          ->set_quiet( true );
    debuff.touch_of_karma =
        make_buff( *this, "touch_of_karma_debuff", p->spec.touch_of_karma )
            // set the percent of the max hp as the default value.
            ->set_default_value( p->spec.touch_of_karma->effectN( 3 ).percent() +
                                 ( p->talent.good_karma->ok() ? p->talent.good_karma->effectN( 1 ).percent() : 0 ) );
  }
  debuff.sunrise_technique = make_buff( *this, "sunrise_technique_debuff", p->find_spell( 273299 ) );

  if ( p->specialization() == MONK_BREWMASTER )
  {
    debuff.keg_smash = make_buff( *this, "keg_smash", p->spec.keg_smash )
                           ->set_default_value( p->spec.keg_smash->effectN( 3 ).percent() );
  }

  debuff.storm_earth_and_fire = make_buff( *this, "storm_earth_and_fire_target" )->set_cooldown( timespan_t::zero() );

  dots.breath_of_fire          = target->get_dot( "breath_of_fire_dot", p );
  dots.enveloping_mist         = target->get_dot( "enveloping_mist", p );
  dots.eye_of_the_tiger_damage = target->get_dot( "eye_of_the_tiger_damage", p );
  dots.eye_of_the_tiger_heal   = target->get_dot( "eye_of_the_tiger_heal", p );
  dots.renewing_mist           = target->get_dot( "renewing_mist", p );
  dots.rushing_jade_wind       = target->get_dot( "rushing_jade_wind", p );
  dots.soothing_mist           = target->get_dot( "soothing_mist", p );
  dots.touch_of_death          = target->get_dot( "touch_of_death", p );
  dots.touch_of_karma          = target->get_dot( "touch_of_karma", p );
}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name, const std::string& options_str )
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
  if ( name == "blackout_strike" )
    return new blackout_strike_t( this, options_str );
  if ( name == "breath_of_fire" )
    return new breath_of_fire_t( *this, options_str );
  if ( name == "expel_harm")
    return new expel_harm_t( this, options_str );
  if ( name == "fortifying_brew" )
    return new fortifying_brew_t( *this, options_str );
  if ( name == "gift_of_the_ox" )
    return new gift_of_the_ox_t( *this, options_str );
  if ( name == "invoke_niuzao" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "invoke_niuzao_the_black_ox" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "ironskin_brew" )
    return new ironskin_brew_t( *this, options_str );
  if ( name == "keg_smash" )
    return new keg_smash_t( *this, options_str );
  if ( name == "purifying_brew" )
    return new purifying_brew_t( *this, options_str );
  if ( name == "provoke" )
    return new provoke_t( this, options_str );
  // Mistweaver
  if ( name == "effuse" )
    return new effuse_t( *this, options_str );
  if ( name == "enveloping_mist" )
    return new enveloping_mist_t( *this, options_str );
  if ( name == "essence_font" )
    return new essence_font_t( this, options_str );
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
    return new touch_of_death_t( this, options_str );
  if ( name == "storm_earth_and_fire" )
    return new storm_earth_and_fire_t( this, options_str );
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
    if ( ( s->result_type == HEAL_DIRECT && heal_cast->base_pct_heal > 0 ) ||
         ( s->result_type == HEAL_OVER_TIME && heal_cast->tick_pct_heal > 0 ) )
      return;
  }

  // Attempt to proc the heal
  if ( active_actions.celestial_fortune && rng().roll( composite_melee_crit_chance() ) )
  {
    active_actions.celestial_fortune->base_dd_max = active_actions.celestial_fortune->base_dd_min = s->result_amount;
    active_actions.celestial_fortune->schedule_execute();
  }
}

void monk_t::trigger_sephuzs_secret( const action_state_t* state, spell_mechanic mechanic, double override_proc_chance )
{
  switch ( mechanic )
  {
    // Interrupts will always trigger sephuz
    case MECHANIC_INTERRUPT:
      break;
    default:
      // By default, proc sephuz on persistent enemies if they are below the "boss level"
      // (playerlevel + 3), and on any kind of transient adds.
      if ( state->target->type != ENEMY_ADD && ( state->target->level() >= sim->max_player_level + 3 ) )
      {
        return;
      }
      break;
  }

  // Ensure Sephuz's Secret can even be procced. If the ring is not equipped, a fallback buff with
  // proc chance of 0 (disabled) will be created
  if ( buff.sephuzs_secret->default_chance == 0 )
  {
    return;
  }

  buff.sephuzs_secret->trigger( 1, buff_t::DEFAULT_VALUE(), override_proc_chance );
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

// monk_t::create_pet =======================================================

pet_t* monk_t::create_pet( const std::string& name, const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( name );

  if ( p )
    return p;

  using namespace pets;
  if ( name == "xuen_the_white_tiger" )
    return new xuen_pet_t( sim, this, name );
  if ( name == "fury_of_xuen" )
    return new fury_of_xuen_pet_t( sim, this, name );
  if ( name == "niuzao_the_black_ox" )
    return new niuzao_pet_t( sim, this );

  return nullptr;
}

// monk_t::create_pets ======================================================

void monk_t::create_pets()
{
  base_t::create_pets();

  if ( talent.invoke_xuen->ok() && ( find_action( "invoke_xuen" ) || find_action( "invoke_xuen_the_white_tiger" ) ) )
  {
    create_pet( "xuen_the_white_tiger" );
  }

  if ( specialization() == MONK_WINDWALKER && azerite.fury_of_xuen.ok() )
  {
    create_pet( "fury_of_xuen" );
  }

  if ( talent.invoke_niuzao->ok() && ( find_action( "invoke_niuzao" ) || find_action( "invoke_niuzao_the_black_ox" ) ) )
  {
    create_pet( "niuzao_the_black_ox" );
  }

  if ( specialization() == MONK_WINDWALKER && find_action( "storm_earth_and_fire" ) )
  {
    pet.sef[ SEF_FIRE ] = new pets::storm_earth_and_fire_pet_t( "fire_spirit", sim, this, true, WEAPON_SWORD );
    // The player BECOMES the Storm Spirit
    // SEF EARTH was changed from 2-handed user to dual welding in Legion
    pet.sef[ SEF_EARTH ] = new pets::storm_earth_and_fire_pet_t( "earth_spirit", sim, this, true, WEAPON_MACE );
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

  // Tier 30 Talents
  talent.celerity    = find_talent_spell( "Celerity" );
  talent.chi_torpedo = find_talent_spell( "Chi Torpedo" );
  talent.tigers_lust = find_talent_spell( "Tiger's Lust" );

  // Tier 45 Talents
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

  // Tier 60 Talents
  talent.tiger_tail_sweep       = find_talent_spell( "Tiger Tail Sweep" );
  talent.summon_black_ox_statue = find_talent_spell( "Summon Black Ox Statue" );  // Brewmaster & Windwalker
  talent.song_of_chi_ji         = find_talent_spell( "Song of Chi-Ji" );          // Mistweaver
  talent.ring_of_peace          = find_talent_spell( "Ring of Peace" );
  // Windwalker
  talent.good_karma = find_talent_spell( "Good Karma" );

  // Tier 75 Talents
  // Windwalker
  talent.inner_strength = find_talent_spell( "Inner Strength" );
  // Mistweaver & Windwalker
  talent.diffuse_magic = find_talent_spell( "Diffuse Magic" );
  // Brewmaster
  talent.bob_and_weave  = find_talent_spell( "Bob and Weave" );
  talent.healing_elixir = find_talent_spell( "Healing Elixir" );
  talent.dampen_harm    = find_talent_spell( "Dampen Harm" );

  // Tier 90 Talents
  // Brewmaster
  talent.special_delivery = find_talent_spell( "Special Delivery" );
  talent.invoke_niuzao    = find_talent_spell( "Invoke Niuzao, the Black Ox" );
  // Windwalker
  talent.hit_combo   = find_talent_spell( "Hit Combo" );
  talent.invoke_xuen = find_talent_spell( "Invoke Xuen, the White Tiger" );
  // Brewmaster & Windwalker
  talent.rushing_jade_wind = find_talent_spell( "Rushing Jade Wind" );
  // Mistweaver
  talent.summon_jade_serpent_statue = find_talent_spell( "Summon Jade Serpent Statue" );
  talent.refreshing_jade_wind       = find_talent_spell( "Refreshing Jade Wind" );
  talent.invoke_chi_ji              = find_talent_spell( "Invoke Chi-Ji, the Red Crane" );

  // Tier 100 Talents
  // Brewmaster
  talent.high_tolerance = find_talent_spell( "High Tolerance" );
  talent.guard          = find_talent_spell( "Guard" );
  talent.blackout_combo = find_talent_spell( "Blackout Combo" );
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
  spec.blackout_kick            = find_class_spell( "Blackout Kick" );
  spec.blackout_kick_2          = find_specialization_spell( 261916 );
  spec.blackout_kick_3          = find_specialization_spell( 261917 );
  spec.crackling_jade_lightning = find_class_spell( "Crackling Jade Lightning" );
  spec.critical_strikes         = find_specialization_spell( "Critical Strikes" );
  spec.effuse                   = find_specialization_spell( "Effuse" );
  spec.effuse_2                 = find_specialization_spell( 231602 );
  spec.leather_specialization   = find_specialization_spell( "Leather Specialization" );
  spec.leg_sweep                = find_class_spell( "Leg Sweep" );
  spec.mystic_touch             = find_class_spell( "Mystic Touch" );
  spec.paralysis                = find_class_spell( "Paralysis" );
  spec.provoke                  = find_class_spell( "Provoke" );
  spec.resuscitate              = find_class_spell( "Resuscitate" );
  spec.rising_sun_kick          = find_specialization_spell( "Rising Sun Kick" );
  spec.rising_sun_kick_2        = find_spell( 262840 );
  spec.roll                     = find_class_spell( "Roll" );
  spec.spear_hand_strike        = find_specialization_spell( "Spear Hand Strike" );
  spec.spinning_crane_kick      = find_specialization_spell( "Spinning Crane Kick" );
  spec.tiger_palm               = find_class_spell( "Tiger Palm" );

  // Brewmaster Specialization
  spec.blackout_strike     = find_specialization_spell( "Blackout Strike" );
  spec.bladed_armor        = find_specialization_spell( "Bladed Armor" );
  spec.breath_of_fire      = find_specialization_spell( "Breath of Fire" );
  spec.brewmasters_balance = find_specialization_spell( "Brewmaster's Balance" );
  spec.brewmaster_monk     = find_specialization_spell( 137023 );
  spec.celestial_fortune   = find_specialization_spell( "Celestial Fortune" );
  spec.expel_harm          = find_specialization_spell( "Expel Harm" );
  spec.fortifying_brew     = find_specialization_spell( "Fortifying Brew" );
  spec.gift_of_the_ox      = find_specialization_spell( "Gift of the Ox" );
  spec.ironskin_brew       = find_specialization_spell( "Ironskin Brew" );
  spec.keg_smash           = find_specialization_spell( "Keg Smash" );
  spec.purifying_brew      = find_specialization_spell( "Purifying Brew" );
  spec.stagger             = find_specialization_spell( "Stagger" );
  spec.zen_meditation      = find_specialization_spell( "Zen Meditation" );

  // Mistweaver Specialization
  spec.detox                      = find_specialization_spell( "Detox" );
  spec.enveloping_mist            = find_specialization_spell( "Enveloping Mist" );
  spec.envoloping_mist_2          = find_specialization_spell( 231605 );
  spec.essence_font               = find_specialization_spell( "Essence Font" );
  spec.life_cocoon                = find_specialization_spell( "Life Cocoon" );
  spec.mistweaver_monk            = find_specialization_spell( 137024 );
  spec.reawaken                   = find_specialization_spell( "Reawaken" );
  spec.renewing_mist              = find_specialization_spell( "Renewing Mist" );
  spec.renewing_mist_2            = find_specialization_spell( 231606 );
  spec.revival                    = find_specialization_spell( "Revival" );
  spec.soothing_mist              = find_specialization_spell( "Soothing Mist" );
  spec.teachings_of_the_monastery = find_specialization_spell( "Teachings of the Monastery" );
  spec.thunder_focus_tea          = find_specialization_spell( "Thunder Focus Tea" );
  spec.thunger_focus_tea_2        = find_specialization_spell( 231876 );
  spec.vivify                     = find_specialization_spell( "Vivify" );

  // Windwalker Specialization
  spec.afterlife                  = find_specialization_spell( "Afterlife" );
  spec.combat_conditioning        = find_specialization_spell( "Combat Conditioning" );
  spec.combo_breaker              = find_specialization_spell( "Combo Breaker" );
  spec.cyclone_strikes            = find_specialization_spell( "Cyclone Strikes" );
  spec.disable                    = find_specialization_spell( "Disable" );
  spec.fists_of_fury              = find_specialization_spell( "Fists of Fury" );
  spec.flying_serpent_kick        = find_specialization_spell( "Flying Serpent Kick" );
  spec.stance_of_the_fierce_tiger = find_specialization_spell( "Stance of the Fierce Tiger" );
  spec.storm_earth_and_fire       = find_specialization_spell( "Storm, Earth, and Fire" );
  spec.storm_earth_and_fire_2     = find_specialization_spell( 231627 );
  spec.touch_of_karma             = find_specialization_spell( "Touch of Karma" );
  spec.touch_of_death             = find_specialization_spell( "Touch of Death" );
  spec.touch_of_death_amplifier   = find_specialization_spell( "Touch of Death Amplifier" );
  spec.windwalker_monk            = find_specialization_spell( 137025 );
  spec.windwalking                = find_specialization_spell( "Windwalking" );

  // Azerite Powers ===================================

  // General
  azerite.sweep_the_leg = find_azerite_spell( "Sweep the Leg" );

  // Multiple
  azerite.glory_of_the_dawn  = find_azerite_spell( "Glory of the Dawn" );
  azerite.strength_of_spirit = find_azerite_spell( "Strength of Spirit" );
  azerite.sunrise_technique  = find_azerite_spell( "Sunrise Technique" );

  // Brewmaster
  azerite.boiling_brew       = find_azerite_spell( "Boiling Brew" );
  azerite.elusive_footwork   = find_azerite_spell( "Elusive Footwork" );
  azerite.fit_to_burst       = find_azerite_spell( "Fit to Burst" );
  azerite.niuzaos_blessing   = find_azerite_spell( "Niuzao's Blessing" );
  azerite.staggering_strikes = find_azerite_spell( "Staggering Strikes" );
  azerite.training_of_niuzao = find_azerite_spell( "Training of Niuzao" );

  // Mistweaver
  azerite.burst_of_life     = find_azerite_spell( "Burst of Life" );
  azerite.celestial_breath  = find_azerite_spell( "Celestial Breath" );
  azerite.font_of_life      = find_azerite_spell( "Font of Life" );
  azerite.invigorating_brew = find_azerite_spell( "Invigorating Brew" );
  azerite.misty_peaks       = find_azerite_spell( "Misty Peaks" );
  azerite.overflowing_mists = find_azerite_spell( "Overflowing Mists" );
  azerite.uplifting_spirits = find_azerite_spell( "Uplifted Spirits" );

  // Windwalker
  azerite.dance_of_chiji    = find_azerite_spell( "Dance of Chi-Ji" );
  azerite.fury_of_xuen      = find_azerite_spell( "Fury of Xuen" );
  azerite.iron_fists        = find_azerite_spell( "Iron Fists" );
  azerite.meridian_strikes  = find_azerite_spell( "Meridian Strikes" );
  azerite.open_palm_strikes = find_azerite_spell( "Open Palm Strikes" );
  azerite.pressure_point    = find_azerite_spell( "Pressure Point" );
  azerite.swift_roundhouse  = find_azerite_spell( "Swift Roundhouse" );

  // Passives =========================================
  // General
  passives.aura_monk        = find_spell( 137022 );
  passives.chi_burst_damage = find_spell( 148135 );
  passives.chi_burst_heal   = find_spell( 130654 );
  passives.chi_wave_damage  = find_spell( 132467 );
  passives.chi_wave_heal    = find_spell( 132463 );
  passives.healing_elixir =
      find_spell( 122281 );  // talent.healing_elixir -> effectN( 1 ).trigger() -> effectN( 1 ).trigger()
  passives.mystic_touch = find_spell( 8647 );

  // Brewmaster
  passives.breath_of_fire_dot  = find_spell( 123725 );
  passives.celestial_fortune   = find_spell( 216521 );
  passives.elusive_brawler     = find_spell( 195630 );
  passives.fortifying_brew     = find_spell( 120954 );
  passives.gift_of_the_ox_heal = find_spell( 124507 );
  passives.ironskin_brew       = find_spell( 215479 );
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
  passives.dizzying_kicks                   = find_spell( 196723 );
  passives.fists_of_fury_tick               = find_spell( 117418 );
  passives.flying_serpent_kick_damage       = find_spell( 123586 );
  passives.focus_of_xuen                    = find_spell( 252768 );
  passives.hit_combo                        = find_spell( 196741 );
  passives.mark_of_the_crane                = find_spell( 228287 );
  passives.touch_of_karma_tick              = find_spell( 124280 );
  passives.whirling_dragon_punch_tick       = find_spell( 158221 );

  // Azerite
  passives.glory_of_the_dawn_dmg            = find_spell( 288636 );
  passives.fury_of_xuen_stacking_buff       = find_spell( 287062 );
  passives.fury_of_xuen_haste_buff          = find_spell( 287063 );

  // Legendaries
  passives.the_emperors_capacitor = find_spell( 235054 );
  passives.the_wind_blows         = find_spell( 281452 );

  // Mastery spells =========================================
  mastery.combo_strikes   = find_mastery_spell( MONK_WINDWALKER );
  mastery.elusive_brawler = find_mastery_spell( MONK_BREWMASTER );
  mastery.gust_of_mists   = find_mastery_spell( MONK_MISTWEAVER );

  // Sample Data
  sample_datas.stagger_total_damage          = get_sample_data( "Total Stagger damage generated" );
  sample_datas.stagger_tick_damage           = get_sample_data( "Stagger damage that was not purified" );
  sample_datas.purified_damage               = get_sample_data( "Stagger damage that was purified" );
  sample_datas.light_stagger_total_damage    = get_sample_data( "Amount of damage purified while at light stagger" );
  sample_datas.moderate_stagger_total_damage = get_sample_data( "Amount of damage purified while at moderate stagger" );
  sample_datas.heavy_stagger_total_damage    = get_sample_data( "Amount of damage purified while at heavy stagger" );

  // Active Action Spells
  // Brewmaster
  active_actions.celestial_fortune = new actions::heals::celestial_fortune_t( *this );
  active_actions.gift_of_the_ox_trigger = new actions::gift_of_the_ox_trigger_t( *this );
  active_actions.gift_of_the_ox_expire = new actions::gift_of_the_ox_expire_t( *this );
  active_actions.stagger_self_damage = new actions::stagger_self_damage_t( this );

  // Windwalker
  active_actions.sunrise_technique = new actions::sunrise_technique_t( this );
  windwalking_aura = new actions::windwalking_aura_t( this );

  // Azerite Traits
  active_actions.fit_to_burst = new actions::heals::fit_to_burst_t(*this);
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
      base.spell_power_per_attack_power                  = spec.brewmaster_monk->effectN( 19 ).percent();
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
      resources.base[ RESOURCE_ENERGY ]                  = 0;
      resources.base[ RESOURCE_CHI ]                     = 0;
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
      break;
    }
    case MONK_WINDWALKER:
    {
      if ( base.distance < 1 )
        base.distance = 5;
      base_gcd += spec.windwalker_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds
      base.attack_power_per_agility     = 1.0;
      base.spell_power_per_attack_power = spec.windwalker_monk->effectN( 15 ).percent();
      resources.base[ RESOURCE_ENERGY ] = 100;
      resources.base[ RESOURCE_ENERGY ] += talent.ascension->effectN( 3 ).base_value();
      resources.base[ RESOURCE_MANA ] = 0;
      resources.base[ RESOURCE_CHI ]  = 4;
      resources.base[ RESOURCE_CHI ] += spec.windwalker_monk->effectN( 12 ).base_value();
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
                         ->set_default_value( find_spell( 119085 )->effectN( 1 ).percent() );

  buff.fortifying_brew = new buffs::fortifying_brew_t( *this, "fortifying_brew", passives.fortifying_brew );

  buff.rushing_jade_wind = new buffs::rushing_jade_wind_buff_t( *this, "rushing_jade_wind", talent.rushing_jade_wind );

  buff.dampen_harm = make_buff( this, "dampen_harm", talent.dampen_harm );

  buff.diffuse_magic = make_buff( this, "diffuse_magic", talent.diffuse_magic )
                           ->set_default_value( talent.diffuse_magic->effectN( 1 ).percent() );

  buff.tier19_oh_8pc = make_buff<stat_buff_t>( this, "grandmasters_wisdom",
                                               sets->set( specialization(), T19OH, B8 )->effectN( 1 ).trigger() );

  // Brewmaster
  buff.bladed_armor = make_buff( this, "bladed_armor", spec.bladed_armor )
                          ->set_default_value( spec.bladed_armor->effectN( 1 ).percent() )
                          ->add_invalidate( CACHE_ATTACK_POWER );

  buff.blackout_combo = make_buff( this, "blackout_combo", talent.blackout_combo->effectN( 5 ).trigger() );

  buff.elusive_brawler = make_buff( this, "elusive_brawler", mastery.elusive_brawler->effectN( 3 ).trigger() )
                             ->add_invalidate( CACHE_DODGE );

  buff.ironskin_brew =
      make_buff( this, "ironskin_brew", passives.ironskin_brew )->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  buff.gift_of_the_ox = new buffs::gift_of_the_ox_buff_t( *this, "gift_of_the_ox", find_spell( 124503 ) );

  buff.spitfire = make_buff( this, "spitfire", talent.spitfire->effectN( 1 ).trigger() );

  timespan_t stagger_duration = passives.heavy_stagger->duration();
  if ( legendary.jewel_of_the_lost_abbey )
    stagger_duration += timespan_t::from_seconds( legendary.jewel_of_the_lost_abbey->effectN( 1 ).base_value() / 10 );
  stagger_duration += timespan_t::from_seconds( talent.bob_and_weave->effectN( 1 ).base_value() / 10 );

  buff.light_stagger    = make_buff( this, "light_stagger", find_spell( 124275 ) );
  buff.moderate_stagger = make_buff( this, "moderate_stagger", find_spell( 124274 ) );
  buff.heavy_stagger    = make_buff( this, "heavy_stagger", passives.heavy_stagger );
  for ( auto&& b : {buff.light_stagger, buff.moderate_stagger, buff.heavy_stagger} )
  {
    b->set_duration( stagger_duration );
    if ( talent.high_tolerance->ok() )
      b->add_invalidate( CACHE_HASTE );
  }

  // Mistweaver
  buff.channeling_soothing_mist = make_buff( this, "channeling_soothing_mist", passives.soothing_mist_heal );

  buff.life_cocoon = make_buff<absorb_buff_t>( this, "life_cocoon", spec.life_cocoon );
  buff.life_cocoon->set_absorb_source( get_stats( "life_cocoon" ) )->set_cooldown( timespan_t::zero() );

  buff.mana_tea =
      make_buff( this, "mana_tea", talent.mana_tea )->set_default_value( talent.mana_tea->effectN( 1 ).percent() );

  buff.lifecycles_enveloping_mist = make_buff( this, "lifecycles_enveloping_mist", find_spell( 197919 ) )
                                        ->set_default_value( find_spell( 197919 )->effectN( 1 ).percent() );

  buff.lifecycles_vivify = make_buff( this, "lifecycles_vivify", find_spell( 197916 ) )
                               ->set_default_value( find_spell( 197916 )->effectN( 1 ).percent() );

  buff.refreshing_jade_wind =
      make_buff( this, "refreshing_jade_wind", talent.refreshing_jade_wind )
          ->set_default_value( talent.refreshing_jade_wind->effectN( 1 ).trigger()->effectN( 1 ).percent() )
          ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.spinning_crane_kick = make_buff( this, "spinning_crane_kick", spec.spinning_crane_kick )
                                 ->set_default_value( spec.spinning_crane_kick->effectN( 2 ).percent() )
                                 ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.teachings_of_the_monastery = make_buff( this, "teachings_of_the_monastery", find_spell( 202090 ) )
                                        ->set_default_value( find_spell( 202090 )->effectN( 1 ).percent() );

  buff.thunder_focus_tea =
      make_buff( this, "thunder_focus_tea", spec.thunder_focus_tea )
          ->set_max_stack( 1 +
                           (int)( talent.focused_thunder ? talent.focused_thunder->effectN( 1 ).base_value() : 0 ) );

  buff.uplifting_trance = make_buff( this, "uplifting_trance", find_spell( 197916 ) )
                              ->set_chance( spec.renewing_mist->effectN( 2 ).percent() +
                                            ( sets->has_set_bonus( MONK_MISTWEAVER, T19, B2 )
                                                  ? sets->set( MONK_MISTWEAVER, T19, B2 )->effectN( 1 ).percent()
                                                  : 0 ) )
                              ->set_default_value( find_spell( 197916 )->effectN( 1 ).percent() );

  // Windwalker
  buff.bok_proc =
      make_buff( this, "bok_proc", passives.bok_proc )
          ->set_chance( azerite.pressure_point.ok() ? azerite.pressure_point.spell_ref().effectN( 2 ).percent()
                                                    : spec.combo_breaker->effectN( 1 ).percent() );

  buff.combo_master = make_buff( this, "combo_master", find_spell( 211432 ) )
                          ->set_default_value( find_spell( 211432 )->effectN( 1 ).base_value() )
                          ->add_invalidate( CACHE_MASTERY );

  buff.combo_strikes =
      make_buff( this, "combo_strikes" )
          ->set_duration( timespan_t::from_minutes( 60 ) )
          ->set_quiet( true )  // In-game does not show this buff but I would like to use it for background stuff
          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.flying_serpent_kick_movement = make_buff( this, "flying_serpent_kick_movement" );  // find_spell( 115057 )

  buff.hit_combo = make_buff( this, "hit_combo", passives.hit_combo )
                       ->set_default_value( passives.hit_combo->effectN( 1 ).percent() )
                       ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.inner_stength = make_buff( this, "inner_strength", find_spell( 261769 ) )
                           ->set_default_value( find_spell( 261769 )->effectN( 1 ).base_value() );

  buff.serenity = new buffs::serenity_buff_t( *this, "serenity", talent.serenity );

  buff.storm_earth_and_fire = make_buff( this, "storm_earth_and_fire", spec.storm_earth_and_fire )
                                  ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                  ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
                                  ->set_can_cancel(false) // Undocumented hotfix 28/09/2018 - SEF can no longer be canceled.
                                  ->set_cooldown( timespan_t::zero() );

  buff.pressure_point = make_buff( this, "pressure_point", find_spell( 247255 ) )
                            ->set_default_value( find_spell( 247255 )->effectN( 1 ).percent() )
                            ->set_refresh_behavior( buff_refresh_behavior::NONE );

  buff.touch_of_karma = new buffs::touch_of_karma_buff_t( *this, "touch_of_karma", find_spell( 125174 ) );

  buff.windwalking_driver = new buffs::windwalking_driver_t( *this, "windwalking_aura_driver", find_spell( 166646 ) );

  // Legendaries
  buff.hidden_masters_forbidden_touch =
      new buffs::hidden_masters_forbidden_touch_t( *this, "hidden_masters_forbidden_touch", find_spell( 213114 ) );

  buff.the_emperors_capacitor = make_buff( this, "the_emperors_capacitor", passives.the_emperors_capacitor )
                                    ->set_default_value( passives.the_emperors_capacitor->effectN( 1 ).percent() );

  // Azerite Traits
  buff.swift_roundhouse = make_buff( this, "swift_roundhouse", find_spell( 278710 ) )
                              ->set_default_value( azerite.swift_roundhouse.value() );

  // Brewmaster
  buff.fit_to_burst = make_buff( this, "fit_to_burst", find_spell( 275893 ) )
                          ->set_trigger_spell( azerite.fit_to_burst.spell() )
                          ->set_reverse( true );
  buff.training_of_niuzao = make_buff<stat_buff_t>( this, "training_of_niuzao", find_spell( 278767 ) )
                            ->add_stat( STAT_MASTERY_RATING, azerite.training_of_niuzao.value() / 3 );
  buff.training_of_niuzao->set_max_stack( 3 );

  // Windwalker
  buff.dance_of_chiji = make_buff( this, "dance_of_chiji", find_spell(286587) );

  buff.fury_of_xuen_stacks = make_buff( this, "fury_of_xuen_stacks", passives.fury_of_xuen_stacking_buff )
                            ->set_default_value( ( passives.fury_of_xuen_stacking_buff->effectN( 3 ).base_value() / 100 ) * 0.01 );

  buff.fury_of_xuen_haste = make_buff<stat_buff_t>( this, "fury_of_xuen_haste", passives.fury_of_xuen_haste_buff )
                            ->add_stat( STAT_HASTE_RATING, azerite.fury_of_xuen.value() );

  buff.iron_fists = make_buff<stat_buff_t>( this, "iron_fists", find_spell( 272806 ) )
                        ->add_stat( STAT_CRIT_RATING, azerite.iron_fists.value() );

  buff.sunrise_technique = make_buff( this, "sunrise_technique", find_spell( 273298 ) );


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
  gain.fist_of_the_white_tiger  = get_gain( "fist_of_the_white_tiger" );
  gain.focus_of_xuen            = get_gain( "focus_of_xuen" );
  gain.gift_of_the_ox           = get_gain( "gift_of_the_ox" );
  gain.glory_of_the_dawn        = get_gain( "glory_of_the_dawn" );
  gain.rushing_jade_wind_tick   = get_gain( "rushing_jade_wind_tick" );
  gain.serenity                 = get_gain( "serenity" );
  gain.spirit_of_the_crane      = get_gain( "spirit_of_the_crane" );
  gain.tiger_palm               = get_gain( "tiger_palm" );

  // Azerite Traits
  gain.open_palm_strikes = get_gain( "open_palm_strikes" );
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.bok_proc = get_proc( "bok_proc" );
}

// monk_t::init_rng =======================================================

void monk_t::init_rng()
{
  player_t::init_rng();
  if ( specialization() == MONK_BREWMASTER )
    rppm.boiling_brew = get_rppm( "boiling_brew", find_spell( 272797 ) );

  else if ( specialization() == MONK_WINDWALKER )
    rppm.dance_of_chiji = get_rppm( "dance_of_chiji", find_spell( 286586 ) );
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

  previous_combo_strike         = CS_NONE;
  t19_melee_4_piece_container_1 = CS_NONE;
  t19_melee_4_piece_container_2 = CS_NONE;
  t19_melee_4_piece_container_3 = CS_NONE;
  spiritual_focus_count         = 0;

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

void monk_t::recalculate_resource_max( resource_e r )
{
  player_t::recalculate_resource_max( r );
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
    sim->out_debug.printf( "%s storm_earth_and_fire target list, n_targets=%u", name(), l.size() );
    range::for_each( l, [this]( player_t* t ) {
      sim->out_debug.printf( "%s cs=%.3f", t->name(),
                             get_target_data( t )->debuff.mark_of_the_crane->remains().total_seconds() );
    } );
  }

  return l;
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
}

// monk_t::retarget_storm_earth_and_fire_pets =======================================

void monk_t::retarget_storm_earth_and_fire_pets() const
{
  if ( pet.sef[ SEF_EARTH ]->sticky_target == true )
  {
    return;
  }

  auto targets   = create_storm_earth_and_fire_target_list();
  auto n_targets = targets.size();
  retarget_storm_earth_and_fire( pet.sef[ SEF_EARTH ], targets, n_targets );
  retarget_storm_earth_and_fire( pet.sef[ SEF_FIRE ], targets, n_targets );
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

/**
 * Haste modifiers affecting both melee_haste and spell_haste.
 */
double shared_composite_haste_modifiers( const monk_t& p, double h )
{
  if ( p.buff.sephuzs_secret->check() )
  {
    h *= 1.0 / ( 1.0 + p.buff.sephuzs_secret->stack_value() );
  }

  // 7.2 Sephuz's Secret passive haste. If the item is missing, default_chance will be set to 0 (by
  // the fallback buff creator).
  if ( p.legendary.sephuzs_secret && p.level() < 120 )
  {
    h *= 1.0 / ( 1.0 + p.legendary.sephuzs_secret->effectN( 3 ).percent() );
  }

  if ( p.talent.high_tolerance->ok() )
  {
    int effect_index = 2;  // Effect index of HT affecting each stagger buff
    for ( auto&& buff : {p.buff.light_stagger, p.buff.moderate_stagger, p.buff.heavy_stagger} )
    {
      if ( buff->check() )
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

  double ap = player_t::composite_melee_attack_power();

  ap += buff.bladed_armor->default_value * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
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

  return a;
}

// monk_t::temporary_movement_modifier =====================================

double monk_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

  if ( buff.sephuzs_secret->up() )
    active = std::max( buff.sephuzs_secret->data().effectN( 1 ).percent(), active );

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
  if ( legendary.sephuzs_secret && level() < 120 )
    ms += legendary.sephuzs_secret->effectN( 2 ).percent();

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
          buffs.windwalking_movement_aura->data().effectN( 1 ).percent() +
              ( legendary.march_of_the_legion ? legendary.march_of_the_legion->effectN( 1 ).percent() : 0.0 ),
          1, timespan_t::zero() );
    }

    resources.current[ RESOURCE_CHI ] = 0;
  }

  if ( spec.bladed_armor->ok() )
    buff.bladed_armor->trigger();
}

// monk_t::assess_damage ====================================================

void monk_t::assess_damage( school_e school, dmg_e dtype, action_state_t* s )
{
  buff.fortifying_brew->up();
  if ( specialization() == MONK_BREWMASTER )
  {
    if ( s->result == RESULT_DODGE )
    {
      if ( buff.elusive_brawler->up() )
        buff.elusive_brawler->expire();

      if ( legendary.anvil_hardened_wristwraps && level() < 120 )
        cooldown.brewmaster_active_mitigation->adjust(
            -1 * timespan_t::from_seconds( legendary.anvil_hardened_wristwraps->effectN( 1 ).base_value() / 10 ) );

      if ( sets->has_set_bonus( MONK_BREWMASTER, T21, B4 ) )
        // Value is saved as 20 instead of 2
        cooldown.breath_of_fire->adjust(
            -1 * timespan_t::from_seconds( sets->set( MONK_BREWMASTER, T21, B4 )->effectN( 1 ).base_value() / 10 ) );
    }
    if ( s->result == RESULT_MISS )
    {
      if ( legendary.anvil_hardened_wristwraps && level() < 120 )
        cooldown.brewmaster_active_mitigation->adjust(
            -1 * timespan_t::from_seconds( legendary.anvil_hardened_wristwraps->effectN( 1 ).base_value() / 10 ) );
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

void monk_t::target_mitigation( school_e school, dmg_e dt, action_state_t* s )
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
    // Register the tick then exit
    sample_datas.stagger_tick_damage->add( s->result_amount );
    return;
  }

  // Diffuse Magic
  if ( buff.diffuse_magic->up() && school != SCHOOL_PHYSICAL )
    s->result_amount *= 1.0 + buff.diffuse_magic->default_value;  // Stored as -60%

  // If Breath of Fire is ticking on the source target, the player receives 5% less damage
  if ( get_target_data( s->action->player )->dots.breath_of_fire->is_ticking() )
    s->result_amount *= 1.0 + passives.breath_of_fire_dot->effectN( 2 ).percent();

  // Inner Strength
  if ( buff.inner_stength->up() )
    s->result_amount *= 1.0 + buff.inner_stength->stack_value();

  // Damage Reduction Cooldowns
  if ( buff.fortifying_brew->up() )
    s->result_amount *= 1.0 - spec.fortifying_brew->effectN( 1 ).percent();

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

void monk_t::assess_damage_imminent_pre_absorb( school_e school, dmg_e dtype, action_state_t* s )
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

      else if ( school != SCHOOL_PHYSICAL )
      {
        double stagger_magic = stagger_pct( s->target->level() ) * spec.stagger->effectN( 5 ).percent();

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

void monk_t::assess_heal( school_e school, dmg_e dmg_type, action_state_t* s )
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
      if ( true_level > 110 )
        return "currents";
      else if ( true_level > 100 )
        return "seventh_demon";
      else if ( true_level > 90 )
        return "greater_draenic_agility_flask";
      else if ( true_level > 85 )
        return "spring_blossoms";
      else if ( true_level > 80 )
        return "winds";
      else if ( true_level > 75 )
        return "endless_rage";
      else if ( true_level > 70 )
        return "relentless_assault";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( true_level > 110 )
        return "endless_fathoms";
      else if ( true_level > 100 )
        return "whispered_pact";
      else if ( true_level > 90 )
        return "greater_draenic_intellect_flask";
      else if ( true_level > 85 )
        return "warm_sun";
      else if ( true_level > 80 )
        return "draconic_mind";
      else if ( true_level > 70 )
        return "blinding_light";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( true_level > 110 )
        return "currents";
      else if ( true_level > 100 )
        return "seventh_demon";
      else if ( true_level > 90 )
        return "greater_draenic_agility_flask";
      else if ( true_level > 85 )
        return "spring_blossoms";
      else if ( true_level > 80 )
        return "winds";
      else if ( true_level > 75 )
        return "endless_rage";
      else if ( true_level > 70 )
        return "relentless_assault";
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
      if ( true_level > 110 )
        return "battle_potion_of_agility";
      else if ( true_level > 100 )
        return "prolonged_power";
      else if ( true_level > 90 )
        return "draenic_agility";
      else if ( true_level > 85 )
        return "virmens_bite";
      else if ( true_level > 80 )
        return "tolvir";
      else if ( true_level > 70 )
        return "wild_magic";
      else if ( true_level > 60 )
        return "haste";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( true_level > 110 )
        return "battle_potion_of_intellect";
      else if ( true_level > 100 )
        return "prolonged_power";
      else if ( true_level > 90 )
        return "draenic_intellect";
      else if ( true_level > 85 )
        return "jade_serpent";
      else if ( true_level > 80 )
        return "volcanic";
      else if ( true_level > 70 )
        return "wild_magic";
      else if ( true_level > 60 )
        return "destruction";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( true_level > 110 )
        return "bursting_blood";
      else if ( true_level > 100 )
        return "prolonged_power";
      else if ( true_level > 90 )
        return "draenic_agility";
      else if ( true_level > 85 )
        return "virmens_bite";
      else if ( true_level > 80 )
        return "tolvir";
      else if ( true_level > 70 )
        return "wild_magic";
      else if ( true_level > 60 )
        return "haste";
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
      if ( true_level > 110 )
        return "bountiful_captains_feast";
      else if ( true_level > 100 )
        return "fishbrul_special";
      else if ( true_level > 90 )
        return "salty_squid_roll";
      else if ( true_level > 85 )
        return "sea_mist_rice_noodles";
      else if ( true_level > 80 )
        return "skewered_eel";
      else if ( true_level > 70 )
        return "blackened_dragonfin";
      else if ( true_level > 60 )
        return "warp_burger";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( true_level > 110 )
        return "bountiful_captains_feast";
      else if ( true_level > 100 )
        return "lavish_suramar_feast";
      else if ( true_level > 90 )
        return "salty_squid_roll";
      else if ( true_level > 85 )
        return "sea_mist_rice_noodles";
      else if ( true_level > 80 )
        return "skewered_eel";
      else if ( true_level > 70 )
        return "blackened_dragonfin";
      else if ( true_level > 60 )
        return "warp_burger";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( true_level > 110 )
        return "bountiful_captains_feast";
      else if ( true_level > 100 )
        return "lavish_suramar_feast";
      else if ( true_level > 90 )
        return "salty_squid_roll";
      else if ( true_level > 85 )
        return "sea_mist_rice_noodles";
      else if ( true_level > 80 )
        return "skewered_eel";
      else if ( true_level > 70 )
        return "blackened_dragonfin";
      else if ( true_level > 60 )
        return "warp_burger";
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
  return ( true_level >= 120 ) ? "battle_scarred"
                               : ( true_level >= 110 ) ? "defiled" : ( true_level >= 100 ) ? "hyper" : "disabled";
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
  pre->add_talent( this, "Chi Burst", "if=(!talent.serenity.enabled|!talent.fist_of_the_white_tiger.enabled)" );
  pre->add_talent( this, "Chi Wave" );
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
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

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
  def->add_action( this, "Gift of the Ox", "if=health<health.max*0.65" );
  def->add_talent( this, "Dampen Harm", "if=incoming_damage_1500ms&buff.fortifying_brew.down" );
  def->add_action( this, "Fortifying Brew", "if=incoming_damage_1500ms&(buff.dampen_harm.down|buff.diffuse_magic.down)" );

  int num_items = (int)items.size();

  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
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
  // Ironskin Brew
  def->add_talent( this, "Invoke Niuzao, the Black Ox", "if=target.time_to_die>25" );
  def->add_action( this, "Ironskin Brew", "if=buff.blackout_combo.down&incoming_damage_1999ms>(health.max*0.1+stagger.last_tick_damage_4)&buff.elusive_brawler.stack<2&!buff.ironskin_brew.up",
                  "Ironskin Brew priority whenever it took significant damage and ironskin brew buff is missing (adjust the health.max coefficient according to intensity of damage taken), and to dump excess charges before BoB." );
  def->add_action( this, "Ironskin Brew", "if=cooldown.brews.charges_fractional>1&cooldown.black_ox_brew.remains<3" );

  // Purifying Brew
  def->add_action( this, "Purifying Brew", "if=stagger.pct>(6*(3-(cooldown.brews.charges_fractional)))&(stagger.last_tick_damage_1>((0.02+0.001*(3-cooldown.brews.charges_fractional))*stagger.last_tick_damage_30))", 
                  "Purifying behaviour is based on normalization (iE the late expression triggers if stagger size increased over the last 30 ticks or 15 seconds)." );

  // Black Ox Brew
  def->add_talent( this, "Black Ox Brew","if=cooldown.brews.charges_fractional<0.5",
                  "Black Ox Brew is currently used to either replenish brews based on less than half a brew charge available, or low energy to enable Keg Smash" );
  def->add_talent( this, "Black Ox Brew", "if=(energy+(energy.regen*cooldown.keg_smash.remains))<40&buff.blackout_combo.down&cooldown.keg_smash.up" );


  def->add_action( this, "Keg Smash", "if=spell_targets>=2",
                  "Offensively, the APL prioritizes KS on cleave, BoS else, with energy spenders and cds sorted below" );
  def->add_action( this, "Tiger Palm", "if=talent.rushing_jade_wind.enabled&buff.blackout_combo.up&buff.rushing_jade_wind.up" );
  def->add_action( this, "Tiger Palm", "if=(talent.invoke_niuzao_the_black_ox.enabled|talent.special_delivery.enabled)&buff.blackout_combo.up" );
  def->add_action( this, "Blackout Strike" );
  def->add_action( this, "Keg Smash" );
  def->add_talent( this, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down" );
  def->add_action( this, "Breath of Fire", "if=buff.blackout_combo.down&(buff.bloodlust.down|(buff.bloodlust.up&&dot.breath_of_fire_dot.refreshable))" );
  def->add_talent( this, "Chi Burst" );
  def->add_talent( this, "Chi Wave" );
  def->add_action( this, "Tiger Palm", "if=!talent.blackout_combo.enabled&cooldown.keg_smash.remains>gcd&(energy+(energy.regen*(cooldown.keg_smash.remains+gcd)))>=65" );
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      def->add_action( racial_actions[ i ] + ",if=energy<31" );
  }
  def->add_talent( this, "Rushing Jade Wind" );
}

// Windwalker Combat Action Priority List ===============================

void monk_t::apl_combat_windwalker()
{
  std::vector<std::string> racial_actions   = get_racial_actions();
  action_priority_list_t* def               = get_action_priority_list( "default" );
  action_priority_list_t* cd                = get_action_priority_list( "cd" );
  action_priority_list_t* serenity          = get_action_priority_list( "serenity" );
  action_priority_list_t* aoe               = get_action_priority_list( "aoe" );
  action_priority_list_t* st                = get_action_priority_list( "st" );

  def->add_action( "auto_attack" );
  def->add_action( this, "Spear Hand Strike", "if=target.debuff.casting.react" );
  def->add_action( this, "Touch of Karma", "interval=90,pct_health=0.5",
                   "Touch of Karma on cooldown, if Good Karma is enabled equal to 100% of maximum health" );

  if ( sim->allow_potions )
  {
    if ( true_level >= 100 )
      def->add_action(
          "potion,if=buff.serenity.up|buff.storm_earth_and_fire.up|(!talent.serenity.enabled&trinket.proc.agility.react)|buff.bloodlust.react|target.time_to_die<=60",
          "Potion if Serenity or Storm, Earth, and Fire are up or you are running serenity and a main stat trinket "
          "procs, or you are under the effect of bloodlust, or target time to die is greater or equal to 60" );
    else
      def->add_action(
          "potion,if=buff.serenity.up|buff.storm_earth_and_fire.up|(!talent.serenity.enabled&trinket.proc.agility.react)"
          "|buff.bloodlust.react|target.time_to_die<=60" );
  }

  def->add_action( "call_action_list,name=serenity,if=buff.serenity.up" );
  def->add_talent( this, "Fist of the White Tiger", "if=(energy.time_to_max<1|(talent.serenity.enabled&cooldown.serenity.remains<2))&chi.max-chi>=3" );
  def->add_action( this, "Tiger Palm", "target_if=min:debuff.mark_of_the_crane.remains,if=(energy.time_to_max<1|(talent.serenity.enabled&cooldown.serenity.remains<2))&chi.max-chi>=2&!prev_gcd.1.tiger_palm" );
  def->add_action( "call_action_list,name=cd" );
  def->add_action( "call_action_list,name=st,if=active_enemies<3",
                   "Call the ST action list if there are 2 or less enemies" );
  def->add_action( "call_action_list,name=aoe,if=active_enemies>=3",
                   "Call the AoE action list if there are 3 or more enemies" );

  // Cooldowns
  cd->add_talent( this, "Invoke Xuen, the White Tiger", "", "Cooldowns" );

  // On-use items
  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      if ( items[ i ].name_str == "unbridled_fury" )
        cd->add_action( "use_item,name=" + items[ i ].name_str +
                        ",if=(!talent.fist_of_the_white_tiger.enabled&cooldown.fist_of_the_white_tiger.remains<14&"
                        "cooldown.fists_of_fury.remains<=15&cooldown.rising_sun_kick.remains<7)|buff.serenity.up" );
      else if ( items[ i ].name_str == "tiny_oozeling_in_a_jar" )
        cd->add_action( "use_item,name=" + items[ i ].name_str + ",if=buff.congealing_goo.stack>=6" );
      else if ( items[ i ].name_str == "horn_of_valor" )
        cd->add_action( "use_item,name=" + items[ i ].name_str +
                        ",if=!talent.serenity.enabled|cooldown.serenity.remains<18|cooldown.serenity.remains>50|target."
                        "time_to_die<=30" );
      else if ( items[ i ].name_str == "vial_of_ceaseless_toxins" )
        cd->add_action(
            "use_item,name=" + items[ i ].name_str +
            ",if=(buff.serenity.up&!equipped.specter_of_betrayal)|(equipped.specter_of_betrayal&(time<5|cooldown."
            "serenity.remains<=8))|!talent.serenity.enabled|target.time_to_die<=cooldown.serenity.remains" );
      else if ( items[ i ].name_str == "specter_of_betrayal" )
        cd->add_action( "use_item,name=" + items[ i ].name_str +
                        ",if=(cooldown.serenity.remains>10|buff.serenity.up)|!talent.serenity.enabled" );
      else if ( ( items[ i ].name_str != "draught_of_souls" ) || ( items[ i ].name_str != "forgefiends_fabricator" ) ||
                ( items[ i ].name_str != "archimondes_hatred_reborn" ) )
        cd->add_action( "use_item,name=" + items[ i ].name_str );
    }
  }

  // Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      cd->add_action( racial_actions[ i ] + ",if=chi.max-chi>=1&energy.time_to_max>=0.5",
                      "Use Arcane Torrent if you are missing at least 1 Chi and won't cap energy within 0.5 seconds" );
    else
      cd->add_action( racial_actions[ i ] );
  }

  cd->add_action( this, "Touch of Death", "if=target.time_to_die>9" );
  cd->add_action( this, "Storm, Earth, and Fire", "if=cooldown.storm_earth_and_fire.charges=2|(cooldown.fists_of_fury.remains<=6&chi>=3&cooldown.rising_sun_kick.remains<=1)|target.time_to_die<=15" );
  cd->add_talent( this, "Serenity", "if=cooldown.rising_sun_kick.remains<=2|target.time_to_die<=12" );

  // Serenity
  serenity->add_action( this, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies<3|prev_gcd.1.spinning_crane_kick", "Serenity priority" );
  serenity->add_action( this, "Fists of Fury", "if=(buff.bloodlust.up&prev_gcd.1.rising_sun_kick)|buff.serenity.remains<1|(active_enemies>1&active_enemies<5)" );
  serenity->add_action( this, "Spinning Crane Kick", "if=!prev_gcd.1.spinning_crane_kick&(active_enemies>=3|(active_enemies=2&prev_gcd.1.blackout_kick))" );
  serenity->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains" );

  // Multiple Targets
  aoe->add_action( this, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=(talent.whirling_dragon_punch.enabled&cooldown.whirling_dragon_punch.remains<5)&cooldown.fists_of_fury.remains>3",
                   "Actions.AoE is intended for use with Hectic_Add_Cleave and currently needs to be optimized" );
  aoe->add_talent( this, "Whirling Dragon Punch" );
  aoe->add_talent( this, "Energizing Elixir", "if=!prev_gcd.1.tiger_palm&chi<=1&energy<50" );
  aoe->add_action( this, "Fists of Fury", "if=energy.time_to_max>3" );
  aoe->add_talent( this, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down" );
  aoe->add_action( this, "Spinning Crane Kick", "if=!prev_gcd.1.spinning_crane_kick&(((chi>3|cooldown.fists_of_fury.remains>6)&(chi>=5|cooldown.fists_of_fury.remains>2))|energy.time_to_max<=3)" );
  aoe->add_talent( this, "Chi Burst", "if=chi<=3" );
  aoe->add_talent( this, "Fist of the White Tiger", "if=chi.max-chi>=3" );
  aoe->add_action( this, "Tiger Palm", "target_if=min:debuff.mark_of_the_crane.remains,if=chi.max-chi>=2&(!talent.hit_combo.enabled|!prev_gcd.1.tiger_palm)" );
  aoe->add_talent( this, "Chi Wave" );
  aoe->add_action( this, "Flying Serpent Kick", "if=buff.bok_proc.down,interrupt=1" );
  aoe->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=!prev_gcd.1.blackout_kick&(buff.bok_proc.up|(talent.hit_combo.enabled&prev_gcd.1.tiger_palm&chi<4))" );

  // Single Target
  st->add_talent( this, "Whirling Dragon Punch" );
  st->add_action( this, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=chi>=5" );
  st->add_action( this, "Fists of Fury", "if=energy.time_to_max>3" );
  st->add_action( this, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains" );
  st->add_action( this, "Spinning Crane Kick", "if=!prev_gcd.1.spinning_crane_kick&buff.dance_of_chiji.up");
  st->add_talent( this, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down&active_enemies>1" );
  st->add_talent( this, "Fist of the White Tiger", "if=chi<=2" );
  st->add_talent( this, "Energizing Elixir", "if=chi<=3&energy<50" );
  st->add_action( this, "Blackout Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=!prev_gcd.1.blackout_kick&(cooldown.rising_sun_kick.remains>3|chi>=3)&(cooldown.fists_of_fury.remains>4|chi>=4|(chi=2&prev_gcd.1.tiger_palm))&buff.swift_roundhouse.stack<2" );
  st->add_talent( this, "Chi Wave" );
  st->add_talent( this, "Chi Burst", "if=chi.max-chi>=1&active_enemies=1|chi.max-chi>=2" );
  st->add_action( this, "Tiger Palm", "target_if=min:debuff.mark_of_the_crane.remains,if=!prev_gcd.1.tiger_palm&chi.max-chi>=2" );
  st->add_action( this, "Flying Serpent Kick", "if=prev_gcd.1.blackout_kick&chi>3&buff.swift_roundhouse.stack<2,interrupt=1" );
}

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

  def->add_action( "run_action_list,name=aoe,if=active_enemies>=4" );
  def->add_action( "call_action_list,name=st,if=active_enemies<4" );

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
  //  aoe -> add_action( this, "Blackout Kick",
  //  "if=buff.teachings_of_the_monastery.stack=3&cooldown.rising_sun_kick.down" ); aoe -> add_action( this, "Tiger
  //  Palm", "if=buff.teachings_of_the_monastery.stack<3|buff.teachings_of_the_monastery.remains<2" );
}

// monk_t::init_actions =====================================================

void monk_t::init_action_list()
{
#ifdef NDEBUG  // Only restrict on release builds.
  // Mistweaver isn't supported atm
  if ( specialization() == MONK_MISTWEAVER && role != ROLE_ATTACK )
  {
    if ( !quiet )
      sim->errorf( "Monk mistweaver healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }
#endif
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }
  if ( main_hand_weapon.group() == WEAPON_2H && off_hand_weapon.group() == WEAPON_1H )
  {
    if ( !quiet )
      sim->errorf(
          "Player %s has a 1-Hand weapon equipped in the Off-Hand while a 2-Hand weapon is equipped in the Main-Hand.",
          name() );
    quiet = true;
    return;
  }
  if ( specialization() == MONK_BREWMASTER && off_hand_weapon.group() == WEAPON_1H )
  {
    if ( !quiet )
      sim->errorf(
          "Player %s has a Brewmaster and has equipped a 1-Hand weapon equipped in the Off-Hand when they are unable "
          "to dual weld.",
          name() );
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
    {
      stagger_base *= 1 + talent.high_tolerance->effectN( 5 ).percent();
    }

    if ( buff.fortifying_brew->up() )
    {
      stagger_base *= 1 + passives.fortifying_brew->effectN( 6 ).percent();
      ;
    }

    if ( buff.ironskin_brew->check() )
    {
      stagger_base *= 1.0 + passives.ironskin_brew->effectN( 1 ).percent();
    }
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

  double k_value     = 0;
  double lvl         = level();
  double level_check = target_level - lvl;

  // End game raiding of each expansion uses the player's level for +1, +2, and +3 level targets
  if ( ( lvl == 60 || lvl == 70 || lvl == 80 || lvl == 85 || lvl == 90 || lvl == 100 || lvl == 110 ) &&
       0 <= level_check && level_check <= 3 )
    k_value = dbc.armor_mitigation_constant( lvl );
  else
    k_value = dbc.armor_mitigation_constant( target_level );

  double stagger = stagger_base / ( stagger_base + k_value );
  return std::min( stagger, 0.99 );
}

// monk_t::current_stagger_tick_dmg ==================================================

double monk_t::current_stagger_tick_dmg()
{
  double dmg = 0;
  if ( active_actions.stagger_self_damage )
    dmg = active_actions.stagger_self_damage->tick_amount();
  return dmg;
}

void monk_t::stagger_damage_changed()
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
  if ( dot && dot->is_ticking() )
  {
    auto current_tick_dmg_per_max_health = current_stagger_tick_dmg() / resources.max[ RESOURCE_HEALTH ];
    sim->print_debug( "Stagger dmg: {} ({}%):", current_stagger_tick_dmg(), current_tick_dmg_per_max_health * 100.0 );
    if ( current_tick_dmg_per_max_health > 0.06 )
    {
      new_buff = buff.heavy_stagger;
      niuzao = 3;
    }
    else if ( current_tick_dmg_per_max_health > 0.03 )
    {
      new_buff = buff.moderate_stagger;
      niuzao = 2;
    }
    else if ( current_tick_dmg_per_max_health > 0.0 )
    {
      new_buff = buff.light_stagger;
      niuzao = 1;
    }
  }
  sim->print_debug( "Stagger new buff is {}.", new_buff ? new_buff->name() : "none" );

  if ( previous_buff && previous_buff != new_buff )
  {
    previous_buff->expire();
    if ( buff.training_of_niuzao->up() )
      buff.training_of_niuzao->expire();
  }
  if ( new_buff && previous_buff != new_buff )
  {
    new_buff->trigger();
    if ( azerite.training_of_niuzao.ok() )
      buff.training_of_niuzao->trigger( niuzao );
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

// monk_t::current_stagger_dmg_percent ==================================================

double monk_t::current_stagger_tick_dmg_percent()
{
  return current_stagger_tick_dmg() / resources.max[ RESOURCE_HEALTH ];
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

// monk_t::create_expression ==================================================

expr_t* monk_t::create_expression( const std::string& name_str )
{
  std::vector<std::string> splits = util::string_split( name_str, "." );
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

      virtual double evaluate() override
      {
        return player.current_stagger_tick_dmg_percent() > stagger_health_pct;
      }
    };
    struct stagger_amount_expr_t : public expr_t
    {
      monk_t& player;
      stagger_amount_expr_t( monk_t& p ) : expr_t( "stagger_amount" ), player( p )
      {
      }

      virtual double evaluate() override
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

      virtual double evaluate() override
      {
        return player.current_stagger_tick_dmg_percent() * 100;
      }
    };

    if ( splits[ 1 ] == "light" )
      return new stagger_threshold_expr_t( *this, light_stagger_threshold );
    else if ( splits[ 1 ] == "moderate" )
      return new stagger_threshold_expr_t( *this, moderate_stagger_threshold );
    else if ( splits[ 1 ] == "heavy" )
      return new stagger_threshold_expr_t( *this, heavy_stagger_threshold );
    else if ( splits[ 1 ] == "amount" )
      return new stagger_amount_expr_t( *this );
    else if ( splits[ 1 ] == "pct" )
      return new stagger_percent_expr_t( *this );
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
      std::vector<std::string> parts = util::string_split( splits[ 1 ], "_" );
      int n                          = std::stoi( parts.back() );

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

      virtual double evaluate() override
      {
        return player.mark_of_the_crane_counter();
      }
    };

    if ( splits[ 1 ] == "count" )
      return new sck_stack_expr_t( *this );
  }

  return base_t::create_expression( name_str );
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

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    if ( p.specialization() == MONK_BREWMASTER )
    {
      double stagger_tick_dmg  = p.sample_datas.stagger_tick_damage->sum();
      double purified_dmg      = p.sample_datas.purified_damage->sum();
      double stagger_total_dmg = stagger_tick_dmg + purified_dmg;

      os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
         << "\t\t\t\t\t<h3 class=\"toggle open\">Stagger Analysis</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      os << "\t\t\t\t\t\t<p style=\"color: red;\">This section is a work in progress</p>\n";

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger base Unbuffed: {} Raid Buffed: {}</p>\n", p.stagger_base_value(),
                  p.sample_datas.buffed_stagger_base );

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger pct (player level) Unbuffed: {:.2f}% Raid Buffed: {:.2f}%</p>\n",
                  100.0 * p.stagger_pct( p.level() ), 100.0 * p.sample_datas.buffed_stagger_pct_player_level );

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger pct (target level) Unbuffed: {:.2f}% Raid Buffed: {:.2f}%</p>\n",
                  100.0 * p.stagger_pct( p.target->level() ), 100.0 * p.sample_datas.buffed_stagger_pct_target_level );

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
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124255\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/ability_rogue_cheatdeath.gif) 0% "
            "50% no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( p.sample_datas.stagger_tick_damage->mean() / 60 ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << p.sample_datas.stagger_tick_damage->count()
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Light Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124275\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra_green.gif) 0% "
            "50% no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Light Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( p.sample_datas.light_stagger_total_damage->mean() / 60 ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << p.sample_datas.light_stagger_total_damage->count() << "</td>\n";
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
         << ( p.sample_datas.moderate_stagger_total_damage->mean() / 60 ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << p.sample_datas.moderate_stagger_total_damage->count() << "</td>\n";
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
         << ( p.sample_datas.heavy_stagger_total_damage->mean() / 60 ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << p.sample_datas.heavy_stagger_total_damage->count() << "</td>\n";
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

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto p              = new monk_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new monk_report_t( *p ) );
    return p;
  }
  virtual bool valid() const override
  {
    return true;
  }

  virtual void static_init() const override
  {
    items::init();
  }

  virtual void register_hotfixes() const override
  {
    /*    hotfix::register_effect( "Monk", "2018-07-14", "Fists of Fury increased by 18.5%.", 303680 )
          .field( "ap_coeff" )
          .operation( hotfix::HOTFIX_MUL)
          .modifier( 1.185 )
          .verification_value( 0.94185 );
        hotfix::register_effect( "Monk", "2017-03-29", "Split Personality cooldown reduction increased to 5 seconds per
       rank (was 3 seconds per rank). [SEF]", 739336) .field( "base_value" ) .operation( hotfix::HOTFIX_SET ) .modifier(
       -5000 ) .verification_value( -3000 ); hotfix::register_effect( "Monk", "2017-03-30", "Split Personality cooldown
       reduction increased to 5 seconds per rank (was 3 seconds per rank). [Serentiy]", 739336) .field( "base_value" )
          .operation( hotfix::HOTFIX_SET )
          .modifier( -5000 )
          .verification_value( -3000 );
    */
  }

  virtual void init( player_t* p ) const override
  {
    p->buffs.windwalking_movement_aura =
        make_buff( p, "windwalking_movement_aura", p->find_spell( 166646 ) )->add_invalidate( CACHE_RUN_SPEED );
  }
  virtual void combat_begin( sim_t* ) const override
  {
  }
  virtual void combat_end( sim_t* ) const override
  {
  }
};
}  // UNNAMED NAMESPACE

const module_t* module_t::monk()
{
  static monk_module_t m;
  return &m;
}

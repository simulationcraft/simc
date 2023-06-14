// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "event_manager.hpp"
#include "player/gear_stats.hpp"
#include "progress_bar.hpp"
#include "sim_ostream.hpp"
#include "sim/option.hpp"
#include "util/concurrency.hpp"
#include "util/rng.hpp"
#include "util/sample_data.hpp"
#include "util/util.hpp"
#include "util/vector_with_callback.hpp"

#include <map>
#include <memory>

struct actor_target_data_t;
struct buff_t;
struct cooldown_t;
class dbc_t;
class dbc_override_t;
struct expr_t;
namespace highchart {
    struct chart_t;
}
struct iteration_data_entry_t;
struct option_t;
struct plot_t;
struct raid_event_t;
struct reforge_plot_t;
struct scale_factor_control_t;
struct sim_control_t;
struct spell_data_expr_t;
struct spell_data_t;
struct work_queue_t;

namespace report::json
{
class report_configuration_t;
}

namespace profileset{
  class profilesets_t;
}

struct sim_progress_t
{
  int current_iterations;
  int total_iterations;
  double pct() const
  { return std::min( 1.0, current_iterations / static_cast<double>(total_iterations) ); }
};

/// Simulation engine
struct sim_t : private sc_thread_t
{
  event_manager_t event_mgr;

  // Output
  sim_ostream_t out_log;
  sim_ostream_t out_debug;
  bool debug;

  /**
   * Error on unknown options (default=false)
   *
   * By default Simulationcraft will ignore unknown sim, player, item, or action-scope options.
   * Enable this to hard-fail the simulator option parsing if an unknown option name is used for a
   * given scope.
   **/
  bool strict_parsing;
  bool canceled;
  // Clean up memory for threads after iterating (defaults to no in normal operation, some options
  // will force-enable the option)
  bool cleanup_threads;
  bool initialized;
  bool fixed_time;
  bool save_profiles;
  bool save_profile_with_actions;  // When saving full profiles, include actions or not
  bool default_actions;

  // Iteration Controls
  timespan_t max_time, expected_iteration_time;
  double vary_combat_length;
  int current_iteration, iterations;
  double target_error;
  role_e target_error_role;
  double current_error;
  double current_mean;
  int analyze_error_interval, analyze_number;

  sim_control_t* control;
  sim_t*      parent;
  player_t*   target;
  player_t*   heal_target;
  vector_with_callback<player_t*> target_list;
  vector_with_callback<player_t*> target_non_sleeping_list;
  vector_with_callback<player_t*> player_list;
  vector_with_callback<player_t*> player_no_pet_list;
  vector_with_callback<player_t*> player_non_sleeping_list;
  vector_with_callback<player_t*> healing_no_pet_list;
  vector_with_callback<player_t*> healing_pet_list;
  player_t*   active_player;
  size_t      current_index; // Current active player
  int         num_players;
  int         num_enemies;
  int         num_tanks;
  int         enemy_targets;
  int         healing; // Creates healing targets. Useful for ferals, I guess.
  int global_spawn_index;
  int         max_player_level;
  timespan_t  queue_lag, queue_lag_stddev;
  timespan_t  gcd_lag, gcd_lag_stddev;
  timespan_t  channel_lag, channel_lag_stddev;
  timespan_t  queue_gcd_reduction;
  timespan_t  default_cooldown_tolerance;
  bool         strict_gcd_queue;
  double      confidence, confidence_estimator;
  // Latency
  timespan_t  world_lag, world_lag_stddev;
  double      travel_variance, default_skill;
  timespan_t  reaction_time, regen_periodicity;
  timespan_t  ignite_sampling_delta;
  int         optimize_expressions;
  int         optimize_expressions_rounds;
  int         current_slot;
  int         optimal_raid, log, debug_each;
  std::vector<uint64_t> debug_seed;
  stat_e      normalized_stat;
  std::string current_name, default_region_str, default_server_str, save_prefix_str, save_suffix_str;
  bool         save_talent_str;
  talent_format talent_input_format;
  auto_dispose< std::vector<player_t*> > actor_list;
  std::string main_target_str;
  int         stat_cache;
  int         max_aoe_enemies;
  double      tmi_window_global;
  double      tmi_bin_size;
  bool        show_etmi;
  bool        requires_regen_event;
  bool        single_actor_batch;
  bool        allow_experimental_specializations;
  int         progressbar_type;
  int         armory_retries;

  // Target options
  double      enemy_death_pct;
  int         rel_target_level, target_level;
  std::string target_race;
  int         target_adds;
  std::string sim_progress_base_str, sim_progress_phase_str;
  int         desired_targets; // desired number of targets


  // Data access
  std::unique_ptr<dbc_t> dbc;
  std::unique_ptr<dbc_override_t> dbc_override;

  // Default stat enchants
  gear_stats_t enchant;

  int timewalk;
  int scale_to_itemlevel; //itemlevel to scale to. if -1, we don't scale down
  int keystone_level; // keystone difficulty for scaling purposes
  int keystone_pct_hp;     // keystone mob health percent
  bool dungeon_route_smart_targeting;            // sets whether the list of mobs will be sorted by their hp
  bool challenge_mode; // if active, players will get scaled down to 620 and set bonuses are deactivated
  bool scale_itemlevel_down_only; // Items below the value of scale_to_itemlevel will not be scaled up.
  bool disable_set_bonuses; // Disables all set bonuses.
  bool enable_taunts;
  bool use_item_verification;  // Disable use-item action verification in the simulator
  unsigned int disable_2_set; // Disables all 2 set bonuses for this tier/integer that this is set as
  unsigned int disable_4_set; // Disables all 4 set bonuses for this tier/integer that this is set as
  unsigned int enable_2_set;// Enables all 2 set bonuses for the tier/integer that this is set as
  unsigned int enable_4_set; // Enables all 4 set bonuses for the tier/integer that this is set as
  const spell_data_t* pvp_rules; // Hidden aura that contains the PvP crit damage reduction
  bool pvp_mode; // Enables PvP mode - reduces crit damage, adjusts PvP gear iLvl
  bool auto_attacks_always_land; /// Allow Auto Attacks (white attacks) to always hit the enemy
  bool log_spell_id; // Add spell data ids to log/debug output where available. (actions, buffs)

  // Actor tracking
  int active_enemies;
  int active_allies;

  std::vector<std::unique_ptr<option_t>> options;
  std::vector<std::string> party_encoding;
  std::vector<std::string> item_db_sources;

  // Random Number Generation
  rng::rng_t _rng;
  uint64_t seed;
  int deterministic;
  int strict_work_queue;
  int average_range, average_gauss;

  // Raid Events
  std::vector<std::unique_ptr<raid_event_t>> raid_events;
  std::string raid_events_str;
  fight_style_e fight_style;
  size_t add_waves;

  // Buffs and Debuffs Overrides
  struct overrides_t
  {
    // Buff overrides
    int arcane_intellect;
    int battle_shout;
    int mark_of_the_wild;
    int power_word_fortitude;
    int windfury_totem;

    // Debuff overrides
    int chaos_brand;
    int mystic_touch;
    int mortal_wounds;
    int bleeding;

    // Misc stuff needs resolving
    int    bloodlust;
    std::vector<uint64_t> target_health;
  } overrides;

  struct auras_t
  {
    buff_t* fallback; // generic global fallback buff
    buff_t* arcane_intellect;
    buff_t* battle_shout;
    buff_t* mark_of_the_wild;
    buff_t* power_word_fortitude;
  } auras;

  // Expansion specific custom parameters. Defaults in the constructor.
  struct legion_opt_t
  {
    // Legion
    int                 infernal_cinders_users = 1;
    int                 engine_of_eradication_orbs = 4;
    int                 void_stalkers_contract_targets = -1;
    double              specter_of_betrayal_overlap = 1.0;
    std::vector<double> cradle_of_anguish_resets;
  } legion_opts;

  struct bfa_opt_t
  {
    /// Chance to spawn the rare droplet
    double              secrets_of_the_deep_chance = 0.1; // TODO: Guessed, needs validation
    /// Chance that the player collects the droplet, defaults to always
    double              secrets_of_the_deep_collect_chance = 1.0;
    /// Gutripper base RPPM when target is above 30%
    double              gutripper_default_rppm = 2.0;
    /// Chance to pick up visage spawned by Seductive Power
    double              seductive_power_pickup_chance = 1.0;
    /// Treacherous Covenant update period.
    timespan_t          covenant_period = 1.0_s;
    /// Chance to gain the buff on each Treacherous Covenant update.
    double              covenant_chance = 1.0;
    /// Chance to gain a stack of Incandescent Sliver each time it ticks.
    double              incandescent_sliver_chance = 1.0;
    /// Fight or Flight proc attempt period
    timespan_t          fight_or_flight_period = 1.0_s;
    /// Chance to gain the buff on each Fight or Flight attempt
    double              fight_or_flight_chance = 0.0;
    /// Chance of being silenced by Harbinger's Inscrutable Will projectile
    double              harbingers_inscrutable_will_silence_chance = 0.0;
    /// Chance avoiding Harbinger's Inscrutable Will projectile by moving
    double              harbingers_inscrutable_will_move_chance = 1.0;
    /// Chance player is above 60% HP for Leggings of the Aberrant Tidesage damage proc
    double              aberrant_tidesage_damage_chance = 1.0;
    /// Chance player is above 90% HP for Fa'thuul's Floodguards damage proc
    double              fathuuls_floodguards_damage_chance = 1.0;
    /// Chance player is above 90% HP for Grips of Forgotten Sanity damage proc
    double              grips_of_forsaken_sanity_damage_chance = 1.0;
    /// Chance player takes damage and loses Untouchable from Stormglide Steps
    double              stormglide_steps_take_damage_chance = 0.0;
    /// Duration of the Lurker's Insidious Gift buff, the player can cancel it early to avoid unnecessary damage. 0 = full duration
    timespan_t          lurkers_insidious_gift_duration = 0_ms;
    /// Expected duration (in seconds) of shield from Abyssal Speaker's Gauntlets. 0 = full duration
    timespan_t          abyssal_speakers_gauntlets_shield_duration = 0_ms;
    /// Expected duration of the absorb provided by Trident of Deep Ocean. 0 = full duration
    timespan_t          trident_of_deep_ocean_duration = 0_ms;
    /// Chance that the player has a higher health percentage than the target for Legplates of Unbound Anguish proc
    double              legplates_of_unbound_anguish_chance = 1.0;
    /// Period to check for if an ally dies with Loyal to the End
    timespan_t          loyal_to_the_end_ally_death_timer = 60_s;
    /// Chance on every check to see if an ally dies with Loyal to the End
    double              loyal_to_the_end_ally_death_chance = 0.0;
    /// Number of allies with the Loyal to the End azerite trait, default = 4 (max)
    int                 loyal_to_the_end_allies = 0;
    /// Number of allies also using the Worldvein Resonance minor
    int                 worldvein_allies = 0;
    /// Chance to proc Reality Shift (normally triggers on moving specific distance)
    double              ripple_in_space_proc_chance = 0.0;
    /// Chance to be in range to hit with Blood of the Enemy major power (12 yd PBAoE)
    double              blood_of_the_enemy_in_range = 1.0;
    /// Period to check for if Undulating Tides gets locked out
    timespan_t          undulating_tides_lockout_timer = 60_s;
    /// Chance on every check to see if Undulating Tides gets locked out
    double              undulating_tides_lockout_chance = 0.0;
    /// Base RPPM for Leviathan's Lure
    double              leviathans_lure_base_rppm = 0.75;
    /// Chance to catch returning wave of Aquipotent Nautilus
    double              aquipotent_nautilus_catch_chance = 1.0;
    /// Chance of having to interrupt casting by moving to void tear from Za'qul's Portal Key
    double              zaquls_portal_key_move_chance = 0.0;
    /// Unleash stacked potency from Anu-Azshara, Staff of the Eternal after X seconds
    timespan_t          anuazshara_unleash_time = 0_ms;
    /// Storm of the Eternal haste and crit stat split ratio.
    double              storm_of_the_eternal_ratio = 0.05;
    /// How long before combat to start channeling Azshara's Font of Power
    timespan_t          font_of_power_precombat_channel = 0_ms;
    /// Average duration of buff in percentage
    double voidtwisted_titanshard_percent_duration = 0.5;
    /// Period between checking if surging vitality can proc
    timespan_t surging_vitality_damage_taken_period = 0_s;
    /// Allies that decrease crit buff when the trinket is used
    unsigned manifesto_allies_start = 0;
    /// Allies that increase vers buff when the first buff expires
    unsigned manifesto_allies_end = 5;
    /// Approximate interval in seconds between raid member major essence uses that trigger Symbiotic Presence.
    timespan_t symbiotic_presence_interval = 22_s;
    /// Percentage of Whispered Truths reductions to be applied to offensive spells.
    double whispered_truths_offensive_chance = 0.75;
    /// Initial stacks for Seductive Power buff
    int                 initial_seductive_power_stacks = 0;
    /// Number of allies affected by Jes' Howler buff
    unsigned            jes_howler_allies = 4;
    /// Initial stacks for Archive of the Titans
    int                 initial_archive_of_the_titans_stacks = 0;
    /// Hps done while using the Azerite Trait Arcane Heart
    unsigned            arcane_heart_hps = 0;
    /// Prepull spell cast count to assume.
    int                 subroutine_recalibration_precombat_stacks = 0;
    /// Additional spell cast count to assume each buff cycle.
    int                 subroutine_recalibration_dummy_casts = 0;
    /// Number of Reorigination array stats on the actors in the sim
    int                 reorigination_array_stacks = 0;
    /// Allow Reorigination Array to ignore scale factor stat changes (default false)
    bool                reorigination_array_ignore_scale_factors = false;
    /// Randomize Variable Intensity Gigavolt Oscillating Reactor start-of-combat oscillation
    bool                randomize_oscillation = true;
    /// Automatically use Oscillating Overload on max stack, true = yes if no use_item, 0 = no
    bool                auto_oscillating_overload = true;
    /// Is the actor in Zuldazar? Relevant for one of the set bonuses.
    bool                zuldazar = false;
    /// Whether the player is in Ny'alotha or not.
    bool nyalotha = true;
    /// Whether the player is in Nazjatar/Eternal Palace for various effects
    bool                nazjatar = true;
    /// Whether the Shiver Venom Crossbow/Lance should assume the target has the Shiver Venom debuff
    bool                shiver_venom = false;
  } bfa_opts;

  struct shadowlands_opt_t
  {
    /// Global enable/disable shadowlands systems
    bool enabled = false;
    /// Chance to catch each expelled sorrowful memory to extend the buff duration
    /// TODO: Set this to a reasonable value
    double combat_meditation_extend_chance = 1.0;
    /// Number of nearby allies & enemies for the pointed courage soulbind
    unsigned pointed_courage_nearby = 3;
    /// Number of nearby allies when you proc lead by example,
    /// the default value of -1 adjusts to 2 for ranged position and 4 for front/back position
    int lead_by_example_nearby = -1;
    /// Number of Stone Legionnaires in party (Stone Legion Heraldry trinket)
    unsigned stone_legionnaires_in_party = 0;
    /// Number of Crimson Choir in party (Cabalist's Effigy trinket)
    unsigned crimson_choir_in_party = 0;
    /// Seconds before combat to apply the Shattered Psyche buff to the player.
    timespan_t memory_of_past_sins_precast = 0_s;
    /// Chance for each target to be hit by a Judgment of the Arbiter arc
    double judgment_of_the_arbiter_arc_chance = 0.0;
    /// Type of corpse used for Volatile Solvent. Accepts corpse type string or buff string
    /// Corpse type: "humanoid", "beast", "dragonkin", "elemental", "giant"
    /// Buff type: "mastery", "primary", "crit", "magic", "physical"
    std::string volatile_solvent_type = "mastery";
    // Overrides the Unbound Changeling trinket to the given version.
    // The versions are given by the "all", "crit", "haste", and "mastery"
    // strings. Anything else will result in the item's bonus IDs being
    // used to determine which version the player is currently using.
    std::string unbound_changeling_stat_type = "default";
    /// Chance player is getting overhealed by Gluttonous Spike proc.
    double gluttonous_spike_overheal_chance = 1.0;
    /// Anima Field Emitter buff duration distribution, defaults to full duration.
    double anima_field_emitter_mean = std::numeric_limits<double>::max(),
           anima_field_emitter_stddev = 0.0;
    /// Retarget Shadowgrasp Totem if the use_item target demises after this many seconds
    timespan_t retarget_shadowgrasp_totem = 0_s;
    /// Sets the chance for the Inscrutable Quantum Device to give no stat buff outside Bloodlust
    double iqd_stat_fail_chance = 0.0;
    /// Sets chance that the actor gets the killing blow when a target demises for Thrill Seeker stacks
    /// The default value of -1.0 adjusts to 1/20 for most sims, and 1/4 for DungeonSlice sims
    double thrill_seeker_killing_blow_chance = -1.0;
    /// Percentage of default duration for the damage portion of Wild Hunt Tactics to use.
    double wild_hunt_tactics_duration_multiplier = 1.0;
    /// Chance bonded hearts will heal a member of the other covenants when grove invigoration procs
    double bonded_hearts_other_covenant_chance = 1.0;
    // Stat buff provided by Theotar's Party Favors soulbind (The Mad Duke's Tea buffs)
    // Buff Types: "primary", "haste", "crit", "versatility"
    std::string party_favor_type = "random";
    // Battlefield Presence enemy count override
    // Defaults to -1 to have the sim constantly adjust the value based on number of enemies in the sim
    int battlefield_presence_enemies = -1;
    /// Number of allies applying Shattered Psyche stacks to the enemy.
    unsigned shattered_psyche_allies = 0;
    /// Seconds before combat to apply the Shattered Psyche buff to the player.
    timespan_t salvaged_fusion_amplifier_precast = 0_s;
    /// Fraction of the time that the player is above the health threshold for Titanic Ocular Gland.
    double titanic_ocular_gland_worthy_chance = 1.0;
    /// Sets the chance that the player successfully faces their Doubt to get the Newfound Resolve buff.
    double newfound_resolve_success_chance = 1.0;
    /// Sets the default delay that the player waits before facing their Doubt.
    /// This is disabled if the APL creates the "newfound_resolve" action.
    timespan_t newfound_resolve_default_delay = 4_s;
    double newfound_resolve_delay_relstddev = 0.2;
    /// Seconds between damage/healing triggers for the Pustule Eruption soulbind, has a minimum 1s ICD
    timespan_t pustule_eruption_interval = 1_s;
    /// Chance that the player will pickup Shredded Soul orb left by Ebonsoul Vise
    double shredded_soul_pickup_chance = 1.0;
    /// Sets the average number of times per minute that the Valiant Strikes soulbind will attempt to heal a player.
    double valiant_strikes_heal_rate = 1.0;
    /// Type stat gained from So'leah's Secret Technique
    /// Buff type: "mastery", "haste", "crit", "versatility"
    std::string soleahs_secret_technique_type = "haste";
    /// How long before combat to start channeling Shadowed Orb of Torment
    timespan_t shadowed_orb_of_torment_precombat_channel = 0_ms;
    /// How often does the player fall below 20% HP for Reactive Defense Matrix?
    timespan_t reactive_defense_matrix_interval = 0_ms;
    /// Percentage of default duration for Field of Blossoms.
    double field_of_blossoms_duration_multiplier = 1.0;
    /// Modifier for Cruciform Veinripper to control uptime. When set to 0, proc rate is not affected.
    double cruciform_veinripper_proc_rate = 0.0;
    /// Modifier for Cruciform Veinripper to control uptime for tanks. When set to 0, proc rate is not affected unless position=front, in which case 0.4 is used.
    double cruciform_veinripper_in_front_rate = 0.0;
    /// How many pustules to generate during pre-combat, as events before the pull will normally remove a few
    unsigned int precombat_pustules = 9;
    // Prevents Soul Ignite from being used a second time to trigger the
    // AoE early. This results in the highest possible damage and the
    // player can alternatively trigger it early by canceling the buff.
    bool disable_soul_igniter_second_use = true;
    /// Disables the execute effect of Inscrutable Quantum Device since it is avoidable in game
    bool disable_iqd_execute = false;
    // Better Together Override
    // Defaults active
    bool better_together_ally = true;
    bool enable_rune_words = false;
    bool enable_domination_gems = false;
    // fleshcraft cancel delay from the_first_sigil
    timespan_t the_first_sigil_fleshcraft_cancel_time = 50_ms;
    // Earthbreaker's Impact weak points triggered
    unsigned int earthbreakers_impact_weak_points = 3;
    // Grim Eclipse Dot Duration override
    double grim_eclipse_dot_duration_multiplier = 1.0;
    // Percentage of default duration for Grim Eclipse haste buff. Set to 90% by default assuming about 1s of movement to get to the Event Horizon
    double grim_eclipse_buff_duration_multiplier = 0.9;
    // Automatically break the Chains of Domination trinket at maximum
    bool chains_of_domination_auto_break = true;
    // First Strike proc period
    double first_strike_chance = 0.0;
    // First Strike proc chance when the period is set
    timespan_t first_strike_period = 5_s;
    // Expected uptime of Adaptive Armor Fragment
    double adaptive_armor_fragment_uptime = 0.5;
    /// Percentage of default duration for Soothing Shade.
    double soothing_shade_duration_multiplier = 1.0;
  } shadowlands_opts;

  struct dragonflight_opt_t
  {
    // Average time before the shield from darkmoon deck: watcher is depleted
    timespan_t darkmoon_deck_watcher_deplete = 2_s;
    // Which roles in your group has a Whispering Incarnate Icon trinket
    std::string whispering_incarnate_icon_roles = "tank/heal/dps";
    // Chance for Decoration of Flame's AoE damage to miss a target
    double decoration_of_flame_miss_chance = 0.05;
    // Minimum time after alltotem of the master comes off cooldown that it will trigger again
    timespan_t alltotem_of_the_master_period = 3_s;
    // Number of balls spawned by Dragon Games Equipment the player will kick at the target
    unsigned int dragon_games_kicks = 3;
    // Minimum RNG multiplier for Dragon Games Equipment's number of kicks
    double dragon_games_rng = 0.75;
    // The Blessing the player will receive if they have Primal Ritual Shell trinket equiped (wind,stone,flame,sea)
    std::string primal_ritual_shell_blessing = "wind";
    // Number of nearby allies for the effect of Allied Wristguards of Companionship
    unsigned int allied_wristguards_allies = 4;
    // Chance for nearby enemies to move out of range for Allied Wristguards of Companionship
    double allied_wristguards_ally_leave_chance = 0.05;
    // Corrupting Rages Average Uptime
    double corrupting_rage_uptime = 0.80;
    // Hood of Surging Time proc chance when the period is set
    double hood_of_surging_time_chance = 0.0;
    // Hood of Surging Time proc period
    timespan_t hood_of_surging_time_period = 5_s;
    // Hood of Surging Time stacks to grant when the period is set
    unsigned int hood_of_surging_time_stacks = 1;
    // Ruby Whelp Shell training levels
    std::string ruby_whelp_shell_training = "";
    // A list of context-aware procs for Ruby Whelp Shell
    std::string ruby_whelp_shell_context = "";
    // Uptime of the mastery buff(Zone of Focus) of Blue Silken Lining (>90% HP)
    double blue_silken_lining_uptime = 0.7;
    // Interval between checking blue_silken_lining_uptime
    timespan_t blue_silken_lining_update_interval = 5_s;
    // Enable or Disable Seething Black Dragonscale's damage
    bool screaming_black_dragonscale_damage = false;
    // Period in which to try to trigger adapative Stonescales. Based on spell data, does not trigger on periodic damage.
    timespan_t adaptive_stonescales_period = 3_s;
  } dragonflight_opts;

  // Auras and De-Buffs
  auto_dispose<std::vector<buff_t*>> buff_list;

  // Global aura related delay
  timespan_t default_aura_delay;
  timespan_t default_aura_delay_stddev;

  auto_dispose<std::vector<cooldown_t*>> cooldown_list;

  /// Status of azerite-related effects
  azerite_control azerite_status;

  // Reporting
  progress_bar_t progress_bar;
  std::unique_ptr<scale_factor_control_t> scaling;
  std::unique_ptr<plot_t> plot;
  std::unique_ptr<reforge_plot_t> reforge_plot;
  chrono::cpu_clock::duration elapsed_cpu;
  chrono::wall_clock::duration elapsed_time;
  std::vector<size_t> work_per_thread;
  size_t work_done;
  double     iteration_dmg, priority_iteration_dmg,  iteration_heal, iteration_absorb;
  simple_sample_data_t raid_dps, total_dmg, raid_hps, total_heal, total_absorb, raid_aps;
  extended_sample_data_t simulation_length;
  chrono::wall_clock::duration merge_time, init_time, analyze_time;
  // Deterministic simulation iteration data collectors for specific iteration
  // replayability
  std::vector<iteration_data_entry_t> iteration_data, low_iteration_data, high_iteration_data;
  // Report percent (how many% of lowest/highest iterations reported, default 2.5%)
  double     report_iteration_data;
  // Minimum number of low/high iterations reported (default 5 of each)
  int        min_report_iteration_data;
  int        report_progress;
  int        bloodlust_percent;
  timespan_t bloodlust_time;
  std::string reference_player_str;
  std::vector<player_t*> players_by_dps;
  std::vector<player_t*> players_by_priority_dps;
  std::vector<player_t*> players_by_hps;
  std::vector<player_t*> players_by_hps_plus_aps;
  std::vector<player_t*> players_by_dtps;
  std::vector<player_t*> players_by_tmi;
  std::vector<player_t*> players_by_name;
  std::vector<player_t*> players_by_apm;
  std::vector<player_t*> players_by_variance;
  std::vector<player_t*> targets_by_name;
  std::vector<std::string> id_dictionary;
  std::map<double, std::vector<double> > divisor_timeline_cache;
  std::vector<report::json::report_configuration_t> json_reports;
  std::string output_file_str, html_file_str, json_file_str;
  std::string reforge_plot_output_file_str;
  std::vector<std::string> error_list;
  int display_build;
  int report_precision;
  int report_pets_separately;
  int report_targets;
  int report_details;
  int report_raw_abilities;
  int report_rng;
  int hosted_html;
  int save_raid_summary;
  int save_gear_comments;
  int statistics_level;
  int separate_stats_by_actions;
  int report_raid_summary;
  int buff_uptime_timeline;
  int buff_stack_uptime_timeline;
  bool json_full_states;
  int decorated_tooltips;

  int allow_potions;
  int allow_food;
  bool allow_flasks;
  int allow_augmentations;
  int solo_raid;
  bool maximize_reporting;
  std::string apikey, user_apitoken;
  bool distance_targeting_enabled;
  bool ignore_invulnerable_targets;
  bool enable_dps_healing;
  bool count_overheal_as_heal;
  double scaling_normalized;
  bool merge_enemy_priority_dmg;

  // Multi-Threading
  mutex_t merge_mutex;
  int threads;
  std::vector<sim_t*> children; // Manual delete!
  int thread_index;
  computer_process::priority_e process_priority;
  
  std::shared_ptr<work_queue_t> work_queue;

  // Related Simulations
  mutex_t relatives_mutex;
  std::vector<sim_t*> relatives;

  // Spell database access
  std::unique_ptr<spell_data_expr_t> spell_query;
  unsigned           spell_query_level;
  std::string        spell_query_xml_output_file_str;

  std::unique_ptr<mutex_t> pause_mutex; // External pause mutex, instantiated an external entity (in our case the GUI).
  bool paused;

  // Highcharts stuff

  // Vector of on-ready charts. These receive individual jQuery handlers in the HTML report (at the
  // end of the report) to load the highcharts into the target div.
  std::vector<std::string> on_ready_chart_data;

  // A map of highcharts data, added as a json object into the HTML report. JQuery installs handlers
  // to correct elements (toggled elements in the HTML report) based on the data.
  std::map<std::string, std::vector<std::string> > chart_data;

  bool chart_show_relative_difference;
  // Use the max metric actor as the relative difference base instead of the min
  bool relative_difference_from_max;
  // Which actor to use as the base for computing relative difference.
  std::string relative_difference_base;
  double chart_boxplot_percentile;

  // List of callbacks to call when an actor_target_data_t object is created. Currently used to
  // initialize the generic targetdata debuffs/dots we have.
  std::vector<std::function<void(actor_target_data_t*)> > target_data_initializer;

  bool display_hotfixes, disable_hotfixes;
  bool display_bonus_ids;

  // Profilesets
  opts::map_list_t profileset_map;
  std::vector<scale_metric_e> profileset_metric;
  std::vector<std::string> profileset_output_data;
  bool profileset_enabled;
  int profileset_work_threads, profileset_init_threads;
  std::unique_ptr<profileset::profilesets_t> profilesets;


  sim_t();
  sim_t( sim_t* parent, int thread_index = 0 );
  sim_t( sim_t* parent, int thread_index, sim_control_t* control );
  ~sim_t() override;

  void run() override;
  int       main( const std::vector<std::string>& args );
  double    iteration_time_adjust();
  double    expected_max_time() const;
  bool      is_canceled() const;
  void      cancel_iteration();
  void      cancel();
  void      interrupt();
  void      add_relative( sim_t* cousin );
  void      remove_relative( sim_t* cousin );
  sim_progress_t progress( std::string* detailed = nullptr, int index = -1 );
  double    progress( std::string& phase, std::string* detailed = nullptr, int index = -1 );
  void      detailed_progress( std::string*, int current_iterations, int total_iterations );
  void      datacollection_begin();
  void      datacollection_end();
  void      reset();
  void      check_actors();
  void      init_fight_style();
  void      init_parties();
  void      init_actors();
  void      init_actor( player_t* );
  void      init_actor_pets();
  void      init();
  void      analyze();
  void      merge( sim_t& other_sim );
  void      merge();
  bool      iterate();
  void      partition();
  bool      execute();
  void      analyze_error();
  void      analyze_iteration_data();
  void      print_options();
  void      add_option( std::unique_ptr<option_t> opt );
  void      create_options();
  bool      parse_option( const std::string& name, const std::string& value );
  void      setup( sim_control_t* );
  bool      time_to_think( timespan_t proc_time );
  player_t* find_player( util::string_view name ) const;
  player_t* find_player( int index ) const;
  cooldown_t* get_cooldown( util::string_view name );
  void      use_optimal_buffs_and_debuffs( int value );
  std::unique_ptr<expr_t>   create_expression( util::string_view name );
  /**
   * Create error with printf formatting.
   */
  template <typename... Args>
  void errorf( util::string_view format, Args&&... args )
  {
    if ( thread_index != 0 )
      return;

    set_error( fmt::sprintf( format, std::forward<Args>(args)... ) );
  }

  /**
   * Create error using fmt libraries python-like formatting syntax.
   */
  template <typename... Args>
  void error( fmt::format_string<Args...> format, Args&&... args )
  {
    if ( thread_index != 0 )
      return;

    set_error( fmt::vformat( format, fmt::make_format_args( std::forward<Args>(args)... ) ) );
  }

  void abort();
  void combat();
  void combat_begin();
  void combat_end();
  void add_chart_data( const highchart::chart_t& chart );
  bool has_raid_event( util::string_view type ) const;

  // Activates the necessary actor/actors before iteration begins.
  void activate_actors();

  void heartbeat_event_callback();
  std::vector<std::function<void(sim_t*)>> heartbeat_event_callback_function;
  void register_heartbeat_event_callback( std::function<void( sim_t*)> fn );

  timespan_t current_time() const
  { return event_mgr.current_time; }
  static double distribution_mean_error( const sim_t& s, const extended_sample_data_t& sd )
  { return s.confidence_estimator * sd.mean_std_dev; }
  void register_target_data_initializer(std::function<void(actor_target_data_t*)> cb)
  { target_data_initializer.push_back( cb ); }
  const rng::rng_t& rng() const
  { return _rng; }
  rng::rng_t& rng()
  { return _rng; }
  double averaged_range( double min, double max );

  // Thread id of this sim_t object
#ifndef SC_NO_THREADING
  std::thread::id thread_id() const
  { return sc_thread_t::thread_id(); }
#endif

  /**
   * Convenient stdout print function using python-like formatting.
   *
   * Print to stdout
   * Print using fmt libraries python-like formatting syntax.
   */

  /**
   * Convenient debug function using python-like formatting.
   *
   * Checks if sim debug is enabled.
   * Print using fmt libraries python-like formatting syntax.
   */
  template <typename... Args>
  void print_debug( fmt::format_string<Args...> format, Args&& ... args )
  {
    if ( ! debug )
      return;

    out_debug.vprint( format, fmt::make_format_args( std::forward<Args>(args)... ) );
  }

  /**
   * Convenient log function using python-like formatting.
   *
   * Checks if sim logging is enabled.
   * Print using fmt libraries python-like formatting syntax.
   */
  template <typename... Args>
  void print_log( fmt::format_string<Args...> format, Args&& ... args )
  {
    if ( ! log )
      return;

    out_log.vprint( format, fmt::make_format_args( std::forward<Args>(args)... ) );
  }

private:
  void set_error(std::string error);
  void do_pause();
  void print_spell_query();
  void enable_debug_seed();
  void disable_debug_seed();
  bool requires_cleanup() const;
};

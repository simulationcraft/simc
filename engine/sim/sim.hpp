// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "event_manager.hpp"
#include "player/gear_stats.hpp"
#include "progress_bar.hpp"
#include "profileset_control.hpp"
#include "sim_ostream.hpp"
#include "sim/option.hpp"
#include "util/concurrency.hpp"
#include "util/rng.hpp"
#include "util/sample_data.hpp"
#include "util/util.hpp"
#include "util/vector_with_callback.hpp"

#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_set>

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
  bool save_full_profile;  // save the full profile instead of only active save_e flags
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
  rng::truncated_gauss_t queue_lag, gcd_lag, channel_lag;
  timespan_t  queue_gcd_reduction;
  timespan_t  default_cooldown_tolerance;
  bool         strict_gcd_queue;
  double      confidence, confidence_estimator;
  // Latency
  rng::truncated_gauss_t world_lag;
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
  auto_dispose< std::vector<player_t*> > actor_list;
  std::string main_target_str;
  int         stat_cache;
  int         max_aoe_enemies;
  bool        requires_regen_event;
  bool        single_actor_batch;
  bool        allow_experimental_specializations;
  bool        enable_all_talents;
  bool        enable_all_sets;
  bool        enable_all_item_effects;
  int         progressbar_type;
  int         armory_retries;
  std::unordered_map<std::string, std::string> item_slot_overrides;

  // Target options
  double      enemy_death_pct;
  int         rel_target_level, target_level;
  std::string target_race;
  int         target_adds;
  std::string sim_progress_base_str, sim_progress_phase_str;
  int         desired_targets; // desired number of targets
  int         desired_tank_targets; // desired number of tank target dummy npcs


  // Data access
  std::unique_ptr<dbc_t> dbc;
  std::unique_ptr<dbc_override_t> dbc_override;

  // Default stat enchants
  gear_stats_t enchant;

  int timewalk;
  int scale_to_itemlevel; //itemlevel to scale to. if -1, we don't scale down
  bool dungeon_route_smart_targeting;            // sets whether the list of mobs will be sorted by their hp
  int dungeon_route_pct_hp; // the portion of full mob hp being used to sim an incomplete party
  int dungeon_route_key_level; // keystone difficulty level
  bool challenge_mode; // if active, players will get scaled down to 620 and set bonuses are deactivated
  bool scale_itemlevel_down_only; // Items below the value of scale_to_itemlevel will not be scaled up.
  bool disable_set_bonuses; // Disables all set bonuses.
  bool enable_taunts;
  bool use_item_verification;  // Disable use-item action verification in the simulator
  std::string disable_2_set; // Disables all 2 set bonuses for the tier that this is set as
  std::string disable_4_set; // Disables all 4 set bonuses for the tier that this is set as
  std::string enable_2_set;// Enables all 2 set bonuses for the tier that this is set as
  std::string enable_4_set; // Enables all 4 set bonuses for the tier that this is set as
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
    int blessing_of_the_bronze;
    int mark_of_the_wild;
    int power_word_fortitude;

    // Debuff overrides
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

  // Auras and De-Buffs
  auto_dispose<std::vector<buff_t*>> buff_list;

  // Global aura related delay
  rng::truncated_gauss_t default_aura_delay;

  auto_dispose<std::vector<cooldown_t*>> cooldown_list;

  // Reporting
  progress_bar_t progress_bar;
  std::unique_ptr<scale_factor_control_t> scaling;
  std::unique_ptr<plot_t> plot;
  std::unique_ptr<reforge_plot_t> reforge_plot;
  chrono::cpu_clock::duration elapsed_cpu;
  chrono::wall_clock::duration elapsed_time;
  std::vector<size_t> work_per_thread;
  size_t work_done;
  double iteration_dmg, priority_iteration_dmg, iteration_heal, iteration_absorb;
  simple_sample_data_t total_dmg, raid_hps, total_heal, total_absorb, raid_aps;
  extended_sample_data_t raid_dps, simulation_length;
  chrono::wall_clock::duration merge_time, init_time, analyze_time;
  // Deterministic simulation iteration data collectors for specific iteration
  // replayability
  std::vector<iteration_data_entry_t> iteration_data, low_iteration_data, high_iteration_data;
  // Report percent (how many% of lowest/highest iterations reported, default 2.5%)
  double report_iteration_data;
  // Minimum number of low/high iterations reported (default 5 of each)
  int min_report_iteration_data;
  // Report all iterations in strict thread & iteration order with no sorting. Takes precedence over above 2 options.
  bool report_strict_iteration_data;
  int report_progress;
  int bloodlust_percent;
  timespan_t bloodlust_time;
  std::string reference_player_str;
  std::vector<player_t*> players_by_dps;
  std::vector<player_t*> players_by_priority_dps;
  std::vector<player_t*> players_by_hps;
  std::vector<player_t*> players_by_hps_plus_aps;
  std::vector<player_t*> players_by_dtps;
  std::vector<player_t*> players_by_name;
  std::vector<player_t*> players_by_apm;
  std::vector<player_t*> players_by_variance;
  std::vector<player_t*> targets_by_name;
  std::vector<std::string> id_dictionary;
  std::map<double, std::vector<double> > divisor_timeline_cache;
  std::vector<report::json::report_configuration_t> json_reports;
  std::string output_file_str, html_file_str, json_file_str;
  std::string reforge_plot_output_file_str;
  std::map<error_level_e, std::unordered_set<std::string>> error_list;
  int display_build;  // 0: none, 1: normal (default), 2: version + hotfix only
  int report_precision;
  int report_pets_separately;
  int report_targets;
  int report_details;
  std::string report_merged_stats;
  bool full_damage_sources_chart;
  bool report_all_variables;
  bool collect_action_sequence;
  int report_rng;
  int hosted_html;
  int offline;
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
  double dhaps_healing_weight;
  double scaling_normalized;
  bool merge_enemy_priority_dmg;

  // sim control
  std::unordered_map<std::string, profileset_controller_t::factory_fn_pair_t> profileset_controller_factory;
  std::vector<std::unique_ptr<profileset_controller_t>> profileset_controller;
  std::deque<profileset_controller_data_wrapper_t> profileset_controller_data;
  opts::map_list_t profileset_controller_options;

  // Multi-Threading
  mutex_t merge_mutex;
  int threads;
  std::vector<sim_t*> children; // Manual delete!
  int thread_index;
  computer_process::priority_e process_priority;
  std::shared_ptr<work_queue_t> work_queue;
  std::vector<std::exception_ptr> exception_queue;
  mutex_t exception_mutex;

  // Related Simulations
  mutex_t relatives_mutex;
  std::vector<sim_t*> relatives;

  // Init mutex
  std::shared_mutex init_mutex;

  // Spell database access
  std::unique_ptr<spell_data_expr_t> spell_query;
  unsigned spell_query_level;
  std::string spell_query_xml_output_file_str;
  unsigned spell_query_wrap;

  std::unique_ptr<mutex_t> pause_mutex; // External pause mutex, instantiated an external entity (in our case the GUI).
  bool paused;

  // Highcharts stuff

  // A map of highcharts data, added as a json object into the HTML report. JQuery installs handlers
  // to correct elements (toggled elements in the HTML report) based on the data. Charts with no toggle
  // are grouped under the empty-string key and rendered as soon as their target div is in the DOM.
  std::map<std::string, std::vector<std::string> > chart_data;

  bool chart_show_relative_difference;
  bool chart_show_relative_difference_percent;
  // Use the max metric actor as the relative difference base instead of the min
  bool relative_difference_from_max;
  // Which actor to use as the base for computing relative difference.
  std::string relative_difference_base;
  double chart_boxplot_percentile;

  // List of callbacks to call when an actor_target_data_t object is created. Currently used to
  // initialize the generic targetdata debuffs/dots we have.
  std::vector<std::function<void( actor_target_data_t* )>> target_data_initializer;

  // Priority-based actor initialization callbacks. Each callback is run on the player object during init_actor() in
  // priority order, and additional callbacks can be inserted at any point from external modules.
  std::vector<std::tuple<int, std::function<void( player_t* )>, std::string>> actor_initializer;

  bool display_hotfixes, disable_hotfixes;
  bool display_bonus_ids;

  // Profilesets
  opts::map_list_t profileset_map;
  unsigned profileset_main_actor_index;
  unsigned profileset_report_player_index;
  std::string profileset_multiactor_base_name;
  std::vector<scale_metric_e> profileset_metric;
  std::vector<std::string> profileset_output_data;
  bool profileset_enabled;
  int profileset_work_threads, profileset_init_threads;
  std::unique_ptr<profileset::profilesets_t> profilesets;
  std::string_view profileset_name;

  sim_t();
  explicit sim_t( sim_t* parent, int thread_index = 0, std::string_view profileset_name = {} );
  sim_t( sim_t* parent, int thread_index, sim_control_t* control, std::string_view profileset_name = {} );
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

  bool is_initialized()
  {
    init_mutex.lock_shared();
    auto i = initialized;
    init_mutex.unlock_shared();

    return i;
  }

  /**
   * Create error with printf formatting.
   */
  template <typename... Args>
  void errorf( error_level_e level, std::string_view format, Args&&... args )
  {
    if ( thread_index != 0 )
      return;

    set_error( level, fmt::sprintf( format, std::forward<Args>(args)... ) );
  }

  template <typename... Args>
  void errorf( std::string_view format, Args&&... args )
  {
    if ( thread_index != 0 )
      return;

    set_error( error_level_e::TRIVIAL, fmt::sprintf( format, std::forward<Args>(args)... ) );
  }

  /**
   * Create error using fmt libraries python-like formatting syntax.
   */
  template <typename... Args>
  void error( error_level_e level, fmt::format_string<Args...> format, Args&&... args )
  {
    if ( thread_index != 0 )
      return;

    set_error( level, fmt::vformat( format, fmt::make_format_args( args... ) ) );
  }

  template <typename... Args>
  void error( fmt::format_string<Args...> format, Args&&... args )
  {
    if ( thread_index != 0 )
      return;

    set_error( error_level_e::TRIVIAL, fmt::vformat( format, fmt::make_format_args( args... ) ) );
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
  std::vector<std::function<void( sim_t* )>> heartbeat_event_callback_function;
  void register_heartbeat_event_callback( std::function<void( sim_t*)> fn );

  void register_target_data_initializer( std::function<void( actor_target_data_t* )> fn );

  void register_actor_initializers();
  // Register a callback
  void register_actor_initializer( int priority, std::function<void( player_t* )> fn, std::string name = "" );
  // Register a player_t member function
  void register_actor_initializer( int priority, void ( player_t::*fn )(), std::string name = "" );
  // Register with an offset from another named initializer
  void register_actor_initializer( std::string_view base, int offset, std::function<void( player_t* )> fn,
                                   std::string name = "" );
  // Register a player_t member function with an offset
  void register_actor_initializer( std::string_view base, int offset, void ( player_t::*fn )(), std::string name = "" );

  timespan_t current_time() const
  { return event_mgr.current_time; }
  static double distribution_mean_error( const sim_t& s, const extended_sample_data_t& sd )
  { return s.confidence_estimator * sd.mean_std_dev; }
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

    out_debug.vprint( format, fmt::make_format_args( args... ) );
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

    out_log.vprint( format, fmt::make_format_args( args... ) );
  }

  bool rethrow_exception_queue();

private:
  void set_error( error_level_e level, std::string error );
  void do_pause();
  void print_spell_query();
  void enable_debug_seed();
  void disable_debug_seed();
  bool requires_cleanup() const;
};

template <typename T>
data_wrapper_t<T> profileset_controller_t::get_data()
{
  auto& pcd = parent->profileset_controller_data;
  assert( pcd.size() > id );
  auto& data = pcd[ id ];
  return { *data.data.get(), data.mutex };
}

template <typename T>
void profileset_controller_t::set_data( T&& data )
{
  auto& pcd = parent->profileset_controller_data;
  assert( pcd.size() > id );
  pcd[ id ].data = std::make_unique<T>( data );
}

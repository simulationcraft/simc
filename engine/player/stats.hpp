// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "util/timespan.hpp"
#include "util/sample_data.hpp"
#include "util/string_view.hpp"
#include "sim/gain.hpp"
#include "player/gear_stats.hpp"

#include <vector>
#include <string>

struct action_t;
struct player_t;
struct sim_t;
struct sc_timeline_t;

struct stats_t : private noncopyable
{
private:
  sim_t& sim;
public:
  const std::string name_str;
  player_t* player;
  stats_t* parent;
  // We should make school and type const or const-like, and either stricly define when, where and who defines the values,
  // or make sure that it is equal to the value of all it's actions.
  school_e school;
  stats_e type;

  std::vector<action_t*> action_list;
  gain_t resource_gain;
  // Flags
  bool analyzed;
  bool quiet;
  bool background;

  simple_sample_data_t num_executes, num_ticks, num_refreshes, num_direct_results, num_tick_results;
  unsigned int iteration_num_executes, iteration_num_ticks, iteration_num_refreshes;
  // Variables used both during combat and for reporting
  simple_sample_data_t total_execute_time, total_tick_time;
  timespan_t iteration_total_execute_time, iteration_total_tick_time;
  double portion_amount;
  simple_sample_data_t total_intervals;
  timespan_t last_execute;
  extended_sample_data_t actual_amount, total_amount, portion_aps, portion_apse;
  std::vector<stats_t*> children;

  struct stats_results_t
  {
  public:
    simple_sample_data_with_min_max_t actual_amount, avg_actual_amount, count;
    simple_sample_data_t total_amount, fight_actual_amount, fight_total_amount, overkill_pct;
    double pct;
  private:
    int iteration_count;
    double iteration_actual_amount, iteration_total_amount;
    friend struct stats_t;
  public:

    stats_results_t();
    void analyze( double num_results );
    void merge( const stats_results_t& other );
    void datacollection_begin();
    void datacollection_end();
  };
  std::array<stats_results_t,FULLTYPE_MAX> direct_results;
  std::array<stats_results_t,RESULT_MAX> tick_results;

  // Reporting only
  std::array<double, RESOURCE_MAX> resource_portion, apr, rpe;
  double rpe_sum, compound_amount, overkill_pct;
  double aps, ape, apet, etpe, ttpt;
  timespan_t total_time;
  std::string aps_distribution_chart;

  std::string timeline_aps_chart;

  struct stats_scaling_t {
    gear_stats_t value;
    gear_stats_t error;
  };
  std::unique_ptr<stats_scaling_t> scaling;
  std::unique_ptr<sc_timeline_t> timeline_amount;

  stats_t( util::string_view name, player_t* );

  void add_child( stats_t* child );
  void consume_resource( resource_e resource_type, double resource_amount );
  full_result_e translate_result( result_e result, block_result_e block_result );
  void add_result( double act_amount, double tot_amount, result_amount_type dmg_type, result_e result, block_result_e block_result, player_t* target );
  void add_execute( timespan_t time, player_t* target );
  void add_tick   ( timespan_t time, player_t* target );
  void add_refresh( player_t* target );
  void datacollection_begin();
  void datacollection_end();
  void reset();
  void analyze();
  void merge( const stats_t& other );
  const std::string& name() const { return name_str; }

  bool has_direct_amount_results() const;
  bool has_tick_amount_results() const;
};
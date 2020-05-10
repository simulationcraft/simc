// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <string>

struct sim_progress_t;
struct sim_t;

struct progress_bar_t
{
  sim_t& sim;
  int steps, updates, interval, update_number;
  double start_time, last_update, max_interval_time;
  std::string status;
  std::string base_str, phase_str;
  size_t work_index, total_work_;
  double elapsed_time;
  size_t time_count;

  progress_bar_t( sim_t& s );
  void init();
  bool update( bool finished = false, int index = -1 );
  void output( bool finished = false );
  void restart();
  void progress();
  void set_base( const std::string& base );
  void set_phase( const std::string& phase );
  void add_simulation_time( double t );
  size_t current_progress() const;
  size_t total_work() const;
  double average_simulation_time() const;

  static std::string format_time( double t );
private:
  size_t compute_total_phases();
  bool update_simple( const sim_progress_t&, bool finished, int index );
  bool update_normal( const sim_progress_t&, bool finished, int index );

  size_t n_stat_scaling_players( const std::string& stat ) const;
  // Compute the number of various option-related phases
  size_t n_plot_phases() const;
  size_t n_reforge_plot_phases() const;
  size_t n_scale_factor_phases() const;
};
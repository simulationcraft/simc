// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "sc_enums.hpp"

#include <string>
#include <unordered_map>

struct sim_t;

struct plot_t
{
public:
  sim_t* sim;
  std::unordered_map<stat_e, double> dps_plot_stats;
  double dps_plot_step;
  int dps_plot_points;
  int dps_plot_iterations;
  double dps_plot_target_error;
  int dps_plot_debug;
  stat_e current_plot_stat;
  int num_plot_stats, remaining_plot_stats, remaining_plot_points;
  bool dps_plot_positive, dps_plot_negative;
  bool dps_plot_display_delta;

  plot_t( sim_t* s );
  void initialize();
  void analyze();
  double progress( std::string& phase, std::string* detailed = nullptr );

private:
  void analyze_stats();
  void write_output_file();
  void create_options();
};

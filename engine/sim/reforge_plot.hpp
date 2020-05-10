// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include <vector>
#include <string>

struct sim_t;

struct reforge_plot_t
{
  sim_t* sim;
  sim_t* current_reforge_sim;
  std::string reforge_plot_stat_str;
  std::vector<stat_e> reforge_plot_stat_indices;
  int    reforge_plot_step;
  int    reforge_plot_amount;
  int    reforge_plot_iterations;
  double reforge_plot_target_error;
  int    reforge_plot_debug;
  int    current_stat_combo;
  int    num_stat_combos;

  reforge_plot_t( sim_t* s );

  void generate_stat_mods( std::vector<std::vector<int> > &stat_mods,
                           const std::vector<stat_e> &stat_indices,
                           int cur_mod_stat,
                           std::vector<int> cur_stat_mods );
  void analyze();
  void analyze_stats();
  double progress( std::string& phase, std::string* detailed = nullptr );
private:
  void write_output_file();
  void create_options();
};
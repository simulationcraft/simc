// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_highchart.hpp"
#include "sc_enums.hpp"
#include <vector>
#include <string>

struct player_t;
struct sim_t;

namespace chart
{
  // Highcharts stuff
  bool generate_raid_gear(highchart::bar_chart_t&, const sim_t&);
  bool generate_raid_downtime(highchart::bar_chart_t&, const sim_t&);
  bool generate_raid_aps(highchart::bar_chart_t&, const sim_t&, const std::string& type);
  bool generate_distribution(highchart::histogram_chart_t&, const player_t* p, const std::vector<size_t>& dist_data,
    const std::string& distribution_name, double avg, double min, double max,
    bool percent = false);
  bool generate_gains(highchart::pie_chart_t&, const player_t&, resource_e);
  bool generate_spent_time(highchart::pie_chart_t&, const player_t&);
  bool generate_stats_sources(highchart::pie_chart_t&, const player_t&, const std::string& title,
    const std::vector<stats_t*>& stats_list);
  bool generate_damage_stats_sources(highchart::pie_chart_t&, const player_t&);
  bool generate_heal_stats_sources(highchart::pie_chart_t&, const player_t&);
  bool generate_raid_dpet(highchart::bar_chart_t&, const sim_t&);
  bool generate_action_dpet(highchart::bar_chart_t&, const player_t&);
  bool generate_apet(highchart::bar_chart_t&, const std::vector<stats_t*>&);
  highchart::time_series_t& generate_stats_timeline(highchart::time_series_t&, const stats_t&);
  highchart::time_series_t& generate_actor_timeline(highchart::time_series_t&, const player_t& p,
    const std::string& attribute, const std::string& series_color,
    const sc_timeline_t& data);
  bool generate_actor_dps_series(highchart::time_series_t& series, const player_t& p);
  bool generate_scale_factors( highchart::bar_chart_t& chart, const player_t& p, scale_metric_e metric );
  bool generate_scaling_plot( highchart::chart_t& chart, const player_t& p, scale_metric_e metric );
  bool generate_reforge_plot( highchart::chart_t& chart, const player_t& p );
}  // namespace chart

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "sc_enums.hpp"
#include "util/span.hpp"
#include "color.hpp"

#include <string_view>
#include <vector>

struct player_t;
struct sim_t;
struct stats_t;
struct sc_timeline_t;

namespace profileset
{
class profile_set_t;
}

namespace highchart
{
struct chart_t;
struct bar_chart_t;
struct histogram_chart_t;
struct pie_chart_t;
struct time_series_t;
}  // namespace highchart

namespace chart
{
  // Highcharts stuff
  bool generate_raid_gear(highchart::bar_chart_t&, const sim_t&);
  bool generate_raid_downtime(highchart::bar_chart_t&, const sim_t&);
  bool generate_raid_aps(highchart::bar_chart_t&, const sim_t&, std::string_view type);
  bool generate_distribution(highchart::histogram_chart_t&, const player_t* p, const std::vector<size_t>& dist_data,
    std::string_view distribution_name, double avg, double min, double max,
    bool percent = false);
  bool generate_gains(highchart::pie_chart_t&, const player_t&, resource_e);
  bool generate_spent_time(highchart::pie_chart_t&, const player_t&);
  bool generate_stats_sources(highchart::pie_chart_t&, const player_t&, std::string_view title,
    const std::vector<stats_t*>& stats_list);
  bool generate_damage_stats_sources(highchart::pie_chart_t&, const player_t&);
  bool generate_heal_stats_sources(highchart::pie_chart_t&, const player_t&);
  bool generate_raid_dpet(highchart::bar_chart_t&, const sim_t&);
  bool generate_action_dpet(highchart::bar_chart_t&, const player_t&);
  bool generate_apet(highchart::bar_chart_t&, const std::vector<stats_t*>&);
  highchart::time_series_t& generate_stats_timeline(highchart::time_series_t&, const stats_t&);
  highchart::time_series_t& generate_actor_timeline(highchart::time_series_t&, const player_t& p,
    std::string_view attribute, color::rgb series_color,
    const sc_timeline_t& data);
  bool generate_actor_dps_series(highchart::time_series_t& series, const player_t& p);
  bool generate_scale_factors( highchart::bar_chart_t& chart, const player_t& p, scale_metric_e metric );
  bool generate_scaling_plot( highchart::chart_t& chart, const player_t& p, scale_metric_e metric );
  bool generate_reforge_plot( highchart::chart_t& chart, const player_t& p );

  // Profilesets
  constexpr size_t MAX_PROFILESET_CHART_ENTRIES = 500;
  void generate_profilesets_chart( highchart::bar_chart_t& chart, const sim_t& sim, size_t chart_id,
                                   util::span<const profileset::profile_set_t*> results,
                                   util::span<const profileset::profile_set_t*> results_mean );
}  // namespace chart

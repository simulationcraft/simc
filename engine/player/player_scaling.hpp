// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/generic.hpp"
#include "player/gear_stats.hpp"
#include "sc_enums.hpp"
#include <array>
#include <vector>

struct player_scaling_t
{
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling;
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling_normalized;
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling_error;
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling_compare_error;
  std::array<double, SCALE_METRIC_MAX> scaling_lag, scaling_lag_error;
  std::array<bool, STAT_MAX> scales_with;
  std::array<double, STAT_MAX> over_cap;
  std::array<std::vector<stat_e>, SCALE_METRIC_MAX> scaling_stats; // sorting vector

  player_scaling_t()
  {
    range::fill( scaling_lag, 0 );
    range::fill( scaling_lag_error, 0 );
    range::fill( scales_with, false );
    range::fill( over_cap, 0 );
  }

  void enable( stat_e stat )
  { scales_with[ stat ] = true; }

  void disable( stat_e stat )
  { scales_with[ stat ] = false; }

  void set( stat_e stat, bool state )
  { scales_with[ stat ] = state; }
};
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include <array>
#include <string>

struct player_t;

namespace gear_weights
{
  std::array<std::string, SCALE_METRIC_MAX> wowhead(const player_t&);
  std::array<std::string, SCALE_METRIC_MAX> pawn(const player_t&);
  std::array<std::string, SCALE_METRIC_MAX> askmrrobot(const player_t&);
}  // gear_weights

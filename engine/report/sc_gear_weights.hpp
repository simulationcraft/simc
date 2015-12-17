// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <array>
#include <string>
#include "sc_enums.hpp"

struct player_t;

namespace gear_weights
{
/**
 * The website works, but the link we send out is not usable. If anyone ever
 * fixes it, just set this to 1.
 */
static const bool LOOTRANK_ENABLED = false;
std::array<std::string, SCALE_METRIC_MAX> lootrank( const player_t& );
std::array<std::string, SCALE_METRIC_MAX> wowhead( const player_t& );
std::array<std::string, SCALE_METRIC_MAX> pawn( const player_t& );
std::array<std::string, SCALE_METRIC_MAX> askmrrobot( const player_t& );
}

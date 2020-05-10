// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <string>

struct action_t;
struct player_t;

namespace consumable
{
  action_t* create_action(player_t*, const std::string& name, const std::string& options);
}
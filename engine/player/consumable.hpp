// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "util/string_view.hpp"

struct action_t;
struct player_t;

namespace consumable
{
  action_t* create_action(player_t*, util::string_view name, util::string_view options);
}
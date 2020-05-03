// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <string>
#include <vector>

// Simulation Setup =========================================================

struct player_description_t
{
  // Add just enough to describe a player

  // name, class, talents, gear, professions, actions explicitly stored
  std::string name;
  // etc

  // flesh out API, these functions cannot depend upon sim_t
  // ideally they remain static, but if not then move to sim_control_t
  static void load_bcp    ( player_description_t& /*etc*/ );
  static void load_wowhead( player_description_t& /*etc*/ );
};

struct combat_description_t
{
  std::string name;
  int target_seconds;
  std::string raid_events;
  // etc
};

struct sim_control_t
{
  combat_description_t combat;
  std::vector<player_description_t> players;
  option_db_t options;
};
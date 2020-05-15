// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include <array>
#include <string>
#include <vector>

struct buff_t;

struct player_processed_report_information_t
{
  bool generated = false;
  bool buff_lists_generated = false;
  std::array<std::string, SCALE_METRIC_MAX> gear_weights_wowhead_std_link, gear_weights_pawn_string, gear_weights_askmrrobot_link;
  std::string save_str;
  std::string save_gear_str;
  std::string save_talents_str;
  std::string save_actions_str;
  std::string comment_str;
  std::string thumbnail_url;
  std::string html_profile_str;
  std::vector<buff_t*> buff_list, dynamic_buffs, constant_buffs;

};
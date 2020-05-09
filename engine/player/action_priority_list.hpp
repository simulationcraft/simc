// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <string>
#include <cstdint>
#include <vector>

struct action_t;
struct player_t;
struct spell_data_t;

struct action_priority_t
{
  std::string action_;
  std::string comment_;

  action_priority_t(const std::string& a, const std::string& c) :
    action_(a), comment_(c)
  { }

  action_priority_t* comment(const std::string& c)
  {
    comment_ = c; return this;
  }
};

struct action_priority_list_t
{
  // Internal ID of the action list, used in conjunction with the "new"
  // call_action_list action, that allows for potential infinite loops in the
  // APL.
  unsigned internal_id;
  uint64_t internal_id_mask;
  std::string name_str;
  std::string action_list_comment_str;
  std::string action_list_str;
  std::vector<action_priority_t> action_list;
  player_t* player;
  bool used;
  std::vector<action_t*> foreground_action_list;
  std::vector<action_t*> off_gcd_actions;
  std::vector<action_t*> cast_while_casting_actions;
  int random; // Used to determine how faceroll something actually is. :D
  action_priority_list_t(std::string name, player_t* p, const std::string& list_comment = std::string()) :
    internal_id(0), internal_id_mask(0), name_str(name), action_list_comment_str(list_comment), player(p), used(false),
    foreground_action_list(), off_gcd_actions(), cast_while_casting_actions(), random(0)
  { }

  action_priority_t* add_action(const std::string& action_priority_str, const std::string& comment = std::string());
  action_priority_t* add_action(const player_t* p, const spell_data_t* s, const std::string& action_name,
    const std::string& action_options = std::string(), const std::string& comment = std::string());
  action_priority_t* add_action(const player_t* p, const std::string& name, const std::string& action_options = std::string(),
    const std::string& comment = std::string());
  action_priority_t* add_talent(const player_t* p, const std::string& name, const std::string& action_options = std::string(),
    const std::string& comment = std::string());
};

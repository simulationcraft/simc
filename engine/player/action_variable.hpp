// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <string>
#include <vector>

struct action_t;

struct action_variable_t
{
  std::string name_;
  double current_value_, default_, constant_value_;
  std::vector<action_t*> variable_actions;

  action_variable_t(const std::string& name, double def = 0);

  double value() const
  {
    return current_value_;
  }

  void reset()
  {
    current_value_ = default_;
  }

  bool is_constant(double* constant_value) const;

  // Implemented in sc_player.cpp
  void optimize();
};
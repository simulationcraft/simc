// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <string>
#include <vector>

struct action_t;

struct action_variable_t
{
  double current_value_, default_value_, constant_value_;
  std::string name_;
  std::vector<action_t*> variable_actions;

  action_variable_t( const std::string& name, double default_value );

  double value() const
  {
    return current_value_;
  }

  void reset()
  {
    current_value_ = default_value_;
  }

  bool is_constant( double* constant_value ) const;

  void optimize();
};
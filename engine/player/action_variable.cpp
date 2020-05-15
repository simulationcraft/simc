// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action_variable.hpp"
#include "action/sc_action.hpp"
#include "action/variable.hpp"
#include "player/sc_player.hpp"
#include <limits>

action_variable_t::action_variable_t(const std::string& name, double def) :
  name_(name), current_value_(def), default_(def),
  constant_value_(std::numeric_limits<double>::lowest())
{ }

bool action_variable_t::is_constant(double* constant_value) const
{
  *constant_value = constant_value_;
  return constant_value_ != std::numeric_limits<double>::lowest();
}

void action_variable_t::optimize()
{
  player_t* player = variable_actions.front()->player;
  if (!player->sim->optimize_expressions)
  {
    return;
  }

  if (player->nth_iteration() != 1)
  {
    return;
  }

  bool is_constant = true;
  for (auto action : variable_actions)
  {
    variable_t* var_action = debug_cast<variable_t*>(action);

    var_action->optimize_expressions();

    is_constant = var_action->is_constant();
    if (!is_constant)
    {
      break;
    }
  }

  // This variable only has constant variable actions associated with it. The constant value will be
  // whatever the value is in current_value_
  if (is_constant)
  {
    player->sim->print_debug("{} variable {} is constant, value={}",
      player->name(), name_, current_value_);
    constant_value_ = current_value_;
    // Make default value also the constant value, so that debug output is sensible
    default_ = current_value_;
  }
}
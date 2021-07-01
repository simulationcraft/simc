// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_action.hpp"
#include "sc_enums.hpp"
#include "util/string_view.hpp"
#include <string>
#include <memory>

struct action_variable_t;
struct expr_t;
struct player_t;

struct variable_t : public action_t
{
  action_var_e operation;
  action_variable_t* var;
  std::string value_str, value_else_str, var_name_str, condition_str;
  std::unique_ptr<expr_t> value_expression;
  std::unique_ptr<expr_t> condition_expression;
  std::unique_ptr<expr_t> value_else_expression;

  variable_t(player_t* player, util::string_view options_str);

  void init_finished() override;

  void reset() override;

  // A variable action is constant if
  // 1) The operation is not SETIF and the value expression is constant
  // 2) The operation is SETIF and both the condition expression and the value (or value expression)
  //    are both constant
  // 3) The operation is reset/floor/ceil and all of the other actions manipulating the variable are
  //    constant
  bool is_constant(double* const_value) const;

  // Variable action expressions have to do an optimization pass before other actions, so that
  // actions with variable expressions can know if the associated variable is constant
  void optimize_expressions();

  // Note note note, doesn't do anything that a real action does
  void execute() override;
};

struct cycling_variable_t : public variable_t
{
  using variable_t::variable_t;

  void execute() override;
};
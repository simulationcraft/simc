// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "action.hpp"
#include "sc_enums.hpp"

#include <memory>
#include <string>

struct action_variable_t;
struct expr_t;
struct player_t;

enum action_var_e
{
  OPERATION_NONE = -1,  // Invalid operation
  OPERATION_SET,        // Set variable to value
  OPERATION_PRINT,      // (debug) Print variable data to standard output
  OPERATION_RESET,      // Reset variable to default value
  OPERATION_ADD,        // Add value to variable
  OPERATION_SUB,        // Subtract value from variable
  OPERATION_MUL,        // Multiply variable by value
  OPERATION_DIV,        // Divide variable by value
  OPERATION_POW,        // Raise variable to power of value
  OPERATION_MOD,        // Take variable remainder of value
  OPERATION_MIN,        // Assign minimum of variable, value to variable
  OPERATION_MAX,        // Assign maximum of variable, value to variable
  OPERATION_FLOOR,      // Floor variable
  OPERATION_CEIL,       // Raise variable to next integer value
  OPERATION_SETIF,      // Set variable to value if condition met
  OPERATION_REPORT,     // Report value changes in HTML sample sequence
};

struct variable_t : public action_t
{
  action_var_e operation;
  action_variable_t* var;
  std::string value_str, value_else_str, condition_str;
  std::unique_ptr<expr_t> value_expression;
  std::unique_ptr<expr_t> condition_expression;
  std::unique_ptr<expr_t> value_else_expression;

  variable_t( player_t* player, std::string_view options_str );

  void init_finished() override;

  void reset() override;

  // A variable action is constant if
  // 1) The operation is not SETIF and the value expression is constant
  // 2) The operation is SETIF and both the condition expression and the value (or value expression) are both constant
  // 3) The operation is reset/floor/ceil and all of the other actions manipulating the variable are constant
  bool is_constant() const;

  // Variable action expressions have to do an optimization pass before other actions, so that actions with variable
  // expressions can know if the associated variable is constant
  void optimize_expressions();

  // Note note note, doesn't do anything that a real action does
  void execute() override;

  bool action_ready() override;

  static constexpr std::pair<action_var_e, std::string_view> operations[] = {
    { OPERATION_SET,    "set"    },
    { OPERATION_PRINT,  "print"  },
    { OPERATION_RESET,  "reset"  },
    { OPERATION_ADD,    "add"    },
    { OPERATION_SUB,    "sub"    },
    { OPERATION_MUL,    "mul"    },
    { OPERATION_DIV,    "div"    },
    { OPERATION_POW,    "pow"    },
    { OPERATION_MOD,    "mod"    },
    { OPERATION_MIN,    "min"    },
    { OPERATION_MAX,    "max"    },
    { OPERATION_FLOOR,  "floor"  },
    { OPERATION_CEIL,   "ceil"   },
    { OPERATION_SETIF,  "setif"  },
    { OPERATION_REPORT, "report" },
  };

  static action_var_e get_operation( std::string_view );
  static std::string_view operation_str( action_var_e );
};

struct cycling_variable_t : public variable_t
{
  using variable_t::variable_t;

  void execute() override;
};

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "variable.hpp"
#include "sim/sc_option.hpp"
#include "player/sc_player.hpp"
#include "player/action_variable.hpp"
#include "player/action_priority_list.hpp"
#include "sim/sc_expressions.hpp"
#include "sim/sc_cooldown.hpp"


variable_t::variable_t(player_t* player, util::string_view options_str) :
  action_t(ACTION_VARIABLE, "variable", player),
  operation(OPERATION_SET),
  var(nullptr),
  value_expression(),
  condition_expression(),
  value_else_expression()
{
  quiet = usable_while_casting = true;
  harmful = proc = callbacks = may_miss = may_crit = may_block = may_parry = may_dodge = false;
  trigger_gcd = timespan_t::zero();

  std::string operation_;
  double default_ = 0;
  timespan_t delay_ = timespan_t::zero();

  add_option(opt_string("name", name_str));
  add_option(opt_string("value", value_str));
  add_option(opt_string("op", operation_));
  add_option(opt_float("default", default_));
  add_option(opt_timespan("delay", delay_));
  add_option(opt_string("condition", condition_str));
  add_option(opt_string("value_else", value_else_str));
  parse_options(options_str);

  auto option_default = player->apl_variable_map.find(name_str);
  if (option_default != player->apl_variable_map.end())
  {
    try
    {
      default_ = std::stod(option_default->second);
    }
    catch (const std::exception&)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Failed to parse player option for variable '{}'", name_str)));
    }
  }

  if (name_str.empty())
  {
    sim->errorf("Player %s unnamed 'variable' action used", player->name());
    background = true;
    return;
  }

  // Figure out operation
  if (!operation_.empty())
  {
    if (util::str_compare_ci(operation_, "set"))
      operation = OPERATION_SET;
    else if (util::str_compare_ci(operation_, "print"))
      operation = OPERATION_PRINT;
    else if (util::str_compare_ci(operation_, "reset"))
      operation = OPERATION_RESET;
    else if (util::str_compare_ci(operation_, "add"))
      operation = OPERATION_ADD;
    else if (util::str_compare_ci(operation_, "sub"))
      operation = OPERATION_SUB;
    else if (util::str_compare_ci(operation_, "mul"))
      operation = OPERATION_MUL;
    else if (util::str_compare_ci(operation_, "div"))
      operation = OPERATION_DIV;
    else if (util::str_compare_ci(operation_, "pow"))
      operation = OPERATION_POW;
    else if (util::str_compare_ci(operation_, "mod"))
      operation = OPERATION_MOD;
    else if (util::str_compare_ci(operation_, "min"))
      operation = OPERATION_MIN;
    else if (util::str_compare_ci(operation_, "max"))
      operation = OPERATION_MAX;
    else if (util::str_compare_ci(operation_, "floor"))
      operation = OPERATION_FLOOR;
    else if (util::str_compare_ci(operation_, "ceil"))
      operation = OPERATION_CEIL;
    else if (util::str_compare_ci(operation_, "setif"))
      operation = OPERATION_SETIF;
    else
    {
      sim->errorf(
        "Player %s unknown operation '%s' given for variable, valid values are 'set', 'print', and 'reset'.",
        player->name(), operation_.c_str());
      background = true;
      return;
    }
  }

  // Printing needs a delay, otherwise the action list will not progress
  if (operation == OPERATION_PRINT && delay_ == timespan_t::zero())
    delay_ = timespan_t::from_seconds(1.0);

  if (operation != OPERATION_FLOOR && operation != OPERATION_CEIL && operation != OPERATION_RESET &&
    operation != OPERATION_PRINT)
  {
    if (value_str.empty())
    {
      sim->errorf("Player %s no value expression given for variable '%s'", player->name(), name_str.c_str());
      background = true;
      return;
    }
    if (operation == OPERATION_SETIF)
    {
      if (condition_str.empty())
      {
        sim->errorf("Player %s no condition expression given for variable '%s'", player->name(), name_str.c_str());
        background = true;
        return;
      }
      if (value_else_str.empty())
      {
        sim->errorf("Player %s no value_else expression given for variable '%s'", player->name(), name_str.c_str());
        background = true;
        return;
      }
    }
  }

  // Add a delay
  if (delay_ > timespan_t::zero())
  {
    std::string cooldown_name = "variable_actor";
    cooldown_name += util::to_string(player->index);
    cooldown_name += "_";
    cooldown_name += name_str;

    cooldown = player->get_cooldown(cooldown_name);
    cooldown->duration = delay_;
  }

  // Find the variable
  for (auto& elem : player->variables)
  {
    if (util::str_compare_ci(elem->name_, name_str))
    {
      var = elem;
      break;
    }
  }

  if (!var)
  {
    player->variables.push_back(new action_variable_t(name_str, default_));
    var = player->variables.back();
  }
}

void variable_t::init_finished()
{
  action_t::init_finished();

  if (!background && operation != OPERATION_FLOOR && operation != OPERATION_CEIL && operation != OPERATION_RESET &&
    operation != OPERATION_PRINT)
  {
    value_expression = expr_t::parse(this, value_str, sim->optimize_expressions);
    if (!value_expression)
    {
      sim->errorf("Player %s unable to parse 'variable' value '%s'", player->name(), value_str.c_str());
      background = true;
    }
    if (operation == OPERATION_SETIF)
    {
      condition_expression = expr_t::parse(this, condition_str, sim->optimize_expressions);
      if (!condition_expression)
      {
        sim->errorf("Player %s unable to parse 'condition' value '%s'", player->name(), condition_str.c_str());
        background = true;
      }
      value_else_expression = expr_t::parse(this, value_else_str, sim->optimize_expressions);
      if (!value_else_expression)
      {
        sim->errorf("Player %s unable to parse 'value_else' value '%s'", player->name(), value_else_str.c_str());
        background = true;
      }
    }
  }

  if (!background)
  {
    var->variable_actions.push_back(this);
  }
}

void variable_t::reset()
{
  action_t::reset();

  double cv = 0;
  // In addition to if= expression removing the variable from the APLs, if the the variable value
  // is constant, we can remove any variable action referencing it from the APL
  if (action_list && sim->optimize_expressions && player->nth_iteration() == 1 &&
    var->is_constant(&cv) && (!if_expr || (if_expr && if_expr->is_constant(&cv))))
  {
    auto it = range::find(action_list->foreground_action_list, this);
    if (it != action_list->foreground_action_list.end())
    {
      sim->print_debug("{} removing variable action {} from APL because the variable value is "
        "constant (value={})",
        *player, signature_str, var->current_value_);

      action_list->foreground_action_list.erase(it);
    }
  }
}

// A variable action is constant if
// 1) The operation is not SETIF and the value expression is constant
// 2) The operation is SETIF and both the condition expression and the value (or value expression)
//    are both constant
// 3) The operation is reset/floor/ceil and all of the other actions manipulating the variable are
//    constant

bool variable_t::is_constant() const
{
  double const_value = 0;
  // If the variable action is conditionally executed, and the conditional execution is not
  // constant, the variable cannot be constant.
  if (if_expr && !if_expr->is_constant(&const_value))
  {
    return false;
  }

  // Special casing, some actions are only constant, if all of the other action variables in the
  // set (that manipulates a variable) are constant
  if (operation == OPERATION_RESET || operation == OPERATION_FLOOR ||
    operation == OPERATION_CEIL)
  {
    auto it = range::find_if(var->variable_actions, [this](const action_t* action) {
      return action != this && !debug_cast<const variable_t*>(action)->is_constant();
      });

    return it == var->variable_actions.end();
  }
  else if (operation != OPERATION_SETIF)
  {
    return value_expression ? value_expression->is_constant(&const_value) : true;
  }
  else
  {
    bool constant = condition_expression->is_constant(&const_value);
    if (!constant)
    {
      return false;
    }

    if (const_value != 0)
    {
      return value_expression->is_constant(&const_value);
    }
    else
    {
      return value_else_expression->is_constant(&const_value);
    }
  }
}

// Variable action expressions have to do an optimization pass before other actions, so that
// actions with variable expressions can know if the associated variable is constant

void variable_t::optimize_expressions()
{
  expr_t::optimize_expression(if_expr);
  expr_t::optimize_expression(value_expression);
  expr_t::optimize_expression(condition_expression);
  expr_t::optimize_expression(value_else_expression);
}

// Note note note, doesn't do anything that a real action does

void variable_t::execute()
{
  if (operation != OPERATION_PRINT)
  {
    sim->print_debug("{} variable name={} op={} value={} default={} sig={}", *player, var->name_,
      operation, var->current_value_, var->default_value_, signature_str);
  }

  switch (operation)
  {
  case OPERATION_SET:
    var->current_value_ = value_expression->eval();
    break;
  case OPERATION_ADD:
    var->current_value_ += value_expression->eval();
    break;
  case OPERATION_SUB:
    var->current_value_ -= value_expression->eval();
    break;
  case OPERATION_MUL:
    var->current_value_ *= value_expression->eval();
    break;
  case OPERATION_DIV:
  {
    auto v = value_expression->eval();
    // Disallow division by zero, set value to zero
    if (v == 0)
    {
      var->current_value_ = 0;
    }
    else
    {
      var->current_value_ /= v;
    }
    break;
  }
  case OPERATION_POW:
    var->current_value_ = std::pow(var->current_value_, value_expression->eval());
    break;
  case OPERATION_MOD:
  {
    // Disallow division by zero, set value to zero
    auto v = value_expression->eval();
    if (v == 0)
    {
      var->current_value_ = 0;
    }
    else
    {
      var->current_value_ = std::fmod(var->current_value_, value_expression->eval());
    }
    break;
  }
  case OPERATION_MIN:
    var->current_value_ = std::min(var->current_value_, value_expression->eval());
    break;
  case OPERATION_MAX:
    var->current_value_ = std::max(var->current_value_, value_expression->eval());
    break;
  case OPERATION_FLOOR:
    var->current_value_ = util::floor(var->current_value_);
    break;
  case OPERATION_CEIL:
    var->current_value_ = util::ceil(var->current_value_);
    break;
  case OPERATION_PRINT:
    // Only spit out prints in main thread
    if (sim->parent == nullptr)
    {
      sim -> out_log.print( "actor={} time={} iterations={} variable={} value={}\n",
                            player->name_str, sim->current_time(), sim->current_iteration,
                            var->name_, var->current_value_ );
    }
    break;
  case OPERATION_RESET:
    var->reset();
    break;
  case OPERATION_SETIF:
    if (condition_expression->eval() != 0)
      var->current_value_ = value_expression->eval();
    else
      var->current_value_ = value_else_expression->eval();
    break;
  default:
    assert(0);
    break;
  }
}

void cycling_variable_t::execute()
{
  if (sim->target_non_sleeping_list.size() > 1)
  {
    player_t* saved_target = target;

    // Note, need to take a copy of the original target list here, instead of a reference. Otherwise
    // if spell_targets (or any expression that uses the target list) modifies it, the loop below
    // may break, since the number of elements on the vector is not the same as it originally was
    std::vector<player_t*> ctl = target_list();
    size_t num_targets = ctl.size();

    for (size_t i = 0; i < num_targets; i++)
    {
      target = ctl[i];
      variable_t::execute();
    }

    target = saved_target;
    return;
  }

  variable_t::execute();
}

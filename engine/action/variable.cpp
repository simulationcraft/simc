// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "variable.hpp"

#include "player/action_priority_list.hpp"
#include "player/action_variable.hpp"
#include "player/player.hpp"
#include "sim/cooldown.hpp"
#include "sim/expressions.hpp"
#include "sim/option.hpp"
#include "sim/sim.hpp"

variable_t::variable_t( player_t* player, std::string_view options_str )
  : action_t( ACTION_VARIABLE, "variable", player ),
    operation( OPERATION_SET ),
    var( nullptr ),
    value_expression(),
    condition_expression(),
    value_else_expression()
{
  quiet = usable_while_casting = true;
  harmful = proc = callbacks = may_miss = may_crit = may_block = may_parry = may_dodge = false;
  trigger_gcd = 0_ms;

  target = player;  // Does not require a hostile target

  std::string operation_str;
  double default_ = 0;
  timespan_t delay_ = 0_ms;

  add_option( opt_string( "name", name_str ) );
  add_option( opt_string( "value", value_str ) );
  add_option( opt_string( "op", operation_str ) );
  add_option( opt_float( "default", default_ ) );
  add_option( opt_timespan( "delay", delay_ ) );
  add_option( opt_string( "condition", condition_str ) );
  add_option( opt_string( "value_else", value_else_str ) );
  parse_options( options_str );

  auto option_default = player->apl_variable_map.find( name_str );
  if ( option_default != player->apl_variable_map.end() )
  {
    if ( !util::is_number( option_default->second ) )
      sim->error( "Invalid value '{}' for 'apl_variable.{}', ignoring.", option_default->second, name_str );
    else
      default_ = std::stod( option_default->second );
  }

  if ( name_str.empty() )
  {
    sim->error( "{} unnamed 'variable' action used.", *player );
    background = true;
    return;
  }

  // Figure out operation
  if ( !operation_str.empty() )
    operation = get_operation( operation_str );

  if ( operation == OPERATION_NONE )
  {
    sim->error( "{} unknown operation '{}' for variable '{}'", *player, operation_str, name_str );
    background = true;
    return;
  }

  // Printing needs a delay, otherwise the action list will not progress
  if ( operation == OPERATION_PRINT && delay_ == 0_ms )
    delay_ = 1_s;

  if ( operation != OPERATION_FLOOR && operation != OPERATION_CEIL && operation != OPERATION_RESET &&
       operation != OPERATION_PRINT && operation != OPERATION_REPORT )
  {
    if ( value_str.empty() )
    {
      sim->error( "{} no value expression given for variable '{}'", *player, name_str );
      background = true;
      return;
    }
    if ( operation == OPERATION_SETIF )
    {
      if ( condition_str.empty() )
      {
        sim->error( "{} no condition expression given for variable '{}'", *player, name_str );
        background = true;
        return;
      }
      if ( value_else_str.empty() )
      {
        sim->error( "{} no value_else expression given for variable '{}'", *player, name_str );
        background = true;
        return;
      }
    }
  }

  // Add a delay
  if ( delay_ > 0_ms )
  {
    auto cooldown_name = fmt::format( "variable_actor{}_{}", util::to_string( player->index ), name_str );
    cooldown = player->get_cooldown( cooldown_name );
    cooldown->duration = delay_;
  }

  // Find the variable
  for ( auto elem : player->variables )
  {
    if ( util::str_compare_ci( elem->name_, name_str ) )
    {
      var = elem;
      break;
    }
  }

  if ( !var )
  {
    var = new action_variable_t( name_str, default_ );
    if ( sim->report_all_variables )
      var->report = true;

    player->variables.push_back( var );
  }

  if ( operation == OPERATION_REPORT )
    var->report = true;
}

void variable_t::init_finished()
{
  action_t::init_finished();

  if ( !background && operation != OPERATION_FLOOR && operation != OPERATION_CEIL && operation != OPERATION_RESET &&
       operation != OPERATION_PRINT && operation != OPERATION_REPORT )
  {
    try
    {
      value_expression = expr_t::parse( this, value_str, sim->optimize_expressions );
      if ( !value_expression )
        throw std::invalid_argument( fmt::format( "Invalid value '{}'.", value_str ) );

      if ( operation == OPERATION_SETIF )
      {
        condition_expression = expr_t::parse( this, condition_str, sim->optimize_expressions );
        if ( !condition_expression )
          throw std::invalid_argument( fmt::format( "Invalid condition '{}'.", condition_str ) );

        value_else_expression = expr_t::parse( this, value_else_str, sim->optimize_expressions );
        if ( !value_else_expression )
          throw std::invalid_argument( fmt::format( "Invalid value_else '{}'.", value_else_str ) );
      }
    }
    catch ( const std::exception& )
    {
      std::throw_with_nested( sc_invalid_apl_argument( fmt::format( "Invalid variable '{}", name_str ) ) );
    }
  }

  if ( !background )
  {
    var->variable_actions.push_back( this );
  }
}

void variable_t::reset()
{
  action_t::reset();

  if ( !var )
    return;

  // In addition to if= expression removing the variable from the APLs, if the the variable value
  // is constant, we can remove any variable action referencing it from the APL
  if ( action_list && sim->optimize_expressions && player->nth_iteration() == 1 && var->is_constant() &&
       ( !if_expr || ( if_expr && if_expr->is_constant() ) ) )
  {
    auto it = range::find( action_list->foreground_action_list, this );
    if ( it != action_list->foreground_action_list.end() )
    {
      sim->print_debug( "{} removing variable action {} from APL because the variable value is constant (value={})",
                        *player, signature_str, var->current_value_ );

      action_list->foreground_action_list.erase( it );
    }
  }
}

// A variable action is constant if
// 1) The operation does not use the current value
// 2) The operation is not SETIF and the value expression is constant
// 3) The operation is SETIF and both the condition expression and the value (or value expression)
//    are both constant
// 4) The operation is reset/floor/ceil and all of the other actions manipulating the variable are
//    constant

bool variable_t::is_constant() const
{
  // If the operation uses the current value, the variable isn't constant
  // TODO: technically, we could handle special cases such as adding zero, but that doesn't seem all that important
  if ( operation >= OPERATION_ADD && operation <= OPERATION_MAX )
  {
    return false;
  }

  // If the variable action is conditionally executed, and the conditional execution is not
  // constant, the variable cannot be constant.
  if ( if_expr && !if_expr->is_constant() )
  {
    return false;
  }

  // Special casing, some actions are only constant, if all of the other action variables in the
  // set (that manipulates a variable) are constant
  if ( operation == OPERATION_RESET || operation == OPERATION_FLOOR || operation == OPERATION_CEIL )
  {
    return !range::any_of( var->variable_actions, [ this ]( const action_t* action ) {
      return action != this && !debug_cast<const variable_t*>( action )->is_constant();
    } );
  }
  else if ( operation != OPERATION_SETIF )
  {
    return value_expression ? value_expression->is_constant() : true;
  }
  else
  {
    bool constant = condition_expression->is_constant();
    if ( !constant )
    {
      return false;
    }

    if ( condition_expression->evaluate() )
    {
      return value_expression->is_constant();
    }
    else
    {
      return value_else_expression->is_constant();
    }
  }

  return false;
}

// Variable action expressions have to do an optimization pass before other actions, so that
// actions with variable expressions can know if the associated variable is constant

void variable_t::optimize_expressions()
{
  expr_t::optimize_expression( if_expr, *sim );
  expr_t::optimize_expression( value_expression, *sim );
  expr_t::optimize_expression( condition_expression, *sim );
  expr_t::optimize_expression( value_else_expression, *sim );
}

// Note note note, doesn't do anything that a real action does

void variable_t::execute()
{
  var->previous_value_ = var->current_value_;

  switch ( operation )
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
      if ( v == 0 )
        var->current_value_ = 0;
      else
        var->current_value_ /= v;
      break;
    }
    case OPERATION_POW:
      var->current_value_ = std::pow( var->current_value_, value_expression->eval() );
      break;
    case OPERATION_MOD:
    {
      // Disallow division by zero, set value to zero
      auto v = value_expression->eval();
      if ( v == 0 )
        var->current_value_ = 0;
      else
        var->current_value_ = std::fmod( var->current_value_, value_expression->eval() );
      break;
    }
    case OPERATION_MIN:
      var->current_value_ = std::min( var->current_value_, value_expression->eval() );
      break;
    case OPERATION_MAX:
      var->current_value_ = std::max( var->current_value_, value_expression->eval() );
      break;
    case OPERATION_FLOOR:
      var->current_value_ = util::floor( var->current_value_ );
      break;
    case OPERATION_CEIL:
      var->current_value_ = util::ceil( var->current_value_ );
      break;
    case OPERATION_PRINT:
      // Only spit out prints in main thread
      if ( sim->parent == nullptr && sim->log )
      {
        sim->out_log.print( "actor={} time={} iterations={} variable={} value={}", player->name_str,
                            sim->current_time(), sim->current_iteration, var->name_, var->current_value_ );
      }
      break;
    case OPERATION_RESET:
      var->reset();
      break;
    case OPERATION_SETIF:
      if ( condition_expression->eval() != 0 )
        var->current_value_ = value_expression->eval();
      else
        var->current_value_ = value_else_expression->eval();
      break;
    default:
      assert( 0 );
      break;
  }

  if ( operation != OPERATION_PRINT && sim->debug )
  {
    sim->print_debug( "{} variable name={} op={} value={} prev_value={} default={} sig={}", *player, var->name_,
                      static_cast<int>( operation ), var->current_value_, var->previous_value_, var->default_value_,
                      signature_str );
  }

  if ( var->report && !var->is_constant() && var->previous_value_ != var->current_value_ )
  {
    player->sequence_add( this, nullptr );
    total_executions++;
  }
}

void variable_t::sequence_add_fn( std::string& a_str, std::string& t_str ) const
{
  a_str = fmt::format( "</b>VAR [{}]<br><b>{}", operation_str( operation ), name_str );
  t_str = fmt::format( "Old:&#160;{:.4f}<br>New:&#160;<b>{:.4f}</b>", var->previous_value_, var->current_value_ );
}

bool variable_t::action_ready()
{
  if ( !select_target() )
    return false;

  if ( line_cooldown->down() )
    return false;

  if ( if_expr && !if_expr->success() )
    return false;

  return true;
}

void cycling_variable_t::execute()
{
  if ( sim->target_non_sleeping_list.size() > 1 )
  {
    player_t* saved_target = target;

    // Note, need to take a copy of the original target list here, instead of a reference. Otherwise
    // if spell_targets (or any expression that uses the target list) modifies it, the loop below
    // may break, since the number of elements on the vector is not the same as it originally was
    std::vector<player_t*> ctl = target_list();
    size_t num_targets = ctl.size();

    for ( size_t i = 0; i < num_targets; i++ )
    {
      target = ctl[ i ];
      variable_t::execute();
    }

    target = saved_target;
    return;
  }

  variable_t::execute();
}

action_var_e variable_t::get_operation( std::string_view op_str )
{
  for ( auto [ op, str ] : operations )
    if ( util::str_compare_ci( str, op_str ) )
      return op;

  return OPERATION_NONE;
}

std::string_view variable_t::operation_str( action_var_e op_type )
{
  for ( auto [ op, str ] : operations )
    if ( op == op_type )
      return str;

  return "unknown";
}

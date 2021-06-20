// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action_variable.hpp"

#include "action/sc_action.hpp"
#include "action/variable.hpp"
#include "player/sc_player.hpp"
#include "sim/sc_sim.hpp"
#include "util/util.hpp"

#include <limits>

action_variable_t::action_variable_t( const std::string& name, double default_value )
  : current_value_( default_value ),
    default_value_( default_value ),
    constant_value_( std::numeric_limits<double>::lowest() ),
    name_( name )
{
}

bool action_variable_t::is_constant( double* constant_value ) const
{
  *constant_value = constant_value_;
  return constant_value_ != std::numeric_limits<double>::lowest();
}

void action_variable_t::optimize()
{
  player_t* player = variable_actions.front()->player;
  auto iteration = player->sim->current_iteration;
  if ( iteration < 0 )
  {
    return;
  }
  if ( player->sim->optimize_expressions - 1 - iteration < 0 )
  {
    return;
  }

  // Do nothing if the variable is already constant
  if (constant_value_ != std::numeric_limits<double>::lowest())
  {
    return;
  }

  for ( auto action : variable_actions )
  {
    variable_t* var_action = debug_cast<variable_t*>( action );

    var_action->optimize_expressions();
  }

  bool is_constant = true;
  double const_value;
  for ( auto action : variable_actions )
  {
    variable_t* var_action = debug_cast<variable_t*>( action );

    is_constant = var_action->is_constant(&const_value);
    if ( !is_constant )
    {
      break;
    }
  }

  // This variable only has constant variable actions associated with it. The constant value will be
  // whatever the value is in current_value_
  if ( is_constant )
  {
    player->sim->print_debug( "{} variable {} is constant, value={}", player->name(), name_, current_value_ );
    constant_value_ = current_value_;
    // Make default value also the constant value, so that debug output is sensible
    default_value_ = current_value_;
  }
}
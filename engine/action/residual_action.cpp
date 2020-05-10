// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "residual_action.hpp"
#include <iostream>
#include <sstream>

residual_action::residual_periodic_state_t::residual_periodic_state_t(action_t* a, player_t* t) :
  action_state_t(a, t),
  tick_amount(0)
{ }

std::ostringstream& residual_action::residual_periodic_state_t::debug_str(std::ostringstream & s)
{
  action_state_t::debug_str(s) << " tick_amount=" << tick_amount; return s;
}

void residual_action::residual_periodic_state_t::initialize()
{
  action_state_t::initialize(); tick_amount = 0;
}

void residual_action::residual_periodic_state_t::copy_state(const action_state_t* o)
{
  action_state_t::copy_state(o);
  const residual_periodic_state_t* dps_t = debug_cast<const residual_periodic_state_t*>(o);
  tick_amount = dps_t->tick_amount;
}

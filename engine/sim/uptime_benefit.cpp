// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "benefit.hpp"
#include "uptime.hpp"

#include "sim/sc_sim.hpp"

benefit_t* benefit_t::collect_ratio(bool collect)
{
  if (sim.report_details)
  {
    ratio.change_mode(!collect);
    ratio.reserve(std::min(as<unsigned>(sim.iterations), 2048U));
  }

  return this;
}

void uptime_base_t::update(bool is_up, timespan_t current_time)
{
  if (is_up)
  {
    if (last_start < timespan_t::zero())
      last_start = current_time;
  }
  else if (last_start >= timespan_t::zero())
  {
    iteration_uptime_sum += current_time - last_start;
    reset();
  }
}

void uptime_t::update(bool is_up, timespan_t current_time)
{
  if (is_up)
  {
    if (last_start < timespan_t::zero())
      last_start = current_time;
  }
  else if (last_start >= timespan_t::zero())
  {
    auto delta = current_time - last_start;
    iteration_uptime_sum += delta;
    uptime_instance.add(delta.total_seconds());
    reset();
  }
}

uptime_t* uptime_t::collect_uptime(bool collect)
{
  if (sim.report_details)
  {
    uptime_sum.change_mode(!collect);
    uptime_sum.reserve(std::min(as<unsigned>(sim.iterations), 2048U));
  }

  return this;
}

uptime_t* uptime_t::collect_duration(bool collect)
{
  if (sim.report_details)
  {
    uptime_instance.change_mode(!collect);
    uptime_instance.reserve(std::min(as<unsigned>(sim.iterations), 2048U));
  }

  return this;
}

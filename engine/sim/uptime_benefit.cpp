// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "benefit.hpp"
#include "uptime.hpp"

#include "sim/sim.hpp"

benefit_t* benefit_t::collect_ratio( sim_t& sim, bool collect )
{
  ratio.change_mode( !collect );
  ratio.reserve( std::min( as<unsigned>( sim.iterations ), 2048U ) );

  return this;
}

void uptime_base_t::update( bool is_up, timespan_t current_time )
{
  if ( is_up )
  {
    if ( last_start < 0_ms )
      last_start = current_time;
  }
  else if ( last_start >= 0_ms)
  {
    iteration_uptime_sum += current_time - last_start;
    reset();
  }
}

void uptime_t::update( bool is_up, timespan_t current_time )
{
  if ( is_up )
  {
    if ( last_start < 0_ms )
      last_start = current_time;
  }
  else if ( last_start >= 0_ms )
  {
    auto delta = current_time - last_start;

    if ( delta > 0_ms )
    {
      iteration_uptime_sum += delta;
      uptime_instance.add( delta.total_seconds() );
    }

    reset();
  }
}

uptime_t* uptime_t::collect_uptime( sim_t& sim, bool collect )
{
  uptime_sum.change_mode( !collect );
  uptime_sum.reserve( std::min( as<unsigned>( sim.iterations ), 2048U ) );

  return this;
}

uptime_t* uptime_t::collect_duration( sim_t& sim, bool collect )
{
  uptime_instance.change_mode( !collect );
  uptime_instance.reserve( std::min( as<unsigned>( sim.iterations ), 2048U ) );

  return this;
}

// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "proc.hpp"

#include "sim/sc_sim.hpp"

proc_t::proc_t( sim_t& s, util::string_view n )
  : sim( s ),
    iteration_count(),
    last_proc( timespan_t::min() ),
    name_str( n ),
    interval_sum( "Interval", true ),
    count( "Count", true )
{
}

void proc_t::occur()
{
  iteration_count++;
  if ( last_proc >= timespan_t::zero() && last_proc < sim.current_time() )
  {
    interval_sum.add( ( sim.current_time() - last_proc ).total_seconds() );
    reset();
  }
  if ( sim.debug )
    sim.out_debug.printf( "[PROC] %s: iteration_count=%u count.sum=%u last_proc=%f", name(),
                          static_cast<unsigned>( iteration_count ), static_cast<unsigned>( count.sum() ),
                          last_proc.total_seconds() );

  last_proc = sim.current_time();
}

void proc_t::reset()
{
  last_proc = timespan_t::min();
}

void proc_t::merge( const proc_t& other )
{
  count.merge( other.count );
  interval_sum.merge( other.interval_sum );
}

void proc_t::analyze()
{
  count.analyze();
  interval_sum.analyze();
}

void proc_t::datacollection_begin()
{
  iteration_count = 0;
}

void proc_t::datacollection_end()
{
  count.add( static_cast<double>( iteration_count ) );
}

const std::string& proc_t::name() const
{
  return name_str;
}

proc_t* proc_t::collect_count( bool collect )
{
  if ( sim.report_details )
  {
    count.change_mode( !collect );
    count.reserve( std::min( as<unsigned>( sim.iterations ), 2048U ) );
  }

  return this;
}

proc_t* proc_t::collect_interval( bool collect )
{
  if ( sim.report_details )
  {
    interval_sum.change_mode( !collect );
    interval_sum.reserve( std::min( as<unsigned>( sim.iterations ), 2048U ) );
  }

  return this;
}
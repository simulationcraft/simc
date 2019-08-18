// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <string>
#include "sc_timespan.hpp"
#include "util/sample_data.hpp"

// Uptime ==================================================================

struct uptime_common_t
{
private:
  timespan_t last_start;
  timespan_t iteration_uptime_sum;
public:
  // These are initialized in SIMPLE mode. Only change mode for infrequent procs to keep memory usage reasonable.
  // Methods to toggle mode via 'collect_<metric>()' are only available for 'uptime_t' in "simulationcraft.hpp".
  extended_sample_data_t uptime_sum;
  extended_sample_data_t uptime_instance;

  uptime_common_t() :
    last_start( timespan_t::min() ),
    iteration_uptime_sum( timespan_t::zero() ),
    uptime_sum( "Uptime", true ),
    uptime_instance( "Duration", true )
  { }

  void update( bool is_up, timespan_t current_time )
  {
    if ( is_up )
    {
      if ( last_start < timespan_t::zero() )
        last_start = current_time;
    }
    else if ( last_start >= timespan_t::zero() )
    {
      auto delta = current_time - last_start;
      iteration_uptime_sum += delta;
      uptime_instance.add( delta.total_seconds() );
      reset();
    }
  }

  void analyze()
  {
    uptime_sum.analyze();
    uptime_instance.analyze();
  }

  void datacollection_begin()
  { iteration_uptime_sum = timespan_t::zero(); }

  void datacollection_end( timespan_t t )
  { uptime_sum.add( t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0 ); }

  void reset()
  { last_start = timespan_t::min(); }

  void merge( const uptime_common_t& other )
  {
    uptime_sum.merge( other.uptime_sum );
    uptime_instance.merge( other.uptime_instance );
  }
};

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
protected:
  timespan_t last_start;
  timespan_t iteration_uptime_sum;

public:
  uptime_common_t() :
    last_start( timespan_t::min() ),
    iteration_uptime_sum( timespan_t::zero() )
  {}

  virtual void update( bool is_up, timespan_t current_time )
  {
    if ( is_up )
    {
      if ( last_start < timespan_t::zero() )
        last_start = current_time;
    }
    else if ( last_start >= timespan_t::zero() )
    {
      iteration_uptime_sum += current_time - last_start;
      reset();
    }
  }

  virtual void merge( const uptime_common_t& ) {}

  virtual void datacollection_end( timespan_t ) {}

  void datacollection_begin()
  { iteration_uptime_sum = timespan_t::zero(); }

  void reset()
  { last_start = timespan_t::min(); }
};

struct uptime_simple_t : public uptime_common_t
{
  simple_sample_data_t uptime_sum;

  uptime_simple_t() : uptime_common_t(),
    uptime_sum()
  {}

  void merge( const uptime_common_t& other ) override
  { uptime_sum.merge( dynamic_cast<const uptime_simple_t&>( other ).uptime_sum ); }

  void datacollection_end( timespan_t t ) override
  { uptime_sum.add( t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0 ); }
};

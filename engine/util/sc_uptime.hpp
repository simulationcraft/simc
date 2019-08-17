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
  simple_sample_data_with_min_max_t uptime_sum;
  simple_sample_data_with_min_max_t uptime_instance;

  uptime_common_t() :
    last_start( timespan_t::min() ),
    iteration_uptime_sum( timespan_t::zero() ),
    uptime_sum(),
    uptime_instance()
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
  void datacollection_begin()
  { iteration_uptime_sum = timespan_t::zero(); }
  void datacollection_end( timespan_t t )
  { uptime_sum.add( t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0 ); }
  void reset() { last_start = timespan_t::min(); }
  void merge( const uptime_common_t& other )
  {
    uptime_sum.merge( other.uptime_sum );
    uptime_instance.merge( other.uptime_instance );
  }
};

struct uptime_t : public uptime_common_t
{
  std::string name_str;

  uptime_t( const std::string& n ) :
    uptime_common_t(), name_str( n )
  {}

  const char* name() const
  { return name_str.c_str(); }
};

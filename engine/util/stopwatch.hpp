// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once
#include "config.hpp"

enum stopwatch_e
{
  STOPWATCH_CPU,
  STOPWATCH_WALL,
  STOPWATCH_THREAD
};

class stopwatch_t
{
public:
  struct time_point_t
  {
    int64_t sec;
    int64_t usec;
  };
  void mark();
  void accumulate();
  double elapsed();
  double current();
  stopwatch_t( stopwatch_e sw_type );
private:
  stopwatch_e type;
  time_point_t _start, _current;
  time_point_t now() const;
};

/// Mark start
inline void stopwatch_t::mark()
{
  _start = now();
}

/// Create new stopwatch and mark starting timepoint
inline stopwatch_t::stopwatch_t( stopwatch_e t ) :
    type( t )
{
  _current.sec = 0; _current.usec = 0;
  mark();
}

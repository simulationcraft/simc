// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "lib/fmt/printf.h"
#include "util/chrono.hpp"

#include <iosfwd>
#include <string>

/**
 * Automatic Timer reporting the time between construction and desctruction of
 * the object.
 */
class report_timer_t
{
private:
  std::string title;
  std::ostream& out;
  chrono::wall_clock::time_point start_time;
  bool started;

public:
  report_timer_t( std::string title, std::ostream& out ) : title( std::move( title ) ), out( out ), started( false )
  {
  }

  void start()
  {
    start_time = chrono::wall_clock::now();
    started    = true;
  }

  ~report_timer_t()
  {
    if ( started )
    {
      fmt::print( out, "{} took {}seconds.\n", title, chrono::elapsed_fp_seconds( start_time ) );
    }
  }
};
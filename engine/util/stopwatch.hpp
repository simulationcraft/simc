// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <chrono>

template <typename Clock>
class stopwatch_t {
  using duration = std::chrono::duration<double>;
public:
  stopwatch_t()
    : _start( Clock::now() ), _current( 0 )
  {}

  void mark() {
    _start = Clock::now();
  }

  void accumulate() {
    _current += Clock::now() - _start;
  }

  double elapsed() const {
    return duration( Clock::now() - _start ).count();
  }

  double current() const {
    return duration( _current ).count();
  }
private:
  typename Clock::time_point _start;
  typename Clock::duration _current;
};

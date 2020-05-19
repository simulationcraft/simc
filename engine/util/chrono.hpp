// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_UTIL_CHRONO_HPP_INCLUDED
#define SC_UTIL_CHRONO_HPP_INCLUDED

#include <chrono>

// simc chrono namespace
namespace chrono {

using wall_clock = std::chrono::steady_clock;

struct cpu_clock {
  using duration   = std::chrono::nanoseconds;
  using rep        = duration::rep;
  using period     = duration::period;
  using time_point = std::chrono::time_point<cpu_clock, duration>;
  static constexpr bool is_steady = true;

  static time_point now() noexcept;
};

struct thread_clock {
  using duration   = std::chrono::nanoseconds;
  using rep        = duration::rep;
  using period     = duration::period;
  using time_point = std::chrono::time_point<thread_clock, duration>;
  static constexpr bool is_steady = true;

  static time_point now() noexcept;
};

template <typename Rep, typename Period>
double to_fp_seconds( std::chrono::duration<Rep, Period> duration )
{
    return std::chrono::duration<double>( duration ).count();
}

template <typename Clock, typename Duration>
double elapsed_fp_seconds( std::chrono::time_point<Clock, Duration> time_point )
{
  return to_fp_seconds( Clock::now() - time_point );
}

} // namespace chrono

#endif // SC_UTIL_CHRONO_HPP_INCLUDED

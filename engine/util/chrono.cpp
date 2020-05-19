// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"
#include "chrono.hpp"

#include <cassert>
#include <stdexcept>
#include <system_error>

#if defined(SC_WINDOWS)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

#if !defined(SC_WINDOWS)
#  include <time.h>
#  include <sys/time.h>
#endif

namespace
{
#if defined(SC_WINDOWS)

enum : int { CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID };
std::chrono::nanoseconds do_clock_gettime( int clock_id )
{
  FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;
  BOOL res = false;
  if ( clock_id == CLOCK_PROCESS_CPUTIME_ID )
    res = GetProcessTimes( GetCurrentProcess(), &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime );
  else if ( clock_id == CLOCK_THREAD_CPUTIME_ID )
    res = GetThreadTimes( GetCurrentThread(), &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime );
  else
    assert( false && "Unsupported clock time." );
  if ( !res )
  {
    DWORD dwErrVal = GetLastError();
    std::error_code ec( dwErrVal, std::system_category() );
    throw std::system_error( ec, "Could not get clock time." );
  }

  //  Returns total user time in 100-ns ticks
  //  Can be tweaked to include kernel times as well.
  ULARGE_INTEGER t;
  t.LowPart = lpUserTime.dwLowDateTime;
  t.HighPart = lpUserTime.dwHighDateTime;

  return std::chrono::nanoseconds( t.QuadPart * 100 );
}

#elif defined(SC_LINUX) || defined(SC_HIGH_PRECISION_STOPWATCH)

std::chrono::nanoseconds do_clock_gettime( clockid_t clock_id )
{
  struct timespec ts;
  clock_gettime( clock_id, &ts );
  return std::chrono::seconds( ts.tv_sec ) + std::chrono::nanoseconds( ts.tv_nsec );
}

#endif
}

/* static */ chrono::cpu_clock::time_point chrono::cpu_clock::now() noexcept {
#if defined(SC_WINDOWS) || defined(SC_LINUX) || defined(SC_HIGH_PRECISION_STOPWATCH)
  return time_point( do_clock_gettime( CLOCK_PROCESS_CPUTIME_ID ) );
#else
  return time_point( std::chrono::nanoseconds( int64_t( clock() * 1e9 / CLOCKS_PER_SEC ) ) );
#endif
}

/* static */ chrono::thread_clock::time_point chrono::thread_clock::now() noexcept {
#if defined(SC_WINDOWS) || defined(SC_LINUX) || defined(SC_HIGH_PRECISION_STOPWATCH)
  return time_point( do_clock_gettime( CLOCK_THREAD_CPUTIME_ID ) );
#else
  struct timeval tv;
  gettimeofday( &tv, nullptr );
  return time_point( std::chrono::seconds( tv.tv_sec ) + std::chrono::microseconds( tv.tv_usec ) );
#endif
}

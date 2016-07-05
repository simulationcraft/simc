// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "stopwatch.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <system_error>
#include <limits>

#if defined(SC_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if !defined(SC_WINDOWS)
#include <sys/time.h>
#endif

// If you turn this on, you will need to add -lrt to LINK_LIBS in Makefile
// #define SC_HIGH_PRECISION_STOPWATCH
#if defined(SC_HIGH_PRECISION_STOPWATCH)
#include <sys/resource.h>
#include <sys/times.h>
#endif

namespace
{
/// Helper function to turn integral sec/usec pair into floating-point seconds.
double time_point_to_sec( const stopwatch_t::time_point_t& t )
{
  return ( double( t.sec ) + double( t.usec ) / 1e6 );
}

#if defined(SC_WINDOWS)

// Credit to http://stackoverflow.com/a/17440673

stopwatch_t::time_point_t windows_wall_time()
{
  LARGE_INTEGER time, freq;
  if ( !QueryPerformanceFrequency( &freq ) )
  {
    DWORD dwErrVal = GetLastError();
    std::error_code ec( dwErrVal, std::system_category() );
    throw std::system_error( ec, "Could not query PerformanceFrequency." );
  }
  if ( !QueryPerformanceCounter( &time ) )
  {
    DWORD dwErrVal = GetLastError();
    std::error_code ec( dwErrVal, std::system_category() );
    throw std::system_error( ec, "Could not query PerformanceCounter." );
  }
  double ticks_per_usec = freq.QuadPart / 1e6;
  stopwatch_t::time_point_t t;
  t.sec = time.QuadPart / freq.QuadPart;
  t.usec = static_cast<int64_t>( time.QuadPart / ticks_per_usec ) % 1000000;
  return t;
}

stopwatch_t::time_point_t windows_user_cpu_time()
{
  FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;
  if ( !GetProcessTimes( GetCurrentProcess(), &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime ) )
  {
    DWORD dwErrVal = GetLastError();
    std::error_code ec( dwErrVal, std::system_category() );
    throw std::system_error( ec, "Could not get ProcessTimes." );
  }

  //  Returns total user time in 100-ns ticks
  //  Can be tweaked to include kernel times as well.
  ULARGE_INTEGER t;
  t.u.LowPart = lpUserTime.dwLowDateTime;
  t.u.HighPart = lpUserTime.dwHighDateTime;

  stopwatch_t::time_point_t out;
  out.sec = t.QuadPart / 10000000;
  out.usec = ( t.QuadPart % 10000000 ) / 10;
  return out;
}

stopwatch_t::time_point_t windows_user_thread_time()
{
  FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;
  if ( !GetThreadTimes( GetCurrentThread(), &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime ) )
  {
    DWORD dwErrVal = GetLastError();
    std::error_code ec( dwErrVal, std::system_category() );
    throw std::system_error( ec, "Could not get ThreadTimes." );
  }
  //  Returns total user time in 100-ns ticks
  //  Can be tweaked to include kernel times as well.
  ULARGE_INTEGER t;
  t.u.LowPart = lpUserTime.dwLowDateTime;
  t.u.HighPart = lpUserTime.dwHighDateTime;

  stopwatch_t::time_point_t out;
  out.sec = t.QuadPart / 10000000;
  out.usec = ( t.QuadPart % 10000000 ) / 10;
  return out;
}
#endif
}

/// Collect current timepoint.
stopwatch_t::time_point_t stopwatch_t::now() const
{
#if defined(SC_WINDOWS)
  if ( type == STOPWATCH_WALL )
  {
    return windows_wall_time();
  } else if ( type == STOPWATCH_CPU )
  {
    return windows_user_cpu_time();
  } else if ( type == STOPWATCH_THREAD )
  {
    return windows_user_thread_time();
  }
#else
#if defined(SC_HIGH_PRECISION_STOPWATCH)
  if ( type == STOPWATCH_WALL )
  {
    struct timespec ts;
    clock_gettime( CLOCK_REALTIME, &ts );
    stopwatch_t::time_point_t out;
    out.sec = int64_t( ts.tv_sec );
    out.usec = int64_t( ts.tv_nsec / 1000 );
    return out;
  }
  else if ( type == STOPWATCH_CPU )
  {
    struct rusage ru;
    getrusage( RUSAGE_SELF, &ru );
    stopwatch_t::time_point_t out;
    out.sec = ru.ru_utime.tv_sec;
    out.usec = ru.ru_utime.tv_usec;
    return out;
  }
  else if ( type == STOPWATCH_THREAD )
  {
    struct rusage ru;
    getrusage( RUSAGE_THREAD, &ru );
    stopwatch_t::time_point_t out;
    out.sec = ru.ru_utime.tv_sec;
    out.usec = ru.ru_utime.tv_usec;
    return out;
  }
#else
  if ( type == STOPWATCH_WALL ||
      type == STOPWATCH_THREAD )
  {
    struct timeval tv;
    gettimeofday( &tv, nullptr );
    stopwatch_t::time_point_t out;
    out.sec = int64_t( tv.tv_sec );
    out.usec = int64_t( tv.tv_usec );
    return out;
  }
  else if ( type == STOPWATCH_CPU )
  {
    stopwatch_t::time_point_t out;
    out.sec = 0;
    out.usec = int64_t( clock() ) * 1e6 / CLOCKS_PER_SEC;
    return out;
  }
#endif
#endif
  assert( false && "stopwatch type not handled" );
  return time_point_t();
}

/// Current time value
double stopwatch_t::current()
{
  return time_point_to_sec( _current );
}

/// Append elapsed time since last start.
void stopwatch_t::accumulate()
{
  time_point_t n( now() );
  _current.sec += ( n.sec - _start.sec );
  _current.usec += ( n.usec - _start.usec );
}

/// Time elapse since start
double stopwatch_t::elapsed()
{
  time_point_t n( now() );
  return time_point_to_sec( time_point_t
  { n.sec - _start.sec, n.usec - _start.usec } );
}

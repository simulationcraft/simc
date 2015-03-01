// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "concurrency.hpp"
#include <iostream>

// CPU thread detection
#if defined( SC_WINDOWS )
// Need sysinfoapi.h, but MSVC may complain. windows.h works for all compliers
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/sysctl.h>
#endif


// Cross-Platform Support for Multi-Threading ===============================

#if defined( NO_THREADS )
// mutex_t::native_t ========================================================

class mutex_t::native_t
{
public:
  void lock()   {}
  void unlock() {}
};

// sc_thread_t::native_t ====================================================

class sc_thread_t::native_t
{
public:
  void launch( sc_thread_t* thr, priority_e ) { thr -> run(); }
  void join() {}
  void set_priority( priority_e  ) {}
  static void set_calling_thread_priority( priority_e ) {}

  static void sleep_seconds( double t )
  {
    std::time_t finish = std::time( 0 ) + t;
    while ( std::time( 0 ) < finish )
      ;
  }

};

#elif defined( SC_WINDOWS ) && ! defined( _POSIX_THREADS )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <process.h>

// mutex_t::native_t ========================================================

class mutex_t::native_t : public nonmoveable
{
  HANDLE mutex_;

public:
  native_t() : mutex_( CreateMutex( 0, FALSE, NULL ) ) {  }
  ~native_t() { CloseHandle( mutex_ ); }

  void lock()   { WaitForSingleObject( mutex_, INFINITE ); }
  void unlock() { ReleaseMutex( mutex_ ); }

  HANDLE primitive() { return mutex_; }
};

namespace { // unnamed namespace
/* Convert our priority enumerations to WinAPI Thread Priority values
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms686277(v=vs.85).aspx
 */
int translate_thread_priority( sc_thread_t::priority_e prio )
{
  switch ( prio )
  {
  case sc_thread_t::NORMAL: return THREAD_PRIORITY_NORMAL;
  case sc_thread_t::ABOVE_NORMAL: return THREAD_PRIORITY_HIGHEST-1;
  case sc_thread_t::BELOW_NORMAL: return THREAD_PRIORITY_LOWEST+1;
  case sc_thread_t::HIGHEST: return THREAD_PRIORITY_HIGHEST;
  case sc_thread_t::LOWEST: return THREAD_PRIORITY_IDLE;
  default: assert( false && "invalid thread priority" ); break;
  }
  return THREAD_PRIORITY_NORMAL;
}

} // unnamed namespace

// sc_thread_t::native_t ====================================================

class sc_thread_t::native_t
{
  HANDLE handle;

#if SC_GCC >= 40204
  __attribute__( ( force_align_arg_pointer ) )
#endif
  static unsigned WINAPI execute( LPVOID t )
  {
    static_cast<sc_thread_t*>( t ) -> run();
    return 0;
  }

public:
  void launch( sc_thread_t* thr, priority_e prio )
  {
    // MinGW wiki suggests using _beginthreadex over CreateThread,
    // and there's no reason NOT to do so with MSVC.
    handle = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, execute,
                                                       thr, 0, NULL ) );
    set_priority( prio );
  }

  void set_priority( priority_e prio )
  { set_thread_priority( handle, prio ); }

  static void set_calling_thread_priority( priority_e prio )
  { set_thread_priority( GetCurrentThread(), prio ); }

  void join()
  {
    WaitForSingleObject( handle, INFINITE );
    CloseHandle( handle );
  }

  static void sleep_seconds( double t )
  { ::Sleep( ( DWORD ) t * 1000 ); }

private:
  static void set_thread_priority( HANDLE handle, priority_e prio )
  {
    if( !SetThreadPriority( handle, translate_thread_priority( prio ) ) )
    {
      std::cerr << "Could not set process priority: " << GetLastError() << "\n";
    }
  }
};

#elif defined( _LIBCPP_VERSION ) || ( defined( _POSIX_THREADS ) && _POSIX_THREADS > 0 ) || defined( _GLIBCXX_HAVE_GTHR_DEFAULT ) || defined( _GLIBCXX__PTHREADS ) || defined( _GLIBCXX_HAS_GTHREADS )
// POSIX
#include <pthread.h>
#include <cstdio>
#include <unistd.h>
#include <errno.h>

// mutex_t::native_t ========================================================

class mutex_t::native_t : public nonmoveable
{
  pthread_mutex_t m;

public:
  native_t()    { pthread_mutex_init( &m, NULL ); }
  ~native_t()   { pthread_mutex_destroy( &m ); }

  void lock()   { pthread_mutex_lock( &m ); }
  void unlock() { pthread_mutex_unlock( &m ); }
  pthread_mutex_t* primitive() { return &m; }
};

// sc_thread_t::native_t ====================================================

class sc_thread_t::native_t
{
  pthread_t t;

  static void* execute( void* t )
  {
    static_cast<sc_thread_t*>( t ) -> run();
    return NULL;
  }

public:
  void launch( sc_thread_t* thr, priority_e prio )
  {
    int rc = pthread_create( &t, NULL, execute, thr );
    if ( rc != 0 )
    {
      perror( "Could not create thread." );
      std::abort();
    }
    set_priority( prio );
  }

  void join() { pthread_join( t, NULL ); }

  void set_priority( priority_e prio )
  { set_thread_priority( t, prio ); }

  static void set_calling_thread_priority( priority_e prio )
  { set_thread_priority( pthread_self(), prio ); }

  static void sleep_seconds( double t )
  { ::sleep( ( unsigned int )t ); }

private:
  static void set_thread_priority( pthread_t t, priority_e prio )
  {
    pthread_attr_t attr;
    int sched_policy;
    if ( pthread_attr_init(&attr) != 0 )
    {
      perror( "Could not init default thread attributes");
      return;
    }
    if (pthread_attr_getschedpolicy(&attr, &sched_policy) != 0 )
    {
      if ( errno != 0 )
      {
        perror( "Could not get schedule policy");
      }
      pthread_attr_destroy(&attr);
      return;
    }

    // Translate our priority enum to posix priority values
    int prio_min = sched_get_priority_min(sched_policy);
    int prio_max = sched_get_priority_max(sched_policy);
    int posix_prio = static_cast<int>( prio ) / 7.0 * ( prio_max - prio_min );
    posix_prio += prio_min;

    sched_param sp;
    sp.sched_priority = posix_prio;

    if ( pthread_setschedparam( t, sched_policy, &sp) != 0 )
    {
      if ( errno != 0 )
      {
        perror( "Could not set thread priority" );
      }
    }

    pthread_attr_destroy(&attr);
  }
};

#else
#error "Unable to detect thread API."
#endif

// mutex_t::mutex_t() =======================================================

mutex_t::mutex_t() : native_handle( new native_t() )
{}

// mutex_t::~mutex_t() ======================================================

mutex_t::~mutex_t()
{ delete native_handle; }

// mutex_t::lock() ==========================================================

void mutex_t::lock()
{ native_handle -> lock(); }

// mutex_t::unlock() ========================================================

void mutex_t::unlock()
{ native_handle -> unlock(); }

// // sc_thread_t::sc_thread_t() ===============================================

sc_thread_t::sc_thread_t() : native_handle( new native_t() )
{}

// sc_thread_t::~sc_thread_t() ==============================================

sc_thread_t::~sc_thread_t()
{ delete native_handle; }

// sc_thread_t::launch() ====================================================

void sc_thread_t::launch( priority_e prio )
{ native_handle -> launch( this, prio ); }


void sc_thread_t::set_priority( priority_e prio )
{ native_handle -> set_priority( prio ); }

void sc_thread_t::set_calling_thread_priority( priority_e prio )
{ native_t::set_calling_thread_priority( prio ); }

// sc_thread_t::wait() ======================================================

void sc_thread_t::wait()
{ native_handle -> join(); }

void sc_thread_t::sleep_seconds( double t )
{ native_t::sleep_seconds( t ); }

/**
 * Get the number of concurrent threads supported by the CPU.
 */
int sc_thread_t::cpu_thread_count()
{
#if defined( SC_WINDOWS )
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
// OS X uses systemctl() to fetch the thread count for the CPU. This returns 8
// (i.e., the logical thread count) on Hyperthreading enabled machines.
#elif defined( SC_OSX )
  int32_t n_threads = -1;
  size_t sizeof_n_threads = sizeof( int32_t );
  int ret = sysctlbyname( "machdep.cpu.thread_count",
      static_cast<void*>( &n_threads ),
      &( sizeof_n_threads ),
      NULL,
      0 );

  // Error, return 0
  if ( ret == -1 )
  {
    return 0;
  }
  else
  {
    return n_threads;
  }
#endif // SC_WINDOWS
  return 0;
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// Cross-Platform Support for Multi-Threading ===============================

#if defined( NO_THREADS )
// mutex_t::native_t ========================================================

class mutex_t::native_t
{
public:
  void lock()   {}
  void unlock() {}
};

// thread_t::native_t =======================================================

class thread_t::native_t
{
public:
  void launch( thread_t* thr ) { thr -> run(); }
  void join() {}

  static void sleep( timespan_t t )
  {
    std::time_t finish = std::time() + t.total_seconds();
    while ( std::time() < finish )
      ;
  }
};

#elif defined( __MINGW32__ ) || defined( _MSC_VER )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <process.h>

// mutex_t::native_t ========================================================

class mutex_t::native_t : public nonmoveable
{
  CRITICAL_SECTION cs;

public:
  native_t()    { InitializeCriticalSection( &cs ); }
  ~native_t()   { DeleteCriticalSection( &cs ); }

  void lock()   { EnterCriticalSection( &cs ); }
  void unlock() { LeaveCriticalSection( &cs ); }
};

// thread_t::native_t =======================================================

class thread_t::native_t
{
  HANDLE handle;

  static unsigned WINAPI execute( LPVOID t )
  {
    static_cast<thread_t*>( t ) -> run();
    return 0;
  }

public:
  void launch( thread_t* thr )
  {
    // MinGW wiki suggests using _beginthreadex over CreateThread,
    // and there's no reason NOT to do so with MSVC.
    handle = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, execute,
                                                       thr, 0, NULL ) );
  }

  void join()
  {
    WaitForSingleObject( handle, INFINITE );
    CloseHandle( handle );
  }

  static void sleep( timespan_t t )
  { ::Sleep( ( DWORD ) t.total_millis() ); }
};

#elif ( defined( _POSIX_THREADS ) && _POSIX_THREADS > 0 ) || defined( _GLIBCXX_HAVE_GTHR_DEFAULT ) || defined( _GLIBCXX__PTHREADS ) || defined( _GLIBCXX_HAS_GTHREADS )
// POSIX
#include <pthread.h>
#include <unistd.h>

// mutex_t::native_t ========================================================

class mutex_t::native_t : public nonmoveable
{
  pthread_mutex_t m;

public:
  native_t()    { pthread_mutex_init( &m, NULL ); }
  ~native_t()   { pthread_mutex_destroy( &m ); }

  void lock()   { pthread_mutex_lock( &m ); }
  void unlock() { pthread_mutex_unlock( &m ); }
};


// thread_t::native_t =======================================================

class thread_t::native_t
{
  pthread_t t;

  static void* execute( void* t )
  {
    static_cast<thread_t*>( t ) -> run();
    return NULL;
  }

public:
  void launch( thread_t* thr ) { pthread_create( &t, NULL, execute, thr ); }
  void join() { pthread_join( t, NULL ); }

  static void sleep( timespan_t t )
  { ::sleep( t.total_seconds() ); }
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

// thread_t::thread_t() =====================================================

thread_t::thread_t() : native_handle( new native_t() )
{}

// thread_t::~thread_t() ====================================================

thread_t::~thread_t()
{ delete native_handle; }

// thread_t::launch() =======================================================

void thread_t::launch()
{ native_handle -> launch( this ); }

// thread_t::wait() =========================================================

void thread_t::wait()
{ native_handle -> join(); }

// thread_t::sleep() =========================================================

void thread_t::sleep( timespan_t t )
{ native_t::sleep( t ); }

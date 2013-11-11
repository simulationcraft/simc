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

// sc_thread_t::native_t ====================================================

class sc_thread_t::native_t
{
public:
  void launch( sc_thread_t* thr ) { thr -> run(); }
  void join() {}
  void set_priority( priority_e  ) {}
  static void set_calling_thread_priority( priority_e ) {}

  static void sleep( timespan_t t )
  {
    std::time_t finish = std::time() + t.total_seconds();
    while ( std::time() < finish )
      ;
  }

};

#elif defined( SC_WINDOWS )
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

namespace { // unnamed namespace
/* Convert our priority enumerations to WinAPI Thread Priority values
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms686277(v=vs.85).aspx
 */
int translate_thread_priority( sc_thread_t::priority_e prio )
{
  switch ( prio )
  {
  case sc_thread_t::NORMAL: return THREAD_PRIORITY_NORMAL;
  case sc_thread_t::ABOVE_NORMAL: return THREAD_PRIORITY_ABOVE_NORMAL;
  case sc_thread_t::BELOW_NORMAL: return THREAD_PRIORITY_BELOW_NORMAL;
  case sc_thread_t::HIGHEST: return THREAD_PRIORITY_HIGHEST;
  case sc_thread_t::LOWEST: return THREAD_PRIORITY_LOWEST;
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

  static void sleep( timespan_t t )
  { ::Sleep( ( DWORD ) t.total_millis() ); }

private:
  static void set_thread_priority( HANDLE handle, priority_e prio )
  {
    if( !SetThreadPriority( handle, translate_thread_priority( prio ) ) )
    {
      std::cerr << "Could not set process priority: " << GetLastError() << "\n";
    }
  }
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

  static void sleep( timespan_t t )
  { ::sleep( ( unsigned int )t.total_seconds() ); }

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
      perror( "Could not get schedule policy");
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
      perror( "Could not set thread priority" );
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

// sc_thread_t::sc_thread_t() ===============================================

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

// sc_thread_t::sleep() =====================================================

void sc_thread_t::sleep( timespan_t t )
{ native_t::sleep( t ); }

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "concurrency.hpp"
#include <iostream>

// Cross-Platform Support for Multi-Threading ===============================

#if defined( NO_THREADS )
// mutex_t::native_t ========================================================

class mutex_t::native_t
{
public:
  void lock()   {}
  void unlock() {}
};

// condition_variable_t::native_t ===========================================

class condition_variable_t::native_t
{
public:
  native_t( mutex_t* )
  { }
  void wait() { }
  void signal() { }
  void broadcast() { }
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
/*
#if ! defined( __MINGW32__ )
class mutex_t::native_t : public nonmoveable
{
  CRITICAL_SECTION cs;

public:
  native_t()    { InitializeCriticalSection( &cs ); }
  ~native_t()   { DeleteCriticalSection( &cs ); }

  void lock()   { EnterCriticalSection( &cs ); }
  void unlock() { LeaveCriticalSection( &cs ); }

  PCRITICAL_SECTION primitive() { return &cs; }
};
#else
*/
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
//#endif /* __MINGW32 __ */

// condition_variable_t::native_t ===========================================
/*
#if ! defined( __MINGW32__ )
class condition_variable_t::native_t : public nonmoveable
{
  CONDITION_VARIABLE cv;
  PCRITICAL_SECTION  cs;

public:
  native_t( mutex_t* m ) : cs( m -> native_mutex() -> primitive() )
  { InitializeConditionVariable( &cv ); }

  ~native_t()
  { }

  void wait()
  { SleepConditionVariableCS( &cv, cs, INFINITE ); }

  void signal()
  { WakeConditionVariable( &cv ); }

  void broadcast()
  { WakeAllConditionVariable( &cv ); }
};
#else
*/
// Emulated condition variable for mingw using win32 thread model. Adapted from
// http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
class condition_variable_t::native_t : public nonmoveable
{
  HANDLE  external_mutex_;
  int waiters_count_;
  CRITICAL_SECTION waiters_count_lock_;
  HANDLE sema_;
  HANDLE waiters_done_;
  size_t was_broadcast_;

public:
  native_t( mutex_t* m ) : 
    external_mutex_( m -> native_mutex() -> primitive() ),
    waiters_count_( 0 ),
    sema_( CreateSemaphore( NULL, 0, 0x7fffffff, NULL ) ),
    waiters_done_( CreateEvent( NULL, FALSE, FALSE, NULL ) ),
    was_broadcast_( 0 )
  {
    InitializeCriticalSection( &waiters_count_lock_ );
  }

  void wait()
  {
    // Increase waiters
    EnterCriticalSection( &waiters_count_lock_ );
    waiters_count_++;
    LeaveCriticalSection( &waiters_count_lock_ );

    // Block on semaphore
    SignalObjectAndWait( external_mutex_, sema_, INFINITE, FALSE );

    // Reduce waiters by one, since we are unblocked
    EnterCriticalSection( &waiters_count_lock_ );
    waiters_count_--;

    int last_waiter = was_broadcast_ && waiters_count_ == 0;

    LeaveCriticalSection( &waiters_count_lock_ );

    // If we are the last object to wait on a broadcast() call, 
    // lets all others proceed, before we do
    if ( last_waiter )
      // Wait on waiters_done_ and acquire external mutex
      SignalObjectAndWait( waiters_done_, external_mutex_, INFINITE, FALSE );
    else
      // Ensure we re-acquire the external mutex
      WaitForSingleObject( external_mutex_, INFINITE );
  }

  // External mutex for this conditional variable MUST BE acquired by the 
  // thread that calls signal().
  void signal()
  {
    EnterCriticalSection( &waiters_count_lock_ );
    int have_waiters = waiters_count_ > 0;
    LeaveCriticalSection( &waiters_count_lock_ );

    // Release single thread if we have anyone blocking on the 
    // semaphore
    if ( have_waiters )
      ReleaseSemaphore( sema_, 1, 0 );
  }

  // External mutex for this conditional variable MUST BE acquired by the 
  // thread that calls broadcast().
  void broadcast()
  {
    EnterCriticalSection( &waiters_count_lock_ );
    int have_waiters = 0;

    if ( waiters_count_ > 0 )
    {
      was_broadcast_ = 1;
      have_waiters = 1;
    }

    if ( have_waiters )
    {
      // Release everyone waiting on sema_
      ReleaseSemaphore( sema_, waiters_count_, 0 );

      LeaveCriticalSection( &waiters_count_lock_ );

      // Wait until the last thread has been unblocked 
      // to ensure fairness.
      WaitForSingleObject( waiters_done_, INFINITE );
      was_broadcast_ = 0;
    }
    else
      LeaveCriticalSection( &waiters_count_lock_ );
  }
};
//#endif /* __MINGW32__ */

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

// condition_variable_t::native_t ===========================================

class condition_variable_t::native_t : public nonmoveable
{
  pthread_cond_t   cv;
  pthread_mutex_t* m;

public:
  native_t( mutex_t* mutex ) : m( mutex -> native_mutex() -> primitive() )
  { pthread_cond_init( &cv, 0 ); }

  ~native_t()
  { pthread_cond_destroy( &cv ); }

  void wait()
  { pthread_cond_wait( &cv, m ); }

  void signal()
  { pthread_cond_signal( &cv ); }

  void broadcast()
  { pthread_cond_broadcast( &cv ); }
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

// condition_variable_t::condition_variable_t() =============================

condition_variable_t::condition_variable_t( mutex_t* m ) : 
  native_handle( new native_t( m ) )
{ }

// condition_variable_t::~condition_variable_t() ============================

condition_variable_t::~condition_variable_t()
{ delete native_handle; }

// condition_variable_t::wait() =============================================

void condition_variable_t::wait()
{ native_handle -> wait(); }

// condition_variable_t::signal() ===========================================

void condition_variable_t::signal()
{ native_handle -> signal(); }

// condition_variable_t::broadcast() ========================================

void condition_variable_t::broadcast()
{ native_handle -> broadcast(); }

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

void sc_thread_t::sleep_seconds( double t )
{ native_t::sleep_seconds( t ); }

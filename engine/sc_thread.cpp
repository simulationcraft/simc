// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

static bool thread_initialized = false;

// Cross-Platform Support for Multi-Threading ===============================

#if defined( NO_THREADS )

// ==========================================================================
// NO MULTI-THREADING SUPPORT
// ==========================================================================

// thread_t::init ===========================================================

void thread_t::init() {}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  sim -> iterate();  // Run sequentially in foreground.
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim ) {}

// thread_t::mutex_init =====================================================

void thread_t::mutex_init( void*& mutex ) {}

// thread_t::mutex_lock =====================================================

void thread_t::mutex_lock( void* mutex ) {}

// thread_t::mutex_unlock ===================================================

void thread_t::mutex_unlock( void* mutex ) {}

#elif defined( __MINGW32__ )

// ==========================================================================
// MULTI-THREADING FOR WINDOWS (MinGW Only)
// ==========================================================================

#include <windows.h>
#include <process.h>

static CRITICAL_SECTION global_mutex;

// thread_execute ===========================================================

static unsigned WINAPI thread_execute( LPVOID sim )
{
  ( ( sim_t* ) sim ) -> iterate();

  return 0;
}

// thread_t::init ===========================================================

void thread_t::init()
{
  InitializeCriticalSection( &global_mutex );
  thread_initialized = true;
}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  assert( thread_initialized );
  HANDLE* handle = new HANDLE();
  sim -> thread_handle = ( void* ) handle;
  //MinGW wiki suggests using _beginthreadex over CreateThread
  *handle = ( HANDLE )_beginthreadex( NULL, 0, thread_execute, ( void* ) sim, 0, NULL );
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
  HANDLE* handle = ( HANDLE* ) ( sim -> thread_handle );
  WaitForSingleObject( *handle, INFINITE );
  if ( handle ) delete handle;
  sim -> thread_handle = NULL;
}

// thread_t::mutex_init =====================================================

void thread_t::mutex_init( void*& mutex )
{
  EnterCriticalSection( &global_mutex );

  if ( ! mutex )
  {
    CRITICAL_SECTION* cs = new CRITICAL_SECTION();
    InitializeCriticalSection( cs );
    mutex = cs;
  }

  LeaveCriticalSection( &global_mutex );
}

// thread_t::mutex_lock =====================================================

void thread_t::mutex_lock( void*& mutex )
{
  if ( ! mutex ) mutex_init( mutex );
  EnterCriticalSection( ( CRITICAL_SECTION* ) mutex );
}

// thread_t::mutex_unlock ===================================================

void thread_t::mutex_unlock( void*& mutex )
{
  LeaveCriticalSection( ( CRITICAL_SECTION* ) mutex );
}

#elif defined( _MSC_VER )

// ==========================================================================
// MULTI-THREADING FOR WINDOWS (MS Visual C++ Only)
// ==========================================================================

#include <windows.h>

static CRITICAL_SECTION global_mutex;

// thread_execute ===========================================================

static DWORD WINAPI thread_execute( __in LPVOID sim )
{
  ( ( sim_t* ) sim ) -> iterate();

  return 0;
}

// thread_t::init ===========================================================

void thread_t::init()
{
  InitializeCriticalSection( &global_mutex );
  thread_initialized = true;
}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  assert( thread_initialized );
  HANDLE* handle = new HANDLE();
  sim -> thread_handle = ( void* ) handle;
  *handle = CreateThread( NULL, 0, thread_execute, ( void* ) sim, 0, NULL );
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
  HANDLE* handle = ( HANDLE* ) ( sim -> thread_handle );
  WaitForSingleObject( *handle, INFINITE );
  if ( handle ) delete handle;
  sim -> thread_handle = NULL;
}

// thread_t::mutex_init =====================================================

void thread_t::mutex_init( void*& mutex )
{
  EnterCriticalSection( &global_mutex );

  if ( ! mutex )
  {
    CRITICAL_SECTION* cs = new CRITICAL_SECTION();
    InitializeCriticalSection( cs );
    mutex = cs;
  }

  LeaveCriticalSection( &global_mutex );
}

// thread_t::mutex_lock =====================================================

void thread_t::mutex_lock( void*& mutex )
{
  if ( ! mutex ) mutex_init( mutex );
  EnterCriticalSection( ( CRITICAL_SECTION* ) mutex );
}

// thread_t::mutex_unlock ===================================================

void thread_t::mutex_unlock( void*& mutex )
{
  LeaveCriticalSection( ( CRITICAL_SECTION* ) mutex );
}

#else

// ==========================================================================
// MULTI_THREADING FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

#include <pthread.h>

static pthread_mutex_t global_mutex;

// thread_execute ===========================================================

static void* thread_execute( void* sim )
{
  ( ( sim_t* ) sim ) -> iterate();

  return NULL;
}

// thread_t::init ===========================================================

void thread_t::init()
{
  pthread_mutex_init( &global_mutex, NULL );
  thread_initialized = true;
}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  assert( thread_initialized );
  pthread_t* pthread = new pthread_t();
  sim -> thread_handle = ( void* ) pthread;
  pthread_create( pthread, NULL, thread_execute, ( void* ) sim );
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
  pthread_t* pthread = ( pthread_t* ) ( sim -> thread_handle );
  pthread_join( *pthread, NULL );
  if ( pthread ) delete pthread;
  sim -> thread_handle = NULL;
}

// thread_t::mutex_init =====================================================

void thread_t::mutex_init( void*& mutex )
{
  pthread_mutex_lock( &global_mutex );

  if ( ! mutex )
  {
    pthread_mutex_t* m = new pthread_mutex_t();
    pthread_mutex_init( m, NULL );
    mutex = m;
  }

  pthread_mutex_unlock( &global_mutex );
}

// thread_t::mutex_lock =====================================================

void thread_t::mutex_lock( void*& mutex )
{
  if ( ! mutex ) mutex_init( mutex );
  pthread_mutex_lock( ( pthread_mutex_t* ) mutex );
}

// thread_t::mutex_unlock ===================================================

void thread_t::mutex_unlock( void*& mutex )
{
  pthread_mutex_unlock( ( pthread_mutex_t* ) mutex );
}

#endif

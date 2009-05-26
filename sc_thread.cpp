// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// Cross-Platform Support for Multi-Threading ===============================

#if defined( NO_THREADS )

// ==========================================================================
// NO MULTI-THREADING SUPPORT
// ==========================================================================

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  sim -> iterate();  // Run sequentially in foreground.
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{}

#elif defined( __MINGW32__ )

// ==========================================================================
// MULTI-THREADING FOR WINDOWS (MinGW Only)
// ==========================================================================

#include <windows.h>
#include <process.h>

static CRITICAL_SECTION global_lock;
static bool global_lock_initialized = false;

// thread_execute ===========================================================

static unsigned WINAPI thread_execute( LPVOID sim )
{
  ( ( sim_t* ) sim ) -> iterate();

  return 0;
}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  if( ! global_lock_initialized )
  {
    InitializeCriticalSection( &global_lock );
    global_lock_initialized = true;
  }
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
  if ( handle )
    delete handle;
  sim -> thread_handle = NULL;
}

// thread_t::lock ===========================================================

void thread_t::lock()
{
  if( global_lock_initialized )
  {
    EnterCriticalSection( &global_lock );
  }
}

// thread_t::unlock =========================================================

void thread_t::unlock()
{
  if( global_lock_initialized )
  {
    LeaveCriticalSection( &global_lock );
  }
}

#elif defined( _MSC_VER )

// ==========================================================================
// MULTI-THREADING FOR WINDOWS (MS Visual C++ Only)
// ==========================================================================

#include <windows.h>

static CRITICAL_SECTION global_lock;
static bool global_lock_initialized = false;

// thread_execute ===========================================================

static DWORD WINAPI thread_execute( __in LPVOID sim )
{
  ( ( sim_t* ) sim ) -> iterate();

  return 0;
}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  if( ! global_lock_initialized )
  {
    InitializeCriticalSection( &global_lock );
    global_lock_initialized = true;
  }
  HANDLE* handle = new HANDLE();
  sim -> thread_handle = ( void* ) handle;
  *handle = CreateThread( NULL, 0, thread_execute, ( void* ) sim, 0, NULL );
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
  HANDLE* handle = ( HANDLE* ) ( sim -> thread_handle );
  WaitForSingleObject( *handle, INFINITE );
  if ( handle )
    delete handle;
  sim -> thread_handle = NULL;
}

// thread_t::lock ===========================================================

void thread_t::lock()
{
  if( global_lock_initialized )
  {
    EnterCriticalSection( &global_lock );
  }
}

// thread_t::unlock =========================================================

void thread_t::unlock()
{
  if( global_lock_initialized )
  {
    LeaveCriticalSection( &global_lock );
  }
}

#else

// ==========================================================================
// MULTI_THREADING FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

#include <pthread.h>

static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
static bool global_lock_initialized = false;

// thread_execute ===========================================================

static void* thread_execute( void* sim )
{
  ( ( sim_t* ) sim ) -> iterate();

  return NULL;
}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  global_lock_initialized = true;
  pthread_t* pthread = new pthread_t();
  sim -> thread_handle = ( void* ) pthread;
  pthread_create( pthread, NULL, thread_execute, ( void* ) sim );
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
  pthread_t* pthread = ( pthread_t* ) ( sim -> thread_handle );
  pthread_join( *pthread, NULL );
  if ( pthread )
    delete pthread;
  sim -> thread_handle = NULL;
}

// thread_t::lock ===========================================================

void thread_t::lock()
{
  if( global_lock_initialized )
  {
    pthread_mutex_lock( &global_lock );
  }
}

// thread_t::unlock =========================================================

void thread_t::unlock()
{
  if( global_lock_initialized )
  {
    pthread_mutex_unlock( &global_lock );
  }
}

#endif

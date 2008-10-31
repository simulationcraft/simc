// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// Cross-Platform Support for Multi-Threading ===============================

#if ! defined( NO_THREADS )

// ==========================================================================
// MULTI_THREAD for UNIX
// ==========================================================================

#if defined( UNIX )

#include <pthread.h>

// thread_execute ===========================================================

static void* thread_execute( void* sim )
{
  ( (sim_t*) sim ) -> iterate();

  return NULL;
}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  pthread_t* pthread = new pthread_t();
  sim -> thread_handle = (void*) pthread;
  pthread_create( pthread, NULL, thread_execute, (void*) sim );
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
  pthread_t* pthread = (pthread_t*) ( sim -> thread_handle );
  pthread_join( *pthread, NULL );
}

// ==========================================================================
// MULTI_THREAD for WINDOWS
// ==========================================================================

#elif defined( WINDOWS )

#include <windows.h>

// thread_execute ===========================================================

static DWORD WINAPI thread_execute( __in LPVOID sim )
{
  ( (sim_t*) sim ) -> iterate();

  return 0;
}

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
#if defined( __MINWG32__ )
  printf( "simcraft: Multi-threading not working for MINGW.  Please removed 'threads=N' from config file.\n" );
  exit(0);
#endif

  HANDLE* handle = new HANDLE();
  sim -> thread_handle = (void*) handle;
  *handle = CreateThread( NULL, 0, thread_execute, (void*) sim, 0, NULL );
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
  HANDLE* handle = (HANDLE*) ( sim -> thread_handle );
  WaitForSingleObject( *handle, INFINITE );
}

// ==========================================================================
// MULTI_THREAD for MAC
// ==========================================================================

#elif defined( MAC )

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  printf( "simcraft: Multi-threading not supported for this platform.  Recompile without 'MULTI_THREAD' defined.\n" );
  exit(0);
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
}

#endif

// ==========================================================================
// No MULTI_THREAD support
// ==========================================================================

#else

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
}

#endif


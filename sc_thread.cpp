// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// Cross-Platform Support for Multi-Threading ===============================

#if defined( NO_THREADS ) || defined( __MINGW32__ )

// ==========================================================================
// NO MULTI-THREADING SUPPORT
// ==========================================================================

// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
}

#elif defined( _MSC_VER )

// ==========================================================================
// MULTI-THREADING FOR WINDOWS (MS Visual C++ Only)
// ==========================================================================

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

#else

// ==========================================================================
// MULTI_THREADING FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

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

#endif

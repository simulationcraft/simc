// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// Cross-Platform Support for Multi-Threading ===============================

namespace thread_impl { // ==================================================
#if defined( NO_THREADS )
// ==========================================================================
// NO MULTI-THREADING SUPPORT
// ==========================================================================

// thread::sleep() ==========================================================

void thread::sleep( int seconds )
{
  const std::time_t finish = std::time() + seconds;
  while ( std::time() < finish )
    ;
}

#elif defined( __MINGW32__ ) || defined( _MSC_VER )
// ==========================================================================
// MULTI-THREADING FOR WINDOWS
// ==========================================================================

#include <process.h>

namespace {
static unsigned WINAPI execute( LPVOID t )
{ static_cast<thread*>( t ) -> run(); return 0; }
}

// thread::launch() =========================================================

void thread::launch()
{
  // MinGW wiki suggests using _beginthreadex over CreateThread,
  // and there's no reason NOT to do so with MSVC.
  handle = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, execute, this, 0, NULL ) );
}

// thread::wait() ===========================================================

void thread::wait()
{ WaitForSingleObject( handle, INFINITE ); CloseHandle( handle ); }

#else // POSIX
// ==========================================================================
// MULTI_THREADING FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

namespace {
static void* execute( void* t )
{ static_cast<thread*>( t ) -> run(); return NULL; }
}

// thread::launch() =========================================================

void thread::launch()
{ pthread_create( &t, NULL, execute, this ); }

#endif

} // namespace thread_impl ==================================================

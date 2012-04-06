// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#if defined( NO_THREADS )
#define THREAD_NONE
#elif defined( __MINGW32__ ) || defined( _MSC_VER )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <process.h>
#define THREAD_WIN32
#elif defined( _POSIX_THREADS ) && _POSIX_THREADS > 0
// POSIX
#include <pthread.h>
#include <unistd.h>
#define THREAD_POSIX
#else
#error "Unable to detect thread API."
#endif

// Cross-Platform Support for Multi-Threading ===============================

namespace thread_impl { // ==================================================
  
  // mutex_native_handle_t ==================================================

  struct mutex_native_handle_t
  {
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)
    CRITICAL_SECTION cs;
#elif defined(THREAD_POSIX)
    pthread_mutex_t m;
#endif
  };

  // thread_native_handle_t =================================================

  struct thread_native_handle_t
  {
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)
    HANDLE handle;
#elif defined(THREAD_POSIX)
    pthread_t t;
#endif
  };

  // static execute =========================================================
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)
  static unsigned WINAPI execute( LPVOID t )
  {
    static_cast<thread_t*>( t ) -> run();
    return 0;
  }
#elif defined(THREAD_POSIX)
  static void* execute( void* t )
  {
    static_cast<thread_t*>( t ) -> run();
    return NULL;
  }
#endif

    

}

// mutex_t::mutex_t() =======================================================

mutex_t::mutex_t()
{
  native_handle = new thread_impl::mutex_native_handle_t();
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)
    InitializeCriticalSection( &native_handle->cs );
#elif defined(THREAD_POSIX)
    pthread_mutex_init( &native_handle->m, NULL );
#endif
}

// mutex_t::~mutex_t() ======================================================

mutex_t::~mutex_t()
{
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)
    DeleteCriticalSection( &native_handle->cs );
#elif defined(THREAD_POSIX)
    pthread_mutex_destroy( &native_handle->m );
#endif
  delete native_handle;
}

// mutex_t::lock() ==========================================================

void mutex_t::lock()
{
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)
    EnterCriticalSection( &native_handle->cs );
#elif defined(THREAD_POSIX)
    pthread_mutex_lock( &native_handle->m );
#endif
}

// mutex_t::unlock() ========================================================

void mutex_t::unlock()
{
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)
    LeaveCriticalSection( &native_handle->cs );
#elif defined(THREAD_POSIX)
    pthread_mutex_unlock( &native_handle->m );
#endif
}

// thread_t::thread_t() =====================================================

thread_t::thread_t()
{
  native_handle = new thread_impl::thread_native_handle_t();
}

// thread_t::~thread_t() ====================================================

thread_t::~thread_t()
{
  delete native_handle;
}

// thread_t::launch() =======================================================

void thread_t::launch()
{
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)

  // MinGW wiki suggests using _beginthreadex over CreateThread,
  // and there's no reason NOT to do so with MSVC.
  native_handle->handle = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, thread_impl::execute, this, 0, NULL ) );
#elif defined(THREAD_POSIX)
  pthread_create( &native_handle->t, NULL, thread_impl::execute, this );
#endif
}

// thread_t::wait() =========================================================

void thread_t::wait()
{
#if defined(THREAD_NONE)
#elif defined(THREAD_WIN32)
  WaitForSingleObject( native_handle->handle, INFINITE );
  CloseHandle( native_handle->handle );
#elif defined(THREAD_POSIX)
  pthread_join( native_handle->t, NULL );
#endif
}

// thread_t::sleep() =========================================================

void thread_t::sleep(int seconds)
{
#if defined(THREAD_NONE)
    const std::time_t finish = std::time() + seconds;
  while ( std::time() < finish )
    ;
#elif defined(THREAD_WIN32)
  ::Sleep( seconds * 1000 );
#elif defined(THREAD_POSIX)
  ::sleep( seconds );
#endif
}

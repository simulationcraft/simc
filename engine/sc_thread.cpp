// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// Cross-Platform Support for Multi-Threading ===============================

#if defined( NO_THREADS )

#include <ctime>

// ==========================================================================
// NO MULTI-THREADING SUPPORT
// ==========================================================================

class mutex_t::impl_t
{
public:
  void lock() {}
  void unlock() {}
};

class thread_t::impl_t
{
public:
  void launch( thread_t& t ) { t.run(); } // Run sequentially in foreground.
  void wait() {}
};

// thread_t::sleep ==========================================================

void thread_t::sleep( int seconds )
{
  const std::time_t finish = std::time() + seconds;
  while ( std::time() < finish )
    ;
}

#elif defined( __MINGW32__ ) || defined( _MSC_VER )

// ==========================================================================
// MULTI-THREADING FOR WINDOWS
// ==========================================================================

#include <windows.h>
#include <process.h>

class mutex_t::impl_t
{
private:
  CRITICAL_SECTION cs;
public:
  impl_t() { InitializeCriticalSection( &cs ); }
  ~impl_t() { DeleteCriticalSection( &cs ); }
  void lock() { EnterCriticalSection( &cs ); }
  void unlock() { LeaveCriticalSection( &cs ); }
};

class thread_t::impl_t
{
private:
  HANDLE handle;

  static unsigned WINAPI beginthreadex_execute( LPVOID t )
  { static_cast<thread_t*>( t ) -> run(); return 0; }

public:
  void launch( thread_t& t )
  {
    // MinGW wiki suggests using _beginthreadex over CreateThread,
    // and there's no reason NOT to do so with MSVC.
    handle = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, beginthreadex_execute, &t, 0, NULL ) );
  }

  void wait() { WaitForSingleObject( handle, INFINITE ); CloseHandle( handle ); }
};

// thread::sleep ============================================================

void thread_t::sleep( int seconds )
{ Sleep( seconds * 1000 ); }

#else // POSIX

// ==========================================================================
// MULTI_THREADING FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

#include <pthread.h>
#include <unistd.h>

class mutex_t::impl_t
{
private:
  pthread_mutex_t m;
public:
  impl_t() { pthread_mutex_init( &m, NULL ); }
  ~impl_t() { pthread_mutex_destroy( &m ); }
  void lock() { pthread_mutex_lock( &m ); }
  void unlock() { pthread_mutex_unlock( &m ); }
};

class thread_t::impl_t
{
private:
  pthread_t t;

  static void* thread_execute( void* t )
  { static_cast<thread_t*>( t ) -> run(); return NULL; }

public:
  void launch( thread_t& thread ) { pthread_create( &t, NULL, thread_execute, &thread ); }
  void wait() { pthread_join( t, NULL ); }
};

// thread_t::sleep ==========================================================

void thread_t::sleep( int seconds )
{ ::sleep( seconds ); }

#endif // POSIX

mutex_t::impl_t mutex_t::global_lock;

// mutex_t::~mutex_t =======================================================

mutex_t::~mutex_t()
{
  // ~mutex_t has to be out-of-line here where impl_t is completely defined
  //  so that auto_ptr can destroy impl properly.
}

// mutex_t::create =========================================================

inline void mutex_t::create()
{
  global_lock.lock();

  if ( ! impl.get() )
    impl.reset( new impl_t );

  global_lock.unlock();
}

// mutex_t::lock ============================================================

void mutex_t::lock()
{
  if ( ! impl.get() ) create();
  impl -> lock();
}

// mutex_t::unlock ==========================================================

void mutex_t::unlock()
{ assert( impl.get() != 0 ); impl -> unlock(); }

// launch ===================================================================

void thread_t::launch()
{
  assert( ! impl );
  if ( !impl )
  {
    impl = new impl_t;
    impl -> launch( *this );
  }
}

// wait =====================================================================

void thread_t::wait()
{
  assert( impl );
  if ( impl )
  {
    impl -> wait();
    delete impl;
    impl = NULL;
  }
}

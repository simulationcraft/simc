// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS ====================================================

static bool thread_initialized = false;

// Cross-Platform Support for Multi-Threading ===============================

#if defined( NO_THREADS )

#include <ctime>

// ==========================================================================
// NO MULTI-THREADING SUPPORT
// ==========================================================================

namespace impl {
  class mutex_t
  {
  public:
    void init() {}
    void lock() {}
    void unlock() {}
    void destroy() {}
  };

  class thread_t
  {
  public:
    void launch( sim_t* sim ) { sim -> iterate(); } // Run sequentially in foreground.
    void wait() {}
  };

  void sleep( int seconds )
  {
    const std::time_t finish = std::time() + seconds;
    while ( std::time() < finish )
      ;
  }
}

#elif defined( __MINGW32__ ) || defined( _MSC_VER )

// ==========================================================================
// MULTI-THREADING FOR WINDOWS
// ==========================================================================

#include <windows.h>
#include <process.h>

namespace impl {
  class mutex_t
  {
  private:
    CRITICAL_SECTION cs;
  public:
    void init() { InitializeCriticalSection( &cs ); }
    void lock() { EnterCriticalSection( &cs ); }
    void unlock() { LeaveCriticalSection( &cs ); }
    void destroy() { DeleteCriticalSection( &cs ); }
  };

  class thread_t
  {
  private:
    HANDLE handle;

#if defined( __MINGW32__ )
    static unsigned WINAPI beginthreadex_execute( LPVOID sim )
    { static_cast<sim_t*>( sim ) -> iterate(); return 0; }

  public:
    void launch( sim_t* sim )
    {
      //MinGW wiki suggests using _beginthreadex over CreateThread
      handle = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, beginthreadex_execute, sim, 0, NULL ) );
    }

#else
    static DWORD WINAPI CreateThread_execute( __in LPVOID sim )
    { static_cast<sim_t*>( sim ) -> iterate(); return 0; }

  public:
    void launch( sim_t* sim )
    {
      handle = CreateThread( NULL, 0, CreateThread_execute, sim, 0, NULL );
    }
#endif

    void wait() { WaitForSingleObject( handle, INFINITE ); CloseHandle( handle ); }
  };

  void sleep( int seconds )
  { Sleep( seconds * 1000 ); }
}

#else // POSIX

// ==========================================================================
// MULTI_THREADING FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

#include <pthread.h>
#include <unistd.h>

namespace impl {
  class mutex_t
  {
  private:
    pthread_mutex_t m;
  public:
    void init() { pthread_mutex_init( &m, NULL ); }
    void lock() { pthread_mutex_lock( &m ); }
    void unlock() { pthread_mutex_unlock( &m ); }
    void destroy() { pthread_mutex_destroy( &m ); }
  };

  class thread_t
  {
  private:
    pthread_t t;
    static void* thread_execute( void* sim )
    { static_cast<sim_t*>( sim ) -> iterate(); return NULL; }

  public:
    void launch( sim_t* sim ) { pthread_create( &t, NULL, thread_execute, sim ); }
    void wait() { pthread_join( t, NULL ); }
  };

  void sleep( int seconds )
  { ::sleep( seconds ); }
}

#endif

static impl::mutex_t global_mutex;
static std::vector<impl::mutex_t*> mutex_list;

} // ANONYMOUS namespace ====================================================

// thread_t::init ===========================================================

void thread_t::init()
{
  if ( ! thread_initialized )
  {
    mutex_list.clear();
    global_mutex.init();
    thread_initialized = true;
  }
}

// thread_t::de_init ========================================================

void thread_t::de_init()
{
  if ( thread_initialized )
  {
    while ( ! mutex_list.empty() )
    {
      impl::mutex_t* m = mutex_list.back();
      mutex_list.pop_back();
      m -> destroy();
      delete m;
    }

    global_mutex.destroy();
    thread_initialized = false;
  }
}

// thread_t::mutex_init =====================================================

void thread_t::mutex_init( void*& mutex )
{
  global_mutex.lock();

  if ( ! mutex )
  {
    impl::mutex_t* m = new impl::mutex_t;
    m -> init();
    mutex_list.push_back( m );
    mutex = m;
  }

  global_mutex.unlock();
}

// thread_t::mutex_lock =====================================================

void thread_t::mutex_lock( void*& mutex )
{
  if ( ! mutex ) mutex_init( mutex );
  static_cast<impl::mutex_t*>( mutex ) -> lock();
}

// thread_t::mutex_unlock ===================================================

void thread_t::mutex_unlock( void*& mutex )
{
  static_cast<impl::mutex_t*>( mutex ) -> unlock();
}

// thread_t::sleep ==========================================================

void thread_t::sleep( int seconds )
{ impl::sleep( seconds ); }


// thread_t::launch =========================================================

void thread_t::launch( sim_t* sim )
{
  assert( thread_initialized );
  impl::thread_t* t = new impl::thread_t;
  sim -> thread_handle = t;
  t -> launch( sim );
}

// thread_t::wait ===========================================================

void thread_t::wait( sim_t* sim )
{
  impl::thread_t* t = static_cast<impl::thread_t*>( sim -> thread_handle );
  if ( t )
  {
    t -> wait();
    sim -> thread_handle = NULL;
    delete t;
  }
}

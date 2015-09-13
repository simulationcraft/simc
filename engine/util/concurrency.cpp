// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "concurrency.hpp"
#include <iostream>

// CPU thread detection
#if defined( SC_WINDOWS )
// Need sysinfoapi.h, but MSVC may complain. windows.h works for all compliers
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/sysctl.h>
#endif


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

class mutex_t::native_t : private nonmoveable
{
  HANDLE mutex_;

public:
  native_t() : mutex_( CreateMutex( 0, FALSE, NULL ) ) {  }
  ~native_t() { CloseHandle( mutex_ ); }

  void lock()   { WaitForSingleObject( mutex_, INFINITE ); }
  void unlock() { ReleaseMutex( mutex_ ); }

  HANDLE primitive() { return mutex_; }
};

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
  void launch( sc_thread_t* thr)
  {
    // MinGW wiki suggests using _beginthreadex over CreateThread,
    // and there's no reason NOT to do so with MSVC.
    handle = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, execute,
                                                       thr, 0, NULL ) );
  }

  void join()
  {
    WaitForSingleObject( handle, INFINITE );
    CloseHandle( handle );
  }

  static void sleep_seconds( double t )
  { ::Sleep( ( DWORD ) t * 1000 ); }
};


#elif defined( _LIBCPP_VERSION ) || ( defined( _POSIX_THREADS ) && _POSIX_THREADS > 0 ) || defined( _GLIBCXX_HAVE_GTHR_DEFAULT ) || defined( _GLIBCXX__PTHREADS ) || defined( _GLIBCXX_HAS_GTHREADS )
// POSIX
#include <pthread.h>
#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
//#include <sys/resource.h>

// mutex_t::native_t ========================================================

class mutex_t::native_t : private nonmoveable
{
  pthread_mutex_t m;

public:
  native_t()    { pthread_mutex_init( &m, nullptr ); }
  ~native_t()   { pthread_mutex_destroy( &m ); }

  void lock()   { pthread_mutex_lock( &m ); }
  void unlock() { pthread_mutex_unlock( &m ); }
  pthread_mutex_t* primitive() { return &m; }
};

// sc_thread_t::native_t ====================================================

class sc_thread_t::native_t
{
  pthread_t t;

  static void* execute( void* t )
  {
    static_cast<sc_thread_t*>( t ) -> run();
    return nullptr;
  }

public:
  void launch( sc_thread_t* thr)
  {
    int rc = pthread_create( &t, nullptr, execute, thr );
    if ( rc != 0 )
    {
      perror( "Could not create thread." );
      std::abort();
    }
  }

  void join() { pthread_join( t, nullptr ); }

  static void sleep_seconds( double t )
  { ::usleep( static_cast<useconds_t >(t * 1000.0) ); }
};


#elif __cplusplus >= 201103L
#include <thread>
#include <mutex>
#include <chrono>

class mutex_t::native_t : private nonmoveable
{
  std::mutex m;

public:
  native_t() : m() {}

  void lock()   { m.lock(); }
  void unlock() { m.unlock(); }
  std::mutex::native_handle_type primitive() { return m.native_handle(); }
};

class sc_thread_t::native_t
{
  std::unique_ptr<std::thread> t;
  static void* execute( sc_thread_t* t )
  {
    t -> run();
    return NULL;
  }

public:
  native_t() :
  t()
  {

  }
  void launch( sc_thread_t* thr, priority_e prio )
  {
    t = std::unique_ptr<std::thread>( new std::thread( &sc_thread_t::native_t::execute, thr ) );
  }

  void join() {
    if ( t ) {
      t -> join();
    }
  }

  void set_priority( priority_e prio )
  {
    // not implemented
  }

  static void set_calling_thread_priority( priority_e prio )
  {
    // not implemented
  }

  static void sleep_seconds( double t )
  { std::this_thread::sleep_for( std::chrono::duration<double>( t ) ); }

private:
};
#else
#error "Unable to detect thread API."
#endif

// mutex_t::mutex_t() =======================================================

mutex_t::mutex_t() :
    native_handle( new native_t() )
{
}

// mutex_t::~mutex_t() ======================================================
// Keep in .cpp file so that std::unique_ptr deleter can see defined native_t class
mutex_t::~mutex_t()
{ }

// mutex_t::lock() ==========================================================

void mutex_t::lock()
{ native_handle -> lock(); }

// mutex_t::unlock() ========================================================

void mutex_t::unlock()
{ native_handle -> unlock(); }

mutex_t::native_t* mutex_t::native_mutex() const
{ return native_handle.get(); }

// // sc_thread_t::sc_thread_t() ===============================================

sc_thread_t::sc_thread_t() : native_handle( new native_t() )
{}

// sc_thread_t::~sc_thread_t() ==============================================
// Keep in .cpp file so that std::unique_ptr deleter can see defined native_t class
sc_thread_t::~sc_thread_t()
{ }

// sc_thread_t::launch() ====================================================

void sc_thread_t::launch()
{ native_handle -> launch( this ); }

// sc_thread_t::wait() ======================================================

void sc_thread_t::wait()
{ native_handle -> join(); }

void sc_thread_t::sleep_seconds( double t )
{ native_t::sleep_seconds( t ); }

/**
 * Get the number of concurrent threads supported by the CPU.
 */
int sc_thread_t::cpu_thread_count()
{
#if defined( SC_WINDOWS )
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
// OS X uses systemctl() to fetch the thread count for the CPU. This returns 8
// (i.e., the logical thread count) on Hyperthreading enabled machines.
#elif defined( SC_OSX )
  int32_t n_threads = -1;
  size_t sizeof_n_threads = sizeof( int32_t );
  int ret = sysctlbyname( "machdep.cpu.thread_count",
      static_cast<void*>( &n_threads ),
      &( sizeof_n_threads ),
      NULL,
      0 );

  // Error, return 0
  if ( ret == -1 )
  {
    return 0;
  }
  else
  {
    return n_threads;
  }
#endif // SC_WINDOWS
  return 0;
}

#if defined(SC_WINDOWS)
#include <windows.h>

DWORD translate_priority( computer_process::priority_e p )
{
  switch ( p )
  {
  case computer_process::NORMAL: return NORMAL_PRIORITY_CLASS;
  case computer_process::ABOVE_NORMAL: return ABOVE_NORMAL_PRIORITY_CLASS;
  case computer_process::BELOW_NORMAL: return BELOW_NORMAL_PRIORITY_CLASS;
  case computer_process::HIGH: return HIGH_PRIORITY_CLASS;
  case computer_process::LOW: return IDLE_PRIORITY_CLASS;
  default: assert( false && "invalid process priority" ); break;
  }
  return NORMAL_PRIORITY_CLASS;
}

void computer_process::set_priority( priority_e p )
{

 DWORD priority = translate_priority(p);
 if ( ! SetPriorityClass(GetCurrentProcess(), priority) )
 {
   DWORD dwError = GetLastError();
   std::cerr << "Failed to set process priority: " << dwError << std::endl;
 }
}

#elif defined(_POSIX_VERSION)
#include <sys/time.h>
#include <sys/resource.h>

int translate_priority( computer_process::priority_e p )
{
  switch ( p )
  {
  case computer_process::NORMAL: return 0;
  case computer_process::ABOVE_NORMAL: return -5;
  case computer_process::BELOW_NORMAL: return 5;
  case computer_process::HIGH: return -10;
  case computer_process::LOW: 10;
  default: assert( false && "invalid process priority" ); break;
  }
  return 0;
}

void computer_process::set_priority( priority_e p )
{

  int priority = translate_priority(p);
  assert( priority <= 19 && priority >= -20 ); // POSIX limits
  if ( setpriority(PRIO_PROCESS, 0 /* calling process */, priority) != 0 )
  {
    perror("Could not set process priority.");
  }
}
#else
void computer_process::set_priority( priority_e p )
{
  // do nothing
}
#endif

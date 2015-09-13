// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "concurrency.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

// C++11 STL multi-threading hook-ups


class mutex_t::native_t : private nonmoveable
{
private:
  std::mutex m;
public:
  native_t() :
    m() {}

  void lock()
  { m.lock(); }

  void unlock()
  { m.unlock(); }

  std::mutex::native_handle_type primitive()
  { return m.native_handle(); }
};

class sc_thread_t::native_t
{
private:
  std::unique_ptr<std::thread> t;

  static void* execute( sc_thread_t* t )
  {
    t -> run();
    return nullptr;
  }
public:
  native_t() :
  t()
  { }

  void launch( sc_thread_t* thr)
  {
    t = std::unique_ptr<std::thread>( new std::thread( &sc_thread_t::native_t::execute, thr ) );
  }

  void join() {
    if ( t ) {
      t -> join();
    }
  }

  static void sleep_seconds( double t )
  {
    auto duration_seconds = std::chrono::duration<double>( t );

    // cast to system_clock duration because of VS problems with its STL implementation
    auto duration_system_clock = std::chrono::duration_cast<std::chrono::system_clock::duration>( duration_seconds );
    std::this_thread::sleep_for( duration_system_clock ); }

  static int cpu_thread_count()
  { return std::thread::hardware_concurrency(); }
private:
};

mutex_t::mutex_t() :
    native_handle( new native_t() )
{
}

mutex_t::~mutex_t()
{
  // Keep in .cpp file so that std::unique_ptr deleter can see defined native_t class
}

void mutex_t::lock()
{ native_handle -> lock(); }

void mutex_t::unlock()
{ native_handle -> unlock(); }

sc_thread_t::sc_thread_t() : native_handle( new native_t() )
{}

sc_thread_t::~sc_thread_t()
{
  // Keep in .cpp file so that std::unique_ptr deleter can see defined native_t class
}

// sc_thread_t::launch() ====================================================

void sc_thread_t::launch()
{ native_handle -> launch( this ); }

// sc_thread_t::wait() ======================================================

void sc_thread_t::wait()
{ native_handle -> join(); }

void sc_thread_t::sleep_seconds( double t )
{ native_t::sleep_seconds( t ); }

/// Get the number of concurrent threads supported by the CPU.
unsigned sc_thread_t::cpu_thread_count()
{ return native_t::cpu_thread_count(); }




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
  case computer_process::LOW: return 10;
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

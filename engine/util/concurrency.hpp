// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

/* Provides platform specific implementations for
 * mutex
 * thread
 * condition variable
 */

#pragma once

#include "config.hpp"
#include "util/generic.hpp"
#include <memory>

#ifndef SC_NO_THREADING
#include <thread>
#endif


class mutex_t : private noncopyable
{
private:
  class native_t;
  std::unique_ptr<native_t> native_handle;

public:
  mutex_t();
  ~mutex_t();

  void lock();
  void unlock();
};

class sc_thread_t : private noncopyable
{
private:
  class native_t;
  std::unique_ptr<native_t> native_handle;
  virtual void run() = 0;
protected:
  sc_thread_t();
  virtual ~sc_thread_t();
public:
#ifndef SC_NO_THREADING
  std::thread::id thread_id() const;
#endif
  void launch();
  void join();
  static void sleep_seconds( double );
  static unsigned cpu_thread_count();
};

class auto_lock_t
{
private:
  mutex_t& mutex;
public:
  auto_lock_t( mutex_t& mutex_ ) : mutex( mutex_ ) { mutex.lock(); }
  ~auto_lock_t() { mutex.unlock(); }
};

#define AUTO_LOCK( m ) auto_lock_t auto_lock( m );

namespace computer_process {

enum priority_e {
  NORMAL,
  ABOVE_NORMAL,
  BELOW_NORMAL,
  HIGH,
  LOW
};
void set_priority( priority_e);

} // computer_process

namespace thread
{
  // Windows (10) needs to promote main thread to higher priority
  void set_main_thread_priority();
}

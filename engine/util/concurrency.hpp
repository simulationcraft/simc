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
#include "generic.hpp"
#include <memory>


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

  native_t* native_mutex() const;
};

class sc_thread_t : private noncopyable
{
private:
  class native_t;
  std::unique_ptr<native_t> native_handle;

protected:
  sc_thread_t();
  virtual ~sc_thread_t();
public:

  virtual void run() = 0;

  void launch();
  void wait();
  static void sleep_seconds( double );
  static int cpu_thread_count();
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

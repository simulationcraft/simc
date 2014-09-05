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


class mutex_t : public noncopyable
{
private:
  class native_t;
  native_t* native_handle;

public:
  mutex_t();
  ~mutex_t();

  void lock();
  void unlock();

  native_t* native_mutex() const
  { return native_handle; }
};

class condition_variable_t : public noncopyable
{
private:
  class native_t;

  native_t* native_handle;

public:
  condition_variable_t( mutex_t* m );
  ~condition_variable_t();

  void wait();

  void signal();
  void broadcast();
};

class sc_thread_t : public noncopyable
{
private:
  class native_t;
  native_t* native_handle;

protected:
  sc_thread_t();
  virtual ~sc_thread_t();
public:
  enum priority_e {
    NORMAL = 3,
    ABOVE_NORMAL = 4,
    BELOW_NORMAL = 2,
    HIGHEST = 5,
    LOWEST = 0
  };
  virtual void run() = 0;

  void launch( priority_e = NORMAL );
  void wait();
  void set_priority( priority_e );

  static void sleep_seconds( double );
  static void set_calling_thread_priority( priority_e );
};

class auto_lock_t
{
private:
  mutex_t& mutex;
public:
  auto_lock_t( mutex_t& mutex_ ) : mutex( mutex_ ) { mutex.lock(); }
  ~auto_lock_t() { mutex.unlock(); }
};

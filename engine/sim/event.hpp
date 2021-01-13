// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "util/timespan.hpp"
#include "util/generic.hpp"
#include "util/format.hpp"

struct actor_t;
struct sim_t;
namespace rng {
    struct rng_t;
}

namespace util {
// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
constexpr unsigned next_power_of_two( unsigned v )
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;

  return v;
}
} // namespace util

// Event ====================================================================
//
// event_t is designed to be a very simple light-weight event transporter and
// as such there are rules of use that must be honored:
//
// (1) The pure virtual execute() method MUST be implemented in the sub-class
// (2) There is 1 * sizeof( event_t ) space available to extend the sub-class
// (3) event_manager_t is responsible for deleting the memory associated with allocated events
// (4) create events throug make_event method
struct event_t : private noncopyable
{
  sim_t& _sim;
  event_t*    next;
  timespan_t  time;
  timespan_t  reschedule_time;
  unsigned    id;
  bool        canceled;
  bool        recycled;
  bool scheduled;
#ifdef ACTOR_EVENT_BOOKKEEPING
  actor_t*    actor;
#endif
  event_t( sim_t& s, actor_t* a = nullptr );
  event_t( actor_t& a );

  // If possible, use one of the two following constructors, which directly
  // schedule the created event
  event_t( sim_t& s, timespan_t delta_time ) : event_t( s )
  {
    schedule( delta_time );
  }
  event_t( actor_t& a, timespan_t delta_time ) : event_t( a )
  {
    schedule( delta_time );
  }

  timespan_t occurs() const  { return ( reschedule_time != timespan_t::zero() ) ? reschedule_time : time; }
  timespan_t remains() const;

  void schedule( timespan_t delta_time );

  void reschedule( timespan_t delta_time );
  sim_t& sim()
  { return _sim; }
  const sim_t& sim() const
  { return _sim; }
  rng::rng_t& rng();

  virtual void execute() = 0; // MUST BE IMPLEMENTED IN SUB-CLASS!
  virtual const char* name() const
  { return "core_event_t"; }

  virtual ~event_t() {}

  template<class T>
  static void cancel( T& e )
  {
    event_t* ref = static_cast<event_t*>(e);
    event_t::cancel( ref );
    e = nullptr;
  }
  static void cancel( event_t*& e );

  friend void format_to( const event_t&, fmt::format_context::iterator );

protected:
  template <typename Event, typename... Args>
  friend Event* make_event( sim_t& sim, Args&&... args );
  static void* operator new( std::size_t size, sim_t& sim );
  static void  operator delete( void*, sim_t& ) { }
  static void  operator delete( void* ) { }
  static void* operator new( std::size_t ) = delete;
};

/**
 * @brief Creates a event
 *
 * This function should be used as the one and only way to create new events. It
 * is used to hide internal placement-new mechanism for efficient event
 * allocation, and also makes sure that any event created is properly added to
 * the event manager (scheduled).
 */
template <typename Event, typename... Args>
inline Event* make_event( sim_t& sim, Args&&... args )
{
  static_assert( std::is_base_of<event_t, Event>::value,
                 "Event must be derived from event_t" );
  static_assert( sizeof( Event ) <= util::next_power_of_two( 2 * sizeof( event_t ) ),
                 "Event type is too big" );
  auto r = new ( sim ) Event( std::forward<Args>(args)... );
  assert( r -> id != 0 && "Event not added to event manager!" );
  return r;
}

template <typename T>
class fn_event_t : public event_t
{
  T fn;

public:
  template <typename U = T>
  fn_event_t( sim_t& s, timespan_t t, U&& fn ) :
    event_t( s, t ), fn( std::forward<U>( fn ) )
  { }

  const char* name() const override
  { return "function_event"; }

  void execute() override
  { fn(); }
};

template <typename T>
class fn_event_repeating_t : public event_t
{
  T fn;
  timespan_t time;
  int n;

public:
  template <typename U = T>
  fn_event_repeating_t( sim_t& s, timespan_t t, U&& fn, int n ) :
    event_t( s, t ), fn( std::forward<U>( fn ) ), time( t ), n( n )
  { }

  const char* name() const override
  { return "repeating_function_event"; }

  void execute() override
  {
    fn();
    if ( n == -1 || --n > 0 )
    {
      make_event<fn_event_repeating_t<T>>( sim(), sim(), time, std::move( fn ), n );
    }
  }
};

template <typename T>
inline event_t* make_event( sim_t& s, timespan_t t, T&& fn )
{
  return make_event<fn_event_t<std::decay_t<T>>>( s, s, t, std::forward<T>( fn ) );
}

template <typename T>
inline event_t* make_repeating_event( sim_t& s, timespan_t t, T&& fn, int n = -1 )
{
  return make_event<fn_event_repeating_t<std::decay_t<T>>>( s, s, t, std::forward<T>( fn ), n );
}

template <typename T>
inline event_t* make_event( sim_t* s, timespan_t t, T&& fn )
{ return make_event( *s, t, std::forward<T>( fn ) ); }

template <typename T>
inline event_t* make_event( sim_t* s, T&& fn )
{ return make_event( *s, timespan_t::zero(), std::forward<T>( fn ) ); }

template <typename T>
inline event_t* make_event( sim_t& s, T&& fn )
{ return make_event( s, timespan_t::zero(), std::forward<T>( fn ) ); }

template <typename T>
inline event_t* make_repeating_event( sim_t* s, timespan_t t, T&& fn, int n = -1 )
{ return make_repeating_event( *s, t, std::forward<T>( fn ), n ); }

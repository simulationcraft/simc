// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "event.hpp"
#include "player/actor.hpp"
#include "sc_sim.hpp" // replace with event manager dependency

// ==========================================================================
// Event
// ==========================================================================

event_t::event_t( sim_t& s, actor_t* a )
  : _sim( s ),
    next( nullptr ),
    time( timespan_t::zero() ),
    reschedule_time( timespan_t::zero() ),
    id( 0 ),
    canceled( false ),
    recycled( false ),
    scheduled( false )
#ifdef ACTOR_EVENT_BOOKKEEPING
    ,
    actor( a )
#endif
{
#ifndef ACTOR_EVENT_BOOKKEEPING
  (void)a;
#endif
}

event_t::event_t( actor_t& a ) : event_t( *a.sim, &a )
{
}

timespan_t event_t::remains() const
{ return occurs() - _sim.event_mgr.current_time; }

rng::rng_t& event_t::rng()
{ return sim().rng(); }

rng::rng_t& event_t::rng() const
{ return sim().rng(); }

/// Placement-new operator for creating events. Do not use in user-code.
void* event_t::operator new( std::size_t size, sim_t& sim )
{
  return sim.event_mgr.allocate_event( size );
}

void event_t::reschedule( timespan_t delta_time )
{
  delta_time += _sim.event_mgr.current_time;

  if ( _sim.debug )
  {
    if ( reschedule_time == timespan_t::zero() )
    {
      _sim.print_debug("Rescheduling event {} ({}) from {} to {}",
          name(), id, time, delta_time );
    }
    else
    {
      _sim.print_debug( "Adjusting reschedule of event {} ({}) from {} to {} time={}",
          name(), id, reschedule_time, delta_time, time );
    }
  }

  reschedule_time = delta_time;
}

// event_t::cancel ==========================================================

void event_t::cancel( event_t*& e )
{
  if ( !e )
    return;

#ifdef ACTOR_EVENT_BOOKKEEPING
  if ( e->_sim.debug && e->actor && !e->canceled )
  {
    e->actor->event_counter--;
    if ( e->actor->event_counter < 0 )
    {
      std::cerr << "event_t::cancel assertion error: e -> player -> events < 0."
                << "event '" << e->name() << "' from '" << e->actor->name()
                << "'.\n";
      assert( false );
    }
  }
#endif

  e->canceled = true;
  e           = nullptr;
}


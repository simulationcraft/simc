// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Event Memory Management
// ==========================================================================

void* core_event_t::allocate( std::size_t size, core_sim_t::event_managment_t& em )
{
  static const std::size_t SIZE = 2 * sizeof( core_event_t );
  assert( SIZE > size ); ( void ) size;

  core_event_t*& list = em.recycled_event_list;

  core_event_t* e = list;

  if ( e )
  {
    list = e -> next;
  }
  else
  {
    e = static_cast<core_event_t*>( malloc( SIZE ) );

    if ( ! e )
    {
      throw std::bad_alloc();
    }
    else
    {
      em.all_events_ever_created.push_back( e );
    }
  }

  return e;
}

void core_event_t::recycle( core_event_t* e )
{
  e -> ~core_event_t();

  core_event_t*& list = e -> _sim.em.recycled_event_list;

  e -> next = list;
  list = e;
}

void core_event_t::release( core_event_t*& list )
{
  while ( list )
  {
    core_event_t* e = list;
    list = e -> next;
    free( e );
  }
}

// ==========================================================================
// Event
// ==========================================================================

core_event_t::core_event_t( core_sim_t& s, const char* n ) :
  _sim( s ), next( nullptr ),  time( timespan_t::zero() ),
  reschedule_time( timespan_t::zero() ),actor( nullptr ), id( 0 ), canceled( false ), name( n )
{}

core_event_t::core_event_t( core_sim_t& s, actor_t* a, const char* n ) :
  _sim( s ), next( nullptr ), time( timespan_t::zero() ),
  reschedule_time( timespan_t::zero() ), actor( a ), id( 0 ), canceled( false ), name( n )
{}

core_event_t::core_event_t( actor_t& a, const char* n ) :
  _sim( *a.sim ), next( nullptr ), time( timespan_t::zero() ),
  reschedule_time( timespan_t::zero() ), actor( &a ), id( 0 ), canceled( false ), name( n )
{}

// event_t::reschedule ======================================================

void core_event_t::reschedule( timespan_t new_time )
{
  reschedule_time = _sim.current_time + new_time;

  if ( _sim.debug )
    _sim.out_debug.printf( "Rescheduling event %s (%d) from %.2f to %.2f",
                name, id, time.total_seconds(), reschedule_time.total_seconds() );
}

// event_t::add_event =======================================================

void core_event_t::add_event( timespan_t delta_time )
{
  _sim.add_event( this, delta_time );
}

// event_t::cancel ==========================================================

void core_event_t::cancel( core_event_t*& e )
{
  if ( ! e ) return;

  if ( actor_t::ACTOR_EVENT_BOOKKEEPING && e -> actor && ! e -> canceled )
  {
    e -> actor -> event_counter--;
    if ( e -> actor -> event_counter < 0 )
    {
      std::cerr << "event_t::cancel assertion error: e -> player -> events < 0."
          << "event '" << e -> name << "' from '" << e -> actor -> name() << "'.\n";
      assert( false );
    }
  }

  e -> canceled = true;
  e = 0;
}

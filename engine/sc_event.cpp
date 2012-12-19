// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Event Memory Management
// ==========================================================================

/* Obtain a event_t* stored in the event_freequeue
 * If there are no free events available, allocate a new one
 */
void* event_freequeue_t::allocate( std::size_t size )
{
  // This override of ::new is ONLY for event_t memory management!
  static const std::size_t SIZE = 2 * sizeof( event_t );
  assert( SIZE > size ); ( void )size;

  if ( !queue.empty() )
  {
    // If there is something in the queue, return its front
    free_event_t* new_event = queue.front();
    queue.pop();
    return new_event;
  }

  return ::operator new( SIZE );
}

/* Store a event_t* in the event_freequeue
 */
void event_freequeue_t::deallocate( void* p )
{
  if ( p )
  {
    // Add event to the end of the queue
    free_event_t* fe = new( p ) free_event_t;
    queue.push( fe );
  }
}

// event_freelist_t::~event_freelist_t ======================================

event_freequeue_t::~event_freequeue_t()
{
  while ( ! queue.empty() )
  {
    free_event_t* p = queue.front();
    delete p;
    queue.pop();
  }
}

// ==========================================================================
// Event
// ==========================================================================

event_t::event_t( sim_t* s, player_t* p, const char* n ) :
  sim( s ), player( p ), name( n ), time( timespan_t::zero() ), reschedule_time( timespan_t::zero() ), id( 0 ), canceled( false )
{ assert( ! p || p -> sim == s ); }

event_t::event_t( player_t* p, const char* n ) :
  sim( p -> sim ), player( p ), name( n ), time( timespan_t::zero() ), reschedule_time( timespan_t::zero() ), id( 0 ), canceled( false )
{}

event_t::event_t( sim_t* s, const char* n ) :
  sim( s ), player( 0 ), name( n ), time( timespan_t::zero() ), reschedule_time( timespan_t::zero() ), id( 0 ), canceled( false )
{}

// event_t::new =============================================================

void* event_t::operator new( std::size_t /* size */ ) throw()
{
  util::fprintf( stderr, "All events must be allocated via: new (sim) event_class_name_t()\n" );
  fflush( stderr );
  abort();
  return NULL;
}

void event_t::reschedule( timespan_t new_time )
{
  reschedule_time = sim -> current_time + new_time;

  if ( sim -> debug ) sim -> output( "Rescheduling event %s (%d) from %.2f to %.2f", name, id, time.total_seconds(), reschedule_time.total_seconds() );

//  if ( ! strcmp( name, "Rabid Expiration" ) ) assert( false );
}

// event_t::cancel_ =========================================================

void event_t::cancel_( event_t* e )
{
  assert( e );
  if ( e -> player && ! e -> canceled )
  {
    e -> player -> events--;
#ifndef NDEBUG
    if ( e -> player -> events < 0 )
    {
      e -> sim -> errorf( "event_t::cancel assertion error: e -> player -> events < 0, event %s from %s.\n", e -> name, e -> player -> name() );
      assert( 0 );
    }
#endif
  }
  e -> canceled = true;
}

// event_t::early_ ==========================================================

void event_t::early_( event_t* e )
{
  assert( e );
  if ( e -> player && ! e -> canceled )
  {
    e -> player -> events--;
    assert( e -> player -> events >= 0 );
  }
  e -> canceled = true;
  e -> execute();
}

// event_t::execute =========================================================

void event_t::execute()
{
  util::printf( "event_t::execute() called for event \"%s\"\n",
                name ? name : "(no name)" );
  assert( 0 );
}

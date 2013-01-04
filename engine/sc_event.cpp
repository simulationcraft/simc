// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Event Memory Management
// ==========================================================================

// event_freelist_t::allocate ===============================================

void* event_freelist_t::allocate( std::size_t size )
{
  // This override of ::new is ONLY for event_t memory management!
  static const std::size_t SIZE = 2 * sizeof( event_t );
  assert( SIZE > size ); ( void )size;

  if ( list )
  {
    free_event_t* new_event = list;
    list = list -> next;
    return new_event;
  }

  return ::operator new( SIZE );
}

// event_freelist_t::deallocate =============================================

void event_freelist_t::deallocate( void* p )
{
  if ( p )
  {
    free_event_t* fe = new( p ) free_event_t;
    fe -> next = list;
    list = fe;
  }
}

// event_freelist_t::~event_freelist_t ======================================

event_freelist_t::~event_freelist_t()
{
  while ( list )
  {
    free_event_t* p = list;
    list = list -> next;
    delete p;
  }
}

// ==========================================================================
// Event
// ==========================================================================

event_t::event_t( sim_t* s, player_t* p, const char* n ) :
  next(), sim( s ), player( p ), name( n ), time( timespan_t::zero() ), reschedule_time( timespan_t::zero() ), id( 0 ), canceled( false )
{ assert( ! p || p -> sim == s ); }

event_t::event_t( player_t* p, const char* n ) :
  next(), sim( p -> sim ), player( p ), name( n ), time( timespan_t::zero() ), reschedule_time( timespan_t::zero() ), id( 0 ), canceled( false )
{}

event_t::event_t( sim_t* s, const char* n ) :
  next(), sim( s ), player( 0 ), name( n ), time( timespan_t::zero() ), reschedule_time( timespan_t::zero() ), id( 0 ), canceled( false )
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

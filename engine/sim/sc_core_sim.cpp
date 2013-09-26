// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace {
/* This basically does the exact same thing as std::set_difference
 * std::set_difference( from.begin(),from.end(), exluding.begin(), exluding.end(), back_inserter( out ) );
 */
void set_difference( const std::vector<core_event_t*>& from, core_event_t* exluding, std::vector<core_event_t*>& out )
{
  for( size_t i = 0, size = from.size(); i < size; ++i )
  {
    core_event_t* e = from[ i ];
    bool found = false;
    for( core_event_t* re = exluding; re; re = re -> next )
    {
      if ( e == re )
      {
        found = true;
        break;
      }
    }
    if ( ! found )
      out.push_back( e );
  }
}
} // end unnamed namespace


core_sim_t::core_sim_t() :
  out_error( std::cerr.rdbuf() ),
  out_debug( nullptr ),
  recycled_event_list( nullptr ),
  timing_wheel(),
  wheel_seconds( 0 ),
  wheel_size( 0 ),
  wheel_mask( 0 ),
  wheel_shift( 5 ),
  timing_slice( 0 ),
  wheel_granularity( 0.0 ),
  all_events_ever_created(),
  events_remaining( 0 ),
  max_events_remaining( 0 ),
  events_processed( 0 ),
  total_events_processed( 0 ),
  global_event_id( 0 ),
  current_time( timespan_t::zero() ),
  event_stopwatch( STOPWATCH_THREAD ),
  monitor_cpu( 0 ),
  debug( false )
{

}

core_sim_t::~core_sim_t()
{
  core_event_t::release( *this );
}

void core_sim_t::flush_events()
{

  /* Instead of iterating over the whole timing wheel,
   * we directly flush the remaining active events == ( all_events_ever_created - recycled_events )
   */
  std::vector<core_event_t*> events_to_flush;
  set_difference( all_events_ever_created, recycled_event_list, events_to_flush );

  for( size_t i = 0, size = events_to_flush.size(); i < size; ++i )
  {
    core_event_t* e = events_to_flush[ i ];
    core_event_t* null_e = e; // necessary evil
    core_event_t::cancel( null_e );
    core_event_t::recycle( e );
  }

  // Clear Timing Wheel
  timing_wheel.assign( timing_wheel.size(), nullptr );

  events_remaining = 0;
  events_processed = 0;
  timing_slice = 0;
  global_event_id = 0;
}

void core_sim_t::reset()
{
  global_event_id = 0;
}

// sim_t::add_event =========================================================

void core_sim_t::add_event( core_event_t* e,
                       timespan_t delta_time )
{
  e -> id   = ++global_event_id;

  if ( delta_time < timespan_t::zero() )
    delta_time = timespan_t::zero();

  if ( unlikely( delta_time > wheel_time ) )
  {
    e -> time = current_time + wheel_time - timespan_t::from_seconds( 1 );
    e -> reschedule_time = current_time + delta_time;
  }
  else
  {
    e -> time = current_time + delta_time;
    e -> reschedule_time = timespan_t::zero();
  }

  // Determine the timing wheel position to which the event will belong
#ifdef SC_USE_INTEGER_TIME
  uint32_t slice = ( uint32_t ) ( e -> time.total_millis() >> wheel_shift ) & wheel_mask;
#else
  uint32_t slice = ( uint32_t ) ( e -> time.total_seconds() * wheel_granularity ) & wheel_mask;
#endif

  // Insert event into the event list at the appropriate time
  core_event_t** prev = &( timing_wheel[ slice ] );
  while ( ( *prev ) && ( *prev ) -> time <= e -> time ) // Find position in the list
  { prev = &( ( *prev ) -> next ); }
  // insert event
  e -> next = *prev;
  *prev = e;

  events_remaining++;
  if ( events_remaining > max_events_remaining ) max_events_remaining = events_remaining;
  if ( actor_t::ACTOR_EVENT_BOOKKEEPING && e -> actor ) e -> actor -> event_counter++;


  if ( debug )
    out_debug.printf( "Add Event: %s %.2f %.2f %d %s",
                    e -> name, e -> time.total_seconds(),
                    e -> reschedule_time.total_seconds(),
                    e -> id, e -> actor ? e -> actor -> name() : "" );

  if ( actor_t::ACTOR_EVENT_BOOKKEEPING && debug && e -> actor )
  {
    out_debug.printf( "Actor %s has %d scheduled events",
                      e -> actor -> name(), e -> actor -> event_counter );
  }

}

bool core_sim_t::init()
{

  // Timing wheel depth defaults to about 17 minutes with a granularity of 32 buckets per second.
  // This makes wheel_size = 32K and it's fully used.
  if ( wheel_seconds     < 1024 ) wheel_seconds     = 1024; // 2^10 Min to ensure limited wrap-around
  if ( wheel_granularity <=   0 ) wheel_granularity = 32;   // 2^5 Time slices per second

  wheel_time = timespan_t::from_seconds( wheel_seconds );

#ifdef SC_USE_INTEGER_TIME
  wheel_size = ( uint32_t ) ( wheel_time.total_millis() >> wheel_shift );
#else
  wheel_size = ( uint32_t ) ( wheel_seconds * wheel_granularity );
#endif

  // Round up the wheel depth to the nearest power of 2 to enable a fast "mod" operation.
  for ( wheel_mask = 2; wheel_mask < wheel_size; wheel_mask *= 2 ) { continue; }
  wheel_size = wheel_mask;
  wheel_mask--;

  // The timing wheel represents an array of event lists: Each time slice has an event list.
  timing_wheel.resize( wheel_size );

  return true;
}

core_event_t* core_sim_t::next_event()
{
  if ( events_remaining == 0 ) return 0;

  while ( true )
  {
    core_event_t*& event_list = timing_wheel[ timing_slice ];
    if ( event_list )
    {
      core_event_t* e = event_list;
      event_list = e -> next;
      events_remaining--;
      events_processed++;
      return e;
    }

    timing_slice++;
    if ( timing_slice == timing_wheel.size() )
    {
      timing_slice = 0;

      if ( debug )
        out_debug << "Time Wheel turns around.";
    }
  }

  return 0;
}

sc_ostream& sc_ostream::printf( const char* format, ... )
{
  char buffer[ 16384 ];

  va_list fmtargs;
  va_start( fmtargs, format );
  int rval = ::vsnprintf( buffer, sizeof( buffer ), format, fmtargs );
  va_end( fmtargs );

  if ( rval >= 0 )
    assert( static_cast<size_t>( rval ) < sizeof( buffer ) );

  *this << buffer;
  return *this;
}

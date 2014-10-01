// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

core_sim_t::event_managment_t::event_managment_t() :
  events_remaining( 0 ),
  events_processed( 0 ),
  timing_slice( 0 ),
  global_event_id( 0 ),
  timing_wheel(),
  recycled_event_list( nullptr ),
  wheel_seconds( 0 ),
  wheel_size( 0 ),
  wheel_mask( 0 ),
  wheel_shift( 5 ),
  wheel_granularity( 0.0 ),
  wheel_time( timespan_t::zero() ),
  all_events_ever_created()
{
  //preallocate_events( 100 ); // Turns it this isn't a performance gain afterall
  all_events_ever_created.reserve( 100 ); // But this is
}

/* Preallocate a certain amount of events and put them into the recycled list
 */
void core_sim_t::event_managment_t::preallocate_events( unsigned num )
{
  all_events_ever_created.reserve( num );
  for ( size_t i = 0; i < num; ++i )
  {
    core_event_t* e = static_cast<core_event_t*>( malloc( 2 * sizeof( core_event_t ) ) );
    if ( ! e )
    {
      throw std::bad_alloc();
    }
    else
    {
      all_events_ever_created.push_back( e );
      core_event_t* l = recycled_event_list;
      recycled_event_list = e;
      e -> next = l;
    }
  }
}

/* Destructor
 * Release all remaining events.
 */
core_sim_t::event_managment_t::~event_managment_t()
{
  core_event_t::release( recycled_event_list );
}

/* Add new event to the timing wheel
 */
void core_sim_t::event_managment_t::add_event( core_event_t* e,
                                               timespan_t delta_time,
                                               timespan_t current_time )
{
  e -> id = ++global_event_id;

  if ( delta_time < timespan_t::zero() )
    delta_time = timespan_t::zero();

  if ( delta_time > wheel_time )
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
  if ( actor_t::ACTOR_EVENT_BOOKKEEPING && e -> actor ) e -> actor -> event_counter++;

}

/* Flush all remaining active events
 */
void core_sim_t::event_managment_t::flush_events()
{
  /* Instead of iterating over the whole timing wheel,
   * we directly flush the remaining active events == ( all_events_ever_created - recycled_events )
   */
  std::vector<core_event_t*> events_to_flush = get_events_to_flush();

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

/* Initialize Event Manager
 */
void core_sim_t::event_managment_t::init()
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
}

/* Select the next event to process
 */
core_event_t* core_sim_t::event_managment_t::next_event()
{
  if ( events_remaining == 0 )
    return nullptr;

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
      // Time Wheel turns around.
    }
  }

  return nullptr;
}

/* Reset
 */
void core_sim_t::event_managment_t::reset()
{
  global_event_id = 0;
}

/* This basically does the exact same thing as std::set_difference
 * std::set_difference( from.begin(),from.end(), exluding.begin(), exluding.end(), back_inserter( out ) );
 */
std::vector<core_event_t*> core_sim_t::event_managment_t::get_events_to_flush() const
{
  std::vector<core_event_t*> out;
  const std::vector<core_event_t*>& from = all_events_ever_created;
  out.reserve( from.size() );
  const core_event_t* exluding = recycled_event_list;
  for( size_t i = 0, size = from.size(); i < size; ++i )
  {
    core_event_t* e = from[ i ];
    bool found = false;
    for( const core_event_t* re = exluding; re; re = re -> next )
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
  return out;
}

/* Constructor
 */
core_sim_t::core_sim_t() :
  em(),
  current_time( timespan_t::zero() ),
  out_std( *this, &std::cout, sim_ostream_t::no_close() ),
  out_log( *this, &std::cout, sim_ostream_t::no_close() ),
  out_debug(*this, &std::cout, sim_ostream_t::no_close() ),
  debug( false ),
  max_time( timespan_t::zero() ),
  expected_iteration_time( timespan_t::zero() ),
  current_iteration( -1 ),
  iterations( 1000 ),
  max_events_remaining( 0 ),
  total_events_processed( 0 ),
  event_stopwatch( STOPWATCH_THREAD ),
  monitor_cpu( false ),
  canceled( 0 ),
  vary_combat_length( 0.0 ),
  iteration_canceled( false )
{

}

/* Destructor
 */
core_sim_t::~core_sim_t()
{

}

// non-virtual Public Functions

/* Insert a event into the simulations event manager.
 * Please use core_event_t::add_event instead
 */
void core_sim_t::add_event( core_event_t* e,
                       timespan_t delta_time )
{
  em.add_event( e, delta_time, current_time );

  if ( em.events_remaining > max_events_remaining ) max_events_remaining = em.events_remaining;

  if ( debug )
    out_debug.printf( "Add Event: %s time=%.4f rs-time=%.4f id=%d actor=%s",
                    e -> name, e -> time.total_seconds(),
                    e -> reschedule_time.total_seconds(),
                    e -> id, e -> actor ? e -> actor -> name() : "" );

  if ( debug && actor_t::ACTOR_EVENT_BOOKKEEPING && e -> actor )
  {
    out_debug.printf( "Actor %s has %d scheduled events",
                      e -> actor -> name(), e -> actor -> event_counter );
  }

}

/* Simulation Time adjustement coefficient
 */
double core_sim_t::iteration_time_adjust() const
{
  if ( iterations <= 1 )
    return 1.0;

  if ( current_iteration == 0 )
    return 1.0;

  return 1.0 + vary_combat_length * ( ( current_iteration % 2 ) ? 1 : -1 ) * current_iteration / ( double ) iterations;
}

/* Very simple helper function to return the largest expected regular simulation time.
 */
double core_sim_t::expected_max_time() const
{
  return max_time.total_seconds() * ( 1.0 + vary_combat_length );
}

/* Is the (whole) simulation canceled?
 */
bool core_sim_t::is_canceled() const
{
  return canceled;
}

void core_sim_t::cancel_iteration( bool cancel )
{
  if ( cancel && debug )
  {
    out_debug << "Iteration canceled.";
  }

  iteration_canceled = cancel;
}

void core_sim_t::combat( int iteration )
{
  if ( debug )
    out_debug << "Starting Simulator";

  current_iteration = iteration;

  begin_combat();

  while ( core_event_t* e = em.next_event() )
  {
    current_time = e -> time;

    if ( actor_t::ACTOR_EVENT_BOOKKEEPING && e -> actor && ! e -> canceled )
    {
      // Perform actor event bookkeeping first
      e -> actor -> event_counter--;
      if ( e -> actor -> event_counter < 0 )
      {
        util::fprintf( stderr, "sim_t::combat assertion error! canceling event %s leaves negative event count for user %s.\n", e -> name, e -> actor -> name() );
        assert( false );
      }
    }

    if ( e -> canceled )
    {
      if ( debug )
        out_debug.printf( "Canceled event: %s", e -> name );
    }
    else if ( e -> reschedule_time > e -> time )
    {
      reschedule_event( e );
      continue;
    }
    else
    {
      if ( debug )
        out_debug.printf( "Executing event: %s %s", e -> name, e -> actor ? e -> actor -> name() : "" );

      if ( monitor_cpu )
      {
        stopwatch_t& sw = e -> actor ? e -> actor -> event_stopwatch : event_stopwatch;
        sw.mark();
        e -> execute();
        sw.accumulate();
      }
      else
      {
        e -> execute();
      }
    }

    core_event_t::recycle( e );

    if ( iteration_canceled )
      break;

    if ( canceled )
      break;
  }

  end_combat();
}

void core_sim_t::combat_begin()
{ }

void core_sim_t::combat_end()
{
  total_events_processed += em.events_processed;
}

/* Initialize simulator
 */
bool core_sim_t::init()
{
  em.init();

  return true;
}

/* Reset the simulator
 */
void core_sim_t::reset()
{
  if ( debug )
    out_debug << "Resetting Simulator";

  em.reset();
  current_time = timespan_t::zero();
  expected_iteration_time = max_time * iteration_time_adjust();
}

/* Begin Combat
 */
void core_sim_t::begin_combat()
{
  reset();

  if ( debug )
    out_debug << "Combat Begin";

  combat_begin();
}

/* End Combat
 * Call virtual combat_end() before flushing events
 */
void core_sim_t::end_combat()
{
  if ( debug )
    out_debug << "Combat End";

  combat_end();

  if ( debug )
    out_debug << "Flush Events";
  em.flush_events();
}

void core_sim_t::reschedule_event( core_event_t* e )
{
  if ( debug )
    out_debug.printf( "Reschedule Event: %s %d", e -> name, e -> id );

  add_event( e, ( e -> reschedule_time - current_time ) );
}

sc_raw_ostream_t& sc_raw_ostream_t::printf( const char* format, ... )
{
  char buffer[ 4048 ];

  va_list fmtargs;
  va_start( fmtargs, format );
  int rval = ::vsnprintf( buffer, sizeof( buffer ), format, fmtargs );
  va_end( fmtargs );

  assert( rval < 0 || ( static_cast<size_t>( rval ) < sizeof( buffer ) ) );
  (void) rval;

  (*_stream) << buffer;
  return *this;
}

sim_ostream_t& sim_ostream_t::printf( const char* format, ... )
{
  char buffer[ 4048 ];

  va_list fmtargs;
  va_start( fmtargs, format );
  int rval = ::vsnprintf( buffer, sizeof( buffer ), format, fmtargs );
  va_end( fmtargs );

  assert( rval < 0 || ( static_cast<size_t>( rval ) < sizeof( buffer ) ) );
  (void) rval;

  _raw << util::to_string( sim.current_time.total_seconds(), 3 ) << " " << buffer << "\n";
  return *this;
}

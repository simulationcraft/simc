// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "event_manager.hpp"
#include "event.hpp"
#include "util/util.hpp"
#include "sim/sc_sim.hpp"
#include "player/sc_player.hpp"


event_manager_t::event_manager_t( sim_t* s )
  : sim( s ),
    events_remaining( 0 ),
    events_processed( 0 ),
    total_events_processed( 0 ),
    max_events_remaining( 0 ),
    timing_slice( 0 ),
    global_event_id( 1 ),  // start at 1, so we can identify event -> id == 0
                           // meaning a unscheduled event.
    timing_wheel(),
    recycled_event_list( nullptr ),
    wheel_seconds( 0 ),
    wheel_size( 0 ),
    wheel_mask( 0 ),
    wheel_shift( 5 ),
    wheel_granularity( 0.0 ),
    wheel_time( timespan_t::zero() ),
    event_stopwatch(),
#ifdef EVENT_QUEUE_DEBUG
    monitor_cpu( false ),
    max_queue_depth( 0 ),
    n_allocated_events( 0 ),
    n_requested_events( 0 ),
    n_end_insert( 0 ),
    events_traversed( 0 ),
    events_added( 0 )
#else
    monitor_cpu( false ),
    canceled( false )
#endif /* EVENT_QUEUE_DEBUG */
{
  allocated_events.reserve( 100 );
}

// event_manager_t::~event_manager_t ========================================

event_manager_t::~event_manager_t()
{
  while ( recycled_event_list )
  {
    event_t* e          = recycled_event_list;
    recycled_event_list = e->next;
    free( e );
  }
}

// event_manager_t::allocate_event ==========================================

void* event_manager_t::allocate_event( const std::size_t size )
{
  static constexpr std::size_t SIZE = util::next_power_of_two( 2 * sizeof( event_t ) );
  assert( SIZE > size );
  (void)size;

  event_t* e = recycled_event_list;
#ifdef EVENT_QUEUE_DEBUG
  n_requested_events++;
  if ( size >= event_requested_size_count.size() )
  {
    event_requested_size_count.resize( size + 1 );
  }
  event_requested_size_count[ size ]++;
#endif
  if ( e )
  {
    recycled_event_list = e->next;
  }
  else
  {
    e = (event_t*)malloc( SIZE );

    if ( !e )
    {
      throw std::bad_alloc();
    }
    else
    {
#ifdef EVENT_QUEUE_DEBUG
      n_allocated_events++;
#endif
      allocated_events.push_back( e );
    }
  }

  return e;
}

// event_manager_t::recycle_event ===========================================

void event_manager_t::recycle_event( event_t* e )
{
  e->~event_t();
  e->recycled         = true;
  e->next             = recycled_event_list;
  recycled_event_list = e;
}

// event_manager_t::add_event ===============================================

void event_manager_t::add_event( event_t* e, timespan_t delta_time )
{
  assert( e -> next == nullptr );
  e->id = ++global_event_id;

  if ( delta_time < timespan_t::zero() )
    delta_time = timespan_t::zero();

  if ( delta_time > wheel_time )
  {
    e->time = current_time + wheel_time - timespan_t::from_seconds( 1 );
    e->reschedule_time = current_time + delta_time;
  }
  else
  {
    e->time            = current_time + delta_time;
    e->reschedule_time = timespan_t::zero();
  }

  // Determine the timing wheel position to which the event will belong
  // Only valid for integer based timespan_t
  uint32_t slice = static_cast<uint32_t>(
      ( e->time.total_millis() >> wheel_shift ) & wheel_mask );

  // Insert event into the event list at the appropriate time
  event_t** prev = &( timing_wheel[ slice ] );
#ifdef EVENT_QUEUE_DEBUG
  unsigned traversed = 0;
#endif

  while ( ( *prev ) &&
          ( *prev )->time <= e->time )  // Find position in the list
  {
    prev = &( ( *prev )->next );
#ifdef EVENT_QUEUE_DEBUG
    traversed++;
#endif
  }
#ifdef EVENT_QUEUE_DEBUG
  events_added++;
  events_traversed += traversed;
  if ( traversed > max_queue_depth )
  {
    max_queue_depth = traversed;
  }
  if ( traversed >= event_queue_depth_samples.size() )
  {
    event_queue_depth_samples.resize( traversed + 1 );
  }
  event_queue_depth_samples[ traversed ].first++;
  if ( !(*prev) )
  {
    event_queue_depth_samples[ traversed ].second++;
    if ( traversed )
    {
      n_end_insert++;
    }
  }
#endif
  // insert event
  e->next = *prev;
  *prev   = e;

  if ( ++events_remaining > max_events_remaining )
    max_events_remaining = events_remaining;

  if ( sim->debug )
    sim->print_debug( "Add Event: {} time={} reschedule={}", *e, e->time, e->reschedule_time );

#ifdef ACTOR_EVENT_BOOKKEEPING
  if ( sim->debug && e->actor )
  {
    e->actor->event_counter++;
    sim->print_debug( "Actor {} has {} scheduled events", e->actor->name(),
                           e->actor->event_counter );
  }
#endif
}

// event_manager_t::reschedule_event ========================================

void event_manager_t::reschedule_event( event_t* e )
{
  sim->print_debug( "Reschedule Event: {}", *e );

  e -> next = nullptr;
  add_event( e, ( e->reschedule_time - current_time ) );
}

// event_manager_t::execute =================================================

bool event_manager_t::execute()
{
  unsigned n_events = 0U;

  static const unsigned MAX_EVENTS = 500U * (
    sim->single_actor_batch ? 1U : sim->player_no_pet_list.size()
  );

  while ( event_t* e = next_event() )
  {
    if ( e->time == current_time )
    {
      if ( ++n_events == MAX_EVENTS )
      {
        cancel_stuck();
      }
    }
    else
    {
      n_events = 0U;
    }

    current_time = e->time;

#ifdef ACTOR_EVENT_BOOKKEEPING
    if ( sim->debug && e->actor && !e->canceled )
    {
      // Perform actor event bookkeeping first
      e->actor->event_counter--;
      if ( e->actor->event_counter < 0 )
      {
        fmt::print( stderr,
                    "sim_t::combat assertion error! canceling event {} "
                    "leaves negative event count for user {}.\n",
                    e->name(), e->actor->name() );
        assert( false );
      }
    }
#endif

    if ( e->canceled )
    {
      sim->print_debug( "Canceled event: {}", *e );
    }
    else if ( e->reschedule_time > e->time )
    {
      reschedule_event( e );
      continue;
    }
    else
    {
      sim->print_debug( "Executing event: {}", *e );

      if ( monitor_cpu )
      {
#ifdef ACTOR_EVENT_BOOKKEEPING
        auto& sw =
            e->actor ? e->actor->event_stopwatch : event_stopwatch;
#else

        auto& sw = event_stopwatch;
#endif
        sw.mark();
        e->execute();
        sw.accumulate();
      }
      else
      {
        e->execute();
      }
    }

    recycle_event( e );

    if ( canceled )
      break;
  }

  total_events_processed += events_processed;

  return true;
}

/// Schedule event on the event manager
void event_t::schedule( timespan_t delta_time )
{
  assert( !scheduled && "Cannot schedule a event twice." );
  scheduled = true;
  _sim.event_mgr.add_event( this, delta_time );
}
// event_manager_t::cancel ==================================================

void event_manager_t::cancel()
{
  canceled = true;
}

// event_manager_t::flush ===================================================

void event_manager_t::flush()
{
  for ( auto e : allocated_events )
  {
    if ( e->recycled )
      continue;
    event_t* null_e = e;  // necessary evil
    event_t::cancel( null_e );
    recycle_event( e );
  }

  // Clear Timing Wheel
  timing_wheel.assign( timing_wheel.size(), nullptr );
}

// event_manager_t::init ====================================================

void event_manager_t::init()
{
  // Timing wheel depth defaults to about 17 minutes with a granularity of 32
  // buckets per second.
  // This makes wheel_size = 32K and it's fully used.
  if ( wheel_seconds < 1024 )
    wheel_seconds = 1024;  // 2^10 Min to ensure limited wrap-around
  if ( wheel_granularity <= 0 )
    wheel_granularity = 32;  // 2^5 Time slices per second

  wheel_time = timespan_t::from_seconds( wheel_seconds );

  // Only valid for integer based timespan_t
  wheel_size = ( uint32_t )( wheel_time.total_millis() >> wheel_shift );

  // Round up the wheel depth to the nearest power of 2 to enable a fast "mod"
  // operation.
  for ( wheel_mask = 2; wheel_mask < wheel_size; wheel_mask *= 2 )
  {
    continue;
  }
  wheel_size = wheel_mask;
  wheel_mask--;

  // The timing wheel represents an array of event lists: Each time slice has an
  // event list.
  timing_wheel.resize( wheel_size );
}

// event_manager_t::next_event ==============================================

event_t* event_manager_t::next_event()
{
  if ( events_remaining == 0 )
    return nullptr;

  while ( true )
  {
    event_t*& event_list = timing_wheel[ timing_slice ];
    if ( event_list )
    {
      event_t* e = event_list;
      event_list = e->next;
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

// event_manager_t::reset ===================================================

void event_manager_t::reset()
{
  events_remaining = 0;
  events_processed = 0;
  timing_slice     = 0;
  global_event_id  = 0;
  canceled         = false;
  current_time     = timespan_t::zero();
}

// event_manager_t::merge ===================================================

void event_manager_t::merge( event_manager_t& other )
{
  max_events_remaining =
      std::max( max_events_remaining, other.max_events_remaining );
  total_events_processed += other.total_events_processed;
#ifdef EVENT_QUEUE_DEBUG
  events_traversed += other.events_traversed;
  events_added += other.events_added;
  n_allocated_events += other.n_allocated_events;
  n_end_insert += other.n_end_insert;
  n_requested_events += other.n_requested_events;
  if ( other.max_queue_depth > max_queue_depth )
  {
    max_queue_depth = other.max_queue_depth;
  }

  if ( other.event_queue_depth_samples.size() >
       event_queue_depth_samples.size() )
  {
    event_queue_depth_samples.resize( other.event_queue_depth_samples.size() );
  }

  for ( size_t i = 0; i < other.event_queue_depth_samples.size(); ++i )
  {
    event_queue_depth_samples[ i ].first +=
        other.event_queue_depth_samples[ i ].first;
    event_queue_depth_samples[ i ].second +=
        other.event_queue_depth_samples[ i ].second;
  }
  for ( size_t i = 0; i < other.event_requested_size_count.size(); ++i )
  {
    event_requested_size_count[ i ] += other.event_requested_size_count[ i ];
  }

#endif
}

// event_manager_t::cancel_stuck ============================================

void event_manager_t::cancel_stuck()
{
  std::string player_str { "(unknown)" };

  if ( sim->player_no_pet_list.size() == 1 )
  {
    player_str = sim->player_no_pet_list[ 0 ]->name();
  }
  else if ( sim->single_actor_batch )
  {
    player_str = sim->player_no_pet_list[ sim->current_index ]->name();
  }

  fmt::print( stderr,
        "Simulator likely stuck on "
        "thread={}, iteration={}, seed={}, player={}, canceling ...\n", sim->thread_index,
        sim->current_iteration, sim->seed, player_str );

  if ( sim->parent )
  {
    sim->parent->event_mgr.cancel_stuck();
  }
  // Parent (thread 0) processing
  else
  {
    sim->cancel();
    range::for_each( sim->relatives, []( sim_t* relative ) {
      relative->cancel_iteration();
    } );
    sim->cancel_iteration();
  }
}


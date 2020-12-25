// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/timespan.hpp"
#include "util/chrono.hpp"
#include "util/stopwatch.hpp"

#include <cstdint>
#include <vector>

struct event_t;
struct sim_t;

// Event manager
struct event_manager_t
{
  sim_t* sim;
  timespan_t current_time;
  uint64_t events_remaining;
  uint64_t events_processed;
  uint64_t total_events_processed;
  uint64_t max_events_remaining;
  unsigned timing_slice, global_event_id;
  std::vector<event_t*> timing_wheel;
  event_t* recycled_event_list;
  int    wheel_seconds, wheel_size, wheel_mask, wheel_shift;
  double wheel_granularity;
  timespan_t wheel_time;
  std::vector<event_t*> allocated_events;

  stopwatch_t<chrono::thread_clock> event_stopwatch;
  bool monitor_cpu;
  bool canceled;
#ifdef EVENT_QUEUE_DEBUG
  unsigned max_queue_depth, n_allocated_events, n_end_insert, n_requested_events;
  uint64_t events_traversed, events_added;
  std::vector<std::pair<unsigned, unsigned> > event_queue_depth_samples;
  std::vector<unsigned> event_requested_size_count;
#endif /* EVENT_QUEUE_DEBUG */

  event_manager_t( sim_t* );
 ~event_manager_t();
  void* allocate_event( std::size_t size );
  void recycle_event( event_t* );
  void add_event( event_t*, timespan_t delta_time );
  void reschedule_event( event_t* );
  event_t* next_event();
  bool execute();
  void cancel();
  void flush();
  void init();
  void reset();
  void merge( event_manager_t& other );
  void cancel_stuck();
};

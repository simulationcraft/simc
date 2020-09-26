// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/timespan.hpp"
#include "sc_enums.hpp"
#include "util/generic.hpp"
#include "util/string_view.hpp"
#include "util/format.hpp"

#include <string>
#include <memory>
#include <cstdint>

struct action_t;
struct action_state_t;
struct dot_t;
struct event_t;
struct expr_t;
struct player_t;
struct sim_t;

struct dot_t : private noncopyable
{
private:
  sim_t& sim;
  bool ticking;
  timespan_t current_duration;
  timespan_t last_start;
  timespan_t prev_tick_time; // Used for rescheduling when refreshing right before last partial tick
  timespan_t extended_time; // Added time per extend_duration for the current dot application
  timespan_t reduced_time; // Removed time per reduce_duration for the current dot application
  int stack;
public:
  event_t* tick_event;
  event_t* end_event;
  double last_tick_factor;

  player_t* const target;
  player_t* const source;
  action_t* current_action;
  action_state_t* state;
  int num_ticks, current_tick;
  int max_stack;
  timespan_t miss_time;
  timespan_t time_to_tick;
  std::string name_str;

  dot_t(util::string_view n, player_t* target, player_t* source);

  void extend_duration( timespan_t extra_seconds, timespan_t max_total_time = timespan_t::min(),
                        uint32_t state_flags = -1, bool count_refresh = true );
  void extend_duration( timespan_t extra_seconds, uint32_t state_flags )
  {
    extend_duration( extra_seconds, timespan_t::min(), state_flags );
  }
  void reduce_duration( timespan_t remove_seconds, uint32_t state_flags = -1, bool count_as_refresh = true );
  void   refresh_duration(uint32_t state_flags = -1);
  void   reset();
  void   cancel();
  void   trigger(timespan_t duration);
  void   decrement(int stacks);
  void   increment(int stacks);
  void   copy(player_t* destination, dot_copy_e = DOT_COPY_START) const;
  void   copy(dot_t* dest_dot) const;
  // Scale on-going dot remaining time by a coefficient during a tick. Note that this should be
  // accompanied with the correct (time related) scaling information in the action's supporting
  // methods (action_t::tick_time, action_t::composite_dot_ruration), otherwise bad things will
  // happen.
  void   adjust(double coefficient);
  // Alternative to adjust() based on the rogue ability Exsanguinate and how it works with hasted bleeds.
  // It rounds the number of ticks to the nearest integer and reschedules the remaining ticks
  // as full ticks from the end. If one tick would theoretically occur before Exsanguinate, it will
  // happen immediately instead.
  // Note that this should be accompanied with the correct (time related) scaling information in the
  // action's supporting methods (action_t::tick_time, action_t::composite_dot_ruration), otherwise
  // bad things will happen.
  void   adjust_full_ticks(double coefficient);
  static std::unique_ptr<expr_t> create_expression(dot_t* dot, action_t* action, action_t* source_action,
    util::string_view name_str, bool dynamic);

  timespan_t remains() const;
  timespan_t time_to_next_tick() const;
  timespan_t duration() const { return !is_ticking() ? timespan_t::zero() : current_duration; }
  int ticks_left() const;
  double ticks_left_fractional() const;
  const char* name() const { return name_str.c_str(); }
  bool is_ticking() const { return ticking; }
  timespan_t get_extended_time() const { return extended_time; }
  double get_last_tick_factor() const { return last_tick_factor; }
  int current_stack() const { return ticking ? stack : 0; }
  bool at_max_stacks( int mod = 0 ) const { return current_stack() + mod >= max_stack; }

  void tick();
  void last_tick();
  bool channel_interrupt();

  friend void format_to( const dot_t&, fmt::format_context::iterator );
private:
  void tick_zero();
  void schedule_tick();
  void start(timespan_t duration);
  void refresh(timespan_t duration);
  void check_tick_zero(bool start);
  bool is_higher_priority_action_available() const;
  void recalculate_num_ticks();

  struct dot_tick_event_t;
  struct dot_end_event_t;
};
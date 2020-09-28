// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dot.hpp"
#include "action/sc_action.hpp"
#include "action/sc_action_state.hpp"
#include "player/sc_player.hpp"
#include "player/stats.hpp"
#include "sim/sc_expressions.hpp"
#include "sim/sc_sim.hpp"
#include "sim/event.hpp"
#include "util/rng.hpp"

// Dot events

struct dot_t::dot_tick_event_t : public event_t
{
public:
  dot_tick_event_t(dot_t* d, timespan_t time_to_tick);

private:
  virtual void execute() override;
  virtual const char* name() const override
  {
    return "Dot Tick";
  }
  dot_t* dot;
};

// DoT End Event ===========================================================

struct dot_t::dot_end_event_t : public event_t
{
public:
  dot_end_event_t(dot_t* d, timespan_t time_to_end);

private:
  virtual void execute() override;
  virtual const char* name() const override
  {
    return "DoT End";
  }
  dot_t* dot;
};


// ==========================================================================
// Dot
// ==========================================================================

dot_t::dot_t( util::string_view n, player_t* t, player_t* s )
  : sim( *( t->sim ) ),
    ticking( false ),
    current_duration( timespan_t::zero() ),
    last_start( timespan_t::min() ),
    prev_tick_time( timespan_t::zero() ),
    extended_time( timespan_t::zero() ),
    reduced_time( timespan_t::zero() ),
    stack( 0 ),
    tick_event( nullptr ),
    end_event( nullptr ),
    last_tick_factor( -1.0 ),
    target( t ),
    source( s ),
    current_action( nullptr ),
    state( nullptr ),
    num_ticks( 0 ),
    current_tick( 0 ),
    max_stack( 0 ),
    miss_time( timespan_t::min() ),
    time_to_tick( timespan_t::zero() ),
    name_str( n )
{
}

// dot_t::cancel ============================================================

void dot_t::cancel()
{
  if ( !ticking )
    return;

  last_tick();
  reset();
}

// dot_t::extend_duration_seconds ===========================================

void dot_t::extend_duration( timespan_t extra_seconds, timespan_t max_total_time, uint32_t state_flags,
                             bool count_as_refresh )
{
  if ( !ticking )
    return;

  if ( state_flags == (uint32_t)-1 )
    state_flags = current_action->snapshot_flags;

  // Make sure this DoT is still ticking......
  assert( tick_event );
  assert( state );
  current_action->snapshot_internal(
      state, state_flags,
      current_action->type == ACTION_HEAL ? result_amount_type::HEAL_OVER_TIME : result_amount_type::DMG_OVER_TIME );

  if ( max_total_time > timespan_t::zero() )
  {
    timespan_t over_cap = remains() + extra_seconds - max_total_time;
    if ( over_cap > timespan_t::zero() )
      extra_seconds -= over_cap;
  }
  current_duration += extra_seconds;
  extended_time += extra_seconds;

  sim.print_log( "{} extends duration of {} on {} by {} second(s).",
                        *source, *this, *target,
                        extra_seconds );

  assert( end_event && "Dot is ticking but has no end event." );
  timespan_t remains = end_event->remains();
  remains += extra_seconds;
  if ( remains != end_event->remains() )
    end_event->reschedule( remains );

  if ( count_as_refresh )
    current_action->stats->add_refresh( state->target );
}

// dot_t::reduce_duration_seconds ===========================================

void dot_t::reduce_duration( timespan_t remove_seconds, uint32_t state_flags, bool count_as_refresh )
{
  if ( !ticking )
    return;

  sim.print_debug( "{} attempts to reduce duration of {} by {}.", *source, *this, remove_seconds );

  assert(remove_seconds > timespan_t::zero() && "Cannot reduce dot duration by a negative amount of time.");

  if ( state_flags == (uint32_t)-1 )
    state_flags = current_action->snapshot_flags;

  // Make sure this DoT is still ticking......
  assert( tick_event );
  assert( state );
  current_action->snapshot_internal(
      state, state_flags,
      current_action->type == ACTION_HEAL ? result_amount_type::HEAL_OVER_TIME : result_amount_type::DMG_OVER_TIME );

  
  if ( remove_seconds >= remains() )
  {
    cancel();

    sim.print_log( "{} reduces duration of {} on {} by {} second(s), removing it.", *source, *this, *target,
                   remove_seconds );

    return;
  }
  current_duration -= remove_seconds;
  reduced_time -= remove_seconds;

  sim.print_log( "{} reduces duration of {} on {} by {} second(s).", *source, *this, *target, remove_seconds );

  if ( sim.debug )
  {
    sim.print_debug( "{} {} new remains {}.", *source, *this, remains() );
  }

  assert( end_event && "Dot is ticking but has no end event." );
  timespan_t remains = end_event->remains();
  remains -= remove_seconds;
  assert(remains > timespan_t::zero());
  if ( remains != end_event->remains() )
  {
    event_t::cancel( end_event );
    end_event = make_event<dot_end_event_t>( sim, this, remains );
  }
  recalculate_num_ticks();

  if ( count_as_refresh )
    current_action->stats->add_refresh( state->target );
}

// dot_t::refresh_duration ==================================================

void dot_t::refresh_duration( uint32_t state_flags )
{
  if ( !ticking )
    return;

  if ( state_flags == (uint32_t)-1 )
    state_flags = current_action->snapshot_flags;

  current_action->snapshot_internal(
      state, state_flags,
      current_action->type == ACTION_HEAL ? result_amount_type::HEAL_OVER_TIME : result_amount_type::DMG_OVER_TIME );

  refresh( current_action->composite_dot_duration( state ) );

  current_action->stats->add_refresh( state->target );
}

/* Reset Dot
 */
void dot_t::reset()
{
  if ( ticking )
    source->remove_active_dot( state->action->internal_id );

  event_t::cancel( tick_event );
  event_t::cancel( end_event );
  time_to_tick     = timespan_t::zero();
  ticking          = false;
  current_tick     = 0;
  stack            = 0;
  extended_time    = timespan_t::zero();
  ticking          = false;
  miss_time        = timespan_t::min();
  last_start       = timespan_t::min();
  prev_tick_time   = timespan_t::zero();
  current_duration = timespan_t::min();
  last_tick_factor = -1.0;
  if ( state )
    action_state_t::release( state );
}

/* Trigger a dot with given duration.
 * Main function to start/refresh a dot
 */
void dot_t::trigger( timespan_t duration )
{
  assert( duration > timespan_t::zero() &&
          "Dot Trigger with duration <= 0 seconds." );

  current_tick     = 0;
  extended_time    = timespan_t::zero();
  last_tick_factor = 1.0;

  if ( ticking )
  {
    refresh( duration );
  }
  else
  {
    start( duration );
  }
}

void dot_t::decrement( int stacks = 1 )
{
  if ( max_stack == 0 || stack <= 0 )
    return;

  if ( stacks == 0 || stack <= stacks )
  {
    cancel();
  }
  else
  {
    stack -= stacks;

    if ( sim.debug )
      sim.out_debug.printf( "dot %s decremented by %d to %d stacks",
                            name_str.c_str(), stacks, stack );
  }
}

void dot_t::increment(int stacks = 1)
{
  if (max_stack == 0 || stack <= 0 || stack == max_stack)
    return;

  if ( stack + stacks > max_stack )
  {
    stack = max_stack;
  }
  else
  {
    stack += stacks;

    if (sim.debug)
      sim.out_debug.printf("dot %s incremented by %d to %d stacks",
        name_str.c_str(), stacks, stack);
  }
}

// For copying a DoT to a different target.
void dot_t::copy( player_t* other_target, dot_copy_e copy_type ) const
{
  if ( target == other_target )
    return;

  dot_t* other_dot = current_action->get_dot( other_target );
  // Copied dot, with the DOT_COPY_START method cancels the ongoing dot on the
  // target, and then starts a fresh dot on it with the source dot's (copied)
  // state
  if ( copy_type == DOT_COPY_START && other_dot->is_ticking() )
    other_dot->cancel();

  // Shared initialize for the target dot state, independent of the copying
  // method
  action_state_t* target_state = nullptr;
  if ( !other_dot->state )
  {
    target_state     = current_action->get_state( state );
    other_dot->state = target_state;
  }
  else
  {
    target_state = other_dot->state;
    target_state->copy_state( state );
  }
  target_state->target = other_dot->target;
  target_state->action = current_action;

  other_dot->current_action = current_action;

  // Default behavior simply copies the current stat of the source dot
  // (including duration) to the target and starts it
  if ( copy_type == DOT_COPY_START )
  {
    other_dot->trigger( current_duration );
  }
  // If we don't start the copied dot from the beginning, we need to bypass a
  // lot of the dot scheduling logic, and simply do the minimum amount possible
  // to get the dot aligned with the source dot, both in terms of state, as
  // well as the remaining duration, and the remaining ongoing tick time.
  else
  {
    timespan_t new_duration, old_remains = timespan_t::zero();

    // Other dot is ticking, the cloning process will be a refresh
    if ( other_dot->is_ticking() )
    {
      old_remains = other_dot->remains();

      // The new duration is computed through our normal refresh duration
      // method. End result (by default) will be source_remains + min(
      // target_remains, 0.3 * source_remains )
      new_duration = current_action->calculate_dot_refresh_duration(
          other_dot, remains() );

      assert( other_dot->end_event && other_dot->tick_event );

      // Cancel target's ongoing events, we are about to re-do them
      event_t::cancel( other_dot->end_event );
      event_t::cancel( other_dot->tick_event );
    }
    // No target dot ticking, just copy the source's remaining time
    else
    {
      new_duration = remains();

      // Add an active dot on the source, since we are starting a new one
      source->add_active_dot( current_action->internal_id );
    }

    if ( sim.debug )
      sim.out_debug.printf(
          "%s cloning %s from %s to %s: source_remains=%.3f "
          "target_remains=%.3f target_duration=%.3f",
          current_action->player->name(), current_action->name(),
          target->name(), other_target->name(), remains().total_seconds(),
          old_remains.total_seconds(), new_duration.total_seconds() );

    // To compute new number of ticks, we use the new duration, plus the
    // source's ongoing remaining tick time, since we are copying the ongoing
    // tick too
    timespan_t computed_tick_duration = new_duration;
    if ( tick_event && tick_event->remains() > new_duration )
      computed_tick_duration += time_to_tick - tick_event->remains();

    // Aand then adjust some things for ease-of-use for now. The copied dot has
    // its current tick reset to 0, and it's last start time is set to current
    // time.
    //
    // TODO?: A more proper way might be to also copy the source dot's last
    // start time and current tick, in practice it is probably meaningless,
    // though.
    other_dot->last_start       = sim.current_time();
    other_dot->current_duration = new_duration;
    other_dot->current_tick     = 0;
    other_dot->extended_time    = timespan_t::zero();
    other_dot->stack            = stack;
    other_dot->time_to_tick     = time_to_tick;
    other_dot->num_ticks =
        as<int>( std::ceil( computed_tick_duration / time_to_tick ) );

    other_dot->ticking = true;
    other_dot->end_event =
        make_event<dot_end_event_t>( sim, other_dot, new_duration );

    other_dot->last_tick_factor = other_dot->current_action->last_tick_factor(
        other_dot, time_to_tick, computed_tick_duration );

    // The clone may happen on tick, or mid tick. If it happens on tick, the
    // source dot will not have a new tick event scheduled yet, so the tick
    // time has to be based on the action's tick time. If cloning happens mid
    // tick, we just use the remaining tick time of the source for the tick
    // time. Tick time will be recalculated on the next tick, implicitly
    // syncing it to the source's tick time.
    timespan_t tick_time;
    if ( tick_event )
      tick_time = tick_event->remains();
    else
      tick_time = other_dot->current_action->tick_time( other_dot->state );

    other_dot->tick_event =
        make_event<dot_tick_event_t>( sim, other_dot, tick_time );
  }
}

// For duplicating a DoT (creating a 2nd instance) on one target.
void dot_t::copy( dot_t* other_dot ) const
{
  assert( current_action );

  // Shared initialize for the target dot state, independent of the copying
  // method
  action_state_t* target_state = nullptr;
  if ( !other_dot->state )
  {
    target_state     = current_action->get_state( state );
    other_dot->state = target_state;
  }
  else
  {
    target_state = other_dot->state;
    target_state->copy_state( state );
  }
  target_state->target = other_dot->target;
  target_state->action = current_action;

  // If we don't start the copied dot from the beginning, we need to bypass a
  // lot of the dot scheduling logic, and simply do the minimum amount possible
  // to get the dot aligned with the source dot, both in terms of state, as
  // well as the remaining duration, and the remaining ongoing tick time.
  timespan_t new_duration;

  // Other dot is ticking, the cloning process will be a refresh
  if ( other_dot->is_ticking() )
  {
    // The new duration is computed through our normal refresh duration
    // method. End result (by default) will be source_remains + min(
    // target_remains, 0.3 * source_remains )
    new_duration =
        current_action->calculate_dot_refresh_duration( other_dot, remains() );

    assert( other_dot->end_event && other_dot->tick_event );

    // Cancel target's ongoing events, we are about to re-do them
    event_t::cancel( other_dot->end_event );
    event_t::cancel( other_dot->tick_event );
  }
  // No target dot ticking, just copy the source's remaining time
  else
  {
    new_duration = remains();

    // Add an active dot on the source, since we are starting a new one
    source->add_active_dot( other_dot->current_action->internal_id );
  }

  if ( sim.debug )
  {
    sim.print_debug( "{} cloning {} on {} to {}: source_remains={} target_remains={} target_duration={}",
                     *current_action->player, *current_action, *target, *other_dot, remains(), other_dot->remains(),
                     new_duration );
  }

  // To compute new number of ticks, we use the new duration, plus the
  // source's ongoing remaining tick time, since we are copying the ongoing
  // tick too
  timespan_t computed_tick_duration = new_duration;
  if ( tick_event && tick_event->remains() > new_duration )
    computed_tick_duration += time_to_tick - tick_event->remains();

  // Aand then adjust some things for ease-of-use for now. The copied dot has
  // its current tick reset to 0, and it's last start time is set to current
  // time.
  //
  // TODO?: A more proper way might be to also copy the source dot's last
  // start time and current tick, in practice it is probably meaningless,
  // though.
  other_dot->last_start       = sim.current_time();
  other_dot->current_duration = new_duration;
  other_dot->current_tick     = 0;
  other_dot->extended_time    = timespan_t::zero();
  other_dot->stack            = stack;
  other_dot->time_to_tick     = time_to_tick;
  other_dot->num_ticks =
      as<int>( std::ceil( computed_tick_duration / time_to_tick ) );

  other_dot->ticking   = true;
  other_dot->end_event = make_event<dot_end_event_t>( sim, other_dot, new_duration );

  other_dot->last_tick_factor = other_dot->current_action->last_tick_factor(
      other_dot, time_to_tick, computed_tick_duration );

  // The clone may happen on tick, or mid tick. If it happens on tick, the
  // source dot will not have a new tick event scheduled yet, so the tick
  // time has to be based on the action's tick time. If cloning happens mid
  // tick, we just use the remaining tick time of the source for the tick
  // time. Tick time will be recalculated on the next tick, implicitly
  // syncing it to the source's tick time.
  timespan_t tick_time;
  if ( tick_event )
    tick_time = tick_event->remains();
  else
    tick_time = other_dot->current_action->tick_time( other_dot->state );

  other_dot->tick_event = make_event<dot_tick_event_t>( sim, other_dot, tick_time );
}

// dot_t::create_expression =================================================
namespace {

struct dot_expr_t : public expr_t
{
  dot_t* static_dot;
  action_t* action, * source_action;
  bool dynamic;
  target_specific_t<dot_t> specific_dot;

  dot_expr_t( util::string_view n, dot_t* d, action_t* a, action_t* sa, bool dy )
    : expr_t( n ),
      static_dot( d ),
      action( a ),
      source_action( sa ),
      dynamic( dy ),
      specific_dot( false )
  {
  }

  // Note, the target of the dot is sourced from the action (APL line) we are creating this action
  // for. This is required so that cycle_targets and target_if work properly in the cases where
  // the option is used on a line where the dot expression refers to a completely different
  // action.
  dot_t* dot()
  {
    if ( !dynamic )
      return static_dot;

    player_t* dot_target = source_action->target;

    action->player->get_target_data( dot_target );

    dot_t*& dot = specific_dot[ dot_target ];
    if ( !dot )
      dot = dot_target->get_dot( action->name_str, action->player );

    return dot;
  }
};

template <typename Fn>
struct fn_dot_expr_t final : public dot_expr_t
{
  Fn fn;

  template <typename T = Fn>
  fn_dot_expr_t( util::string_view n, dot_t* d, action_t* a, action_t* sa, bool dy, T&& fn )
    : dot_expr_t( n, d, a, sa, dy ), fn( std::forward<T>( fn ) )
  { }

  double evaluate() override
  {
    return coerce( fn( dot() ) );
  }
};

} // anon namespace

std::unique_ptr<expr_t> dot_t::create_expression( dot_t* dot, action_t* action, action_t* source_action,
                                  util::string_view name_str, bool dynamic )
{
  if (!dynamic)
  {
    assert(dot && "dot expression are either dynamic or need a static dot");
  }
  if (dynamic)
  {
    assert( action && "dynamic dot expressions require a action.");
  }

  auto make_dot_expr = [dot, action, source_action, dynamic]( util::string_view n, auto&& fn ) {
    using Fn = decltype(fn);
    return std::make_unique<fn_dot_expr_t<std::decay_t<Fn>>>(
            n, dot, action, source_action, dynamic, std::forward<Fn>( fn ) );
  };

  if ( name_str == "ticks" )
  {
    return make_dot_expr( "dot_ticks",
      []( dot_t* dot ) {
        return dot->current_tick;
      } );
  }
  else if ( name_str == "extended_time" )
  {
    return make_dot_expr( "dot_extended_time",
      []( dot_t* dot ) {
        return dot->extended_time;
      } );
  }
  else if ( name_str == "duration" )
  {
    return make_dot_expr( "dot_duration",
      []( dot_t* dot ) {
        if ( !dot->state )
          return 0_ms; // DoT isn't active, return 0. Doing otherwise would
                       // require some effort.
        return dot->current_action->composite_dot_duration( dot->state );
      } );
  }
  else if ( name_str == "refreshable" )
  {
    return make_dot_expr( "dot_refresh",
      [state = std::unique_ptr<action_state_t>()]( dot_t* dot ) mutable {
        // No dot up, thus it'll be refreshable (to full duration)
        if ( dot == nullptr || !dot->is_ticking() )
        {
          return true;
        }

        if ( !state )
        {
          state.reset( dot->current_action->get_state() );
        }

        dot->current_action->snapshot_state( state.get(), result_amount_type::DMG_OVER_TIME );
        timespan_t new_duration =
            dot->current_action->composite_dot_duration( state.get() );

        return dot->current_action->dot_refreshable( dot, new_duration );
      } );
  }
  else if ( name_str == "remains" )
  {
    return make_dot_expr( "dot_remains",
      []( dot_t* dot ) {
        return dot->remains();
      } );
  }
  else if ( name_str == "tick_dmg" )
  {
    return make_dot_expr( "dot_tick_dmg",
      [state = std::unique_ptr<action_state_t>()]( dot_t* dot ) mutable {
        if ( !dot->state )
          return 0.0;

        if ( !state )
        {
          state.reset( dot->current_action->get_state() );
        }
        state->copy_state( dot->state );
        state->result = RESULT_HIT;
        return state->action->calculate_tick_amount( state.get(), dot->current_stack() );
      } );
  }
  else if ( name_str == "crit_dmg" )
  {
    return make_dot_expr( "dot_crit_dmg",
      [state = std::unique_ptr<action_state_t>()]( dot_t* dot ) mutable {
        if ( !dot->state )
          return 0.0;

        if ( !state )
        {
          state.reset( dot->current_action->get_state() );
        }
        state->copy_state( dot->state );
        state->result = RESULT_CRIT;
        return state->action->calculate_tick_amount( state.get(), dot->current_stack() );
      } );
  }
  else if ( name_str == "tick_time_remains" )
  {
    return make_dot_expr( "dot_tick_time_remain",
      []( dot_t* dot ) {
        return dot->is_ticking() ? dot->tick_event->remains() : 0_ms;
      } );
  }
  else if ( name_str == "ticks_remain" )
  {
    return make_dot_expr( "dot_ticks_remain",
      []( dot_t* dot ) {
        return dot->ticks_left();
      } );
  }
  else if ( name_str == "ticks_remain_fractional" )
  {
    return make_dot_expr( "dot_ticks_remain_fractional",
      []( dot_t* dot ) {
        if (!dot->current_action)
          return 0.0;
        if (!dot->is_ticking())
          return 0.0;

        return dot->remains() / dot->current_action->tick_time(dot->state);
      } );
  }
  else if ( name_str == "ticking" )
  {
    return make_dot_expr( "dot_ticking",
      []( dot_t* dot ) {
        return dot->is_ticking();
      } );
  }
  else if ( name_str == "spell_power" )
  {
    return make_dot_expr( "dot_spell_power",
      []( dot_t* dot ) {
        return dot->state ? dot->state->composite_spell_power() : 0;
      } );
  }
  else if ( name_str == "attack_power" )
  {
    return make_dot_expr( "dot_attack_power",
      []( dot_t* dot ) {
        return dot->state ? dot->state->composite_attack_power() : 0;
      } );
  }
  else if ( name_str == "multiplier" )
  {
    return make_dot_expr( "dot_multiplier",
      []( dot_t* dot ) {
        return dot->state ? dot->state->ta_multiplier : 0;
      } );
  }
  else if ( name_str == "pmultiplier" )
  {
    return make_dot_expr( "dot_pmultiplier",
      []( dot_t* dot ) {
        return dot->state ? dot->state->persistent_multiplier : 0;
      } );
  }
  else if ( name_str == "haste_pct" )
  {
    return make_dot_expr( "dot_haste_pct",
      []( dot_t* dot ) {
        return dot->state ? ( 1.0 / dot->state->haste - 1.0 ) * 100 : 0;
      } );
  }
  else if ( name_str == "current_ticks" )
  {
    return make_dot_expr( "dot_current_ticks",
      []( dot_t* dot ) {
        return dot->num_ticks;
      } );
  }
  else if ( name_str == "crit_pct" )
  {
    return make_dot_expr( "dot_crit_pct",
      []( dot_t* dot ) {
        return dot->state ? dot->state->crit_chance * 100.0 : 0;
      } );
  }
  else if ( name_str == "stack" )
  {
    return make_dot_expr( "dot_stack",
      []( dot_t* dot ) {
        return dot->current_stack();
      } );
  }
  else if (name_str == "max_stacks")
  {
    return make_dot_expr( "dot_max_stacks",
      []( dot_t* dot ) {
        return dot->max_stack;
      } );
  }


  return nullptr;
}

/* Remaining dot tick time
 */
timespan_t dot_t::remains() const
{
  if ( !current_action )
    return timespan_t::zero();
  if ( !ticking )
    return timespan_t::zero();
  // return last_start + current_duration - sim.current_time();
  if ( !end_event )
    return timespan_t::zero();
  return end_event->remains();
}

timespan_t dot_t::time_to_next_tick() const
{
  if ( !current_action )
    return timespan_t::zero();
  if ( !ticking )
    return timespan_t::zero();
  return tick_event->remains();
}

/* Returns the ticks left based on the current estimated number of max ticks
 * minus elapsed ticks.
 * Beware: This value may change over time, giving higher or lower values
 * depending on the tick time of the dot!
 */
int dot_t::ticks_left() const
{
  if ( !current_action )
    return 0;
  if ( !ticking )
    return 0;

  // If no tick_event is scheduled but the dot is ticking, ticks_left() was called
  // during tick(). In that case, include the currently processed tick.
  timespan_t next_tick = tick_event ? tick_event->remains() : 0_ms;

  return static_cast<int>(
    1 + std::ceil( ( remains() - next_tick ) / current_action->tick_time( state ) ) );
}

double dot_t::ticks_left_fractional() const
{
  if ( !current_action )
    return 0;
  if ( !ticking )
    return 0;

  timespan_t tick_time = current_action->tick_time( state );
  timespan_t next_tick = tick_event ? tick_event->remains() : 0_ms;
  // Remaining duration after the next tick.
  timespan_t remaining = remains() - next_tick;

  if ( remaining == 0_ms )
    return time_to_tick / tick_time;
  else
    return 1 + remaining / tick_time;
}

/* Called on Dot start if dot action has tick_zero = true set.
 */
void dot_t::tick_zero()
{
  sim.print_debug( "{} {} zero-tick.", *source, *this );

  tick();
}

/**
 * Check if channeling action should be interrupted and do so
 *
 * @return true if action gets interrupted
 */
bool dot_t::channel_interrupt()
{
  assert( ticking );
  if ( current_action->channeled )
  {
    bool interrupt = current_action->option.interrupt;
    if ( !interrupt )
    {
      auto&& expr = current_action->interrupt_if_expr;
      if ( expr )
      {
        interrupt = expr->success();
        if ( sim.debug )
          sim.out_debug.printf( "Dot interrupt expression check=%d",
                                interrupt );
      }
    }
    if ( interrupt )
    {
      bool gcd_ready = current_action->player->gcd_ready <= sim.current_time();
      bool action_available = is_higher_priority_action_available();
      if ( sim.debug )
        sim.out_debug.printf(
            "Dot interrupt check: gcd_ready=%d action_available=%d.", gcd_ready,
            action_available );
      if ( ( gcd_ready || current_action->option.interrupt_immediate ) &&
           action_available )
      {
        if ( current_action->option.interrupt_immediate )
        {
          current_action->interrupt_immediate_occurred = true;
        }
        // cancel dot
        last_tick();
        return true;
      }
    }
  }

  return false;
}

/* Called each time the dot ticks.
 */
void dot_t::tick()
{
  if ( current_action->channeled )
  {
    // If the ability has an interrupt or chain-based option enabled, we need to dynamically regen
    // resources for the actor before the chain/interrupt processing is done.
    if ( current_action->player->resource_regeneration == regen_type::DYNAMIC&&
         ( current_action->option.chain || current_action->option.interrupt ||
           current_action->interrupt_if_expr || current_action->early_chain_if_expr ) )
    {
      current_action->player->do_dynamic_regen();
    }

    if ( current_action->interrupt_auto_attack )
    {  // Channeled dots that interrupt auto attacks will also be interrupted by
      // the target running out of range.
      // This should capture most dot driver spells that require a target in
      // range, such as mind sear, while avoiding abilities like bladestorm that
      // do not.
      if ( current_action->sim->distance_targeting_enabled &&
           !current_action->execute_targeting( current_action ) )
      {
        current_action->reset();
        return;
      }
    }
  }

  sim.print_debug( "{} ticks ({} of {}). last_start={}, duration={} time_to_tick={}", *this, current_tick, num_ticks,
                   last_start, current_duration, time_to_tick );

  current_action->tick( this );
  prev_tick_time = sim.current_time();
}

/* Called when the dot expires, after the last tick() call.
 */
void dot_t::last_tick()
{
  sim.print_debug( "{} fades from {}", *this, *state->target );

  // call action_t::last_tick
  current_action->last_tick( this );

  reset();

  // If channeled, bring player back to life
  if ( current_action->channeled && !current_action->background &&
       current_action->player->arise_time > timespan_t::min() )
  {
    assert( !current_action->player->readying &&
            "Danger Will Robinson! Channeled foreground dot action is trying "
            "to schedule ready event for player already having one." );

    current_action->player->schedule_ready( timespan_t::zero() );
  }
}

void dot_t::schedule_tick()
{
  if ( remains() == timespan_t::zero() )
  {
    return;
  }

  sim.print_debug( "{} schedules tick for {} on {}", *source, *this, *target );

  timespan_t base_tick_time = current_action->tick_time( state );
  time_to_tick              = std::min( base_tick_time, remains() );
  assert( time_to_tick > timespan_t::zero() &&
          "A Dot needs a positive tick time!" );

  // Recalculate num_ticks:
  num_ticks = current_tick + as<int>( std::ceil( remains() / base_tick_time ) );

  last_tick_factor =
      current_action->last_tick_factor( this, base_tick_time, remains() );

  tick_event = make_event<dot_tick_event_t>( sim, this, time_to_tick );

  if ( current_action->channeled )
  {
    if ( current_action->cancel_if_expr && current_action->cancel_if_expr->success() )
    {
      if ( current_action->sim->debug )
      {
        current_action->sim->out_debug.print( "{} '{}' cancel_if returns {}, cancelling channel",
          current_action->player->name(), current_action->signature_str,
          current_action->cancel_if_expr->eval() );
      }
      cancel();
      return;
    }

    // FIXME: Find some way to make this more realistic - the actor shouldn't
    // have to recast quite this early
    // Response: "Have to"?  It might be good to recast early - since the GCD
    // will end sooner. Depends on the situation. -ersimont
    auto&& expr = current_action->early_chain_if_expr;
    if ( ( ( current_action->option.chain && current_tick + 1 == num_ticks ) ||
           ( current_tick > 0 && expr && expr->success() &&
             current_action->player->gcd_ready <= sim.current_time() ) ) &&
         current_action->action_ready() && !is_higher_priority_action_available() )
    {
      // FIXME: We can probably use "source" instead of "action->player"

      current_action->player->channeling = nullptr;
      current_action->player->gcd_ready =
          sim.current_time() + current_action->gcd();
      current_action->set_target( target );
      current_action->execute();
      if ( current_action->result_is_hit(
               current_action->execute_state->result ) )
      {
        current_action->player->channeling = current_action;
        current_action->player->schedule_cwc_ready( timespan_t::zero() );
      }
      else
        cancel();
    }
    else
    {
      current_action->player->channeling = current_action;
    }
  }
}

void dot_t::start( timespan_t duration )
{
  current_duration = duration;
  last_start       = sim.current_time();

  ticking = true;
  stack   = 1;

  end_event = make_event<dot_end_event_t>( sim, this, current_duration );

  sim.print_debug( "{} starts {} on {} with duration={}", *source, *this, *target, duration );

  state->original_x = target->x_position;
  state->original_y = target->y_position;

  check_tick_zero( true );

  schedule_tick();

  source->add_active_dot( current_action->internal_id );

  // Only schedule a tick if thre's enough time to tick at least once.
  // Otherwise, next tick is the last tick, and the end event will handle it
  if ( current_duration <= time_to_tick )
    event_t::cancel( tick_event );
}

/* Precondition: ticking == true
 */
void dot_t::refresh( timespan_t duration )
{
  current_duration =
      current_action->calculate_dot_refresh_duration( this, duration );

  last_start = sim.current_time();

  if ( stack < max_stack )
    stack++;

  assert( end_event && "Dot is ticking but has no end event." );
  timespan_t remaining_duration = end_event->remains();

  // DoT Refresh events have to be canceled instead of refreshed. Otherwise, it
  // is possible to get the dot into a state, where the last reschedule event
  // occurs between the second to last, and the last tick. This will cause the
  // event ordering in the sim to flip (last tick happens before end event),
  // causing the dot to tick twice at the end of the duration.
  event_t::cancel( end_event );
  end_event = make_event<dot_end_event_t>( sim, this, current_duration );

  check_tick_zero( false );

  recalculate_num_ticks();

  if ( sim.debug )
    sim.print_debug( "{} refreshes {} ({}) on {}{}. old_remains={} refresh_duration={} duration={}", *source, *this,
                     stack, *target, current_action->dot_refreshable( this, duration ) ? " (no penalty)" : "",
                     remaining_duration, duration, current_duration );

  // Ensure that the ticker is running when dots are refreshed. It is possible
  // that a specialized dot handling cancels the ticker when there's no time to
  // tick another one (before the periodic effect ends). This is for example
  // used in the Rogue module to implement Sinister Calling.
  if ( !tick_event )
  {
    assert( !current_action->channeled );
    tick_event = make_event<dot_tick_event_t>( sim, this, remaining_duration );
  }

  // When refreshing DoTs before the partial last tick on expiry,
  // reschedule the next tick to occur at its original interval again.
  timespan_t time_to_tick = current_action -> tick_time( state );
  timespan_t next_tick_at = prev_tick_time + time_to_tick;
  timespan_t next_tick_in = next_tick_at - sim.current_time();
  if ( !current_action -> channeled && remaining_duration < next_tick_in )
  {
    event_t::cancel( tick_event );
    tick_event = make_event<dot_tick_event_t>( sim, this, next_tick_in );

    sim.print_debug( "{} reschedules next tick (was a partial) for {} ({}) on {} to happen in {} at {}.", *source,
                     *this, stack, *target, next_tick_in, next_tick_at );
  }
}

void dot_t::recalculate_num_ticks()
{
  num_ticks =
      current_tick +
      as<int>( std::ceil( remains() / current_action->tick_time( state ) ) );
}

void dot_t::check_tick_zero( bool start )
{
  // If we're precasting a helpful dot and we're not in combat, fake precasting
  // by using a first tick.
  // We also reduce the duration by one tick interval in
  // action_t::trigger_dot().
  bool fake_first_tick =
      !current_action->harmful && !current_action->player->in_combat;

  if ( ( current_action->tick_zero || ( current_action->tick_on_application && start ) ) ||
       fake_first_tick )
  {
    timespan_t previous_ttt = time_to_tick;
    time_to_tick            = timespan_t::zero();
    // Recalculate num_ticks:
#ifndef NDEBUG
    timespan_t tick_time = current_action->tick_time( state );
#endif
    assert( tick_time > timespan_t::zero() &&
            "A Dot needs a positive tick time!" );
    recalculate_num_ticks();
    tick_zero();
    if ( remains() <= timespan_t::zero() )
    {
      last_tick();
      return;
    }
    time_to_tick = previous_ttt;
  }
}

bool dot_t::is_higher_priority_action_available() const
{
  assert( current_action->action_list );

  auto player = current_action->player;
  auto apl = current_action->interrupt_global ? player->active_action_list : current_action->action_list;

  player->visited_apls_ = 0;
  auto action = player->select_action( *apl, execute_type::FOREGROUND, current_action );
  if ( action && action->internal_id != current_action->internal_id && player->sim->debug )
  {
    player->sim->out_debug.print( "{} action available for context {}: {}", player->name(),
      current_action->signature_str, action->signature_str );
  }
  return action != nullptr && action->internal_id != current_action->internal_id;
}

void dot_t::adjust( double coefficient )
{
  if ( !is_ticking() )
  {
    return;
  }

  sim_t* sim = current_action->sim;

  timespan_t new_tick_remains = tick_event->remains() * coefficient;
  timespan_t new_dot_remains  = end_event->remains() * coefficient;
  timespan_t new_duration     = current_duration * coefficient;

  if ( sim->debug )
  {
    sim->out_debug.printf(
        "%s adjusts dot %s (on %s): duration=%.3f -> %.3f, next_tick=%.3f -> "
        "%.3f, ends=%.3f -> %.3f",
        current_action->player->name(), current_action->name(), target->name(),
        current_duration.total_seconds(), new_duration.total_seconds(),
        tick_event->occurs().total_seconds(),
        ( sim->current_time() + new_tick_remains ).total_seconds(),
        end_event->occurs().total_seconds(),
        ( sim->current_time() + new_dot_remains ).total_seconds() );
  }

  event_t::cancel( tick_event );
  event_t::cancel( end_event );

  current_duration = new_duration;
  time_to_tick     = time_to_tick * coefficient;
  tick_event       = make_event<dot_tick_event_t>( *sim, this, new_tick_remains );
  //end_event        = new ( *sim ) dot_end_event_t( this, new_dot_remains );
  end_event = make_event<dot_end_event_t>(*sim, this, new_dot_remains );
}

void dot_t::adjust_full_ticks( double coefficient )
{
  if ( !is_ticking() )
  {
    return;
  }

  sim_t* sim = current_action->sim;

  // Always at least 1 tick left (even if we would round down before the last partial)
  int rounded_full_ticks_left = std::max( static_cast<int>( std::round( current_duration / current_action -> tick_time( state ) ) ) - current_tick, 1 );

  timespan_t new_dot_remains  = end_event->remains() * coefficient;
  timespan_t new_duration     = current_duration * coefficient;
  timespan_t new_time_to_tick = time_to_tick * coefficient;
  timespan_t new_tick_remains = new_dot_remains - ( rounded_full_ticks_left - 1 ) * new_time_to_tick;
  if (new_tick_remains <= timespan_t::zero() )
  {
    current_tick++;
    tick();
    new_tick_remains += new_time_to_tick;
    rounded_full_ticks_left--;
  }

  // Also set last_tick_factor to 1 in case the partial was already scheduled.
  if (rounded_full_ticks_left == 1)
    last_tick_factor = 1.0;

  if ( sim->debug )
  {
    sim->out_debug.printf(
        "%s exsanguinates dot %s (on %s): duration=%.3f -> %.3f, next_tick=%.3f -> "
        "%.3f, ends=%.3f -> %.3f",
        current_action->player->name(), current_action->name(), target->name(),
        current_duration.total_seconds(), new_duration.total_seconds(),
        tick_event->occurs().total_seconds(),
        ( sim->current_time() + new_tick_remains ).total_seconds(),
        end_event->occurs().total_seconds(),
        ( sim->current_time() + new_dot_remains ).total_seconds() );
  }

  event_t::cancel( tick_event );
  event_t::cancel( end_event );

  current_duration = new_duration;
  time_to_tick     = new_time_to_tick;
  tick_event       = make_event<dot_tick_event_t>( *sim, this, new_tick_remains );
  end_event        = make_event<dot_end_event_t>( *sim, this, new_dot_remains );
  num_ticks        = current_tick + rounded_full_ticks_left;
}

dot_t::dot_tick_event_t::dot_tick_event_t(dot_t* d, timespan_t time_to_tick) :
  event_t(*d -> source, time_to_tick),
  dot(d)
{
  sim().print_debug( "New DoT Tick Event: {} {} tick {}-of{} time_to_tick={}", *d->source, *dot, dot->current_tick + 1,
                     dot->num_ticks, time_to_tick );
}


void dot_t::dot_tick_event_t::execute()
{
  dot->tick_event = nullptr;
  dot->current_tick++;

  if (dot->current_action->channeled &&
    dot->current_action->action_skill < 1.0 &&
    dot->remains() >= dot->current_action->tick_time(dot->state))
  {
    if (rng().roll(std::max(0.0, dot->current_action->action_skill - dot->current_action->player->current.skill_debuff)))
    {
      dot->tick();
    }
  }
  else // No skill-check required
  {
    dot->tick();
  }

  // Some dots actually cancel themselves mid-tick. If this happens, we presume
  // that the cancel has been "proper", and just stop event execution here, as
  // the dot no longer exists.
  if (!dot->is_ticking())
    return;

  if (!dot->current_action->consume_cost_per_tick(*dot))
  {
    return;
  }

  if (dot->channel_interrupt())
  {
    return;
  }

  // continue ticking
  dot->schedule_tick();
}

dot_t::dot_end_event_t::dot_end_event_t(dot_t* d, timespan_t time_to_end) :
  event_t(*d -> source, time_to_end),
  dot(d)
{
  sim().print_debug( "New DoT End Event: {} {} time_to_end={}", *d->source, *dot, time_to_end );
}

void dot_t::dot_end_event_t::execute()
{
  dot->end_event = nullptr;
  if (dot->current_tick < dot->num_ticks)
  {
    dot->current_tick++;
    dot->tick();
  }
  // If for some reason the last tick has already ticked, ensure that the next tick has not
  // consumed any time yet, i.e., the last tick has occurred on the same timestamp as this end
  // event. This situation may occur in conjunction with extensive dot extension, where the last
  // rescheduling of the dot-end-event occurs between the second to last and last ticks. That will
  // in turn flip the order of the dot-tick-event and dot-end-event.
  else
  {
    assert(!dot->tick_event || (dot->tick_event && dot->time_to_tick == dot->tick_event->remains()));
  }

  // Aand sanity check that the dot has consumed all ticks just in case./
  assert(dot->current_tick == dot->num_ticks);
  dot->last_tick();
}

void format_to( const dot_t& dot, fmt::format_context::iterator out )
{
  fmt::format_to( out, "Dot {}", dot.name_str );
}

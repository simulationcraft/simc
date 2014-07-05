// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Dot
// ==========================================================================

// DoT Tick Event ===========================================================

struct dot_t::dot_tick_event_t final : public event_t
{
public:
  dot_tick_event_t( dot_t* d, timespan_t time_to_tick ) :
      event_t( *d -> source, "DoT Tick" ),
      dot( d )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New DoT Tick Event: %s %s %d-of-%d %.4f",
                  p() -> name(), dot -> name(), dot -> current_tick + 1, dot -> num_ticks, time_to_tick.total_seconds() );

    sim().add_event( this, time_to_tick );
  }

private:
  virtual void execute() override
  {
    dot -> tick_event = nullptr;
    dot -> current_tick++;

    if ( dot -> current_action -> channeled &&
         dot -> current_action -> player -> current.skill < 1.0 &&
         dot -> remains() >= dot -> current_action -> tick_time( dot -> state -> haste ) )
    {
      if ( rng().roll( dot -> current_action -> player -> current.skill ) )
      {
        dot -> tick();
      }
    }
    else // No skill-check required
    {
      dot -> tick();
    }

    assert ( dot -> ticking );
    expr_t* expr = dot -> current_action -> interrupt_if_expr;
    if ( ( dot -> current_action -> channeled
            && dot -> current_action -> player -> gcd_ready <= sim().current_time
            && ( dot -> current_action -> interrupt || ( expr && expr -> success() ) )
            && dot -> is_higher_priority_action_available() ) )
    {
      // cancel dot
      dot -> last_tick();
    }
    else
    {
      // continue ticking
      dot -> schedule_tick();
    }
  }
  dot_t* dot;
};

// DoT End Event ===========================================================

struct dot_t::dot_end_event_t final : public event_t
{
public:
  dot_end_event_t( dot_t* d, timespan_t time_to_end ) :
      event_t( *d -> source, "DoT End" ),
      dot( d )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New DoT End Event: %s %s %d-of-%d %.4f",
                  p() -> name(), dot -> name(), dot -> current_tick + 1, dot -> num_ticks, time_to_end.total_seconds() );

    sim().add_event( this, time_to_end );
  }

private:
  virtual void execute() override
  {
    dot -> end_event = nullptr;
    dot -> current_tick++;
    dot -> tick();
    dot -> last_tick();
  }
  dot_t* dot;
};

dot_t::dot_t( const std::string& n, player_t* t, player_t* s ) :
  sim( *( t -> sim ) ),
  ticking( false ),
  current_duration( timespan_t::zero() ),
  last_start( timespan_t::min() ),
  extended_time( timespan_t::zero() ),
  tick_event( nullptr ),
  end_event( nullptr ),
  last_tick_factor( -1.0 ),
  target( t ),
  source( s ),
  current_action( nullptr ),
  state( nullptr ),
  num_ticks( 0 ),
  current_tick( 0 ),
  miss_time( timespan_t::min() ),
  time_to_tick( timespan_t::zero() ),
  name_str( n )
{}

// dot_t::cancel ============================================================

void dot_t::cancel()
{
  if ( ! ticking )
    return;

  last_tick();
  reset();
}

// dot_t::extend_duration_seconds ===========================================

void dot_t::extend_duration( timespan_t extra_seconds, timespan_t max_total_time, uint32_t state_flags )
{
  if ( ! ticking )
    return;

  if ( state_flags == ( uint32_t ) - 1 ) state_flags = current_action -> snapshot_flags;

  // Make sure this DoT is still ticking......
  assert( tick_event );
  assert( state );
  current_action -> snapshot_internal( state, state_flags, current_action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

  if ( max_total_time > timespan_t::zero() )
  {
    timespan_t over_cap = remains() + extra_seconds - max_total_time;
    if ( over_cap > timespan_t::zero() )
      extra_seconds -= over_cap;
  }
  current_duration += extra_seconds;
  extended_time += extra_seconds;


  if ( sim.log )
  {
    sim.out_log.printf( "%s extends duration of %s on %s by %.1f second(s).",
                source -> name(), name(), target -> name(), extra_seconds.total_seconds() );
  }

  assert( end_event && "Dot is ticking but has no end event." );
  timespan_t remains = end_event -> remains();
  remains += extra_seconds;
  if ( remains != end_event -> remains() )
    end_event -> reschedule( remains );

  current_action -> stats -> add_refresh( state -> target );
}

// dot_t::refresh_duration ==================================================

void dot_t::refresh_duration( uint32_t state_flags )
{
  if ( ! ticking )
    return;

  if ( state_flags == ( uint32_t ) - 1 ) state_flags = current_action -> snapshot_flags;

  current_action -> snapshot_internal( state, state_flags, current_action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

  refresh( current_action -> dot_duration );

  current_action -> stats -> add_refresh( state -> target );
}

/* Reset Dot
 */
void dot_t::reset()
{
  core_event_t::cancel( tick_event );
  core_event_t::cancel( end_event );
  time_to_tick = timespan_t::zero();
  ticking = false;
  current_tick = 0;
  extended_time = timespan_t::zero();
  ticking = false;
  miss_time = timespan_t::min();
  last_start = timespan_t::min();
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
  assert( duration > timespan_t::zero() && "Dot Trigger with duration <= 0 seconds." );

  current_tick = 0;
  extended_time = timespan_t::zero();
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

void dot_t::copy( player_t* other_target, dot_copy_e copy_type )
{
  // assert( target != other_target );

  dot_t* other_dot = current_action -> get_dot( other_target );
  // If the target actor has a dot ticking, we need to cancel it in any case
  if ( other_dot -> is_ticking() )
    other_dot -> cancel();

  other_dot -> copy( this );

  if ( copy_type == DOT_COPY_START )
    other_dot -> trigger( other_dot -> current_duration );
  // If we don't start the copied dot from the beginning, we need to bypass a
  // lot of the dot scheduling logic, and simply do the minimum amount possible
  // to get the dot aligned with the source dot, both in terms of state, as
  // well as the remaining duration, and the remaining ongoing tick time.
  else
  {
    other_dot -> ticking = true;
    other_dot -> end_event = new ( sim ) dot_t::dot_end_event_t( other_dot, remains() );

    // The clone may happen on tick, or mid tick. If it happens on tick, the
    // source dot will not have a new tick event scheduled yet, so the tick
    // time has to be based on the action's tick time. If cloning happens mid
    // tick, we just use the remaining tick time of the source for the tick
    // time. Tick time will be recalculated on the next tick, implicitly
    // syncing it to the source's tick time.
    timespan_t tick_time;
    if ( other_dot -> tick_event )
      tick_time = tick_event -> remains();
    else
      tick_time = other_dot -> current_action -> tick_time( other_dot -> state -> haste );

    other_dot -> tick_event = new ( sim ) dot_t::dot_tick_event_t( other_dot, tick_time );
  }
}

/* Caller needs to handle logic if the source dot was ticking or not!!!
 */
void dot_t::copy( dot_t* other_dot )
{
  if ( ! state )
    state = other_dot -> current_action -> get_state();

  assert( other_dot -> current_duration > timespan_t::zero() );
  assert( other_dot -> ticking );

  state -> copy_state( other_dot -> state );
  state -> target = target;
  current_action = other_dot -> current_action;
  current_tick = other_dot -> current_tick;
  last_start = other_dot -> last_start;
  current_duration = other_dot -> current_duration;
  num_ticks = other_dot -> num_ticks;
  extended_time = other_dot -> extended_time;
  last_tick_factor = other_dot -> last_tick_factor;
}

// dot_t::create_expression =================================================

expr_t* dot_t::create_expression( action_t* action,
                                  const std::string& name_str,
                                  bool dynamic )
{
  struct dot_expr_t : public expr_t
  {
    dot_t* static_dot;
    action_t* action;
    bool dynamic;
    target_specific_t<dot_t*> specific_dot;

    dot_expr_t( const std::string& n, dot_t* d, action_t* a, bool dy ) :
      expr_t( n ), static_dot( d ), action( a ), dynamic( dy ), specific_dot( false ) {}

    dot_t* dot()
    {
      if ( ! dynamic ) return static_dot;
      action -> player -> get_target_data( action -> target );
      dot_t*& dot = specific_dot[ action -> target ];
      if ( ! dot ) dot = action -> target -> get_dot( static_dot -> name(), action -> player );
      return dot;
    }
  };

  if ( name_str == "ticks" )
  {
    struct ticks_expr_t : public dot_expr_t
    {
      ticks_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_ticks", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> current_tick; }
    };
    return new ticks_expr_t( this, action, dynamic );
  }
  else if ( name_str == "extended_time" )
  {
    struct extended_time_expr_t : public dot_expr_t
    {
      extended_time_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "extended_time", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> extended_time.total_seconds(); }
    };
    return new extended_time_expr_t( this, action, dynamic );
  }
  else if ( name_str == "duration" )
  {
    struct duration_expr_t : public dot_expr_t
    {
      // DoT duration expression, returns total duration of current dot.
      duration_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_duration", d, a, dynamic ) {}
      virtual double evaluate()
      {
        dot_t* dot = this -> dot();
        if ( ! dot -> state ) return 0; // DoT isn't active, return 0. Doing otherwise would require some effort.
        return dot -> current_action -> composite_dot_duration( dot -> state ).total_seconds();
      }
    };
    return new duration_expr_t( this, action, dynamic );
  }
  else if ( name_str == "remains" )
  {
    struct remains_expr_t : public dot_expr_t
    {
      remains_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_remains", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> remains().total_seconds(); }
    };
    return new remains_expr_t( this, action, dynamic );
  }
  else if ( name_str == "tick_dmg" )
  {
    struct tick_dmg_expr_t : public dot_expr_t
    {
      tick_dmg_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_tick_dmg", d, a, dynamic ) {}
      virtual double evaluate()
      {
        if ( dot() -> state )
        {
          double damage = dot() -> state -> result_amount;
          if ( dot() -> state -> result == RESULT_CRIT )
            damage /= 1.0 + action -> total_crit_bonus();
          return damage;
        }
        return 0.0;
      }
    };
    return new tick_dmg_expr_t( this, action, dynamic );
  }
  else if ( name_str == "crit_dmg" )
  {
    struct crit_dmg_expr_t : public dot_expr_t
    {
      crit_dmg_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_crit_dmg", d, a, dynamic ) {}
      virtual double evaluate()
      {
        if ( dot() -> state )
        {
          double damage = dot() -> state -> result_amount;
          if ( dot() -> state -> result == RESULT_HIT )
            damage *= 1.0 + action -> total_crit_bonus();
          return damage;
        }
        return 0.0;
      }
    };
    return new crit_dmg_expr_t( this, action, dynamic );
  }
  else if ( name_str == "ticks_remain" )
  {
    struct ticks_remain_expr_t : public dot_expr_t
    {
      ticks_remain_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_ticks_remain", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> ticks_left(); }
    };
    return new ticks_remain_expr_t( this, action, dynamic );
  }
  else if ( name_str == "ticking" )
  {
    struct ticking_expr_t : public dot_expr_t
    {
      ticking_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_ticking", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> ticking; }
    };
    return new ticking_expr_t( this, action, dynamic );
  }
  else if ( name_str == "spell_power" )
  {
    struct dot_spell_power_expr_t : public dot_expr_t
    {
      dot_spell_power_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_spell_power", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> composite_spell_power() : 0; }
    };
    return new dot_spell_power_expr_t( this, action, dynamic );
  }
  else if ( name_str == "attack_power" )
  {
    struct dot_attack_power_expr_t : public dot_expr_t
    {
      dot_attack_power_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_attack_power", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> composite_attack_power() : 0; }
    };
    return new dot_attack_power_expr_t( this, action, dynamic );
  }
  else if ( name_str == "multiplier" )
  {
    struct dot_multiplier_expr_t : public dot_expr_t
    {
      dot_multiplier_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_multiplier", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> ta_multiplier : 0; }
    };
    return new dot_multiplier_expr_t( this, action, dynamic );
  }
  else if ( name_str == "pmultiplier" )
  {
    struct dot_pmultiplier_expr_t : public dot_expr_t
    {
      dot_pmultiplier_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_pmultiplier", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> persistent_multiplier : 0; }
    };
    return new dot_pmultiplier_expr_t( this, action, dynamic );
  }

#if 0
  else if ( name_str == "mastery" )
  {
    struct dot_mastery_expr_t : public dot_expr_t
    {
      dot_mastery_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_mastery", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> total_mastery() : 0; }
    };
    return new dot_mastery_expr_t( this, current_action, dynamic );
  }
#endif

  else if ( name_str == "haste_pct" )
  {
    struct dot_haste_pct_expr_t : public dot_expr_t
    {
      dot_haste_pct_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_haste_pct", d, a, dynamic ) {}
      virtual double evaluate()
      {
        return dot() -> state ?
          ( 1.0 / dot() -> state -> haste - 1.0 ) * 100 : 0;
      }
    };
    return new dot_haste_pct_expr_t( this, action, dynamic );
  }
  else if ( name_str == "current_ticks" )
  {
    struct current_ticks_expr_t : public dot_expr_t
    {
      current_ticks_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_current_ticks", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> num_ticks; }
    };
    return new current_ticks_expr_t( this, action, dynamic );
  }
  else if ( name_str == "crit_pct" )
  {
    struct dot_crit_pct_expr_t : public dot_expr_t
    {
      dot_crit_pct_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_crit_pct", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> crit * 100.0 : 0; }
    };
    return new dot_crit_pct_expr_t( this, action, dynamic );
  }

  return 0;
}

/* Remaining dot tick time
 */
timespan_t dot_t::remains() const
{
  if ( ! current_action ) return timespan_t::zero();
  if ( ! ticking ) return timespan_t::zero();
  return last_start + current_duration - sim.current_time;
}

timespan_t dot_t::time_to_next_tick() const
{
  if ( ! current_action ) return timespan_t::zero();
  if ( ! ticking ) return timespan_t::zero();
  return tick_event -> remains();
}

/* Returns the ticks left based on the current estimated number of max ticks minus elapsed ticks.
 * Beware: This value may change over time, giving higher or lower values depending on the tick time of the dot!
 */
int dot_t::ticks_left() const
{
  if ( ! current_action ) return 0;
  if ( ! ticking ) return 0;
  return ( num_ticks - current_tick );
}

/* Called on Dot start if dot action has tick_zero = true set.
 */
void dot_t::tick_zero()
{
  if ( sim.debug )
    sim.out_debug.printf( "%s zero-tick.", name() );

  tick();
}

/* Called each time the dot ticks.
 */
void dot_t::tick()
{
  if ( sim.debug )
    sim.out_debug.printf( "%s ticks (%d of %d). last_start=%.4f, dur=%.4f tt=%.4f",
                          name(), current_tick, num_ticks, last_start.total_seconds(),
                          current_duration.total_seconds(), time_to_tick.total_seconds() );

  current_action -> tick( this );
}

/* Called when the dot expires, after the last tick() call.
 */
void dot_t::last_tick()
{
  if ( sim.debug )
    sim.out_debug.printf( "%s fades from %s", name(), state -> target -> name() );

  // call action_t::last_tick
  current_action -> last_tick( this );

  reset();

  // If channeled, bring player back to life
  if ( current_action -> channeled )
  {
    if ( current_action -> player -> readying )
      sim.out_error << "Danger Will Robinson!  Danger! " << name();

    current_action -> player -> schedule_ready( timespan_t::zero() );
  }
}

void dot_t::schedule_tick()
{
  if ( sim.debug )
    sim.out_debug.printf( "%s schedules tick for %s on %s", source -> name(), name(), target -> name() );

  time_to_tick = current_action -> tick_time( state -> haste );
  assert( time_to_tick > timespan_t::zero() && "A Dot needs a positive tick time!" );

  // Recalculate num_ticks:
  num_ticks = current_tick + as<int>(std::ceil( remains() / time_to_tick ));
  last_tick_factor = std::min( 1.0, remains() / time_to_tick );

  tick_event = new ( sim ) dot_t::dot_tick_event_t( this, time_to_tick );

  if ( current_action -> channeled )
  {
    // FIXME: Find some way to make this more realistic - the actor shouldn't have to recast quite this early
    // Response: "Have to"?  It might be good to recast early - since the GCD will end sooner. Depends on the situation. -ersimont
    expr_t* expr = current_action -> early_chain_if_expr;
    if ( ( ( current_action -> chain && current_tick + 1 == num_ticks )
           || ( current_tick > 0
                && expr
                && expr -> success()
                && current_action -> player -> gcd_ready <= sim.current_time ) )
         && current_action -> ready()
         && !is_higher_priority_action_available() )
    {
      // FIXME: We can probably use "source" instead of "action->player"

      current_action -> player -> channeling = 0;
      current_action -> player -> gcd_ready = sim.current_time + current_action -> gcd();
      current_action -> execute();
      if ( current_action -> result_is_hit( current_action -> execute_state -> result ) )
      {
        current_action -> player -> channeling = current_action;
      }
      else
        cancel();
    }
    else
    {
      current_action -> player -> channeling = current_action;
    }
  }
}

void dot_t::start( timespan_t duration )
{
  current_duration = duration;
  last_start = sim.current_time;

  ticking = true;

  if ( sim.debug )
    sim.out_debug.printf( "%s starts dot for %s on %s. duration=%f remains()=%f", source -> name(), name(), target -> name(), duration.total_seconds(), remains().total_seconds() );

  check_tick_zero();

  schedule_tick();

  end_event = new ( sim ) dot_t::dot_end_event_t( this, current_duration );
}

/* Precondition: ticking == true
 */
void dot_t::refresh( timespan_t duration )
{
  current_duration = current_action -> calculate_dot_refresh_duration( this, duration );

  last_start = sim.current_time;

  if ( sim.debug )
    sim.out_debug.printf( "%s refreshes dot for %s on %s. duration=%f remains()=%f",
                          source -> name(), name(), target -> name(),
                          current_duration.total_seconds(), remains().total_seconds() );

  check_tick_zero();

  // Recalculate num_ticks:
  num_ticks = current_tick + as<int>(std::ceil(remains() / current_action->tick_time(state->haste) ) );

  assert( end_event && "Dot is ticking but has no end event." );
  end_event -> reschedule( current_duration );
}

void dot_t::check_tick_zero()
{
  // If we're precasting a helpful dot and we're not in combat, fake precasting by using a first tick.
  // We also reduce the duration by one tick interval in action_t::trigger_dot().
  bool fake_first_tick = ! current_action -> harmful && ! current_action -> player -> in_combat;

  if ( current_action -> tick_zero || fake_first_tick )
  {
    timespan_t previous_ttt = time_to_tick;
    time_to_tick = timespan_t::zero();
    // Recalculate num_ticks:
    timespan_t tick_time = current_action->tick_time(state->haste);
    assert( tick_time > timespan_t::zero() && "A Dot needs a positive tick time!" );
	  num_ticks = current_tick + as<int>(std::ceil(remains() / tick_time ) );
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
  action_priority_list_t* active_actions = current_action -> player -> active_action_list;
  size_t num_actions = active_actions -> foreground_action_list.size();
  for ( size_t i = 0; i < num_actions && active_actions -> foreground_action_list[ i ] != current_action; ++i )
  {
    action_t* a = active_actions -> foreground_action_list[ i ];
    // FIXME Why not interrupt a channel for the same spell higher up the action list?
    //if ( a -> id == current_action -> id ) continue;
    if ( a -> ready() )
    {
      return true;
    }
  }
  return false;
}


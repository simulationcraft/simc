// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Dot
// ==========================================================================

namespace { // anonymous namespace


// DoT Tick Event ===========================================================

struct dot_tick_event_t : public event_t
{
  dot_t* dot;

  dot_tick_event_t( sim_t* sim,
                    dot_t* d,
                    timespan_t time_to_tick ) :
    event_t( sim, d -> source, "DoT Tick" ), dot( d )
  {
    if ( sim -> debug )
      sim -> output( "New DoT Tick Event: %s %s %d-of-%d %.2f",
                     player -> name(), dot -> name(), dot -> current_tick + 1, dot -> num_ticks, time_to_tick.total_seconds() );

    sim -> add_event( this, time_to_tick );
  }

  // dot_tick_event_t::execute ================================================

  virtual void execute()
  {
    if ( dot -> current_tick >= dot -> num_ticks )
    {
      sim -> errorf( "Player %s has corrupt tick (%d of %d) event on action %s!\n",
                     player -> name(), dot -> current_tick, dot -> num_ticks, dot -> name() );
      sim -> cancel();
    }

    dot -> tick_event = 0;
    dot -> current_tick++;

    if ( dot -> action -> player -> current.skill < 1.0 &&
         dot -> action -> channeled &&
         dot -> current_tick == dot -> num_ticks )
    {
      if ( sim -> roll( dot -> action -> player -> current.skill ) )
      {
        dot -> action -> tick( dot );
      }
    }
    else // No skill-check required
    {
      dot -> action -> tick( dot );
    }

    if ( dot -> action -> channeled && ( dot -> ticks() > 0 ) )
    {
      expr_t* expr = dot -> action -> interrupt_if_expr;
      if ( expr && expr -> success() )
      {
        dot -> current_tick = dot -> num_ticks;
      }
      if ( ( dot -> action -> interrupt || ( dot -> action -> chain && dot -> current_tick + 1 == dot -> num_ticks ) ) && ( dot -> action -> player -> gcd_ready <=  sim -> current_time ) )
      {
        // Interrupt if any higher priority action is ready.
        for ( size_t i = 0; dot -> action -> player -> active_action_list -> foreground_action_list[ i ] != dot -> action; ++i )
        {
          action_t* a = dot -> action -> player -> active_action_list -> foreground_action_list[ i ];
          if ( a -> id == dot -> action -> id ) continue;
          if ( a -> ready() )
          {
            dot -> current_tick = dot -> num_ticks;
            break;
          }
        }
      }
    }

    if ( dot -> ticking )
    {
      if ( dot -> current_tick == dot -> num_ticks )
      {
        dot -> time_to_tick = timespan_t::zero();
        dot -> action -> last_tick( dot );

        if ( dot -> action -> channeled )
        {
          if ( dot -> action -> player -> readying ) fprintf( sim -> output_file, "Danger Will Robinson!  Danger!  %s\n", dot -> name() );

          dot -> action -> player -> schedule_ready( timespan_t::zero() );
        }
      }
      else dot -> schedule_tick();
    }
  }
};

} // end anonymous namespace
dot_t::dot_t( const std::string& n, player_t* t, player_t* s ) :
  sim( t -> sim ), target( t ), source( s ), action( 0 ), tick_event( 0 ),
  num_ticks( 0 ), current_tick( 0 ), added_ticks( 0 ), ticking( 0 ),
  added_seconds( timespan_t::zero() ), ready( timespan_t::min() ),
  miss_time( timespan_t::min() ),time_to_tick( timespan_t::zero() ),
  name_str( n ), prev_tick_amount( 0.0 ), state( 0 )
{}
// dot_t::cancel ===================================================

void dot_t::cancel()
{
  if ( ! ticking )
    return;

  action -> last_tick( this );
  event_t::cancel( tick_event );
  reset();
}

// dot_t::extend_duration ===================================================

void dot_t::extend_duration( int extra_ticks, bool cap, uint32_t state_flags )
{
  if ( ! ticking )
    return;

  if ( state_flags == ( uint32_t ) -1 ) state_flags = action -> snapshot_flags;

  // Make sure this DoT is still ticking......
  assert( tick_event );

  if ( sim -> log )
    sim -> output( "%s extends duration of %s on %s, adding %d tick(s), totalling %d ticks",
                   source -> name(), name(), target -> name(), extra_ticks, num_ticks + extra_ticks );

  if ( cap )
  {
    // Can't extend beyond initial duration.
    // Assuming this limit is based on current haste, not haste at previous application/extension/refresh.

    int max_extra_ticks;
    max_extra_ticks = std::max( action -> hasted_num_ticks( state -> haste ) - ticks(), 0 );

    extra_ticks = std::min( extra_ticks, max_extra_ticks );
  }

  assert( state );
  action -> snapshot_state( state, state_flags, action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

  added_ticks += extra_ticks;
  num_ticks += extra_ticks;
  recalculate_ready();

  action -> stats -> num_refreshes++;
}

// dot_t::extend_duration_seconds ===========================================

void dot_t::extend_duration_seconds( timespan_t extra_seconds, uint32_t state_flags )
{
  if ( ! ticking )
    return;

  if ( state_flags == ( uint32_t ) -1 ) state_flags = action -> snapshot_flags;

  // Make sure this DoT is still ticking......
  assert( tick_event );

  // Treat extra_ticks as 'seconds added' instead of 'ticks added'
  // Duration left needs to be calculated with old haste for tick_time()
  // First we need the number of ticks remaining after the next one =>
  // ( num_ticks - current_tick ) - 1
  int old_num_ticks = num_ticks;
  int old_remaining_ticks = old_num_ticks - current_tick - 1;
  double old_haste_factor = 0.0;

  old_haste_factor = 1.0 / state -> haste;

  // Multiply with tick_time() for the duration left after the next tick
  timespan_t duration_left;

  duration_left = old_remaining_ticks * action -> tick_time( state -> haste );

  // Add the added seconds
  duration_left += extra_seconds;

  assert( state );
  action -> snapshot_state( state, state_flags, action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

  added_seconds += extra_seconds;

  int new_remaining_ticks;

  new_remaining_ticks = action -> hasted_num_ticks( state -> haste, duration_left );

  num_ticks += ( new_remaining_ticks - old_remaining_ticks );

  if ( sim -> debug )
  {
    sim -> output( "%s extends duration of %s on %s by %.1f second(s). h: %.2f => %.2f, num_t: %d => %d, rem_t: %d => %d",
                   source -> name(), name(), target -> name(), extra_seconds.total_seconds(),
                   old_haste_factor, ( 1.0 / state -> haste ),
                   old_num_ticks, num_ticks,
                   old_remaining_ticks, new_remaining_ticks );
  }
  else if ( sim -> log )
  {
    sim -> output( "%s extends duration of %s on %s by %.1f second(s).",
                   source -> name(), name(), target -> name(), extra_seconds.total_seconds() );
  }

  recalculate_ready();

  action -> stats -> num_refreshes++;
}

// dot_t::recalculate_ready =================================================

void dot_t::recalculate_ready()
{
  // Extending a DoT does not interfere with the next tick event.  To determine the
  // new finish time for the DoT, start from the time of the next tick and add the time
  // for the remaining ticks to that event.
  int remaining_ticks = num_ticks - current_tick;
  if ( remaining_ticks > 0 && tick_event )
  {
    ready = tick_event -> time + action -> tick_time( state -> haste ) * ( remaining_ticks - 1 );
  }
  else
  {
    ready = timespan_t::zero();
  }
}

// dot_t::refresh_duration ==================================================

void dot_t::refresh_duration( uint32_t state_flags )
{
  if ( ! ticking )
    return;

  if ( state_flags == ( uint32_t ) -1 ) state_flags = action -> snapshot_flags;

  // Make sure this DoT is still ticking......
  assert( tick_event );

  if ( sim -> log )
    sim -> output( "%s refreshes duration of %s on %s", source -> name(), name(), target -> name() );

  assert( state );
  action -> snapshot_state( state, state_flags, action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

  current_tick = 0;
  added_ticks = 0;
  added_seconds = timespan_t::zero();

  num_ticks = action -> hasted_num_ticks( state -> haste );

  // tick zero dots tick when refreshed
  if ( action -> tick_zero )
  {
    action -> tick( this );
  }

  recalculate_ready();

  action -> stats -> num_refreshes++;
}

// dot_t::remains ===========================================================

timespan_t dot_t::remains()
{
  if ( ! action ) return timespan_t::zero();
  if ( ! ticking ) return timespan_t::zero();

  return ready - sim -> current_time;
}

// dot_t::reset =============================================================

void dot_t::reset()
{
  event_t::cancel( tick_event );
  current_tick=0;
  added_ticks=0;
  ticking=0;
  added_seconds=timespan_t::zero();
  ready=timespan_t::min();
  miss_time=timespan_t::min();
  prev_tick_amount = 0.0;
  if ( state )
  {
    action -> release_state( state );
    state = 0;
  }
}

// dot_t::schedule_tick =====================================================

void dot_t::schedule_tick()
{
  if ( sim -> debug )
    sim -> output( "%s schedules tick for %s on %s", source -> name(), name(), target -> name() );

  if ( current_tick == 0 )
  {
    prev_tick_amount = 0.0;
    if ( action -> tick_zero )
    {
      time_to_tick = timespan_t::zero();
      action -> tick( this );
      if ( current_tick == num_ticks )
      {
        action -> last_tick( this );
        return;
      }
    }
  }

  time_to_tick = action -> tick_time( state -> haste );

  tick_event = new ( sim ) dot_tick_event_t( sim, this, time_to_tick );

  ticking = 1;

  if ( action -> channeled )
  {
    // FIXME: Find some way to make this more realistic - the actor shouldn't have to recast quite this early
    if ( action -> chain && current_tick + 1 == num_ticks && action -> ready() )
    {
      // FIXME: We can probably use "source" instead of "action->player"

      action -> player -> channeling = 0;
      action -> player -> gcd_ready = sim -> current_time + action -> gcd();
      action -> execute();
      if ( action -> result_is_hit() )
      {
        action -> player -> channeling = action;
      }
      else
      {
        cancel();
        action -> player -> schedule_ready();
      }
    }
    else
    {
      action -> player -> channeling = action;
    }
  }
}

// dot_t::ticks =============================================================

int dot_t::ticks()
{
  if ( ! action ) return 0;
  if ( ! ticking ) return 0;
  return ( num_ticks - current_tick );
}

expr_t* dot_t::create_expression( action_t* action,
                                  const std::string& name_str,
                                  bool dynamic )
{
  struct dot_expr_t : public expr_t
  {
    dot_t* static_dot;
    action_t* action;
    bool dynamic;
    target_specific_t<dot_t> specific_dot;

    dot_expr_t( const std::string& n, dot_t* d, action_t* a, bool dy ) :
      expr_t( n ), static_dot( d ), action( a ), dynamic( dy ), specific_dot( d -> name(), a -> player ) {}

    dot_t* dot()
    {
      if ( ! dynamic ) return static_dot;
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
  else if ( name_str == "ticks_added" )
  {
    struct ticks_added_expr_t : public dot_expr_t
    {
      ticks_added_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "ticks_added", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> added_ticks; }
    };
    return new ticks_added_expr_t( this, action, dynamic );
  }
  else if ( name_str == "duration" )
  {
    struct duration_expr_t : public dot_expr_t
    {
      duration_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_duration", d, a, dynamic ) {}
      virtual double evaluate()
      {
        // FIXME: What exactly is this supposed to be calculating?
        dot_t* dot = this->dot();
        double haste = dot -> state -> haste;
        return ( dot -> action -> num_ticks * dot -> action -> tick_time( haste ) ).total_seconds();
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
        return dot() -> state ? dot() -> state -> result_amount : 0.0;
      }
    };
    return new tick_dmg_expr_t( this, action, dynamic );
  }
  else if ( name_str == "ticks_remain" )
  {
    struct ticks_remain_expr_t : public dot_expr_t
    {
      ticks_remain_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_ticks_remain", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> ticks(); }
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

#if 0
  else if ( name_str == "mastery" )
  {
    struct dot_mastery_expr_t : public dot_expr_t
    {
      dot_mastery_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_mastery", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> total_mastery() : 0; }
    };
    return new dot_mastery_expr_t( this, action, dynamic );
  }
#endif

  else if ( name_str == "haste_pct" )
  {
    struct dot_haste_pct_expr_t : public dot_expr_t
    {
      dot_haste_pct_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_haste_pct", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> haste : 0; }
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

  return 0;
}

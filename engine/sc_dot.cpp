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

  dot_tick_event_t( dot_t* d,
                    timespan_t time_to_tick ) :
    event_t( d -> source, "DoT Tick" ), dot( d )
  {
    if ( sim.debug )
      sim.output( "New DoT Tick Event: %s %s %d-of-%d %.2f",
                  player -> name(), dot -> name(), dot -> current_tick + 1, dot -> num_ticks, time_to_tick.total_seconds() );

    sim.add_event( this, time_to_tick );
  }

  // dot_tick_event_t::execute ================================================

  virtual void execute()
  {
    if ( dot -> current_tick >= dot -> num_ticks )
    {
      sim.errorf( "Player %s has corrupt tick (%d of %d) event on action %s!\n",
                  player -> name(), dot -> current_tick, dot -> num_ticks, dot -> name() );
      sim.cancel();
      assert(0);
    }

    dot -> tick_event = nullptr;
    dot -> current_tick++;

    if ( dot -> current_action -> player -> current.skill < 1.0 &&
         dot -> current_action -> channeled &&
         dot -> current_tick == dot -> num_ticks )
    {
      if ( sim.roll( dot -> current_action -> player -> current.skill ) )
      {
        dot -> current_action -> tick( dot );
      }
    }
    else // No skill-check required
    {
      dot -> current_action -> tick( dot );
    }

    if ( dot -> ticking )
    {
      expr_t* expr = dot -> current_action -> interrupt_if_expr;
      if ( dot -> current_tick == dot -> num_ticks
        || ( dot -> current_action -> channeled
          && dot -> ticks() > 0
          && dot -> current_action -> player -> gcd_ready <= sim.current_time
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
  }
};

} // end anonymous namespace

dot_t::dot_t( const std::string& n, player_t* t, player_t* s ) :
  sim( *( t -> sim ) ), target( t ), source( s ), current_action( 0 ), state( 0 ), tick_event( nullptr ),
  num_ticks( 0 ), current_tick( 0 ), added_ticks( 0 ), ticking( false ),
  added_seconds( timespan_t::zero() ), ready( timespan_t::min() ),
  miss_time( timespan_t::min() ),time_to_tick( timespan_t::zero() ),
  name_str( n ), tick_amount( 0.0 )
{}

bool dot_t::is_higher_priority_action_available()
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

void dot_t::last_tick()
{
  time_to_tick = timespan_t::zero();

  ticking = false;
  event_t::cancel( tick_event );

  if ( sim.debug )
    sim.output( "%s fades from %s", name(), state -> target -> name() );

  // call action_t::last_tick
  current_action -> last_tick( this );

  // reset variables
  if ( state )
    action_state_t::release( state );
  current_tick = 0;
  added_ticks = 0;

  // If channeled, bring player back to life
  if ( current_action -> channeled )
  {
    if ( current_action -> player -> readying )
      fprintf( sim.output_file, "Danger Will Robinson!  Danger!  %s\n", name() );

    current_action -> player -> schedule_ready( timespan_t::zero() );
  }
}

// dot_t::cancel ===================================================

void dot_t::cancel()
{
  if ( ! ticking )
    return;

  last_tick();
  reset();
}

// dot_t::extend_duration ===================================================

void dot_t::extend_duration( int extra_ticks, bool cap, uint32_t state_flags )
{
  if ( ! ticking )
    return;

  if ( state_flags == ( uint32_t ) -1 ) state_flags = current_action -> snapshot_flags;

  // Make sure this DoT is still ticking......
  if ( !tick_event )
  {
    sim.output( "foo" );
  }
  assert( tick_event && "dot_t::extend_duration: ticking==true but tick_event=nullptr" );

  if ( sim.log )
    sim.output( "%s extends duration of %s on %s, adding %d tick(s), totalling %d ticks, and snapshotting with %#.x",
                source -> name(), name(), target -> name(), extra_ticks, num_ticks + extra_ticks, state_flags );

  if ( cap )
  {
    // Can't extend beyond initial duration.
    // Assuming this limit is based on current haste, not haste at previous application/extension/refresh.

    int max_extra_ticks;
    max_extra_ticks = std::max( current_action -> hasted_num_ticks( state -> haste ) - ticks(), 0 );

    extra_ticks = std::min( extra_ticks, max_extra_ticks );
  }

  assert( state );
  current_action -> snapshot_internal( state, state_flags, current_action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

  added_ticks += extra_ticks;
  num_ticks += extra_ticks;
  recalculate_ready();

  current_action -> stats -> add_refresh( state -> target );
}

// dot_t::extend_duration_seconds ===========================================

void dot_t::extend_duration_seconds( timespan_t extra_seconds, uint32_t state_flags )
{
  if ( ! ticking )
    return;

  if ( state_flags == ( uint32_t ) -1 ) state_flags = current_action -> snapshot_flags;

  // Make sure this DoT is still ticking......
  assert( tick_event );

  // Treat extra_ticks as 'seconds added' instead of 'ticks added'
  // Duration left needs to be calculated with old haste for tick_time()
  // First we need the number of ticks remaining after the next one =>
  // ( num_ticks - current_tick ) - 1
  int old_num_ticks = num_ticks;
  int old_remaining_ticks = old_num_ticks - current_tick - 1;
  double old_haste_factor = 1.0 / state -> haste;

  // Multiply with tick_time() for the duration left after the next tick
  timespan_t duration_left;

  duration_left = old_remaining_ticks * current_action -> tick_time( state -> haste );

  // Add the added seconds
  duration_left += extra_seconds;

  assert( state );
  current_action -> snapshot_internal( state, state_flags, current_action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

  added_seconds += extra_seconds;

  int new_remaining_ticks;

  new_remaining_ticks = current_action -> hasted_num_ticks( state -> haste, duration_left );

  num_ticks += ( new_remaining_ticks - old_remaining_ticks );

  if ( sim.debug )
  {
    sim.output( "%s extends duration of %s on %s by %.1f second(s). h: %.2f => %.2f, num_t: %d => %d, rem_t: %d => %d",
                source -> name(), name(), target -> name(), extra_seconds.total_seconds(),
                old_haste_factor, ( 1.0 / state -> haste ),
                old_num_ticks, num_ticks,
                old_remaining_ticks, new_remaining_ticks );
  }
  else if ( sim.log )
  {
    sim.output( "%s extends duration of %s on %s by %.1f second(s).",
                source -> name(), name(), target -> name(), extra_seconds.total_seconds() );
  }

  recalculate_ready();

  current_action -> stats -> add_refresh( state -> target );
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
    ready = tick_event -> time + current_action -> tick_time( state -> haste ) * ( remaining_ticks - 1 );
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

  if ( state_flags == ( uint32_t ) -1 ) state_flags = current_action -> snapshot_flags;

  // Make sure this DoT is still ticking......
  assert( tick_event );

  if ( sim.log )
    sim.output( "%s refreshes duration of %s on %s", source -> name(), name(), target -> name() );

  assert( state );
  current_action -> snapshot_internal( state, state_flags, current_action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

  current_tick = 0;
  added_ticks = 0;
  added_seconds = timespan_t::zero();

  num_ticks = current_action -> hasted_num_ticks( state -> haste );

  // tick zero dots tick when refreshed
  if ( current_action -> tick_zero )
  {
    current_action -> tick( this );
  }

  recalculate_ready();

  current_action -> stats -> add_refresh( state -> target );
}

// dot_t::reset =============================================================

void dot_t::reset()
{
  event_t::cancel( tick_event );
  current_tick = 0;
  added_ticks = 0;
  ticking = false;
  added_seconds = timespan_t::zero();
  ready = timespan_t::min();
  miss_time = timespan_t::min();
  tick_amount = 0.0;
  if ( state )
    action_state_t::release( state );
}

// dot_t::schedule_tick =====================================================

void dot_t::schedule_tick()
{
  if ( sim.debug )
    sim.output( "%s schedules tick for %s on %s", source -> name(), name(), target -> name() );

  if ( current_tick == 0 )
  {
    if ( current_action -> tick_zero )
    {
      time_to_tick = timespan_t::zero();
      current_action -> tick( this );
      if ( current_tick == num_ticks )
      {
        last_tick();
        return;
      }
    }
  }

  time_to_tick = current_action -> tick_time( state -> haste );

  tick_event = new ( sim ) dot_tick_event_t( this, time_to_tick );

  ticking = true;

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
      if ( current_action -> result_is_hit() )
      {
        current_action -> player -> channeling = current_action;
      }
      else
      {
        cancel();
        current_action -> player -> schedule_ready();
      }
    }
    else
    {
      current_action -> player -> channeling = current_action;
    }
  }
}

// dot_t::ticks =============================================================

int dot_t::ticks()
{
  if ( ! current_action ) return 0;
  if ( ! ticking ) return 0;
  return ( num_ticks - current_tick );
}

// dot_t::copy ==============================================================

void dot_t::copy( player_t* other_target )
{
  assert( target != other_target );

  if ( ! ticking )
    return;

  dot_t* other_dot = current_action -> get_dot( other_target );

  if( ! other_dot -> state ) other_dot -> state = current_action -> get_state();

  other_dot -> state -> copy_state( state );
  other_dot -> state -> target = other_target;
  other_dot -> current_action = current_action;
  other_dot -> current_tick = current_tick;
  other_dot -> num_ticks = num_ticks;
  other_dot -> tick_amount = tick_amount;
  if( ! other_dot -> ticking ) other_dot -> schedule_tick();
  other_dot -> recalculate_ready();
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
      expr_t( n ), static_dot( d ), action( a ), dynamic( dy ), specific_dot() {}

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
        if ( ! dot -> state ) return 0;
        double haste = dot -> state -> haste;
        return ( dot -> current_action -> num_ticks * dot -> current_action -> tick_time( haste ) ).total_seconds();
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
    return new dot_mastery_expr_t( this, current_action, dynamic );
  }
#endif

  else if ( name_str == "haste_pct" )
  {
    struct dot_haste_pct_expr_t : public dot_expr_t
    {
      dot_haste_pct_expr_t( dot_t* d, action_t* a, bool dynamic ) :
        dot_expr_t( "dot_haste_pct", d, a, dynamic ) {}
      virtual double evaluate() { return dot() -> state ? dot() -> state -> haste * 100 : 0; }
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

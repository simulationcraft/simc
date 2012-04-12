// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Dot
// ==========================================================================

dot_t::dot_t( const std::string& n, player_t* p ) :
  sim( p -> sim ), player( p ), action( 0 ), tick_event( 0 ),
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

void dot_t::extend_duration( int extra_ticks, bool cap )
{
  if ( ! ticking )
    return;

  // Make sure this DoT is still ticking......
  assert( tick_event );

  if ( sim -> log )
    log_t::output( sim, "%s extends duration of %s on %s, adding %d tick(s), totalling %d ticks",
                   action -> player -> name(), name(), player -> name(), extra_ticks, num_ticks + extra_ticks );

  if ( cap )
  {
    // Can't extend beyond initial duration.
    // Assuming this limit is based on current haste, not haste at previous application/extension/refresh.

    int max_extra_ticks;
    if ( ! state )
      max_extra_ticks = std::max( action -> hasted_num_ticks( action -> player_haste ) - ticks(), 0 );
    else
      max_extra_ticks = std::max( action -> hasted_num_ticks( state -> haste ) - ticks(), 0 );

    extra_ticks = std::min( extra_ticks, max_extra_ticks );
  }

  if ( ! state )
    action -> player_buff();
  else
    action -> snapshot_state( state, action -> snapshot_flags );

  added_ticks += extra_ticks;
  num_ticks += extra_ticks;
  recalculate_ready();
}

// dot_t::extend_duration_seconds ===========================================

void dot_t::extend_duration_seconds( timespan_t extra_seconds )
{
  if ( ! ticking )
    return;

  // Make sure this DoT is still ticking......
  assert( tick_event );

  // Treat extra_ticks as 'seconds added' instead of 'ticks added'
  // Duration left needs to be calculated with old haste for tick_time()
  // First we need the number of ticks remaining after the next one =>
  // ( num_ticks - current_tick ) - 1
  int old_num_ticks = num_ticks;
  int old_remaining_ticks = old_num_ticks - current_tick - 1;
  double old_haste_factor = 0.0;
  if ( ! state )
    old_haste_factor = 1.0 / action -> player_haste;
  else
    old_haste_factor = 1.0 / state -> haste;

  // Multiply with tick_time() for the duration left after the next tick
  timespan_t duration_left;
  if ( ! state )
    duration_left = old_remaining_ticks * action -> tick_time( action -> player_haste );
  else
    duration_left = old_remaining_ticks * action -> tick_time( state -> haste );

  // Add the added seconds
  duration_left += extra_seconds;

  if ( ! state )
    action -> player_buff();
  else
    action -> snapshot_state( state, action -> snapshot_flags );

  added_seconds += extra_seconds;

  int new_remaining_ticks;
  if ( ! state )
    new_remaining_ticks = action -> hasted_num_ticks( action -> player_haste, duration_left );
  else
    new_remaining_ticks = action -> hasted_num_ticks( state -> haste, duration_left );

  num_ticks += ( new_remaining_ticks - old_remaining_ticks );

  if ( sim -> debug )
  {
    log_t::output( sim, "%s extends duration of %s on %s by %.1f second(s). h: %.2f => %.2f, num_t: %d => %d, rem_t: %d => %d",
                   action -> player -> name(), name(), player -> name(), extra_seconds.total_seconds(),
                   old_haste_factor, ( ! state ) ? ( 1.0 / action -> player_haste ) : ( 1.0 / state -> haste ),
                   old_num_ticks, num_ticks,
                   old_remaining_ticks, new_remaining_ticks );
  }
  else if ( sim -> log )
  {
    log_t::output( sim, "%s extends duration of %s on %s by %.1f second(s).",
                   action -> player -> name(), name(), player -> name(), extra_seconds.total_seconds() );
  }

  recalculate_ready();
}

// dot_t::recalculate_ready =================================================

void dot_t::recalculate_ready()
{
  // Extending a DoT does not interfere with the next tick event.  To determine the
  // new finish time for the DoT, start from the time of the next tick and add the time
  // for the remaining ticks to that event.
  int remaining_ticks = num_ticks - current_tick;
  if ( state )
    ready = tick_event -> time + action -> tick_time( state -> haste ) * ( remaining_ticks - 1 );
  else
    ready = tick_event -> time + action -> tick_time( action -> player_haste ) * ( remaining_ticks - 1 );
}

// dot_t::refresh_duration ==================================================

void dot_t::refresh_duration()
{
  if ( ! ticking )
    return;

  // Make sure this DoT is still ticking......
  assert( tick_event );

  if ( sim -> log )
    log_t::output( sim, "%s refreshes duration of %s on %s", action -> player -> name(), name(), player -> name() );

  action -> player_buff();

  current_tick = 0;
  added_ticks = 0;
  added_seconds = timespan_t::zero();
  if ( ! state )
    num_ticks = action -> hasted_num_ticks( action ->  player_haste );
  else
    num_ticks = action -> hasted_num_ticks( state -> haste );

  // tick zero dots tick when refreshed
  if ( action -> tick_zero )
    action -> tick( this );

  recalculate_ready();
}

// dot_t::remains ===========================================================

timespan_t dot_t::remains() const
{
  if ( ! action ) return timespan_t::zero();
  if ( ! ticking ) return timespan_t::zero();

  return ready - player -> sim -> current_time;
}

// dot_t::reset =============================================================

void dot_t::reset()
{
  tick_event=0;
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
    log_t::output( sim, "%s schedules tick for %s on %s", action -> player -> name(), name(), player -> name() );

  if ( current_tick == 0 )
  {
    prev_tick_amount = 0.0;
    if ( action -> tick_zero )
    {
      time_to_tick = timespan_t::zero();
      action -> tick( this );
    }
  }

  if ( ! action -> stateless )
    time_to_tick = action -> tick_time( action -> player_haste );
  else
    time_to_tick = action -> tick_time( state -> haste );

  tick_event = new ( sim ) dot_tick_event_t( sim, this, time_to_tick );

  ticking = 1;

  if ( action -> channeled ) action -> player -> channeling = action;
}

// dot_t::ticks =============================================================

int dot_t::ticks() const
{
  if ( ! action ) return 0;
  if ( ! ticking ) return 0;
  return ( num_ticks - current_tick );
}

struct dot_action_expr_t : public action_expr_t
{
  const dot_t* dot;

  dot_action_expr_t( action_t* a, dot_t* d, const std::string& n, token_type_e t ) :
    action_expr_t( a, n, t ), dot( d )
  {}
};

action_expr_t* dot_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "ticks" )
  {
    struct ticks_expr_t : public dot_action_expr_t
    {
      ticks_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "ticks", TOK_NUM ) {}
      virtual int evaluate() { result_num = dot -> current_tick; return TOK_NUM; }
    };
    return new ticks_expr_t( a, this );
  }
  else if ( name_str == "duration" )
  {
    struct duration_expr_t : public dot_action_expr_t
    {
      duration_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_duration", TOK_NUM ) {}
      virtual int evaluate()
      {
        double haste = ( dot -> state ) ? dot -> state -> haste : action -> player_haste;
        result_num = action -> num_ticks * action -> tick_time( haste ).total_seconds(); return TOK_NUM;
      }
    };
    return new duration_expr_t( a, this );
  }
  else if ( name_str == "remains" )
  {
    struct dot_remains_expr_t : public dot_action_expr_t
    {
      dot_remains_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_remains", TOK_NUM ) {}
      virtual int evaluate() { result_num = dot -> remains().total_seconds(); return TOK_NUM; }
    };
    return new dot_remains_expr_t( a, this );
  }
  else if ( name_str == "tick_dmg" )
  {
    struct dot_tick_dmg_expr_t : public dot_action_expr_t
    {
      dot_tick_dmg_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_tick_dmg", TOK_NUM ) {}
      virtual int evaluate() { result_num = dot -> prev_tick_amount; return TOK_NUM; }
    };
    return new dot_tick_dmg_expr_t( a, this );
  }
  else if ( name_str == "ticks_remain" )
  {
    struct dot_ticks_remain_expr_t : public dot_action_expr_t
    {
      dot_ticks_remain_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_ticks_remain", TOK_NUM ) {}
      virtual int evaluate() { result_num = dot -> ticks(); return TOK_NUM; }
    };
    return new dot_ticks_remain_expr_t( a, this );
  }
  else if ( name_str == "ticking" )
  {
    struct dot_ticking_expr_t : public dot_action_expr_t
    {
      dot_ticking_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_ticking", TOK_NUM ) {}
      virtual int evaluate() { result_num = dot -> ticking; return TOK_NUM; }
    };
    return new dot_ticking_expr_t( a, this );
  }
  else if ( name_str == "spell_power" )
  {
    struct dot_spell_power_expr_t : public dot_action_expr_t
    {
      dot_spell_power_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_spell_power", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( dot -> state ) ? dot -> state -> spell_power : 0; return TOK_NUM; }
    };
    return new dot_spell_power_expr_t( a, this );
  }
  else if ( name_str == "attack_power" )
  {
    struct dot_attack_power_expr_t : public dot_action_expr_t
    {
      dot_attack_power_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_attack_power", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( dot -> state ) ? dot -> state -> attack_power : 0; return TOK_NUM; }
    };
    return new dot_attack_power_expr_t( a, this );
  }

  /*
  else if ( name_str == "mastery" )
  {
    struct dot_mastery_expr_t : public dot_action_expr_t
    {
      dot_mastery_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_mastery", TOK_NUM ), dot( d ) {}
      virtual int evaluate() { result_num = ( dot -> state ) ? dot -> state -> total_mastery() : 0; return TOK_NUM; }
    };
    return new dot_mastery_expr_t( a, this );
  }
  */
  else if ( name_str == "haste_pct" )
  {
    struct dot_haste_pct_expr_t : public dot_action_expr_t
    {
      dot_haste_pct_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_haste_pct", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( dot -> state ) ? dot -> state -> haste : 0; return TOK_NUM; }
    };
    return new dot_haste_pct_expr_t( a, this );
  }
  else if ( name_str == "current_ticks" )
  {
    struct dot_current_ticks_expr_t : public dot_action_expr_t
    {
      dot_current_ticks_expr_t( action_t* a, dot_t* d ) : dot_action_expr_t( a, d, "dot_current_ticks", TOK_NUM ) {}
      virtual int evaluate() { result_num = dot -> num_ticks; return TOK_NUM; }
    };
    return new dot_current_ticks_expr_t( a, this );
  }

  return 0;
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

action_state_t* action_t::get_state( const action_state_t* other )
{
  action_state_t* s = 0;

  if ( state_cache != 0 )
  {
    s = state_cache;
    state_cache = s -> next;
    s -> next = 0;
  }
  else
    s = new_state();

  s -> copy_state( other );
  s -> result = RESULT_NONE;
  s -> result_amount = 0;

  return s;
}

action_state_t* action_t::new_state()
{
  return new action_state_t( this, target );
}

void action_t::release_state( action_state_t* s )
{
  s -> next = state_cache;
  state_cache = s;
}

void action_state_t::copy_state( const action_state_t* o )
{
  if ( this == o || o == 0 ) return;

  action = o -> action; target = o -> target;
  result = o -> result; result_amount = o -> result_amount;
  haste = o -> haste;
  crit = o -> crit;
  target_crit = o -> target_crit;
  attack_power = o -> attack_power;
  spell_power = o -> spell_power;

  da_multiplier = o -> da_multiplier;
  ta_multiplier = o -> ta_multiplier;

  target_da_multiplier = o -> target_da_multiplier;
  target_ta_multiplier = o -> target_ta_multiplier;
}

action_state_t::action_state_t( action_t* a, player_t* t ) :
  action( a ), target( t ),
  result( RESULT_NONE ), result_amount( 0 ),
  haste( 0 ), crit( 0 ), target_crit( 0 ),
  attack_power( 0 ), spell_power( 0 ),
  da_multiplier( 1.0 ), ta_multiplier( 1.0 ),
  target_da_multiplier( 1.0 ), target_ta_multiplier( 1.0 ),
  next( 0 )
{
}

void action_state_t::debug()
{
  action -> sim -> output( "[NEW] %s %s %s: obj=%p snapshot_flags=%#.4x update_flags=%#.4x result=%s amount=%.2f "
                           "haste=%.2f crit=%.2f tgt_crit=%.2f "
                           "ap=%.0f sp=%.0f "
                           "da_mul=%.4f ta_mul=%.4f tgt_da_mul=%.4f tgt_ta_mul=%.4f",
                           action -> player -> name(),
                           action -> name(),
                           target -> name(),
                           this,
                           action -> snapshot_flags,
                           action -> update_flags,
                           util::result_type_string( result ), result_amount,
                           haste, crit, target_crit,
                           attack_power, spell_power,
                           da_multiplier, ta_multiplier,
                           target_da_multiplier, target_ta_multiplier );
}

stateless_travel_event_t::stateless_travel_event_t( sim_t*    sim,
                                                    action_t* a,
                                                    action_state_t* state,
                                                    timespan_t time_to_travel ) :
  event_t( sim, a -> player, "Stateless Action Travel" ), action( a ), state( state )
{
  if ( sim -> debug )
    sim -> output( "New Stateless Action Travel Event: %s %s %.2f",
                   player -> name(), a -> name(), time_to_travel.total_seconds() );

  sim -> add_event( this, time_to_travel );
}

void stateless_travel_event_t::execute()
{
  action -> impact_s( state );
  action -> release_state( state );
  action -> remove_travel_event( this );
}

void action_t::schedule_travel_s( action_state_t* s )
{
  time_to_travel = travel_time();

  if ( ! execute_state )
    execute_state = get_state();

  execute_state -> copy_state( s );

  if ( time_to_travel == timespan_t::zero() )
  {
    impact_s( s );
    release_state( s );
  }
  else
  {
    if ( sim -> log )
    {
      sim -> output( "[NEW] %s schedules travel (%.3f) for %s", player -> name(), time_to_travel.total_seconds(), name() );
    }

    add_travel_event( new ( sim ) stateless_travel_event_t( sim, this, s, time_to_travel ) );
  }
}

void action_t::impact_s( action_state_t* s )
{
  assess_damage( s -> target, s -> result_amount, type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT, s -> result, s );

  if ( result_is_hit( s -> result ) )
  {
    if ( num_ticks > 0 || tick_zero )
    {
      dot_t* dot = get_dot( s -> target );
      int remaining_ticks = dot -> num_ticks - dot -> current_tick;
      if ( dot_behavior == DOT_CLIP ) dot -> cancel();
      dot -> action = this;
      dot -> current_tick = 0;
      dot -> added_ticks = 0;
      dot -> added_seconds = timespan_t::zero();
      // Snapshot the stats
      if ( ! dot -> state )
        dot -> state = get_state( s );
      else
        dot -> state -> copy_state( s );
      dot -> num_ticks = hasted_num_ticks( dot -> state -> haste );

      if ( dot -> ticking )
      {
        assert( dot -> tick_event );

        if ( dot_behavior == DOT_EXTEND ) dot -> num_ticks += std::min( ( int ) ( dot -> num_ticks / 2 ), remaining_ticks );

        // Recasting a dot while it's still ticking gives it an extra tick in total
        dot -> num_ticks++;

        // tick_zero dots tick again when reapplied
        if ( tick_zero )
        {
          // this is hacky, but otherwise uptime gets messed up
          timespan_t saved_tick_time = dot -> time_to_tick;
          dot -> time_to_tick = timespan_t::zero();
          tick( dot );
          dot -> time_to_tick = saved_tick_time;
        }
      }
      else
      {
        if ( school == SCHOOL_BLEED ) s -> target -> debuffs.bleeding -> increment();

        dot -> schedule_tick();
      }
      dot -> recalculate_ready();

      if ( sim -> debug )
        sim -> output( "%s extends dot-ready to %.2f for %s (%s)",
                       player -> name(), dot -> ready.total_seconds(), name(), dot -> name() );
    }
  }
  else
  {
    if ( sim -> log )
    {
      sim -> output( "Target %s avoids %s %s (%s)", target -> name(), player -> name(), name(), util::result_type_string( s -> result ) );
    }
  }
}

// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Sequence Action
// ==========================================================================

// sequence_t::sequence_t ===================================================

sequence_t::sequence_t( player_t* p, const std::string& sub_action_str ) :
  action_t( ACTION_SEQUENCE, "default", p ),
  waiting( false ), sequence_wait_on_ready( -1 ),
  current_action( -1 ), restarted( false ), last_restart( timespan_t::min() )
{
  trigger_gcd = timespan_t::zero();

  std::vector<std::string> splits = util::string_split( sub_action_str, ":" );
  if ( ! splits.empty() )
  {
    add_option( opt_string( "name", name_str ) );
    parse_options( splits[ 0 ] );
  }

  // First token is sequence options, so skip
  for ( size_t i = 1; i < splits.size(); ++i )
  {
    std::string::size_type cut_pt = splits[ i ].find( ',' );
    std::string action_name( splits[ i ], 0, cut_pt );
    std::string action_options;

    if ( cut_pt != std::string::npos )
      action_options.assign( splits[ i ], cut_pt + 1, std::string::npos );

    action_t* a = p -> create_action( action_name, action_options );
    if ( ! a )
    {
      sim -> errorf( "Player %s has unknown sequence action: %s\n", p -> name(), splits[ i ].c_str() );
      sim -> cancel();
      continue;
    }

    a -> sequence = true;
    sub_actions.push_back( a );
  }
  sequence_wait_on_ready = option.wait_on_ready;
  option.wait_on_ready = -1;
}

// sequence_t::schedule_execute =============================================

void sequence_t::schedule_execute( action_state_t* execute_state )
{
  assert( 0 <= current_action && as<std::size_t>( current_action ) < sub_actions.size() );

  if ( waiting )
  {
    if ( sim -> log )
      sim -> out_log.printf( "Player %s is waiting for %.3f, since action #%d (%s) is not ready", player -> name(), player -> available().total_seconds(), current_action, sub_actions[ current_action ] -> name() );
    player -> schedule_ready( player -> available(), true );
    waiting = false;
    return;
  }

  if ( sim -> log )
    sim -> out_log.printf( "Player %s executes Schedule %s action #%d \"%s\"",
                   player -> name(), name(), current_action, sub_actions[ current_action ] -> name() );

  sub_actions[ current_action++ ] -> schedule_execute( execute_state );

  // No longer restarted
  restarted = false;
}

// sequence_t::reset ========================================================

void sequence_t::reset()
{
  action_t::reset();
  /*
    if ( current_action == -1 )
    {
      for ( size_t i = 0; i < sub_actions.size(); ++i )
      {
        if ( sub_actions[ i ] -> wait_on_ready == -1 )
          sub_actions[ i ] -> wait_on_ready = wait_on_ready;
      }
    }
  */
  waiting = false;
  current_action = 0;
  restarted = false;
  last_restart = timespan_t::min();
}

// sequence_t::ready ========================================================

bool sequence_t::ready()
{
  if ( sub_actions.empty() ) return false;
  if ( ! action_t::ready() ) return false;

  for ( int num_sub_actions = as<int>( sub_actions.size() ); current_action < num_sub_actions; ++current_action )
  {
    action_t* a = sub_actions[ current_action ];

    if ( a -> ready() )
      return true;

    // If the sequence is flagged wait_on_ready, we'll wait for available()
    // amount of time, but do not proceed further in the action list, nor
    // in the sequence
    if ( sequence_wait_on_ready == 1 )
    {
      waiting = true;
      return true;
    }
  }

  return false;
}

// ==========================================================================
// Strict Sequence Action
// ==========================================================================

strict_sequence_t::strict_sequence_t( player_t* p, const std::string& sub_action_str ) :
  action_t( ACTION_SEQUENCE, "strict_sequence", p ),
  current_action( 0 ), allow_skip( false )
{
  trigger_gcd = timespan_t::zero();

  std::vector<std::string> splits = util::string_split( sub_action_str, ":" );
  if ( ! splits.empty() )
  {
    add_option( opt_bool( "allow_skip", allow_skip ) );
    add_option( opt_string( "name", seq_name_str ) );
    parse_options( splits[ 0 ] );
  }

  // First token is sequence options, so skip
  for ( size_t i = 1; i < splits.size(); ++i )
  {
    std::string::size_type cut_pt = splits[ i ].find( ',' );
    std::string action_name( splits[ i ], 0, cut_pt );
    std::string action_options;

    if ( cut_pt != std::string::npos )
      action_options.assign( splits[ i ], cut_pt + 1, std::string::npos );

    action_t* a = p -> create_action( action_name, action_options );
    if ( ! a )
    {
      sim -> errorf( "Player %s has unknown strict sequence '%s' action: %s\n", p -> name(), seq_name_str.c_str(), splits[ i ].c_str() );
      sim -> cancel();
      continue;
    }

    a -> sequence = true;
    sub_actions.push_back( a );
  }
}

void strict_sequence_t::cancel()
{
  action_t::cancel();

  if ( player -> strict_sequence == this )
    player -> strict_sequence = nullptr;

  current_action = 0;
}

void strict_sequence_t::reset()
{
  action_t::reset();

  if ( player -> strict_sequence == this )
    player -> strict_sequence = nullptr;

  current_action = 0;
}

void strict_sequence_t::interrupt_action()
{
  action_t::interrupt_action();

  if ( player -> strict_sequence == this )
    player -> strict_sequence = nullptr;

  current_action = 0;
}

bool strict_sequence_t::ready()
{
  if ( sub_actions.empty() ) return false;
  if ( ! action_t::ready() ) return false;

  // Strict sequences need all actions to be usable before it commits
  if ( ! allow_skip )
  {
    for (auto & elem : sub_actions)
    {
      if ( ! elem -> ready() )
        return false;
    }

    return true;
  }
  else
  {
    auto it = range::find_if( sub_actions, []( action_t* a ) {
        return ! a -> background && a -> ready() == true;
    } );
    return it != sub_actions.end(); // Ready if at least one action is usable
  }
}

void strict_sequence_t::schedule_execute( action_state_t* state )
{
  assert( ( current_action == 0 && ! player -> strict_sequence ) ||
          ( current_action < sub_actions.size() && player -> strict_sequence ) );
  // Check that we're not discarding the state since we cannot give it to queue_execute.
  assert( ! state );

  if ( current_action == 0 )
    player -> strict_sequence = this;

  bool scheduled = false;
  if ( ! allow_skip )
  {
    if ( sim -> log )
      sim -> out_log.printf( "Player %s executes strict_sequence '%s' action #%d \"%s\"",
                     player -> name(), seq_name_str.c_str(), current_action, sub_actions[ current_action ] -> name() );

    // Sanity check the strict sequence, so that it does not get stuck
    // mid-sequence. People are expected to write strict_sequences that cannot
    // have this happen, but it's bound to be a problem if some buff for example
    // fades between the first, and last cast (and it's needed by the last cast).
    if ( ! sub_actions[ current_action ] -> ready() )
    {
      if ( sim -> debug )
        sim -> out_debug.printf( "Player %s strict_sequence '%s' action #%d \"%s\" is no longer ready, aborting sequence",
          player -> name(), seq_name_str.c_str(), current_action, sub_actions[ current_action ] -> name() );
      cancel();
      return;
    }

    sub_actions[ current_action++ ] -> queue_execute( execute_type::FOREGROUND );
    scheduled = true;
  }
  else
  {
    // Allow skipping of unusable actions
    while ( current_action < sub_actions.size() &&
            ! sub_actions[ current_action ] -> background &&
            ! sub_actions[ current_action ] -> ready() )
    {
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "Player %s strict_sequence '%s' action #%d \"%s\" is not ready, skipping... ",
          player -> name(), seq_name_str.c_str(), current_action, sub_actions[ current_action ] -> name() );
      }
      current_action++;
    }

    if ( current_action < sub_actions.size() )
    {
      if ( sim -> log )
        sim -> out_log.printf( "Player %s executes strict_sequence '%s' action #%d \"%s\"",
                       player -> name(), seq_name_str.c_str(), current_action, sub_actions[ current_action ] -> name() );

      player -> sequence_add( sub_actions[ current_action ], sub_actions[ current_action ] -> target, sim -> current_time() );
      sub_actions[ current_action++ ] -> queue_execute( execute_type::FOREGROUND );
      scheduled = true;
    }
  }

  // Strict sequence is over, normal APL commences on the next ready event
  if ( current_action == sub_actions.size() )
  {
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "Player %s strict_sequence '%s' end reached, strict sequence over",
        player -> name(), seq_name_str.c_str() );
    }
    player -> strict_sequence = nullptr;
    current_action = 0;

    // If nothing to scheduled, the skippable strict sequence finished and went to the end. Player
    // ready event must be re-kicked so that the actor will not freeze.
    if ( ! scheduled )
    {
      player -> schedule_ready();
    }
  }
}


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
    option_t options[] =
    {
      opt_string( "name", name_str ),
      opt_null()
    };
    parse_options( options, splits[ 0 ] );
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
  sequence_wait_on_ready = wait_on_ready;
  wait_on_ready = -1;
}

// sequence_t::schedule_execute =============================================

void sequence_t::schedule_execute()
{
  assert( 0 <= current_action && static_cast<std::size_t>( current_action ) < sub_actions.size() );

  if ( waiting )
  {
    if ( sim -> log )
      sim -> output( "Player %s is waiting for %.3f, since action #%d (%s) is not ready", player -> name(), player -> available().total_seconds(), current_action, sub_actions[ current_action ] -> name() );
    player -> schedule_ready( player -> available(), true );
    waiting = false;
    return;
  }

  if ( sim -> log )
    sim -> output( "Player %s executes Schedule %s action #%d \"%s\"",
                   player -> name(), name(), current_action, sub_actions[ current_action ] -> name() );

  sub_actions[ current_action++ ] -> schedule_execute();

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

  for ( int num_sub_actions = static_cast<int>( sub_actions.size() ); current_action < num_sub_actions; ++current_action )
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

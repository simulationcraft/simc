// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sequence.hpp"

#include "player/sc_player.hpp"
#include "sim/sc_option.hpp"
#include "sim/sc_sim.hpp"
#include "util/util.hpp"

// ==========================================================================
// Sequence Action
// ==========================================================================

// sequence_t::sequence_t ===================================================

sequence_t::sequence_t( player_t* p, util::string_view sub_action_str ) :
  action_t( ACTION_SEQUENCE, "default", p ),
  waiting( false ), sequence_wait_on_ready( -1 ),
  current_action( -1 ), restarted( false ), last_restart( timespan_t::min() ), has_option( false )
{
  trigger_gcd = timespan_t::zero();

  auto splits = util::string_split<util::string_view>( sub_action_str, ":" );
  if ( !splits.empty() && splits[ 0 ].find( '=' ) != std::string::npos )
  {
    add_option( opt_string( "name", name_str ) );
    parse_options( splits[ 0 ] );
    has_option = true;
  }

  // Skip first token if it's an option
  for ( size_t i = as<size_t>( has_option ); i < splits.size(); ++i )
  {
    auto cut_pt      = splits[ i ].find( ',' );
    auto action_name = splits[ i ].substr( 0, cut_pt );
    std::string action_options;

    if ( cut_pt != util::string_view::npos )
      action_options = std::string( splits[ i ].substr( cut_pt + 1 ) );

    action_t* a = p -> create_action( action_name, action_options );
    if ( ! a )
    {
      sim -> error( "Player {} has unknown sequence action: {}\n", p -> name(), splits[ i ] );
      sim -> cancel();
      continue;
    }

    a -> sequence = true;
    sub_actions.push_back( a );
  }
  sequence_wait_on_ready = option.wait_on_ready;
  option.wait_on_ready = -1;
}

// sequence_t::init_finished ================================================

void sequence_t::init_finished()
{
  action_t::init_finished();

  // Clean-up invalid actions
  sub_actions.erase( std::remove_if( sub_actions.begin(), sub_actions.end(), [] ( action_t* a ) { return a -> background; } ),
                     sub_actions.end() );
}

// sequence_t::schedule_execute =============================================

void sequence_t::schedule_execute( action_state_t* execute_state )
{
  assert( 0 <= current_action && as<std::size_t>( current_action ) < sub_actions.size() );

  if ( waiting )
  {
    timespan_t available = player -> available();
    if ( sim -> log )
      sim -> out_log.printf( "Player %s is waiting for %.3f, since action #%d (%s) is not ready", player -> name(), available.total_seconds(), current_action, sub_actions[ current_action ] -> name() );
    player -> schedule_ready( available, true );
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

strict_sequence_t::strict_sequence_t( player_t* p, util::string_view options )
  : action_t( ACTION_SEQUENCE, "strict_sequence", p ), current_action( 0 ), allow_skip( false )
{
  trigger_gcd = timespan_t::zero();

  auto splits = util::string_split<util::string_view>( options, ":" );
  if ( ! splits.empty() )
  {
    add_option( opt_bool( "allow_skip", allow_skip ) );
    add_option( opt_string( "name", seq_name_str ) );
    parse_options( splits[ 0 ] );
  }

  // First token is sequence options, so skip
  for ( size_t i = 1; i < splits.size(); ++i )
  {
    auto cut_pt      = splits[ i ].find( ',' );
    auto action_name = splits[ i ].substr( 0, cut_pt );
    std::string action_options;

    if ( cut_pt != util::string_view::npos )
      action_options = std::string( splits[ i ].substr( cut_pt + 1 ) );

    action_t* a = p -> create_action( action_name, action_options );
    if ( ! a )
    {
      sim -> error( "Player {} has unknown strict sequence '{}' action: {}\n", p -> name(), seq_name_str, splits[ i ] );
      sim -> cancel();
      continue;
    }

    a -> sequence = true;
    sub_actions.push_back( a );
  }
}

void strict_sequence_t::init_finished()
{
  action_t::init_finished();

  // Clean-up invalid actions
  sub_actions.erase( std::remove_if( sub_actions.begin(), sub_actions.end(), [] ( action_t* a ) { return a -> background; } ),
                     sub_actions.end() );
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
    // Ready if at least one action is usable
    return range::any_of( sub_actions, []( action_t* a ) {
        return ! a -> background && a -> ready();
    } );
  }
}

void strict_sequence_t::schedule_execute( action_state_t* state )
{
  assert( ( current_action == 0 && ! player -> strict_sequence ) ||
          ( current_action < sub_actions.size() && player -> strict_sequence ) );
  // Check that we're not discarding the state since we cannot give it to queue_execute.
  (void)state; assert( ! state );

  if ( current_action == 0 )
    player -> strict_sequence = this;

  bool scheduled = false;
  if ( ! allow_skip )
  {
    if ( sim -> log )
      sim -> out_log.print( "{} executes strict_sequence '{}' action #{} \"{}\"",
                     *player, seq_name_str, current_action, sub_actions[ current_action ] -> name() );

    // Sanity check the strict sequence, so that it does not get stuck
    // mid-sequence. People are expected to write strict_sequences that cannot
    // have this happen, but it's bound to be a problem if some buff for example
    // fades between the first, and last cast (and it's needed by the last cast).
    if ( ! sub_actions[ current_action ] -> ready() )
    {
      if ( sim -> debug )
        sim -> out_debug.print( "{} strict_sequence '{}' action #{} \"{}\" is no longer ready, aborting sequence",
          *player, seq_name_str, current_action, sub_actions[ current_action ] -> name() );
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
        sim -> out_debug.print( "{} strict_sequence '{}' action #{} \"{}\" is not ready, skipping... ",
          *player, seq_name_str, current_action, sub_actions[ current_action ] -> name() );
      }
      current_action++;
    }

    if ( current_action < sub_actions.size() )
    {
      sim -> print_log( "{} executes strict_sequence '{}' action #{} \"{}\"",
                       *player, seq_name_str, current_action, sub_actions[ current_action ] -> name() );

      player -> sequence_add( sub_actions[ current_action ], sub_actions[ current_action ] -> target, sim -> current_time() );
      sub_actions[ current_action++ ] -> queue_execute( execute_type::FOREGROUND );
      scheduled = true;
    }
  }

  // Strict sequence is over, normal APL commences on the next ready event
  if ( current_action == sub_actions.size() )
  {
    sim -> print_debug( "{} strict_sequence '{}' end reached, strict sequence over",
        *player, seq_name_str );
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

void sequence_t::restart() { current_action = 0; restarted = true; last_restart = sim->current_time(); }

bool sequence_t::can_restart()
{
  return !restarted && last_restart < sim->current_time();
}

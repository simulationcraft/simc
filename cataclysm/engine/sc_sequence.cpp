// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Sequence Action
// ==========================================================================

// sequence_t::sequence_t ===================================================

sequence_t::sequence_t( player_t* p, const std::string& sub_action_str ) :
    action_t( ACTION_SEQUENCE, "default", p ), current_action( -1 )
{
  trigger_gcd = 0;

  std::vector<std::string> splits;
  int size = util_t::string_split( splits, sub_action_str, ":" );
  
  option_t options[] =
  {
    { "name", OPT_STRING,  &name_str },
    { NULL, OPT_UNKNOWN, NULL }
  };
  parse_options( options, splits[ 0 ] );

  // First token is sequence options, so skip
  for ( int i=1; i < size; i++ )
  {
    std::string action_name    = splits[ i ];
    std::string action_options = "";

    std::string::size_type cut_pt = action_name.find_first_of( "," );

    if ( cut_pt != action_name.npos )
    {
      action_options = action_name.substr( cut_pt + 1 );
      action_name    = action_name.substr( 0, cut_pt );
    }

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
}

// sequence_t::~sequence_t ===================================================

sequence_t::~sequence_t()
{
}

// sequence_t::schedule_execute ==============================================

void sequence_t::schedule_execute()
{
  sub_actions[ current_action++ ] -> schedule_execute();
}

// sequence_t::reset =========================================================

void sequence_t::reset()
{
  action_t::reset();
  if ( current_action == -1 )
  {
    for ( unsigned i=0; i < sub_actions.size(); i++ )
    {
      action_t* a = sub_actions[ i ];
      if ( a -> wait_on_ready == -1 )
      {
        a -> wait_on_ready = wait_on_ready;
      }
    }
  }
  current_action = 0;
}

// sequence_t::ready =========================================================

bool sequence_t::ready()
{
  int size = ( int ) sub_actions.size();
  if ( size == 0 ) return false;

  wait_on_ready = 0;

  while ( current_action < size )
  {
    action_t* a = sub_actions[ current_action ];

    if ( a -> ready() )
      return true;

    if ( a -> wait_on_ready == 1 )
    {
      wait_on_ready = 1;
      return false;
    }

    current_action++;
  }
  return false;
}

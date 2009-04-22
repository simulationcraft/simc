// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Sequence Action 
// ==========================================================================

// sequence_t::sequence_t ===================================================

sequence_t::sequence_t( const char* n, player_t* p, const std::string& sub_action_str ) :
  action_t( ACTION_OTHER, n, p ), current_action(0)
{
  trigger_gcd = 0;

  std::vector<std::string> splits;
  int size = util_t::string_split( splits, sub_action_str, ":" );

  // First token is sequence options, so skip
  for( int i=1; i < size; i++ )  
  {
    std::string action_name    = splits[ i ];
    std::string action_options = "";

    std::string::size_type cut_pt = action_name.find_first_of( "," );       

    if( cut_pt != action_name.npos )
    {
      action_options = action_name.substr( cut_pt + 1 );
      action_name    = action_name.substr( 0, cut_pt );
    }

    action_t* a = action_t::create_action( p, action_name, action_options );
    if( ! a )
    {
      printf( "sequence_t: Unknown action: %s\n", splits[ i ].c_str() );
      assert( false );
    }

    a -> background = true;
    sub_actions.push_back( a );
  }
  if( sub_actions.size() == 0 )
  {
    printf( "sequence_t: No sub-actions!\n" );
    assert( false );
  }
}

// sequence_t::~sequence_t ===================================================

sequence_t::~sequence_t() 
{
  for( unsigned i=0; i < sub_actions.size(); i++ )
  {
    delete sub_actions[ i ];
  }
}

// sequence_t::schedule_execute ==============================================

void sequence_t::schedule_execute()
{
  sub_actions[ current_action ] -> schedule_execute();
}

// sequence_t::reset =========================================================

void sequence_t::reset()
{
  action_t::reset();
  current_action = 0;
}

// sequence_t::ready =========================================================

bool sequence_t::ready()
{
  int size = sub_actions.size();
  if( size == 0 ) return false;

  while( current_action < size )
  {
    if( sub_actions[ current_action ] -> ready() )
      return true;
  }
  return false;
}

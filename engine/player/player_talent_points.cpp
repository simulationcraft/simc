  // ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player_talent_points.hpp"
  
bool player_talent_points_t::validate( const spell_data_t* spell, int row, int col ) const
{
return has_row_col( row, col ) ||
    range::find_if( validity_fns, [ spell ]( const validity_fn_t& fn ) { return fn( spell ); } ) !=
    validity_fns.end();
}
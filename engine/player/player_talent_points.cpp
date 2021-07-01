// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player_talent_points.hpp"
#include "util/util.hpp"
#include "fmt/format.h"

bool player_talent_points_t::validate( const spell_data_t* spell, int row, int col ) const
{
  return has_row_col( row, col ) || range::any_of( _validity_fns, [spell]( const validity_fn_t& fn ) {
                                      return fn( spell );
                                    } );
}

void player_talent_points_t::clear()
{
  range::fill( _choices, -1 );
}

util::span<const int> player_talent_points_t::choices() const
{
  return util::make_span(_choices);
}

std::string player_talent_points_t::to_string() const
{
  return fmt::format("{{ {} }}", util::string_join(choices(), ", "));
}

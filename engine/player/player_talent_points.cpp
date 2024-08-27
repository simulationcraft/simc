// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player_talent_points.hpp"

#include "dbc/dbc.hpp"
#include "fmt/format.h"
#include "player/player.hpp"
#include "sim/sim.hpp"
#include "util/util.hpp"

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

void player_talent_points_t::parse_talents_numbers( std::string_view talent_string )
{
  clear();

  int i_max = std::min( static_cast<int>( talent_string.size() ), MAX_TALENT_ROWS );

  for ( int i = 0; i < i_max; ++i )
  {
    char c = talent_string[ i ];
    if ( c < '0' || c > ( '0' + MAX_TALENT_COLS ) )
    {
      throw std::runtime_error(fmt::format("Illegal character '{}' in talent encoding.", c ));
    }
    if ( c > '0' )
      select_row_col( i, c - '1' );
  }
}

void player_talent_points_t::replace_spells()
{
  // Search talents for spells to replace.
  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      if ( has_row_col( j, i ) && player->true_level < 20 + ( j == 0 ? -1 : j ) * 5 )
      {
        const talent_data_t* td = talent_data_t::find( player->type, j, i, player->specialization(), player->is_ptr() );
        if ( td && td->replace_id() )
        {
          player->dbc->replace_id( td->replace_id(), td->spell_id() );
          break;
        }
      }
    }
  }
}

const spell_data_t* player_talent_points_t::find_spell( std::string_view n, specialization_e s, bool name_tokenized,
                                                        bool check_validity ) const
{
  if ( s == SPEC_NONE )
  {
    s = player->specialization();
  }

  // Get a talent's spell id for a given talent name
  unsigned spell_id = player->dbc->talent_ability_id( player->type, s, n, name_tokenized );

  if ( !spell_id )
  {
    player->sim->print_debug( "Player {}: Can't find talent with name '{}'.", player->name(), n );
    return spell_data_t::not_found();
  }

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      auto td = talent_data_t::find( player->type, j, i, s, player->is_ptr() );
      if ( !td )
        continue;
      auto spell = dbc::find_spell( player, td->spell_id() );

      // Loop through all our classes talents, and check if their spell's id match the one we maped to the given
      // talent name
      if ( td && ( td->spell_id() == spell_id ) )
      {
        // check if we have the talent enabled or not
        // Level tiers are hardcoded here which means they will need to changed when levels change
        if ( check_validity &&
          ( !validate( spell, j, i ) || player->true_level < 20 + ( j == 0 ? -1 : j ) * 5 ) )
          return spell_data_t::not_found();

        return spell;
      }
    }
  }

  /* Talent not enabled */
  return spell_data_t::not_found();
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "item_import.hpp"

#include "item_data.hpp"

const dbc_item_data_t& dbc_item_data_t::find( unsigned id, bool ptr )
{
  const auto _data = data( ptr );
  auto chunk = range::find_if( _data, [id]( util::span<const dbc_item_data_t> chunk ) {
      return id >= chunk.front().id && id <= chunk.back().id;
    } );
  if ( chunk == _data.end() )
    return nil();

  auto it = range::lower_bound( *chunk, id, {}, &dbc_item_data_t::id );
  if ( it != chunk->end() && it->id == id )
    return *it;

  return nil();
}

util::span<const util::span<const dbc_item_data_t>> dbc_item_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( dbc::items(), dbc::items_ptr(), ptr );
}


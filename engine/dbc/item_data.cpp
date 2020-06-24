// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "item_import.hpp"

#include "item_data.hpp"

util::span<const dbc_item_data_t> dbc_item_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( dbc::items(), dbc::items_ptr(), ptr );
}


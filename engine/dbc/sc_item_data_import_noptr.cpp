// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

/**
 * Separated to reduce compile memory onsumption.
 */

#include "dbc.hpp"
#include "generated/sc_item_data.inc"

item_data_t* dbc::__items_noptr()
{
  item_data_t* p = __item_data;
  return p;
}

size_t dbc::n_items_noptr()
{
  return ITEM_SIZE;
}

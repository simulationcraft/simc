// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

/**
 * Separated to reduce compile memory onsumption.
 */

#include "dbc.hpp"
#if SC_USE_PTR
#include "generated/sc_item_data_ptr.inc"

item_data_t* dbc::__items_ptr()
{
  item_data_t* p = __ptr_item_data;
  return p;
}

size_t dbc::n_items_ptr()
{
  return PTR_ITEM_SIZE;
}

#endif

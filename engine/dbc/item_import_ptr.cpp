// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "item_import_ptr.hpp"
#if SC_USE_PTR == 1
#include "generated/item_data_ptr.inc"
#endif /* SC_USE_PTR */

util::span<const item_data_t> dbc::items_ptr()
{
#if SC_USE_PTR == 1
  return __ptr_item_data;
#else
  return {};
#endif /* SC_USE_PTR */
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "item_import.hpp"

#include "generated/item_data.inc"

util::span<const util::span<const dbc_item_data_t>> dbc::items()
{
  return __item_data;
}

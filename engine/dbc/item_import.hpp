// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_IMPORT_HPP
#define ITEM_IMPORT_HPP

#include "util/span.hpp"

#include "dbc/item_data.hpp"

namespace dbc
{
  util::span<const item_data_t> items();
}

#endif /* ITEM_IMPORT_HPP */



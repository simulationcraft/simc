// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "util/cache.hpp"

struct item_t;
namespace wowhead
{
enum wowhead_e
{
  LIVE,
  PTR,
  BETA
};

bool download_item( item_t&, wowhead_e source, cache::behavior_e cache_behavior );
bool download_item_data( item_t& item, wowhead_e source, cache::behavior_e cache_behavior );
}  // namespace wowhead
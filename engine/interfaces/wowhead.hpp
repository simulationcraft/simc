// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "util/cache.hpp"
#include <string>

struct item_t;

namespace wowhead
{
// 2016-07-20: Wowhead's XML output for item stats produces weird results on certain items that are
// no longer available in game. Skip very high values to let the sim run, but not use completely
// silly values.
enum
{
  WOWHEAD_STAT_MAX = 10000
};

enum wowhead_e
{
  LIVE,
  PTR,
  BETA
};

bool download_item( item_t&, wowhead_e source = LIVE, cache::behavior_e cache_behavior = cache::items() );
bool download_item_data( item_t& item, cache::behavior_e cache_behavior, wowhead_e source );

std::string domain_str( wowhead_e domain );
}
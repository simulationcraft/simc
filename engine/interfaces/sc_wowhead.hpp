// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <string>
#include "../util/cache.hpp"

struct item_t;
struct player_t;
struct action_t;
struct buff_t;

// Wowhead  =================================================================

namespace wowhead
{
enum wowhead_e
{
  LIVE,
  PTR,
  BETA
};

bool download_item( item_t&, wowhead_e source = LIVE, cache::behavior_e b = cache::items() );
bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id,
                     wowhead_e source = LIVE, cache::behavior_e b = cache::items() );
bool download_item_data( item_t&            item,
                         cache::behavior_e  caching,
                         wowhead_e          source );

std::string domain_str( wowhead_e domain );
std::string decorated_spell_name( const std::string& name,
                                  unsigned spell_id,
                                  const std::string& spell_name,
                                  wowhead_e domain,
                                  const std::string& href_parm = std::string(),
                                  bool affix = true );

std::string decorated_action_name( const std::string& name,
                                  action_t* action,
                                  wowhead_e domain,
                                  const std::string& href_parm = std::string(),
                                  bool affix = true );
std::string decorated_buff_name( const std::string& name,
                                 buff_t* buff,
                                 wowhead_e domain,
                                 const std::string& href_parm = std::string(),
                                 bool affix = true );
}

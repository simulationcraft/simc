// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "util/cache.hpp"
#include <string>
#include <vector>

struct item_t;
struct player_t;
struct sim_t;

// Blizzard Community Platform API
namespace bcp_api
{
bool download_guild( sim_t* sim, const std::string& region, const std::string& server, const std::string& name,
                     const std::vector<int>& ranks, int player_type_filter = PLAYER_NONE, int max_rank = 0,
                     cache::behavior_e cache_behavior = cache::players() );

player_t* download_player( sim_t*, const std::string& region, const std::string& server, const std::string& name,
                           const std::string& talents       = std::string( "active" ),
                           cache::behavior_e cache_behavior = cache::players(), bool allow_failures = false );

player_t* from_local_json( sim_t*, const std::string&, const std::string&, const std::string& );

bool download_item( item_t&, cache::behavior_e cache_behavior = cache::items() );
void token_load();
void token_save();

slot_e translate_api_slot( const std::string& slot_str );
bool validate_api_key( const std::string& key );
}

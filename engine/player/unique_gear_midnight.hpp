// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <string_view>
#include <vector>

struct sim_t;
struct special_effect_t;
struct spell_data_t;
struct action_t;
struct player_t;

namespace unique_gear::midnight
{
extern std::vector<unsigned> __mid_special_effect_ids;

void register_special_effects();
void register_target_data_initializers( sim_t& );
void register_actor_initializers( sim_t& );
void register_hotfixes();
action_t* create_action( player_t*, std::string_view, std::string_view );
double bandolier_mul( player_t* p );
}  // namespace unique_gear::midnight

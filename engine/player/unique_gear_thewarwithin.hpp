// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <vector>

struct sim_t;
struct special_effect_t;

namespace unique_gear::thewarwithin
{
extern std::vector<unsigned> __tww_special_effect_ids;
extern std::vector<special_effect_t*> __tww_on_use_effects;

void register_special_effects();
void register_target_data_initializers( sim_t& sim );
void register_hotfixes();
}  // namespace unique_gear::thewarwithin

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

struct sim_t;
struct special_effect_t;

namespace unique_gear::thewarwithin
{
void register_special_effects();
void register_target_data_initializers( sim_t& sim );
void register_hotfixes();
}  // namespace unique_gear::thewarwithin

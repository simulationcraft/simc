// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

struct sim_t;

namespace unique_gear
{
namespace shadowlands
{
void register_hotfixes();
void register_special_effects();
void register_target_data_initializers( sim_t& );

}  // namespace shadowlands
}  // namespace unique_gear
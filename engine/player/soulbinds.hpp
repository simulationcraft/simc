// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SOULBINDS_HPP
#define SOULBINDS_HPP

struct player_t;
struct sim_t;

namespace covenant
{
namespace soulbinds
{
/// Initialize soulbinds through the generic special effect subsystem
void initialize_soulbinds( player_t* player );

/// Register soulbinds through the generic special effect subsystem
void register_special_effects();

/// Register soulbinds target data initializers
void register_target_data_initializers( sim_t* sim );
}  // namespace soulbinds
}  // namespace covenant
#endif  // SOULBINDS_HPP
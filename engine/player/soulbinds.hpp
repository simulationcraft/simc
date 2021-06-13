// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SOULBINDS_HPP
#define SOULBINDS_HPP

#include "util/string_view.hpp"

struct player_t;
struct action_t;
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

/// Create soulbind actions
action_t* create_action( player_t* player, util::string_view name, const std::string& options );
}  // namespace soulbinds
}  // namespace covenant
#endif  // SOULBINDS_HPP

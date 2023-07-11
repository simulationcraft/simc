// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SOULBINDS_HPP
#define SOULBINDS_HPP

#include "targetdata_initializer.hpp"

#include <string>

struct action_t;
struct player_t;
struct sim_t;

namespace covenant::soulbinds
{
struct soulbind_targetdata_initializer_t : public targetdata_initializer_t<const spell_data_t>
{
  std::string name;

  soulbind_targetdata_initializer_t( std::string_view n, unsigned id );

  // return soulbind spell based on name
  const spell_data_t* find( player_t* ) const override;

  // return cached soulbind spell
  const spell_data_t* soulbind( actor_target_data_t* ) const;
};

/// Initialize soulbinds through the generic special effect subsystem
void initialize_soulbinds( player_t* player );

/// Register soulbinds through the generic special effect subsystem
void register_special_effects();

/// Register soulbinds target data initializers
void register_target_data_initializers( sim_t* sim );

/// Create soulbind actions
action_t* create_action( player_t* player, std::string_view name, std::string_view options );
}  // namespace covenant::soulbinds
#endif  // SOULBINDS_HPP

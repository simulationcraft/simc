// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef CHARACTER_LOADOUT_HPP
#define CHARACTER_LOADOUT_HPP

#include "client_data.hpp"
#include "sc_enums.hpp"
#include "specialization.hpp"
#include "util/span.hpp"

struct character_loadout_data_t
{
  unsigned id;
  unsigned class_idx;
  unsigned spec_idx;
  unsigned id_item;

  static util::span<const character_loadout_data_t> data( bool ptr );
  static util::span<const character_loadout_data_t> data( unsigned class_idx, bool ptr );
  static util::span<const character_loadout_data_t> data( unsigned class_idx, unsigned spec_idx, bool ptr );
  static int default_item_level();
};

struct trait_loadout_data_t
{
  unsigned id_spec;
  unsigned id_trait_node_entry;
  unsigned rank;
  unsigned order;

  static util::span<const trait_loadout_data_t> data( bool ptr );
  static util::span<const trait_loadout_data_t> data( specialization_e spec, bool ptr );
};

#endif /* CHARACTER_LOADOUT_HPP */

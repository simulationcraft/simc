// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "item/special_effect.hpp"
#include "util/string_view.hpp"

#include <memory>

struct action_t;
struct player_t;
struct item_t;
struct dbc_item_data_t;

namespace consumable
{
struct consumable_effect_t : public special_effect_t
{
  std::unique_ptr<item_t> consumable_item;  // Dragonflight consumables with item associated with each quality

  consumable_effect_t( player_t* player, const dbc_item_data_t* item_data = nullptr );
};

action_t* create_action( player_t*, util::string_view name, util::string_view options );
}  // namespace consumable

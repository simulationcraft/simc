// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_thewarwithin.hpp"

#include "action/absorb.hpp"
#include "action/dot.hpp"
#include "actor_target_data.hpp"
#include "buff/buff.hpp"
#include "darkmoon_deck.hpp"
#include "dbc/data_enums.hh"
#include "dbc/item_database.hpp"
#include "dbc/spell_data.hpp"
#include "ground_aoe.hpp"
#include "item/item.hpp"
#include "set_bonus.hpp"
#include "sim/cooldown.hpp"
#include "sim/real_ppm.hpp"
#include "sim/sim.hpp"
#include "stats.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"
#include "util/string_view.hpp"

namespace unique_gear::thewarwithin
{
namespace consumables
{
}  // namespace consumables

namespace enchants
{
}  // namespace enchants

namespace embellishments
{
}  // namespace embellishments

namespace items
{
// Trinkets

// Weapons

// Armor
}  // namespace items

namespace sets
{
}  // namespace sets

void register_special_effects()
{
  // Food
  // Phials
  // Potions

  // Enchants

  // Embellishments

  // Trinkets
  // Weapons
  // Armor

  // Sets
}

void register_target_data_initializers( sim_t& sim )
{

}

void register_hotfixes()
{

}
}  // namespace unique_gear::thewarwithin

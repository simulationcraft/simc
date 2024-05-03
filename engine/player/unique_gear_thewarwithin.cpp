// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_thewarwithin.hpp"

#include "action/absorb.hpp"
#include "action/action.hpp"
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
std::vector<unsigned> __tww_special_effect_ids;
std::vector<special_effect_t*> __tww_on_use_effects;

// can be called via unqualified lookup
void register_special_effect( unsigned spell_id, custom_cb_t init_callback, bool fallback = false )
{
  unique_gear::register_special_effect( spell_id, init_callback, fallback );
  __tww_special_effect_ids.push_back( spell_id );
}

void register_special_effect( std::initializer_list<unsigned> spell_ids, custom_cb_t init_callback, bool fallback )
{
  for ( auto id : spell_ids )
    register_special_effect( id, init_callback, fallback );
}

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

  // Disabled (use unique_gear::register_special_effect as we don't need to register these as TWW)
}

void register_target_data_initializers( sim_t& sim )
{

}

void register_hotfixes()
{

}
}  // namespace unique_gear::thewarwithin

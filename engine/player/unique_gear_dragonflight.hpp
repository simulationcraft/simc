// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "dbc/data_enums.hh"
#include "dbc/spell_data.hpp"
#include "util/string_view.hpp"

struct sim_t;
struct special_effect_t;

namespace unique_gear::dragonflight
{
namespace consumables
{
void phial_of_elemental_chaos( special_effect_t& );
void phial_of_glacial_fury( special_effect_t& );
void bottled_putrescence( special_effect_t& );
void chilled_clarity( special_effect_t& );
void shocking_disclosure( special_effect_t& );
}

namespace enchants
{
}

namespace items
{
// Trinkets
// Weapons
// Armor
}

void register_special_effects();
void register_target_data_initializers( sim_t& );
}
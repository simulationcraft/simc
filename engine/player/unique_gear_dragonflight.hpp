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
void iced_phial_of_corrupting_rage( special_effect_t& );
void phial_of_charged_isolation( special_effect_t& );
void phial_of_elemental_chaos( special_effect_t& );
void phial_of_glacial_fury( special_effect_t& );
void phial_of_static_empowerment( special_effect_t& );
void bottled_putrescence( special_effect_t& );
void chilled_clarity( special_effect_t& );
void shocking_disclosure( special_effect_t& );
}

namespace enchants
{
std::function<void( special_effect_t& )> writ_enchant( stat_e, bool cr = true );
void earthen_devotion( special_effect_t& );
void frozen_devotion( special_effect_t& );
void titanic_devotion( special_effect_t& );
void wafting_devotion( special_effect_t& );
}

namespace items
{
// Trinkets
void emerald_coachs_whistle( special_effect_t& );
void darkmoon_deck_inferno( special_effect_t& );
void the_cartographers_calipers( special_effect_t& );
// Weapons
// Armor
}

void register_special_effects();
void register_target_data_initializers( sim_t& );
}
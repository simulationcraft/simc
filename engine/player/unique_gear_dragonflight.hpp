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
void elemental_power( special_effect_t& );
void shocking_disclosure( special_effect_t& );
}

namespace enchants
{
std::function<void( special_effect_t& )> writ_enchant( stat_e stat = STAT_NONE, bool cr = true );
void frozen_devotion( special_effect_t& );
void wafting_devotion( special_effect_t& );
}

namespace items
{
// Trinkets
std::function<void( special_effect_t& )> idol_of_the_aspects( std::string_view );
void darkmoon_deck_dance( special_effect_t& );
void darkmoon_deck_inferno( special_effect_t& );
void darkmoon_deck_rime( special_effect_t& );
void darkmoon_deck_watcher( special_effect_t& );
void conjured_chillglobe( special_effect_t& );
void emerald_coachs_whistle( special_effect_t& );
void spiteful_storm( special_effect_t& );
void whispering_incarnate_icon( special_effect_t& );
void the_cartographers_calipers( special_effect_t& );

// Weapons
void bronzed_grip_wrappings( special_effect_t& );
void fang_adornments( special_effect_t& );

// Armor
void blue_silken_lining( special_effect_t& );
void breath_of_neltharion( special_effect_t& );
void coated_in_slime( special_effect_t& );
void elemental_lariat( special_effect_t& );
void flaring_cowl( special_effect_t& );
void thriving_thorns( special_effect_t& );
}

namespace sets
{
void playful_spirits_fur( special_effect_t& );
}

void register_special_effects();
void register_target_data_initializers( sim_t& );
double toxified_mul( player_t* );
double inhibitor_mul( player_t* );
}

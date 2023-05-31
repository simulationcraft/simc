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
std::function<void( special_effect_t& )> writ_enchant( stat_e stat = STAT_NONE );
void frozen_devotion( special_effect_t& );
void wafting_devotion( special_effect_t& );
void shadowflame_wreathe( special_effect_t& );
}

namespace items
{
// Trinkets
std::function<void( special_effect_t& )> idol_of_the_aspects( std::string_view );
void darkmoon_deck_dance( special_effect_t& );
void darkmoon_deck_inferno( special_effect_t& );
void darkmoon_deck_rime( special_effect_t& );
void darkmoon_deck_watcher( special_effect_t& );
void alacritous_alchemist_stone( special_effect_t& );
void bottle_of_spiraling_winds( special_effect_t& );
void conjured_chillglobe( special_effect_t& );
void irideus_fragment( special_effect_t& );
void emerald_coachs_whistle( special_effect_t& );
void erupting_spear_fragment( special_effect_t& );
void furious_ragefeather( special_effect_t& );
void globe_of_jagged_ice( special_effect_t& );
void idol_of_pure_decay( special_effect_t& );
void shikaari_huntress_arrowhead( special_effect_t& );
void spiteful_storm( special_effect_t& );
void spoils_of_neltharus( special_effect_t& );
void sustaining_alchemist_stone( special_effect_t& );
void timebreaching_talon( special_effect_t& );
void umbrelskuls_fractured_heart( special_effect_t& );
void way_of_controlled_currents( special_effect_t& );
void whispering_incarnate_icon( special_effect_t& );
void the_cartographers_calipers( special_effect_t& );
void tome_of_unstable_power( special_effect_t& );
void blazebinders_hoof( special_effect_t& );
void primal_ritual_shell( special_effect_t& );
void seasoned_hunters_trophy( special_effect_t& );
void desperate_invokers_codex( special_effect_t& );
void iceblood_deathsnare( special_effect_t& );
void fractured_crystalspine_quill( special_effect_t& );

// Weapons
void bronzed_grip_wrappings( special_effect_t& );
void fang_adornments( special_effect_t& );

// Armor
void assembly_guardians_ring( special_effect_t& );
void assembly_scholars_loop( special_effect_t& );
void blue_silken_lining( special_effect_t& );
void breath_of_neltharion( special_effect_t& );
void coated_in_slime( special_effect_t& );
void elemental_lariat( special_effect_t& );
void flaring_cowl( special_effect_t& );
void thriving_thorns( special_effect_t& );
void ever_decaying_spores( special_effect_t& );
}

namespace sets
{
void playful_spirits_fur( special_effect_t& );
void raging_tempests( special_effect_t& );
}

void register_special_effects();
void register_target_data_initializers( sim_t& );
void register_hotfixes();
double toxified_mul( player_t* );
double inhibitor_mul( player_t* );
}

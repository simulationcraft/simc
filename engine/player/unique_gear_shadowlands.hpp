// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "dbc/data_enums.hh"
#include "dbc/spell_data.hpp"
#include "report/reports.hpp"
#include "sim/expressions.hpp"
#include "util/string_view.hpp"

struct sim_t;
struct special_effect_t;

namespace unique_gear::shadowlands
{
namespace consumables
{
void smothered_shank( special_effect_t& );
void surprisingly_palatable_feast( special_effect_t& );
void feast_of_gluttonous_hedonism( special_effect_t& );
void potion_of_deathly_fixation( special_effect_t& );
void potion_of_empowered_exorcisms( special_effect_t& );
void potion_of_phantom_fire( special_effect_t& );
}  // namespace consumables

namespace enchants
{
void celestial_guidance( special_effect_t& );
void lightless_force( special_effect_t& );
void sinful_revelation( special_effect_t& );
}  // namespace enchants

namespace items
{
// Trinkets
void darkmoon_deck_shuffle( special_effect_t& );
void darkmoon_deck_putrescence( special_effect_t& );
void darkmoon_deck_voracity( special_effect_t& );
void stone_legion_heraldry( special_effect_t& );
void cabalists_hymnal( special_effect_t& );
void dreadfire_vessel( special_effect_t& );
void macabre_sheet_music( special_effect_t& );
void glyph_of_assimilation( special_effect_t& );
void soul_igniter( special_effect_t& );
void skulkers_wing( special_effect_t& );
void memory_of_past_sins( special_effect_t& );
void gluttonous_spike( special_effect_t& );
void hateful_chain( special_effect_t& );
void overflowing_anima_prison( special_effect_t& );
void sunblood_amethyst( special_effect_t& );
void flame_of_battle( special_effect_t& );
// void spare_meat_hook( special_effect_t& );  no further implementation needed as of 2020-12-1
void overwhelming_power_crystal( special_effect_t& );
void tablet_of_despair( special_effect_t& );
void rotbriar_sprout( special_effect_t& );
void murmurs_in_the_dark( special_effect_t& );
void instructors_divine_bell( special_effect_t& );
// 9.2 Trinkets & Weapons
void scars_of_fraternal_strife( special_effect_t& );
void singularity_supreme( special_effect_t& );
void resonant_reservoir( special_effect_t& );
void elegy_of_the_eternals( special_effect_t& );
void grim_eclipse( special_effect_t& );
void pulsating_riftshard( special_effect_t& );
void prismatic_brilliance( special_effect_t& );

// Runecarves
void echo_of_eonar( special_effect_t& );
void judgment_of_the_arbiter( special_effect_t& );
void maw_rattle( special_effect_t& );
void norgannons_sagacity( special_effect_t& );
void sephuzs_proclamation( special_effect_t& );
void third_eye_of_the_jailer( special_effect_t& );
void vitality_sacrifice( special_effect_t& );
// Shards of Domination
namespace shards_of_domination
{
int rune_word_active( const player_t*, const spell_data_t*, spell_label );
report::sc_html_stream& generate_report( const player_t&, report::sc_html_stream& );
std::unique_ptr<expr_t> create_expression( const player_t&, util::string_view );
}  // namespace shards_of_domination
}  // namespace items

void register_hotfixes();
void register_special_effects();
action_t* create_action( player_t* player, util::string_view name, util::string_view options );
void register_target_data_initializers( sim_t& );

}  // namespace unique_gear::shadowlands

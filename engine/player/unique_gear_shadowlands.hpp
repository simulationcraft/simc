// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

struct sim_t;
struct special_effect_t;

namespace unique_gear
{
namespace shadowlands
{
namespace consumables
{
void smothered_shank( special_effect_t& );
void surprisingly_palatable_feast( special_effect_t& );
void feast_of_gluttonous_hedonism( special_effect_t& );
void potion_of_deathly_fixation( special_effect_t& );
void potion_of_empowered_exorcisms( special_effect_t& );
void potion_of_phantom_fire( special_effect_t& );
}

namespace enchants
{
void celestial_guidance( special_effect_t& );
void lightless_force( special_effect_t& );
void sinful_revelation( special_effect_t& );
}

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

// Runecarves
void echo_of_eonar( special_effect_t& );
void judgment_of_the_arbiter( special_effect_t& );
void maw_rattle( special_effect_t& );
void norgannons_sagacity( special_effect_t& );
void sephuzs_proclamation( special_effect_t& );
void third_eye_of_the_jailer( special_effect_t& );
void vitality_sacrifice( special_effect_t& );
}  // namespace items

void register_hotfixes();
void register_special_effects();
void register_target_data_initializers( sim_t& );

}  // namespace shadowlands
}  // namespace unique_gear

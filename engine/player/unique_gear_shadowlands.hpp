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

void register_hotfixes();
void register_special_effects();
void register_target_data_initializers( sim_t& );

}  // namespace shadowlands
}  // namespace unique_gear

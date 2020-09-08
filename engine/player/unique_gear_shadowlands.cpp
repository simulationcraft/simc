// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_shadowlands.hpp"

#include "sim/sc_sim.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"

namespace unique_gear
{
namespace shadowlands
{
namespace consumables
{
void smothered_shank( special_effect_t& effect )
{

}

void feast_of_gluttonous_hedonism( special_effect_t& effect )
{

}

void potion_of_deadly_fixation( special_effect_t& effect )
{

}

void potion_of_empowered_exorcism( special_effect_t& effect )
{

}

void potion_of_phantom_fire( special_effect_t& effect )
{

}

void embalmers_oil( special_effect_t& effect )
{

}

void shadowcore_oil( special_effect_t& effect )
{

}
}  // namespace consumables

namespace enchants
{
void celestial_guidance( special_effect_t& effect )
{
  if ( !effect.player->buffs.celestial_guidance )
  {
    effect.player->buffs.celestial_guidance =
        make_buff( effect.player, "celestial_guidance", effect.player->find_spell( 324748 ) )
            ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
            ->add_invalidate( CACHE_AGILITY )
            ->add_invalidate( CACHE_INTELLECT )
            ->add_invalidate( CACHE_STRENGTH );
  }

  effect.custom_buff = effect.player->buffs.celestial_guidance;

  new dbc_proc_callback_t( effect.player, effect );
}

void lightless_force( special_effect_t& effect )
{

}

void sinful_revelation( special_effect_t& effect )
{

}
}  // namespace enchants

void register_hotfixes()
{
}

void register_special_effects()
{
    unique_gear::register_special_effect( 324747, enchants::celestial_guidance );
    unique_gear::register_special_effect( 323932, enchants::lightless_force );
    unique_gear::register_special_effect( 324250, enchants::sinful_revelation );
}

void register_target_data_initializers( sim_t& )
{
}

}  // namespace shadowlands
}  // namespace unique_gear
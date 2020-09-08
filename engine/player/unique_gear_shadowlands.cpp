// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_shadowlands.hpp"

#include "sim/sc_sim.hpp"

#include "actor_target_data.hpp"
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
  effect.trigger_spell_id = 324184;
  effect.execute_action = create_proc_action<proc_spell_t>( "lightless_force", effect );
  effect.execute_action->aoe = -1;
  // TODO: adjust to frontal wave for distance targetting

  new dbc_proc_callback_t( effect.player, effect );
}

void sinful_revelation( special_effect_t& effect )
{
  struct sinful_revelation_cb_t : public dbc_proc_callback_t
  {
    sinful_revelation_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ) {}

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto td = a->player->get_target_data( s->target );
      td->debuff.sinful_revelation->trigger();
    }
  };

  new sinful_revelation_cb_t( effect );
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

void register_target_data_initializers( sim_t& sim )
{
  // Sinful Revelation
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 324250 ) )
    {
      assert( !td->debuff.sinful_revelation );

      td->debuff.sinful_revelation = make_buff( *td, "sinful_revelation", td->source->find_spell( 324260 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER );
      td->debuff.sinful_revelation->reset();
    }
    else
      td->debuff.sinful_revelation = make_buff( *td, "sinful_revelation" )->set_quiet( true );
  } );
}

}  // namespace shadowlands
}  // namespace unique_gear
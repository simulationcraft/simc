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
// 444958 equip driver
//  e1: stacking value
//  e2: on-use value
// 444959 on-use driver & buff
// 451199 proc projectile & buff
void spymasters_web( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "spymasters_web", "spymasters_report" } ) )
    return;

  unsigned equip_id = 444958;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Spymaster's Web missing equip effect" );

  auto equip_data = equip->driver();
  auto buff_data = equip->trigger();

  auto stacking_buff = create_buff<stat_buff_t>( effect.player, buff_data )
    ->set_stat_from_effect_type( A_MOD_STAT, equip_data->effectN( 1 ).average( effect.item ) );

  // TODO: confirm there is an animation delay for the spiders to return before buff is applied
  effect.player->callbacks.register_callback_execute_function( equip_id,
      [ stacking_buff,
        p = effect.player,
        vel = buff_data->missile_speed() ]
      ( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) {
        auto delay = timespan_t::from_seconds( p->get_player_distance( *s->target ) / vel );

        make_event( *p->sim, delay, [ stacking_buff ] { stacking_buff->trigger(); } );
      } );

  new dbc_proc_callback_t( effect.player, *equip );

  auto use_buff = create_buff<stat_buff_t>( effect.player, effect.driver() )
    ->set_stat_from_effect_type( A_MOD_STAT, equip_data->effectN( 2 ).average( effect.item ) )
    ->set_max_stack( stacking_buff->max_stack() )
    ->set_cooldown( 0_ms );

  struct spymasters_web_t : public generic_proc_t
  {
    buff_t* stacking_buff;
    buff_t* use_buff;

    spymasters_web_t( const special_effect_t& e, buff_t* stack, buff_t* use )
      : generic_proc_t( e, "spymasters_web", e.driver() ), stacking_buff( stack ), use_buff( use )
    {}

    // TODO: confirm you cannot use without stacking buff present
    bool ready() override
    {
      return stacking_buff->check();
    }

    void execute() override
    {
      generic_proc_t::execute();

      use_buff->trigger( stacking_buff->check() );
      stacking_buff->expire();
    }
  };

  effect.disable_buff();
  effect.execute_action = create_proc_action<spymasters_web_t>( "spymasters_web", effect, stacking_buff, use_buff );
}

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
  register_special_effect( 444959, items::spymasters_web, true );
  // Weapons
  // Armor

  // Sets

  // Disabled
  register_special_effect( 444958, DISABLED_EFFECT );  // spymaster's web
}

void register_target_data_initializers( sim_t& sim )
{

}

void register_hotfixes()
{

}
}  // namespace unique_gear::thewarwithin

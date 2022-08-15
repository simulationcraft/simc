// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_dragonflight.hpp"

#include "action/dot.hpp"
#include "actor_target_data.hpp"
#include "buff/buff.hpp"
#include "darkmoon_deck.hpp"
#include "ground_aoe.hpp"
#include "item/item.hpp"
#include "sim/sim.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"

namespace unique_gear::dragonflight
{
namespace consumables
{
void phial_of_elemental_chaos( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "elemental_chaos_fire", "elemental_chaos_air", "elemental_chaos_earth",
                                        "elemental_chaos_frost" } ) )
  {
    return;
  }

  auto buff = buff_t::find( effect.player, "phial_of_elemental_chaos" );
  if ( !buff )
  {
    std::vector<buff_t*> buff_list;
    auto amount = effect.driver()->effectN( 1 ).average( effect.item );
    auto duration = effect.driver()->effectN( 1 ).period();

    effect.player->buffs.elemental_chaos_fire = buff_list.emplace_back(
        make_buff<stat_buff_t>( effect.player, "elemental_chaos_fire", effect.player->find_spell( 371348 ) )
            ->add_stat( STAT_CRIT_RATING, amount )
            ->set_default_value_from_effect_type( A_MOD_CRIT_DAMAGE_BONUS )
            ->set_duration( duration ) );
    effect.player->buffs.elemental_chaos_air = buff_list.emplace_back(
        make_buff<stat_buff_t>( effect.player, "elemental_chaos_air", effect.player->find_spell( 371350 ) )
            ->add_stat( STAT_HASTE_RATING, amount )
            ->set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS )
            ->set_duration( duration ) );
    effect.player->buffs.elemental_chaos_earth = buff_list.emplace_back(
        make_buff<stat_buff_t>( effect.player, "elemental_chaos_earth", effect.player->find_spell( 371351 ) )
            ->add_stat( STAT_MASTERY_RATING, amount )
            ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
            ->set_duration( duration ) );
    effect.player->buffs.elemental_chaos_frost = buff_list.emplace_back(
        make_buff<stat_buff_t>( effect.player, "elemental_chaos_frost", effect.player->find_spell( 371353 ) )
            ->add_stat( STAT_VERSATILITY_RATING, amount )
            ->set_default_value_from_effect_type( A_MOD_CRITICAL_HEALING_AMOUNT )
            ->set_duration( duration ) );

    buff = make_buff( effect.player, "phial_of_elemental_chaos", effect.driver() )
      ->set_tick_callback( [ buff_list ]( buff_t* b, int, timespan_t ) {
        buff_list[ b->rng().range( buff_list.size() ) ]->trigger();
      } );
  }

  effect.custom_buff = buff;
}

void bottled_putrescence( special_effect_t& effect )
{
  struct bottled_putrescence_t : public proc_spell_t
  {
    struct bottled_putrescence_tick_t : public proc_spell_t
    {
      bottled_putrescence_tick_t( const special_effect_t& e )
        : proc_spell_t( "bottled_putrescence_tick", e.player, e.player->find_spell( 371993 ) )
      {
        dual = ground_aoe = true;
        aoe = -1;
        // every tick damage is rounded down
        base_dd_min = base_dd_max = util::floor( e.driver()->effectN( 1 ).average( e.item ) );
      }

      result_amount_type amount_type( const action_state_t*, bool ) const override
      {
        return result_amount_type::DMG_OVER_TIME;
      }
    };

    action_t* damage;

    bottled_putrescence_t( const special_effect_t& e )
      : proc_spell_t( "bottled_putrescence", e.player, e.driver() ),
        damage( create_proc_action<bottled_putrescence_tick_t>( "bottled_putresence", e ) )
    {
      damage->stats = stats;
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .target( s->target )
        .pulse_time( damage->base_tick_time )
        .duration( data().effectN( 1 ).trigger()->duration() - 1_ms )  // has 0tick, but no tick at the end
        .action( damage ), true );
    }
  };

  effect.execute_action = create_proc_action<bottled_putrescence_t>( "bottled_putrescence", effect );
}

void chilled_clarity( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "potion_of_chilled_clarity" } ) )
    return;

  auto buff = buff_t::find( effect.player, "potion_of_chilled_clarity" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "potion_of_chilled_clarity", effect.trigger() )
      ->set_default_value_from_effect_type( A_355 )
      ->set_duration( timespan_t::from_seconds( effect.driver()->effectN( 1 ).base_value() ) );
  }

  effect.custom_buff = effect.player->buffs.chilled_clarity = buff;
}

void shocking_disclosure( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "shocking_disclosure" );
  if ( !buff )
  {
    auto damage = create_proc_action<generic_aoe_proc_t>( "shocking_disclosure", effect, "shocking_disclosure",
                                                          effect.trigger() );
    damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
    damage->dual = true;

    if ( auto pot_action = effect.player->find_action( "potion" ) )
      damage->stats = pot_action->stats;

    buff = make_buff( effect.player, "shocking_disclosure", effect.driver() )
      ->set_tick_callback( [ damage ]( buff_t* b, int, timespan_t ) {
        damage->execute_on_target( b->player->target );
      } );
  }

  effect.custom_buff = buff;
}
}  // namespace consumables

namespace enchants
{
}

namespace items
{
// Trinkets
// Weapons
// Armor
}

void register_special_effects()
{
  // Food
  // Phials
  register_special_effect( 371339, consumables::phial_of_elemental_chaos );

  // Potion
  register_special_effect( 372046, consumables::bottled_putrescence );
  register_special_effect( 371149, consumables::chilled_clarity );
  register_special_effect( 371151, consumables::chilled_clarity );
  register_special_effect( 371152, consumables::chilled_clarity );
  register_special_effect( 370816, consumables::shocking_disclosure );

  // Enchants
  // Trinkets
  // Weapons
  // Armor
  // Disabled
}

void register_target_data_initializers( sim_t& sim )
{
}
}  // namespace unique_gear::dragonflight
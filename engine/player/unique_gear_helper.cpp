// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_helper.hpp"

#include "item/item.hpp"

namespace unique_gear
{
unique_gear_pet_t::unique_gear_pet_t( std::string_view name, const special_effect_t& e,
                                      const spell_data_t* summon_spell )
  : pet_t( e.player->sim, e.player, name, true, true ), use_auto_attack( false ), effect( e ), parent_action( nullptr )
{
  if ( summon_spell )
    npc_id = summon_spell->effectN( 1 ).misc_value1();
}

void unique_gear_pet_t::create_buffs()
{
  pet_t::create_buffs();

  buffs.movement->set_quiet( true );
}

void unique_gear_pet_t::arise()
{
  pet_t::arise();

  if ( parent_action )
    parent_action->stats->add_execute( 0_ms, owner );

  if ( use_auto_attack && owner->base.distance > 8 )
  {
    trigger_movement( owner->base.distance, movement_direction_type::TOWARDS );
    auto dur = time_to_move();
    make_event( *sim, dur, [ this, dur ] { update_movement( dur ); } );
  }
}

action_t* unique_gear_pet_t::create_action( std::string_view name, std::string_view options_str )
{
  struct auto_attack_t final : public melee_attack_t
  {
    auto_attack_t( unique_gear_pet_t* p ) : melee_attack_t( "main_hand", p )
    {
      assert( p->main_hand_weapon.type != WEAPON_NONE );
      p->main_hand_attack                    = p->create_auto_attack();
      p->main_hand_attack->weapon            = &( p->main_hand_weapon );
      p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;

      ignore_false_positive = true;
      trigger_gcd           = 0_ms;
      school                = SCHOOL_PHYSICAL;
    }

    void execute() override
    {
      player->main_hand_attack->schedule_execute();
    }

    bool ready() override
    {
      if ( player->is_moving() )
        return false;

      return ( player->main_hand_attack->execute_event == nullptr );
    }
  };

  if ( name == "auto_attack" )
    return new auto_attack_t( this );

  return pet_t::create_action( name, options_str );
}

void unique_gear_pet_t::init_action_list()
{
  action_priority_list_t* def = get_action_priority_list( "default" );
  if ( use_auto_attack )
    def->add_action( "auto_attack" );

  pet_t::init_action_list();
}

void unique_gear_pet_t::add_default_action( std::string_view name )
{
  get_action_priority_list( "default" )->add_action( name );
}

external_special_effect_t::external_special_effect_t( player_t* p, std::string_view name, unsigned item_id,
                                                      unsigned ilevel )
  : special_effect_t( p )
{
  // make a fake
  _item = std::make_unique<item_t>( p, fmt::format( ",id={},ilevel={}", item_id, ilevel ) );
  _item->parse_options();
  _item->initialize_data();
  _item->init();

  auto it = range::find( _item->parsed.data.effects, ITEM_SPELLTRIGGER_ON_EQUIP, &item_effect_t::type );
  if ( it == _item->parsed.data.effects.end() )
  {
    throw sc_invalid_player_argument(
      fmt::format( "Cannot find on-equip effect for external item '{}'.", *_item.get() ) );
  }

  spell_id = it->spell_id;
  name_str = name;
  item = _item.get();
}
}  // namespace unique_gear

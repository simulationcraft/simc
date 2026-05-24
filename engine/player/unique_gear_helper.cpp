// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_helper.hpp"

#include "item/item.hpp"

namespace unique_gear
{
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

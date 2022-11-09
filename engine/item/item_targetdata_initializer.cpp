// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "item_targetdata_initializer.hpp"

#include "item/item.hpp"
#include "player/player.hpp"
#include "player/unique_gear.hpp"

item_targetdata_initializer_t::item_targetdata_initializer_t( unsigned iid, util::span<const slot_e> s )
  : item_id( iid ), spell_id( 0 ), slots_( s.begin(), s.end() )
{}

item_targetdata_initializer_t::item_targetdata_initializer_t( unsigned sid )
  : item_id( 0 ), spell_id( sid )
{}

item_targetdata_initializer_t::~item_targetdata_initializer_t() = default;

// Returns the special effect based on item id and slots to source from. Overridable if more
// esoteric functionality is needed

const special_effect_t* item_targetdata_initializer_t::find_effect( player_t* player ) const
{
  // No need to check items on pets/enemies
  if ( player->is_pet() || player->is_enemy() || player->type == HEALING_ENEMY )
  {
    return nullptr;
  }

  if ( spell_id )
  {
    return unique_gear::find_special_effect( player, spell_id );
  }

  for ( slot_e slot : slots_ )
  {
    if ( player->items[ slot ].parsed.data.id == item_id )
    {
      return player->items[ slot ].parsed.special_effects[ 0 ];
    }
  }

  return nullptr;
}

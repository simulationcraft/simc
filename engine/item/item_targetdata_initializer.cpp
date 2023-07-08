// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "item_targetdata_initializer.hpp"

#include "item/item.hpp"
#include "item/special_effect.hpp"
#include "player/player.hpp"
#include "player/unique_gear.hpp"

item_targetdata_initializer_t::item_targetdata_initializer_t( unsigned iid, util::span<const slot_e> s )
  : effects( false ), debuffs( false ), item_id( iid ), spell_id( 0 ), slots_( s.begin(), s.end() )
{}

item_targetdata_initializer_t::item_targetdata_initializer_t( unsigned sid )
  : effects( false ), debuffs( false ), item_id( 0 ), spell_id( sid )
{}

item_targetdata_initializer_t::~item_targetdata_initializer_t() = default;

// Returns the special effect based on item id and slots to source from. Overridable if more
// esoteric functionality is needed

const special_effect_t* item_targetdata_initializer_t::find_effect( player_t* player ) const
{
  const special_effect_t*& eff = effects[ player ];
  if ( eff )
    return eff;

  // No need to check items on pets/enemies
  if ( player->is_pet() || player->is_enemy() || player->type == HEALING_ENEMY )
  {
    return nullptr;
  }

  if ( spell_id )
  {
    eff = unique_gear::find_special_effect( player, spell_id );
  }
  else
  {
    for ( slot_e slot : slots_ )
    {
      if ( player->items[ slot ].parsed.data.id == item_id )
      {
        eff = player->items[ slot ].parsed.special_effects[ 0 ];
      }
    }
  }

  return eff;
}

bool item_targetdata_initializer_t::init( player_t* player ) const
{
  assert( debuff_fn );

  if ( find_effect( player ) != nullptr )
  {
    debuffs[ player ] = debuff_fn( player );
    return true;
  }

  return false;
}

#include <array>

#include "config.hpp"

#include "item_set_bonus.hpp"

#include "generated/item_set_bonus.inc"
#if SC_USE_PTR == 1
#include "generated/item_set_bonus_ptr.inc"
#endif

bool item_set_bonus_t::has_spec( int spec_id ) const
{
  // Check dbc-driven specs
  if ( spec > 0 )
  {
    if ( spec_id == spec )
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  // Check all specs
  else if ( spec == -1 )
  {
    return true;
  }
  // Give up
  else
  {
    return false;
  }
}

bool item_set_bonus_t::has_spec( specialization_e spec ) const
{
  return has_spec( static_cast<int>( spec ) );
}

bool item_set_bonus_t::has_trait_sub_tree( int trait_sub_tree_id ) const
{
  if ( trait_sub_tree > 0 && trait_sub_tree == trait_sub_tree_id )
    return true;

  if ( trait_sub_tree == -1 )
    return true;

  return false;
}

bool item_set_bonus_t::has_item( unsigned item_id ) const
{
  for ( size_t i = 0; i < SET_BONUS_ITEM_ID_MAX; i++ )
  {
    if ( item_ids[ i ] == item_id )
      return true;

    if ( item_ids[ i ] == 0 )
      break;
  }
  return false;
}

util::span<const item_set_bonus_t> item_set_bonus_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __set_bonus_data, __ptr_set_bonus_data, ptr );
}

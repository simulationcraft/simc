#include <array>

#include "config.hpp"

#include "item_weapon.hpp"

#include "generated/item_weapon.inc"
#if SC_USE_PTR == 1
#include "generated/item_weapon_ptr.inc"
#endif

util::span<const item_damage_one_hand_data_t> item_damage_one_hand_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_damage_one_hand_data, __ptr_item_damage_one_hand_data, ptr );
}

util::span<const item_damage_one_hand_caster_data_t> item_damage_one_hand_caster_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_damage_one_hand_caster_data, __ptr_item_damage_one_hand_caster_data, ptr );
}

util::span<const item_damage_two_hand_data_t> item_damage_two_hand_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_damage_two_hand_data, __ptr_item_damage_two_hand_data, ptr );
}

util::span<const item_damage_two_hand_caster_data_t> item_damage_two_hand_caster_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_damage_two_hand_caster_data, __ptr_item_damage_two_hand_caster_data, ptr );
}


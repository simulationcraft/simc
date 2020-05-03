#include "config.hpp"

#include <algorithm>
#include <array>

#include "item_weapon.hpp"

#include "generated/item_weapon.inc"
#if SC_USE_PTR == 1
#include "generated/item_weapon_ptr.inc"
#endif

arv::array_view<item_damage_one_hand_data_t> item_damage_one_hand_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr
    ? arv::array_view<item_damage_one_hand_data_t>( __ptr_item_damage_one_hand_data )
    : arv::array_view<item_damage_one_hand_data_t>( __item_damage_one_hand_data );
#else
  ( void ) ptr;
  const auto& data = __item_damage_one_hand_data;
#endif

  return data;
}

arv::array_view<item_damage_one_hand_caster_data_t> item_damage_one_hand_caster_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr
    ? arv::array_view<item_damage_one_hand_caster_data_t>( __ptr_item_damage_one_hand_caster_data )
    : arv::array_view<item_damage_one_hand_caster_data_t>( __item_damage_one_hand_caster_data );
#else
  ( void ) ptr;
  const auto& data = __item_damage_one_hand_caster_data;
#endif

  return data;
}

arv::array_view<item_damage_two_hand_data_t> item_damage_two_hand_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr
    ? arv::array_view<item_damage_two_hand_data_t>( __ptr_item_damage_two_hand_data )
    : arv::array_view<item_damage_two_hand_data_t>( __item_damage_two_hand_data );
#else
  ( void ) ptr;
  const auto& data = __item_damage_two_hand_data;
#endif

  return data;
}

arv::array_view<item_damage_two_hand_caster_data_t> item_damage_two_hand_caster_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr
    ? arv::array_view<item_damage_two_hand_caster_data_t>( __ptr_item_damage_two_hand_caster_data )
    : arv::array_view<item_damage_two_hand_caster_data_t>( __item_damage_two_hand_caster_data );
#else
  ( void ) ptr;
  const auto& data = __item_damage_two_hand_caster_data;
#endif

  return data;
}


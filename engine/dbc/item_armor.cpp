#include <array>

#include "config.hpp"

#include "item_armor.hpp"

#include "generated/item_armor.inc"
#if SC_USE_PTR == 1
#include "generated/item_armor_ptr.inc"
#endif

arv::array_view<item_armor_quality_data_t> item_armor_quality_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<item_armor_quality_data_t>( __ptr_item_armor_quality_data )
                        : arv::array_view<item_armor_quality_data_t>( __item_armor_quality_data );
#else
  ( void ) ptr;
  const auto& data = __item_armor_quality_data;
#endif

  return data;
}

arv::array_view<item_armor_shield_data_t> item_armor_shield_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<item_armor_shield_data_t>( __ptr_item_armor_shield_data )
                        : arv::array_view<item_armor_shield_data_t>( __item_armor_shield_data );
#else
  ( void ) ptr;
  const auto& data = __item_armor_shield_data;
#endif

  return data;
}

arv::array_view<item_armor_total_data_t> item_armor_total_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<item_armor_total_data_t>( __ptr_item_armor_total_data )
                        : arv::array_view<item_armor_total_data_t>( __item_armor_total_data );
#else
  ( void ) ptr;
  const auto& data = __item_armor_total_data;
#endif

  return data;
}

arv::array_view<item_armor_location_data_t> item_armor_location_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<item_armor_location_data_t>( __ptr_armor_location_data )
                        : arv::array_view<item_armor_location_data_t>( __armor_location_data );
#else
  ( void ) ptr;
  const auto& data = __armor_location_data;
#endif

  return data;
}


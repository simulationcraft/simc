#include <array>

#include "config.hpp"

#include "item_armor.hpp"

#include "generated/item_armor.inc"
#if SC_USE_PTR == 1
#include "generated/item_armor_ptr.inc"
#endif

util::span<const item_armor_quality_data_t> item_armor_quality_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_armor_quality_data, __ptr_item_armor_quality_data, ptr );
}

util::span<const item_armor_shield_data_t> item_armor_shield_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_armor_shield_data, __ptr_item_armor_shield_data, ptr );
}

util::span<const item_armor_total_data_t> item_armor_total_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_armor_total_data, __ptr_item_armor_total_data, ptr );
}

util::span<const item_armor_location_data_t> item_armor_location_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __armor_location_data, __ptr_armor_location_data, ptr );
}


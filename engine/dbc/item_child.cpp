#include <array>

#include "config.hpp"

#include "item_child.hpp"

util::span<const item_child_equipment_t> item_child_equipment_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_child_equipment_data, __ptr_item_child_equipment_data, ptr );
}

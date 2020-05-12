#include <array>

#include "config.hpp"

#include "item_child.hpp"

#include "generated/item_child.inc"
#if SC_USE_PTR == 1
#include "generated/item_child_ptr.inc"
#endif

util::span<const item_child_equipment_t> item_child_equipment_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_child_equipment_data, __ptr_item_child_equipment_data, ptr );
}




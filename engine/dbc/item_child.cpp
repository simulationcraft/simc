#include <array>

#include "config.hpp"

#include "item_child.hpp"

#include "generated/item_child.inc"
#if SC_USE_PTR == 1
#include "generated/item_child_ptr.inc"
#endif

util::span<const item_child_equipment_t> item_child_equipment_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? util::span<const item_child_equipment_t>( __ptr_item_child_equipment_data )
                        : util::span<const item_child_equipment_t>( __item_child_equipment_data );
#else
  ( void ) ptr;
  const auto& data = __item_child_equipment_data;
#endif

  return data;
}




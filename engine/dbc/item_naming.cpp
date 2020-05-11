#include <array>

#include "config.hpp"

#include "item_naming.hpp"

#include "generated/item_naming.inc"
#if SC_USE_PTR == 1
#include "generated/item_naming_ptr.inc"
#endif

util::span<const item_name_description_t> item_name_description_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_name_description_data, __ptr_item_name_description_data, ptr );
}


#include <algorithm>
#include <array>

#include "config.hpp"

#include "item_effect.hpp"

#include "generated/item_effect.inc"
#if SC_USE_PTR == 1
#include "generated/item_effect_ptr.inc"
#endif

util::span<const item_effect_t> item_effect_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_effect_data, __ptr_item_effect_data, ptr );
}

/* static */ const item_effect_t& item_effect_t::find( unsigned id, bool ptr )
{
  const auto index = SC_DBC_GET_DATA( __item_effect_id_index, __ptr_item_effect_id_index, ptr );
  return dbc::find_indexed( id, data( ptr ), index, &item_effect_t::id );
}

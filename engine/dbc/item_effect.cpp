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
  const auto data = item_effect_t::data( ptr );
  const auto id_index = SC_DBC_GET_DATA( __item_effect_id_index, __ptr_item_effect_id_index, ptr );

  auto it = range::lower_bound( id_index, id, {}, [ data ]( auto index ) { return data[ index ].id; } );
  if ( it != id_index.end() && data[ *it ].id == id )
    return data[ *it ];
  return nil();
}

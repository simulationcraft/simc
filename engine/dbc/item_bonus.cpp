#include <array>

#include "config.hpp"

#include "item_bonus.hpp"

#include "util/generic.hpp"

#include "generated/item_bonus.inc"
#if SC_USE_PTR == 1
#include "generated/item_bonus_ptr.inc"
#endif

util::span<const item_bonus_entry_t> item_bonus_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __item_bonus_data, __ptr_item_bonus_data, ptr );
}

util::span<const item_bonus_entry_t> item_bonus_entry_t::find( unsigned bonus_id, bool ptr )
{
  const auto __data = data( ptr );

  auto r = range::equal_range( __data, bonus_id, {}, &item_bonus_entry_t::bonus_id );
  if ( r.first == __data.end() )
  {
    return {};
  }

  return util::span<const item_bonus_entry_t>( r.first, r.second );
}

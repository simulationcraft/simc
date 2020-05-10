#include <algorithm>
#include <array>

#include "config.hpp"

#include "item_bonus.hpp"

#include "generated/item_bonus.inc"
#if SC_USE_PTR == 1
#include "generated/item_bonus_ptr.inc"
#endif

arv::array_view<item_bonus_entry_t> item_bonus_entry_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto& data = ptr ? __ptr_item_bonus_data :  __item_bonus_data;
#else
  ( void ) ptr;
  const auto& data = __item_bonus_data;
#endif

  return data;
}

arv::array_view<item_bonus_entry_t> item_bonus_entry_t::find( unsigned bonus_id, bool ptr )
{
  const auto __data = data( ptr );

  auto low = std::lower_bound( __data.cbegin(), __data.cend(), bonus_id,
                              []( const item_bonus_entry_t& entry, const unsigned& id ) {
                                return entry.bonus_id < id;
                              } );
  if ( low == __data.end() )
  {
    return {};
  }

  auto high = std::upper_bound( __data.cbegin(), __data.cend(), bonus_id,
                              []( const unsigned& id, const item_bonus_entry_t& entry ) {
                                return id < entry.bonus_id;
                              } );

  return arv::array_view<item_bonus_entry_t>( low, high );
}

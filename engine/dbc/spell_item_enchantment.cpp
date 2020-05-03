#include <algorithm>
#include <array>

#include "spell_item_enchantment.hpp"

#include "generated/spell_item_enchantment.inc"
#if SC_USE_PTR == 1
#include "generated/spell_item_enchantment_ptr.inc"
#endif

const item_enchantment_data_t& item_enchantment_data_t::find( unsigned id, bool ptr )
{
  const auto __data = data( ptr );

  auto it = std::lower_bound( __data.cbegin(), __data.cend(), id,
                              []( const item_enchantment_data_t& b, const unsigned& id ) {
                                return b.id < id;
                              } );

  if ( it != __data.cend() && it->id == id )
  {
    return *it;
  }
  else
  {
    return nil();
  }
}

const item_enchantment_data_t& item_enchantment_data_t::nil()
{
  static item_enchantment_data_t __default {};

  return __default;
}

arv::array_view<item_enchantment_data_t> item_enchantment_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<item_enchantment_data_t>( __ptr_spell_item_ench_data )
                        : arv::array_view<azerite_power_entry_t>( __spell_item_ench_data );
#else
  ( void ) ptr;
  const auto& data = __spell_item_ench_data;
#endif

  return data;
}


#include <array>

#include "config.hpp"

#include "spell_item_enchantment.hpp"

#include "generated/spell_item_enchantment.inc"
#if SC_USE_PTR == 1
#include "generated/spell_item_enchantment_ptr.inc"
#endif

util::span<const item_enchantment_data_t> item_enchantment_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? util::span<const item_enchantment_data_t>( __ptr_spell_item_ench_data )
                        : util::span<const item_enchantment_data_t>( __spell_item_ench_data );
#else
  ( void ) ptr;
  const auto& data = __spell_item_ench_data;
#endif

  return data;
}


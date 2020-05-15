// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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


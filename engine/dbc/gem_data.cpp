// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <array>

#include "config.hpp"

#include "gem_data.hpp"

#include "generated/gem_data.inc"
#if SC_USE_PTR == 1
#include "generated/gem_data_ptr.inc"
#endif

util::span<const gem_property_data_t> gem_property_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __gem_property_data, __ptr_gem_property_data, ptr );
}



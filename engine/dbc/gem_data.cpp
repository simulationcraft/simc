#include <array>

#include "config.hpp"

#include "gem_data.hpp"

#include "generated/gem_data.inc"
#if SC_USE_PTR == 1
#include "generated/gem_data_ptr.inc"
#endif

util::span<const gem_property_data_t> gem_property_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? util::span<const gem_property_data_t>( __ptr_gem_property_data )
                        : util::span<const gem_property_data_t>( __gem_property_data );
#else
  ( void ) ptr;
  const auto& data = __gem_property_data;
#endif

  return data;
}



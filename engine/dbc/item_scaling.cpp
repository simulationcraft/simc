#include <array>

#include "config.hpp"

#include "item_scaling.hpp"

#include "util/generic.hpp"

#include "generated/item_scaling.inc"
#if SC_USE_PTR == 1
#include "generated/item_scaling_ptr.inc"
#endif

util::span<const curve_point_t> curve_point_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __curve_point_data, __ptr_curve_point_data, ptr );
}

util::span<const curve_point_t> curve_point_t::find( unsigned id, bool ptr )
{
  const auto __data = data( ptr );

  auto r = range::equal_range( __data, id, {}, &curve_point_t::curve_id );
  if ( r.first == __data.end() )
  {
    return {};
  }

  return util::span<const curve_point_t>( r.first, r.second );
}


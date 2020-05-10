#include <algorithm>
#include <array>

#include "config.hpp"

#include "rand_prop_points.hpp"

#include "generated/rand_prop_points.inc"
#if SC_USE_PTR == 1
#include "generated/rand_prop_points_ptr.inc"
#endif

arv::array_view<random_prop_data_t> random_prop_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<random_prop_data_t>( __ptr_rand_prop_points_data )
                        : arv::array_view<random_prop_data_t>( __rand_prop_points_data );
#else
  ( void ) ptr;
  const auto& data = __rand_prop_points_data;
#endif

  return data;
}


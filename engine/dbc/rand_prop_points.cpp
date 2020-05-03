#include <algorithm>
#include <array>

#include "rand_prop_points.hpp"

#include "generated/rand_prop_points.inc"
#if SC_USE_PTR == 1
#include "generated/rand_prop_points_ptr.inc"
#endif

const random_prop_data_t& random_prop_data_t::find( unsigned ilevel, bool ptr )
{
  const auto __data = data( ptr );

  auto it = std::lower_bound( __data.cbegin(), __data.cend(), ilevel,
                              []( const random_prop_data_t& b, const unsigned& ilevel ) {
                                return b.ilevel < ilevel;
                              } );

  if ( it != __data.cend() && it->ilevel == ilevel )
  {
    return *it;
  }
  else
  {
    return nil();
  }
}

const random_prop_data_t& random_prop_data_t::nil()
{
  static random_prop_data_t __default {};

  return __default;
}

arv::array_view<random_prop_data_t> random_prop_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<random_prop_data_t>( __ptr_rand_prop_points_data )
                        : arv::array_view<azerite_power_entry_t>( __rand_prop_points_data );
#else
  ( void ) ptr;
  const auto& data = __rand_prop_points_data;
#endif

  return data;
}


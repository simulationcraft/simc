// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <array>

#include "config.hpp"

#include "rand_prop_points.hpp"

#include "generated/rand_prop_points.inc"
#if SC_USE_PTR == 1
#include "generated/rand_prop_points_ptr.inc"
#endif

util::span<const random_prop_data_t> random_prop_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __rand_prop_points_data, __ptr_rand_prop_points_data, ptr );
}


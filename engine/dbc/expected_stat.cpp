// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "expected_stat.hpp"

#include "config.hpp"

#include <array>

#include "generated/expected_stat.inc"
#if SC_USE_PTR == 1
#include "generated/expected_stat_ptr.inc"
#endif

util::span<const expected_stat_t> expected_stat_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __expected_stat_data, __ptr_expected_stat_data, ptr );
}

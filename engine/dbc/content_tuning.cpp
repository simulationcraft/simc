// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "content_tuning.hpp"

#include "config.hpp"

#include <array>

#include "generated/content_tuning.inc"
#if SC_USE_PTR == 1
#include "generated/content_tuning_ptr.inc"
#endif

util::span<const content_tuning_data_t> content_tuning_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __content_tuning_data, __ptr_content_tuning_data, ptr );
}

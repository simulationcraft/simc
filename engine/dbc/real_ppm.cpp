#include <array>

#include "config.hpp"

#include "real_ppm.hpp"

#include "generated/real_ppm.inc"
#if SC_USE_PTR == 1
#include "generated/real_ppm_ptr.inc"
#endif

util::span<const rppm_modifier_t> rppm_modifier_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __rppm_modifier_data, __ptr_rppm_modifier_data, ptr );
}

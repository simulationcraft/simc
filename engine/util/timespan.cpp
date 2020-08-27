// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "fmt/format.h"
#include "sc_timespan.hpp"

namespace simc
{
void format_to( timespan_t x, fmt::format_context::iterator out )
{
  fmt::format_to( out, "{:.3f}", x.total_seconds() );
}
}  // namespace simc
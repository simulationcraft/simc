// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_timespan.hpp"

#include "fmt/format.h"
#include "fmt/ostream.h"

namespace simc {
std::ostream& operator<<(std::ostream &os, timespan_t x)
{
  fmt::print(os, "{:.3f}", x.total_seconds() );
  return os;
}
}
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Log
// ==========================================================================

// log_t::output ============================================================

void log_t::output( sim_t* sim, const char* format, ... )
{
  va_list vap;
  char buffer[2048];

  va_start( vap, format );
  util_t::fprintf( sim -> output_file, "%-8.2f ", sim -> current_time );
  vsnprintf( buffer, 2047,  format, vap );
  buffer[2047] = '\0';
  util_t::fprintf( sim -> output_file, "%s", buffer );
  util_t::fprintf( sim -> output_file, "\n" );
  fflush( sim -> output_file );
}

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
  vsnprintf( buffer, sizeof( buffer ),  format, vap );
  va_end( vap );

  util_t::fprintf( sim -> output_file, "%-8.2f ", sim -> current_time );
  util_t::fprintf( sim -> output_file, "%s", buffer );
  util_t::fprintf( sim -> output_file, "\n" );
  fflush( sim -> output_file );
}

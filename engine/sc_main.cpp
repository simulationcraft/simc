// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <locale>
#include "simulationcraft.hpp"

// ==========================================================================
// MAIN
// ==========================================================================

int main( int argc, char** argv )
{
  std::locale::global( std::locale( "C" ) );
  sim_t sim;
  return sim.main( std::vector<std::string>( argv + 1, argv + argc ) );
}

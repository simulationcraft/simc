// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_profileset.hpp"

namespace profileset
{
sim_control_t* profile_set_t::create_sim_options( const sim_control_t* original )
{
  if ( original == nullptr )
  {
    return nullptr;
  }

  sim_control_t* new_options = new sim_control_t( *original );

  try
  {
    new_options -> options.parse_args( m_options );
  }
  catch ( const std::exception& e ) {
    std::cerr << "ERROR! Incorrect option format: " << e.what() << std::endl;
    delete new_options;
    return nullptr;
  }

  return new_options;
}
} /* Namespace profileset ends */

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"

namespace { // unnamed namespace

void print_player_data( io::ofstream& s, const player_t& p )
{
  s << "Player " << p.name() << "\n";
  p.collected_data.data_str( s );
}

void print_data( io::ofstream& s, const sim_t& sim )
{
  s << "SimulationCraft " << SC_VERSION << std::endl;
  time_t rawtime; time( &rawtime );
  s << "Timestamp: " << ctime( &rawtime ) << std::endl;

  for ( size_t i = 0, size = sim.player_no_pet_list.size(); i < size; ++i )
  {
    const player_t& p = *sim.player_no_pet_list[ i ];
    print_player_data( s, p );
  }
}

} // end unnamed namespace

void report::print_csv_data( sim_t* sim )
{
  io::ofstream s;
  if ( ! sim -> csv_output_file_str.empty() )
  {
    s.open( sim -> csv_output_file_str );
  }
  if ( ! s )
    return;

  print_data( s, *sim );
}

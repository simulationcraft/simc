// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_profileset.hpp"

namespace profileset
{
sim_control_t* profile_set_t::create_sim_options( const sim_control_t*            original,
                                                  const std::vector<std::string>& opts )
{
  if ( original == nullptr )
  {
    return nullptr;
  }

  sim_control_t* new_options = new sim_control_t( *original );

  // Remove profileset options
  auto it = new_options -> options.begin();
  while ( it != new_options -> options.end() )
  {
    const auto& opt = *it;
    if ( util::str_in_str_ci( opt.name, "profileset." ) )
    {
      it = new_options -> options.erase( it );
    }
    else
    {
      ++it;
    }
  }

  try
  {
    new_options -> options.parse_args( opts );
  }
  catch ( const std::exception& e ) {
    std::cerr << "ERROR! Incorrect option format: " << e.what() << std::endl;
    delete new_options;
    return nullptr;
  }

  return new_options;
}

profile_set_t::profile_set_t( const std::string& name, sim_control_t* opts ) :
  m_name( name ), m_options( opts )
{
}

sim_control_t* profile_set_t::options() const
{
  return m_options;
}

profile_set_t::~profile_set_t()
{
  delete m_options;
}


bool parse_profilesets( sim_t* sim )
{
  if ( sim -> profileset_map.size() == 0 )
  {
    return true;
  }

  auto original_control = sim -> control;

  for ( auto it = sim -> profileset_map.begin(); it != sim -> profileset_map.end(); ++it )
  {
    auto control = profile_set_t::create_sim_options( sim -> control, it -> second );
    if ( control == nullptr )
    {
      return false;
    }

    sim -> control = control;

    // Test that profileset options are OK
    try {
      auto test_sim = new sim_t( sim );
      auto ret = test_sim -> init();
      if ( ! ret )
      {
        sim -> control = original_control;
        delete test_sim;
        return false;
      }

      delete test_sim;
    }
    catch ( const std::exception& e )
    {
      std::cerr <<  "ERROR! Setup failure: " << e.what() << std::endl;
      sim -> control = original_control;
      return false;
    }

    sim -> profilesets.push_back( new profile_set_t( it -> first, control ) );
  }

  sim -> control = original_control;

  return true;
}

void create_options( sim_t* sim )
{
  sim -> add_option( opt_map_list( "profileset.", sim -> profileset_map ) );
}

void iterate_profilesets( sim_t* sim )
{
  sim_control_t* original_opts = sim -> control;

  for ( const auto set : sim -> profilesets )
  {
    sim -> control = set -> options();

    auto profile_sim = new sim_t( sim );

    profile_sim -> set_sim_base_str( set -> name() );
    auto ret = profile_sim -> execute();
    delete profile_sim;

    if ( ret == false )
    {
      break;
    }
  }

  sim -> control = original_opts;
}

} /* Namespace profileset ends */

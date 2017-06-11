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

void profile_set_t::done()
{
  m_options = nullptr;
}

bool profilesets_t::validate( sim_t* ps_sim )
{
  if ( ps_sim -> player_no_pet_list.size() > 1 )
  {
    ps_sim -> errorf( "Profileset simulations must have only one actor" );
    return false;
  }

  return true;
}

bool profilesets_t::parse( sim_t* sim )
{
  if ( sim -> profileset_map.size() == 0 )
  {
    return true;
  }

  if ( ! validate( sim ) )
  {
    return false;
  }

  auto original_control = sim -> control;

  for ( auto it = sim -> profileset_map.begin(); it != sim -> profileset_map.end(); ++it )
  {
    if ( it -> second.size() == 0 )
    {
      continue;
    }

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
      if ( ! ret || ! validate( test_sim ) )
      {
        sim -> control = original_control;
        delete test_sim;
        return false;
      }

      delete test_sim;
    }
    catch ( const std::exception& e )
    {
      std::cerr <<  "ERROR! Profileset '" << it -> first << "' Setup failure: " << e.what() << std::endl;
      sim -> control = original_control;
      return false;
    }

    m_profilesets.push_back( std::unique_ptr<profile_set_t>( new profile_set_t( it -> first, control ) ) );
  }

  sim -> control = original_control;

  return true;
}

bool profilesets_t::iterate( sim_t* parent )
{
  sim_control_t* original_opts = parent -> control;

  for ( auto& set : m_profilesets )
  {
    parent -> control = set -> options();

    auto profile_sim = new sim_t( parent );

    // Reset random seed for the profileset sims
    profile_sim -> seed = 0;
    profile_sim -> profileset_enabled = true;
    profile_sim -> set_sim_base_str( set -> name() );
    auto ret = profile_sim -> execute();

    set -> done();

    if ( ret == false )
    {
      delete profile_sim;
      return false;
    }

    const auto player = profile_sim -> player_no_pet_list.data().front();
    auto metric = player -> scaling_for_metric( SCALE_METRIC_DPS );

    set -> result()
      .metric_type( SCALE_METRIC_DPS )
      .metric( metric.value )
      .stddev( metric.stddev )
      .iterations( player -> collected_data.total_iterations );

    delete profile_sim;
  }

  parent -> control = original_opts;

  return true;
}

void profilesets_t::output( js::JsonOutput& root ) const
{
  root[ "metric" ] = util::scale_metric_type_string(
      m_profilesets.front() -> result().metric_type() );

  auto results = root[ "results" ];
  results.make_array();
  range::for_each( m_profilesets, [ &results ]( const std::unique_ptr<profile_set_t>& profileset ) {
    if ( profileset -> result().metric() == 0 )
    {
      return;
    }

    auto obj = results.add();
    const auto& result = profileset -> result();

    obj[ "name" ] = profileset -> name();
    obj[ "value" ] = result.metric();
    obj[ "stddev" ] = result.stddev();
  } );
}

void create_options( sim_t* sim )
{
  sim -> add_option( opt_map_list( "profileset.", sim -> profileset_map ) );
}

} /* Namespace profileset ends */

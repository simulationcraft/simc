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

  auto new_options = new sim_control_t( *original );

  // Remove profileset options
  auto it = new_options -> options.begin();
  while ( it != new_options -> options.end() )
  {
    const auto& opt = *it;
    if ( util::str_in_str_ci( opt.name, "profileset" ) )
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
  // Deleting the profileset simulator object will free the sim_control_t object, so nullptr ours to
  // avoid a double-free.
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
    auto control = profile_set_t::create_sim_options( original_control, it -> second );
    if ( control == nullptr )
    {
      return false;
    }

    sim -> control = control;

    // Test that profileset options are OK, up to the simulation initialization
    try
    {
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
      std::cerr <<  "ERROR! Profileset '" << it -> first << "' Setup failure: "
                << e.what() << std::endl;
      sim -> control = original_control;
      return false;
    }

    m_profilesets.push_back( std::unique_ptr<profile_set_t>(
        new profile_set_t( it -> first, control ) ) );
  }

  sim -> control = original_control;

  return true;
}

bool profilesets_t::iterate( sim_t* parent )
{
  auto original_opts = parent -> control;

  for ( auto& set : m_profilesets )
  {
    parent -> control = set -> options();

    auto profile_sim = new sim_t( parent );

    // Reset random seed for the profileset sims
    profile_sim -> seed = 0;
    profile_sim -> profileset_enabled = true;
    profile_sim -> report_details = 0;
    profile_sim -> set_sim_base_str( set -> name() );
    auto ret = profile_sim -> execute();

    set -> done();

    if ( ret == false || profile_sim -> is_canceled() )
    {
      delete profile_sim;
      return false;
    }

    const auto player = profile_sim -> player_no_pet_list.data().front();
    auto progress = profile_sim -> progress( nullptr, 0 );
    auto data = metric_data( player );

    set -> result()
      .min( data.min )
      .first_quartile( data.first_quartile )
      .median( data.median )
      .mean( data.mean )
      .third_quartile( data.third_quartile )
      .max( data.max )
      .stddev( data.std_dev )
      .iterations( progress.current_iterations );

    delete profile_sim;
  }

  parent -> control = original_opts;

  return true;
}

int profilesets_t::max_name_length() const
{
  size_t len = 0;

  range::for_each( m_profilesets, [ &len ]( const profileset_entry_t& profileset ) {
    if ( profileset -> name().size() > len )
    {
      len = profileset -> name().size();
    }
  } );

  return len;
}

void profilesets_t::output( const sim_t& sim, js::JsonOutput& root ) const
{
  root[ "metric" ] = util::scale_metric_type_string( sim.profileset_metric );

  auto& results = root[ "results" ].make_array();

  range::for_each( m_profilesets, [ &results ]( const profileset_entry_t& profileset ) {
    if ( profileset -> result().mean() == 0 )
    {
      return;
    }

    auto obj = results.add();
    const auto& result = profileset -> result();

    obj[ "name" ] = profileset -> name();
    obj[ "mean" ] = result.mean();
    obj[ "min" ] = result.min();
    obj[ "max" ] = result.max();
    obj[ "stddev" ] = result.stddev();

    if ( result.median() != 0 )
    {
      obj[ "median" ] = result.median();
      obj[ "first_quartile" ] = result.first_quartile();
      obj[ "third_quartile" ] = result.third_quartile();
    }

    obj[ "iterations" ] = as<uint64_t>( result.iterations() );
  } );
}

void profilesets_t::output( const sim_t& sim, FILE* out ) const
{
  if ( m_profilesets.size() == 0 )
  {
    return;
  }

  util::fprintf( out, "\n\nProfilesets (%s):\n",
    util::scale_metric_type_string( sim.profileset_metric ) );

  range::for_each( m_profilesets, [ out ]( const profileset_entry_t& profileset ) {
    util::fprintf( out, "    %-10.3f : %s\n",
      profileset -> result().mean(), profileset -> name().c_str() );
  } );
}

void create_options( sim_t* sim )
{
  sim -> add_option( opt_map_list( "profileset.", sim -> profileset_map ) );
  sim -> add_option( opt_func( "profileset_metric", []( sim_t*             sim,
                                                        const std::string&,
                                                        const std::string& value ) {
    scale_metric_e metric = util::parse_scale_metric( value );
    if ( metric == SCALE_METRIC_NONE )
    {
      return false;
    }

    sim -> profileset_metric = metric;
    return true;
  } ) );
}

statistical_data_t collect( const extended_sample_data_t& c )
{
  return { c.min(), c.percentile( 0.25 ), c.mean(),
           c.percentile( 0.5 ), c.percentile( 0.75 ), c.max(), c.std_dev };
}

statistical_data_t metric_data( const player_t* player )
{
  const auto& d = player -> collected_data;

  switch ( player -> sim -> profileset_metric )
  {
    case SCALE_METRIC_DPS:       return collect( d.dps );
    case SCALE_METRIC_DPSE:      return collect( d.dpse );
    case SCALE_METRIC_HPS:       return collect( d.hps );
    case SCALE_METRIC_HPSE:      return collect( d.hpse );
    case SCALE_METRIC_APS:       return collect( d.aps );
    case SCALE_METRIC_DPSP:      return collect( d.prioritydps );
    case SCALE_METRIC_DTPS:      return collect( d.dtps );
    case SCALE_METRIC_DMG_TAKEN: return collect( d.dmg_taken );
    case SCALE_METRIC_HTPS:      return collect( d.htps );
    case SCALE_METRIC_TMI:       return collect( d.theck_meloree_index );
    case SCALE_METRIC_ETMI:      return collect( d.effective_theck_meloree_index );
    case SCALE_METRIC_DEATHS:    return collect( d.deaths );
    case SCALE_METRIC_HAPS:
    {
      auto hps = collect( d.hps );
      auto aps = collect( d.aps );
      return {
        hps.min + aps.min,
        hps.first_quartile + aps.first_quartile,
        hps.mean + aps.mean,
        hps.median + aps.median,
        hps.third_quartile + aps.first_quartile,
        hps.max + aps.max,
        sqrt( d.aps.mean_variance + d.hps.mean_variance )
      };
    }
    default:                     return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  }
}

} /* Namespace profileset ends */

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

// is_plot_stat =============================================================

bool is_plot_stat( sim_t* sim,
                          stat_e stat )
{
  if ( ! sim -> plot -> dps_plot_stat_str.empty() )
  {
    std::vector<std::string> stat_list = util::string_split( sim -> plot -> dps_plot_stat_str, ",:;/|" );
    bool found = false;
    for ( size_t i = 0; i < stat_list.size() && ! found; i++ )
    {
      found = ( util::parse_stat_type( stat_list[ i ] ) == stat );
    }
    if ( ! found ) return false;
  }

  for ( size_t i = 0; i < sim -> player_no_pet_list.size(); ++i )
  {
    player_t* p = sim -> player_no_pet_list[ i ];
    if ( p -> quiet ) continue;

    if ( p -> scales_with[ stat ] ) return true;
  }

  return false;
}

} // UNNAMED NAMESPACE ====================================================

// ==========================================================================
// Plot
// ==========================================================================

// plot_t::plot_t ===========================================================

plot_t::plot_t( sim_t* s ) :
  sim( s ),
  dps_plot_step( 160.0 ),
  dps_plot_points( 20 ),
  dps_plot_iterations ( -1 ),
  dps_plot_debug( 0 ),
  current_plot_stat( STAT_NONE ),
  num_plot_stats( 0 ),
  remaining_plot_stats( 0 ),
  remaining_plot_points( 0 ),
  dps_plot_positive( 0 ),
  dps_plot_negative( 0 )
{
  create_options();
}

// plot_t::progress =========================================================

double plot_t::progress( std::string& phase, std::string* detailed )
{
  if ( dps_plot_stat_str.empty() ) return 1.0;

  if ( num_plot_stats <= 0 ) return 1;

  if ( current_plot_stat <= 0 ) return 0;

  phase  = "Plot - ";
  phase += util::stat_type_abbrev( current_plot_stat );

  int completed_plot_stats = ( num_plot_stats - remaining_plot_stats );

  double stat_progress = completed_plot_stats / ( double ) num_plot_stats;

  int completed_plot_points = ( dps_plot_points - remaining_plot_points );

  double point_progress = completed_plot_points / ( double ) dps_plot_points;

  stat_progress += point_progress / num_plot_stats;

  sim -> detailed_progress( detailed, completed_plot_stats + completed_plot_points,
                                      num_plot_stats + dps_plot_points );

  return stat_progress;
}

// plot_t::analyze_stats ====================================================

void plot_t::analyze_stats()
{
  if ( dps_plot_stat_str.empty() ) return;

  size_t num_players = sim -> players_by_name.size();
  if ( num_players == 0 ) return;

  remaining_plot_stats = 0;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    if ( is_plot_stat( sim, i ) )
      remaining_plot_stats++;
  num_plot_stats = remaining_plot_stats;

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( sim -> is_canceled() ) break;

    if ( ! is_plot_stat( sim, i ) ) continue;

    current_plot_stat = i;

    if ( sim -> report_progress )
    {
      util::fprintf( stdout, "\nGenerating DPS Plot for %s...\n", util::stat_type_string( i ) );
      fflush( stdout );
    }

    remaining_plot_points = dps_plot_points;

    int start, end;

    if ( dps_plot_positive )
    {
      start = 0;
      end = dps_plot_points;
    }
    else if ( dps_plot_negative )
    {
      start = -dps_plot_points;
      end = 0;
    }
    else
    {
      start = - dps_plot_points / 2;
      end = - start;
    }

    for ( int j = start; j <= end; j++ )
    {
      if ( sim -> is_canceled() ) break;

      sim_t* delta_sim = 0;

      if ( j != 0 )
      {
        delta_sim = new sim_t( sim );
        if ( dps_plot_iterations > 0 ) delta_sim -> iterations = dps_plot_iterations;
        delta_sim -> enchant.add_stat( i, j * dps_plot_step );
        if ( sim -> report_progress )
        {
          std::stringstream  stat_name; stat_name.width( 12 );
          stat_name << std::left << std::string( util::stat_type_abbrev( i ) ) + ":";
          delta_sim -> sim_phase_str = util::to_string( j * dps_plot_step ) + " " + stat_name.str();
        }
        delta_sim -> execute();
        if ( dps_plot_debug )
        {
          sim -> out_debug.raw().printf( "Stat=%s Point=%d\n", util::stat_type_string( i ), j );
          report::print_text( delta_sim, true );

        }
      }

      for ( size_t k = 0; k < num_players; k++ )
      {
        player_t* p = sim -> players_by_name[ k ];

        if ( ! p -> scales_with[ i ] ) continue;

        plot_data_t data;

        if ( delta_sim )
        {
          player_t* delta_p = delta_sim -> find_player( p -> name() );

          data.value = delta_p -> scales_over().value;
          data.error = delta_p -> scales_over().stddev * delta_sim -> confidence_estimator;
        }
        else
        {
          data.value = p -> scales_over().value;
          data.error = p -> scales_over().stddev * sim -> confidence_estimator;
        }
        data.plot_step = j * dps_plot_step;
        p -> dps_plot_data[ i ].push_back( data );
      }

      if ( delta_sim )
      {
        delete delta_sim;
        delta_sim = 0;
        remaining_plot_points--;
      }
    }

    remaining_plot_stats--;
  }
}

// plot_t::analyze ==========================================================

void plot_t::analyze()
{
  if ( sim -> is_canceled() ) return;

  if ( dps_plot_stat_str.empty() ) return;

  analyze_stats();

  if ( sim -> reforge_plot_output_file_str.empty() )
  {
    sim -> errorf( "No reforge plot output file specified.\n" );
    return;
  }

  io::cfile file( sim -> reforge_plot_output_file_str, "w" );
  if ( ! file )
  {
    sim -> errorf( "Unable to open output file '%s' . \n", sim -> reforge_plot_output_file_str.c_str() );
    return;
  }

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( p -> quiet ) continue;

    util::fprintf( file, "%s Plot Results:\n", p -> name_str.c_str() );

    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( sim -> is_canceled() ) break;

      if ( ! is_plot_stat( sim, j ) ) continue;

      current_plot_stat = j;

      util::fprintf( file, "%s, DPS, DPS-Error\n", util::stat_type_string( j ) );

      for ( size_t k = 0; k < p -> dps_plot_data[ j ].size(); k++ )
      {
        util::fprintf( file, "%f, ",
                       p -> dps_plot_data[ j ][ k ].plot_step );
        util::fprintf( file, "%f, ",
                       p -> dps_plot_data[ j ][ k ].value );
        util::fprintf( file, "%f\n",
                       p -> dps_plot_data[ j ][ k ].error );
      }
      util::fprintf( file, "\n" );
    }
  }
}

// plot_t::create_options ===================================================

void plot_t::create_options()
{
  option_t plot_options[] =
  {
    // @option_doc loc=global/scale_factors title="Plots"
    opt_int( "dps_plot_iterations", dps_plot_iterations ),
    opt_int( "dps_plot_points",     dps_plot_points ),
    opt_string( "dps_plot_stat",    dps_plot_stat_str ),
    opt_float( "dps_plot_step",     dps_plot_step ),
    opt_bool( "dps_plot_debug",     dps_plot_debug ),
    opt_bool( "dps_plot_positive", dps_plot_positive ),
    opt_bool( "dps_plot_negative", dps_plot_negative ),
    opt_null()
  };

  option_t::copy( sim -> options, plot_options );
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// is_plot_stat =============================================================

static bool is_plot_stat( sim_t* sim,
                          int    stat )
{
  if ( ! sim -> plot -> dps_plot_stat_str.empty() )
  {
    std::vector<std::string> stat_list;
    int num_stats = util_t::string_split( stat_list, sim -> plot -> dps_plot_stat_str, ",:;/|" );
    bool found = false;
    for ( int i=0; i < num_stats && ! found; i++ )
    {
      found = ( util_t::parse_stat_type( stat_list[ i ] ) == stat );
    }
    if ( ! found ) return false;
  }

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> quiet ) continue;
    if ( p -> is_pet() ) continue;

    if ( p -> scales_with[ stat ] ) return true;
  }

  return false;
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Plot
// ==========================================================================

// plot_t::plot_t ===========================================================

plot_t::plot_t( sim_t* s ) :
  sim( s ),
  dps_plot_step( 20.0 ),
  dps_plot_points( 20 ),
  dps_plot_iterations ( -1 ),
  dps_plot_debug( 0 ),
  current_plot_stat( 0 ),
  num_plot_stats( 0 ),
  remaining_plot_stats( 0 ),
  remaining_plot_points( 0 ),
  dps_plot_positive( 0 )
{
  create_options();
}

// plot_t::progress =========================================================

double plot_t::progress( std::string& phase )
{
  if ( dps_plot_stat_str.empty() ) return 1.0;

  if ( num_plot_stats <= 0 ) return 1;

  if ( current_plot_stat <= 0 ) return 0;

  phase  = "Plot - ";
  phase += util_t::stat_type_abbrev( current_plot_stat );

  double stat_progress = ( num_plot_stats - remaining_plot_stats ) / ( double ) num_plot_stats;

  double point_progress = ( dps_plot_points - remaining_plot_points ) / ( double ) dps_plot_points;

  stat_progress += point_progress / num_plot_stats;

  return stat_progress;
}

// plot_t::analyze_stats ====================================================

void plot_t::analyze_stats()
{
  if ( dps_plot_stat_str.empty() ) return;

  int num_players = ( int ) sim -> players_by_name.size();
  if ( num_players == 0 ) return;

  remaining_plot_stats = 0;
  for ( int i=0; i < STAT_MAX; i++ )
    if ( is_plot_stat( sim, i ) )
      remaining_plot_stats++;
  num_plot_stats = remaining_plot_stats;

  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( sim -> canceled ) break;

    if ( ! is_plot_stat( sim, i ) ) continue;

    current_plot_stat = i;

    if ( sim -> report_progress )
    {
      util_t::fprintf( stdout, "\nGenerating DPS Plot for %s...\n", util_t::stat_type_string( i ) );
      fflush( stdout );
    }

    remaining_plot_points = dps_plot_points;

    int start, end;

    if ( dps_plot_positive )
    {
      start = 0;
      end = dps_plot_points;
    }
    else
    {
      start = - dps_plot_points / 2;
      end = - start;
    }

    for ( int j = start; j <= end; j++ )
    {
      if ( sim -> canceled ) break;

      sim_t* delta_sim=0;

      if ( j != 0 )
      {
        delta_sim = new sim_t( sim );
        if ( dps_plot_iterations > 0 ) delta_sim -> iterations = dps_plot_iterations;
        delta_sim -> enchant.add_stat( i, j * dps_plot_step );
        delta_sim -> execute();
        if ( dps_plot_debug )
        {
          util_t::fprintf( sim -> output_file, "Stat=%s Point=%d\n", util_t::stat_type_string( i ), j );
          report_t::print_text( sim -> output_file, delta_sim, true );
        }
      }

      for ( int k=0; k < num_players; k++ )
      {
        player_t* p = sim -> players_by_name[ k ];

        if ( p -> scales_with[ i ] <= 0 ) continue;

        if ( delta_sim )
        {
          player_t* delta_p = delta_sim -> find_player( p -> name() );

          if ( delta_p -> primary_role() == ROLE_HEAL )
            p -> dps_plot_data[ i ].push_back( delta_p -> hps.mean );
          else
            p -> dps_plot_data[ i ].push_back( delta_p -> dps.mean );
        }
        else
        {
          if ( p -> primary_role() == ROLE_HEAL )
            p -> dps_plot_data[ i ].push_back( p -> hps.mean );
          else
            p -> dps_plot_data[ i ].push_back( p -> dps.mean );
        }
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
  if ( sim -> canceled ) return;

  analyze_stats();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> quiet ) continue;

    chart_t::scaling_dps( p -> scaling_dps_chart, p );
  }
}

// plot_t::create_options ===================================================

void plot_t::create_options()
{
  option_t plot_options[] =
  {
    // @option_doc loc=global/scale_factors title="Plots"
    { "dps_plot_iterations", OPT_INT,    &( dps_plot_iterations ) },
    { "dps_plot_points",     OPT_INT,    &( dps_plot_points     ) },
    { "dps_plot_stat",       OPT_STRING, &( dps_plot_stat_str   ) },
    { "dps_plot_step",       OPT_FLT,    &( dps_plot_step       ) },
    { "dps_plot_debug",      OPT_BOOL,   &( dps_plot_debug      ) },
    { "dps_plot_positive",   OPT_BOOL,   &( dps_plot_positive   ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( sim -> options, plot_options );
}

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
  if ( ! sim -> reforge_plot -> reforge_plot_stat_str.empty() )
  {
    std::vector<std::string> stat_list;
    int num_stats = util_t::string_split( stat_list, sim -> reforge_plot -> reforge_plot_stat_str, ",:;/|" );
    bool found = false;
    for( int i=0; i < num_stats && ! found; i++ )
    {
      found = ( util_t::parse_stat_type( stat_list[ i ] ) == stat );
    }
    if( ! found ) return false;
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
// Reforge Plot
// ==========================================================================

// reforge_plot_t::reforge_plot_t ===========================================

reforge_plot_t::reforge_plot_t( sim_t* s ) :
  sim( s ),
  reforge_plot_output_file( stdout ),
  reforge_plot_step( 20 ),
  reforge_plot_amount( 200 ),
  reforge_plot_iterations( -1 ),
  reforge_plot_debug( 0 ),
  current_stat_combo( 0 ),
  num_stat_combos( 0 )
{
  create_options();
}

// generate_stat_mods =======================================================

void reforge_plot_t::generate_stat_mods( std::vector<std::vector<int> > &stat_mods,
                                         const std::vector<int> &stat_indices,
                                         int cur_mod_stat,
                                         std::vector<int> cur_stat_mods )
{
  if ( cur_mod_stat >= ( int ) stat_indices.size() - 1 )
  {
    int sum = 0;
    for ( int i = 0; i < ( int ) cur_stat_mods.size(); i++ )
    {
      sum += cur_stat_mods[ i ];
    }

    if ( abs( sum ) > reforge_plot_amount )
      return;

    int negative_stat = 0;
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> quiet )
        continue;
      if ( p -> stats.get_stat( stat_indices[ cur_mod_stat ] ) - sum < 0 )
        negative_stat = 1;
    }
    if ( negative_stat )
      return;

    cur_stat_mods[ cur_mod_stat ] = -1 * sum;
    stat_mods.push_back( cur_stat_mods );
    return;
  }

  for ( int mod_amount = -reforge_plot_amount;
        mod_amount <= reforge_plot_amount;
        mod_amount += reforge_plot_step )
  {
    int negative_stat = 0;
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> quiet )
        continue;
      if ( p -> stats.get_stat( stat_indices[ cur_mod_stat ] ) + mod_amount < 0 )
        negative_stat = 1;
    }
    if ( negative_stat )
      continue;

    cur_stat_mods[ cur_mod_stat ] = mod_amount;
    generate_stat_mods( stat_mods, stat_indices, cur_mod_stat + 1, cur_stat_mods );
  }
}

// reforge_plot_t::analyze_stats ============================================

void reforge_plot_t::analyze_stats()
{
  std::vector<std::vector<int> > stat_mods;

  if ( reforge_plot_stat_str.empty() )
    return;

  int num_players = ( int ) sim -> players_by_name.size();

  for ( int i=0; i < STAT_MAX; i++ )
    if ( is_plot_stat( sim, i ) )
      reforge_plot_stat_indices.push_back( i );

  if ( reforge_plot_stat_indices.empty() )
    return;

  //Create vector of all stat_add combinations recursively
  std::vector<int> cur_stat_mods;
  cur_stat_mods.resize( reforge_plot_stat_indices.size() );
  generate_stat_mods( stat_mods, reforge_plot_stat_indices, 0,
                      cur_stat_mods );

  num_stat_combos = stat_mods.size();

  if ( reforge_plot_debug )
  {
    util_t::fprintf( sim -> output_file, "Reforge Plot Stats: " );
    for ( int i=0; i < ( int ) reforge_plot_stat_indices.size(); i++ )
      util_t::fprintf( sim -> output_file, "%s, ", util_t::stat_type_string( reforge_plot_stat_indices[ i ] ) );
    util_t::fprintf( sim -> output_file, "\n" );

    util_t::fprintf( sim -> output_file, "Reforge Plot Stat Mods:\n" );
    for ( int i=0; i < ( int ) stat_mods.size(); i++ )
    {
      for ( int j=0; j < ( int ) stat_mods[ i ].size(); j++ )
        util_t::fprintf( sim -> output_file, "%s: %d ",
                         util_t::stat_type_string( reforge_plot_stat_indices[ j ] ),
                         stat_mods[ i ][ j ] );
      util_t::fprintf( sim -> output_file, "\n" );
    }
  }

  for ( int i=0; i < ( int ) stat_mods.size(); i++ )
  {
    if ( sim -> canceled ) break;

    sim_t* delta_sim=0;
    std::vector<double> delta_result;
    delta_result.resize( stat_mods[ i ].size() + 1 );

    delta_sim = new sim_t( sim );
    if ( reforge_plot_iterations > 0 )
      delta_sim -> iterations = reforge_plot_iterations;
    for ( int j=0; j < ( int ) stat_mods[ i ].size(); j++ )
    {
      delta_sim -> enchant.add_stat( reforge_plot_stat_indices[ j ],
                                     stat_mods[ i ][ j ] );
      delta_result[ j ] = stat_mods[ i ][ j ];
    }

    current_stat_combo = i;
    delta_sim -> execute();

    for ( int k=0; k < num_players; k++ )
    {
      player_t* p = sim -> players_by_name[ k ];

      if ( delta_sim )
      {
        player_t* delta_p = delta_sim -> find_player( p -> name() );
        delta_result[ stat_mods[ i ].size() ] = delta_p -> dps;
        p -> reforge_plot_data.push_back( delta_result );
      }
    }

    if ( delta_sim )
    {
      delete delta_sim;
      delta_sim = 0;
    }
  }

}

// reforge_plot_t::analyze() ================================================

void reforge_plot_t::analyze()
{
  if ( sim -> canceled ) return;

  if ( reforge_plot_stat_str.empty() ) return;

  analyze_stats();

  if( ! reforge_plot_output_file_str.empty() )
  {
    FILE* f = fopen( reforge_plot_output_file_str.c_str(), "w" );
    if ( f )
      reforge_plot_output_file = f;
    else
    {
      sim -> errorf( "Unable to open output file '%s', Using stdout \n", reforge_plot_output_file_str.c_str() );
    }
  }

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> quiet ) continue;

    util_t::fprintf( reforge_plot_output_file, "%s Reforge Plot Results:\n", p -> name_str.c_str() );

    for ( int i=0; i < ( int ) reforge_plot_stat_indices.size(); i++ )
    {
      util_t::fprintf( reforge_plot_output_file, "%s, ",
                       util_t::stat_type_string( reforge_plot_stat_indices[ i ] ) );
    }
    util_t::fprintf( reforge_plot_output_file, " DPS\n" );

    for ( int i=0; i < ( int ) p -> reforge_plot_data.size(); i++ )
    {
      for ( int j=0; j < ( int ) p -> reforge_plot_data[ i ].size(); j++ )
        util_t::fprintf( reforge_plot_output_file, "%f, ",
                         p -> reforge_plot_data[ i ][ j ] );
      util_t::fprintf( reforge_plot_output_file, "\n" );
    }

    chart_t::reforge_dps( p -> reforge_dps_chart, p );
  }
}

// reforge_plot_t::progress =================================================

double reforge_plot_t::progress( std::string& phase )
{
  if ( reforge_plot_stat_str.empty() ) return 1.0;

  if ( num_stat_combos <= 0 ) return 1.0;

  if ( current_stat_combo <= 0 ) return 0.0;

  phase = "Reforge Plot";
  double combo_progress = current_stat_combo / ( double ) num_stat_combos;

  return combo_progress;
}

// reforge_plot_t::create_options ===========================================

void reforge_plot_t::create_options()
{
  option_t plot_options[] =
  {
    { "reforge_plot_iterations", OPT_INT,    &( reforge_plot_iterations ) },
    { "reforge_plot_step",       OPT_INT,    &( reforge_plot_step       ) },
    { "reforge_plot_amount",     OPT_INT,    &( reforge_plot_amount     ) },
    { "reforge_plot_stat",       OPT_STRING, &( reforge_plot_stat_str   ) },
    { "reforge_plot_output_file",OPT_STRING, &( reforge_plot_output_file_str ) },
    { "reforge_plot_debug",      OPT_BOOL,   &( reforge_plot_debug      ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( sim -> options, plot_options );
}



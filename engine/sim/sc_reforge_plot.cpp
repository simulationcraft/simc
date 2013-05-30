// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

// is_plot_stat =============================================================

static bool is_plot_stat( sim_t* sim,
                          stat_e stat )
{
  if ( ! sim -> reforge_plot -> reforge_plot_stat_str.empty() )
  {
    std::vector<std::string> stat_list = util::string_split( sim -> reforge_plot -> reforge_plot_stat_str, ",:;/|" );
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
// Reforge Plot
// ==========================================================================

// reforge_plot_t::reforge_plot_t ===========================================

reforge_plot_t::reforge_plot_t( sim_t* s ) :
  sim( s ),
  current_reforge_sim( 0 ),
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
                                         const std::vector<stat_e> &stat_indices,
                                         int cur_mod_stat,
                                         std::vector<int> cur_stat_mods )
{
  if ( cur_mod_stat >= ( int ) stat_indices.size() - 1 )
  {
    int sum = 0;
    for ( size_t i = 0; i < cur_stat_mods.size() - 1; i++ )
      sum += cur_stat_mods[ i ];

    if ( abs( sum ) > reforge_plot_amount )
      return;

    for ( size_t i = 0; i < sim -> player_no_pet_list.size(); ++i )
    {
      player_t* p = sim -> player_no_pet_list[ i ];
      if ( p -> quiet )
        continue;
      if ( p -> current.stats.get_stat( stat_indices[ cur_mod_stat ] ) - sum < 0 )
        return;
    }

    cur_stat_mods[ cur_mod_stat ] = -1 * sum;
    stat_mods.push_back( cur_stat_mods );
    return;
  }

  for ( int mod_amount = -reforge_plot_amount;
        mod_amount <= reforge_plot_amount;
        mod_amount += reforge_plot_step )
  {
    bool negative_stat = false;
    for ( size_t i = 0; i < sim -> player_no_pet_list.size(); ++i )
    {
      player_t* p = sim -> player_no_pet_list[ i ];
      if ( p -> quiet )
        continue;
      if ( p -> current.stats.get_stat( stat_indices[ cur_mod_stat ] ) + mod_amount < 0 )
      {
        negative_stat = true;
        break;
      }
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
  if ( reforge_plot_stat_str.empty() )
    return;

  size_t num_players = sim -> players_by_name.size();

  reforge_plot_stat_indices.clear();
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( is_plot_stat( sim, i ) )
      reforge_plot_stat_indices.push_back( i );
  }

  if ( reforge_plot_stat_indices.empty() )
    return;

  // Create vector of all stat_add combinations recursively
  std::vector<int> cur_stat_mods( reforge_plot_stat_indices.size() );

  std::vector<std::vector<int> > stat_mods;
  generate_stat_mods( stat_mods, reforge_plot_stat_indices, 0,
                      cur_stat_mods );

  num_stat_combos = as<int>( stat_mods.size() );

  // Figure out if we have different multipliers for stats
  bool same_multiplier = true;
  for ( size_t i = 1; i < reforge_plot_stat_indices.size(); i++ )
  {
    if ( util::stat_itemization_weight( reforge_plot_stat_indices[ 0 ] ) !=
         util::stat_itemization_weight( reforge_plot_stat_indices[ i ] ) )
    {
      same_multiplier = false;
      break;
    }
  }

  // Weight the modifications by stat value, e.g.,
  // 10 Intellect trades for 20 Crit on gems.
  if ( ! same_multiplier )
  {
    for ( int i = 0; i < num_stat_combos; ++i )
    {
      for ( size_t j = 0; j < stat_mods[ i ].size(); ++j )
        stat_mods[ i ][ j ] = ( int ) ( stat_mods[ i ][ j ] * util::stat_itemization_weight( reforge_plot_stat_indices[ j ] ) );
    }
  }

  if ( reforge_plot_debug )
  {
    util::fprintf( sim -> output_file, "Reforge Plot Stats:" );
    for ( size_t i = 0; i < reforge_plot_stat_indices.size(); i++ )
    {
      util::fprintf( sim -> output_file, "%s%s", i ? ", " : " ",
                     util::stat_type_string( reforge_plot_stat_indices[ i ] ) );
    }
    util::fprintf( sim -> output_file, "\n" );

    util::fprintf( sim -> output_file, "Reforge Plot Stat Mods:\n" );
    for ( size_t i = 0; i < stat_mods.size(); i++ )
    {
      for ( size_t j = 0; j < stat_mods[ i ].size(); j++ )
        util::fprintf( sim -> output_file, "%s: %d ",
                       util::stat_type_string( reforge_plot_stat_indices[ j ] ),
                       stat_mods[ i ][ j ] );
      util::fprintf( sim -> output_file, "\n" );
    }
  }

  for ( size_t i = 0; i < stat_mods.size(); i++ )
  {
    if ( sim -> canceled ) break;

    std::vector<plot_data_t> delta_result( stat_mods[ i ].size() + 1 );

    current_reforge_sim = new sim_t( sim );
    if ( reforge_plot_iterations > 0 )
      current_reforge_sim -> iterations = reforge_plot_iterations;
    
    std::string& tmp = current_reforge_sim -> sim_phase_str;
    for ( size_t j = 0; j < stat_mods[ i ].size(); j++ )
    {
      stat_e stat = reforge_plot_stat_indices[ j ];
      int mod = stat_mods[ i ][ j ];
      current_reforge_sim -> enchant.add_stat( stat, mod );
      delta_result[ j ].value = mod;
      delta_result[ j ].error = 0;

      if ( sim -> report_progress )
      {
        tmp += util::to_string( mod ) + " " + util::stat_type_abbrev( stat );

        if ( j < stat_mods[ i ].size() - 1 )
          tmp += ",";
      }
    }

    if ( sim -> report_progress )
    {
      tmp += ":";
      ssize_t n_spaces = 23 - tmp.length();
      if ( n_spaces > 0 )
        tmp.append( n_spaces, ' ' );
    }

    current_stat_combo = i;
    current_reforge_sim -> execute();

    for ( size_t k = 0; k < num_players; k++ )
    {
      player_t* p = sim -> players_by_name[ k ];

      if ( current_reforge_sim )
      {
        plot_data_t& data = delta_result[ stat_mods[ i ].size() ];
        player_t* delta_p = current_reforge_sim -> find_player( p -> name() );

        data.value = delta_p -> scales_over().mean();
        data.error = delta_p -> scales_over().mean_std_dev * current_reforge_sim -> confidence_estimator;

        p -> reforge_plot_data.push_back( delta_result );
      }
    }

    if ( current_reforge_sim )
    {
      delete current_reforge_sim;
      current_reforge_sim = 0;
    }
  }
}

// reforge_plot_t::analyze() ================================================

void reforge_plot_t::analyze()
{
  if ( sim -> canceled ) return;

  if ( reforge_plot_stat_str.empty() ) return;

  analyze_stats();

  io::cfile file;
  if ( ! sim -> reforge_plot_output_file_str.empty() )
  {
    file = io::cfile( sim -> reforge_plot_output_file_str, "a" );
    if ( ! file )
    {
      sim -> errorf( "Unable to open plot output file '%s'.\n", sim -> reforge_plot_output_file_str.c_str() );
      return;
    }
  }

  if ( ! file ) file = io::cfile( sim -> output_file, io::cfile::no_close() );

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( p -> quiet ) continue;

    util::fprintf( file, "%s Reforge Plot Results:\n", p -> name_str.c_str() );

    for ( int i=0; i < ( int ) reforge_plot_stat_indices.size(); i++ )
    {
      util::fprintf( file, "%s, ",
                     util::stat_type_string( reforge_plot_stat_indices[ i ] ) );
    }
    util::fprintf( file, " DPS, DPS-Error\n" );

    for ( size_t i = 0; i < p -> reforge_plot_data.size(); i++ )
    {
      for ( size_t j = 0; j < p -> reforge_plot_data[ i ].size(); j++ )
      {
        util::fprintf( file, "%f, ",
                       p -> reforge_plot_data[ i ][ j ].value );
        if ( j + 1 == p -> reforge_plot_data[ i ].size() )
          util::fprintf( file, "%f, ",
                         p -> reforge_plot_data[ i ][ j ].error );
      }
      util::fprintf( file, "\n" );
    }
  }
}

// reforge_plot_t::progress =================================================

double reforge_plot_t::progress( std::string& phase )
{
  if ( reforge_plot_stat_str.empty() ) return 1.0;

  if ( num_stat_combos <= 0 ) return 1.0;

  if ( current_stat_combo <= 0 ) return 0.0;

  phase = "Reforge - ";
  for ( size_t i = 0; i < reforge_plot_stat_indices.size(); i++ )
  {
    phase += util::stat_type_abbrev( reforge_plot_stat_indices[ i ] );
    if ( i < reforge_plot_stat_indices.size() - 1 )
      phase += " to ";
  }

  double total_iter = num_stat_combos * ( reforge_plot_iterations > 0 ? reforge_plot_iterations : sim -> iterations );
  double reforge_iter = current_stat_combo * ( reforge_plot_iterations > 0 ? reforge_plot_iterations : sim -> iterations );

  if ( current_reforge_sim && current_reforge_sim -> current_iteration > 0 )
  {
    // Add current reforge iterations only if the update does not land on a partition
    // operation
    if ( ( int ) current_reforge_sim -> children.size() == current_reforge_sim -> threads - 1 )
      reforge_iter += current_reforge_sim -> current_iteration * sim -> threads;
    // At partition, add an extra amount of iterations to keep the iterations correct
    else
      reforge_iter += ( reforge_plot_iterations > 0 ? reforge_plot_iterations : sim -> iterations );
  }

  return reforge_iter / total_iter;
}

// reforge_plot_t::create_options ===========================================

void reforge_plot_t::create_options()
{
  option_t plot_options[] =
  {
    opt_int( "reforge_plot_iterations", reforge_plot_iterations ),
    opt_int( "reforge_plot_step", reforge_plot_step ),
    opt_int( "reforge_plot_amount", reforge_plot_amount ),
    opt_string( "reforge_plot_stat", reforge_plot_stat_str ),
    opt_bool( "reforge_plot_debug", reforge_plot_debug ),
    opt_null()
  };

  option_t::copy( sim -> options, plot_options );
}

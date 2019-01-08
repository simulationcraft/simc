// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE ==========================================

/// Will stat be reforge-plotted?
bool is_plot_stat( sim_t* sim, stat_e stat )
{
  // search for explicit stat option
  if ( !sim->reforge_plot->reforge_plot_stat_str.empty() )
  {
    std::vector<std::string> stat_list =
        util::string_split( sim->reforge_plot->reforge_plot_stat_str, ",:;/|" );

    auto it = range::find_if( stat_list, [stat]( const std::string& s ) {
      return stat == util::parse_stat_type( s );
    } );

    if ( it == stat_list.end() )
    {
      // not found
      return false;
    }
  }

  // also check if any player scales_with that stat
  auto it =
      range::find_if( sim->player_no_pet_list, [stat]( const player_t* p ) {
        return !p->quiet && p->scaling->scales_with[ stat ];
      } );

  return it != sim->player_no_pet_list.end();
}

/**
 * Parse user option string into a 2 dimensional vector[plot#][stat]
 */
std::vector<reforge_plot_run_t> build_reforge_plot_stats(
    const std::string stat_string )
{
  std::vector<reforge_plot_run_t> result;

  // First split string into separate reforge plots
  std::vector<std::string> plot_list = util::string_split( stat_string, "/" );

  // Now split each reforge plot string into its stats and parse them
  for ( const std::string& plot : plot_list )
  {
    reforge_plot_run_t r;
    std::vector<std::string> stat_list = util::string_split( plot, "," );

    for ( const std::string& s : stat_list )
    {
      stat_e stat = util::parse_stat_type( s );
      r.reforge_plot_stat_indices.push_back( stat );
    }
    result.push_back( std::move( r ) );
  }

  return result;
}

}  // UNNAMED NAMESPACE ====================================================

// ==========================================================================
// Reforge Plot
// ==========================================================================

// reforge_plot_t::reforge_plot_t ===========================================

reforge_plot_t::reforge_plot_t( sim_t* s )
  : sim( s ),
    current_reforge_sim( nullptr ),
    reforge_plot_step( 20 ),
    reforge_plot_amount( 200 ),
    reforge_plot_iterations( -1 ),
    reforge_plot_target_error( 0 ),
    reforge_plot_debug( 0 ),
    current_stat_combo( -1 ),
    num_stat_combos( 0 )
{
  create_options();
}

// generate_stat_mods =======================================================

void reforge_plot_t::generate_stat_mods(
    std::vector<std::vector<int>>& stat_mods,
    const std::vector<stat_e>& stat_indices,
    int cur_mod_stat,
    std::vector<int> cur_stat_mods )
{
  if ( cur_mod_stat >= as<int>( stat_indices.size() - 1 ) )
  {
    int sum = 0;
    for ( size_t i = 0; i < cur_stat_mods.size() - 1; i++ )
      sum += cur_stat_mods[ i ];

    if ( abs( sum ) > reforge_plot_amount )
      return;

    for ( const player_t* p : sim->player_no_pet_list )
    {
      if ( p->quiet )
        continue;
      if ( p->initial.stats.get_stat( stat_indices[ cur_mod_stat ] ) - sum < 0 )
        return;
    }

    cur_stat_mods[ cur_mod_stat ] = -1 * sum;
    stat_mods.push_back( cur_stat_mods );
    return;
  }

  for ( int mod_amount = -reforge_plot_amount;
        mod_amount <= reforge_plot_amount; mod_amount += reforge_plot_step )
  {
    bool negative_stat = false;
    for ( const player_t* p : sim->player_no_pet_list )
    {
      if ( p->quiet )
        continue;
      if ( p->initial.stats.get_stat( stat_indices[ cur_mod_stat ] ) +
               mod_amount <
           0 )
      {
        negative_stat = true;
        break;
      }
    }
    if ( negative_stat )
      continue;

    cur_stat_mods[ cur_mod_stat ] = mod_amount;
    generate_stat_mods( stat_mods, stat_indices, cur_mod_stat + 1,
                        cur_stat_mods );
  }
}

void reforge_plot_t::debug_plot(
    const reforge_plot_run_t& plot,
    const std::vector<std::vector<int>>& stat_mods )
{
  if ( !reforge_plot_debug )
  {
    return;
  }
  sim->out_debug.raw() << "Reforge Plot Stats:";
  for ( size_t i = 0; i < plot.reforge_plot_stat_indices.size(); i++ )
  {
    sim->out_log.raw().printf(
        "%s%s", i ? ", " : " ",
        util::stat_type_string( plot.reforge_plot_stat_indices[ i ] ) );
  }
  sim->out_log.raw() << "\n";

  sim->out_log.raw() << "Reforge Plot Stat Mods:\n";
  for ( size_t i = 0; i < stat_mods.size(); i++ )
  {
    for ( size_t j = 0; j < stat_mods[ i ].size(); j++ )
      sim->out_log.raw().printf(
          "%s: %d ",
          util::stat_type_string( plot.reforge_plot_stat_indices[ j ] ),
          stat_mods[ i ][ j ] );
    sim->out_log.raw() << "\n";
  }
}

void reforge_plot_t::run_reforge_plot( const reforge_plot_run_t& plot )
{
  // Create vector of all stat_add combinations recursively
  std::vector<int> cur_stat_mods( plot.reforge_plot_stat_indices.size() );
  std::vector<std::vector<int>> stat_mods;
  generate_stat_mods( stat_mods, plot.reforge_plot_stat_indices, 0,
                      cur_stat_mods );

  num_stat_combos = as<int>( stat_mods.size() );

  debug_plot( plot, stat_mods );

  for ( player_t* player : sim->players_by_name )
  {
    player->reforge_plot_data[&plot];
  }

  for ( size_t i = 0; i < stat_mods.size(); i++ )
  {
    if ( sim->is_canceled() )
      break;

    std::vector<plot_data_t> delta_result( stat_mods[ i ].size() + 1 );

    current_reforge_sim = new sim_t( sim );
    if ( reforge_plot_iterations > 0 )
    {
      current_reforge_sim->work_queue->init( reforge_plot_iterations );
    }

    std::stringstream s;
    for ( size_t j = 0; j < stat_mods[ i ].size(); j++ )
    {
      stat_e stat = plot.reforge_plot_stat_indices[ j ];
      int mod     = stat_mods[ i ][ j ];

      current_reforge_sim -> enchant.add_stat( stat, mod );
      delta_result[ j ].value = mod;
      delta_result[ j ].error = 0;

      s << util::to_string( mod ) << " " << util::stat_type_abbrev( stat );
      if ( j < stat_mods[ i ].size() - 1 )
      {
        s << ", ";
      }
    }

    current_stat_combo = as<int>( i );
    current_reforge_sim -> progress_bar.set_base( s.str() );
    current_reforge_sim -> execute();

    for ( player_t* player : sim->players_by_name )
    {
      if ( current_reforge_sim )
      {
        plot_data_t& data = delta_result[ stat_mods[ i ].size() ];
        player_t* delta_p = current_reforge_sim->find_player( player->name() );

        scaling_metric_data_t scaling_data =
            delta_p->scaling_for_metric( player->sim->scaling->scaling_metric );

        data.value = scaling_data.value;
        data.error =
            scaling_data.stddev * current_reforge_sim->confidence_estimator;

        auto& pd = player->reforge_plot_data[&plot];
        pd.push_back(delta_result);
      }
    }

    delete current_reforge_sim;
    current_reforge_sim = nullptr;
  }
}

void reforge_plot_t::run_plots()
{
  reforge_plots = build_reforge_plot_stats( reforge_plot_stat_str );

  if ( reforge_plots.empty() )
    return;

  for ( const auto& plot : reforge_plots )
  {
    run_reforge_plot( plot );
  }
}

void reforge_plot_t::write_output_file()
{
  if ( sim->reforge_plot_output_file_str.empty() )
  {
    return;
  }

  io::ofstream out;
  out.open( sim->reforge_plot_output_file_str,
            io::ofstream::out | io::ofstream::app );
  if ( !out.is_open() )
  {
    sim->errorf( "Unable to open plot output file '%s'.\n",
                 sim->reforge_plot_output_file_str.c_str() );
    return;
  }
  for ( size_t i = 0; i < reforge_plots.size(); ++i )
  {
    const auto& plot = reforge_plots[ i ];
    for ( player_t* player : sim->player_list )
    {
      if ( player->quiet )
        continue;

      out << player->name() << " Reforge Plot Results:\n";

      for ( stat_e stat_index : plot.reforge_plot_stat_indices )
      {
        out << util::stat_type_string( stat_index ) << ", ";
      }
      out << " DPS, DPS-Error\n";

      for ( const auto& plot_data_list : player->reforge_plot_data[&plot] )
      {
        for ( const auto& plot_data : plot_data_list )
        {
          out << plot_data.value << ", ";
        }
        out << plot_data_list.back().error << ", ";
        out << "\n";
      }
    }
  }
}

void reforge_plot_t::start()
{
  if ( sim->is_canceled() )
    return;

  if ( reforge_plot_stat_str.empty() )
    return;

  run_plots();

  write_output_file();
}

// reforge_plot_t::progress =================================================

double reforge_plot_t::progress( std::string& phase, std::string* detailed )
{
  if ( reforge_plot_stat_str.empty() )
    return 1.0;

  if ( num_stat_combos <= 0 )
    return 1.0;

  if ( current_stat_combo <= 0 )
    return 0.0;

  phase = "Reforge - ";
  for ( auto& plot : reforge_plots )
  {
    for ( size_t i = 0; i < plot.reforge_plot_stat_indices.size(); i++ )
    {
      phase += util::stat_type_abbrev( plot.reforge_plot_stat_indices[ i ] );
      if ( i < plot.reforge_plot_stat_indices.size() - 1 )
        phase += " to ";
    }
  }

  int total_iter =
      num_stat_combos * ( reforge_plot_iterations > 0 ? reforge_plot_iterations
                                                      : sim->iterations );
  int reforge_iter = current_stat_combo * ( reforge_plot_iterations > 0
                                                ? reforge_plot_iterations
                                                : sim->iterations );

  if ( current_reforge_sim && current_reforge_sim->current_iteration > 0 )
  {
    // Add current reforge iterations only if the update does not land on a
    // partition
    // operation
    if ( (int)current_reforge_sim->children.size() ==
         current_reforge_sim->threads - 1 )
      reforge_iter += current_reforge_sim->current_iteration * sim->threads;
    // At partition, add an extra amount of iterations to keep the iterations
    // correct
    else
      reforge_iter += ( reforge_plot_iterations > 0 ? reforge_plot_iterations
                                                    : sim->iterations );
  }

  sim->detailed_progress( detailed, (int)reforge_iter, (int)total_iter );

  return static_cast<double>( reforge_iter ) /
         static_cast<double>( total_iter );
}

// reforge_plot_t::create_options ===========================================

void reforge_plot_t::create_options()
{
  sim->add_option(
      opt_int( "reforge_plot_iterations", reforge_plot_iterations ) );
  sim->add_option(
      opt_float( "reforge_plot_target_error", reforge_plot_target_error ) );
  sim->add_option( opt_int( "reforge_plot_step", reforge_plot_step ) );
  sim->add_option( opt_int( "reforge_plot_amount", reforge_plot_amount ) );
  sim->add_option( opt_string( "reforge_plot_stat", reforge_plot_stat_str ) );
  sim->add_option( opt_bool( "reforge_plot_debug", reforge_plot_debug ) );
}

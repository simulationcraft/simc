// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "reforge_plot.hpp"

#include "player/player_scaling.hpp"
#include "player/sc_player.hpp"
#include "scale_factor_control.hpp"
#include "sim/sc_sim.hpp"
#include "sim/work_queue.hpp"
#include "util/io.hpp"

#include <sstream>

namespace
{  // UNNAMED NAMESPACE ==========================================

/// Will stat be reforge-plotted?
bool is_plot_stat( sim_t* sim, stat_e stat )
{
  // search for explicit stat option
  if ( !sim->reforge_plot->reforge_plot_stat_str.empty() )
  {
    auto stat_list =
        util::string_split<util::string_view>( sim->reforge_plot->reforge_plot_stat_str, ",:;/|" );

    if ( !range::any_of( stat_list, [stat]( util::string_view s ) {
      return stat == util::parse_stat_type( s );
    } ) )
    {
      // not found
      return false;
    }
  }

  // also check if any player scales_with that stat
  return range::any_of( sim->player_no_pet_list, [stat]( const player_t* p ) {
        return !p->quiet && p->scaling->scales_with[ stat ];
      } );
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
    const std::vector<stat_e>& stat_indices, int cur_mod_stat,
    std::vector<int> cur_stat_mods )
{
  if ( cur_mod_stat >= (int)stat_indices.size() - 1 )
  {
    int sum = 0;
    for ( size_t i = 0; i < cur_stat_mods.size() - 1; i++ )
      sum += cur_stat_mods[ i ];

    if ( abs( sum ) > reforge_plot_amount )
      return;

    for ( size_t i = 0; i < sim->player_no_pet_list.size(); ++i )
    {
      player_t* p = sim->player_no_pet_list[ i ];
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
    for ( size_t i = 0; i < sim->player_no_pet_list.size(); ++i )
    {
      player_t* p = sim->player_no_pet_list[ i ];
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

// reforge_plot_t::analyze_stats ============================================

void reforge_plot_t::analyze_stats()
{
  if ( reforge_plot_stat_str.empty() )
    return;

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

  std::vector<std::vector<int>> stat_mods;
  generate_stat_mods( stat_mods, reforge_plot_stat_indices, 0, cur_stat_mods );

  num_stat_combos = as<int>( stat_mods.size() );

  if ( reforge_plot_debug )
  {
    sim->out_debug.raw() << "Reforge Plot Stats:";
    for ( size_t i = 0; i < reforge_plot_stat_indices.size(); i++ )
    {
      sim->out_log.raw().print(
          "{}{}", i ? ", " : " ",
          util::stat_type_string( reforge_plot_stat_indices[ i ] ) );
    }
    sim->out_log.raw() << "\n";

    sim->out_log.raw() << "Reforge Plot Stat Mods:\n";
    for ( size_t i = 0; i < stat_mods.size(); i++ )
    {
      for ( size_t j = 0; j < stat_mods[ i ].size(); j++ )
        sim->out_log.raw().print(
            "{}: {} ", util::stat_type_string( reforge_plot_stat_indices[ j ] ),
            stat_mods[ i ][ j ] );
      sim->out_log.raw() << "\n";
    }
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
      stat_e stat = reforge_plot_stat_indices[ j ];
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

        player->reforge_plot_data.push_back( delta_result );
      }
    }

    delete current_reforge_sim;
    current_reforge_sim = nullptr;
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
    sim->error( "Unable to open plot output file '{}'.\n",
                 sim->reforge_plot_output_file_str );
    return;
  }

  for ( player_t* player : sim->player_list )
  {
    if ( player->quiet )
      continue;

    out << player->name() << " Reforge Plot Results:\n";

    for ( stat_e stat_index : reforge_plot_stat_indices )
    {
      out << util::stat_type_string( stat_index ) << ", ";
    }
    out << " DPS, DPS-Error\n";

    for ( const auto& plot_data_list : player->reforge_plot_data )
    {
      for ( const plot_data_t& plot_data : plot_data_list )
      {
        out << plot_data.value << ", ";
      }
      out << plot_data_list.back().error << ", ";
      out << "\n";
    }
  }
}

void reforge_plot_t::analyze()
{
  if ( sim->is_canceled() )
    return;

  if ( reforge_plot_stat_str.empty() )
    return;

  analyze_stats();

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
  for ( size_t i = 0; i < reforge_plot_stat_indices.size(); i++ )
  {
    phase += util::stat_type_abbrev( reforge_plot_stat_indices[ i ] );
    if ( i < reforge_plot_stat_indices.size() - 1 )
      phase += " to ";
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

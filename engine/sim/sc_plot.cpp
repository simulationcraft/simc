// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "plot.hpp"

#include "player/player_scaling.hpp"
#include "player/sc_player.hpp"
#include "report/reports.hpp"
#include "sc_sim.hpp"
#include "sim/work_queue.hpp"
#include "scale_factor_control.hpp"
#include "util/io.hpp"

#include <memory>

namespace
{  // UNNAMED NAMESPACE ==========================================

/// Will stat be plotted?
bool is_plot_stat( sim_t* sim, stat_e stat )
{
  // check if stat is in plot stat option
  if ( !sim->plot->dps_plot_stat_str.empty() )
  {
    auto stat_list =
        util::string_split<util::string_view>( sim->plot->dps_plot_stat_str, ",:;/|" );

    if ( !range::any_of( stat_list, [stat]( util::string_view s ) {
      return stat == util::parse_stat_type( s );
    } ) )
    {
      // not found
      return false;
    }
  }

  // also check if any player scales with that stat
  return range::any_of( sim->player_no_pet_list, [stat]( player_t* p ) {
    return !p->quiet && p->scaling->scales_with[ stat ];
  } );
}

}  // UNNAMED NAMESPACE ====================================================

// ==========================================================================
// Plot
// ==========================================================================

// plot_t::plot_t ===========================================================

plot_t::plot_t( sim_t* s )
  : sim( s ),
    dps_plot_step( 160.0 ),
    dps_plot_points( 20 ),
    dps_plot_iterations( -1 ),
    dps_plot_target_error( 0 ),
    dps_plot_debug( 0 ),
    current_plot_stat( STAT_NONE ),
    num_plot_stats( 0 ),
    remaining_plot_stats( 0 ),
    remaining_plot_points( 0 ),
    dps_plot_positive( false ),
    dps_plot_negative( false )
{
  create_options();
}

/// execute plotting
void plot_t::analyze()
{
  if ( sim->is_canceled() )
    return;

  if ( dps_plot_stat_str.empty() )
    return;

  analyze_stats();

  write_output_file();
}

// plot_t::progress =========================================================

double plot_t::progress( std::string& phase, std::string* detailed )
{
  if ( dps_plot_stat_str.empty() )
    return 1.0;

  if ( num_plot_stats <= 0 )
    return 1;

  if ( current_plot_stat <= 0 )
    return 0;

  phase = "Plot - ";
  phase += util::stat_type_abbrev( current_plot_stat );

  int completed_plot_stats = ( num_plot_stats - remaining_plot_stats );

  double stat_progress = completed_plot_stats / (double)num_plot_stats;

  int completed_plot_points = ( dps_plot_points - remaining_plot_points );

  double point_progress = completed_plot_points / (double)dps_plot_points;

  stat_progress += point_progress / num_plot_stats;

  sim->detailed_progress( detailed,
                          completed_plot_stats + completed_plot_points,
                          num_plot_stats + dps_plot_points );

  return stat_progress;
}

// plot_t::analyze_stats ====================================================

void plot_t::analyze_stats()
{
  if ( dps_plot_stat_str.empty() )
    return;

  if ( sim->players_by_name.empty() )
    return;

  remaining_plot_stats = 0;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    if ( is_plot_stat( sim, i ) )
      remaining_plot_stats++;
  num_plot_stats = remaining_plot_stats;

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( sim->is_canceled() )
      break;

    if ( !is_plot_stat( sim, i ) )
      continue;

    current_plot_stat = i;

    remaining_plot_points = dps_plot_points;

    int start;
    int end;

    if ( dps_plot_positive )
    {
      start = 0;
      end   = dps_plot_points;
    }
    else if ( dps_plot_negative )
    {
      start = -dps_plot_points;
      end   = 0;
    }
    else
    {
      start = -dps_plot_points / 2;
      end   = -start;
    }

    for ( int j = start; j <= end; j++ )
    {
      if ( sim->is_canceled() )
        break;

      std::unique_ptr<sim_t> delta_sim;

      if ( j != 0 )
      {
        delta_sim = std::make_unique<sim_t>( sim );
        if ( dps_plot_iterations > 0 )
        {
          delta_sim->work_queue->init( dps_plot_iterations );
        }
        if ( dps_plot_target_error > 0 )
          delta_sim->target_error = dps_plot_target_error;
        //delta_sim->enchant.add_stat( i, j * dps_plot_step );
        delta_sim->scaling->scale_stat = i;
        delta_sim->scaling->scale_value = j * dps_plot_step;
        delta_sim->progress_bar.set_base( util::to_string( j * dps_plot_step ) + " " + util::stat_type_abbrev( i ) );
        delta_sim->execute();
        if ( dps_plot_debug )
        {
          sim->out_debug.raw().print( "Stat={} Point={}\n",
                                      util::stat_type_string( i ), j );
          report::print_text( delta_sim.get(), true );
        }
      }

      for ( player_t* p : sim->players_by_name )
      {
        if ( !p->scaling->scales_with[ i ] )
          continue;

        plot_data_t data;

        if ( delta_sim )
        {
          player_t* delta_p = delta_sim->find_player( p->name() );

          scaling_metric_data_t scaling_data =
              delta_p->scaling_for_metric( p->sim->scaling->scaling_metric );

          data.value = scaling_data.value;
          data.error = scaling_data.stddev * delta_sim->confidence_estimator;
        }
        else
        {
          scaling_metric_data_t scaling_data =
              p->scaling_for_metric( p->sim->scaling->scaling_metric );
          data.value = scaling_data.value;
          data.error = scaling_data.stddev * sim->confidence_estimator;
        }
        data.plot_step = j * dps_plot_step;
        p->dps_plot_data[ i ].push_back( data );
      }

      if ( delta_sim )
      {
        remaining_plot_points--;
      }
    }

    remaining_plot_stats--;
  }
}

void plot_t::write_output_file()
{
  if ( sim->reforge_plot_output_file_str.empty() )
  {
    return;
  }

  io::ofstream out;
  out.open( sim->reforge_plot_output_file_str );
  if ( !out.is_open() )
  {
    sim->error( "Unable to open output file '{}' . \n",
                 sim->reforge_plot_output_file_str );
    return;
  }

  for ( player_t* player : sim->player_list )
  {
    if ( player->quiet )
      continue;

    out << player->name_str << " Plot Results:\n";

    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( !is_plot_stat( sim, j ) )
        continue;

      out << util::stat_type_string( j ) << ", DPS, DPS-Error\n";

      for ( const plot_data_t& p_data : player->dps_plot_data[ j ] )
      {
        out << p_data.plot_step << ", " << p_data.value << ", " << p_data.error
            << "\n";
      }
      out << "\n";
    }
  }
}

// plot_t::create_options ===================================================

void plot_t::create_options()
{
  sim->add_option( opt_int( "dps_plot_iterations", dps_plot_iterations ) );
  sim->add_option(
      opt_float( "dps_plot_target_error", dps_plot_target_error ) );
  sim->add_option( opt_int( "dps_plot_points", dps_plot_points ) );
  sim->add_option( opt_string( "dps_plot_stat", dps_plot_stat_str ) );
  sim->add_option( opt_float( "dps_plot_step", dps_plot_step ) );
  sim->add_option( opt_bool( "dps_plot_debug", dps_plot_debug ) );
  sim->add_option( opt_bool( "dps_plot_positive", dps_plot_positive ) );
  sim->add_option( opt_bool( "dps_plot_negative", dps_plot_negative ) );
}

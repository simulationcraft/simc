// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "plot.hpp"

#include "dbc/dbc.hpp"
#include "player/player.hpp"
#include "player/player_scaling.hpp"
#include "player/scaling_metric_data.hpp"
#include "report/reports.hpp"
#include "scale_factor_control.hpp"
#include "sim.hpp"
#include "sim/work_queue.hpp"
#include "util/io.hpp"
#include "util/plot_data.hpp"

#include <memory>

// ==========================================================================
// Plot
// ==========================================================================

// plot_t::plot_t ===========================================================

plot_t::plot_t( sim_t* s )
  : sim( s ),
    dps_plot_step( -1 ),
    dps_plot_points( 20 ),
    dps_plot_iterations( -1 ),
    dps_plot_target_error( 0 ),
    dps_plot_debug( 0 ),
    current_plot_stat( STAT_NONE ),
    num_plot_stats( 0 ),
    remaining_plot_stats( 0 ),
    remaining_plot_points( 0 ),
    dps_plot_positive( false ),
    dps_plot_negative( false ),
    dps_plot_display_delta( false )
{
  create_options();
}

void plot_t::initialize()
{
  // set default step to 0.5% worth of rating. use haste if multiple stats are plotted, otherwise use the plotted stat
  if ( dps_plot_step == -1 )
  {
    auto rating = RATING_MELEE_HASTE;
    if ( dps_plot_stats.size() == 1 )
      rating = util::stat_to_rating( dps_plot_stats.begin()->first );

    if ( rating == RATING_MAX )
      rating = RATING_MELEE_HASTE;

    dps_plot_step = util::round( sim->dbc->combat_rating( rating, sim->max_player_level ) * 0.005 );
  }

  // prune out stats no players scale with
  for ( auto it = dps_plot_stats.begin(); it != dps_plot_stats.end(); )
  {
    auto valid = range::any_of( sim->player_no_pet_list, [ stat = it->first ]( player_t* p ) {
      return !p->quiet && p->scaling->scales_with[ stat ];
    } );

    if ( valid )
      it++;
    else
      it = dps_plot_stats.erase( it );
  }
}

/// execute plotting
void plot_t::analyze()
{
  if ( sim->is_canceled() )
    return;

  if ( dps_plot_stats.empty() )
    return;

  analyze_stats();

  write_output_file();
}

// plot_t::progress =========================================================

double plot_t::progress( std::string& phase, std::string* detailed )
{
  if ( dps_plot_stats.empty() )
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

  sim->detailed_progress( detailed, completed_plot_stats + completed_plot_points, num_plot_stats + dps_plot_points );

  return stat_progress;
}

// plot_t::analyze_stats ====================================================

void plot_t::analyze_stats()
{
  if ( sim->players_by_name.empty() )
    return;

  remaining_plot_stats = as<int>( dps_plot_stats.size() );
  num_plot_stats = remaining_plot_stats;

  for ( auto [ i, starting_amount ] : dps_plot_stats )
  {
    if ( sim->is_canceled() )
      break;

    current_plot_stat = i;

    remaining_plot_points = dps_plot_points;

    int start;
    int end;

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
      start = -dps_plot_points / 2;
      end = -start;
    }

    // redo the 0-sim if we need to override the initial rating
    bool redo_zero = starting_amount >= 0;

    if ( redo_zero )
      remaining_plot_points++;

    for ( int j = start; j <= end; j++ )
    {
      if ( sim->is_canceled() )
        break;

      std::unique_ptr<sim_t> delta_sim;

      if ( j != 0 || redo_zero )
      {
        delta_sim = std::make_unique<sim_t>( sim );
        if ( dps_plot_iterations > 0 )
          delta_sim->work_queue->init( dps_plot_iterations );

        if ( dps_plot_target_error > 0 )
          delta_sim->target_error = dps_plot_target_error;

        delta_sim->scaling->scale_stat = i;
        delta_sim->scaling->scale_value = j * dps_plot_step;
        delta_sim->progress_bar.set_base( util::to_string( j * dps_plot_step ) + " " + util::stat_type_abbrev( i ) );
        delta_sim->execute();
        if ( dps_plot_debug )
        {
          sim->out_debug.raw().print( "Stat={} Point={}\n", util::stat_type_string( i ), j );
          report::print_text( delta_sim.get(), true );
        }
      }

      // discard baseline result if we need to redo the 0-sim
      if ( j == 0 && redo_zero && !delta_sim )
        continue;

      for ( player_t* p : sim->players_by_name )
      {
        if ( !p->scaling->scales_with[ i ] )
          continue;

        plot_data_t data;

        if ( delta_sim )
        {
          player_t* delta_p = delta_sim->find_player( p->name() );

          scaling_metric_data_t scaling_data = delta_p->scaling_for_metric( p->sim->scaling->scaling_metric );

          data.value = scaling_data.value;
          data.error = scaling_data.stddev * delta_sim->confidence_estimator;
        }
        else
        {
          scaling_metric_data_t scaling_data = p->scaling_for_metric( p->sim->scaling->scaling_metric );
          data.value = scaling_data.value;
          data.error = scaling_data.stddev * sim->confidence_estimator;
        }
        data.plot_step = j * dps_plot_step;

        if ( dps_plot_display_delta )
        {
          plot_data_t delta;
          delta.value = p->dps_plot_data[ i ].empty() ? 0.0 : data.value - p->dps_plot_data[ i ].back().value;
          delta.error = data.error;
          delta.plot_step = data.plot_step;
          p->dps_plot_delta_data[ i ].push_back( delta );
        }

        p->dps_plot_data[ i ].push_back( data );
      }

      if ( delta_sim )
        remaining_plot_points--;
    }

    remaining_plot_stats--;
  }
}

void plot_t::write_output_file()
{
  if ( sim->reforge_plot_output_file_str.empty() )
    return;

  io::ofstream out;
  out.open( sim->reforge_plot_output_file_str );
  if ( !out.is_open() )
  {
    sim->error( "Unable to open output file '{}' . \n", sim->reforge_plot_output_file_str );
    return;
  }

  for ( player_t* player : sim->player_list )
  {
    if ( player->quiet )
      continue;

    out << player->name_str << " Plot Results:\n";

    for ( const auto& [ j, plot_data_list ] : player->dps_plot_data )
    {
      out << util::stat_type_string( j ) << ", DPS, DPS-Error\n";

      for ( const auto& p_data : plot_data_list )
        out << p_data.plot_step << ", " << p_data.value << ", " << p_data.error << "\n";

      out << "\n";
    }
  }
}

bool parse_plot_string( sim_t* sim, std::string_view, std::string_view value )
{
  auto split = util::string_split<std::string_view>( value, ",;/|" );

  for ( auto stat_str : split )
  {
    auto stat_split = util::string_split<std::string_view>( stat_str, ":" );

    auto stat = util::parse_stat_type( stat_split[ 0 ] );
    if ( stat == STAT_NONE )
      throw std::invalid_argument( "Invalid stat for DPS plot." );

    double starting_amount = -1;
    if ( stat_split.size() > 1 )
    {
      auto amount = util::to_double( stat_split[ 1 ] );
      if ( amount < 0 )
        throw std::invalid_argument( "Invalid stat starting amount for DPS plot." );

      starting_amount = amount;
    }

    sim->plot->dps_plot_stats[ stat ] = starting_amount;
  }

  return true;
}

// plot_t::create_options ===================================================

void plot_t::create_options()
{
  sim->add_option( opt_int( "dps_plot_iterations", dps_plot_iterations ) );
  sim->add_option( opt_float( "dps_plot_target_error", dps_plot_target_error ) );
  sim->add_option( opt_int( "dps_plot_points", dps_plot_points ) );
  sim->add_option( opt_func( "dps_plot_stat", parse_plot_string ) );
  sim->add_option( opt_float( "dps_plot_step", dps_plot_step ) );
  sim->add_option( opt_bool( "dps_plot_debug", dps_plot_debug ) );
  sim->add_option( opt_bool( "dps_plot_positive", dps_plot_positive ) );
  sim->add_option( opt_bool( "dps_plot_negative", dps_plot_negative ) );
  sim->add_option( opt_bool( "dps_plot_display_delta", dps_plot_display_delta ) );
}

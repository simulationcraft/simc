// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "progress_bar.hpp"
#include "sim/sc_sim.hpp"
#include "util/util.hpp"
#include "player/sc_player.hpp"
#include "player/player_scaling.hpp"
#include "sim_control.hpp"
#include "sim/plot.hpp"
#include "sim/reforge_plot.hpp"
#include "sim/work_queue.hpp"
#include "sim/sc_profileset.hpp"
#include "scale_factor_control.hpp"
#include "gsl-lite/gsl-lite.hpp"

#include <iostream>
#include <sstream>

std::string progress_bar_t::format_time( double t )
{
  std::stringstream s;

  int days = 0;
  int hours = 0;
  int minutes = 0;

  double remainder = t;

  if ( remainder >= 86400 )
  {
    days = static_cast<int>(remainder / 86400);
    remainder -= days * 86400;
  }

  if ( remainder >= 3600 )
  {
    hours = static_cast<int>(remainder / 3600);
    remainder -= hours * 3600;
  }

  if ( remainder >= 60 )
  {
    minutes = static_cast<int>(remainder / 60);
    remainder -= minutes * 60;
  }

  if ( days > 0 )
  {
    s << days << "d, ";
  }

  if ( hours > 0 )
  {
    s << hours << "h, ";
  }

  if ( minutes > 0 )
  {
    s << minutes << "m, ";
  }

  s << util::round( remainder, 0 ) << "s";

  return s.str();
}

progress_bar_t::progress_bar_t( sim_t& s ) :
    sim( s ), steps( 20 ), updates( 100 ), interval( 0 ), update_number( 0 ),
    max_interval_time( std::chrono::seconds( 1 ) ), work_index( 0 ), total_work_( 0 ),
    elapsed_time( 0 ), time_count( 0 )
{
}

void progress_bar_t::init()
{
  start_time = chrono::wall_clock::now();
  if ( sim.target_error > 0 )
  {
    interval = sim.analyze_error_interval;
  }
  else
  {
    interval = sim.work_queue -> size();
    if ( sim.deterministic || sim.strict_work_queue )
    {
      interval *= sim.threads;
    }
    interval /= updates;
  }
  if ( interval == 0 )
  {
    interval = 1;
  }
}

void progress_bar_t::restart()
{
  add_simulation_time( chrono::elapsed_fp_seconds( start_time ) );

  start_time = chrono::wall_clock::now();
  last_update = start_time;
  update_number = 0;
}

bool progress_bar_t::update( bool finished, int index )
{
  if ( sim.thread_index != 0 )
  {
    return false;
  }

  if ( ! sim.report_progress )
  {
    return false;
  }

  if ( ! sim.current_iteration )
  {
    return false;
  }

  if ( ! finished && sim.target_error > 0 && sim.current_iteration < sim.analyze_error_interval )
  {
    return false;
  }

  auto progress = sim.progress( nullptr, index );
  if ( ! finished )
  {
    auto update_interval = last_update - chrono::wall_clock::now();
    if ( progress.current_iterations < update_number + interval &&
         update_interval < max_interval_time )
    {
      return false;
    }
    update_number = progress.current_iterations;
    last_update = chrono::wall_clock::now();
  }

  if ( sim.progressbar_type == 1 )
  {
    return update_simple( progress, finished, index );
  }
  else
  {
    return update_normal( progress, finished, index );
  }
}

bool progress_bar_t::update_simple( const sim_progress_t& progress, bool finished, int /* index */ )
{
  auto pct = progress.pct();
  if ( pct <= 0 )
  {
    return false;
  }

  if ( finished )
  {
    pct = 1.0;
  }

  double current_time = chrono::elapsed_fp_seconds( start_time );
  double total_time = current_time / pct;

  double remaining_time = total_time - current_time;

  auto wall_seconds = chrono::elapsed_fp_seconds( start_time );

  status = util::to_string( finished ? progress.total_iterations : progress.current_iterations );
  status += '\t';
  status += util::to_string( progress.total_iterations );
  status += '\t';
  status += util::to_string( progress.current_iterations / wall_seconds / sim.threads );
  if ( sim.target_error > 0 )
  {
    status += '\t';
    status += util::to_string( sim.current_mean );
    status += '\t';
    status += util::to_string( sim.current_error );
  }

  if ( remaining_time > 0 )
  {
    status += '\t';
    status += util::to_string( util::round( remaining_time, 3 ) );
  }
  else
  {
    if ( finished && current_time > 0 )
    {
      status += '\t';
      status += util::to_string( util::round( current_time, 3 ) );
    }
    else
    {
      status += "\t0.000";
    }
  }

  if ( ! finished && total_work() > 0 )
  {
    auto average_spent = average_simulation_time();
    auto phases_left = total_work() - current_progress();
    auto time_left = std::max( 0.0, average_spent - chrono::elapsed_fp_seconds( start_time ) );
    auto total_left = phases_left * average_spent + time_left;

    if ( total_left > 0 )
    {
      status += "\t";
      status += format_time( total_left );
    }
  }

  return true;
}

bool progress_bar_t::update_normal( const sim_progress_t& progress, bool finished, int /* index */ )
{
  auto pct = progress.pct();
  if ( pct <= 0 )
  {
    return false;
  }

  if ( finished )
  {
    pct = 1.0;
  }

  size_t prev_size = status.size();
  fmt::memory_buffer new_status;

  int progress_length = as<int>( std::lround( steps * pct ) );
  if ( progress_length >= 1 )
  {
    fmt::format_to(new_status, "[{:=>{}s}", ">", progress_length);
    fmt::format_to(new_status, "{:.>{}}]", "", steps - progress_length);
  }
  else
  {
    fmt::format_to(new_status, "[{:=>{}}", "", progress_length);
    fmt::format_to(new_status, "{:.>{}}]", "", steps - progress_length);
  }

  double current_time = chrono::elapsed_fp_seconds( start_time );
  double total_time = current_time / pct;

  int remaining_sec = static_cast<int>( total_time - current_time );
  int remaining_min = remaining_sec / 60;
  remaining_sec -= remaining_min * 60;

  fmt::format_to(new_status, " {:d}/{:d}", finished ? progress.total_iterations : progress.current_iterations, progress.total_iterations );

  // Simple estimate of average iteraitons per thread
  auto wall_seconds = chrono::elapsed_fp_seconds( start_time );
  fmt::format_to(new_status, " {:.3f}", progress.current_iterations / wall_seconds / sim.threads );

  if ( sim.target_error > 0 )
  {
    fmt::format_to(new_status, " Mean={:.0f} Error={:.3f}%", sim.current_mean, sim.current_error );
  }

  if ( remaining_min > 0 )
  {
    fmt::format_to(new_status, " {:d}min", remaining_min );
  }

  if ( remaining_sec > 0 )
  {
    fmt::format_to(new_status, " {:d}sec", remaining_sec );
  }

  if ( finished )
  {
    int total_min = gsl::narrow_cast<int>(current_time / 60);
    int total_sec = gsl::narrow_cast<int>(std::fmod( current_time, 60 ));
    int total_msec = gsl::narrow_cast<int>(1000 * ( current_time - static_cast<int>( current_time ) ));
    if ( total_min > 0 )
    {
      fmt::format_to(new_status, " {:d}min", total_min );
    }

    if ( total_sec > 0 )
    {
      fmt::format_to(new_status, " {:d}", total_sec );
      if ( total_msec > 0 )
      {
        fmt::format_to(new_status, ".{:d}", total_msec );
      }

      status += "sec";
    }
    else if ( total_msec > 0 )
    {
      fmt::format_to(new_status, " {:d}msec", total_msec );
    }
  }

  if ( total_work() > 0 )
  {
    auto average_spent = average_simulation_time();
    auto phases_left = total_work() - current_progress();
    auto time_left = std::max( 0.0, average_spent - wall_seconds );
    auto total_left = phases_left * average_spent + time_left;

    if ( total_left > 0 )
    {
      fmt::format_to(new_status, " ({:s})", format_time( total_left ));
    }
  }

  if ( prev_size > new_status.size() )
  {
    fmt::format_to(new_status, "{:<{}}", " ", prev_size - new_status.size() );
  }

  status = fmt::to_string(new_status);

  return true;
}

void progress_bar_t::output( bool finished )
{
  if ( ! sim.report_progress )
  {
    return;
  }

  char delim = sim.progressbar_type == 1 ? '\t' : ' ';
  char terminator = ( sim.progressbar_type == 1 || finished ) ? '\n' : '\r';
  std::stringstream s;
  if ( sim.progressbar_type == 0 )
  {
    s << "Generating ";
  }

  s << base_str;
  // Separate base and phase by a colon for easier parsing
  if ( sim.progressbar_type == 0 )
  {
    s << ":";
  }

  if ( ! phase_str.empty() )
  {
    s << delim;
    s << phase_str;
  }

  s << delim;
  s << current_progress();
  if ( sim.progressbar_type == 0 )
  {
    s << "/";
  }
  else
  {
    s << "\t";
  }
  s << compute_total_phases();
  s << delim;
  s << status;
  s << terminator;

  std::cout << s.str() << std::flush;
}

void progress_bar_t::progress()
{
  if ( sim.thread_index > 0 )
  {
    return;
  }

  if ( sim.parent )
  {
    sim.parent -> progress_bar.progress();
  }
  else
  {
    work_index++;
  }
}

size_t progress_bar_t::compute_total_phases()
{
  if ( sim.parent )
  {
    return sim.parent -> progress_bar.compute_total_phases();
  }

  if ( total_work_ > 0 )
  {
    return total_work_;
  }

  size_t n_actors = 1;
  if ( sim.single_actor_batch )
  {
    n_actors = sim.player_no_pet_list.size();
  }

//  size_t reforge_plot_phases = 0;
//  if ( sim.reforge_plot -> num_stat_combos > 0 )
//  {
//    reforge_plot_phases = n_actors * sim.reforge_plot -> num_stat_combos;
//  }

  auto work = n_actors /* baseline */ +
              n_scale_factor_phases() +
              n_plot_phases() +
              n_reforge_plot_phases() +
              sim.profilesets->n_profilesets();

  // Once profilesets have all been constructed, cache the total work done since we can determine
  // the maximum number of phases then
  if ( sim.profilesets->is_running() )
  {
    total_work_ = work;
  }

  return total_work_ > 0 ? total_work_ : work;
}

void progress_bar_t::set_base( const std::string& base )
{
  base_str = base;
}

void progress_bar_t::set_phase( const std::string& phase )
{
  phase_str = phase;
}

size_t progress_bar_t::current_progress() const
{
  if ( sim.parent )
  {
    return sim.parent -> progress_bar.current_progress();
  }

  return work_index;
}

size_t progress_bar_t::total_work() const
{
  if ( sim.parent )
  {
    return sim.parent -> progress_bar.total_work();
  }

  return total_work_;
}

size_t progress_bar_t::n_stat_scaling_players( util::string_view stat_str ) const
{
  auto stat = util::parse_stat_type( stat_str );
  if ( stat == STAT_NONE )
  {
    return 0;
  }

  // Ensure sim has at least someone who scales with the stat
  return std::count_if( sim.player_no_pet_list.begin(), sim.player_no_pet_list.end(),
    [ stat ]( const player_t* p ) {
      return ! p -> quiet && p -> scaling -> scales_with[ stat ];
    } );
}

size_t progress_bar_t::n_plot_phases() const
{
  if ( sim.plot -> dps_plot_stat_str.empty() )
  {
    return 0;
  }

  size_t n_phases = 0;
  auto stat_list = util::string_split<util::string_view>( sim.plot -> dps_plot_stat_str, ",:;/|" );
  range::for_each( stat_list, [ &n_phases, this ]( util::string_view stat_str ) {
    auto n_players = n_stat_scaling_players( stat_str );

    if ( n_players > 0 )
    {
      // Don't use fancy context-sensitive number of phases, but rather just multiply with the
      // number of actors if single_actor_batch=1. In the future if scale factor calculation is made
      // context (stat) sensitive, adjust this.
      /* n_phases += sim.plot -> dps_plot_points * ( sim.single_actor_batch == 1 ? n_players : 1 ); */
      n_phases += sim.plot -> dps_plot_points *
                  ( sim.single_actor_batch == 1 ? sim.player_no_pet_list.size() : 1 );
    }
  } );

  return n_phases;
}

size_t progress_bar_t::n_scale_factor_phases() const
{
  if ( ! sim.scaling -> calculate_scale_factors )
  {
    return 0;
  }

  size_t n_phases = 0;
  auto stat_list = util::string_split<util::string_view>( sim.scaling -> scale_only_str, ",:;/|" );
  std::vector<stat_e> scale_only;

  range::for_each( stat_list, [ &scale_only ]( util::string_view stat_str ) {
    auto stat = util::parse_stat_type( stat_str );
    if ( stat != STAT_NONE )
    {
      scale_only.push_back( stat );
    }
  } );

  for ( stat_e stat = STAT_NONE; stat < STAT_MAX; ++stat )
  {
    if ( !scale_only.empty() && range::find( scale_only, stat ) == scale_only.end() )
    {
      continue;
    }

    auto n_players = std::count_if( sim.player_no_pet_list.begin(), sim.player_no_pet_list.end(),
      [ stat ]( const player_t* p ) {
        return ! p -> quiet && p -> scale_player && p -> scaling -> scales_with[ stat ];
      } );

    if ( n_players > 0 )
    {
      // Don't use fancy context-sensitive number of phases, but rather just multiply with the
      // number of actors if single_actor_batch=1. In the future if scale factor calculation is made
      // context (stat) sensitive, adjust this.
      /* n_phases += sim.single_actor_batch == 1 ? n_players : 1; */
      n_phases += sim.single_actor_batch == 1 ? sim.player_no_pet_list.size() : 1;

      // Center delta doubles the number of scale factor phases
      if ( sim.scaling -> center_scale_delta )
      {
        n_phases += sim.single_actor_batch == 1 ? sim.player_no_pet_list.size() : 1;
      }
    }
  }

  return n_phases;
}

size_t progress_bar_t::n_reforge_plot_phases() const
{
  if ( sim.reforge_plot -> reforge_plot_stat_str.empty() )
  {
    return 0;
  }

  auto stat_list = util::string_split<util::string_view>( sim.reforge_plot -> reforge_plot_stat_str, ",:;/|" );
  std::vector<stat_e> stat_indices;
  range::for_each( stat_list, [ &stat_indices, this ]( util::string_view stat_str ) {
    if ( n_stat_scaling_players( stat_str ) > 0 )
    {
      stat_indices.push_back( util::parse_stat_type( stat_str ) );
    }
  } );

  std::vector<int> cur_stat_mods( stat_indices.size() );
  std::vector<std::vector<int>> stat_mods;

  sim.reforge_plot -> generate_stat_mods( stat_mods, stat_indices, 0, cur_stat_mods );

  return sim.single_actor_batch == 1
         ? sim.player_no_pet_list.size() * stat_mods.size()
         : stat_mods.size();
}

void progress_bar_t::add_simulation_time( double t )
{
  if ( t <= 0 )
  {
    return;
  }

  if ( sim.parent )
  {
    sim.parent -> progress_bar.add_simulation_time( t );
  }
  else
  {
    elapsed_time += t;
    time_count++;
  }
}

double progress_bar_t::average_simulation_time() const
{
  if ( sim.parent )
  {
    return sim.parent -> progress_bar.average_simulation_time();
  }

  return time_count > 0 ? elapsed_time / time_count : 0;
}

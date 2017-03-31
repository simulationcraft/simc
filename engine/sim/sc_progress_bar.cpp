// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

progress_bar_t::progress_bar_t( sim_t& s ) :
    sim( s ), steps( 20 ), updates( 100 ), interval( 0 ), start_time( 0 )
{
}

void progress_bar_t::init()
{
  start_time = util::wall_time();
  if ( sim.target_error > 0 )
  {
    interval = sim.analyze_error_interval;
  }
  else
  {
    interval = sim.work_queue -> size() / updates;
  }
  if ( interval == 0 )
  {
    interval = 1;
  }
}

void progress_bar_t::restart()
{ start_time = util::wall_time(); }

bool progress_bar_t::update( bool finished, int index )
{
  if ( sim.thread_index != 0 )
    return false;
  if ( !sim.report_progress )
    return false;
  if ( !sim.current_iteration )
    return false;

  if ( !finished )
  {
    if ( sim.current_iteration % interval )
    {
      return false;
    }
  }

  auto progress = sim.progress( nullptr, index );
  auto pct = progress.pct();
  if ( pct <= 0 )
    return false;
  if ( finished )
    pct = 1.0;

  size_t prev_size = status.size();

  status = "[";
  status.insert( 1, steps, '.' );
  status += "]";

  int length = static_cast<int>(steps * pct + 0.5);
  for ( int i = 1; i < length + 1; ++i )
    status[i] = '=';
  if ( length > 0 )
    status[length] = '>';

  double current_time = util::wall_time() - start_time;
  double total_time = current_time / pct;

  int remaining_sec = int( total_time - current_time );
  int remaining_min = remaining_sec / 60;
  remaining_sec -= remaining_min * 60;

  str::format( status, " %d/%d", finished ? progress.total_iterations : progress.current_iterations, progress.total_iterations );

  if ( sim.target_error > 0 )
  {
    str::format( status, " Mean=%.0f Error=%.3f%%", sim.current_mean, sim.current_error );
  }

  if ( remaining_min > 0 )
  {
    str::format( status, " %dmin", remaining_min );
  }

  if ( remaining_sec > 0 )
  {
    str::format( status, " %dsec", remaining_sec );
  }

  if ( finished )
  {
    int total_min = current_time / 60;
    int total_sec = fmod( current_time, 60 );
    int total_msec = 1000 * ( current_time - static_cast<int>( current_time ) );
    if ( total_min > 0 )
    {
      str::format( status, " %dmin", total_min );
    }

    if ( total_sec > 0 )
    {
      str::format( status, " %d", total_sec );
      if ( total_msec > 0 )
      {
        str::format( status, ".%d", total_msec );
      }

      status += "sec";
    }
    else if ( total_msec > 0 )
    {
      str::format( status, " %dmsec", total_msec );
    }

  }

  if ( prev_size > status.size() )
    status.insert( status.end(), ( prev_size - status.size() ), ' ' );

  return true;
}

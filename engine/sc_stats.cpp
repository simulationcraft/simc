// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// stats_t::stats_t =========================================================

stats_t::stats_t( const std::string& n, player_t* p ) :
    name_str( n ), sim( p->sim ), player( p ), next( 0 ), school( SCHOOL_NONE ),
    channeled( false ), analyzed( false ), initialized( false ),
    resource_consumed( 0 ), last_execute( -1 )
{
}

// stats_t::init ============================================================

void stats_t::init()
{
  if ( initialized ) return;
  initialized = true;

  resource_consumed = 0;

  num_buckets = ( int ) sim -> max_time;
  if ( num_buckets == 0 ) num_buckets = 600; // Default to 10 minutes
  num_buckets *= 2;

  timeline_dmg.clear();
  timeline_dmg.insert( timeline_dmg.begin(), num_buckets, 0 );

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    execute_results[ i ].count     = 0;
    execute_results[ i ].min_dmg   = FLT_MAX;
    execute_results[ i ].max_dmg   = 0;
    execute_results[ i ].avg_dmg   = 0;
    execute_results[ i ].total_dmg = 0;

    tick_results[ i ].count     = 0;
    tick_results[ i ].min_dmg   = FLT_MAX;
    tick_results[ i ].max_dmg   = 0;
    tick_results[ i ].avg_dmg   = 0;
    tick_results[ i ].total_dmg = 0;
  }

  num_executes = num_ticks = 0;
  total_execute_time = total_tick_time = 0;
  total_dmg = portion_dmg = 0;
  dps = dpe = dpet = dpr = 0;
  total_intervals = num_intervals = 0;
}

// stats_t::reset ===========================================================

void stats_t::reset( action_t* a )
{
  channeled = a -> channeled;
  last_execute = -1;
}

// stats_t::add_time ========================================================

void stats_t::add_time( double amount,
			int    dmg_type )
{
  if ( dmg_type == DMG_DIRECT )
  {
    total_execute_time += amount;
  }
  else
  {
    total_tick_time += amount;
  }
}

// stats_t::add_result ======================================================

void stats_t::add_result( double amount,
			  int    dmg_type,
			  int    result )
{
  // Check for DoT application
  if( amount == 0 )
    if( result == RESULT_HIT || result == RESULT_CRIT )
      return;

  player -> iteration_dmg += amount;
  total_dmg += amount;

  stats_results_t& r = ( dmg_type == DMG_DIRECT ) ? execute_results[ result ] : tick_results[ result ];

  r.count += 1;
  r.total_dmg += amount;

  if ( amount < r.min_dmg ) r.min_dmg = amount;
  if ( amount > r.max_dmg ) r.max_dmg = amount;

  int index = ( int ) ( sim -> current_time );
  if ( index >= num_buckets )
  {
    timeline_dmg.insert( timeline_dmg.begin() + num_buckets, index, 0 );
    num_buckets += index;
  }

  timeline_dmg[ index ] += amount;
}

// stats_t::add =============================================================

void stats_t::add( double amount,
                   int    dmg_type,
                   int    result,
                   double time )
{
  add_result( amount, dmg_type, result );

  if ( dmg_type == DMG_DIRECT )
  {
    num_executes++;
    total_execute_time += time;

    if ( last_execute > 0 &&
         last_execute != sim -> current_time )
    {
      num_intervals++;
      total_intervals += sim -> current_time - last_execute;
    }
    last_execute = sim -> current_time;
  }
  else
  {
    num_ticks++;
    total_tick_time += time;
  }
}

// stats_t::analyze =========================================================

void stats_t::analyze()
{
  if ( analyzed ) return;
  analyzed = true;

  int num_iterations = sim -> iterations;

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    if ( execute_results[ i ].count != 0 )
    {
      stats_results_t& r = execute_results[ i ];

      r.avg_dmg = r.total_dmg / r.count;

      r.count     /= num_iterations;
      r.total_dmg /= num_iterations;
    }
    if ( tick_results[ i ].count != 0 )
    {
      stats_results_t& r = tick_results[ i ];

      r.avg_dmg = r.total_dmg / r.count;

      r.count     /= num_iterations;
      r.total_dmg /= num_iterations;
    }
  }

  if ( total_dmg > 0 )
  {
    dps  = total_dmg / ( total_execute_time + total_tick_time );
    dpe  = total_dmg / num_executes;
    dpet = total_dmg / ( total_execute_time + ( channeled ? total_tick_time : 0 ) );
    dpr  = ( resource_consumed > 0.0 ) ? ( total_dmg / resource_consumed ) : 0;
  }

  resource_consumed /= num_iterations;

  num_executes /= num_iterations;
  num_ticks    /= num_iterations;

  frequency = num_intervals ? total_intervals / num_intervals : 0;

  total_execute_time /= num_iterations;
  total_tick_time    /= num_iterations;
  total_dmg          /= num_iterations;

  for ( int i=0; i < num_buckets; i++ )
  {
    timeline_dmg[ i ] /= num_iterations;
  }

  timeline_dps.clear();
  timeline_dps.insert( timeline_dps.begin(), num_buckets, 0 );

  int max_buckets = std::min( ( int ) player -> total_seconds, ( int ) timeline_dmg.size() );

  for ( int i=0; i < max_buckets; i++ )
  {
    double window_dmg  = timeline_dmg[ i ];
    int    window_size = 1;

    for ( int j=1; ( j <= 10 ) && ( ( i-j ) >=0 ); j++ )
    {
      window_dmg += timeline_dmg[ i-j ];
      window_size++;
    }
    for ( int j=1; ( j <= 10 ) && ( ( i+j ) < max_buckets ); j++ )
    {
      window_dmg += timeline_dmg[ i+j ];
      window_size++;
    }

    timeline_dps[ i ] = window_dmg / window_size;
  }
}

// stats_t::merge ===========================================================

void stats_t::merge( stats_t* other )
{
  resource_consumed  += other -> resource_consumed;
  num_executes       += other -> num_executes;
  num_ticks          += other -> num_ticks;
  total_execute_time += other -> total_execute_time;
  total_tick_time    += other -> total_tick_time;
  total_dmg          += other -> total_dmg;

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    if ( other -> execute_results[ i ].count != 0 )
    {
      stats_results_t& other_r = other -> execute_results[ i ];
      stats_results_t&       r =          execute_results[ i ];

      r.count     += other_r.count;
      r.total_dmg += other_r.total_dmg;

      if ( other_r.min_dmg < r.min_dmg ) r.min_dmg = other_r.min_dmg;
      if ( other_r.max_dmg > r.max_dmg ) r.max_dmg = other_r.max_dmg;
    }
    if ( other -> tick_results[ i ].count != 0 )
    {
      stats_results_t& other_r = other -> tick_results[ i ];
      stats_results_t&       r =          tick_results[ i ];

      r.count     += other_r.count;
      r.total_dmg += other_r.total_dmg;

      if ( other_r.min_dmg < r.min_dmg ) r.min_dmg = other_r.min_dmg;
      if ( other_r.max_dmg > r.max_dmg ) r.max_dmg = other_r.max_dmg;
    }
  }

  for ( int i=0; i < num_buckets && i < other -> num_buckets; i++ )
  {
    timeline_dmg[ i ] += other -> timeline_dmg[ i ];
  }
}

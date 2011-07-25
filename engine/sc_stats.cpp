// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// stats_t::stats_t =========================================================

stats_t::stats_t( const std::string& n, player_t* p ) :
  name_str( n ), sim( p->sim ), player( p ), next( 0 ), parent( 0 ),
  school( SCHOOL_NONE ), type( STATS_DMG ), analyzed( false ),
  initialized( false ), quiet( false ), resource( RESOURCE_NONE ),
  resource_consumed( 0 ), last_execute( -1 )
{
}

// stats_t::add_child =======================================================

void stats_t::add_child( stats_t* child )
{
  if( child -> parent )
  {
    if ( child -> parent != this )
    {
      sim -> errorf( "stats_t %s already has parent %s, can't parent to %s", child-> name_str.c_str(), child->parent-> name_str.c_str(), name_str.c_str() );
      assert( 0 );
    }
    return;
  }
  child -> parent = this;
  children.push_back( child );
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
    direct_results[ i ].count     = 0;
    direct_results[ i ].min_dmg   = FLT_MAX;
    direct_results[ i ].max_dmg   = 0;
    direct_results[ i ].avg_dmg   = 0;
    direct_results[ i ].total_dmg = 0;
    direct_results[ i ].pct       = 0;

    tick_results[ i ].count     = 0;
    tick_results[ i ].min_dmg   = FLT_MAX;
    tick_results[ i ].max_dmg   = 0;
    tick_results[ i ].avg_dmg   = 0;
    tick_results[ i ].total_dmg = 0;
    tick_results[ i ].pct       = 0;
  }

  num_direct_results = num_tick_results = 0;
  num_executes = num_ticks = 0;
  total_execute_time = total_tick_time = 0;
  total_dmg = portion_dmg = compound_dmg = opportunity_cost = 0;
  dps = dpe = dpet = dpr = etpe = ttpt = 0;
  total_intervals = num_intervals = 0;
}

// stats_t::reset ===========================================================

void stats_t::reset()
{
  last_execute = -1;
}

// stats_t::add_result ======================================================

void stats_t::add_result( double amount,
                          int    dmg_type,
                          int    result )
{
  player -> iteration_dmg += amount;
  total_dmg += amount;

  stats_results_t* r = 0;

  if ( dmg_type == DMG_DIRECT || dmg_type == HEAL_DIRECT || dmg_type == ABSORB )
  {
    r = &( direct_results[ result ] );
    num_direct_results++;
  }
  else
  {
    r = &( tick_results[ result ] );
    num_tick_results++;
  }

  r -> count += 1;
  r -> total_dmg += amount;

  if ( amount < r -> min_dmg ) r -> min_dmg = amount;
  if ( amount > r -> max_dmg ) r -> max_dmg = amount;

  int index = ( int ) ( sim -> current_time );
  if ( index >= num_buckets )
  {
    timeline_dmg.insert( timeline_dmg.begin() + num_buckets, index, 0 );
    num_buckets += index;
  }

  timeline_dmg[ index ] += amount;
}

// stats_t::add_execute =============================================================

void stats_t::add_execute( double time )
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

// stats_t::add_tick =============================================================

void stats_t::add_tick( double time )
{
  num_ticks++;
  total_tick_time += time;
}

// stats_t::analyze =========================================================

void stats_t::analyze()
{
  if ( analyzed ) return;
  analyzed = true;

  bool channeled = false;
  int num_actions = action_list.size();
  for ( int i=0; i < num_actions; i++ )
  {
    action_t* a = action_list[ i ];
    if ( a -> channeled ) channeled = true;
    school   = a -> school;
    resource = a -> resource;
  }

  int num_iterations = sim -> iterations;

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    if ( direct_results[ i ].count != 0 )
    {
      stats_results_t& r = direct_results[ i ];

      r.avg_dmg = r.total_dmg / r.count;
      r.pct = 100.0 * r.count / ( double ) num_direct_results;
      r.count     /= num_iterations;
      r.total_dmg /= num_iterations;
    }
    if ( tick_results[ i ].count != 0 )
    {
      stats_results_t& r = tick_results[ i ];

      r.avg_dmg = r.total_dmg / r.count;
      r.pct = 100.0 * r.count / ( double ) num_tick_results;
      r.count     /= num_iterations;
      r.total_dmg /= num_iterations;
    }
  }

  resource_consumed /= num_iterations;

  num_executes /= num_iterations;
  num_ticks    /= num_iterations;

  num_direct_results /= num_iterations;
  num_tick_results   /= num_iterations;

  rpe = num_executes ? resource_consumed / num_executes : -1;

  double resource_total = player -> resource_lost [ resource ] / num_iterations;

  resource_portion = ( resource_total > 0 ) ? ( resource_consumed / resource_total ) : 0;

  frequency = num_intervals ? total_intervals / num_intervals : 0;

  total_execute_time /= num_iterations;
  total_tick_time    /= num_iterations;
  total_dmg          /= num_iterations;
  opportunity_cost   /= num_iterations;

  compound_dmg = total_dmg - opportunity_cost;

  int num_children = children.size();
  for( int i=0; i < num_children; i++ )
  {
    children[ i ] -> analyze();
    compound_dmg += children[ i ] -> compound_dmg;
  }

  if ( compound_dmg > 0 )
  {
    dpe  = ( num_executes > 0 ) ? ( compound_dmg / num_executes ) : 0;

    double total_time = total_execute_time + total_tick_time;
    dps  = ( total_time > 0 ) ? ( compound_dmg / total_time ) : 0;

    total_time = total_execute_time + ( channeled ? total_tick_time : 0 );
    dpet = ( total_time > 0 ) ? ( compound_dmg / total_time ) : 0;

    dpr  = ( resource_consumed > 0 ) ? ( compound_dmg / resource_consumed ) : 0;
  }

  ttpt = num_ticks ? total_tick_time / num_ticks : 0;
  etpe = num_executes? ( total_execute_time + ( channeled ? total_tick_time : 0 ) ) / num_executes : 0;

  for ( int i=0; i < num_buckets; i++ )
  {
    if( i == ( int ) sim -> divisor_timeline.size() ) break;
    timeline_dmg[ i ] /= sim -> divisor_timeline[ i ];
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
  resource_consumed   += other -> resource_consumed;
  num_direct_results  += other -> num_direct_results;
  num_tick_results    += other -> num_tick_results;
  num_executes        += other -> num_executes;
  num_ticks           += other -> num_ticks;
  total_execute_time  += other -> total_execute_time;
  total_tick_time     += other -> total_tick_time;
  total_dmg           += other -> total_dmg;
  opportunity_cost    += other -> opportunity_cost;

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    if ( other -> direct_results[ i ].count != 0 )
    {
      stats_results_t& other_r = other -> direct_results[ i ];
      stats_results_t&       r =          direct_results[ i ];

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

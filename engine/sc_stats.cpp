// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// stats_t::stats_t =========================================================

stats_t::stats_t( const std::string& n, player_t* p ) :
  name_str( n ), sim( p->sim ), player( p ), next( 0 ), parent( 0 ),
  school( SCHOOL_NONE ), type( STATS_DMG ), analyzed( false ),
  initialized( false ), quiet( false ), background( true ), resource( RESOURCE_NONE ),
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
      sim -> errorf( "stats_t %s already has parent %s, can't parent to %s",
                     child -> name_str.c_str(), child -> parent -> name_str.c_str(), name_str.c_str() );
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

  int num_buckets = ( int ) sim -> max_time;
  if ( num_buckets == 0 ) num_buckets = 600; // Default to 10 minutes
  num_buckets *= 2;

  timeline_amount.reserve( num_buckets );

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    direct_results[ i ].count     = 0;
    direct_results[ i ].min_amount   = FLT_MAX;
    direct_results[ i ].max_amount   = 0;
    direct_results[ i ].avg_amount   = 0;
    direct_results[ i ].total_amount = 0;
    direct_results[ i ].pct       = 0;

    tick_results[ i ].count     = 0;
    tick_results[ i ].min_amount   = FLT_MAX;
    tick_results[ i ].max_amount   = 0;
    tick_results[ i ].avg_amount   = 0;
    tick_results[ i ].total_amount = 0;
    tick_results[ i ].pct       = 0;
  }

  num_direct_results = num_tick_results = 0;
  num_executes = num_ticks = 0;
  total_execute_time = total_tick_time = total_time = 0;
  total_amount = portion_amount = compound_amount = opportunity_cost = 0;
  aps = ape = apet = apr = etpe = ttpt = 0;
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
  total_amount += amount;

  if ( type == STATS_DMG )
    player -> iteration_dmg += amount;
  else
    player -> iteration_heal += amount;

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
  r -> total_amount += amount;

  if ( amount < r -> min_amount ) r -> min_amount = amount;
  if ( amount > r -> max_amount ) r -> max_amount = amount;

  int index = ( int ) ( sim -> current_time );
  if ( index >= ( int ) timeline_amount.size() )
    timeline_amount.resize( index * 2 );

  timeline_amount[ index ] += amount;
}

// stats_t::add_execute =====================================================

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

// stats_t::add_tick ========================================================

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
    if ( ! a -> background ) background = false;
  }

  int num_iterations = sim -> iterations;

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    if ( direct_results[ i ].count != 0 )
    {
      stats_results_t& r = direct_results[ i ];

      r.avg_amount = r.total_amount / r.count;
      r.pct = 100.0 * r.count / ( double ) num_direct_results;
      r.count     /= num_iterations;
      r.total_amount /= num_iterations;
    }
    if ( tick_results[ i ].count != 0 )
    {
      stats_results_t& r = tick_results[ i ];

      r.avg_amount = r.total_amount / r.count;
      r.pct = 100.0 * r.count / ( double ) num_tick_results;
      r.count     /= num_iterations;
      r.total_amount /= num_iterations;
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
  total_amount          /= num_iterations;
  opportunity_cost   /= num_iterations;

  compound_amount = total_amount - opportunity_cost;

  int num_children = children.size();
  for( int i=0; i < num_children; i++ )
  {
    children[ i ] -> analyze();
    compound_amount += children[ i ] -> compound_amount;
  }

  if ( compound_amount > 0 )
  {
    ape  = ( num_executes > 0 ) ? ( compound_amount / num_executes ) : 0;

    total_time = total_execute_time + total_tick_time;
    aps  = ( total_time > 0 ) ? ( compound_amount / total_time ) : 0;

    total_time = total_execute_time + ( channeled ? total_tick_time : 0 );
    apet = ( total_time > 0 ) ? ( compound_amount / total_time ) : 0;

    apr  = ( resource_consumed > 0 ) ? ( compound_amount / resource_consumed ) : 0;
  }
  else
    total_time = total_execute_time + ( channeled ? total_tick_time : 0 );

  ttpt = num_ticks ? total_tick_time / num_ticks : 0;
  etpe = num_executes? ( total_execute_time + ( channeled ? total_tick_time : 0 ) ) / num_executes : 0;

  int num_buckets = ( int ) timeline_amount.size();
  int max_buckets = std::min( num_buckets, ( int ) sim -> divisor_timeline.size() );
  for ( int i=0; i < max_buckets; i++ )
    timeline_amount[ i ] /= sim -> divisor_timeline[ i ];
}

// stats_t::merge ===========================================================

inline void stats_t::stats_results_t::merge( const stats_results_t& other )
{
  if ( other.count != 0 )
  {
    count        += other.count;
    total_amount += other.total_amount;

    if ( other.min_amount < min_amount )
      min_amount = other.min_amount;

    if ( other.max_amount > max_amount )
      max_amount = other.max_amount;
  }
}

void stats_t::merge( const stats_t* other )
{
  resource_consumed   += other -> resource_consumed;
  num_direct_results  += other -> num_direct_results;
  num_tick_results    += other -> num_tick_results;
  num_executes        += other -> num_executes;
  num_ticks           += other -> num_ticks;
  total_execute_time  += other -> total_execute_time;
  total_tick_time     += other -> total_tick_time;
  total_amount        += other -> total_amount;
  opportunity_cost    += other -> opportunity_cost;

  for ( int i=0; i < RESULT_MAX; i++ )
    direct_results[ i ].merge( other -> direct_results[ i ] );

  for ( int i=0; i < RESULT_MAX; i++ )
    tick_results[ i ].merge( other -> tick_results[ i ] );

  int i_max = ( int ) std::min( timeline_amount.size(), other -> timeline_amount.size() );
  for ( int i=0; i < i_max; i++ )
    timeline_amount[ i ] += other -> timeline_amount[ i ];
}

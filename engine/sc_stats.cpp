// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// stats_t::stats_t =========================================================

stats_t::stats_t( const std::string& n, player_t* p ) :
  name_str( n ), sim( p -> sim ), player( p ), next( 0 ), parent( 0 ),
  school( SCHOOL_NONE ), type( STATS_DMG ),
  analyzed( false ), quiet( false ), background( true ),
  resource( RESOURCE_NONE ), resource_consumed( 0 ), /* resource_portion( 0 ), */
  /* frequency( 0 ), */ num_executes( 0 ), num_ticks( 0 ),
  num_direct_results( 0 ), num_tick_results( 0 ),
  total_execute_time( timespan_t::zero ), total_tick_time( timespan_t::zero ), total_time( timespan_t::zero ),
  portion_amount( 0 ),
  aps( 0 ), ape( 0 ), apet( 0 ), apr( 0 ),
  /* rpe( 0 ), */ etpe( 0 ), ttpt( 0 ),
  total_intervals( timespan_t::zero ), num_intervals( 0 ),
  last_execute( timespan_t::min ),
  iteration_actual_amount( 0 ), actual_amount( p -> sim -> statistics_level < 3 ),
  total_amount( p -> sim -> statistics_level < 3 ),portion_aps( p -> sim -> statistics_level < 3 ),
  compound_actual( 0 ), compound_amount( 0 ), opportunity_cost( 0 ),
  direct_results( RESULT_MAX, stats_results_t( p -> sim ) ), tick_results( RESULT_MAX, stats_results_t( p -> sim ) )
{
  int size = ( int ) ( sim -> max_time.total_seconds() * ( 1.0 + sim -> vary_combat_length ) );
  if ( size <= 0 )size = 600; // Default to 10 minutes
  size *= 2;
  size += 3; // Buffer against rounding.

  timeline_amount.assign( size, 0 );
  actual_amount.reserve( sim -> iterations );
  total_amount.reserve( sim -> iterations );
  portion_aps.reserve( sim -> iterations );
}

// stats_t::add_child =======================================================

void stats_t::add_child( stats_t* child )
{
  if ( child -> parent )
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

// stats_t::reset ===========================================================

void stats_t::reset()
{
  last_execute = timespan_t::min;
}

// stats_t::add_result ======================================================

void stats_t::add_result( double act_amount,
                          double tot_amount,
                          int    dmg_type,
                          int    result )
{
  iteration_actual_amount += act_amount;
  iteration_total_amount += tot_amount;

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

  r -> iteration_count += 1;
  r -> actual_amount.add( act_amount );
  r -> total_amount.add( tot_amount );
  r -> iteration_actual_amount += act_amount;
  r -> iteration_total_amount += tot_amount;

  int index = ( int ) ( sim -> current_time.total_seconds() );


  timeline_amount[ index ] += act_amount;
}

// stats_t::add_execute =====================================================

void stats_t::add_execute( timespan_t time )
{
  num_executes++;
  total_execute_time += time;

  if ( likely( last_execute > timespan_t::zero &&
               last_execute != sim -> current_time ) )
  {
    num_intervals++;
    total_intervals += sim -> current_time - last_execute;
  }
  last_execute = sim -> current_time;
}

// stats_t::add_tick ========================================================

void stats_t::add_tick( timespan_t time )
{
  num_ticks++;
  total_tick_time += time;
}

// stats_t::combat_begin ========================================================

void stats_t::combat_begin()
{

  iteration_actual_amount = 0;
  iteration_total_amount = 0;
}

// stats_t::combat_end ========================================================

void stats_t::combat_end()
{
  actual_amount.add( iteration_actual_amount );
  total_amount.add( iteration_total_amount );

  if ( type == STATS_DMG )
    player -> iteration_dmg += iteration_actual_amount;
  else
    player -> iteration_heal += iteration_actual_amount;

  for ( size_t i=0; i < children.size(); i++ )
  {
    iteration_actual_amount += children[ i ] -> iteration_actual_amount;
  }
  portion_aps.add( player -> iteration_fight_length != timespan_t::zero ? iteration_actual_amount / player -> iteration_fight_length.total_seconds() : 0 );

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    direct_results[ i ].combat_end();
    tick_results[ i ].combat_end();
  }
}

// stats_t::analyze =========================================================

void stats_t::analyze()
{
  if ( analyzed ) return;
  analyzed = true;

  bool channeled = false;
  size_t num_actions = action_list.size();
  for ( size_t i=0; i < num_actions; i++ )
  {
    action_t* a = action_list[ i ];
    if ( a -> channeled ) channeled = true;
    school   = a -> school;
    resource = a -> resource;
    if ( ! a -> background ) background = false;
  }

  int num_iterations = sim -> iterations;


  num_direct_results /= num_iterations;
  num_tick_results   /= num_iterations;

  for ( int i=0; i < RESULT_MAX; i++ )
  {
    direct_results[ i ].count.analyze();
    if ( direct_results[ i ].count.mean != 0 )
    {
      stats_results_t& r = direct_results[ i ];

      r.avg_actual_amount.analyze();
      r.count.analyze();
      r.pct = 100.0 * r.count.mean / ( double ) num_direct_results;
      r.fight_total_amount.analyze();
      r.fight_actual_amount.analyze();
      r.actual_amount.analyze();
      r.total_amount.analyze();
      r.overkill_pct = r.fight_total_amount.mean ? 100.0 * ( r.fight_total_amount.mean - r.fight_actual_amount.mean ) / r.fight_total_amount.mean : 0;
    }

    tick_results[ i ].count.analyze();
    if ( tick_results[ i ].count.mean != 0 )
    {
      stats_results_t& r = tick_results[ i ];

      r.avg_actual_amount.analyze();
      r.pct = 100.0 * r.count.mean / ( double ) num_tick_results;
      r.fight_total_amount.analyze();
      r.fight_actual_amount.analyze();
      r.actual_amount.analyze();
      r.total_amount.analyze();
      r.overkill_pct = r.fight_total_amount.mean ? 100.0 * ( r.fight_total_amount.mean - r.fight_actual_amount.mean ) / r.fight_total_amount.mean : 0;
    }
  }

  portion_aps.analyze( true, true, true, 50 );

  resource_consumed  /= num_iterations;

  num_executes       /= num_iterations;
  num_ticks          /= num_iterations;

  rpe = num_executes ? resource_consumed / num_executes : -1;

  double resource_total = player -> resource_lost [ resource ] / num_iterations;

  resource_portion = ( resource_total > 0 ) ? ( resource_consumed / resource_total ) : 0;

  frequency = num_intervals ? total_intervals.total_seconds() / num_intervals : 0;

  total_execute_time /= num_iterations;
  total_tick_time    /= num_iterations;
  total_amount.analyze();
  actual_amount.analyze();
  opportunity_cost   /= num_iterations;

  compound_amount = actual_amount.mean - opportunity_cost;

  size_t num_children = children.size();
  for ( size_t i=0; i < num_children; i++ )
  {
    children[ i ] -> analyze();
    compound_amount += children[ i ] -> compound_amount;
  }

  if ( compound_amount > 0 )
  {
    overkill_pct = total_amount.mean ? 100.0 * ( total_amount.mean - actual_amount.mean ) / total_amount.mean : 0;
    ape  = ( num_executes > 0 ) ? ( compound_amount / num_executes ) : 0;

    total_time = total_execute_time + total_tick_time;
    aps  = ( total_time > timespan_t::zero ) ? ( compound_amount / total_time.total_seconds() ) : 0;

    total_time = total_execute_time + ( channeled ? total_tick_time : timespan_t::zero );
    apet = ( total_time > timespan_t::zero ) ? ( compound_amount / total_time.total_seconds() ) : 0;

    apr  = ( resource_consumed > 0 ) ? ( compound_amount / resource_consumed ) : 0;
  }
  else
    total_time = total_execute_time + ( channeled ? total_tick_time : timespan_t::zero );

  ttpt = num_ticks ? total_tick_time.total_seconds() / num_ticks : 0;
  etpe = num_executes? ( total_execute_time.total_seconds() + ( channeled ? total_tick_time.total_seconds() : 0 ) ) / num_executes : 0;

  int num_buckets = ( int ) timeline_amount.size();
  int max_buckets = std::min( num_buckets, ( int ) sim -> divisor_timeline.size() );
  for ( int i=0; i < max_buckets; i++ )
    timeline_amount[ i ] /= sim -> divisor_timeline[ i ];
}

// stats_results_t::merge ===========================================================

stats_t::stats_results_t::stats_results_t( sim_t* s ) :
  actual_amount( s -> statistics_level < 8, true ),
  total_amount(),
  fight_actual_amount(),
  fight_total_amount(),
  count( s -> statistics_level < 8 ),
  avg_actual_amount( s -> statistics_level < 8, true ),
  iteration_count( 0 ),
  iteration_actual_amount( 0 ),
  iteration_total_amount( 0 ),
  pct( 0 ),
  overkill_pct( 0 )
{
  // Keep non hidden reported numbers clean
  count.mean = 0;
  actual_amount.mean = 0; actual_amount.max=0;
  avg_actual_amount.mean = 0;

  actual_amount.reserve( s -> iterations );
  total_amount.reserve( s -> iterations );
  fight_actual_amount.reserve( s -> iterations );
  fight_total_amount.reserve( s -> iterations );
  count.reserve( s -> iterations );
  avg_actual_amount.reserve( s -> iterations );
}

// stats_results_t::merge ===========================================================

inline void stats_t::stats_results_t::merge( const stats_results_t& other )
{
  count.merge( other.count );
  fight_total_amount.merge( other.fight_total_amount );
  fight_actual_amount.merge( other.fight_actual_amount );
  avg_actual_amount.merge( other.avg_actual_amount );
  actual_amount.merge( other.actual_amount );
  total_amount.merge( other.total_amount );
}

// stats_results_t::combat_end ===========================================================

inline void stats_t::stats_results_t::combat_end()
{
  avg_actual_amount.add( iteration_count ? iteration_actual_amount / iteration_count : 0 );
  count.add( iteration_count );
  fight_actual_amount.add( iteration_actual_amount );
  fight_total_amount.add(  iteration_total_amount );

  iteration_count = 0;
  iteration_actual_amount = 0;
  iteration_total_amount = 0;
}

// stats_t::merge ===========================================================

void stats_t::merge( const stats_t* other )
{
  resource_consumed   += other -> resource_consumed;
  num_direct_results  += other -> num_direct_results;
  num_tick_results    += other -> num_tick_results;
  num_executes        += other -> num_executes;
  num_ticks           += other -> num_ticks;
  total_execute_time  += other -> total_execute_time;
  total_tick_time     += other -> total_tick_time;
  opportunity_cost    += other -> opportunity_cost;

  total_amount.merge( other -> total_amount );
  actual_amount.merge( other -> actual_amount );
  portion_aps.merge( other -> portion_aps );

  for ( int i=0; i < RESULT_MAX; i++ )
    direct_results[ i ].merge( other -> direct_results[ i ] );

  for ( int i=0; i < RESULT_MAX; i++ )
    tick_results[ i ].merge( other -> tick_results[ i ] );

  int i_max = ( int ) std::min( timeline_amount.size(), other -> timeline_amount.size() );
  for ( int i=0; i < i_max; i++ )
    timeline_amount[ i ] += other -> timeline_amount[ i ];
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// stats_t::stats_t =========================================================

stats_t::stats_t( const std::string& n, player_t* p ) :
  sim( *( p -> sim ) ),
  name_str( n ),
  player( p ),
  parent( nullptr ),
  school( SCHOOL_NONE ),
  type( STATS_DMG ),
  resource_gain( n ),
  analyzed( false ),
  quiet( false ),
  background( true ),
  // Variables used both during combat and for reporting
  num_executes(), num_ticks(), num_refreshes(),
  num_direct_results(), num_tick_results(),
  iteration_num_executes( 0 ), iteration_num_ticks( 0 ), iteration_num_refreshes( 0 ),
  total_execute_time(), total_tick_time(),
  iteration_total_execute_time( timespan_t::zero() ),
  iteration_total_tick_time( timespan_t::zero() ),
  portion_amount( 0 ),
  total_intervals(),
  last_execute( timespan_t::min() ),
  actual_amount( name_str + " Actual Amount", p -> sim -> statistics_level < 3 ),
  total_amount( name_str + " Total Amount", p -> sim -> statistics_level < 3 ),
  portion_aps( name_str + " Portion APS", p -> sim -> statistics_level < 3 ),
  portion_apse( name_str + " Portion APSe", p -> sim -> statistics_level < 3 ),
  direct_results( RESULT_MAX ),
  blocked_direct_results( BLOCK_RESULT_MAX ),
  tick_results( RESULT_MAX ),
  blocked_tick_results( BLOCK_RESULT_MAX ),
  // Reporting only
  resource_portion(), apr(), rpe(),
  rpe_sum( 0 ), compound_amount( 0 ), overkill_pct( 0 ),
  aps( 0 ), ape( 0 ), apet( 0 ), etpe( 0 ), ttpt( 0 ),
  total_time( timespan_t::zero() )
{
  actual_amount.reserve( sim.iterations );
  total_amount.reserve( sim.iterations );
  portion_aps.reserve( sim.iterations );
  portion_apse.reserve( sim.iterations );
}

// stats_t::add_child =======================================================

void stats_t::add_child( stats_t* child )
{
  if ( child -> parent )
  {
#ifndef NDEBUG
    if ( child -> parent != this )
    {
      sim.errorf( "stats_t %s already has parent %s, can't parent to %s",
                  child -> name_str.c_str(), child -> parent -> name_str.c_str(), name_str.c_str() );
      assert( 0 );
    }
#endif
    return;
  }
  child -> parent = this;
  children.push_back( child );
}

void stats_t::consume_resource( resource_e resource_type, double resource_amount )
{
  resource_gain.add( resource_type, resource_amount );
}

// stats_t::reset ===========================================================

void stats_t::reset()
{
  last_execute = timespan_t::min();
}

// stats_t::add_result ======================================================

void stats_t::add_result( double act_amount,
                          double tot_amount,
                          dmg_e dmg_type,
                          result_e result,
                          block_result_e block_result,
                          player_t* /* target */ )
{
  stats_results_t* r = 0;
  if ( dmg_type == DMG_DIRECT || dmg_type == HEAL_DIRECT || dmg_type == ABSORB )
    r = &( direct_results[ result ] );
  else
    r = &( tick_results[ result ] );

  r -> iteration_count += 1;
  r -> iteration_actual_amount += act_amount;
  r -> iteration_total_amount += tot_amount;
  r -> actual_amount.add( act_amount );
  r -> total_amount.add( tot_amount );

  stats_results_t* b = 0;
  if ( dmg_type == DMG_DIRECT || dmg_type == HEAL_DIRECT || dmg_type == ABSORB )
    b = &( blocked_direct_results[ block_result ] );
  else
    b = &( blocked_tick_results[ block_result ] );

  b -> iteration_count += 1;
  b -> iteration_actual_amount += act_amount;
  b -> iteration_total_amount += tot_amount;
  b -> actual_amount.add( act_amount );
  b -> total_amount.add( tot_amount );

  timeline_amount.add( sim.current_time, act_amount );
}

// stats_t::add_execute =====================================================

void stats_t::add_execute( timespan_t time,
                           player_t* /* target */ )
{
  iteration_num_executes++;
  iteration_total_execute_time += time;

  if ( likely( last_execute > timespan_t::zero() &&
               last_execute != sim.current_time ) )
  {
    total_intervals.add( sim.current_time.total_seconds() - last_execute.total_seconds() );
  }
  last_execute = sim.current_time;
}

// stats_t::add_tick ========================================================

void stats_t::add_tick( timespan_t time,
                        player_t* /* target */ )
{
  iteration_num_ticks++;
  iteration_total_tick_time += time;
}

// stats_t::add_refresh =====================================================

void stats_t::add_refresh( player_t* /* target */ )
{
  iteration_num_refreshes++;
}

// stats_t::datacollection_begin ============================================

void stats_t::datacollection_begin()
{
  iteration_num_executes = 0;
  iteration_num_ticks = 0;
  iteration_num_refreshes = 0;
  iteration_total_execute_time = timespan_t::zero();
  iteration_total_tick_time = timespan_t::zero();

  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    direct_results[ i ].datacollection_begin();
    tick_results[ i ].datacollection_begin();
  }
  for ( block_result_e i = BLOCK_RESULT_UNBLOCKED; i < BLOCK_RESULT_MAX; i++ )
  {
    blocked_direct_results[ i ].datacollection_begin();
    blocked_tick_results[ i ].datacollection_begin();
  }
}

// stats_t::datacollection_end ==============================================

void stats_t::datacollection_end()
{
  double iaa = 0;
  double ita = 0;
  double idr = 0;
  double itr = 0;

  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    idr += direct_results[ i ].iteration_count;
    itr += tick_results[ i ].iteration_count;
    iaa += direct_results[ i ].iteration_actual_amount + tick_results[ i ].iteration_actual_amount;
    ita += direct_results[ i ].iteration_total_amount + tick_results[ i ].iteration_total_amount;

    direct_results[ i ].datacollection_end();
    tick_results[ i ].datacollection_end();
  }
  for ( block_result_e i = BLOCK_RESULT_UNBLOCKED; i < BLOCK_RESULT_MAX; i++ )
  {
    blocked_direct_results[ i ].datacollection_end();
    blocked_tick_results[ i ].datacollection_end();
  }

  actual_amount.add( iaa );
  total_amount.add( ita );

  total_execute_time.add( iteration_total_execute_time.total_seconds() );
  total_tick_time.add( iteration_total_tick_time.total_seconds() );

  if ( type == STATS_DMG )
    player -> iteration_dmg += iaa;
  else if ( type == STATS_HEAL || type == STATS_ABSORB )
    player -> iteration_heal += iaa;

  portion_aps.add( player -> iteration_fight_length != timespan_t::zero() ? iaa / player -> iteration_fight_length.total_seconds() : 0 );
  portion_apse.add( sim.current_time != timespan_t::zero() ? iaa / sim.current_time.total_seconds() : 0 );


  num_executes.add( iteration_num_executes );
  num_ticks.add( iteration_num_ticks );
  num_refreshes.add( iteration_num_refreshes );
  num_direct_results.add( idr );
  num_tick_results.add( itr );
}

// stats_t::analyze =========================================================

void stats_t::analyze()
{
  if ( analyzed ) return;
  analyzed = true;

  bool channeled = false;
  for ( size_t i = 0; i < action_list.size(); i++ )
  {
    action_t& a = *action_list[ i ];
    if ( a.channeled ) channeled = true;
    if ( ! a.background ) background = false;
  }

  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    direct_results[ i ].analyze( num_direct_results.mean() );
    tick_results[ i ].analyze( num_tick_results.mean() );
  }

  for ( block_result_e i = BLOCK_RESULT_UNBLOCKED; i < BLOCK_RESULT_MAX; i++ )
  {
    blocked_direct_results[ i ].analyze( num_direct_results.mean() );
    blocked_tick_results[ i ].analyze( num_tick_results.mean() );
  }

  portion_aps.analyze_all();
  portion_apse.analyze_all();

  resource_gain.analyze( sim );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    rpe[ i ] = num_executes.mean() ? resource_gain.actual[ i ] / num_executes.mean() : -1;
    rpe_sum += rpe[ i ];

    double resource_total = player -> resource_lost [ i ] / sim.iterations;

    resource_portion[ i ] = ( resource_total > 0 ) ? ( resource_gain.actual[ i ] / resource_total ) : 0;
  }

  total_amount.analyze_all();
  actual_amount.analyze_all();

  compound_amount = actual_amount.count() ? actual_amount.mean() : 0.0;

  for ( size_t i = 0; i < children.size(); i++ )
  {
    children[ i ] -> analyze();
    compound_amount += children[ i ] -> compound_amount;
  }

  if ( compound_amount > 0 )
  {
    overkill_pct = total_amount.mean() ? 100.0 * ( total_amount.mean() - actual_amount.mean() ) / total_amount.mean() : 0;
    ape  = ( num_executes.mean() > 0 ) ? ( compound_amount / num_executes.mean() ) : 0;

    total_time = timespan_t::from_seconds( total_execute_time.mean() + total_tick_time.mean() );
    aps  = ( total_time > timespan_t::zero() ) ? ( compound_amount / total_time.total_seconds() ) : 0;

    total_time = timespan_t::from_seconds( total_execute_time.mean() + ( channeled ? total_tick_time.mean() : 0 ) );
    apet = ( total_time > timespan_t::zero() ) ? ( compound_amount / total_time.total_seconds() ) : 0;

    for ( size_t i = 0; i < RESOURCE_MAX; i++ )
      apr[ i ]  = ( resource_gain.actual[ i ] > 0 ) ? ( compound_amount / resource_gain.actual[ i ] ) : 0;
  }
  else
  {
    total_time = timespan_t::from_seconds( total_execute_time.mean() + ( channeled ? total_tick_time.mean() : 0 ) );
  }

  ttpt = num_ticks.mean() ? total_tick_time.mean() / num_ticks.mean() : 0.0;
  etpe = num_executes.mean() ? ( total_execute_time.mean() + ( channeled ? total_tick_time.mean() : 0.0 ) ) / num_executes.mean() : 0.0;

  timeline_amount.adjust( sim.divisor_timeline );
}

// stats_results_t::merge ===================================================

stats_t::stats_results_t::stats_results_t() :
  actual_amount(),
  avg_actual_amount(),
  total_amount(),
  fight_actual_amount(),
  fight_total_amount(),
  overkill_pct(),
  count(),
  pct( 0 ),
  iteration_count( 0 ),
  iteration_actual_amount( 0 ),
  iteration_total_amount( 0 )
{

}

// stats_results_t::merge ===================================================

void stats_t::stats_results_t::merge( const stats_results_t& other )
{
  count.merge( other.count );
  fight_total_amount.merge( other.fight_total_amount );
  fight_actual_amount.merge( other.fight_actual_amount );
  avg_actual_amount.merge( other.avg_actual_amount );
  actual_amount.merge( other.actual_amount );
  total_amount.merge( other.total_amount );
  overkill_pct.merge( other.overkill_pct );
}
// stats_results_t::datacollection_begin ====================================

void stats_t::stats_results_t::datacollection_begin()
{
  iteration_count = 0;
  iteration_actual_amount = 0.0;
  iteration_total_amount = 0.0;
}

// stats_results_t::combat_end ==============================================

void stats_t::stats_results_t::datacollection_end()
{
  avg_actual_amount.add( iteration_count ? iteration_actual_amount / iteration_count : 0.0 );
  count.add( iteration_count );
  fight_actual_amount.add( iteration_actual_amount );
  fight_total_amount.add(  iteration_total_amount );
  overkill_pct.add( iteration_total_amount ? 100.0 * ( iteration_total_amount - iteration_actual_amount ) / iteration_total_amount : 0.0 );
}

void stats_t::stats_results_t::analyze( double num_results )
{
  pct = num_results ? ( 100.0 * count.mean() / num_results ) : 0.0;
}
// stats_t::merge ===========================================================

void stats_t::merge( const stats_t& other )
{
  resource_gain.merge( other.resource_gain );
  num_direct_results.merge( other.num_direct_results );
  num_tick_results.merge( other.num_tick_results );
  num_executes.merge ( other.num_executes );
  num_ticks.merge( other.num_ticks );
  num_refreshes.merge( other.num_refreshes );
  total_execute_time.merge( other.total_execute_time );
  total_tick_time.merge( other.total_tick_time );

  total_amount.merge( other.total_amount );
  actual_amount.merge( other.actual_amount );
  portion_aps.merge( other.portion_aps );
  portion_apse.merge( other.portion_apse );

  for ( result_e i = RESULT_NONE; i < RESULT_MAX; ++i )
  {
    direct_results[ i ].merge( other.direct_results[ i ] );
    tick_results[ i ].merge( other.tick_results[ i ] );
  }

  for ( block_result_e i = BLOCK_RESULT_UNBLOCKED; i < BLOCK_RESULT_MAX; ++i )
  {
    blocked_direct_results[ i ].merge( other.blocked_direct_results[ i ] );
    blocked_tick_results[ i ].merge( other.blocked_tick_results[ i ] );
  }

  timeline_amount.merge( other.timeline_amount );
}

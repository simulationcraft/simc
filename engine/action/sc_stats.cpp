// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action/sc_action.hpp"
#include "player/stats.hpp"
#include "player/sc_player.hpp"
#include "player/pet.hpp"
#include "sim/sc_sim.hpp"
#include <memory>
#include <unordered_map>

// stats_t::stats_t =========================================================

stats_t::stats_t( util::string_view n, player_t* p ) :
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
  direct_results(),
  tick_results(),
  // Reporting only
  resource_portion(), apr(), rpe(),
  rpe_sum( 0 ), compound_amount( 0 ), overkill_pct( 0 ),
  aps( 0 ), ape( 0 ), apet( 0 ), etpe( 0 ), ttpt( 0 ),
  total_time( timespan_t::zero() ),
  timeline_aps_chart(),
  scaling( nullptr ),
  timeline_amount( nullptr )
{
  int size = std::min( sim.iterations, 10000 );
  actual_amount.reserve( size );
  total_amount.reserve( size );
  portion_aps.reserve( size );
  portion_apse.reserve( size );

  if ( sim.report_details != 0 )
  {
    timeline_amount = std::make_unique<sc_timeline_t>( );
  }
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

full_result_e stats_t::translate_result( result_e result, block_result_e block_result )
{
  full_result_e fulltype = FULLTYPE_NONE;
  switch ( result )
  {
    case RESULT_MISS:   fulltype=FULLTYPE_MISS; break;
    case RESULT_DODGE:  fulltype=FULLTYPE_DODGE; break;
    case RESULT_PARRY:  fulltype=FULLTYPE_PARRY; break;
    case RESULT_GLANCE: fulltype=FULLTYPE_GLANCE; break;
    case RESULT_CRIT:   fulltype=FULLTYPE_CRIT; break;
    case RESULT_HIT:    fulltype=FULLTYPE_HIT; break;
    case RESULT_MAX:    fulltype=FULLTYPE_MAX; break;
    default:            fulltype=FULLTYPE_NONE;
  }

  switch ( result )
  {
    case RESULT_GLANCE:
    case RESULT_CRIT:
    case RESULT_HIT:
    {
      switch ( block_result )
      {
      case BLOCK_RESULT_CRIT_BLOCKED:
        fulltype--;
        SC_FALLTHROUGH;
      case BLOCK_RESULT_BLOCKED:
        fulltype--;
        break;
      default:
        break;
      }
    }
    default: break;
  }
  return fulltype;
}

// stats_t::add_result ======================================================

void stats_t::add_result( double act_amount,
                          double tot_amount,
                          result_amount_type dmg_type,
                          result_e result,
                          block_result_e block_result,
                          player_t* /* target */ )
{
  stats_results_t* r = nullptr;
  if ( dmg_type == result_amount_type::DMG_DIRECT || dmg_type == result_amount_type::HEAL_DIRECT || dmg_type == result_amount_type::ABSORB )
  {
    r = &( direct_results[ translate_result( result, block_result ) ] );
  }
  else
  {
    r = &( tick_results[ result ] );
  }

  r -> iteration_count += 1;
  r -> iteration_actual_amount += act_amount;
  r -> iteration_total_amount += tot_amount;
  r -> actual_amount.add( act_amount );
  r -> total_amount.add( tot_amount );

  // Collect timeline data to stats-specific object if it exists, or to the player's global "damage
  // output" timeline (e.g., when report_details=0).
  if ( timeline_amount )
  {
    timeline_amount -> add( sim.current_time(), act_amount );
  }
  else
  {
    if ( ! player -> is_pet() )
    {
      player -> collected_data.timeline_dmg.add( sim.current_time(), act_amount );
    }
    else if ( player -> is_pet() )
    {
      player -> cast_pet() -> owner -> collected_data.timeline_dmg.add( sim.current_time(), act_amount );
      // If pets get reported separately, collect the damage output to the pet's own timeline as
      // well, for reporting purposes
      if ( sim.report_pets_separately )
      {
        player -> collected_data.timeline_dmg.add( sim.current_time(), act_amount );
      }
    }
  }
}

// stats_t::add_execute =====================================================

void stats_t::add_execute( timespan_t time,
                           player_t* /* target */ )
{
  iteration_num_executes++;
  iteration_total_execute_time += time;

  if ( last_execute > timespan_t::zero() &&
               last_execute != sim.current_time() )
  {
    total_intervals.add( sim.current_time().total_seconds() - last_execute.total_seconds() );
  }
  last_execute = sim.current_time();
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

  range::for_each( direct_results, []( stats_results_t& r ) { r.datacollection_begin(); } );
  range::for_each( tick_results, []( stats_results_t& r ) { r.datacollection_begin(); } );
}

// stats_t::datacollection_end ==============================================

void stats_t::datacollection_end()
{
  double iaa = 0;
  double ita = 0;
  double idr = 0;
  double itr = 0;

  range::for_each( direct_results, [ &idr, &iaa, &ita ]( stats_results_t& r ) {
    idr += r.iteration_count;
    iaa += r.iteration_actual_amount;
    ita += r.iteration_total_amount;

    r.datacollection_end();
  } );

  range::for_each( tick_results, [ &itr, &iaa, &ita ]( stats_results_t& r ) {
    itr += r.iteration_count;
    iaa += r.iteration_actual_amount;
    ita += r.iteration_total_amount;

    r.datacollection_end();
  } );

  actual_amount.add( iaa );
  total_amount.add( ita );

  total_execute_time.add( iteration_total_execute_time.total_seconds() );
  total_tick_time.add( iteration_total_tick_time.total_seconds() );

  if ( type == STATS_DMG )
    player -> iteration_dmg += iaa;
  else if ( type == STATS_HEAL )
    player -> iteration_heal += iaa;
  else if ( type == STATS_ABSORB )
    player -> iteration_absorb += iaa;

  auto uptime = player -> composite_active_time();

  portion_aps.add( uptime != timespan_t::zero() ? iaa / uptime.total_seconds() : 0 );
  portion_apse.add( sim.current_time() != timespan_t::zero() ? iaa / sim.current_time().total_seconds() : 0 );

  num_executes.add( iteration_num_executes );
  num_ticks.add( iteration_num_ticks );
  num_refreshes.add( iteration_num_refreshes );
  num_direct_results.add( idr );
  num_tick_results.add( itr );

  if ( timeline_amount )
  {
    timeline_amount -> add( sim.current_time(), 0.0 );
  }
  else
  {
    player -> collected_data.timeline_dmg.add( sim.current_time(), 0.0 );
  }
}

// stats_t::analyze =========================================================

void stats_t::analyze()
{
  if ( analyzed ) return;
  analyzed = true;

  // When single_actor_batch=1 is used in conjunction with target_error, each actor has run varying
  // number of iterations to finish. The total number of iterations ran for each actor (when
  // single_actor_batch=1) is stored in the actor-collected data structure.
  int iterations = player -> collected_data.total_iterations > 0
                   ? player -> collected_data.total_iterations
                   : sim.iterations;

  bool channeled = false;
  for ( size_t i = 0; i < action_list.size(); i++ )
  {
    action_t& a = *action_list[ i ];
    if ( a.channeled ) channeled = true;
    if ( ! a.background ) background = false;
  }

  range::for_each( direct_results, [ this ]( stats_results_t& r ) {
    r.analyze( num_direct_results.mean() );
  } );

  range::for_each( tick_results, [ this ]( stats_results_t& r ) {
    r.analyze( num_tick_results.mean() );
  } );

  portion_aps.analyze();
  portion_apse.analyze();

  resource_gain.analyze( iterations );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    rpe[ i ] = num_executes.mean() ? resource_gain.actual[ i ] / num_executes.mean() : -1;
    rpe_sum += rpe[ i ];

    double resource_total = player -> iteration_resource_lost [ i ] / iterations;

    resource_portion[ i ] = ( resource_total > 0 ) ? ( resource_gain.actual[ i ] / resource_total ) : 0;
  }

  total_amount.analyze();
  actual_amount.analyze();

  compound_amount = actual_amount.count() ? actual_amount.mean() : 0.0;

  for ( size_t i = 0; i < children.size(); i++ )
  {
    children[ i ] -> analyze();
    if ( type == children[i] -> type )
    {
      compound_amount += children[ i ] -> compound_amount;
    }
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

  if ( timeline_amount )
  {
    if ( ! sim.single_actor_batch )
    {
      timeline_amount -> adjust( sim );
    }
    else
    {
      if ( player -> is_pet() )
      {
        timeline_amount -> adjust( player -> cast_pet() -> owner -> collected_data.fight_length );
      }
      else
      {
        timeline_amount -> adjust( player -> collected_data.fight_length );
      }
    }
  }
}

// stats_results_t::merge ===================================================

stats_t::stats_results_t::stats_results_t() :
  actual_amount(),
  avg_actual_amount(),
  count(),
  total_amount(),
  fight_actual_amount(),
  fight_total_amount(),
  overkill_pct(),
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
    tick_results[ i ].merge( other.tick_results[ i ] );
  }

  for ( full_result_e i = FULLTYPE_NONE; i < FULLTYPE_MAX; i++ )
  {
    direct_results[ i ].merge( other.direct_results[ i ] );
  }

  if ( timeline_amount )
  {
    timeline_amount -> merge( *other.timeline_amount );
  }
}

bool stats_t::has_direct_amount_results() const
{
  return (
      direct_results[ FULLTYPE_HIT ].actual_amount.mean() > 0 ||
      direct_results[ FULLTYPE_HIT_BLOCK ].actual_amount.mean() > 0 ||
      direct_results[ FULLTYPE_HIT_CRITBLOCK ].actual_amount.mean() > 0 ||
      direct_results[ FULLTYPE_GLANCE ].actual_amount.mean() > 0 ||
      direct_results[ FULLTYPE_GLANCE_BLOCK ].actual_amount.mean() > 0 ||
      direct_results[ FULLTYPE_GLANCE_CRITBLOCK ].actual_amount.mean() > 0 ||
      direct_results[ FULLTYPE_CRIT ].actual_amount.mean() > 0 ||
      direct_results[ FULLTYPE_CRIT_BLOCK ].actual_amount.mean() > 0 ||
      direct_results[ FULLTYPE_CRIT_CRITBLOCK ].actual_amount.mean() > 0 );
}

bool stats_t::has_tick_amount_results() const
{
  return (
      tick_results[ RESULT_HIT ].actual_amount.mean() > 0 ||
     tick_results[ RESULT_CRIT ].actual_amount.mean() > 0 );
}

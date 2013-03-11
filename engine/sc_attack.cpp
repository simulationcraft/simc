// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Attack
// ==========================================================================

// attack_t::attack_t =======================================================

// == Attack Constructor ===============
attack_t::attack_t( const std::string&  n,
                    player_t*           p,
                    const spell_data_t* s ) :
  action_t( ACTION_ATTACK, n, p, s ),
  n_results( 0 ), attack_table_sum( 0 )
{
  base_attack_power_multiplier = 1.0;
  crit_bonus = 1.0;

  min_gcd = timespan_t::from_seconds( 1.0 );
  hasted_ticks = false;

  crit_multiplier *= util::crit_multiplier( p -> meta_gem );
}

// attack_t::execute ========================================================

void attack_t::execute()
{
  action_t::execute();

  if ( harmful && callbacks )
  {
    if ( ( execute_state ? execute_state -> result : result ) != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.attack[ execute_state ? execute_state -> result : result ], this );
    }
  }
}

// attack_t::execute_time ===================================================

timespan_t attack_t::execute_time()
{
  if ( base_execute_time == timespan_t::zero() )
    return timespan_t::zero();

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero();

  //sim -> output( "%s execute_time=%f base_execute_time=%f execute_time=%f", name(), base_execute_time * swing_haste(), base_execute_time, swing_haste() );
  return base_execute_time * player -> composite_attack_speed();
}

// attack_t::miss_chance ====================================================

double attack_t::miss_chance( double hit, int delta_level )
{
  double miss = 0.03 + ( delta_level * 0.015 );

  miss -= hit;

  return miss;
}

// attack_t::crit_block_chance ==============================================

double attack_t::crit_block_chance( int /* delta_level */ )
{
  // Tested: Player -> Target, both POSITION_RANGED_FRONT and POSITION_FRONT
  // % is 5%, and not 5% + delta_level * 0.5%.
  // Moved 5% to target_t::composite_tank_block

  // FIXME: Test Target -> Player
  return 0;
}

// attack_t::build_table ====================================================

int attack_t::build_table( std::array<double,RESULT_MAX>& chances,
                           std::array<result_e,RESULT_MAX>& results,
                           double miss_chance, double dodge_chance, 
                           double parry_chance, double glance_chance,
                           double crit_chance )
{
  if ( sim -> debug )
    sim -> output( "attack_t::build_table: %s miss=%.3f dodge=%.3f parry=%.3f glance=%.3f crit=%.3f",
                   name(), miss_chance, dodge_chance, parry_chance, glance_chance, crit_chance );

  assert( crit_chance >= 0 && crit_chance <= 1.0 );

  double limit = 1.0;
  double total = 0;
  int num_results = 0;

  if ( miss_chance > 0 && total < limit )
  {
    total += miss_chance;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_MISS;
    num_results++;
  }
  if ( dodge_chance > 0 && total < limit )
  {
    total += dodge_chance;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_DODGE;
    num_results++;
  }
  if ( parry_chance > 0 && total < limit )
  {
    total += parry_chance;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_PARRY;
    num_results++;
  }
  if ( glance_chance > 0 && total < limit )
  {
    total += glance_chance;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_GLANCE;
    num_results++;
  }
  if ( crit_chance > 0 && total < limit )
  {
    total += crit_chance;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_CRIT;
    num_results++;
  }
  if ( total < 1.0 )
  {
    chances[ num_results ] = 1.0;
    results[ num_results ] = RESULT_HIT;
    num_results++;
  }

  return num_results;
}

// attack_t::calculate_result ===============================================

result_e attack_t::calculate_result( action_state_t* s )
{
  result_e result = RESULT_NONE;

  if ( ! harmful || ! may_hit || ! s -> target ) return RESULT_NONE;

  int delta_level = s -> target -> level - player -> level;
  double miss     = may_miss ? ( miss_chance( composite_hit(), delta_level ) + s -> target -> composite_tank_miss( school ) ) : 0;
  double dodge    = may_dodge ? ( dodge_chance( composite_expertise(), delta_level ) + s -> target -> composite_tank_dodge() ) : 0;
  double parry    = may_parry && player -> position() == POSITION_FRONT ? ( parry_chance( composite_expertise(), delta_level ) + s -> target -> composite_tank_parry() ) : 0;
  double crit     = may_crit ? ( crit_chance( s -> composite_crit() + s -> target -> composite_tank_crit( school ), delta_level ) ) : 0;

  double cur_attack_table_sum = miss + dodge + parry + crit;

  // Only recalculate attack table if the stats have changed
  if ( attack_table_sum != cur_attack_table_sum )
  {
    attack_table_sum = cur_attack_table_sum;
    // Specials are 2-roll calculations, so only pass crit chance to 
    // build_table for non-special attacks
    n_results = build_table( chances, results, miss, dodge, parry, 
                             may_glance ? glance_chance( delta_level ) : 0, 
                             ! special ? std::min( 1.0, crit ) : 0 );
  }

  if ( n_results == 1 )
  {
    result = results[ 0 ];
  }
  else
  {
    // 1-roll attack table with true RNG

    double random = sim -> real();

    for ( int i = 0; i < n_results; i++ )
    {
      if ( random <= chances[ i ] )
      {
        result = results[ i ];
        break;
      }
    }
  }

  assert( result != RESULT_NONE );

  if ( result == RESULT_HIT && special && may_crit ) // Specials are 2-roll calculations
  {
    if ( rng_result -> roll( crit ) )
      result = RESULT_CRIT;
  }

  if ( result == RESULT_HIT && may_block && ( player -> position() == POSITION_FRONT ) ) // Blocks are on their own roll
  {
    double block_total = block_chance( delta_level ) + s -> target -> composite_tank_block();

    double crit_block = crit_block_chance( delta_level ) + s -> target -> composite_tank_crit_block();

    // FIXME: pure assumption on how crit block is handled, needs testing!
    if ( rng_result -> roll( block_total ) )
    {
      if ( rng_result -> roll( crit_block ) )
        result = RESULT_CRIT_BLOCK;
      else
        result = RESULT_BLOCK;
    }
  }

  if ( sim -> debug )
    sim -> output( "%s result for %s is %s", player -> name(), name(), util::result_type_string( result ) );

  return result;
}

void attack_t::init()
{
  action_t::init();

  if ( special )
    may_glance = false;
}

// attack_t::reschedule_auto_attack =========================================

void attack_t::reschedule_auto_attack( double old_swing_haste )
{
  // Note that if attack -> swing_haste() > old_swing_haste, this could
  // probably be handled by rescheduling, but the code is slightly simpler if
  // we just cancel the event and make a new one.
  if ( execute_event && execute_event -> remains() > timespan_t::zero() )
  {
    timespan_t time_to_hit = execute_event -> occurs() - sim -> current_time;
    timespan_t new_time_to_hit = time_to_hit * player -> composite_attack_speed() / old_swing_haste;

    if ( sim -> debug )
    {
      sim_t::output( sim, "Haste change, reschedule %s attack from %f to %f",
                     name(),
                     execute_event -> remains().total_seconds(),
                     new_time_to_hit.total_seconds() );
    }

    if ( new_time_to_hit > time_to_hit )
      execute_event -> reschedule( new_time_to_hit );
    else
    {
      event_t::cancel( execute_event );
      execute_event = start_action_execute_event( new_time_to_hit );
    }
  }
}

// ==========================================================================
// Melee Attack
// ==========================================================================

// melee_attack_t::melee_attack_t =======================================================

// == Melee Attack Constructor ===============

melee_attack_t::melee_attack_t( const std::string&  n,
                                player_t*           p,
                                const spell_data_t* s ) :
  attack_t( n, p, s )
{
  may_miss = may_dodge = may_parry = may_glance = may_block = true;

  // Prevent melee from being scheduled when player is moving
  if ( range < 0 ) range = 5;
}

// melee_attack_t::init ===================================================

void melee_attack_t::init()
{
  attack_t::init();

  if ( special )
    may_glance = false;
}

// melee_attack_t::dodge_chance ===================================================

double melee_attack_t::dodge_chance( double expertise, int delta_level )
{
  return 0.03 + ( delta_level * 0.015 ) - expertise;
}

// melee_attack_t::parry_chance ===================================================

double melee_attack_t::parry_chance( double expertise, int delta_level )
{
  return 0.03 + ( delta_level * 0.015 ) + std::min( 0.0, dodge_chance( expertise, delta_level ) );
}

// melee_attack_t::glance_chance ==================================================

double melee_attack_t::glance_chance( int delta_level )
{
  return ( delta_level + 1 ) * 0.06;
}

// ==========================================================================
// Ranged Attack
// ==========================================================================

// ranged_attack_t::ranged_attack_t =======================================================

// == Ranged Attack Constructor by spell_id_t ===============

ranged_attack_t::ranged_attack_t( const std::string& token,
                                  player_t* p,
                                  const spell_data_t* s ) :
  attack_t( token, p, s )
{

  may_miss  = true;
  may_dodge = true;
}

// ranged_attack_t::dodge_chance ===================================================

double ranged_attack_t::dodge_chance( double expertise, int delta_level )
{
  return 0.03 + ( delta_level * 0.015 ) - expertise;
}

// ranged_attack_t::parry_chance ===================================================

double ranged_attack_t::parry_chance( double /* expertise */, int /* delta_level */ )
{
  // Assumed impossible to parry ranged. Needs checking.
  return 0.0;
}

// ranged_attack_t::glance_chance ==================================================

double ranged_attack_t::glance_chance( int delta_level )
{
  return (  delta_level  + 1 ) * 0.06;
}

// ranged_attack_t::schedule_execute ========================================

void ranged_attack_t::schedule_execute( action_state_t* execute_state )
{
  if ( sim -> log )
  {
    sim -> output( "%s schedules execute for %s (%.0f)", player -> name(), name(),
                   player -> resources.current[ player -> primary_resource() ] );
  }

  time_to_execute = execute_time();

  execute_event = start_action_execute_event( time_to_execute, execute_state );

  if ( ! background )
  {
    player -> executing = this;
    player -> gcd_ready = sim -> current_time + gcd();
    if ( player -> action_queued && sim -> strict_gcd_queue )
    {
      player -> gcd_ready -= sim -> queue_gcd_reduction;
    }
  }
}

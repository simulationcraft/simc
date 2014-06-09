// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Attack
// ==========================================================================

// attack_t::attack_t =======================================================

// == Attack Constructor ====================================================
attack_t::attack_t( const std::string&  n,
                    player_t*           p,
                    const spell_data_t* s ) :
  action_t( ACTION_ATTACK, n, p, s ),
  base_attack_expertise( 0 ),
  auto_attack( false ),
  num_results( 0 ), attack_table_sum( std::numeric_limits<double>::min() )
{
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
    result_e r = execute_state ? execute_state -> result : RESULT_NONE;
    if ( r != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.attack[ execute_state -> result ], this, execute_state );
    }
  }
}

// attack_t::execute_time ===================================================

timespan_t attack_t::execute_time() const
{
  if ( base_execute_time == timespan_t::zero() )
    return timespan_t::zero();

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero();

  //sim -> output( "%s execute_time=%f base_execute_time=%f execute_time=%f", name(), base_execute_time * swing_haste(), base_execute_time, swing_haste() );
  return base_execute_time * player -> cache.attack_speed();
}

// attack_t::miss_chance ====================================================

double attack_t::miss_chance( double hit, player_t* t ) const
{
  // cache.miss() contains the target's miss chance (3.0 base in almost all cases)
  double miss = t -> cache.miss();
  
  // add or subtract 1.5% per level difference
  miss += ( t -> level - player -> level ) * 0.015;

  // asymmetric hit penalty for npcs attacking higher-level players
  if ( ! t -> is_enemy() )
    miss += std::max( t -> level - player -> level - 3, 0 ) * 0.015;

  // subtract the player's hit chance
  miss -= hit;

  return miss;
}

// attack_t::dodge_chance ===================================================

double attack_t::dodge_chance( double expertise, player_t* t ) const
{
  // cache.dodge() contains the target's dodge chance (3.0 base, plus spec bonuses and rating)
  double dodge = t -> cache.dodge();

  // WoD mechanics are unchanged from MoP
  // add or subtract 1.5% per level difference
  dodge += ( t -> level - player -> level ) * 0.015; 

  // subtract the player's expertise chance
  dodge -= expertise;


  return dodge;
}

double attack_t::block_chance( action_state_t* s ) const
{
  // cache.block() contains the target's block chance (3.0 base for bosses, more for shield tanks)
  double block = s -> target -> cache.block();

  // add or subtract 1.5% per level difference
  block += ( s -> target -> level - player -> level ) * 0.015;

  return block;
}

// attack_t::crit_block_chance ==============================================

double attack_t::crit_block_chance( action_state_t* s ) const
{
  // This function is probably unnecessary, as we could just query cache.crit_block() directly.
  // I'm leaving it for consistency with *_chance() and in case future changes modify crit block mechanics

  // Crit Block does not suffer from level-based suppression, return cached value directly
  return s -> target -> cache.crit_block();
}

// attack_t::build_table ====================================================

void attack_t::build_table( double miss_chance, double dodge_chance,
                            double parry_chance, double glance_chance,
                            double crit_chance )
{
  if ( sim -> debug )
    sim -> out_debug.printf( "attack_t::build_table: %s miss=%.3f dodge=%.3f parry=%.3f glance=%.3f crit=%.3f",
                   name(), miss_chance, dodge_chance, parry_chance, glance_chance, crit_chance );

  assert( crit_chance >= 0 && crit_chance <= 1.0 );

  double sum = miss_chance + dodge_chance + parry_chance + crit_chance;

  // Only recalculate attack table if the stats have changed
  if ( attack_table_sum == sum ) return;
  attack_table_sum = sum;

  double limit = 1.0;
  double total = 0;

  num_results = 0;

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
}

// attack_t::calculate_result ===============================================

result_e attack_t::calculate_result( action_state_t* s )
{
  result_e result = RESULT_NONE;

  if ( ! harmful || ! may_hit || ! s -> target ) return RESULT_NONE;

  int delta_level = s -> target -> level - player -> level;

  double miss     = may_miss ? miss_chance( composite_hit(), s -> target ) : 0;
  double dodge    = may_dodge ? dodge_chance( composite_expertise(), s -> target ) : 0;
  double parry    = may_parry && player -> position() == POSITION_FRONT ? parry_chance( composite_expertise(), s -> target ) : 0;
  double crit     = may_crit ? std::max( s -> composite_crit() + s -> target -> cache.crit_avoidance(), 0.0 ) : 0;

  // Specials are 2-roll calculations, so only pass crit chance to
  // build_table for non-special attacks

  build_table( miss, dodge, parry,
               may_glance ? glance_chance( delta_level ) : 0,
               ! special ? std::min( 1.0, crit ) : 0 );

  if ( num_results == 1 )
  {
    result = results[ 0 ];
  }
  else
  {
    // 1-roll attack table with true RNG

    double random = rng().real();

    for ( int i = 0; i < num_results; i++ )
    {
      if ( random <= chances[ i ] )
      {
        result = results[ i ];
        break;
      }
    }
  }

  assert( result != RESULT_NONE );

  // if we have a special, make a second roll for hit/crit
  if ( result == RESULT_HIT && special && may_crit )
  {
    if ( rng().roll( crit ) )
      result = RESULT_CRIT;
  }

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
    timespan_t new_time_to_hit = time_to_hit * player -> cache.attack_speed() / old_swing_haste;

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "Haste change, reschedule %s attack from %f to %f",
                     name(),
                     execute_event -> remains().total_seconds(),
                     new_time_to_hit.total_seconds() );
    }

    if ( new_time_to_hit > time_to_hit )
      execute_event -> reschedule( new_time_to_hit );
    else
    {
      core_event_t::cancel( execute_event );
      execute_event = start_action_execute_event( new_time_to_hit );
    }
  }
}

// ==========================================================================
// Melee Attack
// ==========================================================================

// melee_attack_t::melee_attack_t ===========================================

// == Melee Attack Constructor ==============================================

melee_attack_t::melee_attack_t( const std::string&  n,
                                player_t*           p,
                                const spell_data_t* s ) :
  attack_t( n, p, s )
{
  may_miss = may_dodge = may_parry = may_glance = may_block = true;

  // Prevent melee from being scheduled when player is moving
  if ( range < 0 ) range = 5;
}

// melee_attack_t::init =====================================================

void melee_attack_t::init()
{
  attack_t::init();

  if ( special )
    may_glance = false;
}

// melee_attack_t::parry_chance =============================================

double melee_attack_t::parry_chance( double expertise, player_t* t ) const
{
  // cache.parry() contains the target's parry chance (3.0 base, plus spec bonuses and rating)
  double parry = t -> cache.parry();
  
  // WoD mechanics are similar to MoP
  // add or subtract 1.5% per level difference
  parry += ( t -> level - player -> level ) * 0.015; 

  // 3% additional parry for attacking a level+3 or higher NPC
  if ( t -> is_enemy() && ( t -> level - player -> level ) > 2 )
    parry += 0.03;

  // subtract the player's expertise chance - no longer depends on dodge
  parry -= expertise;

  return parry;
}

// melee_attack_t::glance_chance ============================================

double melee_attack_t::glance_chance( int delta_level ) const
{
  double glance = 0;

  // TODO-WOD: Glance chance increase per 4+ level delta?
  if ( delta_level > 3 )
  {
    glance += 0.10 + 0.10 * delta_level;
  }

  return glance;
}

proc_types melee_attack_t::proc_type() const
{
  if ( ! is_aoe() )
  {
    if ( special )
      return PROC1_MELEE_ABILITY;
    else
      return PROC1_MELEE;
  }
  else
  {
    // "Fake" AOE based attacks as spells
    if ( special )
      return PROC1_AOE_SPELL;
    // AOE white attacks shouldn't really happen ..
    else
      return PROC1_MELEE;
  }
}
// ==========================================================================
// Ranged Attack
// ==========================================================================

// ranged_attack_t::ranged_attack_t =========================================

// == Ranged Attack Constructor by spell_id_t ===============================

ranged_attack_t::ranged_attack_t( const std::string& token,
                                  player_t* p,
                                  const spell_data_t* s ) :
  attack_t( token, p, s )
{

  may_miss  = true;
  may_dodge = true;
}

// Ranged attacks are identical to melee attacks, but cannot be parried or dodged.
// all of the inherited *_chance() methods are accurate.

double ranged_attack_t::composite_target_multiplier( player_t* target ) const
{
  double v = attack_t::composite_target_multiplier( target );

  return v;
}

// ranged_attack_t::schedule_execute ========================================

void ranged_attack_t::schedule_execute( action_state_t* execute_state )
{
  if ( sim -> log )
  {
    sim -> out_log.printf( "%s schedules execute for %s (%.0f)", player -> name(), name(),
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

proc_types ranged_attack_t::proc_type() const
{
  if ( ! is_aoe() )
  {
    if ( special )
      return PROC1_RANGED_ABILITY;
    else
      return PROC1_RANGED;
  }
  else
  {
    // "Fake" AOE based attacks as spells
    if ( special )
      return PROC1_AOE_SPELL;
    // AOE white attacks shouldn't really happen ..
    else
      return PROC1_RANGED;
  }
}

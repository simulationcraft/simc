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
                    const spell_data_t* s,
                    school_type_e       school ) :
  action_t( ACTION_ATTACK, n, p, s, school )
{
  base_attack_power_multiplier = 1.0;
  crit_bonus = 1.0;

  min_gcd = timespan_t::from_seconds( 1.0 );
  hasted_ticks = false;

  if ( p -> meta_gem == META_AGILE_SHADOWSPIRIT         ||
       p -> meta_gem == META_BURNING_SHADOWSPIRIT       ||
       p -> meta_gem == META_CHAOTIC_SKYFIRE            ||
       p -> meta_gem == META_CHAOTIC_SKYFLARE           ||
       p -> meta_gem == META_CHAOTIC_SHADOWSPIRIT       ||
       p -> meta_gem == META_RELENTLESS_EARTHSIEGE      ||
       p -> meta_gem == META_RELENTLESS_EARTHSTORM      ||
       p -> meta_gem == META_REVERBERATING_SHADOWSPIRIT )
  {
    crit_multiplier *= 1.03;
  }
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

// attack_t::swing_haste ====================================================

double attack_t::swing_haste() const
{
  return player -> composite_attack_speed();
}

// attack_t::haste ==========================================================

double attack_t::haste() const
{
  return player -> composite_attack_haste();
}

// attack_t::execute_time ===================================================

timespan_t attack_t::execute_time() const
{
  if ( base_execute_time == timespan_t::zero() )
    return timespan_t::zero();

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero();

  //log_t::output( sim, "%s execute_time=%f base_execute_time=%f execute_time=%f", name(), base_execute_time * swing_haste(), base_execute_time, swing_haste() );
  return base_execute_time * swing_haste();
}

// attack_t::player_buff ====================================================

void attack_t::player_buff()
{
  action_t::player_buff();

  if ( !no_buffs )
  {
    player_hit       += player -> composite_attack_hit();
    player_crit      += player -> composite_attack_crit( weapon );
  }

  if ( sim -> debug )
    log_t::output( sim, "attack_t::player_buff: %s hit=%.2f crit=%.2f",
                   name(), player_hit, player_crit );
}

// attack_t::miss_chance ====================================================

double attack_t::miss_chance( double hit, int delta_level ) const
{
  double miss = 0.0;

  if ( target -> is_enemy() || target -> is_add() )
  {
    miss = 0.03 + ( delta_level * 0.015 );
  }
  else
  {
    // FIXME: needs testing
    miss = 0.03 + ( delta_level * 0.015 );
  }

  miss -= hit;

  if ( miss < 0.00 ) miss = 0.00;
  if ( miss > 0.99 ) miss = 0.99;

  return miss;
}

// attack_t::crit_block_chance ==============================================

double attack_t::crit_block_chance( int /* delta_level */ ) const
{
  // Tested: Player -> Target, both POSITION_RANGED_FRONT and POSITION_FRONT
  // % is 5%, and not 5% + delta_level * 0.5%.
  // Moved 5% to target_t::composite_tank_block

  // FIXME: Test Target -> Player
  return 0;
}

// attack_t::crit_chance ====================================================

double attack_t::crit_chance( double crit, int delta_level ) const
{
  double chance = crit;

  if ( target -> is_enemy() || target -> is_add() )
  {
    if ( delta_level > 2 )
    {
      chance -= ( 0.03 + ( delta_level * 0.006 ) );
    }
    else
    {
      chance -= ( delta_level * 0.002 );
    }
  }
  else
  {
    // FIXME: Assume 0.2% per level
    chance -= delta_level * 0.002;
  }

  return chance;
}

// attack_t::build_table ====================================================

int attack_t::build_table( std::array<double,RESULT_MAX>& chances,
                           std::array<result_type_e,RESULT_MAX>& results,
                           unsigned target_level,
                           double   attack_crit )
{
  double miss=0, dodge=0, parry=0, glance=0, block=0,crit_block=0, crit=0;

  int delta_level = target_level - player -> level;

  if ( may_miss   )   miss =   miss_chance( composite_hit(), delta_level ) + target -> composite_tank_miss( school );
  if ( may_dodge  )  dodge =  dodge_chance( composite_expertise(), delta_level ) + target -> composite_tank_dodge() - target -> diminished_dodge();
  if ( may_parry  )  parry =  parry_chance( composite_expertise(), delta_level ) + target -> composite_tank_parry() - target -> diminished_parry();
  if ( may_glance ) glance = glance_chance( delta_level );

  if ( may_block )
  {
    double block_total = block_chance( delta_level ) + target -> composite_tank_block();

    block = block_total * ( 1 - crit_block_chance( delta_level ) - target -> composite_tank_crit_block() );

    crit_block = block_total * ( crit_block_chance( delta_level ) + target -> composite_tank_crit_block() );
  }

  if ( may_crit && ! special ) // Specials are 2-roll calculations
    crit = crit_chance( attack_crit, delta_level ) + target -> composite_tank_crit( school );

  if ( sim -> debug )
    log_t::output( sim, "attack_t::build_table: %s miss=%.3f dodge=%.3f parry=%.3f glance=%.3f block=%.3f crit_block=%.3f crit=%.3f",
                   name(), miss, dodge, parry, glance, block, crit_block, crit );

  double limit = 1.0;
  double total = 0;
  int num_results = 0;

  if ( miss > 0 && total < limit )
  {
    total += miss;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_MISS;
    num_results++;
  }
  if ( dodge > 0 && total < limit )
  {
    total += dodge;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_DODGE;
    num_results++;
  }
  if ( parry > 0 && total < limit )
  {
    total += parry;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_PARRY;
    num_results++;
  }
  if ( glance > 0 && total < limit )
  {
    total += glance;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_GLANCE;
    num_results++;
  }
  if ( block > 0 && total < limit )
  {
    total += block;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_BLOCK;
    num_results++;
  }
  if ( crit_block > 0 && total < limit )
  {
    total += crit_block;
    if ( total > limit ) total = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_CRIT_BLOCK;
    num_results++;
  }
  if ( crit > 0 && total < limit )
  {
    total += crit;
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

result_type_e attack_t::calculate_result( double crit, unsigned target_level )
{
  direct_dmg = 0;
  result_type_e result = RESULT_NONE;

  if ( ! harmful || ! may_hit ) return RESULT_NONE;

  std::array<double,RESULT_MAX> chances;
  std::array<result_type_e,RESULT_MAX> results;

  int num_results = build_table( chances, results, target_level, crit );

  if ( num_results == 1 )
  {
    result = results[ 0 ];
  }
  else
  {
    // 1-roll attack table with true RNG

    double random = sim -> real();

    for ( int i=0; i < num_results; i++ )
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
    int delta_level = target_level - player -> level;

    if ( rng_result -> roll( crit_chance( crit, delta_level ) ) )
      result = RESULT_CRIT;
  }

  if ( sim -> debug )
    log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );

  return result;
}

void attack_t::init()
{
  action_t::init();

  if ( base_attack_power_multiplier > 0 && ( weapon_power_mod > 0 || direct_power_mod > 0 || tick_power_mod > 0 ) )
    snapshot_flags |= STATE_AP;

  if ( base_spell_power_multiplier > 0 && ( direct_power_mod > 0 || tick_power_mod > 0 ) )
    snapshot_flags |= STATE_SP;
}

// ==========================================================================
// Melee Attack
// ==========================================================================

// melee_attack_t::melee_attack_t =======================================================

// == Melee Attack Constructor ===============

melee_attack_t::melee_attack_t( const std::string&  n,
                                player_t*           p,
                                const spell_data_t* s,
                                school_type_e       sc ) :
  attack_t( n, p, s, sc ),
  base_expertise( 0 ), player_expertise( 0 ), target_expertise( 0 )
{
  may_miss = may_dodge = may_parry = may_glance = may_block = true;

  if ( special )
    may_glance = false;

  if ( player -> position == POSITION_BACK )
  {
    may_block = false;
    may_parry = false;
  }

  // Prevent melee from being scheduled when player is moving
  if ( range < 0 ) range = 5;
}

// melee_attack_t::player_buff ====================================================

void melee_attack_t::player_buff()
{
  attack_t::player_buff();

  if ( !no_buffs )
  {
    player_expertise = player -> composite_attack_expertise( weapon );
  }

  if ( sim -> debug )
    log_t::output( sim, "melee_attack_t::player_buff: %s expertise=%.2f",
                   name(), player_expertise );
}

// melee_attack_t::target_debuff ==================================================

void melee_attack_t::target_debuff( player_t* t, dmg_type_e dt )
{
  attack_t::target_debuff( t, dt );

  target_expertise = 0;
}

// melee_attack_t::total_expertise ================================================

double melee_attack_t::total_expertise() const
{
  return base_expertise + player_expertise + target_expertise;
}

// melee_attack_t::dodge_chance ===================================================

double melee_attack_t::dodge_chance( double expertise, int delta_level ) const
{
  return 0.03 + ( delta_level * 0.015 ) - expertise;
}

// melee_attack_t::parry_chance ===================================================

double melee_attack_t::parry_chance( double expertise, int delta_level ) const
{
  return 0.03 + ( delta_level * 0.015 ) + std::min( 0.0, dodge_chance( expertise, delta_level ) );
}

// melee_attack_t::glance_chance ==================================================

double melee_attack_t::glance_chance( int delta_level ) const
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
                                  const spell_data_t* s,
                                  school_type_e sc ) :
  attack_t( token, p, s, sc ),
  base_expertise( 0 ), player_expertise( 0 ), target_expertise( 0 )
{
  may_miss  = true;
  may_dodge = true;

  if ( player -> position == POSITION_RANGED_FRONT )
    may_block = true;
}

// ranged_attack_t::player_buff ====================================================

void ranged_attack_t::player_buff()
{
  attack_t::player_buff();

  if ( !no_buffs )
  {
    player_expertise = player -> composite_attack_expertise( weapon );
  }

  if ( sim -> debug )
    log_t::output( sim, "ranged_attack_t::player_buff: %s expertise=%.2f",
                   name(), player_expertise );
}

void ranged_attack_t::target_debuff( player_t* t, dmg_type_e dt )
{
  attack_t::target_debuff( t, dt );

  target_expertise = 0;

  if ( !no_debuffs )
  {
    target_multiplier       *= t -> composite_ranged_attack_player_vulnerability();
  }

  if ( sim -> debug )
    log_t::output( sim, "ranged_attack_t::target_debuff: %s mult=%.2f",
                   name(), target_multiplier );
}

// ranged_attack_t::total_expertise ================================================

double ranged_attack_t::total_expertise() const
{
  return base_expertise + player_expertise + target_expertise;
}

// ranged_attack_t::dodge_chance ===================================================

double ranged_attack_t::dodge_chance( double expertise, int delta_level ) const
{
  return 0.03 + ( delta_level * 0.015 ) - expertise;
}

// ranged_attack_t::parry_chance ===================================================

double ranged_attack_t::parry_chance( double /* expertise */, int /* delta_level */ ) const
{
  // Assumed impossible to parry ranged. Needs checking.
  return 0.0;
}

// ranged_attack_t::glance_chance ==================================================

double ranged_attack_t::glance_chance( int delta_level ) const
{
  return (  delta_level  + 1 ) * 0.06;
}

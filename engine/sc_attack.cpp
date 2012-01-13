// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Attack
// ==========================================================================

// attack_t::attack_t =======================================================

void attack_t::init_attack_t_()
{
  player_t* p = player;

  may_miss = may_resist = may_dodge = may_parry = may_glance = may_block = true;

  if ( special ) may_glance = false;

  if ( p -> position == POSITION_BACK )
  {
    may_block = false;
    may_parry = false;
  }
  else if ( p -> position == POSITION_RANGED_FRONT )
  {
    may_block  = true;
    may_dodge  = false;
    may_glance = false; // FIXME! If we decide to make ranged auto-shot become "special" this line goes away.
    may_parry  = false;
  }
  else if ( p -> position == POSITION_RANGED_BACK )
  {
    may_block  = false;
    may_dodge  = false;
    may_glance = false; // FIXME! If we decide to make ranged auto-shot become "special" this line goes away.
    may_parry  = false;
  }

  base_attack_power_multiplier = 1.0;
  crit_bonus = 1.0;

  min_gcd = timespan_t::from_seconds( 1.0 );
  hasted_ticks = false;

  // Prevent melee from being scheduled when player is moving
  if ( range < 0 ) range = 5;

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

attack_t::attack_t( const active_spell_t& s, int t, bool special ) :
  action_t( ACTION_ATTACK, s, t, special )
{
  init_attack_t_();
}

attack_t::attack_t( const char* n, player_t* p, int resource, const school_type school, int tree, bool special ) :
  action_t( ACTION_ATTACK, n, p, resource, school, tree, special ),
  base_expertise( 0 ), player_expertise( 0 ), target_expertise( 0 )
{
  init_attack_t_();
}

attack_t::attack_t( const char* name, const char* sname, player_t* p, int t, bool special ) :
  action_t( ACTION_ATTACK, name, sname, p, t, special ),
  base_expertise( 0 ), player_expertise( 0 ), target_expertise( 0 )
{
  init_attack_t_();
}

attack_t::attack_t( const char* name, const uint32_t id, player_t* p, int t, bool special ) :
  action_t( ACTION_ATTACK, name, id, p, t, special ),
  base_expertise( 0 ), player_expertise( 0 ), target_expertise( 0 )
{
  init_attack_t_();
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
  if ( base_execute_time == timespan_t::zero ) return timespan_t::zero;

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero;

  return base_execute_time * swing_haste();
}

// attack_t::player_buff ====================================================

void attack_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_hit       = p -> composite_attack_hit();
  player_expertise = p -> composite_attack_expertise();
  player_crit      = p -> composite_attack_crit();

  if ( weapon )
  {
    if ( p -> race == RACE_ORC )
    {
      switch ( weapon -> type )
      {
      case WEAPON_AXE:
      case WEAPON_AXE_2H:
      case WEAPON_FIST:
        player_expertise += 0.03;
        break;
      }
    }
    else if ( p -> race == RACE_TROLL )
    {
      switch ( weapon -> type )
      {
      case WEAPON_BOW:
        player_crit += 0.01;
        break;
      }
    }
    else if ( p -> race == RACE_HUMAN )
    {
      switch ( weapon -> type )
      {
      case WEAPON_MACE:
      case WEAPON_MACE_2H:
      case WEAPON_SWORD:
      case WEAPON_SWORD_2H:
        player_expertise += 0.03;
        break;
      }
    }
    else if ( p -> race == RACE_DWARF )
    {
      switch ( weapon -> type )
      {
      case WEAPON_GUN:
        player_crit += 0.01;
        break;
      case WEAPON_MACE:
      case WEAPON_MACE_2H:
        player_expertise += 0.03;
        break;
      }
    }
    else if ( p -> race == RACE_GNOME )
    {
      switch ( weapon -> type )
      {
      case WEAPON_DAGGER:
      case WEAPON_SWORD:
        player_expertise += 0.03;
        break;
      }
    }
  }

  if ( sim -> debug )
    log_t::output( sim, "attack_t::player_buff: %s hit=%.2f expertise=%.2f crit=%.2f",
                   name(), player_hit, player_expertise, player_crit );
}

// attack_t::target_debuff ==================================================

void attack_t::target_debuff( player_t* t, int dmg_type )
{
  action_t::target_debuff( t, dmg_type );

  target_expertise = 0;
}

// attack_t::total_expertise ================================================

double attack_t::total_expertise() const
{
  double e = base_expertise + player_expertise + target_expertise;

  // Round down to dicrete units of Expertise?  Not according to EJ:
  // http://elitistjerks.com/f78/t38095-retesting_hit_table_assumptions/p3/#post1092985
  if ( false ) e = floor( 100.0 * e ) / 100.0;

  return e;
}

// attack_t::miss_chance ====================================================

double attack_t::miss_chance( int delta_level ) const
{
  if ( target -> is_enemy() || target -> is_add() )
  {
    if ( delta_level > 2 )
    {
      // FIXME: needs testing for delta_level > 3
      return 0.06 + ( delta_level - 2 ) * 0.02 - total_hit();
    }
    else
    {
      return 0.05 + delta_level * 0.005 - total_hit();
    }
  }
  else
  {
    // FIXME: needs testing
    if ( delta_level >= 0 )
      return 0.05 + delta_level * 0.02;
    else
      return 0.05 + delta_level * 0.01;
  }
}

// attack_t::dodge_chance ===================================================

double attack_t::dodge_chance( int delta_level ) const
{
  return 0.05 + delta_level * 0.005 - 0.25 * total_expertise();
}

// attack_t::parry_chance ===================================================

double attack_t::parry_chance( int delta_level ) const
{
  // Tested on 03.03.2011 with a data set for delta_level = 5 which gave 22%
  if ( delta_level > 2 )
    return 0.10 + ( delta_level - 2 ) * 0.04 - 0.25 * total_expertise();
  else
    return 0.05 + delta_level * 0.005 - 0.25 * total_expertise();
}

// attack_t::glance_chance ==================================================

double attack_t::glance_chance( int delta_level ) const
{
  return (  delta_level  + 1 ) * 0.06;
}

// attack_t::block_chance ===================================================

double attack_t::block_chance( int /* delta_level */ ) const
{
  // Tested: Player -> Target, both POSITION_RANGED_FRONT and POSITION_FRONT
  // % is 5%, and not 5% + delta_level * 0.5%.
  // Moved 5% to target_t::composite_tank_block

  // FIXME: Test Target -> Player
  return 0;
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

double attack_t::crit_chance( int delta_level ) const
{
  double chance = total_crit();

  if ( target -> is_enemy() || target -> is_add() )
  {
    if ( delta_level > 2 )
    {
      chance -= ( 0.03 + delta_level * 0.006 );
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

int attack_t::build_table( double* chances,
                           int*    results )
{
  double miss=0, dodge=0, parry=0, glance=0, block=0,crit_block=0, crit=0;

  int delta_level = target -> level - player -> level;

  if ( may_miss   )   miss =   miss_chance( delta_level ) + target -> composite_tank_miss( school );
  if ( may_dodge  )  dodge =  dodge_chance( delta_level ) + target -> composite_tank_dodge() - target -> diminished_dodge();
  if ( may_parry  )  parry =  parry_chance( delta_level ) + target -> composite_tank_parry() - target -> diminished_parry();
  if ( may_glance ) glance = glance_chance( delta_level );

  if ( may_block )
  {
    double block_total = block_chance( delta_level ) + target -> composite_tank_block();

    block = block_total * ( 1 - crit_block_chance( delta_level ) - target -> composite_tank_crit_block() );

    crit_block = block_total * ( crit_block_chance( delta_level ) + target -> composite_tank_crit_block() );
  }

  if ( may_crit && ! special ) // Specials are 2-roll calculations
  {
    crit = crit_chance( delta_level ) + target -> composite_tank_crit( school );
  }

  if ( sim -> debug ) log_t::output( sim, "attack_t::build_table: %s miss=%.3f dodge=%.3f parry=%.3f glance=%.3f block=%.3f crit_block=%.3f crit=%.3f",
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

void attack_t::calculate_result()
{
  direct_dmg = 0;

  double chances[ RESULT_MAX ];
  int    results[ RESULT_MAX ];

  result = RESULT_NONE;

  if ( ! harmful || ! may_hit ) return;

  int num_results = build_table( chances, results );

  if ( num_results == 1 )
  {
    result = results[ 0 ];
  }
  else
  {
    if ( sim -> smooth_rng )
    {
      // Encode 1-roll attack table into equivalent multi-roll system

      double prev_chance = 0.0;

      for ( int i=0; i < num_results-1; i++ )
      {
        if ( rng[ results[ i ] ] -> roll( ( chances[ i ] - prev_chance ) / ( 1.0 - prev_chance ) ) )
        {
          result = results[ i ];
          break;
        }
        prev_chance = chances[ i ];
      }
      if ( result == RESULT_NONE ) result = results[ num_results-1 ];
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
  }

  assert( result != RESULT_NONE );

  if ( result_is_hit() )
  {
    if ( binary && rng[ RESULT_RESIST ] -> roll( resistance() ) )
    {
      result = RESULT_RESIST;
    }
    else if ( special && may_crit && ( result == RESULT_HIT ) ) // Specials are 2-roll calculations
    {
      int delta_level = target -> level - player -> level;

      if ( rng[ RESULT_CRIT ] -> roll( crit_chance( delta_level ) ) )
      {
        result = RESULT_CRIT;
      }
    }
  }

  if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

// attack_t::execute ========================================================

void attack_t::execute()
{
  action_t::execute();

  if ( harmful && callbacks )
  {
    if ( result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> attack_callbacks[ result ], this );
    }
  }
}

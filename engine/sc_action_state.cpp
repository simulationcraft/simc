// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

action_state_t* action_t::get_state( const action_state_t* other )
{
  action_state_t* s = 0;

  if ( state_cache != 0 )
  {
    s = state_cache;
    state_cache = s -> next;
    s -> next = 0;
  }
  else
    s = new_state();

  s -> copy_state( other );

  return s;
}

action_state_t* action_t::new_state()
{
  return new action_state_t( this, target );
}

void action_t::release_state( action_state_t* s )
{
  s -> next = state_cache;
  state_cache = s;
}

void action_state_t::copy_state( const action_state_t* o )
{
  if ( o == 0 ) return;

  action = o -> action; target = o -> target;
  result = o -> result; result_amount = o -> result_amount;
  base_hit = o -> base_hit; actor_hit = o -> actor_hit;
  actor_expertise = o -> actor_expertise;
  haste = o -> haste;
  base_crit = o -> base_crit; actor_crit = o -> actor_crit; target_crit = o -> target_crit;
  mastery = o -> mastery;
  base_attack_power = o -> base_attack_power; actor_attack_power = o -> actor_attack_power;
  target_attack_power = o -> target_attack_power;
  base_spell_power = o -> base_spell_power; actor_spell_power = o -> actor_spell_power;
  base_penetration = o -> base_penetration; actor_penetration = o -> actor_penetration;
  target_penetration = o -> target_penetration;

  base_multiplier = o -> base_multiplier; base_da_multiplier = o -> base_da_multiplier;
  base_ta_multiplier = o -> base_ta_multiplier;
  actor_multiplier = o -> actor_multiplier; actor_da_multiplier = o -> actor_da_multiplier;
  actor_ta_multiplier = o -> actor_ta_multiplier;
  target_multiplier = o -> target_multiplier;
  base_ap_multiplier = o -> base_ap_multiplier, actor_ap_multiplier = o -> actor_ap_multiplier;
  base_sp_multiplier = o -> base_sp_multiplier, actor_sp_multiplier = o -> actor_sp_multiplier;
}

action_state_t::action_state_t( action_t* a, player_t* t ) :
  action( a ), target( t ),
  result( RESULT_NONE ), result_amount( 0 ),
  base_hit( 0 ), actor_hit( 0 ),
  actor_expertise( 0 ),
  haste( 0 ),
  base_crit( 0 ), actor_crit( 0 ), target_crit( 0 ),
  mastery( 0 ),
  base_attack_power( 0 ), actor_attack_power( 0 ), target_attack_power( 0 ),
  base_spell_power( 0 ), actor_spell_power( 0 ),
  base_penetration( 0 ), actor_penetration( 0 ), target_penetration( 0 ),
  base_multiplier( 1.0 ), base_da_multiplier( 1.0 ), base_ta_multiplier( 1.0 ),
  actor_multiplier( 1.0 ), actor_da_multiplier( 1.0 ), actor_ta_multiplier( 1.0 ),
  target_multiplier( 1.0 ),
  base_ap_multiplier( 0 ), actor_ap_multiplier( 0 ),
  base_sp_multiplier( 0 ), actor_sp_multiplier( 0 ),
  next( 0 )
{
}

void action_state_t::debug() const
{
  log_t::output( action -> sim,
                 "[NEW] %s %s %s: result=%s amount=%.2f "
                 "mastery=%.2f "
                 "b_hit=%.2f a_hit=%.2f "
                 "a_exp=%.2f "
                 "haste=%.2f "
                 "b_crit=%.2f a_crit=%.2f t_crit=%.2f "
                 "b_pen=%.2f a_pen=%.2f t_pen=%.2f "
                 "b_ap=%.0f a_ap=%.0f t_ap=%.0f b_ap_mul=%.2f a_ap_mul=%.2f "
                 "b_sp=%.0f a_sp=%.0f b_sp_mul=%.2f a_sp_mul=%.2f "
                 "b_mul=%.4f b_da_mul=%.4f b_ta_mul=%.4f "
                 "a_mul=%.4f a_da_mul=%.4f a_ta_mul=%.4f "
                 "t_mul=%.4f",
                 action -> player -> name(),
                 action -> name(),
                 target -> name(),
                 util_t::result_type_string( result ), result_amount,
                 mastery,
                 base_hit, actor_hit, actor_expertise,
                 haste,
                 base_crit, actor_crit, target_crit,
                 base_penetration, actor_penetration, target_penetration,
                 base_attack_power, actor_attack_power, target_attack_power,
                 base_ap_multiplier, actor_ap_multiplier,
                 base_spell_power, actor_spell_power,
                 base_sp_multiplier, actor_sp_multiplier,
                 base_multiplier, base_da_multiplier, base_ta_multiplier,
                 actor_multiplier, actor_da_multiplier, actor_ta_multiplier,
                 target_multiplier );
}

void action_state_t::take_state( uint32_t flags )
{
  if ( flags & STATE_MASTERY )
    snapshot_mastery();

  if ( flags & STATE_CRIT )
    snapshot_crit();

  if ( flags & STATE_HASTE )
    snapshot_haste();

  if ( flags & STATE_HIT )
    snapshot_hit();

  if ( flags & STATE_EXPERTISE )
    snapshot_expertise();

  if ( flags & STATE_AP )
  {
    snapshot_attack_power();
    snapshot_attack_power_multiplier();

    if ( flags & STATE_TARGET_POWER )
      snapshot_target_attack_power();
  }

  if ( flags & STATE_SP )
  {
    snapshot_spell_power();
    snapshot_spell_power_multiplier();
  }

  if ( flags & STATE_PENETRATION )
    snapshot_penetration();

  if ( flags & ( STATE_BASE_MUL | STATE_BASE_MUL_DA | STATE_BASE_MUL_TA ) )
    snapshot_base_multiplier( flags );

  if ( flags & ( STATE_ACTOR_MUL | STATE_ACTOR_MUL_DA | STATE_ACTOR_MUL_TA ) )
    snapshot_actor_multiplier( flags );

  if ( flags & STATE_TARGET_CRIT )
    snapshot_target_crit();

  if ( flags & STATE_TARGET_MUL )
    snapshot_target_multiplier();

  if ( flags & STATE_TARGET_PEN )
    snapshot_target_penetration();
}

stateless_travel_event_t::stateless_travel_event_t( sim_t*    sim,
                                                    action_t* a,
                                                    action_state_t* state,
                                                    timespan_t time_to_travel ) :
  event_t( sim, a -> player ), action( a ), state( state )
{
  name   = "Stateless Action Travel";

  if ( sim -> debug )
    log_t::output( sim, "New Stateless Action Travel Event: %s %s %.2f",
                   player -> name(), a -> name(), time_to_travel.total_seconds() );

  sim -> add_event( this, time_to_travel );
}

void stateless_travel_event_t::execute()
{
  action -> impact_s( state );
  action -> release_state( state );
  if ( action -> travel_event == this )
    action -> travel_event = NULL;
}

void spell_t::calculate_result_s( action_state_t* state )
{
  state -> result        = RESULT_NONE;
  state -> result_amount = 0;

  if ( ! harmful || ! may_hit ) return;

  if ( may_miss )
  {
    if ( rng_result -> roll( miss_chance_s( state ) ) )
      state -> result = RESULT_MISS;
  }

  if ( state -> result == RESULT_NONE && may_resist && binary )
  {
    if ( rng_result -> roll( resistance_s( state ) ) )
      state -> result = RESULT_RESIST;
  }

  if ( state -> result == RESULT_NONE )
  {
    state -> result = RESULT_HIT;

    if ( may_crit )
    {
      if ( rng_result -> roll( crit_chance_s( state ) ) )
        state -> result = RESULT_CRIT;
    }
  }

  if ( sim -> debug )
    log_t::output( sim, "[NEW] %s result for %s is %s",
                   player -> name(),
                   name(),
                   util_t::result_type_string( state -> result ) );
}

void action_t::schedule_travel_s( action_state_t* s )
{
  time_to_travel = travel_time();

  if ( ! state )
    state = get_state();

  state -> copy_state( s );

  if ( time_to_travel == timespan_t::zero )
  {
    impact_s( s );
    release_state( s );
  }
  else
  {
    if ( sim -> log )
    {
      log_t::output( sim, "[NEW] %s schedules travel (%.3f) for %s", player -> name(), time_to_travel.total_seconds(), name() );
    }

    new ( sim ) stateless_travel_event_t( sim, this, s, time_to_travel );
  }
}

double attack_t::crit_chance_s( const action_state_t* state ) const
{
  double chance = state -> total_crit();
  int delta_level = state -> target -> level - state -> action -> player -> level;

  if ( state -> target -> is_enemy() || state -> target -> is_add() )
  {
    if ( delta_level > 2 )
      chance -= ( 0.03 + delta_level * 0.006 );
    else
      chance -= ( delta_level * 0.002 );
  }
  else
    chance -= delta_level * 0.002;

  return chance;
}

double attack_t::miss_chance_s( const action_state_t* state ) const
{
  int delta_level = state -> target -> level - state -> action -> player -> level;

  if ( state -> target -> is_enemy() || state -> target -> is_add() )
  {
    if ( delta_level > 2 )
    {
      // FIXME: needs testing for delta_level > 3
      return 0.06 + ( delta_level - 2 ) * 0.02 - state -> total_hit();
    }
    else
    {
      return 0.05 + delta_level * 0.005 - state -> total_hit();
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

double attack_t::dodge_chance_s( const action_state_t* state ) const
{
  int delta_level = state -> target -> level - state -> action -> player -> level;

  return 0.05 + delta_level * 0.005 - 0.25 * state -> total_expertise();
}

double attack_t::parry_chance_s( const action_state_t* state ) const
{
  int delta_level = state -> target -> level - state -> action -> player -> level;

  // Tested on 03.03.2011 with a data set for delta_level = 5 which gave 22%
  if ( delta_level > 2 )
    return 0.10 + ( delta_level - 2 ) * 0.04 - 0.25 * state -> total_expertise();
  else
    return 0.05 + delta_level * 0.005 - 0.25 * state -> total_expertise();
}

double spell_t::miss_chance_s( const action_state_t* state ) const
{
  int delta_level = state -> target -> level - state -> action -> player -> level;
  double miss = 0;

  if ( delta_level > 2 )
  {
    miss = 0.17 + ( delta_level - 3 ) * 0.11;
  }
  else
  {
    miss = 0.04 + delta_level * 0.01;
  }

  miss -= state -> total_hit();

  if ( miss < 0.00 ) miss = 0.00;
  if ( miss > 0.99 ) miss = 0.99;

  return miss;
}

double spell_t::crit_chance_s( const action_state_t* state ) const
{
  return state -> total_crit();
}

double attack_t::glance_chance_s( const action_state_t* state ) const
{
  return ( state -> target -> level - state -> action -> player -> level  + 1 ) * 0.06;
}

double attack_t::block_chance_s( const action_state_t* /* state */ ) const
{
  // Tested: Player -> Target, both POSITION_RANGED_FRONT and POSITION_FRONT
  // % is 5%, and not 5% + delta_level * 0.5%.
  // Moved 5% to target_t::composite_tank_block

  // FIXME: Test Target -> Player
  return 0;
}

double attack_t::crit_block_chance_s( const action_state_t* /* state */ ) const
{
  // Tested: Player -> Target, both POSITION_RANGED_FRONT and POSITION_FRONT
  // % is 5%, and not 5% + delta_level * 0.5%.
  // Moved 5% to target_t::composite_tank_block

  // FIXME: Test Target -> Player
  return 0;
}

void attack_t::calculate_result_s( action_state_t* state )
{
  double chances[ RESULT_MAX ];
  int    results[ RESULT_MAX ];
  action_t* action = state -> action;

  state -> result        = RESULT_NONE;
  state -> result_amount = 0;

  if ( ! action -> harmful || ! action -> may_hit ) return;

  int num_results = build_table_s( chances, results, state );

  if ( num_results == 1 )
    state -> result = static_cast< result_type_e >( results[ 0 ] );
  else
  {
    // 1-roll attack table

    double random = action -> sim -> real();

    for ( int i = 0; i < num_results; i++ )
    {
      if ( random <= chances[ i ] )
      {
        state -> result = static_cast< result_type_e >( results[ i ] );
        break;
      }
    }
  }

  assert( state -> result != RESULT_NONE );

  if ( action -> result_is_hit( state -> result ) )
  {
    if ( action -> binary && action -> rng_result -> roll( action -> resistance_s( state ) ) )
      state -> result = RESULT_RESIST;
    else if ( action -> special && action -> may_crit && state -> result == RESULT_HIT ) // Specials are 2-roll calculations
    {
      if ( action -> rng_result -> roll( crit_chance_s( state ) ) )
        state -> result = RESULT_CRIT;
    }
  }

  if ( action -> sim -> debug )
    log_t::output( action -> sim, "[NEW] %s result for %s is %s",
                   action -> player -> name(),
                   action -> name(),
                   util_t::result_type_string( state -> result ) );
}

int attack_t::build_table_s( double* chances, int* results, const action_state_t* s )
{
  double miss=0, dodge=0, parry=0, glance=0, block=0,crit_block=0, crit=0;

  if ( may_miss   )   miss =   miss_chance_s( s ) + s -> target -> composite_tank_miss( school );
  if ( may_dodge  )  dodge =  dodge_chance_s( s ) + s -> target -> composite_tank_dodge() - s -> target -> diminished_dodge();
  if ( may_parry  )  parry =  parry_chance_s( s ) + s -> target -> composite_tank_parry() - s -> target -> diminished_parry();
  if ( may_glance ) glance = glance_chance_s( s );

  if ( may_block )
  {
    double block_total = block_chance_s( s ) + s -> target -> composite_tank_block();

    block = block_total * ( 1 - crit_block_chance_s( s ) - s -> target -> composite_tank_crit_block() );

    crit_block = block_total * ( crit_block_chance_s( s ) + s -> target -> composite_tank_crit_block() );
  }

  if ( may_crit && ! special ) // Specials are 2-roll calculations
  {
    crit = crit_chance_s( s ) + s -> target -> composite_tank_crit( school );
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

double heal_t::crit_chance_s( const action_state_t* state ) const
{
  return state -> total_crit();
}

void heal_t::calculate_result_s( action_state_t* state )
{
  state -> result_amount = 0;
  state -> result = RESULT_NONE;

  if ( ! state -> action -> harmful ) return;

  state -> result = RESULT_HIT;

  if ( state -> action -> may_crit )
  {
    if ( state -> action -> rng_result -> roll( crit_chance_s( state ) ) )
    {
      state -> result = RESULT_CRIT;
    }
  }

  if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( state -> result ) );
}

void absorb_t::calculate_result_s( action_state_t* state )
{
  state -> result_amount = 0;
  state -> result = RESULT_NONE;

  if ( ! state -> action -> harmful ) return;

  state -> result = RESULT_HIT;

  if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( state -> result ) );
}

double action_t::calculate_direct_damage_s( int chain_target, const action_state_t* state )
{
  double amount = sim -> range( base_dd_min, base_dd_max );

  if ( round_base_dmg ) amount = floor( amount + 0.5 );

  if ( amount == 0 && weapon_multiplier == 0 && direct_power_mod == 0 )
    return 0;

  amount += base_dd_adder + player_dd_adder + target_dd_adder;

  if ( weapon_multiplier > 0 )
  {
    amount += calculate_weapon_damage_s( state );
    amount *= weapon_multiplier;
  }

  amount += direct_power_mod * state -> total_power();

  amount *= state -> total_da_multiplier();

  if ( state -> result == RESULT_GLANCE )
  {
    double delta_skill = ( state -> target -> level - player -> level ) * 5.0;

    if ( delta_skill < 0.0 )
      delta_skill = 0.0;

    double max_glance = 1.3 - 0.03 * delta_skill;

    if ( max_glance > 0.99 )
      max_glance = 0.99;
    else if ( max_glance < 0.2 )
      max_glance = 0.20;

    double min_glance = 1.4 - 0.05 * delta_skill;

    if ( min_glance > 0.91 )
      min_glance = 0.91;
    else if ( min_glance < 0.01 )
      min_glance = 0.01;

    if ( min_glance > max_glance )
    {
      double temp = min_glance;
      min_glance = max_glance;
      max_glance = temp;
    }

    amount *= sim -> range( min_glance, max_glance ); // 0.75 against +3 targets.
  }
  else if ( state -> result == RESULT_CRIT )
    amount *= 1.0 + total_crit_bonus();

  if ( ! binary )
    amount *= 1.0 - resistance();

  if ( chain_target > 0 && base_add_multiplier != 1.0 )
    amount *= pow( base_add_multiplier, chain_target );

  if ( ! sim -> average_range )
    amount = floor( amount + sim -> real() );

  return amount;
}

double action_t::calculate_weapon_damage_s( const action_state_t* state )
{
  if ( ! weapon || weapon_multiplier <= 0 ) return 0;

  double dmg = sim -> range( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> bonus_dmg;

  timespan_t weapon_speed  = normalize_weapon_speed  ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

  double power_damage = weapon_speed.total_seconds() * weapon_power_mod * state -> total_attack_power();

  double total_dmg = dmg + power_damage;

  // OH penalty
  if ( weapon -> slot == SLOT_OFF_HAND )
    total_dmg *= 0.5;

  if ( sim -> debug )
  {
    log_t::output( sim, "%s weapon damage for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f pd=%.3f ap=%.3f",
                   player -> name(), name(), total_dmg, dmg, weapon -> bonus_dmg, weapon_speed.total_seconds(), power_damage, state -> total_attack_power() );
  }

  return total_dmg;
}

void action_t::impact_s( action_state_t* s )
{
  assess_damage( s -> target, s -> result_amount, DMG_DIRECT, s -> result );

  if ( result_is_hit( s -> result ) )
  {
    if ( num_ticks > 0 )
    {
      dot_t* dot = this -> dot();
      if ( dot_behavior != DOT_REFRESH ) dot -> cancel();
      dot -> action = this;
      dot -> current_tick = 0;
      dot -> added_ticks = 0;
      dot -> added_seconds = timespan_t::zero;
      // Snapshot the stats
      if ( ! dot -> state )
        dot -> state = get_state( s );
      else
        dot -> state -> copy_state( s );
      dot -> num_ticks = hasted_num_ticks( dot -> state -> total_haste() );
      if ( dot -> ticking )
      {
        assert( dot -> tick_event );
        if ( ! channeled )
        {
          // Recasting a dot while it's still ticking gives it an extra tick in total
          dot -> num_ticks++;

          // Fix to refreshing tick_zero dots
          if ( tick_zero )
          {
            dot -> num_ticks++;
          }
        }
      }
      else
      {
        if ( school == SCHOOL_BLEED ) s -> target -> debuffs.bleeding -> increment();

        dot -> schedule_tick();
      }
      dot -> recalculate_ready();

      if ( sim -> debug )
        log_t::output( sim, "%s extends dot-ready to %.2f for %s (%s)",
                       player -> name(), dot -> ready.total_seconds(), name(), dot -> name() );
    }
  }
  else
  {
    if ( sim -> log )
    {
      log_t::output( sim, "Target %s avoids %s %s (%s)", target -> name(), player -> name(), name(), util_t::result_type_string( s -> result ) );
    }
  }
}

void absorb_t::impact_s( action_state_t* s )
{
  if ( s -> result_amount > 0 )
  {
    assess_damage( s -> target, s -> result_amount, ABSORB, s -> result );
  }
}

double action_t::calculate_tick_damage_s( const action_state_t* s )
{
  double dmg = 0;

  if ( base_td == 0 ) base_td = base_td_init;

  if ( base_td == 0 && tick_power_mod == 0 ) return 0;

  dmg  = floor( base_td + 0.5 ) + s -> total_power() * tick_power_mod;
  dmg *= s -> total_ta_multiplier();

  if ( s -> result == RESULT_CRIT )
    dmg *= 1.0 + total_crit_bonus();

  if ( ! binary )
    dmg *= 1.0 - resistance();

  if ( ! sim -> average_range )
    dmg = floor( dmg + sim -> real() );

  return dmg;
}

timespan_t action_t::tick_time_s( const action_state_t* state ) const
{
  timespan_t t = base_tick_time;
  if ( channeled || hasted_ticks )
    t *= state -> haste;
  return t;
}

double action_t::resistance_s( const action_state_t* s ) const
{
  if ( ! may_resist )
    return 0;

  double resist=0;

  double penetration = s -> total_penetration();

  if ( school != SCHOOL_BLEED )
  {
    double resist_rating = s -> target -> composite_spell_resistance( school );

    if ( school == SCHOOL_FROSTFIRE )
    {
      resist_rating = std::min( s -> target -> composite_spell_resistance( SCHOOL_FROST ),
                                s -> target -> composite_spell_resistance( SCHOOL_FIRE  ) );
    }
    else if ( school == SCHOOL_SPELLSTORM )
    {
      resist_rating = std::min( s -> target -> composite_spell_resistance( SCHOOL_ARCANE ),
                                s -> target -> composite_spell_resistance( SCHOOL_NATURE ) );
    }
    else if ( school == SCHOOL_SHADOWFROST )
    {
      resist_rating = std::min( s -> target -> composite_spell_resistance( SCHOOL_SHADOW ),
                                s -> target -> composite_spell_resistance( SCHOOL_FROST  ) );
    }
    else if ( school == SCHOOL_SHADOWFLAME )
    {
      resist_rating = std::min( s -> target -> composite_spell_resistance( SCHOOL_SHADOW ),
                                s -> target -> composite_spell_resistance( SCHOOL_FIRE   ) );
    }

    resist_rating -= penetration;
    if ( resist_rating < 0 ) resist_rating = 0;
    if ( resist_rating > 0 )
    {
      resist = resist_rating / ( resist_rating + s -> action -> player -> half_resistance_rating );
    }

    if ( resist > 1.0 ) resist = 1.0;
  }

  if ( sim -> debug ) log_t::output( sim, "%s queries resistance for %s: %.2f", player -> name(), name(), resist );

  return resist;
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Spell Base
// ==========================================================================

spell_base_t::spell_base_t( action_e at,
                            const std::string& token,
                            player_t* p,
                            const spell_data_t* s ) :
  action_t( at, token, p, s ),
  procs_courageous_primal_diamond( true )
{
  min_gcd = p -> min_gcd;
  gcd_haste = SPEED_SPELL; // Hasten spell GCDs by default
  //min_gcd = timespan_t::from_seconds( 1.0 );
  hasted_ticks = true;
  special = true;

  crit_bonus = 1.0;
  crit_multiplier *= util::crit_multiplier( player -> meta_gem );
}

timespan_t spell_base_t::execute_time() const
{
  timespan_t t = base_execute_time;

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero();

  if ( t <= timespan_t::zero() ) return timespan_t::zero();
  t *= composite_haste();

  return t;
}

timespan_t spell_base_t::tick_time( const action_state_t* state ) const
{
  return action_t::tick_time( state );
}

result_e spell_base_t::calculate_result( action_state_t* s ) const
{
  result_e result = RESULT_NONE;

  if ( ! s -> target ) return RESULT_NONE;

  if ( ! may_hit ) return RESULT_NONE;

  if ( ( result == RESULT_NONE ) && may_miss )
  {
    if ( rng().roll( miss_chance( composite_hit(), s -> target ) ) )
    {
      result = RESULT_MISS;
    }
  }

  if ( result == RESULT_NONE )
  {
    result = RESULT_HIT;

    if ( may_crit )
    {
      if ( rng().roll( std::max( s -> composite_crit_chance(), 0.0 ) ) )
        result = RESULT_CRIT;
    }
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s result for %s is %s", player -> name(), name(), util::result_type_string( result ) );

  return result;
}

void spell_base_t::execute()
{
  action_t::execute();

  if ( player -> last_foreground_action == this )
    player -> debuffs.casting -> expire();
}

void spell_base_t::schedule_execute( action_state_t* execute_state )
{
  action_t::schedule_execute( execute_state );

  if ( ! background && time_to_execute > timespan_t::zero() )
    player -> debuffs.casting -> trigger();
}

proc_types spell_base_t::proc_type() const
{
  if ( ! is_aoe() )
  {
    if ( type == ACTION_SPELL )
      return PROC1_SPELL;
    // Only allow non-harmful abilities with "an amount" to count as heals
    else if ( ( type == ACTION_HEAL || type == ACTION_ABSORB ) && has_amount_result() )
      return PROC1_HEAL;
  }
  else
  {
    if ( type == ACTION_SPELL )
      return PROC1_AOE_SPELL;
    else if ( ( type == ACTION_HEAL || type == ACTION_ABSORB ) && has_amount_result() )
      return PROC1_AOE_HEAL;
  }

  return PROC1_INVALID;
}

// ==========================================================================
// Harmful Spell
// ==========================================================================

spell_t::spell_t( const std::string&  token,
                  player_t*           p,
                  const spell_data_t* s ) :
  spell_base_t( ACTION_SPELL, token, p, s )
{
  may_miss = true;
  may_block = false;
}

double spell_t::miss_chance( double hit, player_t* t ) const
{  
  // base spell miss is double base melee miss
  double miss = t -> cache.miss();
  miss *= 2;

  // 11% level-dependent miss for level+4
  miss += 0.03 * ( t -> level() - player -> level() );
    
  miss += 0.08 * std::max( t -> level() - player -> level() - 3, 0 );

  // subtract the player's hit and expertise
  miss -= hit;

  return miss;
}

void spell_t::assess_damage( dmg_e type,
                             action_state_t* s )
{
  spell_base_t::assess_damage( type, s );
}

dmg_e spell_t::amount_type( const action_state_t* /* state */, bool periodic ) const
{
  if ( periodic )
    return DMG_OVER_TIME;
  else
    return DMG_DIRECT;
}

dmg_e spell_t::report_amount_type( const action_state_t* state ) const
{
  dmg_e result_type = state -> result_type;

  if ( result_type == DMG_DIRECT )
  {
    // Direct ticks are direct damage, that are recorded as ticks
    if ( direct_tick )
      result_type = DMG_OVER_TIME;
    // With direct damage, we need to check if this action is a tick action of
    // someone. If so, then the damage should be recorded as periodic.
    else
    {
      if ( !stats -> action_list.empty() && stats -> action_list.front() -> tick_action == this )
      {
        result_type = DMG_OVER_TIME;
      }
    }
  }

  return result_type;
}

void spell_t::execute()
{
  spell_base_t::execute();
}

void spell_t::init()
{
  spell_base_t::init();

  if ( harmful )
    procs_courageous_primal_diamond = false;
}

// ==========================================================================
// Heal
// ==========================================================================

// heal_t::heal_t ======== Heal Constructor =================================

heal_t::heal_t( const std::string&  token,
                player_t*           p,
                const spell_data_t* s ) :
  spell_base_t( ACTION_HEAL, token, p, s ),
  group_only(),
  base_pct_heal(),
  tick_pct_heal(),
  heal_gain( p -> get_gain( name() ) )
{
  if ( sim -> heal_target && target == sim -> target )
    target = sim -> heal_target;
  else if ( target -> is_enemy() )
    target = p;

  weapon_multiplier = 0.0;
  may_crit          = true;
  tick_may_crit     = true;

  stats -> type = STATS_HEAL;

  for ( size_t i = 1; i <= data().effect_count(); i++ )
  {
    parse_effect_data( data().effectN( i ) );
  }
}

void heal_t::activate()
{
  sim -> player_non_sleeping_list.register_callback( [ this ]( player_t* ) {
    target_cache.is_valid = false;
  } );
}

// heal_t::parse_effect_data ================================================

void heal_t::parse_effect_data( const spelleffect_data_t& e )
{
  base_t::parse_effect_data( e );

  if ( e.ok() )
  {
    if ( e.type() == E_HEAL_PCT )
    {
      base_pct_heal = e.percent();
    }
    else if ( e.subtype() == A_OBS_MOD_HEALTH )
      tick_pct_heal = e.percent();
  }
}

dmg_e heal_t::amount_type( const action_state_t* /* state */, bool periodic ) const
{
  if ( periodic )
    return HEAL_OVER_TIME;
  else
    return HEAL_DIRECT;
}

dmg_e heal_t::report_amount_type( const action_state_t* state ) const
{
  dmg_e result_type = state -> result_type;

  // With direct healing, we need to check if this action is a tick action of
  // someone. If so, then the healing should be recorded as periodic.
  if ( result_type == HEAL_DIRECT )
  {
    // Direct ticks are direct damage, that are recorded as ticks
    if ( direct_tick )
      result_type = HEAL_OVER_TIME;
    else
    {
      if ( stats -> action_list.front() -> tick_action == this )
      {
        result_type = HEAL_OVER_TIME;
      }
    }
  }

  return result_type;
}

double heal_t::composite_pct_heal( const action_state_t* ) const
{
  return base_pct_heal;
}

// heal_t::calculate_direct_amount ==========================================

double heal_t::calculate_direct_amount( action_state_t* state ) const
{
  double pct_heal = composite_pct_heal( state );
  if ( pct_heal )
  {
    double amount = state -> target -> resources.max[ RESOURCE_HEALTH ] * pct_heal;

    // Record initial amount to state
    state -> result_raw = amount;

    if ( state -> result == RESULT_CRIT )
    {
      amount *= 1.0 + total_crit_bonus( state );
    }

    amount *= state -> composite_da_multiplier();

    // Record total amount to state
    state -> result_total = amount;
    return amount;

  }

  return base_t::calculate_direct_amount( state );
}

// heal_t::calculate_tick_amount ============================================

double heal_t::calculate_tick_amount( action_state_t* state, double dmg_multiplier ) const
{
  if ( tick_pct_heal )
  {
    double amount = state -> target -> resources.max[ RESOURCE_HEALTH ] * tick_pct_heal;

    // Record initial amount to state
    state -> result_raw = amount;

    if ( state -> result == RESULT_CRIT )
    {
      amount *= 1.0 + total_crit_bonus( state );
    }

    amount *= state -> composite_ta_multiplier();
    amount *= dmg_multiplier; // dot tick multiplier

    // Record total amount to state
    state -> result_total = amount;

    // replicate debug output of calculate_tick_amount
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s amount for %s on %s: ta=%.0f pct=%.3f b_ta=%.0f bonus_ta=%.0f s_mod=%.2f s_power=%.0f a_mod=%.2f a_power=%.0f mult=%.2f",
        player -> name(), name(), target -> name(), amount,
        tick_pct_heal, base_ta( state ), bonus_ta( state ),
        spell_tick_power_coefficient( state ), state -> composite_spell_power(),
        attack_tick_power_coefficient( state ), state -> composite_attack_power(),
        state -> composite_ta_multiplier() );
    }

    return amount;
  }

  return base_t::calculate_tick_amount( state, dmg_multiplier );
}

// heal_t::execute ==========================================================

void heal_t::execute()
{
  spell_base_t::execute();
}

// heal_t::assess_damage ====================================================

void heal_t::assess_damage( dmg_e heal_type,
                            action_state_t* s )
{
  s -> target -> assess_heal( get_school() , heal_type, s );

  if ( heal_type == HEAL_DIRECT )
  {
    if ( sim -> log )
    {
      sim -> out_log.printf( "%s %s heals %s for %.0f (%.0f) (%s)",
                     player -> name(), name(),
                     s -> target -> name(), s -> result_total, s -> result_amount,
                     util::result_type_string( s -> result ) );
    }
  }
  else // HEAL_OVER_TIME
  {
    if ( sim -> log )
    {
      dot_t* dot = get_dot( s -> target );
      sim -> out_log.printf( "%s %s ticks (%d of %d) %s for %.0f (%.0f) heal (%s)",
                     player -> name(), name(),
                     dot -> current_tick, dot -> num_ticks,
                     s -> target -> name(), s -> result_total, s -> result_amount,
                     util::result_type_string( s -> result ) );
    }
  }

  // New callback system; proc spells on impact. 
  // Note: direct_tick_callbacks should not be used with the new system, 
  // override action_t::proc_type() instead
  if ( callbacks )
  {
    proc_types pt = s -> proc_type();
    proc_types2 pt2 = s -> impact_proc_type2();
    if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
      action_callback_t::trigger( player -> callbacks.procs[ pt ][ pt2 ], this, s );
  }

  if ( player -> record_healing() )
  {
    stats -> add_result( s -> result_amount, s -> result_total, ( direct_tick ? HEAL_OVER_TIME : heal_type ), s -> result, s -> block_result, s -> target );

    // Record external healing too
    if ( player != s -> target )
      s -> target -> gains.health -> add( RESOURCE_HEALTH, s -> result_amount, s -> result_total - s -> result_amount );
    else
      heal_gain -> add( RESOURCE_HEALTH, s -> result_amount, s -> result_total - s -> result_amount );
  }
}

// heal_t::find_greatest_difference_player ==================================

player_t* heal_t::find_greatest_difference_player()
{
  double max = 0;
  player_t* max_player = player;
  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    // No love for pets right now
    double diff = p -> is_pet() ? 0 : p -> resources.max[ RESOURCE_HEALTH ] - p -> resources.current[ RESOURCE_HEALTH ];
    if ( diff > max )
    {
      max = diff;
      max_player = p;
    }
  }
  return max_player;
}

// heal_t::find_lowest_player ===============================================

player_t* heal_t::find_lowest_player()
{
  double min = 1.0;
  player_t* max_player = player;

  for ( size_t i = 0; i < sim -> player_no_pet_list.size(); ++i ) // check players only
  {
    player_t* p = sim -> player_no_pet_list[ i ];
    double hp_pct =  p -> resources.pct( RESOURCE_HEALTH );
    if ( hp_pct < min )
    {
      min = hp_pct;
      max_player = p;
    }
  }

  if ( min == 1.0 ) // no player found - check pets
  {
    for ( size_t i = 0; i < sim -> player_list.size(); ++i )
    {
      player_t* p = sim -> player_list[ i ];
      if ( !p -> is_pet() ) continue;
      double hp_pct =  p -> resources.pct( RESOURCE_HEALTH );
      if ( hp_pct < min )
      {
        min = hp_pct;
        max_player = p;
      }
    }
  }

  return max_player;
}

std::vector<player_t*> heal_t::find_lowest_players( int num_players ) const
{
  // vector in which to store lowest N players
  std::vector<player_t*> lowest_N_players = sim -> player_no_pet_list.data();

  while ( lowest_N_players.size() > static_cast< size_t > ( num_players ) )
  {
    // find the remaining player with the highest health
    double max = -1e7;
    size_t max_player_index = 0;
    for ( size_t i = 0; i < lowest_N_players.size(); ++i )
    {
      player_t* p = lowest_N_players[ i ];
      double hp_pct = p -> resources.pct( RESOURCE_HEALTH );
      if ( hp_pct > max )
      {
        max = hp_pct;
        max_player_index = i;
      }
    }
    // remove that player from lowest_N_players
    lowest_N_players.erase( lowest_N_players.begin() + max_player_index );
  }

  return lowest_N_players;
}

// heal_t::smart_target =====================================================

player_t* heal_t::smart_target() const
{
  std::vector<player_t*> injured_targets;
  player_t* target;
  // First check non-pet target
  for( size_t i = 0 ; i < sim -> healing_no_pet_list.size() ; i++ )
  {
    player_t* temp_p = sim -> healing_no_pet_list[ i ];
    if( temp_p -> health_percentage() < 100 )
    {
      injured_targets.push_back( temp_p );
    }
  }
  // Check pets if we didn't find any injured non-pets
  if( injured_targets.empty() )
  {
    for( size_t i = 0 ; i < sim -> healing_pet_list.size() ; i++ )
    {
      player_t* temp_p = sim -> healing_pet_list[ i ];
      if( temp_p -> health_percentage() < 100 )
      {
        injured_targets.push_back( temp_p );
      }
    }
  }
  // Just choose a full-health target if nobody is injured
  if( injured_targets.empty() )
  {
    injured_targets = sim -> healing_no_pet_list.data();
  }
  // Choose a random member of injured_targets
  target = injured_targets[static_cast<size_t>(rng().real() * injured_targets.size() )];
  return target;
}

// heal_t::num_targets ======================================================

int heal_t::num_targets() const
{
  int count = 0;

  for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> actor_list[ i ];

    if ( ! t -> is_sleeping() && ! t -> is_enemy() && ( t != target ) )
      if ( ! group_only || ( t -> party == target -> party ) )
        count++;
  }

  return count;
}

// heal_t::available_targets ================================================

size_t heal_t::available_targets( std::vector< player_t* >& tl ) const
{
  tl.clear();
  tl.push_back( target );

  for ( size_t i = 0, actors = sim -> player_non_sleeping_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> player_non_sleeping_list[ i ];

    if ( t != target )
      if ( ! group_only || ( t -> party == target -> party ) )
        tl.push_back( t );
  }

  return tl.size();
}

// heal_t::create_expression ================================================

expr_t* heal_t::create_expression( const std::string& name )
{
  class heal_expr_t : public expr_t
  {
  public:
    heal_t& heal;

    heal_expr_t( const std::string& name, heal_t& h ) :
      expr_t( name ), heal( h ) {}
  };

  if ( name_str == "active_allies" )
  {
    struct active_allies_expr_t : public heal_expr_t
    {
      active_allies_expr_t( heal_t& h ) : heal_expr_t( "active_allies", h ) {}
      virtual double evaluate() override { return heal.num_targets(); }
    };
    return new active_allies_expr_t( *this );
  }

  return spell_base_t::create_expression( name );
}

// ==========================================================================
// Absorb
// ==========================================================================

// absorb_t::absorb_t ======== Absorb Constructor by Spell Name =============

absorb_t::absorb_t( const std::string&  token,
                    player_t*           p,
                    const spell_data_t* s ) :
  spell_base_t( ACTION_ABSORB, token, p, s ),
  target_specific( false ),
  creator_( target, token, s )
{
  if ( sim -> heal_target && target == sim -> target )
    target = sim -> heal_target;
  else if ( target -> is_enemy() )
    target = p;

  may_crit = false;

  stats -> type = STATS_ABSORB;
}

void absorb_t::activate()
{
  sim -> player_non_sleeping_list.register_callback( [ this ]( player_t* ) {
    target_cache.is_valid = false;
  } );
}

// absorb_t::execute ========================================================

void absorb_t::execute()
{
  spell_base_t::execute();
}

// absorb_t::impact =========================================================

void absorb_t::impact( action_state_t* s )
{
  assess_damage( type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT, s );
}

// absorb_t::assess_damage ==================================================

void absorb_t::assess_damage( dmg_e  /*heal_type*/ , // commented to remove compiler warning, uncomment if needed
                              action_state_t* s )
{
  //s -> result_amount = s -> target -> resource_gain( RESOURCE_HEALTH, s -> result_amount, 0, this );


  if ( target_specific[ s -> target ] == nullptr )
    target_specific[ s -> target ] = create_buff( s );

  if ( result_is_hit( s -> result ) )
  {
    target_specific[ s -> target ] -> trigger( 1, s -> result_amount );
    if ( sim -> log )
      sim -> out_log.printf( "%s %s applies absorb on %s for %.0f (%.0f) (%s)",
                     player -> name(), name(),
                     s -> target -> name(), s -> result_amount, s -> result_total,
                     util::result_type_string( s -> result ) );
  }

  stats -> add_result( 0.0, s -> result_total, ABSORB, s -> result, s -> block_result, s -> target );
}

// absorb_t::available_targets ==============================================

int absorb_t::num_targets() const
{
  int count = 0;
  for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> actor_list[ i ];

    if ( ! t -> is_sleeping() && ! t -> is_enemy() )
      count++;
  }

  return count;
}

// absorb_t::available_targets ==============================================

size_t absorb_t::available_targets( std::vector< player_t* >& tl ) const
{
  tl.clear();
  tl.push_back( target );

  for ( size_t i = 0, actors = sim -> player_non_sleeping_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> player_non_sleeping_list[ i ];

    if ( t != target )
      tl.push_back( t );
  }

  return tl.size();
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Heal
// ==========================================================================

// ==========================================================================
// Created by philoptik@gmail.com
//
// heal_target is set to player for now.
// ==========================================================================

// heal_t::heal_t ======== Heal Constructor ===================

heal_t::heal_t( const std::string&  token,
                player_t*           p,
                const spell_data_t* s ) :
  spell_base_t( ACTION_HEAL, token, p, s )
{
  if ( sim -> heal_target && target == sim -> target )
    target = sim -> heal_target;
  else if ( target -> is_enemy() )
    target = p;

  group_only = false;

  dot_behavior      = DOT_REFRESH;
  weapon_multiplier = 0.0;
  may_crit          = true;
  tick_may_crit     = true;

  stats -> type = STATS_HEAL;

  crit_bonus = 1.0;

  crit_multiplier = 1.0;

  if ( player -> meta_gem == META_REVITALIZING_SHADOWSPIRIT )
  {
    crit_multiplier *= 1.03;
  }
}

// heal_t::execute ==========================================================

void heal_t::execute()
{
  spell_base_t::execute();

  if ( callbacks )
  {
    if ( result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.heal[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> callbacks.heal[ RESULT_NONE ], this );
    }
  }
}

// heal_t::assess_damage ====================================================

void heal_t::assess_damage( dmg_e heal_type,
                            action_state_t* state )
{
  heal_state_t* s = debug_cast<heal_state_t*>( state );

  s -> target -> assess_heal( school, heal_type, s );

  if ( heal_type == HEAL_DIRECT )
  {
    if ( sim -> log )
    {
      sim -> output( "%s %s heals %s for %.0f (%.0f) (%s)",
                     player -> name(), name(),
                     s -> target -> name(), s -> total_result_amount, s -> result_amount,
                     util::result_type_string( s -> result ) );
    }

    if ( callbacks && ! direct_tick_callbacks ) action_callback_t::trigger( player -> callbacks.direct_heal[ school ], this, s );
    if ( direct_tick_callbacks ) action_callback_t::trigger( player -> callbacks.tick_heal[ school ], this, s );
  }
  else // HEAL_OVER_TIME
  {
    if ( sim -> log )
    {
      dot_t* dot = get_dot( s -> target );
      sim -> output( "%s %s ticks (%d of %d) %s for %.0f (%.0f) heal (%s)",
                     player -> name(), name(),
                     dot -> current_tick, dot -> num_ticks,
                     s -> target -> name(), s -> total_result_amount, s -> result_amount,
                     util::result_type_string( s -> result ) );
    }

    if ( callbacks ) action_callback_t::trigger( player -> callbacks.tick_heal[ school ], this, s );
  }

  stats -> add_result( s -> result_amount, s -> total_result_amount, ( direct_tick ? HEAL_OVER_TIME : heal_type ), s -> result );
}

// heal_t::find_greatest_difference_player ==================================

player_t* heal_t::find_greatest_difference_player()
{
  double diff=0;
  double max=0;
  player_t* max_player = player;
  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    // No love for pets right now
    diff = p -> is_pet() ? 0 : p -> resources.max[ RESOURCE_HEALTH ] - p -> resources.current[ RESOURCE_HEALTH ];
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
  double min=1.0;
  player_t* max_player = player;

  for ( size_t i = 0; i < sim -> player_list.size(); ++i ) // check players only
  {
    player_t* p = sim -> player_list[ i ];
    if ( p -> is_pet() ) continue;
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

// heal_t::available_targets ==============================================

size_t heal_t::available_targets( std::vector< player_t* >& tl )
{
  // TODO: This does not work for heals at all, as it presumes enemies in the
  // actor list.

  if ( group_only )
  {
    tl.push_back( target );

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      if ( ! sim -> actor_list[ i ] -> current.sleeping &&
           ! sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] != target && sim -> actor_list[ i ] -> party == target -> party )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }

  return spell_base_t::available_targets( tl );
}

heal_state_t::heal_state_t( action_t* a, player_t* t ) :
  action_state_t( a, t ),
  total_result_amount( 0 )
{

}

void heal_state_t::copy_state( const action_state_t* o )
{
  action_state_t::copy_state( o );

  if ( o )
  {
    const heal_state_t* other = debug_cast<const heal_state_t*>( o );
    this -> total_result_amount = other -> total_result_amount;
  }
}

action_state_t* heal_t::get_state( const action_state_t* state )
{
  action_state_t* s = spell_base_t::get_state( state );
  heal_state_t* hs = debug_cast< heal_state_t* >( s );

  hs -> total_result_amount = 0.0;

  return s;
}

bool heal_t::is_valid_target( player_t* t )
{
  return ( ! t -> current.sleeping && ! t -> is_enemy() );
}

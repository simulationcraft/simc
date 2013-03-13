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
  spell_base_t( ACTION_HEAL, token, p, s ),
  group_only(),
  pct_heal()
{
  if ( sim -> heal_target && target == sim -> target )
    target = sim -> heal_target;
  else if ( target -> is_enemy() )
    target = p;

  weapon_multiplier = 0.0;
  may_crit          = true;
  tick_may_crit     = true;
  harmful = false;

  stats -> type = STATS_HEAL;
}

// heal_t::parse_effect_data ===========================

void heal_t::parse_effect_data( const spelleffect_data_t& e )
{
  base_t::parse_effect_data( e );

  if ( e.ok() )
  {
    if ( e.type() == E_HEAL_PCT )
    {
      pct_heal = e.average( player );
    }
  }
}

// heal_t::calculate_direct_damage ====================================

double heal_t::calculate_direct_damage( result_e r,
                                        int chain_target,
                                        double attack_power,
                                        double spell_power,
                                        double multiplier,
                                        player_t* t )
{
  if ( pct_heal )
    return t -> resources.max[ RESOURCE_HEALTH ] * pct_heal;

  return base_t::calculate_direct_damage( r,
                                          chain_target,
                                          attack_power,
                                          spell_power,
                                          multiplier,
                                          t );
}
// heal_t::execute ==========================================================

void heal_t::execute()
{
  spell_base_t::execute();

  if ( callbacks )
  {
    result_e r = execute_state ? execute_state -> result : result;
    if ( r != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.heal[ r ], this );
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

// heal_t::num_targets =====================================================

int heal_t::num_targets()
{
  int count = 0;

  for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> actor_list[ i ];

    if ( ! t -> current.sleeping && ! t -> is_enemy() && ( t != target ) )
      if ( ! group_only || ( t -> party == target -> party ) )
        count++;
  }

  return count;
}

// heal_t::available_targets ==============================================

size_t heal_t::available_targets( std::vector< player_t* >& tl )
{
  tl.clear();
  tl.push_back( target );

  for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> actor_list[ i ];

    if ( ! t -> current.sleeping && ! t -> is_enemy() && ( t != target ) )
      if ( ! group_only || ( t -> party == target -> party ) )
        tl.push_back( t );
  }

  return tl.size();
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
      virtual double evaluate() { return heal.num_targets(); }
    };
    return new active_allies_expr_t( *this );
  }

  return spell_base_t::create_expression( name );
}

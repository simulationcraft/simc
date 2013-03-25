// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Absorb
// ==========================================================================

// ==========================================================================
// Created by philoptik@gmail.com
//
// heal_target is set to player for now.
// dmg_e = ABSORB, all crits killed
// ==========================================================================

// absorb_t::absorb_t ======== Absorb Constructor by Spell Name =============

absorb_t::absorb_t( const std::string&  token,
                    player_t*           p,
                    const spell_data_t* s ) :
  spell_base_t( ACTION_ABSORB, token, p, s )
{
  if ( sim -> heal_target && target == sim -> target )
    target = sim -> heal_target;
  else if ( target -> is_enemy() )
    target = p;

  may_crit = false;

  stats -> type = STATS_ABSORB;
}

// absorb_t::execute ========================================================

void absorb_t::execute()
{
  spell_base_t::execute();

  if ( harmful && callbacks )
  {
    result_e r = execute_state ? execute_state -> result : result;
    if ( r != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.absorb[ r ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> callbacks.absorb[ RESULT_NONE ], this );
    }
  }
}

// absorb_t::impact =========================================================

void absorb_t::impact( action_state_t* s )
{
  if ( s -> result_amount > 0 )
  {
    assess_damage( ABSORB, s );
  }
}

// absorb_t::assess_damage ==================================================

void absorb_t::assess_damage( dmg_e    heal_type,
                              action_state_t* state )
{
  heal_state_t* s = debug_cast<heal_state_t*>( state );

  s -> total_result_amount = s -> result_amount;
  s -> result_amount = s -> target -> resource_gain( RESOURCE_HEALTH, s -> result_amount, 0, this );

  if ( sim -> log )
    sim -> output( "%s %s heals %s for %.0f (%.0f) (%s)",
                   player -> name(), name(),
                   s -> target -> name(), s -> result_amount, s -> total_result_amount,
                   util::result_type_string( result ) );

  stats -> add_result( s -> result_amount, s -> total_result_amount, heal_type, s -> result, s -> target );
}


action_state_t* absorb_t::get_state( const action_state_t* state )
{
  action_state_t* s = spell_base_t::get_state( state );
  heal_state_t* hs = debug_cast< heal_state_t* >( s );

  hs -> total_result_amount = 0.0;

  return s;
}

// absorb_t::available_targets ==============================================

int absorb_t::num_targets()
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

size_t absorb_t::available_targets( std::vector< player_t* >& tl )
{
  tl.clear();
  tl.push_back( target );

  for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> actor_list[ i ];

    if ( ! t -> is_sleeping() && ! t -> is_enemy() && ( t != target ) )
      tl.push_back( t );
  }

  return tl.size();
}

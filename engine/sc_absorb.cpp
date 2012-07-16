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
  if ( target -> is_enemy() || target -> is_add() )
    target = player;

  may_crit = false;
  stateless = true;

  stats -> type = STATS_ABSORB;
}

// absorb_t::execute ========================================================

void absorb_t::execute()
{
#ifndef NDEBUG
  if ( !stateless ) // Safety check to ensure stateless flag never gets turned off. Remove when non-stateless system is discontinued.
    assert( 0 );
#endif

  spell_base_t::execute();

  if ( harmful && callbacks )
  {
    if ( ! is_tick_action && result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.absorb[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> callbacks.absorb[ RESULT_NONE ], this );
    }
  }
}

// absorb_t::impact_s =========================================================

void absorb_t::impact_s( action_state_t* s )
{
  if ( s -> result_amount > 0 )
  {
    assess_damage( s -> target, s -> result_amount, ABSORB, s -> result );
  }
}

// absorb_t::assess_damage ==================================================

void absorb_t::assess_damage( player_t*     t,
                              double        heal_amount,
                              dmg_e    heal_type,
                              result_e heal_result,
                              action_state_t* )
{
  direct_dmg = t -> resource_gain( RESOURCE_HEALTH, heal_amount, 0, this );

  if ( sim -> log )
    sim -> output( "%s %s heals %s for %.0f (%.0f) (%s)",
                   player -> name(), name(),
                   t -> name(), direct_dmg, heal_amount,
                   util::result_type_string( result ) );

  stats -> add_result( direct_dmg, heal_amount, heal_type, heal_result );
}


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

  stats -> type = STATS_ABSORB;
}

// absorb_t::execute ========================================================

void absorb_t::execute()
{
  spell_base_t::execute();

  if ( harmful && callbacks )
  {
    if ( result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.absorb[ result ], this );
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
                              action_state_t* s)
{
  direct_dmg = s -> target -> resource_gain( RESOURCE_HEALTH, s -> result_amount, 0, this );

  if ( sim -> log )
    sim -> output( "%s %s heals %s for %.0f (%.0f) (%s)",
                   player -> name(), name(),
                   s -> target -> name(), direct_dmg, s -> result_amount,
                   util::result_type_string( result ) );

  stats -> add_result( direct_dmg, s -> result_amount, heal_type, s -> result );
}


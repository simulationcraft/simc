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
// dmg_type_e = ABSORB, all crits killed
// ==========================================================================

// absorb_t::init_absorb_t_ == Absorb Constructor Initializations ===========

void absorb_t::init_absorb_t_()
{
  if ( target -> is_enemy() || target -> is_add() )
    target = player;

  total_heal = total_actual = 0;

  may_crit = false;

  stats -> type = STATS_ABSORB;
}

// absorb_t::absorb_t ======== Absorb Constructor by Spell Name =============

absorb_t::absorb_t( const std::string& n, player_t* p, const char* sname, talent_tree_type_e t ) :
  spell_base_t( ACTION_ABSORB, n, sname, p, t )
{ init_absorb_t_(); }

// absorb_t::absorb_t ======== absorb Constructor by Spell ID ===============

absorb_t::absorb_t( const std::string& n, player_t* p, const uint32_t id, talent_tree_type_e t ) :
  spell_base_t( ACTION_ABSORB, n, id, p, t )
{ init_absorb_t_(); }

// absorb_t::player_buff ====================================================

void absorb_t::player_buff()
{
  spell_base_t::player_buff();

  player_multiplier    = player -> composite_player_absorb_multiplier   ( school );

  if ( sim -> debug ) log_t::output( sim, "absorb_t::player_buff: %s mult=%.2f",
                                     name(), player_multiplier );
}

// absorb_t::execute ========================================================

void absorb_t::execute()
{
  total_heal = 0;

  spell_base_t::execute();

  if ( harmful && callbacks )
  {
    if ( result != RESULT_NONE )
    {
      //action_callback_t::trigger( player -> absorb_callbacks[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      //action_callback_t::trigger( player -> absorb_callbacks[ RESULT_NONE ], this );
    }
  }
}

// absorb_t::impact =========================================================

void absorb_t::impact( player_t* t, result_type_e impact_result, double travel_dmg=0 )
{
  if ( travel_dmg > 0 )
  {
    assess_damage( t, travel_dmg, ABSORB, impact_result );
  }
}

// absorb_t::assess_damage ==================================================

void absorb_t::assess_damage( player_t*     t,
                              double        heal_amount,
                              dmg_type_e    heal_type,
                              result_type_e heal_result )
{
  double heal_actual = direct_dmg = t -> resource_gain( RESOURCE_HEALTH, heal_amount, 0, this );

  total_heal   += heal_amount;
  total_actual += heal_actual;

  if ( sim -> log )
    log_t::output( sim, "%s %s heals %s for %.0f (%.0f) (%s)",
                   player -> name(), name(),
                   t -> name(), heal_actual, heal_amount,
                   util_t::result_type_string( result ) );

  stats -> add_result( heal_actual, heal_amount, heal_type, heal_result );
}


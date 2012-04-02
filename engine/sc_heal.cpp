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

// heal_t::init_heal_t_ == Heal Constructor Initializations =================

void heal_t::init_heal_t_()
{
  if ( target -> is_enemy() || target -> is_add() )
    target = player;

  group_only = false;

  total_heal = total_actual = 0;

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

// heal_t::heal_t ======== Heal Constructor by Spell Name ===================

heal_t::heal_t( const std::string&  n,
                player_t*           p,
                const char*         sname,
                talent_tree_type_e  t ) :
  spell_base_t( ACTION_HEAL, n, sname, p, t )
{
  init_heal_t_();
}

// heal_t::heal_t ======== Heal Constructor by Spell ID =====================

heal_t::heal_t( const std::string&  n,
                player_t*           p,
                const uint32_t      id,
                talent_tree_type_e  t ) :
  spell_base_t( ACTION_HEAL, n, id, p, t )
{
  init_heal_t_();
}

// heal_t::player_buff ======================================================

void heal_t::player_buff()
{
  spell_base_t::player_buff();

  player_t* p = player;

  player_multiplier    = p -> composite_player_heal_multiplier( school );
  player_dd_multiplier = p -> composite_player_dh_multiplier( school );
  player_td_multiplier = p -> composite_player_th_multiplier( school );


  if ( sim -> debug ) log_t::output( sim, "heal_t::player_buff: %s mult=%.2f dd_mult=%.2f td_mult=%.2f",
                                     name(), player_multiplier, player_dd_multiplier, player_td_multiplier );
}

// heal_t::execute ==========================================================

void heal_t::execute()
{
  total_heal = 0;

  spell_base_t::execute();

  if ( harmful && callbacks )
  {
    if ( result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> heal_callbacks[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> heal_callbacks[ RESULT_NONE ], this );
    }
  }
}

// heal_t::assess_damage ====================================================

void heal_t::assess_damage( player_t* t,
                            double heal_amount,
                            dmg_type_e heal_type,
                            result_type_e heal_result )
{
  player_t::heal_info_t heal = t -> assess_heal( heal_amount, school, heal_type, heal_result, this );

  total_heal   += heal.amount;
  total_actual += heal.actual;

  if ( heal_type == HEAL_DIRECT )
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s %s heals %s for %.0f (%.0f) (%s)",
                     player -> name(), name(),
                     t -> name(), heal.amount, heal.actual,
                     util_t::result_type_string( result ) );
    }

    if ( callbacks ) action_callback_t::trigger( player -> direct_heal_callbacks[ school ], this );
  }
  else // HEAL_OVER_TIME
  {
    if ( sim -> log )
    {
      dot_t* dot = this -> dot();
      log_t::output( sim, "%s %s ticks (%d of %d) %s for %.0f (%.0f) heal (%s)",
                     player -> name(), name(),
                     dot -> current_tick, dot -> num_ticks,
                     t -> name(), heal.amount, heal.actual,
                     util_t::result_type_string( result ) );
    }

    if ( callbacks ) action_callback_t::trigger( player -> tick_heal_callbacks[ school ], this );
  }

  stats -> add_result( sim -> report_overheal ? heal.amount : heal.actual, heal.actual, ( direct_tick ? HEAL_OVER_TIME : heal_type ), heal_result );
}

// heal_t::find_greatest_difference_player ==================================

player_t* heal_t::find_greatest_difference_player()
{
  double diff=0;
  double max=0;
  player_t* max_player = player;
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
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
  double diff=0;
  double max=0;
  player_t* max_player = player;
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    // No love for pets right now
    diff =  p -> is_pet() ? 0 : 1.0 / p -> resources.current[ RESOURCE_HEALTH ];
    if ( diff > max )
    {
      max = diff;
      max_player = p;
    }
  }
  return max_player;
}

// heal_t::available_targets ==============================================

size_t heal_t::available_targets( std::vector< player_t* >& tl ) const
{
  // TODO: This does not work for heals at all, as it presumes enemies in the
  // actor list.

  if ( group_only )
  {
    tl.push_back( target );

    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      if ( ! sim -> actor_list[ i ] -> sleeping &&
           ! sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] != target && sim -> actor_list[ i ] -> party == target -> party )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }

  return spell_base_t::available_targets( tl );
}

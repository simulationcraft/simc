// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

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

  base_spell_power_multiplier = 1.0;
  min_gcd = timespan_t::from_seconds( 1.0 );

  group_only = false;

  total_heal = total_actual = 0;

  dot_behavior      = DOT_REFRESH;
  weapon_multiplier = 0.0;
  may_crit          = true;
  tick_may_crit     = true;
  may_trigger_dtr   = false;
  hasted_ticks = true;

  stats -> type = STATS_HEAL;

  crit_bonus = 1.0;

  crit_multiplier = 1.0;

  if ( player -> meta_gem == META_REVITALIZING_SHADOWSPIRIT )
  {
    crit_multiplier *= 1.03;
  }
}

// heal_t::heal_t ======== Heal Constructor by Spell Name ===================

heal_t::heal_t( const char* n, player_t* player, const char* sname, int t ) :
  action_t( ACTION_HEAL, n, sname, player, t )
{
  init_heal_t_();
}

// heal_t::heal_t ======== Heal Constructor by Spell ID =====================

heal_t::heal_t( const char* n, player_t* player, const uint32_t id, int t ) :
    action_t( ACTION_HEAL, n, id, player, t )
{
  init_heal_t_();
}

// heal_t::player_buff ======================================================

void heal_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_multiplier    = p -> composite_player_heal_multiplier( school );
  player_dd_multiplier = p -> composite_player_dh_multiplier( school );
  player_td_multiplier = p -> composite_player_th_multiplier( school );

  player_crit = p -> composite_spell_crit();

  if ( sim -> debug ) log_t::output( sim, "heal_t::player_buff: %s mult=%.2f dd_mult=%.2f td_mult=%.2f crit=%.2f",
                                     name(), player_multiplier, player_dd_multiplier, player_td_multiplier, player_crit );
}

// heal_t::haste ============================================================

double heal_t::haste() const
{
  return player -> composite_spell_haste();
}

// heal_t::gcd =============================================================

timespan_t heal_t::gcd() const
{
  timespan_t t = action_t::gcd();
  if ( t == timespan_t::zero ) return timespan_t::zero;

  t *= haste();
  if ( t < min_gcd ) t = min_gcd;

  return t;
}

// heal_t::execute_time ====================================================

timespan_t heal_t::execute_time() const
{
  timespan_t t = base_execute_time;

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero;

  if ( player -> buffs.corruption_absolute -> up() )
    return timespan_t::zero;

  if ( t <= timespan_t::zero ) return timespan_t::zero;
  t *= haste();

  return t;
}

// heal_t::execute ==========================================================

void heal_t::execute()
{
  total_heal = 0;

  action_t::execute();

  if ( player -> last_foreground_action == this )
    player -> debuffs.casting -> expire();

  if ( harmful && callbacks )
  {
    if ( result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> heal_callbacks[ result ], this );
      action_callback_t::trigger( player -> spell_callbacks[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> heal_callbacks[ RESULT_NONE ], this );
      action_callback_t::trigger( player -> spell_callbacks[ RESULT_NONE ], this );
    }
  }
}

// heal_t::calculate_result =================================================

void heal_t::calculate_result()
{
  direct_dmg = 0;
  result = RESULT_NONE;

  if ( ! harmful ) return;

  result = RESULT_HIT;

  if ( may_crit )
  {
    if ( rng[ RESULT_CRIT ] -> roll( crit_chance( 0 ) ) )
    {
      result = RESULT_CRIT;
    }
  }

  if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

// heal_t::crit_chance =====================================================

double heal_t::crit_chance( int /* delta_level */ ) const
{
  return total_crit();
}

// heal_t::schedule_execute ================================================

void heal_t::schedule_execute()
{
  action_t::schedule_execute();

  if ( time_to_execute > timespan_t::zero )
    player -> debuffs.casting -> trigger();
}

// heal_t::assess_damage ====================================================

void heal_t::assess_damage( player_t* t,
                            double heal_amount,
                            int    heal_type,
                            int    heal_result )
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
    diff = p -> is_pet() ? 0 : p -> resource_max[ RESOURCE_HEALTH ] - p -> resource_current[ RESOURCE_HEALTH ];
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
    diff =  p -> is_pet() ? 0 : 1.0 / p -> resource_current[ RESOURCE_HEALTH ];
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
         !sim -> actor_list[ i ] -> is_enemy() &&
         sim -> actor_list[ i ] != target && sim -> actor_list[ i ] -> party == target -> party )
      tl.push_back( sim -> actor_list[ i ] );
  }

  return tl.size();
  }

  return action_t::available_targets( tl );
}

// ==========================================================================
// Absorb
// ==========================================================================

// ==========================================================================
// Created by philoptik@gmail.com
//
// heal_target is set to player for now.
// dmg_type = ABSORB, all crits killed
// ==========================================================================

// absorb_t::init_absorb_t_ == Absorb Constructor Initializations ===========

void absorb_t::init_absorb_t_()
{
  if ( target -> is_enemy() || target -> is_add() )
    target = player;

  base_spell_power_multiplier = 1.0;
  min_gcd = timespan_t::from_seconds( 1.0 );

  total_heal = total_actual = 0;
  may_trigger_dtr   = false;

  weapon_multiplier = 0.0;

  stats -> type = STATS_ABSORB;
}

// absorb_t::absorb_t ======== Absorb Constructor by Spell Name =============

absorb_t::absorb_t( const char* n, player_t* player, const char* sname, int t ) :
  action_t( ACTION_ABSORB, n, sname, player, t )
{
  init_absorb_t_();
}

// absorb_t::absorb_t ======== absorb Constructor by Spell ID ===============

absorb_t::absorb_t( const char* n, player_t* player, const uint32_t id, int t ) :
  action_t( ACTION_ABSORB, n, id, player, t )
{
  init_absorb_t_();
}

// absorb_t::player_buff ====================================================

void absorb_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_multiplier    = p -> composite_player_absorb_multiplier   ( school );

  player_crit = p -> composite_spell_crit();

  if ( sim -> debug ) log_t::output( sim, "absorb_t::player_buff: %s crit=%.2f",
                                     name(), player_crit );
}

// absorb_t::haste ==========================================================

double absorb_t::haste() const
{
  return player -> composite_spell_haste();
}

// absorb_t::gcd =============================================================

timespan_t absorb_t::gcd() const
{
  timespan_t t = action_t::gcd();
  if ( t == timespan_t::zero ) return timespan_t::zero;

  t *= haste();
  if ( t < min_gcd ) t = min_gcd;

  return t;
}

// absorb_t::execute_time ====================================================

timespan_t absorb_t::execute_time() const
{
  timespan_t t = base_execute_time;

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero;

  if ( player -> buffs.corruption_absolute -> up() )
    return timespan_t::zero;

  if ( t <= timespan_t::zero ) return timespan_t::zero;
  t *= haste();

  return t;
}

// absorb_t::execute ========================================================


void absorb_t::execute()
{
  total_heal = 0;

  action_t::execute();

  if ( player -> last_foreground_action == this )
    player -> debuffs.casting -> expire();

  if ( harmful && callbacks )
  {
    if ( result != RESULT_NONE )
    {
      //action_callback_t::trigger( player -> heal_callbacks[ result ], this );
      action_callback_t::trigger( player -> spell_callbacks[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      //action_callback_t::trigger( player -> heal_callbacks[ RESULT_NONE ], this );
      action_callback_t::trigger( player -> spell_callbacks[ RESULT_NONE ], this );
    }
  }
}

// absorb_t::impact =========================================================

void absorb_t::impact( player_t* t, int impact_result, double travel_dmg=0 )
{
  if ( travel_dmg > 0 )
  {
    assess_damage( t, travel_dmg, ABSORB, impact_result );
  }
}

// absorb_t::assess_damage ==================================================

void absorb_t::assess_damage( player_t* t,
                              double    heal_amount,
                              int       heal_type,
                              int       heal_result )
{
  double heal_actual = direct_dmg = t -> resource_gain( RESOURCE_HEALTH, heal_amount, 0, this );

  total_heal   += heal_amount;
  total_actual += heal_actual;

  if ( heal_type == ABSORB )
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s %s heals %s for %.0f (%.0f) (%s)",
                     player -> name(), name(),
                     t -> name(), heal_actual, heal_amount,
                     util_t::result_type_string( result ) );
    }

  }

  stats -> add_result( heal_actual, heal_amount, heal_type, heal_result );
}

// absorb_t::calculate_result ===============================================

void absorb_t::calculate_result()
{
  direct_dmg = 0;
  result = RESULT_NONE;

  if ( ! harmful ) return;

  result = RESULT_HIT;

  if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

// spell_t::schedule_execute ================================================

void absorb_t::schedule_execute()
{
  action_t::schedule_execute();

  if ( time_to_execute > timespan_t::zero )
    player -> debuffs.casting -> trigger();
}

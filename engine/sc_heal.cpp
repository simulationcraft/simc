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

  total_heal = total_actual = 0;

  dot_behavior      = DOT_REFRESH;
  weapon_multiplier = 0.0;
  may_crit          = true;
  tick_may_crit     = true;
  may_trigger_dtr   = false;

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

  player_crit = p -> composite_spell_crit();

  if ( sim -> debug ) log_t::output( sim, "heal_t::player_buff: %s crit=%.2f",
                                     name(), player_crit );
}

// heal_t::haste ============================================================

double heal_t::haste() const
{
  return player -> composite_spell_haste();
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
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> heal_callbacks[ RESULT_NONE ], this );
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
  target = player -> target;
  if ( target -> is_enemy() || target -> is_add() )
    target = player;
  heal_target.push_back( target );

  total_heal = total_actual = 0;
  may_trigger_dtr   = false;

  weapon_multiplier = 0.0;

  stats -> type = STATS_ABSORB;
}

// absorb_t::absorb_t ======== Absorb Constructor by Spell Name =============

absorb_t::absorb_t( const char* n, player_t* player, const char* sname, int t ) :
  spell_t( n, sname, player, t )
{
  init_absorb_t_();
}

// absorb_t::absorb_t ======== absorb Constructor by Spell ID ===============

absorb_t::absorb_t( const char* n, player_t* player, const uint32_t id, int t ) :
  spell_t( n, id, player, t )
{
  init_absorb_t_();
}

// absorb_t::parse_options ==================================================

void absorb_t::parse_options( option_t*          options,
                              const std::string& options_str )
{
  spell_t::parse_options( options, options_str );

  if ( target )
  {
    heal_target.clear();
    heal_target.push_back( target );
  }
}

// absorb_t::player_buff ====================================================

void absorb_t::player_buff()
{
  player_multiplier              = 1.0;
  player_dd_multiplier           = 1.0;
  player_td_multiplier           = 1.0;
  player_hit                     = 0;
  player_crit                    = 0;
  player_penetration             = 0;
  player_dd_adder                = 0;
  player_spell_power             = 0;
  player_attack_power            = 0;
  player_spell_power_multiplier  = 1.0;
  player_attack_power_multiplier = 1.0;

  player_t* p = player;

  player_multiplier    = p -> composite_player_absorb_multiplier   ( school );

  if ( base_attack_power_multiplier > 0 )
  {
    player_attack_power            = p -> composite_attack_power();
    player_attack_power_multiplier = p -> composite_attack_power_multiplier();
  }

  if ( base_spell_power_multiplier > 0 )
  {
    player_spell_power            = p -> composite_spell_power( school );
    player_spell_power_multiplier = p -> composite_spell_power_multiplier();
  }

  player_haste = haste();

  if ( sim -> debug )
    log_t::output( sim, "absorb_t::player_buff: %s hit=%.2f crit=%.2f pen=%.0f sp=%.2f ap=%.2f mult=%.2f",
                   name(), player_hit, player_crit, player_penetration,
                   player_spell_power, player_attack_power, player_multiplier );
}

// absorb_t::target_debuff ==================================================

void absorb_t::target_debuff( player_t* /* t */, int /* dmg_type */ )
{
  target_multiplier            = 1.0;
  target_hit                   = 0;
  target_crit                  = 0;
  target_attack_power          = 0;
  target_spell_power           = 0;
  target_penetration           = 0;
  target_dd_adder              = 0;

  if ( sim -> debug )
    log_t::output( sim, "absorb_t::target_debuff: %s hit=%.2f crit=%.2f pen=%.0f ap=%.2f sp=%.2f mult=%.2f",
                   name(), target_hit, target_crit, target_penetration,
                   target_attack_power, target_spell_power, target_multiplier );
}

// absorb_t::haste ==========================================================

double absorb_t::haste() const
{
  double h = spell_t::haste();

  return h;
}

// absorb_t::execute ========================================================

void absorb_t::execute()
{
  if ( ! initialized )
  {
    sim -> errorf( "action_t::execute: action %s from player %s is not initialized.\n", name(), player -> name() );
    assert( 0 );
  }

  if ( sim -> log && ! dual )
  {
    log_t::output( sim, "%s performs %s (%.0f)", player -> name(), name(),
                   player -> resource_current[ player -> primary_resource() ] );
  }

  if ( harmful )
  {
    if ( player -> in_combat == false && sim -> debug )
      log_t::output( sim, "%s enters combat.", player -> name() );

    player -> in_combat = true;
  }

  player_buff();

  for ( unsigned int i = 0; i < heal_target.size(); i++ )
  {
    target_debuff( heal_target[i], HEAL_DIRECT );

    calculate_result();

    direct_dmg = calculate_direct_damage();

    schedule_travel( heal_target[i] );
  }

  consume_resource();

  update_ready();

  if ( ! dual ) stats -> add_execute( time_to_execute );

  if ( repeating && ! proc ) schedule_execute();

  // Add options found in spell_t::execute()
  if ( player -> last_foreground_action == this )
    player -> debuffs.casting -> expire();

  if ( harmful && callbacks )
  {
    if ( ! background ) // OnSpellCast
    {
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

// absorb_t::calculate_direct_damage ========================================

double absorb_t::calculate_direct_damage( int )
{
  double dmg = sim -> range( base_dd_min, base_dd_max );

  if ( round_base_dmg ) dmg = floor( dmg + 0.5 );

  if ( dmg == 0 && weapon_multiplier == 0 && direct_power_mod == 0 ) return 0;

  double base_direct_dmg = dmg;

  dmg += base_dd_adder + player_dd_adder + target_dd_adder;

  dmg += direct_power_mod * total_power();
  dmg *= total_dd_multiplier();

  double init_direct_dmg = dmg;

  if ( ! sim -> average_range ) dmg = floor( dmg + sim -> real() );

  if ( sim -> debug )
  {
    log_t::output( sim, "%s heal for %s: dd=%.0f i_dd=%.0f b_dd=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f",
                   player -> name(), name(), dmg, init_direct_dmg, base_direct_dmg, direct_power_mod,
                   total_power(), base_multiplier * base_dd_multiplier, player_multiplier, target_multiplier );
  }

  return dmg;
}

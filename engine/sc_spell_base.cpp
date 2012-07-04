// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Spell
// ==========================================================================

// spell_base_t::spell_base_t =========================================================

spell_base_t::spell_base_t( action_e at,
                            const std::string& token,
                            player_t* p,
                            const spell_data_t* s ) :
  action_t( at, token, p, s )
{
  base_spell_power_multiplier = 1.0;

  min_gcd = timespan_t::from_seconds( 1.0 );

  hasted_ticks = true;
  special = true;
}

// spell_base_t::haste ===========================================================

double spell_base_t::haste()
{
  return player -> composite_spell_haste();
}

// spell_base_t::gcd =============================================================

timespan_t spell_base_t::gcd()
{
  timespan_t t = action_t::gcd();
  if ( t == timespan_t::zero() ) return timespan_t::zero();

  t *= haste();
  if ( t < min_gcd ) t = min_gcd;

  return t;
}

// spell_base_t::execute_time ====================================================

timespan_t spell_base_t::execute_time()
{
  timespan_t t = base_execute_time;

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero();

  if ( t <= timespan_t::zero() ) return timespan_t::zero();
  t *= haste();

  return t;
}

// spell_base_t::tick_time ======================================================

timespan_t spell_base_t::tick_time( double haste )
{
  if ( ! harmful && ! player -> in_combat )
      return timespan_t::zero();

  return action_t::tick_time( haste );
}

// spell_base_t::player_buff =====================================================

void spell_base_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_crit = p -> composite_spell_crit();

  if ( sim -> debug ) sim -> output( "spell_base_t::player_buff: %s crit=%.2f",
                                     name(), player_crit );
}

// spell_base_t::calculate_result ================================================

result_e spell_base_t::calculate_result( double crit, unsigned target_level )
{
  int delta_level = target_level - player -> level;

  direct_dmg = 0;
  result_e result = RESULT_NONE;

  if ( ! harmful || ! may_hit ) return RESULT_NONE;

  if ( ( result == RESULT_NONE ) && may_miss )
  {
    if ( rng_result -> roll( miss_chance( composite_hit(), delta_level ) ) )
    {
      result = RESULT_MISS;
    }
  }

  if ( result == RESULT_NONE )
  {
    result = RESULT_HIT;

    if ( may_crit )
    {
      if ( rng_result -> roll( crit_chance( crit, delta_level ) ) )
        result = RESULT_CRIT;
    }
  }

  if ( sim -> debug ) sim -> output( "%s result for %s is %s", player -> name(), name(), util::result_type_string( result ) );

  return result;
}

// spell_base_t::execute =========================================================

void spell_base_t::execute()
{
  action_t::execute();

  if ( player -> last_foreground_action == this )
    player -> debuffs.casting -> expire();

  if ( callbacks )
  {
    if ( ( execute_state ? execute_state -> result : result ) != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.spell[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> callbacks.spell[ RESULT_NONE ], this );
    }
  }
}

// spell_base_t::schedule_execute ================================================

void spell_base_t::schedule_execute()
{
  action_t::schedule_execute();

  if ( time_to_execute > timespan_t::zero() )
    player -> debuffs.casting -> trigger();
}

void spell_base_t::init()
{
  action_t::init();

  if ( base_spell_power_multiplier > 0 && ( direct_power_mod > 0 || tick_power_mod > 0
    || ( tick_action && tick_action -> direct_power_mod > 0 ) ) )
    snapshot_flags |= STATE_SP;

  if ( base_attack_power_multiplier > 0 && ( weapon_power_mod > 0 || direct_power_mod > 0 || tick_power_mod > 0
    || ( tick_action && ( tick_action -> direct_power_mod > 0 || tick_action -> weapon_power_mod > 0 ) ) ) )
    snapshot_flags |= STATE_AP;

  if ( num_ticks > 0 && ( hasted_ticks || channeled ) )
    snapshot_flags |= STATE_HASTE;
}

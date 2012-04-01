// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Spell
// ==========================================================================

// spell_base_t::spell_base_t =========================================================

void spell_base_t::init_spell_base_t_()
{
  base_spell_power_multiplier = 1.0;

  min_gcd = timespan_t::from_seconds( 1.0 );

  hasted_ticks = true;
}

spell_base_t::spell_base_t( action_type_e at, const spell_id_t& s, talent_tree_type_e t ) :
  action_t( at, s, t, true )
{
  init_spell_base_t_();
}

spell_base_t::spell_base_t( action_type_e at, const std::string& n, player_t* p, resource_type_e r, school_type_e s, talent_tree_type_e t ) :
  action_t( at, n, p, r, s, t, true )
{
  init_spell_base_t_();
}

spell_base_t::spell_base_t( action_type_e at, const std::string& n, const char* sname, player_t* p, talent_tree_type_e t ) :
  action_t( at, n, sname, p, t, true )
{
  init_spell_base_t_();
}

spell_base_t::spell_base_t( action_type_e at, const std::string& n, const uint32_t id, player_t* p, talent_tree_type_e t ) :
  action_t( at, n, id, p, t, true )
{
  init_spell_base_t_();
}

// spell_base_t::haste ===========================================================

double spell_base_t::haste() const
{
  return player -> composite_spell_haste();
}

// spell_base_t::gcd =============================================================

timespan_t spell_base_t::gcd() const
{
  timespan_t t = action_t::gcd();
  if ( t == timespan_t::zero ) return timespan_t::zero;

  t *= haste();
  if ( t < min_gcd ) t = min_gcd;

  return t;
}

// spell_base_t::execute_time ====================================================

timespan_t spell_base_t::execute_time() const
{
  timespan_t t = base_execute_time;

  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero;

  if ( t <= timespan_t::zero ) return timespan_t::zero;
  t *= haste();

  return t;
}

// spell_base_t::player_buff =====================================================

void spell_base_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_crit = p -> composite_spell_crit();

  if ( sim -> debug ) log_t::output( sim, "spell_base_t::player_buff: %s crit=%.2f",
                                     name(), player_crit );
}

// spell_base_t::crit_chance =====================================================

double spell_base_t::crit_chance( int /* delta_level */ ) const
{ return total_crit(); }

// spell_base_t::calculate_result ================================================

void spell_base_t::calculate_result()
{
  int delta_level = target -> level - player -> level;

  direct_dmg = 0;
  result = RESULT_NONE;

  if ( ! harmful || ! may_hit ) return;

  if ( ( result == RESULT_NONE ) && may_miss )
  {
    if ( rng_result -> roll( miss_chance( delta_level ) ) )
    {
      result = RESULT_MISS;
    }
  }

  if ( ( result == RESULT_NONE ) && may_resist && binary )
  {
    if ( rng_result -> roll( resistance() ) )
    {
      result = RESULT_RESIST;
    }
  }

  if ( result == RESULT_NONE )
  {
    result = RESULT_HIT;

    if ( may_crit )
    {
      if ( rng_result -> roll( crit_chance( delta_level ) ) )
      {
        result = RESULT_CRIT;
      }
    }
  }

  if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

// spell_base_t::execute =========================================================

void spell_base_t::execute()
{
  action_t::execute();

  if ( player -> last_foreground_action == this )
    player -> debuffs.casting -> expire();

  if ( harmful && callbacks )
  {
    if ( result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> spell_callbacks[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> spell_callbacks[ RESULT_NONE ], this );
    }
  }
}

// spell_base_t::schedule_execute ================================================

void spell_base_t::schedule_execute()
{
  action_t::schedule_execute();

  if ( time_to_execute > timespan_t::zero )
    player -> debuffs.casting -> trigger();
}

void spell_base_t::init()
{
  action_t::init();

  if ( base_spell_power_multiplier > 0 &&
       ( direct_power_mod > 0 || tick_power_mod > 0 ) )
    snapshot_flags |= STATE_SP | STATE_TARGET_POWER;

  if ( num_ticks > 0 && hasted_ticks )
    snapshot_flags |= STATE_HASTE;
}

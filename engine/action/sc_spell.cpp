// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "spell.hpp"
#include "heal.hpp"
#include "action/sc_action_state.hpp"
#include "buff/sc_buff.hpp"
#include "dbc/dbc.hpp"
#include "player/sc_player.hpp"
#include "player/stats.hpp"
#include "sim/sc_cooldown.hpp"
#include "util/rng.hpp"
#include <algorithm>

// ==========================================================================
// Spell Base
// ==========================================================================

spell_base_t::spell_base_t(action_e at,
  util::string_view token,
  player_t* p) :
  spell_base_t(at, token, p, spell_data_t::nil())
{

}

spell_base_t::spell_base_t( action_e at,
                            util::string_view token,
                            player_t* p,
                            const spell_data_t* s ) :
  action_t( at, token, p, s ),
  procs_courageous_primal_diamond( true )
{
  min_gcd = p -> min_gcd;
  gcd_type = gcd_haste_type::SPELL_SPEED; // Hasten spell GCDs by default
  special = true;

  crit_bonus = 1.0;
  crit_multiplier *= util::crit_multiplier( player -> meta_gem );
}

timespan_t spell_base_t::execute_time() const
{
  timespan_t t = base_execute_time;

  if ( t <= timespan_t::zero() ) {
    return timespan_t::zero();
  }

  t *= composite_haste();

  return t;
}

result_e spell_base_t::calculate_result( action_state_t* s ) const
{
  result_e result = RESULT_NONE;

  if ( ! s -> target )
    return RESULT_NONE;

  if ( ! may_hit )
    return RESULT_NONE;

  if ( ( result == RESULT_NONE ) && may_miss )
  {
    if ( rng().roll( miss_chance( composite_hit(), s -> target ) ) )
    {
      result = RESULT_MISS;
    }
  }

  if ( result == RESULT_NONE )
  {
    result = RESULT_HIT;

    if ( may_crit )
    {
      if ( rng().roll( std::max( s -> composite_crit_chance(), 0.0 ) ) )
        result = RESULT_CRIT;
    }
  }

  sim->print_debug("{} result for {} is {}.", *player, *this, result );

  return result;
}

void spell_base_t::execute()
{
  action_t::execute();

  if ( player -> last_foreground_action == this )
    player -> debuffs.casting -> expire();
}

void spell_base_t::schedule_execute( action_state_t* execute_state )
{
  action_t::schedule_execute( execute_state );

  if ( ! background && time_to_execute > timespan_t::zero() )
    player -> debuffs.casting -> trigger();
}

double spell_base_t::composite_crit_chance() const
{
  return action_t::composite_crit_chance() + player->cache.spell_crit_chance();
}

double spell_base_t::composite_haste() const
{
  return action_t::composite_haste() * player->cache.spell_speed();
}

double spell_base_t::composite_crit_chance_multiplier() const
{
  return action_t::composite_crit_chance_multiplier() * player->composite_spell_crit_chance_multiplier();
}

double spell_base_t::recharge_multiplier( const cooldown_t& cd ) const
{
  double m = action_t::recharge_multiplier( cd );

  if ( cd.hasted )
  {
    m *= player->cache.spell_haste();
  }

  return m;
}

proc_types spell_base_t::proc_type() const
{
  bool is_heal = ( type == ACTION_HEAL || type == ACTION_ABSORB );

  if ( s_data->ok() )
  {
    switch ( s_data->dmg_class() )
    {
      case SPELL_TYPE_NONE:   return is_heal ? PROC1_NONE_HEAL : PROC1_NONE_SPELL;
      case SPELL_TYPE_MAGIC:  return is_heal ? PROC1_MAGIC_HEAL : PROC1_MAGIC_SPELL;
      case SPELL_TYPE_MELEE:  return PROC1_MELEE_ABILITY;
      case SPELL_TYPE_RANGED: return PROC1_RANGED_ABILITY;
    }
  }

  if ( type == ACTION_SPELL )
    return PROC1_MAGIC_SPELL;
  // Only allow non-harmful abilities with "an amount" to count as heals
  else if ( is_heal && has_amount_result() )
    return PROC1_MAGIC_HEAL;

  return PROC1_NONE_SPELL;
}

// ==========================================================================
// Harmful Spell
// ==========================================================================

spell_t::spell_t(util::string_view token,
  player_t* p) :
  spell_t(token, p, spell_data_t::nil())
{
}

spell_t::spell_t( util::string_view   token,
                  player_t*           p,
                  const spell_data_t* s ) :
  spell_base_t( ACTION_SPELL, token, p, s )
{
  may_miss = true;
  may_block = false;
}

double spell_t::miss_chance( double hit, player_t* t ) const
{  
  // base spell miss is double base melee miss
  double miss = t -> cache.miss();
  miss *= 2;

  // 11% level-dependent miss for level+4
  miss += 0.03 * ( t -> level() - player -> level() );
    
  miss += 0.08 * std::max( t -> level() - player -> level() - 3, 0 );

  // subtract the player's hit and expertise
  miss -= hit;

  return miss;
}

result_amount_type spell_t::amount_type( const action_state_t* /* state */, bool periodic ) const
{
  if ( periodic )
    return result_amount_type::DMG_OVER_TIME;
  else
    return result_amount_type::DMG_DIRECT;
}

result_amount_type spell_t::report_amount_type( const action_state_t* state ) const
{
  result_amount_type result_type = state -> result_type;

  if ( result_type == result_amount_type::DMG_DIRECT )
  {
    // Direct ticks are direct damage, that are recorded as ticks
    if ( direct_tick )
      result_type = result_amount_type::DMG_OVER_TIME;
    // With direct damage, we need to check if this action is a tick action of
    // someone. If so, then the damage should be recorded as periodic.
    else
    {
      if ( !stats -> action_list.empty() && stats -> action_list.front() -> tick_action == this )
      {
        result_type = result_amount_type::DMG_OVER_TIME;
      }
    }
  }

  return result_type;
}

void spell_t::init()
{
  spell_base_t::init();

  if ( harmful )
    procs_courageous_primal_diamond = false;
}

double spell_t::composite_hit() const
{
  return action_t::composite_hit() + player->cache.spell_hit();
}

double spell_t::composite_versatility(const action_state_t* state) const
{
  return spell_base_t::composite_versatility(state) + player->cache.damage_versatility();
}

double spell_t::composite_target_multiplier(player_t* target) const
{
  double mul = action_t::composite_target_multiplier(target);

  mul *= composite_target_damage_vulnerability(target);

  return mul;
}

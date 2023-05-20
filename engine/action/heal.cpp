// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "heal.hpp"
#include "dot.hpp"
#include "action_state.hpp"
#include "action_callback.hpp"
#include "dbc/spell_data.hpp"
#include "player/player.hpp"
#include "player/stats.hpp"
#include "sim/expressions.hpp"
#include "sim/sim.hpp"
#include "util/rng.hpp"

heal_t::heal_t( util::string_view name, player_t* p ) : heal_t( name, p, spell_data_t::nil() ) {}

heal_t::heal_t( util::string_view name, player_t* p, const spell_data_t* s )
  : spell_base_t( ACTION_HEAL, name, p, s ),
    group_only(),
    record_healing(),
    base_pct_heal(),
    tick_pct_heal(),
    heal_gain( p->get_gain( name ) )
{
  if ( sim->heal_target && target == sim->target )
    target = sim->heal_target;
  else if ( !target || target->is_enemy() )
    target = p;

  weapon_multiplier = 0.0;

  stats->type = STATS_HEAL;

  for ( size_t i = 1; i <= data().effect_count(); i++ )
  {
    parse_heal_effect_data( data().effectN( i ) );
  }
}

void heal_t::activate()
{
  sim->player_non_sleeping_list.register_callback( [ this ]( player_t* ) {
    target_cache.is_valid = false;
  } );
}

void heal_t::parse_heal_effect_data( const spelleffect_data_t& e )
{
  if ( e.ok() )
  {
    bool item_scaling = item && data().max_scaling_level() == 0;

    if ( e.type() == E_HEAL_PCT )
    {
      base_pct_heal = e.percent();
    }
    else if ( e.type() == E_HEAL )
    {
      parse_effect_direct_mods( e, item_scaling );
    }
    else if ( e.subtype() == A_PERIODIC_HEAL_PCT )
    {
      tick_pct_heal = e.percent();
      parse_effect_period( e );
    }
    else if ( e.subtype() == A_PERIODIC_HEAL )
    {
      parse_effect_periodic_mods( e, item_scaling );
      parse_effect_period( e );
    }
  }
}

void heal_t::init()
{
  base_t::init();

  record_healing = player->record_healing();
}

double heal_t::composite_da_multiplier( const action_state_t* s ) const
{
  double m = action_multiplier() * action_da_multiplier() * player->cache.player_heal_multiplier( s ) *
             player->composite_player_dh_multiplier( get_school() );

  return m;
}

double heal_t::composite_ta_multiplier( const action_state_t* s ) const
{
  double m = action_multiplier() * action_ta_multiplier() * player->cache.player_heal_multiplier( s ) *
             player->composite_player_th_multiplier( get_school() );

  return m;
}

double heal_t::composite_player_critical_multiplier( const action_state_t* ) const
{
  return player->composite_player_critical_healing_multiplier();
}

double heal_t::composite_versatility( const action_state_t* state ) const
{
  return spell_base_t::composite_versatility( state ) + player->cache.heal_versatility();
}

result_amount_type heal_t::amount_type( const action_state_t* /* state */, bool periodic ) const
{
  if ( periodic )
    return result_amount_type::HEAL_OVER_TIME;
  else
    return result_amount_type::HEAL_DIRECT;
}

result_amount_type heal_t::report_amount_type( const action_state_t* state ) const
{
  result_amount_type result_type = state->result_type;

  // With direct healing, we need to check if this action is a tick action of
  // someone. If so, then the healing should be recorded as periodic.
  if ( result_type == result_amount_type::HEAL_DIRECT )
  {
    // Direct ticks are direct damage, that are recorded as ticks
    if ( direct_tick )
      result_type = result_amount_type::HEAL_OVER_TIME;
    else
    {
      if ( stats->action_list.front()->tick_action == this )
      {
        result_type = result_amount_type::HEAL_OVER_TIME;
      }
    }
  }

  return result_type;
}

double heal_t::composite_pct_heal( const action_state_t* ) const
{
  return base_pct_heal;
}

double heal_t::calculate_direct_amount( action_state_t* state ) const
{
  double pct_heal = composite_pct_heal( state );
  if ( pct_heal )
  {
    double amount = state->target->resources.max[ RESOURCE_HEALTH ] * pct_heal;

    // Record initial amount to state
    state->result_raw = amount;

    if ( state->result == RESULT_CRIT )
    {
      amount *= 1.0 + total_crit_bonus( state );
    }

    amount *= state->composite_da_multiplier();

    // Record total amount to state
    state->result_total = amount;
    return amount;
  }

  return base_t::calculate_direct_amount( state );
}

double heal_t::calculate_tick_amount( action_state_t* state, double dmg_multiplier ) const
{
  if ( tick_pct_heal )
  {
    double amount = state->target->resources.max[ RESOURCE_HEALTH ] * tick_pct_heal;

    // Record initial amount to state
    state->result_raw = amount;

    if ( state->result == RESULT_CRIT )
    {
      amount *= 1.0 + total_crit_bonus( state );
    }

    amount *= state->composite_ta_multiplier();
    amount *= dmg_multiplier;  // dot tick multiplier

    // Record total amount to state
    state->result_total = amount;

    if ( sim->debug )
    {
      sim->print_debug(
          "{} amount for {} on {}: "
          "ta={} pct={} b_ta={} bonus_ta={} s_mod={} s_power={} a_mod={} a_power={} mult={}",
          *player, *this, *target, amount, tick_pct_heal, base_ta( state ), bonus_ta( state ),
          spell_tick_power_coefficient( state ), state->composite_spell_power(), attack_tick_power_coefficient( state ),
          state->composite_attack_power(), state->composite_ta_multiplier() );
    }

    return amount;
  }

  return base_t::calculate_tick_amount( state, dmg_multiplier );
}

void heal_t::assess_damage( result_amount_type heal_type, action_state_t* s )
{
  s->target->assess_heal( get_school(), heal_type, s );

  if ( heal_type == result_amount_type::HEAL_DIRECT )
  {
    sim->print_log( "{} {} heals {} for {} ({}) ({})", *player, *this, *s->target, s->result_total, s->result_amount,
                    s->result );
  }
  else  // result_amount_type::HEAL_OVER_TIME
  {
    if ( sim->log )
    {
      dot_t* dot = get_dot( s->target );
      assert( dot );
      sim->print_log( "{} {} ticks ({} of {}) {} for {} ({}) heal ({})", *player, *this, dot->current_tick,
                      dot->num_ticks(), *s->target, s->result_total, s->result_amount, s->result );
    }
  }

  // New callback system; proc spells on impact.
  // Note: direct_tick_callbacks should not be used with the new system,
  // override action_t::proc_type() instead
  if ( callbacks )
  {
    proc_types pt = s->proc_type();
    proc_types2 pt2 = s->impact_proc_type2();
    if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
      player->trigger_callbacks( pt, pt2, this, s );
  }

  if ( record_healing )
  {
    stats->add_result( ( sim->count_overheal_as_heal ? s->result_total : s->result_amount ), s->result_total,
                       ( direct_tick ? result_amount_type::HEAL_OVER_TIME : heal_type ), s->result, s->block_result,
                       s->target );

    // Record external healing too
    if ( player != s->target )
      s->target->gains.health->add( RESOURCE_HEALTH, s->result_amount, s->result_total - s->result_amount );
    else
      heal_gain->add( RESOURCE_HEALTH, s->result_amount, s->result_total - s->result_amount );
  }
}

player_t* heal_t::find_greatest_difference_player()
{
  double max = 0;
  player_t* max_player = player;
  for ( const auto& p : sim->player_list )
  {
    // No love for pets right now
    double diff = p->is_pet() ? 0 : p->resources.max[ RESOURCE_HEALTH ] - p->resources.current[ RESOURCE_HEALTH ];
    if ( diff > max )
    {
      max = diff;
      max_player = p;
    }
  }
  return max_player;
}

player_t* heal_t::find_lowest_player()
{
  double min = 1.0;
  player_t* max_player = player;

  for ( const auto& p : sim->player_no_pet_list )  // check players only
  {
    double hp_pct = p->resources.pct( RESOURCE_HEALTH );
    if ( hp_pct < min )
    {
      min = hp_pct;
      max_player = p;
    }
  }

  if ( min == 1.0 )  // no player found - check pets
  {
    for ( const auto& p : sim->player_list )
    {
      if ( !p->is_pet() )
        continue;
      double hp_pct = p->resources.pct( RESOURCE_HEALTH );
      if ( hp_pct < min )
      {
        min = hp_pct;
        max_player = p;
      }
    }
  }

  return max_player;
}

std::vector<player_t*> heal_t::find_lowest_players( int num_players ) const
{
  std::vector<player_t*> lowest_N_players = sim->player_no_pet_list.data();

  while ( lowest_N_players.size() > as<size_t>( num_players ) )
  {
    // find the remaining player with the highest health
    double max = std::numeric_limits<double>::lowest();
    size_t max_player_index = 0;
    for ( size_t i = 0; i < lowest_N_players.size(); ++i )
    {
      player_t* p = lowest_N_players[ i ];
      double hp_pct = p->resources.pct( RESOURCE_HEALTH );
      if ( hp_pct > max )
      {
        max = hp_pct;
        max_player_index = i;
      }
    }
    // remove that player from lowest_N_players
    lowest_N_players.erase( lowest_N_players.begin() + max_player_index );
  }

  return lowest_N_players;
}

player_t* heal_t::smart_target() const
{
  std::vector<player_t*> injured_targets;
  player_t* target;
  // First check non-pet target
  for ( const auto& p : sim->healing_no_pet_list )
  {
    if ( p->health_percentage() < 100 )
    {
      injured_targets.push_back( p );
    }
  }
  // Check pets if we didn't find any injured non-pets
  if ( injured_targets.empty() )
  {
    for ( const auto& p : sim->healing_pet_list )
    {
      if ( p->health_percentage() < 100 )
      {
        injured_targets.push_back( p );
      }
    }
  }
  // Just choose a full-health target if nobody is injured
  if ( injured_targets.empty() )
  {
    injured_targets = sim->healing_no_pet_list.data();
  }
  // Choose a random member of injured_targets
  target = injured_targets[ static_cast<size_t>( rng().real() * injured_targets.size() ) ];
  return target;
}

int heal_t::num_targets() const
{
  return as<int>( range::count_if( sim->actor_list, [ this ]( player_t* t ) {
    if ( t->is_sleeping() )
      return false;
    if ( t->is_enemy() )
      return false;
    if ( t == target )
      return false;
    if ( group_only && ( t->party != target->party ) )
      return false;
    return true;
  } ) );
}

size_t heal_t::available_targets( std::vector<player_t*>& target_list ) const
{
  target_list.clear();
  target_list.push_back( target );

  for ( const auto& t : sim->player_non_sleeping_list )
  {
    if ( t != target )
      if ( !group_only || ( t->party == target->party ) )
        target_list.push_back( t );
  }

  return target_list.size();
}

player_t* heal_t::get_expression_target()
{
  return target;
}

std::unique_ptr<expr_t> heal_t::create_expression( util::string_view name )
{
  if ( name_str == "active_allies" )
  {
    return make_fn_expr( "active_allies", [ this ] {
      return num_targets();
    } );
  }

  return spell_base_t::create_expression( name );
}

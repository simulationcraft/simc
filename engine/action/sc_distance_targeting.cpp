// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

/*
 This file contains all functions that treat targets as if they were on an x,y
 plane with coordinates other than 0,0.
 The simulation flag distance_targeting_enabled must be turned on for these to
 do anything.
*/

bool action_t::execute_targeting( action_t* action ) const
{
  if ( action->sim->distance_targeting_enabled )
  {
    if ( action->sim->log )
    {
      action->sim->out_debug.printf(
          "%s action %s - Range %.3f, Radius %.3f, player location "
          "x=%.3f,y=%.3f, target: %s - location: x=%.3f,y=%.3f",
          action->player->name(), action->name(), action->range, action->radius,
          action->player->x_position, action->player->y_position,
          action->target->name(), action->target->x_position,
          action->target->y_position );
    }
    if ( action->time_to_execute > timespan_t::zero() && action->range > 0.0 )
    {  // No need to recheck if the execute time was zero.
      if ( action->player->get_player_distance( *action->target ) >
           action->range + action->target->combat_reach )
      {  // Target is now out of range, we cannot finish the cast.
        return false;
      }
    }
  }
  return true;
}

// This returns a list of all targets currently in range. 
std::vector<player_t*> action_t::targets_in_range_list(
    std::vector<player_t*>& tl ) const
{
  size_t i = tl.size();
  while ( i > 0 )
  {
    i--;
    player_t* target_ = tl[ i ];
    if ( range > 0.0 && player->get_player_distance( *target_ ) > range )
    {
      tl.erase( tl.begin() + i );
    }
    else if ( !ground_aoe && target_->debuffs.invulnerable && target_->debuffs.invulnerable->check() )
    {
      // Cannot target invulnerable mobs, unless it's a ground aoe. It just
      // won't do damage.
      tl.erase( tl.begin() + i );
    }
  }
  return tl;
}

std::vector<player_t*>& action_t::check_distance_targeting(
    std::vector<player_t*>& tl ) const
{
  if ( sim -> distance_targeting_enabled )
  {
    size_t i = tl.size();
    while ( i > 0 )
    {
      i--;
      player_t* t = tl[i];
      if ( t != target )
      {
        if ( sim->log )
        {
          sim->out_debug.printf(
            "%s action %s - Range %.3f, Radius %.3f, player location "
            "x=%.3f,y=%.3f, original target: %s - location: x=%.3f,y=%.3f, "
            "impact target: %s - location: x=%.3f,y=%.3f",
            player->name(), name(), range, radius, player->x_position,
            player->y_position, target->name(), target->x_position,
            target->y_position, t->name(), t->x_position, t->y_position );
        }
        if ( ( ground_aoe && t->debuffs.flying && t->debuffs.flying->check() ) )
        {
          tl.erase( tl.begin() + i );
        }
        else if ( radius > 0 && range > 0 )
        {  // Abilities with range/radius radiate from the target.
          if ( ground_aoe && parent_dot && parent_dot->is_ticking() )
          {  // We need to check the parents dot for location.
            if ( sim->log )
              sim->out_debug.printf( "parent_dot location: x=%.3f,y%.3f",
                                     parent_dot->state->original_x,
                                     parent_dot->state->original_y );
            if ( t->get_ground_aoe_distance( *parent_dot->state ) >
                 radius + t->combat_reach )
            {
              tl.erase( tl.begin() + i );
            }
          }
          else if ( ground_aoe && execute_state )
          {
            if ( t->get_ground_aoe_distance( *execute_state ) >
                 radius + t->combat_reach )
            {
              tl.erase( tl.begin() + i );  // We should just check the child.
            }
          }
          else if ( t->get_player_distance( *target ) > radius )
          {
            tl.erase( tl.begin() + i );
          }
        }  // If they do not have a range, they are likely based on the distance
           // from the player.
        else if ( radius > 0 &&
                  t->get_player_distance( *player ) > radius + t->combat_reach )
        {
          tl.erase( tl.begin() + i );
        }
        else if ( range > 0 &&
                  t->get_player_distance( *player ) > range + t->combat_reach )
        {
          // If they only have a range, then they are a single target ability, or
          // are also based on the distance from the player.
          tl.erase( tl.begin() + i );
        }
      }
    }
    if ( sim->log )
    {
      sim->out_debug.printf( "%s regenerated target cache for %s (%s)",
                             player->name(), signature_str.c_str(), name() );
      for ( size_t j = 0; j < tl.size(); j++ )
      {
        sim->out_debug.printf( "[%u, %s (id=%u) x= %.3f y= %.3f ]",
                               static_cast<unsigned>( j ), tl[j]->name(),
                               tl[j]->actor_index, tl[j]->x_position,
                               tl[j]->y_position );
      }
    }
  }
  return tl;
}

player_t* action_t::select_target_if_target()
{
  if ( target_if_mode == TARGET_IF_NONE )
  {
    return nullptr;
  }

  if ( target_list().size() == 1 )
  {
    // If first is used, don't return a valid target unless the target_if
    // evaluates to non-zero
    if ( target_if_mode == TARGET_IF_FIRST )
    {
      return target_if_expr->evaluate() > 0 ? target : nullptr;
    }
    // For the rest (min/max), return the target
    return target;
  }

  std::vector<player_t*> master_list;
  if ( sim->distance_targeting_enabled )
  {
    if ( !target_cache.is_valid )
    {
      available_targets( target_cache.list );
      master_list           = targets_in_range_list( target_cache.list );
      target_cache.is_valid = true;
    }
    else
    {
      master_list = target_cache.list;
    }
    if ( sim->log )
      sim->out_debug.printf( "%s Number of targets found in range - %.3f",
                             player->name(),
                             static_cast<double>( master_list.size() ) );
    if ( master_list.size() <= 1 )
      return target;
  }
  else
  {
    master_list = target_list();
  }

  player_t* original_target = target;
  player_t* proposed_target = target;
  double current_target_v   = target_if_expr->evaluate();

  double max_ = current_target_v;
  double min_ = current_target_v;

  for ( auto player : master_list )
  {
    target = player;

    // No need to check current target
    if ( target == original_target )
      continue;

    if ( !target_ready( target ) )
    {
      continue;
    }

    double v = target_if_expr->evaluate();

    // Don't swap to targets that evaluate to identical value than the current
    // target
    if ( v == current_target_v )
      continue;

    if ( target_if_mode == TARGET_IF_FIRST && v != 0 )
    {
      current_target_v = v;
      proposed_target  = target;
      break;
    }
    else if ( target_if_mode == TARGET_IF_MAX && v > max_ )
    {
      max_            = v;
      proposed_target = target;
    }
    else if ( target_if_mode == TARGET_IF_MIN && v < min_ )
    {
      min_            = v;
      proposed_target = target;
    }
  }

  // Restore original target
  target = original_target;

  // If "first available target" did not find anything useful, don't execute the
  // action
  if ( target_if_mode == TARGET_IF_FIRST && current_target_v == 0 )
  {
    if ( sim->debug )
    {
      sim->out_debug.printf( "%s target_if no target found for %s", player->name(),
          signature_str.c_str() );
    }
    return nullptr;
  }

  if ( sim->debug )
  {
    sim->out_debug.printf(
        "%s target_if best target: %s - original target - %s - current target "
        "-%s",
        player->name(), proposed_target->name(), original_target->name(),
        target->name() );
  }

  return proposed_target;
}

// action_t::distance_targeting_travel_time
// ======================================

timespan_t action_t::distance_targeting_travel_time(
    action_state_t* /*s*/ ) const
{
  return timespan_t::zero();
}

// Player functions
// =============================================================

// player_t::get_position_distance
// ==============================================

double player_t::get_position_distance( double m, double v ) const
{
  double delta_x = this->x_position - m;
  double delta_y = this->y_position - v;
  double sqrtnum = delta_x * delta_x + delta_y * delta_y;
  return util::approx_sqrt( sqrtnum );
}

// player_t::get_player_distance ===============================================

double player_t::get_player_distance( const player_t& target ) const
{
  if ( sim -> distance_targeting_enabled )
  {
    return get_position_distance( target.x_position, target.y_position );
  }
  else
    return current.distance;
}

// player_t::get_ground_aoe_distance ===========================================

double player_t::get_ground_aoe_distance( const action_state_t& a ) const
{
  return get_position_distance( a.original_x, a.original_y );
}

// player_t::init_distance_targeting ===========================================

void player_t::init_distance_targeting()
{
  if ( !sim->distance_targeting_enabled )
    return;

  x_position = -1 * base.distance;
}

// Generic helper functions ==================================================

// Approximation of square root ==============================================
// Used in calculation of distances instead of std::sqrt as it is significantly
// faster and also returns similar values

double util::approx_sqrt( double number )
{
  if ( number > 0.0 )
  {
    float xhalf = 0.5f * static_cast<float>( number );
    union
    {
      float x;
      int i;
    } u;
    u.x = static_cast<float>( number );
    u.i = 0x5f3759df - ( u.i >> 1 );
    u.x = u.x * ( 1.5f - xhalf * u.x * u.x );
    return static_cast<double>( u.x * number );
  }
  return 0.0;
}

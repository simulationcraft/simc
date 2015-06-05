// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

  bool action_t::impact_targeting( action_state_t* ) const
  {
    return true;
  }

  bool action_t::execute_targeting( action_t* action ) const
  {
    if ( action -> sim -> distance_targeting_enabled )
    {
      if ( action -> sim -> log )
      {
        action -> sim -> out_debug.printf( "%s action %s - Range %.3f, Radius %.3f, player location x=%.3f,y=%.3f, target: %s - location: x=%.3f,y=%.3f",
          action -> player -> name(), action -> name(), action -> range, action -> radius,
          action -> player -> x_position, action -> player -> y_position, action -> target -> name(), action -> target -> x_position, action -> target -> y_position );
      }
      if ( action -> time_to_execute > timespan_t::zero() )
      { // No need to recheck if the execute time was zero.
        if ( action -> range > 0.0 )
        {
          if ( action -> target -> get_player_distance( *action -> player ) > action -> range )
          { // Target is now out of range, we cannot finish the cast.
            return false;
          }
        }
      }
    }
    return true;
  }

  std::vector<player_t*> action_t::targets_in_range_list( std::vector< player_t* >& tl ) const
  {
    size_t i = tl.size();
    while ( i > 0 )
    {
      i--;
      player_t* target_ = tl[i];
      if ( range > 0.0 && target_ -> get_player_distance( *player ) > range )
        tl.erase( tl.begin() + i );
      else if ( !ground_aoe && target_ -> debuffs.invulnerable -> check() ) // Cannot target invulnerable mobs, unless it's a ground aoe. It just won't do damage.
        tl.erase( tl.begin() + i );
    }
    return tl;
  }

  std::vector<player_t*> action_t::check_distance_targeting( std::vector< player_t* >& tl ) const
  {
    size_t i = tl.size();
    while ( i > 0 )
    {
      i--;
      player_t* t = tl[i];
      if ( t != target )
      {
        if ( sim -> log )
        {
          sim -> out_debug.printf( "%s action %s - Range %.3f, Radius %.3f, player location x=%.3f,y=%.3f, original target: %s - location: x=%.3f,y=%.3f, impact target: %s - location: x=%.3f,y=%.3f",
            player -> name(), name(), range, radius,
            player -> x_position, player -> y_position, target -> name(), target -> x_position, target -> y_position, t -> name(), t -> x_position, t -> y_position );
        }
        if ( ( ground_aoe && t -> debuffs.flying -> check() ) || t -> debuffs.invulnerable -> check() )
          tl.erase( tl.begin() + i );
        else if ( radius > 0 && range > 0 )
        { // Abilities with range/radius radiate from the target. 
          if ( ground_aoe && parent_dot && parent_dot -> is_ticking() )
          { // We need to check the parents dot for location.
            if ( sim -> log )
              sim -> out_debug.printf( "parent_dot location: x=%.3f,y%.3f", parent_dot -> state -> original_x, parent_dot -> state -> original_y );
            if ( t -> get_ground_aoe_distance( *parent_dot -> state ) > radius )
              tl.erase( tl.begin() + i );
          }
          else if ( ground_aoe && execute_state )
          {
            if ( t -> get_ground_aoe_distance( *execute_state ) > radius )
              tl.erase( tl.begin() + i ); // We should just check the child.
          }
          else if ( t -> get_player_distance( *target ) > radius )
            tl.erase( tl.begin() + i );
        } // If they do not have a range, they are likely based on the distance from the player.
        else if ( radius > 0 && t -> get_player_distance( *player ) > radius )
          tl.erase( tl.begin() + i );
        else if ( range > 0 && t -> get_player_distance( *player ) > range )
          tl.erase( tl.begin() + i ); // If they only have a range, then they are a single target ability.
      }
    }
    if ( sim -> log )
    {
      sim -> out_debug.printf( "%s regenerated target cache for %s (%s)",
        player -> name(),
        signature_str.c_str(),
        name() );
      for ( size_t i = 0; i < tl.size(); i++ )
      {
        sim -> out_debug.printf( "[%u, %s (id=%u) x= %.3f y= %.3f ]",
          static_cast<unsigned>( i ),
          tl[i] -> name(),
          tl[i] -> actor_index,
          tl[i] -> x_position,
          tl[i] -> y_position );
      }
    }
    return tl;
  }

  player_t* action_t::select_target_if_target()
  {
    if ( target_if_mode == TARGET_IF_NONE || target_list().size() == 1 )
      return target;

    std::vector<player_t*> master_list;
    if ( sim -> distance_targeting_enabled )
    {
      if ( !target_cache.is_valid )
      {
        available_targets( target_cache.list );
        master_list = targets_in_range_list( target_cache.list );
        target_cache.is_valid = true;
      }
      else
      {
        master_list = target_cache.list;
      }
      if ( sim -> log )
        sim -> out_debug.printf( "%s Number of targets found in range - %.3f",
        player -> name(), static_cast<double>( master_list.size() ) );
      if ( master_list.size() <= 1 )
        return target;
    }
    else
    {
      master_list = target_list();
    }

    player_t* original_target = target;
    player_t* proposed_target = target;
    double current_target_v = target_if_expr -> evaluate();

    double max_ = current_target_v;
    double min_ = current_target_v;

    for ( size_t i = 0, end = master_list.size(); i < end; ++i )
    {
      target = master_list[i];

      // No need to check current target
      if ( target == original_target )
        continue;
      double v = target_if_expr -> evaluate();

      // Don't swap to targets that evaluate to identical value than the current target
      if ( v == current_target_v )
        continue;

      if ( target_if_mode == TARGET_IF_FIRST && v != 0 )
      {
        proposed_target = target;
        break;
      }
      else if ( target_if_mode == TARGET_IF_MAX && v > max_ )
      {
        max_ = v;
        proposed_target = target;
      }
      else if ( target_if_mode == TARGET_IF_MIN && v < min_ )
      {
        min_ = v;
        proposed_target = target;
      }
    }

    if ( sim -> log )
      sim -> out_debug.printf( "%s target_if best target: %s - original target - %s - current target -%s", 
        player -> name(), proposed_target -> name(), original_target -> name(), target -> name() );

    target = original_target;

    return proposed_target;
  }
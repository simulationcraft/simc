// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

  bool action_t::impact_targeting( action_state_t* )
  {
    return true;
  }

  bool action_t::execute_targeting( const action_t* action )
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
          if ( action -> target -> get_position_distance( action -> player -> x_position, action -> player -> y_position ) > action -> range )
          { // Target is now out of range, we cannot finish the cast.
            return false;
          }
        }
      }
    }
    return true;
  }

  std::vector<player_t*> action_t::available_targeting( std::vector< player_t* >& tl ) const
  {
    if ( !target_cache.is_valid )
    {
      for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* t = sim -> target_non_sleeping_list[i];

        if ( t -> is_enemy() && ( t != target ) )
        {
          if ( sim -> log )
          {
            sim -> out_debug.printf( "%s action %s - Range %.3f, Radius %.3f, player location x=%.3f,y=%.3f, original target: %s - location: x=%.3f,y=%.3f, impact target: %s - location: x=%.3f,y=%.3f",
              player -> name(), name(), range, radius,
              player -> x_position, player -> y_position, target -> name(), target -> x_position, target -> y_position, t -> name(), t -> x_position, t -> y_position );
          }
          if ( radius > 0 )
          {
            if ( range > 0 ) // Abilities with range/radius radiate from the target. 
            {
              if ( ground_aoe && parent_dot && parent_dot -> is_ticking() )
              { // We need to check the parents dot for location.
                if ( sim -> log )
                  sim -> out_debug.printf( "parent_dot location: x=%.3f,y%.3f", parent_dot -> state -> original_x, parent_dot -> state -> original_y );
                if ( t -> get_position_distance( parent_dot -> state -> original_x, parent_dot -> state -> original_y ) <= radius )
                  tl.push_back( t );
              }
              else if ( ground_aoe && execute_state && t -> get_position_distance( execute_state -> original_x, execute_state -> original_y ) <= radius ) // We should just check the child.
                tl.push_back( t );
              else if ( t -> get_position_distance( target -> x_position, target -> y_position ) <= radius )
                tl.push_back( t );
            } // If they do not have a range, they are likely based on the distance from the player.
            else if ( t -> get_position_distance( player -> x_position, player -> y_position ) <= radius )
              tl.push_back( t );
          }
          else if ( range > 0 ) // If they only have a range, then they are a single target ability.
          {
            if ( t -> get_position_distance( player -> x_position, player -> y_position ) <= range )
              tl.push_back( t );
          }
        }
      }
    }
    return tl;
  }

  player_t* action_t::select_target_if_target()
  {
    if ( target_if_mode == TARGET_IF_NONE || sim -> target_non_sleeping_list.size() == 1 )
    {
      return 0;
    }

    player_t* original_target = target;
    player_t* proposed_target = target;

    double current_target_v = target_if_expr -> evaluate();

    double max_ = current_target_v;
    double min_ = current_target_v;
    std::vector<player_t*> master_list = sim -> target_non_sleeping_list.data();

    for ( size_t i = 0, end = master_list.size(); i < end; ++i )
    {
      target = master_list[i];

      // No need to check current target
      if ( target == original_target )
      {
        continue;
      }
      target_cache.is_valid = false;
      double v = target_if_expr -> evaluate();

      // Don't swap to targets that evaluate to identical value than the current target
      if ( v == current_target_v )
      {
        continue;
      }

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

    target = original_target;

    return proposed_target;
  }
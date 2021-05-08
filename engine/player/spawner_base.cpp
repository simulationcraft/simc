// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "spawner_base.hpp"

#include "player/sc_player.hpp"
#include "util/util.hpp"
#include "sim/sc_sim.hpp"

namespace spawner
{
void merge( sim_t& parent_sim, sim_t& other_sim )
{
  std::vector<std::pair<base_actor_spawner_t*, base_actor_spawner_t*>> deferred;

  // Collect a list of dynamic spawners and their corresponding other_sim counterparts
  for ( auto& player : parent_sim.actor_list )
  {
    // If the player is spawned by normal means, skip
    if ( player->spawner != nullptr )
    {
      continue;
    }

    player_t* other_p = other_sim.find_player( player->index );
    if ( !other_p )
    {
      throw std::runtime_error( fmt::format( "Could not find player {} ({})", player->name(), player->index ) );
    }

    // Grab all spawner objs for later merging. This is done to guarantee that actor_list does not
    // change during the merging of statically created actors.
    range::for_each( player->spawners, [&deferred, other_p]( base_actor_spawner_t* o ) {
      auto other_o = other_p->find_spawner( o->name() );
      if ( other_o == nullptr )
      {
        return;
      }

      std::pair<base_actor_spawner_t*, base_actor_spawner_t*> tuple{o, other_o};
      auto it = range::find( deferred, tuple );
      if ( it != deferred.end() )
      {
        return;
      }

      deferred.push_back( tuple );
    } );
  }

  // Merge all dynamic spawners
  range::for_each( deferred, []( const std::pair<base_actor_spawner_t*, base_actor_spawner_t*>& o ) {
    o.first->merge( o.second );
  } );
}

void create_persistent_actors( player_t& player )
{
  range::for_each( player.spawners, []( base_actor_spawner_t* spawner ) { spawner->create_persistent_actors(); } );
}

void base_actor_spawner_t::register_object()
{
  auto it = range::find_if( m_owner->spawners, [this]( const base_actor_spawner_t* obj ) {
    return util::str_compare_ci( obj->name(), name() );
  } );

  if ( it == m_owner->spawners.end() )
  {
    m_owner->spawners.push_back( this );
  }
  else
  {
    m_owner->sim->errorf( "%s attempting to create duplicate pet spawner object %s", m_owner->name(), name().c_str() );
  }
}

}  // namespace spawner
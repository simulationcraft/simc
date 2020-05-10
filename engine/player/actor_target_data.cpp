// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "actor_target_data.hpp"
#include "sc_player.hpp"
#include "sc_actor_pair.hpp"
#include "sim/sc_sim.hpp"

actor_target_data_t::actor_target_data_t( player_t* target, player_t* source ) :
  actor_pair_t( target, source ), debuff(), dot()
{
  for (auto & elem : source -> sim -> target_data_initializer)
  {
    elem( this );
  }
}
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "target_specific.hpp"
#include "player/sc_player.hpp"

namespace target_specific_helper
{
  size_t get_actor_index(const player_t* player)
  {
    return player->actor_index;
  }
};
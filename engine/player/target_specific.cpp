// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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
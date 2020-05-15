// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player_event.hpp"
#include "sc_player.hpp"

player_event_t::player_event_t(player_t& p, timespan_t delta_time) :
  event_t(p, delta_time),
  _player(&p) {}

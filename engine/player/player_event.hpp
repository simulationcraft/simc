// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sim/event.hpp"
#include "sc_timespan.hpp"

struct player_t;

struct player_event_t : public event_t
{
  player_t* _player;
  player_event_t(player_t& p, timespan_t delta_time);
  player_t* p()
  {
    return player();
  }
  const player_t* p() const
  {
    return player();
  }
  player_t* player()
  {
    return _player;
  }
  const player_t* player() const
  {
    return _player;
  }
  virtual const char* name() const override
  {
    return "event_t";
  }
};
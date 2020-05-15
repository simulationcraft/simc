// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "darkmoon_deck.hpp"
#include "dbc/dbc.hpp"

darkmoon_deck_t::darkmoon_deck_t(const special_effect_t& e) :
  effect(e), player(e.player),
  shuffle_period(effect.driver() -> effectN(1).period())
{ }

timespan_t shuffle_event_t::delta_time(sim_t& sim, bool initial, darkmoon_deck_t* deck)
{
  if (initial)
  {
    return deck->shuffle_period * sim.rng().real();
  }
  else
  {
    return deck->shuffle_period;
  }
}

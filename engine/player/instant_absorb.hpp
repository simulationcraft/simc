// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "util/string_view.hpp"

#include <functional>
#include <string>

struct action_state_t;
struct gain_t;
struct player_t;
struct stats_t;
struct spell_data_t;

// Instant absorbs
struct instant_absorb_t
{
private:
  /* const spell_data_t* spell; */
  std::function<double( const action_state_t* )> absorb_handler;
  stats_t* absorb_stats;
  gain_t* absorb_gain;
  player_t* player;

public:
  std::string name;

  instant_absorb_t( player_t* p, const spell_data_t* s, util::string_view name,
                    std::function<double( const action_state_t* )> handler );

  double consume( const action_state_t* s );
};
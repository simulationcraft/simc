// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "util/generic.hpp"

#include <vector>

struct action_t;
struct action_state_t;
struct player_t;

struct action_callback_t : private noncopyable
{
  player_t* listener;
  bool active;
  bool allow_self_procs;
  bool allow_pet_procs;

  action_callback_t( player_t* l );
  virtual ~action_callback_t() = default;
  virtual void trigger( action_t*, action_state_t* ) = 0;
  virtual void reset() {}
  virtual void initialize() {}
  virtual void activate() { active = true; }
  virtual void deactivate() { active = false; }

  static void trigger( const std::vector<action_callback_t*>& v, action_t* a, action_state_t* state );

  static void reset( const std::vector<action_callback_t*>& v );
};

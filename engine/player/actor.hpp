// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "generic.hpp"
#include "stopwatch.hpp"
#include <string>

struct sim_t;
namespace spawner {
    class base_actor_spawner_t;
}

/* actor_t is a lightweight representation of an actor belonging to a simulation,
 * having a name and some event-related helper functionality.
 */
struct actor_t : private noncopyable
{
  sim_t* sim; // owner
  spawner::base_actor_spawner_t* spawner; // Creation/spawning of the actor delegated here
  std::string name_str;
  int event_counter; // safety counter. Shall never be less than zero

#ifdef ACTOR_EVENT_BOOKKEEPING
  /// Measure cpu time for actor-specific events.
  stopwatch_t event_stopwatch;
#endif // ACTOR_EVENT_BOOKKEEPING

  actor_t( sim_t* s, const std::string& name );
  virtual ~ actor_t() { }
  virtual const char* name() const
  { return name_str.c_str(); }
};

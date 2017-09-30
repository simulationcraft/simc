// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef X7_PANTHEON_HPP
#define X7_PANTHEON_HPP

#include <vector>

#include "sc_timespan.hpp"

struct player_t;
struct real_ppm_t;
struct buff_t;
struct event_t;

namespace unique_gear
{
// Proxy system for 7.3.2 (Argus raid) pantheon empowerment system. Simulates proxy actors
// triggering their respective pantheon buffs, and based on the overall state of the raid (proxy +
// real actors), triggers empowered buffs on the actors
struct pantheon_state_t
{
  // Trinket base drivers
  const std::vector<unsigned> drivers {
    256817, // Mark of Aman'thul
    256819, // Mark of Golganneth
    256825, // Mark of Khaz'goroth
    256822, // Mark of Eonar
    256827, // Mark of Norgannon
    256815, // Mark of Aggramar
  };

  const std::vector<unsigned> marks {
    256818, // Mark of Aman'thul
    256821, // Mark of Golganneth
    256826, // Mark of Khaz'goroth
    256824, // Mark of Eonar
    256828, // Mark of Norgannon
    256816, // Mark of Aggramar
  };

  // Durations of the non-empowered buffs
  std::vector<timespan_t> buff_durations;

  // Empowered pantheon effect callback for trinkets
  using pantheon_effect_cb_t = std::function<void(void)>;
  // A tuple representing a pantheon proxy caster (type), and their haste value
  using pantheon_opt_t = std::pair<size_t, double>;

  // RPPM object must be tied to a player so pick the first one who gets initialized
  player_t*                             player;
  // Parsed user input for proxy pantheon
  std::vector<pantheon_opt_t>           pantheon_opts;
  // RPPM objects associated with proxy buffs
  std::vector<std::vector<real_ppm_t*>> rppm_objs;
  // Start times of proxy buffs (timespan_t::min() if never triggered)
  std::vector<timespan_t>               start_time;
  // Real pantheon trinket buffs per type
  std::vector<std::vector<buff_t*>>     actor_buffs;
  // Pantheon buffs on actors
  std::vector<pantheon_effect_cb_t>     pantheon_effects;
  // Proxy buff attempt ticker
  event_t*                              attempt_event;

  pantheon_state_t( player_t* player );

  // (Attempt to) proc proxy pantheon trinkets. Called by the pantheon event every X seconds
  void attempt();

  // Check and potentially trigger pantheon buffs on all actors with their corresponding trinket.
  // Called by attempt above on a successful proxy buff trigger, and each real pantheon trinket upon
  // successful triggering.
  void trigger_pantheon_buff() const;

  // Register a pantheon buff from a real actor
  void register_pantheon_buff( buff_t* b );

  // Register a pantheon "empowered" effect to trigger
  void register_pantheon_effect( const pantheon_effect_cb_t& effect )
  { pantheon_effects.push_back( effect ); }

  // Start the proxy pantheon system
  void start();

  // Reset state to initial values
  void reset()
  {
    range::fill( start_time, timespan_t::min() );
    attempt_event = nullptr;
  }

  // Does the raid (including real actors and proxy ones) have pantheon empowerment capability?
  bool has_pantheon_capability() const;
private:
  // Parse input option
  void parse_options();
  // Output debug state to debug log
  void debug() const;
};

// Creates the global proxy pantheon object to the simulator object of the actor given
void initialize_pantheon( player_t* player );
} // Namespace unique_gear ends

#endif // X7_PANTHEON_HPP


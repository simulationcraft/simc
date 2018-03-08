// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef X7_PANTHEON_HPP
#define X7_PANTHEON_HPP

#include <vector>
#include <functional>
#include <unordered_map>

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
  // Number of unique buffs required to trigger pantheon empowerment
  static const int    N_UNIQUE_BUFFS = 4;
  // Wildcard slot (Aman'Thul), can replace anything in the other slots
  static const size_t O_WILDCARD_TRINKET = 0;
  // Offset where unique buffs start
  static const size_t O_UNIQUE_TRINKET = 1;

  // Simple state to keep track of per proxy buff, or real actor buff
  struct pantheon_buff_state_t
  {
    buff_t*     actor_buff;
    real_ppm_t* rppm_object;
    timespan_t  proxy_end_time;
  };

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
  // Base trinket effects, bucketed by type
  std::vector<std::vector<pantheon_buff_state_t>> pantheon_state;
  // Real pantheon trinket buffs per type
  std::vector<std::vector<buff_t*>>     actor_buffs;
  // Proxy buff attempt ticker
  event_t*                              attempt_event;
  // Map of actor base pantheon buff pointer -> pantheon effect callback
  std::unordered_map<buff_t*, pantheon_effect_cb_t> cbmap;

  pantheon_state_t( player_t* player );

  // (Attempt to) proc proxy pantheon trinkets. Called by the pantheon event every X seconds
  void attempt();

  // Actor triggers one of the base trinket buffs for the pantheon trinkets
  void trigger_pantheon_buff( buff_t* triggering_buff );

  // Register a pantheon "empowered" effect to trigger
  void register_pantheon_effect( buff_t* actor_base_buff, const pantheon_effect_cb_t& effect )
  {
    assert( buff_type( actor_base_buff ) < actor_buffs.size() );

    actor_buffs[ buff_type( actor_base_buff ) ].push_back( actor_base_buff );
    cbmap[ actor_base_buff ] = effect;
  }

  // Start the proxy pantheon system
  void start();

  // Reset state to initial values
  void reset();

  // Does the raid (including real actors and proxy ones) have pantheon empowerment capability?
  bool has_pantheon_capability() const;
private:
  // Determine if the total buff state allows empowerment to activate
  bool empowerment_state() const;
  // Trigger active empowerment buffs and clear state
  void trigger_empowerment();

  // Generate pantheon proxy ticker delay duration based on user options
  timespan_t pantheon_ticker_delay() const;
  // Parse input option
  void parse_options();
  // Output state to debug log
  void debug() const;

  // Does pantheon state exist for a given proxy base trinket buff, or a real trinket buff
  pantheon_buff_state_t* buff_state( size_t type, const real_ppm_t* );
  pantheon_buff_state_t* buff_state( size_t type, const buff_t* );

  // Translate player (base trinket) buff to pantheon system slot. Returns drivers.size() + 1 (i.e.,
  // more entries than there are pantheon trinket types) if the buff is invalid.
  size_t buff_type( const buff_t* ) const;

  // Clean up buff state
  void cleanup_state();

  // Durations of the non-empowered buffs
  std::vector<timespan_t> buff_durations;

  // Trinket base drivers
  static const std::vector<unsigned> drivers;

  // Trinket "Mark" buffs
  static const std::vector<unsigned> marks;
};

// Creates the global proxy pantheon object to the simulator object of the actor given
void initialize_pantheon( player_t* player );
} // Namespace unique_gear ends

#endif // X7_PANTHEON_HPP


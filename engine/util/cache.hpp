// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

// Cache Control ============================================================

namespace cache {

typedef int era_t;
static const era_t INVALID_ERA = -1;
static const era_t IN_THE_BEGINNING = 0;  // A time before any other possible era;
// used to mark persistent caches at load.

enum behavior_e
{
  ANY,      // * Use any version present in the cache, retrieve if not present.
  CURRENT,  // * Use only current info from the cache; validate old versions as needed.
  ONLY,     // * Use any version present in the cache, fail if not present.
};

class cache_control_t
{
private:
  era_t current_era;
  behavior_e player_cache_behavior;
  behavior_e item_cache_behavior;

public:
  cache_control_t() :
    current_era( IN_THE_BEGINNING ),
    player_cache_behavior( CURRENT ),
    item_cache_behavior( ANY )
  {}

  era_t era() const { return current_era; }
  void advance_era() { ++current_era; }

  behavior_e cache_players() const { return player_cache_behavior; }
  void cache_players( behavior_e b ) { player_cache_behavior = b; }

  behavior_e cache_items() const { return item_cache_behavior; }
  void cache_items( behavior_e b ) { item_cache_behavior = b; }

  static cache_control_t singleton;
};

// Caching system's global notion of the current time.
inline era_t era()
{ return cache_control_t::singleton.era(); }

// Time marches on.
inline void advance_era()
{ cache_control_t::singleton.advance_era(); }

// Get/Set default cache behaviors.
inline behavior_e players()
{ return cache_control_t::singleton.cache_players(); }
inline void players( behavior_e b )
{ cache_control_t::singleton.cache_players( b ); }

inline behavior_e items()
{ return cache_control_t::singleton.cache_items(); }
inline void items( behavior_e b )
{ cache_control_t::singleton.cache_items( b ); }

} // cache

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "dbc/data_enums.hh"

#include <array>
#include <string>
#include <vector>
#include <functional>

struct action_callback_t;
struct sim_t;

// Player Callbacks
struct effect_callbacks_t
{
  sim_t* sim;
  std::vector<action_callback_t*> all_callbacks;
  // New proc system

  // Callbacks (procs) stored in a vector
  typedef std::vector<action_callback_t*> proc_list_t;
  // .. an array of callbacks, for each proc_type2 enum (procced by hit/crit, etc...)
  typedef std::array<proc_list_t, PROC2_TYPE_MAX> proc_on_array_t;
  // .. an array of procced by arrays, for each proc_type enum (procced on aoe, heal, tick, etc...)
  typedef std::array<proc_on_array_t, PROC1_TYPE_MAX> proc_array_t;

  proc_array_t procs;

  effect_callbacks_t( sim_t* sim ) : sim( sim )
  { }

  bool has_callback( const std::function<bool(const action_callback_t*)> cmp ) const;

  ~effect_callbacks_t();

  void reset();

  void register_callback( unsigned proc_flags, unsigned proc_flags2, action_callback_t* cb );

  // Helper to get first instance of object T and return it, if not found, return nullptr
  template <typename T>
  T* get_first_of() const
  {
    for ( size_t i = 0; i < all_callbacks.size(); ++i )
    {
      auto ptr = dynamic_cast<T*>( all_callbacks[ i ] );
      if ( ptr )
      {
        return ptr;
      }
    }
    return nullptr;
  }
private:
  void add_proc_callback( proc_types type, unsigned flags, action_callback_t* cb );
};

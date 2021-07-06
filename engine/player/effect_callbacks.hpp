// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "dbc/data_enums.hh"
#include "action/dbc_proc_callback.hpp"

#include <array>
#include <string>
#include <vector>
#include <functional>
#include <memory>

struct action_t;
struct action_state_t;
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
  /// Register a driver-specific custom trigger callback
  void register_callback_trigger_function( unsigned driver_id, dbc_proc_callback_t::trigger_fn_type t, const dbc_proc_callback_t::trigger_fn_t& fn );
  /// Register a driver-specific custom execute callback
  void register_callback_execute_function( unsigned driver_id, const dbc_proc_callback_t::execute_fn_t& fn );
  /// Get a custom driver-specific trigger callback (if it has been registered)
  dbc_proc_callback_t::trigger_fn_t* callback_trigger_function( unsigned driver_id ) const;
  /// Get the type of the custom driver-specific trigger callback
  dbc_proc_callback_t::trigger_fn_type callback_trigger_function_type( unsigned driver_id ) const;
  dbc_proc_callback_t::execute_fn_t* callback_execute_function( unsigned driver_id ) const;

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
  using callback_trigger_entry_t = std::tuple<unsigned, dbc_proc_callback_t::trigger_fn_type, std::unique_ptr<dbc_proc_callback_t::trigger_fn_t>>;
  using callback_execute_entry_t = std::tuple<unsigned, std::unique_ptr<dbc_proc_callback_t::execute_fn_t>>;

  /// A vector of (driver-id, callback-trigger-function) tuples
  std::vector<callback_trigger_entry_t> trigger_fn;
  /// A vector of (driver-id, callback-execute-function) tuples
  std::vector<callback_execute_entry_t> execute_fn;

  void add_proc_callback( proc_types type, unsigned flags, action_callback_t* cb );
};

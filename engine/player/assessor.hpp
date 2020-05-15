// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/generic.hpp"
#include "sc_enums.hpp"
#include <functional>
#include <vector>

struct action_state_t;

// Assessors, currently a state assessor functionality is defined.
namespace assessor
{
  // Assessor commands (continue evaluating, or stop to this assessor)
  enum command { CONTINUE = 0U, STOP };

  // Default assessor priorities
  enum priority_e
  {
    TARGET_MITIGATION = 100U,    // Target assessing (mitigation etc)
    TARGET_DAMAGE     = 200U,    // Do damage to target (and related functionality)
    LOG               = 300U,    // Logging (including debug output)
    CALLBACKS         = 400U,    // Impact callbacks
    LEECH             = 1000U,   // Leech processing
  };

  // State assessor callback type
  using state_assessor_t = std::function<command(result_amount_type, action_state_t*)>;

  // A simple entry that defines a state assessor
  struct state_assessor_entry_t
  {
    int priority;
    state_assessor_t assessor;
  };

  // State assessor functionality creates an ascending priority-based list of manipulators for state
  // (action_state_t) objects. Each assessor is run on the state object, until assessor::STOP is
  // encountered, or all the assessors have ran.
  //
  // There are a number of default assessors associated for outgoing damage state (the only place
  // where this code is used for now), defined in player_t::init_assessors. Init_assessors is called
  // late in the actor initialization order, and can take advantage of conditional registration
  // based on talents, items, special effects, specialization and other actor-related state.
  //
  // An assessor function takes two parameters, result_amount_type indicating the "damage type", and
  // action_state_t pointer to the state being assessed. The state object can be manipulated by the
  // function, but it may not be freed. The function must return one of priority enum values,
  // typically priority::CONTINUE to continue the pipeline.
  //
  // Assessors are sorted to ascending priority order in player_t::init_finished.
  //
  struct state_assessor_pipeline_t
  {
    std::vector<state_assessor_entry_t> assessors;

    void add( int p, state_assessor_t cb )
    { assessors.push_back( state_assessor_entry_t{p, std::move(cb)} ); }

    void sort()
    {
      range::sort( assessors, []( const state_assessor_entry_t& a, const state_assessor_entry_t& b )
      { return a.priority < b.priority; } );
    }

    void execute( result_amount_type type, action_state_t* state )
    {
      for ( const auto& a: assessors )
      {
        if ( a.assessor( type, state ) == STOP )
        {
          break;
        }
      }
    }
  };
} // Namespace assessor ends

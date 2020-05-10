// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sim/sc_sim.hpp"
#include "player/sc_player.hpp"
#include "util/util.hpp"
#include "dbc/data_enums.hh"
#include <sstream>

// Player Callbacks
template <typename T_CB>
struct effect_callbacks_t
{
  sim_t* sim;
  std::vector<T_CB*> all_callbacks;
  // New proc system

  // Callbacks (procs) stored in a vector
  typedef std::vector<T_CB*> proc_list_t;
  // .. an array of callbacks, for each proc_type2 enum (procced by hit/crit, etc...)
  typedef std::array<proc_list_t, PROC2_TYPE_MAX> proc_on_array_t;
  // .. an array of procced by arrays, for each proc_type enum (procced on aoe, heal, tick, etc...)
  typedef std::array<proc_on_array_t, PROC1_TYPE_MAX> proc_array_t;

  proc_array_t procs;

  effect_callbacks_t( sim_t* sim ) : sim( sim )
  { }

  bool has_callback( const std::function<bool(const T_CB*)> cmp ) const
  { return range::find_if( all_callbacks, cmp ) != all_callbacks.end(); }

  ~effect_callbacks_t()
  { range::sort( all_callbacks ); dispose( all_callbacks.begin(), range::unique( all_callbacks ) ); }

  void reset();

  void register_callback( unsigned proc_flags, unsigned proc_flags2, T_CB* cb );

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
  void add_proc_callback( proc_types type, unsigned flags, T_CB* cb );
};

template <typename T_CB>
void effect_callbacks_t<T_CB>::add_proc_callback(proc_types type,
  unsigned flags,
  T_CB* cb)
{
  std::stringstream s;
  if (sim->debug)
    s << "Registering procs: ";

  // Setup the proc-on-X types for the proc
  for (proc_types2 pt = PROC2_TYPE_MIN; pt < PROC2_TYPE_MAX; pt++)
  {
    if (!(flags & (1 << pt)))
      continue;

    // Healing and ticking spells all require "an amount" on landing, so
    // automatically convert a "proc on spell landed" type to "proc on
    // hit/crit".
    if (pt == PROC2_LANDED &&
      (type == PROC1_PERIODIC || type == PROC1_PERIODIC_TAKEN ||
        type == PROC1_PERIODIC_HEAL || type == PROC1_PERIODIC_HEAL_TAKEN ||
        type == PROC1_NONE_HEAL || type == PROC1_MAGIC_HEAL))
    {
      add_callback(procs[type][PROC2_HIT], cb);
      if (cb->listener->sim->debug)
        s << util::proc_type_string(type) << util::proc_type2_string(PROC2_HIT) << " ";

      add_callback(procs[type][PROC2_CRIT], cb);
      if (cb->listener->sim->debug)
        s << util::proc_type_string(type) << util::proc_type2_string(PROC2_CRIT) << " ";
    }
    // Do normal registration based on the existence of the flag
    else
    {
      add_callback(procs[type][pt], cb);
      if (cb->listener->sim->debug)
        s << util::proc_type_string(type) << util::proc_type2_string(pt) << " ";
    }
  }

  if (sim->debug)
    sim->out_debug.printf("%s", s.str().c_str());
}

template <typename T_CB>
void effect_callbacks_t<T_CB>::register_callback(unsigned proc_flags,
  unsigned proc_flags2,
  T_CB* cb)
{
  // We cannot default the "what kind of abilities proc this callback" flags,
  // they need to be non-zero
  assert(proc_flags != 0 && cb != 0);

  if (sim->debug)
    sim->out_debug.printf("Registering callback proc_flags=%#.8x proc_flags2=%#.8x",
      proc_flags, proc_flags2);

  // Default method for proccing is "on spell landing", if no "what type of
  // result procs this callback" is given
  if (proc_flags2 == 0)
    proc_flags2 = PF2_LANDED;

  for (proc_types t = PROC1_TYPE_MIN; t < PROC1_TYPE_BLIZZARD_MAX; t++)
  {
    // If there's no proc-by-X, we don't need to add anything
    if (!(proc_flags & (1 << t)))
      continue;

    // Skip periodic procs, they are handled below as a special case
    if (t == PROC1_PERIODIC || t == PROC1_PERIODIC_TAKEN)
      continue;

    add_proc_callback(t, proc_flags2, cb);
  }

  // Periodic X done
  if (proc_flags & PF_PERIODIC)
  {
    // 1) Periodic damage only. This is the default behavior of our system when
    // only PROC1_PERIODIC is defined on a trinket.
    if (!(proc_flags & PF_ALL_HEAL) &&                                               // No healing ability type flags
      !(proc_flags2 & PF2_PERIODIC_HEAL))                                         // .. nor periodic healing result type flag
    {
      add_proc_callback(PROC1_PERIODIC, proc_flags2, cb);
    }
    // 2) Periodic heals only. Either inferred by a "proc by direct heals" flag,
    //    or by "proc on periodic heal ticks" flag, but require that there's
    //    no direct / ticked spell damage in flags.
    else if (((proc_flags & PF_ALL_HEAL) || (proc_flags2 & PF2_PERIODIC_HEAL)) && // Healing ability
      !(proc_flags & PF_ALL_DAMAGE) &&                                        // .. with no damaging ability type flags
      !(proc_flags2 & PF2_PERIODIC_DAMAGE))                                  // .. nor periodic damage result type flag
    {
      add_proc_callback(PROC1_PERIODIC_HEAL, proc_flags2, cb);
    }
    // Both
    else
    {
      add_proc_callback(PROC1_PERIODIC, proc_flags2, cb);
      add_proc_callback(PROC1_PERIODIC_HEAL, proc_flags2, cb);
    }
  }

  // Periodic X taken
  if (proc_flags & PF_PERIODIC_TAKEN)
  {
    // 1) Periodic damage only. This is the default behavior of our system when
    // only PROC1_PERIODIC is defined on a trinket.
    if (!(proc_flags & PF_ALL_HEAL_TAKEN) &&                                         // No healing ability type flags
      !(proc_flags2 & PF2_PERIODIC_HEAL))                                         // .. nor periodic healing result type flag
    {
      add_proc_callback(PROC1_PERIODIC_TAKEN, proc_flags2, cb);
    }
    // 2) Periodic heals only. Either inferred by a "proc by direct heals" flag,
    //    or by "proc on periodic heal ticks" flag, but require that there's
    //    no direct / ticked spell damage in flags.
    else if (((proc_flags & PF_ALL_HEAL_TAKEN) || (proc_flags2 & PF2_PERIODIC_HEAL)) && // Healing ability
      !(proc_flags & PF_DAMAGE_TAKEN) &&                                        // .. with no damaging ability type flags
      !(proc_flags & PF_ANY_DAMAGE_TAKEN) &&                                    // .. nor Blizzard's own "any damage taken" flag
      !(proc_flags2 & PF2_PERIODIC_DAMAGE))                                    // .. nor periodic damage result type flag
    {
      add_proc_callback(PROC1_PERIODIC_HEAL_TAKEN, proc_flags2, cb);
    }
    // Both
    else
    {
      add_proc_callback(PROC1_PERIODIC_TAKEN, proc_flags2, cb);
      add_proc_callback(PROC1_PERIODIC_HEAL_TAKEN, proc_flags2, cb);
    }
  }
}

template <typename T_CB>
void effect_callbacks_t<T_CB>::reset()
{
  T_CB::reset(all_callbacks);
}

template <typename T>
inline void add_callback(std::vector<T*>& callbacks, T* cb)
{
  if (range::find(callbacks, cb) == callbacks.end())
    callbacks.push_back(cb);
}
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "effect_callbacks.hpp"

#include "action/action_callback.hpp"
#include "player/sc_player.hpp"
#include "sim/sc_sim.hpp"
#include "util/generic.hpp"

namespace
{
  
void add_callback( std::vector<action_callback_t*>& callbacks, action_callback_t* cb )
{
  if ( range::find( callbacks, cb ) == callbacks.end() )
    callbacks.push_back( cb );
}

}  // namespace

void effect_callbacks_t::add_proc_callback(proc_types type,
  unsigned flags,
  action_callback_t* cb)
{
  std::string s;
  if (sim->debug)
    s += "Registering procs: ";

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
        s.append(util::proc_type_string(type)).append(util::proc_type2_string(PROC2_HIT)).append(" ");

      add_callback(procs[type][PROC2_CRIT], cb);
      if (cb->listener->sim->debug)
        s.append(util::proc_type_string(type)).append(util::proc_type2_string(PROC2_CRIT)).append(" ");
    }
    // Do normal registration based on the existence of the flag
    else
    {
      add_callback(procs[type][pt], cb);
      if (cb->listener->sim->debug)
        s.append(util::proc_type_string(type)).append(util::proc_type2_string(pt)).append(" ");
    }
  }

  if (sim->debug)
    sim->out_debug.print("{}", s);
}

effect_callbacks_t::~effect_callbacks_t()
{
  range::sort( all_callbacks );
  dispose( all_callbacks.begin(), range::unique( all_callbacks ) );
}

bool effect_callbacks_t::has_callback( const std::function<bool( const action_callback_t* )> cmp ) const
{
  return range::any_of( all_callbacks, cmp );
}

void effect_callbacks_t::register_callback(unsigned proc_flags,
  unsigned proc_flags2,
  action_callback_t* cb)
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

void effect_callbacks_t::reset()
{
  action_callback_t::reset(all_callbacks);
}

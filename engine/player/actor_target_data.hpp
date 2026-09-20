// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "actor_pair.hpp"
#include "util/generic.hpp"

struct buff_t;
struct player_t;

struct actor_target_data_t : public actor_pair_t, private noncopyable
{
  struct atd_debuff_t
  {
    // NOTE: add debuffs that have player-scope implications (such as damage taken modifiers) or debuffs that will be
    // exposed to the APL to adjust actions based on their presence.
    //
    // These will require a targetdata_initializer in order to be created for all targets.
    //
    // Debuffs that are entirely self-contained within the proc or use action should be handled within the
    // generic_proc_t or dbc_proc_callback_t via target_specific_debuff.
  } debuff;

  struct atd_dot_t
  {
  } dot;

  actor_target_data_t( player_t* target, player_t* source );
};

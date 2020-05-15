// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include <vector>

struct actor_target_data_t;
struct player_t;
struct special_effect_t;

/**
 * Targetdata initializer for items. When targetdata is constructed (due to a call to
 * player_t::get_target_data failing to find an object for the given target), all targetdata
 * initializers in the sim are invoked. Most class specific targetdata is handled by the derived
 * class-specific targetdata, however there are a couple of trinkets that require "generic"
 * targetdata support. Item targetdata initializers will create the relevant debuffs/buffs needed.
 * Note that the buff/debuff needs to be created always, since expressions for buffs/debuffs presume
 * the buff exists or the whole sim fails to init.
 *
 * See unique_gear::register_target_data_initializers() for currently supported target-based debuffs
 * in generic items.
 */
struct item_targetdata_initializer_t
{
  unsigned item_id;
  std::vector< slot_e > slots_;

  item_targetdata_initializer_t(unsigned iid, const std::vector< slot_e >& s);

  virtual ~item_targetdata_initializer_t();

  // Returns the special effect based on item id and slots to source from. Overridable if more
  // esoteric functionality is needed
  virtual const special_effect_t* find_effect(player_t* player) const;

  // Override to initialize the targetdata object.
  virtual void operator()(actor_target_data_t*) const = 0;
};
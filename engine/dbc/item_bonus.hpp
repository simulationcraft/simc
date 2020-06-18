// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_BONUS_HPP
#define ITEM_BONUS_HPP

#include "util/span.hpp"

#include "client_data.hpp"

struct item_bonus_entry_t
{
  unsigned id;
  unsigned bonus_id;
  unsigned type;
  int      value_1;
  int      value_2;
  int      value_3;
  int      value_4;
  unsigned index;

  static util::span<const item_bonus_entry_t> find( unsigned id, bool ptr );

  static const item_bonus_entry_t& nil()
  { return dbc::nil<item_bonus_entry_t>; }

  static util::span<const item_bonus_entry_t> data( bool ptr );
};

#endif /* ITEM_BONUS_HPP */


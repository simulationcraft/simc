// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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
  unsigned index;

  static util::span<const item_bonus_entry_t> find( unsigned id, bool ptr );

  static const item_bonus_entry_t& nil()
  { return dbc::nil<item_bonus_entry_t>(); }

  static util::span<const item_bonus_entry_t> data( bool ptr );
};

#endif /* ITEM_BONUS_HPP */


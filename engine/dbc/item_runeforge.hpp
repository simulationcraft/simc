// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_RUNEFORGE_HPP
#define ITEM_RUNEFORGE_HPP

#include "util/span.hpp"
#include "util/string_view.hpp"

#include "client_data.hpp"

struct runeforge_legendary_entry_t
{
  unsigned bonus_id;
  unsigned specialization_id;
  unsigned spell_id;
  unsigned mask_inv_type;
  const char* name;

  static util::span<const runeforge_legendary_entry_t> find( unsigned bonus_id, bool ptr )
  { return dbc::find_many<runeforge_legendary_entry_t>( bonus_id, ptr, {}, &runeforge_legendary_entry_t::bonus_id ); }

  static util::span<const runeforge_legendary_entry_t> find( util::string_view name, bool ptr );

  static const runeforge_legendary_entry_t& nil()
  { return dbc::nil<runeforge_legendary_entry_t>; }

  static util::span<const runeforge_legendary_entry_t> data( bool ptr );
};

#endif /* ITEM_RUNEFORGE_HPP */

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef RACIAL_SPELLS_HPP
#define RACIAL_SPELLS_HPP

#include "util/span.hpp"
#include "util/string_view.hpp"

#include "sc_enums.hpp"

#include "client_data.hpp"

struct racial_spell_entry_t
{
  uint64_t    mask_race;
  unsigned    mask_class;
  unsigned    spell_id;
  const char* name;

  static const racial_spell_entry_t& find( util::string_view name, bool ptr, race_e r, player_e c );

  static const racial_spell_entry_t& nil()
  { return dbc::nil<racial_spell_entry_t>; }

  static util::span<const racial_spell_entry_t> data( bool ptr );
};

#endif /* RACIAL_SPELLS_HPP */


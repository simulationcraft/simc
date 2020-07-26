// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef RANK_SPELLS_HPP
#define RANK_SPELLS_HPP

#include "util/span.hpp"
#include "util/string_view.hpp"

#include "client_data.hpp"

struct rank_class_spell_t
{
  unsigned    class_id;
  unsigned    spec_id;
  unsigned    spell_id;
  unsigned    override_spell_id;
  const char* name;
  const char* rank;

  static const rank_class_spell_t& find( util::string_view name, util::string_view rank, bool ptr );

  static const rank_class_spell_t& nil()
  { return dbc::nil<rank_class_spell_t>; }

  static util::span<const rank_class_spell_t> data( bool ptr );
};

#endif /* RANK_SPELLS_HPP */


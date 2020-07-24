// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SPECIALIZATION_SPELL_HPP
#define SPECIALIZATION_SPELL_HPP

#include "util/span.hpp"
#include "util/string_view.hpp"

#include "specialization.hpp"
#include "client_data.hpp"

struct specialization_spell_entry_t
{
  unsigned    class_id;
  unsigned    specialization_id;
  unsigned    spell_id;
  unsigned    override_spell_id;
  const char* name;
  const char* desc;

  static const specialization_spell_entry_t& find( util::string_view name,
                                                   bool ptr,
                                                   specialization_e spec = SPEC_NONE,
                                                   util::string_view desc = "",
                                                   bool tokenized = false );

  static const specialization_spell_entry_t& nil()
  { return dbc::nil<specialization_spell_entry_t>; }

  static util::span<const specialization_spell_entry_t> data( bool ptr );
};

#endif /* SPECIALIZATION_SPELL_HPP */


// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef MASTERY_SPELLS_HPP
#define MASTERY_SPELLS_HPP

#include "util/span.hpp"

#include "specialization.hpp"

#include "client_data.hpp"

struct mastery_spell_entry_t
{
  unsigned specialization_id;
  unsigned spell_id;

  static const mastery_spell_entry_t& find( specialization_e spec, bool ptr )
  {
    return dbc::find<mastery_spell_entry_t>( static_cast<unsigned>( spec ), ptr,
        &mastery_spell_entry_t::specialization_id );
  }

  static const mastery_spell_entry_t& nil()
  { return dbc::nil<mastery_spell_entry_t>; }

  static util::span<const mastery_spell_entry_t> data( bool ptr );
};

#endif /* MASTERY_SPELLS_HPP */

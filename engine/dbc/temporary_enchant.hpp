// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef TEMPORARY_ENCHANT_HPP
#define TEMPORARY_ENCHANT_HPP

#include "util/span.hpp"
#include "util/util.hpp"

#include "client_data.hpp"

struct temporary_enchant_entry_t
{
  unsigned enchant_id;
  unsigned spell_id;
  const char* tokenized_name;

  static const temporary_enchant_entry_t& find( util::string_view name, bool ptr );
  static const temporary_enchant_entry_t& find_by_enchant_id( unsigned id, bool ptr );

  static const temporary_enchant_entry_t& nil()
  { return dbc::nil<temporary_enchant_entry_t>; }

  static util::span<const temporary_enchant_entry_t> data( bool ptr );
};

#endif /* TEMPORARY_ENCHANT_HPP */

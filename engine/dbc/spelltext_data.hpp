// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_SPELLDESC_DATA_HPP
#define SC_SPELLDESC_DATA_HPP

#include "dbc/client_data.hpp"
#include "dbc/client_hotfix_entry.hpp"
#include "util/span.hpp"

struct spelltext_data_t
{
  unsigned spell_id;
  const char* _desc;
  const char* _tooltip;
  const char* _rank;

  const char* desc() const
  { return _desc; }

  const char* tooltip() const
  { return _tooltip; }

  const char* rank() const
  { return _rank; }

  static const spelltext_data_t& nil()
  { return dbc::nil<spelltext_data_t>; }

  static const spelltext_data_t& find( unsigned spell_id, bool ptr )
  { return dbc::find<spelltext_data_t>( spell_id, ptr, &spelltext_data_t::spell_id ); }

  static util::span<const spelltext_data_t> data( bool ptr );
  static util::span<const hotfix::client_hotfix_entry_t> hotfixes( const spelltext_data_t&, bool ptr );
};

struct spelldesc_vars_data_t
{
  unsigned spell_id;
  const char* _desc_vars;

  const char* desc_vars() const
  { return _desc_vars; }

  static const spelldesc_vars_data_t& nil()
  { return dbc::nil<spelldesc_vars_data_t>; }

  static const spelldesc_vars_data_t& find( unsigned spell_id, bool ptr )
  { return dbc::find<spelldesc_vars_data_t>( spell_id, ptr, &spelldesc_vars_data_t::spell_id ); }

  static util::span<const spelldesc_vars_data_t> data( bool ptr );
  static util::span<const hotfix::client_hotfix_entry_t> hotfixes( const spelldesc_vars_data_t&, bool ptr );
};

#endif // SC_SPELLDESC_DATA_HPP

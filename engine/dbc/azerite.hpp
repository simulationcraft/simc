// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef AZERITE_HPP
#define AZERITE_HPP

#include "util/span.hpp"
#include "client_data.hpp"

struct azerite_power_entry_t
{
  unsigned    id;
  unsigned    spell_id;
  unsigned    bonus_id;
  const char* name;
  unsigned    tier;

  static const azerite_power_entry_t& find( unsigned id, bool ptr )
  { return dbc::find<azerite_power_entry_t>( id, ptr, &azerite_power_entry_t::id ); }

  static const azerite_power_entry_t& nil()
  { return dbc::nil<azerite_power_entry_t>; }

  static util::span<const azerite_power_entry_t> data( bool ptr );
};

struct azerite_essence_entry_t
{
  unsigned    id;
  unsigned    category;
  const char* name;

  static const azerite_essence_entry_t& find( unsigned id, bool ptr )
  { return dbc::find<azerite_essence_entry_t>( id, ptr, &azerite_essence_entry_t::id ); }

  static const azerite_essence_entry_t& nil()
  { return dbc::nil<azerite_essence_entry_t>; }

  static const azerite_essence_entry_t& find( util::string_view name, bool tokenized, bool ptr );

  static util::span<const azerite_essence_entry_t> data( bool ptr );
};

struct azerite_essence_power_entry_t
{
  unsigned id;
  unsigned essence_id;
  unsigned rank;
  unsigned spell_id_base[2];
  unsigned spell_id_upgrade[2];

  static const azerite_essence_power_entry_t& find( unsigned id, bool ptr )
  { return dbc::find<azerite_essence_power_entry_t>( id, ptr, &azerite_essence_power_entry_t::id ); }

  static const azerite_essence_power_entry_t& nil()
  { return dbc::nil<azerite_essence_power_entry_t>; }

  static const azerite_essence_power_entry_t& find_by_spell_id( unsigned spell_id, bool ptr );
  static util::span<const azerite_essence_power_entry_t> data( bool ptr );
  static util::span<const azerite_essence_power_entry_t> data_by_essence_id( unsigned essence_id, bool ptr );
};

#endif /* AZERITE_HPP */

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef CLASS_SPELLS_HPP
#define CLASS_SPELLS_HPP

#include "client_data.hpp"
#include "sc_enums.hpp"
#include "specialization.hpp"
#include "util/span.hpp"

struct active_class_spell_t
{
  unsigned class_id;
  unsigned spec_id;
  unsigned spell_id;
  unsigned override_spell_id;
  const char* name;

  static const active_class_spell_t& find( std::string_view name, bool ptr, bool tokenized = false );
  static const active_class_spell_t& find( std::string_view name, player_e class_, bool ptr, bool tokenized = false );
  static const active_class_spell_t& find( std::string_view name, specialization_e spec, bool ptr,
                                           bool tokenized = false );

  static const active_class_spell_t& nil()
  { return dbc::nil<active_class_spell_t>; }

  static util::span<const active_class_spell_t> data( bool ptr );
};

struct active_pet_spell_t
{
  unsigned owner_class_id;
  unsigned spell_id;
  const char* name;

  static const active_pet_spell_t& find( std::string_view name, bool ptr, bool tokenized = false );
  static const active_pet_spell_t& find( std::string_view name, player_e class_, bool ptr, bool tokenized = false );

  static const active_pet_spell_t& nil()
  { return dbc::nil<active_pet_spell_t>; }

  static util::span<const active_pet_spell_t> data( bool ptr );
};

struct passive_class_spell_t
{
  unsigned class_id;
  unsigned spell_id;
  const char* name;

  static const passive_class_spell_t& find( std::string_view name, bool ptr, bool tokenized = false );
  static const passive_class_spell_t& find( std::string_view name, player_e class_, bool ptr, bool tokenized = false );

  static const passive_class_spell_t& nil()
  { return dbc::nil<passive_class_spell_t>; }

  static util::span<const passive_class_spell_t> data( bool ptr );
};

#endif /* CLASS_SPELLS_HPP */

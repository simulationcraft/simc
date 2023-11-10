// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef EXPECTED_STAT_HPP
#define EXPECTED_STAT_HPP

#include "client_data.hpp"
#include "util/span.hpp"

struct expected_stat_t
{
  unsigned level;
  float creature_auto_attack_dps;
  float creature_armor;
  float player_primary_stat;
  float player_secondary_stat;
  float armor_constant;
  float creature_spell_damage;

  static const expected_stat_t& find( unsigned level, bool ptr )
  { return dbc::find<expected_stat_t>( level, ptr, &expected_stat_t::level ); }

  static const expected_stat_t& nil()
  { return dbc::nil<expected_stat_t>; }

  static util::span<const expected_stat_t> data( bool ptr );
};

struct expected_stat_mod_t
{
  unsigned id;
  float creature_auto_attack_dps;
  float creature_armor;
  float player_primary_stat;
  float player_secondary_stat;
  float armor_constant;
  float creature_spell_damage;
  unsigned difficulty;

  static const expected_stat_mod_t& find( unsigned id, bool ptr )
  { return dbc::find<expected_stat_mod_t>( id, ptr, &expected_stat_mod_t::id ); }

  static const expected_stat_mod_t& nil()
  { return dbc::nil<expected_stat_mod_t>; }

  static util::span<const expected_stat_mod_t> data( bool ptr );
};

#endif /* EXPECTED_STAT_HPP */

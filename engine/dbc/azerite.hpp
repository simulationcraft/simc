// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef AZERITE_HPP
#define AZERITE_HPP

#include "util/array_view.hpp"

struct azerite_power_entry_t
{
  unsigned    id;
  unsigned    spell_id;
  unsigned    bonus_id;
  const char* name;
  unsigned    tier;

  static const azerite_power_entry_t& find( unsigned id, bool ptr = false );
  static const azerite_power_entry_t& nil();
  static arv::array_view<azerite_power_entry_t> data( bool ptr = false );
};

#endif /* AZERITE_HPP */

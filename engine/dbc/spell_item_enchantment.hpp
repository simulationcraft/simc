// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SPELL_ITEM_ENCHANTMENT_HPP
#define SPELL_ITEM_ENCHANTMENT_HPP

#include "util/span.hpp"

#include "client_data.hpp"

struct item_enchantment_data_t
{
  unsigned    id;
  unsigned    id_gem;
  int         id_scaling;
  unsigned    min_scaling_level;   // need to verify these
  unsigned    max_scaling_level;
  unsigned    min_ilevel;
  unsigned    max_ilevel;
  unsigned    req_skill;
  unsigned    req_skill_value;
  unsigned    ench_type[3];        // item_enchantment
  int         ench_amount[3];
  unsigned    ench_prop[3];        // item_mod_type
  float       ench_coeff[3];       // item enchant scaling multiplier for data table
  unsigned    id_spell;            // reverse mapped spell id for this enchant
  const char* name;

  static const item_enchantment_data_t& find( unsigned id, bool ptr )
  { return dbc::find<item_enchantment_data_t>( id, ptr, &item_enchantment_data_t::id ); }

  static const item_enchantment_data_t& nil()
  { return dbc::nil<item_enchantment_data_t>; }

  static util::span<const item_enchantment_data_t> data( bool ptr );
};

#endif /* SPELL_ITEM_ENCHANTMENT */

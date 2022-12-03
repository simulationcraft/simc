// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef PERMANENT_ENCHANT_HPP
#define PERMANENT_ENCHANT_HPP

#include "util/span.hpp"
#include "util/util.hpp"

#include "dbc/data_enums.hh"

#include "client_data.hpp"

struct permanent_enchant_entry_t
{
  unsigned enchant_id;
  unsigned rank;
  int item_class;
  int mask_inventory_type;
  int mask_item_subclass;
  const char* tokenized_name;

  static const permanent_enchant_entry_t& find( util::string_view name, unsigned rank,
    int item_class, int inv_type, int item_subclass, bool ptr );
  static const permanent_enchant_entry_t& find( unsigned enchant_id, bool ptr );

  static const permanent_enchant_entry_t& nil()
  { return dbc::nil<permanent_enchant_entry_t>; }

  static util::span<const permanent_enchant_entry_t> data( bool ptr );
};

#endif /* PERMANENT_ENCHANT_HPP */

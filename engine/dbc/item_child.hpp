// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_CHILD_HPP
#define ITEM_CHILD_HPP

#include "util/span.hpp"

#include "client_data.hpp"

struct item_child_equipment_t
{
  unsigned id;
  unsigned id_item;
  unsigned id_child;

  static const item_child_equipment_t& find( unsigned id, bool ptr )
  { return dbc::find<item_child_equipment_t>( id, ptr, &item_child_equipment_t::id ); }

  static const item_child_equipment_t& nil()
  { return dbc::nil<item_child_equipment_t>(); }

  static util::span<const item_child_equipment_t> data( bool ptr );
};

#endif /* ITEM_CHILD_HPP */

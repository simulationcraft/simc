// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_NAMING_HPP
#define ITEM_NAMING_HPP

#include "util/array_view.hpp"

#include "client_data.hpp"

struct item_name_description_t
{
  unsigned id;
  const char* description;

  static const item_name_description_t& find( unsigned id, bool ptr )
  { return dbc::find<item_name_description_t>( id, ptr ); }

  static const item_name_description_t& nil()
  { return dbc::nil<item_name_description_t>(); }

  static arv::array_view<item_name_description_t> data( bool ptr );
};

#endif /* ITEM_NAMING_HPP */

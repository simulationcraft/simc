// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef GEM_DATA_HPP
#define GEM_DATA_HPP

#include "util/span.hpp"

#include "client_data.hpp"

struct gem_property_data_t
{
  unsigned id;
  unsigned enchant_id;
  unsigned color;

  static const gem_property_data_t& find( unsigned id, bool ptr )
  { return dbc::find<gem_property_data_t>( id, ptr, &gem_property_data_t::id ); }

  static const gem_property_data_t& nil()
  { return dbc::nil<gem_property_data_t>(); }

  static util::span<const gem_property_data_t> data( bool ptr );
};

#endif /* GEM_DATA_HPP */


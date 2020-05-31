// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef RAND_PROP_POINTS_HPP
#define RAND_PROP_POINTS_HPP

#include "util/span.hpp"

#include "client_data.hpp"

struct random_prop_data_t
{
  unsigned ilevel;
  unsigned damage_replace_stat;
  unsigned damage_secondary;
  double   p_epic[5];
  double   p_rare[5];
  double   p_uncommon[5];

  static const random_prop_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<random_prop_data_t>( ilevel, ptr, &random_prop_data_t::ilevel ); }

  static const random_prop_data_t& nil()
  { return dbc::nil<random_prop_data_t>(); }

  static util::span<const random_prop_data_t> data( bool ptr );
};

#endif /* RAND_PROP_POINTS_HPP */

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
  double   damage_replace_stat;
  double   damage_secondary;
  float    p_epic[5];
  float    p_rare[5];
  float    p_uncommon[5];

  static const random_prop_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<random_prop_data_t>( ilevel, ptr, &random_prop_data_t::ilevel ); }

  static const random_prop_data_t& nil()
  { return dbc::nil<random_prop_data_t>; }

  static util::span<const random_prop_data_t> data( bool ptr );
};

#endif /* RAND_PROP_POINTS_HPP */

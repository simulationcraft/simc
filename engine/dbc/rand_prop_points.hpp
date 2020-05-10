// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef RAND_PROP_POINTS_HPP
#define RAND_PROP_POINTS_HPP

#include "util/array_view.hpp"

#include "client_data.hpp"

struct random_prop_data_t
{
private:
  using key_t = dbc::ilevel_member_policy_t;

public:
  unsigned ilevel;
  unsigned damage_replace_stat;
  unsigned damage_secondary;
  double   p_epic[5];
  double   p_rare[5];
  double   p_uncommon[5];

  static const random_prop_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<random_prop_data_t, key_t>( ilevel, ptr ); }

  static const random_prop_data_t& nil()
  { return dbc::nil<random_prop_data_t>(); }

  static const arv::array_view<random_prop_data_t> data( bool ptr );
};

#endif /* RAND_PROP_POINTS_HPP */

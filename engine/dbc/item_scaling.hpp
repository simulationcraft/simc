// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_SCALING_HPP
#define ITEM_SCALING_HPP

#include "util/generic.hpp"
#include "util/array_view.hpp"

#include "client_data.hpp"

struct scaling_stat_distribution_t
{
  unsigned id;
  unsigned min_level;
  unsigned max_level;
  unsigned curve_id;

  static const scaling_stat_distribution_t& find( unsigned id, bool ptr )
  { return dbc::find<scaling_stat_distribution_t>( id, ptr ); }

  static const scaling_stat_distribution_t& nil()
  { return dbc::nil<scaling_stat_distribution_t>(); }

  static const arv::array_view<scaling_stat_distribution_t> data( bool ptr );
};

struct curve_point_t
{
  unsigned curve_id;
  unsigned index;
  double   val1;
  double   val2;

  static const arv::array_view<curve_point_t> find( unsigned id, bool ptr );

  static const curve_point_t& nil()
  { return dbc::nil<curve_point_t>(); }

  static const arv::array_view<curve_point_t> data( bool ptr );
};


#endif /* ITEM_SCALING_HPP */



// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef CONTENT_TUNING_HPP
#define CONTENT_TUNING_HPP

#include "client_data.hpp"
#include "util/span.hpp"

struct content_tuning_data_t
{
  unsigned id;
  int flags;
  int id_expansion;
  int min_level_squish;
  int max_level_squish;
  int ilevel;

  static const content_tuning_data_t& find( unsigned id, bool ptr )
  { return dbc::find<content_tuning_data_t>( id, ptr, &content_tuning_data_t::id ); }

  static const content_tuning_data_t& nil()
  { return dbc::nil<content_tuning_data_t>; }

  static util::span<const content_tuning_data_t> data( bool ptr );
};

#endif /* CONTENT_TUNING_HPP */

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef EMBELLISHMENT_DATA_HPP
#define EMBELLISHMENT_DATA_HPP

#include "client_data.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"

struct embellishment_data_t
{
  const char* name;
  unsigned bonus_id;
  unsigned effect_id;

  static util::span<const embellishment_data_t> data( bool ptr );
  static const embellishment_data_t& find( std::string_view name, bool ptr, bool tokenized );
};

#endif
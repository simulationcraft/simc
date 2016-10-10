// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#pragma once

#include <cassert>
#include "generated/sc_specialization_data.inc"

namespace specdata
{
inline unsigned spec_count()
{
  return n_specs;
}

inline specialization_e spec_id( unsigned idx )
{
  assert( idx < spec_count() );
  return __specs[ idx ];
}

inline int spec_idx( specialization_e spec )
{
  assert( spec < spec_to_idx_map_len );
  return __idx_specs[ spec ];
}

}  // namespace specdata

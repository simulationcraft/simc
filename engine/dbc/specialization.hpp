// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SPECIALIZATION_HPP
#define SPECIALIZATION_HPP

#include "generated/sc_specialization_data.inc"
#include <cassert>

namespace specdata {

inline unsigned spec_count()
{ return n_specs; }

inline specialization_e spec_id( unsigned idx )
{ assert( idx < spec_count() ); return __specs[ idx ]; }

inline int spec_idx( specialization_e spec )
{ assert( spec < spec_to_idx_map_len ); return __idx_specs[ spec ]; }

} // namespace specdata

#endif


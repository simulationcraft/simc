// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef REAL_PPM_HPP
#define REAL_PPM_HPP

#include "util/span.hpp"

#include "client_data.hpp"

struct rppm_modifier_t
{
  unsigned         spell_id;
  unsigned         type;
  unsigned         modifier_type;
  double           coefficient;

  static util::span<const rppm_modifier_t> find( unsigned spell_id, bool ptr )
  { return dbc::find_many<rppm_modifier_t>( spell_id, ptr, {}, &rppm_modifier_t::spell_id ); }

  static const rppm_modifier_t& nil()
  { return dbc::nil<rppm_modifier_t>; }

  static util::span<const rppm_modifier_t> data( bool ptr );
};

#endif /* REAL_PPM_HPP */

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "assisted_combat.hpp"

#include "config.hpp"
#include "dbc/dbc.hpp"

#include "generated/assisted_combat.inc"
#if SC_USE_PTR == 1
#include "generated/assisted_combat_ptr.inc"
#endif

util::span<const assisted_combat_step_data_t> assisted_combat_step_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __assisted_combat_step_data, __ptr_assisted_combat_step_data, ptr );
}

util::span<const assisted_combat_step_data_t> assisted_combat_step_data_t::data( specialization_e spec, bool ptr )
{
  auto data = assisted_combat_step_data_t::data( ptr );
  auto _class_range = range::equal_range( data, static_cast<unsigned>( spec ), {}, &assisted_combat_step_data_t::spec_idx );

  return { _class_range.first, _class_range.second };
}

util::span<const assisted_combat_rule_data_t> assisted_combat_rule_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __assisted_combat_rule_data, __ptr_assisted_combat_rule_data, ptr );
}

util::span<const assisted_combat_rule_data_t> assisted_combat_rule_data_t::data( unsigned assisted_combat_step_id, bool ptr )
{
  auto data = assisted_combat_rule_data_t::data( ptr );
  auto _class_range = range::equal_range( data, assisted_combat_step_id, {}, &assisted_combat_rule_data_t::assisted_combat_step_id );

  return { _class_range.first, _class_range.second };
}

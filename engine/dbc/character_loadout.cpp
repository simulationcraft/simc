// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "character_loadout.hpp"

#include "config.hpp"
#include "dbc/dbc.hpp"

#include "generated/character_loadout.inc"
#if SC_USE_PTR == 1
#include "generated/character_loadout_ptr.inc"
#endif

util::span<const character_loadout_data_t> character_loadout_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __character_loadout_data, __ptr_character_loadout_data, ptr );
}

util::span<const character_loadout_data_t> character_loadout_data_t::data( unsigned class_idx, bool ptr )
{
  auto data = character_loadout_data_t::data( ptr );
  auto _class_range = range::equal_range( data, class_idx, {}, &character_loadout_data_t::class_idx );

  return { _class_range.first, _class_range.second };
}

util::span<const character_loadout_data_t> character_loadout_data_t::data( unsigned class_idx, unsigned spec_idx,
                                                                           bool ptr )
{
  auto data = character_loadout_data_t::data( class_idx, ptr );
  auto _spec_range = range::equal_range( data, spec_idx, {}, &character_loadout_data_t::spec_idx );

  return { _spec_range.first, _spec_range.second };
}

int character_loadout_data_t::default_item_level()
{
  return MYTHIC_TARGET_ITEM_LEVEL;
}
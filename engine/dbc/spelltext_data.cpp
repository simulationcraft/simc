
#include "spelltext_data.hpp"

#include <array>

#include "generated/spelltext_data.inc"
#if SC_USE_PTR
#  include "generated/spelltext_data_ptr.inc"
#endif

util::span<const spelltext_data_t> spelltext_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __spelltext_data, __ptr_spelltext_data, ptr );
}

util::span<const hotfix::client_hotfix_entry_t> spelltext_data_t::hotfixes( const spelltext_data_t& std, bool ptr )
{
  auto data = SC_DBC_GET_DATA( __spelltext_hotfix_data, __ptr_spelltext_hotfix_data, ptr );
  return find_hotfixes( data, std.spell_id );
}

util::span<const spelldesc_vars_data_t> spelldesc_vars_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __spelldesc_vars_data, __ptr_spelldesc_vars_data, ptr );
}

util::span<const hotfix::client_hotfix_entry_t> spelldesc_vars_data_t::hotfixes( const spelldesc_vars_data_t& sdvd, bool ptr )
{
  auto data = SC_DBC_GET_DATA( __spelldesc_vars_hotfix_data, __ptr_spelldesc_vars_hotfix_data, ptr );
  return find_hotfixes( data, sdvd.spell_id );
}


#include "spelltext_data.hpp"

#include <array>

struct spelldesc_vars_index_t
{
  unsigned spell_id;
  unsigned id;
};

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
  return find_hotfixes( data, std.id );
}

const spelldesc_vars_data_t& spelldesc_vars_data_t::find( unsigned spell_id, bool ptr )
{
  const auto index_data = SC_DBC_GET_DATA( __spelldesc_vars_index_data, __ptr_spelldesc_vars_index_data, ptr );
  auto index = range::lower_bound( index_data, spell_id, {}, &spelldesc_vars_index_t::spell_id );
  if ( index == index_data.end() || index -> spell_id != spell_id )
    return nil();

  return dbc::find<spelldesc_vars_data_t>( index -> id, ptr, &spelldesc_vars_data_t::id );
}

util::span<const spelldesc_vars_data_t> spelldesc_vars_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __spelldesc_vars_data, __ptr_spelldesc_vars_data, ptr );
}

util::span<const hotfix::client_hotfix_entry_t> spelldesc_vars_data_t::hotfixes( const spelldesc_vars_data_t& sdvd, bool ptr )
{
  auto data = SC_DBC_GET_DATA( __spelldesc_vars_hotfix_data, __ptr_spelldesc_vars_hotfix_data, ptr );
  return find_hotfixes( data, sdvd.id );
}

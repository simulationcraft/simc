#include <array>

#include "config.hpp"
#include "util/util.hpp"

#include "azerite.hpp"

#include "generated/azerite.inc"
#if SC_USE_PTR == 1
#include "generated/azerite_ptr.inc"
#endif

util::span<const azerite_power_entry_t> azerite_power_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __azerite_power_data, __ptr_azerite_power_data, ptr );
}

util::span<const azerite_essence_entry_t> azerite_essence_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __azerite_essence_data, __ptr_azerite_essence_data, ptr );
}

const azerite_essence_entry_t&
azerite_essence_entry_t::find( util::string_view name, bool tokenized, bool ptr )
{
  const auto __data = data( ptr );

  for (const auto& entry : __data)
  {
    if ( tokenized )
    {
      auto s = util::tokenize_fn( entry.name );

      if ( util::str_compare_ci( name, s ) )
      {
        return entry;
      }
    }
    else
    {
      if ( util::str_compare_ci( name, entry.name ) )
      {
        return entry;
      }
    }
  }

  return nil();
}

util::span<const azerite_essence_power_entry_t> azerite_essence_power_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __azerite_essence_power_data, __ptr_azerite_essence_power_data, ptr );
}

util::span<const azerite_essence_power_entry_t>
azerite_essence_power_entry_t::data_by_essence_id( unsigned essence_id, bool ptr )
{
  const auto __data = data( ptr );

  auto r = range::equal_range( __data, essence_id, {}, &azerite_essence_power_entry_t::essence_id );
  if ( r.first == __data.end() )
  {
    return {};
  }

  return { r.first, r.second };
}

const azerite_essence_power_entry_t&
azerite_essence_power_entry_t::find_by_spell_id( unsigned spell_id, bool ptr )
{
  const auto __data = data( ptr );

  auto it = range::find_if( __data, [ spell_id ]( const azerite_essence_power_entry_t& e ) {
    return e.spell_id_base[ 0 ] == spell_id || e.spell_id_base[ 1 ] == spell_id ||
           e.spell_id_upgrade[ 0 ] == spell_id || e.spell_id_upgrade[ 1 ] == spell_id;
  } );

  if ( it != __data.cend() && it -> id != 0 )
  {
    return *it;
  }
  else
  {
    return nil();
  }
}


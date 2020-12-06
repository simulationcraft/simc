#include "config.hpp"

#include "temporary_enchant.hpp"

#include "generated/temporary_enchant.inc"
#if SC_USE_PTR == 1
#include "generated/temporary_enchant_ptr.inc"
#endif

const temporary_enchant_entry_t& temporary_enchant_entry_t::find( util::string_view name, bool ptr )
{
  auto __data = data( ptr );
  auto it = range::lower_bound( __data, name, {}, &temporary_enchant_entry_t::tokenized_name );
  if ( it == __data.end() || !util::str_compare_ci( it->tokenized_name, name ) )
  {
    return nil();
  }

  return *it;
}

const temporary_enchant_entry_t& temporary_enchant_entry_t::find_by_enchant_id(
    unsigned id, bool ptr )
{
  auto __data = data( ptr );
  auto it = range::find_if( __data, [id]( const temporary_enchant_entry_t& e ) {
    return id == e.enchant_id;
  } );

  if ( it == __data.end() )
  {
    return nil();
  }

  return *it;
}


util::span<const temporary_enchant_entry_t> temporary_enchant_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __temporary_enchant_data, __ptr_temporary_enchant_data, ptr );
}

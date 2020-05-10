#include <algorithm>
#include <array>

#include "config.hpp"
#include "util/util.hpp"

#include "azerite.hpp"

#include "generated/azerite.inc"
#if SC_USE_PTR == 1
#include "generated/azerite_ptr.inc"
#endif

arv::array_view<azerite_power_entry_t> azerite_power_entry_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<azerite_power_entry_t>( __ptr_azerite_power_data )
                        : arv::array_view<azerite_power_entry_t>( __azerite_power_data );
#else
  ( void ) ptr;
  const auto& data = __azerite_power_data;
#endif

  return data;
}

arv::array_view<azerite_essence_entry_t> azerite_essence_entry_t::data( bool ptr )
{
  ( void ) ptr;

#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<azerite_essence_entry_t>( __ptr_azerite_essence_data )
                        : arv::array_view<azerite_essence_entry_t>( __azerite_essence_data );
#else
  const auto& data = __azerite_essence_data;
#endif

  return data;
}

const azerite_essence_entry_t&
azerite_essence_entry_t::find( const std::string& name, bool tokenized, bool ptr )
{
  const auto __data = data( ptr );

  for ( auto it = __data.cbegin(); it != __data.cend(); ++it )
  {
    if ( tokenized )
    {
      std::string s = it->name;
      util::tokenize( s );

      if ( util::str_compare_ci( name, s ) )
      {
        return *it;
      }
    }
    else
    {
      if ( util::str_compare_ci( name, it->name ) )
      {
        return *it;
      }
    }
  }

  return nil();
}

arv::array_view<azerite_essence_power_entry_t> azerite_essence_power_entry_t::data( bool ptr )
{
  ( void ) ptr;

#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<azerite_essence_power_entry_t>( __ptr_azerite_essence_power_data )
                        : arv::array_view<azerite_essence_power_entry_t>( __azerite_essence_power_data );
#else
  const auto data = __azerite_essence_power_data;
#endif

  return data;
}

arv::array_view<azerite_essence_power_entry_t>
azerite_essence_power_entry_t::data_by_essence_id( unsigned essence_id, bool ptr )
{
  const auto __data = data( ptr );

  auto begin = std::lower_bound( __data.cbegin(), __data.cend(), essence_id,
                              []( const azerite_essence_power_entry_t& a, const unsigned& id ) {
                                return a.essence_id < id;
                              } );

  if ( begin == __data.cend() )
  {
    return {};
  }

  auto end = std::upper_bound( __data.cbegin(), __data.cend(), essence_id,
                              []( const unsigned& id, const azerite_essence_power_entry_t& a ) {
                                return id < a.essence_id;
                              } );

  return arv::array_view<azerite_essence_power_entry_t>( begin, end );
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


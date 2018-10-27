#include <algorithm>

#include "azerite.hpp"

#include "dbc/dbc.hpp"

#include "generated/azerite.inc"
#if SC_USE_PTR == 1
#include "generated/azerite_ptr.inc"
#endif

const azerite_power_entry_t& azerite_power_entry_t::find( unsigned id, bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<azerite_power_entry_t>(__ptr_azerite_power_data)
    : arv::array_view<azerite_power_entry_t>(__azerite_power_data);
#else
  ( void ) ptr;
  const auto& data = __azerite_power_data;
#endif

  auto it = std::lower_bound( data.cbegin(), data.cend(), id,
                              []( const azerite_power_entry_t& a, const unsigned& id ) {
                                return a.id < id;
                              } );

  if ( it != data.cend() && it -> id == id )
  {
    return *it;
  }
  else
  {
    return nil();
  }
}

const azerite_power_entry_t& azerite_power_entry_t::nil()
{
  static azerite_power_entry_t __default {};

  return __default;
}

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


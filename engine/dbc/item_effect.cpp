#include <algorithm>
#include <iostream>

#include "item_effect.hpp"
#include "dbc.hpp"

#include "generated/item_effect.inc"
#if SC_USE_PTR == 1
#include "generated/item_effect_ptr.inc"
#endif

const item_effect_t& item_effect_t::find( unsigned id, bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr
    ? arv::array_view<item_effect_t>( __ptr_item_effect_data )
    : arv::array_view<item_effect_t>( __item_effect_data );
#else
  ( void ) ptr;
  const auto data = arv::array_view<item_effect_t>( __item_effect_data );
#endif

  auto it = std::lower_bound( data.cbegin(), data.cend(), id,
                              []( const item_effect_t& a, const unsigned& id ) {
                                return a.id < id;
                              } );

  if ( it != data.cend() && it->id == id )
  {
    return *it;
  }
  else
  {
    return nil();
  }
}

const item_effect_t& item_effect_t::nil()
{
  static item_effect_t __default {};

  return __default;
}

arv::array_view<item_effect_t> item_effect_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr
    ? arv::array_view<item_effect_t>( __ptr_item_effect_data )
    : arv::array_view<item_effect_t>( __item_effect_data );
#else
  ( void ) ptr;
  const auto& data = arv::array_view<item_effect_t>( __item_effect_data );
#endif

  return data;
}


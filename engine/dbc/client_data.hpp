// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef CLIENT_DATA_HPP
#define CLIENT_DATA_HPP

#include <algorithm>

#include "config.hpp"

namespace dbc
{
struct ilevel_member_policy_t
{
  template <typename T> static constexpr unsigned key( const T& entry )
  { return static_cast<unsigned>( entry.ilevel ); }
};

struct id_member_policy_t
{
  template <typename T> static constexpr unsigned key( const T& t )
  { return static_cast<unsigned>( t.id ); }
};

template <typename T>
const T& nil()
{
  static T __default {};
  return __default;
}

template <typename T, typename KeyPolicy = id_member_policy_t>
const T& find( unsigned key, bool ptr )
{
  const auto __data = T::data( ptr );

  auto it = std::lower_bound( __data.cbegin(), __data.cend(), key,
                              []( const T& entry, const unsigned& key ) {
                                return KeyPolicy::key( entry ) < key;
                              } );

  if ( it != __data.cend() && KeyPolicy::key( *it ) == key )
  {
    return *it;
  }
  else
  {
    return nil<T>();
  }
}

} // Namespace dbc ends

#if defined(SC_USE_PTR) && (SC_USE_PTR != 0)
# define SC_DBC_GET_DATA( _data__, _data_ptr__, _ptr_flag__ ) \
    ((_ptr_flag__) ? ::util::make_span( _data_ptr__ ).subspan( 0 ) : ::util::make_span( _data__ ).subspan( 0 ))
#else
# define SC_DBC_GET_DATA( _data__, _data_ptr__, _ptr_flag__ ) \
    ((void)(_ptr_flag__), ::util::make_span( _data__ ))
#endif

#endif /* CLIENT_DATA_HPP */

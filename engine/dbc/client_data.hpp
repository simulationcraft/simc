// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef CLIENT_DATA_HPP
#define CLIENT_DATA_HPP

#include "config.hpp"

#include <algorithm>
#include <vector>
#include <string>

#include "util/generic.hpp"
#include "util/span.hpp"

namespace dbc
{

// TODO C++17: replace with inline constexpr
template <typename T>
constexpr const T& nil = meta::static_const_t<T>::value;

template <typename T, typename Proj>
const T& find( unsigned key, bool ptr, Proj proj )
{
  const auto __data = T::data( ptr );

  auto it = range::lower_bound( __data, key, {}, proj );
  if ( it != __data.cend() && std::invoke( proj, *it ) == key )
  {
    return *it;
  }

  return nil<T>;
}

template <typename T, typename Compare = std::less<>, typename Proj = range::identity>
util::span<const T> find_many( unsigned key, bool ptr, Compare comp = Compare{}, Proj proj = Proj{} )
{
  const auto __data = T::data( ptr );

  auto r = range::equal_range( __data, key, comp, proj );
  if ( r.first == __data.end() )
  {
    return {};
  }

  return util::span<const T>( r.first, r.second );
}

template <typename T, size_t N, typename U, size_t M, typename Proj>
const T& find_indexed( unsigned key, util::span<const T, N> data, util::span<const U, M> index, Proj proj )
{
  auto it = range::lower_bound( index, key, {}, [ data, &proj ]( auto index ) {
      return std::invoke( proj, data[ index ] );
    } );
  if ( it != index.end() && std::invoke( proj, data[ *it ] ) == key )
    return data[ *it ];
  return T::nil();
}

// "Index" to provide access to a filtered list of dbc data.
template <typename T, typename Filter>
class filtered_dbc_index_t
{
#if SC_USE_PTR == 0
  std::vector<const T*> __filtered_index[ 1 ];
#else
  std::vector<const T*> __filtered_index[ 2 ];
#endif
  Filter f;

public:
  // Initialize index from given list
  void init( util::span<const T> list, bool ptr )
  {
    for ( const auto& i : list )
    {
      if ( f( i ) )
      {
        __filtered_index[ ptr ].push_back( &i );
      }
    }
  }

  void init( util::span<const util::span<const T>> list, bool ptr )
  {
    for ( auto sublist : list )
      init( sublist, ptr );
  }

  template <typename Predicate>
  const T& get( bool ptr, Predicate&& pred ) const
  {
    const auto& index = __filtered_index[ ptr ];
    auto it = range::find_if( index, std::forward<Predicate>( pred ) );
    if ( it != index.end() )
    {
      const T* v = *it;
      return *v;
    }
    return T::nil();
  }
};

// Return World of Warcraft client data version used to generate the current client data
std::string client_data_version_str( bool ptr );
// Return World of Warcraft client data build version used to generate the current client data
int client_data_build( bool ptr );
// Return the modified timestamp of the hotfix cache file used to generated the current
// client data.
std::string hotfix_date_str( bool ptr );
// Return the build number contained in the hotfix cache file used to generate the current
// client data.
int hotfix_build_version( bool ptr );
// Return the verification hash associated with the hotfix cache file used to generate the
// current client data.
std::string hotfix_hash_str( bool ptr );


} // Namespace dbc ends

#if defined(SC_USE_PTR) && (SC_USE_PTR != 0)
# define SC_DBC_GET_DATA( _data__, _data_ptr__, _ptr_flag__ ) \
    ((_ptr_flag__) ? ::util::make_span( _data_ptr__ ).subspan( 0 ) : ::util::make_span( _data__ ).subspan( 0 ))
#else
# define SC_DBC_GET_DATA( _data__, _data_ptr__, _ptr_flag__ ) \
    ((void)(_ptr_flag__), ::util::make_span( _data__ ))
#endif

#endif /* CLIENT_DATA_HPP */

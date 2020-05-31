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
struct ilevel_member_policy_t
{
  template <typename T> static constexpr unsigned key( const T& entry )
  { return static_cast<unsigned>( entry.ilevel ); }
};

/* id_function_policy and id_member_policy are here to give a standard interface
 * of accessing the id of a data type.
 * Eg. spell_data_t on which the id_function_policy is used has a function 'id()' which returns its id
 * and item_data_t on which id_member_policy is used has a member 'id' which stores its id.
 */
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

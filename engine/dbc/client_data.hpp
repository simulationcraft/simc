// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef CLIENT_DATA_HPP
#define CLIENT_DATA_HPP

#include <algorithm>
#include <vector>

#include "config.hpp"
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

struct id_function_policy_t
{
  template <typename T> static unsigned id( const T& t )
  { return static_cast<unsigned>( t.id() ); }
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

// ==========================================================================
// Indices to provide log time, constant space access to spells, effects, and talents by id.
// ==========================================================================

template <typename T, typename KeyPolicy = id_function_policy_t>
class dbc_index_t
{
private:
  typedef std::pair<T*, T*> index_t; // first = lowest data; second = highest data
// array of size 1 or 2, depending on whether we have PTR data
#if SC_USE_PTR == 0
  index_t idx[ 1 ];
#else
  index_t idx[ 2 ];
#endif

  /* populate idx with pointer to lowest and highest data from a given list
   */
  void populate( index_t& idx, T* list )
  {
    assert( list );
    idx.first = list;
    for ( unsigned last_id = 0; KeyPolicy::id( *list ); last_id = KeyPolicy::id( *list ), ++list )
    {
      // Validate the input range is in fact sorted by id.
      assert( KeyPolicy::id( *list ) > last_id ); ( void )last_id;
    }
    idx.second = list;
  }

public:
  // Initialize index from given list
  void init( T* list, bool ptr )
  {
    assert( ! initialized( ptr ) );
    populate( idx[ ptr ], list );
  }

  // Initialize index under the assumption that 'T::list( bool ptr )' returns a list of data
  void init()
  {
    init( T::list( false ), false );
#if SC_USE_PTR == 1
    init( T::list( true ), true );
#endif
  }

  bool initialized( bool ptr = false ) const
  { return idx[ ptr ].first != 0; }

  // Return the item with the given id, or NULL
  T* get( bool ptr, unsigned id ) const
  {
    assert( initialized( ptr ) );
    const index_t& index = idx[ ptr ];
    T* p = std::lower_bound( index.first, index.second, id,
                             [](const T& lhs, unsigned rhs) {
                               return KeyPolicy::id( lhs ) < rhs;
                             } );
    if ( p != index.second && KeyPolicy::id( *p ) == id )
      return p;
    return nullptr;
  }
};

template <typename T, typename Filter, typename KeyPolicy = id_function_policy_t>
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

} // Namespace dbc ends

#if defined(SC_USE_PTR) && (SC_USE_PTR != 0)
# define SC_DBC_GET_DATA( _data__, _data_ptr__, _ptr_flag__ ) \
    ((_ptr_flag__) ? ::util::make_span( _data_ptr__ ).subspan( 0 ) : ::util::make_span( _data__ ).subspan( 0 ))
#else
# define SC_DBC_GET_DATA( _data__, _data_ptr__, _ptr_flag__ ) \
    ((void)(_ptr_flag__), ::util::make_span( _data__ ))
#endif

#endif /* CLIENT_DATA_HPP */

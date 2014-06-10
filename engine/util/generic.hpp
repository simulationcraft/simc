// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// Collection of generic programming code
// ==========================================================================

#ifndef SC_GENERIC_HPP
#define SC_GENERIC_HPP

#include "config.hpp"

#include <algorithm>
#include "utf8.h"
#include <cassert>

#include <cmath>

#if USE_TR1_NAMESPACE
// Use TR1
#include <tr1/array>
#include <tr1/type_traits>
namespace std {using namespace tr1; }
#else
// Use C++11
#include <array>
#include <type_traits>
#endif

// Type traits and metaprogramming tools ====================================

template <bool Condition, typename T>
struct enable_if { typedef T type; };
template <typename T>
struct enable_if<false, T> {};

template <typename T>
struct iterator_type
{ typedef typename T::iterator type; };

template <typename T>
struct iterator_type<const T>
{ typedef typename T::const_iterator type; };

// iterable enumeration templates ===========================================

/*
 * Enumeration types in C++ implicitly convert to int but not from int (e.g.,
 * "for ( attribute_e i = ATTR_STRENGTH; i < ATTR_MAX; ++i )" won't compile).
 * This is why so much of the code uses int when it really means an enum type.
 * Providing the kind of operations we want to use for enums lets us tighten up
 * our use of the type system and avoid accidentally passing some other thing
 * that converts to int when we really mean an enumeration type.
 *
 * The template functions tell the compiler it can perform prefix and postfix
 * operators ++ and -- on any type by converting it to int and back. The magic
 * with std::enable_if<> restricts those operations to types T for which the
 * type trait "is_iterable_enum<T>" is true. This trait gives us a way to
 * selectively control the functionality for a specific type T by specializing
 * is_iterable_enum<T> as std::true_type or std::false_type.
 */

// All enumerations are iterable by default.
template <typename T>
struct is_iterable_enum : public std::is_enum<T> {};

template <typename T>
inline typename enable_if<is_iterable_enum<T>::value, T&>::type
operator -- ( T& s )
{ return s = static_cast<T>( s - 1 ); }

template <typename T>
inline typename enable_if<is_iterable_enum<T>::value, T>::type
operator -- ( T& s, int )
{
  T tmp = s;
  --s;
  return tmp;
}

template <typename T>
inline typename enable_if<is_iterable_enum<T>::value, T&>::type
operator ++ ( T& s )
{ return s = static_cast<T>( s + 1 ); }

template <typename T>
inline typename enable_if<is_iterable_enum<T>::value, T>::type
operator ++ ( T& s, int )
{
  T tmp = s;
  ++s;
  return tmp;
}

// Generic programming tools ================================================

template <typename T, std::size_t N>
inline std::size_t sizeof_array( const T ( & )[N] )
{ return N; }

template <typename T, std::size_t N>
inline std::size_t sizeof_array( const std::array<T, N>& )
{ return N; }

#if defined(__GXX_EXPERIMENTAL_CXX0X__) && ( ( defined(SC_GCC) && SC_GCC >= 40600 ) || ( defined(SC_CLANG) && SC_CLANG >= 30000 ) ) // Improved compile-time diagnostics.
class noncopyable
{
public:
  noncopyable() = default;
  noncopyable( noncopyable&& ) = default;
  noncopyable& operator = ( noncopyable && ) = default;
  noncopyable( const noncopyable& ) = delete;
  noncopyable& operator = ( const noncopyable& ) = delete;
};
#else
class noncopyable
{
public:
  noncopyable() {}
private:
  noncopyable( const noncopyable& );
  noncopyable& operator = ( const noncopyable& );
};
#endif
class nonmoveable : public noncopyable
{
private:
  // nonmoveable( nonmoveable&& ) = delete;
  // nonmoveable& operator = ( nonmoveable&& ) = delete;
};

// Adapted from (read "stolen") boost::checked_deleter
struct delete_disposer_t
{
  template <typename T>
  void operator () ( T* t ) const
  {
    typedef int force_T_to_be_complete[ sizeof( T ) ? 1 : -1 ];
    ( void )sizeof( force_T_to_be_complete );
    delete t;
  }
};

// Generic algorithms =======================================================

// Wrappers for std::fill, std::fill_n, and std::find that perform any type
// conversions for t at the callsite instead of per assignment in the loop body.
// (i.e., fill( T**, T**, 0 ) will deduce T* for the third argument, as opposed
//  to std::fill( T**, T**, 0 ) simply defaulting to int and failing to compile).
template <typename I>
inline void fill( I first, I last, typename std::iterator_traits<I>::value_type const& t )
{ std::fill( first, last, t ); }

template <typename I>
inline void fill_n( I first, typename std::iterator_traits<I>::difference_type n,
                    typename std::iterator_traits<I>::value_type const& t )
{ std::fill_n( first, n, t ); }

template <typename I>
inline I find( I first, I last, typename std::iterator_traits<I>::value_type const& t )
{ return std::find( first, last, t ); }

template <typename I, typename D>
void dispose( I first, I last, D disposer )
{
  while ( first != last )
    disposer( *first++ );
}

template <typename I>
inline void dispose( I first, I last )
{ dispose( first, last, delete_disposer_t() ); }

// Machinery for range-based generic algorithms =============================

namespace range { // ========================================================
template <typename T>
struct traits
{
  typedef typename iterator_type<T>::type iterator;
  static iterator begin( T& t ) { return t.begin(); }
  static iterator end( T& t ) { return t.end(); }
};

template <typename T, size_t N>
struct traits<T[N]>
{
  typedef T* iterator;
  static iterator begin( T ( &t )[N] ) { return &t[0]; }
  static iterator end( T ( &t )[N] ) { return begin( t ) + N; }
};

template <typename T>
struct traits< std::pair<T, T> >
{
  typedef T iterator;
  static iterator begin( const std::pair<T, T>& t ) { return t.first; }
  static iterator end( const std::pair<T, T>& t ) { return t.second; }
};

template <typename T>
struct traits< const std::pair<T, T> >
{
  typedef T iterator;
  static iterator begin( const std::pair<T, T>& t ) { return t.first; }
  static iterator end( const std::pair<T, T>& t ) { return t.second; }
};

template <typename T>
struct value_type
{
  typedef typename std::iterator_traits<typename traits<T>::iterator>::value_type type;
};

template <typename T>
inline typename traits<T>::iterator begin( T& t )
{ return traits<T>::begin( t ); }

template <typename T>
inline typename traits<const T>::iterator cbegin( const T& t )
{ return range::begin( t ); }

template <typename T>
inline typename traits<T>::iterator end( T& t )
{ return traits<T>::end( t ); }

template <typename T>
inline typename traits<const T>::iterator cend( const T& t )
{ return range::end( t ); }

// Range-based generic algorithms ===========================================

template <typename Range, typename Out>
inline Out copy( const Range& r, Out o )
{ return std::copy( range::begin( r ), range::end( r ), o ); }

template <typename Range, typename D>
inline Range& dispose( Range& r, D disposer )
{ dispose( range::begin( r ), range::end( r ), disposer ); return r; }

template <typename Range>
inline Range& dispose( Range& r )
{ return dispose( r, delete_disposer_t() ); }

template <typename Range>
inline Range& fill( Range& r, typename range::value_type<Range>::type const& t )
{ std::fill( range::begin( r ), range::end( r ), t ); return r; }

#if defined(SC_GCC)
// Workaround for GCC 4.6+ optimization ( -O3 ) issue with filling C-arrays
template <typename T, size_t N>
inline T ( &fill( T ( &r )[N], const T& t ) )[N]
{
  for ( size_t i = 0; i < N; i++ )
    r[ i ] = t;
  return r;
}
#endif

template <typename Range>
inline typename range::traits<Range>::iterator
find( Range& r, typename range::value_type<Range>::type const& t )
{ return std::find( range::begin( r ), range::end( r ), t ); }

template <typename Range, typename F>
inline F for_each( Range& r, F f )
{ return std::for_each( range::begin( r ), range::end( r ), f ); }

template <typename Range, typename Out, typename Predicate>
inline Out remove_copy_if( Range& r, Out o, Predicate p )
{ return std::remove_copy_if( range::begin( r ), range::end( r ), o, p ); }

template <typename Range, typename Out, typename F>
inline Out transform( Range& r, Out o, F f )
{ return std::transform( range::begin( r ), range::end( r ), o, f ); }

template <typename Range, typename Range2, typename Out, typename F>
inline Out transform( Range& r, Range2& r2, Out o, F f )
{ return std::transform( range::begin( r ), range::end( r ), range::begin( r2 ), o, f ); }

template <typename Range, typename F>
inline typename range::traits<Range>::iterator transform_self( Range& r, F f )
{ return std::transform( range::begin( r ), range::end( r ), range::begin( r ), f ); }

template <typename Range1, typename Range2, typename Out>
inline Out set_difference( const Range1& left, const Range2& right, Out o )
{
  return std::set_difference( range::begin( left ), range::end( left ),
                              range::begin( right ), range::end( right ), o );
}

template <typename Range1, typename Range2, typename Out, typename Compare>
inline Out set_difference( const Range1& left, const Range2& right, Out o, Compare c )
{
  return std::set_difference( range::begin( left ), range::end( left ),
                              range::begin( right ), range::end( right ), o, c );
}

template <typename Range1, typename Range2, typename Out>
inline Out set_intersection( const Range1& left, const Range2& right, Out o )
{
  return std::set_intersection( range::begin( left ), range::end( left ),
                                range::begin( right ), range::end( right ), o );
}

template <typename Range1, typename Range2, typename Out, typename Compare>
inline Out set_intersection( const Range1& left, const Range2& right, Out o, Compare c )
{
  return std::set_intersection( range::begin( left ), range::end( left ),
                                range::begin( right ), range::end( right ), o, c );
}

template <typename Range1, typename Range2, typename Out>
inline Out set_union( const Range1& left, const Range2& right, Out o )
{
  return std::set_union( range::begin( left ), range::end( left ),
                         range::begin( right ), range::end( right ), o );
}

template <typename Range1, typename Range2, typename Out, typename Compare>
inline Out set_union( const Range1& left, const Range2& right, Out o, Compare c )
{
  return std::set_union( range::begin( left ), range::end( left ),
                         range::begin( right ), range::end( right ), o, c );
}

template <typename Range>
inline Range& sort( Range& r )
{ std::sort( range::begin( r ), range::end( r ) ); return r; }

template <typename Range, typename Comp>
inline Range& sort( Range& r, Comp c )
{ std::sort( range::begin( r ), range::end( r ), c ); return r; }

template <typename Range>
inline typename range::traits<Range>::iterator unique( Range& r )
{ return std::unique( range::begin( r ), range::end( r ) ); }

template <typename Range, typename Comp>
inline typename range::traits<Range>::iterator unique( Range& r, Comp c )
{ return std::unique( range::begin( r ), range::end( r ), c ); }

template <typename Range>
inline bool is_valid_utf8( const Range& r )
{ return utf8::is_valid( range::begin( r ), range::end( r ) ); }

template <typename Range>
inline typename range::traits<Range>::iterator max_element( Range& r )
{ return std::max_element( range::begin( r ), range::end( r ) ); }

template <typename Range>
inline typename range::traits<Range>::iterator min_element( Range& r )
{ return std::min_element( range::begin( r ), range::end( r ) ); }

} // namespace range ========================================================

// Adapter for container of owned pointers; automatically deletes the
// pointers on destruction.
template <typename Container>
class auto_dispose : public Container
{
private:
  void dispose_() { range::dispose( *this ); }
public:
  ~auto_dispose() { dispose_(); }
  void dispose()  { dispose_(); Container::clear(); }
};

// Fancy type-casting function to use when we "know" what type an object pointer
// really is. Makes sure we are right when debugging.
template <typename To, typename From>
inline To debug_cast( From* ptr )
{
#ifdef NDEBUG
  return static_cast<To>( ptr );
#else
  To result = dynamic_cast<To>( ptr );
  if ( ptr ) assert( result );
  return result;
#endif
}

// Fancy type-casting function to convert between types of different size and signedness.
// When debugging, verifies that the value is representable by both types.
template <typename To, typename From>
inline To as( From f )
{
  To t = static_cast<To>( f );
  // Casting between arithmetic types
  assert( std::is_arithmetic<To>::value );
  assert( std::is_arithmetic<From>::value );
  // is "safe" if (a) it's reversible, and
  assert( f == static_cast<From>( t ) );
  // (b) both values have the same sign.
  assert( ( From() < f ) == ( To() < t ) );
  return t;
}

template <typename T>
inline T clamp( T value, T low, T high )
{
  assert( ! ( high < low ) );
  return ( value < low ? low : ( high < value ? high : value ) );
}

// Remove in O(1) by copying the last element over the element to be
// erased and shrinking the sequence. DOES NOT PRESERVE ELEMENT ORDERING.
template <typename Sequence>
void erase_unordered( Sequence& s, typename Sequence::iterator pos )
{
  assert( ! s.empty() && pos != s.end() );
  typename Sequence::iterator last = s.end();
  --last;
  if ( pos != last )
    *pos = *last; // *pos = std::move( *last );
  s.pop_back();
}

#if 0
// Experimental very lightweight callback handler.
template<class... Args>
class callback_manager_t
{
public:
  callback_manager_t() = default;

  /* Triggers all registered callbacks
   */
  void trigger( Args... a )
  {
    for( auto&& callback : _callbacks ) {
      callback( a... );
    }
  }

  /* Register a callback
   */
  template<typename Fn>
  void register_callback( Fn&& f )
  {
    _callbacks.push_back( std::forward<Fn>(f) );
  }
private:
  std::vector<std::function<void(Args...)>> _callbacks;
};
#endif

#endif // SC_GENERIC_HPP

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/generic.hpp"

#include <functional>
#include <vector>

/* Encapsulated Vector
 * const read access
 * Modifying the vector triggers registered callbacks
 */
template <typename T>
struct vector_with_callback
{
public:
  using callback_type = std::function<void(const T&)>;
  using iterator = typename std::vector<T>::const_iterator;

  /* Register your custom callback, which will be called when the vector is modified
   */
  void register_callback( callback_type c )
  {
    if ( c )
      _callbacks.push_back( std::move( c ) );
  }

  iterator begin() const
  { return _data.begin(); }
  iterator end() const
  { return _data.end(); }

  void push_back( T x )
  {
    _data.push_back( std::move( x ) );
    trigger_callbacks( _data.back() );
  }

  void find_and_erase( const T& x )
  {
    find_and_erase_impl( x, [this](auto it) { _data.erase( it ); } );
  }

  void find_and_erase_unordered( const T& x )
  {
    find_and_erase_impl( x, [this](auto it) { ::erase_unordered( _data, it ); } );
  }

  const std::vector<T>& data() const
  { return _data; }

  const T& operator[]( size_t i ) const
  { return _data[ i ]; }

  size_t size() const
  { return _data.size(); }

  bool empty() const
  { return _data.empty(); }

  void reset_callbacks()
  { _callbacks.clear(); }

  void clear_without_callbacks()
  { _data.clear(); }

private:
  void trigger_callbacks( const T& v ) const
  {
    for ( auto&& callback : _callbacks )
      callback( v );
  }

  template <typename Erase>
  void find_and_erase_impl( const T& v, Erase erase )
  {
    auto it = range::find( _data, v );
    if ( it != _data.end() )
    {
      const T value = std::move( *it );
      erase( it );
      trigger_callbacks( value );
    }
  }

  std::vector<T> _data;
  std::vector<callback_type> _callbacks;
};

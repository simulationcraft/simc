// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <vector>

struct player_t;

/* Encapsulated Vector
 * const read access
 * Modifying the vector triggers registered callbacks
 */
template <typename T>
struct vector_with_callback
{
private:
  std::vector<T> _data;
  std::vector<std::function<void(T)> > _callbacks ;
public:
  /* Register your custom callback, which will be called when the vector is modified
   */
  void register_callback( std::function<void(T)> c )
  {
    if ( c )
      _callbacks.push_back( c );
  }

  typename std::vector<T>::iterator begin()
  { return _data.begin(); }
  typename std::vector<T>::const_iterator begin() const
  { return _data.begin(); }
  typename std::vector<T>::iterator end()
  { return _data.end(); }
  typename std::vector<T>::const_iterator end() const
  { return _data.end(); }

  typedef typename std::vector<T>::iterator iterator;

  void trigger_callbacks(T v) const
  {
    for ( size_t i = 0; i < _callbacks.size(); ++i )
      _callbacks[i](v);
  }

  void push_back( T x )
  { _data.push_back( x ); trigger_callbacks( x ); }

  void find_and_erase( T x )
  {
    typename std::vector<T>::iterator it = range::find( _data, x );
    if ( it != _data.end() )
      erase( it );
  }

  void find_and_erase_unordered( T x )
  {
    typename std::vector<T>::iterator it = range::find( _data, x );
    if ( it != _data.end() )
      erase_unordered( it );
  }

  // Warning: If you directly modify the vector, you need to trigger callbacks manually!
  std::vector<T>& data()
  { return _data; }

  const std::vector<T>& data() const
  { return _data; }

  player_t* operator[]( size_t i ) const
  { return _data[ i ]; }

  size_t size() const
  { return _data.size(); }

  bool empty() const
  { return _data.empty(); }

  void reset_callbacks()
  { _callbacks.clear(); }

private:
  void erase_unordered( typename std::vector<T>::iterator it )
  {
    T _v = *it;
    ::erase_unordered( _data, it );
    trigger_callbacks( _v );
  }

  void erase( typename std::vector<T>::iterator it )
  {
    T _v = *it;
    _data.erase( it );
    trigger_callbacks( _v );
  }
};

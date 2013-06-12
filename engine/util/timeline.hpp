// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#include <vector>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <numeric>
#include "sc_generic.hpp"

template <unsigned HW, typename Fwd, typename Out>
void sliding_window_average( Fwd first, Fwd last, Out out )
{
  typedef typename std::iterator_traits<Fwd>::value_type value_t;
  typedef typename std::iterator_traits<Fwd>::difference_type diff_t;
  diff_t n = std::distance( first, last );
  diff_t HALFWINDOW = static_cast<diff_t>( HW );

  if ( n >= 2 * HALFWINDOW )
  {
    value_t window_sum = value_t();

    // Fill right half of sliding window
    Fwd right = first;
    for ( diff_t count = 0; count < HALFWINDOW; ++count )
      window_sum += *right++;

    // Fill left half of sliding window
    for ( diff_t count = HALFWINDOW; count < 2 * HALFWINDOW; ++count )
    {
      window_sum += *right++;
      *out++ = window_sum / ( count + 1 );
    }

    // Slide until window hits end of data
    while ( right != last )
    {
      window_sum += *right++;
      *out++ = window_sum / ( 2 * HALFWINDOW + 1 );
      window_sum -= *first++;
    }

    // Empty right half of sliding window
    for ( diff_t count = 2 * HALFWINDOW; count > HALFWINDOW; --count )
    {
      *out++ = window_sum / count;
      window_sum -= *first++;
    }
  }
  else
  {
    // input is pathologically small compared to window size, just average everything.
    std::fill_n( out, n, std::accumulate( first, last, value_t() ) / n );
  }
}

template <unsigned HW, typename Range, typename Out>
inline Range& sliding_window_average( Range& r, Out out )
{
  sliding_window_average<HW>( range::begin( r ), range::end( r ), out );
  return r;
}

namespace tl {
// generic Timeline class
template <typename T>
class timeline_t
{
private:
  typedef T data_type;
  std::vector<T> _data;

public:
  // const access to the underlying vector data
  const std::vector<T>& data() const
  { return _data; }

  void init( size_t length )
  { _data.assign( length, data_type() ); }

  void resize( size_t length )
  { _data.resize( length ); }

  // Add 'value' at the specific index
  void add( size_t index, data_type value )
  {
    assert( index < _data.size() );
    _data[ index ] += value;
  }

  // Adjust timeline by dividing through divisor timeline
  template <class A>
  void adjust( const std::vector<A>& divisor_timeline )
  {
    assert( _data.size() >= divisor_timeline.size() );

    for ( size_t j = 0; j < divisor_timeline.size(); j++ )
    {
      _data[ j ] /= divisor_timeline[ j ];
    }
  }

  T average( size_t start, size_t length ) const
  {
    T average = T();
    if ( length == 0 )
      return average;
    assert( start + length <= data().size() );
    for ( size_t j = start; j < start + length; ++j )
    {
      average += data()[ j ];
    }
    return average / length;
  }
  T average() const
  {
    return average( 0, data().size() );
  }

  // Merge with other timeline
  void merge( const timeline_t<data_type>& other )
  {
    for ( size_t j = 0, num_buckets = std::min( _data.size(), other.data().size() ); j < num_buckets; ++j )
      _data[ j ] += other.data()[ j ];

    if ( _data.size() < other.data().size() )
      _data.insert( _data.end(), other.data().begin() + _data.size(), other.data().end() );
  }

  void build_derivative_timeline( timeline_t<data_type>& out ) const
  {
    out._data.reserve( data().size() );
    sliding_window_average<10>( data(), std::back_inserter( out._data ) );
  }

  // Maximum value; 0 if no data available
  data_type max() const
  { return data().empty() ? data_type() : *std::max_element( data().begin(), data().end() ); }

  // Minimum value; 0 if no data available
  data_type min() const
  { return data().empty() ? data_type() : *std::min_element( data().begin(), data().end() ); }

  /*
    // Functions which could be implemented:
    data_type variance() const;
    data_type mean() const;
    template <class A>
    static A covariance( const timeline_t<A>& first, const timeline_t<A>& second );
    data_type covariance( const timeline_t<data_type>& other ) const
    { return covariance( data(), other ); }
  */
};

} // end namespace tl

#endif // TIMELINE_HPP

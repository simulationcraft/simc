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
#include "sample_data.hpp"

template <typename Fwd, typename Out>
void sliding_window_average( Fwd first, Fwd last, unsigned half_window, Out out )
{
  typedef typename std::iterator_traits<Fwd>::value_type value_t;
  typedef typename std::iterator_traits<Fwd>::difference_type diff_t;
  diff_t n = std::distance( first, last );
  diff_t HALFWINDOW = static_cast<diff_t>( half_window );

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

template <typename Range, typename Out>
inline Range& sliding_window_average( Range& r, unsigned half_window, Out out )
{
  sliding_window_average( range::begin( r ), range::end( r ), half_window, out );
  return r;
}

// generic Timeline class
class timeline_t
{
private:
  std::vector<double> _data;

public:
  // const access to the underlying vector data
  const std::vector<double>& data() const
  { return _data; }

  void init( size_t length )
  { _data.assign( length, 0.0 ); }

  void resize( size_t length )
  { _data.resize( length ); }

  // Add 'value' at the specific index
  void add( size_t index, double value )
  {
    if ( index >= _data.capacity() ) // we need to reallocate
    {
      _data.reserve( std::max( size_t(10), _data.capacity() * 2 ) );
      _data.resize( index + 1 );
    }
    else if ( index >= _data.size() ) // we still have enough capacity left, but need to resize up to index
    {
      _data.resize( index + 1 );
    }
    _data.at(index) += value;
  }

  // Adjust timeline by dividing through divisor timeline
  template <class A>
  void adjust( const std::vector<A>& divisor_timeline )
  {

    for ( size_t j = 0, size = std::min( data().size(), divisor_timeline.size() ); j < size; j++ )
    {
      _data[ j ] /= divisor_timeline[ j ];
    }
  }

  double mean() const
  { return statistics::calculate_mean( data().begin(), data().end() ); }

  double mean_stddev() const
  { return statistics::calculate_mean_stddev( data().begin(), data().end() ); }

  // Merge with other timeline
  void merge( const timeline_t& other )
  {
    // merge shared range
    for ( size_t j = 0, num_buckets = std::min( _data.size(), other.data().size() ); j < num_buckets; ++j )
      _data[ j ] += other.data()[ j ];

    // if other is larger, insert tail
    if ( _data.size() < other.data().size() )
      _data.insert( _data.end(), other.data().begin() + _data.size(), other.data().end() );
  }

  void build_sliding_average_timeline( timeline_t& out, unsigned half_window ) const
  {
    out._data.reserve( data().size() );
    sliding_window_average( data(), half_window, std::back_inserter( out._data ) );
  }

  // Maximum value; 0 if no data available
  double max() const
  { return data().empty() ? 0.0 : *std::max_element( data().begin(), data().end() ); }

  // Minimum value; 0 if no data available
  double min() const
  { return data().empty() ? 0.0 : *std::min_element( data().begin(), data().end() ); }

  std::ostream& data_str( std::ostream& s ) const
  {
    s << "Timeline: length: " << data().size();
    //s << " mean: " << mean() << " variance: " << variance << " mean_std_dev: " << mean_std_dev << "\n";
    if ( ! data().empty() )
      s << "data: ";
    for ( size_t i = 0, size = data().size(); i < size; ++i )
    { if ( i > 0 ) s << ", "; s << data()[ i ]; }
    s << "\n";
    return s;
  }
  /*
    // Functions which could be implemented:
    data_type variance() const;
    template <class A>
    static A covariance( const timeline_t<A>& first, const timeline_t<A>& second );
    data_type covariance( const timeline_t<data_type>& other ) const
    { return covariance( data(), other ); }
  */
};

class histogram
{
  std::vector<size_t> _data;
  std::vector<double> _normalized_data;
  double _min,_max;

  static double nan()
  { return std::numeric_limits<double>::quiet_NaN(); }
public:
  histogram() :
    _data(), _normalized_data(), _min( nan() ), _max( nan() )
  {}
  double min() const
  { return _min; };
  double max() const
  { return _max; };

  std::vector<size_t>& data()
  { return _data; }
  std::vector<double>& normalized_data()
  { return _normalized_data; }
  double range() const
  { return max() - min(); };

  double bucket_size() const
  { return range() / _data.size(); }

  void clear()
  { _data.clear(); _normalized_data.clear(); _min = nan(); _max = nan();}

  /* Create Histogram from timeline, with given min/max
   */
  void create_histogram( const timeline_t& tl, size_t num_buckets, double min, double max )
  {
    if ( tl.data().empty() )
      return;
    clear();
    _min = min; _max = max;
    _data = statistics::create_histogram( tl.data().begin(), tl.data().end(), num_buckets, _min, _max );
    _normalized_data = statistics::normalize_histogram( _data );
  }

  /* Create Histogram from timeline, using min/max from tl data
   */
  void create_histogram( const timeline_t& tl, size_t num_buckets )
  {
    if ( tl.data().empty() )
      return;
    double min = *std::min_element( tl.data().begin(), tl.data().end() );
    double max = *std::max_element( tl.data().begin(), tl.data().end() );
    create_histogram( tl, num_buckets, min, max );
  }

  /* Create Histogram from extended sample data, with given min/max
   */
  void create_histogram( const extended_sample_data_t& sd, size_t num_buckets, double min, double max )
  {
    if ( sd.simple || sd.data().empty() )
      return;
    clear();
    _min = min; _max = max;
    _data = statistics::create_histogram( sd.data().begin(), sd.data().end(), num_buckets, _min, _max );
    _normalized_data = statistics::normalize_histogram( _data );
  }

  /* Create Histogram from extended sample data, using min/max from sd data
   */
  void create_histogram( const extended_sample_data_t& sd, size_t num_buckets )
  {
    if ( sd.simple || sd.data().empty() )
      return;
    double min = *std::min_element( sd.data().begin(), sd.data().end() );
    double max = *std::max_element( sd.data().begin(), sd.data().end() );
    create_histogram( sd, num_buckets, min, max );
  }
};
#endif // TIMELINE_HPP

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

#include "generic.hpp"
#include "sample_data.hpp"
#include "sc_timespan.hpp"

struct sim_t;

template <typename Fwd, typename Out>
void sliding_window_average( Fwd first, Fwd last, unsigned window, Out out )
{
  // This function performs an apodized moving average of the data bounded by first and last
  // using a window size defined by the third argument.
  typedef typename std::iterator_traits<Fwd>::value_type value_t;
  typedef typename std::iterator_traits<Fwd>::difference_type diff_t;
  diff_t n = std::distance( first, last );
  diff_t HALFWINDOW = static_cast<diff_t>( window / 2 );
  diff_t WINDOW = static_cast<diff_t>( window );

  // if the array is longer than the window size, perform apodized moving average
  if ( n >= WINDOW )
  {
    value_t window_sum = value_t();

    // Fill right half of sliding window
    Fwd right = first;
    for ( diff_t count = 0; count < HALFWINDOW; ++count )
      window_sum += *right++;

    // Fill left half of sliding window and start storing output values
    for ( diff_t count = HALFWINDOW; count < WINDOW; ++count )
    {
      window_sum += *right++;
      *out++ = window_sum / WINDOW ;
    }

    // Slide until window hits end of data
    while ( right != last )
    {
      window_sum += *right++;
      window_sum -= *first++;
      *out++ = window_sum / WINDOW;
    }

    // Empty right half of sliding window
    for ( diff_t count = 2 * HALFWINDOW; count > HALFWINDOW; --count )
    {
      window_sum -= *first++;
      *out++ = window_sum / WINDOW;
    }
  }
  else
  {
    // input is pathologically small compared to window size, just average everything.
    std::fill_n( out, n, std::accumulate( first, last, value_t() ) / n );
  }
}

template <typename Range, typename Out>
inline Range& sliding_window_average( Range& r, unsigned window, Out out )
{
  sliding_window_average( range::begin( r ), range::end( r ), window, out );
  return r;
}

// generic Timeline class
class timeline_t
{
private:
  std::vector<double> _data;

public:
  timeline_t() : _data() {}

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
      // Reserve data less aggressively than doubling the size every time
      _data.reserve( std::max( size_t( 10 ), static_cast<size_t>( index * 1.25 ) ) );
      _data.resize( index + 1 );
    }
    else if ( index >= _data.size() ) // we still have enough capacity left, but need to resize up to index
    {
      _data.resize( index + 1 );
    }
    _data[ index ] += value;
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
  { 
    if ( data().size() == 0 )
      return 0;

    return statistics::calculate_mean( data() );
  }

  double mean_stddev() const
  { 
    if ( data().size() == 0 )
      return 0;

    return statistics::calculate_mean_stddev( data() );
  }

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

  void build_sliding_average_timeline( timeline_t& out, unsigned window ) const
  {
    out._data.reserve( data().size() );
    sliding_window_average( data(), window, std::back_inserter( out._data ) );
  }

  // Maximum value; 0 if no data available
  double max() const
  { return data().empty() ? 0.0 : *std::max_element( data().begin(), data().end() ); }

  // Minimum value; 0 if no data available
  double min() const
  { return data().empty() ? 0.0 : *std::min_element( data().begin(), data().end() ); }

  void clear()
  { _data.clear(); }

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
  double _min, _max;
  size_t _num_entries; // total value of all _data entries

  static double nan()
  { return std::numeric_limits<double>::quiet_NaN(); }

  /* Calculates the total amount of entries of the internal histogram data
   */
  void calculate_num_entries()
  { _num_entries = std::accumulate( _data.begin(), _data.end(), size_t() ); }
public:
  histogram() :
    _data(), _normalized_data(), _min( nan() ), _max( nan() ), _num_entries()
  {}
  double min() const
  { return _min; }
  double max() const
  { return _max; }

  const std::vector<size_t>& data() const
  { return _data; }
  const std::vector<double>& normalized_data() const
  { return _normalized_data; }
  double range() const
  { return max() - min(); }

  double bucket_size() const
  { return range() / _data.size(); }

  void clear()
  { _data.clear(); _normalized_data.clear(); _min = nan(); _max = nan(); _num_entries = 0; }

  /* Create Histogram from timeline, with given min/max
   */
  void create_histogram( const timeline_t& tl, size_t num_buckets, double min, double max )
  {
    if ( tl.data().empty() )
      return;
    clear();
    _min = min; _max = max;
    _data = statistics::create_histogram( tl.data(), num_buckets, _min, _max );
    calculate_num_entries();
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
    _data = statistics::create_histogram( sd.data(), num_buckets, _min, _max );
    calculate_num_entries();
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

  /* Add a other histogram to this one.
   * Does nothing if histogram parameters ( min, max, num_buckets ) don't match.
   */
  void accumulate( const histogram& other )
  {
    if ( min() != other.min() || max() != other.max() || data().size() != other.data().size() )
      return;

    for ( size_t j = 0, num_buckets = _data.size(); j < num_buckets; ++j )
      _data[ j ] += other.data()[ j ];

    _num_entries += other.num_entries();
  }

  /* Calculates approximation of the percentile of the original distribution
   * The quality of the value depends on how much information was lost when creating the histogram
   * Inside a bucket, we use linear interpolation
   */
  double percentile( double p )
  {
    assert( p >= 0.0 && p <= 1.0 && "p must be within [0.0 1.0]" );
    if ( !num_entries() )
      return 0.0;

    size_t target = static_cast<size_t>( p * num_entries() );

    // Performance Optimization: We assume a roughly balanced distribution,
    // so for p <= 0.5 we start from min counting upwards, otherwise from max counting downwards
    if ( p <= 0.5 )
    {
      size_t count = 0;
      for ( size_t i = 0, size = data().size(); i < size; ++i )
      {
        count += data()[ i ];
        if ( count >= target )
        {
          // We reached the target bucket.

          // Calculate linear interpolation x
          double x = data()[ i ] ? ( count - target ) / data()[ i ] : 0.0;
          assert( x >= 0.0 && x <= 1.0 );

          // Return result
          return _min + ( i + x ) * bucket_size();
        }
      }
    }
    else
    {
      size_t count = num_entries();
      for ( int i = static_cast< int >( data().size() ) - 1; i >= 0; --i )
      {
        count -= data()[ i ];
        if ( count <= target )
        {
          // We reached the target bucket.

          // Calculate linear interpolation x
          double x = data()[ i ] ? ( target - count ) / data()[ i ] : 0.0;
          assert( x >= 0.0 && x <= 1.0 );

          // Return result
          return _max - ( i - x ) * bucket_size();
        }
      }
    }

    assert( false ); return 0.0;
  }

  /* Creates normalized histogram data from internal unnormalized histogram data
   */
  void create_normalized_data()
  {
    _normalized_data = statistics::normalize_histogram( _data );
  }

  /* Returns the total amount of entries of the internal histogram data
   */
  size_t num_entries() const
  { return _num_entries; }
};


/* SimulationCraft timeline:
 * - data_type is double
 * - timespan_t add helper function
 */
struct sc_timeline_t : public timeline_t
{
  typedef timeline_t base_t;
  using timeline_t::add;
  double bin_size;

  sc_timeline_t() : timeline_t(), bin_size( 1.0 ) {}

  // methods to modify/retrieve the bin size
  void set_bin_size( double bin )
  {
    bin_size = bin;
  }
  double get_bin_size() const
  {
    return bin_size;
  }

  // Add 'value' at the corresponding time
  void add( timespan_t current_time, double value )
  { base_t::add( static_cast<size_t>( current_time.total_millis() / 1000 / bin_size ), value ); }

  // Add 'value' at corresponding time, replacing existing entry if new value is larger
  void add_max( timespan_t current_time, double new_value )
  {
    size_t index = static_cast<size_t>( current_time.total_millis() / 1000 / bin_size );
    if ( data().size() == 0 || data().size() <= index )
      add( current_time, new_value );
    else if ( new_value > data().at( index ) )
    {
      add( current_time, new_value - data().at( index ) );
    }
  }

  void adjust( sim_t& sim );
  void adjust( const extended_sample_data_t& adjustor );

  void build_derivative_timeline( sc_timeline_t& out ) const
  { base_t::build_sliding_average_timeline( out, 20 ); }

private:
  static std::vector<double> build_divisor_timeline( const extended_sample_data_t& simulation_length, double bin_size );
};

#endif // TIMELINE_HPP

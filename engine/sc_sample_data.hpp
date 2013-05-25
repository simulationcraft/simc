// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_SAMPLE_DATA_HPP
#define SC_SAMPLE_DATA_HPP

#include <vector>
#include <numeric>
#include "sc_generic.hpp"

// Statistical Sample Data Container

class sample_data_t
{
public:
  std::string name_str;

  // Analyzed Results
  double sum;
  double min;
  double max;

  double mean;
  double variance;
  double std_dev;
  double mean_std_dev;
  std::vector<int> distribution;
  bool simple;
  bool min_max;
private:
  std::vector<double> _data;
  std::vector<double*> _sorted_data; // extra sequence so we can keep the original, unsorted order ( for example to do regression on it )
  unsigned int count;

  bool analyzed_basics;
  bool analyzed_variance;
  bool created_dist;
  bool is_sorted;

public:
  sample_data_t( bool s = true, bool mm = false ) :
    sum( 0 ),
    min( d_max() ),
    max( d_min() ),
    mean( 0 ),
    variance( 0 ),
    std_dev( 0 ),
    mean_std_dev( 0 ),
    simple( s ), min_max( mm ), count( 0 ),
    analyzed_basics( false ), analyzed_variance( false ),
    created_dist( false ), is_sorted( false )
  {}

  sample_data_t( const std::string& n, bool s = true, bool mm = false ) :
    name_str( n ),
    sum( 0 ),
    min( d_max() ),
    max( d_min() ),
    mean( 0 ),
    variance( 0 ),
    std_dev( 0 ),
    mean_std_dev( 0 ),
    simple( s ), min_max( mm ), count( 0 ),
    analyzed_basics( false ), analyzed_variance( false ),
    created_dist( false ), is_sorted( false )
  {}

  void change_mode( bool simple )
  {
    this -> simple = simple;

    clear();
  }

  const char* name() const { return name_str.c_str(); }

  // Reserve memory
  void reserve( std::size_t capacity )
  { if ( ! simple ) _data.reserve( capacity ); }

  // Add a sample
  void add( double x )
  {
    if ( simple )
    {
      if ( min_max )
      {
        if ( x < min )
          min = x;
        if ( x > max )
          max = x;
      }

      sum += x;
      ++count;
    }
    else
    {
      _data.push_back( x );
    }

    analyzed_basics = analyzed_variance = created_dist = is_sorted = false;
  }

  bool basics_analyzed() const { return analyzed_basics; }
  bool variance_analyzed() const { return analyzed_variance; }
  bool distribution_created() const { return created_dist; }
  bool sorted() const { return is_sorted; }
  unsigned int size() const { if ( simple ) return count; return ( unsigned int ) _data.size(); }

  // Analyze collected data
  void analyze( bool calc_basics = true,
                bool calc_variance = true,
                bool s = true,
                bool create_dist = true )
  {
    // Sort Data
    if ( s )
      sort();

    if ( calc_basics )
      analyze_basics();

    // Calculate Variance
    if ( calc_variance )
      analyze_variance();


    if ( create_dist )
      create_distribution();
  }

  /*
   *  Analyze Basics:
   *  Simple: Mean
   *  !Simple: Mean, min/max
   */
  void analyze_basics()
  {
    if ( analyzed_basics )
      return;
    analyzed_basics = true;

    if ( simple )
    {
      if ( count > 0 )
        mean = sum / count;
      return;
    }

    size_t sample_size = data().size();
    if ( sample_size == 0 )
      return;

    if ( sorted() )
    {
      min = *_sorted_data.front();
      max = *_sorted_data.back();
      sum = std::accumulate( data().begin(), data().end(), 0.0 );
    }
    else
    {
      // Calculate Sum, Mean, Min, Max
      sum = min = max = data()[ 0 ];

      for ( size_t i = 1; i < sample_size; i++ )
      {
        double i_data = data()[ i ];
        sum  += i_data;
        if ( i_data < min ) min = i_data;
        if ( i_data > max ) max = i_data;
      }
    }

    mean = sum / sample_size;
  }

  /*
   *  Analyze Variance: Variance, Stddev and Stddev of the mean
   */
  void analyze_variance()
  {
    // Generic programming trick: Bring std::sqrt into scope with a
    // using declaration, so that the unqualified call to
    // sqrt( variance ) below will resolve to std::sqrt for primitive
    // types and an associated sqrt( some_user_defined_type ) for
    // user-defined types.

    if ( analyzed_variance )
      return;
    analyzed_variance = true;

    if ( simple )
      return;

    analyze_basics();

    size_t sample_size = data().size();

    if ( sample_size == 0 )
      return;

    variance = 0;
    for ( size_t i = 0; i < sample_size; i++ )
    {
      double delta = data()[ i ] - mean;
      variance += delta * delta;
    }

    if ( sample_size > 1 )
    {
      variance /= ( sample_size - 1 );
    }

    std_dev = sqrt( variance );

    // Calculate Standard Deviation of the Mean ( Central Limit Theorem )
    if ( sample_size > 1 )
    {
      mean_std_dev = sqrt( variance / sample_size );
    }
  }
  struct sorter
  {
    bool operator()( const double* a, const double* b ) const
    {
      return *a < *b;
    }
  };

  // sort data
  void sort()
  {
    if ( ! is_sorted )
    {
      _sorted_data.resize( _data.size() );
      for ( size_t i = 0; i < _data.size(); ++i )
        _sorted_data[ i ] =  &( _data[ i ] );
      range::sort( _sorted_data, sorter() );
      is_sorted = true;
    }
  }

  /*
   *  Create distribution of the data
   */
  void create_distribution( unsigned int num_buckets = 50 )
  {
    if ( created_dist )
      return;
    created_dist = true;

    if ( simple )
      return;

    if ( !basics_analyzed() )
      return;

    if ( data().empty() )
      return;

    if ( max > min )
    {
      double range = max - min + 2;

      distribution.assign( num_buckets, 0 );
      for ( size_t i = 0; i < data().size(); i++ )
      {
        size_t index = static_cast<size_t>( num_buckets * ( data()[ i ] - min + 1 ) / range );
        assert( index < distribution.size() );
        distribution[ index ]++;
      }
    }
  }

  void clear()
  { count = 0; sum = 0; _sorted_data.clear(); _data.clear(); distribution.clear();  }

  // Access functions

  // calculate percentile
  double percentile( double x ) const
  {
    assert( x >= 0 && x <= 1.0 );

    if ( simple )
      return 0;

    if ( data().empty() )
      return 0;

    if ( !is_sorted )
      return 0;

    // Should be improved to use linear interpolation
    return *( sorted_data()[ ( int ) ( x * ( sorted_data().size() - 1 ) ) ] );
  }

  const std::vector<double>& data() const { return _data; }
  const std::vector<double*>& sorted_data() const { return _sorted_data; }

  void merge( const sample_data_t& other )
  {
    assert( simple == other.simple );

    if ( simple )
    {
      count += other.count;
      sum += other.sum;

      if ( min_max )
      {
        if ( other.min < min ) min = other.min;
        if ( other.max > max ) max = other.max;
      }
    }
    else
      _data.insert( _data.end(), other._data.begin(), other._data.end() );
  }

private:
  // Some numeric_limits helper functions
  static double d_min() { return -std::numeric_limits<double>::infinity(); }
  static double d_max() { return  std::numeric_limits<double>::infinity(); }

}; // sample_data_t

inline double covariance( const sample_data_t& x, const sample_data_t& y )
{
  if ( x.simple || y.simple )
    return 0;

  if ( x.data().size() != y.data().size() )
    return 0;

  if ( ! x.basics_analyzed() || ! y.basics_analyzed() )
    return 0;

  if ( ! x.variance_analyzed() || ! y.variance_analyzed() )
    return 0;

  double corr = 0;

  for ( size_t i = 0; i < x.data().size(); i++ )
  {
    corr += ( x.data()[ i ] - x.mean ) * ( y.data()[ i ] - y.mean );
  }
  if ( x.data().size() > 1 )
    corr /= ( x.data().size() - 1 );

  corr /= x.std_dev;
  corr /= y.std_dev;

  return corr;
}


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

#include "sc_timespan.hpp" // remove dependency

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

  // Add 'value' at the corresponding time
  void add( timespan_t current_time, data_type value )
  { add( static_cast<size_t>( current_time.total_millis() / 1000 ), value ); }

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

  T average( size_t start, size_t length )
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
    average( 0, data().size() );
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
  { return data().empty() ? data_type() : *range::max_element( data() ); }

  // Minimum value; 0 if no data available
  data_type min() const
  { return data().empty() ? data_type() : *range::min_element( data() ); }

  /*
    // Functions which could be implemented:
    data_type variance() const;
    data_type mean() const;
    template <class A>
    static A covariance( const timeline_t<A>& first, const timeline_t<A>& second );
    data_type covariance( const timeline_t<data_type>& other ) const
    { return covariance( data(), other ); }
  */
}; // timeline_t
#endif

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SAMPLE_DATA_HPP
#define SAMPLE_DATA_HPP

#include <vector>
#include <numeric>
#include <limits>
#include "../sc_generic.hpp"

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
  size_t count;
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
    is_sorted( false )
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
    is_sorted( false )
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
      is_sorted = false;
    }
  }

  bool sorted() const
  { return is_sorted; }

  size_t size() const
  {
    if ( simple )
      return count;

    return _data.size();
  }

  // Analyze collected data
  void analyze_all()
  {
    // Sort Data
    sort();

    analyze_basics();

    // Calculate Variance
    analyze_variance();

    create_distribution();
  }

  /*
   *  Analyze Basics:
   *  Simple: Mean
   *  !Simple: Mean, min/max
   */
  void analyze_basics()
  {
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

  /* Analyze Variance: Variance, Stddev and Stddev of the mean
   *
   * Requires: Analyzed Mean
   */
  void analyze_variance()
  {
    if ( simple )
      return;

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

    std_dev = std::sqrt( variance );

    // Calculate Standard Deviation of the Mean ( Central Limit Theorem )
    if ( sample_size > 1 )
    {
      mean_std_dev = std::sqrt( variance / sample_size );
    }
  }

private:
  struct sorter
  {
    bool operator()( const double* a, const double* b ) const
    {
      return *a < *b;
    }
  };
public:
  // sort data
  void sort()
  {
    if ( ! is_sorted && !simple )
    {
      _sorted_data.resize( _data.size() );
      for ( size_t i = 0; i < _data.size(); ++i )
        _sorted_data[ i ] =  &( _data[ i ] );
      range::sort( _sorted_data, sorter() );
      is_sorted = true;
    }
  }

  /* Create distribution of the data
   *
   * Requires: Min, Max analyzed
   */
  void create_distribution( unsigned int num_buckets = 50 )
  {
    if ( simple )
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
  double percentile( double x )
  {
    assert( x >= 0 && x <= 1.0 );

    if ( simple )
      return 0;

    if ( data().empty() )
      return 0;

    if ( !is_sorted )
      sort();

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

/* Requires: Analyzed mean & stddev
 */
inline double covariance( const sample_data_t& x, const sample_data_t& y )
{
  if ( x.simple || y.simple )
    return 0;

  if ( x.data().size() != y.data().size() )
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


#endif // SAMPLE_DATA_HPP

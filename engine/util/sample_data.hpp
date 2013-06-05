// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SAMPLE_DATA_HPP
#define SAMPLE_DATA_HPP

#define SAMPLE_DATA_NO_NAN

#include <vector>
#include <numeric>
#include <limits>
#include <sstream>
#include "../sc_generic.hpp"

/* Simplest Samplest Data container. Only tracks sum and count
 *
 */
class simple_sample_data_t
{
protected:
  double _sum;
  size_t _count;
  static double nan() {
#if defined (SAMPLE_DATA_NO_NAN)
    return 0.0;
#else
    return std::numeric_limits<double>::quiet_NaN();
#endif
  }

public:
  simple_sample_data_t() : _sum( 0.0 ), _count( 0 ) {}

  void add( double x )
  { _sum += x; ++_count; }

  double mean() const
  { return _count ? _sum / _count : nan(); }

  double pretty_mean() const
  { return _count ? _sum / _count : 0.0; }

  double sum() const
  { return _sum; }

  size_t count() const
  { return _count; }

  void merge( const simple_sample_data_t& other )
  {
    _count += other._count;
    _sum  += other._sum;
  }
};

/* Second simplest Samplest Data container. Tracks sum, count as well as min/max
 *
 */
class simple_sample_data_with_min_max_t : public simple_sample_data_t
{
private:
  typedef simple_sample_data_t base_t;

protected:
  bool _found;
  double _min, _max;
  void set_min( double x ) { _min = x; _found = true; }
  void set_max( double x ) { _max = x; _found = true; }

public:
  simple_sample_data_with_min_max_t() :
    base_t(), _found( false ),
    _min( std::numeric_limits<double>::infinity() ), _max( -std::numeric_limits<double>::infinity() ) {}

  void add( double x )
  {
    base_t::add( x );

    if ( x < _min )
      set_min( x );
    if ( x > _max )
      set_max( x );
  }

  bool found_min_max() const
  { return _found; }
  double min() const
  { return _found ? _min : base_t::nan(); }
  double max() const
  { return _found ? _max : base_t::nan(); }

  void merge( const simple_sample_data_with_min_max_t& other )
  {
    base_t::merge( other );

    if ( other.found_min_max() )
    {
      if ( other._min < _min )
      {
        set_min( other._min );
      }
      if ( other._max > _max )
      {
        set_max( other._max );
      }
    }
  }
};

/* Extensive sample_data container with two runtime dependent modes:
 * - simple: Only offers sum, count
 *  -!simple: saves data and offers variance, percentiles, distribution, etc.
 */
class extended_sample_data_t : public simple_sample_data_with_min_max_t
{
  typedef simple_sample_data_with_min_max_t base_t;
public:
  std::string name_str;

  double _mean;
  double variance;
  double std_dev;
  double mean_std_dev;
  std::vector<size_t> distribution;
  bool simple;
private:
  std::vector<double> _data;
  std::vector<double*> _sorted_data; // extra sequence so we can keep the original, unsorted order ( for example to do regression on it )
  bool is_sorted;
public:
  extended_sample_data_t( const std::string& n, bool s = true ) :
    base_t(),
    name_str( n ),
    _mean( 0 ),
    variance( 0 ),
    std_dev( 0 ),
    mean_std_dev( 0 ),
    simple( s ),
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
  void add( const double& x )
  {
    if ( simple )
    {
      base_t::add( x );
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
      return base_t::count();

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
      return;

    size_t sample_size = data().size();
    if ( sample_size == 0 )
      return;

    if ( sorted() )
    {
      base_t::set_min( *_sorted_data.front() );
      base_t::set_max( *_sorted_data.back() );
      base_t::_sum = std::accumulate( data().begin(), data().end(), 0.0 );
    }
    else
    {
      // Calculate Sum, Mean, Min, Max
      double min, max;
      base_t::_sum = min = max = data()[ 0 ];

      for ( size_t i = 1; i < sample_size; i++ )
      {
        base_t::_sum  += data()[ i ];
        if ( data()[ i ] < min ) min = data()[ i ];
        if ( data()[ i ] > max ) max = data()[ i ];
      }
    }

    _mean = base_t::_sum / sample_size;
  }

  double mean() const
  { return simple ? base_t::mean() : _mean; }
  double pretty_mean() const
  { return simple ? base_t::pretty_mean() : _mean; }
  size_t count() const
  { return simple ? base_t::count() : data().size(); }

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
      double delta = data()[ i ] - mean();
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

    if ( base_t::max() > base_t::min() )
    {
      double range = base_t::max() - base_t::min() + 2;

      distribution.assign( num_buckets, 0 );
      for ( size_t i = 0; i < data().size(); i++ )
      {
        size_t index = static_cast<size_t>( num_buckets * ( data()[ i ] - base_t::min() + 1 ) / range );
        assert( index < distribution.size() );
        distribution[ index ]++;
      }
    }
  }

  void clear()
  { base_t::_count = 0; base_t::_sum = 0.0; _sorted_data.clear(); _data.clear(); distribution.clear();  }

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
      return base_t::nan();

    // Should be improved to use linear interpolation
    return *( sorted_data()[ ( int ) ( x * ( sorted_data().size() - 1 ) ) ] );
  }

  const std::vector<double>& data() const { return _data; }
  const std::vector<double*>& sorted_data() const { return _sorted_data; }

  void merge( const extended_sample_data_t& other )
  {
    assert( simple == other.simple );

    if ( simple )
    {
      base_t::merge( other );
    }
    else
      _data.insert( _data.end(), other._data.begin(), other._data.end() );
  }

  std::ostringstream& data_str( std::ostringstream& s )
  {
    s << "Sample_Data: Name: \"" << name_str << "\" count: " << count();
    s << " mean: " << mean() << " variance: " << variance << " mean_std_dev: " << mean_std_dev << "\n";
    for ( size_t i = 0, size = data().size(); i < size; ++i )
    { if ( i > 0 ) s << " "; s << data()[ i ]; }
    s << "\n";
    return s;
  }

}; // sample_data_t

/* Requires: Analyzed mean & stddev
 */
inline double covariance( const extended_sample_data_t& x, const extended_sample_data_t& y )
{
  if ( x.simple || y.simple )
    return 0;

  if ( x.data().size() != y.data().size() )
    return 0;

  double corr = 0;

  for ( size_t i = 0; i < x.data().size(); i++ )
  {
    corr += ( x.data()[ i ] - x.mean() ) * ( y.data()[ i ] - y.mean() );
  }
  if ( x.data().size() > 1 )
    corr /= ( x.data().size() - 1 );

  corr /= x.std_dev;
  corr /= y.std_dev;

  return corr;
}



#endif // SAMPLE_DATA_HPP

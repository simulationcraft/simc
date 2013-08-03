// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SAMPLE_DATA_HPP
#define SAMPLE_DATA_HPP

#include <vector>
#include <numeric>
#include <limits>
#include <sstream>
#include "../sc_generic.hpp"

/* Collection of statistical formulas for sequences
 * Note: Returns 0 for empty sequences
 */
namespace statistics {

/* Arithmetic Sum
 */
template <typename iterator>
typename std::iterator_traits<iterator>::value_type calculate_sum( iterator begin, iterator end )
{
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  return std::accumulate( begin, end, value_t() );
}

/* Arithmetic Mean
 */
template <typename iterator>
typename std::iterator_traits<iterator>::value_type calculate_mean( iterator begin, iterator end )
{
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  typedef typename std::iterator_traits<iterator>::difference_type diff_t;
  diff_t length = end - begin;
  value_t tmp = calculate_sum( begin, end );
  if ( length > 1 )
    tmp /= length;
  return tmp;
}

/* Expected Value of the squared deviation from a given mean
 */
template <typename iterator>
typename std::iterator_traits<iterator>::value_type calculate_variance( iterator begin, iterator end, typename std::iterator_traits<iterator>::value_type mean )
{
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  typedef typename std::iterator_traits<iterator>::difference_type diff_t;
  diff_t length = end - begin;
  value_t tmp = value_t();

  for( ; begin != end; ++begin )
  {
    tmp += ( *begin - mean ) * ( *begin - mean );
  }
  if ( length > 1 )
    tmp /= length;
  return tmp;
}

/* Expected Value of the squared deviation
 */
template <typename iterator>
typename std::iterator_traits<iterator>::value_type calculate_variance( iterator begin, iterator end )
{
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  value_t mean = calculate_mean( begin, end );
  return calculate_variance( begin, end, mean );
}

/* Standard Deviation from a given mean
 */
template <typename iterator>
typename std::iterator_traits<iterator>::value_type calculate_stddev( iterator begin, iterator end, typename std::iterator_traits<iterator>::value_type mean )
{
  return std::sqrt( calculate_variance( begin, end, mean ) );
}

/* Standard Deviation
 */
template <typename iterator>
typename std::iterator_traits<iterator>::value_type calculate_stddev( iterator begin, iterator end )
{
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  value_t mean = calculate_mean( begin, end );
  return std::sqrt( calculate_variance( begin, end, mean ) );
}

/* Standard Deviation of a given sample mean distribution, according to Central Limit Theorem
 */
template <typename iterator>
typename std::iterator_traits<iterator>::value_type calculate_mean_stddev( iterator begin, iterator end, typename std::iterator_traits<iterator>::value_type mean )
{
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  typedef typename std::iterator_traits<iterator>::difference_type diff_t;
  diff_t length = end - begin;
  value_t tmp = calculate_variance( begin, end, mean );
  if ( length > 1 )
    tmp /= length;
  return std::sqrt( tmp );
}

/* Standard Deviation of a given sample mean distribution, according to Central Limit Theorem
 */
template <typename iterator>
typename std::iterator_traits<iterator>::value_type calculate_mean_stddev( iterator begin, iterator end )
{
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  value_t mean = calculate_mean( begin, end );
  return calculate_mean_stddev( begin, end, mean );
}

/* Pearson product-moment correlation coefficient
 */
template <typename iterator1, typename iterator2>
typename std::iterator_traits<iterator1>::value_type calculate_correlation_coefficient( iterator1 first_begin, iterator1 first_end, typename std::iterator_traits<iterator1>::value_type mean1, typename std::iterator_traits<iterator1>::value_type std_dev1, iterator2 second_begin, iterator2 second_end, typename std::iterator_traits<iterator2>::value_type mean2, typename std::iterator_traits<iterator2>::value_type std_dev2 )
{
  typedef typename std::iterator_traits<iterator1>::value_type value1_t;
  typedef typename std::iterator_traits<iterator1>::difference_type diff1_t;
  typedef typename std::iterator_traits<iterator1>::difference_type diff2_t;
  static_assert( ( std::is_same<value1_t, typename std::iterator_traits<iterator2>::value_type>::value ), "Value type of iterators not equal" );

  value1_t result = value1_t();

  diff1_t length1 = first_end - first_begin;
  diff2_t length2 = second_end - second_begin;

  if ( length1 != length2 )
    return result;

  for( ; first_begin != first_end && second_begin != second_end; )
  {
    result += ( *first_begin++ - mean1 ) * ( *second_begin++ - mean2 );
  }

  if ( length1 > 2 )
    result /= ( length1 - 1 );

  result /= std_dev1;
  result /= std_dev2;

  return result;
}

/* Pearson product-moment correlation coefficient
 */
template <typename iterator1, typename iterator2>
typename std::iterator_traits<iterator1>::value_type calculate_correlation_coefficient( iterator1 first_begin, iterator1 first_end, iterator2 second_begin, iterator2 second_end )
{
  typedef typename std::iterator_traits<iterator1>::value_type value1_t;
  typedef typename std::iterator_traits<iterator2>::value_type value2_t;
  typedef typename std::iterator_traits<iterator1>::difference_type diff1_t;
  typedef typename std::iterator_traits<iterator1>::difference_type diff2_t;
  static_assert( ( std::is_same<value1_t, value2_t>::value ), "Value type of iterators not equal" );

  value1_t result = value1_t();

  diff1_t length1 = first_end - first_begin;
  diff2_t length2 = second_end - second_begin;

  if ( length1 != length2 )
    return result;

  value1_t mean1 = calculate_mean( first_begin, first_end );
  value1_t mean2 = calculate_mean( second_begin, second_end );

  value1_t std_dev1 = calculate_stddev( first_begin, first_end );
  value2_t std_dev2 = calculate_stddev( second_begin, second_end );

  result = calculate_correlation_coefficient( first_begin, first_end, mean1, std_dev1, second_begin, second_end, mean2, std_dev2 );
  return result;
}

template <typename iterator>
std::vector<size_t> create_histogram( iterator begin, iterator end, size_t num_buckets, typename std::iterator_traits<iterator>::value_type min, typename std::iterator_traits<iterator>::value_type max )
{
  std::vector<size_t> result;
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  if ( begin == end )
    return result;

  assert( min <= *std::min_element( begin, end ) );
  assert( max >= *std::max_element( begin, end ) );


  value_t range = max - min;
  if ( range <= value_t() )
    return result;

  result.assign( num_buckets, size_t() );
  for ( ; begin != end; ++begin )
  {
    value_t position = ( *begin - min ) / range;
    size_t index = static_cast<size_t>( num_buckets * position );
    if ( index == num_buckets ) // if *begin == max, we want to downgrade it into the last bucket
      --index;
    assert( index < num_buckets );
    result[ index ]++;
  }

  return result;
}

template <typename iterator>
std::vector<size_t> create_histogram( iterator begin, iterator end, size_t num_buckets )
{
  typedef typename std::iterator_traits<iterator>::value_type value_t;
  if ( begin == end )
    return std::vector<size_t>();

  value_t min = *std::min_element( begin, end );
  value_t max = *std::max_element( begin, end );

  return create_histogram( begin, end, num_buckets, min, max );
}

/* Normalizes a histogram.
 * sum over all elements of the histogram will be equal to 1.0
 */
inline std::vector<double> normalize_histogram( const std::vector<size_t>& in, size_t total_num_entries )
{
  std::vector<double> result;

  if ( in.empty() )
    return result;

  assert( total_num_entries == std::accumulate( in.begin(), in.end(), size_t() ) );
  double adjust = 1.0 / total_num_entries;

  for ( size_t i = 0, size = in.size(); i < size; ++i )
    result.push_back( in[ i ] * adjust );

  return result;
}
/* Normalizes a histogram.
 * sum over all elements of the histogram will be equal to 1.0
 */
inline std::vector<double> normalize_histogram( const std::vector<size_t>& in )
{
  std::vector<double> result;

  if ( in.empty() )
    return result;

  size_t count = std::accumulate( in.begin(), in.end(), size_t() );

  return normalize_histogram( in, count );
}

template <typename iterator>
std::vector<double> create_normalized_histogram( iterator begin, iterator end, size_t num_buckets, typename std::iterator_traits<iterator>::value_type min, typename std::iterator_traits<iterator>::value_type max )
{
  return normalize_histogram( create_histogram( begin, end, num_buckets, min, max ) );
}

template <typename iterator>
std::vector<double> create_normalized_histogram( iterator begin, iterator end, size_t num_buckets )
{
  return normalize_histogram( create_histogram( begin, end, num_buckets ) );
}

} // end sd namespace


/* Simplest Samplest Data container. Only tracks sum and count
 *
 */
class simple_sample_data_t
{
public:
  typedef double value_t;
protected:
  static const bool SAMPLE_DATA_NO_NAN = true;
  value_t _sum;
  size_t _count;

  static value_t nan()
  { return SAMPLE_DATA_NO_NAN ? value_t() : std::numeric_limits<value_t>::quiet_NaN(); }

public:
  simple_sample_data_t() : _sum(), _count() {}

  void add( double x )
  { _sum += x; ++_count; }

  value_t mean() const
  { return _count ? _sum / _count : nan(); }

  value_t pretty_mean() const
  { return _count ? _sum / _count : value_t(); }

  value_t sum() const
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
  value_t _min, _max;
  void set_min( double x ) { _min = x; _found = true; }
  void set_max( double x ) { _max = x; _found = true; }

public:
  simple_sample_data_with_min_max_t() :
    base_t(), _found( false ),
    _min( std::numeric_limits<value_t>::infinity() ), _max( -std::numeric_limits<value_t>::infinity() ) {}

  void add( value_t x )
  {
    base_t::add( x );

    if ( x < _min )
      set_min( x );
    if ( x > _max )
      set_max( x );
  }

  bool found_min_max() const
  { return _found; }
  value_t min() const
  { return _found ? _min : base_t::nan(); }
  value_t max() const
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

  value_t _mean, variance, std_dev, mean_std_dev;
  std::vector<size_t> distribution;
  bool simple;
private:
  std::vector<value_t> _data;
  std::vector<value_t*> _sorted_data; // extra sequence so we can keep the original, unsorted order ( for example to do regression on it )
  bool is_sorted;
public:
  extended_sample_data_t( const std::string& n, bool s = true ) :
    base_t(),
    name_str( n ),
    _mean(),
    variance(),
    std_dev(),
    mean_std_dev(),
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
  void add( value_t x )
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

    create_histogram();
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
    { // If we have sorted data, we can just take the front/back as min/max
      base_t::set_min( *_sorted_data.front() );
      base_t::set_max( *_sorted_data.back() );
      base_t::_sum = statistics::calculate_sum( data().begin(), data().end() );
    }
    else
    {
      base_t::set_min( *std::min_element( data().begin(), data().end() ) );
      base_t::set_max( *std::max_element( data().begin(), data().end() ) );
      base_t::_sum = statistics::calculate_sum( data().begin(), data().end() );
    }

    _mean = base_t::_sum / sample_size;
  }

  value_t mean() const
  { return simple ? base_t::mean() : _mean; }
  value_t pretty_mean() const
  { return simple ? base_t::pretty_mean() : _mean; }
  size_t count() const
  { return simple ? base_t::count() : data().size(); }

  /* Analyze Variance: Variance, Stddev and Stddev of the mean
   * Requires: Analyzed Mean
   */
  void analyze_variance()
  {
    if ( simple )
      return;

    size_t sample_size = data().size();

    if ( sample_size == 0 )
      return;

    variance = statistics::calculate_variance( data().begin(), data().end(), mean() );
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
    bool operator()( const value_t* a, const value_t* b ) const
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

  /* Create histogram ( not normalized ) of the data
   *
   * Requires: Min, Max analyzed
   */
  void create_histogram( unsigned int num_buckets = 50 )
  {
    if ( simple )
      return;

    if ( data().empty() )
      return;

    distribution = statistics::create_histogram( data().begin(), data().end(), num_buckets, base_t::min(), base_t::max() );
  }

  void clear()
  { base_t::_count = 0; base_t::_sum = 0.0; _sorted_data.clear(); _data.clear(); distribution.clear();  }

  // Access functions

  // calculate percentile
  value_t percentile( double x ) const
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

  const std::vector<value_t>& data() const { return _data; }
  const std::vector<value_t*>& sorted_data() const { return _sorted_data; }

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

  std::ostream& data_str( std::ostream& s ) const
  {
    s << "Sample_Data \"" << name_str << "\": count: " << count();
    s << " mean: " << mean() << " variance: " << variance << " mean_std_dev: " << mean_std_dev << "\n";

    if ( ! data().empty() )
      s << "data: ";
    for ( size_t i = 0, size = data().size(); i < size; ++i )
    { if ( i > 0 ) s << ", "; s << data()[ i ]; }
    s << "\n";
    return s;
  }

}; // sample_data_t

/* Requires: Analyzed mean & stddev
 */
inline extended_sample_data_t::value_t covariance( const extended_sample_data_t& x, const extended_sample_data_t& y )
{
  if ( x.simple || y.simple )
    return 0;

  return statistics::calculate_correlation_coefficient( x.data().begin(), x.data().end(), x.mean(), x.std_dev, y.data().begin(), y.data().end(), y.mean(), y.std_dev );
}



#endif // SAMPLE_DATA_HPP

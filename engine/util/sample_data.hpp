// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SAMPLE_DATA_HPP
#define SAMPLE_DATA_HPP

#include <limits>
#include <numeric>
#include <iosfwd>
#include <vector>

#include "util/generic.hpp"
#include "util/string_view.hpp"

/* Collection of statistical formulas for sequences
 * Note: Returns 0 for empty sequences
 */
namespace statistics
{
/* Arithmetic Sum
 */
template <typename Range>
range::value_type_t<Range> calculate_sum( const Range& r )
{
  using value_t = range::value_type_t<Range>;
  return std::accumulate( range::begin( r ), range::end( r ), value_t{} );
}

/* Arithmetic Mean
 */
template <typename Range>
range::value_type_t<Range> calculate_mean( const Range& r )
{
  auto length = range::size( r );
  auto tmp    = calculate_sum( r );
  if ( length > 1 )
    tmp /= length;
  return tmp;
}

/* Expected Value of the squared deviation from a given mean
 */
template <typename Range>
range::value_type_t<Range> calculate_variance( const Range& r,
                                               range::value_type_t<Range> mean )
{
  range::value_type_t<Range> tmp{};

  for ( auto value : r )
  {
    tmp += ( value - mean ) * ( value - mean );
  }
  auto length = range::size( r );
  if ( length > 1 )
    tmp /= length;
  return tmp;
}

/* Expected Value of the squared deviation
 */
template <typename Range>
range::value_type_t<Range> calculate_variance( const Range& r )
{
  return calculate_variance( r, calculate_mean( r ) );
}

/* Standard Deviation from a given mean
 */
template <typename Range>
range::value_type_t<Range> calculate_stddev( const Range& r,
                                             range::value_type_t<Range> mean )
{
  return std::sqrt( calculate_variance( r, mean ) );
}

/* Standard Deviation
 */
template <typename Range>
range::value_type_t<Range> calculate_stddev( const Range& r )
{
  return std::sqrt( calculate_variance( r, calculate_mean( r ) ) );
}

/* Standard Deviation of a given sample mean distribution, according to Central
 * Limit Theorem
 */
template <typename Range>
range::value_type_t<Range> calculate_mean_stddev(
    const Range& r, range::value_type_t<Range> mean )
{
  auto tmp    = calculate_variance( r, mean );
  auto length = range::size( r );
  if ( length > 1 )
    tmp /= length;
  return std::sqrt( tmp );
}

/* Standard Deviation of a given sample mean distribution, according to Central
 * Limit Theorem
 */
template <typename Range>
range::value_type_t<Range> calculate_mean_stddev( const Range& r )
{
  return calculate_mean_stddev( r, calculate_mean( r ) );
}

template <typename Range>
std::vector<size_t> create_histogram( const Range& r, size_t num_buckets,
                                      range::value_type_t<Range> min,
                                      range::value_type_t<Range> max )
{
  std::vector<size_t> result;
  if ( range::size( r ) == 0 )
    return result;

  if ( std::isnan(min) || std::isnan(max) )
    return result;

  assert( min <= *range::min_element( r ) );
  assert( max >= *range::max_element( r ) );

  if ( max <= min )
    return result;

  auto range = max - min;

  result.assign( num_buckets, size_t{} );
  for ( auto value : r )
  {
    auto position = ( value - min ) / range;
    size_t index  = static_cast<size_t>( num_buckets * position );
    if ( index == num_buckets )  // if *begin == max, we want to downgrade it
                                 // into the last bucket
      --index;
    assert( index < num_buckets );
    result[ index ]++;
  }

  return result;
}

template <typename Range>
std::vector<size_t> create_histogram( const Range& r, size_t num_buckets )
{
  if ( range::size( r ) == 0 )
    return std::vector<size_t>();

  auto mm  = std::minmax_element( range::begin( r ), range::end( r ) );
  auto min = *mm.first;
  auto max = *mm.second;

  return create_histogram( r, num_buckets, min, max );
}

/* Normalizes a histogram.
 * sum over all elements of the histogram will be equal to 1.0
 */
inline std::vector<double> normalize_histogram( const std::vector<size_t>& in,
                                                size_t total_num_entries )
{
  std::vector<double> result;

  if ( in.empty() )
    return result;

  assert( total_num_entries ==
          std::accumulate( in.begin(), in.end(), size_t() ) );
  double adjust = 1.0 / total_num_entries;

  result.reserve( in.size() );
  for ( auto& elem : in )
    result.push_back( elem * adjust );

  return result;
}
/* Normalizes a histogram.
 * sum over all elements of the histogram will be equal to 1.0
 */
inline std::vector<double> normalize_histogram( const std::vector<size_t>& in )
{
  if ( in.empty() )
    return std::vector<double>();

  size_t count = std::accumulate( in.begin(), in.end(), size_t() );

  return normalize_histogram( in, count );
}

}  // end sd namespace

/* Simplest Samplest Data container. Only tracks sum and count
 *
 */
class simple_sample_data_t
{
public:
  using value_t = double;

protected:
  static const bool SAMPLE_DATA_NO_NAN = true;
  value_t _sum                         = 0.0;
  size_t _count                        = 0;

  static value_t nan()
  {
    return SAMPLE_DATA_NO_NAN ? value_t()
                              : std::numeric_limits<value_t>::quiet_NaN();
  }

public:
  void add( double x )
  {
    _sum += x;
    ++_count;
  }

  value_t mean() const
  {
    return _count ? _sum / _count : nan();
  }

  value_t pretty_mean() const
  {
    return _count ? _sum / _count : value_t();
  }

  value_t sum() const
  {
    return _sum;
  }

  size_t count() const
  {
    return _count;
  }

  void merge( const simple_sample_data_t& other )
  {
    _count += other._count;
    _sum += other._sum;
  }

  void reset()
  {
    _count = 0u;
    _sum   = 0.0;
  }
};

/* Second simplest Samplest Data container. Tracks sum, count as well as min/max
 *
 */
class simple_sample_data_with_min_max_t : public simple_sample_data_t
{
private:
  using base_t = simple_sample_data_t;

protected:
  value_t _min = std::numeric_limits<value_t>::max();
  value_t _max = std::numeric_limits<value_t>::lowest();

  void set_min( double x )
  {
    _min = x < _min ? x : _min;
  }
  void set_max( double x )
  {
    _max = x > _max ? x : _max;
  }

public:
  void add( value_t x )
  {
    base_t::add( x );

    set_min( x );
    set_max( x );
  }

  value_t min() const
  {
    return _min <= _max ? _min : base_t::nan();
  }
  value_t max() const
  {
    return _min <= _max ? _max : base_t::nan();
  }

  void merge( const simple_sample_data_with_min_max_t& other )
  {
    base_t::merge( other );

    set_min( other._min );
    set_max( other._max );
  }

  void reset()
  {
    base_t::reset();
    _min = std::numeric_limits<value_t>::max();
    _max = std::numeric_limits<value_t>::lowest();
  }
};

/* Extensive sample_data container with two runtime dependent modes:
 * - simple: Only offers sum, count
 *  -!simple: saves data and offers variance, percentiles, distribution, etc.
 */
class extended_sample_data_t : public simple_sample_data_with_min_max_t
{
  using base_t = simple_sample_data_with_min_max_t;

public:
  std::string name_str;

  value_t _mean, variance, std_dev, mean_variance, mean_std_dev;
  std::vector<size_t> distribution;
  bool simple;

private:
  std::vector<value_t> _data;
  std::vector<value_t> _sorted_data;  // extra sequence so we can keep the
                                      // original, unsorted order ( for example
                                      // to do regression on it )
  bool is_sorted;

public:
  explicit extended_sample_data_t( util::string_view n, bool s = true )
    : base_t(),
      name_str( n ),
      _mean(),
      variance(),
      std_dev(),
      mean_variance(),
      mean_std_dev(),
      simple( s ),
      is_sorted( false )
  {
  }

  void change_mode( bool simple )
  {
    this->simple = simple;

    clear();
  }

  const std::string& name() const
  {
    return name_str;
  }

  // Reserve memory
  void reserve( std::size_t capacity )
  {
    if ( !simple )
      _data.reserve( capacity );
  }

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
  {
    return is_sorted;
  }

  size_t size() const
  {
    if ( simple )
      return base_t::count();

    return _data.size();
  }

  // Analyze collected data
  void analyze()
  {
    sort();
    analyze_basics();
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

    if ( data().empty() )
      return;

    if ( sorted() )
    {  // If we have sorted data, we can just take the front/back as min/max
      base_t::set_min( _sorted_data.front() );
      base_t::set_max( _sorted_data.back() );
    }
    else
    {
      auto minmax = std::minmax_element( data().begin(), data().end() );
      base_t::set_min( *minmax.first );
      base_t::set_max( *minmax.second );
    }

    base_t::_sum = statistics::calculate_sum( data() );
    _mean        = base_t::_sum / data().size();
  }

  value_t mean() const
  {
    return simple ? base_t::mean() : _mean;
  }
  value_t pretty_mean() const
  {
    return simple ? base_t::pretty_mean() : _mean;
  }
  size_t count() const
  {
    return simple ? base_t::count() : data().size();
  }

  /* Analyze Variance: Variance, Stddev and Stddev of the mean
   * Requires: Analyzed Mean
   */
  void analyze_variance()
  {
    if ( simple )
      return;

    if ( _data.empty() )
      return;

    variance = statistics::calculate_variance( data(), mean() );
    std_dev  = std::sqrt( variance );

    // Calculate Standard Deviation of the Mean ( Central Limit Theorem )
    if ( data().size() > 1 )
    {
      mean_variance = variance / data().size();
      mean_std_dev  = std::sqrt( mean_variance );
    }
  }

public:
  // sort data
  void sort()
  {
    if ( is_sorted )
    {
      return;
    }
    if ( simple )
    {
      return;
    }
    _sorted_data = _data;
    range::sort( _sorted_data );
    is_sorted = true;
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

    distribution = statistics::create_histogram( data(), num_buckets,
                                                 base_t::min(), base_t::max() );
  }

  void clear()
  {
    base_t::reset();
    _sorted_data.clear();
    _data.clear();
    distribution.clear();
  }

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
    return ( sorted_data()[ (int)( x * ( sorted_data().size() - 1 ) ) ] );
  }

  const std::vector<value_t>& data() const
  {
    return _data;
  }
  const std::vector<value_t>& sorted_data() const
  {
    return _sorted_data;
  }

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

  std::ostream& data_str( std::ostream& s ) const;

};  // sample_data_t

#endif  // SAMPLE_DATA_HPP

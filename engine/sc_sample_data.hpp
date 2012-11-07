// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_SAMPLE_DATA_HPP
#define SC_SAMPLE_DATA_HPP

// Statistical Sample Data

template <class Data, class Result>
class sample_data_base
{
  typedef Data data_type;
  typedef Result result_type;

public:
  const std::string name_str;

  // Analyzed Results
  data_type sum;
  data_type min;
  data_type max;


  result_type mean;
  result_type variance;
  result_type std_dev;
  result_type mean_std_dev;
  std::vector<int> distribution;
  bool simple;
  bool min_max;
private:
  std::vector<data_type> _data;
  unsigned int count;

  bool analyzed_basics;
  bool analyzed_variance;
  bool created_dist;
  bool is_sorted;
public:
  // Statistical Sample Data Container
  sample_data_base( bool s = true, bool mm = false ) :
    name_str( std::string() ),
    sum( s ? 0 : std::numeric_limits<data_type>::quiet_NaN() ),
    min( std::numeric_limits<data_type>::infinity() ),
    max( -std::numeric_limits<data_type>::infinity() ),
    mean( std::numeric_limits<result_type>::quiet_NaN() ),
    variance( std::numeric_limits<result_type>::quiet_NaN() ),
    std_dev( std::numeric_limits<result_type>::quiet_NaN() ),
    mean_std_dev( std::numeric_limits<result_type>::quiet_NaN() ),
    simple( s ), min_max( mm ), count( 0 ),
    analyzed_basics( false ),analyzed_variance( false ),
    created_dist( false ), is_sorted( false )
  {
  }

  // Statistical Sample Data Container
  sample_data_base( const std::string& n, bool s = true, bool mm = false ) :
    name_str( n ),
    sum( s ? 0 : std::numeric_limits<data_type>::quiet_NaN() ),
    min( std::numeric_limits<data_type>::infinity() ),
    max( -std::numeric_limits<data_type>::infinity() ),
    mean( std::numeric_limits<result_type>::quiet_NaN() ),
    variance( std::numeric_limits<result_type>::quiet_NaN() ),
    std_dev( std::numeric_limits<result_type>::quiet_NaN() ),
    mean_std_dev( std::numeric_limits<result_type>::quiet_NaN() ),
    simple( s ), min_max( mm ), count( 0 ),
    analyzed_basics( false ),analyzed_variance( false ),
    created_dist( false ), is_sorted( false )
  {
  }

  const char* name() const { return name_str.c_str(); }

  void reserve( std::size_t capacity )
  { if ( ! simple ) _data.reserve( capacity ); }

  // Add sample data
  void add( data_type x = 0 )
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
      _data.push_back( x );

    analyzed_basics = analyzed_variance = created_dist = is_sorted = false;
  }

  bool basics_analyzed() const { return analyzed_basics; }
  bool variance_analyzed() const { return analyzed_variance; }
  bool distribution_created() const { return created_dist; }
  bool sorted() const { return is_sorted; }
  int size() const { if ( simple ) return count; return ( int ) _data.size(); }

  // Analyze collected data
  void analyze(
    bool calc_basics = true,
    bool calc_variance = true,
    bool s = true,
    bool create_dist = true )
  {
    if ( calc_basics )
      analyze_basics();

    // Calculate Variance
    if ( calc_variance )
      analyze_variance();

    // Sort Data
    if ( s )
      sort();

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

    // Calculate Sum, Mean, Min, Max
    sum = min = max = data()[ 0 ];

    for ( size_t i = 1; i < sample_size; i++ )
    {
      double i_data = data()[ i ];
      sum  += i_data;
      if ( i_data < min ) min = i_data;
      if ( i_data > max ) max = i_data;
    }

    mean = sum / sample_size;
  }

  /*
   *  Analyze Variance: Variance, Stddev and Stddev of the mean
   */
  void analyze_variance()
  {
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
    mean_std_dev = std_dev / sqrt ( static_cast<double>( sample_size ) );
  }


  // sort data
  void sort()
  {
    if ( ! is_sorted )
    {
      range::sort( _data );
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

    if ( data().size() == 0 )
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
  { count = 0; sum = 0; _data.clear(); distribution.clear(); }

  // Access functions

  // calculate percentile
  double percentile( double x ) const
  {
    assert( x >= 0 && x <= 1.0 );

    if ( simple || ! is_sorted )
      return std::numeric_limits<double>::quiet_NaN();

    if ( data().empty() )
      return std::numeric_limits<double>::quiet_NaN();

    // Should be improved to use linear interpolation
    return data()[ ( int ) ( x * ( data().size() - 1 ) ) ];
  }

  const std::vector<data_type>& data() const { return _data; }

  void merge( const sample_data_base& other )
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

  double pearson_correlation( const sample_data_base& x, const sample_data_base& y )
  {
    if ( x.simple || y.simple )
      return std::numeric_limits<double>::quiet_NaN();

    if ( x.data().size() != y.data().size() )
      return std::numeric_limits<double>::quiet_NaN();

    if ( ! x.basics_analyzed() || ! y.basics_analyzed() )
      return std::numeric_limits<double>::quiet_NaN();

    if ( ! x.variance_analyzed() || ! y.variance_analyzed() )
      return std::numeric_limits<double>::quiet_NaN();

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

};

typedef sample_data_base<double,double> sample_data_t;

#endif

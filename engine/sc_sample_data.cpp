// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// sample_data_t::sample_data_t =============================================

sample_data_t::sample_data_t( const bool s, const bool mm ):
  sum( s ? 0 : std::numeric_limits<double>::quiet_NaN() ),mean( std::numeric_limits<double>::quiet_NaN() ),
  min( std::numeric_limits<double>::infinity() ),max( -std::numeric_limits<double>::infinity() ),
  variance( std::numeric_limits<double>::quiet_NaN() ),std_dev( std::numeric_limits<double>::quiet_NaN() ),
  mean_std_dev( std::numeric_limits<double>::quiet_NaN() ),
  simple( s ), min_max( mm ), count( 0 ),
  analyzed_basics( false ),analyzed_variance( false ),
  created_dist( false ), is_sorted( false )
{
}

// sample_data_t::analyze ===================================================

void sample_data_t::add( double x )
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
    data.push_back( x );
}


// sample_data_t::analyze ===================================================

void sample_data_t::analyze(
  bool calc_basics,
  bool calc_variance,
  bool s,
  unsigned int create_dist )
{
  if ( calc_basics )
    analyze_basics();

  // Calculate Variance
  if ( calc_variance )
    analyze_variance();

  // Sort Data
  if ( s )
    sort();

  if ( create_dist > 0 )
    create_distribution( create_dist );

}

// sample_data_t::analyze_basics ===============================================

void sample_data_t::analyze_basics()
{
  if ( analyzed_basics )
    return;
  analyzed_basics = true;

  if ( simple )
  {
    mean = sum / count;
    return;
  }

  size_t sample_size = data.size();
  if ( sample_size == 0 )
    return;

  // Calculate Sum, Mean, Min, Max
  sum = min = max = data[ 0 ];

  for ( size_t i=1; i < sample_size; i++ )
  {
    double i_data = data[ i ];
    sum  += i_data;
    if ( i_data < min ) min = i_data;
    if ( i_data > max ) max = i_data;
  }

  mean = sum / sample_size;
}

// sample_data_t::analyze_variance ===============================================

void sample_data_t::analyze_variance()
{
  if ( analyzed_variance )
    return;
  analyzed_variance = true;

  if ( simple )
    return;

  analyze_basics();

  size_t sample_size = data.size();

  if ( sample_size == 0 )
    return;

  variance = 0;
  for ( size_t i=0; i < sample_size; i++ )
  {
    double delta = data[ i ] - mean;
    variance += delta * delta;
  }

  if ( sample_size > 1 )
  {
    variance /= ( sample_size - 1 );
  }

  std_dev = sqrt( variance );

  // Calculate Standard Deviation of the Mean ( Central Limit Theorem )
  mean_std_dev = std_dev / sqrt ( ( double ) sample_size );
}

// sample_data_t::create_dist ===============================================

void sample_data_t::create_distribution( unsigned int num_buckets )
{
  if ( created_dist )
    return;
  created_dist = true;

  if ( simple )
    return;

  if ( !basics_analyzed() )
    return;

  if ( data.size() == 0 )
    return;

  if ( max > min )
  {
    double range = max - min + 2;

    distribution.assign( num_buckets, 0 );
    for ( unsigned int i=0; i < data.size(); i++ )
    {
      int index = ( int ) ( num_buckets * ( data[ i ] - min + 1 ) / range );
      assert( 0 <= index && static_cast<size_t>( index ) < distribution.size() );
      distribution[ index ]++;
    }
  }
}

// sample_data_t::percentile ================================================

double sample_data_t::percentile( double x )
{
  assert( x >= 0 && x <= 1.0 );

  if ( simple )
    return std::numeric_limits<double>::quiet_NaN();

  if ( data.empty() )
    return std::numeric_limits<double>::quiet_NaN();

  sort();

  // Should be improved to use linear interpolation
  return data[ ( int ) ( x * ( data.size() - 1 ) ) ];
}

// sample_data_t::sort ======================================================

void sample_data_t::sort()
{
  if ( ! is_sorted )
  {
    range::sort( data );
    is_sorted = true;
  }
}

// sample_data_t::merge =====================================================

void sample_data_t::merge( const sample_data_t& other )
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
    data.insert( data.end(), other.data.begin(), other.data.end() );
}

// sample_data_t::merge =====================================================


double sample_data_t::pearson_correlation( const sample_data_t& x, const sample_data_t& y )
{
  if ( x.simple || y.simple )
    return std::numeric_limits<double>::quiet_NaN();

  if ( x.data.size() != y.data.size() )
    return std::numeric_limits<double>::quiet_NaN();

  if ( ! x.basics_analyzed() || ! y.basics_analyzed() )
    return std::numeric_limits<double>::quiet_NaN();

  if ( ! x.variance_analyzed() || ! y.variance_analyzed() )
    return std::numeric_limits<double>::quiet_NaN();

  double corr = 0;

  for ( size_t i=0; i < x.data.size(); i++ )
  {
    corr += ( x.data[ i ] - x.mean ) * ( y.data[ i ] - y.mean );
  }
  if ( x.data.size() > 1 )
    corr /= ( x.data.size() - 1 );

  corr /= x.std_dev;
  corr /= y.std_dev;

  return corr;
}

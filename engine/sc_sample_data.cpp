// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// sample_data_t::sample_data_t =============================================

sample_data_t::sample_data_t( const bool s, const bool mm ):
  tmp( 0 ),
  sum( s ? 0 : std::numeric_limits<double>::quiet_NaN() ),mean( std::numeric_limits<double>::quiet_NaN() ),
  min( std::numeric_limits<double>::infinity() ),max( -std::numeric_limits<double>::infinity() ),
  variance( std::numeric_limits<double>::quiet_NaN() ),std_dev( std::numeric_limits<double>::quiet_NaN() ),
  median( std::numeric_limits<double>::quiet_NaN() ), mean_std_dev( std::numeric_limits<double>::quiet_NaN() ),
  simple( s ), min_max( mm ), count( 0 ),
  analyzed( false ),sorted( false )
{
}

// sample_data_t::analyze =============================================

void sample_data_t::add( double x )
{
  count++;

  x += tmp;

  tmp = 0;

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
  }
  else
    data.push_back( x );
}


// sample_data_t::analyze =============================================

void sample_data_t::analyze(
  bool calc_basics,
  bool calc_variance,
  bool s,
  unsigned int create_dist )
{
  if ( analyzed )
    return;
  analyzed = true;


  if ( simple )
  {
    mean = sum / count;
    return;
  }
  else
    assert( ( std::size_t ) count == data.size() );


  size_t sample_size = data.size();

  if ( sample_size == 0 )
    return;

  // Calculate Sum, Mean, Min, Max
  if ( calc_basics || calc_variance || create_dist || create_dist > 0 )
  {
    sum = 0;
    min = std::numeric_limits<double>::max();
    max = 0;

    for ( size_t i=0; i < sample_size; i++ )
    {
      double i_data = data[ i ];
      sum  += i_data;
      if ( i_data < min ) min = i_data;
      if ( i_data > max ) max = i_data;
    }

    mean = sum / sample_size;

    if ( min == std::numeric_limits<double>::max() )
      min = std::numeric_limits<double>::quiet_NaN();
  }

  // Calculate Variance
  if ( calc_variance )
  {
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


  // Sort Data
  if ( s )
  {
    sort_data();

    if ( sample_size % 2 == 1 )
      median = data[ ( sample_size - 1 )/ 2];
    else
      median = ( data[ sample_size / 2 - 1] + data[sample_size / 2] ) / 2.0;
  }

  if ( create_dist > 0 )
    create_distribution( create_dist );

}

// sample_data_t::create_dist =============================================

void sample_data_t::create_distribution( unsigned int num_buckets )
{
  if ( simple )
    return;

  assert( analyzed );

  if ( data.size() == 0 )
    return;

  if ( max > min )
  {
    double range = max - min + 2;

    distribution.assign( num_buckets, 0 );
    for ( unsigned int i=0; i < data.size(); i++ )
    {
      int index = ( int ) ( num_buckets * ( data[ i ] - min + 1 ) / range );
      distribution[ index ]++;
    }
  }
}

// sample_data_t::percentile =============================================

double sample_data_t::percentile( double x )
{
  assert( x >= 0 && x <= 1.0 );

  if ( simple )
    return std::numeric_limits<double>::quiet_NaN();

  size_t sample_size = data.size();

  if ( sample_size == 0 )
    return std::numeric_limits<double>::quiet_NaN();

  sort_data();

  // Should be improved to use linear interpolation
  return data[ ( int ) ( x * ( sample_size - 1 ) ) ];
}

// sample_data_t::sort_data =============================================

void sample_data_t::sort_data()
{
  if ( ! sorted )
  {
    range::sort( data );
    sorted = true;
  }
}

// sample_data_t::merge =============================================

void sample_data_t::merge( const sample_data_t& other )
{
  count += other.count;

  // assert( simple == other.simple );
  if ( simple )
  {
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

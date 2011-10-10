// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// sample_data_t::sample_data_t =============================================

sample_data_t::sample_data_t( bool s ):
  sum( s ? 0 : std::numeric_limits<double>::quiet_NaN() ),mean( std::numeric_limits<double>::quiet_NaN() ),
  min( std::numeric_limits<double>::quiet_NaN() ),max( std::numeric_limits<double>::quiet_NaN() ),
  variance( std::numeric_limits<double>::quiet_NaN() ),std_dev( std::numeric_limits<double>::quiet_NaN() ),
  median( std::numeric_limits<double>::quiet_NaN() ),
  simple( s ), count( 0 ),
  analyzed(),sorted()
{
  if ( simple )
    assign( 1, 0 );
}

// sample_data_t::analyze =============================================

void sample_data_t::add( double x )
{
  count++;

  if ( simple )
  {
    std::vector<double>::back() = x; // Save current value so that it can still get used.
    sum += x;
  }
  else
    std::vector<double>::push_back( x );
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
    assert( ( std::size_t ) count == size() );


  size_t sample_size = size();

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
      double i_data = ( *this )[ i ];
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
      double delta = ( *this )[ i ] - mean;
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
      median = ( *this )[ ( sample_size - 1 )/ 2];
    else
      median = ( ( *this )[ sample_size / 2 - 1] + ( *this )[sample_size / 2] ) / 2.0;
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

  if ( size() == 0 )
    return;

  if ( max > min )
  {
    double range = max - min + 2;

    distribution.assign( num_buckets, 0 );
    for ( unsigned int i=0; i < size(); i++ )
    {
      int index = ( int ) ( num_buckets * ( ( *this )[ i ] - min + 1 ) / range );
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

  size_t sample_size = size();

  if ( sample_size == 0 )
    return std::numeric_limits<double>::quiet_NaN();

  sort_data();

  // Should be improved to use linear interpolation
  return ( *this )[ ( int ) ( x * ( sample_size - 1 ) ) ];
}

// sample_data_t::sort_data =============================================

void sample_data_t::sort_data()
{
  if ( ! sorted && size() > 0 )
  {
    std::sort( begin(), end() );
    sorted = true;
  }
}

// sample_data_t::merge =============================================

void sample_data_t::merge( sample_data_t& other )
{

  count += other.count;

  if ( simple )
    sum += other.sum;
  else
    insert( end(), other.begin(), other.end() );
}

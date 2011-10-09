 // ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// sample_data_t::sample_data_t =============================================

  sample_data_t::sample_data_t( ):
  sum(std::numeric_limits<double>::quiet_NaN()),mean(std::numeric_limits<double>::quiet_NaN()),
  min(std::numeric_limits<double>::quiet_NaN()),max(std::numeric_limits<double>::quiet_NaN()),
  variance(std::numeric_limits<double>::quiet_NaN()),std_dev(std::numeric_limits<double>::quiet_NaN()),
  median(std::numeric_limits<double>::quiet_NaN()),

  calculate_basics( true ),calculate_variance(false),sort(false),

  analyzed(),sorted()

  { }

// sample_data_t::analyze =============================================

  void sample_data_t::analyze(
      bool calc_basics,
      bool calc_variance,
      bool s)
  {
    if ( analyzed )
      return;
    analyzed = true;

    if ( calc_basics ) calculate_basics = true;
    if ( calc_variance ) calculate_variance = true;
    if ( s ) sort = true;


    // Enable necessary pre-calculations

    if ( calculate_variance )
      calculate_basics = true;

    int sample_size = size();

    if ( sample_size == 0 )
      return;

    // Calculate Sum, Mean, Min, Max
    if ( calculate_basics )
    {
      sum = 0;
      min = std::numeric_limits<double>::max();
      max = 0;

      for ( int i=0; i < sample_size; i++ )
      {
        double i_data = (*this)[ i ];
        sum  += i_data;
        if ( i_data < min ) min = i_data;
        if ( i_data > max ) max = i_data;
      }

      mean = sum / sample_size;

      if ( min == std::numeric_limits<double>::max() )
        min = std::numeric_limits<double>::quiet_NaN();
    }

    // Calculate Variance
    if ( calculate_variance )
    {
      variance = 0;
      for ( int i=0; i < sample_size; i++ )
      {
        double delta = (*this)[ i ] - mean;
        variance += delta * delta;
      }

      if ( sample_size > 1 )
      {
        variance /= ( sample_size - 1 );
      }

      std_dev = sqrt( variance );

      // Calculate Standard Deviation of the Mean ( Central Limit Theorem )
      mean_std_dev = std_dev / sqrt ( sample_size );
    }


    // Sort Data
    if ( sort )
    {
      sort_data();

      if ( sample_size % 2 == 1 )
        median = (*this)[ ( sample_size - 1 )/ 2];
      else
        median = ( (*this)[ sample_size / 2 - 1] + (*this)[sample_size / 2] ) / 2.0;
    }

  }

// sample_data_t::percentile =============================================

  double sample_data_t::percentile( double x )
  {
    assert( x >= 0 && x <= 1.0 );

    int sample_size = size();

    if ( sample_size == 0 )
      return std::numeric_limits<double>::quiet_NaN();

    sort_data();

    // Should be improved to use linear interpolation
    return (*this)[ (int) ( x * ( sample_size - 1 ) ) ];
  }

// sample_data_t::sort_data =============================================

  void sample_data_t::sort_data()
  {
    if ( ! sorted && size() > 0)
    {
      std::sort( begin(), end() );
      sorted = true;
    }
  }

// sample_data_t::merge =============================================

  void sample_data_t::merge( sample_data_t& other )
  {
    insert( end(),
                 other.begin(), other.end() );
  }

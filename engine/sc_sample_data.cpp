 // ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// sample_data_t::sample_data_t =============================================

  sample_data_t::sample_data_t( )
  {
    data.clear();
    sum = std::numeric_limits<double>::quiet_NaN();
    mean = std::numeric_limits<double>::quiet_NaN();
    min = std::numeric_limits<double>::quiet_NaN();
    max = std::numeric_limits<double>::quiet_NaN();
    variance = std::numeric_limits<double>::quiet_NaN();
    std_dev = std::numeric_limits<double>::quiet_NaN();
    median = std::numeric_limits<double>::quiet_NaN();


    calculate_sum         = false;
    calculate_mean        = false;
    calculate_min         = false;
    calculate_max         = false;
    calculate_variance    = false;
    calculate_median      = false;
    calculate_mean_std_dev  = false;
    sort                  = false;

    analyzed  = false;
    sorted    = false;
  }

// sample_data_t::analyze =============================================

  void sample_data_t::analyze(
      bool calc_sum,
      bool calc_mean,
      bool calc_min,
      bool calc_max,
      bool calc_variance,
      bool calc_median,
      bool calc_base_error,
      bool s)
  {
    if ( analyzed )
      return;
    analyzed = true;

    calculate_sum = calc_sum;
    calculate_mean = calc_mean;
    calculate_min = calc_min;
    calculate_max = calc_max;
    calculate_variance = calc_variance;
    calculate_median = calc_median;
    calculate_mean_std_dev = calc_base_error;
    sort = s;


    // Enable necessary pre-calculations
    if ( calculate_mean_std_dev )
      calculate_variance = true;

    if ( calculate_variance )
      calculate_mean = true;

    if ( calculate_median )
      sort = true;


    int sample_size = data.size();

    // Calculate Sum, Mean, Min, Max
    if ( calculate_mean || calculate_sum || calculate_min )
    {
      if ( calculate_sum ) sum = 0;
      if ( calculate_min ) min = std::numeric_limits<double>::max();
      if ( calculate_max ) max = 0;

      for ( int i=0; i < sample_size; i++ )
      {
        double i_data = data[ i ];
        if ( calculate_sum ) sum  += i_data;
        if ( calculate_min ) if ( i_data < min ) min = i_data;
        if ( calculate_max ) if ( i_data > max ) max = i_data;
      }

      if ( calculate_mean ) mean = sum / sample_size;
    }

    // Calculate Variance
    if ( calculate_variance )
    {
      variance = 0;
      for ( int i=0; i < sample_size; i++ )
      {
        double delta = data[ i ] - mean;
        variance += delta * delta;
      }

      if ( sample_size > 1 )
      {
        variance /= ( sample_size - 1 );
      }

      std_dev = sqrt( variance );
    }

    // Calculate Standard Deviation of the Mean ( Central Limit Theorem )
    if ( calculate_mean_std_dev )
      mean_std_dev = std_dev / sqrt ( sample_size );

    // Sort Data
    if ( sort )
    {
      sort_data();
    }

    // Calculate True Median
    if ( calculate_median )
    {
      if ( sample_size % 2 == 1 )
        median = data[ ( sample_size - 1 )/ 2];
      else
        median = ( data[ sample_size / 2 - 1] + data[sample_size / 2] ) / 2.0;
    }

  }

// sample_data_t::percentile =============================================

  double sample_data_t::percentile( double x )
  {
    assert( x >= 0 && x <= 1.0 );

    sort_data();

    // Should be improved to use linear interpolation
    return data[ x * ( data.size() - 1)  ];
  }

// sample_data_t::sort_data =============================================

  void sample_data_t::sort_data()
  {
    if ( ! sorted )
    {
      std::sort( data.begin(), data.end() );
      sorted = true;
    }
  }

// sample_data_t::merge =============================================

  void sample_data_t::merge( sample_data_t& other )
  {
    data.insert( data.end(),
                 other.data.begin(), other.data.end() );
  }

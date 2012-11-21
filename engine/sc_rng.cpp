// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_rng.hpp"

// Code to test functionality and performance of our RNG implementations

#ifdef UNIT_TEST
#include "sc_sample_data.hpp"
#include <cstdio>

namespace util {
int64_t milliseconds()
{
  return 1000 * clock() / CLOCKS_PER_SEC;
}
}

int main( int /*argc*/, char** /*argv*/ )
{
  uint64_t n = 1000000000;

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  // double real()
  {
    rngBase<rng_engine_mt_cc0x_t> rng2;

    int start_time = util::milliseconds();


    double average=0;
    for ( uint64_t i = 0; i< n; i++ )
      average += rng2.real();
    average /= n;
    int elapsed_cpu = util::milliseconds() - start_time;
    printf( "%ld calls to rngBase<rng_engine_mt_cc0x_t>::real(): average=%.8f time(ms)=%d numbers/sec=%.0f\n\n", n, average, elapsed_cpu, n * 1000.0 / elapsed_cpu );
  }
#endif // END C++0X

  // double real()
  {
    rngBase<rng_engine_std_t> rng3;

    int start_time = util::milliseconds();


    double average=0;
    for ( uint64_t i = 0; i< n; i++ )
      average += rng3.real();
    average /= n;
    int elapsed_cpu = util::milliseconds() - start_time;
    printf( "%ld calls to rngBase<rng_engine_std_t>::real(): average=%.8f time(ms)=%d numbers/sec=%.0f\n\n", n, average, elapsed_cpu, n * 1000.0 / elapsed_cpu );
  }

  rngBase<rng_engine_dsfmt_t> rng;

  // double real()
  {
    int start_time = util::milliseconds();


    double average=0;
    for ( uint64_t i = 0; i< n; i++ )
      average += rng.real();
    average /= n;
    int elapsed_cpu = util::milliseconds() - start_time;
    printf( "%ld calls to rngBase<rng_engine_dsfmt_t>::real(): average=%.8f time(ms)=%d numbers/sec=%.0f\n\n", n, average, elapsed_cpu, n * 1000.0 / elapsed_cpu );
  }

  // Monte-Carlo PI calculation.
  {
    int start_time = util::milliseconds();


    uint64_t count_in_sphere = 0;
    for ( uint64_t i = 0; i< n; i++ )
    {
      double x1 = rng.real();
      double x2 = rng.real();
      if ( x1*x1 + x2*x2 < 1.0 )
        ++count_in_sphere;
    }
    int elapsed_cpu = util::milliseconds() - start_time;
    printf( "%ld runs for pi-calculation. count in sphere=%ld: pi=%.20f time(ms)=%d numbers/sec=%.0f\n\n", n, count_in_sphere, static_cast<double>( count_in_sphere * 4 ) / n , elapsed_cpu, n * 1000.0 / elapsed_cpu );
  }
  n /= 10;

  // double gauss
  {
    int start_time = util::milliseconds();

    double average=0;
    for ( uint64_t i = 0; i< n; i++ )
      average += rng.gauss( 0,1 );
    average /= n;
    int elapsed_cpu = util::milliseconds() - start_time;
    printf( "%ld calls to gauss(0,1): average=%.8f time(ms)=%d\n\n", n, average, elapsed_cpu );
  }

  // timespan_t gauss
  {
    int start_time = util::milliseconds();

    double average=0;
    for ( uint64_t i = 0; i< n; i++ )
      average += rng.gauss( timespan_t::from_native( 700 ), timespan_t::from_native( 1000 ) ).total_seconds();
    average /= n;
    int elapsed_cpu = util::milliseconds() - start_time;
    printf( "%ld calls to gauss( timespan_t::from_native( 700 ), timespan_t::from_native( 1000 ) ): average=%.8f time(ms)=%d\n\n", n, average, elapsed_cpu );
  }

  // double exgauss
  {
    int start_time = util::milliseconds();

    sample_data_t exgauss_data = sample_data_t( false );
    exgauss_data.reserve( n );

    double average=0;
    for ( uint64_t i = 0; i< n; i++ )
    {
      double result = 0.1 + rng.exgauss( 0.3,0.06,0.25 );
      average += result;
      exgauss_data.add( result );
    }
    average /= n;
    int elapsed_cpu = util::milliseconds() - start_time;

    exgauss_data.analyze();
    printf( "%ld calls to 0.1 + exgauss(0.3,0.06,0.25): time(ms)=%d\n", n, elapsed_cpu );
    printf( "exgauss sd: mean = %.8f 5thpct = %.8f 95thpct = %.8f 0.9999 quantille = %.8f min = %.8f max = %.8f  \n\n", exgauss_data.mean, exgauss_data.percentile( 0.05 ), exgauss_data.percentile( 0.95 ), exgauss_data.percentile( 0.9999 ), exgauss_data.min, exgauss_data.max );

  }

  // timespan_t exgauss
  {
    static const timespan_t mean = timespan_t::from_native( 300 );
    static const timespan_t stddev = timespan_t::from_native( 60 );
    static const timespan_t nu = timespan_t::from_native( 250 );

    int start_time = util::milliseconds();


    double average=0;
    for ( uint64_t i = 0; i< n; i++ )
    {
      double result = 0.1 + rng.exgauss( mean, stddev, nu ).total_seconds();
      average += result;
    }
    average /= n;
    int elapsed_cpu = util::milliseconds() - start_time;

    printf( "%ld calls to timespan_t::from_native( 100 ) + exgauss( timespan_t::from_native( 300 ), timespan_t::from_native( 60 ), timespan_t::from_native( 250 ) ): average=%.8f time(ms)=%d\n\n", n, average, elapsed_cpu );

  }

  // exponential
  {

    int start_time = util::milliseconds();

    double nu = 0.25;
    double average=0;
    for ( uint64_t i = 0; i< n; i++ )
    {
      double result = rng.exponential( nu );
      average += result;
    }
    average /= n;
    int elapsed_cpu = util::milliseconds() - start_time;

    printf( "%ld calls exp nu=0.25: average=%.8f time(ms)=%d\n\n", n, average, elapsed_cpu );

  }


  printf( "\nreal:\n" );
  for ( unsigned i = 1; i <= 100; i++ )
  {
    printf( "  %.8f", rng.real() );
    if ( i % 10 == 0 ) printf( "\n" );
  }

  printf( "\ngauss mean=0, std_dev=1.0:\n" );
  for ( unsigned i = 1; i <= 100; i++ )
  {
    printf( "  %.8f", rng.gauss( 0.0, 1.0 ) );
    if ( i % 10 == 0 ) printf( "\n" );
  }

  printf( "\nroll 30%%:\n" );
  for ( unsigned i = 1; i <= 100; i++ )
  {
    printf( "  %d", rng.roll( 0.30 ) );
    if ( i % 10 == 0 ) printf( "\n" );
  }
}

#endif

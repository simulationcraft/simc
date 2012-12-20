// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#if defined(UNIT_TEST) && ( __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || ( defined(_MSC_VER) && _MSC_VER >= 1700 ) )
#define SC_CXX11_RNG
// Order-of-inclusion bug under MSVC: Include <random> early.
#include <random>
#endif
#include "sc_rng.hpp"

// stdnormal_cdf ============================================================

// Source of the next 2 functions: http://home.online.no/~pjacklam/notes/invnorm/

/*
 * The standard normal CDF, for one random variable.
 *
 *   Author:  W. J. Cody
 *   URL:   http://www.netlib.org/specfun/erf
 *
 * This is the erfc() routine only, adapted by the
 * transform stdnormal_cdf(u)=(erfc(-u/sqrt(2))/2;
 */

double rng::stdnormal_cdf( double u )
{
  static const double a[5] =
  {
    1.161110663653770e-002,3.951404679838207e-001,2.846603853776254e+001,
    1.887426188426510e+002,3.209377589138469e+003
  };
  static const double b[5] =
  {
    1.767766952966369e-001,8.344316438579620e+000,1.725514762600375e+002,
    1.813893686502485e+003,8.044716608901563e+003
  };
  static const double c[9] =
  {
    2.15311535474403846e-8,5.64188496988670089e-1,8.88314979438837594e00,
    6.61191906371416295e01,2.98635138197400131e02,8.81952221241769090e02,
    1.71204761263407058e03,2.05107837782607147e03,1.23033935479799725E03
  };
  static const double d[9] =
  {
    1.00000000000000000e00,1.57449261107098347e01,1.17693950891312499e02,
    5.37181101862009858e02,1.62138957456669019e03,3.29079923573345963e03,
    4.36261909014324716e03,3.43936767414372164e03,1.23033935480374942e03
  };
  static const double p[6] =
  {
    1.63153871373020978e-2,3.05326634961232344e-1,3.60344899949804439e-1,
    1.25781726111229246e-1,1.60837851487422766e-2,6.58749161529837803e-4
  };
  static const double q[6] =
  {
    1.00000000000000000e00,2.56852019228982242e00,1.87295284992346047e00,
    5.27905102951428412e-1,6.05183413124413191e-2,2.33520497626869185e-3
  };

  double y, z;

  if ( u == std::numeric_limits<double>::quiet_NaN() )
    return std::numeric_limits<double>::quiet_NaN();

  if ( u == std::numeric_limits<double>::infinity() )
    return ( u < 0 ? 0.0 : 1.0 );

  y = fabs( u );

  if ( y <= 0.46875* sqrt( 2.0 ) )
  {
    /* evaluate erf() for |u| <= sqrt(2)*0.46875 */
    z = y * y;
    y = u * ( ( ( ( a[0]*z+a[1] )*z+a[2] )*z+a[3] )*z+a[4] )
        /( ( ( ( b[0]*z+b[1] )*z+b[2] )*z+b[3] )*z+b[4] );
    return 0.5 + y;
  }

  z = exp( -y * y / 2 ) / 2;

  if ( y <= 4.0 )
  {
    /* evaluate erfc() for sqrt(2)*0.46875 <= |u| <= sqrt(2)*4.0 */
    y = y / sqrt( 2.0 );
    y =
      ( ( ( ( ( ( ( ( c[0]*y+c[1] )*y+c[2] )*y+c[3] )*y+c[4] )*y+c[5] )*y+c[6] )*y+c[7] )*y+c[8] )


      /( ( ( ( ( ( ( ( d[0]*y+d[1] )*y+d[2] )*y+d[3] )*y+d[4] )*y+d[5] )*y+d[6] )*y+d[7] )*y+d[8] );

    y = z * y;
  }
  else
  {
    /* evaluate erfc() for |u| > sqrt(2)*4.0 */
    z = z * sqrt( 2.0 ) / y;
    y = 2 / ( y * y );
    y = y*( ( ( ( ( p[0]*y+p[1] )*y+p[2] )*y+p[3] )*y+p[4] )*y+p[5] )
        /( ( ( ( ( q[0]*y+q[1] )*y+q[2] )*y+q[3] )*y+q[4] )*y+q[5] );
    y = z*( 1.0 / sqrt ( M_PI ) - y );
  }

  return ( u < 0.0 ? y : 1-y );
}

// stdnormal_inv ============================================================

// This is used to get the normal distribution inverse for our user-specifiable confidence levels
// For example for the default 95% confidence level, this function will return the well known number of
// 1.96, so we know that 95% of the distribution is between -1.96 and +1.96 std deviations from the mean.

/*
 * The inverse standard normal distribution.
 *
 *   Author:      Peter John Acklam <pjacklam@online.no>
 *   URL:         http://home.online.no/~pjacklam
 *
 * This function is based on the MATLAB code from the address above,
 * translated to C, and adapted for our purposes.
 */

double rng::stdnormal_inv( double p )
{
  static const double a[6] =
  {
    -3.969683028665376e+01,  2.209460984245205e+02,
    -2.759285104469687e+02,  1.383577518672690e+02,
    -3.066479806614716e+01,  2.506628277459239e+00
  };
  static const double b[5] =
  {
    -5.447609879822406e+01,  1.615858368580409e+02,
    -1.556989798598866e+02,  6.680131188771972e+01,
    -1.328068155288572e+01
  };
  static const double c[6] =
  {
    -7.784894002430293e-03, -3.223964580411365e-01,
    -2.400758277161838e+00, -2.549732539343734e+00,
    4.374664141464968e+00,  2.938163982698783e+00
  };
  static const double d[4] =
  {
    7.784695709041462e-03,  3.224671290700398e-01,
    2.445134137142996e+00,  3.754408661907416e+00
  };

  double q, t, u;

  if ( p == std::numeric_limits<double>::quiet_NaN() || p > 1.0 || p < 0.0 )
    return std::numeric_limits<double>::quiet_NaN();

  if ( p == 0.0 )
    return - std::numeric_limits<double>::infinity();

  if ( p == 1.0 )
    return std::numeric_limits<double>::infinity();

  q = std::min( p, 1 - p );

  if ( q > 0.02425 )
  {
    /* Rational approximation for central region. */
    u = q - 0.5;
    t = u * u;
    u = u * ( ( ( ( ( a[0]*t+a[1] )*t+a[2] )*t+a[3] )*t+a[4] )*t+a[5] )
        /( ( ( ( ( b[0]*t+b[1] )*t+b[2] )*t+b[3] )*t+b[4] )*t+1 );
  }
  else
  {
    /* Rational approximation for tail region. */
    t = sqrt( -2*log( q ) );
    u = ( ( ( ( ( c[0]*t+c[1] )*t+c[2] )*t+c[3] )*t+c[4] )*t+c[5] )
        /( ( ( ( d[0]*t+d[1] )*t+d[2] )*t+d[3] )*t+1 );
  }

  /* The relative error of the approximation has absolute value less
     than 1.15e-9.  One iteration of Halley's rational method (third
     order) gives full machine precision... */
  t = stdnormal_cdf( u ) - q;    /* error */
  t = t * 2.0 / sqrt( M_PI ) *exp( u*u/2 ); /* f(u)/df(u) */
  u = u-t/( 1+u*t/2 );   /* Halley's method */

  return ( p > 0.5 ? -u : u );
}

#ifdef UNIT_TEST
// Code to test functionality and performance of our RNG implementations

#include "sc_sample_data.hpp"

namespace { // anonymous namespace ==========================================

// ==========================================================================
// C Standard Library Random Number Generator
// ==========================================================================

class rng_engine_std_t
{
  // Whether this RNG works correctly or explodes in your face when
  // multithreading depends on your particular C library implementation,
  // since the standard doesn't describe the behavior of seed()/rand() in the
  // presence of multiple threads. (If it does work, it will likely be very
  // _slow_).
public:
  void seed( uint32_t start )
  { srand( start ); }

  double operator()()
  { return rand() * ( 1.0 / ( 1.0 + RAND_MAX ) ); }
};

// ==========================================================================
// C++ 11 Mersenne Twister Random Number Generator
// ==========================================================================

#ifdef SC_CXX11_RNG
/*
 * This integrates a C++11 random number generator into our native rng_t concept
 * The advantage of this container is the extremely small code, with nearly no
 * maintenance cost.
 * Unfortunately, it is around 40 percent slower than the dsfmt implementation.
 */

class rng_engine_mt_cxx11_hack_t
{
  std::mt19937 engine; // Mersenne twister MT19937
public:
  void seed( uint32_t start ) { engine.seed( start ); }

  double operator()()
  { return ( static_cast<double>( engine() ) - engine.min() ) / ( engine.max() - engine.min() ); }
};

class rng_engine_mt_cxx11_t
{
  std::mt19937 engine; // Mersenne twister MT19937
  std::uniform_real_distribution<double> dist;
public:
  void seed( uint32_t start ) { engine.seed( start ); }
  double operator()() { return dist( engine ); }
};
#endif // END SC_CXX11_RNG

int64_t milliseconds()
{
  return 1000 * clock() / CLOCKS_PER_SEC;
}

template <typename Engine>
void test_one( uint64_t n, const char* name )
{
  rng_base_t<Engine> rng;

  int64_t start_time = milliseconds();

  double average = 0;
  for ( uint64_t i = 0; i < n; ++i )
    average += rng.real();

  average /= n;
  int64_t elapsed_cpu = milliseconds() - start_time;

  std::cout << n << " calls to rng_base_t<" << name
            << ">::real(): average = " << std::setprecision( 8 ) << average
            << ", time = " << elapsed_cpu << " ms"
               ", numbers/sec = " << static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) << "\n\n";
}

// Monte-Carlo PI calculation.
template <typename Engine>
void monte_carlo( uint64_t n, const char* name )
{
  rng_base_t<Engine> rng;
  int64_t start_time = milliseconds();

  uint64_t count_in_sphere = 0;
  for ( uint64_t i = 0; i < n; ++i )
  {
    double x1 = rng.real();
    double x2 = rng.real();
    if ( x1*x1 + x2*x2 < 1.0 )
      ++count_in_sphere;
  }
  int64_t elapsed_cpu = milliseconds() - start_time;

  std::cout << n << " runs for pi-calculation with " << name
            << ". count in sphere = " << count_in_sphere
            << ", pi = " << std::setprecision( 21 ) << static_cast<double>( count_in_sphere ) * 4 / n
            << ", time = " << elapsed_cpu << " ms"
               ", numbers/sec = " << static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) << "\n\n";
}

} // anonymous namespace ====================================================

int main( int /*argc*/, char** /*argv*/ )
{
  uint64_t n = 1000000000;

#define TEST_ONE( e ) test_one<e>( n, #e )

  TEST_ONE( rng_engine_std_t );
  TEST_ONE( rng_engine_mt_t );

#ifdef SC_USE_SSE2
  TEST_ONE( rng_engine_mt_sse2_t );
#endif

#ifdef SC_CXX11_RNG
  TEST_ONE( rng_engine_mt_cxx11_hack_t );
  TEST_ONE( rng_engine_mt_cxx11_t );
#endif // END C++0X


#define MONTE_CARLO( t ) monte_carlo<t>( n, #t )

  MONTE_CARLO( rng_engine_mt_sse2_t );

  n /= 10;

  rng_t rng;

  // double gauss
  {
    int64_t start_time = milliseconds();

    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
      average += rng.gauss( 0, 1 );
    average /= n;
    int64_t elapsed_cpu = milliseconds() - start_time;

    std::cout << n << " calls to gauss(0,1): average = " << std::setprecision( 8 ) << average
              << ", time = " << elapsed_cpu << " ms\n\n";
  }

  // timespan_t gauss
  {
    int64_t start_time = milliseconds();

    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
      average += rng.gauss( timespan_t::from_millis( 700 ), timespan_t::from_millis( 1000 ) ).total_seconds();
    average /= n;
    int64_t elapsed_cpu = milliseconds() - start_time;

    std::cout << n << " calls to gauss( timespan_t::from_millis( 700 ), timespan_t::from_millis( 1000 ) ): "
      "average = " << std::setprecision( 8 ) << average << " time = " << elapsed_cpu << " ms\n\n";
  }

  // double exgauss
  {
    int64_t start_time = milliseconds();

    sample_data_t exgauss_data = sample_data_t( false );
    exgauss_data.reserve( n );

    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
    {
      double result = 0.1 + rng.exgauss( 0.3,0.06,0.25 );
      average += result;
      exgauss_data.add( result );
    }
    average /= n;
    int64_t elapsed_cpu = milliseconds() - start_time;

    exgauss_data.analyze();

    std::cout.precision( 8 );
    std::cout << n << " calls to 0.1 + exgauss(0.3,0.06,0.25): "
                 "time = " << elapsed_cpu << " ms\n"
                 "exgauss sd: mean = " << exgauss_data.mean << ", "
                 "5thpct = " << exgauss_data.percentile( 0.05 ) << ", "
                 "95thpct = " << exgauss_data.percentile( 0.95 ) << ", "
                 "0.9999 quantille = " << exgauss_data.percentile( 0.9999 ) << ", "
                 "min = " << exgauss_data.min << ", max = " << exgauss_data.max << "\n\n";
  }

  // timespan_t exgauss
  {
    static const timespan_t mean = timespan_t::from_millis( 300 );
    static const timespan_t stddev = timespan_t::from_millis( 60 );
    static const timespan_t nu = timespan_t::from_millis( 250 );

    int64_t start_time = milliseconds();
    
    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
    {
      double result = ( timespan_t::from_millis( 100 ) + rng.exgauss( mean, stddev, nu ) ).total_seconds();
      average += result;
    }
    average /= n;
    int64_t elapsed_cpu = milliseconds() - start_time;

    std::cout << n << " calls to timespan_t::from_millis( 100 ) + exgauss( timespan_t::from_millis( 300 ), timespan_t::from_millis( 60 ), timespan_t::from_millis( 250 ) ): "
                 "average = " << average << ", "
                 "time = " << elapsed_cpu << " ms\n\n";
  }

  // exponential
  {
    int64_t start_time = milliseconds();

    double nu = 0.25;
    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
    {
      double result = rng.exponential( nu );
      average += result;
    }
    average /= n;
    int64_t elapsed_cpu = milliseconds() - start_time;

    std::cout << n << " calls exp nu=0.25: "
                 "average = " << average << " "
                 "time = " << elapsed_cpu << " ms\n\n";
  }

  std::cout << "\nreal:\n";
  for ( unsigned i = 1; i <= 100; i++ )
  {
    std::cout << "  " << rng.real();
    if ( i % 10 == 0 )
      std::cout << '\n';
  }

  std::cout << "\ngauss mean=0, std_dev=1.0:\n";
  for ( unsigned i = 1; i <= 100; i++ )
  {
    std::cout << "  " << rng.gauss( 0.0, 1.0 );
    if ( i % 10 == 0 )
      std::cout << '\n';
  }

  std::cout << "\nroll 30%:\n";
  for ( unsigned i = 1; i <= 100; i++ )
  {
    std::cout << "  " << rng.roll( 0.30 );
    if ( i % 10 == 0 )
      std::cout << '\n';
  }
}

#endif // UNIT_TEST

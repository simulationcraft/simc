// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "rng.hpp"

#include <cstdint>
#include <algorithm>

// Pseudo-Random Number Generation ==========================================

namespace rng {

namespace {

/**
 * @brief SplitMix64 Random Number Generator
 *
 * Used to seed other generators
 *
 * All credit goes to Sebastiano Vigna (vigna@acm.org) @2014
 * http://prng.di.unimi.it/
 */
struct split_mix64_t
{
  uint64_t x; // The state can be seeded with any value.

  uint64_t next() noexcept
  {
    uint64_t z = (x += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
  }

  void seed( uint64_t start ) noexcept
  {
    x = start;
  }

  const char* name() const noexcept
  {
    return "SplitMix64";
  }
};

template <typename Range>
void init_state_from_mix64( Range& range, uint64_t start)
{
  split_mix64_t mix64;
  mix64.seed( start );
  for (auto & elem : range)
    elem = mix64.next();
}

constexpr uint64_t rotl( const uint64_t x, int k ) {
  return (x << k) | (x >> (64 - k));
}

} // anon namespace

/**
 * @brief XORSHIFT-128 Random Number Generator
 *
 * All credit goes to Sebastiano Vigna (vigna@acm.org) @2014
 * http://xorshift.di.unimi.it/
 */
uint64_t xorshift128_t::next() noexcept
{
  uint64_t s1 = s[ 0 ];
  const uint64_t s0 = s[ 1 ];
  s[ 0 ] = s0;
  s1 ^= s1 << 23; // a
  return ( s[ 1 ] = ( s1 ^ s0 ^ ( s1 >> 17 ) ^ ( s0 >> 26 ) ) ) + s0; // b, c
}

void xorshift128_t::seed( uint64_t start ) noexcept
{
  init_state_from_mix64(s, start);
}

const char* xorshift128_t::name() const noexcept
{
  return "xorshift128";
}

/**
 * @brief xoshiro256+ Random Number Generator
 *
 * If, however, one has to generate only 64-bit floating-point numbers (by extracting the upper 53 bits) xoshiro256+
 * is a slightly (≈15%) faster [compared to xoshiro256** or xoshiro256++] generator with analogous statistical
 * properties.
 *
 * All credit goes to David Blackman and Sebastiano Vigna (vigna@acm.org) @2018
 * http://prng.di.unimi.it/
 */
uint64_t xoshiro256plus_t::next() noexcept
{
  const uint64_t result = s[0] + s[3];

  const uint64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 45);

  return result;
}

void xoshiro256plus_t::seed( uint64_t start ) noexcept
{
  init_state_from_mix64(s, start);
}

const char* xoshiro256plus_t::name() const noexcept
{
  return "xoshiro256+";
}

/**
 * @brief XORSHIFT-1024 Random Number Generator
 *
 * All credit goes to Sebastiano Vigna (vigna@acm.org) @2014
 * http://xorshift.di.unimi.it/
 */
uint64_t xorshift1024_t::next() noexcept
{
  uint64_t s0 = s[ p ];
  uint64_t s1 = s[ p = ( p + 1 ) & 15 ];
  s1 ^= s1 << 31; // a
  s1 ^= s1 >> 11; // b
  s0 ^= s0 >> 30; // c
  return ( s[ p ] = s0 ^ s1 ) * 1181783497276652981LL;
}

void xorshift1024_t::seed( uint64_t start ) noexcept
{
  init_state_from_mix64(s, start);
}

const char* xorshift1024_t::name() const noexcept
{
  return "xorshift1024";
}

/**
 * @brief The standard normal CDF, for one random variable.
 *
 *   Author:  W. J. Cody
 *   URL:     http://www.netlib.org/specfun/erf
 *   Source:  http://home.online.no/~pjacklam/notes/invnorm/
 *
 * This is the erfc() routine only, adapted by the
 * transform stdnormal_cdf(u)=(erfc(-u/sqrt(2))/2;
 */
double stdnormal_cdf( double u )
{
  if ( u == std::numeric_limits<double>::infinity() )
    return ( u < 0 ? 0.0 : 1.0 );

  double y;
  double z;

  y = fabs( u );

  if ( y <= 0.46875 * sqrt( 2.0 ) )
  {
    static const double a[5] =
    {
      1.161110663653770e-002, 3.951404679838207e-001, 2.846603853776254e+001,
      1.887426188426510e+002, 3.209377589138469e+003
    };

    static const double b[5] =
    {
      1.767766952966369e-001, 8.344316438579620e+000, 1.725514762600375e+002,
      1.813893686502485e+003, 8.044716608901563e+003
    };

    /* evaluate erf() for |u| <= sqrt(2)*0.46875 */
    z = y * y;
    y = u * ( ( ( ( a[0] * z + a[1] ) * z + a[2] ) * z + a[3] ) * z + a[4] )
        / ( ( ( ( b[0] * z + b[1] ) * z + b[2] ) * z + b[3] ) * z + b[4] );
    return 0.5 + y;
  }

  z = exp( -y * y / 2 ) / 2;

  if ( y <= 4.0 )
  {
    static const double c[9] =
    {
      2.15311535474403846e-8, 5.64188496988670089e-1, 8.88314979438837594e00,
      6.61191906371416295e01, 2.98635138197400131e02, 8.81952221241769090e02,
      1.71204761263407058e03, 2.05107837782607147e03, 1.23033935479799725E03
    };

    static const double d[9] =
    {
      1.00000000000000000e00, 1.57449261107098347e01, 1.17693950891312499e02,
      5.37181101862009858e02, 1.62138957456669019e03, 3.29079923573345963e03,
      4.36261909014324716e03, 3.43936767414372164e03, 1.23033935480374942e03
    };

    /* evaluate erfc() for sqrt(2)*0.46875 <= |u| <= sqrt(2)*4.0 */
    y = y / sqrt( 2.0 );
    y =
      ( ( ( ( ( ( ( ( c[0] * y + c[1] ) * y + c[2] ) * y + c[3] ) * y + c[4] ) * y + c[5] ) * y + c[6] ) * y + c[7] ) * y + c[8] )


      / ( ( ( ( ( ( ( ( d[0] * y + d[1] ) * y + d[2] ) * y + d[3] ) * y + d[4] ) * y + d[5] ) * y + d[6] ) * y + d[7] ) * y + d[8] );

    y = z * y;
  }
  else
  {
    static const double p[6] =
    {
      1.63153871373020978e-2, 3.05326634961232344e-1, 3.60344899949804439e-1,
      1.25781726111229246e-1, 1.60837851487422766e-2, 6.58749161529837803e-4
    };
    static const double q[6] =
    {
      1.00000000000000000e00, 2.56852019228982242e00, 1.87295284992346047e00,
      5.27905102951428412e-1, 6.05183413124413191e-2, 2.33520497626869185e-3
    };

    /* evaluate erfc() for |u| > sqrt(2)*4.0 */
    z = z * sqrt( 2.0 ) / y;
    y = 2 / ( y * y );
    y = y * ( ( ( ( ( p[0] * y + p[1] ) * y + p[2] ) * y + p[3] ) * y + p[4] ) * y + p[5] )
        / ( ( ( ( ( q[0] * y + q[1] ) * y + q[2] ) * y + q[3] ) * y + q[4] ) * y + q[5] );
    y = z * ( 1.0 / sqrt ( m_pi ) - y );
  }

  return ( u < 0.0 ? y : 1 - y );
}

/**
 * @brief The inverse standard normal distribution.
 *
 * This is used to get the normal distribution inverse for our user-specifiable confidence levels.
 * For example for the default 95% confidence level, this function will return the well known number of
 * 1.96, so we know that 95% of the distribution is between -1.96 and +1.96 std deviations from the mean.
 *
 *   Author:      Peter John Acklam <pjacklam@online.no>
 *   URL:         http://home.online.no/~pjacklam
 *   Source:      http://home.online.no/~pjacklam/notes/invnorm/
 *
 * This function is based on the MATLAB code from the address above,
 * translated to C, and adapted for our purposes.
 */
double stdnormal_inv( double p )
{
  if ( p > 1.0 || p < 0.0 )
  {
    assert( false );
    return 0;
  }

  if ( p == 0.0 )
    return - std::numeric_limits<double>::infinity();

  if ( p == 1.0 )
    return std::numeric_limits<double>::infinity();

  double q;

  double t;

  double u;

  q = std::min( p, 1 - p );

  if ( q > 0.02425 )
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

    /* Rational approximation for central region. */
    u = q - 0.5;
    t = u * u;
    u = u * ( ( ( ( ( a[0] * t + a[1] ) * t + a[2] ) * t + a[3] ) * t + a[4] ) * t + a[5] )
        / ( ( ( ( ( b[0] * t + b[1] ) * t + b[2] ) * t + b[3] ) * t + b[4] ) * t + 1 );
  }
  else
  {
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

    /* Rational approximation for tail region. */
    t = sqrt( -2 * log( q ) );
    u = ( ( ( ( ( c[0] * t + c[1] ) * t + c[2] ) * t + c[3] ) * t + c[4] ) * t + c[5] )
        / ( ( ( ( d[0] * t + d[1] ) * t + d[2] ) * t + d[3] ) * t + 1 );
  }

  /* The relative error of the approximation has absolute value less
     than 1.15e-9.  One iteration of Halley's rational method (third
     order) gives full machine precision... */
  t = stdnormal_cdf( u ) - q;    /* error */
  t = t * 2.0 / sqrt( m_pi ) * exp( u * u / 2 ); /* f(u)/df(u) */
  u = u - t / ( 1 + u * t / 2 );   /* Halley's method */

  return ( p > 0.5 ? -u : u );
}

} // rng
#ifdef UNIT_TEST
// Code to test functionality and performance of our RNG implementations

#include <random>
#include <tuple>

#include "lib/fmt/format.h"
#include "util/generic.hpp"
#include "util/chrono.hpp"

using test_clock = chrono::cpu_clock;

template <typename Engine>
static void test_one( rng::basic_rng_t<Engine>& rng, uint64_t n )
{
  auto start_time = test_clock::now();

  double average = 0;
  for ( uint64_t i = 0; i < n; ++i )
  {
    double d = rng.real();
    //std::cout << d << "\n";
    average += d;
  }

  average /= n;
  auto elapsed_cpu = chrono::elapsed_fp_seconds(start_time);

  fmt::print( "{} calls to rng::{}::real(), average = {:.8f}, time = {}s, numbers/sec = {}\n\n",
              n, rng.name(), average, elapsed_cpu, static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) );
}

template <typename Engine>
static void test_seed( rng::basic_rng_t<Engine>& rng, uint64_t n )
{
  auto start_time = test_clock::now();

  for ( uint64_t i = 0; i < n; ++i )
  {
    rng.seed( uint64_t( m_pi ) * n );
  }

  auto elapsed_cpu = chrono::elapsed_fp_seconds(start_time);

  fmt::print( "{} calls to rng::{}::seed(), time = {}s, numbers/sec = {}\n\n",
              n, rng.name(), elapsed_cpu, static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) );
}

// Monte-Carlo PI calculation.
template <typename Engine>
static void monte_carlo( rng::basic_rng_t<Engine>& rng, uint64_t n )
{
  auto start_time = test_clock::now();

  uint64_t count_in_sphere = 0;
  for ( uint64_t i = 0; i < n; ++i )
  {
    double x1 = rng.real();
    double x2 = rng.real();
    if ( x1 * x1 + x2 * x2 < 1.0 )
      ++count_in_sphere;
  }
  auto elapsed_cpu = chrono::elapsed_fp_seconds(start_time);

  fmt::print( "{} runs for pi-calculation with {}. count in sphere = {}, pi = {:.21f}, time = {}s, numbers/sec = {}\n\n",
              n, rng.name(), count_in_sphere, static_cast<double>( count_in_sphere ) * 4 / n,
              elapsed_cpu, static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) );
}

template <typename Engine>
static void test_uniform_int( rng::basic_rng_t<Engine>& rng, uint64_t n, unsigned num_buckets )
{
  auto start_time = test_clock::now();

  std::vector<unsigned> histogram(num_buckets);

  for ( uint64_t i = 0; i < n; ++i )
  {
    auto result = rng.range(histogram.size());
    histogram[ result ] += 1;
  }

  auto elapsed_cpu = chrono::elapsed_fp_seconds(start_time);

  double expected_bucket_size = static_cast<double>(n) / histogram.size();

  fmt::print("{} call to rng::{}::range(size_t({}u))\n", n, rng.name(), histogram.size());
  for (unsigned i = 0; i < histogram.size(); ++i)
  {
    double pct = static_cast<double>(histogram[ i ]) / n;
    double diff = static_cast<double>(histogram[ i ]) / expected_bucket_size - 1.0;
    fmt::print("  bucket {:2n}: {:5.2f}% ({}) difference to expected: {:9.6f}%\n", i, pct, histogram[ i ], diff);
  }
  fmt::print("time = {} s\n\n", elapsed_cpu);
}

namespace detail {
template <typename Tuple, typename F, std::size_t... I>
void for_each_impl(Tuple&& t, F&& f, std::index_sequence<I...>)
{
  int _[] = { ( range::invoke( std::forward<F>( f ), std::get<I>( std::forward<Tuple>(t) ) ), 0 )... };
  (void)_;
}
}

template <typename Tuple, typename F>
void for_each(Tuple&& t, F&& f)
{
    return detail::for_each_impl( std::forward<Tuple>(t), std::forward<F>(f),
            std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}

int main( int /*argc*/, char** /*argv*/ )
{
  auto generators = std::make_tuple(
    rng::basic_rng_t<rng::xoshiro256plus_t>{},
    rng::basic_rng_t<rng::xorshift128_t>{},
    rng::basic_rng_t<rng::xorshift1024_t>{}
  );

  std::random_device rd;
  uint64_t seed  = uint64_t(rd()) | (uint64_t(rd()) << 32);
  fmt::print( "Seed: {}\n\n", seed );

  for_each( generators, [seed]( auto& g ) { g.seed( seed ); } );

  for_each( generators, []( auto& g ) { test_one( g, 1'000'000'000 ); } );

  for_each( generators, []( auto& g ) { monte_carlo( g, 1'000'000'000 ); } );

  for_each( generators, []( auto& g ) { test_seed( g, 10'000'000 ); } );

  for_each( generators, []( auto& g ) { test_uniform_int( g, 10'000, 10 ); } );

  fmt::print( "random device: min={} max={}\n\n", rd.min(), rd.max() );

  auto& rng = std::get<0>( generators );
  fmt::print( "Testing {}\n\n", rng.name() );

  const uint64_t n = 100'000'000;

  // double gauss
  {
    auto start_time = test_clock::now();

    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
      average += rng.gauss( 0, 1 );
    average /= n;

    fmt::print( "{} calls to gauss(0, 1): average = {:.8f}, time = {}s\n\n",
                n, average, chrono::elapsed_fp_seconds(start_time) );
  }

  // double exgauss
  {
    auto start_time = test_clock::now();

    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
      average += 0.1 + rng.exgauss( 0.3, 0.06, 0.25 );
    average /= n;

    fmt::print( "{} calls to 0.1 + rng.exgauss( 0.3,0.06,0.25 );: average = {:.8f}, time = {}s\n\n",
                n, average, chrono::elapsed_fp_seconds(start_time) );
  }

  // exponential
  {
    auto start_time = test_clock::now();

    double nu = 0.25;
    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
    {
      double result = rng.exponential( nu );
      average += result;
    }
    average /= n;

    fmt::print( "{} calls exp nu=0.25: average = {}, time = {}s\n\n",
                n, average, chrono::elapsed_fp_seconds(start_time) );
  }

  fmt::print( "\nreal:\n" );
  for ( unsigned i = 1; i <= 100; i++ )
    fmt::print( "  {:>11.8f}{}", rng.real(), i % 10 == 0 ? "\n" : "" );

  fmt::print( "\ngauss mean=0, std_dev=1.0:\n" );
  for ( unsigned i = 1; i <= 100; i++ )
    fmt::print( "  {:>11.8f}{}", rng.gauss( 0.0, 1.0 ), i % 10 == 0 ? "\n" : "" );

  fmt::print( "\nroll 30%:\n" );
  for ( unsigned i = 1; i <= 100; i++ )
    fmt::print( "  {:d}{}", rng.roll( 0.30 ), i % 10 == 0 ? "\n" : "" );

  fmt::print( "\n" );
  fmt::print( "m_pi={}\n", m_pi );
  fmt::print( "calls to rng::stdnormal_inv( double x )\n" );
  fmt::print( "x=0.975: {:.7} should be equal to 1.959964\n", rng::stdnormal_inv( 0.975 ) );
  fmt::print( "x=0.995: {:.8} should be equal to 2.5758293\n", rng::stdnormal_inv( 0.995 ) );
}

#endif // UNIT_TEST

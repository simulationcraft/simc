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
    return 1.0;
  if ( u == -std::numeric_limits<double>::infinity() )
    return 0.0;

  double y;
  double z;

  y = fabs( u );

  if ( y <= 0.46875 * sqrt( 2.0 ) )
  {
    static constexpr double a[5] =
    {
      1.161110663653770e-002, 3.951404679838207e-001, 2.846603853776254e+001,
      1.887426188426510e+002, 3.209377589138469e+003
    };

    static constexpr double b[5] =
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
    return -std::numeric_limits<double>::infinity();

  if ( p == 1.0 )
    return std::numeric_limits<double>::infinity();

  double q = std::min( p, 1 - p );

  double t;
  double u;

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

void truncated_gauss_t::calculate_cdf()
{
#ifndef NDEBUG
  _count++;
#endif
  if ( !_cdf_set )
  {
    auto _mean = static_cast<double>( timespan_t::to_native( mean ) );
    auto _stddev = static_cast<double>( timespan_t::to_native( stddev ) );
    auto _min = static_cast<double>( timespan_t::to_native( min ) );
    auto _max = max == timespan_t::min() ? std::numeric_limits<double>::infinity()
                                         : static_cast<double>( timespan_t::to_native( max ) );

    assert( _min <= _max && "Minimum must be less than or equal to maximum." );

    _min_cdf = stdnormal_cdf( ( _min - _mean ) / _stddev );
    _max_cdf = stdnormal_cdf( ( _max - _mean ) / _stddev );

    _cdf_set = true;
  }
  else
  {
    assert( _min_cdf ==
            stdnormal_cdf( ( static_cast<double>( timespan_t::to_native( min ) ) -
                             static_cast<double>( timespan_t::to_native( mean ) ) ) /
                           static_cast<double>( timespan_t::to_native( stddev ) ) ) );
    assert( _max_cdf ==
            stdnormal_cdf( ( ( max == timespan_t::min() ? std::numeric_limits<double>::infinity()
                                                        : static_cast<double>( timespan_t::to_native( max ) ) ) -
                             static_cast<double>( timespan_t::to_native( mean ) ) ) /
                           static_cast<double>( timespan_t::to_native( stddev ) ) ) );
  }
}

} // rng

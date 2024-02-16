// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

/*! \file rng.hpp */
/*! \defgroup SC_RNG Random Number Generator */

#include "config.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iterator>
#ifdef RNG_STREAM_DEBUG
#include <iostream>
#endif

#include "util/timespan.hpp"

/** \ingroup SC_RNG
 * @brief Random number generation
 */
namespace rng {

/**\ingroup SC_RNG
 * @brief Random number generator wrapper around an rng engine
 *
 * Implements different distribution outputs ( uniform, gauss, etc. )
 */
template <typename Engine>
struct basic_rng_t
{
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
public:
  explicit basic_rng_t() = default;

  basic_rng_t(const basic_rng_t&) = delete;
  basic_rng_t& operator=(const basic_rng_t&) = delete;

  /// Return engine name
  const char* name() const {
    return engine.name();
  }

  /// Seed rng engine
  void seed( uint64_t s ) {
    engine.seed( s );
  }

  /// Reseed using current state
  uint64_t reseed();

  /// Reset any state
  void reset();

  /// Uniform distribution in range [0..1)
  double real();

  /// Bernoulli Distribution
  bool roll( double chance );

  /// Uniform distribution in the range [min..max)
  double range( double min, double max );

  /// Uniform distribution in the range [min..max)
  template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
  T range( T min, T max ) {
    return static_cast<T>(range(static_cast<double>(min), static_cast<double>(max)));
  }

  /// Uniform distribution in the range [0..max)
  template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
  T range( T max ) {
    return range<T>( T{}, max );
  }

  /// Gaussian Distribution
  double gauss( double mean, double stddev );

  // Truncated Gaussian Distribution
  double gauss_ab( double mean, double stddev, double min, double max );
  double gauss_a( double mean, double stddev, double min );
  double gauss_b( double mean, double stddev, double max );

  /// Exponential Distribution
  double exponential( double nu );

  /// Exponentially Modified Gaussian Distribution
  double exgauss( double gauss_mean, double gauss_stddev, double exp_nu );

  template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
  T gauss( T mean, T stddev )
  {
    assert( mean >= std::numeric_limits<double>::min() && mean <= std::numeric_limits<double>::max() );
    assert( stddev >= std::numeric_limits<double>::min() && stddev <= std::numeric_limits<double>::max() );

    return static_cast<T>( gauss( static_cast<double>( mean ), static_cast<double>( stddev ) ) );
  }

  template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
  T gauss_ab( T mean, T stddev, T min, T max )
  {
    assert( mean >= std::numeric_limits<double>::min() && mean <= std::numeric_limits<double>::max() );
    assert( stddev >= std::numeric_limits<double>::min() && stddev <= std::numeric_limits<double>::max() );
    assert( min >= std::numeric_limits<double>::min() && min <= std::numeric_limits<double>::max() );
    assert( max >= std::numeric_limits<double>::min() && max <= std::numeric_limits<double>::max() );

    return static_cast<T>( gauss_ab( static_cast<double>( mean ), static_cast<double>( stddev ),
                                     static_cast<double>( min ), static_cast<double>( max ) ) );
  }

  template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
  T gauss_a( T mean, T stddev, T min )
  {
    assert( mean >= std::numeric_limits<double>::min() && mean <= std::numeric_limits<double>::max() );
    assert( stddev >= std::numeric_limits<double>::min() && stddev <= std::numeric_limits<double>::max() );
    assert( min >= std::numeric_limits<double>::min() && min <= std::numeric_limits<double>::max() );

    return static_cast<T>(
        gauss_a( static_cast<double>( mean ), static_cast<double>( stddev ), static_cast<double>( min ) ) );
  }

  template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
  T gauss_b( T mean, T stddev, T max )
  {
    assert( mean >= std::numeric_limits<double>::min() && mean <= std::numeric_limits<double>::max() );
    assert( stddev >= std::numeric_limits<double>::min() && stddev <= std::numeric_limits<double>::max() );
    assert( max >= std::numeric_limits<double>::min() && max <= std::numeric_limits<double>::max() );

    return static_cast<T>(
        gauss_b( static_cast<double>( mean ), static_cast<double>( stddev ), static_cast<double>( max ) ) );
  }

  /// Timespan uniform distribution in the range [min..max)
  timespan_t range( timespan_t min, timespan_t max );

  /// Timespan Gaussian Distribution
  timespan_t gauss( timespan_t mean, timespan_t stddev );

  /// Timespan exponentially Modified Gaussian Distribution
  timespan_t exgauss( timespan_t mean, timespan_t stddev, timespan_t nu );

  /// Shuffle a range: https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
  template<typename T, typename std::enable_if_t<std::is_same_v<typename std::iterator_traits<T>::iterator_category, std::random_access_iterator_tag>, int> = 0>
  void shuffle( T first, T last )
  {
    size_t n = last - first;
    for ( size_t i = 0; i < n - 1; i++ )
      std::swap( first[ i ], first[ range( i, n ) ] );
  }

private:
  Engine engine;
  // Allow re-use of unused ( but necessary ) random number of a previous call to gauss()
  double gauss_pair_value = 0.0;
  bool   gauss_pair_use = false;
#ifdef RNG_STREAM_DEBUG
  uint64_t n = 0U;
#endif
};

/// Reseed using current state
template <typename Engine>
uint64_t basic_rng_t<Engine>::reseed()
{
  const uint64_t s = engine.next();
#ifdef RNG_STREAM_DEBUG
  ++n;
  std::cout << fmt::format( "[RNG] fn=reseed() engine={} n={} next={}",
                           engine.name(), n, s ) << std::endl;
#endif
  seed( s );
  reset();
  return s;
}

/// Reset state
template <typename Engine>
void basic_rng_t<Engine>::reset()
{
  gauss_pair_value = 0;
  gauss_pair_use = false;
#ifdef RNG_STREAM_DEBUG
  n = 0U;
#endif
}

// ==========================================================================
// Probability Distributions
// ==========================================================================

/// Uniform distribution in range [0..1)
template <typename Engine>
inline double basic_rng_t<Engine>::real()
{
  /// MAGIC! http://en.wikipedia.org/wiki/Double-precision_floating-point_format
  uint64_t ui64 = engine.next();
#ifdef RNG_STREAM_DEBUG
  auto u64raw = ui64;
#endif
  ui64 &= 0x000fffffffffffff;
  ui64 |= 0x3ff0000000000000;
  union { uint64_t ui64; double d; } u;
  u.ui64 = ui64;
#ifdef RNG_STREAM_DEBUG
  ++n;
  std::cout << fmt::format( "[RNG] fn=real() engine={} n={} next={} value={}",
                           engine.name(), n, u64raw, u.d - 1.0 ) << std::endl;
#endif
  return u.d - 1.0;
}

/// Bernoulli Distribution
template <typename Engine>
bool basic_rng_t<Engine>::roll( double chance )
{
  if ( chance <= 0 ) return false;
  if ( chance >= 1 ) return true;
  return real() < chance;
}

/// Uniform distribution in the range [min..max)
template <typename Engine>
double basic_rng_t<Engine>::range( double min, double max )
{
  assert( min <= max );
  return min + real() * ( max - min );
}

/**
 * @brief Gaussian Distribution
 *
 * This code adapted from ftp://ftp.taygeta.com/pub/c/boxmuller.c
 * Implements the Polar form of the Box-Muller Transformation
 *
 * (c) Copyright 1994, Everett F. Carter Jr.
 *     Permission is granted by the author to use
 *     this software for any application provided this
 *     copyright notice is preserved.
 */
template <typename Engine>
double basic_rng_t<Engine>::gauss( double mean, double stddev )
{
  assert(stddev >= 0 && "Calling gauss with negative stddev");

  if ( stddev == 0 )
    return mean;

  if ( gauss_pair_use )
  {
    gauss_pair_use = false;
    return mean + gauss_pair_value * stddev;
  }

  double x1, x2, w, y1, y2;
  do
  {
    x1 = 2.0 * real() - 1.0;
    x2 = 2.0 * real() - 1.0;
    w = x1 * x1 + x2 * x2;
  }
  while ( w >= 1.0 || w == 0.0 );

  w = std::sqrt( ( -2.0 * std::log( w ) ) / w );
  y1 = x1 * w;
  y2 = x2 * w;

  gauss_pair_value = y2;
  gauss_pair_use = true;

  return mean + y1 * stddev;
}

template <typename Engine>
double basic_rng_t<Engine>::gauss_ab( double mean, double stddev, double min, double max )
{
  assert( stddev >= 0.0 && "Stddev must be non-negative." );
  assert( min < max && "Minimum must be less than maximum." );
  assert( mean >= min && mean <= max && "Mean must be contained within the interval [min, max]" );

  if ( stddev == 0.0 )
    return mean;

  double min_cdf      = stdnormal_cdf( ( min - mean ) / stddev );
  double max_cdf      = stdnormal_cdf( ( max - mean ) / stddev );
  double uniform      = real();
  double rescaled_cdf = min_cdf + uniform * ( max_cdf - min_cdf );
  return mean + stddev * stdnormal_inv( rescaled_cdf );
}

template <typename Engine>
double basic_rng_t<Engine>::gauss_a( double mean, double stddev, double min )
{
  return gauss_ab( mean, stddev, min, std::numeric_limits<double>::infinity() );
}

template <typename Engine>
double basic_rng_t<Engine>::gauss_b( double mean, double stddev, double max )
{
  return gauss_ab( mean, stddev, -std::numeric_limits<double>::infinity(), max );
}

/// Exponential Distribution
template <typename Engine>
double basic_rng_t<Engine>::exponential( double nu )
{
  double x;
  do { x = real(); } while ( x >= 1.0 ); // avoid ln(k) where k <= 0. this should be guaranteed by `real()`, but just in case
  // TODO: check if `real()` generates any non-positive values
  return - std::log( 1 - x ) * nu;
}

/// Exponentially Modified Gaussian Distribution
template <typename Engine>
double basic_rng_t<Engine>::exgauss( double gauss_mean, double gauss_stddev, double exp_nu )
{
  return gauss( gauss_mean, gauss_stddev ) + exponential( exp_nu );
}

/// Timespan uniform distribution in the range [min..max)
template <typename Engine>
timespan_t basic_rng_t<Engine>::range( timespan_t min, timespan_t max )
{
  return timespan_t::from_native( range( timespan_t::to_native( min ), timespan_t::to_native( max ) ) );
}

/// Timespan Gaussian Distribution
template <typename Engine>
timespan_t basic_rng_t<Engine>::gauss( timespan_t mean, timespan_t stddev )
{
  return timespan_t::from_native( gauss( static_cast<double>( timespan_t::to_native( mean ) ),
                                         static_cast<double>( timespan_t::to_native( stddev ) ) ) );
}

/// Timespan exponentially Modified Gaussian Distribution
template <typename Engine>
timespan_t basic_rng_t<Engine>::exgauss( timespan_t mean, timespan_t stddev, timespan_t nu )
{
  return timespan_t::from_native( exgauss( static_cast<double>( timespan_t::to_native( mean   ) ),
                                           static_cast<double>( timespan_t::to_native( stddev ) ),
                                           static_cast<double>( timespan_t::to_native( nu ) ) ) );
}

/// RNG Engines

/**
 * @brief XORSHIFT-128 Random Number Generator
 *
 * All credit goes to Sebastiano Vigna (vigna@acm.org) @2014
 * http://xorshift.di.unimi.it/
 */
struct xorshift128_t
{
  uint64_t next() noexcept;
  void seed( uint64_t start ) noexcept;
  const char* name() const noexcept;
private:
  std::array<uint64_t, 2> s;
};

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
struct xoshiro256plus_t
{
  uint64_t next() noexcept;
  void seed( uint64_t start ) noexcept;
  const char* name() const noexcept;
private:
  std::array<uint64_t, 4> s;
};

/**
 * @brief XORSHIFT-1024 Random Number Generator
 *
 * All credit goes to Sebastiano Vigna (vigna@acm.org) @2014
 * http://xorshift.di.unimi.it/
 */
struct xorshift1024_t
{
  uint64_t next() noexcept;
  void seed( uint64_t start ) noexcept;
  const char* name() const noexcept;
private:
  std::array<uint64_t, 16> s;
  int p;
};

// "Default" rng
// Explicitly *NOT* a type alias to allow forward declaraions
struct rng_t : public basic_rng_t<xoshiro256plus_t> {};

} // rng

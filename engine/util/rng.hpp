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

double stdnormal_cdf( double u );
double stdnormal_inv( double u );

/**\ingroup SC_RNG
 * @brief Random number generator wrapper around an rng engine
 *
 * Implements different distribution outputs ( uniform, gauss, etc. )
 */
template <typename Engine>
struct basic_rng_t
{
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

  /// Uniform distribution in the range [0.0..max)
  double range( double max );

  /// Uniform distribution in the range [min..max)
  template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
  T range( T min, T max )
  {
    return static_cast<T>( std::floor( range( static_cast<double>( min ), static_cast<double>( max ) ) ) );
  }

  /// Uniform distribution in the range [0.0..max)
  template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
  T range( T max )
  {
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

  template <typename T, typename = std::enable_if_t<!std::numeric_limits<T>::is_signed>>
  T gauss( T mean, T stddev )
  {
    return static_cast<T>( gauss_a( static_cast<double>( mean ), static_cast<double>( stddev ), 0.0 ) );
  }

  template <typename T, typename = std::enable_if_t<!std::numeric_limits<T>::is_signed>>
  T exgauss( T gauss_mean, T gauss_stddev, T exp_nu )
  {
    return static_cast<T>( gauss_a( static_cast<double>( gauss_mean ), static_cast<double>( gauss_stddev ), 0.0 ) +
                           exponential( static_cast<double>( exp_nu ) ) );
  }

  /// Timespan uniform distribution in the range [min..max)
  timespan_t range( timespan_t min, timespan_t max );

  /// Timespan Gaussian Distribution
  timespan_t gauss( timespan_t mean, timespan_t stddev );
  timespan_t gauss_ab( timespan_t mean, timespan_t stddev, timespan_t min, timespan_t max );
  timespan_t gauss_a( timespan_t mean, timespan_t stddev, timespan_t min );
  timespan_t gauss_b( timespan_t mean, timespan_t stddev, timespan_t max );

  /// Timespan Exponential Distribution
  timespan_t exponential( timespan_t nu );

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

/// Uniform distribution in the range [0.0..max)
template <typename Engine>
double basic_rng_t<Engine>::range( double max )
{
  assert( 0.0 <= max );
  return real() * max;
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
  assert( min <= max && "Minimum must be less than or equal to maximum." );

  if ( stddev == 0.0 )
  {
    assert( mean >= min && mean <= max && "Mean must be contained within the interval [min, max]" );
    return mean;
  }

  if ( min == max )
    return min;

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
  return timespan_t::from_native( gauss_a( static_cast<double>( timespan_t::to_native( mean ) ),
                                           static_cast<double>( timespan_t::to_native( stddev ) ),
                                           0.0 ) );
}

template <typename Engine>
timespan_t basic_rng_t<Engine>::gauss_ab( timespan_t mean, timespan_t stddev, timespan_t min, timespan_t max )
{
  return timespan_t::from_native( gauss_ab( static_cast<double>( timespan_t::to_native( mean ) ),
                                            static_cast<double>( timespan_t::to_native( stddev ) ),
                                            static_cast<double>( timespan_t::to_native( min ) ),
                                            static_cast<double>( timespan_t::to_native( max ) ) ) );
}

template <typename Engine>
timespan_t basic_rng_t<Engine>::gauss_a( timespan_t mean, timespan_t stddev, timespan_t min )
{
  return timespan_t::from_native( gauss_a( static_cast<double>( timespan_t::to_native( mean ) ),
                                           static_cast<double>( timespan_t::to_native( stddev ) ),
                                           static_cast<double>( timespan_t::to_native( min ) ) ) );
}

template <typename Engine>
timespan_t basic_rng_t<Engine>::gauss_b( timespan_t mean, timespan_t stddev, timespan_t max )
{
  return timespan_t::from_native( gauss_ab( static_cast<double>( timespan_t::to_native( mean ) ),
                                            static_cast<double>( timespan_t::to_native( stddev ) ),
                                            0.0,
                                            static_cast<double>( timespan_t::to_native( max ) ) ) );
}

template <typename Engine>
timespan_t basic_rng_t<Engine>::exponential( timespan_t nu )
{
  return timespan_t::from_native( exponential( static_cast<double>( timespan_t::to_native( nu ) ) ) );
}

/// Timespan exponentially Modified Gaussian Distribution
template <typename Engine>
timespan_t basic_rng_t<Engine>::exgauss( timespan_t mean, timespan_t stddev, timespan_t nu )
{
  return gauss( mean, stddev ) + exponential( nu );
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

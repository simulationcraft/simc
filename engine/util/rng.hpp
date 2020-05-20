// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

/*! \file rng.hpp */
/*! \defgroup SC_RNG Random Number Generator */

#include "config.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

#include "sc_timespan.hpp"

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

  /// Uniform distribution in range [0,1]
  double real();

  /// Bernoulli Distribution
  bool roll( double chance );

  /// Uniform distribution in the range [min max]
  double range( double min, double max );

  template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
  T range( T min, T max ) {
    return static_cast<T>(range(static_cast<double>(min), static_cast<double>(max)));
  }

  template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
  T range( T max ) {
    return range<T>( T{}, max );
  }

  /// Gaussian Distribution
  double gauss( double mean, double stddev, bool truncate_low_end = false );

  /// Exponential Distribution
  double exponential( double nu );

  /// Exponentially Modified Gaussian Distribution
  double exgauss( double gauss_mean, double gauss_stddev, double exp_nu );

  /// Timespan uniform distribution in the range [min max]
  timespan_t range( timespan_t min, timespan_t max );

  /// Timespan Gaussian Distribution
  timespan_t gauss( timespan_t mean, timespan_t stddev );

  /// Timespan exponentially Modified Gaussian Distribution
  timespan_t exgauss( timespan_t mean, timespan_t stddev, timespan_t nu );

private:
  Engine engine;
  // Allow re-use of unused ( but necessary ) random number of a previous call to gauss()
  double gauss_pair_value = 0.0;
  bool   gauss_pair_use = false;
};

/// Reseed using current state
template <typename Engine>
uint64_t basic_rng_t<Engine>::reseed()
{
  const uint64_t s = engine.next();
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
}

// ==========================================================================
// Probability Distributions
// ==========================================================================

/// Uniform distribution in range [0,1]
template <typename Engine>
inline double basic_rng_t<Engine>::real()
{
  /// MAGIC! http://en.wikipedia.org/wiki/Double-precision_floating-point_format
  uint64_t ui64 = engine.next();
  ui64 &= 0x000fffffffffffff;
  ui64 |= 0x3ff0000000000000;
  union { uint64_t ui64; double d; } u;
  u.ui64 = ui64;
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

/// Uniform distribution in the range [min max]
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
double basic_rng_t<Engine>::gauss( double mean, double stddev, bool truncate_low_end )
{
  assert(stddev >= 0 && "Calling gauss with negative stddev");

  double z;

  if ( stddev != 0 )
  {
    if ( gauss_pair_use )
    {
      z = gauss_pair_value;
      gauss_pair_use = false;
    }
    else
    {
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

      z = y1;
      gauss_pair_value = y2;
      gauss_pair_use = true;
    }
  }
  else
    z = 0.0;

  double result = mean + z * stddev;

  // True gaussian distribution can of course yield any number at some probability.  So truncate on the low end.
  if ( truncate_low_end && result < 0 )
    result = 0;

  return result;
}

/// Exponential Distribution
template <typename Engine>
double basic_rng_t<Engine>::exponential( double nu )
{
  double x;
  do { x = real(); } while ( x >= 1.0 ); // avoid ln(0)
  return - std::log( 1 - x ) * nu;
}

/// Exponentially Modified Gaussian Distribution
template <typename Engine>
double basic_rng_t<Engine>::exgauss( double gauss_mean, double gauss_stddev, double exp_nu )
{
  double v = gauss( gauss_mean, gauss_stddev ) + exponential( exp_nu );
  return v > 0.0 ? v : 0.0;
}

/// Timespan uniform distribution in the range [min max]
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
  uint64_t s[2];
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
  uint64_t s[4];
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
  uint64_t s[16];
  int p;
};

// "Default" rng
// Explicitly *NOT* a type alias to allow forward declaraions
struct rng_t : public basic_rng_t<xoshiro256plus_t> {};

double stdnormal_cdf( double );
double stdnormal_inv( double );

} // rng

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

/*! \file rng.hpp */
/*! \defgroup SC_RNG Random Number Generator */

#include "config.hpp"
#include <memory>
#include "sc_timespan.hpp"

/** \ingroup SC_RNG
 * @brief Random number generation
 */
namespace rng {
/**\ingroup SC_RNG
 * @brief Random number generator base class
 *
 * Implements different rng-engines, selectable through a factory,
 * as well as different distribution outputs ( uniform, gauss, etc. )
 */
struct rng_t
{
  /// rng engines
  enum type_e { DEFAULT, MURMURHASH, SFMT, STD, TINYMT, XORSHIFT64, XORSHIFT128, XORSHIFT1024 };

  virtual ~rng_t() {}
  /// name of rng engine
  virtual const char* name() const = 0;
  /// seed rng engine
  virtual void seed( uint64_t start ) = 0;
  /// uniform distribution in range [0,1]
  virtual double real() = 0;
  virtual uint64_t reseed();
  virtual void reset();

  bool roll( double chance );
  double range( double min, double max );
  double gauss( double mean, double stddev, bool truncate_low_end = false );
  double exponential( double nu );
  double exgauss( double gauss_mean, double gauss_stddev, double exp_nu );
  timespan_t range( timespan_t min, timespan_t max );
  timespan_t gauss( timespan_t mean, timespan_t stddev );
  timespan_t exgauss( timespan_t mean, timespan_t stddev, timespan_t nu );
protected:
  rng_t();
private:
  // Allow re-use of unused ( but necessary ) random number of a previous call to gauss()  
  double gauss_pair_value; 
  bool   gauss_pair_use;

};

std::unique_ptr<rng_t> create( rng_t::type_e = rng_t::DEFAULT );
rng_t::type_e parse_type( const std::string& name );

double stdnormal_cdf( double );
double stdnormal_inv( double );

} // rng

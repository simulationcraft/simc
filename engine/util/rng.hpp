// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef RNG_HPP
#define RNG_HPP

#include "config.hpp"

#if USE_TR1_NAMESPACE
// Use TR1
#include <tr1/memory>
namespace std {using namespace tr1; }
#else
// Use C++11
#include <memory>
#endif

#include "sc_timespan.hpp"

// ==========================================================================
// Random Number Generator
// ==========================================================================

struct rng_t
{
  enum type_e { DEFAULT, MURMURHASH, SFMT, STD, TINYMT, XORSHIFT64, XORSHIFT128, XORSHIFT1024 };

  virtual const char* name() = 0;
  virtual void seed( uint64_t start ) = 0;
  virtual double real() = 0;

  static rng_t* create( type_e = DEFAULT );
  static type_e parse_type( const std::string& name );

  static double stdnormal_cdf( double );
  static double stdnormal_inv( double );

  // bernoulli distribution
  bool roll( double chance );

  // uniform distribution in the range [min max]
  double range( double min, double max );

  // gaussian distribution
  double gauss( double mean, double stddev, bool truncate_low_end = false );

  // exponential distribution
  double exponential( double nu );

  // Exponentially modified Gaussian distribution
  double exgauss( double gauss_mean, double gauss_stddev, double exp_nu );
  
  // Reseed using current state.  Helpful for replaying from a specific point.
  uint64_t reseed();

  // Convenience timespan_t helper functions
  timespan_t range( timespan_t min, timespan_t max )
  {
    return timespan_t::from_native( range( static_cast<double>( timespan_t::to_native( min ) ),
                                           static_cast<double>( timespan_t::to_native( max ) ) ) );
  }
  timespan_t gauss( timespan_t mean, timespan_t stddev )
  {
    return timespan_t::from_native( gauss( static_cast<double>( timespan_t::to_native( mean ) ),
                                           static_cast<double>( timespan_t::to_native( stddev ) ) ) );
  }
  timespan_t exgauss( timespan_t mean, timespan_t stddev, timespan_t nu )
  {
    return timespan_t::from_native( exgauss( static_cast<double>( timespan_t::to_native( mean   ) ),
					     static_cast<double>( timespan_t::to_native( stddev ) ),
					     static_cast<double>( timespan_t::to_native( nu ) ) ) );
  }

  // Allow re-use of unused ( but necessary ) random number of a previous call to gauss()  
  double gauss_pair_value; 
  bool   gauss_pair_use;

  rng_t() : gauss_pair_value( 0.0 ), gauss_pair_use( false ) {}
  virtual ~rng_t() {}
};

#endif // RNG_HPP

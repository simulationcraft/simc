// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef RNG_HPP
#define RNG_HPP

#include <memory>

namespace rng {

// ==========================================================================
// Random Number Engine / Generator
// ==========================================================================

/* interface class
 */
class engine_t
{
public:
  double operator()()
  { return real(); }

  void seed( unsigned start )
  { _seed(start); }

  enum type // Enumeration of convenient rng engines
  { BEST, MERSENNE_TWISTER, STD, };

  // Factory function
  static std::shared_ptr<engine_t> create( engine_t::type = BEST);

  virtual ~engine_t() {}
private:
  // Override those two methods to create your derived rng engine
  virtual void _seed( unsigned start ) = 0;
  virtual double real() = 0;
};

// ==========================================================================
// Probability Distribution
// ==========================================================================

/* Offers some common probability distributions like
 * - uniform distribution
 * - gaussian distribution
 * - exponential distribution
 * - exgaussian distribution
 * - bernoulli distribution ( roll )
 *
 * It uses a RNG_GENERATOR to obtain uniformly distributed random numbers
 * in the range [0 1], which are then transformed to build the desired distribution.
 */

class distribution_t
{
public:
  distribution_t( std::shared_ptr<engine_t> );

  // Seed rng generator
  void seed( unsigned value );

  // obtain a random value from the rng generator
  double real();

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

private:
  // This is the RNG engine/generator used to obtain random numbers
  // Treat it as a black box which gives numbers between 0 and 1 through its () operator.
  std::shared_ptr<engine_t> _engine;

  double gauss_pair_value; // allow re-use of unused ( but necessary ) random number of a previous call to gauss()
  bool   gauss_pair_use;
};

double stdnormal_cdf( double );
double stdnormal_inv( double );


/* Derived SimulationCraft RNG Distribution class with some timespan_t helper function
 * Uses engine_t::BEST as default engine
 */
class sc_distribution_t : public distribution_t
{
public:
  typedef distribution_t base_t;
  sc_distribution_t( engine_t::type type = engine_t::BEST ) :
    base_t( engine_t::create( type ) ) {}

  // Make sure base functions aren't hidden, because overload resolution does not search for them
  using base_t::range;
  using base_t::gauss;
  using base_t::exgauss;

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
    return timespan_t::from_native(
             exgauss( static_cast<double>( timespan_t::to_native( mean   ) ),
                      static_cast<double>( timespan_t::to_native( stddev ) ),
                      static_cast<double>( timespan_t::to_native( nu ) )
                    )
           );
  }

};

}


#endif // RNG_HPP

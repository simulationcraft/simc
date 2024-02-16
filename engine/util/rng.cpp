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
  int _[] = { ( std::invoke( std::forward<F>( f ), std::get<I>( std::forward<Tuple>(t) ) ), 0 )... };
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

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//#include "dbc/dbc.hpp"

#include <ctime>
#include <stdint.h>
#include <string>
#include "rng.hpp"

// Pseudo-Random Number Generation ==========================================

#if defined(__SSE2__) || ( defined( SC_VS ) && ( defined(_M_X64) || ( defined(_M_IX86_FP) && _M_IX86_FP >= 2 ) ) )
#  define RNG_USE_SSE2
#endif

#if defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32 )
#  if defined(RNG_USE_SSE2)
#    if defined( __MINGW__ ) || defined( __MINGW32__ )
       // <HACK> Include these headers (in this order) to avoid
       // an order-of-inclusion bug with MinGW headers.
#      include <stdlib.h>
       // Workaround MinGW header bug: http://sourceforge.net/tracker/?func=detail&atid=102435&aid=2962480&group_id=2435
       extern "C" {
#        include <emmintrin.h>
       }
#      include <malloc.h>
       // </HACK>
#    else
#      include <emmintrin.h>
#    endif
#  endif
#else
#  if defined(RNG_USE_SSE2)
#    include <emmintrin.h>
#    include <mm_malloc.h>
#  endif
#endif

#include <random>

namespace rng {

namespace {

/// MAGIC! http://en.wikipedia.org/wiki/Double-precision_floating-point_format
double convert_to_double_0_1( uint64_t ui64 )
{
  ui64 &= 0x000fffffffffffff;
  ui64 |= 0x3ff0000000000000;
  union { uint64_t ui64; double d; } u;
  u.ui64 = ui64;
  return u.d - 1.0;
}


/**
 * @brief STL Mersenne twister MT19937
 *
 * This integrates a C++11 random number generator into our native rng_t concept
 * The advantage of this container is the extremely small code, with nearly no
 * maintenance cost.
 * Unfortunately, it is slower than the dsfmt implementation.
 */
struct rng_mt_cxx11_t : public rng_t
{
  std::mt19937 engine; // Mersenne twister MT19937
  std::uniform_real_distribution<double> dist;

  rng_mt_cxx11_t() : dist(0,1) {}

  virtual const char* name() const override { return "mt_cxx11"; }

  virtual void seed( uint64_t start ) override
  { 
    engine.seed( (unsigned) start ); 
  }

  virtual double real() override
  { 
    return dist( engine );
  }
};

struct rng_mt_cxx11_64_t : public rng_t
{
  std::mt19937_64 engine; // Mersenne twister MT19937

  rng_mt_cxx11_64_t() = default;

  virtual const char* name() const override { return "mt_cxx11_64"; }

  virtual void seed( uint64_t start ) override
  {
    engine.seed( start );
  }

  virtual double real() override
  {
    return convert_to_double_0_1(engine());
  }
};


/**
 * @brief MURMURHASH3 Avalanche Seed Munger
 *
 * All credit goes to https://code.google.com/p/smhasher
 */
struct rng_murmurhash_t : public rng_t
{
  uint64_t x; /* The state must be seeded with a nonzero value. */

  uint64_t next() 
  {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    return x ^= x >> 33;
  }

  virtual const char* name() const override { return "murmurhash3"; }

  virtual void seed( uint64_t start ) override
  { 
    assert( start != 0 );
    x = start;
  }

  virtual double real() override
  { 
    return convert_to_double_0_1( next() );
  }
};


/**
 * @brief XORSHIFT-64 Random Number Generator
 *
 * All credit goes to Sebastiano Vigna (vigna@acm.org) @2014
 * http://xorshift.di.unimi.it/
 */
struct rng_xorshift64_t : public rng_t
{
  uint64_t x; /* The state must be seeded with a nonzero value. */

  uint64_t next() 
  {
    x ^= x >> 12; // a
    x ^= x << 25; // b
    x ^= x >> 27; // c
    return x * 2685821657736338717LL;
  }

  virtual const char* name() const override { return "xorshift64"; }

  virtual void seed( uint64_t start ) override
  { 
    assert( start != 0 );
    x = start;
  }

  virtual double real() override
  { 
    return convert_to_double_0_1( next() );
  }
};


/**
 * @brief XORSHIFT-128 Random Number Generator
 *
 * All credit goes to Sebastiano Vigna (vigna@acm.org) @2014
 * http://xorshift.di.unimi.it/
 */
struct rng_xorshift128_t : public rng_t
{
  uint64_t s[ 2 ];

  uint64_t next() 
  { 
    uint64_t s1 = s[ 0 ];
    const uint64_t s0 = s[ 1 ];
    s[ 0 ] = s0;
    s1 ^= s1 << 23; // a
    return ( s[ 1 ] = ( s1 ^ s0 ^ ( s1 >> 17 ) ^ ( s0 >> 26 ) ) ) + s0; // b, c
  }

  virtual const char* name() const override { return "xorshift128"; }

  virtual void seed( uint64_t start ) override
  { 
    rng_murmurhash_t mmh;
    mmh.seed( start );
    for( int i=0; i<16; i++ ) mmh.next();
    s[ 0 ] = mmh.next();
    s[ 1 ] = mmh.next();
  }

  virtual double real() override
  { 
    return convert_to_double_0_1( next() );
  }
};


/**
 * @brief XORSHIFT-1024 Random Number Generator
 *
 * All credit goes to Sebastiano Vigna (vigna@acm.org) @2014
 * http://xorshift.di.unimi.it/
 */
struct rng_xorshift1024_t : public rng_t
{
  uint64_t s[ 16 ]; 
  int p;

  uint64_t next()
  { 
    uint64_t s0 = s[ p ];
    uint64_t s1 = s[ p = ( p + 1 ) & 15 ];
    s1 ^= s1 << 31; // a
    s1 ^= s1 >> 11; // b
    s0 ^= s0 >> 30; // c
    return ( s[ p ] = s0 ^ s1 ) * 1181783497276652981LL; 
  }

  virtual const char* name() const override { return "xorshift1024"; }

  virtual void seed( uint64_t start ) override
  { 
    rng_xorshift64_t xs64;
    xs64.seed( start );
    for(auto & elem : s) elem=xs64.next();
    p = 0;
  }

  virtual double real() override
  { 
    return convert_to_double_0_1( next() );
  }
};


/**
 * @brief SIMD oriented Fast Mersenne Twister(SFMT) pseudorandom number generator
 *
 * WARNING: ALWAYS ALLOCATE THROUGH NEW
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 * URL: http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * The new BSD License is applied to this software.
 */
struct rng_sfmt_t : public rng_t
{
  /** 128-bit data structure */
  union w128_t
  {
#ifdef RNG_USE_SSE2
    __m128i si;
    __m128d sd;
#endif
    uint64_t u[2];
    uint32_t u32[4];
    double d[2];
  };

  static const int DSFMT_MEXP = 19937; // Mersenne Exponent. The period of the sequence is a multiple of 2^MEXP-1.
  static const int DSFMT_N = ( ( DSFMT_MEXP - 128 ) / 104 + 1 );
  static const int DSFMT_N64 = ( DSFMT_N * 2 );

  static const uint64_t DSFMT_LOW_MASK = 0x000FFFFFFFFFFFFFULL;
  static const uint64_t DSFMT_HIGH_CONST = 0x3FF0000000000000ULL;
  static const int DSFMT_SR = 12;

  static const int DSFMT_POS1 = 117;
  static const int DSFMT_SL1 = 19;
  static const uint64_t DSFMT_MSK1 = 0x000ffafffffffb3fULL;
  static const uint64_t DSFMT_MSK2 = 0x000ffdfffc90fffdULL;
  static const uint64_t DSFMT_FIX1 = 0x90014964b32f4329ULL;
  static const uint64_t DSFMT_FIX2 = 0x3b8d12ac548a7c7aULL;
  static const uint64_t DSFMT_PCV1 = 0x3d84e1ac0dc82880ULL;
  static const uint64_t DSFMT_PCV2 = 0x0000000000000001ULL;

  /** the 128-bit internal state array */
  struct dsfmt_t
  {
    w128_t status[DSFMT_N + 1];
    int idx;
  };

  /** global data */
  dsfmt_t dsfmt_global_data;

  /**
   * This function represents the recursion formula.
   * @param r output 128-bit
   * @param a a 128-bit part of the internal state array
   * @param b a 128-bit part of the internal state array
   * @param d a 128-bit part of the internal state array (I/O)
   */
#ifdef RNG_USE_SSE2
  static void do_recursion( w128_t *r, const w128_t *a, const w128_t *b, w128_t *d )
  {
    const int SSE2_SHUFF = 0x1b;

    /** mask data for sse2 */
    static const union { uint64_t u[2]; __m128i m; } sse2_param_mask = {{ DSFMT_MSK1, DSFMT_MSK2 }};

    __m128i v, w, x, y, z;

    x = a->si;
    z = _mm_slli_epi64( x, DSFMT_SL1 );
    y = _mm_shuffle_epi32( d -> si, SSE2_SHUFF );
    z = _mm_xor_si128( z, b -> si );
    y = _mm_xor_si128( y, z );

    v = _mm_srli_epi64( y, DSFMT_SR );
    w = _mm_and_si128( y, sse2_param_mask.m );
    v = _mm_xor_si128( v, x );
    v = _mm_xor_si128( v, w );
    r->si = v;
    d->si = y;
  }
#else
  static void do_recursion( w128_t *r, const w128_t *a, const w128_t *b, w128_t *d )
  {
    uint64_t t0, t1, L0, L1;

    t0 = a->u[0];
    t1 = a->u[1];
    L0 = d->u[0];
    L1 = d->u[1];
    d->u[0] = ( t0 << DSFMT_SL1 ) ^ ( L1 >> 32 ) ^ ( L1 << 32 ) ^ b->u[0];
    d->u[1] = ( t1 << DSFMT_SL1 ) ^ ( L0 >> 32 ) ^ ( L0 << 32 ) ^ b->u[1];
    r->u[0] = ( d->u[0] >> DSFMT_SR ) ^ ( d->u[0] & DSFMT_MSK1 ) ^ t0;
    r->u[1] = ( d->u[1] >> DSFMT_SR ) ^ ( d->u[1] & DSFMT_MSK2 ) ^ t1;
  }
#endif

  /**
   * This function fills the internal state array with double precision
   * floating point pseudorandom numbers of the IEEE 754 format.
   * @param dsfmt dsfmt state vector.
   */
  void dsfmt_gen_rand_all( dsfmt_t *dsfmt )
  {
    int i;
    w128_t lung;

    lung = dsfmt->status[DSFMT_N];
    do_recursion( &dsfmt->status[0], &dsfmt->status[0],
                  &dsfmt->status[DSFMT_POS1], &lung );
    for ( i = 1; i < DSFMT_N - DSFMT_POS1; i++ )
    {
      do_recursion( &dsfmt->status[i], &dsfmt->status[i],
                    &dsfmt->status[i + DSFMT_POS1], &lung );
    }
    for ( ; i < DSFMT_N; i++ )
    {
      do_recursion( &dsfmt->status[i], &dsfmt->status[i],
                    &dsfmt->status[i + DSFMT_POS1 - DSFMT_N], &lung );

    }
    dsfmt->status[DSFMT_N] = lung;
  }

  /**
   * This function initializes the internal state array to fit the IEEE
   * 754 format.
   * @param dsfmt dsfmt state vector.
   */
  void initial_mask( dsfmt_t *dsfmt )
  {
    int i;
    uint64_t *psfmt;

    psfmt = &dsfmt->status[0].u[0];
    for ( i = 0; i < DSFMT_N * 2; i++ )
    {
      psfmt[i] = ( psfmt[i] & DSFMT_LOW_MASK ) | DSFMT_HIGH_CONST;
    }
  }

  /**
   * This function certificate the period of 2^{SFMT_MEXP}-1.
   * @param dsfmt dsfmt state vector.
   */
  void period_certification( dsfmt_t *dsfmt )
  {
    uint64_t pcv[2] = {DSFMT_PCV1, DSFMT_PCV2};
    uint64_t tmp[2];
    uint64_t inner;
    int i;

    tmp[0] = ( dsfmt->status[DSFMT_N].u[0] ^ DSFMT_FIX1 );
    tmp[1] = ( dsfmt->status[DSFMT_N].u[1] ^ DSFMT_FIX2 );

    inner = tmp[0] & pcv[0];
    inner ^= tmp[1] & pcv[1];
    for ( i = 32; i > 0; i >>= 1 )
    {
      inner ^= inner >> i;
    }
    inner &= 1;
    /* check OK */
    if ( inner == 1 )
    {
      return;
    }
    /* check NG, and modification */
    dsfmt->status[DSFMT_N].u[1] ^= 1;
    return;
  }

  int idxof( int i )
  {
    return i;
  }

  /**
   * This function generates and returns unsigned 32-bit integer.
   * This is slower than SFMT, only for convenience usage.
   * dsfmt_init_gen_rand() or dsfmt_init_by_array() must be called
   * before this function.
   * @param dsfmt dsfmt internal state date
   * @return double precision floating point pseudorandom number
   */
  uint32_t dsfmt_genrand_uint32(dsfmt_t *dsfmt) {
      uint32_t r;
      uint64_t *psfmt64 = &dsfmt->status[0].u[0];

      if (dsfmt->idx >= DSFMT_N64) {
          dsfmt_gen_rand_all(dsfmt);
          dsfmt->idx = 0;
      }
      r = psfmt64[dsfmt->idx++] & 0xffffffffU;
      return r;
  }

  /**
   * This function initializes the internal state array with a 32-bit
   * integer seed.
   * @param dsfmt dsfmt state vector.
   * @param seed a 32-bit integer used as the seed.
   * @param mexp caller's mersenne expornent
   */
  void dsfmt_chk_init_gen_rand( dsfmt_t *dsfmt, uint32_t seed )
  {
    int i;
    uint32_t *psfmt;


    psfmt = &dsfmt->status[0].u32[0];
    psfmt[idxof( 0 )] = seed;
    for ( i = 1; i < ( DSFMT_N + 1 ) * 4; i++ )
    {
      psfmt[idxof( i )] = 1812433253UL
                          * ( psfmt[idxof( i - 1 )] ^ ( psfmt[idxof( i - 1 )] >> 30 ) ) + i;
    }
    initial_mask( dsfmt );
    period_certification( dsfmt );
    dsfmt->idx = DSFMT_N64;
  }

  double dsfmt_genrand_close_open( dsfmt_t *dsfmt )
  {
    double *psfmt64 = &dsfmt->status[0].d[0];

    if ( dsfmt->idx >= DSFMT_N64 )
    {
      dsfmt_gen_rand_all( dsfmt );
      dsfmt->idx = 0;
    }
    return psfmt64[dsfmt->idx++];
  }

#if defined(RNG_USE_SSE2)
  rng_sfmt_t()
  {
    // Validate proper alignment for SSE2 types.
    assert( ( uintptr_t ) dsfmt_global_data.status % 16 == 0 );
  }
  // 32-bit libraries typically align malloc chunks to sizeof(double) == 8.
  // This object needs to be aligned to sizeof(__m128d) == 16.
  static void* operator new( size_t size )
  { return _mm_malloc( size, sizeof( __m128d ) ); }
  static void operator delete( void* p )
  { return _mm_free( p ); }
#endif

  virtual const char* name() const override {
#ifdef RNG_USE_SSE2
    return "sse2-sfmt";
#else
    return "sfmt";
#endif
  }
  
  virtual void seed( uint64_t start ) override
  { 
    dsfmt_chk_init_gen_rand( &dsfmt_global_data, (uint32_t) start ); 
  }

  virtual double real() override
  { 
    return dsfmt_genrand_close_open( &dsfmt_global_data ) - 1.0; 
  }

  /**
   * Special implementation because dsfmt only allows 32bit seed
   */
  virtual uint64_t reseed() override
  {
    uint64_t s = dsfmt_genrand_uint32( &dsfmt_global_data );
    seed( s );
    reset();
    return s;
  }
};


/**
 * @brief Tiny Mersenne Twister only 127 bit internal state
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (The University of Tokyo)
 *
 * Copyright (C) 2011 Mutsuo Saito, Makoto Matsumoto,
 * Hiroshima University and The University of Tokyo.
 * All rights reserved.
 */
struct rng_tinymt_t : public rng_t
{
  static const uint64_t TINYMT64_SH0  = 12;
  static const uint64_t TINYMT64_SH1  = 11;
  static const uint64_t TINYMT64_SH8  = 8;
  static const uint64_t TINYMT64_MASK = uint64_t(0x7fffffffffffffff);

  uint64_t status[2];
  uint32_t mat1;
  uint32_t mat2;
  uint64_t tmat;

  void next_state() 
  {
    uint64_t x;
    status[0] &= TINYMT64_MASK;
    x = status[0] ^ status[1];
    x ^= x << TINYMT64_SH0;
    x ^= x >> 32;
    x ^= x << 32;
    x ^= x << TINYMT64_SH1;
    status[0] = status[1];
    status[1] = x;
    status[0] ^= -((int64_t)(x & 1)) & mat1;
    status[1] ^= -((int64_t)(x & 1)) & (((uint64_t)mat2) << 32);
  }

  double temper_conv_open()
  {
    uint64_t x;
    union {
  uint64_t u;
  double d;
    } conv;
    x = status[0] + status[1];
    x ^= status[0] >> TINYMT64_SH8;
    conv.u = ((x ^ (-((int64_t)(x & 1)) & tmat)) >> 12) | uint64_t(0x3ff0000000000001);
    return conv.d;
  }

  uint64_t ini_func2(uint64_t x) 
  {
    return (x ^ (x >> 59)) * uint64_t(58885565329898161);
  }

  void period_certification()
  {
    if ((status[0] & TINYMT64_MASK) == 0 && status[1] == 0) 
    {
      status[0] = 'T';
      status[1] = 'M';
    }
  }

  void init(uint64_t start) 
  {
    status[0] = start ^ ((uint64_t)mat1 << 32);
    status[1] = mat2 ^ tmat;
    for (int i = 1; i < 8; i++) 
    {
      status[i & 1] ^= i + uint64_t(6364136223846793005) * (status[(i - 1) & 1] ^ (status[(i - 1) & 1] >> 62));
    }
    period_certification();
  }

  virtual const char* name() const override { return "tinymt"; }

  virtual void seed( uint64_t start ) override
  {
    // mat1, mat2, and tmat are inputs to the engine
    // I am uncertain how to set them so we'll just grind the seed through MurmurHash.
    rng_murmurhash_t mmh;
    mmh.seed( start );
    for( int i=0; i<16; i++ ) mmh.next();
    do { mat1 = (uint32_t) mmh.next(); } while( mat1 == 0 );
    do { mat2 = (uint32_t) mmh.next(); } while( mat2 == 0 );
    tmat = mmh.next();
    init( start );
  }

  virtual double real() override
  {
    next_state();
    return temper_conv_open() - 1.0;
  }
};

} // unnamed

// ==========================================================================
// Probability Distributions
// ==========================================================================

/// Bernoulli Distribution
bool rng_t::roll( double chance )
{
  if ( chance <= 0 ) return false;
  if ( chance >= 1 ) return true;
  return real() < chance;
}

/// Uniform distribution in the range [min max]
double rng_t::range( double min, double max )
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
double rng_t::gauss( double mean, double stddev, bool truncate_low_end )
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

      w = sqrt( ( -2.0 * log( w ) ) / w );
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
double rng_t::exponential( double nu )
{
  double x;
  do { x = real(); } while ( x >= 1.0 ); // avoid ln(0)
  return - std::log( 1 - x ) * nu;
}

/// Exponentially Modified Gaussian Distribution
double rng_t::exgauss( double gauss_mean, 
           double gauss_stddev, 
           double exp_nu )
{ 
  return std::max( 0.0, gauss( gauss_mean, gauss_stddev ) + exponential( exp_nu ) ); 
}

/// timespan uniform distribution in the range [min max]
timespan_t rng_t::range( timespan_t min, timespan_t max )
{
  return timespan_t::from_native( range( static_cast<double>( timespan_t::to_native( min ) ),
                                         static_cast<double>( timespan_t::to_native( max ) ) ) );
}

/// timespan Gaussian Distribution
timespan_t rng_t::gauss( timespan_t mean, timespan_t stddev )
{
  return timespan_t::from_native( gauss( static_cast<double>( timespan_t::to_native( mean ) ),
                                         static_cast<double>( timespan_t::to_native( stddev ) ) ) );
}

/// tiemspan exponentially Modified Gaussian Distribution
timespan_t rng_t::exgauss( timespan_t mean, timespan_t stddev, timespan_t nu )
{
  return timespan_t::from_native( exgauss( static_cast<double>( timespan_t::to_native( mean   ) ),
             static_cast<double>( timespan_t::to_native( stddev ) ),
             static_cast<double>( timespan_t::to_native( nu ) ) ) );
}

/// Reseed using current state
uint64_t rng_t::reseed()
{
  union { uint64_t s; double d; } w;
  w.d = real();
  seed( w.s );
  reset();
  return w.s;
}

/// reset any state
void rng_t::reset()
{
  gauss_pair_value = 0;
  gauss_pair_use = false;
}

rng_t::rng_t() :
    gauss_pair_value( 0.0 ), gauss_pair_use( false )
{
}

// ==========================================================================
// RNG Engine Broker
// ==========================================================================

/// parse rng type from string
engine_type parse_type( const std::string& n )
{
  if( n == "murmurhash"   ) return engine_type::MURMURHASH;
  if( n == "sfmt"         ) return engine_type::SFMT;
  if( n == "std"          ) return engine_type::STD;
  if( n == "tinymt"       ) return engine_type::TINYMT;
  if( n == "xorshift64"   ) return engine_type::XORSHIFT64;
  if( n == "xorshift128"  ) return engine_type::XORSHIFT128;
  if( n == "xorshift1024" ) return engine_type::XORSHIFT1024;

  return engine_type::DEFAULT;
}

/**
 * Factory method to create a rng object with given rng-engine type
 */
std::unique_ptr<rng_t> create( engine_type t )
{
  switch( t )
  {
  case engine_type::MURMURHASH:
    return std::unique_ptr<rng_t>(new rng_murmurhash_t());

  case engine_type::STD:
    return std::unique_ptr<rng_t>(new rng_mt_cxx11_t());

  case engine_type::SFMT:
    return std::unique_ptr<rng_t>(new rng_sfmt_t());

  case engine_type::TINYMT:
    return std::unique_ptr<rng_t>(new rng_tinymt_t());

  case engine_type::XORSHIFT64:
    return std::unique_ptr<rng_t>(new rng_xorshift64_t());

  case engine_type::XORSHIFT128:
    return std::unique_ptr<rng_t>(new rng_xorshift128_t());

  case engine_type::XORSHIFT1024:
    return std::unique_ptr<rng_t>(new rng_xorshift1024_t());

  case engine_type::DEFAULT:
  default:
    break;
  }

  return create( engine_type::SFMT );
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

  double y, z;

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
    y = z * ( 1.0 / sqrt ( M_PI ) - y );
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

  double q, t, u;

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
  t = t * 2.0 / sqrt( M_PI ) * exp( u * u / 2 ); /* f(u)/df(u) */
  u = u - t / ( 1 + u * t / 2 );   /* Halley's method */

  return ( p > 0.5 ? -u : u );
}

} // rng
#ifdef UNIT_TEST
// Code to test functionality and performance of our RNG implementations

#include <iostream>
#include <iomanip>

namespace rng {
static int64_t milliseconds()
{
  return 1000 * clock() / CLOCKS_PER_SEC;
}

static void test_one( rng_t* rng, uint64_t n )
{
  int64_t start_time = milliseconds();

  double average = 0;
  for ( uint64_t i = 0; i < n; ++i )
  {
    double d = rng -> real();
    //std::cout << d << "\n";
    average += d;
  }

  average /= n;
  int64_t elapsed_cpu = milliseconds() - start_time;

  std::cout << n << " calls to rng::" << rng -> name() << "::real()"
            << ", average = " << std::setprecision( 8 ) << average
            << ", time = " << elapsed_cpu << " ms"
               ", numbers/sec = " << static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) << "\n\n";
}

static void test_seed( rng_t* rng, uint64_t n )
{
  int64_t start_time = milliseconds();

  for ( uint64_t i = 0; i < n; ++i )
  {
    rng -> seed( uint64_t( M_PI ) * n );
  }

  int64_t elapsed_cpu = milliseconds() - start_time;

  std::cout << n << " calls to rng::" << rng -> name() << "::seed()"
            << ", time = " << elapsed_cpu << " ms"
               ", numbers/sec = " << static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) << "\n\n";
}

// Monte-Carlo PI calculation.
static void monte_carlo( rng_t* rng, uint64_t n )
{
  int64_t start_time = milliseconds();

  uint64_t count_in_sphere = 0;
  for ( uint64_t i = 0; i < n; ++i )
  {
    double x1 = rng -> real();
    double x2 = rng -> real();
    if ( x1 * x1 + x2 * x2 < 1.0 )
      ++count_in_sphere;
  }
  int64_t elapsed_cpu = milliseconds() - start_time;

  std::cout << n << " runs for pi-calculation with " << rng -> name()
            << ". count in sphere = " << count_in_sphere
            << ", pi = " << std::setprecision( 21 ) << static_cast<double>( count_in_sphere ) * 4 / n
            << ", time = " << elapsed_cpu << " ms"
               ", numbers/sec = " << static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) << "\n\n";
}

} // namespace rng

int main( int /*argc*/, char** /*argv*/ )
{
  using namespace rng;
  rng_t* rng_mt_cxx11   = new rng_mt_cxx11_t();
  rng_t* rng_mt_cxx11_64   = new rng_mt_cxx11_64_t();
  rng_t* rng_murmurhash   = new rng_murmurhash_t();
  rng_t* rng_sfmt   = new rng_sfmt_t();
  rng_t* rng_tinymt = new rng_tinymt_t();
  rng_t* rng_xs128  = new rng_xorshift128_t();
  rng_t* rng_xs1024 = new rng_xorshift1024_t();

  std::random_device rd;
  uint64_t seed  = uint64_t(rd()) | (uint64_t(rd()) << 32);
  std::cout << "Seed: " << seed << "\n\n";
  //uint64_t seed = M_PI * 1000000;

  rng_mt_cxx11   -> seed( seed );
  rng_mt_cxx11_64   -> seed( seed );
  rng_murmurhash   -> seed( seed );
  rng_sfmt   -> seed( seed );
  rng_tinymt -> seed( seed );
  rng_xs128  -> seed( seed );
  rng_xs1024 -> seed( seed );

  uint64_t n = 100000000;

  test_one( rng_mt_cxx11,   n );
  test_one( rng_mt_cxx11_64,   n );
  test_one( rng_murmurhash,   n );
  test_one( rng_sfmt,   n );
  test_one( rng_tinymt, n );
  test_one( rng_xs128,  n );
  test_one( rng_xs1024, n );

  monte_carlo( rng_mt_cxx11,   n );
  monte_carlo( rng_murmurhash,   n );
  monte_carlo( rng_sfmt,   n );
  monte_carlo( rng_tinymt, n );
  monte_carlo( rng_xs128,  n );
  monte_carlo( rng_xs1024, n );

  test_seed( rng_mt_cxx11,   100000 );
  test_seed( rng_murmurhash,   100000 );
  test_seed( rng_sfmt,   100000 );
  test_seed( rng_tinymt, 100000 );
  test_seed( rng_xs128,  100000 );
  test_seed( rng_xs1024, 100000 );


  std::cout << "random device: min=" << rd.min() << " max=" << rd.max() << "\n\n";

  rng_t* rng = rng_tinymt;

  std::cout << "Testing " << rng -> name() << "\n\n";

  n /= 10;

  // double gauss
  {
    int64_t start_time = milliseconds();

    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
      average += rng -> gauss( 0, 1 );
    average /= n;
    int64_t elapsed_cpu = milliseconds() - start_time;

    std::cout << n << " calls to gauss(0,1): average = " << std::setprecision( 8 ) << average
              << ", time = " << elapsed_cpu << " ms\n\n";
  }

  // double exgauss
  {
    int64_t start_time = milliseconds();

    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
      average += 0.1 + rng -> exgauss( 0.3, 0.06, 0.25 );
    average /= n;
    int64_t elapsed_cpu = milliseconds() - start_time;

    std::cout << n << " calls to 0.1 + rng.exgauss( 0.3,0.06,0.25 );: average = " << std::setprecision( 8 ) << average
              << ", time = " << elapsed_cpu << " ms\n\n";
  }

  // exponential
  {
    int64_t start_time = milliseconds();

    double nu = 0.25;
    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
    {
      double result = rng -> exponential( nu );
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
    std::cout << "  " << rng -> real();
    if ( i % 10 == 0 )
      std::cout << '\n';
  }

  std::cout << "\ngauss mean=0, std_dev=1.0:\n";
  for ( unsigned i = 1; i <= 100; i++ )
  {
    std::cout << "  " << rng -> gauss( 0.0, 1.0 );
    if ( i % 10 == 0 )
      std::cout << '\n';
  }

  std::cout << "\nroll 30%:\n";
  for ( unsigned i = 1; i <= 100; i++ )
  {
    std::cout << "  " << rng -> roll( 0.30 );
    if ( i % 10 == 0 )
      std::cout << '\n';
  }

  std::cout << "\n";
  std::cout << "M_PI=" << M_PI << "\n";
  std::cout << "calls to rng::stdnormal_inv( double x )\n";
  std::cout << "x=0.975: " << rng::stdnormal_inv( 0.975 ) << " should be equal to 1.959964\n";
  std::cout << "x=0.995: " << rng::stdnormal_inv( 0.995 ) << " should be equal to 2.5758293\n";
}

#endif // UNIT_TEST

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

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

#if ( __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || ( defined(_MSC_VER) && _MSC_VER >= 1700 ) )
#define RNG_CXX11
// Order-of-inclusion bug under MSVC: Include <random> early.
#if ! defined( SC_OSX ) || ( defined( SC_OSX ) && ! defined( SC_CLANG ) )
#include <random>
#else
#include <tr1/random>
#endif
#endif

#ifndef M_PI
#define M_PI ( 3.14159265358979323846 )
#endif

namespace rng {

// ==========================================================================
// SFMT Random Number Generator
// ==========================================================================

/**
 * SIMD oriented Fast Mersenne Twister(SFMT) pseudorandom number generator
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * The new BSD License is applied to this software.
 */

// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/

// ALWAYS ALLOCATE THROUGH NEW
template <typename Derived>
class rng_engine_mt_base_t : public engine_t
{
  // "Cleverness" alert: this template uses CRTP (see
  // http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
  // to invoke the do_recursion() implementation in the _Derived_ class.
  // This lets us have two different flavors of the algorithm - one that
  // uses SSE2, and one without - without virtual functions or code
  // duplication.
public:
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

protected:
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
  static void do_recursion( w128_t *r, const w128_t *a, const w128_t *b, w128_t *d )
  { Derived::do_recursion( r, a, b, d ); }

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

public:
  void _seed( unsigned start )
  { dsfmt_chk_init_gen_rand( &dsfmt_global_data, start ); }

  // This is the main interface of the RNG_ENGINE. Returns a uniformly distributed number in the range [0 1]
  double real()
  { return dsfmt_genrand_close_open( &dsfmt_global_data ) - 1.0; }
};

// ==========================================================================
// SFMT Random Number Generator - Standard C, NO SSE2
// ==========================================================================

class rng_engine_mt_t : public rng_engine_mt_base_t<rng_engine_mt_t>
{
public:
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
};

// ==========================================================================
// SFMT Random Number Generator - SSE2 optimizations
// ==========================================================================

#if defined(RNG_USE_SSE2)
class rng_engine_mt_sse2_t : public rng_engine_mt_base_t<rng_engine_mt_sse2_t>
{
public:
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

  rng_engine_mt_sse2_t()
  {
    // Validate proper alignment for SSE2 types.
    assert( ( uintptr_t )dsfmt_global_data.status % 16 == 0 );
  }

  // 32-bit libraries typically align malloc chunks to sizeof(double) == 8.
  // This object needs to be aligned to sizeof(__m128d) == 16.
  static void* operator new( size_t size )
  { return _mm_malloc( size, sizeof( __m128d ) ); }
  static void operator delete( void* p )
  { return _mm_free( p ); }
};
#endif

// ==========================================================================
// C Standard Library Random Number Generator
// ==========================================================================

class rng_engine_std_t : public rng::engine_t
{
  // Whether this RNG works correctly or explodes in your face when
  // multithreading depends on your particular C library implementation,
  // since the standard doesn't describe the behavior of seed()/rand() in the
  // presence of multiple threads. (If it does work, it will likely be very
  // _slow_).
public:
  void _seed( unsigned start )
  { srand( start ); }

  double real()
  { return rand() * ( 1.0 / ( 1.0 + RAND_MAX ) ); }
};

#ifdef RNG_CXX11
// ==========================================================================
// C++ 11 Mersenne Twister Random Number Generator
// ==========================================================================

/*
 * This integrates a C++11 random number generator into our native rng_t concept
 * The advantage of this container is the extremely small code, with nearly no
 * maintenance cost.
 * Unfortunately, it is around 40 percent slower than the dsfmt implementation.
 */
class rng_engine_mt_cxx11_t : public rng::engine_t
{
  std::mt19937 engine_t; // Mersenne twister MT19937
public:
  void _seed( unsigned start ) { engine_t.seed( start ); }

  double real()
  { return ( static_cast<double>( engine_t() ) - engine_t.min() ) / ( engine_t.max() - engine_t.min() ); }
};

#endif // END RNG_CXX11

std::shared_ptr<engine_t> engine_t::create( engine_t::type t )
{
  switch( t )
  {
  case STD:
#ifdef RNG_CXX11
    return std::shared_ptr<engine_t>( new rng_engine_mt_cxx11_t() );
#else
    return std::shared_ptr<engine_t>( new rng_engine_std_t() );
#endif
    break;

  case MERSENNE_TWISTER:
  case BEST:
  default:
    break;
  }
#ifdef RNG_USE_SSE2
    return std::shared_ptr<engine_t>( new rng_engine_mt_sse2_t() );
#else
    return std::shared_ptr<engine_t>( new rng_engine_mt_t() );
#endif
}

// ==========================================================================
// Probability Distributions
// ==========================================================================

distribution_t::distribution_t( std::shared_ptr<engine_t> engine ) :
        _engine( engine ),
        gauss_pair_value( 0.0 ),
        gauss_pair_use( false )
      { }

  // Seed rng generator
  void distribution_t::seed( uint32_t value = static_cast<uint32_t>( std::time( NULL ) ) )
  { _engine -> seed( value ); }

  // obtain a random value from the rng generator
  double distribution_t::real() { return ( *_engine )(); }

  // bernoulli distribution
  bool distribution_t::roll( double chance )
  {
    if ( chance <= 0 ) return false;
    if ( chance >= 1 ) return true;
    return real() < chance;
  }

  // uniform distribution in the range [min max]
  double distribution_t::range( double min, double max )
  {
    assert( min <= max );
    return min + real() * ( max - min );
  }

  // gaussian distribution
  double distribution_t::gauss( double mean, double stddev, bool truncate_low_end )
  {
    /* This code adapted from ftp://ftp.taygeta.com/pub/c/boxmuller.c
     * Implements the Polar form of the Box-Muller Transformation
     *
     * (c) Copyright 1994, Everett F. Carter Jr.
     *     Permission is granted by the author to use
     *     this software for any application provided this
     *     copyright notice is preserved.
     */

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

  // exponential distribution
  double distribution_t::exponential( double nu )
  {
    double x;
    do
    {
      x = real();
    }
    while ( x >= 1.0 ); // avoid ln(0)

    return - std::log( 1 - x ) * nu;
  }

  // Exponentially modified Gaussian distribution
  double distribution_t::exgauss( double gauss_mean, double gauss_stddev, double exp_nu )
  { return std::max( 0.0, gauss( gauss_mean, gauss_stddev ) + exponential( exp_nu ) ); }

// stdnormal_cdf ============================================================

// Source of the next 2 functions: http://home.online.no/~pjacklam/notes/invnorm/

/*
 * The standard normal CDF, for one random variable.
 *
 *   Author:  W. J. Cody
 *   URL:   http://www.netlib.org/specfun/erf
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

// stdnormal_inv ============================================================

// This is used to get the normal distribution inverse for our user-specifiable confidence levels
// For example for the default 95% confidence level, this function will return the well known number of
// 1.96, so we know that 95% of the distribution is between -1.96 and +1.96 std deviations from the mean.

/*
 * The inverse standard normal distribution.
 *
 *   Author:      Peter John Acklam <pjacklam@online.no>
 *   URL:         http://home.online.no/~pjacklam
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

} // end rng namespace

#ifdef UNIT_TEST
// Code to test functionality and performance of our RNG implementations

#include <iostream>
#include <iomanip>

namespace { // anonymous namespace ==========================================


int64_t milliseconds()
{
  return 1000 * clock() / CLOCKS_PER_SEC;
}

template <typename Engine>
void test_one( uint64_t n, const char* name )
{
  using namespace rng;
  distribution_t rng( std::shared_ptr<engine_t>( new Engine() ) );

  int64_t start_time = milliseconds();

  double average = 0;
  for ( uint64_t i = 0; i < n; ++i )
    average += rng.real();

  average /= n;
  int64_t elapsed_cpu = milliseconds() - start_time;

  std::cout << n << " calls to distribution_t<" << name
            << ">::real(): average = " << std::setprecision( 8 ) << average
            << ", time = " << elapsed_cpu << " ms"
               ", numbers/sec = " << static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) << "\n\n";
}

// Monte-Carlo PI calculation.
template <typename Engine>
void monte_carlo( uint64_t n, const char* name )
{
  using namespace rng;
  distribution_t rng( std::shared_ptr<engine_t>( new Engine() ) );
  int64_t start_time = milliseconds();

  uint64_t count_in_sphere = 0;
  for ( uint64_t i = 0; i < n; ++i )
  {
    double x1 = rng.real();
    double x2 = rng.real();
    if ( x1 * x1 + x2 * x2 < 1.0 )
      ++count_in_sphere;
  }
  int64_t elapsed_cpu = milliseconds() - start_time;

  std::cout << n << " runs for pi-calculation with " << name
            << ". count in sphere = " << count_in_sphere
            << ", pi = " << std::setprecision( 21 ) << static_cast<double>( count_in_sphere ) * 4 / n
            << ", time = " << elapsed_cpu << " ms"
               ", numbers/sec = " << static_cast<uint64_t>( n * 1000.0 / elapsed_cpu ) << "\n\n";
}

} // anonymous namespace ====================================================

int main( int /*argc*/, char** /*argv*/ )
{
  using namespace rng;

  uint64_t n = 100000000;

#define TEST_ONE( e ) test_one<e>( n, #e )

  TEST_ONE( rng_engine_std_t );
  TEST_ONE( rng_engine_mt_t );

#ifdef RNG_USE_SSE2
  TEST_ONE( rng_engine_mt_sse2_t );
#endif

#ifdef RNG_CXX11
  TEST_ONE( rng_engine_mt_cxx11_t );
#endif // END RNG_CXX11


#define MONTE_CARLO( t ) monte_carlo<t>( n, #t )

#if defined( RNG_USE_SSE2 )
  MONTE_CARLO( rng_engine_mt_sse2_t );
#else
  MONTE_CARLO( rng_engine_mt_t );
#endif

  n /= 10;

#ifdef RNG_USE_SSE2
  distribution_t rng( std::shared_ptr<engine_t>( new rng_engine_mt_sse2_t() ) );
#else
  distribution_t rng( std::shared_ptr<engine_t>( new rng_engine_mt_t() ) );
#endif


  // double gauss
  {
    int64_t start_time = milliseconds();

    double average = 0;
    for ( uint64_t i = 0; i < n; i++ )
      average += rng.gauss( 0, 1 );
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
      average += 0.1 + rng.exgauss( 0.3, 0.06, 0.25 );
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
      double result = rng.exponential( nu );
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
    std::cout << "  " << rng.real();
    if ( i % 10 == 0 )
      std::cout << '\n';
  }

  std::cout << "\ngauss mean=0, std_dev=1.0:\n";
  for ( unsigned i = 1; i <= 100; i++ )
  {
    std::cout << "  " << rng.gauss( 0.0, 1.0 );
    if ( i % 10 == 0 )
      std::cout << '\n';
  }

  std::cout << "\nroll 30%:\n";
  for ( unsigned i = 1; i <= 100; i++ )
  {
    std::cout << "  " << rng.roll( 0.30 );
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

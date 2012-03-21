// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// rng_t::rng_t =============================================================

rng_t::rng_t( const std::string& n, bool avg_range, bool avg_gauss ) :
  name_str( n ),
  expected_roll( 0 ),  actual_roll( 0 ),  num_roll( 0 ),
  expected_range( 0 ), actual_range( 0 ), num_range( 0 ),
  expected_gauss( 0 ), actual_gauss( 0 ), num_gauss( 0 ),
  next( 0 ),
  gauss_pair_use( false ),
  average_range( avg_range ),
  average_gauss( avg_gauss )
{}

// rng_t::seed ==============================================================

void rng_t::seed( uint32_t )
{}

// rng_t::roll ==============================================================

bool rng_t::roll( double chance )
{
  if ( chance <= 0 ) return 0;
  if ( chance >= 1 ) return 1;
  num_roll++;
  bool result = ( real() < chance );
  expected_roll += chance;
  if ( result ) actual_roll++;
  return result;
}

// rng_t::range =============================================================

double rng_t::range( double min,
                     double max )
{
  assert( min <= max );
  double result = ( min + max ) / 2.0;
  if ( average_range ) return result;
  num_range++;
  expected_range += result;
  if ( min < max ) result =  min + real() * ( max - min );
  actual_range += result;
  return result;
}

// rng_t::gauss =============================================================

double rng_t::gauss( double mean,
                     double stddev,
                     const bool truncate_low_end )
{
  if ( average_gauss ) return mean;

  // This code adapted from ftp://ftp.taygeta.com/pub/c/boxmuller.c
  // Implements the Polar form of the Box-Muller Transformation
  //
  //                  (c) Copyright 1994, Everett F. Carter Jr.
  //                      Permission is granted by the author to use
  //                      this software for any application provided this
  //                      copyright notice is preserved.

  double x1, x2, w, y1, y2, z;

  if ( gauss_pair_use )
  {
    z = gauss_pair_value;
    gauss_pair_use = false;
  }
  else
  {
    do
    {
      x1 = 2.0 * real() - 1.0;
      x2 = 2.0 * real() - 1.0;
      w = x1 * x1 + x2 * x2;
    }
    while ( w >= 1.0 );

    w = sqrt( ( -2.0 * log( w ) ) / w );
    y1 = x1 * w;
    y2 = x2 * w;

    z = y1;
    gauss_pair_value = y2;
    gauss_pair_use = true;
  }

  double result = mean + z * stddev;

  // True gaussian distribution can of course yield any number at some probability.  So truncate on the low end.
  if ( truncate_low_end && result < 0 )
    result = 0;

  num_gauss++;
  expected_gauss += mean;
  actual_gauss += result;

  return result;
}

// rng_t::exgauss ===========================================================

double rng_t::exgauss( double mean, double stddev, double nu )
{
  double x = 0;
  do
  {
    x = real();
  }
  while ( x <=  0 ); // avoid ln(0)

  double result =  ( gauss( mean, stddev ) ) - log( x / nu ) * nu;
  if ( result < 0 ) result = 0;
  if ( result > 5 ) result = 5; // cut it off at 5s

  return result;
}

// rng_t::report ============================================================

void rng_t::report( FILE* file )
{
  util_t::fprintf( file, "RNG %s Actual/Expected Roll=%.6f Range=%.6f Gauss=%.6f\n",
                   name_str.c_str(),
                   ( ( expected_roll  == 0 ) ? 1.0 : actual_roll  / expected_roll  ),
                   ( ( expected_range == 0 ) ? 1.0 : actual_range / expected_range ),
                   ( ( expected_gauss == 0 ) ? 1.0 : actual_gauss / expected_gauss ) );
}

// rng_t::stdnormal_cdf ==

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
double rng_t::stdnormal_cdf( double u )
{
  const double a[5] =
  {
    1.161110663653770e-002,3.951404679838207e-001,2.846603853776254e+001,
    1.887426188426510e+002,3.209377589138469e+003
  };
  const double b[5] =
  {
    1.767766952966369e-001,8.344316438579620e+000,1.725514762600375e+002,
    1.813893686502485e+003,8.044716608901563e+003
  };
  const double c[9] =
  {
    2.15311535474403846e-8,5.64188496988670089e-1,8.88314979438837594e00,
    6.61191906371416295e01,2.98635138197400131e02,8.81952221241769090e02,
    1.71204761263407058e03,2.05107837782607147e03,1.23033935479799725E03
  };
  const double d[9] =
  {
    1.00000000000000000e00,1.57449261107098347e01,1.17693950891312499e02,
    5.37181101862009858e02,1.62138957456669019e03,3.29079923573345963e03,
    4.36261909014324716e03,3.43936767414372164e03,1.23033935480374942e03
  };
  const double p[6] =
  {
    1.63153871373020978e-2,3.05326634961232344e-1,3.60344899949804439e-1,
    1.25781726111229246e-1,1.60837851487422766e-2,6.58749161529837803e-4
  };
  const double q[6] =
  {
    1.00000000000000000e00,2.56852019228982242e00,1.87295284992346047e00,
    5.27905102951428412e-1,6.05183413124413191e-2,2.33520497626869185e-3
  };
  register double y, z;

  if ( u == std::numeric_limits<double>::quiet_NaN() )
    return std::numeric_limits<double>::quiet_NaN();

  if ( u == std::numeric_limits<double>::infinity() )
    return ( u < 0 ? 0.0 : 1.0 );

  y = fabs( u );

  if ( y <= 0.46875* sqrt( 2.0 ) )
  {
    /* evaluate erf() for |u| <= sqrt(2)*0.46875 */
    z = y * y;
    y = u * ( ( ( ( a[0]*z+a[1] )*z+a[2] )*z+a[3] )*z+a[4] )
        /( ( ( ( b[0]*z+b[1] )*z+b[2] )*z+b[3] )*z+b[4] );
    return 0.5 + y;
  }

  z = exp( -y * y / 2 ) / 2;

  if ( y <= 4.0 )
  {
    /* evaluate erfc() for sqrt(2)*0.46875 <= |u| <= sqrt(2)*4.0 */
    y = y / sqrt( 2.0 );
    y =
      ( ( ( ( ( ( ( ( c[0]*y+c[1] )*y+c[2] )*y+c[3] )*y+c[4] )*y+c[5] )*y+c[6] )*y+c[7] )*y+c[8] )


      /( ( ( ( ( ( ( ( d[0]*y+d[1] )*y+d[2] )*y+d[3] )*y+d[4] )*y+d[5] )*y+d[6] )*y+d[7] )*y+d[8] );

    y = z * y;
  }
  else
  {
    /* evaluate erfc() for |u| > sqrt(2)*4.0 */
    z = z * sqrt( 2.0 ) / y;
    y = 2 / ( y * y );
    y = y*( ( ( ( ( p[0]*y+p[1] )*y+p[2] )*y+p[3] )*y+p[4] )*y+p[5] )
        /( ( ( ( ( q[0]*y+q[1] )*y+q[2] )*y+q[3] )*y+q[4] )*y+q[5] );
    y = z*( 1.0 / sqrt ( M_PI ) - y );
  }

  return ( u < 0.0 ? y : 1-y );
};

// rng_t::stdnormal_inv ============================================================


/*
 * The inverse standard normal distribution.
 *
 *   Author:      Peter John Acklam <pjacklam@online.no>
 *   URL:         http://home.online.no/~pjacklam
 *
 * This function is based on the MATLAB code from the address above,
 * translated to C, and adapted for our purposes.
 */
double rng_t::stdnormal_inv( double p )
{
  const double a[6] =
  {
    -3.969683028665376e+01,  2.209460984245205e+02,
    -2.759285104469687e+02,  1.383577518672690e+02,
    -3.066479806614716e+01,  2.506628277459239e+00
  };
  const double b[5] =
  {
    -5.447609879822406e+01,  1.615858368580409e+02,
    -1.556989798598866e+02,  6.680131188771972e+01,
    -1.328068155288572e+01
  };
  const double c[6] =
  {
    -7.784894002430293e-03, -3.223964580411365e-01,
    -2.400758277161838e+00, -2.549732539343734e+00,
    4.374664141464968e+00,  2.938163982698783e+00
  };
  const double d[4] =
  {
    7.784695709041462e-03,  3.224671290700398e-01,
    2.445134137142996e+00,  3.754408661907416e+00
  };

  register double q, t, u;

  if ( p == std::numeric_limits<double>::quiet_NaN() || p > 1.0 || p < 0.0 )
    return std::numeric_limits<double>::quiet_NaN();

  if ( p == 0.0 )
    return - std::numeric_limits<double>::infinity();

  if ( p == 1.0 )
    return std::numeric_limits<double>::infinity();

  q = std::min( p, 1 - p );

  if ( q > 0.02425 )
  {
    /* Rational approximation for central region. */
    u = q - 0.5;
    t = u * u;
    u = u * ( ( ( ( ( a[0]*t+a[1] )*t+a[2] )*t+a[3] )*t+a[4] )*t+a[5] )
        /( ( ( ( ( b[0]*t+b[1] )*t+b[2] )*t+b[3] )*t+b[4] )*t+1 );
  }
  else
  {
    /* Rational approximation for tail region. */
    t = sqrt( -2*log( q ) );
    u = ( ( ( ( ( c[0]*t+c[1] )*t+c[2] )*t+c[3] )*t+c[4] )*t+c[5] )
        /( ( ( ( d[0]*t+d[1] )*t+d[2] )*t+d[3] )*t+1 );
  }

  /* The relative error of the approximation has absolute value less
     than 1.15e-9.  One iteration of Halley's rational method (third
     order) gives full machine precision... */
  t = stdnormal_cdf( u ) - q;    /* error */
  t = t * 2.0 / sqrt( M_PI ) *exp( u*u/2 ); /* f(u)/df(u) */
  u = u-t/( 1+u*t/2 );   /* Halley's method */

  return ( p > 0.5 ? -u : u );
};

namespace { // ANONYMOUS ====================================================

// ==========================================================================
// Standard Random Number Generator
// ==========================================================================

class rng_std_t : public rng_t
{
public:
  rng_std_t( const std::string& name, bool avg_range, bool avg_gauss ) :
    rng_t( name, avg_range, avg_gauss )
  {}

  virtual rng_type type() const // override
  { return RNG_STANDARD; }

  virtual void seed( uint32_t start ) // override
  { srand( start ); }

  virtual double real() // override
  { return rand() * ( 1.0 / ( ( ( double ) RAND_MAX ) + 1.0 ) ); }
};


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

// Mersenne Exponent. The period of the sequence is a multiple of 2^MEXP-1.
#define DSFMT_MEXP 19937

#define DSFMT_N ((DSFMT_MEXP - 128) / 104 + 1)

#define DSFMT_N64 (DSFMT_N * 2)

#ifndef UINT64_C
#define UINT64_C(v) (v ## ULL)
#endif

#define DSFMT_LOW_MASK  UINT64_C(0x000FFFFFFFFFFFFF)
#define DSFMT_HIGH_CONST UINT64_C(0x3FF0000000000000)
#define DSFMT_SR  12

#define DSFMT_POS1  117
#define DSFMT_SL1 19
#define DSFMT_MSK1  UINT64_C(0x000ffafffffffb3f)
#define DSFMT_MSK2  UINT64_C(0x000ffdfffc90fffd)
#define DSFMT_MSK32_1 0x000ffaffU
#define DSFMT_MSK32_2 0xfffffb3fU
#define DSFMT_MSK32_3 0x000ffdffU
#define DSFMT_MSK32_4 0xfc90fffdU
#define DSFMT_FIX1  UINT64_C(0x90014964b32f4329)
#define DSFMT_FIX2  UINT64_C(0x3b8d12ac548a7c7a)
#define DSFMT_PCV1  UINT64_C(0x3d84e1ac0dc82880)
#define DSFMT_PCV2  UINT64_C(0x0000000000000001)
#define DSFMT_IDSTR "dSFMT2-19937:117-19:ffafffffffb3f-ffdfffc90fffd"

#if defined(__SSE2__)
#include <emmintrin.h>
#include <mm_malloc.h>

/** mask data for sse2 */
const __m128i sse2_param_mask = _mm_set_epi32( DSFMT_MSK32_3, DSFMT_MSK32_4,
                                               DSFMT_MSK32_1, DSFMT_MSK32_2 );

#define SSE2_SHUFF 0x1b

/** 128-bit data structure */
union w128_t {
    __m128i si;
    __m128d sd;
    uint64_t u[2];
    uint32_t u32[4];
    double d[2];
};

#else  /* standard C */
/** 128-bit data structure */
union w128_t {
  uint64_t u[2];
  uint32_t u32[4];
  double d[2];
};
#endif

/** the 128-bit internal state array */
struct dsfmt_t {
    w128_t status[DSFMT_N + 1];
    int idx;
};
#if defined(__SSE2__)
/**
 * This function represents the recursion formula.
 * @param r output 128-bit
 * @param a a 128-bit part of the internal state array
 * @param b a 128-bit part of the internal state array
 * @param d a 128-bit part of the internal state array (I/O)
 */
inline static void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *u) {
    __m128i v, w, x, y, z;

    x = a->si;
    z = _mm_slli_epi64(x, DSFMT_SL1);
    y = _mm_shuffle_epi32(u->si, SSE2_SHUFF);
    z = _mm_xor_si128(z, b->si);
    y = _mm_xor_si128(y, z);

    v = _mm_srli_epi64(y, DSFMT_SR);
    w = _mm_and_si128(y, sse2_param_mask);
    v = _mm_xor_si128(v, x);
    v = _mm_xor_si128(v, w);
    r->si = v;
    u->si = y;
}
#else /* standard C */

inline void do_recursion( w128_t* r, const w128_t* a, const w128_t* b, w128_t* lung )
{
    uint64_t t0, t1, L0, L1;

    t0 = a->u[0];
    t1 = a->u[1];
    L0 = lung->u[0];
    L1 = lung->u[1];
    lung->u[0] = (t0 << DSFMT_SL1) ^ (L1 >> 32) ^ (L1 << 32) ^ b->u[0];
    lung->u[1] = (t1 << DSFMT_SL1) ^ (L0 >> 32) ^ (L0 << 32) ^ b->u[1];
    r->u[0] = (lung->u[0] >> DSFMT_SR) ^ (lung->u[0] & DSFMT_MSK1) ^ t0;
    r->u[1] = (lung->u[1] >> DSFMT_SR) ^ (lung->u[1] & DSFMT_MSK2) ^ t1;
}
#endif

void dsfmt_gen_rand_all(dsfmt_t *dsfmt) {
    int i;
    w128_t lung;

    lung = dsfmt->status[DSFMT_N];
    do_recursion(&dsfmt->status[0], &dsfmt->status[0],
     &dsfmt->status[DSFMT_POS1], &lung);
    for (i = 1; i < DSFMT_N - DSFMT_POS1; i++) {
  do_recursion(&dsfmt->status[i], &dsfmt->status[i],
         &dsfmt->status[i + DSFMT_POS1], &lung);
    }
    for (; i < DSFMT_N; i++) {
  do_recursion(&dsfmt->status[i], &dsfmt->status[i],
         &dsfmt->status[i + DSFMT_POS1 - DSFMT_N], &lung);

    }
    dsfmt->status[DSFMT_N] = lung;
}

/**
 * This function initializes the internal state array to fit the IEEE
 * 754 format.
 * @param dsfmt dsfmt state vector.
 */
void initial_mask(dsfmt_t *dsfmt) {
    int i;
    uint64_t *psfmt;

    psfmt = &dsfmt->status[0].u[0];
    for (i = 0; i < DSFMT_N * 2; i++) {
        psfmt[i] = (psfmt[i] & DSFMT_LOW_MASK) | DSFMT_HIGH_CONST;
    }
}

/**
 * This function certificate the period of 2^{SFMT_MEXP}-1.
 * @param dsfmt dsfmt state vector.
 */
void period_certification(dsfmt_t *dsfmt) {
    uint64_t pcv[2] = {DSFMT_PCV1, DSFMT_PCV2};
    uint64_t tmp[2];
    uint64_t inner;
    int i;

    tmp[0] = (dsfmt->status[DSFMT_N].u[0] ^ DSFMT_FIX1);
    tmp[1] = (dsfmt->status[DSFMT_N].u[1] ^ DSFMT_FIX2);

    inner = tmp[0] & pcv[0];
    inner ^= tmp[1] & pcv[1];
    for (i = 32; i > 0; i >>= 1) {
        inner ^= inner >> i;
    }
    inner &= 1;
    /* check OK */
    if (inner == 1) {
  return;
    }
    /* check NG, and modification */
    dsfmt->status[DSFMT_N].u[1] ^= 1;
    return;
}

inline int idxof(int i) {
    return i;
}


/**
 * This function initializes the internal state array with a 32-bit
 * integer seed.
 * @param dsfmt dsfmt state vector.
 * @param seed a 32-bit integer used as the seed.
 * @param mexp caller's mersenne expornent
 */
void dsfmt_chk_init_gen_rand(dsfmt_t *dsfmt, uint32_t seed) {
    int i;
    uint32_t *psfmt;


    psfmt = &dsfmt->status[0].u32[0];
    psfmt[idxof(0)] = seed;
    for (i = 1; i < (DSFMT_N + 1) * 4; i++) {
        psfmt[idxof(i)] = 1812433253UL
      * (psfmt[idxof(i - 1)] ^ (psfmt[idxof(i - 1)] >> 30)) + i;
    }
    initial_mask(dsfmt);
    period_certification(dsfmt);
    dsfmt->idx = DSFMT_N64;
}

/**
 * This function generates and returns double precision pseudorandom
 * number which distributes uniformly in the range [1,2).  This is
 * the primitive and faster than generating numbers in other ranges.
 * dsfmt_init_gen_rand() or dsfmt_init_by_array() must be called
 * before this function.
 */
double dsfmt_genrand_close_open(dsfmt_t *dsfmt) {

    double *psfmt64 = &dsfmt->status[0].d[0];

    if (dsfmt->idx >= DSFMT_N64)
    {
      dsfmt_gen_rand_all(dsfmt);
      dsfmt->idx = 0;
    }
    return psfmt64[dsfmt->idx++];
}

} // ANONYMOUS ==============================================================

class rng_sfmt_t : public rng_t
{
private:
  /** global data */
  dsfmt_t dsfmt_global_data;

public:
  rng_sfmt_t( const std::string& name, bool avg_range=false, bool avg_gauss=false ) :
    rng_t( name, avg_range, avg_gauss )
  { seed( time( NULL ) ); }

  virtual rng_type type() const
  { return RNG_MERSENNE_TWISTER; }

  virtual double real()
  { return dsfmt_genrand_close_open( &dsfmt_global_data ) - 1.0; }

  virtual void seed( uint32_t start )
  { dsfmt_chk_init_gen_rand( &dsfmt_global_data, start ); }

#if defined(__SSE2__)
  // 32-bit libraries typically align malloc chunks to sizeof(double) == 8.
  // This object needs to be aligned to sizeof(__m128d) == 16.
  static void* operator new( size_t size )
  { return _mm_malloc( size, sizeof( dsfmt_t ) ); }
  static void operator delete( void* p )
  { return _mm_free( p ); }
#endif
};


// ==========================================================================
// Base-Class for Normalized RNGs
// ==========================================================================

struct rng_normalized_t : public rng_t
{
  rng_t* base;

  rng_normalized_t( const std::string& name, rng_t* b, bool avg_range=false, bool avg_gauss=false ) :
    rng_t( name, avg_range, avg_gauss ), base( b ) {}

  virtual double real() { return base -> real(); }
  virtual double range( double min, double max ) { return ( min + max ) / 2.0; }
  virtual double gauss( double mean, double /* stddev */, bool /* truncate_low_end */ ) { return mean; }
  virtual timespan_t gauss( timespan_t mean, timespan_t /* stddev */ ) { return mean; }
  virtual bool    roll( double chance ) = 0; // must be overridden
};

// ==========================================================================
// Normalized RNG via Variable Phase Shift
// ==========================================================================

struct rng_phase_shift_t : public rng_normalized_t
{
  std::vector<double> range_distribution;
  std::vector<double> gauss_distribution;
  int range_index, gauss_index;

  rng_phase_shift_t( const std::string& name, rng_t* b, bool avg_range=false, bool avg_gauss=false ) :
    rng_normalized_t( name, b, avg_range, avg_gauss )
  {
    range_distribution.resize( 10 );
    for ( int i=0; i < 5; i++ )
    {
      range_distribution[ i*2   ] = 0.5 + ( i + 0.5 );
      range_distribution[ i*2+1 ] = 0.5 - ( i + 0.5 );
    }
    range_index = ( int ) real() * 10;

    static const double gauss_offset[] = { 0.3, 0.5, 0.8, 1.3, 2.1 };
    gauss_distribution.resize( 10 );
    for ( int i=0; i < 5; i++ )
    {
      gauss_distribution[ i*2   ] = 0.0 + gauss_offset[ i ];
      gauss_distribution[ i*2+1 ] = 0.0 - gauss_offset[ i ];
    }
    gauss_index = ( int ) real() * 10;

    actual_roll = real() - 0.5;
  }
  virtual rng_type type() const { return RNG_PHASE_SHIFT; }

  virtual double range( double min, double max )
  {
    if ( average_range ) return ( min + max ) / 2.0;
    num_range++;
    int size = ( int ) range_distribution.size();
    if ( ++range_index >= size ) range_index = 0;
    double result = min + range_distribution[ range_index ] * ( max - min );
    expected_range += ( max - min ) / 2.0;
    actual_range += result;
    return result;
  }
  virtual double gauss( double mean, double stddev, bool /* truncate_low_end */ = false )
  {
    if ( average_gauss ) return mean;
    num_gauss++;
    int size = ( int ) gauss_distribution.size();
    if ( ++gauss_index >= size ) gauss_index = 0;
    double result = mean + gauss_distribution[ gauss_index ] * stddev;
    expected_gauss += mean;
    actual_gauss += result;
    return result;
  }
  virtual timespan_t gauss( timespan_t mean, timespan_t stddev )
  {
    return TIMESPAN_FROM_NATIVE_VALUE( gauss( TIMESPAN_TO_NATIVE_VALUE( mean ), TIMESPAN_TO_NATIVE_VALUE( stddev ) ) );
  }
  virtual bool roll( double chance )
  {
    if ( chance <= 0 ) return 0;
    if ( chance >= 1 ) return 1;
    num_roll++;
    expected_roll += chance;
    if ( actual_roll < expected_roll )
    {
      actual_roll++;
      return true;
    }
    return false;
  }
};

// ==========================================================================
// Normalized RNG via Distribution Pre-Fill
// ==========================================================================

struct rng_pre_fill_t : public rng_normalized_t
{
  std::vector<double>  roll_distribution;
  std::vector<double> range_distribution;
  std::vector<double> gauss_distribution;
  int roll_index, range_index, gauss_index;

  rng_pre_fill_t( const std::string& name, rng_t* b, bool avg_range=false, bool avg_gauss=false ) :
    rng_normalized_t( name, b, avg_range, avg_gauss )
  {
    range_distribution.resize( 10 );
    for ( int i=0; i < 5; i++ )
    {
      range_distribution[ i*2   ] = 0.5 + ( i + 0.5 );
      range_distribution[ i*2+1 ] = 0.5 - ( i + 0.5 );
    }
    range_index = ( int ) real() * 10;

    double gauss_offset[] = { 0.3, 0.5, 0.8, 1.3, 2.1 };
    gauss_distribution.resize( 10 );
    for ( int i=0; i < 5; i++ )
    {
      gauss_distribution[ i*2   ] = 0.0 + gauss_offset[ i ];
      gauss_distribution[ i*2+1 ] = 0.0 - gauss_offset[ i ];
    }
    gauss_index = ( int ) real() * 10;

    roll_distribution.resize( 10 );
    roll_index = 10;
  }
  virtual rng_type type() const { return RNG_PRE_FILL; }

  virtual bool roll( double chance )
  {
    if ( chance <= 0 ) return 0;
    if ( chance >= 1 ) return 1;
    double avg_chance = ( num_roll > 0 ) ? ( expected_roll / num_roll ) : chance;
    num_roll++;
    int size = ( int ) roll_distribution.size();
    if ( ++roll_index >= size )
    {
      double exact_procs = avg_chance * size + expected_roll - actual_roll;
      int num_procs = ( int ) floor( exact_procs );
      num_procs += base -> roll( exact_procs - num_procs );
      if ( num_procs > size ) num_procs = size;
      int up=1, down=0;
      if ( num_procs > ( size >> 1 ) )
      {
        up = 0;
        down = 1;
        num_procs = size - num_procs;
      }
      for ( int i=0; i < size; i++ ) roll_distribution[ i ] = down;
      while ( num_procs > 0 )
      {
        int index = ( int ) ( real() * size * 0.9999 );
        if ( roll_distribution[ index ] == up ) continue;
        roll_distribution[ index ] = up;
        num_procs--;
      }
      roll_index = 0;
    }
    expected_roll += chance;
    if ( roll_distribution[ roll_index ] )
    {
      actual_roll++;
      return true;
    }
    return false;
  }

  virtual double range( double min, double max )
  {
    if ( average_range ) return ( min + max ) / 2.0;
    int size = ( int ) range_distribution.size();
    if ( ++range_index >= size )
    {
      for ( int i=0; i < size; i++ )
      {
        int other = ( int ) ( real() * size * 0.999 );
        std::swap( range_distribution[ i ], range_distribution[ other ] );
      }
      range_index = 0;
    }
    num_range++;
    double result = min + range_distribution[ range_index ] * ( max - min );
    expected_range += ( max - min ) / 2.0;
    actual_range += result;
    return result;
  }

  virtual double gauss( double mean, double stddev, bool /* truncate_low_end */ = false )
  {
    if ( average_gauss ) return mean;
    int size = ( int ) gauss_distribution.size();
    if ( ++gauss_index >= size )
    {
      for ( int i=0; i < size; i++ )
      {
        int other = ( int ) ( real() * size * 0.999 );
        std::swap( gauss_distribution[ i ], gauss_distribution[ other ] );
      }
      gauss_index = 0;
    }
    num_gauss++;
    double result = mean + gauss_distribution[ gauss_index ] * stddev;
    expected_gauss += mean;
    actual_gauss += result;
    return result;
  }
  virtual timespan_t gauss( timespan_t mean, timespan_t stddev )
  {
    return TIMESPAN_FROM_NATIVE_VALUE( gauss( TIMESPAN_TO_NATIVE_VALUE( mean ), TIMESPAN_TO_NATIVE_VALUE( stddev ) ) );
  }
};

// ==========================================================================
// Distribution
// ==========================================================================

struct distribution_t
{
  double actual, expected;
  std::vector<double> chances, values;
  std::vector<int> counts;
  int last, total_count;

  distribution_t( unsigned int size ) :
    actual( 0 ), expected( 0 ),
    chances( size ), values( size ), counts( size ),
    last( size-1 ), total_count( 0 )
  {
    assert( size > 0 );
  }

  virtual ~distribution_t() {}

  virtual int next_bucket()
  {
    total_count++;

    int best_bucket = -1;
    double best_diff = 0;

    for ( int i=0; i <= last; i++ )
    {
      if ( counts[ i ] < total_count * chances[ i ] )
      {
        double diff = fabs( actual + values[ i ] - expected );

        if ( best_bucket < 0 || diff < best_diff )
        {
          best_bucket = i;
          best_diff = diff;
        }
      }
    }

    if ( best_bucket < 0 ) best_bucket = last;

    actual += values[ best_bucket ];
    counts[ best_bucket ]++;

    return best_bucket;
  }

  virtual double next_value()
  {
    return values[ next_bucket() ];
  }

  virtual void verify( rng_t* rng )
  {
    util_t::printf( "distribution_t::verify:\n" );
    double sum=0;
    for ( int i=0; i <= last; i++ ) sum += chances[ i ];
    util_t::printf( "\tsum: %f\n", sum );
    std::vector<int> rng_counts( chances.size() );
    for ( int i=0; i < total_count; i++ )
    {
      double p = rng -> real();
      int index=0;
      while ( index < last )
      {
        p -= chances[ index ];
        if ( p < 0 ) break;
        index++;
      }
      rng_counts[ index ]++;
    }
    for ( int i=0; i <= last; i++ ) util_t::printf( "\t%d : %d\n", counts[ i ], rng_counts[ i ] );
  }
};

// ==========================================================================
// Range Distribution
// ==========================================================================

struct range_distribution_t : public distribution_t
{
  range_distribution_t() : distribution_t( 10 )
  {
    for ( int i=0; i < 10; i++ )
    {
      chances[ i ] = 0.10;
      values [ i ] = i * 0.10 + 0.05;
    }
  }

  virtual int next_bucket()
  {
    expected += 0.50;
    return distribution_t::next_bucket();
  }
};

// ==========================================================================
// Gauss Distribution
// ==========================================================================

struct gauss_distribution_t : public distribution_t
{
  gauss_distribution_t() : distribution_t( 9 )
  {
    chances[ 0 ] = 0.025;  values[ 0 ] = -2.5;
    chances[ 1 ] = 0.050;  values[ 1 ] = -2.0;
    chances[ 2 ] = 0.100;  values[ 1 ] = -1.5;
    chances[ 3 ] = 0.200;  values[ 2 ] = -0.5;
    chances[ 4 ] = 0.250;  values[ 3 ] =  0.0;
    chances[ 5 ] = 0.200;  values[ 3 ] = +0.5;
    chances[ 6 ] = 0.100;  values[ 4 ] = +1.5;
    chances[ 7 ] = 0.050;  values[ 4 ] = +2.0;
    chances[ 8 ] = 0.025;  values[ 5 ] = +2.5;
  }
};

// ==========================================================================
// Roll Distribution
// ==========================================================================

struct roll_distribution_t : public distribution_t
{
  int distance;
  int num_rolls;
  double avg_fill;

  roll_distribution_t() : distribution_t( 200 ), distance( 0 ), num_rolls( 0 ), avg_fill( 0 ) {}

  virtual void fill()
  {
    double avg_expected = expected / num_rolls;
    if ( avg_fill > 0 && fabs( avg_expected - avg_fill ) < 0.01 ) return;
    double remainder = 1.0;
    for ( int i=0, s = ( int ) chances.size(); i < s; i++ )
    {
      last = i;
      values [ i ] = 1.0 - avg_expected * ( i + 1 );
      chances[ i ] = remainder * avg_expected;
      remainder -= chances[ i ];
      if ( remainder < 0.001 ) break;
    }
    chances[ last ] += remainder;
    avg_fill = avg_expected;
  }

  virtual int reach( double chance )
  {
    num_rolls++;
    expected += chance;
    if ( --distance < 0 )
    {
      fill();
      distance = next_bucket();
    }
    return ( distance == 0 ) ? 1 : 0;
  }
};

// ==========================================================================
// Normalized RNG via Simple Distance Tracking
// ==========================================================================

struct rng_distance_simple_t : public rng_normalized_t
{
  roll_distribution_t  roll_d;
  range_distribution_t range_d;
  gauss_distribution_t gauss_d;

  rng_distance_simple_t( const std::string& name, rng_t* b, bool avg_range=false, bool avg_gauss=false ) :
    rng_normalized_t( name, b, avg_range, avg_gauss )
  {
    roll_d.actual = real();
    range_d.actual = real();
    gauss_d.actual = real() * 2.5;
  }
  virtual rng_type type() const { return RNG_DISTANCE_SIMPLE; }

  virtual bool roll( double chance )
  {
    if ( chance <= 0 ) return 0;
    if ( chance >= 1 ) return 1;
    num_roll++;
    expected_roll += chance;
    if ( roll_d.reach( chance ) )
    {
      actual_roll++;
      return true;
    }
    return false;
  }

  virtual double range( double min, double max )
  {
    if ( average_range ) return ( min + max ) / 2.0;
    num_range++;
    double result = min + range_d.next_value() * ( max - min );
    expected_range += ( max - min ) / 2.0;
    actual_range += result;
    return result;
  }

  virtual double gauss( double mean, double stddev, bool /* truncate_low_end */ = false )
  {
    if ( average_gauss ) return mean;
    num_gauss++;
    double result = mean + gauss_d.next_value() * stddev;
    expected_gauss += mean;
    actual_gauss += result;
    return result;
  }

  virtual timespan_t gauss( timespan_t mean, timespan_t stddev )
  {
    return TIMESPAN_FROM_NATIVE_VALUE( gauss( TIMESPAN_TO_NATIVE_VALUE( mean ), TIMESPAN_TO_NATIVE_VALUE( stddev ) ) );
  }
};

// ==========================================================================
// Normalized RNG via Multiple Band Distance Tracking
// ==========================================================================

struct rng_distance_bands_t : public rng_distance_simple_t
{
  roll_distribution_t roll_bands[ 10 ];

  rng_distance_bands_t( const std::string& name, rng_t* b, bool avg_range=false, bool avg_gauss=false ) :
    rng_distance_simple_t( name, b, avg_range, avg_gauss )
  {
    for ( int i=0; i < 10; i++ ) roll_bands[ i ].actual = real() - 0.5;
  }
  virtual rng_type type() const { return RNG_DISTANCE_BANDS; }

  virtual bool roll( double chance )
  {
    if ( chance <= 0 ) return 0;
    if ( chance >= 1 ) return 1;
    num_roll++;
    expected_roll += chance;
    int band = ( int ) floor( chance * 9.9999 );
    if ( roll_bands[ band ].reach( chance ) )
    {
      actual_roll++;
      return true;
    }
    return false;
  }
};

// ==========================================================================
// Choosing the RNG package.........
// ==========================================================================

rng_t* rng_t::create( sim_t*             sim,
                      const std::string& name,
                      rng_type           type )
{
  if ( type == RNG_DEFAULT     ) type = RNG_PHASE_SHIFT;
  if ( type == RNG_CYCLIC      ) type = RNG_PHASE_SHIFT;
  if ( type == RNG_DISTRIBUTED ) type = RNG_DISTANCE_BANDS;

  rng_t* b = sim -> default_rng();

  bool ar = ( sim -> average_range != 0 );
  bool ag = ( sim -> average_gauss != 0 );

  switch ( type )
  {
  case RNG_STANDARD:         return new rng_std_t            ( name,    ar, ag );
  case RNG_MERSENNE_TWISTER: return new rng_sfmt_t           ( name,    ar, ag );
  case RNG_PHASE_SHIFT:      return new rng_phase_shift_t    ( name, b, ar, ag );
  case RNG_PRE_FILL:         return new rng_pre_fill_t       ( name, b, ar, ag );
  case RNG_DISTANCE_SIMPLE:  return new rng_distance_simple_t( name, b, ar, ag );
  case RNG_DISTANCE_BANDS:   return new rng_distance_bands_t ( name, b, ar, ag );
  default: assert( 0 );      return 0;
  }
}

#ifdef UNIT_TEST

int main( int argc, char** argv )
{
  range_distribution_t range_d;
  util_t::printf( "\nrange:\n" );
  for ( int i=1; i <= 100; i++ )
  {
    util_t::printf( "  %.2f", range_d.next_value() );
    if ( i % 10 == 0 ) util_t::printf( "\n" );
  }

  gauss_distribution_t gauss_d;
  util_t::printf( "\ngauss:\n" );
  for ( int i=1; i <= 100; i++ )
  {
    util_t::printf( "  %.2f", gauss_d.next_value() );
    if ( i % 10 == 0 ) util_t::printf( "\n" );
  }

  roll_distribution_t roll_d;
  util_t::printf( "\nroll:\n" );
  for ( int i=1; i <= 100; i++ )
  {
    util_t::printf( "  %d", roll_d.reach( 0.30 ) );
    if ( i % 10 == 0 ) util_t::printf( "\n" );
  }

  rng_t* rng = new rng_sfmt_t( "global", false, false );

  roll_distribution_t roll_d_05;
  util_t::printf( "\nroll 5%%:\n" );
  for ( int i=0; i < 1000000; i++ ) roll_d_05.reach( 0.05 );
  roll_d_05.verify( rng );

  roll_distribution_t roll_d_30;
  util_t::printf( "\nroll 30%%:\n" );
  for ( int i=0; i < 1000000; i++ ) roll_d_30.reach( 0.30 );
  roll_d_30.verify( rng );

  roll_distribution_t roll_d_80;
  util_t::printf( "\nroll 80%%:\n" );
  for ( int i=0; i < 1000000; i++ ) roll_d_80.reach( 0.80 );
  roll_d_80.verify( rng );
}

#endif

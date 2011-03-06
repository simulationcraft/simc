// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Standard Random Number Generator
// ==========================================================================

// rng_t::rng_t =============================================================

rng_t::rng_t( const std::string& n, bool avg_range, bool avg_gauss ) :
    name_str( n ), gauss_pair_use( false ),
    expected_roll( 0 ),  actual_roll( 0 ),  num_roll( 0 ),
    expected_range( 0 ), actual_range( 0 ), num_range( 0 ),
    expected_gauss( 0 ), actual_gauss( 0 ), num_gauss( 0 ),
    average_range( avg_range ),
    average_gauss( avg_gauss ),
    next( 0 )
{}

// rng_t::real ==============================================================

double rng_t::real()
{
  return rand() * ( 1.0 / ( ( ( double ) RAND_MAX ) + 1.0 ) );
}

// rng_t::roll ==============================================================

int rng_t::roll( double chance )
{
  if ( chance <= 0 ) return 0;
  if ( chance >= 1 ) return 1;
  num_roll++;
  int result = ( real() < chance ) ? 1 : 0;
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
                     double stddev )
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

  // True gaussian distribution can of course yield any number at some probability.  So truncate on the low end.
  double result = mean + z * stddev;
  if ( result < 0 ) result = 0;

  num_gauss++;
  expected_gauss += mean;
  actual_gauss += result;

  return result;
}

// rng_t::seed ==============================================================

void rng_t::seed( uint32_t start )
{
  srand( start );
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

// Mersenne Exponent. The period of the sequence is a multiple of 2^MEXP-1.
#define MEXP 1279

// SFMT generator has an internal state array of 128-bit integers, and N_sfmt is its size.
#define N_sfmt (MEXP / 128 + 1)

// N32 is the size of internal state array when regarded as an array of 32-bit integers.
#define N32 (N_sfmt * 4)

// MEXP_1279 paramaters
#define POS1    7
#define SL1     14
#define SL2     3
#define SR1     5
#define SR2     1
#define MSK1    0xf7fefffdU
#define MSK2    0x7fefcfffU
#define MSK3    0xaff3ef3fU
#define MSK4    0xb5ffff7fU
#define PARITY1 0x00000001U
#define PARITY2 0x00000000U
#define PARITY3 0x00000000U
#define PARITY4 0x20000000U

// a parity check vector which certificate the period of 2^{MEXP}
static uint32_t parity[4] = {PARITY1, PARITY2, PARITY3, PARITY4};

// 128-bit data structure
struct w128_t
{
  uint32_t u[4];
};

struct rng_sfmt_t : public rng_t
{
  w128_t sfmt[N_sfmt]; // the 128-bit internal state array

  uint32_t *psfmt32; // the 32bit integer pointer to the 128-bit internal state array

  int idx; // index counter to the 32-bit internal state array

  rng_sfmt_t( const std::string& name, bool avg_range=false, bool avg_gauss=false );

  virtual ~rng_sfmt_t() {}
  virtual int type() SC_CONST { return RNG_MERSENNE_TWISTER; }
  virtual double real();
  virtual void seed( uint32_t start );
};

// period_certification =====================================================

inline static void period_certification( w128_t* sfmt, uint32_t* psfmt32 )
{
  int inner = 0;
  int i, j;
  uint32_t work;

  for ( i = 0; i < 4; i++ )
    inner ^= psfmt32[i] & parity[i];
  for ( i = 16; i > 0; i >>= 1 )
    inner ^= inner >> i;
  inner &= 1;
  /* check OK */
  if ( inner == 1 )
  {
    return;
  }
  /* check NG, and modification */
  for ( i = 0; i < 4; i++ )
  {
    work = 1;
    for ( j = 0; j < 32; j++ )
    {
      if ( ( work & parity[i] ) != 0 )
      {
        psfmt32[i] ^= work;
        return;
      }
      work = work << 1;
    }
  }
}

// init_gen_rand ============================================================

inline static void init_gen_rand( rng_sfmt_t* r, uint32_t seed )
{
  w128_t*   sfmt    = r -> sfmt;
  uint32_t* psfmt32 = r -> psfmt32;

  int i;

  psfmt32[0] = seed;
  for ( i = 1; i < N32; i++ )
  {
    psfmt32[i] = 1812433253UL * ( psfmt32[i - 1] ^ ( psfmt32[i - 1] >> 30 ) ) + i;
  }
  period_certification( sfmt, psfmt32 );
}

// rshift128 ================================================================

inline static void rshift128( w128_t *out, w128_t const *in, int shift )
{
  uint64_t th, tl, oh, ol;

  th = ( ( uint64_t )in->u[3] << 32 ) | ( ( uint64_t )in->u[2] );
  tl = ( ( uint64_t )in->u[1] << 32 ) | ( ( uint64_t )in->u[0] );

  oh = th >> ( shift * 8 );
  ol = tl >> ( shift * 8 );
  ol |= th << ( 64 - shift * 8 );
  out->u[1] = ( uint32_t )( ol >> 32 );
  out->u[0] = ( uint32_t )ol;
  out->u[3] = ( uint32_t )( oh >> 32 );
  out->u[2] = ( uint32_t )oh;
}

// lshift128 ================================================================

inline static void lshift128( w128_t *out, w128_t const *in, int shift )
{
  uint64_t th, tl, oh, ol;

  th = ( ( uint64_t )in->u[3] << 32 ) | ( ( uint64_t )in->u[2] );
  tl = ( ( uint64_t )in->u[1] << 32 ) | ( ( uint64_t )in->u[0] );

  oh = th << ( shift * 8 );
  ol = tl << ( shift * 8 );
  oh |= tl >> ( 64 - shift * 8 );
  out->u[1] = ( uint32_t )( ol >> 32 );
  out->u[0] = ( uint32_t )ol;
  out->u[3] = ( uint32_t )( oh >> 32 );
  out->u[2] = ( uint32_t )oh;
}

// do_recursion =============================================================

inline static void do_recursion( w128_t *r, w128_t *a, w128_t *b, w128_t *c, w128_t *d )
{
  w128_t x;
  w128_t y;

  lshift128( &x, a, SL2 );
  rshift128( &y, c, SR2 );
  r->u[0] = a->u[0] ^ x.u[0] ^ ( ( b->u[0] >> SR1 ) & MSK1 ) ^ y.u[0] ^ ( d->u[0] << SL1 );
  r->u[1] = a->u[1] ^ x.u[1] ^ ( ( b->u[1] >> SR1 ) & MSK2 ) ^ y.u[1] ^ ( d->u[1] << SL1 );
  r->u[2] = a->u[2] ^ x.u[2] ^ ( ( b->u[2] >> SR1 ) & MSK3 ) ^ y.u[2] ^ ( d->u[2] << SL1 );
  r->u[3] = a->u[3] ^ x.u[3] ^ ( ( b->u[3] >> SR1 ) & MSK4 ) ^ y.u[3] ^ ( d->u[3] << SL1 );
}

// gen_rand_all =============================================================

inline static void gen_rand_all( w128_t*   sfmt,
                                 uint32_t* psfmt32 )
{
  int i;
  w128_t *r1, *r2;

  r1 = &sfmt[N_sfmt - 2];
  r2 = &sfmt[N_sfmt - 1];
  for ( i = 0; i < N_sfmt - POS1; i++ )
  {
    do_recursion( &sfmt[i], &sfmt[i], &sfmt[i + POS1], r1, r2 );
    r1 = r2;
    r2 = &sfmt[i];
  }
  for ( ; i < N_sfmt; i++ )
  {
    do_recursion( &sfmt[i], &sfmt[i], &sfmt[i + POS1 - N_sfmt], r1, r2 );
    r1 = r2;
    r2 = &sfmt[i];
  }
}

// gen_rand32 ===============================================================

inline static uint32_t gen_rand32( rng_sfmt_t* r )
{
  if ( r->idx >= N32 )
  {
    gen_rand_all( r->sfmt, r->psfmt32 );
    r->idx = 0;
  }
  return r->psfmt32[r->idx++];
}

// gen_rand_real ============================================================

inline static double gen_rand_real( rng_sfmt_t* r )
{
  return gen_rand32( r ) * ( 1.0 / 4294967295.0 );  // divide by 2^32-1
}

// rng_sfmt_t::rng_sfmt_t ===================================================

rng_sfmt_t::rng_sfmt_t( const std::string& name, bool avg_range, bool avg_gauss ) :
    rng_t( name, avg_range, avg_gauss )
{
  psfmt32 = &sfmt[0].u[0];
  idx = N32;

  init_gen_rand( this, rand() );
}

// rng_sfmt_t::real =========================================================

double rng_sfmt_t::real()
{
  return gen_rand_real( this );
}

// rng_sfmt_t::seed =========================================================

void rng_sfmt_t::seed( uint32_t start )
{
  init_gen_rand( this, start );
}

// ==========================================================================
// Base-Class for Normalized RNGs
// ==========================================================================

struct rng_normalized_t : public rng_t
{
  rng_t* base;

  rng_normalized_t( const std::string& name, rng_t* b, bool avg_range=false, bool avg_gauss=false ) :
      rng_t( name, avg_range, avg_gauss ), base( b ) {}
  virtual ~rng_normalized_t() {}

  virtual double real() { return base -> real(); }
  virtual double range( double min, double max ) { return ( min + max ) / 2.0; }
  virtual double gauss( double mean, double stddev ) { return mean; }
  virtual int    roll( double chance ) { assert( 0 ); return 0; } // must be overridden
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

    double gauss_offset[] = { 0.3, 0.5, 0.8, 1.3, 2.1 };
    gauss_distribution.resize( 10 );
    for ( int i=0; i < 5; i++ )
    {
      gauss_distribution[ i*2   ] = 0.0 + gauss_offset[ i ];
      gauss_distribution[ i*2+1 ] = 0.0 - gauss_offset[ i ];
    }
    gauss_index = ( int ) real() * 10;

    actual_roll = real() - 0.5;
  }
  virtual ~rng_phase_shift_t() {}
  virtual int type() SC_CONST { return RNG_PHASE_SHIFT; }

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
  virtual double gauss( double mean, double stddev )
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
  virtual int roll( double chance )
  {
    if ( chance <= 0 ) return 0;
    if ( chance >= 1 ) return 1;
    num_roll++;
    expected_roll += chance;
    if ( actual_roll < expected_roll )
    {
      actual_roll++;
      return 1;
    }
    return 0;
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
  virtual ~rng_pre_fill_t() {}
  virtual int type() SC_CONST { return RNG_PRE_FILL; }

  virtual int roll( double chance )
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
      return 1;
    }
    return 0;
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

  virtual double gauss( double mean, double stddev )
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
};

// ==========================================================================
// Distribution
// ==========================================================================

struct distribution_t
{
  int size, last, total_count;
  double actual, expected;
  std::vector<double> chances, values;
  std::vector<int> counts;

  distribution_t( int s ) :
      size( s ), last( s-1 ), total_count( 0 ), actual( 0 ), expected( 0 )
  {
    chances.insert( chances.begin(), size, 0.0 );
    values .insert( values .begin(), size, 0.0 );
    counts .insert( counts .begin(), size, 0   );
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
    std::vector<int> rng_counts;
    rng_counts.insert( rng_counts.begin(), size, 0 );
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
    return  distribution_t::next_bucket();
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
    for ( int i=0; i < size; i++ )
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
  virtual int type() SC_CONST { return RNG_DISTANCE_SIMPLE; }

  virtual int roll( double chance )
  {
    if ( chance <= 0 ) return 0;
    if ( chance >= 1 ) return 1;
    num_roll++;
    expected_roll += chance;
    if ( roll_d.reach( chance ) )
    {
      actual_roll++;
      return 1;
    }
    return 0;
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

  virtual double gauss( double mean, double stddev )
  {
    if ( average_gauss ) return mean;
    num_gauss++;
    double result = mean + gauss_d.next_value() * stddev;
    expected_gauss += mean;
    actual_gauss += result;
    return result;
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
  virtual int type() SC_CONST { return RNG_DISTANCE_BANDS; }

  virtual int roll( double chance )
  {
    if ( chance <= 0 ) return 0;
    if ( chance >= 1 ) return 1;
    num_roll++;
    expected_roll += chance;
    int band = ( int ) floor( chance * 9.9999 );
    if ( roll_bands[ band ].reach( chance ) )
    {
      actual_roll++;
      return 1;
    }
    return 0;
  }
};

// ==========================================================================
// Choosing the RNG package.........
// ==========================================================================

rng_t* rng_t::create( sim_t*             sim,
                      const std::string& name,
                      int                type )
{
  if ( type == RNG_DEFAULT     ) type = RNG_PHASE_SHIFT;
  if ( type == RNG_CYCLIC      ) type = RNG_PHASE_SHIFT;
  if ( type == RNG_DISTRIBUTED ) type = RNG_DISTANCE_BANDS;

  rng_t* b = sim -> deterministic_roll ? sim -> deterministic_rng : sim -> rng;

  bool ar = ( bool ) ( sim -> average_range != 0 );
  bool ag = ( bool ) ( sim -> average_gauss != 0 );

  switch ( type )
  {
  case RNG_STANDARD:         return new rng_t                ( name,    ar, ag );
  case RNG_MERSENNE_TWISTER: return new rng_sfmt_t           ( name,    ar, ag );
  case RNG_PHASE_SHIFT:      return new rng_phase_shift_t    ( name, b, ar, ag );
  case RNG_PRE_FILL:         return new rng_pre_fill_t       ( name, b, ar, ag );
  case RNG_DISTANCE_SIMPLE:  return new rng_distance_simple_t( name, b, ar, ag );
  case RNG_DISTANCE_BANDS:   return new rng_distance_bands_t ( name, b, ar, ag );
  }
  assert( 0 );
  return 0;
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


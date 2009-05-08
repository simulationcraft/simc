// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Standard Random Number Generator
// ==========================================================================

// rng_t::real ==============================================================

double rng_t::real()
{
  return rand() * ( 1.0 / ( ( (double) RAND_MAX ) + 1.0 ) );
}

// rng_t::roll ==============================================================

int rng_t::roll( double chance )
{
  if( chance <= 0 ) return 0;
  if( chance >= 1 ) return 1;

  return ( real() < chance ) ? 1 : 0;
}

// rng_t::range =============================================================

double rng_t::range( double min,
                     double max )
{
  if( min == max ) return min;

  assert( min <= max );

  return min + real() * ( max - min );
}

// rng_t::gaussian ==========================================================

double rng_t::gaussian( double mean,
                        double stddev )
{
  // This code adapted from ftp://ftp.taygeta.com/pub/c/boxmuller.c
  // Implements the Polar form of the Box-Muller Transformation
  //
  //                  (c) Copyright 1994, Everett F. Carter Jr.
  //                      Permission is granted by the author to use
  //                      this software for any application provided this
  //                      copyright notice is preserved.

  double x1, x2, w, y1, y2, z;

  if( gaussian_pair_use )
  {
    z = gaussian_pair_value;
    gaussian_pair_use = false;
  }
  else
  {
    do {
      x1 = 2.0 * real() - 1.0;
      x2 = 2.0 * real() - 1.0;
      w = x1 * x1 + x2 * x2;
    } while ( w >= 1.0 );
    
    w = sqrt( (-2.0 * log( w ) ) / w );
    y1 = x1 * w;
    y2 = x2 * w;

    z = y1;
    gaussian_pair_value = y2;
    gaussian_pair_use = true;
  }

  // True gaussian distribution can of course yield any number at some probability.  So truncate on the low end.
  double value = mean + z * stddev;

  return( value > 0 ? value : 0 );
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

// SFMT generator has an internal state array of 128-bit integers, and N is its size.
#define N (MEXP / 128 + 1)

// N32 is the size of internal state array when regarded as an array of 32-bit integers.
#define N32 (N * 4)

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
  w128_t sfmt[N]; // the 128-bit internal state array 

  uint32_t *psfmt32; // the 32bit integer pointer to the 128-bit internal state array 

  int idx; // index counter to the 32-bit internal state array 
  
  rng_sfmt_t( const std::string& name );

  virtual ~rng_sfmt_t() {}
  virtual double real();
};

// period_certification =====================================================

inline static void period_certification( w128_t* sfmt, uint32_t* psfmt32 ) 
{
    int inner = 0;
    int i, j;
    uint32_t work;

    for (i = 0; i < 4; i++)
        inner ^= psfmt32[i] & parity[i];
    for (i = 16; i > 0; i >>= 1)
        inner ^= inner >> i;
    inner &= 1;
    /* check OK */
    if (inner == 1) {
        return;
    }
    /* check NG, and modification */
    for (i = 0; i < 4; i++) {
        work = 1;
        for (j = 0; j < 32; j++) {
            if ((work & parity[i]) != 0) {
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
    for (i = 1; i < N32; i++) {
        psfmt32[i] = 1812433253UL * (psfmt32[i - 1] ^ (psfmt32[i - 1] >> 30)) + i;
    }
    period_certification( sfmt, psfmt32 );
}

// rshift128 ================================================================

inline static void rshift128(w128_t *out, w128_t const *in, int shift) 
{
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[3] << 32) | ((uint64_t)in->u[2]);
    tl = ((uint64_t)in->u[1] << 32) | ((uint64_t)in->u[0]);

    oh = th >> (shift * 8);
    ol = tl >> (shift * 8);
    ol |= th << (64 - shift * 8);
    out->u[1] = (uint32_t)(ol >> 32);
    out->u[0] = (uint32_t)ol;
    out->u[3] = (uint32_t)(oh >> 32);
    out->u[2] = (uint32_t)oh;
}

// lshift128 ================================================================

inline static void lshift128(w128_t *out, w128_t const *in, int shift) 
{
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[3] << 32) | ((uint64_t)in->u[2]);
    tl = ((uint64_t)in->u[1] << 32) | ((uint64_t)in->u[0]);

    oh = th << (shift * 8);
    ol = tl << (shift * 8);
    oh |= tl >> (64 - shift * 8);
    out->u[1] = (uint32_t)(ol >> 32);
    out->u[0] = (uint32_t)ol;
    out->u[3] = (uint32_t)(oh >> 32);
    out->u[2] = (uint32_t)oh;
}

// do_recursion =============================================================

inline static void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *c, w128_t *d) 
{
    w128_t x;
    w128_t y;

    lshift128(&x, a, SL2);
    rshift128(&y, c, SR2);
    r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SR1) & MSK1) ^ y.u[0] ^ (d->u[0] << SL1);
    r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SR1) & MSK2) ^ y.u[1] ^ (d->u[1] << SL1);
    r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SR1) & MSK3) ^ y.u[2] ^ (d->u[2] << SL1);
    r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SR1) & MSK4) ^ y.u[3] ^ (d->u[3] << SL1);
}

// gen_rand_all =============================================================

inline static void gen_rand_all( w128_t*   sfmt,
                                 uint32_t* psfmt32 ) 
{
    int i;
    w128_t *r1, *r2;

    r1 = &sfmt[N - 2];
    r2 = &sfmt[N - 1];
    for (i = 0; i < N - POS1; i++) {
        do_recursion(&sfmt[i], &sfmt[i], &sfmt[i + POS1], r1, r2);
        r1 = r2;
        r2 = &sfmt[i];
    }
    for (; i < N; i++) {
        do_recursion(&sfmt[i], &sfmt[i], &sfmt[i + POS1 - N], r1, r2);
        r1 = r2;
        r2 = &sfmt[i];
    }
}

// gen_rand32 ===============================================================

inline static uint32_t gen_rand32(rng_sfmt_t* r)
{
    if (r->idx >= N32) {
        gen_rand_all(r->sfmt, r->psfmt32);
        r->idx = 0;
    }
    return r->psfmt32[r->idx++];
}

// gen_rand_real ============================================================

inline static double gen_rand_real(rng_sfmt_t* r)
{
  return gen_rand32(r) * ( 1.0 / 4294967295.0 );  // divide by 2^32-1 
}

// rng_sfmt_t::rng_sfmt_t ===================================================

rng_sfmt_t::rng_sfmt_t( const std::string& n ) : rng_t(n)
{ 
  psfmt32 = &sfmt[0].u[0]; 
  idx = N32; 

  init_gen_rand( this, rand() );
}

// rng_sfmt_t::real ==========================================================

double rng_sfmt_t::real()
{
  return gen_rand_real( this );
}

// ==========================================================================
// Normalized RNG with Phase Shift
// ==========================================================================

struct normalized_rng_t : public rng_t
{
  rng_t* base;
  double fixed_phase_shift;
  double actual, expected;

  normalized_rng_t( const std::string& n, rng_t* b, double fps ) :
    rng_t(n), base(b), fixed_phase_shift(fps), actual(0), expected(0)
  {
    if( fixed_phase_shift == 0 ) fixed_phase_shift = real();
  }
  virtual ~normalized_rng_t() {}

  virtual double real() { return base -> real(); }
  virtual double range( double min, double max ) { return ( min + max ) / 2.0; }
  virtual double gaussian( double mean, double stddev ) { return mean; }

  virtual int roll( double chance )
  {
    if( chance <= 0 ) return 0;
    if( chance >= 1 ) return 1;

    expected += chance;

    double phase_shift = fixed_phase_shift;
    
    if( phase_shift >= 1.0 ) phase_shift *= real();
    
    if( ( actual + phase_shift ) < expected )
    {
      actual++;
      return 1;
    }

    return 0;
  }
};

// ==========================================================================
// Normalized RNG with Distance Tracking
// ==========================================================================

struct normalized_distance_rng_t : public normalized_rng_t
{
  static const int maxN4 = 400;
  int nTries;
  int lastOk;
  int nextGive;
  int nit[maxN4 + 2];
  int N4, N1;
  double lastAvg;

  normalized_distance_rng_t( const std::string& n, rng_t* b ) :
    normalized_rng_t( n, b, 0 ), nTries(0), lastOk(0)
  {
    for (int i = 0; i < maxN4 + 2; i++) nit[i] = 0;
  }
  virtual ~normalized_distance_rng_t() {}

  void setN4(double expP)
  {
    lastAvg = expP;
    N1 = (int) (1 / expP);
    if (N1 > maxN4) N1 = maxN4;
    N4 = 4 * N1;
    if (N4 > maxN4) N4 = maxN4;
    nextGive = N1;
  }

  void resetN4(double expP)
  {
    lastAvg = expP;
    N1 = (int)(1 / expP);
    if (N1 > maxN4) N1 = maxN4;
    int newN4 = 4 * N1;
    if (newN4 > maxN4) newN4 = maxN4;
    if (newN4 > N4)
    {
      int sumOver = 0;
      for (int i = N4 + 2; i <= newN4 + 1; i++) sumOver += nit[i];
      nit[N4 + 1] -= sumOver;
    }
    else
    {
      int sumOver = 0;
      for (int i = newN4 + 2; i <= N4 + 1; i++) sumOver += nit[i];
      nit[newN4 + 1] += sumOver;
    }
    N4=newN4;
  }

  void findNextGive()
  {
    double avgP = expected / nTries;

    // check if average chance is significantly different, to increase number of tracked distances
    if ((lastAvg-avgP) / lastAvg > 0.10) 
      resetN4(avgP);

    // find position that is lagging most
    double exTot = avgP * nTries;
    double exP = avgP * exTot;
    double avg1P = 1 - avgP;
    double sumExp = 0;
    double worst = -1;
    int worstID = 0;
    for (int i = 1; i <= N4+1; i++)
    {
      if (i > N4) 
	exP = exTot - sumExp;
      double difP = exP - nit[i];
      if (difP > worst)
      {
	worstID = i;
	worst = difP;
      }
      sumExp += exP;
      exP *= avg1P;
    }
    if (worstID <= 0)
    {
      worstID = N1;
    }
    // fill that position
    nextGive = worstID;
  }

  virtual int roll( double chance )
  {
    if( chance <= 0 ) return 0;
    if( chance >= 1 ) return 1;

    expected += chance;

    if( ++nTries <= 10 )
    {
      if( nTries == 10 ) // switch to distance-based formula
      {
	double avgP = expected / nTries;
	setN4( avgP );
      }

      double phase_shift = fixed_phase_shift;
      if( phase_shift >= 1.0 ) phase_shift *= real();
    
      if( ( actual + phase_shift ) < expected )
      {
	actual++;
	lastOk = nTries;
	return 1;
      }
    }
    else
    {
      nextGive--;
      if( nextGive <= 0 )
      {
	actual++;
	int dist = nTries - lastOk;
	if( dist > N4 ) dist = N4 + 1;
	nit[dist]++;
	lastOk = nTries;
	findNextGive();
	return 1;
      }
      return 0;
    }

    return 0;
  }
};

// ==========================================================================
// Choosing the RNG package.........
// ==========================================================================

rng_t* rng_t::create( sim_t*             sim,
		      const std::string& name,
		      int                rng_type )
{
  if( rng_type == RNG_STD )
  {
    return new rng_t( name );
  }
  else if( rng_type == RNG_SFMT )
  {
    return new rng_sfmt_t( name );
  }
  else if( rng_type == RNG_NORM_PHASE )
  {
    return new normalized_rng_t( name, sim -> rng, sim -> variable_phase_shift );
  }
  else if( rng_type == RNG_NORM_DISTANCE )
  {
    return new normalized_distance_rng_t( name, sim -> rng );
  }
  assert(0);
  return 0;
}


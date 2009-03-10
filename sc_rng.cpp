// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

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
#define POS1	7
#define SL1	14
#define SL2	3
#define SR1	5
#define SR2	1
#define MSK1	0xf7fefffdU
#define MSK2	0x7fefcfffU
#define MSK3	0xaff3ef3fU
#define MSK4	0xb5ffff7fU
#define PARITY1	0x00000001U
#define PARITY2	0x00000000U
#define PARITY3	0x00000000U
#define PARITY4	0x20000000U

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
  
  rng_sfmt_t();

  virtual ~rng_sfmt_t() {}
  virtual double real();
};

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

inline static uint32_t gen_rand32(rng_sfmt_t* r)
{
    if (r->idx >= N32) {
        gen_rand_all(r->sfmt, r->psfmt32);
	r->idx = 0;
    }
    return r->psfmt32[r->idx++];
}

inline static double gen_rand_real(rng_sfmt_t* r)
{
  return gen_rand32(r) * ( 1.0 / 4294967295.0 );  // divide by 2^32-1 
}

// ==========================================================================
// SFMT Random Number Generator
// ==========================================================================

// rng_sfmt_t::rng_sfmt_t ===================================================

rng_sfmt_t::rng_sfmt_t() 
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
  assert( min <= max );

  return min + real() * ( max - min );
}

// ==========================================================================
// Choosing the RNG package.........
// ==========================================================================

rng_t* rng_t::init( int sfmt )
{
  if( sfmt )
  {
    return new rng_sfmt_t();
  }
  else
  {
    return new rng_t();
  }
}


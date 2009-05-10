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

    r1 = &sfmt[N_sfmt - 2];
    r2 = &sfmt[N_sfmt - 1];
    for (i = 0; i < N_sfmt - POS1; i++) {
        do_recursion(&sfmt[i], &sfmt[i], &sfmt[i + POS1], r1, r2);
        r1 = r2;
        r2 = &sfmt[i];
    }
    for (; i < N_sfmt; i++) {
        do_recursion(&sfmt[i], &sfmt[i], &sfmt[i + POS1 - N_sfmt], r1, r2);
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

rng_sfmt_t::rng_sfmt_t( const std::string& name ) : rng_t(name)
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

  normalized_rng_t( const std::string& name, rng_t* b, double fps ) :
    rng_t(name), base(b), fixed_phase_shift(fps), actual(0), expected(0)
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
    
    if( phase_shift > 1.0 ) phase_shift *= real();
    
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

// ====   struct norm_distribution_t   =====
// utility class that ensure desired distribution and average in "fastest" way
// used by range(), gauss() [and roll() in future]
// Parameters for constructor:
//   - avg: desired average value
//   - N_elements:  number of possible elements/values that will be kept/distributed
//   - values_n: value of each element in array [0..n-1]
//   - chance_n: distribution probability that this element will be chosen
// Since it is for internal use, those using it must ensure:
//   - that supplied distribution probabilities (chance_n) add to 1.00
//   - that average of values_n * chances_n equals supplied "avg"
// Methods:
//   - getNextID(): return next element that should be used [main method]
//   - getNextValue(): same as above, only return value of element
//   - change_N(): change number of tracked elements. 
//     Note: caller must previously resize his chances_n, values_n
//   - change_avg(): change desired average value

struct norm_distribution_t{
private:
public:
	int* counter;
	int N;
	int nTries;

	double* chances;
	double* values;
	int arrSz;
	double currSum;
	bool invalidated;

	norm_distribution_t(int N_elements){
		N=N_elements;
		if (N<=0) N=1;
		arrSz=N*3/2+2;
		counter= new int[arrSz];
		chances= new double[arrSz];
		values= new double[arrSz];
		memset(counter,0,arrSz*sizeof(int));
		memset(chances,0,arrSz*sizeof(double));
		memset(values,0,arrSz*sizeof(double));
		nTries=0;
		invalidated=false;
		currSum=0;
	}

	~norm_distribution_t(){
		delete chances;
		delete values;
		delete counter;
	}


	void change_N(int newN){
		// do we need resizing of arrays?
		if (newN>=arrSz){
			int newSz= newN*3/2+2;
			//create larger arrays
			int* n_counter= new int[newSz];
			double* n_chances= new double[newSz];
			double* n_values= new double[newSz];
			memset(n_counter,0,newSz*sizeof(int));
			memset(n_chances,0,newSz*sizeof(double));
			memset(n_values,0,newSz*sizeof(double));
			//copy old content
			for (int i=0; i<arrSz; i++){
				n_counter[i]= counter[i];
				n_chances[i]=chances[i];
				n_values[i]=values[i];
			}
			// delete old ones and link new ones
			delete counter;
			delete chances;
			delete values;
			counter=n_counter;
			chances=n_chances;
			values=n_values;
			arrSz=newSz;
		}
		// expand counters
		if (newN > N)
		{
		  int sumOver = 0;
		  for (int i = N; i < newN; i++) sumOver += counter[i];
		  counter[N-1] -= sumOver;
		}
		else
		// compress counters
		if (newN < N)
		{
		  int sumOver = 0;
		  for (int i = newN; i < N; i++) sumOver += counter[i];
		  counter[newN-1] += sumOver;
		}
		N=newN;
		invalidated=true;
	}
	
	// main logic of this class
	int getNextID(double avgP){
		// increase number of tries, and calculate current average
		nTries++;
		if (invalidated){
			currSum=0;
			for (int i=0; i<N; i++) currSum+=counter[i]*values[i];
			invalidated=false;
		}
		// find next best suitable: one that is lagging most behind expected number of occurences
		int selected_ID = -1;
		int selected_lag = 0;
		double selected_avg_diff=0;
		for (int i = 0; i < N; i++)
		{
			// difference to expected number of occurences for this element
			int lag = (int) floor(nTries* chances[i] - counter[i] + 0.5);
			// how would selection of this element result in average difference
			double avg_diff= abs((currSum+values[i])/nTries - avgP);
			// now see if this is best choice so far
			if ( (selected_ID<0) || (lag>selected_lag) || 
				 ((lag==selected_lag)&&(avg_diff<selected_avg_diff))
    			) 
			{
				selected_ID=i;
				selected_lag=lag;
				selected_avg_diff= avg_diff;
			}
		}
		// use that position
		counter[selected_ID]++;
		currSum+=values[selected_ID];
		return selected_ID;
	}

	double getNextValue(double avgP){
		return values[getNextID(avgP)];
	}
	
};


// class that expose normalized random to users
struct normalized_distance_rng_t : public normalized_rng_t
{
  // roll variables
  norm_distribution_t* nd_roll;
  int nTries;
  int nextGive;
  double lastAvg;
  // gaussian variables
  norm_distribution_t* nd_gauss;
  // range variables
  norm_distribution_t* nd_range;

  normalized_distance_rng_t( const std::string& n, rng_t* b ) :
    normalized_rng_t( n, b, 0.5 ), nTries(0), nd_roll(0), nd_gauss(0), nd_range(0)
  {
	setGaussN(); // fixed to 7 values, so set here
	setRangeN(); // also fixed to 7 values 
  }

  virtual ~normalized_distance_rng_t() {
	  if (nd_gauss) delete nd_gauss;
	  if (nd_roll)  delete nd_roll;
	  if (nd_range) delete nd_range;
  }

  //set gauss values, fixed 7 elements, track +/- multiples of stddev [-3..+3]
  void setGaussN(){
		if (nd_gauss) return;
		int maxGN = 3;
		nd_gauss = new norm_distribution_t(2*maxGN+1);
		nd_gauss->values[maxGN]=0; // center value, 0
		nd_gauss->chances[maxGN+0]=0.382925;
		nd_gauss->chances[maxGN+1]=0.24173;
		nd_gauss->chances[maxGN+2]=0.060598;
		nd_gauss->chances[maxGN+3]=0.0062095;
		for (int i=1; i<=maxGN; i++){
			nd_gauss->values[maxGN+i]=i;
			nd_gauss->values[maxGN-i]=-i;
			nd_gauss->chances[maxGN-i]=nd_gauss->chances[maxGN+i];
		}
  }

  //set gauss values, fixed 7 elements, track +/- % of half-range  [-0.5..+0.5]
  void setRangeN(){
		if (nd_range) return;
		int maxGN = 3;
		nd_range = new norm_distribution_t(2*maxGN+1);
		for (int i=0; i<=maxGN; i++){
			nd_range->values[maxGN+i]=+i*0.5/maxGN;
			nd_range->values[maxGN-i]=-i*0.5/maxGN;
			nd_range->chances[maxGN+i]=1.0/(2*maxGN+1);
			nd_range->chances[maxGN-i]=1.0/(2*maxGN+1);
		}
  }

  // set roll values, dynamically based on avgP
  void setRollN(){
		double avgP = expected / nTries;
		int maxN4=400;
		int N=100;
		if (avgP>0.001) N = (int) (1 / avgP);
		if (N > maxN4) N = maxN4;
		int N4 = 4 * N;
		if (N4 > maxN4) N4 = maxN4;
		if (!nd_roll)
			nd_roll=new norm_distribution_t(N4+1);
		else
			nd_roll->change_N(N4+1);
		double exP = avgP;
		double sumExp = 0;
		for (int i = 0; i < N4; i++)
		{
			nd_roll->chances[i]=exP;
			nd_roll->values[i]=i+1;
		    sumExp += exP;
		    exP *= (1-avgP);
		}
		nd_roll->chances[N4]=1-sumExp;
		nd_roll->values[N4]=N4+1;
		nextGive=N;
		lastAvg=avgP;
  }

  int roll( double chance )
  {
    if( chance <= 0 ) return 0;
    if( chance >= 1 ) return 1;

	nTries++;
    expected += chance;

    if( !nd_roll )
    {
      if( nTries == 10 ) setRollN();// switch to distance-based formula
      if( ( actual + fixed_phase_shift ) < expected )
      {
			actual++;
			return 1;
      }
    }
    else
    {
      nextGive--;
      if( nextGive <= 0 )
      {
		actual++;
		// check if average chance is significantly different
		double avgP = expected / nTries;
		if ((lastAvg-avgP) / lastAvg > 0.10) 
			setRollN(); // increase number of tracked distances
		else
			nextGive= nd_roll->getNextID(1/chance)+1; // otherwise just take next
		return 1;
      }
      return 0;
    }
    return 0;
  }


  virtual double range( double min, double max ) { 
	  //get shift from middle
	  double rngShift= nd_range->getNextValue(0)*(max-min);
	  //return result
	  return ( min + max ) / 2.0 + rngShift; 
  }

  virtual double gaussian( double mean, double stddev ) {
	  //get how many "stddev" i should shift this time
	  double stdShift= nd_gauss->getNextValue(0)*stddev;
	  // limit to 0..2*mean
	  if (stdShift<-mean) stdShift=-mean;
	  if (stdShift>+mean) stdShift=+mean;
	  //return result
	  return mean+stdShift; 
  }

};


// used to test speed of convergence and correct distribution of possible values
// by inserting next line in main(), after sim_t is declared:
// testRng(&sim); return 1;
void testRng(sim_t* sim){
  // roll( test
  printf("Roll test\n\n");
  normalized_distance_rng_t* rngR= new normalized_distance_rng_t("test_roll", sim->rng);
  int lastRep=1;
  int nDsp=0;
  int lastOK=0, numOK=0;
  int distHist[100]={0};
  double p=0.1;
  int delta=10;
  for (int i=1; i<10000; i++){
	  bool ok=rngR->roll(p);
	  if (ok){
		  numOK++;
		  int dist=i-lastOK;
		  lastOK=i;
		  distHist[dist]++;
		  if (nDsp<10){
			  printf("%d,",dist);
			  nDsp++;
		  }
		  if (i>lastRep+delta){
			  char sb[100];
			  double avg=numOK/(i+0.0);
			  printf(" >> i=%d, OK=%d, A=%1.2lf \n", i, numOK, avg*100 );
			  std::string sr="";
			  double exTot = p * i;
			  double exP = p * exTot;
			  double avg1P = 1 - p;
			  for (int u=1; u<20; u++){
				  double diff= (exP-distHist[u])/exP*100;
				  if (diff<100)
					sprintf(sb,"[%d]=%1.0lf%%,", u, diff  );
				  else
				    sprintf(sb,"[%d]!,", u  );
				  sr+=sb;
    		      exP *= avg1P;
			  }
			  sr+="\n\n";
			  printf(sr.c_str());
			  lastRep=i;
			  nDsp=0;
			  if (i>delta*10) delta*=10;
		  }
	  }
  }
  // gauss test
  printf("Gauss test\n\n");
  normalized_distance_rng_t* rng= new normalized_distance_rng_t("test", sim->rng);
  lastRep=1;
  nDsp=0;
  double avgSum=0;
  for (int i=1; i<1000000; i++){
	  double n=rng->range(7,13);
	  avgSum+=n;
	  if (nDsp<10){
		  printf("%1.0lf,",n);
		  nDsp++;
	  }
	  if (i>=10*lastRep){
		  std::string sr="\n";
		  char sb[100];
		  sprintf(sb,"A=%1.2lf;", avgSum/i );
		  sr+=sb;
		  for (int u=0; u<7; u++){
			  double diff=rng->nd_gauss->counter[u]/(i+0.0);
			  diff=(diff-rng->nd_gauss->chances[u])/rng->nd_gauss->chances[u]*100;
			  sprintf(sb,"[%1.0lf]=%3.0lf%%,", rng->nd_gauss->values[u], diff  );
			  sr+=sb;
		  }
		  sr+="\n";
		  printf(sr.c_str());
		  lastRep=i;
		  nDsp=0;
	  }
  }
  char c;
  scanf("%c",&c);
}


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


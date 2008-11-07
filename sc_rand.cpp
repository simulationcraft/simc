// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

#if ! defined( NO_SFMT )
#define MEXP 1279
// If your math coprocessor does not support SSE2, then remove the following line
#include "./sfmt/SFMT.c"
#endif

void rand_t::init( uint32_t seed )
{
#if ! defined( NO_SFMT )
  init_gen_rand( seed );
#else
  srand( seed );
#endif
}

double rand_t::gen_float()
{
#if ! defined( NO_SFMT )
  return genrand_real1();
#else
  return rand() / ( ( (double) RAND_MAX ) + 1 );
#endif
}

int8_t rand_t::roll( double chance )
{
  if( chance <= 0 ) return 0;
  if( chance >= 1 ) return 1;

  return ( gen_float() < chance ) ? 1 : 0;
}

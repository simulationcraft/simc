// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

#ifdef USE_SFMT
#define MEXP 1279
#include <SFMT.c>
#endif

#define GRANULARITY 1000000

void rand_t::init( uint32_t seed )
{
#ifdef USE_SFMT
  init_gen_rand( seed );
#else
  srand( seed );
#endif
}

uint32_t rand_t::gen_uint32()
{
#ifdef USE_SFMT
   return gen_rand32();
#else
   return rand();
#endif
}

double rand_t::gen_float()
{
  return ( gen_uint32() % GRANULARITY ) / (double) GRANULARITY;
}

int8_t rand_t::roll( double chance )
{
  if( chance <= 0 ) return 0;
  if( chance >= 1 ) return 1;

  if( ( gen_uint32() % GRANULARITY ) < ( chance * GRANULARITY ) )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}


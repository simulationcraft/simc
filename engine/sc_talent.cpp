// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// talent_t::rank_value ====================================================

double talent_t::rank_value( double increment )
{
  return ranks() * increment;
}

// talent_t::rank_value ====================================================

double talent_t::rank_value( double zero,
			     double first, ... )
{
  int r = ranks();

  if( r <= 0 ) return zero;
  if( r == 1 ) return first;

  va_list vap;
  va_start( vap, first );

  double value=0;

  for ( int i=2; i <= r; i++ )
  {
    value = ( double ) va_arg( vap, double );
  }
  va_end( vap );

  return value;
}

// talent_t::rank_value ====================================================

int talent_t::rank_value( int increment )
{
  return ranks() * increment;
}

// talent_t::rank_value ====================================================

int talent_t::rank_value( int zero,
			  int first, ... )
{
  int r = ranks();

  if( r <= 0 ) return zero;
  if( r == 1 ) return first;

  va_list vap;
  va_start( vap, first );

  int value=0;

  for ( int i=2; i <= r; i++ )
  {
    value = ( int ) va_arg( vap, int );
  }
  va_end( vap );

  return value;
}


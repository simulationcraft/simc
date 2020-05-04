// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <cstdint>
#include <vector>

// Iteration data entry for replayability
struct iteration_data_entry_t
{
  double   metric;
  uint64_t seed;
  uint64_t iteration;
  double   iteration_length;
  std::vector <uint64_t> target_health;

  iteration_data_entry_t( double m, double il, uint64_t s, uint64_t h, uint64_t i ) :
    metric( m ), seed( s ), iteration( i ), iteration_length( il )
  { target_health.push_back( h ); }

  iteration_data_entry_t( double m, double il, uint64_t s, uint64_t i ) :
    metric( m ), seed( s ), iteration( i ), iteration_length( il )
  { }

  void add_health( uint64_t h )
  { target_health.push_back( h ); }
};
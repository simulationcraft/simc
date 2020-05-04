// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

struct player_t;

struct actor_pair_t
{
  player_t* target;
  player_t* source;

  actor_pair_t( player_t* target, player_t* source )
    : target( target ), source( source )
  {}

  actor_pair_t( player_t* p = nullptr )
    : target( p ), source( p )
  {}
};

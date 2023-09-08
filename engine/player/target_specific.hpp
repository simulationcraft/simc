// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "util/generic.hpp"

#include <vector>

struct player_t;

namespace target_specific_helper
{
size_t get_actor_index( const player_t* player );
}

template <class T>
struct target_specific_t
{
  bool owner_;

public:
  target_specific_t( bool owner = true ) : owner_( owner ) {}

  T*& operator[]( const player_t* target ) const
  {
    assert( target );
    auto target_index = target_specific_helper::get_actor_index( target );
    if ( data.size() <= target_index )
    {
      data.resize( target_index + 1 );
    }
    return data[ target_index ];
  }

  ~target_specific_t()
  {
    if ( owner_ )
      range::dispose( data );
  }

  const std::vector<T*>& get_entries()
  {
    return data;
  }

private:
  mutable std::vector<T*> data;
};

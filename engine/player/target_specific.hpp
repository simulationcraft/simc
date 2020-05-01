// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "generic.hpp"
#include "player/sc_player.hpp"
#include <vector>

template < class T >
struct target_specific_t
{
  bool owner_;
public:
  target_specific_t( bool owner = true ) : owner_( owner )
  { }

  T*& operator[](  const player_t* target ) const
  {
    assert( target );
    if ( data.size() <= target -> actor_index )
    {
      data.resize( target -> actor_index + 1 );
    }
    return data[ target -> actor_index ];
  }
  ~target_specific_t()
  {
    if ( owner_ )
      range::dispose( data );
  }
private:
  mutable std::vector<T*> data;
};

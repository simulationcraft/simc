// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "sc_enums.hpp"
#include "target_specific.hpp"

#include <vector>

struct actor_target_data_t;
struct player_t;
struct special_effect_t;
struct spell_data_t;

/**
 * Targetdata initializer. When targetdata is constructed (due to a call to player_t::get_target_data failing to find an
 * object for the given target), all targetdata initializers in the sim are invoked. Most class specific targetdata is
 * handled by the derived class-specific targetdata, however there are a couple of trinkets or systems that require
 * "generic" targetdata support. Item targetdata initializers will create the relevant debuffs/buffs needed. Note that
 * the buff/debuff needs to be always be created or assigned the generic sim fallback buff, since expressions for
 * buffs/debuffs presume the buff exists or the whole sim fails to init.
 *
 * See unique_gear::register_target_data_initializers() for currently supported target-based debuffs
 * in generic items.
 */
template <typename T>
struct targetdata_initializer_t
{
private:
  // track if init already performed on actor
  target_specific_t<player_t> init_list;

protected:
  // data obj cached per actor
  target_specific_t<T> data;

public:
  // debuff spell data cached per actor to account for player-scoped overrides
  target_specific_t<const spell_data_t> debuffs;
  // called to check if data is active
  std::function<bool( T* )> active_fn;
  // called when debuff spell data is first cached
  std::function<const spell_data_t*( player_t*, T* )> debuff_fn;

  targetdata_initializer_t() : init_list( false ), data( false ), debuffs( false ) {}

  virtual ~targetdata_initializer_t() = default;

  // returns the data
  virtual T* find( player_t* ) const = 0;

  // populate player-specific data obj & debuff spell data, returns true if data found
  virtual bool init( player_t* p ) const
  {
    assert( active_fn );
    assert( debuff_fn );

    player_t*& check = init_list[ p ];
    if ( check )
      return active_fn( data[ p ] );
    else
      check = p;

    auto d = find( p );
    data[ p ] = d;

    if ( active_fn( d ) )
    {
      debuffs[ p ] = debuff_fn( p, d );
      return true;
    }

    return false;
  }

  // override to initialize the targetdata object
  virtual void operator()( actor_target_data_t* ) const = 0;
};

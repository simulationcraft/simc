// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_EFFECT_HPP
#define ITEM_EFFECT_HPP

#include "util/array_view.hpp"

#include "client_data.hpp"

struct item_effect_t
{
  unsigned id;
  unsigned spell_id;
  int      index;
  int      type;
  int      cooldown_duration;
  int      cooldown_group;
  int      cooldown_group_duration;

  static const item_effect_t& find( unsigned id, bool ptr )
  { return dbc::find<item_effect_t>( id, ptr ); }

  static const item_effect_t& nil()
  { return dbc::nil<item_effect_t>(); }

  static const arv::array_view<item_effect_t> data( bool ptr );
};

#endif /* ITEM_EFFECT_HPP */

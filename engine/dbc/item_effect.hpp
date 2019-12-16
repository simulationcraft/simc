// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_EFFECT_HPP
#define ITEM_EFFECT_HPP

#include "util/array_view.hpp"

struct item_effect_t
{
  unsigned id;
  unsigned spell_id;
  int      index;
  int      type;
  int      cooldown_duration;
  int      cooldown_group;
  int      cooldown_group_duration;

  static const item_effect_t& find( unsigned id, bool ptr = false );
  static const item_effect_t& nil();
  static arv::array_view<item_effect_t> data( bool ptr = false );
};

#endif /* ITEM_EFFECT_HPP */

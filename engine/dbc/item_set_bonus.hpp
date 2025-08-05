// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_SET_BONUS_HPP
#define ITEM_SET_BONUS_HPP

#include "util/span.hpp"

#include "specialization.hpp"

#include "client_data.hpp"

#define SET_BONUS_ITEM_ID_MAX ( 17 )

struct item_set_bonus_t
{
  const char* set_name;
  const char* set_opt_name;
  const char* tier;
  unsigned    enum_id; // tier_e enum value.
  unsigned    set_id;
  unsigned    bonus;
  int         class_id;
  int         spec; // -1 "all"
  int         trait_sub_tree;
  unsigned    spell_id;
  unsigned    item_ids[SET_BONUS_ITEM_ID_MAX];

  bool has_spec( int spec_id ) const;
  bool has_spec( specialization_e ) const;
  bool has_trait_sub_tree( int trait_sub_tree_id ) const;
  bool has_item( unsigned item_id ) const;

  static util::span<const item_set_bonus_t> data( bool ptr );
};

#endif /* ITEM_SET_BONUS_HPP */


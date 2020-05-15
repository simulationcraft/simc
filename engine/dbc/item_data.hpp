// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_HPP
#define ITEM_HPP

#include "util/span.hpp"

#include "client_data.hpp"
#include "data_enums.hh"

#define MAX_ITEM_EFFECT 5
#define MAX_ITEM_STAT 10
#define MAX_ITEM_SOCKET_SLOT 3

struct item_data_t {
  unsigned id;
  const char* name;
  unsigned flags_1;
  unsigned flags_2;
  unsigned type_flags;
  int      level;                 // Ilevel
  int      req_level;
  int      req_skill;
  int      req_skill_level;
  int      quality;
  int      inventory_type;
  int      item_class;
  int      item_subclass;
  int      bind_type;
  double   delay;
  double   dmg_range;
  double   item_modifier;
  uint64_t race_mask;
  unsigned class_mask;
  int      stat_type_e[MAX_ITEM_STAT];       // item_mod_type
  int      stat_alloc[MAX_ITEM_STAT];
  double   stat_socket_mul[MAX_ITEM_STAT];
  int      trigger_spell[MAX_ITEM_EFFECT];      // item_spell_trigger_type
  int      id_spell[MAX_ITEM_EFFECT];
  int      cooldown_duration[MAX_ITEM_EFFECT];
  int      cooldown_group[MAX_ITEM_EFFECT];
  int      cooldown_group_duration[MAX_ITEM_EFFECT];
  int      socket_color[MAX_ITEM_SOCKET_SLOT];       // item_socket_color
  int      gem_properties;
  int      id_socket_bonus;
  int      id_set;
  int      id_scaling_distribution;
  unsigned id_artifact;

  bool is_armor()
  { return item_class == ITEM_CLASS_ARMOR && ( item_subclass >= ITEM_SUBCLASS_ARMOR_CLOTH && item_subclass <= ITEM_SUBCLASS_ARMOR_SHIELD ); }
  bool warforged() const
  { return ( type_flags & RAID_TYPE_WARFORGED ) == RAID_TYPE_WARFORGED; }
  bool lfr() const
  { return ( type_flags & RAID_TYPE_LFR ) == RAID_TYPE_LFR; }
  bool normal() const
  { return type_flags == 0; }
  bool heroic() const
  { return ( type_flags & RAID_TYPE_HEROIC ) == RAID_TYPE_HEROIC; }
  bool mythic() const
  { return ( type_flags & RAID_TYPE_MYTHIC ) == RAID_TYPE_MYTHIC; }

  static const item_data_t& find( unsigned id, bool ptr )
  { return dbc::find<item_data_t>( id, ptr ); }

  static const item_data_t& nil()
  { return dbc::nil<item_data_t>(); }

  static util::span<const item_data_t> data( bool ptr );
};

#endif /* ITEM_HPP */




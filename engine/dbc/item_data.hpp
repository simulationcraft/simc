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

struct dbc_item_data_t {
  struct stats_t {
    int16_t type_e; // item_mod_type
    int16_t alloc;
    float   socket_mul;
  };

  const char* name;
  unsigned id;
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
  float    delay;
  float    dmg_range;
  float    item_modifier;
  const stats_t* _dbc_stats;
  uint8_t  _dbc_stats_count;
  unsigned class_mask;
  uint64_t race_mask;
  int      socket_color[MAX_ITEM_SOCKET_SLOT];       // item_socket_color
  int      gem_properties;
  int      id_socket_bonus;
  int      id_set;
  int      id_curve;
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

  static const dbc_item_data_t& find( unsigned id, bool ptr );

  static const dbc_item_data_t& nil()
  { return dbc::nil<dbc_item_data_t>; }

  static util::span<const util::span<const dbc_item_data_t>> data( bool ptr );
};

#endif /* ITEM_HPP */

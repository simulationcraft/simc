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

  struct effect_t {
    unsigned spell_id;
    int16_t  type; // item_spell_trigger_type
    int16_t  cooldown_group;
    int      cooldown_duration;
    int      cooldown_group_duration;
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
  const effect_t* _effects;
  uint8_t  _dbc_stats_count;
  uint8_t  _effects_count;
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

  util::span<const effect_t> effects() const
  {
    assert( _effects || _effects_count == 0 );
    return { _effects, _effects_count };
  }

  static const dbc_item_data_t& find( unsigned id, bool ptr )
  { return dbc::find<dbc_item_data_t>( id, ptr, &dbc_item_data_t::id ); }

  static const dbc_item_data_t& nil()
  { return dbc::nil<dbc_item_data_t>; }

  static util::span<const dbc_item_data_t> data( bool ptr );

protected:
  dbc_item_data_t( const dbc_item_data_t& ) = default;
  dbc_item_data_t& operator=( const dbc_item_data_t& ) = default;

  void copy_from( const dbc_item_data_t& other )
  { *this = other; }
};

#endif /* ITEM_HPP */

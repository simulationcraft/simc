#ifndef DATA_DEFINITIONS_HH
#define DATA_DEFINITIONS_HH

#include "config.hpp"
#include "data_enums.hh"
#include "specialization.hpp"
#include <cstddef>

// Spell.dbc

#ifdef __OpenBSD__
#pragma pack(1)
#else
#pragma pack( push, 1 )
#endif

struct spell_data_t;
struct spelleffect_data_t;
struct spellpower_data_t;
struct talent_data_t;

struct item_child_equipment_t
{
  unsigned id;
  unsigned id_item;
  unsigned id_child;
};

struct artifact_t
{
  unsigned id;
  unsigned id_spec;
};

struct artifact_power_data_t
{
  unsigned    id;
  unsigned    id_artifact;
  unsigned    power_type;
  unsigned    power_index;
  unsigned    max_rank;
  unsigned    power_spell_id;
  const char* name; // In reality, the spell name

  static const artifact_power_data_t* nil()
  {
    static artifact_power_data_t __nil;
    return &( __nil );
  }
};

struct item_name_description_t
{
  unsigned id;
  const char* description;
};

struct scaling_stat_distribution_t
{
  unsigned id;
  unsigned min_level;
  unsigned max_level;
  unsigned curve_id;
};

struct curve_point_t
{
  unsigned curve_id;
  unsigned index;
  double   val1;
  double   val2;
};

struct item_bonus_tree_entry_t
{
  unsigned id;
  int      tree_id;
  unsigned index; // Unsure
  unsigned child_id; // Child item_bouns_tree_entry_t
  unsigned bonus_id; // Node ID in ItemBonus.db2
};

struct item_bonus_node_entry_t
{
  unsigned id;
  unsigned item_id;
  unsigned tree_id;
};

struct item_bonus_entry_t
{
  unsigned id;
  unsigned bonus_id;
  unsigned type;
  int      value_1;
  int      value_2;
  unsigned index;
};

struct item_set_bonus_t {
#define SET_BONUS_ITEM_ID_MAX ( 10 )
  const char* set_name;
  const char* set_opt_name;
  unsigned    enum_id; // tier_e enum value.
  unsigned    set_id;
  unsigned    tier;
  unsigned    bonus;
  int         class_id;
  int         spec; // -1 "all"
  unsigned    spell_id;
  unsigned    item_ids[SET_BONUS_ITEM_ID_MAX];

  bool has_spec( int spec_id ) const
  {
    // Check dbc-driven specs
    if ( spec > 0 )
    {
      if ( spec_id == spec )
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    // Check all specs
    else if ( spec == -1 )
    {
      return true;
    }
    // Give up
    else
    {
      return false;
    }
  }

  bool has_item( unsigned item_id ) const
  {
    for ( size_t i = 0; i < SET_BONUS_ITEM_ID_MAX; i++ )
    {
      if ( item_ids[ i ] == item_id )
        return true;

      if ( item_ids[ i ] == 0 )
        break;
    }
    return false;
  }
};

struct rppm_modifier_t {
  unsigned         spell_id;
  unsigned         type;
  unsigned         modifier_type;
  double           coefficient;
};

struct item_upgrade_rule_t {
  unsigned id;
  unsigned upgrade_id;
  unsigned item_id;
};

struct item_upgrade_t {
  unsigned id;
  unsigned upgrade_group;
  unsigned previous_upgrade_id;
  unsigned ilevel_delta;
};

struct random_prop_data_t {
  unsigned ilevel;
  unsigned damage_replace_stat;
  double   p_epic[5];
  double   p_rare[5];
  double   p_uncommon[5];
};

struct random_suffix_group_t {
  unsigned    id;
  unsigned    suffix_id[48];
};

struct item_enchantment_data_t {
  unsigned    id;
  int         slot;
  unsigned    id_gem;
  int         id_scaling;
  unsigned    min_scaling_level;   // need to verify these
  unsigned    max_scaling_level;
  unsigned    req_skill;
  unsigned    req_skill_value;
  unsigned    ench_type[3];        // item_enchantment
  int         ench_amount[3];
  unsigned    ench_prop[3];        // item_mod_type
  double      ench_coeff[3];       // item enchant scaling multiplier for data table
  unsigned    id_spell;            // reverse mapped spell id for this enchant
  const char* name;
};

#define MAX_ITEM_EFFECT 5
#define MAX_ITEM_STAT 10

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
  int      socket_color[3];       // item_socket_color
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

  static item_data_t* find( unsigned id, bool ptr = false );
};

struct item_scale_data_t {
  unsigned ilevel;
  double   values[7];             // quality based values for dps
};

struct item_armor_type_data_t {
  unsigned ilevel;
  double   armor_type[4];
};

struct item_socket_cost_data_t {
  unsigned ilevel;
  double   cost;
};

struct gem_property_data_t {
  unsigned id;
  unsigned enchant_id;
  unsigned color;
  unsigned min_ilevel;
};


#ifdef __OpenBSD__
#pragma pack()
#else
#pragma pack( pop )
#endif

#endif


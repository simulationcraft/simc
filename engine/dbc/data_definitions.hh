#ifndef DATA_DEFINITIONS_HH
#define DATA_DEFINITIONS_HH

#include "data_enums.hh"
#include "specialization.hpp"

// Spell.dbc

#ifdef __OpenBSD__
#pragma pack(1)
#else
#pragma pack( push, 1 )
#endif

struct spell_data_t;
struct spelleffect_data_t;
struct talent_data_t;

struct rppm_modifier_t {
  unsigned         spell_id;
  specialization_e spec;
  double           coefficient;
};

struct item_upgrade_rule_t {
  unsigned id;
  unsigned upgrade_ilevel;
  unsigned upgrade_id;
  unsigned item_id;
};

struct item_upgrade_t {
  unsigned id;
  unsigned ilevel;
};

struct random_prop_data_t {
  unsigned ilevel;
  double   p_epic[5];
  double   p_rare[5];
  double   p_uncommon[5];
};

struct random_suffix_data_t {
  unsigned    id;
  const char* suffix;
  unsigned    enchant_id[5];
  unsigned    enchant_alloc[5];
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
  unsigned      class_mask;
  unsigned      race_mask;
  int      stat_type_e[10];       // item_mod_type
  int      stat_val[10];
  int      stat_alloc[10];
  double   stat_socket_mul[10];
  int      trigger_spell[5];      // item_spell_trigger_type
  int      id_spell[5];
  int      cooldown_category[5];
  int      cooldown_category_duration[5];
  int      cooldown_group[5];
  int      cooldown_group_duration[5];
  int      socket_color[3];       // item_socket_color
  int      gem_properties;
  int      id_socket_bonus;
  int      id_set;
  int      id_suffix_group;

  bool is_armor()
  { return item_class == ITEM_CLASS_ARMOR && ( item_subclass >= ITEM_SUBCLASS_ARMOR_CLOTH && item_subclass <= ITEM_SUBCLASS_ARMOR_SHIELD ); }
  bool normal() const
  { return type_flags == 0; }
  bool heroic() const
  { return ( type_flags & RAID_TYPE_HEROIC ) == RAID_TYPE_HEROIC; }
  bool lfr() const
  { return ( type_flags & RAID_TYPE_LFR ) == RAID_TYPE_LFR; }
  bool flex() const
  { return ( type_flags & RAID_TYPE_FLEXIBLE ) == RAID_TYPE_FLEXIBLE; }
  bool elite() const
  { return ( type_flags & RAID_TYPE_ELITE ) == RAID_TYPE_ELITE; }
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

struct glyph_property_data_t {
  unsigned id;
  unsigned spell_id;
};

#ifdef __OpenBSD__
#pragma pack()
#else
#pragma pack( pop )
#endif

#endif


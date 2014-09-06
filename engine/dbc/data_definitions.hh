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

struct item_bonus_tree_entry_t
{
  unsigned id;
  unsigned tree_id;
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
#define SPEC_GUESS_MAX ( 4 )
#define SET_BONUS_ITEM_ID_MAX ( 10 )
  const char* set_name;
  unsigned    set_id;
  unsigned    tier;
  unsigned    bonus;
  unsigned    class_id;
  unsigned    spec_guess[SPEC_GUESS_MAX];
  unsigned    role; // 0 tank, 1 healer, 2 meleedps/hunter, 3 caster
  unsigned    spec;
  unsigned    spell_id;
  unsigned    item_ids[SET_BONUS_ITEM_ID_MAX];

  bool has_spec( unsigned spec_id ) const
  {
    if ( spec > 0 && spec_id == spec )
      return true;
    else if ( spec == 0 )
    {
      for ( size_t i = 0, end = SPEC_GUESS_MAX; i < end; i++ )
      {
        if ( spec_guess[ i ] == spec_id )
          return true;
      }
    }
    return false;
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


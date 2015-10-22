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
struct talent_data_t;

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
  const char* set_name;
  const char* set_opt_name;
  unsigned    enum_id; // tier_e enum value.
  unsigned    set_id;
  unsigned    tier;
  unsigned    bonus;
  int         class_id;
  std::array<int, 4>         spec_guesses;
  int         role; // 0 tank, 1 healer, 2 meleedps/hunter, 3 caster, -1 "all"
  int         spec; // -1 "all"
  unsigned    spell_id;
  std::array<unsigned, 10>    item_ids;

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
    // Check guessed specs
    else if ( spec_guesses.front() > 0 )
    {
      for ( const int spec_guess : spec_guesses )
      {
        if ( spec_guess == spec_id )
        {
          return true;
        }
      }
      return false;
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
    for ( const unsigned id : item_ids )
    {
      if ( id == item_id )
        return true;

      if ( id == 0 )
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
  std::array<double, 5>   p_epic;
  std::array<double, 5>   p_rare;
  std::array<double, 5>   p_uncommon;
};

struct random_suffix_data_t {
  unsigned    id;
  const char* suffix;
  std::array<unsigned, 5>    enchant_id;
  std::array<unsigned, 5>    enchant_alloc;
};

struct random_suffix_group_t {
  unsigned    id;
  std::array<unsigned, 48>    suffix_id;
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
  std::array<unsigned, 3>    ench_type;        // item_enchantment
  std::array<int, 3>         ench_amount;
  std::array<unsigned, 3>    ench_prop;        // item_mod_type
  std::array<double, 3>      ench_coeff;       // item enchant scaling multiplier for data table
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
  std::array<int,10> stat_type_e;       // item_mod_type
  std::array<int,10>      stat_val;
  std::array<int,10>      stat_alloc;
  std::array<double,10>   stat_socket_mul;
  std::array<int,5>      trigger_spell;      // item_spell_trigger_type
  std::array<int,5>      id_spell;
  std::array<int,5>      cooldown_category;
  std::array<int,5>      cooldown_category_duration;
  std::array<int,5>      cooldown_group;
  std::array<int,5>      cooldown_group_duration;
  std::array<int,3>      socket_color;       // item_socket_color
  int      gem_properties;
  int      id_socket_bonus;
  int      id_set;
  int      id_suffix_group;
  unsigned id_scaling_distribution;

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
  std::array<double,7>   values;             // quality based values for dps
};

struct item_armor_type_data_t {
  unsigned ilevel;
  std::array<double,4>   armor_type;
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


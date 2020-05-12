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

struct random_suffix_group_t {
  unsigned    id;
  unsigned    suffix_id[48];
};

struct item_scale_data_t {
  unsigned ilevel;
  double   values[7];             // quality based values for dps
};

struct item_socket_cost_data_t {
  unsigned ilevel;
  double   cost;
};


#ifdef __OpenBSD__
#pragma pack()
#else
#pragma pack( pop )
#endif

#endif


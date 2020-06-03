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


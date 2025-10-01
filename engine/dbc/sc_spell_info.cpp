// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_spell_info.hpp"

#include "dbc.hpp"
#include "dbc/covenant_data.hpp"
#include "dbc/item_set_bonus.hpp"
#include "dbc/trait_data.hpp"
#include "player/covenant.hpp"
#include "util/static_map.hpp"
#include "util/util.hpp"
#include "util/xml.hpp"

#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

static constexpr std::array<std::string_view, 4> _spell_type_map { {
  "None", "Magic", "Melee", "Ranged"
} };

static constexpr auto _hotfix_effect_map = util::make_static_map<unsigned, std::string_view>( {
  {  3, "Index"                   },
  {  4, "Type"                    },
  {  5, "Sub Type"                },
  {  6, "Coefficient"             },
  {  7, "Delta"                   },
  {  8, "Bonus"                   },
  {  9, "SP Coefficient"          },
  { 10, "AP Coefficient"          },
  { 11, "Period"                  },
  { 12, "Min Radius"              },
  { 13, "Max Radius"              },
  { 14, "Base Value"              },
  { 15, "Misc Value"              },
  { 16, "Misc Value 2"            },
  { 17, "Affects Spells"          },
  { 18, "Trigger Spell"           },
  { 19, "Chain Multiplier"        },
  { 20, "Points per Combo Points" },
  { 21, "Points per Level"        },
  { 22, "Mechanic"                },
  { 23, "Chain Targets"           },
  { 24, "Target 1"                },
  { 25, "Target 2"                },
  { 26, "Value Multiplier"        },
  { 27, "PvP Coefficient"         },
  { 28, "Scaling Class"           },
  { 29, "Attribute"               },
} );

static constexpr auto _hotfix_spell_map = util::make_static_map<unsigned, std::string_view>( {
  {  0, "Name"               },
  {  3, "Velocity"           },
  {  4, "School"             },
  {  5, "Class"              },
  {  6, "Race"               },
  {  7, "Scaling Spell"      },
  {  8, "Max Scaling Level"  },
  {  9, "Learn Level"        },
  { 10, "Max Spell Level"    },
  { 11, "Min Range"          },
  { 12, "Max Range"          },
  { 13, "Cooldown"           },
  { 14, "GCD"                },
  { 15, "Category Cooldown"  },
  { 16, "Charges"            },
  { 17, "Charge Cooldown"    },
  { 18, "Category"           },
  { 19, "Duration"           },
  { 20, "Max stacks"         },
  { 21, "Proc Chance"        },
  { 22, "Proc Stacks"        },
  { 23, "Proc Flags 1"       },
  { 24, "Internal Cooldown"  },
  { 25, "RPPM"               },
  { 26, "Item Class"         },
  { 27, "Inventory Mask"     },
  { 28, "Item Subclass"      },
  { 30, "Cast Time"          },
  { 35, "Attributes"         },
  { 36, "Affecting Spells"   },
  { 37, "Spell Family"       },
  { 38, "Stance Mask"        },
  { 39, "Mechanic"           },
  { 40, "Azerite Power Id"   },
  { 41, "Azerite Essence Id" },
  { 46, "Max Aura Level"     },
  { 47, "Spell Type"         },
  { 48, "Max Targets"        },
  { 49, "Required Level"     },
  { 50, "Travel Delay"       },
  { 51, "Min Travel Time"    },
  { 52, "Proc Flags 2"       },
  { 53, "Min Scaling Level"  },
  { 54, "Scale from ilevel"  },
  { 55, "Aura Interrupt"     },
  { 56, "Channel Interrupt"  },
  { 57, "Category Flags"     },
  { 58, "Category Type"      },
} );

static constexpr auto _hotfix_spelltext_map = util::make_static_map<unsigned, std::string_view>( {
  { 0, "Description" },
  { 1, "Tooltip"     },
  { 2, "Rank"        },
} );

static constexpr auto _hotfix_spelldesc_vars_map = util::make_static_map<unsigned, std::string_view>( {
  { 0, "Variables" },
} );

static constexpr auto _hotfix_power_map = util::make_static_map<unsigned, std::string_view>( {
  {  2, "Aura Id"               },
  {  4, "Power Type"            },
  {  5, "Cost"                  },
  {  6, "Max Cost"              },
  {  7, "Cost per Tick"         },
  {  8, "Percent Cost"          },
  {  9, "Max Percent Cost"      },
  { 10, "Percent Cost per Tick" }
} );

template <typename T, size_t N>
std::string map_string( const util::static_map<T, std::string_view, N>& map, T key )
{
  auto it = map.find( key );
  if ( it != map.end() )
    return fmt::format( "{} ({})", it->second, key );
  return fmt::format( "Unknown({})", key );
}

void print_hotfixes( fmt::memory_buffer& buf, util::span<const hotfix::client_hotfix_entry_t> hotfixes,
                     util::static_map_view<unsigned, std::string_view> map )
{
  for ( const auto& hotfix : hotfixes )
  {
    if ( buf.size() > 0 )
      fmt::format_to( std::back_inserter( buf ), ", " );

    auto entry = map.find( hotfix.field_id );
    if ( entry == map.end() )
      fmt::format_to( std::back_inserter( buf ), "Unknown({})", hotfix.field_id );
    else
      fmt::format_to( std::back_inserter( buf ), "{}", entry->second );

    switch ( hotfix.field_type )
    {
      case hotfix::UINT:
        fmt::format_to( std::back_inserter( buf ), " ({} -> {})", hotfix.orig_value.u, hotfix.hotfixed_value.u );
        break;
      case hotfix::INT:
        fmt::format_to( std::back_inserter( buf ), " ({} -> {})", hotfix.orig_value.i, hotfix.hotfixed_value.i );
        break;
      case hotfix::FLOAT:
        fmt::format_to( std::back_inserter( buf ), " ({} -> {})", hotfix.orig_value.f, hotfix.hotfixed_value.f );
        break;
      // Don't print out the changed string for now, seems pointless
      case hotfix::STRING:
        break;
      // Don't print out changed flags either (as there is no data for them)
      case hotfix::FLAGS:
        break;
    }
  }
}

std::string hotfix_map_str( util::span<const hotfix::client_hotfix_entry_t> hotfixes,
                            util::static_map_view<unsigned, std::string_view> map )
{
  fmt::memory_buffer s;
  print_hotfixes( s, hotfixes, map );
  return to_string( s );
}

template <typename Range, typename Callback>
std::string wrap_concatenate( Range&& data, Callback&& fn, size_t wrap, const std::string& delim = ", ",
                              const std::string& wrap_delim = ",\n                   " )
{
  if ( data.empty() )
    return "";

  std::stringstream s;
  size_t len = 0;

  for ( auto it = data.begin(); it != data.end(); it++ )
  {
    auto str = fn( *it );
    auto str_len = str.size();

    if ( it == data.begin() )
    {
      len = str_len;
      s << str;
    }
    else if ( wrap && len + str_len > wrap )
    {
      len = str_len;
      s << wrap_delim << str;
    }
    else
    {
      len += str_len;
      s << delim << str;
    }
  }

  return s.str();
}

template <typename Range>
std::string wrap_join( Range&& data, size_t wrap, const std::string& delim = ", ",
                       const std::string& wrap_delim = ",\n                   " )
{
  return wrap_concatenate( std::forward<Range>( data ), []( std::string_view s ) {
    return s;
  },wrap, delim, wrap_delim );
}

std::streamsize real_ppm_decimals( const spell_data_t* spell, const rppm_modifier_t& modifier )
{
  std::streamsize decimals = 3;
  double rppm_val = spell->real_ppm() * ( 1.0 + modifier.coefficient );
  if ( rppm_val >= 10 )
  {
    decimals += 2;
  }
  else if ( rppm_val >= 1 )
  {
    decimals += 1;
  }
  return decimals;
}

struct proc_map_entry_t
{
  uint64_t flag;
  std::string_view proc;
};
static constexpr std::array<proc_map_entry_t, 39> _proc_flag_map { {
  { PF_HEARTBEAT,              "Heartbeat"                   },
  { PF_KILLING_BLOW,           "Killing Blow"                },
  { PF_MELEE,                  "White Melee"                 },
  { PF_MELEE_TAKEN,            "White Melee Taken"           },
  { PF_MELEE_ABILITY,          "Yellow Melee"                },
  { PF_MELEE_ABILITY_TAKEN,    "Yellow Melee Taken"          },
  { PF_RANGED,                 "White Ranged"                },
  { PF_RANGED_TAKEN,           "White Ranged Taken"          },
  { PF_RANGED_ABILITY,         "Yellow Ranged"               },
  { PF_RANGED_ABILITY_TAKEN,   "Yellow Ranged Taken"         },
  { PF_NONE_HEAL,              "Generic Heal"                },
  { PF_NONE_HEAL_TAKEN,        "Generic Heal Taken"          },
  { PF_NONE_SPELL,             "Generic Hostile Spell"       },
  { PF_NONE_SPELL_TAKEN,       "Generic Hostile Spell Taken" },
  { PF_MAGIC_HEAL,             "Magic Heal"                  },
  { PF_MAGIC_HEAL_TAKEN,       "Magic Heal Taken"            },
  { PF_MAGIC_SPELL,            "Magic Hostile Spell"         },
  { PF_MAGIC_SPELL_TAKEN,      "Magic Hostile Spell Taken"   },
  { PF_PERIODIC,               "Periodic"                    },
  { PF_PERIODIC_TAKEN,         "Periodic Taken"              },
  { PF_ANY_DAMAGE_TAKEN,       "Any Damage Taken"            },
  { PF_HELPFUL_PERIODIC,       "Helpful Periodic"            },
  { PF_MAINHAND,               "Melee Main-Hand"             },
  { PF_OFFHAND,                "Melee Off-Hand"              },
  { PF_DEATH,                  "Death"                       },
  { PF_JUMP,                   "Proc on jump"                },
  { PF_CLONE_SPELL,            "Proc Clone Spell"            },
  { PF_ENTER_COMBAT,           "Enter Combat"                },
  { PF_ENCOUNTER_START,        "Encounter Start"             },
  { PF_CAST_ENDED,             "Cast Ended"                  },
  { PF_LOOTED,                 "Looted"                      },
  { PF_HELPFUL_PERIODIC_TAKEN, "Helpful Periodic Taken"      },
  { PF_TARGET_DIES,            "Target Dies"                 },
  { PF_KNOCKBACK,              "Knockback"                   },
  { PF_CAST_SUCCESSFUL,        "Cast Successful"             },
  { PF_UNKNOWN_36,             "Unknown 36"                  },
  { PF_UNKNOWN_37,             "Unknown 37"                  },
  { PF_UNKNOWN_38,             "Unknown 38"                  },
  { PF_UNKNOWN_39,             "Unknown 39"                  },
} };

struct class_map_entry_t
{
  const char* name;
  player_e pt;
};
static constexpr std::array<class_map_entry_t, 15> _class_map { {
  { nullptr,        PLAYER_NONE   },
  { "Warrior",      WARRIOR       },
  { "Paladin",      PALADIN       },
  { "Hunter",       HUNTER        },
  { "Rogue",        ROGUE         },
  { "Priest",       PRIEST        },
  { "Death Knight", DEATH_KNIGHT  },
  { "Shaman",       SHAMAN        },
  { "Mage",         MAGE          },
  { "Warlock",      WARLOCK       },
  { "Monk",         MONK          },
  { "Druid",        DRUID         },
  { "Demon Hunter", DEMON_HUNTER  },
  { "Evoker",       EVOKER        },
  { nullptr,        PLAYER_NONE   },
} };

static constexpr auto _race_map = util::make_static_map<unsigned, std::string_view>( {
  {  0, "Human"               },
  {  1, "Orc"                 },
  {  2, "Dwarf"               },
  {  3, "Night Elf"           },
  {  4, "Undead"              },
  {  5, "Tauren"              },
  {  6, "Gnome"               },
  {  7, "Troll"               },
  {  8, "Goblin"              },
  {  9, "Blood Elf"           },
  { 10, "Draenei"             },
  { 11, "Dark Iron Dwarf"     },
  { 12, "Vulpera"             },
  { 13, "Mag'har Orc"         },
  { 14, "Mechagnome"          },
  { 15, "Dracthyr"            },
  { 17, "Earthen"             },
  { 21, "Worgen"              },
  { 25, "Pandaren"            },
  { 26, "Nightborne"          },
  { 27, "Highmountain Tauren" },
  { 28, "Void Elf"            },
  { 29, "Lightforged Draenei" },
  { 30, "Zandalari Troll"     },
  { 31, "Kul Tiran"           },
} );

static constexpr auto _targeting_strings = util::make_static_map<unsigned, std::string_view>( {
  { T_UNIT_CASTER,                                   "Self"                                  },
  { T_UNIT_NEARBY_ENEMY,                             "Nearby Enemy"                          },
  { T_UNIT_NEARBY_ALLY,                              "Nearby Ally"                           },
  { T_UNIT_NEARBY_PARTY,                             "Nearby Party"                          },
  { T_UNIT_PET,                                      "Active Pet"                            },
  { T_UNIT_TARGET_ENEMY,                             "Targeted Enemy"                        },
  { T_UNIT_SOURCE_AREA,                              "any in Area"                           },
  { T_UNIT_DESTINATION_AREA,                         "at any in Area"                        },
  { T_DESTINATION_HOME,                              "at Home"                               },
  { T_UNIT_SOURCE_AREA_UNK_11,                       "Unknown in Area"                       },
  { T_UNIT_SOURCE_AREA_ENEMY,                        "Enemy in Area"                         },
  { T_UNIT_DESTINATION_AREA_ENEMY,                   "at Enemy in Area"                      },
  { T_DESTINATION_DB,                                "at Database Entry in Area"             },
  { T_DESTINATION_CASTER,                            "at Self"                               },
  { T_UNIT_CASTER_AREA_PARTY,                        "Party in Area"                         },
  { T_UNIT_TARGET_ALLY,                              "Targeted Ally"                         },
  { T_SOURCE_CASTER,                                 "Source"                                },
  { T_GAMEOBJECT_TARGET,                             "Targeted Gameobject"                   },
  { T_UNIT_CONE_ENEMY_24,                            "Enemies in Cone"                       },
  { T_UNIT_TARGET_ANY,                               "Current Target"                        },
  { T_GAMEOBJECT_ITEM_TARGET,                        "Targeted Item Gameobject"              },
  { T_UNIT_MASTER,                                   "Master"                                },
  { T_DESTINATION_DYNAMIC_OBJECT_ENEMY,              "at Dynamic Enemy in Area"              },
  { T_DESTINATION_DYNAMIC_OBJECT_ALLY,               "at Dynamic Ally in Area"               },
  { T_UNIT_SOURCE_AREA_ALLY,                         "Ally in Area"                          },
  { T_UNIT_DESTINATION_AREA_ALLY,                    "At Ally in Area"                       },
  { T_DESTINATION_CASTER_SUMMON,                     "Summoner in Area"                      },
  { T_UNIT_SOURCE_AREA_PARTY,                        "Party in Area"                         },
  { T_UNIT_DESTINATION_AREA_PARTY,                   "at Party in Area"                      },
  { T_UNIT_TARGET_PARTY,                             "Targeted Party Member"                 },
  { T_UNIT_LAST_TARGET_AREA_PARTY,                   "Last Targeted Party Member"            },
  { T_UNIT_NEARBY,                                   "Nearby Target"                         },
  { T_DESTINATION_CASTER_FISHING,                    "at Fishing Target"                     },
  { T_GAMEOBJECT_NEARBY,                             "Nearby Gameobject"                     },
  { T_DESTINATION_CASTER_FRONT_RIGHT,                "Front Right of Self"                   },
  { T_DESTINATION_CASTER_BACK_RIGHT,                 "Back Right of Self"                    },
  { T_DESTINATION_CASTER_BACK_LEFT,                  "Back Left of Self"                     },
  { T_DESTINATION_CASTER_FRONT_LEFT,                 "Front Left of Self"                    },
  { T_UNIT_TARGET_CHAINHEAL_ALLY,                    "Chain Heal Ally"                       },
  { T_DESTINATION_NEARBY_46,                         "at Nearby"                             },
  { T_DESTINATION_CASTER_FRONT,                      "Front of Self"                         },
  { T_DESTINATION_CASTER_BACK,                       "Back of Self"                          },
  { T_DESTINATION_CASTER_RIGHT,                      "Right of Self"                         },
  { T_DESTINATION_CASTER_LEFT,                       "Left of Self"                          },
  { T_GAMEOBJECT_SOURCE_AREA,                        "Gameobject in Area"                    },
  { T_GAMEOBJECT_DESTINATION_AREA,                   "at Gameobject in Area"                 },
  { T_DESTINATION_TARGET_ENEMY,                      "At Enemy"                              },
  { T_UNIT_CONE_180_DEG_ENEMY,                       "Enemies in Cone (180 degree)"          },
  { T_DEST_CASTER_FRONT_LEAP,                        "at Front of Leap Target"               },
  { T_UNIT_CASTER_AREA_RAID,                         "Raid in Area"                          },
  { T_UNIT_CASTER_TARGET_RAID,                       "Targeted Raid Member"                  },
  { T_UNIT_CASTER_NEARBY_RAID,                       "Raid Nearby"                           },
  { T_UNIT_CONE_ALLY,                                "Allies in Cone"                        },
  { T_UNIT_CONE,                                     "Any in Cone"                           },
  { T_UNIT_TAGET_AREA_RAID_CLASS,                    "Raid Class in Area"                    },
  { T_DESTINATION_CASTER_GROUND_62,                  "at Ground"                             },
  { T_DESTINATION_TARGET_ANY,                        "at Target"                             },
  { T_DESTINATION_TARGET_FRONT,                      "at Target Front"                       },
  { T_DESTINATION_TARGET_BACK,                       "at Target Back"                        },
  { T_DESTINATION_TARGET_RIGHT,                      "at Target Right"                       },
  { T_DESTINATION_TARGET_LEFT,                       "at Target Left"                        },
  { T_DESTINATION_TARGET_FRONT_RIGHT,                "at Target Front Right"                 },
  { T_DESTINATION_TARGET_BACK_RIGHT,                 "at Target Back Right"                  },
  { T_DESTINATION_TARGET_BACK_LEFT,                  "at Target Back Left"                   },
  { T_DESTINATION_TARGET_FRONT_LEFT,                 "at Target Front Left"                  },
  { T_DESTINATION_CASTER_RANDOM,                     "at Random"                             },
  { T_DESTINATION_CASTER_RADIUS,                     "at Radius"                             },
  { T_DESTINATION_TARGET_RANDOM,                     "at Target Random"                      },
  { T_DESTINATION_TARGET_RADIUS,                     "at Target Radius"                      },
  { T_DESTINATION_CHANNEL_TARGET,                    "at Channel Target"                     },
  { T_UNIT_CHANNEL_TARGET,                           "Channel Target"                        },
  { T_DESTINATION_DESTINATION_FRONT,                 "Front in Area"                         },
  { T_DESTINATION_DESTINATION_BACK,                  "Back in Area"                          },
  { T_DESTINATION_DESTINATION_RIGHT,                 "Right in Area"                         },
  { T_DESTINATION_DESTINATION_LEFT,                  "Left in Area"                          },
  { T_DESTINATION_DESTINATION_FRONT_RIGHT,           "Front Right in Area"                   },
  { T_DESTINATION_DESTINATION_BACK_RIGHT,            "Back Right in Area"                    },
  { T_DESTINATION_DESTINATION_BACK_LEFT,             "Back Left in Area"                     },
  { T_DESTINATION_DESTINATION_FRONT_LEFT,            "Front Left in Area"                    },
  { T_DESTINATION_DESTINATION_RANDOM,                "Random in Area"                        },
  { T_DESTINATION_DESTINATION,                       "at Area"                               },
  { T_DESTINATION_DYNAMIC_OBJECT_NONE,               "at Dynamic Object"                     },
  { T_DESTINATION_TRAJECTORY,                        "any in Trajectory"                     },
  { T_UNIT_TARGET_MINIPET,                           "Target's Battlepet"                    },
  { T_DESTINATION_DESTINATION_RADIUS,                "at Radius in Area"                     },
  { T_UNIT_SUMMONER,                                 "Summoner"                              },
  { T_CORPSE_SOURCE_AREA_ENEMY,                      "Enemy Corpse in Area"                  },
  { T_UNIT_VEHICLE,                                  "Vehicle"                               },
  { T_UNIT_TARGET_PASSENGER,                         "Passenger"                             },
  { T_UNIT_PASSENGER_0,                              "Passenger 0"                           },
  { T_UNIT_PASSENGER_1,                              "Passenger 1"                           },
  { T_UNIT_PASSENGER_2,                              "Passenger 2"                           },
  { T_UNIT_PASSENGER_3,                              "Passenger 3"                           },
  { T_UNIT_PASSENGER_4,                              "Passenger 4"                           },
  { T_UNIT_PASSENGER_5,                              "Passenger 5"                           },
  { T_UNIT_PASSENGER_6,                              "Passenger 6"                           },
  { T_UNIT_PASSENGER_7,                              "Passenger 7"                           },
  { T_UNIT_CONE_CASTER_TO_DESTINATION_ENEMY,         "Enemies in Cone to Targeted Enemy"     },
  { T_UNIT_CASTER_AND_PASSENGERS,                    "Self and Passengers"                   },
  { T_DESTINATION_NEARBY_DB,                         "Nearby in Database"                    },
  { T_DESTINATION_NEARBY_107,                        "At Nearby"                             },
  { T_GAMEOBJECT_CONE_CASTER_TO_DESTINATION_ENEMY,   "Gameobject in Cone to Targeted Enemy"  },
  { T_GAMEOBJECT_CONE_CASTER_TO_DESTINATION_ALLY,    "Gameobject in Cone to Targeted Ally"   },
  { T_UNIT_CONE_CASTER_TO_DESTINATION,               "Cone from Self"                        },
  { T_UNIT_SOURCE_AREA_FURTHEST_ENEMY,               "Furthest Enemy in Area"                },
  { T_UNIT_AND_DESTINATION_LAST_ENEMY,               "Last Targeted Enemy"                   },
  { T_UNIT_TARGET_ALLY_OR_RAID,                      "Targeted Ally or Raid"                 },
  { T_CORPSE_SOURCE_AREA_RAID,                       "Raid Corpse in Area"                   },
  { T_UNIT_SELF_AND_SUMMONS,                         "Self and Summons in Area"              },
  { T_CORPSE_TARGET_ALLY,                            "Targeted Ally Corpse"                  },
  { T_UNIT_AREA_THREAT_LIST,                         "Threat List in Area"                   },
  { T_UNIT_AREA_TAP_LIST,                            "Tap List in Area"                      },
  { T_UNIT_TARGET_TAP_LIST,                          "Targeted Tap List"                     },
  { T_DESTINATION_CASTER_GROUND_125,                 "at Ground"                             },
  { T_UNIT_CASTER_AREA_ENEMY_CLUMP,                  "Enemy Clump in Area"                   },
  { T_DESTINATION_CASTER_ENEMY_CLUMP_CENTROID,       "at Center of Enemy Clump in Area"      },
  { T_UNIT_RECT_CASTER_ALLY,                         "Target Reticule: Ally"                 },
  { T_UNIT_RECT_CASTER_ENEMY,                        "Target Reticule: Enemy"                },
  { T_UNIT_RECT_CASTER,                              "Target Reticule"                       },
  { T_UNIT_DESTINATION_SUMMONER,                     "at Summoner"                           },
  { T_UNIT_DESTINATION_TARGET_ALLY,                  "at Targeted Ally"                      },
  { T_UNIT_LINE_CASTER_TO_DESTINATION_ALLY,          "Line to Ally"                          },
  { T_UNIT_LINE_CASTER_TO_DESTINATION_ENEMY,         "Line to Enemy"                         },
  { T_UNIT_LINE_CASTER_TO_DESTINATION,               "Line from Self"                        },
  { T_UNIT_CONE_CASTER_TO_DESTINATION_ALLY,          "Cone to Ally"                          },
  { T_UNIT_DESTINATION_CASTER_MOVEMENT_DIRECTION,    "in Area of Movement Direction"         },
  { T_UNIT_DESTINATION_DESTINATION_GROUND,           "in Area of Ground Area"                },
  { T_DESTINATION_CASTER_CLUMP_CENTROID,             "at Center of Ally Clump in Area"       },
  { T_DESTINATION_NEARBY_ENTRY_OR_DB,                "Nearby: in Database or Entry"          },
  { T_DESTINATION_DESTINATION_TARGET_TOWARDS_CASTER, "in Area From Target to Self"           },
  { T_UNIT_OWN_CRITTER,                              "Own Critter"                           },
} );

static constexpr auto _resource_strings = util::make_static_map<int, std::string_view>( {
  { POWER_HEALTH,        "Health"        },
  { POWER_MANA,          "Base Mana"     },
  { POWER_RAGE,          "Rage"          },
  { POWER_FOCUS,         "Focus"         },
  { POWER_ENERGY,        "Energy"        },
  { POWER_COMBO_POINT,   "Combo Points"  },
  { POWER_RUNE,          "Rune"          },
  { POWER_RUNIC_POWER,   "Runic Power"   },
  { POWER_SOUL_SHARDS,   "Soul Shard"    },
  { POWER_ASTRAL_POWER,  "Astral Power"  },
  { POWER_HOLY_POWER,    "Holy Power"    },
  { POWER_MAELSTROM,     "Maelstrom"     },
  { POWER_CHI,           "Chi"           },
  { POWER_INSANITY,      "Insanity"      },
  { POWER_BURNING_EMBER, "Burning Ember" },
  { POWER_DEMONIC_FURY,  "Demonic Fury"  },
  { POWER_FURY,          "Fury"          },
  { POWER_PAIN,          "Pain"          },
  { POWER_ESSENCE,       "Essence"       },
} );

// Mappings from Classic EnumeratedString.db2
static constexpr auto _attribute_strings = util::make_static_map<unsigned, std::string_view>( {
  {    0, "Proc Failure Burns Charge"                                            },
  {    1, "Uses Ranged Slot"                                                     },
  {    2, "On Next Swing (No Damage)"                                            },
  {    3, "Do Not Log Immune Misses"                                             },
  {    4, "Is Ability"                                                           },
  {    5, "Is Tradeskill"                                                        },
  {    6, "Passive"                                                              },
  {    7, "Do Not Display (Spellbook, Aura Icon, Combat Log)"                    },
  {    8, "Do Not Log"                                                           },
  {    9, "Held Item Only"                                                       },
  {   10, "On Next Swing"                                                        },
  {   11, "Wearer Casts Proc Trigger"                                            },
  {   12, "Server Only"                                                          },
  {   13, "Allow Item Spell In PvP"                                              },
  {   14, "Only Indoors"                                                         },
  {   15, "Only Outdoors"                                                        },
  {   16, "Not Shapeshifted"                                                     },
  {   17, "Only Stealthed"                                                       },
  {   18, "Do Not Sheath"                                                        },
  {   19, "Scales w/ Creature Level"                                             },
  {   20, "Cancels Auto Attack Combat"                                           },
  {   21, "No Active Defense"                                                    },
  {   22, "Track Target in Cast (Player Only)"                                   },
  {   23, "Allow Cast While Dead"                                                },
  {   24, "Allow While Mounted"                                                  },
  {   25, "Cooldown On Event"                                                    },
  {   26, "Aura Is Debuff"                                                       },
  {   27, "Allow While Sitting"                                                  },
  {   28, "Not In Combat (Only Peaceful)"                                        },
  {   29, "No Immunities"                                                        },
  {   30, "Heartbeat Resist"                                                     },
  {   31, "No Aura Cancel"                                                       },
  {   32, "Dismiss Pet First"                                                    },
  {   33, "Use All Mana"                                                         },
  {   34, "Is Channelled"                                                        },
  {   35, "No Redirection"                                                       },
  {   36, "No Skill Increase"                                                    },
  {   37, "Allow While Stealthed"                                                },
  {   38, "Is Self Channelled"                                                   },
  {   39, "No Reflection"                                                        },
  {   40, "Only Peaceful Targets"                                                },
  {   41, "Initiates Combat (Enables Auto-Attack)"                               },
  {   42, "No Threat"                                                            },
  {   43, "Aura Unique"                                                          },
  {   44, "Failure Breaks Stealth"                                               },
  {   45, "Toggle Far Sight"                                                     },
  {   46, "Track Target in Channel"                                              },
  {   47, "Immunity Purges Effect"                                               },
  {   48, "Immunity to Hostile & Friendly Effects"                               },
  {   49, "No AutoCast (AI)"                                                     },
  {   50, "Prevents Anim"                                                        },
  {   51, "Exclude Caster"                                                       },
  {   52, "Finishing Move - Damage"                                              },
  {   53, "Threat only on Miss"                                                  },
  {   54, "Finishing Move - Duration"                                            },
  {   55, "Ignore Owner's Death"                                                 },
  {   56, "Special Skillup"                                                      },
  {   57, "Aura Stays After Combat"                                              },
  {   58, "Require All Targets"                                                  },
  {   59, "Discount Power On Miss"                                               },
  {   60, "No Aura Icon"                                                         },
  {   61, "Name in Channel Bar"                                                  },
  {   62, "Combo on Block (Mainline: Dispel All Stacks)"                         },
  {   63, "Cast When Learned"                                                    },
  {   64, "Allow Dead Target"                                                    },
  {   65, "No shapeshift UI"                                                     },
  {   66, "Ignore Line of Sight"                                                 },
  {   67, "Allow Low Level Buff"                                                 },
  {   68, "Use Shapeshift Bar"                                                   },
  {   69, "Auto Repeat"                                                          },
  {   70, "Cannot cast on tapped"                                                },
  {   71, "Do Not Report Spell Failure"                                          },
  {   72, "Include In Advanced Combat Log"                                       },
  {   73, "Always Cast As Unit"                                                  },
  {   74, "Special Taming Flag"                                                  },
  {   75, "No Target Per-Second Costs"                                           },
  {   76, "Chain From Caster"                                                    },
  {   77, "Enchant own item only"                                                },
  {   78, "Allow While Invisible"                                                },
  {   79, "Do Not Consume if Gained During Cast"                                 },
  {   80, "No Active Pets"                                                       },
  {   81, "Do Not Reset Combat Timers"                                           },
  {   82, "No Jump While Cast Pending"                                           },
  {   83, "Allow While Not Shapeshifted (caster form)"                           },
  {   84, "Initiate Combat Post-Cast (Enables Auto-Attack)"                      },
  {   85, "Fail on all targets immune"                                           },
  {   86, "No Initial Threat"                                                    },
  {   87, "Proc Cooldown On Failure"                                             },
  {   88, "Item Cast With Owner Skill"                                           },
  {   89, "Don't Block Mana Regen"                                               },
  {   90, "No School Immunities"                                                 },
  {   91, "Ignore Weaponskill"                                                   },
  {   92, "Not an Action"                                                        },
  {   93, "Can't Crit"                                                           },
  {   94, "Active Threat"                                                        },
  {   95, "Retain Item Cast"                                                     },
  {   96, "PvP Enabling"                                                         },
  {   97, "No Proc Equip Requirement"                                            },
  {   98, "No Casting Bar Text"                                                  },
  {   99, "Completely Blocked"                                                   },
  {  100, "No Res Timer"                                                         },
  {  101, "No Durability Loss"                                                   },
  {  102, "No Avoidance"                                                         },
  {  103, "DoT Stacking Rule"                                                    },
  {  104, "Only On Player"                                                       },
  {  105, "Not a Proc"                                                           },
  {  106, "Requires Main-Hand Weapon"                                            },
  {  107, "Only Battlegrounds"                                                   },
  {  108, "Only On Ghosts"                                                       },
  {  109, "Hide Channel Bar"                                                     },
  {  110, "Hide In Raid Filter"                                                  },
  {  111, "Normal Ranged Attack"                                                 },
  {  112, "Suppress Caster Procs"                                                },
  {  113, "Suppress Target Procs"                                                },
  {  114, "Always Hit"                                                           },
  {  115, "Instant Target Procs"                                                 },
  {  116, "Allow Aura While Dead"                                                },
  {  117, "Only Proc Outdoors"                                                   },
  {  118, "Casting Cancels Autorepeat (Mainline: Do Not Trigger Target Stand)"   },
  {  119, "No Damage History"                                                    },
  {  120, "Requires Off-Hand Weapon"                                             },
  {  121, "Treat As Periodic"                                                    },
  {  122, "Can Proc From Procs"                                                  },
  {  123, "Only Proc on Caster"                                                  },
  {  124, "Ignore Caster & Target Restrictions"                                  },
  {  125, "Ignore Caster Modifiers"                                              },
  {  126, "Do Not Display Range"                                                 },
  {  127, "Not On AOE Immune"                                                    },
  {  128, "No Cast Log"                                                          },
  {  129, "Class Trigger Only On Target"                                         },
  {  130, "Aura Expires Offline"                                                 },
  {  131, "No Helpful Threat"                                                    },
  {  132, "No Harmful Threat"                                                    },
  {  133, "Allow Client Targeting"                                               },
  {  134, "Cannot Be Stolen"                                                     },
  {  135, "Allow Cast While Casting"                                             },
  {  136, "Ignore Damage Taken Modifiers"                                        },
  {  137, "Combat Feedback When Usable"                                          },
  {  138, "Weapon Speed Cost Scaling"                                            },
  {  139, "No Partial Immunity"                                                  },
  {  140, "Aura Is Buff"                                                         },
  {  141, "Do Not Log Caster"                                                    },
  {  142, "Reactive Damage Proc"                                                 },
  {  143, "Not In Spellbook"                                                     },
  {  144, "Not In Arena or Rated Battleground"                                   },
  {  145, "Ignore Default Arena Restrictions"                                    },
  {  146, "Bouncy Chain Missiles"                                                },
  {  147, "Allow Proc While Sitting"                                             },
  {  148, "Aura Never Bounces"                                                   },
  {  149, "Allow Entering Arena"                                                 },
  {  150, "Proc Suppress Swing Anim"                                             },
  {  151, "Suppress Weapon Procs"                                                },
  {  152, "Auto Ranged Combat"                                                   },
  {  153, "Owner Power Scaling"                                                  },
  {  154, "Only Flying Areas"                                                    },
  {  155, "Force Display Castbar"                                                },
  {  156, "Ignore Combat Timer"                                                  },
  {  157, "Aura Bounce Fails Spell"                                              },
  {  158, "Obsolete"                                                             },
  {  159, "Use Facing From Spell"                                                },
  {  160, "Allow Actions During Channel"                                         },
  {  161, "No Reagent Cost With Aura"                                            },
  {  162, "Remove Entering Arena"                                                },
  {  163, "Allow While Stunned"                                                  },
  {  164, "Triggers Channeling"                                                  },
  {  165, "Limit N"                                                              },
  {  166, "Ignore Area Effect PvP Check"                                         },
  {  167, "Not On Player"                                                        },
  {  168, "Not On Player Controlled NPC"                                         },
  {  169, "Extra Initial Period"                                                 },
  {  170, "Do Not Display Duration"                                              },
  {  171, "Implied Targeting"                                                    },
  {  172, "Melee Chain Targeting"                                                },
  {  173, "Spell Haste Affects Periodic"                                         },
  {  174, "Not Available While Charmed"                                          },
  {  175, "Treat as Area Effect"                                                 },
  {  176, "Aura Affects Not Just Req. Equipped Item"                             },
  {  177, "Allow While Fleeing"                                                  },
  {  178, "Allow While Confused"                                                 },
  {  179, "AI Doesn't Face Target"                                               },
  {  180, "Do Not Attempt a Pet Resummon When Dismounting"                       },
  {  181, "Ignore Target Requirements"                                           },
  {  182, "Not On Trivial"                                                       },
  {  183, "No Partial Resists"                                                   },
  {  184, "Ignore Caster Requirements"                                           },
  {  185, "Always Line of Sight"                                                 },
  {  186, "Always AOE Line of Sight"                                             },
  {  187, "No Caster Aura Icon"                                                  },
  {  188, "No Target Aura Icon"                                                  },
  {  189, "Aura Unique Per Caster"                                               },
  {  190, "Always Show Ground Texture"                                           },
  {  191, "Add Melee Hit Rating"                                                 },
  {  192, "No Cooldown On Tooltip"                                               },
  {  193, "Do Not Reset Cooldown In Arena"                                       },
  {  194, "Not an Attack"                                                        },
  {  195, "Can Assist Immune PC"                                                 },
  {  196, "Ignore For Mod Time Rate"                                             },
  {  197, "Do Not Consume Resources"                                             },
  {  198, "Floating Combat Text On Cast"                                         },
  {  199, "Aura Is Weapon Proc"                                                  },
  {  200, "Do Not Chain To Crowd-Controlled Targets"                             },
  {  201, "Allow On Charmed Targets"                                             },
  {  202, "No Aura Log"                                                          },
  {  203, "Not In Raid Instances"                                                },
  {  204, "Allow While Riding Vehicle"                                           },
  {  205, "Ignore Phase Shift"                                                   },
  {  206, "AI Primary Ranged Attack"                                             },
  {  207, "No Pushback"                                                          },
  {  208, "No Jump Pathing"                                                      },
  {  209, "Allow Equip While Casting"                                            },
  {  210, "Originate From Controller"                                            },
  {  211, "Delay Combat Timer During Cast"                                       },
  {  212, "Aura Icon Only For Caster (Limit 10)"                                 },
  {  213, "Show Mechanic as Combat Text"                                         },
  {  214, "Absorb Cannot Be Ignore"                                              },
  {  215, "Taps immediately"                                                     },
  {  216, "Can Target Untargetable"                                              },
  {  217, "Doesn't Reset Swing Timer if Instant"                                 },
  {  218, "Vehicle Immunity Category"                                            },
  {  219, "Ignore Healing Modifiers"                                             },
  {  220, "Do Not Auto Select Target with Initiates Combat"                      },
  {  221, "Ignore Caster Damage Modifiers"                                       },
  {  222, "Disable Tied Effect Points"                                           },
  {  223, "No Category Cooldown Mods"                                            },
  {  224, "Allow Spell Reflection"                                               },
  {  225, "No Target Duration Mod"                                               },
  {  226, "Disable Aura While Dead"                                              },
  {  227, "Debug Spell"                                                          },
  {  228, "Treat as Raid Buff"                                                   },
  {  229, "Can Be Multi Cast"                                                    },
  {  230, "Don't Cause Spell Pushback"                                           },
  {  231, "Prepare for Vehicle Control End"                                      },
  {  232, "Horde Specific Spell"                                                 },
  {  233, "Alliance Specific Spell"                                              },
  {  234, "Dispel Removes Charges"                                               },
  {  235, "Can Cause Interrupt"                                                  },
  {  236, "Can Cause Silence"                                                    },
  {  237, "No UI Not Interruptible"                                              },
  {  238, "Recast On Resummon"                                                   },
  {  239, "Reset Swing Timer at spell start"                                     },
  {  240, "Only In Spellbook Until Learned"                                      },
  {  241, "Do Not Log PvP Kill"                                                  },
  {  242, "Attack on Charge to Unit"                                             },
  {  243, "Report Spell failure to unit target"                                  },
  {  244, "No Client Fail While Stunned, Fleeing, Confused"                      },
  {  245, "Retain Cooldown Through Load"                                         },
  {  246, "Ignores Cold Weather Flying Requirement"                              },
  {  247, "No Attack Dodge"                                                      },
  {  248, "No Attack Parry"                                                      },
  {  249, "No Attack Miss"                                                       },
  {  250, "Treat as NPC AoE"                                                     },
  {  251, "Bypass No Resurrect Aura"                                             },
  {  252, "Do Not Count For PvP Scoreboard"                                      },
  {  253, "Reflection Only Defends"                                              },
  {  254, "Can Proc From Suppressed Target Procs"                                },
  {  255, "Always Cast Log"                                                      },
  {  256, "No Attack Block"                                                      },
  {  257, "Ignore Dynamic Object Caster"                                         },
  {  258, "Remove Outside Dungeons and Raids"                                    },
  {  259, "Only Target If Same Creator"                                          },
  {  260, "Can Hit AOE Untargetable"                                             },
  {  261, "Allow While Charmed"                                                  },
  {  262, "Aura Required by Client"                                              },
  {  263, "Ignore Sanctuary"                                                     },
  {  264, "Use Target's Level for Spell Scaling"                                 },
  {  265, "Periodic Can Crit"                                                    },
  {  266, "Mirror creature name"                                                 },
  {  267, "Only Players Can Cast This Spell"                                     },
  {  268, "Aura Points On Client"                                                },
  {  269, "Not In Spellbook Until Learned"                                       },
  {  270, "Target Procs On Caster"                                               },
  {  271, "Requires location to be on liquid surface"                            },
  {  272, "Only Target Own Summons"                                              },
  {  273, "Haste Affects Duration"                                               },
  {  274, "Ignore Spellcast Override Cost"                                       },
  {  275, "Allow Targets Hidden by Spawn Tracking"                               },
  {  276, "Requires Equipped Inv Types"                                          },
  {  277, "No 'Summon + Dest from Client' Targeting Pathing Requirement"         },
  {  278, "Melee Haste Affects Periodic"                                         },
  {  279, "Enforce In Combat Ressurection Limit"                                 },
  {  280, "Heal Prediction"                                                      },
  {  281, "No Level Up Toast"                                                    },
  {  282, "Skip Is Known Check"                                                  },
  {  283, "AI Face Target"                                                       },
  {  284, "Not in Battleground"                                                  },
  {  285, "Mastery Affects Points"                                               },
  {  286, "Display Large Aura Icon On Unit Frames (Boss Aura)"                   },
  {  287, "Can Attack ImmunePC"                                                  },
  {  288, "Force Dest Location"                                                  },
  {  289, "Mod Invis Includes Party"                                             },
  {  290, "Only When Illegally Mounted"                                          },
  {  291, "Do Not Log Aura Refresh"                                              },
  {  292, "Missile Speed is Delay (in sec)"                                      },
  {  293, "Ignore Totem Requirements for Casting"                                },
  {  294, "Item Cast Grants Skill Gain"                                          },
  {  295, "Do Not Add to Unlearn List"                                           },
  {  296, "Cooldown Ignores Ranged Weapon"                                       },
  {  297, "Not In Arena"                                                         },
  {  298, "Target Must Be Grounded"                                              },
  {  299, "Allow While Banished Aura State"                                      },
  {  300, "Face unit target upon completion of jump charge"                      },
  {  301, "Haste Affects Melee Ability Casttime"                                 },
  {  302, "Ignore Default Rated Battleground Restrictions"                       },
  {  303, "Do Not Display Power Cost"                                            },
  {  304, "Next modal spell requires same unit target"                           },
  {  305, "AutoCast Off By Default"                                              },
  {  306, "Ignore School Lockout"                                                },
  {  307, "Allow Dark Simulacrum"                                                },
  {  308, "Allow Cast While Channeling"                                          },
  {  309, "Suppress Visual Kit Errors"                                           },
  {  310, "Spellcast Override In Spellbook"                                      },
  {  311, "JumpCharge - no facing control"                                       },
  {  312, "Ignore Caster Healing Modifiers"                                      },
  {  313, "(Programmer Only) Don't consume charge if item deleted"               },
  {  314, "Item Passive On Client"                                               },
  {  315, "Force Corpse Target"                                                  },
  {  316, "Cannot Kill Target"                                                   },
  {  317, "Log Passive"                                                          },
  {  318, "No Movement Radius Bonus"                                             },
  {  319, "Channel Persists on Pet Follow"                                       },
  {  320, "Bypass Visibility Check"                                              },
  {  321, "Ignore Positive Damage Taken Modifiers"                               },
  {  322, "Uses Ranged Slot (Cosmetic Only)"                                     },
  {  323, "Do Not Log Full Overheal"                                             },
  {  324, "NPC Knockback - ignore doors"                                         },
  {  325, "Force Non-Binary Resistance"                                          },
  {  326, "No Summon Log"                                                        },
  {  327, "Ignore instance lock and farm limit on teleport"                      },
  {  328, "Area Effects Use Target Radius"                                       },
  {  329, "Charge/JumpCharge - Use Absolute Speed"                               },
  {  330, "Proc cooldown on a per target basis"                                  },
  {  331, "Lock chest at precast"                                                },
  {  332, "Use Spell Base Level For Scaling"                                     },
  {  333, "Reset cooldown upon ending an encounter"                              },
  {  334, "Rolling Periodic"                                                     },
  {  335, "Spellbook Hidden Until Overridden"                                    },
  {  336, "Defend Against Friendly Cast"                                         },
  {  337, "Allow Defense While Casting"                                          },
  {  338, "Allow Defense While Channeling"                                       },
  {  339, "Allow Fatal Duel Damage"                                              },
  {  340, "Multi-Click Ground Targeting"                                         },
  {  341, "AoE Can Hit Summoned Invis"                                           },
  {  342, "Allow While Stunned By Horror Mechanic"                               },
  {  343, "Visible only to caster (conversations only)"                          },
  {  344, "Update Passives on Apply/Remove"                                      },
  {  345, "Normal Melee Attack"                                                  },
  {  346, "Ignore Feign Death"                                                   },
  {  347, "Caster Death Cancels Persistent Area Auras"                           },
  {  348, "Do Not Log Absorb"                                                    },
  {  349, "This Mount is NOT at the account level"                               },
  {  350, "Prevent Client Cast Cancel"                                           },
  {  351, "Enforce Facing on Primary Target Only"                                },
  {  352, "Lock Caster Movement and Facing While Casting"                        },
  {  353, "Don't Cancel When All Effects are Disabled"                           },
  {  354, "Scales with Casting Item's Level"                                     },
  {  355, "Do Not Log on Learn"                                                  },
  {  356, "Hide Shapeshift Requirements"                                         },
  {  357, "Absorb Falling Damage"                                                },
  {  358, "Unbreakable Channel"                                                  },
  {  359, "Ignore Caster's spell level"                                          },
  {  360, "Transfer Mount Spell"                                                 },
  {  361, "Ignore Spellcast Override Shapeshift Requirements"                    },
  {  362, "Newest Exclusive Complete"                                            },
  {  363, "Not in Instances"                                                     },
  {  364, "Obsolete"                                                             },
  {  365, "Ignore PvP Power"                                                     },
  {  366, "Can Assist Uninteractible"                                            },
  {  367, "Cast When Initial Logging In"                                         },
  {  368, "Not in Mythic+ Mode (Challenge Mode)"                                 },
  {  369, "Cheaper NPC Knockback"                                                },
  {  370, "Ignore Caster Absorb Modifiers"                                       },
  {  371, "Ignore Target Absorb Modifiers"                                       },
  {  372, "Hide Loss of Control UI"                                              },
  {  373, "Allow Harmful on Friendly"                                            },
  {  374, "Cheap Missile AOI"                                                    },
  {  375, "Expensive Missile AOI"                                                },
  {  376, "No Client Fail on No Pet"                                             },
  {  377, "AI Attempt Cast on Immune Player"                                     },
  {  378, "Allow While Stunned by Stun Mechanic"                                 },
  {  379, "Don't close loot window"                                              },
  {  380, "Hide Damage Absorb UI"                                                },
  {  381, "Do Not Treat As Area Effect"                                          },
  {  382, "Check Required Target Aura By Caster"                                 },
  {  383, "Apply Zone Aura Spell To Pets"                                        },
  {  384, "Enable Procs from Suppressed Caster Procs"                            },
  {  385, "Can Proc from Suppressed Caster Procs"                                },
  {  386, "Show Cooldown As Charge Up"                                           },
  {  387, "No PvP Battle Fatigue"                                                },
  {  388, "Treat Self Cast As Reflect"                                           },
  {  389, "Do Not Cancel Area Aura on Spec Switch"                               },
  {  390, "Cooldown on Aura Cancel Until Combat Ends"                            },
  {  391, "Do Not Re-apply Area Aura if it Persists Through Update"              },
  {  392, "Display Toast Message"                                                },
  {  393, "Active Passive"                                                       },
  {  394, "Ignore Damage Cancels Aura Interrupt"                                 },
  {  395, "Face Destination"                                                     },
  {  396, "Immunity Purges Spell"                                                },
  {  397, "Do Not Log Spell Miss"                                                },
  {  398, "Ignore Distance Check On Charge/Jump Charge Done Trigger Spell"       },
  {  399, "Disable known spells while charmed"                                   },
  {  400, "Ignore Damage Absorb"                                                 },
  {  401, "Not In Proving Grounds"                                               },
  {  402, "Override Default SpellClick Range"                                    },
  {  403, "Is In-Game Store Effect"                                              },
  {  404, "Allow during spell override"                                          },
  {  405, "Use float values for scaling amounts"                                 },
  {  406, "Suppress toasts on item push"                                         },
  {  407, "Trigger Cooldown On Spell Start"                                      },
  {  408, "Never Learn"                                                          },
  {  409, "No Deflect"                                                           },
  {  410, "(Deprecated) Use Start-of-Cast Location for Spell Dest"               },
  {  411, "Recompute Aura on Mercenary Mode"                                     },
  {  412, "Use Weighted Random For Flex Max Targets"                             },
  {  413, "Ignore Resilience"                                                    },
  {  414, "Apply Resilience To Self Damage"                                      },
  {  415, "Only Proc From Class Abilities"                                       },
  {  416, "Allow Class Ability Procs"                                            },
  {  417, "Allow While Feared By Fear Mechanic"                                  },
  {  418, "Cooldown Shared With AI Group"                                        },
  {  419, "Interrupts Current Cast"                                              },
  {  420, "Periodic Script Runs Late"                                            },
  {  421, "Recipe Hidden Until Known"                                            },
  {  422, "Can Proc From Lifesteal"                                              },
  {  423, "Nameplate Personal Buffs/Debuffs"                                     },
  {  424, "Cannot Lifesteal/Leech"                                               },
  {  425, "Global Aura"                                                          },
  {  426, "Nameplate Enemy Debuffs"                                              },
  {  427, "Always Allow PvP Flagged Target"                                      },
  {  428, "Do Not Consume Aura Stack On Proc"                                    },
  {  429, "Do Not PvP Flag Caster"                                               },
  {  430, "Always Require PvP Target Match"                                      },
  {  431, "Do Not Fail if No Target"                                             },
  {  432, "Displayed Outside Of Spellbook"                                       },
  {  433, "Check Phase on String ID Results"                                     },
  {  434, "Do Not Enforce Shapeshift Requirements"                               },
  {  435, "Aura Persists Through Tame Pet"                                       },
  {  436, "Periodic Refresh Extends Duration"                                    },
  {  437, "Use Skill Rank As Spell Level"                                        },
  {  438, "Aura Always Shown"                                                    },
  {  439, "Use Spell Level For Item Squish Compensation"                         },
  {  440, "Chain by Most Hit"                                                    },
  {  441, "Do Not Display Cast Time"                                             },
  {  442, "Always Allow Negative Healing Percent Modifiers"                      },
  {  443, "Do Not Allow \"Disable Movement Interrupt\""                          },
  {  444, "Allow Aura On Level Scale"                                            },
  {  445, "Remove Aura On Level Scale"                                           },
  {  446, "Recompute Aura On Level Scale"                                        },
  {  447, "Update Fall Speed After Aura Removal"                                 },
  {  448, "Prevent Jumping During Precast"                                       },
  {  449, "Reagent Consumes Charges"                                             },
  {  451, "Hide Passive From Tooltip"                                            },
  {  468, "Private Aura"                                                         },
} );

static constexpr auto _aura_interrupt_strings = util::make_static_map<unsigned, std::string_view>( {
  { IX_HOSTILE_ACTION_RECEIVED,     "Hostile Action Received"          },
  { IX_DAMAGE,                      "Damage"                           },
  { IX_ACTION,                      "Action"                           },
  { IX_MOVING,                      "Moving"                           },
  { IX_TURNING,                     "Turning"                          },
  { IX_ANIMATION,                   "Animation"                        },
  { IX_DISMOUNT,                    "Dismount"                         },
  { IX_UNDER_WATER,                 "Under Water"                      },
  { IX_ABOVE_WATER,                 "Above Water"                      },
  { IX_SHEATHING,                   "Sheathing"                        },
  { IX_INTERACTIHNG,                "Interacting"                      },
  { IX_LOOTING,                     "Looting"                          },
  { IX_ATTACKING,                   "Attacking"                        },
  { IX_USE_ITEM,                    "Use Item"                         },
  { IX_DAMAGE_CHANNEL_DURATION,     "Damage Channel Duration"          },
  { IX_SHAPESHIFTING,               "Shapeshifting"                    },
  { IX_ACTION_DELAYED,              "Action (Delayed)"                 },
  { IX_MOUNT,                       "Mount"                            },
  { IX_STANDING,                    "Standing"                         },
  { IX_LEAVE_WORLD,                 "Leave World"                      },
  { IX_STEALTH_OR_INVISIBLE,        "Stealth or Invisible"             },
  { IX_INVULNERABILITY,             "Invulnerability"                  },
  { IX_ENTER_WORLD,                 "Enter World"                      },
  { IX_PVP_ACTIVE,                  "PVP Active"                       },
  { IX_DIRECT_DAMAGE,               "Direct Damage"                    },
  { IX_LANDING,                     "Landing"                          },
  { IX_RELEASE,                     "Release"                          },
  { IX_DAMAGE_SCRIPT,               "Damage (Script)"                  },
  { IX_ENTER_COMBAT,                "Enter Combat"                     },
  { IX_LOGIN,                       "Login"                            },
  { IX_SUMMON,                      "Summon"                           },
  { IX_LEAVE_COMBAT,                "Leave Combat"                     },
  { IX_FALLING,                     "Falling"                          },
  { IX_SWIMMING,                    "Swimming"                         },
  { IX_NOT_MOVING,                  "Not Moving"                       },
  { IX_GROUND,                      "Ground"                           },
  { IX_TRANSFORM,                   "Transform"                        },
  { IX_JUMP,                        "Jump"                             },
  { IX_CHANGE_SPECIALIZATION,       "Change Specialization"            },
  { IX_EXIT_VEHICLE,                "Exit Vehicle"                     },
  { IX_RAID_ENCOUNTER_START,        "Raid Encounter Start or M+ Start" },
  { IX_RAID_ENCOUNTER_END,          "Raid Encounter End or M+ Start"   },
  { IX_DISCONNECT,                  "Disconnect"                       },
  { IX_ENTER_INSTANCE,              "Enter Instance"                   },
  { IX_DUEL_END,                    "Duel End"                         },
  { IX_LEAVE_ARENA_OR_BATTLEGROUND, "Leave Arena or Battleground"      },
  { IX_CHANGE_TALENT,               "Change Talent"                    },
  { IX_CHANGE_GLYPH,                "Change Glyph"                     },
  { IX_SEAMLESS_TRANSFER,           "Seamless Transfer"                },
  { IX_LEAVE_WAR_MODE,              "Leave War Mode"                   },
  { IX_TOUCH_GROUND,                "Touch Ground"                     },
  { IX_CHROMIE_TIME,                "Chromie Time"                     },
  { IX_SPLINE_OR_FREE_FLIGHT,       "Spline or Free Flight"            },
  { IX_PROC_OR_PERIODIC_ATTACK,     "Proc or Periodic Attack"          },
  { IX_CHALLENGE_MODE_START,        "Challenge Mode Start"             },
  { IX_ENCOUNTER_START,             "Encounter Start"                  },
  { IX_ENCOUNTER_END,               "Encounter End"                    },
  { IX_RELEASE_EMPOWER,             "Release Empower"                  },
} );

static constexpr auto _category_flag_strings = util::make_static_map<unsigned, std::string_view>( {
//{ 0, "UNUSED" },
  { 1, "Global Cooldown"                  },
  { 2, "Cooldown on Leaving Combat"       },
  { 3, "Cooldown in Days"                 },
  { 4, "Reset Charges on Encounter End"   },
  { 5, "Reset Coooldown on Encounter End" },
  { 6, "Unaffected by Modify Time Rate"   },
} );

static constexpr auto _property_type_strings = util::make_static_map<int, std::string_view>( {
  { P_GENERIC,            "Spell Direct Amount"             },
  { P_DURATION,           "Spell Duration"                  },
  { P_THREAT,             "Spell Generated Threat"          },
  { P_EFFECT_1,           "Spell Effect 1"                  },
  { P_STACK,              "Spell Initial Stacks"            },
  { P_RANGE,              "Spell Range"                     },
  { P_RADIUS,             "Spell Radius"                    },
  { P_CRIT,               "Spell Critical Chance"           },
  { P_EFFECTS,            "Spell Effects"                   },
  { P_PUSHBACK,           "Spell Pushback"                  },
  { P_CAST_TIME,          "Spell Cast Time"                 },
  { P_COOLDOWN,           "Spell Cooldown"                  },
  { P_EFFECT_2,           "Spell Effect 2"                  },
  { P_RESISTANCE,         "Spell Target Resistance"         },
  { P_RESOURCE_COST_1,    "Spell Resource Cost 1"           },
  { P_CRIT_BONUS,         "Spell Critical Bonus Multiplier" },
  { P_PENETRATION,        "Spell Penetration"               },
  { P_CHAIN_TARGETS,      "Spell Chain Targets"             },
  { P_PROC_CHANCE,        "Spell Proc Chance"               },
  { P_TICK_TIME,          "Spell Tick Time"                 },
  { P_CHAIN_MULTIPLIER,   "Spell Chain Multiplier"          },
  { P_GCD,                "Spell Global Cooldown"           },
  { P_TICK_DAMAGE,        "Spell Periodic Amount"           },
  { P_EFFECT_3,           "Spell Effect 3"                  },
  { P_SPELL_POWER,        "Spell Power"                     },
  { P_TRIGGER_DAMAGE,     "Spell Trigger Damage"            },
  { P_PROC_FREQUENCY,     "Spell Proc Frequency"            },
  { P_DAMAGE_TAKEN,       "Spell Amplitude"                 },
  { P_DISPEL_CHANCE,      "Spell Dispel Chance"             },
  { P_CROWD,              "Spell Crowd Damage"              },
  { P_COST_ON_MISS,       "Spell Cost On Miss"              },
  { P_DOSES,              "Spell Doses"                     },
  { P_EFFECT_4,           "Spell Effect 4"                  },
  { P_EFFECT_5,           "Spell Effect 5"                  },
  { P_RESOURCE_COST_2,    "Spell Resource Cost 2"           },
  { P_CHAIN_TARGET_RANGE, "Spell Chain Target Range"        },
  { P_ARENA_MAX_SUMMONS,  "Spell Area Max Summons"          },
  { P_MAX_STACKS,         "Spell Max Stacks"                },
  { P_PROC_COOLDOWN,      "Spell Proc Cooldown"             },
  { P_RESOURCE_COST_3,    "Spell Resource Cost 3"           },
  { P_MAX_TARGETS,        "Spell Max Targets"               },
} );

static constexpr auto _effect_type_strings = util::make_static_map<unsigned, std::string_view>( {
  { E_NONE,                                            "None"                                              },
  { E_INSTAKILL,                                       "Instant Kill"                                      },
  { E_SCHOOL_DAMAGE,                                   "School Damage"                                     },
  { E_DUMMY,                                           "Dummy"                                             },
  { E_PORTAL_TELEPORT,                                 "Portal Teleport"                                   },
  { E_APPLY_AURA,                                      "Apply Aura"                                        },
  { E_ENVIRONMENTAL_DAMAGE,                            "Environmental Damage"                              },
  { E_POWER_DRAIN,                                     "Power Drain"                                       },
  { E_HEALTH_LEECH,                                    "Health Leech"                                      },
  { E_HEAL,                                            "Direct Heal"                                       },
  { E_BIND,                                            "Bind"                                              },
  { E_PORTAL,                                          "Portal"                                            },
  { E_TELEPORT_TO_RETURN_POINT,                        "Teleport to Return Point"                          },
  { E_INCREASE_CURRENCY_CAP,                           "Increase Currency Cap"                             },
  { E_TELEPORT_WITH_SPELL_VISUAL_KIT_LOADING_SCREEN,   "Teleport w/ Loading Screen"                        },
  { E_QUEST_COMPLETE,                                  "Quest Complete"                                    },
  { E_WEAPON_DAMAGE_NOSCHOOL,                          "Weapon Damage"                                     },
  { E_RESURRECT,                                       "Resurrect"                                         },
  { E_ADD_EXTRA_ATTACKS,                               "Extra Attacks"                                     },
  { E_DODGE,                                           "Dodge"                                             },
  { E_EVADE,                                           "Evade"                                             },
  { E_PARRY,                                           "Parry"                                             },
  { E_BLOCK,                                           "Block"                                             },
  { E_CREATE_ITEM,                                     "Create Item"                                       },
  { E_WEAPON,                                          "Weapon Type"                                       },
  { E_DEFENSE,                                         "Defense"                                           },
  { E_PERSISTENT_AREA_AURA,                            "Apply Aura in Area"                                },
  { E_SUMMON,                                          "Summon Guardian"                                   },
  { E_LEAP,                                            "Leap"                                              },
  { E_ENERGIZE,                                        "Energize Power"                                    },
  { E_WEAPON_PERCENT_DAMAGE,                           "Weapon Damage%"                                    },
  { E_TRIGGER_MISSILE,                                 "Trigger Missiles"                                  },
  { E_OPEN_LOCK,                                       "Open Lock"                                         },
  { E_SUMMON_CHANGE_ITEM,                              "Summon Item"                                       },
  { E_APPLY_AREA_AURA_PARTY,                           "Apply Party Aura"                                  },
  { E_LEARN_SPELL,                                     "Learn Spell"                                       },
  { E_SPELL_DEFENSE,                                   "Spell Defense"                                     },
  { E_DISPEL,                                          "Dispel"                                            },
  { E_LANGUAGE,                                        "Language"                                          },
  { E_DUAL_WIELD,                                      "Dual Wield"                                        },
  { E_JUMP,                                            "Jump"                                              },
  { E_JUMP_DEST,                                       "Jump Dest"                                         },
  { E_TELEPORT_UNITS_FACE_CASTER,                      "Teleport Unit Facing Caster"                       },
  { E_SKILL_STEP,                                      "Skill Step"                                        },
  { E_PLAY_MOVIE,                                      "Play Movie"                                        },
  { E_SPAWN,                                           "Spawn"                                             },
  { E_TRADE_SKILL,                                     "Trade Skill"                                       },
  { E_STEALTH,                                         "Stealth"                                           },
  { E_DETECT,                                          "Detect"                                            },
  { E_TRANS_DOOR,                                      "Transition through Door"                           },
  { E_FORCE_CRITICAL_HIT,                              "Force Crit"                                        },
  { E_SET_MAX_BATTLEPET_COUNT,                         "Guaranteed Hit"                                    },
  { E_ENCHANT_ITEM,                                    "Enchant Item"                                      },
  { E_ENCHANT_ITEM_TEMPORARY,                          "Enchant Item Temporary"                            },
  { E_TAMECREATURE,                                    "Tame Creature"                                     },
  { E_SUMMON_PET,                                      "Summon Pet"                                        },
  { E_LEARN_PET_SPELL,                                 "Learn Pet Spell"                                   },
  { E_WEAPON_DAMAGE,                                   "Weapon Damage"                                     },
  { E_CREATE_RANDOM_ITEM,                              "Create Random Item"                                },
  { E_PROFICIENCY,                                     "Proficiency"                                       },
  { E_SEND_EVENT,                                      "Send Event"                                        },
  { E_POWER_BURN,                                      "Power Burn"                                        },
  { E_THREAT,                                          "Threat"                                            },
  { E_TRIGGER_SPELL,                                   "Trigger Spell"                                     },
  { E_APPLY_AREA_AURA_RAID,                            "Apply Aura Raid"                                   },
  { E_RESTORE_ITEM_CHARGES,                            "Recharge Item"                                     },
  { E_HEAL_MAX_HEALTH,                                 "Heal Max Health%"                                  },
  { E_INTERRUPT_CAST,                                  "Interrupt Cast"                                    },
  { E_DISTRACT,                                        "Distract"                                          },
  { E_COMPLETE_WORLD_QUEST,                            "Complete World Quest"                              },
  { E_PICKPOCKET,                                      "Pick Pocket"                                       },
  { E_ADD_FARSIGHT,                                    "Add Farsight"                                      },
  { E_UNTRAIN_TALENTS,                                 "Unlearn Talent"                                    },
  { E_APPLY_GLYPH,                                     "Apply Glyph"                                       },
  { E_HEAL_MECHANICAL,                                 "Heal Mechanical"                                   },
  { E_SUMMON_OBJECT_WILD,                              "Summon Object - Wild"                              },
  { E_SCRIPT_EFFECT,                                   "Server Side Script"                                },
  { E_ATTACK,                                          "Attack"                                            },
  { E_SANCTUARY,                                       "Sanctuary"                                         },
  { E_MODIFY_FOLLOWER_ITEM_LEVEL,                      "Modify Follower Item Level"                        },
  { E_PUSH_ABILITY_TO_ACTION_BAR,                      "Push Ability to Action Bar"                        },
  { E_BIND_SIGHT,                                      "Bind Sight"                                        },
  { E_DUEL,                                            "Duel"                                              },
  { E_STUCK,                                           "Stuck"                                             },
  { E_SUMMON_PLAYER,                                   "Summon Player"                                     },
  { E_ACTIVATE_OBJECT,                                 "Activate Object"                                   },
  { E_WMO_DAMAGE,                                      "Damage Gameobject"                                 },
  { E_WMO_REPAIR,                                      "Repair Gameobject"                                 },
  { E_WMO_CHANGE,                                      "Set Gameobject Destruction State"                  },
  { E_SUMMON_CREDIT_CREATURE,                          "Summon Credit Creature"                            },
  { E_THREAT_ALL,                                      "Threat All"                                        },
  { E_ENCHANT_HELD_ITEM,                               "Enchant Held Item"                                 },
  { E_BREAK_PLAYER_TARGETING,                          "Force Untarget"                                    },
  { E_SELF_RESURRECT,                                  "Self Resurrect"                                    },
  { E_SKINNING,                                        "Skinning"                                          },
  { E_CHARGE,                                          "Charge"                                            },
  { E_SUMMON_ALL_TOTEMS,                               "Summon All Totems"                                 },
  { E_KNOCK_BACK,                                      "Knock Back"                                        },
  { E_DISENCHANT,                                      "Disenchant"                                        },
  { E_INEBRIATE,                                       "Inebriate"                                         },
  { E_FEED_PET,                                        "Feed Pet"                                          },
  { E_DISMISS_PET,                                     "Dismiss Pet"                                       },
  { E_REPUTATION,                                      "Reputation"                                        },
  { E_SUMMON_OBJECT_SLOT1,                             "Summon Object"                                     },
  { E_SURVEY,                                          "Survey"                                            },
  { E_CHANGE_RAID_MARKER,                              "Change Raid Marker"                                },
  { E_SHOW_CORPSE_LOOT,                                "Show Corpse Loot"                                  },
  { E_DISPEL_MECHANIC,                                 "Dispel Mechanic"                                   },
  { E_SUMMON_DEAD_PET,                                 "Summon Dead Pet"                                   },
  { E_DESTROY_ALL_TOTEMS,                              "Destroy All Totems"                                },
  { E_DURABILITY_DAMAGE,                               "Durability Damage"                                 },
  { E_CANCEL_CONVERSATION,                             "Cancel Conversation"                               },
  { E_ATTACK_ME,                                       "Taunt"                                             },
  { E_DURABILITY_DAMAGE_PCT,                           "Durability Damage%"                                },
  { E_SKIN_PLAYER_CORPSE,                              "Skin Player Corpse"                                },
  { E_SPIRIT_HEAL,                                     "Spirit Heal"                                       },
  { E_SKILL,                                           "Skill"                                             },
  { E_APPLY_AREA_AURA_PET,                             "Apply Aura Pet in Area"                            },
  { E_TELEPORT_GRAVEYARD,                              "Teleport to Graveyard"                             },
  { E_NORMALIZED_WEAPON_DMG,                           "Normalized Weapon Damage"                          },
  { E_SEND_TAXI,                                       "Send Taxi"                                         },
  { E_PLAYER_PULL,                                     "Pull Player"                                       },
  { E_MODIFY_THREAT_PERCENT,                           "Modify Threat"                                     },
  { E_STEAL_BENEFICIAL_BUFF,                           "Steal Beneficial Aura"                             },
  { E_PROSPECTING,                                     "Prospect"                                          },
  { E_APPLY_AREA_AURA_FRIEND,                          "Apply Aura Friendly in Area"                       },
  { E_APPLY_AREA_AURA_ENEMY,                           "Apply Aura Enemy in Area"                          },
  { E_REDIRECT_THREAT,                                 "Redirect Threat"                                   },
  { E_PLAY_SOUND,                                      "Play Sound"                                        },
  { E_PLAY_MUSIC,                                      "Play Music"                                        },
  { E_UNLEARN_SPECIALIZATION,                          "Unlearn Profession Spec"                           },
  { E_KILL_CREDIT2,                                    "Kill Credit"                                       },
  { E_CALL_PET,                                        "Call Pet"                                          },
  { E_HEAL_PCT,                                        "Direct Heal%"                                      },
  { E_ENERGIZE_PCT,                                    "Energize Power%"                                   },
  { E_LEAP_BACK,                                       "Directional Knock"                                 },
  { E_CLEAR_QUEST,                                     "Clear Quest"                                       },
  { E_FORCE_CAST,                                      "Force Cast"                                        },
  { E_FORCE_CAST_WITH_VALUE,                           "Force Cast w/ Value"                               },
  { E_TRIGGER_SPELL_WITH_VALUE,                        "Trigger Spell w/ Value"                            },
  { E_APPLY_AREA_AURA_OWNER,                           "Apply Aura Owner in Area"                          },
  { E_KNOCK_BACK_DESTINATION,                          "Directional Knockback"                             },
  { E_PULL_TOWARDS_DESTIANTION,                        "Pull to Destination"                               },
  { E_RESTORE_GARRISON_TROOP_VITALITY,                 "Restore Garrison Troop Vitality"                   },
  { E_QUEST_FAIL,                                      "Fail Quest"                                        },
  { E_TRIGGER_MISSILE_SPELL_WITH_VALUE,                "Trigger Missile Spell w/ Value"                    },
  { E_CHARGE_TO_DESTINATION,                           "Charge to Destination"                             },
  { E_QUEST_START,                                     "Start Quest"                                       },
  { E_TRIGGER_SPELL_2,                                 "Trigger Spell"                                     },
  { E_SUMMON_RAF_FRIEND,                               "Summon Refer a Friend"                             },
  { E_CREATE_TAMED_PET,                                "Create Tamed Pet"                                  },
  { E_TEACH_TAXI_NODE,                                 "Discover Taxi Location"                            },
  { E_TITAN_GRIP,                                      "Titan Grip"                                        },
  { E_ENCHANT_ITEM_PRISMATIC,                          "Add Prismatic Socket"                              },
  { E_CREATE_ITEM_2,                                   "Create Item"                                       },
  { E_MILLING,                                         "Mill"                                              },
  { E_ALLOW_RENAME_PET,                                "Allow Rename Pet"                                  },
  { E_FORCE_CAST_2,                                    "Force Cast"                                        },
  { E_TALENT_SPEC_COUNT,                               "Learn/Unlearn Secondary Spec"                      },
  { E_TALENT_SPEC_SELECT,                              "Activate Specialization"                           },
  { E_OBLITERATE_ITEM,                                 "Obliterate Item"                                   },
  { E_CANCEL_AURA,                                     "Cancel Aura"                                       },
  { E_DAMAGE_TAKEN_PCT_MAX_HEALTH,                     "Take Max Health% Damage"                           },
  { E_GIVE_CURRENCY,                                   "Give Currency"                                     },
  { E_UPDATE_PLAYER_PHASE,                             "Update Player Phase"                               },
  { E_ALLOW_CONTROL_PET,                               "Allow Control Pet"                                 },
  { E_DESTORY_ITEM,                                    "Destroy Item"                                      },
  { E_UPDATE_ZONE_AURAS_AND_PHASES,                    "Update Zone Auras and Phases"                      },
  { E_SUMMON_PERSONAL_GAMEOBJECT,                      "Summon Personal Gameobject"                        },
  { E_RESURRECT_WITH_AURA,                             "Resurrect with Health%"                            },
  { E_UNLOCK_GUILD_VAULT_TAB,                          "Unlock Guild Vault Tab"                            },
  { E_APPLY_AURA_PET,                                  "Apply Aura Pet"                                    },
  { E_SANCTUARY_2,                                     "Sanctuary"                                         },
  { E_DESPAWN_PERSISTENT_AREA_AURA,                    "Despawn Persistent Area Aura"                      },
  { E_CREATE_AREA_TRIGGER,                             "Create Area Trigger"                               },
  { E_UPDATE_AREA_TRIGGER,                             "Update Area Trigger"                               },
  { E_REMOVE_TALENT,                                   "Remove Talent"                                     },
  { E_DESPAWN_AREA_TRIGGER,                            "Despawn Area Trigger"                              },
  { E_REPUTATION_2,                                    "Reputation"                                        },
  { E_RANDOMIZE_ARCHAEOLOGY_DIGSITES,                  "Randomize Archaeology Digsite"                     },
  { E_SUMMON_STABLED_PET_AS_GUARDIAN,                  "Summon Multiple Hunter Pets"                       },
  { E_LOOT,                                            "Loot"                                              },
  { E_CHANGE_PARTY_MEMBERS,                            "Change Party Member"                               },
  { E_TELEPORT_TO_DIGSITE,                             "Teleport to Digsite"                               },
  { E_UNCAGE_BATTLEPET,                                "Uncage Battlepet"                                  },
  { E_START_PET_BATTLE,                                "Start Pet Battlle"                                 },
  { E_PLAY_SCENE_SCRIPT_PACKAGE,                       "Play Scene Script Package"                         },
  { E_CREATE_SCENE_OBJECT,                             "Create Scene Object"                               },
  { E_CREATE_PERSONAL_SCENE_OBJECT,                    "Create Personal Scene Object"                      },
  { E_PLAY_SCENE,                                      "Play Scene"                                        },
  { E_DESPAWN_SUMMON,                                  "Despawn Summon"                                    },
  { E_HEAL_BATTLEPET_PCT,                              "Battlepet Heal%"                                   },
  { E_ENABLE_BATTLE_PETS,                              "Enable Pet Battles"                                },
  { E_APPLY_AURA_PLAYER_AND_PET,                       "Apply Player/Pet Aura"                             },
  { E_REMOVE_AURA_2,                                   "Remove Aura"                                       },
  { E_CHANGE_BATTLEPET_QUALITY,                        "Change Battlepet Quality"                          },
  { E_LAUNCH_QUEST_CHOICE,                             "Launch Quest Choice"                               },
  { E_ALTER_ITEM,                                      "Alter Item"                                        },
  { E_LAUNCH_QUEST_TASK,                               "Launch Quest Task"                                 },
  { E_SET_REPUTATION,                                  "Set Reputation"                                    },
  { E_LEARN_GARRISON_BUILDING,                         "Learn Garrison Building"                           },
  { E_LEARN_GARRISON_SPECIALIZATION,                   "Learn Garrison Specialization"                     },
  { E_REMOVE_AURA_BY_SPELL_LABEL,                      "Remove Aura (Label)"                               },
  { E_JUMP_TO_DESTINATION_2,                           "Jump to Destiantion"                               },
  { E_CREATE_GARRISON,                                 "Create Garrison"                                   },
  { E_UPGRADE_CHARACTER_SPELLS,                        "Upgrade Character Spells"                          },
  { E_CREATE_SHIPMENT,                                 "Create Shipment"                                   },
  { E_UPGRADE_GARRISON,                                "Upgrade Garrison"                                  },
  { E_CREATE_CONVERSATION,                             "Create Conversation"                               },
  { E_ADD_GARRISON_FOLLOWER,                           "Add Garrison Follower"                             },
  { E_ADD_GARRISON_MISSION,                            "Add Garrison Mission"                              },
  { E_CREATE_HEIRLOOM_ITEM,                            "Create Heirloom Item"                              },
  { E_CHANGE_ITEM_BONUSES,                             "Change Item Bonuses"                               },
  { E_ACTIVATE_GARRISON_BUILDING,                      "Activate Garrison Building"                        },
  { E_GRANT_BATTLEPET_LEVEL,                           "Grant Battlepet Level"                             },
  { E_TRIGGER_ACTION_SET,                              "Trigger Action Set"                                },
  { E_TELEPORT_TO_LFG_DUNGEON,                         "Teleport to LFG Dungeon"                           },
  { E_SET_FOLLOWER_QUALITY,                            "Set Follower Quality"                              },
  { E_INCREASE_FOLLOWER_EXPERIENCE,                    "Increase Follower Experience"                      },
  { E_REMOVE_PHASE,                                    "Remove Phase"                                      },
  { E_RANDOMIZE_FOLLOWER_ABILITIES,                    "Randomize Follower Abilities"                      },
  { E_GIVE_EXPERIENCE,                                 "Give Experience"                                   },
  { E_GIVE_RESTED_EXPERIENCE_BONUS,                    "Give Rested Experience Bonus"                      },
  { E_INCREASE_SKILL,                                  "Increase Skill"                                    },
  { E_END_GARRISON_BUILDING_CONSTRUCTION,              "Finish Garrison Building Construction"             },
  { E_GIVE_ARTIFACT_POWER,                             "Give Artifact Power"                               },
  { E_GIVE_ARTIFACT_POWER_NO_BONUS,                    "Give Artifact Power (No Bonus)"                    },
  { E_APPLY_ENCHANT_ILLUSION,                          "Apply Enchant Illusion"                            },
  { E_LEARN_FOLLOWER_ABILITY,                          "Learn Follower Ability"                            },
  { E_UPGRADE_HEIRLOOM,                                "Upgrade Heirloom"                                  },
  { E_FINISH_GARRISON_MISSION,                         "Finish Garrison Mission"                           },
  { E_ADD_GARRISON_MISSION_SET,                        "Add Garrison Mission Set"                          },
  { E_FINISH_SHIPMENT,                                 "Finish Shipment"                                   },
  { E_FORCE_EQUIP_ITEM,                                "Force Equip Item"                                  },
  { E_TAKE_SCREENSHOT,                                 "Take Screenshot"                                   },
  { E_SET_GARRISON_CACHE_SIZE,                         "Set Garrison Cache Size"                           },
  { E_TELEPORT_UNITS,                                  "Teleport Units"                                    },
  { E_GIVE_HONOR,                                      "Give Honor"                                        },
  { E_JUMP_CHARGE,                                     "Jump Charge"                                       },
  { E_LEARN_TRANSMOG_SET,                              "Learn Transmog Set"                                },
  { E_MODIFY_KEYSTONE,                                 "Modify Keystone"                                   },
  { E_RESPEC_AZERITE_EMPOWERED_ITEM,                   "Respect Azerite Item"                              },
  { E_SUMMON_STABLED_PET,                              "Summon Stabled Pet"                                },
  { E_SCRAP_ITEM,                                      "Scrap Item"                                        },
  { E_REPAIR_ITEM,                                     "Repair Item"                                       },
  { E_REMOVE_GEM,                                      "Remove Gem"                                        },
  { E_LEARN_AZERITE_ESSENCE_POWER,                     "Learn Azerite Essence Power"                       },
  { E_SET_ITEM_BONUS_LIST_GROUP_ENTRY,                 "Set Item Bonus List Group Entry"                   },
  { E_CREATE_PRIVATE_CONVERSATION,                     "Create Private Conversation"                       },
  { E_APPLY_MOUNT_EQUIPMENT,                           "Apply Mount Equipment"                             },
  { E_INCREASE_ITEM_BONUS_LIST_GROUP_STEP,             "Increase Item Bonus List Group Step"               },
  { E_APPLY_AREA_AURA_PARTY_NONRANDOM,                 "Apply Aura Party (Non-Random)"                     },
  { E_SET_COVENANT,                                    "Set Covenant"                                      },
  { E_CRAFT_RUNEFORGE_LEGENDARY,                       "Craft Runeforge Legendary"                         },
  { E_LEARN_TRANSMOG_ILLUSION,                         "Learn Transmog Illusion"                           },
  { E_SET_CHROMIE_TIME,                                "Set Chromie Time"                                  },
  { E_LEARN_GARRISON_TALENT,                           "Learn Garrison Talent"                             },
  { E_LEARN_SOULBIND_CONDUIT,                          "Learn Soulbind Conduit"                            },
  { E_CONVERT_ITEMS_TO_CURRENCY,                       "Convert Items to Currency"                         },
  { E_COMPLETE_CAMPAIGN,                               "Complete Campaign"                                 },
  { E_SEND_CHAT_MESSAGE,                               "Send Chat Message"                                 },
  { E_MODIFY_KEYSTONE_2,                               "Modify Keystone"                                   },
  { E_GRANT_BATTLEPET_EXPERIENCE,                      "Grant Battlepet Experience"                        },
  { E_SET_GARRISON_FOLLOWER_LEVEL,                     "Set Garrison Follower Level"                       },
  { E_CRAFT_ITEM,                                      "Craft Item"                                        },
  { E_MODIFY_AURA_STACKS,                              "Modify Aura Stacks"                                },
  { E_REDUCE_REMAINING_COOLDOWN,                       "Reduce Remaining Cooldown"                         },
  { E_MODIFY_COOLDOWN,                                 "Modify Cooldown"                                   },
  { E_MODIFY_COOLDOWN_IN_CATEGORY,                     "Modify Cooldown (Category)"                        },
  { E_RECHARGE_CATEGORY_COOLDOWN_IMMEDIATE,            "Immediate Cooldown Recharge (Category)"            },
  { E_CRAFT_LOOT,                                      "Craft Loot"                                        },
  { E_SALVAGE_ITEM,                                    "Salvage Item"                                      },
  { E_CRAFT_SALVAGE_ITEM,                              "Craft Salvage Item"                                },
  { E_RECRAFT_ITEM,                                    "Recraft Item"                                      },
  { E_CANCEL_ALL_PRIVATE_CONVERSATIONS,                "Cancel All Private Conversations"                  },
  { E_CRAFT_ENCHANTMENT,                               "Craft Enchantment"                                 },
  { E_GATHERING,                                       "Gather"                                            },
  { E_CREATE_TRAIT_TREE_CONFIG,                        "Create Trait Tree Config"                          },
  { E_CHANGE_ACTIVE_COMBAT_TRAIT_CONFIG,               "Change Active Combat Triat Config"                 },
  { E_UPDATE_INTERACTIONS,                             "Update Interactions"                               },
  { E_CANCEL_PRELOAD_WORLD,                            "Cancel World Preload"                              },
  { E_PRELOAD_WORLD,                                   "Preload World"                                     },
  { E_ENSURE_WORLD_LOADED,                             "Check World Loaded"                                },
  { E_CHANGE_ITEM_BONUSES_2,                           "Change Item Bonuses"                               },
  { E_ADD_SOCKET_BONUS,                                "Add Socket Bonus"                                  },
  { E_LEARN_TRANSMOG_APPEARANCE_FROM_ITEM_MOD_APPEARANCE_GROUP, "Learn Transmog Appearance from Item mod Appearance Group" },
  { E_KILL_CREDIT_LABEL_1,                             "Kill Credit (Label)"                               },
  { E_KILL_CREDIT_LABEL_2,                             "Kill Credit (Label)"                               },
  { E_SET_PLAYER_DATA_ELEMENT_ACCOUNT,                 "Set Player Data Element (Account)"                 },
  { E_SET_PLAYER_DATA_ELEMENT_CHARACTER,               "Set Player Data Element (Character)"               },
  { E_SET_PLAYER_DATA_FLAG_ACCOUNT,                    "Set Player Data Flag (Account)"                    },
  { E_SET_PLAYER_DATA_FLAG_CHARACTER,                  "Set Player Data Flag (Character)"                  },
  { E_UI_ACTION,                                       "UI Action"                                         },
  { E_LEARN_WARBAND_SCENE,                             "Learn Warband Scene"                               },
  { E_ASSIST_ACTION,                                   "Assist Action"                                     },
} );

static constexpr auto _effect_subtype_strings = util::make_static_map<unsigned, std::string_view>( {
  { A_NONE,                                  "None"                                              },
  { A_MOD_POSSESS,                           "Possess"                                           },
  { A_PERIODIC_DAMAGE,                       "Periodic Damage"                                   },
  { A_DUMMY,                                 "Dummy"                                             },
  { A_MOD_CONFUSE,                           "Confuse"                                           },
  { A_MOD_CHARM,                             "Charm"                                             },
  { A_MOD_FEAR,                              "Fear"                                              },
  { A_PERIODIC_HEAL,                         "Periodic Heal"                                     },
  { A_MOD_ATTACKSPEED_NORMALIZED,            "Auto Attack Speed (Normalized wDPS)"               },
  { A_MOD_THREAT,                            "Threat"                                            },
  { A_MOD_TAUNT,                             "Taunt"                                             },
  { A_MOD_STUN,                              "Stun"                                              },
  { A_MOD_DAMAGE_DONE,                       "Damage Done"                                       },
  { A_MOD_DAMAGE_TAKEN,                      "Damage Taken"                                      },
  { A_DAMAGE_SHIELD,                         "Damage Shield"                                     },
  { A_MOD_STEALTH,                           "Stealth"                                           },
  { A_MOD_STEALTH_DETECT,                    "Stealth Detection"                                 },
  { A_MOD_INVISIBILITY,                      "Invisibility"                                      },
  { A_MOD_INVISIBILITY_DETECTION,            "Invisibility Detection"                            },
  { A_PERIODIC_HEAL_PCT,                     "Periodic Heal%"                                    },
  { A_OBS_MOD_MANA,                          "Periodic Power% Regen"                             },
  { A_MOD_RESISTANCE,                        "Resistance"                                        },
  { A_PERIODIC_TRIGGER_SPELL,                "Periodic Trigger Spell"                            },
  { A_PERIODIC_ENERGIZE,                     "Periodic Energize Power"                           },
  { A_MOD_PACIFY,                            "Pacify"                                            },
  { A_MOD_ROOT,                              "Root"                                              },
  { A_MOD_SILENCE,                           "Silence"                                           },
  { A_REFLECT_SPELLS,                        "Spell Reflection"                                  },
  { A_MOD_STAT,                              "Attribute"                                         },
  { A_MOD_SKILL,                             "Skill"                                             },
  { A_MOD_INCREASE_SPEED,                    "Increase Speed%"                                   },
  { A_MOD_INCREASE_MOUNTED_SPEED,            "Increase Mounted Speed%"                           },
  { A_MOD_DECREASE_SPEED,                    "Decrease Movement Speed%"                          },
  { A_MOD_INCREASE_HEALTH,                   "Increase Health"                                   },
  { A_MOD_INCREASE_RESOURCE,                 "Increase Resource"                                 },
  { A_MOD_SHAPESHIFT,                        "Shapeshift"                                        },
  { A_EFFECT_IMMUNITY,                       "Immunity Against External Movement"                },
  { A_SCHOOL_IMMUNITY,                       "School Immunity"                                   },
  { A_DAMAGE_IMMUNITY,                       "Damage Immunity"                                   },
  { A_DISPEL_IMMUNITY,                       "Disable Stealth"                                   },
  { A_PROC_TRIGGER_SPELL,                    "Proc Trigger Spell"                                },
  { A_PROC_TRIGGER_DAMAGE,                   "Proc Trigger Damage"                               },
  { A_TRACK_CREATURES,                       "Track Creatures"                                   },
  { A_MOD_PARRY_PERCENT,                     "Modify Parry%"                                     },
  { A_MOD_DODGE_PERCENT,                     "Modify Dodge%"                                     },
  { A_MOD_CRITICAL_HEALING_AMOUNT,           "Modify Critical Heal Bonus"                        },
  { A_MOD_BLOCK_PERCENT,                     "Modify Block%"                                     },
  { A_MOD_CRIT_PERCENT,                      "Modify Crit%"                                      },
  { A_PERIODIC_LEECH,                        "Periodic Health Leech"                             },
  { A_MOD_HIT_CHANCE,                        "Modify Hit%"                                       },
  { A_MOD_SPELL_HIT_CHANCE,                  "Modify Spell Hit%"                                 },
  { A_TRANSFORM,                             "Change Model"                                      },
  { A_MOD_SPELL_CRIT_CHANCE,                 "Modify Spell Crit%"                                },
  { A_MOD_DAMAGE_DONE_CREATURE,              "Modify Damage done to Creature Type"               },
  { A_MOD_PACIFY_SILENCE,                    "Pacify Silence"                                    },
  { A_MOD_SCALE,                             "Scale% (Stacking)"                                 },
  { A_MOD_MAX_RESOURCE_COST,                 "Modify Max Cost"                                   },
  { A_PERIODIC_MANA_LEECH,                   "Periodic Mana Leech"                               },
  { A_MOD_CASTING_SPEED_NOT_STACK,           "Modify Spell Speed%"                               },
  { A_FEIGN_DEATH,                           "Feign Death"                                       },
  { A_MOD_DISARM,                            "Disarm"                                            },
  { A_MOD_STALKED,                           "Stalked"                                           },
  { A_SCHOOL_ABSORB,                         "Absorb Damage"                                     },
  { A_MOD_POWER_COST_SCHOOL_PCT,             "Modify Power Cost%"                                },
  { A_MOD_POWER_COST_SCHOOL,                 "Modify Power Cost"                                 },
  { A_REFLECT_SPELLS_SCHOOL,                 "Reflect Spells"                                    },
  { A_MECHANIC_IMMUNITY,                     "Mechanic Immunity"                                 },
  { A_MOD_DAMAGE_PERCENT_DONE,               "Modify Damage Done%"                               },
  { A_MOD_PERCENT_STAT,                      "Modify Attribute%"                                 },
  { A_SPLIT_DAMAGE_PCT,                      "Transfer Damage%"                                  },
  { A_RESTORE_HEALTH,                        "Restore Health"                                    },
  { A_RESTORE_POWER,                         "Restore Power"                                     },
  { A_MOD_DAMAGE_PERCENT_TAKEN,              "Modify Damage Taken%"                              },
  { A_MOD_HEALTH_REGEN_PERCENT,              "Modify Health Regeneration%"                       },
  { A_PERIODIC_DAMAGE_PERCENT,               "Periodic Max Health% Damage"                       },
  { A_INTERRUPT_REGEN,                       "Interrupt Health Regen"                            },
  { A_MOD_ATTACK_POWER,                      "Modify Attack Power"                               },
  { A_MOD_RESISTANCE_PCT,                    "Modify Armor%"                                     },
  { A_MOD_MELEE_ATTACK_POWER_VERSUS,         "Modify Melee Attack Power vs Race"                 },
  { A_MOD_TOTAL_THREAT,                      "Temporary Thread Reduction"                        },
  { A_WATER_WALK,                            "Modify Attack Power"                               },
  { A_HOVER,                                 "Levitate"                                          },
  { A_ADD_FLAT_MODIFIER,                     "Add Flat Modifier"                                 },
  { A_ADD_PCT_MODIFIER,                      "Add Percent Modifier"                              },
  { A_MOD_POWER_REGEN_PERCENT,               "Modify Power Regen"                                },
  { A_MOD_HEALING,                           "Modify Healing Received"                           },
  { A_MOD_REGEN_DURING_COMBAT,               "Combat Health Regen%"                              },
  { A_MOD_MECHANIC_RESISTANCE,               "Mechanic Resistance"                               },
  { A_MOD_HEALING_RECEIVED_PCT,              "Modify Healing Received%"                          },
  { A_CHECK_PVP_STATE,                       "Check PVP State"                                   },
  { A_MOD_TARGET_RESISTANCE,                 "Modify Target Resistance"                          },
  { A_MOD_RANGED_ATTACK_POWER,               "Modify Ranged Attack Power"                        },
  { A_MOD_MELEE_DAMAGE_TAKEN_PCT,            "Modify Melee Damage Taken%"                        },
  { A_MOD_FIXATE,                            "Fixate"                                            },
  { A_MOD_SPEED_ALWAYS,                      "Increase Movement Speed% (Stacking)"               },
  { A_MOD_MOUNTED_SPEED_ALWAYS,              "Increase Mount Speed% (Stacking)"                  },
  { A_MOD_RANGED_ATTACK_POWER_VERSUS,        "Modify Ranged Attack Power vs Race"                },
  { A_MOD_INCREASE_ENERGY_PERCENT,           "Modify Max Resource%"                              },
  { A_MOD_INCREASE_HEALTH_PERCENT,           "Modify Max Health%"                                },
  { A_MOD_HEALING_DONE,                      "Modify Healing Power"                              },
  { A_MOD_HEALING_DONE_PERCENT,              "Modify Healing% Done"                              },
  { A_MOD_TOTAL_STAT_PERCENTAGE,             "Modify Total Stat%"                                },
  { A_MOD_MELEE_HASTE,                       "Modify Melee Haste%"                               },
  { A_FORCE_REACTION,                        "Force Reaction"                                    },
  { A_MOD_RANGED_HASTE,                      "Modify Ranged Haste%"                              },
  { A_MOD_BASE_RESISTANCE_PCT,               "Modify Base Resistance"                            },
  { A_MOD_RECHARGE_RATE_LABEL,               "Modify Cooldown Recharge Rate% (Label)"            },
  { A_SAFE_FALL,                             "Reduce Fall Damage"                                },
  { A_CREATURE_IMMUNITIES,                   "Creature Immunities"                               },
  { A_MOD_CHARGE_RECHARGE_RATE,              "Modify Charge Cooldown Recharge Rate% (Category)"  },
  { A_REDUCE_PUSHBACK,                       "Modify Casting Pushback"                           },
  { A_MOD_SHIELD_BLOCKVALUE_PCT,             "Modify Block Effectiveness"                        },
  { A_MOD_DETECTED_RANGE,                    "Modify Aggro Distance"                             },
  { A_MOD_AUTO_ATTACK_RANGE,                 "Modify Auto Attack Range"                          },
  { A_MOD_STEALTH_LEVEL,                     "Modify Stealth Detection Level"                    },
  { A_MOD_HEALTH_REGEN_IN_COMBAT,            "Modify Health Regeneration Rate in Combat"         },
  { A_PET_DAMAGE_MULTI,                      "Modify Absorb% Done"                               },
  { A_MOD_CRIT_DAMAGE_BONUS,                 "Modify Crit Damage Done%"                          },
  { A_FORCE_BREATH_BAR,                      "Force Breath Bar"                                  },
  { A_MOD_ATTACK_POWER_PCT,                  "Modify Melee Attack Power%"                        },
  { A_MOD_RANGED_ATTACK_POWER_PCT,           "Modify Ranged Attack Power%"                       },
  { A_MOD_DAMAGE_DONE_VERSUS,                "Modify Damage Done% vs Race"                       },
  { A_MOD_SPEED_NOT_STACK,                   "Increase Movement Speed%"                          },
  { A_MOD_MOUNTED_SPEED_NOT_STACK,           "Increase Mounted Speed%"                           },
  { A_MOD_CHARGE_TYPE_RECHARGE_RATE,         "Modify Charge Cooldown Type Recharge Rate%"        },
  { A_AOE_CHARM,                             "Charmed"                                           },
  { A_MOD_MAX_MANA_PCT,                      "Modify Max Mana%"                                  },
  { A_MOD_ATTACKER_SPELL_CRIT_CHANCE,        "Modify Attacker Spell Crit Chance"                 },
  { A_MOD_FLAT_SPELL_DAMAGE_VERSUS,          "Modify Spell Damage vs Race"                       },
  { A_MOD_SPELL_CURRENCY_REAGENTS_COUNT_PCT, "Modify Spell Reagent Cost%"                        },
  { A_MOD_ATTACKER_MELEE_HIT_CHANCE,         "Modify Attacker Melee Hit Chance"                  },
  { A_MOD_ATTACKER_RANGED_HIT_CHANCE,        "Modify Attacker Ranged Hit Chance"                 },
  { A_MOD_ATTACKER_SPELL_HIT_CHANCE,         "Modify Attacker Spell Hit Chance"                  },
  { A_MOD_ATTACKER_MELEE_CRIT_CHANCE,        "Modify Attacker Melee Crit Chance"                 },
  { A_MOD_ATTACKER_RANGED_CRIT_CHANCE,       "Modify Attacker Ranged Crit Chance"                },
  { A_MOD_RATING,                            "Modify Rating"                                     },
  { A_USE_NORMAL_MOVEMENT_SPEED,             "Use Base Move Speed"                               },
  { A_MOD_MELEE_RANGED_HASTE,                "Modify Ranged and Melee Haste%"                    },
  { A_HASTE_ALL,                             "Modify All Haste%"                                 },
  { A_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE, "Modify Attacker Crit Chance"                   },
  { A_PCT_RATING_ADDED_TO_RATING,            "% of Misc1 Rating Added to Misc2 Rating"           },
  { A_MOD_KILL_XP_PCT,                       "Modify Experience Gained from Kills"               },
  { A_FLY,                                   "Fly"                                               },
  { A_MOD_ATTACKER_MELEE_CRIT_DAMAGE,        "Modify Melee Crit Damage Taken from Attacker"      },
  { A_PREVENT_RELEASE_SPIRIT,                "Prevent Releasing Spirit"                          },
  { A_MOD_RECHARGE_RATE_CATEGORY_MASK,       "Modify Cooldown Recharge Rate (CategoryTypes)"     },
  { A_MOD_RAGE_FROM_DAMAGE_DEALT,            "Modify Rage Generated From Auto Attacks"           },
  { A_HASTE_SPELLS,                          "Modify Casting Speed"                              },
  { A_MOD_MELEE_HASTE_2,                     "Modify Melee Haste"                                },
  { A_ADD_PCT_LABEL_MODIFIER,                "Apply Percent Modifier w/ Label"                   },
  { A_ADD_FLAT_LABEL_MODIFIER,               "Apply Flat Modifier w/ Label"                      },
  { A_MODIFY_SCHOOL,                         "Modify Spell School"                               },
  { A_REMOVE_TAUNT_EFFECTS,                  "Remove Taunt Effects"                              },
  { A_REMOVE_TRANSMOG_COST,                  "Remove Transmog Cost"                              },
  { A_REMOVE_BARBER_COST,                    "Remove Barber Cost"                                },
  { A_LEARN_TALENT,                          "Grant Talent"                                      },
  { A_PERIODIC_DUMMY,                        "Periodic Dummy"                                    },
  { A_DETECT_STEALTH,                        "Stealth Detection"                                 },
  { A_MOD_AOE_DAMAGE_AVOIDANCE,              "Modify AoE Damage Taken%"                          },
  { A_MOD_MAX_HEALTH,                        "Modify Max Health"                                 },
  { A_PROC_TRIGGER_SPELL_WITH_VALUE,         "Trigger Spell with Value"                          },
  { A_MECHANIC_DURATION_MOD,                 "Modify Mechanic Duration% (Stacking)"              },
  { A_CHANGE_ALL_HUMANOID_MODELS,            "Change all Humanoid Models"                        },
  { A_MECHANIC_DURATION_MOD_NOT_STACK,       "Modify Mechanic Duration%"                         },
  { A_MOD_DISPEL_RESIST,                     "Resist Dispel"                                     },
  { A_CONTROL_VEHICLE,                       "Control Vehicle"                                   },
  { A_MOD_SCALE_2,                           "Scale%"                                            },
  { A_MOD_EXPERTISE,                         "Modify Expertise%"                                 },
  { A_FORCE_MOVE_FORWARD,                    "Forced Movement"                                   },
  { A_MOD_SPELL_DAMAGE_FROM_HEALING,         "Modify Spell Damage from Healing"                  },
  { A_MOD_FACTION,                           "Change Faction"                                    },
  { A_COMPREHEND_LANGUAGE,                   "Comprehend Language"                               },
  { A_MOD_DURATION_OF_MAGIC_EFFECTS,         "Modify Debuff Duration%"                           },
  { A_CLONE_CASTER,                          "Copy Appearance"                                   },
  { A_MOD_INCREASE_HEALTH_2,                 "Increase Max Health (Stacking)"                    },
  { A_MOD_BLOCK_CRIT_CHANCE,                 "Modify Critical Block Chance"                      },
  { A_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT,     "Modify Damage Taken% from Mechanic"                },
  { A_NO_REAGENT_USE,                        "No Reagent Cost"                                   },
  { A_MOD_TARGET_RESIST_BY_SPELL_CLASS,      "Modify Damage Taken from Spell School"             },
  { A_OVERRIDE_SUMMONED_OBJECT,              "Modify Periodic Damage Taken%"                     },
  { A_MOD_HOT_RECIEVED_PCT,                  "Modify Periodic Healing Received%"                 },
  { A_ABILITY_IGNORE_AURA_STATE,             "Ignore Aura State"                                 },
  { A_ALLOW_ONLY_ABILITY,                    "Disable Abilities"                                 },
  { A_DISABLE_ATTACKING_EXCEPT_ABILITIES,    "Disable Spells"                                    },
  { A_MOD_IMMUNE_A_APPLY_SCHOOL,             "Immune Debuff Application from School"             },
  { A_MOD_ARMOR_BY_PRIMARY_STAT_PCT,         "Modify Armor by Primary Stat%"                     },
  { A_MOD_DAMAGE_TO_CASTER,                  "Modify Damage Done% to Caster"                     },
  { A_MOD_DAMAGE_FROM_CASTER,                "Modify Damage Taken% from Caster"                  },
  { A_MOD_DAMAGE_FROM_CASTER_SPELLS,         "Modify Damage Taken% from Caster's Spells"         },
  { A_MOD_BLOCK_PCT,                         "Modify Block Value%"                               },
  { A_MOD_BLOCK_FLAT,                        "Add Block Value"                                   },
  { A_MOD_IGNORE_SHAPESHIFT,                 "Modify Stance Mask"                                },
  { A_MOD_DAMAGE_TAKEN_FROM_MECHANIC,        "Modify Damage Taken from Mechanic"                 },
  { A_MOD_TARGET_ARMOR_PCT,                  "Modify Target Armor%"                              },
  { A_MOD_HEALING_RECEIVED_FROM_SPELL,       "Modify Healing Taken% from Caster's Spells"        },
  { A_LINKED_SPELL,                          "Cast Linked Spell"                                 },
  { A_LINKED_SPELL_WITH_VALUE,               "Cast Linked Spell w/ Value"                        },
  { A_MOD_RECHARGE_RATE,                     "Modify Cooldown Recharge Rate%"                    },
  { A_MOD_ALL_CRIT_CHANCE,                   "Modify Critical Strike%"                           },
  { A_MOD_QUEST_XP_PCT,                      "Modify Experience Gained from Quests"              },
  { A_OVERRIDE_SPELLS,                       "Override Spells"                                   },
  { A_PREVENT_REGENERATE_POWER,              "Prevent Power Regeneration"                        },
  { A_MOD_PERIODIC_DAMAGE_TAKEN,             "Modify Periodic Damage Taken"                      },
  { A_SET_VEHICLE_ID,                        "Set Vehicle ID"                                    },
  { A_MOD_ROOT_DISABLE_GRAVITY,              "Modify Root Effects - Disable Gravity"             },
  { A_MOD_STUN_DISABLE_GRAVITY,              "Modify Stun Effects - Disable Gravity"             },
  { A_SHARE_DAMAGE_PCT,                      "Share Damage Taken"                                },
  { A_SCHOOL_HEAL_ABSORB,                    "Absorb Healing"                                    },
  { A_MOD_DAMAGE_DONE_VERSUS_AURASTATE,      "Modify Damage Done Against Target With Aura"       },
  { A_MOD_MINIMUM_SPEED_PCT,                 "Modify Min Speed%"                                 },
  { A_MOD_CRIT_CHANCE_FROM_CASTER,           "Modify Crit Chance% from Caster"                   },
  { A_ENABLE_CAST_WHILE_MOVING_FOR_SPELL_LABEL, "Allow Casting while Moving For Spells (Label)"  },
  { A_MOD_CRIT_CHANCE_FROM_CASTER_SPELLS,    "Modify Crit Chance% from Caster's Spells"          },
  { A_MOD_RESILIENCE,                        "Modify Resilience"                                 },
  { A_MODE_CREATURE_AOE_DAMAGE_AVOIDANCE,    "Modify Creature AoE Damage Avoidance"              },
  { A_IGNORE_COMBAT,                         "Ignore Combat State"                               },
  { A_REPLACE_ANIMATION_SET,                 "Replace Animation (Set)"                           },
  { A_REPLACE_MOUNT_ANIMATION_SET,           "Replace Mount Animation (Set)"                     },
  { A_PREVENT_RESURRECTION,                  "Prevent Resurrection"                              },
  { A_UNDERWATER_WALKING,                    "Enable Underwater Walking"                         },
  { A_SCHOOL_ABSORB_OVERKILL,                "Absorb Overkill Damage From School"                },
  { A_MOD_MASTERY_PCT,                       "Modify Mastery%"                                   },
  { A_MOD_MELEE_AUTO_ATTACK_SPEED,           "Modify Melee Auto Attack Speed%"                   },
  { A_APPLY_HASTED_GCD_LABEL,                "Apply Hasted GCD to Spells in Label"               },
  { A_DISABLE_ACTIONS,                       "Disable Actions"                                   },
  { A_DISABLE_TARGETING,                     "Disable Targeting"                                 },
  { A_TRIGGER_SPELL_ON_POWER_PCT,            "Trigger Spell Based on Resource%"                  },
  { A_MOD_POWER_GAIN_PCT,                    "Modify Resource Generation%"                       },
  { A_CAST_WHILE_MOVING_WHITELIST,           "Cast while Moving (Whitelist)"                     },
  { A_FORCE_WEATHER,                         "Force Weather"                                     },
  { A_OVERRIDE_ACTION_SPELL,                 "Override Action Spell (Misc w/ Base)"              },
  { A_OVERRIDE_ACTION_SPELL_TRIGGERED,       "Override Triggered Action Spell (Misc w/ Base)"    },
  { A_MOD_AUTOATTACK_CRIT_CHANCE,            "Modify Auto Attack Critical Chance"                },
  { A_RESTRICT_MOUNTS,                       "Restrict Mounts"                                   },
  { A_MOD_CRIT_CHANCE_FOR_CASTER_PET,        "Modify Crit Chance% from Caster's Pets"            },
  { A_MOD_RESURRECTION_HEALTH,               "Modify Resurrection Health%"                       },
  { A_MODIFY_CATEGORY_COOLDOWN,              "Modify Cooldown Time (Category)"                   },
  { A_MOD_RANGED_AND_MELEE_AUTO_ATTACK_SPEED,"Modify Ranged and Melee Auto Attack Speed%"        },
  { A_MOD_AUTO_ATTACK_FROM_CASTER,           "Modify Auto Attack Damage Taken% from Caster"      },
  { A_MOD_AUTO_ATTACK_PCT,                   "Modify Auto Attack Damage Done%"                   },
  { A_MOD_IGNORE_ARMOR_PCT,                  "Ignore Armor%"                                     },
  { A_ENABLE_ALTERATE_POWER,                 "Enable Secondary Resource Cost"                    },
  { A_MOD_COOLDOWN_BY_HASTE,                 "Modify Cooldown Duration by Haste%"                },
  { A_MOD_HEALING_DONE_PCT_VS_TARGET_HEALTH, "Modify Healing% Based on Target Health%"           },
  { A_MOD_SPELL_HASTE,                       "Modify Spell Haste%"                               },
  { A_PROC_TRIGGER_SPELL_COPY,               "Duplicate Ability"                                 },
  { A_OVERRIDE_AUTO_ATTACK_WITH_ABILITY,     "Override Auto-Attack with Ability"                 },
  { A_OVERRIDE_SP_PER_AP,                    "Override Spell Power per Attack Power%"            },
  { A_OVERRIDE_AUTO_ATTACK_WITH_SPELL,       "Override Auto-Attack with Spell"                   },
  { A_MOD_SPEED_NO_CONTROL,                  "Force Move (No Control)"                           },
  { A_374,                                   "Reduce Fall Damage%"                               },
  { A_ENABLE_CAST_WHILE_MOVING,              "Cast while Moving"                                 },
  { A_MOD_POSSESS_PET,                       "Possess Pet"                                       },
  { A_MOD_MANA_REGEN_PCT,                    "Modify Mana Regen%"                                },
  { A_MOD_DAMAGE_FROM_CASTER_GUARDIAN,       "Modify Damage Taken% from Caster Guardian"         },
  { A_MOD_DAMAGE_FROM_CASTER_PET,            "Modify Damage Taken% from Caster Pet"              },
  { A_MOD_PET_STAT,                          "Modify Pet Stat"                                   },
  { A_IGNORE_SPELL_COOLDOWN,                 "Ignore Spell Cooldown"                             },
  { A_BLOCK_SPELLS_IN_FRONT,                 "Block Spells In Front"                             },
  { A_AREA_TRIGGER,                           "Area Trigger"                                     },
  { A_TRIGGER_SPELL_ON_POWER_AMOUNT,         "Trigger Spell Based on Resource Amount"            },
  { A_MOD_TIME_RATE,                         "Modify Time Rate"                                  },
  { A_MOD_SKILL_2,                           "Modify Skill"                                      },
  { A_OVERRIDE_POWER_DISPLAY,                "Override Resource Display"                         },
  { A_OVERRIDE_SPELL_VISUAL,                 "Override Spell Visual"                             },
  { A_OVERRIDE_AP_PER_SP,                    "Override Attack Power per Spell Power%"            },
  { A_MOD_RATING_MULTIPLIER,                 "Modify Combat Rating Multiplier"                   },
  { A_KEYBIND_OVERRIDE,                      "Override Keybind"                                  },
  { A_MOD_FEAR_2,                            "Modify Fear"                                       },
  { A_SET_ACTION_BUTTON_SPELL_COUNT,         "Set Action Button Spell Count"                     },
  { A_CAN_TURN_WHILE_FALLING,                "Slow Fall"                                         },
  { A_MOD_MAX_CHARGES,                       "Modify Cooldown Charge (Category)"                 },
  { A_MOD_RANGED_ATTACK_DEFLECT_CHANCE,      "Modify Ranged Attack Deflect Chance"               },
  { A_MOD_RANGED_ATTACK_BLOCK_CHANCE_IN_FRONT, "Modify Ranged Attack Block Chance from Front"    },
  { A_HASTED_COOLDOWN,                       "Hasted Cooldown Duration"                          },
  { A_HASTED_GCD,                            "Hasted Global Cooldown"                            },
  { A_MOD_MAX_RESOURCE,                      "Modify Max Resource"                               },
  { A_MOD_MANA_POOL_PCT,                     "Modify Mana Pool%"                                 },
  { A_MOD_BATTLEPET_EXP_PCT,                 "Modify Battlepet Experience Gained"                },
  { A_MOD_ABSORB_DONE_PERCENT,               "Modify Absorb% Done"                               },
  { A_MOD_ABSORB_RECEIVED_PERCENT,           "Modify Absorb% Received"                           },
  { A_MOD_MANA_COST_PCT,                     "Modify Mana Cost%"                                 },
  { A_CASTER_IGNORE_LINE_OF_SIGHT,           "Caster Ignores Line of Sight"                      },
  { A_SCALE_PLAYER_LEVEL,                    "Scale Player Level"                                },
  { A_TRIGGER_SUMMON_WITH_DURATION_OVERRIDE, "Trigger Summon Spell w/ Duration Override"         },
  { A_MOD_PET_DAMAGE_DONE,                   "Modify Pet Damage Done%"                           },
  { A_MOD_ENVIRONMENTAL_DAMAGE_TAKEN,        "Modify Environmental Damage Taken"                 },
  { A_MOD_MINIMUM_SPEED,                     "Modify Minimum Speed"                              },
  { A_MOD_MULTISTRIKE_DAMAGE,                "Modify Multistrike Damage"                         },
  { A_MOD_MULTISTRIKE_CHANCE,                "Modify Multistrike%"                               },
  { A_MOD_LEECH_PERCENT,                     "Modify Leech%"                                     },
  { A_ADVANCED_FLYING,                       "Dragonriding"                                      },
  { A_MOD_RECHARGE_TIME_CATEGORY,            "Modify Recharge Time (Category)"                   },
  { A_MOD_RECHARGE_TIME_PCT_CATEGORY,        "Modify Recharge Time% (Category)"                  },
  { A_MOD_ROOT_2,                            "Root (Respects Threat Table)"                      },
  { A_HASTED_CATEGORY,                       "Hasted Cooldown Duration (Category)"               },
  { A_HASTED_CATEGORY_REGEN,                 "Hasted Cooldown Regeneration (Category)"           },
  { A_DUAL_WIELD_HIT_PENALTY,                "Dual Wield Hit Chance Penalty"                     },
  { A_MOD_HEALING_AND_ABSORB_FROM_CASTER,    "Modify Healing and Absorb Recieved from Caster"    },
  { A_MOD_PARRY_FROM_CRIT_RATING,            "Modify Parry Rating with Crit Rating"              },
  { A_MOD_ATTACK_POWER_FROM_BONUS_ARMOR,     "Modify Attack Power with Bonus Armor"              },
  { A_MOD_BONUS_ARMOR,                       "Increase Armor"                                    },
  { A_MOD_BONUS_ARMOR_PCT,                   "Modify Armor%"                                     },
  { A_MOD_STAT_BONUS_PCT,                    "Modify Stat Bonus%"                                },
  { A_TRIGGER_SPELL_BY_HEALTH_PCT,           "Trigger Spell Based on Health%"                    },
  { A_MOD_TIME_RATE_BY_SPELL_LABEL,          "Modify Time Rate (Label)"                          },
  { A_MOD_VERSATILITY_PCT,                   "Modify Versatility%"                               },
  { A_PREVENT_DURABILITY_LOSS_FROM_COMBAT,   "Prevent Durability Loss from Combat"               },
  { A_REPLACE_ITEM_BONUS_TREE,               "Replace Item Bonus Tree"                           },
  { A_ALLOW_USING_GAMEOBJECT_WHILE_MOUNTED,  "Allow Interaction while Mounted"                   },
  { A_MOD_ARTIFACT_ITEM_LEVEL,               "Modify Artifact Item Level"                        },
  { A_CONVERT_CONSUMED_RUNE,                 "Convert Consumed Rune"                             },
  { A_SUPPRESS_TRANSFORM,                    "Supress Transformation"                            },
  { A_ALLOW_INTERRUPT_SPELL,                 "Allow Interrupt Spell"                             },
  { A_MOD_MOVEMENT_FORCE_MAGNITUDE,          "Resist Forced Movement%"                           },
  { A_COSMETIC_MOUNTED,                      "Mount Cosmetic Mount"                              },
  { A_DISABLE_GRAVITY,                       "Disable Gravity"                                   },
  { A_493,                                   "Hunter Animal Companion"                           },
  { A_SET_POWER_POINT_CHARGE,                "Set Power Point Charge"                            },
  { A_TRIGGER_SPELL_ON_EXPIRE,               "Trigger Spell on Aura Expire"                      },
  { A_IGNORE_SPELL_CHARGE_COOLDOWN_CATEGORY, "Ignore Spell Charge Cooldown (Category)"           },
  { A_MOD_CRIT_DAMAGE_PCT_FROM_CASTER_SPELLS,"Modify Crit Damage Done% from Caster's Spells"     },
  { A_MOD_VERSATILITY_DAMAGE_BENEFIT,        "Modify Versatility Damage Benefit%"                },
  { A_MOD_VERSATILITY_HEALING_BENEFIT,       "Modify Versatility Healing Benefit%"               },
  { A_MOD_HEALING_RECEIVED_FROM_CASTER,      "Modify Healing Taken% from Caster"                 },
  { A_MOD_DAMAGE_FROM_SPELLS_LABEL,          "Modify Damage Taken% from Spells (Label)"          },
  { A_APPLY_PROFESSION_EFFECT,               "Enable Profession Effects"                         },
  { A_MOD_COOLDOWN_RECOVERY_RATE,            "Modify Cooldown Recovery Rate"                     },
  { A_ALLOW_BLOCKING_SPELLS,                 "Allow Blocking Spells"                             },
  { A_MOD_SPELL_BLOCK_CHANCE,                "Modify Spell Block Chance"                         },
  { A_MOD_AUTO_ATTACK_DAMAGE_PCT,            "Modify Auto Attack Damage%"                        },
  { A_MOD_GUARDIAN_DAMAGE_DONE,              "Modify Guardian Damage Done%"                      },
  { A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL,   "Modify Damage Taken% from Caster's Spells (Label)" },
  { A_MOD_SUPPORT_STAT,                      "Modify Stat With Support Triggers"                 },
  { A_TRIGGER_SPELL_ON_STACK_AMOUNT,         "Trigger Spell on Aura Stack Count"                 },
  { A_MOD_CRITICAL_BLOCK_AMOUNT,             "Modify Critical Block Amount"                      },
  { A_MOD_DAMAGE_DONE_TO_CASTER_FROM_SCHOOL, "Modify Damage done to Caster from School"          },
  { A_MOD_RANGED_ATTACK_SPEED_FLAT,          "Modify Ranged Attack Speed Flat"                   },
} );

static constexpr auto _effect_attribute_strings = util::make_static_map<unsigned, std::string_view>( {
  { EX_NO_IMMUNITY,                           "No Immunity"                                      },
  { EX_POS_RELATIVE_TO_FACING,                "Position is facing relative"                      },
  { EX_JUMP_CHARGE_MELEE_RANGE,               "Jump Charge Unit Melee Range"                     },
  { EX_JUMP_CHARGE_PATH_CHECK,                "Jump Charge Unit Strict Path Check"               },
  { EX_EXCLUDE_OWN_PARTY,                     "Exclude Own Party"                                },
  { EX_ALWAYS_USE_AOE_LOS,                    "Always AOE Line of Sight"                         },
  { EX_SUPPRESS_STACKING,                     "Suppress Points Stacking"                         },
  { EX_CHAIN_FROM_INITIAL,                    "Chain from Initial Target"                        },
  { EX_UNCONTROLLED_NO_BACKWARDS,             "Uncontrolled No Backwards"                        },
  { EX_AURA_STACK,                            "Aura Points Stack"                                },
  { EX_NO_COPY_DMG_INT_PROC,                  "No Copy Damage Interrupts or Procs"               },
  { EX_ADD_TARGET_REACH_TO_AOE,               "Add Target (Dest) Combat Reach to AOE"            },
  { EX_IS_HARMFUL,                            "Is Harmful"                                       },
  { EX_FORCE_SCALE_CAM_MIN_HEIGHT,            "Force Scale to Override Camera Min Height"        },
  { EX_PLAYERS_ONLY,                          "Players Only"                                     },
  { EX_COMPUTE_ON_CAST,                       "Compute Points Only At Cast Time"                 },
  { EX_ENFORCE_LOS_ON_CHAIN,                  "Enforce Line of Sight To Chain Targets"           },
  { EX_AOE_USE_TARGET_RADIUS,                 "Area Effects Use Target Radius"                   },
  { EX_TELEPORT_WITH_VEHICLE,                 "Teleport With Vehicle (during map transfer)"      },
  { EX_SCALE_POINTS_BY_CHALLENGE_MOD_SCALER,  "Scale Points by Challenge Mode Damage Scaler"     },
  { EX_DONT_FAIL_CAST_ON_TARGETING_FAIL,      "Don't Fail Spell On Targeting Failure"            },
  { EX_ALWAYS_HIT,                            "Always Hit"                                       },
  { EX_IGNORE_DURING_COOLDOWN_TIME_RATE_CALC, "Ignore During Cooldown Time Rate Calculation"     },
  { EX_ONLY_AFFECTS_ABSORBS,                  "Damage Only Affects Absorbs"                      },
} );

static constexpr auto _mechanic_strings = util::make_static_map<unsigned, std::string_view>( {
  { MECHANIC_CHARM,           "Charm"          },
  { MECHANIC_DISORIENT,       "Disorient"      },
  { MECHANIC_DISARM,          "Disarm"         },
  { MECHANIC_DISTRACT,        "Distract"       },
  { MECHANIC_FLEE,            "Flee"           },
  { MECHANIC_KNOCKBACK,       "Knockback"      },
  { MECHANIC_ROOT,            "Root"           },
  { MECHANIC_SLOW,            "Slow"           },
  { MECHANIC_SILENCE,         "Silence"        },
  { MECHANIC_SLEEP,           "Sleep"          },
  { MECHANIC_SNARE,           "Snare"          },
  { MECHANIC_STUN,            "Stun"           },
  { MECHANIC_FREEZE,          "Freeze"         },
  { MECHANIC_INCAPACITATE,    "Incapacitate"   },
  { MECHANIC_BLEED,           "Bleed"          },
  { MECHANIC_HEAL,            "Heal"           },
  { MECHANIC_POLYMORPH,       "Polymorph"      },
  { MECHANIC_BANISH,          "Banish"         },
  { MECHANIC_SHIELD,          "Shield"         },
  { MECHANIC_SHACKLE,         "Shackle"        },
  { MECHANIC_MOUNT,           "Mount"          },
  { MECHANIC_INFECT,          "Infect"         },
  { MECHANIC_TURN,            "Turn"           },
  { MECHANIC_HORRIFY,         "Horrify"        },
  { MECHANIC_INVULNERABLE_2,  "Invulnerable 2" },
  { MECHANIC_INTERRUPT,       "Interrupt"      },
  { MECHANIC_DAZE,            "Daze"           },
  { MECHANIC_DISCOVER,        "Discover"       },
  { MECHANIC_INVULNERABLE,    "Invulnerable"   },
  { MECHANIC_SAP,             "Sap"            },
  { MECHANIC_ENRAGE,          "Enrage"         },
  { MECHANIC_WOUND,           "Wound"          },
  { MECHANIC_INFECT_2,        "Infect 2"       },
  { MECHANIC_INFECT_3,        "Infect 3"       },
  { MECHANIC_INFECT_4,        "Infect 4"       },
  { MECHANIC_TAUNT,           "Taunt"          },
} );

static constexpr auto _label_strings = util::make_static_map<int, std::string_view>( {
  { LABEL_CLASS_SPELLS,         "Class Spells"         },
  { LABEL_MAGE_SPELLS,          "Mage Spells"          },
  { LABEL_PRIEST_SPELLS,        "Priest Spells"        },
  { LABEL_WARLOCK_SPELLS,       "Warlock Spells"       },
  { LABEL_ROGUE_SPELLS,         "Rogue Spells"         },
  { LABEL_DRUID_SPELLS,         "Druid Spells"         },
  { LABEL_MONK_SPELLS,          "Monk Spells"          },
  { LABEL_HUNTER_SPELLS,        "Hunter Spells"        },
  { LABEL_SHAMAN_SPELLS,        "Shaman Spells"        },
  { LABEL_WARRIOR_SPELLS,       "Warrior Spells"       },
  { LABEL_PALADIN_SPELLS,       "Paladin Spells"       },
  { LABEL_DEATH_KNIGHT_SPELLS,  "Death Knight Spells"  },
  { LABEL_DEMON_HUNTER_SPELLS,  "Demon Hunter Spells"  },
  { LABEL_AZERITE_ESSENCES,     "Azerite Essences"     },
  { LABEL_MAJOR_COOLDOWNS,      "Major Cooldowns"      },
  { LABEL_HEALING_SPELLS,       "Healing Spells"       },
  { LABEL_COVENANT,             "Covenant Spells"      },
  { LABEL_EVOKER_SPELLS,        "Evoker Spells"        },
  { LABEL_EVOKER_RED_SPELLS,    "Red Evoker Spells"    },
  { LABEL_EVOKER_BLUE_SPELLS,   "Blue Evoker Spells"   },
  { LABEL_EVOKER_GREEN_SPELLS,  "Green Evoker Spells"  },
  { LABEL_EVOKER_BRONZE_SPELLS, "Bronze Evoker Spells" },
  { LABEL_EVOKER_BLACK_SPELLS,  "Black Evoker Spells"  },
  { LABEL_ITEM_EFFECTS,         "Item Effects"         },
} );

static constexpr auto _scaling_class_strings = util::make_static_map<int, std::string_view>( {
  { -1, "Primary Attribute"        },
  { -2, "Restore Health/Resource"  },
  { -3, "Food/Gems Attribute"      },
  { -4, "Food/Gems Attribute"      },
  { -5, "Food/Gems Attribute"      },
  { -6, "Stamina"                  },
  { -7, "Secondary Rating"         },
  { -8, "Replace Primary"          },
  { -9, "Replace Secondary"        },
  { -10, "Restore Mana"            },
} );

std::string mechanic_str( unsigned mechanic )
{
  auto it = _mechanic_strings.find( mechanic );
  if ( it != _mechanic_strings.end() )
  {
    return std::string( it->second );
  }
  return fmt::format( "UnknownMechanic({})", mechanic );
}

std::string label_str( int label, const dbc_t& dbc, size_t wrap )
{
  auto it = _label_strings.find( label );
  if ( it != _label_strings.end() )
  {
    return fmt::format( "Affected Spells (Label): {} ({})", it->second, label );
  }

  auto affected_spells = dbc.spells_by_label( label );
  if ( affected_spells.empty() )
  {
    return "";
  }

  return wrap_concatenate( affected_spells, [ first = affected_spells.front() ]( const spell_data_t* spell ) {
    if ( spell == first )
      return fmt::format( "Affected Spells (Label): {} ({})", spell->name_cstr(), spell->id() );
    else
      return fmt::format( "{} ({})", spell->name_cstr(), spell->id() );
  }, wrap );
}

std::string spell_flags( const spell_data_t* spell )
{
  std::ostringstream s;

  s << "[";

  if ( spell->class_family() != 0 )
    s << "Spell Family (" << spell->class_family() << "), ";

  if ( spell->flags( spell_attribute::SX_PASSIVE ) )
    s << "Passive, ";

  if ( spell->flags( spell_attribute::SX_HIDDEN ) )
    s << "Hidden, ";

  if ( s.tellp() > 1 )
  {
    s.seekp( -2, std::ios_base::cur );
    s << "]";
  }
  else
    return {};

  return s.str();
}

void spell_flags_xml( const spell_data_t* spell, xml_node_t* parent )
{
  if ( spell->flags( spell_attribute::SX_PASSIVE ) )
    parent->add_parm( "passive", "true" );
}

std::string azerite_essence_str( const spell_data_t* spell, util::span<const azerite_essence_power_entry_t> data )
{
  // Locate spell in the array
  auto it = range::find_if( data, [ spell ]( const azerite_essence_power_entry_t& e ) {
    return e.spell_id_base[ 0 ] == spell->id() || e.spell_id_base[ 1 ] == spell->id() ||
           e.spell_id_upgrade[ 0 ] == spell->id() || e.spell_id_upgrade[ 1 ] == spell->id();
  } );

  if ( it == data.end() )
  {
    return "";
  }

  std::ostringstream s;

  s << "(";

  s << "Type: ";

  if ( it->spell_id_base[ 0 ] == spell->id() )
  {
    s << "Major/Base";
  }
  else if ( it->spell_id_base[ 1 ] == spell->id() )
  {
    s << "Minor/Base";
  }
  else if ( it->spell_id_upgrade[ 0 ] == spell->id() )
  {
    s << "Major/Upgrade";
  }
  else if ( it->spell_id_upgrade[ 1 ] == spell->id() )
  {
    s << "Minor/Upgrade";
  }
  else
  {
    s << "Unknown";
  }
  s << ", ";

  s << "Rank: " << it->rank;

  s << ")";

  return s.str();
}

}  // unnamed namespace

std::ostringstream& spell_info::effect_to_str( const dbc_t& dbc, const spell_data_t* spell, const spelleffect_data_t* e,
                                               std::ostringstream& s, int level, unsigned wrap )
{
  std::vector<std::string> tokens;
  std::string tmp_str;
  std::streamsize ssize = s.precision( 7 );

  // Start first line
  s << fmt::format( "#{:16}: ", fmt::format( "{} (id={})", e->index() + 1, e->id() ) );

  // Effect Type
  tmp_str = map_string( _effect_type_strings, e->raw_type() );

  switch ( e->type() )
  {
    case E_SCHOOL_DAMAGE:
      tmp_str += fmt::format( ": {}", util::school_type_string( spell->get_school_type() ) );
      break;
    case E_TRIGGER_SPELL:
    case E_TRIGGER_SPELL_WITH_VALUE:
    case E_TRIGGER_MISSILE:
    case E_TRIGGER_MISSILE_SPELL_WITH_VALUE:
    case E_FORCE_CAST:
    case E_FORCE_CAST_2:
    case E_FORCE_CAST_WITH_VALUE:
    case E_REDUCE_REMAINING_COOLDOWN:
    case E_MODIFY_AURA_STACKS:
    case E_REMOVE_AURA_2:
    case E_CANCEL_AURA:
    case E_PUSH_ABILITY_TO_ACTION_BAR:
      if ( e->trigger_spell_id() )
      {
        if ( dbc.spell( e->trigger_spell_id() ) != spell_data_t::nil() )
          tmp_str += fmt::format( ": {}", dbc.spell( e->trigger_spell_id() )->name_cstr() );
        else
          tmp_str += fmt::format( ": ({})", e->trigger_spell_id() );
      }
      break;
    default:
      break;
  }

  tokens.emplace_back( tmp_str );

  // Effect Subtype
  if ( e->subtype() > 0 )
  {
    tmp_str = map_string( _effect_subtype_strings, e->raw_subtype() );

    switch ( e->subtype() )
    {
      case A_PERIODIC_DAMAGE:
        tmp_str += fmt::format( ": {}", util::school_type_string( spell->get_school_type() ) );
        if ( e->period() != timespan_t::zero() )
          tmp_str += fmt::format( " every {} seconds", e->period().total_seconds() );
        break;
      case A_PERIODIC_HEAL:
      case A_PERIODIC_ENERGIZE:
      case A_PERIODIC_DUMMY:
      case A_PERIODIC_HEAL_PCT:
      case A_PERIODIC_LEECH:
        if ( e->period() != timespan_t::zero() )
          tmp_str += fmt::format( ": every {} seconds", e->period().total_seconds() );
        break;
      case A_PROC_TRIGGER_SPELL:
      case A_PROC_TRIGGER_SPELL_COPY:
      case A_TRIGGER_SPELL_ON_EXPIRE:
      case A_TRIGGER_SPELL_BY_HEALTH_PCT:
      case A_TRIGGER_SPELL_ON_POWER_PCT:
      case A_TRIGGER_SPELL_ON_POWER_AMOUNT:
      case A_TRIGGER_SPELL_ON_STACK_AMOUNT:
      case A_LINKED_SPELL:
      case A_LINKED_SPELL_WITH_VALUE:
      case A_OVERRIDE_AUTO_ATTACK_WITH_SPELL:
      case A_OVERRIDE_AUTO_ATTACK_WITH_ABILITY:
      case A_SET_ACTION_BUTTON_SPELL_COUNT:
        if ( e->trigger_spell_id() )
        {
          if ( dbc.spell( e->trigger_spell_id() ) != spell_data_t::nil() )
            tmp_str += fmt::format( ": {}", dbc.spell( e->trigger_spell_id() )->name_cstr() );
          else
            tmp_str += fmt::format( ": ({})", e->trigger_spell_id() );
        }
        break;
      case A_PERIODIC_TRIGGER_SPELL:
        if ( e->trigger_spell_id() && dbc.spell( e->trigger_spell_id() ) != spell_data_t::nil() )
          tmp_str += fmt::format( ": {}", dbc.spell( e->trigger_spell_id() )->name_cstr() );
        else
          tmp_str += fmt::format( ": Unknown({})", e->trigger_spell_id() );

        if ( e->period() != timespan_t::zero() )
          tmp_str += fmt::format( " every {} seconds", e->period().total_seconds() );
        break;
      case A_TRIGGER_SUMMON_WITH_DURATION_OVERRIDE:
        if ( e->trigger_spell_id() && dbc.spell( e->trigger_spell_id() ) != spell_data_t::nil() )
          tmp_str += fmt::format( ": {}", dbc.spell( e->trigger_spell_id() )->name_cstr() );
        else
          tmp_str += fmt::format( ": Unknown({})", e->trigger_spell_id() );
        if ( dbc.spell( e->spell_id() )->duration().total_seconds() > 0 )
          tmp_str += fmt::format( " for {} seconds", dbc.spell( e->spell_id() )->duration().total_seconds() );
        if ( dbc.spell( e->spell_id() )->duration().total_seconds() < 0 )
          tmp_str += fmt::format( " until cancelled" );
        break;
      case A_OVERRIDE_ACTION_SPELL:
      case A_OVERRIDE_ACTION_SPELL_TRIGGERED:
        if ( e->misc_value1() && e->base_value() )
        {
          if ( dbc.spell( e->misc_value1() ) != spell_data_t::nil() &&
               dbc.spell( as<unsigned>( e->base_value() ) ) != spell_data_t::nil() )
            tmp_str += fmt::format( ": {} overrides {}", dbc.spell( as<unsigned>( e->base_value() ) )->name_cstr(),
                                    dbc.spell( as<unsigned>( e->misc_value1() ) )->name_cstr() );
          else
            tmp_str += fmt::format( ": ({}) overrides ({})", e->base_value(), e->misc_value1() );
        }
        break;
      case A_ADD_FLAT_MODIFIER:
      case A_ADD_PCT_MODIFIER:
      case A_ADD_PCT_LABEL_MODIFIER:
      case A_ADD_FLAT_LABEL_MODIFIER:
        tmp_str += fmt::format( ": {}", map_string( _property_type_strings, e->misc_value1() ) );
        break;
      default:
        break;
    }

    tokens.emplace_back( tmp_str );
  }

  if ( e->_scaling_type )
    tokens.emplace_back( fmt::format( "Scaling Class: {}", map_string( _scaling_class_strings, e->_scaling_type ) ) );

  // TODO: wrap within the attribute list as well?
  if ( e->_attribute )
  {
    std::vector<std::string> attr_str;
    for ( unsigned flag = 0; flag < 32; flag++ )
      if (e->_attribute & ( 1 << flag ))
      {
        auto it = _effect_attribute_strings.find( flag + 1 );
        if( it != _effect_attribute_strings.end() )
          attr_str.push_back( fmt::format( "{} ({})", it->second, flag ) );
        else
          attr_str.push_back( fmt::format( "Unknown({})", flag ) );
      }

    tokens.emplace_back( fmt::format( "Attributes: {}", util::string_join( attr_str ) ) );
  }

  // Print first line
  s << wrap_join( tokens, wrap, " | ", " |\n                   " ) << std::endl;

  // Start second line
  tokens.clear();
  s << "                   ";

  tokens.emplace_back( fmt::format( "Base Value: {}", e->base_value() ) );

  tmp_str = "Scaled Value: ";
  if ( level <= MAX_LEVEL )
  {
    double v_min = dbc.effect_min( e, level );
    double v_max = dbc.effect_max( e, level );

    if ( v_min != v_max )
      tmp_str += fmt::format( "{:.7g} - {:.7g}", v_min, v_max );
    else
      tmp_str += fmt::format( "{:.7g}", v_min );
  }
  else
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( level );
    double item_budget = ilevel_data.p_epic[ 0 ];
    auto coefficient = 1.0;

    if ( e->scaling_class() == PLAYER_SPECIAL_SCALE7 )
    {
      // Technically this should check for the item type, but that's not possible right now
      coefficient = dbc.combat_rating_multiplier( level, CR_MULTIPLIER_TRINKET );
    }
    else if ( e->scaling_class() == PLAYER_SPECIAL_SCALE8 )
    {
      item_budget = ilevel_data.damage_replace_stat;
    }
    else if ( ( e->scaling_class() == PLAYER_NONE || e->scaling_class() == PLAYER_SPECIAL_SCALE9 ) &&
              spell->flags( spell_attribute::SX_SCALE_ILEVEL ) )
    {
      item_budget = ilevel_data.damage_secondary;
    }

    tmp_str += fmt::format( "{:.7g}", item_budget * e->m_coefficient() * coefficient );
  }

  if ( e->m_coefficient() && e->m_delta() )
    tmp_str += fmt::format( " (coefficient={:.7g}, delta={})", e->m_coefficient(), e->m_delta() );
  else if ( e->m_coefficient() )
    tmp_str += fmt::format( " (coefficient={:.7g})", e->m_coefficient() );
  else if ( e->m_delta() )
    tmp_str += fmt::format( " (delta={})", e->m_delta() );

  tokens.emplace_back( tmp_str );

  if ( level <= MAX_LEVEL && e->m_unk() )
    tokens.emplace_back( fmt::format( "Bonus Value: {} ({})", dbc.effect_bonus( e->id(), level ), e->m_unk() ) );

  if ( e->real_ppl() != 0 )
    tokens.emplace_back( fmt::format( "Points Per Level: {}", e->real_ppl() ) );

  if ( e->m_value() != 0 )
    tokens.emplace_back( fmt::format( "Value Multiplier: {}", e->m_value() ) );

  if ( e->sp_coeff() != 0 )
    tokens.emplace_back( fmt::format( "SP Coefficient: {:.7g}", e->sp_coeff() ) );

  if ( e->ap_coeff() != 0 )
    tokens.emplace_back( fmt::format( "AP Coefficient: {:.7g}", e->ap_coeff() ) );

  if ( e->pvp_coeff() != 1.0 )
    tokens.emplace_back( fmt::format( "PvP Coefficient: {:.7g}", e->pvp_coeff() ) );

  if ( e->chain_target() != 0 )
    tokens.emplace_back( fmt::format( "Chain Multiplier: {}", e->chain_multiplier() ) );

  if ( e->type() == E_ENERGIZE || e->type() == E_ENERGIZE_PCT ||
       ( e->type() == E_APPLY_AURA && 
                                  ( e->subtype() == A_MOD_INCREASE_RESOURCE || e->subtype() == A_MOD_MAX_RESOURCE ||
                                    e->subtype() == A_MOD_POWER_REGEN_PERCENT || e->subtype() == A_TRIGGER_SPELL_ON_POWER_AMOUNT || 
                                    e->subtype() == A_TRIGGER_SPELL_ON_POWER_PCT ) ) )
  {
    tokens.emplace_back( fmt::format( "Resource: {}", util::resource_type_string( util::translate_power_type(
                                                        static_cast<power_e>( e->misc_value1() ) ) ) ) );
  }
  else if ( e->type() == E_APPLY_AURA && ( e->subtype() == A_MOD_STAT || e->subtype() == A_MOD_PERCENT_STAT ||
            e->subtype() == A_MOD_STAT_BONUS_PCT ) )
  {
    auto misc1 = e->misc_value1();
    if ( misc1 < -2 || misc1 >= STAT_MAX )
    {
      tokens.emplace_back( fmt::format( "Stat: Invalid ({})", misc1 ) );
    }
    else
    {
      auto stat = misc1 == -2 ? STAT_STR_AGI_INT : misc1 == -1 ? STAT_ALL : static_cast<stat_e>( misc1 + 1 );
      tokens.emplace_back( fmt::format( "Stat: {}", util::stat_type_abbrev( stat ) ) );
    }
  }
  else if ( e->type() == E_APPLY_AURA && ( e->subtype() == A_MOD_RATING || e->subtype() == A_MOD_RATING_MULTIPLIER ) )
  {
    std::vector<const char*> tmp;
    range::transform( util::translate_all_rating_mod( e->misc_value1() ), std::back_inserter( tmp ),
                      &util::stat_type_abbrev );

    tokens.emplace_back( fmt::format( "Rating: {}", util::string_join( tmp ) ) );
  }
  else if ( e->subtype() == A_MOD_MECHANIC_RESISTANCE || e->subtype() == A_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT )
  {
    tokens.emplace_back( fmt::format( "Mechanic: {}", mechanic_str( e->misc_value1() ) ) );
  }
  else if ( e->misc_value1() != 0 )
  {
    if ( range::contains( dbc::effect_category_subtypes(), e->subtype() ) || e->type() == E_MODIFY_COOLDOWN_IN_CATEGORY ||
         e->type() == E_RECHARGE_CATEGORY_COOLDOWN_IMMEDIATE )
    {
      tokens.emplace_back( fmt::format( "Misc Value: {} (Category)", e->misc_value1() ) );
    }
    else if ( e->affected_schools() != 0U )
    {
      tokens.emplace_back( fmt::format( "Misc Value: {:#x}", e->misc_value1() ) );
    }
    else if ( e->subtype() == A_MOD_RECHARGE_RATE_LABEL || e->subtype() == A_MOD_TIME_RATE_BY_SPELL_LABEL ||
              e->subtype() == A_MOD_DAMAGE_FROM_SPELLS_LABEL || e->subtype() == A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL ||
              e->subtype() == A_APPLY_HASTED_GCD_LABEL )
    {
      tokens.emplace_back( fmt::format( "Misc Value: {} (Label)", e->misc_value1() ) );
    }
    else if ( e->subtype() == A_MODIFY_SCHOOL )
    {
      tokens.emplace_back(
          fmt::format( "School: {}", util::school_type_string( dbc::get_school_type( e->misc_value1() ) ) ) );
    }
    else
    {
      tokens.emplace_back( fmt::format( "Misc Value: {}", e->misc_value1() ) );
    }
  }

  if ( e->misc_value2() != 0 )
  {
    if ( e->subtype() == A_MOD_TOTAL_STAT_PERCENTAGE )
    {
      auto misc2 = e->misc_value2();
      size_t idx = 0;
      while ( misc2 )
      {
        if ( misc2 & 0b1 )
          tokens.emplace_back( fmt::format( "Stat: {}", util::stat_type_abbrev( static_cast<stat_e>( idx + 1 ) ) ) );

        misc2 >>= 1;
        idx++;
      }
    }
    else if ( e->subtype() == A_ADD_PCT_LABEL_MODIFIER || e->subtype() == A_ADD_FLAT_LABEL_MODIFIER )
    {
      tokens.emplace_back( fmt::format( "Misc Value 2: {} (Label)", e->misc_value2() ) );
    }
    else if ( e->subtype() == A_SCHOOL_ABSORB || e->subtype() == A_MOD_PET_STAT )
    {
      tokens.emplace_back( fmt::format( "Misc Value 2: {}", e->misc_value2() ) );
    }
    else
    {
      tokens.emplace_back( fmt::format( "Misc Value 2: {:#x}", e->misc_value2() ) );
    }
  }

  if ( e->pp_combo_points() != 0 )
    tokens.emplace_back( fmt::format( "Points Per Combo Point: {}", e->pp_combo_points() ) );

  if ( e->trigger_spell_id() != 0 )
    tokens.emplace_back( fmt::format( "Trigger Spell: {}", e->trigger_spell_id() ) );

  if ( e->radius() > 0 || e->radius_max() > 0 )
  {
    if ( e->radius_max() > 0 && e->radius_max() != e->radius() )
      tokens.emplace_back( fmt::format( "Radius: {} - {} yards", e->radius(), e->radius_max() ) );
    else
      tokens.emplace_back( fmt::format( "Radius: {} yards", e->radius() ) );
  }

  if ( e->mechanic() > 0 )
    tokens.emplace_back( fmt::format( "Mechanic: {}", mechanic_str( e->mechanic() ) ) );

  if ( e->chain_target() > 0 )
    tokens.emplace_back( fmt::format( "Chain Targets: {}", e->chain_target() ) );

  if ( e->target_1() != 0 || e->target_2() != 0 )
  {
    if ( e->target_1() && !e->target_2() )
    {
      tokens.emplace_back( fmt::format( "Target: {}", map_string( _targeting_strings, e->target_1() ) ) );
    }
    else if ( !e->target_1() && e->target_2() )
    {
      tokens.emplace_back( fmt::format( "Target: [{}]", map_string( _targeting_strings, e->target_2() ) ) );
    }
    else
    {
      tokens.emplace_back( fmt::format( "Target: {} -> {}", map_string( _targeting_strings, e->target_1() ),
                                        map_string( _targeting_strings, e->target_2() ) ) );
    }
  }

  // Print second line
  s << wrap_join( tokens, wrap, " | ", " |\n                   " ) << std::endl;

  // Print third optional line
  if ( e->type() == E_APPLY_AURA && e->affected_schools() != 0U )
  {
    s << "                   Affected School(s): ";
    if ( e->affected_schools() == 0x7f )
    {
      s << "All";
    }
    else
    {
      std::vector<std::string> schools;
      for ( school_e school = SCHOOL_NONE; school < SCHOOL_MAX_PRIMARY; school++ )
      {
        if ( e->affected_schools() & dbc::get_school_mask( school ) )
          schools.emplace_back( util::inverse_tokenize( util::school_type_string( school ) ) );
      }

      fmt::print( s, "{}", fmt::join( schools, ", " ) );
    }

    s << std::endl;
  }

  // Print fourth optional line
  std::vector<const spell_data_t*> affected_spells = dbc.effect_affects_spells( spell->class_family(), e );
  if ( !affected_spells.empty() )
  {
    s << "                   ";
    s << wrap_concatenate( affected_spells, [ first = affected_spells.front() ]( const spell_data_t* spell ) {
      if ( spell == first )
        return fmt::format( "Affected Spells: {} ({})", spell->name_cstr(), spell->id() );
      else
        return fmt::format( "{} ({})", spell->name_cstr(), spell->id() );
    }, wrap );
    s << std::endl;
  }

  if ( e->type() == E_APPLY_AURA || e->type() == E_APPLY_AREA_AURA_PARTY || e->type() == E_APPLY_AREA_AURA_RAID ||
       e->type() == E_APPLY_AREA_AURA_ENEMY || e->type() == E_APPLY_AREA_AURA_FRIEND || e->type() == E_APPLY_AURA_PLAYER_AND_PET ||
       e->type() == E_APPLY_AREA_AURA_OWNER || e->type() == E_APPLY_AREA_AURA_PARTY_NONRANDOM || e->type() == E_APPLY_AREA_AURA_PET ||
       e->type() == E_REMOVE_AURA_BY_SPELL_LABEL || e->type() == E_MODIFY_COOLDOWN_IN_CATEGORY || e->type() == E_RECHARGE_CATEGORY_COOLDOWN_IMMEDIATE )
  {
    switch ( e->subtype() )
    {
        case A_MOD_RECHARGE_RATE_LABEL:
        case A_MOD_TIME_RATE_BY_SPELL_LABEL:
        case A_MOD_DAMAGE_FROM_SPELLS_LABEL:
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL:
        case A_ENABLE_CAST_WHILE_MOVING_FOR_SPELL_LABEL:
        case A_APPLY_HASTED_GCD_LABEL:
          if ( auto str = label_str( e->misc_value1(), dbc, wrap ); !str.empty() )
            s << "                   " << str << std::endl;
          break;
        case A_ADD_PCT_LABEL_MODIFIER:
        case A_ADD_FLAT_LABEL_MODIFIER:
          if ( auto str = label_str( e->misc_value2(), dbc, wrap ); !str.empty() )
            s << "                   " << str << std::endl;
          break;
        default:
          break;
    }

    if ( e->type() == E_REMOVE_AURA_BY_SPELL_LABEL )
      if ( auto str = label_str( e->misc_value1(), dbc, wrap ); !str.empty() )
        s << "                   " << str << std::endl;

    if ( range::contains( dbc::effect_category_subtypes(), e->subtype() ) ||
         e->type() == E_MODIFY_COOLDOWN_IN_CATEGORY ||
         e->type() == E_RECHARGE_CATEGORY_COOLDOWN_IMMEDIATE )
    {
      if ( auto affected = dbc.spells_by_category( e->misc_value1() ); !affected.empty() )
      {
        s << "                   Affected Spells (Category): ";
        s << wrap_concatenate( affected, []( const spell_data_t* spell ) {
          return fmt::format( "{} ({})", spell->name_cstr(), spell->id() );
        }, wrap );
        s << std::endl;
      }
    }
  }

  if ( spell->class_family() > 0 )
  {
    std::vector<unsigned> flags;
    for ( size_t i = 0; i < NUM_CLASS_FAMILY_FLAGS; ++i )
    {
      for ( size_t bit = 0; bit < 32; ++bit )
      {
        if ( ( 1 << bit ) & e->_class_flags[ i ] )
          flags.push_back( static_cast<unsigned>( i * 32 + bit ) );
      }
    }

    if ( !flags.empty() )
      fmt::print( s, "                   Family Flags: {}\n", fmt::join( flags, ", " ) );
  }

  const auto hotfixes = spelleffect_data_t::hotfixes( *e, dbc.ptr );
  if ( !hotfixes.empty() )
  {
    if ( hotfixes.front().field_id == hotfix::NEW_ENTRY )
      fmt::print( s, "Hotfixed         : NEW EFFECT\n" );
    else
      fmt::print( s, "Hotfixed         : {}\n", hotfix_map_str( hotfixes, _hotfix_effect_map ) );
  }

  s.precision( ssize );

  return s;
}

static std::string trait_data_to_str( const dbc_t& dbc, const spell_data_t* spell,
                                      const std::vector<const trait_data_t*>& traits )
{
  std::vector<std::string> strings;

  for ( const auto trait : traits )
  {
    std::vector<std::string> nibbles;

    talent_tree tree = static_cast<talent_tree>( trait->tree_index );

    std::vector<std::string> starters;
    auto spec_idx = 0U;
    while ( trait->id_spec_starter[ spec_idx ] != 0 && spec_idx < trait->id_spec_starter.size() )
    {
      auto specialization_str = util::inverse_tokenize(
          dbc::specialization_string( static_cast<specialization_e>( trait->id_spec_starter[ spec_idx ] ) ) );
      if ( util::str_compare_ci( specialization_str, "Unknown" ) )
      {
        starters.emplace_back( fmt::format( "{} ({})", specialization_str, trait->id_spec_starter[ spec_idx ] ) );
      }
      else
      {
        starters.emplace_back( fmt::format( "{}", specialization_str ) );
      }
      ++spec_idx;
    }

    if ( !starters.empty() )
    {
      nibbles.emplace_back( fmt::format( "free=({})", util::string_join( starters, ", " ) ) );
    }

    nibbles.emplace_back( fmt::format( "tree={}", util::talent_tree_string( tree ) ) );
    nibbles.emplace_back( fmt::format( "row={}", trait->row ) );
    nibbles.emplace_back( fmt::format( "col={}", trait->col ) );
    // Disabled for now as tree changes results in entirely new trees making NodeEntryID an unstable identifier
    // nibbles.emplace_back( fmt::format( "entry_id={}", trait->id_trait_node_entry ) );
    nibbles.emplace_back( fmt::format( "max_rank={}", trait->max_ranks ) );
    nibbles.emplace_back( fmt::format( "req_points={}", trait->req_points ) );

    if ( trait->selection_index != -1 )
    {
      nibbles.emplace_back( fmt::format( "select_idx={}", trait->selection_index ) );
    }

    if ( !util::str_compare_ci( spell->name_cstr(), trait->name ) )
    {
      nibbles.emplace_back( fmt::format( "name=\"{}\"", trait->name ) );
    }

    if ( trait->id_replace_spell > 0 )
    {
      const auto replace_spell = dbc.spell( trait->id_replace_spell );
      nibbles.emplace_back(
          fmt::format( "replace=\"{}\" (id={})", replace_spell->name_cstr(), trait->id_replace_spell ) );
    }

    if ( trait->id_override_spell > 0 )
    {
      const auto override_spell = dbc.spell( trait->id_override_spell );
      nibbles.emplace_back(
          fmt::format( "override=\"{}\" (id={})", override_spell->name_cstr(), trait->id_override_spell ) );
    }

    std::vector<std::string> spec_strs;
    for ( auto s_idx : trait->id_spec )
    {
      if ( s_idx == 0 )
        continue;

      auto spec_str = util::inverse_tokenize( dbc::specialization_string( static_cast<specialization_e>( s_idx ) ) );

      if ( util::str_compare_ci( spec_str, "Unknown" ) )
      {
        spec_strs.emplace_back( fmt::format( "{} ({})", spec_str, s_idx ) );
      }
      else
      {
        spec_strs.emplace_back( fmt::format( "{}", spec_str ) );
      }
    }

    if ( tree == talent_tree::HERO )
    {
      strings.emplace_back( fmt::format( "{} ({}) [{}]",
                                         trait_data_t::get_hero_tree_name( trait->id_sub_tree, dbc.ptr ),
                                         !spec_strs.empty() ? util::string_join( spec_strs, ", " ) : "Generic",
                                         util::string_join( nibbles, ", " ) ) );
    }
    else
    {
      strings.emplace_back( fmt::format( "{} [{}]",
                                         !spec_strs.empty() ? util::string_join( spec_strs, ", " ) : "Generic",
                                         util::string_join( nibbles, ", " ) ) );
    }

    const auto trait_effects = trait_definition_effect_entry_t::find( trait->id_trait_definition, dbc.ptr );

    for ( const auto trait_effect : trait_effects )
    {
      std::vector<std::string> trait_effect_nibbles;

      trait_effect_nibbles.emplace_back( fmt::format(
          "op={}", util::trait_definition_op_string( static_cast<trait_definition_op>( trait_effect.operation ) ) ) );

      auto curve_data = curve_point_t::find( trait_effect.id_curve, dbc.ptr );
      if ( !curve_data.empty() )
      {
        std::vector<std::string> value_strs;
        for ( const auto& point : curve_data )
        {
          value_strs.emplace_back( fmt::format( "{}", point.primary2 ) );
        }

        trait_effect_nibbles.emplace_back( fmt::format( "values=({})", util::string_join( value_strs, ", " ) ) );
      }

      strings.emplace_back( fmt::format( "Effect#{} [{}]", trait_effect.effect_index + 1,
                                         util::string_join( trait_effect_nibbles, ", " ) ) );
    }
  }

  return util::string_join( strings, "\n                 : " );
}

std::string spell_info::to_str( const dbc_t& dbc, const spell_data_t* spell, int level, unsigned wrap )
{
  std::ostringstream s;

  if ( spell->has_scaling_effects() && spell->level() > static_cast<unsigned>( level ) )
  {
    s << std::endl
      << "Too low spell level " << level << " for " << spell->name_cstr() << ", minimum is " << spell->level() << "."
      << std::endl
      << std::endl;
    return s.str();
  }

  const spelltext_data_t& spell_text = dbc.spell_text( spell->id() );
  const spelldesc_vars_data_t& spelldesc_vars = dbc.spell_desc_vars( spell->id() );

  std::string name_str = spell->name_cstr();
  if ( spell_text.rank() )
    name_str += " (desc=" + std::string( spell_text.rank() ) + ")";
  s << "Name             : " << name_str << " (id=" << spell->id() << ") " << spell_flags( spell ) << std::endl;

  const auto hotfixes = spell_data_t::hotfixes( *spell, dbc.ptr );
  if ( !hotfixes.empty() && hotfixes.front().field_id == hotfix::NEW_ENTRY )
  {
    fmt::print( s, "Hotfixed         : NEW SPELL\n" );
  }
  else
  {
    fmt::memory_buffer hs;
    print_hotfixes( hs, hotfixes, _hotfix_spell_map );
    print_hotfixes( hs, spelltext_data_t::hotfixes( spell_text, dbc.ptr ), _hotfix_spelltext_map );
    print_hotfixes( hs, spelldesc_vars_data_t::hotfixes( spelldesc_vars, dbc.ptr ), _hotfix_spelldesc_vars_map );
    if ( hs.size() > 0 )
      fmt::print( s, "Hotfixed         : {}\n", to_string( hs ) );
  }

  const unsigned replace_spell_id = dbc.replace_spell_id( spell->id() );
  if ( replace_spell_id > 0 )
  {
    fmt::print( s, "Replaces         : {} (id={})\n", dbc.spell( replace_spell_id )->name_cstr(), replace_spell_id );
  }

  const auto talents = trait_data_t::find_by_spell( talent_tree::INVALID, spell->id(), 0, SPEC_NONE, dbc.ptr );
  if ( !talents.empty() )
  {
    s << "Talent Entry     : " << trait_data_to_str( dbc, spell, talents ) << std::endl;
  }

  if ( spell->class_mask() )
  {
    std::vector<std::string> class_str;
    std::vector<player_e> exclude;
    std::vector<int> unknown;

    if ( dbc.is_specialization_ability( spell->id() ) )
    {
      std::vector<specialization_e> spec_list;
      dbc.ability_specialization( spell->id(), spec_list );

      for ( const specialization_e spec : spec_list )
      {
        if ( spec == PET_FEROCITY || spec == PET_CUNNING || spec == PET_TENACITY )
        {
          class_str.emplace_back(
            fmt::format( "{} Hunter Pet", util::inverse_tokenize( dbc::specialization_string( spec ) ) ) );

          exclude.emplace_back( player_e::HUNTER );
          continue;
        }

        auto specialization_str = util::specialization_string( spec );
        if ( util::str_compare_ci( specialization_str, "Unknown" ) )
        {
          unknown.emplace_back( static_cast<int>( spec ) );
        }
        else
        {
          class_str.emplace_back( specialization_str );
          exclude.emplace_back( dbc::get_class_from_spec( spec ) );
        }
      }
    }

    for ( size_t i = 1; i < std::size( _class_map ); i++ )
    {
      if ( ( spell->class_mask() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
      {
        if ( unknown.size() )
        {
          for ( auto u : unknown )
            class_str.emplace_back( fmt::format( "Unknown {} ({})", _class_map[ i ].name, u ) );

          unknown.clear();
        }
        else if ( range::contains( exclude, _class_map[ i ].pt ) )
        {
          continue;
        }
        else
        {
          class_str.emplace_back( _class_map[ i ].name );
        }
      }
    }

    s << "Class            : " << wrap_join( class_str, wrap ) << std::endl;
  }

  if ( spell->race_mask() )
  {
    std::vector<std::string_view> races;
    for ( unsigned int i = 0; i < sizeof( spell->race_mask() ) * 8; i++ )
    {
      uint64_t mask = uint64_t( 1 ) << i;
      if ( spell->race_mask() & mask )
      {
        auto it = _race_map.find( i );
        if ( it != _race_map.end() )
          races.push_back( it->second );
      }
    }

    fmt::print( s, "Race             : {} (0x{:0x})\n", fmt::join( races, ", " ), spell->race_mask() );
  }

  const auto& covenant_spell = covenant_ability_entry_t::find( spell->name_cstr(), dbc.ptr );
  if ( covenant_spell.spell_id == spell->id() )
  {
    s << "Covenant         : ";
    s << util::inverse_tokenize( util::covenant_type_string( static_cast<covenant_e>( covenant_spell.covenant_id ) ) );
    s << std::endl;
  }

  const auto& soulbind_spell = soulbind_ability_entry_t::find( spell->id(), dbc.ptr );
  if ( soulbind_spell.spell_id == spell->id() )
  {
    s << "Covenant         : ";
    s << util::inverse_tokenize( util::covenant_type_string( static_cast<covenant_e>( soulbind_spell.covenant_id ) ) );
    s << std::endl;
  }
  std::string school_string = util::school_type_string( spell->get_school_type() );
  school_string[ 0 ] = std::toupper( school_string[ 0 ] );
  s << "School           : " << school_string << std::endl;

  std::string spell_type_str = "Unknown(" + util::to_string( spell->dmg_class() ) + ")";
  if ( spell->dmg_class() < _spell_type_map.size() )
  {
    spell_type_str = _spell_type_map[ spell->dmg_class() ];
  }
  s << "Spell Type       : " << spell_type_str << std::endl;

  for ( const spellpower_data_t& pd : spell->powers() )
  {
    s << "Resource         : ";

    if ( pd.type() == POWER_MANA && pd._cost == 0 )
      s << pd.cost() * 100.0 << "%";
    else
      s << pd.cost();

    s << " ";

    if ( pd.max_cost() != 0 )
    {
      s << "- ";
      if ( pd.type() == POWER_MANA && pd._cost_max == 0 )
        s << ( pd.cost() + pd.max_cost() ) * 100.0 << "%";
      else
        s << ( pd.cost() + pd.max_cost() );
      s << " ";
    }

    s << map_string( _resource_strings, pd.raw_type() );

    if ( pd.cost_per_tick() != 0 )
    {
      s << " and ";

      if ( pd.type() == POWER_MANA )
        s << pd.cost_per_tick() * 100.0 << "%";
      else
        s << pd.cost_per_tick();

      s << " " << map_string( _resource_strings, pd.raw_type() ) << " per tick";
    }

    s << " (id=" << pd.id() << ")";

    if ( pd.aura_id() > 0 && dbc.spell( pd.aura_id() )->id() == pd.aura_id() )
      s << " w/ " << dbc.spell( pd.aura_id() )->name_cstr() << " (id=" << pd.aura_id() << ")";

    if ( const auto power_hotfixes = spellpower_data_t::hotfixes( pd, dbc.ptr ); !power_hotfixes.empty() )
    {
      if ( power_hotfixes.front().field_id == hotfix::NEW_ENTRY )
        fmt::print( s, "[Hotfixed: NEW POWER]" );
      else
        fmt::print( s, "[Hotfixed: {}]", hotfix_map_str( power_hotfixes, _hotfix_power_map ) );
    }

    s << std::endl;
  }

  if ( spell->level() > 0 )
  {
    s << "Spell Level      : " << spell->level();
    if ( spell->max_level() > 0 )
      s << " (max " << spell->max_level() << ")";

    s << std::endl;
  }

  if ( spell->max_scaling_level() > 0 )
  {
    s << "Max Scaling Level: " << spell->max_scaling_level();
    s << std::endl;
  }

  if ( spell->min_scaling_level() > 0 )
  {
    s << "Min Scaling Level: " << spell->min_scaling_level();
    s << std::endl;
  }

  if ( spell->scale_from_ilevel() )
  {
    s << "Scale from ilevel: " << spell->scale_from_ilevel();
    s << std::endl;
  }

  if ( spell->max_aura_level() > 0 )
  {
    s << "Max Aura Level   : " << spell->max_aura_level();
    s << std::endl;
  }

  if ( spell->min_range() || spell->max_range() )
  {
    s << "Range            : ";
    if ( spell->min_range() )
      s << (int)spell->min_range() << " - ";

    s << (int)spell->max_range() << " yards" << std::endl;
  }

  if ( spell->max_targets() != 0 )
  {
    fmt::print( s, "Max Targets      : {}{}{}\n",
                spell->max_targets() == -1 ? "Unlimited("
                : spell->max_targets() < 0 ? "Unknown("
                                           : "",
                spell->max_targets(), spell->max_targets() < 0 ? ")" : "" );
  }

  if ( spell->cast_time() > 0_ms )
    s << "Cast Time        : " << spell->cast_time().total_seconds() << " seconds" << std::endl;
  else if ( spell->cast_time() < 0_ms )
    s << "Cast Time        : Ranged Shot" << std::endl;

  if ( spell->gcd() != timespan_t::zero() )
    s << "GCD              : " << spell->gcd().total_seconds() << " seconds" << std::endl;

  if ( spell->missile_speed() )
  {
    if ( spell->flags( spell_attribute::SX_FIXED_TRAVEL_TIME ) )
      s << "Travel Time      : " << spell->missile_speed() << " seconds" << std::endl;
    else
      s << "Velocity         : " << spell->missile_speed() << " yards/sec" << std::endl;
  }

  if ( spell->missile_delay() )
    s << "Travel Delay     : " << spell->missile_delay() << " seconds" << std::endl;

  if ( spell->missile_min_duration() )
    s << "Min Travel Time  : " << spell->missile_min_duration() << " seconds" << std::endl;

  if ( spell->duration() != timespan_t::zero() )
  {
    s << "Duration         : ";
    if ( spell->duration() < timespan_t::zero() )
      s << "Aura (infinite)";
    else
      s << spell->duration().total_seconds() << " seconds";

    s << std::endl;
  }

  if ( spell->equipped_class() == ITEM_CLASS_WEAPON )
  {
    std::vector<std::string> weapon_types;
    for ( auto wt = ITEM_SUBCLASS_WEAPON_AXE; wt < ITEM_SUBCLASS_WEAPON_FISHING_POLE; ++wt )
    {
      if ( spell->equipped_subclass_mask() & ( 1U << static_cast<unsigned>( wt ) ) )
      {
        weapon_types.emplace_back( util::weapon_subclass_string( wt ) );
      }
    }

    for ( auto it = INVTYPE_HEAD; it < INVTYPE_MAX; ++it )
    {
      if ( spell->equipped_invtype_mask() & ( 1U << static_cast<unsigned>( it ) ) )
      {
        weapon_types.emplace_back( util::weapon_class_string( it ) );
      }
    }

    if ( !weapon_types.empty() )
      s << "Requires weapon  : " << wrap_join( weapon_types, wrap ) << std::endl;
  }
  else if ( spell->equipped_class() == ITEM_CLASS_ARMOR )
  {
    std::vector<std::string> armor_types, armor_invtypes;
    if ( spell->equipped_subclass_mask() == 0x1f )
    {
      armor_types.emplace_back( "Any" );
    }
    else
    {
      for ( auto at = ITEM_SUBCLASS_ARMOR_MISC; at < ITEM_SUBCLASS_ARMOR_RELIC; ++at )
      {
        if ( spell->equipped_subclass_mask() & ( 1U << static_cast<unsigned>( at ) ) )
        {
          armor_types.emplace_back( util::armor_subclass_string( at ) );
        }
      }
    }

    for ( auto it = INVTYPE_HEAD; it < INVTYPE_MAX; ++it )
    {
      if ( spell->equipped_invtype_mask() & ( 1U << static_cast<unsigned>( it ) ) )
      {
        armor_invtypes.emplace_back( util::invtype_string( it ) );
      }
    }

    if ( !armor_types.empty() || !armor_invtypes.empty() )
    {
      s << "Requires armor   : ";

      if ( !armor_types.empty() )
        s << util::string_join( armor_types ) << " ";

      s << util::string_join( armor_invtypes ) << std::endl;
    }
  }

  if ( spell->cooldown() > timespan_t::zero() )
    s << "Cooldown         : " << spell->cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell->charges() > 0 || spell->charge_cooldown() > timespan_t::zero() )
  {
    s << "Charges          : " << spell->charges();
    if ( spell->charge_cooldown() > timespan_t::zero() )
      s << " (" << spell->charge_cooldown().total_seconds() << " seconds cooldown)";
    s << std::endl;
  }

  if ( spell->category() > 0 )
  {
    s << "Category         : ";
    std::string category_str = fmt::format( "{} (Type {:#x})", spell->category(), spell->category_type() );
    auto affecting_effects = dbc.effect_categories_affecting_spell( spell );
    if ( affecting_effects.empty() )
    {
      s << category_str;
    }
    else
    {
      s << wrap_concatenate( affecting_effects,
        [ category_str, first = affecting_effects.front() ]( const spelleffect_data_t* e ) {
          if ( e == first )
          {
            return fmt::format( "{}: {} ({} effect#{})", category_str, e->spell()->name_cstr(), e->spell()->id(),
                                e->index() + 1 );
          }
          else
          {
            return fmt::format( "{} ({} effect#{})", e->spell()->name_cstr(), e->spell()->id(), e->index() + 1 );
          }
        },
        wrap );
    }
    s << std::endl;

    if ( spell->category_flags() )
    {
      std::vector<std::string> category_flags_str;
      for ( unsigned flag = 0; flag < 32; flag++ )
      {
        if ( spell->category_flags() & ( 1 << flag ) )
        {
          auto it = _category_flag_strings.find( flag );
          category_flags_str.emplace_back(
            fmt::format( "{} ({})", it == _category_flag_strings.end() ? "Unknown" : it->second, flag ) );
        }
      }
      s << "Category Flags   : " << wrap_join( category_flags_str, wrap ) << std::endl;
    }
  }

  bool first_label = true;
  for ( size_t i = 1, end = spell->label_count(); i <= end; ++i )
  {
    auto label = spell->labelN( i );
    auto affecting_effects = dbc.effect_labels_affecting_label( label );

    if ( !first_label )
    {
      s << "                 : ";
    }
    else
    {
      first_label = false;
      s << "Labels           : ";
    }

    s << label;

    auto it = _label_strings.find( label );
    if ( it != _label_strings.end() )
    {
      s << ": " << it->second;
    }
    else if ( !affecting_effects.empty() )
    {
      s << ": " << wrap_concatenate( affecting_effects, []( const spelleffect_data_t* e ) {
        return fmt::format( "{} ({} effect#{})", e->spell()->name_cstr(), e->spell()->id(), e->index() + 1 );
      }, wrap );
    }

    s << std::endl;
  }

  if ( spell->category_cooldown() > timespan_t::zero() )
    s << "Category Cooldown: " << spell->category_cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell->internal_cooldown() > timespan_t::zero() )
    s << "Internal Cooldown: " << spell->internal_cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell->initial_stacks() > 0 || spell->max_stacks() )
  {
    s << "Stacks           : ";
    if ( spell->initial_stacks() )
      s << spell->initial_stacks() << " initial, ";

    if ( spell->max_stacks() )
      s << spell->max_stacks() << " maximum, ";
    else if ( spell->initial_stacks() && !spell->max_stacks() )
      s << spell->initial_stacks() << " maximum, ";

    s.seekp( -2, std::ios_base::cur );

    s << std::endl;
  }

  if ( spell->proc_chance() > 0 )
    s << "Proc Chance      : " << spell->proc_chance() * 100 << "%" << std::endl;

  if ( spell->real_ppm() != 0 )
  {
    s << "Real PPM         : " << spell->real_ppm();
    auto mod_span = rppm_modifier_t::find( spell->id(), dbc.ptr );

    std::vector<rppm_modifier_t> modifiers( mod_span.begin(), mod_span.end() );
    range::sort( modifiers, []( rppm_modifier_t a, rppm_modifier_t b ) {
      if ( ( a.modifier_type == RPPM_MODIFIER_CLASS && b.modifier_type == RPPM_MODIFIER_CLASS ) ||
           ( a.modifier_type == RPPM_MODIFIER_SPEC && b.modifier_type == RPPM_MODIFIER_SPEC ) )
      {
        return a.type < b.type;
      }

      return a.modifier_type < b.modifier_type;
    } );

    std::vector<std::string> mods;
    for ( const auto& modifier : modifiers )
    {
      switch ( modifier.modifier_type )
      {
        case RPPM_MODIFIER_HASTE:
          mods.emplace_back( "Haste multiplier" );
          break;
        case RPPM_MODIFIER_CRIT:
          mods.emplace_back( "Crit multiplier" );
          break;
        case RPPM_MODIFIER_ILEVEL:
          mods.emplace_back(
              fmt::format( "Itemlevel multiplier [base={}, coeff={}]", modifier.type, modifier.coefficient ) );
          break;
        case RPPM_MODIFIER_CLASS:
        {
          std::vector<std::string> class_str;
          for ( player_e p = PLAYER_NONE; p < PLAYER_MAX; ++p )
          {
            if ( util::class_id_mask( p ) & modifier.type )
            {
              class_str.emplace_back( util::inverse_tokenize( util::player_type_string( p ) ) );
            }
          }

          s.precision( real_ppm_decimals( spell, modifier ) );
          mods.emplace_back( fmt::format( "{}: {}", util::string_join( class_str ),
                                          ( spell->real_ppm() * ( 1.0 + modifier.coefficient ) ) ) );
          break;
        }
        case RPPM_MODIFIER_SPEC:
        {
          s.precision( real_ppm_decimals( spell, modifier ) );
          mods.emplace_back( fmt::format( "{}: {}",
                                          util::specialization_string( static_cast<specialization_e>( modifier.type ) ),
                                          ( spell->real_ppm() * ( 1.0 + modifier.coefficient ) ) ) );
          break;
        }
        case RPPM_MODIFIER_RACE:
        {
          std::vector<std::string> race_str;
          for ( race_e r = RACE_NONE; r < RACE_MAX; ++r )
          {
            if ( util::race_mask( r ) & modifier.type )
            {
              race_str.emplace_back( util::inverse_tokenize( util::race_type_string( r ) ) );
            }
          }

          s.precision( real_ppm_decimals( spell, modifier ) );
          mods.emplace_back( fmt::format( "{}: {}", util::string_join( race_str ),
                                          ( spell->real_ppm() * ( 1.0 + modifier.coefficient ) ) ) );
          break;
        }
        case RPPM_MODIFIER_AURA:
        {
          s.precision( real_ppm_decimals( spell, modifier ) );
          mods.emplace_back( fmt::format( "/w {} (id={}): {}", dbc.spell( modifier.type )->name_cstr(), modifier.type,
                                          spell->real_ppm() * ( 1.0 + modifier.coefficient ) ) );
          break;
        }
        default:
          break;
      }
    }

    if ( !mods.empty() )
    {
      s << " (" << util::string_join( mods ) << ")";
    }
    s << std::endl;
  }

  if ( spell->stance_mask() > 0 )
  {
    fmt::print( s, "Stance Mask      : 0x{:08x}\n", spell->stance_mask() );
  }

  if ( spell->mechanic() > 0 )
  {
    s << "Mechanic         : " << mechanic_str( spell->mechanic() ) << std::endl;
  }

  if ( spell->power_id() > 0 )
  {
    s << "Azerite Power Id : " << spell->power_id() << std::endl;
  }

  if ( spell->essence_id() > 0 )
  {
    s << "Azerite EssenceId: " << spell->essence_id() << " ";

    const auto data = azerite_essence_power_entry_t::data_by_essence_id( spell->essence_id(), dbc.ptr );

    s << azerite_essence_str( spell, data );
    s << std::endl;
  }

  const auto& conduit = conduit_entry_t::find_by_spellid( spell->id(), dbc.ptr );
  if ( conduit.spell_id && conduit.spell_id == spell->id() )
  {
    s << "Conduit Id       : " << conduit.id;

    auto ranks = conduit_rank_entry_t::find( conduit.id, dbc.ptr );
    std::vector<std::string> rank_str;
    range::for_each( ranks, [ &rank_str ]( const conduit_rank_entry_t& entry ) {
      rank_str.emplace_back( fmt::format( "{}", entry.value ) );
    } );

    if ( !ranks.empty() )
    {
      fmt::print( s, " (values={})", fmt::join( rank_str, ", " ) );
    }

    s << std::endl;
  }

  if ( spell->proc_flags() > 0 )
  {
    std::vector<std::string_view> proc_str;
    for ( const auto& info : _proc_flag_map )
      if ( spell->proc_flags() & info.flag )
        proc_str.emplace_back( info.proc );

    fmt::print( s, "Proc Flags       : {}\n", fmt::join( proc_str, ", " ) );
  }

  if ( spell->class_family() > 0 )
  {
    auto affecting_effects = dbc.effects_affecting_spell( spell );
    if ( !affecting_effects.empty() )
    {
      const auto spell_string = []( util::span<const spelleffect_data_t* const> effects ) {
        const spell_data_t* spell = effects.front()->spell();
        if ( effects.size() == 1 )
          return fmt::format( "{} ({} effect#{})", spell->name_cstr(), spell->id(), effects.front()->index() + 1 );

        fmt::memory_buffer s;
        fmt::format_to( std::back_inserter( s ), "{} ({} effects: ", spell->name_cstr(), spell->id() );
        for ( size_t i = 0; i < effects.size(); i++ )
          fmt::format_to( std::back_inserter( s ), "{}#{}", i == 0 ? "" : ", ", effects[ i ]->index() + 1 );
        fmt::format_to( std::back_inserter( s ), ")" );
        return to_string( s );
      };

      range::sort( affecting_effects, []( const spelleffect_data_t* lhs, const spelleffect_data_t* rhs ) {
        return std::make_tuple( lhs->spell_id(), lhs->index() ) < std::make_tuple( rhs->spell_id(), rhs->index() );
      } );

      std::vector<std::string> spell_strings;
      auto effects = util::make_span( affecting_effects );
      while ( !effects.empty() )
      {
        size_t count = 1;
        const unsigned spell_id = effects.front()->spell_id();
        while ( count < effects.size() && effects[ count ]->spell_id() == spell_id )
          count++;
        spell_strings.push_back( spell_string( effects.first( count ) ) );
        effects = effects.subspan( count );
      }

      s << "Affecting Spells : " << wrap_join( spell_strings, wrap ) << std::endl;
    }
  }

  if ( spell->driver_count() > 0 )
  {
    s << "Triggered By     : ";
    s << wrap_concatenate( spell->drivers(), []( const spell_data_t* spell ) {
      return fmt::format( "{} ({})", spell->name_cstr(), spell->id() );
    }, wrap );
    s << std::endl;
  }

  if ( spell->class_family() > 0 )
  {
    std::vector<unsigned> flags;
    for ( size_t i = 0; i < NUM_CLASS_FAMILY_FLAGS; ++i )
    {
      for ( size_t bit = 0; bit < 32; ++bit )
      {
        if ( ( 1 << bit ) & spell->_class_flags[ i ] )
          flags.push_back( static_cast<unsigned>( i * 32 + bit ) );
      }
    }

    if ( !flags.empty() )
      fmt::print( s, "Family Flags     : {}\n", fmt::join( flags, ", " ) );
  }

  std::vector<std::string> attr_str;
  for ( unsigned i = 0; i < NUM_SPELL_FLAGS; i++ )
  {
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( spell->attribute( i ) & ( 1 << flag ) )
      {
        size_t attr_idx = i * 32 + flag;
        auto it = _attribute_strings.find( static_cast<unsigned int>( attr_idx ) );
        attr_str.emplace_back(
          fmt::format( "{} ({})", it == _attribute_strings.end() ? "Unknown" : it->second, attr_idx ) );
      }
    }
  }
  if ( !attr_str.empty() )
    s << "Attributes       : " << wrap_join( attr_str, wrap ) << std::endl;

  std::vector<std::string> aura_int_str;
  for ( unsigned flag = 0; flag < 64; flag++ )
  {
    auto byte = static_cast<unsigned>( flag / 32 );
    auto bit = flag % 32;
    if ( spell->_aura_interrupt[ byte ] & ( 1 << bit ) )
    {
      auto it = _aura_interrupt_strings.find( flag + 1 );
      aura_int_str.emplace_back(
        fmt::format( "{} ({})", it == _aura_interrupt_strings.end() ? "Unknown" : it->second, flag ) );
    }
  }
  if ( !aura_int_str.empty() )
    s << "Aura Interrupt   : " << wrap_join( aura_int_str, wrap ) << std::endl;

  std::vector<std::string> channel_int_str;
  for ( unsigned flag = 0; flag < 64; flag++ )
  {
    auto byte = static_cast<unsigned>( flag / 32 );
    auto bit = flag % 32;
    if ( spell->_channel_interrupt[ byte ] & ( 1 << bit ) )
    {
      auto it = _aura_interrupt_strings.find( flag );
      channel_int_str.emplace_back(
        fmt::format( "{} ({})", it == _aura_interrupt_strings.end() ? "Unknown" : it->second, flag ) );
    }
  }
  if ( !channel_int_str.empty() )
    s << "Channel Interrupt: " << wrap_join( channel_int_str, wrap ) << std::endl;

  s << "Effects          :" << std::endl;
  for ( const spelleffect_data_t& e : spell->effects() )
  {
    if ( e.id() == 0 )
      continue;

    spell_info::effect_to_str( dbc, spell, &e, s, level, wrap );
  }

  if ( spell_text.desc() )
    s << "Description      : " << spell_text.desc() << std::endl;

  if ( spell_text.tooltip() )
    s << "Tooltip          : " << spell_text.tooltip() << std::endl;

  if ( spelldesc_vars.desc_vars() )
    s << "Variables        : " << spelldesc_vars.desc_vars() << std::endl;

  s << std::endl;

  return s.str();
}

std::string spell_info::talent_to_str( const dbc_t& /* dbc */, const trait_data_t* talent, int /* level */ )
{
  std::ostringstream s;

  s << "Name         : " << talent->name << std::endl;
  s << "Entry        : " << talent->id_trait_node_entry << std::endl;
  s << "Node         : " << talent->id_node << std::endl;
  s << "Definition   : " << talent->id_trait_definition << std::endl;
  s << "Tree         : " << util::talent_tree_string( static_cast<talent_tree>( talent->tree_index ) ) << std::endl;
  s << "Class        : " << util::player_type_string( util::translate_class_id( talent->id_class ) ) << std::endl;
  s << "Column       : " << talent->col << std::endl;
  s << "Row          : " << talent->row << std::endl;
  s << "Max Rank     : " << talent->max_ranks << std::endl;
  s << "Spell        : " << talent->id_spell << std::endl;
  if ( talent->id_replace_spell > 0 )
  {
    s << "Replaces     : " << talent->id_replace_spell << std::endl;
  }
  if ( talent->id_override_spell > 0 )
  {
    s << "Overriden by : " << talent->id_override_spell << std::endl;
  }
  s << "Subtree      : " << talent->id_sub_tree << std::endl;
  // s << "Spec         : " << util::specialization_string( talent -> specialization() ) << std::endl;
  s << std::endl;

  return s.str();
}

std::string spell_info::set_bonus_to_str( const dbc_t&, const item_set_bonus_t* set_bonus, int /* level */ )
{
  std::ostringstream s;

  s << "Name          : " << set_bonus->set_name << std::endl;

  auto player_type = static_cast<player_e>( set_bonus->class_id );
  s << "Class         : " << util::player_type_string( player_type ) << std::endl;
  s << "Tier          : " << set_bonus->tier << std::endl;
  s << "Bonus Level   : " << set_bonus->bonus << std::endl;
  if ( set_bonus->spec > 0 )
    s << "Spec          : " << util::specialization_string( static_cast<specialization_e>( set_bonus->spec ) )
      << std::endl;
  s << "Spell ID      : " << set_bonus->spell_id << std::endl;

  s << std::endl;

  return s.str();
}

void spell_info::effect_to_xml( const dbc_t& dbc, const spell_data_t* spell, const spelleffect_data_t* e,
                                xml_node_t* parent, int level )
{
  xml_node_t* node = parent->add_child( "effect" );

  node->add_parm( "number", e->index() + 1 );
  node->add_parm( "id", e->id() );
  node->add_parm( "type", static_cast<int>( e->type() ) );

  if ( _effect_type_strings.contains( e->raw_type() ) )
  {
    node->add_parm( "type_text", map_string( _effect_type_strings, e->raw_type() ) );
  }

  // Put some nice handling on some effect types
  switch ( e->type() )
  {
    case E_SCHOOL_DAMAGE:
      node->add_parm( "school", spell->get_school_type() );
      node->add_parm( "school_text", util::school_type_string( spell->get_school_type() ) );
      break;
    case E_TRIGGER_SPELL:
    case E_TRIGGER_SPELL_WITH_VALUE:
      if ( e->trigger_spell_id() )
      {
        if ( dbc.spell( e->trigger_spell_id() ) != spell_data_t::nil() )
          node->add_parm( "trigger_spell_name", dbc.spell( e->trigger_spell_id() )->name_cstr() );
      }
      break;
    default:
      break;
  }

  node->add_parm( "sub_type", static_cast<int>( e->subtype() ) );

  if ( e->subtype() > 0 )
  {
    node->add_parm( "sub_type_text", map_string( _effect_subtype_strings, e->raw_subtype() ) );

    switch ( e->subtype() )
    {
      case A_PERIODIC_DAMAGE:
        node->add_parm( "school", spell->get_school_type() );
        node->add_parm( "school_text", util::school_type_string( spell->get_school_type() ) );
        if ( e->period() != timespan_t::zero() )
          node->add_parm( "period", e->period().total_seconds() );
        break;
      case A_PERIODIC_ENERGIZE:
      case A_PERIODIC_DUMMY:
        if ( e->period() != timespan_t::zero() )
          node->add_parm( "period", e->period().total_seconds() );
        break;
      case A_PROC_TRIGGER_SPELL:
        if ( e->trigger_spell_id() )
        {
          if ( dbc.spell( e->trigger_spell_id() ) != spell_data_t::nil() )
            node->add_parm( "trigger_spell_name", dbc.spell( e->trigger_spell_id() )->name_cstr() );
        }
        break;
      case A_PERIODIC_TRIGGER_SPELL:
        if ( e->trigger_spell_id() )
        {
          if ( dbc.spell( e->trigger_spell_id() ) != spell_data_t::nil() )
          {
            node->add_parm( "trigger_spell_name", dbc.spell( e->trigger_spell_id() )->name_cstr() );
            if ( e->period() != timespan_t::zero() )
              node->add_parm( "period", e->period().total_seconds() );
          }
        }
        break;
      case A_ADD_FLAT_MODIFIER:
      case A_ADD_PCT_MODIFIER:
        node->add_parm( "modifier", e->misc_value1() );
        if ( _property_type_strings.contains( e->misc_value1() ) )
        {
          node->add_parm( "modifier_text", map_string( _property_type_strings, e->misc_value1() ) );
        }
        break;
      default:
        break;
    }
  }
  node->add_parm( "base_value", e->base_value() );

  if ( level <= MAX_LEVEL )
  {
    double v_min = dbc.effect_min( e->id(), level );
    double v_max = dbc.effect_max( e->id(), level );
    node->add_parm( "scaled_value", v_min );
    if ( v_min != v_max )
    {
      node->add_parm( "scaled_value_max", v_max );
    }
  }
  else
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( level );
    double item_budget = ilevel_data.p_epic[ 0 ];

    node->add_parm( "scaled_value", item_budget * e->m_coefficient() );
  }

  if ( e->m_coefficient() != 0 )
  {
    node->add_parm( "multiplier_coefficient", e->m_coefficient() );
  }

  if ( e->m_delta() != 0 )
  {
    node->add_parm( "multiplier_delta", e->m_delta() );
  }

  if ( level <= MAX_LEVEL )
  {
    if ( e->m_unk() )
    {
      node->add_parm( "bonus_value", dbc.effect_bonus( e->id(), level ) );
      node->add_parm( "bonus_value_multiplier", e->m_unk() );
    }
  }

  if ( e->real_ppl() != 0 )
  {
    node->add_parm( "points_per_level", e->real_ppl() );
  }

  if ( e->sp_coeff() != 0 )
  {
    node->add_parm( "sp_coefficient", e->sp_coeff() );
  }

  if ( e->ap_coeff() != 0 )
  {
    node->add_parm( "ap_coefficient", e->ap_coeff() );
  }

  if ( e->chain_multiplier() != 0 && e->chain_multiplier() != 1.0 )
    node->add_parm( "chain_multiplier", e->chain_multiplier() );

  if ( e->misc_value1() != 0 || e->type() == E_ENERGIZE || e->type() == E_ENERGIZE_PCT )
  {
    if ( e->subtype() == A_MOD_DAMAGE_DONE || e->subtype() == A_MOD_DAMAGE_TAKEN ||
         e->subtype() == A_MOD_DAMAGE_PERCENT_DONE || e->subtype() == A_MOD_DAMAGE_PERCENT_TAKEN )
      node->add_parm( "misc_value_mod_damage", e->misc_value1() );
    else if ( e->type() == E_ENERGIZE || e->type() == E_ENERGIZE_PCT )
      node->add_parm(
          "misc_value_energize",
          util::resource_type_string( util::translate_power_type( static_cast<power_e>( e->misc_value1() ) ) ) );
    else
      node->add_parm( "misc_value", e->misc_value1() );
  }

  if ( e->misc_value2() != 0 )
  {
    node->add_parm( "misc_value_2", e->misc_value2() );
  }

  if ( e->pp_combo_points() != 0 )
    node->add_parm( "points_per_combo_point", e->pp_combo_points() );

  if ( e->trigger_spell_id() != 0 )
    node->add_parm( "trigger_spell_id", e->trigger_spell_id() );
}

void spell_info::to_xml( const dbc_t& dbc, const spell_data_t* spell, xml_node_t* parent, int level )
{
  player_e pt = PLAYER_NONE;

  if ( spell->has_scaling_effects() && spell->level() > static_cast<unsigned>( level ) )
  {
    return;
  }

  xml_node_t* node = parent->add_child( "spell" );

  node->add_parm( "id", spell->id() );
  node->add_parm( "name", spell->name_cstr() );
  spell_flags_xml( spell, node );

  unsigned replace_spell_id = dbc.replace_spell_id( spell->id() );
  if ( replace_spell_id > 0 )
  {
    node->add_parm( "replaces_name", dbc.spell( replace_spell_id )->name_cstr() );
    node->add_parm( "replaces_id", replace_spell_id );
  }

  if ( spell->class_mask() )
  {
    bool pet_ability = false;

    if ( dbc.is_specialization_ability( spell->id() ) )
    {
      std::vector<specialization_e> spec_list;
      std::vector<specialization_e>::iterator iter;
      dbc.ability_specialization( spell->id(), spec_list );

      for ( iter = spec_list.begin(); iter != spec_list.end(); ++iter )
      {
        xml_node_t* spec_node = node->add_child( "spec" );
        spec_node->add_parm( "id", *iter );
        spec_node->add_parm( "name", dbc::specialization_string( *iter ) );
        if ( *iter == PET_FEROCITY || *iter == PET_CUNNING || *iter == PET_TENACITY )
        {
          pet_ability = true;
        }
      }
      spec_list.clear();
    }

    for ( unsigned int i = 1; i < std::size( _class_map ); i++ )
    {
      if ( ( spell->class_mask() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
      {
        xml_node_t* class_node = node->add_child( "class" );
        class_node->add_parm( "id", _class_map[ i ].pt );
        class_node->add_parm( "name", _class_map[ i ].name );
        if ( !pt )
          pt = _class_map[ i ].pt;
      }
    }

    if ( pet_ability )
      node->add_child( "class" )->add_parm( ".", "Pet" );
  }

  if ( spell->race_mask() )
  {
    for ( unsigned int i = 0; i < sizeof( spell->race_mask() ) * 8; i++ )
    {
      uint64_t mask = ( uint64_t( 1 ) << i );
      if ( ( spell->race_mask() & mask ) )
      {
        auto it = _race_map.find( i );
        if ( it != _race_map.end() )
        {
          xml_node_t* race_node = node->add_child( "race" );
          race_node->add_parm( "id", i );
          race_node->add_parm( "name", it->second );
        }
      }
    }
  }

  for ( const spellpower_data_t& pd : spell->powers() )
  {
    if ( pd.cost() == 0 )
      continue;

    xml_node_t* resource_node = node->add_child( "resource" );
    resource_node->add_parm( "type", (signed)pd.type() );

    if ( pd.type() == POWER_MANA )
      resource_node->add_parm( "cost", spell->cost( pd.type() ) * 100.0 );
    else
      resource_node->add_parm( "cost", spell->cost( pd.type() ) );

    if ( _resource_strings.contains( pd.raw_type() ) )
    {
      resource_node->add_parm( "type_name", map_string( _resource_strings, pd.raw_type() ) );
    }

    if ( pd.type() == POWER_MANA )
    {
      resource_node->add_parm( "cost_mana_flat", floor( dbc.resource_base( pt, level ) * pd.cost() ) );
      resource_node->add_parm( "cost_mana_flat_level", level );
    }

    if ( pd.aura_id() > 0 && dbc.spell( pd.aura_id() )->id() == pd.aura_id() )
    {
      resource_node->add_parm( "cost_aura_id", pd.aura_id() );
      resource_node->add_parm( "cost_aura_name", dbc.spell( pd.aura_id() )->name_cstr() );
    }
  }

  if ( spell->level() > 0 )
  {
    node->add_parm( "level", spell->level() );
    if ( spell->max_level() > 0 )
      node->add_parm( "max_level", spell->max_level() );
  }

  if ( spell->min_range() || spell->max_range() )
  {
    if ( spell->min_range() )
      node->add_parm( "range_min", spell->min_range() );
    node->add_parm( "range", spell->max_range() );
  }

  if ( spell->cast_time() > 0_ms )
    node->add_parm( "cast_time_else", spell->cast_time().total_seconds() );
  else if ( spell->cast_time() < 0_ms )
    node->add_parm( "cast_time_range", "ranged_shot" );

  if ( spell->gcd() != timespan_t::zero() )
    node->add_parm( "gcd", spell->gcd().total_seconds() );

  if ( spell->missile_speed() )
    node->add_parm( "velocity", spell->missile_speed() );

  if ( spell->duration() != timespan_t::zero() )
  {
    if ( spell->duration() < timespan_t::zero() )
      node->add_parm( "duration", "-1" );
    else
      node->add_parm( "duration", spell->duration().total_seconds() );
  }

  if ( spell->cooldown() > timespan_t::zero() )
    node->add_parm( "cooldown", spell->cooldown().total_seconds() );

  if ( spell->initial_stacks() > 0 || spell->max_stacks() )
  {
    if ( spell->initial_stacks() )
      node->add_parm( "stacks_initial", spell->initial_stacks() );

    if ( spell->max_stacks() )
      node->add_parm( "stacks_max", spell->max_stacks() );
    else if ( spell->initial_stacks() && !spell->max_stacks() )
      node->add_parm( "stacks_max", spell->initial_stacks() );
  }

  if ( spell->proc_chance() > 0 )
    node->add_parm( "proc_chance", spell->proc_chance() * 100 );  // NP 101 % displayed

  if ( spell->extra_coeff() > 0 )
    node->add_parm( "extra_coefficient", spell->extra_coeff() );

  std::string attribs;
  for ( unsigned int _attribute : spell->_attributes )
  {
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( _attribute & ( 1 << flag ) )
        attribs += "1";
      else
        attribs += "0";
    }
  }
  node->add_child( "attributes" )->add_parm( ".", attribs );

  xml_node_t* effect_node = node->add_child( "effects" );
  effect_node->add_parm( "count", spell->effect_count() );

  for ( const spelleffect_data_t& e : spell->effects() )
  {
    if ( e.id() == 0 )
      continue;

    spell_info::effect_to_xml( dbc, spell, &e, effect_node, level );
  }

  const auto& spell_text = dbc.spell_text( spell->id() );
  if ( spell_text.desc() )
    node->add_child( "description" )->add_parm( ".", spell_text.desc() );

  if ( spell_text.tooltip() )
    node->add_child( "tooltip" )->add_parm( ".", spell_text.tooltip() );

  const auto& spelldesc_vars = dbc.spell_desc_vars( spell->id() );
  if ( spelldesc_vars.desc_vars() )
    node->add_child( "variables" )->add_parm( ".", spelldesc_vars.desc_vars() );
}

void spell_info::talent_to_xml( const dbc_t& /* dbc */, const trait_data_t* talent, xml_node_t* parent,
                                int /* level */ )
{
  xml_node_t* node = parent->add_child( "talent" );

  node->add_parm( "name", talent->name );
  node->add_parm( "id", talent->id_trait_node_entry );
  node->add_parm( "tree", util::talent_tree_string( static_cast<talent_tree>( talent->tree_index ) ) );
  node->add_child( "class" )->add_parm( ".", util::player_type_string( util::translate_class_id( talent->id_class ) ) );

  node->add_parm( "column", talent->col );
  node->add_parm( "row", talent->row );
  node->add_parm( "max_rank", talent->max_ranks );
  node->add_parm( "spell", talent->id_spell );
  if ( talent->id_replace_spell > 0 )
  {
    node->add_parm( "replaces", talent->id_replace_spell );
  }
  if ( talent->id_override_spell > 0 )
  {
    node->add_parm( "overridden", talent->id_override_spell );
  }
}

void spell_info::set_bonus_to_xml( const dbc_t& /* dbc */, const item_set_bonus_t* set_bonus, xml_node_t* parent,
                                   int /* level */ )
{
  xml_node_t* node = parent->add_child( "set_bonus" );

  auto player_type = static_cast<player_e>( set_bonus->class_id );
  node->add_parm( "name", set_bonus->set_name );
  node->add_parm( "class", util::player_type_string( player_type ) );
  node->add_parm( "tier", set_bonus->tier );
  node->add_parm( "bonus_level", set_bonus->bonus );
  if ( set_bonus->spec > 0 )
  {
    node->add_parm( "spec", util::specialization_string( static_cast<specialization_e>( set_bonus->spec ) ) );
  }
  node->add_parm( "spell_id", set_bonus->spell_id );
}

std::string_view spell_info::effect_type_str( const spelleffect_data_t* effect )
{
  auto it = _effect_type_strings.find( effect->type() );
  if ( it != _effect_type_strings.end() )
    return it->second;
  return {};
}

std::string_view spell_info::effect_subtype_str( const spelleffect_data_t* effect )
{
  auto it = _effect_subtype_strings.find( effect->subtype() );
  if ( it != _effect_subtype_strings.end() )
    return it->second;
  return {};
}

std::string_view spell_info::effect_property_str( const spelleffect_data_t* effect )
{
  auto it = _property_type_strings.find( effect->property_type() );
  if ( it != _property_type_strings.end() )
    return it->second;
  return {};
}

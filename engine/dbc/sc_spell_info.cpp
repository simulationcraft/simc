// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_spell_info.hpp"

#include "dbc.hpp"
#include "dbc/item_set_bonus.hpp"
#include "dbc/trait_data.hpp"
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

  std::string s;

  s = fn( data.front() );

  if ( !wrap )
  {
    for ( auto it = data.begin() + 1; it != data.end(); ++it )
    {
      s += delim;
      s += fn( *it );
    }
  }
  else
  {
    size_t len = s.size();
    auto delim_len = delim.size();

    for ( auto it = data.begin() + 1; it != data.end(); ++it )
    {
      auto str = fn( *it );
      auto str_len = str.size();
      auto new_len = len + delim_len + str_len;

      if ( new_len + 1 >= wrap )
      {
        len = str_len;
        s += wrap_delim;
        s += str;
      }
      else
      {
        len += delim_len + str_len;
        s += delim;
        s += str;
      }
    }
  }

  return s;
}

template <typename Range>
std::string wrap_join( Range&& data, size_t wrap, const std::string& delim = ", ",
                       const std::string& wrap_delim = ",\n                   " )
{
  return wrap_concatenate( std::forward<Range>( data ), []( std::string_view s ) {
    return s;
  }, wrap, delim, wrap_delim );
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
  { PF_HEARTBEAT,              "Heartbeat"                   },  // 0x0000000001
  { PF_KILLING_BLOW,           "Killing Blow"                },  // 0x0000000002
  { PF_MELEE,                  "White Melee"                 },  // 0x0000000004
  { PF_MELEE_TAKEN,            "White Melee Taken"           },  // 0x0000000008
  { PF_MELEE_ABILITY,          "Yellow Melee"                },  // 0x0000000010
  { PF_MELEE_ABILITY_TAKEN,    "Yellow Melee Taken"          },  // 0x0000000020
  { PF_RANGED,                 "White Ranged"                },  // 0x0000000040
  { PF_RANGED_TAKEN,           "White Ranged Taken"          },  // 0x0000000080
  { PF_RANGED_ABILITY,         "Yellow Ranged"               },  // 0x0000000100
  { PF_RANGED_ABILITY_TAKEN,   "Yellow Ranged Taken"         },  // 0x0000000200
  { PF_NONE_HELPFUL,           "Generic Helpful"             },  // 0x0000000400
  { PF_NONE_HELPFUL_TAKEN,     "Generic Helpful Taken"       },  // 0x0000000800
  { PF_NONE_HARMFUL,           "Generic Hostile Spell"       },  // 0x0000001000
  { PF_NONE_HARMFUL_TAKEN,     "Generic Hostile Spell Taken" },  // 0x0000002000
  { PF_MAGIC_HEAL,             "Magic Heal"                  },  // 0x0000004000
  { PF_MAGIC_HEAL_TAKEN,       "Magic Heal Taken"            },  // 0x0000008000
  { PF_MAGIC_SPELL,            "Magic Hostile Spell"         },  // 0x0000010000
  { PF_MAGIC_SPELL_TAKEN,      "Magic Hostile Spell Taken"   },  // 0x0000020000
  { PF_PERIODIC,               "Periodic"                    },  // 0x0000040000
  { PF_PERIODIC_TAKEN,         "Periodic Taken"              },  // 0x0000080000
  { PF_ANY_DAMAGE_TAKEN,       "Any Damage Taken"            },  // 0x0000100000
  { PF_HELPFUL_PERIODIC,       "Helpful Periodic"            },  // 0x0000200000
  { PF_MAINHAND,               "Melee Main-Hand"             },  // 0x0000400000
  { PF_OFFHAND,                "Melee Off-Hand"              },  // 0x0000800000
  { PF_DEATH,                  "Death"                       },  // 0x0001000000
  { PF_JUMP,                   "Proc on jump"                },  // 0x0002000000
  { PF_CLONE_SPELL,            "Proc Clone Spell"            },  // 0x0004000000
  { PF_ENTER_COMBAT,           "Enter Combat"                },  // 0x0008000000
  { PF_ENCOUNTER_START,        "Encounter Start"             },  // 0x0010000000
  { PF_CAST_ENDED,             "Cast Ended"                  },  // 0x0020000000
  { PF_LOOTED,                 "Looted"                      },  // 0x0040000000
  { PF_HELPFUL_PERIODIC_TAKEN, "Helpful Periodic Taken"      },  // 0x0080000000
  { PF_TARGET_DIES,            "Target Dies"                 },  // 0x0100000000
  { PF_KNOCKBACK,              "Knockback"                   },  // 0x0200000000
  { PF_CAST_SUCCESSFUL,        "Cast Successful"             },  // 0x0400000000
  { PF_UNKNOWN_36,             "Unknown 36"                  },  // 0x0800000000
  { PF_UNKNOWN_37,             "Unknown 37"                  },  // 0x1000000000
  { PF_UNKNOWN_38,             "Unknown 38"                  },  // 0x2000000000
  { PF_UNKNOWN_39,             "Unknown 39"                  },  // 0x4000000000
} };

struct class_map_entry_t
{
  const char* name;
  player_e pt;
};
static constexpr std::array<class_map_entry_t, 15> _class_map { {
  { nullptr,        PLAYER_NONE   },  // 0
  { "Warrior",      WARRIOR       },  // 13
  { "Paladin",      PALADIN       },  // 8
  { "Hunter",       HUNTER        },  // 5
  { "Rogue",        ROGUE         },  // 10
  { "Priest",       PRIEST        },  // 9
  { "Death Knight", DEATH_KNIGHT  },  // 1
  { "Shaman",       SHAMAN        },  // 11
  { "Mage",         MAGE          },  // 6
  { "Warlock",      WARLOCK       },  // 12
  { "Monk",         MONK          },  // 7
  { "Druid",        DRUID         },  // 3
  { "Demon Hunter", DEMON_HUNTER  },  // 2
  { "Evoker",       EVOKER        },  // 4
  { nullptr,        PLAYER_NONE   },  // 0
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
  { 19, "Haranir"             },
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
  { T_UNIT_CASTER,                                   "Self"                                           },  // 1
  { T_UNIT_NEARBY_ENEMY,                             "Nearby Enemy"                                   },  // 2
  { T_UNIT_NEARBY_ALLY,                              "Nearby Ally"                                    },  // 3
  { T_UNIT_NEARBY_PARTY,                             "Nearby Party"                                   },  // 4
  { T_UNIT_PET,                                      "Active Pet"                                     },  // 5
  { T_UNIT_TARGET_ENEMY,                             "Targeted Enemy"                                 },  // 6
  { T_UNIT_SOURCE_AREA,                              "any in Area"                                    },  // 7
  { T_UNIT_DESTINATION_AREA,                         "at any in Area"                                 },  // 8
  { T_DESTINATION_HOME,                              "at Home"                                        },  // 9
  { T_UNIT_SOURCE_AREA_UNK_11,                       "Unknown in Area"                                },  // 11
  { T_UNIT_SOURCE_AREA_ENEMY,                        "Enemy in Area"                                  },  // 15
  { T_UNIT_DESTINATION_AREA_ENEMY,                   "at Enemy in Area"                               },  // 16
  { T_DESTINATION_DB,                                "at Database Entry in Area"                      },  // 17
  { T_DESTINATION_CASTER,                            "at Self"                                        },  // 18
  { T_UNIT_CASTER_AREA_PARTY,                        "Party in Area"                                  },  // 20
  { T_UNIT_TARGET_ALLY,                              "Targeted Ally"                                  },  // 21
  { T_SOURCE_CASTER,                                 "Source"                                         },  // 22
  { T_GAMEOBJECT_TARGET,                             "Targeted Gameobject"                            },  // 23
  { T_UNIT_CONE_ENEMY_24,                            "Enemies in Cone"                                },  // 24
  { T_UNIT_TARGET_ANY,                               "Current Target"                                 },  // 25
  { T_GAMEOBJECT_ITEM_TARGET,                        "Targeted Item Gameobject"                       },  // 26
  { T_UNIT_MASTER,                                   "Master"                                         },  // 27
  { T_DESTINATION_DYNAMIC_OBJECT_ENEMY,              "at Dynamic Enemy in Area"                       },  // 28
  { T_DESTINATION_DYNAMIC_OBJECT_ALLY,               "at Dynamic Ally in Area"                        },  // 29
  { T_UNIT_SOURCE_AREA_ALLY,                         "Ally in Area"                                   },  // 30
  { T_UNIT_DESTINATION_AREA_ALLY,                    "At Ally in Area"                                },  // 31
  { T_DESTINATION_CASTER_SUMMON,                     "Summoner in Area"                               },  // 32
  { T_UNIT_SOURCE_AREA_PARTY,                        "Party in Area"                                  },  // 33
  { T_UNIT_DESTINATION_AREA_PARTY,                   "at Party in Area"                               },  // 34
  { T_UNIT_TARGET_PARTY,                             "Targeted Party Member"                          },  // 35
  { T_UNIT_LAST_TARGET_AREA_PARTY,                   "Last Targeted Party Member"                     },  // 37
  { T_UNIT_NEARBY,                                   "Nearby Target"                                  },  // 38
  { T_DESTINATION_CASTER_FISHING,                    "at Fishing Target"                              },  // 39
  { T_GAMEOBJECT_NEARBY,                             "Nearby Gameobject"                              },  // 40
  { T_DESTINATION_CASTER_FRONT_RIGHT,                "Front Right of Self"                            },  // 41
  { T_DESTINATION_CASTER_BACK_RIGHT,                 "Back Right of Self"                             },  // 42
  { T_DESTINATION_CASTER_BACK_LEFT,                  "Back Left of Self"                              },  // 43
  { T_DESTINATION_CASTER_FRONT_LEFT,                 "Front Left of Self"                             },  // 44
  { T_UNIT_TARGET_CHAINHEAL_ALLY,                    "Chain Heal Ally"                                },  // 45
  { T_DESTINATION_NEARBY_46,                         "at Nearby"                                      },  // 46
  { T_DESTINATION_CASTER_FRONT,                      "Front of Self"                                  },  // 47
  { T_DESTINATION_CASTER_BACK,                       "Back of Self"                                   },  // 48
  { T_DESTINATION_CASTER_RIGHT,                      "Right of Self"                                  },  // 49
  { T_DESTINATION_CASTER_LEFT,                       "Left of Self"                                   },  // 50
  { T_GAMEOBJECT_SOURCE_AREA,                        "Gameobject in Area"                             },  // 51
  { T_GAMEOBJECT_DESTINATION_AREA,                   "at Gameobject in Area"                          },  // 52
  { T_DESTINATION_TARGET_ENEMY,                      "At Enemy"                                       },  // 53
  { T_UNIT_CONE_180_DEG_ENEMY,                       "Enemies in Cone (180 degree)"                   },  // 54
  { T_DEST_CASTER_FRONT_LEAP,                        "at Front of Leap Target"                        },  // 55
  { T_UNIT_CASTER_AREA_RAID,                         "Raid in Area"                                   },  // 56
  { T_UNIT_CASTER_TARGET_RAID,                       "Targeted Raid Member"                           },  // 57
  { T_UNIT_CASTER_NEARBY_RAID,                       "Raid Nearby"                                    },  // 58
  { T_UNIT_CONE_ALLY,                                "Allies in Cone"                                 },  // 59
  { T_UNIT_CONE,                                     "Any in Cone"                                    },  // 60
  { T_UNIT_TAGET_AREA_RAID_CLASS,                    "Raid Class in Area"                             },  // 61
  { T_DESTINATION_CASTER_GROUND_62,                  "at Ground"                                      },  // 62
  { T_DESTINATION_TARGET_ANY,                        "at Target"                                      },  // 63
  { T_DESTINATION_TARGET_FRONT,                      "at Target Front"                                },  // 64
  { T_DESTINATION_TARGET_BACK,                       "at Target Back"                                 },  // 65
  { T_DESTINATION_TARGET_RIGHT,                      "at Target Right"                                },  // 66
  { T_DESTINATION_TARGET_LEFT,                       "at Target Left"                                 },  // 67
  { T_DESTINATION_TARGET_FRONT_RIGHT,                "at Target Front Right"                          },  // 68
  { T_DESTINATION_TARGET_BACK_RIGHT,                 "at Target Back Right"                           },  // 69
  { T_DESTINATION_TARGET_BACK_LEFT,                  "at Target Back Left"                            },  // 70
  { T_DESTINATION_TARGET_FRONT_LEFT,                 "at Target Front Left"                           },  // 71
  { T_DESTINATION_CASTER_RANDOM,                     "at Random"                                      },  // 72
  { T_DESTINATION_CASTER_RADIUS,                     "at Radius"                                      },  // 73
  { T_DESTINATION_TARGET_RANDOM,                     "at Target Random"                               },  // 74
  { T_DESTINATION_TARGET_RADIUS,                     "at Target Radius"                               },  // 75
  { T_DESTINATION_CHANNEL_TARGET,                    "at Channel Target"                              },  // 76
  { T_UNIT_CHANNEL_TARGET,                           "Channel Target"                                 },  // 77
  { T_DESTINATION_DESTINATION_FRONT,                 "Front in Area"                                  },  // 78
  { T_DESTINATION_DESTINATION_BACK,                  "Back in Area"                                   },  // 79
  { T_DESTINATION_DESTINATION_RIGHT,                 "Right in Area"                                  },  // 80
  { T_DESTINATION_DESTINATION_LEFT,                  "Left in Area"                                   },  // 81
  { T_DESTINATION_DESTINATION_FRONT_RIGHT,           "Front Right in Area"                            },  // 82
  { T_DESTINATION_DESTINATION_BACK_RIGHT,            "Back Right in Area"                             },  // 83
  { T_DESTINATION_DESTINATION_BACK_LEFT,             "Back Left in Area"                              },  // 84
  { T_DESTINATION_DESTINATION_FRONT_LEFT,            "Front Left in Area"                             },  // 85
  { T_DESTINATION_DESTINATION_RANDOM,                "Random in Area"                                 },  // 86
  { T_DESTINATION_DESTINATION,                       "at Area"                                        },  // 87
  { T_DESTINATION_DYNAMIC_OBJECT_NONE,               "at Dynamic Object"                              },  // 88
  { T_DESTINATION_TRAJECTORY,                        "any in Trajectory"                              },  // 89
  { T_UNIT_TARGET_MINIPET,                           "Target's Battlepet"                             },  // 90
  { T_DESTINATION_DESTINATION_RADIUS,                "at Radius in Area"                              },  // 91
  { T_UNIT_SUMMONER,                                 "Summoner"                                       },  // 92
  { T_CORPSE_SOURCE_AREA_ENEMY,                      "Enemy Corpse in Area"                           },  // 93
  { T_UNIT_VEHICLE,                                  "Vehicle"                                        },  // 94
  { T_UNIT_TARGET_PASSENGER,                         "Passenger"                                      },  // 95
  { T_UNIT_PASSENGER_0,                              "Passenger 0"                                    },  // 96
  { T_UNIT_PASSENGER_1,                              "Passenger 1"                                    },  // 97
  { T_UNIT_PASSENGER_2,                              "Passenger 2"                                    },  // 98
  { T_UNIT_PASSENGER_3,                              "Passenger 3"                                    },  // 99
  { T_UNIT_PASSENGER_4,                              "Passenger 4"                                    },  // 100
  { T_UNIT_PASSENGER_5,                              "Passenger 5"                                    },  // 101
  { T_UNIT_PASSENGER_6,                              "Passenger 6"                                    },  // 102
  { T_UNIT_PASSENGER_7,                              "Passenger 7"                                    },  // 103
  { T_UNIT_CONE_CASTER_TO_DESTINATION_ENEMY,         "Enemies in Cone to Targeted Enemy"              },  // 104
  { T_UNIT_CASTER_AND_PASSENGERS,                    "Self and Passengers"                            },  // 105
  { T_DESTINATION_NEARBY_DB,                         "Nearby in Database"                             },  // 106
  { T_DESTINATION_NEARBY_107,                        "At Nearby"                                      },  // 107
  { T_GAMEOBJECT_CONE_CASTER_TO_DESTINATION_ENEMY,   "Gameobject in Cone to Targeted Enemy"           },  // 108
  { T_GAMEOBJECT_CONE_CASTER_TO_DESTINATION_ALLY,    "Gameobject in Cone to Targeted Ally"            },  // 109
  { T_UNIT_CONE_CASTER_TO_DESTINATION,               "Cone from Self"                                 },  // 110
  { T_UNIT_SOURCE_AREA_FURTHEST_ENEMY,               "Furthest Enemy in Area"                         },  // 115
  { T_UNIT_AND_DESTINATION_LAST_ENEMY,               "Last Targeted Enemy"                            },  // 116
  { T_UNIT_TARGET_ALLY_OR_RAID,                      "Targeted Ally or Raid"                          },  // 118
  { T_CORPSE_SOURCE_AREA_RAID,                       "Raid Corpse in Area"                            },  // 119
  { T_UNIT_SELF_AND_SUMMONS,                         "Self and Summons in Area"                       },  // 120
  { T_CORPSE_TARGET_ALLY,                            "Targeted Ally Corpse"                           },  // 121
  { T_UNIT_AREA_THREAT_LIST,                         "Threat List in Area"                            },  // 122
  { T_UNIT_AREA_TAP_LIST,                            "Tap List in Area"                               },  // 123
  { T_UNIT_TARGET_TAP_LIST,                          "Targeted Tap List"                              },  // 124
  { T_DESTINATION_CASTER_GROUND_125,                 "at Ground"                                      },  // 125
  { T_UNIT_CASTER_AREA_ENEMY_CLUMP,                  "Enemy Clump in Area"                            },  // 126
  { T_DESTINATION_CASTER_ENEMY_CLUMP_CENTROID,       "at Center of Enemy Clump in Area"               },  // 127
  { T_UNIT_RECT_CASTER_ALLY,                         "Target Reticule: Ally"                          },  // 128
  { T_UNIT_RECT_CASTER_ENEMY,                        "Target Reticule: Enemy"                         },  // 129
  { T_UNIT_RECT_CASTER,                              "Target Reticule"                                },  // 130
  { T_UNIT_DESTINATION_SUMMONER,                     "at Summoner"                                    },  // 131
  { T_UNIT_DESTINATION_TARGET_ALLY,                  "at Targeted Ally"                               },  // 132
  { T_UNIT_LINE_CASTER_TO_DESTINATION_ALLY,          "Line to Ally"                                   },  // 133
  { T_UNIT_LINE_CASTER_TO_DESTINATION_ENEMY,         "Line to Enemy"                                  },  // 134
  { T_UNIT_LINE_CASTER_TO_DESTINATION,               "Line from Self"                                 },  // 135
  { T_UNIT_CONE_CASTER_TO_DESTINATION_ALLY,          "Cone to Ally"                                   },  // 136
  { T_UNIT_DESTINATION_CASTER_MOVEMENT_DIRECTION,    "in Area of Movement Direction"                  },  // 137
  { T_UNIT_DESTINATION_DESTINATION_GROUND,           "in Area of Ground Area"                         },  // 138
  { T_DESTINATION_CASTER_CLUMP_CENTROID,             "at Center of Ally Clump in Area"                },  // 140
  { T_DESTINATION_NEARBY_ENTRY_OR_DB,                "Nearby: in Database or Entry"                   },  // 142
  { T_DESTINATION_DESTINATION_TARGET_TOWARDS_CASTER, "in Area From Target to Self"                    },  // 148
  { T_UNIT_OWN_CRITTER,                              "Own Critter"                                    },  // 150
  { T_UNIT_CASTER_AREA_ENEMY,                        "Enemy in Area around Caster"                    },  // 151
} );

static constexpr auto _resource_strings = util::make_static_map<int, std::string_view>( {
  { POWER_HEALTH,         "Health"         },  // -2
  { POWER_MANA,           "Base Mana"      },  // 0
  { POWER_RAGE,           "Rage"           },  // 1
  { POWER_FOCUS,          "Focus"          },  // 2
  { POWER_ENERGY,         "Energy"         },  // 3
  { POWER_COMBO_POINT,    "Combo Points"   },  // 4
  { POWER_RUNE,           "Rune"           },  // 5
  { POWER_RUNIC_POWER,    "Runic Power"    },  // 6
  { POWER_SOUL_SHARDS,    "Soul Shard"     },  // 7
  { POWER_ASTRAL_POWER,   "Astral Power"   },  // 8
  { POWER_HOLY_POWER,     "Holy Power"     },  // 9
  { POWER_MAELSTROM,      "Maelstrom"      },  // 11
  { POWER_CHI,            "Chi"            },  // 12
  { POWER_INSANITY,       "Insanity"       },  // 13
  { POWER_ARCANE_CHARGES, "Arcane Charges" },  // 16
  { POWER_FURY,           "Fury"           },  // 17
  { POWER_PAIN,           "Pain"           },  // 18
  { POWER_ESSENCE,        "Essence"        },  // 19
} );

// Mappings from Classic EnumeratedString.db2
static constexpr auto _attribute_strings = util::make_static_map<unsigned, std::string_view>( {
  { 0, "Proc Failure Burns Charge"                                               },
  { SX_RANGED_ABILITY, "Uses Ranged Slot"                                        },  // 1
  { 2, "On Next Swing (No Damage)"                                               },
  { 3, "Do Not Log Immune Misses"                                                },
  { SX_ABILITY, "Is Ability"                                                     },  // 4
  { SX_TRADESKILL_ABILITY, "Is Tradeskill"                                       },  // 5
  { SX_PASSIVE, "Passive"                                                        },  // 6
  { SX_HIDDEN, "Do Not Display (Spellbook, Aura Icon, Combat Log)"               },  // 7
  { 8, "Do Not Log"                                                              },
  { 9, "Held Item Only"                                                          },
  { 10, "On Next Swing"                                                          },
  { 11, "Wearer Casts Proc Trigger"                                              },
  { 12, "Server Only"                                                            },
  { 13, "Allow Item Spell In PvP"                                                },
  { 14, "Only Indoors"                                                           },
  { 15, "Only Outdoors"                                                          },
  { 16, "Not Shapeshifted"                                                       },
  { SX_REQ_STEALTH, "Only Stealthed"                                             },  // 17
  { 18, "Do Not Sheath"                                                          },
  { 19, "Scales w/ Creature Level"                                               },
  { SX_CANCEL_AUTO_ATTACK, "Cancels Auto Attack Combat"                          },  // 20
  { SX_NO_D_P_B, "No Active Defense"                                             },  // 21
  { SX_NO_COMBAT, "Track Target in Cast (Player Only)"                           },  // 22
  { 23, "Allow Cast While Dead"                                                  },
  { 24, "Allow While Mounted"                                                    },
  { 25, "Cooldown On Event"                                                      },
  { 26, "Aura Is Debuff"                                                         },
  { 27, "Allow While Sitting"                                                    },
  { 28, "Not In Combat (Only Peaceful)"                                          },
  { 29, "No Immunities"                                                          },
  { 30, "Heartbeat Resist"                                                       },
  { SX_NO_CANCEL, "No Aura Cancel"                                               },  // 31
  { 32, "Dismiss Pet First"                                                      },
  { 33, "Use All Mana"                                                           },
  { SX_CHANNELED, "Is Channelled"                                                },  // 34
  { 35, "No Redirection"                                                         },
  { 36, "No Skill Increase"                                                      },
  { SX_NO_STEALTH_BREAK, "Allow While Stealthed"                                 },  // 37
  { SX_CHANNELED_2, "Is Self Channelled"                                         },  // 38
  { 39, "No Reflection"                                                          },
  { 40, "Only Peaceful Targets"                                                  },
  { SX_MELEE_COMBAT_START, "Initiates Combat (Enables Auto-Attack)"              },  // 41
  { SX_NO_THREAT, "No Threat"                                                    },  // 42
  { 43, "Aura Unique"                                                            },
  { 44, "Failure Breaks Stealth"                                                 },
  { 45, "Toggle Far Sight"                                                       },
  { 46, "Track Target in Channel"                                                },
  { 47, "Immunity Purges Effect"                                                 },
  { 48, "Immunity to Hostile & Friendly Effects"                                 },
  { 49, "No AutoCast (AI)"                                                       },
  { 50, "Prevents Anim"                                                          },
  { 51, "Exclude Caster"                                                         },
  { 52, "Finishing Move - Damage"                                                },
  { 53, "Threat only on Miss"                                                    },
  { 54, "Finishing Move - Duration"                                              },
  { 55, "Ignore Owner's Death"                                                   },
  { 56, "Special Skillup"                                                        },
  { 57, "Aura Stays After Combat"                                                },
  { 58, "Require All Targets"                                                    },
  { SX_DISCOUNT_ON_MISS, "Discount Power On Miss"                                },  // 59
  { SX_DONT_DISPLAY_IN_AURA_BAR, "No Aura Icon"                                  },  // 60
  { 61, "Name in Channel Bar"                                                    },
  { 62, "Combo on Block (Mainline: Dispel All Stacks)"                           },
  { 63, "Cast When Learned"                                                      },
  { 64, "Allow Dead Target"                                                      },
  { 65, "No shapeshift UI"                                                       },
  { 66, "Ignore Line of Sight"                                                   },
  { 67, "Allow Low Level Buff"                                                   },
  { 68, "Use Shapeshift Bar"                                                     },
  { 69, "Auto Repeat"                                                            },
  { 70, "Cannot cast on tapped"                                                  },
  { 71, "Do Not Report Spell Failure"                                            },
  { 72, "Include In Advanced Combat Log"                                         },
  { 73, "Always Cast As Unit"                                                    },
  { 74, "Special Taming Flag"                                                    },
  { 75, "No Target Per-Second Costs"                                             },
  { 76, "Chain From Caster"                                                      },
  { 77, "Enchant own item only"                                                  },
  { 78, "Allow While Invisible"                                                  },
  { 79, "Do Not Consume if Gained During Cast"                                   },
  { 80, "No Active Pets"                                                         },
  { 81, "Do Not Reset Combat Timers"                                             },
  { 82, "No Jump While Cast Pending"                                             },
  { 83, "Allow While Not Shapeshifted (caster form)"                             },
  { 84, "Initiate Combat Post-Cast (Enables Auto-Attack)"                        },
  { 85, "Fail on all targets immune"                                             },
  { 86, "No Initial Threat"                                                      },
  { 87, "Proc Cooldown On Failure"                                               },
  { 88, "Item Cast With Owner Skill"                                             },
  { 89, "Don't Block Mana Regen"                                                 },
  { 90, "No School Immunities"                                                   },
  { 91, "Ignore Weaponskill"                                                     },
  { 92, "Not an Action"                                                          },
  { SX_CANNOT_CRIT, "Can't Crit"                                                 },  // 93
  { 94, "Active Threat"                                                          },
  { SX_FOOD_AURA, "Retain Item Cast"                                             },  // 95
  { 96, "PvP Enabling"                                                           },
  { 97, "No Proc Equip Requirement"                                              },
  { 98, "No Casting Bar Text"                                                    },
  { 99, "Completely Blocked"                                                     },
  { 100, "No Res Timer"                                                          },
  { 101, "No Durability Loss"                                                    },
  { 102, "No Avoidance"                                                          },
  { 103, "DoT Stacking Rule"                                                     },
  { 104, "Only On Player"                                                        },
  { SX_NOT_A_PROC, "Not a Proc"                                                  },  // 105
  { SX_REQ_MAIN_HAND, "Requires Main-Hand Weapon"                                },  // 106
  { 107, "Only Battlegrounds"                                                    },
  { 108, "Only On Ghosts"                                                        },
  { 109, "Hide Channel Bar"                                                      },
  { 110, "Hide In Raid Filter"                                                   },
  { 111, "Normal Ranged Attack"                                                  },
  { SX_SUPPRESS_CASTER_PROCS, "Suppress Caster Procs"                            },  // 112
  { SX_SUPPRESS_TARGET_PROCS, "Suppress Target Procs"                            },  // 113
  { SX_ALWAYS_HIT, "Always Hit"                                                  },  // 114
  { 115, "Instant Target Procs"                                                  },
  { 116, "Allow Aura While Dead"                                                 },
  { 117, "Only Proc Outdoors"                                                    },
  { 118, "Casting Cancels Autorepeat (Mainline: Do Not Trigger Target Stand)"    },
  { 119, "No Damage History"                                                     },
  { SX_REQ_OFF_HAND, "Requires Off-Hand Weapon"                                  },  // 120
  { SX_TREAT_AS_PERIODIC, "Treat As Periodic"                                    },  // 121
  { SX_CAN_PROC_FROM_PROCS, "Can Proc From Procs"                                },  // 122
  { 123, "Only Proc on Caster"                                                   },
  { 124, "Ignore Caster & Target Restrictions"                                   },
  { 125, "Ignore Caster Modifiers"                                               },
  { 126, "Do Not Display Range"                                                  },
  { 127, "Not On AOE Immune"                                                     },
  { 128, "No Cast Log"                                                           },
  { 129, "Class Trigger Only On Target"                                          },
  { 130, "Aura Expires Offline"                                                  },
  { 131, "No Helpful Threat"                                                     },
  { 132, "No Harmful Threat"                                                     },
  { 133, "Allow Client Targeting"                                                },
  { 134, "Cannot Be Stolen"                                                      },
  { 135, "Allow Cast While Casting"                                              },
  { SX_DISABLE_TARGET_MULT, "Ignore Damage Taken Modifiers"                      },  // 136
  { 137, "Combat Feedback When Usable"                                           },
  { 138, "Weapon Speed Cost Scaling"                                             },
  { 139, "No Partial Immunity"                                                   },
  { 140, "Aura Is Buff"                                                          },
  { 141, "Do Not Log Caster"                                                     },
  { 142, "Reactive Damage Proc"                                                  },
  { 143, "Not In Spellbook"                                                      },
  { 144, "Not In Arena or Rated Battleground"                                    },
  { 145, "Ignore Default Arena Restrictions"                                     },
  { 146, "Bouncy Chain Missiles"                                                 },
  { 147, "Allow Proc While Sitting"                                              },
  { 148, "Aura Never Bounces"                                                    },
  { 149, "Allow Entering Arena"                                                  },
  { 150, "Proc Suppress Swing Anim"                                              },
  { SX_DISABLE_WEAPON_PROCS, "Suppress Weapon Procs"                             },  // 151
  { 152, "Auto Ranged Combat"                                                    },
  { 153, "Owner Power Scaling"                                                   },
  { 154, "Only Flying Areas"                                                     },
  { 155, "Force Display Castbar"                                                 },
  { 156, "Ignore Combat Timer"                                                   },
  { 157, "Aura Bounce Fails Spell"                                               },
  { 158, "Obsolete"                                                              },
  { 159, "Use Facing From Spell"                                                 },
  { 160, "Allow Actions During Channel"                                          },
  { 161, "No Reagent Cost With Aura"                                             },
  { 162, "Remove Entering Arena"                                                 },
  { 163, "Allow While Stunned"                                                   },
  { 164, "Triggers Channeling"                                                   },
  { 165, "Limit N"                                                               },
  { 166, "Ignore Area Effect PvP Check"                                          },
  { 167, "Not On Player"                                                         },
  { 168, "Not On Player Controlled NPC"                                          },
  { SX_TICK_ON_APPLICATION, "Extra Initial Period"                               },  // 169
  { 170, "Do Not Display Duration"                                               },
  { 171, "Implied Targeting"                                                     },
  { 172, "Melee Chain Targeting"                                                 },
  { SX_DOT_HASTED, "Spell Haste Affects Periodic"                                },  // 173
  { 174, "Not Available While Charmed"                                           },
  { SX_TREAT_AS_AREA_EFFECT, "Treat as Area Effect"                              },  // 175
  { 176, "Aura Affects Not Just Req. Equipped Item"                              },
  { 177, "Allow While Fleeing"                                                   },
  { 178, "Allow While Confused"                                                  },
  { 179, "AI Doesn't Face Target"                                                },
  { 180, "Do Not Attempt a Pet Resummon When Dismounting"                        },
  { 181, "Ignore Target Requirements"                                            },
  { 182, "Not On Trivial"                                                        },
  { 183, "No Partial Resists"                                                    },
  { 184, "Ignore Caster Requirements"                                            },
  { 185, "Always Line of Sight"                                                  },
  { SX_REQ_LINE_OF_SIGHT, "Always AOE Line of Sight"                             },  // 186
  { 187, "No Caster Aura Icon"                                                   },
  { 188, "No Target Aura Icon"                                                   },
  { 189, "Aura Unique Per Caster"                                                },
  { 190, "Always Show Ground Texture"                                            },
  { 191, "Add Melee Hit Rating"                                                  },
  { 192, "No Cooldown On Tooltip"                                                },
  { 193, "Do Not Reset Cooldown In Arena"                                        },
  { 194, "Not an Attack"                                                         },
  { 195, "Can Assist Immune PC"                                                  },
  { SX_IGNORE_FOR_MOD_TIME_RATE, "Ignore For Mod Time Rate"                      },  // 196
  { 197, "Do Not Consume Resources"                                              },
  { 198, "Floating Combat Text On Cast"                                          },
  { 199, "Aura Is Weapon Proc"                                                   },
  { 200, "Do Not Chain To Crowd-Controlled Targets"                              },
  { 201, "Allow On Charmed Targets"                                              },
  { 202, "No Aura Log"                                                           },
  { 203, "Not In Raid Instances"                                                 },
  { 204, "Allow While Riding Vehicle"                                            },
  { 205, "Ignore Phase Shift"                                                    },
  { 206, "AI Primary Ranged Attack"                                              },
  { 207, "No Pushback"                                                           },
  { 208, "No Jump Pathing"                                                       },
  { 209, "Allow Equip While Casting"                                             },
  { 210, "Originate From Controller"                                             },
  { 211, "Delay Combat Timer During Cast"                                        },
  { 212, "Aura Icon Only For Caster (Limit 10)"                                  },
  { 213, "Show Mechanic as Combat Text"                                          },
  { 214, "Absorb Cannot Be Ignore"                                               },
  { 215, "Taps immediately"                                                      },
  { 216, "Can Target Untargetable"                                               },
  { 217, "Doesn't Reset Swing Timer if Instant"                                  },
  { 218, "Vehicle Immunity Category"                                             },
  { 219, "Ignore Healing Modifiers"                                              },
  { 220, "Do Not Auto Select Target with Initiates Combat"                       },
  { SX_DISABLE_PLAYER_MULT, "Ignore Caster Damage Modifiers"                     },  // 221
  { 222, "Disable Tied Effect Points"                                            },
  { 223, "No Category Cooldown Mods"                                             },
  { 224, "Allow Spell Reflection"                                                },
  { 225, "No Target Duration Mod"                                                },
  { 226, "Disable Aura While Dead"                                               },
  { 227, "Debug Spell"                                                           },
  { 228, "Treat as Raid Buff"                                                    },
  { 229, "Can Be Multi Cast"                                                     },
  { 230, "Don't Cause Spell Pushback"                                            },
  { 231, "Prepare for Vehicle Control End"                                       },
  { 232, "Horde Specific Spell"                                                  },
  { 233, "Alliance Specific Spell"                                               },
  { 234, "Dispel Removes Charges"                                                },
  { 235, "Can Cause Interrupt"                                                   },
  { 236, "Can Cause Silence"                                                     },
  { 237, "No UI Not Interruptible"                                               },
  { 238, "Recast On Resummon"                                                    },
  { 239, "Reset Swing Timer at spell start"                                      },
  { 240, "Only In Spellbook Until Learned"                                       },
  { 241, "Do Not Log PvP Kill"                                                   },
  { 242, "Attack on Charge to Unit"                                              },
  { 243, "Report Spell failure to unit target"                                   },
  { 244, "No Client Fail While Stunned, Fleeing, Confused"                       },
  { 245, "Retain Cooldown Through Load"                                          },
  { 246, "Ignores Cold Weather Flying Requirement"                               },
  { SX_NO_DODGE, "No Attack Dodge"                                               },  // 247
  { SX_NO_PARRY, "No Attack Parry"                                               },  // 248
  { SX_NO_MISS, "No Attack Miss"                                                 },  // 249
  { 250, "Treat as NPC AoE"                                                      },
  { 251, "Bypass No Resurrect Aura"                                              },
  { 252, "Do Not Count For PvP Scoreboard"                                       },
  { 253, "Reflection Only Defends"                                               },
  { SX_CAN_PROC_FROM_SUPPRESSED_TGT, "Can Proc From Suppressed Target Procs"     },  // 254
  { 255, "Always Cast Log"                                                       },
  { SX_NO_BLOCK, "No Attack Block"                                               },  // 256
  { 257, "Ignore Dynamic Object Caster"                                          },
  { 258, "Remove Outside Dungeons and Raids"                                     },
  { 259, "Only Target If Same Creator"                                           },
  { 260, "Can Hit AOE Untargetable"                                              },
  { 261, "Allow While Charmed"                                                   },
  { 262, "Aura Required by Client"                                               },
  { 263, "Ignore Sanctuary"                                                      },
  { 264, "Use Target's Level for Spell Scaling"                                  },
  { SX_TICK_MAY_CRIT, "Periodic Can Crit"                                        },  // 265
  { 266, "Mirror creature name"                                                  },
  { 267, "Only Players Can Cast This Spell"                                      },
  { 268, "Aura Points On Client"                                                 },
  { 269, "Not In Spellbook Until Learned"                                        },
  { 270, "Target Procs On Caster"                                                },
  { 271, "Requires location to be on liquid surface"                             },
  { 272, "Only Target Own Summons"                                               },
  { SX_DURATION_HASTED, "Haste Affects Duration"                                 },  // 273
  { 274, "Ignore Spellcast Override Cost"                                        },
  { 275, "Allow Targets Hidden by Spawn Tracking"                                },
  { SX_REQUIRES_EQUIPPED_ARMOR_TYPE, "Requires Equipped Inv Types"               },  // 276
  { 277, "No 'Summon + Dest from Client' Targeting Pathing Requirement"          },
  { SX_DOT_HASTED_MELEE, "Melee Haste Affects Periodic"                          },  // 278
  { 279, "Enforce In Combat Ressurection Limit"                                  },
  { 280, "Heal Prediction"                                                       },
  { 281, "No Level Up Toast"                                                     },
  { 282, "Skip Is Known Check"                                                   },
  { 283, "AI Face Target"                                                        },
  { 284, "Not in Battleground"                                                   },
  { SX_MASTERY_AFFECTS_POINTS, "Mastery Affects Points"                          },  // 285
  { 286, "Display Large Aura Icon On Unit Frames (Boss Aura)"                    },
  { 287, "Can Attack ImmunePC"                                                   },
  { 288, "Force Dest Location"                                                   },
  { 289, "Mod Invis Includes Party"                                              },
  { 290, "Only When Illegally Mounted"                                           },
  { 291, "Do Not Log Aura Refresh"                                               },
  { SX_FIXED_TRAVEL_TIME, "Missile Speed is Delay (in sec)"                      },  // 292
  { 293, "Ignore Totem Requirements for Casting"                                 },
  { 294, "Item Cast Grants Skill Gain"                                           },
  { 295, "Do Not Add to Unlearn List"                                            },
  { 296, "Cooldown Ignores Ranged Weapon"                                        },
  { 297, "Not In Arena"                                                          },
  { 298, "Target Must Be Grounded"                                               },
  { 299, "Allow While Banished Aura State"                                       },
  { 300, "Face unit target upon completion of jump charge"                       },
  { 301, "Haste Affects Melee Ability Casttime"                                  },
  { 302, "Ignore Default Rated Battleground Restrictions"                        },
  { 303, "Do Not Display Power Cost"                                             },
  { 304, "Next modal spell requires same unit target"                            },
  { 305, "AutoCast Off By Default"                                               },
  { 306, "Ignore School Lockout"                                                 },
  { 307, "Allow Dark Simulacrum"                                                 },
  { 308, "Allow Cast While Channeling"                                           },
  { 309, "Suppress Visual Kit Errors"                                            },
  { 310, "Spellcast Override In Spellbook"                                       },
  { 311, "JumpCharge - no facing control"                                        },
  { SX_DISABLE_PLAYER_HEALING_MULT, "Ignore Caster Healing Modifiers"            },  // 312
  { 313, "(Programmer Only) Don't consume charge if item deleted"                },
  { 314, "Item Passive On Client"                                                },
  { 315, "Force Corpse Target"                                                   },
  { 316, "Cannot Kill Target"                                                    },
  { 317, "Log Passive"                                                           },
  { 318, "No Movement Radius Bonus"                                              },
  { 319, "Channel Persists on Pet Follow"                                        },
  { 320, "Bypass Visibility Check"                                               },
  { SX_DISABLE_TARGET_POSITIVE_MULT, "Ignore Positive Damage Taken Modifiers"    },  // 321
  { 322, "Uses Ranged Slot (Cosmetic Only)"                                      },
  { 323, "Do Not Log Full Overheal"                                              },
  { 324, "NPC Knockback - ignore doors"                                          },
  { 325, "Force Non-Binary Resistance"                                           },
  { 326, "No Summon Log"                                                         },
  { 327, "Ignore instance lock and farm limit on teleport"                       },
  { 328, "Area Effects Use Target Radius"                                        },
  { 329, "Charge/JumpCharge - Use Absolute Speed"                                },
  { SX_TARGET_SPECIFIC_COOLDOWN, "Proc cooldown on a per target basis"           },  // 330
  { 331, "Lock chest at precast"                                                 },
  { 332, "Use Spell Base Level For Scaling"                                      },
  { 333, "Reset cooldown upon ending an encounter"                               },
  { SX_ROLLING_PERIODIC, "Rolling Periodic"                                      },  // 334
  { 335, "Spellbook Hidden Until Overridden"                                     },
  { 336, "Defend Against Friendly Cast"                                          },
  { 337, "Allow Defense While Casting"                                           },
  { 338, "Allow Defense While Channeling"                                        },
  { 339, "Allow Fatal Duel Damage"                                               },
  { 340, "Multi-Click Ground Targeting"                                          },
  { 341, "AoE Can Hit Summoned Invis"                                            },
  { 342, "Allow While Stunned By Horror Mechanic"                                },
  { 343, "Visible only to caster (conversations only)"                           },
  { 344, "Update Passives on Apply/Remove"                                       },
  { 345, "Normal Melee Attack"                                                   },
  { 346, "Ignore Feign Death"                                                    },
  { 347, "Caster Death Cancels Persistent Area Auras"                            },
  { 348, "Do Not Log Absorb"                                                     },
  { 349, "This Mount is NOT at the account level"                                },
  { 350, "Prevent Client Cast Cancel"                                            },
  { 351, "Enforce Facing on Primary Target Only"                                 },
  { 352, "Lock Caster Movement and Facing While Casting"                         },
  { 353, "Don't Cancel When All Effects are Disabled"                            },
  { SX_SCALE_ILEVEL, "Scales with Casting Item's Level"                          },  // 354
  { 355, "Do Not Log on Learn"                                                   },
  { 356, "Hide Shapeshift Requirements"                                          },
  { 357, "Absorb Falling Damage"                                                 },
  { 358, "Unbreakable Channel"                                                   },
  { 359, "Ignore Caster's spell level"                                           },
  { 360, "Transfer Mount Spell"                                                  },
  { 361, "Ignore Spellcast Override Shapeshift Requirements"                     },
  { 362, "Newest Exclusive Complete"                                             },
  { 363, "Not in Instances"                                                      },
  { 364, "Obsolete"                                                              },
  { 365, "Ignore PvP Power"                                                      },
  { 366, "Can Assist Uninteractible"                                             },
  { 367, "Cast When Initial Logging In"                                          },
  { 368, "Not in Mythic+ Mode (Challenge Mode)"                                  },
  { 369, "Cheaper NPC Knockback"                                                 },
  { 370, "Ignore Caster Absorb Modifiers"                                        },
  { 371, "Ignore Target Absorb Modifiers"                                        },
  { 372, "Hide Loss of Control UI"                                               },
  { 373, "Allow Harmful on Friendly"                                             },
  { 374, "Cheap Missile AOI"                                                     },
  { 375, "Expensive Missile AOI"                                                 },
  { 376, "No Client Fail on No Pet"                                              },
  { 377, "AI Attempt Cast on Immune Player"                                      },
  { 378, "Allow While Stunned by Stun Mechanic"                                  },
  { 379, "Don't close loot window"                                               },
  { 380, "Hide Damage Absorb UI"                                                 },
  { 381, "Do Not Treat As Area Effect"                                           },
  { 382, "Check Required Target Aura By Caster"                                  },
  { 383, "Apply Zone Aura Spell To Pets"                                         },
  { SX_ENABLE_PROCS_FROM_SUPPRESSED, "Enable Procs from Suppressed Caster Procs" },  // 384
  { SX_CAN_PROC_FROM_SUPPRESSED, "Can Proc from Suppressed Caster Procs"         },  // 385
  { 386, "Show Cooldown As Charge Up"                                            },
  { 387, "No PvP Battle Fatigue"                                                 },
  { 388, "Treat Self Cast As Reflect"                                            },
  { 389, "Do Not Cancel Area Aura on Spec Switch"                                },
  { 390, "Cooldown on Aura Cancel Until Combat Ends"                             },
  { 391, "Do Not Re-apply Area Aura if it Persists Through Update"               },
  { 392, "Display Toast Message"                                                 },
  { 393, "Active Passive"                                                        },
  { 394, "Ignore Damage Cancels Aura Interrupt"                                  },
  { 395, "Face Destination"                                                      },
  { 396, "Immunity Purges Spell"                                                 },
  { 397, "Do Not Log Spell Miss"                                                 },
  { 398, "Ignore Distance Check On Charge/Jump Charge Done Trigger Spell"        },
  { 399, "Disable known spells while charmed"                                    },
  { 400, "Ignore Damage Absorb"                                                  },
  { 401, "Not In Proving Grounds"                                                },
  { 402, "Override Default SpellClick Range"                                     },
  { 403, "Is In-Game Store Effect"                                               },
  { 404, "Allow during spell override"                                           },
  { 405, "Use float values for scaling amounts"                                  },
  { 406, "Suppress toasts on item push"                                          },
  { 407, "Trigger Cooldown On Spell Start"                                       },
  { 408, "Never Learn"                                                           },
  { 409, "No Deflect"                                                            },
  { 410, "(Deprecated) Use Start-of-Cast Location for Spell Dest"                },
  { 411, "Recompute Aura on Mercenary Mode"                                      },
  { 412, "Use Weighted Random For Flex Max Targets"                              },
  { 413, "Ignore Resilience"                                                     },
  { 414, "Apply Resilience To Self Damage"                                       },
  { SX_ONLY_PROC_FROM_CLASS_ABILITIES, "Only Proc From Class Abilities"          },  // 415
  { SX_ALLOW_CLASS_ABILITY_PROCS, "Allow Class Ability Procs"                    },  // 416
  { 417, "Allow While Feared By Fear Mechanic"                                   },
  { 418, "Cooldown Shared With AI Group"                                         },
  { 419, "Interrupts Current Cast"                                               },
  { 420, "Periodic Script Runs Late"                                             },
  { 421, "Recipe Hidden Until Known"                                             },
  { 422, "Can Proc From Lifesteal"                                               },
  { 423, "Nameplate Personal Buffs/Debuffs"                                      },
  { 424, "Cannot Lifesteal/Leech"                                                },
  { 425, "Global Aura"                                                           },
  { 426, "Nameplate Enemy Debuffs"                                               },
  { 427, "Always Allow PvP Flagged Target"                                       },
  { 428, "Do Not Consume Aura Stack On Proc"                                     },
  { 429, "Do Not PvP Flag Caster"                                                },
  { 430, "Always Require PvP Target Match"                                       },
  { 431, "Do Not Fail if No Target"                                              },
  { 432, "Displayed Outside Of Spellbook"                                        },
  { 433, "Check Phase on String ID Results"                                      },
  { 434, "Do Not Enforce Shapeshift Requirements"                                },
  { 435, "Aura Persists Through Tame Pet"                                        },
  { SX_REFRESH_EXTENDS_DURATION, "Periodic Refresh Extends Duration"             },  // 436
  { 437, "Use Skill Rank As Spell Level"                                         },
  { 438, "Aura Always Shown"                                                     },
  { 439, "Use Spell Level For Item Squish Compensation"                          },
  { 440, "Chain by Most Hit"                                                     },
  { 441, "Do Not Display Cast Time"                                              },
  { 442, "Always Allow Negative Healing Percent Modifiers"                       },
  { 443, "Do Not Allow \"Disable Movement Interrupt\""                           },
  { 444, "Allow Aura On Level Scale"                                             },
  { 445, "Remove Aura On Level Scale"                                            },
  { 446, "Recompute Aura On Level Scale"                                         },
  { 447, "Update Fall Speed After Aura Removal"                                  },
  { 448, "Prevent Jumping During Precast"                                        },
  { 449, "Reagent Consumes Charges"                                              },
  { 451, "Hide Passive From Tooltip"                                             },
  { 468, "Private Aura"                                                          },
  { SX_ASYNCRONOUS_STACKING_BUFF, "Asynchronous Buff"                            },  // 490
  { SX_IMPORTANT_SPELL, "Important Spell"                                        },  // 491
  { SX_IS_EXTERNAL_DEFENSIVE, "External Defensive"                               },  // 499
  { 506, "Non-secret Aura"                                                       },
  { 511, "Non-secret Spell"                                                      },
  { SX_IS_BIG_DEFENSIVE, "Big Defensive"                                         },  // 512
} );

static constexpr auto _aura_interrupt_strings = util::make_static_map<unsigned, std::string_view>( {
  { IX_HOSTILE_ACTION_RECEIVED,     "Hostile Action Received"          },  // 1
  { IX_DAMAGE,                      "Damage"                           },  // 2
  { IX_ACTION,                      "Action"                           },  // 3
  { IX_MOVING,                      "Moving"                           },  // 4
  { IX_TURNING,                     "Turning"                          },  // 5
  { IX_ANIMATION,                   "Animation"                        },  // 6
  { IX_DISMOUNT,                    "Dismount"                         },  // 7
  { IX_UNDER_WATER,                 "Under Water"                      },  // 8
  { IX_ABOVE_WATER,                 "Above Water"                      },  // 9
  { IX_SHEATHING,                   "Sheathing"                        },  // 10
  { IX_INTERACTIHNG,                "Interacting"                      },  // 11
  { IX_LOOTING,                     "Looting"                          },  // 12
  { IX_ATTACKING,                   "Attacking"                        },  // 13
  { IX_USE_ITEM,                    "Use Item"                         },  // 14
  { IX_DAMAGE_CHANNEL_DURATION,     "Damage Channel Duration"          },  // 15
  { IX_SHAPESHIFTING,               "Shapeshifting"                    },  // 16
  { IX_ACTION_DELAYED,              "Action (Delayed)"                 },  // 17
  { IX_MOUNT,                       "Mount"                            },  // 18
  { IX_STANDING,                    "Standing"                         },  // 19
  { IX_LEAVE_WORLD,                 "Leave World"                      },  // 20
  { IX_STEALTH_OR_INVISIBLE,        "Stealth or Invisible"             },  // 21
  { IX_INVULNERABILITY,             "Invulnerability"                  },  // 22
  { IX_ENTER_WORLD,                 "Enter World"                      },  // 23
  { IX_PVP_ACTIVE,                  "PVP Active"                       },  // 24
  { IX_DIRECT_DAMAGE,               "Direct Damage"                    },  // 25
  { IX_LANDING,                     "Landing"                          },  // 26
  { IX_RELEASE,                     "Release"                          },  // 27
  { IX_DAMAGE_SCRIPT,               "Damage (Script)"                  },  // 28
  { IX_ENTER_COMBAT,                "Enter Combat"                     },  // 29
  { IX_LOGIN,                       "Login"                            },  // 30
  { IX_SUMMON,                      "Summon"                           },  // 31
  { IX_LEAVE_COMBAT,                "Leave Combat"                     },  // 32
  { IX_FALLING,                     "Falling"                          },  // 33
  { IX_SWIMMING,                    "Swimming"                         },  // 34
  { IX_NOT_MOVING,                  "Not Moving"                       },  // 35
  { IX_GROUND,                      "Ground"                           },  // 36
  { IX_TRANSFORM,                   "Transform"                        },  // 37
  { IX_JUMP,                        "Jump"                             },  // 38
  { IX_CHANGE_SPECIALIZATION,       "Change Specialization"            },  // 39
  { IX_EXIT_VEHICLE,                "Exit Vehicle"                     },  // 40
  { IX_RAID_ENCOUNTER_START,        "Raid Encounter Start or M+ Start" },  // 41
  { IX_RAID_ENCOUNTER_END,          "Raid Encounter End or M+ Start"   },  // 42
  { IX_DISCONNECT,                  "Disconnect"                       },  // 43
  { IX_ENTER_INSTANCE,              "Enter Instance"                   },  // 44
  { IX_DUEL_END,                    "Duel End"                         },  // 45
  { IX_LEAVE_ARENA_OR_BATTLEGROUND, "Leave Arena or Battleground"      },  // 46
  { IX_CHANGE_TALENT,               "Change Talent"                    },  // 47
  { IX_CHANGE_GLYPH,                "Change Glyph"                     },  // 48
  { IX_SEAMLESS_TRANSFER,           "Seamless Transfer"                },  // 49
  { IX_LEAVE_WAR_MODE,              "Leave War Mode"                   },  // 50
  { IX_TOUCH_GROUND,                "Touch Ground"                     },  // 51
  { IX_CHROMIE_TIME,                "Chromie Time"                     },  // 52
  { IX_SPLINE_OR_FREE_FLIGHT,       "Spline or Free Flight"            },  // 53
  { IX_PROC_OR_PERIODIC_ATTACK,     "Proc or Periodic Attack"          },  // 54
  { IX_CHALLENGE_MODE_START,        "Challenge Mode Start"             },  // 55
  { IX_ENCOUNTER_START,             "Encounter Start"                  },  // 56
  { IX_ENCOUNTER_END,               "Encounter End"                    },  // 57
  { IX_RELEASE_EMPOWER,             "Release Empower"                  },  // 58
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
  { P_GENERIC,            "Spell Direct Amount"             },  // 0
  { P_DURATION,           "Spell Duration"                  },  // 1
  { P_THREAT,             "Spell Generated Threat"          },  // 2
  { P_EFFECT_1,           "Spell Effect 1"                  },  // 3
  { P_STACK,              "Spell Initial Stacks"            },  // 4
  { P_RANGE,              "Spell Range"                     },  // 5
  { P_RADIUS,             "Spell Radius"                    },  // 6
  { P_CRIT,               "Spell Critical Chance"           },  // 7
  { P_EFFECTS,            "Spell Effects"                   },  // 8
  { P_PUSHBACK,           "Spell Pushback"                  },  // 9
  { P_CAST_TIME,          "Spell Cast Time"                 },  // 10
  { P_COOLDOWN,           "Spell Cooldown"                  },  // 11
  { P_EFFECT_2,           "Spell Effect 2"                  },  // 12
  { P_RESISTANCE,         "Spell Target Resistance"         },  // 13
  { P_RESOURCE_COST_1,    "Spell Resource Cost 1"           },  // 14
  { P_CRIT_BONUS,         "Spell Critical Bonus Multiplier" },  // 15
  { P_PENETRATION,        "Spell Penetration"               },  // 16
  { P_CHAIN_TARGETS,      "Spell Chain Targets"             },  // 17
  { P_PROC_CHANCE,        "Spell Proc Chance"               },  // 18
  { P_TICK_TIME,          "Spell Tick Time"                 },  // 19
  { P_CHAIN_MULTIPLIER,   "Spell Chain Multiplier"          },  // 20
  { P_GCD,                "Spell Global Cooldown"           },  // 21
  { P_TICK_DAMAGE,        "Spell Periodic Amount"           },  // 22
  { P_EFFECT_3,           "Spell Effect 3"                  },  // 23
  { P_COEFFICIENT,        "Spell Coefficient"               },  // 24
  { P_TRIGGER_DAMAGE,     "Spell Trigger Damage"            },  // 25
  { P_PROC_FREQUENCY,     "Spell Proc Frequency"            },  // 26
  { P_DAMAGE_TAKEN,       "Spell Amplitude"                 },  // 27
  { P_DISPEL_CHANCE,      "Spell Dispel Chance"             },  // 28
  { P_CROWD,              "Spell Crowd Damage"              },  // 29
  { P_COST_ON_MISS,       "Spell Cost On Miss"              },  // 30
  { P_DOSES,              "Spell Doses"                     },  // 31
  { P_EFFECT_4,           "Spell Effect 4"                  },  // 32
  { P_EFFECT_5,           "Spell Effect 5"                  },  // 33
  { P_RESOURCE_COST_2,    "Spell Resource Cost 2"           },  // 34
  { P_CHAIN_TARGET_RANGE, "Spell Chain Target Range"        },  // 35
  { P_ARENA_MAX_SUMMONS,  "Spell Area Max Summons"          },  // 36
  { P_MAX_STACKS,         "Spell Max Stacks"                },  // 37
  { P_PROC_COOLDOWN,      "Spell Proc Cooldown"             },  // 38
  { P_RESOURCE_COST_3,    "Spell Resource Cost 3"           },  // 39
  { P_MAX_TARGETS,        "Spell Max Targets"               },  // 40
} );

static constexpr auto _pvp_property_type_strings = util::make_static_map<int, std::string_view>( {
  { P_PVP_DIRECT,   "Direct Amount"   },  // 0
  { P_PVP_PERIODIC, "Periodic Amount" },  // 1
  { P_PVP_ABSORB,   "Absorb Amount"   },  // 2
  { P_PVP_EFFECT_1, "Modify Effect 1" },  // 5
  { P_PVP_EFFECT_2, "Modify Effect 2" },  // 6
  { P_PVP_EFFECT_3, "Modify Effect 3" },  // 7
  { P_PVP_EFFECT_4, "Modify Effect 4" },  // 8
  { P_PVP_EFFECT_5, "Modify Effect 5" },  // 9
} );

static constexpr auto _effect_type_strings = util::make_static_map<unsigned, std::string_view>( {
  { E_NONE,                                            "None"                                              },  // 0
  { E_INSTAKILL,                                       "Instant Kill"                                      },  // 1
  { E_SCHOOL_DAMAGE,                                   "School Damage"                                     },  // 2
  { E_DUMMY,                                           "Dummy"                                             },  // 3
  { E_PORTAL_TELEPORT,                                 "Portal Teleport"                                   },  // 4
  { E_APPLY_AURA,                                      "Apply Aura"                                        },  // 6
  { E_ENVIRONMENTAL_DAMAGE,                            "Environmental Damage"                              },  // 7
  { E_POWER_DRAIN,                                     "Power Drain"                                       },  // 8
  { E_HEALTH_LEECH,                                    "Health Leech"                                      },  // 9
  { E_HEAL,                                            "Direct Heal"                                       },  // 10
  { E_BIND,                                            "Bind"                                              },  // 11
  { E_PORTAL,                                          "Portal"                                            },  // 12
  { E_TELEPORT_TO_RETURN_POINT,                        "Teleport to Return Point"                          },  // 13
  { E_INCREASE_CURRENCY_CAP,                           "Increase Currency Cap"                             },  // 14
  { E_TELEPORT_WITH_SPELL_VISUAL_KIT_LOADING_SCREEN,   "Teleport w/ Loading Screen"                        },  // 15
  { E_QUEST_COMPLETE,                                  "Quest Complete"                                    },  // 16
  { E_WEAPON_DAMAGE_NOSCHOOL,                          "Weapon Damage"                                     },  // 17
  { E_RESURRECT,                                       "Resurrect"                                         },  // 18
  { E_ADD_EXTRA_ATTACKS,                               "Extra Attacks"                                     },  // 19
  { E_DODGE,                                           "Dodge"                                             },  // 20
  { E_EVADE,                                           "Evade"                                             },  // 21
  { E_PARRY,                                           "Parry"                                             },  // 22
  { E_BLOCK,                                           "Block"                                             },  // 23
  { E_CREATE_ITEM,                                     "Create Item"                                       },  // 24
  { E_WEAPON,                                          "Weapon Type"                                       },  // 25
  { E_DEFENSE,                                         "Defense"                                           },  // 26
  { E_PERSISTENT_AREA_AURA,                            "Apply Aura in Area"                                },  // 27
  { E_SUMMON,                                          "Summon Guardian"                                   },  // 28
  { E_LEAP,                                            "Leap"                                              },  // 29
  { E_ENERGIZE,                                        "Energize Power"                                    },  // 30
  { E_WEAPON_PERCENT_DAMAGE,                           "Weapon Damage%"                                    },  // 31
  { E_TRIGGER_MISSILE,                                 "Trigger Missiles"                                  },  // 32
  { E_OPEN_LOCK,                                       "Open Lock"                                         },  // 33
  { E_SUMMON_CHANGE_ITEM,                              "Summon Item"                                       },  // 34
  { E_APPLY_AREA_AURA_PARTY,                           "Apply Party Aura"                                  },  // 35
  { E_LEARN_SPELL,                                     "Learn Spell"                                       },  // 36
  { E_SPELL_DEFENSE,                                   "Spell Defense"                                     },  // 37
  { E_DISPEL,                                          "Dispel"                                            },  // 38
  { E_LANGUAGE,                                        "Language"                                          },  // 39
  { E_DUAL_WIELD,                                      "Dual Wield"                                        },  // 40
  { E_JUMP,                                            "Jump"                                              },  // 41
  { E_JUMP_DEST,                                       "Jump Dest"                                         },  // 42
  { E_TELEPORT_UNITS_FACE_CASTER,                      "Teleport Unit Facing Caster"                       },  // 43
  { E_SKILL_STEP,                                      "Skill Step"                                        },  // 44
  { E_PLAY_MOVIE,                                      "Play Movie"                                        },  // 45
  { E_SPAWN,                                           "Spawn"                                             },  // 46
  { E_TRADE_SKILL,                                     "Trade Skill"                                       },  // 47
  { E_STEALTH,                                         "Stealth"                                           },  // 48
  { E_DETECT,                                          "Detect"                                            },  // 49
  { E_TRANS_DOOR,                                      "Transition through Door"                           },  // 50
  { E_FORCE_CRITICAL_HIT,                              "Force Crit"                                        },  // 51
  { E_SET_MAX_BATTLEPET_COUNT,                         "Guaranteed Hit"                                    },  // 52
  { E_ENCHANT_ITEM,                                    "Enchant Item"                                      },  // 53
  { E_ENCHANT_ITEM_TEMPORARY,                          "Enchant Item Temporary"                            },  // 54
  { E_TAMECREATURE,                                    "Tame Creature"                                     },  // 55
  { E_SUMMON_PET,                                      "Summon Pet"                                        },  // 56
  { E_LEARN_PET_SPELL,                                 "Learn Pet Spell"                                   },  // 57
  { E_WEAPON_DAMAGE,                                   "Weapon Damage"                                     },  // 58
  { E_CREATE_RANDOM_ITEM,                              "Create Random Item"                                },  // 59
  { E_PROFICIENCY,                                     "Proficiency"                                       },  // 60
  { E_SEND_EVENT,                                      "Send Event"                                        },  // 61
  { E_POWER_BURN,                                      "Power Burn"                                        },  // 62
  { E_THREAT,                                          "Threat"                                            },  // 63
  { E_TRIGGER_SPELL,                                   "Trigger Spell"                                     },  // 64
  { E_APPLY_AREA_AURA_RAID,                            "Apply Aura Raid"                                   },  // 65
  { E_RESTORE_ITEM_CHARGES,                            "Recharge Item"                                     },  // 66
  { E_HEAL_MAX_HEALTH,                                 "Heal Max Health%"                                  },  // 67
  { E_INTERRUPT_CAST,                                  "Interrupt Cast"                                    },  // 68
  { E_DISTRACT,                                        "Distract"                                          },  // 69
  { E_COMPLETE_WORLD_QUEST,                            "Complete World Quest"                              },  // 70
  { E_PICKPOCKET,                                      "Pick Pocket"                                       },  // 71
  { E_ADD_FARSIGHT,                                    "Add Farsight"                                      },  // 72
  { E_UNTRAIN_TALENTS,                                 "Unlearn Talent"                                    },  // 73
  { E_APPLY_GLYPH,                                     "Apply Glyph"                                       },  // 74
  { E_HEAL_MECHANICAL,                                 "Heal Mechanical"                                   },  // 75
  { E_SUMMON_OBJECT_WILD,                              "Summon Object - Wild"                              },  // 76
  { E_SCRIPT_EFFECT,                                   "Server Side Script"                                },  // 77
  { E_ATTACK,                                          "Attack"                                            },  // 78
  { E_SANCTUARY,                                       "Sanctuary"                                         },  // 79
  { E_MODIFY_FOLLOWER_ITEM_LEVEL,                      "Modify Follower Item Level"                        },  // 80
  { E_PUSH_ABILITY_TO_ACTION_BAR,                      "Push Ability to Action Bar"                        },  // 81
  { E_BIND_SIGHT,                                      "Bind Sight"                                        },  // 82
  { E_DUEL,                                            "Duel"                                              },  // 83
  { E_STUCK,                                           "Stuck"                                             },  // 84
  { E_SUMMON_PLAYER,                                   "Summon Player"                                     },  // 85
  { E_ACTIVATE_OBJECT,                                 "Activate Object"                                   },  // 86
  { E_WMO_DAMAGE,                                      "Damage Gameobject"                                 },  // 87
  { E_WMO_REPAIR,                                      "Repair Gameobject"                                 },  // 88
  { E_WMO_CHANGE,                                      "Set Gameobject Destruction State"                  },  // 89
  { E_SUMMON_CREDIT_CREATURE,                          "Summon Credit Creature"                            },  // 90
  { E_THREAT_ALL,                                      "Threat All"                                        },  // 91
  { E_ENCHANT_HELD_ITEM,                               "Enchant Held Item"                                 },  // 92
  { E_BREAK_PLAYER_TARGETING,                          "Force Untarget"                                    },  // 93
  { E_SELF_RESURRECT,                                  "Self Resurrect"                                    },  // 94
  { E_SKINNING,                                        "Skinning"                                          },  // 95
  { E_CHARGE,                                          "Charge"                                            },  // 96
  { E_SUMMON_ALL_TOTEMS,                               "Summon All Totems"                                 },  // 97
  { E_KNOCK_BACK,                                      "Knock Back"                                        },  // 98
  { E_DISENCHANT,                                      "Disenchant"                                        },  // 99
  { E_INEBRIATE,                                       "Inebriate"                                         },  // 100
  { E_FEED_PET,                                        "Feed Pet"                                          },  // 101
  { E_DISMISS_PET,                                     "Dismiss Pet"                                       },  // 102
  { E_REPUTATION,                                      "Reputation"                                        },  // 103
  { E_SUMMON_OBJECT_SLOT1,                             "Summon Object"                                     },  // 104
  { E_SURVEY,                                          "Survey"                                            },  // 105
  { E_CHANGE_RAID_MARKER,                              "Change Raid Marker"                                },  // 106
  { E_SHOW_CORPSE_LOOT,                                "Show Corpse Loot"                                  },  // 107
  { E_DISPEL_MECHANIC,                                 "Dispel Mechanic"                                   },  // 108
  { E_SUMMON_DEAD_PET,                                 "Summon Dead Pet"                                   },  // 109
  { E_DESTROY_ALL_TOTEMS,                              "Destroy All Totems"                                },  // 110
  { E_DURABILITY_DAMAGE,                               "Durability Damage"                                 },  // 111
  { E_CANCEL_CONVERSATION,                             "Cancel Conversation"                               },  // 113
  { E_ATTACK_ME,                                       "Taunt"                                             },  // 114
  { E_DURABILITY_DAMAGE_PCT,                           "Durability Damage%"                                },  // 115
  { E_SKIN_PLAYER_CORPSE,                              "Skin Player Corpse"                                },  // 116
  { E_SPIRIT_HEAL,                                     "Spirit Heal"                                       },  // 117
  { E_SKILL,                                           "Skill"                                             },  // 118
  { E_APPLY_AREA_AURA_PET,                             "Apply Aura Pet in Area"                            },  // 119
  { E_TELEPORT_GRAVEYARD,                              "Teleport to Graveyard"                             },  // 120
  { E_NORMALIZED_WEAPON_DMG,                           "Normalized Weapon Damage"                          },  // 121
  { E_SEND_TAXI,                                       "Send Taxi"                                         },  // 123
  { E_PLAYER_PULL,                                     "Pull Player"                                       },  // 124
  { E_MODIFY_THREAT_PERCENT,                           "Modify Threat"                                     },  // 125
  { E_STEAL_BENEFICIAL_BUFF,                           "Steal Beneficial Aura"                             },  // 126
  { E_PROSPECTING,                                     "Prospect"                                          },  // 127
  { E_APPLY_AREA_AURA_FRIEND,                          "Apply Aura Friendly in Area"                       },  // 128
  { E_APPLY_AREA_AURA_ENEMY,                           "Apply Aura Enemy in Area"                          },  // 129
  { E_REDIRECT_THREAT,                                 "Redirect Threat"                                   },  // 130
  { E_PLAY_SOUND,                                      "Play Sound"                                        },  // 131
  { E_PLAY_MUSIC,                                      "Play Music"                                        },  // 132
  { E_UNLEARN_SPECIALIZATION,                          "Unlearn Profession Spec"                           },  // 133
  { E_KILL_CREDIT2,                                    "Kill Credit"                                       },  // 134
  { E_CALL_PET,                                        "Call Pet"                                          },  // 135
  { E_HEAL_PCT,                                        "Direct Heal%"                                      },  // 136
  { E_ENERGIZE_PCT,                                    "Energize Power%"                                   },  // 137
  { E_LEAP_BACK,                                       "Directional Knock"                                 },  // 138
  { E_CLEAR_QUEST,                                     "Clear Quest"                                       },  // 139
  { E_FORCE_CAST,                                      "Force Cast"                                        },  // 140
  { E_FORCE_CAST_WITH_VALUE,                           "Force Cast w/ Value"                               },  // 141
  { E_TRIGGER_SPELL_WITH_VALUE,                        "Trigger Spell w/ Value"                            },  // 142
  { E_APPLY_AREA_AURA_OWNER,                           "Apply Aura Owner in Area"                          },  // 143
  { E_KNOCK_BACK_DESTINATION,                          "Directional Knockback"                             },  // 144
  { E_PULL_TOWARDS_DESTIANTION,                        "Pull to Destination"                               },  // 145
  { E_RESTORE_GARRISON_TROOP_VITALITY,                 "Restore Garrison Troop Vitality"                   },  // 146
  { E_QUEST_FAIL,                                      "Fail Quest"                                        },  // 147
  { E_TRIGGER_MISSILE_SPELL_WITH_VALUE,                "Trigger Missile Spell w/ Value"                    },  // 148
  { E_CHARGE_TO_DESTINATION,                           "Charge to Destination"                             },  // 149
  { E_QUEST_START,                                     "Start Quest"                                       },  // 150
  { E_TRIGGER_SPELL_2,                                 "Trigger Spell"                                     },  // 151
  { E_SUMMON_RAF_FRIEND,                               "Summon Refer a Friend"                             },  // 152
  { E_CREATE_TAMED_PET,                                "Create Tamed Pet"                                  },  // 153
  { E_TEACH_TAXI_NODE,                                 "Discover Taxi Location"                            },  // 154
  { E_TITAN_GRIP,                                      "Titan Grip"                                        },  // 155
  { E_ENCHANT_ITEM_PRISMATIC,                          "Add Prismatic Socket"                              },  // 156
  { E_CREATE_ITEM_2,                                   "Create Item"                                       },  // 157
  { E_MILLING,                                         "Mill"                                              },  // 158
  { E_ALLOW_RENAME_PET,                                "Allow Rename Pet"                                  },  // 159
  { E_FORCE_CAST_2,                                    "Force Cast"                                        },  // 160
  { E_TALENT_SPEC_COUNT,                               "Learn/Unlearn Secondary Spec"                      },  // 161
  { E_TALENT_SPEC_SELECT,                              "Activate Specialization"                           },  // 162
  { E_OBLITERATE_ITEM,                                 "Obliterate Item"                                   },  // 163
  { E_CANCEL_AURA,                                     "Cancel Aura"                                       },  // 164
  { E_DAMAGE_TAKEN_PCT_MAX_HEALTH,                     "Take Max Health% Damage"                           },  // 165
  { E_GIVE_CURRENCY,                                   "Give Currency"                                     },  // 166
  { E_UPDATE_PLAYER_PHASE,                             "Update Player Phase"                               },  // 167
  { E_ALLOW_CONTROL_PET,                               "Allow Control Pet"                                 },  // 168
  { E_DESTORY_ITEM,                                    "Destroy Item"                                      },  // 169
  { E_UPDATE_ZONE_AURAS_AND_PHASES,                    "Update Zone Auras and Phases"                      },  // 170
  { E_SUMMON_PERSONAL_GAMEOBJECT,                      "Summon Personal Gameobject"                        },  // 171
  { E_RESURRECT_WITH_AURA,                             "Resurrect with Health%"                            },  // 172
  { E_UNLOCK_GUILD_VAULT_TAB,                          "Unlock Guild Vault Tab"                            },  // 173
  { E_APPLY_AURA_PET,                                  "Apply Aura Pet"                                    },  // 174
  { E_SANCTUARY_2,                                     "Sanctuary"                                         },  // 176
  { E_DESPAWN_PERSISTENT_AREA_AURA,                    "Despawn Persistent Area Aura"                      },  // 177
  { E_CREATE_AREA_TRIGGER,                             "Create Area Trigger"                               },  // 179
  { E_UPDATE_AREA_TRIGGER,                             "Update Area Trigger"                               },  // 180
  { E_REMOVE_TALENT,                                   "Remove Talent"                                     },  // 181
  { E_DESPAWN_AREA_TRIGGER,                            "Despawn Area Trigger"                              },  // 182
  { E_REPUTATION_2,                                    "Reputation"                                        },  // 184
  { E_RANDOMIZE_ARCHAEOLOGY_DIGSITES,                  "Randomize Archaeology Digsite"                     },  // 187
  { E_SUMMON_STABLED_PET_AS_GUARDIAN,                  "Summon Multiple Hunter Pets"                       },  // 188
  { E_LOOT,                                            "Loot"                                              },  // 189
  { E_CHANGE_PARTY_MEMBERS,                            "Change Party Member"                               },  // 190
  { E_TELEPORT_TO_DIGSITE,                             "Teleport to Digsite"                               },  // 191
  { E_UNCAGE_BATTLEPET,                                "Uncage Battlepet"                                  },  // 192
  { E_START_PET_BATTLE,                                "Start Pet Battlle"                                 },  // 193
  { E_PLAY_SCENE_SCRIPT_PACKAGE,                       "Play Scene Script Package"                         },  // 195
  { E_CREATE_SCENE_OBJECT,                             "Create Scene Object"                               },  // 196
  { E_CREATE_PERSONAL_SCENE_OBJECT,                    "Create Personal Scene Object"                      },  // 197
  { E_PLAY_SCENE,                                      "Play Scene"                                        },  // 198
  { E_DESPAWN_SUMMON,                                  "Despawn Summon"                                    },  // 199
  { E_HEAL_BATTLEPET_PCT,                              "Battlepet Heal%"                                   },  // 200
  { E_ENABLE_BATTLE_PETS,                              "Enable Pet Battles"                                },  // 201
  { E_APPLY_AURA_PLAYER_AND_PET,                       "Apply Player/Pet Aura"                             },  // 202
  { E_REMOVE_AURA_2,                                   "Remove Aura"                                       },  // 203
  { E_CHANGE_BATTLEPET_QUALITY,                        "Change Battlepet Quality"                          },  // 204
  { E_LAUNCH_QUEST_CHOICE,                             "Launch Quest Choice"                               },  // 205
  { E_ALTER_ITEM,                                      "Alter Item"                                        },  // 206
  { E_LAUNCH_QUEST_TASK,                               "Launch Quest Task"                                 },  // 207
  { E_SET_REPUTATION,                                  "Set Reputation"                                    },  // 208
  { E_LEARN_GARRISON_BUILDING,                         "Learn Garrison Building"                           },  // 210
  { E_LEARN_GARRISON_SPECIALIZATION,                   "Learn Garrison Specialization"                     },  // 211
  { E_REMOVE_AURA_BY_SPELL_LABEL,                      "Remove Aura (Label)"                               },  // 212
  { E_JUMP_TO_DESTINATION_2,                           "Jump to Destiantion"                               },  // 213
  { E_CREATE_GARRISON,                                 "Create Garrison"                                   },  // 214
  { E_UPGRADE_CHARACTER_SPELLS,                        "Upgrade Character Spells"                          },  // 215
  { E_CREATE_SHIPMENT,                                 "Create Shipment"                                   },  // 216
  { E_UPGRADE_GARRISON,                                "Upgrade Garrison"                                  },  // 217
  { E_CREATE_CONVERSATION,                             "Create Conversation"                               },  // 219
  { E_ADD_GARRISON_FOLLOWER,                           "Add Garrison Follower"                             },  // 220
  { E_ADD_GARRISON_MISSION,                            "Add Garrison Mission"                              },  // 221
  { E_CREATE_HEIRLOOM_ITEM,                            "Create Heirloom Item"                              },  // 222
  { E_CHANGE_ITEM_BONUSES,                             "Change Item Bonuses"                               },  // 223
  { E_ACTIVATE_GARRISON_BUILDING,                      "Activate Garrison Building"                        },  // 224
  { E_GRANT_BATTLEPET_LEVEL,                           "Grant Battlepet Level"                             },  // 225
  { E_TRIGGER_ACTION_SET,                              "Trigger Action Set"                                },  // 226
  { E_TELEPORT_TO_LFG_DUNGEON,                         "Teleport to LFG Dungeon"                           },  // 227
  { E_SET_FOLLOWER_QUALITY,                            "Set Follower Quality"                              },  // 229
  { E_INCREASE_FOLLOWER_EXPERIENCE,                    "Increase Follower Experience"                      },  // 231
  { E_REMOVE_PHASE,                                    "Remove Phase"                                      },  // 232
  { E_RANDOMIZE_FOLLOWER_ABILITIES,                    "Randomize Follower Abilities"                      },  // 233
  { E_GIVE_EXPERIENCE,                                 "Give Experience"                                   },  // 236
  { E_GIVE_RESTED_EXPERIENCE_BONUS,                    "Give Rested Experience Bonus"                      },  // 237
  { E_INCREASE_SKILL,                                  "Increase Skill"                                    },  // 238
  { E_END_GARRISON_BUILDING_CONSTRUCTION,              "Finish Garrison Building Construction"             },  // 239
  { E_GIVE_ARTIFACT_POWER,                             "Give Artifact Power"                               },  // 240
  { E_GIVE_ARTIFACT_POWER_NO_BONUS,                    "Give Artifact Power (No Bonus)"                    },  // 242
  { E_APPLY_ENCHANT_ILLUSION,                          "Apply Enchant Illusion"                            },  // 243
  { E_LEARN_FOLLOWER_ABILITY,                          "Learn Follower Ability"                            },  // 244
  { E_UPGRADE_HEIRLOOM,                                "Upgrade Heirloom"                                  },  // 245
  { E_FINISH_GARRISON_MISSION,                         "Finish Garrison Mission"                           },  // 246
  { E_ADD_GARRISON_MISSION_SET,                        "Add Garrison Mission Set"                          },  // 247
  { E_FINISH_SHIPMENT,                                 "Finish Shipment"                                   },  // 248
  { E_FORCE_EQUIP_ITEM,                                "Force Equip Item"                                  },  // 249
  { E_TAKE_SCREENSHOT,                                 "Take Screenshot"                                   },  // 250
  { E_SET_GARRISON_CACHE_SIZE,                         "Set Garrison Cache Size"                           },  // 251
  { E_TELEPORT_UNITS,                                  "Teleport Units"                                    },  // 252
  { E_GIVE_HONOR,                                      "Give Honor"                                        },  // 253
  { E_JUMP_CHARGE,                                     "Jump Charge"                                       },  // 254
  { E_LEARN_TRANSMOG_SET,                              "Learn Transmog Set"                                },  // 255
  { E_MODIFY_KEYSTONE,                                 "Modify Keystone"                                   },  // 258
  { E_RESPEC_AZERITE_EMPOWERED_ITEM,                   "Respect Azerite Item"                              },  // 259
  { E_SUMMON_STABLED_PET,                              "Summon Stabled Pet"                                },  // 260
  { E_SCRAP_ITEM,                                      "Scrap Item"                                        },  // 261
  { E_REPAIR_ITEM,                                     "Repair Item"                                       },  // 263
  { E_REMOVE_GEM,                                      "Remove Gem"                                        },  // 264
  { E_LEARN_AZERITE_ESSENCE_POWER,                     "Learn Azerite Essence Power"                       },  // 265
  { E_SET_ITEM_BONUS_LIST_GROUP_ENTRY,                 "Set Item Bonus List Group Entry"                   },  // 266
  { E_CREATE_PRIVATE_CONVERSATION,                     "Create Private Conversation"                       },  // 267
  { E_APPLY_MOUNT_EQUIPMENT,                           "Apply Mount Equipment"                             },  // 268
  { E_INCREASE_ITEM_BONUS_LIST_GROUP_STEP,             "Increase Item Bonus List Group Step"               },  // 269
  { E_APPLY_AREA_AURA_PARTY_NONRANDOM,                 "Apply Aura Party (Non-Random)"                     },  // 271
  { E_SET_COVENANT,                                    "Set Covenant"                                      },  // 272
  { E_CRAFT_RUNEFORGE_LEGENDARY,                       "Craft Runeforge Legendary"                         },  // 273
  { E_LEARN_TRANSMOG_ILLUSION,                         "Learn Transmog Illusion"                           },  // 276
  { E_SET_CHROMIE_TIME,                                "Set Chromie Time"                                  },  // 277
  { E_LEARN_GARRISON_TALENT,                           "Learn Garrison Talent"                             },  // 279
  { E_LEARN_SOULBIND_CONDUIT,                          "Learn Soulbind Conduit"                            },  // 281
  { E_CONVERT_ITEMS_TO_CURRENCY,                       "Convert Items to Currency"                         },  // 282
  { E_COMPLETE_CAMPAIGN,                               "Complete Campaign"                                 },  // 283
  { E_SEND_CHAT_MESSAGE,                               "Send Chat Message"                                 },  // 284
  { E_MODIFY_KEYSTONE_2,                               "Modify Keystone"                                   },  // 285
  { E_GRANT_BATTLEPET_EXPERIENCE,                      "Grant Battlepet Experience"                        },  // 286
  { E_SET_GARRISON_FOLLOWER_LEVEL,                     "Set Garrison Follower Level"                       },  // 287
  { E_CRAFT_ITEM,                                      "Craft Item"                                        },  // 288
  { E_MODIFY_AURA_STACKS,                              "Modify Aura Stacks"                                },  // 289
  { E_REDUCE_REMAINING_COOLDOWN,                       "Reduce Remaining Cooldown"                         },  // 290
  { E_MODIFY_COOLDOWN,                                 "Modify Cooldown"                                   },  // 291
  { E_MODIFY_COOLDOWN_IN_CATEGORY,                     "Modify Cooldown (Category)"                        },  // 292
  { E_RECHARGE_CATEGORY_COOLDOWN_IMMEDIATE,            "Immediate Cooldown Recharge (Category)"            },  // 293
  { E_CRAFT_LOOT,                                      "Craft Loot"                                        },  // 294
  { E_SALVAGE_ITEM,                                    "Salvage Item"                                      },  // 295
  { E_CRAFT_SALVAGE_ITEM,                              "Craft Salvage Item"                                },  // 296
  { E_RECRAFT_ITEM,                                    "Recraft Item"                                      },  // 297
  { E_CANCEL_ALL_PRIVATE_CONVERSATIONS,                "Cancel All Private Conversations"                  },  // 298
  { E_CRAFT_ENCHANTMENT,                               "Craft Enchantment"                                 },  // 301
  { E_GATHERING,                                       "Gather"                                            },  // 302
  { E_CREATE_TRAIT_TREE_CONFIG,                        "Create Trait Tree Config"                          },  // 303
  { E_CHANGE_ACTIVE_COMBAT_TRAIT_CONFIG,               "Change Active Combat Triat Config"                 },  // 304
  { E_UPDATE_INTERACTIONS,                             "Update Interactions"                               },  // 306
  { E_CANCEL_PRELOAD_WORLD,                            "Cancel World Preload"                              },  // 308
  { E_PRELOAD_WORLD,                                   "Preload World"                                     },  // 309
  { E_ENSURE_WORLD_LOADED,                             "Check World Loaded"                                },  // 311
  { E_CHANGE_ITEM_BONUSES_2,                           "Change Item Bonuses"                               },  // 313
  { E_ADD_SOCKET_BONUS,                                "Add Socket Bonus"                                  },  // 314
  { E_LEARN_APPEARANCE_FROM_GROUP,                     "Learn Appearance from Item Mod Appearance Group"   },  // 315
  { E_KILL_CREDIT_LABEL_1,                             "Kill Credit (Label)"                               },  // 316
  { E_KILL_CREDIT_LABEL_2,                             "Kill Credit (Label)"                               },  // 317
  { E_SET_PLAYER_DATA_ELEMENT_ACCOUNT,                 "Set Player Data Element (Account)"                 },  // 335
  { E_SET_PLAYER_DATA_ELEMENT_CHARACTER,               "Set Player Data Element (Character)"               },  // 336
  { E_SET_PLAYER_DATA_FLAG_ACCOUNT,                    "Set Player Data Flag (Account)"                    },  // 337
  { E_SET_PLAYER_DATA_FLAG_CHARACTER,                  "Set Player Data Flag (Character)"                  },  // 338
  { E_UI_ACTION,                                       "UI Action"                                         },  // 339
  { E_LEARN_WARBAND_SCENE,                             "Learn Warband Scene"                               },  // 341
  { E_ASSIST_ACTION,                                   "Assist Action"                                     },  // 345
  { E_EQUIP_TRANSMOG_OUTFIT,                           "Equip Transmog Outfit"                             },  // 347
  { E_GIVE_HOUSE_LEVEL,                                "Give House Level"                                  },  // 348
  { E_LEARN_HOUSING_INTERIOR,                          "Learn Housing Interior"                            },  // 349
  { E_LEARN_HOUSING_EXTERIOR,                          "Learn Housing Exterior"                            },  // 350
  { E_LEARN_HOUSE_THEME,                               "Learn House Theme"                                 },  // 351
  { E_LEARN_HOUSING_COMPONENT_TEXTURE,                 "Learn Housing Component Texture"                   },  // 352
  { E_CREATE_AREA_TRIGGER_2,                           "Create Area Trigger"                               },  // 353
  { E_SET_NEIGHBORHOOD_INITIATIVE,                     "Set Neighborhood Initiative"                       },  // 354
  { E_APPLY_ITEM_BONUS,                                "Apply Item Bonus"                                  },  // 357
  { E_REMOVE_ITEM_BONUS,                               "Remove Item Bonus"                                 },  // 358
  { E_APPLY_ITEM_CONDITION,                            "Apply Item Condition"                              },  // 359
} );

static constexpr auto _effect_subtype_strings = util::make_static_map<unsigned, std::string_view>( {
  { A_NONE,                                  "None"                                              },  // 0
  { A_MOD_POSSESS,                           "Possess"                                           },  // 2
  { A_PERIODIC_DAMAGE,                       "Periodic Damage"                                   },  // 3
  { A_DUMMY,                                 "Dummy"                                             },  // 4
  { A_MOD_CONFUSE,                           "Confuse"                                           },  // 5
  { A_MOD_CHARM,                             "Charm"                                             },  // 6
  { A_MOD_FEAR,                              "Fear"                                              },  // 7
  { A_PERIODIC_HEAL,                         "Periodic Heal"                                     },  // 8
  { A_MOD_ATTACKSPEED_NORMALIZED,            "Auto Attack Speed (Normalized wDPS)"               },  // 9
  { A_MOD_THREAT,                            "Threat"                                            },  // 10
  { A_MOD_TAUNT,                             "Taunt"                                             },  // 11
  { A_MOD_STUN,                              "Stun"                                              },  // 12
  { A_MOD_DAMAGE_DONE,                       "Damage Done"                                       },  // 13
  { A_MOD_DAMAGE_TAKEN,                      "Damage Taken"                                      },  // 14
  { A_DAMAGE_SHIELD,                         "Damage Shield"                                     },  // 15
  { A_MOD_STEALTH,                           "Stealth"                                           },  // 16
  { A_MOD_STEALTH_DETECT,                    "Stealth Detection"                                 },  // 17
  { A_MOD_INVISIBILITY,                      "Invisibility"                                      },  // 18
  { A_MOD_INVISIBILITY_DETECTION,            "Invisibility Detection"                            },  // 19
  { A_PERIODIC_HEAL_PCT,                     "Periodic Heal%"                                    },  // 20
  { A_OBS_MOD_MANA,                          "Periodic Power% Regen"                             },  // 21
  { A_MOD_RESISTANCE,                        "Resistance"                                        },  // 22
  { A_PERIODIC_TRIGGER_SPELL,                "Periodic Trigger Spell"                            },  // 23
  { A_PERIODIC_ENERGIZE,                     "Periodic Energize Power"                           },  // 24
  { A_MOD_PACIFY,                            "Pacify"                                            },  // 25
  { A_MOD_ROOT,                              "Root"                                              },  // 26
  { A_MOD_SILENCE,                           "Silence"                                           },  // 27
  { A_REFLECT_SPELLS,                        "Spell Reflection"                                  },  // 28
  { A_MOD_STAT,                              "Attribute"                                         },  // 29
  { A_MOD_SKILL,                             "Skill"                                             },  // 30
  { A_MOD_INCREASE_SPEED,                    "Increase Speed%"                                   },  // 31
  { A_MOD_INCREASE_MOUNTED_SPEED,            "Increase Mounted Speed%"                           },  // 32
  { A_MOD_DECREASE_SPEED,                    "Decrease Movement Speed%"                          },  // 33
  { A_MOD_INCREASE_HEALTH,                   "Increase Health"                                   },  // 34
  { A_MOD_INCREASE_RESOURCE,                 "Increase Resource"                                 },  // 35
  { A_MOD_SHAPESHIFT,                        "Shapeshift"                                        },  // 36
  { A_EFFECT_IMMUNITY,                       "Immunity Against External Movement"                },  // 37
  { A_SCHOOL_IMMUNITY,                       "School Immunity"                                   },  // 39
  { A_DAMAGE_IMMUNITY,                       "Damage Immunity"                                   },  // 40
  { A_DISPEL_IMMUNITY,                       "Disable Stealth"                                   },  // 41
  { A_PROC_TRIGGER_SPELL,                    "Proc Trigger Spell"                                },  // 42
  { A_PROC_TRIGGER_DAMAGE,                   "Proc Trigger Damage"                               },  // 43
  { A_TRACK_CREATURES,                       "Track Creatures"                                   },  // 44
  { A_MOD_PARRY_PERCENT,                     "Modify Parry%"                                     },  // 47
  { A_MOD_DODGE_PERCENT,                     "Modify Dodge%"                                     },  // 49
  { A_MOD_CRITICAL_HEALING_AMOUNT,           "Modify Critical Heal Bonus"                        },  // 50
  { A_MOD_BLOCK_PERCENT,                     "Modify Block%"                                     },  // 51
  { A_MOD_CRIT_PERCENT,                      "Modify Crit%"                                      },  // 52
  { A_PERIODIC_LEECH,                        "Periodic Health Leech"                             },  // 53
  { A_MOD_HIT_CHANCE,                        "Modify Hit%"                                       },  // 54
  { A_MOD_SPELL_HIT_CHANCE,                  "Modify Spell Hit%"                                 },  // 55
  { A_TRANSFORM,                             "Change Model"                                      },  // 56
  { A_MOD_SPELL_CRIT_CHANCE,                 "Modify Spell Crit%"                                },  // 57
  { A_MOD_DAMAGE_DONE_CREATURE,              "Modify Damage done to Creature Type"               },  // 59
  { A_MOD_PACIFY_SILENCE,                    "Pacify Silence"                                    },  // 60
  { A_MOD_SCALE,                             "Scale% (Stacking)"                                 },  // 61
  { A_MOD_MAX_RESOURCE_COST,                 "Modify Max Cost"                                   },  // 63
  { A_PERIODIC_MANA_LEECH,                   "Periodic Mana Leech"                               },  // 64
  { A_MOD_CASTING_SPEED_NOT_STACK,           "Modify Spell Speed%"                               },  // 65
  { A_FEIGN_DEATH,                           "Feign Death"                                       },  // 66
  { A_MOD_DISARM,                            "Disarm"                                            },  // 67
  { A_MOD_STALKED,                           "Stalked"                                           },  // 68
  { A_SCHOOL_ABSORB,                         "Absorb Damage"                                     },  // 69
  { A_MOD_POWER_COST_SCHOOL_PCT,             "Modify Power Cost%"                                },  // 72
  { A_MOD_POWER_COST_SCHOOL,                 "Modify Power Cost"                                 },  // 73
  { A_REFLECT_SPELLS_SCHOOL,                 "Reflect Spells"                                    },  // 74
  { A_MECHANIC_IMMUNITY,                     "Mechanic Immunity"                                 },  // 77
  { A_MOD_DAMAGE_PERCENT_DONE,               "Modify Damage Done%"                               },  // 79
  { A_MOD_PERCENT_STAT,                      "Modify Base Attribute%"                            },  // 80
  { A_SPLIT_DAMAGE_PCT,                      "Transfer Damage%"                                  },  // 81
  { A_RESTORE_HEALTH,                        "Restore Health"                                    },  // 84
  { A_RESTORE_POWER,                         "Restore Power"                                     },  // 85
  { A_MOD_DAMAGE_PERCENT_TAKEN,              "Modify Damage Taken%"                              },  // 87
  { A_MOD_HEALTH_REGEN_PERCENT,              "Modify Health Regeneration%"                       },  // 88
  { A_PERIODIC_DAMAGE_PERCENT,               "Periodic Max Health% Damage"                       },  // 89
  { A_INTERRUPT_REGEN,                       "Interrupt Health Regen"                            },  // 94
  { A_MOD_ATTACK_POWER,                      "Modify Attack Power"                               },  // 99
  { A_MOD_RESISTANCE_PCT,                    "Modify Armor%"                                     },  // 101
  { A_MOD_MELEE_ATTACK_POWER_VERSUS,         "Modify Melee Attack Power vs Race"                 },  // 102
  { A_MOD_TOTAL_THREAT,                      "Temporary Threat Reduction"                        },  // 103
  { A_WATER_WALK,                            "Modify Attack Power"                               },  // 104
  { A_HOVER,                                 "Levitate"                                          },  // 106
  { A_ADD_FLAT_MODIFIER,                     "Add Flat Modifier"                                 },  // 107
  { A_ADD_PCT_MODIFIER,                      "Add Percent Modifier"                              },  // 108
  { A_MOD_POWER_REGEN_PERCENT,               "Modify Power Regen"                                },  // 110
  { A_MOD_HEALING,                           "Modify Healing Received"                           },  // 115
  { A_MOD_REGEN_DURING_COMBAT,               "Combat Health Regen%"                              },  // 116
  { A_MOD_MECHANIC_RESISTANCE,               "Mechanic Resistance"                               },  // 117
  { A_MOD_HEALING_RECEIVED_PCT,              "Modify Healing Received%"                          },  // 118
  { A_CHECK_PVP_STATE,                       "Check PVP State"                                   },  // 119
  { A_MOD_TARGET_RESISTANCE,                 "Modify Target Resistance"                          },  // 123
  { A_MOD_RANGED_ATTACK_POWER,               "Modify Ranged Attack Power"                        },  // 124
  { A_MOD_MELEE_DAMAGE_TAKEN_PCT,            "Modify Melee Damage Taken%"                        },  // 126
  { A_MOD_FIXATE,                            "Fixate"                                            },  // 128
  { A_MOD_SPEED_ALWAYS,                      "Increase Movement Speed% (Stacking)"               },  // 129
  { A_MOD_MOUNTED_SPEED_ALWAYS,              "Increase Mount Speed% (Stacking)"                  },  // 130
  { A_MOD_RANGED_ATTACK_POWER_VERSUS,        "Modify Ranged Attack Power vs Race"                },  // 131
  { A_INCREASE_RESOURCE_PCT,                 "Increase Resource%"                                },  // 132
  { A_INCREASE_HEALTH_PCT,                   "Increase Health%"                                  },  // 133
  { A_MOD_HEALING_DONE,                      "Modify Healing Power"                              },  // 135
  { A_MOD_HEALING_DONE_PERCENT,              "Modify Healing% Done"                              },  // 136
  { A_MOD_TOTAL_STAT_PERCENTAGE,             "Modify Total Stat%"                                },  // 137
  { A_MOD_MELEE_HASTE,                       "Modify Melee Haste%"                               },  // 138
  { A_FORCE_REACTION,                        "Force Reaction"                                    },  // 139
  { A_MOD_RANGED_HASTE,                      "Modify Ranged Haste%"                              },  // 140
  { A_MOD_BASE_RESISTANCE_PCT,               "Modify Base Resistance"                            },  // 142
  { A_MOD_RECHARGE_RATE_LABEL,               "Modify Cooldown Recharge Rate% (Label)"            },  // 143
  { A_SAFE_FALL,                             "Reduce Fall Damage"                                },  // 144
  { A_CREATURE_IMMUNITIES,                   "Creature Immunities"                               },  // 147
  { A_MOD_CHARGE_RECHARGE_RATE,              "Modify Charge Cooldown Recharge Rate% (Category)"  },  // 148
  { A_REDUCE_PUSHBACK,                       "Modify Casting Pushback"                           },  // 149
  { A_MOD_SHIELD_BLOCKVALUE_PCT,             "Modify Block Effectiveness"                        },  // 150
  { A_MOD_DETECTED_RANGE,                    "Modify Aggro Distance"                             },  // 152
  { A_MOD_AUTO_ATTACK_RANGE,                 "Modify Auto Attack Range"                          },  // 153
  { A_MOD_STEALTH_LEVEL,                     "Modify Stealth Detection Level"                    },  // 154
  { A_MOD_HEALTH_REGEN_IN_COMBAT,            "Modify Health Regeneration Rate in Combat"         },  // 161
  { A_PET_DAMAGE_MULTI,                      "Modify Absorb% Done"                               },  // 157
  { A_MOD_CRIT_DAMAGE_MULTIPLIER,            "Modify Crit Damage Done%"                          },  // 163
  { A_FORCE_BREATH_BAR,                      "Force Breath Bar"                                  },  // 164
  { A_MOD_ATTACK_POWER_PCT,                  "Modify Melee Attack Power%"                        },  // 166
  { A_MOD_RANGED_ATTACK_POWER_PCT,           "Modify Ranged Attack Power%"                       },  // 167
  { A_MOD_DAMAGE_DONE_VERSUS,                "Modify Damage Done% vs Race"                       },  // 168
  { A_MOD_SPEED_NOT_STACK,                   "Increase Movement Speed%"                          },  // 171
  { A_MOD_MOUNTED_SPEED_NOT_STACK,           "Increase Mounted Speed%"                           },  // 172
  { A_MOD_RECHARGE_TIME_PCT_CATEGORY_MASK,   "Modify Recharge Time% (Category Type Mask)"        },  // 173
  { A_AOE_CHARM,                             "Charmed"                                           },  // 177
  { A_MOD_MAX_MANA_PCT,                      "Modify Max Mana%"                                  },  // 178
  { A_MOD_ATTACKER_SPELL_CRIT_CHANCE,        "Modify Attacker Spell Crit Chance"                 },  // 179
  { A_MOD_FLAT_SPELL_DAMAGE_VERSUS,          "Modify Spell Damage vs Race"                       },  // 180
  { A_MOD_SPELL_CURRENCY_REAGENTS_COUNT_PCT, "Modify Spell Reagent Cost%"                        },  // 181
  { A_MOD_ATTACKER_MELEE_HIT_CHANCE,         "Modify Attacker Melee Hit Chance"                  },  // 184
  { A_MOD_ATTACKER_RANGED_HIT_CHANCE,        "Modify Attacker Ranged Hit Chance"                 },  // 185
  { A_MOD_ATTACKER_SPELL_HIT_CHANCE,         "Modify Attacker Spell Hit Chance"                  },  // 186
  { A_MOD_ATTACKER_MELEE_CRIT_CHANCE,        "Modify Attacker Melee Crit Chance"                 },  // 187
  { A_MOD_ATTACKER_RANGED_CRIT_CHANCE,       "Modify Attacker Ranged Crit Chance"                },  // 188
  { A_MOD_RATING,                            "Modify Rating"                                     },  // 189
  { A_USE_NORMAL_MOVEMENT_SPEED,             "Use Base Move Speed"                               },  // 191
  { A_MOD_MELEE_RANGED_HASTE,                "Modify Ranged and Melee Haste%"                    },  // 192
  { A_HASTE_ALL,                             "Modify All Haste%"                                 },  // 193
  { A_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE, "Modify Attacker Crit Chance"                   },  // 197
  { A_PCT_RATING_ADDED_TO_RATING,            "Percent from Rating Added to Rating"               },  // 198
  { A_MOD_KILL_XP_PCT,                       "Modify Experience Gained from Kills"               },  // 200
  { A_FLY,                                   "Fly"                                               },  // 201
  { A_MOD_ATTACKER_MELEE_CRIT_DAMAGE,        "Modify Melee Crit Damage Taken from Attacker"      },  // 203
  { A_PREVENT_RELEASE_SPIRIT,                "Prevent Releasing Spirit"                          },  // 204
  { A_MOD_RECHARGE_TIME_CATEGORY_MASK,       "Modify Recharge Time (Category Type Mask)"         },  // 205
  { A_MOD_RAGE_FROM_DAMAGE_DEALT,            "Modify Rage Generated From Auto Attacks"           },  // 213
  { A_HASTE_SPELLS,                          "Modify Casting Speed"                              },  // 216
  { A_MOD_MELEE_HASTE_2,                     "Modify Melee Haste"                                },  // 217
  { A_ADD_PCT_LABEL_MODIFIER,                "Apply Percent Modifier w/ Label"                   },  // 218
  { A_ADD_FLAT_LABEL_MODIFIER,               "Apply Flat Modifier w/ Label"                      },  // 219
  { A_MODIFY_SCHOOL,                         "Modify Spell School"                               },  // 220
  { A_REMOVE_TAUNT_EFFECTS,                  "Remove Taunt Effects"                              },  // 221
  { A_REMOVE_TRANSMOG_COST,                  "Remove Transmog Cost"                              },  // 222
  { A_REMOVE_BARBER_COST,                    "Remove Barber Cost"                                },  // 223
  { A_LEARN_TALENT,                          "Grant Talent"                                      },  // 224
  { A_PERIODIC_DUMMY,                        "Periodic Dummy"                                    },  // 226
  { A_DETECT_STEALTH,                        "Stealth Detection"                                 },  // 228
  { A_MOD_AOE_DAMAGE_AVOIDANCE,              "Modify AoE Damage Taken%"                          },  // 229
  { A_MOD_MAX_HEALTH,                        "Modify Max Health"                                 },  // 230
  { A_PROC_TRIGGER_SPELL_WITH_VALUE,         "Trigger Spell with Value"                          },  // 231
  { A_MECHANIC_DURATION_MOD,                 "Modify Mechanic Duration% (Stacking)"              },  // 232
  { A_CHANGE_ALL_HUMANOID_MODELS,            "Change all Humanoid Models"                        },  // 233
  { A_MECHANIC_DURATION_MOD_NOT_STACK,       "Modify Mechanic Duration%"                         },  // 234
  { A_MOD_DISPEL_RESIST,                     "Resist Dispel"                                     },  // 235
  { A_CONTROL_VEHICLE,                       "Control Vehicle"                                   },  // 236
  { A_MOD_SCALE_2,                           "Scale%"                                            },  // 239
  { A_MOD_EXPERTISE,                         "Modify Expertise%"                                 },  // 240
  { A_FORCE_MOVE_FORWARD,                    "Forced Movement"                                   },  // 241
  { A_MOD_SPELL_DAMAGE_FROM_HEALING,         "Modify Spell Damage from Healing"                  },  // 242
  { A_MOD_FACTION,                           "Change Faction"                                    },  // 243
  { A_COMPREHEND_LANGUAGE,                   "Comprehend Language"                               },  // 244
  { A_MOD_DURATION_OF_MAGIC_EFFECTS,         "Modify Debuff Duration%"                           },  // 245
  { A_CLONE_CASTER,                          "Copy Appearance"                                   },  // 247
  { A_MOD_INCREASE_HEALTH_2,                 "Increase Max Health (Stacking)"                    },  // 250
  { A_MOD_BLOCK_CRIT_CHANCE,                 "Modify Critical Block Chance"                      },  // 253
  { A_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT,     "Modify Damage Taken% from Mechanic"                },  // 255
  { A_NO_REAGENT_USE,                        "No Reagent Cost"                                   },  // 256
  { A_MOD_TARGET_RESIST_BY_SPELL_CLASS,      "Modify Damage Taken from Spell School"             },  // 257
  { A_OVERRIDE_SUMMONED_OBJECT,              "Modify Periodic Damage Taken%"                     },  // 258
  { A_MOD_HOT_RECIEVED_PCT,                  "Modify Periodic Healing Received%"                 },  // 259
  { A_ABILITY_IGNORE_AURA_STATE,             "Ignore Aura State"                                 },  // 262
  { A_ALLOW_ONLY_ABILITY,                    "Disable Abilities"                                 },  // 263
  { A_DISABLE_ATTACKING_EXCEPT_ABILITIES,    "Disable Spells"                                    },  // 264
  { A_MOD_IMMUNE_A_APPLY_SCHOOL,             "Immune Debuff Application from School"             },  // 267
  { A_MOD_ARMOR_BY_PRIMARY_STAT_PCT,         "Modify Armor by Primary Stat%"                     },  // 268
  { A_MOD_DAMAGE_TO_CASTER,                  "Modify Damage Done% to Caster"                     },  // 269
  { A_MOD_DAMAGE_FROM_CASTER,                "Modify Damage Taken% from Caster"                  },  // 270
  { A_MOD_DAMAGE_FROM_CASTER_SPELLS,         "Modify Damage Taken% from Caster's Spells"         },  // 271
  { A_MOD_BLOCK_PCT,                         "Modify Block Value%"                               },  // 272
  { A_MOD_BLOCK_FLAT,                        "Add Block Value"                                   },  // 274
  { A_MOD_IGNORE_SHAPESHIFT,                 "Modify Stance Mask"                                },  // 275
  { A_MOD_MECHANIC_DAMAGE_DONE_PERCENT,      "Modify Damage Done% from Mechanic"                 },  // 276
  { A_MOD_TARGET_ARMOR_PCT,                  "Modify Target Armor%"                              },  // 280
  { A_MOD_HEALING_RECEIVED_FROM_SPELL,       "Modify Healing Taken% from Caster's Spells"        },  // 283
  { A_LINKED_SPELL,                          "Cast Linked Spell"                                 },  // 284
  { A_LINKED_SPELL_WITH_VALUE,               "Cast Linked Spell w/ Value"                        },  // 285
  { A_MOD_RECHARGE_RATE,                     "Modify Cooldown Recharge Rate%"                    },  // 286
  { A_MOD_ALL_CRIT_CHANCE,                   "Modify Critical Strike%"                           },  // 290
  { A_MOD_QUEST_XP_PCT,                      "Modify Experience Gained from Quests"              },  // 291
  { A_OVERRIDE_SPELLS,                       "Override Spells"                                   },  // 293
  { A_PREVENT_REGENERATE_POWER,              "Prevent Power Regeneration"                        },  // 294
  { A_MOD_PERIODIC_DAMAGE_TAKEN,             "Modify Periodic Damage Taken"                      },  // 295
  { A_SET_VEHICLE_ID,                        "Set Vehicle ID"                                    },  // 296
  { A_MOD_ROOT_DISABLE_GRAVITY,              "Modify Root Effects - Disable Gravity"             },  // 297
  { A_MOD_STUN_DISABLE_GRAVITY,              "Modify Stun Effects - Disable Gravity"             },  // 298
  { A_SHARE_DAMAGE_PCT,                      "Share Damage Taken"                                },  // 300
  { A_SCHOOL_HEAL_ABSORB,                    "Absorb Healing"                                    },  // 301
  { A_MOD_DAMAGE_DONE_VERSUS_AURASTATE,      "Modify Damage Done Against Target With Aura"       },  // 303
  { A_MOD_MINIMUM_SPEED_PCT,                 "Modify Min Speed%"                                 },  // 305
  { A_MOD_CRIT_CHANCE_FROM_CASTER,           "Modify Crit Chance% from Caster"                   },  // 306
  { A_ENABLE_CAST_WHILE_MOVING_FOR_SPELL_LABEL, "Allow Casting while Moving For Spells (Label)"  },  // 307
  { A_MOD_CRIT_CHANCE_FROM_CASTER_SPELLS,    "Modify Crit Chance% from Caster's Spells"          },  // 308
  { A_MOD_RESILIENCE,                        "Modify Resilience"                                 },  // 309
  { A_MODE_CREATURE_AOE_DAMAGE_AVOIDANCE,    "Modify Creature AoE Damage Avoidance"              },  // 310
  { A_IGNORE_COMBAT,                         "Ignore Combat State"                               },  // 311
  { A_REPLACE_ANIMATION_SET,                 "Replace Animation (Set)"                           },  // 312
  { A_REPLACE_MOUNT_ANIMATION_SET,           "Replace Mount Animation (Set)"                     },  // 313
  { A_PREVENT_RESURRECTION,                  "Prevent Resurrection"                              },  // 314
  { A_UNDERWATER_WALKING,                    "Enable Underwater Walking"                         },  // 315
  { A_SCHOOL_ABSORB_OVERKILL,                "Absorb Overkill Damage From School"                },  // 316
  { A_MOD_MASTERY_PCT,                       "Modify Mastery%"                                   },  // 318
  { A_MOD_MELEE_AUTO_ATTACK_SPEED,           "Modify Melee Auto Attack Speed%"                   },  // 319
  { A_APPLY_HASTED_GCD_LABEL,                "Apply Hasted GCD to Spells in Label"               },  // 320
  { A_DISABLE_ACTIONS,                       "Disable Actions"                                   },  // 321
  { A_DISABLE_TARGETING,                     "Disable Targeting"                                 },  // 322
  { A_TRIGGER_SPELL_ON_POWER_PCT,            "Trigger Spell Based on Resource%"                  },  // 328
  { A_MOD_POWER_GAIN_PCT,                    "Modify Resource Generation%"                       },  // 329
  { A_CAST_WHILE_MOVING_WHITELIST,           "Cast while Moving (Whitelist)"                     },  // 330
  { A_FORCE_WEATHER,                         "Force Weather"                                     },  // 331
  { A_OVERRIDE_ACTION_SPELL,                 "Override Action Spell (Misc w/ Base)"              },  // 332
  { A_OVERRIDE_ACTION_SPELL_TRIGGERED,       "Override Triggered Action Spell (Misc w/ Base)"    },  // 333
  { A_MOD_AUTOATTACK_CRIT_CHANCE,            "Modify Auto Attack Critical Chance"                },  // 334
  { A_RESTRICT_MOUNTS,                       "Restrict Mounts"                                   },  // 336
  { A_MOD_CRIT_CHANCE_FOR_CASTER_PET,        "Modify Crit Chance% from Caster's Pets"            },  // 339
  { A_MOD_RESURRECTION_HEALTH,               "Modify Resurrection Health%"                       },  // 340
  { A_MODIFY_CATEGORY_COOLDOWN,              "Modify Cooldown Time (Category)"                   },  // 341
  { A_MOD_RANGED_AND_MELEE_AUTO_ATTACK_SPEED,"Modify Ranged and Melee Auto Attack Speed%"        },  // 342
  { A_MOD_AUTO_ATTACK_FROM_CASTER,           "Modify Auto Attack Damage Taken% from Caster"      },  // 343
  { A_MOD_AUTO_ATTACK_PCT,                   "Modify Auto Attack Damage Done%"                   },  // 344
  { A_MOD_IGNORE_ARMOR_PCT,                  "Ignore Armor%"                                     },  // 345
  { A_ENABLE_ALTERATE_POWER,                 "Enable Secondary Resource Cost"                    },  // 346
  { A_MOD_COOLDOWN_BY_HASTE,                 "Modify Cooldown Duration by Haste%"                },  // 347
  { A_MOD_HEALING_DONE_PCT_VS_TARGET_HEALTH, "Modify Healing% Based on Target Health%"           },  // 354
  { A_MOD_SPELL_HASTE,                       "Modify Spell Haste%"                               },  // 355
  { A_PROC_TRIGGER_SPELL_COPY,               "Duplicate Ability"                                 },  // 360
  { A_OVERRIDE_AUTO_ATTACK_WITH_ABILITY,     "Override Auto-Attack with Ability"                 },  // 361
  { A_OVERRIDE_SP_PER_AP,                    "Override Spell Power per Attack Power%"            },  // 366
  { A_OVERRIDE_AUTO_ATTACK_WITH_SPELL,       "Override Auto-Attack with Spell"                   },  // 367
  { A_MOD_SPEED_NO_CONTROL,                  "Force Move (No Control)"                           },  // 373
  { A_374,                                   "Reduce Fall Damage%"                               },  // 374
  { A_ENABLE_CAST_WHILE_MOVING,              "Cast while Moving"                                 },  // 377
  { A_MOD_POSSESS_PET,                       "Possess Pet"                                       },  // 378
  { A_MOD_MANA_REGEN_PCT,                    "Modify Mana Regen%"                                },  // 379
  { A_MOD_DAMAGE_FROM_CASTER_GUARDIAN,       "Modify Damage Taken% from Caster Guardian"         },  // 380
  { A_MOD_DAMAGE_FROM_CASTER_PET,            "Modify Damage Taken% from Caster Pet"              },  // 381
  { A_MOD_PET_STAT,                          "Modify Pet Stat"                                   },  // 382
  { A_IGNORE_SPELL_COOLDOWN,                 "Ignore Spell Cooldown"                             },  // 383
  { A_BLOCK_SPELLS_IN_FRONT,                 "Block Spells In Front"                             },  // 393
  { A_AREA_TRIGGER,                           "Area Trigger"                                     },  // 395
  { A_TRIGGER_SPELL_ON_POWER_AMOUNT,         "Trigger Spell Based on Resource Amount"            },  // 396
  { A_MOD_TIME_RATE,                         "Modify Time Rate"                                  },  // 399
  { A_MOD_SKILL_2,                           "Modify Skill"                                      },  // 400
  { A_OVERRIDE_POWER_DISPLAY,                "Override Resource Display"                         },  // 402
  { A_OVERRIDE_SPELL_VISUAL,                 "Override Spell Visual"                             },  // 403
  { A_OVERRIDE_AP_PER_SP,                    "Override Attack Power per Spell Power%"            },  // 404
  { A_MOD_RATING_MULTIPLIER,                 "Modify Combat Rating Multiplier"                   },  // 405
  { A_KEYBIND_OVERRIDE,                      "Override Keybind"                                  },  // 406
  { A_MOD_FEAR_2,                            "Modify Fear"                                       },  // 407
  { A_SET_ACTION_BUTTON_SPELL_COUNT,         "Set Action Button Spell Count"                     },  // 408
  { A_CAN_TURN_WHILE_FALLING,                "Slow Fall"                                         },  // 409
  { A_MOD_MAX_CHARGES,                       "Modify Cooldown Charge (Category)"                 },  // 411
  { A_MOD_RANGED_ATTACK_DEFLECT_CHANCE,      "Modify Ranged Attack Deflect Chance"               },  // 413
  { A_MOD_RANGED_ATTACK_BLOCK_CHANCE_IN_FRONT, "Modify Ranged Attack Block Chance from Front"    },  // 414
  { A_HASTED_COOLDOWN,                       "Hasted Cooldown Duration"                          },  // 416
  { A_HASTED_GCD,                            "Hasted Global Cooldown"                            },  // 417
  { A_MOD_MAX_RESOURCE,                      "Modify Max Resource"                               },  // 418
  { A_MOD_MAX_RESOURCE_PCT,                  "Modify Max Resource%"                              },  // 419
  { A_MOD_BATTLEPET_EXP_PCT,                 "Modify Battlepet Experience Gained"                },  // 420
  { A_MOD_ABSORB_DONE_PERCENT,               "Modify Absorb% Done"                               },  // 421
  { A_MOD_ABSORB_RECEIVED_PERCENT,           "Modify Absorb% Received"                           },  // 422
  { A_MOD_MANA_COST_PCT,                     "Modify Mana Cost%"                                 },  // 423
  { A_CASTER_IGNORE_LINE_OF_SIGHT,           "Caster Ignores Line of Sight"                      },  // 424
  { A_SCALE_PLAYER_LEVEL,                    "Scale Player Level"                                },  // 427
  { A_TRIGGER_SUMMON_WITH_DURATION_OVERRIDE, "Trigger Summon Spell w/ Duration Override"         },  // 428
  { A_MOD_PET_DAMAGE_DONE,                   "Modify Pet Damage Done%"                           },  // 429
  { A_MOD_ENVIRONMENTAL_DAMAGE_TAKEN,        "Modify Environmental Damage Taken"                 },  // 436
  { A_MOD_MINIMUM_SPEED,                     "Modify Minimum Speed"                              },  // 437
  { A_MOD_MULTISTRIKE_DAMAGE,                "Modify Multistrike Damage"                         },  // 440
  { A_MOD_MULTISTRIKE_CHANCE,                "Modify Multistrike%"                               },  // 441
  { A_MOD_LEECH_PERCENT,                     "Modify Leech%"                                     },  // 443
  { A_ADVANCED_FLYING,                       "Dragonriding"                                      },  // 446
  { A_MOD_EXP_FROM_CREATURE_TYPE,            "Modify Experience Gained% vs Race"                 },  // 447
  { A_MOD_RECHARGE_TIME_CATEGORY,            "Modify Recharge Time (Category)"                   },  // 453
  { A_MOD_RECHARGE_TIME_PCT_CATEGORY,        "Modify Recharge Time% (Category)"                  },  // 454
  { A_MOD_ROOT_2,                            "Root (Respects Threat Table)"                      },  // 455
  { A_HASTED_CATEGORY,                       "Hasted Cooldown Duration (Category)"               },  // 457
  { A_HASTED_CATEGORY_REGEN,                 "Hasted Cooldown Regeneration (Category)"           },  // 458
  { A_DUAL_WIELD_HIT_PENALTY,                "Dual Wield Hit Chance Penalty"                     },  // 459
  { A_MOD_HEALING_AND_ABSORB_FROM_CASTER,    "Modify Healing and Absorb Recieved from Caster"    },  // 462
  { A_MOD_PARRY_FROM_CRIT_RATING,            "Modify Parry Rating with Crit Rating"              },  // 463
  { A_MOD_ATTACK_POWER_FROM_BONUS_ARMOR,     "Modify Attack Power with Bonus Armor"              },  // 464
  { A_MOD_BONUS_ARMOR,                       "Increase Armor"                                    },  // 465
  { A_MOD_BONUS_ARMOR_PCT,                   "Modify Armor%"                                     },  // 466
  { A_MOD_STAT_BONUS_PCT,                    "Modify Stat Bonus%"                                },  // 467
  { A_TRIGGER_SPELL_BY_HEALTH_PCT,           "Trigger Spell Based on Health%"                    },  // 468
  { A_MOD_TIME_RATE_BY_SPELL_LABEL,          "Modify Time Rate (Label)"                          },  // 470
  { A_MOD_VERSATILITY_PCT,                   "Modify Versatility%"                               },  // 471
  { A_PREVENT_DURABILITY_LOSS_FROM_COMBAT,   "Prevent Durability Loss from Combat"               },  // 473
  { A_REPLACE_ITEM_BONUS_TREE,               "Replace Item Bonus Tree"                           },  // 474
  { A_ALLOW_USING_GAMEOBJECT_WHILE_MOUNTED,  "Allow Interaction while Mounted"                   },  // 475
  { A_MOD_ARTIFACT_ITEM_LEVEL,               "Modify Artifact Item Level"                        },  // 480
  { A_CONVERT_CONSUMED_RUNE,                 "Convert Consumed Rune"                             },  // 481
  { A_SUPPRESS_TRANSFORM,                    "Supress Transformation"                            },  // 483
  { A_ALLOW_INTERRUPT_SPELL,                 "Allow Interrupt Spell"                             },  // 484
  { A_MOD_MOVEMENT_FORCE_MAGNITUDE,          "Resist Forced Movement%"                           },  // 485
  { A_COSMETIC_MOUNTED,                      "Mount Cosmetic Mount"                              },  // 487
  { A_DISABLE_GRAVITY,                       "Disable Gravity"                                   },  // 488
  { A_493,                                   "Hunter Animal Companion"                           },  // 493
  { A_SET_POWER_POINT_CHARGE,                "Set Power Point Charge"                            },  // 494
  { A_TRIGGER_SPELL_ON_EXPIRE,               "Trigger Spell on Aura Expire"                      },  // 495
  { A_IGNORE_SPELL_CHARGE_COOLDOWN_CATEGORY, "Ignore Spell Charge Cooldown (Category)"           },  // 500
  { A_MOD_CRIT_DAMAGE_PCT_FROM_CASTER_SPELLS,"Modify Crit Damage Done% from Caster's Spells"     },  // 501
  { A_MOD_VERSATILITY_DAMAGE_BENEFIT,        "Modify Versatility Damage Benefit%"                },  // 502
  { A_MOD_VERSATILITY_HEALING_BENEFIT,       "Modify Versatility Healing Benefit%"               },  // 503
  { A_MOD_HEALING_RECEIVED_FROM_CASTER,      "Modify Healing Taken% from Caster"                 },  // 504
  { A_MOD_DAMAGE_FROM_SPELLS_LABEL,          "Modify Damage Taken% from Spells (Label)"          },  // 507
  { A_APPLY_PROFESSION_EFFECT,               "Enable Profession Effects"                         },  // 511
  { A_MOD_COOLDOWN_RECOVERY_RATE,            "Modify Cooldown Recovery Rate"                     },  // 519
  { A_ALLOW_BLOCKING_SPELLS,                 "Allow Blocking Spells"                             },  // 528
  { A_MOD_SPELL_BLOCK_CHANCE,                "Modify Spell Block Chance"                         },  // 529
  { A_MOD_AUTO_ATTACK_DAMAGE_PCT,            "Modify Auto Attack Damage%"                        },  // 530
  { A_MOD_GUARDIAN_DAMAGE_DONE,              "Modify Guardian Damage Done%"                      },  // 531
  { A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL,   "Modify Damage Taken% from Caster's Spells (Label)" },  // 537
  { A_MOD_SUPPORT_STAT,                      "Modify Stat With Support Triggers"                 },  // 540
  { A_TRIGGER_SPELL_ON_STACK_AMOUNT,         "Trigger Spell on Aura Stack Count"                 },  // 542
  { A_MOD_CRITICAL_BLOCK_AMOUNT,             "Modify Critical Block Amount"                      },  // 638
  { A_MOD_DAMAGE_DONE_TO_CASTER_FROM_SCHOOL, "Modify Damage done to Caster from School"          },  // 639
  { A_MOD_RANGED_ATTACK_SPEED_FLAT,          "Modify Ranged Attack Speed Flat"                   },  // 643
  { A_MOD_FLAT_PVP_MULTIPLIER,               "Modify Flat PVP Multiplier"                        },  // 646
  { A_MOD_PCT_PVP_MULTIPLIER,                "Modify Percent PVP Multiplier"                     },  // 647
  { A_MOD_FLAT_LABEL_PVP_MULTIPLIER,         "Modify Flat PVP Multiplier by Label"               },  // 648
  { A_MOD_PCT_LABEL_PVP_MULTIPLIER,          "Modify Percent PVP Multiplier by Label"            },  // 649
} );

static constexpr auto _effect_attribute_strings = util::make_static_map<unsigned, std::string_view>( {
  { EX_NO_IMMUNITY,                           "No Immunity"                                      },  // 1
  { EX_POS_RELATIVE_TO_FACING,                "Position is facing relative"                      },  // 2
  { EX_JUMP_CHARGE_MELEE_RANGE,               "Jump Charge Unit Melee Range"                     },  // 3
  { EX_JUMP_CHARGE_PATH_CHECK,                "Jump Charge Unit Strict Path Check"               },  // 4
  { EX_EXCLUDE_OWN_PARTY,                     "Exclude Own Party"                                },  // 5
  { EX_ALWAYS_USE_AOE_LOS,                    "Always AOE Line of Sight"                         },  // 6
  { EX_SUPPRESS_STACKING,                     "Suppress Points Stacking"                         },  // 7
  { EX_CHAIN_FROM_INITIAL,                    "Chain from Initial Target"                        },  // 8
  { EX_UNCONTROLLED_NO_BACKWARDS,             "Uncontrolled No Backwards"                        },  // 9
  { EX_AURA_STACK,                            "Aura Points Stack"                                },  // 10
  { EX_NO_COPY_DMG_INT_PROC,                  "No Copy Damage Interrupts or Procs"               },  // 11
  { EX_ADD_TARGET_REACH_TO_AOE,               "Add Target (Dest) Combat Reach to AOE"            },  // 12
  { EX_IS_HARMFUL,                            "Is Harmful"                                       },  // 13
  { EX_FORCE_SCALE_CAM_MIN_HEIGHT,            "Force Scale to Override Camera Min Height"        },  // 14
  { EX_PLAYERS_ONLY,                          "Players Only"                                     },  // 15
  { EX_COMPUTE_ON_CAST,                       "Compute Points Only At Cast Time"                 },  // 16
  { EX_ENFORCE_LOS_ON_CHAIN,                  "Enforce Line of Sight To Chain Targets"           },  // 17
  { EX_AOE_USE_TARGET_RADIUS,                 "Area Effects Use Target Radius"                   },  // 18
  { EX_TELEPORT_WITH_VEHICLE,                 "Teleport With Vehicle (during map transfer)"      },  // 19
  { EX_SCALE_POINTS_BY_CHALLENGE_MOD_SCALER,  "Scale Points by Challenge Mode Damage Scaler"     },  // 20
  { EX_DONT_FAIL_CAST_ON_TARGETING_FAIL,      "Don't Fail Spell On Targeting Failure"            },  // 21
  { EX_ALWAYS_HIT,                            "Always Hit"                                       },  // 23
  { EX_IGNORE_DURING_COOLDOWN_TIME_RATE_CALC, "Ignore During Cooldown Time Rate Calculation"     },  // 24
  { EX_ONLY_AFFECTS_ABSORBS,                  "Damage Only Affects Absorbs"                      },  // 27
} );

static constexpr auto _mechanic_strings = util::make_static_map<unsigned, std::string_view>( {
  { MECHANIC_CHARM,           "Charm"          },  // 1
  { MECHANIC_DISORIENT,       "Disorient"      },  // 2
  { MECHANIC_DISARM,          "Disarm"         },  // 3
  { MECHANIC_DISTRACT,        "Distract"       },  // 4
  { MECHANIC_FLEE,            "Flee"           },  // 5
  { MECHANIC_KNOCKBACK,       "Knockback"      },  // 6
  { MECHANIC_ROOT,            "Root"           },  // 7
  { MECHANIC_SLOW,            "Slow"           },  // 8
  { MECHANIC_SILENCE,         "Silence"        },  // 9
  { MECHANIC_SLEEP,           "Sleep"          },  // 10
  { MECHANIC_SNARE,           "Snare"          },  // 11
  { MECHANIC_STUN,            "Stun"           },  // 12
  { MECHANIC_FREEZE,          "Freeze"         },  // 13
  { MECHANIC_INCAPACITATE,    "Incapacitate"   },  // 14
  { MECHANIC_BLEED,           "Bleed"          },  // 15
  { MECHANIC_HEAL,            "Heal"           },  // 16
  { MECHANIC_POLYMORPH,       "Polymorph"      },  // 17
  { MECHANIC_BANISH,          "Banish"         },  // 18
  { MECHANIC_SHIELD,          "Shield"         },  // 19
  { MECHANIC_SHACKLE,         "Shackle"        },  // 20
  { MECHANIC_MOUNT,           "Mount"          },  // 21
  { MECHANIC_INFECT,          "Infect"         },  // 22
  { MECHANIC_TURN,            "Turn"           },  // 23
  { MECHANIC_HORRIFY,         "Horrify"        },  // 24
  { MECHANIC_INVULNERABLE_2,  "Invulnerable 2" },  // 25
  { MECHANIC_INTERRUPT,       "Interrupt"      },  // 26
  { MECHANIC_DAZE,            "Daze"           },  // 27
  { MECHANIC_DISCOVER,        "Discover"       },  // 28
  { MECHANIC_INVULNERABLE,    "Invulnerable"   },  // 29
  { MECHANIC_SAP,             "Sap"            },  // 30
  { MECHANIC_ENRAGE,          "Enrage"         },  // 31
  { MECHANIC_WOUND,           "Wound"          },  // 32
  { MECHANIC_INFECT_2,        "Infect 2"       },  // 33
  { MECHANIC_INFECT_3,        "Infect 3"       },  // 34
  { MECHANIC_INFECT_4,        "Infect 4"       },  // 35
  { MECHANIC_TAUNT,           "Taunt"          },  // 36
} );

static constexpr auto _label_strings = util::make_static_map<int, std::string_view>( {
  { LABEL_CLASS_SPELLS,         "Class Spells"         },  // 16
  { LABEL_MAGE_SPELLS,          "Mage Spells"          },  // 17
  { LABEL_PRIEST_SPELLS,        "Priest Spells"        },  // 18
  { LABEL_WARLOCK_SPELLS,       "Warlock Spells"       },  // 19
  { LABEL_ROGUE_SPELLS,         "Rogue Spells"         },  // 20
  { LABEL_DRUID_SPELLS,         "Druid Spells"         },  // 21
  { LABEL_MONK_SPELLS,          "Monk Spells"          },  // 22
  { LABEL_HUNTER_SPELLS,        "Hunter Spells"        },  // 23
  { LABEL_SHAMAN_SPELLS,        "Shaman Spells"        },  // 24
  { LABEL_WARRIOR_SPELLS,       "Warrior Spells"       },  // 25
  { LABEL_PALADIN_SPELLS,       "Paladin Spells"       },  // 26
  { LABEL_DEATH_KNIGHT_SPELLS,  "Death Knight Spells"  },  // 27
  { LABEL_DEMON_HUNTER_SPELLS,  "Demon Hunter Spells"  },  // 66
  { LABEL_AZERITE_ESSENCES,     "Azerite Essences"     },  // 640
  { LABEL_MAJOR_COOLDOWNS,      "Major Cooldowns"      },  // 690
  { LABEL_HEALING_SPELLS,       "Healing Spells"       },  // 741
  { LABEL_COVENANT,             "Covenant Spells"      },  // 976
  { LABEL_EVOKER_SPELLS,        "Evoker Spells"        },  // 1216
  { LABEL_EVOKER_RED_SPELLS,    "Red Evoker Spells"    },  // 1464
  { LABEL_EVOKER_BLUE_SPELLS,   "Blue Evoker Spells"   },  // 1465
  { LABEL_EVOKER_GREEN_SPELLS,  "Green Evoker Spells"  },  // 1466
  { LABEL_EVOKER_BRONZE_SPELLS, "Bronze Evoker Spells" },  // 1467
  { LABEL_EVOKER_BLACK_SPELLS,  "Black Evoker Spells"  },  // 1468
  { LABEL_ITEM_EFFECTS,         "Item Effects"         },  // 3959
} );

static constexpr auto _scaling_class_strings = util::make_static_map<int, std::string_view>( {
  {  -1, "Primary Attribute"       },
  {  -2, "Restore Health/Resource" },
  {  -3, "Food/Gems Attribute"     },
  {  -4, "Food/Gems Attribute"     },
  {  -5, "Food/Gems Attribute"     },
  {  -6, "Stamina"                 },
  {  -7, "Secondary Rating"        },
  {  -8, "Replace Primary"         },
  {  -9, "Replace Secondary"       },
  { -10, "Restore Mana"            },
} );

static constexpr auto _pet_stat_strings = util::make_static_map<int, std::string_view>( {
  {  1, "Health"                   },
  {  2, "Attack Power Inheritence" },
  {  3, "Spell Power Inheritence"  },
  { 24, "Unknown"                  },
  { 28, "Health Regen Rate"        },
} );

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
      return fmt::format( "Affected Spells (Label): {}", *spell );
    else
      return fmt::format( "{}", *spell );
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
      // 12.0 pvp modifier effects (646, 647, 648, 649)
      case A_MOD_FLAT_PVP_MULTIPLIER:
      case A_MOD_PCT_PVP_MULTIPLIER:
      case A_MOD_FLAT_LABEL_PVP_MULTIPLIER:
      case A_MOD_PCT_LABEL_PVP_MULTIPLIER:
        tmp_str += fmt::format( ": {}", map_string( _pvp_property_type_strings, e->misc_value1() ) );
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
         ( e->subtype() == A_MOD_INCREASE_RESOURCE || e->subtype() == A_INCREASE_RESOURCE_PCT ||
           e->subtype() == A_MOD_MAX_RESOURCE || e->subtype() == A_MOD_MAX_RESOURCE_PCT ||
           e->subtype() == A_MOD_POWER_REGEN_PERCENT || e->subtype() == A_TRIGGER_SPELL_ON_POWER_AMOUNT ||
           e->subtype() == A_TRIGGER_SPELL_ON_POWER_PCT ) ) )
  {
    tokens.emplace_back( fmt::format( "Resource: {}", util::resource_type_string( util::power_type_to_resource(
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
  else if ( e->subtype() == A_PCT_RATING_ADDED_TO_RATING )
  {
    tokens.emplace_back(
      fmt::format( "From Rating: {}", util::stat_type_abbrev( util::translate_rating_mod( e->misc_value1() ) ) ) );
  }
  else if ( e->subtype() == A_MOD_MECHANIC_RESISTANCE || e->subtype() == A_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT ||
            e->subtype() == A_MOD_MECHANIC_DAMAGE_DONE_PERCENT )
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
    else if ( e->subtype() == A_TRIGGER_SPELL_ON_STACK_AMOUNT )
    {
      tokens.emplace_back( fmt::format( "Min Stack Count: {}", e->misc_value1() ) );
    }
    else if ( e->type() == E_TRIGGER_SPELL )
    {
      tokens.emplace_back( fmt::format( "Delay: {}_ms", e->misc_value1() ) );
    }
    else if ( e->subtype() == A_MOD_DAMAGE_DONE_VERSUS || e->subtype() == A_MOD_EXP_FROM_CREATURE_TYPE )
    {
      std::vector<std::string> _strs;
      auto _mask = e->misc_value1();
      for ( auto i = 1; _mask; _mask >>= 1, i++ )
        if ( _mask & 1 )
          _strs.emplace_back( util::race_type_string( static_cast<race_e>( i ) ) );

      tokens.emplace_back( fmt::format( "Race: {}", fmt::join( _strs, ", " ) ) );
    }
    else if ( e->subtype() == A_MOD_PET_STAT )
    {
      tokens.emplace_back( fmt::format( "Stat: {}", map_string( _pet_stat_strings, e->misc_value1() ) ) );
    }
    else
    {
      tokens.emplace_back( fmt::format( "Misc Value: {}", e->misc_value1() ) );
    }
  }

  if ( e->misc_value2() != 0 )
  {
    switch( e->subtype() )
    {
    case A_MOD_TOTAL_STAT_PERCENTAGE:
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
    break;
    case A_ADD_PCT_LABEL_MODIFIER:
    case A_ADD_FLAT_LABEL_MODIFIER:
    case A_MOD_PCT_LABEL_PVP_MULTIPLIER:
    case A_MOD_FLAT_LABEL_PVP_MULTIPLIER:
      tokens.emplace_back( fmt::format( "Misc Value 2: {} (Label)", e->misc_value2() ) );
      break;
    case A_PCT_RATING_ADDED_TO_RATING:
      tokens.emplace_back(
        fmt::format( "To Rating: {}", util::stat_type_abbrev( util::translate_rating_mod( e->misc_value2() ) ) ) );
      break;
    case A_TRIGGER_SPELL_ON_STACK_AMOUNT:
      tokens.emplace_back( fmt::format( "Max Stack Count: {}", e->misc_value2() ) );
      break;
    case A_MOD_PET_STAT:
      tokens.emplace_back( fmt::format( "NPC ID: {}", e->misc_value2() ) );
      break;
    default: tokens.emplace_back( fmt::format( "Misc Value 2: {}", e->misc_value2() ) );
      break;
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
        return fmt::format( "Affected Spells: {}", *spell );
      else
        return fmt::format( "{}", *spell );
    }, wrap );
    s << std::endl;
  }

  switch ( e->type() )
  {
    case E_APPLY_AURA:
    case E_APPLY_AREA_AURA_PARTY:
    case E_APPLY_AREA_AURA_RAID:
    case E_APPLY_AREA_AURA_PET:
    case E_APPLY_AREA_AURA_FRIEND:
    case E_APPLY_AREA_AURA_ENEMY:
    case E_APPLY_AREA_AURA_OWNER:
    case E_APPLY_AURA_PET:
    case E_APPLY_AURA_PLAYER_AND_PET:
    case E_REMOVE_AURA_BY_SPELL_LABEL:
    case E_APPLY_AREA_AURA_PARTY_NONRANDOM:
    case E_MODIFY_COOLDOWN_IN_CATEGORY:
    case E_RECHARGE_CATEGORY_COOLDOWN_IMMEDIATE:
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
           e->type() == E_MODIFY_COOLDOWN_IN_CATEGORY || e->type() == E_RECHARGE_CATEGORY_COOLDOWN_IMMEDIATE )
      {
        if ( auto affected = dbc.spells_by_category( e->misc_value1() ); !affected.empty() )
        {
          s << "                   Affected Spells (Category): ";
          s << wrap_concatenate( affected, []( const spell_data_t* s ) { return fmt::format( "{}", *s ); }, wrap );
          s << std::endl;
        }
      }
      break;
    default:
      break;
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

    if ( trait->node_type == NODE_TIERED || trait->node_type == NODE_CHOICE )
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

    if ( ( pd.type() == POWER_MANA || pd.type() == POWER_HEALTH ) && pd._cost == 0 )
      s << pd.cost() * 100.0 << "%";
    else
      s << pd.cost();

    s << " ";

    if ( pd.max_cost() != 0 )
    {
      s << "- ";
      if ( ( pd.type() == POWER_MANA || pd.type() == POWER_HEALTH ) && pd._cost_max == 0 )
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

  if( spell->cone_degrees() != 0 )
    s << "Cone Angle       : " << spell->cone_degrees() << " degrees" << std::endl;

  if ( spell->line_width() != 0 )
    s << "Line Width       : " << spell->line_width() << " yards" << std::endl;

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

  std::array<std::vector<const spelleffect_data_t*>, 5> modified_by;
  auto check_modifying = [ & ]( const spelleffect_data_t* const eff ) {
    switch ( eff->subtype() )
    {
      case A_ADD_FLAT_MODIFIER:
      case A_ADD_PCT_MODIFIER:
      case A_ADD_FLAT_LABEL_MODIFIER:
      case A_ADD_PCT_LABEL_MODIFIER:
        break;
      default:
        return;
    }

    switch ( eff->property_type() )
    {
      case P_EFFECT_1: modified_by[ 0 ].push_back( eff ); return;
      case P_EFFECT_2: modified_by[ 1 ].push_back( eff ); return;
      case P_EFFECT_3: modified_by[ 2 ].push_back( eff ); return;
      case P_EFFECT_4: modified_by[ 3 ].push_back( eff ); return;
      case P_EFFECT_5: modified_by[ 4 ].push_back( eff ); return;
      case P_EFFECTS: range::for_each( modified_by, [ & ]( auto& v ) { v.push_back( eff ); } ); return;
      default: return;
    }
  };

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
      s << ": " << wrap_concatenate( affecting_effects, [ & ]( const spelleffect_data_t* e ) {
        check_modifying( e );
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
      const auto spell_string = [ & ]( util::span<const spelleffect_data_t* const> effects ) {
        const spell_data_t* spell = effects.front()->spell();
        if ( effects.size() == 1 )
        {
          check_modifying( effects.front() );
          return fmt::format( "{} ({} effect#{})", spell->name_cstr(), spell->id(), effects.front()->index() + 1 );
        }

        fmt::memory_buffer s;
        fmt::format_to( std::back_inserter( s ), "{} ({} effects: ", spell->name_cstr(), spell->id() );
        for ( size_t i = 0; i < effects.size(); i++ )
        {
          check_modifying( effects[ i ] );
          fmt::format_to( std::back_inserter( s ), "{}#{}", i == 0 ? "" : ", ", effects[ i ]->index() + 1 );
        }
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
    s << wrap_concatenate( spell->drivers(), []( const spell_data_t* s ) { return fmt::format( "{}", *s ); }, wrap );
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

  auto print_modified_by = [ & ]( const spelleffect_data_t& e ) {
    if ( e.index() >= 5 || modified_by[ e.index() ].empty() )
       return;

    std::vector<std::string> modified_by_str;
    for ( auto eff : modified_by[ e.index() ] )
    {
      modified_by_str.push_back(
        fmt::format( "{} ({} effect#{})", eff->spell()->name_cstr(), eff->spell_id(), eff->index() + 1 ) );
    }

    if ( !modified_by_str.empty() )
      fmt::print( s, "                   Modified By: {}\n", fmt::join( modified_by_str, ", " ) );
  };

  s << "Effects          :" << std::endl;
  for ( const spelleffect_data_t& e : spell->effects() )
  {
    if ( e.id() == 0 )
      continue;

    spell_info::effect_to_str( dbc, spell, &e, s, level, wrap );
    print_modified_by( e );
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
  auto spec_string = []( const std::array<unsigned, 4>& specs ) {
    std::vector<std::string> tokens;

    for ( auto spec : specs )
    {
      if ( spec )
      {
        tokens.emplace_back(
          fmt::format( "{} ({})", dbc::specialization_string( static_cast<specialization_e>( spec ) ), spec ) );
      }
    }

    return util::string_join( tokens, ", " );
  };

  std::ostringstream s;

  s << "Name         : " << talent->name << std::endl;
  s << "Entry        : " << talent->id_trait_node_entry << std::endl;
  s << "Node         : " << talent->id_node << std::endl;
  s << "Definition   : " << talent->id_trait_definition << std::endl;
  s << fmt::format( "Tree         : {} ({})",
                    util::talent_tree_string( static_cast<talent_tree>( talent->tree_index ) ), talent->tree_index )
    << std::endl;
  s << "Class        : " << util::player_type_string( util::translate_class_id( talent->id_class ) ) << std::endl;
  if ( talent->id_spec[ 0 ] != 0 )
  {
    s << "Spec         : " << spec_string( talent->id_spec ) << std::endl;
  }
  if ( talent->id_spec_starter[ 0 ] != 0 )
  {
    s << "Starter      : " << spec_string( talent->id_spec_starter ) << std::endl;
  }
  s << "Column       : " << talent->col << std::endl;
  s << "Row          : " << talent->row << std::endl;
  s << "Req. Points  : " << talent->req_points << std::endl;
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
  s << "Sel. Index   : " << talent->selection_index << std::endl;
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
          util::resource_type_string( util::power_type_to_resource( static_cast<power_e>( e->misc_value1() ) ) ) );
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

std::string_view spell_info::mechanic_str( unsigned mechanic )
{
  auto it = _mechanic_strings.find( mechanic );
  if ( it != _mechanic_strings.end() )
    return it->second;
  return {};
}

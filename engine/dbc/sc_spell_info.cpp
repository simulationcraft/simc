// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace {
struct proc_map_t
{
  int         flag;
  const char* proc;
};

const std::map<unsigned, const std::string> _targeting_map = {
  {  1,  "Self"                },
  {  5,  "Active Pet"          },
  {  6,  "Enemy"               },
  {  7,  "AOE enemy (instant)" },
  {  8,  "AOE Custom"          },
  { 15,  "AOE enemy"           },
  { 18,  "Custom"              },
  { 16,  "AOE enemy (instant)" },
  { 21,  "Friend"              },
  { 22,  "At Caster"           },
  { 28,  "AOE enemy"           },
  { 30,  "AOE friendly"        },
  { 31,  "AOE friendly"        },
  { 42,  "Water Totem"         },
  { 45,  "Chain Friendly"      },
  { 104, "Cone Front"          },
  { 53,  "At Enemy"            },
  { 56,  "Raid Members"        },
};

std::vector<std::string> _hotfix_effect_map = {
  "Id",
  "", // Hotfix field
  "Spell Id",
  "Index",
  "Type",
  "Sub Type",
  "Average",
  "Delta",
  "Bonus",
  "SP Coefficient",
  "AP Coefficient",
  "Period",
  "Min Radius",
  "Max Radius",
  "Base Value",
  "Misc Value",
  "Misc Value 2",
  "Affects Spells",
  "Trigger Spell",
  "Chain Multiplier",
  "Points per Combo Points",
  "Points per Level",
  "Die Sides",
  "Mechanic",
  "Chain Targets",
  "Target 1",
  "Target 2",
  "Value Multiplier"
};

std::vector<std::string> _hotfix_spell_map = {
  "Name",
  "Id",
  "", // Hotfix field
  "Velocity",
  "School",
  "Class",
  "Race",
  "Scaling Spell",
  "Max Scaling Level",
  "Spell Level",
  "Max Spell Level",
  "Min Range",
  "Max Range",
  "Cooldown",
  "GCD",
  "Category Cooldown",
  "Charges",
  "Charge Cooldown",
  "Category",
  "Duration",
  "Max stacks",
  "Proc Chance",
  "Proc Stacks",
  "Proc Flags",
  "Internal Cooldown",
  "RPPM",
  "", "", "", // Equipped items stuff, don't think we use these anywhere
  "Min Cast Time",
  "Max Cast Time",
  "", "", "", // Old cast-time related fields, now zero (cast_div, cast_scaling, cast_scaling_type)
  "", // Replace spell id, we're not flagging these as it's derived information
  "Attributes",
  "Affecting Spells",
  "Spell Family",
  "Stance Mask",
  "Mechanic",
  "Power Id",
  "Description",
  "Tooltip",
  "Variables",
  "Rank",
};

std::vector<std::string> _hotfix_power_map = {
  "Id",
  "Spell Id",
  "Aura Id",
  "", // Hotfix flags
  "Power Type",
  "Cost",
  "Max Cost",
  "Cost per Tick",
  "Percent Cost",
  "Max Percent Cost",
  "Percent Cost per Tick"
};

std::vector<std::string> _hotfix_artifact_power_map = {
  "",
  "Power Id",
  "Index",
  "Spell Id",
  "Value"
};

template <typename DATA_TYPE, typename MAP_TYPE>
std::ostringstream& hotfix_map_str( const DATA_TYPE* data, std::ostringstream& s, const std::vector<std::string>& map )
{
  if ( data -> _hotfix == 0 )
  {
    return s;
  }

  for ( size_t i = 0; i < map.size(); ++i )
  {
    MAP_TYPE shift = (static_cast<MAP_TYPE>( 1 ) << i);

    if ( ! ( data -> _hotfix & shift ) )
    {
      continue;
    }

    if ( s.tellp() > 0 )
    {
      s << ", ";
    }

    if ( map[ i ].empty() )
    {
      s << "Unknown(" << i << ")";
    }
    else
    {
      s << map[ i ];
      auto hotfix_entry = hotfix::hotfix_entry( data, static_cast<unsigned int>( i ) );
      if ( hotfix_entry != nullptr )
      {
        switch ( hotfix_entry -> field_type )
        {
          case hotfix::UINT:
            s << " (" << hotfix_entry -> orig_value.u << " -> " << hotfix_entry -> hotfixed_value.u << ")";
            break;
          case hotfix::INT:
            s << " (" << hotfix_entry -> orig_value.i << " -> " << hotfix_entry -> hotfixed_value.i << ")";
            break;
          case hotfix::FLOAT:
            s << " (" << hotfix_entry -> orig_value.f << " -> " << hotfix_entry -> hotfixed_value.f << ")";
            break;
          // Don't print out the changed string for now, seems pointless
          case hotfix::STRING:
            break;
        }
      }
    }
  }

  return s;
}

std::string spell_hotfix_map_str( const spell_data_t* spell )
{
  std::ostringstream s;

  if ( spell -> _hotfix == dbc::HOTFIX_SPELL_NEW )
  {
    s << "NEW SPELL";
  }
  else
  {
    hotfix_map_str<spell_data_t, uint64_t>( spell, s, _hotfix_spell_map );
  }

  return s.str();
}

std::string effect_hotfix_map_str( const spelleffect_data_t* effect )
{
  std::ostringstream s;

  if ( effect -> _hotfix == dbc::HOTFIX_EFFECT_NEW )
  {
    s << "NEW EFFECT";
  }
  else
  {
    hotfix_map_str<spelleffect_data_t, unsigned>( effect, s, _hotfix_effect_map );
  }

  return s.str();
}

std::string power_hotfix_map_str( const spellpower_data_t* power )
{
  std::ostringstream s;

  if ( power -> _hotfix == dbc::HOTFIX_POWER_NEW )
  {
    s << "NEW POWER";
  }
  else
  {
    hotfix_map_str<spellpower_data_t, unsigned>( power, s, _hotfix_power_map );
  }

  return s.str();
}

std::string artifact_power_hotfix_map_str( const artifact_power_rank_t* rank )
{
  std::ostringstream s;

  hotfix_map_str<artifact_power_rank_t, unsigned>( rank, s, _hotfix_artifact_power_map );

  return s.str();
}

std::string targeting_str( unsigned v )
{
  auto i = _targeting_map.find( v );
  if ( i != _targeting_map.end() )
  {
    return i -> second + "(" + util::to_string( v ) + ")";
  }

  return "Unknown(" + util::to_string( v ) + ")";
}

const struct proc_map_t _proc_flag_map[] =
{
  { PF_KILLED,               "Killed"                  },
  { PF_KILLING_BLOW,         "Killing Blow"            },
  { PF_MELEE,                "White Melee"             },
  { PF_MELEE_TAKEN,          "White Melee Taken"       },
  { PF_MELEE_ABILITY,        "Yellow Melee"            },
  { PF_MELEE_ABILITY_TAKEN,  "Yellow Melee Taken"      },
  { PF_RANGED,               "White Ranged"            },
  { PF_RANGED_TAKEN,         "White Ranged Taken "     },
  { PF_RANGED_ABILITY,       "Yellow Ranged"           },
  { PF_RANGED_ABILITY_TAKEN, "Yellow Ranged Taken"     },
  { PF_AOE_HEAL,             "AOE Heal"                },
  { PF_AOE_HEAL_TAKEN,       "AOE Heal Taken"          },
  { PF_AOE_SPELL,            "AOE Hostile Spell"       },
  { PF_AOE_SPELL_TAKEN,      "AOE Hostile Spell Taken" },
  { PF_HEAL,                 "Heal"                    },
  { PF_HEAL_TAKEN,           "Heal Taken"              },
  { PF_SPELL,                "Hostile Spell"           },
  { PF_SPELL_TAKEN,          "Hostile Spell Taken"     },
  { PF_PERIODIC,             "Periodic"                },
  { PF_PERIODIC_TAKEN,       "Periodic Taken"          },
  { PF_ANY_DAMAGE_TAKEN,     "Any Damage Taken"        },
  { PF_TRAP_TRIGGERED,       "Trap Triggered"          },
  { PF_JUMP,                 "Proc on jump"            },
  { PF_OFFHAND,              "Melee Off-Hand"          },
  { PF_DEATH,                "Death"                   },
  { 0,                       nullptr                         }
};

const struct { const char* name; player_e pt; } _class_map[] =
{
  { nullptr, PLAYER_NONE },
  { "Warrior", WARRIOR },
  { "Paladin", PALADIN },
  { "Hunter", HUNTER },
  { "Rogue", ROGUE },
  { "Priest", PRIEST },
  { "Death Knight", DEATH_KNIGHT },
  { "Shaman", SHAMAN },
  { "Mage", MAGE },
  { "Warlock", WARLOCK },
  { "Monk", MONK },
  { "Druid", DRUID },
  { "Demon Hunter", DEMON_HUNTER },
  { nullptr, PLAYER_NONE },
};

const char * _race_strings[] =
{
  "Human",
  "Orc",
  "Dwarf",
  "Night Elf",
  "Undead",
  "Tauren",
  "Gnome",
  "Troll",
  "Goblin",
  "Blood Elf",
  "Draenei",
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  "Worgen",
  "Pandaren",
  "Pandaren (Alliance)",
  "Pandaren (Horde)",
  nullptr
};

const char * _resource_strings[] =
{
  "Health", // -2
  nullptr,
  "Base Mana",
  "Rage",
  "Focus",
  "Energy",
  "Combo Points",
  "Rune",
  "Runic Power",
  "Soul Shard",
  "Eclipse",
  "Holy Power",
  nullptr,
  "Maelstrom",
  "Chi",
  "Insanity",
  "Burning Ember",
  "Demonic Fury",
  nullptr,
  "Fury",
  nullptr
};

const std::map<unsigned, std::string> _attribute_strings = {
  {   1, "Ranged Ability"           },
  {   5, "Tradeskill ability"       },
  {   6, "Passive"                  },
  {   7, "Hidden"                   },
  {  17, "Requires stealth"         },
  {  20, "Stop attacks"             },
  {  21, "Cannot dodge/parry/block" },
  {  28, "Cannot be used in combat" },
  {  31, "Cannot cancel aura"       },
  {  34, "Channeled"                },
  {  37, "Does not break stealth"   },
  {  38, "Channeled"                },
  {  93, "Cannot crit"              },
  {  95, "Food buff"                },
  { 105, "Not a proc"               },
  { 112, "Disable player procs"     },
  { 113, "Disable target procs"     },
  { 151, "Disable weapon procs"     },
  { 186, "Requires line of sight"   },
};

const char * _property_type_strings[] =
{
  "Generic Modifier",      "Spell Duration",        "Spell Generated Threat", "Spell Effect 1",      "Stack Amount",          // 0
  "Spell Range",           "Spell Radius",          "Spell Critical Chance",  "Spell Tick Time",     "Spell Pushback",        // 5
  "Spell Cast Time",       "Spell Cooldown",        "Spell Effect 2",         nullptr,                     "Spell Resource Cost",   // 10
  "Spell Critical Damage", "Spell Penetration",     "Spell Targets",          "Spell Proc Chance",   "Unknown 2",             // 15
  "Spell Target Bonus",    "Spell Global Cooldown", "Spell Periodic Damage",  "Spell Effect 3",      "Spell Power",           // 20
  nullptr,                       "Spell Proc Frequency",  "Spell Damage Taken",     "Spell Dispel Chance", nullptr,                       // 25
  nullptr,                       nullptr,                       "Spell Effect 4",         nullptr,                     "Runic Power Generation",                       // 30
  nullptr,                       nullptr,                       nullptr,                        nullptr,                     nullptr                        // 35
};

const char * _effect_type_strings[] =
{
  "None",               "Instant Kill",             "School Damage",        "Dummy",                    "Portal Teleport",       // 0
  "Teleport Units",     "Apply Aura",               "Environmental Damage", "Power Drain",              "Health Leech",          // 5
  "Direct Heal",        "Bind",                     "Portal",               "Ritual Base",              "Ritual Specialize",     // 10
  "Ritual Activate",    "Quest Complete",           "Weapon Damage",        "Resurrect",                "Extra Attacks",         // 15
  "Dodge",              "Evade",                    "Parry",                "Block",                    "Create Item",           // 20
  "Weapon Type",        "Defense",                  "Persistent Area Aura", "Summon",                   "Leap",                  // 25
  "Energize Power",     "Weapon Damage%",           "Trigger Missiles",     "Open Lock",                "Summon Item",           // 30
  "Apply Party Aura",   "Learn Spell",              "Spell Defense",        "Dispel",                   "Language",              // 35
  "Dual Wield",         nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 40
  nullptr,                    nullptr,                          nullptr,                      "Stealth",                  "Detect",                // 45
  nullptr,                    nullptr,                          "Guaranteed Hit",       nullptr,                          nullptr,                       // 50
  nullptr,                    "Summon Pet",               nullptr,                      "Weapon Damage",            nullptr,                       // 55
  nullptr,                    nullptr,                          "Power Burn",           "Threat",                   "Trigger Spell",         // 60
  "Apply Raid Aura",    nullptr,                          nullptr,                      "Interrupt Cast",           "Distract",              // 65
  "Pull",               "Pick Pocket",              nullptr,                      nullptr,                          nullptr,                       // 70
  nullptr,                    nullptr,                          "Server Side Script",   "Attack",                   nullptr,                       // 75
  "Add Combo Points",   nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 80
  "Summon Player",      nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 85
  nullptr,                    "Threat All",               nullptr,                      nullptr,                          "Self Resurrect",        // 90
  nullptr,                    "Charge",                   "Summon All Totems",    "Knock Back",               nullptr,                       // 95
  nullptr,                    "Feed Pet",                 "Dismiss Pet",          nullptr,                          nullptr,                       // 100
  nullptr,                    nullptr,                          nullptr,                      nullptr,                          "Summon Dead Pet",       // 105
  "Destroy All Totems", nullptr,                          nullptr,                      "Resurrect",                nullptr,                       // 110
  nullptr,                    nullptr,                          nullptr,                      nullptr,                          "Apply Pet Area Aura",   // 115
  nullptr,                    "Normalized Weapon Damage", nullptr,                      nullptr,                          "Pull Player",           // 120
  "Modify Threat",      "Steal Beneficial Aura",    nullptr,                      "Apply Friendly Area Aura", "Apply Enemy Area Aura", // 125
  "Redirect Threat",    nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 130
  "Call Pet",           "Direct Heal%",             "Energize Power%",      "Leap Back",                nullptr,                       // 135
  nullptr,                    nullptr,                          "Trigger Spell w/ Value", "Apply Owner Area Aura",    nullptr,                       // 140
  nullptr,                    "Activate Rune",            nullptr,                      nullptr,                          nullptr,                       // 145
  nullptr,                    "Trigger Spell",            nullptr,                      nullptr,                          nullptr,                       // 150
  "Titan Grip",         "Add Socket",               "Create Item",          nullptr,                          nullptr,                       // 155
  nullptr,                    nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 160
  nullptr,                    nullptr,                          nullptr,                                                                           // 165
};

const char * _effect_subtype_strings[] =
{
  "None",                       nullptr,                          "Possess",              "Periodic Damage",          "Dummy",                              // 0
  "Confuse",                    "Charm",                    "Fear",                 "Periodic Heal",            "Attack Speed",                       // 5
  "Threat",                     "Taunt",                    "Stun",                 "Damage Done",              "Damage Taken",                       // 10
  "Damage Shield",              "Stealth",                  "Stealth Detection",    "Invisibility",             "Invisibility Detection",             // 15
  "Periodic Heal%",             nullptr,                          "Resistance",           "Periodic Trigger Spell",   "Periodic Energize Power",            // 20
  "Pacify",                     "Root",                     "Silence",              "Spell Reflection",         "Attribute",                          // 25
  "Skill",                      "Increase Speed%",          "Increase Mounted Speed%", "Decrease Movement Speed%", "Increase Health",                 // 30
  "Increase Energy",            "Shapeshift",               "Immunity Against External Movement", nullptr,            "School Immunity",                    // 35
  nullptr,                            nullptr,                          "Proc Trigger Spell",   "Proc Trigger Damage",      "Track Creatures",                    // 40
  nullptr,                            nullptr,                          "Modify Parry%",        nullptr,                          "Modify Dodge%",                      // 45
  "Modify Critical Heal Bonus", "Modify Block%",            "Modify Crit%",         "Periodic Health Leech",    "Modify Hit%",                        // 50
  "Modify Spell Hit%",          nullptr,                          "Modify Spell Crit%",   nullptr,                          nullptr,                                    // 55
  "Pacify Silence",             "Scale%",                   nullptr,                      nullptr,                          "Periodic Mana Leech",                // 60
  "Modify Spell Haste%",        "Feign Death",              "Disarm",               "Stalked",                  "Absorb Damage",                      // 65
  nullptr,                            nullptr,                          "Modify Power Cost%",   "Modify Power Cost",        "Reflect Spells",                     // 70
  nullptr,                            nullptr,                          "Mechanic Immunity",    nullptr,                          "Modify Damage Done%",   // 75
  "Modify Attribute%",          nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 80
  "Modify Power Regeneration",  nullptr,                          "Modify Damage Taken%", nullptr,                          nullptr,                       // 85
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 90
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          "Modify Attack Power",   // 95
  nullptr,                            "Modify Armor%",            nullptr,                      nullptr,                          "Modify Attack Power",   // 100
  nullptr,                            nullptr,                          "Add Flat Modifier",    "Add Percent Modifier",     nullptr,                       // 105
  "Mod Power Regen",                            nullptr,                          nullptr,                      nullptr,                          nullptr,       // 110
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 115
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          "Modify Ranged Attack Power", // 120
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 125
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 130
  "Modify Healing Power",       "Modify Healing% Done",     "Modify Total Stat%",   "Modify Melee Haste%",      nullptr,                       // 135
  "Modify Ranged Haste%",       nullptr,                          "Modify Base Resistance",nullptr,                         nullptr,                       // 140
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 145
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 150
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 155
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 160
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 165
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 170
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 175
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 180
  nullptr,                            nullptr,                          "Modify Attacker Melee Crit Chance",nullptr,              "Modify Rating",         // 185
  nullptr,                            nullptr,                          "Modify Ranged and Melee Haste%",                   "Modify All Haste%", nullptr,  // 190
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 195
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 200
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 205
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 210
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 215
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 220
  nullptr,            "Periodic Dummy",                           nullptr,                      nullptr,                          nullptr,                       // 225
  nullptr,                            "Trigger Spell with Value", nullptr,                      nullptr,                          nullptr,                       // 230
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 235
  "Modify Expertise%",          nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 240
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 245
  "Increase Max Health (Stacking)",nullptr,                       nullptr,                      nullptr,                          nullptr,                       // 250
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 255
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 260
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 265
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 270
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 275
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 280
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 285
  "Modify Critical Strike%",    nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 290
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 295
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 300
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 305
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 310
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          "Modify Melee Attack Speed%",                       // 315
  "Modify Ranged Attack Speed%",nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 320
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 325
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 330
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 335
  nullptr,                            nullptr,                          "Modify Ranged and Melee Attack Speed%", nullptr,         nullptr,                       // 340
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 345
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 350
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 355
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 360
  nullptr,                            "Override Spell Power per Attack Power%",                          nullptr,                      nullptr,                          nullptr,                       // 365
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 370
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 375
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 380
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 385
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 390
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 395
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 400
  "Modify Combat Rating Multiplier",  nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 405
  nullptr,                            "Modify Cooldown Charge",         nullptr,                      nullptr,                          nullptr,                       // 410
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 415
  nullptr,                            "Modify Absorb% Done",      "Modify Absorb% Done",  nullptr,                          nullptr,                       // 420
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 425
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 430
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 435
  nullptr,                            "Modify Multistrike%",      nullptr,                      nullptr,                          nullptr,                       // 440
  nullptr,                            nullptr,                          nullptr,                      nullptr,                          nullptr,                       // 445
};

std::string mechanic_str( unsigned mechanic ) {
  switch ( static_cast<spell_mechanic>( mechanic ) )
  {
    case MECHANIC_BLEED: return "Bleed";
    default:             return "Unknown(" + util::to_string( mechanic ) + ")";
  }
}

std::string spell_flags( const spell_data_t* spell )
{
  std::ostringstream s;

  s << "[";

  if ( spell -> scaling_class() != 0 )
    s << "Scaling Spell (" << spell -> scaling_class() << "), ";

  if ( spell -> class_family() != 0 )
    s << "Spell Family (" << spell -> class_family() << "), ";

  if ( spell -> flags( SPELL_ATTR_PASSIVE ) )
    s << "Passive, ";

  if ( spell -> flags( SPELL_ATTR_HIDDEN ) )
    s << "Hidden, ";

  if ( s.tellp() > 1 )
  {
    s.seekp( -2, std::ios_base::cur );
    s << "]";
  }
  else
    return std::string();

  return s.str();
}

void spell_flags_xml( const spell_data_t* spell, xml_node_t* parent )
{
  if ( spell -> scaling_class() != 0 )
    parent -> add_parm( "scaling_spell", spell -> scaling_class() );

  if ( spell -> flags( SPELL_ATTR_PASSIVE ) )
    parent -> add_parm( "passive", "true" );
}

} // unnamed namespace

std::ostringstream& spell_info::effect_to_str( const dbc_t& dbc,
                                               const spell_data_t*       spell,
                                               const spelleffect_data_t* e,
                                               std::ostringstream&       s,
                                               int level )
{
  std::streamsize ssize = s.precision( 7 );
  char tmp_buffer[512],
       tmp_buffer2[64];

  snprintf( tmp_buffer2, sizeof( tmp_buffer2 ), "(id=%u)", e -> id() );
  snprintf( tmp_buffer, sizeof( tmp_buffer ), "#%d %-*s: ", (int16_t)e -> index() + 1, 14, tmp_buffer2 );
  s << tmp_buffer;

  if ( e -> type() < static_cast< int >( sizeof( _effect_type_strings ) / sizeof( const char* ) ) &&
       _effect_type_strings[ e -> type() ] != nullptr )
  {
    s << _effect_type_strings[ e -> type() ];
    // Put some nice handling on some effect types
    switch ( e -> type() )
    {
      case E_SCHOOL_DAMAGE:
        s << ": " << util::school_type_string( spell -> get_school_type() );
        break;
      case E_TRIGGER_SPELL:
      case E_TRIGGER_SPELL_WITH_VALUE:
        if ( e -> trigger_spell_id() )
        {
          if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
            s << ": " << dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
          else
            s << ": (" << e -> trigger_spell_id() << ")";
        }
        break;
      default:
        break;
    }
  }
  else
    s << "Unknown effect type";
  s << " (" << e -> type() << ")";

  if ( e -> subtype() > 0 )
  {
    if (  e -> subtype() < static_cast< int >( sizeof( _effect_subtype_strings ) / sizeof( const char* ) ) &&
          _effect_subtype_strings[ e -> subtype() ] != nullptr )
    {
      s << " | " << _effect_subtype_strings[ e -> subtype() ];
      switch ( e -> subtype() )
      {
        case A_PERIODIC_DAMAGE:
          s << ": " << util::school_type_string( spell -> get_school_type() );
          if ( e -> period() != timespan_t::zero() )
            s << " every " << e -> period().total_seconds() << " seconds";
          break;
        case A_PERIODIC_HEAL:
        case A_PERIODIC_ENERGIZE:
        case A_PERIODIC_DUMMY:
        case A_OBS_MOD_HEALTH:
          if ( e -> period() != timespan_t::zero() )
            s << ": every " << e -> period().total_seconds() << " seconds";
          break;
        case A_PROC_TRIGGER_SPELL:
          if ( e -> trigger_spell_id() )
          {
            if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
            {
              s << ": " << dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
            }
            else
            {
              s << ": (" << e -> trigger_spell_id() << ")";
            }
          }
          break;
        case A_PERIODIC_TRIGGER_SPELL:
          if ( e -> trigger_spell_id() )
          {
            if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
            {
              s << ": " << dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
              if ( e -> period() != timespan_t::zero() )
                s << " every " << e -> period().total_seconds() << " seconds";
            }
            else
              s << ": (" << e -> trigger_spell_id() << ")";
          }
          break;
        case A_ADD_FLAT_MODIFIER:
        case A_ADD_PCT_MODIFIER:
          if ( e -> misc_value1() < static_cast< int >( sizeof( _property_type_strings ) / sizeof( const char* ) ) &&
               _property_type_strings[ e -> misc_value1() ] != nullptr )
            s << ": " << _property_type_strings[ e -> misc_value1() ];
          break;
        default:
          break;
      }
    }
    else
      s << " | Unknown effect sub type";

    s << " (" << e -> subtype() << ")";
  }

  s << std::endl;

  s << "                   Base Value: " << e -> base_value();
  s << " | Scaled Value: ";

  if ( level <= MAX_LEVEL )
  {
    double v_min = dbc.effect_min( e, level );
    double v_max = dbc.effect_max( e, level );

    s << v_min;
    if ( v_min != v_max )
      s << " - " << v_max;
  }
  else
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( level );
    double item_budget = ilevel_data.p_epic[ 0 ];

    s << item_budget * e -> m_average();
  }

  if ( e -> m_average() != 0 || e -> m_delta() != 0 )
  {
    s << " (avg=" << e -> m_average();
    if ( e -> m_delta() != 0 )
      s << ", dl=" << e -> m_delta();
    s << ")";
  }

  if ( level <= MAX_LEVEL )
  {
    if ( e -> m_unk() )
    {
      s << " | Bonus Value: " << dbc.effect_bonus( e -> id(), level );
      s << " (" << e -> m_unk() << ")";
    }
  }

  if ( e -> real_ppl() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%f", e -> real_ppl() );
    s << " | Points Per Level: " << e -> real_ppl();
  }

  if ( e -> die_sides() != 0 )
  {
    s << " | Value Range: " << e -> die_sides();
  }

  if ( e -> m_value() != 0 )
  {
    s << " | Value Multiplier: " << e -> m_value();
  }

  if ( e -> sp_coeff() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%.5f", e -> sp_coeff() );
    s << " | SP Coefficient: " << tmp_buffer;
  }

  if ( e -> ap_coeff() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%.5f", e -> ap_coeff() );
    s << " | AP Coefficient: " << tmp_buffer;
  }

  if ( e -> chain_multiplier() != 0 && e -> chain_multiplier() != 1.0 )
    s << " | Chain Multiplier: " << e -> chain_multiplier();

  if ( e -> misc_value1() != 0 || e -> type() == E_ENERGIZE )
  {
    if ( e -> subtype() == A_MOD_DAMAGE_DONE ||
         e -> subtype() == A_MOD_DAMAGE_TAKEN ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_DONE ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_TAKEN )
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%#.x", e -> misc_value1() );
    else if ( e -> type() == E_ENERGIZE )
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%s", util::resource_type_string( util::translate_power_type( static_cast<power_e>( e -> misc_value1() ) ) ) );
    else
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%d", e -> misc_value1() );
    s << " | Misc Value: " << tmp_buffer;
  }

  if ( e -> misc_value2() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%#.x", e -> misc_value2() );
    s << " | Misc Value 2: " << tmp_buffer;
  }

  if ( e -> pp_combo_points() != 0 )
    s << " | Points Per Combo Point: " << e -> pp_combo_points();

  if ( e -> trigger_spell_id() != 0 )
    s << " | Trigger Spell: " << e -> trigger_spell_id();

  if ( e -> radius() > 0 || e -> radius_max() > 0 )
  {
    s << " | Radius: " << e -> radius();
    if ( e -> radius_max() > 0 && e -> radius_max() != e -> radius() )
      s << " - " << e -> radius_max();
    s << " yards";
  }

  if ( e -> mechanic() > 0 )
  {
    s << " | Mechanic: " << mechanic_str( e -> mechanic() );
  }

  if ( e -> chain_target() > 0 )
  {
    s << " | Chain Targets: " << e -> chain_target();
  }

  if ( e -> target_1() != 0 || e -> target_2() != 0 )
  {
    s << " | Target: ";
    if ( e -> target_1() && ! e -> target_2() )
    {
      s << targeting_str( e -> target_1() );
    }
    else if ( ! e -> target_1() && e -> target_2() )
    {
      s << "[" << targeting_str( e -> target_2() ) << "]";
    }
    else
    {
      s << targeting_str( e -> target_1() ) << " -> " << targeting_str( e -> target_2() );
    }
  }

  s << std::endl;

  std::vector< const spell_data_t* > affected_spells = dbc.effect_affects_spells( spell -> class_family(), e );
  if ( affected_spells.size() > 0 )
  {
    s << "                   Affected Spells: ";
    for ( size_t i = 0, end = affected_spells.size(); i < end; i++ )
    {
      s << affected_spells[ i ] -> name_cstr() << " (" << affected_spells[ i ] -> id() << ")";
      if ( i < end - 1 )
        s << ", ";
    }
    s << std::endl;
  }

  if ( e -> _hotfix != 0 )
  {
    s << "                   Hotfixed: ";
    s << effect_hotfix_map_str( e );
    s << std::endl;
  }


  s.precision( ssize );

  return s;
}

std::string spell_info::to_str( const dbc_t& dbc, const spell_data_t* spell, int level )
{

  std::ostringstream s;
  player_e pt = PLAYER_NONE;

  if ( spell -> scaling_class() != 0 && spell -> level() > static_cast< unsigned >( level ) )
  {
    s << std::endl << "Too low spell level " << level << " for " << spell -> name_cstr() << ", minimum is " << spell -> level() << "." << std::endl << std::endl;
    return s.str();
  }

  std::string name_str = spell -> name_cstr();
  if ( spell -> rank_str() )
    name_str += " (" + std::string( spell -> rank_str() ) + ")";
  s <<   "Name             : " << name_str << " (id=" << spell -> id() << ") " << spell_flags( spell ) << std::endl;

  if ( spell -> _hotfix != 0 )
  {
    auto hotfix_str = spell_hotfix_map_str( spell );
    if ( ! hotfix_str.empty() )
    {
      s << "Hotfixed         : " << hotfix_str << std::endl;
    }
  }

  if ( spell -> replace_spell_id() > 0 )
  {
    s << "Replaces         : " <<  dbc.spell( spell -> replace_spell_id() ) -> name_cstr();
    s << " (id=" << spell -> replace_spell_id() << ")" << std::endl;
  }

  if ( spell -> class_mask() )
  {
    bool pet_ability = false;
    s << "Class            : ";

    if ( dbc.is_specialization_ability( spell -> id() ) )
    {
      std::vector<specialization_e> spec_list;
      std::vector<specialization_e>::const_iterator iter;
      dbc.ability_specialization( spell -> id(), spec_list );

      for ( iter = spec_list.begin(); iter != spec_list.end(); ++iter )
      {
        if ( *iter == PET_FEROCITY || *iter == PET_CUNNING || *iter == PET_TENACITY )
          pet_ability = true;
        s << util::inverse_tokenize( dbc::specialization_string( *iter ) ) << " ";
      }
      spec_list.clear();
    }

    for ( unsigned int i = 1; i < sizeof_array( _class_map ); i++ )
    {
      if ( ( spell -> class_mask() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
      {
        s << _class_map[ i ].name << ", ";
        if ( ! pt )
          pt = _class_map[ i ].pt;
      }
    }

    s.seekp( -2, std::ios_base::cur );
    if ( pet_ability )
      s << " Pet";
    s << std::endl;
  }

  if ( spell -> race_mask() )
  {
    s << "Race             : ";
    for ( unsigned int i = 0; i < 24; i++ )
    {
      if ( ( spell -> race_mask() & ( 1 << i ) ) && _race_strings[ i ] )
        s << _race_strings[ i ] << ", ";
    }

    s.seekp( -2, std::ios_base::cur );
    s << std::endl;
  }

  std::string school_string = util::school_type_string( spell -> get_school_type() );
  school_string[ 0 ] = std::toupper( school_string[ 0 ] );
  s << "School           : " << school_string << std::endl;

  for ( size_t i = 0; spell -> _power && i < spell -> _power -> size(); i++ )
  {
    const spellpower_data_t* pd = spell -> _power -> at( i );

    s << "Resource         : ";
    if ( pd -> type() == POWER_MANA )
      s << pd -> cost() * 100.0 << "%";
    else
      s << pd -> cost();

    s << " ";

    if ( pd -> max_cost() != 0 )
    {
      s << "- ";
      if ( pd -> type() == POWER_MANA )
        s << ( pd -> cost() + pd -> max_cost() ) * 100.0 << "%";
      else
        s << ( pd -> cost() + pd -> max_cost() );
      s << " ";
    }

    if ( ( pd -> type() + POWER_OFFSET ) < static_cast< int >( sizeof( _resource_strings ) / sizeof( const char* ) ) &&
        _resource_strings[ pd -> type() + POWER_OFFSET ] != nullptr )
      s << _resource_strings[ pd -> type() + POWER_OFFSET ];
    else
      s << "Unknown (" << pd -> type() << ")";

    if ( pd -> type() == POWER_MANA && pd -> cost() != 0 )
    {
      s << " (" << floor( dbc.resource_base( pt, level ) * pd -> cost() ) << " @Level " << level << ")";
    }

    if ( pd -> cost_per_tick() != 0 )
    {
      s << " and ";

      if ( pd -> type() == POWER_MANA )
        s << pd -> cost_per_tick() * 100.0 << "%";
      else
        s << pd -> cost_per_tick();

      s << " ";

      if ( ( pd -> type() + POWER_OFFSET ) < static_cast< int >( sizeof( _resource_strings ) / sizeof( const char* ) ) &&
          _resource_strings[ pd -> type() + POWER_OFFSET ] != nullptr )
        s << _resource_strings[ pd -> type() + POWER_OFFSET ];
      else
        s << "Unknown (" << pd -> type() << ")";

      if ( pd -> type() == POWER_MANA )
      {
        s << " (" << floor( dbc.resource_base( pt, level ) * pd -> cost_per_tick() ) << " @Level " << level << ")";
      }

      s << " per second or tick";
    }

    if ( pd -> aura_id() > 0 && dbc.spell( pd -> aura_id() ) -> id() == pd -> aura_id() )
      s << " w/ " << dbc.spell( pd -> aura_id() ) -> name_cstr() << " (id=" << pd -> aura_id() << ")";

    if ( pd -> _hotfix != 0 )
    {
      auto hotfix_str = power_hotfix_map_str( pd );
      if ( ! hotfix_str.empty() )
      {
        s << " [Hotfixed: " << hotfix_str << "]";
      }
    }

    s << std::endl;
  }

  if ( spell -> level() > 0 )
  {
    s << "Spell Level      : " << ( int ) spell -> level();
    if ( spell -> max_level() > 0 )
      s << " (max " << ( int ) spell -> max_level() << ")";

    s << std::endl;
  }

  if ( spell -> max_scaling_level() > 0 )
  {
    s << "Max Scaling Level: " << ( int ) spell -> max_scaling_level();
    s << std::endl;
  }

  if ( spell -> min_range() || spell -> max_range() )
  {
    s << "Range            : ";
    if ( spell -> min_range() )
      s << ( int ) spell -> min_range() << " - ";

    s << ( int ) spell -> max_range() << " yards" << std::endl;
  }

  if ( spell -> _cast_min > 0 || spell -> _cast_max > 0 )
  {
    s << "Cast Time        : ";

    if ( spell -> _cast_div )
      s << spell -> cast_time( level ).total_seconds();
    else if ( spell -> _cast_min != spell -> _cast_max )
      s << spell -> _cast_min / 1000.0 << " - " << spell -> _cast_max / 1000.0;
    else
      s << spell -> _cast_max / 1000.0;

    s << " seconds" << std::endl;
  }
  else if ( spell -> _cast_min < 0 || spell -> _cast_max < 0 )
    s << "Cast Time        : Ranged Shot" << std::endl;

  if ( spell -> gcd() != timespan_t::zero() )
    s << "GCD              : " << spell -> gcd().total_seconds() << " seconds" << std::endl;

  if ( spell -> missile_speed() )
    s << "Velocity         : " << spell -> missile_speed() << " yards/sec"  << std::endl;

  if ( spell -> duration() != timespan_t::zero() )
  {
    s << "Duration         : ";
    if ( spell -> duration() < timespan_t::zero() )
      s << "Aura (infinite)";
    else
      s << spell -> duration().total_seconds() << " seconds";

    s << std::endl;
  }

  if ( spell -> cooldown() > timespan_t::zero() )
    s << "Cooldown         : " << spell -> cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell -> charges() > 0 || spell -> charge_cooldown() > timespan_t::zero() )
  {
    s << "Charges          : " << spell -> charges();
    if ( spell -> charge_cooldown() > timespan_t::zero() )
      s << " (" << spell -> charge_cooldown().total_seconds() << " seconds cooldown)";
    s << std::endl;
  }

  if ( spell -> category() > 0 )
    s << "Category         : " << spell -> category() << std::endl;

  if ( spell -> category_cooldown() > timespan_t::zero() )
    s << "Category Cooldown: " << spell -> category_cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell -> internal_cooldown() > timespan_t::zero() )
    s << "Internal Cooldown: " << spell -> internal_cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell -> initial_stacks() > 0 || spell -> max_stacks() )
  {
    s << "Stacks           : ";
    if ( spell -> initial_stacks() )
      s << spell -> initial_stacks() << " initial, ";

    if ( spell -> max_stacks() )
      s << spell -> max_stacks() << " maximum, ";
    else if ( spell -> initial_stacks() && ! spell -> max_stacks() )
      s << spell -> initial_stacks() << " maximum, ";

    s.seekp( -2, std::ios_base::cur );
  
    s << std::endl;
  }

  if ( spell -> proc_chance() > 0 )
    s << "Proc Chance      : " << spell -> proc_chance() * 100 << "%" << std::endl;

  if ( spell -> real_ppm() != 0 )
  {
    s << "Real PPM         : " << spell -> real_ppm();
    bool has_modifiers = false;
    std::vector<const rppm_modifier_t*> modifiers = dbc.real_ppm_modifiers( spell -> id() );
    for ( size_t i = 0; i < modifiers.size(); ++i )
    {
      const rppm_modifier_t* rppm_modifier = modifiers[ i ];

      switch ( rppm_modifier -> modifier_type )
      {
        case RPPM_MODIFIER_HASTE:
          if ( ! has_modifiers )
          {
            s << " (";
          }
          s << "Haste multiplier, ";
          has_modifiers = true;
          break;
        case RPPM_MODIFIER_CRIT:
          if ( ! has_modifiers )
          {
            s << " (";
          }
          s << "Crit multiplier, ";
          has_modifiers = true;
          break;
        case RPPM_MODIFIER_ILEVEL:
          if ( ! has_modifiers )
          {
            s << " (";
          }
          s << "Itemlevel multiplier [base=" << rppm_modifier -> type << "], ";
          has_modifiers = true;
          break;
        case RPPM_MODIFIER_SPEC:
        {
          if ( ! has_modifiers )
          {
            s << " (";
          }

          std::streamsize decimals = 3;
          double rppm_val = spell -> real_ppm() * ( 1.0 + rppm_modifier -> coefficient );
          if ( rppm_val >= 10 )
            decimals += 2;
          else if ( rppm_val >= 1 )
            decimals += 1;
          s.precision( decimals );
          s << util::specialization_string( static_cast<specialization_e>( rppm_modifier -> type ) ) << ": " << rppm_val << ", ";
          has_modifiers = true;
          break;
        }
        default:
          break;
      }
    }

    if ( has_modifiers )
    {
      s.seekp( -2, std::ios_base::cur );
      s << ")";
    }
    s << std::endl;
  }

  if ( spell -> stance_mask() > 0 )
  {
    s << "Stance Mask      : 0x";
    std::streamsize ss = s.width();
    s.width( 8 );
    s << std::hex << std::setfill('0') << spell -> stance_mask() << std::endl << std::dec;
    s.width( ss );
  }

  if ( spell -> mechanic() > 0 )
  {
    s << "Mechanic         : " << mechanic_str( spell -> mechanic() ) << std::endl;
  }

  if ( spell -> power_id() > 0 )
  {
    auto powers = dbc.artifact_power_ranks( spell -> power_id() );
    s << "Artifact Power   : Id: " << spell -> power_id() << ", Max Rank: " << ( powers.back() -> index() + 1 );
    if ( powers.size() > 1 )
    {
      std::vector<std::string> artifact_hotfixes;
      std::ostringstream value_str;
      std::ostringstream spells_str;
      value_str << ", Values: [ ";
      bool has_multiple_spells = false;
      for ( size_t i = 0, end = powers.size(); i < end; ++i )
      {
        const auto& power = powers[ i ];

        if ( power -> value() != 0 )
        {
          value_str << power -> value();
        }
        else if ( power -> id_spell() > 0 && power -> id_spell() != spell -> id() )
        {
          auto rank_spell = dbc.spell( power -> id_spell() );
          value_str << rank_spell -> effectN( 1 ).base_value();
          spells_str << rank_spell -> id();
          has_multiple_spells = true;
        }
        else
        {
          value_str << spell -> effectN( 1 ).base_value();
        }

        if ( power -> _hotfix != 0 )
        {
          auto hotfix_str = artifact_power_hotfix_map_str( power );
          std::ostringstream s;
          s << "Rank " << ( power -> index() + 1 ) << ": " << hotfix_str;
          artifact_hotfixes.push_back( s.str() );
        }

        if ( i < powers.size() - 1 )
        {
          value_str << ", ";
          if ( spells_str.tellp() > 0 )
          {
            spells_str << ", ";
          }
        }
      }

      value_str << " ]";
      s << value_str.str();
      if ( spells_str.tellp() > 0 )
      {
        s << ", Rank spells: [ " << spell -> id() << ", " << spells_str.str() << " ]";
      }

      if ( artifact_hotfixes.size() > 0 )
      {
        s << std::endl;

        std::string str;
        for ( size_t i = 0; i < artifact_hotfixes.size(); ++i )
        {
          str += artifact_hotfixes[ i ];
          if ( i < artifact_hotfixes.size() - 1 )
          {
            str += ", ";
          }
        }
        s << "                 : Hotfixes: " << str;
      }
    }
    s << std::endl;
  }

  if ( spell -> proc_flags() > 0 )
  {
    s << "Proc Flags       : ";
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( spell -> proc_flags() & ( 1 << flag ) )
        s << "x";
      else
        s << ".";

      if ( ( flag + 1 ) % 8 == 0 )
        s << " ";
    }
    s << std::endl;
    s << "                 : ";
    for ( size_t i = 0; i < sizeof_array( _proc_flag_map ); i++ )
    {
      if ( spell -> proc_flags() & _proc_flag_map[ i ].flag )
      {
        s << _proc_flag_map[ i ].proc;
        s << ", ";
      }
    }
    std::streampos x = s.tellp();
    s.seekp( x - std::streamoff( 2 ) );
    s << std::endl;
  }

  if ( spell -> class_family() > 0 )
  {
    std::vector< const spelleffect_data_t* > affecting_effects = dbc.effects_affecting_spell( spell );
    std::vector< unsigned > spell_ids;
    if ( affecting_effects.size() > 0 )
    {
      s << "Affecting spells : ";

      for ( size_t i = 0, end = affecting_effects.size(); i < end; i++ )
      {
        const spelleffect_data_t* effect = affecting_effects[ i ];
        if ( std::find( spell_ids.begin(), spell_ids.end(), effect -> spell() -> id() ) != spell_ids.end() )
          continue;

        s << effect -> spell() -> name_cstr() << " (" << effect -> spell() -> id() << ")";
        if ( i < end - 1 )
          s << ", ";

        spell_ids.push_back( effect -> spell() -> id() );
      }

      s << std::endl;
    }
  }

  if ( spell -> _driver )
  {
    s << "Triggered by     : ";
    for ( size_t driver_idx = 0; driver_idx < spell -> _driver -> size(); ++driver_idx )
    {
      const spell_data_t* driver = spell -> _driver -> at( driver_idx );
      s << driver -> name_cstr() << " (" << driver -> id() << ")";
      if ( driver_idx < spell -> _driver -> size() - 1 )
      {
        s << ", ";
      }
    }
    s << std::endl;
  }

  s << "Attributes       : ";
  std::string attr_str;
  for ( unsigned i = 0; i < NUM_SPELL_FLAGS; i++ )
  {
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( spell -> attribute( i ) & ( 1 << flag ) )
      {
        s << "x";
        size_t attr_idx = i * 32 + flag;
        auto it = _attribute_strings.find( static_cast<unsigned int>( attr_idx ) );
        if ( it != _attribute_strings.end() )
        {
          if ( ! attr_str.empty() )
            attr_str += ", ";

          attr_str += it -> second;
          attr_str += " (" + util::to_string( attr_idx ) + ")";
        }
      }
      else
        s << ".";

      if ( ( flag + 1 ) % 8 == 0 )
        s << " ";

      if ( flag == 31 && i % 2 == 0 )
        s << "  ";
    }

    if ( ( i + 1 ) % 2 == 0 && i < NUM_SPELL_FLAGS - 1 )
      s << std::endl << "                 : ";
  }

  if ( ! attr_str.empty() )
      s << std::endl << "                 : " + attr_str;
  s << std::endl;

  s << "Effects          :" << std::endl;

  for ( size_t i = 0; i < spell -> effect_count(); i++ )
  {
    const spelleffect_data_t* e;
    uint32_t effect_id;
    if ( ! ( effect_id = spell -> effectN( i + 1 ).id() ) )
      continue;
    else
      e = &( spell -> effectN( i + 1 ) );

    spell_info::effect_to_str( dbc, spell, e, s, level );
  }

  if ( spell -> desc() )
    s << "Description      : " << spell -> desc() << std::endl;

  if ( spell -> tooltip() )
    s << "Tooltip          : " << spell -> tooltip() << std::endl;

  if ( spell -> desc_vars() )
    s << "Variables        : " << spell -> desc_vars() << std::endl;

  s << std::endl;

  return s.str();
}

std::string spell_info::talent_to_str( const dbc_t& /* dbc */, const talent_data_t* talent, int /* level */ )
{
  std::ostringstream s;

  s <<   "Name         : " << talent -> name_cstr() << " (id=" << talent -> id() << ") " << std::endl;

  if ( talent -> mask_class() )
  {
    s << "Class        : ";
    for ( unsigned int i = 1; i < sizeof_array( _class_map ); i++ )
    {
      if ( ( talent -> mask_class() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
        s << _class_map[ i ].name << ", ";
    }

    s.seekp( -2, std::ios_base::cur );
    s << std::endl;
  }

  s << "Column       : " << talent -> col() + 1    << std::endl;
  s << "Row          : " << talent -> row() + 1    << std::endl;
  s << "Spell        : "        << talent -> spell_id()   << std::endl;
  if ( talent -> replace_id() > 0 )
    s << "Replaces     : "   << talent -> replace_id() << std::endl;
  s << "Spec         : " << util::specialization_string( talent -> specialization() ) << std::endl;
  s << std::endl;

  return s.str();
}

std::string spell_info::set_bonus_to_str( const dbc_t&, const item_set_bonus_t* set_bonus, int /* level */ )
{
  std::ostringstream s;

  s << "Name          : " << set_bonus -> set_name << std::endl;

  player_e player_type = static_cast<player_e>( set_bonus -> class_id);
  s << "Class         : " << util::player_type_string( player_type ) << std::endl;
  s << "Tier          : " << set_bonus -> tier << std::endl;
  s << "Bonus Level   : " << set_bonus -> bonus << std::endl;
  if ( set_bonus -> spec > 0 )
    s << "Spec          : " << util::specialization_string( static_cast<specialization_e>(set_bonus -> spec ) ) << std::endl;
  s << "Spell ID      : " << set_bonus -> spell_id << std::endl;

  s << std::endl;

  return s.str();
}

void spell_info::effect_to_xml( const dbc_t& dbc,
                                const spell_data_t*       spell,
                                const spelleffect_data_t* e,
                                xml_node_t* parent,
                                int level )
{
  xml_node_t* node = parent -> add_child( "effect" );

  node -> add_parm( "number", e -> index() + 1 );
  node -> add_parm( "id", e -> id() );
  node -> add_parm( "type", e -> type() );

  if ( e -> type() < static_cast< int >( sizeof( _effect_type_strings ) / sizeof( const char* ) ) &&
       _effect_type_strings[ e -> type() ] != nullptr )
  {
    node -> add_parm( "type_text", _effect_type_strings[ e -> type() ] );
    // Put some nice handling on some effect types
    switch ( e -> type() )
    {
      case E_SCHOOL_DAMAGE:
        node -> add_parm( "school", spell -> get_school_type() );
        node -> add_parm( "school_text", util::school_type_string( spell -> get_school_type() ) );
        break;
      case E_TRIGGER_SPELL:
      case E_TRIGGER_SPELL_WITH_VALUE:
        if ( e -> trigger_spell_id() )
        {
          if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
            node -> add_parm( "trigger_spell_name", dbc.spell( e -> trigger_spell_id() ) -> name_cstr() );
        }
        break;
      default:
        break;
    }
  }
  else
    node -> add_parm( "type_text", "Unknown" );

  node -> add_parm( "sub_type", e -> subtype() );

  if ( e -> subtype() > 0 )
  {
    if (  e -> subtype() < static_cast< int >( sizeof( _effect_subtype_strings ) / sizeof( const char* ) ) &&
          _effect_subtype_strings[ e -> subtype() ] != nullptr )
    {
      node -> add_parm( "sub_type_text", _effect_subtype_strings[ e -> subtype() ] );
      switch ( e -> subtype() )
      {
        case A_PERIODIC_DAMAGE:
          node -> add_parm( "school", spell -> get_school_type() );
          node -> add_parm( "school_text", util::school_type_string( spell -> get_school_type() ) );
          if ( e -> period() != timespan_t::zero() )
            node -> add_parm( "period", e -> period().total_seconds() );
          break;
        case A_PERIODIC_ENERGIZE:
        case A_PERIODIC_DUMMY:
          if ( e -> period() != timespan_t::zero() )
            node -> add_parm( "period", e -> period().total_seconds() );
          break;
        case A_PROC_TRIGGER_SPELL:
          if ( e -> trigger_spell_id() )
          {
            if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
              node -> add_parm( "trigger_spell_name", dbc.spell( e -> trigger_spell_id() ) -> name_cstr() );
          }
          break;
        case A_PERIODIC_TRIGGER_SPELL:
          if ( e -> trigger_spell_id() )
          {
            if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
            {
              node -> add_parm( "trigger_spell_name", dbc.spell( e -> trigger_spell_id() ) -> name_cstr() );
              if ( e -> period() != timespan_t::zero() )
                node -> add_parm( "period", e -> period().total_seconds() );
            }
          }
          break;
        case A_ADD_FLAT_MODIFIER:
        case A_ADD_PCT_MODIFIER:
          if ( e -> misc_value1() < static_cast< int >( sizeof( _property_type_strings ) / sizeof( const char* ) ) &&
               _property_type_strings[ e -> misc_value1() ] != nullptr )
          {
            node -> add_parm( "modifier", e -> misc_value1() );
            node -> add_parm( "modifier_text", _property_type_strings[ e -> misc_value1() ] );
          }
          break;
        default:
          break;
      }
    }
    else
      node -> add_parm( "sub_type_text", "Unknown" );
  }
  node -> add_parm( "base_value", e -> base_value() );

  if ( level <= MAX_LEVEL )
  {
    double v_min = dbc.effect_min( e -> id(), level );
    double v_max = dbc.effect_max( e -> id(), level );
    node -> add_parm( "scaled_value", v_min  );
    if ( v_min != v_max )
    {
      node -> add_parm( "scaled_value_max", v_max );
    }
  }
  else
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( level );
    double item_budget = ilevel_data.p_epic[ 0 ];

    node -> add_parm( "scaled_value", item_budget * e -> m_average() );
  }

  if ( e -> m_average() != 0 )
  {
    node -> add_parm( "multiplier_average", e -> m_average() );
  }

  if ( e -> m_delta() != 0 )
  {
    node -> add_parm( "multiplier_delta", e -> m_delta() );
  }

  if ( level <= MAX_LEVEL )
  {
    if ( e -> m_unk() )
    {
      node -> add_parm( "bonus_value", dbc.effect_bonus( e -> id(), level ) );
      node -> add_parm( "bonus_value_multiplier", e -> m_unk() );
    }
  }

  if ( e -> real_ppl() != 0 )
  {
    node -> add_parm( "points_per_level", e -> real_ppl() );
  }

  if ( e -> die_sides() != 0 )
  {
    node -> add_parm( "value_range", e -> die_sides() );
  }

  if ( e -> sp_coeff() != 0 )
  {
    node -> add_parm( "sp_coefficient", e -> sp_coeff() );
  }

  if ( e -> ap_coeff() != 0 )
  {
    node -> add_parm( "ap_coefficient", e -> ap_coeff() );
  }

  if ( e -> chain_multiplier() != 0 && e -> chain_multiplier() != 1.0 )
    node -> add_parm( "chain_multiplier", e -> chain_multiplier() );

  if ( e -> misc_value1() != 0 || e -> type() == E_ENERGIZE )
  {
    if ( e -> subtype() == A_MOD_DAMAGE_DONE ||
         e -> subtype() == A_MOD_DAMAGE_TAKEN ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_DONE ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_TAKEN )
      node -> add_parm( "misc_value_mod_damage", e -> misc_value1() );
    else if ( e -> type() == E_ENERGIZE )
      node -> add_parm( "misc_value_energize", util::resource_type_string( util::translate_power_type( static_cast<power_e>( e -> misc_value1() ) ) ) );
    else
      node -> add_parm( "misc_value", e -> misc_value1() );
  }

  if ( e -> misc_value2() != 0 )
  {
    node -> add_parm( "misc_value_2", e -> misc_value2() );
  }

  if ( e -> pp_combo_points() != 0 )
    node -> add_parm( "points_per_combo_point", e -> pp_combo_points() );

  if ( e -> trigger_spell_id() != 0 )
    node -> add_parm( "trigger_spell_id", e -> trigger_spell_id() );
}

void spell_info::to_xml( const dbc_t& dbc, const spell_data_t* spell, xml_node_t* parent, int level )
{
  player_e pt = PLAYER_NONE;

  if ( spell -> scaling_class() != 0 && spell -> level() > static_cast< unsigned >( level ) )
  {
    return;
  }

  xml_node_t* node = parent -> add_child( "spell" );

  node -> add_parm( "id", spell -> id() );
  node -> add_parm( "name", spell -> name_cstr() );
  spell_flags_xml( spell, node );

  if ( spell -> replace_spell_id() > 0 )
  {
    node -> add_parm( "replaces_name", dbc.spell( spell -> replace_spell_id() ) -> name_cstr() );
    node -> add_parm( "replaces_id", spell -> replace_spell_id() );
  }

  if ( spell -> class_mask() )
  {
    bool pet_ability = false;

    if ( dbc.is_specialization_ability( spell -> id() ) )
    {
      std::vector<specialization_e> spec_list;
      std::vector<specialization_e>::iterator iter;
      dbc.ability_specialization( spell -> id(), spec_list );

      for ( iter = spec_list.begin(); iter != spec_list.end(); ++iter )
      {
        xml_node_t* spec_node = node -> add_child( "spec" );
        spec_node -> add_parm( "id", *iter );
        spec_node -> add_parm( "name", dbc::specialization_string( *iter ) );
        if ( *iter == PET_FEROCITY || *iter == PET_CUNNING || *iter == PET_TENACITY )
        {
          pet_ability = true;
        }
      }
      spec_list.clear();
    }

    for ( unsigned int i = 1; i < sizeof_array( _class_map ); i++ )
    {
      if ( ( spell -> class_mask() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
      {
        xml_node_t* class_node = node -> add_child( "class" );
        class_node -> add_parm( "id", _class_map[ i ].pt );
        class_node -> add_parm( "name", _class_map[ i ].name );
        if ( ! pt )
          pt = _class_map[ i ].pt;
      }
    }

    if ( pet_ability )
      node -> add_child( "class" ) -> add_parm( ".",  "Pet" );
  }

  if ( spell -> race_mask() )
  {
    for ( unsigned int i = 0; i < 24; i++ )
    {
      if ( ( spell -> race_mask() & ( 1 << i ) ) && _race_strings[ i ] )
      {
        assert( i < sizeof_array(_race_strings) );
        xml_node_t* race_node = node -> add_child( "race" );
        race_node -> add_parm( "id", i );
        race_node -> add_parm( "name", _race_strings[ i ] );
      }
    }
  }

  for ( size_t i = 0; spell -> _power && i < spell -> _power -> size(); i++ )
  {
    const spellpower_data_t* pd = spell -> _power -> at( i );

    if ( pd -> cost() == 0 )
      continue;

    xml_node_t* resource_node = node -> add_child( "resource" );
    resource_node -> add_parm( "type", ( signed ) pd -> type() );

    if ( pd -> type() == POWER_MANA )
      resource_node -> add_parm( "cost", spell -> cost( pd -> type() ) * 100.0 );
    else
      resource_node -> add_parm( "cost", spell -> cost( pd -> type() ) );

    if ( ( pd -> type() + POWER_OFFSET ) < static_cast< int >( sizeof( _resource_strings ) / sizeof( const char* ) ) &&
         _resource_strings[ pd -> type() + POWER_OFFSET ] != nullptr )
      resource_node -> add_parm( "type_name", _resource_strings[ pd -> type() + POWER_OFFSET ] );

    if ( pd -> type() == POWER_MANA )
    {
      resource_node -> add_parm( "cost_mana_flat", floor( dbc.resource_base( pt, level ) * pd -> cost() ) );
      resource_node -> add_parm( "cost_mana_flat_level", level );
    }

    if ( pd -> aura_id() > 0 && dbc.spell( pd -> aura_id() ) -> id() == pd -> aura_id() )
    {
      resource_node -> add_parm( "cost_aura_id", pd -> aura_id() );
      resource_node -> add_parm( "cost_aura_name", dbc.spell( pd -> aura_id() ) -> name_cstr() );
    }
  }

  if ( spell -> level() > 0 )
  {
    node -> add_parm( "level", spell -> level() );
    if ( spell -> max_level() > 0 )
      node -> add_parm( "max_level", spell -> max_level() );
  }

  if ( spell -> min_range() || spell -> max_range() )
  {
    if ( spell -> min_range() )
      node -> add_parm( "range_min", spell -> min_range() );
    node -> add_parm( "range", spell -> max_range() );
  }

  if ( spell -> _cast_min > 0 || spell -> _cast_max > 0 )
  {
    if ( spell -> _cast_div )
      node -> add_parm( "cast_time", spell -> cast_time( level ).total_seconds() );
    else if ( spell -> _cast_min != spell -> _cast_max )
    {
      node -> add_parm( "cast_time_min", spell -> _cast_min / 1000.0 );
      node -> add_parm( "cast_time_max", spell -> _cast_max / 1000.0 );
    }
    else
      node -> add_parm( "cast_time_else", spell -> _cast_max / 1000.0 );
  }
  else if ( spell -> _cast_min < 0 || spell -> _cast_max < 0 )
    node -> add_parm( "cast_time_range", "ranged_shot" );

  if ( spell -> gcd() != timespan_t::zero() )
    node -> add_parm( "gcd", spell -> gcd().total_seconds() );

  if ( spell -> missile_speed() )
    node -> add_parm( "velocity", spell -> missile_speed() );

  if ( spell -> duration() != timespan_t::zero() )
  {
    if ( spell -> duration() < timespan_t::zero() )
      node -> add_parm( "duration", "-1" );
    else
      node -> add_parm( "duration", spell -> duration().total_seconds() );
  }

  if ( spell -> cooldown() > timespan_t::zero() )
    node -> add_parm( "cooldown", spell -> cooldown().total_seconds() );

  if ( spell -> initial_stacks() > 0 || spell -> max_stacks() )
  {
    if ( spell -> initial_stacks() )
      node -> add_parm( "stacks_initial", spell -> initial_stacks() );

    if ( spell -> max_stacks() )
      node -> add_parm( "stacks_max", spell -> max_stacks() );
    else if ( spell -> initial_stacks() && ! spell -> max_stacks() )
      node -> add_parm( "stacks_max", spell -> initial_stacks() );
  }

  if ( spell -> proc_chance() > 0 )
    node -> add_parm( "proc_chance", spell -> proc_chance() * 100 ); // NP 101 % displayed

  if ( spell -> extra_coeff() > 0 )
    node -> add_parm( "extra_coefficient", spell -> extra_coeff() );

  std::string attribs;
  for ( unsigned i = 0; i < NUM_SPELL_FLAGS; i++ )
  {
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( spell -> _attributes[ i ] & ( 1 << flag ) )
        attribs += "1";
      else
        attribs += "0";
    }
  }
  node -> add_child( "attributes" ) -> add_parm ( ".", attribs );

  xml_node_t* effect_node = node -> add_child( "effects" );
  effect_node -> add_parm( "count", spell -> _effects -> size() );

  for ( size_t i = 0; i < spell -> _effects -> size(); i++ )
  {
    uint32_t effect_id;
    const spelleffect_data_t* e;
    if ( ! ( effect_id = spell -> _effects -> at( i ) -> id() ) )
      continue;
    else
      e = dbc.effect( effect_id );

    spell_info::effect_to_xml( dbc, spell, e, effect_node, level );
  }

  if ( spell -> desc() )
    node -> add_child( "description" ) -> add_parm( ".", spell -> desc() );

  if ( spell -> tooltip() )
    node -> add_child( "tooltip" ) -> add_parm( ".", spell -> tooltip() );

  if ( spell -> _desc_vars )
    node -> add_child( "variables" ) -> add_parm( ".", spell -> _desc_vars );
}

void spell_info::talent_to_xml( const dbc_t& /* dbc */, const talent_data_t* talent, xml_node_t* parent, int /* level */ )
{
  xml_node_t* node = parent -> add_child( "talent" );

  node -> add_parm( "name", talent -> name_cstr() );
  node -> add_parm( "id", talent -> id() );

  if ( talent -> mask_class() )
  {
    for ( unsigned int i = 1; i < sizeof_array( _class_map ); i++ )
    {
      if ( ( talent -> mask_class() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
        node -> add_child( "class" ) -> add_parm( ".",  _class_map[ i ].name );
    }
  }

  node -> add_parm( "column", talent -> col() + 1 );
  node -> add_parm( "row", talent -> row() + 1 );
  node -> add_parm( "spell", talent -> spell_id() );
  if ( talent -> replace_id() > 0 )
    node -> add_parm( "replaces", talent -> replace_id() );
}

void spell_info::set_bonus_to_xml( const dbc_t& /* dbc */, const item_set_bonus_t* set_bonus, xml_node_t* parent, int /* level */ )
{
  xml_node_t* node = parent -> add_child( "set_bonus" );

  player_e player_type = static_cast<player_e>( set_bonus -> class_id);
  node -> add_parm( "name", set_bonus -> set_name );
  node -> add_parm( "class", util::player_type_string( player_type ) );
  node -> add_parm( "tier", set_bonus -> tier );
  node -> add_parm( "bonus_level", set_bonus -> bonus );
  if ( set_bonus -> spec > 0 )
  {
    node -> add_parm( "spec", util::specialization_string( static_cast<specialization_e>( set_bonus -> spec ) ) );
  }
  node -> add_parm( "spell_id", set_bonus -> spell_id );

}

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
  { 0,                       0                         }
};

const struct { const char* name; player_e pt; } _class_map[] =
{
  { 0, PLAYER_NONE },
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
  { 0, PLAYER_NONE },
};

const char * _race_strings[] =
{
  0,
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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  "Worgen",
  "Pandaren",
  "Pandaren (Alliance)",
  "Pandaren (Horde)",
  0
};

const char * _resource_strings[] =
{
  "Health", // -2
  0,
  "Base Mana",
  "Rage",
  "Focus",
  "Energy",
  "Energy",
  "Rune",
  "Runic Power",
  "Soul Shard",
  "Eclipse",
  "Holy Power",
  0,
  0,
  "Chi",
  "Shadow Orb",
  "Burning Ember",
  "Demonic Fury",
  0
};

const char * _property_type_strings[] =
{
  "Generic Modifier",      "Spell Duration",        "Spell Generated Threat", "Spell Effect 1",      "Stack Amount",          // 0
  "Spell Range",           "Spell Radius",          "Spell Critical Chance",  "Spell Tick Time",     "Spell Pushback",        // 5
  "Spell Cast Time",       "Spell Cooldown",        "Spell Effect 2",         0,                     "Spell Resource Cost",   // 10
  "Spell Critical Damage", "Spell Penetration",     "Spell Targets",          "Spell Proc Chance",   "Unknown 2",             // 15
  "Spell Target Bonus",    "Spell Global Cooldown", "Spell Periodic Damage",  "Spell Effect 3",      "Spell Power",           // 20
  0,                       "Spell Proc Frequency",  "Spell Damage Taken",     "Spell Dispel Chance", 0,                       // 25
  0,                       0,                       "Spell Effect 4",         0,                     0,                       // 30
  0,                       0,                       0,                        0,                     0                        // 35
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
  "Dual Wield",         0,                          0,                      0,                          0,                       // 40
  0,                    0,                          0,                      "Stealth",                  "Detect",                // 45
  0,                    0,                          "Guaranteed Hit",       0,                          0,                       // 50
  0,                    "Summon Pet",               0,                      "Weapon Damage",            0,                       // 55
  0,                    0,                          "Power Burn",           "Threat",                   "Trigger Spell",         // 60
  "Apply Raid Aura",    0,                          0,                      "Interrupt Cast",           "Distract",              // 65
  "Pull",               "Pick Pocket",              0,                      0,                          0,                       // 70
  0,                    0,                          "Server Side Script",   "Attack",                   0,                       // 75
  "Add Combo Points",   0,                          0,                      0,                          0,                       // 80
  "Summon Player",      0,                          0,                      0,                          0,                       // 85
  0,                    "Threat All",               0,                      0,                          "Self Resurrect",        // 90
  0,                    "Charge",                   "Summon All Totems",    "Knock Back",               0,                       // 95
  0,                    "Feed Pet",                 "Dismiss Pet",          0,                          0,                       // 100
  0,                    0,                          0,                      0,                          "Summon Dead Pet",       // 105
  "Destroy All Totems", 0,                          0,                      "Resurrect",                0,                       // 110
  0,                    0,                          0,                      0,                          "Apply Pet Area Aura",   // 115
  0,                    "Normalized Weapon Damage", 0,                      0,                          "Pull Player",           // 120
  "Modify Threat",      "Steal Beneficial Aura",    0,                      "Apply Friendly Area Aura", "Apply Enemy Area Aura", // 125
  "Redirect Threat",    0,                          0,                      0,                          0,                       // 130
  "Call Pet",           "Direct Heal%",             "Energize Power%",      "Leap Back",                0,                       // 135
  0,                    0,                          "Trigger Spell w/ Value", "Apply Owner Area Aura",    0,                       // 140
  0,                    "Activate Rune",            0,                      0,                          0,                       // 145
  0,                    "Trigger Spell",            0,                      0,                          0,                       // 150
  "Titan Grip",         "Add Socket",               "Create Item",          0,                          0,                       // 155
  0,                    0,                          0,                      0,                          0,                       // 160
  0,                    0,                          0,                                                                           // 165
};

const char * _effect_subtype_strings[] =
{
  "None",                       0,                          "Possess",              "Periodic Damage",          "Dummy",                              // 0
  "Confuse",                    "Charm",                    "Fear",                 "Periodic Heal",            "Attack Speed",                       // 5
  "Threat",                     "Taunt",                    "Stun",                 "Damage Done",              "Damage Taken",                       // 10
  "Damage Shield",              "Stealth",                  "Stealth Detection",    "Invisibility",             "Invisibility Detection",             // 15
  "Periodic Heal%",             0,                          "Resistance",           "Periodic Trigger Spell",   "Periodic Energize Power",            // 20
  "Pacify",                     "Root",                     "Silence",              "Spell Reflection",         "Attribute",                          // 25
  "Skill",                      "Increase Speed%",          "Increase Mounted Speed%", "Decrease Movement Speed%", "Increase Health",                 // 30
  "Increase Energy",            "Shapeshift",               "Immunity Against External Movement", 0,            "School Immunity",                    // 35
  0,                            0,                          "Proc Trigger Spell",   "Proc Trigger Damage",      "Track Creatures",                    // 40
  0,                            0,                          "Modify Parry%",        0,                          "Modify Dodge%",                      // 45
  "Modify Critical Heal Bonus", "Modify Block%",            "Modify Crit%",         "Periodic Health Leech",    "Modify Hit%",                        // 50
  "Modify Spell Hit%",          0,                          "Modify Spell Crit%",   0,                          0,                                    // 55
  "Pacify Silence",             "Scale%",                   0,                      0,                          "Periodic Mana Leech",                // 60
  "Modify Spell Haste%",        "Feign Death",              "Disarm",               "Stalked",                  "Absorb Damage",                      // 65
  0,                            0,                          "Modify Power Cost%",   "Modify Power Cost",        "Reflect Spells",                     // 70
  0,                            0,                          "Mechanic Immunity",    0,                          "Modify Damage Done%",   // 75
  "Modify Attribute%",          0,                          0,                      0,                          0,                       // 80
  "Modify Power Regeneration",  0,                          "Modify Damage Taken%", 0,                          0,                       // 85
  0,                            0,                          0,                      0,                          0,                       // 90
  0,                            0,                          0,                      0,                          "Modify Attack Power",   // 95
  0,                            "Modify Armor%",            0,                      0,                          "Modify Attack Power",   // 100
  0,                            0,                          "Add Flat Modifier",    "Add Percent Modifier",     0,                       // 105
  "Mod Power Regen",                            0,                          0,                      0,                          0,       // 110
  0,                            0,                          0,                      0,                          0,                       // 115
  0,                            0,                          0,                      0,                          "Modify Ranged Attack Power", // 120
  0,                            0,                          0,                      0,                          0,                       // 125
  0,                            0,                          0,                      0,                          0,                       // 130
  "Modify Healing Power",       "Modify Healing% Done",     "Modify Total Stat%",   "Modify Melee Haste%",      0,                       // 135
  "Modify Ranged Haste%",       0,                          "Modify Base Resistance",0,                         0,                       // 140
  0,                            0,                          0,                      0,                          0,                       // 145
  0,                            0,                          0,                      0,                          0,                       // 150
  0,                            0,                          0,                      0,                          0,                       // 155
  0,                            0,                          0,                      0,                          0,                       // 160
  0,                            0,                          0,                      0,                          0,                       // 165
  0,                            0,                          0,                      0,                          0,                       // 170
  0,                            0,                          0,                      0,                          0,                       // 175
  0,                            0,                          0,                      0,                          0,                       // 180
  0,                            0,                          "Modify Attacker Melee Crit Chance",0,              "Modify Rating",         // 185
  0,                            0,                          "Modify Ranged and Melee Haste%",                   "Modify All Haste%", 0,  // 190
  0,                            0,                          0,                      0,                          0,                       // 195
  0,                            0,                          0,                      0,                          0,                       // 200
  0,                            0,                          0,                      0,                          0,                       // 205
  0,                            0,                          0,                      0,                          0,                       // 210
  0,                            0,                          0,                      0,                          0,                       // 215
  0,                            0,                          0,                      0,                          0,                       // 220
  0,            "Periodic Dummy",                           0,                      0,                          0,                       // 225
  0,                            "Trigger Spell with Value", 0,                      0,                          0,                       // 230
  0,                            0,                          0,                      0,                          0,                       // 235
  "Modify Expertise%",          0,                          0,                      0,                          0,                       // 240
  0,                            0,                          0,                      0,                          0,                       // 245
  "Increase Max Health (Stacking)",0,                       0,                      0,                          0,                       // 250
  0,                            0,                          0,                      0,                          0,                       // 255
  0,                            0,                          0,                      0,                          0,                       // 260
  0,                            0,                          0,                      0,                          0,                       // 265
  0,                            0,                          0,                      0,                          0,                       // 270
  0,                            0,                          0,                      0,                          0,                       // 275
  0,                            0,                          0,                      0,                          0,                       // 280
  0,                            0,                          0,                      0,                          0,                       // 285
  "Modify Critical Strike%",    0,                          0,                      0,                          0,                       // 290
  0,                            0,                          0,                      0,                          0,                       // 295
  0,                            0,                          0,                      0,                          0,                       // 300
  0,                            0,                          0,                      0,                          0,                       // 305
  0,                            0,                          0,                      0,                          0,                       // 310
  0,                            0,                          0,                      0,                          "Modify Melee Attack Speed%",                       // 315
  "Modify Ranged Attack Speed%",0,                          0,                      0,                          0,                       // 320
  0,                            0,                          0,                      0,                          0,                       // 325
  0,                            0,                          0,                      0,                          0,                       // 330
  0,                            0,                          0,                      0,                          0,                       // 335
  0,                            0,                          "Modify Ranged and Melee Attack Speed%", 0,         0,                       // 340
  0,                            0,                          0,                      0,                          0,                       // 345
  0,                            0,                          0,                      0,                          0,                       // 350
  0,                            0,                          0,                      0,                          0,                       // 355
  0,                            0,                          0,                      0,                          0,                       // 360
  0,                            "Override Spell Power per Attack Power%",                          0,                      0,                          0,                       // 365
  0,                            0,                          0,                      0,                          0,                       // 370
  0,                            0,                          0,                      0,                          0,                       // 375
  0,                            0,                          0,                      0,                          0,                       // 380
  0,                            0,                          0,                      0,                          0,                       // 385
  0,                            0,                          0,                      0,                          0,                       // 390
  0,                            0,                          0,                      0,                          0,                       // 395
  0,                            0,                          0,                      0,                          0,                       // 400
  0,                            0,                          0,                      0,                          0,                       // 405
  0,                            0,                          0,                      0,                          0,                       // 410
  0,                            0,                          0,                      0,                          0,                       // 415
  0,                            "Modify Absorb% Done",      "Modify Absorb% Done",  0,                          0,                       // 420
  0,                            0,                          0,                      0,                          0,                       // 425
  0,                            0,                          0,                      0,                          0,                       // 430
  0,                            0,                          0,                      0,                          0,                       // 435
  0,                            "Modify Multistrike%",      0,                      0,                          0,                       // 440
  0,                            0,                          0,                      0,                          0,                       // 445
};

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

  char tmp_buffer[512],
       tmp_buffer2[64];

  snprintf( tmp_buffer2, sizeof( tmp_buffer2 ), "(id=%u)", e -> id() );
  snprintf( tmp_buffer, sizeof( tmp_buffer ), "#%d %-*s: ", e -> index() + 1, 14, tmp_buffer2 );
  s << tmp_buffer;

  if ( e -> type() < static_cast< int >( sizeof( _effect_type_strings ) / sizeof( const char* ) ) &&
       _effect_type_strings[ e -> type() ] != 0 )
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
          _effect_subtype_strings[ e -> subtype() ] != 0 )
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
               _property_type_strings[ e -> misc_value1() ] != 0 )
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

  double v_min = dbc.effect_min( e -> id(), level );
  double v_max = dbc.effect_max( e -> id(), level );

  s << v_min;
  if ( v_min != v_max )
    s << " - " << v_max;

  if ( v_min != e -> base_value() && v_max != e -> base_value() )
  {
    s << " (avg=" << e -> m_average();
    if ( e -> m_delta() != 0 )
      s << ", dl=" << e -> m_delta();
    s << ")";
  }

  if ( e -> m_unk() )
  {
    s << " | Bonus Value: " << dbc.effect_bonus( e -> id(), level );
    s << " (" << e -> m_unk() << ")";
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

  s << std::endl;

  std::stringstream affect_str;
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

    for ( unsigned int i = 1; i < 12; i++ )
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
      if ( ( spell -> race_mask() & ( 1 << ( i - 1 ) ) ) && _race_strings[ i ] )
        s << _race_strings[ i ] << ", ";
    }

    s.seekp( -2, std::ios_base::cur );
    s << std::endl;
  }

  if ( spell -> rune_cost() == 0 && spell -> class_mask() != 0 )
  {
    for ( size_t i = 0; spell -> _power && i < spell -> _power -> size(); i++ )
    {
      const spellpower_data_t* pd = spell -> _power -> at( i );

      if ( pd -> cost() == 0 && pd -> cost_per_second() == 0 )
        continue;

      s << "Resource         : ";
      if ( spell -> cost( pd -> type() ) > 0 )
      {
        if ( pd -> type() == POWER_MANA )
          s << spell -> cost( pd -> type() ) * 100.0 << "%";
        else
          s << spell -> cost( pd -> type() );

        s << " ";

        if ( ( pd -> type() + POWER_OFFSET ) < static_cast< int >( sizeof( _resource_strings ) / sizeof( const char* ) ) &&
            _resource_strings[ pd -> type() + POWER_OFFSET ] != 0 )
          s << _resource_strings[ pd -> type() + POWER_OFFSET ];
        else
          s << "Unknown (" << pd -> type() << ")";

        if ( pd -> type() == POWER_MANA )
        {
          s << " (" << floor( dbc.resource_base( pt, level ) * pd -> cost() ) << " @Level " << level << ")";
        }
      }
      
      if ( pd -> cost_per_second() > 0 )
      {
        if ( pd -> cost() > 0 )
          s << " and ";

        if ( pd -> type() == POWER_MANA )
          s << pd -> cost_per_second() * 100.0 << "%";
        else
          s << pd -> cost_per_second();

        s << " ";

        if ( ( pd -> type() + POWER_OFFSET ) < static_cast< int >( sizeof( _resource_strings ) / sizeof( const char* ) ) &&
            _resource_strings[ pd -> type() + POWER_OFFSET ] != 0 )
          s << _resource_strings[ pd -> type() + POWER_OFFSET ];
        else
          s << "Unknown (" << pd -> type() << ")";

        if ( pd -> type() == POWER_MANA )
        {
          s << " (" << floor( dbc.resource_base( pt, level ) * pd -> cost_per_second() ) << " @Level " << level << ")";
        }

        s << " per second";
      }

      if ( pd -> aura_id() > 0 && dbc.spell( pd -> aura_id() ) -> id() == pd -> aura_id() )
        s << " w/ " << dbc.spell( pd -> aura_id() ) -> name_cstr() << " (id=" << pd -> aura_id() << ")";

      s << std::endl;
    }
  }

  if ( spell -> rune_cost() > 0 && spell -> class_mask() != 0 )
  {
    s << "Rune Cost        : ";

    int b = spell -> rune_cost() & 0x3;
    int u = ( spell -> rune_cost() & 0xC ) >> 2;
    int f = ( spell -> rune_cost() & 0x30 ) >> 4;
    int d = ( spell -> rune_cost() & 0xC0 ) >> 6;
    if ( b > 0 ) s << ( b & 0x1 ? "1" : "2" ) << " Blood, ";
    if ( u > 0 ) s << ( u & 0x1 ? "1" : "2" ) << " Unholy, ";
    if ( f > 0 ) s << ( f & 0x1 ? "1" : "2" ) << " Frost, ";
    if ( d > 0 ) s << ( d & 0x1 ? "1" : "2" ) << " Death, ";

    s.seekp( -2, std::ios_base::cur );

    s << " Rune" << ( b + u + f > 1 ? "s" : "" ) << std::endl;
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

  if ( spell -> runic_power_gain() > 0 )
    s << "Power Gain       : " << spell -> runic_power_gain() << " Runic Power" << std::endl;

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
    for ( unsigned i = 0; i < specdata::spec_count(); i++ )
    {
      const rppm_modifier_t& rppmm = dbc.real_ppm_modifier( specdata::spec_id( i ), spell -> id() );
      if ( rppmm.coefficient != 0 )
      {
        if ( ! has_modifiers )
          s << " (";

        has_modifiers = true;
        std::streamsize decimals = 3;
        double rppm_val = spell -> real_ppm() * ( 1.0 + rppmm.coefficient );
        if ( rppm_val >= 10 )
          decimals += 2;
        else if ( rppm_val >= 1 )
          decimals += 1;
        s.precision( decimals );
        s << util::specialization_string( specdata::spec_id( i ) ) << ": " << rppm_val << ", ";
      }
    }

    if ( has_modifiers )
    {
      s.seekp( -2, std::ios_base::cur );
      s << ")";
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

  s << "Attributes       : ";
  for ( unsigned i = 0; i < NUM_SPELL_FLAGS; i++ )
  {
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( spell -> attribute( i ) & ( 1 << flag ) )
        s << "x";
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
  s << std::endl;

  s << "Effects          :" << std::endl;

  for ( size_t i = 0; i < spell -> effect_count(); i++ )
  {
    const spelleffect_data_t* e;
    uint32_t effect_id;
    if ( ! ( effect_id = spell -> effectN( i + 1 ).id() ) )
      continue;
    else
      e = dbc.effect( effect_id );

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
    for ( unsigned int i = 1; i < 12; i++ )
    {
      if ( talent -> mask_class() & ( 1 << ( i - 1 ) ) )
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
  for( size_t i = 0; i < sizeof_array( set_bonus -> spec_guess ); ++i )
  {
    specialization_e spec = static_cast<specialization_e>( set_bonus -> spec_guess[ i ] );
    if ( spec == SPEC_NONE )
      continue;
    s << "Specc Guess " << i << " : " << util::specialization_string( spec ) << std::endl;
  }
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
       _effect_type_strings[ e -> type() ] != 0 )
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
          _effect_subtype_strings[ e -> subtype() ] != 0 )
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
               _property_type_strings[ e -> misc_value1() ] != 0 )
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

  double v_min = dbc.effect_min( e -> id(), level );
  double v_max = dbc.effect_max( e -> id(), level );
  node -> add_parm( "scaled_value", v_min  );
  if ( v_min != v_max )
  {
    node -> add_parm( "scaled_value_max", v_max );
  }

  if ( v_min != e -> base_value() && v_max != e -> base_value() )
  {
    node -> add_parm( "multiplier_average", e -> m_average() );
    if ( e -> m_delta() != 0 )
      node -> add_parm( "multiplier_delta", e -> m_delta() );
  }

  if ( e -> m_unk() )
  {
    node -> add_parm( "bonus_value", dbc.effect_bonus( e -> id(), level ) );
    node -> add_parm( "bonus_value_multiplier", e -> m_unk() );
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

    for ( unsigned int i = 0; i < 12; i++ )
    {
      if ( spell -> class_mask() & ( 1 << ( i - 1 ) ) )
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
      if ( spell -> race_mask() & ( 1 << ( i - 1 ) ) )
      {
        xml_node_t* race_node = node -> add_child( "race" );
        race_node -> add_parm( "id", i );
        race_node -> add_parm( "name", _race_strings[ i ] );
      }
    }
  }

  if ( spell -> rune_cost() == 0 )
  {
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
           _resource_strings[ pd -> type() + POWER_OFFSET ] != 0 )
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
  }
  else if ( spell -> rune_cost() > 0 )
  {
    xml_node_t* resource_node = node -> add_child( "resource" );
    resource_node -> add_parm( "type", ( signed ) POWER_RUNE );
    resource_node -> add_parm( "type_name", _resource_strings[ POWER_RUNE + POWER_OFFSET ] );
    int b = spell -> rune_cost() & 0x3;
    int u = ( spell -> rune_cost() & 0xC ) >> 2;
    int f = ( spell -> rune_cost() & 0x30 ) >> 4;
    int d = ( spell -> rune_cost() & 0xC0 ) >> 6;
    if ( b > 0 ) resource_node -> add_parm( "cost_rune_blood", b & 0x1 ? 1 : 2 );
    if ( u > 0 ) resource_node -> add_parm( "cost_rune_unholy", u & 0x1 ? 1 : 2 );
    if ( f > 0 ) resource_node -> add_parm( "cost_rune_frost", f & 0x1 ? 1 : 2 );
    if ( d > 0 ) resource_node -> add_parm( "cost_rune_death", d & 0x1 ? 1 : 2 );
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

  if ( spell -> runic_power_gain() > 0 )
    node -> add_parm( "runic_power_gain", spell -> runic_power_gain() );

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
    for ( unsigned int i = 1; i < 12; i++ )
    {
      if ( talent -> mask_class() & ( 1 << ( i - 1 ) ) )
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
  for( size_t i = 0; i < sizeof_array( set_bonus -> spec_guess ); ++i )
  {
    specialization_e spec = static_cast<specialization_e>( set_bonus -> spec_guess[ i ] );
    if ( spec == SPEC_NONE )
      continue;
    node -> add_parm( std::string("spec_guess_") + util::to_string( i ), util::specialization_string( spec ) );
  }
  node -> add_parm( "spec", util::specialization_string( static_cast<specialization_e>( set_bonus -> spec ) ) );
  node -> add_parm( "spell_id", set_bonus -> spell_id );

}

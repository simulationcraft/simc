// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

static const struct { const char* name; player_type pt; } _class_map[] =
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
  { 0, PLAYER_NONE },
  { "Druid", DRUID },
  { 0, PLAYER_NONE },
  { "Monk", MONK }, // FIXME: move to the correct place
};

static const char * _race_strings[] =
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
  0
};

static const char * _resource_strings[] =
{
  "Health", // -2
  0,
  "Base Mana",
  "Rage",
  "Focus",
  "Energy",
  "Happiness",
  "Rune",
  "Runic Power",
  "Soul Shard",
  "Eclipse",
  "Holy Power",
  0,
};

static const char * _property_type_strings[] =
{
  "Generic Modifier",      "Spell Duration",        "Spell Generated Threat", "Spell Effect 1",      "Stack Amount",          // 0
  "Spell Range",           "Spell Radius",          "Spell Critical Chance",  "Spell Tick Time",     "Spell Pushback",        // 5
  "Spell Cast Time",       "Spell Cooldown",        "Spell Effect 2",         0,                     "Spell Resource Cost",   // 10
  "Spell Critical Damage", "Spell Penetration",     "Spell Targets",          "Spell Proc Chance",   "Unknown 2",             // 15
  "Spell Target Bonus",    "Spell Global Cooldown", "Spell Periodic Damage",  "Spell Effect 3",      "Spell Power",           // 20
  0,                       "Spell Proc Frequency",  "Spell Damage Taken",     "Spell Dispel Chance", 0                        // 25
};

static const char * _effect_type_strings[] =
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
  "Titan Grip",         0,                          "Create Item",          0,                          0,                       // 155
  0,                    0,                          0,                      0,                          0,                       // 160
  0,                    0,                          0,                                                                           // 165
};

static const char * _effect_subtype_strings[] =
{
  "None",                       0,                          "Possess",              "Periodic Damage",          "Dummy",                              // 0
  "Confuse",                    "Charm",                    "Fear",                 "Periodic Heal",            "Attack Speed",                       // 5
  "Threat",                     "Taunt",                    "Stun",                 "Damage Done",              "Damage Taken",                       // 10
  "Damage Shield",              "Stealth",                  "Stealth Detection",    "Invisibility",             "Invisibility Detection",             // 15
  0,                            0,                          "Resistance",           "Periodic Trigger Spell",   "Periodic Energize Power",            // 20
  "Pacify",                     "Root",                     "Silence",              "Spell Reflection",         "Attribute",                          // 25
  "Skill",                      "Increase Speed%",          "Increase Mounted Speed%", "Decrease Movement Speed%", "Increase Health",                 // 30
  "Increase Energy",            "Shapeshift",               "Immunity Against External Movement", 0,            "School Immunity",                    // 35
  0,                            0,                          "Proc Trigger Spell",   "Proc Trigger Damage",      "Track Creatures",                    // 40
  0,                            0,                          "Modify Parry%",        0,                          "Modify Dodge%",                      // 45
  "Modify Critical Heal Bonus", "Modify Block%",            "Modify Crit%",         "Periodic Health Leech",    "Modify Hit%",                        // 50
  "Modify Spell Hit%",          0,                          "Modify Spell Crit%",   0,                          0,                                    // 55
  "Pacify Silence",             "Scale%",                   0,                      0,                          "Periodic Mana Leech",                // 60
  "Modify Cast Speed%",         "Feign Death",              "Disarm",               "Stalked",                  "Absorb Damage",                      // 65
  0,                            0,                          "Modify Power Cost%",   "Modify Power Cost",        "Reflect Spells",                     // 70
  0,                            0,                          "Mechanic Immunity",    0,                          "Modify Damage Done%",   // 75
  "Modify Attribute%",          0,                          0,                      0,                          0,                       // 80
  "Modify Power Regeneration",  0,                          "Modify Damage Taken%", 0,                          0,                       // 85
  0,                            0,                          0,                      0,                          0,                       // 90
  0,                            0,                          0,                      0,                          0,                       // 95
  0,                            0,                          0,                      0,                          "Modify Attack Power",   // 100
  0,                            0,                          "Add Flat Modifier",    "Add Percent Modifier",     0,                       // 105
  0,                            0,                          0,                      0,                          0,                       // 110
  0,                            0,                          0,                      0,                          0,                       // 115
  0,                            0,                          0,                      0,                          "Modify Ranged Attack Power", // 120
  0,                            0,                          0,                      0,                          0,                       // 125
  0,                            0,                          0,                      0,                          0,                       // 130
  0,                            0,                          "Modify Total Stat%",   "Modify Haste",             0,                       // 135
  0,                            0,                          0,                      0,                          0,                       // 140
  0,                            0,                          0,                      0,                          0,                       // 145
  0,                            0,                          0,                      0,                          0,                       // 150
  0,                            0,                          0,                      0,                          0,                       // 155
  0,                            0,                          0,                      0,                          0,                       // 160
  0,                            0,                          0,                      0,                          0,                       // 165
  0,                            0,                          0,                      0,                          0,                       // 170
  0,                            0,                          0,                      0,                          0,                       // 175
  0,                            0,                          0,                      0,                          0,                       // 180
  0,                            0,                          0,                      0,                          0,                       // 185
  0,                            0,                          0,                      0,                          0,                       // 190
  0,                            0,                          0,                      0,                          0,                       // 195
  0,                            0,                          0,                      0,                          0,                       // 200
  0,                            0,                          0,                      0,                          0,                       // 205
  0,                            0,                          0,                      0,                          0,                       // 210
  0,                            0,                          0,                      0,                          0,                       // 215
  0,                            0,                          0,                      0,                          0,                       // 220
  0,                            0,                          0,                      0,                          0,                       // 225
  0,                            0,                          0,                      0,                          0,                       // 230
  0,                            0,                          0,                      0,                          0,                       // 235
  "Modify Expertise%",          0,                          0,                      0,                          0,                       // 240
  0,                            0,                          0,                      0,                          0,                       // 245
  0,                            0,                          0,                      0,                          0,                       // 250
  0,                            0,                          0,                      0,                          0,                       // 255
  0,                            0,                          0,                      0,                          0,                       // 260
  0,                            0,                          0,                      0,                          0,                       // 265
  0,                            0,                          0,                      0,                          0,                       // 270
  0,                            0,                          0,                      0,                          0,                       // 275
  0,                            0,                          0,                      0,                          0,                       // 280
  0,                            0,                          0,                      0,                          0,                       // 285
  0,                            0,                          0,                      0,                          0,                       // 290
  0,                            0,                          0,                      0,                          0,                       // 295
  0,                            0,                          0,                      0,                          0,                       // 300
  0,                            0,                          0,                      0,                          0,                       // 305
  0,                            0,                          0,                      0,                          0,                       // 310
  0,                            0,                          0,                      0,                          0,                       // 315
  0,                            0,                          0,                      0,                          0,                       // 320
  0,                            0,                          0,                      0,                          0,                       // 325
  0,                            0,                          0,                      0,                          0,                       // 330
  0,                            0,                          0,                      0,                          0,                       // 335
  0,                            0,                          0,                      0,                          0,                       // 340
  0,                            0,                          0,                      0,                          0,                       // 345
  0,                            0,                          0,                      0,                          0,                       // 350
  0,                            0,                          0,                      0,                          0,                       // 355
};

static std::string spell_flags( sim_t* /* sim */, const spell_data_t* spell )
{
  std::ostringstream s;

  s << "[";

  if ( spell -> scaling_class() != 0 )
    s << "Scaling Spell, ";

  if ( spell -> _gcd == 0 && spell -> _cast_min == 0 && spell -> _cast_max == 0 )
    s << "Off GCD, ";

  if ( s.tellp() > 1 )
  {
    s.seekp( -2, std::ios_base::cur );
    s << "]";
  }
  else
    return std::string();

  return s.str();
}

std::ostringstream& spell_info_t::effect_to_str( sim_t*                    sim,
                                                 const spell_data_t*       spell,
                                                 const spelleffect_data_t* e,
                                                 std::ostringstream&       s )
{
  char tmp_buffer[512],
       tmp_buffer2[64];

  snprintf( tmp_buffer2, sizeof( tmp_buffer2 ), "(id=%u)", e -> id() );
  snprintf( tmp_buffer, sizeof( tmp_buffer ), "#%d %-*s: ", e -> index() + 1, 10, tmp_buffer2 );
  s << tmp_buffer;

  if ( e -> type() < static_cast< int >( sizeof( _effect_type_strings ) / sizeof( const char* ) ) &&
       _effect_type_strings[ e -> type() ] != 0 )
  {
    s << _effect_type_strings[ e -> type() ];
    // Put some nice handling on some effect types
    switch ( e -> type() )
    {
    case E_SCHOOL_DAMAGE:
      s << ": " << util_t::school_type_string( spell_id_t::get_school_type( spell -> school_mask() ) );
      break;
    case E_TRIGGER_SPELL:
    case E_TRIGGER_SPELL_WITH_VALUE:
      if ( e -> trigger_spell_id() ) {
        if( sim -> dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
          s << ": " << sim -> dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
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
        s << ": " << util_t::school_type_string( spell_id_t::get_school_type( spell -> school_mask() ) );
        if ( e -> period() != timespan_t::zero )
          s << " every " << e -> period().total_seconds() << " seconds";
        break;
      case A_PROC_TRIGGER_SPELL:
        if ( e -> trigger_spell_id() && sim -> dbc.spell( e -> trigger_spell_id() ) )
          s << ": " << sim -> dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
        break;
      case A_PERIODIC_TRIGGER_SPELL:
        if ( e -> trigger_spell_id() && sim -> dbc.spell( e -> trigger_spell_id() ) )
        {
          s << ": " << sim -> dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
          if ( e -> period() != timespan_t::zero )
            s << " every " << e -> period().total_seconds() << " seconds";
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

  s << "               Base Value: ";
  double v_min = 0, v_max = 0;

  v_min = sim -> dbc.effect_min( e -> id(), 85 );
  v_max = sim -> dbc.effect_max( e -> id(), 85 );

  s << v_min;
  if ( v_min != v_max )
    s << " - " << v_max;

  if ( v_min != e -> base_value() && v_max != e -> base_value() )
    s << " (" << e -> base_value() << ")";

  if ( e -> m_unk() )
    s << " | Bonus Value: " << sim -> dbc.effect_bonus( e -> id(), 85 );

  if ( e -> real_ppl() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%f", e -> real_ppl() );
    s << " | Points Per Level: " << e -> real_ppl();
  }

  if ( e -> die_sides() != 0 )
  {
    s << " | Value Range: " << e -> die_sides();
  }

  if ( e -> coeff() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%.3f", e -> coeff() );
    s << " | Coefficient: " << tmp_buffer;
  }

  if ( e -> chain_multiplier() != 0 && e -> chain_multiplier() != 1.0 )
    s << " | Chain Multiplier: " << e -> chain_multiplier();

  if ( e -> misc_value1() != 0 )
  {
    if ( e -> subtype() == A_MOD_DAMAGE_DONE ||
         e -> subtype() == A_MOD_DAMAGE_TAKEN ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_DONE ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_TAKEN )
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%#.x", e -> misc_value1() );
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

  s << std::endl;
  return s;
}

std::string spell_info_t::to_str( sim_t* sim, const spell_data_t* spell )
{
  std::ostringstream s;
  player_type pt = PLAYER_NONE;

  s <<   "Name         : " << spell -> name_cstr() << " (id=" << spell -> id() << ") " << spell_flags( sim, spell ) << std::endl;

  if ( spell -> class_mask() )
  {
    s << "Class        : ";
    for ( unsigned int i = 0; i < 12; i++ )
    {
      if ( spell -> class_mask() & ( 1 << ( i - 1 ) ) )
      {
        s << _class_map[ i ].name << ", ";
        if ( ! pt )
          pt = _class_map[ i ].pt;
      }
    }

    s.seekp( -2, std::ios_base::cur );
    s << std::endl;
  }

  if ( spell -> race_mask() )
  {
    s << "Race         : ";
    for ( unsigned int i = 0; i < 24; i++ )
    {
      if ( spell -> race_mask() & ( 1 << ( i - 1 ) ) )
        s << _race_strings[ i ] << ", ";
    }

    s.seekp( -2, std::ios_base::cur );
    s << std::endl;
  }

  if ( ( spell -> _power_type + 2 ) < static_cast< int >( sizeof( _resource_strings ) / sizeof( const char* ) ) &&
       ( spell -> cost() > 0 || ( spell -> _power_type == 5 && spell -> rune_cost() > 0 ) ) )
  {
    s << "Resource     : ";
    if ( spell -> _power_type != 5 && spell -> _power_type != 6 && spell -> _power_type != 1 )
    {
      double base_mana = rating_t::get_attribute_base( sim,
                                                       sim -> dbc,
                                                       85,
                                                       pt,
                                                       RACE_NONE,
                                                       BASE_STAT_MANA );

      s << spell -> _cost << ( spell -> _power_type == 0 ? "% " : " " ) << _resource_strings[ spell -> _power_type + 2 ];
      if ( spell -> _power_type == 0 )
        s << " (" << floor( spell -> cost() * base_mana ) << " mana @" << 85 << ")";

      s << std::endl;
    }
    else if ( spell -> _power_type == 6 || spell -> _power_type == 1 )
      s << spell -> cost() << " " << _resource_strings[ spell -> _power_type + 2 ] << std::endl;
    else if ( spell -> rune_cost() > 0 )
    {
      int b = spell -> rune_cost() & 0x3;
      int u = ( spell -> rune_cost() & 0xC ) >> 2;
      int f = ( spell -> rune_cost() & 0x30 ) >> 4;
      if ( b > 0 ) s << ( b & 0x1 ? "1" : "2" ) << " Blood, ";
      if ( u > 0 ) s << ( u & 0x1 ? "1" : "2" ) << " Unholy, ";
      if ( f > 0 ) s << ( f & 0x1 ? "1" : "2" ) << " Frost, ";

      s.seekp( -2, std::ios_base::cur );

      s << " Rune" << ( b + u + f > 1 ? "s" : "" ) << std::endl;
    }
  }

  if ( spell -> level() > 0 )
  {
    s << "Spell Level  : " << ( int ) spell -> level();
    if ( spell -> max_level() > 0 )
      s << " (max " << ( int ) spell -> max_level() << ")";

    s << std::endl;
  }

  if ( spell -> min_range() || spell -> max_range() )
  {
    s << "Range        : ";
    if ( spell -> min_range() )
      s << ( int ) spell -> min_range() << " - ";

    s << ( int ) spell -> max_range() << " yards" << std::endl;
  }

  if ( spell -> _cast_min > 0 || spell -> _cast_max > 0 )
  {
    s << "Cast Time    : ";

    if ( spell -> _cast_div )
      s << spell -> cast_time( 85 ).total_seconds();
    else if ( spell -> _cast_min != spell -> _cast_max )
      s << spell -> _cast_min / 1000.0 << " - " << spell -> _cast_max / 1000.0;
    else
      s << spell -> _cast_max / 1000.0;

    s << " seconds" << std::endl;
  }
  else if ( spell -> _cast_min < 0 || spell -> _cast_max < 0 )
    s << "Cast Time    : Ranged Shot" << std::endl;

  if ( spell -> gcd() != timespan_t::zero )
    s << "GCD          : " << spell -> gcd().total_seconds() << " seconds" << std::endl;

  if ( spell -> missile_speed() )
    s << "Velocity     : " << spell -> missile_speed() << " yards/sec"  << std::endl;

  if ( spell -> runic_power_gain() > 0 )
    s << "Power Gain   : " << spell -> runic_power_gain() << " Runic Power" << std::endl;

  if ( spell -> duration() != timespan_t::zero )
  {
    s << "Duration     : ";
    if ( spell -> duration() < timespan_t::zero )
      s << "Aura (infinite)";
    else
      s << spell -> duration().total_seconds() << " seconds";

    s << std::endl;
  }

  if ( spell -> cooldown() > timespan_t::zero )
    s << "Cooldown     : " << spell -> cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell -> initial_stacks() > 0 || spell -> max_stacks() )
  {
    s << "Stacks       : ";
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
    s << "Proc Chance  : " << spell -> proc_chance() * 100 << "%" << std::endl;

  if ( spell -> extra_coeff() > 0 )
    s << "Coefficient  : " << spell -> extra_coeff() << std::endl;

  s << "Effects      :" << std::endl;

  uint32_t effect_id;
  const spelleffect_data_t* e;
  for ( int i = 0; i < 3; i++ )
  {
    if ( ! ( effect_id = spell -> _effect[ i ] ) )
      continue;
    else
      e = sim -> dbc.effect( effect_id );

    spell_info_t::effect_to_str( sim, spell, e, s );
  }

  if ( spell -> desc() )
    s << "Description  : " << spell -> desc() << std::endl;

  if ( spell -> tooltip() )
    s << "Tooltip      : " << spell -> tooltip() << std::endl;

  if ( spell -> _desc_vars )
    s << "Variables    : " << spell -> _desc_vars << std::endl;

  s << std::endl;

  return s.str();
}

std::string spell_info_t::talent_to_str( sim_t* sim, const talent_data_t* talent )
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

  s << "Talent Tab   : " << talent -> tab_page() + 1 << std::endl;
  if ( talent -> depends_id() )
  {
    s << "Depends on   : " << sim -> dbc.talent( talent -> depends_id() ) -> name_cstr();
    if ( talent -> depends_rank() > 0 )
      s << " (Rank " << talent -> depends_rank() + 1 << ")";

    s << std::endl;
  }

  s << "Column       : " << talent -> col() + 1 << std::endl;
  s << "Row          : " << talent -> row() + 1 << std::endl;
  for ( int i = 0; i < 3; i++ )
  {
    if ( talent -> _rank_id[ i ] )
      s << "Rank " << i + 1 << " Spell : " << talent -> _rank_id[ i ] << std::endl;
  }

  s << std::endl;

  return s.str();
}

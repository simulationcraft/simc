// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "color.hpp"

#include "fmt/format.h"

namespace color
{

std::string rgb::rgb_str() const
{
  return fmt::format("rgba({}, {}, {}, {})", r_, g_, b_, a_ );
}

std::string rgb::str() const
{
  return fmt::to_string( *this );
}

std::string rgb::hex_str() const
{
  return fmt::format( "{:02X}{:02X}{:02X}", r_, g_, b_ );
}

rgb::operator std::string() const
{
  return str();
}

rgb class_color( player_e type )
{
  switch ( type )
  {
    case PLAYER_NONE:
      return color::GREY;
    case PLAYER_GUARDIAN:
      return color::GREY;
    case DEATH_KNIGHT:
      return color::COLOR_DEATH_KNIGHT;
    case DEMON_HUNTER:
      return color::COLOR_DEMON_HUNTER;
    case DRUID:
      return color::COLOR_DRUID;
    case EVOKER:
      return color::COLOR_EVOKER;
    case HUNTER:
      return color::COLOR_HUNTER;
    case MAGE:
      return color::COLOR_MAGE;
    case MONK:
      return color::COLOR_MONK;
    case PALADIN:
      return color::COLOR_PALADIN;
    case PRIEST:
      return color::COLOR_PRIEST;
    case ROGUE:
      return color::COLOR_ROGUE;
    case SHAMAN:
      return color::COLOR_SHAMAN;
    case WARLOCK:
      return color::COLOR_WARLOCK;
    case WARRIOR:
      return color::COLOR_WARRIOR;
    case ENEMY:
      return color::GREY;
    case ENEMY_ADD:
      return color::GREY;
    case ENEMY_ADD_BOSS:
      return color::GREY;
    case HEALING_ENEMY:
      return color::GREY;
    case TANK_DUMMY:
      return color::GREY;
    default:
      return color::GREY2;
  }
}

rgb resource_color( resource_e type )
{
  switch ( type )
  {
    case RESOURCE_HEALTH:
      return class_color( HUNTER );

    case RESOURCE_MANA:
      return class_color( SHAMAN );

    case RESOURCE_ENERGY:
    case RESOURCE_FOCUS:
    case RESOURCE_COMBO_POINT:
      return class_color( ROGUE );

    case RESOURCE_RAGE:
    case RESOURCE_RUNIC_POWER:
      return class_color( DEATH_KNIGHT );

    case RESOURCE_HOLY_POWER:
      return class_color( PALADIN );

    case RESOURCE_SOUL_SHARD:
      return class_color( WARLOCK );

    case RESOURCE_ASTRAL_POWER:
      return class_color( DRUID );

    case RESOURCE_CHI:
      return class_color( MONK );

    case RESOURCE_MAELSTROM:
      return { "FF9900" };

    case RESOURCE_RUNE:
      return class_color( MAGE );

    case RESOURCE_ESSENCE:
      return class_color( EVOKER );

    case RESOURCE_NONE:
    default:
      return GREY2;
  }
}

rgb stat_color( stat_e type )
{
  switch ( type )
  {
    case STAT_STRENGTH:
      return COLOR_WARRIOR;
    case STAT_AGILITY:
      return COLOR_HUNTER;
    case STAT_INTELLECT:
      return COLOR_MAGE;
    case STAT_SPIRIT:
      return GREY3;
    case STAT_ATTACK_POWER:
      return COLOR_ROGUE;
    case STAT_SPELL_POWER:
      return COLOR_WARLOCK;
    case STAT_CRIT_RATING:
      return COLOR_PALADIN;
    case STAT_HASTE_RATING:
      return COLOR_SHAMAN;
    case STAT_MASTERY_RATING:
      return COLOR_ROGUE.dark();
    case STAT_DODGE_RATING:
      return COLOR_MONK;
    case STAT_PARRY_RATING:
      return TEAL;
    case STAT_ARMOR:
      return COLOR_PRIEST;
    case STAT_BONUS_ARMOR:
      return COLOR_PRIEST;
    case STAT_VERSATILITY_RATING:
      return PURPLE.dark();
    default:
      return GREY2;
  }
}

/* Blizzard shool colors:
 * http://wowprogramming.com/utils/xmlbrowser/live/AddOns/Blizzard_CombatLog/Blizzard_CombatLog.lua
 * search for: SchoolStringTable
 */
// These colors are picked to sort of line up with classes, but match the "feel"
// of the spell class' color
rgb school_color( school_e school )
{
  switch ( school )
  {
    // -- Single Schools
    case SCHOOL_NONE:
      return color::COLOR_NONE;
    case SCHOOL_PHYSICAL:
      return color::PHYSICAL;
    case SCHOOL_HOLY:
      return color::HOLY;
    case SCHOOL_FIRE:
      return color::FIRE;
    case SCHOOL_NATURE:
      return color::NATURE;
    case SCHOOL_FROST:
      return color::FROST;
    case SCHOOL_SHADOW:
      return color::SHADOW;
    case SCHOOL_ARCANE:
      return color::ARCANE;
    // -- Physical and a Magical
    case SCHOOL_FLAMESTRIKE:
      return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_FIRE );
    case SCHOOL_FROSTSTRIKE:
      return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_FROST );
    case SCHOOL_SPELLSTRIKE:
      return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_ARCANE );
    case SCHOOL_STORMSTRIKE:
      return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWSTRIKE:
      return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_SHADOW );
    case SCHOOL_HOLYSTRIKE:
      return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_HOLY );
    // -- Two Magical Schools
    case SCHOOL_FROSTFIRE:
      return color::FROSTFIRE;
    case SCHOOL_SPELLFIRE:
      return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_FIRE );
    case SCHOOL_FIRESTORM:
      return school_color( SCHOOL_FIRE ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWFLAME:
      return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_FIRE );
    case SCHOOL_HOLYFIRE:
      return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_FIRE );
    case SCHOOL_SPELLFROST:
      return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_FROST );
    case SCHOOL_FROSTSTORM:
      return school_color( SCHOOL_FROST ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWFROST:
      return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_FROST );
    case SCHOOL_HOLYFROST:
      return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_FROST );
    case SCHOOL_ASTRAL:
      return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SPELLSHADOW:
      return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_SHADOW );
    case SCHOOL_DIVINE:
      return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_HOLY );
    case SCHOOL_SHADOWSTORM:
      return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_NATURE );
    case SCHOOL_HOLYSTORM:
      return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWLIGHT:
      return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_HOLY );
    //-- Three or more schools
    case SCHOOL_ELEMENTAL:
      return color::ELEMENTAL;
    case SCHOOL_COSMIC:
      return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_NATURE ) +
             school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_ARCANE );
    case SCHOOL_CHROMATIC:
      return school_color( SCHOOL_FIRE ) + school_color( SCHOOL_FROST ) +
             school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_NATURE ) +
             school_color( SCHOOL_SHADOW );
    case SCHOOL_CHROMASTRIKE:
      return school_color( SCHOOL_FIRE ) + school_color( SCHOOL_FROST ) +
             school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_NATURE ) +
             school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_PHYSICAL );
    case SCHOOL_MAGIC:
      return school_color( SCHOOL_FIRE ) + school_color( SCHOOL_FROST ) +
             school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_NATURE ) +
             school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_HOLY );
    case SCHOOL_CHAOS:
      return color::CHAOS;

    default:
      return GREY2;
  }
}
} /* namespace color */

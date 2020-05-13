// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include <string>
#include <iosfwd>

namespace color
{
  struct rgb
  {
    unsigned char r_, g_, b_;
    double a_;

    rgb();

    rgb(unsigned char r, unsigned char g, unsigned char b);
    rgb(double r, double g, double b);
    rgb(const std::string& color);
    rgb(const char* color);

    std::string rgb_str() const;
    std::string str() const;
    std::string hex_str() const;

    rgb& adjust(double v);
    rgb adjust(double v) const;
    rgb dark(double pct = 0.25) const;
    rgb light(double pct = 0.25) const;
    rgb opacity(double pct = 1.0) const;

    rgb& operator=(const std::string& color_str);
    rgb& operator+=(const rgb& other);
    rgb operator+(const rgb& other) const;
    operator std::string() const;

  private:
    bool parse_color(const std::string& color_str);
  };

  std::ostream& operator<<(std::ostream& s, const rgb& r);

  rgb mix(const rgb& c0, const rgb& c1);

  rgb class_color(player_e type);
  rgb resource_color(resource_e type);
  rgb stat_color(stat_e type);
  rgb school_color(school_e school);

  // Class colors
  const rgb COLOR_DEATH_KNIGHT = "C41F3B";
  const rgb COLOR_DEMON_HUNTER = "A330C9";
  const rgb COLOR_DRUID = "FF7D0A";
  const rgb COLOR_HUNTER = "ABD473";
  const rgb COLOR_MAGE = "69CCF0";
  const rgb COLOR_MONK = "00FF96";
  const rgb COLOR_PALADIN = "F58CBA";
  const rgb COLOR_PRIEST = "FFFFFF";
  const rgb COLOR_ROGUE = "FFF569";
  const rgb COLOR_SHAMAN = "0070DE";
  const rgb COLOR_WARLOCK = "9482C9";
  const rgb COLOR_WARRIOR = "C79C6E";

  const rgb WHITE = "FFFFFF";
  const rgb GREY = "333333";
  const rgb GREY2 = "666666";
  const rgb GREY3 = "8A8A8A";
  const rgb YELLOW = COLOR_ROGUE;
  const rgb PURPLE = "9482C9";
  const rgb RED = COLOR_DEATH_KNIGHT;
  const rgb TEAL = "009090";
  const rgb BLACK = "000000";

  // School colors
  const rgb COLOR_NONE = WHITE;
  const rgb PHYSICAL = COLOR_WARRIOR;
  const rgb HOLY = "FFCC00";
  const rgb FIRE = COLOR_DEATH_KNIGHT;
  const rgb NATURE = COLOR_HUNTER;
  const rgb FROST = COLOR_SHAMAN;
  const rgb SHADOW = PURPLE;
  const rgb ARCANE = COLOR_MAGE;
  const rgb ELEMENTAL = COLOR_MONK;
  const rgb FROSTFIRE = "9900CC";
  const rgb CHAOS = "00C800";

  // Item quality colors
  const rgb POOR = "9D9D9D";
  const rgb COMMON = WHITE;
  const rgb UNCOMMON = "1EFF00";
  const rgb RARE = "0070DD";
  const rgb EPIC = "A335EE";
  const rgb LEGENDARY = "FF8000";
  const rgb HEIRLOOM = "E6CC80";

}  // color
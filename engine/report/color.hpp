// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "util/string_view.hpp"

#include "fmt/core.h"

#include <string>

namespace color
{
  struct rgb
  {
    uint8_t r_, g_, b_;
    float a_;

    constexpr rgb() noexcept
      : r_( 0 ), g_( 0 ), b_( 0 ), a_( 1 )
    { }

    constexpr rgb( uint8_t r, uint8_t g, uint8_t b ) noexcept
      : r_( r ), g_( g ), b_( b ), a_( 1 )
    { }

    constexpr rgb( double r, double g, double b ) noexcept
      : r_( static_cast<uint8_t>( r * 255 ) ),
        g_( static_cast<uint8_t>( g * 255 ) ),
        b_( static_cast<uint8_t>( b * 255 ) ),
        a_( 1 )
    { }

    constexpr rgb( util::string_view color ) noexcept
      : r_( 0 ), g_( 0 ), b_( 0 ), a_( 1 )
    {
      parse_color( color );
    }

    std::string rgb_str() const;
    std::string str() const;
    std::string hex_str() const;
    operator std::string() const; // meh...

    rgb& adjust(double v);
    rgb adjust(double v) const;
    rgb dark(double pct = 0.25) const;
    rgb light(double pct = 0.25) const;
    rgb opacity(double pct = 1.0) const;

    rgb& operator=( util::string_view color_str );
    rgb& operator+=( rgb other );
    rgb operator+( rgb other ) const;

  private:
    constexpr void parse_color( util::string_view color_str ) noexcept
    {
      if ( util::starts_with( color_str, '#' ) )
        color_str.remove_prefix( 1 );

      if ( color_str.size() != 6 )
        return;

      struct {
        bool fail = false;
        constexpr uint8_t operator()( char ch ) noexcept {
          if ( ch >= '0' && ch <= '9' ) return static_cast<uint8_t>( ch - '0' );
          if ( ch >= 'A' && ch <= 'F' ) return static_cast<uint8_t>( ch - 'A' + 10 );
          if ( ch >= 'a' && ch <= 'f' ) return static_cast<uint8_t>( ch - 'a' + 10 );
          fail = true;
          return 0;
        }
      } parse;
      uint8_t r = parse( color_str[ 0 ] ) * 16 + parse( color_str[ 1 ] );
      uint8_t g = parse( color_str[ 2 ] ) * 16 + parse( color_str[ 3 ] );
      uint8_t b = parse( color_str[ 4 ] ) * 16 + parse( color_str[ 5 ] );

      if ( parse.fail )
        return;

      r_ = r;
      g_ = g;
      b_ = b;
    }
  };

  rgb class_color(player_e type);
  rgb resource_color(resource_e type);
  rgb stat_color(stat_e type);
  rgb school_color(school_e school);

  // Class colors
  constexpr rgb COLOR_DEATH_KNIGHT( "C41F3B" );
  constexpr rgb COLOR_DEMON_HUNTER( "A330C9" );
  constexpr rgb COLOR_DRUID( "FF7D0A" );
  constexpr rgb COLOR_HUNTER( "ABD473" );
  constexpr rgb COLOR_MAGE( "69CCF0" );
  constexpr rgb COLOR_MONK( "00FF96" );
  constexpr rgb COLOR_PALADIN( "F58CBA" );
  constexpr rgb COLOR_PRIEST( "FFFFFF" );
  constexpr rgb COLOR_ROGUE( "FFF569" );
  constexpr rgb COLOR_SHAMAN( "0070DE" );
  constexpr rgb COLOR_WARLOCK( "9482C9" );
  constexpr rgb COLOR_WARRIOR( "C79C6E" );

  constexpr rgb WHITE( "FFFFFF" );
  constexpr rgb GREY( "333333" );
  constexpr rgb GREY2( "666666" );
  constexpr rgb GREY3( "8A8A8A" );
  constexpr rgb YELLOW = COLOR_ROGUE;
  constexpr rgb PURPLE( "9482C9" );
  constexpr rgb RED = COLOR_DEATH_KNIGHT;
  constexpr rgb TEAL( "009090" );
  constexpr rgb BLACK( "000000" );

  // School colors
  constexpr rgb COLOR_NONE = WHITE;
  constexpr rgb PHYSICAL = COLOR_WARRIOR;
  constexpr rgb HOLY( "FFCC00" );
  constexpr rgb FIRE = COLOR_DEATH_KNIGHT;
  constexpr rgb NATURE = COLOR_HUNTER;
  constexpr rgb FROST = COLOR_SHAMAN;
  constexpr rgb SHADOW = PURPLE;
  constexpr rgb ARCANE = COLOR_MAGE;
  constexpr rgb ELEMENTAL = COLOR_MONK;
  constexpr rgb FROSTFIRE( "9900CC" );
  constexpr rgb CHAOS( "00C800" );

  // Item quality colors
  constexpr rgb POOR( "9D9D9D" );
  constexpr rgb COMMON = WHITE;
  constexpr rgb UNCOMMON( "1EFF00" );
  constexpr rgb RARE( "0070DD" );
  constexpr rgb EPIC( "A335EE" );
  constexpr rgb LEGENDARY( "FF8000" );
  constexpr rgb HEIRLOOM( "E6CC80" );

}  // color

namespace fmt {

template <>
struct formatter<color::rgb> {
  template <typename ParseContext>
  constexpr auto parse( ParseContext& ctx ) { return ctx.begin(); }
  template <typename FormatContext>
  auto format( color::rgb c, FormatContext& ctx ) {
    return format_to( ctx.out(), "#{:02X}{:02X}{:02X}", c.r_, c.g_, c.b_ );
  }
};

} // namespace fmt

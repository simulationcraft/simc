// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef WOW_VERSION_HPP
#define WOW_VERSION_HPP

#include <cstdint>

struct wowv_t
{
  uint8_t expansion;
  uint8_t major;
  uint8_t minor;
  uint32_t build;

  wowv_t( uint8_t expansion = 0, uint8_t major = 0, uint8_t minor = 0, uint32_t build = 0 );

  bool operator==( const wowv_t& other ) const;
  bool operator< ( const wowv_t& other ) const;
  bool operator<=( const wowv_t& other ) const;
  bool operator> ( const wowv_t& other ) const;
  bool operator>=( const wowv_t& other ) const;
};

#endif /* WOW_VERSION_HPP */

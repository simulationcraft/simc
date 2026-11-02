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
  consteval wowv_t( const char* version_name );

  bool operator==( const wowv_t& other ) const;
  bool operator< ( const wowv_t& other ) const;
  bool operator<=( const wowv_t& other ) const;
  bool operator> ( const wowv_t& other ) const;
  bool operator>=( const wowv_t& other ) const;
};

consteval uint32_t wowv_stoi( const char* str, int current_value = 0 )
{
  if ( !str[ 0 ] || str[ 0 ] == '.' )
    return current_value;
  if ( str[ 0 ] < '0' || str[ 0 ] > '9' )
    throw "string is not an integer";
  return wowv_stoi( &str[ 1 ], current_value * 10 + ( str[ 0 ] - '0' ) );
}

consteval wowv_t::wowv_t( const char* version_name ) :
  expansion(), major(), minor(), build()
{
  size_t start = 0;
  size_t end = 0;
  size_t count = 0;
  while ( version_name[ end ] )
  {
    end++;
    char c = version_name[ end ];
    if ( c == '.' || c == 0 )
    {
      if ( count == 0 )
        expansion = wowv_stoi( &version_name[ start ] );
      else if ( count == 1 )
        major = wowv_stoi( &version_name[ start ] );
      else if ( count == 2 )
        minor = wowv_stoi( &version_name[ start ] );
      else if ( count == 3 )
        build = wowv_stoi( &version_name[ start ] );
      start = end + 1;
      count++;
    }
  }
}

#endif /* WOW_VERSION_HPP */

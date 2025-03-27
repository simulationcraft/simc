// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "wow_version.hpp"

wowv_t::wowv_t( uint8_t expansion, uint8_t major, uint8_t minor, uint32_t build ) :
  expansion( expansion ), major( major ), minor( minor ), build( build )
{}

bool wowv_t::operator==( const wowv_t& other ) const
{
  return expansion == other.expansion && major == other.major
         && minor == other.minor && build == other.build;
}

bool wowv_t::operator<( const wowv_t& other ) const
{
  if ( expansion != other.expansion )
    return expansion < other.expansion;

  if ( major != other.major )
    return major < other.major;

  if ( minor != other.minor )
    return minor < other.minor;

  return build < other.build;
}

bool wowv_t::operator<=( const wowv_t& other ) const
{
  return *this == other || *this < other;
}

bool wowv_t::operator>( const wowv_t& other ) const
{
  return !( *this <= other );
}

bool wowv_t::operator>=( const wowv_t& other ) const
{
  return !( *this < other );
}

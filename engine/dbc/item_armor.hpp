// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_ARMOR_HPP
#define ITEM_ARMOR_HPP

#include "util/generic.hpp"
#include "util/span.hpp"

#include "client_data.hpp"

struct item_armor_quality_data_t
{
  unsigned ilevel;
  float    multiplier[7];

  static const item_armor_quality_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_armor_quality_data_t>( ilevel, ptr, &item_armor_quality_data_t::ilevel ); }

  static const item_armor_quality_data_t& nil()
  { return dbc::nil<item_armor_quality_data_t>; }

  static util::span<const item_armor_quality_data_t> data( bool ptr );

  float value( int quality ) const
  {
    return as<unsigned>( quality ) < std::size( multiplier )
      ? multiplier[ as<unsigned>( quality ) ]
      : 0;
  }
};

struct item_armor_shield_data_t
{
  unsigned ilevel;
  float    values[7];

  static const item_armor_shield_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_armor_shield_data_t>( ilevel, ptr, &item_armor_shield_data_t::ilevel ); }

  static const item_armor_shield_data_t& nil()
  { return dbc::nil<item_armor_shield_data_t>; }

  static util::span<const item_armor_shield_data_t> data( bool ptr );

  float value( int quality ) const
  {
    return as<unsigned>( quality ) < std::size( values )
      ? values[ as<unsigned>( quality ) ]
      : 0;
  }
};

struct item_armor_total_data_t
{
  unsigned ilevel;
  float    values[7];

  static const item_armor_total_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_armor_total_data_t>( ilevel, ptr, &item_armor_total_data_t::ilevel ); }

  static const item_armor_total_data_t& nil()
  { return dbc::nil<item_armor_total_data_t>; }

  static util::span<const item_armor_total_data_t> data( bool ptr );

  float value( int subclass ) const
  {
    return as<unsigned>( subclass ) < std::size( values )
      ? values[ as<unsigned>( subclass ) ]
      : 0;
  }
};

struct item_armor_location_data_t
{
  unsigned inv_type;
  float    multiplier[4];

  static const item_armor_location_data_t& find( unsigned inv_type, bool ptr )
  { return dbc::find<item_armor_location_data_t>( inv_type, ptr, &item_armor_location_data_t::inv_type ); }

  static const item_armor_location_data_t& nil()
  { return dbc::nil<item_armor_location_data_t>; }

  static util::span<const item_armor_location_data_t> data( bool ptr );

  float value( int subclass ) const
  {
    return as<unsigned>( subclass ) < std::size( multiplier )
      ? multiplier[ as<unsigned>( subclass ) ]
      : 0;
  }
};

#endif /* ITEM_ARMOR_HPP */


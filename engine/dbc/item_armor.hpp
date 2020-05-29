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
private:
  using key_t = dbc::ilevel_member_policy_t;

public:
  unsigned ilevel;
  double   multiplier[7];

  static const item_armor_quality_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_armor_quality_data_t, key_t>( ilevel, ptr ); }

  static const item_armor_quality_data_t& nil()
  { return dbc::nil<item_armor_quality_data_t>(); }

  static util::span<const item_armor_quality_data_t> data( bool ptr );

  double value( int quality ) const
  {
    return as<unsigned>( quality ) < range::size( multiplier )
      ? multiplier[ as<unsigned>( quality ) ]
      : 0.0;
  }
};

struct item_armor_shield_data_t
{
private:
  using key_t = dbc::ilevel_member_policy_t;
public:
  unsigned ilevel;
  double   values[7];

  static const item_armor_shield_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_armor_shield_data_t, key_t>( ilevel, ptr ); }

  static const item_armor_shield_data_t& nil()
  { return dbc::nil<item_armor_shield_data_t>(); }

  static util::span<const item_armor_shield_data_t> data( bool ptr );

  double value( int quality ) const
  {
    return as<unsigned>( quality ) < range::size( values )
      ? values[ as<unsigned>( quality ) ]
      : 0.0;
  }
};

struct item_armor_total_data_t
{
private:
  using key_t = dbc::ilevel_member_policy_t;

public:
  unsigned ilevel;
  double   values[7];

  static const item_armor_total_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_armor_total_data_t, key_t>( ilevel, ptr ); }

  static const item_armor_total_data_t& nil()
  { return dbc::nil<item_armor_total_data_t>(); }

  static util::span<const item_armor_total_data_t> data( bool ptr );

  double value( int subclass ) const
  {
    return as<unsigned>( subclass ) < range::size( values )
      ? values[ as<unsigned>( subclass ) ]
      : 0.0;
  }
};

struct item_armor_location_data_t
{
private:
  struct key_policy_t
  {
    static constexpr unsigned key( const item_armor_location_data_t& obj )
    { return obj.inv_type; }
  };

public:
  unsigned inv_type;
  double   multiplier[4];

  static const item_armor_location_data_t& find( unsigned inv_type, bool ptr )
  { return dbc::find<item_armor_location_data_t, key_policy_t>( inv_type, ptr ); }

  static const item_armor_location_data_t& nil()
  { return dbc::nil<item_armor_location_data_t>(); }

  static util::span<const item_armor_location_data_t> data( bool ptr );

  double value( int subclass ) const
  {
    return as<unsigned>( subclass ) < range::size( multiplier )
      ? multiplier[ as<unsigned>( subclass ) ]
      : 0.0;
  }
};

#endif /* ITEM_ARMOR_HPP */


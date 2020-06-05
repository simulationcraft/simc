// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_WEAPON_HPP
#define ITEM_WEAPON_HPP

#include "util/generic.hpp"
#include "util/span.hpp"

#include "client_data.hpp"

struct item_damage_one_hand_data_t
{
  unsigned ilevel;
  float    dps[7];

  static const item_damage_one_hand_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_damage_one_hand_data_t>( ilevel, ptr, &item_damage_one_hand_data_t::ilevel ); }

  static const item_damage_one_hand_data_t& nil()
  { return dbc::nil<item_damage_one_hand_data_t>; }

  static util::span<const item_damage_one_hand_data_t> data( bool ptr );

  float value( int quality ) const
  {
    return as<unsigned>( quality ) < range::size( dps )
      ? dps[ as<unsigned>( quality ) ]
      : 0;
  }
};

struct item_damage_one_hand_caster_data_t
{
  unsigned ilevel;
  float    dps[7];

  static const item_damage_one_hand_caster_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_damage_one_hand_caster_data_t>( ilevel, ptr, &item_damage_one_hand_caster_data_t::ilevel ); }

  static const item_damage_one_hand_caster_data_t& nil()
  { return dbc::nil<item_damage_one_hand_caster_data_t>; }

  static util::span<const item_damage_one_hand_caster_data_t> data( bool ptr );

  float value( int quality ) const
  {
    return as<unsigned>( quality ) < range::size( dps )
      ? dps[ as<unsigned>( quality ) ]
      : 0;
  }
};

struct item_damage_two_hand_data_t
{
  unsigned ilevel;
  float    dps[7];

  static const item_damage_two_hand_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_damage_two_hand_data_t>( ilevel, ptr, &item_damage_two_hand_data_t::ilevel ); }

  static const item_damage_two_hand_data_t& nil()
  { return dbc::nil<item_damage_two_hand_data_t>; }

  static util::span<const item_damage_two_hand_data_t> data( bool ptr );

  float value( int quality ) const
  {
    return as<unsigned>( quality ) < range::size( dps )
      ? dps[ as<unsigned>( quality ) ]
      : 0;
  }
};

struct item_damage_two_hand_caster_data_t
{
  unsigned ilevel;
  float    dps[7];

  static const item_damage_two_hand_caster_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_damage_two_hand_caster_data_t>( ilevel, ptr, &item_damage_two_hand_caster_data_t::ilevel ); }

  static const item_damage_two_hand_caster_data_t& nil()
  { return dbc::nil<item_damage_two_hand_caster_data_t>; }

  static util::span<const item_damage_two_hand_caster_data_t> data( bool ptr );

  float value( int quality ) const
  {
    return as<unsigned>( quality ) < range::size( dps )
      ? dps[ as<unsigned>( quality ) ]
      : 0;
  }
};

#endif /* ITEM_WEAPON_HPP */


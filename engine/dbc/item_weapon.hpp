// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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
private:
  using key_t = dbc::ilevel_member_policy_t;

public:
  unsigned ilevel;
  double   dps[7];

  static const item_damage_one_hand_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_damage_one_hand_data_t, key_t>( ilevel, ptr ); }

  static const item_damage_one_hand_data_t& nil()
  { return dbc::nil<item_damage_one_hand_data_t>(); }

  static util::span<const item_damage_one_hand_data_t> data( bool ptr );

  double value( int quality ) const
  {
    return as<unsigned>( quality ) < sizeof_array( dps )
      ? dps[ as<unsigned>( quality ) ]
      : 0.0;
  }
};

struct item_damage_one_hand_caster_data_t
{
private:
  using key_t = dbc::ilevel_member_policy_t;

public:
  unsigned ilevel;
  double   dps[7];

  static const item_damage_one_hand_caster_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_damage_one_hand_caster_data_t, key_t>( ilevel, ptr ); }

  static const item_damage_one_hand_caster_data_t& nil()
  { return dbc::nil<item_damage_one_hand_caster_data_t>(); }

  static util::span<const item_damage_one_hand_caster_data_t> data( bool ptr );

  double value( int quality ) const
  {
    return as<unsigned>( quality ) < sizeof_array( dps )
      ? dps[ as<unsigned>( quality ) ]
      : 0.0;
  }
};

struct item_damage_two_hand_data_t
{
private:
  using key_t = dbc::ilevel_member_policy_t;

public:
  unsigned ilevel;
  double   dps[7];

  static const item_damage_two_hand_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_damage_two_hand_data_t, key_t>( ilevel, ptr ); }

  static const item_damage_two_hand_data_t& nil()
  { return dbc::nil<item_damage_two_hand_data_t>(); }

  static util::span<const item_damage_two_hand_data_t> data( bool ptr );

  double value( int quality ) const
  {
    return as<unsigned>( quality ) < sizeof_array( dps )
      ? dps[ as<unsigned>( quality ) ]
      : 0.0;
  }
};

struct item_damage_two_hand_caster_data_t
{
private:
  using key_t = dbc::ilevel_member_policy_t;

public:
  unsigned ilevel;
  double   dps[7];

  static const item_damage_two_hand_caster_data_t& find( unsigned ilevel, bool ptr )
  { return dbc::find<item_damage_two_hand_caster_data_t, key_t>( ilevel, ptr ); }

  static const item_damage_two_hand_caster_data_t& nil()
  { return dbc::nil<item_damage_two_hand_caster_data_t>(); }

  static util::span<const item_damage_two_hand_caster_data_t> data( bool ptr );

  double value( int quality ) const
  {
    return as<unsigned>( quality ) < sizeof_array( dps )
      ? dps[ as<unsigned>( quality ) ]
      : 0.0;
  }
};

#endif /* ITEM_WEAPON_HPP */


// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_SET_BONUS_HPP
#define ITEM_SET_BONUS_HPP

#include "util/span.hpp"

#include "specialization.hpp"

#include "client_data.hpp"

#define SET_BONUS_ITEM_ID_MAX ( 17 )

struct item_set_bonus_t
{
  const char* set_name;
  const char* set_opt_name;
  unsigned    enum_id; // tier_e enum value.
  unsigned    set_id;
  unsigned    tier;
  unsigned    bonus;
  int         class_id;
  int         spec; // -1 "all"
  unsigned    spell_id;
  unsigned    item_ids[SET_BONUS_ITEM_ID_MAX];

  bool has_spec( int spec_id ) const
  {
    // Check dbc-driven specs
    if ( spec > 0 )
    {
      if ( spec_id == spec )
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    // Check all specs
    else if ( spec == -1 )
    {
      return true;
    }
    // Give up
    else
    {
      return false;
    }
  }

  bool has_spec( specialization_e spec ) const
  {
    return has_spec( static_cast<int>( spec ) );
  }

  bool has_item( unsigned item_id ) const
  {
    for ( size_t i = 0; i < SET_BONUS_ITEM_ID_MAX; i++ )
    {
      if ( item_ids[ i ] == item_id )
        return true;

      if ( item_ids[ i ] == 0 )
        break;
    }
    return false;
  }

  static util::span<const item_set_bonus_t> data( bool ptr );
};

#endif /* ITEM_SET_BONUS_HPP */


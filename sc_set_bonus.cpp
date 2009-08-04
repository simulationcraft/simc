// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// set_bonus_t::set_bonus_t =================================================

set_bonus_t::set_bonus_t()
{
  memset( ( void* ) this, 0x00, sizeof( set_bonus_t ) );

  count[ SET_T4_2PC  ] = count[ SET_T4_4PC  ] = -1;
  count[ SET_T5_2PC  ] = count[ SET_T5_4PC  ] = -1;
  count[ SET_T6_2PC  ] = count[ SET_T6_4PC  ] = -1;
  count[ SET_T7_2PC  ] = count[ SET_T7_4PC  ] = -1;
  count[ SET_T8_2PC  ] = count[ SET_T8_4PC  ] = -1;
  count[ SET_T9_2PC  ] = count[ SET_T9_4PC  ] = -1;
  count[ SET_T10_2PC ] = count[ SET_T10_4PC ] = -1;
}

// set_bonus_t::tier4 =======================================================

int set_bonus_t::tier4_2pc() SC_CONST { return ( count[ SET_T4_2PC ] > 0 || ( count[ SET_T4_2PC ] < 0 && count[ SET_T4 ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier4_4pc() SC_CONST { return ( count[ SET_T4_4PC ] > 0 || ( count[ SET_T4_4PC ] < 0 && count[ SET_T4 ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier5 =======================================================

int set_bonus_t::tier5_2pc() SC_CONST { return ( count[ SET_T5_2PC ] > 0 || ( count[ SET_T5_2PC ] < 0 && count[ SET_T5 ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier5_4pc() SC_CONST { return ( count[ SET_T5_4PC ] > 0 || ( count[ SET_T5_4PC ] < 0 && count[ SET_T5 ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier6 =======================================================

int set_bonus_t::tier6_2pc() SC_CONST { return ( count[ SET_T6_2PC ] > 0 || ( count[ SET_T6_2PC ] < 0 && count[ SET_T6 ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier6_4pc() SC_CONST { return ( count[ SET_T6_4PC ] > 0 || ( count[ SET_T6_4PC ] < 0 && count[ SET_T6 ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier7 =======================================================

int set_bonus_t::tier7_2pc() SC_CONST { return ( count[ SET_T7_2PC ] > 0 || ( count[ SET_T7_2PC ] < 0 && count[ SET_T7 ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier7_4pc() SC_CONST { return ( count[ SET_T7_4PC ] > 0 || ( count[ SET_T7_4PC ] < 0 && count[ SET_T7 ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier8 =======================================================

int set_bonus_t::tier8_2pc() SC_CONST { return ( count[ SET_T8_2PC ] > 0 || ( count[ SET_T8_2PC ] < 0 && count[ SET_T8 ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier8_4pc() SC_CONST { return ( count[ SET_T8_4PC ] > 0 || ( count[ SET_T8_4PC ] < 0 && count[ SET_T8 ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier9 =======================================================

int set_bonus_t::tier9_2pc() SC_CONST { return ( count[ SET_T9_2PC ] > 0 || ( count[ SET_T9_2PC ] < 0 && count[ SET_T9 ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier9_4pc() SC_CONST { return ( count[ SET_T9_4PC ] > 0 || ( count[ SET_T9_4PC ] < 0 && count[ SET_T9 ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier10 =======================================================

int set_bonus_t::tier10_2pc() SC_CONST { return ( count[ SET_T10_2PC ] > 0 || ( count[ SET_T10_2PC ] < 0 && count[ SET_T10 ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier10_4pc() SC_CONST { return ( count[ SET_T10_4PC ] > 0 || ( count[ SET_T10_4PC ] < 0 && count[ SET_T10 ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::spellstrike =================================================

int set_bonus_t::spellstrike() SC_CONST { return ( count[ SET_SPELLSTRIKE ] >= 2 ) ? 1 : 0; }

// set_bonus_t::decode ======================================================

int set_bonus_t::decode( player_t* p,
                         item_t&   item ) SC_CONST
{
  if ( ! item.name() ) return SET_NONE;

  int set = p -> decode_set( item );
  if ( set != SET_NONE ) return set;

  if ( strstr( item.name(), "spellstrike"  ) ) return SET_SPELLSTRIKE;

  return SET_NONE;
}

// set_bonus_t::init ========================================================

bool set_bonus_t::init( player_t* p )
{
  int num_items = p -> items.size();

  for ( int i=0; i < num_items; i++ )
  {
    count[ decode( p, p -> items[ i ] ) ] += 1;
  }

  return true;
}


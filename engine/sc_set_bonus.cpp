// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// set_bonus_t::set_bonus_t =================================================

set_bonus_t::set_bonus_t()
{
  memset( ( void* ) this, 0x00, sizeof( set_bonus_t ) );

  count[ SET_T6_2PC_CASTER  ] = count[ SET_T6_2PC_MELEE  ] = count[ SET_T6_2PC_TANK  ] = -1;
  count[ SET_T6_4PC_CASTER  ] = count[ SET_T6_4PC_MELEE  ] = count[ SET_T6_4PC_TANK  ] = -1;
  count[ SET_T7_2PC_CASTER  ] = count[ SET_T7_2PC_MELEE  ] = count[ SET_T7_2PC_TANK  ] = -1;
  count[ SET_T7_4PC_CASTER  ] = count[ SET_T7_4PC_MELEE  ] = count[ SET_T7_4PC_TANK  ] = -1;
  count[ SET_T8_2PC_CASTER  ] = count[ SET_T8_2PC_MELEE  ] = count[ SET_T8_2PC_TANK  ] = -1;
  count[ SET_T8_4PC_CASTER  ] = count[ SET_T8_4PC_MELEE  ] = count[ SET_T8_4PC_TANK  ] = -1;
  count[ SET_T9_2PC_CASTER  ] = count[ SET_T9_2PC_MELEE  ] = count[ SET_T9_2PC_TANK  ] = -1;
  count[ SET_T9_4PC_CASTER  ] = count[ SET_T9_4PC_MELEE  ] = count[ SET_T9_4PC_TANK  ] = -1;
  count[ SET_T10_2PC_CASTER ] = count[ SET_T10_2PC_MELEE ] = count[ SET_T10_2PC_TANK ] = -1;
  count[ SET_T10_4PC_CASTER ] = count[ SET_T10_4PC_MELEE ] = count[ SET_T10_4PC_TANK ] = -1;
}

// set_bonus_t::tier6 =======================================================

int set_bonus_t::tier6_2pc_caster() SC_CONST { return ( count[ SET_T6_2PC_CASTER ] > 0 || ( count[ SET_T6_2PC_CASTER ] < 0 && count[ SET_T6_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier6_4pc_caster() SC_CONST { return ( count[ SET_T6_4PC_CASTER ] > 0 || ( count[ SET_T6_4PC_CASTER ] < 0 && count[ SET_T6_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier6_2pc_melee() SC_CONST { return ( count[ SET_T6_2PC_MELEE ] > 0 || ( count[ SET_T6_2PC_MELEE ] < 0 && count[ SET_T6_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier6_4pc_melee() SC_CONST { return ( count[ SET_T6_4PC_MELEE ] > 0 || ( count[ SET_T6_4PC_MELEE ] < 0 && count[ SET_T6_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier6_2pc_tank() SC_CONST { return ( count[ SET_T6_2PC_TANK ] > 0 || ( count[ SET_T6_2PC_TANK ] < 0 && count[ SET_T6_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier6_4pc_tank() SC_CONST { return ( count[ SET_T6_4PC_TANK ] > 0 || ( count[ SET_T6_4PC_TANK ] < 0 && count[ SET_T6_TANK ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier7 =======================================================

int set_bonus_t::tier7_2pc_caster() SC_CONST { return ( count[ SET_T7_2PC_CASTER ] > 0 || ( count[ SET_T7_2PC_CASTER ] < 0 && count[ SET_T7_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier7_4pc_caster() SC_CONST { return ( count[ SET_T7_4PC_CASTER ] > 0 || ( count[ SET_T7_4PC_CASTER ] < 0 && count[ SET_T7_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier7_2pc_melee() SC_CONST { return ( count[ SET_T7_2PC_MELEE ] > 0 || ( count[ SET_T7_2PC_MELEE ] < 0 && count[ SET_T7_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier7_4pc_melee() SC_CONST { return ( count[ SET_T7_4PC_MELEE ] > 0 || ( count[ SET_T7_4PC_MELEE ] < 0 && count[ SET_T7_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier7_2pc_tank() SC_CONST { return ( count[ SET_T7_2PC_TANK ] > 0 || ( count[ SET_T7_2PC_TANK ] < 0 && count[ SET_T7_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier7_4pc_tank() SC_CONST { return ( count[ SET_T7_4PC_TANK ] > 0 || ( count[ SET_T7_4PC_TANK ] < 0 && count[ SET_T7_TANK ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier8 =======================================================

int set_bonus_t::tier8_2pc_caster() SC_CONST { return ( count[ SET_T8_2PC_CASTER ] > 0 || ( count[ SET_T8_2PC_CASTER ] < 0 && count[ SET_T8_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier8_4pc_caster() SC_CONST { return ( count[ SET_T8_4PC_CASTER ] > 0 || ( count[ SET_T8_4PC_CASTER ] < 0 && count[ SET_T8_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier8_2pc_melee() SC_CONST { return ( count[ SET_T8_2PC_MELEE ] > 0 || ( count[ SET_T8_2PC_MELEE ] < 0 && count[ SET_T8_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier8_4pc_melee() SC_CONST { return ( count[ SET_T8_4PC_MELEE ] > 0 || ( count[ SET_T8_4PC_MELEE ] < 0 && count[ SET_T8_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier8_2pc_tank() SC_CONST { return ( count[ SET_T8_2PC_TANK ] > 0 || ( count[ SET_T8_2PC_TANK ] < 0 && count[ SET_T8_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier8_4pc_tank() SC_CONST { return ( count[ SET_T8_4PC_TANK ] > 0 || ( count[ SET_T8_4PC_TANK ] < 0 && count[ SET_T8_TANK ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier9 =======================================================

int set_bonus_t::tier9_2pc_caster() SC_CONST { return ( count[ SET_T9_2PC_CASTER ] > 0 || ( count[ SET_T9_2PC_CASTER ] < 0 && count[ SET_T9_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier9_4pc_caster() SC_CONST { return ( count[ SET_T9_4PC_CASTER ] > 0 || ( count[ SET_T9_4PC_CASTER ] < 0 && count[ SET_T9_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier9_2pc_melee() SC_CONST { return ( count[ SET_T9_2PC_MELEE ] > 0 || ( count[ SET_T9_2PC_MELEE ] < 0 && count[ SET_T9_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier9_4pc_melee() SC_CONST { return ( count[ SET_T9_4PC_MELEE ] > 0 || ( count[ SET_T9_4PC_MELEE ] < 0 && count[ SET_T9_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier9_2pc_tank() SC_CONST { return ( count[ SET_T9_2PC_TANK ] > 0 || ( count[ SET_T9_2PC_TANK ] < 0 && count[ SET_T9_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier9_4pc_tank() SC_CONST { return ( count[ SET_T9_4PC_TANK ] > 0 || ( count[ SET_T9_4PC_TANK ] < 0 && count[ SET_T9_TANK ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier10 =======================================================

int set_bonus_t::tier10_2pc_caster() SC_CONST { return ( count[ SET_T10_2PC_CASTER ] > 0 || ( count[ SET_T10_2PC_CASTER ] < 0 && count[ SET_T10_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier10_4pc_caster() SC_CONST { return ( count[ SET_T10_4PC_CASTER ] > 0 || ( count[ SET_T10_4PC_CASTER ] < 0 && count[ SET_T10_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier10_2pc_melee() SC_CONST { return ( count[ SET_T10_2PC_MELEE ] > 0 || ( count[ SET_T10_2PC_MELEE ] < 0 && count[ SET_T10_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier10_4pc_melee() SC_CONST { return ( count[ SET_T10_4PC_MELEE ] > 0 || ( count[ SET_T10_4PC_MELEE ] < 0 && count[ SET_T10_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier10_2pc_tank() SC_CONST { return ( count[ SET_T10_2PC_TANK ] > 0 || ( count[ SET_T10_2PC_TANK ] < 0 && count[ SET_T10_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier10_4pc_tank() SC_CONST { return ( count[ SET_T10_4PC_TANK ] > 0 || ( count[ SET_T10_4PC_TANK ] < 0 && count[ SET_T10_TANK ] >= 4 ) ) ? 1 : 0; }

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
  int num_items = ( int ) p -> items.size();

  for ( int i=0; i < num_items; i++ )
  {
    count[ decode( p, p -> items[ i ] ) ] += 1;
  }

  return true;
}


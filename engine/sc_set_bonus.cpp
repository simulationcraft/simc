// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// set_bonus_t::set_bonus_t =================================================

set_bonus_t::set_bonus_t()
{
  memset( ( void* ) this, 0x00, sizeof( set_bonus_t ) );

  count[ SET_T11_2PC_CASTER ] = count[ SET_T11_2PC_MELEE ] = count[ SET_T11_2PC_TANK ] = count[ SET_T11_2PC_HEAL ] = -1;
  count[ SET_T11_4PC_CASTER ] = count[ SET_T11_4PC_MELEE ] = count[ SET_T11_4PC_TANK ] = count[ SET_T11_4PC_HEAL ] = -1;
  count[ SET_T12_2PC_CASTER ] = count[ SET_T12_2PC_MELEE ] = count[ SET_T12_2PC_TANK ] = count[ SET_T12_2PC_HEAL ] = -1;
  count[ SET_T12_4PC_CASTER ] = count[ SET_T12_4PC_MELEE ] = count[ SET_T12_4PC_TANK ] = count[ SET_T12_4PC_HEAL ] = -1;
  count[ SET_T13_2PC_CASTER ] = count[ SET_T13_2PC_MELEE ] = count[ SET_T13_2PC_TANK ] = count[ SET_T13_2PC_HEAL ] = -1;
  count[ SET_T13_4PC_CASTER ] = count[ SET_T13_4PC_MELEE ] = count[ SET_T13_4PC_TANK ] = count[ SET_T13_4PC_HEAL ] = -1;
  count[ SET_T14_2PC_CASTER ] = count[ SET_T14_2PC_MELEE ] = count[ SET_T14_2PC_TANK ] = count[ SET_T14_2PC_HEAL ] = -1;
  count[ SET_T14_4PC_CASTER ] = count[ SET_T14_4PC_MELEE ] = count[ SET_T14_4PC_TANK ] = count[ SET_T14_4PC_HEAL ] = -1;
}

// set_bonus_t::tier11 =======================================================

int set_bonus_t::tier11_2pc_caster() SC_CONST { return ( count[ SET_T11_2PC_CASTER ] > 0 || ( count[ SET_T11_2PC_CASTER ] < 0 && count[ SET_T11_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier11_4pc_caster() SC_CONST { return ( count[ SET_T11_4PC_CASTER ] > 0 || ( count[ SET_T11_4PC_CASTER ] < 0 && count[ SET_T11_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier11_2pc_melee() SC_CONST { return ( count[ SET_T11_2PC_MELEE ] > 0 || ( count[ SET_T11_2PC_MELEE ] < 0 && count[ SET_T11_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier11_4pc_melee() SC_CONST { return ( count[ SET_T11_4PC_MELEE ] > 0 || ( count[ SET_T11_4PC_MELEE ] < 0 && count[ SET_T11_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier11_2pc_tank() SC_CONST { return ( count[ SET_T11_2PC_TANK ] > 0 || ( count[ SET_T11_2PC_TANK ] < 0 && count[ SET_T11_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier11_4pc_tank() SC_CONST { return ( count[ SET_T11_4PC_TANK ] > 0 || ( count[ SET_T11_4PC_TANK ] < 0 && count[ SET_T11_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier11_2pc_heal() SC_CONST { return ( count[ SET_T11_2PC_HEAL ] > 0 || ( count[ SET_T11_2PC_HEAL ] < 0 && count[ SET_T11_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier11_4pc_heal() SC_CONST { return ( count[ SET_T11_4PC_HEAL ] > 0 || ( count[ SET_T11_4PC_HEAL ] < 0 && count[ SET_T11_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier12 =======================================================

int set_bonus_t::tier12_2pc_caster() SC_CONST { return ( count[ SET_T12_2PC_CASTER ] > 0 || ( count[ SET_T12_2PC_CASTER ] < 0 && count[ SET_T12_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier12_4pc_caster() SC_CONST { return ( count[ SET_T12_4PC_CASTER ] > 0 || ( count[ SET_T12_4PC_CASTER ] < 0 && count[ SET_T12_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier12_2pc_melee() SC_CONST { return ( count[ SET_T12_2PC_MELEE ] > 0 || ( count[ SET_T12_2PC_MELEE ] < 0 && count[ SET_T12_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier12_4pc_melee() SC_CONST { return ( count[ SET_T12_4PC_MELEE ] > 0 || ( count[ SET_T12_4PC_MELEE ] < 0 && count[ SET_T12_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier12_2pc_tank() SC_CONST { return ( count[ SET_T12_2PC_TANK ] > 0 || ( count[ SET_T12_2PC_TANK ] < 0 && count[ SET_T12_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier12_4pc_tank() SC_CONST { return ( count[ SET_T12_4PC_TANK ] > 0 || ( count[ SET_T12_4PC_TANK ] < 0 && count[ SET_T12_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier12_2pc_heal() SC_CONST { return ( count[ SET_T12_2PC_HEAL ] > 0 || ( count[ SET_T12_2PC_HEAL ] < 0 && count[ SET_T12_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier12_4pc_heal() SC_CONST { return ( count[ SET_T12_4PC_HEAL ] > 0 || ( count[ SET_T12_4PC_HEAL ] < 0 && count[ SET_T12_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier13 =======================================================

int set_bonus_t::tier13_2pc_caster() SC_CONST { return ( count[ SET_T13_2PC_CASTER ] > 0 || ( count[ SET_T13_2PC_CASTER ] < 0 && count[ SET_T13_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_caster() SC_CONST { return ( count[ SET_T13_4PC_CASTER ] > 0 || ( count[ SET_T13_4PC_CASTER ] < 0 && count[ SET_T13_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_melee() SC_CONST { return ( count[ SET_T13_2PC_MELEE ] > 0 || ( count[ SET_T13_2PC_MELEE ] < 0 && count[ SET_T13_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_melee() SC_CONST { return ( count[ SET_T13_4PC_MELEE ] > 0 || ( count[ SET_T13_4PC_MELEE ] < 0 && count[ SET_T13_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_tank() SC_CONST { return ( count[ SET_T13_2PC_TANK ] > 0 || ( count[ SET_T13_2PC_TANK ] < 0 && count[ SET_T13_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_tank() SC_CONST { return ( count[ SET_T13_4PC_TANK ] > 0 || ( count[ SET_T13_4PC_TANK ] < 0 && count[ SET_T13_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_heal() SC_CONST { return ( count[ SET_T13_2PC_HEAL ] > 0 || ( count[ SET_T13_2PC_HEAL ] < 0 && count[ SET_T13_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_heal() SC_CONST { return ( count[ SET_T13_4PC_HEAL ] > 0 || ( count[ SET_T13_4PC_HEAL ] < 0 && count[ SET_T13_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier14 =======================================================

int set_bonus_t::tier14_2pc_caster() SC_CONST { return ( count[ SET_T14_2PC_CASTER ] > 0 || ( count[ SET_T14_2PC_CASTER ] < 0 && count[ SET_T14_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_caster() SC_CONST { return ( count[ SET_T14_4PC_CASTER ] > 0 || ( count[ SET_T14_4PC_CASTER ] < 0 && count[ SET_T14_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_melee() SC_CONST { return ( count[ SET_T14_2PC_MELEE ] > 0 || ( count[ SET_T14_2PC_MELEE ] < 0 && count[ SET_T14_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_melee() SC_CONST { return ( count[ SET_T14_4PC_MELEE ] > 0 || ( count[ SET_T14_4PC_MELEE ] < 0 && count[ SET_T14_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_tank() SC_CONST { return ( count[ SET_T14_2PC_TANK ] > 0 || ( count[ SET_T14_2PC_TANK ] < 0 && count[ SET_T14_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_tank() SC_CONST { return ( count[ SET_T14_4PC_TANK ] > 0 || ( count[ SET_T14_4PC_TANK ] < 0 && count[ SET_T14_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_heal() SC_CONST { return ( count[ SET_T14_2PC_HEAL ] > 0 || ( count[ SET_T14_2PC_HEAL ] < 0 && count[ SET_T14_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_heal() SC_CONST { return ( count[ SET_T14_4PC_HEAL ] > 0 || ( count[ SET_T14_4PC_HEAL ] < 0 && count[ SET_T14_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::decode ======================================================

int set_bonus_t::decode( player_t* p,
                         item_t&   item ) SC_CONST
{
  if ( ! item.name() ) return SET_NONE;

  int set = p -> decode_set( item );
  if ( set != SET_NONE ) return set;

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

action_expr_t* set_bonus_t::create_expression( action_t* action,
                                               const std::string& type )
{
  set_type bonus_type = util_t::parse_set_bonus( type );

  if ( bonus_type != SET_NONE )
  {
    struct set_bonus_expr_t : public action_expr_t
    {
      set_type set_bonus_type;
      set_bonus_expr_t( action_t* a, set_type bonus_type ) : action_expr_t( a, util_t::set_bonus_string( bonus_type ), TOK_NUM ), set_bonus_type( bonus_type ) {}
      virtual int evaluate() { result_num = action -> player -> sets -> has_set_bonus( set_bonus_type ); return TOK_NUM; }
    };
    return new set_bonus_expr_t( action, bonus_type );
  }

  return 0;
}

// set_bonus_array_t::set_bonus_array_t ======================================

set_bonus_array_t::set_bonus_array_t( player_t* p, uint32_t a_bonus[ N_TIER ][ N_TIER_BONUS ] ) :
  p ( p )
{
  memset( set_bonuses, 0, sizeof( set_bonuses ) );

  // Map two-dimensional array into correct slots in the one-dimensional set_bonuses
  // array, based on set_type enum
  for ( int i = 0; i < N_TIER; i++ )
  {
    for ( int j = 0; j < N_TIER_BONUS; j++ )
    {
      int b = 0;

      switch( j )
      {
      // 2pc/4pc caster
      case 0:
      case 1:
        b = j + 1;
        break;
      // 2pc/4pc melee
      case 2:
      case 3:
        b = j + 2;
        break;
      case 4:
      case 5:
        b = j + 3;
        break;
      case 6:
      case 7:
        b = j + 4;
        break;
      default:
        break;
      }

      set_bonuses[ 1 + i * 12 + b ] = create_set_bonus( p, a_bonus[ i ][ j ] );

      // if ( set_bonuses[ 1 + i * 12 + b ] )
      //  log_t::output( p -> sim, "Initializing set bonus %u to slot %d", a_bonus[ i ][ j ], 1 + i * 9 + b );
    }
  }

  // Dummy default value that returns 0 always to everything, so missing
  // set bonuses will never give out a value nor crash, even if you dont
  // if ( p -> set_bonus.tierX_Ypc_caster() ), which isnt even necessary
  // in this system
  default_value = new spell_id_t( p, 0 );
}

set_bonus_array_t::~set_bonus_array_t()
{
  for ( int i = 0; i < SET_MAX; i++ )
    delete set_bonuses[ i ];

  delete default_value;
}

// SET_T10_2PC_CASTER = 38, tier = 4, btype = 0, bonus = 1
// 1 + 4 * 9 + 0 * 3 >= 1 * 2
// 1 + 36 + 0 >= 2
// count[ 38 ] > 0 || ( count[ 38 ] < 0 && count[ 37 ] >= 2 )
bool set_bonus_array_t::has_set_bonus( set_type s ) SC_CONST
{
  int tier  = ( s - 1 ) / 12,
      btype = ( ( s - 1 ) - tier * 12 ) / 3,
      bonus = ( s - 1 ) % 3;

  if ( p -> set_bonus.count[ s ] > 0 || ( p -> set_bonus.count[ s ] < 0 && p -> set_bonus.count[ 1 + tier * 12 + btype * 3 ] >= bonus * 2 ) )
    return true;

  return false;
}

const spell_id_t* set_bonus_array_t::set( set_type s ) SC_CONST
{
  if ( has_set_bonus( s ) && set_bonuses[ s ] )
    return set_bonuses[ s ];

  return default_value;
}

// Override this one to create custom spell_id_t returns
const spell_id_t* set_bonus_array_t::create_set_bonus( player_t* p, uint32_t spell_id ) SC_CONST
{
  if ( ! p -> dbc.spell( spell_id ) )
  {
    if ( spell_id > 0 )
    {
      p -> sim -> errorf( "Set bonus spell identifier %u for %s not found in spell data.",
      spell_id,
      p -> name_str.c_str() );
    }
    return 0;
  }
  return new spell_id_t( p, "", spell_id );
}

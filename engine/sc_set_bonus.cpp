// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// set_bonus_t::set_bonus_t =================================================

set_bonus_t::set_bonus_t()
{
  range::fill( count, 0 );

  count[ SET_T11_2PC_CASTER ] = count[ SET_T11_2PC_MELEE ] = count[ SET_T11_2PC_TANK ] = count[ SET_T11_2PC_HEAL ] = -1;
  count[ SET_T11_4PC_CASTER ] = count[ SET_T11_4PC_MELEE ] = count[ SET_T11_4PC_TANK ] = count[ SET_T11_4PC_HEAL ] = -1;
  count[ SET_T12_2PC_CASTER ] = count[ SET_T12_2PC_MELEE ] = count[ SET_T12_2PC_TANK ] = count[ SET_T12_2PC_HEAL ] = -1;
  count[ SET_T12_4PC_CASTER ] = count[ SET_T12_4PC_MELEE ] = count[ SET_T12_4PC_TANK ] = count[ SET_T12_4PC_HEAL ] = -1;
  count[ SET_T13_2PC_CASTER ] = count[ SET_T13_2PC_MELEE ] = count[ SET_T13_2PC_TANK ] = count[ SET_T13_2PC_HEAL ] = -1;
  count[ SET_T13_4PC_CASTER ] = count[ SET_T13_4PC_MELEE ] = count[ SET_T13_4PC_TANK ] = count[ SET_T13_4PC_HEAL ] = -1;
  count[ SET_T14_2PC_CASTER ] = count[ SET_T14_2PC_MELEE ] = count[ SET_T14_2PC_TANK ] = count[ SET_T14_2PC_HEAL ] = -1;
  count[ SET_T14_4PC_CASTER ] = count[ SET_T14_4PC_MELEE ] = count[ SET_T14_4PC_TANK ] = count[ SET_T14_4PC_HEAL ] = -1;
  count[ SET_PVP_2PC_CASTER ] = count[ SET_PVP_2PC_MELEE ] = count[ SET_PVP_2PC_TANK ] = count[ SET_PVP_2PC_HEAL ] = -1;
  count[ SET_PVP_4PC_CASTER ] = count[ SET_PVP_4PC_MELEE ] = count[ SET_PVP_4PC_TANK ] = count[ SET_PVP_4PC_HEAL ] = -1;
}

// set_bonus_t::tier11 ======================================================

int set_bonus_t::tier11_2pc_caster() const { return ( count[ SET_T11_2PC_CASTER ] > 0 || ( count[ SET_T11_2PC_CASTER ] < 0 && count[ SET_T11_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier11_4pc_caster() const { return ( count[ SET_T11_4PC_CASTER ] > 0 || ( count[ SET_T11_4PC_CASTER ] < 0 && count[ SET_T11_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier11_2pc_melee() const { return ( count[ SET_T11_2PC_MELEE ] > 0 || ( count[ SET_T11_2PC_MELEE ] < 0 && count[ SET_T11_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier11_4pc_melee() const { return ( count[ SET_T11_4PC_MELEE ] > 0 || ( count[ SET_T11_4PC_MELEE ] < 0 && count[ SET_T11_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier11_2pc_tank() const { return ( count[ SET_T11_2PC_TANK ] > 0 || ( count[ SET_T11_2PC_TANK ] < 0 && count[ SET_T11_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier11_4pc_tank() const { return ( count[ SET_T11_4PC_TANK ] > 0 || ( count[ SET_T11_4PC_TANK ] < 0 && count[ SET_T11_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier11_2pc_heal() const { return ( count[ SET_T11_2PC_HEAL ] > 0 || ( count[ SET_T11_2PC_HEAL ] < 0 && count[ SET_T11_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier11_4pc_heal() const { return ( count[ SET_T11_4PC_HEAL ] > 0 || ( count[ SET_T11_4PC_HEAL ] < 0 && count[ SET_T11_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier12 ======================================================

int set_bonus_t::tier12_2pc_caster() const { return ( count[ SET_T12_2PC_CASTER ] > 0 || ( count[ SET_T12_2PC_CASTER ] < 0 && count[ SET_T12_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier12_4pc_caster() const { return ( count[ SET_T12_4PC_CASTER ] > 0 || ( count[ SET_T12_4PC_CASTER ] < 0 && count[ SET_T12_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier12_2pc_melee() const { return ( count[ SET_T12_2PC_MELEE ] > 0 || ( count[ SET_T12_2PC_MELEE ] < 0 && count[ SET_T12_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier12_4pc_melee() const { return ( count[ SET_T12_4PC_MELEE ] > 0 || ( count[ SET_T12_4PC_MELEE ] < 0 && count[ SET_T12_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier12_2pc_tank() const { return ( count[ SET_T12_2PC_TANK ] > 0 || ( count[ SET_T12_2PC_TANK ] < 0 && count[ SET_T12_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier12_4pc_tank() const { return ( count[ SET_T12_4PC_TANK ] > 0 || ( count[ SET_T12_4PC_TANK ] < 0 && count[ SET_T12_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier12_2pc_heal() const { return ( count[ SET_T12_2PC_HEAL ] > 0 || ( count[ SET_T12_2PC_HEAL ] < 0 && count[ SET_T12_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier12_4pc_heal() const { return ( count[ SET_T12_4PC_HEAL ] > 0 || ( count[ SET_T12_4PC_HEAL ] < 0 && count[ SET_T12_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier13 ======================================================

int set_bonus_t::tier13_2pc_caster() const { return ( count[ SET_T13_2PC_CASTER ] > 0 || ( count[ SET_T13_2PC_CASTER ] < 0 && count[ SET_T13_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_caster() const { return ( count[ SET_T13_4PC_CASTER ] > 0 || ( count[ SET_T13_4PC_CASTER ] < 0 && count[ SET_T13_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_melee() const { return ( count[ SET_T13_2PC_MELEE ] > 0 || ( count[ SET_T13_2PC_MELEE ] < 0 && count[ SET_T13_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_melee() const { return ( count[ SET_T13_4PC_MELEE ] > 0 || ( count[ SET_T13_4PC_MELEE ] < 0 && count[ SET_T13_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_tank() const { return ( count[ SET_T13_2PC_TANK ] > 0 || ( count[ SET_T13_2PC_TANK ] < 0 && count[ SET_T13_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_tank() const { return ( count[ SET_T13_4PC_TANK ] > 0 || ( count[ SET_T13_4PC_TANK ] < 0 && count[ SET_T13_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_heal() const { return ( count[ SET_T13_2PC_HEAL ] > 0 || ( count[ SET_T13_2PC_HEAL ] < 0 && count[ SET_T13_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_heal() const { return ( count[ SET_T13_4PC_HEAL ] > 0 || ( count[ SET_T13_4PC_HEAL ] < 0 && count[ SET_T13_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier14 ======================================================

int set_bonus_t::tier14_2pc_caster() const { return ( count[ SET_T14_2PC_CASTER ] > 0 || ( count[ SET_T14_2PC_CASTER ] < 0 && count[ SET_T14_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_caster() const { return ( count[ SET_T14_4PC_CASTER ] > 0 || ( count[ SET_T14_4PC_CASTER ] < 0 && count[ SET_T14_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_melee() const { return ( count[ SET_T14_2PC_MELEE ] > 0 || ( count[ SET_T14_2PC_MELEE ] < 0 && count[ SET_T14_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_melee() const { return ( count[ SET_T14_4PC_MELEE ] > 0 || ( count[ SET_T14_4PC_MELEE ] < 0 && count[ SET_T14_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_tank() const { return ( count[ SET_T14_2PC_TANK ] > 0 || ( count[ SET_T14_2PC_TANK ] < 0 && count[ SET_T14_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_tank() const { return ( count[ SET_T14_4PC_TANK ] > 0 || ( count[ SET_T14_4PC_TANK ] < 0 && count[ SET_T14_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_heal() const { return ( count[ SET_T14_2PC_HEAL ] > 0 || ( count[ SET_T14_2PC_HEAL ] < 0 && count[ SET_T14_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_heal() const { return ( count[ SET_T14_4PC_HEAL ] > 0 || ( count[ SET_T14_4PC_HEAL ] < 0 && count[ SET_T14_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::pvp =========================================================

int set_bonus_t::pvp_2pc_caster() const { return ( count[ SET_PVP_2PC_CASTER ] > 0 || ( count[ SET_PVP_2PC_CASTER ] < 0 && count[ SET_PVP_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::pvp_4pc_caster() const { return ( count[ SET_PVP_4PC_CASTER ] > 0 || ( count[ SET_PVP_4PC_CASTER ] < 0 && count[ SET_PVP_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::pvp_2pc_melee() const { return ( count[ SET_PVP_2PC_MELEE ] > 0 || ( count[ SET_PVP_2PC_MELEE ] < 0 && count[ SET_PVP_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::pvp_4pc_melee() const { return ( count[ SET_PVP_4PC_MELEE ] > 0 || ( count[ SET_PVP_4PC_MELEE ] < 0 && count[ SET_PVP_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::pvp_2pc_tank() const { return ( count[ SET_PVP_2PC_TANK ] > 0 || ( count[ SET_PVP_2PC_TANK ] < 0 && count[ SET_PVP_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::pvp_4pc_tank() const { return ( count[ SET_PVP_4PC_TANK ] > 0 || ( count[ SET_PVP_4PC_TANK ] < 0 && count[ SET_PVP_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::pvp_2pc_heal() const { return ( count[ SET_PVP_2PC_HEAL ] > 0 || ( count[ SET_PVP_2PC_HEAL ] < 0 && count[ SET_PVP_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::pvp_4pc_heal() const { return ( count[ SET_PVP_4PC_HEAL ] > 0 || ( count[ SET_PVP_4PC_HEAL ] < 0 && count[ SET_PVP_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::decode ======================================================

int set_bonus_t::decode( player_t* p,
                         item_t&   item ) const
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

inline spell_id_t* set_bonus_array_t::create_set_bonus( uint32_t spell_id )
{
  if ( ! p -> dbc.spell( spell_id ) )
  {
    if ( spell_id > 0 )
    {
      p -> sim -> errorf( "Set bonus spell identifier %u for %s not found in spell data.",
                          spell_id, p -> name_str.c_str() );
    }
    return 0;
  }

  return new spell_id_t( p, "", spell_id );
}

// set_bonus_array_t::set_bonus_array_t =====================================

set_bonus_array_t::set_bonus_array_t( player_t* p, const uint32_t a_bonus[ N_TIER ][ N_TIER_BONUS ] ) :
  default_value( new spell_id_t( p, 0 ) ), set_bonuses(), p( p )
{
  // Map two-dimensional array into correct slots in the one-dimensional set_bonuses
  // array, based on set_type enum
  for ( int tier = 0; tier < N_TIER; ++tier )
  {
    for ( int j = 0; j < N_TIER_BONUS; j++ )
    {
      int b = 3 * j / 2 + 1;
      set_bonuses[ 1 + tier * 12 + b ].reset( create_set_bonus( a_bonus[ tier ][ j ] ) );
    }
  }
}

bool set_bonus_array_t::has_set_bonus( set_type s ) const
{
  if ( p -> set_bonus.count[ s ] > 0 )
    return true;

  if ( p -> set_bonus.count[ s ] < 0 )
  {
    assert( s > 0 );
    int tier  = ( s - 1 ) / 12,
        btype = ( ( s - 1 ) % 12 ) / 3,
        bonus = ( s - 1 ) % 3;

    assert( 1 + tier * 12 + btype * 3 < static_cast<int>( sizeof_array( p -> set_bonus.count ) ) );
    if ( p -> set_bonus.count[ 1 + tier * 12 + btype * 3 ] >= bonus * 2 )
      return true;
  }

  return false;
}

const spell_id_t* set_bonus_array_t::set( set_type s ) const
{
  if ( has_set_bonus( s ) && set_bonuses[ s ].get() )
    return set_bonuses[ s ].get();

  return default_value.get();
}

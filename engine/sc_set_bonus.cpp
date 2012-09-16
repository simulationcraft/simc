// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// set_bonus_t::set_bonus_t =================================================

set_bonus_t::set_bonus_t()
{
  range::fill( count, 0 );

  count[ SET_T13_2PC_CASTER ] = count[ SET_T13_2PC_MELEE ] = count[ SET_T13_2PC_TANK ] = count[ SET_T13_2PC_HEAL ] = -1;
  count[ SET_T13_4PC_CASTER ] = count[ SET_T13_4PC_MELEE ] = count[ SET_T13_4PC_TANK ] = count[ SET_T13_4PC_HEAL ] = -1;
  count[ SET_T14_2PC_CASTER ] = count[ SET_T14_2PC_MELEE ] = count[ SET_T14_2PC_TANK ] = count[ SET_T14_2PC_HEAL ] = -1;
  count[ SET_T14_4PC_CASTER ] = count[ SET_T14_4PC_MELEE ] = count[ SET_T14_4PC_TANK ] = count[ SET_T14_4PC_HEAL ] = -1;
  count[ SET_T15_2PC_CASTER ] = count[ SET_T15_2PC_MELEE ] = count[ SET_T15_2PC_TANK ] = count[ SET_T15_2PC_HEAL ] = -1;
  count[ SET_T15_4PC_CASTER ] = count[ SET_T15_4PC_MELEE ] = count[ SET_T15_4PC_TANK ] = count[ SET_T15_4PC_HEAL ] = -1;
  count[ SET_T16_2PC_CASTER ] = count[ SET_T16_2PC_MELEE ] = count[ SET_T16_2PC_TANK ] = count[ SET_T16_2PC_HEAL ] = -1;
  count[ SET_T16_4PC_CASTER ] = count[ SET_T16_4PC_MELEE ] = count[ SET_T16_4PC_TANK ] = count[ SET_T16_4PC_HEAL ] = -1;
  count[ SET_PVP_2PC_CASTER ] = count[ SET_PVP_2PC_MELEE ] = count[ SET_PVP_2PC_TANK ] = count[ SET_PVP_2PC_HEAL ] = -1;
  count[ SET_PVP_4PC_CASTER ] = count[ SET_PVP_4PC_MELEE ] = count[ SET_PVP_4PC_TANK ] = count[ SET_PVP_4PC_HEAL ] = -1;
}

// set_bonus_t::tier13 ======================================================

int set_bonus_t::tier13_2pc_caster() { return ( count[ SET_T13_2PC_CASTER ] > 0 || ( count[ SET_T13_2PC_CASTER ] < 0 && count[ SET_T13_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_caster() { return ( count[ SET_T13_4PC_CASTER ] > 0 || ( count[ SET_T13_4PC_CASTER ] < 0 && count[ SET_T13_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_melee() { return ( count[ SET_T13_2PC_MELEE ] > 0 || ( count[ SET_T13_2PC_MELEE ] < 0 && count[ SET_T13_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_melee() { return ( count[ SET_T13_4PC_MELEE ] > 0 || ( count[ SET_T13_4PC_MELEE ] < 0 && count[ SET_T13_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_tank() { return ( count[ SET_T13_2PC_TANK ] > 0 || ( count[ SET_T13_2PC_TANK ] < 0 && count[ SET_T13_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_tank() { return ( count[ SET_T13_4PC_TANK ] > 0 || ( count[ SET_T13_4PC_TANK ] < 0 && count[ SET_T13_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier13_2pc_heal() { return ( count[ SET_T13_2PC_HEAL ] > 0 || ( count[ SET_T13_2PC_HEAL ] < 0 && count[ SET_T13_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier13_4pc_heal() { return ( count[ SET_T13_4PC_HEAL ] > 0 || ( count[ SET_T13_4PC_HEAL ] < 0 && count[ SET_T13_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier14 ======================================================

int set_bonus_t::tier14_2pc_caster() { return ( count[ SET_T14_2PC_CASTER ] > 0 || ( count[ SET_T14_2PC_CASTER ] < 0 && count[ SET_T14_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_caster() { return ( count[ SET_T14_4PC_CASTER ] > 0 || ( count[ SET_T14_4PC_CASTER ] < 0 && count[ SET_T14_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_melee() { return ( count[ SET_T14_2PC_MELEE ] > 0 || ( count[ SET_T14_2PC_MELEE ] < 0 && count[ SET_T14_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_melee() { return ( count[ SET_T14_4PC_MELEE ] > 0 || ( count[ SET_T14_4PC_MELEE ] < 0 && count[ SET_T14_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_tank() { return ( count[ SET_T14_2PC_TANK ] > 0 || ( count[ SET_T14_2PC_TANK ] < 0 && count[ SET_T14_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_tank() { return ( count[ SET_T14_4PC_TANK ] > 0 || ( count[ SET_T14_4PC_TANK ] < 0 && count[ SET_T14_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier14_2pc_heal() { return ( count[ SET_T14_2PC_HEAL ] > 0 || ( count[ SET_T14_2PC_HEAL ] < 0 && count[ SET_T14_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier14_4pc_heal() { return ( count[ SET_T14_4PC_HEAL ] > 0 || ( count[ SET_T14_4PC_HEAL ] < 0 && count[ SET_T14_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier15 ======================================================

int set_bonus_t::tier15_2pc_caster() { return ( count[ SET_T15_2PC_CASTER ] > 0 || ( count[ SET_T15_2PC_CASTER ] < 0 && count[ SET_T15_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier15_4pc_caster() { return ( count[ SET_T15_4PC_CASTER ] > 0 || ( count[ SET_T15_4PC_CASTER ] < 0 && count[ SET_T15_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier15_2pc_melee() { return ( count[ SET_T15_2PC_MELEE ] > 0 || ( count[ SET_T15_2PC_MELEE ] < 0 && count[ SET_T15_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier15_4pc_melee() { return ( count[ SET_T15_4PC_MELEE ] > 0 || ( count[ SET_T15_4PC_MELEE ] < 0 && count[ SET_T15_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier15_2pc_tank() { return ( count[ SET_T15_2PC_TANK ] > 0 || ( count[ SET_T15_2PC_TANK ] < 0 && count[ SET_T15_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier15_4pc_tank() { return ( count[ SET_T15_4PC_TANK ] > 0 || ( count[ SET_T15_4PC_TANK ] < 0 && count[ SET_T15_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier15_2pc_heal() { return ( count[ SET_T15_2PC_HEAL ] > 0 || ( count[ SET_T15_2PC_HEAL ] < 0 && count[ SET_T15_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier15_4pc_heal() { return ( count[ SET_T15_4PC_HEAL ] > 0 || ( count[ SET_T15_4PC_HEAL ] < 0 && count[ SET_T15_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::tier16 ======================================================

int set_bonus_t::tier16_2pc_caster() { return ( count[ SET_T16_2PC_CASTER ] > 0 || ( count[ SET_T16_2PC_CASTER ] < 0 && count[ SET_T16_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier16_4pc_caster() { return ( count[ SET_T16_4PC_CASTER ] > 0 || ( count[ SET_T16_4PC_CASTER ] < 0 && count[ SET_T16_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier16_2pc_melee() { return ( count[ SET_T16_2PC_MELEE ] > 0 || ( count[ SET_T16_2PC_MELEE ] < 0 && count[ SET_T16_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier16_4pc_melee() { return ( count[ SET_T16_4PC_MELEE ] > 0 || ( count[ SET_T16_4PC_MELEE ] < 0 && count[ SET_T16_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier16_2pc_tank() { return ( count[ SET_T16_2PC_TANK ] > 0 || ( count[ SET_T16_2PC_TANK ] < 0 && count[ SET_T16_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier16_4pc_tank() { return ( count[ SET_T16_4PC_TANK ] > 0 || ( count[ SET_T16_4PC_TANK ] < 0 && count[ SET_T16_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::tier16_2pc_heal() { return ( count[ SET_T16_2PC_HEAL ] > 0 || ( count[ SET_T16_2PC_HEAL ] < 0 && count[ SET_T16_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::tier16_4pc_heal() { return ( count[ SET_T16_4PC_HEAL ] > 0 || ( count[ SET_T16_4PC_HEAL ] < 0 && count[ SET_T16_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::pvp =========================================================

int set_bonus_t::pvp_2pc_caster() { return ( count[ SET_PVP_2PC_CASTER ] > 0 || ( count[ SET_PVP_2PC_CASTER ] < 0 && count[ SET_PVP_CASTER ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::pvp_4pc_caster() { return ( count[ SET_PVP_4PC_CASTER ] > 0 || ( count[ SET_PVP_4PC_CASTER ] < 0 && count[ SET_PVP_CASTER ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::pvp_2pc_melee() { return ( count[ SET_PVP_2PC_MELEE ] > 0 || ( count[ SET_PVP_2PC_MELEE ] < 0 && count[ SET_PVP_MELEE ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::pvp_4pc_melee() { return ( count[ SET_PVP_4PC_MELEE ] > 0 || ( count[ SET_PVP_4PC_MELEE ] < 0 && count[ SET_PVP_MELEE ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::pvp_2pc_tank() { return ( count[ SET_PVP_2PC_TANK ] > 0 || ( count[ SET_PVP_2PC_TANK ] < 0 && count[ SET_PVP_TANK ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::pvp_4pc_tank() { return ( count[ SET_PVP_4PC_TANK ] > 0 || ( count[ SET_PVP_4PC_TANK ] < 0 && count[ SET_PVP_TANK ] >= 4 ) ) ? 1 : 0; }

int set_bonus_t::pvp_2pc_heal() { return ( count[ SET_PVP_2PC_HEAL ] > 0 || ( count[ SET_PVP_2PC_HEAL ] < 0 && count[ SET_PVP_HEAL ] >= 2 ) ) ? 1 : 0; }

int set_bonus_t::pvp_4pc_heal() { return ( count[ SET_PVP_4PC_HEAL ] > 0 || ( count[ SET_PVP_4PC_HEAL ] < 0 && count[ SET_PVP_HEAL ] >= 4 ) ) ? 1 : 0; }

// set_bonus_t::decode ======================================================

int set_bonus_t::decode( player_t* p,
                         item_t&   item )
{
  if ( ! item.name() ) return SET_NONE;
  
  if (p -> sim -> challenge_mode) return SET_NONE;
    
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

expr_t* set_bonus_t::create_expression( player_t* player,
                                        const std::string& type )
{
  set_e bonus_type = util::parse_set_bonus( type );

  if ( bonus_type == SET_NONE )
    return 0;

  struct set_bonus_expr_t : public expr_t
  {
    player_t& player;
    set_e set_bonus_type;
    set_bonus_expr_t( player_t& p, set_e t ) :
      expr_t( util::set_bonus_string( t ) ), player( p ), set_bonus_type( t ) {}
    virtual double evaluate()
    { return player.sets -> has_set_bonus( set_bonus_type ); }
  };

  assert( player );
  return new set_bonus_expr_t( *player, bonus_type );
}

inline const spell_data_t* set_bonus_array_t::create_set_bonus( uint32_t spell_id )
{
  if ( ! p -> dbc.spell( spell_id ) )
  {
    if ( spell_id > 0 )
    {
      p -> sim -> errorf( "Set bonus spell identifier %u for %s not found in spell data.",
                          spell_id, p -> name_str.c_str() );
    }
    return spell_data_t::nil();
  }

  return ( p -> dbc.spell( spell_id ) );
}

// set_bonus_array_t::set_bonus_array_t =====================================

set_bonus_array_t::set_bonus_array_t( player_t* p, const uint32_t a_bonus[ N_TIER ][ N_TIER_BONUS ] ) :
  default_value( spell_data_t::nil() ), p( p )
{
  // Map two-dimensional array into correct slots in the one-dimensional set_bonuses
  // array, based on set_e enum
  for ( int tier = 0; tier < N_TIER; ++tier )
  {
    for ( int j = 0; j < N_TIER_BONUS; j++ )
    {
      int b = 3 * j / 2 + 1;
      set_bonuses[ 1 + tier * 12 + b ] = create_set_bonus( a_bonus[ tier ][ j ] );
    }
  }
}

bool set_bonus_array_t::has_set_bonus( set_e s )
{
  if ( p -> set_bonus.count[ s ] > 0 )
    return true;

  if ( p -> set_bonus.count[ s ] < 0 )
  {
    assert( s > 0 );
    int tier  = ( s - 1 ) / 12,
        btype = ( ( s - 1 ) % 12 ) / 3,
        bonus = ( s - 1 ) % 3;

    assert( 1 + tier * 12 + btype * 3 < static_cast<int>( p -> set_bonus.count.size() ) );
    if ( p -> set_bonus.count[ 1 + tier * 12 + btype * 3 ] >= bonus * 2 )
      return true;
  }

  return false;
}

const spell_data_t* set_bonus_array_t::set( set_e s )
{
  if ( has_set_bonus( s ) && set_bonuses[ s ] )
    return set_bonuses[ s ];

  return default_value;
}

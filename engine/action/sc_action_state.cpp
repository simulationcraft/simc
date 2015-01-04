// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

action_state_t* action_t::get_state( const action_state_t* other )
{
  action_state_t* s = 0;

  if ( state_cache )
  {
    s = state_cache;
    state_cache = s -> next;
  }
  else
    s = new_state();

  s -> action = this;
  if ( ! other )
    s -> initialize();
  else
    s -> copy_state( other );

  return s;
}

action_state_t* action_t::new_state()
{
  return new action_state_t( this, target );
}

void action_t::release_state( action_state_t* s )
{
  assert( s -> action == this );
  s -> next = state_cache;
  state_cache = s;
}

// Initialize contains all variables that must be reset every time a new
// state object is retrieved using get_state()
void action_state_t::initialize()
{
  result = RESULT_NONE; result_type = RESULT_TYPE_NONE;
  block_result = BLOCK_RESULT_UNBLOCKED;
  result_raw = result_total = result_mitigated = result_absorbed = result_amount = blocked_amount = self_absorb_amount =  0;
}
/*
void action_state_t::copy_state( const action_state_t* o )
{
  if ( this == o || o == 0 ) return;
  *this = *o;
}
*/
void action_state_t::copy_state( const action_state_t* o )
{
#ifndef NDEBUG
  assert( o );

  if ( typeid( this ) != typeid( const_cast<action_state_t*>( o ) ) )
  {
    std::cout << "action_state_t::operator=: state runtime types not equal! this= " << typeid( this ).name() << " o= " << typeid( const_cast<action_state_t*>( o ) ).name() << "\n";
    assert( 0 );
  }
#endif

  target = o -> target; assert( target );
  n_targets = o -> n_targets;
  chain_target = o -> chain_target;
  result_type = o -> result_type;
  result = o -> result;
  result_raw = o -> result_raw;
  result_total = o -> result_total;
  result_mitigated = o -> result_mitigated;
  result_absorbed = o -> result_absorbed;
  result_amount = o -> result_amount;
  blocked_amount = o -> blocked_amount;
  self_absorb_amount = o -> self_absorb_amount;
  haste = o -> haste;
  crit = o -> crit;
  target_crit = o -> target_crit;
  attack_power = o -> attack_power;
  spell_power = o -> spell_power;

  versatility = o -> versatility;
  resolve = o -> resolve;
  da_multiplier = o -> da_multiplier;
  ta_multiplier = o -> ta_multiplier;
  persistent_multiplier = o -> persistent_multiplier;

  target_da_multiplier = o -> target_da_multiplier;
  target_ta_multiplier = o -> target_ta_multiplier;

  target_mitigation_da_multiplier = o -> target_mitigation_da_multiplier;
  target_mitigation_ta_multiplier = o -> target_mitigation_ta_multiplier;
}

action_state_t::action_state_t( action_t* a, player_t* t ) :
  next( 0 ), action( a ), target( t ),
  n_targets( 0 ), chain_target( 0 ),
  result_type( RESULT_TYPE_NONE ), result( RESULT_NONE ), block_result( BLOCK_RESULT_UNKNOWN ),
  result_raw( 0 ), result_total( 0 ), result_mitigated( 0 ),
  result_absorbed( 0 ), result_amount( 0 ), blocked_amount( 0 ), self_absorb_amount( 0 ),
  haste( 0 ), crit( 0 ), target_crit( 0 ),
  attack_power( 0 ), spell_power( 0 ),
  resolve( 1.0 ), versatility( 1.0 ), da_multiplier( 1.0 ), ta_multiplier( 1.0 ), persistent_multiplier( 1.0 ),
  target_da_multiplier( 1.0 ), target_ta_multiplier( 1.0 ),
  target_mitigation_da_multiplier( 1.0 ), target_mitigation_ta_multiplier( 1.0 )
{
  assert( target );
}

std::ostringstream& action_state_t::debug_str( std::ostringstream& s )
{
  s << std::showbase;
  std::streamsize ss = s.precision();

  s << action -> player -> name() << " " << action -> name() << " " << target -> name() << ":";

  s << std::hex;

  s << " snapshot_flags=" << action -> snapshot_flags;
  s << " update_flags=" << action -> update_flags;
  s << " result=" << util::result_type_string( result );
  s << " type=" << util::amount_type_string( result_type );

  s << " proc_type=" << util::proc_type_string( proc_type() );
  s << " exec_proc_type=" << util::proc_type2_string( execute_proc_type2() );
  s << " impact_proc_type=" << util::proc_type2_string( impact_proc_type2() );

  s << std::dec;

  s << " n_targets=" << n_targets;
  s << " chain_target=" << chain_target;

  s << " raw_amount=" << result_raw;
  s << " total_amount=" << result_total;
  s << " mitigated_amount=" << result_mitigated;
  s << " absorbed_amount=" << result_absorbed;
  s << " actual_amount=" << result_amount;
  s << " only_blocked_damage=" << blocked_amount;
  s << " self_absorbed_damage=" << self_absorb_amount;
  s << " ap=" << attack_power;
  s << " sp=" << spell_power;

  s.precision( 4 );

  s << " haste=" << haste;
  s << " crit=" << crit;
  s << " tgt_crit=" << target_crit;
  s << " versatility=" << versatility;
  s << " da_mul=" << da_multiplier;
  s << " ta_mul=" << ta_multiplier;
  s << " per_mul=" << persistent_multiplier;
  s << " tgt_da_mul=" << target_da_multiplier;
  s << " tgt_ta_mul=" << target_ta_multiplier;
  s << " resolve=" << resolve;

  s << " tgt_mitg_da_mul=" << target_mitigation_da_multiplier;
  s << " tgt_mitg_ta_mul=" << target_mitigation_ta_multiplier;

  s.precision( ss );

  return s;
}

void action_state_t::debug()
{
  std::ostringstream s;
  action -> sim -> out_debug.printf( "%s", debug_str( s ).str().c_str() );
}

travel_event_t::travel_event_t( action_t* a,
                                action_state_t* state,
                                timespan_t time_to_travel ) :
  event_t( *a -> player, "Stateless Action Travel" ), action( a ), state( state )
{
  if ( sim().debug )
    sim().out_debug.printf( "New Stateless Action Travel Event: %s %s %.2f",
                p() -> name(), a -> name(), time_to_travel.total_seconds() );

  sim().add_event( this, time_to_travel );
}

void travel_event_t::execute()
{
  if ( ! state -> target -> is_sleeping() )
    action -> impact( state );
  action_state_t::release( state );
  action -> remove_travel_event( this );
}

void action_state_t::release( action_state_t*& s )
{ s -> action -> release_state( s ); s = 0; }

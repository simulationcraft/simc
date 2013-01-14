// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

action_state_t* action_t::get_state( const action_state_t* other )
{
  action_state_t* s = 0;

  if ( state_cache.empty() )
  {
    s = new_state();
  }
  else
  {
    s = state_cache.back();
    state_cache.pop_back();
  }

  s -> copy_state( other );
  s -> action = this;
  s -> result = RESULT_NONE;
  s -> result_amount = 0;

  return s;
}

action_state_t* action_t::new_state()
{
  return new action_state_t( this, target );
}

void action_t::release_state( action_state_t* s )
{
  state_cache.push_back( s );
}

void action_state_t::copy_state( const action_state_t* o )
{
  if ( this == o || o == 0 ) return;
#ifndef NDEBUG
  if ( typeid( this ) != typeid( const_cast<action_state_t*>( o ) ) )
  {
    std::cout << "action_state_t::copy_state: state runtime types not equal! this= " << typeid( this ).name() << " o= " << typeid( const_cast<action_state_t*>( o ) ).name() << "\n";
    assert( 0 );
  }
#endif

  target = o -> target; assert( target );
  result_type = o -> result_type; result = o -> result; result_amount = o -> result_amount;
  haste = o -> haste;
  crit = o -> crit;
  target_crit = o -> target_crit;
  attack_power = o -> attack_power;
  spell_power = o -> spell_power;

  da_multiplier = o -> da_multiplier;
  ta_multiplier = o -> ta_multiplier;

  target_da_multiplier = o -> target_da_multiplier;
  target_ta_multiplier = o -> target_ta_multiplier;
}

action_state_t::action_state_t( action_t* a, player_t* t ) :
  action( a ), target( t ),
  result_type( RESULT_TYPE_NONE ), result( RESULT_NONE ), result_amount( 0 ),
  haste( 0 ), crit( 0 ), target_crit( 0 ),
  attack_power( 0 ), spell_power( 0 ),
  da_multiplier( 1.0 ), ta_multiplier( 1.0 ),
  target_da_multiplier( 1.0 ), target_ta_multiplier( 1.0 )
{
  assert( target );
}

void action_state_t::debug()
{
  action -> sim -> output( "[NEW] %s %s %s: obj=%p snapshot_flags=%#.4x update_flags=%#.4x result=%s result_type=%s amount=%.2f "
                           "haste=%.2f crit=%.2f tgt_crit=%.2f "
                           "ap=%.0f sp=%.0f "
                           "da_mul=%.4f ta_mul=%.4f tgt_da_mul=%.4f tgt_ta_mul=%.4f",
                           action -> player -> name(),
                           action -> name(),
                           target -> name(),
                           this,
                           action -> snapshot_flags,
                           action -> update_flags,
                           util::result_type_string( result ),
                           util::amount_type_string( result_type ), result_amount,
                           haste, crit, target_crit,
                           attack_power, spell_power,
                           da_multiplier, ta_multiplier,
                           target_da_multiplier, target_ta_multiplier );
}

travel_event_t::travel_event_t( action_t* a,
                                action_state_t* state,
                                timespan_t time_to_travel ) :
  event_t( a -> player, "Stateless Action Travel" ), action( a ), state( state )
{
  if ( sim.debug )
    sim.output( "New Stateless Action Travel Event: %s %s %.2f",
                player -> name(), a -> name(), time_to_travel.total_seconds() );

  sim.add_event( this, time_to_travel );
}

void travel_event_t::execute()
{
  if ( ! state -> target -> current.sleeping )
    action -> impact( state );
  action_state_t::release( state );
  action -> remove_travel_event( this );
}

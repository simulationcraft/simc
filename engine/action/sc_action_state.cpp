// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action/sc_action_state.hpp"
#include "action/sc_action.hpp"
#include "player/sc_player.hpp"
#include <sstream>

action_state_t* action_t::get_state( const action_state_t* other )
{
  action_state_t* s = nullptr;

  if ( state_cache )
  {
    s           = state_cache;
    state_cache = s->next;
  }
  else
  {
    s = new_state();
  }

  s->action = this;
  if ( !other )
  {
    s->initialize();
  }
  else
  {
    s->copy_state( other );
  }

  return s;
}

action_state_t* action_t::new_state()
{
  return new action_state_t( this, target );
}

void action_t::release_state( action_state_t* s )
{
  assert( s->action == this );
  s->next     = state_cache;
  state_cache = s;
}

// Initialize contains all variables that must be reset every time a new
// state object is retrieved using get_state()
void action_state_t::initialize()
{
  result       = RESULT_NONE;
  result_type  = result_amount_type::NONE;
  block_result = BLOCK_RESULT_UNBLOCKED;
  result_raw = result_total = result_mitigated = result_absorbed =
      result_amount = blocked_amount = self_absorb_amount = 0;
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
    throw std::runtime_error( fmt::format("action_state_t::operator=: state runtime types not equal! this={} o={}\n",
        typeid( this ).name(), typeid( const_cast<action_state_t*>( o ) ).name() ) );
  }
#endif

  target = o->target;
  assert( target );
  n_targets          = o->n_targets;
  chain_target       = o->chain_target;
  original_x         = o->original_x;
  original_y         = o->original_y;
  result_type        = o->result_type;
  result             = o->result;
  result_raw         = o->result_raw;
  result_total       = o->result_total;
  result_mitigated   = o->result_mitigated;
  result_absorbed    = o->result_absorbed;
  result_crit_bonus  = o->result_crit_bonus;
  result_amount      = o->result_amount;
  blocked_amount     = o->blocked_amount;
  self_absorb_amount = o->self_absorb_amount;
  haste              = o->haste;
  crit_chance        = o->crit_chance;
  target_crit_chance = o->target_crit_chance;
  attack_power       = o->attack_power;
  spell_power        = o->spell_power;

  versatility           = o->versatility;
  da_multiplier         = o->da_multiplier;
  ta_multiplier         = o->ta_multiplier;
  persistent_multiplier = o->persistent_multiplier;
  pet_multiplier        = o->pet_multiplier;

  target_da_multiplier = o->target_da_multiplier;
  target_ta_multiplier = o->target_ta_multiplier;

  target_mitigation_da_multiplier = o->target_mitigation_da_multiplier;
  target_mitigation_ta_multiplier = o->target_mitigation_ta_multiplier;
  target_armor                    = o->target_armor;
}

action_state_t::action_state_t( action_t* a, player_t* t )
  : next( nullptr ),
    action( a ),
    target( t ),
    n_targets( 0 ),
    chain_target( 0 ),
    original_x( 0 ),
    original_y( 0 ),
    result_type( result_amount_type::NONE ),
    result( RESULT_NONE ),
    block_result( BLOCK_RESULT_UNKNOWN ),
    result_raw( 0 ),
    result_total( 0 ),
    result_mitigated( 0 ),
    result_absorbed( 0 ),
    result_crit_bonus( 0 ),
    result_amount( 0 ),
    blocked_amount( 0 ),
    self_absorb_amount( 0 ),
    haste( 1.0 ),
    crit_chance( 0 ),
    target_crit_chance( 0 ),
    attack_power( 0 ),
    spell_power( 0 ),
    versatility( 1.0 ),
    da_multiplier( 1.0 ),
    ta_multiplier( 1.0 ),
    persistent_multiplier( 1.0 ),
    pet_multiplier( 1.0 ),
    target_da_multiplier( 1.0 ),
    target_ta_multiplier( 1.0 ),
    target_mitigation_da_multiplier( 1.0 ),
    target_mitigation_ta_multiplier( 1.0 ),
    target_armor( 0 )
{
  assert( target );
}

std::ostringstream& action_state_t::debug_str( std::ostringstream& s )
{
  s << std::showbase;
  std::streamsize ss = s.precision();

  s << action->player->name() << " " << action->name() << " " << target->name()
    << ":";

  s << std::hex;

  s << " snapshot_flags=";
  if ( action->snapshot_flags > 0 )
  {
    s << "{ " << flags_to_str( action->snapshot_flags ) << " }";
  }
  else
  {
    s << action->snapshot_flags;
  }
  s << " update_flags=";
  if ( action->update_flags > 0 )
  {
    s << "{ " << flags_to_str( action->update_flags ) << " }";
  }
  else
  {
    s << action->update_flags;
  }

  s << " result=" << util::result_type_string( result );
  s << " type=" << util::amount_type_string( result_type );

  s << " proc_type=" << util::proc_type_string( proc_type() );
  s << " exec_proc_type=" << util::proc_type2_string( execute_proc_type2() );
  s << " impact_proc_type=" << util::proc_type2_string( impact_proc_type2() );
  s << " interrupt_proc_type=" << util::proc_type2_string( interrupt_proc_type2() );

  s << std::dec;

  s << " n_targets=" << n_targets;
  s << " chain_target=" << chain_target;
  s << " original_x=" << original_x;
  s << " original_y=" << original_y;

  s << " raw_amount=" << result_raw;
  s << " total_amount=" << result_total;
  s << " mitigated_amount=" << result_mitigated;
  s << " absorbed_amount=" << result_absorbed;
  s << " crit_bonus=" << result_crit_bonus;
  s << " actual_amount=" << result_amount;
  s << " only_blocked_damage=" << blocked_amount;
  s << " self_absorbed_damage=" << self_absorb_amount;
  s << " ap=" << attack_power;
  s << " sp=" << spell_power;

  s.precision( 4 );

  s << " haste=" << haste;
  s << " crit=" << crit_chance;
  s << " tgt_crit=" << target_crit_chance;
  s << " versatility=" << versatility;
  s << " da_mul=" << da_multiplier;
  s << " ta_mul=" << ta_multiplier;
  s << " per_mul=" << persistent_multiplier;
  if ( action->player->is_pet() )
  {
    s << " pet_mul=" << pet_multiplier;
  }
  s << " tgt_da_mul=" << target_da_multiplier;
  s << " tgt_ta_mul=" << target_ta_multiplier;

  s << " tgt_mitg_da_mul=" << target_mitigation_da_multiplier;
  s << " tgt_mitg_ta_mul=" << target_mitigation_ta_multiplier;
  s << " target_armor=" << target_armor;

  s.precision( ss );

  return s;
}

void action_state_t::debug()
{
  std::ostringstream s;
  action->sim->out_debug.print( "{}", debug_str( s ).str() );
}

travel_event_t::travel_event_t( action_t* a, action_state_t* state,
                                timespan_t time_to_travel )
  : event_t( *a->player, time_to_travel ), action( a ), state( state )
{
  sim().print_debug( "New Stateless Action Travel Event: {} {} {}", *a->player, *a, time_to_travel );
}

travel_event_t::~travel_event_t()
{
  if (state && canceled) action_state_t::release(state);
}

void travel_event_t::execute()
{
  if ( !state->target->is_sleeping() )
  {
    action->impact( state );
  }

  action_state_t::release( state );
  action->remove_travel_event( this );
}

void action_state_t::release( action_state_t*& s )
{
  assert( s );
  s->action->release_state( s );
  s = nullptr;
}

std::string action_state_t::flags_to_str( unsigned flags )
{
  std::string str;

  auto concat_flag_str = [flags]( std::string& str, const char* flag_str,
                                  snapshot_state_e state ) {
    if ( flags & state )
    {
      if ( !str.empty() )
      {
        str += "|";
      }

      str += flag_str;
    }
  };

  concat_flag_str( str, "AP", STATE_AP );
  concat_flag_str( str, "SP", STATE_SP );
  concat_flag_str( str, "HST", STATE_HASTE );
  concat_flag_str( str, "CRIT", STATE_CRIT );
  concat_flag_str( str, "VERS", STATE_VERSATILITY );
  concat_flag_str( str, "MUL_DA", STATE_MUL_DA );
  concat_flag_str( str, "MUL_TA", STATE_MUL_TA );
  concat_flag_str( str, "MUL_PER", STATE_MUL_PERSISTENT );
  concat_flag_str( str, "MUL_PET", STATE_MUL_PET );

  concat_flag_str( str, "TGT_CRIT", STATE_TGT_CRIT );
  concat_flag_str( str, "TGT_MUL_DA", STATE_TGT_MUL_DA );
  concat_flag_str( str, "TGT_MUL_TA", STATE_TGT_MUL_TA );

  concat_flag_str( str, "USR1", STATE_USER_1 );
  concat_flag_str( str, "USR2", STATE_USER_2 );
  concat_flag_str( str, "USR3", STATE_USER_3 );
  concat_flag_str( str, "USR4", STATE_USER_4 );

  concat_flag_str( str, "TGT_MIT_DA", STATE_TGT_MITG_DA );
  concat_flag_str( str, "TGT_MIT_TA", STATE_TGT_MITG_TA );
  concat_flag_str( str, "TGT_ARMOR", STATE_TGT_ARMOR );

  return str;
}

// Primary proc type of the result (direct (aoe) damage/heal, periodic
// damage/heal)
proc_types action_state_t::proc_type() const
{
  if (result_type == result_amount_type::DMG_DIRECT || result_type == result_amount_type::HEAL_DIRECT)
    return action->proc_type();
  else if (result_type == result_amount_type::DMG_OVER_TIME)
    return PROC1_PERIODIC;
  else if (result_type == result_amount_type::HEAL_OVER_TIME)
    return PROC1_PERIODIC_HEAL;

  return PROC1_INVALID;
}

// Secondary proc type of the "finished casting" (i.e., execute()). Only
// triggers the "landing", dodge, parry, and miss procs
proc_types2 action_state_t::execute_proc_type2() const
{
  if (result == RESULT_DODGE)
    return PROC2_DODGE;
  else if (result == RESULT_PARRY)
    return PROC2_PARRY;
  else if (result == RESULT_MISS)
    return PROC2_MISS;
  // Bunch up all non-damaging harmful attacks that land into "hit"
  else if (action->harmful)
    return PROC2_LANDED;

  return PROC2_INVALID;
}

proc_types2 action_state_t::cast_proc_type2() const
{
  // Only foreground actions may trigger the "on cast" procs
  if (action->background)
  {
    return PROC2_INVALID;
  }

  if (action->attack_direct_power_coefficient(this) ||
    action->attack_tick_power_coefficient(this) ||
    action->spell_direct_power_coefficient(this) ||
    action->spell_tick_power_coefficient(this) ||
    action->base_ta(this) || action->base_da_min(this) ||
    action->bonus_ta(this) || action->bonus_da(this))
  {
    // This is somewhat naive way to differentiate, better way would be to classify based on the
    // actual proc types, but it will serve our purposes for now.
    return action->harmful ? PROC2_CAST_DAMAGE : PROC2_CAST_HEAL;
  }

  // Generic fallback "on any cast"
  return PROC2_CAST;
}

proc_types2 action_state_t::interrupt_proc_type2() const
{
  if ( action->is_interrupt )
    return PROC2_CAST_INTERRUPT;
  else
    return PROC2_INVALID;
}

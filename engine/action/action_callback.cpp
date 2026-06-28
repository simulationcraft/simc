// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action_callback.hpp"

#include "action.hpp"
#include "player/player.hpp"

proc_data_t::proc_data_t()
  : spell( spell_data_t::nil() ),
    suppress_caster_procs( false ),
    enable_proc_from_suppressed( false ),
    can_proc_from_suppressed( false ),
    suppress_target_procs( false ),
    can_proc_from_suppressed_target( false ),
    allow_class_ability_procs( false ),
    can_only_proc_from_class_abilities( false ),
    can_proc_from_procs( false )
{}

proc_data_t::proc_data_t( const spell_data_t* s_data ) : spell( s_data ? s_data : spell_data_t::nil() )
{
  _init();
}

void proc_data_t::_init()
{
  suppress_caster_procs = spell->flags( spell_attribute::SX_SUPPRESS_CASTER_PROCS );
  enable_proc_from_suppressed = spell->flags( spell_attribute::SX_ENABLE_PROCS_FROM_SUPPRESSED );
  can_proc_from_suppressed = spell->flags( spell_attribute::SX_CAN_PROC_FROM_SUPPRESSED );
  suppress_target_procs = spell->flags( spell_attribute::SX_SUPPRESS_TARGET_PROCS );
  can_proc_from_suppressed_target = spell->flags( spell_attribute::SX_CAN_PROC_FROM_SUPPRESSED_TGT );
  allow_class_ability_procs = spell->flags( spell_attribute::SX_ALLOW_CLASS_ABILITY_PROCS );
  can_only_proc_from_class_abilities = spell->flags( spell_attribute::SX_ONLY_PROC_FROM_CLASS_ABILITIES );
  can_proc_from_procs = spell->flags( spell_attribute::SX_CAN_PROC_FROM_PROCS );
}

bool proc_data_t::check_proc_trigger( const proc_data_t& source, const proc_data_t& target, proc_trigger_type_e type )
{
  if ( target.can_only_proc_from_class_abilities && !source.allow_class_ability_procs )
  {
    return false;
  }

  if ( ( type == proc_trigger_type_e::TRIGGER_ACTION_PROC || type == proc_trigger_type_e::TRIGGER_ACTION_PROC_TAKEN ) &&
       !target.can_proc_from_procs )
  {
    return false;
  }

  if ( type == proc_trigger_type_e::TRIGGER_ACTION_TAKEN || type == proc_trigger_type_e::TRIGGER_ACTION_PROC_TAKEN )
  {
    // TODO: is there a target equivalent of enable_proc_from_suppressed?
    if ( source.suppress_target_procs && !target.can_proc_from_suppressed_target )
      return false;
  }
  else
  {
    // both enable_proc_from_suppressed and can_proc_from_suppressed are needed to overcome suppress_caster_procs
    if ( source.suppress_caster_procs && ( !source.enable_proc_from_suppressed || !target.can_proc_from_suppressed ) )
      return false;
  }

  return true;
}

action_callback_t::action_callback_t( player_t* l )
  : listener( l ), active( true ), allow_self_procs( false ), allow_pet_procs( false )
{
  assert( l );
  if ( range::find( l->callbacks.all_callbacks, this ) == l->callbacks.all_callbacks.end() )
    l->callbacks.all_callbacks.push_back( this );
}

void action_callback_t::trigger( const std::vector<action_callback_t*>& v, const proc_data_t& data, player_t* player,
                                 player_t* target, action_state_t* state, proc_trigger_type_e type )
{
  if ( !player->in_combat )
    return;

  for ( auto cb : v )
    if ( cb->active )
      cb->trigger( data, target, state, type );
}

void action_callback_t::reset( const std::vector<action_callback_t*>& v )
{
  for ( auto cb : v )
    cb->reset();
}

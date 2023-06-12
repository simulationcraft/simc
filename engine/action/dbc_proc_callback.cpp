// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc_proc_callback.hpp"

#include <cassert>

#include "buff/buff.hpp"
#include "item/item.hpp"
#include "item/special_effect.hpp"
#include "player/player.hpp"
#include "action.hpp"
#include "action_state.hpp"
#include "action_callback.hpp"
#include "sim/event.hpp"
#include "sim/real_ppm.hpp"
#include "sim/cooldown.hpp"
#include "sim/sim.hpp"
#include "util/rng.hpp"

struct proc_event_t : public event_t
{
  dbc_proc_callback_t* cb;
  action_t* source_action;
  action_state_t* source_state;

  proc_event_t( dbc_proc_callback_t* c, action_t* a, action_state_t* s )
    : event_t( *a->sim ),
      cb( c ),
      source_action( a ),
      // Note, state has to be cloned as it's about to get recycled back into the action state cache
      source_state( s->action->get_state( s ) )
  {
    schedule( timespan_t::zero() );
  }

  ~proc_event_t() override
  {
    action_state_t::release( source_state );
  }

  const char* name() const override
  {
    return "dbc_proc_event";
  }

  void execute() override
  {
    cb->execute( source_action, source_state );
  }
};

const item_t dbc_proc_callback_t::default_item_ = item_t();

cooldown_t* dbc_proc_callback_t::get_cooldown( player_t* target )
{
  if ( !target_specific_cooldown || !target )
    return cooldown;

  return target_specific_cooldown->get_cooldown( target );
}

void dbc_proc_callback_t::trigger( action_t* a, action_state_t* state )
{
  cooldown_t* cd = get_cooldown( state->target );

  // Fully overridden trigger condition check; if allowed, perform normal proc chance
  // behavior.
  if ( trigger_type == trigger_fn_type::TRIGGER )
  {
    if ( !(*trigger_fn)( this, a, state ) )
    {
      return;
    }
  }
  else
  {
    if ( cd && cd->down() )
      return;

    // Weapon-based proc triggering differs from "old" callbacks. When used
    // (weapon_proc == true), dbc_proc_callback_t _REQUIRES_ that the action
    // has the correct weapon specified. Old style procs allowed actions
    // without any weapon to pass through.
    if ( weapon && ( !a->weapon || ( a->weapon && a->weapon != weapon ) ) )
      return;

    // Don't allow procs to proc itself
    if ( proc_action && state->action && state->action->internal_id == proc_action->internal_id )
    {
      return;
    }

    if ( proc_action && proc_action->harmful )
    {
      // Don't allow players to harm other players, and enemies harm other enemies
      if ( state->action && state->action->player->is_enemy() == state->target->is_enemy() )
      {
        return;
      }
    }

    if ( can_only_proc_from_class_abilites )
    {
      if ( !a->allow_class_ability_procs )
        return;
    }

    if ( !can_proc_from_procs )
    {
      if ( !a->not_a_proc && ( a->background || a->proc ) )
      {
        // only direct damage obeys proc-related attributes
        if ( state->proc_type() != PROC1_PERIODIC && state->proc_type() != PROC1_HELPFUL_PERIODIC )
          return;
      }
    }

    // Additional trigger condition to check before performing proc chance process.
    if ( trigger_type == trigger_fn_type::CONDITION && !(*trigger_fn)( this, a, state ) )
    {
      return;
    }
  }

  bool triggered = roll( a );

  if ( listener->sim->debug )
  {
    listener->sim->print_debug( "{} attempts to proc {} on {}: {:d}", listener->name(),
        effect, a->name(), triggered );
  }

  if ( triggered )
  {
    assert( state && state -> action);
    // Detach proc execution from proc triggering
    make_event<proc_event_t>( *listener->sim, this, a, state );

    if ( cd )
      cd->start();
  }
}

dbc_proc_callback_t::dbc_proc_callback_t( const item_t& i, const special_effect_t& e )
  : action_callback_t( i.player ),
    item( i ),
    effect( e ),
    cooldown( nullptr ),
    target_specific_cooldown( nullptr ),
    rppm( nullptr ),
    proc_chance( 0 ),
    ppm( 0 ),
    proc_buff( nullptr ),
    proc_action( nullptr ),
    weapon( nullptr ),
    expire_on_max_stack( false ),
    trigger_type( trigger_fn_type::NONE ),
    trigger_fn( nullptr ),
    execute_fn( nullptr ),
    can_only_proc_from_class_abilites( false ),
    can_proc_from_procs( false )
{
  assert( e.proc_flags() != 0 );
}

dbc_proc_callback_t::dbc_proc_callback_t( const item_t* i, const special_effect_t& e )
  : action_callback_t( i->player ),
    item( *i ),
    effect( e ),
    cooldown( nullptr ),
    target_specific_cooldown( nullptr ),
    rppm( nullptr ),
    proc_chance( 0 ),
    ppm( 0 ),
    proc_buff( nullptr ),
    proc_action( nullptr ),
    weapon( nullptr ),
    expire_on_max_stack( false ),
    trigger_type( trigger_fn_type::NONE ),
    trigger_fn( nullptr ),
    execute_fn( nullptr ),
    can_only_proc_from_class_abilites( false ),
    can_proc_from_procs( false )
{
  assert( e.proc_flags() != 0 );
}

dbc_proc_callback_t::dbc_proc_callback_t( player_t* p, const special_effect_t& e )
  : action_callback_t( p ),
    item( default_item_ ),
    effect( e ),
    cooldown( nullptr ),
    target_specific_cooldown( nullptr ),
    rppm( nullptr ),
    proc_chance( 0 ),
    ppm( 0 ),
    proc_buff( nullptr ),
    proc_action( nullptr ),
    weapon( nullptr ),
    expire_on_max_stack( false ),
    trigger_type( trigger_fn_type::NONE ),
    trigger_fn( nullptr ),
    execute_fn( nullptr ),
    can_only_proc_from_class_abilites( false ),
    can_proc_from_procs( false )
{
  assert( e.proc_flags() != 0 );
}

/**
 * Initialize the proc callback. This method is called by each actor through
 * player_t::register_callbacks(), which is invoked as the last thing in the
 * actor initialization process.
 */
void dbc_proc_callback_t::initialize()
{
  listener->sim->print_debug( "Initializing proc: {}", effect );

  // Initialize proc chance triggers. Note that this code only chooses one, and
  // prioritizes RPPM > PPM > proc chance.
  if ( effect.rppm() > 0 && effect.rppm_scale() != RPPM_DISABLE )
  {
    rppm = listener->get_rppm( effect.name(), effect.rppm(), effect.rppm_modifier(), effect.rppm_scale() );
    rppm->set_blp_state( static_cast<real_ppm_t::blp>( effect.rppm_blp_ ) );
  }
  else if ( effect.ppm() > 0 )
    ppm = effect.ppm();
  else if ( effect.proc_chance() != 0 )
    proc_chance = effect.proc_chance();

  // Initialize cooldown, if applicable
  if ( effect.cooldown() > timespan_t::zero() )
  {
    cooldown                     = listener->get_cooldown( effect.cooldown_name() );
    cooldown->duration           = effect.cooldown();
    if ( effect.has_target_specific_cooldown() )
    {
      target_specific_cooldown = listener->get_target_specific_cooldown( *cooldown );
    }
  }

  // Initialize proc action
  proc_action = effect.create_action();

  // Initialize the potential proc buff through special_effect_t. Can return 0,
  // in which case the proc does not trigger a buff.
  proc_buff = effect.create_buff();

  if ( effect.weapon_proc && effect.item )
  {
    weapon = effect.item->weapon();
  }

  if ( proc_buff && effect.expire_on_max_stack != -1 )
  {
    expire_on_max_stack = as<bool>( effect.expire_on_max_stack );
  }
  else if ( proc_buff && proc_buff->max_stack() > 1 )
  {
    expire_on_max_stack = true;
  }

  // Register callback to new proc system
  listener->callbacks.register_callback( effect.proc_flags(), effect.proc_flags2(), this );

  // Get custom trigger function if it exists
  if ( effect.driver()->id() && trigger_type == trigger_fn_type::NONE )
  {
    trigger_fn = listener->callbacks.callback_trigger_function( effect.driver()->id() );
    trigger_type = listener->callbacks.callback_trigger_function_type( effect.driver()->id() );
  }

  // Get custom execute function if it exists
  if ( effect.driver()->id() && execute_fn == nullptr )
  {
    execute_fn = listener->callbacks.callback_execute_function( effect.driver()->id() );
  }

  can_only_proc_from_class_abilites = effect.can_only_proc_from_class_abilites();
  can_proc_from_procs = effect.can_proc_from_procs();
}

// Determine target for the callback (action).

player_t* dbc_proc_callback_t::target( const action_state_t* state ) const
{
  // Incoming event to the callback actor, or outgoing
  bool incoming = state->target == listener;

  // Outgoing callbacks always target the target of the state object
  if ( !incoming )
  {
    return state->target;
  }

  // Incoming callbacks target either the callback actor, or the source of the incoming state.
  // Which is selected depends on the type of the callback proc action.
  //
  // Technically, this information is exposed in the client data, but simc needs a proper
  // targeting system first before we start using it.
  switch ( proc_action->type )
  {
      // Heals are always targeted to the callback actor on incoming events
    case ACTION_ABSORB:
    case ACTION_HEAL:
      return listener;
      // The rest are targeted to the source of the callback event
    default:
      return state->action->player;
  }
}

rng::rng_t& dbc_proc_callback_t::rng() const
{
  return listener->rng();
}

bool dbc_proc_callback_t::roll( action_t* action )
{
  if ( rppm )
    return rppm->trigger();
  else if ( ppm > 0 )
    return rng().roll( action->ppm_proc_chance( ppm ) );
  else if ( proc_chance > 0 )
    return rng().roll( proc_chance );

  assert( false );
  return false;
}

/**
 * Base rules for proc execution.
 * 1) If we proc a buff, trigger it
 * 2a) If the buff triggered and is at max stack, and we have an action,
 *     execute the action on the target of the action that triggered this
 *     proc.
 * 2b) If we have no buff, trigger the action on the target of the action
 *     that triggered this proc.
 *
 * TODO: Ticking buffs, though that'd be better served by fusing tick_buff_t
 * into buff_t.
 * TODO: Proc delay
 * TODO: Buff cooldown hackery for expressions. Is this really needed or can
 * we do it in a smarter way (better expression support?)
 */

void dbc_proc_callback_t::execute( action_t* action, action_state_t* state )
{
  if ( state->target->is_sleeping() )
  {
    return;
  }

  if ( execute_fn )
  {
    (*execute_fn)( this, action, state );
  }
  else
  {
    bool triggered = proc_buff == nullptr;
    if ( proc_buff )
      triggered = proc_buff->trigger();

    if ( triggered && proc_action && ( !proc_buff || proc_buff->check() == proc_buff->max_stack() ) )
    {
      // Snapshot a new state for schedule_execute() as AoE-triggered procs may require different targets
      proc_action->set_target( target( state ) );
      auto proc_state = proc_action->get_state();
      proc_state->target = proc_action->target;
      proc_action->snapshot_state( proc_state, proc_action->amount_type( proc_state ) );
      proc_action->schedule_execute( proc_state );

      // Decide whether to expire the buff even with 1 max stack
      if ( expire_on_max_stack )
      {
        assert(proc_buff);
        proc_buff->expire();
      }
    }
  }
}

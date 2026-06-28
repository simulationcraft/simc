// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc_proc_callback.hpp"

#include "action.hpp"
#include "action_callback.hpp"
#include "action_state.hpp"
#include "buff/buff.hpp"
#include "item/item.hpp"
#include "item/special_effect.hpp"
#include "player/player.hpp"
#include "sim/cooldown.hpp"
#include "sim/event.hpp"
#include "sim/proc_rng.hpp"
#include "sim/sim.hpp"
#include "util/rng.hpp"

#include <cassert>

struct proc_event_t : public event_t
{
  dbc_proc_callback_t* cb;
  const spell_data_t* spell;
  player_t* target; 
  action_state_t* source_state;
#ifndef NDEBUG
  std::string debug_str;
#endif

  proc_event_t( dbc_proc_callback_t* c, const spell_data_t* spell, player_t* target, action_state_t* state,
                [[maybe_unused]] proc_trigger_type_e type )
    : event_t( *c->listener->sim ),
      cb( c ),
      spell( spell ),
      target( target ),
      // Note, state has to be cloned as it's about to get recycled back into the action state cache
      source_state( state ? state->action->get_state( state ) : nullptr )
  {
    schedule( timespan_t::zero() );
#ifndef NDEBUG
    std::string _name_str;
    if ( !cb )
      _name_str = name();
    else if ( !cb->effect.name_str.empty() )
      _name_str = cb->effect.name_str;
    else
    {
      if ( cb->effect.generated_name_str.empty() )
        cb->effect.name();

      _name_str = cb->effect.generated_name_str;
    }

    debug_str = fmt::format( "{}:{}-{}", util::proc_trigger_type_string( type ), _name_str, spell->name_cstr() );
#endif
  }

  ~proc_event_t() override
  {
    if ( source_state )
      action_state_t::release( source_state );
  }

  const char* name() const override
  {
    return "dbc_proc_event";
  }
#ifndef NDEBUG
  const char* debug() const override
  {
    return debug_str.c_str();
  }
#endif
  void execute() override
  {
    cb->execute( spell, target, source_state );
  }
};

const item_t dbc_proc_callback_t::default_item_ = item_t();

cooldown_t* dbc_proc_callback_t::get_cooldown( player_t* target )
{
  if ( !target_specific_cooldown || !target )
    return cooldown;

  return target_specific_cooldown->get_cooldown( target );
}

buff_t* dbc_proc_callback_t::find_debuff( player_t* target ) const
{
  if ( !target )
    return nullptr;

  return target_specific_debuff[ target ];
}

buff_t* dbc_proc_callback_t::get_debuff( player_t* target )
{
  if ( !target )
    target = listener->target;
  if ( !target )
    return nullptr;

  buff_t*& debuff = target_specific_debuff[ target ];
  if ( !debuff )
    debuff = create_debuff( target );
  return debuff;
}

buff_t* dbc_proc_callback_t::create_debuff( player_t* target )
{
  std::string name_ = target_debuff->ok() ? target_debuff->name_cstr() : effect.name();
  util::tokenize( name_ );
  return make_buff( actor_pair_t( target, listener ), name_, target_debuff );
}

// Set up the callback to be activated when the buff triggers, and deactivated when the buff expires.
// NOTE: If the callback is created after player_t::init_special_effects(), such as from within create_debuffs() on a
// new target_specific_debuff, init MUST be set to true.
void dbc_proc_callback_t::activate_with_buff( buff_t* buff, bool init )
{
  if ( buff->is_fallback )
    return;

  if ( init )
    initialize();

  deactivate();

  buff->add_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
    if ( !old_ )
      activate();
    else if ( !new_ )
      deactivate();
  } );
}

void dbc_proc_callback_t::deactivate_with_buff( buff_t* buff, bool init )
{
  if ( buff->is_fallback )
    return;

  if ( init )
    initialize();

  buff->add_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
    if ( !old_ )
      deactivate();
    else if ( !new_ )
      activate();
  } );
}

void dbc_proc_callback_t::trigger( const proc_data_t& source_data, player_t* target, action_state_t* state,
                                   proc_trigger_type_e type )
{
  auto cd = get_cooldown( target );
  if ( cd && cd->down() )
    return;

  // Fully overridden trigger condition check; if allowed, perform normal proc chance behavior.
  if ( trigger_type == trigger_fn_type::TRIGGER )
  {
    if ( !( *trigger_fn )( this, source_data.spell, target, state, type ) )
    {
      return;
    }
  }
  else
  {
    // Weapon-based proc triggering differs from "old" callbacks. When used
    // (weapon_proc == true), dbc_proc_callback_t _REQUIRES_ that the action
    // has the correct weapon specified. Old style procs allowed actions
    // without any weapon to pass through.
    if ( weapon )
    {
      assert( state );
      if ( !state->action->weapon || ( state->action->weapon && state->action->weapon != weapon ) )
        return;
    }

    // Don't allow procs to proc itself
    if ( proc_action && state && state->action && state->action->internal_id == proc_action->internal_id )
    {
      return;
    }

    if ( proc_action && proc_action->harmful )
    {
      // Don't allow players to harm other players, and enemies harm other enemies
      if ( listener->is_enemy() == target->is_enemy() )
        return;
    }

    if ( !proc_data_t::check_proc_trigger( source_data, proc_data, type ) )
      return;

    // Additional trigger condition to check before performing proc chance process.
    if ( trigger_type == trigger_fn_type::CONDITION &&
         !( *trigger_fn )( this, source_data.spell, target, state, type ) )
    {
      return;
    }
  }

  bool triggered = roll( state ? state->action : nullptr );

  if ( listener->sim->debug )
  {
    std::string source_str;
    if ( type != proc_trigger_type_e::TRIGGER_HEARTBEAT )
    {
      if ( source_data->ok() )
        source_str = fmt::format( " {}", *source_data.spell );
      else if ( state && state->action )
        source_str = fmt::format( " {}", state->action->name_str );
      else
        source_str = " unknown";
    }

    listener->sim->print_debug( "{} attempts to proc {} on {}{}: {:d}", *listener, effect,
                                util::proc_trigger_type_string( type ), source_str, triggered );
  }

  if ( triggered )
  {
    // Detach proc execution from proc triggering
    make_event<proc_event_t>( *listener->sim, this, source_data.spell, target, state, type );

    if ( cd )
      cd->start();
  }
}

dbc_proc_callback_t::dbc_proc_callback_t( const item_t& i, player_t* p, const special_effect_t& e )
  : action_callback_t( p ),
    item( i ),
    effect( e ),
    cooldown( nullptr ),
    target_specific_cooldown( nullptr ),
    target_specific_debuff( false ),
    target_debuff( spell_data_t::nil() ),
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
    proc_data(),
    can_only_proc_from_class_abilities( proc_data.can_only_proc_from_class_abilities ),
    can_proc_from_procs( proc_data.can_proc_from_procs ),
    can_proc_from_suppressed( proc_data.can_proc_from_suppressed ),
    can_proc_from_suppressed_target( proc_data.can_proc_from_suppressed_target )
{
  assert( e.proc_flags() != 0 );
}

dbc_proc_callback_t::dbc_proc_callback_t( const item_t& i, const special_effect_t& e )
  : dbc_proc_callback_t( i, i.player, e )
{}

dbc_proc_callback_t::dbc_proc_callback_t( const item_t* i, const special_effect_t& e )
  : dbc_proc_callback_t( *i, i->player, e )
{}

dbc_proc_callback_t::dbc_proc_callback_t( player_t* p, const special_effect_t& e )
  : dbc_proc_callback_t( default_item_, p, e )
{}

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

  if ( proc_action && &proc_action->data() == spell_data_t::not_found() )
  {
    listener->sim->error( "{} for {} attempting to use {} without meeting requirements, ignoring.", effect.name(),
                          *listener, *proc_action );

    proc_action = nullptr;
  }

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

  proc_data.spell = effect.driver();
  can_only_proc_from_class_abilities = effect.can_only_proc_from_class_abilities();
  can_proc_from_procs = effect.can_proc_from_procs();
  can_proc_from_suppressed = effect.can_proc_from_suppressed();
  can_proc_from_suppressed_target = effect.can_proc_from_suppressed_target();
}

// Determine target for the callback (action).

player_t* dbc_proc_callback_t::get_target( player_t* target, action_state_t* state, action_t* p_action ) const
{
  auto _action = p_action ? p_action : proc_action;

  // Outgoing callbacks always target the target of the state object
  if ( target != listener )
  {
    return target;
  }

  // Incoming callbacks target either the callback actor, or the source of the incoming state.
  // Which is selected depends on the type of the callback proc action.
  //
  // Technically, this information is exposed in the client data, but simc needs a proper
  // targeting system first before we start using it.
  assert( _action && "Cannot determine target of incoming callback, there is no proc_action" );
  switch ( _action->type )
  {
    case ACTION_ATTACK:
    case ACTION_SPELL:
      // Self Damage and energize targets are redirected to the players main target. Else they target the player.
      // TODO: Verify this behaviour with damage to friendly Allies.
      if ( state->action->player == listener )
        return listener->target;
      SC_FALLTHROUGH;
    default:
      return listener;
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
  else if ( ppm > 0 && action )
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

void dbc_proc_callback_t::execute( const spell_data_t* spell, player_t* target, action_state_t* state )
{
  if ( state && state->target->is_sleeping() )
  {
    return;
  }

  if ( execute_fn )
  {
    ( *execute_fn )( this, spell, target, state );
  }
  else
  {
    bool triggered = proc_buff == nullptr;
    if ( proc_buff )
      triggered = proc_buff->trigger();

    if ( state && triggered && proc_action && ( !proc_buff || proc_buff->check() == proc_buff->max_stack() ) )
    {
      // Snapshot a new state for schedule_execute() as AoE-triggered procs may require different targets
      proc_action->set_target( get_target( target, state ) );
      auto proc_state = proc_action->get_state();
      proc_state->target = proc_action->target;
      proc_action->snapshot_state( proc_state, proc_action->amount_type( proc_state ) );
      proc_action->schedule_execute( proc_state );

      // Decide whether to expire the buff even with 1 max stack
      if ( expire_on_max_stack )
      {
        assert( proc_buff );
        proc_buff->expire();
      }
    }
  }
}

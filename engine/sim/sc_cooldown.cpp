// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_cooldown.hpp"
#include "action/sc_action.hpp"
#include "player/sc_player.hpp"
#include "sim/event.hpp"
#include "sim/sc_expressions.hpp"
#include "sim/sc_sim.hpp"

namespace { // UNNAMED NAMESPACE

struct recharge_event_t : event_t
{
  cooldown_t* cooldown_;

  recharge_event_t( cooldown_t* cd ) :
    event_t( cd->sim, cd->recharge_multiplier * cd->base_duration ),
    cooldown_( cd )
  { }

  recharge_event_t( cooldown_t* cd, timespan_t event_duration ) :
    event_t( cd->sim, event_duration ),
    cooldown_( cd )
  { }

  const char* name() const override
  { return "recharge_event"; }

  void execute() override
  {
    assert( cooldown_->current_charge < cooldown_->charges );
    cooldown_->current_charge++;
    cooldown_->ready = cooldown_t::ready_init();

    if ( cooldown_->current_charge < cooldown_->charges )
    {
      cooldown_->recharge_event = make_event<recharge_event_t>( sim(), cooldown_ );
    }
    else
    {
      cooldown_->recharge_event = nullptr;
      cooldown_->last_charged = sim().current_time();
    }

    if ( sim().debug )
    {
      auto base_duration = cooldown_->base_duration.total_seconds();
      auto dur = cooldown_->recharge_multiplier * base_duration;
      sim().out_debug.printf( "%s recharge cooldown %s regenerated charge, current=%d, total=%d, next=%.3f, ready=%.3f, dur=%.3f, base_dur=%.3f, mul=%f",
        cooldown_->player ? cooldown_->player->name() : "sim", cooldown_->name_str.c_str(), cooldown_->current_charge, cooldown_->charges,
        cooldown_->recharge_event ? cooldown_->recharge_event->occurs().total_seconds() : 0,
        cooldown_->ready.total_seconds(),
        dur,
        base_duration,
        cooldown_->recharge_multiplier );
    }

    cooldown_->update_ready_thresholds();

    if ( cooldown_->player )
    {
      cooldown_->player->trigger_ready();
    }
  }
};

struct ready_trigger_event_t : public event_t
{
  player_t& player;
  cooldown_t* cooldown;

  ready_trigger_event_t( player_t& p, cooldown_t* cd ) :
    event_t( p, cd -> ready - p.sim -> current_time() ),
    player(p),
    cooldown( cd )
  { }

  const char* name() const override
  { return "ready_trigger_event"; }

  void execute() override
  {
    cooldown -> ready_trigger_event = nullptr;
    player.trigger_ready();
  }
};

} // UNNAMED NAMESPACE

cooldown_t::cooldown_t( util::string_view n, player_t& p ) :
  sim( *p.sim ),
  player( &p ),
  name_str( n ),
  duration( 0_ms ),
  ready( ready_init() ),
  reset_react( 0_ms ),
  charges( 1 ),
  recharge_event( nullptr ),
  ready_trigger_event( nullptr ),
  last_start( 0_ms ),
  last_charged( 0_ms ),
  hasted( false ),
  action( nullptr ),
  execute_types_mask( 0U ),
  current_charge( 1 ),
  recharge_multiplier( 1.0 ),
  base_duration( 0_ms )
{ }

cooldown_t::cooldown_t( util::string_view n, sim_t& s ) :
  sim( s ),
  player( nullptr ),
  name_str( n ),
  duration( 0_ms ),
  ready( ready_init() ),
  reset_react( 0_ms ),
  charges( 1 ),
  recharge_event( nullptr ),
  ready_trigger_event( nullptr ),
  last_start( 0_ms ),
  last_charged( 0_ms ),
  hasted( false ),
  action( nullptr ),
  execute_types_mask( 0U ),
  current_charge( 1 ),
  recharge_multiplier( 1.0 ),
  base_duration( 0_ms )
{ }

/**
 * Adjust the recharge multiplier of a dynamic cooldown.
 *
 * Adjust a dynamic cooldown (reduction) multiplier based on the current action associated with the cooldown.
 * Actions are associated by start() calls.
 */
void cooldown_t::adjust_recharge_multiplier()
{
  if ( !ongoing() )
  {
    return;
  }

  double old_multiplier = recharge_multiplier;
  assert( action && "Only cooldowns with associated action can have their recharge multiplier adjusted." );
  recharge_multiplier = action->recharge_multiplier( *this ) * action->recharge_rate_multiplier( *this );
  assert( recharge_multiplier > 0.0 );
  if ( old_multiplier == recharge_multiplier )
  {
    return;
  }

  timespan_t old_ready = ready;
  adjust_remaining_duration( recharge_multiplier / old_multiplier );

  if ( sim.debug )
  {
    sim.out_debug.printf( "%s dynamic cooldown %s adjusted: new_ready=%.3f old_ready=%.3f old_mul=%f new_mul=%f",
        action -> player -> name(), name(), ready.total_seconds(), old_ready.total_seconds(),
        old_multiplier, recharge_multiplier );
  }
}

/**
 * Adjust the base duration of a dynamic cooldown.
 *
 * Adjust a dynamic cooldown duration based on the current action associated with the cooldown.
 * Actions are associated by start() calls.
 */
void cooldown_t::adjust_base_duration()
{
  if ( !ongoing() )
  {
    return;
  }

  timespan_t old_duration = base_duration;
  assert( action && "Only cooldowns with associated action can have their base duration adjusted." );
  base_duration = action->cooldown_base_duration( *this );
  assert( base_duration > 0_ms );
  if ( old_duration == base_duration )
  {
    return;
  }

  timespan_t old_ready = ready;
  adjust_remaining_duration( base_duration / old_duration );

  if ( sim.debug )
  {
    sim.out_debug.printf( "%s dynamic cooldown %s adjusted: new_ready=%.3f old_ready=%.3f old_dur=%.3f new_dur=%.3f",
      action->player->name(), name(), ready.total_seconds(), old_ready.total_seconds(),
      old_duration.total_seconds(), base_duration.total_seconds() );
  }
}

void cooldown_t::adjust_remaining_duration( double delta )
{
  assert( ongoing() && delta > 0.0 );
  assert( charges > 0 && "Cooldown charges must be positive");

  timespan_t new_remains;
  timespan_t remains;
  if ( charges == 1 )
  {
    remains = ready - sim.current_time();
    new_remains = remains * delta;
  }
  else
  {
    remains = recharge_event->remains();
    new_remains = remains * delta;
    // Shortened, reschedule the event
    if ( new_remains < remains )
    {
      event_t::cancel( recharge_event );
      recharge_event = make_event<recharge_event_t>( sim, this, new_remains );
    }
    else if ( new_remains > remains )
    {
      recharge_event->reschedule( new_remains );
    }
  }

  if ( down() )
  {
    ready = sim.current_time() + new_remains;
  }
  if ( charges == 1 )
  {
    last_charged = ready;
  }

  update_ready_thresholds();

  if ( player && player->queueing && player->queueing->cooldown == this )
  {
    player->queueing->reschedule_queue_event();
  }
}

void cooldown_t::adjust( timespan_t amount, bool requires_reaction )
{
  if ( amount == 0_ms )
    return;

  if ( action )
    amount *= action->recharge_rate_multiplier( *this );

  // Normal cooldown, just adjust as we see fit
  if ( charges == 1 )
  {
    // No cooldown ongoing, don't do anything
    if ( up() )
      return;

    // Cooldown resets
    if ( ready + amount <= sim.current_time() )
      reset( requires_reaction );
    // Still some time left, adjust ready
    else
    {
      ready += amount;
      last_charged += amount;
    }

    if ( sim.debug )
      sim.out_debug.printf( "%s cooldown %s adjustment=%.3f, remains=%.3f, ready=%.3f",
        player ? player->name() : "sim", name_str.c_str(), amount.total_seconds(),
        ( ready - sim.current_time() ).total_seconds(), ready.total_seconds() );
  }
  // Charge-based cooldown
  else if ( current_charge < charges )
  {
    // Remaining time on the recharge event
    timespan_t remains = recharge_event->remains() + amount;

    // Didnt recharge a charge, just recreate the recharge event to occur
    // sooner
    if ( remains > 0_ms )
    {
      event_t::cancel( recharge_event );
      recharge_event = make_event<recharge_event_t>( sim, this, remains );

      // If we have no charges, adjust ready time to the new occurrence time
      // of the recharge event, plus a millisecond
      if ( current_charge == 0 )
        ready += amount;

      if ( sim.debug )
        sim.out_debug.printf( "%s recharge cooldown %s adjustment=%.3f, remains=%.3f, occurs=%.3f, ready=%.3f",
          player ? player->name() : "sim", name_str.c_str(), amount.total_seconds(), remains.total_seconds(),
          recharge_event->occurs().total_seconds(), ready.total_seconds() );
    }
    // Recharged at least one charge
    else
    {
      timespan_t cd_duration = recharge_multiplier * base_duration;

      // If the remaining adjustment is greater than cooldown duration,
      // we have to recharge more than one charge.
      int extra_charges = as<int>( -remains.total_millis() / cd_duration.total_millis() );
      reset( requires_reaction, 1 + extra_charges );
      remains += extra_charges * cd_duration;

      // Excess time adjustment goes to the next recharge event, if we didnt
      // max out on charges (recharge_event is still present after reset()
      // call)
      if ( recharge_event )
      {
        event_t::cancel( recharge_event );
        recharge_event = make_event<recharge_event_t>( sim, this, cd_duration + remains );
      }

      if ( sim.debug )
      {
        sim.out_debug.printf( "%s recharge cooldown %s regenerated %d charge(s), current=%d, total=%d, reminder=%.3f, next=%.3f, ready=%.3f",
          player ? player -> name() : "sim", name_str.c_str(),
          1 + extra_charges, current_charge, charges,
          remains.total_seconds(),
          recharge_event ? recharge_event -> occurs().total_seconds() : 0,
          ready.total_seconds() );
      }
    }
  }

  update_ready_thresholds();

  if ( player && player->queueing && player->queueing->cooldown == this )
  {
    player->queueing->reschedule_queue_event();
  }
}

void cooldown_t::reset_init()
{
  ready = ready_init();
  last_start = 0_ms;
  last_charged = 0_ms;
  reset_react = 0_ms;

  current_charge = charges;
  recharge_multiplier = 1.0;
  base_duration = duration;

  recharge_event = nullptr;
  ready_trigger_event = nullptr;
}

void cooldown_t::reset( bool require_reaction, int charges_ )
{
  if ( charges_ == 0 )
    return;
  if ( charges_ < 0 )
    charges_ = charges;

  bool was_down = down();
  ready = ready_init();

  current_charge = std::min( charges, current_charge + charges_ );

  if ( require_reaction && player )
  {
    if ( was_down )
      reset_react = sim.current_time() + player->total_reaction_time();
  }
  else
  {
    reset_react = 0_ms;
  }
  if ( current_charge == charges )
  {
    event_t::cancel( recharge_event );
    last_charged = sim.current_time();
  }
  event_t::cancel( ready_trigger_event );

  update_ready_thresholds();

  if ( player && player->queueing && player->queueing->cooldown == this )
  {
    player->queueing->reschedule_queue_event();
  }

  if ( player )
  {
    player->trigger_ready();
  }
}

timespan_t cooldown_t::remains() const
{
  return std::max( timespan_t::zero(), ready - sim.current_time() );
}

timespan_t cooldown_t::current_charge_remains() const
{ return recharge_event ? recharge_event -> remains() : timespan_t::zero(); }

bool cooldown_t::up() const
{ return ready <= sim.current_time(); }

bool cooldown_t::down() const
{ return ready > sim.current_time(); }

timespan_t cooldown_t::queue_delay() const
{ return std::max( timespan_t::zero(), ready - sim.current_time() ); }

void cooldown_t::start( action_t* a, timespan_t _override, timespan_t delay )
{
  // Zero duration cooldowns are nonsense
  if ( _override == 0_ms || ( _override < 0_ms && duration <= 0_ms ) )
  {
    return;
  }

  reset_react = 0_ms;
  action = a;

  // If a recharge is already in progress, we do not need to update the cooldown
  // state besides removing a charge and adjusting ready.
  if ( recharge_event )
  {
    assert( current_charge > 0 );
    current_charge--;
    // No charges left, the cooldown won't be ready until a recharge event
    // occurs. Note, ready still needs to be properly set as it ultimately
    // controls whether a cooldown is "up".
    if ( current_charge == 0 )
    {
      ready = recharge_event->occurs() + 1_ms;
    }
    return;
  }

  if ( a )
  {
    recharge_multiplier = a->recharge_multiplier( *this ) * a->recharge_rate_multiplier( *this );
  }
  else
  {
    recharge_multiplier = 1.0;
  }

  if ( _override > 0_ms )
  {
    base_duration = _override;
  }
  else if ( a )
  {
    base_duration = a->cooldown_base_duration( *this );
  }
  else
  {
    base_duration = duration;
  }

  timespan_t event_duration = recharge_multiplier * base_duration;
  assert( event_duration > 0_ms );

  if ( delay > 0_ms )
  {
    event_duration += delay;
  }

  // Normal cooldowns have charges = 0 or 1, and do not use the event system to
  // trigger ready status (for efficiency reasons). Charged cooldowns recharge
  // through the event system.
  if ( charges > 1 )
  {
    assert( current_charge > 1 && !recharge_event );
    last_charged = 0_ms;
    current_charge--;
    recharge_event = make_event<recharge_event_t>( sim, this, event_duration );
  }
  else
  {
    ready = sim.current_time() + event_duration;
    last_charged = ready;
  }

  last_start = sim.current_time();

  update_ready_thresholds();

  if ( player && player->ready_type == READY_TRIGGER )
  {
    ready_trigger_event = make_event<ready_trigger_event_t>( sim, *player, this );
  }
}

void cooldown_t::start( timespan_t _override, timespan_t delay )
{
  start( nullptr, _override, delay );
}

timespan_t cooldown_t::cooldown_duration( const cooldown_t* cd )
{
  if ( cd->ongoing() )
    return cd->recharge_multiplier * cd->base_duration;
  else if ( cd->action )
    return cd->action->recharge_multiplier( *cd ) * cd->action->recharge_rate_multiplier( *cd ) * cd->action->cooldown_base_duration( *cd );
  else
    return cd->duration;
}

std::unique_ptr<expr_t> cooldown_t::create_expression( util::string_view name_str )
{
  if ( name_str == "remains" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::remains );
  else if ( name_str == "base_duration" )
  {
    return make_fn_expr( name_str, [ this ]
    {
      if ( ongoing() )
        return base_duration.total_seconds();
      else if ( action )
        return action->cooldown_base_duration( *this ).total_seconds();
      else
        return duration.total_seconds();
    } );
  }
  else if ( name_str == "duration" )
  {
    return make_fn_expr( name_str, [ this ]
    {
      return cooldown_duration( this );
    } );
  }
  else if ( name_str == "up" || name_str == "ready" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::up );
  else if ( name_str == "charges" )
  {
    return make_fn_expr( name_str, [ this ]
    {
      if ( charges <= 1 )
      {
        return up() ? 1.0 : 0.0;
      }
      else
      {
        return as<double>( current_charge );
      }
    } );
  }
  else if ( name_str == "charges_fractional" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::charges_fractional );
  else if ( name_str == "recharge_time" )
  {
    return make_fn_expr( name_str, [ this ]
    {
      if ( charges <= 1 )
        return remains().total_seconds();
      else if ( recharge_event )
        return recharge_event->remains().total_seconds();
      else
        return 0.0;
    } );
  }
  else if ( name_str == "full_recharge_time" )
  {
    return make_fn_expr( name_str, [ this ]
    {
      if ( charges <= 1 )
      {
        return remains().total_seconds();
      }
      else if ( recharge_event )
      {
        auto duration = cooldown_duration( this );
        return ( current_charge_remains() + ( charges - current_charge - 1 ) * duration ).total_seconds();
      }
      else
        return 0.0;
    } );
  }
  else if ( name_str == "max_charges" )
    return make_ref_expr( name_str, charges );

  // For cooldowns that can be reduced through various means, _guess and _expected will return an approximate
  // duration based on a comparison between time since last start and the current remaining cooldown
  else if ( name_str == "remains_guess" || name_str == "remains_expected" )
  {
    return make_fn_expr( name_str, [ this ]
    {
      if ( remains() == duration )
        return duration;
      if ( up() )
        return 0_ms;

      double reduction = ( sim.current_time() - last_start ) /
                         ( duration - remains() );
      return remains() * reduction;
    } );
  }

  else if ( name_str == "duration_guess" || name_str == "duration_expected" )
  {
    return make_fn_expr( name_str, [ this ]
    {
      if ( last_charged == 0_ms || remains() == duration )
        return duration;

      if ( up() )
        return ( last_charged - last_start );

      double reduction = ( sim.current_time() - last_start ) /
                         ( duration - remains() );
      return duration * reduction;
    } );
   }

  throw std::invalid_argument( fmt::format( "Unsupported cooldown expression '{}'.", name_str ) );
}

void cooldown_t::update_ready_thresholds()
{
  if ( player == nullptr )
  {
    return;
  }

  if ( execute_types_mask & ( 1 << static_cast<unsigned>( execute_type::OFF_GCD ) ) )
  {
    player->update_off_gcd_ready();
  }

  if ( execute_types_mask & ( 1 << static_cast<unsigned>( execute_type::CAST_WHILE_CASTING ) ) )
  {
    player->update_cast_while_casting_ready();
  }
}

timespan_t cooldown_t::queueable() const
{
  return ready - player->cooldown_tolerance();
}

double cooldown_t::charges_fractional() const
{
  if ( charges > 1 )
  {
    double c = current_charge;
    if ( recharge_event )
    {
      c += 1 - std::min( 1.0, current_charge_remains() / cooldown_duration( this ) );
    }
    return c;
  }
  else
  {
    if ( up() )
    {
      return 1.0;
    }
    else
    {
      return 1 - std::min( 1.0, remains() / cooldown_duration( this ) );
    }
  }
}

bool cooldown_t::is_ready() const
{
  if (up())
  {
    return true;
  }

  // Cooldowns that are not bound to specific actions should not be considered as queueable in the
  // simulator, ever, so just return up() here. This limits the actual queueable cooldowns to
  // basically only abilities, where the user must press a button to initiate the execution. Note
  // that off gcd abilities that bypass schedule_execute (i.e., action_t::use_off_gcd is set to
  // true) will for now not use the queueing system.
  if (!action || !player)
  {
    return up();
  }

  // Cooldown is not up, and is action-bound (essentially foreground action), check if it's within
  // the player's (or default) cooldown tolerance for queueing.
  return queueable() <= sim.current_time();
}

void format_to( const cooldown_t& cooldown, fmt::format_context::iterator out )
{
  fmt::format_to( out, "Cooldown {}", cooldown.name_str );
}

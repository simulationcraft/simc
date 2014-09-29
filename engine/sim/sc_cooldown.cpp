// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct recharge_event_t : event_t
{
  cooldown_t* cooldown;
  action_t* action;

  recharge_event_t( player_t& p, cooldown_t* cd, action_t* a, timespan_t rdelay, timespan_t delay = timespan_t::zero() ) :
    event_t( p, "recharge_event" ), cooldown( cd ), action( a )
  {
    sim().add_event( this, cd_time( rdelay ) + delay );
  }

  timespan_t cd_time( const timespan_t& delay ) const
  { return delay * cooldown -> get_recharge_multiplier() * ( action ? action -> cooldown_reduction() : 1.0 );  }

  virtual void execute()
  {
    assert( cooldown -> current_charge < cooldown -> charges );
    cooldown -> current_charge++;
    cooldown -> ready = cooldown_t::ready_init();

    if ( cooldown -> current_charge < cooldown -> charges )
    {
      cooldown -> recharge_event = new ( sim() ) recharge_event_t( *p(), cooldown, action, cooldown -> duration );
    }
    else
    {
      cooldown -> recharge_event = 0;
    }

    if ( sim().debug )
      sim().out_debug.printf( "%s recharge cooldown %s regenerated charge, current=%d, total=%d, next=%.3f, ready=%.3f",
        action -> player -> name(), cooldown -> name_str.c_str(), cooldown -> current_charge, cooldown -> charges,
        cooldown -> recharge_event ? cooldown -> recharge_event -> occurs().total_seconds() : 0,
        cooldown -> ready.total_seconds() );
  }
};

struct ready_trigger_event_t : public event_t
{
  cooldown_t* cooldown;

  ready_trigger_event_t( player_t& p, cooldown_t* cd ) :
    event_t( p, "ready_trigger_event" ),
    cooldown( cd )
  {
    sim().add_event( this, cd -> ready - sim().current_time );
  }

  void execute()
  {
    cooldown -> ready_trigger_event = 0;
    p() -> trigger_ready();
  }
};

} // UNNAMED NAMESPACE

cooldown_t::cooldown_t( const std::string& n, player_t& p ) :
  sim( *p.sim ),
  player( &p ),
  name_str( n ),
  duration( timespan_t::zero() ),
  ready( ready_init() ),
  reset_react( timespan_t::zero() ),
  charges( 1 ),
  current_charge( 1 ),
  recharge_event( nullptr ),
  ready_trigger_event( nullptr ),
  last_start( timespan_t::zero() ),
  recharge_multiplier( 1.0 )
{}

cooldown_t::cooldown_t( const std::string& n, sim_t& s ) :
  sim( s ),
  player( nullptr ),
  name_str( n ),
  duration( timespan_t::zero() ),
  ready( ready_init() ),
  reset_react( timespan_t::zero() ),
  charges( 1 ),
  current_charge( 1 ),
  recharge_event( nullptr ),
  ready_trigger_event( nullptr ),
  last_start( timespan_t::zero() ),
  recharge_multiplier( 1.0 )
{}

void cooldown_t::adjust( timespan_t amount, bool require_reaction )
{
  // Normal cooldown, just adjust as we see fit
  if ( charges == 1 )
  {
    // No cooldown ongoing, don't do anything
    if ( up() )
      return;

    // Cooldown resets
    if ( ready + amount <= sim.current_time )
      reset( require_reaction );
    // Still some time left, adjust ready
    else
      ready += amount;
  }
  // Charge-based cooldown
  else if ( current_charge < charges )
  {
    // Remaining time on the recharge event
    timespan_t remains = recharge_event -> remains() + amount;

    // Didnt recharge a charge, just recreate the recharge event to occur
    // sooner
    if ( remains > timespan_t::zero() )
    {
      action_t* a = debug_cast<recharge_event_t*>( recharge_event ) -> action;
      core_event_t::cancel( recharge_event );
      recharge_event = new ( sim ) recharge_event_t( *player, this, a, remains );

      // If we have no charges, adjust ready time to the new occurrence time
      // of the recharge event, plus a millisecond
      if ( current_charge == 0 )
        ready += amount;

      if ( sim.debug )
        sim.out_debug.printf( "%s recharge cooldown %s adjustment=%.3f, remains=%.3f, occurs=%.3f, ready=%.3f",
          player -> name(), name_str.c_str(), amount.total_seconds(), remains.total_seconds(),
          recharge_event -> occurs().total_seconds(), ready.total_seconds() );
    }
    // Recharged a charge
    else
    {
      reset( require_reaction );
      // Excess time adjustment goes to the next recharge event, if we didnt
      // max out on charges (recharge_event is still present after reset()
      // call)
      if ( remains < timespan_t::zero() && recharge_event )
      {
        action_t* a = debug_cast<recharge_event_t*>( recharge_event ) -> action;
        core_event_t::cancel( recharge_event );
        recharge_event = new ( sim ) recharge_event_t( *player, this, a, duration + remains );
      }

      if ( sim.debug )
      {
        sim.out_debug.printf( "%s recharge cooldown %s regenerated charge, current=%d, total=%d, reminder=%.3f, next=%.3f, ready=%.3f",
          player -> name(), name_str.c_str(), current_charge, charges,
          remains.total_seconds(),
          recharge_event ? recharge_event -> occurs().total_seconds() : 0,
          ready.total_seconds() );
      }
    }
  }
}

void cooldown_t::reset_init()
{
  ready = ready_init();
  last_start = timespan_t::zero();
  reset_react = timespan_t::zero();

  current_charge = charges;

  recharge_event = 0;
  ready_trigger_event = 0;
}

void cooldown_t::reset( bool require_reaction )
{
  bool was_down = down();
  ready = ready_init();
  if ( last_start > sim.current_time )
    last_start = timespan_t::zero();
  if ( charges == 1 )
    current_charge = charges;
  else if ( current_charge < charges )
    current_charge ++;
  if ( require_reaction && player )
  {
    if ( was_down )
      reset_react = sim.current_time + player -> total_reaction_time();
  }
  else
  {
    reset_react = timespan_t::zero();
  }
  if ( current_charge == charges )
    core_event_t::cancel( recharge_event );
  core_event_t::cancel( ready_trigger_event );
}

void cooldown_t::start( action_t* action, timespan_t _override, timespan_t delay )
{
  reset_react = timespan_t::zero();
  if ( _override < timespan_t::zero() ) _override = duration;

  // Zero duration cooldowns are nonsense
  if ( _override == timespan_t::zero() )
    return;

  if ( charges > 1 )
  {
    assert( current_charge > 0 );
    current_charge--;

    // Begin a recharge event
    if ( ! recharge_event )
    {
      recharge_event = new ( sim ) recharge_event_t( *player, this, action, duration, delay );
    }
    // No charges left, the cooldown won't be ready until a recharge event
    // occurs
    else if ( current_charge == 0 )
    {
      assert( recharge_event );
      ready = recharge_event -> occurs() + timespan_t::from_millis( 1 );
    }
  }
  else
  {
    ready = sim.current_time + _override * recharge_multiplier + delay;
    last_start = sim.current_time;
  }

  assert( player );
  if ( player -> ready_type == READY_TRIGGER )
    ready_trigger_event = new ( sim ) ready_trigger_event_t( *player, this );
}

void cooldown_t::start( timespan_t _override, timespan_t delay )
{
  start( 0, _override, delay );
}

void cooldown_t::set_recharge_multiplier( double v )
{
  assert( v >= 0.0 && "Cooldown recharge multiplier should probably not be negative." );

  if ( up() )
  {
    // Cooldown not active, just set the recharge multiplier.
    recharge_multiplier = v;
  }
  else
  {
    // Cooldown up, we need to recalculate remaining duration.
    timespan_t new_leftover_adjust = remains() * ( v / recharge_multiplier - 1.0  );
    ready += new_leftover_adjust;
    recharge_multiplier = v;
  }
}

expr_t* cooldown_t::create_expression( action_t*, const std::string& name_str )
{

  if ( name_str == "remains" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::remains );
  else if ( name_str == "duration" )
    return make_ref_expr( name_str, duration );
  else if ( name_str == "up" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::up );

  return nullptr;
}

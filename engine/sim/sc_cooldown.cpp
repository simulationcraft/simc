// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct recharge_event_t : event_t
{
  cooldown_t* cooldown_;
  action_t* action_;
  timespan_t duration_, event_duration_;

  recharge_event_t( cooldown_t* cd, action_t* a, timespan_t base_duration ) :
    event_t( cd -> sim ), cooldown_( cd ), action_( a ),
    duration_( base_duration ),
    event_duration_( cooldown_t::cooldown_duration( cd, base_duration, a ) )
  {
    sim().add_event( this, event_duration_ );
  }
  virtual const char* name() const override
  { return "recharge_event"; }
  recharge_event_t( cooldown_t* cd, action_t* a, timespan_t event_duration, timespan_t base_duration ) :
    event_t( *cd -> player ), cooldown_( cd ), action_( a ),
    duration_( base_duration ), event_duration_( event_duration )
  {
    sim().add_event( this, event_duration_ );
  }
  virtual void execute()
  {
    assert( cooldown_ -> current_charge < cooldown_ -> charges );
    cooldown_ -> current_charge++;
    cooldown_ -> ready = cooldown_t::ready_init();

    if ( cooldown_ -> current_charge < cooldown_ -> charges )
    {
      cooldown_ -> recharge_event = new ( sim() ) recharge_event_t( cooldown_, action_, duration_ );
    }
    else
    {
      cooldown_ -> recharge_event = 0;
      cooldown_ -> last_charged = sim().current_time();
    }

    if ( sim().debug )
      sim().out_debug.printf( "%s recharge cooldown %s regenerated charge, current=%d, total=%d, next=%.3f, ready=%.3f",
        action_ -> player -> name(), cooldown_ -> name_str.c_str(), cooldown_ -> current_charge, cooldown_ -> charges,
        cooldown_ -> recharge_event ? cooldown_ -> recharge_event -> occurs().total_seconds() : 0,
        cooldown_ -> ready.total_seconds() );
  }
};

struct ready_trigger_event_t : public player_event_t
{
  cooldown_t* cooldown;

  ready_trigger_event_t( player_t& p, cooldown_t* cd ) :
    player_event_t( p ),
    cooldown( cd )
  {
    sim().add_event( this, cd -> ready - sim().current_time() );
  }
  virtual const char* name() const override
  { return "ready_trigger_event"; }
  void execute()
  {
    cooldown -> ready_trigger_event = 0;
    p() -> trigger_ready();
  }
};

} // UNNAMED NAMESPACE

timespan_t cooldown_t::cooldown_duration( const cooldown_t* cd,
                                          const timespan_t& override_duration,
                                          const action_t*   cooldown_action )
{
  timespan_t ret;

  if ( override_duration >= timespan_t::zero() )
  {
    ret = override_duration;
  }
  else
  {
    ret = cd -> duration;
  }

  ret *= cd -> get_recharge_multiplier();

  if ( cooldown_action )
  {
    ret *= cooldown_action -> cooldown_reduction();
  }

  return ret;
}

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
  last_charged( timespan_t::zero() ),
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
  last_charged( timespan_t::zero() ),
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
    if ( ready + amount <= sim.current_time() )
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
      recharge_event_t* event = debug_cast<recharge_event_t*>( recharge_event );
      action_t* a = event -> action_;
      timespan_t duration_ = event -> duration_;
      event_t::cancel( recharge_event );
      recharge_event = new ( sim ) recharge_event_t( this, a, remains, duration_ );

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
        recharge_event_t* event = debug_cast<recharge_event_t*>( recharge_event );
        timespan_t duration_ = event -> duration_;
        action_t* a = event -> action_;
        event_t::cancel( recharge_event );

        // Note, the next recharge cycle uses the previous recharge cycle's
        // base duration, if overridden
        timespan_t new_duration = cooldown_duration( this, duration_, a );
        new_duration += remains;

        recharge_event = new ( sim ) recharge_event_t( this, a, new_duration, duration_ );
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
  last_charged = timespan_t::zero();
  reset_react = timespan_t::zero();

  current_charge = charges;

  recharge_event = 0;
  ready_trigger_event = 0;
}

void cooldown_t::reset( bool require_reaction )
{
  bool was_down = down();
  ready = ready_init();
  if ( last_start > sim.current_time() )
    last_start = timespan_t::zero();
  if ( charges == 1 )
    current_charge = charges;
  else if ( current_charge < charges )
    current_charge ++;
  if ( require_reaction && player )
  {
    if ( was_down )
      reset_react = sim.current_time() + player -> total_reaction_time();
  }
  else
  {
    reset_react = timespan_t::zero();
  }
  if ( current_charge == charges )
  {
    event_t::cancel( recharge_event );
    last_charged = sim.current_time();
  }
  event_t::cancel( ready_trigger_event );
}

void cooldown_t::start( action_t* action, timespan_t _override, timespan_t delay )
{
  // Zero duration cooldowns are nonsense
  if ( _override < timespan_t::zero() && duration <= timespan_t::zero() )
  {
    return;
  }

  reset_react = timespan_t::zero();

  timespan_t event_duration = cooldown_duration( this, _override, action );
  if ( event_duration == timespan_t::zero() )
  {
    return;
  }

  if ( delay > timespan_t::zero() )
  {
    event_duration += delay;
  }

  // Normal cooldowns have charges = 0 or 1, and do not use the event system to
  // trigger ready status (for efficiency reasons). Charged cooldowns recharge
  // through the event system.
  if ( charges > 1 )
  {
    last_charged = timespan_t::zero();

    assert( current_charge > 0 );
    current_charge--;

    // Begin a recharge event
    if ( ! recharge_event )
    {
      recharge_event = new ( sim ) recharge_event_t( this, action, event_duration, _override );
    }
    // No charges left, the cooldown won't be ready until a recharge event
    // occurs. Note, ready still needs to be properly set as it ultimately
    // controls whether a cooldown is "up".
    else if ( current_charge == 0 )
    {
      assert( recharge_event );
      ready = recharge_event -> occurs() + timespan_t::from_millis( 1 );
    }
  }
  else
  {
    ready = sim.current_time() + event_duration;
    last_start = sim.current_time();
    last_charged = ready;
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
    return make_mem_fn_expr( name_str, *this, &cooldown_t::duration );
  else if ( name_str == "up" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::up );
  else if ( name_str == "charges" )
    return make_ref_expr( name_str, current_charge );
  else if ( name_str == "charges_fractional" )
  {
    struct charges_fractional_expr_t : public expr_t
    {
      const cooldown_t* cd;
      charges_fractional_expr_t( const cooldown_t* c ) :
        expr_t( "charges_fractional" ), cd( c )
      { }

      virtual double evaluate()
      {
        double charges = cd -> current_charge;
        if ( cd -> recharge_event )
        {
          recharge_event_t* re = debug_cast<recharge_event_t*>( cd -> recharge_event );
          charges += 1 - ( re -> remains() / re -> event_duration_ );
        }
        return charges;
      }
    };
    return new charges_fractional_expr_t( this );
  }
  else if ( name_str == "recharge_time" )
  {
    struct recharge_time_expr_t : public expr_t
    {
      const cooldown_t* cd;
      recharge_time_expr_t( const cooldown_t* c ) :
        expr_t( "recharge_time" ), cd( c )
      { }

      virtual double evaluate()
      {
        if ( cd -> recharge_event )
          return cd -> recharge_event -> remains().total_seconds();
        else
          return cd -> duration.total_seconds();
      }
    };
    return new recharge_time_expr_t( this );
  }
  else if ( name_str == "max_charges" )
    return make_ref_expr( name_str, charges );

  return nullptr;
}

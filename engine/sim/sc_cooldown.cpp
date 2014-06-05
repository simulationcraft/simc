// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct recharge_event_t : event_t
{
  cooldown_t* cooldown;

  recharge_event_t( player_t& p, cooldown_t* cd, timespan_t delay = timespan_t::zero() ) :
    event_t( p, "recharge_event" ), cooldown( cd )
  {
    sim().add_event( this, cd -> duration * cd -> get_recharge_multiplier() + delay );
  }

  virtual void execute()
  {
    assert( cooldown -> current_charge < cooldown -> charges );
    cooldown -> current_charge++;

    if ( cooldown -> current_charge < cooldown -> charges )
    {
      cooldown -> recharge_event = new ( sim() ) recharge_event_t( *p(), cooldown );
    }
    else
    {
      cooldown -> recharge_event = 0;
    }
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
  if ( down() )
  {
    if ( ready + amount <= sim.current_time )
      reset( require_reaction );
    else
      ready += amount;
  }
}

void cooldown_t::reset( bool require_reaction )
{
  bool was_down = down();
  ready = ready_init();
  if ( last_start > sim.current_time )
    last_start = timespan_t::zero();
  current_charge = charges;
  if ( require_reaction && player )
  {
    if ( was_down )
      reset_react = sim.current_time + player -> total_reaction_time();
  }
  else
  {
    reset_react = timespan_t::zero();
  }
  core_event_t::cancel( recharge_event );
  core_event_t::cancel( ready_trigger_event );
}

void cooldown_t::start( timespan_t _override, timespan_t delay )
{
  reset_react = timespan_t::zero();
  if ( _override < timespan_t::zero() ) _override = duration;
  if ( _override > timespan_t::zero() )
  {
    if ( charges > 1 )
    {
      assert( current_charge > 0 );
      current_charge--;

      if ( current_charge == charges - 1 )
      {
        recharge_event = new ( sim ) recharge_event_t( *player, this, delay );
      }
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

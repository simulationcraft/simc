// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct recharge_event_t : event_t
{
  cooldown_t* cooldown;

  recharge_event_t( player_t* p, cooldown_t* cd, timespan_t delay = timespan_t::zero() ) :
    event_t( p -> sim, p, "recharge_event" ), cooldown( cd )
  {
    sim -> add_event( this, cd -> duration + delay );
  }

  virtual void execute()
  {
    assert( cooldown -> current_charge < cooldown -> charges );
    cooldown -> current_charge++;

    if ( cooldown -> current_charge < cooldown -> charges )
    {
      cooldown -> recharge_event = new ( sim ) recharge_event_t( player, cooldown );
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

  ready_trigger_event_t( player_t* p, cooldown_t* cd ) :
    event_t( p -> sim, p, "ready_trigger_event" ),
    cooldown( cd )
  {
    sim -> add_event( this, cd -> ready - sim -> current_time );
  }

  void execute()
  {
    cooldown -> ready_trigger_event = 0;
    player -> trigger_ready();
  }
};

} // UNNAMED NAMESPACE

cooldown_t::cooldown_t( const std::string& n, player_t* p ) :
  sim( p -> sim ),
  player( p ),
  name_str( n ),
  duration( timespan_t::zero() ),
  ready( ready_init() ),
  reset_react( timespan_t::zero() ),
  charges( 1 ),
  current_charge( 1 ),
  recharge_event( 0 ),
  ready_trigger_event( 0 )
{}

cooldown_t::cooldown_t( const std::string& n, sim_t* s ) :
  sim( s ),
  player( 0 ),
  name_str( n ),
  duration( timespan_t::zero() ),
  ready( ready_init() ),
  reset_react( timespan_t::zero() ),
  charges( 1 ),
  current_charge( 1 ),
  recharge_event( 0 ),
  ready_trigger_event( 0 )
{}

void cooldown_t::adjust( timespan_t amount )
{
  ready += amount;

  if ( up() )
    reset( true );
}

void cooldown_t::reset( bool early )
{
  ready = ready_init();
  current_charge = charges;
  if ( early && player )
    reset_react = sim -> current_time + player -> total_reaction_time();
  else
    reset_react = timespan_t::zero();
  event_t::cancel( recharge_event );
  event_t::cancel( ready_trigger_event );
}

void cooldown_t::start( timespan_t override, timespan_t delay )
{
  reset_react = timespan_t::zero();
  if ( override >= timespan_t::zero() ) duration = override;
  if ( duration > timespan_t::zero() )
  {
    if ( charges > 1 )
    {
      assert( current_charge > 0 );
      current_charge--;

      if ( current_charge == charges - 1 )
      {
        recharge_event = new ( sim ) recharge_event_t( player, this, delay );
      }
      else if ( current_charge == 0 )
      {
        assert( recharge_event );
        ready = recharge_event -> time + timespan_t::from_millis( 1 );
      }
    }
    else
    {
      ready = sim -> current_time + duration + delay;
    }
    assert( player );
    if ( player -> ready_type == READY_TRIGGER )
      ready_trigger_event = new ( sim ) ready_trigger_event_t( player, this );
  }
}

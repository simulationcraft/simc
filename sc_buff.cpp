// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// buff_t::buff_t ===========================================================

buff_t::buff_t( sim_t*             s,
		player_t*          p,
		const std::string& n,
		int                ms,
		double             d,
		double             cd,
		int                rng_type,
		int                a ) :
  sim( s ), player( p ), name_str( n ),
  current_stack( 0 ),max_stack( ms ), 
  duration( d ), cooldown( cd ), cooldown_ready( 0 ),
  last_start( -1 ), interval_sum( 0 ), uptime_sum( 0 ),
  up_count( 0 ), down_count( 0 ), interval_count( 0 ), start_count( 0 ),
  aura_id( 0 ), expiration( 0 ), rng( 0 ), next( 0 )
{
  buff_t** tail = 0;

  if( player )
  {
    // Player Buff
    rng = player -> get_rng( n, rng_type );
    tail = &(  player -> buff_list );
  }
  else
  {
    // Aura or Target De-Buff
    rng = sim -> get_rng( n, rng_type );
    tail = &( sim -> buff_list );
  }

  while ( *tail && name_str > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }
  next = *tail;
  *tail = this;
}

// buff_t::may_react ========================================================

bool buff_t::may_react()
{
  if( current_stack == 0 ) return false;
  return sim -> time_to_think( last_start );
}

// buff_t::remains ==========================================================

double buff_t::remains()
{
  if( current_stack <= 0 )
  {
    return 0;
  }
  if( expiration )
  {
    return expiration -> occurs() - sim -> current_time;
  }
  return -1;
}

// buff_t::trigger ==========================================================

bool buff_t::trigger( double chance,
		      int    stacks,
		      double value )
{
  if( cooldown_ready > 0 )
    if( sim -> current_time < cooldown_ready )
      return false;

  if( ! rng -> roll( chance ) )
    return false;

  increment( stacks, value );

  if( cooldown )
    cooldown_ready = sim -> current_time + cooldown;

  return true;
}

// buff_t::increment ========================================================

void buff_t::increment( int    stacks,
			double value )
{
  if( current_stack == 0 ) start();

  current_stack += stacks;
  if( current_stack > max_stack )
    max_stack = current_stack;

  current_value = value;

  if( duration > 0 )
  {
    struct expiration_t : public event_t
    {
      buff_t* buff;
      expiration_t( sim_t* sim, player_t* p, buff_t* b ) : event_t( sim, p ), buff( b )
      {
	name = buff -> name();
	sim -> add_event( this, buff -> duration );
      }
      virtual void execute()
      {
	buff -> expiration = 0;
	buff -> expire();
      }
    };

    if( expiration )
    {
      expiration -> reschedule( duration );
    }
    else
    {
      expiration = new expiration_t( sim, player, this );
    }
  }
}

// buff_t::decrement ========================================================

void buff_t::decrement( int stacks )
{
  current_stack -= stacks;

  if( stacks == 0 || current_stack <= 0 ) 
  {
    expire();
  }
}

// buff_t::start ============================================================

void buff_t::start()
{
  start_count++;
  if( player ) 
  {
    player -> aura_gain( name(), aura_id );
  }
  if( last_start >= 0 )
  {
    interval_sum += sim -> current_time - last_start;
    interval_count++;
  }
  last_start = sim -> current_time;
}

// buff_t::expire ===========================================================

void buff_t::expire()
{
  event_t::cancel( expiration );
  current_stack = 0;
  current_value = 0;
  if( player ) 
  {
    player -> aura_loss( name(), aura_id );
  }
  if( last_start >= 0 )
  {
    uptime_sum += sim -> current_time - last_start;
  }
}

// buff_t::reset ============================================================

void buff_t::reset()
{
  if( ! rng ) rng = sim -> rng;
  cooldown_ready = 0;
  expire();
}

// buff_t::merge ============================================================

void buff_t::merge( buff_t* other )
{
  interval_sum   += other -> interval_sum;
  uptime_sum     += other -> uptime_sum;
  up_count       += other -> up_count;
  down_count     += other -> down_count;
  interval_count += other -> interval_count;
  start_count    += other -> start_count;
}

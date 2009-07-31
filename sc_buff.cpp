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
		double             ch,
		bool               q,
		int                rng_type,
		int                a ) :
  sim(s), player(p), name_str(n),
  current_stack(0),max_stack(ms), 
  duration(d), cooldown(cd), cooldown_ready(0), default_chance(ch),
  last_start(-1), interval_sum(0), uptime_sum(0),
  up_count(0), down_count(0), interval_count(0), 
  start_count(0), refresh_count(0),
  trigger_attempts(0), trigger_successes(0),
  uptime_pct(0), benefit_pct(0), trigger_pct(0), 
  avg_interval(0), avg_start(0), avg_refresh(0),
  constant(false), quiet(q), aura_id(0), expiration(0), rng(0), next(0)
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

// buff_t::remains_gt =======================================================

bool buff_t::remains_gt( double time )
{
  double time_remaining = remains();

  if( time_remaining == 0 ) return false;

  if( time_remaining == -1 ) return true;

  return ( time_remaining > time );
}

// buff_t::remains_lt =======================================================

bool buff_t::remains_lt( double time )
{
  double time_remaining = remains();

  if( time_remaining == 0 ) return false;

  if( time_remaining == -1 ) return false;

  return ( time_remaining < time );
}

// buff_t::trigger ==========================================================

bool buff_t::trigger( int    stacks,
		      double value,
		      double chance )
{
  trigger_attempts++;

  if( cooldown_ready > 0 )
    if( sim -> current_time < cooldown_ready )
      return false;

  if( chance < 0 ) chance = default_chance;

  if( ! rng -> roll( chance ) )
    return false;

  increment( stacks, value );

  if( cooldown > 0 )
    cooldown_ready = sim -> current_time + cooldown;

  trigger_successes++;

  return true;
}

// buff_t::increment ========================================================

void buff_t::increment( int    stacks,
			double value )
{
  if( current_stack == 0 ) 
  {
    if( value < 0 ) value = 1.0;
    start( stacks, value );
  }
  else 
  {
    if( value < 0 ) value = current_value;
    refresh( stacks, value );
  }
}

// buff_t::decrement ========================================================

void buff_t::decrement( int    stacks,
			double value )
{
  if( stacks == 0 || current_stack <= stacks ) 
  {
    expire();
  }
  else 
  {
    current_stack -= stacks;
    if( value >= 0 ) current_value = value;
  }
}

// buff_t::start ============================================================

void buff_t::start( int    stacks,
		    double value )
{
  assert( ! expiration );

  if( sim -> current_time == 0 ) constant = true;

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

  current_stack = std::min( stacks, max_stack );
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
    
    expiration = new ( sim ) expiration_t( sim, player, this );
  }
}

// buff_t::refresh ==========================================================

void buff_t::refresh( int    stacks,
		      double value )
{
  refresh_count++;

  if( current_stack < max_stack )
  {
    current_stack += stacks;
    if( current_stack > max_stack )
      current_stack = max_stack;
  }

  current_value = value;

  if( duration > 0 )
  {
    assert( expiration );
    expiration -> reschedule( duration );
  }
}

// buff_t::expire ===========================================================

void buff_t::expire()
{
  if( current_stack <= 0 ) return;
  event_t::cancel( expiration );
  current_stack = 0;
  current_value = 0;
  if( player ) 
  {
    player -> aura_loss( name(), aura_id );
  }
  if( last_start >= 0 )
  {
    double current_time = player ? ( player -> current_time ) : ( sim -> current_time );
    uptime_sum += current_time - last_start;
  }
  if( sim -> current_time != 0 ) constant = false;
}

// buff_t::reset ============================================================

void buff_t::reset()
{
  cooldown_ready = 0;
  expire();
  last_start = -1;
}

// buff_t::merge ============================================================

void buff_t::merge( buff_t* other )
{
  interval_sum      += other -> interval_sum;
  uptime_sum        += other -> uptime_sum;
  up_count          += other -> up_count;
  down_count        += other -> down_count;
  interval_count    += other -> interval_count;
  start_count       += other -> start_count;
  trigger_attempts  += other -> trigger_attempts;
  trigger_successes += other -> trigger_successes;
}

// buff_t::analyze ==========================================================

void buff_t::analyze()
{
  double total_seconds = player ? player -> total_seconds : sim -> total_seconds;
  if( total_seconds > 0 ) 
  {
    uptime_pct = 100.0 * uptime_sum / total_seconds;
  }
  if( up_count > 0 ) 
  {
    benefit_pct = 100.0 * up_count / ( up_count + down_count );
  }
  if( trigger_attempts > 0 )
  {
    trigger_pct = 100.0 * trigger_successes / trigger_attempts;
  }
  if( interval_count > 0 )
  {
    avg_interval = interval_sum / interval_count;
  }
  avg_start   =   start_count / (double) sim -> iterations;
  avg_refresh = refresh_count / (double) sim -> iterations;
}

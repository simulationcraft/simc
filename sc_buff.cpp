// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// BUFF
// ==========================================================================

// buff_t::buff_t ===========================================================

buff_t::buff_t( sim_t*             s,
                const std::string& n,
                int                ms,
                double             d,
                double             cd,
                double             ch,
                bool               q,
                int                rng_type,
                int                id ) :
  sim( s ), player( 0 ), name_str( n ), aura_str( 0 ),
  current_stack( 0 ), max_stack( ms ), current_value( 0 ),
  duration( d ), cooldown( cd ), cooldown_ready( 0 ), default_chance( ch ),
  last_start( -1 ), interval_sum( 0 ), uptime_sum( 0 ),
  up_count( 0 ), down_count( 0 ), interval_count( 0 ),
  start_count( 0 ), refresh_count( 0 ),
  trigger_attempts( 0 ), trigger_successes( 0 ),
  uptime_pct( 0 ), benefit_pct( 0 ), trigger_pct( 0 ),
  avg_interval( 0 ), avg_start( 0 ), avg_refresh( 0 ),
  constant( false ), quiet( q ), aura_id( id ), expiration( 0 ), rng( 0 ), next( 0 )
{
  rng = sim -> get_rng( n, rng_type );

  buff_t** tail = &( sim -> buff_list );
  while ( *tail && name_str > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }
  next = *tail;
  *tail = this;

  char *buffer = new char[name_str.size() + 16];
  assert( buffer != NULL );

  aura_str.resize( max_stack + 1 );
  for ( int i=1; i <= max_stack; i++ )
  {
    sprintf( buffer, "%s(%d)", name_str.c_str(), i );
    aura_str[ i ] = buffer;
  }
  delete [] buffer;
}

// buff_t::buff_t ===========================================================

buff_t::buff_t( player_t*          p,
                const std::string& n,
                int                ms,
                double             d,
                double             cd,
                double             ch,
                bool               q,
                int                rng_type,
                int                id ) :
  sim( p -> sim ), player( p ), name_str( n ), aura_str( 0 ),
  current_stack( 0 ), max_stack( ms ), current_value( 0 ),
  duration( d ), cooldown( cd ), cooldown_ready( 0 ), default_chance( ch ),
  last_start( -1 ), interval_sum( 0 ), uptime_sum( 0 ),
  up_count( 0 ), down_count( 0 ), interval_count( 0 ),
  start_count( 0 ), refresh_count( 0 ),
  trigger_attempts( 0 ), trigger_successes( 0 ),
  uptime_pct( 0 ), benefit_pct( 0 ), trigger_pct( 0 ),
  avg_interval( 0 ), avg_start( 0 ), avg_refresh( 0 ),
  constant( false ), quiet( q ), aura_id( id ), expiration( 0 ), rng( 0 ), next( 0 )
{
  rng = player -> get_rng( n, rng_type );
  buff_t** tail = &(  player -> buff_list );

  while ( *tail && name_str > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }
  next = *tail;
  *tail = this;

  char *buffer = new char[name_str.size() + 16];
  assert( buffer != NULL );

  aura_str.resize( max_stack + 1 );
  for ( int i=1; i <= max_stack; i++ )
  {
    sprintf( buffer, "%s(%d)", name_str.c_str(), i );
    aura_str[ i ] = buffer;
  }
  delete [] buffer;
}

// buff_t::may_react ========================================================

bool buff_t::may_react()
{
  if ( current_stack == 0 ) return false;
  return sim -> time_to_think( last_start );
}

// buff_t::remains ==========================================================

double buff_t::remains()
{
  if ( current_stack <= 0 )
  {
    return 0;
  }
  if ( expiration )
  {
    return expiration -> occurs() - sim -> current_time;
  }
  return -1;
}

// buff_t::remains_gt =======================================================

bool buff_t::remains_gt( double time )
{
  double time_remaining = remains();

  if ( time_remaining == 0 ) return false;

  if ( time_remaining == -1 ) return true;

  return ( time_remaining > time );
}

// buff_t::remains_lt =======================================================

bool buff_t::remains_lt( double time )
{
  double time_remaining = remains();

  if ( time_remaining == -1 ) return false;

  return ( time_remaining < time );
}

// buff_t::trigger ==========================================================

bool buff_t::trigger( int    stacks,
                      double value,
                      double chance )
{
  if ( max_stack == 0 || chance == 0 ) return false;

  if ( cooldown_ready > 0 )
    if ( sim -> current_time < cooldown_ready )
      return false;

  trigger_attempts++;

  if ( chance < 0 ) chance = default_chance;

  if ( ! rng -> roll( chance ) )
    return false;

  increment( stacks, value );

  if ( cooldown > 0 )
    cooldown_ready = sim -> current_time + cooldown;

  trigger_successes++;

  return true;
}

// buff_t::increment ========================================================

void buff_t::increment( int    stacks,
                        double value )
{
  if ( max_stack == 0 ) return;

  if ( current_stack == 0 )
  {
    start( stacks, value );
  }
  else
  {
    refresh( stacks, value );
  }
}

// buff_t::decrement ========================================================

void buff_t::decrement( int    stacks,
                        double value )
{
  if ( max_stack == 0 ) return;

  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    current_stack -= stacks;
    if ( value >= 0 ) current_value = value;
  }
}

// buff_t::start ============================================================

void buff_t::start( int    stacks,
                    double value )
{
  if ( max_stack == 0 ) return;

  assert( current_stack < max_stack );

  if ( sim -> current_time <= 0.01 ) constant = true;

  start_count++;

  current_stack = std::min( stacks, max_stack );

  if ( value >= 0 ) current_value = value;

  aura_gain();

  if ( last_start >= 0 )
  {
    interval_sum += sim -> current_time - last_start;
    interval_count++;
  }
  last_start = sim -> current_time;

  if ( duration > 0 )
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
  if ( max_stack == 0 ) return;

  refresh_count++;

  if ( current_stack < max_stack )
  {
    current_stack += stacks;
    if ( current_stack > max_stack )
      current_stack = max_stack;

    aura_gain();
  }

  if ( value >= 0 ) current_value = value;

  if ( duration > 0 )
  {
    assert( expiration );
    if( expiration -> occurs() < sim -> current_time + duration )
    {
      expiration -> reschedule( duration );
    }
  }
}

// buff_t::override =========================================================

void buff_t::override( int    stacks,
		       double value )
{
  assert( max_stack > 0 );
  assert( current_stack == 0 );
  duration = 0;
  start( stacks, value );
}

// buff_t::expire ===========================================================

void buff_t::expire()
{
  if ( current_stack <= 0 ) return;
  event_t::cancel( expiration );
  current_stack = 0;
  current_value = 0;
  aura_loss();
  if ( last_start >= 0 )
  {
    double current_time = player ? ( player -> current_time ) : ( sim -> current_time );
    uptime_sum += current_time - last_start;
  }
  if ( sim -> current_time >= 0.01 &&
       sim -> target -> current_health > 0 ) 
  {
    constant = false;
  }
}

// buff_t::aura_gain ========================================================

void buff_t::aura_gain()
{
  if ( player )
  {
    player -> aura_gain( aura_str[ current_stack ].c_str(), aura_id );
  }
  else
  {
    sim -> aura_gain( aura_str[ current_stack ].c_str(), aura_id );
  }
}

// buff_t::aura_loss ========================================================

void buff_t::aura_loss()
{
  if ( player )
  {
    player -> aura_loss( name(), aura_id );
  }
  else
  {
    sim -> aura_loss( name(), aura_id );
  }
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
  refresh_count     += other -> refresh_count;
  trigger_attempts  += other -> trigger_attempts;
  trigger_successes += other -> trigger_successes;
}

// buff_t::analyze ==========================================================

void buff_t::analyze()
{
  double total_seconds = player ? player -> total_seconds : sim -> total_seconds;
  if ( total_seconds > 0 )
  {
    uptime_pct = 100.0 * uptime_sum / total_seconds;
  }
  if ( up_count > 0 )
  {
    benefit_pct = 100.0 * up_count / ( up_count + down_count );
  }
  if ( trigger_attempts > 0 )
  {
    trigger_pct = 100.0 * trigger_successes / trigger_attempts;
  }
  if ( interval_count > 0 )
  {
    avg_interval = interval_sum / interval_count;
  }
  avg_start   =   start_count / ( double ) sim -> iterations;
  avg_refresh = refresh_count / ( double ) sim -> iterations;
}

// buff_t::find =============================================================

buff_t* buff_t::find( sim_t* sim,
                      const std::string& name_str )
{
  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
    if ( name_str == b -> name() )
      return b;

  return 0;
}

// buff_t::find =============================================================

buff_t* buff_t::find( player_t* p,
                      const std::string& name_str )
{
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
    if ( name_str == b -> name() )
      return b;

  return 0;
}

// ==========================================================================
// STAT_BUFF
// ==========================================================================

// stat_buff_t::stat_buff_t =================================================

stat_buff_t::stat_buff_t( player_t*          p,
			  const std::string& n,
			  int                st,
			  double             a,
			  int                ms,
			  double             d,
			  double             cd,
			  double             ch,
			  bool               q,
			  int                rng_type,
			  int                id ) :
  buff_t( p, n, ms, d, cd, ch, q, rng_type, id ), stat(st), amount(a)
{
}

// stat_buff_t::start =======================================================

void stat_buff_t::start( int    stacks,
			 double value )
{
  if ( max_stack == 0 ) return;
  if ( value > 0 ) amount = value;
  buff_t::start( stacks );
  current_value = amount * current_stack;
  player -> stat_gain( stat, current_value );
}

// stat_buff_t::refresh =====================================================

void stat_buff_t::refresh( int    stacks,
			   double value )
{
  if ( max_stack == 0 ) return;
  if ( value > 0 ) 
  {
    if ( value < amount ) return;
    amount = value;
  }
  buff_t::refresh( stacks );
  double delta = amount * current_stack - current_value;
  if( delta > 0 )
  {
    player -> stat_gain( stat, delta );
    current_value += delta;
  }
  else assert( delta == 0 );
}

// stat_buff_t::decrement ===================================================

void stat_buff_t::decrement( int    stacks,
			     double value )
{
  if ( max_stack == 0 ) return;
  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    double delta = amount * stacks;
    player -> stat_loss( stat, delta );
    current_stack -= stacks;
    current_value -= delta;
  }
}

// stat_buff_t::expire ======================================================

void stat_buff_t::expire()
{
  if ( current_stack > 0 )
  {
    player -> stat_loss( stat, current_value );
    buff_t::expire();
  }
}

// ==========================================================================
// DEBUFF
// ==========================================================================

// debuff_t::debuff_t =======================================================

debuff_t::debuff_t( sim_t*             s,
		    const std::string& n,
		    int                ms,
		    double             d,
		    double             cd,
		    double             ch,
		    bool               q,
		    int                rng_type,
		    int                id ) :
  buff_t( s, n, ms, d, cd, ch, q, rng_type, id )
{
}

// debuff_t::aura_gain ======================================================

void debuff_t::aura_gain()
{
  sim -> target -> aura_gain( aura_str[ current_stack ].c_str(), aura_id );
}

// debuff_t::aura_loss ======================================================

void debuff_t::aura_loss()
{
  sim -> target -> aura_loss( name(), aura_id );
}


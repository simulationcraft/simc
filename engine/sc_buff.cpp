// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

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
                bool               r,
                int                rng_type,
                int                id ) :
  sim( s ), player( 0 ), name_str( n ), 
  max_stack( ms ), duration( d ), cooldown( cd ), default_chance( ch ),
  reverse( r ), constant( false ), quiet( q ), aura_id( id )
{
  init();

  rng = sim -> get_rng( n, rng_type );

  buff_t** tail = &( sim -> buff_list );
  while ( *tail && name_str > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }
  next = *tail;
  *tail = this;
}

// buff_t::buff_t ===========================================================

buff_t::buff_t( player_t*          p,
                const std::string& n,
                int                ms,
                double             d,
                double             cd,
                double             ch,
                bool               q,
                bool               r,
                int                rng_type,
                int                id ) :
  sim( p -> sim ), player( p ), name_str( n ), 
  max_stack( ms ), duration( d ), cooldown( cd ), default_chance( ch ),
  reverse( r ), constant( false), quiet( q ), aura_id( id )
{
  init();

  rng = player -> get_rng( n, rng_type );
  buff_t** tail = &(  player -> buff_list );

  while ( *tail && name_str > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }
  next = *tail;
  *tail = this;
}

// buff_t::init =============================================================

void buff_t::init()
{
  current_stack = 0;
  current_value = 0;
  cooldown_ready = 0;
  last_start = -1;
  last_trigger = -1;
  start_intervals_sum = 0;
  trigger_intervals_sum = 0;
  uptime_sum = 0;
  up_count = 0;
  down_count = 0;
  start_intervals = 0;
  trigger_intervals = 0;
  start_count = 0;
  refresh_count = 0;
  trigger_attempts = 0;
  trigger_successes = 0;
  uptime_pct = 0;
  benefit_pct = 0;
  trigger_pct = 0;
  avg_start_interval = 0;
  avg_trigger_interval = 0;
  avg_start = 0;
  avg_refresh = 0;
  constant = false;
  expiration = 0;

  if( max_stack >= 0 )
  {
    stack_occurrence.resize( max_stack + 1 );
    aura_str.resize( max_stack + 1 );

    char *buffer = new char[ name_str.size() + 16 ];

    for ( int i=1; i <= max_stack; i++ )
    {
      sprintf( buffer, "%s(%d)", name_str.c_str(), i );
      aura_str[ i ] = buffer;
    }
    delete [] buffer;
  }
}

// buff_t::may_react ========================================================

bool buff_t::may_react( int stack )
{
  if ( current_stack == 0    ) return false;
  if ( stack > current_stack ) return false;
  if ( stack < 1             ) return false;

  if( stack > max_stack ) return false;

  double occur = stack_occurrence[ stack ];

  if ( occur <= 0 ) return true;

  return sim -> time_to_think( occur );
}

// buff_t::stack_react ======================================================

int buff_t::stack_react()
{
  int stack = 0;

  for( int i=1; i <= current_stack; i++ )
  {
    if ( ! sim -> time_to_think( stack_occurrence[ i ] ) ) break;
    stack++;
  }

  return stack;
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

bool buff_t::trigger( action_t* a,
                      int       stacks,
                      double    value )
{
  double chance = default_chance;
  if ( chance < 0 ) chance = a -> ppm_proc_chance( -chance );
  return trigger( stacks, value, chance );
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

  if( last_trigger > 0 ) 
  {
    trigger_intervals_sum += sim -> current_time - last_trigger;
    trigger_intervals++;
  }
  last_trigger = sim -> current_time;

  if ( reverse )
  {
    decrement( stacks, value );
  }
  else
  {
    increment( stacks, value );
  }

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
  if ( max_stack == 0 || current_stack <= 0 ) return;

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

  assert( current_stack == 0 );

  if ( sim -> current_time <= 0.01 ) constant = true;

  start_count++;

  bump( stacks, value );

  if ( last_start >= 0 )
  {
    start_intervals_sum += sim -> current_time - last_start;
    start_intervals++;
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

  bump( stacks, value );

  if ( duration > 0 )
  {
    assert( expiration );
    if( expiration -> occurs() < sim -> current_time + duration )
    {
      expiration -> reschedule( duration );
    }
  }
}

// buff_t::bump =============================================================

void buff_t::bump( int    stacks,
                   double value )
{
  if ( max_stack == 0 ) return;

  if ( max_stack < 0 )
  {
    current_stack += stacks;
  }
  else if ( current_stack < max_stack )
  {
    int before_stack = current_stack;

    current_stack += stacks;
    if ( current_stack > max_stack )
      current_stack = max_stack;

    aura_gain();

    for( int i=before_stack+1; i <= current_stack; i++ )
    {
      stack_occurrence[ i ] = sim -> current_time;
    }
  }

  if ( value >= 0 ) current_value = value;
}

// buff_t::override =========================================================

void buff_t::override( int    stacks,
                       double value )
{
  assert( max_stack != 0 );
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
  if ( sim -> target -> initial_health == 0 ||
       sim -> target -> current_health > 0 ) 
  {
    constant = false;
  }
}

// buff_t::predict ==========================================================

void buff_t::predict()
{
  // Guarantee that may_react() will return true if the buff is present.

  for( int i=0; i <= current_stack; i++ ) 
  {
    stack_occurrence[ i ] = -1;
  }
}

// buff_t::aura_gain ========================================================

void buff_t::aura_gain()
{
  if ( sim -> log )
  {
    const char* s = ( max_stack < 0 ) ? name() : aura_str[ current_stack ].c_str();

    if ( player )
    {
      player -> aura_gain( s, aura_id );
    }
    else
    {
      sim -> aura_gain( s, aura_id );
    }
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
  last_trigger = -1;
}

// buff_t::merge ============================================================

void buff_t::merge( buff_t* other )
{
  start_intervals_sum   += other -> start_intervals_sum;
  trigger_intervals_sum += other -> trigger_intervals_sum;
  uptime_sum            += other -> uptime_sum;
  up_count              += other -> up_count;
  down_count            += other -> down_count;
  start_intervals       += other -> start_intervals;
  trigger_intervals     += other -> trigger_intervals;
  start_count           += other -> start_count;
  refresh_count         += other -> refresh_count;
  trigger_attempts      += other -> trigger_attempts;
  trigger_successes     += other -> trigger_successes;
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
  if ( start_intervals > 0 )
  {
    avg_start_interval = start_intervals_sum / start_intervals;
  }
  if ( trigger_intervals > 0 )
  {
    avg_trigger_interval = trigger_intervals_sum / trigger_intervals;
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

// buff_t::create_expression ================================================

action_expr_t* buff_t::create_expression( action_t* action,
                                          const std::string& type )
{
  if ( type == "remains" )
  {
    struct buff_remains_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_remains_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_remains", TOK_NUM ), buff(b) {}
      virtual int evaluate() { result_num = buff -> remains(); return TOK_NUM; }
    };
    return new buff_remains_expr_t( action, this );
  }
  else if ( type == "cooldown_remains" )
  {
    struct buff_cooldown_remains_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_cooldown_remains_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_cooldown_remains", TOK_NUM ), buff(b) {}
      virtual int evaluate() { result_num = buff -> cooldown_ready - buff -> sim -> current_time; return TOK_NUM; }
    };
    return new buff_cooldown_remains_expr_t( action, this );
  }
  else if ( type == "up" )
  {
    struct buff_up_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_up_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_up", TOK_NUM ), buff(b) {}
      virtual int evaluate() { result_num = ( buff -> check() > 0 ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new buff_up_expr_t( action, this );
  }
  else if ( type == "down" )
  {
    struct buff_down_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_down_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_down", TOK_NUM ), buff(b) {}
      virtual int evaluate() { result_num = ( buff -> check() <= 0 ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new buff_down_expr_t( action, this );
  }
  else if ( type == "stack" )
  {
    struct buff_stack_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_stack_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_stack", TOK_NUM ), buff(b) {}
      virtual int evaluate() { result_num = buff -> check(); return TOK_NUM; }
    };
    return new buff_stack_expr_t( action, this );
  }
  else if ( type == "react" )
  {
    struct buff_react_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_react_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_react", TOK_NUM ), buff(b) {}
      virtual int evaluate() { result_num = buff -> stack_react(); return TOK_NUM; }
    };
    return new buff_react_expr_t( action, this );
  }
  else if ( type == "cooldown_react" )
  {
    struct buff_cooldown_react_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_cooldown_react_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_cooldown_react", TOK_NUM ), buff(b) {}
      virtual int evaluate() 
      { 
        if ( buff -> check() && ! buff -> may_react() )
        {
          result_num = 0;
        }
        else
        {
          result_num = buff -> cooldown_ready - buff -> sim -> current_time;
        }
        return TOK_NUM; 
      }
    };
    return new buff_cooldown_react_expr_t( action, this );
  }

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
                          bool               r,
                          int                rng_type,
                          int                id ) :
  buff_t( p, n, ms, d, cd, ch, q, r, rng_type, id ), stat(st), amount(a)
{
}

// stat_buff_t::bump ========================================================

void stat_buff_t::bump( int    stacks,
                        double value )
{
  if ( max_stack == 0 ) return;
  if ( value > 0 ) 
  {
    if ( value < amount ) return;
    amount = value;
  }
  buff_t::bump( stacks );
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
                    bool               r,
                    int                rng_type,
                    int                id ) :
  buff_t( s, n, ms, d, cd, ch, q, r, rng_type, id )
{
}

// debuff_t::aura_gain ======================================================

void debuff_t::aura_gain()
{
  if ( sim -> log )
  {
    const char* s = ( max_stack < 0 ) ? name() : aura_str[ current_stack ].c_str();

    sim -> target -> aura_gain( s, aura_id );
  }
}

// debuff_t::aura_loss ======================================================

void debuff_t::aura_loss()
{
  sim -> target -> aura_loss( name(), aura_id );
}


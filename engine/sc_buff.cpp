// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace
{
struct expiration_t : public event_t
{
  buff_t* buff;
  double duration;
  expiration_t( sim_t* sim, player_t* p, buff_t* b, double d ) : event_t( sim, p ), buff( b ),duration( d )
  {
    name = buff -> name();
    sim -> add_event( this, duration );
  }
  virtual void execute()
  {
    buff -> expiration = 0;
    buff -> expire();
  }
};

struct buff_delay_t : public event_t
{
  buff_t* buff;
  int     stacks;
  double  value;

  buff_delay_t( sim_t* sim, player_t* p, buff_t* b, int stacks, double value ) :
    event_t( sim, p ), buff( b ), stacks( stacks ), value( value )
  {
    name = buff -> name();
    double delay_duration = sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev );

    sim -> add_event( this, delay_duration );
  }

  virtual void execute()
  {
    // Add a Cooldown check here to avoid extra processing due to delays
    if ( buff -> cooldown -> remains() ==  0 )
      buff -> execute( stacks, value );
    buff -> delay = 0;
  }
};
}

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
                int                rt,
                int                id ) :
  spell_id_t( 0, n.c_str() ),
  sim( s ), player( 0 ), source( 0 ), initial_source( 0 ), name_str( n ),
  max_stack( ms ), activated( true ), buff_duration( d ), buff_cooldown( cd ), default_chance( ch ),
  reverse( r ), constant( false ), quiet( q ), aura_id( id ), rng_type( rt )
{
  init();
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
                int                rt,
                int                id,
                bool               act ) :
  spell_id_t( p, n.c_str() ),
  sim( p -> sim ), player( p ), source( p ), initial_source( p ), name_str( n ),
  max_stack( ms ), activated( act ), buff_duration( d ), buff_cooldown( cd ), default_chance( ch ),
  reverse( r ), constant( false ), quiet( q ), aura_id( id ), rng_type( rt )
{
  init();
}

// buff_t::init_buff_from_talent_ ===========================================

void buff_t::init_from_talent_( player_t* /* p */, talent_t* talent )
{
  if( talent -> rank() )
  {
    default_chance = talent -> sd -> proc_chance();
    if( default_chance <= 0 ) default_chance = 1.0;

    spell_data_t* spell = talent -> trigger ? talent -> trigger : talent -> sd;

    max_stack = std::max( ( int ) spell -> max_stacks(), 1 );
    buff_duration = spell -> duration();
    buff_cooldown = spell -> cooldown();
    aura_id = spell -> id();
  }
}

// buff_t::buff_t ===========================================================

buff_t::buff_t( player_t* p,
                talent_t* talent, ... ) :
  spell_id_t( p, talent -> trigger ? talent -> trigger -> name_cstr() : talent -> td -> name_cstr() ),
  sim( p -> sim ), player( p ), source( p ), initial_source( p ), name_str( s_token ),
  max_stack( 0 ), activated( true ), buff_duration( 0 ), buff_cooldown( 0 ), default_chance( 0 ),
  reverse( false ), constant( false ), quiet( false ), rng_type( RNG_CYCLIC )
{
  init_from_talent_( p, talent );

  va_list vap;
  va_start( vap, talent );
  parse_options( vap );

  init();
}

// buff_t::buff_t ===========================================================

buff_t::buff_t( player_t* p,
                talent_t* talent ) :
  spell_id_t( p, talent -> trigger ? talent -> trigger -> name_cstr() : talent -> td -> name_cstr() ),
  sim( p -> sim ), player( p ), source( p ), initial_source( p ), name_str( s_token ),
  max_stack( 0 ), activated( true ), buff_duration( 0 ), buff_cooldown( 0 ), default_chance( 0 ),
  reverse( false ), constant( false ), quiet( false ), rng_type( RNG_CYCLIC )
{
  init_from_talent_( p, talent );
  init();
}

// buff_t::init_from_spell_ =================================================

void buff_t::init_from_spell_( player_t* /* p */, spell_data_t* spell )
{
  max_stack = std::max( ( int ) spell -> max_stacks(), 1 );
  default_chance = spell -> proc_chance() ? spell -> proc_chance() : 1.0;
  buff_duration = spell -> duration();
  buff_cooldown = spell -> cooldown();
  aura_id = spell -> id();
}

// buff_t::buff_t ===========================================================

buff_t::buff_t( player_t*     p,
                spell_data_t* spell, ... ) :
  spell_id_t( p, spell -> name_cstr(), spell -> id() ),
  sim( p -> sim ), player( p ), source( p ), initial_source( p ), name_str( s_token ),
  max_stack( 0 ), activated( true ), buff_duration( 0 ), buff_cooldown( 0 ), default_chance( 0 ),
  reverse( false ), constant( false ), quiet( false ), rng_type( RNG_CYCLIC )
{
  init_from_spell_( p, spell );

  va_list vap;
  va_start( vap, spell );
  parse_options( vap );

  init();
}

// buff_t::buff_t ===========================================================

buff_t::buff_t( player_t*     p,
                spell_data_t* spell ) :
  spell_id_t( p, spell -> name_cstr(), spell -> id() ),
  sim( p -> sim ), player( p ), source( p ), initial_source( p ), name_str( s_token ),
  max_stack( 0 ), activated( true ), buff_duration( 0 ), buff_cooldown( 0 ), default_chance( 0 ),
  reverse( false ), constant( false ), quiet( false ), rng_type( RNG_CYCLIC )
{
  init_from_spell_( p, spell );
  init();
}

// buff_t::parse_options ====================================================

void buff_t::parse_options( va_list vap )
{
  while ( true )
  {
    const char* parm = ( const char * ) va_arg( vap, const char * );
    if( ! parm ) break;

    if( ! strcmp( parm, "stacks" ) )
    {
      max_stack = ( int ) va_arg( vap, int );
    }
    else if( ! strcmp( parm, "chance" ) )
    {
      default_chance = ( double ) va_arg( vap, double );
    }
    else if( ! strcmp( parm, "duration" ) )
    {
      buff_duration = ( double ) va_arg( vap, double );
    }
    else if( ! strcmp( parm, "cooldown" ) )
    {
      buff_cooldown = ( double ) va_arg( vap, double );
    }
    else if( ! strcmp( parm, "reverse" ) )
    {
      reverse = va_arg( vap, int ) ? true : false;
    }
    else if( ! strcmp( parm, "quiet" ) )
    {
      quiet = va_arg( vap, int ) ? true : false;
    }
    else if( ! strcmp( parm, "rng" ) )
    {
      rng_type = ( int ) va_arg( vap, int );
    }
  }
  va_end( vap );
}

// buff_t::init =============================================================

void buff_t::init()
{
  current_stack = 0;
  current_value = 0;
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
  overridden = false;
  expiration = 0;
  delay = 0;

  buff_t** tail=0;

  if( player )
  {
    cooldown = player -> get_cooldown( "buff_" + name_str );
    rng = player -> get_rng( name_str, rng_type );
    tail = &( player -> buff_list );
  }
  else
  {
    cooldown = sim -> get_cooldown( "buff_" + name_str );
    rng = sim -> get_rng( name_str, rng_type );
    tail = &( sim -> buff_list );
  }

  buff_duration        = std::min( buff_duration, sim -> wheel_seconds - 2.0 );
  cooldown -> duration = std::min( buff_cooldown, sim -> wheel_seconds - 2.0 );

  while ( *tail && name_str > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }
  next = *tail;
  *tail = this;

  if( max_stack >= 0 )
  {
    stack_occurrence.resize( max_stack + 1 );
    stack_react_time.resize( max_stack + 1 );
    aura_str.resize( max_stack + 1 );

    std::vector<char> buffer( name_str.size() + 16 );
    for ( int i=0; i <= max_stack; i++ )
    {
      snprintf( &buffer[ 0 ], buffer.size(), "%s_%d", name_str.c_str(), i );
      stack_uptime.push_back( new uptime_t( sim, &buffer[ 0 ] ) );
    }

    for ( int i=1; i <= max_stack; i++ )
    {
      snprintf( &buffer[ 0 ], buffer.size(), "%s(%d)", name_str.c_str(), i );
      aura_str[ i ] = &buffer[ 0 ];
    }
  }
}

// buff_t::buff_t ===========================================================

buff_t::buff_t( player_t*          p,
                const std::string& n,
                const char*        sname,
                double             chance,
                double             cd,
                bool               q,
                bool               r,
                int                rt,
                bool               act ) :
  spell_id_t( p, n.c_str(), sname, 0 ),
  sim( p -> sim ), player( p ), source( 0 ), initial_source( 0 ), name_str( n ),
  max_stack( ( max_stacks()!=0 ) ? max_stacks() : ( initial_stacks() != 0 ? initial_stacks() : 1 ) ),
  activated( act ),
  buff_duration( ( duration() > ( p -> sim -> wheel_seconds - 2.0 ) ) ?  ( p -> sim -> wheel_seconds - 2.0 ) : duration() ),
  default_chance( ( chance != -1 ) ? chance : ( ( proc_chance() != 0 ) ? proc_chance() : 1.0 ) ) ,
  reverse( r ), constant( false ), quiet( q ), aura_id( 0 ), rng_type( rt )
{
  init_buff_t_();

  cooldown = player -> get_cooldown( "buff_" + name_str );
  if ( cd < 0.0 )
  {
    cooldown -> duration = p -> dbc.spell( spell_id() ) -> cooldown();
  }
  else
  {
    cooldown -> duration = cd;
  }

  if ( cooldown -> duration > ( sim -> wheel_seconds - 2.0 ) )
    cooldown -> duration = sim -> wheel_seconds - 2.0;

  rng = player -> get_rng( n, rng_type );
  buff_t** tail = &(  player -> buff_list );

  while ( *tail && name_str > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }
  next = *tail;
  *tail = this;

  if ( sim -> debug )
    log_t::output( sim, "Buff Spell status: %s", to_str().c_str() );
}

// buff_t::buff_t ===========================================================

buff_t::buff_t( player_t*          p,
                const uint32_t     id,
                const std::string& n,
                double             chance,
                double             cd,
                bool               q,
                bool               r,
                int                rt,
                bool               act ) :
  spell_id_t( p, n.c_str(), id ),
  sim( p -> sim ), player( p ), source( 0 ), initial_source( 0 ), name_str( n ),
  max_stack( ( max_stacks()!=0 ) ? max_stacks() : ( initial_stacks() != 0 ? initial_stacks() : 1 ) ),
  activated( act ),
  buff_duration( ( duration() > ( p -> sim -> wheel_seconds - 2.0 ) ) ?  ( p -> sim -> wheel_seconds - 2.0 ) : duration() ),
  default_chance( ( chance != -1 ) ? chance : ( ( proc_chance() != 0 ) ? proc_chance() : 1.0 ) ) ,
  reverse( r ), constant( false ), quiet( q ), aura_id( 0 ), rng_type( rt )
{
  init_buff_t_();

  cooldown = player -> get_cooldown( "buff_" + name_str );
  if ( cd < 0.0 )
  {
    cooldown -> duration = p -> dbc.spell( spell_id() ) -> cooldown();
  }
  else
  {
    cooldown -> duration = cd;
  }

  if ( cooldown -> duration > ( sim -> wheel_seconds - 2.0 ) )
    cooldown -> duration = sim -> wheel_seconds - 2.0;

  rng = player -> get_rng( n, rng_type );
  buff_t** tail = &(  player -> buff_list );

  while ( *tail && name_str > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }
  next = *tail;
  *tail = this;

  if ( sim -> debug )
    log_t::output( sim, "Buff Spell status: %s", to_str().c_str() );
}

// buff_t::init_buff_t_ =====================================================

void buff_t::init_buff_t_()
{
  // FIXME! For the love of all that is holy.... FIXME!
  // This routine will disappear once data-access rework is complete

  current_stack = 0;
  current_value = 0;
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
  overridden = false;
  expiration = 0;
  delay = 0;

  if( max_stack >= 0 )
  {
    stack_occurrence.resize( max_stack + 1 );
    stack_react_time.resize( max_stack + 1 );
    aura_str.resize( max_stack + 1 );

    std::vector<char> buffer( name_str.size() + 16 );
    stack_uptime.clear();
    for ( int i=0; i <= max_stack; i++ )
    {
      snprintf( &buffer[ 0 ], buffer.size(), "%s_%d", name_str.c_str(), i );
      stack_uptime.push_back( new uptime_t( sim, &buffer[ 0 ] ) );
    }

    for ( int i=1; i <= max_stack; i++ )
    {
      sprintf( &buffer[ 0 ], "%s(%d)", name_str.c_str(), i );
      aura_str[ i ] = &buffer[ 0 ];
    }
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

  return sim -> current_time  > stack_react_time[ stack ];
}

// buff_t::stack_react ======================================================

int buff_t::stack_react()
{
  int stack = 0;

  for( int i=1; i <= current_stack; i++ )
  {
    if ( stack_react_time[ i ] > sim -> current_time ) break;
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

  if ( cooldown -> remains() > 0 )
    return false;

  if ( player && player -> sleeping )
    return false;

  trigger_attempts++;

  if ( chance < 0 ) chance = default_chance;

  if ( ! rng -> roll( chance ) )
    return false;

  if ( ! activated && player && player -> in_combat && sim -> default_aura_delay > 0 )
  {
    // In-game, procs that happen "close to eachother" are usually delayed into the
    // same time slot. We roughly model this by allowing procs that happen during the
    // buff's already existing delay period to trigger at the same time as the first
    // delayed proc will happen.
    if ( delay )
    {
      buff_delay_t* d = dynamic_cast< buff_delay_t* >( delay );
      d -> stacks += stacks;
      d -> value = value;
    }
    else
      delay = new ( sim ) buff_delay_t( sim, player, this, stacks, value );
  }
  else
    execute( stacks, value );

  return true;
}

void buff_t::execute( int stacks, double value )
{
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

  // new buff cooldown impl
  if ( cooldown -> duration > 0 )
  {
    if ( sim -> debug )
      log_t::output( sim, "%s starts buff %s cooldown (%s) with duration %.2f",
                     ( source ? source -> name() : "someone" ), name(), cooldown -> name(), cooldown -> duration );

    cooldown -> start();
  }

  trigger_successes++;
}

// buff_t::increment ========================================================

void buff_t::increment( int    stacks,
                        double value )
{
  if ( overridden ) return;

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
  if ( overridden ) return;

  if ( max_stack == 0 || current_stack <= 0 ) return;

  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    current_stack -= stacks;
    if ( value >= 0 ) current_value = value;

    for ( unsigned int i = 0; i < stack_uptime.size(); i++ )
    {
      if ( ( int ) i != current_stack )
        stack_uptime[ i ] -> update_uptime( false );
      else
        stack_uptime[ i ] -> update_uptime( true );
    }

    if ( sim -> debug )
      log_t::output( sim, "buff %s decremented by %d to %d stacks",
                     name(), stacks, current_stack );
  }
}

// buff_t::extend_duration ==================================================

void buff_t::extend_duration( player_t* p, double extra_seconds )
{
  assert( expiration );
  assert( extra_seconds < sim -> wheel_seconds );

  if ( extra_seconds > 0 )
  {
    expiration -> reschedule( expiration -> remains() + extra_seconds );

    if ( sim -> debug )
      log_t::output( sim, "%s extends buff %s by %.1f seconds. New expiration time: %.1f",
                     p -> name(), name(), extra_seconds, expiration -> occurs() );
  }
  else if ( extra_seconds < 0 )
  {
    double reschedule_time = expiration -> remains() + extra_seconds;

    if ( reschedule_time <= 0 )
    {
      // When Strength of Soul removes the Weakened Soul debuff completely,
      // there's a delay before the server notifies the client. Modeling
      // this effect as a world lag.
      double lag, dev;

      lag = p -> world_lag_override ? p -> world_lag : sim -> world_lag;
      dev = p -> world_lag_stddev_override ? p -> world_lag_stddev : sim -> world_lag_stddev;
      reschedule_time = p -> rngs.lag_world -> gauss( lag, dev );
    }

    event_t::cancel( expiration );

    expiration = new ( sim ) expiration_t( sim, player, this, reschedule_time );

    if ( sim -> debug )
      log_t::output( sim, "%s decreases buff %s by %.1f seconds. New expiration time: %.1f",
                     p -> name(), name(), extra_seconds, expiration -> occurs() );
  }
}

// buff_t::start ============================================================

void buff_t::start( int    stacks,
                    double value )
{
  if ( max_stack == 0 ) return;

  if ( current_stack != 0 )
  {
    sim -> errorf( "buff_t::start assertion error current_stack is not zero, buff %s from %s.\n", name(), player -> name() );
    assert( 0 );
  }

  if ( sim -> current_time <= 0.01 ) constant = true;

  start_count++;

  bump( stacks, value );

  if ( last_start >= 0 )
  {
    start_intervals_sum += sim -> current_time - last_start;
    start_intervals++;
  }
  last_start = sim -> current_time;

  if ( buff_duration > 0 )
  {

    expiration = new ( sim ) expiration_t( sim, player, this, buff_duration );
  }
}

// buff_t::refresh ==========================================================

void buff_t::refresh( int    stacks,
                      double value )
{
  if ( max_stack == 0 ) return;

  refresh_count++;

  bump( stacks, value );

  if ( buff_duration > 0 )
  {
    assert( expiration );
    if( expiration -> occurs() < sim -> current_time + buff_duration )
    {
      expiration -> reschedule( buff_duration );
    }
  }
}

// buff_t::bump =============================================================

void buff_t::bump( int    stacks,
                   double value )
{
  if ( max_stack == 0 ) return;

  if ( value >= 0 ) current_value = value;

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

    for ( unsigned int i = 0; i < stack_uptime.size(); i++ )
    {
      if ( ( int ) i != current_stack )
        stack_uptime[ i ] -> update_uptime( false );
      else
        stack_uptime[ i ] -> update_uptime( true );
    }

    aura_gain();

    for( int i=before_stack+1; i <= current_stack; i++ )
    {
      stack_occurrence[ i ] = sim -> current_time;
      stack_react_time[ i ] = sim -> current_time + ( player ? ( player -> total_reaction_time() ) : sim -> reaction_time );
    }
  }
}

// buff_t::override =========================================================

void buff_t::override( int    stacks,
                       double value )
{
  if( max_stack == 0 ) return;
  if ( current_stack != 0 )
  {
    sim -> errorf( "buff_t::override assertion error current_stack is not zero, buff %s from %s.\n", name(), player -> name() );
    assert( 0 );
  }

  buff_duration = 0;
  start( stacks, value );
  overridden = true;
}

// buff_t::expire ===========================================================

void buff_t::expire()
{
  if ( current_stack <= 0 ) return;
  event_t::cancel( expiration );
  source = 0;
  current_stack = 0;
  current_value = 0;
  aura_loss();
  if ( last_start >= 0 )
  {
    double current_time = player ? ( player -> current_time ) : ( sim -> current_time );
    uptime_sum += current_time - last_start;
  }

  if ( sim -> target -> resource_base[ RESOURCE_HEALTH ] == 0 ||
       sim -> target -> resource_current[ RESOURCE_HEALTH ] > 0 )
    if ( ! overridden )
    {
      constant = false;
    }

  for ( unsigned int i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ] -> update_uptime( false );
}

// buff_t::predict ==========================================================

void buff_t::predict()
{
  // Guarantee that may_react() will return true if the buff is present.

  for( int i=0; i <= current_stack; i++ )
  {
    stack_occurrence[ i ] = -1;
    stack_react_time[ i ] = -1;
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
      player -> aura_gain( s, current_value );
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
    player -> aura_loss( name(), current_value );
  }
  else
  {
    sim -> aura_loss( name(), aura_id );
  }
}

// buff_t::reset ============================================================

void buff_t::reset()
{
  event_t::cancel( delay );
  cooldown -> reset();
  expire();
  last_start = -1;
  last_trigger = -1;
  uptime_sum = 0;

  for ( unsigned int i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ] -> reset();
}

// buff_t::merge ============================================================

void buff_t::merge( buff_t* other )
{
  start_intervals_sum   += other -> start_intervals_sum;
  trigger_intervals_sum += other -> trigger_intervals_sum;
  uptime_pct            += other -> uptime_pct;
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
  uptime_pct  /= ( double ) sim -> iterations;

  for ( unsigned int i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ] -> analyze();
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

std::string buff_t::to_str() SC_CONST
{
  std::ostringstream s;

  s << spell_id_t::to_str();
  s << " max_stack=" << max_stack;
  s << " initial_stack=" << initial_stacks();
  s << " cooldown=" << cooldown -> duration;
  s << " duration=" << buff_duration;
  s << " default_chance=" << default_chance;

  return s.str();
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
      buff_remains_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_remains", TOK_NUM ), buff( b ) {}
      virtual int evaluate() { result_num = buff -> remains(); return TOK_NUM; }
    };
    return new buff_remains_expr_t( action, this );
  }
  else if ( type == "cooldown_remains" )
  {
    struct buff_cooldown_remains_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_cooldown_remains_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_cooldown_remains", TOK_NUM ), buff( b ) {}
      virtual int evaluate() { result_num = buff -> cooldown -> remains(); return TOK_NUM; }
    };
    return new buff_cooldown_remains_expr_t( action, this );
  }
  else if ( type == "up" )
  {
    struct buff_up_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_up_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_up", TOK_NUM ), buff( b ) {}
      virtual int evaluate() { result_num = ( buff -> check() > 0 ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new buff_up_expr_t( action, this );
  }
  else if ( type == "down" )
  {
    struct buff_down_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_down_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_down", TOK_NUM ), buff( b ) {}
      virtual int evaluate() { result_num = ( buff -> check() <= 0 ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new buff_down_expr_t( action, this );
  }
  else if ( type == "stack" )
  {
    struct buff_stack_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_stack_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_stack", TOK_NUM ), buff( b ) {}
      virtual int evaluate() { result_num = buff -> check(); return TOK_NUM; }
    };
    return new buff_stack_expr_t( action, this );
  }
  else if ( type == "value" )
  {
    struct buff_value_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_value_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_value", TOK_NUM ), buff( b ) {}
      virtual int evaluate() { result_num = buff -> value(); return TOK_NUM; }
    };
    return new buff_value_expr_t( action, this );
  }
  else if ( type == "react" )
  {
    struct buff_react_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_react_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_react", TOK_NUM ), buff( b ) {}
      virtual int evaluate() { result_num = buff -> stack_react(); return TOK_NUM; }
    };
    return new buff_react_expr_t( action, this );
  }
  else if ( type == "cooldown_react" )
  {
    struct buff_cooldown_react_expr_t : public action_expr_t
    {
      buff_t* buff;
      buff_cooldown_react_expr_t( action_t* a, buff_t* b ) : action_expr_t( a, "buff_cooldown_react", TOK_NUM ), buff( b ) {}
      virtual int evaluate()
      {
        if ( buff -> check() && ! buff -> may_react() )
        {
          result_num = 0;
        }
        else
        {
          result_num = buff -> cooldown -> remains();
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
                          int                id,
                          bool               act ) :
  buff_t( p, n, ms, d, cd, ch, q, r, rng_type, id, act ), stat( st ), amount( a )
{
}

// stat_buff_t::stat_buff_t =================================================

stat_buff_t::stat_buff_t( player_t*          p,
                          const uint32_t     id,
                          const std::string& n,
                          int                st,
                          double             a,
                          double             ch,
                          double             cd,
                          bool               q,
                          bool               r,
                          int                rng_type,
                          bool               act ) :
  buff_t( p, id, n, ch, cd, q, r, rng_type, act ), stat( st ), amount( a )
{
}

// stat_buff_t::bump ========================================================

void stat_buff_t::bump( int    stacks,
                        double value )
{
  if ( max_stack == 0 ) return;
  if ( value > 0 )
  {
    amount = value;
  }
  buff_t::bump( stacks );
  double delta = amount * current_stack - current_value;
  if( delta > 0 )
  {
    player -> stat_gain( stat, delta, 0, 0, buff_duration > 0 );
    current_value += delta;
  }
  else assert( delta == 0 );
}

// stat_buff_t::decrement ===================================================

void stat_buff_t::decrement( int    stacks,
                             double /* value */ )
{
  if ( max_stack == 0 ) return;
  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    double delta = amount * stacks;
    player -> stat_loss( stat, delta, 0, buff_duration > 0 );
    current_stack -= stacks;
    current_value -= delta;
  }
}

// stat_buff_t::expire ======================================================

void stat_buff_t::expire()
{
  if ( current_stack > 0 )
  {
    player -> stat_loss( stat, current_value, 0, buff_duration > 0 );
    buff_t::expire();
  }
}


// ==========================================================================
// COST_REDUCTION_BUFF
// ==========================================================================

// cost_reduction_buff_t::cost_reduction_buff_t =============================

cost_reduction_buff_t::cost_reduction_buff_t( player_t*          p,
                                              const std::string& n,
                                              int                sch,
                                              double             a,
                                              int                ms,
                                              double             d,
                                              double             cd,
                                              double             ch,
                                              bool               re,
                                              bool               q,
                                              bool               r,
                                              int                rng_type,
                                              int                id,
                                              bool               act ) :
  buff_t( p, n, ms, d, cd, ch, q, r, rng_type, id, act ), school( sch ), amount( a ), refreshes( re )
{
}

// cost_reduction_buff_t::cost_reduction_buff_t =============================

cost_reduction_buff_t::cost_reduction_buff_t( player_t*          p,
                                              const uint32_t     id,
                                              const std::string& n,
                                              int                sch,
                                              double             a,
                                              double             ch,
                                              double             cd,
                                              bool               re,
                                              bool               q,
                                              bool               r,
                                              int                rng_type,
                                              bool               act ) :
  buff_t( p, id, n, ch, cd, q, r, rng_type, act ), school( sch ), amount( a ), refreshes( re )
{
}

// cost_reduction_buff_t::bump ==============================================

void cost_reduction_buff_t::bump( int    stacks,
                                  double value )
{
  if ( max_stack == 0 ) return;
  if ( value > 0 )
  {
    amount = value;
  }
  buff_t::bump( stacks );
  double delta = amount * current_stack - current_value;
  if( delta > 0 )
  {
    player -> cost_reduction_gain( school, delta );
    current_value += delta;
  }
  else assert( delta == 0 );
}

// cost_reduction_buff_t::decrement =========================================

void cost_reduction_buff_t::decrement( int    stacks,
                                       double /* value */ )
{
  if ( max_stack == 0 ) return;
  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    double delta = amount * stacks;
    player -> cost_reduction_loss( school, delta );
    current_stack -= stacks;
    current_value -= delta;
  }
}

// cost_reduction_buff_t::expire ============================================

void cost_reduction_buff_t::expire()
{
  if ( current_stack > 0 )
  {
    player -> cost_reduction_loss( school, current_value );
    buff_t::expire();
  }
}

// cost_reduction_buff_t::refresh ===========================================

void cost_reduction_buff_t::refresh( int    stacks,
                                     double value )
{
  if ( ! refreshes )
  {
    if ( max_stack == 0 ) return;

    refresh_count++;

    bump( stacks, value );
    return;
  }

  buff_t::refresh( stacks, value );
}

// ==========================================================================
// DEBUFF
// ==========================================================================

// debuff_t::debuff_t =======================================================

debuff_t::debuff_t( player_t*          p,
                    const std::string& n,
                    int                ms,
                    double             d,
                    double             cd,
                    double             ch,
                    bool               q,
                    bool               r,
                    int                rng_type,
                    int                id ) :
  buff_t( p, n, ms, d, cd, ch, q, r, rng_type, id )

{
}

debuff_t::debuff_t( player_t*          p,
                    const uint32_t     id,
                    const std::string& n,
                    double             chance,
                    double             cd,
                    bool               q,
                    bool               r,
                    int                rt ) :
  buff_t( p, id, n, chance, cd, q, r, rt )

{
}


// ==========================================================================
// Generic Passive Buff system
// ==========================================================================

new_buff_t::new_buff_t( player_t*          p,
                        const std::string& n,
                        uint32_t           id,
                        double             override_chance,
                        bool               quiet,
                        bool               reverse,
                        int                rt ) :
  buff_t( p, n, 1, 0, 0, override_chance, quiet, reverse, rt, id ),
  default_stack_charge( 1 ), single( 0 )
{
  uint32_t effect_id;
  int        effects = 0;

  memset( e_data, 0, sizeof( e_data ) );
  const spell_data_t* spell = p -> dbc.spell( id );

  // Find some stuff for the buff, generically
  if ( id > 0 && spell )
  {
    max_stack              = spell -> max_stacks();

    buff_duration          = spell -> duration();
    if ( buff_duration > ( player -> sim -> wheel_seconds - 2.0 ) )
      buff_duration        = player -> sim -> wheel_seconds - 2.0;

    cooldown -> duration   = spell -> cooldown();
    if ( cooldown -> duration > ( player -> sim -> wheel_seconds - 2.0 ) )
      cooldown -> duration = player -> sim -> wheel_seconds - 2.0;

    default_stack_charge   = spell -> initial_stacks();

    if ( override_chance == 0 )
    {
      default_chance       = spell -> proc_chance();

      if ( default_chance == 0.0 )
        default_chance     = 1.0;
    }

    // If there's no max stack, set max stack to proc charges
    if ( max_stack == 0 )
      max_stack            = spell -> initial_stacks();

    // A duration of < 0 in blizzard speak is infinite -> change to 0
    if ( buff_duration < 0 )
      buff_duration        = 0.0;

    // Some spells seem to have a max_stack but no initial stacks, so set
    // initial stacks to 1 if it is 0 in spell data
    if ( default_stack_charge == 0 )
      default_stack_charge = 1;

    // Some spells do not have a max stack, but we need to at least have
    // a single application for a buff. Set the max stack (if 0) to the
    // initial stack value
    if ( max_stack == 0 )
      max_stack            = default_stack_charge;

    // Map effect data for this buff
    for ( int i = 1; i <= MAX_EFFECTS; i++ )
    {
      if ( ! ( effect_id = spell -> effect_id( i ) ) )
        continue;

      if ( i == 1 )
        e_data[ i - 1 ] = spell -> _effect1;
      else if ( i == 2 )
        e_data[ i - 1 ] = spell -> _effect2;
      else if ( i == 3 )
        e_data[ i - 1 ] = spell -> _effect3;

      // Trigger spells will not be used in anything for now
      if ( e_data[ i - 1 ] -> trigger_spell_id() > 0 )
        continue;

      effects++;
    }

    // Optimize if there's only a single effect to use
    if ( effects < 2 )
    {
      for ( int i = 0; i < MAX_EFFECTS; i++ )
      {
        if ( ! e_data[ i ] || e_data[ i ] -> trigger_spell_id() > 0 )
          continue;

        single = e_data[ i ];
        break;
      }
    }

    if ( sim -> debug )
    {
      log_t::output( sim, "Initializing New Buff %s id=%d max_stack=%d default_charges=%d cooldown=%.2f duration=%.2f default_chance=%.2f atomic=%s",
                     name_str.c_str(), id, max_stack, default_stack_charge, cooldown -> duration, buff_duration, default_chance,
                     single ? "true" : "false" );
    }
  }

  init_buff_t_();
}

bool new_buff_t::trigger( int stacks, double value, double chance )
{
  // For buffs going up, use the default_stack_charge as the amount
  return buff_t::trigger( stacks == -1 ? default_stack_charge : stacks, value, chance );
}

double new_buff_t::base_value( effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) SC_CONST
{
  if ( single )
    return dbc_t::fmt_value( single -> base_value(), single -> type(), single -> subtype() );

  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ! e_data[ i ] )
      continue;

    if ( ( type == E_MAX || e_data[ i ] -> type() == type ) &&
         ( sub_type == A_MAX || e_data[ i ] -> subtype() == sub_type ) &&
         ( misc_value == DEFAULT_MISC_VALUE || e_data[ i ] -> misc_value1() == misc_value ) &&
         ( misc_value2 == DEFAULT_MISC_VALUE || e_data[ i ] -> misc_value2() == misc_value2 ) )
      return dbc_t::fmt_value( e_data[ i ] -> base_value(), type, sub_type );
  }

  return 0.0;
}


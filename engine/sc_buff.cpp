// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct react_ready_trigger_t : public event_t
{
  unsigned stack;
  buff_t* buff;

  react_ready_trigger_t( player_t* p, buff_t* b, unsigned s, timespan_t d ) :
    event_t( p -> sim, p, "react_ready_trigger" ), stack( s ), buff( b )
  { sim -> add_event( this, d ); }

  void execute()
  {
    buff -> stack_react_ready_triggers[ stack ] = 0;
    player -> trigger_ready();
  }
};

struct expiration_t : public event_t
{
  buff_t* buff;

  expiration_t( sim_t* sim, player_t* p, buff_t* b, timespan_t d ) : event_t( sim, p, b -> name_str.c_str() ), buff( b )
  { sim -> add_event( this, d ); }

  virtual void execute()
  {
    buff -> expiration = 0;
    buff -> expire();
  }
};

struct buff_delay_t : public event_t
{
  double     value;
  buff_t*    buff;
  int        stacks;
  timespan_t duration;

  buff_delay_t( sim_t* sim, player_t* p, buff_t* b, int stacks, double value, const timespan_t& d ) :
    event_t( sim, p, b -> name_str.c_str() ), value( value ), buff( b ), stacks( stacks ), duration( d )
  {
    timespan_t delay_duration = sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev );
    sim -> add_event( this, delay_duration );
  }

  virtual void execute()
  {
    // Add a Cooldown check here to avoid extra processing due to delays
    if ( buff -> cooldown -> remains() ==  timespan_t::zero() )
      buff -> execute( stacks, value );
    buff -> delay = 0;
  }
};

stat_e translate_stat_buff_misc_number( int x )
{
  switch ( x )
  {
  case 3355443:
  case 33554432:
    return STAT_MASTERY_RATING;
  case 1792:
    return STAT_CRIT_RATING;
  case 917504:
  case 393216: // melee and ranged haste
    return STAT_HASTE_RATING;
  case 1:
    return STAT_AGILITY;
  case 3:
    return STAT_INTELLECT;

  default: break;
  }

  return STAT_NONE;
}
}

buff_t::buff_t( const buff_creation::buff_creator_basics_t& params ) :
  sim( params._sim ),
  player( params._player.target ),
  name_str( params._name ),
  s_data( params.s_data ),
  _max_stack( 1 ),
  default_value( DEFAULT_VALUE() ),
  activated( true ),
  reactable( false ),
  reverse(),
  constant(),
  quiet(),
  overridden(),
  current_value(),
  current_stack(),
  buff_duration( timespan_t() ),
  buff_cooldown( timespan_t() ),
  default_chance( 1.0 ),
  last_start( timespan_t() ),
  last_trigger( timespan_t() ),
  iteration_uptime_sum( timespan_t() ),
  up_count(),
  down_count(),
  start_count(),
  refresh_count(),
  trigger_attempts(),
  trigger_successes(),
  benefit_pct(),
  trigger_pct(),
  avg_start(),
  avg_refresh(),
  source( params._player.source ),
  // FIXME!  It would be great to get rid of "initial_source" and actually
  // understand if a (de)buff is shared or not.
  initial_source( params._player.source ),
  expiration(),
  delay(),
  rng(),
  cooldown(),
  uptime_pct(),
  start_intervals(),
  trigger_intervals()
{
  if ( name_str.empty() )
  {
    assert( data().ok() );

    name_str = dbc_t::get_token( data().id() );

    if ( name_str.empty() )
    {
      name_str = data().name_cstr();
      util::tokenize( name_str );
      assert( ! name_str.empty() );
      dbc_t::add_token( data().id(), name_str );
    }
  }
  else
  {
    util::tokenize( name_str );
  }

  // Set Buff duration
  if ( params._duration == timespan_t::min() )
  {
    if ( data().ok() )
      buff_duration = data().duration();
  }
  else
    buff_duration = params._duration;

  if ( buff_duration < timespan_t::zero() )
  {
    buff_duration = timespan_t::zero();
  }

  // Set Buff Cooldown
  if ( params._cooldown == timespan_t::min() )
  {
    if ( data().ok() )
      buff_cooldown = data().cooldown();
  }
  else
    buff_cooldown = params._cooldown;

  // Set Max stacks
  if ( params._max_stack == -1 )
  {
    if ( data().ok() )
    {
      if ( data().max_stacks() != 0 )
        _max_stack = data().max_stacks();
      else if ( data().initial_stacks() != 0 )
        _max_stack = data().initial_stacks();
    }
  }
  else
    _max_stack = params._max_stack;

  // Set Proc Chance
  if ( params._chance == -1.0 )
  {
    if ( data().ok() )
    {
      if ( data().proc_chance() != 0 )
        default_chance = data().proc_chance();
    }
    else if ( ! data().found() )
    {
      default_chance = 0.0;
    }
  }
  else
    default_chance = params._chance;

  default_value = params._default_value;

  // Set Reverse flag
  if ( params._reverse != -1 )
    reverse = params._reverse != 0;

  // Set Quiet flag
  if ( params._quiet != -1 )
    quiet = params._quiet != 0;

  // Set Activated flag
  if ( params._activated != -1 )
    activated = params._activated != 0;

  if ( initial_source ) // Player Buffs
  {
    player -> buff_list.push_back( this );
  }
  else // Sim Buffs
  {
    sim -> buff_list.push_back( this );
  }

  init();

}

// buff_t::init =============================================================

void buff_t::init()
{
  if ( _max_stack < 1 )
  {
    _max_stack = 1;
    sim->errorf( "buff %s: initialized with max_stack < 1. Setting max_stack to 1.", name_str.c_str() );
  }

  if ( _max_stack > 999 )
  {
    _max_stack = 999;
    sim->errorf( "buff %s: initialized with max_stack > 999. Setting max_stack to 999.", name_str.c_str() );
  }

  // Keep non hidden reported numbers clean
  start_intervals.mean = 0;
  trigger_intervals.mean = 0;

  buff_duration = std::min( buff_duration, timespan_t::from_seconds( sim -> wheel_seconds - 2.0 ) );


  stack_occurrence.resize( _max_stack + 1 );
  stack_react_time.resize( _max_stack + 1 );
  stack_react_ready_triggers.resize( _max_stack + 1 );

  if ( static_cast<int>( stack_uptime.size() ) < _max_stack )
    for ( int i = static_cast<int>( stack_uptime.size() ); i <= _max_stack; ++i )
      stack_uptime.push_back( new buff_uptime_t( sim ) );

  if ( initial_source ) // Player Buffs
  {
    cooldown = initial_source-> get_cooldown( "buff_" + name_str );
    rng = initial_source-> get_rng( name_str );
  }
  else // Sim Buffs
  {
    cooldown = sim -> get_cooldown( "buff_" + name_str );
    rng = sim -> get_rng( name_str );
  }
  cooldown -> duration = buff_cooldown;

  uptime_pct.reserve( sim -> iterations );

}

// buff_t::~buff_t ==========================================================

buff_t::~buff_t()
{
  range::dispose( stack_uptime );
}

// buff_t::combat_begin ==========================================================

void buff_t::combat_begin()
{
  iteration_uptime_sum = timespan_t::zero();
}

// buff_t::combat_end ==========================================================

void buff_t::combat_end()
{
  if ( player )
    uptime_pct.add( player -> iteration_fight_length != timespan_t::zero() ? 100.0 * iteration_uptime_sum / player -> iteration_fight_length : 0 );
  else
    uptime_pct.add( sim -> current_time != timespan_t::zero() ? 100.0 * iteration_uptime_sum / sim -> current_time : 0 );

  for ( size_t i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ] -> combat_end();
}

// buff_t::may_react ========================================================

bool buff_t::may_react( int stack )
{
  if ( current_stack == 0    ) return false;
  if ( stack > current_stack ) return false;
  if ( stack < 1             ) return false;
  if ( ! reactable           ) return false;

  if ( stack > _max_stack ) return false;

  timespan_t occur = stack_occurrence[ stack ];

  if ( occur <= timespan_t::zero() ) return true;

  return sim -> current_time > stack_react_time[ stack ];
}

// buff_t::stack_react ======================================================

int buff_t::stack_react()
{
  int stack = 0;

  for ( int i = 1; i <= current_stack; i++ )
  {
    if ( stack_react_time[ i ] > sim -> current_time ) break;
    stack++;
  }

  return stack;
}

// buff_t::remains ==========================================================

timespan_t buff_t::remains() const
{
  if ( current_stack <= 0 )
  {
    return timespan_t::zero();
  }
  if ( expiration )
  {
    return expiration -> occurs() - sim -> current_time;
  }
  return timespan_t::min();
}

// buff_t::remains_gt =======================================================

bool buff_t::remains_gt( timespan_t time )
{
  timespan_t time_remaining = remains();

  if ( time_remaining == timespan_t::zero() ) return false;

  if ( time_remaining == timespan_t::min() ) return true;

  return ( time_remaining > time );
}

// buff_t::remains_lt =======================================================

bool buff_t::remains_lt( timespan_t time )
{
  timespan_t time_remaining = remains();

  if ( time_remaining == timespan_t::min() ) return false;

  return ( time_remaining < time );
}

// buff_t::trigger ==========================================================

bool buff_t::trigger( action_t*  a,
                      int        stacks,
                      double     value,
                      timespan_t duration )
{
  double chance = default_chance;
  if ( chance < 0 ) chance = a -> ppm_proc_chance( -chance );
  return trigger( stacks, value, chance, duration );
}

// buff_t::trigger ==========================================================

bool buff_t::trigger( int        stacks,
                      double     value,
                      double     chance,
                      timespan_t duration )
{
  if ( _max_stack == 0 || chance == 0 ) return false;

  if ( cooldown -> down() )
    return false;

  if ( player && player -> current.sleeping )
    return false;

  trigger_attempts++;

  if ( chance < 0 ) chance = default_chance;

  if ( ! rng -> roll( chance ) )
    return false;

  if ( value == DEFAULT_VALUE() && default_value != DEFAULT_VALUE() )
    value = default_value;

  if ( ! activated && player && player -> in_combat && sim -> default_aura_delay > timespan_t::zero() )
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
      delay = new ( sim ) buff_delay_t( sim, player, this, stacks, value, duration );
  }
  else
    execute( stacks, value, duration );

  return true;
}

// buff_t::execute ==========================================================

void buff_t::execute( int stacks, double value, timespan_t duration )
{
  if ( last_trigger > timespan_t::zero() )
  {
    trigger_intervals.add( ( sim -> current_time - last_trigger ).total_seconds() );
  }
  last_trigger = sim -> current_time;

  if ( reverse )
  {
    decrement( stacks, value );
  }
  else
  {
    increment( stacks, value, duration );
  }

  // new buff cooldown impl
  if ( cooldown -> duration > timespan_t::zero() )
  {
    if ( sim -> debug )
      sim -> output( "%s starts buff %s cooldown (%s) with duration %.2f",
                     ( source ? source -> name() : "someone" ), name_str.c_str(), cooldown -> name(), cooldown -> duration.total_seconds() );

    cooldown -> start();
  }

  trigger_successes++;
}

// buff_t::increment ========================================================

void buff_t::increment( int        stacks,
                        double     value,
                        timespan_t duration )
{
  if ( overridden ) return;

  if ( _max_stack == 0 ) return;

  if ( current_stack == 0 )
  {
    start( stacks, value, duration );
  }
  else
  {
    refresh( stacks, value, duration );
  }
}

// buff_t::decrement ========================================================

void buff_t::decrement( int    stacks,
                        double value )
{
  if ( overridden ) return;

  if ( _max_stack == 0 || current_stack <= 0 ) return;

  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    if ( static_cast<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ] -> update( false );

    current_stack -= stacks;

    if ( value == DEFAULT_VALUE() && default_value != DEFAULT_VALUE() )
      value = default_value;

    if ( value >= 0 ) current_value = value;

    if ( static_cast<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ] -> update( true );

    if ( sim -> debug )
      sim -> output( "buff %s decremented by %d to %d stacks",
                     name_str.c_str(), stacks, current_stack );
  }
}

// buff_t::extend_duration ==================================================

void buff_t::extend_duration( player_t* p, timespan_t extra_seconds )
{
  assert( expiration );
  assert( extra_seconds.total_seconds() < sim -> wheel_seconds );

  if ( extra_seconds > timespan_t::zero() )
  {
    expiration -> reschedule( expiration -> remains() + extra_seconds );

    if ( sim -> debug )
      sim -> output( "%s extends buff %s by %.1f seconds. New expiration time: %.1f",
                     p -> name(), name_str.c_str(), extra_seconds.total_seconds(), expiration -> occurs().total_seconds() );
  }
  else if ( extra_seconds < timespan_t::zero() )
  {
    timespan_t reschedule_time = expiration -> remains() + extra_seconds;

    if ( reschedule_time <= timespan_t::zero() )
    {
      // When Strength of Soul removes the Weakened Soul debuff completely,
      // there's a delay before the server notifies the client. Modeling
      // this effect as a world lag.
      timespan_t lag, dev;

      lag = p -> world_lag_override ? p -> world_lag : sim -> world_lag;
      dev = p -> world_lag_stddev_override ? p -> world_lag_stddev : sim -> world_lag_stddev;
      reschedule_time = p -> rngs.lag_world -> gauss( lag, dev );
    }

    event_t::cancel( expiration );

    expiration = new ( sim ) expiration_t( sim, player, this, reschedule_time );

    if ( sim -> debug )
      sim -> output( "%s decreases buff %s by %.1f seconds. New expiration time: %.1f",
                     p -> name(), name_str.c_str(), extra_seconds.total_seconds(), expiration -> occurs().total_seconds() );
  }
}

// buff_t::start ============================================================

void buff_t::start( int        stacks,
                    double     value,
                    timespan_t duration )
{
  if ( _max_stack == 0 ) return;

#ifndef NDEBUG
  if ( current_stack != 0 )
  {
    sim -> errorf( "buff_t::start assertion error current_stack is not zero, buff %s from %s.\n", name_str.c_str(), player -> name() );
    assert( 0 );
  }
#endif

  if ( sim -> current_time <= timespan_t::from_seconds( 0.01 ) ) constant = true;

  start_count++;

  bump( stacks, value );

  if ( last_start >= timespan_t::zero() )
  {
    start_intervals.add( ( sim -> current_time - last_start ).total_seconds() );
  }
  last_start = sim -> current_time;

  timespan_t d = ( duration >= timespan_t::zero() ) ? duration : buff_duration;
  if ( d > timespan_t::zero() )
  {
    expiration = new ( sim ) expiration_t( sim, player, this, d );
  }
}

// buff_t::refresh ==========================================================

void buff_t::refresh( int        stacks,
                      double     value,
                      timespan_t duration )
{
  if ( _max_stack == 0 ) return;

  refresh_count++;

  bump( stacks, value );

  timespan_t d = ( duration >= timespan_t::zero() ) ? duration : buff_duration;
  // Make sure we always cancel the expiration event if we get an
  // infinite duration
  if ( d <= timespan_t::zero() )
    event_t::cancel( expiration );
  else
  {
    assert( d > timespan_t::zero() );
    // Infinite duration -> duration of d
    if ( unlikely( ! expiration ) )
      expiration = new ( sim ) expiration_t( sim, player, this, d );
    else
      expiration -> reschedule( d );
  }
}

// buff_t::bump =============================================================

void buff_t::bump( int stacks, double value )
{
  if ( _max_stack == 0 ) return;

//  if ( value >= 0 )
  current_value = value;

  if ( max_stack() < 0 )
  {
    current_stack += stacks;
  }
  else if ( current_stack < max_stack() )
  {
    int before_stack = current_stack;
    stack_uptime[ current_stack ] -> update( false );

    current_stack += stacks;
    if ( current_stack > max_stack() )
      current_stack = max_stack();

    stack_uptime[ current_stack ] -> update( true );

    aura_gain();

    if ( reactable )
    {
      timespan_t now = sim -> current_time;
      timespan_t total_reaction_time = ( player ? ( player -> total_reaction_time() ) : sim -> reaction_time );
      timespan_t react = now + total_reaction_time;
      for ( int i = before_stack+1; i <= current_stack; i++ )
      {
        stack_occurrence[ i ] = now;
        stack_react_time[ i ] = react;
        if ( player -> ready_type == READY_TRIGGER )
        {
          stack_react_ready_triggers[ i ] = new ( sim ) react_ready_trigger_t( player, this, i, total_reaction_time );
        }
      }
    }
  }

  if ( player ) player -> trigger_ready();
}

// buff_t::override =========================================================

void buff_t::override( int stacks, double value )
{
  if ( _max_stack == 0 ) return;
#ifndef NDEBUG
  if ( current_stack != 0 )
  {
    sim -> errorf( "buff_t::override assertion error current_stack is not zero, buff %s from %s.\n", name_str.c_str(), player -> name() );
    assert( 0 );
  }
#endif

  if ( value == DEFAULT_VALUE() )
    value = default_value;

  buff_duration = timespan_t::zero();
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
  if ( last_start >= timespan_t::zero() )
  {
    iteration_uptime_sum += sim -> current_time - last_start;
  }

  if ( sim -> target -> resources.base[ RESOURCE_HEALTH ] == 0 ||
       sim -> target -> resources.current[ RESOURCE_HEALTH ] > 0 )
    if ( ! overridden )
    {
      constant = false;
    }

  for ( size_t i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ] -> update( false );

  if ( reactable && player -> ready_type == READY_TRIGGER )
  {
    for ( size_t i = 0; i < stack_react_ready_triggers.size(); i++ )
      event_t::cancel( stack_react_ready_triggers[ i ] );
  }

  if ( player ) player -> trigger_ready();
}

// buff_t::predict ==========================================================

void buff_t::predict()
{
  // Guarantee that may_react() will return true if the buff is present.
  fill( &stack_occurrence[ 0 ], &stack_occurrence[ current_stack + 1 ], timespan_t::min() );
  fill( &stack_react_time[ 0 ], &stack_react_time[ current_stack + 1 ], timespan_t::min() );
}

// buff_t::aura_gain ========================================================

void buff_t::aura_gain()
{
  if ( sim -> log )
  {
    std::string s = name_str + "_" + util::to_string( current_stack );

    if ( player )
    {
      if ( sim -> log && ! player->current.sleeping )
      {
        sim -> output( "%s gains %s ( value=%.2f )", player -> name(), s.c_str(), current_value );
      }
    }
    else
    {
      if ( sim -> log ) sim -> output( "Raid gains %s ( value=%.2f )", s.c_str(), current_value );
    }
  }
}

// buff_t::aura_loss ========================================================

void buff_t::aura_loss()
{
  if ( player )
  {
    if ( sim -> log && ! player -> current.sleeping )
      sim -> output( "%s loses %s", player -> name(), name_str.c_str() );
  }
  else
  {
    if ( sim -> log ) sim -> output( "Raid loses %s",  name_str.c_str() );
  }
}

// buff_t::reset ============================================================

void buff_t::reset()
{
  event_t::cancel( delay );
  cooldown -> reset();
  expire();
  last_start = timespan_t::min();
  last_trigger = timespan_t::min();
}

// buff_t::merge ============================================================

void buff_t::merge( const buff_t& other )
{
  start_intervals.merge( other.start_intervals );
  trigger_intervals.merge( other.trigger_intervals );
  up_count              += other.up_count;
  down_count            += other.down_count;
  start_count           += other.start_count;
  refresh_count         += other.refresh_count;
  trigger_attempts      += other.trigger_attempts;
  trigger_successes     += other.trigger_successes;

  uptime_pct.merge( other.uptime_pct );

#ifndef NDEBUG
  if ( stack_uptime.size() != other.stack_uptime.size() )
  {
    sim->errorf( "buff_t::merge buff %s of player %s stack_uptime vector not of equal length.\n", name_str.c_str(), player ? player -> name() : "" );
    assert( 0 );
  }
#endif

  for ( size_t i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ] -> merge ( *( other.stack_uptime[ i ] ) );
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
  start_intervals.analyze();
  trigger_intervals.analyze();
  avg_start   =   start_count / ( double ) sim -> iterations;
  avg_refresh = refresh_count / ( double ) sim -> iterations;
  uptime_pct.analyze();

  for ( size_t i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ] -> analyze();
}

// buff_t::find =============================================================

buff_t* buff_t::find( const std::vector<buff_t*>& buffs,
                      const std::string& name_str,
                      player_t* source )
{
  for ( size_t i = 0; i < buffs.size(); i++ )
  {
    buff_t* b = buffs[ i ];

    // FIXME!  It would be great to get rid of "initial_source" and actually
    // understand if a (de)buff is shared or not.

    if ( name_str == b -> name_str )
      if ( ! source || ( source == b -> initial_source ) )
        return b;
  }

  return NULL;
}

// buff_t::to_str ===========================================================

std::string buff_t::to_str() const
{
  std::ostringstream s;

  s << data().to_str();
  s << " max_stack=" << _max_stack;
  s << " initial_stack=" << data().initial_stacks();
  s << " cooldown=" << cooldown -> duration.total_seconds();
  s << " duration=" << buff_duration.total_seconds();
  s << " default_chance=" << default_chance;

  return s.str();
}

// buff_t::create_expression ================================================

expr_t* buff_t::create_expression(  std::string buff_name,
                                    action_t* action,
                                    const std::string& type,
                                    buff_t* static_buff )
{
  struct buff_expr_t : public expr_t
  {
    std::string buff_name;
    action_t* action;
    buff_t* static_buff;
    target_specific_t<buff_t> specific_buff;

    buff_expr_t( const std::string& n, std::string bn, action_t* a, buff_t* b ) :
      expr_t( n ), buff_name( bn ), action( a ), static_buff( b ), specific_buff( bn, a -> player ) {}

    buff_t* buff()
    {
      if ( static_buff ) return static_buff;
      buff_t*& buff = specific_buff[ action -> target ];
      if ( ! buff )
      {
        action -> player -> get_target_data( action -> target );
        buff = buff_t::find( action -> target, buff_name, action -> player );
        if ( ! buff ) buff = buff_t::find( action -> target, buff_name, action -> target ); // Raid debuffs
        if ( ! buff )
        {
          action -> sim -> errorf( "Reference to unknown buff/debuff %s by player %s", buff_name.c_str(), action -> player -> name() );
          assert( 0 );
          action -> sim -> cancel();
          // Prevent segfault
          buff = buff_creator_t( action -> player, "dummy" );
        }
      }
      return buff;
    }
  };

  if ( type == "duration" )
  {
    struct duration_expr_t : public buff_expr_t
    {
      duration_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_duration", bn, a, b ) {}
      virtual double evaluate() { return buff() -> buff_duration.total_seconds(); }
    };
    return new duration_expr_t( buff_name, action, static_buff );
  }
  if ( type == "remains" )
  {
    struct remains_expr_t : public buff_expr_t
    {
      remains_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_remains", bn, a, b ) {}
      virtual double evaluate() { return buff() -> remains().total_seconds(); }
    };
    return new remains_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "cooldown_remains" )
  {
    struct cooldown_remains_expr_t : public buff_expr_t
    {
      cooldown_remains_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_cooldown_remains", bn, a, b ) {}
      virtual double evaluate() { return buff() -> cooldown -> remains().total_seconds(); }
    };
    return new cooldown_remains_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "up" )
  {
    struct up_expr_t : public buff_expr_t
    {
      up_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_up", bn, a, b ) {}
      virtual double evaluate() { return buff() -> check() > 0; }
    };
    return new up_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "down" )
  {
    struct down_expr_t : public buff_expr_t
    {
      down_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_down", bn, a, b ) {}
      virtual double evaluate() { return buff() -> check() <= 0; }
    };
    return new down_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "stack" )
  {
    struct stack_expr_t : public buff_expr_t
    {
      stack_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_stack", bn, a, b ) {}
      virtual double evaluate() { return buff() -> check(); }
    };
    return new stack_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "max_stack" )
  {
    struct max_stack_expr_t : public buff_expr_t
    {
      max_stack_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_max_stack", bn, a, b ) {}
      virtual double evaluate() { return buff() -> max_stack(); }
    };
    return new max_stack_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "value" )
  {
    struct value_expr_t : public buff_expr_t
    {
      value_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_value", bn, a, b ) {}
      virtual double evaluate() { return buff() -> value(); }
    };
    return new value_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "react" )
  {
    struct react_expr_t : public buff_expr_t
    {
      react_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_react", bn, a, b ) {}
      virtual double evaluate() { return buff() -> stack_react(); }
    };
    static_buff -> reactable = true;
    return new react_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "cooldown_react" )
  {
    struct cooldown_react_expr_t : public buff_expr_t
    {
      cooldown_react_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_cooldown_react", bn, a, b ) {}
      virtual double evaluate()
      {
        if ( buff() -> check() && ! buff() -> may_react() )
          return 0;
        else
          return buff() -> cooldown -> remains().total_seconds();
      }
    };
    return new cooldown_react_expr_t( buff_name, action, static_buff );
  }

  return 0;
}

// ==========================================================================
// STAT_BUFF
// ==========================================================================

// stat_buff_t::stat_buff_t =================================================

stat_buff_t::stat_buff_t( const stat_buff_creator_t& params ) :
  buff_t( params )
{
  if ( params.stats.size() == 0  )
  {
    bool has_ap = false;

    for ( size_t i = 1; i <= data()._effects -> size(); i++ )
    {
      if ( data().effectN( i ).subtype() == A_MOD_RATING )
      {
        double amount = player -> dbc.effect_average( data().effectN( i ).id(), player -> level );
        stat_e stat = translate_stat_buff_misc_number( data().effectN( 1 ).misc_value1() );

        stats.push_back( buff_stat_t( stat, amount ) );

        break; // only parse first effect for now
      }
      else if ( ! has_ap && ( data().effectN( i ).subtype() == A_MOD_RANGED_ATTACK_POWER || data().effectN( i ).subtype() == A_MOD_ATTACK_POWER ) )
      {
        has_ap = true;
        double amount = player -> dbc.effect_average( data().effectN( i ).id(), player -> level );
        stat_e stat = STAT_ATTACK_POWER;

        stats.push_back( buff_stat_t( stat, amount ) );
      }
      else if ( data().effectN( i ).subtype() == A_MOD_DAMAGE_DONE && data().effectN( i ).misc_value1() == 126 )
      {
        // examples: blood fury 33702
        double amount = player -> dbc.effect_average( data().effectN( i ).id(), player -> level );
        stat_e stat = STAT_SPELL_POWER;

        stats.push_back( buff_stat_t( stat, amount ) );
      }
    }
  }
  else // parse stats from params
  {
    for ( size_t i = 0; i < params.stats.size(); ++i )
    {
      stats.push_back( buff_stat_t( params.stats[ i ].stat, params.stats[ i ].amount, params.stats[ i ].check_func, params.stats[ i ].data ) );
    }
  }
}

// stat_buff_t::bump ========================================================

void stat_buff_t::bump( int stacks, double /* value */ )
{

  buff_t::bump( stacks );
  for ( size_t i = 0; i < stats.size(); ++i )
  {
    if ( stats[ i ].check_func && ! stats[ i ].check_func( stats[ i ].data ? stats[ i ].data : player ) ) continue;
    double delta = stats[ i ].amount * current_stack - stats[ i ].current_value;
    if ( delta > 0 )
    {
      player -> stat_gain( stats[ i ].stat, delta, 0, 0, buff_duration > timespan_t::zero() );
      stats[ i ].current_value += delta;
    }
    else
      assert( delta == 0 );
  }
}

// stat_buff_t::decrement ===================================================

void stat_buff_t::decrement( int stacks, double /* value */ )
{
  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    for ( size_t i = 0; i < stats.size(); ++i )
    {
      double delta = stats[ i ].amount * stacks;
      player -> stat_loss( stats[ i ].stat, ( delta <= stats[ i ].current_value ) ? delta : 0.0, 0, 0, buff_duration > timespan_t::zero() );
      stats[ i ].current_value -= delta;
    }
    current_stack -= stacks;

    if ( player ) player -> trigger_ready();
  }
}

// stat_buff_t::expire ======================================================

void stat_buff_t::expire()
{
  if ( current_stack > 0 )
  {
    for ( size_t i = 0; i < stats.size(); ++i )
    {
      player -> stat_loss( stats[ i ].stat, stats[ i ].current_value, 0, 0, buff_duration > timespan_t::zero() );
      stats[ i ].current_value = 0;
    }
    buff_t::expire();
  }
}

// ==========================================================================
// COST_REDUCTION_BUFF
// ==========================================================================

cost_reduction_buff_t::cost_reduction_buff_t( const cost_reduction_buff_creator_t& params ) :
  buff_t( params ), amount( params._amount ), school( params._school ), refreshes( params._refreshes )
{
}

// cost_reduction_buff_t::bump ==============================================

void cost_reduction_buff_t::bump( int stacks,double value )
{
  if ( value > 0 )
  {
    amount = value;
  }
  buff_t::bump( stacks );
  double delta = amount * current_stack - current_value;
  if ( delta > 0 )
  {
    player -> cost_reduction_gain( school, delta );
    current_value += delta;
  }
  else assert( delta == 0 );
}

// cost_reduction_buff_t::decrement =========================================

void cost_reduction_buff_t::decrement( int stacks, double /* value */ )
{
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

void cost_reduction_buff_t::refresh( int        stacks,
                                     double     value,
                                     timespan_t duration )
{
  if ( ! refreshes )
  {
    refresh_count++;

    bump( stacks, value );
    return;
  }

  buff_t::refresh( stacks, value, duration );
}

// ==========================================================================
// HASTE_BUFF
// ==========================================================================

haste_buff_t::haste_buff_t( const haste_buff_creator_t& params ) :
  buff_t( params )
{ }

// haste_buff_t::execute ====================================================

void haste_buff_t::execute( int stacks, double value, timespan_t duration )
{
  int is_up = check();
  double old_attack_speed = 0;

  if ( ! is_up && ( player -> main_hand_attack || player -> off_hand_attack ) )
    old_attack_speed = player -> composite_attack_speed();

  buff_t::execute( stacks, value, duration );

  // Down -> Up, haste remaining swing speeds
  if ( ! is_up )
  {
    if ( player -> main_hand_attack )
      player -> main_hand_attack -> reschedule_auto_attack( old_attack_speed );
    if ( player -> off_hand_attack )
      player -> off_hand_attack -> reschedule_auto_attack( old_attack_speed );
  }
}

// haste_buff_t::expire =====================================================

void haste_buff_t::expire()
{
  int is_up = check();
  double old_attack_speed = 0;

  if ( is_up && ( player -> main_hand_attack || player -> off_hand_attack ) )
    old_attack_speed = player -> composite_attack_speed();

  buff_t::expire();

  // Up -> Down, slow down remaining swing speeds
  if ( is_up )
  {
    if ( player -> main_hand_attack )
      player -> main_hand_attack -> reschedule_auto_attack( old_attack_speed );
    if ( player -> off_hand_attack )
      player -> off_hand_attack -> reschedule_auto_attack( old_attack_speed );
  }
}

// ==========================================================================
// TICK_BUFF
// ==========================================================================

tick_buff_t::tick_buff_t( const tick_buff_creator_t& params ) :
  buff_t( params ), period( params._period )
{
  if ( ! s_data || s_data == spell_data_t::nil() || s_data == spell_data_t::not_found() || period != timespan_t::min() )
    return;

  for ( size_t i = 1; i <= s_data -> _effects -> size(); i++ )
  {
    const spelleffect_data_t& e = s_data -> effectN( i );
    if ( ! e.ok() || e.type() != E_APPLY_AURA )
      continue;

    switch ( e.subtype() )
    {
    case A_PERIODIC_ENERGIZE:
    case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
    case A_PERIODIC_HEALTH_FUNNEL:
    case A_PERIODIC_MANA_LEECH:
    case A_PERIODIC_DAMAGE_PERCENT:
    case A_PERIODIC_DUMMY:
    case A_PERIODIC_TRIGGER_SPELL:
    {
      period = e.period();
      break;
    }
    default: break;
    }
  }
}

// tick_buff_t::trigger =====================================================

bool tick_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  assert( period != timespan_t::min() && period != timespan_t::zero() );

  timespan_t carryover = timespan_t::zero();

  if ( current_stack > 0 )
  {
    int result = static_cast< int >( remains() / period );
    timespan_t carryover = remains();
    carryover -= period * result;
    assert( carryover <= period );
    if ( sim -> debug )
      sim -> output( "%s carryover duration from ongoing tick: %f", name(), carryover.total_seconds() );
  }

  if ( duration == timespan_t::min() && carryover != timespan_t::zero() )
  {
    assert( buff_duration > timespan_t::zero() );
    duration = buff_duration + carryover;
  }
  else
    duration += carryover;

  return buff_t::trigger( stacks, value, chance, duration );
}

// ==========================================================================
// DEBUFF
// ==========================================================================

debuff_t::debuff_t( const buff_creator_basics_t& params ) :
  buff_t( params )
{}

// Absorb Buff


absorb_buff_t::absorb_buff_t( const absorb_buff_creator_t& params ) :
  buff_t( params ), absorb_source( params._absorb_source )
{
}

void buff_creator_basics_t::init()
{
  _chance = -1.0;
  _max_stack = -1;
  _duration = timespan_t::min();
  _cooldown = timespan_t::min();
  _quiet = -1;
  _reverse = -1;
  _activated = -1;
  _default_value = buff_t::DEFAULT_VALUE();
}

buff_creator_basics_t::buff_creator_basics_t( actor_pair_t p, const std::string& n, const spell_data_t* sp ) :
  _player( p ), _sim( p.source->sim ), _name( n ), s_data( sp )
{ init(); }

buff_creator_basics_t::buff_creator_basics_t( actor_pair_t p , uint32_t id, const std::string& n ) :
  _player( p ), _sim( p.source->sim ), _name( n ), s_data( _player.source ? _player.source->find_spell( id ) : spell_data_t::nil() )
{ init(); }

buff_creator_basics_t::buff_creator_basics_t( sim_t* s, const std::string& n, const spell_data_t* sp ) :
  _player( actor_pair_t() ), _sim( s ), _name( n ), s_data( sp )
{ init(); }

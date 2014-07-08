// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct buff_event_t : public event_t
{
  buff_t* buff;

  buff_event_t( buff_t* b, const char* name, timespan_t d ) :
    event_t( *b -> sim, b -> player, name ), buff( b )
  { sim().add_event( this, d ); }

  buff_event_t( buff_t* b, timespan_t d ) :
    event_t( *b -> sim, b -> player, b -> name() ), buff( b )
  { sim().add_event( this, d ); }
};

struct react_ready_trigger_t : public buff_event_t
{
  unsigned stack;

  react_ready_trigger_t( buff_t* b, unsigned s, timespan_t d ) :
    buff_event_t( b, "react_ready_trigger", d ), stack( s )
  {}

  void execute()
  {
    buff -> stack_react_ready_triggers[ stack ] = 0;

    if ( p() )
      p() -> trigger_ready();
  }
};

struct expiration_t : public buff_event_t
{
  expiration_t( buff_t* b, timespan_t d ) : buff_event_t( b, d ) {}

  virtual void execute()
  {
    buff -> expiration = 0;
    buff -> expire();
  }
};

struct tick_t : public buff_event_t
{
  double current_value;
  int current_stacks;

  tick_t( buff_t* b, timespan_t d, double value, int stacks ) :
    buff_event_t( b, d ), current_value( value ), current_stacks( stacks )
  { }

  void execute()
  {
    buff -> tick_event = 0;

    // For tick number calculations, always include the +1ms so we get correct
    // tick number labeling on the last tick, just before the buff expires.
    timespan_t elapsed = buff -> elapsed( buff -> sim -> current_time ) + timespan_t::from_millis( 1 ),
               total_duration = elapsed + buff -> remains();
    int current_tick = static_cast<int>( elapsed / buff -> buff_period ),
        total_ticks = static_cast<int>( total_duration / buff -> buff_period );

    if ( buff -> sim -> debug )
      buff -> sim -> out_debug.printf( "%s buff %s ticks (%d of %d).", buff -> player -> name(), buff -> name(), current_tick, total_ticks );

    // Tick callback is called before the aura stack count is altered to ensure
    // that the buff is always up during the "tick". Last tick detection can be
    // made through the int arguments passed to the function call.
    if ( buff -> tick_callback )
      buff -> tick_callback( buff, current_tick, total_ticks );

    if ( ! buff -> reverse )
      buff -> bump( current_stacks, current_value );
    else
      buff -> decrement( current_stacks, current_value );

    timespan_t period = buff -> buff_period;
    if ( buff -> remains() >= period )
    {
      // Reorder the last tick to happen 1ms before expiration
      if ( buff -> remains() == period )
        period -= timespan_t::from_millis( 1 );
      buff -> tick_event = new ( *buff -> sim ) tick_t( buff, period, current_value, current_stacks );
    }
  }
};

struct buff_delay_t : public buff_event_t
{
  double     value;
  timespan_t duration;
  int        stacks;

  buff_delay_t( buff_t* b, int stacks, double value, timespan_t d ) :
    buff_event_t( b, b -> rng().gauss( b -> sim -> default_aura_delay, b -> sim -> default_aura_delay_stddev ) ),
    value( value ), duration( d ), stacks( stacks )
  {}

  virtual void execute()
  {
    // Add a Cooldown check here to avoid extra processing due to delays
    if ( buff -> cooldown -> remains() ==  timespan_t::zero() )
      buff -> execute( stacks, value, duration );
    buff -> delay = nullptr;
  }
};

struct expiration_delay_t : public buff_event_t
{
  expiration_delay_t( buff_t* b, timespan_t d ) :
    buff_event_t( b, d )
  { }

  virtual void execute()
  {
    buff -> expiration_delay = nullptr;
    // Call real expire after a delay
    buff -> expire();
  }
};
}

buff_t::buff_t( const buff_creation::buff_creator_basics_t& params ) :
  sim( params._sim ),
  player( params._player.target ),
  name_str( params._name ),
  s_data( params.s_data ),
  source( params._player.source ),
  expiration(),
  delay(),
  expiration_delay(),
  cooldown(),
  _max_stack( 1 ),
  default_value( DEFAULT_VALUE() ),
  activated( true ),
  reactable( false ),
  reverse(),
  constant(),
  quiet(),
  overridden(),
  requires_invalidation(),
  current_value(),
  current_stack(),
  buff_duration( timespan_t() ),
  default_chance( 1.0 ),
  buff_period( timespan_t::min() ),
  tick_behavior( BUFF_TICK_NONE ),
  tick_event( 0 ),
  last_start( timespan_t() ),
  last_trigger( timespan_t() ),
  iteration_uptime_sum( timespan_t() ),
  up_count(),
  down_count(),
  start_count(),
  refresh_count(),
  trigger_attempts(),
  trigger_successes(),
  simulation_max_stack( 0 ),
  benefit_pct(),
  trigger_pct(),
  avg_start(),
  avg_refresh(),
  uptime_pct(),
  start_intervals(),
  trigger_intervals()

{
  if ( source ) // Player Buffs
  {
    player -> buff_list.push_back( this );
    cooldown = source -> get_cooldown( "buff_" + name_str );
  }
  else // Sim Buffs
  {
    sim -> buff_list.push_back( this );
    cooldown = sim -> get_cooldown( "buff_" + name_str );
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
    {
      assert( ! ( data().cooldown() != timespan_t::zero() && data().internal_cooldown() != timespan_t::zero() ) );

      if ( data().cooldown() != timespan_t::zero() )
        cooldown -> duration = data().cooldown();
      else
        cooldown -> duration = data().internal_cooldown();
    }
  }
  else
    cooldown -> duration = params._cooldown;

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

  if ( params._period >= timespan_t::zero() )
    buff_period = params._period;
  else
  {
    for ( size_t i = 1; i <= s_data -> effect_count(); i++ )
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
          buff_period = e.period();
          break;
        }
        default: break;
      }
    }
  }

  if ( params._behavior != BUFF_TICK_NONE )
    tick_behavior = params._behavior;
  // If period is set, buf no buff tick behavior, set the behavior
  // automatically to clipped ticks
  else if ( buff_period > timespan_t::zero() && params._behavior == BUFF_TICK_NONE )
    tick_behavior = BUFF_TICK_CLIP;

  if ( params._tick_callback )
    tick_callback = params._tick_callback;

  invalidate_list = params._invalidate_list;
  requires_invalidation = ! invalidate_list.empty();

  if ( player && ! player -> cache.active ) requires_invalidation = false;

  if ( _max_stack < 1 )
  {
    sim -> errorf( "buff %s: initialized with max_stack < 1 (%d). Setting max_stack to 1.\n", name_str.c_str(), _max_stack );
    _max_stack = 1;
  }

  if ( _max_stack > 999 )
  {
    _max_stack = 999;
    sim -> errorf( "buff %s: initialized with max_stack > 999. Setting max_stack to 999.\n", name_str.c_str() );
  }

  // Keep non hidden reported numbers clean
  //start_intervals.mean = 0;
  // trigger_intervals.mean = 0;

  stack_occurrence.resize( _max_stack + 1 );
  stack_react_time.resize( _max_stack + 1 );
  stack_react_ready_triggers.resize( _max_stack + 1 );

  if ( as<int>( stack_uptime.size() ) < _max_stack )
    for ( int i = as<int>( stack_uptime.size() ); i <= _max_stack; ++i )
      stack_uptime.push_back( new buff_uptime_t() );
}

void buff_t::add_invalidate( cache_e c )
{
  if ( range::find( invalidate_list, c ) == invalidate_list.end() ) // avoid duplication
  {
    invalidate_list.push_back( c );
    requires_invalidation = true;
  }
}

// buff_t::datacollection_begin =============================================

void buff_t::datacollection_begin()
{
  iteration_uptime_sum = timespan_t::zero();

  up_count = down_count = 0;
  trigger_successes = trigger_attempts = 0;
  start_count = 0;
  refresh_count = 0;

  for ( int i = 0; i <= simulation_max_stack; i++ )
    stack_uptime[ i ] -> datacollection_begin();
}

// buff_t::datacollection_end ===============================================

void buff_t::datacollection_end()
{
  timespan_t& time = player ? player -> iteration_fight_length : sim -> current_time;

  uptime_pct.add( time != timespan_t::zero() ? 100.0 * iteration_uptime_sum / time : 0 );

  for ( int i = 0; i <= simulation_max_stack; i++ )
    stack_uptime[ i ] -> datacollection_end( time );

  double benefit = up_count > 0 ? 100.0 * up_count / ( up_count + down_count ) : 0;
  benefit_pct.add( benefit );

  double _trigger_pct = trigger_attempts > 0 ? 100.0 * trigger_successes / trigger_attempts : 0;
  trigger_pct.add( _trigger_pct );

  avg_start.add( start_count );
  avg_refresh.add( refresh_count );
}

// buff_t::total_stack ======================================================

int buff_t::total_stack()
{
  int s = current_stack;

  if ( delay )
    s += debug_cast< buff_delay_t* >( delay ) -> stacks;

  return std::min( s, _max_stack );
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

bool buff_t::remains_gt( timespan_t time ) const
{
  timespan_t time_remaining = remains();

  if ( time_remaining == timespan_t::zero() ) return false;

  if ( time_remaining == timespan_t::min() ) return true;

  return ( time_remaining > time );
}

// buff_t::remains_lt =======================================================

bool buff_t::remains_lt( timespan_t time ) const
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

  if ( player && player -> is_sleeping() )
    return false;

  trigger_attempts++;

  if ( chance < 0 ) chance = default_chance;

  if ( ! rng().roll( chance ) )
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
      buff_delay_t& d = *static_cast< buff_delay_t* >( delay );
      d.stacks += stacks;
      d.value = value;
    }
    else
      delay = new ( *sim ) buff_delay_t( this, stacks, value, duration );
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

  // If the buff has a tick event ongoing, the rules change a bit for ongoing
  // ticking buffs, we treat executes as another "normal trigger", which
  // refreshes the buff
  if ( tick_event )
    increment( stacks == 1 ? ( reverse ? _max_stack : stacks ) : stacks, value, duration );
  else
  {
    if ( reverse && current_stack > 0 )
      decrement( stacks, value );
    else
      increment( stacks == 1 ? ( reverse ? _max_stack : stacks ) : stacks, value, duration );
  }

  // new buff cooldown impl
  if ( cooldown -> duration > timespan_t::zero() )
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s starts buff %s cooldown (%s) with duration %.2f",
                     source_name().c_str(), name_str.c_str(), cooldown -> name(), cooldown -> duration.total_seconds() );

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
    if ( requires_invalidation ) invalidate_cache();

    if ( as<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ] -> update( false, sim -> current_time );

    current_stack -= stacks;

    if ( value == DEFAULT_VALUE() && default_value != DEFAULT_VALUE() )
      value = default_value;

    if ( value >= 0 ) current_value = value;

    if ( as<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ] -> update( true, sim -> current_time );

    if ( sim -> debug )
      sim -> out_debug.printf( "buff %s decremented by %d to %d stacks",
                     name_str.c_str(), stacks, current_stack );
  }
}

// buff_t::extend_duration ==================================================

void buff_t::extend_duration( player_t* p, timespan_t extra_seconds )
{
  assert( expiration );

  if ( extra_seconds > timespan_t::zero() )
  {
    expiration -> reschedule( expiration -> remains() + extra_seconds );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s extends buff %s by %.1f seconds. New expiration time: %.1f",
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
      reschedule_time = rng().gauss( lag, dev );
    }

    core_event_t::cancel( expiration );

    expiration = new ( *sim ) expiration_t( this, reschedule_time );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s decreases buff %s by %.1f seconds. New expiration time: %.1f",
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
    sim -> errorf( "buff_t::start assertion error current_stack is not zero, buff %s from %s.\n",
                   name_str.c_str(), source_name().c_str() );
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
    expiration = new ( *sim ) expiration_t( this, d );

    if ( tick_behavior != BUFF_TICK_NONE && 
         buff_period > timespan_t::zero() &&
         buff_period <= d )
    {
      timespan_t tick_time = buff_period;
      // Reorder the last tick to happen 1ms before expiration
      if ( tick_time == d )
        tick_time -= timespan_t::from_millis( 1 );
      assert( ! tick_event );
      tick_event = new ( *sim ) tick_t( this, tick_time, current_value, reverse ? 1 : stacks );
    }
  }
}

// buff_t::refresh ==========================================================

void buff_t::refresh( int        stacks,
                      double     value,
                      timespan_t duration )
{
  if ( _max_stack == 0 ) return;

  bump( stacks, value );

  refresh_count++;

  timespan_t d = ( duration >= timespan_t::zero() ) ? duration : buff_duration;

  // Carryover on ticking buffs that refresh, instead of clip
  if ( d > timespan_t::zero() && buff_period > timespan_t::zero() && tick_behavior == BUFF_TICK_REFRESH )
  {
    timespan_t carryover = remains() % buff_period;
    assert( carryover < buff_period );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s %s carryover duration from ongoing tick: %.3f", 
          player -> name(), name(), carryover.total_seconds() );

    d += carryover;
  }

  // Make sure we always cancel the expiration event if we get an
  // infinite duration
  if ( d <= timespan_t::zero() )
  {
    core_event_t::cancel( expiration );
    // Infinite ticking buff refreshes shouldnt happen, but cancel ongoing 
    // tick event just to be sure.
    core_event_t::cancel( tick_event );
  }
  else
  {
    assert( d > timespan_t::zero() );
    // Infinite duration -> duration of d
    if ( ! expiration )
      expiration = new ( *sim ) expiration_t( this, d );
    else
      expiration -> reschedule( d );

    if ( tick_event && tick_behavior == BUFF_TICK_CLIP )
    {
      core_event_t::cancel( tick_event );
      timespan_t tick_time = buff_period;
      // Reorder the last tick to happen 1ms before expiration
      if ( tick_time == d )
        tick_time -= timespan_t::from_millis( 1 );
      tick_event = new ( *sim ) tick_t( this, tick_time, current_value, reverse ? 1 : stacks );
    }
  }
}

// buff_t::bump =============================================================

void buff_t::bump( int stacks, double value )
{
  if ( _max_stack == 0 ) return;

  current_value = value;

  if ( requires_invalidation ) invalidate_cache();

  if ( max_stack() < 0 )
  {
    current_stack += stacks;
  }
  else if ( current_stack < max_stack() )
  {
    int before_stack = current_stack;

    current_stack += stacks;
    if ( current_stack > max_stack() )
      current_stack = max_stack();

    if ( before_stack != current_stack )
    {
      stack_uptime[ before_stack ] -> update( false, sim -> current_time );
      stack_uptime[ current_stack ] -> update( true, sim -> current_time );
    }

    aura_gain();

    if ( reactable )
    {
      timespan_t total_reaction_time = ( player ? ( player -> total_reaction_time() ) : sim -> reaction_time );
      timespan_t react = sim -> current_time + total_reaction_time;
      for ( int i = before_stack + 1; i <= current_stack; i++ )
      {
        stack_occurrence[ i ] = sim -> current_time;
        stack_react_time[ i ] = react;
        if ( player && player -> ready_type == READY_TRIGGER )
        {
          stack_react_ready_triggers[ i ] = new ( *sim ) react_ready_trigger_t( this, i, total_reaction_time );
        }
      }
    }

    if ( current_stack > simulation_max_stack )
      simulation_max_stack = current_stack;
  }

  if ( player ) player -> trigger_ready();
}

// buff_t::override =========================================================

void buff_t::override_buff( int stacks, double value )
{
  if ( _max_stack == 0 ) return;
#ifndef NDEBUG
  if ( current_stack != 0 )
  {
    sim -> errorf( "buff_t::override assertion error current_stack is not zero, buff %s from %s.\n",
                   name_str.c_str(), source_name().c_str() );
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

void buff_t::expire( timespan_t delay )
{
  if ( current_stack <= 0 ) return;

  if ( delay > timespan_t::zero() ) // Expiration Delay
  {
    if ( ! expiration_delay ) // Don't reschedule already existing expiration delay
    {
      expiration_delay = new ( *sim ) expiration_delay_t( this, delay );
    }
    return;
  }
  else
  {
    core_event_t::cancel( expiration_delay );
  }

  core_event_t::cancel( expiration );
  core_event_t::cancel( tick_event );

  assert( as<std::size_t>( current_stack ) < stack_uptime.size() );
  stack_uptime[ current_stack ] -> update( false, sim -> current_time );

  current_stack = 0;
  current_value = 0;
  aura_loss();
  if ( requires_invalidation ) invalidate_cache();
  if ( last_start >= timespan_t::zero() )
  {
    iteration_uptime_sum += sim -> current_time - last_start;
    if ( ! constant && ! overridden && sim -> buff_uptime_timeline )
    {
      timespan_t start_time = timespan_t::from_seconds( last_start.total_millis() / 1000 ) ;
      timespan_t end_time = timespan_t::from_seconds( sim -> current_time.total_millis() / 1000 );
      timespan_t begin_uptime = (( timespan_t::from_seconds( 1 ) - last_start ) % timespan_t::from_seconds( 1 ) );
      timespan_t end_uptime = (sim -> current_time % timespan_t::from_seconds( 1 ));

      if ( last_start % timespan_t::from_seconds( 1 ) == timespan_t::zero() )
        begin_uptime = timespan_t::from_seconds( 1 );

      uptime_array.add( start_time, begin_uptime.total_seconds() );
      for ( timespan_t i = start_time + timespan_t::from_millis( 1000 ); i < end_time; i = i + timespan_t::from_millis( 1000 ) )
        uptime_array.add( i, 1 );

      if ( end_uptime != timespan_t::zero() )
        uptime_array.add( end_time, end_uptime.total_seconds() );
    }
  }

  if ( sim -> target -> resources.base[ RESOURCE_HEALTH ] == 0 ||
       sim -> target -> resources.current[ RESOURCE_HEALTH ] > 0 )
    if ( ! overridden )
    {
      constant = false;
    }

  if ( reactable && player && player -> ready_type == READY_TRIGGER )
  {
    for ( size_t i = 0; i < stack_react_ready_triggers.size(); i++ )
      core_event_t::cancel( stack_react_ready_triggers[ i ] );
  }

  expire_override(); // virtual expire call

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
      if ( sim -> log && ! player -> is_sleeping() )
      {
        sim -> out_log.printf( "%s gains %s ( value=%.2f )", player -> name(), s.c_str(), current_value );
      }
    }
    else
    {
      if ( sim -> log )
        sim -> out_log.printf( "Raid gains %s ( value=%.2f )", s.c_str(), current_value );
    }
  }
}

// buff_t::aura_loss ========================================================

void buff_t::aura_loss()
{
  if ( player )
  {
    if ( sim -> log && ! player -> is_sleeping() )
      sim -> out_log.printf( "%s loses %s", player -> name(), name_str.c_str() );
  }
  else
  {
    if ( sim -> log ) sim -> out_log.printf( "Raid loses %s",  name_str.c_str() );
  }
}

// buff_t::reset ============================================================

void buff_t::reset()
{
  core_event_t::cancel( delay );
  core_event_t::cancel( expiration_delay );
  core_event_t::cancel( tick_event );
  cooldown -> reset( false );
  expire();
  last_start = timespan_t::min();
  last_trigger = timespan_t::min();
}

// buff_t::merge ============================================================

void buff_t::merge( const buff_t& other )
{
  start_intervals.merge( other.start_intervals );
  trigger_intervals.merge( other.trigger_intervals );

  uptime_pct.merge( other.uptime_pct );
  benefit_pct.merge( other.benefit_pct );
  trigger_pct.merge( other.trigger_pct );
  avg_start.merge( other.avg_start );
  avg_refresh.merge( other.avg_refresh );
  if ( sim -> buff_uptime_timeline )
    uptime_array.merge( other.uptime_array );

#ifndef NDEBUG
  if ( stack_uptime.size() != other.stack_uptime.size() )
  {
    sim -> errorf( "buff_t::merge buff %s from %s stack_uptime vector not of equal length.\n",
                   name_str.c_str(), source_name().c_str() );
    assert( 0 );
  }
#endif

  for ( size_t i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ] -> merge ( *( other.stack_uptime[ i ] ) );
}

// buff_t::analyze ==========================================================

void buff_t::analyze()
{

  if ( sim -> buff_uptime_timeline )
    uptime_array.adjust( sim -> divisor_timeline );
}

// buff_t::find =============================================================

buff_t* buff_t::find( const std::vector<buff_t*>& buffs,
                      const std::string& name_str,
                      player_t* source )
{
  for ( size_t i = 0; i < buffs.size(); i++ )
  {
    buff_t* b = buffs[ i ];

    if ( name_str == b -> name_str )
      if ( ! source || ( source == b -> source ) )
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
    target_specific_t<buff_t*> specific_buff;

    buff_expr_t( const std::string& n, const std::string& bn, action_t* a, buff_t* b ) :
      expr_t( n ), buff_name( bn ), action( a ), static_buff( b ), specific_buff( false ) {}

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
  else if ( type == "stack_pct" )
  {
    struct stack_pct_expr_t : public buff_expr_t
    {
      stack_pct_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_stack_pct", bn, a, b ) {}
      virtual double evaluate() { return 100.0 * buff() -> check() / buff() -> max_stack(); }
    };
    return new stack_pct_expr_t( buff_name, action, static_buff );
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
  else if ( type == "react_pct" )
  {
    struct react_pct_expr_t : public buff_expr_t
    {
      react_pct_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_react_pct", bn, a, b ) {}
      virtual double evaluate() { return 100.0 * buff() -> stack_react() / buff() -> max_stack(); }
    };
    static_buff -> reactable = true;
    return new react_pct_expr_t( buff_name, action, static_buff );
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

#ifdef SC_STAT_CACHE

// buff_t::invalidate_cache =================================================

void buff_t::invalidate_cache()
{
  if ( player )
  {
    for ( int i = as<int>( invalidate_list.size() ) - 1; i >= 0; i-- )
    {
      player -> invalidate_cache( invalidate_list[ i ] );
    }
  }
  else // It is an aura...  Ouch.
  {
    for ( int i = as<int>( sim -> player_no_pet_list.size() ) - 1; i >= 0; i-- )
    {
      player_t* p = sim -> player_no_pet_list[ i ];
      for ( int j = as<int>( invalidate_list.size() ) - 1; j >= 0; j-- )
        p -> invalidate_cache( invalidate_list[ j ] );
    }
  }
}

#endif

// ==========================================================================
// STAT_BUFF
// ==========================================================================

// stat_buff_t::stat_buff_t =================================================

stat_buff_t::stat_buff_t( const stat_buff_creator_t& params ) :
  buff_t( params )
{
  if ( params.stats.empty() )
  {
    bool has_ap = false;

    for ( size_t i = 1; i <= data().effect_count(); i++ )
    {
      stat_e s = STAT_NONE;
      double amount = 0;
      const spelleffect_data_t& effect = data().effectN( i );

      if ( effect.subtype() == A_MOD_STAT )
        s = static_cast< stat_e >( effect.misc_value1() + 1 );
      else if ( effect.subtype() == A_MOD_RATING )
        s = util::translate_rating_mod( effect.misc_value1() );
      else if ( effect.subtype() == A_MOD_DAMAGE_DONE && effect.misc_value1() == 126 )
        s = STAT_SPELL_POWER;
      else if ( effect.subtype() == A_MOD_RESISTANCE )
        s = STAT_BONUS_ARMOR;
      else if ( ! has_ap && ( effect.subtype() == A_MOD_ATTACK_POWER || effect.subtype() == A_MOD_RANGED_ATTACK_POWER ) )
      {
        s = STAT_ATTACK_POWER;
        has_ap = true;
      }
      else if ( effect.subtype() == A_MOD_INCREASE_HEALTH_2 || effect.subtype() == A_MOD_INCREASE_HEALTH )
        s = STAT_MAX_HEALTH;

      if ( params.item )
        amount = util::round( effect.average( params.item ) );
      else
        amount = effect.average( player, std::min( MAX_LEVEL, player -> level ) );

      if ( s != STAT_NONE )
        stats.push_back( buff_stat_t( s, amount ) );
    }
  }
  else // parse stats from params
  {
    for ( size_t i = 0; i < params.stats.size(); ++i )
    {
      stats.push_back( buff_stat_t( params.stats[ i ].stat, params.stats[ i ].amount, params.stats[ i ].check_func ) );
    }
  }
}

// stat_buff_t::bump ========================================================

void stat_buff_t::bump( int stacks, double /* value */ )
{

  buff_t::bump( stacks );
  for ( size_t i = 0; i < stats.size(); ++i )
  {
    if ( stats[ i ].check_func && ! stats[ i ].check_func( *this ) ) continue;
    double delta = stats[ i ].amount * current_stack - stats[ i ].current_value;
    if ( delta > 0 )
      player -> stat_gain( stats[ i ].stat, delta, 0, 0, buff_duration > timespan_t::zero() );
    else if ( delta < 0 )
      player -> stat_loss( stats[ i ].stat, std::fabs( delta ), 0, 0, buff_duration > timespan_t::zero() );
    else
      assert( delta == 0 );
    stats[ i ].current_value += delta;
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
    if ( as<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ] -> update( false, sim -> current_time );

    for ( size_t i = 0; i < stats.size(); ++i )
    {
      double delta = stats[ i ].amount * stacks;
      if ( delta > 0 )
        player -> stat_loss( stats[ i ].stat, ( delta <= stats[ i ].current_value ) ? delta : 0.0, 0, 0, buff_duration > timespan_t::zero() );
      else if ( delta < 0 )
        player -> stat_gain( stats[ i ].stat, ( delta >= stats[ i ].current_value ) ? std::fabs( delta ) : 0.0, 0, 0, buff_duration > timespan_t::zero() );
      stats[ i ].current_value -= delta;
    }
    current_stack -= stacks;

    invalidate_cache();

    if ( as<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ] -> update( true, sim -> current_time );

    if ( sim -> debug )
      sim -> out_debug.printf( "buff %s decremented by %d to %d stacks",
                     name_str.c_str(), stacks, current_stack );

    if ( player ) player -> trigger_ready();
  }
}

// stat_buff_t::expire ======================================================

void stat_buff_t::expire_override()
{
  for ( size_t i = 0; i < stats.size(); ++i )
  {
    if ( stats[ i ].current_value > 0 )
      player -> stat_loss( stats[ i ].stat, stats[ i ].current_value, 0, 0, buff_duration > timespan_t::zero() );
    else if ( stats[ i ].current_value < 0 )
      player -> stat_gain( stats[ i ].stat, std::fabs( stats[ i ].current_value ), 0, 0, buff_duration > timespan_t::zero() );
    stats[ i ].current_value = 0;
  }

  buff_t::expire_override();
}

// ==========================================================================
// COST_REDUCTION_BUFF
// ==========================================================================

cost_reduction_buff_t::cost_reduction_buff_t( const cost_reduction_buff_creator_t& params ) :
  buff_t( params ), amount( params._amount ), school( params._school )
{
  // Detect school / amount
  if ( params._school == SCHOOL_NONE )
  {
    for ( size_t i = 1, e = data().effect_count(); i <= e; i++ )
    {
      const spelleffect_data_t& effect = data().effectN( i );
      if ( effect.type() != E_APPLY_AURA || effect.subtype() != A_MOD_POWER_COST_SCHOOL )
        continue;

      school = dbc::get_school_type( effect.misc_value1() );
      if ( params._amount == 0 )
      {
        if ( params.item )
          amount = util::round( effect.average( params.item ) );
        else
          amount = effect.average( player, std::min( MAX_LEVEL, player -> level ) );

        amount = std::fabs( amount );
      }
      break;
    }
  }
}

// cost_reduction_buff_t::bump ==============================================

void cost_reduction_buff_t::bump( int stacks, double value )
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

void cost_reduction_buff_t::expire_override()
{
  player -> cost_reduction_loss( school, current_value );

  buff_t::expire_override();
}

// ==========================================================================
// HASTE_BUFF
// ==========================================================================

haste_buff_t::haste_buff_t( const haste_buff_creator_t& params ) :
  buff_t( params )
{
  add_invalidate( CACHE_HASTE );
  requires_invalidation = true;
}

// haste_buff_t::execute ====================================================

void haste_buff_t::execute( int stacks, double value, timespan_t duration )
{
  int is_up = check();
  double old_attack_speed = 0;

  if ( ! is_up && ( player -> main_hand_attack || player -> off_hand_attack ) )
    old_attack_speed = player -> cache.attack_speed();

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

void haste_buff_t::expire_override()
{
  double old_attack_speed = 0;

  if ( player -> main_hand_attack || player -> off_hand_attack )
    old_attack_speed = player -> cache.attack_speed();

  buff_t::expire_override();

  // Up -> Down, slow down remaining swing speeds
  if ( player -> main_hand_attack )
    player -> main_hand_attack -> reschedule_auto_attack( old_attack_speed );
  if ( player -> off_hand_attack )
    player -> off_hand_attack -> reschedule_auto_attack( old_attack_speed );
}

// tick_buff_t::trigger =====================================================
/*
bool tick_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  assert( period > timespan_t::zero() );

  if ( duration == timespan_t::min() )
  {
    assert( buff_duration > timespan_t::zero() );
    duration = buff_duration;
  }

  if ( remains() > timespan_t::zero() )
  {
    timespan_t carryover = remains() % period;
    assert( carryover < period );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s carryover duration from ongoing tick: %f", name(), carryover.total_seconds() );

    duration += carryover;
  }

  return buff_t::trigger( stacks, value, chance, duration );
}
*/
// ==========================================================================
// DEBUFF
// ==========================================================================

debuff_t::debuff_t( const buff_creator_basics_t& params ) :
  buff_t( params )
{}

// Absorb Buff


absorb_buff_t::absorb_buff_t( const absorb_buff_creator_t& params ) :
  buff_t( params ),
  absorb_school( params._absorb_school ),
  absorb_source( params._absorb_source ),
  absorb_gain( params._absorb_gain )
{
  assert( player && "Absorb Buffs only supported for player!" );

  if ( absorb_source )
    absorb_source -> type = STATS_ABSORB;

  if ( params._absorb_school == SCHOOL_CHAOS )
  {
    for ( size_t i = 1, e = data().effect_count(); i <= e; i++ )
    {
      const spelleffect_data_t& effect = data().effectN( i );
      if ( effect.type() != E_APPLY_AURA || effect.subtype() != A_SCHOOL_ABSORB )
        continue;

      absorb_school = dbc::get_school_type( effect.misc_value1() );
      if ( params._default_value == DEFAULT_VALUE() )
      {
        if ( params.item )
          default_value = util::round( effect.average( params.item ) );
        else
          default_value = effect.average( player, std::min( MAX_LEVEL, player -> level ) );
      }
      break;
    }
  }
}

void absorb_buff_t::start( int stacks, double value, timespan_t duration )
{
  if ( max_stack() == 0 ) return;
  buff_t::start( stacks, value, duration );

  assert( range::find( player -> absorb_buff_list, this ) == player -> absorb_buff_list.end()
          && "Attempting to add absorb buff to absorb_buffs list twice" );

  player -> absorb_buff_list.push_back( this );
}

void absorb_buff_t::expire_override()
{
  buff_t::expire_override();

  std::vector<absorb_buff_t*>::iterator it = range::find( player -> absorb_buff_list, this );
  if ( it != player -> absorb_buff_list.end() )
    player -> absorb_buff_list.erase( it );
}

void absorb_buff_t::consume( double amount )
{
  if ( absorb_source )
    absorb_source -> add_result( amount, 0, ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, player );

  if ( absorb_gain )
    absorb_gain -> add( RESOURCE_HEALTH, amount, 0 );

  if ( sim -> debug )
    sim -> out_debug.printf( "%s %s absorbs %.2f", player -> name(), name(), amount );

  current_value -= amount;
  absorb_used( amount );

  if ( current_value <= 0 )
    expire();
  else if ( sim -> debug )
    sim -> out_debug.printf( "%s %s absorb remaining %.2f", player -> name(), name(), current_value );
}

void buff_creator_basics_t::init()
{
  _chance = -1.0;
  _max_stack = -1;
  _duration = timespan_t::min();
  _cooldown = timespan_t::min();
  _period = timespan_t::min();
  _quiet = -1;
  _reverse = -1;
  _activated = -1;
  _behavior = BUFF_TICK_NONE;
  _default_value = buff_t::DEFAULT_VALUE();
}

buff_creator_basics_t::buff_creator_basics_t( actor_pair_t p, const std::string& n, const spell_data_t* sp, const item_t* item ) :
  _player( p ), _sim( p.source->sim ), _name( n ), s_data( sp ), item( item )
{ init(); }

buff_creator_basics_t::buff_creator_basics_t( actor_pair_t p , uint32_t id, const std::string& n, const item_t* item ) :
  _player( p ), _sim( p.source->sim ), _name( n ), s_data( _player.source ? _player.source->find_spell( id ) : spell_data_t::nil() ), item( item )
{ init(); }

buff_creator_basics_t::buff_creator_basics_t( sim_t* s, const std::string& n, const spell_data_t* sp, const item_t* item ) :
  _player( actor_pair_t() ), _sim( s ), _name( n ), s_data( sp ), item( item )
{ init(); }

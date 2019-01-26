// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE

struct buff_expr_t : public expr_t
{
  std::string buff_name;
  action_t* action;
  buff_t* static_buff;
  target_specific_t<buff_t> specific_buff;
  double default_value;

  buff_expr_t( const std::string& n, const std::string& bn, action_t* a, buff_t* b, double default_ = 0 )
    : expr_t( n ), buff_name( bn ), action( a ), static_buff( b ), specific_buff( false ),
      default_value( default_ )
  { }

  virtual buff_t* create() const
  {
    assert( action && "Cannot create dynamic buff expressions without a action." );

    action->player->get_target_data( action->target );
    auto buff = buff_t::find( action->target, buff_name, action->player );
    if ( !buff )
      buff = buff_t::find( action->target, buff_name, action->target );  // Raid debuffs

    if ( !buff )
    {
      action->sim->error(
          "Unable to build buff action expression for action '{}': "
          "Reference to unknown buff/debuff '{}' by player {}.",
          action->name(), buff_name, action->player->name() );
      action->sim->cancel();
      // Prevent segfault
      buff = buff_creator_t( action->player, "dummy" );
    }

    return buff;
  }

  virtual buff_t* buff() const
  {
    if ( static_buff )
      return static_buff;
    buff_t*& buff = specific_buff[ action->target ];
    if ( !buff )
    {
      buff = create();
    }
    return buff;
  }

  bool is_constant( double* v ) override
  {
    bool constant = buff()->s_data != spell_data_t::nil() && !buff()->s_data->ok();

    *v = default_value;

    return constant;
  }
};

struct buff_event_t : public event_t
{
  buff_t* buff;

  buff_event_t( buff_t* b, timespan_t d ) : event_t( *b->sim, d ), buff( b )
  {
  }
};

struct react_ready_trigger_t : public buff_event_t
{
  unsigned stack;

  react_ready_trigger_t( buff_t* b, unsigned s, timespan_t d ) : buff_event_t( b, d ), stack( s )
  {
  }

  virtual const char* name() const override
  {
    return "react_ready_trigger";
  }

  void execute() override
  {
    buff->stack_react_ready_triggers[ stack ] = nullptr;

    if ( buff->player )
      buff->player->trigger_ready();
  }
};

struct expiration_t : public buff_event_t
{
  unsigned stack;

  expiration_t( buff_t* b, unsigned s, timespan_t d ) : buff_event_t( b, d ), stack( s )
  {
  }

  expiration_t( buff_t* b, timespan_t d ) : buff_event_t( b, d ), stack( 0 )
  {
    if ( b->stack_behavior == buff_stack_behavior::ASYNCHRONOUS )
    {
      b->sim->errorf( "{} {} creates asynchronous expiration with no stack count.",
          *buff->player, *buff );
      b->sim->cancel();
    }
  }

  const char* name() const override
  {
    return "buff_expiration";
  }

  virtual void execute() override
  {
    assert( buff->expiration.size() );

    buff->expiration.erase( buff->expiration.begin() );

    if ( buff->stack_behavior == buff_stack_behavior::ASYNCHRONOUS )
      buff->decrement( stack );
    else
      buff->expire();
  }
};

struct tick_t : public buff_event_t
{
  double current_value;
  int current_stacks;
  timespan_t tick_time;

  tick_t( buff_t* b, timespan_t d, double value, int stacks )
    : buff_event_t( b, d ), current_value( value ), current_stacks( stacks ), tick_time( d )
  {
  }

  const char* name() const override
  {
    return "buff_tick";
  }

  void execute() override
  {
    buff->tick_event = nullptr;
    buff->current_tick++;

    // For tick number calculations, always include the +1ms so we get correct
    // tick number labeling on the last tick, just before the buff expires.
    int total_ticks =
        buff->expiration.empty() ? -1 : buff->current_tick + static_cast<int>( buff->remains() / buff->tick_time() );

    buff->sim->print_debug( "{} {} ticks ({} of {}).",
        *buff->player, *buff, buff->current_tick, total_ticks );

    // Tick callback is called before the aura stack count is altered to ensure
    // that the buff is always up during the "tick". Last tick detection can be
    // made through the int arguments passed to the function call.
    if ( buff->tick_callback )
      buff->tick_callback( buff, total_ticks, tick_time );

    if ( !( buff->freeze_stacks() ) )
    {
      if ( !buff->reverse )
        buff->bump( current_stacks, current_value );
      else
        buff->decrement( current_stacks, current_value );
    }
    timespan_t period = buff->tick_time();
    if ( buff->current_stack > 0 &&
         ( buff->remains() >= period || buff->buff_duration == timespan_t::zero() ) )
    {
      // Reorder the last tick to happen 1ms before expiration
      if ( buff->remains() == period )
        period -= timespan_t::from_millis( 1 );
      buff->tick_event = make_event<tick_t>( *buff->sim, buff, period, current_value, current_stacks );
    }
  }
};

struct buff_delay_t : public buff_event_t
{
  double value;
  timespan_t duration;
  int stacks;

  buff_delay_t( buff_t* b, int stacks, double value, timespan_t d )
    : buff_event_t( b, b->rng().gauss( b->sim->default_aura_delay, b->sim->default_aura_delay_stddev ) ),
      value( value ),
      duration( d ),
      stacks( stacks )
  {
  }

  const char* name() const override
  {
    return "buff_delay_event";
  }

  virtual void execute() override
  {
    // Add a Cooldown check here to avoid extra processing due to delays
    if ( buff->cooldown->remains() == timespan_t::zero() )
      buff->execute( stacks, value, duration );
    buff->delay = nullptr;
  }
};

struct expiration_delay_t : public buff_event_t
{
  expiration_delay_t( buff_t* b, timespan_t d ) : buff_event_t( b, d )
  {
  }

  virtual void execute() override
  {
    buff->expiration_delay = nullptr;
    // Call real expire after a delay
    buff->expire();
  }
};

expr_t* create_buff_expression( std::string buff_name, const std::string& type, action_t* action, buff_t* static_buff )
{
  if ( !static_buff && !action )
  {
    assert( false && "Buff expressions require either a valid action or a valid static buff" );
    return nullptr;
  }

  if ( type == "duration" )
  {
    struct duration_expr_t : public buff_expr_t
    {
      duration_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_duration", bn, a, b )
      { }

      double evaluate() override
      { return buff()->buff_duration.total_seconds(); }
    };

    return new duration_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "remains" )
  {
    struct remains_expr_t : public buff_expr_t
    {
      remains_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_remains", bn, a, b )
      { }

      double evaluate() override
      { return buff()->remains().total_seconds(); }
    };

    return new remains_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "cooldown_remains" )
  {
    struct cooldown_remains_expr_t : public buff_expr_t
    {
      cooldown_remains_expr_t( std::string bn, action_t* a, buff_t* b )
        : buff_expr_t( "buff_cooldown_remains", bn, a, b )
      { }

      double evaluate() override
      { return buff()->cooldown->remains().total_seconds(); }
    };

    return new cooldown_remains_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "up" )
  {
    struct up_expr_t : public buff_expr_t
    {
      up_expr_t( std::string bn, action_t* a, buff_t* b ) : buff_expr_t( "buff_up", bn, a, b )
      { }

      double evaluate() override
      { return buff()->check() > 0; }
    };

    return new up_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "down" )
  {
    struct down_expr_t : public buff_expr_t
    {
      down_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_down", bn, a, b, 1.0 )
      { }

      double evaluate() override
      { return buff()->check() <= 0; }
    };

    return new down_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "stack" )
  {
    struct stack_expr_t : public buff_expr_t
    {
      stack_expr_t( std::string bn, action_t* a, buff_t* b ) : buff_expr_t( "buff_stack", bn, a, b )
      { }

      double evaluate() override
      { return buff()->check(); }
    };

    return new stack_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "stack_pct" )
  {
    struct stack_pct_expr_t : public buff_expr_t
    {
      stack_pct_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_stack_pct", bn, a, b )
      { }

      double evaluate() override
      { return 100.0 * buff()->check() / buff()->max_stack(); }
    };

    return new stack_pct_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "max_stack" )
  {
    struct max_stack_expr_t : public buff_expr_t
    {
      max_stack_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_max_stack", bn, a, b, 1.0 )
      { }

      double evaluate() override
      { return buff()->max_stack(); }
    };

    return new max_stack_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "value" )
  {
    struct value_expr_t : public buff_expr_t
    {
      value_expr_t( std::string bn, action_t* a, buff_t* b ) : buff_expr_t( "buff_value", bn, a, b )
      { }

      double evaluate() override
      { return buff()->value(); }
    };

    return new value_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "react" )
  {
    struct react_expr_t : public buff_expr_t
    {
      react_expr_t( std::string bn, action_t* a, buff_t* b ) : buff_expr_t( "buff_react", bn, a, b )
      {
        if ( b )
          b->reactable = true;
      }

      buff_t* create() const override
      {
        auto b = buff_expr_t::create();
        b->reactable = true;
        return b;
      }

      double evaluate() override
      { return buff()->stack_react(); }
    };

    return new react_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "react_pct" )
  {
    struct react_pct_expr_t : public buff_expr_t
    {
      react_pct_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_react_pct", bn, a, b )
      {
        if ( b )
          b->reactable = true;
      }

      buff_t* create() const override
      {
        auto b = buff_expr_t::create();
        b->reactable = true;
        return b;
      }

      double evaluate() override
      { return 100.0 * buff()->stack_react() / buff()->max_stack(); }
    };

    return new react_pct_expr_t( buff_name, action, static_buff );
  }
  else if ( type == "cooldown_react" )
  {
    struct cooldown_react_expr_t : public buff_expr_t
    {
      cooldown_react_expr_t( std::string bn, action_t* a, buff_t* b ) :
        buff_expr_t( "buff_cooldown_react", bn, a, b )
      { }

      double evaluate() override
      {
        if ( buff()->check() && !buff()->may_react() )
          return 0;
        else
          return buff()->cooldown->remains().total_seconds();
      }
    };

    return new cooldown_react_expr_t( buff_name, action, static_buff );
  }

  throw std::invalid_argument( fmt::format( "Unsupported buff expression '{}'.", type ) );
  return nullptr;
}

}  // namespace

buff_t::buff_t( actor_pair_t q, const std::string& name, const spell_data_t* spell_data, const item_t* item )
  : buff_t( buff_creation::buff_creator_basics_t( q, name, spell_data, item ) )
{
}
buff_t::buff_t( sim_t* sim, const std::string& name, const spell_data_t* spell_data, const item_t* item )
  : buff_t( buff_creation::buff_creator_basics_t( sim, name, spell_data, item ) )
{
}

buff_t::buff_t( const buff_creation::buff_creator_basics_t& params )
  : sim( params._sim ),
    player( params._player.target ),
    item( params.item ),
    name_str( params._name ),
    s_data( params.s_data ),
    source( params._player.source ),
    expiration(),
    delay(),
    expiration_delay(),
    cooldown(),
    rppm( nullptr ),
    _max_stack( params._max_stack ),
    trigger_data( s_data ),
    default_value( DEFAULT_VALUE() ),
    activated( true ),
    reactable( false ),
    reverse(),
    constant(),
    quiet(),
    overridden(),
    can_cancel( true ),
    requires_invalidation(),
    reverse_stack_reduction( 1 ),
    current_value(),
    current_stack(),
    buff_duration( params._duration ),
    default_chance( 1.0 ),
    manual_chance( params._chance ),
    current_tick( 0 ),
    buff_period( timespan_t::min() ),
    tick_time_behavior( buff_tick_time_behavior::UNHASTED ),
    tick_behavior( buff_tick_behavior::NONE ),
    tick_event( nullptr ),
    tick_zero( false ),
    last_start( timespan_t() ),
    last_trigger( timespan_t() ),
    iteration_uptime_sum( timespan_t() ),
    last_benefite_update( timespan_t() ),
    up_count(),
    down_count(),
    start_count(),
    refresh_count(),
    expire_count(),
    overflow_count(),
    overflow_total(),
    trigger_attempts(),
    trigger_successes(),
    simulation_max_stack( 0 ),
    invalidate_list(),
    haste_type( HASTE_NONE ),
    benefit_pct(),
    trigger_pct(),
    avg_start(),
    avg_refresh(),
    avg_expire(),
    avg_overflow_count(),
    avg_overflow_total(),
    uptime_pct(),
    start_intervals(),
    trigger_intervals(),
    change_regen_rate( false )
{
  if ( source )  // Player Buffs
  {
    player->buff_list.push_back( this );
    cooldown = source->get_cooldown( "buff_" + name_str );
  }
  else  // Sim Buffs
  {
    sim->buff_list.push_back( this );
    cooldown = sim->get_cooldown( "buff_" + name_str );
  }

  // Set Buff duration
  set_duration( buff_duration );

  // Set Buff Cooldown
  set_cooldown( params._cooldown );

  set_trigger_spell( params._trigger_data );

  // If there's no overridden proc chance (%), setup any potential custom RPPM-affecting attribute
  if ( params._chance == -1 )
  {
    set_rppm( params._rppm_scale, params._rppm_freq, params._rppm_mod );
  }

  set_default_value( params._default_value );

  // Set Reverse flag
  if ( params._reverse != -1 )
    set_reverse( params._reverse );

  // Set Quiet flag
  if ( params._quiet != -1 )
    set_quiet( params._quiet );

  // Set Activated flag
  if ( params._activated != -1 )
    set_activated( params._activated );

  if ( params._can_cancel != -1 )
    set_can_cancel( params._can_cancel );

  set_period( params._period );

  set_tick_behavior( params._tick_behavior );

  if ( params._tick_callback )
    set_tick_callback( std::move( params._tick_callback ) );

  set_tick_zero( params._initial_tick );
  set_tick_time_behavior( params._tick_time_behavior );
  if ( tick_time_behavior == buff_tick_time_behavior::CUSTOM )
  {
    assert( params._tick_time_callback );
    set_tick_time_callback( params._tick_time_callback );
  }

  if ( params._affects_regen == -1 && player && player->regen_type == REGEN_DYNAMIC )
  {
    for ( auto& elem : params._invalidate_list )
    {
      if ( player->regen_caches[ elem ] )
        change_regen_rate = true;
    }
  }
  else
  {
    set_affects_regen( params._affects_regen );
  }

  if ( params._refresh_duration_callback )
    set_refresh_duration_callback( params._refresh_duration_callback );
  set_refresh_behavior( params._refresh_behavior );

  set_stack_behavior( params._stack_behavior );

  assert( refresh_behavior != buff_refresh_behavior::CUSTOM || refresh_duration_callback );

  invalidate_list       = params._invalidate_list;
  requires_invalidation = !invalidate_list.empty();
  init_haste_type();

  if ( player && !player->cache.active )
    requires_invalidation = false;

  set_stack_change_callback( params._stack_change_callback );

  set_max_stack( _max_stack );

  update_trigger_calculations();
}

void buff_t::update_trigger_calculations()
{
  // No override of proc chance or RPPM-related attributes, setup the buff object's proc chance. The
  // spell used for the attributes is either the buff spell (by default), or the given trigger_spell
  // in buff_creator_t.
  if ( manual_chance == -1 )
  {
    default_chance = 1.0;
    if ( !rppm )
    {
      if ( trigger_data->ok() )
      {
        if ( trigger_data->real_ppm() > 0 )
        {
          rppm = player->get_rppm( "buff_" + name_str + "_rppm", trigger_data, item );
        }
        else if ( trigger_data->proc_chance() != 0 )
        {
          default_chance = trigger_data->proc_chance();
        }
      }
      // Note, if the spell is "not found", then the buff is disabled.  This allows the system to
      // easily enable/disable spells based on conditional things (such as talents, artifacts,
      // specialization, etc.).
      else if ( !trigger_data->found() )
      {
        default_chance = 0.0;
      }
    }
  }
  else
  {
    default_chance = manual_chance;
    rppm           = nullptr;
  }
}

buff_t* buff_t::set_chance( double chance )
{
  manual_chance = chance;
  update_trigger_calculations();
  return this;
}

buff_t* buff_t::set_duration( timespan_t duration )
{
  // Set Buff duration
  if ( duration == timespan_t::min() )
  {
    if ( data().ok() )
    {
      buff_duration = data().duration();
    }
    else
    {
      buff_duration = timespan_t();
    }
  }
  else
  {
    buff_duration = duration;
  }

  if ( buff_duration < timespan_t::zero() )
  {
    buff_duration = timespan_t::zero();
  }
  return this;
}

buff_t* buff_t::set_max_stack( int max_stack )
{
  // Set Max stacks
  if ( max_stack == -1 )
  {
    if ( data().ok() )
    {
      if ( data().max_stacks() != 0 )
      {
        _max_stack = data().max_stacks();
      }
      else if ( data().initial_stacks() != 0 )
      {
        _max_stack = std::abs( data().initial_stacks() );
      }
      else
      {
        _max_stack = 1;
      }
    }
    else
    {
      _max_stack = 1;
    }
  }
  else
  {
    _max_stack = max_stack;
  }

  if ( _max_stack < 1 )
  {
    sim->error( "{} initialized with max_stack < 1 ({}). Setting max_stack to 1.\n", *this, _max_stack );
    _max_stack = 1;
  }

  if ( _max_stack > 999 )
  {
    _max_stack = 999;
    sim->errorf( "{} initialized with max_stack > 999. Setting max_stack to 999.\n", *this );
  }

  stack_react_time.resize( _max_stack + 1 );
  stack_react_ready_triggers.resize( _max_stack + 1 );

  if ( as<int>( stack_uptime.size() ) < _max_stack + 1 )
  {
    stack_uptime.resize( _max_stack + 1 );
  }
  return this;
}

buff_t* buff_t::set_cooldown( timespan_t duration )
{
  // Set Buff duration
  if ( duration == timespan_t::min() )
  {
    if ( data().ok() )
    {
      if ( data().cooldown() != timespan_t::zero() )
      {
        cooldown->duration = data().cooldown();
      }
      else
      {
        cooldown->duration = data().internal_cooldown();
      }
    }
  }
  else
  {
    cooldown->duration = duration;
  }

  if ( cooldown->duration < timespan_t::zero() )
  {
    cooldown->duration = timespan_t::zero();
  }

  return this;
}

buff_t* buff_t::set_period( timespan_t period )
{
  if ( period >= timespan_t::zero() )
  {
    buff_period = period;
  }
  else
  {
    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      const spelleffect_data_t& e = s_data->effectN( i );
      if ( !e.ok() || e.type() != E_APPLY_AURA )
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
        default:
          break;
      }
    }
  }

  // Recheck tick behaviour, which is dependent on buff_period.
  set_tick_behavior( tick_behavior );

  return this;
}

buff_t* buff_t::add_invalidate( cache_e c )
{
  if ( c == CACHE_NONE )
  {
    return this;
  }

  if ( range::find( invalidate_list, c ) == invalidate_list.end() )  // avoid duplication
  {
    invalidate_list.push_back( c );
    requires_invalidation = true;
    if ( !player || player->regen_caches[ c ] )
    {
      change_regen_rate = true;
    }
    init_haste_type();
  }
  return this;
}

buff_t* buff_t::set_default_value( double value )
{
  default_value = value;
  return this;
}

buff_t* buff_t::set_reverse( bool r )
{
  reverse = r;
  return this;
}

buff_t* buff_t::set_quiet( bool q )
{
  quiet = q;
  return this;
}

buff_t* buff_t::set_activated( bool a )
{
  activated = a;
  return this;
}

buff_t* buff_t::set_can_cancel( bool cc )
{
  can_cancel = cc;
  return this;
}

buff_t* buff_t::set_tick_behavior( buff_tick_behavior behavior )
{
  tick_behavior = behavior;

  // If period is set, but no buff tick behavior, set the behavior automatically to clipped ticks
  if ( behavior == buff_tick_behavior::NONE && buff_period > timespan_t::zero() )
  {
    tick_behavior = buff_tick_behavior::CLIP;
  }

  return this;
}

buff_t* buff_t::set_tick_callback( buff_tick_callback_t fn )
{
  tick_callback = std::move( fn );
  return this;
}

buff_t* buff_t::set_tick_time_callback( buff_tick_time_callback_t cb )
{
  set_tick_time_behavior( buff_tick_time_behavior::CUSTOM );
  tick_time_callback = std::move( cb );
  return this;
}

buff_t* buff_t::set_affects_regen( bool state )
{
  change_regen_rate = state;
  return this;
}

buff_t* buff_t::set_refresh_behavior( buff_refresh_behavior b )
{
  if ( b == buff_refresh_behavior::NONE )
  {
    // In wod, default behavior for ticking buffs is to pandemic-extend the duration
    if ( tick_behavior == buff_tick_behavior::CLIP || tick_behavior == buff_tick_behavior::REFRESH )
    {
      refresh_behavior = buff_refresh_behavior::PANDEMIC;
    }
    // Otherwise, just do the full-duration refresh
    else
    {
      refresh_behavior = buff_refresh_behavior::DURATION;
    }
  }
  else
  {
    refresh_behavior = b;
  }
  assert( refresh_behavior != buff_refresh_behavior::CUSTOM || refresh_duration_callback );
  return this;
}

buff_t* buff_t::set_refresh_duration_callback( buff_refresh_duration_callback_t cb )
{
  if ( cb )
  {
    refresh_behavior          = buff_refresh_behavior::CUSTOM;
    refresh_duration_callback = std::move( cb );
  }
  return this;
}

buff_t* buff_t::set_rppm( rppm_scale_e scale, double freq, double mod )
{
  if ( scale == RPPM_DISABLE )
  {
    rppm = nullptr;
    return this;
  }

  if ( freq > -1 )
  {
    rppm = player->get_rppm( "buff_" + name_str + "_rppm", trigger_data, item );
    rppm->set_frequency( freq );
  }

  if ( mod > -1 )
  {
    rppm = player->get_rppm( "buff_" + name_str + "_rppm", trigger_data, item );
    rppm->set_modifier( mod );
  }

  if ( scale != RPPM_NONE )
  {
    rppm = player->get_rppm( "buff_" + name_str + "_rppm", trigger_data, item );
    rppm->set_scaling( scale );
  }
  return this;
}

buff_t* buff_t::set_trigger_spell( const spell_data_t* s )
{
  // If the params specifies a trigger spell (even if it's not found), use it instead of the actual
  // spell data of the buff.
  if ( s != spell_data_t::nil() )
  {
    trigger_data = s;
  }
  update_trigger_calculations();
  return this;
}

buff_t* buff_t::set_stack_change_callback( const buff_stack_change_callback_t& cb )
{
  stack_change_callback = cb;
  return this;
}

buff_t* buff_t::set_reverse_stack_count( int count )
{
  reverse_stack_reduction = count;
  return this;
}

buff_t* buff_t::set_stack_behavior( buff_stack_behavior b )
{
  stack_behavior = b;
  return this;
}

void buff_t::datacollection_begin()
{
  iteration_uptime_sum = timespan_t::zero();

  up_count = down_count = 0;
  trigger_successes = trigger_attempts = 0;
  start_count                          = 0;
  refresh_count                        = 0;
  expire_count                         = 0;
  overflow_count                       = 0;
  overflow_total                       = 0;

  for ( int i = 0; i <= simulation_max_stack; i++ )
    stack_uptime[ i ].datacollection_begin();
}

void buff_t::datacollection_end()
{
  // Debuffs need to ensure that the source is active (when single_actor_batch=1) to ensure that
  // reporting stays correct.
  if ( sim->single_actor_batch && source != player )
  {
    if ( !source->is_enemy() && source != sim->player_no_pet_list[ sim->current_index ] )
    {
      return;
    }
  }

  timespan_t time = player ? player->iteration_fight_length : sim->current_time();

  uptime_pct.add( time != timespan_t::zero() ? 100.0 * iteration_uptime_sum / time : 0 );

  for ( int i = 0; i <= simulation_max_stack; i++ )
    stack_uptime[ i ].datacollection_end( time );

  if ( up_count > 0 )
  {
    benefit_pct.add( 100.0 * up_count / ( up_count + down_count ) );
  }

  if ( trigger_attempts > 0 )
  {
    trigger_pct.add( 100.0 * trigger_successes / trigger_attempts );
  }

  avg_start.add( start_count );
  avg_refresh.add( refresh_count );
  avg_expire.add( expire_count );
  avg_overflow_count.add( overflow_count );
  avg_overflow_total.add( overflow_total );
}

timespan_t buff_t::refresh_duration( const timespan_t& new_duration ) const
{
  switch ( refresh_behavior )
  {
    case buff_refresh_behavior::DISABLED:
      return timespan_t::zero();
    case buff_refresh_behavior::DURATION:
      return new_duration;
    case buff_refresh_behavior::TICK:
    {
      assert( tick_event );
      timespan_t residual = remains() % static_cast<tick_t*>( tick_event )->tick_time;
      sim->print_debug(
            "{} {} carryover duration from ongoing tick: {}, refresh_duration={} new_duration={}",
            *player, *this,
            residual, new_duration, ( new_duration + residual ) );

      return new_duration + residual;
    }
    case buff_refresh_behavior::PANDEMIC:
    {
      timespan_t residual = std::min( new_duration * 0.3, remains() );
      sim->print_debug(
            "{} {} carryover duration from ongoing tick: {}, refresh_duration={} new_duration={}",
            *player, *this,
            residual, new_duration, ( new_duration + residual ) );

      return new_duration + residual;
    }
    case buff_refresh_behavior::EXTEND:
      return remains() + new_duration;
    case buff_refresh_behavior::CUSTOM:
      return refresh_duration_callback( this, new_duration );
    default:
    {
      assert( false );
      return new_duration;
    }
  }
}

timespan_t buff_t::tick_time() const
{
  switch ( tick_time_behavior )
  {
    case buff_tick_time_behavior::HASTED:
      assert( player );
      return buff_period * player->cache.spell_speed();
    case buff_tick_time_behavior::CUSTOM:
      assert( tick_time_callback );
      return tick_time_callback( this, current_tick );
    default:
      return buff_period;
  }
}

int buff_t::stack()
{
  int cs = current_stack;
  if ( last_benefite_update != sim->current_time() )
  {
    // make sure we only record a benfit once per sim event
    last_benefite_update = sim->current_time();
    if ( cs > 0 )
      up_count++;
    else
      down_count++;
  }
  return cs;
}

int buff_t::total_stack()
{
  int s = current_stack;

  if ( delay )
    s += debug_cast<buff_delay_t*>( delay )->stacks;

  return std::min( s, _max_stack );
}

bool buff_t::may_react( int stack )
{
  if ( current_stack == 0 )
    return false;
  if ( stack > current_stack )
    return false;
  if ( stack < 1 )
    return false;
  if ( !reactable )
    return false;

  if ( stack > _max_stack )
    return false;

  return sim->current_time() > stack_react_time[ stack ];
}

int buff_t::stack_react()
{
  int stack = current_stack;

  for ( int i = current_stack; i >= 1; i-- )
  {
    if ( stack_react_time[ i ] <= sim->current_time() )
      break;
    stack--;
  }

  return stack;
}

timespan_t buff_t::remains() const
{
  if ( current_stack <= 0 )
  {
    return timespan_t::zero();
  }
  if ( !expiration.empty() )
  {
    return expiration.back()->occurs() - sim->current_time();
  }
  return timespan_t::min();
}

bool buff_t::remains_gt( timespan_t time ) const
{
  timespan_t time_remaining = remains();

  if ( time_remaining == timespan_t::zero() )
    return false;

  if ( time_remaining == timespan_t::min() )
    return true;

  return ( time_remaining > time );
}

bool buff_t::remains_lt( timespan_t time ) const
{
  timespan_t time_remaining = remains();

  if ( time_remaining == timespan_t::min() )
    return false;

  return ( time_remaining < time );
}

bool buff_t::trigger( action_t* a, int stacks, double value, timespan_t duration )
{
  double chance = default_chance;
  if ( chance < 0 )
    chance = a->ppm_proc_chance( -chance );
  return trigger( stacks, value, chance, duration );
}

bool buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  if ( _max_stack == 0 || chance == 0 )
    return false;

  if ( cooldown->down() )
    return false;

  if ( player && player->is_sleeping() )
    return false;

  trigger_attempts++;

  if ( rppm )
  {
    double c = chance;
    if ( chance > 0 )
    {
      c = rppm->get_frequency();
      rppm->set_frequency( chance );
    }

    bool triggered = rppm->trigger();

    if ( chance > 0 )
    {
      rppm->set_frequency( c );
    }

    if ( !triggered )
    {
      return false;
    }
  }
  else
  {
    if ( chance < 0 )
      chance = default_chance;

    if ( !rng().roll( chance ) )
      return false;
  }

  if ( ( !activated || stack_behavior == buff_stack_behavior::ASYNCHRONOUS ) && player && player->in_combat &&
       sim->default_aura_delay > timespan_t::zero() )
  {
    // In-game, procs that happen "close to eachother" are usually delayed into the
    // same time slot. We roughly model this by allowing procs that happen during the
    // buff's already existing delay period to trigger at the same time as the first
    // delayed proc will happen.
    if ( delay )
    {
      buff_delay_t& d = *static_cast<buff_delay_t*>( delay );
      d.stacks += stacks;
      d.value = value;
    }
    else
      delay = make_event<buff_delay_t>( *sim, this, stacks, value, duration );
  }
  else
    execute( stacks, value, duration );

  return true;
}

void buff_t::execute( int stacks, double value, timespan_t duration )
{
  if ( value == DEFAULT_VALUE() && default_value != DEFAULT_VALUE() )
    value = default_value;

  if ( last_trigger > timespan_t::zero() )
  {
    trigger_intervals.add( ( sim->current_time() - last_trigger ).total_seconds() );
  }
  last_trigger = sim->current_time();

  if ( data().id() > 0 )
  {
    expansion::bfa::trigger_leyshocks_grand_compilation( data().id(), source );
  }

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
  if ( cooldown->duration > timespan_t::zero() )
  {
    sim->print_debug( "{} starts {} cooldown ({}) with duration {}", source_name(),
                             *this, cooldown->name(), cooldown->duration );

    cooldown->start();
  }

  trigger_successes++;
}

void buff_t::increment( int stacks, double value, timespan_t duration )
{
  if ( overridden )
    return;

  if ( _max_stack == 0 )
    return;

  if ( current_stack == 0 || stack_behavior == buff_stack_behavior::ASYNCHRONOUS )
  {
    start( stacks, value, duration );
  }
  else
  {
    if ( value == DEFAULT_VALUE() )
      value = current_value;

    refresh( stacks, value, duration );
  }
}

void buff_t::decrement( int stacks, double value )
{
  if ( overridden )
    return;

  if ( _max_stack == 0 || current_stack <= 0 )
    return;

  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    adjust_haste();

    int old_stack = current_stack;

    if ( requires_invalidation )
      invalidate_cache();

    if ( as<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ].update( false, sim->current_time() );

    current_stack -= stacks;

    if ( value != DEFAULT_VALUE() )
      current_value = value;

    if ( as<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ].update( true, sim->current_time() );

    sim->print_debug( "{} decremented by {} to {} stacks.", *this, stacks, current_stack );

    if ( old_stack != current_stack && stack_change_callback )
    {
      stack_change_callback( this, old_stack, current_stack );
    }
  }
}

void buff_t::extend_duration( player_t* p, timespan_t extra_seconds )
{
  if ( !check() )
  {
    return;
  }

  if ( stack_behavior == buff_stack_behavior::ASYNCHRONOUS )
  {
    throw std::runtime_error( fmt::format( "'{}' attempts to extend asynchronous buff '{}'.", p->name(), name() ) );
  }

  assert( expiration.size() == 1 );

  if ( extra_seconds > timespan_t::zero() )
  {
    expiration.front()->reschedule( expiration.front()->remains() + extra_seconds );

    sim->print_log( "{} extends {} by {}. New expiration time: {}",
        *p, *this, extra_seconds, expiration.front()->occurs().total_seconds() );
  }
  else if ( extra_seconds < timespan_t::zero() )
  {
    timespan_t reschedule_time = expiration.front()->remains() + extra_seconds;

    if ( reschedule_time <= timespan_t::zero() )
    {
      // When Strength of Soul removes the Weakened Soul debuff completely,
      // there's a delay before the server notifies the client. Modeling
      // this effect as a world lag.
      timespan_t lag, dev;

      lag             = p->world_lag_override ? p->world_lag : sim->world_lag;
      dev             = p->world_lag_stddev_override ? p->world_lag_stddev : sim->world_lag_stddev;
      reschedule_time = rng().gauss( lag, dev );
    }

    event_t::cancel( expiration.front() );
    expiration.erase( expiration.begin() );

    expiration.push_back( make_event<expiration_t>( *sim, this, reschedule_time ) );

    sim->print_debug( "{} decreases {} by {}. New expiration: {}",
        *p, *this, -extra_seconds, expiration.back()->occurs().total_seconds() );
  }
}

void buff_t::start( int stacks, double value, timespan_t duration )
{
  if ( _max_stack == 0 )
    return;

#ifndef NDEBUG
  if ( stack_behavior != buff_stack_behavior::ASYNCHRONOUS && current_stack != 0 )
  {
    sim->error( "buff_t::start assertion error current_stack is not zero, {} from {}.\n", *this,
                 source_name() );
    assert( false );
  }
#endif

  timespan_t d = ( duration >= timespan_t::zero() ) ? duration : buff_duration;

  if ( sim->current_time() <= timespan_t::from_seconds( 0.01 ) )
  {
    if ( d == timespan_t::zero() || ( d > timespan_t::from_seconds( sim->expected_max_time() ) ) )
      constant = true;
  }

  start_count++;

  if ( player && change_regen_rate )
    player->do_dynamic_regen();
  else if ( change_regen_rate )
  {
    for ( size_t i = 0, end = sim->player_non_sleeping_list.size(); i < end; i++ )
    {
      player_t* actor = sim->player_non_sleeping_list[ i ];
      if ( actor->regen_type != REGEN_DYNAMIC || actor->is_pet() )
        continue;

      for ( auto& elem : invalidate_list )
      {
        if ( actor->regen_caches[ elem ] )
        {
          actor->do_dynamic_regen();
          break;
        }
      }
    }
  }

  int before_stacks = check();

  bump( stacks, value );

  if ( last_start >= timespan_t::zero() )
  {
    start_intervals.add( ( sim->current_time() - last_start ).total_seconds() );
  }

  if ( before_stacks <= 0 )
  {
    last_start = sim->current_time();
  }

  if ( d > timespan_t::zero() )
  {
    expiration.push_back( make_event<expiration_t>( *sim, this, stacks, d ) );
    if ( expiration.size() > 1 )
    {
      range::sort( expiration, []( const event_t* a, const event_t* b ) {
        return a->remains() < b->remains();
      } );
    }
    /* TOCHECK: This seems wrong, since bump() already removes expiration events when we are at max stacks
    if ( check() == before_stacks && stack_behavior == buff_stack_behavior::ASYNCHRONOUS )
    {
      event_t::cancel( expiration.front() );
      expiration.erase( expiration.begin() );
    }
    */
  }

  timespan_t period = tick_time();
  if ( tick_behavior != buff_tick_behavior::NONE && period > timespan_t::zero() &&
       ( period <= d || d == timespan_t::zero() ) )
  {
    current_tick = 0;

    // Reorder the last tick to happen 1ms before expiration
    if ( period == d )
      period -= timespan_t::from_millis( 1 );
    assert( !tick_event );
    tick_event = make_event<tick_t>( *sim, this, period, current_value, reverse ? reverse_stack_reduction : stacks );

    if ( tick_zero && tick_callback )
    {
      tick_callback( this, expiration.empty() ? -1 : static_cast<int>( remains() / period ), timespan_t::zero() );
    }
  }
}

void buff_t::refresh( int stacks, double value, timespan_t duration )
{
  if ( _max_stack == 0 )
    return;

  bump( stacks, value );

  refresh_count++;

  timespan_t d;
  if ( duration > timespan_t::zero() )
    d = refresh_duration( duration );
  else if ( duration == timespan_t::zero() )
    d = duration;
  else
    d = refresh_duration( buff_duration );

  if ( refresh_behavior == buff_refresh_behavior::DISABLED && duration != timespan_t::zero() )
    return;

  // Make sure we always cancel the expiration event if we get an
  // infinite duration
  if ( d <= timespan_t::zero() )
  {
    if ( !expiration.empty() )
    {
      event_t::cancel( expiration.front() );
      expiration.erase( expiration.begin() );
    }
    // Infinite ticking buff refreshes shouldnt happen, but cancel ongoing
    // tick event just to be sure.
    event_t::cancel( tick_event );
  }
  else
  {
    // Infinite duration -> duration of d
    if ( expiration.empty() )
      expiration.push_back( make_event<expiration_t>( *sim, this, d ) );
    else
    {
      timespan_t duration_remains = expiration.front()->remains();
      if ( duration_remains < d )
        expiration.front()->reschedule( d );
      else if ( duration_remains > d )
      {
        event_t::cancel( expiration.front() );
        expiration.erase( expiration.begin() );
        expiration.push_back( make_event<expiration_t>( *sim, this, d ) );
      }
    }

    if ( tick_event && tick_behavior == buff_tick_behavior::CLIP )
    {
      event_t::cancel( tick_event );
      current_tick      = 0;
      timespan_t period = tick_time();
      // Reorder the last tick to happen 1ms before expiration
      if ( period == d )
        period -= timespan_t::from_millis( 1 );
      tick_event = make_event<tick_t>( *sim, this, period, current_value, reverse ? 1 : stacks );
    }

    if ( tick_zero && tick_callback )
    {
      tick_callback( this, expiration.empty() ? -1 : static_cast<int>( remains() / tick_time() ), timespan_t::zero() );
    }
  }
}

void buff_t::bump( int stacks, double value )
{
  if ( _max_stack == 0 )
    return;

  bool haste_to_be_adjusted = false; // Flag to check if we need to adjust haste at the end of bump

  if ( value != current_value )
  {
    haste_to_be_adjusted = true;
  }
  current_value = value;

  if ( requires_invalidation )
    invalidate_cache();

  int old_stack = current_stack;

  if ( max_stack() < 0 )
  {
    current_stack += stacks;
    haste_to_be_adjusted = true;
  }
  else if ( current_stack < max_stack() )
  {
    haste_to_be_adjusted = true;
    int before_stack = current_stack;

    current_stack += stacks;
    if ( current_stack > max_stack() )
    {
      int overflow = current_stack - max_stack();

      overflow_count++;
      overflow_total += overflow;
      current_stack = max_stack();

      if ( stack_behavior == buff_stack_behavior::ASYNCHRONOUS )
      {
        // Can't trigger more than max stack at a time
        overflow -= std::max( 0, stacks - max_stack() );

        /* Replace the oldest buff with the new one. We do this by cancelling
        expiration events until their stack count add up to the overflow. */
        while ( overflow > 0 )
        {
          event_t* e     = expiration.front();
          int exp_stacks = debug_cast<expiration_t*>( e )->stack;

          if ( exp_stacks > overflow )
          {
            debug_cast<expiration_t*>( e )->stack -= overflow;
            break;
          }
          else
          {
            event_t::cancel( e );
            expiration.erase( expiration.begin() );
            overflow -= exp_stacks;
          }
        }
      }
    }

    if ( before_stack != current_stack )
    {
      stack_uptime[ before_stack ].update( false, sim->current_time() );
      stack_uptime[ current_stack ].update( true, sim->current_time() );
    }

    aura_gain();

    if ( reactable )
    {
      timespan_t total_reaction_time = ( player ? ( player->total_reaction_time() ) : sim->reaction_time );
      timespan_t react               = sim->current_time() + total_reaction_time;
      for ( int i = before_stack + 1; i <= current_stack; i++ )
      {
        stack_react_time[ i ] = react;
        if ( player && player->ready_type == READY_TRIGGER )
        {
          if ( !stack_react_ready_triggers[ i ] )
          {
            stack_react_ready_triggers[ i ] = make_event<react_ready_trigger_t>( *sim, this, i, total_reaction_time );
          }
          else
          {
            timespan_t next_react = sim->current_time() + total_reaction_time;
            if ( next_react > stack_react_ready_triggers[ i ]->occurs() )
            {
              stack_react_ready_triggers[ i ]->reschedule( next_react );
            }
            else if ( next_react < stack_react_ready_triggers[ i ]->occurs() )
            {
              event_t::cancel( stack_react_ready_triggers[ i ] );
              stack_react_ready_triggers[ i ] = make_event<react_ready_trigger_t>( *sim, this, i, total_reaction_time );
            }
          }
        }
      }
    }

    if ( current_stack > simulation_max_stack )
      simulation_max_stack = current_stack;
  }
  else
  {
    overflow_count++;
    overflow_total += stacks;
  }

  if ( old_stack != current_stack && stack_change_callback )
  {
    stack_change_callback( this, old_stack, current_stack );
  }

  if (haste_to_be_adjusted)
  {
    adjust_haste();
  }

  if ( player )
    player->trigger_ready();
}

void buff_t::override_buff( int stacks, double value )
{
  if ( _max_stack == 0 )
    return;

#ifndef NDEBUG
  if ( current_stack != 0 )
  {
    sim->error( "buff_t::override assertion error current_stack is not zero, {} from {}.\n", *this,
                 source_name() );
    assert( false );
  }
#endif

  if ( value == DEFAULT_VALUE() )
    value = default_value;

  buff_duration = timespan_t::zero();
  start( stacks, value );
  overridden = true;
}

void buff_t::expire( timespan_t delay )
{
  if ( current_stack <= 0 )
    return;

  if ( delay > timespan_t::zero() )  // Expiration Delay
  {
    if ( !expiration_delay )  // Don't reschedule already existing expiration delay
    {
      expiration_delay = make_event<expiration_delay_t>( *sim, this, delay );
    }
    return;
  }
  else
  {
    event_t::cancel( expiration_delay );
  }

  timespan_t remaining_duration = timespan_t::zero();
  int expiration_stacks         = current_stack;
  if ( !expiration.empty() )
  {
    remaining_duration = expiration.back()->remains();

    while ( !expiration.empty() )
    {
      event_t::cancel( expiration.front() );
      expiration.erase( expiration.begin() );
    }
  }
  event_t::cancel( tick_event );

  assert( as<std::size_t>( current_stack ) < stack_uptime.size() );
  stack_uptime[ current_stack ].update( false, sim->current_time() );

  if ( player && change_regen_rate )
    player->do_dynamic_regen();
  else if ( change_regen_rate )
  {
    for ( size_t i = 0, end = sim->player_non_sleeping_list.size(); i < end; i++ )
    {
      player_t* actor = sim->player_non_sleeping_list[ i ];
      if ( actor->regen_type != REGEN_DYNAMIC || actor->is_pet() )
        continue;

      for ( auto& elem : invalidate_list )
      {
        if ( actor->regen_caches[ elem ] )
        {
          actor->do_dynamic_regen();
          break;
        }
      }
    }
  }

  int old_stack = current_stack;

  current_stack = 0;
  if ( requires_invalidation )
    invalidate_cache();
  if ( last_start >= timespan_t::zero() )
  {
    iteration_uptime_sum += sim->current_time() - last_start;
    if ( !constant && !overridden && sim->buff_uptime_timeline )
    {
      timespan_t start_time   = timespan_t::from_seconds( last_start.total_millis() / 1000 );
      timespan_t end_time     = timespan_t::from_seconds( sim->current_time().total_millis() / 1000 );
      timespan_t begin_uptime = ( ( timespan_t::from_seconds( 1 ) - last_start ) % timespan_t::from_seconds( 1 ) );
      timespan_t end_uptime   = ( sim->current_time() % timespan_t::from_seconds( 1 ) );

      if ( last_start % timespan_t::from_seconds( 1 ) == timespan_t::zero() )
        begin_uptime = timespan_t::from_seconds( 1 );

      uptime_array.add( start_time, begin_uptime.total_seconds() );
      for ( timespan_t i = start_time + timespan_t::from_millis( 1000 ); i < end_time;
            i            = i + timespan_t::from_millis( 1000 ) )
        uptime_array.add( i, 1 );

      if ( end_uptime != timespan_t::zero() )
        uptime_array.add( end_time, end_uptime.total_seconds() );
    }
  }

  if ( sim->target->resources.base[ RESOURCE_HEALTH ] == 0 || sim->target->resources.current[ RESOURCE_HEALTH ] > 0 )
    if ( !overridden )
    {
      constant = false;
    }

  if ( reactable && player && player->ready_type == READY_TRIGGER )
  {
    for ( size_t i = 0; i < stack_react_ready_triggers.size(); i++ )
      event_t::cancel( stack_react_ready_triggers[ i ] );
  }

  if ( buff_duration > timespan_t::zero() && remaining_duration == timespan_t::zero() )
  {
    expire_count++;
  }

  expire_override( expiration_stacks, remaining_duration );  // virtual expire call

  current_value = 0;
  aura_loss();

  if ( stack_change_callback )
  {
    stack_change_callback( this, old_stack, current_stack );
  }

  adjust_haste();

  if ( player )
    player->trigger_ready();
}

void buff_t::predict()
{
  // Guarantee that may_react() will return true if the buff is present.
  std::fill( stack_react_time.begin(), stack_react_time.begin() + current_stack + 1, timespan_t::min() );
}

void buff_t::aura_gain()
{
  if ( sim->log )
  {
    std::string buff_display_name = fmt::format("{}_{}", name_str, current_stack );

    if ( player )
    {
      if ( !player->is_sleeping() )
      {
        sim->print_log( "{} gains {} (value={})", *player, buff_display_name, current_value );
      }
    }
    else
    {
      sim->print_log( "Raid gains {} (value={})", buff_display_name, current_value );
    }
  }
}

void buff_t::aura_loss()
{
  if ( player )
  {
    if ( !player->is_sleeping() )
    {
      sim->print_log( "{} loses {}", *player, *this );
    }
  }
  else
  {
    sim->print_log( "Raid loses {}", *this );
  }
}

void buff_t::reset()
{
  event_t::cancel( delay );
  event_t::cancel( expiration_delay );
  event_t::cancel( tick_event );
  cooldown->reset( false );
  expire();
  last_start   = timespan_t::min();
  last_trigger = timespan_t::min();
}

void buff_t::merge( const buff_t& other )
{
  start_intervals.merge( other.start_intervals );
  trigger_intervals.merge( other.trigger_intervals );

  uptime_pct.merge( other.uptime_pct );
  benefit_pct.merge( other.benefit_pct );
  trigger_pct.merge( other.trigger_pct );
  avg_start.merge( other.avg_start );
  avg_refresh.merge( other.avg_refresh );
  avg_expire.merge( other.avg_expire );
  avg_overflow_count.merge( other.avg_overflow_count );
  avg_overflow_total.merge( other.avg_overflow_total );
  if ( sim->buff_uptime_timeline )
    uptime_array.merge( other.uptime_array );

#ifndef NDEBUG
  if ( stack_uptime.size() != other.stack_uptime.size() )
  {
    sim->error( "buff_t::merge {} from {} stack_uptime vector not of equal length.\n", *this,
                 source_name() );
    assert( false );
  }
#endif

  for ( size_t i = 0; i < stack_uptime.size(); i++ )
    stack_uptime[ i ].merge( other.stack_uptime[ i ] );
}

void buff_t::analyze()
{
  if ( sim->buff_uptime_timeline )
  {
    if ( !sim->single_actor_batch )
    {
      uptime_array.adjust( *sim );
    }
    else
    {
      uptime_array.adjust( source->collected_data.fight_length );
    }
  }
}

buff_t* buff_t::find( const std::vector<buff_t*>& buffs, const std::string& name_str, player_t* source )
{
  for ( buff_t* buff : buffs )
  {
    if ( name_str == buff->name_str )
    {
      if ( !source || ( source == buff->source ) )
      {
        return buff;
      }
    }
  }

  return nullptr;
}

namespace
{
struct potion_spell_filter
{
  int spell_id;

  potion_spell_filter( unsigned spell_id ) : spell_id( as<int>(spell_id) )
  {
  }

  bool operator()( const item_data_t* item ) const
  {
    return range::contains_value(item->id_spell, spell_id);
  }
};
}  // namespace

static buff_t* find_potion_buff( const std::vector<buff_t*>& buffs, player_t* source )
{
  for ( auto b : buffs )
  {
    if ( b->data().id() == 0 )
    {
      continue;
    }

    if ( source && source != b->source )
    {
      continue;
    }

    auto item =
        dbc::find_consumable( ITEM_SUBCLASS_POTION, maybe_ptr( b->player->dbc.ptr ), potion_spell_filter( b->data().id() ) );
    if ( item )
    {
      return b;
    }
  }

  return nullptr;
}

buff_t* buff_t::find_expressable( const std::vector<buff_t*>& buffs, const std::string& name, player_t* source )
{
  if ( util::str_compare_ci( "potion", name ) )
    return find_potion_buff( buffs, source );
  else
    return find( buffs, name, source );
}

std::string buff_t::to_str() const
{
  std::ostringstream s;

  s << data().to_str();
  s << " max_stack=" << _max_stack;
  s << " initial_stack=" << data().initial_stacks();
  s << " cooldown=" << cooldown->duration.total_seconds();
  s << " duration=" << buff_duration.total_seconds();
  s << " default_chance=" << default_chance;

  return s.str();
}

expr_t* buff_t::create_expression( std::string buff_name, const std::string& type, action_t& action )
{
  return create_buff_expression( buff_name, type, &action, nullptr );
}

expr_t* buff_t::create_expression( std::string buff_name, const std::string& type, buff_t& static_buff )
{
  return create_buff_expression( buff_name, type, nullptr, &static_buff );
}

#if defined( SC_USE_STAT_CACHE )

void buff_t::invalidate_cache()
{
  if ( player )
  {
    for ( int i = as<int>( invalidate_list.size() ) - 1; i >= 0; i-- )
    {
      player->invalidate_cache( invalidate_list[ i ] );
    }
  }
  else  // It is an aura...  Ouch.
  {
    for ( int i = as<int>( sim->player_no_pet_list.size() ) - 1; i >= 0; i-- )
    {
      player_t* p = sim->player_no_pet_list[ i ];
      for ( int j = as<int>( invalidate_list.size() ) - 1; j >= 0; j-- )
        p->invalidate_cache( invalidate_list[ j ] );
    }
  }
}

#endif

buff_t* buff_t::find( sim_t* s, const std::string& name )
{
  return find( s->buff_list, name );
}

buff_t* buff_t::find( player_t* p, const std::string& name, player_t* source )
{
  return find( p->buff_list, name, source );
}

std::string buff_t::source_name() const
{
  if ( player )
    return player->name_str;

  return "noone";
}

rng::rng_t& buff_t::rng()
{
  return sim->rng();
}

/**
 * Adjust the players dynamic effects affected by haste changes.
 */
void buff_t::adjust_haste()
{
  if ( haste_type == HASTE_NONE )
  {
    return;
  }
  assert(player && "Haste buffs for non-player buffs are not supported.");

  player->adjust_dynamic_cooldowns();
  // player->adjust_global_cooldown( haste_type );
  player->adjust_auto_attack( haste_type );
}

void buff_t::init_haste_type()
{
  // All haste > everything
  if ( range::find( invalidate_list, CACHE_HASTE ) != invalidate_list.end() )
  {
    haste_type = HASTE_ANY;
  }

  // Select one of the specific types
  if ( haste_type == HASTE_NONE )
  {
    if ( range::find( invalidate_list, CACHE_SPELL_HASTE ) != invalidate_list.end() )
    {
      haste_type = HASTE_SPELL;
    }
    else if ( range::find( invalidate_list, CACHE_ATTACK_HASTE ) != invalidate_list.end() )
    {
      haste_type = HASTE_ATTACK;
    }
    else if ( range::find( invalidate_list, CACHE_SPEED ) != invalidate_list.end() )
    {
      haste_type = SPEED_ANY;
    }
    else if ( range::find( invalidate_list, CACHE_SPELL_SPEED ) != invalidate_list.end() )
    {
      haste_type = SPEED_SPELL;
    }
    else if ( range::find( invalidate_list, CACHE_ATTACK_SPEED ) != invalidate_list.end() )
    {
      haste_type = SPEED_ATTACK;
    }
  }
}

std::ostream& operator<<(std::ostream &os, const buff_t& b)
{
  fmt::print(os, "Buff {}", b.name() );
  return os;
}

// ==========================================================================
// STAT_BUFF
// ==========================================================================

stat_buff_t::stat_buff_t( actor_pair_t q, const std::string& name, const spell_data_t* spell, const item_t* item )
  : buff_t( q, name, spell, item ),
    stat_gain( player->get_gain( name + "_buff" ) ),  // append _buff for now to check usage
    manual_stats_added( false )
{
  bool has_ap = false;

  for ( size_t i = 1; i <= data().effect_count(); i++ )
  {
    stat_e s                         = STAT_NONE;
    double amount                    = 0;
    const spelleffect_data_t& effect = data().effectN( i );

    if ( item )
      amount = util::round( effect.average( item ) );
    else
      amount = util::round( effect.average( player, std::min( MAX_LEVEL, player->level() ) ) );

    if ( effect.subtype() == A_MOD_STAT )
    {
      if ( effect.misc_value1() >= 0 )
      {
        s = static_cast<stat_e>( effect.misc_value1() + 1 );
      }
      else if ( effect.misc_value1() == -1 )
      {
        s = STAT_ALL;
      }
    }
    else if ( effect.subtype() == A_MOD_RATING )
    {
      std::vector<stat_e> k = util::translate_all_rating_mod( effect.misc_value1() );
      if ( item )
      {
        amount = item_database::apply_combat_rating_multiplier( *item, amount );
      }

      for ( size_t j = 0; j < k.size(); j++ )
      {
        stats.push_back( buff_stat_t( k[ j ], amount ) );
      }
    }
    else if ( effect.subtype() == A_MOD_DAMAGE_DONE && ( effect.misc_value1() & 0x7E ) )
    {
      s = STAT_SPELL_POWER;
    }
    else if ( effect.subtype() == A_MOD_RESISTANCE )
    {
      s = STAT_BONUS_ARMOR;
    }
    else if ( !has_ap && ( effect.subtype() == A_MOD_ATTACK_POWER || effect.subtype() == A_MOD_RANGED_ATTACK_POWER ) )
    {
      s      = STAT_ATTACK_POWER;
      has_ap = true;
    }
    else if ( effect.subtype() == A_MOD_INCREASE_HEALTH_2 || effect.subtype() == A_MOD_INCREASE_HEALTH )
    {
      s = STAT_MAX_HEALTH;
    }
    else if ( effect.subtype() == A_465 )
    {
      s = STAT_BONUS_ARMOR;
    }

    if ( s != STAT_NONE )
    {
      stats.emplace_back( s, amount );
    }
  }
}

stat_buff_t* stat_buff_t::add_stat( stat_e s, double a, std::function<bool( const stat_buff_t& )> c )
{
  if ( !manual_stats_added )
  {
    // If we are the first to add manual stats, clear the spell_data parsed ones.
    stats.clear();
  }
  manual_stats_added = true;

  stats.emplace_back( s, a, c );

  return this;
}

void stat_buff_t::bump( int stacks, double /* value */ )
{
  buff_t::bump( stacks );

  for ( auto& buff_stat : stats )
  {
    if ( buff_stat.check_func && !buff_stat.check_func( *this ) )
      continue;

    double delta = buff_stat.amount * current_stack - buff_stat.current_value;
    if ( delta > 0 )
    {
      player->stat_gain( buff_stat.stat, delta, stat_gain, nullptr, buff_duration > timespan_t::zero() );
    }
    else if ( delta < 0 )
    {
      player->stat_loss( buff_stat.stat, std::fabs( delta ), stat_gain, nullptr, buff_duration > timespan_t::zero() );
    }

    buff_stat.current_value += delta;
  }
}

void stat_buff_t::decrement( int stacks, double /* value */ )
{
  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    int old_stack = current_stack;

    if ( as<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ].update( false, sim->current_time() );

    for ( auto& buff_stat : stats )
    {
      double delta = buff_stat.amount * stacks;
      if ( delta > 0 )
      {
        player->stat_loss( buff_stat.stat, ( delta <= buff_stat.current_value ) ? delta : 0.0, stat_gain, nullptr,
                           buff_duration > timespan_t::zero() );
      }
      else if ( delta < 0 )
      {
        player->stat_gain( buff_stat.stat, ( delta >= buff_stat.current_value ) ? std::fabs( delta ) : 0.0, stat_gain,
                           nullptr, buff_duration > timespan_t::zero() );
      }
      buff_stat.current_value -= delta;
    }
    current_stack -= stacks;

    invalidate_cache();

    if ( as<std::size_t>( current_stack ) < stack_uptime.size() )
      stack_uptime[ current_stack ].update( true, sim->current_time() );

    sim->print_debug( "{} decremented by {} to {} stacks.", *this, stacks, current_stack );

    if ( old_stack != current_stack && stack_change_callback )
    {
      stack_change_callback( this, old_stack, current_stack );
    }

    if ( player )
      player->trigger_ready();
  }
}

void stat_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  for ( auto& buff_stat : stats )
  {
    if ( buff_stat.current_value > 0 )
    {
      player->stat_loss( buff_stat.stat, buff_stat.current_value, stat_gain, nullptr,
                         buff_duration > timespan_t::zero() );
    }
    else if ( buff_stat.current_value < 0 )
    {
      player->stat_gain( buff_stat.stat, std::fabs( buff_stat.current_value ), stat_gain, nullptr,
                         buff_duration > timespan_t::zero() );
    }
    buff_stat.current_value = 0;
  }

  buff_t::expire_override( expiration_stacks, remaining_duration );
}

// ==========================================================================
// COST_REDUCTION_BUFF
// ==========================================================================

cost_reduction_buff_t::cost_reduction_buff_t( actor_pair_t q, const std::string& name, const spell_data_t* spell,
                                              const item_t* item )
  : buff_t( q, name, spell, item ), amount(), school( SCHOOL_NONE )
{
  // Detect school / amount
  for ( size_t i = 1, e = data().effect_count(); i <= e; i++ )
  {
    const spelleffect_data_t& effect = data().effectN( i );
    if ( effect.type() != E_APPLY_AURA || effect.subtype() != A_MOD_POWER_COST_SCHOOL )
      continue;

    school = dbc::get_school_type( effect.misc_value1() );

    if ( item )
      amount = util::round( effect.average( item ) );
    else
      amount = effect.average( player, std::min( MAX_LEVEL, player->level() ) );

    amount = std::fabs( amount );
    break;
  }
}

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
    player->cost_reduction_gain( school, delta );
    current_value += delta;
  }
  else
    assert( delta == 0 );
}

void cost_reduction_buff_t::decrement( int stacks, double /* value */ )
{
  if ( stacks == 0 || current_stack <= stacks )
  {
    expire();
  }
  else
  {
    double delta = amount * stacks;
    player->cost_reduction_loss( school, delta );
    current_stack -= stacks;
    current_value -= delta;
  }
}

void cost_reduction_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  player->cost_reduction_loss( school, current_value );

  buff_t::expire_override( expiration_stacks, remaining_duration );
}

cost_reduction_buff_t* cost_reduction_buff_t::set_reduction( school_e school, double amount )
{
  this->amount = amount;
  this->school = school;
  return this;
}

absorb_buff_t::absorb_buff_t( actor_pair_t q, const std::string& name, const spell_data_t* spell, const item_t* item )
  : buff_t( q, name, spell, item ),
    absorb_school( SCHOOL_CHAOS ),
    absorb_source(),
    absorb_gain(),
    high_priority( false ),
    eligibility()
{
  assert( player && "Absorb Buffs only supported for player!" );

  set_absorb_school( absorb_school );
}

void absorb_buff_t::start( int stacks, double value, timespan_t duration )
{
  if ( max_stack() == 0 )
    return;

  buff_t::start( stacks, value, duration );

  assert( range::find( player->absorb_buff_list, this ) == player->absorb_buff_list.end() &&
          "Attempting to add absorb buff to absorb_buffs list twice" );

  player->absorb_buff_list.push_back( this );
}

void absorb_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  buff_t::expire_override( expiration_stacks, remaining_duration );

  if ( !player )
  {
    return;
  }

  auto it = range::find( player->absorb_buff_list, this );
  if ( it != player->absorb_buff_list.end() )
    player->absorb_buff_list.erase( it );
}

double absorb_buff_t::consume( double amount )
{
  // Limit the consumption to the current size of the buff.
  amount = std::min( amount, current_value );

  if ( absorb_source )
    absorb_source->add_result( amount, 0, ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, player );

  if ( absorb_gain )
    absorb_gain->add( RESOURCE_HEALTH, amount, 0 );

  current_value -= amount;

  sim->print_debug( "{} {} absorbs {} (remaining: {})", *player, *this, amount, current_value );

  absorb_used( amount );

  if ( current_value <= 0 )
    expire();

  return amount;
}

absorb_buff_t* absorb_buff_t::set_absorb_gain( gain_t* g )
{
  absorb_gain = g;
  return this;
}

absorb_buff_t* absorb_buff_t::set_absorb_source( stats_t* s )
{
  if ( s )
    s->type = STATS_ABSORB;
  absorb_source = s;
  return this;
}

absorb_buff_t* absorb_buff_t::set_absorb_school( school_e s )
{
  absorb_school = s;
  if ( s == SCHOOL_CHAOS )
  {
    for ( size_t i = 1, e = data().effect_count(); i <= e; i++ )
    {
      const spelleffect_data_t& effect = data().effectN( i );
      if ( effect.type() != E_APPLY_AURA || effect.subtype() != A_SCHOOL_ABSORB )
        continue;

      absorb_school = dbc::get_school_type( effect.misc_value1() );
      if ( manual_chance == DEFAULT_VALUE() )
      {
        if ( item )
          default_value = util::round( effect.average( item ) );
        else
          default_value = effect.average( player, std::min( MAX_LEVEL, player->level() ) );
      }
      break;
    }
  }
  return this;
}

absorb_buff_t* absorb_buff_t::set_absorb_high_priority( bool hp )
{
  high_priority = hp;
  // TODO: check if player absorb_priority and instant_absorb_list could be automatically
  // populated from here somehow.
  return this;
}

absorb_buff_t* absorb_buff_t::set_absorb_eligibility( absorb_eligibility e )
{
  eligibility = e;
  // TODO: check if player absorb_priority and instant_absorb_list could be automatically
  // populated from here somehow.
  return this;
}

void buff_creator_basics_t::init()
{
  _chance             = -1.0;
  _max_stack          = -1;
  _duration           = timespan_t::min();
  _cooldown           = timespan_t::min();
  _period             = timespan_t::min();
  _quiet              = -1;
  _reverse            = -1;
  _activated          = -1;
  _can_cancel         = -1;
  _tick_time_behavior = buff_tick_time_behavior::UNHASTED;
  _tick_behavior      = buff_tick_behavior::NONE;
  _refresh_behavior   = buff_refresh_behavior::NONE;
  _stack_behavior     = buff_stack_behavior::DEFAULT;
  _default_value      = buff_t::DEFAULT_VALUE();
  _affects_regen      = -1;
  _initial_tick       = false;
  _rppm_freq          = -1;
  _rppm_mod           = -1;
  _rppm_scale         = RPPM_NONE;
  _trigger_data       = spell_data_t::nil();
}

buff_creator_basics_t::buff_creator_basics_t( actor_pair_t p, const std::string& n, const spell_data_t* sp,
                                              const item_t* item )
  : _player( p ), _sim( p.source->sim ), _name( n ), s_data( sp ), item( item )
{
  init();
}

buff_creator_basics_t::buff_creator_basics_t( actor_pair_t p, uint32_t id, const std::string& n, const item_t* item )
  : _player( p ),
    _sim( p.source->sim ),
    _name( n ),
    s_data( _player.source ? _player.source->find_spell( id ) : spell_data_t::nil() ),
    item( item )
{
  init();
}

buff_creator_basics_t::buff_creator_basics_t( sim_t* s, const std::string& n, const spell_data_t* sp,
                                              const item_t* item )
  : _player( actor_pair_t() ), _sim( s ), _name( n ), s_data( sp ), item( item )
{
  init();
}

buff_creator_t::operator buff_t*() const
{
  return new buff_t( *this );
}

bool movement_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  if ( player->buffs.norgannons_foresight_ready )
  {
    player->buffs.norgannons_foresight_ready->expire( timespan_t::from_seconds( 5 ) );
    player->buffs.norgannons_foresight->expire();
  }
  return buff_t::trigger( stacks, value, chance, duration );
}

void movement_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  buff_t::expire_override( expiration_stacks, remaining_duration );
  source->finish_moving();
}

void invulnerable_debuff_t::start( int stacks, double value, timespan_t duration )
{
  buff_t::start( stacks, value, duration );
  if ( sim->ignore_invulnerable_targets && range::contains_value( sim->target_non_sleeping_list, player ) )
  {
    sim->target_non_sleeping_list.find_and_erase_unordered( player );
    sim->active_enemies--;
  }
}

void invulnerable_debuff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  buff_t::expire_override( expiration_stacks, remaining_duration );
  if ( sim->ignore_invulnerable_targets && !range::contains_value( sim->target_non_sleeping_list, player ) )
  {
    sim->target_non_sleeping_list.push_back( player );
    sim->active_enemies++;
  }
}

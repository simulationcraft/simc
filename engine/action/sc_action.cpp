// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Action
// ==========================================================================

namespace { // anonymous namespace

struct player_gcd_event_t : public player_event_t
{
  player_gcd_event_t( player_t& p, timespan_t delta_time ) :
      player_event_t( p, delta_time )
  {
    if ( sim().debug )
      sim().out_debug << "New Player-Ready-GCD Event: " << p.name();
  }

  const char* name() const override
  { return "Player-Ready-GCD"; }

  void select_action( action_priority_list_t* alist )
  {
    for ( auto i = p() -> active_off_gcd_list -> off_gcd_actions.begin();
          i < p() -> active_off_gcd_list -> off_gcd_actions.end(); ++i )
    {
      action_t* a = *i;
      if ( ! a -> ready() )
      {
        continue;
      }

      // Don't attempt to execute an off gcd action that's already being queued (this should not
      // happen anyhow)
      if ( ! p() -> queueing || ( p() -> queueing -> internal_id != a -> internal_id ) )
      {
        auto queue_delay = a -> cooldown -> queue_delay();
        // Don't queue the action if GCD would elapse before the action is usable again
        if ( queue_delay == timespan_t::zero() ||
             ( a -> player -> readying &&
             sim().current_time() + queue_delay < a -> player -> readying -> occurs() ) )
        {
          // If we're queueing something, it's something different from what we are about to do.
          // Cancel existing queued (off gcd) action, and queue the new one.
          if ( p() -> queueing )
          {
            event_t::cancel( p() -> queueing -> queue_event );
            p() -> queueing = nullptr;
          }
          a -> queue_execute( true );
        }
      }

      // Need to restart because the active action list changed
      if ( alist != p() -> active_action_list )
      {
        p() -> activate_action_list( p() -> active_action_list, true );
        select_action( p() -> active_action_list );
        p() -> activate_action_list( alist, true );
      }

      // If we're queueing an action, no point looking for more
      if ( p() -> queueing )
      {
        break;
      }
    }
  }

  void execute() override
  {
    p() -> off_gcd = nullptr;

    // It is possible to orchestrate events such that an action is currently executing when an
    // off-gcd event occurs, if this is the case, don't do anything
    if ( ! p() -> executing )
    {
      select_action( p() -> active_action_list );
    }

    // Create a new Off-GCD event only in the case we didnt find anything to queue (could use an
    // ability right away) and the action we executed was not a run_action_list. Note also that an
    // off-gcd event may have been created in do_off_gcd_execute() if this Player-Ready-GCD
    // execution found an off gcd action to execute, in this case, do not create a new event.
    if ( ! p() -> off_gcd && ! p() -> queueing && ! p() -> restore_action_list )
    {
      p() -> off_gcd = make_event<player_gcd_event_t>( sim(), *p(), timespan_t::from_seconds( 0.1 ) );
    }

    if ( p() -> restore_action_list != 0 )
    {
      p() -> activate_action_list( p() -> restore_action_list );
      p() -> restore_action_list = 0;
    }
  }
};

/**
 * Hack to bypass some of the full execution chain to be able to re-use normal actions as "off gcd actions" (usable
 * during gcd). Will directly execute the action (instead of going through schedule_execute processing), and parts of
 * our execution chain where relevant (e.g., line cooldown, stats tracking).
 */
void do_off_gcd_execute( action_t* action )
{
  action -> execute();
  action -> line_cooldown.start();
  if ( ! action -> quiet )
  {
    action -> player -> iteration_executed_foreground_actions++;
    action -> total_executions++;
    action -> player -> sequence_add( action, action -> target, action -> sim -> current_time() );
  }

  // If we executed a queued off-gcd action, we need to re-kick the player gcd event if there's
  // still time to poll for new actions to use.
  timespan_t interval = timespan_t::from_seconds( 0.1 );
  if ( ! action -> player -> off_gcd &&
       action -> sim -> current_time() + interval < action -> player -> gcd_ready )
  {
    action -> player -> off_gcd = make_event<player_gcd_event_t>( *action -> sim, *action -> player, interval );
  }

  if ( action -> player -> queueing == action )
  {
    action -> player -> queueing = nullptr;
  }
}

struct queued_action_execute_event_t : public event_t
{
  action_t* action;
  bool off_gcd;

  queued_action_execute_event_t( action_t* a, const timespan_t& t, bool off_gcd ) :
    event_t( *a -> sim, t ), action( a ), off_gcd( off_gcd )
  {
  }

  const char* name() const override
  { return "Queued-Action-Execute"; }

  void execute() override
  {
    action -> queue_event = nullptr;

    // Sanity check assert to catch violations. Will only trigger (if ever) with off gcd actions,
    // and even then only in the case of bugs.
    assert( action -> cooldown -> ready <= sim().current_time() );

    // On very very rare occasions, charge-based cooldowns (which update through an event, but
    // indicate readiness through cooldown_t::ready) can have their recharge event and the
    // queued-action-execute event occur on the same timestamp in such a way that the events flip to
    // the wrong order (queued-action-execute comes before recharge event). If this is the case, we
    // need to flip them around to ensure that the sim internal state checks do not fail. The
    // solution is to simply recreate the queued-action-execute event on the same timestamp, which
    // will once again flip the ordering (i.e., lets the recharge event occur first).
    if ( action -> cooldown -> charges > 1 && action -> cooldown -> current_charge == 0 &&
         action -> cooldown -> recharge_event &&
         action -> cooldown -> recharge_event -> remains() == timespan_t::zero() )
    {
      action -> queue_event = make_event<queued_action_execute_event_t>( sim(), action, timespan_t::zero(), off_gcd );
      // Note, processing ends here
      return;
    }

    if ( off_gcd )
    {
      do_off_gcd_execute( action );
    }
    else
    {
      action -> schedule_execute();
    }
  }
};

// Action Execute Event =====================================================

struct action_execute_event_t : public player_event_t
{
  action_t* action;
  action_state_t* execute_state;

  action_execute_event_t( action_t* a, timespan_t time_to_execute,
                          action_state_t* state = 0 ) :
      player_event_t( *a->player, time_to_execute ), action( a ),
        execute_state( state )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New Action Execute Event: %s %s %.1f (target=%s, marker=%c)",
                  p() -> name(), a -> name(), time_to_execute.total_seconds(),
                  ( state ) ? state -> target -> name() : a -> target -> name(),
                  ( a -> marker ) ? a -> marker : '0' );
  }

  virtual const char* name() const override
  { return "Action-Execute"; }


  ~action_execute_event_t()
  {
    // Ensure we properly release the carried execute_state even if this event
    // is never executed.
    if ( execute_state )
      action_state_t::release( execute_state );
  }

  virtual void execute() override
  {
    player_t* target = action -> target;

    // Pass the carried execute_state to the action. This saves us a few
    // cycles, as we don't need to make a copy of the state to pass to
    // action -> pre_execute_state.
    if ( execute_state )
    {
      target = execute_state -> target;
      action -> pre_execute_state = execute_state;
      execute_state = 0;
    }

    action -> execute_event = 0;

    if ( target -> is_sleeping() && action -> pre_execute_state )
    {
      action_state_t::release( action -> pre_execute_state );
      action -> pre_execute_state = 0;
    }

    if ( !target -> is_sleeping() )
    {
      // Action target must follow any potential pre-execute-state target if it differs from the
      // current (default) target of the action.
      if ( target != action -> target )
      {
        action -> target = target;
      }
      action -> execute();
    }

    assert( ! action -> pre_execute_state );

    if ( action -> background ) return;

    if ( ! p() -> channeling )
    {
      assert( ! p() -> readying &&
              "Danger Will Robinson! Danger! Non-channeling action is trying to overwrite player-ready-event upon execute." );

      p() -> schedule_ready( timespan_t::zero() );
    }

    if ( p() -> active_off_gcd_list == 0 )
      return;

    // Kick off the during-gcd checker, first run is immediately after
    event_t::cancel( p() -> off_gcd );

    if ( ! p() -> channeling && p() -> gcd_ready > sim().current_time() )
    {
      p() -> off_gcd = make_event<player_gcd_event_t>( sim(), *p(), timespan_t::zero() );
    }
  }
};

struct power_entry_without_aura
{
  bool operator()( const spellpower_data_t* p )
  {
    return p -> aura_id() == 0;
  }
};

} // unnamed namespace


/**
 * @brief add to action list without restriction
 *
 * Anything goes to action priority list.
 */
action_priority_t* action_priority_list_t::add_action( const std::string& action_priority_str,
                                                       const std::string& comment )
{
  if ( action_priority_str.empty() )
    return nullptr;
  action_list.push_back( action_priority_t( action_priority_str, comment ) );
  return &( action_list.back() );
}

/**
 * @brief add to action list & check spelldata
 *
 * Check the validity of spell data before anything goes to action priority list
 */
action_priority_t* action_priority_list_t::add_action( const player_t* p,
                                                       const spell_data_t* s,
                                                       const std::string& action_name,
                                                       const std::string& action_options,
                                                       const std::string& comment )
{
  if ( ! s || ! s -> ok() || ! s -> is_level( p -> true_level ) )
    return nullptr;

  std::string str = action_name;
  if ( ! action_options.empty() )
    str += "," + action_options;

  return add_action( str, comment );
}

/**
 * @brief add to action list & check class spell with given name
 *
 * Check the availability of a class spell of "name" and the validity of it's
 * spell data before anything goes to action priority list
 */
action_priority_t* action_priority_list_t::add_action( const player_t* p,
                                                       const std::string& name,
                                                       const std::string& action_options,
                                                       const std::string& comment )
{
  const spell_data_t* s = p -> find_class_spell( name );
  if ( s == spell_data_t::not_found() )
    s = p -> find_specialization_spell( name );
  return add_action( p, s, dbc::get_token( s -> id() ), action_options, comment );
}

/**
 * @brief add talent action to action list & check talent availability
 *
 * Check the availability of a talent spell of "name" and the validity of it's
 * spell data before anything goes to action priority list. Note that this will
 * ignore the actual talent check so we can make action list entries for
 * talents, even though the profile does not have a talent.
 *
 * In addition, the method automatically checks for the presence of an if
 * expression that includes the "talent guard", i.e., "talent.X.enabled" in it.
 * If omitted, it will be automatically added to the if expression (or
 * if expression will be created if it is missing).
 */
action_priority_t* action_priority_list_t::add_talent( const player_t* p,
                                                       const std::string& name,
                                                       const std::string& action_options,
                                                       const std::string& comment )
{
  const spell_data_t* s = p -> find_talent_spell( name, "", SPEC_NONE, false, false );
  return add_action( p, s, dbc::get_token( s -> id() ), action_options, comment );
}

action_t::options_t::options_t()
  : moving( -1 ),
    wait_on_ready( -1 ),
    max_cycle_targets(),
    target_number(),
    interrupt(),
    chain(),
    cycle_targets(),
    cycle_players(),
    interrupt_immediate(),
    if_expr_str(),
    target_if_str(),
    interrupt_if_expr_str(),
    early_chain_if_expr_str(),
    sync_str(),
    target_str()
{
}

action_t::action_t( action_e       ty,
                    const std::string&  token,
                    player_t*           p,
                    const spell_data_t* s ) :
  s_data( s ? s : spell_data_t::nil() ),
  sim( p -> sim ),
  type( ty ),
  name_str( util::tokenize_fn(token) ),
  player( p ),
  target( p -> target ),
  item(),
  weapon(),
  default_target( p -> target ),
  school( SCHOOL_NONE ),
  id(),
  internal_id( as<unsigned>( p -> get_action_id( name_str ) ) ),
  resource_current( RESOURCE_NONE ),
  aoe(),
  dual(),
  callbacks( true ),
  special(),
  channeled(),
  sequence(),
  quiet(),
  background(),
  use_off_gcd(),
  interrupt_auto_attack( true ),
  ignore_false_positive(),
  action_skill( p -> base.skill ),
  direct_tick(),
  repeating(),
  harmful( true ),
  proc(),
  initialized(),
  may_hit( true ),
  may_miss(),
  may_dodge(),
  may_parry(),
  may_glance(),
  may_block(),
  may_crit(),
  tick_may_crit(),
  tick_zero(),
  hasted_ticks(),
  consume_per_tick_(),
  split_aoe_damage(),
  normalize_weapon_speed(),
  ground_aoe(),
  round_base_dmg( true),
  dynamic_tick_action( true), // WoD updates everything on tick by default. If you need snapshotted values for a periodic effect, use persistent multipliers.
  interrupt_immediate_occurred(),
  hit_any_target(),
  dot_behavior( DOT_REFRESH ),
  ability_lag(),
  ability_lag_stddev(),
  rp_gain(),
  min_gcd(),
  gcd_haste( HASTE_NONE ),
  trigger_gcd( p -> base_gcd ),
  range(-1.0),
  radius(-1.0),
  weapon_power_mod(),
  attack_power_mod(),
  spell_power_mod(),
  amount_delta(),
  base_execute_time(),
  base_tick_time(),
  dot_duration(),
  dot_max_stack( 1 ),
  base_costs(),
  secondary_costs(),
  base_costs_per_tick(),
  base_dd_min(),
  base_dd_max(),
  base_td(),
  base_dd_multiplier( 1.0 ),
  base_td_multiplier( 1.0 ),
  base_multiplier( 1.0 ),
  base_hit(),
  base_crit(),
  crit_multiplier( 1.0 ),
  crit_bonus_multiplier( 1.0 ),
  crit_bonus(),
  base_dd_adder(),
  base_ta_adder(),
  weapon_multiplier( 1.0 ),
  chain_multiplier( 1.0 ),
  chain_bonus_damage(),
  base_aoe_multiplier( 1.0 ),
  base_recharge_multiplier( 1.0 ),
  base_teleport_distance(),
  travel_speed(),
  energize_amount(),
  movement_directionality( MOVEMENT_NONE ),
  parent_dot(),
  child_action(),
  tick_action(),
  execute_action(),
  impact_action(),
  gain( p -> get_gain( name_str ) ),
  energize_type( ENERGIZE_NONE ),
  energize_resource( RESOURCE_NONE ),
  cooldown( p -> get_cooldown( name_str ) ),
  internal_cooldown( p -> get_cooldown( name_str + "_internal" ) ),
  stats(p -> get_stats( name_str, this )),
  execute_event(),
  queue_event(),
  time_to_execute(),
  time_to_travel(),
  last_resource_cost(),
  num_targets_hit(),
  marker(),
  option(),
  if_expr(),
  target_if_mode( TARGET_IF_NONE ),
  target_if_expr(),
  interrupt_if_expr(),
  early_chain_if_expr(),
  sync_action(),
  signature_str(),
  target_specific_dot( false ),
  action_list(),
  starved_proc(),
  total_executions(),
  line_cooldown( "line_cd", *p ),
  signature(),
  execute_state(),
  pre_execute_state(),
  snapshot_flags(),
  update_flags( STATE_TGT_MUL_DA | STATE_TGT_MUL_TA | STATE_TGT_CRIT),
  target_cache(),
  options(),
  state_cache(),
  travel_events()
{
  assert( option.cycle_targets == 0 );
  assert( !name_str.empty() && "Abilities must have valid name_str entries!!" );

  if ( sim -> initialized )
  {
    sim -> errorf( "Player %s action %s created after simulator initialization.",
      player -> name(), name() );
  }

  if ( sim -> current_iteration > 0 )
  {
    sim -> errorf( "Player %s creating action %s ouside of the first iteration", player -> name(), name() );
    assert( false );
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "Player %s creates action %s (%d)", player -> name(), name(), ( data().ok() ? data().id() : -1 ) );

  if ( !player -> initialized )
  {
    sim -> errorf( "Actions must not be created before player_t::init().  Culprit: %s %s\n", player -> name(), name() );
    sim -> cancel();
  }

  player -> action_list.push_back( this );

  if ( data().ok() )
  {
    parse_spell_data( data() );
  }

  if ( s_data == spell_data_t::not_found() )
  {
    // this is super-spammy, may just want to disable this after we're sure this section is working as intended.
    if ( sim -> debug )
      sim -> errorf( "Player %s attempting to use action %s without the required talent, spec, class, or race; ignoring.\n",
      player -> name(), name() );
    background = true;
  }

  add_option( opt_deprecated( "bloodlust", "if=buff.bloodlust.react" ) );
  add_option( opt_deprecated( "haste<", "if=spell_haste>= or if=attack_haste>=" ) );
  add_option( opt_deprecated( "health_percentage<", "if=target.health.pct<=" ) );
  add_option( opt_deprecated( "health_percentage>", "if=target.health.pct>=" ) );
  add_option( opt_deprecated( "invulnerable", "if=target.debuff.invulnerable.react" ) );
  add_option( opt_deprecated( "not_flying", "if=target.debuff.flying.down" ) );
  add_option( opt_deprecated( "flying", "if=target.debuff.flying.react" ) );
  add_option( opt_deprecated( "time<", "if=time<=" ) );
  add_option( opt_deprecated( "time>", "if=time>=" ) );
  add_option( opt_deprecated( "travel_speed", "if=travel_speed" ) );
  add_option( opt_deprecated( "vulnerable", "if=target.debuff.vulnerable.react" ) );
  add_option( opt_deprecated( "label", "N/A" ) );

  add_option( opt_string( "if", option.if_expr_str ) );
  add_option( opt_string( "interrupt_if", option.interrupt_if_expr_str ) );
  add_option( opt_string( "early_chain_if", option.early_chain_if_expr_str ) );
  add_option( opt_bool( "interrupt", option.interrupt ) );
  add_option( opt_bool( "chain", option.chain ) );
  add_option( opt_bool( "cycle_targets", option.cycle_targets ) );
  add_option( opt_bool( "cycle_players", option.cycle_players ) );
  add_option( opt_int( "max_cycle_targets", option.max_cycle_targets ) );
  add_option( opt_string( "target_if", option.target_if_str ) );
  add_option( opt_bool( "moving", option.moving ) );
  add_option( opt_string( "sync", option.sync_str ) );
  add_option( opt_bool( "wait_on_ready", option.wait_on_ready ) );
  add_option( opt_string( "target", option.target_str ) );
  add_option( opt_timespan( "line_cd", line_cooldown.duration ) );
  add_option( opt_float( "action_skill", action_skill ) );
  // Interrupt_immediate forces a channeled action to interrupt on tick (if requested), even if the
  // GCD has not elapsed.
  add_option( opt_bool( "interrupt_immediate", option.interrupt_immediate ) );
}

action_t::~action_t()
{
  delete execute_state;
  delete pre_execute_state;
  delete if_expr;
  delete target_if_expr;
  delete interrupt_if_expr;
  delete early_chain_if_expr;

  while ( state_cache != 0 )
  {
    action_state_t* s = state_cache;
    state_cache = s -> next;
    delete s;
  }
}

/**
 * Parse spell data values and write them into corresponding action_t members.
 */
void action_t::parse_spell_data( const spell_data_t& spell_data )
{
  if ( ! spell_data.ok() )
  {
    sim -> errorf( "%s %s: parse_spell_data: no spell to parse.\n", player -> name(), name() );
    return;
  }

  id                   = spell_data.id();
  base_execute_time    = spell_data.cast_time( player -> level() );
  range                = spell_data.max_range();
  travel_speed         = spell_data.missile_speed();
  trigger_gcd          = spell_data.gcd();
  school               = spell_data.get_school_type();

  cooldown -> duration = timespan_t::zero();

  if ( spell_data.charges() > 0 && spell_data.charge_cooldown() > timespan_t::zero() )
  {
    cooldown -> duration = spell_data.charge_cooldown();
    cooldown -> charges = spell_data.charges();
    if ( spell_data.cooldown() > timespan_t::zero() )
    {
      internal_cooldown -> duration = spell_data.cooldown();
    }
  }
  else if ( spell_data.cooldown() > timespan_t::zero() )
  {
    cooldown -> duration = spell_data.cooldown();
  }

  if (spell_data._power)
  {
    if (spell_data._power->size() == 1 && spell_data._power -> at( 0 ) -> aura_id() == 0 )
    {
      resource_current = spell_data._power->at(0)->resource();
    }
    else
    {
      // Find the first power entry without a aura id
      std::vector<const spellpower_data_t*>::iterator it = std::find_if(
          spell_data._power -> begin(), spell_data._power -> end(),
          power_entry_without_aura() );
      if (it != spell_data._power -> end())
      {
        resource_current = (*it) -> resource();
      }
    }
  }

  for ( size_t i = 0; spell_data._power && i < spell_data._power -> size(); i++ )
  {
    const spellpower_data_t* pd = ( *spell_data._power )[ i ];

    if ( pd -> _cost != 0 )
      base_costs[ pd -> resource() ] = pd -> cost();
    else
      base_costs[ pd -> resource() ] = floor( pd -> cost() * player -> resources.base[ pd -> resource() ] );

    secondary_costs[ pd -> resource() ] = pd -> max_cost();

    if ( pd -> _cost_per_tick != 0 )
      base_costs_per_tick[ pd -> resource() ] = pd -> cost_per_tick();
    else
      base_costs_per_tick[ pd -> resource() ] = floor( pd -> cost_per_tick() * player -> resources.base[ pd -> resource() ] );
  }

  for ( size_t i = 1; i <= spell_data.effect_count(); i++ )
  {
    parse_effect_data( spell_data.effectN( i ) );
  }
}

// action_t::parse_effect_data ==============================================
void action_t::parse_effect_data( const spelleffect_data_t& spelleffect_data )
{
  if ( ! spelleffect_data.ok() )
  {
    return;
  }

  // Only use item level-based scaling if there's no max scaling level defined for the spell
  bool item_scaling = item && data().max_scaling_level() == 0;

  // Technically, there could be both a single target and an aoe effect in a single spell, but that
  // probably will never happen.
  if ( spelleffect_data.chain_target() > 1 )
  {
    aoe = spelleffect_data.chain_target();
  }

  switch ( spelleffect_data.type() )
  {
      // Direct Damage
    case E_HEAL:
    case E_SCHOOL_DAMAGE:
    case E_HEALTH_LEECH:
      spell_power_mod.direct  = spelleffect_data.sp_coeff();
      attack_power_mod.direct = spelleffect_data.ap_coeff();
      amount_delta            = spelleffect_data.m_delta();
      base_dd_min      = item_scaling ? spelleffect_data.min( item ) : spelleffect_data.min( player, player -> level() );
      base_dd_max      = item_scaling ? spelleffect_data.max( item ) : spelleffect_data.max( player, player -> level() );
      radius           = spelleffect_data.radius_max();
      break;

    case E_NORMALIZED_WEAPON_DMG:
      normalize_weapon_speed = true;
    case E_WEAPON_DAMAGE:
      base_dd_min      = item_scaling ? spelleffect_data.min( item ) : spelleffect_data.min( player, player -> level() );
      base_dd_max      = item_scaling ? spelleffect_data.max( item ) : spelleffect_data.max( player, player -> level() );
      weapon           = &( player -> main_hand_weapon );
      radius           = spelleffect_data.radius_max();
      break;

    case E_WEAPON_PERCENT_DAMAGE:
      weapon            = &( player -> main_hand_weapon );
      weapon_multiplier = item_scaling ? spelleffect_data.min( item ) : spelleffect_data.min( player, player -> level() );
      radius            = spelleffect_data.radius_max();
      break;

      // Dot
    case E_PERSISTENT_AREA_AURA:
      radius = spelleffect_data.radius_max();
      if ( radius < 0 )
        radius = spelleffect_data.radius();
      break;
    case E_APPLY_AURA:
      switch ( spelleffect_data.subtype() )
      {
        case A_PERIODIC_DAMAGE:
        case A_PERIODIC_LEECH:
        case A_PERIODIC_HEAL:
          spell_power_mod.tick  = spelleffect_data.sp_coeff();
          attack_power_mod.tick = spelleffect_data.ap_coeff();
          radius           = spelleffect_data.radius_max();
          base_td          = item_scaling ? spelleffect_data.average( item ) : spelleffect_data.average( player, player -> level() );
        case A_PERIODIC_ENERGIZE:
        case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
        case A_PERIODIC_HEALTH_FUNNEL:
        case A_PERIODIC_MANA_LEECH:
        case A_PERIODIC_DAMAGE_PERCENT:
        case A_PERIODIC_DUMMY:
        case A_PERIODIC_TRIGGER_SPELL:
        case A_OBS_MOD_HEALTH:
          if ( spelleffect_data.period() > timespan_t::zero() )
          {
            base_tick_time   = spelleffect_data.period();
            dot_duration = spelleffect_data.spell() -> duration();
          }
          break;
        case A_SCHOOL_ABSORB:
          spell_power_mod.direct  = spelleffect_data.sp_coeff();
          attack_power_mod.direct = spelleffect_data.ap_coeff();
          amount_delta            = spelleffect_data.m_delta();
          base_dd_min      = item_scaling ? spelleffect_data.min( item ) : spelleffect_data.min( player, player -> level() );
          base_dd_max      = item_scaling ? spelleffect_data.max( item ) : spelleffect_data.max( player, player -> level() );
          radius           = spelleffect_data.radius_max();
          break;
        case A_ADD_FLAT_MODIFIER:
          switch ( spelleffect_data.misc_value1() )
          case E_APPLY_AURA:
          switch ( spelleffect_data.subtype() )
          {
            case P_CRIT:
              base_crit += 0.01 * spelleffect_data.base_value();
              break;
            case P_COOLDOWN:
              cooldown -> duration += spelleffect_data.time_value();
              break;
            default: break;
          }
          break;
        default: break;
      }
      break;
    case E_ENERGIZE:
      if ( energize_type == ENERGIZE_NONE )
      {
        energize_type     = ENERGIZE_ON_HIT;
        energize_resource = spelleffect_data.resource_gain_type();
        energize_amount   = spelleffect_data.resource( energize_resource );
      }
      break;
    default: break;
  }
}

// action_t::parse_options ==================================================
void action_t::parse_target_str()
{
  // FIXME: Move into constructor when parse_action is called from there.
  if ( ! option.target_str.empty() )
  {
    if ( option.target_str[ 0 ] >= '0' && option.target_str[ 0 ] <= '9' )
    {
      option.target_number = atoi( option.target_str.c_str() );
      player_t* p = find_target_by_number( option.target_number );
      // Numerical targeting is intended to be dynamic, so don't give an error message if we can't find the target yet
      if ( p ) target = p;
    }
    else if ( util::str_compare_ci( option.target_str, "self" ) )
      target = this -> player;
    else
    {
      player_t* p = sim -> find_player( option.target_str );

      if ( p )
        target = p;
      else
        sim -> errorf( "%s %s: Unable to locate target for action '%s'.\n", player -> name(), name(), signature_str.c_str() );
    }
  }
}

// action_t::parse_options ==================================================

void action_t::parse_options( const std::string& options_str )
{
  try
  {
    opts::parse( sim, name(), options, options_str );
  }
  catch ( const std::exception& e )
  {
    sim -> errorf( "%s %s: Unable to parse options str '%s': %s", player -> name(), name(), options_str.c_str(), e.what() );
    sim -> cancel();
  }

  parse_target_str();
}

bool action_t::verify_actor_level() const
{
  if ( data().id() && ! data().is_level( player -> true_level ) && data().level() <= MAX_LEVEL )
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required level (%d < %d).\n",
      player -> name(), name(), player -> true_level, data().level() );
    return false;
  }

  return true;
}

bool action_t::verify_actor_spec() const
{
  std::vector<specialization_e> spec_list;
  specialization_e _s = player -> specialization();
  if ( data().id() && player -> dbc.ability_specialization( data().id(), spec_list ) && range::find( spec_list, _s ) == spec_list.end() )
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required spec.\n",
      player -> name(), name() );

    return false;
  }

  return true;
}

// action_t::base_cost ======================================================

double action_t::base_cost() const
{
  resource_e cr = current_resource();
  double c = base_costs[ cr ];
  if ( secondary_costs[ cr ] != 0 )
  {
    c += secondary_costs[ cr ];
  }

  return c;
}

/**
 * Resource cost of the action for current_resource()
 */
double action_t::cost() const
{
  if ( ! harmful && ! player -> in_combat )
    return 0;

  resource_e cr = current_resource();

  double c;
  if ( secondary_costs[ cr ] == 0 )
  {
    c = base_costs[ cr ];
  }
  // For now, treat secondary cost as "maximum of player current resource, min + max cost". Entirely
  // possible we need to add some additional functionality (such as an overridable method) to
  // determine the cost, if the default behavior is not universal.
  else
  {
    if ( player -> resources.current[ cr ] >= base_costs[ cr ] )
    {
      c = std::min( base_cost(), player -> resources.current[ cr ] );
    }
    else
    {
      c = base_costs[ cr ];
    }
  }

  c -= player -> current.resource_reduction[ get_school() ];

  if ( cr == RESOURCE_MANA && player -> buffs.courageous_primal_diamond_lucidity -> check() )
    c = 0;

  if ( c < 0 ) c = 0;

  if ( sim -> debug )
    sim -> out_debug.printf( "action_t::cost: %s base_cost=%.2f secondary_cost=%.2f cost=%.2f resource=%s", name(), base_costs[ cr ], secondary_costs[ cr ], c, util::resource_type_string( cr ) );

  return floor( c );
}

double action_t::cost_per_tick( resource_e r ) const
{
  return base_costs_per_tick[ r ];
}

// action_t::gcd ============================================================

timespan_t action_t::gcd() const
{
  if ( ( ! harmful && ! player -> in_combat ) || trigger_gcd == timespan_t::zero() )
    return timespan_t::zero();

  timespan_t gcd_ = trigger_gcd;
  switch ( gcd_haste )
  {
    // Note, HASTE_ANY should never be used for actions. It does work as a crutch though, since
    // action_t::composite_haste will return the correct haste value.
    case HASTE_ANY:
    case HASTE_SPELL:
    case HASTE_ATTACK:
      gcd_ *= composite_haste();
      break;
    case SPEED_SPELL:
      gcd_ *= player -> cache.spell_speed();
      break;
    case SPEED_ATTACK:
      gcd_ *= player -> cache.attack_speed();
      break;
    // SPEED_ANY is nonsensical for GCD reduction, since we don't have action_t::composite_speed()
    // to give the correct speed value.
    case SPEED_ANY:
    default:
      break;
  }

  if ( gcd_ < min_gcd )
  {
    gcd_ = min_gcd;
  }

  return gcd_;
}

/** False Positive skill chance, executes command regardless of expression. */
double action_t::false_positive_pct() const
{
  double failure_rate = 0.0;

  if ( action_skill == 1 && player -> current.skill_debuff == 0 )
    return failure_rate;

  if ( !player -> in_combat || background || player -> strict_sequence || ignore_false_positive )
    return failure_rate;

  failure_rate = ( 1 - action_skill ) / 2;
  failure_rate += player -> current.skill_debuff / 2;

  if ( dot_duration > timespan_t::zero() )
  {
    if ( dot_t* d = find_dot( target ) )
    {
      if ( d -> remains() < dot_duration / 2 )
        failure_rate *= 1 - d -> remains() / ( dot_duration / 2 );
    }
  }

  return failure_rate;
}

double action_t::false_negative_pct() const
{
  double failure_rate = 0.0;

  if ( action_skill == 1 && player -> current.skill_debuff == 0 )
    return failure_rate;

  if ( !player -> in_combat || background || player -> strict_sequence )
    return failure_rate;

  failure_rate = ( 1 - action_skill ) / 2;

  failure_rate += player -> current.skill_debuff / 2;

  return failure_rate;
}

// action_t::travel_time ====================================================

timespan_t action_t::travel_time() const
{
  if ( travel_speed == 0 ) return timespan_t::zero();

  double distance;
  distance = player -> get_player_distance( *target );

  if ( execute_state && execute_state -> target )
    distance += execute_state -> target -> height;

  if ( distance == 0 ) return timespan_t::zero();

  double t = distance / travel_speed;

  double v = sim -> travel_variance;

  if ( v )
    t = rng().gauss( t, v );

  return timespan_t::from_seconds( t );
}

// action_t::total_crit_bonus ===============================================

double action_t::total_crit_bonus( action_state_t* state ) const
{
  double crit_multiplier_buffed = crit_multiplier * composite_player_critical_multiplier( state );
  double base_crit_bonus = crit_bonus;
  if ( sim -> pvp_crit )
    base_crit_bonus -= 0.5; // Players in pvp take 150% critical hits baseline.
  if ( player -> buffs.amplification )
    base_crit_bonus += player -> passive_values.amplification_1;
  if ( player -> buffs.amplification_2 )
    base_crit_bonus += player -> passive_values.amplification_2;

  double bonus = ( ( 1.0 + base_crit_bonus ) * crit_multiplier_buffed - 1.0 ) * composite_crit_damage_bonus_multiplier();

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s crit_bonus for %s: cb=%.3f b_cb=%.2f b_cm=%.2f b_cbm=%.2f",
                   player -> name(), name(), bonus, crit_bonus, crit_multiplier_buffed, composite_crit_damage_bonus_multiplier() );
  }

  return bonus;
}

// action_t::calculate_weapon_damage ========================================

double action_t::calculate_weapon_damage( double attack_power ) const
{
  if ( ! weapon || weapon_multiplier <= 0 ) return 0;

  double dmg = sim -> averaged_range( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> bonus_dmg;

  timespan_t weapon_speed  = normalize_weapon_speed  ? weapon -> get_normalized_speed() : weapon -> swing_time;

  double power_damage = weapon_speed.total_seconds() * weapon_power_mod * attack_power;

  double total_dmg = dmg + power_damage;

  // OH penalty
  if ( weapon -> slot == SLOT_OFF_HAND )
    total_dmg *= 0.5;

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s weapon damage for %s: base=%.0f-%.0f td=%.3f wd=%.3f bd=%.3f ws=%.3f pd=%.3f ap=%.3f",
                   player -> name(), name(), weapon -> min_dmg, weapon -> max_dmg, total_dmg, dmg, weapon -> bonus_dmg, weapon_speed.total_seconds(), power_damage, attack_power );
  }

  return total_dmg;
}

// action_t::calculate_tick_amount ==========================================

double action_t::calculate_tick_amount( action_state_t* state, double dot_multiplier ) const
{
  double amount = 0;

  if ( base_ta( state ) == 0 && spell_tick_power_coefficient( state ) == 0 && attack_tick_power_coefficient( state ) == 0) return 0;

  amount  = floor( base_ta( state ) + 0.5 );
  amount += bonus_ta( state );
  amount += state -> composite_spell_power() * spell_tick_power_coefficient( state );
  amount += state -> composite_attack_power() * attack_tick_power_coefficient( state );
  amount *= state -> composite_ta_multiplier();

  double init_tick_amount = amount;

  if ( ! sim -> average_range )
    amount = floor( amount + rng().real() );

  // Record raw amount to state
  state -> result_raw = amount;

  amount *= dot_multiplier;

  // Record total amount to state
  state -> result_total = amount;

  // Apply crit damage bonus immediately to periodic damage since there is no travel time (and
  // subsequent impact).
  amount = calculate_crit_damage_bonus( state );

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s amount for %s on %s: ta=%.0f i_ta=%.0f b_ta=%.0f bonus_ta=%.0f s_mod=%.2f s_power=%.0f a_mod=%.2f a_power=%.0f mult=%.2f, tick_mult=%.2f",
                   player -> name(), name(), state -> target -> name(), amount,
                   init_tick_amount, base_ta( state ), bonus_ta( state ),
                   spell_tick_power_coefficient( state ), state -> composite_spell_power(),
                   attack_tick_power_coefficient( state ), state -> composite_attack_power(),
                   state -> composite_ta_multiplier(),
                   dot_multiplier );
  }

  return amount;
}

// action_t::calculate_direct_amount ========================================

double action_t::calculate_direct_amount( action_state_t* state ) const
{
  double amount = sim -> averaged_range( base_da_min( state ), base_da_max( state ) );

  if ( round_base_dmg ) amount = floor( amount + 0.5 );

  if ( amount == 0 && weapon_multiplier == 0 && attack_direct_power_coefficient( state ) == 0 && spell_direct_power_coefficient( state ) == 0 ) return 0;

  double base_direct_amount = amount;
  double weapon_amount = 0;

  amount += bonus_da( state );

  if ( weapon_multiplier > 0 )
  {
    // x% weapon damage + Y
    // e.g. Obliterate, Shred, Backstab
    amount += calculate_weapon_damage( state -> attack_power );
    amount *= weapon_multiplier;
    weapon_amount = amount;
  }
  amount += spell_direct_power_coefficient( state ) * ( state -> composite_spell_power() );
  amount += attack_direct_power_coefficient( state ) * ( state -> composite_attack_power() );
  amount *= state -> composite_da_multiplier();

  // damage variation in WoD is based on the delta field in the spell data, applied to entire amount
  double delta_mod = amount_delta_modifier( state );
  if ( ! sim -> average_range &&  delta_mod > 0 )
    amount *= 1 + delta_mod / 2 * sim -> averaged_range( -1.0, 1.0 );

  // AoE with decay per target
  if ( state -> chain_target > 0 && chain_multiplier != 1.0 )
    amount *= pow( chain_multiplier, state -> chain_target );

  if ( state -> chain_target > 0 && chain_bonus_damage != 0.0 )
    amount *= std::max( 1.0 + chain_bonus_damage * state -> chain_target, 0.0 );

  // AoE with static reduced damage per target
  if ( state -> chain_target > 0 )
    amount *= base_aoe_multiplier;

  // Spell splits damage across all targets equally
  if ( state -> action -> split_aoe_damage )
    amount /= state -> n_targets;

  amount *= composite_aoe_multiplier( state );

  // Spell goes over the maximum number of AOE targets - ignore for enemies
  if ( ! state -> action -> split_aoe_damage && state -> n_targets > static_cast< size_t >( sim -> max_aoe_enemies ) && ! state -> action -> player -> is_enemy() )
    amount *= sim -> max_aoe_enemies / static_cast< double >( state -> n_targets );

  // Record initial amount to state
  state -> result_raw = amount;

  if ( state -> result == RESULT_GLANCE )
  {
    double delta_skill = ( state -> target -> level() - player -> level() ) * 5.0;

    if ( delta_skill < 0.0 )
      delta_skill = 0.0;

    double max_glance = 1.3 - 0.03 * delta_skill;

    if ( max_glance > 0.99 )
      max_glance = 0.99;
    else if ( max_glance < 0.2 )
      max_glance = 0.20;

    double min_glance = 1.4 - 0.05 * delta_skill;

    if ( min_glance > 0.91 )
      min_glance = 0.91;
    else if ( min_glance < 0.01 )
      min_glance = 0.01;

    if ( min_glance > max_glance )
    {
      double temp = min_glance;
      min_glance = max_glance;
      max_glance = temp;
    }

    amount *= sim -> averaged_range( min_glance, max_glance ); // 0.75 against +3 targets.
  }

  if ( ! sim -> average_range ) amount = floor( amount + rng().real() );

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s amount for %s: dd=%.0f i_dd=%.0f w_dd=%.0f b_dd=%.0f s_mod=%.2f s_power=%.0f a_mod=%.2f a_power=%.0f mult=%.2f w_mult=%.2f",
                   player -> name(), name(), amount, state -> result_raw, weapon_amount, base_direct_amount,
                   spell_direct_power_coefficient( state ), state -> composite_spell_power(),
                   attack_direct_power_coefficient( state ), state -> composite_attack_power(),
                   state -> composite_da_multiplier(), weapon_multiplier );
  }

  // Record total amount to state
  if ( result_is_miss( state -> result ) )
  {
    state -> result_total = 0.0;
    return 0.0;
  }
  else
  {
    state -> result_total = amount;
    return amount;
  }
}

// action_t::calculate_crit_damage_bonus ====================================

double action_t::calculate_crit_damage_bonus( action_state_t* state ) const
{
  if ( state -> result == RESULT_CRIT )
  {
    state -> result_total *= 1.0 + total_crit_bonus( state );
  }

  return state -> result_total;
}

// action_t::consume_resource ===============================================

void action_t::consume_resource()
{
  resource_e cr = current_resource();

  if ( cr == RESOURCE_NONE || base_cost() == 0 || proc ) return;

  last_resource_cost = cost();

  player -> resource_loss( cr, last_resource_cost, nullptr, this );

  if ( sim -> log )
    sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                   last_resource_cost, util::resource_type_string( cr ),
                   name(), player -> resources.current[ cr ] );

  stats -> consume_resource( current_resource(), last_resource_cost );
}

// action_t::num_targets  ==================================================

int action_t::num_targets() const
{
  int count = 0;
  for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> actor_list[ i ];

    if ( ! t -> is_sleeping() && t -> is_enemy() )
      count++;
  }

  return count;
}

// action_t::available_targets ==============================================

size_t action_t::available_targets( std::vector< player_t* >& tl ) const
{
  tl.clear();
  if ( ! target -> is_sleeping() )
    tl.push_back( target );

  for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> target_non_sleeping_list[i];

    if ( t -> is_enemy() && ( t != target ) )
    {
      tl.push_back( t );
    }
  }

  if ( sim -> debug && !sim -> distance_targeting_enabled )
  {
    sim -> out_debug.printf( "%s regenerated target cache for %s (%s)",
        player -> name(),
        signature_str.c_str(),
        name() );
    for ( size_t i = 0; i < tl.size(); i++ )
    {
      sim -> out_debug.printf( "[%u, %s (id=%u)]",
          static_cast<unsigned>( i ),
          tl[ i ] -> name(),
          tl[ i ] -> actor_index );
    }
  }

  return tl.size();
}

// action_t::target_list ====================================================

std::vector< player_t* >& action_t::target_list() const
{
  // Check if target cache is still valid. If not, recalculate it
  if ( !target_cache.is_valid )
  {
    available_targets( target_cache.list ); // This grabs the full list of targets, which will also pickup various awfulness that some classes have.. such as prismatic crystal.
    check_distance_targeting( target_cache.list );
    target_cache.is_valid = true;
  }

  return target_cache.list;
}

player_t* action_t::find_target_by_number( int number ) const
{
  std::vector< player_t* >& tl = target_list();
  size_t total_targets = tl.size();

  for ( size_t i = 0, j = 1; i < total_targets; i++ )
  {
    player_t* t = tl[ i ];

    int n = ( t == player -> target ) ? 1 : as<int>( ++j );

    if ( n == number )
      return t;
  }

  return 0;
}

// action_t::calculate_block_result =========================================
// moved here now that we found out that spells can be blocked (Holy Shield)
// block_chance() and crit_block_chance() govern whether any given attack can
// be blocked or not (zero return if not)

block_result_e action_t::calculate_block_result( action_state_t* s ) const
{
  block_result_e block_result = BLOCK_RESULT_UNBLOCKED;

  // Blocks also get a their own roll, and glances/crits can be blocked.
  if ( result_is_hit( s -> result ) && may_block && ( player -> position() == POSITION_FRONT ) && ! ( s -> result == RESULT_NONE ) )
  {
    double block_total = block_chance( s );

    if ( block_total > 0 )
    {
      double crit_block = crit_block_chance( s );

      // Roll once for block, then again for crit block if the block succeeds
      if ( rng().roll( block_total ) )
      {
        if ( rng().roll( crit_block ) )
          block_result = BLOCK_RESULT_CRIT_BLOCKED;
        else
          block_result = BLOCK_RESULT_BLOCKED;
      }
    }
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s result for %s is %s", player -> name(), name(), util::block_result_type_string( block_result ) );

  return block_result;
}

// action_t::execute ========================================================

void action_t::execute()
{
#ifndef NDEBUG
  if ( ! initialized )
  {
    sim -> errorf( "action_t::execute: action %s from player %s is not initialized.\n", name(), player -> name() );
    assert( 0 );
  }
#endif

  if ( &data() == &spell_data_not_found_t::singleton )
  {
    sim -> errorf( "Player %s could not find spell data for action %s\n", player -> name(), name() );
    sim -> cancel();
  }

  if ( n_targets() == 0 && target -> is_sleeping() )
    return;

  if ( !execute_targeting( this ) )
  {
    cancel(); // This cancels the cast if the target moves out of range while the spell is casting.
    return;
  }

  if ( sim -> log && ! dual )
  {
    sim -> out_log.printf( "%s performs %s (%.0f)", player -> name(), name(),
                   player -> resources.current[ player -> primary_resource() ] );
  }

  hit_any_target = false;
  num_targets_hit = 0;
  interrupt_immediate_occurred = false;

  if ( harmful )
  {
    if ( player -> in_combat == false && sim -> debug )
      sim -> out_debug.printf( "%s enters combat.", player -> name() );

    player -> in_combat = true;
  }

  size_t num_targets;
  if ( is_aoe() ) // aoe
  {
    std::vector< player_t* >& tl = target_list();
    num_targets = ( n_targets() < 0 ) ? tl.size() : std::min( tl.size(), as<size_t>( n_targets() ) );
    for ( size_t t = 0, max_targets = tl.size(); t < num_targets && t < max_targets; t++ )
    {
      action_state_t* s = get_state( pre_execute_state );
      s -> target = tl[ t ];
      s -> n_targets = std::min( num_targets, tl.size() );
      s -> chain_target = as<int>( t );
      if ( ! pre_execute_state )
      {
        snapshot_state( s, amount_type( s ) );
      }
      // Even if pre-execute state is defined, we need to snapshot target-specific state variables
      // for aoe spells.
      else
      {
        snapshot_internal( s, snapshot_flags & STATE_TARGET, amount_type( s ) );
      }
      s -> result = calculate_result( s );
      s -> block_result = calculate_block_result( s );

      s -> result_amount = calculate_direct_amount( s );

      if ( sim -> debug )
        s -> debug();

      schedule_travel( s );
    }
  }
  else // single target
  {
    num_targets = 1;

    action_state_t* s = get_state( pre_execute_state );
    s -> target = target;
    s -> n_targets = 1;
    s -> chain_target = 0;
    if ( ! pre_execute_state ) snapshot_state( s, amount_type( s ) );
    s -> result = calculate_result( s );
    s -> block_result = calculate_block_result( s );

    s -> result_amount = calculate_direct_amount( s );

    if ( sim -> debug )
      s -> debug();

    schedule_travel( s );
  }

  if ( player -> regen_type == REGEN_DYNAMIC )
  {
    player -> do_dynamic_regen();
  }

  update_ready(); // Based on testing with warrior mechanics, Blizz updates cooldowns before consuming resources.
                  // This is very rarely relevant.
  consume_resource();

  if ( ! dual && ( !player -> first_cast || !harmful ) ) stats -> add_execute( time_to_execute, target );

  if ( pre_execute_state )
    action_state_t::release( pre_execute_state );

  // The rest of the execution depends on actually executing on target. Note that execute_state
  // can be nullptr if there are not valid targets to hit on.
  if ( num_targets > 0 )
  {
    if ( composite_teleport_distance( execute_state ) > 0 )
      do_teleport( execute_state );

    if ( execute_state && execute_action && result_is_hit( execute_state -> result ) )
    {
      assert( ! execute_action -> pre_execute_state );
      execute_action -> schedule_execute( execute_action -> get_state( execute_state ) );
    }

    // Proc generic abilities on execute.
    proc_types pt;
    if ( execute_state && callbacks && ( pt = execute_state -> proc_type() ) != PROC1_INVALID )
    {
      proc_types2 pt2;

      // "On spell cast", only performed for foreground actions
      if ( ( pt2 = execute_state -> cast_proc_type2() ) != PROC2_INVALID )
      {
        action_callback_t::trigger( player -> callbacks.procs[ pt ][ pt2 ], this, execute_state );
      }

      // "On an execute result"
      if ( ( pt2 = execute_state -> execute_proc_type2() ) != PROC2_INVALID )
      {
        action_callback_t::trigger( player -> callbacks.procs[ pt ][ pt2 ], this, execute_state );
      }
    }
  }

  // Restore the default target after execution. This is required so that
  // target caches do not get into an inconsistent state, if the target of this
  // action (defined by a number) spawns/despawns dynamically during an
  // iteration.
  if ( option.target_number > 0 && target != default_target )
  {
    target = default_target;
  }

  if ( energize_type_() == ENERGIZE_ON_CAST || ( energize_type_() == ENERGIZE_ON_HIT && hit_any_target ) )
  {
    auto amount = composite_energize_amount( execute_state );
    if ( amount != 0 )
    {
      player -> resource_gain( energize_resource_(), amount, energize_gain( execute_state ), this );
    }
  }
  else if ( energize_type_() == ENERGIZE_PER_HIT )
  {
    auto amount = composite_energize_amount( execute_state ) * num_targets_hit;
    if ( amount != 0 )
    {
      player -> resource_gain( energize_resource_(), amount, energize_gain( execute_state ), this );
    }
  }

  if ( repeating && ! proc ) schedule_execute();
}

// action_t::tick ===========================================================

void action_t::tick( dot_t* d )
{
  assert( ! d -> target -> is_sleeping() );

  // Always update the state of the base dot. This is required to allow tick action-based dots to
  // update the driver's state per tick (for example due to haste changes -> tick time).
  update_state( d -> state, amount_type( d -> state, true ) );

  if ( tick_action )
  {
    if ( tick_action -> pre_execute_state )
      action_state_t::release( tick_action -> pre_execute_state );

    action_state_t* state = tick_action -> get_state( d -> state );
    if ( dynamic_tick_action )
      update_state( state, amount_type( state, tick_action -> direct_tick ) );
    state -> da_multiplier = state -> ta_multiplier * d -> get_last_tick_factor();
    state -> target_da_multiplier = state -> target_ta_multiplier;
    tick_action -> target = d -> target;
    tick_action -> schedule_execute( state );
  }
  else
  {
    d -> state -> result = RESULT_HIT;

    if ( tick_may_crit && rng().roll( d -> state -> composite_crit_chance() ) )
      d -> state -> result = RESULT_CRIT;

    d -> state -> result_amount = calculate_tick_amount( d -> state, d -> get_last_tick_factor() * d -> current_stack() );

    assess_damage( amount_type( d -> state, true ), d -> state );

    if ( sim -> debug )
      d -> state -> debug();
  }

  if ( energize_type_() == ENERGIZE_PER_TICK )
  {
    player -> resource_gain( energize_resource_(),
      composite_energize_amount( d -> state ), gain, this );
  }

  stats -> add_tick( d -> time_to_tick, d -> state -> target );

  player -> trigger_ready();
}

// action_t::last_tick ======================================================

void action_t::last_tick( dot_t* d )
{
  if ( get_school() == SCHOOL_PHYSICAL )
  {
    buff_t* b = d -> state -> target -> debuffs.bleeding;
    if ( b -> current_value > 0 ) b -> current_value -= 1.0;
    if ( b -> current_value == 0 ) b -> expire();
  }

  if ( channeled && player -> channeling == this )
  {
    player -> channeling = 0;
    player -> readying = 0;
  }
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( dmg_e type, action_state_t* s )
{
  // Execute outbound damage assessor pipeline on the state object
  player -> assessor_out_damage.execute( type, s );

  // TODO: Should part of this move to assessing, priority_iteration_damage for example?
  if ( s -> result_amount > 0 || result_is_miss( s -> result ) )
  {
    if ( s -> target == sim -> target )
    {
      player -> priority_iteration_dmg += s -> result_amount;
    }
    record_data( s );
  }
}

// action_t::record_data ====================================================

void action_t::record_data( action_state_t* data )
{
  if ( ! stats )
    return;

  stats -> add_result( data -> result_amount, data -> result_total,
                       report_amount_type( data ),
                       data -> result,
                       ( may_block || player -> position() != POSITION_BACK )
                         ? data -> block_result
                         : BLOCK_RESULT_UNKNOWN,
                       data -> target );
}

// action_t::schedule_execute ===============================================

// Should be called only by foreground action executions (i.e., Player-Ready event calls
// player_t::execute_action() ). Background actions should (and are) directly call
// action_t::schedule_execute. Off gcd actions will either directly execute the action, or schedule
// a queued off-gcd execution.
void action_t::queue_execute( bool off_gcd )
{
  auto queue_delay = cooldown -> queue_delay();
  if ( queue_delay > timespan_t::zero() )
  {
    queue_event = make_event<queued_action_execute_event_t>( *sim, this, queue_delay, off_gcd );
    player -> queueing = this;
  }
  else
  {
    if ( off_gcd )
    {
      do_off_gcd_execute( this );
    }
    else
    {
      schedule_execute();
    }
  }
}

void action_t::schedule_execute( action_state_t* execute_state )
{
  if ( sim -> log )
  {
    sim -> out_log.printf( "%s schedules execute for %s", player -> name(), name() );
  }

  time_to_execute = execute_time();

  execute_event = start_action_execute_event( time_to_execute, execute_state );

  if ( trigger_gcd > timespan_t::zero() )
    player -> off_gcdactions.clear();

  if ( ! background )
  {
    // We were queueing this on an almost finished cooldown, so queueing is over, and we begin
    // executing this action.
    if ( player -> queueing == this )
    {
      player -> queueing = nullptr;
    }

    player -> executing = this;
    // Setup the GCD ready time, and associated haste-related values
    player -> gcd_ready = sim -> current_time() + gcd();
    player -> gcd_haste_type = gcd_haste;
    switch ( gcd_haste )
    {
      case HASTE_SPELL:
        player -> gcd_current_haste_value = player -> cache.spell_haste();
        break;
      case HASTE_ATTACK:
        player -> gcd_current_haste_value = player -> cache.attack_haste();
        break;
      case SPEED_SPELL:
        player -> gcd_current_haste_value = player -> cache.spell_speed();
        break;
      case SPEED_ATTACK:
        player -> gcd_current_haste_value = player -> cache.attack_speed();
        break;
      default:
        break;
    }

    if ( player -> action_queued && sim -> strict_gcd_queue )
    {
      player -> gcd_ready -= sim -> queue_gcd_reduction;
    }

    if ( special && time_to_execute > timespan_t::zero() && ! proc && interrupt_auto_attack )
    {
      // While an ability is casting, the auto_attack is paused
      // So we simply reschedule the auto_attack by the ability's casttime
      timespan_t time_to_next_hit;
      // Mainhand
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
      {
        time_to_next_hit  = player -> main_hand_attack -> execute_event -> occurs();
        time_to_next_hit -= sim -> current_time();
        time_to_next_hit += time_to_execute;
        player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
      // Offhand
      if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
      {
        time_to_next_hit  = player -> off_hand_attack -> execute_event -> occurs();
        time_to_next_hit -= sim -> current_time();
        time_to_next_hit += time_to_execute;
        player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
    }
  }
}

// action_t::reschedule_execute =============================================

void action_t::reschedule_execute( timespan_t time )
{
  if ( sim -> log )
  {
    sim -> out_log.printf( "%s reschedules execute for %s", player -> name(), name() );
  }

  timespan_t delta_time = sim -> current_time() + time - execute_event -> occurs();

  time_to_execute += delta_time;

  if ( delta_time > timespan_t::zero() )
  {
    execute_event -> reschedule( time );
  }
  else // Impossible to reschedule events "early".  Need to be canceled and re-created.
  {
    action_state_t* state = debug_cast<action_execute_event_t*>( execute_event ) -> execute_state;
    event_t::cancel( execute_event );
    execute_event = start_action_execute_event( time, state );
  }
}

// action_t::update_ready ===================================================

void action_t::update_ready( timespan_t cd_duration /* = timespan_t::min() */ )
{
  if ( ( cd_duration > timespan_t::zero() ||
         ( cd_duration == timespan_t::min() && cooldown -> duration > timespan_t::zero() ) ) &&
       ! dual )
  {
    timespan_t delay = timespan_t::zero();

    if ( ! background && ! proc )
    { /*This doesn't happen anymore due to the gcd queue, in WoD if an ability has a cooldown of 20 seconds,
      it is usable exactly every 20 seconds with proper Lag tolerance set in game.
      The only situation that this could happen is when world lag is over 400, as blizzard does not allow
      custom lag tolerance to go over 400.
      */
      timespan_t lag, dev;

      lag = player -> world_lag_override ? player -> world_lag : sim -> world_lag;
      dev = player -> world_lag_stddev_override ? player -> world_lag_stddev : sim -> world_lag_stddev;
      delay = rng().gauss( lag, dev );
      if ( delay > timespan_t::from_millis( 400 ) )
      {
        delay -= timespan_t::from_millis( 400 ); //Even high latency players get some benefit from CLT.
        if ( sim -> debug )
          sim -> out_debug.printf( "%s delaying the cooldown finish of %s by %f", player -> name(), name(), delay.total_seconds() );
      }
      else
        delay = timespan_t::zero();
    }

    cooldown -> start( this, cd_duration, delay );

    if ( sim->debug )
    {
      sim->out_debug.printf(
          "%s starts cooldown for %s (%s, %d/%d). Duration=%fs Delay=%fs. Will "
          "be ready at %.4f",
          player->name(), name(), cooldown->name(), cooldown->current_charge,
          cooldown->charges, cd_duration.total_seconds(), delay.total_seconds(),
          cooldown->ready.total_seconds() );
    }

    if ( internal_cooldown -> duration > timespan_t::zero() )
    {
      internal_cooldown -> start( this );
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s starts internal_cooldown for %s (%s). Will be ready at %.3f",
            player -> name(), name(), internal_cooldown -> name(), internal_cooldown -> ready.total_seconds() );
      }
    }
  }
}

// action_t::usable_moving ==================================================

bool action_t::usable_moving() const
{
  if ( player -> buffs.norgannons_foresight_ready && player -> buffs.norgannons_foresight_ready -> check() )
    return true;

  if ( execute_time() > timespan_t::zero() )
    return false;

  if ( channeled )
    return false;

  if ( range > 0 && range <= 5 )
    return false;

  return true;
}

// action_t::ready ==========================================================

bool action_t::ready()
{
  // Check conditions that do NOT pertain to the target before cycle_targets
  if ( cooldown -> is_ready() == false )
    return false;

  if ( internal_cooldown -> down() )
    return false;

  if ( rng().roll( false_negative_pct() ) )
    return false;

  if ( line_cooldown.down() )
    return false;

  if ( sync_action && ! sync_action -> ready() )
    return false;

  if ( player -> is_moving() && ! usable_moving() )
    return false;

  if ( option.moving != -1 && option.moving != ( player -> is_moving() ? 1 : 0 ) )
    return false;

  if ( ! player -> resource_available( current_resource(), cost() ) )
  {
    if ( starved_proc ) starved_proc ->  occur();
    return false;
  }

  if ( target_if_mode != TARGET_IF_NONE )
  {
    player_t* potential_target = select_target_if_target();
    if ( potential_target )
    {
      // If the target changes, we need to regenerate the target cache to get the new primary target
      // as the first element of target_list. Only do this for abilities that are aoe.
      if ( n_targets() > 1 && potential_target != target )
      {
        target_cache.is_valid = false;
      }
      if ( child_action.size() > 0 )
      {// If spell_targets is used on the child instead of the parent action, we need to reset the cache for that action as well.
        for ( size_t i = 0; i < child_action.size(); i++ )
          child_action[i] -> target_cache.is_valid = false;
      }
      target = potential_target;
    }
    else
      return false;
  }

  if ( option.cycle_targets )
  {
    player_t* saved_target = target;
    option.cycle_targets = false;
    bool found_ready = false;

    // Note, need to take a copy of the original target list here, instead of a reference. Otherwise
    // if spell_targets (or any expression that uses the target list) modifies it, the loop below
    // may break, since the number of elements on the vector is not the same as it originally was
    std::vector< player_t* > ctl = target_list();
    size_t num_targets = ctl.size();

    if ( ( option.max_cycle_targets > 0 ) && ( ( size_t ) option.max_cycle_targets < num_targets ) )
      num_targets = option.max_cycle_targets;

    for ( size_t i = 0; i < num_targets; i++ )
    {
      target = ctl[i];
      if ( ready() )
      {
        found_ready = true;
        break;
      }
    }

    option.cycle_targets = true;

    if ( found_ready )
    {
      // If the target changes, we need to regenerate the target cache to get the new primary target
      // as the first element of target_list. Only do this for abilities that are aoe.
      if ( n_targets() > 1 && target != saved_target )
      {
        target_cache.is_valid = false;
      }
      return true;
    }

    target = saved_target;

    return false;
  }

  if ( option.cycle_players ) // Used when healing players in the raid.
  {
    player_t* saved_target = target;
    option.cycle_players = false;
    bool found_ready = false;

    std::vector<player_t*>& tl = sim -> player_no_pet_list.data();

    size_t num_targets = tl.size();

    if ( ( option.max_cycle_targets > 0 ) && ( (size_t)option.max_cycle_targets < num_targets ) )
      num_targets = option.max_cycle_targets;

    for ( size_t i = 0; i < num_targets; i++ )
    {
      target = tl[i];
      if ( ready() )
      {
        found_ready = true;
        break;
      }
    }

    option.cycle_players = true;

    if ( found_ready ) return true;

    target = saved_target;

    return false;
  }

  if ( option.target_number )
  {
    player_t* saved_target  = target;
    int saved_target_number = option.target_number;
    option.target_number = 0;

    target = find_target_by_number( saved_target_number );

    bool is_ready = false;

    if ( target ) is_ready = ready();

    option.target_number = saved_target_number;

    if ( is_ready ) return true;

    target = saved_target;

    return false;
  }

  if ( sim -> distance_targeting_enabled && range > 0 &&
    player -> get_player_distance( *target ) > range + target -> combat_reach )
    return false;

  if ( target -> debuffs.invulnerable -> check() && harmful )
    return false;

  if ( target -> is_sleeping() )
    return false;

  if ( ! has_movement_directionality() )
    return false;

  if ( rng().roll( false_positive_pct() ) )
    return true;

  if ( if_expr && !if_expr -> success() )
    return false;

  return true;
}

// action_t::init ===========================================================

void action_t::init()
{
  if ( initialized ) return;

  if ( ! verify_actor_level() || ! verify_actor_spec() )
  {
    background = true;
  }


  assert( !( impact_action && tick_action ) && "Both tick_action and impact_action should not be used in a single action." );

  assert( !( n_targets() && channeled ) && "DONT create a channeled aoe spell!" );

  if ( !option.sync_str.empty() )
  {
    sync_action = player -> find_action( option.sync_str );

    if ( !sync_action )
    {
      sim -> errorf( "Unable to find sync action '%s' for primary action '%s'\n", option.sync_str.c_str(), name() );
      sim -> cancel();
    }
  }

  if ( option.cycle_targets && option.target_number )
  {
    option.target_number = 0;
    sim -> errorf( "Player %s trying to use both cycle_targets and a numerical target for action %s - defaulting to cycle_targets\n", player -> name(), name() );
  }

  if ( tick_action )
  {
    tick_action -> direct_tick = true;
    tick_action -> dual = true;
    tick_action -> stats = stats;
    tick_action -> parent_dot = target -> get_dot( name_str, player );
    if ( tick_action -> parent_dot && range > 0 && tick_action -> radius > 0 && tick_action -> is_aoe() )
     // If the parent spell has a range, the tick_action has a radius and is an aoe spell, then the tick action likely also has a range.
     // This will allow distance_target_t to correctly determine spells that radiate from the target, instead of the player.
       tick_action -> range = range;
    stats -> action_list.push_back( tick_action );
  }

  stats -> school      = get_school() ;

  if ( quiet ) stats -> quiet = true;

  if ( may_crit || tick_may_crit )
    snapshot_flags |= STATE_CRIT | STATE_TGT_CRIT;

  if ( ( base_td > 0 || spell_power_mod.tick > 0 || attack_power_mod.tick > 0 || tick_action ) && dot_duration > timespan_t::zero() )
    snapshot_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA | STATE_MUL_PERSISTENT | STATE_VERSATILITY;

  if ( base_dd_min > 0 || ( spell_power_mod.direct > 0 || attack_power_mod.direct > 0 ) || weapon_multiplier > 0 )
    snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_MUL_PERSISTENT | STATE_VERSATILITY;

  if ( player -> is_pet() &&
       ( snapshot_flags & ( STATE_MUL_DA | STATE_MUL_TA | STATE_TGT_MUL_DA | STATE_TGT_MUL_TA |
                            STATE_MUL_PERSISTENT | STATE_VERSATILITY ) ) )
  {
    snapshot_flags |= STATE_MUL_PET;
  }

  if ( school == SCHOOL_PHYSICAL )
    snapshot_flags |= STATE_TGT_ARMOR;

  // Tick actions use tick multipliers, so snapshot them too if direct
  // multipliers are snapshot for "base" ability
  if ( tick_action && ( snapshot_flags & STATE_MUL_DA ) > 0 )
  {
    snapshot_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA;
  }

  if ( ( spell_power_mod.direct > 0 || spell_power_mod.tick > 0 ) )
  {
    snapshot_flags |= STATE_SP;
  }

  if ( ( weapon_power_mod > 0 || attack_power_mod.direct > 0 || attack_power_mod.tick > 0 ) )
  {
    snapshot_flags |= STATE_AP;
  }

  if ( dot_duration > timespan_t::zero() && ( hasted_ticks || channeled ) )
    snapshot_flags |= STATE_HASTE;

  // WOD: Dot Snapshoting is gone
  update_flags |= snapshot_flags;

  // WOD: Yank out persistent multiplier from update flags, so they get
  // snapshot once at the application of the spell
  update_flags &= ~STATE_MUL_PERSISTENT;

  // Channeled dots get haste snapshoted
  if ( channeled )
  {
    update_flags &= ~STATE_HASTE;
  }

  if ( !(background || sequence) && (action_list && action_list -> name_str == "precombat") )
  {
    if ( harmful )
    {
      if ( this -> travel_speed > 0 || this -> base_execute_time > timespan_t::zero() )
      {
        player -> precombat_action_list.push_back( this );
      }
      else
        sim -> errorf( "Player %s attempted to add harmful action %s to precombat action list. Only spells with travel time/cast time may be cast here.", player -> name(), name() );
    }
    else
      player -> precombat_action_list.push_back( this );
  }
  else if ( ! ( background || sequence ) && action_list )
    player -> find_action_priority_list( action_list -> name_str ) -> foreground_action_list.push_back( this );

  initialized = true;

#ifndef NDEBUG
  if ( sim -> debug && sim -> distance_targeting_enabled )
    sim -> out_debug.printf( "%s - radius %.1f - range - %.1f", name(), radius, range );
#endif

  consume_per_tick_ = range::find_if( base_costs_per_tick, []( const double& d ) { return d != 0; } ) != base_costs_per_tick.end();

  // Setup default target in init
  default_target = target;

  // Make sure background is set for triggered actions.
  // Leads to double-readying of the player otherwise.
  assert( ( !execute_action || execute_action->background ) &&
          "Execute action needs to be set to background." );
  assert( ( !tick_action || tick_action->background ) &&
          "Tick action needs to be set to background." );
  assert( ( !impact_action || impact_action->background ) &&
          "Impact action needs to be set to background." );
}

bool action_t::init_finished()
{
  bool ret = true;

  if ( !option.target_if_str.empty() )
  {
    std::string::size_type offset = option.target_if_str.find( ':' );
    if ( offset != std::string::npos )
    {
      std::string target_if_type_str = option.target_if_str.substr( 0, offset );
      option.target_if_str.erase( 0, offset + 1 );
      if ( util::str_compare_ci( target_if_type_str, "max" ) )
      {
        target_if_mode = TARGET_IF_MAX;
      }
      else if ( util::str_compare_ci( target_if_type_str, "min" ) )
      {
        target_if_mode = TARGET_IF_MIN;
      }
      else if ( util::str_compare_ci( target_if_type_str, "first" ) )
      {
        target_if_mode = TARGET_IF_FIRST;
      }
      else
      {
        sim -> errorf( "%s unknown target_if mode '%s' for choose_target. Valid values are 'min', 'max', 'first'.",
          player -> name(), target_if_type_str.c_str() );
        background = true;
      }
    }
    else if ( !option.target_if_str.empty() )
    {
      target_if_mode = TARGET_IF_FIRST;
    }

    if ( !option.target_if_str.empty() &&
      ( target_if_expr = expr_t::parse( this, option.target_if_str, sim -> optimize_expressions ) ) == 0 )
      ret = false;
  }

  if ( ! option.if_expr_str.empty() &&
       ( if_expr = expr_t::parse( this, option.if_expr_str, sim -> optimize_expressions ) ) == 0 )
  {
    ret = false;
  }

  if ( ! option.interrupt_if_expr_str.empty() &&
       ( interrupt_if_expr = expr_t::parse( this, option.interrupt_if_expr_str, sim -> optimize_expressions ) ) == 0 )
  {
    ret = false;
  }

  if ( ! option.early_chain_if_expr_str.empty() &&
       ( early_chain_if_expr = expr_t::parse( this, option.early_chain_if_expr_str, sim -> optimize_expressions ) ) == 0 )
  {
    ret = false;
  }

  return ret;
}

// action_t::reset ==========================================================

void action_t::reset()
{
  if ( pre_execute_state )
  {
    action_state_t::release( pre_execute_state );
  }
  cooldown -> reset_init();
  internal_cooldown -> reset_init();
  line_cooldown.reset_init();
  execute_event = nullptr;
  queue_event = nullptr;
  interrupt_immediate_occurred = false;
  travel_events.clear();
  target = default_target;

  if( sim -> current_iteration == 1 )
  {
    if( if_expr )
    {
      if_expr = if_expr -> optimize();
      if ( sim -> optimize_expressions && if_expr -> always_false() )
      {
        assert( action_list );
        std::vector<action_t*>::iterator i = std::find( action_list -> foreground_action_list.begin(),
                                                        action_list -> foreground_action_list.end(),
                                                        this );
        if ( i != action_list -> foreground_action_list.end() )
        {
          action_list -> foreground_action_list.erase( i );
        }
      }
    }
    if ( target_if_expr ) target_if_expr = target_if_expr -> optimize();
    if( interrupt_if_expr ) interrupt_if_expr = interrupt_if_expr -> optimize();
    if( early_chain_if_expr ) early_chain_if_expr = early_chain_if_expr -> optimize();
  }
}

// action_t::cancel =========================================================

void action_t::cancel()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "action %s of %s is canceled", name(), player -> name() );

  if ( channeled )
  {
    if ( dot_t* d = find_dot( target ) )
      d -> cancel();
  }

  bool was_busy = false;

  if ( player -> queueing == this )
  {
    was_busy = true;
    player -> queueing = nullptr;
  }
  if ( player -> executing  == this )
  {
    was_busy = true;
    player -> executing  = nullptr;
  }
  if ( player -> channeling == this )
  {
    was_busy = true;
    player -> channeling  = nullptr;
  }

  event_t::cancel( execute_event );
  event_t::cancel( queue_event );

  player -> debuffs.casting -> expire();

  if ( was_busy && player -> arise_time >= timespan_t::zero() && ! player -> readying )
    player -> schedule_ready();
}

// action_t::interrupt ======================================================

void action_t::interrupt_action()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "action %s of %s is interrupted", name(), player -> name() );

  // Don't start cooldown if we're queueing this action
  if ( ! player -> queueing && cooldown -> duration > timespan_t::zero() && ! dual )
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s starts cooldown for %s (%s)", player -> name(), name(), cooldown -> name() );

    // Cooldown must be usable to start it. TODO: Is this really right? Interrupting a cooldowned
    // cast should not start the cooldown, imo?
    if ( cooldown -> up() )
    {
      cooldown -> start( this );
    }
  }

  // Don't start internal cooldown if we're queueing this action
  if ( ! player -> queueing && internal_cooldown -> duration > timespan_t::zero() && ! dual )
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s starts internal_cooldown for %s (%s)",
          player -> name(), name(), internal_cooldown -> name() );

    internal_cooldown -> start( this );
  }

  if ( player -> executing  == this ) player -> executing = nullptr;
  if ( player -> queueing   == this ) player -> queueing = nullptr;
  if ( player -> channeling == this )
  {
    dot_t* dot = get_dot( execute_state -> target );
    assert( dot -> is_ticking() );
    dot -> cancel();
  }

  event_t::cancel( execute_event );
  event_t::cancel( queue_event );

  player -> debuffs.casting -> expire();
}

// action_t::check_spec =====================================================

void action_t::check_spec( specialization_e necessary_spec )
{
  if ( player -> specialization() != necessary_spec )
  {
    sim -> errorf( "Player %s attempting to execute action %s without %s spec.\n",
                   player -> name(), name(), dbc::specialization_string( necessary_spec ).c_str() );

    background = true; // prevent action from being executed
  }
}

// action_t::check_spec =====================================================

void action_t::check_spell( const spell_data_t* sp )
{
  if ( ! sp -> ok() )
  {
    sim -> errorf( "Player %s attempting to execute action %s without spell ok().\n",
                   player -> name(), name() );

    background = true; // prevent action from being executed
  }
}

// action_t::create_expression ==============================================

expr_t* action_t::create_expression( const std::string& name_str )
{
  class action_expr_t : public expr_t
  {
  public:
    action_t& action;

    action_expr_t( const std::string& name, action_t& a ) :
      expr_t( name ), action( a ) {}
  };

  class amount_expr_t : public action_expr_t
  {
  public:
    dmg_e           amount_type;
    result_e        result_type;
    action_state_t* state;
    bool            average_crit;

    amount_expr_t( const std::string& name, dmg_e at, action_t& a, result_e rt = RESULT_NONE ) :
      action_expr_t( name, a ), amount_type( at ), result_type( rt ), state( a.get_state() ),
      average_crit( false )
    {
      if ( result_type == RESULT_NONE )
      {
        result_type = RESULT_HIT;
        average_crit = true;
      }

      state -> n_targets    = 1;
      state -> chain_target = 0;
      state -> result       = result_type;
    }

    virtual double evaluate() override
    {
      action.snapshot_state( state, amount_type );
      state -> target = action.target;
      double a;
      if ( amount_type == DMG_OVER_TIME || amount_type == HEAL_OVER_TIME )
        a = action.calculate_tick_amount( state, 1.0 /* Assumes full tick & one stack */ );
      else
      {
        state -> result_amount = action.calculate_direct_amount( state );
        if ( state -> result == RESULT_CRIT )
        {
          state -> result_amount = action.calculate_crit_damage_bonus( state );
        }
        if ( amount_type == DMG_DIRECT )
          state -> target -> target_mitigation( action.get_school(), amount_type, state );
        a = state -> result_amount;
      }

      if ( average_crit )
      {
        a *= 1.0 + clamp( state -> crit_chance + state -> target_crit_chance, 0.0, 1.0 ) *
          action.composite_player_critical_multiplier( state );
      }

      return a;
    }

    virtual ~amount_expr_t()
    { delete state; }
  };

  if ( name_str == "cast_time" )
    return make_mem_fn_expr( name_str, *this, &action_t::execute_time );
  else if ( name_str == "usable" )
  {
    return make_mem_fn_expr( name_str, *cooldown, &cooldown_t::is_ready );
  }
  else if ( name_str == "usable_in" )
  {
    return make_fn_expr( name_str, [ this ]() {
      if ( ! cooldown -> is_ready() )
      {
        return cooldown -> remains().total_seconds();
      }
      auto ready_at = ( cooldown -> ready - cooldown -> player -> cooldown_tolerance() );
      auto current_time = cooldown -> sim.current_time();
      if ( ready_at <= current_time )
      {
        return 0.0;
      }

      return ( ready_at - current_time ).total_seconds();
    } );
  }
  else if ( name_str == "cost" )
    return make_mem_fn_expr( name_str, *this, &action_t::cost );
  else if ( name_str == "target" )
  {
    struct target_expr_t : public action_expr_t
    {
      target_expr_t( action_t& a ) : action_expr_t( "target", a )
      { }

      double evaluate() override
      { return static_cast<double>( action.target -> actor_index ); }
    };
    return new target_expr_t( *this );
  }
  else if ( name_str == "gcd" )
    return make_mem_fn_expr( name_str, *this, &action_t::gcd );
  else if ( name_str == "execute_time" )
  {
    struct execute_time_expr_t: public action_expr_t
    {
      action_state_t* state;
      action_t& action;
      execute_time_expr_t( action_t& a ):
        action_expr_t( "execute_time", a ), state( a.get_state() ), action( a )
      { }

      double evaluate() override
      {
        if ( action.channeled )
        {
          action.snapshot_state( state, RESULT_TYPE_NONE );
          state -> target = action.target;
          return action.composite_dot_duration( state ).total_seconds() + action.execute_time().total_seconds();
        }
        else
          return std::max( action.execute_time().total_seconds(), action.gcd().total_seconds() );
      }

      virtual ~execute_time_expr_t()
      {
        delete state;
      }
    };
    return new execute_time_expr_t( *this );
  }
  else if ( name_str == "cooldown" )
    return make_ref_expr( name_str, cooldown -> duration );
  else if ( name_str == "tick_time" )
  {
    struct tick_time_expr_t : public action_expr_t
    {
      tick_time_expr_t( action_t& a ) : action_expr_t( "tick_time", a ) {}
      virtual double evaluate() override
      {
        dot_t* dot = action.get_dot();
        if ( dot -> is_ticking() )
          return action.tick_time( dot -> state ).total_seconds();
        else
          return 0;
      }
    };
    return new tick_time_expr_t( *this );
  }
  else if ( name_str == "new_tick_time" )
  {
    struct new_tick_time_expr_t : public action_expr_t
    {
      action_state_t* state;

      new_tick_time_expr_t( action_t& a ) : action_expr_t( "new_tick_time", a ), state( a.get_state() ) {}
      virtual double evaluate() override
      {
        action.snapshot_state( state, DMG_OVER_TIME );
        return action.tick_time( state ).total_seconds();
      }

      ~new_tick_time_expr_t()
      { action_state_t::release( state ); }
    };
    return new new_tick_time_expr_t( *this );
  }
  else if ( name_str == "travel_time" )
    return make_mem_fn_expr( name_str, *this, &action_t::travel_time );

  // FIXME! DoT Expressions should not need to get the dot itself.
  else if ( expr_t* q = get_dot() -> create_expression( this, name_str, true ) )
    return q;

  else if ( name_str == "miss_react" )
  {
    struct miss_react_expr_t : public action_expr_t
    {
      miss_react_expr_t( action_t& a ) : action_expr_t( "miss_react", a ) {}
      virtual double evaluate() override
      {
        dot_t* dot = action.get_dot();
        if ( dot -> miss_time < timespan_t::zero() ||
             action.sim -> current_time() >= ( dot -> miss_time ) )
          return true;
        else
          return false;
      }
    };
    return new miss_react_expr_t( *this );
  }
  else if ( name_str == "cooldown_react" )
  {
    struct cooldown_react_expr_t : public action_expr_t
    {
      cooldown_react_expr_t( action_t& a ) : action_expr_t( "cooldown_react", a ) {}
      virtual double evaluate() override
      {
        return action.cooldown -> up() &&
               action.cooldown -> reset_react <= action.sim -> current_time();
      }
    };
    return new cooldown_react_expr_t( *this );
  }
  else if ( name_str == "cast_delay" )
  {
    struct cast_delay_expr_t : public action_expr_t
    {
      cast_delay_expr_t( action_t& a ) : action_expr_t( "cast_delay", a ) {}
      virtual double evaluate() override
      {
        if ( action.sim -> debug )
        {
          action.sim -> out_debug.printf( "%s %s cast_delay(): can_react_at=%f cur_time=%f",
                                action.player -> name_str.c_str(),
                                action.name_str.c_str(),
                                ( action.player -> cast_delay_occurred + action.player -> cast_delay_reaction ).total_seconds(),
                                action.sim -> current_time().total_seconds() );
        }

        if ( action.player -> cast_delay_occurred == timespan_t::zero() ||
             action.player -> cast_delay_occurred + action.player -> cast_delay_reaction < action.sim -> current_time() )
          return true;
        else
          return false;
      }
    };
    return new cast_delay_expr_t( *this );
  }
  else if ( name_str == "tick_multiplier" )
  {
    struct tick_multiplier_expr_t : public expr_t
    {
      action_t* action;
      action_state_t* state;

      tick_multiplier_expr_t( action_t* a ) :
        expr_t( "tick_multiplier" ), action( a ), state( a -> get_state() )
      {
        state -> n_targets    = 1;
        state -> chain_target = 0;
      }

      virtual double evaluate() override
      {
        action -> snapshot_state( state, RESULT_TYPE_NONE );
        state -> target = action -> target;

        return action -> composite_ta_multiplier( state );
      }

      virtual ~tick_multiplier_expr_t()
      { delete state; }
    };

    return new tick_multiplier_expr_t( this );
  }
  else if ( name_str == "persistent_multiplier" )
  {
    struct persistent_multiplier_expr_t : public expr_t
    {
      action_t* action;
      action_state_t* state;

      persistent_multiplier_expr_t( action_t* a ) :
        expr_t( "persistent_multiplier" ), action( a ), state( a -> get_state() )
      {
        state -> n_targets    = 1;
        state -> chain_target = 0;
      }

      virtual double evaluate() override
      {
        action -> snapshot_state( state, RESULT_TYPE_NONE );
        state -> target = action -> target;

        return action -> composite_persistent_multiplier( state );
      }

      virtual ~persistent_multiplier_expr_t()
      { delete state; }
    };

    return new persistent_multiplier_expr_t( this );
  }
  else if ( name_str == "charges"
    || name_str == "charges_fractional"
    || name_str == "max_charges"
    || name_str == "recharge_time"
    || name_str == "full_recharge_time" )
  {
    return cooldown -> create_expression( this, name_str );
  }
  else if ( name_str == "damage" )
    return new amount_expr_t( name_str, DMG_DIRECT, *this );
  else if ( name_str == "hit_damage" )
    return new amount_expr_t( name_str, DMG_DIRECT, *this, RESULT_HIT );
  else if ( name_str == "crit_damage" )
    return new amount_expr_t( name_str, DMG_DIRECT, *this, RESULT_CRIT );
  else if ( name_str == "hit_heal" )
    return new amount_expr_t( name_str, HEAL_DIRECT, *this, RESULT_HIT );
  else if ( name_str == "crit_heal" )
    return new amount_expr_t( name_str, HEAL_DIRECT, *this, RESULT_CRIT );
  else if ( name_str == "tick_damage" )
    return new amount_expr_t( name_str, DMG_OVER_TIME, *this );
  else if ( name_str == "hit_tick_damage" )
    return new amount_expr_t( name_str, DMG_OVER_TIME, *this, RESULT_HIT );
  else if ( name_str == "crit_tick_damage" )
    return new amount_expr_t( name_str, DMG_OVER_TIME, *this, RESULT_CRIT );
  else if ( name_str == "tick_heal" )
    return new amount_expr_t( name_str, HEAL_OVER_TIME, *this, RESULT_HIT );
  else if ( name_str == "crit_tick_heal" )
    return new amount_expr_t( name_str, HEAL_OVER_TIME, *this, RESULT_CRIT );
  else if ( name_str == "crit_pct_current" )
  {
    struct crit_pct_current_expr_t : public expr_t
    {
      action_t* action;
      action_state_t* state;

      crit_pct_current_expr_t( action_t* a ) :
        expr_t( "crit_pct_current" ), action( a ), state( a -> get_state() )
      {
        state -> n_targets = 1;
        state -> chain_target = 0;
      }
      virtual double evaluate() override
      {
        state -> target = action -> target;
        action -> snapshot_state( state, RESULT_TYPE_NONE );

        return std::min( 100.0, state -> composite_crit_chance() * 100.0 );
      }

      virtual ~crit_pct_current_expr_t()
      { delete state; }
    };
    return new crit_pct_current_expr_t( this );
  }

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits.size() == 2 )
  {
    if ( splits[0] == "active_enemies_within" )
    {
      if ( sim -> distance_targeting_enabled )
      {
        struct active_enemies_t: public expr_t
        {
          action_t* action;
          const std::string& yards;
          double yards_from_player;
          int num_targets;
          active_enemies_t( action_t* p, const std::string& r ):
            expr_t( "active_enemies_within" ), action( p ), yards( r )
          {
            yards_from_player = util::str_to_num<int>( yards );
            num_targets = 0;
          }

          double evaluate() override
          {
            num_targets = 0;
            for ( size_t i = 0, actors = action -> player -> sim -> target_non_sleeping_list.size(); i < actors; i++ )
            {
              player_t* t = action -> player -> sim -> target_non_sleeping_list[i];
              if ( action -> player -> get_player_distance( *t ) <= yards_from_player )
                num_targets++;
            }
            return num_targets;
          }
        };
        return new active_enemies_t( this, splits[1] );
      }
      else
      { // If distance targeting is not enabled, default to active_enemies behavior.
        return make_ref_expr( name_str, sim -> active_enemies );
      }
    }
    if ( splits[0] == "prev" )
    {
      struct prev_expr_t: public action_expr_t
      {
        action_t* prev;
        prev_expr_t( action_t& a, const std::string& prev_action ): action_expr_t( "prev", a ),
          prev( a.player -> find_action( prev_action ) )
        {}
        virtual double evaluate() override
        {
          if ( action.player -> last_foreground_action )
            return action.player -> last_foreground_action -> internal_id == prev -> internal_id;
          return false;
        }
      };
      return new prev_expr_t( *this, splits[1] );
    }
    else if ( splits[0] == "prev_off_gcd" )
    {
      struct prev_gcd_expr_t: public action_expr_t
      {
        action_t* previously_off_gcd;
        prev_gcd_expr_t( action_t& a, const std::string& offgcdaction ): action_expr_t( "prev_off_gcd", a ),
          previously_off_gcd( a.player -> find_action( offgcdaction ) )
        {
        }
        virtual double evaluate() override
        {
          if ( previously_off_gcd != nullptr && action.player -> off_gcdactions.size() > 0 )
          {
            for ( size_t i = 0; i < action.player -> off_gcdactions.size(); i++ )
            {
              if ( action.player -> off_gcdactions[i] -> internal_id == previously_off_gcd -> internal_id )
                return true;
            }
          }
          return false;
        }
      };
      return new prev_gcd_expr_t( *this, splits[1] );
    }
    else if ( splits[0] == "gcd" )
    {
      if ( splits[1] == "max" )
      {
        struct gcd_expr_t: public action_expr_t
        {
          double gcd_time;
          gcd_expr_t( action_t & a ): action_expr_t( "gcd", a ),
          gcd_time( 0 )
          {
          }
          double evaluate() override
          {
            gcd_time = action.player -> base_gcd.total_seconds();
            if ( action.player -> cache.attack_haste() < action.player -> cache.spell_haste() )
              gcd_time *= action.player -> cache.attack_haste();
            else
              gcd_time *= action.player -> cache.spell_haste();

            auto min_gcd = action.min_gcd.total_seconds();
            if ( min_gcd == 0 )
            {
              min_gcd = 0.750;
            }

            if ( gcd_time < min_gcd )
              gcd_time = min_gcd;
            return gcd_time;
          }
        };
        return new gcd_expr_t( *this );
      }
      else if ( splits[1] == "remains" )
      {
        struct gcd_remains_expr_t: public action_expr_t
        {
          double gcd_remains;
          gcd_remains_expr_t( action_t & a ): action_expr_t( "gcd", a ),
          gcd_remains( 0 )
          {
          }
          double evaluate() override
          {
            gcd_remains = ( action.player -> gcd_ready - action.sim -> current_time() ).total_seconds();
            if ( gcd_remains < 0 ) // It's possible for this to return negative numbers.
              gcd_remains = 0;
            return gcd_remains;
          }
        };
        return new gcd_remains_expr_t( *this );
      }
    }
  }

  if ( splits.size() <= 2 && splits[0] == "spell_targets" )
  {
    if ( sim -> distance_targeting_enabled )
    {
      struct spell_targets_t: public expr_t
      {
        action_t* spell;
        action_t& original_spell;
        const std::string name_of_spell;
        bool second_attempt;
        spell_targets_t( action_t& a, const std::string spell_name ): expr_t( "spell_targets" ), original_spell( a ),
          name_of_spell( spell_name ), second_attempt( false )
        {
          spell = a.player -> find_action( spell_name );
          if ( !spell )
          {
            for ( size_t i = 0, size = a.player -> pet_list.size(); i < size; i++ )
            {
              spell = a.player -> pet_list[i] -> find_action( spell_name );
            }
          }
        }

        double evaluate() override
        {
          if ( spell )
          {
            spell -> target = original_spell.target;
            spell -> target_cache.is_valid = false;
            spell -> target_list();
            return static_cast<double>( spell -> target_list().size() );
          }
          else if ( !second_attempt )
          { // There are cases where spell_targets may be looking for a spell that hasn't had an action created yet.
            // This allows it to check one more time during the sims runtime, just in case the action has been created.
            spell = original_spell.player -> find_action( name_of_spell );
            if ( !spell )
            {
              for ( size_t i = 0, size = original_spell.player -> pet_list.size(); i < size; i++ )
              {
                spell = original_spell.player -> pet_list[i] -> find_action( name_of_spell );
              }
            }
            if ( !spell )
            {
              original_spell.sim -> errorf( "Warning: %s used invalid spell_targets action \"%s\"", original_spell.player -> name(), name_of_spell.c_str() );
            }
            else
            {
              spell -> target = original_spell.target;
              spell -> target_cache.is_valid = false;
              spell -> target_list();
              return static_cast<double>( spell -> target_list().size() );
            }
            second_attempt = true;
          }
          return 0;
        }
      };
      return new spell_targets_t( *this, splits.size() > 1 ? splits[ 1 ] : this -> name_str );
    }
    else
    { // If distance targeting is not enabled, default to active_enemies behavior.
      return make_ref_expr( name_str, sim -> active_enemies );
    }
  }

  if ( splits.size() == 3 && splits[0] == "prev_gcd" )
  {
    int gcd = util::to_int( splits[1] );
    if ( gcd <= 0 )
    {
      sim -> errorf( "%s expression creation error: invalid parameters for expression 'prev_gcd.<number>.<action>'",
            player -> name() );
      return 0;
    }

    struct prevgcd_expr_t: public action_expr_t {
      int gcd;
      action_t* previously_used;
      prevgcd_expr_t( action_t& a, int gcd, const std::string& prev_action ): action_expr_t( "prev_gcd", a ),
        gcd( gcd ), // prevgcd.1.action will mean 1 gcd ago, prevgcd.2.action will mean 2 gcds ago, etc.
        previously_used( a.player -> find_action( prev_action ) )
      {}
      virtual double evaluate() override
      {
        if ( as<int>(action.player -> prev_gcd_actions.size()) >= gcd )
          return ( *( action.player -> prev_gcd_actions.end() - gcd ) ) -> internal_id == previously_used -> internal_id;
        return false;
      }
    };
    return new prevgcd_expr_t( *this, gcd, splits[2] );
  }
  if ( splits.size() == 3 && splits[ 0 ] == "dot" )
  {
    auto expr = target -> get_dot( splits[ 1 ], player ) -> create_expression( this, splits[ 2 ], true );
    if ( expr )
    {
      return expr;
    }
  }
  if ( splits.size() == 3 && splits[ 0 ] == "enemy_dot" )
  {
    // simple by-pass to test
    auto dt_ = player -> get_dot( splits[ 1 ], target ) -> create_expression( this, splits[ 2 ], false );
    if ( dt_ )
      return dt_;

    // more complicated version, cycles through possible sources
    std::vector<expr_t*> dot_expressions;
    for ( size_t i = 0, size = sim -> target_list.size(); i < size; i++ )
    {
      dot_t* d = player -> get_dot( splits[ 1 ], sim -> target_list[ i ] );
      dot_expressions.push_back( d -> create_expression( this, splits[ 2 ], false ) );
    }
    struct enemy_dots_expr_t : public expr_t
    {
      std::vector<expr_t*> expr_list;

      enemy_dots_expr_t( const std::vector<expr_t*>& expr_list ) :
        expr_t( "enemy_dot" ), expr_list( expr_list )
      {
      }

      double evaluate() override
      {
        double ret = 0;
        for (auto expr : expr_list)
        {
          double expr_result = expr -> eval();
          if ( expr_result != 0 && ( expr_result < ret || ret == 0 ) )
            ret = expr_result;
        }
        return ret;
      }
    };

    return new enemy_dots_expr_t( dot_expressions );
  }
  if ( splits.size() == 3 && splits[0] == "buff" )
  {
    return player -> create_expression( this, name_str );
  }

  if ( splits.size() == 3 && splits[ 0 ] == "debuff" )
  {
    expr_t* debuff_expr = buff_t::create_expression( splits[ 1 ], this, splits[ 2 ] );
    if ( debuff_expr ) return debuff_expr;
  }

  if ( splits.size() == 3 && splits[ 0 ] == "aura" )
  {
    return sim -> create_expression( this, name_str );
  }

  if ( splits.size() >= 2 && splits[ 0 ] == "target" )
  {
    // Find target
    player_t* expr_target = 0;
    if ( splits[ 1 ][ 0 ] >= '0' && splits[ 1 ][ 0 ] <= '9' )
    {
      expr_target = find_target_by_number( atoi( splits[ 1 ].c_str() ) );
      assert( expr_target );
    }

    size_t start_rest = 2;
    if ( ! expr_target )
    {
      start_rest = 1;
    }
    else
    {
      if ( splits.size() == 2 )
      {
        sim -> errorf( "%s expression creation error: insufficient parameters for expression 'target.<number>.<expression>'",
            player -> name() );
        return 0;
      }
    }

    if ( util::str_compare_ci( splits[ start_rest ], "distance" ) )
      return make_ref_expr( "distance", player -> base.distance );

    std::string rest = splits[ start_rest ];
    for ( size_t i = start_rest + 1; i < splits.size(); ++i )
      rest += '.' + splits[ i ];

    // Target.1.foo expression, bail out early.
    if ( expr_target )
      return expr_target -> create_expression( this, rest );

    // Ensure that we can create an expression, if not, bail out early
    auto expr_ptr = target -> create_expression( this, rest );
    if ( expr_ptr == nullptr )
    {
      return nullptr;
    }
    // Delete the freshly created expression that tested for expression validity
    delete expr_ptr;

    // Proxy target based expression, allowing "dynamic switching" of targets
    // for the "target.<expression>" expressions. Generates a suitable
    // expression on demand for each target during run-time.
    //
    // Note, if we ever go dynamic spawning of adds and such, this method needs
    // to change (evaluate() has to resize the pointer array run-time, instead
    // of statically sized one based on constructor).
    struct target_proxy_expr_t : public action_expr_t
    {
      std::vector<expr_t*> proxy_expr;
      std::string suffix_expr_str;

      target_proxy_expr_t( action_t& a, const std::string& expr_str ) :
        action_expr_t( "target_proxy_expr", a ), suffix_expr_str( expr_str )
      {
        proxy_expr.resize( a.sim -> actor_list.size() + 1, 0 );
      }

      double evaluate() override
      {
        if ( proxy_expr[ action.target -> actor_index ] == 0 )
        {
          proxy_expr[ action.target -> actor_index ] = action.target -> create_expression( &action, suffix_expr_str );
        }
        assert( proxy_expr[ action.target -> actor_index ] );

        return proxy_expr[ action.target -> actor_index ] -> eval();
      }

      ~target_proxy_expr_t()
      { range::dispose( proxy_expr ); }
    };

    return new target_proxy_expr_t( *this, rest );
  }

  // necessary for self.target.*, self.dot.*
  if ( splits.size() >= 2 && splits[ 0 ] == "self" )
  {
    std::string rest = splits[1];
    for ( size_t i = 2; i < splits.size(); ++i )
      rest += '.' + splits[i];
    return player -> create_expression( this, rest );
  }

  // necessary for sim.target.*
  if ( splits.size() >= 2 && splits[ 0 ] == "sim" )
  {
    std::string rest = splits[ 1 ];
    for ( size_t i = 2; i < splits.size(); ++i )
      rest += '.' + splits[ i ];
    return sim -> create_expression( this, rest );
  }

  if ( name_str == "enabled" )
  {
    struct ok_expr_t : public action_expr_t
    {
      ok_expr_t( action_t& a ) : action_expr_t( "enabled", a ) {}
      virtual double evaluate() override { return action.data().found(); }
    };
    return new ok_expr_t( *this );
  }
  else if ( name_str == "executing" )
  {
    struct executing_expr_t : public action_expr_t
    {
      executing_expr_t( action_t& a ) : action_expr_t( "executing", a ) {}
      virtual double evaluate() override { return action.execute_event != 0; }
    };
    return new executing_expr_t( *this );
  }

  return player -> create_expression( this, name_str );
}

// action_t::ppm_proc_chance ================================================

double action_t::ppm_proc_chance( double PPM ) const
{
  if ( weapon )
  {
    return weapon -> proc_chance_on_swing( PPM );
  }
  else
  {
    timespan_t t = execute_time();
    // timespan_t time = channeled ? base_tick_time : base_execute_time;

    if ( t == timespan_t::zero() ) t = timespan_t::from_seconds( 1.5 ); // player -> base_gcd;

    return ( PPM * t.total_minutes() );
  }
}

// action_t::tick_time ======================================================

timespan_t action_t::tick_time( const action_state_t* state ) const
{
  timespan_t t = base_tick_time;
  if ( channeled || hasted_ticks )
  {
    t *= state -> haste;
  }
  return t;
}

// action_t::snapshot_internal ==============================================

void action_t::snapshot_internal( action_state_t* state, unsigned flags, dmg_e rt )
{
  assert( state );

  state -> result_type = rt;

  if ( flags & STATE_CRIT )
    state -> crit_chance = composite_crit_chance() * composite_crit_chance_multiplier();

  if ( flags & STATE_HASTE )
    state -> haste = composite_haste();

  if ( flags & STATE_AP )
    state -> attack_power = composite_attack_power() * player -> composite_attack_power_multiplier();

  if ( flags & STATE_SP )
    state -> spell_power = composite_spell_power() * player -> composite_spell_power_multiplier();

  if ( flags & STATE_VERSATILITY )
    state -> versatility = composite_versatility( state );

  if ( flags & STATE_MUL_DA )
    state -> da_multiplier = composite_da_multiplier( state );

  if ( flags & STATE_MUL_TA )
    state -> ta_multiplier = composite_ta_multiplier( state );

  if ( flags & STATE_MUL_PERSISTENT )
    state -> persistent_multiplier = composite_persistent_multiplier( state );

  if ( flags & STATE_MUL_PET )
    state -> pet_multiplier = player -> cast_pet() -> owner -> composite_player_pet_damage_multiplier( state );

  if ( flags & STATE_TGT_MUL_DA )
    state -> target_da_multiplier = composite_target_da_multiplier( state -> target );

  if ( flags & STATE_TGT_MUL_TA )
    state -> target_ta_multiplier = composite_target_ta_multiplier( state -> target );

  if ( flags & STATE_TGT_CRIT )
    state -> target_crit_chance = composite_target_crit_chance( state -> target ) * composite_crit_chance_multiplier();

  if ( flags & STATE_TGT_MITG_DA )
    state -> target_mitigation_da_multiplier = composite_target_mitigation( state -> target, get_school() );

  if ( flags & STATE_TGT_MITG_TA )
    state -> target_mitigation_ta_multiplier = composite_target_mitigation( state -> target, get_school() );

  if ( flags & STATE_TGT_ARMOR )
    state -> target_armor = target_armor( state -> target );
}

void action_t::consolidate_snapshot_flags()
{
  if ( tick_action    ) tick_action    -> consolidate_snapshot_flags();
  if ( execute_action ) execute_action -> consolidate_snapshot_flags();
  if ( impact_action  ) impact_action  -> consolidate_snapshot_flags();
  if ( tick_action    ) snapshot_flags |= tick_action    -> snapshot_flags;
  if ( execute_action ) snapshot_flags |= execute_action -> snapshot_flags;
  if ( impact_action  ) snapshot_flags |= impact_action  -> snapshot_flags;
}

timespan_t action_t::composite_dot_duration( const action_state_t* s ) const
{
  if ( channeled )
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  return dot_duration;
}

event_t* action_t::start_action_execute_event( timespan_t t, action_state_t* execute_event )
{
  return make_event<action_execute_event_t>( *sim, this, t, execute_event );
}

void action_t::do_schedule_travel( action_state_t* state, const timespan_t& time_ )
{
  if ( time_ <= timespan_t::zero() )
  {
    impact( state );
    action_state_t::release( state );
  }
  else
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s schedules travel (%.3f) for %s",
          player -> name(), time_.total_seconds(), name() );

    travel_events.push_back( make_event<travel_event_t>( *sim, this, state, time_ ) );
  }
}

void action_t::schedule_travel( action_state_t* s )
{
  if ( ! execute_state )
    execute_state = get_state();

  execute_state -> copy_state( s );

  time_to_travel = distance_targeting_travel_time( s ); // This is used for spells that don't use the typical player ---> main target travel time.
  if ( time_to_travel == timespan_t::zero() )
    time_to_travel = travel_time();

  do_schedule_travel( s, time_to_travel );

  if ( result_is_hit( s -> result ) )
  {
    hit_any_target = true;
    num_targets_hit++;
  }
}

void action_t::impact( action_state_t* s )
{
  if ( !impact_targeting( s ) )
    return;

  // Note, Critical damage bonus for direct amounts is computed on impact, instead of cast finish.
  s -> result_amount = calculate_crit_damage_bonus( s );

  assess_damage( ( type == ACTION_HEAL || type == ACTION_ABSORB ) ? HEAL_DIRECT : DMG_DIRECT, s );

  if ( result_is_hit( s -> result ) )
  {
    trigger_dot( s );

    if ( impact_action )
    {
      assert( ! impact_action -> pre_execute_state );

      // If the action has no travel time, we can reuse the snapshoted state,
      // otherwise let impact_action resnapshot modifiers
      if ( time_to_travel == timespan_t::zero() )
        impact_action -> pre_execute_state = impact_action -> get_state( s );

      assert( impact_action -> background );
      impact_action -> target = s -> target;
      impact_action -> execute();
    }
  }
  else
  {
    if ( sim -> log )
    {
      sim -> out_log.printf( "Target %s avoids %s %s (%s)", s -> target -> name(), player -> name(), name(), util::result_type_string( s -> result ) );
    }
  }
}

void action_t::trigger_dot( action_state_t* s )
{
  timespan_t duration = composite_dot_duration( s );
  if ( duration <= timespan_t::zero() && ! tick_zero )
    return;

  // To simulate precasting HoTs, remove one tick worth of duration if precombat.
  // We also add a fake zero_tick in dot_t::check_tick_zero().
  if ( ! harmful && ! player -> in_combat && ! tick_zero )
    duration -= tick_time( s );

  dot_t* dot = get_dot( s -> target );

  if ( dot_behavior == DOT_CLIP ) dot -> cancel();

  dot -> current_action = this;
  dot -> max_stack = dot_max_stack;

  if ( ! dot -> state ) dot -> state = get_state();
  dot -> state -> copy_state( s );

  if ( !dot -> is_ticking() )
  {
    if ( get_school() == SCHOOL_PHYSICAL && harmful )
    {
      buff_t* b = s -> target -> debuffs.bleeding;
      if ( b -> current_value > 0 )
      {
        b -> current_value += 1.0;
      }
      else b -> start( 1, 1.0 );
    }
  }

  dot -> trigger( duration );
}

/**
 * Determine if a travel event for given target currently exists.
 */
bool action_t::has_travel_events_for( const player_t* target ) const
{
  for ( const auto& travel_event : travel_events )
  {
    if ( travel_event -> state -> target == target )
      return true;
  }

  return false;
}

void action_t::remove_travel_event( travel_event_t* e )
{
  std::vector<travel_event_t*>::iterator pos = range::find( travel_events, e );
  if ( pos != travel_events.end() )
    erase_unordered( travel_events, pos );
}

void action_t::do_teleport( action_state_t* state )
{
  player -> teleport( composite_teleport_distance( state ) );
}

/**
 * Calculates the new dot length after a refresh
 * Necessary because we have both pandemic behaviour ( last 30% of the dot are preserved )
 * and old Cata/MoP behavior ( only time to the next tick is preserved )
 */
timespan_t action_t::calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const
{
  if ( ! channeled )
  {
    // WoD Pandemic
    // New WoD Formula: Get no malus during the last 30% of the dot.
    return std::min( triggered_duration * 0.3, dot -> remains() ) + triggered_duration;
  }
  else
  {
    return dot -> time_to_next_tick() + triggered_duration;
  }
}

bool action_t::dot_refreshable( const dot_t* dot, const timespan_t& triggered_duration ) const
{
  if ( ! channeled )
  {
    return dot -> remains() <= triggered_duration * 0.3;
  }
  else
  {
    return dot -> remains() <= dot -> time_to_next_tick();
  }
}

call_action_list_t::call_action_list_t( player_t* player, const std::string& options_str ) :
  action_t( ACTION_CALL, "call_action_list", player ), alist( 0 )
{
  std::string alist_name;
  int randomtoggle = 0;
  add_option( opt_string( "name", alist_name ) );
  add_option( opt_int( "random", randomtoggle ) );
  parse_options( options_str );

  ignore_false_positive = true; // Truly terrible things could happen if a false positive comes back on this.

  if ( alist_name.empty() )
  {
    sim -> errorf( "Player %s uses call_action_list without specifying the name of the action list\n", player -> name() );
    sim -> cancel();
  }

  alist = player -> find_action_priority_list( alist_name );

  if ( randomtoggle == 1 )
    alist -> random = randomtoggle;

  if ( ! alist )
  {
    sim -> errorf( "Player %s uses call_action_list with unknown action list %s\n", player -> name(), alist_name.c_str() );
    sim -> cancel();
  }
}

/**
 * If the action is still ticking and all resources could be successfully consumed,
 * return true to indicate continued ticking.
 */
bool action_t::consume_cost_per_tick( const dot_t& /* dot */ )
{
  if ( ! consume_per_tick_ )
  {
    return true;
  }

  if ( player -> get_active_dots( internal_id ) == 0 )
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s: %s ticking cost ends because dot is no longer ticking.", player -> name(), name() );
    return false;
  }

  // Consume resources
  /*
   * Assumption: If not enough resource is available, still consume as much as possible
   * and cancel action afterwards.
   * philoptik 2015-03-23
   */
  bool cancel_action = false;
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    double cost = cost_per_tick( r );
    if ( cost <= 0.0 )
      continue;

    bool enough_resource_available = player -> resource_available( r, cost );
    if ( ! enough_resource_available )
    {
      if ( sim -> log )
        sim -> out_log.printf( "%s: %s not enough resource for ticking cost %.1f %s for %s (%.0f). Going to cancel the action.",
                               player -> name(), name(),
                               cost, util::resource_type_string( r ),
                               name(), player -> resources.current[ r ] );
    }

    last_resource_cost = player -> resource_loss( r, cost, nullptr, this );
    stats -> consume_resource( r, last_resource_cost );

    if ( sim -> log )
      sim -> out_log.printf( "%s: %s consumes ticking cost %.1f (%.1f) %s for %s (%.0f).",
                             player -> name(),
                             name(),
                             cost, last_resource_cost, util::resource_type_string( r ),
                             name(), player -> resources.current[ r ] );

    if ( ! enough_resource_available )
    {
      cancel_action = true;
    }
  }

  if ( cancel_action )
  {
    cancel();
    range::for_each( target_list(), [ this ]( player_t* target ) {
      if ( dot_t* d = find_dot( target ) )
      {
        d -> cancel();
      }
    } );
    return false;
  }

  return true;
}

dot_t* action_t::get_dot( player_t* t )
{
  if ( ! t ) t = target;
  if ( ! t ) return nullptr;

  dot_t*& dot = target_specific_dot[ t ];
  if ( ! dot ) dot = t -> get_dot( name_str, player );
  return dot;
}

dot_t* action_t::find_dot( player_t* t ) const
{
  if ( ! t ) return nullptr;
  return target_specific_dot[ t ];
}

void action_t::add_child( action_t* child )
{
  child -> parent_dot = target -> get_dot( name_str, player );
  child_action.push_back( child );
  if ( child -> parent_dot && range > 0 && child -> radius > 0 && child -> is_aoe() )
  {
    // If the parent spell has a range, the tick_action has a radius and is an aoe spell, then the tick action likely
    // also has a range. This will allow distance_target_t to correctly determine spells that radiate from the target,
    // instead of the player.
    child->range = range;
  }
  stats -> add_child( child -> stats );
}

bool action_t::has_movement_directionality() const
{
  // If ability has no movement restrictions, it'll be usable
  if ( movement_directionality == MOVEMENT_NONE || movement_directionality == MOVEMENT_OMNI )
    return true;
  else
  {
    movement_direction_e m = player -> movement_direction();

    // If player isnt moving, allow everything
    if ( m == MOVEMENT_NONE )
      return true;
    else
      return m == movement_directionality;
  }
}

void action_t::reschedule_queue_event()
{
  if ( ! queue_event )
  {
    return;
  }

  timespan_t new_queue_delay = cooldown -> queue_delay();

  if ( new_queue_delay <= timespan_t::zero() )
  {
    return;
  }

  timespan_t remaining = queue_event -> remains();

  // The actual queue delay did not change, so no need to do anything
  if ( new_queue_delay == remaining )
  {
    return;
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s %s adjusting queue-delayed execution, old=%.3f new=%.3f",
        player -> name(), name(), remaining.total_seconds(), new_queue_delay.total_seconds() );
  }

  if ( new_queue_delay > remaining )
  {
    queue_event -> reschedule( new_queue_delay );
  }
  else
  {
    bool off_gcd = debug_cast<queued_action_execute_event_t*>( queue_event ) -> off_gcd;

    event_t::cancel( queue_event );
    queue_event = make_event<queued_action_execute_event_t>( *sim, this, new_queue_delay, off_gcd );
  }
}

/**
 * Acquire a new target, where the context is the actor that sources the retarget event, and the actor-level candidate
 * is given as a parameter (selected by player_t::acquire_target). Default target acquirement simply assigns the
 * actor-selected candidate target to the current target. Event contains the retarget event type, context contains the
 * (optional) actor that triggered the event.
 */
void action_t::acquire_target( retarget_event_e /* event */,
                               player_t*        /* context */,
                               player_t*        candidate_target )
{
  // Don't change targets if they are not of the same generic type (both enemies, or both friendlies)
  if ( target && target -> is_enemy() != candidate_target -> is_enemy() )
  {
    return;
  }

  // If the user has indicated a target number for the action, don't adjust targets
  if ( option.target_number > 0 )
  {
    return;
  }

  if ( target != candidate_target )
  {
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s %s target change, current=%s candidate=%s", player -> name(),
        name(), target ? target -> name() : "(none)", candidate_target -> name() );
    }
    target = candidate_target;
    target_cache.is_valid = false;
  }
}

void action_t::activate()
{
  sim -> target_non_sleeping_list.register_callback( [ this ]( player_t* ) {
    target_cache.is_valid = false;
  } );
}

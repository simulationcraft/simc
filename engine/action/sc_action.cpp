// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Action
// ==========================================================================

namespace { // anonymous namespace

struct player_gcd_event_t : public event_t
{
  player_gcd_event_t( player_t& p, timespan_t delta_time ) :
      event_t( p, "Player-Ready-GCD" )
  {
    if ( sim().debug )
      sim().out_debug << "New Player-Ready-GCD Event: " << p.name();

    sim().add_event( this, delta_time );
  }

  virtual void execute()
  {
    for ( std::vector<action_t*>::const_iterator i = p() -> active_off_gcd_list -> off_gcd_actions.begin();
          i < p() -> active_off_gcd_list -> off_gcd_actions.end(); ++i )
    {
      action_t* a = *i;
      if ( a -> ready() )
      {
        action_priority_list_t* alist = p() -> active_action_list;

        a -> execute();
        a -> line_cooldown.start();
        if ( ! a -> quiet )
        {
          p() -> iteration_executed_foreground_actions++;
          a -> total_executions++;

          p() -> sequence_add( a, a -> target, sim().current_time );
        }

        // Need to restart because the active action list changed
        if ( alist != p() -> active_action_list )
        {
          p() -> activate_action_list( p() -> active_action_list, true );
          execute();
          p() -> activate_action_list( alist, true );
          return;
        }
      }
    }

    if ( p() -> restore_action_list != 0 )
    {
      p() -> activate_action_list( p() -> restore_action_list );
      p() -> restore_action_list = 0;
    }

    p() -> off_gcd = new ( sim() ) player_gcd_event_t( *p(), timespan_t::from_seconds( 0.1 ) );
  }
};
// Action Execute Event =====================================================

struct action_execute_event_t : public event_t
{
  action_t* action;
  action_state_t* execute_state;

  action_execute_event_t( action_t* a, timespan_t time_to_execute,
                          action_state_t* state = 0 ) :
      event_t( *a->player, "Action-Execute" ), action( a ),
        execute_state( state )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New Action Execute Event: %s %s %.1f (target=%s, marker=%c)",
                  p() -> name(), a -> name(), time_to_execute.total_seconds(),
                  ( state ) ? state -> target -> name() : a -> target -> name(), 
                  ( a -> marker ) ? a -> marker : '0' );

    sim().add_event( this, time_to_execute );
  }

  // Ensure we properly release the carried execute_state even if this event
  // is never executed.
  ~action_execute_event_t()
  {
    if ( execute_state )
      action_state_t::release( execute_state );
  }

  // action_execute_event_t::execute ========================================

  virtual void execute()
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

    if ( ! target -> is_sleeping() )
      action -> execute();

    assert( ! action -> pre_execute_state );

    if ( action -> background ) return;

    if ( ! p() -> channeling )
    {
      if ( p() -> readying )
        sim().out_error.printf( "Danger Will Robinson!  Danger!  action %s player %s\n",
                    action -> name(), p() -> name() );

      p() -> schedule_ready( timespan_t::zero() );
    }

    if ( p() -> active_off_gcd_list == 0 )
      return;

    // Kick off the during-gcd checker, first run is immediately after
    core_event_t::cancel( p() -> off_gcd );

    if ( ! p() -> channeling )
      p() -> off_gcd = new ( sim() ) player_gcd_event_t( *p(), timespan_t::zero() );
  }

};

struct aoe_target_list_callback_t
{
  action_t* action;
  aoe_target_list_callback_t( action_t* a ) :
    action( a ) {}

  void operator()()
  {
    // Invalidate target cache
    action -> target_cache.is_valid = false;
  }
};

} // end anonymous namespace


// action_priority_list_t::add_action =======================================

// Anything goes to action priority list.
action_priority_t* action_priority_list_t::add_action( const std::string& action_priority_str,
                                                       const std::string& comment )
{
  if ( action_priority_str.empty() ) return 0;
  action_list.push_back( action_priority_t( action_priority_str, comment ) );
  return &( action_list.back() );
}

// Check the validity of spell data before anything goes to action priority list
action_priority_t* action_priority_list_t::add_action( const player_t* p,
                                                       const spell_data_t* s,
                                                       const std::string& action_name,
                                                       const std::string& action_options,
                                                       const std::string& comment )
{
  if ( ! s || ! s -> ok() || ! s -> is_level( p -> level ) ) return 0;

  std::string str = action_name;
  if ( ! action_options.empty() ) str += "," + action_options;

  return add_action( str, comment );
}

// Check the availability of a class spell of "name" and the validity of it's
// spell data before anything goes to action priority list
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

// action_priority_list_t::add_talent =======================================

// Check the availability of a talent spell of "name" and the validity of it's
// spell data before anything goes to action priority list. Note that this will
// ignore the actual talent check so we can make action list entries for
// talents, even though the profile does not have a talent.
//
// In addition, the method automatically checks for the presence of an if
// expression that includes the "talent guard", i.e., "talent.X.enabled" in it.
// If omitted, it will be automatically added to the if expression (or
// if expression will be created if it is missing).
action_priority_t* action_priority_list_t::add_talent( const player_t* p,
                                                       const std::string& name,
                                                       const std::string& action_options,
                                                       const std::string& comment )
{
  const spell_data_t* s = p -> find_talent_spell( name, "", SPEC_NONE, false, false );
  std::string talent_check_str = "talent." + dbc::get_token( s -> id() ) + ".enabled";
  bool found_if = false;
  bool found_talent_check = false;

  if ( util::str_in_str_ci( action_options, "if=" ) )
    found_if = true;

  if ( util::str_in_str_ci( action_options, talent_check_str ) )
    found_talent_check = true;

  std::string opts;
  if ( ! found_if && ! found_talent_check )
    opts = "if=" + talent_check_str;
  else if ( found_if && ! found_talent_check )
  {
    // Naively parenthesis the existing expressions if there are logical
    // operators in it
    bool has_logical_operators = util::str_in_str_ci( action_options, "&" ) ||
                                 util::str_in_str_ci( action_options, "|" );

    size_t if_pos = action_options.find( "if=" ) + 3;
    opts += action_options.substr( 0, if_pos ) + talent_check_str + "&";
    if ( has_logical_operators ) opts += "(";
    opts += action_options.substr( if_pos );
    if ( has_logical_operators ) opts += ")";
  }
  else
    opts = action_options;

  return add_action( p, s, dbc::get_token( s -> id() ), opts, comment );
}

// action_t::action_t =======================================================

action_t::action_t( action_e       ty,
                    const std::string&  token,
                    player_t*           p,
                    const spell_data_t* s ) :
  s_data( s ? s : spell_data_t::nil() ),
  sim( p -> sim ),
  type( ty ),
  name_str( token ),
  player( p ),
  target( p -> target ),
  target_cache(),
  school( SCHOOL_NONE ),
  id(),
  resource_current( RESOURCE_NONE ),
  aoe(),
  pre_combat( 0 ),
  may_multistrike( -1 ),
  dual(),
  callbacks( true ),
  special(),
  channeled(),
  background(),
  sequence(),
  use_off_gcd(),
  quiet(),
  direct_tick(),
  direct_tick_callbacks(),
  periodic_hit(),
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
  may_crush(),
  may_crit(),
  tick_may_crit(),
  tick_zero(),
  hasted_ticks(),
  dot_behavior( DOT_CLIP ),
  ability_lag( timespan_t::zero() ),
  ability_lag_stddev( timespan_t::zero() ),
  rp_gain(),
  min_gcd( timespan_t() ),
  trigger_gcd( player -> base_gcd ),
  range(),
  weapon_power_mod(),
  attack_power_mod(),
  spell_power_mod(),
  base_execute_time( timespan_t::zero() ),
  base_tick_time( timespan_t::zero() ),
  dot_duration( timespan_t::zero() ),
  time_to_execute( timespan_t::zero() ),
  time_to_travel( timespan_t::zero() ),
  target_specific_dot( false ),
  total_executions(),
  line_cooldown( cooldown_t( "line_cd", *p ) )
{
  dot_behavior                   = DOT_CLIP;
  trigger_gcd                    = player -> base_gcd;
  range                          = -1.0;
  weapon_power_mod               = 1.0 / 3.5;


  base_dd_min                    = 0.0;
  base_dd_max                    = 0.0;
  base_td                        = 0.0;
  base_td_multiplier             = 1.0;
  base_dd_multiplier             = 1.0;
  base_multiplier                = 1.0;
  base_hit                       = 0.0;
  base_crit                      = 0.0;
  rp_gain                        = 0.0;
  base_spell_power               = 0.0;
  base_attack_power              = 0.0;
  crit_multiplier                = 1.0;
  crit_bonus_multiplier          = 1.0;
  base_dd_adder                  = 0.0;
  base_ta_adder                  = 0.0;
  weapon                         = NULL;
  weapon_multiplier              = 1.0;
  base_add_multiplier            = 1.0;
  base_aoe_multiplier            = 1.0;
  split_aoe_damage               = false;
  normalize_weapon_speed         = false;
  stats                          = NULL;
  execute_event                  = 0;
  travel_speed                   = 0.0;
  resource_consumed              = 0.0;
  moving                         = -1;
  wait_on_ready                  = -1;
  interrupt                      = 0;
  chain                          = 0;
  cycle_targets                  = 0;
  max_cycle_targets              = 0;
  target_number                  = 0;
  round_base_dmg                 = true;
  if_expr_str.clear();
  if_expr                        = NULL;
  interrupt_if_expr_str.clear();
  interrupt_if_expr              = NULL;
  early_chain_if_expr_str.clear();
  early_chain_if_expr            = NULL;
  sync_str.clear();
  sync_action                    = NULL;
  marker                         = 0;
  last_reaction_time             = timespan_t::zero();
  tick_action                    = 0;
  execute_action                 = 0;
  impact_action                  = 0;
  dynamic_tick_action            = false;
  special_proc                   = false;
  // New Stuff
  snapshot_flags = 0;
  update_flags = STATE_TGT_MUL_DA | STATE_TGT_MUL_TA | STATE_TGT_CRIT;
  execute_state = 0;
  pre_execute_state = 0;
  action_list = "";
  movement_directionality = MOVEMENT_NONE;
  base_teleport_distance = 0;

  range::fill( base_costs, 0.0 );
  range::fill( costs_per_second, 0 );
  
  assert( ! name_str.empty() && "Abilities must have valid name_str entries!!" );

  util::tokenize( name_str );

  if ( sim -> debug )
    sim -> out_debug.printf( "Player %s creates action %s (%d)", player -> name(), name(), ( s_data -> ok() ? s_data -> id() : -1 ) );

  if ( ! player -> initialized )
  {
    sim -> errorf( "Actions must not be created before player_t::init().  Culprit: %s %s\n", player -> name(), name() );
    sim -> cancel();
  }

  player -> action_list.push_back( this );

  cooldown = player -> get_cooldown( name_str );

  stats = player -> get_stats( name_str , this );

  if ( data().ok() )
  {
    parse_spell_data( data() );
  }

  if ( data().id() && ! data().is_level( player -> level ) && data().level() <= MAX_LEVEL )
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required level (%d < %d).\n",
                   player -> name(), name(), player -> level, data().level() );

    background = true; // prevent action from being executed
  }

  std::vector<specialization_e> spec_list;
  specialization_e _s = player -> specialization();
  if ( data().id() && player -> dbc.ability_specialization( data().id(), spec_list ) && range::find( spec_list, _s ) == spec_list.end() )
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required spec.\n",
                   player -> name(), name() );

    background = true; // prevent action from being executed
  }

  if ( s_data == spell_data_t::not_found() )
  {
    // this is super-spammy, may just want to disable this after we're sure this section is working as intended.
    if ( sim -> debug )
      sim -> errorf( "Player %s attempting to use action %s without the required talent, spec, class, or race; ignoring.\n", 
                   player -> name(), name() );
    background = true;
  }

  spec_list.clear();
}

action_t::~action_t()
{
  delete execute_state;
  delete pre_execute_state;
  delete if_expr;
  delete interrupt_if_expr;
  delete early_chain_if_expr;
}

// action_t::parse_data =====================================================

void action_t::parse_spell_data( const spell_data_t& spell_data )
{
  if ( ! spell_data.id() ) // FIXME: Replace/augment with ok() check once it is in there
  {
    sim -> errorf( "%s %s: parse_spell_data: no spell to parse.\n", player -> name(), name() );
    return;
  }

  id                   = spell_data.id();
  base_execute_time    = spell_data.cast_time( player -> level );
  cooldown -> duration = spell_data.cooldown();
  range                = spell_data.max_range();
  travel_speed         = spell_data.missile_speed();
  trigger_gcd          = spell_data.gcd();
  school               = spell_data.get_school_type();
  rp_gain              = spell_data.runic_power_gain();

  if ( spell_data._power && spell_data._power -> size() == 1 )
    resource_current = spell_data._power -> at( 0 ) -> resource();

  for ( size_t i = 0; spell_data._power && i < spell_data._power -> size(); i++ )
  {
    const spellpower_data_t* pd = ( *spell_data._power )[ i ];

    if ( pd -> _cost > 0 )
      base_costs[ pd -> resource() ] = pd -> cost();
    else
      base_costs[ pd -> resource() ] = floor( pd -> cost() * player -> resources.base[ pd -> resource() ] );

    if ( pd -> _cost_per_second > 0 )
      costs_per_second[ pd -> resource() ] = ( int ) pd -> cost_per_second();
    else
      costs_per_second[ pd -> resource() ] = ( int ) floor( pd -> cost_per_second() * player -> resources.base[ pd -> resource() ] );
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

  switch ( spelleffect_data.type() )
  {
      // Direct Damage
    case E_HEAL:
    case E_SCHOOL_DAMAGE:
    case E_HEALTH_LEECH:
      spell_power_mod.direct = spelleffect_data.sp_coeff();
      attack_power_mod.direct = spelleffect_data.ap_coeff(); // change to ap_coeff
      base_dd_min      = player -> dbc.effect_min( spelleffect_data.id(), player -> level );
      base_dd_max      = player -> dbc.effect_max( spelleffect_data.id(), player -> level );
      break;

    case E_NORMALIZED_WEAPON_DMG:
      normalize_weapon_speed = true;
    case E_WEAPON_DAMAGE:
      base_dd_min      = player -> dbc.effect_min( spelleffect_data.id(), player -> level );
      base_dd_max      = player -> dbc.effect_max( spelleffect_data.id(), player -> level );
      weapon = &( player -> main_hand_weapon );
      break;

    case E_WEAPON_PERCENT_DAMAGE:
      weapon = &( player -> main_hand_weapon );
      weapon_multiplier = player -> dbc.effect_min( spelleffect_data.id(), player -> level );
      break;

      // Dot
    case E_PERSISTENT_AREA_AURA:
    case E_APPLY_AURA:
      switch ( spelleffect_data.subtype() )
      {
        case A_PERIODIC_DAMAGE:
        case A_PERIODIC_LEECH:
        case A_PERIODIC_HEAL:
          spell_power_mod.tick = spelleffect_data.sp_coeff();
          attack_power_mod.tick = spelleffect_data.ap_coeff(); // change to ap_coeff
          base_td          = player -> dbc.effect_average( spelleffect_data.id(), player -> level );
        case A_PERIODIC_ENERGIZE:
        case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
        case A_PERIODIC_HEALTH_FUNNEL:
        case A_PERIODIC_MANA_LEECH:
        case A_PERIODIC_DAMAGE_PERCENT:
        case A_PERIODIC_DUMMY:
        case A_PERIODIC_TRIGGER_SPELL:
          if ( spelleffect_data.period() > timespan_t::zero() )
          {
            base_tick_time   = spelleffect_data.period();
            dot_duration = spelleffect_data.spell() -> duration();
          }
          break;
        case A_SCHOOL_ABSORB:
          spell_power_mod.direct = spelleffect_data.sp_coeff();
          attack_power_mod.direct = spelleffect_data.ap_coeff(); // change to ap_coeff
          base_dd_min      = player -> dbc.effect_min( spelleffect_data.id(), player -> level );
          base_dd_max      = player -> dbc.effect_max( spelleffect_data.id(), player -> level );
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
        case A_ADD_PCT_MODIFIER:
          switch ( spelleffect_data.misc_value1() )
          {
            case P_RESOURCE_COST:
              base_costs[ player -> primary_resource() ] *= 1 + 0.01 * spelleffect_data.base_value();
              break;
          }
          break;
        default: break;
      }
      break;
    default: break;
  }
}

// action_t::parse_options ==================================================

void action_t::parse_options( option_t*          options,
                              const std::string& options_str )
{
  option_t base_options[] =
  {
    // deprecated options: Point users to the correct ones
    opt_deprecated( "bloodlust", "if=buff.bloodlust.react" ),
    opt_deprecated( "haste<", "if=spell_haste>= or if=attack_haste>=" ),
    opt_deprecated( "health_percentage<", "if=target.health.pct<=" ),
    opt_deprecated( "health_percentage>", "if=target.health.pct>=" ),
    opt_deprecated( "invulnerable", "if=target.debuff.invulnerable.react" ),
    opt_deprecated( "not_flying", "if=target.debuff.flying.down" ),
    opt_deprecated( "flying", "if=target.debuff.flying.react" ),
    opt_deprecated( "time<", "if=time<=" ),
    opt_deprecated( "time>", "if=time>=" ),
    opt_deprecated( "travel_speed", "if=travel_speed" ),
    opt_deprecated( "vulnerable", "if=target.debuff.vulnerable.react" ),
    opt_deprecated( "use_off_gcd", "" ),

    opt_string( "if", if_expr_str ),
    opt_string( "interrupt_if", interrupt_if_expr_str ),
    opt_string( "early_chain_if", early_chain_if_expr_str ),
    opt_bool( "interrupt", interrupt ),
    opt_bool( "chain", chain ),
    opt_bool( "cycle_targets", cycle_targets ),
    opt_int( "max_cycle_targets", max_cycle_targets ),
    opt_bool( "moving", moving ),
    opt_string( "sync", sync_str ),
    opt_bool( "wait_on_ready", wait_on_ready ),
    opt_string( "target", target_str ),
    opt_string( "label", label_str ),
    opt_bool( "precombat", pre_combat ),
    opt_timespan( "line_cd", line_cooldown.duration ),
    opt_null()
  };

  std::vector<option_t> merged_options;
  option_t::merge( merged_options, options, base_options );

  if ( ! option_t::parse( sim, name(), merged_options, options_str ) )
  {
    sim -> errorf( "%s %s: Unable to parse options str '%s'.\n", player -> name(), name(), options_str.c_str() );
    sim -> cancel();
  }

  // FIXME: Move into constructor when parse_action is called from there.
  if ( ! target_str.empty() )
  {
    if ( target_str[ 0 ] >= '0' && target_str[ 0 ] <= '9' )
    {
      target_number = atoi( target_str.c_str() );
      player_t* p = find_target_by_number( target_number );
      // Numerical targeting is intended to be dynamic, so don't give an error message if we can't find the target yet
      if ( p ) target = p;
    }
    else
    {
      player_t* p = sim -> find_player( target_str );

      if ( p )
        target = p;
      else
        sim -> errorf( "%s %s: Unable to locate target '%s'.\n", player -> name(), name(), options_str.c_str() );
    }
  }
}

// action_t::cost ===========================================================

double action_t::cost() const
{
  if ( ! harmful && ! player -> in_combat )
    return 0;

  resource_e cr = current_resource();

  double c = base_costs[ cr ];

  c -= player -> current.resource_reduction[ get_school() ];

  if ( cr == RESOURCE_MANA && player -> buffs.courageous_primal_diamond_lucidity -> check() )
    c = 0;

  if ( c < 0 ) c = 0;

  if ( sim -> debug )
    sim -> out_debug.printf( "action_t::cost: %s %.2f %.2f %s", name(), base_costs[ cr ], c, util::resource_type_string( cr ) );

  return floor( c );
}

// action_t::gcd ============================================================

timespan_t action_t::gcd() const
{
  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero();

  return trigger_gcd;
}

// action_t::travel_time ====================================================

timespan_t action_t::travel_time() const
{
  if ( travel_speed == 0 ) return timespan_t::zero();

  double distance = player -> current.distance;
  if ( execute_state && execute_state -> target )
    distance += execute_state -> target -> size;

  if ( distance == 0 ) return timespan_t::zero();

  double t = distance / travel_speed;

  double v = sim -> travel_variance;

  if ( v )
  {
    t = rng().gauss( t, v );
  }

  return timespan_t::from_seconds( t );
}

// action_t::total_crit_bonus ===============================================

double action_t::total_crit_bonus() const
{
  double crit_multiplier_buffed = crit_multiplier * composite_player_critical_multiplier();
  double bonus = ( ( 1.0 + crit_bonus ) * crit_multiplier_buffed - 1.0 ) * crit_bonus_multiplier;

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s crit_bonus for %s: cb=%.3f b_cb=%.2f b_cm=%.2f b_cbm=%.2f",
                   player -> name(), name(), bonus, crit_bonus, crit_multiplier_buffed, crit_bonus_multiplier );
  }

  return bonus;
}

// action_t::calculate_weapon_damage ========================================

double action_t::calculate_weapon_damage( double attack_power )
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

double action_t::calculate_tick_amount( action_state_t* state, double dot_multiplier )
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

  if ( state -> result == RESULT_CRIT )
    amount *= 1.0 + total_crit_bonus();
  else if ( state -> result == RESULT_MULTISTRIKE )
    amount *= composite_multistrike_multiplier( state );
  else if ( state -> result == RESULT_MULTISTRIKE_CRIT )
    amount *= composite_multistrike_multiplier( state ) * ( 1.0 + total_crit_bonus() );

  amount *= dot_multiplier;

  // Record total amount to state
  state -> result_total = amount;

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s amount for %s on %s: ta=%.0f i_ta=%.0f b_ta=%.0f bonus_ta=%.0f s_mod=%.2f s_power=%.0f a_mod=%.2f a_power=%.0f mult=%.2f, tick_mult=%.2f",
                   player -> name(), name(), target -> name(), amount,
                   init_tick_amount, base_ta( state ), bonus_ta( state ),
                   spell_tick_power_coefficient( state ), state -> composite_spell_power(),
                   attack_tick_power_coefficient( state ), state -> composite_attack_power(),
                   state -> composite_ta_multiplier(),
                   dot_multiplier );
  }

  return amount;
}

// action_t::calculate_direct_amount ========================================

double action_t::calculate_direct_amount( action_state_t* state )
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

  // AoE with decay per target
  if ( state -> chain_target > 0 && base_add_multiplier != 1.0 )
    amount *= pow( base_add_multiplier, state -> chain_target );

  // AoE with static reduced damage per target
  if ( state -> chain_target > 0 && base_aoe_multiplier != 1.0 )
    amount *= base_aoe_multiplier;

  // Spell splits damage across all targets equally
  if ( state -> action -> split_aoe_damage )
    amount /= state -> n_targets;

  // Spell goes over the maximum number of AOE targets
  if ( ! state -> action -> split_aoe_damage && state -> n_targets > static_cast< size_t >( sim -> max_aoe_enemies ) )
    amount *= sim -> max_aoe_enemies / static_cast< double >( state -> n_targets );

  // Record initial amount to state
  state -> result_raw = amount;

  if ( state -> result == RESULT_GLANCE )
  {
    double delta_skill = ( state -> target -> level - player -> level ) * 5.0;

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
  else if ( state -> result == RESULT_CRIT )
  {
    amount *= 1.0 + total_crit_bonus();
  }
  else if ( state -> result == RESULT_MULTISTRIKE )
  {
    amount *= composite_multistrike_multiplier( state );
  }
  else if ( state -> result == RESULT_MULTISTRIKE_CRIT )
  {
    amount *= composite_multistrike_multiplier( state ) * ( 1.0 + total_crit_bonus() );
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

// action_t::consume_resource ===============================================

void action_t::consume_resource()
{
  resource_e cr = current_resource();

  if ( cr == RESOURCE_NONE || base_costs[ cr ] == 0 || proc ) return;

  resource_consumed = cost();

  player -> resource_loss( cr, resource_consumed, 0, this );

  if ( sim -> log )
    sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                   resource_consumed, util::resource_type_string( cr ),
                   name(), player -> resources.current[ cr ] );

  stats -> consume_resource( current_resource(), resource_consumed );
}

// action_t::available_targets ==============================================

int action_t::num_targets()
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
  tl.push_back( target );

  for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
  {
    player_t* t = sim -> target_non_sleeping_list[ i ];

    if ( t -> is_enemy() && ( t != target ) )
      tl.push_back( t );
  }

  return tl.size();
}

// action_t::target_list ====================================================

std::vector< player_t* >& action_t::target_list() const
{
  // Check if target cache is still valid. If not, recalculate it
  if ( ! target_cache.is_valid )
  {
    available_targets( target_cache.list );
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

block_result_e action_t::calculate_block_result( action_state_t* s )
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

  if ( sim -> log && ! dual )
  {
    sim -> out_log.printf( "%s performs %s (%.0f)", player -> name(), name(),
                   player -> resources.current[ player -> primary_resource() ] );
  }

  if ( n_targets() == 0 && target -> is_sleeping() )
    return;

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
    num_targets = ( n_targets() < 0 ) ? tl.size() : n_targets();
    for ( size_t t = 0, max_targets = tl.size(); t < num_targets && t < max_targets; t++ )
    {
      action_state_t* s = get_state( pre_execute_state );
      s -> target = tl[ t ];
      s -> n_targets = std::min( num_targets, tl.size() );
      s -> chain_target = as<int>( t );
      if ( ! pre_execute_state ) snapshot_state( s, type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT );
      s -> result = calculate_result( s );
      s -> block_result = calculate_block_result( s );

      s -> result_amount = calculate_direct_amount( s );


      if ( sim -> debug )
        s -> debug();

      schedule_travel( s );

      schedule_multistrike( execute_state, type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT );
    }
  }
  else // single target
  {
    num_targets = 1;

    action_state_t* s = get_state( pre_execute_state );
    s -> target = target;
    s -> n_targets = 1;
    s -> chain_target = 0;
    if ( ! pre_execute_state ) snapshot_state( s, type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT );
    s -> result = calculate_result( s );
    s -> block_result = calculate_block_result( s );

    s -> result_amount = calculate_direct_amount( s );

    if ( sim -> debug )
      s -> debug();

    schedule_travel( s );

    schedule_multistrike( execute_state, type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT );
  }

  consume_resource();

  update_ready();

  if ( ! dual ) stats -> add_execute( time_to_execute, target );

  if ( pre_execute_state )
    action_state_t::release( pre_execute_state );

  // The rest of the execution depends on actually executing on target
  if ( num_targets > 0 )
  {
    if ( composite_teleport_distance( execute_state ) > 0 )
      do_teleport( execute_state );

    if ( execute_action && result_is_hit( execute_state -> result ) )
    {
      assert( ! execute_action -> pre_execute_state );
      execute_action -> schedule_execute( execute_action -> get_state( execute_state ) );
    }

    // New callback system; proc abilities on execute. 
    // Note: direct_tick_callbacks should not be used with the new system, 
    // override action_t::proc_type() instead
    if ( callbacks )
    {
      proc_types pt = execute_state -> proc_type();
      proc_types2 pt2 = execute_state -> execute_proc_type2();

      // "On spell cast"
      if ( ! background && pt != PROC1_INVALID )
        action_callback_t::trigger( player -> callbacks.procs[ pt ][ PROC2_CAST ], this, execute_state );

      // "On an execute result"
      if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
        action_callback_t::trigger( player -> callbacks.procs[ pt ][ pt2 ], this, execute_state );
    }
  }

  if ( repeating && ! proc ) schedule_execute();
}

// action_t::calculate_multistrike_result ===================================

result_e action_t::calculate_multistrike_result( action_state_t* s )
{
  if ( ! s -> target ) return RESULT_NONE;
  if ( ! may_multistrike ) return RESULT_NONE;
  //if ( ! harmful ) return RESULT_NONE; See if this screws anything up. 

  result_e r = RESULT_NONE;
  if ( rng().roll( composite_multistrike() ) )
  {
    r = RESULT_MULTISTRIKE;

    if ( rng().roll( std::max( s -> composite_crit(), 0.0 ) ) )
      r = RESULT_MULTISTRIKE_CRIT;
  }

  return r;
}

// action_t::schedule_multistrike ===========================================

// Schedule multistrikes off of an ability execute; return the number of
// multistrikes generated by the execution (per target). The multistrike state
// will be based off of the passed state, with independent result, and damage
// resolution.
int action_t::schedule_multistrike( action_state_t* state, dmg_e type, double tick_multiplier )
{
  if ( ! may_multistrike )
    return 0;

  if ( state -> result_amount <= 0 )
    return 0;

  if ( ! result_is_hit( state -> result ) )
    return 0;

  // multistrike can proc twice
  int n_strikes = 0;
  for ( int i = 0; i < 2; i++ )
  {
    result_e r = calculate_multistrike_result( state );
    if ( ! result_is_multistrike( r ) )
      continue;

    action_state_t* ms_state = get_state( state );
    ms_state -> target = state -> target;
    ms_state -> n_targets = 1;
    ms_state -> chain_target = 0;
    ms_state -> result = r;

    if ( type == DMG_DIRECT || type == HEAL_DIRECT )
      multistrike_direct( state, ms_state );
    else if ( type == DMG_OVER_TIME || type == HEAL_OVER_TIME )
      multistrike_tick( state, ms_state, tick_multiplier );

    // Schedule multistrike "execute"; in reality it calls either impact, or
    // assess_damage (for ticks).
    new ( *sim ) multistrike_execute_event_t( ms_state );

    n_strikes++;
  }

  return n_strikes;
}

// action_t::multistrike_direct =============================================

// Generate a direct damage multistrike. Note that the multistrike damage will
// travel like normal damage events, based off of the ability travel time.
void action_t::multistrike_direct( const action_state_t*, action_state_t* ms_state )
{
  ms_state -> result_amount = calculate_direct_amount( ms_state );
}

// action_t::tick ===========================================================

void action_t::tick( dot_t* d )
{
  if ( tick_action )
  {
    if ( tick_action -> pre_execute_state )
    {
      action_state_t::release( tick_action -> pre_execute_state );
    }
    action_state_t* state = tick_action -> get_state( d -> state );
    if ( dynamic_tick_action )
      snapshot_state( state, type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );
    else
      update_state( state, type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );
    state -> da_multiplier = state -> ta_multiplier;
    state -> target_da_multiplier = state -> target_ta_multiplier;
    tick_action -> schedule_execute( state );
  }
  else
  {
    d -> state -> result = RESULT_HIT;
    update_state( d -> state, type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

    if ( tick_may_crit && rng().roll( d -> state -> composite_crit() ) )
      d -> state -> result = RESULT_CRIT;

    d -> state -> result_amount = calculate_tick_amount( d -> state, d -> get_last_tick_factor() );

    assess_damage( type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME, d -> state );

    if ( sim -> debug )
      d -> state -> debug();

    schedule_multistrike( d -> state, type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );
  }

  if ( harmful && callbacks && type != ACTION_HEAL )
    action_callback_t::trigger( player -> callbacks.tick[ d -> state -> result ], this, d -> state );

  if ( ! periodic_hit ) stats -> add_tick( d -> time_to_tick, d -> state -> target );

  player -> trigger_ready();
}

// action_t::multistrike_tick ===============================================

// Generate a periodic multistrike
void action_t::multistrike_tick( const action_state_t*, action_state_t* ms_state, double tick_multiplier )
{
  ms_state -> result_amount = calculate_tick_amount( ms_state, tick_multiplier );
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
}

// action_t::update_resolve ======================================================

void action_t::update_resolve( dmg_e type,
                                 action_state_t* s )
{
  // pointers to make life easy
  player_t* source = s -> action -> player;
  player_t* target = s -> target;

  // check that the target has Resolve, check for damage type, and check that the source player is an enemy
  if ( target -> resolve_manager.is_started() && ( type == DMG_DIRECT || type == DMG_OVER_TIME ) && source -> is_enemy() )
  {
    assert( source -> actor_spawn_index >= 0 && "Trying to register resolve damage event from a unspawned player! Something is seriously broken in player_t::arise/demise." );

    // bool for auto attack, to make code easier to read
    bool is_auto_attack = ( player -> main_hand_attack && s -> action == player -> main_hand_attack ) || ( player -> off_hand_attack && s -> action == player -> off_hand_attack );

    // Resolve is only updated on damage taken events. The one exception is auto-attacks, which grant Resolve even on a dodge/parry.
    // If this is a miss that isn't an auto-attack, we can bail out early (and not recalculate)
    if ( result_is_miss( s -> result ) && ! is_auto_attack )
      return;

    // Store raw damage of attack result
    double raw_resolve_amount = s -> result_raw;
    
    // If the attack does zero damage, it's irrelevant for the purposes
    // Skip updating the Resolve tables if the damage is zero to limit unnecessary events
    if ( raw_resolve_amount > 0.0 )
    {
      // modify according to damage type; spell damage gives 2.5x as much Resolve
      raw_resolve_amount *= ( get_school() == SCHOOL_PHYSICAL ? 1.0 : 2.5 );

      // normalize by player's current health, ignoring any temporary health buffs
      raw_resolve_amount /= ( target -> resources.max[ RESOURCE_HEALTH ] - target -> resources.temporary[ RESOURCE_HEALTH ] );

      // update the player's resolve diminishing return list first!
      target -> resolve_manager.add_diminishing_return_entry( source, source -> get_raw_dps( s ), sim -> current_time );

      assert( source -> actor_spawn_index >= 0 && "Trying to register resolve damage event from a unspawned player!" );

      // get diminishing returns - done at the time of the event, never recalculated
      // see http://us.battle.net/wow/en/forum/topic/13087818929?page=6#105
      int rank = target -> resolve_manager.get_diminsihing_return_rank( source -> actor_spawn_index );

      // update the player's resolve damage table 
      target -> resolve_manager.add_damage_event( raw_resolve_amount / rank, sim -> current_time );
    
      // cycle through the resolve damage table and add the appropriate amount of Resolve from each event
      target -> resolve_manager.update();
    }


  } // END Resolve
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( dmg_e    type,
                              action_state_t* s )
{
  // hook up resolve here, before armor mitigation, avoidance, and dmg reduction effects, etc.
  update_resolve( type, s );

  s -> target -> assess_damage( get_school(), type, s );

  if ( sim -> debug )
    s -> debug();

  if ( type == DMG_DIRECT )
  {
    if ( sim -> log )
    {
      sim -> out_log.printf( "%s %s hits %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     s -> target -> name(), s -> result_amount,
                     util::school_type_string( get_school() ),
                     util::result_type_string( s -> result ) );
    }

    if ( s -> result_amount > 0.0 && ! result_is_multistrike( s -> result ) )
    {
      if ( direct_tick_callbacks )
      {
        action_callback_t::trigger( player -> callbacks.tick_damage[ get_school() ], this, s );
      }
      else
      {
        if ( callbacks )
        {
          action_callback_t::trigger( player -> callbacks.direct_damage[ get_school() ], this, s );
          if ( s -> result == RESULT_CRIT )
          {
            action_callback_t::trigger( player -> callbacks.direct_crit[ get_school() ], this, s );
          }
        }
      }
    }
  }
  else // DMG_OVER_TIME
  {
    if ( sim -> log )
    {
      dot_t* dot = get_dot( s -> target );
      sim -> out_log.printf( "%s %s ticks (%d of %d) %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     dot -> current_tick, dot -> num_ticks,
                     s -> target -> name(), s -> result_amount,
                     util::school_type_string( get_school() ),
                     util::result_type_string( s -> result ) );
    }

    if ( ! result_is_multistrike( s -> result ) && callbacks && s -> result_amount > 0.0 )
      action_callback_t::trigger( player -> callbacks.tick_damage[ get_school() ], this, s );
  }

  // New callback system; proc spells on impact. 
  // TODO: Not used for now.
  // Note: direct_tick_callbacks should not be used with the new system, 
  // override action_t::proc_type() instead
  if ( callbacks )
  {
    proc_types pt = s -> proc_type();
    proc_types2 pt2 = s -> impact_proc_type2();
    if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
      action_callback_t::trigger( player -> callbacks.procs[ pt ][ pt2 ], this, s );
  }

  stats -> add_result( s -> result_amount, s -> result_amount, ( direct_tick ? DMG_OVER_TIME : periodic_hit ? DMG_DIRECT : type ), s -> result, s -> block_result, s -> target );
}

// action_t::schedule_execute ===============================================

void action_t::schedule_execute( action_state_t* execute_state )
{
  if ( sim -> log )
  {
    sim -> out_log.printf( "%s schedules execute for %s", player -> name(), name() );
  }

  time_to_execute = execute_time();

  execute_event = start_action_execute_event( time_to_execute, execute_state );

  if ( ! background )
  {
    player -> executing = this;
    player -> gcd_ready = sim -> current_time + gcd();
    if ( player -> action_queued && sim -> strict_gcd_queue )
    {
      player -> gcd_ready -= sim -> queue_gcd_reduction;
    }

    if ( special && time_to_execute > timespan_t::zero() && ! proc )
    {
      // While an ability is casting, the auto_attack is paused
      // So we simply reschedule the auto_attack by the ability's casttime
      timespan_t time_to_next_hit;
      // Mainhand
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
      {
        time_to_next_hit  = player -> main_hand_attack -> execute_event -> occurs();
        time_to_next_hit -= sim -> current_time;
        time_to_next_hit += time_to_execute;
        player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
      // Offhand
      if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
      {
        time_to_next_hit  = player -> off_hand_attack -> execute_event -> occurs();
        time_to_next_hit -= sim -> current_time;
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

  timespan_t delta_time = sim -> current_time + time - execute_event -> occurs();

  time_to_execute += delta_time;

  if ( delta_time > timespan_t::zero() )
  {
    execute_event -> reschedule( time );
  }
  else // Impossible to reschedule events "early".  Need to be canceled and re-created.
  {
    action_state_t* state = debug_cast<action_execute_event_t*>( execute_event ) -> execute_state;
    core_event_t::cancel( execute_event );
    execute_event = start_action_execute_event( time, state );
  }
}

// action_t::update_ready ===================================================

void action_t::update_ready( timespan_t cd_duration /* = timespan_t::min() */ )
{
  timespan_t delay = timespan_t::zero();

  if ( cd_duration < timespan_t::zero() )
    cd_duration = cooldown -> duration;

  if ( cd_duration > timespan_t::zero() && ! dual )
  {

    if ( ! background && ! proc )
    {
      timespan_t lag, dev;

      lag = player -> world_lag_override ? player -> world_lag : sim -> world_lag;
      dev = player -> world_lag_stddev_override ? player -> world_lag_stddev : sim -> world_lag_stddev;
      delay = rng().gauss( lag, dev );
      if ( sim -> debug )
        sim -> out_debug.printf( "%s delaying the cooldown finish of %s by %f", player -> name(), name(), delay.total_seconds() );
    }

    cooldown -> start( cd_duration, delay );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s starts cooldown for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
  }
}

// action_t::usable_moving ==================================================

bool action_t::usable_moving() const
{
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
  if ( cooldown -> down() )
    return false;

  if ( ! player -> resource_available( current_resource(), cost() ) )
    return false;

  // Player Skill must not affect precombat actions
  if ( player -> in_combat && player -> current.skill < 1.0 && ! rng().roll( player -> current.skill ) )
    return false;

  if ( line_cooldown.down() )
    return false;

  if ( sync_action && ! sync_action -> ready() )
    return false;

  if ( player -> is_moving() && ! usable_moving() )
    return false;

  if ( moving != -1 && moving != ( player -> is_moving() ? 1 : 0 ) )
    return false;

  if ( cycle_targets )
  {
    player_t* saved_target  = target;
    cycle_targets = 0;
    bool found_ready = false;

    std::vector< player_t* >& tl = target_list();
    size_t num_targets = tl.size();

    if ( ( max_cycle_targets > 0 ) && ( ( size_t ) max_cycle_targets < num_targets ) )
      num_targets = max_cycle_targets;

    for ( size_t i = 0; i < num_targets; i++ )
    {
      target = tl[ i ];
      if ( ready() )
      {
        found_ready = true;
        break;
      }
    }

    cycle_targets = 1;

    if ( found_ready ) return true;

    target = saved_target;

    return false;
  }

  if ( target_number )
  {
    player_t* saved_target  = target;
    int saved_target_number = target_number;
    target_number = 0;

    target = find_target_by_number( saved_target_number );

    bool is_ready = false;

    if ( target ) is_ready = ready();

    if ( is_ready ) return true;

    target = saved_target;

    return false;
  }

  // Check actions that DO or MAY pertain to the target after cycle_targets.

  if ( if_expr && ! if_expr -> success() )
    return false;

  if ( target -> debuffs.invulnerable -> check() && harmful )
    return false;

  if ( target -> is_sleeping() )
    return false;

  if ( ! has_movement_directionality() )
    return false;

  return true;
}

// action_t::init ===========================================================

void action_t::init()
{
  if ( initialized ) return;

  assert( ! ( impact_action && tick_action ) && "Both tick_action and impact_action should not be used in a single action." );

  assert( ! ( n_targets() && channeled ) && "DONT create a channeled aoe spell!" );

  if ( ! sync_str.empty() )
  {
    sync_action = player -> find_action( sync_str );

    if ( ! sync_action )
    {
      sim -> errorf( "Unable to find sync action '%s' for primary action '%s'\n", sync_str.c_str(), name() );
      sim -> cancel();
    }
  }

  if ( cycle_targets && target_number )
  {
    target_number = 0;
    sim -> errorf( "Player %s trying to use both cycle_targets and a numerical target for action %s - defaulting to cycle_targets\n", player -> name(), name() );
  }

  if ( ! if_expr_str.empty() )
  {
    if_expr = expr_t::parse( this, if_expr_str );
  }

  if ( ! interrupt_if_expr_str.empty() )
  {
    interrupt_if_expr = expr_t::parse( this, interrupt_if_expr_str );
  }

  if ( ! early_chain_if_expr_str.empty() )
  {
    early_chain_if_expr = expr_t::parse( this, early_chain_if_expr_str );
  }

  if ( tick_action )
  {
    tick_action -> direct_tick = true;
    tick_action -> dual = true;
    tick_action -> stats = stats;
    stats -> action_list.push_back( tick_action );
  }

  stats -> school      = get_school() ;

  if ( quiet ) stats -> quiet = true;

  if ( may_multistrike == -1 )
    may_multistrike = may_crit || tick_may_crit;

  if ( may_crit || tick_may_crit )
    snapshot_flags |= STATE_CRIT | STATE_TGT_CRIT;

  if ( ( spell_power_mod.tick > 0 || attack_power_mod.tick > 0 ) && dot_duration > timespan_t::zero() )
    snapshot_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA | STATE_MUL_PERSISTENT | STATE_VERSATILITY;

  if ( ( spell_power_mod.direct > 0 || attack_power_mod.direct > 0 ) || weapon_multiplier > 0 )
    snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_MUL_PERSISTENT | STATE_VERSATILITY;

  if ( ( spell_power_mod.direct > 0 || spell_power_mod.tick > 0 ) )
  {
    snapshot_flags |= STATE_SP;
    if ( player -> role == ROLE_TANK )
      snapshot_flags |= STATE_RESOLVE;
  }

  if ( ( weapon_power_mod > 0 || attack_power_mod.direct > 0 || attack_power_mod.tick > 0 ) )
  {
    snapshot_flags |= STATE_AP;
    if ( player -> role == ROLE_TANK )
      snapshot_flags |= STATE_RESOLVE;
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

  if ( ! ( background || sequence ) && ( pre_combat || action_list == "precombat" ) )
  {
    if ( harmful )
    {
      if ( player -> first_cast == true )
      {
        if ( this -> travel_speed > 0 || this -> base_execute_time > timespan_t::zero() )
        {
          player -> first_cast = false;
          player -> precombat_action_list.push_back( this );
        }
        else
          sim -> errorf( "Player %s attempted to add harmful action %s to precombat action list. Only one spell with travel time/cast time may be cast here.", player -> name(), name() );
      }
      else
        sim -> errorf( "Player %s attempted to add harmful action %s to precombat action list. A maximum of one harmful spell may be cast here.", player -> name(), name() );
    }
    else
      player -> precombat_action_list.push_back( this );
  }
  else if ( ! ( background || sequence ) && ! action_list.empty() )
    player -> find_action_priority_list( action_list ) -> foreground_action_list.push_back( this );

  initialized = true;

  init_target_cache();
}

void action_t::init_target_cache()
{
  sim -> target_non_sleeping_list.register_callback( aoe_target_list_callback_t( this ) );
}

// action_t::reset ==========================================================

void action_t::reset()
{
  if ( pre_execute_state )
  {
    action_state_t::release( pre_execute_state );
  }
  cooldown -> reset( false );
  line_cooldown.reset( false );
  execute_event = 0;
  travel_events.clear();
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

  if ( player -> executing  == this )
  {
    was_busy = true;
    player -> executing  = 0;
  }
  if ( player -> channeling == this )
  {
    was_busy = true;
    player -> channeling  = 0;
  }

  core_event_t::cancel( execute_event );

  player -> debuffs.casting -> expire();

  if ( was_busy && ! player -> is_sleeping() )
    player -> schedule_ready();
}

// action_t::interrupt ======================================================

void action_t::interrupt_action()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "action %s of %s is interrupted", name(), player -> name() );

  if ( cooldown -> duration > timespan_t::zero() && ! dual )
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s starts cooldown for %s (%s)", player -> name(), name(), cooldown -> name() );

    cooldown -> start();
  }

  if ( player -> executing  == this ) player -> executing  = 0;
  if ( player -> channeling == this )
  {
    dot_t* dot = get_dot();
    assert( dot -> is_ticking() );
    dot -> cancel();
  }

  core_event_t::cancel( execute_event );

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

    amount_expr_t( const std::string& name, dmg_e at, result_e rt, action_t& a ) :
      action_expr_t( name, a ), amount_type( at ), result_type( rt ), state( a.get_state() )
    {
      state -> n_targets    = 1;
      state -> chain_target = 0;
      state -> result       = result_type;
    }

    virtual double evaluate()
    {
      action.snapshot_state( state, amount_type );
      state -> target = action.target;
      if ( amount_type == DMG_OVER_TIME || amount_type == HEAL_OVER_TIME )
        return action.calculate_tick_amount( state, 1.0 /* Assumes full tick damage calculation */ );
      else
      {
        state -> result_amount = action.calculate_direct_amount( state );
        if ( amount_type == DMG_DIRECT )
          state -> target -> target_mitigation( action.get_school(), amount_type, state );
        return state -> result_amount;
      }
    }

    virtual ~amount_expr_t()
    { delete state; }
  };

  if ( name_str == "cast_time" )
    return make_mem_fn_expr( name_str, *this, &action_t::execute_time );
  else if ( name_str == "execute_time" )
  {
    struct execute_time_expr_t : public action_expr_t
    {
      execute_time_expr_t( action_t& a ) : action_expr_t( "execute_time", a )
      { }

      double evaluate()
      { return std::max( action.execute_time().total_seconds(), action.gcd().total_seconds() ); }
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
      virtual double evaluate()
      {
        dot_t* dot = action.get_dot();
        if ( dot -> is_ticking() )
          return action.tick_time( action.composite_haste() ).total_seconds();
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
      new_tick_time_expr_t( action_t& a ) : action_expr_t( "new_tick_time", a ) {}
      virtual double evaluate()
      {
        return action.tick_time( action.player -> cache.spell_speed() ).total_seconds();
      }
    };
    return new new_tick_time_expr_t( *this );
  }
  else if ( name_str == "gcd" )
    return make_mem_fn_expr( name_str, *this, &action_t::gcd );
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
      virtual double evaluate()
      {
        dot_t* dot = action.get_dot();
        if ( dot -> miss_time < timespan_t::zero() ||
             action.sim -> current_time >= ( dot -> miss_time + action.last_reaction_time ) )
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
      virtual double evaluate()
      {
        return action.cooldown -> up() &&
               action.cooldown -> reset_react <= action.sim -> current_time;
      }
    };
    return new cooldown_react_expr_t( *this );
  }
  else if ( name_str == "cast_delay" )
  {
    struct cast_delay_expr_t : public action_expr_t
    {
      cast_delay_expr_t( action_t& a ) : action_expr_t( "cast_delay", a ) {}
      virtual double evaluate()
      {
        if ( action.sim -> debug )
        {
          action.sim -> out_debug.printf( "%s %s cast_delay(): can_react_at=%f cur_time=%f",
                                action.player -> name_str.c_str(),
                                action.name_str.c_str(),
                                ( action.player -> cast_delay_occurred + action.player -> cast_delay_reaction ).total_seconds(),
                                action.sim -> current_time.total_seconds() );
        }

        if ( action.player -> cast_delay_occurred == timespan_t::zero() ||
             action.player -> cast_delay_occurred + action.player -> cast_delay_reaction < action.sim -> current_time )
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

      virtual double evaluate()
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

      virtual double evaluate()
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
  else if ( name_str == "charges" )
    return make_ref_expr( name_str, cooldown -> current_charge );
  else if ( name_str == "charges_fractional" )
  {
    struct charges_fractional_expr_t : public expr_t
    {
      action_t* action;
      charges_fractional_expr_t( action_t* a ) :
        expr_t( "charges_fractional" ), action( a )
      {}
      virtual double evaluate()
      {
        cooldown_t* c = action -> cooldown;
        double charges = c -> current_charge;
        if ( action -> cooldown -> recharge_event )
          charges += 1 - ( c -> recharge_event -> remains() / c -> duration );
        return charges;
      }
    };
    return new charges_fractional_expr_t( this );
  }
  else if ( name_str == "max_charges" )
    return make_ref_expr( name_str, cooldown -> charges );
  else if ( name_str == "recharge_time" )
  {
    struct recharge_time_expr_t : public expr_t
    {
      action_t* action;
      recharge_time_expr_t( action_t* a ) : expr_t( "recharge_time" ), action( a ) {}
      virtual double evaluate()
      {
        if ( action -> cooldown -> recharge_event )
          return action -> cooldown -> recharge_event -> remains().total_seconds();
        else
          return action -> cooldown -> duration.total_seconds();
      }
    };
    return new recharge_time_expr_t( this );
  }
  else if ( name_str == "hit_damage" )
    return new amount_expr_t( name_str, DMG_DIRECT, RESULT_HIT, *this );
  else if ( name_str == "crit_damage" )
    return new amount_expr_t( name_str, DMG_DIRECT, RESULT_CRIT, *this );
  else if ( name_str == "hit_heal" )
    return new amount_expr_t( name_str, HEAL_DIRECT, RESULT_HIT, *this );
  else if ( name_str == "crit_heal" )
    return new amount_expr_t( name_str, HEAL_DIRECT, RESULT_CRIT, *this );
  else if ( name_str == "tick_damage" )
    return new amount_expr_t( name_str, DMG_OVER_TIME, RESULT_HIT, *this );
  else if ( name_str == "crit_tick_damage" )
    return new amount_expr_t( name_str, DMG_OVER_TIME, RESULT_CRIT, *this );
  else if ( name_str == "tick_heal" )
    return new amount_expr_t( name_str, HEAL_OVER_TIME, RESULT_HIT, *this );
  else if ( name_str == "crit_tick_heal" )
    return new amount_expr_t( name_str, HEAL_OVER_TIME, RESULT_CRIT, *this );
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
      virtual double evaluate()
      {
        state -> target = action -> target;
        action -> snapshot_state( state, RESULT_TYPE_NONE );

        return std::min( 100.0, state -> composite_crit() * 100.0 );
      }

      virtual ~crit_pct_current_expr_t()
      { delete state; }
    };
    return new crit_pct_current_expr_t( this );
  }

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits.size() == 2 )
  {
    if ( splits[ 0 ] == "prev" )
    {
      struct prev_expr_t : public action_expr_t
      {
        std::string prev_action;
        prev_expr_t( action_t& a, const std::string& prev_action ) : action_expr_t( "prev", a ), prev_action( prev_action ) {}
        virtual double evaluate()
        {
          if ( action.player -> last_foreground_action )
            return action.player -> last_foreground_action -> name_str == prev_action;
          return false;
        }
      };

      return new prev_expr_t( *this, splits[ 1 ] );
    }
  }

  if ( splits.size() == 3 && splits[ 0 ] == "dot" )
  {
    return target -> get_dot( splits[ 1 ], player ) -> create_expression( this, splits[ 2 ], true );
  }
  if ( splits.size() == 3 && splits[ 0 ] == "enemy_dot" )
  {
    // simple by-pass to test
    return player -> get_dot( splits[ 1 ], target ) -> create_expression( this, splits[ 2 ], false );

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

      double evaluate()
      {
        double ret = 0;
        for ( size_t i = 0, size = expr_list.size(); i < size; i++ )
        {
          double expr_result = expr_list[ i ] -> eval();
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

  if ( splits.size() > 2 && splits[ 0 ] == "target" )
  {
    // Find target
    player_t* expr_target;
    if ( splits[ 1 ][ 0 ] >= '0' && splits[ 1 ][ 0 ] <= '9' )
      expr_target = find_target_by_number( atoi( splits[ 1 ].c_str() ) );
    else
      expr_target = sim -> find_player( splits[ 1 ] );
    size_t start_rest = 2;
    if ( ! expr_target )
    {
      expr_target = target;
      start_rest = 1;
    }

    assert( expr_target );

    std::string rest = splits[ start_rest ];
    for ( size_t i = start_rest + 1; i < splits.size(); ++i )
      rest += '.' + splits[ i ];

    return expr_target -> create_expression( this, rest );
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
      virtual double evaluate() { return action.data().found(); }
    };
    return new ok_expr_t( *this );
  }
  else if ( name_str == "executing" )
  {
    struct executing_expr_t : public action_expr_t
    {
      executing_expr_t( action_t& a ) : action_expr_t( "executing", a ) {}
      virtual double evaluate() { return action.execute_event != 0; }
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

timespan_t action_t::tick_time( double haste ) const
{
  timespan_t t = base_tick_time;
  if ( channeled || hasted_ticks )
  {
    t *= haste;
  }
  return t;
}

// action_t::snapshot_internal ==============================================

void action_t::snapshot_internal( action_state_t* state, uint32_t flags, dmg_e rt )
{
  assert( state );

  state -> result_type = rt;

  if ( flags & STATE_CRIT )
    state -> crit = composite_crit() * composite_crit_multiplier();

  if ( flags & STATE_HASTE )
    state -> haste = composite_haste();

  if ( flags & STATE_AP )
    state -> attack_power = int( composite_attack_power() * player -> composite_attack_power_multiplier() );

  if ( flags & STATE_SP )
    state -> spell_power = int( composite_spell_power() * player -> composite_spell_power_multiplier() );

  if ( flags & STATE_VERSATILITY )
    state -> versatility = composite_versatility( state );

  if ( flags & STATE_MUL_DA )
    state -> da_multiplier = composite_da_multiplier( state );

  if ( flags & STATE_MUL_TA )
    state -> ta_multiplier = composite_ta_multiplier( state );

  if ( flags & STATE_MUL_PERSISTENT )
    state -> persistent_multiplier = composite_persistent_multiplier( state );

  if ( flags & STATE_TGT_MUL_DA )
    state -> target_da_multiplier = composite_target_da_multiplier( state -> target );

  if ( flags & STATE_TGT_MUL_TA )
    state -> target_ta_multiplier = composite_target_ta_multiplier( state -> target );

  if ( flags & STATE_TGT_CRIT )
    state -> target_crit = composite_target_crit( state -> target ) * composite_crit_multiplier();

  if ( flags & STATE_TGT_MITG_DA )
    state -> target_mitigation_da_multiplier = composite_target_mitigation( state -> target, get_school() );

  if ( flags & STATE_TGT_MITG_TA )
    state -> target_mitigation_ta_multiplier = composite_target_mitigation( state -> target, get_school() );

  if ( flags & STATE_RESOLVE )
    state -> resolve = composite_resolve( state );
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
    return dot_duration * ( tick_time( s -> haste ) / base_tick_time );
  }

  return dot_duration;
}

double action_t::composite_target_crit( player_t* target ) const
{
  return std::min( player -> level - target -> level, 0 ) / 100.0;
}

core_event_t* action_t::start_action_execute_event( timespan_t t, action_state_t* execute_event )
{
  return new ( *sim ) action_execute_event_t( this, t, execute_event );
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

    add_travel_event( new ( *sim ) travel_event_t( this, state, time_ ) );
  }
}

void action_t::schedule_travel( action_state_t* s )
{
  if ( ! execute_state )
    execute_state = get_state();

  execute_state -> copy_state( s );

  time_to_travel = travel_time();

  do_schedule_travel( s, time_to_travel );
}

void action_t::impact( action_state_t* s )
{
  assess_damage( type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT, s );

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
      impact_action -> execute();
    }
  }
  else if ( result_is_multistrike( s -> result ) )
    ; // TODO: WOD-MULTISTRIKE
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
    duration -= tick_time( s -> haste );

  dot_t* dot = get_dot( s -> target );

  if ( dot_behavior == DOT_CLIP ) dot -> cancel();

  dot -> current_action = this;

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

bool action_t::has_travel_events_for( const player_t* target ) const
{
  for ( size_t i = 0; i < travel_events.size(); ++i )
  {
    if ( travel_events[ i ] -> state -> target == target )
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

/* Calculates the new dot length after a refresh
 * Necessary because we have both pandemic behaviour ( last 30% of the dot are preserved )
 * and old Cata/MoP behavior ( only time to the next tick is preserved )
 */
timespan_t action_t::calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const
{
  // WoD Pandemic
  return std::min( triggered_duration * 0.3, dot -> remains() ) + triggered_duration; // New WoD Formula: Get no malus during the last 30% of the dot.

}

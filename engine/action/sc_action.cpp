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
  player_gcd_event_t( player_t* p,
                      timespan_t delta_time ) :
    event_t( p, "Player-Ready-GCD" )
  {
    if ( sim.debug )
      sim.output( "New Player-Ready-GCD Event: %s", p -> name() );

    sim.add_event( this, delta_time );
  }

  virtual void execute()
  {
    for ( std::vector<action_t*>::const_iterator i = player -> active_off_gcd_list -> off_gcd_actions.begin();
          i < player -> active_off_gcd_list -> off_gcd_actions.end(); ++i )
    {
      action_t* a = *i;
      if ( a -> ready() )
      {
        action_priority_list_t* alist = player -> active_action_list;

        a -> execute();
        a -> line_cooldown.start();
        if ( ! a -> quiet )
        {
          player -> iteration_executed_foreground_actions++;
          a -> total_executions++;

          player -> sequence_add( a, a -> target, sim.current_time );
        }

        // Need to restart because the active action list changed
        if ( alist != player -> active_action_list )
        {
          player -> activate_action_list( player -> active_action_list, true );
          execute();
          player -> activate_action_list( alist, true );
          return;
        }
      }
    }

    if ( player -> restore_action_list != 0 )
    {
      player -> activate_action_list( player -> restore_action_list );
      player -> restore_action_list = 0;
    }

    player -> off_gcd = new ( sim ) player_gcd_event_t( player, timespan_t::from_seconds( 0.1 ) );
  }
};
// Action Execute Event =====================================================

struct action_execute_event_t : public event_t
{
  action_t* action;
  action_state_t* execute_state;

  action_execute_event_t( action_t* a,
                          timespan_t time_to_execute,
                          action_state_t* state = 0 ) :
    event_t( a -> player, "Action-Execute" ), action( a ), execute_state( state )
  {
    if ( sim.debug )
      sim.output( "New Action Execute Event: %s %s %.1f (target=%s, marker=%c)",
                  player -> name(), a -> name(), time_to_execute.total_seconds(),
                  a -> target -> name(), ( a -> marker ) ? a -> marker : '0' );
    sim.add_event( this, time_to_execute );
  }

  // Ensure we properly release the carried execute_state even if this event
  // is never executed.
  ~action_execute_event_t()
  { if ( execute_state ) action_state_t::release( execute_state ); }

  // action_execute_event_t::execute ========================================

  virtual void execute()
  {
    // Pass the carried execute_state to the action. This saves us a few
    // cycles, as we don't need to make a copy of the state to pass to
    // action -> pre_execute_state.
    if ( execute_state )
    {
      action -> pre_execute_state = execute_state;
      execute_state = 0;
    }

    action -> execute_event = 0;
    action -> execute();

    if ( action -> background ) return;

    if ( ! player -> channeling )
    {
      if ( player -> readying )
        sim.output( "Danger Will Robinson!  Danger!  action %s player %s\n",
                    action -> name(), player -> name() );

      player -> schedule_ready( timespan_t::zero() );
    }

    if ( player -> active_off_gcd_list == 0 )
      return;

    // Kick off the during-gcd checker, first run is immediately after
    event_t::cancel( player -> off_gcd );

    if ( ! player -> channeling )
      player -> off_gcd = new ( sim ) player_gcd_event_t( player, timespan_t::zero() );
  }

};

struct aoe_target_list_callback_t : public callback_t
{
  action_t* action;
  aoe_target_list_callback_t( action_t* a ) : callback_t(), action( a ) {}

  virtual void execute()
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
  const spell_data_t* s = p -> find_talent_spell( name, "", false, false );
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
  result(),
  resource_current( RESOURCE_NONE ),
  aoe(),
  pre_combat( 0 ),
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
  item_proc(),
  proc_ignores_slot(),
  discharge_proc(),
  auto_cast(),
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
  direct_power_mod(),
  tick_power_mod(),
  base_execute_time( timespan_t::zero() ),
  base_tick_time( timespan_t::zero() ),
  time_to_execute( timespan_t::zero() ),
  time_to_travel( timespan_t::zero() ),
  total_executions(),
  line_cooldown( cooldown_t( "line_cd", *p ) )
{
  dot_behavior                   = DOT_CLIP;
  trigger_gcd                    = player -> base_gcd;
  range                          = -1.0;
  weapon_power_mod               = 1.0 / 14.0;


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
  base_spell_power_multiplier    = 0.0;
  base_attack_power_multiplier   = 0.0;
  crit_multiplier                = 1.0;
  crit_bonus_multiplier          = 1.0;
  base_dd_adder                  = 0.0;
  base_ta_adder                  = 0.0;
  stormlash_da_multiplier        = 1.0;
  stormlash_ta_multiplier        = 0.0; // Stormlash is disabled for ticks by default
  num_ticks                      = 0;
  weapon                         = NULL;
  weapon_multiplier              = 1.0;
  base_add_multiplier            = 1.0;
  base_aoe_multiplier            = 1.0;
  split_aoe_damage               = false;
  normalize_weapon_speed         = false;
  rng_result                     = NULL;
  rng_travel                     = NULL;
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

  range::fill( base_costs, 0.0 );
  range::fill( costs_per_second, 0 );

  if ( name_str.empty() )
  {
    assert( data().ok() );

    name_str = dbc::get_token( data().id() );

    if ( name_str.empty() )
    {
      name_str = data().name_cstr();
      util::tokenize( name_str );
      assert( ! name_str.empty() );
      player -> dbc.add_token( data().id(), name_str );
    }
  }
  else
  {
    util::tokenize( name_str );
  }

  if ( sim -> debug ) sim -> output( "Player %s creates action %s (%d)", player -> name(), name(), ( s_data -> ok() ? s_data -> id() : -1 ) );

  if ( unlikely( ! player -> initialized ) )
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
  spec_list.clear();
}

action_t::~action_t()
{
  delete execute_state;
  delete pre_execute_state;
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

  if ( likely( spell_data._power && spell_data._power -> size() == 1 ) )
    resource_current = spell_data._power -> at( 0 ) -> resource();

  for ( size_t i = 0; spell_data._power && i < spell_data._power -> size(); i++ )
  {
    const spellpower_data_t* pd = spell_data._power -> at( i );

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
      direct_power_mod = spelleffect_data.coeff();
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
          tick_power_mod   = spelleffect_data.coeff();
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
            num_ticks        = ( int ) ( spelleffect_data.spell() -> duration() / base_tick_time );
          }
          break;
        case A_SCHOOL_ABSORB:
          direct_power_mod = spelleffect_data.coeff();
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

double action_t::cost()
{
  if ( ! harmful && ! player -> in_combat )
    return 0;

  resource_e cr = current_resource();

  double c = base_costs[ cr ];

  c -= player -> current.resource_reduction[ school ];

  if ( cr == RESOURCE_MANA && player -> buffs.courageous_primal_diamond_lucidity -> check() )
    c = 0;

  if ( c < 0 ) c = 0;

  if ( sim -> debug ) sim -> output( "action_t::cost: %s %.2f %.2f %s", name(), base_costs[ cr ], c, util::resource_type_string( cr ) );

  return floor( c );
}

// action_t::gcd ============================================================

timespan_t action_t::gcd()
{
  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero();

  return trigger_gcd;
}

// action_t::travel_time ====================================================

timespan_t action_t::travel_time()
{
  if ( travel_speed == 0 ) return timespan_t::zero();

  if ( player -> current.distance == 0 ) return timespan_t::zero();

  double t = player -> current.distance / travel_speed;

  double v = sim -> travel_variance;

  if ( v )
  {
    t = rng_travel -> gauss( t, v );
  }

  return timespan_t::from_seconds( t );
}

// action_t::crit_chance ====================================================

double action_t::crit_chance( double crit, int /* delta_level */ )
{
  double chance = crit;

  if ( chance < 0.0 )
    chance = 0.0;

  return chance;
}

// action_t::total_crit_bonus ===============================================

double action_t::total_crit_bonus()
{
  double crit_multiplier_buffed = crit_multiplier * ( 1.0 + player -> buffs.skull_banner -> value() );
  double bonus = ( ( 1.0 + crit_bonus ) * crit_multiplier_buffed - 1.0 ) * crit_bonus_multiplier;

  if ( sim -> debug )
  {
    sim -> output( "%s crit_bonus for %s: cb=%.3f b_cb=%.2f b_cm=%.2f b_cbm=%.2f",
                   player -> name(), name(), bonus, crit_bonus, crit_multiplier_buffed, crit_bonus_multiplier );
  }

  return bonus;
}

// action_t::calculate_weapon_damage ========================================

double action_t::calculate_weapon_damage( double attack_power )
{
  if ( ! weapon || weapon_multiplier <= 0 ) return 0;

  double dmg = sim -> averaged_range( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> bonus_dmg;

  timespan_t weapon_speed  = normalize_weapon_speed  ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

  double power_damage = weapon_speed.total_seconds() * weapon_power_mod * attack_power;

  double total_dmg = dmg + power_damage;

  // OH penalty
  if ( weapon -> slot == SLOT_OFF_HAND )
    total_dmg *= 0.5;

  if ( sim -> debug )
  {
    sim -> output( "%s weapon damage for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f pd=%.3f ap=%.3f",
                   player -> name(), name(), total_dmg, dmg, weapon -> bonus_dmg, weapon_speed.total_seconds(), power_damage, attack_power );
  }

  return total_dmg;
}

// action_t::calculate_tick_amount ==========================================

double action_t::calculate_tick_amount( action_state_t* state )
{
  double amount = 0;

  if ( base_ta( state ) == 0 && tick_power_coefficient( state ) == 0 ) return 0;

  amount  = floor( base_ta( state ) + 0.5 ) + bonus_ta( state ) + state -> composite_power() * tick_power_coefficient( state );
  amount *= state -> composite_ta_multiplier();

  double init_tick_amount = amount;

  if ( state -> result == RESULT_CRIT )
    amount *= 1.0 + total_crit_bonus();

  if ( ! sim -> average_range )
    amount = floor( amount + sim -> real() );

  if ( sim -> debug )
  {
    sim -> output( "%s amount for %s on %s: ta=%.0f i_ta=%.0f b_ta=%.0f bonus_ta=%.0f mod=%.2f power=%.0f mult=%.2f",
                   player -> name(), name(), target -> name(), amount,
                   init_tick_amount, base_ta( state ), bonus_ta( state ),
                   tick_power_coefficient( state ), state -> composite_power(),
                   state -> composite_ta_multiplier() );
  }

  return amount;
}

// action_t::calculate_direct_amount ========================================

double action_t::calculate_direct_amount( action_state_t* state, int chain_target )
{
  double amount = sim -> averaged_range( base_da_min( state ), base_da_max( state ) );

  if ( round_base_dmg ) amount = floor( amount + 0.5 );

  if ( amount == 0 && weapon_multiplier == 0 && direct_power_coefficient( state ) == 0 ) return 0;

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
  amount += direct_power_coefficient( state ) * ( state -> composite_power() );
  amount *= state -> composite_da_multiplier();

  double init_direct_amount = amount;

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

  // AoE with decay per target
  if ( chain_target > 0 && base_add_multiplier != 1.0 )
    amount *= pow( base_add_multiplier, chain_target );

  // AoE with static reduced damage per target
  if ( chain_target > 1 && base_aoe_multiplier != 1.0 )
    amount *= base_aoe_multiplier;

  if ( ! sim -> average_range ) amount = floor( amount + sim -> real() );

  if ( sim -> debug )
  {
    sim -> output( "%s amount for %s: dd=%.0f i_dd=%.0f w_dd=%.0f b_dd=%.0f mod=%.2f power=%.0f mult=%.2f w_mult=%.2f",
                   player -> name(), name(), amount, init_direct_amount, weapon_amount, base_direct_amount, direct_power_coefficient( state ),
                   state -> composite_power(), state -> composite_da_multiplier(), weapon_multiplier );
  }

  // Record total amount to state
  state -> result_total = amount;

  return amount;
}

// action_t::consume_resource ===============================================

void action_t::consume_resource()
{
  resource_e cr = current_resource();

  if ( cr == RESOURCE_NONE || base_costs[ cr ] == 0 || proc ) return;

  resource_consumed = cost();

  player -> resource_loss( cr, resource_consumed, 0, this );

  if ( sim -> log )
    sim -> output( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
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

size_t action_t::available_targets( std::vector< player_t* >& tl )
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

std::vector< player_t* >& action_t::target_list()
{
  // Check if target cache is still valid. If not, recalculate it
  if ( ! target_cache.is_valid )
  {
    available_targets( target_cache.list );
    target_cache.is_valid = true;
  }

  return target_cache.list;
}

player_t* action_t::find_target_by_number( int number )
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

// action_t::execute ========================================================

void action_t::execute()
{
#ifndef NDEBUG
  if ( unlikely( ! initialized ) )
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
    sim -> output( "%s performs %s (%.0f)", player -> name(), name(),
                   player -> resources.current[ player -> primary_resource() ] );
  }

  if ( aoe == 0 && target -> is_sleeping() )
    return;

  if ( harmful )
  {
    if ( player -> in_combat == false && sim -> debug )
      sim -> output( "%s enters combat.", player -> name() );

    player -> in_combat = true;
  }

  if ( aoe == -1 || aoe > 0 ) // aoe
  {
    std::vector< player_t* >& tl = target_list();

    size_t num_targets = ( aoe < 0 ) ? tl.size() : aoe;
    for ( size_t t = 0, max_targets = tl.size(); t < num_targets && t < max_targets; t++ )
    {
      action_state_t* s = get_state( pre_execute_state );
      s -> target = tl[ t ];
      if ( ! pre_execute_state ) snapshot_state( s, type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT );
      s -> result = calculate_result( s );

      if ( result_is_hit( s -> result ) )
        s -> result_amount = calculate_direct_amount( s, as<int>( t + 1 ) );

      if ( split_aoe_damage )
        s -> result_amount /= num_targets;

      if ( ! split_aoe_damage && tl.size() > static_cast< size_t >( sim -> max_aoe_enemies ) )
        s -> result_amount *= sim -> max_aoe_enemies / static_cast< double >( tl.size() );

      if ( sim -> debug )
        s -> debug();

      schedule_travel( s );
    }
  }
  else // single target
  {
    action_state_t* s = get_state( pre_execute_state );
    s -> target = target;
    if ( ! pre_execute_state ) snapshot_state( s, type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT );
    s -> result = calculate_result( s );

    if ( result_is_hit( s -> result ) )
      s -> result_amount = calculate_direct_amount( s, 0 );

    if ( sim -> debug )
      s -> debug();

    schedule_travel( s );
  }

  consume_resource();

  update_ready();

  if ( ! dual ) stats -> add_execute( time_to_execute, target );

  if ( pre_execute_state )
  {
    action_state_t::release( pre_execute_state );
  }

  if ( execute_action && result_is_hit( execute_state -> result ) )
  {
    assert( ! execute_action -> pre_execute_state );
    execute_action -> schedule_execute( execute_action -> get_state( execute_state ) );
  }

  if ( repeating && ! proc ) schedule_execute();
}

// action_t::tick ===========================================================

void action_t::tick( dot_t* d )
{
  if ( sim -> debug ) sim -> output( "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );

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

    if ( tick_may_crit && rng_result -> roll( crit_chance( d -> state -> composite_crit(), d -> state -> target -> level - player -> level ) ) )
      d -> state -> result = RESULT_CRIT;

    d -> state -> result_amount = calculate_tick_amount( d -> state );

    assess_damage( type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME, d -> state );
  }

  if ( harmful && callbacks && type != ACTION_HEAL )
    action_callback_t::trigger( player -> callbacks.tick[ d -> state -> result ], this, d -> state );

  if ( sim -> debug )
    d -> state -> debug();

  if ( ! periodic_hit ) stats -> add_tick( d -> time_to_tick, d -> state -> target );

  player -> trigger_ready();
}

// action_t::last_tick ======================================================

void action_t::last_tick( dot_t* d )
{
  if ( school == SCHOOL_PHYSICAL )
  {
    buff_t* b = d -> state -> target -> debuffs.bleeding;
    if ( b -> current_value > 0 ) b -> current_value -= 1.0;
    if ( b -> current_value == 0 ) b -> expire();
  }
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( dmg_e    type,
                              action_state_t* s )
{
  // hook up vengeance here, before armor mitigation, avoidance, and dmg reduction effects, etc.
  if ( s -> target -> vengeance_is_started() && ( type == DMG_DIRECT || type == DMG_OVER_TIME ) && s-> action -> player -> is_enemy() )
  {
    if ( result_is_miss( s -> result ) && //is a miss
         ( s -> action -> player -> level >= ( s -> target -> level + 3 ) ) && // is a boss
         !( player -> main_hand_attack && s -> action == player -> main_hand_attack ) ) // is no auto_attack
    {
      //extend duration
      s -> target -> buffs.vengeance -> trigger( 1,
                                                 s -> target -> buffs.vengeance -> value(),
                                                 1.0 ,
                                                 timespan_t::from_seconds( 20.0 ) );
    }
    else // vengeance from successful hit or missed auto attack
    {

      // factor out weakened_blows
      double raw_damage =  ( school == SCHOOL_PHYSICAL ? 1.0 : 2.5 ) * s -> action -> player -> debuffs.weakened_blows -> check() ?
                           s -> result_amount / ( 1.0 - s -> action -> player -> debuffs.weakened_blows -> value() ) :
                           s -> result_amount;

      // Take swing time for auto_attacks, take 60 for special attacks (this is how blizzard does it)
      // TODO: Do off hand autoattacks generate vengeance?
      double attack_frequency = 0.0;
      if ( player -> main_hand_attack && s -> action == player -> main_hand_attack )
        attack_frequency = 1.0 / s -> action -> execute_time().total_seconds();
      else
        attack_frequency = 1.0 / 60.0;

      // if it was a miss use average value instead. (as Blizzard does)
      if ( result_is_miss( s -> result ) )
      {
        raw_damage = s -> target -> buffs.vengeance -> value() / 20 / attack_frequency / 0.018;
      }

      // Create new vengeance value
      double new_amount = 0.018 * raw_damage; // new vengeance from hit



      new_amount += s -> target -> buffs.vengeance -> value() *
                    s -> target -> buffs.vengeance -> remains().total_seconds() / 20.0; // old diminished vengeance

      double vengeance_equil = 0.018 * raw_damage * attack_frequency * 20;
      if ( vengeance_equil / 2.0 > new_amount )
        new_amount = vengeance_equil / 2.0;

      if ( new_amount > s -> target -> resources.max[ RESOURCE_HEALTH ] ) new_amount = s -> target -> resources.max[ RESOURCE_HEALTH ];

      if ( sim -> debug )
      {
        sim -> output( "%s updated vengeance due to %s from %s. New vengeance.value=%.2f vengeance.damage=%.2f.\n",
                       s -> target -> name(), s -> action -> name(), s -> action -> player -> name() , new_amount,
                       s -> result_amount );
      }

      s -> target -> buffs.vengeance -> trigger( 1, new_amount, 1, timespan_t::from_seconds( 20.0 ) );
    }
  } // END Vengeance

  s -> target -> assess_damage( school, type, s );

  if ( sim -> debug )
    s -> debug();

  if ( type == DMG_DIRECT )
  {
    if ( sim -> log )
    {
      sim -> output( "%s %s hits %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     s -> target -> name(), s -> result_amount,
                     util::school_type_string( school ),
                     util::result_type_string( s -> result ) );
    }

    if ( s -> result_amount > 0.0 )
    {
      if ( direct_tick_callbacks )
      {
        action_callback_t::trigger( player -> callbacks.tick_damage[ school ], this, s );
      }
      else
      {
        if ( callbacks )
        {
          action_callback_t::trigger( player -> callbacks.direct_damage[ school ], this, s );
          if ( s -> result == RESULT_CRIT )
          {
            action_callback_t::trigger( player -> callbacks.direct_crit[ school ], this, s );
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
      sim -> output( "%s %s ticks (%d of %d) %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     dot -> current_tick, dot -> num_ticks,
                     s -> target -> name(), s -> result_amount,
                     util::school_type_string( school ),
                     util::result_type_string( dot -> state -> result ) );
    }

    if ( callbacks && s -> result_amount > 0.0 ) action_callback_t::trigger( player -> callbacks.tick_damage[ school ], this, s );
  }

  stats -> add_result( s -> result_amount, s -> result_amount, ( direct_tick ? DMG_OVER_TIME : periodic_hit ? DMG_DIRECT : type ), s -> result, s -> target );
}

// action_t::schedule_execute ===============================================

void action_t::schedule_execute( action_state_t* execute_state )
{
  if ( sim -> log )
  {
    sim -> output( "%s schedules execute for %s", player -> name(), name() );
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
    sim -> output( "%s reschedules execute for %s", player -> name(), name() );
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
    event_t::cancel( execute_event );
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
      delay = player -> rngs.lag_world -> gauss( lag, dev );
      if ( sim -> debug ) sim -> output( "%s delaying the cooldown finish of %s by %f", player -> name(), name(), delay.total_seconds() );
    }

    cooldown -> start( cd_duration, delay );

    if ( sim -> debug ) sim -> output( "%s starts cooldown for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
  }
  if ( num_ticks )
  {
    if ( result_is_miss() )
    {
      dot_t* dot = get_dot( target );
      last_reaction_time = player -> total_reaction_time();
      if ( sim -> debug )
        sim -> output( "%s pushes out re-cast (%.2f) on miss for %s (%s)",
                       player -> name(), last_reaction_time.total_seconds(), name(), dot -> name() );

      dot -> miss_time = sim -> current_time + time_to_travel;
    }
  }
}

// action_t::usable_moving ==================================================

bool action_t::usable_moving()
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
  if ( player -> in_combat && player -> current.skill < 1.0 && ! sim -> roll( player -> current.skill ) )
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

  if ( unlikely( target -> is_sleeping() ) )
    return false;

  return true;
}

// action_t::init ===========================================================

void action_t::init()
{
  if ( initialized ) return;

  rng_result = player -> get_rng( name_str + "_result" );

  assert( !( aoe && channeled ) && "DONT create a channeled aoe spell!" );

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

  if ( sim -> travel_variance && travel_speed && player -> current.distance )
    rng_travel = player -> get_rng( name_str + "_travel" );

  if ( tick_action )
  {
    tick_action -> direct_tick = true;
    tick_action -> dual = true;
    tick_action -> stats = stats;
    stats -> action_list.push_back( tick_action );
  }

  stats -> school      = school;

  if ( quiet ) stats -> quiet = true;

  if ( may_crit || tick_may_crit )
    snapshot_flags |= STATE_CRIT | STATE_TGT_CRIT;

  if ( base_td > 0 || num_ticks > 0 )
    snapshot_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA;

  if ( ( base_dd_min > 0 && base_dd_max > 0 ) || weapon_multiplier > 0 )
    snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;

  if ( base_spell_power_multiplier > 0 && ( direct_power_mod > 0 || tick_power_mod > 0 ) )
    snapshot_flags |= STATE_SP;

  if ( base_attack_power_multiplier > 0 && ( weapon_power_mod > 0 || direct_power_mod > 0 || tick_power_mod > 0 ) )
    snapshot_flags |= STATE_AP;

  if ( num_ticks > 0 && ( hasted_ticks || channeled ) )
    snapshot_flags |= STATE_HASTE;

  if ( pre_combat || action_list == "precombat" )
  {
    if ( harmful )
      sim -> errorf( "Player %s attempted to add harmful action %s to precombat action list", player -> name(), name() );
    else
      player -> precombat_action_list.push_back( this );
  }
  else if ( ! ( background || sequence ) )
    player -> find_action_priority_list( action_list ) -> foreground_action_list.push_back( this );

  initialized = true;

  init_target_cache();
}

void action_t::init_target_cache()
{
  target_cache.callback = new aoe_target_list_callback_t( this );
  sim -> target_non_sleeping_list.register_callback( target_cache.callback );
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
  // FIXME! Is this really necessary? All DOTs get reset during player_t::reset()
  dot_t* dot = find_dot();
  if ( dot ) dot -> reset();
  result = RESULT_NONE;
  execute_event = 0;
  travel_events.clear();
}

// action_t::cancel =========================================================

void action_t::cancel()
{
  if ( sim -> debug ) sim -> output( "action %s of %s is canceled", name(), player -> name() );

  if ( channeled )
    get_dot() -> cancel();

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

  event_t::cancel( execute_event );

  player -> debuffs.casting -> expire();

  if ( was_busy && ! player -> is_sleeping() )
    player -> schedule_ready();
}

// action_t::interrupt ======================================================

void action_t::interrupt_action()
{
  if ( sim -> debug ) sim -> output( "action %s of %s is interrupted", name(), player -> name() );

  if ( cooldown -> duration > timespan_t::zero() && ! dual )
  {
    if ( sim -> debug ) sim -> output( "%s starts cooldown for %s (%s)", player -> name(), name(), cooldown -> name() );

    cooldown -> start();
  }

  if ( player -> executing  == this ) player -> executing  = 0;
  if ( player -> channeling == this )
  {
    dot_t* dot = get_dot();
    if ( dot -> ticking ) last_tick( dot );
    player -> channeling = nullptr;
    dot -> reset();
  }

  event_t::cancel( execute_event );

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
    { }

    virtual double evaluate()
    {
      action.snapshot_state( state, amount_type );
      state -> result = result_type;
      state -> target = action.target;
      if ( amount_type == DMG_OVER_TIME || amount_type == HEAL_OVER_TIME )
        return action.calculate_tick_amount( state );
      else
      {
        state -> result_amount = action.calculate_direct_amount( state, 0 );
        if ( amount_type == DMG_DIRECT )
          state -> target -> target_mitigation( action.school, amount_type, state );
        return state -> result_amount;
      }
    }
  };

  if ( name_str == "n_ticks" )
  {
    struct n_ticks_expr_t : public action_expr_t
    {
      n_ticks_expr_t( action_t& a ) : action_expr_t( "n_ticks", a ) {}
      virtual double evaluate()
      {
        dot_t* dot = action.get_dot();
        int n_ticks = action.hasted_num_ticks( action.player -> cache.spell_speed() );
        if ( action.dot_behavior == DOT_EXTEND && dot -> ticking )
          n_ticks += std::min( ( int ) ( n_ticks / 2 ), dot -> num_ticks - dot -> current_tick );
        return n_ticks;
      }
    };
    return new n_ticks_expr_t( *this );
  }
  else if ( name_str == "add_ticks" )
  {
    struct add_ticks_expr_t : public action_expr_t
    {
      add_ticks_expr_t( action_t& a ) : action_expr_t( "add_ticks", a ) {}
      virtual double evaluate() { return action.hasted_num_ticks( action.player -> cache.spell_speed() ); }
    };
    return new add_ticks_expr_t( *this );
  }
  else if ( name_str == "cast_time" )
    return make_mem_fn_expr( name_str, *this, &action_t::execute_time );
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
        if ( dot -> ticking )
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
          action.sim -> output( "%s %s cast_delay(): can_react_at=%f cur_time=%f",
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
    return make_mem_fn_expr( "tick_multiplier", *this, &action_t::composite_ta_multiplier );
  else if ( name_str == "charges" )
    return make_ref_expr( name_str, cooldown -> current_charge );
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
          return ( action -> cooldown -> recharge_event -> time - action -> sim -> current_time ).total_seconds();
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
      crit_pct_current_expr_t( action_t* a ) : expr_t( "crit_pct_current" ), action( a ) {}
      virtual double evaluate()
      {
        return action -> composite_crit() * 100.0;
      }
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

double action_t::ppm_proc_chance( double PPM )
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

// action_t::real_ppm_proc_chance ===========================================

double action_t::real_ppm_proc_chance( double PPM, timespan_t last_trigger, timespan_t last_successful_proc, rppm_scale_e scales_with )
{
  // Old RPPM formula
  double coeff = 1.0 / std::min( player -> cache.spell_haste(), player -> cache.attack_haste() );
  double seconds = std::min( ( sim -> current_time - last_trigger ).total_seconds(), 10.0 );

  switch ( scales_with )
  {
    case RPPM_ATTACK_CRIT:
      coeff *= 1.0 + player -> cache.attack_crit();
      break;
    case RPPM_SPELL_CRIT:
      coeff *= 1.0 + player -> cache.spell_crit();
      break;
    default: break;
  }

  double old_rppm_chance = ( PPM * ( seconds / 60.0 ) ) * coeff;


  // RPPM Extension added on 12. March 2013: http://us.battle.net/wow/en/blog/8953693?page=44
  // Formula see http://us.battle.net/wow/en/forum/topic/8197741003#1
  double last_success = std::min( ( sim -> current_time - last_successful_proc ).total_seconds(), 1000.0 );

  double expected_average_proc_interval = 60.0 / ( PPM * coeff );
  return std::max( 1.0, 1 + ( ( last_success / expected_average_proc_interval - 1.5 ) * 3.0 ) )  * old_rppm_chance;
}

// action_t::tick_time ======================================================

timespan_t action_t::tick_time( double haste )
{
  timespan_t t = base_tick_time;
  if ( channeled || hasted_ticks )
  {
    t *= haste;
  }
  return t;
}

// action_t::hasted_num_ticks ===============================================

int action_t::hasted_num_ticks( double haste, timespan_t d )
{
  if ( ! hasted_ticks )
  {
    if ( d < timespan_t::zero() )
      return num_ticks;
    else
      return static_cast<int>( d / base_tick_time );
  }

#ifndef NDEBUG
  if ( haste <= 0.0 )
  {
    sim -> errorf( "%s action_t::hasted_num_ticks, action %s haste <= 0.0", player -> name(), name() );
    assert( false );
  }
#endif

  // For the purposes of calculating the number of ticks, the tick time is rounded to the 3rd decimal place.
  // It's important that we're accurate here so that we model haste breakpoints correctly.

  if ( d < timespan_t::zero() )
    d = num_ticks * base_tick_time;

  timespan_t t = timespan_t::from_millis( ( int ) ( ( base_tick_time.total_millis() * haste ) + 0.5 ) );

  double n = d / t;

  // banker's rounding
  if ( n - 0.5 == ( double ) ( int ) n && ( ( int ) n ) % 2 == 0 )
    return ( int ) ceil ( n - 0.5 );

  return ( int ) floor( n + 0.5 );
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

  if ( flags & STATE_MUL_DA )
    state -> da_multiplier = composite_da_multiplier();

  if ( flags & STATE_MUL_TA )
    state -> ta_multiplier = composite_ta_multiplier();

  if ( flags & STATE_TGT_MUL_DA )
    state -> target_da_multiplier = composite_target_da_multiplier( state -> target );

  if ( flags & STATE_TGT_MUL_TA )
    state -> target_ta_multiplier = composite_target_ta_multiplier( state -> target );

  if ( flags & STATE_TGT_CRIT )
    state -> target_crit = composite_target_crit( state -> target ) * composite_crit_multiplier();

  if ( flags & STATE_TGT_MITG_DA )
    state -> target_mitigation_da_multiplier = composite_target_mitigation( state -> target, school );

  if ( flags & STATE_TGT_MITG_TA )
    state -> target_mitigation_ta_multiplier = composite_target_mitigation( state -> target, school );
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

double action_t::composite_target_crit( player_t* target )
{
  double c = 0.0;

  // "Crit chances of players against mobs that are higher level than you are reduced by 1% per level difference, in Mists."
  // Ghostcrawler on 20/6/2012 at http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97
  if ( ( target -> is_enemy() || target -> is_add() ) && ( target -> level > player -> level ) )
  {
    c -= 0.01 * ( target -> level - player -> level );
  }

  return c;
}

event_t* action_t::start_action_execute_event( timespan_t t, action_state_t* execute_event )
{
  return new ( *sim ) action_execute_event_t( this, t, execute_event );
}

void action_t::schedule_travel( action_state_t* s )
{
  time_to_travel = travel_time();

  if ( ! execute_state )
    execute_state = get_state();

  execute_state -> copy_state( s );

  if ( time_to_travel == timespan_t::zero() )
  {
    impact( s );
    action_state_t::release( s );
  }
  else
  {
    if ( sim -> log )
    {
      sim -> output( "[NEW] %s schedules travel (%.3f) for %s", player -> name(), time_to_travel.total_seconds(), name() );
    }

    add_travel_event( new ( *sim ) travel_event_t( this, s, time_to_travel ) );
  }
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
  else
  {
    if ( sim -> log )
    {
      sim -> output( "Target %s avoids %s %s (%s)", s -> target -> name(), player -> name(), name(), util::result_type_string( s -> result ) );
    }
  }
}

void action_t::trigger_dot( action_state_t* s )
{
  if ( num_ticks <= 0 && ! tick_zero )
    return;

  dot_t* dot = get_dot( s -> target );

  int remaining_ticks = dot -> num_ticks - dot -> current_tick;
  if ( dot_behavior == DOT_CLIP ) dot -> cancel();

  dot -> current_action = this;
  dot -> current_tick = 0;
  dot -> added_ticks = 0;
  dot -> added_seconds = timespan_t::zero();

  if ( ! dot -> state ) dot -> state = get_state();
  dot -> state -> copy_state( s );
  dot -> num_ticks = hasted_num_ticks( dot -> state -> haste );

  if ( dot -> ticking )
  {
    assert( dot -> tick_event );

    if ( dot_behavior == DOT_EXTEND ) dot -> num_ticks += std::min( ( int ) ( dot -> num_ticks / 2 ), remaining_ticks );

    // Recasting a dot while it's still ticking gives it an extra tick in total
    dot -> num_ticks++;

    // tick_zero dots tick again when reapplied
    if ( tick_zero )
    {
      // this is hacky, but otherwise uptime gets messed up
      timespan_t saved_tick_time = dot -> time_to_tick;
      dot -> time_to_tick = timespan_t::zero();
      tick( dot );
      dot -> time_to_tick = saved_tick_time;
    }
  }
  else
  {
    if ( school == SCHOOL_PHYSICAL )
    {
      buff_t* b = s -> target -> debuffs.bleeding;
      if ( b -> current_value > 0 )
      {
        b -> current_value += 1.0;
      }
      else b -> start( 1, 1.0 );
    }

    dot -> schedule_tick();
  }
  dot -> recalculate_ready();

  if ( sim -> debug )
    sim -> output( "%s extends dot-ready to %.2f for %s (%s)",
                   player -> name(), dot -> ready.total_seconds(), name(), dot -> name() );
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
